/**
 * @file ab_testing_framework.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 100/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#pragma once

#include <chrono>
#include <memory>
#include <string>
#include <vector>

#include "learning_metrics.h"

namespace themis::rag::learning {

/**
 * @brief Status of an A/B test
 */
enum class ABTestStatus {
    ACTIVE,      ///< Test is currently running
    COMPLETED,   ///< Test completed with decision
    PROMOTED,    ///< Treatment was promoted to production
    ROLLED_BACK, ///< Treatment was rolled back
    CANCELLED    ///< Test was manually cancelled
};

/**
 * @brief Configuration for an A/B test
 */
struct ABTestConfig {
    std::string test_id;
    std::string component;               ///< Component being tested (LoRA, Prompt, Retrieval)
    double traffic_split      = 0.1;     ///< Percentage for treatment (0.0-1.0)
    size_t min_samples        = 1000;    ///< Minimum samples needed
    double significance_level = 0.05;    ///< p-value threshold
    double min_improvement    = 0.02;    ///< Minimum improvement to promote (2%)
    std::chrono::hours max_duration{72}; ///< Maximum test duration
};

/**
 * @brief Metrics for a test group (control or treatment)
 */
struct GroupMetrics {
    size_t sample_count = 0;
    double success_rate = 0.0;
    double mean         = 0.0;
    double std_dev      = 0.0;
    std::vector<double> observations;
};

/**
 * @brief A/B testing framework
 *
 * Manages multiple concurrent A/B tests with automatic traffic splitting,
 * metrics collection, and statistical validation.
 */
class ABTestingFramework {
  public:
    ABTestingFramework();
    ~ABTestingFramework();

    /**
     * @brief Start a new A/B test
     * @param config Test configuration
     * @return true if test started successfully
     */
    bool startTest(const ABTestConfig &config);

    /**
     * @brief Record an observation for a test
     * @param test_id Test identifier
     * @param is_treatment true for treatment group, false for control
     * @param success true if interaction was successful
     * @param metric_value Optional numeric metric value
     */
    void recordObservation(const std::string &test_id, bool is_treatment, bool success, double metric_value = 0.0);

    /**
     * @brief Check if user should be assigned to treatment group
     * @param test_id Test identifier
     * @param user_id User/session identifier for consistent assignment
     * @return true if user should receive treatment
     */
    bool shouldUseTreatment(const std::string &test_id, const std::string &user_id);

    /**
     * @brief Evaluate A/B test and get statistical results
     * @param test_id Test identifier
     * @return Test results with statistical analysis
     */
    ABTestResult evaluateTest(const std::string &test_id);

    /**
     * @brief Get all active tests
     */
    std::vector<std::string> getActiveTests() const;

    /**
     * @brief Get test status
     */
    ABTestStatus getTestStatus(const std::string &test_id) const;

    /**
     * @brief Complete a test (promote or rollback)
     * @param test_id Test identifier
     * @param promote true to promote, false to rollback
     */
    void completeTest(const std::string &test_id, bool promote);

    /**
     * @brief Cancel an active test
     */
    void cancelTest(const std::string &test_id);

  private:
    struct Impl;
    std::unique_ptr<Impl> impl_;

    // Statistical methods
    double calculateTStatistic(const GroupMetrics &control, const GroupMetrics &treatment);

    double calculatePValue(double t_statistic, size_t df);

    // Hash function for consistent user assignment
    size_t hashUserId(const std::string &user_id);
};

} // namespace themis::rag::learning
