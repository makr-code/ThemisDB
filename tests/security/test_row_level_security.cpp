#include <gtest/gtest.h>
#include "security/row_level_security.h"
#include "security/access_control_manager.h"

using namespace themis::security;

// Helper: build a JSON array of order rows
static nlohmann::json makeOrders() {
    return nlohmann::json::array({
        {{"id", 1}, {"owner", "alice"}, {"tenant_id", "acme"}, {"amount", 100}},
        {{"id", 2}, {"owner", "bob"},   {"tenant_id", "acme"}, {"amount", 200}},
        {{"id", 3}, {"owner", "alice"}, {"tenant_id", "beta"}, {"amount", 300}},
        {{"id", 4}, {"owner", "carol"}, {"tenant_id", "beta"}, {"amount", 400}},
    });
}

// Helper: build a SecurityContext
static SecurityContext makeCtx(
    const std::string& user_id,
    const std::vector<std::string>& roles = {},
    const std::unordered_map<std::string, std::string>& attrs = {}
) {
    SecurityContext ctx;
    ctx.user_id    = user_id;
    ctx.roles      = roles;
    ctx.attributes = attrs;
    return ctx;
}

// ============================================================================
// RLSPredicate unit tests
// ============================================================================

TEST(RLSPredicateTest, StaticEqMatch) {
    RLSPredicate pred{"tenant_id", "eq", "\"acme\"", ""};
    nlohmann::json row = {{"tenant_id", "acme"}};
    SecurityContext ctx = makeCtx("alice");
    EXPECT_TRUE(pred.evaluate(row, ctx));
}

TEST(RLSPredicateTest, StaticEqNoMatch) {
    RLSPredicate pred{"tenant_id", "eq", "\"beta\"", ""};
    nlohmann::json row = {{"tenant_id", "acme"}};
    SecurityContext ctx = makeCtx("alice");
    EXPECT_FALSE(pred.evaluate(row, ctx));
}

TEST(RLSPredicateTest, StaticNeMatch) {
    RLSPredicate pred{"tenant_id", "ne", "\"acme\"", ""};
    nlohmann::json row = {{"tenant_id", "beta"}};
    SecurityContext ctx = makeCtx("alice");
    EXPECT_TRUE(pred.evaluate(row, ctx));
}

TEST(RLSPredicateTest, NumericGtMatch) {
    RLSPredicate pred{"amount", "gt", "150", ""};
    nlohmann::json row = {{"amount", 200}};
    SecurityContext ctx = makeCtx("alice");
    EXPECT_TRUE(pred.evaluate(row, ctx));
}

TEST(RLSPredicateTest, NumericLeMatch) {
    RLSPredicate pred{"amount", "le", "100", ""};
    nlohmann::json row = {{"amount", 100}};
    SecurityContext ctx = makeCtx("alice");
    EXPECT_TRUE(pred.evaluate(row, ctx));
}

TEST(RLSPredicateTest, InOperatorMatch) {
    RLSPredicate pred{"tenant_id", "in", "[\"acme\",\"beta\"]", ""};
    nlohmann::json row = {{"tenant_id", "acme"}};
    SecurityContext ctx = makeCtx("alice");
    EXPECT_TRUE(pred.evaluate(row, ctx));
}

TEST(RLSPredicateTest, InOperatorNoMatch) {
    RLSPredicate pred{"tenant_id", "in", "[\"acme\",\"beta\"]", ""};
    nlohmann::json row = {{"tenant_id", "gamma"}};
    SecurityContext ctx = makeCtx("alice");
    EXPECT_FALSE(pred.evaluate(row, ctx));
}

TEST(RLSPredicateTest, UserAttrMatch) {
    // owner == ctx.user_id
    RLSPredicate pred{"owner", "eq", "", "user_id"};
    nlohmann::json row = {{"owner", "alice"}};
    SecurityContext ctx = makeCtx("alice");
    EXPECT_TRUE(pred.evaluate(row, ctx));
}

