/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            llm_judge_integration.cpp                          ║
  Version:         0.0.47                                             ║
  Last Modified:   2026-04-15 18:50:29                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟠 BETA                                         ║
    • Quality Score:   59.0/100                                       ║
    • Total Lines:     237                                            ║
    • Open Issues:     TODOs: 0, Stubs: 2                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 67965456c8  2026-03-22  Add constructors with default config for various classes ... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: 🔧 In Progress                                               ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/**
 * @file llm_judge_integration.cpp
 * @brief Implementation of LLM integration for RAG Judge
 */

#include "rag/llm_judge_integration.h"
#include "utils/logger.h"
#include "utils/retry_policy.h"
#include <thread>
#include <chrono>
#include <stdexcept>

namespace themis::rag::judge {

LLMJudgeIntegration::LLMJudgeIntegration(ILLMInferenceEngine* engine)
    : LLMJudgeIntegration(engine, Config{}) {
}

LLMJudgeIntegration::LLMJudgeIntegration(ILLMInferenceEngine* engine, const Config& config)
    : config_(config), mock_mode_active_(false), mock_mode_warning_shown_(false) {
    if (engine == nullptr && !config_.allow_mock) {
        throw std::invalid_argument(
            "LLMJudgeIntegration: engine must not be nullptr when allow_mock is false. "
            "Pass a valid ILLMInferenceEngine* or set config.allow_mock = true for testing.");
    }
    if (engine != nullptr) {
        // Wire the engine's generate() into the inference function slot
        inference_fn_ = [engine](const std::string& prompt) {
            return engine->generate(prompt);
        };
        mock_mode_active_ = false;
        THEMIS_INFO("LLMJudgeIntegration initialized with injected inference engine");
    } else {
        // allow_mock = true AND engine = nullptr → fall back to default mock
        inference_fn_ = defaultInference;
        mock_mode_active_ = true;
        if (config_.warn_on_mock_mode) {
            THEMIS_WARN("LLMJudgeIntegration initialized with nullptr engine in MOCK MODE "
                        "(allow_mock=true) - evaluations will use stub responses");
        }
    }
}

LLMJudgeIntegration::LLMJudgeIntegration()
    : LLMJudgeIntegration(Config{}) {
}

LLMJudgeIntegration::LLMJudgeIntegration(const Config& config)
    : config_(config), mock_mode_active_(false), mock_mode_warning_shown_(false) {
    // Only set default inference function if mock mode is explicitly enabled
    if (config_.use_mock_mode) {
        inference_fn_ = defaultInference;
        mock_mode_active_ = true;
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
    THEMIS_DEBUG("Evaluating dimension {} with LLM", static_cast&lt;int&gt;(dimension));
    
    // Generate prompt
    std::string prompt = template_mgr.generatePrompt(dimension, input);
    
    if (prompt.empty()) {
        THEMIS_ERROR("Failed to generate prompt for dimension {}", static_cast&lt;int&gt;(dimension));
        ParsedResponse error_response;
        error_response.success = false;
        error_response.error_message = "Failed to generate prompt";
        return error_response;
    }
    
    // Call LLM with retries
    const themis::utils::RetryConfig llm_retry_cfg{
        /* max_attempts       */ static_cast<uint32_t>(config_.max_retries),
        /* initial_backoff_ms */ 100u,
        /* max_backoff_ms     */ 30'000u,
        /* multiplier         */ 2.0,
        /* jitter_fraction    */ 0.0,
    };
    auto llm_result = themis::utils::retry_with_backoff(
        [&]() -> std::optional<std::string> {
            try {
                auto r = callLLM(prompt);
                if (!r.empty()) return r;
            } catch (const std::exception& e) {
                THEMIS_WARN("LLM call failed: {}", e.what());
            }
            return std::nullopt;
        },
        llm_retry_cfg);

    std::string llm_response = llm_result.value_or("");
    
    if (llm_response.empty()) {
        THEMIS_ERROR("LLM failed to respond after {} attempts", config_.max_retries);
        ParsedResponse error_response;
        error_response.success = false;
        error_response.error_message = "LLM failed to respond";
        return error_response;
    }
    
    // Parse response
    ParsedResponse parsed = ResponseParser::parse(llm_response);

    // Gap 7 (AI_ML_IMPACT_ASSESSMENT.md §7): mark the result as mock-produced
    // so callers can filter it from production dashboards without having to
    // separately call isMockMode().
    if (isMockMode()) {
        parsed.is_mock = true;
    }

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
                 static_cast&lt;int&gt;(dimension), prompt.size());

    const themis::utils::RetryConfig dim_retry_cfg{
        /* max_attempts       */ static_cast<uint32_t>(config_.max_retries),
        /* initial_backoff_ms */ 100u,
        /* max_backoff_ms     */ 30'000u,
        /* multiplier         */ 2.0,
        /* jitter_fraction    */ 0.0,
    };
    auto dim_result = themis::utils::retry_with_backoff(
        [&]() -> std::optional<std::string> {
            try {
                auto r = callLLM(prompt);
                if (!r.empty()) return r;
            } catch (const std::exception& e) {
                THEMIS_WARN("LLM call failed (dim={}): {}", static_cast&lt;int&gt;(dimension), e.what());
            }
            return std::nullopt;
        },
        dim_retry_cfg);

    if (dim_result) return *dim_result;

    THEMIS_ERROR("LLM failed to respond for dimension {}", static_cast&lt;int&gt;(dimension));
    return "{}";  // Return empty JSON object as safe fallback
}

void LLMJudgeIntegration::setInferenceFunction(
    std::function<std::string(const std::string&)> fn
) {
    inference_fn_ = fn;
    mock_mode_active_ = false;
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
        error_msg += "Options: (1) call setInferenceFunction() with a valid LLM inference function; ";
        error_msg += "(2) pass an ILLMInferenceEngine* to the constructor; ";
        error_msg += "(3) set config.allow_mock = true or config.use_mock_mode = true for testing.";
        THEMIS_ERROR("{}", error_msg);
        throw std::runtime_error(error_msg);
    }
    
