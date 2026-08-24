#include <benchmark/benchmark.h>
#include "llm/lora_framework/gpu_lora_layers.h"
#include <spdlog/spdlog.h>

#ifndef THEMIS_ENABLE_GPU

static void BM_FusedLoRAKernels_GPUDisabled(benchmark::State& state) {
    for (auto _ : state) {
        state.SkipWithError("Fused LoRA kernel benchmarks are disabled in this build");
        break;
    }
}

// Disabled: fused LoRA CUDA/HIP kernels require GPU runner | Deadline: v1.9.0 | Issue: #5
BENCHMARK(BM_FusedLoRAKernels_GPUDisabled);
BENCHMARK_MAIN();

#else

using namespace themis::llm::lora;

// Helper to check if CUDA is available
static bool has_cuda() {
    static bool checked = false;
    static bool available = false;
    
    if (!checked) {
        auto backends = GPUMemoryManager::detect_backends();
        for (const auto& backend : backends) {
            if (backend.type == themis::acceleration::BackendType::CUDA && backend.available) {
                available = true;
                break;
            }
        }
        checked = true;
    }
    
    return available;
}

// Helper to check if HIP is available
static bool has_hip() {
    static bool checked = false;
    static bool available = false;
    
    if (!checked) {
        auto backends = GPUMemoryManager::detect_backends();
        for (const auto& backend : backends) {
            if (backend.type == themis::acceleration::BackendType::HIP && backend.available) {
                available = true;
                break;
            }
        }
        checked = true;
    }
    
    return available;
}

// ============================================================================
// Forward Pass Benchmarks
// ============================================================================

static void BM_LoRAForward_CPU(benchmark::State& state) {
    size_t batch_size = state.range(0);
    size_t in_dim = state.range(1);
    size_t out_dim = state.range(2);
    size_t rank = state.range(3);
    
    GPULoRALayer layer(in_dim, out_dim, rank, 1.0f, Device::cpu(), false);
    
    auto B = gpu_tensor_utils::xavier_uniform({in_dim, rank}, Device::cpu());
    auto A = gpu_tensor_utils::xavier_uniform({rank, out_dim}, Device::cpu());
    layer.set_weights(B, A);
    
    GPUTensor input({batch_size, in_dim}, Device::cpu());
    input.fill(0.5f);
    
    for (auto _ : state) {
        auto output = layer.forward(input);
        benchmark::DoNotOptimize(output);
    }
    
    // Compute FLOPs: 2 matmuls (input@B, result@A)
    // First matmul: 2 * batch_size * in_dim * rank FLOPs
    // Second matmul: 2 * batch_size * rank * out_dim FLOPs
    // Total: 2 * (batch_size * in_dim * rank + batch_size * rank * out_dim)
    int64_t flops_per_iter = 2 * batch_size * in_dim * rank + 2 * batch_size * rank * out_dim;
    state.SetItemsProcessed(state.iterations() * flops_per_iter);
    state.counters["GFLOPS"] = benchmark::Counter(
        flops_per_iter * state.iterations(),
        benchmark::Counter::kIsRate,
        benchmark::Counter::kIs1000
    );
}

static void BM_LoRAForward_CUDA_Unfused(benchmark::State& state) {
    if (!has_cuda()) {
        state.SkipWithError("CUDA not available");
        return;
    }
    
    size_t batch_size = state.range(0);
    size_t in_dim = state.range(1);
    size_t out_dim = state.range(2);
    size_t rank = state.range(3);
    
    GPULoRALayer layer(in_dim, out_dim, rank, 1.0f, Device::cuda(), false);
    
    auto B = gpu_tensor_utils::xavier_uniform({in_dim, rank}, Device::cuda());
    auto A = gpu_tensor_utils::xavier_uniform({rank, out_dim}, Device::cuda());
    layer.set_weights(B, A);
    
    GPUTensor input({batch_size, in_dim}, Device::cuda());
    input.fill(0.5f);
    
    // Warmup
    for (int i = 0; i < 10; ++i) {
        auto output = layer.forward(input);
    }
    
    for (auto _ : state) {
        auto output = layer.forward(input);
        benchmark::DoNotOptimize(output);
    }
    
    int64_t flops_per_iter = 2 * batch_size * in_dim * rank + 2 * batch_size * rank * out_dim;
    state.SetItemsProcessed(state.iterations() * flops_per_iter);
    state.counters["GFLOPS"] = benchmark::Counter(
        flops_per_iter * state.iterations(),
        benchmark::Counter::kIsRate,
        benchmark::Counter::kIs1000
    );
}

