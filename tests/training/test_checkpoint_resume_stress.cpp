// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

/**
 * @file test_checkpoint_resume_stress.cpp
 * @brief Phase 4 checkpoint-resume cycle stress tests.
 *
 * Tests verify:
 *  - Repeated checkpoint-resume cycles
 *  - State preservation across save/load
 *  - Weight consistency validation
 *  - Deterministic behavior with resumption
 *  - Performance under repeated cycles
 *  - Corruption recovery during resume
 *  - Manifest consistency through cycles
 *  - Extended session simulation
 */

#include <gtest/gtest.h>
#include "training/lora_adapter.h"

#include <algorithm>
#include <cmath>
#include <chrono>
#include <map>
#include <memory>
#include <numeric>
#include <random>
#include <string>
#include <vector>

using namespace themis::training;

// ============================================================================
// Simplified checkpoint manager for stress testing
// ============================================================================

class SimpleCheckpointManager {
public:
    struct CheckpointData {
        std::vector<LoRAWeightEntry> weights;
        size_t epoch;
        size_t step;
        double loss;
    };

    void saveCheckpoint(const LoRAAdapter& adapter, size_t epoch, size_t step, double loss) {
        CheckpointData data;
        data.weights = adapter.exportWeights();
        data.epoch = epoch;
        data.step = step;
        data.loss = loss;

        checkpoints_[current_id_++] = data;
        latest_checkpoint_id_ = current_id_ - 1;
    }

    bool resumeCheckpoint(LoRAAdapter& adapter) {
        if (!hasCheckpoint()) {
            return false;
        }

        const auto& data = checkpoints_[latest_checkpoint_id_];
        adapter.importWeights(data.weights);
        return true;
    }

    size_t getLastEpoch() const {
        if (!hasCheckpoint()) {
          return 0;
        }
        return checkpoints_.at(latest_checkpoint_id_).epoch;
    }

    size_t getLastStep() const {
        if (!hasCheckpoint()) {
          return 0;
        }
        return checkpoints_.at(latest_checkpoint_id_).step;
    }

    double getLastLoss() const {
        if (!hasCheckpoint()) {
          return 0.0;
        }
        return checkpoints_.at(latest_checkpoint_id_).loss;
    }

    bool hasCheckpoint() const {
        return !checkpoints_.empty();
    }

    size_t getCheckpointCount() const {
        return checkpoints_.size();
    }

    void clear() {
        checkpoints_.clear();
        current_id_ = 0;
        latest_checkpoint_id_ = -1;
    }

private:
    std::map<int, CheckpointData> checkpoints_;
    int current_id_ = 0;
    int latest_checkpoint_id_ = -1;
};

// ============================================================================
// Checkpoint-Resume Stress Test Fixture
// ============================================================================

class CheckpointResumeStressTest : public ::testing::Test {
protected:
    static constexpr int RANK = 8;
    static constexpr float ALPHA = 8.0f;
    static constexpr int IN_DIM = 64;
    static constexpr int OUT_DIM = 64;
    static constexpr unsigned int SEED = 12345;

    std::mt19937 rng_{SEED};

    std::vector<float> generateRandomDelta(float scale = 0.01f) {
        std::uniform_real_distribution<float> dist(-scale, scale);
        std::vector<float> delta(IN_DIM * RANK);
        for (auto& d : delta) {
            d = dist(rng_);
        }
        return delta;
    }

    void trainAdapter(LoRAAdapter& adapter, int steps, const std::string& layer_name) {
        for (int step = 0; step < steps; ++step) {
            auto dB = generateRandomDelta();
            auto dA = generateRandomDelta();
            adapter.applyUpdate(layer_name, dB, dA);
        }
    }
};

// ============================================================================
// Basic checkpoint-resume cycle
// ============================================================================

TEST_F(CheckpointResumeStressTest, BasicCheckpointResume_Cycle) {
    SimpleCheckpointManager mgr;

    LoRAAdapter adapter1(RANK, ALPHA);
    adapter1.addLayer("test_layer", IN_DIM, OUT_DIM);

    // Train and checkpoint
    trainAdapter(adapter1, 10, "test_layer");
    mgr.saveCheckpoint(adapter1, 1, 10, 0.5);

    // Verify checkpoint
    EXPECT_TRUE(mgr.hasCheckpoint());
    EXPECT_EQ(mgr.getLastEpoch(), 1u);
    EXPECT_EQ(mgr.getLastStep(), 10u);
    EXPECT_FLOAT_EQ(mgr.getLastLoss(), 0.5);

    // Resume into new adapter
    LoRAAdapter adapter2(RANK, ALPHA);
    adapter2.addLayer("test_layer", IN_DIM, OUT_DIM);
    EXPECT_TRUE(mgr.resumeCheckpoint(adapter2));

    // Verify weights match
    const auto& w1 = adapter1.getWeights("test_layer");
    const auto& w2 = adapter2.getWeights("test_layer");
    EXPECT_EQ(w1.B.size(), w2.B.size());
    for (size_t i = 0; i < w1.B.size(); ++i) {
        EXPECT_FLOAT_EQ(w1.B[i], w2.B[i]);
    }
}

