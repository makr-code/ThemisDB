#include <benchmark/benchmark.h>
#include "llm/lora_framework/multi_gpu_lora_layer.h"
#include "llm/lora_framework/multi_gpu_trainer.h"
#include "llm/lora_framework/multi_gpu.h"
#include "llm/lora_framework/gpu_tensor.h"
#include <chrono>
#include <vector>
#include <memory>

#ifndef THEMIS_ENABLE_GPU

static void BM_MultiGPUScaling_GPUDisabled(benchmark::State& state) {
    for (auto _ : state) {
        state.SkipWithError("Multi-GPU scaling benchmarks are disabled in this build");
        break;
    }
}
// Disabled: multi-GPU scaling requires multi-GPU CUDA runner | Deadline: v1.9.0 | Issue: #5
BENCHMARK(BM_MultiGPUScaling_GPUDisabled);

BENCHMARK_MAIN();

#else

using namespace themis::llm::lora;

/**
 * @file bench_multi_gpu_scaling.cpp
 * @brief Multi-GPU Scaling Benchmarks for Data Parallel Training
 * 
 * Tests:
 * - Data parallelism scaling efficiency (1x, 2x, 4x GPUs)
 * - Gradient synchronization overhead (NCCL/RCCL all-reduce)
 * - Communication vs computation ratio
 * - Target: 80-95% scaling efficiency
 */

// Benchmark configurations
constexpr size_t BATCH_SIZES[] = {4, 8, 16, 32};
constexpr size_t HIDDEN_DIM = 1024;
constexpr size_t LORA_RANK = 8;
constexpr int WARMUP_ITERS = 3;
constexpr int MEASURE_ITERS = 10;

// ============================================================================
// Helper Functions
// ============================================================================

static int get_gpu_count() {
    auto backends = GPUMemoryManager::detect_backends();
    int max_gpus = 0;
    for (const auto& backend : backends) {
        if (backend.available && 
            (backend.type == themis::acceleration::BackendType::CUDA ||
             backend.type == themis::acceleration::BackendType::HIP)) {
            // For simplicity, assume we can query device count
            // In real implementation, this would query cudaGetDeviceCount/hipGetDeviceCount
            max_gpus = std::max(max_gpus, 4);  // Cap at 4 for benchmark
        }
    }
    return max_gpus;
}

static bool multi_gpu_available(int num_gpus) {
    return get_gpu_count() >= num_gpus;
}

// ============================================================================
// Single GPU Baseline (for scaling comparison)
// ============================================================================

static void BM_SingleGPU_TrainingStep(benchmark::State& state) {
    if (!multi_gpu_available(1)) {
        state.SkipWithError("GPU not available");
        return;
    }
    
    size_t batch_size = state.range(0);
    
    // Create single GPU context
    MultiGPUContext ctx(1);
    
    // Create layer
    MultiGPULoRALayer layer(HIDDEN_DIM, HIDDEN_DIM, LORA_RANK, 1.0f, ctx);
    
    // Create input tensor
    std::vector<GPUTensor> inputs;
    inputs.emplace_back(std::vector<size_t>{batch_size, HIDDEN_DIM}, Device::cuda(0));
    inputs[0].fill(0.5f);
    
    std::vector<GPUTensor> targets;
    targets.emplace_back(std::vector<size_t>{batch_size, HIDDEN_DIM}, Device::cuda(0));
    targets[0].fill(0.3f);
    
    // Create trainer
    MultiGPULoRATrainer::Config config;
    config.learning_rate = 0.001f;
    MultiGPULoRATrainer trainer(ctx, config);
    
    // Warmup
    for (int i = 0; i < WARMUP_ITERS; ++i) {
        trainer.train_step(layer, inputs, targets);
    }
    
    // Measure
    for (auto _ : state) {
        auto start = std::chrono::high_resolution_clock::now();
        trainer.train_step(layer, inputs, targets);
        auto end = std::chrono::high_resolution_clock::now();
        
        auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        state.SetIterationTime(elapsed.count() / 1e6);
    }
    
    double samples_per_sec = batch_size / (state.iterations() * state.max_iterations * 1e-6);
    state.counters["samples/sec"] = samples_per_sec;
    state.counters["batch_size"] = batch_size;
    state.counters["num_gpus"] = 1;
    state.counters["scaling_efficiency"] = 1.0;  // Baseline
    
    state.SetLabel("1 GPU");
}

