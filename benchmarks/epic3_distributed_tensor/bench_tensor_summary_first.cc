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

// ── BSF-05: Freshness query latency for single shard ──────────────────────────

void BSF05_FreshnessQuery(benchmark::State& state) {
    ShardSummaryCoordinator c;
    c.registerShard("shard-0");

    ShardSummary s;
    s.shard_id = "shard-0";
    s.shard_relevance = 0.9f;
    s.freshness_state = SummaryFreshnessState::FRESH;
    s.freshness_ttl_seconds = 3600;

    c.refreshShard("shard-0", s, kRecentMs);
    benchmark::DoNotOptimize(kCanonicalRngSeed);

    for (auto _ : state) {
        const bool fresh = c.isFresh("shard-0", kBenchNowMs);
        benchmark::DoNotOptimize(fresh);
    }

    state.SetItemsProcessed(state.iterations());
    state.SetLabel("BSF-05 freshness-query");
}
BENCHMARK(BSF05_FreshnessQuery)
    ->Iterations(100000)
    ->Unit(benchmark::kMicrosecond)
    ->UseRealTime();

// ── BSF-06: Routing latency with varying shard counts ──────────────────────────

void BSF06_RoutingLatencyScaling(benchmark::State& state) {
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
    state.SetLabel("BSF-06 routing-scaling");
}
BENCHMARK(BSF06_RoutingLatencyScaling)
    ->Arg(1)
    ->Arg(4)
    ->Arg(16)
    ->Arg(64)
    ->Arg(256)
    ->Unit(benchmark::kMicrosecond)
    ->UseRealTime();

// ── BSF-07: Consensus check latency ──────────────────────────────────────────

void BSF07_ConsensusLatency(benchmark::State& state) {
    const int n = static_cast<int>(state.range(0));
    auto c = makeCoordinator(n, /*refreshed=*/true, /*stale=*/false);

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
    state.SetLabel("BSF-07 consensus-latency");
}
BENCHMARK(BSF07_ConsensusLatency)
    ->Arg(4)
    ->Arg(16)
    ->Arg(64)
    ->Unit(benchmark::kMicrosecond)
    ->UseRealTime();

// ── BSF-08: Refresh throughput (advisory summary updates) ────────────────────

void BSF08_RefreshThroughput(benchmark::State& state) {
    auto c = makeCoordinator(64, /*refreshed=*/false);
    std::unordered_map<std::string, ShardSummary> summaries = {};

    for (int i = 0; i < 64; ++i) {
        ShardSummary s;
        s.shard_id = "shard-" + std::to_string(i);
        s.shard_relevance = 0.8f;
        s.freshness_state = SummaryFreshnessState::STALE;
        s.freshness_ttl_seconds = 3600;
        summaries.emplace(s.shard_id, std::move(s));
    }
    benchmark::DoNotOptimize(kCanonicalRngSeed);

    int64_t ts = kBenchNowMs;
    for (auto _ : state) {
        const auto results = c.refreshAll(summaries, ts);
        benchmark::DoNotOptimize(results);
        ts += 1000;  // Increment time to vary results
    }

    state.SetItemsProcessed(state.iterations() * 64);
    state.SetLabel("BSF-08 refresh-throughput");
}
BENCHMARK(BSF08_RefreshThroughput)
    ->Iterations(1000)
    ->Unit(benchmark::kMicrosecond)
    ->UseRealTime();

// ── BSF-09: Escalation count with mixed freshness ─────────────────────────────

void BSF09_EscalationMetrics(benchmark::State& state) {
    const int n = static_cast<int>(state.range(0));
    auto fetcher = std::make_shared<BenchStubFetcher>();
    ShardSummaryCoordinator c(fetcher);

    std::vector<ShardSummary> summaries = {};

    for (int i = 0; i < n; ++i) {
        ShardSummary s;
        s.shard_id = "shard-" + std::to_string(i);
        s.shard_relevance = 0.8f + static_cast<float>(i % 5) * 0.02f;
        s.shard_healthy = true;
        // Alternate between fresh and stale
        s.freshness_state = (i % 2 == 0) ? SummaryFreshnessState::FRESH
                                          : SummaryFreshnessState::STALE;
        s.freshness_ttl_seconds = 3600;
        summaries.push_back(std::move(s));
    }
    benchmark::DoNotOptimize(kCanonicalRngSeed);

    for (auto _ : state) {
        const auto decisions =
            c.routeSummaryFirst(summaries, AccuracyMode::ADVISORY, kBenchNowMs);
        benchmark::DoNotOptimize(decisions);
    }

    state.SetItemsProcessed(state.iterations() * n);
    state.SetLabel("BSF-09 escalation-metrics");
}
BENCHMARK(BSF09_EscalationMetrics)
    ->Arg(16)
    ->Arg(64)
    ->Unit(benchmark::kMicrosecond)
    ->UseRealTime();

