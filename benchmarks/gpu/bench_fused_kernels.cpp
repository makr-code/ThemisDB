#include <benchmark/benchmark.h>
#include "llm/lora_framework/gpu_lora_layers.h"
#include "llm/lora_framework/gpu_tensor.h"
#include <chrono>

#ifndef THEMIS_ENABLE_GPU

static void BM_FusedKernels_GPUDisabled(benchmark::State& state) {
    for (auto _ : state) {
        state.SkipWithError("Fused-kernel benchmarks are disabled in this build");
        break;
    }
}

// Disabled: fused CUDA/HIP kernels require GPU runner | Deadline: v1.9.0 | Issue: #5
BENCHMARK(BM_FusedKernels_GPUDisabled);
BENCHMARK_MAIN();

#else

using namespace themis::llm::lora;

// Benchmark configurations matching different LoRA use cases
constexpr size_t SMALL_RANK = 4;      // Typical for fine-tuning
constexpr size_t MEDIUM_RANK = 8;     // Common LoRA rank
constexpr size_t LARGE_RANK = 16;     // Higher quality adaptations
constexpr size_t XLARGE_RANK = 32;    // Maximum typical rank

constexpr size_t SMALL_DIM = 256;     // Small models
constexpr size_t MEDIUM_DIM = 768;    // BERT-base, GPT-2 small
constexpr size_t LARGE_DIM = 1024;    // Larger transformers
constexpr size_t XLARGE_DIM = 2048;   // GPT-3, LLaMA-7B

constexpr size_t BATCH_SIZE = 16;     // Typical batch size

// Expected performance targets (from Phase 10.3 specification)
// Forward: 1.5-1.8x speedup (3 kernels → 1 kernel)
// Backward: 1.7-2.0x speedup (4 kernels → 1 kernel)
// Optimizer: 1.3-1.5x speedup (3-4 kernels → 1 kernel)

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

static bool hip_available() {
    auto backends = GPUMemoryManager::detect_backends();
    for (const auto& backend : backends) {
        if (backend.type == themis::acceleration::BackendType::HIP && backend.available) {
            return true;
        }
    }
    return false;
}

// ============================================================================
// Forward Pass Benchmarks (Fused vs Unfused)
// ============================================================================

static void BM_LoRA_Forward_Unfused_CUDA(benchmark::State& state) {
    if (!cuda_available()) {
        state.SkipWithError("CUDA not available");
        return;
    }
    
    size_t in_dim = state.range(0);
    size_t out_dim = state.range(0);
    size_t rank = state.range(1);
    
    GPULoRALayer layer(in_dim, out_dim, rank, 1.0f, Device::cuda(), false);  // unfused
    GPUTensor input({BATCH_SIZE, in_dim}, Device::cuda());
    input.fill(0.5f);
    
    // Warmup
    for (int i = 0; i < 10; ++i) {
        auto output = layer.forward(input);
        benchmark::DoNotOptimize(output);
    }
    
    for (auto _ : state) {
        auto output = layer.forward(input);
        benchmark::DoNotOptimize(output);
    }
    
    // Calculate throughput (elements processed per second)
    int64_t elements = BATCH_SIZE * out_dim;
    state.SetItemsProcessed(state.iterations() * elements);
    state.SetLabel("Unfused");
}

static void BM_LoRA_Forward_Fused_CUDA(benchmark::State& state) {
    if (!cuda_available()) {
        state.SkipWithError("CUDA not available");
        return;
    }
    
    size_t in_dim = state.range(0);
    size_t out_dim = state.range(0);
    size_t rank = state.range(1);
    
    GPULoRALayer layer(in_dim, out_dim, rank, 1.0f, Device::cuda(), true);  // fused
    GPUTensor input({BATCH_SIZE, in_dim}, Device::cuda());
    input.fill(0.5f);
    
    // Warmup
    for (int i = 0; i < 10; ++i) {
        auto output = layer.forward(input);
        benchmark::DoNotOptimize(output);
    }
    
    for (auto _ : state) {
        auto output = layer.forward(input);
        benchmark::DoNotOptimize(output);
    }
    
    int64_t elements = BATCH_SIZE * out_dim;
    state.SetItemsProcessed(state.iterations() * elements);
    state.SetLabel("Fused");
}

