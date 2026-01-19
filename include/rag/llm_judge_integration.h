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
    };
    
    /**
     * @brief Construct LLM integration
     * @param config Integration configuration
     */
    explicit LLMJudgeIntegration(const Config& config = {});
    
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

private:
    Config config_;
    std::function<std::string(const std::string&)> inference_fn_;
    
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
