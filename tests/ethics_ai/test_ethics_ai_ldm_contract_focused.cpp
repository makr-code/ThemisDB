/**
 * @file test_ethics_ai_ldm_contract_focused.cpp
 * @brief LDM Phase 1-5 contract tests for ethics_ai module.
 * @note Test IDs: EAL-01..EAL-08
 * @note Coverage: DiscourseMode, equal-weight contract, ABSTAIN failsafe, MetaVerdict
 *                 convergence, cross-cultural flag, mirror-school audit, Legal-DB flag.
 *
 * All tests use mock/deterministic fixtures only — no file I/O, no real LLM.
 * The stub LLM path in DiscourseOrchestrator is exercised directly.
 */

#include <gtest/gtest.h>

#include "ethics_ai/ethics_ai_types.h"
#include "ethics_ai/ethics_selection_router.h"
#include "ethics_ai/discourse_orchestrator.h"
#include "ethics_ai/meta_verdict_builder.h"
#include "ethics_ai/mirror_school_handler.h"

#include <algorithm>
#include <cmath>
#include <memory>
#include <string>
#include <vector>

using namespace themis::plugins::ethics;

// ============================================================================
// Mock IEthicsProfileRegistry
// ============================================================================

namespace {

/// Minimal mock registry that returns a fixed set of school metadata.
class MockEthicsProfileRegistry : public IEthicsProfileRegistry {
public:
    void addSchool(const std::string& school_id,
                   const std::string& taxonomy_class = "deontological") {
        EthicsProfileMeta m;
        m.school_id      = school_id;
        m.taxonomy_class = taxonomy_class;
        m.name           = school_id;
        schools_.push_back(std::move(m));
    }

    std::vector<EthicsProfileMeta> queryIndex(
        const EthicsIndexQuery& /*query*/) const override {
        return schools_;
    }

    std::variant<PhilosophyProfile, Status> getProfile(
        const std::string& school_id) override {
        PhilosophyProfile p;
        p.school_id = school_id;
        return p;
    }

    std::variant<size_t, Status> rebuildIndex(
        const std::string& /*directory*/) override {
        return schools_.size();
    }

    size_t indexSize() const override { return schools_.size(); }

    bool hasProfile(const std::string& school_id) const override {
        for (const auto& m : schools_) {
            if (m.school_id == school_id) return true;
        }
        return false;
    }

private:
    std::vector<EthicsProfileMeta> schools_;
};

/// Build a 22-school mock registry with the canonical LDM school set.
std::unique_ptr<MockEthicsProfileRegistry> make22SchoolRegistry() {
    auto reg = std::make_unique<MockEthicsProfileRegistry>();

    // Cluster A — Deontological (4 schools)
    reg->addSchool("kant",           "deontological");
    reg->addSchool("contractualism", "deontological");
    reg->addSchool("rawls",          "deontological");
    reg->addSchool("rationalism",    "deontological");

    // Cluster B — Consequentialist (2 schools)
    reg->addSchool("utilitarianism", "consequentialist");
    reg->addSchool("adam_smith",     "consequentialist");

    // Cluster C — Virtue (2 schools)
    reg->addSchool("socratic",       "virtue");
    reg->addSchool("konfuzianismus", "virtue");

    // Cluster D — Cultural-Religious (3 schools)
    reg->addSchool("islamische_ethik",   "cultural_religious");
    reg->addSchool("juedische_bioethik", "cultural_religious");
    reg->addSchool("buddhistische_ethik","cultural_religious");

    // Cluster E — Non-Mainstream (6 schools)
    reg->addSchool("nietzsche",  "non_mainstream");
    reg->addSchool("marx",       "non_mainstream");
    reg->addSchool("schopenhauer","non_mainstream");
    reg->addSchool("dilthey",    "non_mainstream");
    reg->addSchool("arendt",     "non_mainstream");
    reg->addSchool("durkheim",   "non_mainstream");

    // Cluster F — Institutional (5 schools)
    reg->addSchool("behoerden_ethik",    "institutional");
    reg->addSchool("universitaere_ethik","institutional");
    reg->addSchool("wiener",             "institutional");
    reg->addSchool("merton",             "institutional");
    reg->addSchool("leopold",            "institutional");

    return reg;
}

/// Build a plan from a mock registry in a given mode.
DiscourseOrchestratorPlan makePlan(IEthicsProfileRegistry* reg,
                                   DiscourseMode mode) {
    RouterConfig cfg;
    cfg.discourse_mode = mode;
    EthicsSelectionRouter router(reg, cfg);
    return router.planDiscourse();
}

} // anonymous namespace

