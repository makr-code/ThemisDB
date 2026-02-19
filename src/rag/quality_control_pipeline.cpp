/**
 * @file quality_control_pipeline.cpp
 * @brief Implementation of quality control pipeline for RAG evaluation
 */

#include "rag/quality_control_pipeline.h"
#include "rag/faithfulness_evaluator.h"
#include "rag/relevance_evaluator.h"
#include "rag/completeness_evaluator.h"
#include "rag/coherence_evaluator.h"
#include "utils/logger.h"
#include <algorithm>
#include <numeric>
#include <sstream>
#include <unordered_map>

namespace themis::rag::judge {

// ═══════════════════════════════════════════════════════════
// Implementation
// ═══════════════════════════════════════════════════════════

struct QualityControlPipeline::Impl {
    Config config;
    
    // Components
    std::unique_ptr<RAGJudge> judge;
    std::shared_ptr<LLMJudgeClient> llm_client;
    std::shared_ptr<NLIFaithfulnessVerifier> nli_verifier;
    
    // Statistics
    std::vector<PerformanceMetrics> all_metrics;
    std::unordered_map<std::string, QCPipelineResult> result_cache;
    
    Impl(const Config& cfg) : config(cfg) {
        // Initialize RAG Judge with config
        judge = std::make_unique<RAGJudge>(config.judge_config);
        
        THEMIS_INFO("QualityControlPipeline initialized with time target: {:.0f}ms", 
                   config.max_evaluation_time_ms);
    }
    
    /**
     * @brief Generate cache key for query-answer pair
     */
    std::string getCacheKey(
        const std::string& query,
        const std::string& answer
    ) {
        // Simple hash - in production, use better hashing
        return query + "|||" + answer;
    }
    
    /**
     * @brief Check if evaluation meets quality criteria
     */
    std::vector<QualityCheckResult> performQualityChecks(
        const EvaluationResult& result
    ) {
        std::vector<QualityCheckResult> checks;
        
        // Check 1: Overall score is in valid range
        if (result.overall_score < 0.0 || result.overall_score > 1.0) {
            checks.push_back({
                false,
                "overall",
                "Overall score out of range [0, 1]",
                0.0
            });
        } else {
            checks.push_back({
                true,
                "overall",
                "Overall score in valid range",
                1.0
            });
        }
        
        // Check 2: Individual dimension scores are valid
        for (const auto& [dim, score] : result.dimension_scores) {
            if (score < 0.0 || score > 1.0) {
                checks.push_back({
                    false,
                    dim,
                    "Dimension score out of range [0, 1]",
                    0.0
                });
            } else {
                checks.push_back({
                    true,
                    dim,
                    "Dimension score valid",
                    1.0
                });
            }
        }
        
        // Check 3: Confidence is above threshold
        if (result.confidence < config.min_confidence) {
            checks.push_back({
                false,
                "confidence",
                "Confidence below threshold: " + std::to_string(result.confidence),
                result.confidence
            });
        } else {
            checks.push_back({
                true,
                "confidence",
                "Confidence meets threshold",
                result.confidence
            });
        }
        
        return checks;
    }
    
    /**
     * @brief Calculate performance metrics
     */
    PerformanceMetrics calculateMetrics(
        const std::chrono::steady_clock::time_point& start_time,
        const std::chrono::steady_clock::time_point& end_time
    ) {
        PerformanceMetrics metrics;
        
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(
            end_time - start_time
        );
        metrics.total_time_ms = duration.count() / 1000.0;
        
        // Component times (would be tracked individually in full implementation)
        metrics.faithfulness_time_ms = 0.0;
        metrics.relevance_time_ms = 0.0;
        metrics.completeness_time_ms = 0.0;
        metrics.coherence_time_ms = 0.0;
        metrics.llm_call_time_ms = 0.0;
        metrics.nli_call_time_ms = 0.0;
        
        // Counts
        metrics.llm_calls_count = 0;
        metrics.nli_calls_count = 0;
        metrics.cache_hits = 0;
        metrics.cache_misses = 0;
        
        // Check if time target was met
        metrics.met_time_target = metrics.total_time_ms <= config.max_evaluation_time_ms;
        
        return metrics;
    }
    