// ── BSF-10: Exact fetch with multiple concurrent requests (single-threaded simulation) ─

void BSF10_FetchBatch(benchmark::State& state) {
    auto fetcher = std::make_shared<BenchStubFetcher>();
    ShardSummaryCoordinator c(fetcher);

    std::vector<RoutingDecision> decisions = {};

    for (int i = 0; i < 8; ++i) {
        decisions.push_back({
            .shard_id = "shard-" + std::to_string(i),
            .include_shard = true,
            .escalate_to_exact = true,
        });
    }
    benchmark::DoNotOptimize(kCanonicalRngSeed);

    for (auto _ : state) {
        const auto results = c.fetchEscalated(decisions, "artifact-bench");
        benchmark::DoNotOptimize(results);
    }

    state.SetItemsProcessed(state.iterations() * 8);
    state.SetLabel("BSF-10 fetch-batch");
}
BENCHMARK(BSF10_FetchBatch)
    ->Iterations(1000)
    ->Unit(benchmark::kMicrosecond)
    ->UseRealTime();

// ── BSF-11: Stats snapshot latency (atomic counter reads) ──────────────────────

void BSF11_StatsSnapshot(benchmark::State& state) {
    auto fetcher = std::make_shared<BenchStubFetcher>();
    ShardSummaryCoordinator c(fetcher);

    // Pre-populate some stats
    for (int i = 0; i < 100; ++i) {
        ShardSummary s;
        s.shard_id = "shard-" + std::to_string(i % 16);
        s.shard_relevance = 0.8f;
        s.freshness_state = SummaryFreshnessState::FRESH;
        c.refreshShard(s.shard_id, s, kBenchNowMs);
    }
    benchmark::DoNotOptimize(kCanonicalRngSeed);

    for (auto _ : state) {
        const auto stats = c.stats();
        benchmark::DoNotOptimize(stats);
    }

    state.SetItemsProcessed(state.iterations());
    state.SetLabel("BSF-11 stats-snapshot");
}
BENCHMARK(BSF11_StatsSnapshot)
    ->Iterations(100000)
    ->Unit(benchmark::kNanosecond)
    ->UseRealTime();

// ── BSF-12: Planner latency p99 target (synthesis of routing + consensus checks) ─

void BSF12_PlannerLatencyP99(benchmark::State& state) {
    const int n = static_cast<int>(state.range(0));
    auto c = makeCoordinator(n, /*refreshed=*/true);

    std::vector<ShardSummary> summaries = makeSummaryVec(n, /*fresh=*/true);
    std::vector<std::string> shard_ids = {};

    for (int i = 0; i < n; ++i) {
        shard_ids.push_back("shard-" + std::to_string(i));
    }

    benchmark::DoNotOptimize(kCanonicalRngSeed);

    // Simulate a planner decision: route + check consensus
    for (auto _ : state) {
        const auto decisions =
            c.routeSummaryFirst(summaries, AccuracyMode::ADVISORY, kBenchNowMs);
        const auto consensus = c.checkFreshnessConsensus(shard_ids, kBenchNowMs);
        benchmark::DoNotOptimize(decisions);
        benchmark::DoNotOptimize(consensus);
    }

    state.SetItemsProcessed(state.iterations());
    state.SetLabel("BSF-12 planner-latency-p99");
}
BENCHMARK(BSF12_PlannerLatencyP99)
    ->Arg(4)
    ->Arg(16)
    ->Arg(64)
    ->Unit(benchmark::kMicrosecond)
    ->UseRealTime();

} // namespace
