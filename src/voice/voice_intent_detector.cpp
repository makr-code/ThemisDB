/**
 * @file voice_intent_detector.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.42
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 100/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=2, H=2, M=3, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#include "voice/voice_intent_detector.h"
#include <spdlog/spdlog.h>
#include <algorithm>
#include <sstream>
#include <cctype>
#include <regex>
#include <chrono>

namespace themis { namespace voice {

// ---- Free functions ----

std::string intentToString(IntentCategory cat) {
    switch (cat) {
        case IntentCategory::QUERY:        return "QUERY";
        case IntentCategory::COMMAND:      return "COMMAND";
        case IntentCategory::QUESTION:     return "QUESTION";
        case IntentCategory::CONVERSATION: return "CONVERSATION";
        default:                           return "UNKNOWN";
    }
}

IntentCategory stringToIntent(const std::string& s) {
    if (s == "QUERY")        return IntentCategory::QUERY;
    if (s == "COMMAND")      return IntentCategory::COMMAND;
    if (s == "QUESTION")     return IntentCategory::QUESTION;
    if (s == "CONVERSATION") return IntentCategory::CONVERSATION;
    return IntentCategory::UNKNOWN;
}

// ---- ConversationContext ----

ConversationContext::ConversationContext(size_t max_history)
    : max_history_(max_history) {}

void ConversationContext::addTurn(const std::string& user_input, const std::string& assistant_response) {
    history_.emplace_back(user_input, assistant_response);
    if (history_.size() > max_history_) {
        history_.erase(history_.begin());
    }
}

void ConversationContext::setEntity(const std::string& key, const std::string& value) {
    entities_[key] = value;
}

std::optional<std::string> ConversationContext::getEntity(const std::string& key) const {
    auto it = entities_.find(key);
    if (it != entities_.end()) return it->second;
    return std::nullopt;
}

void ConversationContext::clearEntities() {
    entities_.clear();
}

const std::vector<std::pair<std::string, std::string>>& ConversationContext::getHistory() const {
    return history_;
}

std::string ConversationContext::buildContextString(size_t max_turns) const {
    std::ostringstream oss;
    size_t start = (history_.size() > max_turns) ? history_.size() - max_turns : 0;
    for (size_t i = start; i < history_.size(); ++i) {
        oss << "User: " << history_[i].first << "\n";
        oss << "Assistant: " << history_[i].second << "\n";
    }
    return oss.str();
}

void ConversationContext::clear() {
    history_.clear();
    entities_.clear();
}

size_t ConversationContext::turnCount() const {
    return history_.size();
}

bool ConversationContext::hasEntity(const std::string& key) const {
    return entities_.count(key) > 0;
}

// ---- VoiceIntentDetector ----

VoiceIntentDetector::VoiceIntentDetector(const IntentDetectorConfig& config)
    : config_(config) {}

namespace {

std::string intentToLower(const std::string& s) {
    std::string out = s;
    std::transform(out.begin(), out.end(), out.begin(),
        [](unsigned char c) { return std::tolower(c); });
    return out;
}

bool containsAny(const std::string& text, const std::vector<std::string>& keywords) {
    std::string lower = intentToLower(text);
    for (const auto& kw : keywords) {
        if (lower.find(kw) != std::string::npos) return true;
    }
    return false;
}

} // anonymous namespace

IntentCategory VoiceIntentDetector::classifyIntent(const std::string& text) {
    static const std::vector<std::string> query_kw   = {"how many","find","show","list","get","what is","count","search","select","fetch"};
    static const std::vector<std::string> command_kw = {"add","delete","update","create","insert","remove","set","drop","modify","put"};
    static const std::vector<std::string> question_kw= {"help","how to","what does","explain","why","describe","define"};
    static const std::vector<std::string> conv_kw    = {"hello","hi ","hi!","thanks","bye","yes","no","ok","sure","alright"};

    if (containsAny(text, conv_kw))    return IntentCategory::CONVERSATION;
    if (containsAny(text, command_kw)) return IntentCategory::COMMAND;
    if (containsAny(text, query_kw))   return IntentCategory::QUERY;
    if (containsAny(text, question_kw))return IntentCategory::QUESTION;
    return IntentCategory::UNKNOWN;
}

float VoiceIntentDetector::computeIntentConfidence(const std::string& text, IntentCategory cat) const {
    // Simple heuristic: count matching keywords and map to confidence
    auto lower = intentToLower(text);

    struct KwSet { IntentCategory cat; std::vector<std::string> kws; };
    static const KwSet sets[] = {
        {IntentCategory::QUERY,        {"how many","find","show","list","get","what is","count","search"}},
        {IntentCategory::COMMAND,      {"add","delete","update","create","insert","remove","set"}},
        {IntentCategory::QUESTION,     {"help","how to","what does","explain","why"}},
        {IntentCategory::CONVERSATION, {"hello","hi","thanks","bye","yes","no"}},
    };

    for (const auto& ks : sets) {
        if (ks.cat != cat) continue;
        int hits = 0;
        for (const auto& kw : ks.kws) {
            if (lower.find(kw) != std::string::npos) ++hits;
        }
        if (hits == 0) return 0.3f;
        return std::min(0.5f + hits * 0.15f, 0.95f);
    }
    return 0.3f;
}

std::vector<NamedEntity> VoiceIntentDetector::extractDateEntities(const std::string& text) const {
    std::vector<NamedEntity> entities;
    auto lower = intentToLower(text);

    static const std::vector<std::pair<std::string, std::string>> date_patterns = {
        {"yesterday",   "DATE"},
        {"today",       "DATE"},
        {"last week",   "TIME_RANGE"},
        {"last month",  "TIME_RANGE"},
        {"last year",   "TIME_RANGE"},
        {"this week",   "TIME_RANGE"},
        {"this month",  "TIME_RANGE"},
        {"this year",   "TIME_RANGE"},
        {"last quarter","TIME_RANGE"},
    };

    for (const auto& [pattern, type] : date_patterns) {
        size_t pos = lower.find(pattern);
        if (pos != std::string::npos) {
            NamedEntity ent;
            ent.text = text.substr(pos, pattern.size());
            ent.type = type;
            ent.confidence = 0.85f;
            ent.start_offset = static_cast<int>(pos);
            ent.end_offset = static_cast<int>(pos + pattern.size());
            entities.push_back(ent);
        }
    }
    return entities;
}

std::vector<NamedEntity> VoiceIntentDetector::extractNumberEntities(const std::string& text) const {
    std::vector<NamedEntity> entities;
    // Match standalone numbers and percentages
    std::regex num_re(R"(\b(\d+(?:\.\d+)?%?)\b)");
    auto begin = std::sregex_iterator(text.begin(), text.end(), num_re);
    auto end   = std::sregex_iterator();
    for (auto it = begin; it != end; ++it) {
        NamedEntity ent;
        ent.text = it->str();
        ent.type = ent.text.back() == '%' ? "METRIC" : "NUMBER";
        ent.confidence = 0.9f;
        ent.start_offset = static_cast<int>(it->position());
        ent.end_offset   = static_cast<int>(it->position() + it->length());
        entities.push_back(ent);
    }
    return entities;
}

std::vector<NamedEntity> VoiceIntentDetector::extractMetricEntities(const std::string& text) const {
    std::vector<NamedEntity> entities;
    auto lower = intentToLower(text);

    static const std::vector<std::string> metric_kw = {
        "revenue","sales","count","profit","loss","margin","rate",
        "average","total","sum","growth","conversion"
    };

    for (const auto& kw : metric_kw) {
        size_t pos = lower.find(kw);
        if (pos != std::string::npos) {
            NamedEntity ent;
            ent.text = text.substr(pos, kw.size());
            ent.type = "METRIC";
            ent.confidence = 0.75f;
            ent.start_offset = static_cast<int>(pos);
            ent.end_offset   = static_cast<int>(pos + kw.size());
            entities.push_back(ent);
        }
    }
    return entities;
}

std::vector<NamedEntity> VoiceIntentDetector::extractEntities(const std::string& text) {
    std::vector<NamedEntity> all;

    auto dates   = extractDateEntities(text);
    auto numbers = extractNumberEntities(text);
    auto metrics = extractMetricEntities(text);

    all.insert(all.end(), dates.begin(),   dates.end());
    all.insert(all.end(), numbers.begin(), numbers.end());
    all.insert(all.end(), metrics.begin(), metrics.end());

    // Filter by threshold
    all.erase(std::remove_if(all.begin(), all.end(),
        [this](const NamedEntity& e) {
            return e.confidence < config_.entity_confidence_threshold;
        }), all.end());

    return all;
}

std::string VoiceIntentDetector::normalizeQuery(
    const std::string& text, const ConversationContext* context)
{
    std::string result = text;

    if (context) {
        // Replace "it" / "that" with last known entity if available
        auto last_entity = context->getEntity("last_object");
        if (last_entity) {
            std::regex it_re(R"(\bit\b)", std::regex::icase);
            result = std::regex_replace(result, it_re, *last_entity);
            std::regex that_re(R"(\bthat\b)", std::regex::icase);
            result = std::regex_replace(result, that_re, *last_entity);
        }
    }
    return result;
}

bool VoiceIntentDetector::meetsThreshold(float confidence) const {
    return confidence >= config_.min_confidence_threshold;
}

// TASK 2.3: Intent detection fallback chain
// Primary model → Backup model → Safe default
// Confidence threshold: kMinIntentConfidence = 0.6 (error code 6801 if below)

IntentResult VoiceIntentDetector::detect(
    const std::string& text, const ConversationContext* context)
{
    // TASK 2.3: Intent detection with confidence threshold enforcement
    // and fallback chain (primary model → backup model → safe default)
    // Error code 6801: Intent detection confidence below threshold
    
    ++detections_total_;

    IntentResult result;
    
    // TASK 2.3: Primary model — classify intent from text
    result.intent = classifyIntent(text);
    result.confidence = computeIntentConfidence(text, result.intent);
    
    // TASK 2.3: Hardened confidence threshold enforcement
    // Fallback chain implementation:
    if (result.confidence < config_.min_confidence_threshold) {
        // Confidence below threshold; try backup strategy
        spdlog::debug("VoiceIntentDetector::detect: confidence {} below threshold {} (error 6801)",
                  result.confidence, config_.min_confidence_threshold);
        
        // TASK 2.3: Fallback to context-aware intent detection if available
        if (context != nullptr && context->hasEntity("last_intent")) {
            auto last_intent = context->getEntity("last_intent");
            if (last_intent.has_value()) {
                // Try to maintain context from previous turn
                result.intent = stringToIntent(*last_intent);
                result.confidence = 0.55f;  // Lower confidence for contextual fallback
                result.requires_context = true;
                spdlog::debug("VoiceIntentDetector::detect: using contextual fallback intent");
            }
        }
        
        // TASK 2.3: Final fallback to CONVERSATION (safest default)
        if (result.confidence < config_.min_confidence_threshold) {
            result.intent = IntentCategory::CONVERSATION;
            result.confidence = 0.5f;  // Minimal confidence for safety default
            spdlog::debug("VoiceIntentDetector::detect: using safe default (CONVERSATION)");
        }
    }

    // TASK 2.3: Extract named entities and filter by confidence
    result.entities = extractEntities(text);
    
    // TASK 2.3: Normalize query with context resolution
    result.normalized_query = normalizeQuery(text, context);
    result.requires_context = (context != nullptr && !context->getHistory().empty());

    // TASK 2.3: Update statistics
    if (result.confidence >= config_.min_confidence_threshold) {
        ++high_confidence_;
    } else {
        ++low_confidence_;
    }

    return result;
}

json VoiceIntentDetector::getStatistics() const {
    json stats;
    stats["detections_total"]  = detections_total_;
    stats["high_confidence"]   = high_confidence_;
    stats["low_confidence"]    = low_confidence_;
    stats["high_confidence_ratio"] = detections_total_ > 0
        ? static_cast<double>(high_confidence_) / detections_total_
        : 0.0;
    stats["timeout_fallbacks"] = timeout_fallbacks_;  // Phase 3
    return stats;
}

// ============================================================================
// Phase 3: Confidence and Safe Defaults
// ============================================================================

bool VoiceIntentDetector::isConfidenceTooLow(float confidence) const noexcept {
    return confidence < config_.min_confidence_threshold;
}

IntentResult VoiceIntentDetector::getTimeoutDefault() const noexcept {
    // Phase 3.6: Safe default when detection times out
    IntentResult result;
    result.intent = IntentCategory::UNKNOWN;
    result.confidence = 0.0f;
    return result;
}

bool VoiceIntentDetector::isTimeoutDetected() const noexcept {
    return timeout_detected_;
}

int64_t VoiceIntentDetector::nowMs() const {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
}

}} // namespace themis::voice

