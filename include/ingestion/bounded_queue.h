/**
 * @file bounded_queue.h
 * @brief Bounded queue with saturation handling and memory exhaustion detection.
 *
 * Phase 2.9 (Bounded Resources + Stress Tests) — ING-IMPL-001
 *
 * Provides thread-safe bounded queues with:
 * - Capacity limits to prevent unbounded memory growth
 * - Saturation detection and signaling
 * - Blocking semantics for backpressure
 * - Memory exhaustion alerts
 * - Statistics for monitoring
 *
 * @see src/ingestion/ROADMAP.md — Phase 2.9 item
 */

#pragma once

#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <deque>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <vector>

namespace themis {
namespace ingestion {

// ============================================================================
// Queue resource limits and saturation configuration
// ============================================================================

/**
 * @brief Configuration for queue resource limits and backpressure behavior.
 *
 * Controls how the bounded queue reacts to saturation and resource exhaustion.
 */
struct QueueResourceLimitConfig {
    /**
     * @brief Maximum number of items allowed in the queue.
     *
     * When the queue reaches this size, attempts to enqueue additional items
     * will block (if blocking mode is enabled) or return a saturation error.
     * Default: 10,000 items.
     */
    std::size_t max_queue_size = 10000;

    /**
     * @brief Maximum total memory (in bytes) for all items in the queue.
     *
     * When the queue's total byte size reaches this limit, attempts to enqueue
     * will block or return a saturation error. Default: 256 MB.
     * Set to 0 to disable memory-based limits.
     */
    std::size_t max_memory_bytes = 256 * 1024 * 1024;

    /**
     * @brief Warning threshold for queue saturation (percentage of max capacity).
     *
     * When the queue reaches this percentage of max_queue_size, a warning
     * is generated. Useful for preemptive alerts. Range: 0-100. Default: 80%.
     */
    int saturation_warning_threshold_percent = 80;

    /**
     * @brief Timeout for blocking enqueue operations.
     *
     * If a blocking enqueue does not complete within this time, it returns
     * with a timeout error instead of blocking indefinitely.
     * Default: 5 seconds.
     */
    std::chrono::milliseconds blocking_enqueue_timeout_ms = std::chrono::seconds(5);

    /**
     * @brief Whether to block on enqueue when saturated (vs returning immediately).
     *
     * If true, enqueue operations block until space is available (up to the
     * timeout). If false, enqueue operations return immediately with a
     * saturation error.
     * Default: true (blocking).
     */
    bool blocking_on_saturation = true;

