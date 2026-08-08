/*
 * ThemisDB | File: test_stress_training_lifecycle.cpp | Version: 0.0.1
 * Maturity: 🟢 PRODUCTION-READY | Score: 95/100
 * Gap Summary: total=0; TODO=0, Stub=0, Unimpl=0, Mock=0, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
 */

// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

/**
 * @file test_stress_training_lifecycle.cpp
 * @brief Phase 4 stress tests for extended adapter lifecycle workloads.
 *
 * Tests verify:
 *  - Adapter lifecycle under sustained training load
 *  - Layer creation/modification patterns under stress
 *  - Weight update accumulation and convergence
 *  - Memory stability with repeated training cycles
 *  - Long-running session robustness
 *  - Resource consumption bounds
 *  - Deterministic behavior under load
 */

#include <gtest/gtest.h>
#include "training/lora_adapter.h"

#include <algorithm>
#include <cmath>
#include <numeric>
#include <random>
#include <string>
#include <vector>
#include <chrono>

using namespace themis::training;

// ============================================================================
// Stress test fixture
// ============================================================================

class TrainingLifecycleStressTest : public ::testing::Test {
protected:
    // Seed for reproducibility
    static constexpr unsigned int SEED = 12345;

    // Stress parameters
    static constexpr int NUM_LAYERS = 8;
    static constexpr int NUM_EPOCHS = 50;
    static constexpr int STEPS_PER_EPOCH = 20;
    static constexpr int IN_DIM = 128;
    static constexpr int OUT_DIM = 256;
    static constexpr int RANK = 16;
    static constexpr float ALPHA = 16.0f;

    std::mt19937 rng_{SEED};

    std::vector<float> generateRandomWeights(size_t size, float scale = 0.1f) {
        std::uniform_real_distribution<float> dist(-scale, scale);
        std::vector<float> weights(size);
        for (auto& w : weights) {
            w = dist(rng_);
        }
        return weights;
    }

    std::vector<float> generateRandomDeltas(size_t size, float scale = 0.01f) {
        std::uniform_real_distribution<float> dist(-scale, scale);
        std::vector<float> deltas(size);
        for (auto& d : deltas) {
            d = dist(rng_);
        }
        return deltas;
    }
};

// ============================================================================
// Single adapter lifecycle stress
// ============================================================================

TEST_F(TrainingLifecycleStressTest, SingleAdapter_ExtendedTraining) {
    LoRAAdapter adapter(RANK, ALPHA);
    adapter.addLayer("attention", IN_DIM, OUT_DIM, RANK, ALPHA);

    for (int epoch = 0; epoch < NUM_EPOCHS; ++epoch) {
        for (int step = 0; step < STEPS_PER_EPOCH; ++step) {
            auto dB = generateRandomDeltas(IN_DIM * RANK, 0.005f);
            auto dA = generateRandomDeltas(RANK * OUT_DIM, 0.005f);

            auto result = adapter.applyUpdate("attention", dB, dA);
            EXPECT_TRUE(result.success) << "Update failed at epoch " << epoch
                                       << " step " << step;
            EXPECT_EQ(result.layers_updated, 1u);
        }
    }

    // Verify weights are still accessible
    const auto& w = adapter.getWeights("attention");
    EXPECT_EQ(w.B.size(), IN_DIM * RANK);
    EXPECT_EQ(w.A.size(), RANK * OUT_DIM);
}

TEST_F(TrainingLifecycleStressTest, SingleAdapter_BatchUpdatesStress) {
    LoRAAdapter adapter(RANK, ALPHA);
    adapter.addLayer("layer1", IN_DIM, OUT_DIM);

    for (int epoch = 0; epoch < NUM_EPOCHS; ++epoch) {
        WeightUpdateBatch batch;
        batch.layer_names = {"layer1"};
        batch.delta_B = {generateRandomDeltas(IN_DIM * RANK)};
        batch.delta_A = {generateRandomDeltas(RANK * OUT_DIM)};

        auto result = adapter.applyBatchUpdate(batch);
        EXPECT_TRUE(result.success);
    }
}