static void BM_LoRAForward_CUDA_Fused(benchmark::State& state) {
    if (!has_cuda()) {
        state.SkipWithError("CUDA not available");
        return;
    }
    
    size_t batch_size = state.range(0);
    size_t in_dim = state.range(1);
    size_t out_dim = state.range(2);
    size_t rank = state.range(3);
    
    GPULoRALayer layer(in_dim, out_dim, rank, 1.0f, Device::cuda(), true);
    
    auto B = gpu_tensor_utils::xavier_uniform({in_dim, rank}, Device::cuda());
    auto A = gpu_tensor_utils::xavier_uniform({rank, out_dim}, Device::cuda());
    layer.set_weights(B, A);
    
    GPUTensor input({batch_size, in_dim}, Device::cuda());
    input.fill(0.5f);
    
    // Warmup
    for (int i = 0; i < 10; ++i) {
        auto output = layer.forward(input);
    }
    
    for (auto _ : state) {
        auto output = layer.forward(input);
        benchmark::DoNotOptimize(output);
    }
    
    int64_t flops_per_iter = 2 * batch_size * in_dim * rank + 2 * batch_size * rank * out_dim;
    state.SetItemsProcessed(state.iterations() * flops_per_iter);
    state.counters["GFLOPS"] = benchmark::Counter(
        flops_per_iter * state.iterations(),
        benchmark::Counter::kIsRate,
        benchmark::Counter::kIs1000
    );
}

static void BM_LoRAForward_HIP_Unfused(benchmark::State& state) {
    if (!has_hip()) {
        state.SkipWithError("HIP not available");
        return;
    }
    
    size_t batch_size = state.range(0);
    size_t in_dim = state.range(1);
    size_t out_dim = state.range(2);
    size_t rank = state.range(3);
    
    GPULoRALayer layer(in_dim, out_dim, rank, 1.0f, Device::hip(), false);
    
    auto B = gpu_tensor_utils::xavier_uniform({in_dim, rank}, Device::hip());
    auto A = gpu_tensor_utils::xavier_uniform({rank, out_dim}, Device::hip());
    layer.set_weights(B, A);
    
    GPUTensor input({batch_size, in_dim}, Device::hip());
    input.fill(0.5f);
    
    // Warmup
    for (int i = 0; i < 10; ++i) {
        auto output = layer.forward(input);
    }
    
    for (auto _ : state) {
        auto output = layer.forward(input);
        benchmark::DoNotOptimize(output);
    }
    
    int64_t flops_per_iter = 2 * batch_size * in_dim * rank + 2 * batch_size * rank * out_dim;
    state.SetItemsProcessed(state.iterations() * flops_per_iter);
    state.counters["GFLOPS"] = benchmark::Counter(
        flops_per_iter * state.iterations(),
        benchmark::Counter::kIsRate,
        benchmark::Counter::kIs1000
    );
}

