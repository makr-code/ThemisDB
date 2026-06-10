/**
 * @file thread_join_utils.h
 * @brief Helpers for bounded shutdown joins on cooperative worker threads.
 *
 * These helpers are intended for shutdown paths that already requested worker
 * cancellation (for example by toggling an atomic stop flag and notifying a
 * condition variable). They bound the caller's wait so teardown cannot block
 * forever if a worker becomes stuck while still allowing the underlying thread
 * to be joined asynchronously by a detached watcher.
 */

#pragma once

#include <chrono>
#include <future>
#include <thread>
#include <utility>

namespace themis::utils {

/// Default shutdown deadline used by bounded thread joins.
inline constexpr auto kDefaultThreadJoinTimeout = std::chrono::seconds(5);

/**
 * @brief Wait for a cooperative worker thread to finish within a deadline.
 *
 * @param thread Thread to join.
 * @param timeout Maximum time the caller waits for the join to complete.
 * @return true when the thread joined before the deadline, false when the
 *         join is still pending in the detached watcher thread.
 *
 * @note Call this only after the owner has already requested thread shutdown.
 *       On timeout the caller returns promptly while a detached watcher keeps
 *       waiting for the underlying join to finish.
 */
[[nodiscard]] inline bool joinThreadWithin(
    std::thread& thread,
    std::chrono::milliseconds timeout =
        std::chrono::duration_cast<std::chrono::milliseconds>(
            kDefaultThreadJoinTimeout)) {
    if (!thread.joinable()) {
        return true;
    }

    std::promise<void> done;
    auto future = done.get_future();
    std::thread watcher([inner = std::move(thread), promise = std::move(done)]() mutable {
        if (inner.joinable()) {
            inner.join();
        }
        promise.set_value();
    });
    watcher.detach();

    return future.wait_for(timeout) == std::future_status::ready;
}

}  // namespace themis::utils