// ============================================================================
// EAL-01: SELECTION_ONLY → plan.ebene1_school_ids.empty()
// ============================================================================

TEST(EthicsAiLdmContract, EAL01_DiscourseMode_SelectionOnly_EmptyPlan) {
    auto reg = make22SchoolRegistry();
    RouterConfig cfg;
    cfg.discourse_mode = DiscourseMode::SELECTION_ONLY;
    EthicsSelectionRouter router(reg.get(), cfg);

    const auto plan = router.planDiscourse();

    EXPECT_EQ(plan.mode, DiscourseMode::SELECTION_ONLY);
    EXPECT_TRUE(plan.ebene1_school_ids.empty())
        << "SELECTION_ONLY must not populate ebene1_school_ids";
    EXPECT_TRUE(plan.cluster_map.empty())
        << "SELECTION_ONLY must not populate cluster_map";
}

// ============================================================================
// EAL-02: LAYERED_FAST → initial_weight == 1/N for all N schools
// ============================================================================

TEST(EthicsAiLdmContract, EAL02_EqualWeightContract_InitialWeight) {
    auto reg = make22SchoolRegistry();
    const auto plan = makePlan(reg.get(), DiscourseMode::LAYERED_FAST);

    ASSERT_FALSE(plan.ebene1_school_ids.empty())
        << "LAYERED_FAST must populate ebene1_school_ids";

    const std::size_t N       = plan.ebene1_school_ids.size();
    const double      expected = 1.0 / static_cast<double>(N);

    EXPECT_NEAR(plan.initial_weight, expected, 1e-9)
        << "initial_weight must equal 1/N (equal-weight contract)";
}

// ============================================================================
// EAL-03: inject timeout-stub LLM → school with timed_out=true has
//         ldm_verdict=ABSTAIN; school still in participating_schools
// ============================================================================

TEST(EthicsAiLdmContract, EAL03_Ebene1_AbstainFailClosed_OnTimeout) {
    auto reg = make22SchoolRegistry();
    auto plan = makePlan(reg.get(), DiscourseMode::LAYERED_FAST);
    ASSERT_GE(plan.ebene1_school_ids.size(), 1u);

    RouterConfig cfg;
    cfg.discourse_mode = DiscourseMode::LAYERED_FAST;

    DiscourseOrchestrator orch(reg.get(), cfg);

    // Inject a timeout-stub LLM: always sleeps longer than the 1 ms timeout.
    orch.setLLMInferenceFn([](const std::string& school_id,
                               const std::string& /*dilemma*/)
                               -> DiscourseRoundOutput {
        // Simulate a blocking call (well beyond any reasonable timeout).
        std::this_thread::sleep_for(std::chrono::seconds(60));
        DiscourseRoundOutput out;
        out.school_id   = school_id;
        out.ldm_verdict = DiscourseVerdict::PERMIT;
        return out;
    });
    // Set a very short timeout so the stub above always expires.
    orch.setSchoolTimeoutMs(1);

    MirrorSchoolPolicy policy;  // Mirror inactive (default OFF).

    const auto results = orch.runEbene1(plan, "test dilemma", policy);

    // All schools must be present (EU AI Act Art. 13).
    EXPECT_EQ(results.size(), plan.ebene1_school_ids.size());

    // All schools must have timed_out=true and ABSTAIN.
    for (const auto& out : results) {
        EXPECT_TRUE(out.timed_out)
            << "School " << out.school_id << " must have timed_out=true";
        EXPECT_EQ(out.ldm_verdict, DiscourseVerdict::ABSTAIN)
            << "School " << out.school_id << " must have ldm_verdict=ABSTAIN on timeout";
    }

    // Verify school_ids in results match plan order.
    for (std::size_t i = 0; i < results.size(); ++i) {
        EXPECT_EQ(results[i].school_id, plan.ebene1_school_ids[i]);
    }

    // Build MetaVerdict and verify schools appear in participating_schools.
    MetaVerdictBuilder builder;
    LegalGrounding grounding;
    grounding.grounding_available = true;
    const auto mv = builder.buildMetaVerdict(
        results, {}, grounding, DiscourseMode::LAYERED_FAST, {});

    for (const auto& sid : plan.ebene1_school_ids) {
        EXPECT_TRUE(std::find(mv.participating_schools.begin(),
                              mv.participating_schools.end(),
                              sid) != mv.participating_schools.end())
            << "School " << sid << " must be in participating_schools";
    }
}

// ============================================================================
// EAL-04: all schools ABSTAIN → MetaVerdict with DISSENT
// ============================================================================

