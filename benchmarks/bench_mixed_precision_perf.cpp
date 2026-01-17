#include <benchmark/benchmark.h>
#include "llm/lora_framework/mixed_precision.h"
#include "llm/lora_framework/gpu_lora_layers.h"
#include "llm/lora_framework/gpu_tensor.h"
#include "llm/lora_framework/gpu_memory.h"
#include <chrono>
#include <vector>

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
        if (backend.type == acceleration::BackendType::CUDA && backend.available) {
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
    
    // Mixed precision trainer (disabled for FP32)
    MixedPrecisionConfig config;
    config.mode = PrecisionMode::FP32;
    MixedPrecisionTrainer mp_trainer(config);
    
    float learning_rate = 0.001f;
    
    // Warmup
    for (int i = 0; i < WARMUP_ITERS; ++i) {
        auto output = layer.forward(input);
        auto grad_output = output - target;
        auto grad_input = layer.backward(grad_output);
        
        auto params = layer.parameters();
        auto grads = layer.gradients();
        for (size_t i = 0; i < params.size(); ++i) {
            auto scaled_grad = (*grads[i]) * learning_rate;
            *params[i] = *params[i] - scaled_grad;
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
        for (size_t i = 0; i < params.size(); ++i) {
            auto scaled_grad = (*grads[i]) * learning_rate;
            *params[i] = *params[i] - scaled_grad;
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
    
    // Create layer with FP32 master weights
    GPULoRALayer layer(HIDDEN_DIM, HIDDEN_DIM, LORA_RANK, 1.0f, Device::cuda(), true);
    
    // Create tensors in FP16 for forward/backward
    GPUTensor input({batch_size, HIDDEN_DIM}, Device::cuda(), DType::FLOAT16);
    GPUTensor target({batch_size, HIDDEN_DIM}, Device::cuda(), DType::FLOAT16);
    input.fill(0.5f);
    target.fill(0.3f);
    
    // Mixed precision trainer
    MixedPrecisionConfig config;
    config.mode = PrecisionMode::FP16;
    config.loss_scale = 1024.0f;
    config.dynamic_loss_scaling = true;
    MixedPrecisionTrainer mp_trainer(config);
    
    float learning_rate = 0.001f;
    
    // Warmup
    for (int i = 0; i < WARMUP_ITERS; ++i) {
        auto output = layer.forward(input);
        auto grad_output = output - target;
        
        // Scale loss for FP16
        float loss_value = 0.5f;  // Dummy loss
        float scaled_loss = mp_trainer.scale_loss(loss_value);
        
        auto grad_input = layer.backward(grad_output);
        
        // Unscale gradients
        auto grads = layer.gradients();
        std::vector<Tensor*> grad_ptrs;
        // Note: Simplified - real implementation would handle GPUTensor
        bool no_overflow = mp_trainer.unscale_gradients(grad_ptrs);
        
        if (no_overflow) {
            auto params = layer.parameters();
            for (size_t i = 0; i < params.size(); ++i) {
                auto scaled_grad = (*grads[i]) * learning_rate;
                *params[i] = *params[i] - scaled_grad;
            }
        }
        
        layer.zero_grad();
        mp_trainer.update_loss_scale(!no_overflow);
    }
    
    // Measure
    for (auto _ : state) {
        auto start = std::chrono::high_resolution_clock::now();
        
        auto output = layer.forward(input);
        auto grad_output = output - target;
        
        float loss_value = 0.5f;
        float scaled_loss = mp_trainer.scale_loss(loss_value);
        
        auto grad_input = layer.backward(grad_output);
        
        auto grads = layer.gradients();
        std::vector<Tensor*> grad_ptrs;
        bool no_overflow = mp_trainer.unscale_gradients(grad_ptrs);
        
        if (no_overflow) {
            auto params = layer.parameters();
            for (size_t i = 0; i < params.size(); ++i) {
                auto scaled_grad = (*grads[i]) * learning_rate;
                *params[i] = *params[i] - scaled_grad;
            }
        }
        
        layer.zero_grad();
        mp_trainer.update_loss_scale(!no_overflow);
        
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
// Automatic Mixed Precision (AMP)
// ============================================================================

static void BM_Training_AMP(benchmark::State& state) {
    if (!cuda_available()) {
        state.SkipWithError("CUDA not available");
        return;
    }
    
    size_t batch_size = state.range(0);
    
    // Create layer with FP32 master weights
    GPULoRALayer layer(HIDDEN_DIM, HIDDEN_DIM, LORA_RANK, 1.0f, Device::cuda(), true);
    
    // Create tensors (AMP will auto-cast)
    GPUTensor input({batch_size, HIDDEN_DIM}, Device::cuda(), DType::FLOAT32);
    GPUTensor target({batch_size, HIDDEN_DIM}, Device::cuda(), DType::FLOAT32);
    input.fill(0.5f);
    target.fill(0.3f);
    
    // AMP trainer
    MixedPrecisionConfig config;
    config.mode = PrecisionMode::AMP;
    config.loss_scale = 2048.0f;
    config.dynamic_loss_scaling = true;
    MixedPrecisionTrainer mp_trainer(config);
    
    float learning_rate = 0.001f;
    
    // Warmup
    for (int i = 0; i < WARMUP_ITERS; ++i) {
        // Auto-cast to FP16 for forward pass
        auto input_fp16 = mp_trainer.to_lower_precision(input);
        auto output = layer.forward(input_fp16);
        auto output_fp32 = mp_trainer.to_fp32(output);
        
        auto grad_output = output_fp32 - target;
        float loss_value = 0.5f;
        float scaled_loss = mp_trainer.scale_loss(loss_value);
        
        auto grad_output_fp16 = mp_trainer.to_lower_precision(grad_output);
        auto grad_input = layer.backward(grad_output_fp16);
        
        auto grads = layer.gradients();
        std::vector<Tensor*> grad_ptrs;
        bool no_overflow = mp_trainer.unscale_gradients(grad_ptrs);
        
        if (no_overflow) {
            auto params = layer.parameters();
            for (size_t i = 0; i < params.size(); ++i) {
                auto scaled_grad = (*grads[i]) * learning_rate;
                *params[i] = *params[i] - scaled_grad;
            }
        }
        
        layer.zero_grad();
        mp_trainer.update_loss_scale(!no_overflow);
    }
    
    // Measure
    for (auto _ : state) {
        auto start = std::chrono::high_resolution_clock::now();
        
        auto input_fp16 = mp_trainer.to_lower_precision(input);
        auto output = layer.forward(input_fp16);
        auto output_fp32 = mp_trainer.to_fp32(output);
        
        auto grad_output = output_fp32 - target;
        float loss_value = 0.5f;
        float scaled_loss = mp_trainer.scale_loss(loss_value);
        
        auto grad_output_fp16 = mp_trainer.to_lower_precision(grad_output);
        auto grad_input = layer.backward(grad_output_fp16);
        
        auto grads = layer.gradients();
        std::vector<Tensor*> grad_ptrs;
        bool no_overflow = mp_trainer.unscale_gradients(grad_ptrs);
        
        if (no_overflow) {
            auto params = layer.parameters();
            for (size_t i = 0; i < params.size(); ++i) {
                auto scaled_grad = (*grads[i]) * learning_rate;
                *params[i] = *params[i] - scaled_grad;
            }
        }
        
        layer.zero_grad();
        mp_trainer.update_loss_scale(!no_overflow);
        
        auto end = std::chrono::high_resolution_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        state.SetIterationTime(elapsed.count() / 1e6);
    }
    
    double samples_per_sec = batch_size / (state.iterations() * state.max_iterations * 1e-6);
    state.counters["samples/sec"] = samples_per_sec;
    state.counters["batch_size"] = batch_size;
    
    state.SetLabel("AMP");
}

BENCHMARK(BM_Training_AMP)
    ->Arg(4)->Arg(8)->Arg(16)->Arg(32)
    ->UseManualTime();

// ============================================================================
// Loss Scaling Overhead
// ============================================================================

static void BM_LossScaling_Overhead(benchmark::State& state) {
    if (!cuda_available()) {
        state.SkipWithError("CUDA not available");
        return;
    }
    
    bool dynamic_scaling = state.range(0) == 1;
    
    MixedPrecisionConfig config;
    config.mode = PrecisionMode::FP16;
    config.loss_scale = 1024.0f;
    config.dynamic_loss_scaling = dynamic_scaling;
    MixedPrecisionTrainer mp_trainer(config);
    
    // Create dummy gradients
    size_t num_params = 10;
    std::vector<Tensor> tensors;
    std::vector<Tensor*> grad_ptrs;
    
    for (size_t i = 0; i < num_params; ++i) {
        tensors.emplace_back(std::vector<size_t>{1024, 1024});
        tensors.back().fill(0.001f);
        grad_ptrs.push_back(&tensors.back());
    }
    
    // Warmup
    for (int i = 0; i < WARMUP_ITERS; ++i) {
        float loss = 0.5f;
        float scaled_loss = mp_trainer.scale_loss(loss);
        bool no_overflow = mp_trainer.unscale_gradients(grad_ptrs);
        mp_trainer.update_loss_scale(!no_overflow);
    }
    
    // Measure
    for (auto _ : state) {
        auto start = std::chrono::high_resolution_clock::now();
        
        float loss = 0.5f;
        float scaled_loss = mp_trainer.scale_loss(loss);
        bool no_overflow = mp_trainer.unscale_gradients(grad_ptrs);
        mp_trainer.update_loss_scale(!no_overflow);
        
        auto end = std::chrono::high_resolution_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        state.SetIterationTime(elapsed.count() / 1e6);
    }
    
    state.counters["dynamic_scaling"] = dynamic_scaling ? 1 : 0;
    state.counters["overhead_us"] = state.iterations() * state.max_iterations * 1e6;
    
    state.SetLabel(dynamic_scaling ? "Dynamic" : "Static");
}

BENCHMARK(BM_LossScaling_Overhead)
    ->Arg(0)  // Static scaling
    ->Arg(1)  // Dynamic scaling
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
    std::vector<GPUTensor> activations;
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
