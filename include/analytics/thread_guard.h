/**
 * @file thread_guard.h
 * @brief RAII wrapper for thread lifecycle management
 * 
 * Ensures threads are properly joined on scope exit, preventing resource leaks
 * and ensuring predictable cleanup in both normal and exception paths.
 * 
 * Copyright (c) 2025 VCC-URN Project
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <thread>
#include <utility>
#include <cassert>

namespace themisdb {
namespace analytics {

/**
 * RAII wrapper for std::thread lifecycle management.
 * 
 * Ensures the thread is joined on destruction, preventing:
 * - Thread resource leaks
 * - Detached thread issues
 * - Undefined behavior from outliving thread objects
 * 
 * Usage:
 * @code
 *   {
 *       ThreadGuard guard([](){ std::cout << "Running in thread\n"; });
 *       // Thread is running...
 *   }  // Thread is automatically joined here
 * @endcode
 */
class ThreadGuard {
private:
    std::thread thread_;

public:
    /**
     * Construct with a callable that will run in the thread.
     * @tparam Func Callable type
     * @param func The function/lambda to run in the thread
     */
    template<typename Func>
    explicit ThreadGuard(Func&& func)
        : thread_(std::forward<Func>(func)) {}

    /**
     * Construct with a member function and object.
     * @tparam T The class type
     * @tparam Func The member function type
     * @param func Pointer to member function
     * @param obj Pointer to object
     */
    template<typename T, typename Func>
    ThreadGuard(Func T::* func, T* obj)
        : thread_(func, obj) {}

    // Deleted copy (prevent unintended thread duplication)
    ThreadGuard(const ThreadGuard&) = delete;
    ThreadGuard& operator=(const ThreadGuard&) = delete;

    // Movable (transfer ownership)
    ThreadGuard(ThreadGuard&& other) noexcept
        : thread_(std::move(other.thread_)) {
        // other.thread_ is now empty
    }

    ThreadGuard& operator=(ThreadGuard&& other) noexcept {
        if (thread_.joinable()) {
            thread_.join();
        }
        thread_ = std::move(other.thread_);
        return *this;
    }

    /**
     * Check if the thread is joinable.
     */
    bool joinable() const noexcept {
        return thread_.joinable();
    }

    /**
     * Get the thread ID.
     */
    std::thread::id get_id() const noexcept {
        return thread_.get_id();
    }

    /**
     * Wait for the thread to finish.
     * Can be called multiple times safely (second call is no-op).
     */
    void join() {
        if (thread_.joinable()) {
            thread_.join();
        }
    }

    /**
     * Destructor: ensures thread is joined.
     * RAII: Cleanup guaranteed on scope exit, even if exception thrown.
     */
    ~ThreadGuard() {
        join();
    }

    /**
     * Get access to underlying thread (for advanced use cases).
     * WARNING: Do not call .detach() on the returned thread!
     */
    std::thread& native_handle() noexcept {
        return thread_;
    }
};

} // namespace analytics
} // namespace themisdb
