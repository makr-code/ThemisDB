/**
 * @file test_prompt_ab_experiment.cpp
 * @brief Unit tests for PromptABExperimentFramework (v1.9.0).
 *
 * Acceptance criteria:
 *  AC-1   PromptABExperimentFramework default-constructs without error.
 *  AC-2   create() assigns a non-empty experiment_id when none is provided.
 *  AC-3   create() preserves a caller-supplied experiment_id.
 *  AC-4   create() clamps split_pct to [0, 100].
 *  AC-5   getExperiment() returns the stored experiment.
 *  AC-6   getExperiment() returns nullopt for unknown ID.
 *  AC-7   listExperiments() returns all registered experiments.
 *  AC-8   stop() sets status to INCONCLUSIVE for a RUNNING experiment.
 *  AC-9   stop() returns false for an unknown experiment ID.
 *  AC-10  assignVariant() returns CONTROL for an unknown experiment ID.
 *  AC-11  assignVariant() returns CONTROL when split_pct == 0.
 *  AC-12  assignVariant() returns TREATMENT when split_pct == 100.
 *  AC-13  assignVariant() is deterministic: same request_id always yields same variant.
 *  AC-14  assignVariant() returns CONTROL for a stopped experiment.
 *  AC-15  recordOutcome() returns false for an unknown experiment ID.
 *  AC-16  recordOutcome() returns false for a stopped experiment.
 *  AC-17  recordOutcome() appends scores correctly (confirmed via getSummary counts).
 *  AC-18  getOutcomes() returns all recorded outcomes.
 *  AC-19  ExperimentOutcome::toJson() contains all required keys.
 *  AC-20  checkSignificance() returns false when samples < min_samples.
 *  AC-21  checkSignificance() detects WINNER_TREATMENT when treatment scores are clearly higher.
 *  AC-22  checkSignificance() detects WINNER_CONTROL when control scores are clearly higher.
 *  AC-23  checkSignificance() does not fire on identical score distributions.
 *  AC-24  recordOutcome() auto-promotes winner once min_samples are reached (WinnerCallback fired).
 *  AC-25  promoteWinner() returns treatment version ID when WINNER_TREATMENT.
 *  AC-26  promoteWinner() returns control version ID when WINNER_CONTROL.
 *  AC-27  promoteWinner() returns empty string when no winner has been declared.
 *  AC-28  promoteWinner() sets status to COMPLETED.
 *  AC-29  getSummary() contains delta_pct, p_value, significant, and winner_version_id.
 *  AC-30  PromptExperiment::toJson() / fromJson() round-trips correctly.
 */

#include <gtest/gtest.h>

#include "prompt_engineering/prompt_ab_experiment.h"

#include <algorithm>
#include <cmath>
#include <string>
#include <vector>

using namespace themis::prompt_engineering;

// ============================================================================
// Helpers
// ============================================================================

namespace {

PromptExperiment makeExperiment(
    const std::string& template_id     = "tpl",
    const std::string& control_v       = "v1.0",
    const std::string& treatment_v     = "v1.1",
    int                split_pct       = 50,
    std::size_t        min_samples     = 5,
    double             confidence_level = 0.95) {
    PromptExperiment e;
    e.template_id           = template_id;
    e.control_version_id    = control_v;
    e.treatment_version_id  = treatment_v;
    e.split_pct             = split_pct;
    e.min_samples           = min_samples;
    e.confidence_level      = confidence_level;
    return e;
}

// Feed `n` observations for each variant into `fw`.
void feedOutcomes(PromptABExperimentFramework& fw,
                  const std::string& id,
                  ExperimentVariant   variant,
                  double              score,
                  std::size_t         n) {
    for (std::size_t i = 0; i < n; ++i) {
        fw.recordOutcome(id, variant, score,
                         "req-" + std::to_string(i));
    }
}

} // anonymous namespace

// ============================================================================
// AC-1  Default construction
// ============================================================================

TEST(PromptABExperimentFrameworkTest, DefaultConstructs) {
    EXPECT_NO_THROW(PromptABExperimentFramework fw);
}

// ============================================================================
// AC-2  create() assigns non-empty ID when none supplied
// ============================================================================

TEST(PromptABExperimentFrameworkTest, CreateAssignsId) {
    PromptABExperimentFramework fw;
    const auto id = fw.create(makeExperiment());
    EXPECT_FALSE(id.empty());
}

// ============================================================================
// AC-3  create() preserves caller-supplied experiment_id
// ============================================================================

