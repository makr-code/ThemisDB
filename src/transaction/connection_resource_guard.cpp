/**
 * @file connection_resource_guard.cpp
 * @brief RAII-style connection management implementation
 * @version 1.0
 * @note Maturity: 🟢 PRODUCTION-READY
 *
 * Implementation of connection guards for exception-safe connection management.
 */

#include "transaction/connection_resource_guard.h"
#include "utils/logger.h"
#include <spdlog/spdlog.h>

namespace themis {
namespace transaction {

// ============================================================================
// ConnectionGuard Implementation
// ============================================================================

ConnectionGuard::ConnectionGuard(
    storage::DatabaseConnectionManager& manager,
    bool blocking,
    std::chrono::seconds timeout
)
    : manager_(&manager),
      conn_(manager.acquireConnection(blocking, timeout)),
      error_occurred_(false),
      released_(false) {
    
    if (!conn_) {
        THEMIS_DEBUG("Connection acquisition returned nullptr");
    } else {
        THEMIS_DEBUG("Connection acquired successfully");
    }
}

ConnectionGuard::~ConnectionGuard() noexcept {
    release();
}

ConnectionGuard::ConnectionGuard(ConnectionGuard&& other) noexcept
    : manager_(other.manager_),
      conn_(std::move(other.conn_)),
      error_occurred_(other.error_occurred_),
      released_(other.released_) {
    
    // Mark source as released to prevent double-release
    other.released_ = true;
    other.manager_ = nullptr;
}

ConnectionGuard& ConnectionGuard::operator=(ConnectionGuard&& other) noexcept {
    if (this != &other) {
        // Release current connection first
        release();
        
        manager_ = other.manager_;
        conn_ = std::move(other.conn_);
        error_occurred_ = other.error_occurred_;
        released_ = other.released_;
        
        // Mark source as released to prevent double-release
        other.released_ = true;
        other.manager_ = nullptr;
    }
    return *this;
}

std::shared_ptr<storage::DatabaseConnectionManager::Connection>
ConnectionGuard::getConnection() noexcept {
    return conn_;
}

std::shared_ptr<const storage::DatabaseConnectionManager::Connection>
ConnectionGuard::getConnection() const noexcept {
    return conn_;
}

bool ConnectionGuard::isValid() const noexcept {
    return conn_ != nullptr && conn_->isValid();
}

void ConnectionGuard::markError(std::string_view error_desc) noexcept {
    error_occurred_ = true;
    if (!error_desc.empty()) {
        THEMIS_DEBUG("Connection error marked: {}", error_desc);
    }
}

void ConnectionGuard::release() noexcept {
    if (released_ || !manager_ || !conn_) {
        return;
    }
    
    try {
        manager_->releaseConnection(conn_, error_occurred_);
        THEMIS_DEBUG("Connection released (error_occurred={})", error_occurred_);
    } catch (const std::exception& e) {
        THEMIS_WARN("Exception during connection release: {}", e.what());
    } catch (...) {
        THEMIS_WARN("Unknown exception during connection release");
    }
    
    released_ = true;
    conn_.reset();
    manager_ = nullptr;
}

bool ConnectionGuard::isReleased() const noexcept {
    return released_;
}

// ============================================================================
// ConnectionScopeTracker Implementation
// ============================================================================

ConnectionScopeTracker::ConnectionScopeTracker(
    std::string_view operation_name,
    bool is_write
)
    : operation_name_(operation_name),
      is_write_(is_write),
      start_time_(std::chrono::system_clock::now()),
      recorded_(false) {
    
    THEMIS_DEBUG("Connection scope tracker started for '{}' (write={})",
                 operation_name_, is_write_);
}

ConnectionScopeTracker::~ConnectionScopeTracker() noexcept {
    if (!recorded_) {
        recordFailure("Destroyed without explicit success/failure recording");
    }
}

void ConnectionScopeTracker::recordSuccess() noexcept {
    if (recorded_) {
        THEMIS_WARN("Connection scope already recorded");
        return;
    }
    
    auto duration = getDurationMs();
    THEMIS_DEBUG("Connection operation '{}' completed successfully ({}ms)",
                 operation_name_, duration);
    recorded_ = true;
}

void ConnectionScopeTracker::recordFailure(std::string_view error_msg) noexcept {
    if (recorded_) {
        THEMIS_WARN("Connection scope already recorded");
        return;
    }
    
    auto duration = getDurationMs();
    THEMIS_WARN("Connection operation '{}' failed ({}ms): {}",
                operation_name_, duration, error_msg);
    recorded_ = true;
}

uint64_t ConnectionScopeTracker::getDurationMs() const noexcept {
    auto now = std::chrono::system_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
        now - start_time_);
    return duration.count();
}

// ============================================================================
// TransactionConnectionGuard Implementation
// ============================================================================

TransactionConnectionGuard::TransactionConnectionGuard(
    uint64_t txn_id,
    storage::DatabaseConnectionManager& manager
)
    : txn_id_(txn_id),
      manager_(&manager),
      success_count_(0),
      failure_count_(0),
      total_connection_time_ms_(0) {
    
    THEMIS_DEBUG("Transaction connection guard created for txn {}", txn_id_);
}

TransactionConnectionGuard::~TransactionConnectionGuard() noexcept {
    releaseAllConnections();
    
    THEMIS_DEBUG(
        "Transaction {} connection guard destroyed: {} connections, "
        "{} successes, {} failures, {}ms total",
        txn_id_,static_cast<int>(connections_.size()), success_count_, failure_count_,
        total_connection_time_ms_
    );
}

std::shared_ptr<storage::DatabaseConnectionManager::Connection>
TransactionConnectionGuard::acquireConnection(
    std::string_view operation_name,
    bool is_write
) noexcept {
    if (!manager_) {
        THEMIS_WARN("Transaction {} connection guard manager is null", txn_id_);
        return nullptr;
    }
    
    auto conn = manager_->acquireConnection(true, std::chrono::seconds(10));
    if (!conn) {
        THEMIS_WARN("Transaction {} failed to acquire connection for '{}'",
                    txn_id_, operation_name);
        failure_count_++;
        return nullptr;
    }
    
    connections_.push_back(conn);
    trackers_.emplace_back(operation_name, is_write);
    
    THEMIS_DEBUG("Transaction {} acquired connection for '{}' (total: {})",
                 txn_id_, operation_name,static_cast<int>(connections_.size()));
    
    return conn;
}

void TransactionConnectionGuard::recordSuccess(
    std::string_view operation_name
) noexcept {
    if (trackers_.empty()) {
        THEMIS_WARN("No active tracker to record success for '{}'", operation_name);
        return;
    }
    
    auto& tracker = trackers_.back();
    tracker.recordSuccess();
    total_connection_time_ms_ += tracker.getDurationMs();
    success_count_++;
}

void TransactionConnectionGuard::recordFailure(
    std::string_view operation_name,
    std::string_view error_msg
) noexcept {
    if (trackers_.empty()) {
        THEMIS_WARN("No active tracker to record failure for '{}'", operation_name);
        return;
    }
    
    auto& tracker = trackers_.back();
    tracker.recordFailure(error_msg);
    total_connection_time_ms_ += tracker.getDurationMs();
    failure_count_++;
}

uint64_t TransactionConnectionGuard::getConnectionTimeMs() const noexcept {
    return total_connection_time_ms_;
}

size_t TransactionConnectionGuard::getConnectionCount() const noexcept {
    return static_cast<int>(connections_.size());
}

size_t TransactionConnectionGuard::getSuccessCount() const noexcept {
    return success_count_;
}

size_t TransactionConnectionGuard::getFailureCount() const noexcept {
    return failure_count_;
}

void TransactionConnectionGuard::releaseAllConnections() noexcept {
    if (!manager_) {
        return;
    }
    
    for (auto& conn : connections_) {
        try {
            manager_->releaseConnection(conn, false);
        } catch (const std::exception& e) {
            THEMIS_WARN("Exception releasing connection for txn {}: {}",
                       txn_id_, e.what());
        } catch (...) {
            THEMIS_WARN("Unknown exception releasing connection for txn {}", txn_id_);
        }
    }
    
    connections_.clear();
    trackers_.clear();
    manager_ = nullptr;
}

} // namespace transaction
} // namespace themis
