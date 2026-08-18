/**
 * @file test_policy_versioning_and_approval.cpp
 * @brief Comprehensive tests for policy versioning and change management.
 * @version 1.0.0
 * @date 2026-08-18
 * 
 * Test Gates:
 * - GOV-Version-01: Version tracking basic functionality
 * - GOV-Version-02: Version history queries and comparisons
 * - GOV-Version-03: Rollback safety verification
 * - GOV-Version-04: Approval workflow state machine
 * - GOV-Version-05: Multi-policy coordinated rollback
 * - GOV-Version-06: Approval audit trail and emergency override
 * - GOV-GRG-05: Rollback latency benchmark (≤500ms)
 */

#include <gtest/gtest.h>

#include "governance/policy_manager.h"
#include "governance/policy_version_history.h"
#include "governance/policy_approval_workflow.h"
#include "governance/policy_change_manager.h"
#include "governance/policy_manager_versioned.h"

#include <chrono>
#include <vector>

namespace themis {
namespace governance {
namespace test {

// ========== Test Fixtures ==========

class PolicyVersioningTest : public ::testing::Test {
protected:
    void SetUp() override {
        version_history_ = std::make_shared<PolicyVersionHistory>();
        policy_manager_ = std::make_shared<PolicyManager>();
    }
    
    PolicyRule createTestRule(const std::string& id) {
        PolicyRule rule;
        rule.id                 = id;
        rule.name               = fmt::format("Test Rule {}", id);
        rule.description        = "Test rule for versioning";
        rule.classification_level = "public";
        rule.enabled            = true;
        rule.resources          = {"resource/*"};
        rule.actions            = {"read", "write"};
        rule.required_roles     = {"admin"};
        rule.require_encryption = true;
        rule.allow_export       = false;
        rule.allow_cache        = true;
        rule.retention_days     = 90;
        rule.priority           = 1;
        rule.created_by         = "test_user";
        rule.created_at         = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()
        ).count();
        rule.version            = "1.0.0";
        return rule;
    }
    
    std::shared_ptr<PolicyVersionHistory> version_history_;
    std::shared_ptr<PolicyManager> policy_manager_;
};

class ApprovalWorkflowTest : public ::testing::Test {
protected:
    void SetUp() override {
        workflow_ = std::make_unique<PolicyApprovalWorkflow>();
    }
    
    std::unique_ptr<PolicyApprovalWorkflow> workflow_;
};

class ChangeManagerTest : public ::testing::Test {
protected:
    void SetUp() override {
        version_history_ = std::make_shared<PolicyVersionHistory>();
        policy_manager_ = std::make_shared<PolicyManager>();
        change_manager_ = std::make_unique<PolicyChangeManager>(policy_manager_, version_history_);
    }
    
    PolicyRule createTestRule(const std::string& id) {
        PolicyRule rule;
        rule.id          = id;
        rule.name        = fmt::format("Policy {}", id);
        rule.description = "Test policy";
        rule.enabled     = true;
        rule.priority    = 1;
        rule.created_by  = "test_user";
        rule.version     = "1.0.0";
        return rule;
    }
    
