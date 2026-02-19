/**
 * @file quality_control_pipeline.cpp
 * @brief Implementation of Quality Control Pipeline
 */

#include "rag/quality_control_pipeline.h"
#include "rag/continuous_learning_client.h"
#include "utils/logger.h"
#include <algorithm>
#include <numeric>
#include <sstream>

namespace themis::rag::judge {

// ═══════════════════════════════════════════════════════════
// Implementation
// ═══════════════════════════════════════════════════════════

struct QualityControlPipeline::Impl {
    Config config;
    std::shared_ptr<LLMJudgeClient> llm_judge_client;
    std::shared_ptr<GEvalEvaluator> geval_evaluator;
    std::shared_ptr<NLIFaithfulnessVerifier> nli_verifier;
    std::unique_ptr<ContinuousLearningClient> cl_client;
    std::function<void(const QCResult&)> qc_callback;
    
    // Statistics
    Statistics stats;
    std::mutex stats_mutex;
    
    Impl(const Config& cfg) : config(cfg) {}
};

// ═══════════════════════════════════════════════════════════
// Constructor & Destructor
// ═══════════════════════════════════════════════════════════

QualityControlPipeline::QualityControlPipeline()
    : QualityControlPipeline(Config{}) {
}

QualityControlPipeline::QualityControlPipeline(const Config& config)
    : impl_(std::make_unique<Impl>(config)) {
    
    // Create default components if not provided
    impl_->geval_evaluator = std::make_shared<GEvalEvaluator>();
    impl_->nli_verifier = std::make_shared<NLIFaithfulnessVerifier>();
    
    THEMIS_INFO("QualityControlPipeline initialized with mode: {}", 
                static_cast<int>(config.default_mode));
}

QualityControlPipeline::QualityControlPipeline(
    const Config& config,
    std::shared_ptr<LLMJudgeClient> llm_judge_client,
    std::shared_ptr<GEvalEvaluator> geval_evaluator,
    std::shared_ptr<NLIFaithfulnessVerifier> nli_verifier
) : impl_(std::make_unique<Impl>(config)) {
    
    impl_->llm_judge_client = llm_judge_client;
    impl_->geval_evaluator = geval_evaluator ? geval_evaluator : std::make_shared<GEvalEvaluator>();
    impl_->nli_verifier = nli_verifier ? nli_verifier : std::make_shared<NLIFaithfulnessVerifier>();
    
    THEMIS_INFO("QualityControlPipeline initialized with custom components");
}

QualityControlPipeline::~QualityControlPipeline() = default;

// ═══════════════════════════════════════════════════════════
// Public Methods
// ═══════════════════════════════════════════════════════════

QCResult QualityControlPipeline::runQualityControl(
    const std::string& query,
    const std::vector<RetrievedDocument>& documents,
    const std::string& generated_answer,
    QCMode mode
) {
    auto start_time = std::chrono::steady_clock::now();
    
    THEMIS_DEBUG("Running quality control in mode: {}", static_cast<int>(mode));
    
    // Create evaluation input
    EvaluationInput input;
    input.query = query;
    input.documents = documents;
    input.generated_answer = generated_answer;
    
    // Run appropriate mode
    QCResult result;
    result.mode = mode;
    result.retry_count = 0;
    
    switch (mode) {
        case QCMode::FAST:
            result = runFastMode(input);
            break;
        case QCMode::BALANCED:
            result = runBalancedMode(input);
            break;
        case QCMode::THOROUGH:
            result = runThoroughMode(input);
            break;
    }
    
    // Calculate overall latency
    result.latency = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - start_time
    );
    
    // Make decision
    result.decision = makeDecision(result.overall_score, impl_->config);
    result.passed_threshold = (result.overall_score >= impl_->config.accept_threshold);
    
    // Handle retry logic
    if (impl_->config.enable_retry && shouldRetry(result, 0)) {
        THEMIS_INFO("Quality score {:.3f} warrants retry", result.overall_score);
        
        for (int retry = 0; retry < impl_->config.max_retries; retry++) {
            // Re-run with same mode
            QCResult retry_result = runQualityControl(query, documents, generated_answer, mode);
            retry_result.retry_count = retry + 1;
            
            // Check if retry improved score
            if (retry_result.overall_score > result.overall_score + impl_->config.retry_improvement_threshold) {
                THEMIS_INFO("Retry {} improved score from {:.3f} to {:.3f}", 
                           retry + 1, result.overall_score, retry_result.overall_score);
                result = retry_result;
                
                // Stop if acceptable
                if (result.overall_score >= impl_->config.accept_threshold) {
                    break;
                }
            }
        }
    }
    
