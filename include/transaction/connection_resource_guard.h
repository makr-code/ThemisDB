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

class ConnectionGuard {
public:
    explicit ConnectionGuard(
        storage::DatabaseConnectionManager& manager,
        bool blocking = true,
        std::chrono::seconds timeout = std::chrono::seconds(10)
    );

    ~ConnectionGuard() noexcept;

    // Non-copyable to prevent double-release
    ConnectionGuard(const ConnectionGuard&) = delete;
    ConnectionGuard& operator=(const ConnectionGuard&) = delete;

    // Moveable for efficient transfer
    ConnectionGuard(ConnectionGuard&& other) noexcept;
    ConnectionGuard& operator=(ConnectionGuard&& other) noexcept;

    std::shared_ptr<storage::DatabaseConnectionManager::Connection> 
    getConnection() noexcept;

    std::shared_ptr<const storage::DatabaseConnectionManager::Connection>
    getConnection() const noexcept;

    /**
     * @brief Is Valid.
     * @return True when the operation succeeds.
     * @note Exception safety: noexcept.
     */
    bool isValid() const noexcept;

    /**
     * @brief Mark Error.
     * @param[in] error_desc Input parameter.
     * @note Exception safety: noexcept.
     */
    void markError(std::string_view error_desc) noexcept;

    /**
     * @brief Release.
     * @note Exception safety: noexcept.
     */
    void release() noexcept;

    /**
     * @brief Is Released.
     * @return True when the operation succeeds.
     * @note Exception safety: noexcept.
     */
    bool isReleased() const noexcept;

private:
    storage::DatabaseConnectionManager* manager_;
    std::shared_ptr<storage::DatabaseConnectionManager::Connection> conn_;
    bool error_occurred_{false};
    bool released_{false};
};

class ConnectionScopeTracker {
public:
    explicit ConnectionScopeTracker(
        std::string_view operation_name,
        bool is_write = false
    );

    ~ConnectionScopeTracker() noexcept;

    /**
     * @brief Record Success.
     * @note Exception safety: noexcept.
     */
    void recordSuccess() noexcept;

    /**
     * @brief Record Failure.
     * @param[in] error_msg Input parameter.
     * @note Exception safety: noexcept.
     */
    void recordFailure(std::string_view error_msg) noexcept;

    /**
     * @brief Get Duration Ms.
     * @return Return value.
     * @note Exception safety: noexcept.
     */
    uint64_t getDurationMs() const noexcept;

private:
    std::string operation_name_;
    bool is_write_;
    std::chrono::system_clock::time_point start_time_;
    bool recorded_{false};
};

class TransactionConnectionGuard {
public:
    /**
     * @brief Transaction Connection Guard.
     * @param[in] txn_id Identifier of the txn.
     * @param[in,out] manager Input/output parameter.
     * @return Return value.
     */
    explicit TransactionConnectionGuard(
        uint64_t txn_id,
        storage::DatabaseConnectionManager& manager
    );

    ~TransactionConnectionGuard() noexcept;

    // Non-copyable
    TransactionConnectionGuard(const TransactionConnectionGuard&) = delete;
    TransactionConnectionGuard& operator=(const TransactionConnectionGuard&) = delete;

    std::shared_ptr<storage::DatabaseConnectionManager::Connection>
    acquireConnection(
        std::string_view operation_name,
        bool is_write = false
    ) noexcept;

    /**
     * @brief Record Success.
     * @param[in] operation_name Name of the operation.
     * @note Exception safety: noexcept.
     */
    void recordSuccess(std::string_view operation_name) noexcept;

    /**
     * @brief Record Failure.
     * @param[in] operation_name Name of the operation.
     * @param[in] error_msg Input parameter.
     * @note Exception safety: noexcept.
     */
    void recordFailure(
        std::string_view operation_name,
        std::string_view error_msg
    ) noexcept;

    /**
     * @brief Get Connection Time Ms.
     * @return Return value.
     * @note Exception safety: noexcept.
     */
    uint64_t getConnectionTimeMs() const noexcept;

    /**
     * @brief Get Connection Count.
     * @return Return value.
     * @note Exception safety: noexcept.
     */
    size_t getConnectionCount() const noexcept;

    /**
     * @brief Get Success Count.
     * @return Return value.
     * @note Exception safety: noexcept.
     */
    size_t getSuccessCount() const noexcept;

    /**
     * @brief Get Failure Count.
     * @return Return value.
     * @note Exception safety: noexcept.
     */
    size_t getFailureCount() const noexcept;

    /**
     * @brief Release All Connections.
     * @note Exception safety: noexcept.
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

template<typename Func>
bool executeWithConnection(
    storage::DatabaseConnectionManager& manager,
    Func&& operation,
    std::string_view operation_name = "operation"
) noexcept {
    /**
     * @brief Guard.
     * @param[in] manager Input parameter.
     * @return Return value.
     */
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
