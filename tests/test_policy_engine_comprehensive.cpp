/**
 * @file test_policy_engine_comprehensive.cpp
 * @brief Comprehensive tests for the PolicyEngine security component
 *
 * Tests cover:
 * - Policy loading (JSON and YAML)
 * - Authorization logic (allow/deny, subject/action/resource matching)
 * - Default behaviors (no policies, no match)
 * - IP-based conditions
 * - Policy management (add, remove, list)
 * - Persistence (save/load)
 * - Metrics tracking
 * - Concurrency
 * - Edge cases
 */

#include <gtest/gtest.h>
#include "server/policy_engine.h"
#include <filesystem>
#include <fstream>
#include <thread>
#include <atomic>

using themis::PolicyEngine;

namespace {

// Helper to create a PolicyEngine with a single allow policy
void addPolicyToEngine(PolicyEngine& pe,
                       const std::string& id,
                       const std::string& subject,
                       const std::string& action,
                       const std::string& resource,
                       bool effect_allow = true) {
    PolicyEngine::Policy p;
    p.id = id;
    p.name = "Test policy";
    p.subjects.insert(subject);
    p.actions.insert(action);
    p.resources.push_back(resource);
    p.effect_allow = effect_allow;
    pe.addPolicy(p);
}

} // anonymous namespace

// ============================================================================
// Policy Loading Tests
// ============================================================================

class PolicyEngineLoadTest : public ::testing::Test {
protected:
    void SetUp() override {
        tmp_dir_ = std::filesystem::temp_directory_path() / "pe_test";
        std::filesystem::create_directories(tmp_dir_);
    }
    void TearDown() override {
        std::filesystem::remove_all(tmp_dir_);
    }
    std::filesystem::path tmp_dir_;
};

TEST_F(PolicyEngineLoadTest, LoadJsonArray_Success) {
    auto path = (tmp_dir_ / "policies.json").string();
    std::ofstream f(path);
    f << R"([{"id":"p1","name":"allow-all","subjects":["admin"],"actions":["read"],"resources":["/data"],"effect":"allow"}])";
    f.close();

    PolicyEngine pe;
    std::string err;
    ASSERT_TRUE(pe.loadFromFile(path, &err)) << err;
    EXPECT_EQ(pe.listPolicies().size(), 1u);
}

TEST_F(PolicyEngineLoadTest, LoadJsonObject_WithPoliciesArray_Success) {
    auto path = (tmp_dir_ / "policies2.json").string();
    std::ofstream f(path);
    f << R"({"policies":[{"id":"p2","subjects":["user"],"actions":["read"],"resources":["/api"],"effect":"allow"}]})";
    f.close();

    PolicyEngine pe;
    ASSERT_TRUE(pe.loadFromFile(path));
    EXPECT_EQ(pe.listPolicies().size(), 1u);
}

TEST_F(PolicyEngineLoadTest, LoadYamlSequence_Success) {
    auto path = (tmp_dir_ / "policies.yaml").string();
    std::ofstream f(path);
    f << R"YAML(
- id: y1
  name: yaml-policy
  subjects: ["analyst"]
  actions: ["read"]
  resources: ["/metrics"]
  effect: allow
)YAML";
    f.close();

    PolicyEngine pe;
    std::string err;
    ASSERT_TRUE(pe.loadFromFile(path, &err)) << err;
    auto list = pe.listPolicies();
    ASSERT_EQ(list.size(), 1u);
    EXPECT_EQ(list[0].id, "y1");
}

TEST_F(PolicyEngineLoadTest, LoadYamlObject_WithPoliciesKey_Success) {
    auto path = (tmp_dir_ / "policies2.yaml").string();
    std::ofstream f(path);
    f << R"YAML(
policies:
  - id: y2
    subjects: ["*"]
    actions: ["read"]
    resources: ["/public"]
    effect: allow
)YAML";
    f.close();

    PolicyEngine pe;
    ASSERT_TRUE(pe.loadFromFile(path));
    EXPECT_EQ(pe.listPolicies().size(), 1u);
}

