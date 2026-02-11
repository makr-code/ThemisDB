#include <gtest/gtest.h>
#include "governance/policy_manager.h"
#include "governance/policy_coordinator.h"
#include "server/policy_manager_api_handler.h"
#include "server/policy_engine.h"
#include <memory>
#include <fstream>
#include <thread>

using namespace themis::governance;
using namespace themis::server;
namespace http = boost::beast::http;

/**
 * @brief End-to-End Integration Tests for GAP-004 Security & Governance
 * 
 * These tests validate the complete integration of:
 * - PolicyManager (RBAC rules)
 * - PolicyEngine (VS classification)
 * - PolicyCoordinator (unified evaluation)
 * - PolicyManagerApiHandler (HTTP API)
 * 
 * Test Scenarios:
 * 1. Multi-role user access with different policy configurations
 * 2. Classification-based + RBAC combined enforcement
 * 3. HTTP API workflows (create -> evaluate -> update -> delete)
 * 4. Policy persistence and reload
 * 5. Performance under load
 */
class PolicyIntegrationE2ETest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create PolicyManager with test rules
        policy_manager = std::make_shared<PolicyManager>();
        
        // Create PolicyEngine (mock for testing)
        policy_engine = std::make_shared<PolicyEngine>();
        
        // Create PolicyCoordinator combining both
        coordinator = std::make_unique<PolicyCoordinator>(
            policy_engine,
            policy_manager
        );
        
        // Create API handler
        api_handler = std::make_unique<PolicyManagerApiHandler>(
            policy_manager,
            nullptr  // No auth for testing
        );
        
        // Setup test policies
        setupTestPolicies();
    }
    
    void TearDown() override {
        // Clean up test files
        std::remove(getTestYamlPath().c_str());
        std::remove(getTestJsonPath().c_str());
    }
    
    std::string getTestYamlPath() const {
        return "/tmp/test_policies_" + std::to_string(std::hash<std::thread::id>{}(std::this_thread::get_id())) + ".yaml";
    }
    
    std::string getTestJsonPath() const {
        return "/tmp/test_policies_" + std::to_string(std::hash<std::thread::id>{}(std::this_thread::get_id())) + ".json";
    }
    
    void setupTestPolicies() {
        // Policy 1: Sensitive data - requires encryption, no export
        PolicyRule sensitive_rule;
        sensitive_rule.id = "sensitive_data_policy";
        sensitive_rule.name = "Sensitive Data Protection";
        sensitive_rule.resources = {"data/sensitive/*", "data/personal/*"};
        sensitive_rule.actions = {"*"};
        sensitive_rule.required_roles = {"operator", "admin"};
        sensitive_rule.require_encryption = true;
        sensitive_rule.require_signature = true;
        sensitive_rule.allow_export = false;
        sensitive_rule.allow_cache = false;
        sensitive_rule.retention_days = 90;
        sensitive_rule.redaction_level = "strict";
        sensitive_rule.audit_access = true;
        sensitive_rule.audit_changes = true;
        sensitive_rule.enabled = true;
        sensitive_rule.priority = 100;
        policy_manager->addRule(sensitive_rule);
        
        // Policy 2: Public data - less restrictive
        PolicyRule public_rule;
        public_rule.id = "public_data_policy";
        public_rule.name = "Public Data Access";
        public_rule.resources = {"data/public/*"};
        public_rule.actions = {"read"};
        public_rule.required_roles = {};  // No role required
        public_rule.require_encryption = false;
        public_rule.require_signature = false;
        public_rule.allow_export = true;
        public_rule.allow_cache = true;
        public_rule.retention_days = 365;
        public_rule.redaction_level = "none";
        public_rule.audit_access = false;
        public_rule.audit_changes = false;
        public_rule.enabled = true;
        public_rule.priority = 50;
        policy_manager->addRule(public_rule);
        
        // Policy 3: Admin operations
        PolicyRule admin_rule;
        admin_rule.id = "admin_operations_policy";
        admin_rule.name = "Admin Operations";
        admin_rule.resources = {"admin/*", "config/*"};
        admin_rule.actions = {"*"};
        admin_rule.required_roles = {"admin"};
        admin_rule.require_encryption = true;
        admin_rule.require_signature = true;
        admin_rule.allow_export = false;
        admin_rule.allow_cache = false;
        admin_rule.retention_days = 730;
        admin_rule.redaction_level = "standard";
        admin_rule.audit_access = true;
        admin_rule.audit_changes = true;
        admin_rule.enabled = true;
        admin_rule.priority = 200;
        policy_manager->addRule(admin_rule);
    }
    
    http::request<http::string_body> makeRequest(
        http::verb method,
        const std::string& target,
        const std::string& body = ""
    ) {
        http::request<http::string_body> req{method, target, 11};
        req.set(http::field::host, "localhost");
        req.set(http::field::content_type, "application/json");
        req.set(http::field::authorization, "Bearer test-token");
        req.body() = body;
        req.prepare_payload();
        return req;
    }
    
    std::shared_ptr<PolicyManager> policy_manager;
    std::shared_ptr<PolicyEngine> policy_engine;
    std::unique_ptr<PolicyCoordinator> coordinator;
    std::unique_ptr<PolicyManagerApiHandler> api_handler;
};