TEST(EthicsAiLdmContract, EAL04_MetaVerdict_AllAbstain_ProducesDissent) {
    // Build Ebene-1 results where all schools ABSTAIN.
    const std::vector<std::string> school_ids = {
        "kant", "rawls", "utilitarianism", "islamische_ethik"
    };

    std::vector<DiscourseRoundOutput> results;
    for (const auto& sid : school_ids) {
        DiscourseRoundOutput out;
        out.school_id   = sid;
        out.ldm_verdict = DiscourseVerdict::ABSTAIN;
        out.verdict     = "ABSTAIN";
        out.timed_out   = true;
        out.initial_weight = 0.25;
        results.push_back(out);
    }

    MetaVerdictBuilder builder;
    LegalGrounding grounding;
    grounding.grounding_available = true;
    const auto mv = builder.buildMetaVerdict(
        results, {}, grounding, DiscourseMode::LAYERED_FAST, {});

    EXPECT_EQ(mv.convergence_verdict, MetaVerdict::ConvergenceVerdict::DISSENT)
        << "All-ABSTAIN must produce DISSENT";
    EXPECT_EQ(mv.dominant_verdict, DiscourseVerdict::ABSTAIN)
        << "dominant_verdict must be ABSTAIN when all abstained";
    EXPECT_EQ(mv.participating_schools.size(), school_ids.size())
        << "All schools must be in participating_schools";
}

// ============================================================================
// EAL-05: 10 PROHIBIT / 2 ABSTAIN → convergence_score > 0.75 → CLEAR_CONSENSUS
// ============================================================================

TEST(EthicsAiLdmContract, EAL05_MetaVerdict_ClearConsensus_ConvergenceAbove075) {
    std::vector<DiscourseRoundOutput> results;

    // 10 active PROHIBIT schools.
    const std::vector<std::string> prohibit_ids = {
        "kant", "rawls", "contractualism", "rationalism",
        "socratic", "nietzsche", "marx", "arendt",
        "behoerden_ethik", "universitaere_ethik"
    };
    for (const auto& sid : prohibit_ids) {
        DiscourseRoundOutput out;
        out.school_id      = sid;
        out.ldm_verdict    = DiscourseVerdict::PROHIBIT;
        out.verdict        = "PROHIBIT";
        out.timed_out      = false;
        out.initial_weight = 1.0 / 12.0;
        results.push_back(out);
    }

    // 2 ABSTAIN schools.
    for (const auto& sid : std::vector<std::string>{"utilitarianism", "adam_smith"}) {
        DiscourseRoundOutput out;
        out.school_id      = sid;
        out.ldm_verdict    = DiscourseVerdict::ABSTAIN;
        out.verdict        = "ABSTAIN";
        out.timed_out      = true;
        out.initial_weight = 1.0 / 12.0;
        results.push_back(out);
    }

    MetaVerdictBuilder builder;
    LegalGrounding grounding;
    grounding.grounding_available = true;
    const auto mv = builder.buildMetaVerdict(
        results, {}, grounding, DiscourseMode::LAYERED_FULL, {});

    // N_active = 10; dominant_count = 10; convergence_score = 10/10 = 1.0 > 0.75
    EXPECT_GT(mv.convergence_score, 0.75)
        << "convergence_score must be > 0.75 (10 PROHIBIT / 2 ABSTAIN)";
    EXPECT_EQ(mv.convergence_verdict, MetaVerdict::ConvergenceVerdict::CLEAR_CONSENSUS)
        << "must produce CLEAR_CONSENSUS";
    EXPECT_EQ(mv.dominant_verdict, DiscourseVerdict::PROHIBIT);
    EXPECT_EQ(mv.participating_schools.size(), 12u);
}

// ============================================================================
// EAL-06: kant + islamische_ethik + konfuzianismus all PROHIBIT →
//         cross_cultural_flag == true
// ============================================================================

TEST(EthicsAiLdmContract, EAL06_CrossCulturalFlag_MultiRegionConsensus) {
    std::vector<DiscourseRoundOutput> results;

    // Three schools from distinct cultural regions all PROHIBIT.
    for (const auto& sid : std::vector<std::string>{
             "kant",              // Western-European
             "islamische_ethik",  // Islamic
             "konfuzianismus"     // East-Asian
         })
    {
        DiscourseRoundOutput out;
        out.school_id      = sid;
        out.ldm_verdict    = DiscourseVerdict::PROHIBIT;
        out.verdict        = "PROHIBIT";
        out.timed_out      = false;
        out.initial_weight = 1.0 / 3.0;
        results.push_back(out);
    }

    MetaVerdictBuilder builder;
    LegalGrounding grounding;
    grounding.grounding_available = true;
    const auto mv = builder.buildMetaVerdict(
        results, {}, grounding, DiscourseMode::LAYERED_FULL, {});

    EXPECT_TRUE(mv.cross_cultural_flag)
        << "cross_cultural_flag must be true when ≥ 2 distinct cultural regions "
           "share the dominant verdict";
    EXPECT_EQ(mv.dominant_verdict, DiscourseVerdict::PROHIBIT);
}

