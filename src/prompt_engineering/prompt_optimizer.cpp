/**
 * @file prompt_optimizer.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 100/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=2, M=0, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#include "prompt_engineering/prompt_optimizer.h"
#include "prompt_engineering/meta_prompt_generator.h"
#include "utils/logger.h"
#include <algorithm>
#include <sstream>

namespace themis {
namespace prompt_engineering {

PromptOptimizer::PromptOptimizer(const OptimizationConfig& config)
    : config_(config) {
    THEMIS_DEBUG("Initialized PromptOptimizer with max_iterations={}, target_score={}",
                 config_.max_iterations, config_.target_score);
}

OptimizerResult PromptOptimizer::optimize(
    const std::string& initial_prompt,
    const std::vector<TestCase>& test_cases,
    EvaluationFunction eval_fn,
    ImprovementFunction improve_fn
) {
    THEMIS_INFO("Starting prompt optimization with {} test cases",static_cast<int>(test_cases.size()));
    
    if (test_cases.empty()) {
        THEMIS_ERROR("Cannot optimize without test cases");
        return OptimizerResult{};
    }
    
    if (!eval_fn) {
        THEMIS_ERROR("Evaluation function is required");
        return OptimizerResult{};
    }
    
    // Use default improvement function if not provided
    if (!improve_fn) {
        improve_fn = defaultImprovePrompt;
    }
    
    OptimizerResult result;
    result.optimized_prompt = initial_prompt;
    
    std::string current_prompt = initial_prompt;
    double current_score = eval_fn(current_prompt, test_cases);
    double previous_score = 0.0;
    
    result.score_history.push_back(current_score);
    result.prompt_history.push_back(current_prompt);
    
    if (config_.enable_version_control) {
        history_.push_back({current_prompt, current_score});
    }
    
    THEMIS_INFO("Initial prompt score: {:.4f}", current_score);
    
    for (size_t i = 0; i < config_.max_iterations; ++i) {
        // Check if we should continue
        if (!shouldContinue(i, current_score, previous_score)) {
            THEMIS_INFO("Optimization converged after {} iterations", i + 1);
            result.converged = true;
            break;
        }
        
        // Generate feedback
        std::string feedback = generateFeedback(current_prompt, current_score, test_cases);
        
        // Improve prompt
        std::string improved_prompt = improve_fn(current_prompt, current_score, feedback);
        
        // Validate improved prompt
        if (!validatePrompt(improved_prompt)) {
            THEMIS_WARN("Generated invalid prompt at iteration {}, keeping current", i + 1);
            continue;
        }
        
        // Evaluate improved prompt
        previous_score = current_score;
        current_score = eval_fn(improved_prompt, test_cases);
        
        THEMIS_INFO("Iteration {} score: {:.4f} (delta: {:+.4f})",
                    i + 1, current_score, current_score - previous_score);
        
        result.score_history.push_back(current_score);
        result.prompt_history.push_back(improved_prompt);
        
        if (config_.enable_version_control) {
            history_.push_back({improved_prompt, current_score});
        }
        
        current_prompt = improved_prompt;
        result.iterations++;
    }
    
    result.optimized_prompt = current_prompt;
    result.final_score = current_score;
    result.converged = result.converged || (current_score >= config_.target_score);
    
    result.metadata["initial_score"] = result.score_history[0];
    result.metadata["improvement"] = current_score - result.score_history[0];
    result.metadata["relative_improvement"] = 
        (result.score_history[0] > 0) ? 
        (current_score - result.score_history[0]) / result.score_history[0] : 0.0;
    
    THEMIS_INFO("Optimization complete: final_score={:.4f}, improvement={:.4f}, iterations={}",
                result.final_score, 
                result.final_score - result.score_history[0],
                result.iterations);
    
    return result;
}

std::string PromptOptimizer::generateFeedback(
    const std::string& /*prompt*/,
    double score,
    const std::vector<TestCase>& test_cases
) const {
    std::ostringstream feedback = {};
    
    feedback << "Current prompt score: " << score << "\n\n";
    
    if (score < 0.5) {
        feedback << "Performance is poor. Consider:\n";
        feedback << "- Making instructions more specific and clear\n";
        feedback << "- Adding examples to illustrate desired behavior\n";
        feedback << "- Breaking down complex tasks into steps\n";
    } else if (score < 0.7) {
        feedback << "Performance is moderate. Consider:\n";
        feedback << "- Refining instructions for edge cases\n";
        feedback << "- Adding constraints or output format specifications\n";
        feedback << "- Including more diverse examples\n";
    } else if (score < 0.9) {
        feedback << "Performance is good. Consider:\n";
        feedback << "- Fine-tuning wording for clarity\n";
        feedback << "- Ensuring consistent output format\n";
        feedback << "- Optimizing prompt length\n";
    } else {
        feedback << "Performance is excellent. Consider:\n";
        feedback << "- Minor optimizations for efficiency\n";
        feedback << "- Testing on more challenging cases\n";
    }
    
    feedback << "\nNumber of test cases: " << test_cases.size() << "\n";
    
    return feedback.str();
}