// Test 1: Multi-role user access scenarios
TEST_F(PolicyIntegrationE2ETest, MultiRoleUserAccessControl) {
    // Scenario 1: Operator accessing sensitive data (allowed with restrictions)
    auto decision1 = policy_manager->evaluatePolicy(
        "data/sensitive/users",
        "read",
        {"operator"}
    );
    EXPECT_TRUE(decision1.allowed);
    EXPECT_TRUE(decision1.require_encryption);
    EXPECT_TRUE(decision1.require_signature);
    EXPECT_FALSE(decision1.allow_export);
    EXPECT_TRUE(decision1.audit_access);
    
    // Scenario 2: Guest accessing sensitive data (denied - no required role)
    auto decision2 = policy_manager->evaluatePolicy(
        "data/sensitive/users",
        "read",
        {"guest"}
    );
    EXPECT_FALSE(decision2.allowed);  // No matching role
    
    // Scenario 3: Anyone accessing public data (allowed)
    auto decision3 = policy_manager->evaluatePolicy(
        "data/public/docs",
        "read",
        {}  // No roles
    );
    EXPECT_TRUE(decision3.allowed);
    EXPECT_FALSE(decision3.require_encryption);
    EXPECT_TRUE(decision3.allow_export);
    
    // Scenario 4: Admin accessing admin resources (allowed)
    auto decision4 = policy_manager->evaluatePolicy(
        "admin/settings",
        "write",
        {"admin"}
    );
    EXPECT_TRUE(decision4.allowed);
    EXPECT_TRUE(decision4.require_encryption);
    EXPECT_EQ(decision4.retention_days, 730);
    
    // Scenario 5: Operator trying admin operation (denied)
    auto decision5 = policy_manager->evaluatePolicy(
        "admin/settings",
        "write",
        {"operator"}
    );
    EXPECT_FALSE(decision5.allowed);
}

// Test 2: Unified PolicyCoordinator evaluation
TEST_F(PolicyIntegrationE2ETest, UnifiedPolicyEvaluation) {
    std::unordered_map<std::string, std::string> headers = {
        {"X-Classification", "geheim"}
    };
    
    // Combined evaluation with both systems
    auto unified_decision = coordinator->evaluate(
        headers,
        "/api/data",
        "data/sensitive/users",
        "read",
        {"operator"}
    );
    
    // Verify both RBAC and classification decisions are combined
    EXPECT_TRUE(unified_decision.rbac_allowed);
    EXPECT_TRUE(unified_decision.require_encryption);
    EXPECT_FALSE(unified_decision.export_allowed);  // Note: member is export_allowed, not allow_export
    EXPECT_TRUE(unified_decision.audit_access);
    
    // Most restrictive wins
    EXPECT_EQ(unified_decision.retention_days, 90);  // Shortest retention
}

