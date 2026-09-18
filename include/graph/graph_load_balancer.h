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

class GraphQueryScheduler {
public:
    enum class Priority : int {
        LOW    = 0,
        NORMAL = 1,
        HIGH   = 2,
        URGENT = 3
    };

    struct QueryTask {
        uint64_t  id = 0;       ///< Unique task identifier
        Priority  priority; ///< Scheduling priority
        std::string label;  ///< Human-readable query label
        std::function<void()> work; ///< The actual work to execute
        std::chrono::steady_clock::time_point enqueued_at; ///< Enqueue timestamp
    };

    explicit GraphQueryScheduler(size_t max_queue_depth = 0)
        : max_queue_depth_(max_queue_depth) {}

    uint64_t submit(std::string label,
                    Priority priority,
                    std::function<void()> work) {
        /**
         * @brief Lock.
         * @param[in] mutex_ Input parameter.
         * @return Return value.
         */
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

    std::optional<QueryTask> next(bool block = false) {
        /**
         * @brief Lock.
         * @param[in] mutex_ Input parameter.
         * @return Return value.
         */
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

    size_t pending() const {
        /**
         * @brief Lock.
         * @param[in] mutex_ Input parameter.
         * @return Return value.
         */
        std::lock_guard<std::mutex> lock(mutex_);
        return queue_.size();
    }

    uint64_t submittedCount() const {
        return submitted_count_.load(std::memory_order_relaxed);
    }

    uint64_t dispatchedCount() const {
        return dispatched_count_.load(std::memory_order_relaxed);
    }

    uint64_t completedCount() const {
        return completed_count_.load(std::memory_order_relaxed);
    }

    /**
     * @brief Stop.
     * @details Calls: lock(), notify_all().
     */
    void stop() {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            stopped_ = true;
        }
        cv_.notify_all();
    }

    /**
     * @brief Clear Pending.
     * @return Return value.
     * @details Calls: lock(), size(), empty(), pop().
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

class GraphShardBalancer {
public:
    enum class Strategy {
        ROUND_ROBIN,   ///< Cycle through shards in insertion order
        LEAST_LOADED,  ///< Select shard with fewest in-flight queries
        LATENCY_AWARE  ///< Select shard with lowest EMA latency
    };

    struct ShardStats {
        std::string shard_id;         ///< Shard identifier
        uint64_t    inflight     = 0; ///< Currently in-flight query count
        uint64_t    total_routed = 0; ///< Total queries routed to this shard
        double      ema_latency_ms = 0.0; ///< EMA of observed latencies
        bool        healthy      = true;  ///< Health flag

        /**
         * @brief Record Latency.
         * @param[in] latency_ms Input parameter.
         * @details Implements recordLatency without additional internal calls.
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
     * @brief Graph Shard Balancer.
     * @param[in] strategy Input parameter.
     * @param[in] shard_ids Input parameter.
     * @return Return value.
     */
    explicit GraphShardBalancer(Strategy strategy,
                                 std::vector<std::string> shard_ids)
        : strategy_(strategy) {
        if (shard_ids.empty())
            throw std::invalid_argument("GraphShardBalancer: shard list must not be empty");
        /**
         * @brief Lock.
         * @param[in] mutex_ Input parameter.
         * @return Return value.
         */
        std::lock_guard<std::mutex> lock(mutex_);
        for (auto& id : shard_ids) {
            shard_order_.push_back(id);
            stats_[id].shard_id = id;
        }
    }

    /**
     * @brief Select Shard.
     * @return Return value.
     * @details Calls: lock(), empty(), selectRoundRobin(), selectLeastLoaded(), selectLatencyAware().
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
     * @brief On Query Started.
     * @param[in] shard_id Identifier of the shard.
     * @details Calls: lock(), find(), end().
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
     * @brief On Query Completed.
     * @param[in] shard_id Identifier of the shard.
     * @param[in] latency_ms Input parameter.
     * @details Calls: lock(), find(), end(), recordLatency().
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
     * @brief Set Shard Health.
     * @param[in] shard_id Identifier of the shard.
     * @param[in] healthy Input parameter.
     * @details Calls: lock(), find(), end().
     */
    void setShardHealth(const std::string& shard_id, bool healthy) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = stats_.find(shard_id);
        if (it != stats_.end()) {
          it->second.healthy = healthy;
        }
    }

    /**
     * @brief Add Shard.
     * @param[in] shard_id Identifier of the shard.
     * @details Calls: lock(), count(), push_back().
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
     * @brief Remove Shard.
     * @param[in] shard_id Identifier of the shard.
     * @return True when the operation succeeds.
     * @details Calls: lock(), find(), end(), erase(), std::remove(), begin(), size().
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

    std::vector<ShardStats> allStats() const {
        /**
         * @brief Lock.
         * @param[in] mutex_ Input parameter.
         * @return Return value.
         */
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

    std::optional<ShardStats> shardStats(const std::string& shard_id) const {
        /**
         * @brief Lock.
         * @param[in] mutex_ Input parameter.
         * @return Return value.
         */
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = stats_.find(shard_id);
        if (it == stats_.end()) {
          return std::nullopt;
        }
        return it->second;
    }

    Strategy strategy() const { return strategy_; }

    /**
     * @brief Set Strategy.
     * @param[in] strategy Input parameter.
     * @details Implements setStrategy without additional internal calls.
     */
    void setStrategy(Strategy strategy) { strategy_ = strategy; }

    size_t shardCount() const {
        /**
         * @brief Lock.
         * @param[in] mutex_ Input parameter.
         * @return Return value.
         */
        std::lock_guard<std::mutex> lock(mutex_);
        return shard_order_.size();
    }

private:
    /**
     * @brief All three selectors must be called under mutex_.
     * @return Return value.
     * @details Calls: size(), at().
     */

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

    /**
     * @brief Select Least Loaded.
     * @return Return value.
     * @details Calls: at().
     */
    std::string selectLeastLoaded() {
        std::string best = {};
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

    /**
     * @brief Select Latency Aware.
     * @return Return value.
     * @details Calls: max(), at().
     */
    std::string selectLatencyAware() {
        std::string best = {};
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
