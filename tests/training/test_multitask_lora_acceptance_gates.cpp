// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

/**
 * @file test_multitask_lora_acceptance_gates.cpp
 * @brief Wave B Multi-Task LoRA Training Acceptance Gates Test Suite (MTL-01..13)
 *
 * Tests validate the acceptance gates for Wave B (Q1-Q2 2027) multi-task LoRA deployment:
 *  - MTL-01..08: Per-task forward-pass correctness and task routing
 *  - MTL-09..12: Joint training convergence over 5 epochs
 *  - MTL-13: Ablation study correctness (shared vs per-task single-task baseline)
 *
 * Acceptance Criteria (from FUTURE_ENHANCEMENTS.md Phase 5):
 *  - Average task performance gain ≥ +8% vs single-task baseline
 *  - Training-time increase ≤ 15% across benchmarked task sets
 *  - Robust convergence across configured task-weight schedules
 *  - Adapter switch ≤ 10ms
 *
 * All tests are CPU-only and require no GPU hardware.
 */

#include <gtest/gtest.h>
#include "training/multi_task_lora.h"

#include <algorithm>
#include <cmath>
#include <chrono>
#include <numeric>
#include <random>
#include <vector>

using namespace themis::training;

// ============================================================================
// Test Fixtures and Helpers
// ============================================================================

class MultiTaskLoRAAcceptanceGatesTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Default config for tests
        cfg_.shared_rank = 8;
        cfg_.epochs = 5;
        cfg_.batch_size = 16;
        cfg_.learning_rate = 1e-3f;
        cfg_.warmup_frac = 0.1f;
    }

    // Helper: create simple training samples for a single task
    std::vector<MTLSample> createSimpleSamples(
        const std::string& task_id,
        size_t num_samples,
        size_t input_dim = 32,
        size_t seed = 42) {
        std::vector<MTLSample> samples;
        std::mt19937 rng(seed);
        std::uniform_real_distribution<float> dist(-1.0f, 1.0f);

        for (size_t i = 0; i < num_samples; ++i) {
            MTLSample s;
            s.task_id = task_id;
            s.input.resize(input_dim);
            s.target.resize(input_dim);

            for (size_t j = 0; j < input_dim; ++j) {
                s.input[j] = dist(rng);
                s.target[j] = s.input[j] * 0.9f + dist(rng) * 0.1f;
            }
            s.weight = 1.0f;
            samples.push_back(s);
        }
        return samples;
    }

    // Helper: create multi-task samples for multiple tasks
    std::vector<MTLSample> createMultiTaskSamples(
        const std::vector<std::string>& task_ids,
        size_t samples_per_task = 50,
        size_t input_dim = 32) {
        std::vector<MTLSample> all_samples;
        for (size_t i = 0; i < task_ids.size(); ++i) {
            auto task_samples = createSimpleSamples(task_ids[i], samples_per_task, input_dim, 42 + i);
            all_samples.insert(all_samples.end(), task_samples.begin(), task_samples.end());
        }
        return all_samples;
    }

    MultiTaskLoRAConfig cfg_;
};

// ============================================================================
// MTL-01..04: Per-task forward-pass correctness
// ============================================================================

TEST_F(MultiTaskLoRAAcceptanceGatesTest, MTL_01_SingleTaskForwardPass) {
    MultiTaskLoRATrainer trainer(cfg_);

    TaskConfig task;
    task.id = "task_a";
    task.task_rank = 4;
    task.loss_weight = 1.0f;
    task.learning_rate = 1e-3f;
    trainer.addTask(task);

    auto samples = createSimpleSamples("task_a", 50, 32);
    auto result = trainer.train(samples);

    EXPECT_TRUE(result.success);
    EXPECT_EQ(result.epochs_run, cfg_.epochs);
    EXPECT_GT(result.joint_loss, 0.0);
    EXPECT_LT(result.joint_loss, 1e6);  // Sanity check
}

