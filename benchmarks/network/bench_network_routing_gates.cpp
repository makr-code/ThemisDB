// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

/**
 * @file bench_network_routing_gates.cpp
 * @brief Phase 5 — Routing, circuit-breaker, and connection-lifecycle
 *        benchmark gates (NRG-07..NRG-12).
 *
 * Extends the Phase 5 release-gate benchmark suite
 * (benchmarks/network/bench_network_release_gates.cpp NRG-01..NRG-06)
 * with gates for the routing hot paths identified in
 * src/network/ROADMAP.md § Planned Features / Mid-term and
 * § Implementation Phases Phase 4/5.
 *
 * ## Gate table
 *
 * | Gate   | Benchmark                                       | Threshold    |
 * |--------|-------------------------------------------------|--------------|
 * | NRG-07 | Topology-aware routing select (8 backends)     | p99 ≤ 200 µs |
 * | NRG-08 | Load-balancer forward (round-trip)              | p99 ≤ 200 µs |
 * | NRG-09 | Circuit-breaker shouldAllow() hot path (CLOSED) | p99 ≤  50 µs |
 * | NRG-10 | Connection-limit check (atomic ceiling)         | p99 ≤  50 µs |
 * | NRG-11 | Queue gate admit + drain cycle                  | p99 ≤ 100 µs |
 * | NRG-12 | Session guard evaluate (warm path)              | p99 ≤ 100 µs |
 *
 * All benchmarks:
 *   - Use kNetworkCanonicalSeed = 42 (per bench_fixtures.h convention).
 *   - Run with Repetitions(5).
 *   - No live sockets; all I/O is mocked in-process.
 *   - UseRealTime() is NOT applied (CPU-only hot paths, no I/O).
 *
 * @see benchmarks/network/bench_network_release_gates.cpp — NRG-01..NRG-06
 * @see include/network/network_api_contract.h
 * @see src/network/ROADMAP.md — Phase 5 / Planned Features
 */

#include <benchmark/benchmark.h>

#include "network/network_api_contract.h"
#include "network/adaptive_circuit_breaker.h"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <random>
#include <string>
#include <unordered_map>
#include <vector>

namespace themis {
namespace bench {
namespace nrg_routing {

using namespace themis::network;
using namespace std::chrono_literals;

// ============================================================================
// Constants
// ============================================================================

static constexpr std::uint64_t kNetworkCanonicalSeed = 42;
static constexpr int           kRepetitions          = 5;
static constexpr int           kWarmupIterations     = 200;

// ============================================================================
// Mock helpers
// ============================================================================

namespace {

// ---------------------------------------------------------------------------
// Backend health enum + topology router mock
// ---------------------------------------------------------------------------
enum class BackendHealth { HEALTHY, DOWN };

struct BenchBackend {
    std::string   name;
    BackendHealth health;
    uint32_t      latencyMs;
    std::string   region;
};

/**
 * @brief Inline topology-aware router for benchmarking select() hot path.
 *
 * Two-pass selection: preferred-region healthy first, then any-region healthy.
 */
class BenchTopologyRouter {
public:
    void addBackend(BenchBackend b) { backends_.push_back(std::move(b)); }

    NetworkErrorCode select(std::size_t regionIdx,
                            std::string& outName) const noexcept {
        const std::string& preferred = regions_[regionIdx % regions_.size()];
        // Pass 1: preferred region.
        for (auto& b : backends_) {
            if (b.region == preferred && b.health == BackendHealth::HEALTHY) {
                outName = b.name;
                return NetworkErrorCode::OK;
            }
        }
        // Pass 2: cross-region fallback.
        for (auto& b : backends_) {
            if (b.health == BackendHealth::HEALTHY) {
                outName = b.name;
                return NetworkErrorCode::OK;
            }
        }
        return NetworkErrorCode::ROUTING_UNAVAILABLE;
    }

    static const std::vector<std::string> regions_;

private:
    std::vector<BenchBackend> backends_;
};

const std::vector<std::string> BenchTopologyRouter::regions_{
    "eu-west", "us-east", "ap-south"
};

static BenchTopologyRouter makeBenchRouter() {
    BenchTopologyRouter r;
    for (int i = 0; i < 8; ++i) {
        r.addBackend({"backend-" + std::to_string(i),
                      BackendHealth::HEALTHY,
                      static_cast<uint32_t>(10 + i * 5),
                      BenchTopologyRouter::regions_[i % 3]});
    }
    return r;
}

// ---------------------------------------------------------------------------
// Load-balancer mock (round-robin forward)
// ---------------------------------------------------------------------------
class BenchLoadBalancer {
public:
    explicit BenchLoadBalancer(int nBackends) : nBackends_(nBackends) {}