// Test 3: HTTP API workflow - Create, Read, Update, Delete
TEST_F(PolicyIntegrationE2ETest, HTTPAPIWorkflow) {
    // Step 1: Create new rule via API
    nlohmann::json new_rule = {
        {"id", "test_rule_e2e"},
        {"name", "E2E Test Rule"},
        {"description", "Created via E2E test"},
        {"resources", {"test/*"}},
        {"actions", {"read", "write"}},
        {"required_roles", {"tester"}},
        {"require_encryption", true},
        {"enabled", true},
        {"priority", 75}
    };
    
    auto create_req = makeRequest(http::verb::post, "/policies/rules", new_rule.dump());
    auto create_res = api_handler->handleCreateRule(create_req);
    EXPECT_EQ(create_res.result(), http::status::created);
    
    // Step 2: Verify rule exists via API
    auto get_req = makeRequest(http::verb::get, "/policies/rules/test_rule_e2e");
    auto get_res = api_handler->handleGetRule(get_req, "test_rule_e2e");
    EXPECT_EQ(get_res.result(), http::status::ok);
    
    auto get_body = nlohmann::json::parse(get_res.body());
    EXPECT_EQ(get_body["name"].get<std::string>(), "E2E Test Rule");
    
    // Step 3: Evaluate policy with new rule
    nlohmann::json eval_request = {
        {"resource", "test/data"},
        {"action", "read"},
        {"user_roles", {"tester"}}
    };
    
    auto eval_req = makeRequest(http::verb::post, "/policies/evaluate", eval_request.dump());
    auto eval_res = api_handler->handleEvaluatePolicy(eval_req);
    EXPECT_EQ(eval_res.result(), http::status::ok);
    
    auto eval_body = nlohmann::json::parse(eval_res.body());
    EXPECT_TRUE(eval_body["decision"]["allowed"].get<bool>());
    EXPECT_TRUE(eval_body["decision"]["require_encryption"].get<bool>());
    
    // Step 4: Update rule via API
    nlohmann::json update_rule = {
        {"name", "Updated E2E Test Rule"},
        {"resources", {"test/*", "test2/*"}},
        {"actions", {"read"}},
        {"required_roles", {"tester", "admin"}},
        {"require_encryption", false},
        {"enabled", true}
    };
    
    auto update_req = makeRequest(http::verb::put, "/policies/rules/test_rule_e2e", update_rule.dump());
    auto update_res = api_handler->handleUpdateRule(update_req, "test_rule_e2e");
    EXPECT_EQ(update_res.result(), http::status::ok);
    
    // Step 5: Verify update
    auto get2_req = makeRequest(http::verb::get, "/policies/rules/test_rule_e2e");
    auto get2_res = api_handler->handleGetRule(get2_req, "test_rule_e2e");
    auto get2_body = nlohmann::json::parse(get2_res.body());
    EXPECT_EQ(get2_body["name"].get<std::string>(), "Updated E2E Test Rule");
    EXPECT_FALSE(get2_body["require_encryption"].get<bool>());
    
    // Step 6: Delete rule via API
    auto delete_req = makeRequest(http::verb::delete_, "/policies/rules/test_rule_e2e");
    auto delete_res = api_handler->handleDeleteRule(delete_req, "test_rule_e2e");
    EXPECT_EQ(delete_res.result(), http::status::ok);
    
    // Step 7: Verify deletion
    auto get3_req = makeRequest(http::verb::get, "/policies/rules/test_rule_e2e");
    auto get3_res = api_handler->handleGetRule(get3_req, "test_rule_e2e");
    EXPECT_EQ(get3_res.result(), http::status::not_found);
}

