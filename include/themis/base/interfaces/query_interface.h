/**
 * @file query_interface.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <memory>
#include <string>
#include <optional>
#include "utils/expected.h"
#include "utils/logger.h"
#include "query/request_context.h"

namespace themis {

/**
 * @brief Result of a query execution
 *
 * @deprecated Use Result<std::string> from utils/expected.h instead.
 *   Kept for backward compatibility during migration.
 *   All new code must use Result<std::string> from utils/expected.h.
 */
struct [[deprecated("Use Result<std::string> from utils/expected.h instead of QueryResult")]] QueryResult {
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
    [[nodiscard]] virtual bool evaluate(const std::string& expression, const void* context) const = 0;
    
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
     * @brief Execute a query with named bind variables.
     *
     * @param query          AQL query string.
     * @param bind_vars_json Named bind variables as a JSON object string
     *                       (e.g. `{"@col":"users","limit":10}`).
     *                       Pass `"{}"` or an empty string when there are no
     *                       bind variables.
     * @return JSON-encoded result string, or error.
     *
     * @note The default implementation calls the single-argument @c execute()
     *       overload and emits a warning when non-empty bind variables are
     *       supplied, because they cannot be forwarded.  Engine implementations
     *       that natively support bind variables should override this method.
     */
    [[nodiscard]] virtual Result<std::string> execute(
        const std::string& query,
        const std::string& bind_vars_json)
    {
        if (!bind_vars_json.empty() && bind_vars_json != "{}") {
            THEMIS_WARN("IQueryEngine::execute called with bind_vars but no "
                        "forwarding implementation; bind_vars ignored");
        }
        return execute(query);
    }
    
    /**
     * @brief Execute a query with bind variables, a request context and stop reporting.
     *
     * @param query          AQL query string.
     * @param bind_vars_json Named bind variables as JSON object string (see the
     *                       two-argument overload).
     * @param request        Deadline / cancellation context of the calling request.
     * @param[out] stop_reason Set to StopReason::None when the call completed
     *                       without observing a stop; otherwise the observed
     *                       reason (cancellation takes precedence over deadline).
     * @return Query output or error. When @p stop_reason is not None the caller
     *         MUST discard the returned value (success or error) and report the
     *         stop instead; no partial output may be surfaced.
     *
     * @warning The default implementation is checkpoint-only. It checks @p request
     *          before delegating to the two-argument @c execute() and again after it
     *          returns; a stop is observed only at those two points and a running
     *          call is NOT interrupted. If a stop is observed before the call,
     *          the engine is not invoked. If observed after, the (possibly
     *          successful) result is returned unchanged with @p stop_reason set
     *          and must be discarded by the caller. Engines that need cooperative
     *          in-flight cancellation or deadline enforcement MUST override this
     *          method and poll @c request.shouldStop() during execution.
     *          Engines that do not override it cannot be interrupted mid-call.
     *          Implementations must not retain @p request after this synchronous
     *          call returns; its cancellation probe may be borrowed from the caller.
     *
     * @note Source compatible: existing implementations need no change.
     */
    [[nodiscard]] virtual Result<std::string> execute(
        const std::string& query,
        const std::string& bind_vars_json,
        const query::RequestContext& request,
        query::StopReason& stop_reason)
    {
        stop_reason = request.stopReason();
        if (stop_reason != query::StopReason::None) {
            return Err<std::string>(
                stop_reason == query::StopReason::Cancelled
                    ? errors::ErrorCode::ERR_QUERY_CANCELLED
                    : errors::ErrorCode::ERR_QUERY_TIMEOUT,
                "query stopped before execution");
        }
        auto result = execute(query, bind_vars_json);
        stop_reason = request.stopReason();
        return result;
    }

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
