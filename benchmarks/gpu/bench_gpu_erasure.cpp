/**
 * ThemisDB GPU Erasure Coding Performance Benchmark
 * 
 * Benchmarks comparing CPU vs GPU performance for erasure coding operations
 */

#include <benchmark/benchmark.h>
#include "sharding/gpu_erasure_coder.h"
#include "sharding/redundancy_strategy.h"
#include <vector>
#include <random>

#ifndef THEMIS_ENABLE_GPU

static void BM_GPUErasure_GPUDisabled(benchmark::State& state) {
    for (auto _ : state) {
        state.SkipWithError("GPU erasure benchmarks are disabled in this build");
        break;
    }
}
// Disabled: GPU erasure coding requires CUDA/HIP runner | Deadline: v1.9.0 | Issue: #5
BENCHMARK(BM_GPUErasure_GPUDisabled);

BENCHMARK_MAIN();

#else

using namespace themis::sharding;

// ═══════════════════════════════════════════════════════════
// Helper Functions
// ═══════════════════════════════════════════════════════════

static std::vector<uint8_t> generateRandomData(size_t size) {
    std::vector<uint8_t> data(size);
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 255);
    
    for (auto& byte : data) {
        byte = static_cast<uint8_t>(dis(gen));
    }
    return data;
}

// ═══════════════════════════════════════════════════════════
// CPU Benchmarks
// ═══════════════════════════════════════════════════════════

static void BM_CPU_Encode_1MB(benchmark::State& state) {
    auto coder = std::make_unique<GPUErasureCoder>(
        AccelerationType::CPU_ONLY,
        GPUConfig{},
        ErasureCodingAlgorithm::REED_SOLOMON
    );
    
    auto data = generateRandomData(1 * 1024 * 1024);
    uint32_t data_shards = 10;
    uint32_t parity_shards = 4;
    
    for (auto _ : state) {
        auto chunks = coder->encode(data, data_shards, parity_shards);
        benchmark::DoNotOptimize(chunks);
    }
    
    state.SetBytesProcessed(state.iterations() * data.size());
}
BENCHMARK(BM_CPU_Encode_1MB);

static void BM_CPU_Encode_10MB(benchmark::State& state) {
    auto coder = std::make_unique<GPUErasureCoder>(
        AccelerationType::CPU_ONLY,
        GPUConfig{},
        ErasureCodingAlgorithm::REED_SOLOMON
    );
    
    auto data = generateRandomData(10 * 1024 * 1024);
    uint32_t data_shards = 10;
    uint32_t parity_shards = 4;
    
    for (auto _ : state) {
        auto chunks = coder->encode(data, data_shards, parity_shards);
        benchmark::DoNotOptimize(chunks);
    }
    
    state.SetBytesProcessed(state.iterations() * data.size());
}
BENCHMARK(BM_CPU_Encode_10MB);

static void BM_CPU_Encode_100MB(benchmark::State& state) {
    auto coder = std::make_unique<GPUErasureCoder>(
        AccelerationType::CPU_ONLY,
        GPUConfig{},
        ErasureCodingAlgorithm::REED_SOLOMON
    );
    
    auto data = generateRandomData(100 * 1024 * 1024);
    uint32_t data_shards = 10;
    uint32_t parity_shards = 4;
    
    for (auto _ : state) {
        auto chunks = coder->encode(data, data_shards, parity_shards);
        benchmark::DoNotOptimize(chunks);
    }
    
    state.SetBytesProcessed(state.iterations() * data.size());
}
BENCHMARK(BM_CPU_Encode_100MB);

// ═══════════════════════════════════════════════════════════
// GPU Benchmarks
// ═══════════════════════════════════════════════════════════

static void BM_GPU_Encode_1MB(benchmark::State& state) {
    auto coder = std::make_unique<GPUErasureCoder>(
        AccelerationType::AUTO,
        GPUConfig{},
        ErasureCodingAlgorithm::REED_SOLOMON
    );
    
    auto data = generateRandomData(1 * 1024 * 1024);
    uint32_t data_shards = 10;
    uint32_t parity_shards = 4;
    
    // Reset stats before benchmark
    coder->resetStats();
    
    for (auto _ : state) {
        auto chunks = coder->encode(data, data_shards, parity_shards);
        benchmark::DoNotOptimize(chunks);
    }
    
    state.SetBytesProcessed(state.iterations() * data.size());
    
    // Report GPU usage (only once at the end)
    if (state.thread_index() == 0) {
        auto stats = coder->getStats();
        state.counters["gpu_encodes"] = static_cast<double>(stats.gpu_encodes);
        state.counters["cpu_fallbacks"] = static_cast<double>(stats.cpu_fallbacks);
    }
}
BENCHMARK(BM_GPU_Encode_1MB);

static void BM_GPU_Encode_10MB(benchmark::State& state) {
    auto coder = std::make_unique<GPUErasureCoder>(
        AccelerationType::AUTO,
        GPUConfig{},
        ErasureCodingAlgorithm::REED_SOLOMON
    );
    
    auto data = generateRandomData(10 * 1024 * 1024);
    uint32_t data_shards = 10;
    uint32_t parity_shards = 4;
    
    // Reset stats before benchmark
    coder->resetStats();
    
    for (auto _ : state) {
        auto chunks = coder->encode(data, data_shards, parity_shards);
        benchmark::DoNotOptimize(chunks);
    }
    
    state.SetBytesProcessed(state.iterations() * data.size());
    
    if (state.thread_index() == 0) {
        auto stats = coder->getStats();
        state.counters["gpu_encodes"] = static_cast<double>(stats.gpu_encodes);
        state.counters["cpu_fallbacks"] = static_cast<double>(stats.cpu_fallbacks);
    }
}
BENCHMARK(BM_GPU_Encode_10MB);

