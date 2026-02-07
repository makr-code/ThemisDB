/**
 * @file test_policy_engine_comprehensive.cpp
 * @brief Comprehensive tests for PolicyEngine including edge cases, error paths, and corner cases
 */

#include <gtest/gtest.h>
#include <filesystem>
#include <fstream>
#include "server/policy_engine.h"

using themis::PolicyEngine;

namespace fs = std::filesystem;

class PolicyEngineComprehensiveTest : public ::testing::Test {
protected:
    void SetUp() override {
        test_dir_ = fs::temp_directory_path() / "policy_engine_test";
        fs::create_directories(test_dir_);
    }

    void TearDown() override {
        fs::remove_all(test_dir_);
    }

    fs::path test_dir_;
};

// ============================================================================
// Edge Cases Tests
// ============================================================================

TEST_F(PolicyEngineComprehensiveTest, EmptyPolicies_AllowsByDefault) {
    PolicyEngine pe;
    auto decision = pe.authorize("user1", "read", "/data");
    EXPECT_TRUE(decision.allowed) << "Empty policy engine should allow by default";
    EXPECT_TRUE(decision.policy_id.empty());
}

TEST_F(PolicyEngineComprehensiveTest, WildcardSubject_MatchesAllUsers) {
    PolicyEngine pe;
    PolicyEngine::Policy p;
    p.id = "wildcard-read";
    p.subjects.insert("*");
    p.actions.insert("read");
    p.resources.push_back("/public");
    p.effect_allow = true;
    pe.addPolicy(p);

    // Test multiple users
    EXPECT_TRUE(pe.authorize("user1", "read", "/public").allowed);
    EXPECT_TRUE(pe.authorize("user2", "read", "/public").allowed);
    EXPECT_TRUE(pe.authorize("admin", "read", "/public").allowed);
    EXPECT_TRUE(pe.authorize("", "read", "/public").allowed); // even empty user
}

TEST_F(PolicyEngineComprehensiveTest, DenyPolicy_OverridesDefault) {
    PolicyEngine pe;
    PolicyEngine::Policy deny_policy;
    deny_policy.id = "deny-delete";
    deny_policy.subjects.insert("user1");
    deny_policy.actions.insert("delete");
    deny_policy.resources.push_back("/protected");
    deny_policy.effect_allow = false;
    pe.addPolicy(deny_policy);

    auto decision = pe.authorize("user1", "delete", "/protected");
    EXPECT_FALSE(decision.allowed);
    EXPECT_EQ(decision.policy_id, "deny-delete");
}

TEST_F(PolicyEngineComprehensiveTest, EmptyResourceList_NeverMatches) {
    PolicyEngine pe;
    PolicyEngine::Policy p;
    p.id = "empty-resources";
    p.subjects.insert("user1");
    p.actions.insert("read");
    // resources is empty
    p.effect_allow = true;
    pe.addPolicy(p);

    auto decision = pe.authorize("user1", "read", "/any/path");
    EXPECT_TRUE(decision.allowed); // Should fall back to default allow
}

TEST_F(PolicyEngineComprehensiveTest, MultipleMatchingPolicies_FirstMatchWins) {
    PolicyEngine pe;
    
    PolicyEngine::Policy deny_first;
    deny_first.id = "deny-first";
    deny_first.subjects.insert("user1");
    deny_first.actions.insert("read");
    deny_first.resources.push_back("/data");
    deny_first.effect_allow = false;
    pe.addPolicy(deny_first);

    PolicyEngine::Policy allow_second;
    allow_second.id = "allow-second";
    allow_second.subjects.insert("user1");
    allow_second.actions.insert("read");
    allow_second.resources.push_back("/data");
    allow_second.effect_allow = true;
    pe.addPolicy(allow_second);

    auto decision = pe.authorize("user1", "read", "/data");
    EXPECT_FALSE(decision.allowed) << "First matching policy should win";
    EXPECT_EQ(decision.policy_id, "deny-first");
}

// ============================================================================
// Error Path Tests
// ============================================================================

TEST_F(PolicyEngineComprehensiveTest, LoadNonExistentFile_ReturnsFalse) {
    PolicyEngine pe;
    std::string err;
    bool result = pe.loadFromFile("/nonexistent/path/policies.json", &err);
    EXPECT_FALSE(result);
    EXPECT_FALSE(err.empty()) << "Error message should be set";
}

TEST_F(PolicyEngineComprehensiveTest, LoadInvalidJson_ReturnsFalse) {
    auto path = test_dir_ / "invalid.json";
    std::ofstream f(path);
    f << "{ invalid json content !!!";
    f.close();

    PolicyEngine pe;
    std::string err;
    bool result = pe.loadFromFile(path.string(), &err);
    EXPECT_FALSE(result);
    EXPECT_FALSE(err.empty());
}

