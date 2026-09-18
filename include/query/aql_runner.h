/**
 * @file aql_runner.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
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

/**
 * @brief High-level convenience dispatcher for AQL execution.
 * @param[in] aql Input parameter.
 * @param[in,out] engine Input/output parameter.
 * @return Return value.
 * @details Translates AQL to internal query forms and invokes the proper QueryEngine method. Returns Result<nlohmann::json> for unified error handling. GAP-002: Migrated from std::pair<Status, json> to Result<json>
 */
Result<nlohmann::json> executeAql(const std::string& aql, query::QueryEngine& engine);

/**
 * @brief Execute SQL.
 * @param[in] sql Input parameter.
 * @param[in,out] engine Input/output parameter.
 * @return Return value.
 */
Result<nlohmann::json> executeSQL(const std::string& sql, query::QueryEngine& engine);

/**
 * @brief Execute Aql With Limits.
 * @param[in] aql Input parameter.
 * @param[in,out] engine Input/output parameter.
 * @param[in] limits Input parameter.
 * @return Return value.
 */
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

Result<nlohmann::json> explainAql(const std::string& aql, query::QueryEngine& engine,
                                  bool analyze = false);

Result<std::string> explainAqlText(const std::string& aql, query::QueryEngine& engine,
                                   bool analyze = false);

/**
 * @brief Explain Aql Dot.
 * @param[in] aql Input parameter.
 * @param[in,out] engine Input/output parameter.
 * @return Return value.
 */
Result<std::string> explainAqlDot(const std::string& aql, query::QueryEngine& engine);

/**
 * @brief Execute Multi Statement Aql.
 * @param[in] aql Input parameter.
 * @param[in,out] engine Input/output parameter.
 * @return Return value.
 */
Result<nlohmann::json> executeMultiStatementAql(const std::string& aql, query::QueryEngine& engine);

/**
 * @brief Execute Multi Statement Aql.
 * @param[in] aql Input parameter.
 * @param[in,out] engine Input/output parameter.
 * @param[in,out] storage Input/output parameter.
 * @return Return value.
 */
Result<nlohmann::json> executeMultiStatementAql(const std::string&                            aql,
                                                 query::QueryEngine&                            engine,
                                                 query::MutationExecutor::StorageContext*       storage);

/**
 * @brief ── Row-level security (RLS) wrappers ──────────────────────────────────────── These functions execute AQL normally and then apply row-level security policies from @p rls to filter the result rows based on @p ctx.
 * @param[in] aql Input parameter.
 * @param[in,out] engine Input/output parameter.
 * @param[in,out] rls Input/output parameter.
 * @param[in] ctx Input parameter.
 * @return Return value.
 * @details If no policies match the queried collection and security context, the result is returned unchanged (no filtering overhead).
 */

Result<nlohmann::json> executeAqlWithRLS(
    const std::string& aql,
    query::QueryEngine& engine,
    security::RLSManager& rls,
    const security::SecurityContext& ctx
);

/**
 * @brief ── Type-annotated execution (for client SDK code generation) ─────────────
 * @param[in] aql Input parameter.
 * @param[in,out] engine Input/output parameter.
 * @return Return value.
 */

Result<query::AnnotatedQueryResult> executeAqlAnnotated(
    const std::string& aql,
    query::QueryEngine& engine
);

Result<nlohmann::json> executeAqlCancellable(
    const std::string& aql,
    query::QueryEngine& engine,
    const std::string& request_id,
    query::QueryCanceller& canceller = query::QueryCanceller::instance()
);

} // namespace themis