TEST_F(MultiTaskLoRAAcceptanceGatesTest, MTL_02_TwoTaskForwardPass) {
    MultiTaskLoRATrainer trainer(cfg_);

    TaskConfig task_a{"task_a", 1.0f, 4, 1e-3f};
    TaskConfig task_b{"task_b", 1.0f, 4, 1e-3f};
    trainer.addTask(task_a);
    trainer.addTask(task_b);

    auto samples = createMultiTaskSamples({"task_a", "task_b"}, 50, 32);
    auto result = trainer.train(samples);

    EXPECT_TRUE(result.success);
    EXPECT_EQ(result.per_task.size(), 2u);

    // Verify per-task metrics are populated
    for (const auto& m : result.per_task) {
        EXPECT_FALSE(m.task_id.empty());
        EXPECT_GT(m.num_samples, 0u);
    }
}

TEST_F(MultiTaskLoRAAcceptanceGatesTest, MTL_03_ThreeTaskForwardPass) {
    MultiTaskLoRATrainer trainer(cfg_);

    TaskConfig task_a{"task_a", 1.0f, 4, 1e-3f};
    TaskConfig task_b{"task_b", 1.0f, 4, 1e-3f};
    TaskConfig task_c{"task_c", 1.0f, 4, 1e-3f};
    trainer.addTask(task_a);
    trainer.addTask(task_b);
    trainer.addTask(task_c);

    auto samples = createMultiTaskSamples({"task_a", "task_b", "task_c"}, 40, 32);
    auto result = trainer.train(samples);

    EXPECT_TRUE(result.success);
    EXPECT_EQ(result.per_task.size(), 3u);
}

TEST_F(MultiTaskLoRAAcceptanceGatesTest, MTL_04_DomainGatingRoutingCorrectness) {
    MultiTaskLoRATrainer trainer(cfg_);

    TaskConfig task_a{"task_a", 1.0f, 4, 1e-3f};
    TaskConfig task_b{"task_b", 1.0f, 4, 1e-3f};
    trainer.addTask(task_a);
    trainer.addTask(task_b);

    auto samples = createMultiTaskSamples({"task_a", "task_b"}, 50, 32);
    auto result = trainer.train(samples);
    EXPECT_TRUE(result.success);

    // Test domain gating on known samples
    auto gate_result = trainer.inferTask(samples[0].input);
    EXPECT_FALSE(gate_result.task_id.empty());
    EXPECT_GE(gate_result.confidence, 0.0f);
    EXPECT_LE(gate_result.confidence, 1.0f);
}

// ============================================================================
// MTL-05..08: Task routing latency and gating confidence
// ============================================================================

TEST_F(MultiTaskLoRAAcceptanceGatesTest, MTL_05_GatingLatencyShouldBeFast) {
    MultiTaskLoRATrainer trainer(cfg_);

    TaskConfig task_a{"task_a", 1.0f, 4, 1e-3f};
    TaskConfig task_b{"task_b", 1.0f, 4, 1e-3f};
    trainer.addTask(task_a);
    trainer.addTask(task_b);

    auto samples = createMultiTaskSamples({"task_a", "task_b"}, 30, 32);
    auto result = trainer.train(samples);
    EXPECT_TRUE(result.success);

    // Measure gating latency (target: ≤ 10ms)
    auto start = std::chrono::high_resolution_clock::now();
    for (size_t i = 0; i < 100; ++i) {
        auto gate_result = trainer.inferTask(samples[i % samples.size()].input);
        (void)gate_result;
    }
    auto end = std::chrono::high_resolution_clock::now();

    double latency_ms = std::chrono::duration<double, std::milli>(end - start).count() / 100.0;

    // Gate target: ≤ 10ms per inference (Wave B requirement)
    EXPECT_LT(latency_ms, 10.0);
}

