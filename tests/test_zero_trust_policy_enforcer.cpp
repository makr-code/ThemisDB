/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            test_zero_trust_policy_enforcer.cpp                ║
  Version:         0.0.7                                              ║
  Last Modified:   2026-04-13 20:53:55                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     389                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
    • 72c5fa5ba9  2026-02-23  feat(security): implement zero-trust network policy enfor... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include <gtest/gtest.h>
#include "security/zero_trust_policy_enforcer.h"

using namespace themis::security;

// ============================================================================
// Helper: build a minimal ZeroTrustContext
// ============================================================================

static ZeroTrustContext makeCtx(const std::string& user_id,
                                 const std::string& client_ip,
                                 const std::string& token = "valid-token",
                                 const std::string& resource = "data",
                                 const std::string& action = "read") {
    ZeroTrustContext ctx;
    ctx.request_id = "req-test";
    ctx.user_id    = user_id;
    ctx.client_ip  = client_ip;
    ctx.token      = token;
    ctx.resource   = resource;
    ctx.action     = action;
    ctx.timestamp  = std::chrono::system_clock::now();
    return ctx;
}

// ============================================================================
// Policy management
// ============================================================================

TEST(ZeroTrustPolicyEnforcerTest, AddAndRemovePolicy) {
    ZeroTrustPolicyEnforcer enforcer;

    NetworkPolicy p;
    p.policy_id = "p1";
    p.identity  = "alice";
    p.allowed_cidrs = {"10.0.0.0/8"};
    p.default_deny  = true;

    enforcer.addNetworkPolicy(p);
    EXPECT_EQ(enforcer.getNetworkPolicies().size(), 1u);

    EXPECT_TRUE(enforcer.removeNetworkPolicy("p1"));
    EXPECT_EQ(enforcer.getNetworkPolicies().size(), 0u);

    // Removing non-existent policy returns false
    EXPECT_FALSE(enforcer.removeNetworkPolicy("p1"));
}

TEST(ZeroTrustPolicyEnforcerTest, ReplacePolicyWithSameId) {
    ZeroTrustPolicyEnforcer enforcer;

    NetworkPolicy p1;
    p1.policy_id = "p1";
    p1.identity  = "alice";
    p1.allowed_cidrs = {"10.0.0.0/8"};
    enforcer.addNetworkPolicy(p1);

    NetworkPolicy p2;
    p2.policy_id = "p1";
    p2.identity  = "bob";
    p2.allowed_cidrs = {"192.168.0.0/16"};
    enforcer.addNetworkPolicy(p2);

    // Should still have exactly one policy
    EXPECT_EQ(enforcer.getNetworkPolicies().size(), 1u);
    EXPECT_EQ(enforcer.getNetworkPolicies()[0].identity, "bob");
}

// ============================================================================
// IP / CIDR matching
// ============================================================================

TEST(ZeroTrustPolicyEnforcerTest, IpAllowedInCidr) {
    ZeroTrustPolicyEnforcer enforcer;
    NetworkPolicy p;
    p.policy_id = "p1";
    p.identity  = "alice";
    p.allowed_cidrs = {"10.0.0.0/8"};
    p.default_deny  = true;
    enforcer.addNetworkPolicy(p);

    EXPECT_TRUE(enforcer.isIpAllowed("10.1.2.3", "alice"));
    EXPECT_TRUE(enforcer.isIpAllowed("10.255.255.255", "alice"));
}

TEST(ZeroTrustPolicyEnforcerTest, IpDeniedOutsideCidr) {
    ZeroTrustPolicyEnforcer enforcer;
    NetworkPolicy p;
    p.policy_id = "p1";
    p.identity  = "alice";
    p.allowed_cidrs = {"10.0.0.0/8"};
    p.default_deny  = true;
    enforcer.addNetworkPolicy(p);

    EXPECT_FALSE(enforcer.isIpAllowed("192.168.1.1", "alice"));
    EXPECT_FALSE(enforcer.isIpAllowed("172.16.0.1", "alice"));
}

