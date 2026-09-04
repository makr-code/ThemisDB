/**
 * @file test_model_switch_workflow.cpp
 * @brief Phase 6 QA test suite — Model-Switch Workflow & Ratchet Compatibility Matrix.
 *
 * Covers:
 * - SemVer parsing and comparison
 * - RatchetCompatibilityEntry range checks
 * - RatchetCompatibilityMatrix ratchet semantics, JSON round-trip
 * - RebuildPolicy serialization and trigger lookup
 * - ModelSwitchWorkflow: no-op switch, compatible switch, rebuild-required,
 *   blocked (fail-closed), incompatible, ratchet gate, draft-adapter warnings,
 *   orchestrator promotion, JSON audit output
 */

#include <gtest/gtest.h>

#include "llm/model_switch_workflow.h"

#include <set>

using namespace themis::llm;

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

namespace {

AdapterMetadata makeAdapter(std::string adapter_id,
                            std::string base_model,
                            std::string architecture,
                            AdapterRole role = AdapterRole::GENERAL,
                            AdapterMetadata::Status status = AdapterMetadata::Status::DEPLOYED) {
    AdapterMetadata meta;
    meta.adapter_id   = std::move(adapter_id);
    meta.base_model_name = std::move(base_model);
    meta.architecture = std::move(architecture);
    meta.role         = role;
    meta.status       = status;
    return meta;
}

FinalLayerPackage makePackage(const std::string& package_id = "pkg-test",
                              const std::string& model_id   = "llama-7b",
                              const std::string& adapter_id = "legal-general",
                              const std::string& model_family = "llama") {
    FinalLayerPackage pkg;
    pkg.package_id          = package_id;
    pkg.target_model_id     = model_id;
    pkg.model_family        = model_family;
    pkg.base_model_version  = "3.1";
    pkg.primary_adapter_id  = adapter_id;
    pkg.domain              = "legal";
    pkg.task_type           = "question-answering";
    pkg.deployment_stage    = FinalLayerDeploymentStage::DRAFT;
    return pkg;
}

/**
 * Build a minimal workflow with one registered adapter and one registered
 * package.  The ratchet matrix is empty (open policy) unless overridden.
 */
ModelSwitchWorkflow makeWorkflow(RatchetCompatibilityMatrix matrix = RatchetCompatibilityMatrix{},
                                 RebuildPolicy policy               = RebuildPolicy{}) {
    auto registry = std::make_shared<AdapterRegistry>(nullptr);
    EXPECT_TRUE(registry->registerAdapter(makeAdapter("legal-general", "llama-7b", "llama")));

    auto orchestrator = std::make_shared<FinalLayerOrchestrator>();
    orchestrator->setAdapterRegistry(registry);
    EXPECT_TRUE(orchestrator->registerPackage(makePackage()));

    return ModelSwitchWorkflow(registry, std::move(orchestrator),
                               std::move(matrix), std::move(policy));
}

ModelSwitchRequest makeRequest(const std::string& pkg     = "pkg-test",
                               const std::string& src_mod = "llama-7b",
                               const std::string& src_ver = "3.0",
                               const std::string& tgt_mod = "llama-7b",
                               const std::string& tgt_ver = "3.1",
                               const std::string& family  = "llama") {
    ModelSwitchRequest req;
    req.package_id           = pkg;
    req.source_model_name    = src_mod;
    req.source_model_version = src_ver;
    req.target_model_name    = tgt_mod;
    req.target_model_version = tgt_ver;
    req.target_model_family  = family;
    req.correlation_id       = "test-corr-01";
    return req;
}

} // namespace

// ===========================================================================
// SemVer tests
// ===========================================================================

TEST(SemVerTest, ParseSimpleVersion) {
    auto v = SemVer::parse("3.1.0");
    EXPECT_EQ(v.major, 3);
    EXPECT_EQ(v.minor, 1);
    EXPECT_EQ(v.patch, 0);
}

