#include <benchmark/benchmark.h>
#include "llm/lora_framework/vulkan_kernels.h"
#include "llm/lora_framework/vulkan_context.h"
#include "llm/lora_framework/vulkan_buffer.h"
#include "llm/lora_framework/vulkan_pipeline.h"
#include <vector>
#include <random>
#include <memory>

#ifndef THEMIS_ENABLE_GPU

static void BM_VulkanLoRA_GPUDisabled(benchmark::State& state) {
    for (auto _ : state) {
        state.SkipWithError("Vulkan LoRA benchmarks are disabled in this build");
        break;
    }
}
// Disabled: Vulkan LoRA pipeline requires Vulkan-capable GPU runner | Deadline: v1.9.0 | Issue: #5
BENCHMARK(BM_VulkanLoRA_GPUDisabled);

BENCHMARK_MAIN();

#else

using namespace themis::lora::vulkan;

// ============================================================================
// Benchmark Configuration
// ============================================================================

// Standard dimensions for LoRA training
constexpr int SMALL_DIM = 256;
constexpr int MEDIUM_DIM = 768;   // Typical for BERT, GPT-2
constexpr int LARGE_DIM = 2048;   // Typical for larger models
constexpr int LORA_RANK = 8;      // Typical LoRA rank

// Expected performance targets (based on design goals)
// Keep both tracks:
// - E2E: includes host-device transfers and synchronization costs.
// - HW: hardware-near/device-resident corridor for kernel-side processing.
namespace ExpectedPerformance {
    // Matrix multiplication (768x768)
    constexpr double MATMUL_768_TARGET_E2E_MS = 12.0;
    constexpr double MATMUL_768_TARGET_HW_MS = 4.0;
    constexpr double MATMUL_768_CPU_BASELINE_MS = 10.0;
    
    // Element-wise ops (1M elements)
    constexpr double ELEMENTWISE_1M_TARGET_E2E_MS = 20.0;
    constexpr double ELEMENTWISE_1M_TARGET_HW_MS = 6.0;
    constexpr double ELEMENTWISE_1M_CPU_BASELINE_MS = 2.0;
    
    // Full training step
    constexpr double TRAINING_STEP_TARGET_E2E_MS = 10.0;
    constexpr double TRAINING_STEP_TARGET_HW_MS = 6.0;
    constexpr double TRAINING_STEP_CPU_BASELINE_MS = 160.0;
    
    // Speedup targets
    constexpr double TARGET_SPEEDUP_E2E = 16.0;
    constexpr double TARGET_SPEEDUP_HW = 25.0;
}

// ============================================================================
// Helper Functions
// ============================================================================

class VulkanBenchmarkFixture : public benchmark::Fixture {
public:
    void SetUp(const ::benchmark::State& state) override {
        // Initialize Vulkan once for all benchmarks
        static bool initialized = false;
        if (!initialized) {
            if (!is_vulkan_available()) {
                auto& mutable_state = const_cast<benchmark::State&>(state);
                mutable_state.SkipWithError("Vulkan not available");
                return;
            }
            if (!initialize_vulkan_lora(0)) {
                auto& mutable_state = const_cast<benchmark::State&>(state);
                mutable_state.SkipWithError("Failed to initialize Vulkan");
                return;
            }
            initialized = true;
        }
    }
    
    static void TearDownTestCase() {
        cleanup_vulkan_lora();
    }
    
    std::vector<float> generate_random_data(size_t size, int seed = 42) {
        std::mt19937 gen(seed);
        std::uniform_real_distribution<float> dist(-1.0f, 1.0f);
        std::vector<float> data(size);
        for (auto& val : data) {
            val = dist(gen);
        }
        return data;
    }
};

// ============================================================================
// Matrix Multiplication Benchmarks
// ============================================================================

