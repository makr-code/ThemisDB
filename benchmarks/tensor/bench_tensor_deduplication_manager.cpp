/*
 * ThemisDB | File: bench_tensor_deduplication_manager.cpp | Version: 0.0.1
 * Maturity: 🟢 PRODUCTION-READY | Score: 100/100
 * Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
 */

#include <benchmark/benchmark.h>

#include "graph/tensor_deduplication_manager.h"
#include "storage/tensor_network_storage_engine.h"
#include "storage/tensor_train_decomposer.h"

#include <memory>
#include <random>
#include <string>
#include <vector>

using themis::graph::DeduplicationConfig;
using themis::graph::FingerprintGraphConfig;
using themis::graph::TensorDeduplicationManager;
using themis::graph::TensorFingerprintGraph;
using themis::storage::InMemoryTensorBackend;
using themis::storage::TensorNetworkStorageEngine;
using themis::storage::TensorStorageConfig;
using themis::storage::TensorTrainDecomposer;

namespace {

std::vector<float> randVec(std::size_t n, unsigned seed) {
    std::mt19937 rng(seed);
    std::normal_distribution<float> d(0.0f, 1.0f);
    std::vector<float> values(n);
    for (auto& value : values) {
        value = d(rng);
    }
    return values;
}

std::shared_ptr<TensorNetworkStorageEngine> makeEngine() {
    auto backend = std::make_shared<InMemoryTensorBackend>();
    TensorStorageConfig cfg;
    cfg.tt_config.eps = 0.05;
    cfg.min_compression_ratio = 0.0;
    return std::make_shared<TensorNetworkStorageEngine>(backend, cfg);
}

std::shared_ptr<TensorDeduplicationManager>
makeDedupManager(const std::shared_ptr<TensorNetworkStorageEngine>& engine,
                 double similarity_threshold = 0.90) {
    FingerprintGraphConfig fp_cfg;
    fp_cfg.similarity_threshold = similarity_threshold;
    fp_cfg.num_hash_funcs = 64;
    fp_cfg.num_bands = 16;
    fp_cfg.cache_trains_in_memory = true;

    auto fp_graph = std::make_shared<TensorFingerprintGraph>(fp_cfg);
    auto decomposer = std::make_shared<TensorTrainDecomposer>();

    DeduplicationConfig cfg;
    cfg.similarity_threshold = similarity_threshold;
    cfg.allow_full_storage_fallback = true;
    return std::make_shared<TensorDeduplicationManager>(
        engine, fp_graph, decomposer, cfg);
}

void preloadCanonicals(TensorDeduplicationManager& mgr, std::size_t count, unsigned seed_base) {
    for (std::size_t i = 0; i < count; ++i) {
        auto data = randVec(16, seed_base + static_cast<unsigned>(i));
        mgr.store("canon_" + std::to_string(i), data, {16, 1}, "tenant", "collection", "field");
    }
}

} // namespace

static void BM_TDM_SnapshotRestoreRoundTrip(benchmark::State& state) {
    const auto tensor_count = static_cast<std::size_t>(state.range(0));

    auto engine = makeEngine();
    auto mgr = makeDedupManager(engine);
    preloadCanonicals(*mgr, tensor_count, 5000);

    for (auto _ : state) {
        const std::string snapshot_key = "bench_snapshot";
        const bool snap_ok = mgr->snapshotGraph(snapshot_key);
        if (!snap_ok) {
            state.SkipWithError("snapshotGraph failed");
            break;
        }

        auto restored_mgr = makeDedupManager(engine);
        const bool restore_ok = restored_mgr->restoreGraph(snapshot_key);
        if (!restore_ok) {
            state.SkipWithError("restoreGraph failed");
            break;
        }
        benchmark::DoNotOptimize(restored_mgr->getStats().total_tensors);
    }

    state.SetItemsProcessed(
        static_cast<int64_t>(state.iterations()) *
        static_cast<int64_t>(tensor_count));
}

