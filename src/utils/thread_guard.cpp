/**
 * @file thread_guard.cpp
 * @brief Implementation of exception-safe RAII thread management
 *
 * Provides ThreadGuard - an RAII wrapper for std::thread with timeout support.
 * Ensures proper thread cleanup even in exceptional circumstances.
 *
 * @version 0.1.0
 * @since 2026-08-07
 */

#include "utils/thread_guard.h"

#include <cassert>
#include <condition_variable>
#include <iostream>
#include <mutex>
#include <stdexcept>
#include <system_error>

// Conditional logging based on spdlog availability
#ifdef THEMIS_HAS_SPDLOG
#include <spdlog/spdlog.h>
#define LOG_WARN(msg, ...) spdlog::warn(msg, ##__VA_ARGS__)
#define LOG_ERROR(msg, ...) spdlog::error(msg, ##__VA_ARGS__)
#else
#define LOG_WARN(msg, ...) std::cerr << "[WARN] " << msg << std::endl
#define LOG_ERROR(msg, ...) std::cerr << "[ERROR] " << msg << std::endl
#endif

namespace themis::utils {

ThreadGuard::ThreadGuard(
    std::thread thread,
    std::chrono::milliseconds timeout
)
    : thread_(std::move(thread))
    , timeout_(timeout)
    , joined_(false)
{
    if (!thread_.joinable()) {
        throw std::invalid_argument("ThreadGuard: cannot guard a non-joinable thread");
    }
}

ThreadGuard::~ThreadGuard() noexcept {
    try {
        if (thread_.joinable()) {
            // Attempt join with timeout
            if (!join_with_timeout()) {
                // Log timeout warning but don't throw
                LOG_WARN(
                    "ThreadGuard destructor: thread join timed out after {}ms; "
                    "thread left in indeterminate state (design threads for cooperative shutdown)",
                    timeout_.count()
                );
            }
        }
    } catch (const std::exception& e) {
        // Destructor must not throw - log and suppress
        LOG_ERROR("ThreadGuard destructor: unexpected exception during join: {}", e.what());
    } catch (...) {
        // Catch-all for unknown exceptions - should never happen but defensive coding
        LOG_ERROR("ThreadGuard destructor: unknown exception during join");
    }
}

bool ThreadGuard::join_with_timeout() noexcept {
    if (joined_) {
        return true; // Already joined successfully
    }

    if (!thread_.joinable()) {
        return true; // Nothing to join
    }

    try {
        // Implement timeout-aware join using a helper thread that monitors completion.
        // The thread's native handle is not portable, so we use a condition variable
        // and a detached monitor thread to implement the timeout semantics.
        // Shared state is heap-allocated to avoid dangling references on timeout.
        struct State {
            std::atomic<bool> finished{false};
            std::mutex cv_mutex;
            std::condition_variable cv;
        };
        auto state = std::make_shared<State>();

        // Move the thread into a monitor wrapper so we can join it and signal completion.
        std::thread monitor(
            [worker = std::move(thread_), state]() mutable {
                worker.join();
                {
                    std::lock_guard<std::mutex> lk(state->cv_mutex);
                    state->finished = true;
                }
                state->cv.notify_one();
            }
        );
        monitor.detach();

        // Wait for up to timeout_ for the worker to finish.
        std::unique_lock<std::mutex> lk(state->cv_mutex);
        bool completed = state->cv.wait_for(lk, timeout_, [&state] { return state->finished.load(); });

        if (completed) {
            joined_ = true;
            return true;
        }

        // Timeout: the worker thread is still running; it was detached above and will
        // continue in the background.  The destructor warning is appropriate here.
        return false;
    } catch (const std::system_error& e) {
        LOG_ERROR("ThreadGuard::join_with_timeout: system error: {}", e.what());
        return false;
    } catch (const std::exception& e) {
        LOG_ERROR("ThreadGuard::join_with_timeout: unexpected exception: {}", e.what());
        return false;
    }
}

} // namespace themis::utils
