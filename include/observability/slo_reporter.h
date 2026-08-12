/**
 * @file slo_reporter.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.13
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <chrono>
#include <deque>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <vector>
#include <nlohmann/json.hpp>

namespace themis {
namespace observability {

using json = nlohmann::json;

// ---------------------------------------------------------------------------
// SLO configuration and status types
// ---------------------------------------------------------------------------

/**
 * @brief Classification of a burn-rate alert by urgency.
 *
 * Follows the Google SRE multi-window alerting model:
 *
 * | Level   | Burn-rate | Short window | Long window | Severity |
 * |---------|-----------|-------------|-------------|----------|
 * | FAST    | 14.4×     | 1 h         | 5 min       | critical |
 * | MEDIUM  | 6×        | 6 h         | 30 min      | warning  |
 * | SLOW    | 3×        | 24 h        | 2 h         | info     |
 */
enum class BurnRateLevel {
    FAST,   ///< 14.4× – budget exhausted in ~2 h (critical)
    MEDIUM, ///< 6×    – budget exhausted in ~5 h (warning)
    SLOW    ///< 3×    – budget exhausted in ~10 h (info)
};

/** Returns the numeric burn-rate multiplier for a level. */
inline double burnRateMultiplier(BurnRateLevel level) noexcept {
    switch (level) {
        case BurnRateLevel::FAST:   return 14.4;
        case BurnRateLevel::MEDIUM: return 6.0;
        case BurnRateLevel::SLOW:   return 3.0;
    }
    return 1.0;
}

/** Returns the severity label string for a burn-rate level. */
inline const char* burnRateSeverity(BurnRateLevel level) noexcept {
    switch (level) {
        case BurnRateLevel::FAST:   return "critical";
        case BurnRateLevel::MEDIUM: return "warning";
        case BurnRateLevel::SLOW:   return "info";
    }
    return "info";
}

/**
 * @brief Definition of a single Service Level Objective (SLO).
 *
 * An SLO specifies a target reliability level for a named service indicator.
 * The SloReporter measures actual reliability against this target and tracks
 * how fast the error budget is being consumed.
 *
 * ### Example
 * ```cpp
 * SloDefinition slo;
 * slo.name      = "query_availability";
 * slo.objective = 0.999;   // 99.9 % → error budget = 0.1 %
 * slo.window    = std::chrono::hours(24);
 * reporter.registerSlo(slo);
 * ```
 */
struct SloDefinition {
    /// Unique SLO name (e.g. `"query_availability"`, `"write_latency_p99"`).
    std::string name;

    /**
     * @brief The SLO target expressed as a fraction in [0, 1].
     *
     * For an availability SLO of 99.9 % this is 0.999.
     * The error budget is `1 - objective` (e.g. 0.001 for 99.9 %).
     */
    double objective{0.999};

    /**
     * @brief Rolling evaluation window.
     *
     * Requests older than @c window are expired from the sliding buffer.
     * Default: 24 hours (a common SLO compliance window).
     */
    std::chrono::seconds window{std::chrono::hours(24)};
};

/**
 * @brief A single firing burn-rate alert.
 */
struct BurnRateAlert {
    BurnRateLevel level;    ///< Urgency classification
    double burn_rate;       ///< Measured burn rate (ratio to target error rate)
    double window_hours;    ///< Duration of the measurement window in hours
    std::string severity;   ///< "critical" / "warning" / "info"
    std::string message;    ///< Human-readable alert message
};

/**
 * @brief Current compliance status snapshot for one SLO.
 */
struct SloStatus {
    std::string name;               ///< SLO name
    double objective;               ///< Target (e.g. 0.999)

    /// Measured SLI: fraction of good requests in the evaluation window.
    double current_sli{1.0};

    /// Total error budget as a fraction (= 1 − objective).
    double error_budget_total{0.0};

    /// Remaining error budget as a fraction of the total budget.
    /// 1.0 = fully intact; 0.0 = exhausted.
    double error_budget_remaining{1.0};

    /// Requests counted in the current window (good + bad).
    uint64_t total_requests{0};

    /// Number of bad (error) requests in the current window.
    uint64_t error_requests{0};

    /// Current burn rate: actual_error_rate / target_error_rate.
    /// A value of 1.0 means budget is consumed at exactly the SLO-allowed rate.
    double burn_rate{0.0};

    /// True when current_sli >= objective (SLO is being met).
    bool slo_met{true};

    /// Burn-rate alerts that are currently firing for this SLO.
    std::vector<BurnRateAlert> active_burn_rate_alerts;

    /** Serialize to JSON. */
    json toJson() const;
};

// ---------------------------------------------------------------------------
// SloReporter
// ---------------------------------------------------------------------------

