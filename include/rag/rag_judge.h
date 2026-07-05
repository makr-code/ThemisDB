/**
 * @file rag_judge.h
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
#include <chrono>
#include <cmath>
#include <optional>

namespace themis::rag::judge {

/**
 * @brief Evaluation dimension for scoring
 */
enum class EvaluationDimension {
    FAITHFULNESS,         ///< Answer supported by retrieved documents
    RELEVANCE,            ///< Answer addresses the query
    COMPLETENESS,         ///< All query aspects covered
    COHERENCE,            ///< Logical and well-structured
    ETHICAL_COMPLIANCE,   ///< Respects ethical guidelines and human autonomy
    OVERALL               ///< Weighted combination
};

/**
 * @brief Voting strategy for ensemble judges
 */
enum class VotingStrategy {
    MAJORITY_VOTING,      ///< Simple majority
    WEIGHTED_AVERAGE,     ///< Weight by judge confidence
    CONFIDENCE_WEIGHTED,  ///< Weight by calibrated confidence
    HIERARCHICAL          ///< Cascading with disagreement resolution
};

/**
 * @brief Evaluation mode
 */
enum class EvaluationMode {
    FAST,        ///< Quick single-dimension check (~100ms)
    BALANCED,    ///< Multi-dimension evaluation (~500ms)
    THOROUGH     ///< Full evaluation with CoT and verification (~2s)
};

/**
 * @brief Bias score for a document (Wave A3: Fairness & Bias Detection).
 *
 * Quantifies detected biases across multiple dimensions (gender, occupational,
 * ethnicity, intersectional). Added in v1.20.0 for ethical RAG evaluation.
 *
 * @reference Bolukbasi et al. (2016) "Man is to Computer Programmer
 *            as Woman is to Homemaker: Debiasing Word Embeddings"
 *            NeurIPS 2016, arXiv:1607.06520
 */
struct BiasScore {
    /// Overall bias magnitude (0.0 = no bias, 1.0 = extreme bias)
    double overall_score = 0.0;

    /// Gender bias component (0.0–1.0)
    double gender_bias = 0.0;

    /// Occupational stereotype bias (0.0–1.0)
    double occupational_bias = 0.0;

    /// Ethnicity/cultural bias (0.0–1.0)
    double ethnicity_bias = 0.0;

    /// Stereotype density: freq(biased_terms) / total_terms in passage
    double stereotype_density = 0.0;

    /// True if this document likely contains problematic bias
    bool flagged = false;

    /// Confidence in the bias score (0.0–1.0)
    double confidence = 0.0;
};

/**
 * @brief Retrieved document for evaluation context
 */
struct RetrievedDocument {
    std::string id;
    std::string content;
    double similarity_score;
    std::unordered_map<std::string, std::string> metadata;

    /// Optional bias score (Wave A3: populated by FairnessDetector if enabled)
    std::optional<BiasScore> bias_score;
};

/**
 * @brief Complete evaluation result
 */
struct EvaluationResult {
    // Dimension scores (0.0 - 1.0)
    double faithfulness_score;
    double relevance_score;
    double completeness_score;
    double coherence_score;
    double ethical_compliance_score;             ///< NEW: Ethical compliance
    double overall_score;
    
    // Detailed analysis
    std::string explanation;                     ///< Chain-of-thought reasoning
    std::vector<std::string> verified_claims;    ///< Claims supported by documents
    std::vector<std::string> unverified_claims;  ///< Claims without support
    std::vector<std::string> improvements;       ///< Suggested improvements
    
    // Ethical evaluation details
    std::vector<std::string> ethical_violations; ///< NEW: Detected ethical issues
    bool respects_human_autonomy;                ///< NEW: Respects user autonomy
    bool shows_moral_diversity;                  ///< NEW: Shows diverse perspectives
    bool has_ethical_citations;                  ///< NEW: Cites sources for moral claims

    // AI Safety / reliability fields
    /// True when config.enable_prompt_injection_screening is true and documents were non-empty
    bool injection_screened = false;
    /// Number of injection findings across all context documents; 0 when injection_screened is false
    size_t injection_findings_count = 0;
    /// True when evaluation was blocked because at least one finding had HIGH or higher severity
    bool injection_blocked = false;

    // Quality assessment
    bool passed_quality_threshold;               ///< Meets minimum quality
    double confidence;                           ///< Judge's confidence in evaluation
    
    // Metadata
    std::chrono::milliseconds evaluation_time;
    std::string judge_model;
};

/**
 * @brief Comparison result for pairwise evaluation
 */
struct ComparisonResult {
    enum class Winner {
        ANSWER_A,
        ANSWER_B,
        TIE
    };
    
    Winner winner;
    std::string reasoning;
    double confidence;
    
