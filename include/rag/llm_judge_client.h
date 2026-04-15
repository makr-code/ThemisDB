/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            llm_judge_client.h                                 ║
  Version:         0.0.45                                             ║
  Last Modified:   2026-04-15 07:08:46                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     183                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/**
 * @file llm_judge_client.h
 * @brief LLM Judge Client - Connects prompts to InferenceEngineEnhanced
 * 
 * This client bridges RAG Judge evaluations to the LLM inference engine,
 * enabling automated evaluation with proper caching and batching.
 */

#pragma once

#include <string>
#include <vector>
#include <memory>
#include <chrono>

// Forward declarations
namespace themis::llm {
    class InferenceEngineEnhanced;
    class ILLMPlugin;
}

namespace themis::rag::judge {

/**
 * @brief Response from an LLM evaluation
 */
struct EvaluationResponse {
    double score = 0.0;               ///< Evaluation score (0-1)
    std::string reasoning;             ///< Explanation for the score
    double confidence = 0.0;           ///< Confidence in the evaluation (0-1)
    std::string raw_response;          ///< Raw LLM response
    std::chrono::milliseconds evaluation_time; ///< Time taken for evaluation
};

/**
 * @brief LLM Judge Client
 * 
 * Connects RAG Judge evaluation prompts to InferenceEngineEnhanced for
 * efficient LLM-based evaluation with caching and batching support.
 */
class LLMJudgeClient {
public:
    /**
     * @brief Configuration for LLM Judge Client
     */
    struct Config {
        std::string model_name = "default";
        double temperature = 0.3;       ///< Low temp for consistent evaluation
        int max_tokens = 1024;
        int timeout_ms = 30000;
        int priority = 0;               ///< Request priority
        
        // Caching and batching
        bool enable_caching = true;
        bool enable_batching = true;
        size_t batch_size = 8;
        size_t batch_timeout_ms = 100;
        
        // Stop sequences for evaluation
        std::vector<std::string> stop_sequences = {"\n\n", "---"};
    };
    
    /**
     * @brief Construct client with default config
     */
    LLMJudgeClient();
    
    /**
     * @brief Construct client with custom config
     * @param config Client configuration
     */
    explicit LLMJudgeClient(const Config& config);
    
    /**
     * @brief Destructor
     */
    ~LLMJudgeClient();
    
    /**
     * @brief Evaluate a single prompt
     * @param prompt Evaluation prompt
     * @return LLM response text
     */
    std::string evaluate(const std::string& prompt);
    
    /**
     * @brief Evaluate multiple prompts in batch
     * @param prompts Vector of evaluation prompts
     * @return Vector of LLM responses
     */
    std::vector<std::string> evaluateBatch(
        const std::vector<std::string>& prompts
    );
    
    /**
     * @brief Evaluate a specific dimension for RAG
     * @param query Original user query
     * @param answer Generated answer
     * @param documents Retrieved documents (id, content pairs)
     * @param dimension Dimension name (e.g., "faithfulness", "relevance")
     * @return Structured evaluation response
     */
    EvaluationResponse evaluateDimension(
        const std::string& query,
        const std::string& answer,
        const std::vector<std::pair<std::string, std::string>>& documents,
        const std::string& dimension
    );
    
    /**
     * @brief Set custom inference engine
     * @param engine Shared pointer to inference engine
     */
    void setInferenceEngine(
        std::shared_ptr<llm::InferenceEngineEnhanced> engine
    );
    
    /**
     * @brief Register a model for inference
     * @param model_id Model identifier
     * @param plugin LLM plugin implementation
     */
    void registerModel(
        const std::string& model_id,
        std::shared_ptr<llm::ILLMPlugin> plugin
    );
    
    /**
     * @brief Get current configuration
     * @return Current config
     */
    Config getConfig() const;
    
    /**
     * @brief Update configuration
     * @param config New configuration
     */
    void setConfig(const Config& config);

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
    
    /**
     * @brief Generate unique request ID
     * @return Request ID string
     */
    std::string generateRequestId();
    
    /**
     * @brief Parse JSON-like evaluation response
     * @param response Raw LLM response
     * @param parsed Parsed output structure
     */
    void parseEvaluationResponse(
        const std::string& response,
        EvaluationResponse& parsed
    );
};

} // namespace themis::rag::judge
