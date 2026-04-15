/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            test_zero_trust_policy_enforcer.cpp                ║
  Version:         0.0.14                                             ║
  Last Modified:   2026-04-15 18:19:25                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     388                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
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

// ============================================================================
// Phase 3.2 — IPv6 CIDR support
// ============================================================================

TEST(ZeroTrustPolicyEnforcerTest, IPv6CidrMatchesExact) {
    ZeroTrustPolicyEnforcer enforcer;
    NetworkPolicy p;
    p.policy_id = "v6-exact";
    p.identity  = "alice";
    p.allowed_cidrs = {"2001:db8::/32"};
    p.default_deny  = true;
    enforcer.addNetworkPolicy(p);

    // Address inside the /32 block
    auto ctx = makeCtx("alice", "2001:db8::1");
    auto res = enforcer.verify(ctx);
    EXPECT_TRUE(res.verified) << res.reason;
}

TEST(ZeroTrustPolicyEnforcerTest, IPv6CidrRejectsOutsideBlock) {
    ZeroTrustPolicyEnforcer enforcer;
    NetworkPolicy p;
    p.policy_id = "v6-block";
    p.identity  = "bob";
    p.allowed_cidrs = {"2001:db8::/32"};
    p.default_deny  = true;
    enforcer.addNetworkPolicy(p);

    // Address outside the block
    auto ctx = makeCtx("bob", "2001:db9::1");
    auto res = enforcer.verify(ctx);
    EXPECT_FALSE(res.verified);
}

TEST(ZeroTrustPolicyEnforcerTest, IPv6SlashOneHundredTwentyEightExactMatch) {
    ZeroTrustPolicyEnforcer enforcer;
    NetworkPolicy p;
    p.policy_id = "v6-128";
    p.identity  = "carol";
    p.allowed_cidrs = {"::1/128"};
    p.default_deny  = true;
    enforcer.addNetworkPolicy(p);

    EXPECT_TRUE(enforcer.verify(makeCtx("carol", "::1")).verified);
    EXPECT_FALSE(enforcer.verify(makeCtx("carol", "::2")).verified);
}

TEST(ZeroTrustPolicyEnforcerTest, IPv6SlashZeroMatchesAny) {
    ZeroTrustPolicyEnforcer enforcer;
    NetworkPolicy p;
    p.policy_id = "v6-any";
    p.identity  = "dave";
    p.allowed_cidrs = {"::/0"};
    p.default_deny  = true;
    enforcer.addNetworkPolicy(p);

    EXPECT_TRUE(enforcer.verify(makeCtx("dave", "2001:db8::cafe")).verified);
    EXPECT_TRUE(enforcer.verify(makeCtx("dave", "::1")).verified);
}

TEST(ZeroTrustPolicyEnforcerTest, IPv6MalformedRejected) {
    ZeroTrustPolicyEnforcer enforcer;
    NetworkPolicy p;
    p.policy_id = "v6-malformed";
    p.identity  = "eve";
    p.allowed_cidrs = {"2001:db8::/32"};
    p.default_deny  = true;
    enforcer.addNetworkPolicy(p);

    // "not-an-ip" is not a valid IPv6 address
    auto ctx = makeCtx("eve", "not-an-ip");
    auto res = enforcer.verify(ctx);
    EXPECT_FALSE(res.verified);
}

TEST(ZeroTrustPolicyEnforcerTest, IPv4MappedIPv6NormalisedToIPv4) {
    ZeroTrustPolicyEnforcer enforcer;
    NetworkPolicy p;
    p.policy_id = "v4-mapped";
    p.identity  = "frank";
    p.allowed_cidrs = {"10.0.0.0/8"};  // IPv4 CIDR
    p.default_deny  = true;
    enforcer.addNetworkPolicy(p);

    // ::ffff:10.1.2.3 should normalise to 10.1.2.3 and match the IPv4 CIDR
    auto ctx = makeCtx("frank", "::ffff:10.1.2.3");
    auto res = enforcer.verify(ctx);
    EXPECT_TRUE(res.verified) << res.reason;
}

