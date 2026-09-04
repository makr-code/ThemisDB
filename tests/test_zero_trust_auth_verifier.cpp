#include <gtest/gtest.h>
#include "auth/zero_trust_auth_verifier.h"
#include "auth/session_manager.h"
#include "security/zero_trust_policy_enforcer.h"

#include <atomic>
#include <chrono>
#include <thread>

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

// ============================================================================
// Config: re_evaluation_interval
// ============================================================================

TEST(ZeroTrustAuthVerifierTest, DefaultReEvaluationInterval) {
    ZeroTrustAuthVerifier::Config cfg;
    EXPECT_EQ(cfg.re_evaluation_interval, std::chrono::milliseconds(300000));
}

TEST(ZeroTrustAuthVerifierTest, CustomReEvaluationInterval) {
    ZeroTrustAuthVerifier::Config cfg;
    cfg.re_evaluation_interval = std::chrono::milliseconds(60000);
    EXPECT_EQ(cfg.re_evaluation_interval, std::chrono::milliseconds(60000));
}

// ============================================================================
// Background session monitoring
// ============================================================================

TEST(ZeroTrustAuthVerifierTest, MonitoredSessionCountZeroByDefault) {
    ZeroTrustAuthVerifier verifier;
    EXPECT_EQ(verifier.monitoredSessionCount(), 0u);
}

TEST(ZeroTrustAuthVerifierTest, StartMonitoringIncreasesCount) {
    ZeroTrustAuthVerifier verifier;
    SessionManager sm;
    const auto sid = sm.createSession("alice");

    ZeroTrustAuthVerifier::MonitoredSession ms;
    ms.session_id = sid;
    ms.user_id    = "alice";
    ms.token      = "valid-token";
    ms.client_ip  = "10.0.0.1";
    ms.resource   = "data";
    ms.action     = "read";

    verifier.startSessionMonitoring(ms, &sm);
    EXPECT_EQ(verifier.monitoredSessionCount(), 1u);

    verifier.stopSessionMonitoring(sid);
    EXPECT_EQ(verifier.monitoredSessionCount(), 0u);
}

TEST(ZeroTrustAuthVerifierTest, StopMonitoringOnUnknownIdIsNoOp) {
    ZeroTrustAuthVerifier verifier;
    // Must not throw or crash
    EXPECT_NO_THROW(verifier.stopSessionMonitoring("nonexistent-session"));
    EXPECT_EQ(verifier.monitoredSessionCount(), 0u);
}

TEST(ZeroTrustAuthVerifierTest, MultipleSessionsCanBeMonitored) {
    ZeroTrustAuthVerifier verifier;
    SessionManager sm;

    for (int i = 0; i < 3; ++i) {
        auto sid = sm.createSession("user" + std::to_string(i));
        ZeroTrustAuthVerifier::MonitoredSession ms;
        ms.session_id = sid;
        ms.user_id    = "user" + std::to_string(i);
        ms.token      = "tok";
        ms.client_ip  = "10.0.0.1";
        ms.resource   = "data";
        ms.action     = "read";
        verifier.startSessionMonitoring(ms, &sm);
    }
    EXPECT_EQ(verifier.monitoredSessionCount(), 3u);
}

TEST(ZeroTrustAuthVerifierTest, FailedReEvaluationTerminatesSession) {
    // Token starts valid; we flip it to invalid so re-evaluation fails.
    std::atomic<bool> token_ok{true};
    auto cb = [&token_ok](const std::string&, const std::string&) -> bool {
        return token_ok.load();
    };

    // Use a very short re-evaluation interval so the test does not wait 300 s.
    ZeroTrustAuthVerifier::Config cfg;
    cfg.re_evaluation_interval = std::chrono::milliseconds(50);

    ZeroTrustAuthVerifier verifier(cfg, cb);

    SessionManager sm;
    const auto sid = sm.createSession("alice");
    EXPECT_EQ(sm.listSessions("alice").size(), 1u);

    ZeroTrustAuthVerifier::MonitoredSession ms;
    ms.session_id = sid;
    ms.user_id    = "alice";
    ms.token      = "my-token";
    ms.client_ip  = "10.0.0.1";
    ms.resource   = "data";
    ms.action     = "read";

    // Register monitoring BEFORE invalidating the token so we get a clean start.
    verifier.startSessionMonitoring(ms, &sm);

    // Invalidate the token — next re-evaluation should revoke the session.
    token_ok.store(false);

    // Wait for the background worker to revoke the session (up to 2 s).
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(2);
    while (std::chrono::steady_clock::now() < deadline) {
        if (sm.listSessions("alice").empty()) {
          break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }

    EXPECT_TRUE(sm.listSessions("alice").empty())
        << "Expected session to be terminated by background re-evaluation";
    EXPECT_EQ(verifier.monitoredSessionCount(), 0u)
        << "Expected session removed from monitoring after revocation";
}

TEST(ZeroTrustAuthVerifierTest, PassingReEvaluationKeepsSession) {
    // Token remains valid — session must NOT be terminated.
    auto cb = [](const std::string&, const std::string&) -> bool { return true; };

    ZeroTrustAuthVerifier::Config cfg;
    cfg.re_evaluation_interval = std::chrono::milliseconds(50);

    ZeroTrustAuthVerifier verifier(cfg, cb);

    SessionManager sm;
    const auto sid = sm.createSession("bob");

    ZeroTrustAuthVerifier::MonitoredSession ms;
    ms.session_id = sid;
    ms.user_id    = "bob";
    ms.token      = "good-token";
    ms.client_ip  = "10.0.0.1";
    ms.resource   = "data";
    ms.action     = "read";

    verifier.startSessionMonitoring(ms, &sm);

    // Let a few re-evaluation cycles run.
    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    // Session should still be alive.
    EXPECT_EQ(sm.listSessions("bob").size(), 1u)
        << "Session should remain active while re-evaluation passes";

    verifier.stopSessionMonitoring(sid);
}
