/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            bench_gpu_vector_index.cpp                         ║
  Version:         0.0.42                                             ║
  Last Modified:   2026-04-14 18:35:00                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     436                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 202546ee10  2026-04-13  perf: add Disabled-Stub-Policy comments to all 21 *_Disab... ║
    • 9c9ead9b4f  2026-04-09  Implement feature X to enhance user experience and optimi... ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include "index/gpu_vector_index.h"
#include <benchmark/benchmark.h>
#include <random>
#include <vector>

#ifndef THEMIS_ENABLE_GPU

static void BM_GPUVectorIndex_GPUDisabled(benchmark::State& state) {
    for (auto _ : state) {
        state.SkipWithError("GPU vector index benchmarks are disabled in this build");
        break;
    }
}
// Disabled: GPU vector index requires CUDA/Vulkan runner | Deadline: v1.9.0 | Issue: #5
BENCHMARK(BM_GPUVectorIndex_GPUDisabled);

BENCHMARK_MAIN();

#else

using namespace themis::index;

// NOTE: GPU benchmarks (CUDA, Vulkan, HIP) are disabled in v1.5.x.
// GPU backends were removed - see docs/FUTURE_GPU_SUPPORT.md for roadmap.
// Only CPU benchmarks are available in this version.

// Helper function to generate random vectors
std::vector<std::vector<float>> generateRandomVectors(size_t count, int dimension, int seed = 42) {
    std::mt19937 gen(seed);
    std::uniform_real_distribution<float> dist(-1.0f, 1.0f);
    
    std::vector<std::vector<float>> vectors;
    vectors.reserve(count);
    
    for (size_t i = 0; i < count; ++i) {
        std::vector<float> vec(dimension);
        for (int j = 0; j < dimension; ++j) {
            vec[j] = dist(gen);
        }
        vectors.push_back(vec);
    }
    
    return vectors;
}

// =============================================================================
// Index Building Benchmarks
// =============================================================================

static void BM_IndexBuild_CPU(benchmark::State& state) {
    int dimension = state.range(0);
    size_t numVectors = state.range(1);
    
    auto vectors = generateRandomVectors(numVectors, dimension);
    std::vector<std::string> ids;
    for (size_t i = 0; i < numVectors; ++i) {
        ids.push_back("vec_" + std::to_string(i));
    }
    
    for (auto _ : state) {
        GPUVectorIndex::Config config;
        config.backend = GPUVectorIndex::Backend::CPU;
        config.metric = GPUVectorIndex::DistanceMetric::L2;
        
        GPUVectorIndex index(config);
        index.initialize(dimension);
        
        benchmark::DoNotOptimize(index.addVectorBatch(ids, vectors));
        
        index.shutdown();
    }
    
    state.SetItemsProcessed(state.iterations() * numVectors);
    state.SetLabel(std::to_string(dimension) + "D, " + std::to_string(numVectors) + " vectors");
}

// GPU benchmarks disabled in v1.5.x - backends removed
// Will be re-enabled in v2.x when GPU support is added
#if 0
#ifdef THEMIS_ENABLE_CUDA
static void BM_IndexBuild_CUDA(benchmark::State& state) {
    int dimension = state.range(0);
    size_t numVectors = state.range(1);
    
    auto vectors = generateRandomVectors(numVectors, dimension);
    std::vector<std::string> ids;
    for (size_t i = 0; i < numVectors; ++i) {
        ids.push_back("vec_" + std::to_string(i));
    }
    
    for (auto _ : state) {
        GPUVectorIndex::Config config;
        config.backend = GPUVectorIndex::Backend::CUDA;
        config.metric = GPUVectorIndex::DistanceMetric::L2;
        
        GPUVectorIndex index(config);
        if (!index.initialize(dimension)) {
            state.SkipWithError("CUDA not available");
            return;
        }
        
        benchmark::DoNotOptimize(index.addVectorBatch(ids, vectors));
        
        index.shutdown();
    }
    
    state.SetItemsProcessed(state.iterations() * numVectors);
    state.SetLabel(std::to_string(dimension) + "D, " + std::to_string(numVectors) + " vectors");
}
#endif
#endif  // GPU benchmarks disabled

// =============================================================================
// Search Benchmarks
// =============================================================================

