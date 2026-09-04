#include <benchmark/benchmark.h>

#include "acceleration/cpu_backend.h"
#include "storage/base_entity.h"
#include "storage/rocksdb_wrapper.h"
#include "index/vector_index.h"
#include "themis/gpu/query_accelerator.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <numeric>
#include <mutex>
#include <random>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

using namespace themis;
using namespace themis::acceleration;
using namespace themis::gpu;

namespace {

std::vector<float> makeFloatBuffer(std::size_t count, std::uint32_t seed = 42) {
    std::mt19937 rng(seed);
    std::uniform_real_distribution<float> dist(-1.0f, 1.0f);
    std::vector<float> values(count);
    for (auto& value : values) {
        value = dist(rng);
    }
    return values;
}

std::vector<float> makeNormalizedFloatBuffer(std::size_t rows, std::size_t dim, std::uint32_t seed = 42) {
    auto values = makeFloatBuffer(rows * dim, seed);
    for (std::size_t row = 0; row < rows; ++row) {
        float* base = values.data() + row * dim;
        float norm_sq = 0.0f;
        for (std::size_t d = 0; d < dim; ++d) {
            norm_sq += base[d] * base[d];
        }
        const float norm = std::sqrt(std::max(norm_sq, 1e-12f));
        for (std::size_t d = 0; d < dim; ++d) {
            base[d] /= norm;
        }
    }
    return values;
}

float scalarL2DistanceSq(const float* a, const float* b, std::size_t dim) {
    float distance = 0.0f;
    for (std::size_t i = 0; i < dim; ++i) {
        const float diff = a[i] - b[i];
        distance += diff * diff;
    }
    return distance;
}

float scalarCosineDistance(const float* a, const float* b, std::size_t dim) {
    float dot = 0.0f;
    float norm_a = 0.0f;
    float norm_b = 0.0f;
    for (std::size_t i = 0; i < dim; ++i) {
        dot += a[i] * b[i];
        norm_a += a[i] * a[i];
        norm_b += b[i] * b[i];
    }
    const float denom = std::sqrt(std::max(norm_a * norm_b, 1e-12f));
    return 1.0f - (dot / denom);
}

std::vector<std::pair<std::uint32_t, float>> scalarTopKSearch(
    const float* query,
    const float* database,
    std::size_t  num_vectors,
    std::size_t  dim,
    std::size_t  top_k,
    bool         use_l2) {
    std::vector<std::pair<std::uint32_t, float>> distances;
    distances.reserve(num_vectors);

    for (std::size_t i = 0; i < num_vectors; ++i) {
        const float* candidate = database + i * dim;
        const float distance = use_l2
            ? scalarL2DistanceSq(query, candidate, dim)
            : scalarCosineDistance(query, candidate, dim);
        distances.emplace_back(static_cast<std::uint32_t>(i), distance);
    }

    const std::size_t actual_k = std::min(top_k, distances.size());
    std::partial_sort(
        distances.begin(),
        distances.begin() + actual_k,
        distances.end(),
        [](const auto& lhs, const auto& rhs) {
            if (lhs.second != rhs.second) {
                return lhs.second < rhs.second;
            }
            return lhs.first < rhs.first;
        });
    distances.resize(actual_k);
    return distances;
}

CPUVectorBackend& cpuBackend() {
    static CPUVectorBackend backend;
    static const bool initialized = backend.initialize();
    (void)initialized;
    return backend;
}

struct HnswSearchEnv {
    std::string db_path;
    std::unique_ptr<RocksDBWrapper> db;
    std::unique_ptr<VectorIndexManager> index;
    std::vector<std::vector<float>> dataset;
    int dim = 0;
    std::size_t count = 0;
};

HnswSearchEnv& cosineHnswEnv() {
    static HnswSearchEnv env;
    static std::once_flag init_once;
    std::call_once(init_once, [&]() {
        env.dim = 128;
        env.count = 4000;
        env.db_path = (std::filesystem::temp_directory_path() / "themis_bench_ann_cpu_gpu_dispatch_hnsw").string();
        std::error_code ec = {};
        std::filesystem::remove_all(env.db_path, ec);

        RocksDBWrapper::Config cfg;
        cfg.db_path = env.db_path;
        cfg.memtable_size_mb = 64;
        cfg.block_cache_size_mb = 128;
        cfg.compression_default = "lz4";
        cfg.compression_bottommost = "zstd";

        env.db = std::make_unique<RocksDBWrapper>(cfg);
        if (!env.db->open()) {
            throw std::runtime_error("Failed to open RocksDB for ANN HNSW benchmark");
        }

        env.index = std::make_unique<VectorIndexManager>(*env.db);
        auto status = env.index->init("chunks", env.dim, VectorIndexManager::Metric::COSINE, 16, 200, 64);
        if (!status.ok) {
            throw std::runtime_error("VectorIndexManager::init failed: " + status.message);
        }

        auto raw = makeNormalizedFloatBuffer(env.count, static_cast<std::size_t>(env.dim), 777);
        env.dataset.reserve(env.count);
        for (std::size_t i = 0; i < env.count; ++i) {
            std::vector<float> row(raw.begin() + static_cast<std::ptrdiff_t>(i * env.dim),
                                   raw.begin() + static_cast<std::ptrdiff_t>((i + 1) * env.dim));
            env.dataset.push_back(row);
            BaseEntity entity("ann_" + std::to_string(i));
            entity.setField("embedding", Value{row});
            auto insert_status = env.index->addEntity(entity);
            if (!insert_status.ok) {
                throw std::runtime_error("VectorIndexManager::addEntity failed: " + insert_status.message);
            }
        }

    });

    return env;
}

} // namespace

