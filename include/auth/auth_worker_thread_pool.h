/**
 * @file auth_worker_thread_pool.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.13
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
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

class AuthWorkerThreadPool {
public:
    static constexpr size_t kMinThreads = 4;
    static constexpr size_t kMaxThreads = 32;

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

    ~AuthWorkerThreadPool() noexcept {
        shutdown();
    }

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
            /**
             * @brief Lock.
             * @param[in] queue_mutex_ Input parameter.
             * @return Return value.
             */
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

    void shutdown() noexcept {
        {
            /**
             * @brief Lock.
             * @param[in] queue_mutex_ Input parameter.
             * @return Return value.
             */
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

    size_t threadCount() const noexcept {
        /**
         * @brief Lock.
         * @param[in] queue_mutex_ Input parameter.
         * @return Return value.
         */
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
    std::condition_variable         cv_ = {};

    /**
     * @brief Spawn one new worker thread.
     * @details Must be called with queue_mutex_ held. Calls: emplace_back(), void(), lock(), wait(), empty(), std::move(), front(), pop().
     */
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

    /**
     * @brief Grow the pool by one thread if every current worker is busy and we haven't hit max_threads_.
     * @details Must be called with queue_mutex_ held. Calls: size(), spawnWorker().
     */
    void tryGrow() {
        if (idle_count_ == 0 && workers_.size() < max_threads_) {
            spawnWorker();
        }
    }
};

} // namespace auth
} // namespace themis
