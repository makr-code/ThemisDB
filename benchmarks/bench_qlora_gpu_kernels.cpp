/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            bench_qlora_gpu_kernels.cpp                        ║
  Version:         0.0.38                                             ║
  Last Modified:   2026-04-13 04:12:04                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     374                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
    • a629043ab2  2026-02-22  Audit: document gaps found - benchmarks and stale annotat... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/**
 * @file bench_qlora_gpu.cpp
 * @brief Performance benchmarks for QLoRA GPU kernel optimizations
 */

#include <benchmark/benchmark.h>

#ifdef THEMIS_ENABLE_CUDA

#include "llm/lora_framework/quantization_kernels.h"
#include "llm/lora_framework/quantization.h"
#include <cuda_runtime.h>
#include <vector>
#include <random>

using namespace themis::llm::lora;
using namespace themis::llm::lora::cuda;

namespace {

/**
 * @brief Generate random test data for benchmarks
 * @note Uses fixed seed (42) for consistent performance measurements
 *       across benchmark runs. Different data distributions can be tested
 *       by varying mean/stddev parameters.
 */
std::vector<float> generateRandomData(size_t size) {
    std::vector<float> data(size);
    std::mt19937 gen(42);  // Fixed seed for reproducible benchmarks
    std::normal_distribution<float> dist(0.0f, 1.0f);
    for (size_t i = 0; i < size; i++) {
        data[i] = dist(gen);
    }
    return data;
}

} // namespace

// ============================================================================
// NF4 Quantization Benchmarks
// ============================================================================

static void BM_NF4_Quantization_GPU(benchmark::State& state) {
    const size_t num_elements = state.range(0);
    const size_t block_size = 64;
    const size_t num_blocks = (num_elements + block_size - 1) / block_size;
    
    auto input = generateRandomData(num_elements);
    
    // Allocate GPU memory
    float* d_input;
    uint8_t* d_output;
    float* d_scales;
    float* d_zeros;
    
    cudaMalloc(&d_input, num_elements * sizeof(float));
    cudaMalloc(&d_output, (num_elements + 1) / 2);
    cudaMalloc(&d_scales, num_blocks * sizeof(float));
    cudaMalloc(&d_zeros, num_blocks * sizeof(float));
    
    cudaMemcpy(d_input, input.data(), num_elements * sizeof(float), cudaMemcpyHostToDevice);
    
    // Warmup
    launch_quantize_nf4_kernel(d_input, d_output, d_scales, d_zeros, num_elements, block_size);
    cudaDeviceSynchronize();
    
    // Benchmark
    for (auto _ : state) {
        launch_quantize_nf4_kernel(d_input, d_output, d_scales, d_zeros, num_elements, block_size);
        cudaDeviceSynchronize();
    }
    
    state.SetItemsProcessed(state.iterations() * num_elements);
    state.SetBytesProcessed(state.iterations() * num_elements * sizeof(float));
    
    cudaFree(d_input);
    cudaFree(d_output);
    cudaFree(d_scales);
    cudaFree(d_zeros);
}

BENCHMARK(BM_NF4_Quantization_GPU)
    ->Arg(1024)           // 1K params
    ->Arg(10240)          // 10K params
    ->Arg(102400)         // 100K params
    ->Arg(1024 * 1024)    // 1M params
    ->Arg(10 * 1024 * 1024)  // 10M params
    ->Unit(benchmark::kMillisecond);

// ============================================================================
// INT8 Quantization Benchmarks
// ============================================================================

static void BM_INT8_Quantization_GPU(benchmark::State& state) {
    const size_t num_elements = state.range(0);
    const size_t block_size = 64;
    const size_t num_blocks = (num_elements + block_size - 1) / block_size;
    
    auto input = generateRandomData(num_elements);
    
    float* d_input;
    int8_t* d_output;
    float* d_scales;
    
    cudaMalloc(&d_input, num_elements * sizeof(float));
    cudaMalloc(&d_output, num_elements * sizeof(int8_t));
    cudaMalloc(&d_scales, num_blocks * sizeof(float));
    
    cudaMemcpy(d_input, input.data(), num_elements * sizeof(float), cudaMemcpyHostToDevice);
    
    // Warmup
    launch_quantize_int8_kernel(d_input, d_output, d_scales, num_elements, block_size);
    cudaDeviceSynchronize();
    
    for (auto _ : state) {
        launch_quantize_int8_kernel(d_input, d_output, d_scales, num_elements, block_size);
        cudaDeviceSynchronize();
    }
    
    state.SetItemsProcessed(state.iterations() * num_elements);
    state.SetBytesProcessed(state.iterations() * num_elements * sizeof(float));
    
    cudaFree(d_input);
    cudaFree(d_output);
    cudaFree(d_scales);
}

