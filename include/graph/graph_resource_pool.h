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

/**
 * @brief Generic bounded resource pool modelling a connection pool.
 *
 * Resources are created by a user-supplied factory function.  A bounded
 * number of resources are kept alive and recycled.  Callers acquire a
 * ScopedResource RAII handle; the resource is automatically returned on
 * handle destruction.
 *
 * @tparam T Resource type.
 */
template <typename T>
class GraphConnectionPool {
public:
    /// RAII handle returned by acquire().
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

        /// Access the underlying resource (must be valid).
        T& get() { return *res_; }
        const T& get() const { return *res_; }

        /// Return true if this handle holds a valid resource.
        explicit operator bool() const { return res_ != nullptr; }

    private:
        void release() {
            if (res_ && pool_) {
                pool_->returnResource(std::move(res_));
                pool_ = nullptr;
            }
        }

        std::shared_ptr<T>      res_;
        GraphConnectionPool<T>* pool_ = nullptr;
    };

    /**
     * @brief Construct a connection pool.
     *
     * @param pool_size Maximum concurrent resources.
     * @param factory   Factory that creates a new resource instance.
     */
    explicit GraphConnectionPool(size_t pool_size,
                                  std::function<std::shared_ptr<T>()> factory)
        : pool_size_(pool_size), factory_(std::move(factory)) {
        if (pool_size_ == 0)
            throw std::invalid_argument("GraphConnectionPool: pool_size must be > 0");
        // Pre-warm the pool.
        std::lock_guard<std::mutex> lock(mutex_);
        for (size_t i = 0; i < pool_size_; ++i) {
            free_.push(factory_());
        }
    }

    /**
     * @brief Acquire a resource, blocking until one becomes available.
     *
     * @return RAII handle owning the acquired resource.
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
     * @brief Try to acquire a resource without blocking.
     *
     * @return RAII handle on success, or an empty handle if the pool is
     *         exhausted.
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

    /**
     * @brief Return the pool capacity (maximum concurrent resources).
     * @return Pool capacity.
     */
    size_t capacity() const { return pool_size_; }

    /**
     * @brief Return the number of resources currently available.
     * @return Available count.
     */
    size_t available() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return free_.size();
    }

    /**
     * @brief Return the total number of successful acquisitions.
     * @return Acquisition count.
     */
    uint64_t acquiredCount() const {
        return acquired_count_.load(std::memory_order_relaxed);
    }

private:
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

/**
 * @brief Fixed-size thread pool for graph traversal tasks.
 *
 * Worker threads consume tasks from a shared queue.  The pool is shut down
 * gracefully on destruction (all queued tasks complete before threads exit).
 */
class GraphThreadPool {
public:
    /**
     * @brief Construct a thread pool with the given number of worker threads.
     *
     * @param num_threads Number of worker threads (must be > 0).
     */
    explicit GraphThreadPool(size_t num_threads) {
        if (num_threads == 0)
            throw std::invalid_argument("GraphThreadPool: num_threads must be > 0");
        workers_.reserve(num_threads);
        for (size_t i = 0; i < num_threads; ++i) {
            workers_.emplace_back([this] { workerLoop(); });
        }
    }

    /// Shut down: drain the queue, join all threads.
    ~GraphThreadPool() { shutdown(); }

    // Non-copyable, non-movable (threads hold a raw this pointer).
    GraphThreadPool(const GraphThreadPool&)            = delete;
    GraphThreadPool& operator=(const GraphThreadPool&) = delete;

    /**
     * @brief Submit a callable for asynchronous execution.
     *
     * @tparam F Callable type returning R.
     * @param  f Task to execute.
     * @return std::future<R> that will hold the result.
     */
    template <typename F>
    auto submit(F&& f) -> std::future<decltype(f())> {
        using R = decltype(f());
        auto task    = std::make_shared<std::packaged_task<R()>>(std::forward<F>(f));
        auto future  = task->get_future();
        {
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

    /**
     * @brief Return the number of worker threads.
     * @return Thread count.
     */
    size_t threadCount() const { return workers_.size(); }

    /**
     * @brief Return the total number of tasks submitted.
     * @return Submitted task count.
     */
    uint64_t queuedCount() const {
        return queued_count_.load(std::memory_order_relaxed);
    }

    /**
     * @brief Return the total number of tasks completed.
     * @return Completed task count.
     */
    uint64_t completedCount() const {
        return completed_count_.load(std::memory_order_relaxed);
    }

    /**
     * @brief Gracefully shut down the pool.
     *
     * Signals all workers to stop after their current task and waits for
     * them to join.  Safe to call multiple times.
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

    /**
     * @brief Return true if the pool has been shut down.
     * @return Stopped state.
     */
    bool isStopped() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return stopped_;
    }

private:
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

/**
 * @brief Fixed-size byte buffer pool for graph traversal scratch space.
 *
 * Maintains a pool of pre-allocated @p buffer_size byte vectors.  Callers
 * acquire a buffer via @ref acquire(), use it, and return it via the RAII
 * @ref ScopedBuffer handle.  Reduces allocation pressure during high-frequency
 * traversal operations.
 */
class GraphBufferPool {
public:
    using Buffer = std::vector<uint8_t>;

    /// RAII buffer handle.
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
         * @brief Access the underlying buffer.
         * @return Reference to the byte vector.
         */
        Buffer& get() { return buf_; }

        /**
         * @brief Return buffer capacity in bytes.
         * @return Buffer size.
         */
        size_t capacity() const { return buf_.capacity(); }

        /// Return true if this handle holds a valid buffer.
        explicit operator bool() const { return pool_ != nullptr; }

    private:
        void release() {
            if (pool_) {
                pool_->returnBuffer(std::move(buf_));
                pool_ = nullptr;
            }
        }
        Buffer          buf_;
        GraphBufferPool* pool_ = nullptr;
    };

    /**
     * @brief Construct a buffer pool.
     *
     * @param num_buffers  Number of pre-allocated buffers.
     * @param buffer_size  Size in bytes of each buffer.
     */
    GraphBufferPool(size_t num_buffers, size_t buffer_size)
        : buffer_size_(buffer_size) {
        if (num_buffers == 0 || buffer_size == 0)
            throw std::invalid_argument("GraphBufferPool: invalid dimensions");
        std::lock_guard<std::mutex> lock(mutex_);
        for (size_t i = 0; i < num_buffers; ++i) {
            Buffer buf(buffer_size, 0);
            buf.reserve(buffer_size);
            free_.push(std::move(buf));
        }
    }

    /**
     * @brief Acquire a buffer, blocking until one is available.
     * @return RAII buffer handle.
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
     * @brief Try to acquire a buffer without blocking.
     * @return RAII buffer handle, or empty if pool is exhausted.
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

    /**
     * @brief Return the configured buffer size in bytes.
     * @return Buffer size.
     */
    size_t bufferSize() const { return buffer_size_; }

    /**
     * @brief Return the number of currently available (free) buffers.
     * @return Available count.
     */
    size_t available() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return free_.size();
    }

    /**
     * @brief Return the total number of successful buffer acquisitions.
     * @return Acquisition count.
     */
    uint64_t acquiredCount() const {
        return acquired_count_.load(std::memory_order_relaxed);
    }

private:
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
