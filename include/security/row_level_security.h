/**
 * @file row_level_security.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
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

struct RLSPredicate {
    std::string field;        ///< Row field name to test
    std::string op;           ///< Comparison operator
    std::string value;        ///< Static literal value (JSON-encoded scalar/array)
    std::string user_attr;    ///< SecurityContext attribute key (mutually exclusive with value)

    /**
     * @brief Evaluate.
     * @param[in] row Input parameter.
     * @param[in] ctx Input parameter.
     * @return True when the operation succeeds.
     */
    bool evaluate(const nlohmann::json& row, const SecurityContext& ctx) const;

    /**
     * @brief To Json.
     * @return Return value.
     */
    nlohmann::json toJson() const;
    /**
     * @brief From Json.
     * @param[in] j Input parameter.
     * @return Return value.
     */
    static RLSPredicate fromJson(const nlohmann::json& j);
};

enum class RLSPolicyType {
    PERMISSIVE,
    RESTRICTIVE
};

struct RLSPolicy {
    std::string id;                          ///< Unique policy identifier
    std::string collection;                  ///< Target collection ("" = all)
    std::vector<std::string> applicable_roles; ///< Matching roles (empty = all users)
    RLSPredicate predicate;                  ///< Row filter condition
    RLSPolicyType type = RLSPolicyType::PERMISSIVE;
    bool enabled = true;

    /**
     * @brief To Json.
     * @return Return value.
     */
    nlohmann::json toJson() const;
    /**
     * @brief From Json.
     * @param[in] j Input parameter.
     * @return Return value.
     */
    static RLSPolicy fromJson(const nlohmann::json& j);
};

class RLSManager {
public:
    RLSManager() = default;

    // -------------------------------------------------------------------------
    // Policy management
    // -------------------------------------------------------------------------

    /**
     * @brief Add Policy.
     * @param[in] policy Input parameter.
     */
    void addPolicy(const RLSPolicy& policy);

    /**
     * @brief Remove a retention policy by name.
     * @param[in] policy_id Identifier of the policy.
     * @return True when the policy existed and was removed.
     */
    bool removePolicy(const std::string& policy_id);

    /**
     * @brief Look up a retention policy by name.
     * @param[in] policy_id Identifier of the policy.
     * @return Pointer to the stored policy on success, or an error if it is missing.
     */
    std::optional<RLSPolicy> getPolicy(const std::string& policy_id) const;

    /**
     * @brief List Policies.
     * @return Return value.
     */
    std::vector<std::string> listPolicies() const;

    /**
     * @brief Clear Policies For Collection.
     * @param[in] collection Input parameter.
     */
    void clearPoliciesForCollection(const std::string& collection);

    /**
     * @brief Clear All Policies.
     */
    void clearAllPolicies();

    /**
     * @brief Load From Json.
     * @param[in] j Input parameter.
     * @return Return value.
     */
    size_t loadFromJson(const nlohmann::json& j);

    /**
     * @brief To Json.
     * @return Return value.
     */
    nlohmann::json toJson() const;

    // -------------------------------------------------------------------------
    // Row filtering
    // -------------------------------------------------------------------------

    /**
     * @brief Filter Rows.
     * @param[in] collection Input parameter.
     * @param[in] ctx Input parameter.
     * @param[in] rows Input parameter.
     * @return Return value.
     */
    nlohmann::json filterRows(
        const std::string& collection,
        const SecurityContext& ctx,
        const nlohmann::json& rows
    ) const;

    /**
     * @brief Is Active.
     * @param[in] collection Input parameter.
     * @param[in] ctx Input parameter.
     * @return True when the operation succeeds.
     */
    bool isActive(
        const std::string& collection,
        const SecurityContext& ctx
    ) const;

private:
    mutable std::mutex mutex_;
    std::unordered_map<std::string, RLSPolicy> policies_;  ///< keyed by policy.id

    /**
     * @brief Matching Policies.
     * @param[in] collection Input parameter.
     * @param[in] ctx Input parameter.
     * @return Return value.
     */
    std::vector<const RLSPolicy*> matchingPolicies(
        const std::string& collection,
        const SecurityContext& ctx
    ) const;  ///< called with mutex_ held
};

} // namespace security
} // namespace themis
