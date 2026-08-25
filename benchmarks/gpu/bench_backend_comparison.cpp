#include <benchmark/benchmark.h>
#include "llm/lora_framework/gpu_lora_layers.h"
#include "llm/lora_framework/gpu_tensor.h"
#include "llm/lora_framework/gpu_memory.h"
#include "acceleration/compute_backend.h"
#include <chrono>
#include <vector>

#ifndef THEMIS_ENABLE_GPU

static void BM_BackendComparison_GPUDisabled(benchmark::State& state) {
    for (auto _ : state) {
        state.SkipWithError("GPU backend benchmarks are disabled in this build");
        break;
    }
}

// Disabled: GPU/CUDA backend unavailable on current CI runner | Deadline: v1.9.0 | Issue: #5
BENCHMARK(BM_BackendComparison_GPUDisabled);
BENCHMARK_MAIN();

#else

using namespace themis::llm::lora;
namespace accel = themis::acceleration;

/**
 * @file bench_backend_comparison.cpp
 * @brief Backend Comparison Benchmarks
 * 
 * Tests:
 * - CUDA vs HIP performance
 * - Vulkan fallback overhead
 * - CPU baseline comparison
 * - Backend initialization cost
 */

// Benchmark configurations
constexpr size_t HIDDEN_DIM = 768;
constexpr size_t LORA_RANK = 8;
constexpr size_t BATCH_SIZE = 16;
constexpr int WARMUP_ITERS = 3;

// ============================================================================
// Helper Functions
// ============================================================================

static std::vector<GPUMemoryManager::BackendInfo> get_available_backends() {
    return GPUMemoryManager::detect_backends();
}

static bool backend_available(accel::BackendType type) {
    auto backends = get_available_backends();
    for (const auto& backend : backends) {
        if (backend.type == type && backend.available) {
            return true;
        }
    }
    return false;
}

// ============================================================================
// CPU Baseline
// ============================================================================

static void BM_Backend_CPU(benchmark::State& state) {
    size_t batch_size = state.range(0);
    
    // Create layer on CPU
    GPULoRALayer layer(HIDDEN_DIM, HIDDEN_DIM, LORA_RANK, 1.0f, Device::cpu(), false);
    
    GPUTensor input({batch_size, HIDDEN_DIM}, Device::cpu());
    GPUTensor target({batch_size, HIDDEN_DIM}, Device::cpu());
    input.fill(0.5f);
    target.fill(0.3f);
    
    // Warmup
    for (int i = 0; i < WARMUP_ITERS; ++i) {
        auto output = layer.forward(input);
        auto grad_output = output - target;
        auto grad_input = layer.backward(grad_output);
    }
    
    // Measure
    for (auto _ : state) {
        auto start = std::chrono::high_resolution_clock::now();
        
        auto output = layer.forward(input);
        auto grad_output = output - target;
        auto grad_input = layer.backward(grad_output);
        
        auto end = std::chrono::high_resolution_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        state.SetIterationTime(elapsed.count() / 1e6);
    }
    
    double samples_per_sec = batch_size / (state.iterations() * state.max_iterations * 1e-6);
    state.counters["samples/sec"] = samples_per_sec;
    state.counters["batch_size"] = static_cast<double>(batch_size);
    state.counters["speedup_vs_cpu"] = 1.0;  // Baseline
    
    state.SetLabel("CPU");
}

BENCHMARK(BM_Backend_CPU)
    ->Arg(4)->Arg(8)->Arg(16)
    ->UseManualTime();

// ============================================================================
// CUDA Backend
// ============================================================================