BENCHMARK(BM_TDM_SnapshotRestoreRoundTrip)
    ->Arg(100)
    ->Arg(1000)
    ->Unit(benchmark::kMillisecond);

static void BM_TDM_JournalReplayThroughput(benchmark::State& state) {
    const auto mutation_count = static_cast<std::size_t>(state.range(0));

    for (auto _ : state) {
        auto engine = makeEngine();
        auto mgr = makeDedupManager(engine);
        preloadCanonicals(*mgr, 128, 7000);

        const std::string snapshot_key = "journal_snap";
        if (!mgr->snapshotGraph(snapshot_key)) {
            state.SkipWithError("snapshotGraph failed");
            break;
        }

        // Post-snapshot mutations become journal entries.
        for (std::size_t i = 0; i < mutation_count; ++i) {
            auto data = randVec(16, 8000 + static_cast<unsigned>(i));
            mgr->store("mut_" + std::to_string(i % 64), data, {16, 1}, "tenant", "collection", "field");
        }

        auto replay_mgr = makeDedupManager(engine);
        if (!replay_mgr->restoreGraph(snapshot_key)) {
            state.SkipWithError("restoreGraph failed");
            break;
        }
        if (!replay_mgr->replayMutationJournal(snapshot_key)) {
            state.SkipWithError("replayMutationJournal failed");
            break;
        }
        benchmark::DoNotOptimize(replay_mgr->getStats().total_tensors);
    }

    state.SetItemsProcessed(
        static_cast<int64_t>(state.iterations()) *
        static_cast<int64_t>(mutation_count));
    state.counters["journal_mutations_per_sec"] = benchmark::Counter(
        static_cast<double>(state.iterations()) * static_cast<double>(mutation_count),
        benchmark::Counter::kIsRate);
}

BENCHMARK(BM_TDM_JournalReplayThroughput)
    ->Arg(100)
    ->Arg(1000)
    ->Arg(5000)
    ->Unit(benchmark::kMillisecond);

// ─────────────────────────────────────────────────────────────────────────────
// BM_TDM_StoreOverwriteVsInsert
//
// Compares store() performance on overwriting existing keys vs inserting new
// keys. Target: overwrite overhead <= 5% vs insert (FUTURE_ENHANCEMENTS.md).
// ─────────────────────────────────────────────────────────────────────────────

static void BM_TDM_StoreOverwrite(benchmark::State& state) {
    const auto tensor_count = static_cast<std::size_t>(state.range(0));
    
    for (auto _ : state) {
        auto engine = makeEngine();
        auto mgr = makeDedupManager(engine);
        
        // Pre-populate with base tensors
        std::vector<std::string> tensor_ids;
        for (std::size_t i = 0; i < tensor_count; ++i) {
            std::string id = "tensor_" + std::to_string(i);
            tensor_ids.push_back(id);
            auto data = randVec(16, 1000 + static_cast<unsigned>(i));
            mgr->store(id, data, {16, 1}, "tenant", "collection", "field");
        }
        
        // Benchmark: overwrite existing tensors (same IDs, different data)
        state.ResumeTiming();
        for (std::size_t i = 0; i < tensor_count; ++i) {
            auto data = randVec(16, 2000 + static_cast<unsigned>(i));
            mgr->store(tensor_ids[i], data, {16, 1}, "tenant", "collection", "field");
        }
        state.PauseTiming();
        
        benchmark::DoNotOptimize(mgr->getStats().total_bytes_stored);
    }
    
    state.SetLabel("tensors=" + std::to_string(tensor_count));
    state.SetItemsProcessed(
        static_cast<int64_t>(state.iterations()) *
        static_cast<int64_t>(tensor_count));
}

