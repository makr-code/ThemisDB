#include "index/gpu_vector_index.h"
#include <benchmark/benchmark.h>
#include <random>
#include <string>
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

static const char* backendLabel(GPUVectorIndex::Backend backend) {
    switch (backend) {
        case GPUVectorIndex::Backend::AUTO:
            return "AUTO";
        case GPUVectorIndex::Backend::CPU:
            return "CPU";
        case GPUVectorIndex::Backend::VULKAN:
            return "VULKAN";
        case GPUVectorIndex::Backend::CUDA:
            return "CUDA";
        case GPUVectorIndex::Backend::HIP:
            return "HIP";
        default:
            return "UNKNOWN";
    }
}

static bool initializeExpectedBackend(
    benchmark::State& state,
    GPUVectorIndex& index,
    int dimension,
    GPUVectorIndex::Backend expected_backend,
    const std::string& scenario_label) {

    if (!index.initialize(dimension)) {
        state.SkipWithError((scenario_label + ": initialize() failed").c_str());
        return false;
    }

    const auto active = index.getActiveBackend();
    if (active != expected_backend) {
        const std::string msg = scenario_label + ": expected backend " +
                                backendLabel(expected_backend) +
                                ", got " + backendLabel(active);
        state.SkipWithError(msg.c_str());
        return false;
    }

    state.counters["gpu_active"] =
        (active == GPUVectorIndex::Backend::CPU) ? 0.0 : 1.0;
    state.counters["backend_vulkan"] =
        (active == GPUVectorIndex::Backend::VULKAN) ? 1.0 : 0.0;
    state.SetLabel(scenario_label + " backend=" + backendLabel(active));
    return true;
}

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
    
    GPUVectorIndex::Config config;
    config.backend = GPUVectorIndex::Backend::CPU;
    config.metric = GPUVectorIndex::DistanceMetric::L2;

    for (auto _ : state) {
        // Measure only the batch insert/build work, not backend bring-up.
        state.PauseTiming();
        GPUVectorIndex index(config);
        if (!index.initialize(dimension)) {
            state.SkipWithError("index_build_cpu: initialize() failed");
            return;
        }
        state.ResumeTiming();

        benchmark::DoNotOptimize(index.addVectorBatch(ids, vectors));

        state.PauseTiming();
        index.shutdown();
        state.ResumeTiming();
    }
    
    state.SetItemsProcessed(state.iterations() * numVectors);
    state.SetLabel(std::to_string(dimension) + "D, " + std::to_string(numVectors) + " vectors");
}

#ifdef THEMIS_ENABLE_VULKAN
static void BM_IndexBuild_VULKAN(benchmark::State& state) {
    int dimension = state.range(0);
    size_t numVectors = state.range(1);
    
    auto vectors = generateRandomVectors(numVectors, dimension);
    std::vector<std::string> ids;
    for (size_t i = 0; i < numVectors; ++i) {
        ids.push_back("vec_" + std::to_string(i));
    }
    
    GPUVectorIndex::Config config;
    config.backend = GPUVectorIndex::Backend::VULKAN;
    config.metric = GPUVectorIndex::DistanceMetric::L2;
    config.allowCPUFallback = false;

    for (auto _ : state) {
        // Measure only the batch insert/build work, not backend bring-up.
        state.PauseTiming();
        GPUVectorIndex index(config);
        if (!initializeExpectedBackend(
                state,
                index,
                dimension,
                GPUVectorIndex::Backend::VULKAN,
                "index_build_vulkan")) {
            return;
        }
        state.ResumeTiming();

        benchmark::DoNotOptimize(index.addVectorBatch(ids, vectors));

        state.PauseTiming();
        index.shutdown();
        state.ResumeTiming();
    }
    
    state.SetItemsProcessed(state.iterations() * numVectors);
    state.SetLabel(std::to_string(dimension) + "D, " + std::to_string(numVectors) + " vectors");
}
#endif

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
    
    size_t queryIndex = 0;
    
    // Benchmark
    for (auto _ : state) {
        const auto& query = vectors[queryIndex % vectors.size()];
        ++queryIndex;
        auto results = index.search(query, k);
        benchmark::DoNotOptimize(results);
    }
    
    index.shutdown();
    
    state.SetItemsProcessed(state.iterations());
    state.SetLabel(std::to_string(dimension) + "D, " + 
                  std::to_string(numVectors) + " vectors, k=" + std::to_string(k));
}

#ifdef THEMIS_ENABLE_VULKAN
static void BM_Search_VULKAN(benchmark::State& state) {
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
    config.backend = GPUVectorIndex::Backend::VULKAN;
    config.metric = GPUVectorIndex::DistanceMetric::L2;
    config.allowCPUFallback = false;
    
    GPUVectorIndex index(config);
    if (!initializeExpectedBackend(
            state,
            index,
            dimension,
            GPUVectorIndex::Backend::VULKAN,
            "search_vulkan")) {
        return;
    }
    index.addVectorBatch(ids, vectors);
    
    size_t queryIndex = 0;
    
    // Benchmark
    for (auto _ : state) {
        const auto& query = vectors[queryIndex % vectors.size()];
        ++queryIndex;
        auto results = index.search(query, k);
        benchmark::DoNotOptimize(results);
    }
    
    index.shutdown();
    
    state.SetItemsProcessed(state.iterations());
    state.SetLabel(std::to_string(dimension) + "D, " +
                  std::to_string(numVectors) + " vectors, k=" + std::to_string(k));
}
#endif

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

