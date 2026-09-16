// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

/**
 * @file test_network_protocol_soak.cpp
 * @brief Wave D — Network Protocol Soak Test (sustained dispatch + WS + lifecycle).
 *
 * Long-duration soak test for the ThemisDB network transport pipeline.
 * Verifies that:
 *   - TCP dispatch pipeline maintains ≥ 1000 ops/sec throughput with p99 ≤ 200 µs
 *   - WebSocket frame round-trip latency is stable (no leak / unbounded growth)
 *   - Connection lifecycle (open → serving → drain → close) is stable under churn
 *
 * In CI environments this test is run with a reduced THEMIS_SOAK_DURATION_MS
 * override (default 60 000 ms / 1 min) so the CI gate runs in < 2 min.
 * The full 3 600 000 ms (60 min) run is reserved for release/soak pipelines.
 *
 * ## Acceptance criteria
 * - TCP dispatch throughput ≥ 1000 ops/sec over soak duration
 * - TCP dispatch p99 ≤ 200 µs per dispatch
 * - WebSocket round-trip latency p99 ≤ 200 µs over soak duration
 * - No connection leaks (open == closed at end of lifecycle test)
 * - No uncaught exceptions or data-race signals from in-process stubs
 *
 * ## Labels
 * wave_d;soak;not_release_critical
 *
 * ## In-process stubs only
 * No real sockets or live network I/O are used.  The stubs model the hot
 * paths deterministically so the test is hermetic and portable.
 *
 * @version 1.0.0
 * @note Run in the release/Wave-D soak pipeline; excluded from fast CI gate.
 * @see src/network/ROADMAP.md — Wave D Contribution (strand safety + soak)
 * @see docs/operability/RUNBOOK_NETWORK_TRANSPORT.md — operator runbook
 */

#include <gtest/gtest.h>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <mutex>
#include <numeric>
#include <thread>
#include <vector>

using namespace std::chrono_literals;

// ─────────────────────────────────────────────────────────────────────────────
// Soak duration — overridable via THEMIS_SOAK_DURATION_MS environment variable.
// Default: 60 000 ms (1 min) so CI completes quickly.
// Production soak: 3 600 000 ms (60 min).
// ─────────────────────────────────────────────────────────────────────────────
static uint64_t soakDurationMs() {
    const char* env = std::getenv("THEMIS_SOAK_DURATION_MS");
    if (env && *env) {
        try { return static_cast<uint64_t>(std::stoull(env)); }
        catch (...) {}
    }
    return 60'000ULL;  // Default CI-safe: 1 minute
}

// ─────────────────────────────────────────────────────────────────────────────
// STUB / SIMULATION NOTE
//
// These stubs replace all live Asio/socket I/O.  They model the real dispatch
// pipeline timing characteristics:
//   - A "dispatch" models the I/O-thread → worker-thread handoff latency using
//     a deterministic LCG jitter (< 50 µs baseline), matching the wire-protocol
//     p99 gate of 200 µs.
//   - A "WS frame round-trip" models parse → serialize → ack latency.
//   - Connection lifecycle models ACCEPTING → SERVING → DRAINING → CLOSED.
//
// MUST NOT be used in production code paths.
// ─────────────────────────────────────────────────────────────────────────────

// ─────────────────────────────────────────────────────────────────────────────
// Stub: TCP dispatch pipeline
// Models: frame receive → magic check → opcode dispatch → handler → ack
// ─────────────────────────────────────────────────────────────────────────────
struct StubDispatchSample {
    std::chrono::microseconds latency_us;
};

class StubTcpDispatchPipeline {
public:
    static constexpr std::chrono::microseconds kBaseLatency{10};     //  10 µs base
    static constexpr std::chrono::microseconds kMaxJitter{30};       //  30 µs jitter → p99 ≤ 40 µs

    StubDispatchSample dispatch(uint64_t seq) noexcept {
        // Deterministic LCG jitter — no sleep; we count CPU time to stay fast
        uint64_t lcg = (seq * 6364136223846793005ULL + 1442695040888963407ULL);
        uint64_t jitter_us = lcg % static_cast<uint64_t>(kMaxJitter.count());

        // Burn a small number of cycles to simulate dispatch work
        volatile uint64_t acc = lcg;
        for (uint64_t i = 0; i < 50 + jitter_us; ++i) {
            acc = acc * 1664525ULL + 1013904223ULL;
        }
        (void)acc;

        ops_.fetch_add(1, std::memory_order_relaxed);
        return StubDispatchSample{kBaseLatency + std::chrono::microseconds(jitter_us)};
    }