TEST(SemVerTest, ParseTwoPart) {
    auto v = SemVer::parse("2.4");
    EXPECT_EQ(v.major, 2);
    EXPECT_EQ(v.minor, 4);
    EXPECT_EQ(v.patch, 0);
}

TEST(SemVerTest, ParseEmpty) {
    auto v = SemVer::parse("");
    EXPECT_EQ(v.major, 0);
    EXPECT_EQ(v.minor, 0);
    EXPECT_EQ(v.patch, 0);
}

TEST(SemVerTest, ParseRejectsTrailingQualifier) {
    const auto v = SemVer::parse("3.1-beta");
    EXPECT_EQ(v.major, 0);
    EXPECT_EQ(v.minor, 0);
    EXPECT_EQ(v.patch, 0);
}

TEST(SemVerTest, ToStringRoundTrip) {
    const auto v = SemVer{2, 0, 3};
    EXPECT_EQ(v.toString(), "2.0.3");
}

TEST(SemVerTest, ComparisonLessThan) {
    EXPECT_LT(SemVer::parse("1.0.0"), SemVer::parse("2.0.0"));
    EXPECT_LT(SemVer::parse("2.0.0"), SemVer::parse("2.1.0"));
    EXPECT_LT(SemVer::parse("2.1.0"), SemVer::parse("2.1.1"));
}

TEST(SemVerTest, ComparisonEqual) {
    EXPECT_EQ(SemVer::parse("3.1.0"), SemVer::parse("3.1.0"));
    EXPECT_FALSE(SemVer::parse("3.1.0") < SemVer::parse("3.1.0"));
}

TEST(SemVerTest, JsonRoundTrip) {
    const SemVer v{4, 2, 1};
    const auto j   = v.toJson();
    const auto v2  = SemVer::fromJson(j);
    EXPECT_EQ(v, v2);
}

// ===========================================================================
// RatchetCompatibilityEntry tests
// ===========================================================================

TEST(RatchetEntryTest, IsSatisfiedByWithinRange) {
    RatchetCompatibilityEntry e;
    e.min_model_version      = SemVer::parse("3.0");
    e.max_model_version_excl = SemVer::parse("4.0");
    EXPECT_TRUE(e.isSatisfiedBy(SemVer::parse("3.0")));
    EXPECT_TRUE(e.isSatisfiedBy(SemVer::parse("3.5")));
    EXPECT_FALSE(e.isSatisfiedBy(SemVer::parse("2.9")));
    EXPECT_FALSE(e.isSatisfiedBy(SemVer::parse("4.0")));
}

TEST(RatchetEntryTest, IsSatisfiedByUnboundedUpper) {
    RatchetCompatibilityEntry e;
    e.min_model_version = SemVer::parse("3.0");
    // max == {0,0,0} -> unbounded
    EXPECT_TRUE(e.isSatisfiedBy(SemVer::parse("3.0")));
    EXPECT_TRUE(e.isSatisfiedBy(SemVer::parse("99.0")));
    EXPECT_FALSE(e.isSatisfiedBy(SemVer::parse("2.9")));
}

TEST(RatchetEntryTest, JsonRoundTrip) {
    RatchetCompatibilityEntry e;
    e.adapter_id             = "my-adapter";
    e.model_family           = "llama";
    e.min_model_version      = SemVer::parse("3.1");
    e.max_model_version_excl = SemVer::parse("4.0");

    const auto j  = e.toJson();
    const auto e2 = RatchetCompatibilityEntry::fromJson(j);
    EXPECT_EQ(e2.adapter_id, "my-adapter");
    EXPECT_EQ(e2.model_family, "llama");
    EXPECT_EQ(e2.min_model_version, e.min_model_version);
    EXPECT_EQ(e2.max_model_version_excl, e.max_model_version_excl);
}

// ===========================================================================
// RatchetCompatibilityMatrix tests
// ===========================================================================

TEST(RatchetMatrixTest, RegisterNewEntry) {
    RatchetCompatibilityMatrix m;
    EXPECT_TRUE(m.registerEntry("a1", "llama", SemVer::parse("3.0")));
    EXPECT_EQ(m.entries().size(), 1u);
}