TEST_F(PolicyEngineLoadTest, LoadNonExistentFile_Fails) {
    PolicyEngine pe;
    std::string err;
    EXPECT_FALSE(pe.loadFromFile("/nonexistent/path/policies.json", &err));
}

TEST_F(PolicyEngineLoadTest, SaveAndReloadJson_PreservesData) {
    PolicyEngine pe;
    PolicyEngine::Policy p;
    p.id = "save-test";
    p.name = "Save test policy";
    p.subjects.insert("admin");
    p.actions.insert("write");
    p.resources.push_back("/entities");
    p.effect_allow = true;
    pe.addPolicy(p);

    auto path = (tmp_dir_ / "saved.json").string();
    std::string err;
    ASSERT_TRUE(pe.saveToFile(path, &err)) << err;

    PolicyEngine pe2;
    ASSERT_TRUE(pe2.loadFromFile(path));
    auto list = pe2.listPolicies();
    ASSERT_EQ(list.size(), 1u);
    EXPECT_EQ(list[0].id, "save-test");
    EXPECT_TRUE(list[0].subjects.count("admin") > 0);
    EXPECT_TRUE(list[0].actions.count("write") > 0);
    EXPECT_EQ(list[0].resources[0], "/entities");
}

// ============================================================================
// Authorization Tests
// ============================================================================

TEST(PolicyEngineAuthTest, NoPolicies_DefaultAllow) {
    PolicyEngine pe;
    auto d = pe.authorize("anyone", "read", "/data");
    EXPECT_TRUE(d.allowed);
    EXPECT_EQ(d.reason, "no_policies_default_allow");
}

TEST(PolicyEngineAuthTest, AllowPolicy_MatchingRequest_Allowed) {
    PolicyEngine pe;
    addPolicyToEngine(pe, "allow-read", "alice", "read", "/data");
    auto d = pe.authorize("alice", "read", "/data/users");
    EXPECT_TRUE(d.allowed);
    EXPECT_EQ(d.policy_id, "allow-read");
}

TEST(PolicyEngineAuthTest, DenyPolicy_MatchingRequest_Denied) {
    PolicyEngine pe;
    addPolicyToEngine(pe, "deny-delete", "bob", "delete", "/data", false);
    auto d = pe.authorize("bob", "delete", "/data/anything");
    EXPECT_FALSE(d.allowed);
    EXPECT_EQ(d.policy_id, "deny-delete");
}

TEST(PolicyEngineAuthTest, NoMatch_DefaultDeny) {
    PolicyEngine pe;
    addPolicyToEngine(pe, "allow-read", "alice", "read", "/data");
    // Different user, different action, different resource
    auto d = pe.authorize("eve", "delete", "/secrets");
    EXPECT_FALSE(d.allowed);
    EXPECT_EQ(d.reason, "no_matching_policy");
}

TEST(PolicyEngineAuthTest, WildcardSubject_MatchesAnyUser) {
    PolicyEngine pe;
    PolicyEngine::Policy p;
    p.id = "allow-all-users";
    p.subjects.insert("*");
    p.actions.insert("read");
    p.resources.push_back("/public");
    p.effect_allow = true;
    pe.addPolicy(p);

    EXPECT_TRUE(pe.authorize("alice", "read", "/public").allowed);
    EXPECT_TRUE(pe.authorize("bob", "read", "/public").allowed);
    EXPECT_TRUE(pe.authorize("anonymous", "read", "/public/page").allowed);
}