// ============================================================================
// EAL-07: mirror school output always in MetaVerdict.minority_dissent,
//         even for CLEAR_CONSENSUS
// ============================================================================

TEST(EthicsAiLdmContract, EAL07_MirrorSchool_PresentInMinorityDissent_EvenForClearConsensus) {
    // Set up CLEAR_CONSENSUS main schools.
    std::vector<DiscourseRoundOutput> main_results;
    const std::vector<std::string> school_ids = {
        "kant", "rawls", "contractualism", "rationalism", "socratic"
    };
    for (const auto& sid : school_ids) {
        DiscourseRoundOutput out;
        out.school_id   = sid;
        out.ldm_verdict = DiscourseVerdict::PROHIBIT;
        out.verdict     = "PROHIBIT";
        out.timed_out   = false;
        out.initial_weight = 0.2;
        main_results.push_back(out);
    }

    // Run mirror schools via MirrorSchoolHandler stub.
    MirrorSchoolHandler handler;
    // Stub (default) — no real LLM injection needed.
    const auto mirror_results = handler.runMirror(
        {"islamische_ethik", "konfuzianismus", "buddhistische_ethik", "juedische_bioethik"},
        "test dilemma",
        "bioethics");

    ASSERT_EQ(mirror_results.size(), 4u)
        << "All 4 mirror schools must be returned";

    // Build MetaVerdict with mirror_dissent injected.
    MetaVerdictBuilder builder;
    LegalGrounding grounding;
    grounding.grounding_available = true;
    const auto mv = builder.buildMetaVerdict(
        main_results, {}, grounding, DiscourseMode::LAYERED_FULL, mirror_results);

    // Verify CLEAR_CONSENSUS (5/5 = 1.0 > 0.75).
    EXPECT_EQ(mv.convergence_verdict, MetaVerdict::ConvergenceVerdict::CLEAR_CONSENSUS);

    // Mirror schools must be present in minority_dissent.
    EXPECT_EQ(mv.minority_dissent.size(), 4u)
        << "All 4 mirror schools must appear in minority_dissent";

    const std::vector<std::string> expected_mirrors = {
        "islamische_ethik", "konfuzianismus", "buddhistische_ethik", "juedische_bioethik"
    };
    for (const auto& sid : expected_mirrors) {
        const bool found = std::any_of(mv.minority_dissent.begin(),
                                       mv.minority_dissent.end(),
                                       [&sid](const DiscourseRoundOutput& o) {
                                           return o.school_id == sid;
                                       });
        EXPECT_TRUE(found)
            << "Mirror school " << sid << " must be present in minority_dissent";
    }
}

// ============================================================================
// EAL-08: Legal-DB unavailable → MetaVerdict.legal_grounding.grounding_available==false
//         (no exception thrown)
// ============================================================================

TEST(EthicsAiLdmContract, EAL08_LegalGrounding_Unavailable_FlagObservable) {
    std::vector<DiscourseRoundOutput> results;
    DiscourseRoundOutput out;
    out.school_id      = "kant";
    out.ldm_verdict    = DiscourseVerdict::PROHIBIT;
    out.verdict        = "PROHIBIT";
    out.timed_out      = false;
    out.initial_weight = 1.0;
    results.push_back(out);

    // Simulate Legal-DB unavailability.
    LegalGrounding unavailable_grounding;
    unavailable_grounding.grounding_available = false;  // DB offline.
    unavailable_grounding.citation_ids.clear();
    unavailable_grounding.norm_refs.clear();

    MetaVerdictBuilder builder;

    // Must NOT throw.
    MetaVerdict mv;
    EXPECT_NO_THROW({
        mv = builder.buildMetaVerdict(
            results, {}, unavailable_grounding, DiscourseMode::LAYERED_FAST, {});
    }) << "Legal-DB unavailability must not throw — observable via flag only";

    EXPECT_FALSE(mv.legal_grounding.grounding_available)
        << "legal_grounding.grounding_available must be false when Legal-DB offline";
}

// ============================================================================
// Main
// ============================================================================

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