    /**
     * @brief Generate quality summary
     */
    std::string generateQualitySummary(
        const std::vector<QualityCheckResult>& checks,
        const PerformanceMetrics& metrics
    ) {
        std::ostringstream summary;
        
        size_t passed = std::count_if(checks.begin(), checks.end(),
                                     [](const auto& c) { return c.passed; });
        
        summary << "Quality Checks: " << passed << "/" << checks.size() << " passed\n";
        summary << "Performance: " << metrics.total_time_ms << "ms ";
        summary << (metrics.met_time_target ? "(PASS)" : "(FAIL - exceeds 500ms target)") << "\n";
        
        // List failed checks
        for (const auto& check : checks) {
            if (!check.passed) {
                summary << "  - FAILED [" << check.dimension << "]: " << check.reason << "\n";
            }
        }
        
        return summary.str();
    }
};

// ═══════════════════════════════════════════════════════════
// Constructor & Destructor
// ═══════════════════════════════════════════════════════════

QualityControlPipeline::QualityControlPipeline()
    : QualityControlPipeline(Config{}) {
}

QualityControlPipeline::QualityControlPipeline(const Config& config)
    : impl_(std::make_unique<Impl>(config)) {
}

QualityControlPipeline::~QualityControlPipeline() = default;

// ═══════════════════════════════════════════════════════════
// Public Methods
// ═══════════════════════════════════════════════════════════

QCPipelineResult QualityControlPipeline::evaluate(
    const std::string& query,
    const std::vector<RetrievedDocument>& documents,
    const std::string& answer
) {
    QCPipelineResult result;
    
    // Check cache first
    if (impl_->config.enable_result_caching) {
        std::string cache_key = impl_->getCacheKey(query, answer);
        auto it = impl_->result_cache.find(cache_key);
        if (it != impl_->result_cache.end()) {
            THEMIS_DEBUG("Returning cached evaluation result");
            return it->second;
        }
    }
    
    // Start timing
    auto start_time = std::chrono::steady_clock::now();
    
    try {
        // Run evaluation through RAG Judge
        result.evaluation = impl_->judge->evaluate(query, documents, answer);
        
        // End timing
        auto end_time = std::chrono::steady_clock::now();
        
        // Calculate metrics
        result.metrics = impl_->calculateMetrics(start_time, end_time);
        
        // Perform quality checks
        result.quality_checks = impl_->performQualityChecks(result.evaluation);
        
        // Determine overall quality pass/fail
        result.overall_quality_passed = std::all_of(
            result.quality_checks.begin(),
            result.quality_checks.end(),
            [](const auto& c) { return c.passed; }
        );
        
        // Generate summary
        result.quality_summary = impl_->generateQualitySummary(
            result.quality_checks,
            result.metrics
        );
        
        // Store metrics
        impl_->all_metrics.push_back(result.metrics);
        
        // Cache result
        if (impl_->config.enable_result_caching) {
            std::string cache_key = impl_->getCacheKey(query, answer);
            impl_->result_cache[cache_key] = result;
        }
        
        THEMIS_INFO("Evaluation completed in {:.2f}ms (target: {:.0f}ms, passed: {})",
                   result.metrics.total_time_ms,
                   impl_->config.max_evaluation_time_ms,
                   result.metrics.met_time_target);
        
    } catch (const std::exception& e) {
        THEMIS_ERROR("Evaluation failed: {}", e.what());
        
        // Return error result
        auto end_time = std::chrono::steady_clock::now();
        result.metrics = impl_->calculateMetrics(start_time, end_time);
        result.overall_quality_passed = false;
        result.quality_summary = std::string("Evaluation failed: ") + e.what();
    }
    
    return result;
}

std::vector<QCPipelineResult> QualityControlPipeline::evaluateBatch(
    const std::vector<std::tuple<
        std::string,
        std::vector<RetrievedDocument>,
        std::string
    >>& evaluations
) {
    std::vector<QCPipelineResult> results;
    results.reserve(evaluations.size());
    
    // For now, process sequentially
    // TODO: Implement parallel batch processing
    for (const auto& [query, docs, answer] : evaluations) {
        results.push_back(evaluate(query, docs, answer));
    }
    
    THEMIS_INFO("Batch evaluation completed: {} items", results.size());
    return results;
}

void QualityControlPipeline::setLLMClient(std::shared_ptr<LLMJudgeClient> client) {
    impl_->llm_client = client;
    THEMIS_INFO("LLM client set");
}

void QualityControlPipeline::setNLIVerifier(std::shared_ptr<NLIFaithfulnessVerifier> verifier) {
    impl_->nli_verifier = verifier;
    THEMIS_INFO("NLI verifier set");
}

PerformanceMetrics QualityControlPipeline::getAggregateMetrics() const {
    if (impl_->all_metrics.empty()) {
        return PerformanceMetrics{};
    }
    
    PerformanceMetrics aggregate;
    
    // Average all metrics
    for (const auto& m : impl_->all_metrics) {
        aggregate.total_time_ms += m.total_time_ms;
        aggregate.faithfulness_time_ms += m.faithfulness_time_ms;
        aggregate.relevance_time_ms += m.relevance_time_ms;
        aggregate.completeness_time_ms += m.completeness_time_ms;
        aggregate.coherence_time_ms += m.coherence_time_ms;
        aggregate.llm_call_time_ms += m.llm_call_time_ms;
        aggregate.nli_call_time_ms += m.nli_call_time_ms;
        aggregate.llm_calls_count += m.llm_calls_count;
        aggregate.nli_calls_count += m.nli_calls_count;
        aggregate.cache_hits += m.cache_hits;
        aggregate.cache_misses += m.cache_misses;
    }
    
    size_t count = impl_->all_metrics.size();
    aggregate.total_time_ms /= count;
    aggregate.faithfulness_time_ms /= count;
    aggregate.relevance_time_ms /= count;
    aggregate.completeness_time_ms /= count;
    aggregate.coherence_time_ms /= count;
    aggregate.llm_call_time_ms /= count;
    aggregate.nli_call_time_ms /= count;
    
    // Calculate pass rate
    size_t passed = std::count_if(
        impl_->all_metrics.begin(),
        impl_->all_metrics.end(),
        [](const auto& m) { return m.met_time_target; }
    );
    aggregate.met_time_target = (static_cast<double>(passed) / count) >= 0.95;
    
    return aggregate;
}

void QualityControlPipeline::clearCache() {
    impl_->result_cache.clear();
    THEMIS_INFO("Pipeline cache cleared");
}

void QualityControlPipeline::setConfig(const Config& config) {
    impl_->config = config;
    // Recreate judge with new config
    impl_->judge = std::make_unique<RAGJudge>(config.judge_config);
    THEMIS_INFO("Pipeline configuration updated");
}

QualityControlPipeline::Config QualityControlPipeline::getConfig() const {
    return impl_->config;
}

} // namespace themis::rag::judge