TEST(ZeroTrustPolicyEnforcerTest, DeniedCidrTakesPrecedence) {
    ZeroTrustPolicyEnforcer enforcer;
    NetworkPolicy p;
    p.policy_id = "p1";
    p.identity  = "alice";
    p.allowed_cidrs = {"10.0.0.0/8"};
    p.denied_cidrs  = {"10.0.0.0/24"};  // block a sub-net
    p.default_deny  = true;
    enforcer.addNetworkPolicy(p);

    // IP inside blocked subnet → denied even though /8 allows it
    EXPECT_FALSE(enforcer.isIpAllowed("10.0.0.5", "alice"));
    // IP outside blocked subnet but inside allowed /8 → allowed
    EXPECT_TRUE(enforcer.isIpAllowed("10.1.0.5", "alice"));
}

TEST(ZeroTrustPolicyEnforcerTest, DefaultDenyFalseAllowsUnmatchedIp) {
    ZeroTrustPolicyEnforcer enforcer;
    NetworkPolicy p;
    p.policy_id = "p1";
    p.identity  = "alice";
    p.allowed_cidrs = {"10.0.0.0/8"};
    p.default_deny  = false;
    enforcer.addNetworkPolicy(p);

    // Outside allowed CIDR but default_deny=false → allowed
    EXPECT_TRUE(enforcer.isIpAllowed("192.168.1.1", "alice"));
}

TEST(ZeroTrustPolicyEnforcerTest, NoPoliciesAllowsAll) {
    // When no policies are registered there is nothing to check, so allow.
    ZeroTrustPolicyEnforcer enforcer;
    EXPECT_TRUE(enforcer.isIpAllowed("1.2.3.4", "unknown-user"));
}

TEST(ZeroTrustPolicyEnforcerTest, PoliciesRegisteredButNoneMatchIdentityDenies) {
    ZeroTrustPolicyEnforcer enforcer;
    NetworkPolicy p;
    p.policy_id = "p1";
    p.identity  = "alice";
    p.allowed_cidrs = {"10.0.0.0/8"};
    p.default_deny  = true;
    enforcer.addNetworkPolicy(p);

    // "bob" has no policy → default deny when other policies exist
    EXPECT_FALSE(enforcer.isIpAllowed("10.1.2.3", "bob"));
}

TEST(ZeroTrustPolicyEnforcerTest, ExactIpCidr) {
    ZeroTrustPolicyEnforcer enforcer;
    NetworkPolicy p;
    p.policy_id = "p1";
    p.identity  = "alice";
    p.allowed_cidrs = {"192.168.1.100/32"};
    p.default_deny  = true;
    enforcer.addNetworkPolicy(p);

    EXPECT_TRUE(enforcer.isIpAllowed("192.168.1.100", "alice"));
    EXPECT_FALSE(enforcer.isIpAllowed("192.168.1.101", "alice"));
}

TEST(ZeroTrustPolicyEnforcerTest, WildcardCidrZeroPrefix) {
    ZeroTrustPolicyEnforcer enforcer;
    NetworkPolicy p;
    p.policy_id = "p1";
    p.identity  = "alice";
    p.allowed_cidrs = {"0.0.0.0/0"};
    p.default_deny  = true;
    enforcer.addNetworkPolicy(p);

    EXPECT_TRUE(enforcer.isIpAllowed("1.2.3.4", "alice"));
    EXPECT_TRUE(enforcer.isIpAllowed("255.255.255.255", "alice"));
}

// ============================================================================
// Token verification
// ============================================================================

TEST(ZeroTrustPolicyEnforcerTest, NoVerifierPassesAll) {
    ZeroTrustPolicyEnforcer enforcer; // no token_verifier
    EXPECT_TRUE(enforcer.verifyToken("any-token", "any-user"));
}

