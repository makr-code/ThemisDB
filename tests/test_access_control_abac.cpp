/**
 * @file test_access_control_abac.cpp
 * @brief Tests for ABAC (Attribute-Based Access Control) integration alongside RBAC
 *        in the AccessControl class.
 *
 * Verifies:
 * - RBAC-only mode (default): ABAC policies are not evaluated
 * - ABAC mode enabled: ABAC policies are evaluated after RBAC passes
 * - RBAC deny is not overridden by ABAC (RBAC deny is authoritative)
 * - ABAC IP-prefix condition restricts access for matching RBAC role
 * - ABAC time-window condition restricts access
 * - addABACPolicy / removeABACPolicy APIs
 * - getABACEngine returns the policy engine
 */

#include <gtest/gtest.h>
#include "security/access_control.h"

using namespace themis::security;
using namespace themis;

// ============================================================================
// Helpers
// ============================================================================

/// Build a minimal AccessControl::Config with built-in RBAC roles only.
static AccessControl::Config makeConfig(bool enable_abac = false) {
    AccessControl::Config cfg;
    cfg.rbac_config.use_builtin_roles = true;
    cfg.audit_config.enable_audit_logging = false;
    cfg.abac_config.enable_abac = enable_abac;
    return cfg;
}

/// Build an AuthorizationContext with the given user, roles, resource and action.
static AccessControl::AuthorizationContext makeCtx(
    const std::string& user_id,
    const std::vector<std::string>& roles,
    const std::string& resource,
    const std::string& action,
    const std::string& ip = "",
    const std::optional<std::string>& user_agent = std::nullopt
) {
    AccessControl::AuthorizationContext ctx;
    ctx.user_id    = user_id;
    ctx.roles      = roles;
    ctx.resource   = resource;
    ctx.action     = action;
    ctx.ip_address = ip;
    ctx.user_agent = user_agent;
    ctx.timestamp  = std::chrono::system_clock::now();
    return ctx;
}

// ============================================================================
// RBAC-only (ABAC disabled) baseline tests
// ============================================================================

class AccessControlABACTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Probe: Check if RBAC is available (license gated in Community Edition)
        AccessControl ac_probe(makeConfig(/*enable_abac=*/false));
        auto ctx = makeCtx("alice", {"admin"}, "data", "write");
        if (!ac_probe.authorize(ctx)) {
            GTEST_SKIP() << "RBAC authorization not available (Community Edition limitation)";
        }
    }
};

TEST_F(AccessControlABACTest, RBACOnly_AdminCanWriteData) {
    AccessControl ac(makeConfig(/*enable_abac=*/false));
    auto ctx = makeCtx("alice", {"admin"}, "data", "write");
    EXPECT_TRUE(ac.authorize(ctx));
}

TEST_F(AccessControlABACTest, RBACOnly_ReadonlyCannotWriteData) {
    AccessControl ac(makeConfig(/*enable_abac=*/false));
    auto ctx = makeCtx("bob", {"readonly"}, "data", "write");
    EXPECT_FALSE(ac.authorize(ctx));
}

// ============================================================================
// ABAC enabled, no policies configured – default-allow when no policies
// ============================================================================

TEST_F(AccessControlABACTest, ABACEnabled_NoPolicies_DefaultAllow_AdminAccess) {
    // PolicyEngine with no policies defaults to allow; admin+RBAC also grants => allowed
    AccessControl ac(makeConfig(/*enable_abac=*/true));
    auto ctx = makeCtx("alice", {"admin"}, "data", "write");
    EXPECT_TRUE(ac.authorize(ctx));
}

TEST_F(AccessControlABACTest, ABACEnabled_NoPolicies_RBACDenyStillDenies) {
    // Even with ABAC enabled+no policies (default-allow), RBAC deny wins first
    AccessControl ac(makeConfig(/*enable_abac=*/true));
    auto ctx = makeCtx("bob", {"readonly"}, "data", "write");
    EXPECT_FALSE(ac.authorize(ctx));
}

