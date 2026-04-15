/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            quality_control_pipeline.h                         ║
  Version:         0.0.46                                             ║
  Last Modified:   2026-04-15 18:04:51                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     338                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/**
 * @file quality_control_pipeline.h
 * @brief Multi-stage quality control pipeline for RAG-generated answers
 * 
 * Orchestrates multiple quality checks:
 * 1. LLM-as-Judge evaluation (multiple dimensions)
 * 2. G-Eval probabilistic scoring
 * 3. NLI faithfulness verification
 * 4. Quality gates with automatic retry
 * 5. Feedback to continuous learning
 */

#pragma once

#include "rag/llm_judge_client.h"
#include "rag/geval_evaluator.h"
#include "rag/nli_faithfulness_verifier.h"
#include "rag/rag_judge.h"
#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <chrono>

namespace themis::rag::judge {

/**
 * @brief Quality control stage
 */
enum class QualityStage {
    FAST_SCREENING,     ///< Quick initial check (<50ms)
    BALANCED_EVAL,      ///< Multi-dimension evaluation (<500ms)
    THOROUGH_VERIFY,    ///< Full verification with NLI (<2s)
    CONTINUOUS_LEARN    ///< Feedback to learning system
};

/**
 * @brief Quality gate result
 */
enum class QualityGateStatus {
    PASSED,             ///< Meets all thresholds
    FAILED,             ///< Failed quality checks
    RETRY_NEEDED,       ///< Should retry generation
    ESCALATE            ///< Needs human review
};

/**
 * @brief Dimension-specific quality score
 */
struct DimensionScore {
    std::string dimension;
    double score;              ///< 0-1
    double confidence;         ///< 0-1
    std::string method;        ///< "llm", "geval", "nli"
    std::string explanation;
};

/**
 * @brief Quality check result
 */
struct QualityCheckResult {
    QualityGateStatus status;
    double overall_score;      ///< Aggregate score (0-1)
    double confidence;         ///< Aggregate confidence (0-1)
    
    // Dimension scores
    std::vector<DimensionScore> dimension_scores;
    
    // Stage timing
    std::chrono::milliseconds fast_stage_time;
    std::chrono::milliseconds balanced_stage_time;
    std::chrono::milliseconds thorough_stage_time;
    std::chrono::milliseconds total_time;
    
    // Failure reasons (if failed)
    std::vector<std::string> failure_reasons;
    
    // Recommendations
    bool should_retry;
    std::vector<std::string> improvement_suggestions;
    
    // Learning feedback
    bool sent_to_learning_system;
    std::string learning_feedback_id;

    // Citation coverage (filled by thorough stage when enable_citation_check is true)
    double citation_coverage = 0.0;  ///< Fraction of answer sentences with at least one source citation [0, 1]
};

/**
 * @brief Quality control pipeline
 * 
 * Multi-stage pipeline for comprehensive quality control:
 * 
 * Stage 1: Fast Screening (<50ms)
 * - Quick LLM judge on critical dimension (faithfulness)
 * - Early rejection of clearly bad answers
 * 
 * Stage 2: Balanced Evaluation (<500ms)
 * - Multi-dimension LLM judging
 * - G-Eval probabilistic scoring
 * - Aggregate quality assessment
 * 
 * Stage 3: Thorough Verification (<2s)
 * - NLI faithfulness verification
 * - Claim-level analysis
 * - Citation checking
 * 
 * Stage 4: Continuous Learning
 * - Send feedback to learning orchestrator
 * - Update quality models
 * - Track long-term quality trends
 */
class QualityControlPipeline {
public:
    /**
     * @brief Configuration for quality pipeline
     */
    struct Config {
        // Stage enablement
        bool enable_fast_stage = true;
        bool enable_balanced_stage = true;
        bool enable_thorough_stage = true;
        bool enable_learning_feedback = true;
        
        // Quality thresholds
        double fast_stage_threshold = 0.6;      ///< Min score to pass fast stage
        double balanced_stage_threshold = 0.7;  ///< Min score to pass balanced
        double thorough_stage_threshold = 0.8;  ///< Min score for production
        
        // Dimension weights
        double faithfulness_weight = 0.35;
        double relevance_weight = 0.25;
        double completeness_weight = 0.15;
        double coherence_weight = 0.15;
        double ethical_weight = 0.10;
        
        // Retry policy
        bool enable_auto_retry = true;
        int max_retries = 2;
        double retry_threshold = 0.5;           ///< Min score to allow retry
        
