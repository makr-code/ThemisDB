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
#include <sstream>
#include <string_view>

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

std::string makeUnavailableJudgeJson(std::string_view reason) {
    std::ostringstream response = {};
    response << R"({"score":-1,"confidence":0.0,"reasoning":")"
             << reason
             << R"(","success":false})";
    return response.str();
}
} // namespace

LLMJudgeIntegration::LLMJudgeIntegration(ILLMInferenceEngine* engine)
    : LLMJudgeIntegration(engine, Config{}) {
}

LLMJudgeIntegration::LLMJudgeIntegration(ILLMInferenceEngine* engine, const Config& config)
    : config_(config) {
    if (engine == nullptr) {
        throw std::invalid_argument(
            "LLMJudgeIntegration: engine must not be nullptr. "
            "Pass a valid ILLMInferenceEngine* or use the config-only constructor "
            "and setInferenceFunction() before evaluation.");
    }

    inference_fn_ = [engine](const std::string& prompt) {
        return engine->generate(prompt);
    };
    THEMIS_INFO("LLMJudgeIntegration initialized with injected inference engine");
}

LLMJudgeIntegration::LLMJudgeIntegration()
    : LLMJudgeIntegration(Config{}) {
}

LLMJudgeIntegration::LLMJudgeIntegration(const Config& config)
    : config_(config) {
    inference_fn_ = nullptr;
    THEMIS_INFO("LLMJudgeIntegration initialized without backend - setInferenceFunction() before use");
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
                if (!r.empty()) {
                  return r;
                }
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
        return makeUnavailableJudgeJson("llm_unavailable");
    }

    THEMIS_DEBUG("LLMJudgeIntegration::evaluateDimension dim={} prompt_len={}",
                 static_cast<int>(dimension),static_cast<int>(prompt.size()));

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
                if (!r.empty()) {
                  return r;
                }
            } catch (const std::exception& e) {
                THEMIS_WARN("LLM call failed (dim={}): {}", static_cast<int>(dimension), e.what());
            }
            return std::nullopt;
        },
        dim_retry_cfg);

    if (dim_result) {
      return *dim_result;
    }

    THEMIS_ERROR("LLM failed to respond for dimension {}", static_cast<int>(dimension));
    return makeUnavailableJudgeJson("llm_unavailable");
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
    if (!config_.enable_llm_judge) {
        throw std::runtime_error("llm_unavailable: THEMIS_ENABLE_LLM_JUDGE gate is disabled");
    }
    if (!inference_fn_) {
        std::string error_msg = "llm_unavailable: no inference backend configured. ";
        error_msg += "Provide ILLMBackend via constructor or setInferenceFunction().";
        THEMIS_ERROR("{}", error_msg);
        throw std::runtime_error(error_msg);
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

bool LLMJudgeIntegration::isMockMode() const {
    return false;
}

} // namespace themis::rag::judge