TEST(RLSPredicateTest, UserAttrNoMatch) {
    RLSPredicate pred{"owner", "eq", "", "user_id"};
    nlohmann::json row = {{"owner", "bob"}};
    SecurityContext ctx = makeCtx("alice");
    EXPECT_FALSE(pred.evaluate(row, ctx));
}

TEST(RLSPredicateTest, UserCustomAttrMatch) {
    RLSPredicate pred{"tenant_id", "eq", "", "tenant"};
    nlohmann::json row = {{"tenant_id", "acme"}};
    SecurityContext ctx = makeCtx("alice", {}, {{"tenant", "acme"}});
    EXPECT_TRUE(pred.evaluate(row, ctx));
}

TEST(RLSPredicateTest, MissingFieldReturnsFalse) {
    RLSPredicate pred{"nonexistent_field", "eq", "\"x\"", ""};
    nlohmann::json row = {{"other", "x"}};
    SecurityContext ctx = makeCtx("alice");
    EXPECT_FALSE(pred.evaluate(row, ctx));
}

TEST(RLSPredicateTest, Serialisation) {
    RLSPredicate orig{"owner", "eq", "", "user_id"};
    nlohmann::json j = orig.toJson();
    RLSPredicate loaded = RLSPredicate::fromJson(j);
    EXPECT_EQ(loaded.field,     orig.field);
    EXPECT_EQ(loaded.op,        orig.op);
    EXPECT_EQ(loaded.value,     orig.value);
    EXPECT_EQ(loaded.user_attr, orig.user_attr);
}

// ============================================================================
// RLSPolicy serialisation
// ============================================================================

TEST(RLSPolicyTest, Serialisation) {
    RLSPolicy orig;
    orig.id               = "p1";
    orig.collection       = "orders";
    orig.applicable_roles = {"analyst"};
    orig.predicate        = {"owner", "eq", "", "user_id"};
    orig.type             = RLSPolicyType::RESTRICTIVE;
    orig.enabled          = false;

    nlohmann::json j = orig.toJson();
    RLSPolicy loaded = RLSPolicy::fromJson(j);

    EXPECT_EQ(loaded.id,               orig.id);
    EXPECT_EQ(loaded.collection,       orig.collection);
    EXPECT_EQ(loaded.applicable_roles, orig.applicable_roles);
    EXPECT_EQ(loaded.predicate.field,  orig.predicate.field);
    EXPECT_EQ(loaded.type,             RLSPolicyType::RESTRICTIVE);
    EXPECT_FALSE(loaded.enabled);
}

// ============================================================================
// RLSManager – policy management
// ============================================================================

TEST(RLSManagerTest, AddAndRetrievePolicy) {
    RLSManager rls;
    RLSPolicy p;
    p.id         = "tenant_isolation";
    p.collection = "orders";
    p.predicate  = {"tenant_id", "eq", "\"acme\"", ""};

    rls.addPolicy(p);
    auto fetched = rls.getPolicy("tenant_isolation");
    ASSERT_TRUE(fetched.has_value());
    EXPECT_EQ(fetched->id,         "tenant_isolation");
    EXPECT_EQ(fetched->collection, "orders");
}

TEST(RLSManagerTest, RemovePolicy) {
    RLSManager rls;
    RLSPolicy p;
    p.id         = "p1";
    p.collection = "orders";
    p.predicate  = {"owner", "eq", "\"alice\"", ""};

    rls.addPolicy(p);
    EXPECT_TRUE(rls.removePolicy("p1"));
    EXPECT_FALSE(rls.getPolicy("p1").has_value());
    EXPECT_FALSE(rls.removePolicy("p1"));  // Already gone
}

TEST(RLSManagerTest, ListPolicies) {
    RLSManager rls;
    for (int i = 1; i <= 3; ++i) {
        RLSPolicy p;
        p.id         = "p" + std::to_string(i);
        p.collection = "col";
        p.predicate  = {"f", "eq", "\"v\"", ""};
        rls.addPolicy(p);
    }
    auto ids = rls.listPolicies();
    EXPECT_EQ(ids.size(), 3u);
}