TEST(ZeroTrustPolicyEnforcerTest, IPv6DeniedCidrTakesPrecedence) {
    ZeroTrustPolicyEnforcer enforcer;
    NetworkPolicy p;
    p.policy_id = "v6-deny";
    p.identity  = "grace";
    p.allowed_cidrs = {"2001:db8::/32"};
    p.denied_cidrs  = {"2001:db8::bad:0/112"};
    p.default_deny  = true;
    enforcer.addNetworkPolicy(p);

    // Inside allowed block but also inside denied block
    auto ctx = makeCtx("grace", "2001:db8::bad:1");
    auto res = enforcer.verify(ctx);
    EXPECT_FALSE(res.verified);
}

// ============================================================================
// Phase 3.1 — Adaptive continuous re-verification
// ============================================================================

TEST(ZeroTrustPolicyEnforcerTest, ContinuousReVerificationTriggersWhenExpired) {
    // Set up an enforcer with a very short re-verification interval.
    // The first request should pass; on re-checking with an invalid token,
    // the session should be denied.
    auto verifier = [](const std::string& tok, const std::string&) {
        return tok == "valid";
    };
    ZeroTrustPolicyEnforcer enforcer(verifier);

    NetworkPolicy p;
    p.policy_id = "reVerify";
    p.identity  = "alice";
    p.allowed_cidrs = {"0.0.0.0/0"};
    p.default_deny  = false;
    // Interval: 0ms (always re-verify)
    p.continuous_verification_interval_ms = std::chrono::milliseconds{0};
    enforcer.addNetworkPolicy(p);

    // Bad token → should be denied
    auto ctx = makeCtx("alice", "10.0.0.1", "bad-token");
    ctx.last_verified_at = std::chrono::system_clock::now() - std::chrono::seconds{3600};
    auto res = enforcer.verify(ctx);
    EXPECT_FALSE(res.verified);
}

TEST(ZeroTrustPolicyEnforcerTest, RiskScoreThresholdRevokesSession) {
    ZeroTrustPolicyEnforcer enforcer;

    NetworkPolicy p;
    p.policy_id = "risky";
    p.identity  = "mallory";
    p.allowed_cidrs = {"0.0.0.0/0"};
    p.default_deny  = false;
    p.risk_score_threshold = 0.5;  // revoke when risk > 0.5
    enforcer.addNetworkPolicy(p);

    auto ctx = makeCtx("mallory", "1.2.3.4");
    ctx.session_risk_score = 0.8;  // above threshold
    auto res = enforcer.verify(ctx);
    EXPECT_FALSE(res.verified);
    EXPECT_NE(res.reason.find("risk score"), std::string::npos);
}

TEST(ZeroTrustPolicyEnforcerTest, RiskScoreBelowThresholdAllowed) {
    ZeroTrustPolicyEnforcer enforcer;

    NetworkPolicy p;
    p.policy_id = "safe";
    p.identity  = "alice";
    p.allowed_cidrs = {"0.0.0.0/0"};
    p.default_deny  = false;
    p.risk_score_threshold = 0.9;
    enforcer.addNetworkPolicy(p);

    auto ctx = makeCtx("alice", "10.0.0.1");
    ctx.session_risk_score = 0.2;  // well below threshold
    auto res = enforcer.verify(ctx);
    EXPECT_TRUE(res.verified) << res.reason;
}

TEST(ZeroTrustPolicyEnforcerTest, RiskScoreReducesTrustScore) {
    ZeroTrustPolicyEnforcer enforcer;

    NetworkPolicy p;
    p.policy_id = "trust-check";
    p.identity  = "alice";
    p.allowed_cidrs = {"0.0.0.0/0"};
    p.default_deny  = false;
    enforcer.addNetworkPolicy(p);

    auto ctx_low  = makeCtx("alice", "10.0.0.1");
    auto ctx_high = makeCtx("alice", "10.0.0.1");
    ctx_low.session_risk_score  = 0.0;
    ctx_high.session_risk_score = 0.3;

    auto res_low  = enforcer.verify(ctx_low);
    auto res_high = enforcer.verify(ctx_high);

    EXPECT_TRUE(res_low.verified);
    EXPECT_TRUE(res_high.verified);
    // Higher risk → lower trust score
    EXPECT_GT(res_low.trust_score, res_high.trust_score);
}