TEST(ZeroTrustPolicyEnforcerTest, VerifierCalledWithCorrectArgs) {
    std::string captured_token, captured_user;
    auto verifier = [&](const std::string& tok, const std::string& uid) {
        captured_token = tok;
        captured_user  = uid;
        return true;
    };

    ZeroTrustPolicyEnforcer enforcer(verifier);
    EXPECT_TRUE(enforcer.verifyToken("my-token", "alice"));
    EXPECT_EQ(captured_token, "my-token");
    EXPECT_EQ(captured_user, "alice");
}

TEST(ZeroTrustPolicyEnforcerTest, VerifierReturnsFalseOnBadToken) {
    auto verifier = [](const std::string& tok, const std::string&) {
        return tok == "valid";
    };
    ZeroTrustPolicyEnforcer enforcer(verifier);
    EXPECT_TRUE(enforcer.verifyToken("valid", "alice"));
    EXPECT_FALSE(enforcer.verifyToken("invalid", "alice"));
}

// ============================================================================
// Trust score
// ============================================================================

TEST(ZeroTrustPolicyEnforcerTest, TrustScoreMaxWhenAllSignalsOk) {
    ZeroTrustPolicyEnforcer enforcer;
    ZeroTrustContext ctx = makeCtx("alice", "10.1.2.3");
    ctx.device_id  = "device-abc";
    ctx.timestamp  = std::chrono::system_clock::now();

    double score = enforcer.computeTrustScore(ctx, /*identity_ok=*/true, /*network_ok=*/true);
    EXPECT_DOUBLE_EQ(score, 1.0);
}

TEST(ZeroTrustPolicyEnforcerTest, TrustScoreReducedWithoutDevice) {
    ZeroTrustPolicyEnforcer enforcer;
    ZeroTrustContext ctx = makeCtx("alice", "10.1.2.3");
    // No device_id
    ctx.timestamp = std::chrono::system_clock::now();

    double score = enforcer.computeTrustScore(ctx, /*identity_ok=*/true, /*network_ok=*/true);
    EXPECT_DOUBLE_EQ(score, 0.9);
}

TEST(ZeroTrustPolicyEnforcerTest, TrustScoreZeroWhenBothFail) {
    ZeroTrustPolicyEnforcer enforcer;
    ZeroTrustContext ctx = makeCtx("alice", "10.1.2.3");
    ctx.timestamp = std::chrono::system_clock::now();

    double score = enforcer.computeTrustScore(ctx, /*identity_ok=*/false, /*network_ok=*/false);
    EXPECT_LE(score, 0.2); // only freshness bonus possible
}

// ============================================================================
// End-to-end: verify()
// ============================================================================

TEST(ZeroTrustPolicyEnforcerTest, VerifyPassesWithValidContextAndPolicy) {
    auto verifier = [](const std::string& tok, const std::string&) {
        return tok == "valid";
    };
    ZeroTrustPolicyEnforcer enforcer(verifier);

    NetworkPolicy p;
    p.policy_id = "p1";
    p.identity  = "alice";
    p.allowed_cidrs = {"10.0.0.0/8"};
    p.default_deny  = true;
    enforcer.addNetworkPolicy(p);

    auto ctx = makeCtx("alice", "10.1.2.3", "valid");
    auto result = enforcer.verify(ctx);

    EXPECT_TRUE(result.verified);
    EXPECT_TRUE(result.identity_verified);
    EXPECT_TRUE(result.network_policy_passed);
    EXPECT_GT(result.trust_score, 0.0);
    EXPECT_EQ(result.request_id, "req-test");
}

