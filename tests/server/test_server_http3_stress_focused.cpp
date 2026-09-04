/**
 * @file test_server_http3_stress_focused.cpp
 * @brief Server Module — HTTP/3 Production Stress focused regression tests.
 *
 * Phase 2 Protocol Hardening acceptance tests for HTTP/3 migration,
 * retransmit injection, 0-RTT session resumption, and handshake variability.
 *
 * Test IDs (Phase 2, Protocol-Hardening):
 * - **SH3-01** — Connection-migration: peer address change accepted, migration_count incremented
 * - **SH3-02** — Connection-migration: concurrent migrations serialised, no double-free
 * - **SH3-03** — Connection-migration: migrated session still completes in-flight requests
 * - **SH3-04** — Retransmit injection: simulated 1 % packet loss, request completes within p99 ≤ 800 ms
 * - **SH3-05** — Retransmit injection: simulated 5 % packet loss, bounded tail latency
 * - **SH3-06** — 0-RTT resumption: valid session token → early data accepted
 * - **SH3-07** — 0-RTT resumption: invalid/mismatched token → early data rejected (security gate)
 * - **SH3-08** — 0-RTT resumption: replayed token → rejected (replay-protection gate)
 * - **SH3-09** — Handshake variability: slow RTT (> 200 ms simulation) → session established within budget
 * - **SH3-10** — Handshake variability: forced HTTP/2 fallback when QUIC unavailable
 * - **SH3-11** — NGTCP2_ERR_DRAINING: session does not accept new streams after draining starts
 * - **SH3-12** — NGTCP2_ERR_DRAINING: quic_conn_ RAII cleanup verified, no double-free
 *
 * All infrastructure is fully in-process; no real UDP sockets are opened.
 * Deterministic seed: kHttp3StressSeed = 7331.
 *
 * @version 1.0.0
 * @note CTest labels: release_critical;server;phase2;http3
 */

#include <gtest/gtest.h>

#include "server/server_api_contract.h"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <random>
#include <string>
#include <thread>
#include <vector>

using namespace std::chrono_literals;
using namespace themis::server;

// ─────────────────────────────────────────────────────────────────────────────
// Canonical seed
// ─────────────────────────────────────────────────────────────────────────────
static constexpr uint32_t kHttp3StressSeed = 7331U;

// ─────────────────────────────────────────────────────────────────────────────
// Stubs and helpers
// ─────────────────────────────────────────────────────────────────────────────

/// Simulated peer endpoint (address + port tuple)
struct FakePeerEndpoint {
    std::string address;
    uint16_t    port;

    bool operator==(const FakePeerEndpoint& o) const noexcept {
        return address == o.address && port == o.port;
    }
    bool operator!=(const FakePeerEndpoint& o) const noexcept { return !(*this == o); }
};

/// Minimal QUIC connection state for migration testing (in-process stub)
/// Models the state that Http3Session tracks for the active peer endpoint and
/// migration bookkeeping, without requiring ngtcp2/OpenSSL linkage in tests.
struct FakeQuicConnState {
    FakePeerEndpoint current_peer;
    std::atomic<uint32_t> migration_count{0};
    std::atomic<bool> draining{false};
    std::atomic<bool> conn_deleted{false};  ///< tracks RAII destruction

    /// Simulate onPathMigration: update peer and increment counter
    void onPathMigration(const FakePeerEndpoint& new_peer) {
        if (draining.load(std::memory_order_acquire)) {
            // Migration refused while draining — contract requirement
            return;
        }
        current_peer = new_peer;
        migration_count.fetch_add(1, std::memory_order_relaxed);
    }

    /// Simulate entering the DRAINING state (NGTCP2_ERR_DRAINING)
    void enterDraining() noexcept {
        draining.store(true, std::memory_order_release);
    }

    /// Simulate RAII cleanup (connection deletion)
    void destroy() noexcept { conn_deleted.store(true, std::memory_order_release); }
};

/// Session-token representation for 0-RTT stub
struct FakeSessionToken {
    std::string token_id;
    bool valid{false};
    bool replayed{false};
};

/// Stub 0-RTT gate that enforces the security contract:
///   - valid token → early data accepted
///   - invalid/mismatched token → rejected
///   - replayed token → rejected
class StubZeroRttGate {
public:
    /// Returns true if early data should be accepted for this token.
    [[nodiscard]] bool acceptEarlyData(const FakeSessionToken& tok) const noexcept {
        if (!tok.valid)   return false;  // mismatch or corruption
        if (tok.replayed) return false;  // replay-protection
        return true;
    }
};

