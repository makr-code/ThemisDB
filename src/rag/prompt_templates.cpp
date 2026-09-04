/**
 * @file prompt_templates.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 100/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=2, H=2, M=1, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#include "rag/prompt_templates.h"
#include "utils/logger.h"
#include <fstream>
#include <sstream>
#include <regex>

namespace themis::rag::judge {

PromptTemplateManager::PromptTemplateManager() {
    // Initialize with default templates
    templates_[EvaluationDimension::FAITHFULNESS] = getFaithfulnessTemplate();
    templates_[EvaluationDimension::RELEVANCE] = getRelevanceTemplate();
    templates_[EvaluationDimension::COMPLETENESS] = getCompletenessTemplate();
    templates_[EvaluationDimension::COHERENCE] = getCoherenceTemplate();
    
    // Initialize with default few-shot examples
    few_shot_examples_[EvaluationDimension::FAITHFULNESS] = getFaithfulnessExamples();
    few_shot_examples_[EvaluationDimension::RELEVANCE] = getRelevanceExamples();
    few_shot_examples_[EvaluationDimension::COMPLETENESS] = getCompletenessExamples();
    few_shot_examples_[EvaluationDimension::COHERENCE] = getCoherenceExamples();
}

bool PromptTemplateManager::loadTemplatesFromDirectory(const std::string& template_dir) {
    THEMIS_INFO("Loading prompt templates from directory: {}", template_dir);
    
    // Load templates for each dimension
    std::vector<std::pair<EvaluationDimension, std::string>> files = {
        {EvaluationDimension::FAITHFULNESS, template_dir + "/faithfulness.txt"},
        {EvaluationDimension::RELEVANCE, template_dir + "/relevance.txt"},
        {EvaluationDimension::COMPLETENESS, template_dir + "/completeness.txt"},
        {EvaluationDimension::COHERENCE, template_dir + "/coherence.txt"}
    };
    
    bool success = true;
    for (const auto& pair : files) {
        const EvaluationDimension dim = pair.first;
        const std::string& file = pair.second;
        
        if (!loadTemplate(dim, file)) {
            THEMIS_WARN("Failed to load template: {}", file);
            success = false;
        }
    }
    
    return success;
}

bool PromptTemplateManager::loadTemplate(EvaluationDimension dimension, const std::string& filepath) {
    std::ifstream file(filepath);
    if (!file.is_open()) {
        THEMIS_ERROR("Failed to open template file: {}", filepath);
        return false;
    }
    
    std::stringstream buffer = {};
    buffer << file.rdbuf();
    templates_[dimension] = buffer.str();
    
    THEMIS_DEBUG("Loaded template for dimension {} from {}", 
                 static_cast<int>(dimension), filepath);
    return true;
}

std::string PromptTemplateManager::generatePrompt(
    EvaluationDimension dimension,
    const EvaluationInput& input
) const {
    auto it = templates_.find(dimension);
    if (it == templates_.end()) {
        THEMIS_ERROR("No template found for dimension {}", static_cast<int>(dimension));
        return "";
    }
    
    std::string prompt = it->second;
    
    // Add few-shot examples if available
    auto examples_it = few_shot_examples_.find(dimension);
    if (examples_it != few_shot_examples_.end() && !examples_it->second.empty()) {
        std::string examples = formatFewShotExamples(examples_it->second);
        // Insert examples before the main task
        prompt = prompt + "\n\n" + examples + "\n\n";
    }
    
    // Replace placeholders
    prompt = replacePlaceholders(prompt, input);
    
    return prompt;
}

void PromptTemplateManager::setFewShotExamples(
    EvaluationDimension dimension,
    const std::vector<FewShotExample>& examples
) {
    few_shot_examples_[dimension] = examples;
    THEMIS_DEBUG("Set {} few-shot examples for dimension {}", 
                 examples.size(), static_cast<int>(dimension));
}

std::string PromptTemplateManager::getTemplate(EvaluationDimension dimension) const {
    auto it = templates_.find(dimension);
    if (it != templates_.end()) {
        return it->second;
    }
    return "";
}

void PromptTemplateManager::setTemplate(EvaluationDimension dimension, const std::string& template_str) {
    templates_[dimension] = template_str;
}

PromptTemplateManager PromptTemplateManager::createDefault() {
    return PromptTemplateManager();
}

// Default template implementations

std::string PromptTemplateManager::getFaithfulnessTemplate() {
    return R"(You are an expert evaluator assessing the faithfulness of a generated answer to its source documents.

**Task:** Evaluate if the answer is fully supported by the provided context documents.

**Instructions:**
1. Identify all factual claims in the answer
2. For each claim, check if it can be verified from the context
3. Rate faithfulness on a scale of 1-5:
   - 5: All claims fully supported by context
   - 4: Most claims supported, minor unverifiable details
   - 3: Mix of verifiable and unverifiable claims
   - 2: Significant unverifiable content
   - 1: Mostly contradictory or unsupported

**Context Documents:**
{context}

**Generated Answer:**
{answer}

**Provide your evaluation in JSON format:**
{
  "score": <1-5>,
  "confidence": <0.0-1.0>,
  "reasoning": "<step-by-step explanation>",
  "supporting_claims": ["<claim1>", "<claim2>"],
  "unsupported_claims": ["<claim3>"]
}
)";
}

std::string PromptTemplateManager::getRelevanceTemplate() {
    return R"(You are an expert evaluator assessing the relevance of an answer to a query.

**Task:** Evaluate how well the answer addresses the original query.

**Instructions:**
1. Identify the key aspects of the query
2. Check if the answer addresses each aspect
3. Assess if there is any irrelevant information
4. Rate relevance on a scale of 1-5:
   - 5: Directly answers all query aspects, no noise
   - 4: Answers most aspects with minor gaps
   - 3: Partially relevant, misses some aspects
   - 2: Tangentially related to query
   - 1: Off-topic or irrelevant

**Query:**
{query}

**Context Documents:**
{context}

**Generated Answer:**
{answer}

**Provide your evaluation in JSON format:**
{
  "score": <1-5>,
  "confidence": <0.0-1.0>,
  "reasoning": "<step-by-step explanation>",
  "covered_aspects": ["<aspect1>", "<aspect2>"],
  "missing_aspects": ["<aspect3>"]
}
)";
}

std::string PromptTemplateManager::getCompletenessTemplate() {
    return R"(You are an expert evaluator assessing the completeness of an answer.

**Task:** Evaluate if the answer comprehensively addresses all aspects of the query.

**Instructions:**
1. Identify all aspects that should be covered
2. Assess the depth and breadth of coverage for each
3. Note any missing information that should be included
4. Rate completeness on a scale of 1-5:
   - 5: All aspects thoroughly covered
   - 4: Most aspects covered adequately
   - 3: Some aspects missing or superficial
   - 2: Many important aspects missing
   - 1: Minimal coverage, largely incomplete

**Query:**
{query}

**Context Documents:**
{context}

**Generated Answer:**
{answer}

**Provide your evaluation in JSON format:**
{
  "score": <1-5>,
  "confidence": <0.0-1.0>,
  "reasoning": "<step-by-step explanation>",
  "covered_aspects": ["<aspect1>", "<aspect2>"],
  "missing_information": ["<info1>", "<info2>"]
}
)";
}

std::string PromptTemplateManager::getCoherenceTemplate() {
    return R"(You are an expert evaluator assessing the coherence and quality of an answer.

**Task:** Evaluate the logical structure, clarity, and consistency of the answer.

**Instructions:**
1. Assess logical flow and argument structure
2. Check for internal consistency and contradictions
3. Evaluate clarity and readability
4. Rate coherence on a scale of 1-5:
   - 5: Excellent structure, clear, and consistent
   - 4: Good structure with minor issues
   - 3: Adequate but with some clarity or consistency issues
   - 2: Poor structure or significant inconsistencies
   - 1: Incoherent or contradictory

**Query:**
{query}

**Generated Answer:**
{answer}

**Provide your evaluation in JSON format:**
{
  "score": <1-5>,
  "confidence": <0.0-1.0>,
  "reasoning": "<step-by-step explanation>",
  "strengths": ["<strength1>", "<strength2>"],
  "weaknesses": ["<weakness1>", "<weakness2>"]
}
)";
}

// Default few-shot examples

std::vector<FewShotExample> PromptTemplateManager::getFaithfulnessExamples() {
    return {
        {
            "What is the capital of France?",
            "Paris is the capital and most populous city of France.",
            "The capital of France is Paris.",
            5.0,
            "Fully supported - answer directly states what is in the context."
        },
        {
            "What is the population of Tokyo?",
            "Tokyo is the capital of Japan with a metropolitan population over 37 million.",
            "Tokyo has a population of 37 million and is one of the world's most expensive cities.",
            3.0,
            "Partially supported - population is correct, but 'most expensive' is not in context."
        }
    };
}

std::vector<FewShotExample> PromptTemplateManager::getRelevanceExamples() {
    return {
        {
            "How do I reset my password?",
            "To reset password, click 'Forgot Password', enter email, and follow the link.",
            "You can reset your password by clicking the 'Forgot Password' link on the login page.",
            5.0,
            "Directly answers the query with clear instructions."
        }
    };
}

std::vector<FewShotExample> PromptTemplateManager::getCompletenessExamples() {
    return {
        {
            "What are the benefits and risks of solar energy?",
            "Solar energy benefits: clean, renewable, reduces bills. Risks: high initial cost, weather dependent.",
            "Solar energy is clean and renewable, which helps reduce electricity bills.",
            3.0,
            "Covers benefits but completely misses the risks aspect of the query."
        }
    };
}

std::vector<FewShotExample> PromptTemplateManager::getCoherenceExamples() {
    return {
        {
            "Explain photosynthesis",
            "",
            "Photosynthesis is the process where plants convert light into energy. This happens in chloroplasts using chlorophyll. The result is glucose and oxygen.",
            5.0,
            "Clear, logical flow from definition to mechanism to output."
        }
    };
}

std::string PromptTemplateManager::replacePlaceholders(
    const std::string& template_str,
    const EvaluationInput& input
) const {
    std::string result = template_str;
    
    // Replace {query}
    size_t pos = 0;
    while ((pos = result.find("{query}", pos)) != std::string::npos) {
        result.replace(pos, 7, input.query);
        pos += input.query.length();
    }
    
    // Replace {answer}
    pos = 0;
    while ((pos = result.find("{answer}", pos)) != std::string::npos) {
        result.replace(pos, 8, input.generated_answer);
        pos += input.generated_answer.length();
    }
    
    // Replace {context} with concatenated documents
    std::ostringstream context_stream = {};
    for (size_t i = 0; i <static_cast<int>(input.documents.size()); ++i) {
        context_stream << "Document " << (i + 1) << ":\n"
                      << input.documents[i].content << "\n\n";
    }
    std::string context = context_stream.str();
    
    pos = 0;
    while ((pos = result.find("{context}", pos)) != std::string::npos) {
        result.replace(pos, 9, context);
        pos += context.length();
    }
    
    return result;
}

std::string PromptTemplateManager::formatFewShotExamples(
    const std::vector<FewShotExample>& examples
) const {
    std::ostringstream stream = {};
    stream << "**Few-Shot Examples:**\n\n";
    
    for (size_t i = 0; i < examples.size(); ++i) {
        const auto& ex = examples[i];
        stream << "Example " << (i + 1) << ":\n";
        stream << "Query: " << ex.query << "\n";
        if (!ex.context.empty()) {
            stream << "Context: " << ex.context << "\n";
        }
        stream << "Answer: " << ex.answer << "\n";
        stream << "Score: " << ex.score << "/5\n";
        stream << "Explanation: " << ex.explanation << "\n\n";
    }
    
    return stream.str();
}

} // namespace themis::rag::judge

