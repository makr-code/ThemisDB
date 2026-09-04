// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

/**
 * @file test_training_lora_adapter.cpp
 * @brief Unit tests for training::LoRAAdapter – the real LoRA weight
 *        manipulation layer in the training module.
 *
 * Tests verify:
 *  - Layer registration and removal
 *  - Kaiming B / zero A initialisation
 *  - setWeights round-trip
 *  - applyUpdate (single-layer additive delta)
 *  - applyBatchUpdate (multi-layer atomic deltas)
 *  - forward pass: real matmul (output = (input @ B @ A) × scaling)
 *  - exportWeights / importWeights round-trip
 *  - Error handling (unknown layer, size mismatch, invalid construction)
 */

#include <gtest/gtest.h>
#include "training/lora_adapter.h"

#include <algorithm>
#include <cmath>
#include <numeric>
#include <string>
#include <vector>

using namespace themis::training;

// ============================================================================
// Construction and default configuration
// ============================================================================

TEST(TrainingLoRAAdapterTest, ConstructsWithDefaults) {
    LoRAAdapter adapter;
    EXPECT_EQ(adapter.layerCount(), 0u);
    EXPECT_EQ(adapter.totalParameterCount(), 0u);
    EXPECT_TRUE(adapter.layerNames().empty());
}

TEST(TrainingLoRAAdapterTest, ConstructsWithCustomRankAlpha) {
    LoRAAdapter adapter(8, 16.0f);
    // Just check that construction succeeds
    EXPECT_EQ(adapter.layerCount(), 0u);
}

TEST(TrainingLoRAAdapterTest, InvalidRankThrows) {
    EXPECT_THROW(LoRAAdapter(0, 8.0f), std::invalid_argument);
}

TEST(TrainingLoRAAdapterTest, InvalidAlphaThrows) {
    EXPECT_THROW(LoRAAdapter(4, 0.0f), std::invalid_argument);
    EXPECT_THROW(LoRAAdapter(4, -1.0f), std::invalid_argument);
}

// ============================================================================
// Layer management
// ============================================================================

TEST(TrainingLoRAAdapterTest, AddLayer_Success) {
    LoRAAdapter adapter(4, 8.0f);
    adapter.addLayer("query", 64, 64);
    EXPECT_EQ(adapter.layerCount(), 1u);
    EXPECT_TRUE(adapter.hasLayer("query"));
}

TEST(TrainingLoRAAdapterTest, AddLayer_MultipleDistinctNames) {
    LoRAAdapter adapter(4, 8.0f);
    adapter.addLayer("query", 64, 64);
    adapter.addLayer("key",   64, 64);
    adapter.addLayer("value", 64, 64);
    EXPECT_EQ(adapter.layerCount(), 3u);

    auto names = adapter.layerNames();
    std::sort(names.begin(), names.end());
    ASSERT_EQ(names.size(), 3u);
    EXPECT_EQ(names[0], "key");
    EXPECT_EQ(names[1], "query");
    EXPECT_EQ(names[2], "value");
}

TEST(TrainingLoRAAdapterTest, AddLayer_DuplicateNameThrows) {
    LoRAAdapter adapter;
    adapter.addLayer("layer", 32, 32);
    EXPECT_THROW(adapter.addLayer("layer", 32, 32), std::invalid_argument);
}

TEST(TrainingLoRAAdapterTest, AddLayer_ZeroDimThrows) {
    LoRAAdapter adapter;
    EXPECT_THROW(adapter.addLayer("l", 0, 32), std::invalid_argument);
    EXPECT_THROW(adapter.addLayer("l", 32, 0), std::invalid_argument);
}

TEST(TrainingLoRAAdapterTest, AddLayer_RankExceedsDimThrows) {
    LoRAAdapter adapter;
    // rank = 8 > out_dim = 4
    EXPECT_THROW(adapter.addLayer("l", 64, 4, 8), std::invalid_argument);
}

TEST(TrainingLoRAAdapterTest, AddLayer_OverrideRankAndAlpha) {
    LoRAAdapter adapter(4, 8.0f);
    adapter.addLayer("custom", 64, 64, 16, 32.0f);
    const auto& w = adapter.getWeights("custom");
    EXPECT_EQ(w.rank,  16u);
    EXPECT_FLOAT_EQ(w.alpha, 32.0f);
}

TEST(TrainingLoRAAdapterTest, RemoveLayer_ExistingSucceeds) {
    LoRAAdapter adapter;
    adapter.addLayer("q", 32, 32);
    EXPECT_TRUE(adapter.removeLayer("q"));
    EXPECT_FALSE(adapter.hasLayer("q"));
    EXPECT_EQ(adapter.layerCount(), 0u);
}