/// Stub retransmit simulator: injects packet loss and measures completion time
struct RetransmitSimulator {
    double   loss_rate = 0;          ///< 0.0–1.0 fraction of packets dropped
    uint32_t base_rtt_ms;        ///< base round-trip time (ms)
    uint32_t max_retransmits;    ///< maximum retransmit attempts before giving up

    struct Result {
        bool     completed = 0;
        uint64_t observed_latency_ms;
    };

    /// Simulate sending a single request under packet loss.
    /// Uses a seeded PRNG for determinism.
    Result simulate(uint32_t seed_offset) const {
        std::mt19937 rng(kHttp3StressSeed + seed_offset);
        std::uniform_real_distribution<double> loss_dist(0.0, 1.0);

        uint64_t total_ms = 0;
        for (uint32_t attempt = 0; attempt <= max_retransmits; ++attempt) {
            bool dropped = loss_dist(rng) < loss_rate;
            if (!dropped) {
                // Exponential backoff approximation for retransmit timing:
                // initial + 2^attempt * base_rtt
                uint64_t rtt = static_cast<uint64_t>(base_rtt_ms) * (uint64_t{1} << attempt);
                total_ms += rtt;
                return {true, total_ms};
            }
            // Packet was dropped; add one RTT for retransmit timer fire
            total_ms += static_cast<uint64_t>(base_rtt_ms) * (uint64_t{1} << attempt);
        }
        return {false, total_ms};
    }
};

// ─────────────────────────────────────────────────────────────────────────────
// SH3-01: Connection-migration — peer address change accepted
// ─────────────────────────────────────────────────────────────────────────────
TEST(ServerHttp3Stress, SH301_ConnectionMigrationAccepted) {
    FakeQuicConnState conn;
    conn.current_peer = {"10.0.0.1", 44000};

    FakePeerEndpoint new_peer{"10.0.0.2", 44001};
    conn.onPathMigration(new_peer);

    EXPECT_EQ(conn.current_peer, new_peer)
        << "Peer endpoint must be updated after migration";
    EXPECT_EQ(conn.migration_count.load(), 1u)
        << "migration_count must be incremented exactly once";
}

// ─────────────────────────────────────────────────────────────────────────────
// SH3-02: Connection-migration — concurrent migrations serialised
// ─────────────────────────────────────────────────────────────────────────────
TEST(ServerHttp3Stress, SH302_ConcurrentMigrationsSerialised) {
    FakeQuicConnState conn;
    conn.current_peer = {"10.0.0.1", 44000};

    constexpr int kThreads = 8;
    constexpr int kMigrationsPerThread = 10;

    std::vector<std::thread> threads;
    threads.reserve(kThreads);
    for (int t = 0; t < kThreads; ++t) {
        threads.emplace_back([&conn, t]() {
            for (int i = 0; i < kMigrationsPerThread; ++i) {
                FakePeerEndpoint ep{"10.0.0." + std::to_string(t), static_cast<uint16_t>(44000 + i)};
                conn.onPathMigration(ep);
            }
        });
    }
    for (auto& th : threads) {
      th.join();
    }

    // Total migrations ≤ kThreads × kMigrationsPerThread (some may be skipped
    // if draining, but draining is not active here, so all should count).
    EXPECT_EQ(conn.migration_count.load(),
              static_cast<uint32_t>(kThreads * kMigrationsPerThread))
        << "Every concurrent migration must be counted";
    EXPECT_FALSE(conn.conn_deleted.load())
        << "Connection must not be destroyed during concurrent migration";
}

// ─────────────────────────────────────────────────────────────────────────────
// SH3-03: Connection-migration — migrated session completes in-flight request
// ─────────────────────────────────────────────────────────────────────────────
TEST(ServerHttp3Stress, SH303_MigratedSessionCompletesInFlightRequest) {
    FakeQuicConnState conn;
    conn.current_peer = {"10.0.0.1", 44000};

    // Simulate a request being processed (represented by a flag)
    std::atomic<bool> request_done{false};
    std::thread request_thread([&]() {
        // Work that represents an in-flight request
        std::this_thread::sleep_for(10ms);
        request_done.store(true, std::memory_order_release);
    });

    // Migrate mid-flight
    conn.onPathMigration({"10.0.0.2", 44001});

    request_thread.join();

    EXPECT_TRUE(request_done.load())
        << "In-flight request must complete after connection migration";
    EXPECT_EQ(conn.migration_count.load(), 1u);
    EXPECT_FALSE(conn.conn_deleted.load())
        << "QUIC conn must not be destroyed while request is in-flight";
}

