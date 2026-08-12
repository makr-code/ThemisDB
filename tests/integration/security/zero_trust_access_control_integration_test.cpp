/**
 * @file zero_trust_access_control_integration_test.cpp
 * @brief Integration test for zero-trust + AccessControlManager
 *
 * Tests the full per-request identity verification flow:
 *   1. Authentication via AuthMiddleware (static token)
 *   2. Zero-trust identity and network policy verification
 *   3. RBAC/ABAC authorization
 *
 * Verifies that:
 * - Zero-trust denial blocks the request before RBAC is evaluated
 * - A request that passes zero-trust proceeds to RBAC normally
 * - Disabling zero-trust leaves the existing auth+RBAC flow unchanged
 */

#include "security/access_control_manager.h"
#include "security/zero_trust_policy_enforcer.h"
#include "server/auth_middleware.h"
#include <gtest/gtest.h>
#include <memory>
#include <string>

namespace themis {
namespace test {

// ============================================================================
// Helpers
// ============================================================================

/// Build an AuthMiddleware with a single static token "tok:<user_id>" → user_id.
static std::shared_ptr<AuthMiddleware> makeAuthMiddleware(
    const std::string& user_id,
    const std::string& token)
{
    auto mw = std::make_shared<AuthMiddleware>();
    AuthMiddleware::TokenConfig cfg;
    cfg.token   = token;
    cfg.user_id = user_id;
    mw->addToken(cfg);
    return mw;
}

/// Build an AccessControlManager with the reader role for user_id.
static std::unique_ptr<security::AccessControlManager> makeAcm(
    bool enable_zero_trust,
    const std::string& user_id,
    const std::string& token)
{
    security::AccessControlConfig cfg;
    cfg.enable_audit_logging = false;
    cfg.enable_zero_trust    = enable_zero_trust;

    auto acm = std::make_unique<security::AccessControlManager>(cfg);
    acm->setAuthMiddleware(makeAuthMiddleware(user_id, token));

    // Grant user_id the "reader" role with read access to "data"
    security::Role reader_role;
    reader_role.name        = "reader";
    reader_role.permissions = {{"data", "read"}};
    acm->getRBAC()->addRole(reader_role);
    acm->assignRole(user_id, "reader");

    return acm;
}

/// Build a ZeroTrustPolicyEnforcer that accepts tokens of the form "tok:<user_id>"
/// and allows only the "10.0.0.0/8" network for the given identity.
static std::unique_ptr<security::ZeroTrustPolicyEnforcer> makeEnforcer(
    const std::string& identity,
    const std::string& token)
{
    auto enforcer = std::make_unique<security::ZeroTrustPolicyEnforcer>(
        [token](const std::string& tok, const std::string&) {
            return tok == token;
        });

    security::NetworkPolicy policy;
    policy.policy_id     = "corp";
    policy.identity      = identity;
    policy.allowed_cidrs = {"10.0.0.0/8"};
    policy.default_deny  = true;
    enforcer->addNetworkPolicy(policy);
    return enforcer;
}

// ============================================================================
// Tests
// ============================================================================

/// Zero-trust disabled (default) – existing auth+RBAC flow is unchanged.
TEST(ZeroTrustAccessControlIntegrationTest, ZeroTrustDisabledFlowIsUnchanged) {
    auto acm = makeAcm(/*enable_zero_trust=*/false, "alice", "alice-token");

    // Good token, allowed resource → granted
    auto ok = acm->checkAccess("alice-token", "data", "read", "192.168.1.1");
    EXPECT_TRUE(ok.granted) << ok.reason;

    // Bad token → authentication failure
    auto denied = acm->checkAccess("bad-token", "data", "read", "10.0.0.1");
    EXPECT_FALSE(denied.granted);
}

/// Zero-trust enabled: valid identity from allowed network → allowed.
TEST(ZeroTrustAccessControlIntegrationTest, ZeroTrustEnabledAllowedNetworkPasses) {
    auto acm     = makeAcm(/*enable_zero_trust=*/true, "alice", "alice-token");
    auto enforcer = makeEnforcer("alice", "alice-token");
    acm->setZeroTrustEnforcer(enforcer.get());

    auto decision = acm->checkAccess("alice-token", "data", "read", "10.5.6.7");
    EXPECT_TRUE(decision.granted) << decision.reason;

    // Zero-trust counters: one allowed request
    EXPECT_EQ(enforcer->getMetrics().requests_total.load(),   1u);
    EXPECT_EQ(enforcer->getMetrics().requests_allowed.load(), 1u);
    EXPECT_EQ(enforcer->getMetrics().requests_denied.load(),  0u);
}

/// Zero-trust enabled: valid identity from blocked network → denied before RBAC.
TEST(ZeroTrustAccessControlIntegrationTest, ZeroTrustEnabledBlockedNetworkDenies) {
    auto acm     = makeAcm(/*enable_zero_trust=*/true, "alice", "alice-token");
    auto enforcer = makeEnforcer("alice", "alice-token");
    acm->setZeroTrustEnforcer(enforcer.get());

    // alice is authenticated but comes from 192.168.x.x → zero-trust denies
    auto decision = acm->checkAccess("alice-token", "data", "read", "192.168.1.1");
    EXPECT_FALSE(decision.granted);
    EXPECT_NE(decision.reason.find("Zero-trust"), std::string::npos) << decision.reason;

    // Verify the network-policy denial counter was incremented
    EXPECT_EQ(enforcer->getMetrics().network_policy_denials.load(), 1u);
    EXPECT_EQ(enforcer->getMetrics().requests_denied.load(),        1u);
    EXPECT_EQ(enforcer->getMetrics().requests_allowed.load(),       0u);
}

/// Zero-trust enabled: invalid token → identity failure logged in enforcer.
TEST(ZeroTrustAccessControlIntegrationTest, ZeroTrustEnabledBadTokenDenies) {
    auto acm = makeAcm(/*enable_zero_trust=*/true, "alice", "alice-token");

    // Enforcer uses strict token matching
    security::ZeroTrustPolicyEnforcer enforcer(
        [](const std::string& tok, const std::string&) {
            return tok == "alice-token";
        });
    acm->setZeroTrustEnforcer(&enforcer);

    // Authentication passes (static token "bad-token" not in AuthMiddleware)
    // → authentication fails first; zero-trust never reached
    auto decision = acm->checkAccess("bad-token", "data", "read", "10.0.0.1");
    EXPECT_FALSE(decision.granted);
    // Zero-trust enforcer was not called (authentication failed first)
    EXPECT_EQ(enforcer.getMetrics().requests_total.load(), 0u);
}

/// setZeroTrustEnforcer(nullptr) disables zero-trust at runtime.
TEST(ZeroTrustAccessControlIntegrationTest, SetNullptrDisablesZeroTrust) {
    auto acm     = makeAcm(/*enable_zero_trust=*/true, "alice", "alice-token");
    auto enforcer = makeEnforcer("alice", "alice-token");
    acm->setZeroTrustEnforcer(enforcer.get());

    // With enforcer set: blocked IP → denied
    auto denied = acm->checkAccess("alice-token", "data", "read", "192.168.1.1");
    EXPECT_FALSE(denied.granted);

    // Detach enforcer → zero-trust check is skipped even with enable_zero_trust=true
    acm->setZeroTrustEnforcer(nullptr);
    auto allowed = acm->checkAccess("alice-token", "data", "read", "192.168.1.1");
    EXPECT_TRUE(allowed.granted) << allowed.reason;
}

/// Zero-trust enabled but enforcer not set → treated as if disabled.
TEST(ZeroTrustAccessControlIntegrationTest, EnableFlagWithNoEnforcerIsNoop) {
    auto acm = makeAcm(/*enable_zero_trust=*/true, "alice", "alice-token");
    // Deliberately do NOT call setZeroTrustEnforcer

    // Should behave as if zero-trust is disabled
    auto decision = acm->checkAccess("alice-token", "data", "read", "192.168.1.1");
    EXPECT_TRUE(decision.granted) << decision.reason;
}

} // namespace test
} // namespace themis