BENCHMARK_DEFINE_F(VulkanBenchmarkFixture, MatMul_Small)(benchmark::State& state) {
    const int M = SMALL_DIM, N = SMALL_DIM, K = SMALL_DIM;
    
    auto A = generate_random_data(M * K);
    auto B = generate_random_data(K * N);
    std::vector<float> C(M * N);
    
    // Warmup
    launch_matmul_shader(A.data(), B.data(), C.data(), M, N, K, 1.0f);
    
    for (auto _ : state) {
        launch_matmul_shader(A.data(), B.data(), C.data(), M, N, K, 1.0f);
    }
    
    // Report FLOPs (2 * M * N * K for matrix multiplication)
    state.SetItemsProcessed(state.iterations() * 2LL * M * N * K);
    state.SetBytesProcessed(state.iterations() * (M * K + K * N + M * N) * sizeof(float));
    
    // Add custom counters
    state.counters["Dimension"] = M;
    state.counters["FLOPs"] = benchmark::Counter(
        static_cast<double>(2LL * M * N * K), benchmark::Counter::kIsRate);
}
BENCHMARK_REGISTER_F(VulkanBenchmarkFixture, MatMul_Small)->Unit(benchmark::kMillisecond);

BENCHMARK_DEFINE_F(VulkanBenchmarkFixture, MatMul_Medium)(benchmark::State& state) {
    const int M = MEDIUM_DIM, N = MEDIUM_DIM, K = MEDIUM_DIM;
    
    auto A = generate_random_data(M * K);
    auto B = generate_random_data(K * N);
    std::vector<float> C(M * N);
    
    // Warmup
    launch_matmul_shader(A.data(), B.data(), C.data(), M, N, K, 1.0f);
    
    for (auto _ : state) {
        launch_matmul_shader(A.data(), B.data(), C.data(), M, N, K, 1.0f);
    }
    
    state.SetItemsProcessed(state.iterations() * 2LL * M * N * K);
    state.SetBytesProcessed(state.iterations() * (M * K + K * N + M * N) * sizeof(float));
    
    state.counters["Dimension"] = M;
    state.counters["FLOPs"] = benchmark::Counter(
        static_cast<double>(2LL * M * N * K), benchmark::Counter::kIsRate);
    state.counters["Target_ms"] = ExpectedPerformance::MATMUL_768_TARGET_E2E_MS;
    state.counters["Target_E2E_ms"] = ExpectedPerformance::MATMUL_768_TARGET_E2E_MS;
    state.counters["Target_HW_ms"] = ExpectedPerformance::MATMUL_768_TARGET_HW_MS;
    state.counters["CPU_Baseline_ms"] = ExpectedPerformance::MATMUL_768_CPU_BASELINE_MS;
}
BENCHMARK_REGISTER_F(VulkanBenchmarkFixture, MatMul_Medium)->Unit(benchmark::kMillisecond);

BENCHMARK_DEFINE_F(VulkanBenchmarkFixture, MatMul_Large)(benchmark::State& state) {
    const int M = LARGE_DIM, N = LARGE_DIM, K = LARGE_DIM;
    
    auto A = generate_random_data(M * K);
    auto B = generate_random_data(K * N);
    std::vector<float> C(M * N);
    
    // Warmup
    launch_matmul_shader(A.data(), B.data(), C.data(), M, N, K, 1.0f);
    
    for (auto _ : state) {
        launch_matmul_shader(A.data(), B.data(), C.data(), M, N, K, 1.0f);
    }
    
    state.SetItemsProcessed(state.iterations() * 2 * M * N * K);
    state.SetBytesProcessed(state.iterations() * (M * K + K * N + M * N) * sizeof(float));
    
    state.counters["Dimension"] = M;
    state.counters["FLOPs"] = benchmark::Counter(
        static_cast<double>(2LL * M * N * K), benchmark::Counter::kIsRate);
}
BENCHMARK_REGISTER_F(VulkanBenchmarkFixture, MatMul_Large)->Unit(benchmark::kMillisecond);

// ============================================================================
// Element-wise Operations Benchmarks
// ============================================================================

