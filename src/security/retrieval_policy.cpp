/**
 * @file retrieval_policy.cpp
 * @brief Implementation of RetrievalPolicy
 *
 * Manages access control policies for retrieval operations.
 *
 * @date 2026-09-24
 */

#include "security/retrieval_policy.h"

#include <spdlog/spdlog.h>
#include <algorithm>
#include <regex>

namespace themis::security {

// ============================================================================
// PolicyCondition Implementation
// ============================================================================

nlohmann::json PolicyCondition::to_json() const {
  return nlohmann::json{
      {"classification", static_cast<int>(classification)},
      {"encryption_required", encryption_required},
      {"audit_required", audit_required},
      {"mfa_required", mfa_required},
      {"max_result_size", max_result_size},
      {"rate_limit_qps", rate_limit_qps},
  };
}

PolicyCondition PolicyCondition::from_json(const nlohmann::json& j) {
  PolicyCondition result;
  if (j.contains("classification")) {
    result.classification = static_cast<DataClassification>(j["classification"].get<int>());
  }
  if (j.contains("encryption_required")) result.encryption_required = j["encryption_required"];
  if (j.contains("audit_required")) result.audit_required = j["audit_required"];
  if (j.contains("mfa_required")) result.mfa_required = j["mfa_required"];
  if (j.contains("max_result_size")) result.max_result_size = j["max_result_size"];
  if (j.contains("rate_limit_qps")) result.rate_limit_qps = j["rate_limit_qps"];
  return result;
}

// ============================================================================
// PolicyRule Implementation
// ============================================================================

bool PolicyRule::Matches(PolicyAction action, const std::string& resource,
                         const std::string& principal) const {
  if (action != this->action) {
    return false;
  }

  // Match resource pattern
  try {
    std::regex resource_regex(resource_pattern);
    if (!std::regex_search(resource, resource_regex)) {
      return false;
    }
  } catch (const std::regex_error& e) {
    spdlog::warn("[PolicyRule] Invalid resource regex '{}': {}", resource_pattern, e.what());
    return false;
  }

  // Match principal pattern
  try {
    std::regex principal_regex(principal_pattern);
    if (!std::regex_search(principal, principal_regex)) {
      return false;
    }
  } catch (const std::regex_error& e) {
    spdlog::warn("[PolicyRule] Invalid principal regex '{}': {}", principal_pattern,
                 e.what());
    return false;
  }

  return true;
}

nlohmann::json PolicyRule::to_json() const {
  return nlohmann::json{
      {"rule_id", rule_id},
      {"action", static_cast<int>(action)},
      {"resource_pattern", resource_pattern},
      {"principal_pattern", principal_pattern},
      {"condition", condition.to_json()},
      {"allow", allow},
  };
}

PolicyRule PolicyRule::from_json(const nlohmann::json& j) {
  PolicyRule result;
  if (j.contains("rule_id")) result.rule_id = j["rule_id"];
  if (j.contains("action")) result.action = static_cast<PolicyAction>(j["action"].get<int>());
  if (j.contains("resource_pattern")) result.resource_pattern = j["resource_pattern"];
  if (j.contains("principal_pattern")) result.principal_pattern = j["principal_pattern"];
  if (j.contains("condition")) result.condition = PolicyCondition::from_json(j["condition"]);
  if (j.contains("allow")) result.allow = j["allow"];
  return result;
}

// ============================================================================
// RetrievalPolicy Implementation
// ============================================================================

RetrievalPolicy::RetrievalPolicy(const std::string& policy_id, uint32_t version_number)
    : policy_id_(policy_id), version_number_(version_number) {}

RetrievalPolicy::~RetrievalPolicy() = default;

void RetrievalPolicy::AddRule(const PolicyRule& rule) {
  rules_.push_back(rule);
  spdlog::debug("[RetrievalPolicy] Added rule {} to policy '{}'", rule.rule_id, policy_id_);
}

bool RetrievalPolicy::RemoveRule(uint32_t rule_id) {
  auto it = std::find_if(rules_.begin(), rules_.end(),
                         [rule_id](const PolicyRule& r) { return r.rule_id == rule_id; });
  if (it != rules_.end()) {
    rules_.erase(it);
    spdlog::debug("[RetrievalPolicy] Removed rule {} from policy '{}'", rule_id, policy_id_);
    return true;
  }
  return false;
}

bool RetrievalPolicy::IsAllowed(PolicyAction action, const std::string& resource,
                                const std::string& principal) const {
  // Evaluate rules in order (first match wins)
  for (const auto& rule : rules_) {
    if (rule.Matches(action, resource, principal)) {
      spdlog::debug(
          "[RetrievalPolicy] Rule {} matched for action={}, resource={}, principal={}: "
          "allow={}",
          rule.rule_id, static_cast<int>(action), resource, principal, rule.allow);
      return rule.allow;
    }
  }

  // No rules matched: default deny
  spdlog::debug("[RetrievalPolicy] No rules matched for action={}, resource={}, principal={}: "
                "default DENY",
                static_cast<int>(action), resource, principal);
  return false;
}

PolicyCondition RetrievalPolicy::GetCondition(PolicyAction action, const std::string& resource,
                                              const std::string& principal) const {
  for (const auto& rule : rules_) {
    if (rule.Matches(action, resource, principal) && rule.allow) {
      return rule.condition;
    }
  }
  return PolicyCondition();  // Return empty condition if no match
}

nlohmann::json RetrievalPolicy::to_json() const {
  nlohmann::json rules_array = nlohmann::json::array();
  for (const auto& rule : rules_) {
    rules_array.push_back(rule.to_json());
  }

  return nlohmann::json{
      {"policy_id", policy_id_},
      {"version_number", version_number_},
      {"rules", rules_array},
  };
}

RetrievalPolicy RetrievalPolicy::from_json(const nlohmann::json& j) {
  if (!j.contains("policy_id") || !j.contains("version_number")) {
    throw std::runtime_error("Missing policy_id or version_number in JSON");
  }

  RetrievalPolicy result(j["policy_id"].get<std::string>(), j["version_number"].get<uint32_t>());

  if (j.contains("rules")) {
    for (const auto& rule_json : j["rules"]) {
      result.AddRule(PolicyRule::from_json(rule_json));
    }
  }

  return result;
}

}  // namespace themis::security
