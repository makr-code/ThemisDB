/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            llm_judge_integration.h                            ║
  Version:         0.0.46                                             ║
  Last Modified:   2026-04-15 18:04:48                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   86.0/100                                       ║
    • Total Lines:     211                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 67965456c8  2026-03-22  Add constructors with default config for various classes ... ║
    • 883e2e12b5  2026-03-15  feat(rag): replace LLMIntegration stub + add ILLMInferenc... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/**
 * @file llm_judge_integration.h
 * @brief LLM integration wrapper for RAG Judge
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
    virtual std::string generate(const std::string& prompt) = 0;
};

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
        std::string model_name = "default";
        double temperature = 0.3;
        int max_tokens = 1024;
        int max_retries = 3;
        int timeout_ms = 30000;
        bool use_json_mode = true;
        
        // Mock mode configuration
        bool use_mock_mode = false;           // Enable mock responses (for testing only)
        bool warn_on_mock_mode = true;        // Log warning once when mock mode is used
        bool allow_mock = false;              // Allow nullptr engine (opt-in for tests; default false = fail fast in production)
    };
    
    /**
     * @brief Construct with an explicit inference engine (production path).
     *
     * @param engine Pointer to an ILLMInferenceEngine implementation.
     *               Must not be nullptr unless @p config.allow_mock is true.
     * @param config Integration configuration.
     * @throws std::invalid_argument if @p engine is nullptr and
     *         @p config.allow_mock is false.
     */
    explicit LLMJudgeIntegration(ILLMInferenceEngine* engine);
    explicit LLMJudgeIntegration(ILLMInferenceEngine* engine, const Config& config);

    /**
     * @brief Construct LLM integration
     * @param config Integration configuration
     */
    LLMJudgeIntegration();
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
