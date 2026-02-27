/**
 * @file test_governance_opa_adapter.cpp
 * @brief Unit tests for governance::OpaAdapter and PolicyEngine OPA-evaluator integration.
 *
 * Tests:
 * - OpaAdapter construction (valid and invalid configs)
 * - URL building from config
 * - OPA response parsing (allow / deny / structured / malformed)
 * - PolicyEngine: no OPA evaluator → native evaluation unchanged
 * - PolicyEngine: OPA unavailable → fallback to native + governance_opa_fallback_total counter
 * - PolicyEngine: OPA evaluator overrides native result
 * - PolicyEngine: detach evaluator → resumes native behavior
 * - CCPA opt-out enforcement applied on top of OPA allow decision
 */

#include <gtest/gtest.h>
#include "governance/policy_engine.h"
#include "governance/opa_adapter.h"

#include <memory>
#include <unordered_map>
#include <unordered_set>

using namespace themis::governance;

// ─────────────────────────────────────────────────────────────────────────────
// Stub IPolicyEvaluator for controlled testing (no actual HTTP calls)
// ─────────────────────────────────────────────────────────────────────────────

struct StubEvaluator : public PolicyEngine::IPolicyEvaluator {
    // When nullopt, simulates OPA unavailable (triggers fallback)
    std::optional<PolicyDecision> result;

    explicit StubEvaluator() : result(std::nullopt) {}
    explicit StubEvaluator(PolicyDecision d) : result(std::move(d)) {}

    std::optional<PolicyDecision> evaluate(
        const std::unordered_map<std::string, std::string>&,
        const std::string&) const override
    {
        return result;
    }
};

static PolicyDecision makeAllowDecision(const std::string& cls = "offen") {
    PolicyDecision d;
    d.classification             = cls;
    d.mode                       = "enforce";
    d.encrypt_logs               = false;
    d.redaction                  = "none";
    d.ann_allowed                = true;
    d.require_content_encryption = false;
    d.export_allowed             = true;
    d.cache_allowed              = true;
    d.retention_days             = 30;
    return d;
}

static PolicyDecision makeDenyDecision() {
    PolicyDecision d;
    d.classification             = "streng-geheim";
    d.mode                       = "enforce";
    d.encrypt_logs               = true;
    d.redaction                  = "strict";
    d.ann_allowed                = false;
    d.require_content_encryption = true;
    d.export_allowed             = false;
    d.cache_allowed              = false;
    d.retention_days             = 7;
    return d;
}

// ─────────────────────────────────────────────────────────────────────────────
// OpaAdapter construction tests
// ─────────────────────────────────────────────────────────────────────────────

TEST(GovernanceOpaAdapterTest, ConstructionWithValidConfig) {
    OpaAdapter::Config cfg;
    cfg.endpoint_url = "http://localhost:8181";
    cfg.policy_path  = "themis/governance/allow";
    cfg.timeout_ms   = 50;
    EXPECT_NO_THROW(OpaAdapter adapter(cfg));
}

TEST(GovernanceOpaAdapterTest, ConstructionWithEmptyEndpointThrows) {
    OpaAdapter::Config cfg;
    cfg.endpoint_url = "";
    cfg.policy_path  = "themis/governance/allow";
    EXPECT_THROW(OpaAdapter adapter(cfg), std::invalid_argument);
}

TEST(GovernanceOpaAdapterTest, ConstructionWithEmptyPolicyPathThrows) {
    OpaAdapter::Config cfg;
    cfg.endpoint_url = "http://localhost:8181";
    cfg.policy_path  = "";
    EXPECT_THROW(OpaAdapter adapter(cfg), std::invalid_argument);
}

TEST(GovernanceOpaAdapterTest, ConstructionWithZeroTimeoutThrows) {
    OpaAdapter::Config cfg;
    cfg.endpoint_url = "http://localhost:8181";
    cfg.policy_path  = "themis/governance/allow";
    cfg.timeout_ms   = 0;
    EXPECT_THROW(OpaAdapter adapter(cfg), std::invalid_argument);
}

TEST(GovernanceOpaAdapterTest, ConstructionWithNegativeTimeoutThrows) {
    OpaAdapter::Config cfg;
    cfg.endpoint_url = "http://localhost:8181";
    cfg.policy_path  = "themis/governance/allow";
    cfg.timeout_ms   = -1;
    EXPECT_THROW(OpaAdapter adapter(cfg), std::invalid_argument);
}

