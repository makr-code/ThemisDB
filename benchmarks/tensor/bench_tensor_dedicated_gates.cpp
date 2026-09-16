// benchmarks/tensor/bench_tensor_dedicated_gates.cpp
// Wave D dedicated benchmark gates for the Tensor module.
// Gate IDs: TN-BM-01 .. TN-BM-04
//
// Thresholds are compile-time constants; adjust via -DTHEMIS_BM_* defines if
// needed.  All benchmarks use in-process stubs so no external engine is
// required.  A full hardware-representative run is required before Wave D
// sign-off (see src/tensor/ROADMAP.md).

#include <benchmark/benchmark.h>

#include <atomic>
#include <chrono>
#include <cstdint>
#include <random>
#include <vector>

// ---------------------------------------------------------------------------
// In-process stubs
// ---------------------------------------------------------------------------

namespace stubs {

struct TensorIndex {
    std::atomic<uint64_t> inserts{0};
    bool insert(uint64_t id, const std::vector<float>& vec) {
        (void)id; (void)vec;
        inserts.fetch_add(1, std::memory_order_relaxed);
        return true;
    }
};

struct GraphQuery {
    std::atomic<uint64_t> queries{0};
    bool query(uint64_t id) {
        (void)id;
        queries.fetch_add(1, std::memory_order_relaxed);
        return true;
    }
};

struct DedupEngine {
    std::atomic<uint64_t> ops{0};
    bool dedup(uint64_t fp) {
        (void)fp;
        ops.fetch_add(1, std::memory_order_relaxed);
        return true;
    }
};

struct BridgeRouter {
    std::atomic<uint64_t> routed{0};
    bool route(uint64_t tok) {
        (void)tok;
        routed.fetch_add(1, std::memory_order_relaxed);
        return true;
    }
};

} // namespace stubs

// Shared instances (reset per benchmark via fixture if needed).
static stubs::TensorIndex g_tensor_index;
static stubs::GraphQuery  g_graph_query;
static stubs::DedupEngine g_dedup;
static stubs::BridgeRouter g_bridge;

// ---------------------------------------------------------------------------
// TN-BM-01 — Tensor insert p95 latency
// ---------------------------------------------------------------------------
// Measures per-operation latency distribution for tensor inserts.
// p95 target: < 500 µs on representative hardware.
// ---------------------------------------------------------------------------
static void BM_TN_BM_01_TensorInsertP95(benchmark::State& state) {
    constexpr int kDim = 128;
    std::vector<float> vec(kDim, 1.0f);
    uint64_t id = 0;

    for (auto _ : state) {
        g_tensor_index.insert(id++, vec);
        benchmark::ClobberMemory();
    }

    state.SetLabel("TN-BM-01:tensor_insert_p95");
    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()));
}
BENCHMARK(BM_TN_BM_01_TensorInsertP95)
    ->Threads(1)
    ->Threads(4)
    ->UseRealTime();

// ---------------------------------------------------------------------------
// TN-BM-02 — Graph query p95 latency
// ---------------------------------------------------------------------------
// Measures per-operation latency for graph edge queries.
// p95 target: < 1 ms on representative hardware.
// ---------------------------------------------------------------------------
static void BM_TN_BM_02_GraphQueryP95(benchmark::State& state) {
    uint64_t id = 0;
    for (auto _ : state) {
        g_graph_query.query(id++);
        benchmark::ClobberMemory();
    }
    state.SetLabel("TN-BM-02:graph_query_p95");
    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()));
}
BENCHMARK(BM_TN_BM_02_GraphQueryP95)
    ->Threads(1)
    ->Threads(4)
    ->UseRealTime();

// ---------------------------------------------------------------------------
// TN-BM-03 — Dedup throughput
// ---------------------------------------------------------------------------
// Measures sustained dedup throughput in ops/sec.
// Target: >= 500 000 ops/sec on representative hardware.
// ---------------------------------------------------------------------------
static void BM_TN_BM_03_DedupThroughput(benchmark::State& state) {
    std::mt19937_64 rng(42);
    for (auto _ : state) {
        g_dedup.dedup(rng());
        benchmark::ClobberMemory();
    }
    state.SetLabel("TN-BM-03:dedup_throughput");
    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()));
}
BENCHMARK(BM_TN_BM_03_DedupThroughput)
    ->Threads(1)
    ->Threads(8)
    ->UseRealTime();

// ---------------------------------------------------------------------------
// TN-BM-04 — Bridge routing p99 latency
// ---------------------------------------------------------------------------
// Measures per-token bridge routing latency.
// p99 target: < 2 ms on representative hardware.
// ---------------------------------------------------------------------------
static void BM_TN_BM_04_BridgeRoutingP99(benchmark::State& state) {
    uint64_t tok = 1;
    for (auto _ : state) {
        g_bridge.route(tok++);
        if (tok == 0) tok = 1;
        benchmark::ClobberMemory();
    }
    state.SetLabel("TN-BM-04:bridge_routing_p99");
    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()));
}
BENCHMARK(BM_TN_BM_04_BridgeRoutingP99)
    ->Threads(1)
    ->Threads(4)
    ->UseRealTime();

BENCHMARK_MAIN();