BENCHMARK_DEFINE_F(VulkanBenchmarkFixture, ElementwiseAdd)(benchmark::State& state) {
    size_t size = state.range(0);
    
    auto A = generate_random_data(size);
    auto B = generate_random_data(size);
    std::vector<float> C(size);
    
    // Warmup
    launch_add_shader(A.data(), B.data(), C.data(), size);
    
    for (auto _ : state) {
        launch_add_shader(A.data(), B.data(), C.data(), size);
    }
    
    state.SetItemsProcessed(state.iterations() * size);
    state.SetBytesProcessed(state.iterations() * 3 * size * sizeof(float));
    
    state.counters["Elements"] = static_cast<double>(size);
    if (size == 1048576) {  // 1M elements
        state.counters["Target_ms"] = ExpectedPerformance::ELEMENTWISE_1M_TARGET_E2E_MS;
        state.counters["Target_E2E_ms"] = ExpectedPerformance::ELEMENTWISE_1M_TARGET_E2E_MS;
        state.counters["Target_HW_ms"] = ExpectedPerformance::ELEMENTWISE_1M_TARGET_HW_MS;
        state.counters["CPU_Baseline_ms"] = ExpectedPerformance::ELEMENTWISE_1M_CPU_BASELINE_MS;
    }
}
BENCHMARK_REGISTER_F(VulkanBenchmarkFixture, ElementwiseAdd)
    ->Arg(65536)      // 64K
    ->Arg(262144)     // 256K
    ->Arg(1048576)    // 1M
    ->Arg(4194304)    // 4M
    ->Unit(benchmark::kMillisecond);

BENCHMARK_DEFINE_F(VulkanBenchmarkFixture, ElementwiseMultiply)(benchmark::State& state) {
    size_t size = state.range(0);
    
    auto A = generate_random_data(size);
    auto B = generate_random_data(size);
    std::vector<float> C(size);
    
    // Warmup
    launch_multiply_shader(A.data(), B.data(), C.data(), size);
    
    for (auto _ : state) {
        launch_multiply_shader(A.data(), B.data(), C.data(), size);
    }
    
    state.SetItemsProcessed(state.iterations() * size);
    state.SetBytesProcessed(state.iterations() * 3 * size * sizeof(float));
}
BENCHMARK_REGISTER_F(VulkanBenchmarkFixture, ElementwiseMultiply)
    ->Arg(65536)
    ->Arg(262144)
    ->Arg(1048576)
    ->Arg(4194304)
    ->Unit(benchmark::kMillisecond);

BENCHMARK_DEFINE_F(VulkanBenchmarkFixture, ScalarMultiply)(benchmark::State& state) {
    size_t size = state.range(0);
    float scalar = 2.5f;
    
    auto A = generate_random_data(size);
    std::vector<float> B(size);
    
    // Warmup
    launch_scalar_multiply_shader(A.data(), B.data(), scalar, size);
    
    for (auto _ : state) {
        launch_scalar_multiply_shader(A.data(), B.data(), scalar, size);
    }
    
    state.SetItemsProcessed(state.iterations() * size);
    state.SetBytesProcessed(state.iterations() * 2 * size * sizeof(float));
}
BENCHMARK_REGISTER_F(VulkanBenchmarkFixture, ScalarMultiply)
    ->Arg(65536)
    ->Arg(262144)
    ->Arg(1048576)
    ->Arg(4194304)
    ->Unit(benchmark::kMillisecond);

// ============================================================================
// Transpose Benchmarks
// ============================================================================

BENCHMARK_DEFINE_F(VulkanBenchmarkFixture, Transpose)(benchmark::State& state) {
    int rows = static_cast<int>(state.range(0));
    int cols = static_cast<int>(state.range(1));
    
    auto input = generate_random_data(rows * cols);
    std::vector<float> output(rows * cols);
    
    // Warmup
    launch_transpose_shader(input.data(), output.data(), rows, cols);
    
    for (auto _ : state) {
        launch_transpose_shader(input.data(), output.data(), rows, cols);
    }
    
    state.SetItemsProcessed(state.iterations() * rows * cols);
    state.SetBytesProcessed(state.iterations() * 2 * rows * cols * sizeof(float));
    
    state.counters["Rows"] = rows;
    state.counters["Cols"] = cols;
}
BENCHMARK_REGISTER_F(VulkanBenchmarkFixture, Transpose)
    ->Args({256, 256})
    ->Args({768, 768})
    ->Args({1024, 1024})
    ->Args({2048, 2048})
    ->Unit(benchmark::kMillisecond);

