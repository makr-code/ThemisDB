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

/**
 * @brief Query configuration and execution parameters.
 */
struct QueryConfig {
    std::string query_text;              ///< SQL query to execute
    std::chrono::milliseconds timeout{5000};  ///< Query execution timeout
    int max_retries{3};                  ///< Max retries on connection failure
    bool auto_commit{true};              ///< Auto-commit results
};

/**
 * @brief Query execution result with metadata.
 */
struct QueryResult {
    bool success{false};                 ///< Execution succeeded
    std::string error_message;           ///< Error details if failed
    std::vector<std::vector<std::string>> rows;  ///< Result rows
    int64_t row_count{0};                ///< Number of rows returned
    int64_t affected_rows{0};            ///< Number of rows affected
    std::chrono::milliseconds duration{0};  ///< Query execution time
};

/**
 * @brief Aggregation batch configuration.
 */
struct AggregationBatch {
    std::string query;                   ///< Aggregation query
    std::vector<std::string> group_keys; ///< Grouping columns
    std::vector<std::string> agg_columns;///< Aggregation columns
    int batch_size{1000};                ///< Batch processing size
};

/**
 * @brief Main analytics query execution engine.
 *
 * Thread-safe query executor with connection pool management. All database
 * connections are managed via RAII ConnectionGuard to ensure automatic cleanup
 * on scope exit, including exception paths.
 *
 * Phase 2 A-2 Gaps Addressed:
 * - A-2-01: Connection release on normal exit
 * - A-2-02: Connection release on exception
 * - A-2-03: Exception-safe query execution
 * - A-2-04: Null-check guards before pool access
 * - A-2-05: Error logging with connection diagnostics
 * - A-2-06: Retry logic with fresh connection
 * - A-2-07: Connection lifetime documentation
 * - A-2-08: Timeout configuration
 * - A-2-09: Pool exhaustion fallback
 * - A-2-10: Query cancellation safety
 */
class AnalyticsEngine {
public:
    /**
     * Construct engine with connection pool.
     *
     * @param pool Shared connection pool (must outlive engine)
     * @throws std::invalid_argument if pool is null
     */
    explicit AnalyticsEngine(std::shared_ptr<ConnectionPool> pool);

    /**
     * Destructor: cleanup and pool release.
     */
    ~AnalyticsEngine() noexcept;

    // ========================================================================
    // Core Query Execution (Gaps A-2-01 to A-2-05)
    // ========================================================================

    /**
     * Execute a single query with automatic connection management.
     *
     * **RAII Pattern:** Connection acquired, then released on scope exit.
     * **Exception Safety:** Strong exception guarantee.
     *
     * Phase 2 A-2-01: Connection released on normal exit
     * Phase 2 A-2-02: Connection released on exception
     * Phase 2 A-2-03: Exception-safe query execution
     *
     * @param config Query configuration
     * @return Query result (success field indicates outcome)
     *
     * @throws std::runtime_error if connection pool exhausted after retries
     */
    QueryResult ExecuteQuery(const QueryConfig& config);

    /**
     * Execute batch aggregation query.
     *
     * Phase 2 A-2-04: Null-check guards before pool access
     * Phase 2 A-2-05: Error logging with connection diagnostics
     *
     * @param batch Aggregation batch configuration
     * @return Query result with aggregated data
     *
     * @throws std::invalid_argument if batch is malformed
     */
    QueryResult RunAggregation(const AggregationBatch& batch);

    /**
     * Process query batch with automatic retry and connection recovery.
     *
     * Phase 2 A-2-06: Retry logic with fresh connection
     * Phase 2 A-2-07: Connection lifetime documentation
     *
     * @param queries Vector of query configurations
     * @return Vector of results, one per input query
     */
    std::vector<QueryResult> ProcessBatch(const std::vector<QueryConfig>& queries);

    // ========================================================================
    // Configuration & Diagnostics (Gaps A-2-08 to A-2-10)
    // ========================================================================

    /**
     * Set query execution timeout.
     *
     * Phase 2 A-2-08: Timeout configuration for connection wait
     *
     * @param timeout Timeout duration in milliseconds
     */
    void SetQueryTimeout(std::chrono::milliseconds timeout) noexcept {
        query_timeout_ = timeout;
    }

    /**
     * Set connection pool size.
     *
     * @param size Number of connections in pool
     * @throws std::invalid_argument if size <= 0
     */
    void SetPoolSize(int size);

    /**
     * Get connection pool health statistics.
     *
     * Phase 2 A-2-09: Pool exhaustion fallback strategy
     *
     * @return {available_connections, total_connections, peak_used}
     */
    struct PoolStats {
        int available{0};
        int total{0};
        int peak_used{0};
    };
    PoolStats GetPoolStats() const noexcept;

    /**
     * Check if pool is exhausted.
     *
     * @return true if no connections available
     */
    bool IsPoolExhausted() const noexcept;

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
    // Helper Methods
    // ========================================================================

    /**
     * Execute query with retry logic and exception handling.
     *
     * Phase 2 A-2-10: Query cancellation safety
     *
     * @param config Query configuration
     * @param retry_count Current retry number
     * @return Query result
     */
    QueryResult ExecuteWithRetry(const QueryConfig& config, int retry_count = 0);

    /**
     * Log connection diagnostics on failure.
     *
     * @param error Error message
     * @param connection_id Connection ID (or -1 if not acquired)
     */
    void LogConnectionDiagnostics(const std::string& error, int connection_id = -1) const;

    /**
     * Validate connection pool state.
     *
     * @throws std::runtime_error if pool is unavailable
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