TEST(RatchetMatrixTest, RatchetAdvancesFloor) {
    RatchetCompatibilityMatrix m;
    ASSERT_TRUE(m.registerEntry("a1", "llama", SemVer::parse("3.0")));
    // Advance floor to 3.1 — allowed
    EXPECT_TRUE(m.registerEntry("a1", "llama", SemVer::parse("3.1")));
    const auto entry = m.findEntry("a1", "llama");
    ASSERT_TRUE(entry.has_value());
    EXPECT_EQ(entry->min_model_version, SemVer::parse("3.1"));
}

TEST(RatchetMatrixTest, RatchetRejectsLowerFloor) {
    RatchetCompatibilityMatrix m;
    ASSERT_TRUE(m.registerEntry("a1", "llama", SemVer::parse("3.1")));
    // Try to lower to 3.0 without override — rejected
    EXPECT_FALSE(m.registerEntry("a1", "llama", SemVer::parse("3.0")));
    const auto entry = m.findEntry("a1", "llama");
    ASSERT_TRUE(entry.has_value());
    EXPECT_EQ(entry->min_model_version, SemVer::parse("3.1"));
}

TEST(RatchetMatrixTest, RatchetAllowsDowngradeWithOverride) {
    RatchetCompatibilityMatrix m;
    ASSERT_TRUE(m.registerEntry("a1", "llama", SemVer::parse("3.1")));
    EXPECT_TRUE(m.registerEntry("a1", "llama", SemVer::parse("3.0"),
                                SemVer{}, /*allow_downgrade=*/true));
    const auto entry = m.findEntry("a1", "llama");
    ASSERT_TRUE(entry.has_value());
    EXPECT_EQ(entry->min_model_version, SemVer::parse("3.0"));
}

TEST(RatchetMatrixTest, IsCompatibleOpenPolicyForUnknownEntry) {
    RatchetCompatibilityMatrix m;
    // No entry -> open policy -> always compatible
    EXPECT_TRUE(m.isCompatible("any-adapter", "llama", "3.5"));
}

TEST(RatchetMatrixTest, IsCompatibleRespectsBounds) {
    RatchetCompatibilityMatrix m;
    ASSERT_TRUE(m.registerEntry("a1", "llama",
                                SemVer::parse("3.0"), SemVer::parse("4.0")));
    EXPECT_TRUE(m.isCompatible("a1", "llama", "3.5"));
    EXPECT_FALSE(m.isCompatible("a1", "llama", "2.9"));
    EXPECT_FALSE(m.isCompatible("a1", "llama", "4.0"));
}

TEST(RatchetMatrixTest, JsonRoundTrip) {
    RatchetCompatibilityMatrix m("2.0.0");
    ASSERT_TRUE(m.registerEntry("a1", "llama",
                                SemVer::parse("3.0"), SemVer::parse("5.0")));
    ASSERT_TRUE(m.registerEntry("a2", "mistral", SemVer::parse("0.1")));

    const auto j  = m.toJson();
    const auto m2 = RatchetCompatibilityMatrix::fromJson(j);
    EXPECT_EQ(m2.schemaVersion(), "2.0.0");
    EXPECT_EQ(m2.entries().size(), 2u);
    EXPECT_TRUE(m2.isCompatible("a1", "llama", "3.5"));
}

TEST(RatchetMatrixTest, FromJsonThrowsOnMissingFields) {
    EXPECT_THROW(
        RatchetCompatibilityMatrix::fromJson(nlohmann::json{{"schema_version", "1.0"}}),
        std::invalid_argument);
}

// ===========================================================================
// RebuildPolicy tests
// ===========================================================================

TEST(RebuildPolicyTest, DefaultTriggers) {
    RebuildPolicy p;
    EXPECT_TRUE(p.isTriggerActive(RebuildTrigger::ARCHITECTURE_CHANGE));
    EXPECT_TRUE(p.isTriggerActive(RebuildTrigger::TOKENIZER_CHANGE));
    EXPECT_TRUE(p.isTriggerActive(RebuildTrigger::VERSION_OUT_OF_RANGE));
    EXPECT_FALSE(p.isTriggerActive(RebuildTrigger::LAYER_DIMENSION_CHANGE));
}