TEST(RLSManagerTest, ClearAllPolicies) {
    RLSManager rls;
    for (int i = 0; i < 5; ++i) {
        RLSPolicy p;
        p.id        = "p" + std::to_string(i);
        p.predicate = {"f", "eq", "\"v\"", ""};
        rls.addPolicy(p);
    }
    rls.clearAllPolicies();
    EXPECT_TRUE(rls.listPolicies().empty());
}

TEST(RLSManagerTest, LoadFromJson) {
    RLSManager rls;
    nlohmann::json j = {
        {"policies", {
            {{"id","p1"},{"collection","orders"},{"applicable_roles",nlohmann::json::array()},
             {"predicate",{{"field","owner"},{"op","eq"},{"value",""},{"user_attr","user_id"}}},
             {"type","permissive"},{"enabled",true}},
            {{"id","p2"},{"collection","users"},{"applicable_roles",{"admin"}},
             {"predicate",{{"field","active"},{"op","eq"},{"value","true"},{"user_attr",""}}},
             {"type","restrictive"},{"enabled",true}}
        }}
    };

    size_t loaded = rls.loadFromJson(j);
    EXPECT_EQ(loaded, 2u);
    EXPECT_TRUE(rls.getPolicy("p1").has_value());
    EXPECT_TRUE(rls.getPolicy("p2").has_value());
}

TEST(RLSManagerTest, ToJson) {
    RLSManager rls;
    RLSPolicy p;
    p.id         = "p1";
    p.collection = "orders";
    p.predicate  = {"owner", "eq", "", "user_id"};
    rls.addPolicy(p);

    nlohmann::json j = rls.toJson();
    ASSERT_TRUE(j.contains("policies"));
    EXPECT_EQ(j["policies"].size(), 1u);
}

// ============================================================================
// RLSManager – row filtering: no policy (pass-through)
// ============================================================================

TEST(RLSManagerFilterTest, NoPoliciesPassAll) {
    RLSManager rls;
    SecurityContext ctx = makeCtx("alice", {"analyst"});
    auto rows    = makeOrders();
    auto result  = rls.filterRows("orders", ctx, rows);
    EXPECT_EQ(result.size(), 4u);
}

TEST(RLSManagerFilterTest, DisabledPolicyPassesAll) {
    RLSManager rls;
    RLSPolicy p;
    p.id         = "p1";
    p.collection = "orders";
    p.predicate  = {"owner", "eq", "", "user_id"};
    p.enabled    = false;
    rls.addPolicy(p);

    SecurityContext ctx = makeCtx("alice");
    auto result = rls.filterRows("orders", ctx, makeOrders());
    EXPECT_EQ(result.size(), 4u);
}

// ============================================================================
// RLSManager – row filtering: role matching
// ============================================================================

TEST(RLSManagerFilterTest, RoleNotMatchedPassesAll) {
    // Policy applies only to "analyst" role; user has "admin" role → no RLS.
    RLSManager rls;
    RLSPolicy p;
    p.id               = "p1";
    p.collection       = "orders";
    p.applicable_roles = {"analyst"};
    p.predicate        = {"owner", "eq", "", "user_id"};
    rls.addPolicy(p);

    SecurityContext ctx = makeCtx("alice", {"admin"});
    auto result = rls.filterRows("orders", ctx, makeOrders());
    EXPECT_EQ(result.size(), 4u);
}

