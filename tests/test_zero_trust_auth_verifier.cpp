/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            test_zero_trust_auth_verifier.cpp                  ║
  Version:         0.0.1                                              ║
  Last Modified:   2026-03-02 04:07:49                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     231                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • e8e02c9ec  2026-02-24  feat(auth): implement zero-trust continuous verification ... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include <gtest/gtest.h>
#include "auth/zero_trust_auth_verifier.h"
#include "security/zero_trust_policy_enforcer.h"

using namespace themis::auth;
using namespace themis::security;

// ============================================================================
// Helper
// ============================================================================

static ZeroTrustAuthVerifier::Request makeReq(
    const std::string& user_id,
    const std::string& client_ip,
    const std::string& token    = "valid-token",
    const std::string& resource = "data",
    const std::string& action   = "read")
{
    ZeroTrustAuthVerifier::Request req;
    req.request_id = "req-test";
    req.user_id    = user_id;
    req.client_ip  = client_ip;
    req.token      = token;
    req.resource   = resource;
    req.action     = action;
    return req;
}

// ============================================================================
// Construction and defaults
// ============================================================================

TEST(ZeroTrustAuthVerifierTest, DefaultConstructorAllowsAllWithNoPolicies) {
    // No token verifier, no network policies → everything passes
    ZeroTrustAuthVerifier verifier;
    auto d = verifier.verify(makeReq("alice", "10.1.2.3"));
    EXPECT_TRUE(d.allowed);
    EXPECT_TRUE(d.identity_verified);
    EXPECT_TRUE(d.network_ok);
}

// ============================================================================
// Token verification
// ============================================================================

TEST(ZeroTrustAuthVerifierTest, BadTokenDeniesRequest) {
    auto cb = [](const std::string& tok, const std::string&) {
        return tok == "valid";
    };
    ZeroTrustAuthVerifier verifier({}, cb);

    auto d = verifier.verify(makeReq("alice", "1.2.3.4", "BAD"));
    EXPECT_FALSE(d.allowed);
    EXPECT_FALSE(d.identity_verified);
}

TEST(ZeroTrustAuthVerifierTest, ValidTokenPassesIdentityCheck) {
    auto cb = [](const std::string& tok, const std::string&) {
        return tok == "valid";
    };
    ZeroTrustAuthVerifier verifier({}, cb);

    auto d = verifier.verify(makeReq("alice", "1.2.3.4", "valid"));
    EXPECT_TRUE(d.allowed);
    EXPECT_TRUE(d.identity_verified);
}

// ============================================================================
// Network policy enforcement
// ============================================================================

TEST(ZeroTrustAuthVerifierTest, NetworkPolicyDeniesOutsideAllowedCidr) {
    ZeroTrustAuthVerifier verifier;

    NetworkPolicy p;
    p.policy_id     = "corp";
    p.identity      = "alice";
    p.allowed_cidrs = {"10.0.0.0/8"};
    p.default_deny  = true;
    verifier.addNetworkPolicy(p);

    auto d = verifier.verify(makeReq("alice", "192.168.1.1"));
    EXPECT_FALSE(d.allowed);
    EXPECT_FALSE(d.network_ok);
    EXPECT_TRUE(d.identity_verified);
}

TEST(ZeroTrustAuthVerifierTest, NetworkPolicyAllowsInsideAllowedCidr) {
    ZeroTrustAuthVerifier verifier;

    NetworkPolicy p;
    p.policy_id     = "corp";
    p.identity      = "alice";
    p.allowed_cidrs = {"10.0.0.0/8"};
    p.default_deny  = true;
    verifier.addNetworkPolicy(p);

    auto d = verifier.verify(makeReq("alice", "10.5.6.7"));
    EXPECT_TRUE(d.allowed);
    EXPECT_TRUE(d.network_ok);
}

TEST(ZeroTrustAuthVerifierTest, DeniedCidrBlocksEvenIfInAllowedRange) {
    ZeroTrustAuthVerifier verifier;

    NetworkPolicy p;
    p.policy_id     = "corp";
    p.identity      = "alice";
    p.allowed_cidrs = {"10.0.0.0/8"};
    p.denied_cidrs  = {"10.0.0.0/24"};
    p.default_deny  = true;
    verifier.addNetworkPolicy(p);

    auto d = verifier.verify(makeReq("alice", "10.0.0.5"));
    EXPECT_FALSE(d.allowed);
}