BENCHMARK(BM_SingleGPU_TrainingStep)
    ->Arg(4)->Arg(8)->Arg(16)->Arg(32)
    ->UseManualTime();

// ============================================================================
// 2-GPU Data Parallel Training
// ============================================================================

static void BM_TwoGPU_DataParallel(benchmark::State& state) {
    if (!multi_gpu_available(2)) {
        state.SkipWithError("2 GPUs not available");
        return;
    }
    
    size_t total_batch_size = state.range(0);
    size_t per_gpu_batch_size = total_batch_size / 2;
    
    // Create 2-GPU context
    MultiGPUContext ctx(2);
    
    // Create layer
    MultiGPULoRALayer layer(HIDDEN_DIM, HIDDEN_DIM, LORA_RANK, 1.0f, ctx);
    
    // Create sharded input tensors (one per GPU)
    std::vector<GPUTensor> inputs = {};

    for (int i = 0; i < 2; ++i) {
        inputs.emplace_back(std::vector<size_t>{per_gpu_batch_size, HIDDEN_DIM}, Device::cuda(i));
        inputs[i].fill(0.5f);
    }
    
    std::vector<GPUTensor> targets = {};

    for (int i = 0; i < 2; ++i) {
        targets.emplace_back(std::vector<size_t>{per_gpu_batch_size, HIDDEN_DIM}, Device::cuda(i));
        targets[i].fill(0.3f);
    }
    
    // Create trainer with gradient synchronization
    MultiGPULoRATrainer::Config config;
    config.learning_rate = 0.001f;
    config.sync_every_step = true;
    MultiGPULoRATrainer trainer(ctx, config);
    
    // Warmup
    for (int i = 0; i < WARMUP_ITERS; ++i) {
        trainer.train_step(layer, inputs, targets);
    }
    
    // Measure
    for (auto _ : state) {
        auto start = std::chrono::high_resolution_clock::now();
        trainer.train_step(layer, inputs, targets);
        auto end = std::chrono::high_resolution_clock::now();
        
        auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        state.SetIterationTime(elapsed.count() / 1e6);
    }
    
    double samples_per_sec = total_batch_size / (state.iterations() * state.max_iterations * 1e-6);
    state.counters["samples/sec"] = samples_per_sec;
    state.counters["batch_size"] = total_batch_size;
    state.counters["num_gpus"] = 2;
    
    // Calculate scaling efficiency (ideal 2x speedup)
    // Note: This requires storing baseline results, simplified here
    state.counters["target_efficiency"] = 0.85;  // Target 85%+
    
    state.SetLabel("2 GPUs");
}

BENCHMARK(BM_TwoGPU_DataParallel)
    ->Arg(8)->Arg(16)->Arg(32)
    ->UseManualTime();

// ============================================================================
// 4-GPU Data Parallel Training
// ============================================================================

static void BM_FourGPU_DataParallel(benchmark::State& state) {
    if (!multi_gpu_available(4)) {
        state.SkipWithError("4 GPUs not available");
        return;
    }
    
    size_t total_batch_size = state.range(0);
    size_t per_gpu_batch_size = total_batch_size / 4;
    
    // Create 4-GPU context
    MultiGPUContext ctx(4);
    
    // Create layer
    MultiGPULoRALayer layer(HIDDEN_DIM, HIDDEN_DIM, LORA_RANK, 1.0f, ctx);
    
    // Create sharded input tensors (one per GPU)
    std::vector<GPUTensor> inputs = {};

    for (int i = 0; i < 4; ++i) {
        inputs.emplace_back(std::vector<size_t>{per_gpu_batch_size, HIDDEN_DIM}, Device::cuda(i));
        inputs[i].fill(0.5f);
    }
    
    std::vector<GPUTensor> targets = {};

    for (int i = 0; i < 4; ++i) {
        targets.emplace_back(std::vector<size_t>{per_gpu_batch_size, HIDDEN_DIM}, Device::cuda(i));
        targets[i].fill(0.3f);
    }
    
    // Create trainer with gradient synchronization
    MultiGPULoRATrainer::Config config;
    config.learning_rate = 0.001f;
    config.sync_every_step = true;
    MultiGPULoRATrainer trainer(ctx, config);
    
    // Warmup
    for (int i = 0; i < WARMUP_ITERS; ++i) {
        trainer.train_step(layer, inputs, targets);
    }
    
    // Measure
    for (auto _ : state) {
        auto start = std::chrono::high_resolution_clock::now();
        trainer.train_step(layer, inputs, targets);
        auto end = std::chrono::high_resolution_clock::now();
        
        auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        state.SetIterationTime(elapsed.count() / 1e6);
    }
    
    double samples_per_sec = total_batch_size / (state.iterations() * state.max_iterations * 1e-6);
    state.counters["samples/sec"] = samples_per_sec;
    state.counters["batch_size"] = total_batch_size;
    state.counters["num_gpus"] = 4;
    state.counters["target_efficiency"] = 0.80;  // Target 80%+
    
    state.SetLabel("4 GPUs");
}