// ============================================================================
// LoRA Gradient Computation Benchmarks
// ============================================================================

BENCHMARK_DEFINE_F(VulkanBenchmarkFixture, LoRA_GradA)(benchmark::State& state) {
    int batch = 32;
    int rank = LORA_RANK;
    int out_dim = static_cast<int>(state.range(0));
    float scaling = 1.0f;
    
    auto h = generate_random_data(batch * rank);
    auto grad_output = generate_random_data(batch * out_dim);
    std::vector<float> grad_A(rank * out_dim);
    
    // Warmup
    launch_lora_grad_A_shader(h.data(), grad_output.data(), grad_A.data(),
                               batch, rank, out_dim, scaling);
    
    for (auto _ : state) {
        launch_lora_grad_A_shader(h.data(), grad_output.data(), grad_A.data(),
                                   batch, rank, out_dim, scaling);
    }
    
    // FLOPs: batch * rank * out_dim * 2 (multiply + accumulate)
    state.SetItemsProcessed(state.iterations() * batch * rank * out_dim * 2);
    
    state.counters["Batch"] = batch;
    state.counters["Rank"] = rank;
    state.counters["OutDim"] = out_dim;
}
BENCHMARK_REGISTER_F(VulkanBenchmarkFixture, LoRA_GradA)
    ->Arg(256)
    ->Arg(768)
    ->Arg(2048)
    ->Unit(benchmark::kMillisecond);

BENCHMARK_DEFINE_F(VulkanBenchmarkFixture, LoRA_GradB)(benchmark::State& state) {
    int batch = 32;
    int in_dim = static_cast<int>(state.range(0));
    int rank = LORA_RANK;
    
    auto input = generate_random_data(batch * in_dim);
    auto grad_h = generate_random_data(batch * rank);
    std::vector<float> grad_B(in_dim * rank);
    
    // Warmup
    launch_lora_grad_B_shader(input.data(), grad_h.data(), grad_B.data(),
                               batch, in_dim, rank);
    
    for (auto _ : state) {
        launch_lora_grad_B_shader(input.data(), grad_h.data(), grad_B.data(),
                                   batch, in_dim, rank);
    }
    
    // FLOPs: batch * in_dim * rank * 2
    state.SetItemsProcessed(state.iterations() * batch * in_dim * rank * 2);
    
    state.counters["Batch"] = batch;
    state.counters["InDim"] = in_dim;
    state.counters["Rank"] = rank;
}
BENCHMARK_REGISTER_F(VulkanBenchmarkFixture, LoRA_GradB)
    ->Arg(256)
    ->Arg(768)
    ->Arg(2048)
    ->Unit(benchmark::kMillisecond);

BENCHMARK_DEFINE_F(VulkanBenchmarkFixture, LoRA_FusedForward)(benchmark::State& state) {
    const int batch = 32;
    const int in_dim = MEDIUM_DIM;
    const int rank = LORA_RANK;
    const int out_dim = MEDIUM_DIM;
    const float scaling = 1.0f;

    auto input = generate_random_data(batch * in_dim);
    auto B = generate_random_data(in_dim * rank);
    auto A = generate_random_data(rank * out_dim);
    std::vector<float> output(batch * out_dim);

    launch_fused_lora_forward(
        input.data(),
        B.data(),
        A.data(),
        output.data(),
        static_cast<size_t>(batch),
        static_cast<size_t>(in_dim),
        static_cast<size_t>(rank),
        static_cast<size_t>(out_dim),
        scaling);

    for (auto _ : state) {
        launch_fused_lora_forward(
            input.data(),
            B.data(),
            A.data(),
            output.data(),
            static_cast<size_t>(batch),
            static_cast<size_t>(in_dim),
            static_cast<size_t>(rank),
            static_cast<size_t>(out_dim),
            scaling);
    }

    const int64_t flops =
        2LL * batch * in_dim * rank +
        2LL * batch * rank * out_dim;
    state.SetItemsProcessed(state.iterations() * flops);

    state.counters["Batch"] = batch;
    state.counters["Dim"] = in_dim;
    state.counters["Rank"] = rank;
    state.counters["Target_E2E_ms"] = 8.0;
    state.counters["Target_HW_ms"] = 4.0;
}
BENCHMARK_REGISTER_F(VulkanBenchmarkFixture, LoRA_FusedForward)->Unit(benchmark::kMillisecond);

