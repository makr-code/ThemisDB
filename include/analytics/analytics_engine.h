/**
 * @file analytics_engine.h
 * @brief Main analytics query execution engine with connection pool management.
 * @version 1.0.0
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Phase 2 A-2: DB Connection Leak (10 gaps) — RAII guards + exception-safe cleanup
 *
 * Executes analytical queries with automatic connection lifecycle management
 * using RAII ConnectionGuard pattern. Prevents connection pool exhaustion and
 * resource leaks in normal and exceptional paths.
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
struct QueryResult;

struct QueryConfig {
    std::string query_text;              ///< SQL query to execute
    std::chrono::milliseconds timeout{5000};  ///< Query execution timeout
    int max_retries{3};                  ///< Max retries on connection failure
    bool auto_commit{true};              ///< Auto-commit results
};

struct QueryResult {
    bool success{false};                 ///< Execution succeeded
    std::string error_message;           ///< Error details if failed
    std::vector<std::vector<std::string>> rows;  ///< Result rows
    int64_t row_count{0};                ///< Number of rows returned
    int64_t affected_rows{0};            ///< Number of rows affected
    std::chrono::milliseconds duration{0};  ///< Query execution time
};

struct AggregationBatch {
    std::string query;                   ///< Aggregation query
    std::vector<std::string> group_keys; ///< Grouping columns
    std::vector<std::string> agg_columns;///< Aggregation columns
    int batch_size{1000};                ///< Batch processing size
};

class AnalyticsEngine {
public:
    /**
     * @brief Analytics Engine.
     * @param[in] pool Input parameter.
     * @return Return value.
     */
    explicit AnalyticsEngine(std::shared_ptr<ConnectionPool> pool);

    ~AnalyticsEngine() noexcept;


    /**
     * @brief Execute Query.
     * @param[in] config Input parameter.
     * @return Return value.
     */
    QueryResult ExecuteQuery(const QueryConfig& config);

    /**
     * @brief Run Aggregation.
     * @param[in] batch Input parameter.
     * @return Return value.
     */
    QueryResult RunAggregation(const AggregationBatch& batch);

    /**
     * @brief Process Batch.
     * @param[in] queries Input parameter.
     * @return Return value.
     */
    std::vector<QueryResult> ProcessBatch(const std::vector<QueryConfig>& queries);

    // ========================================================================
    // Configuration & Diagnostics (Gaps A-2-08 to A-2-10)
    // ========================================================================

    void SetQueryTimeout(std::chrono::milliseconds timeout) noexcept {
        query_timeout_ = timeout;
    }

    /**
     * @brief Set Pool Size.
     * @param[in] size Input parameter.
     */
    void SetPoolSize(int size);

    struct PoolStats {
        int available{0};
        int total{0};
        int peak_used{0};
    };
    /**
     * @brief Get Pool Stats.
     * @return Return value.
     * @note Exception safety: noexcept.
     */
    PoolStats GetPoolStats() const noexcept;

    /**
     * @brief Is Pool Exhausted.
     * @return True when the operation succeeds.
     * @note Exception safety: noexcept.
     */
    bool IsPoolExhausted() const noexcept;

    std::shared_ptr<ConnectionPool> GetPool() const noexcept {
        return pool_;
    }

private:
    // ========================================================================
    // Helper Methods
    // ========================================================================

    QueryResult ExecuteWithRetry(const QueryConfig& config, int retry_count = 0);

    void LogConnectionDiagnostics(const std::string& error, int connection_id = -1) const;

    /**
     * @brief Validate Pool State.
     */
    void ValidatePoolState() const;

    // ========================================================================
    // Member Variables
    // ========================================================================

    std::shared_ptr<ConnectionPool> pool_;      ///< Connection pool (never null)
    std::chrono::milliseconds query_timeout_{5000};  ///< Query timeout
    int max_retries_{3};                        ///< Max retry attempts
    int peak_pool_usage_{0};                    ///< Peak connection usage
};

}  // namespace analytics
}  // namespace themisdb
