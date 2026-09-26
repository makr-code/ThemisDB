/**
 * @file policy_enforcer.cpp
 * @brief Implementation of PolicyEnforcer
 *
 * Runtime query-time policy enforcement.
 *
 * @date 2026-09-24
 */

#include "security/policy_enforcer.h"

#include <spdlog/spdlog.h>
#include <fstream>

namespace themis::security {

// ============================================================================
// QueryContext Implementation
// ============================================================================

nlohmann::json QueryContext::to_json() const {
  return nlohmann::json{
      {"query_id", query_id},
      {"principal_id", principal_id},
      {"resource_id", resource_id},
      {"action", static_cast<int>(action)},
      {"query_text", query_text},
      {"data_classification", static_cast<int>(data_classification)},
      {"timestamp", std::chrono::system_clock::to_time_t(timestamp)},
  };
}

// ============================================================================
// EnforcementDecision Implementation
// ============================================================================

nlohmann::json EnforcementDecision::to_json() const {
  return nlohmann::json{
      {"allowed", allowed},
      {"denial_reason", denial_reason},
      {"encryption_required", encryption_required},
      {"audit_required", audit_required},
      {"mfa_required", mfa_required},
      {"max_result_size", max_result_size},
      {"rate_limit_qps", rate_limit_qps},
      {"policy_rule_matched", policy_rule_matched},
  };
}

// ============================================================================
// PolicyEnforcer Implementation
// ============================================================================

PolicyEnforcer::PolicyEnforcer(const std::string& policy_store_path)
    : policy_store_path_(policy_store_path) {
  spdlog::info("[PolicyEnforcer] Initialized with policy store: {}", policy_store_path);
}

PolicyEnforcer::~PolicyEnforcer() = default;

void PolicyEnforcer::LoadPolicy(const std::string& policy_id, const RetrievalPolicy& policy) {
  policies_[policy_id] = std::make_unique<RetrievalPolicy>(policy);
  spdlog::info("[PolicyEnforcer] Loaded policy '{}' with {} rules", policy_id, policy.RuleCount());
}

RetrievalPolicy* PolicyEnforcer::GetPolicy(const std::string& policy_id) {
  auto it = policies_.find(policy_id);
  if (it != policies_.end()) {
    return it->second.get();
  }
  return nullptr;
}

EnforcementDecision PolicyEnforcer::EnforceQuery(const QueryContext& context) {
  EnforcementDecision decision;
  decision.policy_rule_matched = FindMatchingPolicy(context.resource_id);

  if (decision.policy_rule_matched.empty()) {
    // No policy found for resource
    decision.allowed = false;
    decision.denial_reason = "No policy defined for resource";
    spdlog::warn(
        "[PolicyEnforcer] Query {} denied: no policy for resource '{}'", context.query_id,
        context.resource_id);
    return decision;
  }

  auto policy = GetPolicy(decision.policy_rule_matched);
  if (!policy) {
    decision.allowed = false;
    decision.denial_reason = "Policy not loaded";
    spdlog::error(
        "[PolicyEnforcer] Query {} denied: policy '{}' not loaded", context.query_id,
        decision.policy_rule_matched);
    return decision;
  }

  // Check authorization
  decision.allowed = policy->IsAllowed(context.action, context.resource_id, context.principal_id);

  if (!decision.allowed) {
    decision.denial_reason = "Authorization denied by policy";
    spdlog::warn(
        "[PolicyEnforcer] Query {} denied: principal '{}' not authorized for {} on '{}'",
        context.query_id, context.principal_id, static_cast<int>(context.action),
        context.resource_id);
    return decision;
  }

  // Get constraints for authorized query
  auto condition = policy->GetCondition(context.action, context.resource_id, context.principal_id);
  decision.encryption_required = condition.encryption_required;
  decision.audit_required = condition.audit_required;
  decision.mfa_required = condition.mfa_required;
  decision.max_result_size = condition.max_result_size;
  decision.rate_limit_qps = condition.rate_limit_qps;

  spdlog::debug(
      "[PolicyEnforcer] Query {} authorized: encryption={}, audit={}, "
      "mfa={}, max_size={}, rate_limit={}",
      context.query_id, decision.encryption_required, decision.audit_required,
      decision.mfa_required, decision.max_result_size, decision.rate_limit_qps);

  return decision;
}

bool PolicyEnforcer::IsAuthorized(const QueryContext& context) {
  auto decision = EnforceQuery(context);
  return decision.IsPermitted();
}

PolicyCondition PolicyEnforcer::GetConstraints(const QueryContext& context) {
  auto policy_id = FindMatchingPolicy(context.resource_id);
  if (policy_id.empty()) {
    return PolicyCondition();
  }

  auto policy = GetPolicy(policy_id);
  if (!policy) {
    return PolicyCondition();
  }

  return policy->GetCondition(context.action, context.resource_id, context.principal_id);
}

RetrievalPolicy PolicyEnforcer::UpdatePolicy(const std::string& policy_id,
                                             const RetrievalPolicy& new_policy) {
  auto old_policy = GetPolicy(policy_id);
  if (!old_policy) {
    throw std::runtime_error("Policy not loaded: " + policy_id);
  }

  // Save old version
  auto old_copy = std::make_unique<RetrievalPolicy>(*old_policy);
  policy_versions_[policy_id] = std::move(old_copy);

  // Load new policy
  LoadPolicy(policy_id, new_policy);

  spdlog::info("[PolicyEnforcer] Updated policy '{}' to version {}", policy_id,
               new_policy.GetVersion());

  // Return old policy for caller (for logging/rollback)
  return *old_policy;
}

bool PolicyEnforcer::RollbackPolicy(const std::string& policy_id,
                                    const RetrievalPolicy& previous_policy) {
  auto it = policy_versions_.find(policy_id);
  if (it == policy_versions_.end()) {
    spdlog::error("[PolicyEnforcer] No version history for policy '{}'", policy_id);
    return false;
  }

  LoadPolicy(policy_id, previous_policy);
  policy_versions_.erase(it);

  spdlog::info("[PolicyEnforcer] Rolled back policy '{}' to version {}", policy_id,
               previous_policy.GetVersion());
  return true;
}

void PolicyEnforcer::PersistPolicy(const std::string& policy_id, const RetrievalPolicy& policy) {
  std::string policy_path = policy_store_path_ + "/" + policy_id + ".json";
  std::ofstream file(policy_path);
  if (!file.is_open()) {
    throw std::runtime_error("Failed to open policy file for writing: " + policy_path);
  }

  auto json = policy.to_json();
  file << json.dump(2);  // Pretty-printed
  file.close();

  spdlog::debug("[PolicyEnforcer] Persisted policy '{}' to {}", policy_id, policy_path);
}

RetrievalPolicy PolicyEnforcer::LoadPolicyFromStorage(const std::string& policy_id) {
  std::string policy_path = policy_store_path_ + "/" + policy_id + ".json";
  std::ifstream file(policy_path);
  if (!file.is_open()) {
    spdlog::warn("[PolicyEnforcer] Policy file not found: {}", policy_path);
    return RetrievalPolicy(policy_id, 0);
  }

  try {
    auto json = nlohmann::json::parse(file);
    auto policy = RetrievalPolicy::from_json(json);
    file.close();
    spdlog::info("[PolicyEnforcer] Loaded policy '{}' from storage", policy_id);
    return policy;
  } catch (const std::exception& e) {
    file.close();
    throw std::runtime_error("Failed to parse policy JSON: " + std::string(e.what()));
  }
}

std::vector<std::string> PolicyEnforcer::GetLoadedPolicies() const {
  std::vector<std::string> policy_ids;
  for (const auto& entry : policies_) {
    policy_ids.push_back(entry.first);
  }
  return policy_ids;
}

std::string PolicyEnforcer::FindMatchingPolicy(const std::string& resource_id) const {
  // Simple matching: exact match first, then pattern matching
  if (policies_.find(resource_id) != policies_.end()) {
    return resource_id;
  }

  // Try finding a policy that matches the resource pattern
  for (const auto& entry : policies_) {
    const auto& policy = entry.second;
    // Check if any rule in this policy matches the resource
    for (const auto& rule : policy->GetRules()) {
      try {
        std::regex pattern(rule.resource_pattern);
        if (std::regex_search(resource_id, pattern)) {
          return entry.first;
        }
      } catch (const std::regex_error& e) {
        spdlog::warn("[PolicyEnforcer] Invalid regex pattern: {}", e.what());
      }
    }
  }

  return "";  // No matching policy found
}

}  // namespace themis::security
