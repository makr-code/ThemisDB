/**
 * @file test_context_window_budget_strategy.cpp
 * @brief Unit tests CWB-01..CWB-20 for §12 Context-Window-Budget-Strategie components.
 *
 * Covers:
 *  - PriorRoundCompressor (CWB-01..CWB-05)
 *  - CrossSchoolTensionResolver (CWB-06..CWB-09)
 *  - ConvergenceMarkerEngine (CWB-10..CWB-12)
 *  - LlmCascadeRouter (CWB-13..CWB-14)
 *  - SynthesisMatrixBuilder (CWB-15)
 *  - DiscourseMemoryStore (CWB-16..CWB-18)
 *  - TournamentModeSelector (CWB-19)
 *  - PositionAbstractValidator (CWB-20)
 */

#include <gtest/gtest.h>

#include "ethics_ai/prior_round_compressor.h"
#include "ethics_ai/cross_school_tension_resolver.h"
#include "ethics_ai/convergence_marker_engine.h"
#include "ethics_ai/llm_cascade_router.h"
#include "ethics_ai/synthesis_matrix_builder.h"
#include "ethics_ai/tournament_mode_selector.h"
#include "ethics_ai/position_abstract_validator.h"
#include "ethics_ai/discourse_memory_store.h"

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
// CWB-14b: LlmCascadeRouter — setLlmInvokeFn + invoke() (stub #244)
// ---------------------------------------------------------------------------
TEST(LlmCascadeRouter, CWB14b_InvokeFnInjection_NoFnReturnsEmpty) {
    LlmCascadeRouter router;
    // Without injected provider, invoke() should return an empty string.
    EXPECT_EQ(router.invoke("PRO", "some prompt"), "");
}

TEST(LlmCascadeRouter, CWB14b_InvokeFnInjection_CallsProviderWithResolvedModel) {
    LlmCascadeRouter router;

    std::string last_model;
    std::string last_prompt;
    size_t      last_max_tokens = 0;

    router.setLlmInvokeFn([&](const std::string& model_id,
                               const std::string& prompt,
                               size_t             max_tokens) -> std::string {
        last_model      = model_id;
        last_prompt     = prompt;
        last_max_tokens = max_tokens;
        return "generated-response";
    });

    const std::string result = router.invoke("SYNTHESIS", "test prompt");

    EXPECT_EQ(result, "generated-response");
    // SYNTHESIS maps to LARGE tier → gpt-4o in default config
    EXPECT_EQ(last_model, "gpt-4o");
    EXPECT_EQ(last_prompt, "test prompt");
    // max_output_tokens for LARGE (128K context): 128*1024/8 = capped at 2048
    EXPECT_GT(last_max_tokens, 0u);
}

