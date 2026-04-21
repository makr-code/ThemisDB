// Copyright 2026 ThemisDB — Licensed under MIT License
//
// FPD — Federated Poisoning Detection unit tests
//
// Test groups:
//   FPD-01  setGradientOutlierFilter() wiring — filter is invoked during aggregation
//   FPD-02  Benign gradients pass the L2-norm filter (no rejection)
//   FPD-03  Poisoned outlier is rejected by L2-norm filter; round still succeeds
//   FPD-04  Two outliers rejected — round aborts when remaining < min_participants
//   FPD-05  filteredGradientsCount() accumulates across multiple rounds
//   FPD-06  getStats() exposes total_gradients_filtered field
//   FPD-07  Custom always-reject filter rejects all → round throws
//   FPD-08  makeL2NormOutlierFilter() with tight threshold rejects outlier
//   FPD-09  makeL2NormOutlierFilter() with wide threshold accepts all
//   FPD-10  Filter disabled (nullptr) — all gradients accepted
//
// All tests: no network, no file I/O, < 100 ms total.

#include <gtest/gtest.h>

#include "distributed_knowledge/lora_federation_coordinator.h"

#include <cmath>
#include <stdexcept>
#include <string>
#include <vector>

using namespace themis::distributed_knowledge;

// ============================================================================
// Helpers
// ============================================================================

namespace {

FederationConfig makeConfig(size_t min_participants = 2,
                            double dp_sensitivity  = 1e-9)
{
    FederationConfig cfg;
    cfg.min_participants = min_participants;
    cfg.dp_epsilon       = 0.5;
    cfg.dp_delta         = 1e-5;
    cfg.dp_sensitivity   = dp_sensitivity;  // near-zero noise for determinism
    return cfg;
}

EncryptedGradient makeGrad(const std::string& shard_id,
                            uint64_t round      = 1,
                            double   layer_val  = 0.01)
{
    EncryptedGradient g;
    g.shard_id     = shard_id;
    g.round        = round;
    g.sample_count = 100;
    g.data         = {{"layer_0", layer_val}, {"layer_1", layer_val * 0.5}};
    return g;
}

/// A gradient with a very large L2 norm (simulates a poisoning injection).
EncryptedGradient makePoisonedGrad(const std::string& shard_id,
                                    uint64_t round = 1,
                                    double   layer_val = 9999.0)
{
    return makeGrad(shard_id, round, layer_val);
}

} // anonymous namespace

// ============================================================================
// FPD-01: filter callable is invoked during auto-aggregation
// ============================================================================

TEST(FPD_PoisoningDetection, FPD_01_FilterIsInvokedDuringAggregation) {
    auto cfg = makeConfig(2);
    LoRAFederationCoordinator coord(cfg);

    size_t invocation_count = 0;
    coord.setGradientOutlierFilter(
        [&invocation_count](const EncryptedGradient&,
                             const std::map<std::string, EncryptedGradient>&) -> bool {
            ++invocation_count;
            return true;  // accept all
        });

    coord.submitGradient(makeGrad("s1"));
    coord.submitGradient(makeGrad("s2"));  // auto-triggers

    // Filter must be called exactly once per gradient in the round (2 gradients)
    EXPECT_EQ(invocation_count, 2u);
}

// ============================================================================
// FPD-02: benign gradients pass the L2-norm filter
// ============================================================================

TEST(FPD_PoisoningDetection, FPD_02_BenignGradientsPassL2Filter) {
    auto cfg = makeConfig(3);
    LoRAFederationCoordinator coord(cfg);
    coord.setGradientOutlierFilter(
        LoRAFederationCoordinator::makeL2NormOutlierFilter(2.5));

    // Three similar gradients — all should be accepted
    coord.submitGradient(makeGrad("s1", 1, 0.01));
    coord.submitGradient(makeGrad("s2", 1, 0.011));
    coord.submitGradient(makeGrad("s3", 1, 0.012));

    EXPECT_TRUE(coord.lastDelta().has_value())
        << "All benign gradients should be accepted and round should complete";
    EXPECT_EQ(coord.filteredGradientsCount(), 0u);
}

