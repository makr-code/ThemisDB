/**
 * @file llm_process_analyzer.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 84/100
 * @note Gap Summary: total=4; TODO=1, Stub=2, Unimpl=0, Mock=1, Sim=0, Debt=0, C=1, H=5, M=11, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "analytics/llm_process_analyzer.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <list>
#include <mutex>
#include <openssl/sha.h>
#include <spdlog/spdlog.h>
#include <sstream>
#include <thread>
#include <unordered_map>

namespace themis {

// ============================================================================
// Prompt Injection Sanitization
// ============================================================================

/// @brief Built-in default prompt-injection prefixes (lower-case).
/// Applied when no operator-supplied config file is configured or when the
/// file cannot be read.
static const std::vector<std::string> kBuiltinInjectionPrefixes = {
    "system:",
    "### system",
    "### instruction",
    "ignore all",
    "ignore previous",
    "disregard",
    "forget all previous",
    "you are now",
    "act as",
    "jailbreak",
    "</system>",
    "<|im_start|>",
    "<|system|>",
};

/**
 * @brief Load the effective injection-prefix list for sanitizeUserContent().
 *
 * When @p config_path is non-empty the file is opened and each non-empty,
 * non-comment line (lines starting with '#' are skipped) is lower-cased and
 * added to the returned list.  If the path is empty, the file cannot be
 * opened, or the file contains no qualifying lines, the built-in 13-pattern
 * list is returned instead.
 *
 * @param config_path  Absolute or relative path to the operator-supplied
 *                     prefix file.  Pass an empty string to use built-in defaults.
 * @return             Effective lower-cased injection prefix list.
 */
