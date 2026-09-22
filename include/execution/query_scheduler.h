/**
 * @file query_scheduler.h
 * @brief Deadline-ordered query scheduler for the execution module.
 *
 * The scheduler provides a small in-process admission queue for higher-level
 * execution code. Queries are ordered by their computed absolute deadline and
 * tied by query id (FIFO for equal deadlines). Capacity is bounded, overload is
 * signalled explicitly, and completions can be reported back for SLA metrics.
 *
 * @note `Config::urgent_window_ms` and `Config::default_sla_ms` are currently
 *       reserved compatibility fields. They are stored in the public contract
 *       but are not consulted by the live implementation yet.
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
#include <unordered_map>
#include <vector>

namespace themis::execution {

/**
 * @brief Coarse SLA label carried with each queued query.
 *
 * The current dequeue comparator orders strictly by absolute deadline rather
 * than by this enum directly. The label is still used for metrics and the
 * low-priority shedding rule.
 */
enum class SLAPriority : int {
    HIGH   = 2,  ///< Caller marks the query as latency sensitive.
    MEDIUM = 1,  ///< Default priority for routine work.
    LOW    = 0,  ///< Best-effort work that can be shed under overload.
};

/**
 * @brief One queued execution request.
 */
struct QueryEntry {
    using ExecuteFn = std::function<void()>;

    std::uint64_t id          = 0;    ///< Unique monotonically increasing query id.
    SLAPriority   priority    = SLAPriority::MEDIUM; ///< Caller-provided SLA label.
    std::chrono::steady_clock::time_point deadline{};  ///< Absolute execution deadline.
    std::chrono::steady_clock::time_point enqueue_time{}; ///< Time at which the query entered the scheduler.
    ExecuteFn     execute;            ///< Callable executed by the downstream consumer.
    std::string   name;               ///< Optional diagnostic name.
};

/**
 * @brief Bounded deadline-ordered scheduler for query execution requests.
 */
class QueryScheduler {
public:
    /**
     * @brief Scheduler configuration.
     */
    struct Config {
        std::size_t max_queue_depth = 1000;  ///< Maximum number of queued entries before producers wait.
        std::size_t shed_threshold  = 5000;  ///< LOW-priority items are rejected once the queue reaches this depth.
        long        urgent_window_ms = 5000; ///< Reserved for future priority-promotion behavior; unused today.
        long        default_sla_ms   = 50;   ///< Reserved default SLA value for future callers; unused today.
    };

    /**
     * @brief Snapshot of scheduler runtime counters.
     */
    struct Metrics {
        std::size_t  queue_depth_high   = 0; ///< Currently queued HIGH-priority entries.
        std::size_t  queue_depth_medium = 0; ///< Currently queued MEDIUM-priority entries.
        std::size_t  queue_depth_low    = 0; ///< Currently queued LOW-priority entries.
        std::uint64_t total_enqueued    = 0; ///< Total successful enqueue operations.
        std::uint64_t total_dequeued    = 0; ///< Total successful dequeue operations.
        std::uint64_t total_shed        = 0; ///< Total LOW-priority entries rejected at the shed threshold.
        std::uint64_t completed_in_sla  = 0; ///< Number of reported completions that met their recorded deadline.
        std::uint64_t completed_total   = 0; ///< Number of completion reports received.
        double       sla_compliance_pct = 0.0; ///< Completion percentage within SLA, or 0 when no completions were reported.
        double       avg_enqueue_us     = 0.0; ///< Average successful enqueue latency in microseconds.
        double       avg_dequeue_us     = 0.0; ///< Average successful dequeue latency in microseconds.
    };

    /**
     * @brief Construct a scheduler with default configuration.
     */
    QueryScheduler();

    /**
     * @brief Construct a scheduler with an explicit configuration snapshot.
     * @param cfg Runtime limits and reserved compatibility settings.
     */
    explicit QueryScheduler(const Config& cfg);

    /**
     * @brief Destroy the scheduler after initiating shutdown.
     */
    ~QueryScheduler();

    /**
     * @brief Copying is disabled because the scheduler owns synchronization state.
     * @param other Unused source scheduler instance.
     */
    QueryScheduler(const QueryScheduler& other) = delete;

    /**
     * @brief Copy assignment is disabled because the scheduler owns synchronization state.
     * @param other Unused source scheduler instance.
     * @return This scheduler instance; the operator is deleted and cannot be used.
     */
    QueryScheduler& operator=(const QueryScheduler& other) = delete;

