/**
 * @file geo_policy.h
 * @brief Per-query spatial resource quotas and access-policy contract.
 *
 * `SpatialQueryPolicy` is a plain-old-data aggregate that callers attach to
 * spatial API calls to enforce per-query limits and to declare which
 * collections the issuing principal is permitted to query spatially.
 *
 * The policy is intentionally **not** a security mechanism by itself — it is
 * a resource-budget declaration consumed by the query execution layer.
 * The access-control check (`allowed_collections`) is advisory and must be
 * enforced at the server boundary; see `src/server/spatial_api_handler.cpp`.
 *
 * ### Design constraints
 * - POD-compatible (no virtual dispatch, no heap allocation).
 * - Zero-initialised default values are deliberately conservative.
 * - Validation via `validateSpatialQueryPolicy()` is cheap and allocation-free.
 *
 * @version 1.0.0
 * @note Geo API contract: `include/geo/geo_api_contract.h` v1.0.0 (frozen).
 */

#pragma once

#include <chrono>
#include <cstddef>
#include <string>
#include <vector>

namespace themis {
namespace geo {

// ---------------------------------------------------------------------------
// SpatialQueryPolicy
// ---------------------------------------------------------------------------

/**
 * @brief Per-query spatial resource quotas and collection access list.
 *
 * All fields have safe, conservative defaults that allow a query to complete
 * on typical datasets without triggering limits.  Operators may tighten these
 * limits to protect shared resources.
 *
 * ### Thread safety
 * `SpatialQueryPolicy` instances are value types.  Reading and writing from
 * multiple threads is safe as long as the usual data-race rules are followed.
 */
struct SpatialQueryPolicy {
    /**
     * @brief Maximum number of R-Tree candidates returned by the index layer
     *        before the refinement pass.
     *
     * Setting this to a smaller value limits worst-case memory allocation in
     * the candidate buffer.  A value of 0 means "no limit".
     *
     * Default: 100 000.
     */
    std::size_t max_candidates = 100'000;

    /**
     * @brief Maximum R-Tree traversal depth.
     *
     * Limits the recursion depth of tree descent when the underlying index
     * implementation supports bounded traversal.  A value of 0 means
     * "use the implementation default".
     *
     * Default: 64.
     */
    int max_depth = 64;

    /**
     * @brief Per-query wall-clock timeout.
     *
     * The query execution layer should abort a spatial query that exceeds this
     * duration and return a `GeoErrorCode::QUERY_TIMEOUT` error.
     * A value of zero disables timeout enforcement.
     *
     * Default: 5 000 ms.
     */
    std::chrono::milliseconds timeout{5'000};

    /**
     * @brief Maximum number of result pairs for spatial-join queries.
     *
     * Spatial-join operations may produce O(n²) pairs in the worst case.
     * This limit bounds the result set size and prevents runaway allocations.
     * A value of 0 means "no limit" (use with caution).
     *
     * Default: 1 000 000.
     */
    std::size_t max_result_pairs = 1'000'000;

    /**
     * @brief Explicit allowlist of collection names the query may access.
     *
     * When non-empty, the query execution layer MUST reject any attempt to
     * query a collection whose name is not present in this list.
     * An empty list means "all collections are permitted" (open policy).
     *
     * @note This is an advisory field — authoritative access control is
     *       enforced at the server boundary, not inside the geo module.
     */
    std::vector<std::string> allowed_collections;
};

// ---------------------------------------------------------------------------
// Policy validation
// ---------------------------------------------------------------------------

/**
 * @brief Validation result for a `SpatialQueryPolicy`.
 *
 * `ok()` returns true when the policy passes all range checks.  Individual
 * violations are described in `violations`.
 */
struct SpatialQueryPolicyValidation {
    /// True when no violations were found.
    [[nodiscard]] bool ok() const noexcept { return violations.empty(); }

    /// Human-readable description of each validation violation (may be empty).
    std::vector<std::string> violations;
};

/**
 * @brief Validate a `SpatialQueryPolicy` for internal consistency.
 *
 * Checks include:
 * - `max_depth` is non-negative.
 * - `timeout` is non-negative (zero means "disabled", which is valid).
 * - Individual `allowed_collections` entries are non-empty strings.
 *
 * @param policy  The policy to validate.
 * @return Validation result; call `ok()` to test for success.
 */
[[nodiscard]] SpatialQueryPolicyValidation
validateSpatialQueryPolicy(const SpatialQueryPolicy& policy);

/**
 * @brief Return the default spatial query policy.
 *
 * Equivalent to value-initialising a `SpatialQueryPolicy` but provided as a
 * named function for readability and future extensibility.
 *
 * @return A policy with all fields set to their documented defaults.
 */
[[nodiscard]] SpatialQueryPolicy defaultSpatialQueryPolicy() noexcept;

/**
 * @brief Check whether a given collection name is permitted by the policy.
 *
 * Returns `true` when `policy.allowed_collections` is empty (open policy) or
 * when @p collection_name appears in `policy.allowed_collections`.
 *
 * @param policy           The active policy.
 * @param collection_name  Name of the collection to check.
 * @return true if access is permitted, false otherwise.
 */
[[nodiscard]] bool isSpatialCollectionPermitted(const SpatialQueryPolicy& policy,
                                                const std::string& collection_name);

} // namespace geo
} // namespace themis