// Register forward pass benchmarks with different configurations
BENCHMARK(BM_LoRA_Forward_Unfused_CUDA)
    ->Args({MEDIUM_DIM, SMALL_RANK})    // 768, rank=4
    ->Args({MEDIUM_DIM, MEDIUM_RANK})   // 768, rank=8
    ->Args({MEDIUM_DIM, LARGE_RANK})    // 768, rank=16
    ->Args({LARGE_DIM, MEDIUM_RANK})    // 1024, rank=8
    ->Args({XLARGE_DIM, LARGE_RANK})    // 2048, rank=16
    ->Unit(benchmark::kMicrosecond);

BENCHMARK(BM_LoRA_Forward_Fused_CUDA)
    ->Args({MEDIUM_DIM, SMALL_RANK})
    ->Args({MEDIUM_DIM, MEDIUM_RANK})
    ->Args({MEDIUM_DIM, LARGE_RANK})
    ->Args({LARGE_DIM, MEDIUM_RANK})
    ->Args({XLARGE_DIM, LARGE_RANK})
    ->Unit(benchmark::kMicrosecond);

// ============================================================================
// Backward Pass Benchmarks (Fused vs Unfused)
// ============================================================================

static void BM_LoRA_Backward_Unfused_CUDA(benchmark::State& state) {
    if (!cuda_available()) {
        state.SkipWithError("CUDA not available");
        return;
    }
    
    size_t in_dim = state.range(0);
    size_t out_dim = state.range(0);
    size_t rank = state.range(1);
    
    GPULoRALayer layer(in_dim, out_dim, rank, 1.0f, Device::cuda(), false);  // unfused
    GPUTensor input({BATCH_SIZE, in_dim}, Device::cuda());
    input.fill(0.5f);
    
    // Forward pass first (required for backward)
    auto output = layer.forward(input);
    
    GPUTensor grad_output({BATCH_SIZE, out_dim}, Device::cuda());
    grad_output.fill(1.0f);
    
    // Warmup
    for (int i = 0; i < 10; ++i) {
        auto grad_input = layer.backward(grad_output);
        benchmark::DoNotOptimize(grad_input);
    }
    
    for (auto _ : state) {
        auto grad_input = layer.backward(grad_output);
        benchmark::DoNotOptimize(grad_input);
    }
    
    int64_t elements = BATCH_SIZE * in_dim;
    state.SetItemsProcessed(state.iterations() * elements);
    state.SetLabel("Unfused");
}

static void BM_LoRA_Backward_Fused_CUDA(benchmark::State& state) {
    if (!cuda_available()) {
        state.SkipWithError("CUDA not available");
        return;
    }
    
    size_t in_dim = state.range(0);
    size_t out_dim = state.range(0);
    size_t rank = state.range(1);
    
    GPULoRALayer layer(in_dim, out_dim, rank, 1.0f, Device::cuda(), true);  // fused
    GPUTensor input({BATCH_SIZE, in_dim}, Device::cuda());
    input.fill(0.5f);
    
    // Forward pass first
    auto output = layer.forward(input);
    
    GPUTensor grad_output({BATCH_SIZE, out_dim}, Device::cuda());
    grad_output.fill(1.0f);
    
    // Warmup
    for (int i = 0; i < 10; ++i) {
        auto grad_input = layer.backward(grad_output);
        benchmark::DoNotOptimize(grad_input);
    }
    
    for (auto _ : state) {
        auto grad_input = layer.backward(grad_output);
        benchmark::DoNotOptimize(grad_input);
    }
    
    int64_t elements = BATCH_SIZE * in_dim;
    state.SetItemsProcessed(state.iterations() * elements);
    state.SetLabel("Fused");
}

BENCHMARK(BM_LoRA_Backward_Unfused_CUDA)
    ->Args({MEDIUM_DIM, SMALL_RANK})
    ->Args({MEDIUM_DIM, MEDIUM_RANK})
    ->Args({MEDIUM_DIM, LARGE_RANK})
    ->Args({LARGE_DIM, MEDIUM_RANK})
    ->Args({XLARGE_DIM, LARGE_RANK})
    ->Unit(benchmark::kMicrosecond);

BENCHMARK(BM_LoRA_Backward_Fused_CUDA)
    ->Args({MEDIUM_DIM, SMALL_RANK})
    ->Args({MEDIUM_DIM, MEDIUM_RANK})
    ->Args({MEDIUM_DIM, LARGE_RANK})
    ->Args({LARGE_DIM, MEDIUM_RANK})
    ->Args({XLARGE_DIM, LARGE_RANK})
    ->Unit(benchmark::kMicrosecond);

// ============================================================================
// Optimizer Step Benchmarks (Fused vs Unfused)
// ============================================================================