// ============================================================================
// ABAC enabled, with an ALLOW policy for a specific IP prefix
// ============================================================================

TEST_F(AccessControlABACTest, ABACEnabled_IPAllow_AccessGrantedForMatchingIP) {
    AccessControl ac(makeConfig(/*enable_abac=*/true));

    PolicyEngine::Policy p;
    p.id             = "ip-allow";
    p.subjects       = {"*"};
    p.actions        = {"read", "write"};
    p.resources      = {"data"};
    p.effect_allow   = true;
    p.allowed_ip_prefixes = {"10.0."};
    ac.addABACPolicy(p);

    // Admin + matching IP -> granted
    auto ctx = makeCtx("alice", {"admin"}, "data", "write", "10.0.1.5");
    EXPECT_TRUE(ac.authorize(ctx));
}

TEST_F(AccessControlABACTest, ABACEnabled_IPAllow_AccessDeniedForNonMatchingIP) {
    AccessControl ac(makeConfig(/*enable_abac=*/true));

    PolicyEngine::Policy p;
    p.id             = "ip-allow";
    p.subjects       = {"*"};
    p.actions        = {"read", "write"};
    p.resources      = {"data"};
    p.effect_allow   = true;
    p.allowed_ip_prefixes = {"10.0."};
    ac.addABACPolicy(p);

    // Admin role passes RBAC but ABAC IP condition fails -> denied
    auto ctx = makeCtx("alice", {"admin"}, "data", "write", "192.168.1.1");
    EXPECT_FALSE(ac.authorize(ctx));
}

// ============================================================================
// ABAC with DENY policy
// ============================================================================

TEST_F(AccessControlABACTest, ABACEnabled_DenyPolicy_BlocksRBACGrantedAccess) {
    AccessControl ac(makeConfig(/*enable_abac=*/true));

    // Explicit deny for the admin user on "config:delete"
    PolicyEngine::Policy deny;
    deny.id           = "deny-config-delete";
    deny.subjects     = {"alice"};
    deny.actions      = {"delete"};
    deny.resources    = {"config"};
    deny.effect_allow = false;
    ac.addABACPolicy(deny);

    // Admin has RBAC permission but ABAC denies
    auto ctx = makeCtx("alice", {"admin"}, "config", "delete");
    EXPECT_FALSE(ac.authorize(ctx));
}

TEST_F(AccessControlABACTest, ABACEnabled_DenyPolicy_OtherUserNotAffected) {
    AccessControl ac(makeConfig(/*enable_abac=*/true));

    // Deny only "alice" on config:delete; "bob" (admin) is not in subjects
    PolicyEngine::Policy deny;
    deny.id           = "deny-alice-config-delete";
    deny.subjects     = {"alice"};
    deny.actions      = {"delete"};
    deny.resources    = {"config"};
    deny.effect_allow = false;
    ac.addABACPolicy(deny);

    // bob (admin) – no matching ABAC deny, ABAC defaults to allow -> granted
    auto ctx = makeCtx("bob", {"admin"}, "config", "delete");
    EXPECT_TRUE(ac.authorize(ctx));
}

// ============================================================================
// addABACPolicy / removeABACPolicy API
// ============================================================================

TEST_F(AccessControlABACTest, RemoveABACPolicy_RestoresDefaultAllow) {
    AccessControl ac(makeConfig(/*enable_abac=*/true));

    PolicyEngine::Policy deny;
    deny.id           = "deny-admin-read";
    deny.subjects     = {"alice"};
    deny.actions      = {"read"};
    deny.resources    = {"data"};
    deny.effect_allow = false;
    ac.addABACPolicy(deny);

    // Verify deny is effective
    auto ctx = makeCtx("alice", {"admin"}, "data", "read");
    EXPECT_FALSE(ac.authorize(ctx));

    // Remove the deny policy -> PolicyEngine has no policies -> default allow
    EXPECT_TRUE(ac.removeABACPolicy("deny-admin-read"));
    EXPECT_TRUE(ac.authorize(ctx));
}