BENCHMARK(BM_INT8_Quantization_GPU)
    ->Arg(1024)
    ->Arg(10240)
    ->Arg(102400)
    ->Arg(1024 * 1024)
    ->Arg(10 * 1024 * 1024)
    ->Unit(benchmark::kMillisecond);

// ============================================================================
// Dequantization Benchmarks
// ============================================================================

static void BM_NF4_Dequantization_GPU(benchmark::State& state) {
    const size_t num_elements = state.range(0);
    const size_t block_size = 64;
    const size_t num_blocks = (num_elements + block_size - 1) / block_size;
    
    auto input = generateRandomData(num_elements);
    
    float* d_input;
    uint8_t* d_quantized;
    float* d_scales;
    float* d_zeros;
    float* d_output;
    
    cudaMalloc(&d_input, num_elements * sizeof(float));
    cudaMalloc(&d_quantized, (num_elements + 1) / 2);
    cudaMalloc(&d_scales, num_blocks * sizeof(float));
    cudaMalloc(&d_zeros, num_blocks * sizeof(float));
    cudaMalloc(&d_output, num_elements * sizeof(float));
    
    cudaMemcpy(d_input, input.data(), num_elements * sizeof(float), cudaMemcpyHostToDevice);
    
    // Quantize once
    launch_quantize_nf4_kernel(d_input, d_quantized, d_scales, d_zeros, num_elements, block_size);
    cudaDeviceSynchronize();
    
    // Benchmark dequantization
    for (auto _ : state) {
        launch_dequantize_nf4_kernel(d_quantized, d_scales, d_zeros, d_output, num_elements, block_size);
        cudaDeviceSynchronize();
    }
    
    state.SetItemsProcessed(state.iterations() * num_elements);
    state.SetBytesProcessed(state.iterations() * num_elements * sizeof(float));
    
    cudaFree(d_input);
    cudaFree(d_quantized);
    cudaFree(d_scales);
    cudaFree(d_zeros);
    cudaFree(d_output);
}

BENCHMARK(BM_NF4_Dequantization_GPU)
    ->Arg(1024)
    ->Arg(10240)
    ->Arg(102400)
    ->Arg(1024 * 1024)
    ->Arg(10 * 1024 * 1024)
    ->Unit(benchmark::kMillisecond);

// ============================================================================
// Fused Kernel Benchmarks
// ============================================================================

static void BM_Fused_Dequant_MatMul_NF4(benchmark::State& state) {
    const size_t M = 32;
    const size_t K = state.range(0);
    const size_t N = state.range(0);
    const size_t block_size = 64;
    const size_t num_blocks = (K * N + block_size - 1) / block_size;
    
    auto input = generateRandomData(M * K);
    auto weights = generateRandomData(K * N);
    
    // Allocate and quantize weights
    float* d_weights_fp32;
    uint8_t* d_weights_quant;
    float* d_scales;
    float* d_zeros;
    float* d_input;
    float* d_output;
    
    cudaMalloc(&d_weights_fp32, K * N * sizeof(float));
    cudaMalloc(&d_weights_quant, (K * N + 1) / 2);
    cudaMalloc(&d_scales, num_blocks * sizeof(float));
    cudaMalloc(&d_zeros, num_blocks * sizeof(float));
    cudaMalloc(&d_input, M * K * sizeof(float));
    cudaMalloc(&d_output, M * N * sizeof(float));
    
    cudaMemcpy(d_weights_fp32, weights.data(), K * N * sizeof(float), cudaMemcpyHostToDevice);
    cudaMemcpy(d_input, input.data(), M * K * sizeof(float), cudaMemcpyHostToDevice);
    
    launch_quantize_nf4_kernel(d_weights_fp32, d_weights_quant, d_scales, d_zeros, K * N, block_size);
    cudaDeviceSynchronize();
    
    // Benchmark fused kernel
    for (auto _ : state) {
        launch_fused_dequant_matmul_kernel(
            d_weights_quant, d_scales, d_zeros, d_input, d_output, M, K, N, block_size, true);
        cudaDeviceSynchronize();
    }
    
    // FLOPS: 2 * M * K * N (multiply-add)
    state.SetItemsProcessed(state.iterations() * 2 * M * K * N);
    
    cudaFree(d_weights_fp32);
    cudaFree(d_weights_quant);
    cudaFree(d_scales);
    cudaFree(d_zeros);
    cudaFree(d_input);
    cudaFree(d_output);
}