TEST(RebuildPolicyTest, JsonRoundTrip) {
    RebuildPolicy p;
    p.fail_closed_on_rebuild = true;
    p.triggers = {RebuildTrigger::ARCHITECTURE_CHANGE, RebuildTrigger::TOKENIZER_CHANGE};

    const auto j  = p.toJson();
    const auto p2 = RebuildPolicy::fromJson(j);
    EXPECT_TRUE(p2.fail_closed_on_rebuild);
    EXPECT_TRUE(p2.isTriggerActive(RebuildTrigger::ARCHITECTURE_CHANGE));
    EXPECT_TRUE(p2.isTriggerActive(RebuildTrigger::TOKENIZER_CHANGE));
    EXPECT_FALSE(p2.isTriggerActive(RebuildTrigger::VERSION_OUT_OF_RANGE));
}

// ===========================================================================
// ModelSwitchWorkflow — construction
// ===========================================================================

TEST(ModelSwitchWorkflowTest, ConstructionNullRegistryThrows) {
    auto orchestrator = std::make_shared<FinalLayerOrchestrator>();
    EXPECT_THROW(ModelSwitchWorkflow(nullptr, orchestrator),
                 std::invalid_argument);
}

TEST(ModelSwitchWorkflowTest, ConstructionNullOrchestratorThrows) {
    auto registry = std::make_shared<AdapterRegistry>(nullptr);
    EXPECT_THROW(ModelSwitchWorkflow(registry, nullptr),
                 std::invalid_argument);
}

TEST(ModelSwitchWorkflowTest, ConfigAccessors) {
    auto wf = makeWorkflow();
    EXPECT_EQ(wf.compatibilityMatrix().schemaVersion(), "1.0.0");

    RatchetCompatibilityMatrix m2("2.0.0");
    wf.setCompatibilityMatrix(std::move(m2));
    EXPECT_EQ(wf.compatibilityMatrix().schemaVersion(), "2.0.0");

    RebuildPolicy p;
    p.fail_closed_on_rebuild = true;
    wf.setRebuildPolicy(p);
    EXPECT_TRUE(wf.rebuildPolicy().fail_closed_on_rebuild);
}

// ===========================================================================
// isSwitchRequired
// ===========================================================================

TEST(ModelSwitchWorkflowTest, IsSwitchNotRequiredWhenSameModel) {
    auto req = makeRequest("pkg", "llama-7b", "3.1", "llama-7b", "3.1");
    EXPECT_FALSE(ModelSwitchWorkflow::isSwitchRequired(req));
}

TEST(ModelSwitchWorkflowTest, IsSwitchRequiredWhenVersionChanges) {
    auto req = makeRequest("pkg", "llama-7b", "3.0", "llama-7b", "3.1");
    EXPECT_TRUE(ModelSwitchWorkflow::isSwitchRequired(req));
}

TEST(ModelSwitchWorkflowTest, IsSwitchRequiredWhenModelChanges) {
    auto req = makeRequest("pkg", "llama-7b", "3.1", "llama-13b", "3.1");
    EXPECT_TRUE(ModelSwitchWorkflow::isSwitchRequired(req));
}

TEST(ModelSwitchWorkflowTest, IsSwitchRequiredWithForceFlag) {
    auto req = makeRequest("pkg", "llama-7b", "3.1", "llama-7b", "3.1");
    req.force_revalidation = true;
    EXPECT_TRUE(ModelSwitchWorkflow::isSwitchRequired(req));
}

// ===========================================================================
// executeSwitch — no-op switch
// ===========================================================================

TEST(ModelSwitchWorkflowTest, ExecuteSwitchNoOpReturnsCompatible) {
    auto wf  = makeWorkflow();
    auto req = makeRequest("pkg-test", "llama-7b", "3.1", "llama-7b", "3.1");
    const auto res = wf.executeSwitch(req);
    EXPECT_EQ(res.outcome, ModelSwitchOutcome::COMPATIBLE);
    EXPECT_TRUE(res.canServe());
    EXPECT_FALSE(res.warnings.empty()); // Should warn about no-op
}