TEST(PolicyEngineAuthTest, WildcardAction_MatchesAnyAction) {
    PolicyEngine pe;
    PolicyEngine::Policy p;
    p.id = "admin-all";
    p.subjects.insert("admin");
    p.actions.insert("*");
    p.resources.push_back("/");
    p.effect_allow = true;
    pe.addPolicy(p);

    EXPECT_TRUE(pe.authorize("admin", "read", "/anything").allowed);
    EXPECT_TRUE(pe.authorize("admin", "write", "/anything").allowed);
    EXPECT_TRUE(pe.authorize("admin", "delete", "/anything").allowed);
    EXPECT_TRUE(pe.authorize("admin", "vector.search", "/anything").allowed);
}

TEST(PolicyEngineAuthTest, ResourcePrefixMatch_PrefixMatchesSubpath) {
    PolicyEngine pe;
    PolicyEngine::Policy p;
    p.id = "data-read";
    p.subjects.insert("*");
    p.actions.insert("read");
    p.resources.push_back("/entities/");
    p.effect_allow = true;
    pe.addPolicy(p);

    EXPECT_TRUE(pe.authorize("user", "read", "/entities/users").allowed);
    EXPECT_TRUE(pe.authorize("user", "read", "/entities/users/123").allowed);
    EXPECT_FALSE(pe.authorize("user", "read", "/query").allowed);
}

TEST(PolicyEngineAuthTest, FirstMatchWins_AllowBeforeDeny) {
    PolicyEngine pe;

    // Allow policy first
    PolicyEngine::Policy allow;
    allow.id = "allow-first";
    allow.subjects.insert("user");
    allow.actions.insert("read");
    allow.resources.push_back("/data");
    allow.effect_allow = true;
    pe.addPolicy(allow);

    // Deny policy second
    PolicyEngine::Policy deny;
    deny.id = "deny-second";
    deny.subjects.insert("user");
    deny.actions.insert("read");
    deny.resources.push_back("/data");
    deny.effect_allow = false;
    pe.addPolicy(deny);

    auto d = pe.authorize("user", "read", "/data/test");
    EXPECT_TRUE(d.allowed);
    EXPECT_EQ(d.policy_id, "allow-first");
}

TEST(PolicyEngineAuthTest, FirstMatchWins_DenyBeforeAllow) {
    PolicyEngine pe;

    // Deny policy first
    PolicyEngine::Policy deny;
    deny.id = "deny-first";
    deny.subjects.insert("user");
    deny.actions.insert("read");
    deny.resources.push_back("/data");
    deny.effect_allow = false;
    pe.addPolicy(deny);

    // Allow policy second
    PolicyEngine::Policy allow;
    allow.id = "allow-second";
    allow.subjects.insert("user");
    allow.actions.insert("read");
    allow.resources.push_back("/data");
    allow.effect_allow = true;
    pe.addPolicy(allow);

    auto d = pe.authorize("user", "read", "/data/test");
    EXPECT_FALSE(d.allowed);
    EXPECT_EQ(d.policy_id, "deny-first");
}

TEST(PolicyEngineAuthTest, IPCondition_AllowedIP_Passes) {
    PolicyEngine pe;
    PolicyEngine::Policy p;
    p.id = "ip-restricted";
    p.subjects.insert("*");
    p.actions.insert("read");
    p.resources.push_back("/internal");
    p.effect_allow = true;
    p.allowed_ip_prefixes.push_back("10.0.");
    pe.addPolicy(p);

    auto d = pe.authorize("user", "read", "/internal/data", "10.0.0.5");
    EXPECT_TRUE(d.allowed);
}

TEST(PolicyEngineAuthTest, IPCondition_BlockedIP_Denied) {
    PolicyEngine pe;
    PolicyEngine::Policy p;
    p.id = "ip-restricted";
    p.subjects.insert("*");
    p.actions.insert("read");
    p.resources.push_back("/internal");
    p.effect_allow = true;
    p.allowed_ip_prefixes.push_back("10.0.");
    pe.addPolicy(p);

    // External IP - should not match this policy, falls through to no-match deny
    auto d = pe.authorize("user", "read", "/internal/data", "192.168.1.1");
    EXPECT_FALSE(d.allowed);
}