    // Per-dimension comparison
    std::unordered_map<std::string, Winner> dimension_winners;
};

/**
 * @brief Configuration for RAG Judge
 */
struct RAGJudgeConfig {
    EvaluationMode mode = EvaluationMode::BALANCED;
    
    // Model selection
    std::string judge_model = "default";         ///< LLM model for judging
    bool use_chain_of_thought = true;           ///< Enable CoT reasoning
    
    // Scoring weights (must sum to 1.0)
    double faithfulness_weight = 0.35;
    double relevance_weight = 0.25;
    double completeness_weight = 0.15;
    double coherence_weight = 0.10;
    double ethical_compliance_weight = 0.15;     ///< NEW: Weight for ethical compliance
    
    // Quality thresholds
    double quality_threshold = 0.7;              ///< Minimum overall score
    double faithfulness_threshold = 0.8;         ///< Critical for factual accuracy
    double ethical_compliance_threshold = 0.7;   ///< NEW: Minimum ethical compliance
    
    // Ethical compliance settings
    bool enable_ethical_evaluation = true;       ///< NEW: Enable ethical evaluation
    bool ethical_veto_power = true;              ///< NEW: Ethical compliance can VETO
    
    // Ethical compliance weights (must sum to 1.0)
    double autonomy_respect_weight = 0.40;       ///< NEW: Respect for human autonomy
    double moral_diversity_weight = 0.30;        ///< NEW: Moral perspective diversity
    double citation_quality_weight = 0.30;       ///< NEW: Citation for ethical claims
    
    // Ethical detection thresholds
    int bias_detection_threshold = 5;            ///< NEW: Number of absolute statements for bias detection
    
    // Advanced options
    bool enable_claim_verification = true;       ///< Verify each claim
    bool enable_citation_check = true;           ///< Check source attribution
    size_t max_claims_to_verify = 10;          ///< Limit for performance
    
    // NEW: Quality Control Integration
    bool use_llm_judge_client = false;           ///< Use new LLM Judge Client (requires InferenceEngineEnhanced)
    bool use_nli_verifier = true;                ///< Use NLI model for claim verification
    bool use_geval_scoring = false;              ///< Use G-Eval probabilistic scoring
    bool use_quality_control_pipeline = false;   ///< Use full QC pipeline instead of basic judge
    
    // Performance
    bool cache_evaluations = true;               ///< Cache results for identical inputs
    bool async_evaluation = false;               ///< Run evaluation asynchronously
    size_t batch_size = 8;                      ///< For batch processing

    // AI Safety: prompt-injection screening
    bool enable_prompt_injection_screening = true;  ///< Scan retrieved docs before evaluation
    bool block_on_high_severity_injection  = true;  ///< Block (skip LLM eval) on HIGH+ severity

    // AI Safety: bias tracking
    bool enable_bias_tracking = true;               ///< Track evaluations for bias analysis
    
    /**
     * @brief Validate that weights sum to approximately 1.0
     * @return true if weights are valid, false otherwise
     */
    bool validateWeights() const {
        const double epsilon = 0.001;
        double main_sum = faithfulness_weight + relevance_weight + 
                         completeness_weight + coherence_weight + 
                         ethical_compliance_weight;
        double ethical_sum = autonomy_respect_weight + moral_diversity_weight + 
                           citation_quality_weight;
        
        bool main_valid = std::abs(main_sum - 1.0) < epsilon;
        bool ethical_valid = std::abs(ethical_sum - 1.0) < epsilon;
        
        return main_valid && ethical_valid;
    }
};

/**
 * @brief Input for evaluation
 */
struct EvaluationInput {
    std::string query;
    std::vector<RetrievedDocument> documents;
    std::string generated_answer;
    std::unordered_map<std::string, std::string> metadata;
    std::string tenant_id; ///< Tenant identifier for cache isolation; empty means global/anonymous
};

/**
 * @brief RAG test case for batch evaluation
 */
struct RAGTestCase {
    std::string test_id;
    std::string query;
    std::vector<RetrievedDocument> documents;
    std::string generated_answer;
    std::string expected_answer;                 ///< Optional ground truth
    std::unordered_map<std::string, double> expected_scores; ///< Optional
};

/**
 * @brief Main RAG Judge class
 * 
 * Evaluates RAG system outputs using LLM-based assessment across
 * multiple quality dimensions. Supports single evaluation, pairwise
 * comparison, and batch processing.
 */
class RAGJudge {
public:
    /**
     * @brief Default constructor for judge
     */
    RAGJudge();
    
    /**
     * @brief Construct judge with configuration
     * @param config Evaluation configuration
     */
    explicit RAGJudge(const RAGJudgeConfig& config);
    
    /**
     * @brief Destructor
     */
    ~RAGJudge();
    
