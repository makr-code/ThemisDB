/**
 * @file knowledge_gap_detector.h
 * @brief Knowledge Gap Detection for RAG Systems
 * 
 * Detects when retrieved documents are insufficient to answer a query reliably.
 * Implements multi-level detection strategies based on similarity scores,
 * LLM confidence metrics, and explicit gap detection signals.
 */

#pragma once

#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include <functional>

namespace themis::llm::rag::knowledge_gap {

/**
 * @brief Type of knowledge gap detected
 */
enum class GapType {
    LOW_SIMILARITY,           ///< Retrieved documents have low semantic similarity
    INSUFFICIENT_DOCS,        ///< Not enough documents retrieved
    UNCERTAIN_GENERATION,     ///< LLM indicates low confidence in generation
    MISSING_ASPECTS,          ///< Query aspects not covered by documents
    CONFLICTING_INFO,         ///< Retrieved documents contain contradictions
    OUTDATED_INFO,           ///< Information may be outdated
    NONE                     ///< No gap detected
};

/**
 * @brief Recommended action when gap is detected
 */
enum class FallbackStrategy {
    EXPAND_SEARCH,            ///< Broaden search with relaxed constraints
    REFORMULATE_QUERY,        ///< Rephrase and retry search
    MULTI_HOP_RETRIEVAL,      ///< Perform iterative retrieval
    INSUFFICIENT_DATA_RESPONSE, ///< Return explicit "insufficient information" message
    ESCALATE_TO_BROADER_SOURCE, ///< Query additional data sources
    NONE                      ///< No fallback needed
};

/**
 * @brief Configuration for detection sensitivity
 */
enum class DetectionMode {
    FAST,        ///< Quick detection using only similarity scores (~10ms)
    BALANCED,    ///< Moderate detection with some LLM checks (~100ms)
    THOROUGH     ///< Comprehensive detection with full validation (~500ms+)
};

/**
 * @brief Retrieved document information
 */
struct RetrievedDocument {
    std::string id;
    std::string content;
    double similarity_score;
    std::unordered_map<std::string, std::string> metadata;
};

/**
 * @brief Context for generation and gap detection
 */
struct GenerationContext {
    double token_probability_avg;    ///< Average token probability
    double perplexity;                ///< Generation perplexity
    std::vector<double> token_probs; ///< Per-token probabilities
    bool generation_started;          ///< Whether generation has begun
};

/**
 * @brief Result of gap detection analysis
 */
struct DetectionResult {
    bool gap_detected;                           ///< Whether a gap was found
    double confidence_score;                     ///< Confidence in the detection (0-1)
    GapType gap_type;                           ///< Type of gap detected
    std::vector<std::string> missing_aspects;    ///< Missing query aspects
    FallbackStrategy recommendation;             ///< Recommended fallback action
    std::string explanation;                     ///< Human-readable explanation
    
    // Detailed metrics
    double avg_similarity_score;                 ///< Average document similarity
    size_t num_retrieved_docs;                   ///< Number of documents retrieved
    double coverage_score;                       ///< Query coverage by documents (0-1)
};

/**
 * @brief Configuration parameters for gap detection
 */
struct KnowledgeGapConfig {
    DetectionMode mode = DetectionMode::BALANCED;
    
    // Thresholds
    double similarity_threshold = 0.75;          ///< Minimum similarity score
    size_t min_documents = 3;                    ///< Minimum required documents
    double confidence_threshold = 0.7;           ///< Minimum confidence threshold
    double coverage_threshold = 0.8;             ///< Minimum query coverage
    
    // Advanced options
    bool enable_self_consistency_check = true;   ///< Check answer consistency
    size_t self_consistency_samples = 3;         ///< Number of samples for consistency
    bool enable_claim_verification = true;       ///< Verify claims against sources
    bool enable_query_aspect_analysis = true;    ///< Analyze query aspect coverage
    
