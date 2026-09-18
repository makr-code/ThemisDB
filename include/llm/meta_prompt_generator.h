/**
 * @file meta_prompt_generator.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 100/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#pragma once

#include <string>
#include <vector>
#include <nlohmann/json.hpp>

namespace themis {
namespace llm {

struct MetaPromptConfig {
    std::string improvement_strategy = "iterative"; ///< Strategy: iterative, analytical, creative
    bool include_examples = true;      ///< Include few-shot examples in meta-prompt
    bool include_constraints = true;   ///< Include constraints and requirements
    size_t max_prompt_length = 2000;   ///< Maximum meta-prompt length
};

struct MetaPromptResult {
    std::string meta_prompt;           ///< Generated meta-prompt
    std::string improvement_suggestion; ///< Specific improvement suggestions
    std::vector<std::string> key_insights; ///< Key insights for improvement
    nlohmann::json metadata;           ///< Additional metadata
};

class MetaPromptGenerator {
public:
    explicit MetaPromptGenerator(const MetaPromptConfig& config = MetaPromptConfig{});
    
    MetaPromptResult generateImprovementPrompt(
        const std::string& original_prompt,
        const std::string& feedback,
        double score,
        const std::string& task_description = ""
    ) const;
    
    std::string generateAnalysisPrompt(
        const std::string& prompt,
        const std::vector<std::pair<std::string, std::string>>& examples
    ) const;
    
    /**
     * @brief Generate Improvement Suggestions.
     * @param[in] prompt Input parameter.
     * @param[in] weakness Input parameter.
     * @return Return value.
     */
    std::vector<std::string> generateImprovementSuggestions(
        const std::string& prompt,
        const std::string& weakness
    ) const;
    
    std::vector<std::string> extractSuccessPatterns(
        const std::vector<std::pair<std::string, double>>& successful_prompts
    ) const;
    
    const MetaPromptConfig& getConfig() const { return config_; }
    
    /**
     * @brief Set Config.
     * @param[in] config Input parameter.
     * @details Implements setConfig without additional internal calls.
     */
    void setConfig(const MetaPromptConfig& config) { config_ = config; }

private:
    MetaPromptConfig config_;
    
    /**
     * @brief Build Improvement Instructions.
     * @param[in] feedback Input parameter.
     * @param[in] score Input parameter.
     * @return Return value.
     */
    std::string buildImprovementInstructions(
        const std::string& feedback,
        double score
    ) const;
    
    /**
     * @brief Build Constraints.
     * @return Return value.
     */
    std::string buildConstraints() const;
    
    /**
     * @brief Build Example Section.
     * @param[in] original_prompt Input parameter.
     * @return Return value.
     */
    std::string buildExampleSection(
        const std::string& original_prompt
    ) const;
    
    /**
     * @brief Analyze Prompt Structure.
     * @param[in] prompt Input parameter.
     * @return Return value.
     */
    nlohmann::json analyzePromptStructure(const std::string& prompt) const;
};

} // namespace llm
} // namespace themis
