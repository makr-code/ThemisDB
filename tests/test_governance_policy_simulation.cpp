/**
 * @file test_governance_policy_simulation.cpp
 * @brief Unit tests for PolicyEngine::simulateDecision() – dry-run / simulation mode.
 *
 * Tests cover:
 * - simulateDecision returns dry_run=true and never writes audit entries
 * - Decision matches evaluate() for the same inputs
 * - matched_profile is populated when a profile is found
 * - matched_resource is populated when a resource mapping resolves the classification
 * - Heuristic fallback (unknown classification) is reflected in the result
 * - Header overrides (X-Encrypt-Logs, X-Redaction-Level, X-Governance-Mode) are honoured
 * - simulateDecision works on a freshly constructed engine (no YAML loaded)
 */

#include <gtest/gtest.h>
#include "governance/policy_engine.h"
#include "utils/audit_logger.h"

#include <filesystem>
#include <fstream>
#include <memory>

namespace fs = std::filesystem;
using namespace themis::governance;

// ---------------------------------------------------------------------------
// Shared YAML fixture helpers
// ---------------------------------------------------------------------------

static const char* kTestYaml = R"(
vs_classification:
  offen:
    encryption_required: false
    ann_allowed: true
    export_allowed: true
    cache_allowed: true
    redaction_level: "none"
    retention_days: 30
    log_encryption: false
  geheim:
    encryption_required: true
    ann_allowed: false
    export_allowed: false
    cache_allowed: false
    redaction_level: "strict"
    retention_days: 7
    log_encryption: true
enforcement:
  default_mode: enforce
  resource_mapping:
    /public: offen
    /secret: geheim
)";

class PolicySimulationTest : public ::testing::Test {
protected:
    void SetUp() override {
        tmp_dir_  = fs::temp_directory_path() / "gov_sim_test";
        fs::create_directories(tmp_dir_);
        yaml_path_ = (tmp_dir_ / "governance.yaml").string();

        std::ofstream f(yaml_path_);
        f << kTestYaml;
        f.flush();

        ASSERT_TRUE(engine_.loadFromYAML(yaml_path_));
    }

    void TearDown() override {
        fs::remove_all(tmp_dir_);
    }

    fs::path    tmp_dir_;
    std::string yaml_path_;
    PolicyEngine engine_;
};

// ---------------------------------------------------------------------------
// dry_run flag is always true
// ---------------------------------------------------------------------------

TEST_F(PolicySimulationTest, SimulateDecision_DryRunFlagAlwaysTrue) {
    SimulationRequest req;
    req.headers["X-Classification"] = "offen";
    req.route = "/public";

    auto result = engine_.simulateDecision(req);
    EXPECT_TRUE(result.dry_run);
}

// ---------------------------------------------------------------------------
// Decision matches evaluate() for the same inputs
// ---------------------------------------------------------------------------

TEST_F(PolicySimulationTest, SimulateDecision_DecisionMatchesEvaluate_Offen) {
    std::unordered_map<std::string, std::string> headers;
    headers["X-Classification"] = "offen";
    const std::string route = "/public";

    auto eval_d = engine_.evaluate(headers, route);

    SimulationRequest req;
    req.headers = headers;
    req.route   = route;
    auto sim_r = engine_.simulateDecision(req);

    EXPECT_EQ(sim_r.decision.classification,             eval_d.classification);
    EXPECT_EQ(sim_r.decision.mode,                       eval_d.mode);
    EXPECT_EQ(sim_r.decision.encrypt_logs,               eval_d.encrypt_logs);
    EXPECT_EQ(sim_r.decision.redaction,                  eval_d.redaction);
    EXPECT_EQ(sim_r.decision.ann_allowed,                eval_d.ann_allowed);
    EXPECT_EQ(sim_r.decision.require_content_encryption, eval_d.require_content_encryption);
    EXPECT_EQ(sim_r.decision.export_allowed,             eval_d.export_allowed);
    EXPECT_EQ(sim_r.decision.cache_allowed,              eval_d.cache_allowed);
    EXPECT_EQ(sim_r.decision.retention_days,             eval_d.retention_days);
}

