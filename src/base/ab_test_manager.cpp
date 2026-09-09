/**
 * @file ab_test_manager.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=2, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


// A/B testing framework implementation.
//
// See include/themis/base/ab_test_manager.h for the public API.

#include "themis/base/ab_test_manager.h"

#include <cmath>
#include <functional>
#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>

#include "observability/metrics_collector.h"
#include "themis/base/interfaces/storage_interface.h"

namespace themis {
namespace modules {

// =============================================================================
// Internal helpers (string conversion for ABTestStatus)
// =============================================================================

namespace {

static std::string statusToString(ABTestStatus s) {
    switch (s) {
        case ABTestStatus::ACTIVE:
            return "ACTIVE";
        case ABTestStatus::PROMOTED:
            return "PROMOTED";
        case ABTestStatus::ROLLED_BACK:
            return "ROLLED_BACK";
        case ABTestStatus::CANCELLED:
            return "CANCELLED";
    }
    return "CANCELLED";
}

static ABTestStatus statusFromString(const std::string &s) {
    if (s == "ACTIVE") {
        return ABTestStatus::ACTIVE;
    }
    if (s == "PROMOTED") {
        return ABTestStatus::PROMOTED;
    }
    if (s == "ROLLED_BACK") {
        return ABTestStatus::ROLLED_BACK;
    }
    return ABTestStatus::CANCELLED;
}

static nlohmann::json configToJson(const ABModuleTestConfig &cfg) {
    return {{"test_id", cfg.test_id},
            {"module_name", cfg.module_name},
            {"control_path", cfg.control_path},
            {"treatment_path", cfg.treatment_path},
            {"traffic_split", cfg.traffic_split},
            {"min_samples", cfg.min_samples},
            {"significance_level", cfg.significance_level},
            {"min_improvement", cfg.min_improvement},
            {"max_duration_hours", cfg.max_duration.count()},
            {"thompson_stop_threshold", cfg.thompson_stop_threshold}};
}

static ABModuleTestConfig configFromJson(const nlohmann::json &j) {
    ABModuleTestConfig cfg;
    cfg.test_id                 = j.value("test_id", "");
    cfg.module_name             = j.value("module_name", "");
    cfg.control_path            = j.value("control_path", "");
    cfg.treatment_path          = j.value("treatment_path", "");
    cfg.traffic_split           = j.value("traffic_split", 0.1);
    cfg.min_samples             = j.value("min_samples", size_t{100});
    cfg.significance_level      = j.value("significance_level", 0.05);
    cfg.min_improvement         = j.value("min_improvement", 0.02);
    cfg.max_duration            = std::chrono::hours{j.value("max_duration_hours", int64_t{72})};
    cfg.thompson_stop_threshold = j.value("thompson_stop_threshold", 0.95);
    return cfg;
}

} // anonymous namespace

// =============================================================================
// Construction / Destruction
// =============================================================================

ABTestManager::ABTestManager() = default;

ABTestManager::ABTestManager(HotReloadManager &reload_manager) : reload_manager_(&reload_manager) {}

ABTestManager::~ABTestManager() = default;

// =============================================================================
// Observability & persistence wiring
// =============================================================================

void ABTestManager::setStorageEngine(IStorageEngine *engine) {
    storage_engine_ = engine;
}

void ABTestManager::setMetricsCollector(observability::MetricsCollector *metrics) {
    metrics_collector_ = metrics;
}

void ABTestManager::start() {
    if (!storage_engine_) {
        return;
    }

    auto scan_result
        = storage_engine_->scanPrefix("ab_test::", [this](std::string_view /*key*/, std::string_view value) -> bool {
              try {
                  auto j = nlohmann::json::parse(value);

                  TestEntry entry;
                  entry.config           = configFromJson(j.at("config"));
                  entry.status           = statusFromString(j.value("status", "ACTIVE"));
                  entry.persisted_only   = true;
                  entry.treatment_loaded = false;
                  entry.loader_ptr       = nullptr;
                  entry.start_time       = std::chrono::system_clock::now();

                  // Restore variant accumulators
                  const auto &ctrl_j                    = j.at("control");
                  entry.control.metrics.sample_count    = ctrl_j.value("sample_count", size_t{0});
                  entry.control.metrics.success_count   = ctrl_j.value("success_count", size_t{0});
                  entry.control.metrics.success_rate    = ctrl_j.value("success_rate", 0.0);
                  entry.control.metrics.mean_latency_ms = ctrl_j.value("mean_latency_ms", 0.0);
                  entry.control.metrics.std_dev_latency = ctrl_j.value("std_dev_latency", 0.0);
                  entry.control.total_latency_ms        = ctrl_j.value("total_latency_ms", 0.0);
                  entry.control.sum_sq_latency          = ctrl_j.value("sum_sq_latency", 0.0);

                  const auto &trt_j                       = j.at("treatment");
                  entry.treatment.metrics.sample_count    = trt_j.value("sample_count", size_t{0});
                  entry.treatment.metrics.success_count   = trt_j.value("success_count", size_t{0});
                  entry.treatment.metrics.success_rate    = trt_j.value("success_rate", 0.0);
                  entry.treatment.metrics.mean_latency_ms = trt_j.value("mean_latency_ms", 0.0);
                  entry.treatment.metrics.std_dev_latency = trt_j.value("std_dev_latency", 0.0);
                  entry.treatment.total_latency_ms        = trt_j.value("total_latency_ms", 0.0);
                  entry.treatment.sum_sq_latency          = trt_j.value("sum_sq_latency", 0.0);

                  const std::string test_id = entry.config.test_id;
                  if (test_id.empty()) {
                      return true; // skip corrupt entries
                  }

                  std::lock_guard<std::mutex> lock(mutex_);
                  // GAP-FIX repeated_search: use emplace with the iterator from
                  // find() so we search the map only once instead of find + [].
                  auto pos = tests_.find(test_id);
                  if (pos == tests_.end()) {
                      spdlog::info("ABTestManager::start(): restored test '{}'", test_id);
                      tests_.emplace(test_id, std::move(entry));
                  }
              } catch (const std::exception &ex) {
                  spdlog::warn("ABTestManager::start(): failed to deserialise entry: {}", ex.what());
              }
              return true; // continue scan
          });

    if (!scan_result.has_value()) {
        spdlog::warn("ABTestManager::start(): storage scan failed");
    }
}

