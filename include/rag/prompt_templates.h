/**
 * @file prompt_templates.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 100/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
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
     * @return Return value.
     */
    static PromptTemplateManager createDefault();

private:
    std::unordered_map<EvaluationDimension, std::string> templates_;
    std::unordered_map<EvaluationDimension, std::vector<FewShotExample>> few_shot_examples_;
    
    /**
     * @brief Default template strings
     * @return Return value.
     */
    static std::string getFaithfulnessTemplate();
    /**
     * @brief TBD: Describe getRelevanceTemplate.
     * @return Return value.
     */
    static std::string getRelevanceTemplate();
    /**
     * @brief TBD: Describe getCompletenessTemplate.
     * @return Return value.
     */
    static std::string getCompletenessTemplate();
    /**
     * @brief TBD: Describe getCoherenceTemplate.
     * @return Return value.
     */
    static std::string getCoherenceTemplate();
    
    /**
     * @brief Default few-shot examples
     * @return Return value.
     */
    static std::vector<FewShotExample> getFaithfulnessExamples();
    /**
     * @brief TBD: Describe getRelevanceExamples.
     * @return Return value.
     */
    static std::vector<FewShotExample> getRelevanceExamples();
    /**
     * @brief TBD: Describe getCompletenessExamples.
     * @return Return value.
     */
    static std::vector<FewShotExample> getCompletenessExamples();
    /**
     * @brief TBD: Describe getCoherenceExamples.
     * @return Return value.
     */
    static std::vector<FewShotExample> getCoherenceExamples();
    
    /**
     * @brief Helper to replace placeholders in template
     * @param[in] template_str Input parameter.
     * @param[in] input Input parameter.
     * @return Return value.
     */
    std::string replacePlaceholders(
        const std::string& template_str,
        const EvaluationInput& input
    ) const;
    
    /**
     * @brief Format few-shot examples for prompt
     * @param[in] examples Input parameter.
     * @return Return value.
     */
    std::string formatFewShotExamples(
        const std::vector<FewShotExample>& examples
    ) const;
};

} // namespace themis::rag::judge
