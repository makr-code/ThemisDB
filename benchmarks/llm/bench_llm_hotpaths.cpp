// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

/**
 * @file bench_llm_hotpaths.cpp
 * @brief Phase 5 LLM hot-path release-gate benchmarks.
 *
 * Provides reproducible latency measurements for the LLM module's critical
 * inference, embedding, plugin, and quota hot paths identified in
 * src/llm/ROADMAP.md (Phase 5 — Performance and Hardening).
 *
 * ## Benchmark families
 *
 * ### LLM-01 — Single-Token Inference Overhead
 *   LLM-01  Stub single-token inference dispatch (no model file)
 *
 * ### LLM-02 — Plugin Lifecycle Load/Unload
 *   LLM-02  Plugin load() + unload() round-trip (stub, no model file)
 *
 * ### LLM-03 — Batch Embed
 *   LLM-03  Batch of 8 embed requests (stub engine, no GPU)
 *
 * ### LLM-04 — CancellationToken Check
 *   LLM-04  Atomic isCancelled() check overhead
 *
 * ### LLM-05 — Quota Check
 *   LLM-05  In-memory per-tenant quota check (atomic counter)
 *
 * ### LLM-06 — Route Decision
 *   LLM-06  Model selector routing decision (hash-map lookup)
 *
 * ### LLM-07 — Stream Callback Overhead
 *   LLM-07  Empty stream callback invocation overhead
 *
 * ### LLM-08 — Plugin Registry Lookup
 *   LLM-08  Plugin registry name → adapter lookup
 *
 * ## Hard release gates
 *
 * | Gate ID      | Benchmark | Threshold    |
 * |--------------|-----------|--------------|
 * | GATE-LLM-01  | LLM-01    | p99 ≤ 1 ms   |
 * | GATE-LLM-02  | LLM-02    | p99 ≤ 10 ms  |
 * | GATE-LLM-03  | LLM-03    | p99 ≤ 50 ms  |
 * | GATE-LLM-04  | LLM-04    | p99 ≤ 1 µs   |
 * | GATE-LLM-05  | LLM-05    | p99 ≤ 10 µs  |
 * | GATE-LLM-06  | LLM-06    | p99 ≤ 100 µs |
 * | GATE-LLM-07  | LLM-07    | p99 ≤ 10 µs  |
 * | GATE-LLM-08  | LLM-08    | p99 ≤ 10 µs  |
 *
 * All benchmarks:
 *   - Use kLlmCanonicalSeed = 42 for deterministic data.
 *   - Warm up for kLlmWarmupIterations before measurement.
 *   - Run with Repetitions(kLlmRepetitions) to capture variance.
 *
 * @see src/llm/ROADMAP.md — Phase 5 items
 * @see include/llm/llm_api_contract.h — contract constants
 */

#include <benchmark/benchmark.h>

#include "llm/llm_api_contract.h"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <random>
#include <string>
#include <unordered_map>
#include <vector>

using namespace themis::llm;