static void BM_Backend_CUDA(benchmark::State& state) {
    if (!backend_available(accel::BackendType::CUDA)) {
        state.SkipWithError("CUDA not available");
        return;
    }
    
    size_t batch_size = state.range(0);
    
    // Create layer on CUDA
    GPULoRALayer layer(HIDDEN_DIM, HIDDEN_DIM, LORA_RANK, 1.0f, Device::cuda(), true);
    
    GPUTensor input({batch_size, HIDDEN_DIM}, Device::cuda());
    GPUTensor target({batch_size, HIDDEN_DIM}, Device::cuda());
    input.fill(0.5f);
    target.fill(0.3f);
    
    // Warmup
    for (int i = 0; i < WARMUP_ITERS; ++i) {
        auto output = layer.forward(input);
        auto grad_output = output - target;
        auto grad_input = layer.backward(grad_output);
    }
    
    // Measure
    for (auto _ : state) {
        auto start = std::chrono::high_resolution_clock::now();
        
        auto output = layer.forward(input);
        auto grad_output = output - target;
        auto grad_input = layer.backward(grad_output);
        
        auto end = std::chrono::high_resolution_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        state.SetIterationTime(elapsed.count() / 1e6);
    }
    
    double samples_per_sec = batch_size / (state.iterations() * state.max_iterations * 1e-6);
    state.counters["samples/sec"] = samples_per_sec;
    state.counters["batch_size"] = static_cast<double>(batch_size);
    state.counters["target_speedup"] = 3.0;  // Target 2-4x
    
    state.SetLabel("CUDA");
}

BENCHMARK(BM_Backend_CUDA)
    ->Arg(4)->Arg(8)->Arg(16)
    ->UseManualTime();

// ============================================================================
// HIP Backend (AMD)
// ============================================================================

static void BM_Backend_HIP(benchmark::State& state) {
    if (!backend_available(accel::BackendType::HIP)) {
        state.SkipWithError("HIP not available");
        return;
    }
    
    size_t batch_size = state.range(0);
    
    // Create layer on HIP
    GPULoRALayer layer(HIDDEN_DIM, HIDDEN_DIM, LORA_RANK, 1.0f, Device::hip(), true);
    
    GPUTensor input({batch_size, HIDDEN_DIM}, Device::hip());
    GPUTensor target({batch_size, HIDDEN_DIM}, Device::hip());
    input.fill(0.5f);
    target.fill(0.3f);
    
    // Warmup
    for (int i = 0; i < WARMUP_ITERS; ++i) {
        auto output = layer.forward(input);
        auto grad_output = output - target;
        auto grad_input = layer.backward(grad_output);
    }
    
    // Measure
    for (auto _ : state) {
        auto start = std::chrono::high_resolution_clock::now();
        
        auto output = layer.forward(input);
        auto grad_output = output - target;
        auto grad_input = layer.backward(grad_output);
        
        auto end = std::chrono::high_resolution_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        state.SetIterationTime(elapsed.count() / 1e6);
    }
    
    double samples_per_sec = batch_size / (state.iterations() * state.max_iterations * 1e-6);
    state.counters["samples/sec"] = samples_per_sec;
    state.counters["batch_size"] = static_cast<double>(batch_size);
    state.counters["target_speedup"] = 3.0;  // Target 2-4x
    
    state.SetLabel("HIP");
}

BENCHMARK(BM_Backend_HIP)
    ->Arg(4)->Arg(8)->Arg(16)
    ->UseManualTime();

// ============================================================================
// Vulkan Backend (Cross-platform fallback)
// ============================================================================

static void BM_Backend_Vulkan(benchmark::State& state) {
    if (!backend_available(accel::BackendType::VULKAN)) {
        state.SkipWithError("Vulkan not available");
        return;
    }
    
    size_t batch_size = state.range(0);
    
    // Create layer on Vulkan
    GPULoRALayer layer(HIDDEN_DIM, HIDDEN_DIM, LORA_RANK, 1.0f, Device::vulkan(), true);
    
    GPUTensor input({batch_size, HIDDEN_DIM}, Device::vulkan());
    GPUTensor target({batch_size, HIDDEN_DIM}, Device::vulkan());
    input.fill(0.5f);
    target.fill(0.3f);
    
    // Warmup
    for (int i = 0; i < WARMUP_ITERS; ++i) {
        auto output = layer.forward(input);
        auto grad_output = output - target;
        auto grad_input = layer.backward(grad_output);
    }
    
    // Measure
    for (auto _ : state) {
        auto start = std::chrono::high_resolution_clock::now();
        
        auto output = layer.forward(input);
        auto grad_output = output - target;
        auto grad_input = layer.backward(grad_output);
        
        auto end = std::chrono::high_resolution_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        state.SetIterationTime(elapsed.count() / 1e6);
    }
    
    double samples_per_sec = batch_size / (state.iterations() * state.max_iterations * 1e-6);
    state.counters["samples/sec"] = samples_per_sec;
    state.counters["batch_size"] = static_cast<double>(batch_size);
    
    state.SetLabel("Vulkan");
}

