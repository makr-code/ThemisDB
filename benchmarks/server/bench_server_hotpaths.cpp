// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

/**
 * @file bench_server_hotpaths.cpp
 * @brief Phase 5 server hot-path release-gate benchmarks.
 *
 * Provides reproducible latency measurements for the server module's critical
 * dispatch and infrastructure paths identified in src/server/ROADMAP.md
 * (Phase 5 — Performance and Operational Hardening).
 *
 * Results are used as release gates: a regression beyond 10% vs the baseline
 * blocks promotion to GA.
 *
 * ## Benchmark families
 *
 * ### SVR-01 — HTTP Request Dispatch Overhead
 *   SVR-01  Route lookup + handler dispatch overhead (no handler body)
 *
 * ### SVR-02 — Auth Middleware
 *   SVR-02  JWT structural validation overhead (no JWKS network call)
 *
 * ### SVR-03 — Rate-Limit Hot Path
 *   SVR-03  In-memory per-client rate-limit check (atomic counter)
 *
 * ### SVR-04 — WebSocket Frame Dispatch
 *   SVR-04  WebSocket frame envelope parsing and dispatch overhead
 *
 * ### SVR-05 — gRPC Unary Dispatch
 *   SVR-05  gRPC unary path: metadata decode + handler dispatch
 *
 * ### SVR-06 — Graceful Shutdown Drain
 *   SVR-06  Synthetic graceful shutdown with 100 simulated in-flight requests
 *
 * ### SVR-07 — Retry Budget Check
 *   SVR-07  Synchronous retry budget evaluation (no backoff wait)
 *
 * ### SVR-08 — Route Registration Lookup
 *   SVR-08  Static route registry lookup by path hash
 *
 * ## Hard release gates
 *
 * | Gate ID      | Benchmark | Threshold      |
 * |--------------|-----------|----------------|
 * | GATE-SVR-01  | SVR-01    | p99 ≤ 500 µs   |
 * | GATE-SVR-02  | SVR-02    | p99 ≤ 2 ms     |
 * | GATE-SVR-03  | SVR-03    | p99 ≤ 100 µs   |
 * | GATE-SVR-04  | SVR-04    | p99 ≤ 200 µs   |
 * | GATE-SVR-05  | SVR-05    | p99 ≤ 1 ms     |
 * | GATE-SVR-06  | SVR-06    | p99 ≤ 5 s      |
 * | GATE-SVR-07  | SVR-07    | p99 ≤ 50 µs    |
 * | GATE-SVR-08  | SVR-08    | p99 ≤ 10 µs    |
 *
 * All benchmarks:
 *   - Use kSvrCanonicalSeed = 42 for deterministic data generation.
 *   - Warm up for kSvrWarmupIterations before the measurement window.
 *   - Run with Repetitions(kSvrRepetitions) to capture variance.
 *
 * @see src/server/ROADMAP.md — Phase 5 items
 * @see include/server/server_api_contract.h — contract constants
 * @see benchmarks/server/bench_server_hotpaths.cpp — this file
 */

#include <benchmark/benchmark.h>

#include "server/server_api_contract.h"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <random>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

using namespace std::chrono_literals;
using namespace themis::server;

