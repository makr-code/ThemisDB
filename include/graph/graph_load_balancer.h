/**
 * @file graph_load_balancer.h
 * @brief Phase-3 load balancing utilities for the graph module.
 *
 * Provides:
 *  - @ref themis::graph::GraphQueryScheduler  – priority-based query scheduler
 *  - @ref themis::graph::GraphShardBalancer   – latency-aware shard selection
 *
 * Design goals:
 *  - Priority-queue scheduling with high/normal/low priority levels.
 *  - Least-loaded and round-robin shard selection strategies.
 *  - Latency tracking per shard for adaptive routing.
 *  - Thread-safe for concurrent producers and consumers.
 *
 * @version 1.9.0
 * @note Maturity: 🟢 PRODUCTION-READY
 */

#pragma once

#include <algorithm>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <functional>
#include <mutex>
#include <optional>
#include <queue>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

namespace themis {
namespace graph {

// ─────────────────────────────────────────────────────────────────────────────
// GraphQueryScheduler
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Priority-based query scheduler for graph operations.
 *
 * Queries are submitted with a @ref Priority level.  The scheduler
 * dispatches the highest-priority pending query first.  Within the same
 * priority level, ordering is FIFO (insertion order).
 *
 * The scheduler is primarily a queue; actual execution is performed by the
 * caller who dequeues tasks via @ref next().
 */
class GraphQueryScheduler {
public:
    /**
     * @brief Query priority levels (higher numeric value = higher priority).
     */
    enum class Priority : int {
        LOW    = 0,
        NORMAL = 1,
        HIGH   = 2,
        URGENT = 3
    };

    /**
     * @brief A scheduled query task.
     */
    struct QueryTask {
        uint64_t  id = 0;       ///< Unique task identifier
        Priority  priority; ///< Scheduling priority
        std::string label;  ///< Human-readable query label
        std::function<void()> work; ///< The actual work to execute
        std::chrono::steady_clock::time_point enqueued_at; ///< Enqueue timestamp
    };

    /**
     * @brief Construct a scheduler with optional maximum queue depth.
     *
     * @param max_queue_depth Maximum pending tasks (0 = unlimited).
     */
    explicit GraphQueryScheduler(size_t max_queue_depth = 0)
        : max_queue_depth_(max_queue_depth) {}

    /**
     * @brief Submit a query task for scheduling.
     *
     * @param label    Descriptive label for the query.
     * @param priority Scheduling priority.
     * @param work     Callable to execute when dequeued.
     * @return Unique task ID.
     * @throws std::overflow_error if the queue is full (max_queue_depth > 0).
     */
    uint64_t submit(std::string label,
                    Priority priority,
                    std::function<void()> work) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (max_queue_depth_ > 0 && queue_.size() >= max_queue_depth_) {
            throw std::overflow_error("GraphQueryScheduler: queue capacity exceeded");
        }
        const uint64_t id = next_id_++;
        queue_.push(QueryTask{id, priority, std::move(label),
                              std::move(work),
                              std::chrono::steady_clock::now()});
        cv_.notify_one();
        ++submitted_count_;
        return id;
    }

    /**
     * @brief Dequeue and return the highest-priority pending task.
     *
     * @param block If true, block until a task is available.
     * @return The task, or std::nullopt if the queue is empty (non-blocking mode).
     */
    std::optional<QueryTask> next(bool block = false) {
        std::unique_lock<std::mutex> lock(mutex_);
        if (block) {
            cv_.wait(lock, [this] { return !queue_.empty() || stopped_; });
        }
        if (queue_.empty()) {
          return std::nullopt;
        }
        QueryTask task = std::move(const_cast<QueryTask&>(queue_.top()));
        queue_.pop();
        ++dispatched_count_;
        return task;
    }

    /**
     * @brief Execute the next queued task on the calling thread.
     *
     * @param block If true, block until a task is available.
     * @return true if a task was executed; false if the queue was empty.
     */
    bool executeNext(bool block = false) {
        auto task = next(block);
        if (!task) {
          return false;
        }
        if (task->work) {
          task->work();
        }
        ++completed_count_;
        return true;
    }

    /**
     * @brief Return the current number of pending tasks.
     * @return Pending task count.
     */
    size_t pending() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return queue_.size();
    }

    /**
     * @brief Return the total number of submitted tasks.
     * @return Submitted count.
     */
    uint64_t submittedCount() const {
        return submitted_count_.load(std::memory_order_relaxed);
    }

    /**
     * @brief Return the total number of dispatched tasks.
     * @return Dispatched count.
     */
    uint64_t dispatchedCount() const {
        return dispatched_count_.load(std::memory_order_relaxed);
    }