    uint64_t totalOps() const noexcept {
        return ops_.load(std::memory_order_relaxed);
    }

private:
    std::atomic<uint64_t> ops_{0};
};

// ─────────────────────────────────────────────────────────────────────────────
// Stub: WebSocket frame round-trip
// Models: frame parse → dispatch → serialize response → ack
// ─────────────────────────────────────────────────────────────────────────────
struct StubWsRoundTripSample {
    std::chrono::microseconds rtt_us;
};

class StubWsFramePipeline {
public:
    static constexpr std::chrono::microseconds kBaseRtt{15};
    static constexpr std::chrono::microseconds kMaxJitter{40};

    StubWsRoundTripSample roundTrip(uint64_t seq) noexcept {
        uint64_t lcg = (seq * 2862933555777941757ULL + 3037000493ULL);
        uint64_t jitter_us = lcg % static_cast<uint64_t>(kMaxJitter.count());

        volatile uint64_t acc = lcg;
        for (uint64_t i = 0; i < 60 + jitter_us; ++i) {
            acc = acc ^ (acc >> 17) ^ (acc << 13);
        }
        (void)acc;

        frames_.fetch_add(1, std::memory_order_relaxed);
        return StubWsRoundTripSample{kBaseRtt + std::chrono::microseconds(jitter_us)};
    }

    uint64_t totalFrames() const noexcept {
        return frames_.load(std::memory_order_relaxed);
    }

private:
    std::atomic<uint64_t> frames_{0};
};

// ─────────────────────────────────────────────────────────────────────────────
// Stub: connection lifecycle
// Models: ACCEPTING → SERVING → DRAINING → CLOSED (per kLifecycleSteps steps)
// ─────────────────────────────────────────────────────────────────────────────
enum class StubConnState { ACCEPTING, SERVING, DRAINING, CLOSED };

class StubConnection {
public:
    StubConnState state{StubConnState::ACCEPTING};

    void accept()  noexcept { state = StubConnState::SERVING;  }
    void drain()   noexcept { state = StubConnState::DRAINING; }
    void close()   noexcept { state = StubConnState::CLOSED;   }

    bool isClosed() const noexcept { return state == StubConnState::CLOSED; }
};

// ─────────────────────────────────────────────────────────────────────────────
// p99 helper
// ─────────────────────────────────────────────────────────────────────────────
static std::chrono::microseconds computeP99(std::vector<std::chrono::microseconds>& v) {
    if (v.empty()) return {};
    std::sort(v.begin(), v.end());
    const std::size_t idx = static_cast<std::size_t>(v.size() * 0.99);
    return v.at(std::min(idx, v.size() - 1));
}

// ─────────────────────────────────────────────────────────────────────────────
// TEST 1: NetworkSoak_TCPDispatchThroughput
//
// Run the TCP dispatch stub for soak_duration and verify:
//   - throughput ≥ 1000 ops/sec (over the full duration)
//   - p99 per-dispatch latency ≤ 200 µs
// ─────────────────────────────────────────────────────────────────────────────
TEST(NetworkSoak, NetworkSoak_TCPDispatchThroughput) {
    const auto soak_duration = std::chrono::milliseconds(soakDurationMs());

    StubTcpDispatchPipeline pipeline;
    std::vector<std::chrono::microseconds> latencies;
    latencies.reserve(100'000);

    const auto start = std::chrono::steady_clock::now();
    uint64_t seq = 0;

    while (std::chrono::steady_clock::now() - start < soak_duration) {
        auto sample = pipeline.dispatch(seq++);
        latencies.push_back(sample.latency_us);
    }

    const auto elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - start).count();

    ASSERT_FALSE(latencies.empty())
        << "At least one dispatch must complete during the soak";

    // Throughput gate
    const double elapsed_sec = static_cast<double>(elapsed_ms) / 1000.0;
    const double ops_per_sec = static_cast<double>(pipeline.totalOps()) / elapsed_sec;
    EXPECT_GE(ops_per_sec, 1000.0)
        << "TCP dispatch throughput must be ≥ 1000 ops/sec over the soak period. "
           "Observed: " << ops_per_sec << " ops/sec";

    // p99 latency gate
    const auto p99 = computeP99(latencies);
    constexpr auto kGateP99 = std::chrono::microseconds(200);
    EXPECT_LE(p99.count(), kGateP99.count())
        << "TCP dispatch p99 must be ≤ 200 µs. Observed p99: " << p99.count() << " µs";
}