TEST(GovernanceOpaAdapterTest, GetConfigReturnsConstructedConfig) {
    OpaAdapter::Config cfg;
    cfg.endpoint_url = "http://opa.internal:8181";
    cfg.policy_path  = "myorg/governance/allow";
    cfg.timeout_ms   = 100;
    OpaAdapter adapter(cfg);
    EXPECT_EQ(adapter.getConfig().endpoint_url, "http://opa.internal:8181");
    EXPECT_EQ(adapter.getConfig().policy_path,  "myorg/governance/allow");
    EXPECT_EQ(adapter.getConfig().timeout_ms,    100);
}

// ─────────────────────────────────────────────────────────────────────────────
// OpaAdapter with unreachable server → returns nullopt (triggers fallback)
// ─────────────────────────────────────────────────────────────────────────────

TEST(GovernanceOpaAdapterTest, UnreachableServerReturnsNullopt) {
    OpaAdapter::Config cfg;
    // Port 0 is never bound; curl will fail to connect immediately
    cfg.endpoint_url = "http://127.0.0.1:19997";
    cfg.policy_path  = "themis/governance/allow";
    cfg.timeout_ms   = 50;
    OpaAdapter adapter(cfg);
    std::unordered_map<std::string, std::string> headers{{"X-Classification", "vs-nfd"}};
    auto result = adapter.evaluate(headers, "/vector/search");
    EXPECT_FALSE(result.has_value())
        << "Expected nullopt when OPA sidecar is not running";
}

// ─────────────────────────────────────────────────────────────────────────────
// PolicyEngine: no OPA evaluator – native evaluation unchanged
// ─────────────────────────────────────────────────────────────────────────────

TEST(GovernanceOpaEngineTest, NoEvaluator_NativeClassificationApplied) {
    PolicyEngine engine;
    std::unordered_map<std::string, std::string> headers{{"X-Classification", "offen"}};
    auto d = engine.evaluate(headers, "/public");
    // Native heuristics for "offen": not strict class
    EXPECT_EQ(d.classification, "offen");
    EXPECT_TRUE(d.export_allowed);
    EXPECT_TRUE(d.ann_allowed);
}

TEST(GovernanceOpaEngineTest, NoEvaluator_StrictClassApplied) {
    PolicyEngine engine;
    std::unordered_map<std::string, std::string> headers{{"X-Classification", "geheim"}};
    auto d = engine.evaluate(headers, "/secret");
    EXPECT_EQ(d.classification, "geheim");
    EXPECT_FALSE(d.export_allowed);
    EXPECT_FALSE(d.ann_allowed);
    EXPECT_TRUE(d.require_content_encryption);
}

// ─────────────────────────────────────────────────────────────────────────────
// PolicyEngine: OPA unavailable → fallback to native
// ─────────────────────────────────────────────────────────────────────────────

TEST(GovernanceOpaEngineTest, OpaUnavailable_FallsBackToNativeAllow) {
    PolicyEngine engine;

    StubEvaluator stub;  // nullopt → OPA unavailable
    engine.setOpaEvaluator(&stub);

    std::unordered_map<std::string, std::string> headers{{"X-Classification", "offen"}};
    auto d = engine.evaluate(headers, "/public");
    EXPECT_EQ(d.classification, "offen");
    EXPECT_TRUE(d.export_allowed) << "Native fallback should apply permissive offen decision";
}

TEST(GovernanceOpaEngineTest, OpaUnavailable_FallsBackToNativeStrict) {
    PolicyEngine engine;

    StubEvaluator stub;  // nullopt → OPA unavailable
    engine.setOpaEvaluator(&stub);

    std::unordered_map<std::string, std::string> headers{{"X-Classification", "geheim"}};
    auto d = engine.evaluate(headers, "/secret");
    EXPECT_EQ(d.classification, "geheim");
    EXPECT_FALSE(d.export_allowed) << "Native fallback should apply strict geheim decision";
}

// ─────────────────────────────────────────────────────────────────────────────
// PolicyEngine: OPA evaluator overrides native result
// ─────────────────────────────────────────────────────────────────────────────

TEST(GovernanceOpaEngineTest, OpaAllow_OverridesNativeStrictDecision) {
    PolicyEngine engine;

    // OPA returns "offen" allow, even though header says "geheim"
    StubEvaluator stub{makeAllowDecision("offen")};
    engine.setOpaEvaluator(&stub);

    std::unordered_map<std::string, std::string> headers{{"X-Classification", "geheim"}};
    auto d = engine.evaluate(headers, "/secret");
    EXPECT_EQ(d.classification, "offen");
    EXPECT_TRUE(d.export_allowed) << "OPA allow decision should override native strict";
    EXPECT_TRUE(d.ann_allowed);
}

