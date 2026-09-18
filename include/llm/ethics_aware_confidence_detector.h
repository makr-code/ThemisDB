/**
 * @file ethics_aware_confidence_detector.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 100/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#pragma once

#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include <mutex>

namespace themis {
namespace llm {

struct EthicsAwareConfidenceConfig {
    // Thresholds
    float min_autonomy_respect = 0.7f;        ///< Minimum autonomy respect score
    float min_transparency = 0.6f;            ///< Minimum transparency score
    float min_technical_confidence = 0.5f;    ///< Minimum technical confidence
    
    // Weights for combined score
    float technical_weight = 0.40f;           ///< Weight for technical confidence
    float autonomy_weight = 0.35f;            ///< Weight for autonomy respect
    float transparency_weight = 0.25f;        ///< Weight for transparency
    
    // Detection settings
    bool enable_patronizing_detection = true;  ///< Enable patronizing language detection
    bool enable_entropy_analysis = true;       ///< Enable token entropy analysis
    bool enable_uncertainty_detection = true;  ///< Enable uncertainty acknowledgment detection
    
    // Performance
    bool cache_results = true;                 ///< Cache confidence scores
    size_t max_cache_size = 1000;             ///< Maximum cache entries
    
    /**
     * @brief Validate Weights.
     * @return True when the operation succeeds.
     */
    bool validateWeights() const;
};

struct ConfidenceResult {
    // Individual scores (0.0 - 1.0)
    float technical_confidence = 0.0f;         ///< Technical confidence (entropy-based)
    float autonomy_respect_score = 0.0f;       ///< Autonomy respect score
    float transparency_score = 0.0f;           ///< Transparency/limitation acknowledgment
    
    // Combined score
    float combined_confidence = 0.0f;          ///< Weighted combination
    
    // Detection results
    bool has_patronizing_language = false;     ///< Detected patronizing language
    bool acknowledges_uncertainty = false;      ///< Acknowledges limitations
    bool preserves_human_choice = false;        ///< Preserves human agency
    
    // Detailed analysis
    std::vector<std::string> patronizing_phrases;  ///< Detected problematic phrases
    std::vector<std::string> imperatives;          ///< Detected imperative commands
    std::vector<std::string> hedge_words;          ///< Detected uncertainty indicators
    
    // Technical metrics
    float avg_token_entropy = 0.0f;            ///< Average token-level entropy
    float perplexity = 0.0f;                   ///< Overall perplexity
    
    // Quality assessment
    bool meets_quality_threshold = false;       ///< Meets minimum quality standards
    std::string reasoning;                      ///< Explanation of scores
};

struct TokenConfidence {
    /**
     * @brief Token Confidence.
     * @return Return value.
     */
    virtual ~TokenConfidence() = default;
    std::string token;
    float probability = 0.0f;
    float entropy = 0.0f;
    int position = 0;
};

class EthicsAwareConfidenceDetector {
public:
    explicit EthicsAwareConfidenceDetector(
        const EthicsAwareConfidenceConfig& config = {}
    );
    
    ~EthicsAwareConfidenceDetector();
    
    // ═══════════════════════════════════════════════════════════
    // Core functionality
    // ═══════════════════════════════════════════════════════════
    
    ConfidenceResult detectConfidence(
        const std::string& text,
        const std::vector<TokenConfidence>& token_confidences = {}
    );
    
    ConfidenceResult detectConfidenceWithContext(
        const std::string& text,
        const std::string& query,
        const std::vector<std::string>& context,
        const std::vector<TokenConfidence>& token_confidences = {}
    );
    
    /**
     * @brief ═══════════════════════════════════════════════════════════ Individual dimension evaluation ═══════════════════════════════════════════════════════════
     * @param[in] text Input parameter.
     * @param[in] token_confidences Input parameter.
     * @return Return value.
     */
    
    float evaluateTechnicalConfidence(
        const std::string& text,
        const std::vector<TokenConfidence>& token_confidences
    );
    
    float evaluateAutonomyRespect(
        const std::string& text,
        const std::string& query = ""
    );
    
    /**
     * @brief Evaluate Transparency.
     * @param[in] text Input parameter.
     * @return Return value.
     */
    float evaluateTransparency(const std::string& text);
    
    /**
     * @brief ═══════════════════════════════════════════════════════════ Pattern detection ═══════════════════════════════════════════════════════════
     * @param[in] text Input parameter.
     * @return Return value.
     */
    
    std::vector<std::string> detectPatronizingLanguage(const std::string& text);
    
    /**
     * @brief Detect Imperatives.
     * @param[in] text Input parameter.
     * @return Return value.
     */
    std::vector<std::string> detectImperatives(const std::string& text);
    