static void BM_ANN_ScalarL2TopK(benchmark::State& state) {
    const std::size_t num_vectors = static_cast<std::size_t>(state.range(0));
    const std::size_t dim = static_cast<std::size_t>(state.range(1));
    const std::size_t top_k = static_cast<std::size_t>(state.range(2));

    const auto query = makeFloatBuffer(dim, 123);
    const auto database = makeFloatBuffer(num_vectors * dim, 456);

    for (auto _ : state) {
        auto result = scalarTopKSearch(query.data(), database.data(), num_vectors, dim, top_k, true);
        benchmark::DoNotOptimize(result);
    }

    state.SetItemsProcessed(state.iterations() * static_cast<int64_t>(num_vectors));
    state.counters["mode"] = 0;
    state.counters["top_k"] = static_cast<double>(top_k);
}

BENCHMARK(BM_ANN_ScalarL2TopK)
    ->Args({1000, 128, 10})
    ->Args({4000, 128, 10})
    ->Args({16000, 128, 10})
    ->Args({16000, 128, 100})
    ->Unit(benchmark::kMicrosecond);

static void BM_ANN_CPUSIMDDispatchBatchKnn(benchmark::State& state) {
    const std::size_t num_vectors = static_cast<std::size_t>(state.range(0));
    const std::size_t dim = static_cast<std::size_t>(state.range(1));
    const std::size_t top_k = static_cast<std::size_t>(state.range(2));
    const std::size_t batch_size = static_cast<std::size_t>(state.range(3));

    const auto queries = makeFloatBuffer(batch_size * dim, 1001);
    const auto database = makeFloatBuffer(num_vectors * dim, 1002);
    auto& backend = cpuBackend();

    for (auto _ : state) {
        auto result = backend.batchKnnSearch(
            queries.data(), batch_size, dim,
            database.data(), num_vectors, top_k, true);
        benchmark::DoNotOptimize(result);
    }

    state.SetItemsProcessed(state.iterations() * static_cast<int64_t>(batch_size * num_vectors));
    state.counters["mode"] = 1;
    state.counters["batch_size"] = static_cast<double>(batch_size);
    state.counters["top_k"] = static_cast<double>(top_k);
}

BENCHMARK(BM_ANN_CPUSIMDDispatchBatchKnn)
    ->Args({1000, 128, 10, 1})
    ->Args({4000, 128, 10, 1})
    ->Args({4000, 128, 10, 32})
    ->Args({16000, 128, 10, 32})
    ->Args({16000, 128, 100, 32})
    ->Unit(benchmark::kMicrosecond);