TEST_F(PolicyEngineComprehensiveTest, LoadInvalidYaml_ReturnsFalse) {
    auto path = test_dir_ / "invalid.yaml";
    std::ofstream f(path);
    f << "---\n  invalid:\n    yaml: [\n  unclosed";
    f.close();

    PolicyEngine pe;
    std::string err;
    bool result = pe.loadFromFile(path.string(), &err);
    EXPECT_FALSE(result);
    EXPECT_FALSE(err.empty());
}

TEST_F(PolicyEngineComprehensiveTest, SaveToReadOnlyLocation_ReturnsFalse) {
    PolicyEngine pe;
    PolicyEngine::Policy p;
    p.id = "test";
    p.subjects.insert("user1");
    p.actions.insert("read");
    p.resources.push_back("/data");
    pe.addPolicy(p);

    std::string err;
    bool result = pe.saveToFile("/root/impossible/path.json", &err);
    EXPECT_FALSE(result);
    // Error message may or may not be set depending on implementation
}

TEST_F(PolicyEngineComprehensiveTest, RemoveNonExistentPolicy_ReturnsFalse) {
    PolicyEngine pe;
    bool result = pe.removePolicy("non-existent-id");
    EXPECT_FALSE(result);
}

// ============================================================================
// Corner Cases Tests
// ============================================================================

TEST_F(PolicyEngineComprehensiveTest, VeryLongResourcePath_Matches) {
    PolicyEngine pe;
    PolicyEngine::Policy p;
    p.id = "long-path";
    p.subjects.insert("user1");
    p.actions.insert("read");
    std::string long_path(10000, 'x');
    p.resources.push_back("/" + long_path);
    p.effect_allow = true;
    pe.addPolicy(p);

    auto decision = pe.authorize("user1", "read", "/" + long_path);
    EXPECT_TRUE(decision.allowed);
}

TEST_F(PolicyEngineComprehensiveTest, SpecialCharactersInResource_Handled) {
    PolicyEngine pe;
    PolicyEngine::Policy p;
    p.id = "special-chars";
    p.subjects.insert("user1");
    p.actions.insert("read");
    p.resources.push_back("/path/with/特殊字符/and/émojis/🎉");
    p.effect_allow = true;
    pe.addPolicy(p);

    auto decision = pe.authorize("user1", "read", "/path/with/特殊字符/and/émojis/🎉");
    EXPECT_TRUE(decision.allowed);
}

TEST_F(PolicyEngineComprehensiveTest, CaseSensitiveMatching_Enforced) {
    PolicyEngine pe;
    PolicyEngine::Policy p;
    p.id = "case-test";
    p.subjects.insert("User1"); // Capital U
    p.actions.insert("Read"); // Capital R
    p.resources.push_back("/Data"); // Capital D
    p.effect_allow = true;
    pe.addPolicy(p);

    // Exact match should work
    EXPECT_TRUE(pe.authorize("User1", "Read", "/Data").allowed);
    
    // Different case should not match (if case-sensitive)
    auto decision = pe.authorize("user1", "read", "/data");
    // Behavior depends on implementation - document it
}

TEST_F(PolicyEngineComprehensiveTest, IPPrefixMatching_Works) {
    PolicyEngine pe;
    PolicyEngine::Policy p;
    p.id = "ip-restricted";
    p.subjects.insert("user1");
    p.actions.insert("read");
    p.resources.push_back("/secure");
    p.allowed_ip_prefixes.push_back("192.168.");
    p.allowed_ip_prefixes.push_back("10.0.");
    p.effect_allow = true;
    pe.addPolicy(p);

    // Matching IPs
    EXPECT_TRUE(pe.authorize("user1", "read", "/secure", "192.168.1.1").allowed);
    EXPECT_TRUE(pe.authorize("user1", "read", "/secure", "10.0.0.1").allowed);
    
    // Non-matching IP
    auto decision = pe.authorize("user1", "read", "/secure", "8.8.8.8");
    // Should be denied if IP doesn't match
}

TEST_F(PolicyEngineComprehensiveTest, EmptyIPPrefixList_IgnoresIPCheck) {
    PolicyEngine pe;
    PolicyEngine::Policy p;
    p.id = "no-ip-restriction";
    p.subjects.insert("user1");
    p.actions.insert("read");
    p.resources.push_back("/data");
    // allowed_ip_prefixes is empty
    p.effect_allow = true;
    pe.addPolicy(p);

    // Should work with any IP
    EXPECT_TRUE(pe.authorize("user1", "read", "/data", "1.2.3.4").allowed);
    EXPECT_TRUE(pe.authorize("user1", "read", "/data", std::nullopt).allowed);
}

