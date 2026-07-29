/**
 * @file iquery_limit_policy.h
 * @brief Abstract query-limit policy for GraphQL, AQL, and HTTP payloads in ThemisDB.
 *
 * Governs four query-safety axes: GraphQL nesting depth, GraphQL/AQL
 * complexity score, request payload size, and result-row ceiling.  Sits in
 * Tier 2 of the four-tier resource-governance chain:
 *
 * @code
 *   compile-time constexpr (edition.h)   ← absolute ceiling, never overridable
 *   RuntimeLicenseGate                   ← edition-tier ceiling
 *   IQueryLimitPolicy   (this file)      ← signed-plugin fine-tuning
 *   QueryLimitConfig                     ← per-deployment operational tuning
 * @endcode
 *
 * All implementations must be individually thread-safe.
 *
 * @note This interface is part of the edition-policy plugin contract
 *       (IEditionPolicyPlugin::createQueryLimitPolicy).  Claimed limits are
 *       validated against the compile-time ceilings in edition.h
 *       (QUERY_MAX_GRAPHQL_DEPTH, QUERY_MAX_GRAPHQL_COMPLEXITY,
 *        QUERY_MAX_PAYLOAD_BYTES, QUERY_MAX_RESULT_ROWS) before
 *       EditionManager accepts the policy.
 */

#pragma once

#include <cstdint>
#include <string>

namespace themis {
namespace query {

/**
 * @brief Abstract query-safety limit policy.
 *
 * Controls four axes that constrain potentially expensive or abusive queries:
 *  - **GraphQL/AQL depth** — maximum query nesting level.
 *  - **GraphQL/AQL complexity** — maximum computed complexity score.
 *  - **Payload size** — maximum HTTP request body in bytes.
 *  - **Result rows** — maximum rows returned by a single query execution.
 *
 * Implementations are installed into EditionManager via
 * `EditionManager::installQueryLimitPolicy()` and are consulted by both
 * the HTTP handler and the query executor.
 */
class IQueryLimitPolicy {
public:
    virtual ~IQueryLimitPolicy() = default;

    // Non-copyable, non-movable by default.
    IQueryLimitPolicy(const IQueryLimitPolicy&)            = delete;
    IQueryLimitPolicy& operator=(const IQueryLimitPolicy&) = delete;

    // -------------------------------------------------------------------------
    // Depth limit
    // -------------------------------------------------------------------------

    /**
     * @brief Return true iff a query with @p depth nesting levels is permitted.
     *
     * @param depth  Observed query AST depth (number of nested selection levels).
     * @return true when depth ≤ maxGraphQLDepth() or maxGraphQLDepth() == 0.
     */
    [[nodiscard]] virtual bool isDepthAllowed(uint32_t depth) const = 0;

    /**
     * @brief Maximum allowed query nesting depth; 0 = unlimited.
     */
    [[nodiscard]] virtual uint32_t maxGraphQLDepth() const noexcept = 0;

    // -------------------------------------------------------------------------
    // Complexity limit
    // -------------------------------------------------------------------------

    /**
     * @brief Return true iff a query with @p complexity score is permitted.
     *
     * @param complexity  Computed query complexity (resolver-weighted node count).
     * @return true when complexity ≤ maxGraphQLComplexity() or limit is 0.
     */
    [[nodiscard]] virtual bool isComplexityAllowed(uint32_t complexity) const = 0;

    /**
     * @brief Maximum allowed query complexity score; 0 = unlimited.
     */
    [[nodiscard]] virtual uint32_t maxGraphQLComplexity() const noexcept = 0;

    // -------------------------------------------------------------------------
    // Payload size limit
    // -------------------------------------------------------------------------

    /**
     * @brief Return true iff a request body of @p payload_bytes is permitted.
     *
     * @param payload_bytes  Size of the incoming request body in bytes.
     * @return true when payload_bytes ≤ maxPayloadBytes() or limit is 0.
     */
    [[nodiscard]] virtual bool isPayloadAllowed(uint64_t payload_bytes) const = 0;

    /**
     * @brief Maximum allowed request payload in bytes; 0 = unlimited.
     */
    [[nodiscard]] virtual uint64_t maxPayloadBytes() const noexcept = 0;

    // -------------------------------------------------------------------------
    // Result row limit
    // -------------------------------------------------------------------------

    /**
     * @brief Return true iff returning @p row_count rows is permitted.
     *
     * Should be checked at the result-serialisation exit to enforce
     * server-side row caps independent of client-supplied LIMIT clauses.
     *
     * @param row_count  Number of rows the executor intends to return.
     * @return true when row_count ≤ maxResultRows() or limit is 0.
     */
    [[nodiscard]] virtual bool isResultSizeAllowed(uint64_t row_count) const = 0;

    /**
     * @brief Maximum rows returned per query execution; 0 = unlimited.
     */
    [[nodiscard]] virtual uint64_t maxResultRows() const noexcept = 0;

    // -------------------------------------------------------------------------
    // Status
    // -------------------------------------------------------------------------

    /**
     * @brief Return true when any query-limit enforcement is active.
     *
     * Implementations should return false when all limits are 0 (unlimited)
     * so that callers may bypass the check on hot paths.
     */
    [[nodiscard]] virtual bool isEnforced() const noexcept = 0;

protected:
    IQueryLimitPolicy() = default;
    IQueryLimitPolicy(IQueryLimitPolicy&&) = default;
    IQueryLimitPolicy& operator=(IQueryLimitPolicy&&) = default;
};

} // namespace query
} // namespace themis
