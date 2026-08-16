/**
 * @file connection_guard.h
 * @brief RAII wrapper for database connection lifecycle management in Analytics module
 *
 * Ensures database connections are properly released on scope exit, preventing:
 * - Connection pool exhaustion
 * - Resource leaks on early returns
 * - Connection leaks in exception paths
 *
 * Phase 3 Batch A-2: Close 20 HIGH-severity db_connection_leak gaps
 *
 * Copyright (c) 2025 VCC-URN Project
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <functional>
#include <memory>
#include <stdexcept>

namespace themisdb {
namespace analytics {

/**
 * RAII wrapper for database connection lifecycle management.
 *
 * Ensures database connections are returned to the pool on destruction,
 * preventing resource exhaustion in both normal and exception paths.
 *
 * Usage:
 * @code
 *   {
 *       const int connection_id = acquireConnectionId();
 *       auto guard = ConnectionGuard::acquire(
 *           connection_id,
 *           [&] { releaseConnection(connection_id); });
 *       executeQuery();
 *       // Connection automatically released when guard goes out of scope
 *   }
 * @endcode
 *
 * SAFETY: This guard is exception-safe. Even if an exception is thrown
 * after acquiring the connection, the destructor will release it.
 */
class ConnectionGuard {
public:
    /// Callback type for connection release operations
    using Releaser = std::function<void()>;

    /**
     * Acquire a connection from the database manager.
     *
     * @param connection_id Unique identifier for this connection
     * @param releaser Callback to invoke when connection should be released
     *
     * @return ConnectionGuard that manages the connection lifetime
     *
     * SAFETY: The releaser callback MUST NOT throw exceptions.
     *         If it does, the exception will be caught and suppressed.
     */
    static auto acquire(int connection_id, Releaser releaser) -> ConnectionGuard {
        return ConnectionGuard(connection_id, std::move(releaser));
    }

    /**
     * Construct a connection guard.
     *
     * @param connection_id Unique identifier for this connection
     * @param releaser Callback to invoke on destruction
     */
    explicit ConnectionGuard(int connection_id, Releaser releaser) noexcept
        : connection_id_(connection_id),
          releaser_(std::move(releaser)),
          is_released_(false) {}

    // ========================================================================
    // Move semantics (transfer ownership)
    // ========================================================================

    /**
     * Move constructor: transfer connection ownership
     */
    ConnectionGuard(ConnectionGuard&& other) noexcept
        : connection_id_(other.connection_id_),
          releaser_(std::move(other.releaser_)),
          is_released_(other.is_released_) {
        other.connection_id_ = -1;
        other.releaser_ = nullptr;
        other.is_released_ = true;
    }

    /**
     * Move assignment: transfer connection ownership
     */
    ConnectionGuard& operator=(ConnectionGuard&& other) noexcept {
        if (this != &other) {
            release();  // Clean up current connection
            connection_id_ = other.connection_id_;
            releaser_ = std::move(other.releaser_);
            is_released_ = other.is_released_;
            
            other.connection_id_ = -1;
            other.releaser_ = nullptr;
            other.is_released_ = true;
        }
        return *this;
    }

    // ========================================================================
    // Deleted copy semantics (prevent accidental duplication)
    // ========================================================================

    ConnectionGuard(const ConnectionGuard&) = delete;
    ConnectionGuard& operator=(const ConnectionGuard&) = delete;

    // ========================================================================
    // Accessor and release methods
    // ========================================================================

    /**
     * Get the connection identifier
     *
     * @return Connection ID (or -1 if already released)
     */
    int getId() const noexcept {
        return connection_id_;
    }

    /**
     * Check if connection is still held
     *
     * @return true if connection is held, false if released
     */
    bool isHeld() const noexcept {
        return !is_released_;
    }

    /**
     * Manually release the connection.
     *
     * Called automatically in destructor, but can be called explicitly
     * if needed. Safe to call multiple times (subsequent calls are no-ops).
     *
     * SAFETY: This method catches and suppresses all exceptions.
     */
    void release() noexcept {
        if (!is_released_ && releaser_) {
            try {
                releaser_();
            } catch (...) {
                // CRITICAL: We MUST NOT throw from here because this may be
                // called from the destructor. Swallow the exception and log
                // if possible (but we can't depend on logging in destructor).
            }
            is_released_ = true;
            releaser_ = nullptr;
        }
    }

    /**
     * Destructor: ensures connection is released.
     *
     * RAII: Cleanup guaranteed on scope exit, even if exception thrown.
     *
     * THREAD-SAFETY: If the connection needs to be released by a specific
     * thread (e.g., the I/O thread that acquired it), ensure the guard
     * is destroyed on that thread.
     */
    ~ConnectionGuard() noexcept {
        release();
    }

private:
    int connection_id_;      ///< Unique identifier for this connection
    Releaser releaser_;      ///< Callback to release connection
    bool is_released_;       ///< Has the connection been released?
};

}  // namespace analytics
}  // namespace themisdb
