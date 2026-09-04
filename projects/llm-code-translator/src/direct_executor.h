/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            direct_executor.h                                  ║
  Version:         0.0.47                                             ║
  Last Modified:   2026-04-15 18:48:12                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   92.0/100                                       ║
    • Total Lines:     173                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • d275653619  2026-04-14  update after codefindings               ║
    • a2d7c07202  2026-04-14  update after codefindings               ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#pragma once

#include "execution_plan.h"
#include <map>
#include <memory>
#include <string>
#include <vector>
#include <functional>

namespace themis {
namespace llm_translator {

// Forward declaration for database interface
class DatabaseInterface;

// Direct executor - interprets and executes execution plans
class DirectExecutor {
public:
    explicit DirectExecutor(std::shared_ptr<DatabaseInterface> db);
    ~DirectExecutor();
    
    // Execute a single plan
    ExecutionResult execute(const ExecutionPlan& plan);
    
    // Resource limits for execution
    struct ResourceLimits {
        int64_t max_execution_time_ms = 30000;  // 30 seconds
        size_t max_memory_bytes = 1024 * 1024 * 1024;  // 1GB
        size_t max_result_rows = 1000000;  // 1M rows
    };
    
    // Execute with resource limits
    ExecutionResult execute(const ExecutionPlan& plan, const ResourceLimits& limits);
    
    // Enable/disable execution metrics
    void enableMetrics(bool enable);
    
    // Get execution statistics
    struct ExecutionStats {
        size_t total_executions = 0;
        size_t successful_executions;
        size_t failed_executions;
        int64_t total_execution_time_ms;
        int64_t avg_execution_time_ms;
    };
    
    ExecutionStats getStats() const;
    void resetStats();

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
    
    // Execution methods for each operation type
    ExecutionResult executeQuery(const ExecutionPlan& plan, const ResourceLimits& limits);
    ExecutionResult executeAggregate(const ExecutionPlan& plan, const ResourceLimits& limits);
    ExecutionResult executeTransform(const ExecutionPlan& plan, const ResourceLimits& limits);
    ExecutionResult executeJoin(const ExecutionPlan& plan, const ResourceLimits& limits);
    ExecutionResult executeGraphTraverse(const ExecutionPlan& plan, const ResourceLimits& limits);
    ExecutionResult executeVectorSearch(const ExecutionPlan& plan, const ResourceLimits& limits);
    ExecutionResult executeTimeSeries(const ExecutionPlan& plan, const ResourceLimits& limits);
    ExecutionResult executeMutation(const ExecutionPlan& plan, const ResourceLimits& limits);
    
    // Helper methods
    bool checkResourceLimits(const ResourceLimits& limits, int64_t elapsed_ms, size_t memory_used);
    nlohmann::json applyFilters(const nlohmann::json& data, const std::vector<FilterCondition>& filters);
    nlohmann::json applyPagination(const nlohmann::json& data, int limit, int offset);
};

// Database interface - abstraction for ThemisDB operations
class DatabaseInterface {
public:
    virtual ~DatabaseInterface() = default;
    
    // Basic CRUD operations
    virtual nlohmann::json scan(const std::string& datasource) = 0;
    virtual nlohmann::json get(const std::string& datasource, const nlohmann::json& key) = 0;
    virtual bool put(const std::string& datasource, const nlohmann::json& key, const nlohmann::json& value) = 0;
    virtual bool del(const std::string& datasource, const nlohmann::json& key) = 0;
    
    // Batch operations
    virtual std::vector<nlohmann::json> multiGet(const std::string& datasource, 
                                                   const std::vector<nlohmann::json>& keys) = 0;
    
    // Index operations
    virtual nlohmann::json indexScan(const std::string& datasource, 
                                      const std::string& index_name,
                                      const nlohmann::json& start_key,
                                      const nlohmann::json& end_key) = 0;
    
    // Graph operations
    virtual nlohmann::json graphTraverse(const std::string& datasource,
                                         const nlohmann::json& start_node,
                                         const std::string& traversal_type,
                                         int max_depth) = 0;
    
    // Vector operations
    virtual nlohmann::json vectorSearch(const std::string& datasource,
                                        const std::vector<float>& query_vector,
                                        int top_k,
                                        const std::string& distance_metric) = 0;
    
    // Time series operations
    virtual nlohmann::json timeSeriesQuery(const std::string& datasource,
                                           const std::string& start_time,
                                           const std::string& end_time,
                                           const std::string& aggregation) = 0;
};

// STUB/SIMULATION NOTE:
// Purpose: Provide deterministic in-memory database behavior for translator development and tests.
// Activation: Used when callers instantiate MockDatabase instead of a production database adapter.
// Production Delta: Data is process-local and in-memory only; no external storage engine or network calls.
// Removal Plan: Keep for tests; do not use in production execution paths once full adapters are wired.
class MockDatabase : public DatabaseInterface {
public:
    MockDatabase();
    ~MockDatabase() override = default;
    
    nlohmann::json scan(const std::string& datasource) override;
    nlohmann::json get(const std::string& datasource, const nlohmann::json& key) override;
    bool put(const std::string& datasource, const nlohmann::json& key, const nlohmann::json& value) override;
    bool del(const std::string& datasource, const nlohmann::json& key) override;
    std::vector<nlohmann::json> multiGet(const std::string& datasource, 
                                          const std::vector<nlohmann::json>& keys) override;
    nlohmann::json indexScan(const std::string& datasource, 
                             const std::string& index_name,
                             const nlohmann::json& start_key,
                             const nlohmann::json& end_key) override;
    nlohmann::json graphTraverse(const std::string& datasource,
                                 const nlohmann::json& start_node,
                                 const std::string& traversal_type,
                                 int max_depth) override;
    nlohmann::json vectorSearch(const std::string& datasource,
                               const std::vector<float>& query_vector,
                               int top_k,
                               const std::string& distance_metric) override;
    nlohmann::json timeSeriesQuery(const std::string& datasource,
                                   const std::string& start_time,
                                   const std::string& end_time,
                                   const std::string& aggregation) override;
    
    // For testing - load sample data
    void loadSampleData();

private:
    std::map<std::string, std::vector<nlohmann::json>> data_;
};

} // namespace llm_translator
} // namespace themis
