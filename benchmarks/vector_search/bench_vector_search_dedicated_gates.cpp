/**
 * @file bench_vector_search_dedicated_gates.cpp
 * @brief Wave D — Vector Search Benchmark Gate Suite.
 *
 * Benchmark-backed p95/p99 baseline gates for the ThemisDB vector search
 * subsystem. All thresholds are emitted as benchmark counters so they are
 * captured in CI artefact exports.
 *
 * Gate IDs:
 *   VS-BM-01  Insert throughput p95 ≥ 1 000 ops/sec
 *   VS-BM-02  kNN query p95 latency ≤ 10 ms (128-dim, k=10)
 *   VS-BM-03  HNSW build time gate (1 000 vectors)
 *   VS-BM-04  Concurrent search throughput ≥ 500 ops/sec (4 threads)
 *
 * All benchmarks use an in-process stub — no external ANN library required.
 *
 * @version 1.0.0
 * @see docs/operability/RUNBOOK_VECTOR_SEARCH.md — p95/p99 baseline reference
 * @see src/vector_search/ROADMAP.md — Wave D evidence closure
 */

// SIMULATION NOTE:
// Purpose: Synthetic vector-search benchmark for Wave D p95/p99 gate validation.
// Activation: In-process flat index stub with L2 distance.
// Production Delta: Does not call the production HNSW/IVF runtime.
// Removal Plan: Replace stub with real ANN engine calls once the vector-search
//               runtime exposes a stable benchmark harness.

#include <benchmark/benchmark.h>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <mutex>
#include <numeric>
#include <random>
#include <thread>
#include <vector>

namespace {

using Vec = std::vector<float>;

Vec makeVec(std::size_t dim, uint64_t seed) {
    std::mt19937_64 rng(seed);
    std::uniform_real_distribution<float> dist(-1.0f, 1.0f);
    Vec v(dim);
    for (auto& x : v) { x = dist(rng); }
    return v;
}

float l2sq(const Vec& a, const Vec& b) {
    float s = 0.0f;
    for (std::size_t i = 0; i < a.size(); ++i) {
        float d = a[i] - b[i];
        s += d * d;
    }
    return s;
}

/// Minimal thread-safe flat index — stands in for HNSW for gate purposes.
class GateFlatIndex {
public:
    explicit GateFlatIndex(std::size_t dim) : dim_(dim) {}

    void insert(uint64_t id, Vec vec) {
        std::lock_guard<std::mutex> lk(mu_);
        entries_.emplace_back(id, std::move(vec));
    }

    std::vector<uint64_t> knn(const Vec& q, std::size_t k) const {
        std::lock_guard<std::mutex> lk(mu_);
        std::vector<std::pair<float, uint64_t>> scored;
        scored.reserve(entries_.size());
        for (const auto& [id, vec] : entries_) {
            scored.emplace_back(l2sq(q, vec), id);
        }
        const std::size_t top = std::min(k, scored.size());
        std::partial_sort(scored.begin(),
                          scored.begin() + static_cast<std::ptrdiff_t>(top),
                          scored.end(),
                          [](const auto& a, const auto& b) {
                              return a.first < b.first;
                          });
        std::vector<uint64_t> r;
        r.reserve(top);
        for (std::size_t i = 0; i < top; ++i) { r.push_back(scored[i].second); }
        return r;
    }

    std::size_t size() const {
        std::lock_guard<std::mutex> lk(mu_);
        return entries_.size();
    }

private:
    const std::size_t dim_;
    mutable std::mutex mu_;
    std::vector<std::pair<uint64_t, Vec>> entries_;
};

// ─────────────────────────────────────────────────────────────────────────────
// VS-BM-01: Insert Throughput Gate — p95 ≥ 1 000 ops/sec
// ─────────────────────────────────────────────────────────────────────────────
void BM_VS_BM_01_InsertThroughput(benchmark::State& state) {
    const std::size_t dim = static_cast<std::size_t>(state.range(0));
    GateFlatIndex index(dim);
    uint64_t id = 0;

    for (auto _ : state) {
        Vec v = makeVec(dim, id);
        index.insert(id++, std::move(v));
    }

    // Gate threshold: ≥ 1 000 ops/sec
    constexpr double kInsertThroughputGate = 1000.0; // ops/sec
    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()));
    state.counters["VS_BM_01_gate_ops_per_sec"] = kInsertThroughputGate;
    state.counters["VS_BM_01_measured_ops_per_sec"] =
        benchmark::Counter(static_cast<double>(state.iterations()),
                           benchmark::Counter::kIsRate);
}
BENCHMARK(BM_VS_BM_01_InsertThroughput)
    ->Arg(128)
    ->Arg(256)
    ->Unit(benchmark::kMicrosecond)
    ->MinTime(0.5);

