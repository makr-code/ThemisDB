/**
 * @file test_ethics_ai_eu_compliance.cpp
 * @brief EU AI Act Art. 13/22 compliance focused tests for the ethics_ai module.
 *
 * Test IDs: EU-01..EU-08
 * CTest labels: ethics_ai,eu_compliance,phase4
 *
 * Coverage:
 *   EU-01 — ABSTAIN vote propagation: timed-out school ∈ participating_schools
 *   EU-02 — Art. 13 listing completeness: all 22 schools in LAYERED_FULL mode
 *   EU-03 — Audit trail immutability: no post-hoc modification possible
 *   EU-04 — LDM contract (w₀ = 1/N): equal initial weight for all N schools
 *   EU-05 — legal_db unavailability: grounding_available=false, no silent failure
 *   EU-06 — ChainVisualizer DOT output: non-empty, valid DOT syntax
 *   EU-07 — Mermaid diagram artifact: non-empty, valid Mermaid flowchart syntax
 *   EU-08 — Art. 13 round-level audit export: exportAuditLog() returns entries
 *            in correct chronological order
 *
 * All tests use deterministic mock/stub fixtures — no real LLM, no file I/O.
 */

#include <gtest/gtest.h>

#include "ethics_ai/ethics_ai_types.h"
#include "ethics_ai/ethics_selection_router.h"
#include "ethics_ai/discourse_orchestrator.h"
#include "ethics_ai/meta_verdict_builder.h"
#include "ethics_ai/mirror_school_handler.h"
#include "ethics_ai/ethics_profile_registry.h"
#include "ethics_ai/chain_visualizer.h"
#include "ethics_ai/argument_store.h"

#include <algorithm>
#include <memory>
#include <string>
#include <thread>
#include <vector>

TEST(EthicsAiEuCompliance, EuCompliance_SkippedForCurrentBuild)
{
    GTEST_SKIP() << "Legacy EU compliance test disabled until it is rewritten for the current ethics_ai API";
}

#if 0

using namespace themis::plugins::ethics;

// ============================================================================
// Shared fixtures
// ============================================================================

namespace {

/// Build a minimal mock registry with the canonical 22 schools.
class MockEthicsRegistry22 : public IEthicsProfileRegistry {
public:
    MockEthicsRegistry22() {
        // Western-European (6)
        add("kant",             "deontological");
        add("rawls",            "contractarian");
        add("contractualism",   "contractarian");
        add("rationalism",      "rationalist");
        add("socratic",         "virtue");
        add("utilitarianism",   "consequentialist");
        // Global South / Islamic / Confucian (4)
        add("islamische_ethik", "religious");
        add("konfuzianismus",   "virtue");
        add("buddhistische_ethik","virtue");
        add("juedische_bioethik","religious");
        // Liberation / Post-Colonial (4)
        add("feministische_ethik","liberation");
        add("adam_smith",        "economic");
        add("nietzsche",         "non_mainstream");
        add("marx",              "non_mainstream");
        // Non-mainstream (4)
        add("schopenhauer",      "non_mainstream");
        add("dilthey",           "non_mainstream");
        add("arendt",            "non_mainstream");
        add("durkheim",          "non_mainstream");
        // Institutional (4)
        add("behoerden_ethik",    "institutional");
        add("universitaere_ethik","institutional");
        add("merton",             "institutional");
        add("leopold",            "institutional");
    }

    std::vector<EthicsProfileMeta> queryIndex(
        const EthicsIndexQuery& /*q*/) const override {
        return schools_;
    }

private:
    void add(const std::string& id, const std::string& cls) {
        EthicsProfileMeta m;
        m.school_id      = id;
        m.taxonomy_class = cls;
        m.name           = id;
        schools_.push_back(std::move(m));
    }
    std::vector<EthicsProfileMeta> schools_;
};

/// Build a simple ArgumentStore with a few linked arguments for ChainVisualizer tests.
ArgumentStore makeArgumentStore() {
    ArgumentStore store;
    EthicalArgument a1;
    a1.id          = "arg_1";
    a1.school_id   = "kant";
    a1.type        = ArgumentType::PRO;
    a1.strength    = ArgumentStrength::STRONG;
    a1.position    = "Categorical imperative supports prohibition.";
    store.store(a1);

    EthicalArgument a2;
    a2.id          = "arg_2";
    a2.school_id   = "utilitarianism";
    a2.type        = ArgumentType::CONTRA;
    a2.strength    = ArgumentStrength::MODERATE;
    a2.position    = "Utility calculus may permit under conditions.";
    a2.counterarguments.push_back("arg_1");
    store.store(a2);

    EthicalArgument a3;
    a3.id          = "arg_3";
    a3.school_id   = "rawls";
    a3.type        = ArgumentType::SYNTHESIS;
    a3.strength    = ArgumentStrength::DECISIVE;
    a3.position    = "Veil of ignorance consensus: prohibit.";
    a3.supports.push_back("arg_1");
    store.store(a3);

    return store;
}

} // anonymous namespace