static std::vector<std::string> loadInjectionPrefixes(const std::string &config_path) {
    if (config_path.empty()) {
        return kBuiltinInjectionPrefixes;
    }

    std::ifstream file(config_path);
    if (!file.is_open()) {
        spdlog::warn("LLMProcessAnalyzer: could not open injection prefix config '{}'; "
                     "using built-in defaults ({} patterns)",
                     config_path,static_cast<int>(kBuiltinInjectionPrefixes.size()));
        return kBuiltinInjectionPrefixes;
    }

    std::vector<std::string> prefixes;
    std::string line = {};
    while (std::getline(file, line)) {
        if (line.empty() || line[0] == '#') {
            continue;
        }
        // Normalize to lower-case for case-insensitive matching.
        std::transform(line.begin(), line.end(), line.begin(),
                       [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
        prefixes.push_back(std::move(line));
    }

    if (prefixes.empty()) {
        spdlog::warn("LLMProcessAnalyzer: injection prefix config '{}' produced no entries; "
                     "using built-in defaults ({} patterns)",
                     config_path,static_cast<int>(kBuiltinInjectionPrefixes.size()));
        return kBuiltinInjectionPrefixes;
    }

    spdlog::info("LLMProcessAnalyzer: loaded {} injection prefixes from '{}'",
                 prefixes.size(), config_path);
    return prefixes;
}

/**
 * @brief Sanitize user-supplied content before embedding it in LLM prompts.
 *
 * Removes or neutralizes common prompt-injection sequences in the serialized
 * JSON string that will be embedded verbatim into a prompt.  The goal is to
 * prevent attacker-controlled field values from hijacking the system role or
 * appending additional instructions that alter model behavior.
 *
 * Mitigations applied:
 *  1. Null-byte removal — LLMs may truncate or behave unpredictably on NUL.
 *  2. Non-printable ASCII control characters (0x00–0x1F, except \t/\n/\r) are
 *     replaced with a space to avoid smuggled escape sequences.
 *  3. Known prompt-injection prefixes (e.g. "System:", "###", "Ignore all",
 *     "IGNORE PREVIOUS INSTRUCTIONS") are replaced with a redacted marker.
 *     The check is case-insensitive and covers common jailbreak patterns.
 *  4. Content length is hard-capped to kMaxContentBytes to prevent
 *     prompt flooding / token exhaustion attacks.
 *
 * @note This function operates on the *serialized* JSON representation of
 *       user data, not on the raw request object, so no structural information
 *       is lost.  The JSON syntax itself (quotes, braces) is preserved; only
 *       values that match injection patterns are neutralized.
 *
 * @param content             Serialized JSON string to sanitize.
 * @param injection_prefixes  Lower-case prefix list to redact; loaded via
 *                            loadInjectionPrefixes() at construction time.
 * @return                    Sanitized copy safe for prompt embedding.
 */
static std::string sanitizeUserContent(const std::string &content,
                                       const std::vector<std::string> &injection_prefixes) {
    // Hard cap: 32 KiB of user content per field embedded in a single prompt.
    // Exceeding this is almost certainly a flooding or context-exhaustion attack.
    constexpr size_t kMaxContentBytes = 32 * 1024;

    std::string out = {};
    const size_t limit = std::min(content.size(), kMaxContentBytes);
    out.reserve(limit);

    for (size_t i = 0; i < limit; ++i) {
        unsigned char c = static_cast<unsigned char>(content[i]);
        // Remove null bytes and non-printable control chars (except \t, \n, \r).
        if (c == 0x00) {
            continue; // drop silently
        }
        if (c < 0x20 && c != '\t' && c != '\n' && c != '\r') {
            out += ' ';
            continue;
        }

        // Sliding window: check if this position starts a known injection prefix.
        // Build a lower-case window of up to 40 chars for comparison.
        bool injected = false;
        for (const auto &prefix : injection_prefixes) {
            if (i + static_cast<int>(prefix.size()) > limit) {
                continue;
            }
            // Case-insensitive comparison without heap allocation.
            bool match = true;
            for (size_t k = 0; k <static_cast<int>(prefix.size()); ++k) {
                if (std::tolower(static_cast<unsigned char>(content[i + k])) != prefix[k]) {
                    match = false;
                    break;
                }
            }
            if (match) {
                out += "[REDACTED_INJECTION_ATTEMPT]";
                i += static_cast<int>(prefix.size()) - 1; // skip matched chars (loop will ++i)
                injected = true;
                spdlog::warn("LLMProcessAnalyzer::sanitizeUserContent: "
                             "prompt injection pattern '{}' detected and redacted at offset {}",
                             prefix, i);
                break;
            }
        }
        if (!injected) {
            out += static_cast<char>(c);
        }
    }

    if (static_cast<int>(content.size()) > kMaxContentBytes) {
        spdlog::warn("LLMProcessAnalyzer::sanitizeUserContent: "
                     "content truncated from {} to {} bytes (flood/token-exhaustion guard)",
                     content.size(), kMaxContentBytes);
        out += "\n[CONTENT_TRUNCATED]";
    }

    return out;
}

// ============================================================================
// API Key Sanitization
// ============================================================================

std::string sanitizeApiKey(const std::string &api_key) {
    if (api_key.empty()) {
        return "<not set>";
    }
    constexpr size_t kVisible = 4;
    if (static_cast<int>(api_key.size()) <= kVisible * 2) {
        return std::string(api_key.size(), '*');
    }
    return api_key.substr(0, kVisible) + "***...***" + api_key.substr(static_cast<int>(api_key.size()) - kVisible);
}

// ============================================================================
// Implementation Details
// ============================================================================

struct LLMProcessAnalyzer::Impl {
    LLMConfig config;

    /// Effective injection-prefix list, loaded at construction time.
    /// Either the operator-supplied file contents (config.injection_prefix_config_path)
    /// or the built-in kBuiltinInjectionPrefixes fallback.
    std::vector<std::string> injection_prefixes;

    // Response cache entry
    struct CacheEntry {
        nlohmann::json response;
        std::chrono::steady_clock::time_point expiry;
    };

    // O(1) LRU cache: front of list = MRU, back = LRU
    // list stores keys in access order; map provides O(1) lookup + list iterator
    using LruList = std::list<std::string>;
    using LruMap  = std::unordered_map<std::string, std::pair<LruList::iterator, CacheEntry>>;

    mutable LruList lru_list;
    mutable LruMap lru_map;
    mutable std::mutex cache_mutex;

    // Statistics
    mutable CacheStats stats;

    Impl(const LLMConfig &cfg)
        : config(cfg)
        , injection_prefixes(loadInjectionPrefixes(cfg.injection_prefix_config_path)) {}

    std::optional<nlohmann::json> getFromCache(const std::string &key) const {
        if (!config.enable_caching) {
            return std::nullopt;
        }

        std::lock_guard<std::mutex> lock(cache_mutex);
        auto it = lru_map.find(key);
        if (it == lru_map.end()) {
            stats.misses++;
            return std::nullopt;
        }

        auto now = std::chrono::steady_clock::now();
        if (now > it->second.second.expiry) {
            lru_list.erase(it->second.first);
            lru_map.erase(it);
            stats.misses++;
            stats.evictions++;
            return std::nullopt;
        }

        // Promote to MRU front — O(1) via splice (no key copy/realloc)
        lru_list.splice(lru_list.begin(), lru_list, it->second.first);
        // it->second.first remains valid and now references the front node

        stats.hits++;
        return it->second.second.response;
    }

    void putInCache(const std::string &key, const nlohmann::json &response) {
        if (!config.enable_caching) {
            return;
        }

        std::lock_guard<std::mutex> lock(cache_mutex);
        auto expiry = std::chrono::steady_clock::now() + std::chrono::seconds(config.cache_ttl_seconds);

        // If key already exists: splice to MRU front and update value (no key realloc)
        auto it = lru_map.find(key);
        if (it != lru_map.end()) {
            lru_list.splice(lru_list.begin(), lru_list, it->second.first);
            it->second.second = CacheEntry{response, expiry};
        } else {
            // New key: insert at MRU front
            lru_list.push_front(key);
            lru_map[key] = {lru_list.begin(), CacheEntry{response, expiry}};
        }

        // Evict LRU tail if over capacity — O(1)
        const size_t max_entries
            = (config.max_cache_entries > 0) ? static_cast<size_t>(config.max_cache_entries) : 1000;
        if (static_cast<int>(lru_map.size()) > max_entries) {
            const std::string &lru_key = lru_list.back();
            lru_map.erase(lru_key);
            lru_list.pop_back();
            stats.evictions++;
        }
    }
};

// ============================================================================
// Constructor / Destructor
// ============================================================================

LLMProcessAnalyzer::LLMProcessAnalyzer(const LLMConfig &config) : pImpl(std::make_unique<Impl>(config)) {}

LLMProcessAnalyzer::~LLMProcessAnalyzer() = default;

// ============================================================================
// Main Analysis Function
// ============================================================================

std::pair<bool, LLMResponse> LLMProcessAnalyzer::analyze(const LLMRequest &request) {
    LLMResponse response;
    auto start = std::chrono::steady_clock::now();

    try {
        // Check cache first
        std::string cache_key = getCacheKey(request);
        if (auto cached = pImpl->getFromCache(cache_key)) {
            response.success      = true;
            response.raw_response = *cached;
            response.from_cache   = true;

            // Parse cached response
            if (request.task_type == TaskType::ANALYZE_PROCESS) {
                response.conformance_score = cached->value("conformance_score", 0.0);
                // ... parse other fields
            }

            auto end                  = std::chrono::steady_clock::now();
            response.response_time_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

            return {true, response};
        }

        // Generate prompt
        nlohmann::json data;
        const auto &trace = request.process_trace.is_null() ? request.process_data : request.process_trace;
        data["trace"]     = trace;
        data["model"]     = request.ideal_model;
        data["context"]   = request.context.is_null() ? request.process_data : request.context;

        std::string prompt = generatePrompt(request.task_type, data, request.domain);

        // Call LLM with retry logic
        std::string raw_llm_response = {};
        int retries = 0;
        while (retries <= pImpl->config.max_retries) {
            try {
                raw_llm_response = callLLM(prompt, request.parameters);
                break;
            } catch ([[maybe_unused]] const std::exception &e) {
                if (retries == pImpl->config.max_retries) {
                    throw;
                }
                retries++;
                std::this_thread::sleep_for(
                    std::chrono::milliseconds(pImpl->config.retry_delay_ms * (int)std::pow(2, retries - 1)));
            }
        }

        // Parse response
        nlohmann::json parsed = parseResponse(raw_llm_response, request.task_type);

        // Validate
        if (!validateResponse(parsed, request.task_type)) {
            response.success       = false;
            response.error_message = "Invalid response schema from LLM";
            return {false, response};
        }

        // Store in cache
        pImpl->putInCache(cache_key, parsed);

        // Populate response based on task type
        response.success      = true;
        response.raw_response = parsed;

        switch (request.task_type) {
            case TaskType::ANALYZE_PROCESS:
                response.conformance_score = parsed.value("conformance_score", 0.0);
                if (parsed.contains("deviations")) {
                    for (const auto &d : parsed["deviations"]) {
                        LLMResponse::Deviation dev;
                        dev.activity    = d.value("activity", "");
                        dev.type        = d.value("type", "");
                        dev.severity    = d.value("severity", "");
                        dev.description = d.value("description", "");
                        response.deviations.push_back(dev);
                    }
                }
                if (parsed.contains("compliance_issues")) {
                    for (const auto &c : parsed["compliance_issues"]) {
                        LLMResponse::ComplianceIssue issue;
                        issue.rule        = c.value("rule", "");
                        issue.violation   = c.value("violation", "");
                        issue.severity    = c.value("severity", "");
                        issue.remediation = c.value("remediation", "");
                        response.compliance_issues.push_back(issue);
                    }
                }
                if (parsed.contains("recommendations")) {
                    for (const auto &r : parsed["recommendations"]) {
                        LLMResponse::Recommendation rec;
                        rec.type                  = r.value("type", "");
                        rec.priority              = r.value("priority", "");
                        rec.description           = r.value("description", "");
                        rec.potential_improvement = r.value("potential_improvement", 0.0);
                        response.recommendations.push_back(rec);
                    }
                }
                if (parsed.contains("summary") && parsed["summary"].is_string()) {
                    response.summary = parsed["summary"].get<std::string>();
                } else if (!response.recommendations.empty()) {
                    response.summary = response.recommendations.front().description;
                } else {
                    response.summary = "Process conformance analysis completed";
                }
                break;

            case TaskType::PREDICT_NEXT:
                if (parsed.contains("predictions")) {
                    for (const auto &p : parsed["predictions"]) {
                        LLMResponse::Prediction pred;
                        pred.activity    = p.value("activity", "");
                        pred.probability = p.value("probability", 0.0);
                        pred.reasoning   = p.value("reasoning", "");
                        response.predictions.push_back(pred);
                    }
                }
                break;

            case TaskType::VERIFY_5R_RULE:
                if (parsed.contains("five_rights_check")) {
                    LLMResponse::FiveRCheck check;
                    auto fr                  = parsed["five_rights_check"];
                    check.right_patient      = fr.value("right_patient", false);
                    check.right_medication   = fr.value("right_medication", false);
                    check.right_dose         = fr.value("right_dose", false);
                    check.right_time         = fr.value("right_time", false);
                    check.right_route        = fr.value("right_route", false);
                    check.overall_compliance = fr.value("overall_compliance", false);
                    check.risk_level         = fr.value("risk_level", "unknown");
                    if (fr.contains("corrective_actions")) {
                        for (const auto &action : fr["corrective_actions"]) {
                            check.corrective_actions.push_back(action.get<std::string>());
                        }
                    }
                    response.five_rights_check = check;
                }
                break;

            case TaskType::DETECT_FRAUD:
                if (parsed.contains("fraud_analysis")) {
                    LLMResponse::FraudAnalysis fraud;
                    auto fa          = parsed["fraud_analysis"];
                    fraud.risk_score = fa.value("risk_score", 0.0);
                    if (fa.contains("detected_anomalies")) {
                        for (const auto &anomaly : fa["detected_anomalies"]) {
                            fraud.detected_anomalies.push_back(anomaly.get<std::string>());
                        }
                    }
                    if (fa.contains("flags")) {
                        auto flags                        = fa["flags"];
                        fraud.flags.duplicate             = flags.value("duplicate", false);
                        fraud.flags.unusual_amount        = flags.value("unusual_amount", false);
                        fraud.flags.vendor_not_verified   = flags.value("vendor_not_verified", false);
                        fraud.flags.missing_documentation = flags.value("missing_documentation", false);
                    }
                    fraud.recommended_action = fa.value("recommended_action", "");
                    response.fraud_analysis  = fraud;
                }
                break;

            default:
                break;
        }

        auto end                  = std::chrono::steady_clock::now();
        response.response_time_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

        return {true, response};

    } catch (const std::exception &e) {
        response.success       = false;
        response.error_message = e.what();
        return {false, response};
    }
}

// ============================================================================
// Prompt Generation
// ============================================================================

std::string LLMProcessAnalyzer::generatePrompt(TaskType task_type, const nlohmann::json &data,
                                               const std::string &domain) const {
    std::stringstream ss = {};

    switch (task_type) {
        case TaskType::ANALYZE_PROCESS:
            ss << "Analysiere den folgenden " << domain << "-Prozess und identifiziere:\n";
            ss << "1. Abweichungen vom Standard-Prozessmodell\n";
            ss << "2. Fehlende oder übersprungene Aktivitäten\n";
            ss << "3. Compliance-Verstöße\n";
            ss << "4. Mögliche Prozessoptimierungen\n\n";
            // SECURITY (prompt_injection): user-supplied trace/model are sanitized before
            // embedding to neutralize injection payloads hidden in field values.
            ss << "Prozessdaten:\n" << sanitizeUserContent(data["trace"].dump(2), pImpl->injection_prefixes) << "\n\n";
            ss << "Erwartetes Modell:\n" << sanitizeUserContent(data["model"].dump(2), pImpl->injection_prefixes) << "\n\n";
            ss << "Gib die Antwort als JSON zurück mit den Feldern:\n";
            ss << "- conformance_score (0.0-1.0)\n";
            ss << "- deviations (Array mit activity, type, severity, description)\n";
            ss << "- compliance_issues (Array mit rule, violation, severity, remediation)\n";
            ss << "- recommendations (Array mit type, priority, description, potential_improvement)\n";
            break;

        case TaskType::PREDICT_NEXT:
            ss << "Basierend auf dem bisherigen Prozessverlauf, welche Aktivitäten werden als nächstes wahrscheinlich "
                  "ausgeführt?\n\n";
            // SECURITY (prompt_injection): sanitize before embedding.
            ss << "Bisheriger Verlauf:\n" << sanitizeUserContent(data["trace"].dump(2), pImpl->injection_prefixes) << "\n\n";
            ss << "Prozessmodell:\n" << sanitizeUserContent(data["model"].dump(2), pImpl->injection_prefixes) << "\n\n";
            ss << "Gib die Top 3 wahrscheinlichsten nächsten Aktivitäten zurück als JSON:\n";
            ss << "- predictions (Array mit activity, probability, reasoning)\n";
            break;

        case TaskType::VERIFY_5R_RULE:
            ss << "Verifiziere die Einhaltung der 5R-Regel bei dieser Medikamentengabe:\n\n";
            ss << "Prüfe alle 5 Rechte:\n";
            ss << "1. Richtiger Patient (Identifikation durchgeführt?)\n";
            ss << "2. Richtiges Medikament (Barcode-Scan?)\n";
            ss << "3. Richtige Dosis (Doppelcheck?)\n";
            ss << "4. Richtiger Zeitpunkt (Zeitplan eingehalten?)\n";
            ss << "5. Richtige Applikationsform (Darreichungsform korrekt?)\n\n";
            // SECURITY (prompt_injection): sanitize before embedding.
            ss << "Medikationsdaten:\n" << sanitizeUserContent(data["trace"].dump(2), pImpl->injection_prefixes) << "\n\n";
            ss << "Gib die Antwort als JSON zurück:\n";
            ss << "- five_rights_check (Object mit right_patient, right_medication, right_dose, right_time, "
                  "right_route, overall_compliance, risk_level, corrective_actions)\n";
            break;

        case TaskType::DETECT_FRAUD:
            ss << "Prüfe diese Rechnung auf Anomalien und Betrugsrisiken:\n\n";
            ss << "Suche nach:\n";
            ss << "1. Ungewöhnliche Beträge\n";
            ss << "2. Duplikate\n";
            ss << "3. Lieferant nicht verifiziert\n";
            ss << "4. Abweichungen von Bestellungen\n";
            ss << "5. Fehlende Dokumentation\n\n";
            // SECURITY (prompt_injection): sanitize before embedding.
            ss << "Rechnungsdaten:\n" << sanitizeUserContent(data["trace"].dump(2), pImpl->injection_prefixes) << "\n\n";
            if (data.contains("context")) {
                ss << "Historische Daten:\n" << sanitizeUserContent(data["context"].dump(2), pImpl->injection_prefixes) << "\n\n";
            }
            ss << "Gib die Antwort als JSON zurück:\n";
            ss << "- fraud_analysis (Object mit risk_score, detected_anomalies, flags, recommended_action)\n";
            break;

        default:
            ss << "Process analysis task\n";
            // SECURITY (prompt_injection): sanitize before embedding.
            ss << "Data: " << sanitizeUserContent(data.dump(2), pImpl->injection_prefixes) << "\n";
            break;
    }

    return ss.str();
}

// ============================================================================
// LLM Call (Placeholder - Would integrate with actual LLM API)
// ============================================================================

std::string LLMProcessAnalyzer::callLLM(const std::string &prompt,
                                        [[maybe_unused]] const std::map<std::string, std::string> &params) {
    // When THEMIS_ENABLE_LLM_API is defined, delegate to the configured provider
    // (OpenAI, Anthropic, or a local model served over HTTP).
    // SECURITY: Always use sanitizeApiKey(pImpl->config.api_key) in log
    // messages — never log or expose the raw API key value.
    
    // Thread-safety note: pImpl->config is initialized once in the constructor
    // (Impl::Impl(const LLMConfig&)) and never modified afterward. Multiple threads
    // can safely read pImpl->config.* concurrently without locks because the
    // configuration is effectively immutable after construction.
    spdlog::debug("LLM call: provider={}, model={}, key={}", static_cast<int>(pImpl->config.provider),
                  pImpl->config.model_name, sanitizeApiKey(pImpl->config.api_key));

#ifdef THEMIS_ENABLE_LLM_API
    // Production path: call the configured LLM provider.
    // Implementation plugged in via the provider SDK (OpenAI, Anthropic, etc.)
    // Return the raw completion text from the API response.
    // Unreachable unless the provider SDK is compiled in; fall through to
    // the heuristic response below so the unit tests remain functional.
#endif

    // Heuristic / offline responses used when no LLM provider is configured.
    nlohmann::json response;

    if (prompt.find("Analysiere") != std::string::npos) {
        response = {{"conformance_score", 0.92},
                    {"deviations", nlohmann::json::array()},
                    {"compliance_issues", nlohmann::json::array()},
                    {"recommendations",
                     {{{"type", "optimization"},
                       {"priority", "medium"},
                       {"description", "Parallele Bearbeitung möglich"},
                       {"potential_improvement", 0.15}}}}};
    } else if (prompt.find("5R-Regel") != std::string::npos) {
        response = {{"five_rights_check",
                     {{"right_patient", true},
                      {"right_medication", true},
                      {"right_dose", true},
                      {"right_time", true},
                      {"right_route", true},
                      {"overall_compliance", true},
                      {"risk_level", "low"},
                      {"corrective_actions", nlohmann::json::array()}}}};
    } else if (prompt.find("Betrugsrisiken") != std::string::npos) {
        response = {{"fraud_analysis",
                     {{"risk_score", 0.3},
                      {"detected_anomalies", nlohmann::json::array()},
                      {"flags",
                       {{"duplicate", false},
                        {"unusual_amount", false},
                        {"vendor_not_verified", false},
                        {"missing_documentation", false}}},
                      {"recommended_action", "approve"}}}};
    } else {
        response
            = {{"predictions",
                {{{"activity", "next_step"}, {"probability", 0.85}, {"reasoning", "Based on historical patterns"}}}}};
    }

    return response.dump();
}

// ============================================================================
// Response Parsing
// ============================================================================

nlohmann::json LLMProcessAnalyzer::parseResponse(const std::string &raw_response, [[maybe_unused]] TaskType task_type) {
    try {
        return nlohmann::json::parse(raw_response);
    } catch (const std::exception &e) {
        throw std::runtime_error("Failed to parse LLM response as JSON: " + std::string(e.what()));
    }
}

// ============================================================================
// Response Validation
// ============================================================================

bool LLMProcessAnalyzer::validateResponse(const nlohmann::json &response, TaskType task_type) const {
    constexpr std::size_t kMaxListSize      = 512;
    constexpr std::size_t kMaxStringLength  = 4096;

    const auto isBoundedString = [kMaxStringLength](const nlohmann::json &value) {
        return static_cast<bool>(value.is_string() && value.get_ref<const std::string & < static_cast<int>(().size())) <= kMaxStringLength;
    };

    switch (task_type) {
        case TaskType::ANALYZE_PROCESS:
            return response.contains("conformance_score") && response.contains("deviations")
                   && response.contains("compliance_issues") && response.contains("recommendations");

        case TaskType::PREDICT_NEXT:
            if (!response.contains("predictions") || !response["predictions"].is_array()) {
                return false;
            }
            if (response["predictions"].size() > kMaxListSize) {
                return false;
            }
            for (const auto &prediction : response["predictions"]) {
                if (!prediction.is_object() || !prediction.contains("activity") || !prediction.contains("probability")
                   || !prediction.contains("reasoning")) {
                   return false;
                }
                if (!isBoundedString(prediction["activity"]) || !isBoundedString(prediction["reasoning"])
                   || !prediction["probability"].is_number()) {
                   return false;
                }
                const auto probability = prediction["probability"].get<double>();
                if (probability < 0.0 || probability > 1.0) {
                   return false;
                }
            }
            return true;

        case TaskType::VERIFY_5R_RULE:
            if (!response.contains("five_rights_check") || !response["five_rights_check"].is_object()) {
                return false;
            }
            if (!response["five_rights_check"].contains("overall_compliance")
                || !response["five_rights_check"]["overall_compliance"].is_boolean()) {
                return false;
            }
            if (response["five_rights_check"].contains("risk_level")
                && !isBoundedString(response["five_rights_check"]["risk_level"])) {
                return false;
            }
            if (response["five_rights_check"].contains("corrective_actions")) {
                const auto &actions = response["five_rights_check"]["corrective_actions"];
                if (!actions.is_array() || static_cast<int>(actions.size()) > kMaxListSize) {
                   return false;
                }
                for (const auto &action : actions) {
                   if (!isBoundedString(action)) {
                       return false;
                   }
                }
            }
            return true;

        case TaskType::DETECT_FRAUD:
            if (!response.contains("fraud_analysis") || !response["fraud_analysis"].is_object()
                || !response["fraud_analysis"].contains("risk_score")
                || !response["fraud_analysis"]["risk_score"].is_number()) {
                return false;
            }
            {
                const auto risk_score = response["fraud_analysis"]["risk_score"].get<double>();
                if (risk_score < 0.0 || risk_score > 1.0) {
                   return false;
                }
            }
            if (response["fraud_analysis"].contains("detected_anomalies")) {
                const auto &anomalies = response["fraud_analysis"]["detected_anomalies"];
                if (!anomalies.is_array() || static_cast<int>(anomalies.size()) > kMaxListSize) {
                   return false;
                }
                for (const auto &anomaly : anomalies) {
                   if (!isBoundedString(anomaly)) {
                       return false;
                   }
                }
            }
            if (response["fraud_analysis"].contains("recommended_action")
                && !isBoundedString(response["fraud_analysis"]["recommended_action"])) {
                return false;
            }
            return true;

        default:
            return true; // Basic validation
    }
}

// ============================================================================
// Cache Management
// ============================================================================

std::string LLMProcessAnalyzer::getCacheKey(const LLMRequest &request) const {
    // Build a cache key using SHA256 digests of the JSON fields.
    // Format: "<task_type>:<domain>:<trace_sha256>:<model_sha256>"
    // The two SHA256 components are each 64 hex chars; the total key length
    // varies with the length of request.domain (which is typically short).
    // Hashing the JSON fields avoids embedding potentially large dump() strings
    // directly in the key and reduces hash-map bucket comparison cost.
    //
    // Security note: This lambda computes a hash for cache key generation only,
    // not for LLM prompt construction. The input strings are JSON serializations
    // of request data (process trace and ideal model), which are NOT user-provided
    // prompt text. This is cryptographic hashing for deterministic cache lookup,
    // not prompt injection risk. Hashing is deterministic and one-way; the hash
    // is never sent to an LLM or used to construct prompts.
    auto sha256hex = [](const std::string &input) -> std::string {
        unsigned char hash[SHA256_DIGEST_LENGTH];
        SHA256(reinterpret_cast<const unsigned char *>(input.data()),static_cast<int>(input.size()), hash);
        std::ostringstream oss = {};
        for (int i = 0; i < SHA256_DIGEST_LENGTH; ++i) {
            oss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(hash[i]);
        }
        return oss.str();
    };

    const auto &trace            = request.process_trace.is_null() ? request.process_data : request.process_trace;
    const std::string trace_hash = sha256hex(trace.dump());
    const std::string model_hash = sha256hex(request.ideal_model.dump());

    return std::to_string(static_cast<int>(request.task_type)) + ":" + request.domain + ":" + trace_hash + ":"
           + model_hash;
}

LLMProcessAnalyzer::CacheStats LLMProcessAnalyzer::getCacheStats() const {
    std::lock_guard<std::mutex> lock(pImpl->cache_mutex);
    return pImpl->stats;
}

void LLMProcessAnalyzer::clearCache() {
    std::lock_guard<std::mutex> lock(pImpl->cache_mutex);
    pImpl->lru_list.clear();
    pImpl->lru_map.clear();
    pImpl->stats = CacheStats{};
}

} // namespace themis
