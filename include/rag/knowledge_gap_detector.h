/**
 * @file knowledge_gap_detector.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 100/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#pragma once

#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include <functional>

namespace themis::rag::knowledge_gap {

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
    ETHICAL_PERSPECTIVE_GAP,  ///< Ethical context detected but insufficient diverse perspectives
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
    double token_probability_avg = 0;    ///< Average token probability
    double perplexity;                ///< Generation perplexity
    std::vector<double> token_probs; ///< Per-token probabilities
    bool generation_started;          ///< Whether generation has begun
};

/**
 * @brief Result of gap detection analysis
 */
struct DetectionResult {
    bool gap_detected = 0;                           ///< Whether a gap was found
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
    
    // Phase 2: Token Probability Tracking
    bool enable_token_probability = true;        ///< Enable token probability tracking
    double perplexity_threshold = 100.0;         ///< Perplexity anomaly threshold
    size_t perplexity_window_size = 10;          ///< Sliding window size for perplexity
    double outlier_zscore_threshold = 3.0;       ///< Z-score threshold for outlier detection
    
    // Phase 2: Self-Consistency Check
    // Ethical perspective gap settings
    bool enable_ethical_gap_detection = true;    ///< Enable ethical perspective gap detection
    size_t min_ethical_perspectives = 2;         ///< Minimum diverse perspectives required
    double ethical_diversity_threshold = 0.6;    ///< Minimum perspective diversity score
    int ethical_keyword_threshold = 2;           ///< Minimum ethical keywords to classify as ethical query
    
    // Advanced options
    bool enable_self_consistency_check = true;   ///< Check answer consistency
    size_t self_consistency_samples = 5;         ///< Number of samples for consistency (3-5)
    std::vector<double> temperature_range = {0.7, 0.8, 0.9}; ///< Temperature variations for generating diverse samples
    double consistency_threshold = 0.6;          ///< Minimum consistency score
    size_t consistency_timeout_ms = 10000;       ///< Max timeout per sample (10s)
    
    // Phase 2: FLARE-Style Active Retrieval
    bool enable_flare = true;                    ///< Enable FLARE active retrieval (default: true since v1.4.0)
    size_t max_retrieval_rounds = 3;             ///< Max re-retrieval rounds
    double flare_confidence_threshold = 0.5;     ///< Trigger re-retrieval if confidence < 0.5
    
    // Advanced options
    bool enable_claim_verification = true;       ///< Verify claims against sources
    bool enable_query_aspect_analysis = true;    ///< Analyze query aspect coverage
    
    // Fallback configuration
    std::vector<FallbackStrategy> fallback_chain = {
        FallbackStrategy::EXPAND_SEARCH,
        FallbackStrategy::REFORMULATE_QUERY,
        FallbackStrategy::INSUFFICIENT_DATA_RESPONSE
    };

    // Tenant isolation
    std::string tenant_id; ///< Tenant identifier threaded through retrieval calls; empty = global/anonymous
};

/**
 * @brief Callback type for dynamic document retrieval in the FLARE loop.
 *
 * The callable receives a reformulated query string and the maximum number
 * of documents to return (k).  It must return a (possibly empty) vector of
 * RetrievedDocument values.  An empty return value is interpreted as "no
 * additional documents found" and stops the FLARE iteration early.
 *
 * Example wiring with VectorIndexManager:
 * @code
 *   detector->setRetrievalCallback(
 *       [&vec_mgr, &db](const std::string& q, size_t k)
 *           -> std::vector<RetrievedDocument>
 *       {
 *           auto embedding = embed(q);
 *           if (embedding.empty()) return {};
 *           auto [st, results] = vec_mgr.searchKnn(embedding, k);
 *           if (!st.ok) return {};
 *           return rag::convertToRetrievedDocuments(results, db);
 *       });
 * @endcode
 */
