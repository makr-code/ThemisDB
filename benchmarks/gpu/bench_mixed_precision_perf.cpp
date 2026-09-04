#include <benchmark/benchmark.h>
#include "llm/lora_framework/mixed_precision.h"
#include "llm/lora_framework/gpu_lora_layers.h"
#include "llm/lora_framework/gpu_tensor.h"
#include "llm/lora_framework/gpu_memory.h"
#include <chrono>
#include <vector>

#ifndef THEMIS_ENABLE_GPU

static void BM_MixedPrecision_GPUDisabled(benchmark::State& state) {
    for (auto _ : state) {
        state.SkipWithError("Mixed precision GPU benchmarks are disabled in this build");
        break;
    }
}
// Disabled: mixed precision GPU kernels require CUDA runner | Deadline: v1.9.0 | Issue: #5
BENCHMARK(BM_MixedPrecision_GPUDisabled);

BENCHMARK_MAIN();

#else

using namespace themis::llm::lora;

/**
 * @file bench_mixed_precision_perf.cpp
 * @brief Mixed Precision Performance Analysis
 * 
 * Tests:
 * - FP32 vs FP16 throughput comparison
 * - Memory usage reduction (target: 50%)
 * - Speedup from Tensor Cores (target: 2-3x)
 * - Loss scaling overhead measurement
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

static bool cuda_available() {
    auto backends = GPUMemoryManager::detect_backends();
    for (const auto& backend : backends) {
        if (backend.type == themis::acceleration::BackendType::CUDA && backend.available) {
            return true;
        }
    }
    return false;
}

static bool tensor_cores_available() {
    // Check if GPU supports Tensor Cores (compute capability >= 7.0)
    // This is a simplified check; real implementation would query device properties
    return cuda_available();
}

// ============================================================================
// FP32 Baseline Benchmarks
// ============================================================================

static void BM_Training_FP32(benchmark::State& state) {
    if (!cuda_available()) {
        state.SkipWithError("CUDA not available");
        return;
    }
    
    size_t batch_size = state.range(0);
    
    // Create layer with FP32
    GPULoRALayer layer(HIDDEN_DIM, HIDDEN_DIM, LORA_RANK, 1.0f, Device::cuda(), true);
    
    // Create tensors in FP32
    GPUTensor input({batch_size, HIDDEN_DIM}, Device::cuda(), DType::FLOAT32);
    GPUTensor target({batch_size, HIDDEN_DIM}, Device::cuda(), DType::FLOAT32);
    input.fill(0.5f);
    target.fill(0.3f);
    
    float learning_rate = 0.001f;
    
    // Warmup
    for (int i = 0; i < WARMUP_ITERS; ++i) {
        auto output = layer.forward(input);
        auto grad_output = output - target;
        auto grad_input = layer.backward(grad_output);
        
        auto params = layer.parameters();
        auto grads = layer.gradients();
        for (size_t j = 0; j < params.size(); ++j) {
            auto scaled_grad = (*grads[j]) * learning_rate;
            *params[j] = *params[j] - scaled_grad;
        }
        layer.zero_grad();
    }
    
    // Measure
    for (auto _ : state) {
        auto start = std::chrono::high_resolution_clock::now();
        
        auto output = layer.forward(input);
        auto grad_output = output - target;
        auto grad_input = layer.backward(grad_output);
        
        auto params = layer.parameters();
        auto grads = layer.gradients();
        for (size_t j = 0; j < params.size(); ++j) {
            auto scaled_grad = (*grads[j]) * learning_rate;
            *params[j] = *params[j] - scaled_grad;
        }
        layer.zero_grad();
        
        auto end = std::chrono::high_resolution_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        state.SetIterationTime(elapsed.count() / 1e6);
    }
    
    // Memory statistics
    GPUMemoryManager mem_mgr;
    auto stats = mem_mgr.get_stats(Device::cuda());
    
    double samples_per_sec = batch_size / (state.iterations() * state.max_iterations * 1e-6);
    state.counters["samples/sec"] = samples_per_sec;
    state.counters["batch_size"] = batch_size;
    state.counters["memory_MB"] = stats.allocated_bytes / (1024.0 * 1024.0);
    state.counters["speedup_vs_fp32"] = 1.0;  // Baseline
    
    state.SetLabel("FP32");
}

BENCHMARK(BM_Training_FP32)
    ->Arg(4)->Arg(8)->Arg(16)->Arg(32)
    ->UseManualTime();

// ============================================================================
// FP16 Benchmarks
// ============================================================================

static void BM_Training_FP16(benchmark::State& state) {
    if (!cuda_available()) {
        state.SkipWithError("CUDA not available");
        return;
    }
    
    size_t batch_size = state.range(0);
    
    // Create layer with FP32 master weights (standard practice)
    GPULoRALayer layer(HIDDEN_DIM, HIDDEN_DIM, LORA_RANK, 1.0f, Device::cuda(), true);
    
    // Create tensors in FP16 for forward/backward
    GPUTensor input({batch_size, HIDDEN_DIM}, Device::cuda(), DType::FLOAT16);
    GPUTensor target({batch_size, HIDDEN_DIM}, Device::cuda(), DType::FLOAT16);
    input.fill(0.5f);
    target.fill(0.3f);
    
    float learning_rate = 0.001f;
    
    // Warmup
    for (int i = 0; i < WARMUP_ITERS; ++i) {
        auto output = layer.forward(input);
        auto grad_output = output - target;
        auto grad_input = layer.backward(grad_output);
        
        auto params = layer.parameters();
        auto grads = layer.gradients();
        for (size_t j = 0; j < params.size(); ++j) {
            auto scaled_grad = (*grads[j]) * learning_rate;
            *params[j] = *params[j] - scaled_grad;
        }
        layer.zero_grad();
    }
    
    // Measure
    for (auto _ : state) {
        auto start = std::chrono::high_resolution_clock::now();
        
        auto output = layer.forward(input);
        auto grad_output = output - target;
        auto grad_input = layer.backward(grad_output);
        
        auto params = layer.parameters();
        auto grads = layer.gradients();
        for (size_t j = 0; j < params.size(); ++j) {
            auto scaled_grad = (*grads[j]) * learning_rate;
            *params[j] = *params[j] - scaled_grad;
        }
        layer.zero_grad();
        
        auto end = std::chrono::high_resolution_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        state.SetIterationTime(elapsed.count() / 1e6);
    }
    
    // Memory statistics
    GPUMemoryManager mem_mgr;
    auto stats = mem_mgr.get_stats(Device::cuda());
    
    double samples_per_sec = batch_size / (state.iterations() * state.max_iterations * 1e-6);
    state.counters["samples/sec"] = samples_per_sec;
    state.counters["batch_size"] = batch_size;
    state.counters["memory_MB"] = stats.allocated_bytes / (1024.0 * 1024.0);
    state.counters["target_speedup"] = 2.0;  // Target 2x
    state.counters["target_memory_reduction"] = 0.5;  // Target 50% reduction
    
    state.SetLabel("FP16");
}

BENCHMARK(BM_Training_FP16)
    ->Arg(4)->Arg(8)->Arg(16)->Arg(32)
    ->UseManualTime();

// ============================================================================
// Memory Usage Comparison
// ============================================================================

static void BM_Memory_FP32_vs_FP16(benchmark::State& state) {
    if (!cuda_available()) {
        state.SkipWithError("CUDA not available");
        return;
    }
    
    bool use_fp16 = state.range(0) == 1;
    size_t batch_size = 16;
    
    GPUMemoryManager mem_mgr;
    auto initial_stats = mem_mgr.get_stats(Device::cuda());
    
    // Create layer
    GPULoRALayer layer(HIDDEN_DIM, HIDDEN_DIM, LORA_RANK, 1.0f, Device::cuda(), true);
    
    // Create tensors
    DType dtype = use_fp16 ? DType::FLOAT16 : DType::FLOAT32;
    GPUTensor input({batch_size, HIDDEN_DIM}, Device::cuda(), dtype);
    GPUTensor target({batch_size, HIDDEN_DIM}, Device::cuda(), dtype);
    input.fill(0.5f);
    target.fill(0.3f);
    
    // Allocate activation cache (simulated)
    std::vector<GPUTensor> activations = {};

    for (int i = 0; i < 10; ++i) {
        activations.emplace_back(std::vector<size_t>{batch_size, HIDDEN_DIM}, Device::cuda(), dtype);
        activations.back().fill(0.1f);
    }
    
    auto final_stats = mem_mgr.get_stats(Device::cuda());
    
    for (auto _ : state) {
        // Just measure memory, no computation
        benchmark::DoNotOptimize(activations);
    }
    
    double allocated_mb = (final_stats.allocated_bytes - initial_stats.allocated_bytes) / (1024.0 * 1024.0);
    state.counters["memory_MB"] = allocated_mb;
    state.counters["precision"] = use_fp16 ? 16 : 32;
    
    state.SetLabel(use_fp16 ? "FP16" : "FP32");
}

BENCHMARK(BM_Memory_FP32_vs_FP16)
    ->Arg(0)  // FP32
    ->Arg(1); // FP16

// ============================================================================
// Tensor Core Speedup (CUDA-specific)
// ============================================================================

static void BM_TensorCore_Speedup(benchmark::State& state) {
    if (!tensor_cores_available()) {
        state.SkipWithError("Tensor Cores not available");
        return;
    }
    
    bool use_tensor_cores = state.range(0) == 1;
    size_t batch_size = 32;
    
    // Create layer with FP16 (required for Tensor Cores)
    GPULoRALayer layer(HIDDEN_DIM, HIDDEN_DIM, LORA_RANK, 1.0f, Device::cuda(), true);
    
    DType dtype = use_tensor_cores ? DType::FLOAT16 : DType::FLOAT32;
    GPUTensor input({batch_size, HIDDEN_DIM}, Device::cuda(), dtype);
    GPUTensor target({batch_size, HIDDEN_DIM}, Device::cuda(), dtype);
    input.fill(0.5f);
    target.fill(0.3f);
    
    // Warmup
    for (int i = 0; i < WARMUP_ITERS; ++i) {
        auto output = layer.forward(input);
        benchmark::DoNotOptimize(output);
    }
    
    // Measure
    for (auto _ : state) {
        auto start = std::chrono::high_resolution_clock::now();
        auto output = layer.forward(input);
        benchmark::DoNotOptimize(output);
        auto end = std::chrono::high_resolution_clock::now();
        
        auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        state.SetIterationTime(elapsed.count() / 1e6);
    }
    
    state.counters["tensor_cores"] = use_tensor_cores ? 1 : 0;
    state.counters["target_speedup"] = 2.5;  // Target 2-3x speedup
    
    state.SetLabel(use_tensor_cores ? "TensorCores" : "CUDA_Cores");
}

BENCHMARK(BM_TensorCore_Speedup)
    ->Arg(0)  // Without Tensor Cores (FP32)
    ->Arg(1)  // With Tensor Cores (FP16)
    ->UseManualTime();

BENCHMARK_MAIN();

#endif  // THEMIS_ENABLE_GPU