// ============================================================================
// End-to-End LoRA Training Step Benchmark
// ============================================================================

BENCHMARK_DEFINE_F(VulkanBenchmarkFixture, LoRA_TrainingStep)(benchmark::State& state) {
    // Simulates a complete LoRA training step: forward + backward pass
    const int batch = 32;
    const int in_dim = MEDIUM_DIM;
    const int out_dim = MEDIUM_DIM;
    const int rank = LORA_RANK;
    const float scaling = 1.0f;
    
    // Allocate all tensors
    auto input = generate_random_data(batch * in_dim);
    auto B = generate_random_data(in_dim * rank);
    auto A = generate_random_data(rank * out_dim);
    auto grad_output = generate_random_data(batch * out_dim);
    
    std::vector<float> h(batch * rank);
    std::vector<float> output(batch * out_dim);
    std::vector<float> grad_A(rank * out_dim);
    std::vector<float> grad_B(in_dim * rank);
    
    // Warmup
    launch_matmul_shader(input.data(), B.data(), h.data(), batch, rank, in_dim, 1.0f);
    launch_matmul_shader(h.data(), A.data(), output.data(), batch, out_dim, rank, scaling);
    launch_lora_grad_A_shader(h.data(), grad_output.data(), grad_A.data(),
                               batch, rank, out_dim, scaling);
    launch_lora_grad_B_shader(input.data(), h.data(), grad_B.data(),
                               batch, in_dim, rank);
    
    for (auto _ : state) {
        // Forward pass
        launch_matmul_shader(input.data(), B.data(), h.data(), batch, rank, in_dim, 1.0f);
        launch_matmul_shader(h.data(), A.data(), output.data(), batch, out_dim, rank, scaling);
        
        // Backward pass
        launch_lora_grad_A_shader(h.data(), grad_output.data(), grad_A.data(),
                                   batch, rank, out_dim, scaling);
        launch_lora_grad_B_shader(input.data(), h.data(), grad_B.data(),
                                   batch, in_dim, rank);
    }
    
    // Total FLOPs for forward and backward
    int64_t forward_flops = 2 * batch * rank * in_dim + 2 * batch * out_dim * rank;
    int64_t backward_flops = 2 * batch * rank * out_dim + 2 * batch * in_dim * rank;
    state.SetItemsProcessed(state.iterations() * (forward_flops + backward_flops));
    
    state.counters["Batch"] = batch;
    state.counters["Dim"] = in_dim;
    state.counters["Rank"] = rank;
    state.counters["Target_ms"] = ExpectedPerformance::TRAINING_STEP_TARGET_E2E_MS;
    state.counters["Target_E2E_ms"] = ExpectedPerformance::TRAINING_STEP_TARGET_E2E_MS;
    state.counters["Target_HW_ms"] = ExpectedPerformance::TRAINING_STEP_TARGET_HW_MS;
    state.counters["CPU_Baseline_ms"] = ExpectedPerformance::TRAINING_STEP_CPU_BASELINE_MS;
    state.counters["Target_Speedup"] = ExpectedPerformance::TARGET_SPEEDUP_E2E;
    state.counters["Target_E2E_Speedup"] = ExpectedPerformance::TARGET_SPEEDUP_E2E;
    state.counters["Target_HW_Speedup"] = ExpectedPerformance::TARGET_SPEEDUP_HW;
}
BENCHMARK_REGISTER_F(VulkanBenchmarkFixture, LoRA_TrainingStep)->Unit(benchmark::kMillisecond);