/**
 * @brief Retrieval callback invoked by FLARE active retrieval.
 *
 * F5-2: The callback receives the query, max result count, and a
 * tenant identifier.  The tenant_id MUST be forwarded to the underlying
 * vector store so that dynamic re-retrieval always queries the same
 * tenant's document corpus as the original request — without this, a
 * reformulated FLARE query can silently cross tenant boundaries.
 */
using RetrievalCallback =
    std::function<std::vector<RetrievedDocument>(
        const std::string& query,
        size_t k,
        const std::string& tenant_id)>;

/**
 * @brief Callback type for LLM-based self-consistency sample generation.
 *
 * When injected via KnowledgeGapDetector::setLlmSampleFn(), the
 * `generateMultipleSamples()` internal method delegates to this function
 * instead of the built-in heuristic sentence-cycling path.
 *
 * The callable receives:
 *   - @p query      The original user query.
 *   - @p num_samples  The requested number of answer candidates.
 *
 * It must return a vector of strings (answer candidates) of any non-zero
 * size.  An empty return value causes `generateMultipleSamples()` to fall
 * back to the heuristic path.
 *
 * Example wiring with an ILLMPlugin:
 * @code
 *   detector->setLlmSampleFn(
 *       [&llm](const std::string& q, size_t n) {
 *           std::vector<std::string> samples;
 *           samples.reserve(n);
 *           for (size_t i = 0; i < n; ++i)
 *               samples.push_back(llm.generate(q, 0.8f)); // temperature
 *           return samples;
 *       });
 * @endcode
 */
using LlmSampleFn =
    std::function<std::vector<std::string>(const std::string& query,
                                           size_t             num_samples)>;

/**
 * @brief Callback type for runtime claim verification (C1 groundedness).
 *
 * When injected via KnowledgeGapDetector::setClaimVerificationFn(), the
 * post-generation claim check delegates claim verification to this function.
 * If unset, the detector falls back to the built-in term-overlap heuristic.
 */
