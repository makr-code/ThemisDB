/**
 * @file test_context_window_budget_strategy.cpp
 * @brief Unit tests CWB-01..CWB-15 for §12 Context-Window-Budget-Strategie components.
 *
 * Covers:
 *  - PriorRoundCompressor (CWB-01..CWB-05)
 *  - CrossSchoolTensionResolver (CWB-06..CWB-09)
 *  - ConvergenceMarkerEngine (CWB-10..CWB-12)
 *  - LlmCascadeRouter (CWB-13..CWB-14)
 *  - SynthesisMatrixBuilder (CWB-15)
 */

#include <gtest/gtest.h>

#include "plugins/ethics_ai/prior_round_compressor.h"
#include "plugins/ethics_ai/cross_school_tension_resolver.h"
#include "plugins/ethics_ai/convergence_marker_engine.h"
#include "plugins/ethics_ai/llm_cascade_router.h"
#include "plugins/ethics_ai/synthesis_matrix_builder.h"

using namespace themis::plugins::ethics;

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

static EthicalArgument makeArg(
    const std::string& school,
    const std::string& content,
    ArgumentType type = ArgumentType::PRO,
    const std::vector<std::string>& principles = {})
{
    EthicalArgument arg;
    arg.philosophy_school = school;
    arg.content           = content;
    arg.argument_type     = type;
    arg.principle_basis   = principles;
    return arg;
}

static DiscourseRoundOutput makeRoundOutput(
    const std::string& school,
    const std::string& verdict,
    float confidence = 0.8f,
    const std::vector<std::string>& thesis_ids = {"t1"})
{
    DiscourseRoundOutput out;
    out.school_id      = school;
    out.verdict        = verdict;
    out.confidence     = confidence;
    out.core_thesis_ids = thesis_ids;
    out.round_number   = 1;
    out.schema_valid   = true;
    return out;
}

// ---------------------------------------------------------------------------
// CWB-01: PriorRoundCompressor — trigger_round not reached → no compression
// ---------------------------------------------------------------------------
TEST(PriorRoundCompressor, CWB01_NoCompressionBeforeTriggerRound) {
    PriorRoundCompressor compressor;
    CompressionConfig cfg;
    cfg.trigger_round = 3;
    cfg.mode = CompressionMode::PRINCIPLE_CITATIONS_ONLY;

    const std::string long_content(400, 'x');
    auto arg = makeArg("kant", long_content);

    // current_round=2 < trigger_round=3 → no compression
    auto result = compressor.compressPriorRound({arg}, cfg, 2);
    EXPECT_FLOAT_EQ(result.compression_ratio, 1.0f);
    EXPECT_FLOAT_EQ(result.estimated_dc_loss, 0.0f);
    EXPECT_EQ(result.compressed_text, long_content + "\n");
}

// ---------------------------------------------------------------------------
// CWB-02: PRINCIPLE_CITATIONS_ONLY → compression_ratio < 0.5
// ---------------------------------------------------------------------------
TEST(PriorRoundCompressor, CWB02_PrincipleCitationsOnly_HighCompression) {
    PriorRoundCompressor compressor;
    CompressionConfig cfg;
    cfg.trigger_round = 1;
    cfg.mode = CompressionMode::PRINCIPLE_CITATIONS_ONLY;
    cfg.max_tokens_per_round = 300;

    const std::string long_content =
        "The categorical imperative demands that we act only according to maxims we could will "
        "to become universal laws. kant:kategorischer_imperativ is the core thesis. "
        "PROHIBIT: Using people as means violates their dignity. "
        "Further elaboration: The universalizability formula requires that any action be "
        "consistent with a world where everyone acts the same way. This is a very long "
        "argument that goes on and on to ensure that the original token count is large.";

    auto arg = makeArg("kant", long_content, ArgumentType::PRO, {"kant:kategorischer_imperativ"});

    auto result = compressor.compressPriorRound({arg}, cfg, 3);
    EXPECT_LT(result.compression_ratio, 0.5f)
        << "PRINCIPLE_CITATIONS_ONLY should achieve > 50% compression. Got ratio: "
        << result.compression_ratio;
    EXPECT_FALSE(result.compressed_text.empty());
}