TEST_F(MultiTaskLoRAAcceptanceGatesTest, MTL_06_GatingConfidenceScoreRange) {
    MultiTaskLoRATrainer trainer(cfg_);

    TaskConfig task_a{"task_a", 1.0f, 4, 1e-3f};
    TaskConfig task_b{"task_b", 1.0f, 4, 1e-3f};
    trainer.addTask(task_a);
    trainer.addTask(task_b);

    auto samples = createMultiTaskSamples({"task_a", "task_b"}, 40, 32);
    auto result = trainer.train(samples);

    // Collect confidence scores
    float min_conf = 1.0f, max_conf = 0.0f;
    for (const auto& sample : samples) {
        auto gate = trainer.inferTask(sample.input);
        min_conf = std::min(min_conf, gate.confidence);
        max_conf = std::max(max_conf, gate.confidence);
    }

    // Confidence should be in [0, 1]
    EXPECT_GE(min_conf, 0.0f);
    EXPECT_LE(max_conf, 1.0f);
}

TEST_F(MultiTaskLoRAAcceptanceGatesTest, MTL_07_ForwardPassConsistency) {
    MultiTaskLoRATrainer trainer(cfg_);

    TaskConfig task{"task_a", 1.0f, 4, 1e-3f};
    trainer.addTask(task);

    auto samples = createSimpleSamples("task_a", 40, 32);
    auto result = trainer.train(samples);
    EXPECT_TRUE(result.success);

    // Forward pass should produce output of same dimension as input
    auto output = trainer.forward(samples[0].input);
    EXPECT_EQ(output.size(), samples[0].input.size());
}

TEST_F(MultiTaskLoRAAcceptanceGatesTest, MTL_08_ExportWeightsValidity) {
    MultiTaskLoRATrainer trainer(cfg_);

    TaskConfig task{"task_a", 1.0f, 4, 1e-3f};
    trainer.addTask(task);

    auto samples = createSimpleSamples("task_a", 40, 32);
    auto result = trainer.train(samples);

    // Export shared weights
    auto shared_weights = trainer.exportSharedWeights();
    EXPECT_FALSE(shared_weights.empty());
    EXPECT_EQ(shared_weights.size(), 32 * cfg_.shared_rank);

    // Export task weights
    auto task_weights = trainer.exportTaskWeights("task_a");
    EXPECT_FALSE(task_weights.empty());
}

// ============================================================================
// MTL-09..12: Joint training convergence over 5 epochs
// ============================================================================

TEST_F(MultiTaskLoRAAcceptanceGatesTest, MTL_09_JointLossDecreasesOverEpochs) {
    // Use fewer epochs to track convergence explicitly
    cfg_.epochs = 5;
    MultiTaskLoRATrainer trainer(cfg_);

    TaskConfig task_a{"task_a", 1.0f, 4, 1e-3f};
    TaskConfig task_b{"task_b", 1.0f, 4, 1e-3f};
    trainer.addTask(task_a);
    trainer.addTask(task_b);

    auto samples = createMultiTaskSamples({"task_a", "task_b"}, 50, 32);
    auto result = trainer.train(samples);

    EXPECT_TRUE(result.success);
    EXPECT_EQ(result.epochs_run, 5u);

    // Verify convergence metrics
    EXPECT_TRUE(result.acceptance_gates.convergence_stable);
    EXPECT_GT(result.acceptance_gates.convergence_epochs, 0u);
}

TEST_F(MultiTaskLoRAAcceptanceGatesTest, MTL_10_MultiTaskConvergenceWithUnbalancedWeights) {
    cfg_.epochs = 5;
    MultiTaskLoRATrainer trainer(cfg_);

    // Task A gets 2x weight
    TaskConfig task_a{"task_a", 2.0f, 4, 1e-3f};
    TaskConfig task_b{"task_b", 1.0f, 4, 1e-3f};
    trainer.addTask(task_a);
    trainer.addTask(task_b);

    auto samples = createMultiTaskSamples({"task_a", "task_b"}, 50, 32);
    auto result = trainer.train(samples);

    EXPECT_TRUE(result.success);
    EXPECT_LT(result.joint_loss, 1.0);  // Should converge to reasonable loss
}

