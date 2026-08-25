/**
 * @file bench_approximate_radius_search.cpp
 * @brief Performance benchmarks for ApproximateRadiusSearch
 * 
 * Validates production readiness by measuring:
 * - Search latency across different dataset sizes
 * - Throughput for batch operations
 * - Recall vs performance tradeoffs
 * - Different radius values impact
 * - Different metrics (L2, Cosine, Dot-product)
 */

#include <benchmark/benchmark.h>
#include <memory>
#include <random>
#include <filesystem>
#include <string>
#include <vector>
#include <algorithm>
#include <cmath>

#include "storage/rocksdb_wrapper.h"
#include "storage/base_entity.h"
#include "index/vector_index.h"
#include "index/approximate_radius_search.h"

using themis::RocksDBWrapper;
using themis::BaseEntity;
using themis::VectorIndexManager;
using themis::vector::ApproximateRadiusSearch;

namespace {

struct VecUtil {
    static std::vector<float> randomVec(int dim, std::mt19937& rng) {
        std::uniform_real_distribution<float> dist(0.0f, 1.0f);
        std::vector<float> v(dim);
        for (int i = 0; i < dim; ++i) v[i] = dist(rng);
        // L2-Normalize for stable COSINE values
        float s = 0.0f; 
        for (float x : v) s += x * x; 
        s = std::sqrt(std::max(s, 1e-12f));
        for (float& x : v) x /= s;
        return v;
    }
    
    static std::string padInt(size_t v, int width = 8) {
        char buf[32]; 
        std::snprintf(buf, sizeof(buf), "%0*zu", width, v); 
        return std::string(buf);
    }
};

struct RadiusSearchEnv {
    std::shared_ptr<RocksDBWrapper> db;
    std::shared_ptr<VectorIndexManager> vim;
    std::shared_ptr<ApproximateRadiusSearch> radius_search;
    int dim = 128;
    size_t N = 10000;  // Default dataset size
    bool ready = false;
    std::vector<std::vector<float>> dataset;
    VectorIndexManager::Metric metric = VectorIndexManager::Metric::COSINE;

    static RadiusSearchEnv& instance() { 
        static RadiusSearchEnv env; 
        return env; 
    }

    void initOnce(size_t dataset_size, VectorIndexManager::Metric m = VectorIndexManager::Metric::COSINE) {
        if (ready && N == dataset_size && metric == m) return;
        
        // Clean state if reinitializing
        if (ready) {
            radius_search.reset();
            vim.reset();
            db.reset();
            dataset.clear();
            ready = false;
        }
        
        N = dataset_size;
        metric = m;
        
        const std::string db_path = "data/themis_bench_radius_search";
        std::error_code ec; 
        std::filesystem::remove_all(db_path, ec);

        RocksDBWrapper::Config cfg; 
        cfg.db_path = db_path; 
        cfg.memtable_size_mb = 256; 
        cfg.block_cache_size_mb = 512;
        cfg.compression_default = "lz4"; 
        cfg.compression_bottommost = "zstd";
        
        db = std::make_shared<RocksDBWrapper>(cfg);
        if (!db->open()) {
            throw std::runtime_error("Failed to open RocksDB for radius search benchmark");
        }

        vim = std::make_shared<VectorIndexManager>(*db);
        auto st = vim->init("vectors", dim, metric, /*M*/16, /*efC*/200, /*ef*/64);
        if (!st.ok) {
            throw std::runtime_error("VectorIndex init failed: " + st.message);
        }

        // Generate and insert dataset
        dataset.reserve(N);
        std::mt19937 rng(42);
        for (size_t i = 0; i < N; ++i) {
            auto vec = VecUtil::randomVec(dim, rng);
            dataset.push_back(vec);
            BaseEntity e("v_" + VecUtil::padInt(i));
            e.setField("embedding", themis::Value{vec});
            auto rst = vim->addEntity(e);
            if (!rst.ok) {
                throw std::runtime_error("addEntity failed at i=" + std::to_string(i));
            }
        }

        // Create radius search instance
        radius_search = std::make_shared<ApproximateRadiusSearch>(*vim);

        ready = true;
    }
};

} // namespace

// ---------------------------------------------------------------------------
// Benchmark 1: Basic radius search with varying dataset sizes
// ---------------------------------------------------------------------------
static void BM_RadiusSearch_DatasetSize(benchmark::State& state) {
    const size_t dataset_size = static_cast<size_t>(state.range(0));
    const float radius = 0.5f;
    
    auto& env = RadiusSearchEnv::instance();
    env.initOnce(dataset_size);

    std::mt19937 rng(123);
    std::uniform_int_distribution<size_t> pick(0, env.N - 1);

    ApproximateRadiusSearch::SearchConfig config;
    config.radius = radius;
    config.metric = ApproximateRadiusSearch::Metric::COSINE;
    config.max_results = 1000;
    config.sort_results = true;

    size_t total_results = 0;
    for (auto _ : state) {
        const auto& query = env.dataset[pick(rng)];
        auto result = env.radius_search->search(query, config);
        if (!result.has_value()) {
            state.SkipWithError("Search failed");
            break;
        }
        total_results += result.value().results.size();
    }
    
    state.counters["avg_results"] = benchmark::Counter(
        static_cast<double>(total_results) / static_cast<double>(state.iterations()),
        benchmark::Counter::kAvgIterations
    );
    state.counters["queries_per_sec"] = benchmark::Counter(
        static_cast<double>(state.iterations()),
        benchmark::Counter::kIsRate
    );
}

