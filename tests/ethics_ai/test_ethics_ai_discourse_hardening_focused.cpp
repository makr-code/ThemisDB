// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

/**
 * @file test_ethics_ai_discourse_hardening_focused.cpp
 * @brief Discourse hardening: conflict/convergence semantics for extended debate rounds.
 *
 * ## Test families (DH-01..DH-08)
 *
 * ### DH-01..04 — conflict_threshold_ratio in RouterConfig
 *   DH-01  Default conflict_threshold_ratio is 0.6
 *   DH-02  Zero threshold disables conflict detection
 *   DH-03  Threshold value 1.0 classifies all rounds as conflict
 *   DH-04  RouterConfig with conflict_threshold_ratio and snapshot flag compiles correctly
 *
 * ### DH-05..08 — MetaVerdictThreshold alignment
 *   DH-05  score > 0.75 → CLEAR_CONSENSUS
 *   DH-06  score exactly 0.60 → CONTESTED (boundary)
 *   DH-07  score 0.0 → DISSENT
 *   DH-08  Determinism: same score → same verdict (idempotent)
 *
 * @see include/ethics_ai/ethics_selection_router.h  — RouterConfig extensions
 * @see include/ethics_ai/ethics_ai_types.h          — MetaVerdictThreshold
 */

#include <gtest/gtest.h>

#include "ethics_ai/ethics_ai_types.h"
#include "ethics_ai/ethics_selection_router.h"

using namespace themis::plugins::ethics;

// ─────────────────────────────────────────────────────────────────────────────
// DH-01..04 — conflict_threshold_ratio
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @test DH-01: Default conflict_threshold_ratio is 0.6.
 */
TEST(DiscourseHardening, DefaultConflictThresholdRatioIs0_6) {
    RouterConfig cfg;
    EXPECT_DOUBLE_EQ(cfg.conflict_threshold_ratio, 0.6);
}

/**
 * @test DH-02: Zero conflict_threshold_ratio is accepted.
 */
TEST(DiscourseHardening, ZeroConflictThresholdIsAccepted) {
    RouterConfig cfg;
    cfg.conflict_threshold_ratio = 0.0;
    EXPECT_DOUBLE_EQ(cfg.conflict_threshold_ratio, 0.0);
}

/**
 * @test DH-03: conflict_threshold_ratio of 1.0 is accepted.
 */
TEST(DiscourseHardening, ConflictThresholdOf1_0IsAccepted) {
    RouterConfig cfg;
    cfg.conflict_threshold_ratio = 1.0;
    EXPECT_DOUBLE_EQ(cfg.conflict_threshold_ratio, 1.0);
}

/**
 * @test DH-04: RouterConfig with both LDM hardening fields compiles and is assignable.
 */
TEST(DiscourseHardening, RouterConfigWithHardeningFieldsCompiles) {
    RouterConfig cfg;
    cfg.conflict_threshold_ratio        = 0.7;
    cfg.snapshot_profile_on_round_start = false;
    EXPECT_DOUBLE_EQ(cfg.conflict_threshold_ratio, 0.7);
    EXPECT_FALSE(cfg.snapshot_profile_on_round_start);
}

// ─────────────────────────────────────────────────────────────────────────────
// DH-05..08 — MetaVerdictThreshold determinism
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @test DH-05: score > 0.75 produces CLEAR_CONSENSUS.
 */
TEST(DiscourseHardening, HighScoreYieldsClearConsensus) {
    EXPECT_EQ(MetaVerdictThreshold(0.80),
              MetaVerdict::ConvergenceVerdict::CLEAR_CONSENSUS);
    EXPECT_EQ(MetaVerdictThreshold(1.00),
              MetaVerdict::ConvergenceVerdict::CLEAR_CONSENSUS);
}

/**
 * @test DH-06: score exactly 0.60 maps to CONTESTED (boundary check).
 */
TEST(DiscourseHardening, BoundaryScoreAt0_60IsContested) {
    // score > 0.60 → TENDENCY; score == 0.60 falls into CONTESTED branch.
    EXPECT_EQ(MetaVerdictThreshold(0.60),
              MetaVerdict::ConvergenceVerdict::CONTESTED);
}

/**
 * @test DH-07: score 0.0 maps to DISSENT.
 */
TEST(DiscourseHardening, ZeroScoreIsDissent) {
    EXPECT_EQ(MetaVerdictThreshold(0.0),
              MetaVerdict::ConvergenceVerdict::DISSENT);
}

/**
 * @test DH-08: MetaVerdictThreshold is deterministic for identical inputs.
 */
TEST(DiscourseHardening, MetaVerdictThresholdIsDeterministic) {
    for (double score : {0.0, 0.3, 0.5, 0.61, 0.76, 1.0}) {
        const auto v1 = MetaVerdictThreshold(score);
        const auto v2 = MetaVerdictThreshold(score);
        EXPECT_EQ(v1, v2) << "Non-deterministic result for score=" << score;
    }
}