// ============================================================================
// FPD-03: outlier is rejected; round still succeeds with remaining participants
// ============================================================================

TEST(FPD_PoisoningDetection, FPD_03_SingleOutlierRejectedRoundSucceeds) {
    // min_participants=2 so 3 submitted - 1 rejected = 2 → still above threshold
    auto cfg = makeConfig(2);
    LoRAFederationCoordinator coord(cfg);
    coord.setGradientOutlierFilter(
        LoRAFederationCoordinator::makeL2NormOutlierFilter(2.5));

    // Two normal gradients and one poisoned outlier
    coord.submitGradient(makeGrad("s1", 1, 0.01));
    coord.submitGradient(makeGrad("s2", 1, 0.011));
    auto poison = makePoisonedGrad("s-evil", 1, 9999.0);
    coord.submitGradient(poison);
    // Auto-trigger fires when 3rd gradient arrives (>= min_participants=2)

    ASSERT_TRUE(coord.lastDelta().has_value())
        << "Round should complete using the 2 benign gradients";
    EXPECT_GE(coord.filteredGradientsCount(), 1u)
        << "Outlier gradient must be counted as filtered";
}

// ============================================================================
// FPD-04: two outliers rejected → fewer than min_participants remaining → throws
// ============================================================================

TEST(FPD_PoisoningDetection, FPD_04_TwoOutliersRejectedBelowThresholdThrows) {
    // min_participants=3; only 2 legit submitted (+ 1 poisoned) → after filter
    // only 2 remain → below min_participants=3 → error
    FederationConfig cfg = makeConfig(3);
    LoRAFederationCoordinator coord(cfg);
    coord.setGradientOutlierFilter(
        LoRAFederationCoordinator::makeL2NormOutlierFilter(0.5));  // tight threshold

    // Submit two outlier-range gradients and one legitimate gradient.
    // With a tight z=0.5 threshold and large variation the outliers are rejected.
    coord.submitGradient(makeGrad("s1", 1, 0.01));
    coord.submitGradient(makePoisonedGrad("s-evil-1", 1, 500.0));
    coord.submitGradient(makePoisonedGrad("s-evil-2", 1, 600.0));

    // Auto-trigger fires on third gradient — expect throw with diagnostic message
    try {
        coord.triggerAggregation();
        FAIL() << "Expected std::runtime_error when participants drop below min_participants";
    } catch (const std::runtime_error& e) {
        const std::string msg(e.what());
        EXPECT_TRUE(msg.find("insufficient") != std::string::npos ||
                    msg.find("participants") != std::string::npos)
            << "Exception message must contain diagnostic info; got: " << msg;
    }
}

// ============================================================================
// FPD-05: filteredGradientsCount() accumulates across rounds
// ============================================================================

TEST(FPD_PoisoningDetection, FPD_05_FilteredCountAccumulatesAcrossRounds) {
    auto cfg = makeConfig(2);
    LoRAFederationCoordinator coord(cfg);

    // Always-reject filter for the outlier shard
    coord.setGradientOutlierFilter(
        [](const EncryptedGradient& g,
           const std::map<std::string, EncryptedGradient>&) -> bool {
            return g.shard_id != "poison";
        });

    // Round 1: 2 good + 1 poisoned → 1 filtered, round succeeds
    coord.submitGradient(makeGrad("s1", 1));
    coord.submitGradient(makeGrad("s2", 1));
    coord.submitGradient(makeGrad("poison", 1));
    coord.triggerAggregation();

    // Round 2: 2 good + 1 poisoned → 1 more filtered
    coord.submitGradient(makeGrad("s1", 2));
    coord.submitGradient(makeGrad("s2", 2));
    coord.submitGradient(makeGrad("poison", 2));
    coord.triggerAggregation();

    EXPECT_GE(coord.filteredGradientsCount(), 2u)
        << "Filtered count must accumulate across rounds";
}

// ============================================================================
// FPD-06: getStats() exposes total_gradients_filtered
// ============================================================================

