/**
 * @file llm_judge_client.h
 * @brief Client wrapper for InferenceEngineEnhanced integration with RAG Judge
 * 
 * Provides a clean interface for RAG Judge components to interact with
 * InferenceEngineEnhanced, handling lifecycle management, request formatting,
 * and response parsing.
 */

#pragma once

#include <string>
#include <memory>
#include <functional>
#include <vector>

// Forward declarations
namespace themis::llm {
    class InferenceEngineEnhanced;
    struct InferenceResponse;
}

namespace themis::rag::judge {

/**
 * @brief Token probability information from LLM generation
 */
struct TokenProbInfo {
    int token_id;
    std::string token_text;
    double probability;
    double log_probability;
};

/**
 * @brief LLM Judge client response with extended information
 */
struct LLMJudgeResponse {
    std::string text;                          ///< Generated text
    std::vector<TokenProbInfo> token_probs;    ///< Token probabilities
    double generation_time_ms;                 ///< Generation time
    bool from_cache;                           ///< Whether response was cached
    std::string model_id;                      ///< Model used
};

/**
 * @brief Client wrapper for InferenceEngineEnhanced
 * 
 * Manages the lifecycle and interaction with InferenceEngineEnhanced for
 * RAG Judge components. Provides methods for:
 * - Standard text generation
 * - Token probability extraction (for G-Eval)
 * - Batch processing
 * - Cache management
 * 
 * Thread-safe and optimized for <500ms evaluation targets.
 */
class LLMJudgeClient {
public:
    /**
     * @brief Configuration for LLM Judge client
     */
    struct Config {
        // Inference settings
        double temperature = 0.3;              ///< Low temp for consistent evaluation
        size_t max_tokens = 1024;              ///< Max tokens per generation
        bool use_json_mode = true;             ///< Enable JSON output formatting
        
        // Performance settings
        bool enable_caching = true;            ///< Enable context caching
        size_t request_timeout_ms = 30000;     ///< Request timeout (30s)
        int priority = 0;                      ///< Request priority
        
        // Token probability settings
        bool extract_token_probs = false;      ///< Extract token probabilities
        size_t top_k_tokens = 5;               ///< Number of top tokens to track
        
        // Model preference
        std::string preferred_model_id;        ///< Preferred model (empty = auto)
    };

    /**
     * @brief Construct client with shared engine
     * @param engine Shared pointer to InferenceEngineEnhanced
     */
    explicit LLMJudgeClient(std::shared_ptr<llm::InferenceEngineEnhanced> engine);
    
    /**
     * @brief Construct client with config (creates engine internally)
     */
    explicit LLMJudgeClient(const Config& config = Config{});
    
    /**
     * @brief Destructor
     */
    ~LLMJudgeClient();
    
    /**
     * @brief Generate text using LLM
     * @param prompt Input prompt
     * @param config Optional per-request config override
     * @return LLM response with metadata
     */
    LLMJudgeResponse generate(const std::string& prompt);
    LLMJudgeResponse generate(const std::string& prompt, const Config& config);
    
    /**
     * @brief Generate multiple samples for self-consistency
     * @param prompt Input prompt
     * @param num_samples Number of samples
     * @param config Optional config override
     * @return Vector of responses
     */
    std::vector<LLMJudgeResponse> generateMultiple(
        const std::string& prompt,
        size_t num_samples,
        const Config& config = Config{}
    );
    
    /**
     * @brief Extract token probabilities for specific tokens
     * 
     * Used by G-Eval to extract probabilities for score tokens (1-5).
     * 
     * @param prompt Evaluation prompt
     * @param target_tokens Target token texts (e.g., ["1", "2", "3", "4", "5"])
     * @param config Optional config override
     * @return Probability distribution over target tokens
     */
    std::vector<double> extractTokenProbabilities(
        const std::string& prompt,
        const std::vector<std::string>& target_tokens,
        const Config& config = Config{}
    );
    
    /**
     * @brief Set the inference engine
     * @param engine Shared pointer to engine
     */
    void setEngine(std::shared_ptr<llm::InferenceEngineEnhanced> engine);
    
    /**
     * @brief Get the inference engine
     * @return Shared pointer to engine (may be null)
     */
    std::shared_ptr<llm::InferenceEngineEnhanced> getEngine() const;
    
    /**
     * @brief Check if client is ready (has engine)
     * @return true if engine is available
     */
    bool isReady() const;
    
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
     * @brief Clear inference cache
     */
    void clearCache();
    
    /**
     * @brief Prewarm cache with common prompts
     * @param prompts Common evaluation prompts
     */
    void prewarmCache(const std::vector<std::string>& prompts);

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace themis::rag::judge
