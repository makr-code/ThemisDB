// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

#include <gtest/gtest.h>

#include "training/ada_lora_adapter.h"

#include <vector>

using namespace themis::training;

TEST(TrainingAdaLoRAEdgeRegressions, AddLayerDeterministicInitializationByName) {
    AdaLoRAAdapter first(4, 8.0f, 16);
    AdaLoRAAdapter second(4, 8.0f, 16);

    first.addLayer("stable_layer", 16, 16, 4, 8.0f);
    second.addLayer("stable_layer", 16, 16, 4, 8.0f);

    const auto [b1, a1] = first.getWeights("stable_layer");
    const auto [b2, a2] = second.getWeights("stable_layer");

    ASSERT_EQ(b1.size(), b2.size());
    ASSERT_EQ(a1.size(), a2.size());
    for (size_t i = 0; i < b1.size(); ++i) {
        EXPECT_FLOAT_EQ(b1[i], b2[i]);
    }
    for (size_t i = 0; i < a1.size(); ++i) {
        EXPECT_FLOAT_EQ(a1[i], a2[i]);
    }
}

TEST(TrainingAdaLoRAEdgeRegressions, ReallocateRanksBudgetBelowLayerCountThrows) {
    AdaLoRAAdapter adapter(4, 8.0f, 16);
    adapter.addLayer("layer_1", 32, 32, 4, 8.0f);
    adapter.addLayer("layer_2", 32, 32, 4, 8.0f);
    adapter.addLayer("layer_3", 32, 32, 4, 8.0f);

    EXPECT_THROW(adapter.reallocateRanks(2), std::invalid_argument);
}

TEST(TrainingAdaLoRAEdgeRegressions, ReallocateRanksZeroImportanceDeterministicDistribution) {
    AdaLoRAAdapter adapter(4, 8.0f, 16);
    adapter.addLayer("layer_a", 32, 32, 4, 8.0f);
    adapter.addLayer("layer_b", 32, 32, 4, 8.0f);
    adapter.addLayer("layer_c", 32, 32, 4, 8.0f);

    const auto result = adapter.reallocateRanks(5);
    EXPECT_TRUE(result.success);
    EXPECT_EQ(result.total_active_rank, 5u);
    EXPECT_EQ(adapter.getActiveRank("layer_a"), 2u);
    EXPECT_EQ(adapter.getActiveRank("layer_b"), 2u);
    EXPECT_EQ(adapter.getActiveRank("layer_c"), 1u);
}
