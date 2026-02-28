/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            ab_test_manager.cpp                                ║
  Version:         0.0.1                                              ║
  Last Modified:   2026-02-28                                         ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

// A/B testing framework implementation.
//
// See include/themis/base/ab_test_manager.h for the public API.

#include "themis/base/ab_test_manager.h"

#include <cmath>
#include <functional>

#include <spdlog/spdlog.h>

namespace themis {
namespace modules {

// =============================================================================
// Construction / Destruction
// =============================================================================

ABTestManager::ABTestManager() = default;

ABTestManager::ABTestManager(HotReloadManager& reload_manager)
    : reload_manager_(&reload_manager) {}

ABTestManager::~ABTestManager() = default;

// =============================================================================
// Test lifecycle
// =============================================================================

bool ABTestManager::startTest(const ABModuleTestConfig& config, ModuleLoader& loader) {
    std::lock_guard<std::mutex> lock(mutex_);

    if (tests_.count(config.test_id) > 0) {
        spdlog::warn("ABTestManager: test '{}' already exists", config.test_id);
        return false;
    }

    TestEntry entry;
    entry.config     = config;
    entry.status     = ABTestStatus::ACTIVE;
    entry.start_time = std::chrono::system_clock::now();
    entry.loader_ptr = &loader;

    // Attempt to load the treatment binary under the derived treatment key so
    // both variants are resident at the same time.  A failure here is non-fatal
    // — the test is still registered; routing will always use the control until
    // the treatment becomes available (or the test is re-configured).
    auto result = loader.loadModule(config.treatment_path,
                                    treatmentKey(config.module_name));
    if (result.success) {
        entry.treatment_loaded = true;
        spdlog::info("ABTestManager: test '{}' started – treatment '{}' loaded from '{}'",
                     config.test_id, config.module_name, config.treatment_path);
    } else {
        entry.treatment_loaded = false;
        spdlog::warn("ABTestManager: test '{}' started but treatment binary could not be "
                     "loaded ({}); all traffic will go to control",
                     config.test_id, result.errorMessage);
    }

    tests_[config.test_id] = std::move(entry);
    return true;
}

bool ABTestManager::promoteTest(const std::string& test_id) {
    // Capture the information we need under the lock, then do the I/O outside.
    std::string   module_name;
    std::string   treatment_path;

    {
        std::lock_guard<std::mutex> lock(mutex_);

        auto it = tests_.find(test_id);
        if (it == tests_.end()) {
            spdlog::error("ABTestManager::promoteTest: test '{}' not found", test_id);
            return false;
        }
        if (it->second.status != ABTestStatus::ACTIVE) {
            spdlog::warn("ABTestManager::promoteTest: test '{}' is not ACTIVE", test_id);
            return false;
        }
        module_name    = it->second.config.module_name;
        treatment_path = it->second.config.treatment_path;
    }

    // Unload the treatment slot first (it was loaded under the treatment key).
    // We do this before the hot-reload so the loader doesn't have the binary
    // registered under two names simultaneously.
    {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = tests_.find(test_id);
        if (it != tests_.end()) {
            unloadTreatment(it->second);
        }
    }

    // Use HotReloadManager to atomically replace the control with the treatment
    // binary.  The caller still gets a useful return value even without a
    // reload manager configured.
    bool reload_ok = true;
    if (reload_manager_) {
        auto hr = reload_manager_->reloadModule(module_name, treatment_path);
        reload_ok = hr.success;
        if (!reload_ok) {
            spdlog::error("ABTestManager::promoteTest: hot-reload failed for '{}': {}",
                          module_name, hr.errorMessage);
        }
    }

    {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = tests_.find(test_id);
        if (it != tests_.end()) {
            it->second.status = ABTestStatus::PROMOTED;
        }
    }

    spdlog::info("ABTestManager: test '{}' promoted (module '{}')", test_id, module_name);
    return reload_ok;
}

bool ABTestManager::rollbackTest(const std::string& test_id) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = tests_.find(test_id);
    if (it == tests_.end()) {
        spdlog::error("ABTestManager::rollbackTest: test '{}' not found", test_id);
        return false;
    }
    if (it->second.status != ABTestStatus::ACTIVE) {
        spdlog::warn("ABTestManager::rollbackTest: test '{}' is not ACTIVE", test_id);
        return false;
    }

    unloadTreatment(it->second);
    it->second.status = ABTestStatus::ROLLED_BACK;

    spdlog::info("ABTestManager: test '{}' rolled back", test_id);
    return true;
}

void ABTestManager::cancelTest(const std::string& test_id) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = tests_.find(test_id);
    if (it == tests_.end() || it->second.status != ABTestStatus::ACTIVE) return;

    unloadTreatment(it->second);
    it->second.status = ABTestStatus::CANCELLED;

    spdlog::info("ABTestManager: test '{}' cancelled", test_id);
}

// =============================================================================
// Traffic routing
// =============================================================================

bool ABTestManager::shouldUseTreatment(const std::string& test_id,
                                        const std::string& request_key) const {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = tests_.find(test_id);
    if (it == tests_.end() || it->second.status != ABTestStatus::ACTIVE) {
        return false;
    }

    size_t hash       = hashRequestKey(request_key);
    double normalized = static_cast<double>(hash % 10000u) / 10000.0;
    return normalized < it->second.config.traffic_split;
}

bool ABTestManager::isTreatmentLoaded(const std::string& test_id) const {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = tests_.find(test_id);
    return it != tests_.end() && it->second.treatment_loaded;
}

