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

class ThreadGuard {
private:
    std::thread thread_;

public:
    template<typename Func>
    /**
     * @brief Thread Guard.
     * @param[in] func Input parameter.
     * @return Return value.
     */
    explicit ThreadGuard(Func&& func)
        : thread_(std::forward<Func>(func)) {}

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

    bool joinable() const noexcept {
        return thread_.joinable();
    }

    std::thread::id get_id() const noexcept {
        return thread_.get_id();
    }

    /**
     * @brief Join.
     * @details Calls: joinable().
     */
    void join() {
        if (thread_.joinable()) {
            thread_.join();
        }
    }

    ~ThreadGuard() {
        join();
    }

    std::thread& native_handle() noexcept {
        return thread_;
    }
};

} // namespace analytics
} // namespace themisdb