TEST(PromptABExperimentFrameworkTest, CreatePreservesId) {
    PromptABExperimentFramework fw;
    PromptExperiment e = makeExperiment();
    e.experiment_id = "my-custom-id";
    const auto id = fw.create(e);
    EXPECT_EQ(id, "my-custom-id");
}

// ============================================================================
// AC-4  create() clamps split_pct to [0, 100]
// ============================================================================

TEST(PromptABExperimentFrameworkTest, CreateClampsSplitPct) {
    PromptABExperimentFramework fw;
    {
        auto e = makeExperiment("tpl", "v1", "v2", -10);
        const auto id = fw.create(e);
        EXPECT_EQ(fw.getExperiment(id)->split_pct, 0);
    }
    {
        auto e = makeExperiment("tpl", "v1", "v2", 200);
        const auto id = fw.create(e);
        EXPECT_EQ(fw.getExperiment(id)->split_pct, 100);
    }
}

// ============================================================================
// AC-5  getExperiment() returns stored experiment
// ============================================================================

TEST(PromptABExperimentFrameworkTest, GetExperimentReturnsStored) {
    PromptABExperimentFramework fw;
    const auto id = fw.create(makeExperiment("tplX", "vA", "vB", 30));
    const auto opt = fw.getExperiment(id);
    ASSERT_TRUE(opt.has_value());
    EXPECT_EQ(opt->template_id, "tplX");
    EXPECT_EQ(opt->control_version_id, "vA");
    EXPECT_EQ(opt->treatment_version_id, "vB");
    EXPECT_EQ(opt->split_pct, 30);
}

// ============================================================================
// AC-6  getExperiment() returns nullopt for unknown ID
// ============================================================================

TEST(PromptABExperimentFrameworkTest, GetExperimentUnknown) {
    PromptABExperimentFramework fw;
    EXPECT_FALSE(fw.getExperiment("no-such-id").has_value());
}

// ============================================================================
// AC-7  listExperiments() returns all registered experiments
// ============================================================================

TEST(PromptABExperimentFrameworkTest, ListExperimentsAll) {
    PromptABExperimentFramework fw;
    fw.create(makeExperiment("t1", "v1", "v2"));
    fw.create(makeExperiment("t2", "v1", "v2"));
    fw.create(makeExperiment("t3", "v1", "v2"));
    EXPECT_EQ(fw.listExperiments().size(), 3u);
}

// ============================================================================
// AC-8  stop() sets status to INCONCLUSIVE for a RUNNING experiment
// ============================================================================

TEST(PromptABExperimentFrameworkTest, StopSetsInconclusive) {
    PromptABExperimentFramework fw;
    const auto id = fw.create(makeExperiment());
    EXPECT_TRUE(fw.stop(id));
    EXPECT_EQ(fw.getStatus(id), ExperimentStatus::INCONCLUSIVE);
}

// ============================================================================
// AC-9  stop() returns false for unknown ID
// ============================================================================

TEST(PromptABExperimentFrameworkTest, StopUnknownReturnsFalse) {
    PromptABExperimentFramework fw;
    EXPECT_FALSE(fw.stop("nonexistent"));
}

// ============================================================================
// AC-10  assignVariant() returns CONTROL for unknown experiment ID
// ============================================================================

TEST(PromptABExperimentFrameworkTest, AssignVariantUnknownReturnsControl) {
    PromptABExperimentFramework fw;
    ExperimentContext ctx{"unknown-exp", "user-1"};
    EXPECT_EQ(fw.assignVariant(ctx), ExperimentVariant::CONTROL);
}

// ============================================================================
// AC-11  assignVariant() returns CONTROL when split_pct == 0
// ============================================================================

TEST(PromptABExperimentFrameworkTest, AssignVariantZeroSplit) {
    PromptABExperimentFramework fw;
    const auto id = fw.create(makeExperiment("t", "v1", "v2", 0));
    for (int i = 0; i < 20; ++i) {
        ExperimentContext ctx{id, "user-" + std::to_string(i)};
        EXPECT_EQ(fw.assignVariant(ctx), ExperimentVariant::CONTROL);
    }
}

// ============================================================================
// AC-12  assignVariant() returns TREATMENT when split_pct == 100
// ============================================================================

TEST(PromptABExperimentFrameworkTest, AssignVariantFullSplit) {
    PromptABExperimentFramework fw;
    const auto id = fw.create(makeExperiment("t", "v1", "v2", 100));
    for (int i = 0; i < 20; ++i) {
        ExperimentContext ctx{id, "user-" + std::to_string(i)};
        EXPECT_EQ(fw.assignVariant(ctx), ExperimentVariant::TREATMENT);
    }
}

