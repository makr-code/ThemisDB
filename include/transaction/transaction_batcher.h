/**
 * @file transaction_batcher.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.12
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <deque>
#include <functional>
#include <future>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

namespace themis {

/**
 * @brief Automatic write batching and coalescing for high-throughput ingestion.
 *
 * TransactionBatcher queues concurrent commit operations and groups them into
 * timed batches, amortising per-commit overhead (WAL sync, lock release, etc.)
 * across many small transactions.
 *
 * Key features:
 *   - Configurable batch window (1–100 ms) controls the latency/throughput trade-off.
 *   - Maximum batch size caps unbounded queue growth; an immediate flush is triggered
 *     as soon as the queue reaches max_batch_size.
 *   - Fair FIFO scheduling prevents starvation: items are processed in submission order.
 *   - Per-table BatchPolicy overrides allow different windows and size limits per table.
 *   - Adaptive sizing: when enable_adaptive is true the batcher widens the window
 *     under low load (fewer items arriving than min_batch_size) and narrows it
 *     when the queue approaches max_batch_size, trading latency for throughput.
 *   - flush() forces an immediate drain of all pending items.
 *
 * Thread safety:
 *   - submitAsync() is safe to call concurrently from any number of threads.
 *   - setBatchConfig(), setTablePolicy(), flush(), and getStats() are also thread-safe.
 *   - The destructor blocks until the flush thread has drained all pending items;
 *     all submitted futures are guaranteed to be resolved before it returns.
 *     Do not call submitAsync() or flush() concurrently with destruction.
 *
 * Example — high-throughput ingestion:
 * @code
 *   TransactionBatcher batcher;
 *   batcher.setBatchConfig({
 *       .window           = std::chrono::milliseconds(10),
 *       .max_batch_size   = 5000,
 *       .enable_adaptive  = true,
 *   });
 *
 *   // Set aggressive policy for the "orders" table
 *   batcher.setTablePolicy("orders", {
 *       .window         = std::chrono::milliseconds(20),
 *       .max_batch_size = 10000,
 *   });
 *
 *   std::vector<std::future<TransactionBatcher::Status>> futures;
 *   for (auto& record : records) {
 *       futures.push_back(batcher.submitAsync(
 *           [&mgr, record]() -> TransactionBatcher::Status {
 *               auto id  = mgr.beginTransaction();
 *               auto* tx = mgr.getTransaction(id);
 *               tx->putEntity("orders", record);
 *               auto st  = mgr.commitTransaction(id);
 *               return st.ok ? TransactionBatcher::Status::OK()
 *                            : TransactionBatcher::Status::Error(st.message);
 *           },
 *           "orders"   // table hint for per-table policy lookup
 *       ));
 *   }
 *
 *   for (auto& f : futures) {
 *       auto status = f.get();  // waits for the batch containing this commit
 *   }
 * @endcode
 */
class TransactionBatcher {
public:
    // ── Status ────────────────────────────────────────────────────────────────

    /// Lightweight result type returned by each batched commit operation.
    struct Status {
        bool        ok{true};
        std::string message;

        static Status OK()                   { return {}; }
        static Status Error(std::string msg) { return {false, std::move(msg)}; }
    };

    // ── BatchConfig ───────────────────────────────────────────────────────────

    /**
     * @brief Global batching configuration.
     *
     * All time values are clamped to [1 ms, 100 ms] by setBatchConfig().
     */
    struct BatchConfig {
        /// Maximum time to wait before flushing a non-full batch.
        /// Range: [1 ms, 100 ms]. Default: 5 ms.
        std::chrono::microseconds window{5000};

        /// Flush immediately when the pending queue reaches this count.
        /// Must be >= 1. Default: 1000.
        size_t max_batch_size{1000};

        /// Minimum number of items the adaptive algorithm targets per batch.
        /// Must be >= 1. Default: 10.
        size_t min_batch_size{10};

        /// When true the batcher autonomously widens the window under low load
        /// (fewer items per batch than min_batch_size) and narrows it when the
        /// queue approaches max_batch_size, trading latency for throughput.
        bool enable_adaptive{true};
    };

    // ── BatchPolicy ───────────────────────────────────────────────────────────

    /**
     * @brief Per-table (or per-key-prefix) override policy.
     *
     * A zero value for any field means "inherit from the global BatchConfig".
     * Register policies via setTablePolicy().
     */
    struct BatchPolicy {
        /// Override window for this table.  Zero = use global window.
        std::chrono::microseconds window{0};

        /// Override max_batch_size for this table.  Zero = use global.
        size_t max_batch_size{0};

        /// Override min_batch_size for this table.  Zero = use global.
        size_t min_batch_size{0};
    };

    // ── Stats ─────────────────────────────────────────────────────────────────

    /// Aggregate statistics accumulated since construction (or last reset).
    struct Stats {
        /// Number of batches that have been flushed (including forced flushes).
        uint64_t batches_flushed{0};

        /// Total commit operations that completed successfully.
        uint64_t transactions_committed{0};

        /// Total commit operations that returned an error Status.
        uint64_t transactions_failed{0};

        /// Running average number of items per flushed batch.
        double avg_batch_size{0.0};

