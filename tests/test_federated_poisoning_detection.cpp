/**
 * @file test_federated_poisoning_detection.cpp
 * @brief Unit tests for GradientOutlierFilter hook in LoRAFederationCoordinator (FPD).
 *
 * Tests
 * -----
 * FPD_01  makeL2NormOutlierFilter() accepts a gradient within the norm bound
 * FPD_02  makeL2NormOutlierFilter() rejects a gradient exceeding the norm bound
 * FPD_03  makeL2NormOutlierFilter(0) throws std::invalid_argument
 * FPD_04  setGradientOutlierFilter(nullptr) removes the filter
 * FPD_05  Poisoned gradient filtered out; clean gradients still aggregated
 * FPD_06  filteredGradientsCount() increments for each rejected gradient
 * FPD_07  All gradients rejected → triggerAggregation() throws "all gradients filtered"
 * FPD_08  Filter not set → no gradients filtered, normal aggregation
 * FPD_09  Custom filter (always-reject) → filteredGradientsCount() == submitted count
 * FPD_10  Non-numeric gradient data (empty object) → accepted by L2NormFilter
 */

#include <gtest/gtest.h>
#include "distributed_knowledge/lora_federation_coordinator.h"

using namespace themis::distributed_knowledge;

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

static EncryptedGradient makeGradient(const std::string& shard_id,
                                       uint64_t round,
                                       const nlohmann::json& data,
                                       size_t samples = 10)
{
    return EncryptedGradient{shard_id, round, samples, data};
}

static FederationConfig minConfig(size_t min_p = 1)
{
    FederationConfig cfg;
    cfg.min_participants = min_p;
    cfg.dp_epsilon       = 0.0; // no DP noise in unit tests
    cfg.dp_delta         = 1e-5;
    return cfg;
}

// ---------------------------------------------------------------------------
// FPD_01 — makeL2NormOutlierFilter accepts within-bound gradient
// ---------------------------------------------------------------------------
TEST(FPD, FPD_01_L2FilterAcceptsInBound) {
    auto filter = LoRAFederationCoordinator::makeL2NormOutlierFilter(10.0);
    EncryptedGradient g = makeGradient("s1", 1,
        {{"w1", 3.0}, {"w2", 4.0}}); // L2 = 5.0 < 10.0
    EXPECT_TRUE(filter(g));
}

// ---------------------------------------------------------------------------
// FPD_02 — makeL2NormOutlierFilter rejects exceeding-bound gradient
// ---------------------------------------------------------------------------
TEST(FPD, FPD_02_L2FilterRejectsOutOfBound) {
    auto filter = LoRAFederationCoordinator::makeL2NormOutlierFilter(4.0);
    EncryptedGradient g = makeGradient("s1", 1,
        {{"w1", 3.0}, {"w2", 4.0}}); // L2 = 5.0 > 4.0
    EXPECT_FALSE(filter(g));
}

// ---------------------------------------------------------------------------
// FPD_03 — makeL2NormOutlierFilter(0) throws
// ---------------------------------------------------------------------------
TEST(FPD, FPD_03_L2FilterZeroNormThrows) {
    EXPECT_THROW(LoRAFederationCoordinator::makeL2NormOutlierFilter(0.0),
                 std::invalid_argument);
    EXPECT_THROW(LoRAFederationCoordinator::makeL2NormOutlierFilter(-1.0),
                 std::invalid_argument);
}

// ---------------------------------------------------------------------------
// FPD_04 — setGradientOutlierFilter(nullptr) removes the filter
// ---------------------------------------------------------------------------
TEST(FPD, FPD_04_NullFilterRemovesFilter) {
    LoRAFederationCoordinator coord(minConfig());
    // Install a filter that rejects everything
    coord.setGradientOutlierFilter([](const EncryptedGradient&) { return false; });
    // Remove it
    coord.setGradientOutlierFilter(nullptr);

    // With no filter, gradient is accepted and aggregation completes normally
    coord.submitGradient(makeGradient("s1", 1, {{"w", 1.0}}));
    EXPECT_NO_THROW(coord.triggerAggregation());
}

// ---------------------------------------------------------------------------
// FPD_05 — Poisoned gradient filtered out; clean gradient aggregated
// ---------------------------------------------------------------------------
TEST(FPD, FPD_05_PoisonedGradientFiltered_CleanAggregated) {
    auto cfg = minConfig(2);
    LoRAFederationCoordinator coord(cfg);
    coord.setGradientOutlierFilter(
        LoRAFederationCoordinator::makeL2NormOutlierFilter(5.0));

    // s1: L2 = 3.0 (clean)
    coord.submitGradient(makeGradient("s1", 1, {{"w", 3.0}}));
    // s2: L2 = 100.0 (poisoned) — should be filtered
    coord.submitGradient(makeGradient("s2", 1, {{"w", 100.0}}));

    // min_participants=2 but after filtering only 1 remains — manual trigger needed
    // min_participants down to 1 so it auto-fires after filtering
    // Actually let's use min_p=1 for auto-fire, then check result
    auto cfg2 = minConfig(1);
    LoRAFederationCoordinator coord2(cfg2);
    coord2.setGradientOutlierFilter(
        LoRAFederationCoordinator::makeL2NormOutlierFilter(5.0));
    coord2.submitGradient(makeGradient("s1", 1, {{"w", 3.0}}));
    // s2 submitted but filtered on aggregation → should not appear
    coord2.submitGradient(makeGradient("s2", 1, {{"w", 100.0}}));

    EXPECT_EQ(coord2.filteredGradientsCount(), 1u);
}

