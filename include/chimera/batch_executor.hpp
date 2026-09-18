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

struct BatchConfig {
    size_t batch_size = 1000;

    size_t pipeline_depth = 2;

    uint32_t timeout_ms = 5000;

    bool auto_commit = true;

    bool fail_fast = false;
};

struct BatchStatistics {
    size_t rows_processed = 0;

    size_t rows_committed = 0;

    size_t rows_failed = 0;

    uint64_t total_time_ms = 0;

    size_t operation_count = 0;

    std::string last_error;
};

class IBatchAdapter {
public:
    /**
     * @brief IBatch Adapter.
     * @return Return value.
     */
    virtual ~IBatchAdapter() = default;

    /**
     * @brief Queue insert.
     * @param[in] table_name Name of the table.
     * @param[in] row Input parameter.
     * @return Return value.
     */
    virtual Result<bool> queue_insert(
        const std::string& table_name,
        const RelationalRow& row
    ) = 0;

    /**
     * @brief Queue insert batch.
     * @param[in] table_name Name of the table.
     * @param[in] rows Input parameter.
     * @return Return value.
     */
    virtual Result<bool> queue_insert_batch(
        const std::string& table_name,
        const std::vector<RelationalRow>& rows
    ) = 0;

    /**
     * @brief Queue update.
     * @param[in] table_name Name of the table.
     * @param[in] row Input parameter.
     * @param[in] where_clause Input parameter.
     * @return Return value.
     */
    virtual Result<bool> queue_update(
        const std::string& table_name,
        const RelationalRow& row,
        const std::string& where_clause
    ) = 0;

    /**
     * @brief Queue delete.
     * @param[in] table_name Name of the table.
     * @param[in] where_clause Input parameter.
     * @return Return value.
     */
    virtual Result<bool> queue_delete(
        const std::string& table_name,
        const std::string& where_clause
    ) = 0;

    /**
     * @brief Flush.
     * @return Return value.
     */
    virtual Result<BatchStatistics> flush() = 0;

    /**
     * @brief Get pending count.
     * @return Return value.
     */
    virtual size_t get_pending_count() const = 0;

    /**
     * @brief Set batch config.
     * @param[in] config Input parameter.
     * @return Return value.
     */
    virtual Result<bool> set_batch_config(const BatchConfig& config) = 0;

    /**
     * @brief Get batch config.
     * @return Return value.
     */
    virtual const BatchConfig& get_batch_config() const = 0;
};

} // namespace chimera