    // Warn once if in mock mode
    if (mock_mode_active_ && config_.warn_on_mock_mode && !mock_mode_warning_shown_) {
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
    // STUB/SIMULATION NOTE:
    // Purpose: Provide a structurally-valid LLM-judge response when no real
    //          ILLMInferenceEngine is injected, enabling unit tests and offline
    //          evaluation pipelines without a live model endpoint.
    // Activation: Called only when config.use_mock_mode == true or allow_mock == true
    //             AND engine == nullptr.  Production deployments always inject a real
    //             engine; the mock path is never reached.
    // Production Delta: Returns a hardcoded score=4.0 / confidence=0.85 regardless
    //                   of the prompt content.  Real scores are model-generated and
    //                   prompt-dependent.  As of 2026-04-21 the caller (evaluateWithLLM)
    //                   sets ParsedResponse::is_mock=true on the parsed result so
    //                   callers can filter mock data from production dashboards
    //                   (AI_ML_IMPACT_ASSESSMENT.md §7, Gap 7 — implemented).
    // Roadmap ref: src/rag/ROADMAP.md § "Phase 9: AI Reliability & Safety Evaluation Program"
    // Removal Plan: Full removal when LLMTokenBudgetManager (Gap 6) and a real engine
    //               DI path are the only supported entry points.  Track in
    //               rag/FUTURE_ENHANCEMENTS.md §Gap 7.
    // Roadmap ref: src/rag/FUTURE_ENHANCEMENTS.md § "LLMIntegration and LLMJudgeIntegration: Replace Stub/Mock Mode"
    (void)prompt; // unused in mock path — intentional
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
    return mock_mode_active_;
}

} // namespace themis::rag::judge