        /// Running average time from submitAsync() to Status resolution (ms).
        double avg_latency_ms{0.0};

        /// Number of times the adaptive algorithm adjusted the window.
        uint64_t adaptive_adjustments{0};
    };

    // ── Constructor / Destructor ──────────────────────────────────────────────

    /// Construct a batcher with default BatchConfig and start the flush thread.
    TransactionBatcher();

    /// Destroy the batcher.  Blocks until the flush thread has drained all
    /// pending items and exited cleanly.
    ~TransactionBatcher();

    // Non-copyable, non-movable (owns a background thread and mutexes)
    TransactionBatcher(const TransactionBatcher&)            = delete;
    TransactionBatcher& operator=(const TransactionBatcher&) = delete;
    TransactionBatcher(TransactionBatcher&&)                 = delete;
    TransactionBatcher& operator=(TransactionBatcher&&)      = delete;

    // ── Configuration ─────────────────────────────────────────────────────────

    /**
     * @brief Replace the global batching configuration.
     *
     * The new configuration takes effect on the next flush cycle.
     * window is clamped to [1 ms, 100 ms]; size values are clamped to >= 1.
     */
    void setBatchConfig(const BatchConfig& config);

    /// Return a snapshot of the current global configuration.
    BatchConfig getBatchConfig() const;

    /**
     * @brief Install or replace a per-table batching policy.
     *
     * @param table  Table name or key prefix used as the policy key.
     * @param policy Override policy; zero fields inherit from the global config.
     */
    void setTablePolicy(const std::string& table, const BatchPolicy& policy);

    /**
     * @brief Retrieve the policy registered for @p table.
     *
     * @return The registered BatchPolicy, or a zero-valued policy if none is set.
     */
    BatchPolicy getTablePolicy(const std::string& table) const;

    // ── Core API ──────────────────────────────────────────────────────────────

    /**
     * @brief Submit a commit operation for asynchronous batched execution.
     *
     * The commit function @p commit_fn is called by the background flush thread
     * as part of the next batch.  The returned future resolves to the Status
     * returned by @p commit_fn.
     *
     * If the batcher is being destroyed (flush thread is stopping), the future
     * resolves immediately with Status::Error("batcher is shutting down").
     *
     * @param commit_fn   Callable invoked by the flush thread; must be thread-safe
     *                    and return a Status.
     * @param table_hint  Optional table name used to look up a per-table policy.
     *                    An empty string uses the global BatchConfig.
     * @return            A future that resolves once the commit has been executed.
     */
    std::future<Status> submitAsync(std::function<Status()> commit_fn,
                                    const std::string& table_hint = "");

    /**
     * @brief Force an immediate flush of all pending items.
     *
     * Signals the background thread to drain the queue now and blocks until the
     * flush is complete.  Useful for end-of-batch or shutdown sequences.
     */
    void flush();

    /// Return a snapshot of the current aggregate statistics.
    Stats getStats() const;

private:
    // ── Internal types ────────────────────────────────────────────────────────

    struct PendingEntry {
        std::function<Status()>               commit_fn;
        std::promise<Status>                  promise;
        std::string                           table_hint;
        std::chrono::steady_clock::time_point submitted_at;
        /// Absolute time by which this item must be flushed (submitted_at + effective window).
        std::chrono::steady_clock::time_point deadline;
    };

    // ── Helper: resolve effective policy for a given table hint ───────────────

    /// Return the effective (window, max_batch_size, min_batch_size) for @p table,
    /// blending per-table overrides with the global config.
    struct EffectivePolicy {
        std::chrono::microseconds window;
        size_t                    max_batch_size;
        size_t                    min_batch_size;
    };
    EffectivePolicy effectivePolicyFor(const std::string& table) const;

    // ── Background thread ─────────────────────────────────────────────────────

    void flushLoop();

    /// Drain @p batch: invoke each commit_fn and resolve the associated promise.
    /// Updates stats atomically.
    void executeBatch(std::vector<PendingEntry>& batch);

    /// Possibly widen or narrow the adaptive window based on recent throughput.
    void adaptWindow(size_t batch_size, std::chrono::microseconds elapsed);

    // ── State ─────────────────────────────────────────────────────────────────

    mutable std::mutex    config_mutex_;
    BatchConfig           config_;
    std::unordered_map<std::string, BatchPolicy> table_policies_;

    mutable std::mutex              queue_mutex_;
    std::condition_variable         queue_cv_;
    /// Notified whenever a batch finishes executing (batch_in_progress_ → false).
    std::condition_variable         flush_cv_;
    std::deque<PendingEntry>        queue_;
    bool                            flush_requested_{false};
    /// True while the flush thread holds a local batch and is running executeBatch().
    bool                            batch_in_progress_{false};

    std::thread               flush_thread_;
    std::atomic<bool>         stopping_{false};

    mutable std::mutex        stats_mutex_;
    Stats                     stats_;

    // Adaptive state (guarded by config_mutex_)
    std::chrono::microseconds adaptive_window_{5000};
    std::deque<double>        recent_throughputs_;  // items/sec samples
};

} // namespace themis