// Dataset sizes: 1K, 10K, 100K, 1M
BENCHMARK(BM_RadiusSearch_DatasetSize)
    ->Args({1000})
    ->Args({10000})
    ->Args({100000})
    ->Unit(benchmark::kMillisecond);

// ---------------------------------------------------------------------------
// Benchmark 2: Different radius values (smaller radius = fewer results = faster)
// ---------------------------------------------------------------------------
static void BM_RadiusSearch_RadiusVariation(benchmark::State& state) {
    const float radius = static_cast<float>(state.range(0)) / 100.0f;  // 0.1, 0.3, 0.5, 0.7, 1.0
    
    auto& env = RadiusSearchEnv::instance();
    env.initOnce(10000);

    std::mt19937 rng(123);
    std::uniform_int_distribution<size_t> pick(0, env.N - 1);

    ApproximateRadiusSearch::SearchConfig config;
    config.radius = radius;
    config.metric = ApproximateRadiusSearch::Metric::COSINE;
    config.max_results = 10000;
    config.sort_results = true;

    size_t total_results = 0;
    for (auto _ : state) {
        const auto& query = env.dataset[pick(rng)];
        auto result = env.radius_search->search(query, config);
        if (!result.has_value()) {
            state.SkipWithError("Search failed");
            break;
        }
        total_results += result.value().results.size();
    }
    
    state.counters["avg_results"] = benchmark::Counter(
        static_cast<double>(total_results) / static_cast<double>(state.iterations()),
        benchmark::Counter::kAvgIterations
    );
    state.counters["radius"] = radius;
}

BENCHMARK(BM_RadiusSearch_RadiusVariation)
    ->Arg(10)   // 0.1
    ->Arg(30)   // 0.3
    ->Arg(50)   // 0.5
    ->Arg(70)   // 0.7
    ->Arg(100)  // 1.0
    ->Unit(benchmark::kMillisecond);

// ---------------------------------------------------------------------------
// Benchmark 3: Batch search throughput
// ---------------------------------------------------------------------------
static void BM_RadiusSearch_BatchSearch(benchmark::State& state) {
    const size_t batch_size = static_cast<size_t>(state.range(0));
    const float radius = 0.5f;
    
    auto& env = RadiusSearchEnv::instance();
    env.initOnce(10000);

    std::mt19937 rng(123);
    std::uniform_int_distribution<size_t> pick(0, env.N - 1);

    ApproximateRadiusSearch::SearchConfig config;
    config.radius = radius;
    config.metric = ApproximateRadiusSearch::Metric::COSINE;
    config.max_results = 1000;
    config.sort_results = true;

    // Prepare batch queries
    std::vector<std::vector<float>> batch_queries;
    batch_queries.reserve(batch_size);
    for (size_t i = 0; i < batch_size; ++i) {
        batch_queries.push_back(env.dataset[pick(rng)]);
    }

    for (auto _ : state) {
        auto result = env.radius_search->batchSearch(batch_queries, config);
        if (!result.has_value()) {
            state.SkipWithError("Batch search failed");
            break;
        }
        benchmark::DoNotOptimize(result.value());
    }
    
    state.counters["batch_size"] = static_cast<double>(batch_size);
    state.counters["queries_per_sec"] = benchmark::Counter(
        static_cast<double>(state.iterations() * batch_size),
        benchmark::Counter::kIsRate
    );
}

BENCHMARK(BM_RadiusSearch_BatchSearch)
    ->Arg(1)
    ->Arg(10)
    ->Arg(50)
    ->Arg(100)
    ->Unit(benchmark::kMillisecond);

// ---------------------------------------------------------------------------
// Benchmark 4: searchWithTargetCount (adaptive radius)
// ---------------------------------------------------------------------------
static void BM_RadiusSearch_TargetCount(benchmark::State& state) {
    const int target_count = static_cast<int>(state.range(0));
    
    auto& env = RadiusSearchEnv::instance();
    env.initOnce(10000);

    std::mt19937 rng(123);
    std::uniform_int_distribution<size_t> pick(0, env.N - 1);

    ApproximateRadiusSearch::SearchConfig config;
    config.radius = 1.0f;  // Starting radius
    config.metric = ApproximateRadiusSearch::Metric::COSINE;
    config.sort_results = true;

    size_t total_results = 0;
    for (auto _ : state) {
        const auto& query = env.dataset[pick(rng)];
        auto result = env.radius_search->searchWithTargetCount(query, target_count, config);
        if (!result.has_value()) {
            state.SkipWithError("Target count search failed");
            break;
        }
        total_results += result.value().results.size();
    }
    
    state.counters["target_count"] = static_cast<double>(target_count);
    state.counters["avg_results"] = benchmark::Counter(
        static_cast<double>(total_results) / static_cast<double>(state.iterations()),
        benchmark::Counter::kAvgIterations
    );
}