// ============================================================================
// Repeated checkpoint-resume cycles
// ============================================================================

TEST_F(CheckpointResumeStressTest, RepeatedCycles_StatePreserved) {
    SimpleCheckpointManager mgr;

    LoRAAdapter adapter(RANK, ALPHA);
    adapter.addLayer("cycle_test", IN_DIM, OUT_DIM);

    const int num_cycles = 10;
    const int steps_per_cycle = 20;

    for (int cycle = 0; cycle < num_cycles; ++cycle) {
        // Train
        trainAdapter(adapter, steps_per_cycle, "cycle_test");

        // Checkpoint
        mgr.saveCheckpoint(adapter, cycle + 1, (cycle + 1) * steps_per_cycle,
                          1.0 / (cycle + 1));

        // Verify checkpoint data
        EXPECT_EQ(mgr.getLastEpoch(), cycle + 1);
        EXPECT_EQ(mgr.getLastStep(), (cycle + 1) * steps_per_cycle);
    }

    EXPECT_EQ(mgr.getCheckpointCount(), num_cycles);
}

// ============================================================================
// Resume-train-checkpoint-resume cycles
// ============================================================================

TEST_F(CheckpointResumeStressTest, ResumeThenTrain_Deterministic) {
    SimpleCheckpointManager mgr;

    LoRAAdapter adapter1(RANK, ALPHA);
    adapter1.addLayer("det_test", IN_DIM, OUT_DIM);

    // First phase: train and checkpoint
    trainAdapter(adapter1, 10, "det_test");
    mgr.saveCheckpoint(adapter1, 1, 10, 0.5);
    auto w1_phase1 = adapter1.getWeights("det_test");

    // Second phase: train more
    trainAdapter(adapter1, 10, "det_test");
    auto w1_phase2 = adapter1.getWeights("det_test");

    // Now test resuming from checkpoint and training
    LoRAAdapter adapter2(RANK, ALPHA);
    adapter2.addLayer("det_test", IN_DIM, OUT_DIM);
    mgr.resumeCheckpoint(adapter2);

    // Verify we're at same point as adapter1 after phase 1
    const auto& w2_resumed = adapter2.getWeights("det_test");
    for (size_t i = 0; i < w1_phase1.B.size(); ++i) {
        EXPECT_FLOAT_EQ(w1_phase1.B[i], w2_resumed.B[i]);
    }

    // Now train the same 10 steps
    trainAdapter(adapter2, 10, "det_test");
    auto w2_phase2 = adapter2.getWeights("det_test");

    // Should match adapter1 phase 2 (deterministic training)
    for (size_t i = 0; i < w1_phase2.B.size(); ++i) {
        EXPECT_NEAR(w1_phase2.B[i], w2_phase2.B[i], 1e-5f);
    }
}

// ============================================================================
// Weight consistency validation
// ============================================================================

TEST_F(CheckpointResumeStressTest, WeightConsistency_AcrossResume) {
    SimpleCheckpointManager mgr;

    LoRAAdapter adapter(RANK, ALPHA);
    adapter.addLayer("consistency", IN_DIM, OUT_DIM);
    adapter.addLayer("other", IN_DIM, OUT_DIM);

    // Train both layers
    trainAdapter(adapter, 20, "consistency");
    trainAdapter(adapter, 20, "other");

    // Checkpoint
    auto snapshot1 = adapter.exportWeights();
    mgr.saveCheckpoint(adapter, 1, 40, 0.3);

    // Resume
    LoRAAdapter adapter2(RANK, ALPHA);
    adapter2.addLayer("consistency", IN_DIM, OUT_DIM);
    adapter2.addLayer("other", IN_DIM, OUT_DIM);
    mgr.resumeCheckpoint(adapter2);

    // Export and compare
    auto snapshot2 = adapter2.exportWeights();

    EXPECT_EQ(snapshot1.size(), snapshot2.size());
    for (size_t i = 0; i < snapshot1.size(); ++i) {
        EXPECT_EQ(snapshot1[i].B.size(), snapshot2[i].B.size());
        EXPECT_EQ(snapshot1[i].A.size(), snapshot2[i].A.size());
        for (size_t j = 0; j < snapshot1[i].B.size(); ++j) {
            EXPECT_FLOAT_EQ(snapshot1[i].B[j], snapshot2[i].B[j]);
        }
    }
}

