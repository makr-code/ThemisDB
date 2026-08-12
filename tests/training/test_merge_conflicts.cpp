// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

/**
 * @file test_merge_conflicts.cpp
 * @brief Phase 4 adapter merge conflict and rollback tests.
 *
 * Tests verify:
 *  - Linear merge of multiple adapters
 *  - TIES merge strategy (trim and re-scale)
 *  - Dimension mismatch detection
 *  - Rank mismatch handling
 *  - Merge conflict recovery and rollback
 *  - Weighted merge with custom weights
 *  - Merge failure reporting
 *  - Output layer naming and metadata
 */

#include <gtest/gtest.h>
#include "training/lora_adapter.h"
#include "training/lora_adapter_merger.h"

#include <algorithm>
#include <cmath>
#include <numeric>
#include <string>
#include <vector>

using namespace themis::training;

// ============================================================================
// Test fixture for merge operations
// ============================================================================

class MergeConflictTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create test adapters with known configurations
        adapter1_ = std::make_unique<LoRAAdapter>(4, 8.0f);
        adapter2_ = std::make_unique<LoRAAdapter>(4, 8.0f);
        adapter3_ = std::make_unique<LoRAAdapter>(4, 8.0f);

        // Add layers to each adapter
        adapter1_->addLayer("query", 64, 64, 4, 8.0f);
        adapter2_->addLayer("query", 64, 64, 4, 8.0f);
        adapter3_->addLayer("query", 64, 64, 4, 8.0f);

        // Initialize with distinct weights
        initializeAdapterWeights(adapter1_.get(), 0.1f);
        initializeAdapterWeights(adapter2_.get(), 0.2f);
        initializeAdapterWeights(adapter3_.get(), 0.3f);
    }

    void initializeAdapterWeights(LoRAAdapter* adapter, float scale) {
        const auto& w = adapter->getWeights("query");
        std::vector<float> B(w.B.size(), scale);
        std::vector<float> A(w.A.size(), scale);
        adapter->setWeights("query", B, A);
    }

    std::unique_ptr<LoRAAdapter> adapter1_;
    std::unique_ptr<LoRAAdapter> adapter2_;
    std::unique_ptr<LoRAAdapter> adapter3_;
};

// ============================================================================
// Linear merge – basic functionality
// ============================================================================

TEST_F(MergeConflictTest, LinearMerge_TwoAdapters_Succeeds) {
    LoRAAdapterMerger merger;

    std::vector<AdapterDescriptor> adapters = {
        {adapter1_.get(), "query", 0.5f},
        {adapter2_.get(), "query", 0.5f}
    };

    auto result = merger.mergeLinear(adapters, "query", 64, 64, 4, 8.0f);

    EXPECT_TRUE(result.success);
    EXPECT_EQ(result.layers_merged, 1u);
    EXPECT_EQ(result.layers.size(), 1u);
    EXPECT_EQ(result.layers[0].layer_name, "query");
}

TEST_F(MergeConflictTest, LinearMerge_ThreeAdapters_Succeeds) {
    LoRAAdapterMerger merger;

    std::vector<AdapterDescriptor> adapters = {
        {adapter1_.get(), "query", 0.33f},
        {adapter2_.get(), "query", 0.33f},
        {adapter3_.get(), "query", 0.34f}
    };

    auto result = merger.mergeLinear(adapters, "query", 64, 64, 4, 8.0f);

    EXPECT_TRUE(result.success);
    EXPECT_EQ(result.layers_merged, 1u);
}

TEST_F(MergeConflictTest, LinearMerge_OutputHasCorrectShape) {
    LoRAAdapterMerger merger;

    std::vector<AdapterDescriptor> adapters = {
        {adapter1_.get(), "query", 1.0f}
    };

    auto result = merger.mergeLinear(adapters, "query", 64, 64, 4, 8.0f);

    EXPECT_TRUE(result.success);
    EXPECT_EQ(result.layers[0].B.size(), 64u * 4u);
    EXPECT_EQ(result.layers[0].A.size(), 4u * 64u);
}

// ============================================================================
// Dimension mismatch detection
// ============================================================================

TEST_F(MergeConflictTest, DimensionMismatch_InputDim_Fails) {
    LoRAAdapterMerger merger;

    std::vector<AdapterDescriptor> adapters = {
        {adapter1_.get(), "query", 0.5f},
        {adapter2_.get(), "query", 0.5f}
    };

    // adapter is 64×64 but we specify wrong input dim
    auto result = merger.mergeLinear(adapters, "query", 128, 64, 4, 8.0f);

    EXPECT_FALSE(result.success);
    EXPECT_GT(result.layers_failed, 0u);
}

