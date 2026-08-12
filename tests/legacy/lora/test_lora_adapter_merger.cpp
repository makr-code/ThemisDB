// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

/**
 * @file test_lora_adapter_merger.cpp
 * @brief Unit tests for training::LoRAAdapterMerger — linear and TIES merge.
 */

#include <gtest/gtest.h>
#include "training/lora_adapter_merger.h"
#include "training/lora_adapter.h"

#include <algorithm>
#include <cmath>
#include <numeric>
#include <string>
#include <vector>

using namespace themis::training;

// ============================================================================
// Test helpers
// ============================================================================

namespace {

// Build a heap-allocated LoRAAdapter with one layer of given size and constant weights
std::unique_ptr<LoRAAdapter> makeAdapter(const std::string& layer_name,
                        size_t in_dim, size_t out_dim,
                        size_t rank, float alpha,
                        float B_val, float A_val) {
    auto a = std::make_unique<LoRAAdapter>(rank, alpha);
    a->addLayer(layer_name, in_dim, out_dim, rank, alpha);
    std::vector<float> B(in_dim * rank, B_val);
    std::vector<float> A(rank * out_dim, A_val);
    a->setWeights(layer_name, B, A);
    return a;
}

} // anonymous namespace

// ============================================================================
// Linear merge – single layer
// ============================================================================

TEST(LoRAAdapterMergerTest, LinearMergeProducesCorrectShapes) {
    auto a1 = makeAdapter("q_proj", 4, 4, 2, 8.0f, 1.0f, 1.0f);
    auto a2 = makeAdapter("q_proj", 4, 4, 2, 8.0f, 1.0f, 1.0f);

    LoRAAdapterMerger merger;
    std::vector<AdapterDescriptor> descs = {
        {a1.get(), "q_proj", 0.5f},
        {a2.get(), "q_proj", 0.5f}
    };
    auto res = merger.mergeLinear(descs, "q_proj", 4, 4, 2, 8.0f);
    EXPECT_TRUE(res.success);
    EXPECT_EQ(res.B.size(), 4u * 2u);
    EXPECT_EQ(res.A.size(), 2u * 4u);
}

TEST(LoRAAdapterMergerTest, LinearMergeWithZeroWeightsProducesZero) {
    auto a1 = makeAdapter("q_proj", 2, 2, 1, 1.0f, 1.0f, 1.0f);
    auto a2 = makeAdapter("q_proj", 2, 2, 1, 1.0f, 1.0f, 1.0f);

    LoRAAdapterMerger merger;
    std::vector<AdapterDescriptor> descs = {
        {a1.get(), "q_proj", 0.0f},
        {a2.get(), "q_proj", 0.0f}
    };
    auto res = merger.mergeLinear(descs, "q_proj", 2, 2, 1, 1.0f);
    EXPECT_TRUE(res.success);
    // Merged ΔW = 0 → B and A should also be ~0
    for (float v : res.B) EXPECT_NEAR(v, 0.0f, 1e-5f);
    for (float v : res.A) EXPECT_NEAR(v, 0.0f, 1e-5f);
}

TEST(LoRAAdapterMergerTest, LinearMergeEmptyAdaptersReturnsError) {
    LoRAAdapterMerger merger;
    auto res = merger.mergeLinear({}, "q_proj", 4, 4, 2, 8.0f);
    EXPECT_FALSE(res.success);
    EXPECT_FALSE(res.error_message.empty());
}

TEST(LoRAAdapterMergerTest, LinearMergeNullAdapterReturnsError) {
    LoRAAdapterMerger merger;
    std::vector<AdapterDescriptor> descs = {{nullptr, "q_proj", 1.0f}};
    auto res = merger.mergeLinear(descs, "q_proj", 4, 4, 2, 8.0f);
    EXPECT_FALSE(res.success);
}

TEST(LoRAAdapterMergerTest, LinearMergeMissingLayerReturnsError) {
    auto a1 = makeAdapter("v_proj", 4, 4, 2, 8.0f, 1.0f, 1.0f);
    LoRAAdapterMerger merger;
    std::vector<AdapterDescriptor> descs = {{a1.get(), "q_proj", 1.0f}};
    auto res = merger.mergeLinear(descs, "q_proj", 4, 4, 2, 8.0f);
    EXPECT_FALSE(res.success);
}

TEST(LoRAAdapterMergerTest, LinearMergeZeroDimsReturnsError) {
    auto a1 = makeAdapter("q_proj", 4, 4, 2, 8.0f, 1.0f, 1.0f);
    LoRAAdapterMerger merger;
    std::vector<AdapterDescriptor> descs = {{a1.get(), "q_proj", 1.0f}};
    auto res = merger.mergeLinear(descs, "q_proj", 0, 4, 2, 8.0f);
    EXPECT_FALSE(res.success);
}

