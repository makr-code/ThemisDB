/**
 * @file per_query_cost_model.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


// Per-Query Cost Model Integration with Query Optimizer
// Phase 3 performance optimization: integrates hardware cycle measurement
// (CycleMetrics) with the OptimizerCostModel to calibrate cost estimates
// from actual query execution feedback.
//
// Key idea: record actual cycles and wall-clock time per query execution,
// accumulate statistics, and periodically calibrate OptimizerCostModel
// constants so that future plan estimates are anchored to real hardware.
//
// Expected gain: ~10-30% better plan selection accuracy on repeat queries.

#pragma once

#include "performance/cycle_metrics.h"
#include "query/optimizer_cost_model.h"

#include <atomic>
#include <chrono>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

namespace themis {
namespace performance {
namespace phase3 {

/**
 * @brief Per-query execution cost record.
 *
 * Captures both hardware cycle counts (from CycleMetrics) and derived
 * wall-clock time for a single query execution.  Stored in the ring
 * buffer inside PerQueryCostModel.
 */
struct QueryCostRecord {
    std::string query_type;            ///< "table_scan", "index_scan", "join", etc.
    uint64_t    cycles_elapsed   = 0;  ///< CPU cycles measured with RDTSCP
    double      execution_time_ms = 0.0; ///< Wall-clock duration in milliseconds
    size_t      rows_processed   = 0;  ///< Rows touched during execution
    size_t      pages_read       = 0;  ///< Pages fetched from storage
    double      estimated_cost   = 0.0; ///< OptimizerCostModel estimate at plan time
    double      cost_ratio       = 1.0; ///< estimated_cost / actual_cost (>1 = over-estimated)

    /// Execution path actually used (set by higher-level consumer after planning).
    OptimizerCostModel::SerializationAdvice::ExecutionPath exec_path_used =
        OptimizerCostModel::SerializationAdvice::ExecutionPath::CPU_SINGLE;

    /// Time spent on serialization/deserialization within this query (ms).
    /// Populated by consumers that measure serialization overhead separately.
    double serialization_time_ms = 0.0;
};

/**
 * @brief Per-query cost model that integrates with OptimizerCostModel.
 *
 * Records actual query execution costs using HardwareCycleCounter and
 * derives calibration factors that can be applied to OptimizerCostModel
 * constants so subsequent cost estimates are grounded in real hardware
 * measurements.
 *
 * Thread-safety: all public methods are safe to call concurrently.
 * The internal record store is protected by a mutex; the RAII timer
 * helper is single-threaded per query.
 *
 * Usage:
 * @code
 *   PerQueryCostModel pcm;
 *   OptimizerCostModel model;
 *
 *   // Wrap a query execution:
 *   {
 *       auto guard = pcm.beginQuery("index_scan", 42.0); // estimated
 *       // ... execute query ...
 *       guard.end(rows_returned, pages_read);
 *   }
 *
 *   // Periodically calibrate:
 *   pcm.calibrate(model);
 * @endcode
 */
class PerQueryCostModel {
public:
    /// Maximum records kept in the rolling history.
    static constexpr size_t MAX_RECORDS = 4096;

    PerQueryCostModel();
    ~PerQueryCostModel();

    // Non-copyable (owns mutable state)
    PerQueryCostModel(const PerQueryCostModel&)            = delete;
    PerQueryCostModel& operator=(const PerQueryCostModel&) = delete;

    // -----------------------------------------------------------------
    // RAII query timer
    // -----------------------------------------------------------------

    /**
     * @brief RAII guard returned by beginQuery().
     *
     * Calls end() automatically on destruction if not called explicitly.
     */
    class QueryGuard {
    public:
        QueryGuard(PerQueryCostModel& model,
                   std::string query_type,
                   double      estimated_cost) noexcept;
        ~QueryGuard() noexcept;

        // Non-copyable
        QueryGuard(const QueryGuard&)            = delete;
        QueryGuard& operator=(const QueryGuard&) = delete;

