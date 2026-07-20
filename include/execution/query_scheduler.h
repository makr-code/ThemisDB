/**
 * @file query_scheduler.h
 * @brief Phase 3 P3-04-C/D: SLA-aware query scheduler for ThemisDB.
 *
 * Provides @ref QueryScheduler, a thread-safe, deadline-driven priority queue
 * for scheduling incoming queries before dispatching them to the executor.
 *
 * ### Scheduling policy (Earliest-Deadline-First with fairness floor)
 *  - Each query carries a deadline computed from its SLA class:
 *    - HIGH  (SLA < 10 ms):   deadline = enqueue_time + sla_ms
 *    - MEDIUM (10–100 ms):    deadline = enqueue_time + sla_ms
 *    - LOW  (> 100 ms):       deadline = enqueue_time + sla_ms
 *  - The scheduler always dequeues the entry with the earliest deadline.
 *  - Dynamic re-prioritisation: entries within 5 seconds of their deadline
 *    are promoted to a "urgent" bucket processed first.
 *  - Fairness: LOW-priority queries hold at most 80 % of the queue;
 *    20 % is reserved so LOW-priority queries are never fully starved.
 *
 * ### Backpressure
 *  - @ref enqueue() blocks when queue depth > @c Config::max_queue_depth.
 *  - Load shedding: when depth > @c Config::shed_threshold, the lowest-
 *    priority query at the back of the queue is rejected.
 *
 * ### Metrics
 *  - Queue depth per priority level.
 *  - SLA compliance rate (completions within deadline / total completions).
 *  - Average scheduling overhead (enqueue + dequeue latency).
 *
 * @version 1.0.0
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Status: Block B P3-04-C/D delivery
 */

#pragma once

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <queue>
#include <stdexcept>
#include <string>
#include <vector>

namespace themis::execution {

// ============================================================================
// QueryEntry
// ============================================================================

/**
 * @brief SLA priority class for a query.
 */
enum class SLAPriority : int {
    HIGH   = 2,  ///< SLA < 10 ms
    MEDIUM = 1,  ///< 10 ms ≤ SLA < 100 ms
    LOW    = 0,  ///< SLA ≥ 100 ms
};

/**
 * @brief A single query entry in the scheduler queue.
 */
struct QueryEntry {
    using ExecuteFn = std::function<void()>;

    std::uint64_t id          = 0;    ///< Unique monotonically-increasing query ID.
    SLAPriority   priority    = SLAPriority::MEDIUM;
    std::chrono::steady_clock::time_point deadline{};  ///< Absolute deadline.
    std::chrono::steady_clock::time_point enqueue_time{};
    ExecuteFn     execute;            ///< The query thunk.
    std::string   name;               ///< Optional diagnostic name.
};

// ============================================================================
// QueryScheduler
// ============================================================================

/**
 * @brief SLA-aware, deadline-first query scheduler.
 *
 * @see QueryEntry, SLAPriority
 */
class QueryScheduler {
public:
    /**
     * @brief Configuration.
     */
    struct Config {
        std::size_t max_queue_depth = 1000;  ///< Backpressure threshold.
        std::size_t shed_threshold  = 5000;  ///< Load-shed threshold.
        /// Re-prioritisation window: promote if deadline within this many ms.
        long        urgent_window_ms = 5000;
        /// Default SLA for MEDIUM queries if none specified.
        long        default_sla_ms   = 50;
    };

    /**
     * @brief Metrics snapshot.
     */
    struct Metrics {
        std::size_t  queue_depth_high   = 0;
        std::size_t  queue_depth_medium = 0;
        std::size_t  queue_depth_low    = 0;
        std::uint64_t total_enqueued    = 0;
        std::uint64_t total_dequeued    = 0;
        std::uint64_t total_shed        = 0;
        std::uint64_t completed_in_sla  = 0; ///< Completions within deadline.
        std::uint64_t completed_total   = 0;
        double       sla_compliance_pct = 0.0;
        double       avg_enqueue_us     = 0.0;
        double       avg_dequeue_us     = 0.0;
    };

