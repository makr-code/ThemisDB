/**
 * @file llm_judge_integration.cpp
 * @brief Implementation of LLM integration for RAG Judge
 */

#include "rag/llm_judge_integration.h"
#include "utils/logger.h"
#include <thread>
#include <chrono>

namespace themis::rag::judge {

LLMJudgeIntegration::LLMJudgeIntegration(const Config& config)
    : config_(config) {
    // Set default inference function (stub for now)
    inference_fn_ = defaultInference;
}

ParsedResponse LLMJudgeIntegration::evaluateWithLLM(
    EvaluationDimension dimension,
    const EvaluationInput& input,
    const PromptTemplateManager& template_mgr
) {
    THEMIS_DEBUG("Evaluating dimension {} with LLM", static_cast<int>(dimension));
    
    // Generate prompt
    std::string prompt = template_mgr.generatePrompt(dimension, input);
    
    if (prompt.empty()) {
        THEMIS_ERROR("Failed to generate prompt for dimension {}", static_cast<int>(dimension));
        ParsedResponse error_response;
        error_response.success = false;
        error_response.error_message = "Failed to generate prompt";
        return error_response;
    }
    
    // Call LLM with retries
    std::string llm_response;
    int attempts = 0;
    
    while (attempts < config_.max_retries) {
        try {
            llm_response = callLLM(prompt);
            
            if (!llm_response.empty()) {
                break;
            }
        } catch (const std::exception& e) {
            THEMIS_WARN("LLM call failed (attempt {}/{}): {}", 
                       attempts + 1, config_.max_retries, e.what());
        }
        
        attempts++;
        
        if (attempts < config_.max_retries) {
            // Exponential backoff
            int delay_ms = 100 * (1 << attempts);
            std::this_thread::sleep_for(std::chrono::milliseconds(delay_ms));
        }
    }
    
    if (llm_response.empty()) {
        THEMIS_ERROR("LLM failed to respond after {} attempts", config_.max_retries);
        ParsedResponse error_response;
        error_response.success = false;
        error_response.error_message = "LLM failed to respond";
        return error_response;
    }
    
    // Parse response
    ParsedResponse parsed = ResponseParser::parse(llm_response);
    
    if (!parsed.success) {
        THEMIS_WARN("Failed to parse LLM response: {}", parsed.error_message);
    }
    
    return parsed;
}

std::string LLMJudgeIntegration::evaluateDimension(
    const std::string& prompt,
    EvaluationDimension dimension
) {
    THEMIS_DEBUG("LLMJudgeIntegration::evaluateDimension dim={} prompt_len={}",
                 static_cast<int>(dimension), prompt.size());

    int attempts = 0;
    std::string response;

    while (attempts < config_.max_retries) {
        try {
            response = callLLM(prompt);
            if (!response.empty()) {
                return response;
            }
        } catch (const std::exception& e) {
            THEMIS_WARN("LLM call failed (attempt {}/{}): {}",
                        attempts + 1, config_.max_retries, e.what());
        }
        attempts++;
        if (attempts < config_.max_retries) {
            int delay_ms = 100 * (1 << attempts);
            std::this_thread::sleep_for(std::chrono::milliseconds(delay_ms));
        }
    }

    THEMIS_ERROR("LLM failed to respond for dimension {}", static_cast<int>(dimension));
    return "{}";  // Return empty JSON object as safe fallback
}

void LLMJudgeIntegration::setInferenceFunction(
    std::function<std::string(const std::string&)> fn
) {
    inference_fn_ = fn;
    THEMIS_INFO("Custom inference function set for LLM judge");
}

void LLMJudgeIntegration::setConfig(const Config& config) {
    config_ = config;
}

LLMJudgeIntegration::Config LLMJudgeIntegration::getConfig() const {
    return config_;
}

std::string LLMJudgeIntegration::callLLM(const std::string& prompt) {
    if (!inference_fn_) {
        throw std::runtime_error("No inference function set");
    }
    
    THEMIS_DEBUG("Calling LLM with prompt length: {} chars", prompt.length());
    
    // Call the inference function
    std::string response = inference_fn_(prompt);
    
    THEMIS_DEBUG("LLM responded with length: {} chars", response.length());
    
    return response;
}

std::string LLMJudgeIntegration::defaultInference(const std::string& prompt) {
    // This is a stub implementation that returns a mock response
    // In production, this should call the actual LLM inference engine
    
    THEMIS_DEBUG("Using default (stub) inference function");
    
    // Return a mock JSON response
    return R"({
  "score": 4.0,
  "confidence": 0.85,
  "reasoning": "This is a mock evaluation response. The actual LLM inference engine is not connected.",
  "supporting_claims": ["Mock claim 1", "Mock claim 2"],
  "unsupported_claims": []
})";
}

} // namespace themis::rag::judge
