/**
 * @file ab_test_manager.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


// A/B testing framework with module swapping for ThemisDB base module.
//
// Enables running two variants of a plugin binary in parallel for
// controlled experiments without a database restart.
//
// Typical usage:
// @code
//   ABTestManager ab_mgr(reload_manager);
//
//   ABModuleTestConfig cfg;
//   cfg.test_id        = "storage_v2_trial";
//   cfg.module_name    = "themis_storage";
//   cfg.control_path   = "/plugins/themis_storage_v1.so";
//   cfg.treatment_path = "/plugins/themis_storage_v2.so";
//   cfg.traffic_split  = 0.1;   // 10 % of requests go to v2
//
//   ab_mgr.startTest(cfg, loader);
//
//   // Per request:
//   bool use_v2 = ab_mgr.shouldUseTreatment("storage_v2_trial", request_id);
//   bool ok = dispatch_to_module(use_v2 ? ab_mgr.treatmentKey("themis_storage")
//                                       : "themis_storage");
//   ab_mgr.recordOutcome("storage_v2_trial", use_v2, ok, latency_ms);
//
//   // After collecting enough samples:
//   auto result = ab_mgr.evaluateTest("storage_v2_trial");
//   if (result.is_significant && result.improvement > cfg.min_improvement) {
//       ab_mgr.promoteTest("storage_v2_trial");  // hot-swaps v1 → v2
//   } else {
//       ab_mgr.rollbackTest("storage_v2_trial"); // unloads v2, keeps v1
//   }
// @endcode
//
// See src/base/ROADMAP.md – Long-term: A/B testing framework using module swapping

#pragma once

#include "themis/base/hot_reload_manager.h"
#include "themis/base/module_loader.h"

#include <chrono>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

// Forward declarations to avoid heavy transitive includes.
namespace themis { class IStorageEngine; }
namespace themis { namespace observability { class MetricsCollector; } }

namespace themis {
namespace modules {

// =============================================================================
// ABTestStatus
// =============================================================================

/**
 * @brief Lifecycle status of one A/B module test.
 */
enum class ABTestStatus {
    ACTIVE,       ///< Test running: traffic is being split
    PROMOTED,     ///< Treatment was promoted to production via hot-reload
    ROLLED_BACK,  ///< Treatment was unloaded; control remains in place
    CANCELLED     ///< Test was manually cancelled
};

// =============================================================================
// ABModuleTestConfig
// =============================================================================

/**
 * @brief Configuration for a single A/B module test.
 */
struct ABModuleTestConfig {
    std::string test_id;           ///< Unique test identifier
    std::string module_name;       ///< Logical module name already registered
    std::string control_path;      ///< Filesystem path to the control binary
    std::string treatment_path;    ///< Filesystem path to the treatment binary
    double      traffic_split      = 0.1;  ///< Fraction routed to treatment [0, 1]
    size_t      min_samples        = 100;  ///< Min samples required for evaluation
    double      significance_level = 0.05; ///< p-value threshold
    double      min_improvement    = 0.02; ///< Minimum improvement to consider promoting
    std::chrono::hours max_duration{72};   ///< Maximum test duration
    /// Bayesian Thompson Sampling auto-stop threshold.
    /// When P(treatment beats control) exceeds this value the test is auto-concluded.
    /// Set to 0.0 to disable auto-stop.
    double      thompson_stop_threshold = 0.95;
};

// =============================================================================
// ABVariantMetrics
// =============================================================================

/**
 * @brief Aggregated metrics for one variant (control or treatment).
 */
struct ABVariantMetrics {
    size_t sample_count    = 0;    ///< Total requests dispatched to this variant
    size_t success_count   = 0;    ///< Requests that returned a successful outcome
    double success_rate    = 0.0;  ///< success_count / sample_count
    double mean_latency_ms = 0.0;  ///< Running mean of observed latencies
    double std_dev_latency = 0.0;  ///< Sample standard deviation of latencies
};

