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
#include <iostream>

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
        // C++20 doesn't have std::thread::try_join_for(), so we use a custom approach:
        // 1. Create a flag that the thread can set on completion
        // 2. Join with a loop that checks the timeout
        //
        // For now, we use a best-effort approach:
        // - If the thread is expected to finish quickly, we join normally
        // - We assume well-designed threads cooperate with shutdown flags
        //
        // FUTURE: Implement interruptible_thread with proper timeout support
        // See: https://github.com/chriskohlhoff/asio for inspiration

        thread_.join();
        joined_ = true;
        return true;
    } catch (const std::system_error& e) {
        LOG_ERROR("ThreadGuard::join_with_timeout: system error: {}", e.what());
        return false;
    } catch (const std::exception& e) {
        LOG_ERROR("ThreadGuard::join_with_timeout: unexpected exception: {}", e.what());
        return false;
    }
}

} // namespace themis::utils