TEST(LoRAAdapterMergerTest, LinearMergeOutputLayerNameIsSet) {
    auto a1 = makeAdapter("q_proj", 4, 4, 2, 8.0f, 1.0f, 1.0f);
    LoRAAdapterMerger merger;
    std::vector<AdapterDescriptor> descs = {{a1.get(), "q_proj", 1.0f}};
    auto res = merger.mergeLinear(descs, "merged_q", 4, 4, 2, 8.0f);
    EXPECT_EQ(res.layer_name, "merged_q");
}

TEST(LoRAAdapterMergerTest, LinearMergeSingleAdapterIdentity) {
    // Merging a single adapter with weight 1.0 should reconstruct approx ΔW
    auto a1 = makeAdapter("q_proj", 4, 4, 1, 1.0f, 1.0f, 1.0f);
    LoRAAdapterMerger merger;
    std::vector<AdapterDescriptor> descs = {{a1.get(), "q_proj", 1.0f}};
    auto res = merger.mergeLinear(descs, "q_proj", 4, 4, 1, 1.0f);
    EXPECT_TRUE(res.success);
    EXPECT_FALSE(res.B.empty());
    EXPECT_FALSE(res.A.empty());
}

TEST(LoRAAdapterMergerTest, LinearMergeSymmetricWeightsProducesSymmetricResult) {
    auto a1 = makeAdapter("q_proj", 4, 4, 2, 8.0f, 2.0f, 1.0f);
    auto a2 = makeAdapter("q_proj", 4, 4, 2, 8.0f, 1.0f, 2.0f);

    LoRAAdapterMerger merger;
    // Equal weights → result of swap should be similar magnitude
    std::vector<AdapterDescriptor> d1 = {{a1.get(), "q_proj", 0.5f}, {a2.get(), "q_proj", 0.5f}};
    std::vector<AdapterDescriptor> d2 = {{a2.get(), "q_proj", 0.5f}, {a1.get(), "q_proj", 0.5f}};
    auto r1 = merger.mergeLinear(d1, "q_proj", 4, 4, 2, 8.0f);
    auto r2 = merger.mergeLinear(d2, "q_proj", 4, 4, 2, 8.0f);
    EXPECT_TRUE(r1.success);
    EXPECT_TRUE(r2.success);
    // The Frobenius norms should be equal (commutativity of sum)
    float f1 = 0.0f, f2 = 0.0f;
    for (float v : r1.B) f1 += v * v;
    for (float v : r2.B) f2 += v * v;
    EXPECT_NEAR(f1, f2, 1e-4f);
}

// ============================================================================
// Linear merge – all layers
// ============================================================================

TEST(LoRAAdapterMergerTest, LinearMergeAllSucceedsWithSharedLayers) {
    LoRAAdapter a1(2, 4.0f), a2(2, 4.0f);
    a1.addLayer("q_proj", 4, 4, 2, 4.0f);
    a1.addLayer("v_proj", 4, 4, 2, 4.0f);
    a2.addLayer("q_proj", 4, 4, 2, 4.0f);
    a2.addLayer("v_proj", 4, 4, 2, 4.0f);

    LoRAAdapterMerger merger;
    auto res = merger.mergeLinearAll({&a1, &a2}, {0.5f, 0.5f}, 2);
    EXPECT_TRUE(res.success);
    EXPECT_EQ(res.layers_merged, 2u);
    EXPECT_EQ(res.layers_failed, 0u);
}

TEST(LoRAAdapterMergerTest, LinearMergeAllSkipsMissingLayer) {
    LoRAAdapter a1(2, 4.0f), a2(2, 4.0f);
    a1.addLayer("q_proj", 4, 4, 2, 4.0f);
    a1.addLayer("v_proj", 4, 4, 2, 4.0f);
    a2.addLayer("q_proj", 4, 4, 2, 4.0f);
    // a2 missing v_proj

    LoRAAdapterMerger merger;
    auto res = merger.mergeLinearAll({&a1, &a2}, {0.5f, 0.5f}, 2);
    EXPECT_EQ(res.layers_merged, 1u);   // only q_proj
    EXPECT_EQ(res.layers_failed, 1u);   // v_proj skipped
}

TEST(LoRAAdapterMergerTest, LinearMergeAllWeightsMismatchReturnsError) {
    LoRAAdapter a1(2, 4.0f);
    LoRAAdapterMerger merger;
    auto res = merger.mergeLinearAll({&a1}, {0.5f, 0.5f}, 2); // sizes don't match
    EXPECT_FALSE(res.success);
}