// =============================================================================
// Test lifecycle
// =============================================================================

bool ABTestManager::startTest(const ABModuleTestConfig &config, ModuleLoader &loader) {
    // First check: reject truly duplicate test IDs (not just persisted-only ones).
    {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = tests_.find(config.test_id);
        if (it != tests_.end() && !it->second.persisted_only) {
            spdlog::warn("ABTestManager: test '{}' already exists", config.test_id);
            return false;
        }
    }

    // Load the treatment binary OUTSIDE the mutex — loadModule may perform
    // slow disk or network I/O and must not hold the global lock.
    const std::string tkey = treatmentKey(config.module_name);
    auto load_result       = loader.loadModule(config.treatment_path, tkey);
    bool treatment_loaded  = load_result.success;
    if (treatment_loaded) {
        // GAP-FIX sensitive_data_logging: treatment_path is a filesystem path
        // and must not be broadcast at INFO level; log the module name only.
        spdlog::info("ABTestManager: treatment '{}' loaded successfully", config.module_name);
    } else {
        spdlog::warn("ABTestManager: treatment binary could not be loaded ({}); "
                     "all traffic will go to control",
                     load_result.errorMessage);
    }

    // Second check + insert/re-activate under lock.
    TestEntry entry_to_persist;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = tests_.find(config.test_id);
        if (it != tests_.end()) {
            if (!it->second.persisted_only) {
                // A concurrent thread registered the same ID while we loaded.
                if (treatment_loaded) {
                    loader.unloadModule(tkey);
                }
                spdlog::warn("ABTestManager: test '{}' was registered by another thread "
                             "during module load; discarding",
                             config.test_id);
                return false;
            }
            // Re-attach a persisted-only entry: update loader + config,
            // preserve accumulated metrics and the persisted status.
            it->second.config           = config;
            it->second.loader_ptr       = &loader;
            it->second.treatment_loaded = treatment_loaded;
            it->second.persisted_only   = false;
            if (it->second.status == ABTestStatus::ACTIVE) {
                spdlog::info("ABTestManager: test '{}' re-activated from persistence", config.test_id);
            } else {
                spdlog::info("ABTestManager: test '{}' restored from persistence with terminal status '{}'; "
                             "loader re-attached without changing status",
                             config.test_id, statusToString(it->second.status));
            }
            entry_to_persist = it->second;
        } else {
            TestEntry entry;
            entry.config           = config;
            entry.status           = ABTestStatus::ACTIVE;
            entry.start_time       = std::chrono::system_clock::now();
            entry.loader_ptr       = &loader;
            entry.treatment_loaded = treatment_loaded;
            entry.persisted_only   = false;
            spdlog::info("ABTestManager: test '{}' started", config.test_id);
            tests_[config.test_id] = entry;
            entry_to_persist       = entry;
        }
    }

    persistTestEntry(config.test_id, entry_to_persist);
    return true;
}

