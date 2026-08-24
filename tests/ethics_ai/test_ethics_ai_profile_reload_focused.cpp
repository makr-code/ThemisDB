// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

/**
 * @file test_ethics_ai_profile_reload_focused.cpp
 * @brief Profile-reload + selection-router snapshot-guard regression tests.
 *
 * ## Test families (PR-01..PR-08)
 *
 * ### PR-01..03 — snapshot_profile_on_round_start flag
 *   PR-01  Default value of snapshot_profile_on_round_start is true
 *   PR-02  Flag can be set to false (legacy mode)
 *   PR-03  Flag is preserved through copy construction of RouterConfig
 *
 * ### PR-04..08 — Snapshot guard semantics (contract-level)
 *   PR-04  With snapshot=true, a second RouterConfig copy is independent of the first
 *   PR-05  RouterConfig copy carries the same conflict_threshold_ratio
 *   PR-06  Two configs with different thresholds do not interfere
 *   PR-07  RouterConfig default-constructed with snapshot=true is safe to use
 *   PR-08  Changing school_bias on a copy does not affect the original
 *
 * @see include/ethics_ai/ethics_selection_router.h  — RouterConfig.snapshot_profile_on_round_start
 */

#include <gtest/gtest.h>

#include "ethics_ai/ethics_selection_router.h"

using namespace themis::plugins::ethics;

// ─────────────────────────────────────────────────────────────────────────────
// PR-01..03 — snapshot_profile_on_round_start flag
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @test PR-01: Default value of snapshot_profile_on_round_start is true.
 */
TEST(ProfileReloadGuard, DefaultSnapshotFlagIsTrue) {
    RouterConfig cfg;
    EXPECT_TRUE(cfg.snapshot_profile_on_round_start);
}

/**
 * @test PR-02: Flag can be set to false (legacy/test mode).
 */
TEST(ProfileReloadGuard, FlagCanBeSetToFalse) {
    RouterConfig cfg;
    cfg.snapshot_profile_on_round_start = false;
    EXPECT_FALSE(cfg.snapshot_profile_on_round_start);
}

/**
 * @test PR-03: Flag is preserved through copy construction.
 */
TEST(ProfileReloadGuard, FlagPreservedOnCopy) {
    RouterConfig original;
    original.snapshot_profile_on_round_start = false;

    RouterConfig copy = original;
    EXPECT_FALSE(copy.snapshot_profile_on_round_start);

    RouterConfig copy2;
    copy2 = original;
    EXPECT_FALSE(copy2.snapshot_profile_on_round_start);
}

// ─────────────────────────────────────────────────────────────────────────────
// PR-04..08 — Snapshot guard semantics
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @test PR-04: A copied RouterConfig is independent of the original.
 */
TEST(ProfileReloadGuard, CopiedConfigIsIndependent) {
    RouterConfig original;
    original.top_n = 5;
    RouterConfig snapshot = original;  // simulates snapshot at round start

    // Modify the original after snapshot (simulates mid-round reload).
    original.top_n = 10;

    // Snapshot must retain the pre-reload value.
    EXPECT_EQ(snapshot.top_n, 5u);
    EXPECT_EQ(original.top_n, 10u);
}

/**
 * @test PR-05: Copied config carries the same conflict_threshold_ratio.
 */
TEST(ProfileReloadGuard, SnapshotCopiesConflictThreshold) {
    RouterConfig original;
    original.conflict_threshold_ratio = 0.75;
    RouterConfig snapshot = original;
    EXPECT_DOUBLE_EQ(snapshot.conflict_threshold_ratio, 0.75);
}

/**
 * @test PR-06: Two configs with different thresholds do not interfere.
 */
TEST(ProfileReloadGuard, IndependentConfigsDoNotInterfere) {
    RouterConfig cfg_a;
    cfg_a.conflict_threshold_ratio = 0.5;
    RouterConfig cfg_b;
    cfg_b.conflict_threshold_ratio = 0.8;

    EXPECT_DOUBLE_EQ(cfg_a.conflict_threshold_ratio, 0.5);
    EXPECT_DOUBLE_EQ(cfg_b.conflict_threshold_ratio, 0.8);
}

/**
 * @test PR-07: Default-constructed RouterConfig with snapshot=true is usable.
 */
TEST(ProfileReloadGuard, DefaultConstructedConfigIsUsable) {
    RouterConfig cfg;
    EXPECT_TRUE(cfg.snapshot_profile_on_round_start);
    EXPECT_GT(cfg.top_n, 0u);
    EXPECT_DOUBLE_EQ(cfg.conflict_threshold_ratio, 0.6);
}

/**
 * @test PR-08: Modifying school_bias on a copy does not affect the original.
 */
TEST(ProfileReloadGuard, SchoolBiasCopyIsDeep) {
    RouterConfig original;
    original.school_bias["kant"] = 1.5;
    RouterConfig snapshot = original;

    // Simulate reload: add a new school to the original.
    original.school_bias["utilitarianism"] = 0.8;

    // Snapshot must not see the newly added school.
    EXPECT_EQ(snapshot.school_bias.count("utilitarianism"), 0u);
    EXPECT_EQ(original.school_bias.count("utilitarianism"), 1u);
}