static void BM_LoRAForward_HIP_Fused(benchmark::State& state) {
    if (!has_hip()) {
        state.SkipWithError("HIP not available");
        return;
    }
    
    size_t batch_size = state.range(0);
    size_t in_dim = state.range(1);
    size_t out_dim = state.range(2);
    size_t rank = state.range(3);
    
    GPULoRALayer layer(in_dim, out_dim, rank, 1.0f, Device::hip(), true);
    
    auto B = gpu_tensor_utils::xavier_uniform({in_dim, rank}, Device::hip());
    auto A = gpu_tensor_utils::xavier_uniform({rank, out_dim}, Device::hip());
    layer.set_weights(B, A);
    
    GPUTensor input({batch_size, in_dim}, Device::hip());
    input.fill(0.5f);
    
    // Warmup
    for (int i = 0; i < 10; ++i) {
        auto output = layer.forward(input);
    }
    
    for (auto _ : state) {
        auto output = layer.forward(input);
        benchmark::DoNotOptimize(output);
    }
    
    int64_t flops_per_iter = 2 * batch_size * in_dim * rank + 2 * batch_size * rank * out_dim;
    state.SetItemsProcessed(state.iterations() * flops_per_iter);
    state.counters["GFLOPS"] = benchmark::Counter(
        flops_per_iter * state.iterations(),
        benchmark::Counter::kIsRate,
        benchmark::Counter::kIs1000
    );
}

// ============================================================================
// Backward Pass Benchmarks
// ============================================================================

static void BM_LoRABackward_CUDA_Unfused(benchmark::State& state) {
    if (!has_cuda()) {
        state.SkipWithError("CUDA not available");
        return;
    }
    
    size_t batch_size = state.range(0);
    size_t in_dim = state.range(1);
    size_t out_dim = state.range(2);
    size_t rank = state.range(3);
    
    GPULoRALayer layer(in_dim, out_dim, rank, 1.0f, Device::cuda(), false);
    
    auto B = gpu_tensor_utils::xavier_uniform({in_dim, rank}, Device::cuda());
    auto A = gpu_tensor_utils::xavier_uniform({rank, out_dim}, Device::cuda());
    layer.set_weights(B, A);
    
    GPUTensor input({batch_size, in_dim}, Device::cuda());
    input.fill(0.5f);
    
    GPUTensor grad_output({batch_size, out_dim}, Device::cuda());
    grad_output.fill(1.0f);
    
    // Warmup
    for (int i = 0; i < 10; ++i) {
        layer.forward(input);
        layer.backward(grad_output);
    }
    
    for (auto _ : state) {
        layer.forward(input);
        auto grad_input = layer.backward(grad_output);
        benchmark::DoNotOptimize(grad_input);
    }
    
    // Backward pass has similar FLOPs to forward
    int64_t flops_per_iter = 2 * (2 * batch_size * in_dim * rank + 2 * batch_size * rank * out_dim);
    state.SetItemsProcessed(state.iterations() * flops_per_iter);
}

static void BM_LoRABackward_CUDA_Fused(benchmark::State& state) {
    if (!has_cuda()) {
        state.SkipWithError("CUDA not available");
        return;
    }
    
    size_t batch_size = state.range(0);
    size_t in_dim = state.range(1);
    size_t out_dim = state.range(2);
    size_t rank = state.range(3);
    
    GPULoRALayer layer(in_dim, out_dim, rank, 1.0f, Device::cuda(), true);
    
    auto B = gpu_tensor_utils::xavier_uniform({in_dim, rank}, Device::cuda());
    auto A = gpu_tensor_utils::xavier_uniform({rank, out_dim}, Device::cuda());
    layer.set_weights(B, A);
    
    GPUTensor input({batch_size, in_dim}, Device::cuda());
    input.fill(0.5f);
    
    GPUTensor grad_output({batch_size, out_dim}, Device::cuda());
    grad_output.fill(1.0f);
    
    // Warmup
    for (int i = 0; i < 10; ++i) {
        layer.forward(input);
        layer.backward(grad_output);
    }
    
    for (auto _ : state) {
        layer.forward(input);
        auto grad_input = layer.backward(grad_output);
        benchmark::DoNotOptimize(grad_input);
    }
    
    int64_t flops_per_iter = 2 * (2 * batch_size * in_dim * rank + 2 * batch_size * rank * out_dim);
    state.SetItemsProcessed(state.iterations() * flops_per_iter);
}