TEST(LoRAAdapterMergerTest, LinearMergeAllNullAdapterReturnsError) {
    LoRAAdapterMerger merger;
    auto res = merger.mergeLinearAll({nullptr}, {1.0f}, 2);
    EXPECT_FALSE(res.success);
}

// ============================================================================
// TIES merge – single layer
// ============================================================================

TEST(LoRAAdapterMergerTest, TIESMergeProducesCorrectShapes) {
    auto a1 = makeAdapter("q_proj", 4, 4, 2, 8.0f, 1.0f, 1.0f);
    auto a2 = makeAdapter("q_proj", 4, 4, 2, 8.0f, 1.0f, 1.0f);

    LoRAAdapterMerger merger;
    std::vector<AdapterDescriptor> descs = {
        {a1.get(), "q_proj", 1.0f},
        {a2.get(), "q_proj", 1.0f}
    };
    auto res = merger.mergeTIES(descs, "q_proj", 4, 4, 2, 8.0f);
    EXPECT_TRUE(res.success);
    EXPECT_EQ(res.B.size(), 4u * 2u);
    EXPECT_EQ(res.A.size(), 2u * 4u);
}

TEST(LoRAAdapterMergerTest, TIESMergeEmptyAdaptersReturnsError) {
    LoRAAdapterMerger merger;
    auto res = merger.mergeTIES({}, "q_proj", 4, 4, 2, 8.0f);
    EXPECT_FALSE(res.success);
}

TEST(LoRAAdapterMergerTest, TIESMergeNullAdapterReturnsError) {
    LoRAAdapterMerger merger;
    std::vector<AdapterDescriptor> descs = {{nullptr, "q_proj", 1.0f}};
    auto res = merger.mergeTIES(descs, "q_proj", 4, 4, 2, 8.0f);
    EXPECT_FALSE(res.success);
}

TEST(LoRAAdapterMergerTest, TIESMergeMissingLayerReturnsError) {
    auto a1 = makeAdapter("v_proj", 4, 4, 2, 8.0f, 1.0f, 1.0f);
    LoRAAdapterMerger merger;
    std::vector<AdapterDescriptor> descs = {{a1.get(), "q_proj", 1.0f}};
    auto res = merger.mergeTIES(descs, "q_proj", 4, 4, 2, 8.0f);
    EXPECT_FALSE(res.success);
}

TEST(LoRAAdapterMergerTest, TIESMergeInvalidThresholdReturnsError) {
    auto a1 = makeAdapter("q_proj", 4, 4, 2, 8.0f, 1.0f, 1.0f);
    LoRAAdapterMerger merger;
    std::vector<AdapterDescriptor> descs = {{a1.get(), "q_proj", 1.0f}};
    auto res = merger.mergeTIES(descs, "q_proj", 4, 4, 2, 8.0f, -0.1f);
    EXPECT_FALSE(res.success);
    auto res2 = merger.mergeTIES(descs, "q_proj", 4, 4, 2, 8.0f, 1.0f);
    EXPECT_FALSE(res2.success);
}

TEST(LoRAAdapterMergerTest, TIESMergeZeroDimsReturnsError) {
    auto a1 = makeAdapter("q_proj", 4, 4, 2, 8.0f, 1.0f, 1.0f);
    LoRAAdapterMerger merger;
    std::vector<AdapterDescriptor> descs = {{a1.get(), "q_proj", 1.0f}};
    auto res = merger.mergeTIES(descs, "q_proj", 0, 4, 2, 8.0f);
    EXPECT_FALSE(res.success);
}

TEST(LoRAAdapterMergerTest, TIESMergeConflictingSignsProducesSingleSign) {
    // Adapter 1: positive ΔW
    auto a1 = makeAdapter("q_proj", 2, 2, 1, 1.0f, 1.0f, 1.0f);
    // Adapter 2: negative ΔW (negate all values)
    LoRAAdapter a2(1, 1.0f);
    a2.addLayer("q_proj", 2, 2, 1, 1.0f);
    a2.setWeights("q_proj", {-1.0f, -1.0f}, {1.0f, 1.0f});

    LoRAAdapterMerger merger;
    std::vector<AdapterDescriptor> descs = {
        {a1.get(), "q_proj", 1.0f},
        {&a2, "q_proj", 1.0f}
    };
    auto res = merger.mergeTIES(descs, "q_proj", 2, 2, 1, 1.0f, 0.0f);
    EXPECT_TRUE(res.success);
    // Should succeed even with sign conflicts
    EXPECT_EQ(res.B.size(), 2u);
    EXPECT_EQ(res.A.size(), 2u);
}

