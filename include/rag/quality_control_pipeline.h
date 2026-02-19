/**
 * @file quality_control_pipeline.h
 * @brief Quality Control Pipeline for RAG outputs
 * 
 * Multi-stage quality control orchestration with Fast, Balanced, and Thorough
 * modes. Integrates LLM-as-Judge, G-Eval, and NLI verification with automatic
 * retry logic.
 */

#pragma once

#include "rag/rag_judge.h"
#include "rag/llm_judge_client.h"
#include "rag/geval_evaluator.h"
#include "rag/nli_faithfulness_verifier.h"
#include <string>
#include <vector>
#include <memory>
#include <functional>

namespace themis::rag::judge {

/**
 * @brief Quality control decision
 */
enum class QCDecision {
    ACCEPT,      ///< Quality meets threshold, accept output
    REJECT,      ///< Quality below threshold, reject output
    RETRY,       ///< Quality marginal, retry with different parameters
    WARN         ///< Quality acceptable but with warnings
};

/**
 * @brief Quality control mode (performance vs thoroughness)
 */
enum class QCMode {
    FAST,        ///< <50ms - single-pass faithfulness check
    BALANCED,    ///< <500ms - multi-dimension evaluation
    THOROUGH     ///< <2s - full evaluation with NLI and G-Eval
};

/**
 * @brief Quality control result
 */
struct QCResult {
    QCDecision decision;             ///< Accept/Reject/Retry/Warn
    double overall_score;            ///< Combined quality score (0-1)
    
    // Dimension scores
    double faithfulness_score;
    double relevance_score;
    double completeness_score;
    double coherence_score;
    
    // Analysis
    std::string explanation;         ///< Why this decision was made
    std::vector<std::string> warnings;  ///< Non-fatal issues
    std::vector<std::string> recommendations;  ///< Improvement suggestions
    
    // Detailed evaluations
    EvaluationResult llm_judge_result;     ///< From LLM judge
    std::vector<GEvalResult> geval_results;  ///< G-Eval scores
    std::vector<NLIResult> nli_results;    ///< NLI verifications
    
    // Metadata
    QCMode mode;                     ///< Mode used for this evaluation
    std::chrono::milliseconds latency;  ///< Total processing time
    int retry_count;                 ///< Number of retries performed
    bool passed_threshold;           ///< Met quality threshold
};

/**
 * @brief Quality Control Pipeline
 * 
 * Orchestrates multi-stage quality control with configurable modes:
 * 
 * Fast Mode (<50ms):
 * - Quick faithfulness check using NLI
 * - Single dimension evaluation
 * 
 * Balanced Mode (<500ms):
 * - Multi-dimension LLM judge evaluation
 * - Selective NLI verification for key claims
 * 
 * Thorough Mode (<2s):
 * - Full LLM judge evaluation
 * - G-Eval probabilistic scoring
 * - Comprehensive NLI claim verification
 * - Citation quality analysis
 */
class QualityControlPipeline {
public:
    /**
     * @brief Configuration for quality control pipeline
     */
    struct Config {
        QCMode default_mode = QCMode::BALANCED;
        
        // Quality thresholds
        double accept_threshold = 0.75;      ///< Accept if score >= this
        double reject_threshold = 0.50;      ///< Reject if score < this
        double warn_threshold = 0.65;        ///< Warn if score < this but >= reject
        
        // Retry configuration
        bool enable_retry = true;
        int max_retries = 2;
        double retry_improvement_threshold = 0.05;  ///< Min improvement to accept retry
        
        // Performance targets (timeouts)
        int fast_timeout_ms = 50;
        int balanced_timeout_ms = 500;
        int thorough_timeout_ms = 2000;
        
        // Feature toggles
        bool enable_nli_verification = true;
        bool enable_geval_scoring = true;
        bool enable_claim_extraction = true;
        bool enable_citation_check = true;
        
        // Continuous learning integration
        bool log_to_continuous_learning = true;
        std::string cl_endpoint;            ///< Continuous learning service endpoint
    };
    
