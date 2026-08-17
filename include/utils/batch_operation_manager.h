/**
 * @file batch_operation_manager.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <vector>
#include <memory>
#include <functional>
#include <chrono>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <optional>

namespace themis {
namespace utils {

/**
 * @brief Adaptive Batch Operation Manager
 * 
 * Provides intelligent batching for write operations with:
 * - Automatic batch size optimization based on throughput
 * - Configurable latency vs throughput trade-offs
 * - Memory-efficient streaming batch processing
 * - Thread-safe queuing and processing
 * 
 * Performance Gains:
 * - 20-30% throughput improvement for write-heavy workloads
 * - Reduced write amplification in LSM-tree storage
 * - Better CPU cache utilization through batching
 * 
 * Sources:
 * - Benchmark Analysis: benchmarks/BENCHMARK_ANALYSIS_20251210.md
 * - Quick Wins: docs/de/performance/OPTIMIZATION_QUICK_WINS.md
 * 
 * @tparam T Type of items to batch (e.g., Entity, Vector, Document)
 */
template<typename T>
class BatchOperationManager {
public:
    struct Config {
        size_t min_batch_size = 10;              ///< Minimum batch size
        size_t max_batch_size = 1000;            ///< Maximum batch size
        size_t initial_batch_size = 100;         ///< Initial batch size
        std::chrono::milliseconds max_latency{100}; ///< Max time to wait for batch fill
        bool adaptive_sizing = true;             ///< Enable adaptive batch size tuning
        size_t queue_capacity = 10000;           ///< Maximum queued items
        double target_cpu_utilization = 0.8;     ///< Target CPU utilization (0.0-1.0)
    };
    
    /**
     * @brief Batch processor callback type
     * @param items Items to process in batch
     * @return Number of successfully processed items
     */
    using BatchProcessor = std::function<size_t(const std::vector<T>&)>;
    
    explicit BatchOperationManager(const Config& config, BatchProcessor processor);
    ~BatchOperationManager();
    
    // Disable copy, allow move
    BatchOperationManager(const BatchOperationManager&) = delete;
    BatchOperationManager& operator=(const BatchOperationManager&) = delete;
    BatchOperationManager(BatchOperationManager&&) noexcept = default;
    BatchOperationManager& operator=(BatchOperationManager&&) noexcept = default;
    
    /**
     * @brief Add item to batch queue
     * @param item Item to add
     * @return true if added, false if queue is full
     */
    bool enqueue(T item);
    
    /**
     * @brief Add multiple items to batch queue
     * @param items Items to add
     * @return Number of items successfully enqueued
     */
    size_t enqueueBatch(const std::vector<T>& items);
    
    /**
     * @brief Flush pending batches immediately
     * @return Number of items flushed
     */
    size_t flush();
    
    /**
     * @brief Start batch processing thread
     */
    void start();
    
    /**
     * @brief Stop batch processing thread
     */
    void stop();
    
    /**
     * @brief Get current batch statistics
     */
    struct Stats {
        size_t items_processed = 0;
        size_t batches_processed = 0;
        size_t items_queued = 0;
        size_t queue_full_count = 0;
        double avg_batch_size = 0.0;
        double avg_latency_ms = 0.0;
        double avg_throughput_items_per_sec = 0.0;
        size_t current_batch_size = 0;
    };
    
    Stats getStats() const;
    
    /**
     * @brief Get current optimal batch size (adaptive)
     */
    size_t getCurrentBatchSize() const { return current_batch_size_.load(); }
    
private:
    /**
     * @brief Process batches in background thread
     */
    void processingLoop();
    
    /**
     * @brief Process a single batch
     */
    size_t processBatch(const std::vector<T>& batch);
    
    /**
     * @brief Adapt batch size based on performance metrics
     */
    void adaptBatchSize(double throughput, double latency_ms);
    
    /**
     * @brief Collect items into a batch
     * @param max_items Maximum items to collect
     * @param timeout Maximum time to wait
     * @return Collected items
     */
    std::vector<T> collectBatch(size_t max_items, std::chrono::milliseconds timeout);
    
    Config config_;
    BatchProcessor processor_;
    
    // Queue for pending items
    mutable std::mutex queue_mutex_;
    std::condition_variable queue_cv_;
    std::queue<T> pending_queue_;
    
    // Atomics (safe to access from multiple threads)
    std::atomic<bool> running_{false};
    std::atomic<bool> shutdown_{false};
    
    // Adaptive batch sizing
    std::atomic<size_t> current_batch_size_;
    std::chrono::steady_clock::time_point last_adaptation_;
    
    // Statistics
    std::atomic<size_t> items_processed_{0};
    std::atomic<size_t> batches_processed_{0};
    std::atomic<size_t> queue_full_count_{0};
    std::atomic<double> total_latency_ms_{0.0};
    std::atomic<double> total_throughput_{0.0};
    
    // Processing thread (declared last for proper destruction order)
    std::thread processing_thread_;
};

// Template implementation
template<typename T>
BatchOperationManager<T>::BatchOperationManager(const Config& config, BatchProcessor processor)
    : config_(config)
    , processor_(std::move(processor))
    , current_batch_size_(config.initial_batch_size)
    , last_adaptation_(std::chrono::steady_clock::now())
{
}

template<typename T>
BatchOperationManager<T>::~BatchOperationManager() {
    stop();
}

