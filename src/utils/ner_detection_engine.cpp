/**
 * @file ner_detection_engine.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.18
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=4, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "utils/ner_detection_engine.h"
#include <algorithm>
#include <cctype>
#include <spdlog/spdlog.h>

namespace themis {
namespace utils {

// ============================================================================
// Construction
// ============================================================================

NERDetectionEngine::NERDetectionEngine()
    : enabled_(false)
    , model_available_(false)  // Initially model is unavailable until loaded
    , min_confidence_(0.70)
    , default_redaction_mode_("strict") {

    signature_.engine_type = "ner";
    signature_.version = "1.0.0";
    signature_.signature_id = "embedded-ner-engine";
    signature_.signer = "Embedded Default";
    signature_.signed_at = "2026-02-22T00:00:00Z";
    signature_.cert_serial = "EMBEDDED";

    loadDefaults();
    model_available_ = true;  // Default gazetteers are now loaded
}

// ============================================================================
// IPIIDetectionEngine interface
// ============================================================================

std::string NERDetectionEngine::getName() const { return "ner"; }
std::string NERDetectionEngine::getVersion() const { return signature_.version; }

bool NERDetectionEngine::isEnabled() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return enabled_;
}

PluginSignature NERDetectionEngine::getSignature() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return signature_;
}

bool NERDetectionEngine::initialize(const nlohmann::json& config) {
    std::lock_guard<std::mutex> lock(mutex_);
    last_error_.clear();

    try {
        enabled_ = config.value("enabled", true);

        if (config.contains("version")) {
            signature_.version = config["version"].get<std::string>();
        }

        if (config.contains("signature")) {
            auto sig = config["signature"];
            signature_.config_hash  = sig.value("config_hash",  "");
            signature_.signature    = sig.value("signature",    "");
            signature_.signature_id = sig.value("signature_id", "");
            signature_.cert_serial  = sig.value("cert_serial",  "");
            signature_.signed_at    = sig.value("signed_at",    "");
            signature_.signer       = sig.value("signer",       "");
        }

        loadFromConfig(config);
        rebuildFieldHints();
        
        // Verify that model data (gazetteers) are actually loaded
        // If all gazetteers are empty after loading, mark model as unavailable
        if (honorifics_.empty() && org_suffixes_.empty() && location_prepositions_.empty()) {
            model_available_ = false;
            spdlog::warn("NERDetectionEngine: All gazetteers are empty after loading; model marked unavailable");
        } else {
            model_available_ = true;
        }

        spdlog::info("NERDetectionEngine: Initialized (model_available={}, honorifics={}, org_suffixes={}, "
                     "location_prepositions={})",
                     model_available_, honorifics_.size(), org_suffixes_.size(), location_prepositions_.size());
        return true;

    } catch (const std::exception& e) {
        model_available_ = false;
        last_error_ = std::string("Initialization failed: ") + e.what();
        spdlog::error("NERDetectionEngine: {} (model marked unavailable)", last_error_);
        return false;
    }
}

bool NERDetectionEngine::reload(const nlohmann::json& config) {
    std::lock_guard<std::mutex> lock(mutex_);

    // Save current state for rollback
    auto old_honorifics          = honorifics_;
    auto old_org_suffixes        = org_suffixes_;
    auto old_location_prep       = location_prepositions_;
    auto old_field_hints         = field_name_hints_;
    auto old_redaction_modes     = redaction_modes_;

    last_error_.clear();

    try {
        if (!loadFromConfig(config)) {
            honorifics_            = old_honorifics;
            org_suffixes_          = old_org_suffixes;
            location_prepositions_ = old_location_prep;
            field_name_hints_      = old_field_hints;
            redaction_modes_       = old_redaction_modes;
            spdlog::error("NERDetectionEngine: Reload failed, retained previous config");
            return false;
        }

        rebuildFieldHints();
        spdlog::info("NERDetectionEngine: Reloaded successfully");
        return true;

    } catch (const std::exception& e) {
        honorifics_            = old_honorifics;
        org_suffixes_          = old_org_suffixes;
        location_prepositions_ = old_location_prep;
        field_name_hints_      = old_field_hints;
        redaction_modes_       = old_redaction_modes;
        last_error_ = std::string("Reload failed: ") + e.what();
        spdlog::error("NERDetectionEngine: {}", last_error_);
        return false;
    }
}

std::vector<PIIFinding> NERDetectionEngine::detectInText(const std::string& text) const {
    std::lock_guard<std::mutex> lock(mutex_);

    if (!enabled_ || text.empty()) {
        return {};
    }
    
    // Fail-closed: throw when model is unavailable so the caller (e.g.
    // PIIStreamScanner) can catch the exception and emit a sentinel finding
    // that blocks data from passing through unscanned.  Returning an empty
    // vector would be fail-open (silent bypass).
    if (!model_available_) {
        spdlog::warn("NERDetectionEngine: detectInText called but model is unavailable – throwing (fail-closed)");
        throw std::runtime_error("NERDetectionEngine: model is unavailable");
    }

    auto tokens = tokenise(text);
    std::vector<PIIFinding> findings;

    detectPersonNames(tokens, findings);
    detectOrganizations(tokens, findings);
    detectLocations(tokens, findings);

    // Sort by start_offset
    std::sort(findings.begin(), findings.end(),
              [](const PIIFinding& a, const PIIFinding& b) {
                  return a.start_offset < b.start_offset;
              });

    return findings;
}

PIIType NERDetectionEngine::classifyFieldName(const std::string& field_name) const {
    std::lock_guard<std::mutex> lock(mutex_);

    std::string lower = toLower(field_name);

    auto it = field_name_hints_.find(lower);
    if (it != field_name_hints_.end()) {
        return it->second;
    }

    // Partial / substring match
    for (const auto& [hint, type] : field_name_hints_) {
        if (lower.find(hint) != std::string::npos) {
            return type;
        }
    }

    return PIIType::UNKNOWN;
}

std::string NERDetectionEngine::getRedactionRecommendation(PIIType type) const {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = redaction_modes_.find(type);
    if (it != redaction_modes_.end()) {
        return it->second;
    }
    return default_redaction_mode_;
}

std::string NERDetectionEngine::getLastError() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return last_error_;
}

nlohmann::json NERDetectionEngine::getMetadata() const {
    std::lock_guard<std::mutex> lock(mutex_);

    nlohmann::json meta;
    meta["engine_type"]            = "ner";
    meta["version"]                = signature_.version;
    meta["enabled"]                = enabled_;
    meta["honorific_count"]        = honorifics_.size();
    meta["org_suffix_count"]       = org_suffixes_.size();
    meta["location_prep_count"]    = location_prepositions_.size();
    meta["signature_id"]           = signature_.signature_id;
    meta["signer"]                 = signature_.signer;
    meta["signed_at"]              = signature_.signed_at;
    return meta;
}

// ============================================================================
// Internal helpers
// ============================================================================

void NERDetectionEngine::loadDefaults() {
    honorifics_ = {
        "mr.", "mrs.", "ms.", "miss", "dr.", "prof.", "sir", "dame",
        "rev.", "capt.", "col.", "gen.", "sgt.", "cpl.", "pvt.",
        "pres.", "sen.", "rep.", "gov.", "amb.", "sec."
    };

    org_suffixes_ = {
        "inc.", "corp.", "ltd.", "llc", "llp", "plc", "gmbh", "ag",
        "sa", "nv", "bv", "oy", "ab", "as", "spa", "srl",
        "inc", "corp", "ltd", "foundation", "institute", "authority",
        "group", "holdings", "partners", "associates", "solutions",
        "technologies", "systems", "services", "enterprises", "international"
    };

    location_prepositions_ = {
        "in", "at", "from", "near", "to", "via", "through",
        "across", "over", "under", "around", "between"
    };

    redaction_modes_[PIIType::PERSON_NAME]   = "strict";
    redaction_modes_[PIIType::ORGANIZATION]  = "partial";
    redaction_modes_[PIIType::LOCATION]      = "partial";
}

bool NERDetectionEngine::loadFromConfig(const nlohmann::json& config) {
    if (config.contains("settings")) {
        auto& s = config["settings"];
        min_confidence_        = s.value("min_confidence",        min_confidence_);
        default_redaction_mode_ = s.value("default_redaction_mode", default_redaction_mode_);
    }

    // Override honorifics if provided
    if (config.contains("honorifics") && config["honorifics"].is_array()) {
        honorifics_.clear();
        for (const auto& h : config["honorifics"]) {
            honorifics_.insert(toLower(h.get<std::string>()));
        }
    }

    // Override org_suffixes if provided
    if (config.contains("org_suffixes") && config["org_suffixes"].is_array()) {
        org_suffixes_.clear();
        for (const auto& s : config["org_suffixes"]) {
            org_suffixes_.insert(toLower(s.get<std::string>()));
        }
    }

    // Override location_prepositions if provided
    if (config.contains("location_prepositions") && config["location_prepositions"].is_array()) {
        location_prepositions_.clear();
        for (const auto& p : config["location_prepositions"]) {
            location_prepositions_.insert(toLower(p.get<std::string>()));
        }
    }

    // Load per-type redaction modes if provided
    if (config.contains("redaction_modes") && config["redaction_modes"].is_object()) {
        for (auto& [key, val] : config["redaction_modes"].items()) {
            PIIType type = PIITypeUtils::fromString(key);
            if (type != PIIType::UNKNOWN) {
                redaction_modes_[type] = val.get<std::string>();
            }
        }
    }

    return true;
}

void NERDetectionEngine::rebuildFieldHints() {
    field_name_hints_.clear();

    // Person name field hints
    for (const auto& hint : {"name", "full_name", "fullname", "person", "contact_name",
                              "first_name", "last_name", "given_name", "surname",
                              "firstname", "lastname", "author", "employee", "patient",
                              "customer_name", "recipient", "sender"}) {
        field_name_hints_[hint] = PIIType::PERSON_NAME;
    }

    // Organization field hints
    for (const auto& hint : {"company", "organization", "organisation", "employer",
                              "institution", "corp", "firm", "business", "vendor",
                              "supplier", "client_company", "org"}) {
        field_name_hints_[hint] = PIIType::ORGANIZATION;
    }

    // Location field hints
    for (const auto& hint : {"address", "city", "country", "location", "region",
                              "state", "province", "zip", "postal_code", "street",
                              "hometown", "birthplace", "residence", "place"}) {
        field_name_hints_[hint] = PIIType::LOCATION;
    }
}

// ============================================================================
// Tokeniser
// ============================================================================

std::vector<NERDetectionEngine::Token> NERDetectionEngine::tokenise(const std::string& text) {
    std::vector<Token> tokens;
    size_t i = 0;
    const size_t n = text.size();

    while (i < n) {
        // Skip whitespace
        if (std::isspace(static_cast<unsigned char>(text[i]))) {
            ++i;
            continue;
        }

        // Collect a word token: any run of non-whitespace characters
        size_t start = i;
        while (i < n && !std::isspace(static_cast<unsigned char>(text[i]))) {
            ++i;
        }

        Token t;
        t.text   = text.substr(start, i - start);
        t.offset = start;
        tokens.push_back(std::move(t));
    }

    return tokens;
}

// ============================================================================
// Detection passes
// ============================================================================

void NERDetectionEngine::detectPersonNames(
    const std::vector<Token>& tokens,
    std::vector<PIIFinding>& out) const {

    const size_t n = tokens.size();

    for (size_t i = 0; i < n; ++i) {
        std::string lower_tok = toLower(tokens[i].text);

        // Check if this token is an honorific
        if (honorifics_.count(lower_tok) == 0) {
            continue;
        }

        // Collect following capitalised name tokens (1–3 words)
        size_t name_start = i + 1;
        size_t name_end   = name_start;

        while (name_end < n && (name_end - name_start) < 3) {
            const std::string& word = tokens[name_end].text;
            // Stop at punctuation-only tokens or non-capitalised words
            if (word.empty() || !isCapitalized(word)) {
                break;
            }
            ++name_end;
        }

        if (name_end == name_start) {
            // No name tokens followed the honorific; skip
            continue;
        }

        double confidence = (name_end - name_start >= 2) ? 0.92 : 0.80;
        if (confidence < min_confidence_) continue;

        // Span includes the honorific prefix
        out.push_back(makeSpan(tokens, i, name_end - 1, PIIType::PERSON_NAME,
                               confidence, "PERSON_NAME_HONORIFIC"));
    }
}

void NERDetectionEngine::detectOrganizations(
    const std::vector<Token>& tokens,
    std::vector<PIIFinding>& out) const {

    const size_t n = tokens.size();

    for (size_t i = 0; i < n; ++i) {
        std::string lower_tok = toLower(tokens[i].text);

        // Strip trailing punctuation for suffix lookup
        std::string stripped = lower_tok;
        while (!stripped.empty() && !std::isalnum(static_cast<unsigned char>(stripped.back()))) {
            stripped.pop_back();
        }
        // Add period back for suffix check (e.g. "Inc." stored as "inc.")
        std::string with_period = stripped + ".";

        bool is_suffix = (org_suffixes_.count(lower_tok) > 0 ||
                          org_suffixes_.count(stripped)   > 0 ||
                          org_suffixes_.count(with_period) > 0);

        if (!is_suffix) continue;

        // Look backwards for capitalised name tokens (1–4 words)
        size_t org_start = i;
        size_t look = (i > 0) ? i - 1 : i;
        size_t window = 0;

        while (window < 4 && look < n) { // look < n guards unsigned underflow
            if (!isCapitalized(tokens[look].text)) break;
            org_start = look;
            ++window;
            if (look == 0) break;
            --look;
        }

        if (org_start == i) {
            // No capitalised prefix found; still emit with lower confidence
            double conf = 0.65;
            if (conf < min_confidence_) continue;
            out.push_back(makeSpan(tokens, i, i, PIIType::ORGANIZATION,
                                   conf, "ORG_SUFFIX_ONLY"));
            continue;
        }

        double confidence = (window >= 2) ? 0.88 : 0.75;
        if (confidence < min_confidence_) continue;

        out.push_back(makeSpan(tokens, org_start, i, PIIType::ORGANIZATION,
                               confidence, "ORG_SUFFIX_WITH_PREFIX"));
    }
}

void NERDetectionEngine::detectLocations(
    const std::vector<Token>& tokens,
    std::vector<PIIFinding>& out) const {

    const size_t n = tokens.size();

    for (size_t i = 0; i + 1 < n; ++i) {
        std::string lower_tok = toLower(tokens[i].text);

        // Strip trailing punctuation/comma for preposition check
        std::string stripped = lower_tok;
        while (!stripped.empty() && !std::isalpha(static_cast<unsigned char>(stripped.back()))) {
            stripped.pop_back();
        }

        if (location_prepositions_.count(stripped) == 0) continue;

        // Collect following capitalised tokens (1–3 words)
        size_t loc_start = i + 1;
        size_t loc_end   = loc_start;

        while (loc_end < n && (loc_end - loc_start) < 3) {
            const std::string& word = tokens[loc_end].text;
            if (word.empty() || !isCapitalized(word)) break;
            ++loc_end;
        }

        if (loc_end == loc_start) continue;

        double confidence = (loc_end - loc_start >= 2) ? 0.82 : 0.72;
        if (confidence < min_confidence_) continue;

        out.push_back(makeSpan(tokens, loc_start, loc_end - 1,
                               PIIType::LOCATION, confidence, "LOCATION_PREPOSITION"));
    }
}

// ============================================================================
// Static helpers
// ============================================================================

bool NERDetectionEngine::isCapitalized(const std::string& word) {
    if (word.empty()) return false;
    // First alphabetic character must be uppercase
    for (unsigned char c : word) {
        if (std::isalpha(c)) {
            return std::isupper(c) != 0;
        }
    }
    return false;
}

std::string NERDetectionEngine::toLower(const std::string& s) {
    std::string out = s;
    std::transform(out.begin(), out.end(), out.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return out;
}

PIIFinding NERDetectionEngine::makeSpan(
    const std::vector<Token>& tokens,
    size_t first,
    size_t last,
    PIIType type,
    double confidence,
    const std::string& pattern_name) {

    PIIFinding f;
    f.type         = type;
    f.confidence   = confidence;
    f.pattern_name = pattern_name;
    f.engine_name  = "ner";
    f.start_offset = tokens[first].offset;

    // end_offset = offset after the last character of the last token
    const Token& last_tok = tokens[last];
    f.end_offset = last_tok.offset + last_tok.text.size();

    // Build value by joining tokens
    std::string value;
    for (size_t i = first; i <= last; ++i) {
        if (i > first) value += ' ';
        value += tokens[i].text;
    }
    f.value = std::move(value);

    return f;
}

// ============================================================================
// Factory function (registered in pii_detection_engine.cpp)
// ============================================================================

std::unique_ptr<IPIIDetectionEngine> createNEREngine() {
    return std::make_unique<NERDetectionEngine>();
}

} // namespace utils
} // namespace themis