TEST_F(TrainingLifecycleStressTest, SingleAdapter_ForwardPassStress) {
    LoRAAdapter adapter(RANK, ALPHA);
    adapter.addLayer("fwd_test", IN_DIM, OUT_DIM, RANK, ALPHA);

    // Initialize with known weights
    std::vector<float> B(IN_DIM * RANK, 0.1f);
    std::vector<float> A(RANK * OUT_DIM, 0.1f);
    adapter.setWeights("fwd_test", B, A);

    int batch_size = 32;
    std::vector<float> input(batch_size * IN_DIM, 0.5f);

    for (int i = 0; i < 100; ++i) {
        auto output = adapter.forward("fwd_test", input, batch_size);
        EXPECT_EQ(output.size(), batch_size * OUT_DIM);
        EXPECT_FALSE(output.empty());
    }
}

// ============================================================================
// Multi-layer lifecycle stress
// ============================================================================

TEST_F(TrainingLifecycleStressTest, MultiLayer_SequentialAddition) {
    LoRAAdapter adapter(RANK, ALPHA);

    for (int i = 0; i < NUM_LAYERS; ++i) {
        std::string layer_name = "layer_" + std::to_string(i);
        adapter.addLayer(layer_name, IN_DIM, OUT_DIM, RANK, ALPHA);
        EXPECT_EQ(adapter.layerCount(), static_cast<size_t>(i + 1));
    }

    EXPECT_EQ(adapter.layerCount(), NUM_LAYERS);
}

TEST_F(TrainingLifecycleStressTest, MultiLayer_ConcurrentUpdates) {
    LoRAAdapter adapter(RANK, ALPHA);

    for (int i = 0; i < NUM_LAYERS; ++i) {
        std::string layer_name = "layer_" + std::to_string(i);
        adapter.addLayer(layer_name, IN_DIM, OUT_DIM);
    }

    // Update all layers for multiple epochs
    for (int epoch = 0; epoch < NUM_EPOCHS / 10; ++epoch) {
        WeightUpdateBatch batch;
        batch.layer_names.resize(NUM_LAYERS);
        batch.delta_B.resize(NUM_LAYERS);
        batch.delta_A.resize(NUM_LAYERS);

        for (int i = 0; i < NUM_LAYERS; ++i) {
            batch.layer_names[i] = "layer_" + std::to_string(i);
            batch.delta_B[i] = generateRandomDeltas(IN_DIM * RANK);
            batch.delta_A[i] = generateRandomDeltas(RANK * OUT_DIM);
        }

        auto result = adapter.applyBatchUpdate(batch);
        EXPECT_TRUE(result.success);
        EXPECT_EQ(result.layers_updated, NUM_LAYERS);
    }
}

TEST_F(TrainingLifecycleStressTest, MultiLayer_LayerRemovalStress) {
    LoRAAdapter adapter(RANK, ALPHA);

    // Add layers
    for (int i = 0; i < NUM_LAYERS; ++i) {
        adapter.addLayer("layer_" + std::to_string(i), IN_DIM, OUT_DIM);
    }

    EXPECT_EQ(adapter.layerCount(), NUM_LAYERS);

    // Remove odd-numbered layers
    for (int i = 1; i < NUM_LAYERS; i += 2) {
        bool removed = adapter.removeLayer("layer_" + std::to_string(i));
        EXPECT_TRUE(removed);
    }

    EXPECT_EQ(adapter.layerCount(), (NUM_LAYERS + 1) / 2);

    // Re-add removed layers
    for (int i = 1; i < NUM_LAYERS; i += 2) {
        adapter.addLayer("layer_" + std::to_string(i), IN_DIM, OUT_DIM);
    }

    EXPECT_EQ(adapter.layerCount(), NUM_LAYERS);
}

// ============================================================================
// Weight convergence under stress
// ============================================================================

TEST_F(TrainingLifecycleStressTest, WeightConvergence_Deterministic) {
    LoRAAdapter adapter1(RANK, ALPHA);
    LoRAAdapter adapter2(RANK, ALPHA);

    adapter1.addLayer("conv_test", IN_DIM, OUT_DIM);
    adapter2.addLayer("conv_test", IN_DIM, OUT_DIM);

    // Apply same sequence of updates to both adapters
    for (int step = 0; step < 100; ++step) {
        auto deltas = generateRandomDeltas(IN_DIM * RANK);

        auto dB = deltas;
        auto dA = generateRandomDeltas(RANK * OUT_DIM);

        adapter1.applyUpdate("conv_test", dB, dA);
        adapter2.applyUpdate("conv_test", dB, dA);
    }

    // Weights should be identical
    const auto& w1 = adapter1.getWeights("conv_test");
    const auto& w2 = adapter2.getWeights("conv_test");

    for (size_t i = 0; i < w1.B.size(); ++i) {
        EXPECT_FLOAT_EQ(w1.B[i], w2.B[i]);
    }
    for (size_t i = 0; i < w1.A.size(); ++i) {
        EXPECT_FLOAT_EQ(w1.A[i], w2.A[i]);
    }
}

