/**
 * @file graph_resource_pool.h
 * @brief Phase-3 resource pooling utilities for the graph module.
 *
 * Provides:
 *  - @ref themis::graph::GraphConnectionPool  – bounded connection-like resource pool
 *  - @ref themis::graph::GraphThreadPool      – fixed-size thread pool for graph tasks
 *  - @ref themis::graph::GraphBufferPool      – pooled fixed-size byte buffers
 *
 * Design goals:
 *  - RAII ownership via ScopedResource handles.
 *  - No raw new/delete.
 *  - Configurable pool sizes with blocking or non-blocking acquire semantics.
 *  - Thread-safe for concurrent access.
 *
 * @version 1.9.0
 * @note Maturity: 🟢 PRODUCTION-READY
 */

#pragma once

#include <atomic>
#include <condition_variable>
#include <functional>
#include <future>
#include <memory>
#include <mutex>
#include <optional>
#include <queue>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

namespace themis {
namespace graph {

// ─────────────────────────────────────────────────────────────────────────────
// GraphConnectionPool
// ─────────────────────────────────────────────────────────────────────────────

template <typename T>
class GraphConnectionPool {
public:
    class ScopedResource {
    public:
        ScopedResource() = default;

        ScopedResource(std::shared_ptr<T> res, GraphConnectionPool<T>* pool)
            : res_(std::move(res)), pool_(pool) {}

        ScopedResource(ScopedResource&& o) noexcept
            : res_(std::move(o.res_)), pool_(o.pool_) {
            o.pool_ = nullptr;
        }

        ScopedResource& operator=(ScopedResource&& o) noexcept {
            if (this != &o) {
                release();
                res_  = std::move(o.res_);
                pool_ = o.pool_;
                o.pool_ = nullptr;
            }
            return *this;
        }

        ~ScopedResource() { release(); }

        /**
         * @brief Get.
         * @return Return value.
         * @details Implements get without additional internal calls.
         */
        T& get() { return *res_; }
        const T& get() const { return *res_; }

        explicit operator bool() const { return res_ != nullptr; }

    private:
        /**
         * @brief Release.
         * @details Calls: returnResource(), std::move().
         */
        void release() {
            if (res_ && pool_) {
                pool_->returnResource(std::move(res_));
                pool_ = nullptr;
            }
        }

        std::shared_ptr<T>      res_;
        GraphConnectionPool<T>* pool_ = nullptr;
    };

    explicit GraphConnectionPool(size_t pool_size,
                                  std::function<std::shared_ptr<T>()> factory)
        : pool_size_(pool_size), factory_(std::move(factory)) {
        if (pool_size_ == 0)
            throw std::invalid_argument("GraphConnectionPool: pool_size must be > 0");
        /**
         * @brief Pre-warm the pool.
         * @param[in] mutex_ Input parameter.
         * @return Return value.
         */
        std::lock_guard<std::mutex> lock(mutex_);
        for (size_t i = 0; i < pool_size_; ++i) {
            free_.push(factory_());
        }
    }

    /**
     * @brief Acquire.
     * @return Return value.
     * @details Calls: lock(), wait(), empty(), std::move(), front(), pop(), ScopedResource().
     */
    ScopedResource acquire() {
        std::unique_lock<std::mutex> lock(mutex_);
        cv_.wait(lock, [this] { return !free_.empty(); });
        auto res = std::move(free_.front());
        free_.pop();
        ++acquired_count_;
        return ScopedResource(std::move(res), this);
    }

    /**
     * @brief Try Acquire.
     * @return Return value.
     * @details Calls: lock(), empty(), std::move(), front(), pop(), ScopedResource().
     */
    std::optional<ScopedResource> tryAcquire() {
        std::lock_guard<std::mutex> lock(mutex_);
        if (free_.empty()) {
          return std::nullopt;
        }
        auto res = std::move(free_.front());
        free_.pop();
        ++acquired_count_;
        return ScopedResource(std::move(res), this);
    }

    size_t capacity() const { return pool_size_; }

    size_t available() const {
        /**
         * @brief Lock.
         * @param[in] mutex_ Input parameter.
         * @return Return value.
         */
        std::lock_guard<std::mutex> lock(mutex_);
        return free_.size();
    }

