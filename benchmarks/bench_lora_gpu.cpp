#include <benchmark/benchmark.h>
#include "llm/lora_framework/gpu_tensor.h"
#include "llm/lora_framework/lora_layers.h"
#include <chrono>

#ifndef THEMIS_ENABLE_GPU

static void BM_LoRAGPU_Disabled(benchmark::State& state) {
    for (auto _ : state) {
        state.SkipWithError("LoRA GPU benchmarks are disabled in this build");
        break;
    }
}
// Disabled: LoRA GPU operations require CUDA runner | Deadline: v1.9.0 | Issue: #5
BENCHMARK(BM_LoRAGPU_Disabled);

BENCHMARK_MAIN();

#else

using namespace themis::llm::lora;

// Benchmark configurations
constexpr size_t SMALL_SIZE = 256;
constexpr size_t MEDIUM_SIZE = 768;
constexpr size_t LARGE_SIZE = 2048;

// ============================================================================
// Element-wise Operations Benchmarks
// ============================================================================

static void BM_ElementwiseAdd_CPU(benchmark::State& state) {
    size_t size = state.range(0);
    GPUTensor a({size, size}, Device::cpu());
    GPUTensor b({size, size}, Device::cpu());
    a.fill(1.0f);
    b.fill(2.0f);
    
    for (auto _ : state) {
        auto c = a + b;
        benchmark::DoNotOptimize(c);
    }
    
    state.SetItemsProcessed(state.iterations() * size * size);
}

static void BM_ElementwiseAdd_CUDA(benchmark::State& state) {
    // Check if CUDA is available
    auto backends = GPUMemoryManager::detect_backends();
    bool has_cuda = false;
    for (const auto& backend : backends) {
        if (backend.type == themis::acceleration::BackendType::CUDA && backend.available) {
            has_cuda = true;
            break;
        }
    }
    
    if (!has_cuda) {
        state.SkipWithError("CUDA not available");
        return;
    }
    
    size_t size = state.range(0);
    GPUTensor a({size, size}, Device::cuda());
    GPUTensor b({size, size}, Device::cuda());
    a.fill(1.0f);
    b.fill(2.0f);
    
    for (auto _ : state) {
        auto c = a + b;
        benchmark::DoNotOptimize(c);
    }
    
    state.SetItemsProcessed(state.iterations() * size * size);
}

BENCHMARK(BM_ElementwiseAdd_CPU)->Arg(SMALL_SIZE)->Arg(MEDIUM_SIZE)->Arg(LARGE_SIZE);
BENCHMARK(BM_ElementwiseAdd_CUDA)->Arg(SMALL_SIZE)->Arg(MEDIUM_SIZE)->Arg(LARGE_SIZE);

// ============================================================================
// Matrix Multiplication Benchmarks
// ============================================================================

static void BM_MatMul_CPU(benchmark::State& state) {
    size_t size = state.range(0);
    GPUTensor a({size, size}, Device::cpu());
    GPUTensor b({size, size}, Device::cpu());
    a.fill(1.0f);
    b.fill(2.0f);
    
    for (auto _ : state) {
        auto c = a.matmul(b);
        benchmark::DoNotOptimize(c);
    }
    
    // FLOPs: 2 * M * N * K for matrix multiplication
    state.SetItemsProcessed(state.iterations() * 2 * size * size * size);
}

static void BM_MatMul_CUDA(benchmark::State& state) {
    auto backends = GPUMemoryManager::detect_backends();
    bool has_cuda = false;
    for (const auto& backend : backends) {
        if (backend.type == themis::acceleration::BackendType::CUDA && backend.available) {
            has_cuda = true;
            break;
        }
    }
    
    if (!has_cuda) {
        state.SkipWithError("CUDA not available");
        return;
    }
    
    size_t size = state.range(0);
    GPUTensor a({size, size}, Device::cuda());
    GPUTensor b({size, size}, Device::cuda());
    a.fill(1.0f);
    b.fill(2.0f);
    
    for (auto _ : state) {
        auto c = a.matmul(b);
        benchmark::DoNotOptimize(c);
    }
    
    state.SetItemsProcessed(state.iterations() * 2 * size * size * size);
}

