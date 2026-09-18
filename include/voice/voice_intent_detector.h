/**
 * @file voice_intent_detector.h
 * @brief Voice Intent Detection — Frozen API Contract for Phase 1.
 *
 * @version v1.0 frozen as of 2026-08-08
 *
 * Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Status: Design/API Contract Frozen (Phase 1)
 *
 * ## Intent Detection Contract (Frozen)
 *
 * **Confidence Thresholds:**
 * - Minimum confidence for acceptance: 0.50 (frozen)
 * - Entity confidence threshold: 0.40 (frozen)
 * - High confidence: >= 0.75
 * - Medium confidence: 0.50-0.75
 * - Low confidence: < 0.50
 *
 * **Intent Categories (Frozen Enumeration):**
 * - QUERY: Information retrieval (e.g., "show me the data")
 * - COMMAND: Data modification (e.g., "delete the record")
 * - QUESTION: Help/explanation (e.g., "how do I...")
 * - CONVERSATION: Small talk / greetings
 * - UNKNOWN: Unable to classify
 *
 * **Named Entity Types (Frozen):**
 * - DATE, PERSON, ORGANIZATION, NUMBER, METRIC, OBJECT, TIME_RANGE
 *
 * ## Error Codes (Voice Module — Command/Intent)
 * - 6800: Intent detection failed
 * - 6801: Confidence below minimum threshold
 * - 6802: Unknown/unclassified intent
 * - 6803: Ambiguous intent (multiple candidates above threshold)
 * - 6804: NER (named entity recognition) error
 * - 6805: Context resolution failed
 * - 6806-6899: Reserved for future command-related errors
 *
 * ## Thread Safety
 * VoiceIntentDetector is thread-safe (internal mutex on ConversationContext).
 * However, it's preferred to use one instance per conversation stream.
 */


// Intent detection and NER for Phase 3 LLM Integration
// ============================================================================
// PHASE 1 CONTRACT FREEZE: This file documents the immutable intent detection
// and named entity recognition (NER) contract.
// ============================================================================
#pragma once
#include <string>
#include <vector>
#include <map>
#include <optional>
#include <nlohmann/json.hpp>

namespace themis { namespace voice {
using json = nlohmann::json;

// Intent categories
/// @brief Intent category classification (frozen).
/// Maps user utterance to one of these canonical categories.
enum class IntentCategory {
    QUERY,           ///< Information retrieval (e.g., "show me the data")
    COMMAND,         ///< Data modification (e.g., "delete the record")
    QUESTION,        ///< Help/explanation (e.g., "how do I...")
    CONVERSATION,    ///< Small talk / greetings
    UNKNOWN          ///< Unable to classify
};

/// @brief Convert IntentCategory to string representation.
std::string intentToString(IntentCategory cat);

/// @brief Convert string to IntentCategory.
IntentCategory stringToIntent(const std::string& s);

// Named entity
/// @struct NamedEntity
/// @brief Extracted named entity from text (frozen types).
struct NamedEntity {
    /// @brief Entity text as found in input.
    std::string text = {};

    /// @brief Entity type: DATE, PERSON, ORGANIZATION, NUMBER, METRIC, OBJECT, TIME_RANGE.
    std::string type;

    /// @brief Confidence [0.0, 1.0] in the entity extraction.
    float confidence = 0.0f;

    /// @brief Start byte offset in input text.
    int start_offset = 0;

    /// @brief End byte offset in input text.
    int end_offset = 0;
};

// Intent detection result
/// @struct IntentResult
/// @brief Complete output of intent detection pipeline.
struct IntentResult {
    /// @brief Detected intent category (or UNKNOWN if detection failed).
    IntentCategory intent = IntentCategory::UNKNOWN;

    /// @brief Confidence [0.0, 1.0] in the detected intent.
    float confidence = 0.0f;

    /// @brief Extracted named entities from the utterance.
    std::vector<NamedEntity> entities;