static void BM_SGD_Step_Unfused_CUDA(benchmark::State& state) {
    if (!cuda_available()) {
        state.SkipWithError("CUDA not available");
        return;
    }
    
    size_t param_size = state.range(0);
    
    GPUTensor param({param_size}, Device::cuda());
    param.fill(1.0f);
    param.requires_grad = true;
    param.grad = std::make_unique<GPUTensor>(GPUTensor({param_size}, Device::cuda()));
    param.grad->fill(0.1f);
    
    // Note: Optimizer fusion is internal, tested via layer training
    // This benchmark measures parameter update time
    float lr = 0.01f;
    
    for (auto _ : state) {
        // Simulate unfused optimizer: separate operations
        auto grad = param.grad->clone();
        auto update = grad * lr;
        param = param - update;
        benchmark::DoNotOptimize(param);
    }
    
    state.SetItemsProcessed(state.iterations() * param_size);
    state.SetLabel("Unfused");
}

// ============================================================================
// Full Training Step Benchmarks (End-to-End)
// ============================================================================

static void BM_LoRA_TrainingStep_Unfused_CUDA(benchmark::State& state) {
    if (!cuda_available()) {
        state.SkipWithError("CUDA not available");
        return;
    }
    
    size_t dim = state.range(0);
    size_t rank = state.range(1);
    
    GPULoRALayer layer(dim, dim, rank, 1.0f, Device::cuda(), false);  // unfused
    GPUSGDOptimizer optimizer(0.001f, 0.0f, 0.0f);
    optimizer.add_parameters(layer.parameters());
    
    GPUTensor input({BATCH_SIZE, dim}, Device::cuda());
    GPUTensor target({BATCH_SIZE, dim}, Device::cuda());
    input.fill(0.5f);
    target.fill(0.6f);
    
    // Warmup
    for (int i = 0; i < 5; ++i) {
        auto output = layer.forward(input);
        auto diff = output - target;
        auto grad = diff * (2.0f / BATCH_SIZE);
        auto grad_input = layer.backward(grad);
        optimizer.step();
        layer.zero_grad();
    }
    
    for (auto _ : state) {
        auto output = layer.forward(input);
        auto diff = output - target;
        auto grad = diff * (2.0f / BATCH_SIZE);
        auto grad_input = layer.backward(grad);
        optimizer.step();
        layer.zero_grad();
        benchmark::DoNotOptimize(grad_input);
    }
    
    state.SetLabel("Unfused (target: baseline)");
}

static void BM_LoRA_TrainingStep_Fused_CUDA(benchmark::State& state) {
    if (!cuda_available()) {
        state.SkipWithError("CUDA not available");
        return;
    }
    
    size_t dim = state.range(0);
    size_t rank = state.range(1);
    
    GPULoRALayer layer(dim, dim, rank, 1.0f, Device::cuda(), true);  // fused
    GPUSGDOptimizer optimizer(0.001f, 0.0f, 0.0f);
    optimizer.add_parameters(layer.parameters());
    
    GPUTensor input({BATCH_SIZE, dim}, Device::cuda());
    GPUTensor target({BATCH_SIZE, dim}, Device::cuda());
    input.fill(0.5f);
    target.fill(0.6f);
    
    // Warmup
    for (int i = 0; i < 5; ++i) {
        auto output = layer.forward(input);
        auto diff = output - target;
        auto grad = diff * (2.0f / BATCH_SIZE);
        auto grad_input = layer.backward(grad);
        optimizer.step();
        layer.zero_grad();
    }
    
    for (auto _ : state) {
        auto output = layer.forward(input);
        auto diff = output - target;
        auto grad = diff * (2.0f / BATCH_SIZE);
        auto grad_input = layer.backward(grad);
        optimizer.step();
        layer.zero_grad();
        benchmark::DoNotOptimize(grad_input);
    }
    
    state.SetLabel("Fused (target: 1.5-2x faster)");
}

BENCHMARK(BM_LoRA_TrainingStep_Unfused_CUDA)
    ->Args({MEDIUM_DIM, MEDIUM_RANK})   // 768, rank=8 (realistic BERT fine-tuning)
    ->Args({LARGE_DIM, LARGE_RANK})     // 1024, rank=16
    ->Args({XLARGE_DIM, LARGE_RANK})    // 2048, rank=16 (larger models)
    ->Unit(benchmark::kMicrosecond);