BENCHMARK(BM_Backend_Vulkan)
    ->Arg(4)->Arg(8)->Arg(16)
    ->UseManualTime();

// ============================================================================
// DirectX Backend (Windows)
// ============================================================================

#ifdef _WIN32
static void BM_Backend_DirectX(benchmark::State& state) {
    if (!backend_available(accel::BackendType::DIRECTX)) {
        state.SkipWithError("DirectX not available");
        return;
    }
    
    size_t batch_size = state.range(0);
    
    // Create layer on DirectX
    GPULoRALayer layer(HIDDEN_DIM, HIDDEN_DIM, LORA_RANK, 1.0f, Device::directx(), true);
    
    GPUTensor input({batch_size, HIDDEN_DIM}, Device::directx());
    GPUTensor target({batch_size, HIDDEN_DIM}, Device::directx());
    input.fill(0.5f);
    target.fill(0.3f);
    
    // Warmup
    for (int i = 0; i < WARMUP_ITERS; ++i) {
        auto output = layer.forward(input);
        auto grad_output = output - target;
        auto grad_input = layer.backward(grad_output);
    }
    
    // Measure
    for (auto _ : state) {
        auto start = std::chrono::high_resolution_clock::now();
        
        auto output = layer.forward(input);
        auto grad_output = output - target;
        auto grad_input = layer.backward(grad_output);
        
        auto end = std::chrono::high_resolution_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        state.SetIterationTime(elapsed.count() / 1e6);
    }
    
    double samples_per_sec = batch_size / (state.iterations() * state.max_iterations * 1e-6);
    state.counters["samples/sec"] = samples_per_sec;
    state.counters["batch_size"] = static_cast<double>(batch_size);
    
    state.SetLabel("DirectX");
}

BENCHMARK(BM_Backend_DirectX)
    ->Arg(4)->Arg(8)->Arg(16)
    ->UseManualTime();
#endif

// ============================================================================
// Backend Initialization Cost
// ============================================================================

static void BM_Backend_Init_CPU(benchmark::State& state) {
    for (auto _ : state) {
        auto start = std::chrono::high_resolution_clock::now();
        
        GPUMemoryManager mem_mgr(accel::BackendType::CPU);
        
        auto end = std::chrono::high_resolution_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        state.SetIterationTime(elapsed.count() / 1e6);
    }
    
    state.SetLabel("CPU Init");
}

BENCHMARK(BM_Backend_Init_CPU)->UseManualTime();

static void BM_Backend_Init_CUDA(benchmark::State& state) {
    if (!backend_available(accel::BackendType::CUDA)) {
        state.SkipWithError("CUDA not available");
        return;
    }
    
    for (auto _ : state) {
        auto start = std::chrono::high_resolution_clock::now();
        
        GPUMemoryManager mem_mgr(accel::BackendType::CUDA);
        
        auto end = std::chrono::high_resolution_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        state.SetIterationTime(elapsed.count() / 1e6);
    }
    
    state.SetLabel("CUDA Init");
}

BENCHMARK(BM_Backend_Init_CUDA)->UseManualTime();

static void BM_Backend_Init_HIP(benchmark::State& state) {
    if (!backend_available(accel::BackendType::HIP)) {
        state.SkipWithError("HIP not available");
        return;
    }
    
    for (auto _ : state) {
        auto start = std::chrono::high_resolution_clock::now();
        
        GPUMemoryManager mem_mgr(accel::BackendType::HIP);
        
        auto end = std::chrono::high_resolution_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        state.SetIterationTime(elapsed.count() / 1e6);
    }
    
    state.SetLabel("HIP Init");
}

BENCHMARK(BM_Backend_Init_HIP)->UseManualTime();

static void BM_Backend_Init_Vulkan(benchmark::State& state) {
    if (!backend_available(accel::BackendType::VULKAN)) {
        state.SkipWithError("Vulkan not available");
        return;
    }
    
    for (auto _ : state) {
        auto start = std::chrono::high_resolution_clock::now();
        
        GPUMemoryManager mem_mgr(accel::BackendType::VULKAN);
        
        auto end = std::chrono::high_resolution_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        state.SetIterationTime(elapsed.count() / 1e6);
    }
    
    state.SetLabel("Vulkan Init");
}

