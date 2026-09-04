/**
 * @file auth_worker_thread_pool.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.13
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <algorithm>
#include <condition_variable>
#include <functional>
#include <future>
#include <memory>
#include <mutex>
#include <queue>
#include <stdexcept>
#include <thread>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

namespace themis {
namespace auth {

/**
 * @brief Thread pool for dispatching blocking authentication tasks (LDAP, HTTP)
 *        off the calling thread, eliminating head-of-line blocking.
 *
 * Maintains between kMinThreads and kMaxThreads worker threads.  Tasks
 * submitted via submit() run on a worker thread; the caller receives a
 * std::future<T> immediately and is never blocked.
 *
 * New worker threads are spawned on demand (up to kMaxThreads) when all
 * current workers are busy.
 *
 * Thread-safety: submit() and shutdown() are safe to call concurrently.
 * Construction and destruction must not be concurrent with any other method.
 *
 * Performance targets (auth roadmap v1.2.0):
 *   - LDAP bind latency P99 ≤ 50 ms visible to callers even when backend
 *     latency is 200 ms (no head-of-line blocking)
 *   - JWT JWKS refresh never blocks the validation hot path for more than 1 ms
 */
class AuthWorkerThreadPool {
public:
    static constexpr size_t kMinThreads = 4;
    static constexpr size_t kMaxThreads = 32;

    /**
     * @brief Construct and start worker threads.
     *
     * @param min_threads  Threads to start immediately (clamped to [1, max_threads]).
     * @param max_threads  Upper bound on thread count (clamped to [1, kMaxThreads]).
     */
    explicit AuthWorkerThreadPool(size_t min_threads = kMinThreads,
                                  size_t max_threads = kMaxThreads)
        : max_threads_(std::min(std::max(max_threads, size_t{1}), kMaxThreads)),
          stop_(false),
          idle_count_(0)
    {
        const size_t start = std::min(std::max(min_threads, size_t{1}), max_threads_);
        workers_.reserve(max_threads_);
        for (size_t i = 0; i < start; ++i) {
            spawnWorker();
        }
    }

    // Non-copyable, non-movable
    AuthWorkerThreadPool(const AuthWorkerThreadPool&)             = delete;
    AuthWorkerThreadPool& operator=(const AuthWorkerThreadPool&)  = delete;
    AuthWorkerThreadPool(AuthWorkerThreadPool&&)                   = delete;
    AuthWorkerThreadPool& operator=(AuthWorkerThreadPool&&)        = delete;

    /**
     * @brief Signal stop, drain all pending tasks, and join every worker thread.
     */
    ~AuthWorkerThreadPool() noexcept {
        shutdown();
    }

    /**
     * @brief Submit a callable to the pool.
     *
     * Returns a std::future<ReturnType> that becomes ready once the callable
     * finishes.  If the callable throws, the exception is propagated through
     * the future.
     *
     * @tparam Func  Callable type
     * @tparam Args  Argument types forwarded to Func
     * @throws std::runtime_error if the pool has already been shut down
     */
    template <typename Func, typename... Args>
    auto submit(Func&& f, Args&&... args)
        -> std::future<std::invoke_result_t<Func, Args...>>
    {
        using ReturnType = std::invoke_result_t<Func, Args...>;

        auto task = std::make_shared<std::packaged_task<ReturnType()>>(
            [func = std::forward<Func>(f),
             bound_args = std::tuple<std::decay_t<Args>...>(
                 std::forward<Args>(args)...)]() mutable -> ReturnType {
                // Use if constexpr to correctly handle void return types:
                // 'return void_expr;' is valid C++17 but some toolchains warn.
                if constexpr (std::is_void_v<ReturnType>) {
                    std::apply(std::move(func), std::move(bound_args));
                } else {
                    return std::apply(std::move(func), std::move(bound_args));
                }
            });

        std::future<ReturnType> fut = task->get_future();

        {
            std::lock_guard<std::mutex> lock(queue_mutex_);
            if (stop_) {
                throw std::runtime_error(
                    "AuthWorkerThreadPool: cannot submit to a stopped pool");
            }
            tasks_.emplace([task]() { (*task)(); });
            tryGrow();
        }
        cv_.notify_one();
        return fut;
    }

    /**
     * @brief Gracefully stop the pool.
     *
     * Sets the stop flag, wakes all workers, and joins every thread.  Pending
     * tasks that have not yet started will still be executed before each worker
     * exits.  Safe to call multiple times.
     */
    void shutdown() noexcept {
        {
            std::lock_guard<std::mutex> lock(queue_mutex_);
            if (stop_) {
              return;
            }
            stop_ = true;
        }
        cv_.notify_all();
        for (auto& t : workers_) {
            if (t.joinable()) {
              t.join();
            }
        }
        workers_.clear();
    }

    /// Return the current number of live worker threads.
    size_t threadCount() const noexcept {
        std::lock_guard<std::mutex> lock(queue_mutex_);
        return workers_.size();
    }

private:
    size_t                          max_threads_;
    bool                            stop_;
    size_t                          idle_count_;  ///< Workers currently waiting
    std::vector<std::thread>        workers_;
    std::queue<std::function<void()>> tasks_;
    mutable std::mutex              queue_mutex_;
    std::condition_variable         cv_;

    // Spawn one new worker thread. Must be called with queue_mutex_ held.
    void spawnWorker() {
        workers_.emplace_back([this] {
            for (;;) {
                std::function<void()> task;
                {
                    std::unique_lock<std::mutex> lock(queue_mutex_);
                    ++idle_count_;
                    cv_.wait(lock, [this] { return stop_ || !tasks_.empty(); });
                    --idle_count_;
                    if (stop_ && tasks_.empty()) {
                      return;
                    }
                    task = std::move(tasks_.front());
                    tasks_.pop();
                }
                task();
            }
        });
    }

    // Grow the pool by one thread if every current worker is busy and we
    // haven't hit max_threads_.  Must be called with queue_mutex_ held.
    void tryGrow() {
        if (idle_count_ == 0 && workers_.size() < max_threads_) {
            spawnWorker();
        }
    }
};

} // namespace auth
} // namespace themis
