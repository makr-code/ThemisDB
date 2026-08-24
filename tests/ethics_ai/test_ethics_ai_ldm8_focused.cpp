// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

/**
 * @file test_ethics_ai_ldm8_focused.cpp
 * @brief LDM-8: AdaLoRA adapter for score-bias correction (LDM8-01..LDM8-04).
 *
 * ## Test families
 *
 *   LDM8-01  IdentityAdaLoRABiasCorrector: hasAdapter() == false for any school
 *   LDM8-02  IdentityAdaLoRABiasCorrector: applyBiasCorrection() returns raw_score unchanged
 *   LDM8-03  ScalarAdaLoRABiasCorrector: hasAdapter() reflects registered schools
 *   LDM8-04  ScalarAdaLoRABiasCorrector: corrected score is clamped to [0.0, 1.0]
 *
 * @see include/ethics_ai/ethics_ai_types.h — LDM-8 types
 */

#include <gtest/gtest.h>

#include "ethics_ai/ethics_ai_types.h"

#include <memory>

using namespace themis::plugins::ethics;

// ─────────────────────────────────────────────────────────────────────────────
// LDM8-01..02 — IdentityAdaLoRABiasCorrector
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @test LDM8-01: IdentityAdaLoRABiasCorrector has no adapters for any school.
 */
TEST(AdaLoRABiasCorrector, IdentityHasNoAdapters) {
    IdentityAdaLoRABiasCorrector corrector;
    EXPECT_FALSE(corrector.hasAdapter("kant"));
    EXPECT_FALSE(corrector.hasAdapter("maori_ethics"));
    EXPECT_FALSE(corrector.hasAdapter(""));
}

/**
 * @test LDM8-02: IdentityAdaLoRABiasCorrector returns raw_score unchanged.
 */
TEST(AdaLoRABiasCorrector, IdentityReturnsRawScoreUnchanged) {
    IdentityAdaLoRABiasCorrector corrector;
    for (double score : {0.0, 0.25, 0.5, 0.75, 1.0}) {
        EXPECT_DOUBLE_EQ(corrector.applyBiasCorrection("any_school", score), score)
            << "Identity corrector must not modify score=" << score;
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// LDM8-03..04 — ScalarAdaLoRABiasCorrector
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @test LDM8-03: hasAdapter() returns true only for registered schools.
 */
TEST(AdaLoRABiasCorrector, ScalarHasAdapterOnlyForRegistered) {
    ScalarAdaLoRABiasCorrector corrector;
    EXPECT_FALSE(corrector.hasAdapter("kant"));

    corrector.registerAdapter("kant", 1.2);
    EXPECT_TRUE(corrector.hasAdapter("kant"));
    EXPECT_FALSE(corrector.hasAdapter("utilitarianism"));
}

/**
 * @test LDM8-04: Corrected score is clamped to [0.0, 1.0] and deterministic.
 */
TEST(AdaLoRABiasCorrector, ScalarClampsResultToUnitInterval) {
    ScalarAdaLoRABiasCorrector corrector;
    // Factor > 1: boosts scores; 1.0 * 1.5 = 1.5 → clamped to 1.0
    corrector.registerAdapter("maori_ethics", 1.5);
    EXPECT_DOUBLE_EQ(corrector.applyBiasCorrection("maori_ethics", 1.0), 1.0);
    EXPECT_DOUBLE_EQ(corrector.applyBiasCorrection("maori_ethics", 0.6), 0.9);

    // Factor < 1: reduces scores; 0.5 * 0.4 = 0.2
    corrector.registerAdapter("latin_liberation_theology", 0.4);
    EXPECT_DOUBLE_EQ(corrector.applyBiasCorrection("latin_liberation_theology", 0.5), 0.2);

    // Unknown school: identity (returns raw_score).
    EXPECT_DOUBLE_EQ(corrector.applyBiasCorrection("unknown_school", 0.7), 0.7);

    // Determinism: same inputs → same output.
    const double r1 = corrector.applyBiasCorrection("maori_ethics", 0.3);
    const double r2 = corrector.applyBiasCorrection("maori_ethics", 0.3);
    EXPECT_DOUBLE_EQ(r1, r2);
}

/**
 * @test LDM8 — Polymorphic dispatch through IAdaLoRABiasCorrector base pointer.
 */
TEST(AdaLoRABiasCorrector, PolymorphicDispatchWorksCorrectly) {
    auto scalar = std::make_unique<ScalarAdaLoRABiasCorrector>();
    scalar->registerAdapter("maori_ethics", 1.1);

    std::unique_ptr<IAdaLoRABiasCorrector> corrector = std::move(scalar);

    EXPECT_TRUE(corrector->hasAdapter("maori_ethics"));
    EXPECT_DOUBLE_EQ(corrector->applyBiasCorrection("maori_ethics", 0.5),
                     0.55);  // 0.5 * 1.1 = 0.55
}
