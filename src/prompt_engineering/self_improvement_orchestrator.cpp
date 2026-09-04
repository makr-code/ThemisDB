/**
 * @file self_improvement_orchestrator.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 100/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=4, H=7, M=4, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#include "prompt_engineering/self_improvement_orchestrator.h"
#include "prompt_engineering/prompt_performance_tracker.h"
#include "prompt_engineering/prompt_optimizer.h"
#include "prompt_engineering/prompt_manager.h"
#include "prompt_engineering/prompt_evaluator.h"
#include "prompt_engineering/feedback_collector.h"
#include "utils/logger.h"
#include <algorithm>
#include <random>
#include <sstream>
#include <iomanip>
#include <cmath>

namespace themis {
namespace prompt_engineering {

// ============================================================================
// OptimizationResult Implementation
// ============================================================================

nlohmann::json OptimizationResult::toJson() const {
    nlohmann::json j;
    j["prompt_id"] = prompt_id;
    j["original_version"] = original_version;
    j["optimized_version"] = optimized_version;
    j["status"] = static_cast<int>(status);
    j["baseline_score"] = baseline_score;
    j["optimized_score"] = optimized_score;
    j["improvement"] = improvement;
    j["iterations"] = iterations;
    
    auto started_time = std::chrono::system_clock::to_time_t(started_at);
    j["started_at"] = started_time;
    
    auto completed_time = std::chrono::system_clock::to_time_t(completed_at);
    j["completed_at"] = completed_time;
    
    j["metadata"] = metadata;
    
    return j;
}

// ============================================================================
// ABTest Implementation
// ============================================================================

nlohmann::json ABTest::toJson() const {
    nlohmann::json j;
    j["test_id"] = test_id;
    j["prompt_id"] = prompt_id;
    j["version_a"] = version_a;
    j["version_b"] = version_b;
    j["samples_a"] = samples_a;
    j["samples_b"] = samples_b;
    j["score_a"] = score_a;
    j["score_b"] = score_b;
    j["is_significant"] = is_significant;
    j["p_value"] = p_value;
    
    auto started_time = std::chrono::system_clock::to_time_t(started_at);
    j["started_at"] = started_time;
    
    auto completed_time = std::chrono::system_clock::to_time_t(completed_at);
    j["completed_at"] = completed_time;
    
    return j;
}

// ============================================================================
// SelfImprovementOrchestrator Implementation
// ============================================================================

SelfImprovementOrchestrator::SelfImprovementOrchestrator(
    const ImprovementConfig& config,
    std::shared_ptr<PromptPerformanceTracker> tracker,
    std::shared_ptr<PromptOptimizer> optimizer,
    std::shared_ptr<PromptManager> manager,
    std::shared_ptr<PromptEvaluator> evaluator
)
    : config_(config)
    , tracker_(tracker)
    , optimizer_(optimizer)
    , manager_(manager)
    , evaluator_(evaluator)
{
    THEMIS_INFO("SelfImprovementOrchestrator initialized with min_success_rate={}, min_executions={}",
                config_.min_success_rate, config_.min_executions);
}

std::vector<OptimizationResult> SelfImprovementOrchestrator::runAutoOptimization() {
    // Collect candidate prompt IDs and their test cases while holding the lock.
    // The actual optimization is run without the lock to avoid holding it during
    // a potentially long-running operation.
    std::vector<std::pair<std::string, std::vector<TestCase>>> candidates;

    {
        std::lock_guard<std::mutex> lock(mutex_);

        if (!tracker_) {
            THEMIS_ERROR("PromptPerformanceTracker not available");
            return {};
        }

        auto all_metrics = tracker_->getAllMetrics();
        THEMIS_INFO("Running auto-optimization check on {} prompts",static_cast<int>(all_metrics.size()));

        for (const auto& metrics : all_metrics) {
            if (!shouldOptimize(metrics.prompt_id)) {
                continue;
            }

            THEMIS_INFO("Triggering optimization for prompt: {}", metrics.prompt_id);

            // Build synthetic test cases from historical positive-feedback entries.
            auto test_cases = buildTestCasesFromFeedback(metrics.prompt_id, 50);

            if (test_cases.empty()) {
                THEMIS_DEBUG("No feedback-derived test cases for '{}' – deferring",
                             metrics.prompt_id);
                last_optimization_[metrics.prompt_id] = std::chrono::system_clock::now();
                continue;
            }

            candidates.emplace_back(metrics.prompt_id, std::move(test_cases));
        }
    }

    // Run each optimization without holding the mutex.
    std::vector<OptimizationResult> results = {};

    results.reserve(candidates.size());
    for (auto& [prompt_id, test_cases] : candidates) {
        results.push_back(optimizePrompt(prompt_id, test_cases));
    }
    return results;
}

OptimizationResult SelfImprovementOrchestrator::optimizePrompt(
    const std::string& prompt_id,
    const std::vector<TestCase>& test_cases
) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    OptimizationResult result;
    result.prompt_id = prompt_id;
    result.status = OptimizationStatus::NOT_STARTED;
    result.started_at = std::chrono::system_clock::now();

    auto finalizeResult = [&]([[maybe_unused]] OptimizationStatus fallback_status) {
        if (result.status == OptimizationStatus::NOT_STARTED) {
            result.status = fallback_status;
        }
        result.completed_at = std::chrono::system_clock::now();
        optimization_history_[prompt_id].push_back(result);
        last_optimization_[prompt_id] = result.completed_at;
        return result;
    };
    
    if (!optimizer_ || !manager_ || !tracker_) {
        THEMIS_ERROR("Required components not available");
        result.status = OptimizationStatus::FAILED;
        return finalizeResult(OptimizationStatus::FAILED);
    }
    
    // Get current prompt template
    auto template_opt = manager_->getTemplate(prompt_id);
    if (!template_opt.has_value()) {
        THEMIS_ERROR("Prompt template not found: {}", prompt_id);
        result.status = OptimizationStatus::FAILED;
        return finalizeResult(OptimizationStatus::FAILED);
    }
    
    result.original_version = template_opt->content;
    
    // Get current performance metrics
    auto metrics_opt = tracker_->getMetrics(prompt_id);
    if (metrics_opt.has_value()) {
        result.baseline_score = metrics_opt->success_rate;
    }
    
    THEMIS_INFO("Starting optimization for prompt '{}' (baseline_score={:.2f})",
                prompt_id, result.baseline_score);
    
    result.status = OptimizationStatus::IN_PROGRESS;
    
    try {
        // Configure optimizer
        OptimizationConfig opt_config;
        opt_config.max_iterations = config_.max_iterations;
        opt_config.target_score = std::min(1.0, result.baseline_score + config_.target_improvement);
        
        optimizer_->setConfig(opt_config);
        
        // Create evaluation function using the PromptEvaluator when available.
        // 
        // Note: A truly accurate evaluation requires executing the prompt through an
        // LLM and scoring the LLM output against expected results.  Callers that have
        // LLM access should supply a custom eval_fn to optimizer_->optimize().
        // When no LLM is available (the default), we fall back to structural similarity
        // between (prompt + input) and expected_output, which serves as a proxy for
        // how well the prompt template incorporates the expected content patterns.
        //
        // The "\n" separator between prompt and input is a neutral token boundary
        // that ensures the tokeniser treats them as separate segments without merging
        // words across the join; it is not a formatting requirement.
        auto eval_fn = [this](const std::string& prompt,
                              const std::vector<TestCase>& cases) -> double {
            // Guard: evaluateBatch() returns 0.0 for empty inputs, which would
            // mislead the optimizer into thinking the prompt is worthless.
            if (evaluator_ && !cases.empty()) {
                std::vector<std::string> prompt_with_inputs;
                std::vector<std::string> expected = {};

                prompt_with_inputs.reserve(cases.size());
                expected.reserve(cases.size());
                for (const auto& tc : cases) {
                    // Combine prompt template with the test input as a single
                    // representation; newline acts as a neutral token boundary.
                    prompt_with_inputs.push_back(prompt + "\n" + tc.input);
                    expected.push_back(tc.expected_output);
                }
                return evaluator_->evaluateBatch(prompt_with_inputs, expected).overall_score;
            }

            // Structural heuristic fallback when no evaluator is available
            double score = 0.3;
            if (prompt.find("Task") != std::string::npos ||
                prompt.find("task") != std::string::npos) score += 0.15;
            if (prompt.find("Example") != std::string::npos ||
                prompt.find("example") != std::string::npos) score += 0.15;
            if (prompt.length() > 100) {
              score += 0.1;
            }
            if (prompt.find("Format") != std::string::npos ||
                prompt.find("Output") != std::string::npos) score += 0.1;
            return std::min(1.0, score);
        };
        
        // Run optimization
        auto opt_result = optimizer_->optimize(
            result.original_version,
            test_cases,
            eval_fn
        );
        
        result.optimized_version = opt_result.optimized_prompt;
        result.optimized_score = opt_result.final_score;
        result.improvement = (result.optimized_score - result.baseline_score) / 
                            std::max(0.01, result.baseline_score);
        result.iterations = opt_result.iterations;
        
        // Check if improvement is significant
        if (result.improvement >= config_.target_improvement * 0.5) {
            // Improvement is good enough
            if (config_.enable_ab_testing) {
                result.status = OptimizationStatus::AB_TESTING;
                
                // Start A/B test
                std::string test_id = startABTest(
                    prompt_id,
                    result.original_version,
                    result.optimized_version,
                    config_.ab_test_sample_size
                );
                
                result.metadata["ab_test_id"] = test_id;
                
                THEMIS_INFO("Optimization completed, starting A/B test: {}", test_id);
            } else {
                // Deploy directly without A/B testing
                result.status = OptimizationStatus::DEPLOYED;
                deployOptimizedVersion(prompt_id, result.optimized_version);
                
                THEMIS_INFO("Optimization completed and deployed (no A/B testing)");
            }
        } else {
            // Improvement not significant enough
            result.status = OptimizationStatus::COMPLETED;
            THEMIS_WARN("Optimization improvement ({:.2%}) below threshold ({:.2%})",
                       result.improvement, config_.target_improvement);
        }
        
    } catch (const std::exception& e) {
        THEMIS_ERROR("Optimization failed: {}", e.what());
        result.status = OptimizationStatus::FAILED;
        result.metadata["error"] = e.what();
    }
    
    return finalizeResult(result.status == OptimizationStatus::NOT_STARTED
                              ? OptimizationStatus::COMPLETED
                              : result.status);
}

std::string SelfImprovementOrchestrator::startABTest(
    const std::string& prompt_id,
    const std::string& version_a,
    const std::string& version_b,
    size_t sample_size
) {
    // Note: Lock already held by caller
    
    ABTest test;
    test.test_id = generateTestId();
    test.prompt_id = prompt_id;
    test.version_a = version_a;
    test.version_b = version_b;
    test.started_at = std::chrono::system_clock::now();
    
    if (sample_size == 0) {
        sample_size = config_.ab_test_sample_size;
    }
    
    active_ab_tests_[test.test_id] = test;
    
    THEMIS_INFO("Started A/B test {} for prompt {} (sample_size={})",
                test.test_id, prompt_id, sample_size);
    
    return test.test_id;
}

void SelfImprovementOrchestrator::recordABTestObservation(
    const std::string& test_id,
    const std::string& version_used,
    bool success,
    double /*latency_ms*/
) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = active_ab_tests_.find(test_id);
    if (it == active_ab_tests_.end()) {
        THEMIS_WARN("A/B test not found: {}", test_id);
        return;
    }
    
    ABTest& test = it->second;
    
    // Determine which version and update stats
    bool is_version_a = (version_used == test.version_a || version_used == "a");
    
    if (is_version_a) {
        // Update version A stats
        double new_score = success ? 1.0 : 0.0;
        test.score_a = (test.score_a * test.samples_a + new_score) / (test.samples_a + 1);
        test.samples_a++;
    } else {
        // Update version B stats
        double new_score = success ? 1.0 : 0.0;
        test.score_b = (test.score_b * test.samples_b + new_score) / (test.samples_b + 1);
        test.samples_b++;
    }
    
    THEMIS_DEBUG("A/B test {} observation: version={}, success={}, samples_a={}, samples_b={}",
                 test_id, version_used, success, test.samples_a, test.samples_b);
    
    // Check if test is complete
    checkABTestCompletion(test_id);
}

