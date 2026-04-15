/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            hallucination_dashboard.h                          ║
  Version:         0.0.14                                             ║
  Last Modified:   2026-04-15 18:04:48                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     277                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/**
 * @file hallucination_dashboard.h
 * @brief Hallucination rate tracking dashboard for the RAG evaluation pipeline
 *
 * Records per-query faithfulness scores from RAG evaluations and computes
 * rolling-window statistics to monitor hallucination rates over time.
 *
 * A "hallucination event" is defined as any evaluation whose faithfulness
 * score falls below the configured @c faithfulness_threshold (default 0.8).
 * The hallucination rate is the fraction of such events in the current window.
 *
 * Features:
 *  - Thread-safe recording and querying via a single mutex
 *  - Sliding window (configurable size, default 200 evaluations)
 *  - Per-snapshot statistics: rate, mean faithfulness, trend, min/max
 *  - Configurable alert threshold with optional alert callback
 *  - CSV export for offline analysis
 *  - Human-readable report via printReport()
 *
 * Integration:
 * @code
 *   HallucinationDashboard dashboard;
 *   // after each RAG evaluation:
 *   dashboard.record(eval_result);
 *   // periodically check for alerts:
 *   auto snap = dashboard.snapshot();
 *   if (snap.alert_triggered) { ... }
 * @endcode
 */

#pragma once

#include "rag/rag_judge.h"

#include <chrono>
#include <deque>
#include <functional>
#include <iostream>
#include <memory>
#include <mutex>
#include <ostream>
#include <string>
#include <vector>

namespace themis::rag::judge {

/**
 * @brief A single recorded hallucination-tracking entry
 */
struct HallucinationEntry {
    std::chrono::system_clock::time_point timestamp;
    std::string query;               ///< Query associated with the evaluation
    double faithfulness_score = 0.0; ///< Faithfulness score [0, 1]
    bool   is_hallucination   = false; ///< true when faithfulness < threshold
    std::string evaluation_mode;     ///< "FAST" | "BALANCED" | "THOROUGH"
};

/**
 * @brief Severity level for hallucination alerts
 */
enum class AlertSeverity {
    INFO,    ///< Rate slightly above threshold
    WARNING, ///< Rate moderately above threshold
    CRITICAL ///< Rate well above threshold
};

/**
 * @brief Alert issued when the hallucination rate exceeds a threshold
 */
struct HallucinationAlert {
    AlertSeverity severity;
    double current_rate;           ///< Current hallucination rate [0, 1]
    double threshold;              ///< Alert threshold that was exceeded
    size_t window_size;            ///< Number of evaluations in window
    std::chrono::system_clock::time_point timestamp;
    std::string message;
};

/**
 * @brief Point-in-time snapshot of hallucination dashboard statistics
 */
struct DashboardSnapshot {
    size_t total_recorded   = 0; ///< Total evaluations ever recorded
    size_t window_size      = 0; ///< Number of evaluations in current window
    size_t hallucination_count = 0; ///< Hallucination events in window

    double hallucination_rate  = 0.0; ///< fraction [0, 1]
    double mean_faithfulness   = 0.0; ///< Average faithfulness in window
    double min_faithfulness    = 1.0; ///< Minimum faithfulness in window
    double max_faithfulness    = 0.0; ///< Maximum faithfulness in window
    double std_faithfulness    = 0.0; ///< Std-dev of faithfulness in window

    /// Linear-regression slope of faithfulness over the window.
    /// Positive = improving, negative = degrading.
    double faithfulness_trend = 0.0;

    bool alert_triggered = false;    ///< Whether an alert threshold was exceeded
    std::vector<HallucinationAlert> active_alerts;
};

/**
 * @brief Configuration for HallucinationDashboard
 */
struct HallucinationDashboardConfig {
    /// Faithfulness score below which an evaluation counts as a hallucination.
    double faithfulness_threshold = 0.8;

    /// Maximum number of evaluations retained in the rolling window.
    size_t window_size = 200;

