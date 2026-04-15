/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            prompt_templates.h                                 ║
  Version:         0.0.43                                             ║
  Last Modified:   2026-04-15 04:12:47                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     143                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/**
 * @file prompt_templates.h
 * @brief Prompt template management for RAG Judge evaluations
 */

#pragma once

#include "rag/rag_judge.h"
#include <string>
#include <unordered_map>
#include <vector>

namespace themis::rag::judge {

/**
 * @brief Few-shot example for prompt templates
 */
struct FewShotExample {
    std::string query;
    std::string context;
    std::string answer;
    double score;
    std::string explanation;
};

/**
 * @brief Prompt template manager
 * 
 * Manages prompt templates for each evaluation dimension with:
 * - Chain-of-thought instructions
 * - Few-shot examples
 * - Output format specifications
 */
class PromptTemplateManager {
public:
    PromptTemplateManager();
    
    /**
     * @brief Load custom templates from directory
     * @param template_dir Directory containing template files
     * @return true if loaded successfully
     */
    bool loadTemplatesFromDirectory(const std::string& template_dir);
    
    /**
     * @brief Load specific template from file
     * @param dimension Evaluation dimension
     * @param filepath Path to template file
     * @return true if loaded successfully
     */
    bool loadTemplate(EvaluationDimension dimension, const std::string& filepath);
    
    /**
     * @brief Generate evaluation prompt for given dimension
     * @param dimension Evaluation dimension
     * @param input Evaluation input
     * @return Formatted prompt string
     */
    std::string generatePrompt(
        EvaluationDimension dimension,
        const EvaluationInput& input
    ) const;
    
    /**
     * @brief Set few-shot examples for dimension
     * @param dimension Evaluation dimension
     * @param examples Vector of few-shot examples
     */
    void setFewShotExamples(
        EvaluationDimension dimension,
        const std::vector<FewShotExample>& examples
    );
    
    /**
     * @brief Get template for dimension
     * @param dimension Evaluation dimension
     * @return Template string
     */
    std::string getTemplate(EvaluationDimension dimension) const;
    
    /**
     * @brief Set custom template for dimension
     * @param dimension Evaluation dimension
     * @param template_str Template string with placeholders
     */
    void setTemplate(EvaluationDimension dimension, const std::string& template_str);
    
    /**
     * @brief Get default templates
     */
    static PromptTemplateManager createDefault();

private:
    std::unordered_map<EvaluationDimension, std::string> templates_;
    std::unordered_map<EvaluationDimension, std::vector<FewShotExample>> few_shot_examples_;
    
    // Default template strings
    static std::string getFaithfulnessTemplate();
    static std::string getRelevanceTemplate();
    static std::string getCompletenessTemplate();
    static std::string getCoherenceTemplate();
    
    // Default few-shot examples
    static std::vector<FewShotExample> getFaithfulnessExamples();
    static std::vector<FewShotExample> getRelevanceExamples();
    static std::vector<FewShotExample> getCompletenessExamples();
    static std::vector<FewShotExample> getCoherenceExamples();
    
    // Helper to replace placeholders in template
    std::string replacePlaceholders(
        const std::string& template_str,
        const EvaluationInput& input
    ) const;
    
    // Format few-shot examples for prompt
    std::string formatFewShotExamples(
        const std::vector<FewShotExample>& examples
    ) const;
};

} // namespace themis::rag::judge