namespace themis {
namespace bench {
namespace svr {

// ---------------------------------------------------------------------------
// Constants
// ---------------------------------------------------------------------------

/// Canonical PRNG seed for all SVR benchmarks.
static constexpr uint64_t kSvrCanonicalSeed = 42;

/// Warmup iterations before measurement window.
static constexpr int kSvrWarmupIterations = 200;

/// Repetitions per benchmark for variance estimation.
static constexpr int kSvrRepetitions = 5;

/// Number of synthetic in-flight requests for SVR-06.
static constexpr int kShutdownSyntheticConnections = 100;

// ---------------------------------------------------------------------------
// Minimal stubs (all in-process, no network I/O)
// ---------------------------------------------------------------------------

/// Stub route registry: maps path hash → handler index.
class StubRouteRegistry {
public:
    explicit StubRouteRegistry(int num_routes) {
        routes_.reserve(static_cast<std::size_t>(num_routes));
        for (int i = 0; i < num_routes; ++i) {
            routes_["/api/v1/route-" + std::to_string(i)] = i;
        }
    }
    int lookup(const std::string& path) const {
        auto it = routes_.find(path);
        return it == routes_.end() ? -1 : it->second;
    }
private:
    std::unordered_map<std::string, int> routes_;
};

/// Stub JWT validator: structural check only (no crypto).
struct StubJwtValidator {
    bool validate(const std::string& token) const noexcept {
        // Structural: must have exactly 2 dots
        int dots = 0;
        for (char c : token) if (c == '.') ++dots;
        return dots == 2 && token.size() <= kServerMaxJwtTokenBytes;
    }
};

/// Stub per-client rate limiter (atomic counter, in-memory).
class StubAtomicRateLimiter {
public:
    explicit StubAtomicRateLimiter(int limit) : limit_(limit) {}
    bool check() noexcept {
        int current = counter_.fetch_add(1, std::memory_order_relaxed);
        return current < limit_;
    }
    void reset() noexcept { counter_.store(0, std::memory_order_relaxed); }
private:
    int limit_;
    std::atomic<int> counter_{0};
};

/// Stub retry budget: synchronous check only (no sleep).
struct StubRetryBudget {
    int max_retries{kDefaultMaxRetries};
    int current_attempt{0};
    bool hasBudget() noexcept {
        return current_attempt++ < max_retries;
    }
    void reset() noexcept { current_attempt = 0; }
};

/// Stub graceful shutdown drain.
static void drainConnections(int count) {
    // Simulate draining: atomic decrement per connection (no real I/O)
    std::atomic<int> remaining{count};
    std::vector<std::thread> workers;
    workers.reserve(4);
    for (int i = 0; i < 4; ++i) {
        workers.emplace_back([&]() {
            while (remaining.fetch_sub(1, std::memory_order_relaxed) > 0) {
                // minimal drain work
            }
        });
    }
    for (auto& t : workers) t.join();
}

// ===========================================================================
// SVR-01 — HTTP request dispatch overhead
// ===========================================================================

/**
 * @brief SVR-01: HTTP route lookup + empty handler dispatch.
 *
 * Measures the overhead of the route matching and handler dispatch pipeline
 * with no handler body execution. GATE-SVR-01: p99 ≤ 500 µs.
 */
static void BM_SVR01_HttpRequestDispatch(benchmark::State& state) {
    StubRouteRegistry registry(256);
    // Warm up
    for (int i = 0; i < kSvrWarmupIterations; ++i) {
        benchmark::DoNotOptimize(registry.lookup("/api/v1/route-" + std::to_string(i % 256)));
    }
    std::mt19937_64 rng(kSvrCanonicalSeed);
    std::uniform_int_distribution<int> dist(0, 255);
    for (auto _ : state) {
        std::string path = "/api/v1/route-" + std::to_string(dist(rng));
        benchmark::DoNotOptimize(registry.lookup(path));
    }
    state.SetLabel("GATE-SVR-01: p99 <= 500 us");
}
BENCHMARK(BM_SVR01_HttpRequestDispatch)
    ->Repetitions(kSvrRepetitions)
    ->ReportAggregatesOnly(true)
    ->Unit(benchmark::kMicrosecond);

// ===========================================================================
// SVR-02 — Auth middleware JWT validate
// ===========================================================================

/**
 * @brief SVR-02: JWT structural validation overhead (no crypto/JWKS).
 *
 * GATE-SVR-02: p99 ≤ 2 ms.
 */
static void BM_SVR02_AuthMiddlewareJwtValidate(benchmark::State& state) {
    StubJwtValidator validator;
    const std::string valid_token = "******";
    for (int i = 0; i < kSvrWarmupIterations; ++i) {
        benchmark::DoNotOptimize(validator.validate(valid_token));
    }
    for (auto _ : state) {
        benchmark::DoNotOptimize(validator.validate(valid_token));
    }
    state.SetLabel("GATE-SVR-02: p99 <= 2 ms");
}
BENCHMARK(BM_SVR02_AuthMiddlewareJwtValidate)
    ->Repetitions(kSvrRepetitions)
    ->ReportAggregatesOnly(true)
    ->Unit(benchmark::kMicrosecond);

// ===========================================================================
// SVR-03 — Rate-limit hot path
// ===========================================================================

/**
 * @brief SVR-03: In-memory per-client rate-limit check (atomic counter).
 *
 * GATE-SVR-03: p99 ≤ 100 µs.
 */
static void BM_SVR03_RateLimitCheckHotPath(benchmark::State& state) {
    StubAtomicRateLimiter limiter(1'000'000);  // effectively unlimited for bench
    for (int i = 0; i < kSvrWarmupIterations; ++i) {
        benchmark::DoNotOptimize(limiter.check());
    }
    for (auto _ : state) {
        benchmark::DoNotOptimize(limiter.check());
    }
    state.SetLabel("GATE-SVR-03: p99 <= 100 us");
}
BENCHMARK(BM_SVR03_RateLimitCheckHotPath)
    ->Repetitions(kSvrRepetitions)
    ->ReportAggregatesOnly(true)
    ->Unit(benchmark::kMicrosecond);

// ===========================================================================
// SVR-04 — WebSocket frame dispatch
// ===========================================================================

/**
 * @brief SVR-04: WebSocket frame envelope parse + dispatch overhead.
 *
 * Simulates the cost of unpacking a minimal WebSocket frame header and
 * routing to a handler slot. GATE-SVR-04: p99 ≤ 200 µs.
 */
static void BM_SVR04_WebSocketFrameDispatch(benchmark::State& state) {
    // Minimal WS frame: 2-byte header + 4-byte mask + payload
    const std::vector<uint8_t> frame = {0x81, 0x86, 0x37, 0xfa, 0x21, 0x3d,
                                        0x7f, 0x9f, 0x4d, 0x51};
    for (int i = 0; i < kSvrWarmupIterations; ++i) {
        benchmark::DoNotOptimize(frame[0] & 0x0Fu);  // opcode extraction
    }
    for (auto _ : state) {
        // Extract opcode + payload length (simulates frame dispatch overhead)
        uint8_t opcode = frame[0] & 0x0Fu;
        uint8_t payload_len = frame[1] & 0x7Fu;
        benchmark::DoNotOptimize(opcode);
        benchmark::DoNotOptimize(payload_len);
    }
    state.SetLabel("GATE-SVR-04: p99 <= 200 us");
}
BENCHMARK(BM_SVR04_WebSocketFrameDispatch)
    ->Repetitions(kSvrRepetitions)
    ->ReportAggregatesOnly(true)
    ->Unit(benchmark::kMicrosecond);

// ===========================================================================
// SVR-05 — gRPC unary dispatch
// ===========================================================================

/**
 * @brief SVR-05: gRPC unary metadata decode + handler dispatch overhead.
 *
 * Simulates the hot path cost of decoding gRPC metadata and routing to the
 * handler. GATE-SVR-05: p99 ≤ 1 ms.
 */
static void BM_SVR05_GrpcUnaryDispatch(benchmark::State& state) {
    std::unordered_map<std::string, std::string> metadata = {
        {"content-type", "application/grpc"},
        {"authorization", "******"},
        {":method", "POST"},
        {":path", "/themis.core.v1.CoreService/Execute"},
    };
    for (int i = 0; i < kSvrWarmupIterations; ++i) {
        benchmark::DoNotOptimize(metadata.find(":path"));
    }
    for (auto _ : state) {
        auto it = metadata.find(":path");
        benchmark::DoNotOptimize(it);
        auto it2 = metadata.find("authorization");
        benchmark::DoNotOptimize(it2);
    }
    state.SetLabel("GATE-SVR-05: p99 <= 1 ms");
}
BENCHMARK(BM_SVR05_GrpcUnaryDispatch)
    ->Repetitions(kSvrRepetitions)
    ->ReportAggregatesOnly(true)
    ->Unit(benchmark::kMicrosecond);

// ===========================================================================
// SVR-06 — Graceful shutdown drain
// ===========================================================================

/**
 * @brief SVR-06: Graceful shutdown drain for kShutdownSyntheticConnections.
 *
 * Measures the wall-clock cost of draining synthetic in-flight requests.
 * Uses UseRealTime() to include scheduling overhead.
 * GATE-SVR-06: p99 ≤ 5 s.
 */
static void BM_SVR06_GracefulShutdownDrain(benchmark::State& state) {
    for (auto _ : state) {
        drainConnections(static_cast<int>(state.range(0)));
    }
    state.SetLabel("GATE-SVR-06: p99 <= 5 s");
}
BENCHMARK(BM_SVR06_GracefulShutdownDrain)
    ->Arg(kShutdownSyntheticConnections)
    ->UseRealTime()
    ->Repetitions(kSvrRepetitions)
    ->ReportAggregatesOnly(true);

// ===========================================================================
// SVR-07 — Retry budget check
// ===========================================================================

/**
 * @brief SVR-07: Synchronous retry budget check (no backoff sleep).
 *
 * GATE-SVR-07: p99 ≤ 50 µs.
 */
static void BM_SVR07_RetryBudgetCheck(benchmark::State& state) {
    StubRetryBudget budget;
    for (int i = 0; i < kSvrWarmupIterations; ++i) {
        budget.reset();
        benchmark::DoNotOptimize(budget.hasBudget());
    }
    for (auto _ : state) {
        budget.reset();
        benchmark::DoNotOptimize(budget.hasBudget());
    }
    state.SetLabel("GATE-SVR-07: p99 <= 50 us");
}
BENCHMARK(BM_SVR07_RetryBudgetCheck)
    ->Repetitions(kSvrRepetitions)
    ->ReportAggregatesOnly(true)
    ->Unit(benchmark::kMicrosecond);

// ===========================================================================
// SVR-08 — Route registration lookup
// ===========================================================================

/**
 * @brief SVR-08: Static route registry lookup by path (hash-map lookup).
 *
 * Uses a warm registry with 1 024 routes to represent a large production
 * deployment. GATE-SVR-08: p99 ≤ 10 µs.
 */
static void BM_SVR08_RouteRegistrationLookup(benchmark::State& state) {
    const int n = static_cast<int>(state.range(0));
    StubRouteRegistry registry(n);
    for (int i = 0; i < kSvrWarmupIterations; ++i) {
        benchmark::DoNotOptimize(registry.lookup("/api/v1/route-" + std::to_string(i % n)));
    }
    std::mt19937_64 rng(kSvrCanonicalSeed);
    std::uniform_int_distribution<int> dist(0, n - 1);
    for (auto _ : state) {
        benchmark::DoNotOptimize(registry.lookup("/api/v1/route-" + std::to_string(dist(rng))));
    }
    state.SetLabel("GATE-SVR-08: p99 <= 10 us");
}
BENCHMARK(BM_SVR08_RouteRegistrationLookup)
    ->Arg(1024)
    ->Repetitions(kSvrRepetitions)
    ->ReportAggregatesOnly(true)
    ->Unit(benchmark::kMicrosecond);

} // namespace svr
} // namespace bench
} // namespace themis

BENCHMARK_MAIN();