TEST(RLSManagerFilterTest, RoleMatchedFiltersRows) {
    // Policy applies to "analyst"; alice has "analyst" → only alice's rows visible.
    RLSManager rls;
    RLSPolicy p;
    p.id               = "p1";
    p.collection       = "orders";
    p.applicable_roles = {"analyst"};
    p.predicate        = {"owner", "eq", "", "user_id"};  // owner == ctx.user_id
    rls.addPolicy(p);

    SecurityContext ctx = makeCtx("alice", {"analyst"});
    auto result = rls.filterRows("orders", ctx, makeOrders());
    // Rows 1 and 3 have owner=="alice"
    EXPECT_EQ(result.size(), 2u);
    for (const auto& row : result) {
        EXPECT_EQ(row["owner"].get<std::string>(), "alice");
    }
}

// ============================================================================
// RLSManager – row filtering: static value predicate
// ============================================================================

TEST(RLSManagerFilterTest, StaticTenantIsolation) {
    RLSManager rls;
    RLSPolicy p;
    p.id         = "tenant_iso";
    p.collection = "orders";
    p.predicate  = {"tenant_id", "eq", "\"acme\"", ""};
    rls.addPolicy(p);

    SecurityContext ctx = makeCtx("alice");
    auto result = rls.filterRows("orders", ctx, makeOrders());
    // Only rows with tenant_id=="acme" (rows 1 and 2)
    EXPECT_EQ(result.size(), 2u);
    for (const auto& row : result) {
        EXPECT_EQ(row["tenant_id"].get<std::string>(), "acme");
    }
}

// ============================================================================
// RLSManager – row filtering: PERMISSIVE vs RESTRICTIVE
// ============================================================================

TEST(RLSManagerFilterTest, PermissiveUnionSemantics) {
    // Two permissive policies: rows visible if owner==alice OR tenant_id==beta
    RLSManager rls;
    {
        RLSPolicy p;
        p.id         = "p1";
        p.collection = "orders";
        p.predicate  = {"owner", "eq", "", "user_id"};
        p.type       = RLSPolicyType::PERMISSIVE;
        rls.addPolicy(p);
    }
    {
        RLSPolicy p;
        p.id         = "p2";
        p.collection = "orders";
        p.predicate  = {"tenant_id", "eq", "\"beta\"", ""};
        p.type       = RLSPolicyType::PERMISSIVE;
        rls.addPolicy(p);
    }

    SecurityContext ctx = makeCtx("alice");
    auto result = rls.filterRows("orders", ctx, makeOrders());
    // Row 1: owner==alice ✓
    // Row 2: owner==bob ✗, tenant==acme ✗  → filtered
    // Row 3: owner==alice ✓
    // Row 4: tenant==beta ✓
    EXPECT_EQ(result.size(), 3u);
}

TEST(RLSManagerFilterTest, RestrictiveIntersectionSemantics) {
    // Two restrictive policies: rows visible if owner==alice AND tenant_id==acme
    RLSManager rls;
    {
        RLSPolicy p;
        p.id         = "p1";
        p.collection = "orders";
        p.predicate  = {"owner", "eq", "", "user_id"};
        p.type       = RLSPolicyType::RESTRICTIVE;
        rls.addPolicy(p);
    }
    {
        RLSPolicy p;
        p.id         = "p2";
        p.collection = "orders";
        p.predicate  = {"tenant_id", "eq", "\"acme\"", ""};
        p.type       = RLSPolicyType::RESTRICTIVE;
        rls.addPolicy(p);
    }

    SecurityContext ctx = makeCtx("alice");
    auto result = rls.filterRows("orders", ctx, makeOrders());
    // Only row 1 satisfies both conditions (owner==alice AND tenant==acme)
    EXPECT_EQ(result.size(), 1u);
    EXPECT_EQ(result[0]["id"].get<int>(), 1);
}

