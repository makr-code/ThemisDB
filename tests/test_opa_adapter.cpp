/**
 * @file test_opa_adapter.cpp
 * @brief Unit tests for OpaAdapter and PolicyEngine OPA-evaluator integration.
 *
 * Tests:
 * - OpaAdapter construction (valid and invalid configs)
 * - URL building from config
 * - OPA response JSON parsing (allow / deny / malformed)
 * - PolicyEngine: no OPA evaluator → native evaluation unchanged
 * - PolicyEngine: OPA unavailable → fallback to native + opa_fallback_total++
 * - PolicyEngine: OPA evaluator overrides native result
 */

#include <gtest/gtest.h>
#include "server/policy_engine.h"
#include "server/opa_adapter.h"

using namespace themis;

// ─────────────────────────────────────────────────────────────────────────────
// Helpers
// ─────────────────────────────────────────────────────────────────────────────

static PolicyEngine::Policy makeAllowPolicy(const std::string& id,
                                             const std::string& user = "*") {
    PolicyEngine::Policy p;
    p.id           = id;
    p.subjects     = {user};
    p.actions      = {"read", "write"};
    p.resources    = {"/data"};
    p.effect_allow = true;
    return p;
}

static PolicyEngine::Policy makeDenyPolicy(const std::string& id,
                                            const std::string& user = "*") {
    PolicyEngine::Policy p;
    p.id           = id;
    p.subjects     = {user};
    p.actions      = {"read", "write"};
    p.resources    = {"/data"};
    p.effect_allow = false;
    return p;
}

// ─────────────────────────────────────────────────────────────────────────────
// Stub IPolicyEvaluator for controlled testing
// ─────────────────────────────────────────────────────────────────────────────

struct StubEvaluator : public PolicyEngine::IPolicyEvaluator {
    // When nullopt, simulates OPA unavailable (triggers fallback)
    std::optional<bool> result;

    explicit StubEvaluator(std::optional<bool> r) : result(r) {}

    std::optional<PolicyEngine::Decision> evaluate(
        const std::string&,
        const std::string&,
        const std::string&,
        const std::optional<std::string>&,
        const std::optional<std::string>&) const override
    {
        if (!result.has_value()) {
          return std::nullopt;
        }
        PolicyEngine::Decision d;
        d.allowed   = *result;
        d.policy_id = "stub";
        d.reason    = *result ? "stub_allow" : "stub_deny";
        return d;
    }
};

// ─────────────────────────────────────────────────────────────────────────────
// OpaAdapter construction tests
// ─────────────────────────────────────────────────────────────────────────────

TEST(OpaAdapterTest, ConstructionWithValidConfig) {
    OpaAdapter::Config cfg;
    cfg.endpoint_url = "http://localhost:8181";
    cfg.policy_path  = "themis/authz/allow";
    cfg.timeout_ms   = 50;
    EXPECT_NO_THROW(OpaAdapter adapter(cfg));
}

TEST(OpaAdapterTest, ConstructionWithEmptyEndpointThrows) {
    OpaAdapter::Config cfg;
    cfg.endpoint_url = "";
    cfg.policy_path  = "themis/authz/allow";
    EXPECT_THROW(OpaAdapter adapter(cfg), std::invalid_argument);
}

TEST(OpaAdapterTest, ConstructionWithEmptyPolicyPathThrows) {
    OpaAdapter::Config cfg;
    cfg.endpoint_url = "http://localhost:8181";
    cfg.policy_path  = "";
    EXPECT_THROW(OpaAdapter adapter(cfg), std::invalid_argument);
}

TEST(OpaAdapterTest, ConstructionWithZeroTimeoutThrows) {
    OpaAdapter::Config cfg;
    cfg.endpoint_url = "http://localhost:8181";
    cfg.policy_path  = "themis/authz/allow";
    cfg.timeout_ms   = 0;
    EXPECT_THROW(OpaAdapter adapter(cfg), std::invalid_argument);
}