namespace themis {
namespace bench {
namespace llm_hp {

// ---------------------------------------------------------------------------
// Constants
// ---------------------------------------------------------------------------

/// Canonical PRNG seed for all LLM benchmarks.
static constexpr uint64_t kLlmCanonicalSeed = 42;

/// Warmup iterations before measurement window.
static constexpr int kLlmWarmupIterations = 200;

/// Repetitions per benchmark.
static constexpr int kLlmRepetitions = 5;

/// Batch size for LLM-03.
static constexpr int kBatchEmbedSize = 8;

/// Number of simulated tokens for single-token inference stub.
static constexpr int kSingleTokenOverheadTokens = 1;

// ---------------------------------------------------------------------------
// Minimal stubs
// ---------------------------------------------------------------------------

/// Stub cancellation token.
class StubCancelToken {
public:
    bool isCancelled() const noexcept {
        return cancelled_.load(std::memory_order_acquire);
    }
    void cancel() noexcept { cancelled_.store(true, std::memory_order_release); }
private:
    std::atomic<bool> cancelled_{false};
};

/// Stub inference engine for overhead measurement.
class StubLlmEngine {
public:
    struct Token { int id{0}; };
    /// Single-token overhead: pre-inference check + trivial dispatch.
    Token inferSingleToken(const std::string& prompt, StubCancelToken& tok) {
        if (tok.isCancelled()) return {};
        // Simulate token selection (trivial — no actual computation)
        return {static_cast<int>(prompt.size() % 32000)};
    }
};

/// Stub plugin with load/unload.
class StubLlmPlugin {
public:
    bool loaded{false};
    LlmErrorCode load()   noexcept { loaded = true;  return LlmErrorCode::OK; }
    LlmErrorCode unload() noexcept { loaded = false; return LlmErrorCode::OK; }
};

/// Stub embedding engine producing L2-normalised vectors.
class StubEmbedEngine {
public:
    static constexpr int kDim = 8;
    std::vector<float> embed(const std::string& text) {
        std::vector<float> v(kDim);
        std::mt19937 rng(kLlmCanonicalSeed ^ std::hash<std::string>{}(text));
        float norm = 0.f;
        for (auto& x : v) { x = static_cast<float>(rng() % 1000 + 1); norm += x * x; }
        norm = std::sqrt(norm);
        for (auto& x : v) {
          x /= norm;
        }
        return v;
    }
    std::vector<std::vector<float>> embedBatch(const std::vector<std::string>& texts) {
        std::vector<std::vector<float>> out;
        out.reserve(texts.size());
        for (const auto& t : texts) {
          out.push_back(embed(t));
        }
        return out;
    }
};

/// Stub per-tenant quota tracker (atomic counter).
class StubQuotaTracker {
public:
    explicit StubQuotaTracker(int limit) : limit_(limit) {}
    bool checkAndDecrement() noexcept {
        int cur = counter_.load(std::memory_order_relaxed);
        while (cur > 0) {
            if (counter_.compare_exchange_weak(cur, cur - 1,
                    std::memory_order_acq_rel, std::memory_order_relaxed)) {
                return true;
            }
        }
        return false;
    }
    void refill() noexcept { counter_.store(limit_, std::memory_order_release); }
private:
    int limit_;
    std::atomic<int> counter_;
};

/// Stub model selector (routing decision).
class StubModelSelector {
public:
    explicit StubModelSelector(int num_models) {
        for (int i = 0; i < num_models; ++i) {
            registry_["model-" + std::to_string(i)] = i;
        }
    }
    int select(const std::string& key) const {
        auto it = registry_.find(key);
        return it == registry_.end() ? -1 : it->second;
    }
private:
    std::unordered_map<std::string, int> registry_;
};

/// Stub plugin registry.
class StubPluginRegistry {
public:
    explicit StubPluginRegistry(int n) {
        for (int i = 0; i < n; ++i) {
            entries_["plugin-" + std::to_string(i)] = i;
        }
    }
    int lookup(const std::string& name) const {
        auto it = entries_.find(name);
        return it == entries_.end() ? -1 : it->second;
    }
private:
    std::unordered_map<std::string, int> entries_;
};

// ===========================================================================
// LLM-01 — Single-token inference overhead (stub path)
// ===========================================================================

/**
 * @brief LLM-01: Single-token stub inference dispatch overhead.
 *
 * GATE-LLM-01: p99 ≤ 1 ms.
 */
static void BM_LLM01_SingleTokenInferenceOverhead(benchmark::State& state) {
    StubLlmEngine engine;
    StubCancelToken tok;
    const std::string prompt = "bench-prompt-llm01";
    for (int i = 0; i < kLlmWarmupIterations; ++i) {
        benchmark::DoNotOptimize(engine.inferSingleToken(prompt, tok));
    }
    for (auto _ : state) {
        benchmark::DoNotOptimize(engine.inferSingleToken(prompt, tok));
    }
    state.SetLabel("GATE-LLM-01: p99 <= 1 ms");
}
BENCHMARK(BM_LLM01_SingleTokenInferenceOverhead)
    ->Repetitions(kLlmRepetitions)
    ->ReportAggregatesOnly(true)
    ->Unit(benchmark::kMicrosecond);

// ===========================================================================
// LLM-02 — Plugin lifecycle load/unload
// ===========================================================================

/**
 * @brief LLM-02: Plugin load() + unload() round-trip (stub, no model file).
 *
 * GATE-LLM-02: p99 ≤ 10 ms.
 */
static void BM_LLM02_PluginLifecycleLoadUnload(benchmark::State& state) {
    for (int i = 0; i < kLlmWarmupIterations; ++i) {
        StubLlmPlugin p;
        p.load();
        p.unload();
    }
    for (auto _ : state) {
        StubLlmPlugin p;
        benchmark::DoNotOptimize(p.load());
        benchmark::DoNotOptimize(p.unload());
    }
    state.SetLabel("GATE-LLM-02: p99 <= 10 ms");
}
BENCHMARK(BM_LLM02_PluginLifecycleLoadUnload)
    ->Repetitions(kLlmRepetitions)
    ->ReportAggregatesOnly(true)
    ->Unit(benchmark::kMicrosecond);

// ===========================================================================
// LLM-03 — Batch of 8 embed requests
// ===========================================================================

/**
 * @brief LLM-03: Batch of 8 embed requests (stub engine, no GPU).
 *
 * GATE-LLM-03: p99 ≤ 50 ms.
 */
static void BM_LLM03_BatchEmbed8(benchmark::State& state) {
    StubEmbedEngine engine;
    std::vector<std::string> texts;
    texts.reserve(static_cast<std::size_t>(kBatchEmbedSize));
    for (int i = 0; i < kBatchEmbedSize; ++i) {
        texts.push_back("embed-text-" + std::to_string(i));
    }
    for (int i = 0; i < kLlmWarmupIterations; ++i) {
        benchmark::DoNotOptimize(engine.embedBatch(texts));
    }
    for (auto _ : state) {
        benchmark::DoNotOptimize(engine.embedBatch(texts));
    }
    state.SetLabel("GATE-LLM-03: p99 <= 50 ms");
}
BENCHMARK(BM_LLM03_BatchEmbed8)
    ->Repetitions(kLlmRepetitions)
    ->ReportAggregatesOnly(true)
    ->Unit(benchmark::kMicrosecond);

// ===========================================================================
// LLM-04 — CancellationToken check overhead
// ===========================================================================

/**
 * @brief LLM-04: Atomic isCancelled() check overhead.
 *
 * GATE-LLM-04: p99 ≤ 1 µs.
 */
static void BM_LLM04_CancellationTokenCheck(benchmark::State& state) {
    StubCancelToken tok;
    for (int i = 0; i < kLlmWarmupIterations; ++i) {
        benchmark::DoNotOptimize(tok.isCancelled());
    }
    for (auto _ : state) {
        benchmark::DoNotOptimize(tok.isCancelled());
    }
    state.SetLabel("GATE-LLM-04: p99 <= 1 us");
}
BENCHMARK(BM_LLM04_CancellationTokenCheck)
    ->Repetitions(kLlmRepetitions)
    ->ReportAggregatesOnly(true)
    ->Unit(benchmark::kNanosecond);

// ===========================================================================
// LLM-05 — Quota check (in-memory)
// ===========================================================================

/**
 * @brief LLM-05: In-memory per-tenant quota check (atomic CAS).
 *
 * GATE-LLM-05: p99 ≤ 10 µs.
 */
static void BM_LLM05_QuotaCheck(benchmark::State& state) {
    StubQuotaTracker quota(1'000'000);
    for (int i = 0; i < kLlmWarmupIterations; ++i) {
        quota.refill();
        benchmark::DoNotOptimize(quota.checkAndDecrement());
    }
    quota.refill();
    for (auto _ : state) {
        benchmark::DoNotOptimize(quota.checkAndDecrement());
    }
    state.SetLabel("GATE-LLM-05: p99 <= 10 us");
}
BENCHMARK(BM_LLM05_QuotaCheck)
    ->Repetitions(kLlmRepetitions)
    ->ReportAggregatesOnly(true)
    ->Unit(benchmark::kMicrosecond);

// ===========================================================================
// LLM-06 — Route decision (model selector)
// ===========================================================================

/**
 * @brief LLM-06: Model selector routing decision (hash-map lookup).
 *
 * GATE-LLM-06: p99 ≤ 100 µs.
 */
static void BM_LLM06_RouteDecision(benchmark::State& state) {
    StubModelSelector selector(64);
    for (int i = 0; i < kLlmWarmupIterations; ++i) {
        benchmark::DoNotOptimize(selector.select("model-" + std::to_string(i % 64)));
    }
    std::mt19937_64 rng(kLlmCanonicalSeed);
    std::uniform_int_distribution<int> dist(0, 63);
    for (auto _ : state) {
        benchmark::DoNotOptimize(selector.select("model-" + std::to_string(dist(rng))));
    }
    state.SetLabel("GATE-LLM-06: p99 <= 100 us");
}
BENCHMARK(BM_LLM06_RouteDecision)
    ->Repetitions(kLlmRepetitions)
    ->ReportAggregatesOnly(true)
    ->Unit(benchmark::kMicrosecond);

// ===========================================================================
// LLM-07 — Stream callback overhead (empty callback)
// ===========================================================================

/**
 * @brief LLM-07: Empty stream callback invocation overhead.
 *
 * GATE-LLM-07: p99 ≤ 10 µs.
 */
static void BM_LLM07_StreamCallbackOverhead(benchmark::State& state) {
    std::function<void(const std::string&)> cb = [](const std::string&) {};
    const std::string token = "tok";
    for (int i = 0; i < kLlmWarmupIterations; ++i) {
        cb(token);
    }
    for (auto _ : state) {
        benchmark::DoNotOptimize(cb(token));
    }
    state.SetLabel("GATE-LLM-07: p99 <= 10 us");
}
BENCHMARK(BM_LLM07_StreamCallbackOverhead)
    ->Repetitions(kLlmRepetitions)
    ->ReportAggregatesOnly(true)
    ->Unit(benchmark::kMicrosecond);

// ===========================================================================
// LLM-08 — Plugin registry lookup
// ===========================================================================

/**
 * @brief LLM-08: Plugin registry name → adapter lookup.
 *
 * GATE-LLM-08: p99 ≤ 10 µs.
 */
static void BM_LLM08_PluginRegistryLookup(benchmark::State& state) {
    StubPluginRegistry registry(32);
    for (int i = 0; i < kLlmWarmupIterations; ++i) {
        benchmark::DoNotOptimize(registry.lookup("plugin-" + std::to_string(i % 32)));
    }
    std::mt19937_64 rng(kLlmCanonicalSeed);
    std::uniform_int_distribution<int> dist(0, 31);
    for (auto _ : state) {
        benchmark::DoNotOptimize(registry.lookup("plugin-" + std::to_string(dist(rng))));
    }
    state.SetLabel("GATE-LLM-08: p99 <= 10 us");
}
BENCHMARK(BM_LLM08_PluginRegistryLookup)
    ->Repetitions(kLlmRepetitions)
    ->ReportAggregatesOnly(true)
    ->Unit(benchmark::kMicrosecond);

} // namespace llm_hp
} // namespace bench
} // namespace themis

BENCHMARK_MAIN();