// ============================================================================
// AC-13  assignVariant() is deterministic for the same request_id
// ============================================================================

TEST(PromptABExperimentFrameworkTest, AssignVariantDeterministic) {
    PromptABExperimentFramework fw;
    const auto id = fw.create(makeExperiment("t", "v1", "v2", 50));
    ExperimentContext ctx{id, "stable-user-42"};
    const auto first = fw.assignVariant(ctx);
    for (int i = 0; i < 10; ++i) {
        EXPECT_EQ(fw.assignVariant(ctx), first);
    }
}

// ============================================================================
// AC-14  assignVariant() returns CONTROL for stopped experiment
// ============================================================================

TEST(PromptABExperimentFrameworkTest, AssignVariantStoppedReturnsControl) {
    PromptABExperimentFramework fw;
    const auto id = fw.create(makeExperiment("t", "v1", "v2", 100));
    fw.stop(id);
    ExperimentContext ctx{id, "user-1"};
    EXPECT_EQ(fw.assignVariant(ctx), ExperimentVariant::CONTROL);
}

// ============================================================================
// AC-15  recordOutcome() returns false for unknown experiment
// ============================================================================

TEST(PromptABExperimentFrameworkTest, RecordOutcomeUnknownReturnsFalse) {
    PromptABExperimentFramework fw;
    EXPECT_FALSE(fw.recordOutcome("no-exp",
                                  ExperimentVariant::CONTROL,
                                  0.8, "req-1"));
}

// ============================================================================
// AC-16  recordOutcome() returns false for stopped experiment
// ============================================================================

TEST(PromptABExperimentFrameworkTest, RecordOutcomeStoppedReturnsFalse) {
    PromptABExperimentFramework fw;
    const auto id = fw.create(makeExperiment());
    fw.stop(id);
    EXPECT_FALSE(fw.recordOutcome(id, ExperimentVariant::CONTROL, 0.8, "r"));
}

// ============================================================================
// AC-17  recordOutcome() appends scores (confirmed via getSummary counts)
// ============================================================================

TEST(PromptABExperimentFrameworkTest, RecordOutcomeIncrementsCount) {
    PromptABExperimentFramework fw;
    const auto id = fw.create(makeExperiment("t", "v1", "v2", 50, 1000));
    fw.recordOutcome(id, ExperimentVariant::CONTROL,   0.8, "r1");
    fw.recordOutcome(id, ExperimentVariant::CONTROL,   0.8, "r2");
    fw.recordOutcome(id, ExperimentVariant::TREATMENT, 0.9, "r3");
    const auto s = fw.getSummary(id);
    ASSERT_TRUE(s.has_value());
    EXPECT_EQ(s->control_samples,   2u);
    EXPECT_EQ(s->treatment_samples, 1u);
}

// ============================================================================
// AC-18  getOutcomes() returns all recorded outcomes
// ============================================================================

TEST(PromptABExperimentFrameworkTest, GetOutcomesAll) {
    PromptABExperimentFramework fw;
    const auto id = fw.create(makeExperiment("t", "v1", "v2", 50, 1000));
    fw.recordOutcome(id, ExperimentVariant::CONTROL,   0.7, "r1");
    fw.recordOutcome(id, ExperimentVariant::TREATMENT, 0.9, "r2");
    fw.recordOutcome(id, ExperimentVariant::CONTROL,   0.8, "r3");
    EXPECT_EQ(fw.getOutcomes(id).size(), 3u);
}

// ============================================================================
// AC-19  ExperimentOutcome::toJson() contains all required keys
// ============================================================================

TEST(PromptABExperimentFrameworkTest, OutcomeToJsonKeys) {
    PromptABExperimentFramework fw;
    const auto id = fw.create(makeExperiment("t", "v1", "v2", 50, 1000));
    fw.recordOutcome(id, ExperimentVariant::CONTROL, 0.8, "req-1");
    const auto outcomes = fw.getOutcomes(id);
    ASSERT_FALSE(outcomes.empty());
    const auto j = outcomes.front().toJson();
    EXPECT_TRUE(j.contains("experiment_id"));
    EXPECT_TRUE(j.contains("variant"));
    EXPECT_TRUE(j.contains("score"));
    EXPECT_TRUE(j.contains("request_id"));
    EXPECT_TRUE(j.contains("timestamp"));
}

// ============================================================================
// AC-20  checkSignificance() returns false when samples < min_samples
// ============================================================================

