/**
 * @file result_aggregator.h
 * @brief Result writing and aggregation with connection pool management.
 * @version 1.0.0
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Phase 2 A-2: DB Connection Leak (10 gaps) — Scoped guards + transaction safety
 *
 * Writes aggregated results to database with automatic connection lifecycle
 * management. Provides transaction safety and automatic cleanup on exceptions.
 *
 * Copyright (c) 2025 VCC-URN Project
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <chrono>
#include <functional>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

#include "analytics/connection_guard.h"

namespace themisdb {
namespace analytics {

// Forward declarations
class ConnectionPool;

/**
 * @brief Individual result record to be written.
 */
struct ResultRecord {
    std::string record_id;               ///< Unique record identifier
    std::vector<std::string> values;     ///< Field values
    int64_t timestamp_ms{0};             ///< Record timestamp
    bool is_transactional{true};         ///< Is part of transaction
};

/**
 * @brief Batch of results to write to database.
 */
struct ResultBatch {
    std::string batch_id;                ///< Unique batch identifier
    std::vector<ResultRecord> records;   ///< Result records in batch
    int64_t batch_num{0};                ///< Sequence number
    bool auto_flush{true};               ///< Auto-flush after write
};

/**
 * @brief Result write operation outcome.
 */
struct WriteResult {
    bool success{false};                 ///< Write succeeded
    std::string error_message;           ///< Error details if failed
    int64_t records_written{0};          ///< Number of records written
    int64_t records_failed{0};           ///< Number of failed records
    std::chrono::milliseconds duration{0};  ///< Write operation time
};

/**
 * @brief Result aggregator with connection pool management.
 *
 * Thread-safe result writer with automatic connection lifecycle management.
 * Implements transaction safety with rollback on exceptions. All database
 * connections are managed via RAII ConnectionGuard.
 *
 * Phase 2 A-2 Gaps Addressed:
 * - A-2-11: Connection release on normal exit (WriteResults)
 * - A-2-12: Connection release on exception (WriteResults)
 * - A-2-13: Scoped transaction guards (begin/commit/rollback)
 * - A-2-14: Batch flush with connection reuse
 * - A-2-15: Exception handler for write failures
 * - A-2-16: Connection health check before use
 * - A-2-17: Remove dangling connection references
 * - A-2-18: Cleanup on FlushBuffer exception
 * - A-2-19: Pool cleanup on destructor
 * - A-2-20: Error recovery paths documented
 */
class ResultAggregator {
public:
    /**
     * Construct aggregator with connection pool.
     *
     * @param pool Shared connection pool (must outlive aggregator)
     * @throws std::invalid_argument if pool is null
     */
    explicit ResultAggregator(std::shared_ptr<ConnectionPool> pool);

    /**
     * Destructor: cleanup and pool release.
     *
     * Phase 2 A-2-19: Pool cleanup on destructor
     */
    ~ResultAggregator() noexcept;

    // ========================================================================
    // Result Writing (Gaps A-2-11 to A-2-13)
    // ========================================================================

    /**
     * Write results with transaction safety.
     *
     * Acquires connection via RAII guard, begins transaction, writes records,
     * and commits. On any exception, transaction is rolled back and connection
     * returned to pool via guard destructor.
     *
     * **RAII Pattern:** Connection acquired, released on scope exit.
     * **Exception Safety:** Strong exception guarantee with rollback.
     * **Transaction Safety:** BEGIN/COMMIT/ROLLBACK via guards.
     *
     * Phase 2 A-2-11: Connection release on normal exit
     * Phase 2 A-2-12: Connection release on exception
     * Phase 2 A-2-13: Scoped transaction guards
     *
     * @param batch Results to write
     * @return Write result (success field indicates outcome)
     *
     * @throws std::runtime_error if connection pool exhausted
     * @throws std::exception if write operation fails (after rollback)
     */
    WriteResult WriteResults(const ResultBatch& batch);

    /**
     * Flush pending results to database.
     *
     * Phase 2 A-2-14: Batch flush with connection reuse
     * Phase 2 A-2-18: Cleanup on FlushBuffer exception
     *
     * @return Write result
     *
     * @throws std::runtime_error if flush fails
     */
    WriteResult FlushBuffer();

    /**
     * Close connection and cleanup resources.
     *
     * Phase 2 A-2-17: Remove dangling connection references
     * Phase 2 A-2-20: Error recovery paths
     *
     * @return true if cleanup successful
     */
    bool CloseConnection();

    // ========================================================================
    // Health & Diagnostics (Gaps A-2-15 to A-2-16)
    // ========================================================================

    /**
     * Check connection health before use.
     *
     * Phase 2 A-2-16: Connection health check before use
     *
     * @return true if connection pool is healthy
     */
    bool IsConnectionHealthy() const noexcept;

    /**
     * Get aggregator statistics.
     *
     * @return {total_written, total_failed, peak_batch_size, error_count}
     */
    struct Stats {
        int64_t total_written{0};
        int64_t total_failed{0};
        int64_t peak_batch_size{0};
        int error_count{0};
    };
    Stats GetStats() const noexcept;

    /**
     * Reset statistics counters.
     */
    void ResetStats() noexcept;

    /**
     * Set batch size for automatic flush.
     *
     * @param size Batch size threshold
     */
    void SetBatchSize(int size) noexcept {
        batch_size_ = size;
    }

    /**
     * Get underlying connection pool.
     *
     * @return Shared pointer to pool
     */
    std::shared_ptr<ConnectionPool> GetPool() const noexcept {
        return pool_;
    }

private:
    // ========================================================================
    // Helper Methods (Exception-Safe)
    // ========================================================================

    /**
     * Begin transaction with error handling.
     *
     * Phase 2 A-2-15: Exception handler for write failures
     *
     * @param connection_id Connection ID
     * @throws std::runtime_error if transaction cannot start
     */
    void BeginTransaction(int connection_id);

    /**
     * Commit transaction with cleanup on failure.
     *
     * @param connection_id Connection ID
     * @throws std::runtime_error if commit fails
     */
    void CommitTransaction(int connection_id);

    /**
     * Rollback transaction on exception.
     *
     * @param connection_id Connection ID
     * @note No-throw guarantee
     */
    void RollbackTransaction(int connection_id) noexcept;

    /**
     * Write single record to connection.
     *
     * @param connection_id Connection ID
     * @param record Record to write
     * @throws std::runtime_error if write fails
     */
    void WriteRecord(int connection_id, const ResultRecord& record);

    /**
     * Log aggregator diagnostics.
     *
     * @param error Error message
     */
    void LogDiagnostics(const std::string& error) const;

    // ========================================================================
    // Member Variables
    // ========================================================================

    std::shared_ptr<ConnectionPool> pool_;      ///< Connection pool (never null)
    std::vector<ResultRecord> buffer_;          ///< Pending records
    int batch_size_{1000};                      ///< Auto-flush threshold
    int64_t total_written_{0};                  ///< Cumulative records written
    int64_t total_failed_{0};                   ///< Cumulative write failures
    int error_count_{0};                        ///< Recent error count
};

}  // namespace analytics
}  // namespace themisdb