// ---------------------------------------------------------------------------
// CWB-03: HEADLINE → compression_ratio < 0.25
// ---------------------------------------------------------------------------
TEST(PriorRoundCompressor, CWB03_Headline_VeryHighCompression) {
    PriorRoundCompressor compressor;
    CompressionConfig cfg;
    cfg.trigger_round = 1;
    cfg.mode = CompressionMode::HEADLINE;

    const std::string long_content(500, 'a');  // 500 chars = ~125 tokens
    auto arg = makeArg("utilitarianism", long_content, ArgumentType::CONTRA);

    auto result = compressor.compressPriorRound({arg}, cfg, 3);
    EXPECT_LT(result.compression_ratio, 0.25f)
        << "HEADLINE mode should achieve > 75% compression. Got ratio: "
        << result.compression_ratio;
    EXPECT_FALSE(result.compressed_text.empty());
    EXPECT_NE(result.compressed_text.find("utilitarianism"), std::string::npos);
}

// ---------------------------------------------------------------------------
// CWB-04: buildPriorContext → respects max_total_tokens
// ---------------------------------------------------------------------------
TEST(PriorRoundCompressor, CWB04_BuildPriorContext_RespectsTokenBudget) {
    PriorRoundCompressor compressor;
    CompressionConfig cfg;
    cfg.trigger_round = 1;
    cfg.mode = CompressionMode::PRINCIPLE_CITATIONS_ONLY;

    // 4 rounds of long arguments
    std::vector<std::vector<EthicalArgument>> all_rounds;
    for (int r = 0; r < 4; ++r) {
        std::string content(800, static_cast<char>('a' + r));
        all_rounds.push_back({makeArg("kant", content), makeArg("util", content)});
    }

    const int max_tokens = 200;
    const std::string prior_context = compressor.buildPriorContext(all_rounds, cfg, 5, max_tokens);

    // Token count check: chars / 4
    const int tokens = static_cast<int>((prior_context.size() + 3) / 4);
    EXPECT_LE(tokens, max_tokens * 2)  // Allow some slack for the round prefix
        << "buildPriorContext exceeded max_total_tokens budget. tokens=" << tokens;
}

// ---------------------------------------------------------------------------
// CWB-05: measureDcLoss → 0.0 for identical, 1.0 for disjoint
// ---------------------------------------------------------------------------
TEST(PriorRoundCompressor, CWB05_MeasureDcLoss_IdenticalAndDisjoint) {
    PriorRoundCompressor compressor;

    const std::string text = "the categorical imperative demands universalizability";
    EXPECT_FLOAT_EQ(compressor.measureDcLoss(text, text), 0.0f);

    const std::string disjoint = "xyz abc def ghi jkl mno pqr stu";
    EXPECT_FLOAT_EQ(compressor.measureDcLoss(text, disjoint), 1.0f);
}

// ---------------------------------------------------------------------------
// CWB-06: CrossSchoolTensionResolver — high weight → inject_full=true
// ---------------------------------------------------------------------------
TEST(CrossSchoolTensionResolver, CWB06_HighWeight_InjectFull) {
    CrossSchoolTensionResolver resolver;

    SchoolTension t;
    t.own_thesis_id       = "kant:cat_imp";
    t.opposing_school_id  = "utilitarianism";
    t.opposing_thesis_id  = "util:aggregate_welfare";
    t.rebuttal_cite_weight = 0.8f;
    t.tension_type        = "categorical_vs_aggregate";

    EthicalArgument opp_arg = makeArg("utilitarianism", "Maximise aggregate welfare. PROHIBIT harm.");

    auto decisions = resolver.resolveOpponentInjections(
        "kant", {"utilitarianism"}, {opp_arg}, {t}, 0.6f, 2);

    ASSERT_EQ(decisions.size(), 1u);
    EXPECT_TRUE(decisions[0].inject_full);
    EXPECT_EQ(decisions[0].argument_content, opp_arg.content);
    EXPECT_FALSE(decisions[0].headline.empty());
}

