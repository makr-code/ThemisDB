/**
 * @file prompt_optimizer.h
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
#include <optional>
#include <functional>
#include <nlohmann/json.hpp>

namespace themis {
namespace llm {

struct OptimizationConfig {
    size_t max_iterations = 5;        ///< Maximum optimization rounds
    double min_improvement = 0.05;     ///< Minimum score improvement to continue
    double target_score = 0.9;         ///< Target quality score (0.0-1.0)
    bool enable_version_control = true; ///< Track optimization history
    size_t num_test_cases = 10;       ///< Number of test cases for evaluation
};

struct OptimizationResult {
    /**
     * @brief Optimization Result.
     * @return Return value.
     */
    virtual ~OptimizationResult() = default;
    std::string optimized_prompt;      ///< Final optimized prompt
    double final_score = 0.0;          ///< Final evaluation score
    size_t iterations = 0;             ///< Number of iterations performed
    bool converged = false;            ///< Whether optimization converged
    std::vector<double> score_history; ///< Score at each iteration
    std::vector<std::string> prompt_history; ///< Prompts at each iteration
    nlohmann::json metadata;           ///< Additional metadata
};

struct TestCase {
    std::string input;                 ///< Input to the prompt
    std::string expected_output;       ///< Expected output
    nlohmann::json context;            ///< Additional context
};

using EvaluationFunction = std::function<double(
    const std::string& prompt,
    const std::vector<TestCase>& test_cases
)>;

using ImprovementFunction = std::function<std::string(
    const std::string& current_prompt,
    double current_score,
    const std::string& feedback
)>;

class PromptOptimizer {
public:
    explicit PromptOptimizer(const OptimizationConfig& config = OptimizationConfig{});
    
    OptimizationResult optimize(
        const std::string& initial_prompt,
        const std::vector<TestCase>& test_cases,
        EvaluationFunction eval_fn,
        ImprovementFunction improve_fn = nullptr
    );
    
    /**
     * @brief Generate Feedback.
     * @param[in] prompt Input parameter.
     * @param[in] score Input parameter.
     * @param[in] test_cases Input parameter.
     * @return Return value.
     */
    std::string generateFeedback(
        const std::string& prompt,
        double score,
        const std::vector<TestCase>& test_cases
    ) const;
    
    /**
     * @brief Default Improve Prompt.
     * @param[in] current_prompt Input parameter.
     * @param[in] current_score Input parameter.
     * @param[in] feedback Input parameter.
     * @return Return value.
     */
    static std::string defaultImprovePrompt(
        const std::string& current_prompt,
        double current_score,
        const std::string& feedback
    );
    
    std::vector<std::pair<std::string, double>> getHistory() const;
    
    /**
     * @brief Clear History.
     */
    void clearHistory();
    
    const OptimizationConfig& getConfig() const { return config_; }
    
    /**
     * @brief Set Config.
     * @param[in] config Input parameter.
     * @details Implements setConfig without additional internal calls.
     */
    void setConfig(const OptimizationConfig& config) { config_ = config; }

private:
    OptimizationConfig config_;
    std::vector<std::pair<std::string, double>> history_;
    
    /**
     * @brief Should Continue.
     * @param[in] iteration Input parameter.
     * @param[in] current_score Input parameter.
     * @param[in] previous_score Input parameter.
     * @return True when the operation succeeds.
     */
    bool shouldContinue(size_t iteration, double current_score, double previous_score) const;
    
    /**
     * @brief Validate Prompt.
     * @param[in] prompt Input parameter.
     * @return True when the operation succeeds.
     */
    bool validatePrompt(const std::string& prompt) const;
};

} // namespace llm
} // namespace themis
