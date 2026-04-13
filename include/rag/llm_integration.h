/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            llm_integration.h                                  ║
  Version:         0.0.38                                             ║
  Last Modified:   2026-04-13 04:18:59                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     195                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/**
 * @file llm_integration.h
 * @brief LLM Integration utilities for RAG components
 * 
 * Provides common interfaces for integrating RAG components with the LLM inference engine.
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
    std::string token;
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
     * @param options Generation options
     * @return Generated text
     */
    static std::string generate(const std::string& prompt);
    static std::string generate(
        const std::string& prompt,
        const LLMGenerationOptions& options
    );
    
    /**
     * @brief Generate multiple samples for self-consistency
     * @param prompt The prompt to send to the LLM
     * @param num_samples Number of samples to generate
     * @param options Generation options
     * @return Vector of generated texts
     */
    static std::vector<std::string> generateMultipleSamples(
        const std::string& prompt,
        size_t num_samples
    );
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