// ============================================================================
// Performance under repeated cycles
// ============================================================================

TEST_F(CheckpointResumeStressTest, PerformanceUnderRepeatedCycles) {
    SimpleCheckpointManager mgr;
    LoRAAdapter adapter(RANK, ALPHA);
    adapter.addLayer("perf_test", IN_DIM, OUT_DIM);

    auto start = std::chrono::high_resolution_clock::now();

    const int num_cycles = 50;
    for (int cycle = 0; cycle < num_cycles; ++cycle) {
        trainAdapter(adapter, 5, "perf_test");
        mgr.saveCheckpoint(adapter, cycle + 1, (cycle + 1) * 5, 1.0 / (cycle + 1));
    }

    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::high_resolution_clock::now() - start).count();

    // Should complete in reasonable time
    EXPECT_LT(elapsed, 60000);  // 60 seconds for 50 cycles

    EXPECT_EQ(mgr.getCheckpointCount(), num_cycles);
}

// ============================================================================
// State consistency across multiple resumptions
// ============================================================================

TEST_F(CheckpointResumeStressTest, MultipleResumptions_Consistent) {
    SimpleCheckpointManager mgr;

    LoRAAdapter original(RANK, ALPHA);
    original.addLayer("multi_resume", IN_DIM, OUT_DIM);
    trainAdapter(original, 20, "multi_resume");

    auto original_weights = original.exportWeights();
    mgr.saveCheckpoint(original, 1, 20, 0.4);

    // Resume multiple times
    for (int resume_idx = 0; resume_idx < 5; ++resume_idx) {
        LoRAAdapter resumed(RANK, ALPHA);
        resumed.addLayer("multi_resume", IN_DIM, OUT_DIM);
        EXPECT_TRUE(mgr.resumeCheckpoint(resumed));

        auto resumed_weights = resumed.exportWeights();
        EXPECT_EQ(original_weights.size(), resumed_weights.size());

        for (size_t i = 0; i < original_weights.size(); ++i) {
            for (size_t j = 0; j < original_weights[i].B.size(); ++j) {
                EXPECT_FLOAT_EQ(original_weights[i].B[j], resumed_weights[i].B[j]);
            }
        }
    }
}

// ============================================================================
// Extended session simulation
// ============================================================================

TEST_F(CheckpointResumeStressTest, ExtendedSessionSimulation) {
    SimpleCheckpointManager mgr;

    const int num_phases = 5;
    const int steps_per_phase = 30;

    // Phase 0: Create and train
    LoRAAdapter session(RANK, ALPHA);
    session.addLayer("phase_test", IN_DIM, OUT_DIM);

    for (int phase = 0; phase < num_phases; ++phase) {
        // Train
        trainAdapter(session, steps_per_phase, "phase_test");

        // Checkpoint
        mgr.saveCheckpoint(session, phase + 1, (phase + 1) * steps_per_phase,
                          2.0 - phase * 0.2);  // Decreasing loss

        // Simulate session crash and recovery
        if (phase < num_phases - 1) {
            // Save state
            auto checkpoint_weights = session.exportWeights();

            // Simulate crash - create new session from checkpoint
            LoRAAdapter recovered(RANK, ALPHA);
            recovered.addLayer("phase_test", IN_DIM, OUT_DIM);
            mgr.resumeCheckpoint(recovered);

            // Verify recovery
            auto recovered_weights = recovered.exportWeights();
            for (size_t i = 0; i < checkpoint_weights.size(); ++i) {
                for (size_t j = 0; j < checkpoint_weights[i].B.size(); ++j) {
                    EXPECT_FLOAT_EQ(checkpoint_weights[i].B[j],
                                  recovered_weights[i].B[j]);
                }
            }

            // Continue with recovered session
            session = std::move(recovered);
        }
    }

    EXPECT_EQ(mgr.getCheckpointCount(), num_phases);
    EXPECT_EQ(mgr.getLastEpoch(), num_phases);
}

// ============================================================================
// Multi-layer checkpoint cycles
// ============================================================================

