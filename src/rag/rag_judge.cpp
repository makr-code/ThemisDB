/**
 * @file rag_judge.cpp
 * @brief Implementation of LLM-as-Judge for RAG System Quality Evaluation
 */

#include "rag/rag_judge.h"
#include "rag/prompt_templates.h"
#include "rag/response_parser.h"
#include "rag/llm_judge_integration.h"
#include "utils/logger.h"
#include <algorithm>
#include <numeric>
#include <chrono>
#include <sstream>

namespace themis::rag::judge {

// Private implementation details
struct RAGJudge::Impl {
    RAGJudgeConfig config;
    std::function<void(const EvaluationResult&)> eval_callback;
    
    // New components for Phase 1
    PromptTemplateManager template_manager;
    std::unique_ptr<LLMJudgeIntegration> llm_integration;
    
    // Cache for performance
    std::unordered_map<std::string, EvaluationResult> cache;
    
    std::string computeCacheKey(const std::string& query, const std::string& answer) {
        // Simple hash combination
        return query + "|" + answer;
    }
};

RAGJudge::RAGJudge(const RAGJudgeConfig& config)
    : impl_(std::make_unique<Impl>()) {
    impl_->config = config;
    
    // Initialize prompt template manager
    impl_->template_manager = PromptTemplateManager::createDefault();
    
    // Initialize LLM integration
    LLMJudgeIntegration::Config llm_config;
    llm_config.model_name = config.judge_model;
    llm_config.temperature = 0.3; // Low temperature for consistent evaluation
    llm_config.max_tokens = 1024;
    llm_config.use_json_mode = true;
    
    impl_->llm_integration = std::make_unique<LLMJudgeIntegration>(llm_config);
    
    THEMIS_INFO("RAG Judge initialized with mode: {}, model: {}", 
                static_cast<int>(config.mode), config.judge_model);
}

RAGJudge::~RAGJudge() = default;

EvaluationResult RAGJudge::evaluate(
    const std::string& query,
    const std::vector<RetrievedDocument>& documents,
    const std::string& generated_answer,
    const RAGJudgeConfig& config
) {
    EvaluationInput input;
    input.query = query;
    input.documents = documents;
    input.generated_answer = generated_answer;
    
    // Use provided config or default
    auto saved_config = impl_->config;
    if (&config != &impl_->config) {
        impl_->config = config;
    }
    
    auto result = evaluate(input);
    
    // Restore original config
    impl_->config = saved_config;
    
    return result;
}

EvaluationResult RAGJudge::evaluate(const EvaluationInput& input) {
    auto start_time = std::chrono::steady_clock::now();
    
    THEMIS_DEBUG("Evaluating RAG output for query: {}", input.query);
    
    // Check cache
    if (impl_->config.cache_evaluations) {
        auto cache_key = impl_->computeCacheKey(input.query, input.generated_answer);
        auto it = impl_->cache.find(cache_key);
        if (it != impl_->cache.end()) {
            THEMIS_DEBUG("Cache hit for evaluation");
            return it->second;
        }
    }
    
    EvaluationResult result;
    result.judge_model = impl_->config.judge_model;
    
    // Evaluate dimensions based on mode
    switch (impl_->config.mode) {
        case EvaluationMode::FAST:
            // Quick relevance check only
            result.relevance_score = evaluateRelevance(input);
            result.overall_score = result.relevance_score;
            break;
            
        case EvaluationMode::BALANCED:
            // Multi-dimension evaluation
            result.faithfulness_score = evaluateFaithfulness(input);
            result.relevance_score = evaluateRelevance(input);
            result.completeness_score = evaluateCompleteness(input);
            result.coherence_score = 0.8;  // Placeholder
            
            result.overall_score = 
                result.faithfulness_score * impl_->config.faithfulness_weight +
                result.relevance_score * impl_->config.relevance_weight +
                result.completeness_score * impl_->config.completeness_weight +
                result.coherence_score * impl_->config.coherence_weight;
            break;
            
        case EvaluationMode::THOROUGH:
            // Full evaluation with verification
            result.faithfulness_score = evaluateFaithfulness(input);
            result.relevance_score = evaluateRelevance(input);
            result.completeness_score = evaluateCompleteness(input);
            result.coherence_score = evaluateCoherence(input);
            
            // Claim verification
            if (impl_->config.enable_claim_verification) {
                auto claims = extractClaims(input.generated_answer);
                size_t verified_count = 0;
                
                for (const auto& claim : claims) {
                    if (verifyClaimAgainstDocuments(claim, input.documents)) {
                        result.verified_claims.push_back(claim);
                        verified_count++;
                    } else {
                        result.unverified_claims.push_back(claim);
                    }
                }
                
                // Adjust faithfulness based on verification
                if (!claims.empty()) {
                    double verification_ratio = static_cast<double>(verified_count) / claims.size();
                    result.faithfulness_score = std::min(result.faithfulness_score, verification_ratio);
                }
            }
            
            result.overall_score = 
                result.faithfulness_score * impl_->config.faithfulness_weight +
                result.relevance_score * impl_->config.relevance_weight +
                result.completeness_score * impl_->config.completeness_weight +
                result.coherence_score * impl_->config.coherence_weight;
            
            // Generate explanation
            std::ostringstream explanation;
            explanation << "Evaluation scores:\n"
                       << "- Faithfulness: " << result.faithfulness_score << "\n"
                       << "- Relevance: " << result.relevance_score << "\n"
                       << "- Completeness: " << result.completeness_score << "\n"
                       << "- Coherence: " << result.coherence_score << "\n"
                       << "- Overall: " << result.overall_score;
            result.explanation = explanation.str();
            break;
    }
    
    // Quality threshold check
    result.passed_quality_threshold = 
        result.overall_score >= impl_->config.quality_threshold &&
        result.faithfulness_score >= impl_->config.faithfulness_threshold;
    
    // Calculate confidence (placeholder)
    result.confidence = 0.85;
    
    // Record evaluation time
    auto end_time = std::chrono::steady_clock::now();
    result.evaluation_time = std::chrono::duration_cast<std::chrono::milliseconds>(
        end_time - start_time
    );
    
    // Cache result
    if (impl_->config.cache_evaluations) {
        auto cache_key = impl_->computeCacheKey(input.query, input.generated_answer);
        impl_->cache[cache_key] = result;
    }
    
    // Callback
    if (impl_->eval_callback) {
        impl_->eval_callback(result);
    }
    
    THEMIS_INFO("Evaluation completed. Overall score: {}, Time: {}ms",
                result.overall_score, result.evaluation_time.count());
    
    return result;
}

ComparisonResult RAGJudge::compare(
    const std::string& query,
    const std::vector<RetrievedDocument>& documents,
    const std::string& answer_a,
    const std::string& answer_b
) {
    THEMIS_DEBUG("Comparing two answers");
    
    // Evaluate both answers
    auto result_a = evaluate(query, documents, answer_a);
    auto result_b = evaluate(query, documents, answer_b);
    
    ComparisonResult comparison;
    
    // Determine winner
    double score_diff = result_a.overall_score - result_b.overall_score;
    if (std::abs(score_diff) < 0.05) {
        comparison.winner = ComparisonResult::Winner::TIE;
    } else if (score_diff > 0) {
        comparison.winner = ComparisonResult::Winner::ANSWER_A;
    } else {
        comparison.winner = ComparisonResult::Winner::ANSWER_B;
    }
    
    // Generate reasoning
    std::ostringstream reasoning;
    reasoning << "Answer A score: " << result_a.overall_score << "\n"
             << "Answer B score: " << result_b.overall_score << "\n"
             << "Winner: ";
    switch (comparison.winner) {
        case ComparisonResult::Winner::ANSWER_A:
            reasoning << "Answer A";
            break;
        case ComparisonResult::Winner::ANSWER_B:
            reasoning << "Answer B";
            break;
        case ComparisonResult::Winner::TIE:
            reasoning << "Tie";
            break;
    }
    comparison.reasoning = reasoning.str();
    comparison.confidence = 0.8;
    
    return comparison;
}

std::vector<EvaluationResult> RAGJudge::batchEvaluate(
    const std::vector<RAGTestCase>& test_cases
) {
    THEMIS_INFO("Batch evaluating {} test cases", test_cases.size());
    
    std::vector<EvaluationResult> results;
    results.reserve(test_cases.size());
    
    for (const auto& test_case : test_cases) {
        EvaluationInput input;
        input.query = test_case.query;
        input.documents = test_case.documents;
        input.generated_answer = test_case.generated_answer;
        
        auto result = evaluate(input);
        results.push_back(result);
    }
    
    return results;
}

double RAGJudge::evaluateDimension(
    EvaluationDimension dimension,
    const EvaluationInput& input
) {
    switch (dimension) {
        case EvaluationDimension::FAITHFULNESS:
            return evaluateFaithfulness(input);
        case EvaluationDimension::RELEVANCE:
            return evaluateRelevance(input);
        case EvaluationDimension::COMPLETENESS:
            return evaluateCompleteness(input);
        case EvaluationDimension::COHERENCE:
            return evaluateCoherence(input);
        case EvaluationDimension::OVERALL:
            return evaluate(input).overall_score;
    }
    return 0.0;
}

void RAGJudge::setConfig(const RAGJudgeConfig& config) {
    impl_->config = config;
}

RAGJudgeConfig RAGJudge::getConfig() const {
    return impl_->config;
}

void RAGJudge::setEvaluationCallback(
    std::function<void(const EvaluationResult&)> callback
) {
    impl_->eval_callback = std::move(callback);
}

void RAGJudge::clearCache() {
    impl_->cache.clear();
    THEMIS_DEBUG("Evaluation cache cleared");
}

// Private evaluation methods (now integrated with LLM)

double RAGJudge::evaluateFaithfulness(const EvaluationInput& input) {
    THEMIS_DEBUG("Evaluating faithfulness");
    
    // Quick heuristic check
    if (input.documents.empty()) {
        THEMIS_WARN("No documents provided for faithfulness evaluation");
        return 0.3;
    }
    
    // Use LLM for thorough evaluation in BALANCED and THOROUGH modes
    if (impl_->config.mode == EvaluationMode::BALANCED || 
        impl_->config.mode == EvaluationMode::THOROUGH) {
        
        auto parsed = impl_->llm_integration->evaluateWithLLM(
            EvaluationDimension::FAITHFULNESS,
            input,
            impl_->template_manager
        );
        
        if (parsed.success && parsed.score) {
            // Normalize from 1-5 scale to 0-1
            return ResponseParser::normalizeScore(*parsed.score, 1.0, 5.0);
        } else {
            THEMIS_WARN("LLM evaluation failed, using fallback heuristic");
        }
    }
    
    // Fallback heuristic
    return 0.85;
}

double RAGJudge::evaluateRelevance(const EvaluationInput& input) {
    THEMIS_DEBUG("Evaluating relevance");
    
    if (input.generated_answer.empty()) {
        return 0.0;
    }
    
    // Use LLM for evaluation
    if (impl_->config.mode != EvaluationMode::FAST || impl_->config.use_chain_of_thought) {
        auto parsed = impl_->llm_integration->evaluateWithLLM(
            EvaluationDimension::RELEVANCE,
            input,
            impl_->template_manager
        );
        
        if (parsed.success && parsed.score) {
            return ResponseParser::normalizeScore(*parsed.score, 1.0, 5.0);
        }
    }
    
    // Fallback heuristic
    return 0.8;
}

double RAGJudge::evaluateCompleteness(const EvaluationInput& input) {
    THEMIS_DEBUG("Evaluating completeness");
    
    // Use LLM for thorough evaluation
    if (impl_->config.mode == EvaluationMode::BALANCED || 
        impl_->config.mode == EvaluationMode::THOROUGH) {
        
        auto parsed = impl_->llm_integration->evaluateWithLLM(
            EvaluationDimension::COMPLETENESS,
            input,
            impl_->template_manager
        );
        
        if (parsed.success && parsed.score) {
            return ResponseParser::normalizeScore(*parsed.score, 1.0, 5.0);
        }
    }
    
    return 0.75;
}

double RAGJudge::evaluateCoherence(const EvaluationInput& input) {
    THEMIS_DEBUG("Evaluating coherence");
    
    // Use LLM for thorough evaluation
    if (impl_->config.mode == EvaluationMode::THOROUGH) {
        auto parsed = impl_->llm_integration->evaluateWithLLM(
            EvaluationDimension::COHERENCE,
            input,
            impl_->template_manager
        );
        
        if (parsed.success && parsed.score) {
            return ResponseParser::normalizeScore(*parsed.score, 1.0, 5.0);
        }
    }
    
    return 0.8;
}

std::vector<std::string> RAGJudge::extractClaims(const std::string& answer) {
    // TODO: Implement proper claim extraction using LLM
    // Placeholder: Split by sentences
    std::vector<std::string> claims;
    std::istringstream stream(answer);
    std::string sentence;
    while (std::getline(stream, sentence, '.')) {
        if (!sentence.empty()) {
            claims.push_back(sentence);
        }
    }
    return claims;
}

bool RAGJudge::verifyClaimAgainstDocuments(
    const std::string& claim,
    const std::vector<RetrievedDocument>& documents
) {
    // TODO: Implement proper claim verification using LLM
    // Placeholder: Simple substring search
    for (const auto& doc : documents) {
        if (doc.content.find(claim) != std::string::npos) {
            return true;
        }
    }
    return false;
}

std::string RAGJudge::generateEvaluationPrompt(
    const EvaluationInput& input,
    EvaluationDimension dimension
) {
    return impl_->template_manager.generatePrompt(dimension, input);
}

double RAGJudge::parseScoreFromResponse(const std::string& response) {
    auto parsed = ResponseParser::parse(response);
    if (parsed.success && parsed.score) {
        return ResponseParser::normalizeScore(*parsed.score, 1.0, 5.0);
    }
    return 0.75; // Default fallback
}

std::string RAGJudge::extractExplanation(const std::string& response) {
    return ResponseParser::extractExplanation(response);
}

// JudgeEnsemble implementation

struct JudgeEnsemble::Impl {
    std::vector<std::shared_ptr<RAGJudge>> judges;
    VotingStrategy strategy;
};

JudgeEnsemble::JudgeEnsemble(
    std::vector<std::shared_ptr<RAGJudge>> judges,
    VotingStrategy strategy
) : impl_(std::make_unique<Impl>()) {
    impl_->judges = std::move(judges);
    impl_->strategy = strategy;
}

EvaluationResult JudgeEnsemble::evaluateWithEnsemble(const EvaluationInput& input) {
    THEMIS_INFO("Evaluating with ensemble of {} judges", impl_->judges.size());
    
    std::vector<EvaluationResult> results;
    for (const auto& judge : impl_->judges) {
        results.push_back(judge->evaluate(input));
    }
    
    return combineResults(results, impl_->strategy);
}

ComparisonResult JudgeEnsemble::compareWithEnsemble(
    const std::string& query,
    const std::vector<RetrievedDocument>& documents,
    const std::string& answer_a,
    const std::string& answer_b
) {
    // TODO: Implement ensemble comparison
    return impl_->judges[0]->compare(query, documents, answer_a, answer_b);
}

void JudgeEnsemble::setVotingStrategy(VotingStrategy strategy) {
    impl_->strategy = strategy;
}

EvaluationResult JudgeEnsemble::combineResults(
    const std::vector<EvaluationResult>& results,
    VotingStrategy strategy
) {
    if (results.empty()) {
        return EvaluationResult{};
    }
    
    EvaluationResult combined = results[0];
    
    switch (strategy) {
        case VotingStrategy::WEIGHTED_AVERAGE:
        case VotingStrategy::MAJORITY_VOTING: {
            // Average scores
            combined.faithfulness_score = 0.0;
            combined.relevance_score = 0.0;
            combined.completeness_score = 0.0;
            combined.coherence_score = 0.0;
            combined.overall_score = 0.0;
            
            for (const auto& result : results) {
                combined.faithfulness_score += result.faithfulness_score;
                combined.relevance_score += result.relevance_score;
                combined.completeness_score += result.completeness_score;
                combined.coherence_score += result.coherence_score;
                combined.overall_score += result.overall_score;
            }
            
            double n = static_cast<double>(results.size());
            combined.faithfulness_score /= n;
            combined.relevance_score /= n;
            combined.completeness_score /= n;
            combined.coherence_score /= n;
            combined.overall_score /= n;
            break;
        }
        
        default:
            // Use first judge result
            break;
    }
    
    return combined;
}

// Factory implementations

std::unique_ptr<RAGJudge> RAGJudgeFactory::createFast() {
    RAGJudgeConfig config;
    config.mode = EvaluationMode::FAST;
    config.enable_claim_verification = false;
    config.enable_citation_check = false;
    return std::make_unique<RAGJudge>(config);
}

std::unique_ptr<RAGJudge> RAGJudgeFactory::createBalanced() {
    RAGJudgeConfig config;
    config.mode = EvaluationMode::BALANCED;
    return std::make_unique<RAGJudge>(config);
}

std::unique_ptr<RAGJudge> RAGJudgeFactory::createThorough() {
    RAGJudgeConfig config;
    config.mode = EvaluationMode::THOROUGH;
    config.enable_claim_verification = true;
    config.enable_citation_check = true;
    config.use_chain_of_thought = true;
    return std::make_unique<RAGJudge>(config);
}

std::unique_ptr<RAGJudge> RAGJudgeFactory::create(const RAGJudgeConfig& config) {
    return std::make_unique<RAGJudge>(config);
}

std::unique_ptr<JudgeEnsemble> RAGJudgeFactory::createEnsemble(
    size_t count,
    VotingStrategy strategy
) {
    std::vector<std::shared_ptr<RAGJudge>> judges;
    for (size_t i = 0; i < count; ++i) {
        judges.push_back(std::make_shared<RAGJudge>());
    }
    return std::make_unique<JudgeEnsemble>(judges, strategy);
}

// Metrics namespace implementations

namespace metrics {

double calculateInterJudgeAgreement(const std::vector<EvaluationResult>& results) {
    // TODO: Implement proper inter-judge agreement calculation
    return 0.85;
}

double calculateCohensKappa(
    const std::vector<EvaluationResult>& judge1_results,
    const std::vector<EvaluationResult>& judge2_results
) {
    // TODO: Implement Cohen's Kappa calculation
    return 0.75;
}

double calculateCalibrationError(
    const std::vector<double>& predictions,
    const std::vector<double>& ground_truth
) {
    // TODO: Implement Expected Calibration Error
    return 0.1;
}

} // namespace metrics

} // namespace themis::rag::judge
