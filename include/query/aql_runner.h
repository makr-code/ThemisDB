/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            aql_runner.h                                       ║
  Version:         0.0.32                                             ║
  Last Modified:   2026-02-23 03:57:29                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     62                                             ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 8ece79254  2026-02-21  feat(query): wire QueryPlanVisualizer into AQL pipeline v... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#pragma once

#include <string>
#include <utility>
#include <nlohmann/json.hpp>
#include "query/aql_parser.h"
#include "query/aql_translator.h"
#include "query/result_type_annotation.h"
#include "query/query_resource_limits.h"
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

// High-level convenience dispatcher for AQL execution.
// Translates AQL to internal query forms and invokes the proper QueryEngine method.
// Returns Result<nlohmann::json> for unified error handling.
// GAP-002: Migrated from std::pair<Status, json> to Result<json>
Result<nlohmann::json> executeAql(const std::string& aql, QueryEngine& engine);

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
    QueryEngine& engine,
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
Result<nlohmann::json> explainAql(const std::string& aql, QueryEngine& engine,
                                  bool analyze = false);

/// Returns the execution plan as indented text (PostgreSQL-style EXPLAIN output).
Result<std::string> explainAqlText(const std::string& aql, QueryEngine& engine,
                                   bool analyze = false);

/// Returns the execution plan as a Graphviz DOT digraph string.
/// The output can be piped to `dot -Tpng -o plan.png` for a visual diagram.
Result<std::string> explainAqlDot(const std::string& aql, QueryEngine& engine);

/// Execute a multi-statement AQL transaction block.
///
/// The @p aql string must have the form:
///   BEGIN
///     <AQL statement 1>
///     <AQL statement 2>
///     ...
///   COMMIT | ROLLBACK
///
/// If the block ends with COMMIT, every statement is executed in order and the
/// combined results are returned as a JSON array (one entry per statement).
/// If the block ends with ROLLBACK, no statement is executed and a JSON object
/// @c {"type":"rollback","statements":N} is returned.
///
/// On parse or execution failure the function returns an Err.
Result<nlohmann::json> executeMultiStatementAql(const std::string& aql, QueryEngine& engine);

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
    QueryEngine& engine,
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
    QueryEngine& engine
);

} // namespace themis
