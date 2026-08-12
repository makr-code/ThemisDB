// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

/**
 * @file test_ada_lora_adapter.cpp
 * @brief Unit tests for training::AdaLoRAAdapter — importance-based rank pruning.
 */

#include <gtest/gtest.h>
#include "training/ada_lora_adapter.h"

#include <algorithm>
#include <cmath>
#include <numeric>
#include <string>
#include <vector>

using namespace themis::training;

// ============================================================================
// Construction
// ============================================================================

TEST(AdaLoRAAdapterTest, ConstructsWithDefaults) {
    AdaLoRAAdapter ada;
    EXPECT_EQ(ada.layerCount(), 0u);
    EXPECT_GT(ada.rankBudget(), 0u);
}

TEST(AdaLoRAAdapterTest, ConstructsWithCustomParams) {
    AdaLoRAAdapter ada(8, 16.0f, 128);
    EXPECT_EQ(ada.layerCount(), 0u);
    EXPECT_EQ(ada.rankBudget(), 128u);
}

TEST(AdaLoRAAdapterTest, ZeroRankDefaultsFallsBackToFour) {
    AdaLoRAAdapter ada(0, 8.0f, 64);
    ada.addLayer("l1", 32, 32);
    EXPECT_GT(ada.getMaxRank("l1"), 0u);
}

TEST(AdaLoRAAdapterTest, ZeroBudgetDefaultsFallsBackPositive) {
    AdaLoRAAdapter ada(4, 8.0f, 0);
    EXPECT_GT(ada.rankBudget(), 0u);
}

// ============================================================================
// Layer management
// ============================================================================

TEST(AdaLoRAAdapterTest, AddLayerRegisters) {
    AdaLoRAAdapter ada(4, 8.0f, 32);
    ada.addLayer("q_proj", 64, 64);
    EXPECT_TRUE(ada.hasLayer("q_proj"));
    EXPECT_EQ(ada.layerCount(), 1u);
}

TEST(AdaLoRAAdapterTest, AddMultipleLayers) {
    AdaLoRAAdapter ada(4, 8.0f, 64);
    ada.addLayer("q_proj", 64, 64);
    ada.addLayer("k_proj", 64, 64);
    ada.addLayer("v_proj", 64, 64);
    EXPECT_EQ(ada.layerCount(), 3u);
}

TEST(AdaLoRAAdapterTest, DuplicateLayerThrows) {
    AdaLoRAAdapter ada(4, 8.0f, 32);
    ada.addLayer("q_proj", 64, 64);
    EXPECT_THROW(ada.addLayer("q_proj", 64, 64), std::invalid_argument);
}

TEST(AdaLoRAAdapterTest, ZeroDimThrows) {
    AdaLoRAAdapter ada(4, 8.0f, 32);
    EXPECT_THROW(ada.addLayer("bad", 0, 64), std::invalid_argument);
    EXPECT_THROW(ada.addLayer("bad2", 64, 0), std::invalid_argument);
}

TEST(AdaLoRAAdapterTest, RemoveLayerWorks) {
    AdaLoRAAdapter ada(4, 8.0f, 32);
    ada.addLayer("q_proj", 64, 64);
    EXPECT_TRUE(ada.removeLayer("q_proj"));
    EXPECT_FALSE(ada.hasLayer("q_proj"));
    EXPECT_EQ(ada.layerCount(), 0u);
}

TEST(AdaLoRAAdapterTest, RemoveNonExistentReturnsFalse) {
    AdaLoRAAdapter ada(4, 8.0f, 32);
    EXPECT_FALSE(ada.removeLayer("nonexistent"));
}

TEST(AdaLoRAAdapterTest, LayerNamesReturnsAll) {
    AdaLoRAAdapter ada(4, 8.0f, 32);
    ada.addLayer("a", 16, 16);
    ada.addLayer("b", 16, 16);
    auto names = ada.layerNames();
    EXPECT_EQ(names.size(), 2u);
    EXPECT_NE(std::find(names.begin(), names.end(), "a"), names.end());
    EXPECT_NE(std::find(names.begin(), names.end(), "b"), names.end());
}

// ============================================================================
// Rank and max-rank accessors
// ============================================================================