BENCHMARK(BM_MatMul_CPU)->Arg(SMALL_SIZE)->Arg(MEDIUM_SIZE);
BENCHMARK(BM_MatMul_CUDA)->Arg(SMALL_SIZE)->Arg(MEDIUM_SIZE);

// ============================================================================
// Transpose Benchmarks
// ============================================================================

static void BM_Transpose_CPU(benchmark::State& state) {
    size_t size = state.range(0);
    GPUTensor a({size, size}, Device::cpu());
    a.fill(1.0f);
    
    for (auto _ : state) {
        auto b = a.transpose();
        benchmark::DoNotOptimize(b);
    }
    
    state.SetItemsProcessed(state.iterations() * size * size);
}

static void BM_Transpose_CUDA(benchmark::State& state) {
    auto backends = GPUMemoryManager::detect_backends();
    bool has_cuda = false;
    for (const auto& backend : backends) {
        if (backend.type == themis::acceleration::BackendType::CUDA && backend.available) {
            has_cuda = true;
            break;
        }
    }
    
    if (!has_cuda) {
        state.SkipWithError("CUDA not available");
        return;
    }
    
    size_t size = state.range(0);
    GPUTensor a({size, size}, Device::cuda());
    a.fill(1.0f);
    
    for (auto _ : state) {
        auto b = a.transpose();
        benchmark::DoNotOptimize(b);
    }
    
    state.SetItemsProcessed(state.iterations() * size * size);
}

BENCHMARK(BM_Transpose_CPU)->Arg(SMALL_SIZE)->Arg(MEDIUM_SIZE)->Arg(LARGE_SIZE);
BENCHMARK(BM_Transpose_CUDA)->Arg(SMALL_SIZE)->Arg(MEDIUM_SIZE)->Arg(LARGE_SIZE);

// ============================================================================
// Scalar Multiplication Benchmarks
// ============================================================================

static void BM_ScalarMul_CPU(benchmark::State& state) {
    size_t size = state.range(0);
    GPUTensor a({size, size}, Device::cpu());
    a.fill(1.0f);
    
    for (auto _ : state) {
        auto b = a * 2.5f;
        benchmark::DoNotOptimize(b);
    }
    
    state.SetItemsProcessed(state.iterations() * size * size);
}

static void BM_ScalarMul_CUDA(benchmark::State& state) {
    auto backends = GPUMemoryManager::detect_backends();
    bool has_cuda = false;
    for (const auto& backend : backends) {
        if (backend.type == themis::acceleration::BackendType::CUDA && backend.available) {
            has_cuda = true;
            break;
        }
    }
    
    if (!has_cuda) {
        state.SkipWithError("CUDA not available");
        return;
    }
    
    size_t size = state.range(0);
    GPUTensor a({size, size}, Device::cuda());
    a.fill(1.0f);
    
    for (auto _ : state) {
        auto b = a * 2.5f;
        benchmark::DoNotOptimize(b);
    }
    
    state.SetItemsProcessed(state.iterations() * size * size);
}

BENCHMARK(BM_ScalarMul_CPU)->Arg(SMALL_SIZE)->Arg(MEDIUM_SIZE)->Arg(LARGE_SIZE);
BENCHMARK(BM_ScalarMul_CUDA)->Arg(SMALL_SIZE)->Arg(MEDIUM_SIZE)->Arg(LARGE_SIZE);

// ============================================================================
// Device Transfer Benchmarks
// ============================================================================