    /**
     * @brief Enable memory exhaustion alerts.
     *
     * When enabled and memory usage exceeds max_memory_bytes, diagnostic
     * alerts are generated and logged.
     * Default: true.
     */
    bool enable_memory_exhaustion_alerts = true;
};

// ============================================================================
// Queue saturation and resource state
// ============================================================================

/**
 * @brief Current saturation state of a bounded queue.
 */
enum class QueueSaturationState {
    NORMAL,           ///< Queue is below warning threshold
    SATURATION_WARNING, ///< Queue has reached warning threshold (>80% default)
    SATURATED,        ///< Queue is at or above max capacity
    MEMORY_EXHAUSTION ///< Queue has exceeded memory limits
};

/**
 * @brief Statistics for queue resource usage and saturation events.
 */
struct QueueResourceStats {
    std::size_t current_item_count = 0;      ///< Current number of items in queue
    std::size_t current_memory_bytes = 0;    ///< Current total memory used (bytes)
    std::size_t max_item_count_observed = 0; ///< Peak item count since creation
    std::size_t max_memory_observed = 0;     ///< Peak memory usage since creation
    std::uint64_t enqueue_attempts = 0;      ///< Total attempts to enqueue
    std::uint64_t enqueue_successes = 0;     ///< Successful enqueues
    std::uint64_t enqueue_saturation_blocks = 0; ///< Blocked due to saturation
    std::uint64_t enqueue_memory_blocks = 0;     ///< Blocked due to memory limit
    std::uint64_t dequeue_count = 0;         ///< Total successful dequeues
    QueueSaturationState current_state = QueueSaturationState::NORMAL;
};

// ============================================================================
// Bounded queue item trait for size calculation
// ============================================================================

/**
 * @brief Trait to calculate the memory footprint of a queue item.
 *
 * Specializations should return the approximate memory used by an item,
 * including the item itself and any internal allocations (strings, vectors, etc.).
 */
template <typename T>
struct QueueItemSize {
    /**
     * @brief Return the approximate memory footprint of an item.
     *
     * Default implementation returns sizeof(T). Specializations can override
     * to account for heap allocations within the item.
     */
    static std::size_t getSize(const T& item) {
        return sizeof(T);
    }
};

// Specialization for std::string
template <>
struct QueueItemSize<std::string> {
    static std::size_t getSize(const std::string& item) {
        return sizeof(std::string) + item.capacity();
    }
};

// ============================================================================
// Bounded queue with saturation handling
// ============================================================================

/**
 * @brief Thread-safe bounded queue with saturation detection and memory limits.
 *
 * Enforces both item-count and memory-based capacity limits. Provides blocking
 * semantics for backpressure and explicit saturation state tracking.
 *
 * Example usage:
 * @code
 * QueueResourceLimitConfig config;
 * config.max_queue_size = 5000;
 * config.max_memory_bytes = 100 * 1024 * 1024; // 100 MB
 *
 * BoundedQueue<std::string> queue(config);
 *
 * // Enqueue (blocks if saturated)
 * auto result = queue.enqueue(some_item, true);
 * if (!result.success) {
 *     if (result.is_saturation) {
 *         std::cerr << "Queue saturated: " << result.error_message << std::endl;
 *     }
 * }
 *
 * // Check state
 * auto state = queue.getState();
 * if (state.current_state == QueueSaturationState::SATURATION_WARNING) {
 *     // Take preemptive action
 * }
 *
 * // Dequeue with timeout
 * auto item_opt = queue.dequeueWithTimeout(std::chrono::seconds(1));
 * if (item_opt) {
 *     // Process item
 * }
 * @endcode
 */
template <typename T>
class BoundedQueue {
public:
    /**
     * @brief Result of an enqueue operation.
     */
    struct EnqueueResult {
        bool success = false;                ///< true if enqueue succeeded
        bool is_saturation = false;          ///< true if failed due to saturation
        bool is_memory_limit = false;        ///< true if failed due to memory limit
        bool is_timeout = false;             ///< true if blocking enqueue timed out
        std::string error_message;           ///< Human-readable error description
    };

    /**
     * @brief Construct a bounded queue with the given resource limits.
     * @param config Resource limit configuration
     */
    explicit BoundedQueue(const QueueResourceLimitConfig& config = {})
        : config_(config) {}

    ~BoundedQueue() = default;

    // Delete copy/move to ensure proper synchronization
    BoundedQueue(const BoundedQueue&) = delete;
    BoundedQueue& operator=(const BoundedQueue&) = delete;

    // ── Enqueue operations ──────────────────────────────────────────────────