TEST(TrainingLoRAAdapterTest, RemoveLayer_NonExistentReturnsFalse) {
    LoRAAdapter adapter;
    EXPECT_FALSE(adapter.removeLayer("does_not_exist"));
}

TEST(TrainingLoRAAdapterTest, HasLayer_FalseForUnknown) {
    LoRAAdapter adapter;
    EXPECT_FALSE(adapter.hasLayer("xyz"));
}

// ============================================================================
// Initialisation – Kaiming B, zero A
// ============================================================================

TEST(TrainingLoRAAdapterTest, BMatrix_InitialisedWithNonZeroKaiming) {
    LoRAAdapter adapter(4, 8.0f);
    adapter.addLayer("layer", 64, 64);
    const auto& w = adapter.getWeights("layer");

    // B must have at least one non-zero entry after Kaiming init
    bool has_nonzero = std::any_of(w.B.begin(), w.B.end(),
                                   [](float v) { return std::abs(v) > 1e-9f; });
    EXPECT_TRUE(has_nonzero) << "B should be Kaiming-initialised (non-zero)";
}

TEST(TrainingLoRAAdapterTest, AMatrix_InitialisedAllZero) {
    LoRAAdapter adapter(4, 8.0f);
    adapter.addLayer("layer", 64, 64);
    const auto& w = adapter.getWeights("layer");

    for (size_t i = 0; i < w.A.size(); ++i)
        EXPECT_FLOAT_EQ(w.A[i], 0.0f) << "A[" << i << "] should be zero at init";
}

TEST(TrainingLoRAAdapterTest, DifferentLayers_DifferentBMatrices) {
    // Two layers with the same dimensions but different names must receive
    // distinct Kaiming initialisations (different seeds).
    LoRAAdapter adapter(4, 8.0f);
    adapter.addLayer("alpha", 32, 32);
    adapter.addLayer("beta",  32, 32);

    const auto& wA = adapter.getWeights("alpha");
    const auto& wB = adapter.getWeights("beta");

    bool any_diff = false;
    for (size_t i = 0; i < wA.B.size(); ++i) {
        if (wA.B[i] != wB.B[i]) { any_diff = true; break; }
    }
    EXPECT_TRUE(any_diff) << "Different layer names should produce different B initialisations";
}

// ============================================================================
// Parameter counting
// ============================================================================

TEST(TrainingLoRAAdapterTest, TotalParameterCount_MatchesExpected) {
    LoRAAdapter adapter(4, 8.0f);
    // in_dim=64, out_dim=64, rank=4 → B: 64×4=256, A: 4×64=256 → 512 per layer
    adapter.addLayer("l1", 64, 64, 4);
    adapter.addLayer("l2", 64, 64, 4);
    EXPECT_EQ(adapter.totalParameterCount(), 2u * (64u * 4u + 4u * 64u));
}

// ============================================================================
// setWeights / getWeights round-trip
// ============================================================================

TEST(TrainingLoRAAdapterTest, SetWeights_RoundTrip) {
    LoRAAdapter adapter(4, 8.0f);
    adapter.addLayer("layer", 8, 8, 2);

    std::vector<float> B_new(8 * 2, 1.5f);
    std::vector<float> A_new(2 * 8, 2.5f);
    adapter.setWeights("layer", B_new, A_new);

    const auto& w = adapter.getWeights("layer");
    for (float v : w.B) {
      EXPECT_FLOAT_EQ(v, 1.5f);
    }
    for (float v : w.A) {
      EXPECT_FLOAT_EQ(v, 2.5f);
    }
}

TEST(TrainingLoRAAdapterTest, SetWeights_UnknownLayerThrows) {
    LoRAAdapter adapter;
    EXPECT_THROW(adapter.setWeights("ghost", {}, {}), std::out_of_range);
}

TEST(TrainingLoRAAdapterTest, SetWeights_BsizeMismatchThrows) {
    LoRAAdapter adapter(4, 8.0f);
    adapter.addLayer("l", 8, 8, 2);

    std::vector<float> bad_B(5, 0.0f);   // wrong size
    std::vector<float> good_A(2 * 8, 0.0f);
    EXPECT_THROW(adapter.setWeights("l", bad_B, good_A), std::invalid_argument);
}

TEST(TrainingLoRAAdapterTest, SetWeights_AsizeMismatchThrows) {
    LoRAAdapter adapter(4, 8.0f);
    adapter.addLayer("l", 8, 8, 2);

    std::vector<float> good_B(8 * 2, 0.0f);
    std::vector<float> bad_A(3, 0.0f);   // wrong size
    EXPECT_THROW(adapter.setWeights("l", good_B, bad_A), std::invalid_argument);
}

