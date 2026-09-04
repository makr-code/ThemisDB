/**
 * @file meta_prompt_generator.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 100/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=10, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#include "prompt_engineering/meta_prompt_generator.h"
#include "utils/logger.h"
#include <sstream>
#include <algorithm>
#include <cctype>

namespace themis {
namespace prompt_engineering {

MetaPromptGenerator::MetaPromptGenerator(const MetaPromptConfig& config)
    : config_(config) {
    THEMIS_DEBUG("Initialized MetaPromptGenerator with strategy={}",
                 config_.improvement_strategy);
}

MetaPromptResult MetaPromptGenerator::generateImprovementPrompt(
    const std::string& original_prompt,
    const std::string& feedback,
    double score,
    const std::string& task_description
) const {
    MetaPromptResult result;
    
    std::ostringstream meta_prompt = {};
    
    // Header
    meta_prompt << "# Prompt Improvement Task\n\n";
    
    // Task description
    if (!task_description.empty()) {
        meta_prompt << "## Task Context\n";
        meta_prompt << task_description << "\n\n";
    }
    
    // Original prompt
    meta_prompt << "## Current Prompt\n";
    meta_prompt << "```\n" << original_prompt << "\n```\n\n";
    
    // Performance feedback
    meta_prompt << "## Performance Feedback\n";
    meta_prompt << "Current Score: " << score << " / 1.0\n";
    meta_prompt << feedback << "\n\n";
    
    // Improvement instructions
    meta_prompt << buildImprovementInstructions(feedback, score);
    
    // Constraints
    if (config_.include_constraints) {
        meta_prompt << buildConstraints();
    }
    
    // Examples
    if (config_.include_examples) {
        meta_prompt << buildExampleSection(original_prompt);
    }
    
    // Output format
    meta_prompt << "## Output Format\n";
    meta_prompt << "Provide the improved prompt in a clear, structured format.\n";
    meta_prompt << "Include explanations of key changes made.\n\n";
    
    result.meta_prompt = meta_prompt.str();

    if (!result.metadata.is_object()) {
        result.metadata = nlohmann::json::object();
    }

    // If a live LLM provider is attached, invoke it to get a real improved prompt
    if (llm_provider_) {
        THEMIS_DEBUG("Calling LLM provider '{}' for real-time improvement", llm_provider_->name());
        try {
            std::string llm_response = llm_provider_->complete(result.meta_prompt);
            if (!llm_response.empty()) {
                result.improvement_suggestion = llm_response;
                result.metadata["llm_provider"] = llm_provider_->name();
                result.metadata["llm_generated"] = true;
                THEMIS_DEBUG("LLM provider returned {} chars", llm_response.size());
                // key_insights remain from the template-based path below
            } else {
                THEMIS_WARN("LLM provider '{}' returned empty response – falling back to template",
                            llm_provider_->name());
            }
        } catch (const std::exception& ex) {
            THEMIS_ERROR("LLM provider '{}' threw: {} – falling back to template",
                         llm_provider_->name(), ex.what());
        }
    }
    
    // Generate specific improvement suggestions (template-based fallback / additional hints)
    if (!result.metadata.value("llm_generated", false)) {
        result.improvement_suggestion = "Consider the following improvements:\n";
        if (score < 0.5) {
            result.improvement_suggestion += "- Clarify the task objectives\n";
            result.improvement_suggestion += "- Add step-by-step instructions\n";
            result.improvement_suggestion += "- Include concrete examples\n";
        } else if (score < 0.7) {
            result.improvement_suggestion += "- Refine edge case handling\n";
            result.improvement_suggestion += "- Specify output format more clearly\n";
            result.improvement_suggestion += "- Add constraint specifications\n";
        } else {
            result.improvement_suggestion += "- Optimize for conciseness\n";
            result.improvement_suggestion += "- Ensure consistent formatting\n";
            result.improvement_suggestion += "- Fine-tune language for clarity\n";
        }
    }
    
    // Extract key insights
    if (score < 0.5) {
        result.key_insights.push_back("Prompt lacks clarity or specificity");
        result.key_insights.push_back("Consider restructuring with clear sections");
    } else if (score < 0.9) {
        result.key_insights.push_back("Prompt is functional but can be improved");
        result.key_insights.push_back("Focus on edge cases and formatting");
    } else {
        result.key_insights.push_back("Prompt performs well");
        result.key_insights.push_back("Minor optimizations possible");
    }
    
    result.metadata["score"] = score;
    result.metadata["strategy"] = config_.improvement_strategy;
    result.metadata["original_length"] = original_prompt.length();
    
    THEMIS_DEBUG("Generated meta-prompt of length {}", result.meta_prompt.length());
    
    return result;
}

std::string MetaPromptGenerator::generateAnalysisPrompt(
    const std::string& prompt,
    const std::vector<std::pair<std::string, std::string>>& examples
) const {
    std::ostringstream analysis = {};
    
    analysis << "# Prompt Quality Analysis\n\n";
    
    analysis << "## Prompt to Analyze\n";
    analysis << "```\n" << prompt << "\n```\n\n";
    
    analysis << "## Test Examples\n";
    for (size_t i = 0; i < examples.size(); ++i) {
        analysis << "### Example " << (i + 1) << "\n";
        analysis << "**Input**: " << examples[i].first << "\n";
        analysis << "**Expected Output**: " << examples[i].second << "\n\n";
    }
    
    analysis << "## Analysis Tasks\n";
    analysis << "1. Evaluate clarity and specificity of instructions\n";
    analysis << "2. Assess whether examples cover the input space adequately\n";
    analysis << "3. Identify potential edge cases not addressed\n";
    analysis << "4. Rate overall prompt quality (0-10)\n";
    analysis << "5. Provide specific improvement recommendations\n\n";
    
    return analysis.str();
}

std::vector<std::string> MetaPromptGenerator::generateImprovementSuggestions(
    const std::string& prompt,
    const std::string& weakness
) const {
    std::vector<std::string> suggestions;
    
    // Analyze prompt structure
    auto structure = analyzePromptStructure(prompt);
    
    if (weakness.find("clarity") != std::string::npos ||
        weakness.find("unclear") != std::string::npos) {
        suggestions.push_back("Break down instructions into numbered steps");
        suggestions.push_back("Use clear, simple language");
        suggestions.push_back("Define any technical terms");
    }
    
    if (weakness.find("example") != std::string::npos ||
        weakness.find("demonstration") != std::string::npos) {
        suggestions.push_back("Add 2-3 concrete examples");
        suggestions.push_back("Show both simple and complex cases");
        suggestions.push_back("Highlight key patterns in examples");
    }
    
    if (weakness.find("format") != std::string::npos ||
        weakness.find("output") != std::string::npos) {
        suggestions.push_back("Specify exact output format");
        suggestions.push_back("Provide output template or schema");
        suggestions.push_back("Include formatting requirements");
    }
    
    if (weakness.find("constraint") != std::string::npos ||
        weakness.find("limitation") != std::string::npos) {
        suggestions.push_back("List explicit constraints");
        suggestions.push_back("Define boundary conditions");
        suggestions.push_back("Specify error handling requirements");
    }
    
    // Generic suggestions if no specific weakness identified
    if (suggestions.empty()) {
        suggestions.push_back("Review and simplify language");
        suggestions.push_back("Add structure with headers");
        suggestions.push_back("Include validation criteria");
    }
    
    return suggestions;
}

std::vector<std::string> MetaPromptGenerator::extractSuccessPatterns(
    const std::vector<std::pair<std::string, double>>& successful_prompts
) const {
    std::vector<std::string> patterns;
    
    if (successful_prompts.empty()) {
        return patterns;
    }
    
    // Analyze common characteristics
    bool has_examples = false;
    bool has_steps = false;
    bool has_format_spec = false;
    bool has_constraints = false;
    
    for (const auto& [prompt, score] : successful_prompts) {
        if (score < 0.8) continue; // Only analyze high-performing prompts
        
        if (prompt.find("Example") != std::string::npos ||
            prompt.find("example") != std::string::npos) {
            has_examples = true;
        }
        
        if (prompt.find("Step") != std::string::npos ||
            prompt.find("1.") != std::string::npos ||
            prompt.find("2.") != std::string::npos) {
            has_steps = true;
        }
        
        if (prompt.find("Format:") != std::string::npos ||
            prompt.find("Output:") != std::string::npos) {
            has_format_spec = true;
        }
        
        if (prompt.find("Constraint") != std::string::npos ||
            prompt.find("must") != std::string::npos ||
            prompt.find("should") != std::string::npos) {
            has_constraints = true;
        }
    }
    
    // Extract patterns
    if (has_examples) {
        patterns.push_back("High-performing prompts include concrete examples");
    }
    
    if (has_steps) {
        patterns.push_back("Successful prompts use step-by-step instructions");
    }
    
    if (has_format_spec) {
        patterns.push_back("Clear output format specifications improve performance");
    }
    
    if (has_constraints) {
        patterns.push_back("Explicit constraints help guide the model");
    }
    
    // Add general patterns
    patterns.push_back("Structure prompts with clear sections (task, examples, output)");
    patterns.push_back("Use precise, unambiguous language");
    patterns.push_back("Include both positive and negative examples when relevant");
    
    return patterns;
}

std::string MetaPromptGenerator::buildImprovementInstructions(
    const std::string& /*feedback*/,
    double /*score*/
) const {
    std::ostringstream instructions = {};
    
    instructions << "## Improvement Instructions\n";
    
    if (config_.improvement_strategy == "iterative") {
        instructions << "Make incremental improvements based on the feedback:\n";
        instructions << "1. Identify the main weaknesses\n";
        instructions << "2. Address them systematically\n";
        instructions << "3. Preserve what works well\n";
        instructions << "4. Ensure changes are coherent\n\n";
    } else if (config_.improvement_strategy == "analytical") {
        instructions << "Analyze the prompt systematically:\n";
        instructions << "1. Break down into components\n";
        instructions << "2. Evaluate each component\n";
        instructions << "3. Redesign weak components\n";
        instructions << "4. Integrate improvements\n\n";
    } else if (config_.improvement_strategy == "creative") {
        instructions << "Explore creative improvements:\n";
        instructions << "1. Consider alternative phrasings\n";
        instructions << "2. Try different structural approaches\n";
        instructions << "3. Experiment with examples\n";
        instructions << "4. Test novel techniques\n\n";
    }
    
    return instructions.str();
}

