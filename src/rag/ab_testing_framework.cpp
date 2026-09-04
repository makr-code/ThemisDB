/**
 * @file ab_testing_framework.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 100/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=1, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#include "rag/ab_testing_framework.h"

#include <cmath>
#include <functional>
#include <mutex>
#include <numeric>
#include <unordered_map>

namespace themis::rag::learning {

struct TestData {
    ABTestConfig config;
    ABTestStatus status = ABTestStatus::ACTIVE;
    GroupMetrics control;
    GroupMetrics treatment;
    std::chrono::system_clock::time_point start_time;
};

struct ABTestingFramework::Impl {
    std::unordered_map<std::string, TestData> tests;
    mutable std::mutex mutex;
};

ABTestingFramework::ABTestingFramework() : impl_(std::make_unique<Impl>()) {}

ABTestingFramework::~ABTestingFramework() = default;

bool ABTestingFramework::startTest(const ABTestConfig &config) {
    std::lock_guard<std::mutex> lock(impl_->mutex);

    if (impl_->tests.count(config.test_id) > 0) {
        return false; // Test already exists
    }

    TestData data;
    data.config                  = config;
    data.status                  = ABTestStatus::ACTIVE;
    data.start_time              = std::chrono::system_clock::now();
    impl_->tests[config.test_id] = data;

    return true;
}

void ABTestingFramework::recordObservation(const std::string &test_id, bool is_treatment, bool success,
                                           [[maybe_unused]] double metric_value) {
    std::lock_guard<std::mutex> lock(impl_->mutex);

    auto it = impl_->tests.find(test_id);
    if (it == impl_->tests.end() || it->second.status != ABTestStatus::ACTIVE) {
        return; // Test not found or not active
    }

    GroupMetrics &group = is_treatment ? it->second.treatment : it->second.control;
    group.sample_count++;
    group.observations.push_back(success ? 1.0 : 0.0);

    // Update success rate
    size_t success_count = 0;
    for (double obs : group.observations) {
        if (obs > 0.5)
            success_count++;
    }
    group.success_rate = static_cast<double>(success_count) / group.sample_count;

    // Update mean and std dev
    double sum = std::accumulate(group.observations.begin(), group.observations.end(), 0.0);
    group.mean = sum / group.sample_count;
    
    // Calculate sample standard deviation (using n-1)
    if (group.sample_count > 1) {
        double sq_sum = 0.0;
        for (double obs : group.observations) {
            sq_sum += (obs - group.mean) * (obs - group.mean);
        }
        group.std_dev = std::sqrt(sq_sum / (group.sample_count - 1));
    } else {
        group.std_dev = 0.0;
    }
}

bool ABTestingFramework::shouldUseTreatment(const std::string &test_id, const std::string &user_id) {
    std::lock_guard<std::mutex> lock(impl_->mutex);

    auto it = impl_->tests.find(test_id);
    if (it == impl_->tests.end() || it->second.status != ABTestStatus::ACTIVE) {
        return false;
    }

    // Hash user_id and use traffic split to determine group
    size_t hash       = hashUserId(user_id);
    double normalized = static_cast<double>(hash % 10000) / 10000.0;

    return normalized < it->second.config.traffic_split;
}

ABTestResult ABTestingFramework::evaluateTest(const std::string &test_id) {
    std::lock_guard<std::mutex> lock(impl_->mutex);

    auto it = impl_->tests.find(test_id);
    if (it == impl_->tests.end()) {
        return ABTestResult{}; // Empty result
    }

    const TestData &data          = it->second;
    const GroupMetrics &control   = data.control;
    const GroupMetrics &treatment = data.treatment;

    ABTestResult result;
    result.test_id                = test_id;
    result.control_success_rate   = control.success_rate;
    result.treatment_success_rate = treatment.success_rate;
    result.improvement            = treatment.success_rate - control.success_rate;
    result.sample_size_control    = control.sample_count;
    result.sample_size_treatment  = treatment.sample_count;

    // Perform t-test if enough samples
    if (control.sample_count >= 30 && treatment.sample_count >= 30) {
        double t_stat         = calculateTStatistic(control, treatment);
        size_t df             = control.sample_count + treatment.sample_count - 2;
        result.p_value        = calculatePValue(t_stat, df);
        result.is_significant = (result.p_value < data.config.significance_level);
    }

    return result;
}

std::vector<std::string> ABTestingFramework::getActiveTests() const {
    std::lock_guard<std::mutex> lock(impl_->mutex);

    std::vector<std::string> active = {};

    for (const auto &[test_id, data] : impl_->tests) {
        if (data.status == ABTestStatus::ACTIVE) {
            active.push_back(test_id);
        }
    }
    return active;
}

ABTestStatus ABTestingFramework::getTestStatus(const std::string &test_id) const {
    std::lock_guard<std::mutex> lock(impl_->mutex);

    auto it = impl_->tests.find(test_id);
    if (it == impl_->tests.end()) {
        return ABTestStatus::CANCELLED;
    }
    return it->second.status;
}

void ABTestingFramework::completeTest(const std::string &test_id, bool promote) {
    std::lock_guard<std::mutex> lock(impl_->mutex);

    auto it = impl_->tests.find(test_id);
    if (it == impl_->tests.end()) {
        return;
    }

    it->second.status = promote ? ABTestStatus::PROMOTED : ABTestStatus::ROLLED_BACK;
}

void ABTestingFramework::cancelTest(const std::string &test_id) {
    std::lock_guard<std::mutex> lock(impl_->mutex);

    auto it = impl_->tests.find(test_id);
    if (it != impl_->tests.end()) {
        it->second.status = ABTestStatus::CANCELLED;
    }
}

double ABTestingFramework::calculateTStatistic(const GroupMetrics &control, const GroupMetrics &treatment) {
    // Two-sample t-test (Welch's t-test for unequal variances)
    double mean_diff = treatment.mean - control.mean;

    double se_squared = (control.std_dev * control.std_dev / control.sample_count)
                        + (treatment.std_dev * treatment.std_dev / treatment.sample_count);

    if (se_squared < 1e-10) {
        return 0.0; // Avoid division by zero
    }

    return mean_diff / std::sqrt(se_squared);
}

double ABTestingFramework::calculatePValue(double t_statistic, size_t df) {
    static_cast<void>(df);
    // Simplified p-value calculation using normal approximation
    // For large df (>30), t-distribution ≈ normal distribution

    double abs_t = std::abs(t_statistic);

    // Approximate cumulative normal distribution
    // Using error function approximation
    double z          = abs_t / std::sqrt(2.0);
    double erf_approx = std::tanh(1.2 * z + 0.2 * z * z * z);
    double cdf        = 0.5 * (1.0 + erf_approx);

    // Two-tailed p-value
    return 2.0 * (1.0 - cdf);
}

size_t ABTestingFramework::hashUserId(const std::string &user_id) {
    // Simple hash function for user assignment
    std::hash<std::string> hasher;
    return hasher(user_id);
}

} // namespace themis::rag::learning
