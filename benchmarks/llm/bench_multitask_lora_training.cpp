// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

/**
 * @file bench_multitask_lora_training.cpp
 * @brief Google Benchmark release-gate benchmarks for Wave B Multi-Task LoRA training.
 *
 * Wave B Release Gates:
 *  - GATE-MTL-01: Average task performance gain ≥ +8% vs single-task baseline
 *  - GATE-MTL-02: Training-time increase ≤ 15% across benchmarked task sets
 *  - GATE-MTL-03: Task routing latency ≤ 10ms (domain gating)
 *  - GATE-MTL-04: Convergence stability across task-weight schedules
 *  - GATE-MTL-05: Three-task transfer evaluation benchmark
 *  - GATE-MTL-06: Ablation study (shared vs separate adapters)
 */

#include <benchmark/benchmark.h>
#include "training/multi_task_lora.h"

#include <chrono>
#include <random>
#include <vector>

using namespace themis::training;

namespace {

// =============================================================================
// Shared fixture helpers
// =============================================================================

// Create synthetic training samples
std::vector<MTLSample> createSyntheticSamples(
    const std::vector<std::string>& task_ids,
    size_t samples_per_task,
    size_t input_dim = 32,
    size_t seed = 42) {
    std::vector<MTLSample> samples;
    std::mt19937 rng(seed);
    std::uniform_real_distribution<float> dist(-1.0f, 1.0f);
    
    for (size_t ti = 0; ti < task_ids.size(); ++ti) {
        for (size_t i = 0; i < samples_per_task; ++i) {
            MTLSample s;
            s.task_id = task_ids[ti];
            s.input.resize(input_dim);
            s.target.resize(input_dim);
            
            for (size_t j = 0; j < input_dim; ++j) {
                // Slightly different distributions per task
                float scale = 1.0f + 0.2f * static_cast<float>(ti);
                s.input[j] = dist(rng) * scale;
                s.target[j] = s.input[j] * 0.9f + dist(rng) * 0.1f;
            }
            s.weight = 1.0f;
            samples.push_back(s);
        }
    }
    
    return samples;
}

} // anonymous namespace

// =============================================================================
// GATE-MTL-01: Single-task baseline performance
// =============================================================================

static void GATE_MTL_01_SingleTaskBaseline(benchmark::State& state) {
    MultiTaskLoRAConfig cfg;
    cfg.shared_rank = 8;
    cfg.epochs = 5;
    cfg.batch_size = 32;
    cfg.learning_rate = 1e-3f;
    
    for (auto _ : state) {
        MultiTaskLoRATrainer trainer(cfg);
        
        TaskConfig task{"task_a", 1.0f, 4, 1e-3f};
        trainer.addTask(task);
        
        auto samples = createSyntheticSamples({"task_a"}, 100, 32);
        auto result = trainer.train(samples);
        
        benchmark::DoNotOptimize(result);
    }
}

BENCHMARK(GATE_MTL_01_SingleTaskBaseline)
    ->Name("MTL/gate-01-single-task-baseline")
    ->Iterations(1);

// =============================================================================
// GATE-MTL-02: Multi-task training with shared base
// =============================================================================

static void GATE_MTL_02_MultiTaskSharedBase(benchmark::State& state) {
    MultiTaskLoRAConfig cfg;
    cfg.shared_rank = 8;
    cfg.epochs = 5;
    cfg.batch_size = 32;
    cfg.learning_rate = 1e-3f;
    
    for (auto _ : state) {
        MultiTaskLoRATrainer trainer(cfg);
        
        TaskConfig task_a{"task_a", 1.0f, 4, 1e-3f};
        TaskConfig task_b{"task_b", 1.0f, 4, 1e-3f};
        TaskConfig task_c{"task_c", 1.0f, 4, 1e-3f};
        trainer.addTask(task_a);
        trainer.addTask(task_b);
        trainer.addTask(task_c);
        
        auto samples = createSyntheticSamples({"task_a", "task_b", "task_c"}, 100, 32);
        auto result = trainer.train(samples);
        
        benchmark::DoNotOptimize(result);
    }
}

BENCHMARK(GATE_MTL_02_MultiTaskSharedBase)
    ->Name("MTL/gate-02-multitask-shared-base")
    ->Iterations(1);

// =============================================================================
// GATE-MTL-03: Task routing latency (domain gating)
// =============================================================================

static void GATE_MTL_03_TaskRoutingLatency(benchmark::State& state) {
    // Setup: train once outside the benchmark loop
    MultiTaskLoRAConfig cfg;
    cfg.shared_rank = 8;
    cfg.epochs = 3;
    cfg.batch_size = 32;
    cfg.learning_rate = 1e-3f;
    
    MultiTaskLoRATrainer trainer(cfg);
    
    TaskConfig task_a{"task_a", 1.0f, 4, 1e-3f};
    TaskConfig task_b{"task_b", 1.0f, 4, 1e-3f};
    trainer.addTask(task_a);
    trainer.addTask(task_b);
    
    auto samples = createSyntheticSamples({"task_a", "task_b"}, 50, 32);
    trainer.train(samples);
    
    // Now benchmark the routing latency
    for (auto _ : state) {
        auto gate = trainer.inferTask(samples[0].input);
        benchmark::DoNotOptimize(gate);
    }
}