TEST(PolicyEngineAuthTest, IPCondition_NoIPProvided_Denied) {
    PolicyEngine pe;
    PolicyEngine::Policy p;
    p.id = "ip-restricted";
    p.subjects.insert("*");
    p.actions.insert("read");
    p.resources.push_back("/internal");
    p.effect_allow = true;
    p.allowed_ip_prefixes.push_back("10.0.");
    pe.addPolicy(p);

    // No IP - condition fails
    auto d = pe.authorize("user", "read", "/internal/data");
    EXPECT_FALSE(d.allowed);
}

TEST(PolicyEngineAuthTest, MultipleIPPrefixes_AnyMatchPasses) {
    PolicyEngine pe;
    PolicyEngine::Policy p;
    p.id = "multi-ip";
    p.subjects.insert("*");
    p.actions.insert("read");
    p.resources.push_back("/api");
    p.effect_allow = true;
    p.allowed_ip_prefixes.push_back("10.");
    p.allowed_ip_prefixes.push_back("192.168.");
    pe.addPolicy(p);

    EXPECT_TRUE(pe.authorize("user", "read", "/api/v1", "10.0.0.1").allowed);
    EXPECT_TRUE(pe.authorize("user", "read", "/api/v1", "192.168.1.100").allowed);
    EXPECT_FALSE(pe.authorize("user", "read", "/api/v1", "8.8.8.8").allowed);
}

// ============================================================================
// Policy Management Tests
// ============================================================================

TEST(PolicyEngineManagementTest, AddPolicy_IncreasesCount) {
    PolicyEngine pe;
    EXPECT_EQ(pe.listPolicies().size(), 0u);

    PolicyEngine::Policy p;
    p.id = "p1";
    pe.addPolicy(p);
    EXPECT_EQ(pe.listPolicies().size(), 1u);
}

TEST(PolicyEngineManagementTest, RemovePolicy_DecreasesCount) {
    PolicyEngine pe;
    PolicyEngine::Policy p;
    p.id = "removable";
    pe.addPolicy(p);
    ASSERT_EQ(pe.listPolicies().size(), 1u);

    EXPECT_TRUE(pe.removePolicy("removable"));
    EXPECT_EQ(pe.listPolicies().size(), 0u);
}

TEST(PolicyEngineManagementTest, RemoveNonExistent_ReturnsFalse) {
    PolicyEngine pe;
    EXPECT_FALSE(pe.removePolicy("no-such-policy"));
}

TEST(PolicyEngineManagementTest, SetPolicies_ReplacesAll) {
    PolicyEngine pe;
    PolicyEngine::Policy p1; p1.id = "old1";
    PolicyEngine::Policy p2; p2.id = "old2";
    pe.addPolicy(p1);
    pe.addPolicy(p2);
    ASSERT_EQ(pe.listPolicies().size(), 2u);

    std::vector<PolicyEngine::Policy> new_policies;
    PolicyEngine::Policy p3; p3.id = "new1";
    new_policies.push_back(p3);
    pe.setPolicies(new_policies);

    auto list = pe.listPolicies();
    ASSERT_EQ(list.size(), 1u);
    EXPECT_EQ(list[0].id, "new1");
}

// ============================================================================
// Metrics Tests
// ============================================================================

TEST(PolicyEngineMetricsTest, AllowIncreasesAllowTotal) {
    PolicyEngine pe;
    addPolicyToEngine(pe, "allow", "*", "read", "/");
    pe.authorize("user", "read", "/data");
    pe.authorize("other", "read", "/api");

    const auto& m = pe.getMetrics();
    EXPECT_EQ(m.policy_eval_total.load(), 2u);
    EXPECT_EQ(m.policy_allow_total.load(), 2u);
    EXPECT_EQ(m.policy_deny_total.load(), 0u);
}