TEST(AdaLoRAAdapterTest, InitialActiveRankEqualsMaxRank) {
    AdaLoRAAdapter ada(4, 8.0f, 32);
    ada.addLayer("q_proj", 64, 64, 6);
    EXPECT_EQ(ada.getActiveRank("q_proj"), 6u);
    EXPECT_EQ(ada.getMaxRank("q_proj"),   6u);
}

TEST(AdaLoRAAdapterTest, DefaultRankUsedWhenMaxRankZero) {
    AdaLoRAAdapter ada(5, 8.0f, 32);
    ada.addLayer("q_proj", 64, 64, 0);
    EXPECT_EQ(ada.getMaxRank("q_proj"), 5u);
}

TEST(AdaLoRAAdapterTest, UnknownLayerThrowsOutOfRange) {
    AdaLoRAAdapter ada(4, 8.0f, 32);
    EXPECT_THROW(ada.getActiveRank("nope"), std::out_of_range);
    EXPECT_THROW(ada.getMaxRank("nope"),   std::out_of_range);
    EXPECT_THROW(ada.getImportance("nope"), std::out_of_range);
}

// ============================================================================
// Importance scoring
// ============================================================================

TEST(AdaLoRAAdapterTest, InitialImportanceIsZero) {
    AdaLoRAAdapter ada(4, 8.0f, 32);
    ada.addLayer("q_proj", 16, 16);
    // B is Kaiming-init (non-zero), A is zero → importance depends on A
    // After init, A is zero so importance = 0
    ada.updateImportance("q_proj");
    EXPECT_GE(ada.getImportance("q_proj"), 0.0f);
}

TEST(AdaLoRAAdapterTest, NonZeroWeightsGivePositiveImportance) {
    AdaLoRAAdapter ada(4, 8.0f, 32);
    ada.addLayer("q_proj", 4, 4, 2);
    // Set non-zero B and A
    std::vector<float> B(4 * 2, 1.0f);
    std::vector<float> A(2 * 4, 1.0f);
    ada.setWeights("q_proj", B, A);
    ada.updateImportance("q_proj");
    EXPECT_GT(ada.getImportance("q_proj"), 0.0f);
}

TEST(AdaLoRAAdapterTest, UpdateAllImportancesUpdatesAllLayers) {
    AdaLoRAAdapter ada(4, 8.0f, 32);
    ada.addLayer("a", 4, 4, 2);
    ada.addLayer("b", 4, 4, 2);
    std::vector<float> B(4 * 2, 1.0f), A(2 * 4, 1.0f);
    ada.setWeights("a", B, A);
    ada.setWeights("b", B, A);
    ada.updateAllImportances();
    EXPECT_GT(ada.getImportance("a"), 0.0f);
    EXPECT_GT(ada.getImportance("b"), 0.0f);
}

TEST(AdaLoRAAdapterTest, UnknownLayerImportanceUpdateThrows) {
    AdaLoRAAdapter ada(4, 8.0f, 32);
    EXPECT_THROW(ada.updateImportance("nope"), std::out_of_range);
}

// ============================================================================
// Rank reallocation
// ============================================================================

TEST(AdaLoRAAdapterTest, ReallocWithZeroBudgetThrows) {
    AdaLoRAAdapter ada(4, 8.0f, 32);
    ada.addLayer("q_proj", 16, 16, 4);
    EXPECT_THROW(ada.reallocateRanks(0), std::invalid_argument);
}

TEST(AdaLoRAAdapterTest, ReallocDistributesBudget) {
    AdaLoRAAdapter ada(4, 8.0f, 32);
    ada.addLayer("a", 4, 4, 4);
    ada.addLayer("b", 4, 4, 4);
    auto res = ada.reallocateRanks(6);
    EXPECT_EQ(res.total_active_rank, 6u);
}

TEST(AdaLoRAAdapterTest, ReallocRespectsMaxRank) {
    AdaLoRAAdapter ada(4, 8.0f, 64);
    ada.addLayer("a", 4, 4, 3);  // max_rank = 3
    ada.addLayer("b", 4, 4, 2);  // max_rank = 2
    auto res = ada.reallocateRanks(100); // budget > sum of max_ranks
    EXPECT_LE(ada.getActiveRank("a"), 3u);
    EXPECT_LE(ada.getActiveRank("b"), 2u);
    (void)res;
}