    // Log to continuous learning if enabled
    if (impl_->config.log_to_continuous_learning) {
        logToContinuousLearning(result);
    }
    
    // Call callback if set
    if (impl_->qc_callback) {
        impl_->qc_callback(result);
    }
    
    // Record statistics
    recordEvaluation(result);
    
    return result;
}

QCResult QualityControlPipeline::runAdaptiveQC(
    const std::string& query,
    const std::vector<RetrievedDocument>& documents,
    const std::string& generated_answer,
    int time_budget_ms
) {
    // Select mode based on time budget
    QCMode selected_mode;
    
    if (time_budget_ms < 100) {
        selected_mode = QCMode::FAST;
    } else if (time_budget_ms < 1000) {
        selected_mode = QCMode::BALANCED;
    } else {
        selected_mode = QCMode::THOROUGH;
    }
    
    THEMIS_DEBUG("Adaptive QC selected mode: {} for budget: {}ms", 
                static_cast<int>(selected_mode), time_budget_ms);
    
    return runQualityControl(query, documents, generated_answer, selected_mode);
}

std::vector<QCResult> QualityControlPipeline::batchQualityControl(
    const std::vector<EvaluationInput>& inputs,
    QCMode mode
) {
    std::vector<QCResult> results;
    results.reserve(inputs.size());
    
    for (const auto& input : inputs) {
        results.push_back(runQualityControl(
            input.query,
            input.documents,
            input.generated_answer,
            mode
        ));
    }
    
    return results;
}

void QualityControlPipeline::setQCCallback(std::function<void(const QCResult&)> callback) {
    impl_->qc_callback = callback;
}

QualityControlPipeline::Config QualityControlPipeline::getConfig() const {
    return impl_->config;
}

void QualityControlPipeline::setConfig(const Config& config) {
    impl_->config = config;
}

QualityControlPipeline::Statistics QualityControlPipeline::getStatistics() const {
    std::lock_guard<std::mutex> lock(impl_->stats_mutex);
    return impl_->stats;
}

// ═══════════════════════════════════════════════════════════
// Private Methods - Mode Implementations
// ═══════════════════════════════════════════════════════════

QCResult QualityControlPipeline::runFastMode(const EvaluationInput& input) {
    auto start = std::chrono::steady_clock::now();
    
    QCResult result;
    result.mode = QCMode::FAST;
    
    // Constants for text processing
    constexpr size_t MIN_SENTENCE_LENGTH = 20;
    constexpr size_t MAX_CLAIMS_FAST_MODE = 3;
    
    // Fast mode: Quick faithfulness check only
    if (impl_->config.enable_nli_verification && impl_->nli_verifier) {
        // Extract key claims (simplified - just split by sentences)
        std::vector<std::string> claims;
        std::istringstream answer_stream(input.generated_answer);
        std::string sentence;
        while (std::getline(answer_stream, sentence, '.')) {
            if (!sentence.empty() && sentence.length() > MIN_SENTENCE_LENGTH) {
                claims.push_back(sentence);
                if (claims.size() >= MAX_CLAIMS_FAST_MODE) break;  // Limit for fast mode
            }
        }
        
        // Verify claims against documents
        double faithfulness_sum = 0.0;
        for (const auto& claim : claims) {
            // Verify against all documents, take best match
            double best_entailment = 0.0;
            for (const auto& doc : input.documents) {
                auto nli_result = impl_->nli_verifier->verifyClaim(claim, doc.content);
                if (nli_result.success) {
                    result.nli_results.push_back(nli_result);
                    best_entailment = std::max(best_entailment, nli_result.entailment_prob);
                }
            }
            faithfulness_sum += best_entailment;
        }
        
        result.faithfulness_score = claims.empty() ? 0.5 : faithfulness_sum / claims.size();
    } else {
        // Fallback: simple heuristic
        result.faithfulness_score = 0.7;
    }
    
    // Fast mode: use faithfulness as overall score
    result.overall_score = result.faithfulness_score;
    result.relevance_score = 0.0;  // Not evaluated in fast mode
    result.completeness_score = 0.0;
    result.coherence_score = 0.0;
    
    // Check timeout
    if (checkTimeout(start, QCMode::FAST)) {
        result.warnings.push_back("Fast mode exceeded timeout");
    }
    
    result.explanation = "Fast mode: faithfulness-only evaluation";
    
    return result;
}