TEST(PromptABExperimentFrameworkTest, CheckSignificanceInsufficientSamples) {
    PromptABExperimentFramework fw;
    // min_samples = 200; we only add 3.
    const auto id = fw.create(makeExperiment("t", "v1", "v2", 50, 200));
    fw.recordOutcome(id, ExperimentVariant::CONTROL,   0.6, "r1");
    fw.recordOutcome(id, ExperimentVariant::TREATMENT, 0.9, "r2");
    EXPECT_FALSE(fw.checkSignificance(id));
    EXPECT_EQ(fw.getStatus(id), ExperimentStatus::RUNNING);
}

// ============================================================================
// AC-21  checkSignificance() → WINNER_TREATMENT when treatment clearly better
// ============================================================================

TEST(PromptABExperimentFrameworkTest, CheckSignificanceTreatmentWins) {
    PromptABExperimentFramework fw;
    const auto id = fw.create(makeExperiment("t", "v1", "v2", 50, 10));
    feedOutcomes(fw, id, ExperimentVariant::CONTROL,   0.3, 20);
    feedOutcomes(fw, id, ExperimentVariant::TREATMENT, 0.9, 20);
    fw.checkSignificance(id);
    EXPECT_EQ(fw.getStatus(id), ExperimentStatus::WINNER_TREATMENT);
}

// ============================================================================
// AC-22  checkSignificance() → WINNER_CONTROL when control clearly better
// ============================================================================

TEST(PromptABExperimentFrameworkTest, CheckSignificanceControlWins) {
    PromptABExperimentFramework fw;
    const auto id = fw.create(makeExperiment("t", "v1", "v2", 50, 10));
    feedOutcomes(fw, id, ExperimentVariant::CONTROL,   0.9, 20);
    feedOutcomes(fw, id, ExperimentVariant::TREATMENT, 0.3, 20);
    fw.checkSignificance(id);
    EXPECT_EQ(fw.getStatus(id), ExperimentStatus::WINNER_CONTROL);
}

// ============================================================================
// AC-23  checkSignificance() does not fire on identical distributions
// ============================================================================

TEST(PromptABExperimentFrameworkTest, CheckSignificanceIdenticalNoWinner) {
    PromptABExperimentFramework fw;
    const auto id = fw.create(makeExperiment("t", "v1", "v2", 50, 10));
    feedOutcomes(fw, id, ExperimentVariant::CONTROL,   0.75, 30);
    feedOutcomes(fw, id, ExperimentVariant::TREATMENT, 0.75, 30);
    fw.checkSignificance(id);
    // With identical means and 0 variance the t-stat is 0, p=1.0 → no winner.
    EXPECT_EQ(fw.getStatus(id), ExperimentStatus::RUNNING);
}

// ============================================================================
// AC-24  recordOutcome() auto-promotes winner via WinnerCallback
// ============================================================================

TEST(PromptABExperimentFrameworkTest, AutoPromotionCallbackFired) {
    int callback_count = 0;
    std::string cb_winner_version;
    ExperimentVariant cb_variant = ExperimentVariant::CONTROL;

    PromptABExperimentFramework fw;
    fw.setWinnerCallback([&](const std::string&,
                              ExperimentVariant w,
                              const std::string& version) {
        ++callback_count;
        cb_variant        = w;
        cb_winner_version = version;
    });

    const auto id = fw.create(makeExperiment("t", "v1.0", "v1.1", 50, 10));
    feedOutcomes(fw, id, ExperimentVariant::CONTROL,   0.3, 10);
    feedOutcomes(fw, id, ExperimentVariant::TREATMENT, 0.9, 10);
    // The last recordOutcome() triggers auto-check → callback fires.
    EXPECT_EQ(callback_count, 1);
    EXPECT_EQ(cb_variant,        ExperimentVariant::TREATMENT);
    EXPECT_EQ(cb_winner_version, "v1.1");
}

// ============================================================================
// AC-25  promoteWinner() returns treatment version ID when WINNER_TREATMENT
// ============================================================================

TEST(PromptABExperimentFrameworkTest, PromoteWinnerReturnsTreatmentVersion) {
    PromptABExperimentFramework fw;
    const auto id = fw.create(makeExperiment("t", "vControl", "vTreatment", 50, 10));
    feedOutcomes(fw, id, ExperimentVariant::CONTROL,   0.3, 20);
    feedOutcomes(fw, id, ExperimentVariant::TREATMENT, 0.9, 20);
    fw.checkSignificance(id);
    ASSERT_EQ(fw.getStatus(id), ExperimentStatus::WINNER_TREATMENT);
    EXPECT_EQ(fw.promoteWinner(id), "vTreatment");
}