// Test 4: Policy persistence and reload
TEST_F(PolicyIntegrationE2ETest, PolicyPersistenceAndReload) {
    // Save policies to YAML
    std::string yaml_path = getTestYamlPath();
    bool saved_yaml = policy_manager->saveRules(yaml_path);
    EXPECT_TRUE(saved_yaml);
    
    // Save policies to JSON
    std::string json_path = getTestJsonPath();
    bool saved_json = policy_manager->saveRules(json_path);
    EXPECT_TRUE(saved_json);
    
    // Create new manager and load from YAML
    auto manager_yaml = std::make_shared<PolicyManager>();
    bool loaded_yaml = manager_yaml->loadRules(yaml_path);
    EXPECT_TRUE(loaded_yaml);
    
    auto rules_yaml = manager_yaml->listRules();
    EXPECT_EQ(rules_yaml.size(), 3);  // Should have all 3 test policies
    
    // Create new manager and load from JSON
    auto manager_json = std::make_shared<PolicyManager>();
    bool loaded_json = manager_json->loadRules(json_path);
    EXPECT_TRUE(loaded_json);
    
    auto rules_json = manager_json->listRules();
    EXPECT_EQ(rules_json.size(), 3);
    
    // Verify loaded policies work correctly
    auto decision = manager_yaml->evaluatePolicy(
        "data/sensitive/users",
        "read",
        {"operator"}
    );
    EXPECT_TRUE(decision.allowed);
    EXPECT_TRUE(decision.require_encryption);
}

// Test 5: Wildcard pattern matching edge cases
TEST_F(PolicyIntegrationE2ETest, WildcardPatternMatching) {
    // Test nested wildcard matching
    auto decision1 = policy_manager->evaluatePolicy(
        "data/sensitive/deeply/nested/resource",
        "read",
        {"operator"}
    );
    EXPECT_TRUE(decision1.allowed);  // Matches data/sensitive/*
    
    // Test exact match vs wildcard
    auto decision2 = policy_manager->evaluatePolicy(
        "data/public/readme.txt",
        "read",
        {}
    );
    EXPECT_TRUE(decision2.allowed);  // Matches data/public/*
    
    // Test no match - NOTE: Default behavior is "allow" when no rules match
    // This is a security policy decision documented in PolicyManager
    auto decision3 = policy_manager->evaluatePolicy(
        "data/internal/resource",
        "read",
        {"operator"}
    );
    EXPECT_TRUE(decision3.allowed);  // Default allow when no rules match
}

// Test 6: Multi-rule aggregation with conflicts
TEST_F(PolicyIntegrationE2ETest, MultiRuleAggregation) {
    // Add conflicting rule for testing
    PolicyRule lenient_rule;
    lenient_rule.id = "lenient_sensitive_policy";
    lenient_rule.resources = {"data/sensitive/*"};
    lenient_rule.actions = {"read"};
    lenient_rule.required_roles = {"operator"};
    lenient_rule.require_encryption = false;  // Conflicts with sensitive_data_policy
    lenient_rule.allow_export = true;  // Conflicts
    lenient_rule.retention_days = 180;  // Different retention
    lenient_rule.enabled = true;
    lenient_rule.priority = 80;
    policy_manager->addRule(lenient_rule);
    
    // Evaluate with conflicting rules
    auto decision = policy_manager->evaluatePolicy(
        "data/sensitive/users",
        "read",
        {"operator"}
    );
    
    // Most restrictive wins
    EXPECT_TRUE(decision.allowed);
    EXPECT_TRUE(decision.require_encryption);  // OR logic - true wins
    EXPECT_FALSE(decision.allow_export);  // AND logic - false wins
    EXPECT_EQ(decision.retention_days, 90);  // MIN logic - shortest wins
}