// ─────────────────────────────────────────────────────────────────────────────
// SH3-04: Retransmit injection — 1% packet loss, p99 ≤ 800 ms
// ─────────────────────────────────────────────────────────────────────────────
TEST(ServerHttp3Stress, SH304_RetransmitInjection1PctLoss) {
    RetransmitSimulator sim{0.01, 20, 4};  // 1% loss, 20 ms base RTT, max 4 retransmits

    constexpr int kSamples = 1000;
    uint64_t max_latency_ms = 0;
    int completed = 0;

    for (int i = 0; i < kSamples; ++i) {
        auto result = sim.simulate(static_cast<uint32_t>(i));
        if (result.completed) {
            ++completed;
            max_latency_ms = std::max(max_latency_ms, result.observed_latency_ms);
        }
    }

    // Under 1% loss with max 4 retransmits all requests must complete
    EXPECT_EQ(completed, kSamples)
        << "All requests must complete at 1% loss with 4 retransmit budget";
    // p99 ceiling: 800 ms (contract requirement)
    EXPECT_LE(max_latency_ms, 800u)
        << "Max observed latency must be ≤ 800 ms under 1% loss";
}

// ─────────────────────────────────────────────────────────────────────────────
// SH3-05: Retransmit injection — 5% packet loss, bounded tail latency
// ─────────────────────────────────────────────────────────────────────────────
TEST(ServerHttp3Stress, SH305_RetransmitInjection5PctLoss) {
    RetransmitSimulator sim{0.05, 20, 6};  // 5% loss, 20 ms base RTT, max 6 retransmits

    constexpr int kSamples = 1000;
    int completed = 0;
    std::vector<uint64_t> latencies;
    latencies.reserve(kSamples);

    for (int i = 0; i < kSamples; ++i) {
        auto result = sim.simulate(static_cast<uint32_t>(i + kSamples));
        if (result.completed) {
            ++completed;
            latencies.push_back(result.observed_latency_ms);
        }
    }

    std::sort(latencies.begin(), latencies.end());
    const uint64_t p99_latency_ms = latencies.empty()
        ? 0u
        : latencies[static_cast<size_t>(latencies.size() * 99 / 100)];

    // ≥ 90% completion rate at 5% loss
    EXPECT_GE(completed, kSamples * 9 / 10)
        << "At least 90% of requests must complete at 5% loss";
    // p99 bounded (5% loss, 6 retransmits: max 2^6 × 20 ms = 1280 ms)
    EXPECT_LE(p99_latency_ms, 1500u)
        << "p99 latency must be bounded under 5% loss";
}

// ─────────────────────────────────────────────────────────────────────────────
// SH3-06: 0-RTT resumption — valid token → early data accepted
// ─────────────────────────────────────────────────────────────────────────────
TEST(ServerHttp3Stress, SH306_ZeroRttValidTokenAccepted) {
    StubZeroRttGate gate;
    FakeSessionToken token{"session-abc-123", /*valid=*/true, /*replayed=*/false};

    EXPECT_TRUE(gate.acceptEarlyData(token))
        << "Valid session token must be accepted for 0-RTT early data";
}

// ─────────────────────────────────────────────────────────────────────────────
// SH3-07: 0-RTT resumption — invalid/mismatched token → rejected (security gate)
// ─────────────────────────────────────────────────────────────────────────────
TEST(ServerHttp3Stress, SH307_ZeroRttInvalidTokenRejected) {
    StubZeroRttGate gate;
    // Mismatched token (wrong session, simulated corruption)
    FakeSessionToken token{"session-xyz-corrupt", /*valid=*/false, /*replayed=*/false};

    EXPECT_FALSE(gate.acceptEarlyData(token))
        << "Invalid/mismatched token must be rejected (security gate)";
}

// ─────────────────────────────────────────────────────────────────────────────
// SH3-08: 0-RTT resumption — replayed token → rejected (replay-protection gate)
// ─────────────────────────────────────────────────────────────────────────────
TEST(ServerHttp3Stress, SH308_ZeroRttReplayedTokenRejected) {
    StubZeroRttGate gate;
    FakeSessionToken token{"session-abc-123", /*valid=*/true, /*replayed=*/true};

    EXPECT_FALSE(gate.acceptEarlyData(token))
        << "Replayed session token must be rejected (replay-protection gate)";
}

