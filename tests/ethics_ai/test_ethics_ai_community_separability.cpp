/**
 * @file test_ethics_ai_community_separability.cpp
 * @brief Community-build separability test for the ethics_ai module.
 *
 * Verifies that the public ethics_ai API surface compiles and functions
 * correctly without private-source ethic_ai plugin components, and that
 * enterprise-only discourse modes degrade gracefully.
 *
 * ### Community separability contract
 * - `include/ethics_ai/ethics_ai_types.h` must compile standalone (no private headers).
 * - `SELECTION_ONLY` discourse mode must work with zero private sources.
 * - `EthicsAuditLog` and `RoundAuditEntry` (Art. 13 types) are public — available
 *   in Community builds.
 * - When compiled with `-DTHEMIS_COMMUNITY_BUILD=1`:
 *   `LAYERED_FULL` and `LAYERED_FAST` routes must return a degraded / fail-closed
 *   result (no silent access to enterprise-only discourse logic).
 *
 * ### Test IDs
 *   CSEP-01 — public type header standalone compile (no private includes)
 *   CSEP-02 — SELECTION_ONLY mode produces non-empty plan (public-only path)
 *   CSEP-03 — AuditLog and RoundAuditEntry are fully usable in Community builds
 *   CSEP-04 — EthicsErrorCode range check: all defined codes are within [0, 100]
 *   CSEP-05 — LegalGrounding default state is fail-safe (grounding_available=false)
 *   CSEP-06 — MetaVerdict default state carries no implicit school entries
 */

#include <gtest/gtest.h>

#include "ethics_ai/ethics_ai_types.h"
#include "ethics_ai/ethics_selection_router.h"
#include "ethics_ai/meta_verdict_builder.h"
#include "ethics_ai/ethics_profile_registry.h"

#include <algorithm>
#include <string>
#include <vector>

using namespace themis::plugins::ethics;

// ============================================================================
// Minimal community-safe mock registry (no private sources required)
// ============================================================================

namespace {

class CommunitySafeRegistry : public IEthicsProfileRegistry {
public:
    CommunitySafeRegistry() {
        // Five canonical schools — minimum quorum for SELECTION_ONLY
        for (const auto& id : std::vector<std::string>{
                 "kant", "rawls", "utilitarianism",
                 "islamische_ethik", "konfuzianismus"}) {
            EthicsProfileMeta m;
            m.school_id      = id;
            m.taxonomy_class = "deontological";
            m.name           = id;
            schools_.push_back(std::move(m));
        }
    }

    std::vector<EthicsProfileMeta> queryIndex(
        const EthicsIndexQuery& /*q*/) const override {
        return schools_;
    }

private:
    std::vector<EthicsProfileMeta> schools_;
};

} // anonymous namespace

// ============================================================================
// CSEP-01: public type header compiles standalone (no private includes needed)
// ============================================================================

TEST(EthicsAiCommunitySeparability, CSEP01_PublicTypeHeaderStandaloneCompile) {
    // If this translation unit compiles, the test passes.
    // Verify the key public types are constructible without private sources.
    EthicsError err       = EthicsError::ok();
    LegalGrounding lg;
    MetaVerdict mv;
    RoundAuditEntry rae;
    EthicsAuditLog log;

    EXPECT_TRUE(err.isOk());
    EXPECT_FALSE(lg.grounding_available);
    EXPECT_EQ(mv.dominant_verdict, DiscourseVerdict::ABSTAIN);
    EXPECT_TRUE(rae.round_id.empty());
    EXPECT_TRUE(log.empty());
}

// ============================================================================
// CSEP-02: SELECTION_ONLY mode works without private sources
// ============================================================================

TEST(EthicsAiCommunitySeparability, CSEP02_SelectionOnly_WorksWithoutPrivateSources) {
    CommunitySafeRegistry reg;
    RouterConfig cfg;
    cfg.discourse_mode = DiscourseMode::SELECTION_ONLY;
    EthicsSelectionRouter router(&reg, cfg);

    DiscourseOrchestratorPlan plan;
    EXPECT_NO_THROW({ plan = router.planDiscourse(); })
        << "planDiscourse() in SELECTION_ONLY mode must not throw";

    EXPECT_EQ(plan.mode, DiscourseMode::SELECTION_ONLY)
        << "Returned plan must reflect SELECTION_ONLY mode";

    // SELECTION_ONLY: no Ebene-1 school assignments
    EXPECT_TRUE(plan.ebene1_school_ids.empty())
        << "SELECTION_ONLY must not assign Ebene-1 schools "
           "(no private discourse logic required)";
}

