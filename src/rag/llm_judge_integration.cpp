/**
 * @file llm_judge_integration.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 87/100
 * @note Gap Summary: total=24; TODO=1, Stub=5, Unimpl=0, Mock=16, Sim=1, Debt=1, C=2, H=24, M=2, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#include "rag/llm_judge_integration.h"
#include "utils/logger.h"
#include "utils/retry_policy.h"
#include <thread>
#include <chrono>
#include <stdexcept>
#include <functional>
#include <iomanip>
#include <sstream>

namespace themis::rag::judge {

namespace {
ParsedResponse makeUnavailableJudgeResponse(std::string message) {
    ParsedResponse response{};
    response.success = false;
    response.score = -1.0;
    response.confidence = 0.0;
    response.reasoning = "llm_unavailable";
    response.error_message = std::move(message);
    response.is_mock = false;
    return response;
}
} // namespace

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
        if (config_.allow_mock && config_.use_mock_mode) {
            inference_fn_ = defaultInference;
            mock_mode_active_ = true;
            if (config_.warn_on_mock_mode) {
                THEMIS_WARN("LLMJudgeIntegration initialized with nullptr engine in MOCK MODE "
                            "(allow_mock=true + use_mock_mode=true) - test-only path");
            }
        } else {
            inference_fn_ = nullptr;
            mock_mode_active_ = false;
            THEMIS_WARN("LLMJudgeIntegration initialized without engine and without explicit mock mode; "
                        "calls will return llm_unavailable");
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
    if (!config_.enable_llm_judge) {
        return makeUnavailableJudgeResponse("llm_unavailable: THEMIS_ENABLE_LLM_JUDGE gate is disabled");
    }

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
        return makeUnavailableJudgeResponse("llm_unavailable: backend did not return a response");
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
    if (!config_.enable_llm_judge) {
        return R"({"score":-1,"reason":"llm_unavailable","success":false})";
    }

    THEMIS_DEBUG("LLMJudgeIntegration::evaluateDimension dim={} prompt_len={}",
                 static_cast<int>(dimension), prompt.size());

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
                THEMIS_WARN("LLM call failed (dim={}): {}", static_cast<int>(dimension), e.what());
            }
            return std::nullopt;
        },
        dim_retry_cfg);

    if (dim_result) return *dim_result;

    THEMIS_ERROR("LLM failed to respond for dimension {}", static_cast<int>(dimension));
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
    if (!config_.enable_llm_judge) {
        throw std::runtime_error("llm_unavailable: THEMIS_ENABLE_LLM_JUDGE gate is disabled");
    }
    if (!inference_fn_) {
        std::string error_msg = "llm_unavailable: no inference backend configured. ";
        error_msg += "Provide ILLMBackend via constructor or setInferenceFunction(); ";
        error_msg += "optional mock path requires allow_mock=true and use_mock_mode=true.";
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
    
    // Validate response before returning
    if (response.empty()) {
        THEMIS_WARN("LLMJudgeIntegration: Empty response from inference function");
        // Return a structured fallback response
        return R"({"score":0.0,"confidence":0.0,"reasoning":"llm_unavailable: empty_response","supporting_claims":[],"unsupported_claims":[],"success":false})";
    }
    
    THEMIS_DEBUG("LLM responded with length: {} chars", response.length());
    
    return response;
}

std::string LLMJudgeIntegration::defaultInference(const std::string& prompt) {
    // PERMANENT FALLBACK NOTE:
    // Purpose: Provide a structurally-valid LLM-judge response when no real
    //          ILLMInferenceEngine is injected, enabling unit tests and offline
    //          evaluation pipelines without a live model endpoint.
    // Activation: Called only when config.use_mock_mode == true or allow_mock == true
    //             AND engine == nullptr.  Production deployments always inject a real
    //             engine via the constructor or setInferenceFunction(); this path is
    //             never reached in production.
    // Production path: Inject via constructor:
    //     LLMJudgeIntegration judge(engine_ptr);
    //   or at runtime:
    //     judge.setInferenceFunction([&model](const std::string& p){ return model.generate(p); });
    // Behaviour: Uses deterministic prompt-hash heuristics; ParsedResponse::is_mock
    //            is set to true by evaluateWithLLM() so callers can filter mock data
    //            from production dashboards (AI_ML_IMPACT_ASSESSMENT.md §7).
    THEMIS_DEBUG("Using mock inference function (for testing only)");

    const auto prompt_hash = std::hash<std::string>{}(prompt);
    const double score = 2.5 + static_cast<double>(prompt_hash % 251) / 100.0;  // [2.50, 5.00]
    const double confidence = 0.60 + static_cast<double>((prompt_hash >> 8U) % 351U) / 1000.0;  // [0.600, 0.951]

    std::ostringstream mock_response;
    mock_response << std::fixed << std::setprecision(3)
                  << "{\n"
                  << "  \"score\": " << score << ",\n"
                  << "  \"confidence\": " << confidence << ",\n"
                  << "  \"reasoning\": \"Mock evaluation generated from prompt hash. Replace with a real LLM engine for production scoring.\",\n"
                  << "  \"supporting_claims\": [\"Mock claim " << (1U + (prompt_hash % 3U)) << "\"],\n"
                  << "  \"unsupported_claims\": []\n"
                  << "}";
    return mock_response.str();
}

bool LLMJudgeIntegration::isMockMode() const {
    return mock_mode_active_;
}

} // namespace themis::rag::judge