    /**
     * @brief Construct pipeline with configuration
     * @param config Pipeline configuration
     * @param llm_judge_client LLM judge client (optional, creates default if null)
     * @param geval_evaluator G-Eval evaluator (optional)
     * @param nli_verifier NLI verifier (optional)
     */
    QualityControlPipeline();
    explicit QualityControlPipeline(const Config& config);
    QualityControlPipeline(
        const Config& config,
        std::shared_ptr<LLMJudgeClient> llm_judge_client,
        std::shared_ptr<GEvalEvaluator> geval_evaluator,
        std::shared_ptr<NLIFaithfulnessVerifier> nli_verifier
    );
    
    /**
     * @brief Destructor
     */
    ~QualityControlPipeline();
    
    /**
     * @brief Run quality control on RAG output
     * @param query Original query
     * @param documents Retrieved documents
     * @param generated_answer Answer to evaluate
     * @param mode Quality control mode (uses default if not specified)
     * @return Quality control result with decision
     */
    QCResult runQualityControl(
        const std::string& query,
        const std::vector<RetrievedDocument>& documents,
        const std::string& generated_answer,
        QCMode mode = QCMode::BALANCED
    );
    
    /**
     * @brief Run quality control with automatic mode selection
     * Selects mode based on query complexity and available time budget
     */
    QCResult runAdaptiveQC(
        const std::string& query,
        const std::vector<RetrievedDocument>& documents,
        const std::string& generated_answer,
        int time_budget_ms = 500
    );
    
    /**
     * @brief Batch quality control on multiple outputs
     * @param inputs Vector of evaluation inputs
     * @param mode Quality control mode
     * @return Vector of QC results
     */
    std::vector<QCResult> batchQualityControl(
        const std::vector<EvaluationInput>& inputs,
        QCMode mode = QCMode::BALANCED
    );
    
    /**
     * @brief Set callback for quality control completion
     * @param callback Function called after each QC run
     */
    void setQCCallback(std::function<void(const QCResult&)> callback);
    
    /**
     * @brief Get current configuration
     */
    Config getConfig() const;
    
    /**
     * @brief Update configuration
     */
    void setConfig(const Config& config);
    
    /**
     * @brief Get performance statistics
     */
    struct Statistics {
        size_t total_evaluations = 0;
        size_t accepted = 0;
        size_t rejected = 0;
        size_t retried = 0;
        size_t warned = 0;
        double avg_latency_ms = 0.0;
        double avg_score = 0.0;
        std::unordered_map<QCMode, size_t> mode_usage;
    };
    Statistics getStatistics() const;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
    
    // Stage implementations
    QCResult runFastMode(const EvaluationInput& input);
    QCResult runBalancedMode(const EvaluationInput& input);
    QCResult runThoroughMode(const EvaluationInput& input);
    
    // Decision logic
    QCDecision makeDecision(double overall_score, const Config& config);
    bool shouldRetry(const QCResult& result, int attempt_num);
    
    // Continuous learning integration
    void logToContinuousLearning(const QCResult& result);
    
    // Performance monitoring
    void recordEvaluation(const QCResult& result);
    bool checkTimeout(std::chrono::steady_clock::time_point start, QCMode mode);
};

/**
 * @brief Factory for creating quality control pipelines
 */
class QualityControlPipelineFactory {
public:
    /**
     * @brief Create pipeline for fast mode
     */
    static std::unique_ptr<QualityControlPipeline> createFast();
    
    /**
     * @brief Create pipeline for balanced mode
     */
    static std::unique_ptr<QualityControlPipeline> createBalanced();
    
    /**
     * @brief Create pipeline for thorough mode
     */
    static std::unique_ptr<QualityControlPipeline> createThorough();
    
    /**
     * @brief Create custom pipeline
     */
    static std::unique_ptr<QualityControlPipeline> create(
        const QualityControlPipeline::Config& config
    );
};

} // namespace themis::rag::judge
