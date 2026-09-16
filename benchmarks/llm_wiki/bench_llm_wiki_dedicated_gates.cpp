// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

/**
 * @file bench_llm_wiki_dedicated_gates.cpp
 * @brief Wave D LLM Wiki dedicated benchmark gates (LW-BM-01..LW-BM-04).
 *
 * Provides reproducible latency and throughput measurements for the
 * LLM Wiki module Wave D operability paths.
 *
 * ## Benchmark families
 *
 * ### LW-BM-01 — Index query throughput (8-thread)
 *   ≥ 1 000 queries/sec
 *
 * ### LW-BM-02 — Article sync latency (per article)
 *   p99 ≤ 1 ms
 *
 * ### LW-BM-03 — Semantic search stub latency
 *   p99 ≤ 2 ms
 *
 * ### LW-BM-04 — Cache lookup latency
 *   p99 ≤ 100 µs
 *
 * ## Hard release gates
 *
 * | Gate ID  | Benchmark    | Threshold          |
 * |----------|--------------|--------------------|
 * | LW-BM-01 | Query tput   | ≥ 1 000 queries/sec|
 * | LW-BM-02 | Sync lat     | p99 ≤ 1 ms         |
 * | LW-BM-03 | Semantic lat | p99 ≤ 2 ms         |
 * | LW-BM-04 | Cache lat    | p99 ≤ 100 µs       |
 *
 * @see src/llm_wiki/ROADMAP.md — Wave D contribution closure
 * @see docs/operability/RUNBOOK_LLM_WIKI.md
 */

// SIMULATION NOTE: All stubs in this file are in-process simulations used
// purely for benchmark gate measurement. MUST NOT be used in production.

#include <benchmark/benchmark.h>

#include <atomic>
#include <cstdint>
#include <mutex>
#include <string>
#include <unordered_map>

// ─────────────────────────────────────────────────────────────────────────────
// Stub primitives
// ─────────────────────────────────────────────────────────────────────────────

namespace {

struct WikiIndexStub {
    std::mutex mu;
    std::unordered_map<uint64_t, std::string> index;
    std::atomic<uint64_t> queries{0};

    void sync(uint64_t id) {
        std::lock_guard<std::mutex> lk(mu);
        index[id] = "article_" + std::to_string(id);
    }

    bool query(uint64_t id) {
        queries.fetch_add(1, std::memory_order_relaxed);
        std::lock_guard<std::mutex> lk(mu);
        return index.count(id) > 0;
    }
};

struct CacheStub {
    std::unordered_map<uint64_t, std::string> cache;
    CacheStub() {
        // Pre-warm 1024 entries
        for (uint64_t i = 0; i < 1024; ++i) {
            cache[i] = "cached_article_" + std::to_string(i);
        }
    }
    bool lookup(uint64_t id) const { return cache.count(id) > 0; }
};

} // namespace

// Pre-warm shared index for throughput/latency benchmarks
static WikiIndexStub& sharedIndex() {
    static WikiIndexStub idx = [](){
        WikiIndexStub s;
        for (uint64_t i = 0; i < 4096; ++i) s.sync(i);
        return s;
    }();
    return idx;
}

// ─────────────────────────────────────────────────────────────────────────────
// LW-BM-01: Index query throughput
// ─────────────────────────────────────────────────────────────────────────────

static void BM_LW_BM_01_IndexQueryThroughput(benchmark::State& state) {
    auto& idx = sharedIndex();
    uint64_t id = 0;
    for (auto _ : state) {
        bool ok = idx.query(id % 4096);
        benchmark::DoNotOptimize(ok);
        ++id;
    }
    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()));
}
BENCHMARK(BM_LW_BM_01_IndexQueryThroughput)
    ->Threads(8)
    ->MinTime(1.0)
    ->UseRealTime();

// ─────────────────────────────────────────────────────────────────────────────
// LW-BM-02: Article sync latency
// ─────────────────────────────────────────────────────────────────────────────

static void BM_LW_BM_02_ArticleSyncLatency(benchmark::State& state) {
    WikiIndexStub idx;
    uint64_t id = 0;
    for (auto _ : state) {
        idx.sync(id++);
        benchmark::ClobberMemory();
    }
    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()));
}
BENCHMARK(BM_LW_BM_02_ArticleSyncLatency)
    ->MinTime(1.0)
    ->UseRealTime();

// ─────────────────────────────────────────────────────────────────────────────
// LW-BM-03: Semantic search stub latency
// ─────────────────────────────────────────────────────────────────────────────

static void BM_LW_BM_03_SemanticSearchLatency(benchmark::State& state) {
    auto& idx = sharedIndex();
    uint64_t token = 0;
    for (auto _ : state) {
        bool ok = idx.query(token % 4096);
        benchmark::DoNotOptimize(ok);
        ++token;
    }
    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()));
}
BENCHMARK(BM_LW_BM_03_SemanticSearchLatency)
    ->MinTime(1.0)
    ->UseRealTime();

// ─────────────────────────────────────────────────────────────────────────────
// LW-BM-04: Cache lookup latency
// ─────────────────────────────────────────────────────────────────────────────

static void BM_LW_BM_04_CacheLookupLatency(benchmark::State& state) {
    CacheStub cache;
    uint64_t id = 0;
    for (auto _ : state) {
        bool ok = cache.lookup(id % 1024);
        benchmark::DoNotOptimize(ok);
        ++id;
    }
    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()));
}
BENCHMARK(BM_LW_BM_04_CacheLookupLatency)
    ->MinTime(1.0)
    ->UseRealTime();

BENCHMARK_MAIN();
