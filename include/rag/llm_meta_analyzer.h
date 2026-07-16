/**
 * @file llm_meta_analyzer.h
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
#include <memory>
#include <unordered_map>
#include <functional>

namespace themis::rag {

/**
 * @brief Base class for LLM-based meta-analysis
 * 
 * Provides shared infrastructure for:
 * - Ethics Judge (detect ethical implications)
 * - Quality Judge (evaluate RAG quality)
 * - Gap Detector (detect knowledge gaps)
 */
class LLMMetaAnalyzer {
public:
    /**
     * @brief Configuration for meta-analysis
     */
    struct AnalysisConfig {
        std::string judge_model = "default";
        bool use_chain_of_thought = true;
        int max_retries = 3;
        double min_confidence = 0.7;
        bool enable_caching = true;
        size_t cache_size = 1000;
    };

    /**
     * @brief Result of meta-analysis
     */
    struct AnalysisResult {
        bool detection_positive = false;
        double confidence = 0.0;
        std::string reasoning;
        std::unordered_map<std::string, double> dimension_scores;
        std::string raw_response;
    };

    /**
     * @brief Virtual destructor
     */
    virtual ~LLMMetaAnalyzer() = default;

    /**
     * @brief Load configuration
     * @param config Configuration to load
     */
    virtual void loadConfig(const AnalysisConfig& config);

    /**
     * @brief Get current configuration
     * @return Current configuration
     */
    virtual AnalysisConfig getConfig() const;

    /**
     * @brief Clear analysis cache
     */
    virtual void clearCache();

protected:
    /**
     * @brief Build prompt for LLM analysis
     * @param task_description Description of the analysis task
     * @param input_text Input text to analyze
     * @param criteria Evaluation criteria
     * @return Formatted prompt string
     */
    std::string buildPrompt(
        const std::string& task_description,
        const std::string& input_text,
        const std::vector<std::string>& criteria
    );

    /**
     * @brief Build prompt with chain-of-thought
     * @param task_description Description of the analysis task
     * @param input_text Input text to analyze
     * @param criteria Evaluation criteria
     * @return Formatted prompt with CoT instructions
     */
    std::string buildPromptWithCoT(
        const std::string& task_description,
        const std::string& input_text,
        const std::vector<std::string>& criteria
    );
    /**
     * @brief Build prompt with chain-of-thought
     * @param task_description Description of the analysis task
     * @param input_text Input text to analyze
     * @param criteria Evaluation criteria
     * @param examples Optional examples
     * @return Formatted prompt with CoT instructions
     */
    std::string buildPromptWithCoT(
        const std::string& task_description,
        const std::string& input_text,
        const std::vector<std::string>& criteria,
        const std::vector<std::string>& examples
    );

    /**
     * @brief Parse LLM response
     * @param llm_response Raw LLM response
     * @return Structured analysis result
     */
    AnalysisResult parseResponse(const std::string& llm_response);

    /**
     * @brief Parse score from LLM response
     * @param response Response text
     * @param dimension Optional dimension name
     * @return Parsed score (0.0-1.0)
     */
    double parseScore(const std::string& response, const std::string& dimension = "");

    /**
     * @brief Extract reasoning from LLM response
     * @param response Response text
     * @return Extracted reasoning
     */
    std::string extractReasoning(const std::string& response);

    /**
     * @brief Call LLM for inference
     * @param prompt Prompt to send to LLM
     * @return LLM response
     */
    virtual std::string callLLM(const std::string& prompt);

    /**
     * @brief Export metrics for monitoring
     * @param metrics Map to store metrics
     */
    virtual void exportMetrics(std::unordered_map<std::string, double>& metrics);

    /**
     * @brief Compute cache key for input
     * @param input Input string
     * @return Cache key
     */
    std::string computeCacheKey(const std::string& input);

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace themis::rag
