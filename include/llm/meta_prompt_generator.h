/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            meta_prompt_generator.h                            ║
  Version:         0.0.38                                             ║
  Last Modified:   2026-04-13 04:16:42                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     165                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/**
 * @file meta_prompt_generator.h
 * @brief Meta-prompt generation for prompt improvement
 * 
 * Generates prompts that improve other prompts using:
 * - Template-based generation
 * - Feedback incorporation
 * - Pattern learning from successful prompts
 */

#pragma once

#include <string>
#include <vector>
#include <nlohmann/json.hpp>

namespace themis {
namespace llm {

/**
 * @brief Configuration for meta-prompt generation
 */
struct MetaPromptConfig {
    std::string improvement_strategy = "iterative"; ///< Strategy: iterative, analytical, creative
    bool include_examples = true;      ///< Include few-shot examples in meta-prompt
    bool include_constraints = true;   ///< Include constraints and requirements
    size_t max_prompt_length = 2000;   ///< Maximum meta-prompt length
};

/**
 * @brief Meta-prompt generation result
 */
struct MetaPromptResult {
    std::string meta_prompt;           ///< Generated meta-prompt
    std::string improvement_suggestion; ///< Specific improvement suggestions
    std::vector<std::string> key_insights; ///< Key insights for improvement
    nlohmann::json metadata;           ///< Additional metadata
};

/**
 * @brief Meta-prompt generator class
 * 
 * Generates prompts for improving other prompts:
 * - Creates structured improvement instructions
 * - Incorporates feedback and performance data
 * - Suggests specific modifications
 * - Learns from successful prompt patterns
 */
class MetaPromptGenerator {
public:
    /**
     * @brief Constructor
     * @param config Meta-prompt configuration
     */
    explicit MetaPromptGenerator(const MetaPromptConfig& config = MetaPromptConfig{});
    
    /**
     * @brief Generate a meta-prompt for improving a prompt
     * @param original_prompt The prompt to improve
     * @param feedback Feedback about the prompt's performance
     * @param score Current performance score
     * @param task_description Description of the task
     * @return Meta-prompt result
     */
    MetaPromptResult generateImprovementPrompt(
        const std::string& original_prompt,
        const std::string& feedback,
        double score,
        const std::string& task_description = ""
    ) const;
    
    /**
     * @brief Generate a prompt for analyzing prompt quality
     * @param prompt Prompt to analyze
     * @param examples Example inputs/outputs
     * @return Analysis meta-prompt
     */
    std::string generateAnalysisPrompt(
        const std::string& prompt,
        const std::vector<std::pair<std::string, std::string>>& examples
    ) const;
    
    /**
     * @brief Generate suggestions for specific improvements
     * @param prompt Current prompt
     * @param weakness Identified weakness
     * @return Improvement suggestions
     */
    std::vector<std::string> generateImprovementSuggestions(
        const std::string& prompt,
        const std::string& weakness
    ) const;
    
    /**
     * @brief Extract patterns from successful prompts
     * @param successful_prompts List of high-performing prompts
     * @return Common patterns and best practices
     */
    std::vector<std::string> extractSuccessPatterns(
        const std::vector<std::pair<std::string, double>>& successful_prompts
    ) const;
    
    /**
     * @brief Get current configuration
     */
    const MetaPromptConfig& getConfig() const { return config_; }
    
    /**
     * @brief Update configuration
     */
    void setConfig(const MetaPromptConfig& config) { config_ = config; }

private:
    MetaPromptConfig config_;
    
    /**
     * @brief Build improvement instructions based on feedback
     */
    std::string buildImprovementInstructions(
        const std::string& feedback,
        double score
    ) const;
    
    /**
     * @brief Generate constraint clauses
     */
    std::string buildConstraints() const;
    
    /**
     * @brief Generate example section for meta-prompt
     */
    std::string buildExampleSection(
        const std::string& original_prompt
    ) const;
    
    /**
     * @brief Analyze prompt structure
     */
    nlohmann::json analyzePromptStructure(const std::string& prompt) const;
};

} // namespace llm
} // namespace themis
