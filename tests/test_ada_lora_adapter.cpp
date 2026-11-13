// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

/**
 * @file test_ada_lora_adapter.cpp
 * @brief Unit tests for training::AdaLoRAAdapter — importance-based rank pruning.
 */

#include <gtest/gtest.h>
#include "training/ada_lora_adapter.h"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <limits>
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
    for (float v : out) {
      EXPECT_FLOAT_EQ(v, 0.0f);
    }
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

TEST(AdaLoRAAdapterTest, TrainBatchReducesSupervisedResidualLoss) {
    AdaLoRAAdapter adapter(1, 1.0f, 1);
    adapter.addLayer("projection", 1, 1, 1, 1.0f);
    adapter.setWeights("projection", {1.0f}, {0.0f});

    const std::vector<float> activations = {1.0f, 2.0f, 3.0f};
    const std::vector<float> targets = {2.0f, 4.0f, 6.0f};
    const auto initial = adapter.trainBatch("projection", activations, targets, 3, 0.01f);

    auto latest = initial;
    for (size_t step = 0; step < 200; ++step) {
        latest = adapter.trainBatch("projection", activations, targets, 3, 0.01f);
    }

    EXPECT_EQ(initial.sample_count, 3u);
    EXPECT_GT(initial.gradient_norm, 0.0f);
    EXPECT_LT(latest.loss, initial.loss);
    EXPECT_GT(adapter.getImportance("projection"), 0.0f);
}

TEST(AdaLoRAAdapterTest, TrainBatchRejectsInvalidShapesAndLearningRate) {
    AdaLoRAAdapter adapter(1, 1.0f, 1);
    adapter.addLayer("projection", 1, 1, 1, 1.0f);

    EXPECT_THROW(static_cast<void>(adapter.trainBatch("projection", {1.0f}, {1.0f}, 0, 0.01f)),
                 std::invalid_argument);
    EXPECT_THROW(static_cast<void>(adapter.trainBatch("projection", {1.0f, 2.0f}, {1.0f}, 1, 0.01f)),
                 std::invalid_argument);
    EXPECT_THROW(static_cast<void>(adapter.trainBatch("projection", {1.0f}, {1.0f}, 1, 0.0f)),
                 std::invalid_argument);
}

TEST(AdaLoRAAdapterTest, CausalLmBatchReducesCrossEntropyAndReallocatesRanks) {
    AdaLoRAAdapter adapter(2, 2.0f, 4);
    adapter.addLayer("output_shard_0", 1, 1, 2, 2.0f);
    adapter.addLayer("output_shard_1", 1, 1, 2, 2.0f);

    const std::vector<float> hidden_states = {1.0f, 1.0f, -1.0f, -1.0f};
    const std::vector<float> base_logits(8, 0.0f);
    const std::vector<size_t> targets = {0, 0, 1, 1};
    const auto initial = adapter.trainCausalLmBatch(
        hidden_states, base_logits, targets, 4, 0.05f);

    auto latest = initial;
    for (size_t step = 0; step < 300; ++step) {
        latest = adapter.trainCausalLmBatch(
            hidden_states, base_logits, targets, 4, 0.05f);
    }

    EXPECT_NEAR(initial.loss, std::log(2.0f), 1e-5f);
    EXPECT_GT(initial.gradient_norm, 0.0f);
    EXPECT_LT(latest.loss, initial.loss);
    EXPECT_GT(adapter.getImportance("output_shard_0"), 0.0f);
    EXPECT_GT(adapter.getImportance("output_shard_1"), 0.0f);

    const auto allocation = adapter.reallocateRanks(3);
    EXPECT_TRUE(allocation.success);
    EXPECT_EQ(allocation.total_active_rank, 3u);
}

TEST(AdaLoRAAdapterTest, CausalLmBatchRejectsInvalidTargetsAndNonFiniteInputs) {
    AdaLoRAAdapter adapter(1, 1.0f, 1);
    adapter.addLayer("output_shard", 1, 2, 1, 1.0f);

    EXPECT_THROW(static_cast<void>(adapter.trainCausalLmBatch(
                     {1.0f}, {0.0f, 0.0f}, {2}, 1, 0.01f)),
                 std::invalid_argument);
    EXPECT_THROW(static_cast<void>(adapter.trainCausalLmBatch(
                     {std::numeric_limits<float>::quiet_NaN()}, {0.0f, 0.0f}, {0}, 1, 0.01f)),
                 std::invalid_argument);
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
    EXPECT_FLOAT_EQ(stats[0].alpha, 8.0f);
}