    // Fallback configuration
    std::vector<FallbackStrategy> fallback_chain = {
        FallbackStrategy::EXPAND_SEARCH,
        FallbackStrategy::REFORMULATE_QUERY,
        FallbackStrategy::INSUFFICIENT_DATA_RESPONSE
    };
};

/**
 * @brief Main Knowledge Gap Detector class
 * 
 * Implements multi-level detection strategies:
 * - Level 1: Pre-generation (similarity, document count)
 * - Level 2: During generation (token probabilities, perplexity)
 * - Level 3: Post-generation (consistency, claim verification)
 */
class KnowledgeGapDetector {
public:
    /**
     * @brief Construct detector with configuration
     * @param config Detection configuration parameters
     */
    explicit KnowledgeGapDetector(const KnowledgeGapConfig& config = {});
    
    /**
     * @brief Destructor
     */
    ~KnowledgeGapDetector();
    
    /**
     * @brief Detect knowledge gap before generation
     * @param query User query string
     * @param documents Retrieved documents
     * @return Detection result with recommendations
     */
    DetectionResult detectPreGeneration(
        const std::string& query,
        const std::vector<RetrievedDocument>& documents
    );
    
    /**
     * @brief Detect knowledge gap during generation
     * @param query User query string
     * @param documents Retrieved documents
     * @param context Generation context with probabilities
     * @return Detection result with recommendations
     */
    DetectionResult detectDuringGeneration(
        const std::string& query,
        const std::vector<RetrievedDocument>& documents,
        const GenerationContext& context
    );
    
    /**
     * @brief Detect knowledge gap after generation
     * @param query User query string
     * @param documents Retrieved documents
     * @param generated_answer The generated answer
     * @return Detection result with recommendations
     */
    DetectionResult detectPostGeneration(
        const std::string& query,
        const std::vector<RetrievedDocument>& documents,
        const std::string& generated_answer
    );
    
    /**
     * @brief Comprehensive detection (all levels)
     * @param query User query string
     * @param documents Retrieved documents
     * @param generated_answer The generated answer
     * @param context Optional generation context
     * @return Combined detection result
     */
    DetectionResult detectGap(
        const std::string& query,
        const std::vector<RetrievedDocument>& documents,
        const std::string& generated_answer,
        const GenerationContext& context = {}
    );
    
    /**
     * @brief Update configuration
     * @param config New configuration
     */
    void setConfig(const KnowledgeGapConfig& config);
    
    /**
     * @brief Get current configuration
     * @return Current configuration
     */
    KnowledgeGapConfig getConfig() const;
    
    /**
     * @brief Set callback for when gaps are detected
     * @param callback Function to call on gap detection
     */
    void setGapDetectionCallback(
        std::function<void(const DetectionResult&)> callback
    );

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
    
    // Internal detection methods
    double calculateAverageSimilarity(const std::vector<RetrievedDocument>& docs);
    double calculateQueryCoverage(const std::string& query, 
                                  const std::vector<RetrievedDocument>& docs);
    std::vector<std::string> extractQueryAspects(const std::string& query);
    std::vector<std::string> findMissingAspects(
        const std::string& query,
        const std::vector<RetrievedDocument>& docs
    );
    bool checkSelfConsistency(const std::string& query,
                             const std::vector<RetrievedDocument>& docs);
    std::vector<std::string> extractClaims(const std::string& answer);
    bool verifyClaim(const std::string& claim,
                    const std::vector<RetrievedDocument>& docs);
};

/**
 * @brief Factory for creating detectors with different configurations
 */
class KnowledgeGapDetectorFactory {
public:
    /**
     * @brief Create a fast detector (pre-generation only)
     */
    static std::unique_ptr<KnowledgeGapDetector> createFast();
    
    /**
     * @brief Create a balanced detector (pre + during generation)
     */
    static std::unique_ptr<KnowledgeGapDetector> createBalanced();
    
    /**
     * @brief Create a thorough detector (all detection levels)
     */
    static std::unique_ptr<KnowledgeGapDetector> createThorough();
    
    /**
     * @brief Create a custom configured detector
     */
    static std::unique_ptr<KnowledgeGapDetector> create(const KnowledgeGapConfig& config);
};

} // namespace themis::llm::rag::knowledge_gap