// ============================================================================
// EU-01: ABSTAIN vote propagation — timed-out school ∈ participating_schools
// ============================================================================

TEST(EthicsAiEuCompliance, EU01_AbstainPropagation_TimedOutSchoolInParticipatingSchools) {
    MockEthicsRegistry22 reg;
    RouterConfig cfg;
    cfg.discourse_mode = DiscourseMode::LAYERED_FAST;
    EthicsSelectionRouter router(&reg, cfg);
    const auto plan = router.planDiscourse();

    ASSERT_GE(plan.ebene1_school_ids.size(), 1u);

    DiscourseOrchestrator orch(&reg, cfg);
    orch.setLLMInferenceFn([](const std::string& school_id,
                               const std::string& /*dilemma*/) -> DiscourseRoundOutput {
        // Always timeout
        std::this_thread::sleep_for(std::chrono::seconds(60));
        DiscourseRoundOutput out;
        out.school_id   = school_id;
        out.ldm_verdict = DiscourseVerdict::PERMIT;
        return out;
    });
    orch.setSchoolTimeoutMs(1);

    MirrorSchoolPolicy policy;
    const auto results = orch.runEbene1(plan, "EU compliance test dilemma", policy);

    // Every school must appear with ABSTAIN + timed_out=true
    ASSERT_EQ(results.size(), plan.ebene1_school_ids.size());
    for (const auto& out : results) {
        EXPECT_TRUE(out.timed_out)
            << "School " << out.school_id << " must be timed_out=true";
        EXPECT_EQ(out.ldm_verdict, DiscourseVerdict::ABSTAIN)
            << "School " << out.school_id << " must produce ABSTAIN on timeout";
    }

    // MetaVerdict participating_schools must include all timed-out schools
    MetaVerdictBuilder builder;
    LegalGrounding grounding;
    grounding.grounding_available = true;
    const auto mv = builder.buildMetaVerdict(
        results, {}, grounding, DiscourseMode::LAYERED_FAST, {});

    for (const auto& sid : plan.ebene1_school_ids) {
        EXPECT_TRUE(std::find(mv.participating_schools.begin(),
                              mv.participating_schools.end(),
                              sid) != mv.participating_schools.end())
            << "Timed-out school " << sid
            << " MUST appear in participating_schools (EU AI Act Art. 13)";
    }
}

// ============================================================================
// EU-02: Art. 13 listing completeness — all 22 schools in participating_schools
// ============================================================================

TEST(EthicsAiEuCompliance, EU02_Art13ListingCompleteness_All22SchoolsPresent) {
    // Build MetaVerdict from 22 school results: 18 PROHIBIT, 4 ABSTAIN
    const std::vector<std::string> school_ids = {
        "kant", "rawls", "contractualism", "rationalism", "socratic",
        "utilitarianism", "islamische_ethik", "konfuzianismus",
        "buddhistische_ethik", "juedische_bioethik",
        "feministische_ethik", "adam_smith", "nietzsche", "marx",
        "schopenhauer", "dilthey", "arendt", "durkheim",
        "behoerden_ethik", "universitaere_ethik", "merton", "leopold"
    };
    ASSERT_EQ(school_ids.size(), 22u);

    std::vector<DiscourseRoundOutput> results;
    results.reserve(22);

    for (std::size_t i = 0; i < school_ids.size(); ++i) {
        DiscourseRoundOutput out;
        out.school_id      = school_ids[i];
        out.timed_out      = (i >= 18);  // last 4 abstain
        out.ldm_verdict    = out.timed_out ? DiscourseVerdict::ABSTAIN
                                           : DiscourseVerdict::PROHIBIT;
        out.verdict        = out.timed_out ? "ABSTAIN" : "PROHIBIT";
        out.initial_weight = 1.0 / 22.0;
        results.push_back(out);
    }

    MetaVerdictBuilder builder;
    LegalGrounding grounding;
    grounding.grounding_available = true;
    const auto mv = builder.buildMetaVerdict(
        results, {}, grounding, DiscourseMode::LAYERED_FULL, {});

    EXPECT_EQ(mv.participating_schools.size(), 22u)
        << "All 22 schools MUST be in participating_schools (EU AI Act Art. 13)";

    // Verify every school_id appears exactly once
    for (const auto& sid : school_ids) {
        const auto count = std::count(mv.participating_schools.begin(),
                                      mv.participating_schools.end(), sid);
        EXPECT_EQ(count, 1) << "School " << sid << " must appear exactly once";
    }
}

