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

struct ResultRecord {
    std::string record_id;               ///< Unique record identifier
    std::vector<std::string> values;     ///< Field values
    int64_t timestamp_ms{0};             ///< Record timestamp
    bool is_transactional{true};         ///< Is part of transaction
};

struct ResultBatch {
    std::string batch_id;                ///< Unique batch identifier
    std::vector<ResultRecord> records;   ///< Result records in batch
    int64_t batch_num{0};                ///< Sequence number
    bool auto_flush{true};               ///< Auto-flush after write
};

struct WriteResult {
    bool success{false};                 ///< Write succeeded
    std::string error_message;           ///< Error details if failed
    int64_t records_written{0};          ///< Number of records written
    int64_t records_failed{0};           ///< Number of failed records
    std::chrono::milliseconds duration{0};  ///< Write operation time
};

class ResultAggregator {
public:
    /**
     * @brief Result Aggregator.
     * @param[in] pool Input parameter.
     * @return Return value.
     */
    explicit ResultAggregator(std::shared_ptr<ConnectionPool> pool);

    ~ResultAggregator() noexcept;

    /**
     * @brief ======================================================================== Result Writing (Gaps A-2-11 to A-2-13) ========================================================================
     * @param[in] batch Input parameter.
     * @return Return value.
     */

    WriteResult WriteResults(const ResultBatch& batch);

    /**
     * @brief Flush Buffer.
     * @return Return value.
     */
    WriteResult FlushBuffer();

    /**
     * @brief Close Connection.
     * @return True when the operation succeeds.
     */
    bool CloseConnection();

    /**
     * @brief ======================================================================== Health & Diagnostics (Gaps A-2-15 to A-2-16) ========================================================================
     * @return True when the operation succeeds.
     * @note Exception safety: noexcept.
     */

    bool IsConnectionHealthy() const noexcept;

    struct Stats {
        int64_t total_written{0};
        int64_t total_failed{0};
        int64_t peak_batch_size{0};
        int error_count{0};
    };
    /**
     * @brief Get Stats.
     * @return Return value.
     * @note Exception safety: noexcept.
     */
    Stats GetStats() const noexcept;

    /**
     * @brief Reset Stats.
     * @note Exception safety: noexcept.
     */
    void ResetStats() noexcept;

    void SetBatchSize(int size) noexcept {
        batch_size_ = size;
    }

    std::shared_ptr<ConnectionPool> GetPool() const noexcept {
        return pool_;
    }

private:
    // ========================================================================
    // Helper Methods (Exception-Safe)
    // ========================================================================

    /**
     * @brief Begin Transaction.
     * @param[in] connection_id Identifier of the connection.
     */
    void BeginTransaction(int connection_id);

    /**
     * @brief Commit Transaction.
     * @param[in] connection_id Identifier of the connection.
     */
    void CommitTransaction(int connection_id);

    /**
     * @brief Rollback Transaction.
     * @param[in] connection_id Identifier of the connection.
     * @note Exception safety: noexcept.
     */
    void RollbackTransaction(int connection_id) noexcept;

    /**
     * @brief Write Record.
     * @param[in] connection_id Identifier of the connection.
     * @param[in] record Input parameter.
     */
    void WriteRecord(int connection_id, const ResultRecord& record);

    /**
     * @brief Log Diagnostics.
     * @param[in] error Input parameter.
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