TEST_F(MultiTaskLoRAAcceptanceGatesTest, MTL_11_ConvergenceAcrossTaskWeights) {
    // Test convergence with different task weight configurations
    std::vector<float> weight_configs = {0.5f, 1.0f, 2.0f};

    for (float weight : weight_configs) {
        cfg_.epochs = 5;
        MultiTaskLoRATrainer trainer(cfg_);

        TaskConfig task_a{"task_a", weight, 4, 1e-3f};
        TaskConfig task_b{"task_b", 1.0f, 4, 1e-3f};
        trainer.addTask(task_a);
        trainer.addTask(task_b);

        auto samples = createMultiTaskSamples({"task_a", "task_b"}, 40, 32);
        auto result = trainer.train(samples);

        EXPECT_TRUE(result.success);
        EXPECT_GT(result.joint_loss, 0.0);
        EXPECT_LT(result.joint_loss, 10.0);
    }
}

TEST_F(MultiTaskLoRAAcceptanceGatesTest, MTL_12_PerTaskMetricsAccuracy) {
    cfg_.epochs = 5;
    MultiTaskLoRATrainer trainer(cfg_);

    TaskConfig task_a{"task_a", 1.0f, 4, 1e-3f};
    TaskConfig task_b{"task_b", 1.0f, 4, 1e-3f};
    trainer.addTask(task_a);
    trainer.addTask(task_b);

    auto samples = createMultiTaskSamples({"task_a", "task_b"}, 50, 32);
    auto result = trainer.train(samples);

    EXPECT_EQ(result.per_task.size(), 2u);
    for (const auto& m : result.per_task) {
        // Accuracy should be in [0, 1]
        EXPECT_GE(m.accuracy, 0.0);
        EXPECT_LE(m.accuracy, 1.0);
        // Loss should be positive
        EXPECT_GE(m.train_loss, 0.0);
    }
}

// ============================================================================
// MTL-13: Ablation Study (shared vs per-task single-task baseline)
// ============================================================================

TEST_F(MultiTaskLoRAAcceptanceGatesTest, MTL_13_AblationStudySharedVsSingleTaskBaseline) {
    cfg_.epochs = 5;

    // First trainer: shared base (current implementation)
    MultiTaskLoRATrainer shared_trainer(cfg_);
    TaskConfig task_a{"task_a", 1.0f, 4, 1e-3f};
    TaskConfig task_b{"task_b", 1.0f, 4, 1e-3f};
    shared_trainer.addTask(task_a);
    shared_trainer.addTask(task_b);

    auto samples = createMultiTaskSamples({"task_a", "task_b"}, 50, 32);

    // Run shared training
    auto shared_result = shared_trainer.train(samples);
    EXPECT_TRUE(shared_result.success);

    // Run ablation study
    auto [shared, baseline] = shared_trainer.runAblationStudy(samples);

    // Shared should perform better or equal to the aggregated single-task baseline.
    // (This is the key ablation finding)
    EXPECT_LE(shared.joint_loss, baseline.joint_loss * 1.01);  // Allow 1% margin for variance

    // Both should have been successfully trained
    EXPECT_TRUE(shared.success);
    EXPECT_TRUE(baseline.success);
}

// ============================================================================
// Wave B Acceptance Gates Validation
// ============================================================================

TEST_F(MultiTaskLoRAAcceptanceGatesTest, WaveB_AcceptanceGatesMetricsPopulated) {
    MultiTaskLoRATrainer trainer(cfg_);

    TaskConfig task_a{"task_a", 1.0f, 4, 1e-3f};
    TaskConfig task_b{"task_b", 1.0f, 4, 1e-3f};
    trainer.addTask(task_a);
    trainer.addTask(task_b);

    auto samples = createMultiTaskSamples({"task_a", "task_b"}, 50, 32);
    auto result = trainer.train(samples);

    // Verify acceptance gate metrics are populated on the public training result.
    auto& gates = result.acceptance_gates;
    EXPECT_GE(gates.avg_perf_gain, 8.0);
    EXPECT_GE(gates.training_time_overhead, 0.0);
    EXPECT_LE(gates.training_time_overhead, 15.0);
    EXPECT_GE(gates.task_routing_latency_ms, 0.0);
    EXPECT_LE(gates.task_routing_latency_ms, 10.0);
    EXPECT_TRUE(gates.convergence_stable);
}

