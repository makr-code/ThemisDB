/**
 * @file prompt_optimizer.h
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
#include <optional>
#include <functional>
#include <nlohmann/json.hpp>

namespace themis {
namespace prompt_engineering {

/**
 * @brief Configuration for prompt optimization
 */
struct OptimizationConfig {
    size_t max_iterations = 5;        ///< Maximum optimization rounds
    double min_improvement = 0.05;     ///< Minimum score improvement to continue
    double target_score = 0.9;         ///< Target quality score (0.0-1.0)
    bool enable_version_control = true; ///< Track optimization history
    size_t num_test_cases = 10;       ///< Number of test cases for evaluation
};

/**
 * @brief Internal result of prompt optimization (renamed to avoid conflict with self_improvement_orchestrator.h)
 */
struct OptimizerResult {
    std::string optimized_prompt;         ///< Final optimized prompt
    double final_score = 0.0;             ///< Final evaluation score
    size_t iterations = 0;                ///< Number of iterations performed
    bool converged = false;               ///< Whether optimization converged
    std::vector<double> score_history;    ///< Score at each iteration
    std::vector<std::string> prompt_history; ///< Prompt at each iteration
    nlohmann::json metadata;              ///< Additional metadata
};

/**
 * @brief Test case for prompt evaluation
 */
struct TestCase {
    std::string input;                 ///< Input to the prompt
    std::string expected_output;       ///< Expected output
    nlohmann::json context;            ///< Additional context
};

/**
 * @brief Evaluation function type
 * Takes a prompt and test cases, returns a score (0.0-1.0)
 */
using EvaluationFunction = std::function<double(
    const std::string& prompt,
    const std::vector<TestCase>& test_cases
)>;

/**
 * @brief Prompt improvement function type
 * Takes current prompt, score, and feedback, returns improved prompt
 */
using ImprovementFunction = std::function<std::string(
    const std::string& current_prompt,
    double current_score,
    const std::string& feedback
)>;

/**
 * @brief Main prompt optimizer class
 * 
 * Implements iterative prompt improvement using:
 * - Feedback-driven refinement
 * - Multi-round optimization
 * - Version control and history tracking
 */
class PromptOptimizer {
public:
    /**
     * @brief Constructor
     * @param config Optimization configuration
     */
    explicit PromptOptimizer(const OptimizationConfig& config = OptimizationConfig{});
    
    /**
     * @brief Optimize a prompt using test cases
     * @param initial_prompt Starting prompt template
     * @param test_cases Test cases for evaluation
     * @param eval_fn Evaluation function
     * @param improve_fn Improvement function (optional, uses default if not provided)
     * @return Optimization result
     */
    OptimizerResult optimize(
        const std::string& initial_prompt,
        const std::vector<TestCase>& test_cases,
        EvaluationFunction eval_fn,
        ImprovementFunction improve_fn = nullptr
    );
    
    /**
     * @brief Generate feedback for a prompt based on evaluation
     * @param prompt Current prompt
     * @param score Current score
     * @param test_cases Test cases
     * @return Feedback string
     */
    std::string generateFeedback(
        const std::string& prompt,
        double score,
        const std::vector<TestCase>& test_cases
    ) const;
    
    /**
     * @brief Default improvement function using meta-prompting
     * @param current_prompt Current prompt
     * @param current_score Current score
     * @param feedback Feedback about the prompt
     * @return Improved prompt
     */
    static std::string defaultImprovePrompt(
        const std::string& current_prompt,
        double current_score,
        const std::string& feedback
    );
    
    /**
     * @brief Get optimization history
     * @return Vector of (prompt, score) pairs
     */
    std::vector<std::pair<std::string, double>> getHistory() const;
    
    /**
     * @brief Clear optimization history
     */
    void clearHistory();
    
    /**
     * @brief Get current configuration
     */
    const OptimizationConfig& getConfig() const { return config_; }
    
    /**
     * @brief Update configuration
     */
    void setConfig(const OptimizationConfig& config) { config_ = config; }

private:
    OptimizationConfig config_;
    std::vector<std::pair<std::string, double>> history_;
    
    /**
     * @brief Check if optimization should continue
     */
    bool shouldContinue(size_t iteration, double current_score, double previous_score) const;
    
    /**
     * @brief Validate prompt quality
     */
    bool validatePrompt(const std::string& prompt) const;
};

} // namespace prompt_engineering
} // namespace themis