// Test 7: Statistics and monitoring
TEST_F(PolicyIntegrationE2ETest, StatisticsAndMonitoring) {
    auto stats_req = makeRequest(http::verb::get, "/policies/stats");
    auto stats_res = api_handler->handleGetStats(stats_req);
    EXPECT_EQ(stats_res.result(), http::status::ok);
    
    auto stats = nlohmann::json::parse(stats_res.body());
    EXPECT_EQ(stats["total_rules"].get<int>(), 3);
    EXPECT_EQ(stats["enabled_rules"].get<int>(), 3);
    EXPECT_EQ(stats["disabled_rules"].get<int>(), 0);
    
    // Disable one rule and check stats again
    auto rule = policy_manager->getRule("public_data_policy");
    ASSERT_TRUE(rule.has_value());
    rule->enabled = false;
    policy_manager->removeRule("public_data_policy");
    policy_manager->addRule(*rule);
    
    auto stats_req2 = makeRequest(http::verb::get, "/policies/stats");
    auto stats_res2 = api_handler->handleGetStats(stats_req2);
    auto stats2 = nlohmann::json::parse(stats_res2.body());
    EXPECT_EQ(stats2["enabled_rules"].get<int>(), 2);
    EXPECT_EQ(stats2["disabled_rules"].get<int>(), 1);
}

// Test 8: Performance under load
TEST_F(PolicyIntegrationE2ETest, PerformanceUnderLoad) {
    const int iterations = 1000;
    auto start = std::chrono::high_resolution_clock::now();
    
    for (int i = 0; i < iterations; ++i) {
        auto decision = policy_manager->evaluatePolicy(
            "data/sensitive/resource_" + std::to_string(i),
            "read",
            {"operator"}
        );
        EXPECT_TRUE(decision.allowed);
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    // Should complete 1000 evaluations in reasonable time
    // NOTE: Threshold is 2000ms to account for slower CI systems
    // Expected baseline on modern hardware: < 1000ms
    EXPECT_LT(duration.count(), 2000);
    
    // Average should be < 2ms per evaluation (< 1ms on typical systems)
    double avg_ms = static_cast<double>(duration.count()) / iterations;
    EXPECT_LT(avg_ms, 2.0);
}

// Test 9: Coordinator null handling and fallback
TEST_F(PolicyIntegrationE2ETest, CoordinatorNullHandling) {
    // Create coordinator with null PolicyEngine
    auto coordinator_no_engine = std::make_unique<PolicyCoordinator>(
        nullptr,
        policy_manager
    );
    
    auto decision1 = coordinator_no_engine->evaluate(
        {},
        "/api/test",
        "data/sensitive/users",
        "read",
        {"operator"}
    );
    EXPECT_TRUE(decision1.rbac_allowed);
    
    // Create coordinator with null PolicyManager
    auto coordinator_no_manager = std::make_unique<PolicyCoordinator>(
        policy_engine,
        nullptr
    );
    
    std::unordered_map<std::string, std::string> headers = {
        {"X-Classification", "geheim"}
    };
    
    auto decision2 = coordinator_no_manager->evaluate(
        headers,
        "/api/test",
        "data/test",
        "read",
        {"operator"}
    );
    // Should still work with classification only
    EXPECT_FALSE(decision2.rbac_allowed);  // No RBAC rules to evaluate
}

// Test 10: Complex role hierarchies
TEST_F(PolicyIntegrationE2ETest, ComplexRoleHierarchies) {
    // User with multiple roles
    std::vector<std::string> multi_roles = {"guest", "operator", "analyst"};
    
    auto decision1 = policy_manager->evaluatePolicy(
        "data/sensitive/users",
        "read",
        multi_roles
    );
    EXPECT_TRUE(decision1.allowed);  // Has operator role
    
    // User with admin role (most privileged)
    std::vector<std::string> admin_roles = {"admin"};
    
    auto decision2 = policy_manager->evaluatePolicy(
        "admin/settings",
        "delete",
        admin_roles
    );
    EXPECT_TRUE(decision2.allowed);
    
    // User with insufficient roles
    std::vector<std::string> insufficient_roles = {"guest", "analyst"};
    
    auto decision3 = policy_manager->evaluatePolicy(
        "admin/settings",
        "write",
        insufficient_roles
    );
    EXPECT_FALSE(decision3.allowed);
}