TEST(RLSManagerFilterTest, MixedPermissiveAndRestrictive) {
    // Permissive: owner==alice  (OR semantics)
    // Restrictive: tenant_id==acme  (AND semantics)
    // Row visible if (owner==alice) AND (tenant_id==acme)
    RLSManager rls;
    {
        RLSPolicy p;
        p.id         = "perm1";
        p.collection = "orders";
        p.predicate  = {"owner", "eq", "", "user_id"};
        p.type       = RLSPolicyType::PERMISSIVE;
        rls.addPolicy(p);
    }
    {
        RLSPolicy p;
        p.id         = "rest1";
        p.collection = "orders";
        p.predicate  = {"tenant_id", "eq", "\"acme\"", ""};
        p.type       = RLSPolicyType::RESTRICTIVE;
        rls.addPolicy(p);
    }

    SecurityContext ctx = makeCtx("alice");
    auto result = rls.filterRows("orders", ctx, makeOrders());
    EXPECT_EQ(result.size(), 1u);
    EXPECT_EQ(result[0]["id"].get<int>(), 1);
}

// ============================================================================
// RLSManager – global policies (empty collection)
// ============================================================================

TEST(RLSManagerFilterTest, GlobalPolicyAppliesToAllCollections) {
    RLSManager rls;
    RLSPolicy p;
    p.id         = "global_owner";
    p.collection = "";  // empty = all collections
    p.predicate  = {"owner", "eq", "", "user_id"};
    rls.addPolicy(p);

    SecurityContext ctx = makeCtx("bob");
    // Apply to "orders" collection
    auto result = rls.filterRows("orders", ctx, makeOrders());
    EXPECT_EQ(result.size(), 1u);
    EXPECT_EQ(result[0]["owner"].get<std::string>(), "bob");
}

// ============================================================================
// RLSManager – isActive
// ============================================================================

TEST(RLSManagerTest, IsActiveTrueWhenPolicyMatches) {
    RLSManager rls;
    RLSPolicy p;
    p.id               = "p1";
    p.collection       = "orders";
    p.applicable_roles = {"analyst"};
    p.predicate        = {"owner", "eq", "", "user_id"};
    rls.addPolicy(p);

    SecurityContext ctx = makeCtx("alice", {"analyst"});
    EXPECT_TRUE(rls.isActive("orders", ctx));
}

TEST(RLSManagerTest, IsActiveFalseWhenNoMatchingPolicy) {
    RLSManager rls;
    RLSPolicy p;
    p.id               = "p1";
    p.collection       = "orders";
    p.applicable_roles = {"analyst"};
    p.predicate        = {"owner", "eq", "", "user_id"};
    rls.addPolicy(p);

    SecurityContext ctx = makeCtx("alice", {"admin"});  // not analyst
    EXPECT_FALSE(rls.isActive("orders", ctx));
}

TEST(RLSManagerTest, IsActiveFalseWhenDisabled) {
    RLSManager rls;
    RLSPolicy p;
    p.id         = "p1";
    p.collection = "orders";
    p.enabled    = false;
    p.predicate  = {"owner", "eq", "", "user_id"};
    rls.addPolicy(p);

    SecurityContext ctx = makeCtx("alice");
    EXPECT_FALSE(rls.isActive("orders", ctx));
}

// ============================================================================
// RLSManager – non-array rows pass through unchanged
// ============================================================================

TEST(RLSManagerFilterTest, NonArrayRowsPassThrough) {
    RLSManager rls;
    RLSPolicy p;
    p.id         = "p1";
    p.collection = "orders";
    p.predicate  = {"owner", "eq", "", "user_id"};
    rls.addPolicy(p);

    SecurityContext ctx = makeCtx("alice");
    nlohmann::json not_an_array = {{"key", "value"}};
    auto result = rls.filterRows("orders", ctx, not_an_array);
    // Object passthrough – unchanged
    EXPECT_EQ(result, not_an_array);
}

// ============================================================================
// RLSManager – addPolicy rejects empty id
// ============================================================================

TEST(RLSManagerTest, AddPolicyRejectsEmptyId) {
    RLSManager rls;
    RLSPolicy p;
    p.id        = "";
    p.predicate = {"owner", "eq", "\"alice\"", ""};
    EXPECT_THROW(rls.addPolicy(p), std::invalid_argument);
}