// ============================================================================
// AC-26  promoteWinner() returns control version ID when WINNER_CONTROL
// ============================================================================

TEST(PromptABExperimentFrameworkTest, PromoteWinnerReturnsControlVersion) {
    PromptABExperimentFramework fw;
    const auto id = fw.create(makeExperiment("t", "vControl", "vTreatment", 50, 10));
    feedOutcomes(fw, id, ExperimentVariant::CONTROL,   0.9, 20);
    feedOutcomes(fw, id, ExperimentVariant::TREATMENT, 0.3, 20);
    fw.checkSignificance(id);
    ASSERT_EQ(fw.getStatus(id), ExperimentStatus::WINNER_CONTROL);
    EXPECT_EQ(fw.promoteWinner(id), "vControl");
}

// ============================================================================
// AC-27  promoteWinner() returns empty when no winner declared
// ============================================================================

TEST(PromptABExperimentFrameworkTest, PromoteWinnerEmptyWhenRunning) {
    PromptABExperimentFramework fw;
    const auto id = fw.create(makeExperiment("t", "v1", "v2", 50, 1000));
    EXPECT_TRUE(fw.promoteWinner(id).empty());
}

// ============================================================================
// AC-28  promoteWinner() sets status to COMPLETED
// ============================================================================

TEST(PromptABExperimentFrameworkTest, PromoteWinnerSetsCompleted) {
    PromptABExperimentFramework fw;
    const auto id = fw.create(makeExperiment("t", "v1", "v2", 50, 10));
    feedOutcomes(fw, id, ExperimentVariant::CONTROL,   0.3, 20);
    feedOutcomes(fw, id, ExperimentVariant::TREATMENT, 0.9, 20);
    fw.checkSignificance(id);
    fw.promoteWinner(id);
    EXPECT_EQ(fw.getStatus(id), ExperimentStatus::COMPLETED);
}

// ============================================================================
// AC-29  getSummary() contains expected fields
// ============================================================================

TEST(PromptABExperimentFrameworkTest, GetSummaryFields) {
    PromptABExperimentFramework fw;
    const auto id = fw.create(makeExperiment("t", "v1", "v2", 50, 10));
    feedOutcomes(fw, id, ExperimentVariant::CONTROL,   0.6, 15);
    feedOutcomes(fw, id, ExperimentVariant::TREATMENT, 0.8, 15);

    const auto summary = fw.getSummary(id);
    ASSERT_TRUE(summary.has_value());
    EXPECT_EQ(summary->control_samples,   15u);
    EXPECT_EQ(summary->treatment_samples, 15u);
    EXPECT_NEAR(summary->mean_control_score,   0.6, 1e-9);
    EXPECT_NEAR(summary->mean_treatment_score, 0.8, 1e-9);
    EXPECT_GT(summary->delta_pct, 0.0);  // treatment > control
    EXPECT_GE(summary->p_value, 0.0);
    EXPECT_LE(summary->p_value, 1.0);

    const auto j = summary->toJson();
    EXPECT_TRUE(j.contains("experiment"));
    EXPECT_TRUE(j.contains("delta_pct"));
    EXPECT_TRUE(j.contains("p_value"));
    EXPECT_TRUE(j.contains("significant"));
    EXPECT_TRUE(j.contains("winner_version_id"));
}

// ============================================================================
// AC-30  PromptExperiment::toJson() / fromJson() round-trip
// ============================================================================

TEST(PromptABExperimentFrameworkTest, ExperimentJsonRoundTrip) {
    PromptExperiment e;
    e.experiment_id        = "test-exp-42";
    e.template_id          = "tpl-99";
    e.control_version_id   = "ctrl-v";
    e.treatment_version_id = "treat-v";
    e.split_pct            = 30;
    e.min_samples          = 100;
    e.confidence_level     = 0.99;
    e.status               = ExperimentStatus::RUNNING;

    const auto j    = e.toJson();
    const auto back = PromptExperiment::fromJson(j);

    EXPECT_EQ(back.experiment_id,        e.experiment_id);
    EXPECT_EQ(back.template_id,          e.template_id);
    EXPECT_EQ(back.control_version_id,   e.control_version_id);
    EXPECT_EQ(back.treatment_version_id, e.treatment_version_id);
    EXPECT_EQ(back.split_pct,            e.split_pct);
    EXPECT_EQ(back.min_samples,          e.min_samples);
    EXPECT_DOUBLE_EQ(back.confidence_level, e.confidence_level);
    EXPECT_EQ(back.status,               e.status);
}