// ============================================================================
// applyUpdate – single layer
// ============================================================================

TEST(TrainingLoRAAdapterTest, ApplyUpdate_MutatesWeightsAdditively) {
    LoRAAdapter adapter(4, 8.0f);
    adapter.addLayer("q", 8, 8, 2);

    // Snapshot initial weights
    const auto& w0 = adapter.getWeights("q");
    std::vector<float> B_init = w0.B;
    std::vector<float> A_init = w0.A;

    std::vector<float> dB(8 * 2, 0.1f);
    std::vector<float> dA(2 * 8, 0.2f);
    auto res = adapter.applyUpdate("q", dB, dA);

    EXPECT_TRUE(res.success);
    EXPECT_EQ(res.layers_updated, 1u);

    const auto& w1 = adapter.getWeights("q");
    for (size_t i = 0; i < w1.B.size(); ++i)
        EXPECT_NEAR(w1.B[i], B_init[i] + 0.1f, 1e-5f);
    for (size_t i = 0; i < w1.A.size(); ++i)
        EXPECT_NEAR(w1.A[i], A_init[i] + 0.2f, 1e-5f);
}

TEST(TrainingLoRAAdapterTest, ApplyUpdate_UnknownLayerThrows) {
    LoRAAdapter adapter;
    EXPECT_THROW(adapter.applyUpdate("ghost", {}, {}), std::out_of_range);
}

TEST(TrainingLoRAAdapterTest, ApplyUpdate_DeltaBsizeMismatchThrows) {
    LoRAAdapter adapter(4, 8.0f);
    adapter.addLayer("l", 8, 8, 2);

    std::vector<float> bad_dB(1, 0.0f);
    std::vector<float> good_dA(2 * 8, 0.0f);
    EXPECT_THROW(adapter.applyUpdate("l", bad_dB, good_dA), std::invalid_argument);
}

TEST(TrainingLoRAAdapterTest, ApplyUpdate_DeltaAsizeMismatchThrows) {
    LoRAAdapter adapter(4, 8.0f);
    adapter.addLayer("l", 8, 8, 2);

    std::vector<float> good_dB(8 * 2, 0.0f);
    std::vector<float> bad_dA(1, 0.0f);
    EXPECT_THROW(adapter.applyUpdate("l", good_dB, bad_dA), std::invalid_argument);
}

// ============================================================================
// applyBatchUpdate – multi-layer
// ============================================================================

TEST(TrainingLoRAAdapterTest, ApplyBatchUpdate_AllLayersUpdated) {
    LoRAAdapter adapter(4, 8.0f);
    adapter.addLayer("q", 8, 8, 2);
    adapter.addLayer("k", 8, 8, 2);

    // Snapshot initial A matrices (all zeros at init)
    auto A_q_init = adapter.getWeights("q").A;
    auto A_k_init = adapter.getWeights("k").A;

    WeightUpdateBatch batch;
    batch.layer_names = {"q", "k"};
    batch.delta_B = {std::vector<float>(8 * 2, 0.05f),
                     std::vector<float>(8 * 2, 0.07f)};
    batch.delta_A = {std::vector<float>(2 * 8, 0.03f),
                     std::vector<float>(2 * 8, 0.04f)};

    auto res = adapter.applyBatchUpdate(batch);

    EXPECT_TRUE(res.success);
    EXPECT_EQ(res.layers_updated, 2u);
    EXPECT_EQ(res.layers_skipped, 0u);

    // Verify A values changed
    const auto& wq = adapter.getWeights("q");
    for (size_t i = 0; i < wq.A.size(); ++i)
        EXPECT_NEAR(wq.A[i], A_q_init[i] + 0.03f, 1e-5f);

    const auto& wk = adapter.getWeights("k");
    for (size_t i = 0; i < wk.A.size(); ++i)
        EXPECT_NEAR(wk.A[i], A_k_init[i] + 0.04f, 1e-5f);
}

TEST(TrainingLoRAAdapterTest, ApplyBatchUpdate_SkipsUnknownLayers) {
    LoRAAdapter adapter(4, 8.0f);
    adapter.addLayer("real", 8, 8, 2);

    WeightUpdateBatch batch;
    batch.layer_names = {"real", "ghost"};
    batch.delta_B = {std::vector<float>(8 * 2, 0.1f),
                     std::vector<float>(8 * 2, 0.1f)};
    batch.delta_A = {std::vector<float>(2 * 8, 0.1f),
                     std::vector<float>(2 * 8, 0.1f)};

    auto res = adapter.applyBatchUpdate(batch);
    EXPECT_TRUE(res.success);
    EXPECT_EQ(res.layers_updated, 1u);
    EXPECT_EQ(res.layers_skipped, 1u);
}