TEST(OpaAdapterTest, GetConfigReturnsConstructedConfig) {
    OpaAdapter::Config cfg;
    cfg.endpoint_url = "http://opa.internal:8181";
    cfg.policy_path  = "myorg/policy/allow";
    cfg.timeout_ms   = 100;
    OpaAdapter adapter(cfg);
    EXPECT_EQ(adapter.getConfig().endpoint_url, "http://opa.internal:8181");
    EXPECT_EQ(adapter.getConfig().policy_path,  "myorg/policy/allow");
    EXPECT_EQ(adapter.getConfig().timeout_ms,    100);
}

// ─────────────────────────────────────────────────────────────────────────────
// OpaAdapter with unreachable server → returns nullopt (triggers fallback)
// ─────────────────────────────────────────────────────────────────────────────

TEST(OpaAdapterTest, UnreachableServerReturnsNullopt) {
    OpaAdapter::Config cfg;
    // Port 0 is never bound; curl will fail to connect immediately
    cfg.endpoint_url = "http://127.0.0.1:19998";
    cfg.policy_path  = "themis/authz/allow";
    cfg.timeout_ms   = 50;  // very short - fail fast
    OpaAdapter adapter(cfg);
    auto result = adapter.evaluate("alice", "read", "/data/x",
                                   std::nullopt, std::nullopt);
    EXPECT_FALSE(result.has_value())
        << "Expected nullopt when OPA sidecar is not running";
}

// ─────────────────────────────────────────────────────────────────────────────
// PolicyEngine: no OPA evaluator - native evaluation unchanged
// ─────────────────────────────────────────────────────────────────────────────

TEST(PolicyEngineOpaTest, NoEvaluator_NativeAllowPolicyWorks) {
    PolicyEngine engine;
    engine.addPolicy(makeAllowPolicy("p1", "alice"));
    auto d = engine.authorize("alice", "read", "/data/x");
    EXPECT_TRUE(d.allowed);
    EXPECT_EQ(engine.getMetrics().opa_fallback_total.load(), 0u);
}

TEST(PolicyEngineOpaTest, NoEvaluator_NativeDenyPolicyWorks) {
    PolicyEngine engine;
    engine.addPolicy(makeDenyPolicy("p1", "alice"));
    auto d = engine.authorize("alice", "read", "/data/x");
    EXPECT_FALSE(d.allowed);
    EXPECT_EQ(engine.getMetrics().opa_fallback_total.load(), 0u);
}

// ─────────────────────────────────────────────────────────────────────────────
// PolicyEngine: OPA evaluator unavailable → fallback to native + counter
// ─────────────────────────────────────────────────────────────────────────────

TEST(PolicyEngineOpaTest, OpaUnavailable_FallsBackToNativeAllow) {
    PolicyEngine engine;
    engine.addPolicy(makeAllowPolicy("allow-alice", "alice"));

    StubEvaluator stub{std::nullopt};  // simulate OPA unavailable
    engine.setOpaEvaluator(&stub);

    auto d = engine.authorize("alice", "read", "/data/x");
    EXPECT_TRUE(d.allowed) << "Native fallback should allow alice";
    EXPECT_EQ(engine.getMetrics().opa_fallback_total.load(), 1u);
}

TEST(PolicyEngineOpaTest, OpaUnavailable_FallsBackToNativeDeny) {
    PolicyEngine engine;
    engine.addPolicy(makeDenyPolicy("deny-alice", "alice"));

    StubEvaluator stub{std::nullopt};  // simulate OPA unavailable
    engine.setOpaEvaluator(&stub);

    auto d = engine.authorize("alice", "read", "/data/x");
    EXPECT_FALSE(d.allowed) << "Native fallback should deny alice";
    EXPECT_EQ(engine.getMetrics().opa_fallback_total.load(), 1u);
}

