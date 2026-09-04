/**
 * @file quality_control_pipeline.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 100/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=2, H=0, M=4, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#include "rag/quality_control_pipeline.h"
#include "rag/citation_highlighter.h"
#include "utils/logger.h"
#include <algorithm>
#include <mutex>
#include <numeric>
#include <sstream>
#include <iomanip>
#include <nlohmann/json.hpp>

namespace themis::rag::judge {

using json = nlohmann::json;
using themis::rag::CitationHighlighter;
using themis::rag::SourceChunk;

// ═══════════════════════════════════════════════════════════
// QualityControlPipeline Implementation
// ═══════════════════════════════════════════════════════════

struct QualityControlPipeline::Impl {
    Config config;
    
    // Components
    std::shared_ptr<LLMJudgeClient> llm_judge;
    std::shared_ptr<GEvalEvaluator> geval;
    std::shared_ptr<NLIFaithfulnessVerifier> nli_verifier;
    
    // Citation highlighter (used in thorough stage)
    CitationHighlighter citation_highlighter;

    // Callbacks
    std::function<void(const QualityCheckResult&)> failure_callback;
    std::function<void(const std::string&, const QualityCheckResult&)> learning_callback;
    
    // Statistics
    mutable std::mutex stats_mutex;
    struct Stats {
        size_t total_checks = 0;
        size_t passed_fast = 0;
        size_t passed_balanced = 0;
        size_t passed_thorough = 0;
        size_t failed = 0;
        size_t retries = 0;
        double avg_fast_time_ms = 0.0;
        double avg_balanced_time_ms = 0.0;
        double avg_thorough_time_ms = 0.0;
    } stats;
    
    explicit Impl(Config cfg) : config(std::move(cfg)) {
        // Initialize components with default configs
        llm_judge = std::make_shared<LLMJudgeClient>();
        
        GEvalEvaluator::Config geval_config;
        geval_config.num_samples = 3;
        geval = std::make_shared<GEvalEvaluator>(geval_config);
        
        NLIFaithfulnessVerifier::Config nli_config;
        nli_config.entailment_threshold = 0.7;
        nli_verifier = std::make_shared<NLIFaithfulnessVerifier>(nli_config);
        
        THEMIS_INFO("QualityControlPipeline initialized");
    }
};

QualityControlPipeline::QualityControlPipeline()
    : QualityControlPipeline(Config{}) {
}

QualityControlPipeline::QualityControlPipeline(const Config& config)
    : impl_(std::make_unique<Impl>(Config{config})) {
}

QualityControlPipeline::~QualityControlPipeline() = default;

QualityCheckResult QualityControlPipeline::runQualityControl(
    const std::string& query,
    const std::string& answer,
    const std::vector<RetrievedDocument>& documents
) {
    auto overall_start = std::chrono::steady_clock::now();
    
    QualityCheckResult result;
    impl_->stats.total_checks++;
    
    THEMIS_INFO("Starting quality control pipeline for query: {}", 
               query.substr(0, 50));
    
    // Stage 1: Fast Screening
    if (impl_->config.enable_fast_stage) {
        auto stage_result = runFastStage(query, answer, documents);
        result.fast_stage_time = stage_result.fast_stage_time;
        
        if (stage_result.status == QualityGateStatus::FAILED) {
            result.status = QualityGateStatus::FAILED;
            result.failure_reasons.emplace_back("Failed fast screening stage");
            impl_->stats.failed++;
            
            if (impl_->failure_callback) {
                impl_->failure_callback(result);
            }
            
            return result;
        }
        
        // Merge dimension scores
        result.dimension_scores.insert(result.dimension_scores.end(),
                                      stage_result.dimension_scores.begin(),
                                      stage_result.dimension_scores.end());
        
        // Update statistics
        {
            std::lock_guard<std::mutex> lock(impl_->stats_mutex);
            impl_->stats.total_checks++;
            const auto passed_fast_count = static_cast<double>(impl_->stats.passed_fast);
            const auto fast_stage_ms = static_cast<double>(result.fast_stage_time.count());
            impl_->stats.avg_fast_time_ms =
                ((impl_->stats.avg_fast_time_ms * passed_fast_count) + fast_stage_ms)
                / (passed_fast_count + 1.0);
            impl_->stats.passed_fast++;
        }
    }
    
    // Stage 2: Balanced Evaluation
    if (impl_->config.enable_balanced_stage) {
        auto stage_result = runBalancedStage(query, answer, documents);
        result.balanced_stage_time = stage_result.balanced_stage_time;
        
        if (stage_result.status == QualityGateStatus::FAILED) {
            result.status = QualityGateStatus::RETRY_NEEDED;
            result.should_retry = true;
            result.failure_reasons.emplace_back("Failed balanced evaluation stage");
            impl_->stats.failed++;
            
            return result;
        }
        
        // Merge dimension scores
        result.dimension_scores.insert(result.dimension_scores.end(),
                                      stage_result.dimension_scores.begin(),
                                      stage_result.dimension_scores.end());
        impl_->stats.passed_balanced++;
    }
    
    // Stage 3: Thorough Verification
    if (impl_->config.enable_thorough_stage) {
        auto stage_result = runThoroughStage(query, answer, documents);
        result.thorough_stage_time = stage_result.thorough_stage_time;
        
        if (stage_result.status == QualityGateStatus::FAILED) {
            result.status = QualityGateStatus::ESCALATE;
            result.failure_reasons.emplace_back("Failed thorough verification");
            impl_->stats.failed++;
            
            return result;
        }
        
        // Merge dimension scores
        result.dimension_scores.insert(result.dimension_scores.end(),
                                      stage_result.dimension_scores.begin(),
                                      stage_result.dimension_scores.end());
        result.citation_coverage = stage_result.citation_coverage;
        impl_->stats.passed_thorough++;
    }
    
    // Compute overall score
    result.overall_score = computeOverallScore(result.dimension_scores);
    
    // Compute aggregate confidence
    if (!result.dimension_scores.empty()) {
        double total_conf = 0.0;
        for (const auto& dim : result.dimension_scores) {
            total_conf += dim.confidence;
        }
        result.confidence = total_conf / static_cast<double>(result.dimension_scores.size());
    } else {
        result.confidence = 0.5;
    }
    
    // Determine final status
    result.status = QualityGateStatus::PASSED;
    
    auto overall_end = std::chrono::steady_clock::now();
    result.total_time = std::chrono::duration_cast<std::chrono::milliseconds>(
        overall_end - overall_start);
    
    // Stage 4: Learning Feedback
    if (impl_->config.enable_learning_feedback) {
        sendLearningFeedback(query, result);
    }
    
    THEMIS_INFO("Quality control complete: score={:.3f}, status={}, time={}ms",
               result.overall_score, static_cast<int>(result.status),
               result.total_time.count());
    
    return result;
}

QualityCheckResult QualityControlPipeline::runStage(
    QualityStage stage,
    const std::string& query,
    const std::string& answer,
    const std::vector<RetrievedDocument>& documents
) {
    switch (stage) {
        case QualityStage::FAST_SCREENING:
            return runFastStage(query, answer, documents);
        case QualityStage::BALANCED_EVAL:
            return runBalancedStage(query, answer, documents);
        case QualityStage::THOROUGH_VERIFY:
            return runThoroughStage(query, answer, documents);
        default:
            QualityCheckResult empty;
            empty.status = QualityGateStatus::FAILED;
            return empty;
    }
}

QualityCheckResult QualityControlPipeline::runFastStage(
    const std::string& query,
    const std::string& answer,
    const std::vector<RetrievedDocument>& documents
) {
    auto start = std::chrono::steady_clock::now();
    static_cast<void>(query);
    
    QualityCheckResult result;
    
    THEMIS_DEBUG("Running fast screening stage");
    
    // Convert documents to simple pairs
    std::vector<std::pair<std::string, std::string>> doc_pairs;
    doc_pairs.reserve(documents.size());
    for (const auto& doc : documents) {
        doc_pairs.emplace_back(doc.id, doc.content);
    }
    
    // Quick faithfulness check (most critical dimension)
    try {
        auto eval_response = impl_->llm_judge->evaluateDimension(
            query, answer, doc_pairs, "faithfulness");
        
        DimensionScore faith_score;
        faith_score.dimension = "faithfulness";
        faith_score.score = eval_response.score;
        faith_score.confidence = eval_response.confidence;
        faith_score.method = "llm";
        faith_score.explanation = eval_response.reasoning;
        
        result.dimension_scores.push_back(faith_score);
        
        if (faith_score.score < impl_->config.fast_stage_threshold) {
            result.status = QualityGateStatus::FAILED;
            result.failure_reasons.push_back(
                "Faithfulness score too low: " + std::to_string(faith_score.score));
        } else {
            result.status = QualityGateStatus::PASSED;
        }
        
    } catch (const std::exception& e) {
        THEMIS_ERROR("Fast stage failed: {}", e.what());
        result.status = QualityGateStatus::FAILED;
        result.failure_reasons.push_back(std::string("Fast stage error: ") + e.what());
    }
    
    auto end = std::chrono::steady_clock::now();
    result.fast_stage_time = std::chrono::duration_cast<std::chrono::milliseconds>(
        end - start);
    
    // Update stats AFTER incrementing passed_fast counter in the caller
    
    return result;
}

QualityCheckResult QualityControlPipeline::runBalancedStage(
    const std::string& query,
    const std::string& answer,
    const std::vector<RetrievedDocument>& documents
) {
    auto start = std::chrono::steady_clock::now();
    
    QualityCheckResult result;
    
    THEMIS_DEBUG("Running balanced evaluation stage");
    
    std::vector<std::pair<std::string, std::string>> doc_pairs;
    doc_pairs.reserve(documents.size());
    for (const auto& doc : documents) {
        doc_pairs.emplace_back(doc.id, doc.content);
    }
    
    // Evaluate multiple dimensions using G-Eval
    std::vector<std::string> dimensions = {"relevance", "completeness", "coherence"};
    
    for (const auto& dim : dimensions) {
        try {
            auto geval_result = impl_->geval->evaluate(query, answer, doc_pairs, dim);
            
            DimensionScore score;
            score.dimension = dim;
            score.score = geval_result.geval_score;
            score.confidence = geval_result.confidence;
            score.method = "geval";
            score.explanation = geval_result.reasoning;
            
            result.dimension_scores.push_back(score);
            
        } catch (const std::exception& e) {
            THEMIS_WARN("G-Eval failed for {}: {}", dim, e.what());
        }
    }
    
    // Compute stage score
    double stage_score = computeOverallScore(result.dimension_scores);
    
    if (stage_score < impl_->config.balanced_stage_threshold) {
        result.status = QualityGateStatus::FAILED;
        result.failure_reasons.push_back(
            "Balanced stage score too low: " + std::to_string(stage_score));
    } else {
        result.status = QualityGateStatus::PASSED;
    }
    
    auto end = std::chrono::steady_clock::now();
    result.balanced_stage_time = std::chrono::duration_cast<std::chrono::milliseconds>(
        end - start);
    
    return result;
}

QualityCheckResult QualityControlPipeline::runThoroughStage(
    const std::string& query,
    const std::string& answer,
    const std::vector<RetrievedDocument>& documents
) {
    auto start = std::chrono::steady_clock::now();
    static_cast<void>(query);
    
    QualityCheckResult result;
    
    THEMIS_DEBUG("Running thorough verification stage");
    
    std::vector<std::pair<std::string, std::string>> doc_pairs;
    doc_pairs.reserve(documents.size());
    for (const auto& doc : documents) {
        doc_pairs.emplace_back(doc.id, doc.content);
    }
    
    // NLI-based faithfulness verification
    try {
        auto nli_result = impl_->nli_verifier->verify(answer, doc_pairs);
        
        DimensionScore faith_score;
        faith_score.dimension = "faithfulness_nli";
        faith_score.score = nli_result.faithfulness_score;
        faith_score.confidence = 0.9;  // NLI is generally high confidence
        faith_score.method = "nli";
        faith_score.explanation = nli_result.explanation;
        
        result.dimension_scores.push_back(faith_score);
        
        if (!nli_result.is_faithful) {
            result.status = QualityGateStatus::FAILED;
            result.failure_reasons.push_back("NLI verification failed");
            
            // Add specific failure reasons
            if (nli_result.contradicted_claims > 0) {
                result.failure_reasons.push_back(
                    std::to_string(nli_result.contradicted_claims) + 
                    " contradicted claims found");
            }
            if (nli_result.unsupported_claims > nli_result.supported_claims) {
                result.failure_reasons.push_back(
                    "More unsupported than supported claims");
            }
        } else {
            result.status = QualityGateStatus::PASSED;
        }
        
    } catch (const std::exception& e) {
        THEMIS_ERROR("Thorough stage failed: {}", e.what());
        result.status = QualityGateStatus::FAILED;
        result.failure_reasons.push_back(std::string("NLI error: ") + e.what());
    }
    
    // Citation coverage check (map answer sentences to source chunks)
    if (impl_->config.enable_citation_check && !documents.empty()) {
        std::vector<SourceChunk> source_chunks = {};

        source_chunks.reserve(documents.size());
        size_t chunk_idx = 0;
        for (const auto& doc : documents) {
            SourceChunk sc;
            sc.doc_id      = doc.id;
            sc.chunk_index = chunk_idx++;
            sc.content     = doc.content;
            source_chunks.push_back(std::move(sc));
        }

        auto citation_result = impl_->citation_highlighter.highlight(answer, source_chunks);

        DimensionScore cit_score;
        cit_score.dimension   = "citation_coverage";
        cit_score.score       = citation_result.citation_coverage;
        cit_score.confidence  = citation_result.mean_similarity;
        cit_score.method      = "citation_highlighter";
        cit_score.explanation = "Fraction of answer sentences mapped to source chunks";

        result.dimension_scores.push_back(cit_score);
        result.citation_coverage = citation_result.citation_coverage;

        THEMIS_DEBUG("Citation coverage check: coverage={:.2f}, mean_sim={:.3f}",
                     citation_result.citation_coverage, citation_result.mean_similarity);
    }

    auto end = std::chrono::steady_clock::now();
    result.thorough_stage_time = std::chrono::duration_cast<std::chrono::milliseconds>(
        end - start);
    
    return result;
}

void QualityControlPipeline::sendLearningFeedback(
    const std::string& query,
    const QualityCheckResult& result
) {
    if (!impl_->config.enable_learning_feedback) {
        return;
    }
    
    THEMIS_DEBUG("Sending feedback to learning system");
    
    if (impl_->learning_callback) {
        impl_->learning_callback(query, result);
    }
    
    // In production, would send to continuous learning orchestrator
    // via HTTP/gRPC to the learning_orchestrator_url
}

double QualityControlPipeline::computeOverallScore(
    const std::vector<DimensionScore>& scores
) {
    if (scores.empty()) {
        return 0.0;
    }
    
    // Weight by dimension name
    std::unordered_map<std::string, double> weights = {
        {"faithfulness", impl_->config.faithfulness_weight},
        {"faithfulness_nli", impl_->config.faithfulness_weight},
        {"relevance", impl_->config.relevance_weight},
        {"completeness", impl_->config.completeness_weight},
        {"coherence", impl_->config.coherence_weight},
        {"ethical", impl_->config.ethical_weight},
        {"citation_coverage", 0.0}  // informational; does not affect weighted overall score
    };
    
    double weighted_sum = 0.0;
    double total_weight = 0.0;
    
    for (const auto& score : scores) {
        double weight = 1.0;  // Default weight
        
        auto it = weights.find(score.dimension);
        if (it != weights.end()) {
            weight = it->second;
        }
        
        weighted_sum += score.score * weight;
        total_weight += weight;
    }
    
    return total_weight > 0.0 ? weighted_sum / total_weight : 0.0;
}

QualityGateStatus QualityControlPipeline::determineStatus(
    double score,
    QualityStage stage
) {
    double threshold = 0.7;
    
    switch (stage) {
        case QualityStage::FAST_SCREENING:
            threshold = impl_->config.fast_stage_threshold;
            break;
        case QualityStage::BALANCED_EVAL:
            threshold = impl_->config.balanced_stage_threshold;
            break;
        case QualityStage::THOROUGH_VERIFY:
            threshold = impl_->config.thorough_stage_threshold;
            break;
        default:
            break;
    }
    
    if (score >= threshold) {
        return QualityGateStatus::PASSED;
    }
    if (score >= impl_->config.retry_threshold) {
        return QualityGateStatus::RETRY_NEEDED;
    }
    return QualityGateStatus::FAILED;
}

void QualityControlPipeline::setFailureCallback(
    std::function<void(const QualityCheckResult&)> callback
) {
    impl_->failure_callback = callback;
}

void QualityControlPipeline::setLearningCallback(
    std::function<void(const std::string&, const QualityCheckResult&)> callback
) {
    impl_->learning_callback = callback;
}

void QualityControlPipeline::setLLMJudgeClient(
    std::shared_ptr<LLMJudgeClient> client
) {
    impl_->llm_judge = client;
}

void QualityControlPipeline::setGEvalEvaluator(
    std::shared_ptr<GEvalEvaluator> evaluator
) {
    impl_->geval = evaluator;
}

void QualityControlPipeline::setNLIVerifier(
    std::shared_ptr<NLIFaithfulnessVerifier> verifier
) {
    impl_->nli_verifier = verifier;
}

QualityControlPipeline::Config QualityControlPipeline::getConfig() const {
    return impl_->config;
}

void QualityControlPipeline::setConfig(const Config& config) {
    impl_->config = config;
}

std::string QualityControlPipeline::getStatistics() const {
    const json stats = {
        {"total_checks", impl_->stats.total_checks},
        {"passed_fast", impl_->stats.passed_fast},
        {"passed_balanced", impl_->stats.passed_balanced},
        {"passed_thorough", impl_->stats.passed_thorough},
        {"failed", impl_->stats.failed},
        {"retries", impl_->stats.retries},
        {"avg_fast_time_ms", impl_->stats.avg_fast_time_ms},
        {"avg_balanced_time_ms", impl_->stats.avg_balanced_time_ms},
        {"avg_thorough_time_ms", impl_->stats.avg_thorough_time_ms}
    };
    
    return stats.dump(2);
}

void QualityControlPipeline::resetStatistics() {
    impl_->stats = Impl::Stats{};
}

// ═══════════════════════════════════════════════════════════
// QualityPipelineFactory Implementation
// ═══════════════════════════════════════════════════════════

std::unique_ptr<QualityControlPipeline> QualityPipelineFactory::createFast() {
    QualityControlPipeline::Config config;
    config.enable_fast_stage = true;
    config.enable_balanced_stage = false;
    config.enable_thorough_stage = false;
    config.enable_learning_feedback = false;
    
    return std::make_unique<QualityControlPipeline>(config);
}

std::unique_ptr<QualityControlPipeline> QualityPipelineFactory::createBalanced() {
    QualityControlPipeline::Config config;
    config.enable_fast_stage = true;
    config.enable_balanced_stage = true;
    config.enable_thorough_stage = false;
    config.enable_learning_feedback = false;
    
    return std::make_unique<QualityControlPipeline>(config);
}

std::unique_ptr<QualityControlPipeline> QualityPipelineFactory::createThorough() {
    QualityControlPipeline::Config config;
    config.enable_fast_stage = true;
    config.enable_balanced_stage = true;
    config.enable_thorough_stage = true;
    config.enable_learning_feedback = false;
    
    return std::make_unique<QualityControlPipeline>(config);
}

std::unique_ptr<QualityControlPipeline> QualityPipelineFactory::createProduction() {
    QualityControlPipeline::Config config;
    config.enable_fast_stage = true;
    config.enable_balanced_stage = true;
    config.enable_thorough_stage = true;
    config.enable_learning_feedback = true;
    config.enable_async_feedback = true;
    
    return std::make_unique<QualityControlPipeline>(config);
}

} // namespace themis::rag::judge