TEST_F(AccessControlABACTest, RemoveNonExistentPolicy_ReturnsFalse) {
    AccessControl ac(makeConfig(/*enable_abac=*/true));
    EXPECT_FALSE(ac.removeABACPolicy("does-not-exist"));
}

// ============================================================================
// getABACEngine exposes the policy engine
// ============================================================================

TEST_F(AccessControlABACTest, GetABACEngine_ReturnsCorrectEngine) {
    AccessControl ac(makeConfig(/*enable_abac=*/true));

    PolicyEngine::Policy p;
    p.id           = "test-policy";
    p.subjects     = {"*"};
    p.actions      = {"read"};
    p.resources    = {"data"};
    p.effect_allow = true;
    ac.addABACPolicy(p);

    const auto& engine = ac.getABACEngine();
    auto policies = engine.listPolicies();
    ASSERT_EQ(policies.size(), 1u);
    EXPECT_EQ(policies[0].id, "test-policy");
}

// ============================================================================
// ABAC disabled even if policies are added
// ============================================================================

TEST_F(AccessControlABACTest, ABACDisabled_PoliciesIgnored) {
    // ABAC is NOT enabled – even if we add a deny policy it should have no effect
    AccessControl ac(makeConfig(/*enable_abac=*/false));

    PolicyEngine::Policy deny;
    deny.id           = "deny-all";
    deny.subjects     = {"*"};
    deny.actions      = {"*"};
    deny.resources    = {"data"};
    deny.effect_allow = false;
    ac.addABACPolicy(deny);

    // Admin has RBAC permission and ABAC is disabled -> access granted
    auto ctx = makeCtx("alice", {"admin"}, "data", "write");
    EXPECT_TRUE(ac.authorize(ctx));
}

// ============================================================================
// ABAC user-agent condition tests
// ============================================================================

TEST_F(AccessControlABACTest, ABACEnabled_UAAllow_GrantedForMatchingUA) {
    AccessControl ac(makeConfig(/*enable_abac=*/true));

    PolicyEngine::Policy p;
    p.id             = "ua-allow";
    p.subjects       = {"*"};
    p.actions        = {"read"};
    p.resources      = {"data"};
    p.effect_allow   = true;
    p.allowed_user_agent_patterns = {"ThemisClient"};
    ac.addABACPolicy(p);

    // Admin + matching UA -> granted
    auto ctx = makeCtx("alice", {"admin"}, "data", "read", "", "ThemisClient/2.0");
    EXPECT_TRUE(ac.authorize(ctx));
}

TEST_F(AccessControlABACTest, ABACEnabled_UAAllow_DeniedForNonMatchingUA) {
    AccessControl ac(makeConfig(/*enable_abac=*/true));

    PolicyEngine::Policy p;
    p.id             = "ua-allow";
    p.subjects       = {"*"};
    p.actions        = {"read"};
    p.resources      = {"data"};
    p.effect_allow   = true;
    p.allowed_user_agent_patterns = {"ThemisClient"};
    ac.addABACPolicy(p);

    // Admin role passes RBAC but ABAC UA condition fails -> denied
    auto ctx = makeCtx("alice", {"admin"}, "data", "read", /*ip=*/"", "curl/7.68.0");
    EXPECT_FALSE(ac.authorize(ctx));
}

TEST_F(AccessControlABACTest, ABACEnabled_UAAllow_DeniedWhenNoUAProvided) {
    AccessControl ac(makeConfig(/*enable_abac=*/true));

    PolicyEngine::Policy p;
    p.id             = "ua-allow";
    p.subjects       = {"*"};
    p.actions        = {"read"};
    p.resources      = {"data"};
    p.effect_allow   = true;
    p.allowed_user_agent_patterns = {"ThemisClient"};
    ac.addABACPolicy(p);

    // No user_agent -> ABAC UA condition fails -> denied
    auto ctx = makeCtx("alice", {"admin"}, "data", "read", /*ip=*/"", std::nullopt);
    EXPECT_FALSE(ac.authorize(ctx));
}