// =============================================================================
// ABModuleTestResult
// =============================================================================

/**
 * @brief Statistical evaluation snapshot for a test.
 */
struct ABModuleTestResult {
    std::string test_id;
    size_t  sample_size_control       = 0;    ///< Control group sample size
    size_t  sample_size_treatment     = 0;    ///< Treatment group sample size
    double  control_success_rate      = 0.0;  ///< Control success proportion
    double  treatment_success_rate    = 0.0;  ///< Treatment success proportion
    double  improvement               = 0.0;  ///< treatment − control success rate
    double  p_value                   = 1.0;  ///< Two-tailed p-value
    bool    is_significant            = false;///< p_value < significance_level
    double  control_mean_latency_ms   = 0.0;  ///< Mean latency of control
    double  treatment_mean_latency_ms = 0.0;  ///< Mean latency of treatment
};

// =============================================================================
// ABTestMetricRow
// =============================================================================

/**
 * @brief A flat metric snapshot for one variant of one A/B test.
 *
 * Returned by ABTestManager::exportMetricsSnapshot() for admin API consumption.
 */
struct ABTestMetricRow {
    std::string  test_id;
    std::string  variant;          ///< "control" or "treatment"
    size_t       requests    = 0;  ///< Total requests routed to this variant
    size_t       conversions = 0;  ///< Successful outcomes
    double       success_rate    = 0.0; ///< conversions / requests
    double       mean_latency_ms = 0.0; ///< Running mean latency
    double       latency_p99_ms  = 0.0; ///< Estimated p99 (mean + 2.33 σ)
    ABTestStatus status          = ABTestStatus::ACTIVE;
};

// =============================================================================
// ABTestManager
// =============================================================================

/**
 * @brief Manages A/B tests where each test runs two variants of a module binary
 *        simultaneously and routes requests between them.
 *
 * Thread safety: all public methods are thread-safe.
 */
class ABTestManager {
public:
    // -------------------------------------------------------------------------
    // Construction
    // -------------------------------------------------------------------------

    ABTestManager();

    /**
     * @brief Construct with an optional HotReloadManager for promotion support.
     *
     * When a HotReloadManager is provided, promoteTest() will call
     * HotReloadManager::reloadModule() to atomically swap the control binary
     * with the treatment binary without a database restart.
     */
    explicit ABTestManager(HotReloadManager& reload_manager);

    ~ABTestManager();

    ABTestManager(const ABTestManager&)            = delete;
    ABTestManager& operator=(const ABTestManager&) = delete;

    // -------------------------------------------------------------------------
    // Observability & persistence wiring
    // -------------------------------------------------------------------------

    /**
     * @brief Attach a storage engine for RocksDB persistence (optional).
     *
     * When set, test configs and metrics are persisted under the key prefix
     * @c "ab_test::" so they survive server restarts.  Call before start().
     *
     * @param engine  Non-owning pointer to an IStorageEngine implementation.
     *                Pass nullptr to disable persistence.
     */
    void setStorageEngine(IStorageEngine* engine);

    /**
     * @brief Attach a MetricsCollector for observability export (optional).
     *
     * When set, recordOutcome() emits per-variant counters and gauges to the
     * collector *outside* the internal mutex so the hot-path is not blocked.
     *
     * @param metrics  Non-owning pointer to a MetricsCollector instance.
     *                 Pass nullptr to disable metrics emission.
     */
    void setMetricsCollector(observability::MetricsCollector* metrics);

    /**
     * @brief Load previously persisted test entries from storage.
     *
     * Scans the @c "ab_test::" key prefix in the attached storage engine and
     * restores config and accumulated metrics for each entry.  Active tests are
     * restored with @c treatment_loaded = false; callers should re-call
     * startTest() with the same test_id to re-attach a module loader and reload
     * the treatment binary while preserving the accumulated metrics.
     *
     * No-op when no storage engine has been set.
     */
    void start();

    // -------------------------------------------------------------------------
    // Test lifecycle
    // -------------------------------------------------------------------------