bool ABTestManager::promoteTest(const std::string &test_id) {
    // Capture the information we need under the lock, then do the I/O outside.
    std::string module_name = {};
    std::string treatment_path = {};

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
        auto hr   = reload_manager_->reloadModule(module_name, treatment_path);
        reload_ok = hr.success;
        if (!reload_ok) {
            spdlog::error("ABTestManager::promoteTest: hot-reload failed for '{}': {}", module_name, hr.errorMessage);
        }
    }

    TestEntry entry_snap;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = tests_.find(test_id);
        // Guard: only promote if the test is still ACTIVE.  A concurrent
        // cancelTest() could have already set the status to CANCELLED between
        // the first lock block and here; do not overwrite that terminal state.
        if (it != tests_.end() && it->second.status == ABTestStatus::ACTIVE) {
            it->second.status = ABTestStatus::PROMOTED;
            entry_snap        = it->second;
        }
    }

    persistTestEntry(test_id, entry_snap);
    spdlog::info("ABTestManager: test '{}' promoted (module '{}')", test_id, module_name);
    return reload_ok;
}

bool ABTestManager::rollbackTest(const std::string &test_id) {
    TestEntry entry_snap;
    {
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
        entry_snap        = it->second;
    }

    persistTestEntry(test_id, entry_snap);
    spdlog::info("ABTestManager: test '{}' rolled back", test_id);
    return true;
}

void ABTestManager::cancelTest(const std::string &test_id) {
    TestEntry entry_snap;
    bool did_cancel = false;
    {
        std::lock_guard<std::mutex> lock(mutex_);

        auto it = tests_.find(test_id);
        if (it == tests_.end() || it->second.status != ABTestStatus::ACTIVE) {
            return;
        }

        unloadTreatment(it->second);
        it->second.status = ABTestStatus::CANCELLED;
        entry_snap        = it->second;
        did_cancel        = true;
    }

    if (did_cancel) {
        persistTestEntry(test_id, entry_snap);
        spdlog::info("ABTestManager: test '{}' cancelled", test_id);
    }
}

// =============================================================================
// Traffic routing
// =============================================================================

bool ABTestManager::shouldUseTreatment(const std::string &test_id, const std::string &request_key) const {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = tests_.find(test_id);
    if (it == tests_.end() || it->second.status != ABTestStatus::ACTIVE) {
        return false;
    }

    size_t hash       = hashRequestKey(request_key);
    double normalized = static_cast<double>(hash % 10000) / 10000.0;
    return normalized < it->second.config.traffic_split;
}

bool ABTestManager::isTreatmentLoaded(const std::string &test_id) const {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = tests_.find(test_id);
    return it != tests_.end() && it->second.treatment_loaded;
}

// =============================================================================
// Metrics recording
// =============================================================================