BENCHMARK(BM_LoRA_TrainingStep_Fused_CUDA)
    ->Args({MEDIUM_DIM, MEDIUM_RANK})
    ->Args({LARGE_DIM, LARGE_RANK})
    ->Args({XLARGE_DIM, LARGE_RANK})
    ->Unit(benchmark::kMicrosecond);

// ============================================================================
// HIP Benchmarks (AMD GPU)
// ============================================================================

static void BM_LoRA_Forward_Fused_HIP(benchmark::State& state) {
    if (!hip_available()) {
        state.SkipWithError("HIP not available");
        return;
    }
    
    size_t in_dim = state.range(0);
    size_t out_dim = state.range(0);
    size_t rank = state.range(1);
    
    GPULoRALayer layer(in_dim, out_dim, rank, 1.0f, Device::hip(), true);
    GPUTensor input({BATCH_SIZE, in_dim}, Device::hip());
    input.fill(0.5f);
    
    for (auto _ : state) {
        auto output = layer.forward(input);
        benchmark::DoNotOptimize(output);
    }
    
    int64_t elements = BATCH_SIZE * out_dim;
    state.SetItemsProcessed(state.iterations() * elements);
    state.SetLabel("Fused (HIP)");
}

BENCHMARK(BM_LoRA_Forward_Fused_HIP)
    ->Args({MEDIUM_DIM, MEDIUM_RANK})
    ->Unit(benchmark::kMicrosecond);

// ============================================================================
// Memory Bandwidth Benchmarks
// ============================================================================

static void BM_MemoryBandwidth_Unfused_CUDA(benchmark::State& state) {
    if (!cuda_available()) {
        state.SkipWithError("CUDA not available");
        return;
    }
    
    size_t dim = state.range(0);
    size_t rank = state.range(1);
    
    GPULoRALayer layer(dim, dim, rank, 1.0f, Device::cuda(), false);
    GPUTensor input({BATCH_SIZE, dim}, Device::cuda());
    input.fill(0.5f);
    
    // Measure forward + backward (full training step memory traffic)
    for (auto _ : state) {
        auto output = layer.forward(input);
        GPUTensor grad_output({BATCH_SIZE, dim}, Device::cuda());
        grad_output.fill(1.0f);
        auto grad_input = layer.backward(grad_output);
        benchmark::DoNotOptimize(grad_input);
    }
    
    // Calculate approximate memory traffic
    // Unfused: Multiple passes over data
    // Forward: 3 passes (input@B read/write, h@A read/write, scale read/write)
    // Backward: 4 passes (multiple gradient computations)
    int64_t bytes_per_iter = 
        (BATCH_SIZE * dim + dim * rank + rank * dim) * sizeof(float) * 5;  // Approximate
    
    state.SetBytesProcessed(state.iterations() * bytes_per_iter);
    state.SetLabel("Unfused (baseline memory)");
}

static void BM_MemoryBandwidth_Fused_CUDA(benchmark::State& state) {
    if (!cuda_available()) {
        state.SkipWithError("CUDA not available");
        return;
    }
    
    size_t dim = state.range(0);
    size_t rank = state.range(1);
    
    GPULoRALayer layer(dim, dim, rank, 1.0f, Device::cuda(), true);
    GPUTensor input({BATCH_SIZE, dim}, Device::cuda());
    input.fill(0.5f);
    
    for (auto _ : state) {
        auto output = layer.forward(input);
        GPUTensor grad_output({BATCH_SIZE, dim}, Device::cuda());
        grad_output.fill(1.0f);
        auto grad_input = layer.backward(grad_output);
        benchmark::DoNotOptimize(grad_input);
    }
    
    // Fused: Reduced passes (66-75% reduction in memory traffic)
    // Forward: 1 pass, Backward: 1 pass
    int64_t bytes_per_iter = 
        (BATCH_SIZE * dim + dim * rank + rank * dim) * sizeof(float) * 2;  // Reduced
    
    state.SetBytesProcessed(state.iterations() * bytes_per_iter);
    state.SetLabel("Fused (target: 66-75% less memory)");
}

BENCHMARK(BM_MemoryBandwidth_Unfused_CUDA)
    ->Args({MEDIUM_DIM, MEDIUM_RANK})
    ->Unit(benchmark::kMicrosecond);

BENCHMARK(BM_MemoryBandwidth_Fused_CUDA)
    ->Args({MEDIUM_DIM, MEDIUM_RANK})
    ->Unit(benchmark::kMicrosecond);

// ============================================================================
// Main
// ============================================================================

BENCHMARK_MAIN();

#endif  // THEMIS_ENABLE_GPU