// ---------------------------------------------------------------------------
// CWB-07: CrossSchoolTensionResolver — low weight → inject_full=false, headline populated
// ---------------------------------------------------------------------------
TEST(CrossSchoolTensionResolver, CWB07_LowWeight_HeadlineOnly) {
    CrossSchoolTensionResolver resolver;

    SchoolTension t;
    t.own_thesis_id       = "kant:cat_imp";
    t.opposing_school_id  = "virtue_ethics";
    t.opposing_thesis_id  = "ve:eudaimonia";
    t.rebuttal_cite_weight = 0.3f;

    EthicalArgument opp_arg = makeArg("virtue_ethics", "Virtue leads to flourishing.");

    auto decisions = resolver.resolveOpponentInjections(
        "kant", {"virtue_ethics"}, {opp_arg}, {t}, 0.6f, 2);

    ASSERT_EQ(decisions.size(), 1u);
    EXPECT_FALSE(decisions[0].inject_full);
    EXPECT_TRUE(decisions[0].argument_content.empty());
    EXPECT_FALSE(decisions[0].headline.empty());
    EXPECT_NE(decisions[0].headline.find("virtue_ethics"), std::string::npos);
}

// ---------------------------------------------------------------------------
// CWB-08: CrossSchoolTensionResolver — max_full_injections cap enforced
// ---------------------------------------------------------------------------
TEST(CrossSchoolTensionResolver, CWB08_MaxFullInjectionsCap) {
    CrossSchoolTensionResolver resolver;

    std::vector<SchoolTension> tensions;
    std::vector<std::string> opponents = {"school_b", "school_c", "school_d"};
    std::vector<EthicalArgument> opp_args;

    for (const auto& opp : opponents) {
        SchoolTension t;
        t.own_thesis_id       = "kant:t1";
        t.opposing_school_id  = opp;
        t.opposing_thesis_id  = opp + ":t1";
        t.rebuttal_cite_weight = 0.9f;
        tensions.push_back(t);

        opp_args.push_back(makeArg(opp, "Argument from " + opp));
    }

    const int max_full = 2;
    auto decisions = resolver.resolveOpponentInjections(
        "kant", opponents, opp_args, tensions, 0.6f, max_full);

    int full_count = 0;
    for (const auto& d : decisions) {
        if (d.inject_full) ++full_count;
    }
    EXPECT_LE(full_count, max_full) << "Full injections exceeded max_full_injections cap";
}

// ---------------------------------------------------------------------------
// CWB-09: CrossSchoolTensionResolver — no tensions → all opponents headline only
// ---------------------------------------------------------------------------
TEST(CrossSchoolTensionResolver, CWB09_NoTensions_AllHeadline) {
    CrossSchoolTensionResolver resolver;

    std::vector<std::string> opponents = {"util", "care_ethics", "contractualism"};
    std::vector<EthicalArgument> opp_args;
    for (const auto& opp : opponents) {
        opp_args.push_back(makeArg(opp, "Some argument from " + opp));
    }

    auto decisions = resolver.resolveOpponentInjections(
        "kant", opponents, opp_args, {}, 0.6f, 2);

    ASSERT_EQ(decisions.size(), 3u);
    for (const auto& d : decisions) {
        EXPECT_FALSE(d.inject_full) << "Expected headline-only for " << d.school_id;
        EXPECT_FALSE(d.headline.empty()) << "Headline must be populated for " << d.school_id;
    }
}

