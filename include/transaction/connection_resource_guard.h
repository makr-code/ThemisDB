/**
 * @file connection_resource_guard.h
 * @brief RAII-style connection management for transaction operations
 * @version 1.0
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note This module provides exception-safe connection lifecycle management
 *
 * This header defines RAII wrappers and guards for database connection management
 * in the transaction module. All connections acquired through these guards are
 * guaranteed to be released, even in the presence of exceptions.
 *
 * @see storage/database_connection_manager.h for underlying connection pool
 */

#pragma once

#include <chrono>
#include <cstdint>
#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>
#include "storage/database_connection_manager.h"
#include "utils/logger.h"

namespace themis {
namespace transaction {

/**
 * @brief RAII guard for database connections
 *
 * Ensures that a connection is automatically released when the guard
 * goes out of scope, even if an exception is thrown.
 *
 * **Thread Safety**: Not thread-safe. Each thread should have its own guard.
 *
 * **Exception Safety**: Strong exception guarantee. If construction fails,
 * no resources are leaked.
 *
 * **Usage Pattern**:
 * ```cpp
 * {
 *     ConnectionGuard guard(connection_manager);
 *     auto conn = guard.getConnection();
 *     if (!conn) {
 *         THEMIS_WARN("Failed to acquire connection");
 *         return; // Guard cleanup still happens
 *     }
 *     // Use connection...
 * }  // Connection automatically released on scope exit
 * ```
 */
class ConnectionGuard {
public:
    /**
     * @brief Create a connection guard
     *
     * Attempts to acquire a connection from the manager.
     *
     * @param manager Reference to DatabaseConnectionManager
     * @param blocking Whether to block waiting for available connection
     * @param timeout Maximum time to wait for connection acquisition
     * @throws std::runtime_error if connection manager is null or invalid
     */
    explicit ConnectionGuard(
        storage::DatabaseConnectionManager& manager,
        bool blocking = true,
        std::chrono::seconds timeout = std::chrono::seconds(10)
    );

    /**
     * @brief Destructor - releases connection
     *
     * Automatically returns the connection to the pool, even if
     * an exception is being handled. If an error occurred during
     * use, the connection is marked accordingly.
     *
     * **Exception Safety**: No-throw guarantee
     */
    ~ConnectionGuard() noexcept;

    // Non-copyable to prevent double-release
    ConnectionGuard(const ConnectionGuard&) = delete;
    ConnectionGuard& operator=(const ConnectionGuard&) = delete;

    // Moveable for efficient transfer
    ConnectionGuard(ConnectionGuard&& other) noexcept;
    ConnectionGuard& operator=(ConnectionGuard&& other) noexcept;

    /**
     * @brief Get the acquired connection
     *
     * Returns the acquired connection, or nullptr if acquisition failed.
     * The returned connection is owned by this guard and must not be
     * manually released.
     *
     * @return Shared pointer to connection, or nullptr
     *
     * **Thread Safety**: Safe to call from any thread (but connection
     * itself is not thread-safe for operations)
     */
    std::shared_ptr<storage::DatabaseConnectionManager::Connection> 
    getConnection() noexcept;

    /**
     * @brief Get the acquired connection (const version)
     *
     * @return Const shared pointer to connection, or nullptr
     */
    std::shared_ptr<const storage::DatabaseConnectionManager::Connection>
    getConnection() const noexcept;

    /**
     * @brief Check if connection is valid
     *
     * @return true if connection was acquired and is valid
     */
    bool isValid() const noexcept;

    /**
     * @brief Manually mark an error condition
     *
     * Call this method if an error occurs during connection use.
     * The connection will be marked for health re-evaluation when released.
     *
     * @param error_desc Error description for logging
     *
     * **Thread Safety**: Safe to call from any thread
     */
    void markError(std::string_view error_desc) noexcept;

    /**
     * @brief Manually release the connection
     *
     * Allows early release of the connection without waiting for
     * guard destruction. Subsequent calls have no effect.
     *
     * **Thread Safety**: Safe to call from any thread
     */
    void release() noexcept;

    /**
     * @brief Check if connection has been released
     *
     * @return true if connection has been released (manually or via destructor)
     */
    bool isReleased() const noexcept;

private:
    storage::DatabaseConnectionManager* manager_;
    std::shared_ptr<storage::DatabaseConnectionManager::Connection> conn_;
    bool error_occurred_{false};
    bool released_{false};
};

/**
 * @brief Connection pool usage scope tracker
 *
 * Tracks active connection usage for debugging and diagnostics.
 * Automatically records connection acquisition and release.
 */
class ConnectionScopeTracker {
public:
    /**
     * @brief Create a scope tracker
     *
     * @param operation_name Name of the operation using connection
     * @param is_write true if this is a write operation
     */
    explicit ConnectionScopeTracker(
        std::string_view operation_name,
        bool is_write = false
    );

    ~ConnectionScopeTracker() noexcept;

    /**
     * @brief Record operation completion
     */
    void recordSuccess() noexcept;

    /**
     * @brief Record operation failure
     *
     * @param error_msg Error description
     */
    void recordFailure(std::string_view error_msg) noexcept;