    /**
     * @brief Return the total number of completed tasks.
     * @return Completed count.
     */
    uint64_t completedCount() const {
        return completed_count_.load(std::memory_order_relaxed);
    }

    /**
     * @brief Signal scheduler to stop (unblocks any waiting next() calls).
     */
    void stop() {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            stopped_ = true;
        }
        cv_.notify_all();
    }

    /**
     * @brief Clear all pending tasks without executing them.
     * @return Number of tasks cleared.
     */
    size_t clearPending() {
        std::lock_guard<std::mutex> lock(mutex_);
        size_t count = queue_.size();
        while (!queue_.empty()) {
          queue_.pop();
        }
        return count;
    }

private:
    struct TaskCompare {
        bool operator()(const QueryTask& a, const QueryTask& b) const {
            if (a.priority != b.priority)
                return static_cast<int>(a.priority) < static_cast<int>(b.priority);
            // FIFO within same priority
            return a.id > b.id;
        }
    };

    mutable std::mutex mutex_;
    std::condition_variable cv_;
    std::priority_queue<QueryTask, std::vector<QueryTask>, TaskCompare> queue_;
    uint64_t  next_id_          = 0;
    bool      stopped_          = false;
    size_t    max_queue_depth_  = 0;
    std::atomic<uint64_t> submitted_count_{0};
    std::atomic<uint64_t> dispatched_count_{0};
    std::atomic<uint64_t> completed_count_{0};
};

// ─────────────────────────────────────────────────────────────────────────────
// GraphShardBalancer
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Latency-aware shard selection balancer.
 *
 * Tracks per-shard load (in-flight query count) and exponential moving
 * average (EMA) of observed query latencies.  Supports two selection
 * strategies:
 *  - @ref Strategy::ROUND_ROBIN    – cycles through shards in order.
 *  - @ref Strategy::LEAST_LOADED   – picks the shard with the fewest in-flight queries.
 *  - @ref Strategy::LATENCY_AWARE  – picks the shard with the lowest EMA latency.
 */
class GraphShardBalancer {
public:
    /**
     * @brief Shard selection strategy.
     */
    enum class Strategy {
        ROUND_ROBIN,   ///< Cycle through shards in insertion order
        LEAST_LOADED,  ///< Select shard with fewest in-flight queries
        LATENCY_AWARE  ///< Select shard with lowest EMA latency
    };

    /**
     * @brief Per-shard statistics snapshot.
     */
    struct ShardStats {
        std::string shard_id;         ///< Shard identifier
        uint64_t    inflight     = 0; ///< Currently in-flight query count
        uint64_t    total_routed = 0; ///< Total queries routed to this shard
        double      ema_latency_ms = 0.0; ///< EMA of observed latencies
        bool        healthy      = true;  ///< Health flag

        /**
         * @brief Update EMA latency with a new observation.
         * @param latency_ms Observed query latency in milliseconds.
         */
        void recordLatency(double latency_ms) {
            static constexpr double kAlpha = 0.15;
            if (total_routed == 0) {
                ema_latency_ms = latency_ms;
            } else {
                ema_latency_ms = kAlpha * latency_ms + (1.0 - kAlpha) * ema_latency_ms;
            }
        }
    };

    /**
     * @brief Construct a balancer with the given strategy and shard list.
     *
     * @param strategy   Shard selection strategy.
     * @param shard_ids  List of shard identifiers to balance across.
     */
    explicit GraphShardBalancer(Strategy strategy,
                                 std::vector<std::string> shard_ids)
        : strategy_(strategy) {
        if (shard_ids.empty())
            throw std::invalid_argument("GraphShardBalancer: shard list must not be empty");
        std::lock_guard<std::mutex> lock(mutex_);
        for (auto& id : shard_ids) {
            shard_order_.push_back(id);
            stats_[id].shard_id = id;
        }
    }

    /**
     * @brief Select the best shard for the next query.
     *
     * @return Selected shard ID, or empty string if all shards are unhealthy.
     */
    std::string selectShard() {
        std::lock_guard<std::mutex> lock(mutex_);
        if (shard_order_.empty()) return {};

        switch (strategy_) {
            case Strategy::ROUND_ROBIN:
                return selectRoundRobin();
            case Strategy::LEAST_LOADED:
                return selectLeastLoaded();
            case Strategy::LATENCY_AWARE:
                return selectLatencyAware();
        }
        return selectRoundRobin();
    }