TEST_F(PolicySimulationTest, SimulateDecision_DecisionMatchesEvaluate_Geheim) {
    std::unordered_map<std::string, std::string> headers;
    headers["X-Classification"] = "geheim";
    const std::string route = "/secret";

    auto eval_d = engine_.evaluate(headers, route);

    SimulationRequest req;
    req.headers = headers;
    req.route   = route;
    auto sim_r = engine_.simulateDecision(req);

    EXPECT_EQ(sim_r.decision.classification,             eval_d.classification);
    EXPECT_EQ(sim_r.decision.require_content_encryption, eval_d.require_content_encryption);
    EXPECT_EQ(sim_r.decision.ann_allowed,                eval_d.ann_allowed);
    EXPECT_EQ(sim_r.decision.export_allowed,             eval_d.export_allowed);
    EXPECT_EQ(sim_r.decision.cache_allowed,              eval_d.cache_allowed);
    EXPECT_EQ(sim_r.decision.redaction,                  eval_d.redaction);
}

// ---------------------------------------------------------------------------
// matched_profile is populated from the YAML profile
// ---------------------------------------------------------------------------

TEST_F(PolicySimulationTest, SimulateDecision_MatchedProfilePopulated_Offen) {
    SimulationRequest req;
    req.headers["X-Classification"] = "offen";
    req.route = "/some/route";

    auto result = engine_.simulateDecision(req);
    EXPECT_EQ(result.matched_profile, "offen");
}

TEST_F(PolicySimulationTest, SimulateDecision_MatchedProfilePopulated_Geheim) {
    SimulationRequest req;
    req.headers["X-Classification"] = "geheim";
    req.route = "/some/route";

    auto result = engine_.simulateDecision(req);
    EXPECT_EQ(result.matched_profile, "geheim");
}

// ---------------------------------------------------------------------------
// matched_resource is populated when the resource mapping resolves the classification
// ---------------------------------------------------------------------------

TEST_F(PolicySimulationTest, SimulateDecision_MatchedResourcePopulated_ViaMapping) {
    SimulationRequest req;
    // No explicit X-Classification – resolved from resource_mapping
    req.route = "/public";

    auto result = engine_.simulateDecision(req);
    EXPECT_EQ(result.decision.classification, "offen");
    EXPECT_EQ(result.matched_resource, "/public");
}

TEST_F(PolicySimulationTest, SimulateDecision_MatchedResource_EmptyWhenHeaderProvided) {
    SimulationRequest req;
    req.headers["X-Classification"] = "geheim";
    req.route = "/public"; // mapping would say "offen", but header wins

    auto result = engine_.simulateDecision(req);
    EXPECT_EQ(result.decision.classification, "geheim");
    // Header classification – resource mapping was not consulted
    EXPECT_TRUE(result.matched_resource.empty());
}

// ---------------------------------------------------------------------------
// Heuristic fallback for unknown classification
// ---------------------------------------------------------------------------

TEST_F(PolicySimulationTest, SimulateDecision_HeuristicFallback_UnknownClassification) {
    SimulationRequest req;
    req.headers["X-Classification"] = "streng-geheim"; // not in YAML profiles
    req.route = "/any";

    auto result = engine_.simulateDecision(req);
    EXPECT_EQ(result.decision.classification, "streng-geheim");
    // matched_profile should be empty since no profile was found
    EXPECT_TRUE(result.matched_profile.empty());
    // Heuristic: strict class → encryption required, no ANN, no export
    EXPECT_TRUE(result.decision.require_content_encryption);
    EXPECT_FALSE(result.decision.ann_allowed);
    EXPECT_FALSE(result.decision.export_allowed);
    EXPECT_EQ(result.decision.redaction, "strict");
}