std::string MetaPromptGenerator::buildConstraints() const {
    std::ostringstream constraints = {};
    
    constraints << "## Constraints\n";
    constraints << "- Keep the improved prompt focused and concise\n";
    constraints << "- Maintain the original task intent\n";
    constraints << "- Use clear, professional language\n";
    
    if (config_.max_prompt_length > 0) {
        constraints << "- Target length: ~" << config_.max_prompt_length << " characters\n";
    }
    
    constraints << "\n";
    
    return constraints.str();
}

std::string MetaPromptGenerator::buildExampleSection(
    const std::string& /*original_prompt*/
) const {
    std::ostringstream examples = {};
    
    examples << "## Example Improvements\n";
    examples << "### Pattern 1: Adding Structure\n";
    examples << "Before: 'Do the task'\n";
    examples << "After: '# Task\\n## Instructions\\n1. Step one\\n2. Step two'\n\n";
    
    examples << "### Pattern 2: Adding Examples\n";
    examples << "Before: 'Classify sentiment'\n";
    examples << "After: 'Classify sentiment. Examples: \"great\" -> positive, \"bad\" -> negative'\n\n";
    
    return examples.str();
}

nlohmann::json MetaPromptGenerator::analyzePromptStructure(const std::string& prompt) const {
    nlohmann::json analysis;
    
    analysis["length"] = prompt.length();
    
    // Use word boundary checks for more accurate detection
    auto contains_word = [&prompt](const std::string& word) {
        // Simple word boundary check - could be improved with regex
        std::string lower_prompt = prompt;
        std::transform(lower_prompt.begin(), lower_prompt.end(), lower_prompt.begin(), ::tolower);
        size_t pos = lower_prompt.find(word);
        if (pos == std::string::npos) {
          return false;
        }
        
        // Check boundaries
        bool start_ok = (pos == 0 || !std::isalnum(lower_prompt[pos - 1]));
        bool end_ok = (pos + word.length() >= lower_prompt.length() || 
                      !std::isalnum(lower_prompt[pos + word.length()]));
        return start_ok && end_ok;
    };
    
    analysis["has_examples"] = contains_word("example") || contains_word("examples");
    analysis["has_steps"] = contains_word("step") || contains_word("steps") || 
                            prompt.find("1.") != std::string::npos;
    analysis["has_headers"] = (prompt.find("#") != std::string::npos);
    analysis["has_format_spec"] = contains_word("format") || contains_word("output");
    
    // Count sentences (rough approximation)
    size_t sentence_count = 0;
    for (char c : prompt) {
        if (c == '.' || c == '!' || c == '?') {
            sentence_count++;
        }
    }
    analysis["sentence_count"] = sentence_count;
    
    return analysis;
}

} // namespace prompt_engineering
} // namespace themis
