/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            voice_intent_detector.h                            ║
  Version:         0.0.39                                             ║
  Last Modified:   2026-04-15 05:39:29                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     135                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
    • 28a4b23b94  2026-02-23  Refactor tests and update error handling ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

// Intent detection and NER for Phase 3 LLM Integration
#pragma once
#include <string>
#include <vector>
#include <map>
#include <optional>
#include <nlohmann/json.hpp>

namespace themis { namespace voice {
using json = nlohmann::json;

// Intent categories
enum class IntentCategory {
    QUERY,           // Data retrieval
    COMMAND,         // Modify data
    QUESTION,        // Help/explanation
    CONVERSATION,    // Small talk
    UNKNOWN
};

std::string intentToString(IntentCategory cat);
IntentCategory stringToIntent(const std::string& s);

// Named entity
struct NamedEntity {
    std::string text;
    std::string type;   // DATE, PERSON, ORGANIZATION, NUMBER, METRIC, OBJECT, TIME_RANGE
    float confidence = 0.0f;
    int start_offset = 0;
    int end_offset = 0;
};

// Intent detection result
struct IntentResult {
    IntentCategory intent = IntentCategory::UNKNOWN;
    float confidence = 0.0f;
    std::vector<NamedEntity> entities;
    std::string normalized_query;
    json context_hints;         // Suggested context keys to resolve
    bool requires_context = false;
};

// Context manager for multi-turn conversations
class ConversationContext {
public:
    ConversationContext() = default;
    explicit ConversationContext(size_t max_history);

    void addTurn(const std::string& user_input, const std::string& assistant_response);
    void setEntity(const std::string& key, const std::string& value);
    std::optional<std::string> getEntity(const std::string& key) const;
    void clearEntities();

    const std::vector<std::pair<std::string, std::string>>& getHistory() const;
    std::string buildContextString(size_t max_turns = 5) const;
    void clear();

    size_t turnCount() const;
    bool hasEntity(const std::string& key) const;

private:
    std::vector<std::pair<std::string, std::string>> history_;
    std::map<std::string, std::string> entities_;
    size_t max_history_ = 20;
};

// Configuration
struct IntentDetectorConfig {
    float min_confidence_threshold = 0.5f;
    float entity_confidence_threshold = 0.4f;
    bool use_llm_for_ambiguous = false;  // Requires LLM to be available
    size_t max_context_turns = 10;
    std::vector<std::string> custom_entity_patterns;
};

// VoiceIntentDetector: Phase 3 production component
class VoiceIntentDetector {
public:
    explicit VoiceIntentDetector(const IntentDetectorConfig& config = {});
    ~VoiceIntentDetector() = default;

    // Detect intent from text
    IntentResult detect(const std::string& text, const ConversationContext* context = nullptr);

    // Extract named entities from text
    std::vector<NamedEntity> extractEntities(const std::string& text);

    // Classify intent category
    IntentCategory classifyIntent(const std::string& text);

    // Check if confidence meets threshold
    bool meetsThreshold(float confidence) const;

    // Normalize query (expand abbreviations, resolve pronouns with context)
    std::string normalizeQuery(const std::string& text, const ConversationContext* context = nullptr);

    // Statistics
    json getStatistics() const;

private:
    IntentDetectorConfig config_;
    uint64_t detections_total_ = 0;
    uint64_t high_confidence_ = 0;
    uint64_t low_confidence_ = 0;

    std::vector<NamedEntity> extractDateEntities(const std::string& text) const;
    std::vector<NamedEntity> extractNumberEntities(const std::string& text) const;
    std::vector<NamedEntity> extractMetricEntities(const std::string& text) const;
    float computeIntentConfidence(const std::string& text, IntentCategory cat) const;
};

}} // namespace themis::voice