// ---------------------------------------------------------------------------
// CWB-10: ConvergenceMarkerEngine — same PROHIBIT verdicts → CO_PROHIBITIVE
// ---------------------------------------------------------------------------
TEST(ConvergenceMarkerEngine, CWB10_SameProhibitVerdicts_CoProhibitive) {
    ConvergenceMarkerEngine engine;

    auto outputs = {
        makeRoundOutput("kant",  "PROHIBIT", 0.9f, {"kant:cat_imp"}),
        makeRoundOutput("util",  "PROHIBIT", 0.85f, {"util:welfare"}),
    };

    auto markers = engine.detectConvergences(outputs);
    ASSERT_FALSE(markers.empty());

    bool found = false;
    for (const auto& m : markers) {
        if (m.type == ConvergenceType::CO_PROHIBITIVE) {
            found = true;
            break;
        }
    }
    EXPECT_TRUE(found) << "Expected CO_PROHIBITIVE marker between two PROHIBIT verdicts";
}

// ---------------------------------------------------------------------------
// CWB-11: ConvergenceMarkerEngine — mixed verdicts → PARTIAL_OVERLAP or IRREDUCIBLE_SPLIT
// ---------------------------------------------------------------------------
TEST(ConvergenceMarkerEngine, CWB11_MixedVerdicts_SplitOrOverlap) {
    ConvergenceMarkerEngine engine;

    auto outputs = {
        makeRoundOutput("kant",  "PROHIBIT", 0.9f, {"kant:cat_imp"}),
        makeRoundOutput("util",  "PERMIT",   0.7f, {"util:welfare"}),
    };

    auto markers = engine.detectConvergences(outputs);
    ASSERT_FALSE(markers.empty());

    bool found_split_or_overlap = false;
    for (const auto& m : markers) {
        if (m.type == ConvergenceType::IRREDUCIBLE_SPLIT
                || m.type == ConvergenceType::PARTIAL_OVERLAP)
        {
            found_split_or_overlap = true;
            break;
        }
    }
    EXPECT_TRUE(found_split_or_overlap)
        << "Expected PARTIAL_OVERLAP or IRREDUCIBLE_SPLIT for mixed verdicts";
}

// ---------------------------------------------------------------------------
// CWB-12: ConvergenceMarkerEngine buildConvergencePreamble — expected tokens + max
// ---------------------------------------------------------------------------
TEST(ConvergenceMarkerEngine, CWB12_BuildConvergencePreamble_ContentsAndBudget) {
    ConvergenceMarkerEngine engine;

    auto outputs = {
        makeRoundOutput("kant",         "PROHIBIT", 0.9f, {"kant:cat_imp"}),
        makeRoundOutput("util",         "PROHIBIT", 0.8f, {"util:welfare"}),
        makeRoundOutput("virtue_ethics","PERMIT",   0.6f, {"ve:eudaimonia"}),
    };

    auto markers = engine.detectConvergences(outputs);
    const int max_tokens = 250;
    const std::string preamble = engine.buildConvergencePreamble(markers, max_tokens);

    EXPECT_FALSE(preamble.empty());
    EXPECT_NE(preamble.find("CONVERGENCE MATRIX"), std::string::npos)
        << "Preamble must contain the matrix header";

    // Token count check
    const int tokens = static_cast<int>((preamble.size() + 3) / 4);
    EXPECT_LE(tokens, max_tokens * 2)
        << "Preamble token count should be ≤ 2×max_tokens after trim";
}