static void BM_Search_CPU(benchmark::State& state) {
    int dimension = state.range(0);
    size_t numVectors = state.range(1);
    int k = state.range(2);
    
    // Setup
    auto vectors = generateRandomVectors(numVectors, dimension);
    std::vector<std::string> ids;
    for (size_t i = 0; i < numVectors; ++i) {
        ids.push_back("vec_" + std::to_string(i));
    }
    
    GPUVectorIndex::Config config;
    config.backend = GPUVectorIndex::Backend::CPU;
    config.metric = GPUVectorIndex::DistanceMetric::L2;
    
    GPUVectorIndex index(config);
    index.initialize(dimension);
    index.addVectorBatch(ids, vectors);
    
    auto query = vectors[0];
    
    // Benchmark
    for (auto _ : state) {
        auto results = index.search(query, k);
        benchmark::DoNotOptimize(results);
    }
    
    index.shutdown();
    
    state.SetItemsProcessed(state.iterations());
    state.SetLabel(std::to_string(dimension) + "D, " + 
                  std::to_string(numVectors) + " vectors, k=" + std::to_string(k));
}

#if 0  // GPU benchmarks disabled in v1.5.x
#ifdef THEMIS_ENABLE_CUDA
static void BM_Search_CUDA(benchmark::State& state) {
    int dimension = state.range(0);
    size_t numVectors = state.range(1);
    int k = state.range(2);
    
    // Setup
    auto vectors = generateRandomVectors(numVectors, dimension);
    std::vector<std::string> ids;
    for (size_t i = 0; i < numVectors; ++i) {
        ids.push_back("vec_" + std::to_string(i));
    }
    
    GPUVectorIndex::Config config;
    config.backend = GPUVectorIndex::Backend::CUDA;
    config.metric = GPUVectorIndex::DistanceMetric::L2;
    
    GPUVectorIndex index(config);
    if (!index.initialize(dimension)) {
        state.SkipWithError("CUDA not available");
        return;
    }
    index.addVectorBatch(ids, vectors);
    
    auto query = vectors[0];
    
    // Benchmark
    for (auto _ : state) {
        auto results = index.search(query, k);
        benchmark::DoNotOptimize(results);
    }
    
    index.shutdown();
    
    state.SetItemsProcessed(state.iterations());
    state.SetLabel(std::to_string(dimension) + "D, " + 
                  std::to_string(numVectors) + " vectors, k=" + std::to_string(k));
}
#endif
#endif  // GPU benchmarks disabled

// =============================================================================
// Batch Search Benchmarks
// =============================================================================

static void BM_BatchSearch_CPU(benchmark::State& state) {
    int dimension = state.range(0);
    size_t numVectors = state.range(1);
    size_t batchSize = state.range(2);
    int k = 10;
    
    // Setup
    auto vectors = generateRandomVectors(numVectors, dimension);
    std::vector<std::string> ids;
    for (size_t i = 0; i < numVectors; ++i) {
        ids.push_back("vec_" + std::to_string(i));
    }
    
    GPUVectorIndex::Config config;
    config.backend = GPUVectorIndex::Backend::CPU;
    config.metric = GPUVectorIndex::DistanceMetric::L2;
    
    GPUVectorIndex index(config);
    index.initialize(dimension);
    index.addVectorBatch(ids, vectors);
    
    auto queries = generateRandomVectors(batchSize, dimension, 100);
    
    // Benchmark
    for (auto _ : state) {
        auto results = index.searchBatch(queries, k);
        benchmark::DoNotOptimize(results);
    }
    
    index.shutdown();
    
    state.SetItemsProcessed(state.iterations() * batchSize);
    state.SetLabel(std::to_string(dimension) + "D, " + 
                  std::to_string(numVectors) + " vectors, batch=" + std::to_string(batchSize));
}

#if 0  // GPU benchmarks disabled in v1.5.x
#ifdef THEMIS_ENABLE_CUDA
static void BM_BatchSearch_CUDA(benchmark::State& state) {
    int dimension = state.range(0);
    size_t numVectors = state.range(1);
    size_t batchSize = state.range(2);
    int k = 10;
    
    // Setup
    auto vectors = generateRandomVectors(numVectors, dimension);
    std::vector<std::string> ids;
    for (size_t i = 0; i < numVectors; ++i) {
        ids.push_back("vec_" + std::to_string(i));
    }
    
    GPUVectorIndex::Config config;
    config.backend = GPUVectorIndex::Backend::CUDA;
    config.metric = GPUVectorIndex::DistanceMetric::L2;
    
    GPUVectorIndex index(config);
    if (!index.initialize(dimension)) {
        state.SkipWithError("CUDA not available");
        return;
    }
    index.addVectorBatch(ids, vectors);
    
    auto queries = generateRandomVectors(batchSize, dimension, 100);
    
    // Benchmark
    for (auto _ : state) {
        auto results = index.searchBatch(queries, k);
        benchmark::DoNotOptimize(results);
    }
    
    index.shutdown();
    
    state.SetItemsProcessed(state.iterations() * batchSize);
    state.SetLabel(std::to_string(dimension) + "D, " + 
                  std::to_string(numVectors) + " vectors, batch=" + std::to_string(batchSize));
}
#endif
#endif  // GPU benchmarks disabled