BENCHMARK(BM_FourGPU_DataParallel)
    ->Arg(16)->Arg(32)
    ->UseManualTime();

// ============================================================================
// Gradient Synchronization Overhead
// ============================================================================

static void BM_GradientSync_Overhead(benchmark::State& state) {
    if (!multi_gpu_available(2)) {
        state.SkipWithError("2 GPUs not available");
        return;
    }
    
    int num_gpus = state.range(0);
    size_t tensor_size = state.range(1);  // Number of parameters to sync
    
    if (!multi_gpu_available(num_gpus)) {
        state.SkipWithError("Required GPUs not available");
        return;
    }
    
    // Create context
    MultiGPUContext ctx(num_gpus);
    
    // Create layer to test gradient synchronization
    MultiGPULoRALayer layer(tensor_size, tensor_size, 8, 1.0f, ctx);
    
    // Create dummy inputs and perform forward/backward to generate gradients
    std::vector<GPUTensor> inputs = {};

    for (int i = 0; i < num_gpus; ++i) {
        inputs.emplace_back(std::vector<size_t>{1, tensor_size}, Device::cuda(i));
        inputs[i].fill(1.0f / num_gpus);
    }
    
    // Warmup
    for (int i = 0; i < WARMUP_ITERS; ++i) {
        auto outputs = layer.forward(inputs);
        layer.backward(outputs);
        layer.synchronize_gradients();
        layer.zero_grad();
    }
    
    // Measure
    for (auto _ : state) {
        // Generate gradients
        auto outputs = layer.forward(inputs);
        layer.backward(outputs);
        
        // Measure gradient sync time
        auto start = std::chrono::high_resolution_clock::now();
        layer.synchronize_gradients();
        auto end = std::chrono::high_resolution_clock::now();
        
        layer.zero_grad();
        
        auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        state.SetIterationTime(elapsed.count() / 1e6);
    }
    
    // Calculate approximate bandwidth (simplified)
    size_t param_count = tensor_size * tensor_size * 2;  // A and B matrices
    size_t bytes_transferred = param_count * sizeof(float);
    double bandwidth_gbps = bytes_transferred / (state.iterations() * state.max_iterations * 1e-9);
    
    state.counters["bandwidth_GB/s"] = bandwidth_gbps;
    state.counters["num_gpus"] = num_gpus;
    state.counters["tensor_size_MB"] = (bytes_transferred) / (1024.0 * 1024.0);
    
    state.SetLabel(std::to_string(num_gpus) + " GPUs");
}

BENCHMARK(BM_GradientSync_Overhead)
    ->Args({2, 1024 * 1024})    // 4MB tensor, 2 GPUs
    ->Args({2, 4 * 1024 * 1024})  // 16MB tensor, 2 GPUs
    ->Args({4, 1024 * 1024})    // 4MB tensor, 4 GPUs
    ->Args({4, 4 * 1024 * 1024})  // 16MB tensor, 4 GPUs
    ->UseManualTime();

// ============================================================================
// Communication vs Computation Ratio
// ============================================================================