        // Citation checking (Stage 3 thorough verification)
        bool enable_citation_check = true;  ///< Run citation-coverage check via CitationHighlighter

        // Performance targets
        int fast_stage_timeout_ms = 50;
        int balanced_stage_timeout_ms = 500;
        int thorough_stage_timeout_ms = 2000;
        
        // Learning feedback
        std::string learning_orchestrator_url;
        bool enable_async_feedback = true;
    };
    
    /**
     * @brief Construct pipeline with default config
     */
    QualityControlPipeline();
    
    /**
     * @brief Construct pipeline with custom config
     * @param config Pipeline configuration
     */
    explicit QualityControlPipeline(const Config& config);
    
    /**
     * @brief Destructor
     */
    ~QualityControlPipeline();
    
    /**
     * @brief Run quality control on generated answer
     * @param query Original user query
     * @param answer Generated answer to check
     * @param documents Retrieved documents used
     * @return Quality check result with pass/fail and details
     */
    QualityCheckResult runQualityControl(
        const std::string& query,
        const std::string& answer,
        const std::vector<RetrievedDocument>& documents
    );
    
    /**
     * @brief Run specific stage only
     * @param stage Stage to run
     * @param query User query
     * @param answer Generated answer
     * @param documents Retrieved documents
     * @return Quality check result for that stage
     */
    QualityCheckResult runStage(
        QualityStage stage,
        const std::string& query,
        const std::string& answer,
        const std::vector<RetrievedDocument>& documents
    );
    
    /**
     * @brief Set callback for quality gate failures
     * @param callback Function to call when quality gate fails
     */
    void setFailureCallback(
        std::function<void(const QualityCheckResult&)> callback
    );
    
    /**
     * @brief Set callback for learning feedback
     * @param callback Function to call when sending feedback
     */
    void setLearningCallback(
        std::function<void(const std::string&, const QualityCheckResult&)> callback
    );
    
    /**
     * @brief Set LLM judge client
     * @param client Shared pointer to LLM judge client
     */
    void setLLMJudgeClient(std::shared_ptr<LLMJudgeClient> client);
    
    /**
     * @brief Set G-Eval evaluator
     * @param evaluator Shared pointer to G-Eval evaluator
     */
    void setGEvalEvaluator(std::shared_ptr<GEvalEvaluator> evaluator);
    
    /**
     * @brief Set NLI verifier
     * @param verifier Shared pointer to NLI verifier
     */
    void setNLIVerifier(std::shared_ptr<NLIFaithfulnessVerifier> verifier);
    
    /**
     * @brief Get current configuration
     * @return Current config
     */
    Config getConfig() const;
    
    /**
     * @brief Update configuration
     * @param config New configuration
     */
    void setConfig(const Config& config);
    
    /**
     * @brief Get pipeline statistics
     * @return JSON with statistics
     */
    std::string getStatistics() const;
    
    /**
     * @brief Reset statistics
     */
    void resetStatistics();

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
    
    // Stage implementations
    QualityCheckResult runFastStage(
        const std::string& query,
        const std::string& answer,
        const std::vector<RetrievedDocument>& documents
    );
    
    QualityCheckResult runBalancedStage(
        const std::string& query,
        const std::string& answer,
        const std::vector<RetrievedDocument>& documents
    );
    
    QualityCheckResult runThoroughStage(
        const std::string& query,
        const std::string& answer,
        const std::vector<RetrievedDocument>& documents
    );
    
    void sendLearningFeedback(
        const std::string& query,
        const std::string& answer,
        const QualityCheckResult& result
    );
    
    double computeOverallScore(const std::vector<DimensionScore>& scores);
    QualityGateStatus determineStatus(double score, QualityStage stage);
};

/**
 * @brief Quality control pipeline factory
 */
class QualityPipelineFactory {
public:
    /**
     * @brief Create fast pipeline (screening only)
     */
    static std::unique_ptr<QualityControlPipeline> createFast();
    
    /**
     * @brief Create balanced pipeline (fast + balanced stages)
     */
    static std::unique_ptr<QualityControlPipeline> createBalanced();
    
    /**
     * @brief Create thorough pipeline (all stages)
     */
    static std::unique_ptr<QualityControlPipeline> createThorough();
    
    /**
     * @brief Create production pipeline with learning feedback
     */
    static std::unique_ptr<QualityControlPipeline> createProduction();
};

} // namespace themis::rag::judge