bool SelfImprovementOrchestrator::checkABTestCompletion(const std::string& test_id) {
    // Note: Lock already held by caller in most cases
    
    auto it = active_ab_tests_.find(test_id);
    if (it == active_ab_tests_.end()) {
        return false;
    }
    
    ABTest& test = it->second;
    
    // Check if we have enough samples
    size_t target_samples = config_.ab_test_sample_size / 2; // Each version gets half
    if (test.samples_a < target_samples || test.samples_b < target_samples) {
        return false;
    }
    
    // Perform statistical analysis
    analyzeABTest(test);
    
    test.completed_at = std::chrono::system_clock::now();
    
    THEMIS_INFO("A/B test {} completed: score_a={:.3f}, score_b={:.3f}, significant={}, p={:.4f}",
                test_id, test.score_a, test.score_b, test.is_significant, test.p_value);
    
    // Decide whether to deploy based on results
    if (test.is_significant && test.score_b > test.score_a) {
        // Version B is significantly better - deploy it
        THEMIS_INFO("A/B test {} shows version B is better - deploying", test_id);
        deployOptimizedVersion(test.prompt_id, test.version_b);
        
        // Update optimization history
        for (auto& results : optimization_history_[test.prompt_id]) {
            if (results.metadata.contains("ab_test_id") && 
                results.metadata["ab_test_id"] == test_id) {
                results.status = OptimizationStatus::DEPLOYED;
            }
        }
    } else {
        // Version B is not better - keep version A
        THEMIS_WARN("A/B test {} shows no significant improvement - keeping original", test_id);
        
        // Update optimization history
        for (auto& results : optimization_history_[test.prompt_id]) {
            if (results.metadata.contains("ab_test_id") && 
                results.metadata["ab_test_id"] == test_id) {
                results.status = OptimizationStatus::COMPLETED;
            }
        }
    }
    
    return true;
}

