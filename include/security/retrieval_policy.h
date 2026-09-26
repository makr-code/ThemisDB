/**
 * @file retrieval_policy.h
 * @brief Policy enforcement for retrieval operations
 *
 * Defines policies for authorization (AUTHZ), audit requirements (AUDIT),
 * and encryption guardrails (ENCRYPTION) applied at query time.
 *
 * **Policy Matching:**
 * - Action: read, query, export, bulk_export
 * - Resource: index_id, document_class, user_role
 * - Condition: data_classification, encryption_required, audit_required
 *
 * **Enforcement:**
 * - AUTHZ: Deny unless explicitly allowed (default deny)
 * - AUDIT: Log all decisions and query results
 * - ENCRYPTION: Require encryption for sensitive data
 *
 * @date 2026-09-24
 */

#pragma once

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <cstdint>
#include <regex>
#include <nlohmann/json.hpp>

namespace themis::security {

/**
 * @enum PolicyAction
 * @brief Allowed actions in retrieval policies
 */
enum class PolicyAction {
  Read = 0,         ///< Read/query operation
  Query = 1,        ///< Query with result filtering
  Export = 2,       ///< Export subset of results
  BulkExport = 3    ///< Bulk export of data
};

/**
 * @enum DataClassification
 * @brief Data sensitivity classification
 */
enum class DataClassification {
  Public = 0,
  Internal = 1,
  Confidential = 2,
  Secret = 3
};

/**
 * @struct PolicyCondition
 * @brief Conditions for policy matching
 */
struct PolicyCondition {
  DataClassification classification{DataClassification::Public};
  bool encryption_required{false};
  bool audit_required{false};
  bool mfa_required{false};
  uint32_t max_result_size{0};  ///< Max results per query (0 = unlimited)
  uint32_t rate_limit_qps{0};   ///< Queries per second (0 = unlimited)

  /**
   * @brief Serialize to JSON
   * @return JSON representation
   */
  nlohmann::json to_json() const;

  /**
   * @brief Deserialize from JSON
   * @param j JSON object
   * @return Deserialized PolicyCondition
   */
  static PolicyCondition from_json(const nlohmann::json& j);
};

/**
 * @struct PolicyRule
 * @brief Single policy rule with action/resource matching
 */
struct PolicyRule {
  uint32_t rule_id{0};
  PolicyAction action;
  std::string resource_pattern;   ///< Resource regex pattern (index_id, doc_class, etc.)
  std::string principal_pattern;  ///< Principal regex pattern (user_id, role, etc.)
  PolicyCondition condition;
  bool allow{false};  ///< Allow or Deny

  /**
   * @brief Check if rule matches query context
   * @param action Action being performed
   * @param resource Resource being accessed
   * @param principal User/principal making request
   * @return true if rule matches all criteria
   */
  bool Matches(PolicyAction action, const std::string& resource,
               const std::string& principal) const;

  /// @brief Serialize this struct to JSON.
  /// @return JSON object with all PolicyRule fields.
  nlohmann::json to_json() const;
  /// @brief Deserialize from JSON.
  /// @param j JSON object to parse.
  /// @return Deserialized PolicyRule.
  static PolicyRule from_json(const nlohmann::json& j);
};

/**
 * @class RetrievalPolicy
 * @brief Complete set of retrieval access control policies
 *
 * Policies are evaluated in order (first match wins).
 * Default behavior is DENY (unless explicitly allowed).
 *
 * **Policy Versioning:**
 * - Each policy set has a version number
 * - Policies can be rolled back to previous versions
 * - Version history tracked in metadata
 */
class RetrievalPolicy {
 public:
  /**
   * @brief Create a new policy set
   * @param policy_id Unique policy identifier
   * @param version_number Version of this policy set
   */
  RetrievalPolicy(const std::string& policy_id, uint32_t version_number);

  ~RetrievalPolicy();

  /**
   * @brief Add a policy rule
   * @param rule Rule to add
   */
  void AddRule(const PolicyRule& rule);

  /**
   * @brief Remove rule by ID
   * @param rule_id Rule to remove
   * @return true if rule existed and was removed
   */
  bool RemoveRule(uint32_t rule_id);

  /**
   * @brief Evaluate if action is allowed
   *
   * Evaluates rules in order, returns true on first ALLOW match,
   * false if any DENY matches or no rules match (default deny).
   *
   * @param action Action being performed
   * @param resource Resource being accessed
   * @param principal User/principal making request
   * @return true if action allowed, false otherwise
   */
  bool IsAllowed(PolicyAction action, const std::string& resource,
                 const std::string& principal) const;

  /**
   * @brief Get matching policy condition for query
   *
   * Returns the most specific condition from matching ALLOW rule.
   *
   * @param action Action being performed
   * @param resource Resource being accessed
   * @param principal User/principal making request
   * @return Matching condition, or default (empty) if no match
   */
  PolicyCondition GetCondition(PolicyAction action, const std::string& resource,
                               const std::string& principal) const;

  /**
   * @brief Serialize policy to JSON
   * @return Complete policy JSON with all rules
   */
  nlohmann::json to_json() const;

  /**
   * @brief Deserialize policy from JSON
   * @param j JSON object with policy definition
   * @return Deserialized RetrievalPolicy
   * @throws std::runtime_error if JSON malformed
   */
  static RetrievalPolicy from_json(const nlohmann::json& j);

  /**
   * @brief Get policy ID
   * @return Policy identifier
   */
  std::string GetPolicyId() const { return policy_id_; }

  /**
   * @brief Get policy version
   * @return Version number
   */
  uint32_t GetVersion() const { return version_number_; }

  /**
   * @brief Get all rules
   * @return Vector of all policy rules
   */
  const std::vector<PolicyRule>& GetRules() const { return rules_; }

  /**
   * @brief Get rule count
   * @return Number of rules in this policy
   */
  size_t RuleCount() const { return rules_.size(); }

 private:
  std::string policy_id_;
  uint32_t version_number_;
  std::vector<PolicyRule> rules_;  ///< Rules evaluated in order
};

}  // namespace themis::security