// =============================================================================
// Distance Metric Benchmarks
// =============================================================================

static void BM_DistanceMetric_L2(benchmark::State& state) {
    int dimension = state.range(0);
    size_t numVectors = state.range(1);
    
    auto vectors = generateRandomVectors(numVectors, dimension);
    std::vector<std::string> ids;
    for (size_t i = 0; i < numVectors; ++i) {
        ids.push_back("vec_" + std::to_string(i));
    }
    
    GPUVectorIndex::Config config;
    config.backend = GPUVectorIndex::Backend::CPU;
    config.metric = GPUVectorIndex::DistanceMetric::L2;
    
    GPUVectorIndex index(config);
    index.initialize(dimension);
    index.addVectorBatch(ids, vectors);
    
    auto query = vectors[0];
    
    for (auto _ : state) {
        auto results = index.search(query, 10);
        benchmark::DoNotOptimize(results);
    }
    
    index.shutdown();
}

static void BM_DistanceMetric_Cosine(benchmark::State& state) {
    int dimension = state.range(0);
    size_t numVectors = state.range(1);
    
    auto vectors = generateRandomVectors(numVectors, dimension);
    std::vector<std::string> ids;
    for (size_t i = 0; i < numVectors; ++i) {
        ids.push_back("vec_" + std::to_string(i));
    }
    
    GPUVectorIndex::Config config;
    config.backend = GPUVectorIndex::Backend::CPU;
    config.metric = GPUVectorIndex::DistanceMetric::COSINE;
    
    GPUVectorIndex index(config);
    index.initialize(dimension);
    index.addVectorBatch(ids, vectors);
    
    auto query = vectors[0];
    
    for (auto _ : state) {
        auto results = index.search(query, 10);
        benchmark::DoNotOptimize(results);
    }
    
    index.shutdown();
}

// =============================================================================
// Register Benchmarks
// =============================================================================

// Index building
BENCHMARK(BM_IndexBuild_CPU)
    ->Args({128, 1000})
    ->Args({128, 10000})
    ->Args({384, 1000})
    ->Args({768, 1000})
    ->Unit(benchmark::kMillisecond);

#if 0  // GPU benchmarks disabled in v1.5.x
#ifdef THEMIS_ENABLE_CUDA
BENCHMARK(BM_IndexBuild_CUDA)
    ->Args({128, 1000})
    ->Args({128, 10000})
    ->Args({384, 1000})
    ->Args({768, 1000})
    ->Unit(benchmark::kMillisecond);
#endif
#endif  // GPU benchmarks disabled

// Single query search
BENCHMARK(BM_Search_CPU)
    ->Args({128, 1000, 10})
    ->Args({128, 10000, 10})
    ->Args({384, 1000, 10})
    ->Args({768, 1000, 10})
    ->Unit(benchmark::kMicrosecond);

#if 0  // GPU benchmarks disabled in v1.5.x
#ifdef THEMIS_ENABLE_CUDA
BENCHMARK(BM_Search_CUDA)
    ->Args({128, 1000, 10})
    ->Args({128, 10000, 10})
    ->Args({384, 1000, 10})
    ->Args({768, 1000, 10})
    ->Unit(benchmark::kMicrosecond);
#endif
#endif  // GPU benchmarks disabled

// Batch search
BENCHMARK(BM_BatchSearch_CPU)
    ->Args({128, 10000, 10})
    ->Args({128, 10000, 100})
    ->Args({128, 10000, 500})
    ->Args({384, 10000, 100})
    ->Unit(benchmark::kMillisecond);

#if 0  // GPU benchmarks disabled in v1.5.x
#ifdef THEMIS_ENABLE_CUDA
BENCHMARK(BM_BatchSearch_CUDA)
    ->Args({128, 10000, 10})
    ->Args({128, 10000, 100})
    ->Args({128, 10000, 500})
    ->Args({384, 10000, 100})
    ->Unit(benchmark::kMillisecond);
#endif
#endif  // GPU benchmarks disabled

// Distance metrics
BENCHMARK(BM_DistanceMetric_L2)
    ->Args({128, 10000})
    ->Args({384, 10000})
    ->Unit(benchmark::kMicrosecond);

BENCHMARK(BM_DistanceMetric_Cosine)
    ->Args({128, 10000})
    ->Args({384, 10000})
    ->Unit(benchmark::kMicrosecond);

BENCHMARK_MAIN();

#endif  // THEMIS_ENABLE_GPU