TEST(ZeroTrustPolicyEnforcerTest, VerifyDeniesOnBadToken) {
    auto verifier = [](const std::string& tok, const std::string&) {
        return tok == "valid";
    };
    ZeroTrustPolicyEnforcer enforcer(verifier);

    NetworkPolicy p;
    p.policy_id = "p1";
    p.identity  = "alice";
    p.allowed_cidrs = {"10.0.0.0/8"};
    p.default_deny  = true;
    enforcer.addNetworkPolicy(p);

    auto ctx = makeCtx("alice", "10.1.2.3", "BAD-TOKEN");
    auto result = enforcer.verify(ctx);

    EXPECT_FALSE(result.verified);
    EXPECT_FALSE(result.identity_verified);
}

TEST(ZeroTrustPolicyEnforcerTest, VerifyDeniesOnNetworkPolicyViolation) {
    ZeroTrustPolicyEnforcer enforcer; // no token verifier → token check passes

    NetworkPolicy p;
    p.policy_id = "p1";
    p.identity  = "alice";
    p.allowed_cidrs = {"10.0.0.0/8"};
    p.default_deny  = true;
    enforcer.addNetworkPolicy(p);

    auto ctx = makeCtx("alice", "192.168.1.1"); // outside allowed network
    auto result = enforcer.verify(ctx);

    EXPECT_FALSE(result.verified);
    EXPECT_TRUE(result.identity_verified);
    EXPECT_FALSE(result.network_policy_passed);
    EXPECT_EQ(result.policy_id, "p1");
}

TEST(ZeroTrustPolicyEnforcerTest, VerifyDeniesOnDeniedCidr) {
    ZeroTrustPolicyEnforcer enforcer;

    NetworkPolicy p;
    p.policy_id = "p1";
    p.identity  = "alice";
    p.allowed_cidrs = {"10.0.0.0/8"};
    p.denied_cidrs  = {"10.0.0.0/24"};
    p.default_deny  = true;
    enforcer.addNetworkPolicy(p);

    auto ctx = makeCtx("alice", "10.0.0.5");
    auto result = enforcer.verify(ctx);

    EXPECT_FALSE(result.verified);
}

// ============================================================================
// Metrics
// ============================================================================

TEST(ZeroTrustPolicyEnforcerTest, MetricsAreUpdatedCorrectly) {
    ZeroTrustPolicyEnforcer enforcer; // no verifier, no policies → allow all

    auto ctx = makeCtx("alice", "1.2.3.4");
    enforcer.verify(ctx);
    enforcer.verify(ctx);

    EXPECT_EQ(enforcer.getMetrics().requests_total.load(), 2u);
    EXPECT_EQ(enforcer.getMetrics().requests_allowed.load(), 2u);
    EXPECT_EQ(enforcer.getMetrics().requests_denied.load(), 0u);
}

TEST(ZeroTrustPolicyEnforcerTest, MetricsDenialCountedOnNetworkViolation) {
    ZeroTrustPolicyEnforcer enforcer;

    NetworkPolicy p;
    p.policy_id = "p1";
    p.identity  = "alice";
    p.allowed_cidrs = {"10.0.0.0/8"};
    p.default_deny  = true;
    enforcer.addNetworkPolicy(p);

    auto ctx = makeCtx("alice", "192.168.1.1");
    enforcer.verify(ctx);

    EXPECT_EQ(enforcer.getMetrics().network_policy_denials.load(), 1u);
    EXPECT_EQ(enforcer.getMetrics().requests_denied.load(), 1u);
    EXPECT_EQ(enforcer.getMetrics().requests_allowed.load(), 0u);
}

TEST(ZeroTrustPolicyEnforcerTest, MetricsIdentityFailureCounted) {
    auto verifier = [](const std::string& tok, const std::string&) {
        return tok == "valid";
    };
    ZeroTrustPolicyEnforcer enforcer(verifier);

    auto ctx = makeCtx("alice", "1.2.3.4", "bad-token");
    enforcer.verify(ctx);

    EXPECT_EQ(enforcer.getMetrics().identity_failures.load(), 1u);
    EXPECT_EQ(enforcer.getMetrics().requests_denied.load(), 1u);
}