    /**
     * @brief Register and start an A/B test.
     *
     * Loads the treatment binary under treatmentKey(config.module_name) so
     * both variants are simultaneously resident.  If the treatment binary
     * cannot be loaded (e.g. file not found), the test is still registered
     * and routing will always fall back to control until the binary becomes
     * available.
     *
     * @param config  Test configuration.
     * @param loader  ModuleLoader to use for loading the treatment binary.
     * @return true if the test was registered; false if a test with the same
     *         test_id already exists.
     */
    bool startTest(const ABModuleTestConfig& config, ModuleLoader& loader);

    /**
     * @brief Promote the treatment variant to production via hot-reload.
     *
     * If a HotReloadManager was provided at construction, calls
     * HotReloadManager::reloadModule(module_name, treatment_path) to
     * atomically replace the running control binary with the treatment binary.
     * The treatment slot is then unloaded and the test status becomes PROMOTED.
     *
     * If no HotReloadManager was configured, the status is still set to
     * PROMOTED (useful for tests that manage module lifetimes externally).
     *
     * @param test_id  Test identifier.
     * @return true on success, false if the test was not found or not ACTIVE.
     */
    bool promoteTest(const std::string& test_id);

    /**
     * @brief Roll back: unload the treatment module and keep the control.
     *
     * Unloads the treatment binary and transitions the test to ROLLED_BACK.
     * The control module remains running undisturbed.
     *
     * @param test_id  Test identifier.
     * @return true on success, false if the test was not found or not ACTIVE.
     */
    bool rollbackTest(const std::string& test_id);

    /**
     * @brief Cancel a test without making a promote/rollback decision.
     *
     * Unloads the treatment module if it was loaded and marks the test
     * CANCELLED.  No-op if the test is not currently ACTIVE.
     *
     * @param test_id  Test identifier.
     */
    void cancelTest(const std::string& test_id);

    // -------------------------------------------------------------------------
    // Traffic routing
    // -------------------------------------------------------------------------

    /**
     * @brief Determine which variant a request should be routed to.
     *
     * Uses a stable hash of @p request_key so a given key is always assigned
     * to the same variant (deterministic routing).
     *
     * @param test_id      Test identifier.
     * @param request_key  Any string (user ID, session ID, query hash, …).
     * @return true  → route to treatment,
     *         false → route to control (also returned when the test is not
     *                 ACTIVE or not found).
     */
    bool shouldUseTreatment(const std::string& test_id,
                             const std::string& request_key) const;

    /**
     * @brief Check whether the treatment binary was successfully loaded.
     *
     * @return true if the treatment is loaded, false otherwise.
     */
    bool isTreatmentLoaded(const std::string& test_id) const;

    // -------------------------------------------------------------------------
    // Metrics recording
    // -------------------------------------------------------------------------

    /**
     * @brief Record the outcome of a single dispatched request.
     *
     * @param test_id      Test identifier.
     * @param is_treatment true = outcome belongs to treatment group.
     * @param success      Whether the request completed successfully.
     * @param latency_ms   Optional observed latency in milliseconds.
     */
    void recordOutcome(const std::string& test_id,
                       bool is_treatment,
                       bool success,
                       double latency_ms = 0.0);

    // -------------------------------------------------------------------------
    // Statistical evaluation
    // -------------------------------------------------------------------------

    /**
     * @brief Compute a statistical evaluation snapshot for the test.
     *
     * Performs a two-proportion z-test on the success rates once both groups
     * have reached min_samples.  Returns an empty result if the test is not
     * found.
     *
     * @param test_id  Test identifier.
     * @return Statistical result including p-value and significance flag.
     */
    ABModuleTestResult evaluateTest(const std::string& test_id) const;

    // -------------------------------------------------------------------------
    // Queries
    // -------------------------------------------------------------------------

    ABTestStatus             getTestStatus(const std::string& test_id) const;
    std::vector<std::string> getActiveTests() const;
    ABVariantMetrics         getControlMetrics(const std::string& test_id) const;
    ABVariantMetrics         getTreatmentMetrics(const std::string& test_id) const;