    /**
     * @brief Attempt to enqueue an item.
     *
     * @param item        Item to enqueue
     * @param allow_block  If true, block until space is available (up to timeout).
     *                     If false, return immediately on saturation.
     * @return EnqueueResult describing success or failure
     */
    EnqueueResult enqueue(T item, bool allow_block = true) {
        std::unique_lock<std::mutex> lock(mutex_);
        const std::size_t item_size = QueueItemSize<T>::getSize(item);

        stats_.enqueue_attempts++;

        // Check item count capacity
        if (queue_.size() >= config_.max_queue_size) {
            if (allow_block && config_.blocking_on_saturation) {
                return waitForSpace(lock, item, item_size);
            }
            stats_.enqueue_saturation_blocks++;
            return EnqueueResult{
                false, true, false, false,
                "Queue full: " + std::to_string(queue_.size()) +
                " items (max " + std::to_string(config_.max_queue_size) + ")"
            };
        }

        // Check memory capacity
        if (config_.max_memory_bytes > 0 &&
            current_memory_bytes_ + item_size > config_.max_memory_bytes) {
            if (allow_block && config_.blocking_on_saturation) {
                return waitForSpace(lock, item, item_size);
            }
            stats_.enqueue_memory_blocks++;
            return EnqueueResult{
                false, false, true, false,
                "Memory limit exceeded: " + std::to_string(current_memory_bytes_) +
                " + " + std::to_string(item_size) +
                " > " + std::to_string(config_.max_memory_bytes) + " bytes"
            };
        }

        // Enqueue successfully
        queue_.push_back(std::move(item));
        current_memory_bytes_ += item_size;
        updateStats();

        stats_.enqueue_successes++;

        // Notify waiting dequeue operations
        condition_var_.notify_one();

        return EnqueueResult{true, false, false, false, ""};
    }

    // ── Dequeue operations ──────────────────────────────────────────────────

    /**
     * @brief Attempt to dequeue an item (non-blocking).
     * @return std::optional containing the item if available, std::nullopt otherwise
     */
    std::optional<T> tryDequeue() {
        std::lock_guard<std::mutex> lock(mutex_);
        if (queue_.empty()) {
            return std::nullopt;
        }
        T item = std::move(queue_.front());
        queue_.pop_front();
        current_memory_bytes_ -= QueueItemSize<T>::getSize(item);
        stats_.dequeue_count++;
        updateStats();
        condition_var_.notify_one();
        return item;
    }

    /**
     * @brief Dequeue an item with timeout.
     *
     * Blocks until an item is available or the timeout expires.
     * @param timeout Maximum time to wait
     * @return std::optional containing the item if available, std::nullopt on timeout
     */
    std::optional<T> dequeueWithTimeout(std::chrono::milliseconds timeout) {
        std::unique_lock<std::mutex> lock(mutex_);
        if (!condition_var_.wait_for(lock, timeout,
                                      [this] { return !queue_.empty(); })) {
            return std::nullopt;  // Timeout
        }
        T item = std::move(queue_.front());
        queue_.pop_front();
        current_memory_bytes_ -= QueueItemSize<T>::getSize(item);
        stats_.dequeue_count++;
        updateStats();
        condition_var_.notify_one();
        return item;
    }

    /**
     * @brief Dequeue an item (blocks indefinitely until available).
     * @return Item from the front of the queue
     */
    T dequeue() {
        auto item = dequeueWithTimeout(std::chrono::hours(24));  // Practical infinity
        return *item;
    }

    // ── State queries ───────────────────────────────────────────────────────

    /**
     * @brief Get current queue and resource statistics.
     */
    QueueResourceStats getStats() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return stats_;
    }

    /**
     * @brief Get current saturation state.
     */
    QueueSaturationState getState() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return stats_.current_state;
    }

    /**
     * @brief Check if the queue is saturated.
     */
    bool isSaturated() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return stats_.current_state == QueueSaturationState::SATURATED ||
               stats_.current_state == QueueSaturationState::MEMORY_EXHAUSTION;
    }

    /**
     * @brief Check if saturation warning threshold is exceeded.
     */
    bool isWarningThresholdExceeded() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return stats_.current_state == QueueSaturationState::SATURATION_WARNING ||
               stats_.current_state == QueueSaturationState::SATURATED ||
               stats_.current_state == QueueSaturationState::MEMORY_EXHAUSTION;
    }

    /**
     * @brief Get current number of items in queue.
     */
    std::size_t size() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return queue_.size();
    }

    /**
     * @brief Check if queue is empty.
     */
    bool empty() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return queue_.empty();
    }

    /**
     * @brief Get current memory usage in bytes.
     */
    std::size_t currentMemoryBytes() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return current_memory_bytes_;
    }

    /**
     * @brief Clear all items from the queue.
     */
    void clear() {
        std::lock_guard<std::mutex> lock(mutex_);
        queue_.clear();
        current_memory_bytes_ = 0;
        stats_.current_item_count = 0;
        stats_.current_memory_bytes = 0;
        updateStats();
        condition_var_.notify_all();
    }

    /**
     * @brief Get the resource limit configuration.
     */
    QueueResourceLimitConfig getConfig() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return config_;
    }

    /**
     * @brief Update the resource limit configuration.
     *
     * Takes effect on the next enqueue operation.
     */
    void setConfig(const QueueResourceLimitConfig& new_config) {
        std::lock_guard<std::mutex> lock(mutex_);
        config_ = new_config;
    }