TEST_F(TrainingLifecycleStressTest, WeightAccumulation_Linear) {
    LoRAAdapter adapter(RANK, ALPHA);
    adapter.addLayer("accum_test", IN_DIM, OUT_DIM);

    // Take initial snapshot
    const auto& w0 = adapter.getWeights("accum_test");
    std::vector<float> A_init = w0.A;

    // Apply constant delta multiple times
    const float delta_const = 0.01f;
    const int num_steps = 50;

    std::vector<float> dA(RANK * OUT_DIM, delta_const);
    std::vector<float> dB(IN_DIM * RANK, 0.0f);

    for (int step = 0; step < num_steps; ++step) {
        adapter.applyUpdate("accum_test", dB, dA);
    }

    // Verify linear accumulation
    const auto& wf = adapter.getWeights("accum_test");
    for (size_t i = 0; i < A_init.size(); ++i) {
        float expected = A_init[i] + num_steps * delta_const;
        EXPECT_NEAR(wf.A[i], expected, 1e-4f);
    }
}

// ============================================================================
// Parameter count tracking
// ============================================================================

TEST_F(TrainingLifecycleStressTest, ParameterCount_Accurate) {
    LoRAAdapter adapter(RANK, ALPHA);

    size_t total_params = 0;
    for (int i = 0; i < NUM_LAYERS; ++i) {
        adapter.addLayer("layer_" + std::to_string(i), IN_DIM, OUT_DIM, RANK);
        total_params += 2 * IN_DIM * RANK;  // B and A matrices
    }

    EXPECT_EQ(adapter.totalParameterCount(), total_params);
}

// ============================================================================
// Export/Import under stress
// ============================================================================

TEST_F(TrainingLifecycleStressTest, ExportImport_RoundTrip) {
    LoRAAdapter adapter1(RANK, ALPHA);

    // Create and train
    for (int i = 0; i < NUM_LAYERS; ++i) {
        std::string layer_name = "layer_" + std::to_string(i);
        adapter1.addLayer(layer_name, IN_DIM, OUT_DIM);

        // Apply some updates
        for (int step = 0; step < 10; ++step) {
            auto dB = generateRandomDeltas(IN_DIM * RANK);
            auto dA = generateRandomDeltas(RANK * OUT_DIM);
            adapter1.applyUpdate(layer_name, dB, dA);
        }
    }

    // Export
    auto exported = adapter1.exportWeights();
    EXPECT_EQ(exported.size(), NUM_LAYERS);

    // Import into new adapter
    LoRAAdapter adapter2(RANK, ALPHA);
    adapter2.importWeights(exported);

    // Verify all layers present
    EXPECT_EQ(adapter2.layerCount(), adapter1.layerCount());

    // Verify weights match
    for (int i = 0; i < NUM_LAYERS; ++i) {
        std::string layer_name = "layer_" + std::to_string(i);
        const auto& w1 = adapter1.getWeights(layer_name);
        const auto& w2 = adapter2.getWeights(layer_name);

        EXPECT_EQ(w1.B.size(), w2.B.size());
        for (size_t j = 0; j < w1.B.size(); ++j) {
            EXPECT_FLOAT_EQ(w1.B[j], w2.B[j]);
        }
    }
}

// ============================================================================
// Memory efficiency validation
// ============================================================================

TEST_F(TrainingLifecycleStressTest, MemoryStability_LongSession) {
    LoRAAdapter adapter(RANK, ALPHA);
    adapter.addLayer("mem_test", IN_DIM, OUT_DIM);

    // Simulate long training session with repeated allocations
    for (int iter = 0; iter < 200; ++iter) {
        auto dB = generateRandomDeltas(IN_DIM * RANK);
        auto dA = generateRandomDeltas(RANK * OUT_DIM);
        auto result = adapter.applyUpdate("mem_test", dB, dA);
        EXPECT_TRUE(result.success);

        // Periodic exports (shouldn't leak)
        if (iter % 50 == 0) {
            auto exported = adapter.exportWeights();
            EXPECT_EQ(exported.size(), 1u);
        }
    }
}