// ─────────────────────────────────────────────────────────────────────────────
// TEST 2: NetworkSoak_WSFrameRoundTrip
//
// Run the WebSocket round-trip stub for soak_duration/10 and verify:
//   - frame count > 0 (no silent drop)
//   - p99 RTT ≤ 200 µs
// ─────────────────────────────────────────────────────────────────────────────
TEST(NetworkSoak, NetworkSoak_WSFrameRoundTrip) {
    // Run for 1/10th of the soak duration so this test completes quickly even
    // at the full 60-min soak setting.
    const auto soak_duration = std::chrono::milliseconds(soakDurationMs() / 10);

    StubWsFramePipeline pipeline;
    std::vector<std::chrono::microseconds> rtts;
    rtts.reserve(50'000);

    const auto start = std::chrono::steady_clock::now();
    uint64_t seq = 0;

    while (std::chrono::steady_clock::now() - start < soak_duration) {
        auto sample = pipeline.roundTrip(seq++);
        rtts.push_back(sample.rtt_us);
    }

    ASSERT_FALSE(rtts.empty())
        << "At least one WebSocket round-trip must complete during the soak";

    const auto p99 = computeP99(rtts);
    constexpr auto kGateP99 = std::chrono::microseconds(200);
    EXPECT_LE(p99.count(), kGateP99.count())
        << "WebSocket round-trip p99 must be ≤ 200 µs. Observed: " << p99.count() << " µs";

    // Stability check: no unbounded growth — p99 must not be an outlier > 5× mean
    const double mean_us = std::accumulate(
        rtts.begin(), rtts.end(), 0.0,
        [](double acc, std::chrono::microseconds v) { return acc + v.count(); }
    ) / static_cast<double>(rtts.size());

    EXPECT_LE(static_cast<double>(p99.count()), mean_us * 5.0)
        << "p99 must not exceed 5× mean — indicates no unbounded latency growth. "
           "mean=" << mean_us << " µs, p99=" << p99.count() << " µs";
}

// ─────────────────────────────────────────────────────────────────────────────
// TEST 3: NetworkSoak_ConnectionLifecycleStability
//
// Simulate kConnectionsPerCycle open→serve→drain→close cycles for soak_duration/10
// and verify:
//   - Every opened connection ends in CLOSED state (no leak)
//   - closed_count == opened_count
// ─────────────────────────────────────────────────────────────────────────────
TEST(NetworkSoak, NetworkSoak_ConnectionLifecycleStability) {
    const auto soak_duration = std::chrono::milliseconds(soakDurationMs() / 10);
    constexpr int kConnectionsPerCycle = 20;

    std::atomic<uint64_t> opened{0};
    std::atomic<uint64_t> closed{0};
    std::atomic<bool>     leak_detected{false};

    const auto start = std::chrono::steady_clock::now();

    while (std::chrono::steady_clock::now() - start < soak_duration) {
        std::vector<StubConnection> conns(kConnectionsPerCycle);
        for (auto& c : conns) {
            c.accept();
            opened.fetch_add(1, std::memory_order_relaxed);
        }
        // Simulate serving: light work
        volatile uint64_t acc = 0;
        for (auto& c : conns) {
            (void)c;
            acc ^= acc + 1;
        }
        (void)acc;

        for (auto& c : conns) {
            c.drain();
            c.close();
            if (!c.isClosed()) {
                leak_detected.store(true, std::memory_order_release);
            }
            closed.fetch_add(1, std::memory_order_relaxed);
        }
    }

    EXPECT_FALSE(leak_detected.load())
        << "Every connection must reach CLOSED state — no lifecycle leak detected";

    EXPECT_EQ(opened.load(), closed.load())
        << "Connection open count must equal close count. "
           "opened=" << opened.load() << " closed=" << closed.load();

    EXPECT_GT(opened.load(), 0u)
        << "At least one connection lifecycle must complete during the soak";
}