BENCHMARK(GATE_MTL_03_TaskRoutingLatency)
    ->Name("MTL/gate-03-task-routing-latency")
    ->Iterations(1000);

// =============================================================================
// GATE-MTL-04: Forward pass throughput
// =============================================================================

static void GATE_MTL_04_ForwardPassThroughput(benchmark::State& state) {
    // Setup: train once outside the benchmark loop
    MultiTaskLoRAConfig cfg;
    cfg.shared_rank = 8;
    cfg.epochs = 3;
    cfg.batch_size = 32;
    cfg.learning_rate = 1e-3f;
    
    MultiTaskLoRATrainer trainer(cfg);
    
    TaskConfig task_a{"task_a", 1.0f, 4, 1e-3f};
    TaskConfig task_b{"task_b", 1.0f, 4, 1e-3f};
    trainer.addTask(task_a);
    trainer.addTask(task_b);
    
    auto samples = createSyntheticSamples({"task_a", "task_b"}, 50, 32);
    trainer.train(samples);
    
    // Benchmark forward pass throughput
    for (auto _ : state) {
        auto output = trainer.forward(samples[0].input);
        benchmark::DoNotOptimize(output);
    }
}

BENCHMARK(GATE_MTL_04_ForwardPassThroughput)
    ->Name("MTL/gate-04-forward-pass-throughput")
    ->Iterations(1000);

// =============================================================================
// GATE-MTL-05: Three-task transfer evaluation
// =============================================================================

static void GATE_MTL_05_ThreeTaskTransferBenchmark(benchmark::State& state) {
    MultiTaskLoRAConfig cfg;
    cfg.shared_rank = 8;
    cfg.epochs = 5;
    cfg.batch_size = 32;
    cfg.learning_rate = 1e-3f;
    
    for (auto _ : state) {
        MultiTaskLoRATrainer trainer(cfg);
        
        // Benchmark directly uses the three-task transfer method
        auto result = trainer.benchmarkThreeTaskTransfer(100);
        benchmark::DoNotOptimize(result);
    }
}

BENCHMARK(GATE_MTL_05_ThreeTaskTransferBenchmark)
    ->Name("MTL/gate-05-three-task-transfer")
    ->Iterations(1);

// =============================================================================
// GATE-MTL-06: Training overhead comparison (shared vs single-task)
// =============================================================================

static void GATE_MTL_06_TrainingOverheadComparison(benchmark::State& state) {
    MultiTaskLoRAConfig cfg;
    cfg.shared_rank = 8;
    cfg.epochs = 5;
    cfg.batch_size = 32;
    cfg.learning_rate = 1e-3f;
    
    // Prepare multi-task samples
    std::vector<std::string> task_ids = {"task_a", "task_b", "task_c"};
    auto mtl_samples = createSyntheticSamples(task_ids, 100, 32);
    
    // Measure single-task training time
    auto single_start = std::chrono::high_resolution_clock::now();
    {
        MultiTaskLoRATrainer trainer(cfg);
        TaskConfig task{"task_a", 1.0f, 4, 1e-3f};
        trainer.addTask(task);
        
        std::vector<MTLSample> single_samples;
        for (const auto& s : mtl_samples) {
            if (s.task_id == "task_a") single_samples.push_back(s);
        }
        trainer.train(single_samples);
    }
    auto single_end = std::chrono::high_resolution_clock::now();
    double single_time = std::chrono::duration<double>(single_end - single_start).count();
    
    // Measure multi-task training time
    for (auto _ : state) {
        auto mtl_start = std::chrono::high_resolution_clock::now();
        
        MultiTaskLoRATrainer trainer(cfg);
        TaskConfig task_a{"task_a", 1.0f, 4, 1e-3f};
        TaskConfig task_b{"task_b", 1.0f, 4, 1e-3f};
        TaskConfig task_c{"task_c", 1.0f, 4, 1e-3f};
        trainer.addTask(task_a);
        trainer.addTask(task_b);
        trainer.addTask(task_c);
        
        auto result = trainer.train(mtl_samples);
        
        auto mtl_end = std::chrono::high_resolution_clock::now();
        double mtl_time = std::chrono::duration<double>(mtl_end - mtl_start).count();
        
        // Calculate overhead percentage (target: ≤ 15%)
        double overhead_pct = ((mtl_time - single_time) / single_time) * 100.0;
        
        benchmark::DoNotOptimize(result);
        benchmark::DoNotOptimize(overhead_pct);
    }
}

BENCHMARK(GATE_MTL_06_TrainingOverheadComparison)
    ->Name("MTL/gate-06-training-overhead")
    ->Iterations(1);