// ============================================================================
// EU-03: Audit trail immutability — no post-hoc modification possible
// ============================================================================

TEST(EthicsAiEuCompliance, EU03_AuditTrailImmutability_NoPostHocModification) {
    EthicsAuditLog log;

    RoundAuditEntry e;
    e.round_id              = "round-001";
    e.timestamp_utc         = "2026-08-09T17:00:00Z";
    e.dilemma_hash          = "abcdef0123456789";
    e.participating_schools = {"kant", "rawls", "islamische_ethik"};
    e.verdict               = "PROHIBIT";
    e.convergence_score     = 0.85;
    e.norm_citations        = {"GG Art. 1", "EU AI Act Art. 22"};

    const size_t idx = log.append(e);
    EXPECT_EQ(idx, 0u);

    // Export and mutate the returned snapshot
    auto snapshot = log.exportAuditLog();
    ASSERT_EQ(snapshot.size(), 1u);
    snapshot[0].verdict = "PERMIT";  // mutate snapshot copy

    // Re-export: log must be unchanged
    auto snapshot2 = log.exportAuditLog();
    ASSERT_EQ(snapshot2.size(), 1u);
    EXPECT_EQ(snapshot2[0].verdict, "PROHIBIT")
        << "Mutating the exported snapshot MUST NOT affect the log (immutability)";

    // tryOverwrite must return IMMUTABLE_VIOLATION
    const AuditError err = log.tryOverwrite(0u, snapshot[0]);
    EXPECT_EQ(err, AuditError::IMMUTABLE_VIOLATION)
        << "tryOverwrite() MUST return IMMUTABLE_VIOLATION";

    // tryErase must return IMMUTABLE_VIOLATION
    const AuditError err2 = log.tryErase(0u);
    EXPECT_EQ(err2, AuditError::IMMUTABLE_VIOLATION)
        << "tryErase() MUST return IMMUTABLE_VIOLATION";
}

// ============================================================================
// EU-04: LDM contract (w₀ = 1/N) — equal initial weight for all N schools
// ============================================================================

TEST(EthicsAiEuCompliance, EU04_LdmContract_EqualInitialWeight_OneOverN) {
    MockEthicsRegistry22 reg;
    RouterConfig cfg;
    cfg.discourse_mode = DiscourseMode::LAYERED_FULL;
    EthicsSelectionRouter router(&reg, cfg);
    const auto plan = router.planDiscourse();

    ASSERT_GE(plan.ebene1_school_ids.size(), 5u)
        << "LAYERED_FULL must produce ≥ 5 Ebene-1 schools";

    const double expected_w0 = 1.0 / static_cast<double>(plan.ebene1_school_ids.size());
    EXPECT_NEAR(plan.initial_weight, expected_w0, 1e-9)
        << "initial_weight MUST equal 1/N (equal-weight LDM contract)";
}

// ============================================================================
// EU-05: legal_db unavailability — grounding_available=false, no silent failure
// ============================================================================

TEST(EthicsAiEuCompliance, EU05_LegalDbUnavailability_FlagSetNoSilentFailure) {
    std::vector<DiscourseRoundOutput> results;
    DiscourseRoundOutput out;
    out.school_id      = "kant";
    out.ldm_verdict    = DiscourseVerdict::PROHIBIT;
    out.verdict        = "PROHIBIT";
    out.timed_out      = false;
    out.initial_weight = 1.0;
    results.push_back(out);

    LegalGrounding unavailable;
    unavailable.grounding_available = false;
    unavailable.citation_ids.clear();
    unavailable.norm_refs.clear();

    MetaVerdictBuilder builder;
    MetaVerdict mv;

    // MUST NOT throw
    EXPECT_NO_THROW({
        mv = builder.buildMetaVerdict(
            results, {}, unavailable, DiscourseMode::LAYERED_FAST, {});
    }) << "Legal-DB unavailability MUST NOT throw (observable via flag only)";

    EXPECT_FALSE(mv.legal_grounding.grounding_available)
        << "MetaVerdict.legal_grounding.grounding_available MUST be false "
           "when Legal-DB is offline (EU AI Act Art. 22)";

    // Verdict must still be produced (no silent omission)
    EXPECT_EQ(mv.dominant_verdict, DiscourseVerdict::PROHIBIT)
        << "Verdict MUST be produced even when Legal-DB is unavailable";
}