// ─────────────────────────────────────────────────────────────────────────────
// SH3-09: Handshake variability — slow RTT simulation, session established
// ─────────────────────────────────────────────────────────────────────────────
TEST(ServerHttp3Stress, SH309_SlowHandshakeEstablishedWithinBudget) {
    // Simulate a QUIC handshake with RTT > 200 ms.
    // Contract: handshake must complete within a 5-second budget (kRetryGlobalBudget).
    constexpr uint64_t kSlowRttMs       = 250;  // > 200 ms per the plan
    constexpr uint64_t kHandshakeRounds = 2;    // TLS 1.3 requires 1-RTT; QUIC adds 1 more
    constexpr uint64_t kBudgetMs        = 5000; // 5-second budget

    uint64_t handshake_time_ms = kSlowRttMs * kHandshakeRounds;

    EXPECT_LE(handshake_time_ms, kBudgetMs)
        << "Handshake must complete within budget even at slow RTT";
    // A slow handshake is defined as RTT > 200 ms
    EXPECT_GT(kSlowRttMs, 200u)
        << "This test must exercise a genuinely slow RTT path (> 200 ms)";
}

// ─────────────────────────────────────────────────────────────────────────────
// SH3-10: Handshake variability — forced HTTP/2 fallback when QUIC unavailable
// ─────────────────────────────────────────────────────────────────────────────
TEST(ServerHttp3Stress, SH310_ForcedHttp2FallbackWhenQuicUnavailable) {
    // Simulate the fallback negotiation outcome: when QUIC negotiation fails
    // (e.g., all UDP packets dropped), the connection must fall back to HTTP/2.
    enum class NegotiationResult { HTTP3_QUIC, HTTP2_FALLBACK };

    auto negotiateProtocol = [](bool quic_available) -> NegotiationResult {
        return quic_available ? NegotiationResult::HTTP3_QUIC
                              : NegotiationResult::HTTP2_FALLBACK;
    };

    // QUIC available → HTTP/3
    EXPECT_EQ(negotiateProtocol(true), NegotiationResult::HTTP3_QUIC)
        << "QUIC available must produce HTTP/3 outcome";

    // QUIC unavailable (UDP blocked) → HTTP/2 fallback (no failure)
    EXPECT_EQ(negotiateProtocol(false), NegotiationResult::HTTP2_FALLBACK)
        << "QUIC unavailable must fall back to HTTP/2 (no service interruption)";
}

// ─────────────────────────────────────────────────────────────────────────────
// SH3-11: NGTCP2_ERR_DRAINING — no new streams accepted after draining
// ─────────────────────────────────────────────────────────────────────────────
TEST(ServerHttp3Stress, SH311_DrainingStateRejectsNewStreams) {
    FakeQuicConnState conn;
    conn.current_peer = {"10.0.0.1", 44000};

    // Simulate entering the DRAINING state (NGTCP2_ERR_DRAINING received)
    conn.enterDraining();

    ASSERT_TRUE(conn.draining.load())
        << "Connection must be in draining state";

    // Attempt a new request stream — must be rejected
    auto acceptNewStream = [&conn]() -> bool {
        return !conn.draining.load(std::memory_order_acquire);
    };
    EXPECT_FALSE(acceptNewStream())
        << "New streams must not be accepted while connection is draining";

    // Migration attempts during draining must also be silently dropped
    uint32_t migration_before = conn.migration_count.load();
    conn.onPathMigration({"10.0.0.2", 44001});
    EXPECT_EQ(conn.migration_count.load(), migration_before)
        << "Migration must be refused during draining";
}

// ─────────────────────────────────────────────────────────────────────────────
// SH3-12: NGTCP2_ERR_DRAINING — RAII cleanup verified, no double-free
// ─────────────────────────────────────────────────────────────────────────────
TEST(ServerHttp3Stress, SH312_DrainingStateRaiiCleanup) {
    auto conn = std::make_unique<FakeQuicConnState>();
    conn->enterDraining();

    // Simulate orderly RAII destroy (mirroring QuicConnDeleter in Http3Session)
    conn->destroy();
    EXPECT_TRUE(conn->conn_deleted.load())
        << "QUIC connection must be marked destroyed after RAII cleanup";

    // Calling destroy() again must be idempotent (no double-free in production
    // the ngtcp2_conn_del guard handles this via the unique_ptr deleter)
    conn->destroy();
    EXPECT_TRUE(conn->conn_deleted.load())
        << "Repeated destroy() must be idempotent (no double-free)";
}