// ============================================================================
// Benchmark Configurations
// ============================================================================

// Small models (e.g., BERT-base)
// Args: batch_size, in_dim, out_dim, rank
BENCHMARK(BM_LoRAForward_CPU)->Args({4, 768, 768, 8})->Unit(benchmark::kMicrosecond);
BENCHMARK(BM_LoRAForward_CUDA_Unfused)->Args({4, 768, 768, 8})->Unit(benchmark::kMicrosecond);
BENCHMARK(BM_LoRAForward_CUDA_Fused)->Args({4, 768, 768, 8})->Unit(benchmark::kMicrosecond);
BENCHMARK(BM_LoRAForward_HIP_Unfused)->Args({4, 768, 768, 8})->Unit(benchmark::kMicrosecond);
BENCHMARK(BM_LoRAForward_HIP_Fused)->Args({4, 768, 768, 8})->Unit(benchmark::kMicrosecond);

// Varying batch sizes
BENCHMARK(BM_LoRAForward_CUDA_Unfused)->Args({1, 768, 768, 8})->Unit(benchmark::kMicrosecond);
BENCHMARK(BM_LoRAForward_CUDA_Fused)->Args({1, 768, 768, 8})->Unit(benchmark::kMicrosecond);
BENCHMARK(BM_LoRAForward_CUDA_Unfused)->Args({8, 768, 768, 8})->Unit(benchmark::kMicrosecond);
BENCHMARK(BM_LoRAForward_CUDA_Fused)->Args({8, 768, 768, 8})->Unit(benchmark::kMicrosecond);
BENCHMARK(BM_LoRAForward_CUDA_Unfused)->Args({16, 768, 768, 8})->Unit(benchmark::kMicrosecond);
BENCHMARK(BM_LoRAForward_CUDA_Fused)->Args({16, 768, 768, 8})->Unit(benchmark::kMicrosecond);
BENCHMARK(BM_LoRAForward_CUDA_Unfused)->Args({32, 768, 768, 8})->Unit(benchmark::kMicrosecond);
BENCHMARK(BM_LoRAForward_CUDA_Fused)->Args({32, 768, 768, 8})->Unit(benchmark::kMicrosecond);

// Varying ranks
BENCHMARK(BM_LoRAForward_CUDA_Unfused)->Args({8, 768, 768, 4})->Unit(benchmark::kMicrosecond);
BENCHMARK(BM_LoRAForward_CUDA_Fused)->Args({8, 768, 768, 4})->Unit(benchmark::kMicrosecond);
BENCHMARK(BM_LoRAForward_CUDA_Unfused)->Args({8, 768, 768, 16})->Unit(benchmark::kMicrosecond);
BENCHMARK(BM_LoRAForward_CUDA_Fused)->Args({8, 768, 768, 16})->Unit(benchmark::kMicrosecond);
BENCHMARK(BM_LoRAForward_CUDA_Unfused)->Args({8, 768, 768, 32})->Unit(benchmark::kMicrosecond);
BENCHMARK(BM_LoRAForward_CUDA_Fused)->Args({8, 768, 768, 32})->Unit(benchmark::kMicrosecond);

// Larger models (e.g., LLaMA-7B dimensions)
BENCHMARK(BM_LoRAForward_CUDA_Unfused)->Args({4, 4096, 4096, 16})->Unit(benchmark::kMicrosecond);
BENCHMARK(BM_LoRAForward_CUDA_Fused)->Args({4, 4096, 4096, 16})->Unit(benchmark::kMicrosecond);

// FFN dimensions (expansion factor 4)
BENCHMARK(BM_LoRAForward_CUDA_Unfused)->Args({4, 768, 3072, 16})->Unit(benchmark::kMicrosecond);
BENCHMARK(BM_LoRAForward_CUDA_Fused)->Args({4, 768, 3072, 16})->Unit(benchmark::kMicrosecond);