    uint64_t acquiredCount() const {
        return acquired_count_.load(std::memory_order_relaxed);
    }

private:
    /**
     * @brief Return Resource.
     * @param[in] res Input parameter.
     * @details Calls: lock(), push(), std::move(), notify_one().
     */
    void returnResource(std::shared_ptr<T> res) {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            free_.push(std::move(res));
        }
        cv_.notify_one();
    }

    size_t                              pool_size_;
    std::function<std::shared_ptr<T>()> factory_;
    mutable std::mutex                  mutex_;
    std::condition_variable             cv_;
    std::queue<std::shared_ptr<T>>      free_;
    std::atomic<uint64_t>               acquired_count_{0};
};

// ─────────────────────────────────────────────────────────────────────────────
// GraphThreadPool
// ─────────────────────────────────────────────────────────────────────────────

class GraphThreadPool {
public:
    /**
     * @brief Graph Thread Pool.
     * @param[in] num_threads Input parameter.
     * @return Return value.
     * @throws std::invalid_argument if an error occurs.
     * @details Calls: reserve(), emplace_back(), workerLoop().
     */
    explicit GraphThreadPool(size_t num_threads) {
        if (num_threads == 0)
            throw std::invalid_argument("GraphThreadPool: num_threads must be > 0");
        workers_.reserve(num_threads);
        for (size_t i = 0; i < num_threads; ++i) {
            workers_.emplace_back([this] { workerLoop(); });
        }
    }

    ~GraphThreadPool() { shutdown(); }

    // Non-copyable, non-movable (threads hold a raw this pointer).
    GraphThreadPool(const GraphThreadPool&)            = delete;
    GraphThreadPool& operator=(const GraphThreadPool&) = delete;

    template <typename F>
    auto submit(F&& f) -> std::future<decltype(f())> {
        using R = decltype(f());
        auto task    = std::make_shared<std::packaged_task<R()>>(std::forward<F>(f));
        auto future  = task->get_future();
        {
            /**
             * @brief Lock.
             * @param[in] mutex_ Input parameter.
             * @return Return value.
             */
            std::lock_guard<std::mutex> lock(mutex_);
            if (stopped_) {
              throw std::runtime_error("GraphThreadPool: pool is stopped");
            }
            tasks_.emplace([task] { (*task)(); });
            ++queued_count_;
        }
        cv_.notify_one();
        return future;
    }

    size_t threadCount() const { return workers_.size(); }

    uint64_t queuedCount() const {
        return queued_count_.load(std::memory_order_relaxed);
    }

    uint64_t completedCount() const {
        return completed_count_.load(std::memory_order_relaxed);
    }

    /**
     * @brief Shutdown.
     * @details Calls: lock(), notify_all(), joinable(), join().
     */
    void shutdown() {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            if (stopped_) {
              return;
            }
            stopped_ = true;
        }
        cv_.notify_all();
        for (auto& t : workers_) {
            if (t.joinable()) {
              t.join();
            }
        }
    }

    bool isStopped() const {
        /**
         * @brief Lock.
         * @param[in] mutex_ Input parameter.
         * @return Return value.
         */
        std::lock_guard<std::mutex> lock(mutex_);
        return stopped_;
    }

private:
    /**
     * @brief Worker Loop.
     * @details Calls: void(), lock(), wait(), empty(), std::move(), front(), pop(), task().
     */
    void workerLoop() {
        while (true) {
            std::function<void()> task;
            {
                std::unique_lock<std::mutex> lock(mutex_);
                cv_.wait(lock, [this] { return stopped_ || !tasks_.empty(); });
                if (stopped_ && tasks_.empty()) {
                  return;
                }
                task = std::move(tasks_.front());
                tasks_.pop();
            }
            task();
            ++completed_count_;
        }
    }

    mutable std::mutex              mutex_;
    std::condition_variable         cv_;
    std::vector<std::thread>        workers_;
    std::queue<std::function<void()>> tasks_;
    bool                            stopped_ = false;
    std::atomic<uint64_t>           queued_count_{0};
    std::atomic<uint64_t>           completed_count_{0};
};

// ─────────────────────────────────────────────────────────────────────────────
// GraphBufferPool
// ─────────────────────────────────────────────────────────────────────────────