    /**
     * @brief Detect Uncertainty Acknowledgment.
     * @param[in] text Input parameter.
     * @return Return value.
     */
    std::vector<std::string> detectUncertaintyAcknowledgment(const std::string& text);
    
    /**
     * @brief Check Choice Preservation.
     * @param[in] text Input parameter.
     * @return True when the operation succeeds.
     */
    bool checkChoicePreservation(const std::string& text);
    
    /**
     * @brief ═══════════════════════════════════════════════════════════ Configuration ═══════════════════════════════════════════════════════════
     * @param[in] config Input parameter.
     */
    
    void setConfig(const EthicsAwareConfidenceConfig& config);
    
    /**
     * @brief Get Config.
     * @return Return value.
     */
    EthicsAwareConfidenceConfig getConfig() const;
    
    /**
     * @brief Clear Cache.
     */
    void clearCache();
    
    // ═══════════════════════════════════════════════════════════
    // Statistics
    // ═══════════════════════════════════════════════════════════
    
    struct Statistics {
        uint64_t total_detections = 0;
        uint64_t patronizing_detected = 0;
        uint64_t low_confidence_detected = 0;
        uint64_t cache_hits = 0;
        uint64_t cache_misses = 0;
        
        // Score distributions
        float avg_technical_confidence = 0.0f;
        float avg_autonomy_respect = 0.0f;
        float avg_transparency = 0.0f;
        float avg_combined_confidence = 0.0f;
    };
    
    /**
     * @brief Return access control statistics.
     * @return Access control statistics.
     */
    Statistics getStatistics() const;
    
    /**
     * @brief Reset Statistics.
     */
    void resetStatistics();

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
    
    // Pattern matching helpers
    /**
     * @brief Contains Pattern.
     * @param[in] text Input parameter.
     * @param[in] patterns Input parameter.
     * @return True when the operation succeeds.
     */
    bool containsPattern(const std::string& text, const std::vector<std::string>& patterns);
    /**
     * @brief Count Pattern Matches.
     * @param[in] text Input parameter.
     * @param[in] patterns Input parameter.
     * @return Return value.
     */
    int countPatternMatches(const std::string& text, const std::vector<std::string>& patterns);
    /**
     * @brief To Lower Case.
     * @param[in] text Input parameter.
     * @return Return value.
     */
    std::string toLowerCase(const std::string& text);
    
    // Entropy calculation
    /**
     * @brief Calculate Token Entropy.
     * @param[in] tokens Input parameter.
     * @return Return value.
     */
    float calculateTokenEntropy(const std::vector<TokenConfidence>& tokens);
    /**
     * @brief Calculate Perplexity.
     * @param[in] tokens Input parameter.
     * @return Return value.
     */
    float calculatePerplexity(const std::vector<TokenConfidence>& tokens);
    
    // Scoring helpers
    /**
     * @brief Combine Scores.
     * @param[in] technical Input parameter.
     * @param[in] autonomy Input parameter.
     * @param[in] transparency Input parameter.
     * @return Return value.
     */
    float combineScores(float technical, float autonomy, float transparency);
    /**
     * @brief Generate Reasoning.
     * @param[in] result Input parameter.
     * @return Return value.
     */
    std::string generateReasoning(const ConfidenceResult& result);
    
    // Cache management
    /**
     * @brief Generate Cache Key.
     * @param[in] text Input parameter.
     * @return Return value.
     */
    std::string generateCacheKey(const std::string& text);
    /**
     * @brief Get Cached Result.
     * @param[in] key Input parameter.
     * @param[in,out] result Input/output parameter.
     * @return True when the operation succeeds.
     */
    bool getCachedResult(const std::string& key, ConfidenceResult& result);
    /**
     * @brief Cache Result.
     * @param[in] key Input parameter.
     * @param[in] result Input parameter.
     */
    void cacheResult(const std::string& key, const ConfidenceResult& result);
};

class ConfidenceDetectorFactory {
public:
    /**
     * @brief Create Default.
     * @return Return value.
     */
    static std::unique_ptr<EthicsAwareConfidenceDetector> createDefault();
    
    /**
     * @brief Create Strict.
     * @return Return value.
     */
    static std::unique_ptr<EthicsAwareConfidenceDetector> createStrict();
    
    /**
     * @brief Create Lenient.
     * @return Return value.
     */
    static std::unique_ptr<EthicsAwareConfidenceDetector> createLenient();
    
    /**
     * @brief Create.
     * @param[in] config Input parameter.
     * @return Return value.
     */
    static std::unique_ptr<EthicsAwareConfidenceDetector> create(
        const EthicsAwareConfidenceConfig& config
    );
};

} // namespace llm
} // namespace themis