// =============================================================================
// Metrics recording
// =============================================================================

void ABTestManager::recordOutcome(const std::string& test_id,
                                   bool is_treatment,
                                   bool success,
                                   double latency_ms) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = tests_.find(test_id);
    if (it == tests_.end() || it->second.status != ABTestStatus::ACTIVE) return;

    VariantData& vd = is_treatment ? it->second.treatment : it->second.control;
    ABVariantMetrics& m = vd.metrics;

    m.sample_count++;
    if (success) m.success_count++;
    m.success_rate = static_cast<double>(m.success_count) / m.sample_count;

    // Online mean and variance (Welford's algorithm) for latency.
    if (latency_ms >= 0.0) {
        vd.total_latency_ms += latency_ms;
        double old_mean      = m.mean_latency_ms;
        m.mean_latency_ms    = vd.total_latency_ms / m.sample_count;
        vd.sum_sq_latency   += (latency_ms - old_mean) * (latency_ms - m.mean_latency_ms);
        if (m.sample_count > 1) {
            m.std_dev_latency = std::sqrt(vd.sum_sq_latency / (m.sample_count - 1));
        }
    }
}

// =============================================================================
// Statistical evaluation
// =============================================================================

ABModuleTestResult ABTestManager::evaluateTest(const std::string& test_id) const {
    std::lock_guard<std::mutex> lock(mutex_);

    ABModuleTestResult result;
    result.test_id = test_id;

    auto it = tests_.find(test_id);
    if (it == tests_.end()) return result;

    const ABVariantMetrics& ctrl = it->second.control.metrics;
    const ABVariantMetrics& trt  = it->second.treatment.metrics;

    result.sample_size_control       = ctrl.sample_count;
    result.sample_size_treatment     = trt.sample_count;
    result.control_success_rate      = ctrl.success_rate;
    result.treatment_success_rate    = trt.success_rate;
    result.improvement               = trt.success_rate - ctrl.success_rate;
    result.control_mean_latency_ms   = ctrl.mean_latency_ms;
    result.treatment_mean_latency_ms = trt.mean_latency_ms;

    const size_t min_n = it->second.config.min_samples;
    if (ctrl.sample_count >= min_n && trt.sample_count >= min_n) {
        double z       = calculateZStatistic(ctrl, trt);
        result.p_value = calculatePValue(z);
        result.is_significant =
            (result.p_value < it->second.config.significance_level);
    }

    return result;
}

// =============================================================================
// Queries
// =============================================================================

ABTestStatus ABTestManager::getTestStatus(const std::string& test_id) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = tests_.find(test_id);
    return (it != tests_.end()) ? it->second.status : ABTestStatus::CANCELLED;
}

std::vector<std::string> ABTestManager::getActiveTests() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<std::string> active;
    for (const auto& [id, entry] : tests_) {
        if (entry.status == ABTestStatus::ACTIVE) active.push_back(id);
    }
    return active;
}

ABVariantMetrics ABTestManager::getControlMetrics(const std::string& test_id) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = tests_.find(test_id);
    return (it != tests_.end()) ? it->second.control.metrics : ABVariantMetrics{};
}

ABVariantMetrics ABTestManager::getTreatmentMetrics(const std::string& test_id) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = tests_.find(test_id);
    return (it != tests_.end()) ? it->second.treatment.metrics : ABVariantMetrics{};
}

// =============================================================================
// Helpers
// =============================================================================

/*static*/ std::string ABTestManager::treatmentKey(const std::string& module_name) {
    return module_name + "__ab_treatment__";
}

void ABTestManager::unloadTreatment(TestEntry& entry) {
    if (entry.treatment_loaded && entry.loader_ptr) {
        entry.loader_ptr->unloadModule(treatmentKey(entry.config.module_name));
        entry.treatment_loaded = false;
    }
}

// =============================================================================
// Statistics helpers
// =============================================================================

/*static*/ double ABTestManager::calculateZStatistic(const ABVariantMetrics& ctrl,
                                                      const ABVariantMetrics& trt) {
    // Two-proportion z-test.
    if (ctrl.sample_count == 0 || trt.sample_count == 0) return 0.0;

    double p1 = ctrl.success_rate;
    double p2 = trt.success_rate;
    double n1 = static_cast<double>(ctrl.sample_count);
    double n2 = static_cast<double>(trt.sample_count);

    // Pooled proportion.
    double p_pool = (p1 * n1 + p2 * n2) / (n1 + n2);
    double denom  = std::sqrt(p_pool * (1.0 - p_pool) * (1.0 / n1 + 1.0 / n2));
    if (denom < 1e-10) return 0.0;

    return (p2 - p1) / denom;
}

/*static*/ double ABTestManager::calculatePValue(double z_statistic) {
    // Two-tailed p-value via complementary error function approximation
    // (Abramowitz & Stegun 7.1.26).
    double abs_z = std::abs(z_statistic);
    double x     = abs_z / std::sqrt(2.0);

    double t    = 1.0 / (1.0 + 0.3275911 * x);
    double poly = t * ( 0.254829592
                + t * (-0.284496736
                + t * ( 1.421413741
                + t * (-1.453152027
                + t *   1.061405429))));
    double erfc_approx = poly * std::exp(-x * x);

    return std::min(1.0, erfc_approx); // erfc(|z|/√2) ≈ two-tailed p-value
}

/*static*/ size_t ABTestManager::hashRequestKey(const std::string& key) {
    std::hash<std::string> hasher;
    return hasher(key);
}

} // namespace modules
} // namespace themis
