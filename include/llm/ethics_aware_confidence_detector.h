/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            ethics_aware_confidence_detector.h                 ║
  Version:         0.0.41                                             ║
  Last Modified:   2026-04-14 11:25:33                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     345                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
    • a629043ab2  2026-02-22  Audit: document gaps found - benchmarks and stale annotat... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/**
 * @file ethics_aware_confidence_detector.h
 * @brief Ethics-aware confidence detection for LLM outputs
 * 
 * Multi-level confidence scoring that respects human autonomy based on:
 * - Technical confidence (token entropy, perplexity)
 * - Autonomy respect scoring (detects patronizing language)
 * - Transparency scoring (acknowledges limitations)
 * - Combined ethics-aware confidence metric
 * 
 * Scientific foundation:
 * - Manakul et al. (2023): SelfCheckGPT - Hallucination detection
 * - Kuhn et al. (2023): Semantic entropy for uncertainty estimation
 * - UN Human Rights (1948): Respect for human autonomy
 */

#pragma once

#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include <mutex>

namespace themis {
namespace llm {

/**
 * @brief Configuration for ethics-aware confidence detection
 */
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
     * @brief Validate configuration weights
     * @return true if weights sum to approximately 1.0
     */
    bool validateWeights() const;
};

/**
 * @brief Result of confidence detection
 */
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

/**
 * @brief Token-level confidence information
 */
struct TokenConfidence {
    std::string token;
    float probability;
    float entropy;
    int position;
};

/**
 * @brief Ethics-aware confidence detector
 * 
 * Detects confidence levels in LLM outputs while ensuring ethical compliance.
 * Combines technical confidence metrics (entropy, perplexity) with ethical
 * dimensions (autonomy respect, transparency) to provide a comprehensive
 * confidence score that prevents hallucinations while respecting human autonomy.
 */
class EthicsAwareConfidenceDetector {
public:
    /**
     * @brief Constructor
     * @param config Configuration for detection
     */
    explicit EthicsAwareConfidenceDetector(
        const EthicsAwareConfidenceConfig& config = {}
    );
    
    /**
     * @brief Destructor
     */
    ~EthicsAwareConfidenceDetector();
    
    // ═══════════════════════════════════════════════════════════
    // Core functionality
    // ═══════════════════════════════════════════════════════════
    
    /**
     * @brief Detect confidence in generated text
     * @param text Generated text to analyze
     * @param token_confidences Optional token-level confidence data
     * @return Confidence result with scores and analysis
     */
    ConfidenceResult detectConfidence(
        const std::string& text,
        const std::vector<TokenConfidence>& token_confidences = {}
    );
    
    /**
     * @brief Detect confidence with conversation context
     * @param text Generated text to analyze
     * @param query Original user query
     * @param context Conversation history
     * @param token_confidences Optional token-level confidence data
     * @return Confidence result with context-aware analysis
     */
    ConfidenceResult detectConfidenceWithContext(
        const std::string& text,
        const std::string& query,
        const std::vector<std::string>& context,
        const std::vector<TokenConfidence>& token_confidences = {}
    );
    
    // ═══════════════════════════════════════════════════════════
    // Individual dimension evaluation
    // ═══════════════════════════════════════════════════════════
    
    /**
     * @brief Evaluate technical confidence (entropy-based)
     * @param text Generated text
     * @param token_confidences Token-level confidence data
     * @return Technical confidence score (0-1)
     */
    float evaluateTechnicalConfidence(
        const std::string& text,
        const std::vector<TokenConfidence>& token_confidences
    );
    
    /**
     * @brief Evaluate autonomy respect (patronizing language detection)
     * @param text Generated text
     * @param query Optional user query for context
     * @return Autonomy respect score (0-1)
     */
    float evaluateAutonomyRespect(
        const std::string& text,
        const std::string& query = ""
    );
    
    /**
     * @brief Evaluate transparency (uncertainty acknowledgment)
     * @param text Generated text
     * @return Transparency score (0-1)
     */
    float evaluateTransparency(const std::string& text);
    
    // ═══════════════════════════════════════════════════════════
    // Pattern detection
    // ═══════════════════════════════════════════════════════════
    
    /**
     * @brief Detect patronizing language patterns
     * @param text Text to analyze
     * @return Vector of detected patronizing phrases
     */
    std::vector<std::string> detectPatronizingLanguage(const std::string& text);
    
    /**
     * @brief Detect imperative commands
     * @param text Text to analyze
     * @return Vector of detected imperative phrases
     */
    std::vector<std::string> detectImperatives(const std::string& text);
    
    /**
     * @brief Detect uncertainty acknowledgment
     * @param text Text to analyze
     * @return Vector of detected hedge words/phrases
     */
    std::vector<std::string> detectUncertaintyAcknowledgment(const std::string& text);
    
    /**
     * @brief Check if text preserves human choice
     * @param text Text to analyze
     * @return true if human agency is preserved
     */
    bool checkChoicePreservation(const std::string& text);
    
    // ═══════════════════════════════════════════════════════════
    // Configuration
    // ═══════════════════════════════════════════════════════════
    
    /**
     * @brief Update configuration
     * @param config New configuration
     */
    void setConfig(const EthicsAwareConfidenceConfig& config);
    
    /**
     * @brief Get current configuration
     * @return Current configuration
     */
    EthicsAwareConfidenceConfig getConfig() const;
    
    /**
     * @brief Clear detection cache
     */
    void clearCache();
    
    // ═══════════════════════════════════════════════════════════
    // Statistics
    // ═══════════════════════════════════════════════════════════
    
    /**
     * @brief Statistics for monitoring
     */
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
     * @brief Get statistics
     * @return Current statistics
     */
    Statistics getStatistics() const;
    
    /**
     * @brief Reset statistics
     */
    void resetStatistics();

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
    
    // Pattern matching helpers
    bool containsPattern(const std::string& text, const std::vector<std::string>& patterns);
    int countPatternMatches(const std::string& text, const std::vector<std::string>& patterns);
    std::string toLowerCase(const std::string& text);
    
    // Entropy calculation
    float calculateTokenEntropy(const std::vector<TokenConfidence>& tokens);
    float calculatePerplexity(const std::vector<TokenConfidence>& tokens);
    
    // Scoring helpers
    float combineScores(float technical, float autonomy, float transparency);
    std::string generateReasoning(const ConfidenceResult& result);
    
    // Cache management
    std::string generateCacheKey(const std::string& text);
    bool getCachedResult(const std::string& key, ConfidenceResult& result);
    void cacheResult(const std::string& key, const ConfidenceResult& result);
};

/**
 * @brief Factory for creating confidence detectors
 */
class ConfidenceDetectorFactory {
public:
    /**
     * @brief Create detector with default configuration
     */
    static std::unique_ptr<EthicsAwareConfidenceDetector> createDefault();
    
    /**
     * @brief Create detector with strict thresholds
     */
    static std::unique_ptr<EthicsAwareConfidenceDetector> createStrict();
    
    /**
     * @brief Create detector with lenient thresholds
     */
    static std::unique_ptr<EthicsAwareConfidenceDetector> createLenient();
    
    /**
     * @brief Create detector with custom configuration
     */
    static std::unique_ptr<EthicsAwareConfidenceDetector> create(
        const EthicsAwareConfidenceConfig& config
    );
};

} // namespace llm
} // namespace themis