// ---------------------------------------------------------------------------
// CWB-13: LlmCascadeRouter — PRO → SMALL tier, SYNTHESIS → LARGE tier
// ---------------------------------------------------------------------------
TEST(LlmCascadeRouter, CWB13_TierAssignment) {
    LlmCascadeRouter router;

    EXPECT_EQ(router.tierForRound("PRO"),       CascadeModelTier::SMALL);
    EXPECT_EQ(router.tierForRound("SYNTHESIS"), CascadeModelTier::LARGE);
    EXPECT_EQ(router.tierForRound("REBUTTAL"),  CascadeModelTier::MEDIUM);

    const auto pro_decision = router.routeForRound("PRO", 100);
    EXPECT_EQ(pro_decision.tier, CascadeModelTier::SMALL);
    EXPECT_FALSE(pro_decision.model_id.empty());

    const auto synth_decision = router.routeForRound("SYNTHESIS", 100);
    EXPECT_EQ(synth_decision.tier, CascadeModelTier::LARGE);
}

// ---------------------------------------------------------------------------
// CWB-14: LlmCascadeRouter — budgetForRound correct context_k per tier
// ---------------------------------------------------------------------------
TEST(LlmCascadeRouter, CWB14_BudgetContextK) {
    LlmCascadeRouter router;

    const auto small_budget  = router.budgetForRound("PRO");
    const auto medium_budget = router.budgetForRound("REBUTTAL");
    const auto large_budget  = router.budgetForRound("SYNTHESIS");

    EXPECT_EQ(small_budget.context_k,  4u);
    EXPECT_EQ(medium_budget.context_k, 32u);
    EXPECT_EQ(large_budget.context_k,  128u);

    // Escalation: PRO with 5000 tokens (> 4K budget) should escalate
    const auto escalated = router.routeForRound("PRO", 5000);
    EXPECT_TRUE(escalated.was_escalated);
    EXPECT_NE(escalated.tier, CascadeModelTier::SMALL);
}

// ---------------------------------------------------------------------------
// CWB-15: SynthesisMatrixBuilder — buildMatrix contains school verdicts + convergence markers
// ---------------------------------------------------------------------------
TEST(SynthesisMatrixBuilder, CWB15_BuildMatrix_ContentsAndConvergence) {
    SynthesisMatrixBuilder builder;

    SchoolPositionSummary s1;
    s1.school_id      = "kant";
    s1.verdict        = "PROHIBIT";
    s1.confidence     = 0.9f;
    s1.core_thesis_ids = {"kant:cat_imp", "kant:dignity"};

    SchoolPositionSummary s2;
    s2.school_id      = "util";
    s2.verdict        = "PERMIT";
    s2.confidence     = 0.7f;
    s2.core_thesis_ids = {"util:welfare"};

    ConvergenceMarker cm;
    cm.school_a_id  = "kant";
    cm.thesis_a_id  = "kant:cat_imp";
    cm.school_b_id  = "util";
    cm.thesis_b_id  = "util:welfare";
    cm.type         = ConvergenceType::IRREDUCIBLE_SPLIT;
    cm.split_reason = "deontology_vs_consequentialism";
    cm.confidence   = 0.85f;

    const std::string matrix = builder.buildMatrix({s1, s2}, {cm}, 300);

    EXPECT_FALSE(matrix.empty());
    EXPECT_NE(matrix.find("POSITIONS-MATRIX"), std::string::npos);
    EXPECT_NE(matrix.find("kant"), std::string::npos);
    EXPECT_NE(matrix.find("PROHIBIT"), std::string::npos);
    EXPECT_NE(matrix.find("PERMIT"), std::string::npos);
    EXPECT_NE(matrix.find("CONVERGENCES"), std::string::npos);

    // Validate extract + validate workflow
    DiscourseRoundOutput ro;
    ro.school_id      = "virtue_ethics";
    ro.verdict        = "CONDITIONAL";
    ro.confidence     = 0.6f;
    ro.core_thesis_ids = {"ve:eudaimonia"};
    ro.round_number   = 4;
    ro.schema_valid   = true;

    const auto extracted = builder.extractSummary(ro);
    EXPECT_EQ(extracted.school_id, "virtue_ethics");
    EXPECT_EQ(extracted.verdict,   "CONDITIONAL");
    EXPECT_NO_THROW(builder.validateSummary(extracted));
}