TEST(TrainingLoRAAdapterTest, ApplyBatchUpdate_SizeMismatchVectorsThrows) {
    LoRAAdapter adapter;
    WeightUpdateBatch bad_batch;
    bad_batch.layer_names = {"a", "b"};
    bad_batch.delta_B     = {std::vector<float>(4, 0.0f)};  // only 1 entry
    bad_batch.delta_A     = {std::vector<float>(4, 0.0f),
                              std::vector<float>(4, 0.0f)};
    EXPECT_THROW(adapter.applyBatchUpdate(bad_batch), std::invalid_argument);
}

// ============================================================================
// Forward pass
// ============================================================================

TEST(TrainingLoRAAdapterTest, Forward_ZeroAtInit) {
    // A is all-zero at init → output must be all-zero regardless of input
    LoRAAdapter adapter(4, 8.0f);
    adapter.addLayer("q", 8, 8, 2);

    std::vector<float> input(3 * 8, 0.5f);  // batch_size=3, in_dim=8
    auto output = adapter.forward("q", input, 3);

    ASSERT_EQ(output.size(), 3u * 8u);
    for (float v : output)
        EXPECT_NEAR(v, 0.0f, 1e-5f) << "Initial adapter output must be zero (A=0)";
}

TEST(TrainingLoRAAdapterTest, Forward_OutputShape) {
    LoRAAdapter adapter(4, 8.0f);
    adapter.addLayer("q", 16, 32, 4);

    std::vector<float> input(5 * 16, 1.0f);  // batch_size=5, in_dim=16
    auto output = adapter.forward("q", input, 5);

    EXPECT_EQ(output.size(), 5u * 32u);  // batch_size × out_dim
}

TEST(TrainingLoRAAdapterTest, Forward_NonZeroAfterAUpdate) {
    // After setting A to non-zero, the forward pass must produce non-zero output
    LoRAAdapter adapter(4, 8.0f);
    adapter.addLayer("q", 8, 8, 2);

    // Set A to all 1.0 so output = (input @ B) × scaling (non-zero)
    std::vector<float> B_new(8 * 2, 0.1f);
    std::vector<float> A_new(2 * 8, 1.0f);
    adapter.setWeights("q", B_new, A_new);

    std::vector<float> input(2 * 8, 0.5f);  // batch_size=2, in_dim=8
    auto output = adapter.forward("q", input, 2);

    bool has_nonzero = std::any_of(output.begin(), output.end(),
                                   [](float v) { return std::abs(v) > 1e-7f; });
    EXPECT_TRUE(has_nonzero) << "Output should be non-zero after A is set to 1.0";
}

TEST(TrainingLoRAAdapterTest, Forward_ScalingApplied) {
    // With B=I and A=I (rank=2, dim=2), output = input × (alpha/rank)
    const float alpha = 8.0f;
    const size_t rank = 2;
    LoRAAdapter adapter(rank, alpha);
    adapter.addLayer("q", 2, 2, rank, alpha);

    // B = identity (2×2)
    std::vector<float> B_id = {1.0f, 0.0f,
                                0.0f, 1.0f};
    // A = identity (2×2)
    std::vector<float> A_id = {1.0f, 0.0f,
                                0.0f, 1.0f};
    adapter.setWeights("q", B_id, A_id);

    // Single sample: input = [1.0, 2.0]
    std::vector<float> input = {1.0f, 2.0f};
    auto output = adapter.forward("q", input, 1);

    const float scaling = alpha / static_cast<float>(rank);   // = 4.0
    ASSERT_EQ(output.size(), 2u);
    EXPECT_NEAR(output[0], 1.0f * scaling, 1e-4f);
    EXPECT_NEAR(output[1], 2.0f * scaling, 1e-4f);
}

TEST(TrainingLoRAAdapterTest, Forward_UnknownLayerThrows) {
    LoRAAdapter adapter;
    EXPECT_THROW(adapter.forward("ghost", {}, 1), std::out_of_range);
}

TEST(TrainingLoRAAdapterTest, Forward_InputSizeMismatchThrows) {
    LoRAAdapter adapter(4, 8.0f);
    adapter.addLayer("q", 8, 8, 2);

    std::vector<float> wrong_input(7, 0.5f);  // 7 != batch_size(1) × in_dim(8)
    EXPECT_THROW(adapter.forward("q", wrong_input, 1), std::invalid_argument);
}