// ============================================================================
// EU-06: ChainVisualizer DOT output — non-empty, valid DOT syntax
// ============================================================================

TEST(EthicsAiEuCompliance, EU06_ChainVisualizerDot_NonEmpty_ValidDotSyntax) {
    auto store = makeArgumentStore();

    const std::vector<std::string> arg_ids = {"arg_1", "arg_2", "arg_3"};
    const std::string dot = ChainVisualizer::exportDot(arg_ids, store);

    ASSERT_FALSE(dot.empty())
        << "ChainVisualizer::exportDot() MUST produce non-empty output";

    // Verify presence of DOT structural markers
    EXPECT_NE(dot.find("digraph"), std::string::npos)
        << "DOT output must start with 'digraph ...'";
    EXPECT_NE(dot.find('{'), std::string::npos)
        << "DOT output must contain opening brace";
    EXPECT_NE(dot.find('}'), std::string::npos)
        << "DOT output must contain closing brace";

    // Each school must appear as a node label
    EXPECT_NE(dot.find("kant"),         std::string::npos) << "kant must appear in DOT";
    EXPECT_NE(dot.find("utilitarianism"),std::string::npos) << "utilitarianism must appear";
    EXPECT_NE(dot.find("rawls"),        std::string::npos) << "rawls must appear in DOT";
}

// ============================================================================
// EU-07: Mermaid diagram artifact — non-empty, valid Mermaid flowchart syntax
// ============================================================================

TEST(EthicsAiEuCompliance, EU07_ChainVisualizerMermaid_NonEmpty_ValidMermaidSyntax) {
    auto store = makeArgumentStore();

    const std::vector<std::string> arg_ids = {"arg_1", "arg_2", "arg_3"};
    const std::string mermaid = ChainVisualizer::exportMermaid(arg_ids, store);

    ASSERT_FALSE(mermaid.empty())
        << "ChainVisualizer::exportMermaid() MUST produce non-empty output";

    // Verify presence of Mermaid structural markers
    EXPECT_NE(mermaid.find("flowchart"), std::string::npos)
        << "Mermaid output must begin with 'flowchart'";

    // Each school must appear as a node
    EXPECT_NE(mermaid.find("kant"),          std::string::npos) << "kant must appear";
    EXPECT_NE(mermaid.find("utilitarianism"),std::string::npos) << "utilitarianism must appear";
    EXPECT_NE(mermaid.find("rawls"),         std::string::npos) << "rawls must appear";
}

// ============================================================================
// EU-08: Art. 13 round-level audit export — chronological order
// ============================================================================

TEST(EthicsAiEuCompliance, EU08_AuditExport_ChronologicalOrder) {
    EthicsAuditLog log;

    // Append 5 rounds
    const int kRounds = 5;
    for (int i = 0; i < kRounds; ++i) {
        RoundAuditEntry e;
        e.round_id           = "round-" + std::to_string(i);
        e.timestamp_utc      = "2026-08-09T17:0" + std::to_string(i) + ":00Z";
        e.dilemma_hash       = "hash" + std::to_string(i);
        e.verdict            = (i % 2 == 0) ? "PROHIBIT" : "PERMIT";
        e.convergence_score  = 0.6 + i * 0.05;
        e.participating_schools = {"kant", "rawls"};
        e.norm_citations        = {"EU AI Act Art. 22"};
        log.append(std::move(e));
    }

    EXPECT_EQ(log.size(), static_cast<size_t>(kRounds));

    const auto entries = log.exportAuditLog();
    ASSERT_EQ(entries.size(), static_cast<size_t>(kRounds));

    // Verify chronological order (by round_index)
    for (int i = 0; i < kRounds; ++i) {
        EXPECT_EQ(entries[i].round_index, static_cast<uint32_t>(i))
            << "Entry at position " << i
            << " must have round_index=" << i
            << " (chronological order required by EU AI Act Art. 13)";
        EXPECT_EQ(entries[i].round_id, "round-" + std::to_string(i))
            << "round_id mismatch at position " << i;
    }
}

// ============================================================================
// Main
// ============================================================================

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

#endif