#ifdef THEMIS_ENABLE_VULKAN
static void BM_BatchSearch_VULKAN(benchmark::State& state) {
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
    config.backend = GPUVectorIndex::Backend::VULKAN;
    config.metric = GPUVectorIndex::DistanceMetric::L2;
    config.allowCPUFallback = false;
    
    GPUVectorIndex index(config);
    if (!initializeExpectedBackend(
            state,
            index,
            dimension,
            GPUVectorIndex::Backend::VULKAN,
            "batch_search_vulkan")) {
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

#ifdef THEMIS_ENABLE_VULKAN
// Explicit ANN Vulkan evidence benchmark:
// - Enforces Vulkan backend selection (no CPU fallback)
// - Triggers SPIR-V compute dispatch via warmup + measured searches
// - Reports device-resident hint via VRAM usage after upload
static void BM_ANN_ExplicitVulkanPath(benchmark::State& state) {
    const int dimension = static_cast<int>(state.range(0));
    const size_t numVectors = static_cast<size_t>(state.range(1));
    const int k = static_cast<int>(state.range(2));

    auto vectors = generateRandomVectors(numVectors, dimension, 4242);
    std::vector<std::string> ids;
    ids.reserve(numVectors);
    for (size_t i = 0; i < numVectors; ++i) {
        ids.push_back("ann_vec_" + std::to_string(i));
    }

    GPUVectorIndex::Config config;
    config.backend = GPUVectorIndex::Backend::VULKAN;
    config.metric = GPUVectorIndex::DistanceMetric::L2;
    config.allowCPUFallback = false;

    GPUVectorIndex index(config);
    if (!initializeExpectedBackend(
            state,
            index,
            dimension,
            GPUVectorIndex::Backend::VULKAN,
            "ann_explicit_vulkan")) {
        return;
    }

    if (!index.addVectorBatch(ids, vectors)) {
        state.SkipWithError("ann_explicit_vulkan: addVectorBatch() failed");
        index.shutdown();
        return;
    }

    // Warmup to force initial SPIR-V dispatch and setup costs out of measured loop.
    {
        const auto warmup = index.search(vectors.front(), k);
        benchmark::DoNotOptimize(warmup);
    }

    const auto statsAfterUpload = index.getStatistics();
    const bool deviceResidentLikely = statsAfterUpload.vramUsageBytes > 0;

    size_t queryIndex = 0;
    for (auto _ : state) {
        const auto& query = vectors[queryIndex % vectors.size()];
        ++queryIndex;

        const auto results = index.search(query, static_cast<size_t>(k));
        benchmark::DoNotOptimize(results);
    }

    const auto finalStats = index.getStatistics();
    state.counters["ann_device_selection_ok"] =
        (finalStats.activeBackend == GPUVectorIndex::Backend::VULKAN) ? 1.0 : 0.0;
    state.counters["ann_spirv_dispatch_calls"] =
        static_cast<double>(state.iterations() + 1);  // +1 warmup dispatch
    state.counters["ann_device_resident_hint"] = deviceResidentLikely ? 1.0 : 0.0;
    state.counters["ann_vram_bytes"] = static_cast<double>(statsAfterUpload.vramUsageBytes);

    state.SetItemsProcessed(state.iterations());
    state.SetLabel(
        std::to_string(dimension) + "D, " +
        std::to_string(numVectors) + " vectors, k=" + std::to_string(k) +
        ", explicit-vulkan-path");

    index.shutdown();
}
#endif

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

#ifdef THEMIS_ENABLE_VULKAN
BENCHMARK(BM_IndexBuild_VULKAN)
    ->Args({128, 1000})
    ->Args({128, 10000})
    ->Args({384, 1000})
    ->Args({768, 1000})
    ->Unit(benchmark::kMillisecond);
#endif

// Single query search
BENCHMARK(BM_Search_CPU)
    ->Args({128, 1000, 10})
    ->Args({128, 10000, 10})
    ->Args({384, 1000, 10})
    ->Args({768, 1000, 10})
    ->Unit(benchmark::kMicrosecond);

#ifdef THEMIS_ENABLE_VULKAN
BENCHMARK(BM_Search_VULKAN)
    ->Args({128, 1000, 10})
    ->Args({128, 10000, 10})
    ->Args({384, 1000, 10})
    ->Args({768, 1000, 10})
    ->Unit(benchmark::kMicrosecond);
#endif

// Batch search
BENCHMARK(BM_BatchSearch_CPU)
    ->Args({128, 10000, 10})
    ->Args({128, 10000, 100})
    ->Args({128, 10000, 500})
    ->Args({384, 10000, 100})
    ->Unit(benchmark::kMillisecond);

#ifdef THEMIS_ENABLE_VULKAN
BENCHMARK(BM_BatchSearch_VULKAN)
    ->Args({128, 10000, 10})
    ->Args({128, 10000, 100})
    ->Args({128, 10000, 500})
    ->Args({384, 10000, 100})
    ->Unit(benchmark::kMillisecond);
#endif

#ifdef THEMIS_ENABLE_VULKAN
BENCHMARK(BM_ANN_ExplicitVulkanPath)
    ->Args({128, 10000, 10})
    ->Args({384, 10000, 10})
    ->Unit(benchmark::kMicrosecond);
#endif

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