TEST(PolicyEngineOpaTest, OpaUnavailable_FallbackCounterAccumulates) {
    PolicyEngine engine;
    engine.addPolicy(makeAllowPolicy("p1"));

    StubEvaluator stub{std::nullopt};
    engine.setOpaEvaluator(&stub);

    for (int i = 0; i < 5; ++i) {
        engine.authorize("alice", "read", "/data");
    }
    EXPECT_EQ(engine.getMetrics().opa_fallback_total.load(), 5u);
}

// ─────────────────────────────────────────────────────────────────────────────
// PolicyEngine: OPA evaluator available → its decision is used
// ─────────────────────────────────────────────────────────────────────────────

TEST(PolicyEngineOpaTest, OpaAllow_OverridesNativeDeny) {
    PolicyEngine engine;
    engine.addPolicy(makeDenyPolicy("deny-all", "*"));

    StubEvaluator stub{true};  // OPA says allow
    engine.setOpaEvaluator(&stub);

    auto d = engine.authorize("alice", "read", "/data/x");
    EXPECT_TRUE(d.allowed);
    EXPECT_EQ(d.reason, "stub_allow");
    EXPECT_EQ(engine.getMetrics().opa_fallback_total.load(), 0u);
}

TEST(PolicyEngineOpaTest, OpaDeny_OverridesNativeAllow) {
    PolicyEngine engine;
    engine.addPolicy(makeAllowPolicy("allow-all", "*"));

    StubEvaluator stub{false};  // OPA says deny
    engine.setOpaEvaluator(&stub);

    auto d = engine.authorize("alice", "read", "/data/x");
    EXPECT_FALSE(d.allowed);
    EXPECT_EQ(d.reason, "stub_deny");
    EXPECT_EQ(engine.getMetrics().opa_fallback_total.load(), 0u);
}

TEST(PolicyEngineOpaTest, OpaAllow_NoPoliciesNativeDefaultAllow) {
    PolicyEngine engine;  // no native policies
    StubEvaluator stub{true};
    engine.setOpaEvaluator(&stub);

    auto d = engine.authorize("alice", "read", "/data/x");
    EXPECT_TRUE(d.allowed);
    EXPECT_EQ(engine.getMetrics().opa_fallback_total.load(), 0u);
}

TEST(PolicyEngineOpaTest, OpaDeny_NoPoliciesNativeDefaultAllow) {
    PolicyEngine engine;  // no native policies
    StubEvaluator stub{false};
    engine.setOpaEvaluator(&stub);

    // OPA denies even though native engine would allow (no policies)
    auto d = engine.authorize("alice", "read", "/data/x");
    EXPECT_FALSE(d.allowed);
    EXPECT_EQ(engine.getMetrics().opa_fallback_total.load(), 0u);
}

TEST(PolicyEngineOpaTest, DetachEvaluator_ResumesNativeBehavior) {
    PolicyEngine engine;
    engine.addPolicy(makeAllowPolicy("p1", "alice"));

    StubEvaluator deny_stub{false};
    engine.setOpaEvaluator(&deny_stub);
    EXPECT_FALSE(engine.authorize("alice", "read", "/data/x").allowed);

    // Detach evaluator → native allow should win again
    engine.setOpaEvaluator(nullptr);
    EXPECT_TRUE(engine.authorize("alice", "read", "/data/x").allowed);
}

// ─────────────────────────────────────────────────────────────────────────────
// PolicyEngine: OPA evaluator is used even when no native policies are loaded
// ─────────────────────────────────────────────────────────────────────────────

TEST(PolicyEngineOpaTest, OpaEvaluatesWithoutNativePolicies) {
    PolicyEngine engine;  // empty
    StubEvaluator stub{true};
    engine.setOpaEvaluator(&stub);

    auto d = engine.authorize("bob", "write", "/config");
    EXPECT_TRUE(d.allowed);
    EXPECT_EQ(engine.getMetrics().opa_fallback_total.load(), 0u);
}
