/**
 * @file workload_adaptive_optimizer.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.10
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


// Copyright 2026 ThemisDB
// Licensed under MIT License

#pragma once

#include <atomic>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <mutex>
#include <shared_mutex>
#include <string>
#include <thread>
#include <vector>

namespace themis {
namespace performance {

// ============================================================================
// WorkloadAdaptiveOptimizer -- Issue #230 (v1.9.0)
// ============================================================================

/**
 * @brief Classification of the current database workload.
 */
enum class WorkloadType {
    OLTP,        ///< High-concurrency, short transactions
    OLAP,        ///< Complex analytical queries
    MIXED,       ///< Both OLTP and OLAP
    GRAPH,       ///< Graph traversal and analytics
    VECTOR,      ///< Vector similarity search
    TIMESERIES,  ///< Time-series queries
    UNKNOWN      ///< Insufficient data to classify
};

/**
 * @brief Snapshot of runtime workload characteristics.
 */
struct WorkloadProfile {
    WorkloadType type                 = WorkloadType::UNKNOWN;
    double       read_write_ratio     = 1.0;   ///< reads / (reads + writes)
    double       avg_query_complexity = 1.0;   ///< relative units [1..10]
    size_t       avg_result_size      = 0;     ///< rows
    size_t       concurrent_queries   = 0;
    std::vector<std::string> hot_tables;
};

/**
 * @brief Optimisation strategy selected for a workload profile.
 */
struct OptimizationStrategy {
    bool        enable_jit_compilation    = false;
    bool        enable_parallel_execution = true;
    size_t      thread_pool_size          = 4;
    size_t      cache_size_mb             = 256;
    std::string join_algorithm            = "hash";   ///< hash|sort-merge|nested-loop
    std::string index_type                = "btree";  ///< btree|hash|brin
};

/**
 * @brief Callback invoked when the optimizer adapts to a workload change.
 */
using AdaptationCallback =
    std::function<void(const WorkloadProfile& old_profile,
                       const WorkloadProfile& new_profile,
                       const OptimizationStrategy& strategy)>;

/**
 * @brief Workload-adaptive optimizer (v1.9.0, Issue #230).
 *
 * Research basis: "Adaptive Execution" (SIGMOD'19).
 *
 * Features:
 *  - Workload Classification: OLTP/OLAP/MIXED/GRAPH/VECTOR/TIMESERIES based
 *    on QPS, concurrency, read/write ratio, and query complexity.
 *  - Dynamic Strategy Selection: join algorithm, index type, thread pool
 *    size, and cache size per workload.
 *  - Resource Reallocation: reports recommended resources in OptimizationStrategy.
 *  - Performance Feedback: background adaptation loop (configurable interval).
 *  - Predictive Scaling: raises thread_pool_size when QPS trend is upward.
 *
 * Thread safety: all public methods are thread-safe.
 */
class WorkloadAdaptiveOptimizer {
public:
    // =========================================================================
    // Construction
    // =========================================================================

    WorkloadAdaptiveOptimizer();
    ~WorkloadAdaptiveOptimizer();

    WorkloadAdaptiveOptimizer(const WorkloadAdaptiveOptimizer&)            = delete;
    WorkloadAdaptiveOptimizer& operator=(const WorkloadAdaptiveOptimizer&) = delete;

    // =========================================================================
    // Workload observation feed
    // =========================================================================

    /**
     * @brief Record a completed query.
     *
     * @param is_write     true for INSERT/UPDATE/DELETE.
     * @param complexity   Relative complexity [1..10].
     * @param result_rows  Rows returned.
     * @param table_name   Primary table (empty = unknown).
     * @param latency_us   End-to-end latency in microseconds.
     */
    void record_query(bool               is_write,
                      double             complexity,
                      size_t             result_rows,
                      const std::string& table_name = {},
                      uint64_t           latency_us = 0);

    /**
     * @brief Update the current concurrent query count.
     */
    void set_concurrent_queries(size_t n);

    // =========================================================================
    // Classification and strategy
    // =========================================================================

    /**
     * @brief Classify the current workload from recent observations.
     */
    WorkloadProfile classify_workload() const;

    /**
     * @brief Return the optimal OptimizationStrategy for a given profile.
     */
    OptimizationStrategy get_strategy(const WorkloadProfile& profile) const;

    /**
     * @brief Apply a strategy (updates internal state; triggers callback).
     */
    void apply_strategy(const OptimizationStrategy& strategy);

    /**
     * @brief Return the currently applied strategy.
     */
    OptimizationStrategy current_strategy() const;

    // =========================================================================
    // Automatic adaptation
    // =========================================================================

    /**
     * @brief Start a background thread that re-classifies the workload and
     *        applies the best strategy every interval.
     */
    void enable_auto_adapt(std::chrono::seconds interval = std::chrono::seconds{60});

    /**
     * @brief Stop the background adaptation thread.
     */
    void disable_auto_adapt();

    /** @brief Return true when the background thread is running. */
    bool is_auto_adapt_enabled() const noexcept;

    /**
     * @brief Register a callback invoked on every adaptation.
     *
     * Only one callback is supported; subsequent calls replace the previous.
     */
    void set_callback(AdaptationCallback cb);

    // =========================================================================
    // Statistics
    // =========================================================================

    struct Stats {
        uint64_t total_queries_recorded = 0;
        uint64_t total_adaptations      = 0;
        WorkloadType last_workload_type = WorkloadType::UNKNOWN;
    };

    Stats get_stats() const;
    void  reset_stats();

    /**
     * @brief Return the current workload profile-drift score in [0.0, 1.0].
     *
     * Drift is defined as the L2 distance between the current workload type
     * distribution and the distribution observed during the last adaptation,
     * normalised to [0.0, 1.0].
     *
     * Used by Loop 2 (Workload) in ContinuousLearningOrchestrator to decide
     * whether to trigger retraining (threshold: > 0.1).
     *
     * @return Drift score in [0.0, 1.0].  Returns 0.0 when fewer than two
     *         adaptation events have been recorded.
     */
    [[nodiscard]] double getProfileDrift() const;

private:
    void adapt_once();

    // Rolling observation window (last kWindowSize queries)
    static constexpr size_t kWindowSize = 512;

    struct QueryObs {
        bool     is_write;
        double   complexity;
        size_t   result_rows;
        uint64_t latency_us;
        std::string table_name;
    };

    mutable std::shared_mutex  obs_mutex_;
    std::vector<QueryObs> observations_;  ///< circular buffer (appended, front evicted)
    std::atomic<size_t>   concurrent_queries_{0};

    mutable std::shared_mutex     strategy_mutex_;
    OptimizationStrategy   current_strategy_;

    mutable std::shared_mutex     stats_mutex_;
    Stats                  stats_;

    AdaptationCallback     callback_;

    // Auto-adapt thread
    std::thread            adapt_thread_;
    std::atomic<bool>      adapt_running_{false};
    std::chrono::seconds   adapt_interval_{60};
};

}  // namespace performance
}  // namespace themis
