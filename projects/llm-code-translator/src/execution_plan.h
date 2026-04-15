/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            execution_plan.h                                   ║
  Version:         0.0.44                                             ║
  Last Modified:   2026-04-15 05:39:52                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     137                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#pragma once

#include <string>
#include <vector>
#include <map>
#include <variant>
#include <nlohmann/json.hpp>

namespace themis {
namespace llm_translator {

/**
 * @brief Supported operation types for execution plans
 */
enum class OperationType {
    QUERY,           // Data retrieval with filters
    AGGREGATE,       // Aggregation operations (GROUP BY, COUNT, SUM, etc.)
    TRANSFORM,       // Data transformation
    JOIN,            // Multi-source data joining
    GRAPH_TRAVERSE,  // Graph traversal operations
    VECTOR_SEARCH,   // Similarity search with embeddings
    TIME_SERIES,     // Time-series analysis
    MUTATION         // Insert/Update/Delete operations
};

/**
 * @brief Value type for plan parameters (supports multiple types)
 */
using PlanValue = std::variant<
    std::string,
    int64_t,
    double,
    bool,
    std::vector<std::string>
>;

/**
 * @brief Filter condition for query operations
 */
struct FilterCondition {
    std::string field;
    std::string op;  // "=", ">", "<", ">=", "<=", "!=", "IN", "LIKE", etc.
    PlanValue value;
    
    nlohmann::json toJson() const;
    static FilterCondition fromJson(const nlohmann::json& j);
};

/**
 * @brief Aggregation specification
 */
struct Aggregation {
    std::string function;  // COUNT, SUM, AVG, MIN, MAX, etc.
    std::string field;
    std::string alias;
    
    nlohmann::json toJson() const;
    static Aggregation fromJson(const nlohmann::json& j);
};

/**
 * @brief Execution plan representing database operations
 * 
 * Platform-independent, JSON-serializable representation of data operations.
 * Can be executed directly, JIT-compiled, or converted to assembly/machine code.
 */
struct ExecutionPlan {
    OperationType operation;
    std::string datasource;
    std::vector<FilterCondition> filters;
    std::vector<std::string> fields;
    std::vector<std::string> group_by;
    std::vector<Aggregation> aggregations;
    std::map<std::string, PlanValue> parameters;
    
    // Metadata
    std::string original_prompt;
    double confidence_score = 0.0;
    std::string llm_model_used;
    int64_t generation_time_ms = 0;
    
    /**
     * @brief Serialize to JSON
     */
    nlohmann::json toJson() const;
    
    /**
     * @brief Deserialize from JSON
     */
    static ExecutionPlan fromJson(const nlohmann::json& j);
    
    /**
     * @brief Validate plan for correctness and security
     */
    bool validate(std::string* error_message = nullptr) const;
};

/**
 * @brief Result of executing an execution plan
 */
struct ExecutionResult {
    bool success = false;
    std::string error_message;
    nlohmann::json data;  // Result data
    int64_t execution_time_ms = 0;
    int64_t rows_affected = 0;
    
    // Performance metrics
    int64_t db_calls = 0;
    int64_t bytes_read = 0;
    int64_t cache_hits = 0;
};

} // namespace llm_translator
} // namespace themis