std::optional<ABTest> SelfImprovementOrchestrator::getABTestResults(const std::string& test_id) const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = active_ab_tests_.find(test_id);
    if (it != active_ab_tests_.end()) {
        return it->second;
    }
    
    return std::nullopt;
}

bool SelfImprovementOrchestrator::rollbackPrompt(const std::string& prompt_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Get optimization history
    auto hist_it = optimization_history_.find(prompt_id);
    if (hist_it == optimization_history_.end() || hist_it->second.empty()) {
        THEMIS_WARN("No optimization history found for prompt: {}", prompt_id);
        return false;
    }
    
    // Find the last deployed optimization
    auto& history = hist_it->second;
    for (auto it = history.rbegin(); it != history.rend(); ++it) {
        if (it->status == OptimizationStatus::DEPLOYED) {
            // Rollback to original version
            deployOptimizedVersion(prompt_id, it->original_version);
            it->status = OptimizationStatus::ROLLED_BACK;
            
            THEMIS_INFO("Rolled back prompt '{}' to original version", prompt_id);
            return true;
        }
    }
    
    THEMIS_WARN("No deployed optimization found to rollback for prompt: {}", prompt_id);
    return false;
}

std::vector<OptimizationResult> SelfImprovementOrchestrator::getOptimizationHistory(
    const std::string& prompt_id
) const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = optimization_history_.find(prompt_id);
    if (it != optimization_history_.end()) {
        return it->second;
    }
    
    return {};
}