BENCHMARK(BM_Fused_Dequant_MatMul_NF4)
    ->Arg(256)
    ->Arg(512)
    ->Arg(768)
    ->Arg(1024)
    ->Unit(benchmark::kMillisecond);

// ============================================================================
// Mixed Precision Benchmarks
// ============================================================================

static void BM_FP16_MatMul(benchmark::State& state) {
    const size_t M = 32;
    const size_t K = state.range(0);
    const size_t N = state.range(0);
    
    auto input_a = generateRandomData(M * K);
    auto input_b = generateRandomData(K * N);
    
    float* d_A;
    float* d_B;
    float* d_C;
    
    cudaMalloc(&d_A, M * K * sizeof(float));
    cudaMalloc(&d_B, K * N * sizeof(float));
    cudaMalloc(&d_C, M * N * sizeof(float));
    
    cudaMemcpy(d_A, input_a.data(), M * K * sizeof(float), cudaMemcpyHostToDevice);
    cudaMemcpy(d_B, input_b.data(), K * N * sizeof(float), cudaMemcpyHostToDevice);
    
    // Warmup
    launch_fp16_matmul_kernel(d_A, d_B, d_C, M, K, N);
    cudaDeviceSynchronize();
    
    for (auto _ : state) {
        launch_fp16_matmul_kernel(d_A, d_B, d_C, M, K, N);
        cudaDeviceSynchronize();
    }
    
    state.SetItemsProcessed(state.iterations() * 2 * M * K * N);
    
    cudaFree(d_A);
    cudaFree(d_B);
    cudaFree(d_C);
}

BENCHMARK(BM_FP16_MatMul)
    ->Arg(256)
    ->Arg(512)
    ->Arg(768)
    ->Arg(1024)
    ->Unit(benchmark::kMillisecond);

// ============================================================================
// Memory Transfer Benchmarks
// ============================================================================

static void BM_Memory_Transfer_HtoD(benchmark::State& state) {
    const size_t size = state.range(0) * sizeof(float);
    
    std::vector<float> host_data(state.range(0), 1.0f);
    float* d_data;
    cudaMalloc(&d_data, size);
    
    for (auto _ : state) {
        cudaMemcpy(d_data, host_data.data(), size, cudaMemcpyHostToDevice);
        cudaDeviceSynchronize();
    }
    
    state.SetBytesProcessed(state.iterations() * size);
    
    cudaFree(d_data);
}

BENCHMARK(BM_Memory_Transfer_HtoD)
    ->Arg(1024)
    ->Arg(102400)
    ->Arg(1024 * 1024)
    ->Unit(benchmark::kMillisecond);

static void BM_Memory_Transfer_HtoD_Async(benchmark::State& state) {
    const size_t size = state.range(0) * sizeof(float);
    
    GPUMemoryManager manager;
    std::vector<float> host_data(state.range(0), 1.0f);
    void* d_data = manager.allocateQuantizedBuffer(state.range(0) * sizeof(float), false);
    
    cudaStream_t stream;
    cudaStreamCreate(&stream);
    
    for (auto _ : state) {
        manager.transferToGPUAsync(d_data, host_data.data(), size, stream);
        cudaStreamSynchronize(stream);
    }
    
    state.SetBytesProcessed(state.iterations() * size);
    
    cudaStreamDestroy(stream);
    manager.freeDevice(d_data);
}

BENCHMARK(BM_Memory_Transfer_HtoD_Async)
    ->Arg(1024)
    ->Arg(102400)
    ->Arg(1024 * 1024)
    ->Unit(benchmark::kMillisecond);

#endif // THEMIS_ENABLE_CUDA

BENCHMARK_MAIN();