    /**
     * @brief Constructs the scheduler with default configuration.
     */
    QueryScheduler();

    /**
     * @brief Constructs the scheduler.
     * @param cfg  Configuration.
     */
    explicit QueryScheduler(const Config& cfg);

    ~QueryScheduler();

    // Non-copyable, non-movable.
    QueryScheduler(const QueryScheduler&)            = delete;
    QueryScheduler& operator=(const QueryScheduler&) = delete;

    /**
     * @brief Enqueues a query for scheduling.
     *
     * @param execute     The query thunk.
     * @param priority    SLA priority class.
     * @param sla_ms      Deadline relative to now (milliseconds).
     * @param name        Optional diagnostic name.
     * @param timeout     Maximum backpressure wait time.
     * @return Assigned query ID, or 0 if enqueue failed (shutdown or timeout).
     */
    [[nodiscard]] std::uint64_t enqueue(
        QueryEntry::ExecuteFn execute,
        SLAPriority           priority  = SLAPriority::MEDIUM,
        long                  sla_ms    = 50,
        std::string           name      = {},
        std::chrono::milliseconds timeout = std::chrono::seconds(5));

    /**
     * @brief Dequeues the highest-priority (earliest-deadline) query.
     *
     * Blocks until a query is available or @p timeout elapses.
     *
     * @param out      Output: the dequeued entry.
     * @param timeout  Maximum wait duration.
     * @return @c true if an entry was dequeued; @c false on timeout/shutdown.
     */
    bool dequeue(QueryEntry& out,
                 std::chrono::milliseconds timeout = std::chrono::seconds(5));

    /**
     * @brief Reports completion of a query (for SLA tracking).
     *
     * @param query_id    ID returned by @ref enqueue().
     * @param completion_time  When the query finished.
     */
    void reportCompletion(
        std::uint64_t query_id,
        std::chrono::steady_clock::time_point completion_time
            = std::chrono::steady_clock::now());

    /// @brief Returns current metrics.
    [[nodiscard]] Metrics metrics() const noexcept;

    /// @brief Total entries currently in the queue.
    [[nodiscard]] std::size_t size() const noexcept;

    /// @brief Initiates graceful shutdown.
    void shutdown() noexcept;

    /// @brief Returns true once shutdown has been requested.
    [[nodiscard]] bool is_shutdown() const noexcept {
        return shutdown_.load(std::memory_order_acquire);
    }

private:
    // Comparator: earlier deadline wins; tie-break by ID (FIFO).
    struct EarliestDeadlineFirst {
        bool operator()(const QueryEntry& a, const QueryEntry& b) const {
            if (a.deadline != b.deadline) {
                return a.deadline > b.deadline;  // min-heap by deadline.
            }
            return a.id > b.id;  // FIFO for equal deadlines.
        }
    };

    using PQueue = std::priority_queue<QueryEntry,
                                       std::vector<QueryEntry>,
                                       EarliestDeadlineFirst>;

    Config cfg_;

    mutable std::mutex      mutex_;
    std::condition_variable enqueue_cv_;  ///< Notified when capacity available.
    std::condition_variable dequeue_cv_;  ///< Notified when items arrive.

    PQueue               queue_;
    std::atomic<bool>    shutdown_{false};
    std::atomic<uint64_t> next_id_{1};

    // Metrics (protected by mutex_).
    std::uint64_t total_enqueued_      = 0;
    std::uint64_t total_dequeued_      = 0;
    std::uint64_t total_shed_          = 0;
    std::uint64_t completed_in_sla_    = 0;
    std::uint64_t completed_total_     = 0;
    double        enqueue_latency_sum_ = 0.0;
    double        dequeue_latency_sum_ = 0.0;

    // Deadline tracking for reportCompletion: query_id → deadline.
    std::unordered_map<std::uint64_t,
        std::chrono::steady_clock::time_point> pending_deadlines_;
};

}  // namespace themis::execution

// Required for pending_deadlines_ type completeness.
#include <unordered_map>
