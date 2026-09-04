/**
 * @file llm_integration.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 100/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#pragma once

#include <string>
#include <vector>
#include <functional>
#include <memory>
#include <unordered_map>

// Forward declarations
namespace themis::llm {
    class InferenceEngineEnhanced;
}

namespace themis::rag {

/**
 * @brief Token probability information from LLM generation
 */
struct TokenProbability {
    std::string token = {};
    double probability;
    size_t position;
};

/**
 * @brief Callback for receiving token probabilities during generation
 */
using TokenProbabilityCallback = std::function<void(const TokenProbability&)>;

/**
 * @brief LLM generation options for RAG components
 */
struct LLMGenerationOptions {
    double temperature = 0.7;
    size_t max_tokens = 512;
    bool stream = false;
    bool include_token_probabilities = false;
    TokenProbabilityCallback token_callback;
    
    // For self-consistency checks
    size_t num_samples = 1;
    std::vector<unsigned int> seeds;
};

/**
 * @brief Prompt template for LLM-based evaluation
 */
struct PromptTemplate {
    std::string system_prompt;
    std::string user_template;
    std::string few_shot_examples;
    std::string output_format_instruction;
    
    /**
     * @brief Format the prompt with given variables
     * @param variables Map of variable names to values
     * @return Formatted prompt string
     */
    std::string format(const std::unordered_map<std::string, std::string>& variables) const;
};

/**
 * @brief LLM response with parsed scores
 */
struct LLMEvaluationResponse {
    std::string raw_response;
    double score;
    double confidence;
    std::string explanation;
    std::vector<std::string> reasoning_steps;
    bool parse_successful;
};

/**
 * @brief LLM Integration Manager for RAG components
 * 
 * Provides a unified interface for RAG components to interact with the LLM inference engine.
 */
class LLMIntegration {
public:
    /**
     * @brief Set the inference engine to use
     * @param engine Shared pointer to the inference engine
     */
    static void setInferenceEngine(std::shared_ptr<llm::InferenceEngineEnhanced> engine);
    
    /**
     * @brief Get the current inference engine
     * @return Shared pointer to the inference engine (may be null)
     */
    static std::shared_ptr<llm::InferenceEngineEnhanced> getInferenceEngine();
    
    /**
     * @brief Generate text using the LLM
     * @param prompt The prompt to send to the LLM
     * @return Generated text
     */
    static std::string generate(const std::string& prompt);
    
    /**
     * @brief Generate text using the LLM with custom options
     * @param prompt The prompt to send to the LLM
     * @param options Generation options
     * @return Generated text
     */
    static std::string generate(
        const std::string& prompt,
        const LLMGenerationOptions& options
    );
    
    /**
     * @brief Generate multiple samples for self-consistency
     * @param prompt The prompt to send to the LLM
     * @param num_samples Number of samples to generate
     * @return Vector of generated texts
     */
    static std::vector<std::string> generateMultipleSamples(
        const std::string& prompt,
        size_t num_samples
    );
    
    /**
     * @brief Generate multiple samples with custom options
     * @param prompt The prompt to send to the LLM
     * @param num_samples Number of samples to generate
     * @param options Generation options
     * @return Vector of generated texts
     */
    static std::vector<std::string> generateMultipleSamples(
        const std::string& prompt,
        size_t num_samples,
        const LLMGenerationOptions& options
    );
    
    /**
     * @brief Parse evaluation response from LLM
     * @param response Raw LLM response
     * @return Parsed evaluation response
     */
    static LLMEvaluationResponse parseEvaluationResponse(const std::string& response);
    
    /**
     * @brief Calculate perplexity from token probabilities
     * @param token_probs Vector of token probabilities
     * @return Perplexity score
     */
    static double calculatePerplexity(const std::vector<double>& token_probs);
    
    /**
     * @brief Calculate semantic similarity between two texts
     * @param text1 First text
     * @param text2 Second text
     * @return Similarity score (0-1)
     */
    static double calculateSemanticSimilarity(
        const std::string& text1,
        const std::string& text2
    );
};

/**
 * @brief Prompt template library for common RAG evaluation tasks
 */
class PromptLibrary {
public:
    // Knowledge Gap Detector prompts
    static PromptTemplate getConfidenceEvaluationPrompt();
    static PromptTemplate getClaimVerificationPrompt();
    static PromptTemplate getConsistencyCheckPrompt();
    
    // RAG Judge prompts
    static PromptTemplate getFaithfulnessEvaluationPrompt();
    static PromptTemplate getRelevanceEvaluationPrompt();
    static PromptTemplate getCompletenessEvaluationPrompt();
    static PromptTemplate getCoherenceEvaluationPrompt();
    static PromptTemplate getPairwiseComparisonPrompt();
};

} // namespace themis::rag