/**
 * @brief SLO/SLA compliance reporter with multi-window burn-rate alerting.
 *
 * ### Overview
 * @c SloReporter tracks user-defined SLOs by maintaining a sliding ring-buffer
 * of request observations per SLO.  On each call to @c record() the caller
 * classifies the request as good (successful) or bad (error).  The reporter
 * continuously computes:
 *
 * - **SLI** (Service Level Indicator): `good_requests / total_requests`
 * - **Error budget remaining**: how much of the allowed error rate is left
 * - **Burn rate**: `actual_error_rate / target_error_rate`
 * - **Burn-rate alerts**: FAST (14.4×), MEDIUM (6×), SLOW (3×) levels
 *
 * Metrics are published to @c MetricsCollector under the
 * `themis_slo_*` namespace so they can be scraped by Prometheus.
 *
 * ### Thread safety
 * All public methods are thread-safe.
 *
 * ### Usage
 * ```cpp
 * SloReporter reporter;
 *
 * SloDefinition slo;
 * slo.name      = "query_availability";
 * slo.objective = 0.999;
 * slo.window    = std::chrono::hours(24);
 * reporter.registerSlo(slo);
 *
 * // For every request:
 * reporter.record("query_availability", !request_failed);
 *
 * // Periodic or on-demand:
 * reporter.publishMetrics();
 * auto report = reporter.generateReport();
 * ```
 */
class SloReporter {
public:
    /**
     * @brief Configuration for the SloReporter.
     */
    struct Config {
        /**
         * Maximum number of request samples retained per SLO per window.
         * Older samples are evicted to bound memory usage.
         * Default: 100,000.
         */
        size_t max_samples_per_slo{100'000};
    };

    explicit SloReporter();
    explicit SloReporter(const Config& config);
    ~SloReporter();

    // Non-copyable
    SloReporter(const SloReporter&) = delete;
    SloReporter& operator=(const SloReporter&) = delete;

    /**
     * @brief Register a new SLO.
     *
     * If an SLO with the same name already exists it is replaced.
     *
     * @param slo  SLO definition to register.
     */
    void registerSlo(const SloDefinition& slo);

    /**
     * @brief Record one request observation for a named SLO.
     *
     * @param slo_name     Name of the SLO (must have been registered via @c registerSlo).
     * @param good_request @c true if the request satisfied the SLI, @c false otherwise.
     * @param timestamp    Observation timestamp (defaults to now).
     *
     * Observations for unknown SLO names are silently ignored.
     */
    void record(const std::string& slo_name, bool good_request,
                std::chrono::system_clock::time_point timestamp =
                    std::chrono::system_clock::now());

    /**
     * @brief Return the current compliance status for one SLO.
     *
     * @throws std::out_of_range if the SLO name is not registered.
     */
    SloStatus getStatus(const std::string& slo_name) const;

    /**
     * @brief Return compliance status snapshots for all registered SLOs.
     */
    std::vector<SloStatus> getAllStatuses() const;

    /**
     * @brief Publish SLO metrics to @c MetricsCollector.
     *
     * Emits the following gauge families:
     * - `themis_slo_current_sli{slo="<name>"}` – current SLI value
     * - `themis_slo_error_budget_remaining{slo="<name>"}` – fraction remaining (0–1)
     * - `themis_slo_burn_rate{slo="<name>"}` – current burn rate
     * - `themis_slo_met{slo="<name>"}` – 1 if SLO is met, 0 otherwise
     */
    void publishMetrics() const;

    /**
     * @brief Generate a human-readable compliance report.
     */
    std::string generateReport() const;

    /**
     * @brief Generate a JSON compliance report.
     */
    json generateReportJson() const;

    /**
     * @brief Remove all registered SLOs and clear all accumulated data.
     */
    void clear();

    /**
     * @brief Return the number of registered SLOs.
     */
    size_t sloCount() const;

private:
    // ----- Internal types -----

    /// A single timestamped request sample.
    struct Sample {
        std::chrono::system_clock::time_point ts;
        bool good;
    };

    /**
     * @brief Per-SLO sliding window state.
     */
    struct SloState {
        SloDefinition def;
        /// Ring buffer of recent request samples.
        std::deque<Sample> samples;
    };

    // ----- Helpers (caller must hold mutex_) -----

    /**
     * @brief Expire samples that fall outside the SLO's window.
     *
     * @c now is passed in to avoid calling @c system_clock::now() repeatedly
     * under the lock.
     */
    static void expireSamples(SloState& state,
                              std::chrono::system_clock::time_point now);

    /**
     * @brief Compute the SloStatus from the current @c SloState.
     *
     * @c now must equal the value used for @c expireSamples so that counts
     * and statistics are consistent.
     */
    static SloStatus computeStatus(const SloState& state);

    /**
     * @brief Compute the burn rate for a subset of samples in the given window.
     *
     * @param samples  Complete sample deque (already expired to the full window).
     * @param window   Sub-window duration to evaluate.
     * @param now      Reference point (wall clock at evaluation time).
     * @param allowed_error_rate  = 1 - slo.objective
     * @return         Burn rate, or 0.0 if there are no samples in the window.
     */
    static double computeBurnRate(const std::deque<Sample>& samples,
                                  std::chrono::seconds window,
                                  std::chrono::system_clock::time_point now,
                                  double allowed_error_rate) noexcept;

    /**
     * @brief Extract the burn-rate gauge value for a specific level from a
     *        pre-computed SloStatus (used by publishMetrics to avoid redundant
     *        re-computation).
     */
    double computeBurnRateLevelPublish(const SloStatus& s,
                                       BurnRateLevel level) const;

    // ----- State -----
    mutable std::mutex mutex_;
    Config config_;
    std::map<std::string, SloState> slos_;
};

} // namespace observability
} // namespace themis

