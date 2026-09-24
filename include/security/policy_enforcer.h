/**
 * @file policy_enforcer.h
 * @brief Runtime policy enforcement at query execution time
 *
 * Applies RetrievalPolicy rules to incoming queries and enforces
 * authorization, audit logging, and encryption requirements.
 *
 * **Enforcement Workflow:**
 * 1. Load policy for resource
 * 2. Check authorization (IsAllowed)
 * 3. Get enforcement conditions (encryption, audit, rate limit)
 * 4. Log audit event (if audit_required)
 * 5. Execute query with applied constraints
 * 6. Apply result filtering (classification, max_size)
 *
 * @date 2026-09-24
 */

#pragma once

#include "security/retrieval_policy.h"
#include <memory>
#include <nlohmann/json.hpp>
#include <chrono>
#include <map>

namespace themis::security {

/**
 * @struct QueryContext
 * @brief Runtime context for query enforcement
 */
struct QueryContext {
  std::string query_id;           ///< Unique query identifier
  std::string principal_id;       ///< User/application making request
  std::string resource_id;        ///< Resource being accessed
  PolicyAction action;
  std::string query_text;
  DataClassification data_classification;
  std::chrono::system_clock::time_point timestamp;

  /**
   * @brief Convert to JSON for audit logging
   * @return JSON representation
   */
  nlohmann::json to_json() const;
};

/**
 * @struct EnforcementDecision
 * @brief Decision from policy enforcement evaluation
 */
struct EnforcementDecision {
  bool allowed{false};
  std::string denial_reason;
  bool encryption_required{false};
  bool audit_required{false};
  bool mfa_required{false};
  uint32_t max_result_size{0};
  uint32_t rate_limit_qps{0};
  std::string policy_rule_matched;

  /**
   * @brief Check if this decision permits the query
   * @return true if query can proceed
   */
  bool IsPermitted() const { return allowed; }

  /**
   * @brief Convert to JSON for logging
   * @return JSON representation
   */
  nlohmann::json to_json() const;
};

/**
 * @class PolicyEnforcer
 * @brief Runtime policy enforcement engine
 *
 * Evaluates queries against loaded policies and enforces
 * authorization, auditing, and encryption requirements.
 *
 * **Thread Safety:** Thread-safe for concurrent query evaluation.
 * Policy updates are atomic with versioning.
 */
class PolicyEnforcer {
 public:
  /**
   * @brief Create a new policy enforcer
   * @param policy_store_path Path to store policies (for persistence)
   */
  explicit PolicyEnforcer(const std::string& policy_store_path);

  ~PolicyEnforcer();

  // Delete copy operations; move allowed for async I/O
  PolicyEnforcer(const PolicyEnforcer&) = delete;
  PolicyEnforcer& operator=(const PolicyEnforcer&) = delete;
  PolicyEnforcer(PolicyEnforcer&&) = default;
  PolicyEnforcer& operator=(PolicyEnforcer&&) = default;

  /**
   * @brief Load policy for a resource
   * @param policy_id Policy identifier
   * @param policy Policy object to load
   */
  void LoadPolicy(const std::string& policy_id, const RetrievalPolicy& policy);

  /**
   * @brief Get loaded policy
   * @param policy_id Policy identifier
   * @return Policy object, or nullptr if not loaded
   */
  RetrievalPolicy* GetPolicy(const std::string& policy_id);

  /**
   * @brief Enforce policy on a query
   *
   * Evaluates all matching policy rules and returns enforcement decision.
   *
   * @param context Query context (principal, resource, action, etc.)
   * @return EnforcementDecision with allow/deny and constraints
   */
  EnforcementDecision EnforceQuery(const QueryContext& context);

  /**
   * @brief Check if query is authorized
   * @param context Query context
   * @return true if policy allows query, false otherwise
   */
  bool IsAuthorized(const QueryContext& context);

  /**
   * @brief Get constraints for authorized query
   *
   * Returns encryption, audit, and rate limiting constraints.
   * Only valid if query is authorized.
   *
   * @param context Query context
   * @return PolicyCondition with enforcement constraints
   */
  PolicyCondition GetConstraints(const QueryContext& context);

  /**
   * @brief Update policy to new version
   *
   * Atomically replaces policy version. Old policy is preserved
   * for potential rollback.
   *
   * @param policy_id Policy identifier
   * @param new_policy New policy version
   * @return Previous policy version (for rollback)
   */
  RetrievalPolicy UpdatePolicy(const std::string& policy_id,
                               const RetrievalPolicy& new_policy);

  /**
   * @brief Rollback policy to previous version
   * @param policy_id Policy identifier
   * @param previous_policy Previous policy to restore
   * @return true if rollback succeeded
   */
  bool RollbackPolicy(const std::string& policy_id, const RetrievalPolicy& previous_policy);

  /**
   * @brief Persist policy to storage
   * @param policy_id Policy identifier
   * @param policy Policy to persist
   * @throws std::runtime_error on write failure
   */
  void PersistPolicy(const std::string& policy_id, const RetrievalPolicy& policy);

  /**
   * @brief Load policy from storage
   * @param policy_id Policy identifier
   * @return Loaded policy, or empty policy if not found
   * @throws std::runtime_error on read failure
   */
  RetrievalPolicy LoadPolicyFromStorage(const std::string& policy_id);

  /**
   * @brief Get all loaded policies
   * @return Vector of all loaded policy IDs
   */
  std::vector<std::string> GetLoadedPolicies() const;

 private:
  std::string policy_store_path_;
  std::map<std::string, std::unique_ptr<RetrievalPolicy>> policies_;
  std::map<std::string, std::unique_ptr<RetrievalPolicy>> policy_versions_;  ///< Version history

  /**
   * @brief Find matching policy for resource
   * @param resource_id Resource being accessed
   * @return Policy ID, or empty string if not found
   */
  std::string FindMatchingPolicy(const std::string& resource_id) const;
};

}  // namespace themis::security