    /**
     * @brief Enqueue a query for later execution.
     * @param execute Callable owned by the queued entry.
     * @param priority Caller-provided SLA label used for metrics and shedding.
     * @param sla_ms Relative SLA budget in milliseconds; converted to an absolute deadline at enqueue time.
     * @param name Optional diagnostic name stored with the queued entry.
     * @param timeout Maximum time to wait for queue capacity before rejecting the submission.
     * @return A non-zero query id on success, or `0` if shutdown is in progress,
     *         the wait for capacity times out, or a LOW-priority entry is shed.
     */
    [[nodiscard]] std::uint64_t enqueue(
        QueryEntry::ExecuteFn execute,
        SLAPriority           priority  = SLAPriority::MEDIUM,
        long                  sla_ms    = 50,
        std::string           name      = {},
        std::chrono::milliseconds timeout = std::chrono::seconds(5));

    /**
     * @brief Dequeue the next query ordered by absolute deadline.
     * @param out Receives the dequeued entry on success.
     * @param timeout Maximum time to wait for an entry before returning `false`.
     * @return `true` when an entry was dequeued, otherwise `false` if the wait
     *         timed out or the scheduler reached shutdown with no remaining work.
     */
    bool dequeue(QueryEntry& out,
                 std::chrono::milliseconds timeout = std::chrono::seconds(5));

    /**
     * @brief Report completion status for a previously enqueued query id.
     * @param query_id Identifier returned by `enqueue()`.
     * @param completion_time Completion timestamp used for SLA accounting.
     *
     * Missing ids are ignored. Callers that rely on `Metrics::completed_total`
     * and `Metrics::sla_compliance_pct` must invoke this method explicitly.
     */
    void reportCompletion(
        std::uint64_t query_id,
        std::chrono::steady_clock::time_point completion_time
            = std::chrono::steady_clock::now());

    /**
     * @brief Return a consistent snapshot of the current scheduler metrics.
     * @return Scheduler metrics copied under the internal mutex.
     */
    [[nodiscard]] Metrics metrics() const noexcept;

    /**
     * @brief Return the current queue depth.
     * @return Number of entries currently stored in the scheduler queue.
     */
    [[nodiscard]] std::size_t size() const noexcept;

    /**
     * @brief Stop accepting new entries and wake blocked waiters.
     */
    void shutdown() noexcept;

    /**
     * @brief Report whether shutdown has started.
     * @return `true` once shutdown has been requested.
     */
    [[nodiscard]] bool is_shutdown() const noexcept {
        return shutdown_.load(std::memory_order_acquire);
    }

private:
    /**
     * @brief Comparator implementing earliest-deadline-first ordering.
     */
    struct EarliestDeadlineFirst {
        /**
         * @brief Order two queued entries for the internal priority queue.
         * @param a Left-hand entry.
         * @param b Right-hand entry.
         * @return `true` when `a` should be ordered behind `b`.
         */
        bool operator()(const QueryEntry& a, const QueryEntry& b) const {
            if (a.deadline != b.deadline) {
                return a.deadline > b.deadline;
            }
            return a.id > b.id;
        }
    };

    using PQueue = std::priority_queue<QueryEntry,
                                       std::vector<QueryEntry>,
                                       EarliestDeadlineFirst>;

    Config cfg_;

    mutable std::mutex      mutex_;
    std::condition_variable enqueue_cv_;  ///< Notified when capacity becomes available.
    std::condition_variable dequeue_cv_;  ///< Notified when items arrive or shutdown begins.

    PQueue                queue_;
    std::atomic<bool>     shutdown_{false};
    std::atomic<uint64_t> next_id_{1};

    std::uint64_t total_enqueued_      = 0;
    std::uint64_t total_dequeued_      = 0;
    std::uint64_t total_shed_          = 0;
    std::uint64_t completed_in_sla_    = 0;
    std::uint64_t completed_total_     = 0;
    double        enqueue_latency_sum_ = 0.0;
    double        dequeue_latency_sum_ = 0.0;

    std::size_t   count_high_          = 0;
    std::size_t   count_medium_        = 0;
    std::size_t   count_low_           = 0;

    std::unordered_map<std::uint64_t,
        std::chrono::steady_clock::time_point> pending_deadlines_;
};

}  // namespace themis::execution
