/**
 * @file test_retrieval_policy.cpp
 * @brief Tests for RetrievalPolicy and PolicyEnforcer
 *
 * Test cases: POLICY-01..08
 *
 * @date 2026-09-24
 */

#include <gtest/gtest.h>
#include <filesystem>

#include "security/retrieval_policy.h"
#include "security/policy_enforcer.h"

using namespace themis::security;

class RetrievalPolicyTest : public ::testing::Test {
 protected:
  RetrievalPolicy CreateBasicPolicy() {
    RetrievalPolicy policy("basic-policy", 1);

    // Rule 1: Allow reads by admin on any resource
    PolicyRule rule1;
    rule1.rule_id = 1;
    rule1.action = PolicyAction::Read;
    rule1.resource_pattern = ".*";  // Any resource
    rule1.principal_pattern = "admin.*";
    rule1.allow = true;
    rule1.condition.encryption_required = false;
    rule1.condition.audit_required = true;
    policy.AddRule(rule1);

    // Rule 2: Allow queries by user on public data
    PolicyRule rule2;
    rule2.rule_id = 2;
    rule2.action = PolicyAction::Query;
    rule2.resource_pattern = "public_.*";
    rule2.principal_pattern = "user.*";
    rule2.allow = true;
    rule2.condition.encryption_required = false;
    rule2.condition.audit_required = false;
    policy.AddRule(rule2);

    return policy;
  }
};

/**
 * @test POLICY-01: Rule matching on action, resource, principal
 */
TEST_F(RetrievalPolicyTest, POLICY_01_RuleMatching) {
  auto policy = CreateBasicPolicy();

  // Admin can read (any resource)
  EXPECT_TRUE(policy.IsAllowed(PolicyAction::Read, "index_1", "admin_user"));

  // User cannot read (not in allowed action)
  EXPECT_FALSE(policy.IsAllowed(PolicyAction::Read, "index_1", "user_1"));

  // User can query public data
  EXPECT_TRUE(policy.IsAllowed(PolicyAction::Query, "public_index", "user_1"));

  // User cannot query private data (pattern mismatch)
  EXPECT_FALSE(policy.IsAllowed(PolicyAction::Query, "private_index", "user_1"));
}

/**
 * @test POLICY-02: Default deny when no rules match
 */
TEST_F(RetrievalPolicyTest, POLICY_02_DefaultDeny) {
  RetrievalPolicy policy("empty-policy", 1);
  // No rules added

  // Everything should be denied
  EXPECT_FALSE(policy.IsAllowed(PolicyAction::Read, "any_resource", "any_principal"));
  EXPECT_FALSE(policy.IsAllowed(PolicyAction::Query, "any_resource", "any_principal"));
  EXPECT_FALSE(policy.IsAllowed(PolicyAction::Export, "any_resource", "any_principal"));
}

/**
 * @test POLICY-03: Get enforcement conditions for allowed query
 */
TEST_F(RetrievalPolicyTest, POLICY_03_GetConditions) {
  auto policy = CreateBasicPolicy();

  // Admin read should have encryption=false, audit=true
  auto condition = policy.GetCondition(PolicyAction::Read, "index_1", "admin_user");
  EXPECT_FALSE(condition.encryption_required);
  EXPECT_TRUE(condition.audit_required);

  // User query should have encryption=false, audit=false
  condition = policy.GetCondition(PolicyAction::Query, "public_index", "user_1");
  EXPECT_FALSE(condition.encryption_required);
  EXPECT_FALSE(condition.audit_required);

  // Denied query should return empty condition
  condition = policy.GetCondition(PolicyAction::Read, "index_1", "user_1");
  EXPECT_FALSE(condition.encryption_required);
  EXPECT_FALSE(condition.audit_required);
}

/**
 * @test POLICY-04: First matching rule wins
 */
