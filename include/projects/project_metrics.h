/**
 * @file project_metrics.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 1.0.0
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <atomic>
#include <string>
#include <cstdint>

namespace themis {
namespace projects {

/**
 * @brief Thread-safe Prometheus metrics sink for the Projects module.
 *
 * Tracks two metric families:
 *
 * **Collaboration change counter**
 * - `projects_changes_total`  — total `notifyChange()` calls across all projects
 *
 * **Diff latency histogram (simple counter-pair)**
 * - `project_diff_calls_total`       — total `ProjectDiff::diff()` invocations
 * - `project_diff_duration_ms_total` — cumulative wall-clock latency in ms
 *
 * The counter-pair pattern (sum + count) is equivalent to a Prometheus
 * summary without quantiles and lets the consumer compute the mean:
 *   mean_latency_ms = project_diff_duration_ms_total / project_diff_calls_total
 *
 * @note All methods are thread-safe via `std::atomic` relaxed/acq-rel ops.
 *
 * ### Prometheus text output
 * @code
 * # HELP projects_changes_total Total collaboration change events recorded.
 * # TYPE projects_changes_total counter
 * projects_changes_total 42
 *
 * # HELP project_diff_calls_total Total ProjectDiff::diff() invocations.
 * # TYPE project_diff_calls_total counter
 * project_diff_calls_total 7
 *
 * # HELP project_diff_duration_ms_total Cumulative diff computation time in ms.
 * # TYPE project_diff_duration_ms_total counter
 * project_diff_duration_ms_total 135
 * @endcode
 */
class ProjectMetrics {
public:
    ProjectMetrics() noexcept = default;

    /// Not copyable — each owner should hold a shared_ptr<ProjectMetrics>.
    ProjectMetrics(const ProjectMetrics&)            = delete;
    ProjectMetrics& operator=(const ProjectMetrics&) = delete;

    // ── Recording API ─────────────────────────────────────────────────────

    /**
     * @brief Increment the collaboration-change counter by one.
     *
     * Call once per `CollaborationManager::notifyChange()` invocation.
     * Thread-safe.
     */
    void recordChange() noexcept {
        changes_total_.fetch_add(1, std::memory_order_relaxed);
    }

    /**
     * @brief Record one completed `ProjectDiff::diff()` call.
     *
     * @param latency_ms  Wall-clock duration of the diff in milliseconds.
     *
     * Thread-safe.
     */
    void recordDiff(uint64_t latency_ms) noexcept {
        diff_calls_total_.fetch_add(1, std::memory_order_relaxed);
        diff_duration_ms_total_.fetch_add(latency_ms, std::memory_order_relaxed);
    }

    // ── Read API ──────────────────────────────────────────────────────────

    /// Return the total number of change events recorded.
    [[nodiscard]] uint64_t changesTotal() const noexcept {
        return changes_total_.load(std::memory_order_relaxed);
    }

    /// Return the total number of diff calls.
    [[nodiscard]] uint64_t diffCallsTotal() const noexcept {
        return diff_calls_total_.load(std::memory_order_relaxed);
    }

    /// Return the cumulative diff duration in milliseconds.
    [[nodiscard]] uint64_t diffDurationMsTotal() const noexcept {
        return diff_duration_ms_total_.load(std::memory_order_relaxed);
    }

    // ── Prometheus export ─────────────────────────────────────────────────

    /**
     * @brief Produce a Prometheus text-format (v0.0.4) metrics payload.
     *
     * Returns an empty string when no data has been recorded yet
     * (avoids emitting zero-value metrics on a fresh instance).
     */
    [[nodiscard]] std::string getMetricsText() const;

private:
    std::atomic<uint64_t> changes_total_{0};
    std::atomic<uint64_t> diff_calls_total_{0};
    std::atomic<uint64_t> diff_duration_ms_total_{0};
};

} // namespace projects
} // namespace themis