void ABTestManager::recordOutcome(const std::string &test_id, bool is_treatment, bool success, double latency_ms) {
    // Values captured under the mutex for out-of-lock use.
    bool emit_metrics   = false;
    bool emit_success   = false;
    double emit_p99_lat = 0.0;

    bool check_thompson = false;
    size_t ctrl_success = 0, ctrl_total = 0;
    size_t trt_success = 0, trt_total = 0;
    double thompson_threshold = 0.0;
    size_t min_samples        = 0;

    bool do_persist = false;
    TestEntry entry_snap;

    {
        std::lock_guard<std::mutex> lock(mutex_);

        auto it = tests_.find(test_id);
        if (it == tests_.end() || it->second.status != ABTestStatus::ACTIVE) {
            return;
        }

        VariantData &vd     = is_treatment ? it->second.treatment : it->second.control;
        ABVariantMetrics &m = vd.metrics;

        m.sample_count++;
        if (success) {
            m.success_count++;
        }
        m.success_rate = static_cast<double>(m.success_count) / m.sample_count;

        // Online mean and variance (Welford's algorithm) for latency.
        if (latency_ms >= 0.0) {
            vd.total_latency_ms += latency_ms;
            double old_mean   = m.mean_latency_ms;
            m.mean_latency_ms = vd.total_latency_ms / m.sample_count;
            vd.sum_sq_latency += (latency_ms - old_mean) * (latency_ms - m.mean_latency_ms);
            if (m.sample_count > 1) {
                // Clamp to >= 0 to guard against tiny negative values from
                // floating-point rounding that would produce NaN via sqrt.
                double variance   = std::max(0.0, vd.sum_sq_latency / (m.sample_count - 1));
                m.std_dev_latency = std::sqrt(variance);
            }
        }

        // Capture values for MetricsCollector emission (outside mutex).
        if (metrics_collector_) {
            emit_metrics = true;
            emit_success = success;
            // p99 estimate via normal approximation: μ + 2.3263σ
            emit_p99_lat = m.mean_latency_ms + 2.3263 * m.std_dev_latency;
        }

        // Capture values for Thompson Sampling check (outside mutex).
        const ABVariantMetrics &ctrl_m = it->second.control.metrics;
        const ABVariantMetrics &trt_m  = it->second.treatment.metrics;
        ctrl_success                   = ctrl_m.success_count;
        ctrl_total                     = ctrl_m.sample_count;
        trt_success                    = trt_m.success_count;
        trt_total                      = trt_m.sample_count;
        thompson_threshold             = it->second.config.thompson_stop_threshold;
        min_samples                    = it->second.config.min_samples;
        check_thompson = (thompson_threshold > 0.0 && ctrl_total >= min_samples && trt_total >= min_samples);

        // Periodic snapshot persist: every 100 recordOutcome() calls.
        it->second.outcome_count++;
        if (storage_engine_ && (it->second.outcome_count % 100 == 0)) {
            do_persist = true;
            entry_snap = it->second;
        }
    }

    // -----------------------------------------------------------------------
    // Out-of-lock work: no mutex held below this line.
    // -----------------------------------------------------------------------

    // 1. Emit per-variant counters and gauges to MetricsCollector.
    if (emit_metrics && metrics_collector_) {
        const std::string variant = is_treatment ? "treatment" : "control";
        const std::string prefix  = "ab_test." + test_id + "." + variant;
        metrics_collector_->addCounter(prefix + ".requests", 1);
        if (emit_success) {
            metrics_collector_->addCounter(prefix + ".conversions", 1);
        }
        metrics_collector_->setGauge(prefix + ".latency_p99", emit_p99_lat);
    }

    // 2. Persist periodic snapshot.
    if (do_persist) {
        persistTestEntry(test_id, entry_snap);
    }

    // 3. Bayesian Thompson Sampling auto-stop.
    if (check_thompson) {
        const size_t ctrl_failure = ctrl_total - ctrl_success;
        const size_t trt_failure  = trt_total - trt_success;
        double prob               = thompsonProbTreatmentWins(ctrl_success, ctrl_failure, trt_success, trt_failure);
        if (prob > thompson_threshold) {
            spdlog::info("ABTestManager: Thompson auto-stop: treatment wins '{}' "
                         "(p_treatment_wins={:.4f})",
                         test_id, prob);
            promoteTest(test_id);
        } else if ((1.0 - prob) > thompson_threshold) {
            spdlog::info("ABTestManager: Thompson auto-stop: control wins '{}' "
                         "(p_control_wins={:.4f})",
                         test_id, 1.0 - prob);
            rollbackTest(test_id);
        }
    }
}

// =============================================================================
// Statistical evaluation
// =============================================================================