TEST_F(RetrievalPolicyTest, POLICY_04_FirstMatchWins) {
  RetrievalPolicy policy("priority-policy", 1);

  // Rule 1: Allow all reads
  PolicyRule rule1;
  rule1.rule_id = 1;
  rule1.action = PolicyAction::Read;
  rule1.resource_pattern = ".*";
  rule1.principal_pattern = ".*";
  rule1.allow = true;
  rule1.condition.audit_required = true;
  policy.AddRule(rule1);

  // Rule 2: Deny reads for user_1 (added after rule1)
  PolicyRule rule2;
  rule2.rule_id = 2;
  rule2.action = PolicyAction::Read;
  rule2.resource_pattern = ".*";
  rule2.principal_pattern = "user_1";
  rule2.allow = false;
  policy.AddRule(rule2);

  // Rule1 should match first, so user_1 is allowed (should be true, not false from rule2)
  EXPECT_TRUE(policy.IsAllowed(PolicyAction::Read, "index_1", "user_1"));
}

/**
 * @test POLICY-05: Policy versioning
 */
TEST_F(RetrievalPolicyTest, POLICY_05_PolicyVersioning) {
  RetrievalPolicy policy_v1("versioned-policy", 1);
  RetrievalPolicy policy_v2("versioned-policy", 2);

  EXPECT_EQ(policy_v1.GetVersion(), 1);
  EXPECT_EQ(policy_v2.GetVersion(), 2);

  // Add different rules to each version
  PolicyRule rule1;
  rule1.rule_id = 1;
  rule1.action = PolicyAction::Read;
  rule1.resource_pattern = ".*";
  rule1.principal_pattern = ".*";
  rule1.allow = true;
  policy_v1.AddRule(rule1);

  EXPECT_EQ(policy_v1.RuleCount(), 1);
  EXPECT_EQ(policy_v2.RuleCount(), 0);
}

/**
 * @test POLICY-06: JSON serialization/deserialization
 */
TEST_F(RetrievalPolicyTest, POLICY_06_JsonSerialization) {
  auto original = CreateBasicPolicy();

  // Serialize to JSON
  auto json = original.to_json();
  EXPECT_TRUE(json.contains("policy_id"));
  EXPECT_TRUE(json.contains("version_number"));
  EXPECT_TRUE(json.contains("rules"));

  // Deserialize back
  auto restored = RetrievalPolicy::from_json(json);
  EXPECT_EQ(restored.GetPolicyId(), original.GetPolicyId());
  EXPECT_EQ(restored.GetVersion(), original.GetVersion());
  EXPECT_EQ(restored.RuleCount(), original.RuleCount());

  // Behavior should be identical
  EXPECT_EQ(restored.IsAllowed(PolicyAction::Read, "index_1", "admin_user"),
            original.IsAllowed(PolicyAction::Read, "index_1", "admin_user"));
}

/**
 * @test POLICY-07: Remove rule
 */
TEST_F(RetrievalPolicyTest, POLICY_07_RemoveRule) {
  auto policy = CreateBasicPolicy();
  EXPECT_EQ(policy.RuleCount(), 2);

  // Remove first rule
  EXPECT_TRUE(policy.RemoveRule(1));
  EXPECT_EQ(policy.RuleCount(), 1);

  // Admin read should now be denied (rule removed)
  EXPECT_FALSE(policy.IsAllowed(PolicyAction::Read, "index_1", "admin_user"));

  // Try to remove non-existent rule
  EXPECT_FALSE(policy.RemoveRule(999));
}

/**
 * @test POLICY-08: Complex condition enforcement
 */