QCResult QualityControlPipeline::runBalancedMode(const EvaluationInput& input) {
    auto start = std::chrono::steady_clock::now();
    
    QCResult result;
    result.mode = QCMode::BALANCED;
    
    // Balanced mode: Multi-dimension evaluation with selective verification
    
    // 1. Faithfulness with NLI (selective - top claims only)
    if (impl_->config.enable_nli_verification && impl_->nli_verifier) {
        std::vector<std::string> claims;
        std::istringstream answer_stream(input.generated_answer);
        std::string sentence;
        while (std::getline(answer_stream, sentence, '.')) {
            if (!sentence.empty() && sentence.length() > 20) {
                claims.push_back(sentence);
                if (claims.size() >= 5) break;  // Limit for balanced mode
            }
        }
        
        double faithfulness_sum = 0.0;
        for (const auto& claim : claims) {
            std::vector<std::string> doc_contents;
            for (const auto& doc : input.documents) {
                doc_contents.push_back(doc.content);
            }
            auto nli_result = impl_->nli_verifier->verifyAgainstMultipleDocs(claim, doc_contents);
            if (nli_result.success) {
                result.nli_results.push_back(nli_result);
                faithfulness_sum += nli_result.entailment_prob;
            }
        }
        
        result.faithfulness_score = claims.empty() ? 0.5 : faithfulness_sum / claims.size();
    } else {
        result.faithfulness_score = 0.75;
    }
    
    // 2. Relevance (heuristic - keyword overlap)
    constexpr size_t MIN_WORD_LENGTH = 3;
    
    std::string query_lower = input.query;
    std::string answer_lower = input.generated_answer;
    std::transform(query_lower.begin(), query_lower.end(), query_lower.begin(), ::tolower);
    std::transform(answer_lower.begin(), answer_lower.end(), answer_lower.begin(), ::tolower);
    
    std::istringstream query_stream(query_lower);
    std::vector<std::string> query_words;
    std::string word;
    while (query_stream >> word) {
        if (word.length() > MIN_WORD_LENGTH) query_words.push_back(word);
    }
    
    int matches = 0;
    for (const auto& qword : query_words) {
        if (answer_lower.find(qword) != std::string::npos) {
            matches++;
        }
    }
    
    result.relevance_score = query_words.empty() ? 0.5 : 
                             static_cast<double>(matches) / query_words.size();
    
    // 3. Completeness (heuristic - answer length)
    size_t answer_length = input.generated_answer.length();
    if (answer_length < 50) {
        result.completeness_score = 0.3;
    } else if (answer_length < 200) {
        result.completeness_score = 0.6;
    } else if (answer_length < 500) {
        result.completeness_score = 0.8;
    } else {
        result.completeness_score = 0.9;
    }
    
    // 4. Coherence (heuristic - sentence count and structure)
    size_t sentence_count = std::count(input.generated_answer.begin(), 
                                      input.generated_answer.end(), '.');
    result.coherence_score = sentence_count > 0 && sentence_count < 20 ? 0.8 : 0.6;
    
    // Compute overall score
    result.overall_score = (result.faithfulness_score * 0.4 +
                           result.relevance_score * 0.3 +
                           result.completeness_score * 0.15 +
                           result.coherence_score * 0.15);
    
    if (checkTimeout(start, QCMode::BALANCED)) {
        result.warnings.push_back("Balanced mode exceeded timeout");
    }
    
    result.explanation = "Balanced mode: multi-dimension evaluation with selective NLI";
    
    return result;
}