BENCHMARK_DEFINE_F(VulkanBenchmarkFixture, LoRA_TrainingStep_Fused)(benchmark::State& state) {
    // Hardware-near variant using fused forward/backward entry points.
    const size_t batch = 32;
    const size_t in_dim = MEDIUM_DIM;
    const size_t out_dim = MEDIUM_DIM;
    const size_t rank = LORA_RANK;
    const float scaling = 1.0f;

    auto input = generate_random_data(batch * in_dim);
    auto B = generate_random_data(in_dim * rank);
    auto A = generate_random_data(rank * out_dim);
    auto grad_output = generate_random_data(batch * out_dim);

    std::vector<float> output(batch * out_dim);
    std::vector<float> grad_A(rank * out_dim);
    std::vector<float> grad_B(in_dim * rank);
    std::vector<float> grad_input(batch * in_dim);

    launch_fused_lora_forward(
        input.data(),
        B.data(),
        A.data(),
        output.data(),
        batch,
        in_dim,
        rank,
        out_dim,
        scaling);

    launch_fused_lora_backward(
        input.data(),
        B.data(),
        A.data(),
        grad_output.data(),
        grad_A.data(),
        grad_B.data(),
        grad_input.data(),
        batch,
        in_dim,
        rank,
        out_dim,
        scaling);

    for (auto _ : state) {
        launch_fused_lora_forward(
            input.data(),
            B.data(),
            A.data(),
            output.data(),
            batch,
            in_dim,
            rank,
            out_dim,
            scaling);

        launch_fused_lora_backward(
            input.data(),
            B.data(),
            A.data(),
            grad_output.data(),
            grad_A.data(),
            grad_B.data(),
            grad_input.data(),
            batch,
            in_dim,
            rank,
            out_dim,
            scaling);
    }

    const int64_t forward_flops =
        2LL * static_cast<int64_t>(batch) * static_cast<int64_t>(rank) * static_cast<int64_t>(in_dim) +
        2LL * static_cast<int64_t>(batch) * static_cast<int64_t>(out_dim) * static_cast<int64_t>(rank);
    const int64_t backward_flops =
        2LL * static_cast<int64_t>(batch) * static_cast<int64_t>(rank) * static_cast<int64_t>(out_dim) +
        2LL * static_cast<int64_t>(batch) * static_cast<int64_t>(in_dim) * static_cast<int64_t>(rank);
    state.SetItemsProcessed(state.iterations() * (forward_flops + backward_flops));

    state.counters["Batch"] = static_cast<double>(batch);
    state.counters["Dim"] = static_cast<double>(in_dim);
    state.counters["Rank"] = static_cast<double>(rank);
    state.counters["Target_E2E_ms"] = 8.0;
    state.counters["Target_HW_ms"] = 5.0;
}
BENCHMARK_REGISTER_F(VulkanBenchmarkFixture, LoRA_TrainingStep_Fused)->Unit(benchmark::kMillisecond);

// ============================================================================
// Memory Bandwidth Benchmarks
// ============================================================================

BENCHMARK_DEFINE_F(VulkanBenchmarkFixture, BufferUploadDownload)(benchmark::State& state) {
    size_t size = state.range(0) * sizeof(float);
    
    VulkanContext context;
    if (!context.initialize(0, false)) {
        state.SkipWithError("Failed to initialize Vulkan context");
        return;
    }
    
    auto data = generate_random_data(size / sizeof(float));
    std::vector<float> result(size / sizeof(float));
    
    for (auto _ : state) {
        VulkanBuffer buffer(&context, size, VulkanBuffer::Usage::DeviceLocal);
        buffer.upload(data.data(), size);
        buffer.download(result.data(), size);
    }
    
    state.SetBytesProcessed(state.iterations() * 2 * size);  // Upload + Download
    state.counters["Size_MB"] = (size / 1024.0 / 1024.0);
}
BENCHMARK_REGISTER_F(VulkanBenchmarkFixture, BufferUploadDownload)
    ->Arg(1024)       // 1K floats = 4KB
    ->Arg(262144)     // 256K floats = 1MB
    ->Arg(1048576)    // 1M floats = 4MB
    ->Arg(4194304)    // 4M floats = 16MB
    ->Unit(benchmark::kMillisecond);

// ============================================================================
// Main
// ============================================================================

BENCHMARK_MAIN();

#endif  // THEMIS_ENABLE_GPU