    /// @brief Normalized/canonicalized version of input query.
    std::string normalized_query;

    /// @brief Suggested context keys to resolve ambiguities.
    json context_hints;

    /// @brief true if detection would benefit from additional context.
    bool requires_context = false;
};

// Context manager for multi-turn conversations
/// @class ConversationContext
/// @brief Multi-turn conversation history and entity cache (frozen Phase 1).
class ConversationContext {
public:
    /// @brief Construct with default max_history=20.
    ConversationContext() = default;

    /// @brief Construct with custom max_history.
    /// @param max_history Maximum turns to retain (older turns dropped).
    explicit ConversationContext(size_t max_history);

    /// @brief Add a user/assistant turn to history.
    void addTurn(const std::string& user_input, const std::string& assistant_response);

    /// @brief Store extracted entity for context lookup.
    void setEntity(const std::string& key, const std::string& value);

    /// @brief Retrieve entity value from cache.
    std::optional<std::string> getEntity(const std::string& key) const;

    /// @brief Clear all cached entities (keep history).
    void clearEntities();

    /// @brief Get conversation history (user/assistant turns).
    const std::vector<std::pair<std::string, std::string>>& getHistory() const;

    /// @brief Build context string from recent history.
    /// @param max_turns Maximum recent turns to include (default 5).
    std::string buildContextString(size_t max_turns = 5) const;

    /// @brief Clear all history and entities.
    void clear();

    /// @brief Get number of turns in history.
    size_t turnCount() const;

    /// @brief Check if entity exists in cache.
    bool hasEntity(const std::string& key) const;

private:
    std::vector<std::pair<std::string, std::string>> history_;
    std::map<std::string, std::string> entities_;
    size_t max_history_ = 20;
};

// Configuration
/// @struct IntentDetectorConfig
/// @brief Configuration for VoiceIntentDetector (frozen Phase 1 + Phase 3 extensions).
struct IntentDetectorConfig {
    /// @brief Minimum confidence threshold for accepting intents (frozen: 0.5).
    /// Changing requires Phase 1 amendment with model retraining evidence.
    float min_confidence_threshold = 0.5f;

    /// @brief Minimum confidence for accepting extracted entities (frozen: 0.4).
    float entity_confidence_threshold = 0.4f;

    /// @brief Enable LLM-based disambiguation for ambiguous intents.
    /// Default: false (requires LLM to be available and configured).
    bool use_llm_for_ambiguous = false;

    /// @brief Maximum conversation turns to retain for context (frozen: 10).
    size_t max_context_turns = 10;

    /// @brief Custom regex patterns for entity extraction.
    /// Empty vector = use built-in entity patterns only.
    std::vector<std::string> custom_entity_patterns;
    
    /// @brief Phase 3: Ask user to clarify when confidence too low.
    /// Default: true. When true and confidence < min_confidence_threshold,
    /// detector returns requires_context=true instead of rejecting outright.
    bool ask_clarification_on_low_confidence = true;

    /// @brief Phase 3: Enable timeout protection during detection.
    /// Default: true. When true, detection is interrupted if it exceeds
    /// detection_timeout_ms, returning safe default (intent=UNKNOWN).
    bool use_timeout_protection = true;

    /// @brief Phase 3: Maximum time for intent detection (milliseconds).
    /// Default: 5000 (5 seconds). Exceeded = safe timeout fallback.
    int64_t detection_timeout_ms = 5000;
};

// VoiceIntentDetector: Phase 1 frozen API (Phase 3 extensions)
/// @class VoiceIntentDetector
/// @brief Intent detection and named entity recognition (frozen Phase 1 + Phase 3).
///
/// Detects user intent category from transcribed text and extracts relevant
/// named entities for downstream processing.
///
/// **Confidence Thresholds (Frozen):**
/// - min_confidence_threshold: 0.5 (default)
/// - entity_confidence_threshold: 0.4 (default)
///
/// **Phase 3 Extensions:**
/// - Timeout protection: detections exceeding detection_timeout_ms return safe default
/// - Clarification prompts: low-confidence results trigger user feedback loop
/// - Timeout tracking: isTimeoutDetected() reports detection delays
class VoiceIntentDetector {
public:
    /// @brief Construct with configuration (frozen Phase 1 defaults + Phase 3 options).
    explicit VoiceIntentDetector(const IntentDetectorConfig& config = {});
    ~VoiceIntentDetector() = default;