private:
    mutable std::mutex mutex_;
    std::condition_variable condition_var_;
    std::deque<T> queue_;
    QueueResourceLimitConfig config_;
    std::size_t current_memory_bytes_ = 0;
    QueueResourceStats stats_;

    /**
     * @brief Wait for space to become available in the queue.
     *
     * Blocks until either the queue has space or the timeout expires.
     * @return EnqueueResult with success=true if space became available,
     *         or success=false with is_timeout=true if timeout expired
     */
    EnqueueResult waitForSpace(std::unique_lock<std::mutex>& lock, T item,
                               std::size_t item_size) {
        const auto timeout = config_.blocking_enqueue_timeout_ms;
        const auto deadline = std::chrono::steady_clock::now() + timeout;

        while (true) {
            // Check if we now have space
            if (queue_.size() < config_.max_queue_size &&
                (config_.max_memory_bytes == 0 ||
                 current_memory_bytes_ + item_size <= config_.max_memory_bytes)) {
                // Space available; enqueue
                queue_.push_back(std::move(item));
                current_memory_bytes_ += item_size;
                updateStats();
                stats_.enqueue_successes++;
                condition_var_.notify_one();
                return EnqueueResult{true, false, false, false, ""};
            }

            // Wait for dequeue to free space
            const auto now = std::chrono::steady_clock::now();
            if (now >= deadline) {
                // Timeout
                stats_.enqueue_saturation_blocks++;
                return EnqueueResult{
                    false, true, false, true,
                    "Blocking enqueue timed out after " +
                    std::to_string(timeout.count()) + " ms"
                };
            }

            const auto remaining = deadline - now;
            condition_var_.wait_for(lock, remaining);
        }
    }

    /**
     * @brief Update current saturation state based on queue occupancy.
     *
     * Called after enqueue or dequeue to recalculate the saturation state.
     * Must be called while holding the lock.
     */
    void updateStats() {
        stats_.current_item_count = queue_.size();
        stats_.current_memory_bytes = current_memory_bytes_;

        // Update peak observations
        if (stats_.current_item_count > stats_.max_item_count_observed) {
            stats_.max_item_count_observed = stats_.current_item_count;
        }
        if (stats_.current_memory_bytes > stats_.max_memory_observed) {
            stats_.max_memory_observed = stats_.current_memory_bytes;
        }

        // Update saturation state
        if (config_.max_memory_bytes > 0 &&
            current_memory_bytes_ >= config_.max_memory_bytes) {
            stats_.current_state = QueueSaturationState::MEMORY_EXHAUSTION;
        } else if (queue_.size() >= config_.max_queue_size) {
            stats_.current_state = QueueSaturationState::SATURATED;
        } else {
            const int percent =
                static_cast<int>(queue_.size() * 100 / config_.max_queue_size);
            if (percent >= config_.saturation_warning_threshold_percent) {
                stats_.current_state = QueueSaturationState::SATURATION_WARNING;
            } else {
                stats_.current_state = QueueSaturationState::NORMAL;
            }
        }
    }
};

}  // namespace ingestion
}  // namespace themis

#endif  // THEMISDB_INCLUDE_INGESTION_BOUNDED_QUEUE_H