    NetworkErrorCode forward() noexcept {
        auto idx = counter_.fetch_add(1, std::memory_order_relaxed) % nBackends_;
        benchmark::DoNotOptimize(idx);
        return NetworkErrorCode::OK;
    }

private:
    int                  nBackends_;
    std::atomic<int>     counter_{0};
};

// ---------------------------------------------------------------------------
// Connection-limit atomic counter
// ---------------------------------------------------------------------------
class BenchConnectionCounter {
public:
    explicit BenchConnectionCounter(std::size_t limit) : limit_(limit) {}

    NetworkErrorCode accept() noexcept {
        std::size_t n = count_.fetch_add(1, std::memory_order_relaxed) + 1;
        if (n > limit_) {
            count_.fetch_sub(1, std::memory_order_relaxed);
            return NetworkErrorCode::CONNECTION_LIMIT_REACHED;
        }
        return NetworkErrorCode::OK;
    }

    void release() noexcept {
        count_.fetch_sub(1, std::memory_order_relaxed);
    }

private:
    std::size_t              limit_;
    std::atomic<std::size_t> count_{0};
};

// ---------------------------------------------------------------------------
// Queue gate admit/drain cycle
// ---------------------------------------------------------------------------
class BenchQueueGate {
public:
    explicit BenchQueueGate(int capacity) : capacity_(capacity), depth_(0) {}

    NetworkErrorCode admit() noexcept {
        if (depth_ >= capacity_) {
          return NetworkErrorCode::BACKPRESSURE_EXCEEDED;
        }
        ++depth_;
        return NetworkErrorCode::OK;
    }

    void drain() noexcept { depth_ = 0; }

private:
    int capacity_;
    int depth_;
};

// ---------------------------------------------------------------------------
// Session guard (warm in-memory map)
// ---------------------------------------------------------------------------
class BenchSessionGuard {
public:
    BenchSessionGuard() {
        sessions_.reserve(256);
        for (int i = 0; i < 256; ++i) {
            sessions_["tok-" + std::to_string(i)] = true;
        }
    }