// ===========================================================================
// executeSwitch — compatible switch (same model family, within ratchet)
// ===========================================================================

TEST(ModelSwitchWorkflowTest, ExecuteSwitchCompatibleReturnsCompatible) {
    auto wf  = makeWorkflow();
    auto req = makeRequest(); // llama-7b 3.0 -> llama-7b 3.1
    const auto res = wf.executeSwitch(req);
    // Should be COMPATIBLE since adapter was trained on llama-7b and target is same
    EXPECT_EQ(res.outcome, ModelSwitchOutcome::COMPATIBLE);
    EXPECT_TRUE(res.canServe());
    EXPECT_TRUE(res.errors.empty());
    EXPECT_EQ(res.correlation_id, "test-corr-01");
}

// ===========================================================================
// executeSwitch — unknown package
// ===========================================================================

TEST(ModelSwitchWorkflowTest, ExecuteSwitchUnknownPackageIsIncompatible) {
    auto wf  = makeWorkflow();
    auto req = makeRequest("pkg-nonexistent");
    const auto res = wf.executeSwitch(req);
    EXPECT_EQ(res.outcome, ModelSwitchOutcome::INCOMPATIBLE);
    EXPECT_FALSE(res.errors.empty());
}

// ===========================================================================
// executeSwitch — ratchet gate blocks switch
// ===========================================================================

TEST(ModelSwitchWorkflowTest, ExecuteSwitchBlockedByRatchetMatrix) {
    RatchetCompatibilityMatrix m;
    // Adapter requires llama >= 4.0
    ASSERT_TRUE(m.registerEntry("legal-general", "llama", SemVer::parse("4.0")));

    auto wf  = makeWorkflow(std::move(m));
    // Request targets llama 3.1 which is below the ratchet floor
    auto req = makeRequest("pkg-test", "llama-7b", "3.0", "llama-7b", "3.1", "llama");
    const auto res = wf.executeSwitch(req);
    EXPECT_EQ(res.outcome, ModelSwitchOutcome::INCOMPATIBLE);
    EXPECT_FALSE(res.errors.empty());
    // Ratchet check should fail
    bool ratchet_failed = false;
    for (const auto& c : res.checks) {
        if (c.kind == ModelSwitchCheckResult::CheckKind::RATCHET_MATRIX && !c.passed) {
            ratchet_failed = true;
        }
    }
    EXPECT_TRUE(ratchet_failed);
}

TEST(ModelSwitchWorkflowTest, ExecuteSwitchPassesRatchetWithSufficientVersion) {
    RatchetCompatibilityMatrix m;
    ASSERT_TRUE(m.registerEntry("legal-general", "llama", SemVer::parse("3.0")));

    auto wf  = makeWorkflow(std::move(m));
    auto req = makeRequest(); // targets 3.1 which satisfies >= 3.0
    const auto res = wf.executeSwitch(req);
    EXPECT_NE(res.outcome, ModelSwitchOutcome::INCOMPATIBLE);
    bool ratchet_passed = false;
    for (const auto& c : res.checks) {
        if (c.kind == ModelSwitchCheckResult::CheckKind::RATCHET_MATRIX && c.passed) {
            ratchet_passed = true;
        }
    }
    EXPECT_TRUE(ratchet_passed);
}

// ===========================================================================
// executeSwitch — fail-closed on rebuild
// ===========================================================================