    /// Hallucination rate that triggers an INFO alert.
    double alert_threshold_info     = 0.05; ///< 5 % rate → INFO
    /// Hallucination rate that triggers a WARNING alert.
    double alert_threshold_warning  = 0.15; ///< 15 % rate → WARNING
    /// Hallucination rate that triggers a CRITICAL alert.
    double alert_threshold_critical = 0.30; ///< 30 % rate → CRITICAL
};

/**
 * @brief Hallucination rate tracking dashboard
 *
 * Thread-safe sliding-window tracker that accepts RAG evaluation results and
 * exposes aggregated statistics for monitoring and alerting.
 */
class HallucinationDashboard {
public:
    /**
     * @brief Construct with default configuration
     */
    HallucinationDashboard();

    /**
     * @brief Construct with custom configuration
     * @param config Dashboard configuration
     */
    explicit HallucinationDashboard(const HallucinationDashboardConfig& config);

    ~HallucinationDashboard();

    // ─── Recording ────────────────────────────────────────────────────────

    /**
     * @brief Record a full RAG evaluation result
     *
     * Extracts the faithfulness score and query metadata from the result.
     * The @p query string is stored for display purposes.
     *
     * @param result   Evaluation result produced by RAGJudge
     * @param query    User query associated with the evaluation
     * @param mode     Evaluation mode ("FAST", "BALANCED", "THOROUGH")
     */
    void record(const EvaluationResult& result,
                const std::string& query = "",
                const std::string& mode  = "BALANCED");

    /**
     * @brief Record a faithfulness score directly
     *
     * Useful when only the faithfulness score is available without
     * a full EvaluationResult.
     *
     * @param faithfulness_score Score in [0, 1]
     * @param query              Optional query string
     * @param mode               Evaluation mode string
     */
    void recordFaithfulness(double faithfulness_score,
                            const std::string& query = "",
                            const std::string& mode  = "BALANCED");

    // ─── Querying ─────────────────────────────────────────────────────────

    /**
     * @brief Compute and return a point-in-time statistics snapshot
     */
    DashboardSnapshot snapshot() const;

    /**
     * @brief Current hallucination rate over the rolling window
     * @return Fraction [0, 1] of evaluations classified as hallucinations
     */
    double hallucinationRate() const;

    /**
     * @brief Return the N most recent hallucination entries
     * @param n Maximum number of entries to return (0 = all)
     */
    std::vector<HallucinationEntry> recentEntries(size_t n = 0) const;

    // ─── Alerting ─────────────────────────────────────────────────────────

    using AlertCallback = std::function<void(const HallucinationAlert&)>;

    /**
     * @brief Register a callback invoked whenever an alert is generated
     *
     * The callback is called while the internal mutex is NOT held, so
     * callers may safely call any dashboard method from within it.
     */
    void setAlertCallback(AlertCallback callback);

    /**
     * @brief Manually check thresholds and fire callbacks if exceeded
     *
     * Called automatically after every record(), but can also be triggered
     * explicitly.
     *
     * @return Alerts generated during this check (may be empty)
     */
    std::vector<HallucinationAlert> checkAlerts();

    // ─── Persistence / Reporting ──────────────────────────────────────────

    /**
     * @brief Export entries in the current window to a CSV file
     *
     * Columns: timestamp_epoch_s, query, faithfulness_score,
     *           is_hallucination, evaluation_mode
     *
     * @param filepath Destination file path
     * @return true on success, false if the file could not be opened
     */
    bool exportCSV(const std::string& filepath) const;

    /**
     * @brief Print a human-readable dashboard report to an output stream
     * @param os Target stream (default: std::cout)
     */
    void printReport(std::ostream& os = std::cout) const;

    /**
     * @brief Reset all recorded data and statistics
     */
    void reset();

    /**
     * @brief Return the active configuration
     */
    const HallucinationDashboardConfig& config() const { return config_; }

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
    HallucinationDashboardConfig config_;
    AlertCallback alert_callback_;
    mutable std::mutex mutex_;

    void recordEntry(HallucinationEntry entry);
    void fireAlertsUnlocked(double rate);

    double computeMean(const std::deque<double>& data) const;
    double computeStdDev(const std::deque<double>& data, double mean) const;
    double computeTrend(const std::deque<double>& data) const;
};

} // namespace themis::rag::judge