BENCHMARK(BM_RadiusSearch_TargetCount)
    ->Arg(5)
    ->Arg(10)
    ->Arg(20)
    ->Arg(50)
    ->Arg(100)
    ->Unit(benchmark::kMillisecond);

// ---------------------------------------------------------------------------
// Benchmark 5: estimateResultCount performance
// ---------------------------------------------------------------------------
static void BM_RadiusSearch_EstimateCount(benchmark::State& state) {
    const float radius = static_cast<float>(state.range(0)) / 100.0f;
    
    auto& env = RadiusSearchEnv::instance();
    env.initOnce(10000);

    std::mt19937 rng(123);
    std::uniform_int_distribution<size_t> pick(0, env.N - 1);

    for (auto _ : state) {
        const auto& query = env.dataset[pick(rng)];
        auto result = env.radius_search->estimateResultCount(
            query, 
            radius, 
            ApproximateRadiusSearch::Metric::COSINE
        );
        if (!result.has_value()) {
            state.SkipWithError("Estimation failed");
            break;
        }
        benchmark::DoNotOptimize(result.value());
    }
}

BENCHMARK(BM_RadiusSearch_EstimateCount)
    ->Arg(30)   // 0.3
    ->Arg(50)   // 0.5
    ->Arg(70)   // 0.7
    ->Unit(benchmark::kMillisecond);

// ---------------------------------------------------------------------------
// Benchmark 6: searchById performance
// ---------------------------------------------------------------------------
static void BM_RadiusSearch_SearchById(benchmark::State& state) {
    const float radius = 0.5f;
    
    auto& env = RadiusSearchEnv::instance();
    env.initOnce(10000);

    std::mt19937 rng(123);
    std::uniform_int_distribution<size_t> pick(0, env.N - 1);

    ApproximateRadiusSearch::SearchConfig config;
    config.radius = radius;
    config.metric = ApproximateRadiusSearch::Metric::COSINE;
    config.max_results = 1000;
    config.sort_results = true;

    for (auto _ : state) {
        size_t idx = pick(rng);
        std::string id = "v_" + VecUtil::padInt(idx);
        auto result = env.radius_search->searchById(id, config);
        if (!result.has_value()) {
            state.SkipWithError("searchById failed");
            break;
        }
        benchmark::DoNotOptimize(result.value());
    }
}

BENCHMARK(BM_RadiusSearch_SearchById)
    ->Unit(benchmark::kMillisecond);

// ---------------------------------------------------------------------------
// Benchmark 7: Different metrics comparison (L2, COSINE, DOT)
// ---------------------------------------------------------------------------
static void BM_RadiusSearch_Metrics(benchmark::State& state) {
    const int metric_type = static_cast<int>(state.range(0));
    
    VectorIndexManager::Metric vim_metric;
    ApproximateRadiusSearch::Metric ars_metric;
    
    switch (metric_type) {
        case 0: // L2
            vim_metric = VectorIndexManager::Metric::L2;
            ars_metric = ApproximateRadiusSearch::Metric::L2;
            break;
        case 1: // COSINE
            vim_metric = VectorIndexManager::Metric::COSINE;
            ars_metric = ApproximateRadiusSearch::Metric::COSINE;
            break;
        case 2: // DOT
            vim_metric = VectorIndexManager::Metric::DOT;
            ars_metric = ApproximateRadiusSearch::Metric::DOT_PRODUCT;
            break;
        default:
            state.SkipWithError("Invalid metric");
            return;
    }
    
    auto& env = RadiusSearchEnv::instance();
    env.initOnce(10000, vim_metric);

    std::mt19937 rng(123);
    std::uniform_int_distribution<size_t> pick(0, env.N - 1);

    ApproximateRadiusSearch::SearchConfig config;
    config.radius = 0.5f;
    config.metric = ars_metric;
    config.max_results = 1000;
    config.sort_results = true;

    for (auto _ : state) {
        const auto& query = env.dataset[pick(rng)];
        auto result = env.radius_search->search(query, config);
        if (!result.has_value()) {
            state.SkipWithError("Search failed");
            break;
        }
        benchmark::DoNotOptimize(result.value());
    }
}

BENCHMARK(BM_RadiusSearch_Metrics)
    ->Arg(0)  // L2
    ->Arg(1)  // COSINE
    ->Arg(2)  // DOT
    ->Unit(benchmark::kMillisecond);

BENCHMARK_MAIN();