BENCHMARK(BM_Backend_Init_Vulkan)->UseManualTime();

// ============================================================================
// Vulkan Overhead vs CUDA
// ============================================================================

static void BM_Vulkan_Overhead(benchmark::State& state) {
    bool use_vulkan = state.range(0) == 1;
    
    Device device = use_vulkan ? Device::vulkan() : Device::cuda();
    
    if (use_vulkan && !backend_available(accel::BackendType::VULKAN)) {
        state.SkipWithError("Vulkan not available");
        return;
    }
    
    if (!use_vulkan && !backend_available(accel::BackendType::CUDA)) {
        state.SkipWithError("CUDA not available");
        return;
    }
    
    size_t batch_size = 16;
    
    GPULoRALayer layer(HIDDEN_DIM, HIDDEN_DIM, LORA_RANK, 1.0f, device, true);
    
    GPUTensor input({batch_size, HIDDEN_DIM}, device);
    GPUTensor target({batch_size, HIDDEN_DIM}, device);
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
    
    state.SetLabel(use_vulkan ? "Vulkan" : "CUDA");
}

BENCHMARK(BM_Vulkan_Overhead)
    ->Arg(0)  // CUDA
    ->Arg(1)  // Vulkan
    ->UseManualTime();

// ============================================================================
// Backend Detection Performance
// ============================================================================

static void BM_Backend_Detection(benchmark::State& state) {
    for (auto _ : state) {
        auto start = std::chrono::high_resolution_clock::now();
        
        auto backends = GPUMemoryManager::detect_backends();
        benchmark::DoNotOptimize(backends);
        
        auto end = std::chrono::high_resolution_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        state.SetIterationTime(elapsed.count() / 1e6);
    }
    
    state.SetLabel("Detection");
}

BENCHMARK(BM_Backend_Detection)->UseManualTime();

// ============================================================================
// Device Selection Overhead
// ============================================================================

static void BM_Device_Selection(benchmark::State& state) {
    for (auto _ : state) {
        auto start = std::chrono::high_resolution_clock::now();
        
        Device device = GPUMemoryManager::auto_select_device();
        benchmark::DoNotOptimize(device);
        
        auto end = std::chrono::high_resolution_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        state.SetIterationTime(elapsed.count() / 1e6);
    }
    
    state.SetLabel("Auto-select");
}

BENCHMARK(BM_Device_Selection)->UseManualTime();

// ============================================================================
// Cross-backend Memory Transfer
// ============================================================================

static void BM_CrossBackend_Transfer(benchmark::State& state) {
    if (!backend_available(accel::BackendType::CUDA)) {
        state.SkipWithError("CUDA not available");
        return;
    }
    
    size_t num_elements = 1024 * 1024;  // 4MB
    
    // Create tensor on CUDA
    GPUTensor cuda_tensor({num_elements}, Device::cuda());
    cuda_tensor.fill(1.0f);
    
    // Warmup
    for (int i = 0; i < WARMUP_ITERS; ++i) {
        auto cpu_tensor = cuda_tensor.to(Device::cpu());
        benchmark::DoNotOptimize(cpu_tensor);
    }
    
    // Measure
    for (auto _ : state) {
        auto start = std::chrono::high_resolution_clock::now();
        
        // Transfer CUDA → CPU → CUDA
        auto cpu_tensor = cuda_tensor.to(Device::cpu());
        auto back_to_gpu = cpu_tensor.to(Device::cuda());
        benchmark::DoNotOptimize(back_to_gpu);
        
        auto end = std::chrono::high_resolution_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        state.SetIterationTime(elapsed.count() / 1e6);
    }
    
    double bytes_transferred = 2 * num_elements * sizeof(float);
    double time_sec = state.iterations() * state.max_iterations * 1e-6;
    double bandwidth_gbps = bytes_transferred / (1024.0 * 1024.0 * 1024.0) / time_sec;
    
    state.counters["bandwidth_GB/s"] = bandwidth_gbps;
    
    state.SetLabel("CUDA↔CPU");
}

BENCHMARK(BM_CrossBackend_Transfer)->UseManualTime();

BENCHMARK_MAIN();

#endif  // THEMIS_ENABLE_GPU