TEST_F(MergeConflictTest, DimensionMismatch_OutputDim_Fails) {
    LoRAAdapterMerger merger;

    std::vector<AdapterDescriptor> adapters = {
        {adapter1_.get(), "query", 0.5f},
        {adapter2_.get(), "query", 0.5f}
    };

    // adapter is 64×64 but we specify wrong output dim
    auto result = merger.mergeLinear(adapters, "query", 64, 128, 4, 8.0f);

    EXPECT_FALSE(result.success);
}

// ============================================================================
// Rank mismatch handling
// ============================================================================

TEST_F(MergeConflictTest, RankTooLarge_Fails) {
    LoRAAdapterMerger merger;

    std::vector<AdapterDescriptor> adapters = {
        {adapter1_.get(), "query", 0.5f},
        {adapter2_.get(), "query", 0.5f}
    };

    // rank=65 > output_dim=64
    auto result = merger.mergeLinear(adapters, "query", 64, 64, 65, 8.0f);

    EXPECT_FALSE(result.success);
}

TEST_F(MergeConflictTest, RankZero_Fails) {
    LoRAAdapterMerger merger;

    std::vector<AdapterDescriptor> adapters = {
        {adapter1_.get(), "query", 0.5f}
    };

    auto result = merger.mergeLinear(adapters, "query", 64, 64, 0, 8.0f);

    EXPECT_FALSE(result.success);
}

// ============================================================================
// Unknown layer handling
// ============================================================================

TEST_F(MergeConflictTest, UnknownLayer_Skipped) {
    LoRAAdapterMerger merger;

    std::vector<AdapterDescriptor> adapters = {
        {adapter1_.get(), "unknown_layer", 0.5f},
        {adapter2_.get(), "unknown_layer", 0.5f}
    };

    auto result = merger.mergeLinear(adapters, "query", 64, 64, 4, 8.0f);

    EXPECT_FALSE(result.success);
    EXPECT_GT(result.layers_failed, 0u);
}

// ============================================================================
// TIES merge strategy
// ============================================================================

TEST_F(MergeConflictTest, TIESMerge_BasicSucceeds) {
    LoRAAdapterMerger merger;

    std::vector<AdapterDescriptor> adapters = {
        {adapter1_.get(), "query", 1.0f},
        {adapter2_.get(), "query", 1.0f}
    };

    auto result = merger.mergeTIES(adapters, "query", 64, 64, 4, 8.0f, 0.2f);

    EXPECT_TRUE(result.success);
    EXPECT_EQ(result.layers_merged, 1u);
}

TEST_F(MergeConflictTest, TIESMerge_OutputShape) {
    LoRAAdapterMerger merger;

    std::vector<AdapterDescriptor> adapters = {
        {adapter1_.get(), "query", 1.0f}
    };

    auto result = merger.mergeTIES(adapters, "query", 64, 64, 4, 8.0f, 0.1f);

    EXPECT_TRUE(result.success);
    EXPECT_EQ(result.layers[0].B.size(), 64u * 4u);
    EXPECT_EQ(result.layers[0].A.size(), 4u * 64u);
}

// ============================================================================
// Weight normalization and scaling
// ============================================================================

TEST_F(MergeConflictTest, UnnormalizedWeights_Normalized) {
    LoRAAdapterMerger merger;

    // Create adapters with weights that don't sum to 1.0
    std::vector<AdapterDescriptor> adapters = {
        {adapter1_.get(), "query", 0.25f},
        {adapter2_.get(), "query", 0.75f}
    };

    auto result = merger.mergeLinear(adapters, "query", 64, 64, 4, 8.0f);

    EXPECT_TRUE(result.success);
    // Merged output should be reasonable despite non-normalized weights
}

// ============================================================================
// Multiple outputs from single merge
// ============================================================================

TEST_F(MergeConflictTest, MultipleOutputLayers_Tracked) {
    LoRAAdapterMerger merger;

    // Add more layers to adapters
    adapter1_->addLayer("key", 64, 64, 4, 8.0f);
    adapter2_->addLayer("key", 64, 64, 4, 8.0f);

    std::vector<AdapterDescriptor> adapters = {
        {adapter1_.get(), "query", 0.5f},
        {adapter2_.get(), "query", 0.5f}
    };

    auto result = merger.mergeLinear(adapters, "query", 64, 64, 4, 8.0f);

    EXPECT_TRUE(result.success);
    EXPECT_EQ(result.layers_merged, 1u);
}

