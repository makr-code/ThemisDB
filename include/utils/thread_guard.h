/**
 * @file thread_guard.h
 * @brief RAII thread management with timeout support
 *
 * Provides exception-safe thread management ensuring proper cleanup on destruction.
 * Implements timeout-aware thread joining to prevent infinite hangs.
 *
 * @version 0.1.0
 * @since 2026-08-07
 */

#pragma once

#include <chrono>
#include <memory>
#include <mutex>
#include <thread>
#include <condition_variable>
#include <spdlog/spdlog.h>

namespace themis::utils {

/**
 * @brief Exception-safe RAII wrapper for std::thread with timeout support.
 *
 * Ensures:
 * - Thread is properly joined or detached on destruction
 * - Timeout prevents indefinite hangs in destructors
 * - Exception-safe: no exceptions thrown from destructor
 * - Strong exception guarantee during construction
 *
 * Usage:
 * ```cpp
 * std::thread worker = std::thread([]() { /* work */ });
 * ThreadGuard guard(std::move(worker), std::chrono::seconds(30));
 * // Thread automatically joins with 30-second timeout on guard destruction
 * ```
 *
 * @tparam Duration Timeout duration type (default: std::chrono::seconds)
 *
 * @note This class is move-only; copying is explicitly deleted to prevent
 *       ambiguous ownership semantics.
 *
 * @note If join times out, the thread is left in an indeterminate state.
 *       The thread is NOT forcefully terminated (C++ has no portable way to do this).
 *       Prefer to design threads with cooperative shutdown (atomic flags + notify).
 */
class ThreadGuard {
public:
    /**
     * @brief Construct a ThreadGuard from a joinable thread and timeout.
     *
     * @param thread Rvalue reference to std::thread (will be moved)
     * @param timeout Maximum duration to wait for thread completion
     *
     * @exception std::invalid_argument if thread is not joinable
     * @exception std::system_error if internal synchronization fails
     *
     * @note Exception guarantee: STRONG - if an exception is thrown, the guard
     *       is not constructed and the thread reference is returned to the caller.
     */
    explicit ThreadGuard(
        std::thread thread,
        std::chrono::milliseconds timeout = std::chrono::seconds(30)
    );

    /**
     * @brief Destructor: joins thread with timeout, logs any issues
     *
     * @note noexcept - guaranteed not to throw exceptions
     * - If join succeeds: silent cleanup
     * - If join times out: logs WARNING, leaves thread in indeterminate state
     * - If thread was already joined/detached: silent (idempotent)
     *
     * @warning Never throws - exceptions are logged but suppressed
     */
    ~ThreadGuard() noexcept;

    /**
     * @brief Explicitly join the thread with timeout.
     *
     * @return true if thread joined successfully, false if timeout occurred
     *
     * @exception std::system_error if join fails for system reasons
     *
     * @note If thread is not joinable, returns false (idempotent).
     * @note Subsequent calls return immediately with false (thread already joined).
     */
    bool join_with_timeout() noexcept;

    /**
     * @brief Check if thread is currently joinable.
     *
     * @return true if std::thread::joinable() returns true
     */
    [[nodiscard]] bool is_joinable() const noexcept {
        return thread_.joinable();
    }

    /**
     * @brief Get reference to underlying thread (for advanced use cases).
     *
     * @return Reference to the std::thread
     *
     * @warning Direct access to the thread bypasses timeout protection.
     *          Prefer not to use this unless necessary.
     */
    std::thread& get_thread() noexcept {
        return thread_;
    }

    // Prevent copying
    ThreadGuard(const ThreadGuard&) = delete;
    ThreadGuard& operator=(const ThreadGuard&) = delete;

    // Allow move semantics
    ThreadGuard(ThreadGuard&&) noexcept = default;
    ThreadGuard& operator=(ThreadGuard&&) noexcept = default;

private:
    std::thread thread_;
    std::chrono::milliseconds timeout_;
    bool joined_ = false;
};

/**
 * @brief Factory function to create a ThreadGuard from a callable.
 *
 * Convenient wrapper to create a thread and guard it in one expression.
 *
 * Usage:
 * ```cpp
 * auto guard = make_thread_guard(
 *     []() { /* background work */ },
 *     std::chrono::seconds(60)
 * );
 * ```
 *
 * @tparam Func Callable type
 * @param func Callable to execute in thread
 * @param timeout Join timeout (default: 30 seconds)
 * @return ThreadGuard managing the created thread
 */
template<typename Func>
ThreadGuard make_thread_guard(
    Func&& func,
    std::chrono::milliseconds timeout = std::chrono::seconds(30)
) {
    return ThreadGuard(
        std::thread(std::forward<Func>(func)),
        timeout
    );
}

} // namespace themis::utils