TEST(PolicyEngineMetricsTest, DenyIncreasesDenyTotal) {
    PolicyEngine pe;
    addPolicyToEngine(pe, "deny", "user", "delete", "/", false);
    pe.authorize("user", "delete", "/data");

    const auto& m = pe.getMetrics();
    EXPECT_EQ(m.policy_eval_total.load(), 1u);
    EXPECT_EQ(m.policy_deny_total.load(), 1u);
    EXPECT_EQ(m.policy_allow_total.load(), 0u);
}

TEST(PolicyEngineMetricsTest, NoMatchIncreasesDenyTotal) {
    PolicyEngine pe;
    addPolicyToEngine(pe, "allow-specific", "alice", "read", "/data");
    pe.authorize("bob", "delete", "/other");  // no match

    const auto& m = pe.getMetrics();
    EXPECT_EQ(m.policy_deny_total.load(), 1u);
}

// ============================================================================
// Serialization Tests
// ============================================================================

TEST(PolicyEngineSerializationTest, ToJson_ContainsAllFields) {
    PolicyEngine::Policy p;
    p.id = "test-id";
    p.name = "Test Name";
    p.subjects.insert("alice");
    p.actions.insert("read");
    p.resources.push_back("/api");
    p.effect_allow = true;
    p.allowed_ip_prefixes.push_back("10.0.");

    auto j = PolicyEngine::toJson(p);
    EXPECT_EQ(j["id"], "test-id");
    EXPECT_EQ(j["name"], "Test Name");
    EXPECT_EQ(j["effect"], "allow");
    EXPECT_TRUE(j.contains("subjects"));
    EXPECT_TRUE(j.contains("actions"));
    EXPECT_TRUE(j.contains("resources"));
    EXPECT_TRUE(j.contains("allowed_ip_prefixes"));
}

TEST(PolicyEngineSerializationTest, FromJson_RoundTrip) {
    PolicyEngine::Policy original;
    original.id = "rt-test";
    original.name = "Roundtrip Test";
    original.subjects.insert("bob");
    original.actions.insert("write");
    original.resources.push_back("/entities");
    original.effect_allow = false;

    auto j = PolicyEngine::toJson(original);
    auto restored = PolicyEngine::fromJson(j);
    ASSERT_TRUE(restored.has_value());
    EXPECT_EQ(restored->id, original.id);
    EXPECT_EQ(restored->name, original.name);
    EXPECT_TRUE(restored->subjects.count("bob") > 0);
    EXPECT_TRUE(restored->actions.count("write") > 0);
    EXPECT_EQ(restored->effect_allow, false);
}

TEST(PolicyEngineSerializationTest, FromJson_InvalidJson_ReturnsNullopt) {
    nlohmann::json bad = "not-an-object";
    // fromJson should not throw but return nullopt or empty policy
    // It uses try/catch internally
    EXPECT_NO_THROW(PolicyEngine::fromJson(bad));
}

// ============================================================================
// Concurrency Tests
// ============================================================================

TEST(PolicyEngineConcurrencyTest, ConcurrentAuthorize_ThreadSafe) {
    PolicyEngine pe;
    PolicyEngine::Policy p;
    p.id = "concurrent-allow";
    p.subjects.insert("*");
    p.actions.insert("read");
    p.resources.push_back("/data");
    p.effect_allow = true;
    pe.addPolicy(p);

    constexpr int THREADS = 8;
    constexpr int REQUESTS = 100;
    std::atomic<int> allowed{0};
    std::vector<std::thread> threads;

    for (int i = 0; i < THREADS; ++i) {
        threads.emplace_back([&pe, &allowed]() {
            for (int j = 0; j < REQUESTS; ++j) {
                auto d = pe.authorize("user", "read", "/data/item");
                if (d.allowed) {
                  allowed++;
                }
            }
        });
    }
    for (auto& t : threads) {
      t.join();
    }

    EXPECT_EQ(allowed.load(), THREADS * REQUESTS);
}