    /**
     * @brief Export a flat metric snapshot for all known tests.
     *
     * Returns two rows per test (control + treatment) with request/conversion
     * counts, success rates, mean latency and estimated p99.  Intended for the
     * admin API.
     *
     * @return One ABTestMetricRow per variant per test.
     */
    std::vector<ABTestMetricRow> exportMetricsSnapshot() const;

    // -------------------------------------------------------------------------
    // Helpers
    // -------------------------------------------------------------------------

    /**
     * @brief Internal module key used to register the treatment binary.
     *
     * The treatment binary is loaded under this derived name so it can
     * coexist with the control binary in the same ModuleLoader.
     *
     * @param module_name  Logical module name (e.g. "themis_storage").
     * @return             Derived key (e.g. "themis_storage__ab_treatment__").
     */
    static std::string treatmentKey(const std::string& module_name);

private:
    // -------------------------------------------------------------------------
    // Internal per-test state
    // -------------------------------------------------------------------------

    struct VariantData {
        ABVariantMetrics metrics;
        // Running accumulators for online mean/variance of latency.
        double total_latency_ms = 0.0;
        double sum_sq_latency   = 0.0; ///< Sum of squared deviations (Welford)
    };

    struct TestEntry {
        ABModuleTestConfig config;
        ABTestStatus       status            = ABTestStatus::ACTIVE;
        bool               treatment_loaded  = false;
        bool               persisted_only    = false; ///< Restored from storage; not yet re-activated
        std::chrono::system_clock::time_point start_time;
        ModuleLoader*      loader_ptr        = nullptr; ///< Non-owning
        VariantData        control;
        VariantData        treatment;
        size_t             outcome_count     = 0; ///< Number of recordOutcome() calls (for periodic persist)
    };

    mutable std::mutex mutex_;
    std::unordered_map<std::string, TestEntry> tests_;
    HotReloadManager*                reload_manager_    = nullptr; ///< Optional, non-owning
    IStorageEngine*                  storage_engine_    = nullptr; ///< Optional, non-owning
    observability::MetricsCollector* metrics_collector_ = nullptr; ///< Optional, non-owning

    // -------------------------------------------------------------------------
    // Statistics helpers
    // -------------------------------------------------------------------------

    /// Two-proportion z-test statistic.
    static double calculateZStatistic(const ABVariantMetrics& ctrl,
                                      const ABVariantMetrics& trt);

    /// Approximate two-tailed p-value from z-statistic (A&S 26.2.17).
    static double calculatePValue(double z_statistic);

    /// Stable hash for deterministic request routing.
    static size_t hashRequestKey(const std::string& key);

    /// Unload the treatment binary for a test entry (caller must hold mutex_).
    void unloadTreatment(TestEntry& entry);

    // -------------------------------------------------------------------------
    // Persistence helpers
    // -------------------------------------------------------------------------

    /// Serialize and write @p entry to storage under key @c "ab_test::<test_id>".
    /// No-op when storage_engine_ is null.  Must NOT be called while holding mutex_.
    void persistTestEntry(const std::string& test_id, const TestEntry& entry) const;

    // -------------------------------------------------------------------------
    // Bayesian helpers
    // -------------------------------------------------------------------------

    /**
     * @brief Estimate P(treatment_success_rate > control_success_rate) via the
     *        normal approximation of the Beta-Binomial posterior.
     *
     * Uses uniform Beta(1,1) priors.  Returns a value in [0, 1].
     *
     * @param ctrl_success  Control successes.
     * @param ctrl_failure  Control failures.
     * @param trt_success   Treatment successes.
     * @param trt_failure   Treatment failures.
     */
    static double thompsonProbTreatmentWins(size_t ctrl_success, size_t ctrl_failure,
                                            size_t trt_success,  size_t trt_failure);
};

} // namespace modules
} // namespace themis