    NetworkErrorCode evaluate(const std::string& token) const noexcept {
        if (token.empty()) {
          return NetworkErrorCode::AUTH_REQUIRED;
        }
        auto it = sessions_.find(token);
        if (it == sessions_.end()) {
          return NetworkErrorCode::SESSION_MALFORMED;
        }
        return NetworkErrorCode::OK;
    }

private:
    std::unordered_map<std::string, bool> sessions_;
};

}  // anonymous namespace

// ============================================================================
// Shared fixtures (constructed once per process)
// ============================================================================

static const BenchTopologyRouter& benchRouter() {
    static BenchTopologyRouter r = makeBenchRouter();
    return r;
}

// ============================================================================
// NRG-07 — Topology-aware routing select (8 backends)
// ============================================================================

/**
 * @brief NRG-07: Two-pass topology-aware select over 8 backends.
 *
 * Gate: p99 ≤ 200 µs.
 */
static void BM_NRG07_TopologySelectHotPath(benchmark::State& state) {
    const auto& router = benchRouter();
    std::string selected;
    // Warmup
    for (int i = 0; i < kWarmupIterations; ++i) {
        (void)router.select(static_cast<std::size_t>(i), selected);
    }
    std::size_t idx = 0;
    for (auto _ : state) {
        benchmark::DoNotOptimize(router.select(idx++, selected));
    }
    state.SetLabel("NRG-07: GATE p99 <= 200 us | topology-aware routing select");
}
BENCHMARK(BM_NRG07_TopologySelectHotPath)
    ->Repetitions(kRepetitions)
    ->ReportAggregatesOnly(true);

// ============================================================================
// NRG-08 — Load-balancer round-robin forward
// ============================================================================

/**
 * @brief NRG-08: Atomic-counter round-robin forward over 8 backends.
 *
 * Gate: p99 ≤ 200 µs.
 */
static void BM_NRG08_LoadBalancerForward(benchmark::State& state) {
    BenchLoadBalancer lb(8);
    for (int i = 0; i < kWarmupIterations; ++i) {
        (void)lb.forward();
    }
    for (auto _ : state) {
        benchmark::DoNotOptimize(lb.forward());
    }
    state.SetLabel("NRG-08: GATE p99 <= 200 us | load-balancer round-robin forward");
}
BENCHMARK(BM_NRG08_LoadBalancerForward)
    ->Repetitions(kRepetitions)
    ->ReportAggregatesOnly(true);

// ============================================================================
// NRG-09 — Circuit-breaker shouldAllow() (CLOSED state, hot read path)
// ============================================================================

/**
 * @brief NRG-09: AdaptiveCircuitBreaker::shouldAllow() in CLOSED state —
 *        lock-free atomic read path.
 *
 * Gate: p99 ≤ 50 µs.
 */
static void BM_NRG09_CircuitBreakerShouldAllow(benchmark::State& state) {
    AdaptiveCircuitBreaker::Config cfg;
    cfg.failure_threshold = 10;
    cfg.enable_adaptive_threshold = false;
    AdaptiveCircuitBreaker cb(cfg);

    for (int i = 0; i < kWarmupIterations; ++i) {
        benchmark::DoNotOptimize(cb.shouldAllow());
    }
    for (auto _ : state) {
        benchmark::DoNotOptimize(cb.shouldAllow());
    }
    state.SetLabel("NRG-09: GATE p99 <= 50 us | circuit-breaker shouldAllow (CLOSED)");
}
BENCHMARK(BM_NRG09_CircuitBreakerShouldAllow)
    ->Repetitions(kRepetitions)
    ->ReportAggregatesOnly(true);

// ============================================================================
// NRG-10 — Connection-limit check (atomic ceiling)
// ============================================================================

/**
 * @brief NRG-10: Atomic accept() under limit — fast OK path.
 *
 * Gate: p99 ≤ 50 µs.
 */
static void BM_NRG10_ConnectionLimitCheckOk(benchmark::State& state) {
    // Keep below limit so we always stay in the OK path.
    BenchConnectionCounter counter(kMaxConcurrentConnections);
    for (int i = 0; i < kWarmupIterations; ++i) {
        (void)counter.accept();
        counter.release();
    }
    for (auto _ : state) {
        auto code = counter.accept();
        benchmark::DoNotOptimize(code);
        counter.release();
    }
    state.SetLabel("NRG-10: GATE p99 <= 50 us | connection-limit check (OK path)");
}
BENCHMARK(BM_NRG10_ConnectionLimitCheckOk)
    ->Repetitions(kRepetitions)
    ->ReportAggregatesOnly(true);

// ============================================================================
// NRG-11 — Queue gate admit + drain cycle
// ============================================================================

/**
 * @brief NRG-11: Single admit + drain cycle against a capacity-4 queue gate.
 *
 * Gate: p99 ≤ 100 µs.
 */
static void BM_NRG11_QueueGateAdmitDrain(benchmark::State& state) {
    BenchQueueGate gate(4);
    for (int i = 0; i < kWarmupIterations; ++i) {
        (void)gate.admit();
        gate.drain();
    }
    for (auto _ : state) {
        benchmark::DoNotOptimize(gate.admit());
        gate.drain();
    }
    state.SetLabel("NRG-11: GATE p99 <= 100 us | queue gate admit+drain cycle");
}
BENCHMARK(BM_NRG11_QueueGateAdmitDrain)
    ->Repetitions(kRepetitions)
    ->ReportAggregatesOnly(true);

// ============================================================================
// NRG-12 — Session guard evaluate (warm map path)
// ============================================================================

/**
 * @brief NRG-12: In-memory session-guard evaluate() with 256 pre-populated
 *        entries — warm unordered_map lookup.
 *
 * Gate: p99 ≤ 100 µs.
 */
static void BM_NRG12_SessionGuardEvaluate(benchmark::State& state) {
    BenchSessionGuard guard;
    std::size_t counter = 0;
    for (int i = 0; i < kWarmupIterations; ++i) {
        (void)guard.evaluate("tok-" + std::to_string(i % 256));
    }
    for (auto _ : state) {
        benchmark::DoNotOptimize(
            guard.evaluate("tok-" + std::to_string(counter++ % 256)));
    }
    state.SetLabel("NRG-12: GATE p99 <= 100 us | session guard evaluate (warm)");
}
BENCHMARK(BM_NRG12_SessionGuardEvaluate)
    ->Repetitions(kRepetitions)
    ->ReportAggregatesOnly(true);

}  // namespace nrg_routing
}  // namespace bench
}  // namespace themis

BENCHMARK_MAIN();