std::vector<ABTest> SelfImprovementOrchestrator::getActiveABTests() const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::vector<ABTest> tests = {};

    tests.reserve(active_ab_tests_.size());
    
    for (const auto& [id, test] : active_ab_tests_) {
        tests.push_back(test);
    }
    
    return tests;
}

void SelfImprovementOrchestrator::setConfig(const ImprovementConfig& config) {
    std::lock_guard<std::mutex> lock(mutex_);
    config_ = config;
    THEMIS_INFO("Updated SelfImprovementOrchestrator configuration");
}

bool SelfImprovementOrchestrator::shouldOptimize(const std::string& prompt_id) const {
    // Note: Lock already held by caller in most cases
    
    if (!tracker_) {
        return false;
    }
    
    // Get performance metrics
    auto metrics_opt = tracker_->getMetrics(prompt_id);
    if (!metrics_opt.has_value()) {
        return false;
    }
    
    const auto& metrics = metrics_opt.value();
    
    // Check minimum execution threshold
    if (metrics.total_executions < config_.min_executions) {
        return false;
    }
    
    // Check if performance is below threshold
    if (metrics.success_rate >= config_.min_success_rate) {
        return false;
    }
    
    // Check if enough time has passed since last optimization
    if (!canReoptimize(prompt_id)) {
        return false;
    }
    
    return true;
}