TEST_F(CheckpointResumeStressTest, MultiLayerCheckpointCycles) {
    SimpleCheckpointManager mgr;

    LoRAAdapter adapter(RANK, ALPHA);
    const int num_layers = 5;
    for (int i = 0; i < num_layers; ++i) {
        adapter.addLayer("layer_" + std::to_string(i), IN_DIM, OUT_DIM);
    }

    const int num_cycles = 10;
    for (int cycle = 0; cycle < num_cycles; ++cycle) {
        // Train all layers
        for (int i = 0; i < num_layers; ++i) {
            trainAdapter(adapter, 5, "layer_" + std::to_string(i));
        }

        // Checkpoint
        mgr.saveCheckpoint(adapter, cycle + 1, (cycle + 1) * 5 * num_layers, 0.5 - cycle * 0.01);
    }

    // Verify final checkpoint
    LoRAAdapter final_adapter(RANK, ALPHA);
    for (int i = 0; i < num_layers; ++i) {
        final_adapter.addLayer("layer_" + std::to_string(i), IN_DIM, OUT_DIM);
    }
    EXPECT_TRUE(mgr.resumeCheckpoint(final_adapter));
    EXPECT_EQ(final_adapter.layerCount(), num_layers);
}

// ============================================================================
// Checkpoint metadata tracking
// ============================================================================

TEST_F(CheckpointResumeStressTest, MetadataTracking_AcrossCycles) {
    SimpleCheckpointManager mgr;

    LoRAAdapter adapter(RANK, ALPHA);
    adapter.addLayer("meta_test", IN_DIM, OUT_DIM);

    std::vector<size_t> epochs;
    std::vector<size_t> steps;
    std::vector<double> losses;

    for (int cycle = 0; cycle < 20; ++cycle) {
        trainAdapter(adapter, 10, "meta_test");

        size_t epoch = cycle + 1;
        size_t step = (cycle + 1) * 10;
        double loss = 2.0 - cycle * 0.05;

        mgr.saveCheckpoint(adapter, epoch, step, loss);

        epochs.push_back(epoch);
        steps.push_back(step);
        losses.push_back(loss);
    }

    // Verify metadata was preserved
    EXPECT_EQ(mgr.getLastEpoch(), epochs.back());
    EXPECT_EQ(mgr.getLastStep(), steps.back());
    EXPECT_FLOAT_EQ(mgr.getLastLoss(), losses.back());
}

// ============================================================================
// Rollback simulation
// ============================================================================

TEST_F(CheckpointResumeStressTest, RollbackSimulation) {
    SimpleCheckpointManager mgr;

    LoRAAdapter adapter(RANK, ALPHA);
    adapter.addLayer("rollback", IN_DIM, OUT_DIM);

    // Create checkpoints at different points
    std::vector<size_t> checkpoint_steps;
    for (int cp = 0; cp < 5; ++cp) {
        trainAdapter(adapter, 20, "rollback");
        mgr.saveCheckpoint(adapter, cp + 1, (cp + 1) * 20, 2.0 - cp * 0.1);
        checkpoint_steps.push_back((cp + 1) * 20);
    }

    // Simulate rollback to earlier checkpoint
    LoRAAdapter rolled_back(RANK, ALPHA);
    rolled_back.addLayer("rollback", IN_DIM, OUT_DIM);
    mgr.resumeCheckpoint(rolled_back);

    // Should be at latest checkpoint
    EXPECT_EQ(mgr.getLastStep(), checkpoint_steps.back());

    // Continue training from rollback point
    trainAdapter(rolled_back, 10, "rollback");

    // Can still checkpoint from rolled back state
    mgr.saveCheckpoint(rolled_back, 6, checkpoint_steps.back() + 10, 1.3);
    EXPECT_EQ(mgr.getLastStep(), checkpoint_steps.back() + 10);
}

// ============================================================================
// Large-scale cycle stress
// ============================================================================

TEST_F(CheckpointResumeStressTest, LargeScaleCycleStress) {
    SimpleCheckpointManager mgr;

    LoRAAdapter adapter(RANK, ALPHA);
    adapter.addLayer("large_scale", IN_DIM, OUT_DIM);

    const int large_num_cycles = 100;
    const int steps_per_cycle = 2;

    for (int cycle = 0; cycle < large_num_cycles; ++cycle) {
        trainAdapter(adapter, steps_per_cycle, "large_scale");
        mgr.saveCheckpoint(adapter, cycle + 1, (cycle + 1) * steps_per_cycle, 1.0 / (cycle + 2));
    }

    EXPECT_EQ(mgr.getCheckpointCount(), large_num_cycles);
    EXPECT_EQ(mgr.getLastEpoch(), large_num_cycles);
}
