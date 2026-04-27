/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            query_interface.h                                  ║
  Version:         0.0.47                                             ║
  Last Modified:   2026-04-15 18:47:22                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     138                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#pragma once

#include <memory>
#include <string>
#include <optional>
#include "utils/expected.h"

namespace themis {

/**
 * @brief Result of a query execution
 *
 * @deprecated Use Result<T> pattern instead.
 *   Kept for backward compatibility during migration.
 *   All new code must use Result<std::string> from utils/expected.h.
 */
struct [[deprecated("Use Result<T> (utils/expected.h) instead of QueryResult")]] QueryResult {
    bool success = false;
    std::optional<std::string> error_message;
    
    [[nodiscard]] bool hasError() const { return error_message.has_value(); }
};

/**
 * @brief Interface for expression evaluation
 * 
 * Provides abstract interface for evaluating filter expressions
 * and WHERE clauses without depending on concrete query engine implementations.
 */
class IExpressionEvaluator {
public:
    virtual ~IExpressionEvaluator() = default;
    
    /**
     * @brief Evaluate an expression against provided context
     * 
     * @param expression The filter expression to evaluate
     * @param context Opaque context pointer (e.g., document, row data)
     * @return true if expression evaluates to true, false otherwise
     */
    [[nodiscard]] virtual bool evaluate(const std::string& expression, const void* context) = 0;
    
    /**
     * @brief Get the type of expression language supported
     * 
     * @return String identifying the expression language (e.g., "AQL", "SQL")
     */
    [[nodiscard]] virtual std::string get_expression_type() const = 0;
};

/// Shared pointer type for IExpressionEvaluator
using IExpressionEvaluatorPtr = std::shared_ptr<IExpressionEvaluator>;

/**
 * @brief Interface for query engine
 * 
 * Provides abstract interface for query parsing, validation, and execution
 * without depending on concrete query engine implementations.
 */
class IQueryEngine {
public:
    virtual ~IQueryEngine() = default;
    
    /**
     * @brief Execute a query
     * 
     * @param query Query string
     * @return Result<std::string> with query output or error details
     * 
     * @note Legacy signature returning QueryResult is deprecated.
     *       Implementations should migrate to Result<std::string>.
     */
    [[nodiscard]] virtual Result<std::string> execute(const std::string& query) = 0;
    
    /**
     * @brief Validate a query without executing it
     * 
     * @param query Query string
     * @return Result<void> - success if valid, error with details otherwise
     */
    [[nodiscard]] virtual Result<void> validate(const std::string& query) const = 0;
    
    /**
     * @brief Create an expression evaluator
     * 
     * @return Result<std::unique_ptr<IExpressionEvaluator>> with evaluator or error
     */
    [[nodiscard]] virtual Result<std::unique_ptr<IExpressionEvaluator>> createExpressionEvaluator() const = 0;
    
    /**
     * @brief Generate execution plan explanation
     * 
     * @param query Query string
     * @return Result<std::string> with execution plan or error
     */
    [[nodiscard]] virtual Result<std::string> explainQuery(const std::string& query) const = 0;
};

/// Shared pointer type for IQueryEngine
using IQueryEnginePtr = std::shared_ptr<IQueryEngine>;

/**
 * @brief Factory interface for query engines
 */
class IQueryEngineFactory {
public:
    virtual ~IQueryEngineFactory() = default;
    
    /**
     * @brief Create a query engine instance
     * 
     * @return Shared pointer to query engine
     */
    [[nodiscard]] virtual IQueryEnginePtr create() = 0;
};

} // namespace themis
