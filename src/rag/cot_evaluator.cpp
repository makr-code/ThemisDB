/**
 * @file cot_evaluator.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 100/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=4, M=3, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#include "rag/cot_evaluator.h"
#include "rag/llm_judge_integration.h"
#include "rag/response_parser.h"
#include "utils/logger.h"
#include <nlohmann/json.hpp>
#include <sstream>
#include <regex>
#include <mutex>
#include <set>
#include <iterator>
#include <algorithm>

namespace themis::rag::judge {

using json = nlohmann::json;

struct CoTEvaluator::Impl {
    Config config;
    std::unique_ptr<LLMJudgeIntegration> llm_integration;
    ResponseParser parser;
    mutable std::mutex state_mutex;  // Protect shared state access
    
    // Generate CoT prompt
    std::string generateCoTPrompt(
        const std::string& query,
        const std::string& answer,
        const std::vector<std::pair<std::string, std::string>>& documents,
        const std::string& dimension
    ) {
        std::ostringstream prompt = {};
        
        prompt << "Evaluate the following answer using chain-of-thought reasoning.\n\n";
        prompt << "Dimension: " << dimension << "\n\n";
        prompt << "Query: " << query << "\n\n";
        
        if (!documents.empty()) {
            prompt << "Retrieved Documents:\n";
            for (size_t i = 0; i < std::min(documents.size(), size_t(3)); ++i) {
                prompt << "Doc " << (i+1) << ": " << documents[i].second << "\n";
            }
            prompt << "\n";
        }
        
        prompt << "Answer: " << answer << "\n\n";
        
        prompt << R"(Evaluate step-by-step:

Step 1: [Ask a relevant question]
Observation: [What do you observe about the answer?]
Evidence: [What evidence supports your observation?]
Conclusion: [What can you conclude from this step?]

Step 2: [Ask another relevant question]
Observation: [What do you observe?]
Evidence: [What evidence do you find?]
Conclusion: [What can you conclude?]

Step 3: [Ask another relevant question]
Observation: [What do you observe?]
Evidence: [What evidence do you find?]
Conclusion: [What can you conclude?]

Final Score: [0.0-1.0]
Final Reasoning: [Brief summary of your reasoning]

Evaluation:)";
        
        return prompt.str();
    }
};

CoTEvaluator::CoTEvaluator()
    : CoTEvaluator(Config{}) {
}

CoTEvaluator::CoTEvaluator(const Config& config)
    : impl_(std::make_unique<Impl>()) {
    impl_->config = config;
    
    // Initialize LLM integration
    LLMJudgeIntegration::Config llm_config;
    llm_config.model_name = "default";
    llm_config.temperature = 0.3;
    llm_config.max_tokens = 1024;  // More tokens for detailed reasoning
    impl_->llm_integration = std::make_unique<LLMJudgeIntegration>(llm_config);
    
    THEMIS_DEBUG("CoTEvaluator initialized");
}

CoTEvaluator::~CoTEvaluator() = default;

std::vector<ReasoningStep> CoTEvaluator::parseCoTResponse(const std::string& response) {
    std::vector<ReasoningStep> steps;
    
    // Parse step-by-step reasoning
    std::regex step_regex(R"(Step\s+(\d+):\s*(.+?)\s*(?:Observation|Question):\s*(.+?)\s*(?:Evidence|Support):\s*(.+?)\s*Conclusion:\s*(.+?)(?=Step|\n\n|Final|$))",
                         std::regex::icase);
    
    auto steps_begin = std::sregex_iterator(response.begin(), response.end(), step_regex);
    auto steps_end = std::sregex_iterator();
    
    for (auto it = steps_begin; it != steps_end; ++it) {
        ReasoningStep step;
        
        try {
            step.step_number = std::stoi(it->str(1));
            step.question = it->str(2);
            step.observation = it->str(3);
            step.evidence = it->str(4);
            step.conclusion = it->str(5);
            
            steps.push_back(step);
        } catch (const std::exception& e) {
            THEMIS_WARN("Failed to parse reasoning step: {}", e.what());
        }
    }
    
    // Fallback: simple line-based parsing
    if (steps.empty()) {
        std::istringstream stream(response);
        std::string line = {};
        int step_num = 0;
        ReasoningStep current_step;
        
        while (std::getline(stream, line)) {
            if (line.find("Step") != std::string::npos) {
                if (step_num > 0) {
                    steps.push_back(current_step);
                }
                current_step = ReasoningStep();
                current_step.step_number = ++step_num;
                current_step.question = line;
            } else if (line.find("Observation:") != std::string::npos || 
                      line.find("Question:") != std::string::npos) {
                current_step.observation = line;
            } else if (line.find("Evidence:") != std::string::npos) {
                current_step.evidence = line;
            } else if (line.find("Conclusion:") != std::string::npos) {
                current_step.conclusion = line;
            }
        }
        
        if (step_num > 0) {
            steps.push_back(current_step);
        }
    }
    
    THEMIS_DEBUG("Parsed {} reasoning steps",static_cast<int>(steps.size()));
    return steps;
}

std::vector<std::string> CoTEvaluator::validateLogicConsistency(
    const std::vector<ReasoningStep>& steps
) {
    std::vector<std::string> inconsistencies;
    
    if (!impl_->config.enable_logic_validation) {
        return inconsistencies;
    }
    
    // Reserve space for expected inconsistencies
    inconsistencies.reserve(std::max<size_t>(size_t(1), steps.size() / 4));
    
    // Check for contradictions between steps
    for (size_t i = 0; i < steps.size(); ++i) {
        for (size_t j = i + 1; j < steps.size(); ++j) {
            const auto& step_i = steps[i];
            const auto& step_j = steps[j];
            
            // Simple contradiction detection using negation words
            std::string conclusion_i = step_i.conclusion;
            std::string conclusion_j = step_j.conclusion;
            
            std::transform(conclusion_i.begin(), conclusion_i.end(), 
                          conclusion_i.begin(), ::tolower);
            std::transform(conclusion_j.begin(), conclusion_j.end(), 
                          conclusion_j.begin(), ::tolower);
            
            // Check for opposing conclusions
            std::vector<std::string> negations = {"not", "no", "never", "cannot", "isn't", "doesn't"};
            
            bool i_has_negation = false;
            bool j_has_negation = false;
            
            for (const auto& neg : negations) {
                if (conclusion_i.find(neg) != std::string::npos) {
                  i_has_negation = true;
                }
                if (conclusion_j.find(neg) != std::string::npos) {
                  j_has_negation = true;
                }
            }
            
            // If one is negated and other isn't, check for common key terms
            if (i_has_negation != j_has_negation) {
                // Extract key terms
                std::set<std::string> terms_i, terms_j;
                std::istringstream stream_i(conclusion_i), stream_j(conclusion_j);
                std::string word = {};
                
                while (stream_i >> word) {
                    if (word.length() > 4) {
                      terms_i.insert(word);
                    }
                }
                while (stream_j >> word) {
                    if (word.length() > 4) {
                      terms_j.insert(word);
                    }
                }
                
                std::set<std::string> common;
                std::set_intersection(terms_i.begin(), terms_i.end(),
                                    terms_j.begin(), terms_j.end(),
                                    std::inserter(common, common.begin()));
                
                if (static_cast<int>(common.size()) >= 2) {
                    std::ostringstream inconsistency = {};
                    inconsistency << "Potential contradiction between Step " 
                                 << step_i.step_number << " and Step " 
                                 << step_j.step_number;
                    inconsistencies.push_back(inconsistency.str());
                }
            }
        }
    }
    
    return inconsistencies;
}

double CoTEvaluator::extractFinalScore(
    const std::vector<ReasoningStep>& steps,
    const std::string& response
) {
    // Try to extract from "Final Score:" line
    std::regex score_regex(R"(Final\s+Score:\s*([0-9.]+))", std::regex::icase);
    std::smatch match = {};
    
    if (std::regex_search(response, match, score_regex)) {
        try {
            double score = std::stod(match[1].str());
            return std::max(0.0, std::min(1.0, score));
        } catch (const std::exception& e) {
            THEMIS_WARN("Failed to parse final score: {}", e.what());
        }
    }
    
    // Fallback: Use number of positive conclusions
    if (!steps.empty()) {
        int positive_count = 0;
        for (const auto& step : steps) {
            std::string conclusion = step.conclusion;
            std::transform(conclusion.begin(), conclusion.end(), 
                          conclusion.begin(), ::tolower);
            
            // Simple sentiment analysis
            if (conclusion.find("good") != std::string::npos ||
                conclusion.find("supported") != std::string::npos ||
                conclusion.find("accurate") != std::string::npos ||
                conclusion.find("relevant") != std::string::npos) {
                positive_count++;
            }
        }
        
        return static_cast<double>(positive_count) / static_cast<double>(steps.size());
    }
    
    return 0.5;  // Default neutral score
}

CoTEvaluationResult CoTEvaluator::evaluate(
    const std::string& query,
    const std::string& answer,
    const std::vector<std::pair<std::string, std::string>>& documents,
    const std::string& dimension
) {
    CoTEvaluationResult result;
    
    THEMIS_DEBUG("Starting CoT evaluation for dimension: {}", dimension);
    
    // Generate and execute CoT prompt
    std::string prompt = impl_->generateCoTPrompt(query, answer, documents, dimension);
    
    try {
        std::string response = impl_->llm_integration->evaluateDimension(
            prompt, EvaluationDimension::OVERALL
        );
        
        // Parse reasoning steps
        result.reasoning_steps = parseCoTResponse(response);
        
        // Extract final score
        result.final_score = extractFinalScore(result.reasoning_steps, response);
        
        // Validate logic consistency
        result.inconsistencies = validateLogicConsistency(result.reasoning_steps);
        result.logic_consistent = result.inconsistencies.empty();
        
        // Extract final reasoning
        std::regex reasoning_regex(R"(Final\s+Reasoning:\s*(.+?)(?:\n\n|$))", 
                      std::regex::icase);
        std::smatch match = {};
        
        if (std::regex_search(response, match, reasoning_regex)) {
            result.final_reasoning = match[1].str();
        } else {
            // Fallback: use last step conclusion
            if (!result.reasoning_steps.empty()) {
                result.final_reasoning = result.reasoning_steps.back().conclusion;
            }
        }
        
    } catch (const std::exception& e) {
        THEMIS_ERROR("CoT evaluation failed: {}", e.what());
        result.final_score = 0.5;
        result.logic_consistent = false;
    }
    
    THEMIS_INFO("CoT evaluation complete: score={:.2f}, steps={}, consistent={}",
                result.final_score,static_cast<int>(result.reasoning_steps.size()), result.logic_consistent);
    
    return result;
}

} // namespace themis::rag::judge