TEST(FPD_PoisoningDetection, FPD_06_GetStatsExposesTotalGradientsFiltered) {
    auto cfg = makeConfig(2);
    LoRAFederationCoordinator coord(cfg);

    coord.setGradientOutlierFilter(
        [](const EncryptedGradient& g,
           const std::map<std::string, EncryptedGradient>&) -> bool {
            return g.shard_id != "bad";
        });

    coord.submitGradient(makeGrad("s1", 1));
    coord.submitGradient(makeGrad("bad", 1));
    coord.submitGradient(makeGrad("s2", 1));
    coord.triggerAggregation();

    const auto stats = coord.getStats();
    ASSERT_TRUE(stats.contains("total_gradients_filtered"));
    EXPECT_GE(stats["total_gradients_filtered"].get<uint64_t>(), 1u);
}

// ============================================================================
// FPD-07: custom always-reject filter → round throws
// ============================================================================

TEST(FPD_PoisoningDetection, FPD_07_AlwaysRejectFilterThrows) {
    auto cfg = makeConfig(2);
    LoRAFederationCoordinator coord(cfg);

    // Filter that rejects every gradient
    coord.setGradientOutlierFilter(
        [](const EncryptedGradient&,
           const std::map<std::string, EncryptedGradient>&) -> bool {
            return false;
        });

    coord.submitGradient(makeGrad("s1", 1));
    coord.submitGradient(makeGrad("s2", 1));

    EXPECT_THROW(coord.triggerAggregation(), std::runtime_error);
}

// ============================================================================
// FPD-08: makeL2NormOutlierFilter() with tight threshold rejects outlier
// ============================================================================

TEST(FPD_PoisoningDetection, FPD_08_L2FilterTightThresholdRejectsOutlier) {
    auto cfg = makeConfig(2);
    LoRAFederationCoordinator coord(cfg);
    // Very tight threshold: anything > 0.1 stddev from mean gets rejected
    coord.setGradientOutlierFilter(
        LoRAFederationCoordinator::makeL2NormOutlierFilter(0.1));

    // Two similar small gradients + one huge outlier
    coord.submitGradient(makeGrad("s1", 1, 0.01));
    coord.submitGradient(makeGrad("s2", 1, 0.011));
    coord.submitGradient(makePoisonedGrad("poison", 1, 100.0));

    coord.triggerAggregation();

    EXPECT_GE(coord.filteredGradientsCount(), 1u)
        << "Outlier must be rejected with tight z_threshold=0.1";
}

// ============================================================================
// FPD-09: makeL2NormOutlierFilter() with wide threshold accepts all
// ============================================================================

TEST(FPD_PoisoningDetection, FPD_09_L2FilterWideThresholdAcceptsAll) {
    auto cfg = makeConfig(2);
    LoRAFederationCoordinator coord(cfg);
    // Extremely wide threshold — nothing should be rejected
    coord.setGradientOutlierFilter(
        LoRAFederationCoordinator::makeL2NormOutlierFilter(1000.0));

    coord.submitGradient(makeGrad("s1", 1, 0.01));
    coord.submitGradient(makePoisonedGrad("s2", 1, 500.0));
    coord.triggerAggregation();

    EXPECT_EQ(coord.filteredGradientsCount(), 0u)
        << "No gradient should be rejected with a very wide threshold";
}

// ============================================================================
// FPD-10: filter not set (nullptr / default) — all gradients accepted
// ============================================================================

TEST(FPD_PoisoningDetection, FPD_10_NoFilterAcceptsAll) {
    auto cfg = makeConfig(2);
    LoRAFederationCoordinator coord(cfg);
    // Do NOT call setGradientOutlierFilter — default is no filter

    coord.submitGradient(makeGrad("s1", 1));
    coord.submitGradient(makePoisonedGrad("s2", 1, 9999.0));

    EXPECT_TRUE(coord.lastDelta().has_value())
        << "Without a filter all gradients must be accepted";
    EXPECT_EQ(coord.filteredGradientsCount(), 0u);
}