class GraphBufferPool {
public:
    using Buffer = std::vector<uint8_t>;

    class ScopedBuffer {
    public:
        ScopedBuffer() = default;
        ScopedBuffer(Buffer buf, GraphBufferPool* pool)
            : buf_(std::move(buf)), pool_(pool) {}

        ScopedBuffer(ScopedBuffer&& o) noexcept
            : buf_(std::move(o.buf_)), pool_(o.pool_) {
            o.pool_ = nullptr;
        }

        ScopedBuffer& operator=(ScopedBuffer&& o) noexcept {
            if (this != &o) {
                release();
                buf_  = std::move(o.buf_);
                pool_ = o.pool_;
                o.pool_ = nullptr;
            }
            return *this;
        }

        ~ScopedBuffer() { release(); }

        /**
         * @brief Get.
         * @return Return value.
         * @details Implements get without additional internal calls.
         */
        Buffer& get() { return buf_; }

        size_t capacity() const { return buf_.capacity(); }

        explicit operator bool() const { return pool_ != nullptr; }

    private:
        /**
         * @brief Release.
         * @details Calls: returnBuffer(), std::move().
         */
        void release() {
            if (pool_) {
                pool_->returnBuffer(std::move(buf_));
                pool_ = nullptr;
            }
        }
        Buffer          buf_;
        GraphBufferPool* pool_ = nullptr;
    };

    GraphBufferPool(size_t num_buffers, size_t buffer_size)
        : buffer_size_(buffer_size) {
        if (num_buffers == 0 || buffer_size == 0)
            throw std::invalid_argument("GraphBufferPool: invalid dimensions");
        /**
         * @brief Lock.
         * @param[in] mutex_ Input parameter.
         * @return Return value.
         */
        std::lock_guard<std::mutex> lock(mutex_);
        for (size_t i = 0; i < num_buffers; ++i) {
            Buffer buf(buffer_size, 0);
            buf.reserve(buffer_size);
            free_.push(std::move(buf));
        }
    }

    /**
     * @brief Acquire.
     * @return Return value.
     * @details Calls: lock(), wait(), empty(), std::move(), front(), pop(), ScopedBuffer().
     */
    ScopedBuffer acquire() {
        std::unique_lock<std::mutex> lock(mutex_);
        cv_.wait(lock, [this] { return !free_.empty(); });
        auto buf = std::move(free_.front());
        free_.pop();
        ++acquired_count_;
        return ScopedBuffer(std::move(buf), this);
    }

    /**
     * @brief Try Acquire.
     * @return Return value.
     * @details Calls: lock(), empty(), std::move(), front(), pop(), ScopedBuffer().
     */
    std::optional<ScopedBuffer> tryAcquire() {
        std::lock_guard<std::mutex> lock(mutex_);
        if (free_.empty()) {
          return std::nullopt;
        }
        auto buf = std::move(free_.front());
        free_.pop();
        ++acquired_count_;
        return ScopedBuffer(std::move(buf), this);
    }

    size_t bufferSize() const { return buffer_size_; }

    size_t available() const {
        /**
         * @brief Lock.
         * @param[in] mutex_ Input parameter.
         * @return Return value.
         */
        std::lock_guard<std::mutex> lock(mutex_);
        return free_.size();
    }

    uint64_t acquiredCount() const {
        return acquired_count_.load(std::memory_order_relaxed);
    }

private:
    /**
     * @brief Return Buffer.
     * @param[in] buf Input parameter.
     * @details Calls: assign(), lock(), push(), std::move(), notify_one().
     */
    void returnBuffer(Buffer buf) {
        // Restore the buffer to its canonical pool size in case the caller
        // resized or moved-from it via ScopedBuffer::get().  assign() sets
        // exactly buffer_size_ elements to zero in one step, covering both
        // the size-mismatch and the zero-fill requirements.
        buf.assign(buffer_size_, uint8_t{0});
        {
            std::lock_guard<std::mutex> lock(mutex_);
            free_.push(std::move(buf));
        }
        cv_.notify_one();
    }

    size_t                      buffer_size_;
    mutable std::mutex          mutex_;
    std::condition_variable     cv_;
    std::queue<Buffer>          free_;
    std::atomic<uint64_t>       acquired_count_{0};
};

} // namespace graph
} // namespace themis