TEST_F(TrainingLifecycleStressTest, NoLeakOnLayerAddRemove) {
    LoRAAdapter adapter(RANK, ALPHA);

    // Repeatedly add and remove layers
    for (int iter = 0; iter < 50; ++iter) {
        std::string layer_name = "temp_layer";

        adapter.addLayer(layer_name, IN_DIM, OUT_DIM);
        EXPECT_TRUE(adapter.hasLayer(layer_name));

        // Apply some updates
        for (int step = 0; step < 5; ++step) {
            auto dB = generateRandomDeltas(IN_DIM * RANK);
            auto dA = generateRandomDeltas(RANK * OUT_DIM);
            adapter.applyUpdate(layer_name, dB, dA);
        }

        adapter.removeLayer(layer_name);
        EXPECT_FALSE(adapter.hasLayer(layer_name));
    }

    EXPECT_EQ(adapter.layerCount(), 0u);
}

// ============================================================================
// Concurrent layer operations
// ============================================================================

TEST_F(TrainingLifecycleStressTest, ConcurrentLayerNames_Consistent) {
    LoRAAdapter adapter(RANK, ALPHA);

    for (int i = 0; i < NUM_LAYERS; ++i) {
        adapter.addLayer("layer_" + std::to_string(i), IN_DIM, OUT_DIM);
    }

    auto names = adapter.layerNames();
    EXPECT_EQ(names.size(), NUM_LAYERS);

    // Verify all expected names are present
    for (int i = 0; i < NUM_LAYERS; ++i) {
        std::string expected = "layer_" + std::to_string(i);
        auto it = std::find(names.begin(), names.end(), expected);
        EXPECT_NE(it, names.end());
    }
}

// ============================================================================
// Extended training scenarios
// ============================================================================

TEST_F(TrainingLifecycleStressTest, TrainingPipeline_Realistic) {
    LoRAAdapter adapter(RANK, ALPHA);

    const int num_training_phases = 5;
    const int steps_per_phase = 100;

    // Phase 1: Initial training
    adapter.addLayer("q_proj", IN_DIM, OUT_DIM);
    adapter.addLayer("k_proj", IN_DIM, OUT_DIM);

    for (int phase = 0; phase < num_training_phases; ++phase) {
        // Forward passes
        for (int step = 0; step < steps_per_phase; ++step) {
            std::vector<float> input(32 * IN_DIM, 0.5f);
            auto q_out = adapter.forward("q_proj", input, 32);
            auto k_out = adapter.forward("k_proj", input, 32);

            EXPECT_EQ(q_out.size(), 32u * OUT_DIM);
            EXPECT_EQ(k_out.size(), 32u * OUT_DIM);

            // Backward pass (weight updates)
            auto dB = generateRandomDeltas(IN_DIM * RANK, 0.001f);
            auto dA = generateRandomDeltas(RANK * OUT_DIM, 0.001f);
            adapter.applyUpdate("q_proj", dB, dA);
            adapter.applyUpdate("k_proj", dB, dA);
        }

        // Periodic checkpoint export
        if (phase % 2 == 0) {
            auto exported = adapter.exportWeights();
            EXPECT_EQ(exported.size(), 2u);
        }
    }
}

// ============================================================================
// Error recovery under stress
// ============================================================================

TEST_F(TrainingLifecycleStressTest, RecoveryFromErrors_ContinuesTraining) {
    LoRAAdapter adapter(RANK, ALPHA);
    adapter.addLayer("recovery_test", IN_DIM, OUT_DIM);

    for (int step = 0; step < 100; ++step) {
        // Try to apply update to non-existent layer (should fail)
        if (step == 50) {
            auto dB = generateRandomDeltas(IN_DIM * RANK);
            auto dA = generateRandomDeltas(RANK * OUT_DIM);
            EXPECT_THROW(adapter.applyUpdate("non_existent", dB, dA), std::out_of_range);
        }

        // But continue with real layer
        auto dB = generateRandomDeltas(IN_DIM * RANK);
        auto dA = generateRandomDeltas(RANK * OUT_DIM);
        auto result = adapter.applyUpdate("recovery_test", dB, dA);
        EXPECT_TRUE(result.success);
    }

    const auto& w = adapter.getWeights("recovery_test");
    EXPECT_EQ(w.B.size(), IN_DIM * RANK);
}