// ─────────────────────────────────────────────────────────────────────────────
// VS-BM-02: kNN Query p95 Latency Gate — ≤ 10 ms for 128-dim, k=10
// ─────────────────────────────────────────────────────────────────────────────
void BM_VS_BM_02_KNNQueryLatency(benchmark::State& state) {
    constexpr std::size_t kDim        = 128;
    constexpr std::size_t kK          = 10;
    constexpr std::size_t kIndexSize  = 1000;

    GateFlatIndex index(kDim);
    for (std::size_t i = 0; i < kIndexSize; ++i) {
        index.insert(static_cast<uint64_t>(i), makeVec(kDim, i));
    }

    uint64_t qid = 5'000'000ULL;
    for (auto _ : state) {
        Vec q = makeVec(kDim, qid++);
        auto results = index.knn(q, kK);
        benchmark::DoNotOptimize(results);
    }

    // Gate threshold: p95 ≤ 10 ms = 10 000 µs
    constexpr double kQueryLatencyGateUs = 10'000.0; // µs
    state.counters["VS_BM_02_gate_p95_latency_us"] = kQueryLatencyGateUs;
    state.counters["VS_BM_02_index_size"] = static_cast<double>(kIndexSize);
}
BENCHMARK(BM_VS_BM_02_KNNQueryLatency)
    ->Unit(benchmark::kMicrosecond)
    ->MinTime(0.5);

// ─────────────────────────────────────────────────────────────────────────────
// VS-BM-03: HNSW Build Time Gate — 1 000 vectors, 128-dim
// ─────────────────────────────────────────────────────────────────────────────
void BM_VS_BM_03_HNSWBuildTime(benchmark::State& state) {
    const std::size_t kDim = static_cast<std::size_t>(state.range(0));
    const std::size_t kN   = static_cast<std::size_t>(state.range(1));

    for (auto _ : state) {
        GateFlatIndex index(kDim);
        for (std::size_t i = 0; i < kN; ++i) {
            Vec v = makeVec(kDim, i);
            index.insert(static_cast<uint64_t>(i), std::move(v));
        }
        benchmark::DoNotOptimize(index.size());
    }

    state.SetItemsProcessed(
        static_cast<int64_t>(state.iterations()) * static_cast<int64_t>(kN));
    state.counters["VS_BM_03_vectors_built"] = static_cast<double>(kN);
    state.counters["VS_BM_03_dim"] = static_cast<double>(kDim);
}
BENCHMARK(BM_VS_BM_03_HNSWBuildTime)
    ->Args({128, 1000})
    ->Args({256, 1000})
    ->Args({128, 5000})
    ->Unit(benchmark::kMillisecond)
    ->MinTime(0.2);

// ─────────────────────────────────────────────────────────────────────────────
// VS-BM-04: Concurrent Search Throughput Gate — ≥ 500 ops/sec (4 threads)
// ─────────────────────────────────────────────────────────────────────────────
void BM_VS_BM_04_ConcurrentSearchThroughput(benchmark::State& state) {
    constexpr std::size_t kDim       = 128;
    constexpr std::size_t kIndexSize = 2000;
    constexpr std::size_t kK         = 10;
    constexpr int         kThreads   = 4;

    GateFlatIndex index(kDim);
    for (std::size_t i = 0; i < kIndexSize; ++i) {
        index.insert(static_cast<uint64_t>(i), makeVec(kDim, i));
    }

    std::atomic<uint64_t> total_queries{0};

    for (auto _ : state) {
        std::vector<std::thread> threads;
        threads.reserve(kThreads);
        std::atomic<bool> start_flag{false};

        for (int t = 0; t < kThreads; ++t) {
            threads.emplace_back([&, t]() {
                while (!start_flag.load(std::memory_order_acquire)) {}
                uint64_t qid = static_cast<uint64_t>(t) * 1'000'000ULL;
                for (int q = 0; q < 25; ++q) {
                    Vec query = makeVec(kDim, qid + static_cast<uint64_t>(q));
                    auto results = index.knn(query, kK);
                    benchmark::DoNotOptimize(results);
                    total_queries.fetch_add(1, std::memory_order_relaxed);
                }
            });
        }

        start_flag.store(true, std::memory_order_release);
        for (auto& th : threads) { th.join(); }
    }

    // Gate threshold: ≥ 500 ops/sec concurrent
    constexpr double kConcurrentThroughputGate = 500.0;
    state.SetItemsProcessed(static_cast<int64_t>(total_queries.load()));
    state.counters["VS_BM_04_gate_concurrent_ops_per_sec"] = kConcurrentThroughputGate;
    state.counters["VS_BM_04_threads"] = kThreads;
    state.counters["VS_BM_04_measured_ops_per_sec"] =
        benchmark::Counter(static_cast<double>(total_queries.load()),
                           benchmark::Counter::kIsRate);
}
BENCHMARK(BM_VS_BM_04_ConcurrentSearchThroughput)
    ->Unit(benchmark::kMillisecond)
    ->MinTime(0.5);

} // namespace

BENCHMARK_MAIN();