using ClaimVerificationFn =
    std::function<bool(const std::string& claim,
                       const std::vector<RetrievedDocument>& docs)>;

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
     * @brief Construct detector with default configuration
     */
    KnowledgeGapDetector();
    
    /**
     * @brief Construct detector with custom configuration
     * @param config Detection configuration parameters
     */
    explicit KnowledgeGapDetector(const KnowledgeGapConfig& config);
    
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
     * @return Combined detection result
     */
    DetectionResult detectGap(
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
        const GenerationContext& context
    );
    
    /**
     * @brief FLARE-style forward-looking active retrieval
     * @param query User query string
     * @param initial_documents Initial retrieved documents
     * @param tenant_id Optional tenant scope for FLARE re-retrieval.
     * @return Detection result after iterative retrieval
     * 
     * Generates answer sentence-by-sentence, monitoring confidence
     * and dynamically retrieving more documents when confidence drops.
     */
    DetectionResult detectWithActiveRetrieval(
        const std::string& query,
        std::vector<RetrievedDocument>& initial_documents,
        const std::string& tenant_id = {}  ///< F5-2: tenant scope for FLARE re-retrieval
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

    /**
     * @brief Set the retrieval callback used by the FLARE active-retrieval loop.
     *
     * When set, @c detectWithActiveRetrieval() calls this function each time
     * the current document set does not provide sufficient coverage and a
     * reformulated sub-query is available.  The callback must be thread-safe
     * with respect to the surrounding context but does not need to be
     * re-entrant.
     *
     * Passing a default-constructed (empty) function disables dynamic
     * retrieval; the FLARE loop will then exit after the initial document set
     * check without attempting further searches.
     *
     * @param fn RetrievalCallback — receives (query, k) and returns documents.
     */
    void setRetrievalCallback(RetrievalCallback fn);

    /**
     * @brief Inject an LLM-based sample generator for the self-consistency check.
     *
     * When set, `checkSelfConsistency()` calls this function to obtain answer
     * candidates instead of cycling document sentences.  Pass an empty
     * (default-constructed) function to revert to the heuristic path.
     *
     * @param fn  LlmSampleFn — receives (query, num_samples) and returns candidates.
     */
    void setLlmSampleFn(LlmSampleFn fn);

    /**
     * @brief Inject a runtime claim verifier for C1 groundedness checks.
     *
     * When set, claim verification in detectPostGeneration() delegates to this
     * function. Pass an empty function to restore the built-in heuristic path.
     *
     * @param fn ClaimVerificationFn — receives (claim, docs) and returns verified/not verified.
     */
    void setClaimVerificationFn(ClaimVerificationFn fn);

    /**
     * @brief Detect ethical perspective gap
     * @param query User query string
     * @param documents Retrieved documents
     * @return Detection result for ethical perspective gap
     */
    DetectionResult detectEthicalPerspectiveGap(
        const std::string& query,
        const std::vector<RetrievedDocument>& documents
    );

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
    
    // Internal detection methods (Phase 1)
    double calculateAverageSimilarity(const std::vector<RetrievedDocument>& docs);
    double calculateQueryCoverage(const std::string& query, 
                                  const std::vector<RetrievedDocument>& docs);
    std::vector<std::string> extractQueryAspects(const std::string& query);
    std::vector<std::string> findMissingAspects(
        const std::string& query,
        const std::vector<RetrievedDocument>& docs
    );
    std::vector<std::string> extractClaims(const std::string& answer);
    bool verifyClaim(const std::string& claim,
                    const std::vector<RetrievedDocument>& docs);
    
    // Phase 2: Token Probability & Perplexity
    double calculatePerplexity(const std::vector<double>& token_probs);
    double calculateSlidingWindowPerplexity(const std::vector<double>& token_probs,
                                           size_t window_size);
    bool detectPerplexityAnomaly(double perplexity, double threshold);
    double calculateConfidenceScore(const std::vector<double>& token_probs);
    std::vector<double> removeOutlierTokens(const std::vector<double>& token_probs,
                                           double zscore_threshold);
    double calculateMovingAverage(const std::vector<double>& values, size_t window_size);
    
    // Phase 2: Self-Consistency Check
    bool checkSelfConsistency(const std::string& query,
                             const std::vector<RetrievedDocument>& docs);
    std::vector<std::string> generateMultipleSamples(const std::string& query,
                                                    const std::vector<RetrievedDocument>& docs,
                                                    size_t num_samples);
    double calculateSemanticSimilarity(const std::string& text1, const std::string& text2);
    double calculateConsistencyScore(const std::vector<std::string>& samples);
    bool detectContradiction(const std::string& text1, const std::string& text2);
    
    // Phase 2: FLARE Active Retrieval
    std::vector<std::string> splitIntoSentences(const std::string& text);
    double monitorSentenceConfidence(const std::string& sentence,
                                    const std::vector<RetrievedDocument>& docs);
    std::string reformulateQuery(const std::string& original_query,
                                const std::string& missing_info);
    std::vector<RetrievedDocument> performDynamicRetrieval(const std::string& query,
                                                           const std::string& tenant_id = {});
    // Ethical gap detection helpers
    bool isEthicalQuery(const std::string& query);
    int countEthicalPerspectives(const std::vector<RetrievedDocument>& docs);
    double calculatePerspectiveDiversity(const std::vector<RetrievedDocument>& docs);
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

    /**
     * @brief Create a production-ready detector with FLARE enabled (v1.4.0+)
     *
     * FLARE (Feedback Loop Active Retrieval) with Token Perplexity Threshold
     * gating is enabled by default. Use this factory for new deployments.
     */
    static std::unique_ptr<KnowledgeGapDetector> createProductionReady();

    /**
     * @brief Create a legacy-compatible detector with FLARE disabled
     *
     * Provides backward compatibility with v1.3.x behaviour where FLARE was
     * disabled. Use this factory when migrating from v1.3 or when FLARE must
     * be explicitly opted out.
     */
    static std::unique_ptr<KnowledgeGapDetector> createLegacy();
};

} // namespace themis::rag::knowledge_gap