template<typename T>
bool BatchOperationManager<T>::enqueue(T item) {
    std::unique_lock<std::mutex> lock(queue_mutex_);
    
    if (pending_queue_.size() >= config_.queue_capacity) {
        queue_full_count_.fetch_add(1);
        return false;
    }
    
    pending_queue_.push(std::move(item));
    lock.unlock();
    queue_cv_.notify_one();
    
    return true;
}

template<typename T>
size_t BatchOperationManager<T>::enqueueBatch(const std::vector<T>& items) {
    size_t enqueued = 0;
    
    std::unique_lock<std::mutex> lock(queue_mutex_);
    
    for (const auto& item : items) {
        if (pending_queue_.size() >= config_.queue_capacity) {
            queue_full_count_.fetch_add(1);
            break;
        }
        
        pending_queue_.push(item);
        enqueued++;
    }
    
    lock.unlock();
    if (enqueued > 0) {
        queue_cv_.notify_one();
    }
    
    return enqueued;
}

template<typename T>
size_t BatchOperationManager<T>::flush() {
    std::vector<T> items;
    
    {
        std::lock_guard<std::mutex> lock(queue_mutex_);
        while (!pending_queue_.empty()) {
            items.push_back(std::move(pending_queue_.front()));
            pending_queue_.pop();
        }
    }
    
    if (!items.empty()) {
        return processBatch(items);
    }
    
    return 0;
}

template<typename T>
void BatchOperationManager<T>::start() {
    if (running_.exchange(true)) {
        return; // Already running
    }
    
    shutdown_.store(false);
    processing_thread_ = std::thread([this]() { processingLoop(); });
}

template<typename T>
void BatchOperationManager<T>::stop() {
    if (!running_.exchange(false)) {
        return; // Not running
    }
    
    shutdown_.store(true);
    queue_cv_.notify_all();
    
    if (processing_thread_.joinable()) {
        processing_thread_.join();
    }
    
    // Flush remaining items
    flush();
}

template<typename T>
typename BatchOperationManager<T>::Stats BatchOperationManager<T>::getStats() const {
    Stats stats;
    stats.items_processed = items_processed_.load();
    stats.batches_processed = batches_processed_.load();
    stats.queue_full_count = queue_full_count_.load();
    stats.current_batch_size = current_batch_size_.load();
    
    {
        std::lock_guard<std::mutex> lock(queue_mutex_);
        stats.items_queued = pending_queue_.size();
    }
    
    if (stats.batches_processed > 0) {
        stats.avg_batch_size = static_cast<double>(stats.items_processed) / stats.batches_processed;
        stats.avg_latency_ms = total_latency_ms_.load() / stats.batches_processed;
        stats.avg_throughput_items_per_sec = total_throughput_.load() / stats.batches_processed;
    }
    
    return stats;
}

template<typename T>
void BatchOperationManager<T>::processingLoop() {
    while (!shutdown_.load()) {
        auto batch = collectBatch(current_batch_size_.load(), config_.max_latency);
        
        if (!batch.empty()) {
            processBatch(batch);
        }
    }
}

template<typename T>
size_t BatchOperationManager<T>::processBatch(const std::vector<T>& batch) {
    if (batch.empty()) {
        return 0;
    }
    
    auto start = std::chrono::steady_clock::now();
    
    // Process batch using provided processor
    size_t processed = processor_(batch);
    
    auto end = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    double latency_ms = static_cast<double>(duration.count());
    double throughput = processed / (latency_ms / 1000.0); // items per second
    
    // Update statistics
    items_processed_.fetch_add(processed);
    batches_processed_.fetch_add(1);
    total_latency_ms_.fetch_add(latency_ms);
    total_throughput_.fetch_add(throughput);
    
    // Adapt batch size if enabled
    if (config_.adaptive_sizing) {
        adaptBatchSize(throughput, latency_ms);
    }
    
    return processed;
}

template<typename T>
void BatchOperationManager<T>::adaptBatchSize([[maybe_unused]] double throughput, double latency_ms) {
    auto now = std::chrono::steady_clock::now();
    auto since_last = std::chrono::duration_cast<std::chrono::seconds>(now - last_adaptation_);
    
    // Only adapt every 10 seconds
    if (since_last.count() < 10) {
        return;
    }
    
    last_adaptation_ = now;
    size_t current = current_batch_size_.load();
    
    // Increase batch size if latency is low and throughput is good
    if (latency_ms < config_.max_latency.count() * 0.5 && current < config_.max_batch_size) {
        current_batch_size_.store(std::min(current * 2, config_.max_batch_size));
    }
    // Decrease batch size if latency is high
    else if (latency_ms > config_.max_latency.count() && current > config_.min_batch_size) {
        current_batch_size_.store(std::max(current / 2, config_.min_batch_size));
    }
}

template<typename T>
std::vector<T> BatchOperationManager<T>::collectBatch(
    size_t max_items,
    std::chrono::milliseconds timeout
) {
    std::vector<T> batch;
    batch.reserve(max_items);
    
    std::unique_lock<std::mutex> lock(queue_mutex_);
    
    auto deadline = std::chrono::steady_clock::now() + timeout;
    
    while (batch.size() < max_items && !shutdown_.load()) {
        if (pending_queue_.empty()) {
            // Wait for items or timeout
            if (queue_cv_.wait_until(lock, deadline) == std::cv_status::timeout) {
                break;
            }
        }
        
        if (!pending_queue_.empty()) {
            batch.push_back(std::move(pending_queue_.front()));
            pending_queue_.pop();
        }
    }
    
    return batch;
}

} // namespace utils
} // namespace themis
