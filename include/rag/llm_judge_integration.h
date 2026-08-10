/**
 * @file llm_judge_integration.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 89/100
 * @note Gap Summary: total=13; TODO=1, Stub=3, Unimpl=0, Mock=9, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#pragma once

#include "rag/rag_judge.h"
#include "rag/prompt_templates.h"
#include "rag/response_parser.h"
#include <string>
#include <memory>
#include <functional>

namespace themis::rag::judge {

/**
 * @brief Abstract inference engine interface for dependency injection
 *
 * Implement this interface to provide a real or test LLM backend to
 * LLMJudgeIntegration via the engine-injection constructor.
 *
 * Example (test double returning random scores):
 * @code
 *   struct RandomEngine : ILLMInferenceEngine {
 *       std::string generate(const std::string&) override {
 *           return R"({"score":)" + std::to_string(dist_(rng_)) + R"(,"confidence":0.8})";
 *       }
 *   private:
 *       std::mt19937 rng_{std::random_device{}()};
 *       std::uniform_real_distribution<double> dist_{1.0, 5.0};
 *   };
 * @endcode
 */
struct ILLMInferenceEngine {
    virtual ~ILLMInferenceEngine() = default;
    /**
     * @brief Generate a response for the given prompt.
     * @param prompt The full prompt text.
     * @return Generated response text.
     */
    [[nodiscard]] virtual std::string generate(const std::string& prompt) = 0;
};

/// Alias used by roadmap/acceptance wording for the judge backend adapter.
using ILLMBackend = ILLMInferenceEngine;

/**
 * @brief LLM integration wrapper for judge evaluations
 * 
 * Handles communication with LLM inference engine, including:
 * - Prompt generation and submission
 * - Response retrieval and parsing
 * - Error handling and retries
 * 
 * Usage Example (Production with injected engine):
 * @code
 *   struct MyEngine : ILLMInferenceEngine {
 *       std::string generate(const std::string& prompt) override {
 *           return myBackend.infer(prompt);
 *       }
 *   };
 *   MyEngine engine;
 *   LLMJudgeIntegration integration(&engine);
 * @endcode
 *
 * Usage Example (Production with setInferenceFunction):
 * @code
 *   LLMJudgeIntegration::Config config;
 *   config.use_mock_mode = false;
 *   LLMJudgeIntegration integration(config);
 *   integration.setInferenceFunction([](const std::string& prompt) {
 *       return myCustomLLMBackend.infer(prompt);
 *   });
 * @endcode
 * 
 * Usage Example (Testing with Mock):
 * @code
 *   LLMJudgeIntegration::Config config;
 *   config.use_mock_mode = true;
 *   LLMJudgeIntegration integration(config);
 *   // No inference function needed - will use mock responses
 * @endcode
 */
class LLMJudgeIntegration {
public:
    /**
     * @brief Configuration for LLM integration
     */
    struct Config {
#if defined(THEMIS_ENABLE_LLM_JUDGE)
        static constexpr bool kDefaultJudgeEnabled = true;
#else
        static constexpr bool kDefaultJudgeEnabled = false;
#endif

        std::string model_name = "default";
        double temperature = 0.3;
        int max_tokens = 1024;
        int max_retries = 3;
        int timeout_ms = 30000;
        bool use_json_mode = true;
        bool enable_llm_judge = kDefaultJudgeEnabled; // gate for real backend dispatch
        
        // Mock mode configuration
        bool use_mock_mode = false;           // Enable mock responses (for testing only)
        bool warn_on_mock_mode = true;        // Log warning once when mock mode is used
        bool allow_mock = false;              // Allow nullptr engine (tests only, requires use_mock_mode=true)
    };
    
    /**
     * @brief Construct with explicit inference engine (production path)
     *
     * @param engine Pointer to an ILLMInferenceEngine implementation.
     *               Must not be nullptr unless config.allow_mock is true.
     * @throws std::invalid_argument if engine is nullptr and allow_mock is false.
     */
    explicit LLMJudgeIntegration(ILLMInferenceEngine* engine);
    
    /**
     * @brief Construct with inference engine and configuration
     *
     * @param engine Pointer to an ILLMInferenceEngine implementation.
     * @param config Integration configuration.
     * @throws std::invalid_argument if engine is nullptr and config.allow_mock is false.
     */
    explicit LLMJudgeIntegration(ILLMInferenceEngine* engine, const Config& config);

    /**
     * @brief Construct with default configuration
     */
    LLMJudgeIntegration();
    
    /**
     * @brief Construct with custom configuration
     * @param config Integration configuration
     */
    explicit LLMJudgeIntegration(const Config& config);
    
    /**
     * @brief Evaluate using LLM
     * @param dimension Evaluation dimension
     * @param input Evaluation input
     * @param template_mgr Prompt template manager
     * @return Parsed evaluation response
     */
    ParsedResponse evaluateWithLLM(
        EvaluationDimension dimension,
        const EvaluationInput& input,
        const PromptTemplateManager& template_mgr
    );

    /**
     * @brief Directly evaluate a prompt for a dimension.
     *        Lightweight helper used by evaluator components.
     */
    std::string evaluateDimension(
        const std::string& prompt,
        EvaluationDimension dimension
    );
    
    /**
     * @brief Set LLM inference function (for dependency injection/testing)
     * @param fn Function that takes prompt and returns LLM response
     */
    void setInferenceFunction(
        std::function<std::string(const std::string&)> fn
    );
    
    /**
     * @brief Update configuration
     * @param config New configuration
     */
    void setConfig(const Config& config);
    
    /**
     * @brief Get current configuration
     * @return Current configuration
     */
    Config getConfig() const;
    
    /**
     * @brief Check if currently in mock mode
     * @return true if using mock responses
     */
    bool isMockMode() const;

private:
    Config config_;
    std::function<std::string(const std::string&)> inference_fn_;
    bool mock_mode_active_ = false;        // Tracks whether stub inference is currently active
    bool mock_mode_warning_shown_ = false;  // Track if warning has been shown
    
    /**
     * @brief Call LLM inference with retries
     * @param prompt Input prompt
     * @return LLM response
     */
    std::string callLLM(const std::string& prompt);
    
    /**
     * @brief Default inference function (stub)
     * @param prompt Input prompt
     * @return Mock response
     */
    static std::string defaultInference(const std::string& prompt);
};

} // namespace themis::rag::judge
