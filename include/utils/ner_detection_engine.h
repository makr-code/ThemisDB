/**
 * @file ner_detection_engine.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.18
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include "pii_detection_engine.h"
#include <unordered_set>
#include <unordered_map>
#include <mutex>
#include <vector>
#include <string>

namespace themis {
namespace utils {

/**
 * @brief Named Entity Recognition (NER) based PII detection engine
 *
 * Rule-based NER engine for detecting unstructured PII that regex cannot capture:
 * - PERSON_NAME:   detected via honorific prefixes (Mr., Dr., Prof., etc.) and
 *                  capitalized name patterns, configurable first/last name gazetteers
 * - ORGANIZATION:  detected via legal-entity suffixes (Inc., Corp., Ltd., GmbH, etc.)
 *                  and configurable organization keyword lists
 * - LOCATION:      detected via geographic preposition context ("in Paris", "at Geneva")
 *                  and configurable city/country gazetteers
 *
 * This engine complements RegexDetectionEngine (which handles structured PII such as
 * email, phone, SSN) with NER capabilities for free-form natural language text.
 *
 * Features:
 * - No external ML-framework dependency (rule-based / gazetteer NER)
 * - YAML-configurable name lists, title prefixes, org suffixes, location hints
 * - Runtime reload with config rollback on failure
 * - Confidence scores per entity type based on detection signal strength
 * - Thread-safe for concurrent access
 *
 * Example YAML configuration:
 * @code{.yaml}
 * type: "ner"
 * enabled: true
 * settings:
 *   min_confidence: 0.70
 *   default_redaction_mode: "strict"
 * honorifics: ["Mr.", "Mrs.", "Ms.", "Dr.", "Prof.", "Sir", "Dame"]
 * org_suffixes: ["Inc.", "Corp.", "Ltd.", "LLC", "GmbH", "AG", "plc"]
 * location_prepositions: ["in", "at", "from", "near", "to"]
 * @endcode
 */
class NERDetectionEngine : public IPIIDetectionEngine {
public:
    NERDetectionEngine();
    ~NERDetectionEngine() override = default;

    // IPIIDetectionEngine interface
    std::string getName() const override;
    std::string getVersion() const override;
    bool isEnabled() const override;
    PluginSignature getSignature() const override;
    bool initialize(const nlohmann::json& config) override;
    bool reload(const nlohmann::json& config) override;
    std::vector<PIIFinding> detectInText(const std::string& text) const override;
    PIIType classifyFieldName(const std::string& field_name) const override;
    std::string getRedactionRecommendation(PIIType type) const override;
    std::string getLastError() const override;
    nlohmann::json getMetadata() const override;

private:
    // Engine state
    bool enabled_;
    bool model_available_;  // Tracks whether gazetteers/model data are loaded
    mutable std::string last_error_;
    PluginSignature signature_;
    mutable std::mutex mutex_;

    // Settings
    double min_confidence_;
    std::string default_redaction_mode_;

    // Honorific / title prefixes that introduce a person name (case-insensitive)
    std::unordered_set<std::string> honorifics_;

    // Legal-entity suffixes that mark an organisation (case-insensitive)
    std::unordered_set<std::string> org_suffixes_;

    // Prepositions whose following capitalised word is likely a location
    std::unordered_set<std::string> location_prepositions_;

    // Configurable field-name hints for classifyFieldName()
    std::unordered_map<std::string, PIIType> field_name_hints_;

    // Redaction modes per PIIType
    std::unordered_map<PIIType, std::string> redaction_modes_;

    // Internal helpers
    void loadDefaults();
    bool loadFromConfig(const nlohmann::json& config);
    void rebuildFieldHints();

    // Tokeniser: splits text into (token, start_offset) pairs
    struct Token {
        std::string text;
        size_t offset;
    };
    static std::vector<Token> tokenise(const std::string& text);

    // Per-entity detection passes
    void detectPersonNames(
        const std::vector<Token>& tokens,
        std::vector<PIIFinding>& out) const;
    void detectOrganizations(
        const std::vector<Token>& tokens,
        std::vector<PIIFinding>& out) const;
    void detectLocations(
        const std::vector<Token>& tokens,
        std::vector<PIIFinding>& out) const;

    // Capitalisation helpers
    static bool isCapitalized(const std::string& word);
    static std::string toLower(const std::string& s);

    // Build a PIIFinding spanning tokens[first..last] (inclusive)
    static PIIFinding makeSpan(
        const std::vector<Token>& tokens,
        size_t first,
        size_t last,
        PIIType type,
        double confidence,
        const std::string& pattern_name);
};

} // namespace utils
} // namespace themis