static void BM_TDM_StoreInsert(benchmark::State& state) {
    const auto tensor_count = static_cast<std::size_t>(state.range(0));
    
    for (auto _ : state) {
        auto engine = makeEngine();
        auto mgr = makeDedupManager(engine);
        
        // Benchmark: insert new tensors (fresh IDs each iteration)
        for (std::size_t i = 0; i < tensor_count; ++i) {
            auto data = randVec(16, 3000 + static_cast<unsigned>(i));
            mgr->store("new_tensor_" + std::to_string(i), data, {16, 1}, "tenant", "collection", "field");
        }
        
        benchmark::DoNotOptimize(mgr->getStats().total_bytes_stored);
    }
    
    state.SetLabel("tensors=" + std::to_string(tensor_count));
    state.SetItemsProcessed(
        static_cast<int64_t>(state.iterations()) *
        static_cast<int64_t>(tensor_count));
}

BENCHMARK(BM_TDM_StoreOverwrite)
    ->Arg(100)
    ->Arg(1000)
    ->Unit(benchmark::kMillisecond)
    ->Repetitions(3);

BENCHMARK(BM_TDM_StoreInsert)
    ->Arg(100)
    ->Arg(1000)
    ->Unit(benchmark::kMillisecond)
    ->Repetitions(3);

// ─────────────────────────────────────────────────────────────────────────────
// BM_TDM_MixedReadHeavyWorkload
//
// Measures sustained throughput for mixed workload: 90% queries/retrieval,
// 10% store/update operations. Target: >= 2,000 ops/sec without unbounded
// memory growth (FUTURE_ENHANCEMENTS.md).
// ─────────────────────────────────────────────────────────────────────────────

static void BM_TDM_MixedReadHeavyWorkload(benchmark::State& state) {
    const auto total_ops = static_cast<std::size_t>(state.range(0));
    const std::size_t query_ops = (total_ops * 90) / 100;
    const std::size_t update_ops = (total_ops * 10) / 100;
    
    auto engine = makeEngine();
    auto mgr = makeDedupManager(engine);
    
    // Pre-populate with base tensors
    for (std::size_t i = 0; i < 100; ++i) {
        auto data = randVec(16, 5000 + static_cast<unsigned>(i));
        mgr->store("base_" + std::to_string(i), data, {16, 1}, "tenant", "collection", "field");
    }
    
    std::mt19937 rng(42);
    std::uniform_int_distribution<std::size_t> base_dist(0, 99);
    std::size_t tensor_counter = 100;
    
    // Initial memory snapshot
    auto initial_stats = mgr->getStats();
    
    for (auto _ : state) {
        // 90% reads
        for (std::size_t i = 0; i < query_ops; ++i) {
            std::size_t idx = base_dist(rng);
            auto rec = mgr->getRecord("base_" + std::to_string(idx));
            benchmark::DoNotOptimize(rec);
        }
        
        // 10% writes (new tensors)
        for (std::size_t i = 0; i < update_ops; ++i) {
            auto data = randVec(16, 6000 + static_cast<unsigned>(tensor_counter));
            mgr->store("dynamic_" + std::to_string(tensor_counter++), data, 
                      {16, 1}, "tenant", "collection", "field");
        }
    }
    
    // Track memory growth (informational)
    auto final_stats = mgr->getStats();
    state.counters["ops_per_sec"] = benchmark::Counter(
        static_cast<double>(state.iterations()) * static_cast<double>(total_ops),
        benchmark::Counter::kIsRate);
    state.counters["total_bytes_stored"] = 
        static_cast<double>(final_stats.total_bytes_stored);
    
    state.SetLabel("ops=" + std::to_string(total_ops));
    state.SetItemsProcessed(
        static_cast<int64_t>(state.iterations()) *
        static_cast<int64_t>(total_ops));
}

BENCHMARK(BM_TDM_MixedReadHeavyWorkload)
    ->Arg(1000)
    ->Arg(10000)
    ->Unit(benchmark::kMillisecond)
    ->Repetitions(3)
    ->UseRealTime();

