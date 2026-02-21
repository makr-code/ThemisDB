/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            llm_judge_integration.cpp                          ║
  Version:         0.0.17                                             ║
  Last Modified:   2026-02-21 18:23:08                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟡 RELEASE-CANDIDATE                            ║
    • Quality Score:   62.0/100                                       ║
    • Total Lines:     211                                            ║
    • Open Issues:     TODOs: 0, Stubs: 2                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 56752fde6  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • c3f305f42  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • e178371a5  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 234245ceb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • b8b369411  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ⚠️  Needs Work                                              ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/**
 * @file llm_judge_integration.cpp
 * @brief Implementation of LLM integration for RAG Judge
 */

#include "rag/llm_judge_integration.h"
#include "utils/logger.h"
#include <thread>
#include <chrono>

namespace themis::rag::judge {

LLMJudgeIntegration::LLMJudgeIntegration()
    : LLMJudgeIntegration(Config{}) {
}

LLMJudgeIntegration::LLMJudgeIntegration(const Config& config)
    : config_(config), mock_mode_warning_shown_(false) {
    // Only set default inference function if mock mode is explicitly enabled
    if (config_.use_mock_mode) {
        inference_fn_ = defaultInference;
        if (config_.warn_on_mock_mode) {
            THEMIS_WARN("LLMJudgeIntegration initialized in MOCK MODE - evaluations will use stub responses");
        }
    } else {
        // In production mode, require inference function to be set
        inference_fn_ = nullptr;
        THEMIS_INFO("LLMJudgeIntegration initialized - inference function must be set before use");
    }
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
        // Provide helpful error message
        std::string error_msg = "No inference function set. ";
        error_msg += "Call setInferenceFunction() with a valid LLM inference function, ";
        error_msg += "or enable mock mode (config.use_mock_mode = true) for testing.";
        THEMIS_ERROR("{}", error_msg);
        throw std::runtime_error(error_msg);
    }
    
    // Warn once if in mock mode
    if (config_.use_mock_mode && config_.warn_on_mock_mode && !mock_mode_warning_shown_) {
        THEMIS_WARN("LLM evaluation using MOCK MODE - results are not real (warning shown once)");
        mock_mode_warning_shown_ = true;
    }
    
    THEMIS_DEBUG("Calling LLM with prompt length: {} chars", prompt.length());
    
    // Call the inference function
    std::string response = inference_fn_(prompt);
    
    THEMIS_DEBUG("LLM responded with length: {} chars", response.length());
    
    return response;
}

std::string LLMJudgeIntegration::defaultInference(const std::string& prompt) {
    // Mock inference function for testing only
    // This should only be used when explicitly enabled via config.use_mock_mode = true
    
    THEMIS_DEBUG("Using mock inference function (for testing only)");
    
    // Return a mock JSON response
    return R"({
  "score": 4.0,
  "confidence": 0.85,
  "reasoning": "This is a mock evaluation response. The actual LLM inference engine is not connected.",
  "supporting_claims": ["Mock claim 1", "Mock claim 2"],
  "unsupported_claims": []
})";
}

bool LLMJudgeIntegration::isMockMode() const {
    return config_.use_mock_mode;
}

} // namespace themis::rag::judge