QCResult QualityControlPipeline::runThoroughMode(const EvaluationInput& input) {
    auto start = std::chrono::steady_clock::now();
    
    QCResult result;
    result.mode = QCMode::THOROUGH;
    
    // Thorough mode: Full evaluation with all components
    
    // 1. G-Eval scoring (if enabled)
    if (impl_->config.enable_geval_scoring && impl_->geval_evaluator) {
        std::vector<std::pair<std::string, std::string>> docs;
        for (const auto& doc : input.documents) {
            docs.push_back({doc.id, doc.content});
        }
        
        // Evaluate each dimension with G-Eval
        auto faith_geval = impl_->geval_evaluator->evaluate(
            input.query, input.generated_answer, docs, "faithfulness"
        );
        result.geval_results.push_back(faith_geval);
        result.faithfulness_score = faith_geval.geval_score;
        
        auto rel_geval = impl_->geval_evaluator->evaluate(
            input.query, input.generated_answer, docs, "relevance"
        );
        result.geval_results.push_back(rel_geval);
        result.relevance_score = rel_geval.geval_score;
        
        auto comp_geval = impl_->geval_evaluator->evaluate(
            input.query, input.generated_answer, docs, "completeness"
        );
        result.geval_results.push_back(comp_geval);
        result.completeness_score = comp_geval.geval_score;
        
        auto coh_geval = impl_->geval_evaluator->evaluate(
            input.query, input.generated_answer, docs, "coherence"
        );
        result.geval_results.push_back(coh_geval);
        result.coherence_score = coh_geval.geval_score;
    } else {
        // Fallback to balanced mode scoring
        auto balanced_result = runBalancedMode(input);
        result.faithfulness_score = balanced_result.faithfulness_score;
        result.relevance_score = balanced_result.relevance_score;
        result.completeness_score = balanced_result.completeness_score;
        result.coherence_score = balanced_result.coherence_score;
    }
    
    // 2. Comprehensive NLI verification
    if (impl_->config.enable_nli_verification && impl_->nli_verifier) {
        std::vector<std::string> claims;
        std::istringstream answer_stream(input.generated_answer);
        std::string sentence;
        while (std::getline(answer_stream, sentence, '.')) {
            if (!sentence.empty() && sentence.length() > 20) {
                claims.push_back(sentence);
            }
        }
        
        // Verify all claims
        for (const auto& claim : claims) {
            std::vector<std::string> doc_contents;
            for (const auto& doc : input.documents) {
                doc_contents.push_back(doc.content);
            }
            auto nli_result = impl_->nli_verifier->verifyAgainstMultipleDocs(claim, doc_contents);
            if (nli_result.success) {
                result.nli_results.push_back(nli_result);
            }
        }
        
        // Adjust faithfulness based on NLI results
        if (!result.nli_results.empty()) {
            double nli_faithfulness = nli_utils::aggregateFaithfulness(result.nli_results);
            // Blend with G-Eval score
            result.faithfulness_score = (result.faithfulness_score * 0.5 + nli_faithfulness * 0.5);
        }
    }
    
    // Compute overall score
    result.overall_score = (result.faithfulness_score * 0.4 +
                           result.relevance_score * 0.3 +
                           result.completeness_score * 0.15 +
                           result.coherence_score * 0.15);
    
    if (checkTimeout(start, QCMode::THOROUGH)) {
        result.warnings.push_back("Thorough mode exceeded timeout");
    }
    
    result.explanation = "Thorough mode: full evaluation with G-Eval and comprehensive NLI";
    
    // Add detailed recommendations
    if (result.faithfulness_score < 0.7) {
        result.recommendations.push_back("Improve faithfulness: verify claims against source documents");
    }
    if (result.relevance_score < 0.7) {
        result.recommendations.push_back("Improve relevance: ensure answer directly addresses the query");
    }
    if (result.completeness_score < 0.7) {
        result.recommendations.push_back("Improve completeness: address all aspects of the query");
    }
    
    return result;
}

// ═══════════════════════════════════════════════════════════
// Private Methods - Decision Logic
// ═══════════════════════════════════════════════════════════

QCDecision QualityControlPipeline::makeDecision(double overall_score, const Config& config) {
    if (overall_score >= config.accept_threshold) {
        return QCDecision::ACCEPT;
    } else if (overall_score < config.reject_threshold) {
        return QCDecision::REJECT;
    } else if (overall_score < config.warn_threshold) {
        return QCDecision::WARN;
    } else {
        return QCDecision::RETRY;
    }
}

bool QualityControlPipeline::shouldRetry(const QCResult& result, int attempt_num) {
    if (attempt_num >= impl_->config.max_retries) {
        return false;
    }
    
    // Retry if decision is RETRY or WARN
    return (result.decision == QCDecision::RETRY || result.decision == QCDecision::WARN);
}