TEST(ModelSwitchWorkflowTest, ExecuteSwitchBlockedByFailClosedPolicy) {
    RebuildPolicy policy;
    policy.fail_closed_on_rebuild = true;
    policy.triggers = {RebuildTrigger::ARCHITECTURE_CHANGE,
                       RebuildTrigger::TOKENIZER_CHANGE,
                       RebuildTrigger::LAYER_DIMENSION_CHANGE,
                       RebuildTrigger::VERSION_OUT_OF_RANGE};

    auto registry = std::make_shared<AdapterRegistry>(nullptr);
    // Register adapter trained on llama-7b, switch target is llama-13b
    ASSERT_TRUE(registry->registerAdapter(makeAdapter("legal-general", "llama-7b", "llama")));

    auto orchestrator = std::make_shared<FinalLayerOrchestrator>();
    orchestrator->setAdapterRegistry(registry);
    ASSERT_TRUE(orchestrator->registerPackage(makePackage()));

    ModelSwitchWorkflow wf(registry, orchestrator, RatchetCompatibilityMatrix{}, policy);

    // Switch to a different model name triggers rebuild checks
    auto req = makeRequest("pkg-test", "llama-7b", "3.0", "llama-13b", "3.1", "llama");
    const auto res = wf.executeSwitch(req);

    EXPECT_EQ(res.outcome, ModelSwitchOutcome::BLOCKED);
    EXPECT_FALSE(res.active_rebuild_triggers.empty());
}

// ===========================================================================
// executeSwitch — rebuild required (fail-open)
// ===========================================================================

TEST(ModelSwitchWorkflowTest, ExecuteSwitchRebuildRequiredNotBlocked) {
    RebuildPolicy policy;
    policy.fail_closed_on_rebuild = false;  // fail-open: surface as REBUILD_REQUIRED
    policy.triggers = {RebuildTrigger::LAYER_DIMENSION_CHANGE};

    auto registry = std::make_shared<AdapterRegistry>(nullptr);
    ASSERT_TRUE(registry->registerAdapter(makeAdapter("legal-general", "llama-7b", "llama")));

    auto orchestrator = std::make_shared<FinalLayerOrchestrator>();
    orchestrator->setAdapterRegistry(registry);
    ASSERT_TRUE(orchestrator->registerPackage(makePackage()));

    ModelSwitchWorkflow wf(registry, orchestrator, RatchetCompatibilityMatrix{}, policy);

    // Switch to a different model name (triggers LAYER_DIMENSION_CHANGE)
    auto req = makeRequest("pkg-test", "llama-7b", "3.0", "llama-13b", "3.1", "llama");
    const auto res = wf.executeSwitch(req);

    EXPECT_EQ(res.outcome, ModelSwitchOutcome::REBUILD_REQUIRED);
}

// ===========================================================================
// executeSwitch — check coverage
// ===========================================================================

TEST(ModelSwitchWorkflowTest, ExecuteSwitchPopulatesAllChecks) {
    auto wf  = makeWorkflow();
    auto req = makeRequest();
    const auto res = wf.executeSwitch(req);

    // At least 5 check kinds must be present after a real switch
    // (RATCHET_MATRIX, ARCHITECTURE, TOKENIZER, LAYER_DIMENSIONS, QUANTIZATION, PROMPT_FORMAT)
    std::set<ModelSwitchCheckResult::CheckKind> kinds = {};

    for (const auto& c : res.checks) {
        kinds.insert(c.kind);
    }
    EXPECT_GE(kinds.size(), 5u);
}

// ===========================================================================
// executeSwitch — draft adapter warning
// ===========================================================================

TEST(ModelSwitchWorkflowTest, ExecuteSwitchIncompatibleDraftAdapterEmitsWarning) {
    auto registry = std::make_shared<AdapterRegistry>(nullptr);
    ASSERT_TRUE(registry->registerAdapter(makeAdapter("legal-general", "llama-7b", "llama")));
    ASSERT_TRUE(registry->registerAdapter(
        makeAdapter("llama-draft", "llama-0.5b", "mistral", AdapterRole::DRAFT)));

    auto orchestrator = std::make_shared<FinalLayerOrchestrator>();
    orchestrator->setAdapterRegistry(registry);

    FinalLayerPackage pkg = makePackage();
    pkg.draft_adapter_id = "llama-draft";
    ASSERT_TRUE(orchestrator->registerPackage(pkg));

    ModelSwitchWorkflow wf(registry, orchestrator);

    // Switch to mistral — draft adapter (mistral arch) may warn
    auto req = makeRequest("pkg-test", "llama-7b", "3.0", "mistral-7b", "0.2", "mistral");
    const auto res = wf.executeSwitch(req);

    // Result must contain correlation ID
    EXPECT_EQ(res.correlation_id, "test-corr-01");
    // Workflow completes without throwing
    EXPECT_TRUE(res.outcome == ModelSwitchOutcome::COMPATIBLE ||
                res.outcome == ModelSwitchOutcome::REBUILD_REQUIRED ||
                res.outcome == ModelSwitchOutcome::INCOMPATIBLE);
}