static void BM_ANN_MixedPathGpuQueryAccelerator(benchmark::State& state) {
    const std::size_t num_vectors = static_cast<std::size_t>(state.range(0));
    const std::size_t dim = static_cast<std::size_t>(state.range(1));
    const std::size_t top_k = static_cast<std::size_t>(state.range(2));
    const std::size_t batch_size = static_cast<std::size_t>(state.range(3));

    GPUQueryAccelerator::Config config;
    config.gpu_threshold_rows = 4096;
    config.force_cpu = false;
    config.enable_graph_cache = true;
    GPUQueryAccelerator accelerator(config);

    const auto queries = makeFloatBuffer(batch_size * dim, 2001);
    const auto database = makeFloatBuffer(num_vectors * dim, 2002);

    bool last_used_gpu = false;
    for (auto _ : state) {
        auto result = accelerator.annSearch(queries, batch_size, dim, database, num_vectors, top_k, true);
        last_used_gpu = result.used_gpu;
        benchmark::DoNotOptimize(result);
    }

    const auto stats = accelerator.getStats();
    state.SetItemsProcessed(state.iterations() * static_cast<int64_t>(batch_size * num_vectors));
    state.counters["mode"] = 2;
    state.counters["batch_size"] = static_cast<double>(batch_size);
    state.counters["top_k"] = static_cast<double>(top_k);
    state.counters["used_gpu"] = last_used_gpu ? 1.0 : 0.0;
    state.counters["gpu_ops"] = static_cast<double>(stats.gpu_ops);
    state.counters["cpu_fallback_ops"] = static_cast<double>(stats.cpu_fallback_ops);
}

BENCHMARK(BM_ANN_MixedPathGpuQueryAccelerator)
    ->Args({1000, 128, 10, 1})
    ->Args({4000, 128, 10, 1})
    ->Args({4000, 128, 10, 32})
    ->Args({16000, 128, 10, 32})
    ->Args({16000, 128, 100, 32})
    ->Unit(benchmark::kMicrosecond);

static void BM_ANN_HNSWFrontdoorCosine(benchmark::State& state) {
    const std::size_t batch_size = static_cast<std::size_t>(state.range(0));
    const std::size_t top_k = static_cast<std::size_t>(state.range(1));

    auto& env = cosineHnswEnv();
    std::size_t query_cursor = 0;

    for (auto _ : state) {
        for (std::size_t i = 0; i < batch_size; ++i) {
            const auto& query = env.dataset[(query_cursor + i) % env.dataset.size()];
            auto [status, result] = env.index->searchKnn(query, top_k);
            if (!status.ok) {
                state.SkipWithError(status.message.c_str());
                return;
            }
            benchmark::DoNotOptimize(result);
        }
        query_cursor = (query_cursor + batch_size) % env.dataset.size();
    }

    state.SetItemsProcessed(state.iterations() * static_cast<int64_t>(batch_size));
    state.counters["mode"] = 3;
    state.counters["batch_size"] = static_cast<double>(batch_size);
    state.counters["top_k"] = static_cast<double>(top_k);
    state.counters["vectors"] = static_cast<double>(env.count);
}

BENCHMARK(BM_ANN_HNSWFrontdoorCosine)
    ->Args({1, 10})
    ->Args({1, 100})
    ->Args({32, 10})
    ->Args({128, 10})
    ->Unit(benchmark::kMicrosecond);

static void BM_Smoke_ANN_Scalar(benchmark::State& state) {
    const auto query = makeFloatBuffer(64, 3001);
    const auto database = makeFloatBuffer(256 * 64, 3002);
    for (auto _ : state) {
        auto result = scalarTopKSearch(query.data(), database.data(), 256, 64, 5, true);
        benchmark::DoNotOptimize(result);
    }
}
BENCHMARK(BM_Smoke_ANN_Scalar)->Unit(benchmark::kMicrosecond);

static void BM_Smoke_ANN_CPUSIMD(benchmark::State& state) {
    auto& backend = cpuBackend();
    const auto queries = makeFloatBuffer(4 * 64, 3003);
    const auto database = makeFloatBuffer(256 * 64, 3004);
    for (auto _ : state) {
        auto result = backend.batchKnnSearch(queries.data(), 4, 64, database.data(), 256, 5, true);
        benchmark::DoNotOptimize(result);
    }
}
BENCHMARK(BM_Smoke_ANN_CPUSIMD)->Unit(benchmark::kMicrosecond);

static void BM_Smoke_ANN_MixedPath(benchmark::State& state) {
    GPUQueryAccelerator::Config config;
    config.gpu_threshold_rows = 128;
    config.enable_graph_cache = true;
    GPUQueryAccelerator accelerator(config);
    const auto queries = makeFloatBuffer(4 * 64, 3005);
    const auto database = makeFloatBuffer(256 * 64, 3006);
    for (auto _ : state) {
        auto result = accelerator.annSearch(queries, 4, 64, database, 256, 5, true);
        benchmark::DoNotOptimize(result);
    }
}
BENCHMARK(BM_Smoke_ANN_MixedPath)->Unit(benchmark::kMicrosecond);

BENCHMARK_MAIN();