void QualityControlPipeline::logToContinuousLearning(const QCResult& result) {
    // Create continuous learning client if not already created
    if (!impl_->cl_client && impl_->config.log_to_continuous_learning) {
        ContinuousLearningClient::Config cl_config;
        cl_config.endpoint = impl_->config.cl_endpoint.empty() ? 
                            "http://localhost:8080/metrics" : 
                            impl_->config.cl_endpoint;
        cl_config.enable_logging = true;
        cl_config.enable_triggers = true;
        
        impl_->cl_client = std::make_unique<ContinuousLearningClient>(cl_config);
        
        // Set trigger callback
        impl_->cl_client->setTriggerCallback([](const OptimizationTrigger& trigger) {
            THEMIS_INFO("Optimization trigger: {} - {}", 
                       trigger.trigger_type, trigger.recommendation);
        });
    }
    
    // Log result to continuous learning
    if (impl_->cl_client) {
        impl_->cl_client->logQCResult(result);
    }
    
    THEMIS_DEBUG("Logged QC result to continuous learning: score={:.3f}, decision={}", 
                result.overall_score, static_cast<int>(result.decision));
}

void QualityControlPipeline::recordEvaluation(const QCResult& result) {
    std::lock_guard<std::mutex> lock(impl_->stats_mutex);
    
    impl_->stats.total_evaluations++;
    
    switch (result.decision) {
        case QCDecision::ACCEPT:
            impl_->stats.accepted++;
            break;
        case QCDecision::REJECT:
            impl_->stats.rejected++;
            break;
        case QCDecision::RETRY:
            impl_->stats.retried++;
            break;
        case QCDecision::WARN:
            impl_->stats.warned++;
            break;
    }
    
    impl_->stats.mode_usage[result.mode]++;
    
    // Update averages
    double n = static_cast<double>(impl_->stats.total_evaluations);
    impl_->stats.avg_latency_ms = 
        (impl_->stats.avg_latency_ms * (n - 1) + result.latency.count()) / n;
    impl_->stats.avg_score = 
        (impl_->stats.avg_score * (n - 1) + result.overall_score) / n;
}

bool QualityControlPipeline::checkTimeout(
    std::chrono::steady_clock::time_point start,
    QCMode mode
) {
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - start
    );
    
    int timeout;
    switch (mode) {
        case QCMode::FAST:
            timeout = impl_->config.fast_timeout_ms;
            break;
        case QCMode::BALANCED:
            timeout = impl_->config.balanced_timeout_ms;
            break;
        case QCMode::THOROUGH:
            timeout = impl_->config.thorough_timeout_ms;
            break;
        default:
            timeout = impl_->config.balanced_timeout_ms;
    }
    
    return elapsed.count() > timeout;
}

// ═══════════════════════════════════════════════════════════
// Factory Methods
// ═══════════════════════════════════════════════════════════

std::unique_ptr<QualityControlPipeline> QualityControlPipelineFactory::createFast() {
    QualityControlPipeline::Config config;
    config.default_mode = QCMode::FAST;
    config.enable_geval_scoring = false;
    config.enable_claim_extraction = false;
    config.enable_citation_check = false;
    config.fast_timeout_ms = 50;
    
    return std::make_unique<QualityControlPipeline>(config);
}

std::unique_ptr<QualityControlPipeline> QualityControlPipelineFactory::createBalanced() {
    QualityControlPipeline::Config config;
    config.default_mode = QCMode::BALANCED;
    config.enable_geval_scoring = false;  // Use heuristics for speed
    config.enable_nli_verification = true;
    config.balanced_timeout_ms = 500;
    
    return std::make_unique<QualityControlPipeline>(config);
}

std::unique_ptr<QualityControlPipeline> QualityControlPipelineFactory::createThorough() {
    QualityControlPipeline::Config config;
    config.default_mode = QCMode::THOROUGH;
    config.enable_geval_scoring = true;
    config.enable_nli_verification = true;
    config.enable_claim_extraction = true;
    config.enable_citation_check = true;
    config.thorough_timeout_ms = 2000;
    
    return std::make_unique<QualityControlPipeline>(config);
}

std::unique_ptr<QualityControlPipeline> QualityControlPipelineFactory::create(
    const QualityControlPipeline::Config& config
) {
    return std::make_unique<QualityControlPipeline>(config);
}

} // namespace themis::rag::judge