static void BM_GPU_Encode_100MB(benchmark::State& state) {
    auto coder = std::make_unique<GPUErasureCoder>(
        AccelerationType::AUTO,
        GPUConfig{},
        ErasureCodingAlgorithm::REED_SOLOMON
    );
    
    auto data = generateRandomData(100 * 1024 * 1024);
    uint32_t data_shards = 10;
    uint32_t parity_shards = 4;
    
    // Reset stats before benchmark
    coder->resetStats();
    
    for (auto _ : state) {
        auto chunks = coder->encode(data, data_shards, parity_shards);
        benchmark::DoNotOptimize(chunks);
    }
    
    state.SetBytesProcessed(state.iterations() * data.size());
    
    if (state.thread_index() == 0) {
        auto stats = coder->getStats();
        state.counters["gpu_encodes"] = static_cast<double>(stats.gpu_encodes);
        state.counters["cpu_fallbacks"] = static_cast<double>(stats.cpu_fallbacks);
    }
}
BENCHMARK(BM_GPU_Encode_100MB);

// ═══════════════════════════════════════════════════════════
// Batch Encoding Benchmarks
// ═══════════════════════════════════════════════════════════

static void BM_GPU_BatchEncode_Small(benchmark::State& state) {
    auto coder = std::make_unique<GPUErasureCoder>(
        AccelerationType::AUTO,
        GPUConfig{},
        ErasureCodingAlgorithm::REED_SOLOMON
    );
    
    // Create batch of small blocks
    std::vector<std::vector<uint8_t>> data_blocks;
    for (int i = 0; i < 64; i++) {
        data_blocks.push_back(generateRandomData(64 * 1024));  // 64KB each
    }
    
    uint32_t data_shards = 10;
    uint32_t parity_shards = 4;
    
    for (auto _ : state) {
        auto results = coder->batchEncode(data_blocks, data_shards, parity_shards);
        benchmark::DoNotOptimize(results);
    }
    
    size_t total_bytes = data_blocks.size() * data_blocks[0].size();
    state.SetBytesProcessed(state.iterations() * total_bytes);
}
BENCHMARK(BM_GPU_BatchEncode_Small);

static void BM_GPU_BatchEncode_Medium(benchmark::State& state) {
    auto coder = std::make_unique<GPUErasureCoder>(
        AccelerationType::AUTO,
        GPUConfig{},
        ErasureCodingAlgorithm::REED_SOLOMON
    );
    
    // Create batch of medium blocks
    std::vector<std::vector<uint8_t>> data_blocks;
    for (int i = 0; i < 32; i++) {
        data_blocks.push_back(generateRandomData(512 * 1024));  // 512KB each
    }
    
    uint32_t data_shards = 10;
    uint32_t parity_shards = 4;
    
    for (auto _ : state) {
        auto results = coder->batchEncode(data_blocks, data_shards, parity_shards);
        benchmark::DoNotOptimize(results);
    }
    
    size_t total_bytes = data_blocks.size() * data_blocks[0].size();
    state.SetBytesProcessed(state.iterations() * total_bytes);
}
BENCHMARK(BM_GPU_BatchEncode_Medium);

// ═══════════════════════════════════════════════════════════
// Varying Redundancy Benchmarks
// ═══════════════════════════════════════════════════════════

static void BM_GPU_Encode_LowRedundancy(benchmark::State& state) {
    auto coder = std::make_unique<GPUErasureCoder>(
        AccelerationType::AUTO,
        GPUConfig{},
        ErasureCodingAlgorithm::REED_SOLOMON
    );
    
    auto data = generateRandomData(10 * 1024 * 1024);
    uint32_t data_shards = 10;
    uint32_t parity_shards = 2;  // Low redundancy
    
    for (auto _ : state) {
        auto chunks = coder->encode(data, data_shards, parity_shards);
        benchmark::DoNotOptimize(chunks);
    }
    
    state.SetBytesProcessed(state.iterations() * data.size());
}
BENCHMARK(BM_GPU_Encode_LowRedundancy);

static void BM_GPU_Encode_HighRedundancy(benchmark::State& state) {
    auto coder = std::make_unique<GPUErasureCoder>(
        AccelerationType::AUTO,
        GPUConfig{},
        ErasureCodingAlgorithm::REED_SOLOMON
    );
    
    auto data = generateRandomData(10 * 1024 * 1024);
    uint32_t data_shards = 10;
    uint32_t parity_shards = 6;  // High redundancy
    
    for (auto _ : state) {
        auto chunks = coder->encode(data, data_shards, parity_shards);
        benchmark::DoNotOptimize(chunks);
    }
    
    state.SetBytesProcessed(state.iterations() * data.size());
}
BENCHMARK(BM_GPU_Encode_HighRedundancy);

// Main function
BENCHMARK_MAIN();

#endif  // THEMIS_ENABLE_GPU