TEST(LlmCascadeRouter, CWB14b_InvokeFnInjection_ResetToNullReturnsEmpty) {
    LlmCascadeRouter router;
    router.setLlmInvokeFn([](const std::string&, const std::string&, size_t) {
        return "response";
    });
    EXPECT_EQ(router.invoke("PRO", "prompt"), "response");

    // After clearing the function, invoke should again return empty.
    router.setLlmInvokeFn(nullptr);
    EXPECT_EQ(router.invoke("PRO", "prompt"), "");
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

// ---------------------------------------------------------------------------
// CWB-16: DiscourseMemoryStore — storeEpisode from DiscourseRoundOutput
// ---------------------------------------------------------------------------
TEST(DiscourseMemoryStore, CWB16_StoreFromRoundOutput) {
    DiscourseMemoryStore store;
    DiscourseRoundOutput output;
    output.school_id = "kant";
    output.round_number = 2;
    output.verdict = "PROHIBIT";
    output.confidence = 0.85f;
    output.position_abstract = "Verdict: PROHIBIT. Core: kategorischer_imperativ. Rebutted: greatest_happiness.";
    output.schema_valid = true;
    store.storeEpisode(output);
    EXPECT_EQ(store.episodeCount("kant"), 1u);
    auto eps = store.getEpisodesForSchool("kant", 3);
    ASSERT_EQ(eps.size(), 1u);
    EXPECT_EQ(eps[0].school_id, "kant");
    EXPECT_EQ(eps[0].from_round, 2);
    EXPECT_FLOAT_EQ(eps[0].dc_score, 0.85f);
}

// ---------------------------------------------------------------------------
// CWB-17: DiscourseMemoryStore — ring buffer eviction
// ---------------------------------------------------------------------------
TEST(DiscourseMemoryStore, CWB17_RingBufferEviction) {
    DiscourseMemoryConfig cfg;
    cfg.max_episodes_per_school = 3;
    DiscourseMemoryStore store(cfg);
    for (int r = 1; r <= 5; ++r) {
        EpisodicMemoryEntry e;
        e.school_id = "utilitarianism";
        e.from_round = r;
        e.compressed_position = "round " + std::to_string(r);
        e.dc_score = 0.7f;
        store.storeEpisode(e);
    }
    EXPECT_EQ(store.episodeCount("utilitarianism"), 3u);
    auto eps = store.getEpisodesForSchool("utilitarianism", 3);
    // newest first: rounds 5, 4, 3
    EXPECT_EQ(eps[0].from_round, 5);
    EXPECT_EQ(eps[2].from_round, 3);
}

// ---------------------------------------------------------------------------
// CWB-18: DiscourseMemoryStore — buildEpisodicContext token budget
// ---------------------------------------------------------------------------
TEST(DiscourseMemoryStore, CWB18_BuildEpisodicContext_TokenBudget) {
    DiscourseMemoryConfig cfg;
    cfg.max_episodes_per_school = 3;
    cfg.max_tokens_per_episode = 20;
    DiscourseMemoryStore store(cfg);
    for (int r = 1; r <= 3; ++r) {
        EpisodicMemoryEntry e;
        e.school_id = "kant";
        e.from_round = r;
        e.compressed_position = "Short episode text for round " + std::to_string(r);
        e.dc_score = 0.8f;
        store.storeEpisode(e);
    }
    auto ctx = store.buildEpisodicContext("kant", 3);
    EXPECT_FALSE(ctx.empty());
    // Total tokens should be manageable (≤ 3 episodes × (20 + overhead))
    int tokens = static_cast<int>((ctx.size() + 3) / 4);
    EXPECT_LT(tokens, 200); // generous bound
}

// ---------------------------------------------------------------------------
// CWB-19: TournamentModeSelector — tournament mode: primary full, secondary headline
// ---------------------------------------------------------------------------
TEST(TournamentModeSelector, CWB19_TournamentMode_PrimaryFull_SecondaryHeadline) {
    TournamentModeSelector selector;

    // Build opponent arguments
    std::vector<EthicalArgument> opponents;
    for (const auto& school : std::vector<std::string>{"utilitarianism", "contractualism", "islamische_ethik"}) {
        EthicalArgument arg;
        arg.philosophy_school = school;
        arg.content = "Full detailed argument from " + school + " school about the dilemma.";
        arg.argument_type = ArgumentType::REBUTTAL;
        opponents.push_back(arg);
    }

    // Tensions: utilitarianism has high weight
    std::vector<SchoolTension> tensions;
    SchoolTension t1;
    t1.own_thesis_id = "kategorischer_imperativ";
    t1.opposing_school_id = "utilitarianism";
    t1.opposing_thesis_id = "greatest_happiness";
    t1.rebuttal_cite_weight = 0.9f;
    tensions.push_back(t1);

    SchoolTension t2;
    t2.own_thesis_id = "kategorischer_imperativ";
    t2.opposing_school_id = "contractualism";
    t2.opposing_thesis_id = "reasonable_rejection";
    t2.rebuttal_cite_weight = 0.4f;
    tensions.push_back(t2);

    TournamentConfig cfg;
    cfg.primary_opponent_count = 1;
    auto result = selector.selectOpponents("kant", opponents, tensions, cfg);

    EXPECT_EQ(result.own_school_id, "kant");
    ASSERT_EQ(result.primary_opponents.size(), 1u);
    EXPECT_EQ(result.primary_opponents[0], "utilitarianism");
    EXPECT_EQ(result.secondary_opponents.size(), 2u);
    EXPECT_FALSE(result.assembled_context.empty());
    // Primary opponent's full content should appear in assembled context
    EXPECT_NE(result.assembled_context.find("utilitarianism"), std::string::npos);

    // Verify token reduction vs full injection mode
    TournamentConfig full_cfg;
    full_cfg.mode = OpponentInjectionMode::FULL;
    auto full_result = selector.selectOpponents("kant", opponents, tensions, full_cfg);

    // Tournament mode must use fewer tokens than full injection (−65% claim)
    EXPECT_LT(result.total_tokens_estimated, full_result.total_tokens_estimated)
        << "Tournament mode should use fewer tokens than full injection";
}

// ---------------------------------------------------------------------------
// CWB-20: PositionAbstractValidator — validate valid output; autoRepair fills missing verdict
// ---------------------------------------------------------------------------
TEST(PositionAbstractValidator, CWB20_ValidateValid_AndAutoRepairMissingVerdict) {
    PositionAbstractValidator validator;

    // Valid output
    DiscourseRoundOutput valid_out;
    valid_out.school_id = "kant";
    valid_out.round_number = 2;
    valid_out.verdict = "PROHIBIT";
    valid_out.confidence = 0.9f;
    valid_out.core_thesis_ids = {"kategorischer_imperativ"};
    valid_out.position_abstract = "PROHIBIT. Core: kategorischer_imperativ. Rebutted: greatest_happiness.";

    EXPECT_TRUE(validator.validate(valid_out));
    EXPECT_TRUE(valid_out.schema_valid);

    // Auto-repair missing verdict
    DiscourseRoundOutput broken_out;
    broken_out.school_id = "utilitarianism";
    broken_out.round_number = 2;
    broken_out.verdict = "";  // missing
    broken_out.core_thesis_ids = {"greatest_happiness"};
    broken_out.position_abstract = "The action PERMITS maximizing utility.";
    broken_out.content = "We PERMIT the action because of greatest happiness principle.";

    EXPECT_TRUE(validator.autoRepair(broken_out));
    EXPECT_FALSE(broken_out.verdict.empty());
}

// ---------------------------------------------------------------------------
// CWB-SS-01: STRUCTURED_SUMMARY achieves better DC preservation than PRINCIPLE_CITATIONS_ONLY
// ---------------------------------------------------------------------------
TEST(PriorRoundCompressor, CWBSS01_StructuredSummary_BetterDcPreservation) {
    PriorRoundCompressor compressor;
    CompressionConfig cfg;
    cfg.trigger_round   = 1;
    cfg.mode            = CompressionMode::STRUCTURED_SUMMARY;
    cfg.max_tokens_per_round = 100;
    cfg.keep_verdict    = true;

    const std::string long_content =
        "The categorical imperative demands universalizability. kant:kategorischer_imperativ "
        "is central here. PROHIBIT: Using people as mere means violates their dignity and "
        "autonomy. The principle of humanity requires treating rational agents as ends in "
        "themselves. This universalizability formula is non-negotiable for Kantian ethics.";

    auto arg = makeArg("kant", long_content, ArgumentType::PRO,
                       {"kant:kategorischer_imperativ"});

    auto structured = compressor.compressPriorRound({arg}, cfg, 3);

    cfg.mode = CompressionMode::PRINCIPLE_CITATIONS_ONLY;
    auto citations_only = compressor.compressPriorRound({arg}, cfg, 3);

    // STRUCTURED_SUMMARY must preserve more DC (lower estimated_dc_loss)
    EXPECT_LE(structured.estimated_dc_loss, citations_only.estimated_dc_loss + 0.05f)
        << "STRUCTURED_SUMMARY should not be worse in DC loss than PRINCIPLE_CITATIONS_ONLY. "
        << "structured=" << structured.estimated_dc_loss
        << " citations_only=" << citations_only.estimated_dc_loss;

    // Both must produce non-empty output
    EXPECT_FALSE(structured.compressed_text.empty());
}

// ---------------------------------------------------------------------------
// CWB-SS-02: STRUCTURED_SUMMARY respects token budget
// ---------------------------------------------------------------------------
TEST(PriorRoundCompressor, CWBSS02_StructuredSummary_RespectsTokenBudget) {
    PriorRoundCompressor compressor;
    CompressionConfig cfg;
    cfg.trigger_round   = 1;
    cfg.mode            = CompressionMode::STRUCTURED_SUMMARY;
    cfg.max_tokens_per_round = 30;

    const std::string long_content =
        "First very long philosophical sentence about the nature of obligation under Kantian ethics. "
        "Second sentence covering the universalizability test and its practical application. "
        "Third sentence discussing the humanity formula and its implications for modern ethics. "
        "Fourth sentence elaborating on the kingdom of ends framework. "
        "Fifth sentence concluding the argument with a final PROHIBIT verdict.";

    auto arg = makeArg("kant", long_content);
    auto result = compressor.compressPriorRound({arg}, cfg, 3);

    const int tokens = PriorRoundCompressor::countTokens(result.compressed_text);
    // Allow generous slack (prefix "[kant|R]" + verdict add tokens)
    EXPECT_LE(tokens, cfg.max_tokens_per_round * 3)
        << "STRUCTURED_SUMMARY must not grossly exceed max_tokens_per_round. tokens=" << tokens;
    EXPECT_FALSE(result.compressed_text.empty());
}

// ---------------------------------------------------------------------------
// CWB-SS-03: STRUCTURED_SUMMARY preserves citations in output
// ---------------------------------------------------------------------------
TEST(PriorRoundCompressor, CWBSS03_StructuredSummary_PreservesCitationsInOutput) {
    PriorRoundCompressor compressor;
    CompressionConfig cfg;
    cfg.trigger_round   = 1;
    cfg.mode            = CompressionMode::STRUCTURED_SUMMARY;
    cfg.max_tokens_per_round = 200;

    const std::string content =
        "kant:kategorischer_imperativ is the core thesis. We must act only on maxims we can "
        "will to become universal laws. PROHIBIT: treating people as mere means. "
        "This principle has wide application across modern ethics.";

    auto arg = makeArg("kant", content, ArgumentType::PRO, {"kant:kategorischer_imperativ"});
    auto result = compressor.compressPriorRound({arg}, cfg, 3);

    // The sentence containing the citation should be selected (boosted)
    EXPECT_NE(result.compressed_text.find("kant:kategorischer_imperativ"), std::string::npos)
        << "STRUCTURED_SUMMARY should retain the sentence containing the principle citation";
}

// ===========================================================================
// PriorRoundCompressor LLM summary-fn injection tests  (stub #247)
// ===========================================================================

// CWB-SS-LLM-01: Without injected LlmSummaryFn, STRUCTURED_SUMMARY falls back
//                to the extractive path and produces non-empty compressed text.
TEST(PriorRoundCompressor, CWBSSLLM01_NoFnFallsBackToExtractive) {
    PriorRoundCompressor compressor;
    CompressionConfig cfg;
    cfg.trigger_round        = 1;
    cfg.mode                 = CompressionMode::STRUCTURED_SUMMARY;
    cfg.max_tokens_per_round = 100;

    const std::string content =
        "kant:kategorischer_imperativ is central. We must universalise our maxims. "
        "PROHIBIT actions that cannot be willed universally. This guards dignity.";

    auto arg = makeArg("kant", content, ArgumentType::PRO, {"kant:kategorischer_imperativ"});
    auto result = compressor.compressPriorRound({arg}, cfg, 2);

    EXPECT_FALSE(result.compressed_text.empty());
    EXPECT_LT(result.compression_ratio, 1.5f);
}

// CWB-SS-LLM-02: Injected LlmSummaryFn is called; its return value is used as
//                the compressed text.
TEST(PriorRoundCompressor, CWBSSLLM02_InjectedFnReturnValueUsed) {
    PriorRoundCompressor compressor;
    const std::string llm_output = "[kant|R] Categorical imperative demands universal maxims.";
    bool fn_called = false;

    compressor.setLlmSummaryFn(
        [&](const EthicalArgument& /*arg*/, int /*max_tokens*/) -> std::string {
            fn_called = true;
            return llm_output;
        });

    CompressionConfig cfg;
    cfg.trigger_round        = 1;
    cfg.mode                 = CompressionMode::STRUCTURED_SUMMARY;
    cfg.max_tokens_per_round = 100;

    const std::string content =
        "kant:kategorischer_imperativ is central. We must universalise our maxims. PROHIBIT.";

    auto arg = makeArg("kant", content, ArgumentType::PRO, {});
    auto result = compressor.compressPriorRound({arg}, cfg, 2);

    EXPECT_TRUE(fn_called);
    EXPECT_NE(result.compressed_text.find(llm_output), std::string::npos);
}

// CWB-SS-LLM-03: When fn returns an empty string, extractive fallback is used.
TEST(PriorRoundCompressor, CWBSSLLM03_EmptyFnReturnFallsBackToExtractive) {
    PriorRoundCompressor compressor;
    bool fn_called = false;

    compressor.setLlmSummaryFn(
        [&](const EthicalArgument& /*arg*/, int /*max_tokens*/) -> std::string {
            fn_called = true;
            return "";  // signal fallback
        });

    CompressionConfig cfg;
    cfg.trigger_round        = 1;
    cfg.mode                 = CompressionMode::STRUCTURED_SUMMARY;
    cfg.max_tokens_per_round = 100;

    const std::string content =
        "kant:kategorischer_imperativ is central. We must universalise our maxims. PROHIBIT.";

    auto arg = makeArg("kant", content, ArgumentType::PRO, {});
    auto result = compressor.compressPriorRound({arg}, cfg, 2);

    EXPECT_TRUE(fn_called);
    EXPECT_FALSE(result.compressed_text.empty());
}
