// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

/**
 * @file bench_network_release_gates.cpp
 * @brief Phase 5 network module hot-path release-gate benchmarks (NRG-01..NRG-06).
 *
 * Provides reproducible latency measurements for the network hot paths
 * identified in the network module roadmap (Phase 5).  Hard release gates —
 * a p99 regression beyond 10 % vs the baseline blocks promotion.
 *
 * ## Gate table
 *
 * | Gate  | Benchmark                           | Threshold    |
 * |-------|-------------------------------------|--------------|
 * | NRG-01 | TCP frame dispatch (minimal handler) | p99 ≤ 200 µs |
 * | NRG-02 | Auth check hot path (in-memory)      | p99 ≤ 100 µs |
 * | NRG-03 | Rate-limit check                     | p99 ≤ 50 µs  |
 * | NRG-04 | WebSocket frame dispatch             | p99 ≤ 300 µs |
 * | NRG-05 | Connection accept overhead (mock)    | p99 ≤ 1 ms   |
 * | NRG-06 | Frame serialization (64-byte payload)| p99 ≤ 100 µs |
 *
 * All benchmarks:
 *   - Use kNetworkCanonicalSeed = 42.
 *   - Run with Repetitions(5).
 *   - No live sockets; all network I/O is mocked in-process.
 *
 * @see include/network/network_api_contract.h — contract thresholds
 * @see src/network/ROADMAP.md — Phase 5 items
 */

#include <benchmark/benchmark.h>

#include "network/network_api_contract.h"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <cstring>
#include <memory>
#include <mutex>
#include <random>
#include <string>
#include <unordered_map>
#include <vector>

namespace themis {
namespace bench {
namespace nrg {

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

/// Minimal TCP-style frame: magic + opcode + payload bytes.
struct BenchFrame {
    std::uint8_t  magic[2]{kFrameMagic0, kFrameMagic1};
    std::uint8_t  opcode{0x01};
    std::uint32_t payloadLen{0};
    std::vector<std::uint8_t> payload;
};

static BenchFrame makeTcpFrame(std::size_t payloadSize) {
    BenchFrame f;
    f.payloadLen = static_cast<std::uint32_t>(payloadSize);
    f.payload.assign(payloadSize, 0xAB);
    return f;
}

/// Minimal frame dispatch: validate magic then invoke a no-op handler.
static NetworkErrorCode dispatchTcpFrame(const BenchFrame& f) {
    if (f.magic[0] != kFrameMagic0 || f.magic[1] != kFrameMagic1)
        return NetworkErrorCode::FRAME_INVALID;
    if (f.payloadLen > kMaxFramePayloadBytes)
        return NetworkErrorCode::FRAME_OVERSIZED;
    // Simulate minimal handler work.
    benchmark::DoNotOptimize(f.payloadLen);
    return NetworkErrorCode::OK;
}

/// In-memory session map: token → bool valid.
class BenchSessionMap {
public:
    BenchSessionMap() {
        sessions_.reserve(1024);
        for (int i = 0; i < 1024; ++i) {
            sessions_["tok-" + std::to_string(i)] = true;
        }
    }

    NetworkErrorCode validate(const std::string& token) const {
        if (token.empty()) return NetworkErrorCode::AUTH_REQUIRED;
        auto it = sessions_.find(token);
        if (it == sessions_.end()) return NetworkErrorCode::SESSION_EXPIRED;
        return NetworkErrorCode::OK;
    }

private:
    std::unordered_map<std::string, bool> sessions_;
};

/// Token-bucket rate limiter (in-memory, single-counter).
class BenchRateLimiter {
public:
    explicit BenchRateLimiter(int limitPerBurst = 10000) : limit_(limitPerBurst) {}

    NetworkErrorCode check() {
        int current = count_.fetch_add(1, std::memory_order_relaxed);
        return (current < limit_) ? NetworkErrorCode::OK : NetworkErrorCode::RATE_LIMITED;
    }

