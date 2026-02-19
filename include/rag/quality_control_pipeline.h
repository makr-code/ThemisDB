/**
 * @file quality_control_pipeline.h
 * @brief Quality control pipeline for RAG evaluation
 * 
 * Orchestrates multi-stage evaluation with performance monitoring
 * and quality checks to ensure <500ms target and high accuracy.
 */

#pragma once

#include "rag/rag_judge.h"
#include "rag/llm_judge_client.h"
#include "rag/nli_faithfulness_verifier.h"
#include <string>
#include <vector>
#include <memory>
#include <chrono>

namespace themis::rag::judge {

/**
 * @brief Performance metrics for evaluation
 */
struct PerformanceMetrics {
    double total_time_ms;
    double faithfulness_time_ms;
    double relevance_time_ms;
    double completeness_time_ms;
    double coherence_time_ms;
    double llm_call_time_ms;
    double nli_call_time_ms;
    
    size_t llm_calls_count;
    size_t nli_calls_count;
    size_t cache_hits;
    size_t cache_misses;
    
    bool met_time_target;  ///< Whether <500ms target was met
};

/**
 * @brief Quality check result
 */
struct QualityCheckResult {
    bool passed;
    std::string dimension;
    std::string reason;
    double confidence;
};

/**
 * @brief Quality control pipeline result
 */
struct QCPipelineResult {
    EvaluationResult evaluation;
    PerformanceMetrics metrics;
    std::vector<QualityCheckResult> quality_checks;
    bool overall_quality_passed;
    std::string quality_summary;
};

/**
 * @brief Quality control pipeline for RAG evaluation
 * 
 * Provides:
 * - Multi-stage evaluation orchestration
 * - Performance monitoring (<500ms target)
 * - Quality checks and validation
 * - Adaptive optimization based on constraints
 * - Caching and batching for efficiency
 * 
 * Usage:
 * @code
 *   QualityControlPipeline pipeline;
 *   auto result = pipeline.evaluate(query, docs, answer);
 *   if (result.metrics.met_time_target && result.overall_quality_passed) {
 *       // Use result.evaluation
 *   }
 * @endcode
 */
class QualityControlPipeline {
public:
    /**
     * @brief Configuration for quality control pipeline
     */
    struct Config {
        // Performance targets
        double max_evaluation_time_ms = 500.0;  ///< Target: <500ms
        bool enforce_time_limit = true;         ///< Abort if over time
        
        // Quality thresholds
        double min_confidence = 0.6;            ///< Min confidence for valid eval
        double min_score_variance = 0.0;        ///< Min variance across samples
        
        // Optimization settings
        bool enable_adaptive_sampling = true;   ///< Reduce samples if time is tight
        bool enable_parallel_evaluation = true; ///< Run evaluators in parallel
        bool enable_early_stopping = true;      ///< Stop if high confidence reached
        
        // Component selection
        bool use_faithfulness = true;
        bool use_relevance = true;
        bool use_completeness = true;
        bool use_coherence = true;
        
        // Cache and batch settings
        bool enable_result_caching = true;
        size_t batch_size = 8;
        
        // RAG Judge configuration
        RAGJudgeConfig judge_config;
    };

    /**
     * @brief Construct pipeline with configuration
     */
    QualityControlPipeline();
    explicit QualityControlPipeline(const Config& config);
    
    /**
     * @brief Destructor
     */
    ~QualityControlPipeline();
    
    /**
     * @brief Run full evaluation pipeline with quality control
     * 
     * @param query Original query
     * @param documents Retrieved documents
     * @param answer Generated answer
     * @return Pipeline result with evaluation and metrics
     */
    QCPipelineResult evaluate(
        const std::string& query,
        const std::vector<RetrievedDocument>& documents,
        const std::string& answer
    );
    
    /**
     * @brief Batch evaluate multiple query-answer pairs
     * 
     * @param evaluations Vector of (query, docs, answer) tuples
     * @return Vector of pipeline results
     */
    std::vector<QCPipelineResult> evaluateBatch(
        const std::vector<std::tuple<
            std::string,
            std::vector<RetrievedDocument>,
            std::string
        >>& evaluations
    );
    
    /**
     * @brief Set LLM judge client
     * @param client Shared pointer to LLM judge client
     */
    void setLLMClient(std::shared_ptr<LLMJudgeClient> client);
    
    /**
     * @brief Set NLI verifier
     * @param verifier Shared pointer to NLI verifier
     */
    void setNLIVerifier(std::shared_ptr<NLIFaithfulnessVerifier> verifier);
    
    /**
     * @brief Get aggregate performance statistics
     * @return Performance metrics averaged over all evaluations
     */
    PerformanceMetrics getAggregateMetrics() const;
    
    /**
     * @brief Clear result cache
     */
    void clearCache();
    
    /**
     * @brief Update configuration
     * @param config New configuration
     */
    void setConfig(const Config& config);
    
    /**
     * @brief Get current configuration
     * @return Current configuration
     */
    Config getConfig() const;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace themis::rag::judge