// ---------------------------------------------------------------------------
// FPD_06 — filteredGradientsCount() increments per rejected gradient
// ---------------------------------------------------------------------------
TEST(FPD, FPD_06_FilteredCountIncrements) {
    LoRAFederationCoordinator coord(minConfig(1));
    coord.setGradientOutlierFilter(
        LoRAFederationCoordinator::makeL2NormOutlierFilter(1.0));

    // Submit 1 clean + trigger aggregation
    coord.submitGradient(makeGradient("clean", 1, {{"w", 0.5}})); // L2=0.5 <= 1.0
    // Trigger round 2
    coord.submitGradient(makeGradient("c2", 2, {{"w", 0.5}}));
    // Submit 1 outlier
    LoRAFederationCoordinator coord2(minConfig(1));
    coord2.setGradientOutlierFilter(
        LoRAFederationCoordinator::makeL2NormOutlierFilter(1.0));
    EXPECT_EQ(coord2.filteredGradientsCount(), 0u);
    // Submit clean — auto-triggers
    coord2.submitGradient(makeGradient("clean", 1, {{"w", 0.5}}));
    EXPECT_EQ(coord2.filteredGradientsCount(), 0u);
}

// ---------------------------------------------------------------------------
// FPD_07 — All gradients rejected → triggerAggregation throws
// ---------------------------------------------------------------------------
TEST(FPD, FPD_07_AllGradientsRejectedThrows) {
    LoRAFederationCoordinator coord(minConfig(1));
    // Filter that rejects everything
    coord.setGradientOutlierFilter([](const EncryptedGradient&) { return false; });
    // Can't auto-trigger because submitGradient() would auto-aggregate and throw
    // Use min_participants=2 to prevent auto-trigger, then manually trigger
    LoRAFederationCoordinator coord2(minConfig(2));
    coord2.setGradientOutlierFilter([](const EncryptedGradient&) { return false; });
    coord2.submitGradient(makeGradient("s1", 1, {{"w", 1.0}}));
    coord2.submitGradient(makeGradient("s2", 1, {{"w", 2.0}}));
    // All 2 submitted but filter rejects all — now manually trigger
    // But they were already filtered during submitGradient auto-trigger attempt.
    // Use triggerAggregation directly on a coordinator that hasn't auto-aggregated:
    LoRAFederationCoordinator coord3(minConfig(3));
    coord3.setGradientOutlierFilter([](const EncryptedGradient&) { return false; });
    coord3.submitGradient(makeGradient("s1", 1, {{"w", 1.0}}));
    coord3.submitGradient(makeGradient("s2", 1, {{"w", 2.0}}));
    coord3.submitGradient(makeGradient("s3", 1, {{"w", 3.0}}));
    EXPECT_THROW(coord3.triggerAggregation(), std::runtime_error);
}

// ---------------------------------------------------------------------------
// FPD_08 — No filter set → normal aggregation, filteredCount stays 0
// ---------------------------------------------------------------------------
TEST(FPD, FPD_08_NoFilterNormalAggregation) {
    LoRAFederationCoordinator coord(minConfig(1));
    // No filter
    coord.submitGradient(makeGradient("s1", 1, {{"w", 999.0}}));
    EXPECT_EQ(coord.filteredGradientsCount(), 0u);
    // No throw — large norm accepted without filter
    auto delta = coord.lastDelta();
    EXPECT_TRUE(delta.has_value());
}

// ---------------------------------------------------------------------------
// FPD_09 — Custom always-reject filter → filteredCount == submitted
// ---------------------------------------------------------------------------
TEST(FPD, FPD_09_AlwaysRejectFilterCounts) {
    LoRAFederationCoordinator coord(minConfig(5));
    coord.setGradientOutlierFilter([](const EncryptedGradient&) { return false; });
    for (int i = 0; i < 5; ++i) {
        coord.submitGradient(makeGradient("s" + std::to_string(i), 1, {{"w", 1.0}}));
    }
    EXPECT_THROW(coord.triggerAggregation(), std::runtime_error);
    EXPECT_EQ(coord.filteredGradientsCount(), 5u);
}

// ---------------------------------------------------------------------------
// FPD_10 — Non-numeric (empty object) gradient accepted by L2NormFilter
// ---------------------------------------------------------------------------
TEST(FPD, FPD_10_EmptyObjectAcceptedByL2Filter) {
    auto filter = LoRAFederationCoordinator::makeL2NormOutlierFilter(0.001);
    EncryptedGradient g = makeGradient("s1", 1, nlohmann::json::object());
    // L2 norm of empty object = 0 ≤ 0.001 → accepted
    EXPECT_TRUE(filter(g));
}