// ============================================================================
// Trust score threshold
// ============================================================================

TEST(ZeroTrustAuthVerifierTest, LowTrustScoreDeniesEvenIfChecksPass) {
    // Set a very high threshold that an old, device-less request cannot meet
    ZeroTrustAuthVerifier::Config cfg;
    cfg.min_trust_score = 1.0;  // Require perfect score

    ZeroTrustAuthVerifier verifier(cfg);
    // No network policies (passes network check) and no token verifier (passes token check)
    // trust_score = 0.4 (identity) + 0.4 (network) + 0.1 (freshness) = 0.9, no device → 0.9
    // 0.9 < 1.0 → denied
    auto req = makeReq("alice", "1.2.3.4");
    // No device_id → score = 0.9
    auto d = verifier.verify(req);
    EXPECT_FALSE(d.allowed);
    EXPECT_GT(d.trust_score, 0.0);
}

TEST(ZeroTrustAuthVerifierTest, HighEnoughTrustScoreAllows) {
    ZeroTrustAuthVerifier::Config cfg;
    cfg.min_trust_score = 0.5;

    ZeroTrustAuthVerifier verifier(cfg);
    auto d = verifier.verify(makeReq("alice", "1.2.3.4"));
    EXPECT_TRUE(d.allowed);
    EXPECT_GT(d.trust_score, 0.5);
}

// ============================================================================
// Policy management
// ============================================================================

TEST(ZeroTrustAuthVerifierTest, AddAndRemovePolicy) {
    ZeroTrustAuthVerifier verifier;

    NetworkPolicy p;
    p.policy_id     = "p1";
    p.identity      = "alice";
    p.allowed_cidrs = {"10.0.0.0/8"};
    p.default_deny  = true;
    verifier.addNetworkPolicy(p);

    EXPECT_EQ(verifier.getNetworkPolicies().size(), 1u);
    EXPECT_TRUE(verifier.removeNetworkPolicy("p1"));
    EXPECT_EQ(verifier.getNetworkPolicies().size(), 0u);
    EXPECT_FALSE(verifier.removeNetworkPolicy("p1"));
}

// ============================================================================
// Metrics
// ============================================================================

TEST(ZeroTrustAuthVerifierTest, MetricsAreForwardedFromEnforcer) {
    ZeroTrustAuthVerifier verifier;

    verifier.verify(makeReq("alice", "1.2.3.4"));
    verifier.verify(makeReq("bob", "2.3.4.5"));

    EXPECT_EQ(verifier.getMetrics().requests_total.load(), 2u);
    EXPECT_EQ(verifier.getMetrics().requests_allowed.load(), 2u);
}

TEST(ZeroTrustAuthVerifierTest, MetricsDeniedCountedOnTokenFailure) {
    auto cb = [](const std::string& tok, const std::string&) {
        return tok == "ok";
    };
    ZeroTrustAuthVerifier verifier({}, cb);

    verifier.verify(makeReq("alice", "1.2.3.4", "bad"));
    EXPECT_EQ(verifier.getMetrics().identity_failures.load(), 1u);
    EXPECT_EQ(verifier.getMetrics().requests_denied.load(), 1u);
}

// ============================================================================
// Continuous verification contract
// ============================================================================

TEST(ZeroTrustAuthVerifierTest, EachCallIsIndependent_NoSessionCache) {
    // Demonstrates continuous verification: a token that becomes invalid
    // must fail even after a previous successful call
    bool token_valid = true;
    auto cb = [&token_valid](const std::string&, const std::string&) {
        return token_valid;
    };
    ZeroTrustAuthVerifier verifier({}, cb);

    auto req = makeReq("alice", "1.2.3.4", "some-token");

    // First call: valid
    auto d1 = verifier.verify(req);
    EXPECT_TRUE(d1.allowed);

    // Token is revoked/expired
    token_valid = false;

    // Second call: same request, now denied (continuous verification)
    auto d2 = verifier.verify(req);
    EXPECT_FALSE(d2.allowed);
}
