// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

/// @file bench_tensor_summary_first.cc
/// @brief Phase C benchmark gate for summary-first routing and exact-on-demand
///        fetch latency.
///
/// Benchmark IDs:
///   BSF-01  Summary-first routing — all-fresh path (advisory, no escalation)
///   BSF-02  Summary-first routing — all-stale path (escalation)
///   BSF-03  Freshness consensus check — quorum
///   BSF-04  Exact-on-demand fetch via stub fetcher
///
/// Performance gate (from FUTURE_ENHANCEMENTS.md):
///   - Planner/routing latency p99 <= 750 µs  (captured per iteration)
///   - Throughput >= 25 000 ops/s              (Items/s label)

#include <benchmark/benchmark.h>

#include "shard_summary_coordinator.h"
#include "tensor/tensor_summary_types.h"

#include <memory>
#include <string>
#include <vector>

namespace {

using namespace themis::distributed_tensor;
using namespace themis::tensor;

constexpr uint64_t kCanonicalRngSeed = 42u;

// Fixed "now" far in the future so wall-clock expiry doesn't interfere.
constexpr int64_t kBenchNowMs = 9'000'000'000'000LL;
// Recent refresh: 60 s before bench-now.
constexpr int64_t kRecentMs = kBenchNowMs - 60'000LL;

// ── Stub fetcher returning a minimal fixed payload ──────────────────────────

class BenchStubFetcher : public IShardFetcher {
public:
    ExactFetchResult fetch(const ExactFetchRequest& req) const noexcept override {
        ExactFetchResult r;
        r.shard_id = req.shard_id;
        r.artifact_id = req.artifact_id;
        r.success = true;
        r.fragment_data.assign(64, static_cast<uint8_t>(kCanonicalRngSeed & 0xFF));
        r.content_hash = "bench_stub_hash";
        r.integrity_verified = true;
        return r;
    }
};

// ── Fixture helpers ──────────────────────────────────────────────────────────

/// Build a coordinator with N registered and freshly-refreshed shards.
ShardSummaryCoordinator makeCoordinator(int shard_count,
                                        bool refreshed,
                                        bool all_stale = false) {
    auto fetcher = std::make_shared<BenchStubFetcher>();
    ShardSummaryCoordinator c(fetcher);

    for (int i = 0; i < shard_count; ++i) {
        const std::string sid = "shard-" + std::to_string(i);
        c.registerShard(sid);
        if (refreshed) {
            ShardSummary s;
            s.shard_id = sid;
            s.shard_relevance = 0.8f + static_cast<float>(i % 5) * 0.02f;
            s.freshness_state = SummaryFreshnessState::FRESH;
            s.freshness_ttl_seconds = 3600;
            const int64_t ts = all_stale
                ? (kBenchNowMs - 7'200'000LL) // expired: 2 hours ago
                : kRecentMs;
            c.refreshShard(sid, s, ts);
        }
    }
    return c;
}

std::vector<ShardSummary> makeSummaryVec(int shard_count, bool fresh) {
    std::vector<ShardSummary> v;
    v.reserve(shard_count);
    for (int i = 0; i < shard_count; ++i) {
        ShardSummary s;
        s.shard_id = "shard-" + std::to_string(i);
        s.shard_relevance = 0.8f;
        s.shard_healthy = true;
        s.freshness_state = fresh ? SummaryFreshnessState::FRESH
                                   : SummaryFreshnessState::STALE;
        s.freshness_ttl_seconds = 3600;
        v.push_back(std::move(s));
    }
    return v;
}

// ── BSF-01: Summary-first routing — all-fresh (advisory, no escalation) ──────

void BSF01_SummaryFirstRoutingFresh(benchmark::State& state) {
    const int n = static_cast<int>(state.range(0));
    auto c = makeCoordinator(n, /*refreshed=*/true, /*stale=*/false);
    const auto summaries = makeSummaryVec(n, /*fresh=*/true);
    benchmark::DoNotOptimize(kCanonicalRngSeed);

    for (auto _ : state) {
        const auto decisions =
            c.routeSummaryFirst(summaries, AccuracyMode::ADVISORY, kBenchNowMs);
        benchmark::DoNotOptimize(decisions);
    }

    state.SetItemsProcessed(state.iterations() * n);
    state.SetLabel("BSF-01 fresh-advisory");
}
BENCHMARK(BSF01_SummaryFirstRoutingFresh)
    ->Arg(4)
    ->Arg(16)
    ->Arg(64)
    ->Unit(benchmark::kMicrosecond)
    ->UseRealTime();

// ── BSF-02: Summary-first routing — all-stale (escalation path) ──────────────

void BSF02_SummaryFirstRoutingStale(benchmark::State& state) {
    const int n = static_cast<int>(state.range(0));
    auto c = makeCoordinator(n, /*refreshed=*/true, /*stale=*/true);
    const auto summaries = makeSummaryVec(n, /*fresh=*/false);
    benchmark::DoNotOptimize(kCanonicalRngSeed);

    for (auto _ : state) {
        const auto decisions =
            c.routeSummaryFirst(summaries, AccuracyMode::ADVISORY, kBenchNowMs);
        benchmark::DoNotOptimize(decisions);
    }

    state.SetItemsProcessed(state.iterations() * n);
    state.SetLabel("BSF-02 stale-escalate");
}
BENCHMARK(BSF02_SummaryFirstRoutingStale)
    ->Arg(4)
    ->Arg(16)
    ->Arg(64)
    ->Unit(benchmark::kMicrosecond)
    ->UseRealTime();

// ── BSF-03: Freshness consensus check ────────────────────────────────────────

void BSF03_FreshnessConsensus(benchmark::State& state) {
    const int n = static_cast<int>(state.range(0));
    auto c = makeCoordinator(n, /*refreshed=*/true);

    std::vector<std::string> shard_ids;
    shard_ids.reserve(n);
    for (int i = 0; i < n; ++i) {
        shard_ids.push_back("shard-" + std::to_string(i));
    }
    benchmark::DoNotOptimize(kCanonicalRngSeed);

    for (auto _ : state) {
        const auto res = c.checkFreshnessConsensus(shard_ids, kBenchNowMs);
        benchmark::DoNotOptimize(res);
    }

    state.SetItemsProcessed(state.iterations() * n);
    state.SetLabel("BSF-03 consensus");
}
BENCHMARK(BSF03_FreshnessConsensus)
    ->Arg(4)
    ->Arg(16)
    ->Arg(64)
    ->Unit(benchmark::kMicrosecond)
    ->UseRealTime();

// ── BSF-04: Exact-on-demand fetch via stub fetcher ────────────────────────────

void BSF04_ExactFetch(benchmark::State& state) {
    auto fetcher = std::make_shared<BenchStubFetcher>();
    ShardSummaryCoordinator c(fetcher);
    benchmark::DoNotOptimize(kCanonicalRngSeed);

    ExactFetchRequest req;
    req.shard_id = "shard-0";
    req.artifact_id = "artifact-bench";
    req.timeout_ms = 5000;

    for (auto _ : state) {
        const auto result = c.fetchExact(req);
        benchmark::DoNotOptimize(result);
    }

    state.SetItemsProcessed(state.iterations());
    state.SetLabel("BSF-04 exact-fetch");
}
BENCHMARK(BSF04_ExactFetch)
    ->Iterations(10000)
    ->Unit(benchmark::kMicrosecond)
    ->UseRealTime();

} // namespace