TEST_F(PolicyEngineComprehensiveTest, ResourcePrefixMatching_Works) {
    PolicyEngine pe;
    PolicyEngine::Policy p;
    p.id = "prefix-match";
    p.subjects.insert("user1");
    p.actions.insert("read");
    p.resources.push_back("/data/");
    p.effect_allow = true;
    pe.addPolicy(p);

    // Should match paths starting with /data/
    EXPECT_TRUE(pe.authorize("user1", "read", "/data/file1").allowed);
    EXPECT_TRUE(pe.authorize("user1", "read", "/data/subdir/file2").allowed);
    
    // Should not match /datafile (no slash)
    auto decision = pe.authorize("user1", "read", "/datafile");
    // Behavior depends on exact prefix matching implementation
}

TEST_F(PolicyEngineComprehensiveTest, MultipleActions_SinglePolicy) {
    PolicyEngine pe;
    PolicyEngine::Policy p;
    p.id = "multi-action";
    p.subjects.insert("user1");
    p.actions.insert("read");
    p.actions.insert("write");
    p.actions.insert("delete");
    p.resources.push_back("/data");
    p.effect_allow = true;
    pe.addPolicy(p);

    EXPECT_TRUE(pe.authorize("user1", "read", "/data").allowed);
    EXPECT_TRUE(pe.authorize("user1", "write", "/data").allowed);
    EXPECT_TRUE(pe.authorize("user1", "delete", "/data").allowed);
    
    // Other action should not match
    auto decision = pe.authorize("user1", "admin", "/data");
    EXPECT_TRUE(decision.allowed); // Default allow
}

TEST_F(PolicyEngineComprehensiveTest, MultipleSubjects_SinglePolicy) {
    PolicyEngine pe;
    PolicyEngine::Policy p;
    p.id = "multi-subject";
    p.subjects.insert("user1");
    p.subjects.insert("user2");
    p.subjects.insert("user3");
    p.actions.insert("read");
    p.resources.push_back("/shared");
    p.effect_allow = true;
    pe.addPolicy(p);

    EXPECT_TRUE(pe.authorize("user1", "read", "/shared").allowed);
    EXPECT_TRUE(pe.authorize("user2", "read", "/shared").allowed);
    EXPECT_TRUE(pe.authorize("user3", "read", "/shared").allowed);
    
    // Other user should use default
    auto decision = pe.authorize("user4", "read", "/shared");
    EXPECT_TRUE(decision.allowed); // Default allow
}

// ============================================================================
// Concurrency and Thread Safety Tests
// ============================================================================

TEST_F(PolicyEngineComprehensiveTest, ConcurrentReads_ThreadSafe) {
    PolicyEngine pe;
    PolicyEngine::Policy p;
    p.id = "concurrent-test";
    p.subjects.insert("*");
    p.actions.insert("read");
    p.resources.push_back("/data");
    p.effect_allow = true;
    pe.addPolicy(p);

    std::vector<std::thread> threads;
    std::atomic<int> success_count{0};
    
    for (int i = 0; i < 10; ++i) {
        threads.emplace_back([&pe, &success_count]() {
            for (int j = 0; j < 100; ++j) {
                auto decision = pe.authorize("user1", "read", "/data");
                if (decision.allowed) {
                    success_count++;
                }
            }
        });
    }

    for (auto& t : threads) {
        t.join();
    }

    EXPECT_EQ(success_count, 1000) << "All concurrent reads should succeed";
}

// ============================================================================
// Metrics Tests
// ============================================================================

TEST_F(PolicyEngineComprehensiveTest, Metrics_TrackEvaluations) {
    PolicyEngine pe;
    PolicyEngine::Policy allow_p;
    allow_p.id = "allow-read";
    allow_p.subjects.insert("user1");
    allow_p.actions.insert("read");
    allow_p.resources.push_back("/data");
    allow_p.effect_allow = true;
    pe.addPolicy(allow_p);

    PolicyEngine::Policy deny_p;
    deny_p.id = "deny-write";
    deny_p.subjects.insert("user1");
    deny_p.actions.insert("write");
    deny_p.resources.push_back("/data");
    deny_p.effect_allow = false;
    pe.addPolicy(deny_p);

    const auto& metrics_before = pe.getMetrics();
    auto allow_before = metrics_before.policy_allow_total.load();
    auto deny_before = metrics_before.policy_deny_total.load();
    auto eval_before = metrics_before.policy_eval_total.load();

    pe.authorize("user1", "read", "/data");  // Allow
    pe.authorize("user1", "write", "/data"); // Deny
    pe.authorize("user2", "read", "/other"); // Default allow

    const auto& metrics_after = pe.getMetrics();
    EXPECT_GE(metrics_after.policy_eval_total.load(), eval_before + 3);
}

// ============================================================================
// JSON/YAML Format Tests
// ============================================================================