// ============================================================================
// Private Methods
// ============================================================================

std::string SelfImprovementOrchestrator::generateTestId() const {
    static thread_local std::mt19937_64 gen((std::random_device())());
    static std::uniform_int_distribution<uint64_t> dis;
    auto now = std::chrono::system_clock::now().time_since_epoch();
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now).count();
    
    std::ostringstream oss = {};
    oss << "abtest_" << std::hex << std::setfill('0')
        << std::setw(12) << ms
        << "_"
        << std::setw(8) << dis(gen);
    return oss.str();
}

bool SelfImprovementOrchestrator::canReoptimize(const std::string& prompt_id) const {
    auto it = last_optimization_.find(prompt_id);
    if (it == last_optimization_.end()) {
        return true; // Never optimized before
    }
    
    auto now = std::chrono::system_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::hours>(now - it->second);
    
    return elapsed >= config_.reoptimize_interval;
}

void SelfImprovementOrchestrator::analyzeABTest(ABTest& test) {
    // Two-proportion z-test for A/B test significance
    
    // Calculate pooled proportion
    double p_pooled = (test.score_a * test.samples_a + test.score_b * test.samples_b) /
                      (test.samples_a + test.samples_b);
    
    // Calculate standard error
    double se = std::sqrt(p_pooled * (1.0 - p_pooled) * 
                         (1.0 / test.samples_a + 1.0 / test.samples_b));
    
    // Avoid division by zero
    if (se < 1e-10) {
        test.p_value = 1.0;
        test.is_significant = false;
        return;
    }
    
    // Calculate z-score
    double z = (test.score_b - test.score_a) / se;
    double abs_z = std::abs(z);

    // Compute one-tailed p-value using the standard normal CDF
    // P(Z > |z|) = 1 - Phi(|z|) via the complementary error function:
    //   Phi(x) = 0.5 * erfc(-x / sqrt(2))
    //   P(Z > abs_z) = 0.5 * erfc(abs_z / sqrt(2))
    double p_one_tailed = 0.5 * std::erfc(abs_z / std::sqrt(2.0));

    // Two-tailed p-value
    test.p_value = 2.0 * p_one_tailed;
    
    // Check significance (one-tailed: version B is better)
    double alpha = 1.0 - config_.ab_test_confidence;
    test.is_significant = (p_one_tailed < alpha) && (test.score_b > test.score_a);
}

void SelfImprovementOrchestrator::deployOptimizedVersion(
    const std::string& prompt_id,
    const std::string& version
) {
    if (!manager_) {
        THEMIS_ERROR("PromptManager not available for deployment");
        return;
    }
    
    // Get current template
    auto template_opt = manager_->getTemplate(prompt_id);
    if (!template_opt.has_value()) {
        THEMIS_ERROR("Cannot deploy - template not found: {}", prompt_id);
        return;
    }
    
    // Update template content
    auto updated_template = template_opt.value();
    updated_template.content = version;
    
    // Update metadata to track deployment
    updated_template.metadata["last_deployed"] = 
        std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    
    // Create new version (in production, this would use PromptVersionControl)
    manager_->createTemplate(updated_template);
    
    THEMIS_INFO("Deployed optimized version for prompt: {}", prompt_id);
}

std::vector<TestCase> SelfImprovementOrchestrator::buildTestCasesFromFeedback(
    const std::string& prompt_id,
    size_t max_cases
) const {
    // Note: caller may hold mutex_; FeedbackCollector has its own lock.
    if (!feedback_collector_) {
        return {};
    }

    // Pull positive feedback entries – these represent successful query/response
    // pairs that can serve as reference examples for evaluation.
    auto positive = feedback_collector_->getFeedback(
        prompt_id,
        max_cases,
        FeedbackType::USER_POSITIVE
    );

    std::vector<TestCase> test_cases = {};

    test_cases.reserve(positive.size());

    for (const auto& entry : positive) {
        if (entry.query.empty()) {
          continue;
        }

        TestCase tc;
        tc.input           = entry.query;
        tc.expected_output = entry.response;  // Best known response for this query
        test_cases.push_back(std::move(tc));
    }

    THEMIS_DEBUG("Built {} synthetic test cases from feedback for prompt '{}'",
                 test_cases.size(), prompt_id);
    return test_cases;
}

} // namespace prompt_engineering
} // namespace themis