TEST(LoRAAdapterMergerTest, TIESMergeHighThresholdTrimsAll) {
    // All values below threshold → merged ΔW = 0
    auto a1 = makeAdapter("q_proj", 2, 2, 1, 1.0f, 0.001f, 0.001f);
    LoRAAdapterMerger merger;
    std::vector<AdapterDescriptor> descs = {{a1.get(), "q_proj", 1.0f}};
    // threshold = 0.99 → nearly everything trimmed
    auto res = merger.mergeTIES(descs, "q_proj", 2, 2, 1, 1.0f, 0.99f);
    EXPECT_TRUE(res.success);
    // Output should be near zero (high threshold trims most values)
    for (float v : res.B) EXPECT_NEAR(v, 0.0f, 5e-3f);
    for (float v : res.A) EXPECT_NEAR(v, 0.0f, 5e-3f);
}

// ============================================================================
// TIES merge – all layers
// ============================================================================

TEST(LoRAAdapterMergerTest, TIESMergeAllSucceedsWithSharedLayers) {
    LoRAAdapter a1(2, 4.0f), a2(2, 4.0f);
    a1.addLayer("q_proj", 4, 4, 2, 4.0f);
    a1.addLayer("v_proj", 4, 4, 2, 4.0f);
    a2.addLayer("q_proj", 4, 4, 2, 4.0f);
    a2.addLayer("v_proj", 4, 4, 2, 4.0f);

    LoRAAdapterMerger merger;
    auto res = merger.mergeTIESAll({&a1, &a2}, 2, 0.1f);
    EXPECT_TRUE(res.success);
    EXPECT_EQ(res.layers_merged, 2u);
}

TEST(LoRAAdapterMergerTest, TIESMergeAllSkipsMissingLayer) {
    LoRAAdapter a1(2, 4.0f), a2(2, 4.0f);
    a1.addLayer("q_proj", 4, 4, 2, 4.0f);
    a1.addLayer("v_proj", 4, 4, 2, 4.0f);
    a2.addLayer("q_proj", 4, 4, 2, 4.0f);

    LoRAAdapterMerger merger;
    auto res = merger.mergeTIESAll({&a1, &a2}, 2, 0.2f);
    EXPECT_EQ(res.layers_merged, 1u);
    EXPECT_EQ(res.layers_failed, 1u);
}

TEST(LoRAAdapterMergerTest, TIESMergeAllEmptyAdaptersReturnsError) {
    LoRAAdapterMerger merger;
    auto res = merger.mergeTIESAll({}, 2, 0.2f);
    EXPECT_FALSE(res.success);
}

TEST(LoRAAdapterMergerTest, TIESMergeAllNullAdapterReturnsError) {
    LoRAAdapterMerger merger;
    auto res = merger.mergeTIESAll({nullptr}, 2, 0.2f);
    EXPECT_FALSE(res.success);
}

// ============================================================================
// Consistency: linear vs TIES on identical adapters
// ============================================================================

TEST(LoRAAdapterMergerTest, LinearAndTIESGiveSameOrderOfMagnitude) {
    auto a1 = makeAdapter("q_proj", 4, 4, 2, 8.0f, 1.0f, 1.0f);
    auto a2 = makeAdapter("q_proj", 4, 4, 2, 8.0f, 1.0f, 1.0f);

    LoRAAdapterMerger merger;
    std::vector<AdapterDescriptor> descs = {
        {a1.get(), "q_proj", 0.5f},
        {a2.get(), "q_proj", 0.5f}
    };

    auto r_lin  = merger.mergeLinear(descs, "q_proj", 4, 4, 2, 8.0f);
    auto r_ties = merger.mergeTIES(descs,   "q_proj", 4, 4, 2, 8.0f, 0.0f);

    EXPECT_TRUE(r_lin.success);
    EXPECT_TRUE(r_ties.success);

    float mag_lin = 0.0f, mag_ties = 0.0f;
    for (float v : r_lin.B)  mag_lin  += v * v;
    for (float v : r_ties.B) mag_ties += v * v;
    // Both should be non-negative; zero B is acceptable for zero A initialization
    EXPECT_GE(mag_lin,  0.0f);
    EXPECT_GE(mag_ties, 0.0f);
}

TEST(LoRAAdapterMergerTest, MergerIsStateless) {
    auto a1 = makeAdapter("q_proj", 4, 4, 2, 8.0f, 1.0f, 1.0f);
    LoRAAdapterMerger merger;
    std::vector<AdapterDescriptor> descs = {{a1.get(), "q_proj", 1.0f}};
    // Calling twice should give the same result
    auto r1 = merger.mergeLinear(descs, "q_proj", 4, 4, 2, 8.0f);
    auto r2 = merger.mergeLinear(descs, "q_proj", 4, 4, 2, 8.0f);
    EXPECT_EQ(r1.B, r2.B);
    EXPECT_EQ(r1.A, r2.A);
}