    /**
     * @brief Get operation duration in milliseconds
     */
    uint64_t getDurationMs() const noexcept;

private:
    std::string operation_name_;
    bool is_write_;
    std::chrono::system_clock::time_point start_time_;
    bool recorded_{false};
};

/**
 * @brief Transaction connection lifecycle guard
 *
 * Manages the complete lifecycle of connections used within a transaction.
 * Provides exception-safe transaction resource management.
 *
 * **Usage Pattern**:
 * ```cpp
 * {
 *     TransactionConnectionGuard txn_guard(txn_id, connection_manager);
 *
 *     auto conn = txn_guard.acquireConnection();
 *     if (!conn) {
 *         THEMIS_ERROR("Failed to acquire connection for transaction");
 *         return;
 *     }
 *
 *     try {
 *         // Perform transaction work...
 *         txn_guard.recordSuccess();
 *     } catch (const std::exception& e) {
 *         THEMIS_WARN("Transaction operation failed: {}", e.what());
 *         txn_guard.recordFailure(e.what());
 *         // Guard ensures cleanup even on exception
 *     }
 * }  // All connections automatically released here
 * ```
 */
class TransactionConnectionGuard {
public:
    /**
     * @brief Create a transaction connection guard
     *
     * @param txn_id Transaction ID for logging
     * @param manager Reference to DatabaseConnectionManager
     */
    explicit TransactionConnectionGuard(
        uint64_t txn_id,
        storage::DatabaseConnectionManager& manager
    );

    ~TransactionConnectionGuard() noexcept;

    // Non-copyable
    TransactionConnectionGuard(const TransactionConnectionGuard&) = delete;
    TransactionConnectionGuard& operator=(const TransactionConnectionGuard&) = delete;

    /**
     * @brief Acquire a connection for the transaction
     *
     * @param operation_name Name of the operation (for tracking)
     * @param is_write Whether this is a write operation
     * @return Shared pointer to connection, or nullptr on failure
     */
    std::shared_ptr<storage::DatabaseConnectionManager::Connection>
    acquireConnection(
        std::string_view operation_name,
        bool is_write = false
    ) noexcept;

    /**
     * @brief Record successful operation
     *
     * @param operation_name Name of the operation
     */
    void recordSuccess(std::string_view operation_name) noexcept;

    /**
     * @brief Record failed operation
     *
     * @param operation_name Name of the operation
     * @param error_msg Error description
     */
    void recordFailure(
        std::string_view operation_name,
        std::string_view error_msg
    ) noexcept;

    /**
     * @brief Get accumulated connection time
     *
     * @return Total time spent in connection operations (milliseconds)
     */
    uint64_t getConnectionTimeMs() const noexcept;

    /**
     * @brief Get number of connections used
     */
    size_t getConnectionCount() const noexcept;

    /**
     * @brief Get number of successful operations
     */
    size_t getSuccessCount() const noexcept;

    /**
     * @brief Get number of failed operations
     */
    size_t getFailureCount() const noexcept;

    /**
     * @brief Manually release all connections
     *
     * Releases all acquired connections and resets state.
     * Called automatically on destruction.
     */
    void releaseAllConnections() noexcept;

private:
    uint64_t txn_id_;
    storage::DatabaseConnectionManager* manager_;
    std::vector<std::shared_ptr<storage::DatabaseConnectionManager::Connection>> connections_;
    std::vector<ConnectionScopeTracker> trackers_;
    size_t success_count_{0};
    size_t failure_count_{0};
    uint64_t total_connection_time_ms_{0};
};

/**
 * @brief Helper function to safely execute operation with connection
 *
 * Automatically acquires connection, executes operation, and releases
 * connection. Provides exception-safe wrapper for single operations.
 *
 * **Exception Safety**: Strong guarantee
 *
 * @tparam Func Callable type that accepts shared_ptr<Connection>
 * @param manager Reference to DatabaseConnectionManager
 * @param operation Callable to execute with connection
 * @param operation_name Name for logging
 * @return true if operation completed successfully
 *
 * **Usage**:
 * ```cpp
 * bool success = executeWithConnection(
 *     connection_manager,
 *     [](auto conn) {
 *         // Use connection
 *         return conn->ping();
 *     },
 *     "ping_operation"
 * );
 * ```
 */
template<typename Func>
bool executeWithConnection(
    storage::DatabaseConnectionManager& manager,
    Func&& operation,
    std::string_view operation_name = "operation"
) noexcept {
    ConnectionGuard guard(manager);
    auto conn = guard.getConnection();
    
    if (!conn) {
        THEMIS_WARN("Failed to acquire connection for {}", operation_name);
        return false;
    }
    
    try {
        using operation_result_t = std::invoke_result_t<
            Func,
            std::shared_ptr<storage::DatabaseConnectionManager::Connection>
        >;

        if constexpr (std::is_void_v<operation_result_t>) {
            operation(conn);
            return true;
        } else if constexpr (std::is_convertible_v<operation_result_t, bool>) {
            return static_cast<bool>(operation(conn));
        } else {
            static_assert(std::is_convertible_v<operation_result_t, bool>,
                "executeWithConnection() requires operation to return void or bool-convertible type");
            return false;
        }
    } catch (const std::exception& e) {
        THEMIS_WARN("Operation '{}' failed: {}", operation_name, e.what());
        guard.markError(e.what());
        return false;
    }
}

} // namespace transaction
} // namespace themis