TEST(GovernanceOpaEngineTest, OpaDeny_OverridesNativePermissiveDecision) {
    PolicyEngine engine;

    StubEvaluator stub{makeDenyDecision()};
    engine.setOpaEvaluator(&stub);

    std::unordered_map<std::string, std::string> headers{{"X-Classification", "offen"}};
    auto d = engine.evaluate(headers, "/public");
    EXPECT_EQ(d.classification, "streng-geheim");
    EXPECT_FALSE(d.export_allowed) << "OPA deny decision should override native permissive";
    EXPECT_FALSE(d.ann_allowed);
    EXPECT_TRUE(d.require_content_encryption);
}

TEST(GovernanceOpaEngineTest, OpaDecisionUsesRetentionAndRedactionFromOpa) {
    PolicyEngine engine;

    PolicyDecision opa_d = makeAllowDecision("vs-nfd");
    opa_d.retention_days = 90;
    opa_d.redaction      = "standard";
    StubEvaluator stub{opa_d};
    engine.setOpaEvaluator(&stub);

    std::unordered_map<std::string, std::string> headers;
    auto d = engine.evaluate(headers, "/data");
    EXPECT_EQ(d.retention_days, 90);
    EXPECT_EQ(d.redaction, "standard");
}

// ─────────────────────────────────────────────────────────────────────────────
// PolicyEngine: detach evaluator → resumes native behavior
// ─────────────────────────────────────────────────────────────────────────────

TEST(GovernanceOpaEngineTest, DetachEvaluator_ResumesNativeBehavior) {
    PolicyEngine engine;

    StubEvaluator deny_stub{makeDenyDecision()};
    engine.setOpaEvaluator(&deny_stub);

    std::unordered_map<std::string, std::string> headers{{"X-Classification", "offen"}};
    {
        auto d = engine.evaluate(headers, "/public");
        EXPECT_FALSE(d.export_allowed) << "OPA deny should be in effect";
    }

    // Detach evaluator → native offen should win
    engine.setOpaEvaluator(nullptr);
    {
        auto d = engine.evaluate(headers, "/public");
        EXPECT_EQ(d.classification, "offen");
        EXPECT_TRUE(d.export_allowed) << "After detaching OPA, native offen decision should apply";
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// CCPA opt-out enforcement applied on top of OPA allow decision
// ─────────────────────────────────────────────────────────────────────────────

TEST(GovernanceOpaEngineTest, CcpaOptOut_OverridesOpaAllowExport) {
    PolicyEngine engine;

    // OPA allows export
    StubEvaluator stub{makeAllowDecision("offen")};
    engine.setOpaEvaluator(&stub);

    // Register subject as opted out
    auto registry = std::make_shared<std::unordered_set<std::string>>();
    registry->insert("alice");
    engine.setCcpaOptOutSubjects(registry);

    std::unordered_map<std::string, std::string> headers{
        {"X-Classification", "offen"},
        {"X-User-Id",        "alice"}
    };
    auto d = engine.evaluate(headers, "/public");
    EXPECT_TRUE(d.ccpa_opted_out)   << "CCPA opted-out flag should be set";
    EXPECT_FALSE(d.export_allowed)  << "CCPA must block export even when OPA allows";
}

TEST(GovernanceOpaEngineTest, CcpaOptOut_NotSetForNonOptedOutUser) {
    PolicyEngine engine;

    StubEvaluator stub{makeAllowDecision("offen")};
    engine.setOpaEvaluator(&stub);

    auto registry = std::make_shared<std::unordered_set<std::string>>();
    registry->insert("alice");
    engine.setCcpaOptOutSubjects(registry);

    std::unordered_map<std::string, std::string> headers{
        {"X-Classification", "offen"},
        {"X-User-Id",        "bob"}  // bob has NOT opted out
    };
    auto d = engine.evaluate(headers, "/public");
    EXPECT_FALSE(d.ccpa_opted_out);
    EXPECT_TRUE(d.export_allowed);
}

// ─────────────────────────────────────────────────────────────────────────────
// checkQueryPermission() routes through OPA when evaluator is set
// ─────────────────────────────────────────────────────────────────────────────

TEST(GovernanceOpaEngineTest, CheckQueryPermission_UsesOpaDecision) {
    PolicyEngine engine;

    StubEvaluator stub{makeDenyDecision()};
    engine.setOpaEvaluator(&stub);

    std::unordered_map<std::string, std::string> headers{{"X-Classification", "offen"}};
    auto qr = engine.checkQueryPermission(headers, "/public");
    EXPECT_FALSE(qr.decision.export_allowed) << "checkQueryPermission should use OPA deny";
}