    /**
     * @brief Evaluate a single RAG output
     * @param query User query
     * @param documents Retrieved documents
     * @param generated_answer Generated answer to evaluate
     * @return Evaluation result with scores and analysis
     */
    EvaluationResult evaluate(
        const std::string& query,
        const std::vector<RetrievedDocument>& documents,
        const std::string& generated_answer
    );
    
    /**
     * @brief Evaluate a single RAG output with custom configuration
     * @param query User query
     * @param documents Retrieved documents
     * @param generated_answer Generated answer to evaluate
     * @param config Optional override configuration
     * @return Evaluation result with scores and analysis
     */
    EvaluationResult evaluate(
        const std::string& query,
        const std::vector<RetrievedDocument>& documents,
        const std::string& generated_answer,
        const RAGJudgeConfig& config
    );
    
    /**
     * @brief Evaluate a structured input
     * @param input Complete evaluation input
     * @return Evaluation result
     */
    EvaluationResult evaluate(const EvaluationInput& input);
    
    /**
     * @brief Compare two answers pairwise
     * @param query User query
     * @param documents Retrieved documents
     * @param answer_a First answer
     * @param answer_b Second answer
     * @return Comparison result indicating winner
     */
    ComparisonResult compare(
        const std::string& query,
        const std::vector<RetrievedDocument>& documents,
        const std::string& answer_a,
        const std::string& answer_b
    );
    
    /**
     * @brief Batch evaluate multiple test cases
     * @param test_cases Vector of test cases
     * @return Vector of evaluation results
     */
    std::vector<EvaluationResult> batchEvaluate(
        const std::vector<RAGTestCase>& test_cases
    );
    
    /**
     * @brief Evaluate specific dimension only
     * @param dimension Dimension to evaluate
     * @param input Evaluation input
     * @return Score for that dimension (0-1)
     */
    double evaluateDimension(
        EvaluationDimension dimension,
        const EvaluationInput& input
    );
    
    /**
     * @brief Update configuration
     * @param config New configuration
     */
    void setConfig(const RAGJudgeConfig& config);
    
    /**
     * @brief Get current configuration
     * @return Current configuration
     */
    RAGJudgeConfig getConfig() const;
    
    /**
     * @brief Set callback for evaluation completion
     * @param callback Function to call after each evaluation
     */
    void setEvaluationCallback(
        std::function<void(const EvaluationResult&)> callback
    );
    
    /**
     * @brief Clear evaluation cache
     */
    void clearCache();

    /**
     * @brief Summary of bias analysis accumulated over evaluation history.
     *
     * Returned by getBiasAnalysis(). Self-contained; does not require
     * inclusion of bias_detector.h by the caller.
     */
    struct BiasAnalysisSummary {
        bool has_significant_length_bias   = false; ///< Correlation between score and answer length
        bool has_significant_position_bias = false; ///< Preference for first/second doc position
        double length_bias_magnitude       = 0.0;   ///< Absolute Pearson correlation [0,1]
        double position_bias_magnitude     = 0.0;   ///< Deviation from 50/50 position split [0,1]
        size_t samples_analyzed            = 0;     ///< Number of evaluations included
    };

    /**
     * @brief Run bias analysis over all accumulated evaluations.
     *
     * Only meaningful when config.enable_bias_tracking is true.
     * Returns a zeroed-out summary when fewer than the minimum required
     * samples have been accumulated.
     *
     * @return BiasAnalysisSummary for the current evaluation history.
     */
    BiasAnalysisSummary getBiasAnalysis() const;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
    RAGJudgeConfig getConfigSnapshot() const;
    EvaluationResult evaluateWithConfig(const EvaluationInput& input, const RAGJudgeConfig& config);
    
    // Internal evaluation methods
    double evaluateFaithfulness(const EvaluationInput& input);
    double evaluateRelevance(const EvaluationInput& input);
    double evaluateCompleteness(const EvaluationInput& input);
    double evaluateCoherence(const EvaluationInput& input);
    double evaluateEthicalCompliance(const EvaluationInput& input);
    
    // Ethical compliance sub-evaluations
    double evaluateAutonomyRespect(const EvaluationInput& input);
    double evaluateMoralDiversity(const EvaluationInput& input);
    double evaluateCitationQuality(const EvaluationInput& input);
    
    // Ethical detection helpers
    bool detectPatronizingLanguage(const std::string& text);
    bool checkChoicePreservation(const std::string& text);
    int countMoralPerspectives(const std::string& text);
    bool detectBias(const std::string& text);
    bool hasEthicalCitations(const std::string& text);
    
    std::vector<std::string> extractClaims(const std::string& answer);
    std::vector<std::string> extractClaimsViaLLM(const std::string& answer);
    std::vector<std::string> extractClaimsViaHeuristic(const std::string& answer);