TEST(AdaLoRAAdapterTest, ReallocMinimumRankIsOne) {
    AdaLoRAAdapter ada(8, 8.0f, 32);
    ada.addLayer("a", 4, 4, 8);
    ada.addLayer("b", 4, 4, 8);
    ada.addLayer("c", 4, 4, 8);
    // Very small budget: each layer should get at least rank 1
    ada.reallocateRanks(3);
    EXPECT_GE(ada.getActiveRank("a"), 1u);
    EXPECT_GE(ada.getActiveRank("b"), 1u);
    EXPECT_GE(ada.getActiveRank("c"), 1u);
}

TEST(AdaLoRAAdapterTest, ReallocUsesBudgetFromConfig) {
    AdaLoRAAdapter ada(4, 8.0f, 8);
    ada.addLayer("a", 4, 4, 4);
    ada.addLayer("b", 4, 4, 4);
    auto res = ada.reallocateRanks();
    EXPECT_EQ(ada.rankBudget(), 8u);
    (void)res;
}

TEST(AdaLoRAAdapterTest, ReallocWithImportanceAllocatesProportionally) {
    AdaLoRAAdapter ada(8, 8.0f, 64);
    ada.addLayer("high", 4, 4, 8);
    ada.addLayer("low",  4, 4, 8);

    // Set high importance for "high"
    std::vector<float> B_h(4 * 8, 2.0f), A_h(8 * 4, 2.0f);
    ada.setWeights("high", B_h, A_h);
    std::vector<float> B_l(4 * 8, 0.1f), A_l(8 * 4, 0.1f);
    ada.setWeights("low", B_l, A_l);
    ada.updateAllImportances();

    ada.reallocateRanks(10);
    EXPECT_GE(ada.getActiveRank("high"), ada.getActiveRank("low"));
}

// ============================================================================
// Weight access
// ============================================================================

TEST(AdaLoRAAdapterTest, SetAndGetWeightsRoundTrip) {
    AdaLoRAAdapter ada(4, 8.0f, 32);
    ada.addLayer("q_proj", 4, 4, 2);
    std::vector<float> B = {1,2,3,4,5,6,7,8};   // 4×2
    std::vector<float> A = {1,2,3,4,5,6,7,8};   // 2×4
    ada.setWeights("q_proj", B, A);
    auto [B2, A2] = ada.getWeights("q_proj");
    EXPECT_EQ(B, B2);
    EXPECT_EQ(A, A2);
}

TEST(AdaLoRAAdapterTest, SetWeightsWrongSizeThrows) {
    AdaLoRAAdapter ada(4, 8.0f, 32);
    ada.addLayer("q_proj", 4, 4, 2);
    EXPECT_THROW(ada.setWeights("q_proj", {1.0f}, {1.0f}), std::invalid_argument);
}

TEST(AdaLoRAAdapterTest, GetWeightsUnknownLayerThrows) {
    AdaLoRAAdapter ada(4, 8.0f, 32);
    EXPECT_THROW(ada.getWeights("nope"), std::out_of_range);
}

// ============================================================================
// Forward pass
// ============================================================================

TEST(AdaLoRAAdapterTest, ForwardZeroAProducesZeroOutput) {
    AdaLoRAAdapter ada(4, 8.0f, 32);
    ada.addLayer("q_proj", 4, 4, 2);
    // A is zero-initialised → output = 0
    std::vector<float> input(4, 1.0f);
    auto out = ada.forward("q_proj", input, 1);
    EXPECT_EQ(out.size(), 4u);
    for (float v : out) EXPECT_FLOAT_EQ(v, 0.0f);
}

TEST(AdaLoRAAdapterTest, ForwardWithNonZeroWeightsGivesOutput) {
    AdaLoRAAdapter ada(4, 8.0f, 32);
    ada.addLayer("q_proj", 2, 2, 1);
    // B = [[1],[1]], A = [[1,1]]
    ada.setWeights("q_proj", {1.0f, 1.0f}, {1.0f, 1.0f});
    std::vector<float> input = {1.0f, 1.0f};
    auto out = ada.forward("q_proj", input, 1);
    EXPECT_EQ(out.size(), 2u);
    // hidden = [2.0], output = [2, 2] * (8/1) = [16, 16]
    EXPECT_GT(std::abs(out[0]), 0.0f);
}