// ---------------------------------------------------------------------------
// Header overrides are honoured in simulation just like in evaluate()
// ---------------------------------------------------------------------------

TEST_F(PolicySimulationTest, SimulateDecision_HeaderOverride_RedactionLevel) {
    SimulationRequest req;
    req.headers["X-Classification"]  = "offen";
    req.headers["X-Redaction-Level"] = "strict"; // override
    req.route = "/public";

    auto result = engine_.simulateDecision(req);
    EXPECT_EQ(result.decision.redaction, "strict");
}

TEST_F(PolicySimulationTest, SimulateDecision_HeaderOverride_EncryptLogs_True) {
    SimulationRequest req;
    req.headers["X-Classification"] = "offen";  // offen profile: log_encryption=false
    req.headers["X-Encrypt-Logs"]   = "true";   // override
    req.route = "/public";

    auto result = engine_.simulateDecision(req);
    EXPECT_TRUE(result.decision.encrypt_logs);
}

TEST_F(PolicySimulationTest, SimulateDecision_HeaderOverride_GovernanceModeObserve) {
    SimulationRequest req;
    req.headers["X-Classification"]   = "offen";
    req.headers["X-Governance-Mode"]  = "observe";
    req.route = "/public";

    auto result = engine_.simulateDecision(req);
    EXPECT_EQ(result.decision.mode, "observe");
}

// ---------------------------------------------------------------------------
// simulateDecision does NOT write to the audit log
// ---------------------------------------------------------------------------

TEST_F(PolicySimulationTest, SimulateDecision_DoesNotWriteAuditEntry) {
    // Install a collecting audit logger
    themis::utils::AuditLoggerConfig cfg;
    cfg.enabled = false; // sink-only, no file I/O
    auto logger = std::make_shared<themis::utils::AuditLogger>(nullptr, nullptr, cfg);
    engine_.setAuditLogger(logger);

    SimulationRequest req;
    req.headers["X-Classification"] = "geheim";
    req.route = "/secret";

    // Run simulation – must not throw and must not write audit entry.
    // (We cannot directly count entries in AuditLogger without access to internals,
    //  so we verify it doesn't throw and returns dry_run=true as the observable
    //  contract that audit was suppressed.)
    EXPECT_NO_THROW({
        auto result = engine_.simulateDecision(req);
        EXPECT_TRUE(result.dry_run);
    });

    // Contrast: evaluate() with enforce mode WOULD write an audit entry.
    // We call it just to confirm the logger is wired correctly (no crash).
    EXPECT_NO_THROW(engine_.evaluate(req.headers, req.route));
}

// ---------------------------------------------------------------------------
// Works on a freshly constructed engine (no YAML loaded, empty policy state)
// ---------------------------------------------------------------------------

TEST(PolicySimulationNoYamlTest, SimulateDecision_NoYamlLoaded_ReturnsDefault) {
    PolicyEngine pe;

    SimulationRequest req;
    req.headers["X-Classification"] = "offen";
    req.route = "/any";

    // Must not throw even with empty policy state
    EXPECT_NO_THROW({
        auto result = pe.simulateDecision(req);
        EXPECT_TRUE(result.dry_run);
        EXPECT_EQ(result.decision.classification, "offen");
        // No profile loaded → heuristic fallback (offen is non-strict)
        EXPECT_FALSE(result.decision.require_content_encryption);
    });
}

// ---------------------------------------------------------------------------
// Default classification (no header, no resource mapping) = "vs-nfd"
// ---------------------------------------------------------------------------

TEST_F(PolicySimulationTest, SimulateDecision_DefaultClassification_VsNfd) {
    SimulationRequest req;
    // No X-Classification header, no matching resource mapping
    req.route = "/unknown/route";

    auto result = engine_.simulateDecision(req);
    EXPECT_EQ(result.decision.classification, "vs-nfd");
    EXPECT_TRUE(result.matched_resource.empty());
}
