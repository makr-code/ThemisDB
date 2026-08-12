// Benchmark: Tensor Fingerprint Graph Performance
// Phase 8.5 Acceptance Criteria:
//   • Fingerprint + LSH insert ≤ 10 ms per tensor
//   • Similar-tensor graph query ≤ 50 ms for 100K nodes
//   • ≥ 40% storage reduction for LLM weight repositories

#include "graph/tensor_fingerprint_graph.h"
#include "graph/tensor_deduplication_manager.h"
#include "storage/tensor_train_decomposer.h"
#include "storage/tensor_network_storage_engine.h"

#include <benchmark/benchmark.h>
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <memory>
#include <random>
#include <string>
#include <vector>

using namespace themis::graph;
using namespace themis::storage;

// ============================================================================
// Helpers
// ============================================================================

static std::vector<float> makeRandVec(std::size_t n, unsigned seed = 42) {
    std::mt19937 rng(seed);
    std::normal_distribution<float> dist(0.0f, 1.0f);
    std::vector<float> v(n);
    for (auto& x : v) x = dist(rng);
    return v;
}

static std::vector<float> makeSharedVec(std::size_t n, unsigned base_seed,
                                         float noise_scale = 0.001f) {
    auto v = makeRandVec(n, base_seed);
    std::mt19937 rng(base_seed ^ 0xDEADBEEFU);
    std::normal_distribution<float> noise(0.0f, noise_scale);
    for (auto& x : v) x += noise(rng);
    return v;
}

static TTTrain makeBenchTrain(const std::vector<float>& data,
                               const std::vector<std::size_t>& shape,
                               double eps = 0.05) {
    TensorTrainDecomposer dec;
    TensorTrainConfig cfg;
    cfg.eps      = eps;
    cfg.max_rank = 8;
    [[maybe_unused]] auto [t, error] = dec.decompose(data, shape, cfg);
    return std::move(t);
}

static std::shared_ptr<TensorNetworkStorageEngine> makeBenchEngine() {
    auto backend = std::make_shared<InMemoryTensorBackend>();
    TensorStorageConfig cfg;
    cfg.tt_config.eps          = 0.05;
    cfg.min_compression_ratio  = 0.0;
    return std::make_shared<TensorNetworkStorageEngine>(backend, cfg);
}

// ============================================================================
// BM_FingerprintInsert
//
// Measures insert() latency including fingerprint computation + LSH bucketing.
// Acceptance criterion: ≤ 10 ms per tensor at all tested modes sizes.
// ============================================================================

class FingerprintInsertFixture : public benchmark::Fixture {
public:
    void SetUp(const ::benchmark::State& state) override {
        FingerprintGraphConfig cfg;
        cfg.similarity_threshold   = 0.95;
        cfg.num_hash_funcs         = 128;
        cfg.num_bands              = 32;
        cfg.cache_trains_in_memory = true;
        graph_ = std::make_unique<TensorFingerprintGraph>(cfg);

        // Pre-compute tensors to isolate insert latency from decomposition.
        const auto mode_n = static_cast<std::size_t>(state.range(0));
        const std::vector<std::size_t> shape(mode_n, 2U);
        std::size_t total_elems = 1U;
        for (std::size_t i = 0; i < mode_n; ++i) total_elems *= 2U;
        trains_.reserve(kBatchSize);
        for (std::size_t i = 0; i < kBatchSize; ++i) {
            auto data = makeRandVec(total_elems, static_cast<unsigned>(i));
            trains_.push_back(makeBenchTrain(data, shape));
        }
        counter_ = 0;
    }

    void TearDown(const ::benchmark::State&) override {
        graph_.reset();
        trains_.clear();
    }

protected:
    std::unique_ptr<TensorFingerprintGraph> graph_;
    std::vector<TTTrain>                    trains_;
    std::size_t                             counter_ = 0;
    static constexpr std::size_t            kBatchSize = 256;
};

BENCHMARK_DEFINE_F(FingerprintInsertFixture, BM_FingerprintInsert)(
    benchmark::State& state) {
    std::size_t iter = 0;
    for (auto _ : state) {
        const auto idx = iter % trains_.size();
        graph_->insert("tensor_" + std::to_string(iter), trains_[idx]);
        ++iter;
    }
    state.counters["inserts_per_sec"] = benchmark::Counter(
        static_cast<double>(iter),
        benchmark::Counter::kIsRate);
}

// Range: mode counts {1,2,4,8}
BENCHMARK_REGISTER_F(FingerprintInsertFixture, BM_FingerprintInsert)
    ->Arg(1)
    ->Arg(2)
    ->Arg(4)
    ->Arg(8)
    ->Unit(benchmark::kMillisecond)
    ->Iterations(200);

// ============================================================================
// BM_FindSimilar
//
// Measures findSimilar() latency on a pre-populated graph with N nodes.
// Acceptance criterion: ≤ 50 ms for 100K nodes.
// ============================================================================

class FindSimilarFixture : public benchmark::Fixture {
public:
    void SetUp(const ::benchmark::State& state) override {
        const std::size_t node_count = static_cast<std::size_t>(state.range(0));

        FingerprintGraphConfig cfg;
        cfg.similarity_threshold   = 0.95;
        cfg.num_hash_funcs         = 128;
        cfg.num_bands              = 32;
        cfg.max_candidates         = 1000;
        cfg.top_k                  = 50;
        cfg.cache_trains_in_memory = true;
        graph_ = std::make_unique<TensorFingerprintGraph>(cfg);

        // Populate graph with unique random tensors.
        for (std::size_t i = 0; i < node_count; ++i) {
            auto data = makeRandVec(16, static_cast<unsigned>(i + 1000));
            auto t    = makeBenchTrain(data, {16, 1});
            graph_->insert("n_" + std::to_string(i), t);
        }

        // Build a representative query tensor (similar to node 0).
        auto qdata = makeSharedVec(16, 1000, 0.01f);
        query_     = makeBenchTrain(qdata, {16, 1});
    }

