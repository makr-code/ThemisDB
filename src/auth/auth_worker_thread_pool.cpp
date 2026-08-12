/**
 * @file auth_worker_thread_pool.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.13
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=1, H=0, M=1, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 *
 * @note **Stub Implementation**: Provides a basic async thread pool for auth operations
 *       (LDAP, HTTP, certificate validation). Performs task queuing and worker dispatch.
 *       See auth_worker_thread_pool.h for performance targets and concurrency guarantees.
 */

#include "auth/auth_worker_thread_pool.h"
#include <spdlog/spdlog.h>
#include <chrono>

namespace themis {
namespace auth {

// Helper: wrap a task and execution result
namespace detail {
/** @brief Task holder. */
class TaskHolder {
public:
    virtual ~TaskHolder() = default;
    virtual void execute() = 0;
};

template <typename F, typename R>
/** @brief Concrete task. */
class ConcreteTask : public TaskHolder {
private:
    F func_;
    std::promise<R> promise_;

public:
    explicit ConcreteTask(F f) : func_(std::move(f)) {}

    void execute() override {
        try {
            if constexpr (std::is_void_v<R>) {
                func_();
                promise_.set_value();
            } else {
                promise_.set_value(func_());
            }
        } catch (...) {
            promise_.set_exception(std::current_exception());
        }
    }

    std::future<R> get_future() {
        return promise_.get_future();
    }
};
}  // namespace detail

AuthWorkerThreadPool::AuthWorkerThreadPool(size_t min_threads, size_t max_threads)
    : kMinThreads(std::max(size_t(1), min_threads)),
      kMaxThreads(std::max(kMinThreads, max_threads)),
      active_threads_(0),
      shutdown_(false) {
    // Initialize minimum worker threads
    for (size_t i = 0; i < kMinThreads; ++i) {
        spawnWorkerThread();
    }
}

AuthWorkerThreadPool::~AuthWorkerThreadPool() {
    shutdown();
}

void AuthWorkerThreadPool::shutdown() {
    {
        std::unique_lock lock(queue_mutex_);
        if (shutdown_) return;
        shutdown_ = true;
    }
    queue_cv_.notify_all();

    // Wait for all worker threads to finish
    std::unique_lock lock(queue_mutex_);
    queue_cv_.wait(lock, [this] { return active_threads_ == 0; });
}

void AuthWorkerThreadPool::spawnWorkerThread() {
    ++active_threads_;
    std::thread worker([this] { workerLoop(); });
    worker.detach();  // Managed by active_threads_ counter
}

void AuthWorkerThreadPool::workerLoop() {
    while (true) {
        std::unique_lock lock(queue_mutex_);
        queue_cv_.wait(lock, [this] { return !task_queue_.empty() || shutdown_; });

        if (shutdown_ && task_queue_.empty()) {
            --active_threads_;
            queue_cv_.notify_all();
            return;
        }

        if (!task_queue_.empty()) {
            auto task = std::move(task_queue_.front());
            task_queue_.pop();
            lock.unlock();

            try {
                task();
            } catch (const std::exception& e) {
                spdlog::error("Auth worker thread pool task raised exception: {}", e.what());
            }
        }
    }
}

}  // namespace auth
}  // namespace themis