    bool verifyClaimAgainstDocuments(
        const std::string& claim,
        const std::vector<RetrievedDocument>& documents
    );
    bool verifyClaimViaNLI(
        const std::string& claim,
        const std::vector<RetrievedDocument>& documents
    );
    bool verifyClaimViaLLM(
        const std::string& claim,
        const std::vector<RetrievedDocument>& documents
    );
    bool verifyClaimViaSemantic(
        const std::string& claim,
        const std::vector<RetrievedDocument>& documents
    );

    std::vector<std::string> tokenizeForMatching(const std::string& text);
    double calculateTermOverlap(
        const std::vector<std::string>& terms1,
        const std::vector<std::string>& terms2
    );
    
    std::string generateEvaluationPrompt(
        const EvaluationInput& input,
        EvaluationDimension dimension
    );
    
    double parseScoreFromResponse(const std::string& response);
    std::string extractExplanation(const std::string& response);
};

/**
 * @brief Ensemble of multiple judges for robust evaluation
 */
class JudgeEnsemble {
public:
    /**
     * @brief Construct ensemble with judges
     * @param judges Vector of judge instances
     * @param strategy Voting strategy for combining results
     */
    JudgeEnsemble(
        std::vector<std::shared_ptr<RAGJudge>> judges,
        VotingStrategy strategy = VotingStrategy::WEIGHTED_AVERAGE
    );

    /**
     * @brief Destructor (virtual for polymorphic cleanup)
     */
    virtual ~JudgeEnsemble();
    
    /**
     * @brief Move constructor
     * @param other Source ensemble (ownership transferred)
     */
    JudgeEnsemble(JudgeEnsemble&& other) noexcept;
    
    /**
     * @brief Move assignment operator
     * @param other Source ensemble (ownership transferred)
     * @return Reference to this
     */
    JudgeEnsemble& operator=(JudgeEnsemble&& other) noexcept;
    
    // Delete copy operations to enforce move semantics
    JudgeEnsemble(const JudgeEnsemble&) = delete;
    JudgeEnsemble& operator=(const JudgeEnsemble&) = delete;
    
    /**
     * @brief Evaluate with ensemble
     * @param input Evaluation input
     * @return Combined evaluation result
     */
    EvaluationResult evaluateWithEnsemble(const EvaluationInput& input);
    
    /**
     * @brief Compare with ensemble
     * @param query User query
     * @param documents Retrieved documents
     * @param answer_a First answer
     * @param answer_b Second answer
     * @return Combined comparison result
     */
    ComparisonResult compareWithEnsemble(
       const std::string& query,
       const std::vector<RetrievedDocument>& documents,
       const std::string& answer_a,
       const std::string& answer_b
    );
    
    /**
     * @brief Set voting strategy
     * @param strategy New voting strategy
     */
    void setVotingStrategy(VotingStrategy strategy);

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
    
    EvaluationResult combineResults(
        const std::vector<EvaluationResult>& results,
        VotingStrategy strategy
    );
};

/**
 * @brief Factory for creating judges with different configurations
 */
class RAGJudgeFactory {
public:
    /**
     * @brief Create a fast judge (single-dimension check)
     */
    static std::unique_ptr<RAGJudge> createFast();
    
    /**
     * @brief Create a balanced judge (multi-dimension)
     */
    static std::unique_ptr<RAGJudge> createBalanced();
    
    /**
     * @brief Create a thorough judge (full evaluation)
     */
    static std::unique_ptr<RAGJudge> createThorough();
    
    /**
     * @brief Create a custom configured judge
     */
    static std::unique_ptr<RAGJudge> create(const RAGJudgeConfig& config);
    
    /**
     * @brief Create an ensemble of judges
     * @param count Number of judges in ensemble
     * @param strategy Voting strategy
     */
    static std::unique_ptr<JudgeEnsemble> createEnsemble(
        size_t count = 3,
        VotingStrategy strategy = VotingStrategy::WEIGHTED_AVERAGE
    );
};

/**
 * @brief Utilities for evaluation metrics and analysis
 */
namespace metrics {

/**
 * @brief Calculate agreement between judges
 * @param results Vector of evaluation results
 * @return Agreement score (0-1)
 */
double calculateInterJudgeAgreement(const std::vector<EvaluationResult>& results);

/**
 * @brief Calculate Cohen's Kappa for judge consistency
 */
double calculateCohensKappa(
    const std::vector<EvaluationResult>& judge1_results,
    const std::vector<EvaluationResult>& judge2_results
);

/**
 * @brief Calculate calibration error
 * @param predictions Judge predictions
 * @param ground_truth Ground truth values
 * @return Expected Calibration Error (ECE)
 */
double calculateCalibrationError(
    const std::vector<double>& predictions,
    const std::vector<double>& ground_truth
);

} // namespace metrics

} // namespace themis::rag::judge
