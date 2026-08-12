/**
 * @file row_level_security.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <string>
#include <vector>
#include <mutex>
#include <unordered_map>
#include <optional>
#include <nlohmann/json.hpp>

namespace themis {
namespace security {

// Forward declaration
struct SecurityContext;

/**
 * @brief Row-level security (RLS) policy predicate.
 *
 * Describes a condition evaluated against a single JSON row.
 * Two predicate styles are supported:
 *
 *  1. Static value:  field `op` value
 *     e.g. {"field":"tenant_id","op":"eq","value":"acme"}
 *
 *  2. User attribute: field `op` SecurityContext member
 *     e.g. {"field":"owner","op":"eq","user_attr":"user_id"}
 *     Supported user_attr keys: "user_id", or any key from
 *     SecurityContext::attributes.
 *
 * Supported operators: "eq", "ne", "lt", "le", "gt", "ge", "in"
 * For "in", value must be a JSON array.
 */
struct RLSPredicate {
    std::string field;        ///< Row field name to test
    std::string op;           ///< Comparison operator
    std::string value;        ///< Static literal value (JSON-encoded scalar/array)
    std::string user_attr;    ///< SecurityContext attribute key (mutually exclusive with value)

    /// Evaluate predicate against a JSON row and security context.
    /// @return true if the row satisfies the predicate.
    bool evaluate(const nlohmann::json& row, const SecurityContext& ctx) const;

    nlohmann::json toJson() const;
    static RLSPredicate fromJson(const nlohmann::json& j);
};

/**
 * @brief RLS policy type, following the PostgreSQL model.
 *
 * PERMISSIVE  – multiple permissive policies are OR'd; a row is visible if
 *               at least one applicable permissive policy allows it.
 * RESTRICTIVE – multiple restrictive policies are AND'd; a row is visible only
 *               if every applicable restrictive policy allows it.
 *
 * When both types exist for a collection, a row must satisfy (any permissive)
 * AND (all restrictive) conditions.
 */
enum class RLSPolicyType {
    PERMISSIVE,
    RESTRICTIVE
};

/**
 * @brief A named row-level security policy applied to a specific collection.
 *
 * Policies are matched to a request by:
 *  - collection name (empty string = applies to ALL collections)
 *  - applicable_roles (empty = applies to ALL users, including those with no role)
 *
 * The predicate is evaluated for each result row.  Rows that do not satisfy
 * the predicate are silently excluded from the result set.
 */
struct RLSPolicy {
    std::string id;                          ///< Unique policy identifier
    std::string collection;                  ///< Target collection ("" = all)
    std::vector<std::string> applicable_roles; ///< Matching roles (empty = all users)
    RLSPredicate predicate;                  ///< Row filter condition
    RLSPolicyType type = RLSPolicyType::PERMISSIVE;
    bool enabled = true;

    nlohmann::json toJson() const;
    static RLSPolicy fromJson(const nlohmann::json& j);
};

/**
 * @brief Row-level security manager.
 *
 * Central registry for RLS policies.  Call filterRows() to remove rows that
 * the current user is not allowed to see.
 *
 * Thread-safety: all public methods are protected by an internal mutex.
 *
 * Usage:
 * @code
 * RLSManager rls;
 * rls.addPolicy({
 *     "tenant_isolation", "orders", {"analyst"},
 *     {"tenant_id", "eq", "", "user_id"},
 *     RLSPolicyType::PERMISSIVE
 * });
 *
 * // After executing a query:
 * nlohmann::json filtered = rls.filterRows("orders", ctx, result_rows);
 * @endcode
 */
class RLSManager {
public:
    RLSManager() = default;

    // -------------------------------------------------------------------------
    // Policy management
    // -------------------------------------------------------------------------

    /// Register or replace a policy (identified by policy.id).
    void addPolicy(const RLSPolicy& policy);

    /// Remove a policy by id.
    /// @return true if the policy existed and was removed.
    bool removePolicy(const std::string& policy_id);

    /// Retrieve a policy by id.
    std::optional<RLSPolicy> getPolicy(const std::string& policy_id) const;

    /// List all registered policy ids.
    std::vector<std::string> listPolicies() const;

    /// Remove all policies for a given collection (empty string = global).
    void clearPoliciesForCollection(const std::string& collection);

    /// Remove all policies.
    void clearAllPolicies();

    /// Load policies from a JSON value.
    /// Expected format: { "policies": [ <RLSPolicy JSON>, ... ] }
    /// @return Number of policies loaded.
    size_t loadFromJson(const nlohmann::json& j);

    /// Persist all policies to JSON.
    nlohmann::json toJson() const;

    // -------------------------------------------------------------------------
    // Row filtering
    // -------------------------------------------------------------------------

    /**
     * @brief Filter a JSON array of rows according to applicable RLS policies.
     *
     * Policies are matched by:
     *  1. policy.collection == collection OR policy.collection.empty()
     *  2. policy.enabled == true
     *  3. ctx has at least one role in policy.applicable_roles, OR
     *     policy.applicable_roles is empty (applies to all users)
     *
     * Filtering logic (PostgreSQL-compatible):
     *  - If NO policies match: all rows pass (no RLS).
     *  - If ONLY PERMISSIVE policies match: a row passes if ANY allows it.
     *  - If ONLY RESTRICTIVE policies match: a row passes only if ALL allow it.
     *  - If BOTH types match: a row passes if (any PERMISSIVE allows it) AND
     *    (all RESTRICTIVE allow it).
     *
     * @param collection  Name of the queried collection.
     * @param ctx         Security context of the requesting user.
     * @param rows        JSON array of row objects returned by the query engine.
     * @return            Filtered JSON array (subset of rows).
     */
    nlohmann::json filterRows(
        const std::string& collection,
        const SecurityContext& ctx,
        const nlohmann::json& rows
    ) const;

    /**
     * @brief Determine whether RLS is active for a collection/user combination.
     *
     * Returns true when at least one enabled policy matches the
     * collection and security context.
     */
    bool isActive(
        const std::string& collection,
        const SecurityContext& ctx
    ) const;

private:
    mutable std::mutex mutex_;
    std::unordered_map<std::string, RLSPolicy> policies_;  ///< keyed by policy.id

    /// Collect policies that are applicable to (collection, ctx).
    std::vector<const RLSPolicy*> matchingPolicies(
        const std::string& collection,
        const SecurityContext& ctx
    ) const;  ///< called with mutex_ held
};

} // namespace security
} // namespace themis