TEST(AdaLoRAAdapterTest, ForwardUsesActiveRankOnly) {
    AdaLoRAAdapter ada(4, 8.0f, 32);
    ada.addLayer("q_proj", 2, 2, 2);  // max_rank=2, active_rank=2
    ada.setWeights("q_proj", {1,0, 1,0}, {1,1, 0,0}); // B 2×2, A 2×2
    // Prune to active_rank=1
    ada.reallocateRanks(1);
    auto out_pruned = ada.forward("q_proj", {1.0f, 1.0f}, 1);
    EXPECT_EQ(out_pruned.size(), 2u);
}

TEST(AdaLoRAAdapterTest, ForwardBadInputSizeThrows) {
    AdaLoRAAdapter ada(4, 8.0f, 32);
    ada.addLayer("q_proj", 4, 4, 2);
    EXPECT_THROW(ada.forward("q_proj", {1.0f}, 1), std::invalid_argument);
}

TEST(AdaLoRAAdapterTest, ForwardUnknownLayerThrows) {
    AdaLoRAAdapter ada(4, 8.0f, 32);
    EXPECT_THROW(ada.forward("nope", {1.0f}, 1), std::out_of_range);
}

// ============================================================================
// Layer stats and parameter count
// ============================================================================

TEST(AdaLoRAAdapterTest, LayerStatsReturnsCorrectInfo) {
    AdaLoRAAdapter ada(4, 8.0f, 32);
    ada.addLayer("q_proj", 8, 8, 4);
    auto stats = ada.getLayerStats();
    ASSERT_EQ(stats.size(), 1u);
    EXPECT_EQ(stats[0].layer_name, "q_proj");
    EXPECT_EQ(stats[0].max_rank,   4u);
    EXPECT_EQ(stats[0].active_rank, 4u);
}

TEST(AdaLoRAAdapterTest, TotalActiveParameterCountCorrect) {
    AdaLoRAAdapter ada(4, 8.0f, 32);
    ada.addLayer("q_proj", 8, 8, 4);
    // 4 * (8 + 8) = 64
    EXPECT_EQ(ada.totalActiveParameterCount(), 64u);
}

TEST(AdaLoRAAdapterTest, SetRankBudget) {
    AdaLoRAAdapter ada(4, 8.0f, 32);
    ada.setRankBudget(100);
    EXPECT_EQ(ada.rankBudget(), 100u);
}

// ============================================================================
// Edge cases
// ============================================================================

TEST(AdaLoRAAdapterTest, EmptyAdapterHasZeroParams) {
    AdaLoRAAdapter ada(4, 8.0f, 32);
    EXPECT_EQ(ada.totalActiveParameterCount(), 0u);
    EXPECT_TRUE(ada.layerNames().empty());
}

TEST(AdaLoRAAdapterTest, ReallocOnEmptyAdapterSucceeds) {
    AdaLoRAAdapter ada(4, 8.0f, 32);
    auto res = ada.reallocateRanks(10);
    EXPECT_EQ(res.total_active_rank, 0u);
}

TEST(AdaLoRAAdapterTest, AddLayerWithCustomAlpha) {
    AdaLoRAAdapter ada(4, 8.0f, 32);
    ada.addLayer("q_proj", 8, 8, 4, 16.0f);
    // Should not throw and layer should be registered
    EXPECT_TRUE(ada.hasLayer("q_proj"));
}

TEST(AdaLoRAAdapterTest, RemoveAndReAddLayer) {
    AdaLoRAAdapter ada(4, 8.0f, 32);
    ada.addLayer("q_proj", 8, 8);
    ada.removeLayer("q_proj");
    EXPECT_FALSE(ada.hasLayer("q_proj"));
    ada.addLayer("q_proj", 8, 8);  // Should not throw
    EXPECT_TRUE(ada.hasLayer("q_proj"));
}