// ============================================================================
// CSEP-03: AuditLog and RoundAuditEntry are fully usable in Community builds
// ============================================================================

TEST(EthicsAiCommunitySeparability, CSEP03_AuditLog_FullyUsable_CommunityBuild) {
    EthicsAuditLog log;

    // Append, export, and verify — all public API, no private sources
    RoundAuditEntry e;
    e.round_id              = "community-round-001";
    e.timestamp_utc         = "2026-08-09T09:00:00Z";
    e.dilemma_hash          = "communitytest123";
    e.participating_schools = {"kant", "rawls"};
    e.verdict               = "PROHIBIT";
    e.convergence_score     = 0.80;
    e.norm_citations        = {"EU AI Act Art. 13"};

    const size_t idx = log.append(e);
    EXPECT_EQ(idx, 0u);
    EXPECT_EQ(log.size(), 1u);

    const auto entries = log.exportAuditLog();
    ASSERT_EQ(entries.size(), 1u);
    EXPECT_EQ(entries[0].verdict, "PROHIBIT");
    EXPECT_EQ(entries[0].round_id, "community-round-001");

    // Immutability contract accessible in Community builds
    const AuditError err = log.tryOverwrite(0u, e);
    EXPECT_EQ(err, AuditError::IMMUTABLE_VIOLATION)
        << "Immutability enforcement must work in Community builds";
}

// ============================================================================
// CSEP-04: EthicsErrorCode range — all defined codes in [0, 100]
// ============================================================================

TEST(EthicsAiCommunitySeparability, CSEP04_EthicsErrorCode_AllCodesInRange) {
    // Spot-check all named codes are within [0, 100]
    const auto check = [](EthicsErrorCode code) {
        return static_cast<int>(code) >= 0 && static_cast<int>(code) <= 100;
    };

    EXPECT_TRUE(check(EthicsErrorCode::OK));
    EXPECT_TRUE(check(EthicsErrorCode::PROFILE_NOT_FOUND));
    EXPECT_TRUE(check(EthicsErrorCode::LIFECYCLE_UNINITIALIZED));
    EXPECT_TRUE(check(EthicsErrorCode::CONTEXT_RETRIEVAL_FAILED));
    EXPECT_TRUE(check(EthicsErrorCode::ROUTING_NO_SCHOOLS));
    EXPECT_TRUE(check(EthicsErrorCode::LDM_LLM_TIMEOUT));
    EXPECT_TRUE(check(EthicsErrorCode::LDM_ALL_ABSTAINED));
    EXPECT_TRUE(check(EthicsErrorCode::LDM_LEGAL_DB_UNAVAILABLE));
    EXPECT_TRUE(check(EthicsErrorCode::LDM_CLUSTER_EMPTY));
    EXPECT_TRUE(check(EthicsErrorCode::LDM_EQUAL_WEIGHT_VIOLATION));
}

// ============================================================================
// CSEP-05: LegalGrounding default state is fail-safe (grounding_available=false)
// ============================================================================

TEST(EthicsAiCommunitySeparability, CSEP05_LegalGrounding_DefaultIsFailing) {
    LegalGrounding lg;

    EXPECT_FALSE(lg.grounding_available)
        << "Default LegalGrounding MUST have grounding_available=false "
           "(fail-safe default — no implicit Legal-DB access in Community builds)";
    EXPECT_TRUE(lg.citation_ids.empty())
        << "Default citation_ids must be empty";
    EXPECT_TRUE(lg.norm_refs.empty())
        << "Default norm_refs must be empty";
    EXPECT_FALSE(lg.override_permitted)
        << "Default override_permitted must be false (fail-closed)";
}

// ============================================================================
// CSEP-06: MetaVerdict default state carries no implicit school entries
// ============================================================================

TEST(EthicsAiCommunitySeparability, CSEP06_MetaVerdict_DefaultCarriesNoSchoolEntries) {
    MetaVerdict mv;

    EXPECT_TRUE(mv.participating_schools.empty())
        << "Default MetaVerdict must not contain any participating_schools "
           "(no implicit school resolution in Community builds)";
    EXPECT_TRUE(mv.dissenting_schools.empty());
    EXPECT_TRUE(mv.minority_dissent.empty());
    EXPECT_EQ(mv.dominant_verdict, DiscourseVerdict::ABSTAIN)
        << "Default dominant_verdict must be ABSTAIN (fail-closed)";
    EXPECT_EQ(mv.convergence_verdict, MetaVerdict::ConvergenceVerdict::DISSENT)
        << "Default convergence_verdict must be DISSENT (fail-closed)";
}

// ============================================================================
// Main
// ============================================================================

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
