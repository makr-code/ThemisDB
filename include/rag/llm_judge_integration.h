/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            llm_judge_integration.h                            ║
  Version:         0.0.20                                             ║
  Last Modified:   2026-02-21 19:14:27                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   81.0/100                                       ║
    • Total Lines:     170                                            ║
    • Open Issues:     TODOs: 0, Stubs: 2                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 468bda607  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 189cdf5b1  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • a5676b06f  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 56752fde6  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • c3f305f42  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
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
 * @brief LLM integration wrapper for judge evaluations
 * 
 * Handles communication with LLM inference engine, including:
 * - Prompt generation and submission
 * - Response retrieval and parsing
 * - Error handling and retries
 * 
 * Usage Example (Production with EmbeddedLLM):
 * @code
 *   LLMJudgeIntegration::Config config;
 *   config.use_mock_mode = false;
 *   LLMJudgeIntegration integration(config);
 *   
 *   // Option 1: Use EmbeddedLLM directly
 *   auto& llm = EmbeddedLLMManager::instance().get();
 *   integration.setInferenceFunction([&llm](const std::string& prompt) {
 *       return llm.generate(prompt, 1024);
 *   });
 * 
 *   // Option 2: Use custom backend
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
    };
    
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