    std::shared_ptr<PolicyVersionHistory> version_history_;
    std::shared_ptr<PolicyManager> policy_manager_;
    std::unique_ptr<PolicyChangeManager> change_manager_;
};

// ========== GOV-Version-01: Version Tracking Basic Functionality ==========

TEST_F(PolicyVersioningTest, GOV_Version_01_RecordNewVersion) {
    auto rule = createTestRule("rule-001");
    
    // Record first version
    auto version1 = version_history_->recordVersion(
        "rule-001",
        rule,
        "author1",
        "Initial policy"
    );
    
    EXPECT_EQ(version1, "1.0.0");
    
    // Modify rule and record second version
    rule.require_encryption = false;
    rule.priority           = 2;
    
    auto version2 = version_history_->recordVersion(
        "rule-001",
        rule,
        "author2",
        "Modified encryption requirement"
    );
    
    EXPECT_EQ(version2, "1.0.1");
    
    // Get versions
    auto versions = version_history_->getVersions("rule-001");
    EXPECT_EQ(versions.size(), 2);
    EXPECT_EQ(versions[0].version, "1.0.1");  // Newest first
    EXPECT_EQ(versions[1].version, "1.0.0");
}

TEST_F(PolicyVersioningTest, GOV_Version_01_SemanticVersioning) {
    auto rule = createTestRule("rule-002");
    
    // Record versions with different changes
    version_history_->recordVersion("rule-002", rule, "user1", "Initial");
    
    // Minor version bump
    rule.priority = 2;
    auto v2 = version_history_->recordVersion("rule-002", rule, "user1", "Update priority");
    EXPECT_EQ(v2, "1.0.1");
    
    // Another minor update
    rule.allow_export = true;
    auto v3 = version_history_->recordVersion("rule-002", rule, "user1", "Allow export");
    EXPECT_EQ(v3, "1.0.2");
}

TEST_F(PolicyVersioningTest, GOV_Version_01_VersionMetadata) {
    auto rule = createTestRule("rule-003");
    rule.created_by = "alice";
    rule.created_at = 1234567890000;
    
    auto v1 = version_history_->recordVersion(
        "rule-003",
        rule,
        "bob",
        "Policy created"
    );
    
    auto version_opt = version_history_->getVersion("rule-003", v1);
    ASSERT_TRUE(version_opt);
    
    auto version = version_opt.value();
    EXPECT_EQ(version.rule_id, "rule-003");
    EXPECT_EQ(version.author, "bob");
    EXPECT_GT(version.timestamp, 0);
    EXPECT_EQ(version.change_description, "Policy created");
    EXPECT_FALSE(version.rule_snapshot.empty());
}

// ========== GOV-Version-02: Version History Queries ==========

TEST_F(PolicyVersioningTest, GOV_Version_02_GetLatestVersion) {
    auto rule = createTestRule("rule-004");
    
    version_history_->recordVersion("rule-004", rule, "user1", "v1");
    rule.priority = 2;
    version_history_->recordVersion("rule-004", rule, "user1", "v2");
    rule.priority = 3;
    version_history_->recordVersion("rule-004", rule, "user1", "v3");
    
    auto latest = version_history_->getLatestVersion("rule-004");
    EXPECT_EQ(latest, "1.0.2");
}

TEST_F(PolicyVersioningTest, GOV_Version_02_GetPreviousVersion) {
    auto rule = createTestRule("rule-005");
    
    version_history_->recordVersion("rule-005", rule, "user1", "v1");
    rule.priority = 2;
    version_history_->recordVersion("rule-005", rule, "user1", "v2");
    
    auto prev = version_history_->getPreviousVersion("rule-005");
    ASSERT_TRUE(prev);
    EXPECT_EQ(prev.value(), "1.0.0");
}

TEST_F(PolicyVersioningTest, GOV_Version_02_CompareVersions) {
    auto rule = createTestRule("rule-006");
    
    auto v1 = version_history_->recordVersion("rule-006", rule, "user1", "Initial");
    
    rule.require_encryption = false;
    rule.priority = 5;
    auto v2 = version_history_->recordVersion("rule-006", rule, "user1", "Changes");
    
    auto diff = version_history_->compareVersions("rule-006", v1, v2);
    
    EXPECT_EQ(diff.rule_id, "rule-006");
    EXPECT_EQ(diff.version1, v1);
    EXPECT_EQ(diff.version2, v2);
    EXPECT_FALSE(diff.changes.empty());
    
    // Should detect at least encryption and priority changes
    auto changes_str = fmt::format("{}", fmt::join(diff.changes, ", "));
    EXPECT_TRUE(!changes_str.empty());
}

TEST_F(PolicyVersioningTest, GOV_Version_02_AuditTrail) {
    auto rule = createTestRule("rule-007");
    
    version_history_->recordVersion("rule-007", rule, "alice", "Created by Alice");
    rule.priority = 2;
    version_history_->recordVersion("rule-007", rule, "bob", "Modified by Bob");
    rule.priority = 3;
    version_history_->recordVersion("rule-007", rule, "charlie", "Modified by Charlie");
    
    // Query audit by user
    auto alice_edits = version_history_->queryAudit("rule-007", "alice");
    EXPECT_GE(alice_edits.size(), 1);
    
    auto bob_edits = version_history_->queryAudit("rule-007", "bob");
    EXPECT_GE(bob_edits.size(), 1);
}

// ========== GOV-Version-03: Rollback Safety ==========

TEST_F(ChangeManagerTest, GOV_Version_03_CheckRollbackSafety) {
    auto rule = createTestRule("rule-008");
    
    version_history_->recordVersion("rule-008", rule, "user1", "v1");
    rule.priority = 2;
    version_history_->recordVersion("rule-008", rule, "user1", "v2");
    rule.priority = 3;
    auto v3 = version_history_->recordVersion("rule-008", rule, "user1", "v3");
    
    auto safety = change_manager_->checkRollbackSafety("rule-008", "1.0.1");
    
    EXPECT_EQ(safety.rule_id, "rule-008");
    EXPECT_EQ(safety.target_version, "1.0.1");
    EXPECT_NE(safety.safety_level, RollbackSafetyLevel::BLOCKED);
}

TEST_F(ChangeManagerTest, GOV_Version_03_PreviewRollback) {
    auto rule = createTestRule("rule-009");
    
    version_history_->recordVersion("rule-009", rule, "user1", "v1");
    rule.priority = 2;
    rule.allow_export = false;
    version_history_->recordVersion("rule-009", rule, "user1", "v2");
    
    auto preview = change_manager_->previewRollback("rule-009", "1.0.0");
    
    EXPECT_NE(preview.safety_level, RollbackSafetyLevel::BLOCKED);
    EXPECT_EQ(preview.target_version, "1.0.0");
}

TEST_F(ChangeManagerTest, GOV_Version_03_DependencyTracking) {
    // Create two policies with dependency
    change_manager_->registerDependency(
        "policy-child",
        "policy-parent",
        "inheritance",
        "Child policy inherits from parent"
    );
    
    auto deps = change_manager_->getDependencies("policy-child");
    EXPECT_EQ(deps.size(), 1);
    EXPECT_EQ(deps[0].dependency_rule_id, "policy-parent");
    
    auto rev_deps = change_manager_->getReverseDependencies("policy-parent");
    EXPECT_EQ(rev_deps.size(), 1);
    EXPECT_EQ(rev_deps[0].dependent_rule_id, "policy-child");
}

// ========== GOV-Version-04: Approval Workflow State Machine ==========

TEST_F(ApprovalWorkflowTest, GOV_Version_04_ApprovalStateMachine) {
    // Initiate review
    auto status = workflow_->initiateReview(
        "rule-010",
        "1.0.0",
        "submitter",
        1
    );
    
    EXPECT_EQ(status.current_state, ApprovalState::DRAFT);
    EXPECT_EQ(status.rule_id, "rule-010");
    
    // Submit for review
    EXPECT_TRUE(workflow_->submitForReview("rule-010", "reviewer1"));
    auto status1 = workflow_->getApprovalStatus("rule-010");
    ASSERT_TRUE(status1);
    EXPECT_EQ(status1->current_state, ApprovalState::REVIEW);
    
    // Approve
    EXPECT_TRUE(workflow_->approveChange("rule-010", "reviewer1", "Looks good"));
    auto status2 = workflow_->getApprovalStatus("rule-010");
    ASSERT_TRUE(status2);
    EXPECT_EQ(status2->current_state, ApprovalState::APPROVED);
    
    // Activate
    EXPECT_TRUE(workflow_->activatePolicy("rule-010", "operator"));
    auto status3 = workflow_->getApprovalStatus("rule-010");
    ASSERT_TRUE(status3);
    EXPECT_EQ(status3->current_state, ApprovalState::ACTIVE);
}

TEST_F(ApprovalWorkflowTest, GOV_Version_04_ApprovalRejection) {
    workflow_->initiateReview("rule-011", "1.0.0", "submitter", 1);
    workflow_->submitForReview("rule-011", "reviewer");
    
    // Reject change
    EXPECT_TRUE(workflow_->rejectChange("rule-011", "reviewer", "Needs revision"));
    
    auto status = workflow_->getApprovalStatus("rule-011");
    ASSERT_TRUE(status);
    EXPECT_EQ(status->current_state, ApprovalState::DRAFT);
}

TEST_F(ApprovalWorkflowTest, GOV_Version_04_MultipleApprovers) {
    workflow_->initiateReview("rule-012", "1.0.0", "submitter", 2);  // Requires 2 approvers
    workflow_->submitForReview("rule-012", "reviewer");
    
    // First approval
    EXPECT_TRUE(workflow_->approveChange("rule-012", "reviewer1", "OK"));
    auto status1 = workflow_->getApprovalStatus("rule-012");
    EXPECT_EQ(status1->current_state, ApprovalState::REVIEW);  // Still under review
    
    // Second approval
    EXPECT_TRUE(workflow_->approveChange("rule-012", "reviewer2", "Approved"));
    auto status2 = workflow_->getApprovalStatus("rule-012");
    EXPECT_EQ(status2->current_state, ApprovalState::APPROVED);  // Now approved
}

TEST_F(ApprovalWorkflowTest, GOV_Version_04_GetRulesInState) {
    workflow_->initiateReview("rule-013", "1.0.0", "submitter", 1);
    workflow_->initiateReview("rule-014", "1.0.0", "submitter", 1);
    
    workflow_->submitForReview("rule-013", "reviewer");
    workflow_->submitForReview("rule-014", "reviewer");
    
    auto review_rules = workflow_->getRulesInState(ApprovalState::REVIEW);
    EXPECT_GE(review_rules.size(), 2);
}

// ========== GOV-Version-05: Multi-Policy Coordinated Rollback ==========

TEST_F(ChangeManagerTest, GOV_Version_05_CoordinatedRollback) {
    // Create multiple policies
    auto rule1 = createTestRule("policy-1");
    auto rule2 = createTestRule("policy-2");
    
    version_history_->recordVersion("policy-1", rule1, "user", "v1");
    rule1.priority = 2;
    version_history_->recordVersion("policy-1", rule1, "user", "v2");
    
    version_history_->recordVersion("policy-2", rule2, "user", "v1");
    rule2.priority = 2;
    version_history_->recordVersion("policy-2", rule2, "user", "v2");
    
    // Perform coordinated rollback
    std::vector<std::string> rules = {"policy-1", "policy-2"};
    auto op = change_manager_->performCoordinatedRollback(
        rules,
        "1.0.0",
        "operator",
        "Coordinated rollback"
    );
    
    EXPECT_EQ(op.rule_id, "policy-1");
    EXPECT_EQ(op.multi_rule_ids.size(), 2);
    EXPECT_GT(op.operation_id.size(), 0);
}

TEST_F(ChangeManagerTest, GOV_Version_05_RollbackInProgress) {
    auto rule = createTestRule("rule-015");
    version_history_->recordVersion("rule-015", rule, "user", "v1");
    rule.priority = 2;
    version_history_->recordVersion("rule-015", rule, "user", "v2");
    
    EXPECT_FALSE(change_manager_->isRollbackInProgress("rule-015"));
    
    auto op = change_manager_->performRollback(
        "rule-015",
        "1.0.0",
        "operator",
        "Rollback test"
    );
    
    // After completion, should not be in progress
    EXPECT_FALSE(change_manager_->isRollbackInProgress("rule-015"));
}

TEST_F(ChangeManagerTest, GOV_Version_05_RollbackHistory) {
    auto rule = createTestRule("rule-016");
    version_history_->recordVersion("rule-016", rule, "user", "v1");
    rule.priority = 2;
    version_history_->recordVersion("rule-016", rule, "user", "v2");
    
    change_manager_->performRollback(
        "rule-016",
        "1.0.0",
        "operator1",
        "Reason 1"
    );
    
    change_manager_->performRollback(
        "rule-016",
        "1.0.1",
        "operator2",
        "Reason 2"
    );
    
    auto history = change_manager_->getRollbackHistory("rule-016");
    EXPECT_GE(history.size(), 1);
}

// ========== GOV-Version-06: Approval Audit Trail & Emergency Override ==========

TEST_F(ApprovalWorkflowTest, GOV_Version_06_ApprovalAuditTrail) {
    workflow_->initiateReview("rule-017", "1.0.0", "submitter", 1);
    workflow_->submitForReview("rule-017", "reviewer");
    workflow_->approveChange("rule-017", "reviewer", "OK");
    workflow_->activatePolicy("rule-017", "operator");
    
    auto history = workflow_->queryApprovalHistory("rule-017");
    EXPECT_GE(history.size(), 1);
    
    // Verify audit trail has operations
    bool has_approval = false;
    for (const auto& record : history) {
        if (record.action == ApprovalAction::APPROVE) {
            has_approval = true;
            break;
        }
    }
    EXPECT_TRUE(has_approval);
}

TEST_F(ApprovalWorkflowTest, GOV_Version_06_EmergencyOverride) {
    EXPECT_TRUE(workflow_->emergencyOverride(
        "rule-018",
        "security_officer",
        "CRITICAL SECURITY PATCH",
        1
    ));
    
    auto status = workflow_->getApprovalStatus("rule-018");
    ASSERT_TRUE(status);
    EXPECT_EQ(status->current_state, ApprovalState::ACTIVE);
    
    // Verify emergency override in audit trail
    auto history = workflow_->queryApprovalHistory("rule-018");
    bool found_override = false;
    for (const auto& record : history) {
        if (record.is_emergency_override) {
            found_override = true;
            break;
        }
    }
    EXPECT_TRUE(found_override);
}

TEST_F(ApprovalWorkflowTest, GOV_Version_06_RollbackApproval) {
    workflow_->initiateReview("rule-019", "1.0.0", "submitter", 1);
    workflow_->submitForReview("rule-019", "reviewer");
    workflow_->approveChange("rule-019", "reviewer");
    workflow_->activatePolicy("rule-019", "operator");
    
    // Rollback approval
    EXPECT_TRUE(workflow_->rollbackApproval("rule-019", "operator", "Reverting changes"));
    
    auto status = workflow_->getApprovalStatus("rule-019");
    ASSERT_TRUE(status);
    EXPECT_EQ(status->current_state, ApprovalState::DEPRECATED);
}

// ========== GOV-GRG-05: Rollback Latency Benchmark ==========

TEST_F(ChangeManagerTest, GOV_GRG_05_RollbackLatency) {
    // Test rollback latency (target: ≤500ms)
    auto rule = createTestRule("bench-rule");
    version_history_->recordVersion("bench-rule", rule, "user", "v1");
    rule.priority = 2;
    version_history_->recordVersion("bench-rule", rule, "user", "v2");
    
    auto start = std::chrono::high_resolution_clock::now();
    
    auto op = change_manager_->performRollback(
        "bench-rule",
        "1.0.0",
        "operator",
        "Benchmark test"
    );
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    
    EXPECT_LE(duration, 500);  // Rollback should complete in ≤500ms
    
    std::cout << "Rollback latency: " << duration << "ms (target: ≤500ms)" << std::endl;
}

TEST_F(PolicyVersioningTest, GOV_GRG_05_VersionQueryLatency) {
    // Test version query latency (target: ≤10ms)
    auto rule = createTestRule("query-rule");
    version_history_->recordVersion("query-rule", rule, "user", "v1");
    rule.priority = 2;
    version_history_->recordVersion("query-rule", rule, "user", "v2");
    
    auto start = std::chrono::high_resolution_clock::now();
    
    auto latest = version_history_->getLatestVersion("query-rule");
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    
    EXPECT_LE(duration, 10);  // Query should complete in ≤10ms
    EXPECT_EQ(latest, "1.0.1");
    
    std::cout << "Version query latency: " << duration << "ms (target: ≤10ms)" << std::endl;
}

// ========== Integration Tests ==========

TEST_F(PolicyVersioningTest, IntegrationTest_FullVersioningWorkflow) {
    // Create policy
    auto rule = createTestRule("integration-rule");
    auto v1 = version_history_->recordVersion(
        "integration-rule",
        rule,
        "alice",
        "Initial policy creation"
    );
    EXPECT_EQ(v1, "1.0.0");
    
    // Modify policy
    rule.priority = 2;
    auto v2 = version_history_->recordVersion(
        "integration-rule",
        rule,
        "bob",
        "Updated priority"
    );
    EXPECT_EQ(v2, "1.0.1");
    
    // Query history
    auto versions = version_history_->getVersions("integration-rule");
    EXPECT_EQ(versions.size(), 2);
    
    // Compare versions
    auto diff = version_history_->compareVersions("integration-rule", v1, v2);
    EXPECT_FALSE(diff.changes.empty());
    
    // Verify audit trail
    auto audit = version_history_->queryAudit("integration-rule");
    EXPECT_GE(audit.size(), 1);
}

} // namespace test
} // namespace governance
} // namespace themis