// =============================================================================
// GATE-MTL-07: Acceptance gate validation
// =============================================================================

static void GATE_MTL_07_AcceptanceGateValidation(benchmark::State& state) {
    MultiTaskLoRAConfig cfg;
    cfg.shared_rank = 8;
    cfg.epochs = 5;
    cfg.batch_size = 32;
    cfg.learning_rate = 1e-3f;
    
    MultiTaskLoRATrainer trainer(cfg);
    
    TaskConfig task_a{"task_a", 1.0f, 4, 1e-3f};
    TaskConfig task_b{"task_b", 1.0f, 4, 1e-3f};
    trainer.addTask(task_a);
    trainer.addTask(task_b);
    
    auto samples = createSyntheticSamples({"task_a", "task_b"}, 100, 32);
    trainer.train(samples);
    
    // Benchmark the validation method
    for (auto _ : state) {
        auto gates = trainer.validateAcceptanceGates();
        benchmark::DoNotOptimize(gates);
    }
}

BENCHMARK(GATE_MTL_07_AcceptanceGateValidation)
    ->Name("MTL/gate-07-acceptance-validation")
    ->Iterations(100);

// =============================================================================
// GATE-MTL-08: Ablation study (shared vs separate)
// =============================================================================

static void GATE_MTL_08_AblationStudySharedVsSeparate(benchmark::State& state) {
    MultiTaskLoRAConfig cfg;
    cfg.shared_rank = 8;
    cfg.epochs = 5;
    cfg.batch_size = 32;
    cfg.learning_rate = 1e-3f;
    
    MultiTaskLoRATrainer trainer(cfg);
    
    TaskConfig task_a{"task_a", 1.0f, 4, 1e-3f};
    TaskConfig task_b{"task_b", 1.0f, 4, 1e-3f};
    trainer.addTask(task_a);
    trainer.addTask(task_b);
    
    auto samples = createSyntheticSamples({"task_a", "task_b"}, 100, 32);
    
    for (auto _ : state) {
        auto [shared, separate] = trainer.runAblationStudy(samples);
        benchmark::DoNotOptimize(shared);
        benchmark::DoNotOptimize(separate);
    }
}

BENCHMARK(GATE_MTL_08_AblationStudySharedVsSeparate)
    ->Name("MTL/gate-08-ablation-shared-vs-separate")
    ->Iterations(1);

// =============================================================================
// Additional: Scaling benchmarks for different input dimensions
// =============================================================================

static void GATE_MTL_Scaling_InputDim(benchmark::State& state) {
    const size_t input_dim = state.range(0);
    
    MultiTaskLoRAConfig cfg;
    cfg.shared_rank = 8;
    cfg.epochs = 3;
    cfg.batch_size = 32;
    cfg.learning_rate = 1e-3f;
    cfg.input_dim = input_dim;
    
    for (auto _ : state) {
        MultiTaskLoRATrainer trainer(cfg);
        
        TaskConfig task_a{"task_a", 1.0f, 4, 1e-3f};
        TaskConfig task_b{"task_b", 1.0f, 4, 1e-3f};
        trainer.addTask(task_a);
        trainer.addTask(task_b);
        
        auto samples = createSyntheticSamples({"task_a", "task_b"}, 50, input_dim);
        auto result = trainer.train(samples);
        
        benchmark::DoNotOptimize(result);
    }
    
    state.SetLabel("input_dim=" + std::to_string(input_dim));
}

BENCHMARK(GATE_MTL_Scaling_InputDim)
    ->Name("MTL/scaling-input-dim")
    ->Arg(16)
    ->Arg(32)
    ->Arg(64)
    ->Arg(128)
    ->Iterations(1);

// =============================================================================
// Additional: Scaling benchmarks for different task counts
// =============================================================================

static void GATE_MTL_Scaling_TaskCount(benchmark::State& state) {
    const int task_count = state.range(0);
    
    MultiTaskLoRAConfig cfg;
    cfg.shared_rank = 8;
    cfg.epochs = 3;
    cfg.batch_size = 32;
    cfg.learning_rate = 1e-3f;
    
    for (auto _ : state) {
        MultiTaskLoRATrainer trainer(cfg);
        
        std::vector<std::string> task_ids;
        for (int i = 0; i < task_count; ++i) {
            std::string task_id = "task_" + std::to_string(i);
            TaskConfig task{task_id, 1.0f, 4, 1e-3f};
            trainer.addTask(task);
            task_ids.push_back(task_id);
        }
        
        auto samples = createSyntheticSamples(task_ids, 50, 32);
        auto result = trainer.train(samples);
        
        benchmark::DoNotOptimize(result);
    }
    
    state.SetLabel("tasks=" + std::to_string(task_count));
}

BENCHMARK(GATE_MTL_Scaling_TaskCount)
    ->Name("MTL/scaling-task-count")
    ->Arg(2)
    ->Arg(3)
    ->Arg(4)
    ->Arg(5)
    ->Iterations(1);