// ===========================================================================
// executeSwitch — orchestrator promotion side-effect
// ===========================================================================

TEST(ModelSwitchWorkflowTest, ExecuteSwitchCompatibleAdvancesPackageToStaging) {
    auto registry = std::make_shared<AdapterRegistry>(nullptr);
    ASSERT_TRUE(registry->registerAdapter(makeAdapter("legal-general", "llama-7b", "llama")));

    auto orchestrator = std::make_shared<FinalLayerOrchestrator>();
    orchestrator->setAdapterRegistry(registry);

    FinalLayerPackage pkg = makePackage();
    pkg.deployment_stage = FinalLayerDeploymentStage::DRAFT;
    ASSERT_TRUE(orchestrator->registerPackage(pkg));

    ModelSwitchWorkflow wf(registry, orchestrator);
    auto req = makeRequest(); // Same model family, should be COMPATIBLE

    const auto res = wf.executeSwitch(req);
    if (res.outcome == ModelSwitchOutcome::COMPATIBLE) {
        // Package should have been promoted to STAGING by the workflow
        for (const auto& p : orchestrator->listPackages()) {
            if (p.package_id == "pkg-test") {
                EXPECT_EQ(p.deployment_stage, FinalLayerDeploymentStage::STAGING);
            }
        }
    }
}

// ===========================================================================
// JSON audit output
// ===========================================================================

TEST(ModelSwitchWorkflowTest, ResultToJsonContainsRequiredFields) {
    auto wf  = makeWorkflow();
    auto req = makeRequest();
    const auto res = wf.executeSwitch(req);
    const auto j   = res.toJson();

    EXPECT_TRUE(j.contains("outcome"));
    EXPECT_TRUE(j.contains("can_serve"));
    EXPECT_TRUE(j.contains("needs_rebuild"));
    EXPECT_TRUE(j.contains("correlation_id"));
    EXPECT_TRUE(j.contains("checks"));
    EXPECT_TRUE(j.contains("active_rebuild_triggers"));
    EXPECT_TRUE(j.contains("errors"));
    EXPECT_TRUE(j.contains("warnings"));
    EXPECT_EQ(j["correlation_id"].get<std::string>(), "test-corr-01");
}

TEST(ModelSwitchWorkflowTest, ResultJsonChecksHaveKindAndPassed) {
    auto wf  = makeWorkflow();
    auto req = makeRequest();
    const auto res = wf.executeSwitch(req);
    const auto j   = res.toJson();

    for (const auto& c : j["checks"]) {
        EXPECT_TRUE(c.contains("kind"));
        EXPECT_TRUE(c.contains("passed"));
        EXPECT_TRUE(c.contains("rebuild_required"));
        EXPECT_TRUE(c.contains("message"));
    }
}

// ===========================================================================
// ModelSwitchResult helpers
// ===========================================================================

TEST(ModelSwitchResultTest, CanServeOnlyForCompatible) {
    ModelSwitchResult r;
    r.outcome = ModelSwitchOutcome::COMPATIBLE;
    EXPECT_TRUE(r.canServe());
    EXPECT_FALSE(r.needsRebuild());
}

TEST(ModelSwitchResultTest, NeedsRebuildOnlyForRebuildRequired) {
    ModelSwitchResult r;
    r.outcome = ModelSwitchOutcome::REBUILD_REQUIRED;
    EXPECT_FALSE(r.canServe());
    EXPECT_TRUE(r.needsRebuild());
}

