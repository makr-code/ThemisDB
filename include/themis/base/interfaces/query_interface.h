/// @file query_interface.h
/// @brief Abstract interface for query execution and expression evaluation
/// 
/// This interface defines the contract for query engines in ThemisDB.
/// It enables dependency inversion by allowing components (especially indexes)
/// to evaluate expressions without depending on concrete query implementations.
/// 
/// Design Goals:
/// - Break circular dependencies between Query ↔ Storage ↔ Index
/// - Enable index filters to evaluate WHERE clauses independently
/// - Support isolated unit testing with mock implementations
/// - Allow multiple query execution strategies
/// 
/// @note This is a Phase 1 interface definition. Implementations will be
///       refactored in subsequent phases to use this interface.

#pragma once

#include "themis/base/export.h"
#include <any>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <variant>
#include <vector>

namespace themis {

/// @brief Represents a value that can be of different types
using QueryValue = std::variant<
    std::monostate,           // null
    bool,                      // boolean
    int64_t,                   // integer
    double,                    // floating point
    std::string,               // string
    std::vector<float>,        // vector (for embeddings)
    std::vector<uint8_t>       // binary blob
>;

/// @brief Row/document data as field name -> value mapping
using RowData = std::unordered_map<std::string, QueryValue>;

/// @brief Abstract interface for expression evaluation
/// 
/// Enables components (like indexes) to evaluate filter expressions
/// without depending on the full query engine implementation.
/// 
/// Example use case:
/// - Index needs to filter results based on WHERE clause
/// - Index receives IExpressionEvaluator from query engine
/// - Index calls evaluate() for each candidate row
/// - No circular dependency on query implementation
class THEMIS_BASE_API IExpressionEvaluator {
public:
    virtual ~IExpressionEvaluator() = default;

    /// @brief Evaluate an expression against row data
    /// 
    /// @param expression The expression to evaluate (implementation-specific format)
    /// @param row_data The row data to evaluate against
    /// @return Evaluation result, or std::nullopt if expression is invalid
    virtual std::optional<QueryValue> evaluate(
        std::string_view expression,
        const RowData& row_data) const = 0;

    /// @brief Evaluate a boolean expression (for WHERE clauses)
    /// 
    /// Convenience method that evaluates an expression and returns
    /// a boolean result. Non-boolean results are coerced to boolean
    /// using standard truthiness rules.
    /// 
    /// @param expression The expression to evaluate
    /// @param row_data The row data to evaluate against
    /// @return true if expression evaluates to true, false otherwise
    virtual bool evaluateBoolean(
        std::string_view expression,
        const RowData& row_data) const = 0;

    /// @brief Check if this evaluator can handle a given expression
    /// 
    /// @param expression The expression to check
    /// @return true if the expression is valid and can be evaluated
    virtual bool canEvaluate(std::string_view expression) const = 0;
};

/// @brief Query result set
/// 
/// Represents the result of a query execution.
/// Results are returned as a vector of rows, where each row
/// is a map of field names to values.
struct QueryResult {
    std::vector<RowData> rows;
    uint64_t total_count = 0;  // Total matching rows (may be > rows.size() if limited)
    std::string error_message;  // Error message if query failed
    bool success = true;        // Whether query executed successfully

    bool hasError() const { return !success; }
    bool isEmpty() const { return rows.empty(); }
    size_t size() const { return rows.size(); }
};

/// @brief Abstract interface for query execution
/// 
/// Defines the contract for executing queries against the database.
/// This interface is intentionally minimal and focused on execution,
/// not parsing or optimization.
/// 
/// Dependencies are injected:
/// - Storage engine is provided at construction or via setter
/// - Index manager is provided at construction or via setter
/// 
/// This interface intentionally has NO circular dependencies.
class THEMIS_BASE_API IQueryEngine {
public:
    virtual ~IQueryEngine() = default;

    /// @brief Execute a query string
    /// 
    /// @param query The query to execute (format depends on implementation)
    /// @return Query result set
    virtual QueryResult execute(std::string_view query) = 0;

    /// @brief Parse a query without executing it
    /// 
    /// Useful for query validation and plan generation.
    /// 
    /// @param query The query to parse
    /// @return true if query is syntactically valid, false otherwise
    virtual bool validate(std::string_view query) const = 0;

    /// @brief Create an expression evaluator for filter expressions
    /// 
    /// Returns an evaluator that can be used by other components
    /// (like indexes) to evaluate expressions independently.
    /// 
    /// @return Expression evaluator instance
    virtual std::unique_ptr<IExpressionEvaluator> createExpressionEvaluator() const = 0;

    /// @brief Get query execution plan (for debugging/optimization)
    /// 
    /// @param query The query to analyze
    /// @return Human-readable execution plan, or empty string if unavailable
    virtual std::string explainQuery(std::string_view query) const = 0;
};

/// @brief Query execution context
/// 
/// Provides runtime context for query execution, including:
/// - User/session information
/// - Transaction context
/// - Performance hints
struct QueryContext {
    std::string user_id;
    std::string session_id;
    std::any transaction_handle;  // Opaque transaction handle from storage
    uint64_t timeout_ms = 0;      // Query timeout (0 = no timeout)
    bool enable_cache = true;     // Whether to use query cache
    bool read_only = false;       // Whether this is a read-only query

    QueryContext() = default;
};

/// @brief Factory interface for creating query engines
/// 
/// Enables dependency injection of query engine implementations
class THEMIS_BASE_API IQueryEngineFactory {
public:
    virtual ~IQueryEngineFactory() = default;

    /// @brief Create a new query engine instance
    /// @param config Configuration string (implementation-specific)
    /// @return Query engine instance, or nullptr on failure
    virtual std::unique_ptr<IQueryEngine> createQueryEngine(
        const std::string& config) = 0;
};

} // namespace themis
