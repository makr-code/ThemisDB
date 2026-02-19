/**
 * @file llm_judge_client.h
 * @brief LLM Judge Client - Connects RAG Judge to InferenceEngineEnhanced
 * 
 * Bridges the RAG Judge evaluation system with the LLM inference engine,
 * providing structured evaluation responses with scores, confidence, and reasoning.
 */

#pragma once

#include "rag/rag_judge.h"
#include "llm/inference_engine_enhanced.h"
#include <string>
#include <memory>
#include <unordered_map>

namespace themis::rag::judge {

/**
 * @brief Response from LLM judge evaluation
 */
struct LLMJudgeResponse {
    double score;                    ///< Normalized score (0-1)
    double confidence;               ///< Confidence in the score (0-1)
    std::string reasoning;           ///< Chain-of-thought explanation
    std::unordered_map<std::string, double> dimension_scores;  ///< Sub-dimension scores
    bool success;                    ///< Whether evaluation succeeded
    std::string error_message;       ///< Error message if failed
    std::chrono::milliseconds latency;  ///< Inference latency
};

/**
 * @brief LLM Judge Client
 * 
 * Manages communication with InferenceEngineEnhanced for judge evaluations.
 * Handles prompt construction, response parsing, and error recovery.
 */
class LLMJudgeClient {
public:
    /**
     * @brief Configuration for LLM Judge Client
     */
    struct Config {
        std::string model_id = "default";
        double temperature = 0.3;       ///< Low temp for consistent evaluation
        int max_tokens = 1024;
        int max_retries = 3;
        int timeout_ms = 10000;
        bool enable_caching = true;
        bool extract_token_probs = false;  ///< For G-Eval integration
    };
    
    /**
     * @brief Construct client with configuration
     * @param config Client configuration
     * @param inference_engine Shared inference engine
     */
    LLMJudgeClient(
        const Config& config,
        std::shared_ptr<llm::InferenceEngineEnhanced> inference_engine
    );
    
    /**
     * @brief Destructor
     */
    ~LLMJudgeClient();
    
    /**
     * @brief Evaluate using LLM judge
     * @param prompt Evaluation prompt
     * @param dimension Dimension being evaluated
     * @return LLM judge response with score and reasoning
     */
    LLMJudgeResponse evaluate(
        const std::string& prompt,
        EvaluationDimension dimension
    );
    
    /**
     * @brief Evaluate with custom inference request
     * @param prompt Evaluation prompt
     * @param dimension Dimension being evaluated
     * @param request_config Custom inference request configuration
     * @return LLM judge response
     */
    LLMJudgeResponse evaluateWithConfig(
        const std::string& prompt,
        EvaluationDimension dimension,
        const llm::InferenceEngineEnhanced::EnhancedInferenceRequest& request_config
    );
    
    /**
     * @brief Parse LLM response into structured evaluation
     * @param response Raw LLM response text
     * @param dimension Dimension being evaluated
     * @return Parsed judge response
     */
    static LLMJudgeResponse parseResponse(
        const std::string& response,
        EvaluationDimension dimension
    );
    
    /**
     * @brief Extract score from response text
     * @param response Response text
     * @return Normalized score (0-1), or -1 if parsing failed
     */
    static double extractScore(const std::string& response);
    
    /**
     * @brief Extract reasoning from response text
     * @param response Response text
     * @return Reasoning text
     */
    static std::string extractReasoning(const std::string& response);
    
    /**
     * @brief Extract confidence from response or compute from token probabilities
     * @param response Response text
     * @param token_probs Optional token probabilities
     * @return Confidence score (0-1)
     */
    static double extractConfidence(
        const std::string& response,
        const std::vector<double>& token_probs = {}
    );
    
    /**
     * @brief Get current configuration
     */
    Config getConfig() const;
    
    /**
     * @brief Update configuration
     */
    void setConfig(const Config& config);

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
    
    // Internal helpers
    std::string formatPromptForInference(
        const std::string& prompt,
        EvaluationDimension dimension
    );
    
    LLMJudgeResponse handleInferenceError(
        const std::string& error_msg,
        EvaluationDimension dimension
    );
};

} // namespace themis::rag::judge