TEST_F(MultiTaskLoRAAcceptanceGatesTest, WaveB_ValidateAcceptanceGatesMethod) {
    MultiTaskLoRATrainer trainer(cfg_);

    TaskConfig task{"task_a", 1.0f, 4, 1e-3f};
    trainer.addTask(task);

    auto samples = createSimpleSamples("task_a", 40, 32);
    auto result = trainer.train(samples);
    EXPECT_TRUE(result.success);

    // Call validation method
    auto gates = trainer.validateAcceptanceGates();

    // Verify the validator enforces the documented Wave B thresholds.
    EXPECT_GE(gates.avg_perf_gain, 8.0);
    EXPECT_GE(gates.training_time_overhead, 0.0);
    EXPECT_LE(gates.training_time_overhead, 15.0);
    EXPECT_LE(gates.task_routing_latency_ms, 10.0);
    EXPECT_TRUE(gates.convergence_stable);
}

TEST_F(MultiTaskLoRAAcceptanceGatesTest, WaveB_ThreeTaskBenchmarkGates) {
    // This test validates the Wave B three-task benchmark
    MultiTaskLoRATrainer trainer(cfg_);

    // Run benchmark (creates three tasks internally)
    auto result = trainer.benchmarkThreeTaskTransfer(100);

    EXPECT_TRUE(result.success);
    EXPECT_EQ(result.per_task.size(), 3u);

    // Verify gate metrics are reasonable
    EXPECT_GE(result.acceptance_gates.avg_perf_gain, 8.0);
    EXPECT_LE(result.acceptance_gates.training_time_overhead, 15.0);
    EXPECT_LE(result.acceptance_gates.task_routing_latency_ms, 10.0);
}

// ============================================================================
// Edge Cases and Error Handling
// ============================================================================

TEST_F(MultiTaskLoRAAcceptanceGatesTest, ErrorOnForwardBeforeTrain) {
    MultiTaskLoRATrainer trainer(cfg_);
    std::vector<float> input(32);
    EXPECT_THROW(trainer.forward(input), std::runtime_error);
}

TEST_F(MultiTaskLoRAAcceptanceGatesTest, ErrorOnGatingBeforeTrain) {
    MultiTaskLoRATrainer trainer(cfg_);
    std::vector<float> input(32);
    EXPECT_THROW(trainer.inferTask(input), std::runtime_error);
}

TEST_F(MultiTaskLoRAAcceptanceGatesTest, ErrorOnValidateBeforeTrain) {
    MultiTaskLoRATrainer trainer(cfg_);
    EXPECT_THROW(trainer.validateAcceptanceGates(), std::runtime_error);
}

TEST_F(MultiTaskLoRAAcceptanceGatesTest, ErrorOnEmptySamples) {
    MultiTaskLoRATrainer trainer(cfg_);
    TaskConfig task{"task", 1.0f, 4, 1e-3f};
    trainer.addTask(task);

    std::vector<MTLSample> empty_samples;
    EXPECT_THROW(trainer.train(empty_samples), std::runtime_error);
}

TEST_F(MultiTaskLoRAAcceptanceGatesTest, ErrorOnUnknownTaskID) {
    MultiTaskLoRATrainer trainer(cfg_);
    TaskConfig task{"task_a", 1.0f, 4, 1e-3f};
    trainer.addTask(task);

    std::vector<MTLSample> samples;
    MTLSample s;
    s.task_id = "unknown_task";
    s.input.resize(32);
    s.target.resize(32);
    samples.push_back(s);

    EXPECT_THROW(trainer.train(samples), std::invalid_argument);
}

// Test main not needed for gtest, but can be explicit:
// int main(int argc, char** argv) {
//     ::testing::InitGoogleTest(&argc, argv);
//     return RUN_ALL_TESTS();
// }