// ============================================================================
// exportWeights / importWeights
// ============================================================================

TEST(TrainingLoRAAdapterTest, ExportImport_RoundTrip) {
    LoRAAdapter adapter(4, 8.0f);
    adapter.addLayer("a", 8, 8, 2);
    adapter.addLayer("b", 16, 8, 4);

    // Modify some weights so they're not at init values
    std::vector<float> dB(8 * 2, 0.5f);
    std::vector<float> dA(2 * 8, 0.3f);
    adapter.applyUpdate("a", dB, dA);

    auto exported = adapter.exportWeights();
    EXPECT_EQ(exported.size(), 2u);

    // Import into a fresh adapter
    LoRAAdapter adapter2(4, 8.0f);
    adapter2.importWeights(exported);
    EXPECT_EQ(adapter2.layerCount(), 2u);
    EXPECT_TRUE(adapter2.hasLayer("a"));
    EXPECT_TRUE(adapter2.hasLayer("b"));

    const auto& wa1 = adapter.getWeights("a");
    const auto& wa2 = adapter2.getWeights("a");
    ASSERT_EQ(wa1.B.size(), wa2.B.size());
    for (size_t i = 0; i < wa1.B.size(); ++i)
        EXPECT_FLOAT_EQ(wa1.B[i], wa2.B[i]);
    for (size_t i = 0; i < wa1.A.size(); ++i)
        EXPECT_FLOAT_EQ(wa1.A[i], wa2.A[i]);
}

TEST(TrainingLoRAAdapterTest, ImportWeights_OverwritesExistingLayer) {
    LoRAAdapter adapter(4, 8.0f);
    adapter.addLayer("q", 8, 8, 2);

    LoRAWeightEntry entry;
    entry.layer_name = "q";
    entry.in_dim     = 8;
    entry.out_dim    = 8;
    entry.rank       = 2;
    entry.alpha      = 8.0f;
    entry.B.assign(8 * 2, 99.0f);
    entry.A.assign(2 * 8, 88.0f);

    adapter.importWeights({entry});

    const auto& w = adapter.getWeights("q");
    EXPECT_FLOAT_EQ(w.B[0], 99.0f);
    EXPECT_FLOAT_EQ(w.A[0], 88.0f);
}

TEST(TrainingLoRAAdapterTest, ImportWeights_AddsNewLayer) {
    LoRAAdapter adapter;
    EXPECT_FALSE(adapter.hasLayer("new"));

    LoRAWeightEntry entry;
    entry.layer_name = "new";
    entry.in_dim     = 4;
    entry.out_dim    = 4;
    entry.rank       = 2;
    entry.alpha      = 4.0f;
    entry.B.assign(4 * 2, 1.0f);
    entry.A.assign(2 * 4, 0.0f);
    adapter.importWeights({entry});

    EXPECT_TRUE(adapter.hasLayer("new"));
}

TEST(TrainingLoRAAdapterTest, ImportWeights_SizeMismatchThrows) {
    LoRAAdapter adapter;

    LoRAWeightEntry bad;
    bad.layer_name = "x";
    bad.in_dim     = 8;
    bad.out_dim    = 8;
    bad.rank       = 2;
    bad.alpha      = 4.0f;
    bad.B.assign(3, 0.0f);   // wrong size (expect 8×2=16)
    bad.A.assign(16, 0.0f);

    EXPECT_THROW(adapter.importWeights({bad}), std::invalid_argument);
}

// ============================================================================
// Batch update convergence sanity: repeated delta applications shift weights
// ============================================================================

TEST(TrainingLoRAAdapterTest, RepeatedBatchUpdates_WeightsAccumulate) {
    LoRAAdapter adapter(4, 8.0f);
    adapter.addLayer("l", 8, 8, 2);

    const float delta = 0.01f;
    const int   steps = 10;
    std::vector<float> dA_step(2 * 8, delta);
    std::vector<float> dB_step(8 * 2, 0.0f);  // only update A for clarity

    // Initial A is all-zeros
    for (int s = 0; s < steps; ++s) {
        WeightUpdateBatch batch;
        batch.layer_names = {"l"};
        batch.delta_B     = {dB_step};
        batch.delta_A     = {dA_step};
        adapter.applyBatchUpdate(batch);
    }

    // After `steps` updates each A element should equal steps * delta
    const auto& w = adapter.getWeights("l");
    for (float v : w.A)
        EXPECT_NEAR(v, static_cast<float>(steps) * delta, 1e-4f);
}