ABModuleTestResult ABTestManager::evaluateTest(const std::string &test_id) const {
    std::lock_guard<std::mutex> lock(mutex_);

    ABModuleTestResult result;
    result.test_id = test_id;

    auto it = tests_.find(test_id);
    if (it == tests_.end()) {
        return result;
    }

    const ABVariantMetrics &ctrl = it->second.control.metrics;
    const ABVariantMetrics &trt  = it->second.treatment.metrics;

    result.sample_size_control       = ctrl.sample_count;
    result.sample_size_treatment     = trt.sample_count;
    result.control_success_rate      = ctrl.success_rate;
    result.treatment_success_rate    = trt.success_rate;
    result.improvement               = trt.success_rate - ctrl.success_rate;
    result.control_mean_latency_ms   = ctrl.mean_latency_ms;
    result.treatment_mean_latency_ms = trt.mean_latency_ms;

    const size_t min_n = it->second.config.min_samples;
    if (ctrl.sample_count >= min_n && trt.sample_count >= min_n) {
        double z              = calculateZStatistic(ctrl, trt);
        result.p_value        = calculatePValue(z);
        result.is_significant = (result.p_value < it->second.config.significance_level);
    }

    return result;
}

// =============================================================================
// Queries
// =============================================================================

ABTestStatus ABTestManager::getTestStatus(const std::string &test_id) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = tests_.find(test_id);
    return (it != tests_.end()) ? it->second.status : ABTestStatus::CANCELLED;
}

std::vector<std::string> ABTestManager::getActiveTests() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<std::string> active = {};

    for (const auto &[id, entry] : tests_) {
        if (entry.status == ABTestStatus::ACTIVE) {
            active.push_back(id);
        }
    }
    return active;
}

ABVariantMetrics ABTestManager::getControlMetrics(const std::string &test_id) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = tests_.find(test_id);
    return (it != tests_.end()) ? it->second.control.metrics : ABVariantMetrics{};
}

ABVariantMetrics ABTestManager::getTreatmentMetrics(const std::string &test_id) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = tests_.find(test_id);
    return (it != tests_.end()) ? it->second.treatment.metrics : ABVariantMetrics{};
}

std::vector<ABTestMetricRow> ABTestManager::exportMetricsSnapshot() const {
    std::lock_guard<std::mutex> lock(mutex_);

    std::vector<ABTestMetricRow> rows = {};

    rows.reserve(tests_.size() * 2);

    for (const auto &[id, entry] : tests_) {
        auto makeRow = [&](const std::string &variant, const ABVariantMetrics &m) {
            ABTestMetricRow row;
            row.test_id         = id;
            row.variant         = variant;
            row.requests        = m.sample_count;
            row.conversions     = m.success_count;
            row.success_rate    = m.success_rate;
            row.mean_latency_ms = m.mean_latency_ms;
            row.latency_p99_ms  = m.mean_latency_ms + 2.3263 * m.std_dev_latency;
            row.status          = entry.status;
            return row;
        };
        rows.push_back(makeRow("control", entry.control.metrics));
        rows.push_back(makeRow("treatment", entry.treatment.metrics));
    }
    return rows;
}

// =============================================================================
// Helpers
// =============================================================================

/*static*/ std::string ABTestManager::treatmentKey(const std::string &module_name) {
    return module_name + "__ab_treatment__";
}

void ABTestManager::unloadTreatment(TestEntry &entry) {
    if (entry.treatment_loaded && entry.loader_ptr) {
        entry.loader_ptr->unloadModule(treatmentKey(entry.config.module_name));
        entry.treatment_loaded = false;
    }
}

// =============================================================================
// Persistence helpers
// =============================================================================

