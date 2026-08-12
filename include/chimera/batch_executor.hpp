/**
 * @file batch_executor.hpp
 * @brief Batch operation executor for throughput-focused workloads.
 *
 * Wraps an IDatabaseAdapter and coalesces individual operations into
 * configurable batches, trading per-operation latency for aggregate throughput.
 */

#pragma once

#include "chimera/database_adapter.hpp"
#include <chrono>
#include <cstdint>
#include <memory>
#include <mutex>
#include <queue>
#include <vector>

namespace chimera {

/**
 * @struct BatchConfig
 * @brief Configuration for batch operation behavior.
 */
struct BatchConfig {
    /// Target batch size before auto-flush (rows).
    size_t batch_size = 1000;

    /// Number of batches to pipeline concurrently.
    size_t pipeline_depth = 2;

    /// Auto-flush timeout in milliseconds; 0 = no timeout.
    uint32_t timeout_ms = 5000;

    /// Whether to wrap batch operations in a transaction.
    bool auto_commit = true;

    /// Whether to immediately propagate errors or buffer them.
    bool fail_fast = false;
};

/**
 * @struct BatchStatistics
 * @brief Aggregated statistics from a batch operation.
 */
struct BatchStatistics {
    /// Total rows processed in this batch.
    size_t rows_processed = 0;

    /// Total rows successfully committed.
    size_t rows_committed = 0;

    /// Total rows that failed.
    size_t rows_failed = 0;

    /// Total time spent in this batch (milliseconds).
    uint64_t total_time_ms = 0;

    /// Number of individual operations (may differ from rows for bulk ops).
    size_t operation_count = 0;

    /// Last error encountered, if any.
    std::string last_error;
};

/**
 * @class IBatchAdapter
 * @brief Mixin interface for batched operations.
 * 
 * @details
 * Adapters that support batch operations (for throughput optimization)
 * should inherit from this interface along with IDatabaseAdapter.
 * 
 * Batch operations are queued and executed in groups to reduce round-trips
 * to the backend database.
 */
class IBatchAdapter {
public:
    virtual ~IBatchAdapter() = default;

    /**
     * @brief Queue a row for insertion (batched).
     * 
     * @param table_name Target table or collection.
     * @param row Row to insert.
     * 
     * @return Result<bool> indicating if row was queued (does not mean success).
     * 
     * @details
     * The row is added to the batch queue. It is not executed immediately.
     * Call flush() to execute all queued operations.
     */
    virtual Result<bool> queue_insert(
        const std::string& table_name,
        const RelationalRow& row
    ) = 0;

    /**
     * @brief Queue multiple rows for insertion (batched).
     * 
     * @param table_name Target table or collection.
     * @param rows Rows to insert.
     * 
     * @return Result<bool> indicating if rows were queued.
     */
    virtual Result<bool> queue_insert_batch(
        const std::string& table_name,
        const std::vector<RelationalRow>& rows
    ) = 0;

    /**
     * @brief Queue a row for update (batched).
     * 
     * @param table_name Target table.
     * @param row New values.
     * @param where_clause WHERE condition (adapter-specific syntax).
     * 
     * @return Result<bool> indicating if operation was queued.
     */
    virtual Result<bool> queue_update(
        const std::string& table_name,
        const RelationalRow& row,
        const std::string& where_clause
    ) = 0;

    /**
     * @brief Queue a row for deletion (batched).
     * 
     * @param table_name Target table.
     * @param where_clause WHERE condition (adapter-specific syntax).
     * 
     * @return Result<bool> indicating if operation was queued.
     */
    virtual Result<bool> queue_delete(
        const std::string& table_name,
        const std::string& where_clause
    ) = 0;

    /**
     * @brief Force immediate execution of all queued batch operations.
     * 
     * @return Result<BatchStatistics> with aggregated statistics.
     * 
     * @details
     * Executes all pending operations in the queue. On success,
     * returns detailed statistics including rows_committed and rows_failed.
     */
    virtual Result<BatchStatistics> flush() = 0;

    /**
     * @brief Get the number of operations currently pending in the batch queue.
     * 
     * @return Number of queued operations (not yet executed).
     */
    virtual size_t get_pending_count() const = 0;

    /**
     * @brief Update batch configuration for subsequent operations.
     * 
     * @param config New batch configuration.
     * @return Result<bool> indicating success.
     */
    virtual Result<bool> set_batch_config(const BatchConfig& config) = 0;

    /**
     * @brief Get the current batch configuration.
     * 
     * @return Current BatchConfig.
     */
    virtual const BatchConfig& get_batch_config() const = 0;
};

} // namespace chimera

#endif // CHIMERA_BATCH_EXECUTOR_HPP