TEST_F(RetrievalPolicyTest, POLICY_08_ComplexConditions) {
  RetrievalPolicy policy("complex-policy", 1);

  PolicyRule rule;
  rule.rule_id = 1;
  rule.action = PolicyAction::Export;
  rule.resource_pattern = "confidential_.*";
  rule.principal_pattern = "admin.*";
  rule.allow = true;
  rule.condition.encryption_required = true;
  rule.condition.audit_required = true;
  rule.condition.mfa_required = true;
  rule.condition.max_result_size = 10000;
  rule.condition.rate_limit_qps = 10;
  rule.condition.classification = DataClassification::Confidential;
  policy.AddRule(rule);

  auto condition = policy.GetCondition(PolicyAction::Export, "confidential_data", "admin_1");
  EXPECT_TRUE(condition.encryption_required);
  EXPECT_TRUE(condition.audit_required);
  EXPECT_TRUE(condition.mfa_required);
  EXPECT_EQ(condition.max_result_size, 10000);
  EXPECT_EQ(condition.rate_limit_qps, 10);
  EXPECT_EQ(condition.classification, DataClassification::Confidential);
}

// ============================================================================
// PolicyEnforcer Tests
// ============================================================================

class PolicyEnforcerTest : public ::testing::Test {
 protected:
  void SetUp() override {
    temp_dir_ = std::filesystem::temp_directory_path() / "test_policy_store_XXXXXX";
    std::filesystem::create_directories(temp_dir_);
  }

  void TearDown() override {
    if (std::filesystem::exists(temp_dir_)) {
      std::filesystem::remove_all(temp_dir_);
    }
  }

  std::filesystem::path temp_dir_;

  PolicyEnforcer CreateEnforcer() {
    return PolicyEnforcer(temp_dir_.string());
  }
};

/**
 * @test PolicyEnforcer query evaluation
 */
TEST_F(PolicyEnforcerTest, QueryEnforcement) {
  auto enforcer = CreateEnforcer();
  auto policy = RetrievalPolicy("test-policy", 1);

  PolicyRule rule;
  rule.rule_id = 1;
  rule.action = PolicyAction::Query;
  rule.resource_pattern = "public_.*";
  rule.principal_pattern = "user_.*";
  rule.allow = true;
  policy.AddRule(rule);

  enforcer.LoadPolicy("test-policy", policy);

  QueryContext context;
  context.query_id = "q1";
  context.principal_id = "user_1";
  context.resource_id = "public_index";
  context.action = PolicyAction::Query;

  auto decision = enforcer.EnforceQuery(context);
  EXPECT_TRUE(decision.allowed);
  EXPECT_EQ(decision.denial_reason, "");
}

/**
 * @test PolicyEnforcer denial
 */
TEST_F(PolicyEnforcerTest, QueryDenial) {
  auto enforcer = CreateEnforcer();
  auto policy = RetrievalPolicy("test-policy", 1);
  // No rules added → default deny

  enforcer.LoadPolicy("test-policy", policy);

  QueryContext context;
  context.query_id = "q1";
  context.principal_id = "user_1";
  context.resource_id = "private_index";
  context.action = PolicyAction::Query;

  auto decision = enforcer.EnforceQuery(context);
  EXPECT_FALSE(decision.allowed);
  EXPECT_FALSE(decision.denial_reason.empty());
}

/**
 * @test PolicyEnforcer policy update and rollback
 */
TEST_F(PolicyEnforcerTest, PolicyUpdateRollback) {
  auto enforcer = CreateEnforcer();
  auto policy_v1 = RetrievalPolicy("test-policy", 1);

  PolicyRule rule1;
  rule1.rule_id = 1;
  rule1.action = PolicyAction::Read;
  rule1.resource_pattern = ".*";
  rule1.principal_pattern = "admin.*";
  rule1.allow = true;
  policy_v1.AddRule(rule1);

  enforcer.LoadPolicy("test-policy", policy_v1);

  // Update to new version
  auto policy_v2 = RetrievalPolicy("test-policy", 2);
  // No rules in v2 → all denied
  
  auto old_policy = enforcer.UpdatePolicy("test-policy", policy_v2);
  EXPECT_EQ(old_policy.GetVersion(), 1);

  // Rollback to v1
  enforcer.RollbackPolicy("test-policy", old_policy);
  auto current = enforcer.GetPolicy("test-policy");
  EXPECT_EQ(current->GetVersion(), 1);
}