std::string PromptOptimizer::defaultImprovePrompt(
    const std::string& current_prompt,
    double current_score,
    const std::string& feedback
) {
    // Production-ready: Use MetaPromptGenerator for systematic improvements
    MetaPromptGenerator meta_gen;
    
    auto meta_result = meta_gen.generateImprovementPrompt(
        current_prompt,
        feedback,
        current_score,
        "" // task_description optional
    );
    
    // Build improved prompt with structured sections
    std::ostringstream improved = {};
    improved << "# Improved Prompt\n\n";
    improved << "## Task\n";
    improved << current_prompt << "\n\n";
    
    // Add improvement suggestions as actionable guidelines
    improved << "## Improvement Guidelines\n";
    improved << meta_result.improvement_suggestion << "\n\n";
    
    // Add key insights for context
    if (!meta_result.key_insights.empty()) {
        improved << "## Key Insights\n";
        for (const auto& insight : meta_result.key_insights) {
            improved << "- " << insight << "\n";
        }
        improved << "\n";
    }
    
    // Add specific guidance based on score
    if (current_score < 0.7) {
        improved << "## Additional Guidelines\n";
        improved << "- Provide clear, specific responses\n";
        improved << "- Follow the instructions exactly\n";
        improved << "- Maintain consistent format\n\n";
    }
    
    return improved.str();
}

std::vector<std::pair<std::string, double>> PromptOptimizer::getHistory() const {
    return history_;
}

void PromptOptimizer::clearHistory() {
    history_.clear();
    THEMIS_DEBUG("Cleared optimization history");
}

bool PromptOptimizer::shouldContinue(
    size_t iteration,
    double current_score,
    double previous_score
) const {
    // Check max iterations
    if (iteration >= config_.max_iterations) {
        return false;
    }
    
    // Check if target reached
    if (current_score >= config_.target_score) {
        return false;
    }
    
    // Check if improvement is too small
    if (iteration > 0) {
        double improvement = current_score - previous_score;
        if (improvement < config_.min_improvement) {
            THEMIS_DEBUG("Improvement {:.4f} below threshold {:.4f}",
                        improvement, config_.min_improvement);
            return false;
        }
    }
    
    return true;
}

bool PromptOptimizer::validatePrompt(const std::string& prompt) const {
    // Basic validation
    if (prompt.empty()) {
        return false;
    }
    
    // Check minimum length
    if (prompt.length() < 10) {
        THEMIS_WARN("Prompt too short: {} characters", prompt.length());
        return false;
    }
    
    // Check maximum length (avoid token limit issues)
    if (prompt.length() > 10000) {
        THEMIS_WARN("Prompt too long: {} characters", prompt.length());
        return false;
    }
    
    return true;
}

} // namespace prompt_engineering
} // namespace themis