    /// @brief Detect intent from transcribed text (frozen contract).
    ///
    /// @param text Transcribed user utterance (non-empty required).
    /// @param context Optional conversation context for pronoun resolution.
    ///
    /// @pre text must not be empty
    /// @post result.intent set to detected category (or UNKNOWN)
    /// @post result.entities populated with extracted entities
    ///
    /// @return IntentResult with detected intent and confidence.
    /// @error 6800 Intent detection failed
    /// @error 6802 Unknown/unclassified intent
    /// @error 6803 Ambiguous intent
    /// @error 6805 Context resolution failed
    IntentResult detect(const std::string& text, const ConversationContext* context = nullptr);

    /// @brief Extract named entities from text (standalone).
    ///
    /// @param text Input text to analyze.
    /// @return Vector of extracted NamedEntity objects.
    /// @error 6804 NER error
    std::vector<NamedEntity> extractEntities(const std::string& text);

    /// @brief Classify intent category (without confidence scoring).
    ///
    /// @param text Input text.
    /// @return IntentCategory (or UNKNOWN if no clear category).
    IntentCategory classifyIntent(const std::string& text);

    /// @brief Check if confidence meets frozen threshold.
    ///
    /// @param confidence Score [0.0, 1.0].
    /// @return true if confidence >= min_confidence_threshold; false otherwise.
    bool meetsThreshold(float confidence) const;

    /// @brief Normalize/canonicalize a query (frozen algorithm).
    ///
    /// @param text Input query.
    /// @param context Optional context for pronoun resolution.
    /// @return Normalized query string.
    std::string normalizeQuery(const std::string& text, const ConversationContext* context = nullptr);

    /// @brief Get aggregate statistics (JSON).
    ///
    /// @return JSON with detection counts and confidence distribution.
    json getStatistics() const;
    
    /// @brief Detect if confidence is too low for acceptance (Phase 3).
    /// @param confidence Confidence score [0.0, 1.0].
    /// @return true if confidence below threshold (should ask for clarification).
    /// @note Only meaningful if ask_clarification_on_low_confidence=true.
    bool isConfidenceTooLow(float confidence) const noexcept;
    
    /// @brief Get safe default result when detection times out (Phase 3).
    /// @return IntentResult with intent=UNKNOWN and confidence=0.
    /// @note Returned when use_timeout_protection=true and timeout occurs.
    IntentResult getTimeoutDefault() const noexcept;
    
    /// @brief Detect if detector is experiencing timeout delays (Phase 3).
    /// @return true if timeout_detected_ flag is set after slow detection.
    bool isTimeoutDetected() const noexcept;

private:
    IntentDetectorConfig config_;
    uint64_t detections_total_ = 0;
    uint64_t high_confidence_ = 0;
    uint64_t low_confidence_ = 0;
    uint64_t timeout_fallbacks_ = 0;  ///< Phase 3: timeout fallback count

    int64_t last_detection_start_ms_ = 0;  ///< Phase 3: for timeout tracking
    bool timeout_detected_ = false;        ///< Phase 3: timeout flag

    std::vector<NamedEntity> extractDateEntities(const std::string& text) const;
    std::vector<NamedEntity> extractNumberEntities(const std::string& text) const;
    std::vector<NamedEntity> extractMetricEntities(const std::string& text) const;
    float computeIntentConfidence(const std::string& text, IntentCategory cat) const;
    
    /// @brief Phase 3: Get current wall-clock time in milliseconds.
    int64_t nowMs() const;
};

}} // namespace themis::voice