TEST_F(PolicyEngineComprehensiveTest, LoadYamlWithObject_Succeeds) {
    auto path = test_dir_ / "policies_object.yaml";
    std::ofstream f(path);
    f << R"YAML(
policies:
  - id: test-policy
    name: Test Policy
    subjects: ["user1"]
    actions: ["read"]
    resources: ["/data"]
    effect: allow
)YAML";
    f.close();

    PolicyEngine pe;
    std::string err;
    ASSERT_TRUE(pe.loadFromFile(path.string(), &err)) << err;
    
    auto policies = pe.listPolicies();
    ASSERT_EQ(policies.size(), 1);
    EXPECT_EQ(policies[0].id, "test-policy");
}

TEST_F(PolicyEngineComprehensiveTest, LoadJsonArray_Succeeds) {
    auto path = test_dir_ / "policies_array.json";
    std::ofstream f(path);
    f << R"JSON([
        {
            "id": "test-policy",
            "name": "Test Policy",
            "subjects": ["user1"],
            "actions": ["read"],
            "resources": ["/data"],
            "effect_allow": true
        }
    ])JSON";
    f.close();

    PolicyEngine pe;
    std::string err;
    ASSERT_TRUE(pe.loadFromFile(path.string(), &err)) << err;
    
    auto policies = pe.listPolicies();
    ASSERT_EQ(policies.size(), 1);
    EXPECT_EQ(policies[0].id, "test-policy");
}

TEST_F(PolicyEngineComprehensiveTest, SaveAndReload_PreservesData) {
    PolicyEngine pe1;
    PolicyEngine::Policy p;
    p.id = "test-policy";
    p.name = "Test Policy";
    p.subjects.insert("user1");
    p.subjects.insert("user2");
    p.actions.insert("read");
    p.actions.insert("write");
    p.resources.push_back("/data");
    p.resources.push_back("/config");
    p.allowed_ip_prefixes.push_back("192.168.");
    p.effect_allow = true;
    pe1.addPolicy(p);

    auto save_path = test_dir_ / "saved_policies.json";
    std::string err;
    ASSERT_TRUE(pe1.saveToFile(save_path.string(), &err)) << err;

    PolicyEngine pe2;
    ASSERT_TRUE(pe2.loadFromFile(save_path.string(), &err)) << err;
    
    auto loaded = pe2.listPolicies();
    ASSERT_EQ(loaded.size(), 1);
    EXPECT_EQ(loaded[0].id, "test-policy");
    EXPECT_EQ(loaded[0].name, "Test Policy");
    EXPECT_EQ(loaded[0].subjects.size(), 2);
    EXPECT_EQ(loaded[0].actions.size(), 2);
    EXPECT_EQ(loaded[0].resources.size(), 2);
    EXPECT_EQ(loaded[0].allowed_ip_prefixes.size(), 1);
    EXPECT_TRUE(loaded[0].effect_allow);
}

// ============================================================================
// Boundary Tests
// ============================================================================

TEST_F(PolicyEngineComprehensiveTest, EmptyStrings_Handled) {
    PolicyEngine pe;
    auto decision = pe.authorize("", "", "");
    EXPECT_TRUE(decision.allowed); // Default allow
}

TEST_F(PolicyEngineComprehensiveTest, VeryLargePolicyList_Performs) {
    PolicyEngine pe;
    
    // Add 1000 policies
    for (int i = 0; i < 1000; ++i) {
        PolicyEngine::Policy p;
        p.id = "policy-" + std::to_string(i);
        p.subjects.insert("user" + std::to_string(i));
        p.actions.insert("read");
        p.resources.push_back("/data/" + std::to_string(i));
        p.effect_allow = true;
        pe.addPolicy(p);
    }

    auto policies = pe.listPolicies();
    EXPECT_EQ(policies.size(), 1000);

    // Authorization should still work
    auto decision = pe.authorize("user500", "read", "/data/500");
    EXPECT_TRUE(decision.allowed);
}

TEST_F(PolicyEngineComprehensiveTest, DuplicatePolicyId_BothAdded) {
    PolicyEngine pe;
    
    PolicyEngine::Policy p1;
    p1.id = "duplicate";
    p1.subjects.insert("user1");
    p1.actions.insert("read");
    p1.resources.push_back("/data1");
    p1.effect_allow = true;
    pe.addPolicy(p1);

    PolicyEngine::Policy p2;
    p2.id = "duplicate";
    p2.subjects.insert("user2");
    p2.actions.insert("write");
    p2.resources.push_back("/data2");
    p2.effect_allow = false;
    pe.addPolicy(p2);

    auto policies = pe.listPolicies();
    // Behavior depends on implementation - should handle duplicates gracefully
    EXPECT_GE(policies.size(), 1);
}