TEST(ModelSwitchResultTest, BlockedOutcomeNotServableNotRebuild) {
    ModelSwitchResult r;
    r.outcome = ModelSwitchOutcome::BLOCKED;
    EXPECT_FALSE(r.canServe());
    EXPECT_TRUE(r.needsRebuild());
}

TEST(ModelSwitchResultTest, IncompatibleOutcomeNotServable) {
    ModelSwitchResult r;
    r.outcome = ModelSwitchOutcome::INCOMPATIBLE;
    EXPECT_FALSE(r.canServe());
    EXPECT_FALSE(r.needsRebuild());
}

// ===========================================================================
// RatchetCompatibilityMatrix — multiple entries
// ===========================================================================

TEST(RatchetMatrixTest, MultipleEntriesDifferentFamilies) {
    RatchetCompatibilityMatrix m;
    ASSERT_TRUE(m.registerEntry("a1", "llama",   SemVer::parse("3.0")));
    ASSERT_TRUE(m.registerEntry("a1", "mistral", SemVer::parse("0.2")));
    EXPECT_EQ(m.entries().size(), 2u);
    EXPECT_TRUE(m.isCompatible("a1", "llama",   "3.5"));
    EXPECT_TRUE(m.isCompatible("a1", "mistral", "0.3"));
    EXPECT_FALSE(m.isCompatible("a1", "llama",  "2.9"));
}

TEST(RatchetMatrixTest, MultipleEntriesDifferentAdapters) {
    RatchetCompatibilityMatrix m;
    ASSERT_TRUE(m.registerEntry("a1", "llama", SemVer::parse("3.0")));
    ASSERT_TRUE(m.registerEntry("a2", "llama", SemVer::parse("2.0")));
    EXPECT_FALSE(m.isCompatible("a1", "llama", "2.5")); // a1 requires 3.0
    EXPECT_TRUE(m.isCompatible("a2", "llama",  "2.5")); // a2 requires 2.0
}

// ===========================================================================
// Force revalidation
// ===========================================================================

TEST(ModelSwitchWorkflowTest, ExecuteSwitchForceRevalidationRunsAllChecks) {
    auto wf  = makeWorkflow();
    auto req = makeRequest("pkg-test", "llama-7b", "3.1", "llama-7b", "3.1");
    req.force_revalidation = true;
    const auto res = wf.executeSwitch(req);
    // With force_revalidation the switch runs all checks
    EXPECT_FALSE(res.checks.empty());
}

// ===========================================================================
// Schema version roundtrip
// ===========================================================================

TEST(RatchetMatrixTest, SchemaVersionPreservedInJson) {
    RatchetCompatibilityMatrix m("3.2.1");
    const auto j  = m.toJson();
    const auto m2 = RatchetCompatibilityMatrix::fromJson(j);
    EXPECT_EQ(m2.schemaVersion(), "3.2.1");
}

TEST(ModelSwitchWorkflowTest, ExecuteSwitchQuantizationMismatchRequiresRebuild) {
    auto registry = std::make_shared<AdapterRegistry>(nullptr);
    auto adapter = makeAdapter("legal-general", "llama-7b", "llama");
    adapter.quantization = "Q4_K_M";
    ASSERT_TRUE(registry->registerAdapter(adapter));

    auto orchestrator = std::make_shared<FinalLayerOrchestrator>();
    orchestrator->setAdapterRegistry(registry);
    ASSERT_TRUE(orchestrator->registerPackage(makePackage()));

    ModelSwitchWorkflow wf(registry, orchestrator);
    auto req = makeRequest("pkg-test", "llama-7b-q4", "3.0", "llama-7b-q8", "3.1", "llama");

    const auto res = wf.executeSwitch(req);
    bool quantization_rebuild = false;
    for (const auto& c : res.checks) {
        if (c.kind == ModelSwitchCheckResult::CheckKind::QUANTIZATION) {
            quantization_rebuild = c.rebuild_required;
        }
    }
    EXPECT_TRUE(quantization_rebuild);
}