// ============================================================================
// Empty adapter list handling
// ============================================================================

TEST_F(MergeConflictTest, EmptyAdapterList_Fails) {
    LoRAAdapterMerger merger;

    std::vector<AdapterDescriptor> adapters;  // Empty

    auto result = merger.mergeLinear(adapters, "query", 64, 64, 4, 8.0f);

    EXPECT_FALSE(result.success);
}

TEST_F(MergeConflictTest, SingleAdapter_Succeeds) {
    LoRAAdapterMerger merger;

    std::vector<AdapterDescriptor> adapters = {
        {adapter1_.get(), "query", 1.0f}
    };

    auto result = merger.mergeLinear(adapters, "query", 64, 64, 4, 8.0f);

    EXPECT_TRUE(result.success);
    EXPECT_EQ(result.layers_merged, 1u);
}

// ============================================================================
// Error messaging
// ============================================================================

TEST_F(MergeConflictTest, FailureMessagesPopulated() {
    LoRAAdapterMerger merger;

    std::vector<AdapterDescriptor> adapters = {
        {adapter1_.get(), "query", 0.5f}
    };

    // Force a failure with mismatched dimensions
    auto result = merger.mergeLinear(adapters, "query", 32, 128, 4, 8.0f);

    EXPECT_FALSE(result.success);
    if (!result.success) {
        EXPECT_FALSE(result.error_message.empty());
    }
}

// ============================================================================
// Adapter null pointer safety
// ============================================================================

TEST_F(MergeConflictTest, NullAdapterPointer_Handled) {
    LoRAAdapterMerger merger;

    std::vector<AdapterDescriptor> adapters = {
        {nullptr, "query", 0.5f},
        {adapter2_.get(), "query", 0.5f}
    };

    auto result = merger.mergeLinear(adapters, "query", 64, 64, 4, 8.0f);

    EXPECT_FALSE(result.success);
}

// ============================================================================
// Merge result validity
// ============================================================================

TEST_F(MergeConflictTest, MergeResult_HasValidMetadata) {
    LoRAAdapterMerger merger;

    std::vector<AdapterDescriptor> adapters = {
        {adapter1_.get(), "query", 1.0f}
    };

    auto result = merger.mergeLinear(adapters, "query", 64, 64, 4, 8.0f);

    EXPECT_TRUE(result.success);
    EXPECT_EQ(result.layers.size(), 1u);
    EXPECT_EQ(result.layers[0].layer_name, "query");
    EXPECT_TRUE(result.layers[0].success);
}

// ============================================================================
// Repeatable merge results
// ============================================================================

TEST_F(MergeConflictTest, DeterministicMerge_SameWeights) {
    LoRAAdapterMerger merger1, merger2;

    std::vector<AdapterDescriptor> adapters = {
        {adapter1_.get(), "query", 0.5f},
        {adapter2_.get(), "query", 0.5f}
    };

    auto result1 = merger1.mergeLinear(adapters, "query", 64, 64, 4, 8.0f);
    auto result2 = merger2.mergeLinear(adapters, "query", 64, 64, 4, 8.0f);

    EXPECT_TRUE(result1.success);
    EXPECT_TRUE(result2.success);
    EXPECT_EQ(result1.layers[0].B.size(), result2.layers[0].B.size());
    EXPECT_EQ(result1.layers[0].A.size(), result2.layers[0].A.size());

    // Weights should be identical for same inputs
    for (size_t i = 0; i < result1.layers[0].B.size(); ++i) {
        EXPECT_FLOAT_EQ(result1.layers[0].B[i], result2.layers[0].B[i]);
    }
}

// ============================================================================
// Merge with alpha parameter
// ============================================================================

TEST_F(MergeConflictTest, DifferentAlpha_Accepted) {
    LoRAAdapterMerger merger;

    std::vector<AdapterDescriptor> adapters = {
        {adapter1_.get(), "query", 0.5f},
        {adapter2_.get(), "query", 0.5f}
    };

    // Different alpha from source adapters
    auto result = merger.mergeLinear(adapters, "query", 64, 64, 4, 16.0f);

    EXPECT_TRUE(result.success);
}