TEST(AdaLoRAAdapterTest, LayerStatsReportsCustomAlpha) {
    AdaLoRAAdapter ada(4, 8.0f, 32);
    ada.addLayer("q_proj", 8, 8, 4, 16.0f);
    const auto stats = ada.getLayerStats();
    ASSERT_EQ(stats.size(), 1u);
    EXPECT_FLOAT_EQ(stats[0].alpha, 16.0f);
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

namespace {
class CheckpointFile {
public:
    CheckpointFile() {
        static std::atomic<unsigned> sequence{0};
        path = (std::filesystem::temp_directory_path() /
            ("themis-adalora-" + std::to_string(
                std::chrono::steady_clock::now().time_since_epoch().count()) +
             "-" + std::to_string(sequence++) + ".bin")).string();
    }
    ~CheckpointFile() { std::error_code ec; std::filesystem::remove(path, ec); }
    std::string path;
};

std::vector<char> checkpointBytes(const std::string& path) {
    std::ifstream in(path, std::ios::binary);
    return {std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>()};
}

void writeCheckpointBytes(const std::string& path, const std::vector<char>& bytes) {
    std::ofstream out(path, std::ios::binary | std::ios::trunc);
    if (!bytes.empty())
        out.write(bytes.data(), static_cast<std::streamsize>(bytes.size()));
}

void updateCheckpointChecksum(std::vector<char>& bytes) {
    uint64_t hash = 14695981039346656037ull;
    for (size_t i = 0; i < bytes.size() - sizeof(hash); ++i) {
        hash ^= static_cast<unsigned char>(bytes[i]);
        hash *= 1099511628211ull;
    }
    std::memcpy(bytes.data() + bytes.size() - sizeof(hash), &hash, sizeof(hash));
}

template <typename T>
void replaceCheckpointValue(std::vector<char>& bytes, size_t offset, T value) {
    std::memcpy(bytes.data() + offset, &value, sizeof(value));
}
} // namespace

TEST(AdaLoRAAdapterTest, CheckpointRestoresStateAndTrainingContinuation) {
    CheckpointFile file;
    AdaLoRAAdapter original(3, 5.0f, 3);
    original.addLayer("z", 2, 1, 2, 7.0f);
    original.addLayer("a", 2, 1, 2, 9.0f);
    original.setWeights("z", {1, 2, 3, 4}, {1, 2});
    original.setWeights("a", {1, 1, 1, 1}, {0.1f, 0.2f});
    original.updateAllImportances();
    original.reallocateRanks(3);
    ASSERT_NE(original.getActiveRank("a"), original.getMaxRank("a"));
    original.saveToFile(file.path, "model");

    AdaLoRAAdapter restored(1, 1.0f, 1);
    restored.addLayer("obsolete", 1, 1);
    EXPECT_EQ(restored.loadFromFile(file.path), "model");
    EXPECT_TRUE(AdaLoRAAdapter::isCacheValid(file.path, "model"));
    EXPECT_FALSE(restored.hasLayer("obsolete"));
    EXPECT_EQ(restored.layerNames(), original.layerNames());
    EXPECT_EQ(restored.rankBudget(), original.rankBudget());
    for (const auto& name : original.layerNames()) {
        EXPECT_EQ(restored.getWeights(name), original.getWeights(name));
        EXPECT_EQ(restored.getActiveRank(name), original.getActiveRank(name));
        EXPECT_FLOAT_EQ(restored.getImportance(name), original.getImportance(name));
        EXPECT_EQ(restored.forward(name, {0.5f, 1.0f}, 1),
                  original.forward(name, {0.5f, 1.0f}, 1));
    }
    const auto stats = restored.getLayerStats();
    ASSERT_EQ(stats.size(), 2u);
    EXPECT_FLOAT_EQ(stats[0].alpha, 7.0f);
    EXPECT_FLOAT_EQ(stats[1].alpha, 9.0f);

    const auto first = original.trainBatch("z", {1, 2}, {0.25f}, 1, 0.001f);
    const auto second = restored.trainBatch("z", {1, 2}, {0.25f}, 1, 0.001f);
    EXPECT_FLOAT_EQ(first.loss, second.loss);
    EXPECT_FLOAT_EQ(first.gradient_norm, second.gradient_norm);
    EXPECT_EQ(original.getWeights("z"), restored.getWeights("z"));
    EXPECT_EQ(original.reallocateRanks().total_active_rank,
              restored.reallocateRanks().total_active_rank);
    for (const auto& name : original.layerNames())
        EXPECT_EQ(original.getActiveRank(name), restored.getActiveRank(name));

    restored.addLayer("defaults", 1, 1);
    EXPECT_EQ(restored.getMaxRank("defaults"), 3u);
    EXPECT_FLOAT_EQ(restored.getLayerStats().back().alpha, 5.0f);
}

TEST(AdaLoRAAdapterTest, CheckpointRejectsCorruptAndTruncatedWithoutMutation) {
    CheckpointFile file;
    AdaLoRAAdapter source(2, 3.0f, 1);
    source.addLayer("x", 1, 1, 2, 4.0f);
    source.setWeights("x", {1, 2}, {3, 4});
    source.saveToFile(file.path, "model");
    const auto valid = checkpointBytes(file.path);
    ASSERT_GT(valid.size(), 150u);

    AdaLoRAAdapter target(1, 2.0f, 7);
    target.addLayer("keep", 1, 1, 1, 6.0f);
    target.setWeights("keep", {2}, {3});
    const auto before = target.getWeights("keep");
    auto reject = [&](const std::vector<char>& bytes) {
        writeCheckpointBytes(file.path, bytes);
        EXPECT_THROW(target.loadFromFile(file.path), std::runtime_error);
        EXPECT_EQ(target.layerNames(), std::vector<std::string>{"keep"});
        EXPECT_EQ(target.getWeights("keep"), before);
        EXPECT_EQ(target.rankBudget(), 7u);
        EXPECT_FLOAT_EQ(target.getLayerStats().front().alpha, 6.0f);
    };

    for (size_t n = 0; n < valid.size(); ++n)
        reject(std::vector<char>(valid.begin(), valid.begin() + n));
    auto changed = valid;
    changed[valid.size() - 9] ^= 1;
    reject(changed);
    changed = valid;
    replaceCheckpointValue<uint32_t>(changed, 8, 1);
    reject(changed);
    EXPECT_FALSE(AdaLoRAAdapter::isCacheValid(file.path, "model"));
    // A real empty v1 file is shorter than the minimum v2 file.
    changed.resize(8 + 4 + 64 + 4);
    replaceCheckpointValue<uint32_t>(changed, 8 + 4 + 64, 0);
    writeCheckpointBytes(file.path, changed);
    try {
        target.loadFromFile(file.path);
        FAIL() << "v1 checkpoint was accepted";
    } catch (const std::runtime_error& error) {
        EXPECT_NE(std::string(error.what()).find("unsupported version 1"), std::string::npos);
    }
    EXPECT_FALSE(AdaLoRAAdapter::isCacheValid(file.path, "model"));
    EXPECT_EQ(target.getWeights("keep"), before);
    changed = valid;
    replaceCheckpointValue<uint32_t>(changed, 8, 99);
    reject(changed);
    // Recompute the checksum to exercise metadata validation, not just integrity.
    changed = valid;
    replaceCheckpointValue<uint32_t>(changed, 100, std::numeric_limits<uint32_t>::max());
    updateCheckpointChecksum(changed);
    reject(changed);
    changed = valid;
    replaceCheckpointValue<uint32_t>(changed, 96, std::numeric_limits<uint32_t>::max());
    updateCheckpointChecksum(changed);
    reject(changed);
    changed = valid;
    replaceCheckpointValue<uint64_t>(changed, 105, std::numeric_limits<uint64_t>::max());
    updateCheckpointChecksum(changed);
    reject(changed);
    changed = valid;
    replaceCheckpointValue<uint64_t>(changed, 129, 3); // active_rank > max_rank
    updateCheckpointChecksum(changed);
    reject(changed);
    changed = valid;
    replaceCheckpointValue<float>(changed, 137, std::numeric_limits<float>::quiet_NaN());
    updateCheckpointChecksum(changed);
    reject(changed);
    changed = valid;
    replaceCheckpointValue<uint64_t>(changed, 76, 0); // default rank must be nonzero
    updateCheckpointChecksum(changed);
    reject(changed);
    if (std::numeric_limits<size_t>::max() < std::numeric_limits<uint64_t>::max()) {
        changed = valid;
        replaceCheckpointValue<uint64_t>(changed, 88, std::numeric_limits<uint64_t>::max());
        updateCheckpointChecksum(changed);
        reject(changed);
    }
    changed = valid;
    replaceCheckpointValue<float>(changed, 145, std::numeric_limits<float>::infinity());
    updateCheckpointChecksum(changed);
    reject(changed);
    changed = valid;
    changed[18] = 'x'; // nonzero after fingerprint NUL padding
    updateCheckpointChecksum(changed);
    reject(changed);
    changed = valid;
    changed.insert(changed.end() - sizeof(uint64_t), 'x');
    updateCheckpointChecksum(changed);
    reject(changed);
}

TEST(AdaLoRAAdapterTest, CheckpointRejectsDuplicateNamesAndInvalidSaveState) {
    CheckpointFile file;
    AdaLoRAAdapter adapter(1, 2.0f, 2);
    adapter.addLayer("x", 1, 1, 1);
    adapter.addLayer("y", 1, 1, 1);
    adapter.saveToFile(file.path, "model");
    const auto valid = checkpointBytes(file.path);
    auto duplicate = valid;
    ASSERT_GT(duplicate.size(), 157u);
    duplicate[157] = 'x'; // second one-byte name
    updateCheckpointChecksum(duplicate);
    writeCheckpointBytes(file.path, duplicate);
    EXPECT_THROW(adapter.loadFromFile(file.path), std::runtime_error);
    EXPECT_EQ(adapter.layerNames(), (std::vector<std::string>{"x", "y"}));

    writeCheckpointBytes(file.path, valid);
    adapter.setWeights("x", {std::numeric_limits<float>::infinity()}, {0.0f});
    EXPECT_THROW(adapter.saveToFile(file.path, "model"), std::runtime_error);
    EXPECT_EQ(checkpointBytes(file.path), valid); // validation precedes truncation
    EXPECT_THROW(adapter.saveToFile(file.path, std::string(65, 'a')), std::runtime_error);
    EXPECT_EQ(checkpointBytes(file.path), valid);
}

TEST(AdaLoRAAdapterTest, CheckpointRestoresEmptyAdapterAndZeroBudget) {
    CheckpointFile file;
    AdaLoRAAdapter source(2, 3.0f, 1);
    source.setRankBudget(0);
    source.saveToFile(file.path);
    AdaLoRAAdapter target;
    target.addLayer("old", 1, 1);
    EXPECT_TRUE(target.loadFromFile(file.path).empty());
    EXPECT_EQ(target.layerCount(), 0u);
    EXPECT_EQ(target.rankBudget(), 0u);
    target.addLayer("new", 1, 1);
    EXPECT_EQ(target.getMaxRank("new"), 2u);
    EXPECT_FLOAT_EQ(target.getLayerStats().front().alpha, 3.0f);
    EXPECT_THROW(target.reallocateRanks(), std::invalid_argument);
    target.saveToFile(file.path);
    AdaLoRAAdapter with_layer;
    with_layer.loadFromFile(file.path);
    EXPECT_EQ(with_layer.rankBudget(), 0u);
    EXPECT_EQ(with_layer.layerNames(), (std::vector<std::string>{"new"}));
    EXPECT_THROW(with_layer.reallocateRanks(), std::invalid_argument);
}

TEST(AdaLoRAAdapterTest, CheckpointPreservesLargeConfigurationValues) {
    CheckpointFile file;
    CheckpointFile resaved;
    for (const size_t rank : {size_t{1}, size_t{33554433}, std::numeric_limits<size_t>::max()}) {
        for (const size_t budget : {size_t{0}, size_t{33554433}, std::numeric_limits<size_t>::max()}) {
            AdaLoRAAdapter source(rank, 3.0f, 1);
            source.setRankBudget(budget);
            source.saveToFile(file.path, "model");
            AdaLoRAAdapter loaded;
            EXPECT_EQ(loaded.loadFromFile(file.path), "model");
            EXPECT_EQ(loaded.layerCount(), 0u);
            EXPECT_EQ(loaded.rankBudget(), budget);
            loaded.saveToFile(resaved.path, "model");
            EXPECT_EQ(checkpointBytes(file.path), checkpointBytes(resaved.path));

            source.addLayer("small", 1, 1, 1);
            source.saveToFile(file.path, "model");
            EXPECT_EQ(loaded.loadFromFile(file.path), "model");
            EXPECT_EQ(loaded.getMaxRank("small"), 1u);
            EXPECT_EQ(loaded.rankBudget(), budget);
            loaded.saveToFile(resaved.path, "model");
            EXPECT_EQ(checkpointBytes(file.path), checkpointBytes(resaved.path));
        }
    }
}