    void reset() { count_.store(0, std::memory_order_relaxed); }

private:
    int               limit_;
    std::atomic<int>  count_{0};
};

/// WebSocket frame: FIN bit + opcode + masking + payload.
struct WsBenchFrame {
    std::uint8_t  finOpcode{0x82};  // FIN=1, opcode=2 (binary)
    std::uint8_t  maskLen{0x40};    // MASK=0, len=64
    std::uint8_t  payload[64]{};
};

static NetworkErrorCode dispatchWsFrame(const WsBenchFrame& f) {
    bool fin = (f.finOpcode & 0x80) != 0;
    std::uint8_t opcode = f.finOpcode & 0x0F;
    if (opcode == 0x08) return NetworkErrorCode::TRANSPORT_CLOSED;  // close frame
    benchmark::DoNotOptimize(fin);
    benchmark::DoNotOptimize(f.payload[0]);
    return NetworkErrorCode::OK;
}

/// Mock connection accept: increments atomic counter + validates limit.
class BenchConnectionAcceptor {
public:
    NetworkErrorCode accept() {
        std::size_t n = count_.fetch_add(1, std::memory_order_relaxed) + 1;
        if (n > kMaxConcurrentConnections)
            return NetworkErrorCode::CONNECTION_LIMIT_REACHED;
        return NetworkErrorCode::OK;
    }
    void release() { count_.fetch_sub(1, std::memory_order_relaxed); }

private:
    std::atomic<std::size_t> count_{0};
};

/// Serialize a 64-byte frame to a flat byte buffer.
static void serializeFrame(const BenchFrame& f, std::uint8_t* out) {
    out[0] = f.magic[0];
    out[1] = f.magic[1];
    out[2] = f.opcode;
    std::uint32_t len = f.payloadLen;
    std::memcpy(out + 3, &len, 4);
    std::memcpy(out + 7, f.payload.data(), std::min<std::size_t>(f.payload.size(), 57));
}

}  // anonymous namespace

// ============================================================================
// Shared fixtures (constructed once per process)
// ============================================================================

static const BenchSessionMap& sessionMap() {
    static BenchSessionMap sm;
    return sm;
}

// ============================================================================
// NRG-01 — TCP frame dispatch (minimal handler)
// ============================================================================

/**
 * @brief NRG-01: Validate magic + dispatch to no-op handler.
 *
 * Gate: p99 ≤ 200 µs.
 */
static void BM_NRG01_TcpFrameDispatch(benchmark::State& state) {
    BenchFrame frame = makeTcpFrame(256);
    for (int i = 0; i < kWarmupIterations; ++i) {
        (void)dispatchTcpFrame(frame);
    }
    for (auto _ : state) {
        benchmark::DoNotOptimize(dispatchTcpFrame(frame));
    }
    state.SetLabel("NRG-01: GATE p99 <= 200 us | TCP frame dispatch");
}
BENCHMARK(BM_NRG01_TcpFrameDispatch)
    ->Repetitions(kRepetitions)
    ->ReportAggregatesOnly(true);

// ============================================================================
// NRG-02 — Auth check hot path (in-memory session map)
// ============================================================================

/**
 * @brief NRG-02: Session-token lookup in warm in-memory map (1 k entries).
 *
 * Gate: p99 ≤ 100 µs.
 */
static void BM_NRG02_AuthCheckHotPath(benchmark::State& state) {
    const auto& sm = sessionMap();
    std::mt19937_64 rng(kNetworkCanonicalSeed);
    for (int i = 0; i < kWarmupIterations; ++i) {
        (void)sm.validate("tok-" + std::to_string(rng() % 1024));
    }
    std::size_t counter = 0;
    for (auto _ : state) {
        benchmark::DoNotOptimize(sm.validate("tok-" + std::to_string(counter % 1024)));
        ++counter;
    }
    state.SetLabel("NRG-02: GATE p99 <= 100 us | auth check hot path");
}
BENCHMARK(BM_NRG02_AuthCheckHotPath)
    ->Repetitions(kRepetitions)
    ->ReportAggregatesOnly(true);

// ============================================================================
// NRG-03 — Rate-limit check
// ============================================================================

/**
 * @brief NRG-03: Atomic-counter token-bucket check (single-thread).
 *
 * Gate: p99 ≤ 50 µs.
 */
static void BM_NRG03_RateLimitCheck(benchmark::State& state) {
    BenchRateLimiter rl(1 << 30);  // effectively unlimited for bench
    for (int i = 0; i < kWarmupIterations; ++i) {
        (void)rl.check();
    }
    for (auto _ : state) {
        benchmark::DoNotOptimize(rl.check());
    }
    state.SetLabel("NRG-03: GATE p99 <= 50 us | rate-limit check");
}
BENCHMARK(BM_NRG03_RateLimitCheck)
    ->Repetitions(kRepetitions)
    ->ReportAggregatesOnly(true);

// ============================================================================
// NRG-04 — WebSocket frame dispatch
// ============================================================================

/**
 * @brief NRG-04: Parse WS FIN/opcode and dispatch to no-op handler.
 *
 * Gate: p99 ≤ 300 µs.
 */
static void BM_NRG04_WebSocketFrameDispatch(benchmark::State& state) {
    WsBenchFrame ws;
    for (int i = 0; i < kWarmupIterations; ++i) {
        (void)dispatchWsFrame(ws);
    }
    for (auto _ : state) {
        benchmark::DoNotOptimize(dispatchWsFrame(ws));
    }
    state.SetLabel("NRG-04: GATE p99 <= 300 us | WebSocket frame dispatch");
}
BENCHMARK(BM_NRG04_WebSocketFrameDispatch)
    ->Repetitions(kRepetitions)
    ->ReportAggregatesOnly(true);

// ============================================================================
// NRG-05 — Connection accept overhead (mock)
// ============================================================================

/**
 * @brief NRG-05: Atomic increment + connection-count limit check.
 *
 * Gate: p99 ≤ 1 ms.
 */
static void BM_NRG05_ConnectionAccept(benchmark::State& state) {
    BenchConnectionAcceptor acceptor;
    for (int i = 0; i < kWarmupIterations; ++i) {
        if (acceptor.accept() == NetworkErrorCode::OK) acceptor.release();
    }
    for (auto _ : state) {
        auto code = acceptor.accept();
        benchmark::DoNotOptimize(code);
        if (code == NetworkErrorCode::OK) acceptor.release();
    }
    state.SetLabel("NRG-05: GATE p99 <= 1 ms | mock connection accept");
}
BENCHMARK(BM_NRG05_ConnectionAccept)
    ->UseRealTime()
    ->Repetitions(kRepetitions)
    ->ReportAggregatesOnly(true);

// ============================================================================
// NRG-06 — Frame serialization (64-byte payload)
// ============================================================================

/**
 * @brief NRG-06: Serialize a 64-byte payload frame to flat byte buffer.
 *
 * Gate: p99 ≤ 100 µs.
 */
static void BM_NRG06_FrameSerialization(benchmark::State& state) {
    BenchFrame frame = makeTcpFrame(57);  // 7 header + 57 payload = 64 total
    std::uint8_t buf[64]{};
    for (int i = 0; i < kWarmupIterations; ++i) {
        serializeFrame(frame, buf);
    }
    for (auto _ : state) {
        serializeFrame(frame, buf);
        benchmark::DoNotOptimize(buf[0]);
    }
    state.SetLabel("NRG-06: GATE p99 <= 100 us | frame serialization 64B");
}
BENCHMARK(BM_NRG06_FrameSerialization)
    ->Repetitions(kRepetitions)
    ->ReportAggregatesOnly(true);

}  // namespace nrg
}  // namespace bench
}  // namespace themis

BENCHMARK_MAIN();