static void BM_CommCompute_Ratio(benchmark::State& state) {
    if (!multi_gpu_available(2)) {
        state.SkipWithError("2 GPUs not available");
        return;
    }
    
    int num_gpus = state.range(0);
    size_t batch_size = state.range(1);
    
    if (!multi_gpu_available(num_gpus)) {
        state.SkipWithError("Required GPUs not available");
        return;
    }
    
    MultiGPUContext ctx(num_gpus);
    MultiGPULoRALayer layer(HIDDEN_DIM, HIDDEN_DIM, LORA_RANK, 1.0f, ctx);
    
    size_t per_gpu_batch = batch_size / num_gpus;
    
    // Create inputs
    std::vector<GPUTensor> inputs = {};

    for (int i = 0; i < num_gpus; ++i) {
        inputs.emplace_back(std::vector<size_t>{per_gpu_batch, HIDDEN_DIM}, Device::cuda(i));
        inputs[i].fill(0.5f);
    }
    
    std::vector<GPUTensor> targets = {};

    for (int i = 0; i < num_gpus; ++i) {
        targets.emplace_back(std::vector<size_t>{per_gpu_batch, HIDDEN_DIM}, Device::cuda(i));
        targets[i].fill(0.3f);
    }
    
    MultiGPULoRATrainer::Config config;
    config.learning_rate = 0.001f;
    config.sync_every_step = true;
    config.enable_profiling = true;
    MultiGPULoRATrainer trainer(ctx, config);
    
    // Warmup
    for (int i = 0; i < WARMUP_ITERS; ++i) {
        trainer.train_step(layer, inputs, targets);
    }
    
    // Measure with timing breakdown
    double total_compute_time = 0.0;
    double total_comm_time = 0.0;
    
    for (auto _ : state) {
        auto start = std::chrono::high_resolution_clock::now();
        
        // Forward pass (compute)
        auto compute_start = std::chrono::high_resolution_clock::now();
        auto outputs = layer.forward(inputs);
        auto compute_end = std::chrono::high_resolution_clock::now();
        total_compute_time += std::chrono::duration_cast<std::chrono::microseconds>(
            compute_end - compute_start).count();
        
        // Backward pass (compute)
        compute_start = std::chrono::high_resolution_clock::now();
        std::vector<GPUTensor> grad_outputs = {};

        for (size_t i = 0; i < outputs.size(); ++i) {
            grad_outputs.push_back(outputs[i] - targets[i]);
        }
        auto grad_inputs = layer.backward(grad_outputs);
        compute_end = std::chrono::high_resolution_clock::now();
        total_compute_time += std::chrono::duration_cast<std::chrono::microseconds>(
            compute_end - compute_start).count();
        
        // Gradient synchronization (communication)
        auto comm_start = std::chrono::high_resolution_clock::now();
        layer.synchronize_gradients();
        auto comm_end = std::chrono::high_resolution_clock::now();
        total_comm_time += std::chrono::duration_cast<std::chrono::microseconds>(
            comm_end - comm_start).count();
        
        layer.zero_grad();
        
        auto end = std::chrono::high_resolution_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        state.SetIterationTime(elapsed.count() / 1e6);
    }
    
    double avg_compute_ms = total_compute_time / state.iterations() / 1000.0;
    double avg_comm_ms = total_comm_time / state.iterations() / 1000.0;
    double comm_ratio = avg_comm_ms / (avg_compute_ms + avg_comm_ms);
    
    state.counters["compute_ms"] = avg_compute_ms;
    state.counters["comm_ms"] = avg_comm_ms;
    state.counters["comm_ratio"] = comm_ratio;
    state.counters["num_gpus"] = num_gpus;
    state.counters["batch_size"] = batch_size;
    
    state.SetLabel(std::to_string(num_gpus) + " GPUs, " + 
                   std::to_string(static_cast<int>(comm_ratio * 100)) + "% comm");
}

BENCHMARK(BM_CommCompute_Ratio)
    ->Args({2, 8})->Args({2, 16})->Args({2, 32})
    ->Args({4, 16})->Args({4, 32})
    ->UseManualTime();

BENCHMARK_MAIN();

#endif  // THEMIS_ENABLE_GPU