    /**
     * @brief Record that a query was routed to a shard (increments in-flight).
     *
     * @param shard_id Target shard.
     */
    void onQueryStarted(const std::string& shard_id) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = stats_.find(shard_id);
        if (it == stats_.end()) {
          return;
        }
        ++it->second.inflight;
        ++it->second.total_routed;
    }

    /**
     * @brief Record that a query completed on a shard.
     *
     * Decrements in-flight count and updates EMA latency.
     *
     * @param shard_id   Target shard.
     * @param latency_ms Observed query latency in milliseconds.
     */
    void onQueryCompleted(const std::string& shard_id, double latency_ms) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = stats_.find(shard_id);
        if (it == stats_.end()) {
          return;
        }
        if (it->second.inflight > 0) {
          --it->second.inflight;
        }
        it->second.recordLatency(latency_ms);
    }

    /**
     * @brief Mark a shard as healthy or unhealthy.
     *
     * Unhealthy shards are excluded from selection.
     *
     * @param shard_id Shard identifier.
     * @param healthy  true to mark healthy; false to mark unhealthy.
     */
    void setShardHealth(const std::string& shard_id, bool healthy) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = stats_.find(shard_id);
        if (it != stats_.end()) {
          it->second.healthy = healthy;
        }
    }

    /**
     * @brief Add a new shard to the balancer.
     *
     * @param shard_id New shard identifier.
     */
    void addShard(const std::string& shard_id) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (stats_.count(shard_id)) {
          return;
        }
        shard_order_.push_back(shard_id);
        stats_[shard_id].shard_id = shard_id;
    }

    /**
     * @brief Remove a shard from the balancer.
     *
     * @param shard_id Shard identifier to remove.
     * @return true if the shard was present and removed.
     */
    bool removeShard(const std::string& shard_id) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = stats_.find(shard_id);
        if (it == stats_.end()) {
          return false;
        }
        shard_order_.erase(
            std::remove(shard_order_.begin(), shard_order_.end(), shard_id),
            shard_order_.end());
        stats_.erase(it);
        // Reset round-robin index if it's now out of range.
        if (rr_index_ >= shard_order_.size()) {
          rr_index_ = 0;
        }
        return true;
    }

    /**
     * @brief Return a snapshot of all per-shard statistics.
     * @return Vector of ShardStats, one per registered shard.
     */
    std::vector<ShardStats> allStats() const {
        std::lock_guard<std::mutex> lock(mutex_);
        std::vector<ShardStats> out = {};

        out.reserve(stats_.size());
        for (const auto& id : shard_order_) {
            auto it = stats_.find(id);
            if (it != stats_.end()) {
              out.push_back(it->second);
            }
        }
        return out;
    }

    /**
     * @brief Return statistics for a specific shard.
     *
     * @param shard_id Shard identifier.
     * @return ShardStats, or std::nullopt if the shard is unknown.
     */
    std::optional<ShardStats> shardStats(const std::string& shard_id) const {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = stats_.find(shard_id);
        if (it == stats_.end()) {
          return std::nullopt;
        }
        return it->second;
    }

    /**
     * @brief Return the current strategy.
     * @return Selection strategy.
     */
    Strategy strategy() const { return strategy_; }

    /**
     * @brief Update the selection strategy at runtime.
     * @param strategy New strategy.
     */
    void setStrategy(Strategy strategy) { strategy_ = strategy; }

    /**
     * @brief Return the number of registered shards.
     * @return Shard count.
     */
    size_t shardCount() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return shard_order_.size();
    }

private:
    // All three selectors must be called under mutex_.

    std::string selectRoundRobin() {
        for (size_t i = 0; i < shard_order_.size(); ++i) {
            size_t idx     = rr_index_ % shard_order_.size();
            rr_index_      = (rr_index_ + 1) % shard_order_.size();
            const auto& id = shard_order_[idx];
            if (stats_.at(id).healthy) {
              return id;
            }
        }
        return {};
    }

    std::string selectLeastLoaded() {
        std::string best;
        uint64_t    min_load = UINT64_MAX;
        for (const auto& id : shard_order_) {
            const auto& s = stats_.at(id);
            if (!s.healthy) {
              continue;
            }
            if (s.inflight < min_load) {
                min_load = s.inflight;
                best     = id;
            }
        }
        return best;
    }

    std::string selectLatencyAware() {
        std::string best;
        double      min_latency = std::numeric_limits<double>::max();
        for (const auto& id : shard_order_) {
            const auto& s = stats_.at(id);
            if (!s.healthy) {
              continue;
            }
            const double lat = (s.total_routed == 0) ? 0.0 : s.ema_latency_ms;
            if (lat < min_latency) {
                min_latency = lat;
                best        = id;
            }
        }
        return best;
    }

    mutable std::mutex  mutex_;
    Strategy            strategy_;
    std::vector<std::string>                   shard_order_;
    std::unordered_map<std::string, ShardStats> stats_;
    size_t              rr_index_ = 0;
};

} // namespace graph
} // namespace themis
