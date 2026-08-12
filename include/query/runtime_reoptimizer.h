/**
 * @file runtime_reoptimizer.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include "query/adaptive_optimizer.h"
#include <chrono>
#include <memory>
#include <string>

namespace themis {

using AdaptiveQueryStats = ::themis::query::AdaptiveQueryStats;
using AdaptivePlanSelector = ::themis::query::AdaptivePlanSelector;

/**
 * @brief Runtime Query Re-optimizer
 *
 * Wraps query execution to collect runtime statistics and enable adaptive
 * re-optimization.  For each query:
 *
 *   1. Call computeQueryHash() to derive a stable identifier from the AQL text.
 *   2. Call beginExecution() at the start; it returns an ExecutionContext that
 *      captures the start time and estimated cardinality.
 *   3. During execution call shouldReoptimize() at natural checkpoints to detect
 *      when the optimizer's estimates are significantly off and a plan switch
 *      should be considered.
 *   4. Call recordExecution() (or use the RAII ExecutionGuard) when the query
 *      finishes to persist actual statistics for future executions.
 *   5. On subsequent executions of the same query pattern, call
 *      getAdjustmentFactor() to retrieve a correction multiplier for cardinality
 *      estimates produced by the static cost model.
 *
 * Thread safety: All public methods are thread-safe.  A single RuntimeReoptimizer
 * instance may be shared across threads (e.g., as a process-global singleton).
 */
class RuntimeReoptimizer {
public:
    /**
     * @brief Lightweight RAII context for a single query execution.
     *
     * Returned by beginExecution(); automatically records statistics on
     * destruction if finish() has not been called explicitly.
     */
    struct ExecutionContext {
        std::string query_hash;
        size_t estimated_rows = 0;
        std::chrono::steady_clock::time_point start_time;
    };

    /**
     * @brief RAII guard that records execution statistics on scope exit.
     *
     * Obtain via beginExecution().  Call finish() with the actual row count
     * before the guard goes out of scope; otherwise it records 0 actual rows.
     */
    class ExecutionGuard {
    public:
        ExecutionGuard(RuntimeReoptimizer& owner, ExecutionContext ctx);
        ExecutionGuard(const ExecutionGuard&) = delete;
        ExecutionGuard& operator=(const ExecutionGuard&) = delete;
        ExecutionGuard(ExecutionGuard&&) noexcept;
        ExecutionGuard& operator=(ExecutionGuard&&) noexcept;
        ~ExecutionGuard();

        /// Record the final row count and close the guard without waiting for
        /// scope exit.  Safe to call multiple times; only the first call records.
        void finish(size_t actual_rows);

        const ExecutionContext& context() const { return ctx_; }

    private:
        RuntimeReoptimizer* owner_ = nullptr;
        ExecutionContext ctx_;
        size_t actual_rows_ = 0;
        bool finished_ = false;
    };

    RuntimeReoptimizer();

    /**
     * @brief Compute a stable hash string for a (possibly normalised) AQL query.
     *
     * The hash is derived from the raw text via FNV-1a so it is deterministic
     * across processes and platforms.  Callers should pass a normalised form
     * (lower-cased, whitespace-collapsed) to maximise hit rates.
     *
     * @return A 16-character lowercase hexadecimal string.
     */
    static std::string computeQueryHash(const std::string& aql_text);

    /**
     * @brief Start tracking a new query execution.
     *
     * @param query_hash      Hash obtained from computeQueryHash().
     * @param estimated_rows  Cardinality estimate from the static cost model.
     * @return An ExecutionContext capturing the start time and metadata.
     */
    ExecutionContext beginExecution(const std::string& query_hash,
                                   size_t estimated_rows) const;

    /**
     * @brief Begin execution and return an RAII guard that auto-records stats.
     *
     * Prefer this over beginExecution() + recordExecution() to avoid missed
     * recordings on exception paths.
     */
    ExecutionGuard beginExecutionGuard(const std::string& query_hash,
                                       size_t estimated_rows);

    /**
     * @brief Check whether the optimizer should switch to an alternative plan.
     *
     * Should be called at natural execution checkpoints (e.g., after each page
     * of rows is read).  Returns false when re-optimization is disabled.
     *
     * @param query_hash       Identifies the query pattern.
     * @param rows_so_far      Rows materialised so far.
     * @param estimated_total  Original cardinality estimate.
     * @param progress         Fraction of query completed (0.0–1.0).
     * @param threshold        Mis-estimation ratio that triggers a switch.
     * @return true if the plan should be reconsidered.
     */
    bool shouldReoptimize(const std::string& query_hash,
                          size_t rows_so_far,
                          size_t estimated_total,
                          double progress,
                          double threshold = 5.0) const;

    /**
     * @brief Persist runtime statistics for a completed query execution.
     *
     * @param query_hash        Stable identifier for the query pattern.
     * @param estimated_rows    Cardinality estimate used during planning.
     * @param actual_rows       Actual rows returned by the execution.
     * @param execution_time_ms Wall-clock execution time in milliseconds.
     */
    void recordExecution(const std::string& query_hash,
                         size_t estimated_rows,
                         size_t actual_rows,
                         double execution_time_ms);

    /**
     * @brief Return a correction factor for cardinality estimates.
     *
     * A value < 1.0 means the optimizer historically overestimates; > 1.0 means
     * it underestimates.  Returns 1.0 when there is no history or when
     * re-optimization is disabled.
     */
    double getAdjustmentFactor(const std::string& query_hash) const;

    /**
     * @brief Check whether the query has a history of significant mis-estimation.
     *
     * @param query_hash  Query pattern identifier.
     * @param threshold   Ratio above/below which mis-estimation is flagged.
     */
    bool hasMisestimation(const std::string& query_hash,
                          double threshold = 2.0) const;

    /**
     * @brief Remove statistics older than the given retention window.
     */
    void pruneOldStats(std::chrono::hours retention = std::chrono::hours(24));

    /**
     * @brief Enable or disable runtime re-optimization globally.
     *
     * When disabled, shouldReoptimize() always returns false and
     * getAdjustmentFactor() always returns 1.0, but statistics are still
     * recorded so the feature can be re-enabled without losing history.
     */
    void enable(bool enabled = true);
    bool isEnabled() const;

    /// Total number of query executions recorded since construction.
    size_t totalExecutions() const;

    /// Read-only access to the underlying statistics store.
    const AdaptiveQueryStats& stats() const;

private:
    std::shared_ptr<AdaptiveQueryStats> stats_;
    std::shared_ptr<AdaptivePlanSelector> selector_;
    bool enabled_ = true;
};

} // namespace themis