static void BM_CPUToGPU_Transfer(benchmark::State& state) {
    auto backends = GPUMemoryManager::detect_backends();
    bool has_cuda = false;
    for (const auto& backend : backends) {
        if (backend.type == themis::acceleration::BackendType::CUDA && backend.available) {
            has_cuda = true;
            break;
        }
    }
    
    if (!has_cuda) {
        state.SkipWithError("CUDA not available");
        return;
    }
    
    size_t size = state.range(0);
    GPUTensor cpu_tensor({size, size}, Device::cpu());
    cpu_tensor.fill(1.0f);
    
    for (auto _ : state) {
        auto gpu_tensor = cpu_tensor.to(Device::cuda());
        benchmark::DoNotOptimize(gpu_tensor);
    }
    
    state.SetBytesProcessed(state.iterations() * size * size * sizeof(float));
}

static void BM_GPUToCPU_Transfer(benchmark::State& state) {
    auto backends = GPUMemoryManager::detect_backends();
    bool has_cuda = false;
    for (const auto& backend : backends) {
        if (backend.type == themis::acceleration::BackendType::CUDA && backend.available) {
            has_cuda = true;
            break;
        }
    }
    
    if (!has_cuda) {
        state.SkipWithError("CUDA not available");
        return;
    }
    
    size_t size = state.range(0);
    GPUTensor gpu_tensor({size, size}, Device::cuda());
    gpu_tensor.fill(1.0f);
    
    for (auto _ : state) {
        auto cpu_tensor = gpu_tensor.to(Device::cpu());
        benchmark::DoNotOptimize(cpu_tensor);
    }
    
    state.SetBytesProcessed(state.iterations() * size * size * sizeof(float));
}

BENCHMARK(BM_CPUToGPU_Transfer)->Arg(SMALL_SIZE)->Arg(MEDIUM_SIZE)->Arg(LARGE_SIZE);
BENCHMARK(BM_GPUToCPU_Transfer)->Arg(SMALL_SIZE)->Arg(MEDIUM_SIZE)->Arg(LARGE_SIZE);

// ============================================================================
// Combined Operations (simulating training step)
// ============================================================================

static void BM_TrainingStep_CPU(benchmark::State& state) {
    size_t size = state.range(0);
    
    // Simulate LoRA training step: forward + backward
    GPUTensor input({1, size}, Device::cpu());
    GPUTensor B({size, 8}, Device::cpu());  // Rank 8
    GPUTensor A({8, size}, Device::cpu());
    
    input.fill(1.0f);
    B.fill(0.1f);
    A.fill(0.1f);
    
    for (auto _ : state) {
        // Forward: input @ B @ A
        auto h = input.matmul(B);
        auto output = h.matmul(A);
        
        // Backward (simplified): transpose operations
        auto A_t = A.transpose();
        auto B_t = B.transpose();
        
        benchmark::DoNotOptimize(output);
        benchmark::DoNotOptimize(A_t);
        benchmark::DoNotOptimize(B_t);
    }
}

static void BM_TrainingStep_CUDA(benchmark::State& state) {
    auto backends = GPUMemoryManager::detect_backends();
    bool has_cuda = false;
    for (const auto& backend : backends) {
        if (backend.type == themis::acceleration::BackendType::CUDA && backend.available) {
            has_cuda = true;
            break;
        }
    }
    
    if (!has_cuda) {
        state.SkipWithError("CUDA not available");
        return;
    }
    
    size_t size = state.range(0);
    
    GPUTensor input({1, size}, Device::cuda());
    GPUTensor B({size, 8}, Device::cuda());
    GPUTensor A({8, size}, Device::cuda());
    
    input.fill(1.0f);
    B.fill(0.1f);
    A.fill(0.1f);
    
    for (auto _ : state) {
        // Forward: input @ B @ A
        auto h = input.matmul(B);
        auto output = h.matmul(A);
        
        // Backward (simplified)
        auto A_t = A.transpose();
        auto B_t = B.transpose();
        
        benchmark::DoNotOptimize(output);
        benchmark::DoNotOptimize(A_t);
        benchmark::DoNotOptimize(B_t);
    }
}

BENCHMARK(BM_TrainingStep_CPU)->Arg(MEDIUM_SIZE);
BENCHMARK(BM_TrainingStep_CUDA)->Arg(MEDIUM_SIZE);

BENCHMARK_MAIN();

#endif  // THEMIS_ENABLE_GPU
