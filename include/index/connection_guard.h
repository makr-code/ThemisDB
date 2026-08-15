/**
 * @file connection_guard.h
 * @brief RAII wrapper for index module database connection lifecycle management
 * 
 * Phase 3 Batch A-6: Database Connection Leak Prevention
 * 
 * Ensures database connections are released in all code paths:
 * - Normal completion ✓
 * - Early returns ✓
 * - Exception handling ✓
 * 
 * @version 2026-08-15
 * @status Phase 3 A-6 Connection Leak Prevention
 */

#ifndef THEMIS_INDEX_CONNECTION_GUARD_H
#define THEMIS_INDEX_CONNECTION_GUARD_H

#include <memory>
#include <functional>
#include <atomic>
#include <stdexcept>

namespace themis {
namespace index {

/**
 * @brief RAII wrapper for connection lifecycle management
 * 
 * Guarantees cleanup on all exit paths:
 * - Scope exit (normal)
 * - Exception throws (unwinding)
 * - Early returns (short-circuit)
 * 
 * Thread-safe design:
 * - Move semantics (exclusive ownership transfer)
 * - No copying (prevents accidental double-cleanup)
 * - Atomic release flag (prevents double-release)
 * 
 * Example usage:
 * ```cpp
 * auto guard = ConnectionGuard::acquire(db_);
 * if (!guard) return false;  // Early return: guard cleanup triggered
 * try {
 *     auto result = performDatabaseOperation();
 * } catch (...) {
 *     // Exception: guard cleanup triggered
 * }
 * // Scope exit: guard cleanup triggered
 * ```
 * 
 * Phase 3 A-6: Gap A-6.1 through A-6.34 (34 connection leak sites)
 */
class ConnectionGuard {
public:
    using Releaser = std::function<void()>;
    
    /**
     * Construct with release function
     * 
     * @param releaser Function to call on destruction
     *        MUST be exception-safe (catches internal exceptions)
     * @param connection_id Optional identifier for logging/debugging
     */
    explicit ConnectionGuard(Releaser releaser, int connection_id = -1) noexcept
        : releaser_(std::move(releaser)),
          connection_id_(connection_id),
          released_(false) {}
    
    /**
     * Default constructor: empty guard (no release needed)
     */
    ConnectionGuard() noexcept
        : connection_id_(-1),
          released_(false) {}
    
    /**
     * Delete copy constructor: exclusive ownership
     */
    ConnectionGuard(const ConnectionGuard&) = delete;
    
    /**
     * Delete copy assignment: exclusive ownership
     */
    ConnectionGuard& operator=(const ConnectionGuard&) = delete;
    
    /**
     * Move constructor: transfer ownership
     * 
     * Moved-from object becomes inactive (won't release on destruction)
     */
    ConnectionGuard(ConnectionGuard&& other) noexcept
        : releaser_(std::move(other.releaser_)),
          connection_id_(other.connection_id_),
          released_(other.released_) {
        other.connection_id_ = -1;
        other.released_ = true;  // Mark moved-from as released
    }
    
    /**
     * Move assignment: transfer ownership with cleanup of current guard
     */
    ConnectionGuard& operator=(ConnectionGuard&& other) noexcept {
        if (this != &other) {
            release();  // Clean up current guard first
            releaser_ = std::move(other.releaser_);
            connection_id_ = other.connection_id_;
            released_ = other.released_;
            
            other.connection_id_ = -1;
            other.released_ = true;  // Mark moved-from as released
        }
        return *this;
    }
    
    /**
     * Destructor: exception-safe cleanup
     * 
     * Guaranteed to call releaser() exactly once if not yet released.
     * Catches and suppresses all exceptions per C++ guidelines.
     */
    ~ConnectionGuard() noexcept {
        release();
    }
    
    /**
     * Check if guard is still active (has not released yet)
     * 
     * @return true if connection still needs cleanup, false otherwise
     */
    bool isActive() const noexcept {
        return !released_.load(std::memory_order_acquire);
    }
    
    /**
     * Get connection ID (for debugging/logging)
     * 
     * @return Connection identifier, or -1 if not set
     */
    int getId() const noexcept {
        return connection_id_;
    }
    
    /**
     * Manual release (optional, idempotent)
     * 
     * Safe to call multiple times.
     * Prevents releaser() from being called more than once.
     * 
     * Guarantees:
     * - Exception-safe (catches all internal exceptions)
     * - Idempotent (safe to call multiple times)
     * - No-throw (noexcept)
     */
    void release() noexcept {
        if (!released_.exchange(true, std::memory_order_acq_rel)) {
            if (releaser_) {
                try {
                    releaser_();
                } catch (...) {
                    // RAII guarantee: never throw from cleanup
                    // Suppress exception per C++ guidelines
                    // In production, this should be logged via telemetry
                }
            }
        }
    }
    
    /**
     * Check if guard is valid (has a releaser function)
     * 
     * @return true if guard can perform cleanup
     */
    explicit operator bool() const noexcept {
        return static_cast<bool>(releaser_);
    }

private:
    Releaser releaser_;
    int connection_id_;
    std::atomic<bool> released_;
};

/**
 * Factory helper for creating ConnectionGuard with type-safe cleanup
 * 
 * Usage:
 * ```cpp
 * auto guard = makeConnectionGuard([&conn] { db_->releaseConnection(conn); });
 * ```
 */
template<typename Func>
inline ConnectionGuard makeConnectionGuard(Func&& cleanup) noexcept {
    return ConnectionGuard(std::forward<Func>(cleanup));
}

/**
 * Factory helper with connection ID for logging
 */
template<typename Func>
inline ConnectionGuard makeConnectionGuard(Func&& cleanup, int conn_id) noexcept {
    return ConnectionGuard(std::forward<Func>(cleanup), conn_id);
}

}  // namespace themis::index
}  // namespace themis

#endif  // THEMIS_INDEX_CONNECTION_GUARD_H