        // Movable
        QueryGuard(QueryGuard&&) noexcept;
        QueryGuard& operator=(QueryGuard&&) = delete;

        /**
         * @brief Finalise the measurement and push the record.
         * @param rows_processed  Number of rows touched.
         * @param pages_read      Number of storage pages read.
         */
        void end(size_t rows_processed = 0, size_t pages_read = 0) noexcept;

    private:
        PerQueryCostModel*                       model_;
        std::string                              query_type_;
        double                                   estimated_cost_;
        uint64_t                                 start_cycles_;
        std::chrono::steady_clock::time_point    start_wall_;
        bool                                     ended_;
    };

    /**
     * @brief Start timing a query execution.
     * @param query_type    Logical type label, e.g. "table_scan".
     * @param estimated_cost  OptimizerCostModel estimate for this plan.
     * @return RAII guard – call guard.end() or let it destruct automatically.
     */
    [[nodiscard]]
    QueryGuard beginQuery(const std::string& query_type,
                          double             estimated_cost = 0.0) noexcept;

    // -----------------------------------------------------------------
    // Calibration
    // -----------------------------------------------------------------

    /**
     * @brief Compute calibration factors from recorded history.
     *
     * Returns a map of OptimizerCostModel constant names to updated
     * values derived from measured CPU and I/O rates.  Pass to
     * OptimizerCostModel::calibrateCosts().
     *
     * Additionally emits:
     *  - "gpu_row_threshold_low"  when GPU-path records show high serialization
     *    overhead (serialization_time_ms / execution_time_ms > 0.5), suggesting
     *    the GPU breakeven threshold should be raised.
     *  - "msgpack_row_threshold"  when CPU_SINGLE records show the row count
     *    distribution warrants switching to binary format at a lower threshold.
     *
     * @param current  Optional pointer to the model's current CostConstants.
     *   When provided the GPU/msgpack threshold adjustments are computed
     *   relative to the actual configured values rather than the compile-time
     *   defaults, preventing threshold oscillation after repeated calibrations.
     *   Pass nullptr (default) for backwards compatibility.
     */
    std::unordered_map<std::string, double> getCalibrationFactors(
        const OptimizerCostModel::CostConstants* current = nullptr) const;

    /**
     * @brief Apply calibration directly to an OptimizerCostModel.
     *
     * Convenience wrapper around getCalibrationFactors() +
     * OptimizerCostModel::calibrateCosts().
     */
    void calibrate(OptimizerCostModel& model) const;

    // -----------------------------------------------------------------
    // Statistics
    // -----------------------------------------------------------------

    struct Stats {
        size_t total_queries;
        double avg_execution_time_ms;
        double avg_cost_ratio;           ///< estimated/actual; 1.0 = perfect
        double p50_execution_time_ms;
        double p95_execution_time_ms;
        std::unordered_map<std::string, double> per_type_avg_time_ms;
        std::unordered_map<std::string, size_t> per_type_count;
    };

    /** @brief Aggregate statistics over all recorded queries. */
    Stats getStats() const;

    /**
     * @brief Return the most recent records (up to @p limit).
     * @param limit Maximum number of records to return.
     */
    std::vector<QueryCostRecord> getRecentRecords(size_t limit = 100) const;

    /** @brief Reset all accumulated records. */
    void reset() noexcept;

    /** @brief Total number of queries recorded since construction or last reset(). */
    size_t totalQueries() const noexcept { return total_queries_.load(std::memory_order_relaxed); }

private:
    friend class QueryGuard;

    /**
     * @brief Push a completed record into the rolling buffer.
     *
     * Called from QueryGuard::end(); internal use only.
     */
    void pushRecord(QueryCostRecord record) noexcept;

    mutable std::mutex              mutex_;
    std::vector<QueryCostRecord>    records_;   ///< Rolling window
    std::atomic<size_t>             total_queries_{0};
    std::atomic<size_t>             write_pos_{0};
};

} // namespace phase3
} // namespace performance
} // namespace themis
