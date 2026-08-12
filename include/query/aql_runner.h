/**
 * @file aql_runner.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <string>
#include <utility>
#include <nlohmann/json.hpp>
#include "query/aql_parser.h"
#include "query/aql_translator.h"
#include "query/result_type_annotation.h"
#include "query/query_resource_limits.h"
#include "query/query_canceller.h"
#include "query/sql_parser.h"
#include "query/mutation_executor.h"
#include "query_engine.h"
#include "utils/expected.h"

// Forward declarations for RLS
namespace themis {
namespace security {
    class RLSManager;
    struct SecurityContext;
} // namespace security
} // namespace themis

namespace themis {

using ConjunctiveQuery = ::themis::query::ConjunctiveQuery;
using RecursivePathQuery = ::themis::query::RecursivePathQuery;
using TraversalDirection = ::themis::query::TraversalDirection;

// High-level convenience dispatcher for AQL execution.
// Translates AQL to internal query forms and invokes the proper QueryEngine method.
// Returns Result<nlohmann::json> for unified error handling.
// GAP-002: Migrated from std::pair<Status, json> to Result<json>
Result<nlohmann::json> executeAql(const std::string& aql, query::QueryEngine& engine);

/// Execute a SQL query (SELECT / INSERT INTO / UPDATE … SET / DELETE FROM) via
/// the SQL dialect compatibility layer.
///
/// The SQL statement is first parsed by SQLParser into an AST, then transpiled
/// to AQL by SQLToAQLTranspiler, and finally executed through the same pipeline
/// as executeAql().  The returned JSON envelope has the same shape as executeAql().
///
/// Supported statements: SELECT, INSERT INTO, UPDATE … SET, DELETE FROM.
///
/// @param sql    The SQL statement to execute.
/// @param engine The QueryEngine instance to execute against.
/// @return       Result<nlohmann::json> on success, or an Err with
///               ERR_QUERY_PARSE_FAILED when the SQL cannot be parsed /
///               transpiled.
Result<nlohmann::json> executeSQL(const std::string& sql, query::QueryEngine& engine);

/// Execute AQL with per-query resource limits (max rows, max memory, timeout).
///
/// Enforces the supplied limits on the result of executeAql():
/// - @p limits.max_rows:         returns ERR_QUERY_RESOURCE_EXHAUSTED when the
///                               result contains more rows than the limit.
/// - @p limits.max_memory_bytes: returns ERR_QUERY_RESOURCE_EXHAUSTED when the
///                               serialised JSON result exceeds the byte budget.
/// - @p limits.timeout_ms:       returns ERR_QUERY_TIMEOUT when execution takes
///                               longer than the configured timeout.
///
/// A limit value of 0 means unlimited (the check is skipped).
Result<nlohmann::json> executeAqlWithLimits(
    const std::string& aql,
    query::QueryEngine& engine,
    const query::QueryResourceLimits& limits
);

// ── Query plan visualisation (EXPLAIN / EXPLAIN ANALYZE) ─────────────────────
//
// All three functions parse and translate the AQL query but stop before execution;
// they return the optimised query plan instead.  Non-conjunctive query forms
// (graph traversal, vector+geo, OR queries, …) fall back to a single SeqScan node
// that describes the query type.

/// Returns the execution plan as a JSON object (EXPLAIN format).
/// Set @p analyze=true to include an `actual_time_ms` / `actual_rows` skeleton
/// (populated with sentinel values −1 / 0 since no execution is performed here).
Result<nlohmann::json> explainAql(const std::string& aql, query::QueryEngine& engine,
                                  bool analyze = false);

/// Returns the execution plan as indented text (PostgreSQL-style EXPLAIN output).
Result<std::string> explainAqlText(const std::string& aql, query::QueryEngine& engine,
                                   bool analyze = false);

/// Returns the execution plan as a Graphviz DOT digraph string.
/// The output can be piped to `dot -Tpng -o plan.png` for a visual diagram.
Result<std::string> explainAqlDot(const std::string& aql, query::QueryEngine& engine);

/// Execute a multi-statement AQL transaction block.
///
/// The @p aql string must have the form:
///   BEGIN [;]
///     <AQL statement 1> [;]
///     <AQL statement 2> [;]
///     ...
///   COMMIT | ROLLBACK [;]
///
/// If the block ends with COMMIT, every statement is executed in order and the
/// combined results are returned as a JSON array (one entry per statement).
/// If the block ends with ROLLBACK, no statement is executed and a JSON object
/// @c {"type":"rollback","statements":N} is returned.
///
/// On parse or execution failure the function returns an Err.
///
/// @param aql     Multi-statement AQL block string.
/// @param engine  QueryEngine used to execute read queries.
Result<nlohmann::json> executeMultiStatementAql(const std::string& aql, query::QueryEngine& engine);

/// Execute a multi-statement AQL transaction block with DML mutation support.
///
/// Phase 4 overload: identical to the two-argument form but also supports
/// INSERT/UPDATE/DELETE/REMOVE/REPLACE/UPSERT statements inside the block.
/// Mutations are executed atomically — if any statement fails, all mutations
/// that have already been applied are rolled back via the undo log maintained
/// in @p storage.
///
/// @param aql      Multi-statement AQL block string.
/// @param engine   QueryEngine used to execute read queries.
/// @param storage  StorageContext used to execute DML mutations.  Mutations
///                 in the block are silently skipped when @p storage is null.
Result<nlohmann::json> executeMultiStatementAql(const std::string&                            aql,
                                                 query::QueryEngine&                            engine,
                                                 query::MutationExecutor::StorageContext*       storage);

// ── Row-level security (RLS) wrappers ────────────────────────────────────────
//
// These functions execute AQL normally and then apply row-level security
// policies from @p rls to filter the result rows based on @p ctx.
//
// If no policies match the queried collection and security context, the result
// is returned unchanged (no filtering overhead).

/// Execute AQL with row-level security filtering applied to the result.
/// Only rows that satisfy the applicable RLS policies for the user described
/// by @p ctx are included in the returned result set.
Result<nlohmann::json> executeAqlWithRLS(
    const std::string& aql,
    query::QueryEngine& engine,
    security::RLSManager& rls,
    const security::SecurityContext& ctx
);

// ── Type-annotated execution (for client SDK code generation) ─────────────

/// Execute AQL and return the result together with its inferred type schema.
///
/// The schema is derived from the "results" array inside the JSON payload.
/// It can be serialised with AnnotatedQueryResult::schema.toJson() and fed
/// to client SDK generators to produce strongly-typed accessor code.
///
/// On execution failure the function returns an Err identical to executeAql().
Result<query::AnnotatedQueryResult> executeAqlAnnotated(
    const std::string& aql,
    query::QueryEngine& engine
);

/// Execute AQL with cooperative cancellation support.
///
/// Registers the query under @p request_id in the process-wide
/// QueryCanceller registry before execution.  The registration is
/// automatically cleaned up (unregistered) when this function returns.
///
/// Cancellation is checked once before execution begins.  Any thread that
/// calls QueryCanceller::instance().cancel(@p request_id) while the query
/// is running will cause the next cooperative checkpoint to return
/// ERR_QUERY_CANCELLED.
///
/// The @p canceller parameter exists for testing; production callers should
/// omit it to use the process-wide singleton.
///
/// @param aql          The AQL query string to execute.
/// @param engine       The QueryEngine instance to execute against.
/// @param request_id   Caller-assigned unique identifier for this query.
/// @param canceller    Optional: override the canceller registry (for tests).
/// @return             Result<nlohmann::json> on success, or an Err with
///                     ERR_QUERY_CANCELLED when the query was cancelled.
Result<nlohmann::json> executeAqlCancellable(
    const std::string& aql,
    query::QueryEngine& engine,
    const std::string& request_id,
    query::QueryCanceller& canceller = query::QueryCanceller::instance()
);

} // namespace themis