    void TearDown(const ::benchmark::State&) override {
        graph_.reset();
    }

protected:
    std::unique_ptr<TensorFingerprintGraph> graph_;
    TTTrain                                  query_;
};

// This benchmark is intentionally fixed to 100K nodes to match the
// Phase-8.5 acceptance criterion.
BENCHMARK_DEFINE_F(FindSimilarFixture, BM_FindSimilar_100K)(benchmark::State& state) {
    std::size_t queries = 0;
    for (auto _ : state) {
        auto results = graph_->findSimilar(query_, 50);
        benchmark::DoNotOptimize(results);
        ++queries;
    }
    state.counters["queries_per_sec"] = benchmark::Counter(
        static_cast<double>(queries),
        benchmark::Counter::kIsRate);
    state.counters["nodes"] = benchmark::Counter(
        static_cast<double>(state.range(0)));
}

BENCHMARK_REGISTER_F(FindSimilarFixture, BM_FindSimilar_100K)
    ->Arg(100000)
    ->Unit(benchmark::kMillisecond);

// ============================================================================
// BM_StorageReductionRatio
//
// Measures deduplication effectiveness on simulated LLM weight sets with
// shared Transformer blocks. Acceptance criterion: ≥ 40% storage reduction.
// ============================================================================

class StorageReductionFixture : public benchmark::Fixture {
public:
    void SetUp(const ::benchmark::State& state) override {
        const std::size_t num_weight_sets = static_cast<std::size_t>(state.range(0));
        const std::size_t shared_blocks   = 8;   // blocks shared across sets
        const std::size_t unique_blocks   = 2;   // unique blocks per set
        const std::size_t block_size      = 64;  // elements per block

        engine_ = makeBenchEngine();

        FingerprintGraphConfig fp_cfg;
        fp_cfg.similarity_threshold   = 0.90;
        fp_cfg.num_hash_funcs         = 128;
        fp_cfg.num_bands              = 32;
        fp_cfg.cache_trains_in_memory = true;
        auto fp  = std::make_shared<TensorFingerprintGraph>(fp_cfg);
        auto dec = std::make_shared<TensorTrainDecomposer>();

        DeduplicationConfig dedup_cfg;
        dedup_cfg.similarity_threshold    = 0.90;
        dedup_cfg.allow_full_storage_fallback = true;
        mgr_ = std::make_unique<TensorDeduplicationManager>(engine_, fp, dec, dedup_cfg);

        // Pre-generate shared Transformer block data (seed 1..shared_blocks).
        shared_blocks_.resize(shared_blocks);
        for (std::size_t b = 0; b < shared_blocks; ++b) {
            shared_blocks_[b] = makeRandVec(block_size, static_cast<unsigned>(b + 1));
        }

        // Store num_weight_sets * (shared + unique) tensors.
        for (std::size_t s = 0; s < num_weight_sets; ++s) {
            for (std::size_t b = 0; b < shared_blocks; ++b) {
                // Add tiny noise so TT-decomposition is non-trivial.
                auto data = makeSharedVec(block_size, static_cast<unsigned>(b + 1), 5e-4f);
                const auto tid = "ws" + std::to_string(s) + "_shared_" + std::to_string(b);
                mgr_->store(tid, data, {block_size, 1}, "bench", "lm", "w" + std::to_string(b));
            }
            for (std::size_t u = 0; u < unique_blocks; ++u) {
                auto data = makeRandVec(
                    block_size,
                    static_cast<unsigned>(s * 100 + unique_blocks + u + 9000));
                const auto tid = "ws" + std::to_string(s) + "_unique_" + std::to_string(u);
                mgr_->store(tid, data, {block_size, 1}, "bench", "lm",
                            "wu" + std::to_string(s * unique_blocks + u));
            }
        }
    }

    void TearDown(const ::benchmark::State&) override {
        mgr_.reset();
        engine_.reset();
        shared_blocks_.clear();
    }

protected:
    std::shared_ptr<TensorNetworkStorageEngine> engine_;
    std::unique_ptr<TensorDeduplicationManager> mgr_;
    std::vector<std::vector<float>>             shared_blocks_;
};

BENCHMARK_DEFINE_F(StorageReductionFixture, BM_StorageReductionRatio)(
    benchmark::State& state) {
    for (auto _ : state) {
        auto stats = mgr_->getStats();
        benchmark::DoNotOptimize(stats);
    }

    const auto stats     = mgr_->getStats();
    const double ratio   = stats.dedup_ratio;
    const double savings = stats.total_bytes_stored > 0
        ? static_cast<double>(stats.bytes_saved) /
          static_cast<double>(stats.bytes_saved + stats.total_bytes_stored)
        : 0.0;

    state.counters["dedup_ratio"]      = benchmark::Counter(ratio);
    state.counters["savings_pct"]      = benchmark::Counter(savings * 100.0);
    state.counters["total_tensors"]    = benchmark::Counter(
        static_cast<double>(stats.total_tensors));
    state.counters["canonical_tensors"] = benchmark::Counter(
        static_cast<double>(stats.canonical_tensors));
    state.counters["delta_tensors"]    = benchmark::Counter(
        static_cast<double>(stats.delta_tensors));
}

// 10 and 100 weight sets (100 = target for acceptance criterion)
BENCHMARK_REGISTER_F(StorageReductionFixture, BM_StorageReductionRatio)
    ->Arg(10)
    ->Arg(100)
    ->Unit(benchmark::kMillisecond);

BENCHMARK_MAIN();