void ABTestManager::persistTestEntry(const std::string &test_id, const TestEntry &entry) const {
    if (!storage_engine_) {
        return;
    }
    if (test_id.empty()) {
        return;
    }

    // Serialise variant accumulator data.
    auto variantJson = [](const VariantData &vd) -> nlohmann::json {
        return {{"sample_count", vd.metrics.sample_count},
                {"success_count", vd.metrics.success_count},
                {"success_rate", vd.metrics.success_rate},
                {"mean_latency_ms", vd.metrics.mean_latency_ms},
                {"std_dev_latency", vd.metrics.std_dev_latency},
                {"total_latency_ms", vd.total_latency_ms},
                {"sum_sq_latency", vd.sum_sq_latency}};
    };

    try {
        nlohmann::json j;
        j["config"]    = configToJson(entry.config);
        j["status"]    = statusToString(entry.status);
        j["control"]   = variantJson(entry.control);
        j["treatment"] = variantJson(entry.treatment);

        const std::string key   = "ab_test::" + test_id;
        const std::string value = j.dump();
        auto res                = storage_engine_->put(key, value);
        if (!res.has_value()) {
            spdlog::warn("ABTestManager: failed to persist test '{}': {}", test_id, res.error().message());
        }
    } catch (const std::exception &ex) {
        spdlog::error("ABTestManager: serialisation error for test '{}': {}", test_id, ex.what());
    }
}

// =============================================================================
// Statistics helpers
// =============================================================================

/*static*/ double ABTestManager::calculateZStatistic(const ABVariantMetrics &ctrl, const ABVariantMetrics &trt) {
    // Two-proportion z-test.
    if (ctrl.sample_count == 0 || trt.sample_count == 0) {
        return 0.0;
    }

    double p1 = ctrl.success_rate;
    double p2 = trt.success_rate;
    double n1 = static_cast<double>(ctrl.sample_count);
    double n2 = static_cast<double>(trt.sample_count);

    // Pooled proportion.
    double p_pool = (p1 * n1 + p2 * n2) / (n1 + n2);
    double denom  = std::sqrt(p_pool * (1.0 - p_pool) * (1.0 / n1 + 1.0 / n2));
    if (denom < 1e-10) {
        return 0.0;
    }

    return (p2 - p1) / denom;
}

/*static*/ double ABTestManager::calculatePValue(double z_statistic) {
    // Two-tailed p-value via complementary error function approximation
    // (Abramowitz & Stegun 7.1.26).
    double abs_z = std::abs(z_statistic);
    double x     = abs_z / std::sqrt(2.0);

    double t    = 1.0 / (1.0 + 0.3275911 * x);
    double poly = t * (0.254829592 + t * (-0.284496736 + t * (1.421413741 + t * (-1.453152027 + t * 1.061405429))));
    double erfc_approx = poly * std::exp(-x * x);

    return std::min(1.0, erfc_approx); // erfc(|z|/√2) ≈ two-tailed p-value
}

/*static*/ size_t ABTestManager::hashRequestKey(const std::string &key) {
    std::hash<std::string> hasher;
    return hasher(key);
}

// =============================================================================
// Bayesian Thompson Sampling helper
// =============================================================================

/*static*/ double ABTestManager::thompsonProbTreatmentWins(size_t ctrl_success, size_t ctrl_failure, size_t trt_success,
                                                           size_t trt_failure) {
    // Use Beta(α=successes+1, β=failures+1) posteriors (uniform Beta(1,1) prior).
    // Normal approximation for P(Beta_treatment > Beta_control):
    //   μ  = α / (α + β)
    //   σ² = αβ / ((α+β)² (α+β+1))
    //   P(trt > ctrl) ≈ Φ( (μ_trt - μ_ctrl) / sqrt(σ²_trt + σ²_ctrl) )

    const double a1 = static_cast<double>(ctrl_success) + 1.0;
    const double b1 = static_cast<double>(ctrl_failure) + 1.0;
    const double a2 = static_cast<double>(trt_success) + 1.0;
    const double b2 = static_cast<double>(trt_failure) + 1.0;

    const double n1   = a1 + b1;
    const double n2   = a2 + b2;
    const double mu1  = a1 / n1;
    const double mu2  = a2 / n2;
    const double var1 = (a1 * b1) / (n1 * n1 * (n1 + 1.0));
    const double var2 = (a2 * b2) / (n2 * n2 * (n2 + 1.0));

    const double total_var = var1 + var2;
    if (total_var < 1e-15) {
        return (mu2 > mu1) ? 1.0 : 0.0;
    }

    const double z = (mu2 - mu1) / std::sqrt(total_var);
    // Standard normal CDF: Φ(z) = 0.5 * (1 + erf(z / √2))
    return 0.5 * (1.0 + std::erf(z / std::sqrt(2.0)));
}

} // namespace modules
} // namespace themis