// Backward pass benchmarks
BENCHMARK(BM_LoRABackward_CUDA_Unfused)->Args({4, 768, 768, 8})->Unit(benchmark::kMicrosecond);
BENCHMARK(BM_LoRABackward_CUDA_Fused)->Args({4, 768, 768, 8})->Unit(benchmark::kMicrosecond);
BENCHMARK(BM_LoRABackward_CUDA_Unfused)->Args({8, 768, 768, 8})->Unit(benchmark::kMicrosecond);
BENCHMARK(BM_LoRABackward_CUDA_Fused)->Args({8, 768, 768, 8})->Unit(benchmark::kMicrosecond);
BENCHMARK(BM_LoRABackward_CUDA_Unfused)->Args({16, 768, 768, 8})->Unit(benchmark::kMicrosecond);
BENCHMARK(BM_LoRABackward_CUDA_Fused)->Args({16, 768, 768, 8})->Unit(benchmark::kMicrosecond);

// ============================================================================
// Phase 2: Optimized Kernel Benchmarks
// ============================================================================

static void BM_LoRAForward_CUDA_Optimized(benchmark::State& state) {
    if (!has_cuda()) {
        state.SkipWithError("CUDA not available");
        return;
    }
    
    size_t batch_size = state.range(0);
    size_t in_dim = state.range(1);
    size_t out_dim = state.range(2);
    size_t rank = state.range(3);
    
    GPULoRALayer layer(in_dim, out_dim, rank, 1.0f, Device::cuda(), true);
    
    auto B = gpu_tensor_utils::xavier_uniform({in_dim, rank}, Device::cuda());
    auto A = gpu_tensor_utils::xavier_uniform({rank, out_dim}, Device::cuda());
    layer.set_weights(B, A);
    
    GPUTensor input({batch_size, in_dim}, Device::cuda());
    input.fill(0.5f);
    
    // Note: This uses the current fused kernel. When optimized kernel is integrated
    // into the layer, this will automatically use the optimized version.
    // Warmup
    for (int i = 0; i < 10; ++i) {
        auto output = layer.forward(input);
    }
    
    for (auto _ : state) {
        auto output = layer.forward(input);
        benchmark::DoNotOptimize(output);
    }
    
    int64_t flops_per_iter = 2 * batch_size * in_dim * rank + 2 * batch_size * rank * out_dim;
    state.SetItemsProcessed(state.iterations() * flops_per_iter);
    state.counters["GFLOPS"] = benchmark::Counter(
        flops_per_iter * state.iterations(),
        benchmark::Counter::kIsRate,
        benchmark::Counter::kIs1000
    );
}

// Phase 2 optimized kernel benchmarks
BENCHMARK(BM_LoRAForward_CUDA_Optimized)->Args({4, 768, 768, 8})->Unit(benchmark::kMicrosecond);
BENCHMARK(BM_LoRAForward_CUDA_Optimized)->Args({8, 768, 768, 8})->Unit(benchmark::kMicrosecond);
BENCHMARK(BM_LoRAForward_CUDA_Optimized)->Args({16, 768, 768, 8})->Unit(benchmark::kMicrosecond);
BENCHMARK(BM_LoRAForward_CUDA_Optimized)->Args({32, 768, 768, 8})->Unit(benchmark::kMicrosecond);

// Compare optimized vs base fused for key configurations
BENCHMARK(BM_LoRAForward_CUDA_Fused)->Args({32, 768, 768, 16})->Unit(benchmark::kMicrosecond);
BENCHMARK(BM_LoRAForward_CUDA_Optimized)->Args({32, 768, 768, 16})->Unit(benchmark::kMicrosecond);

BENCHMARK_MAIN();

#endif  // THEMIS_ENABLE_GPU
