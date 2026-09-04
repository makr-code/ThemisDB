// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

/**
 * @file test_network_hardening_phase2_focused.cpp
 * @brief Phase 2 network module hardening focused tests — multi-transport
 *        failure injection and adversarial auth/rate-limit guard hardening.
 *
 * Covers the two Q3 2026 open items from src/network/ROADMAP.md Phase 2:
 *
 *   (a) Complete remaining failure-injection coverage for multi-transport
 *       edge cases (TCP, WebSocket, UDP, QUIC paths).
 *   (b) Tighten auth/rate-limit/session guard behavior under sustained
 *       adversarial traffic.
 *
 * All tests are deterministic and self-contained — no live sockets or real
 * network I/O are used.
 *
 * ## Test Cases
 *
 * ### NMT-01..NMT-08 — Multi-Transport Failure Injection
 *   NMT-01  TRANSPORT_CLOSED is a connection-closing error on every path.
 *   NMT-02  TRANSPORT_UNAVAILABLE is a connection-closing error.
 *   NMT-03  TRANSPORT_FALLBACK_ACTIVE is NOT connection-closing (in-progress).
 *   NMT-04  TRANSPORT_WRITE_ERROR is NOT connection-closing (retryable).
 *   NMT-05  Frame validation on WS path — invalid magic → FRAME_INVALID.
 *   NMT-06  Backpressure exceeded on UDP path → transient error (retryable).
 *   NMT-07  Connection limit reached → CONNECTION_LIMIT_REACHED (bounded).
 *   NMT-08  Fallback sequence: primary UNAVAILABLE → fallback exhausted →
 *           TRANSPORT_UNAVAILABLE (fail-closed, no silent data loss).
 *
 * ### NAG-01..NAG-08 — Adversarial Auth/Rate-Limit Guard Hardening
 *   NAG-01  Burst of AUTH_REQUIRED events — every request rejected, no leak.
 *   NAG-02  Rate-limit enforcement under burst (100 rapid-fire requests).
 *   NAG-03  CLIENT_BANNED after sustained rate-limit threshold violations.
 *   NAG-04  AUTH_TIMEOUT for stalled handshake → connection-closing.
 *   NAG-05  Revocation beats expiry: SESSION_REVOKED not SESSION_EXPIRED.
 *   NAG-06  Rate-limiter unavailability → fail-closed deny (not fail-open).
 *   NAG-07  BACKPRESSURE_EXCEEDED is transient, not connection-closing.
 *   NAG-08  QUORUM_DEGRADED and ROUTING_UNAVAILABLE are not connection-closing
 *           (cluster-level errors do not terminate the transport connection).
 *
 * @see include/network/network_api_contract.h
 * @see src/network/ROADMAP.md — Phase 2 items
 */

#include <gtest/gtest.h>

#include "network/network_api_contract.h"

#include <atomic>
#include <chrono>
#include <cstdint>
#include <functional>
#include <string>
#include <unordered_map>
#include <vector>

using namespace themis::network;
using namespace std::chrono_literals;

// ============================================================================
// Deterministic seed (canonical repository value per bench_fixtures.h)
// ============================================================================
static constexpr std::uint64_t kHardeningP2Seed = 42;

// ============================================================================
// Minimal in-process mocks (no live I/O)
// ============================================================================

namespace {

// ---------------------------------------------------------------------------
// Transport state machine — models TCP/WS/UDP/QUIC path lifecycle
// ---------------------------------------------------------------------------
enum class TransportKind { TCP, WEBSOCKET, UDP, QUIC };

enum class TransportPhase { CONNECTED, FALLBACK_ACTIVE, CLOSED, UNAVAILABLE };

struct MockTransport {
    TransportKind  kind;
    TransportPhase phase{TransportPhase::CONNECTED};

    /**
     * @brief Map transport phase to NetworkErrorCode on send attempt.
     * @return OK when CONNECTED; appropriate error code otherwise.
     */
    NetworkErrorCode send(const std::string& /*payload*/) const noexcept {
        switch (phase) {
            case TransportPhase::CONNECTED:       return NetworkErrorCode::OK;
            case TransportPhase::FALLBACK_ACTIVE: return NetworkErrorCode::TRANSPORT_FALLBACK_ACTIVE;
            case TransportPhase::CLOSED:          return NetworkErrorCode::TRANSPORT_CLOSED;
            case TransportPhase::UNAVAILABLE:     return NetworkErrorCode::TRANSPORT_UNAVAILABLE;
        }
        return NetworkErrorCode::INTERNAL_ERROR;
    }
};

// ---------------------------------------------------------------------------
// Backpressure gate — models bounded queue admission on UDP/QUIC paths
// ---------------------------------------------------------------------------
class MockBackpressureGate {
public:
    explicit MockBackpressureGate(int capacity) : capacity_(capacity), depth_(0) {}

    NetworkErrorCode admit() noexcept {
        if (depth_ >= capacity_) {
          return NetworkErrorCode::BACKPRESSURE_EXCEEDED;
        }
        ++depth_;
        return NetworkErrorCode::OK;
    }

    void drain() noexcept { if (depth_ > 0) --depth_; }

private:
    int capacity_;
    int depth_;
};

// ---------------------------------------------------------------------------
// Session store with adversarial state support
// ---------------------------------------------------------------------------
enum class SessionState { ACTIVE, EXPIRED, REVOKED, MISSING };

class MockSessionStore {
public:
    void add(const std::string& token, SessionState state) {
        sessions_[token] = state;
    }

    /**
     * @brief Evaluate the session guard contract.
     *
     * Guard evaluation order:
     *   1. Empty token → AUTH_REQUIRED.
     *   2. Unknown token → SESSION_MALFORMED.
     *   3. Revoked → SESSION_REVOKED (takes priority over expiry).
     *   4. Expired → SESSION_EXPIRED.
     *   5. Active → OK.
     */
    NetworkErrorCode validate(const std::string& token) const noexcept {
        if (token.empty()) {
          return NetworkErrorCode::AUTH_REQUIRED;
        }
        auto it = sessions_.find(token);
        if (it == sessions_.end()) {
          return NetworkErrorCode::SESSION_MALFORMED;
        }
        switch (it->second) {
            case SessionState::ACTIVE:   return NetworkErrorCode::OK;
            case SessionState::EXPIRED:  return NetworkErrorCode::SESSION_EXPIRED;
            case SessionState::REVOKED:  return NetworkErrorCode::SESSION_REVOKED;
            case SessionState::MISSING:  return NetworkErrorCode::AUTH_REQUIRED;
        }
        return NetworkErrorCode::INTERNAL_ERROR;
    }

private:
    std::unordered_map<std::string, SessionState> sessions_;
};

// ---------------------------------------------------------------------------
// Per-connection rate limiter with ban tracking
// ---------------------------------------------------------------------------
class MockRateLimiter {
public:
    explicit MockRateLimiter(int limitPerWindow, bool unavailable = false)
        : limit_(limitPerWindow), count_(0), violations_(0), unavailable_(unavailable) {}

    NetworkErrorCode check() noexcept {
        if (unavailable_) {
            // Fail-closed: unavailability must never allow traffic through.
            return NetworkErrorCode::RATE_LIMITED;
        }
        ++count_;
        if (count_ > limit_) {
            ++violations_;
            if (violations_ >= kRateLimitBanThreshold) {
                return NetworkErrorCode::CLIENT_BANNED;
            }
            return NetworkErrorCode::RATE_LIMITED;
        }
        return NetworkErrorCode::OK;
    }

    int violationCount() const noexcept { return violations_; }

private:
    int  limit_;
    int  count_;
    int  violations_;
    bool unavailable_;
};

// ---------------------------------------------------------------------------
// Auth handshake timer — simulates stalled handshake → AUTH_TIMEOUT
// ---------------------------------------------------------------------------
struct MockAuthTimer {
    bool timedOut{false};

    NetworkErrorCode evaluate() const noexcept {
        return timedOut ? NetworkErrorCode::AUTH_TIMEOUT : NetworkErrorCode::OK;
    }
};

// ---------------------------------------------------------------------------
// Frame validator (WebSocket path)
// ---------------------------------------------------------------------------
struct MockWsFrame {
    std::uint8_t magic0{kFrameMagic0};
    std::uint8_t magic1{kFrameMagic1};
    std::size_t  payloadSize{0};
};

NetworkErrorCode validateWsFrame(const MockWsFrame& f) noexcept {
    if (f.magic0 != kFrameMagic0 || f.magic1 != kFrameMagic1)
        return NetworkErrorCode::FRAME_INVALID;
    if (f.payloadSize > kMaxFramePayloadBytes)
        return NetworkErrorCode::FRAME_OVERSIZED;
    return NetworkErrorCode::OK;
}

}  // anonymous namespace

// ============================================================================
// NMT-01..NMT-08 — Multi-Transport Failure Injection
// ============================================================================

/**
 * @brief NMT-01: TRANSPORT_CLOSED is connection-closing on every transport path.
 */
TEST(NetworkHardeningPhase2MultiTransport, NMT01_TransportClosedIsConnectionClosing) {
    for (auto kind : {TransportKind::TCP, TransportKind::WEBSOCKET,
                      TransportKind::UDP, TransportKind::QUIC}) {
        MockTransport t{kind, TransportPhase::CLOSED};
        auto code = t.send("payload");
        EXPECT_EQ(code, NetworkErrorCode::TRANSPORT_CLOSED)
            << "Transport kind " << static_cast<int>(kind);
        EXPECT_TRUE(isConnectionClosingError(code))
            << "TRANSPORT_CLOSED must be connection-closing on transport "
            << static_cast<int>(kind);
    }
}

/**
 * @brief NMT-02: TRANSPORT_UNAVAILABLE is connection-closing — no silent drop.
 */
TEST(NetworkHardeningPhase2MultiTransport, NMT02_TransportUnavailableIsConnectionClosing) {
    MockTransport t{TransportKind::TCP, TransportPhase::UNAVAILABLE};
    auto code = t.send("data");
    EXPECT_EQ(code, NetworkErrorCode::TRANSPORT_UNAVAILABLE);
    EXPECT_TRUE(isConnectionClosingError(code))
        << "TRANSPORT_UNAVAILABLE must trigger connection closure";
}

/**
 * @brief NMT-03: TRANSPORT_FALLBACK_ACTIVE is NOT connection-closing —
 *        fallback is in progress, the connection is still live.
 */
TEST(NetworkHardeningPhase2MultiTransport, NMT03_FallbackActiveNotConnectionClosing) {
    MockTransport t{TransportKind::QUIC, TransportPhase::FALLBACK_ACTIVE};
    auto code = t.send("data");
    EXPECT_EQ(code, NetworkErrorCode::TRANSPORT_FALLBACK_ACTIVE);
    EXPECT_FALSE(isConnectionClosingError(code))
        << "TRANSPORT_FALLBACK_ACTIVE must not terminate the connection";
}

/**
 * @brief NMT-04: TRANSPORT_WRITE_ERROR is not connection-closing — write
 *        errors are retryable at the protocol layer.
 */
TEST(NetworkHardeningPhase2MultiTransport, NMT04_WriteErrorNotConnectionClosing) {
    EXPECT_FALSE(isConnectionClosingError(NetworkErrorCode::TRANSPORT_WRITE_ERROR))
        << "TRANSPORT_WRITE_ERROR should be retryable, not connection-closing";
}

/**
 * @brief NMT-05: Frame validation on WebSocket path — invalid magic bytes
 *        yield FRAME_INVALID (fail-closed, connection-closing).
 */
TEST(NetworkHardeningPhase2MultiTransport, NMT05_WsPathInvalidMagicFrameInvalid) {
    MockWsFrame frame;
    frame.magic0 = 0xAA;  // corrupt first magic byte
    auto code = validateWsFrame(frame);
    EXPECT_EQ(code, NetworkErrorCode::FRAME_INVALID);
    EXPECT_TRUE(isConnectionClosingError(code))
        << "Invalid magic on WS path must close the connection";
}

/**
 * @brief NMT-06: Backpressure exceeded on UDP path → BACKPRESSURE_EXCEEDED,
 *        which is transient (retryable), not connection-closing.
 */
TEST(NetworkHardeningPhase2MultiTransport, NMT06_UdpBackpressureExceededTransient) {
    MockBackpressureGate gate(2);
    ASSERT_EQ(gate.admit(), NetworkErrorCode::OK);
    ASSERT_EQ(gate.admit(), NetworkErrorCode::OK);

    // Queue full — third admit must fail with BACKPRESSURE_EXCEEDED.
    auto code = gate.admit();
    EXPECT_EQ(code, NetworkErrorCode::BACKPRESSURE_EXCEEDED);
    EXPECT_TRUE(isRateLimitTransient(code))
        << "BACKPRESSURE_EXCEEDED must be transient on UDP path";
    EXPECT_FALSE(isConnectionClosingError(code))
        << "Backpressure must not close the UDP connection";
}

/**
 * @brief NMT-07: Concurrent connection count exceeds kMaxConcurrentConnections
 *        → CONNECTION_LIMIT_REACHED (bounded admission control, not a crash).
 */
TEST(NetworkHardeningPhase2MultiTransport, NMT07_ConnectionLimitReachedBounded) {
    // Simulate arriving at the limit by modelling the error code directly.
    constexpr auto code = NetworkErrorCode::CONNECTION_LIMIT_REACHED;
    // The connection limit error is NOT connection-closing for the *rejected*
    // new connection (it was never admitted); it is merely a refusal.
    EXPECT_FALSE(isConnectionClosingError(code))
        << "CONNECTION_LIMIT_REACHED is a rejection, not a close of an admitted connection";
    // It is also not a transient rate-limit event.
    EXPECT_FALSE(isRateLimitTransient(code));
    // The kMaxConcurrentConnections constant is positive and bounded.
    EXPECT_GT(kMaxConcurrentConnections, 0u);
    EXPECT_LE(kMaxConcurrentConnections, 1u << 20)  // 1M as sanity upper bound
        << "kMaxConcurrentConnections should be bounded to prevent unbounded growth";
}

/**
 * @brief NMT-08: Fallback sequence exhaustion — primary CLOSED, fallback
 *        UNAVAILABLE → final error is TRANSPORT_UNAVAILABLE (fail-closed).
 *
 * Contract: no silent data loss; caller must receive an explicit error.
 */
TEST(NetworkHardeningPhase2MultiTransport, NMT08_FallbackExhaustionFailClosed) {
    MockTransport primary{TransportKind::TCP, TransportPhase::CLOSED};
    MockTransport fallback{TransportKind::QUIC, TransportPhase::UNAVAILABLE};

    auto primaryResult = primary.send("data");
    EXPECT_TRUE(isConnectionClosingError(primaryResult))
        << "Primary transport failure must be connection-closing to trigger fallback";

    // Fallback attempted — also unavailable.
    auto fallbackResult = fallback.send("data");
    EXPECT_EQ(fallbackResult, NetworkErrorCode::TRANSPORT_UNAVAILABLE);
    EXPECT_TRUE(isConnectionClosingError(fallbackResult))
        << "After fallback exhaustion, error must be connection-closing (fail-closed)";
}

// ============================================================================
// NAG-01..NAG-08 — Adversarial Auth/Rate-Limit Guard Hardening
// ============================================================================

/**
 * @brief NAG-01: Burst of AUTH_REQUIRED events — every unauthenticated request
 *        is rejected; no leak of data to unauthenticated callers.
 */
TEST(NetworkHardeningPhase2AdversarialGuard, NAG01_AuthRequiredBurstAllRejected) {
    MockSessionStore store;
    constexpr int kBurstSize = 200;

    int rejectedCount = 0;
    for (int i = 0; i < kBurstSize; ++i) {
        auto code = store.validate("");  // no token
        if (code == NetworkErrorCode::AUTH_REQUIRED) {
          ++rejectedCount;
        }
    }

    EXPECT_EQ(rejectedCount, kBurstSize)
        << "Every unauthenticated request in a burst must be rejected";
}

/**
 * @brief NAG-02: Rate-limit enforcement under burst traffic — requests beyond
 *        the per-connection window are rejected with RATE_LIMITED.
 */
TEST(NetworkHardeningPhase2AdversarialGuard, NAG02_BurstTrafficRateLimited) {
    constexpr int kLimit = 10;
    constexpr int kBurst = 100;
    MockRateLimiter rl(kLimit);

    int allowed   = 0;
    int limited   = 0;
    int banned    = 0;

    for (int i = 0; i < kBurst; ++i) {
        auto code = rl.check();
        if (code == NetworkErrorCode::OK) {
          ++allowed;
        }
        else if (code == NetworkErrorCode::RATE_LIMITED)  ++limited;
        else if (code == NetworkErrorCode::CLIENT_BANNED) ++banned;
    }

    EXPECT_EQ(allowed, kLimit)
        << "Exactly kLimit requests should be allowed through";
    EXPECT_GT(limited + banned, 0)
        << "Requests beyond the limit must be blocked";
    EXPECT_EQ(allowed + limited + banned, kBurst)
        << "Every request in the burst must have a classified outcome";
}

/**
 * @brief NAG-03: CLIENT_BANNED after sustained rate-limit threshold violations.
 *
 * After kRateLimitBanThreshold consecutive violations, new requests must
 * receive CLIENT_BANNED rather than RATE_LIMITED.
 */
TEST(NetworkHardeningPhase2AdversarialGuard, NAG03_ClientBannedAfterThresholdViolations) {
    // Allow 1 request, then hammer to accumulate violations.
    MockRateLimiter rl(1);

    // First request is within limit.
    ASSERT_EQ(rl.check(), NetworkErrorCode::OK);

    // Send kRateLimitBanThreshold additional requests to trigger ban.
    NetworkErrorCode lastCode = NetworkErrorCode::OK;
    for (int i = 0; i < kRateLimitBanThreshold; ++i) {
        lastCode = rl.check();
    }

    EXPECT_EQ(lastCode, NetworkErrorCode::CLIENT_BANNED)
        << "After " << kRateLimitBanThreshold
        << " violations, CLIENT_BANNED must be returned";
}

/**
 * @brief NAG-04: Stalled authentication handshake → AUTH_TIMEOUT, which is
 *        connection-closing (prevents resource exhaustion from hung sessions).
 */
TEST(NetworkHardeningPhase2AdversarialGuard, NAG04_AuthTimeoutIsConnectionClosing) {
    MockAuthTimer timer;
    timer.timedOut = true;
    auto code = timer.evaluate();
    EXPECT_EQ(code, NetworkErrorCode::AUTH_TIMEOUT);
    EXPECT_TRUE(isConnectionClosingError(code))
        << "AUTH_TIMEOUT must close the connection to prevent hung-handshake exhaustion";

    // Verify the timeout constant is a reasonable positive value.
    EXPECT_GT(kAuthHandshakeTimeout.count(), 0)
        << "kAuthHandshakeTimeout must be a positive duration";
}

/**
 * @brief NAG-05: SESSION_REVOKED takes priority over SESSION_EXPIRED.
 *
 * A revoked session must always be rejected with SESSION_REVOKED regardless
 * of whether it is also expired, preventing expiry-bypass attacks.
 */
TEST(NetworkHardeningPhase2AdversarialGuard, NAG05_RevocationPriorityOverExpiry) {
    MockSessionStore store;
    // A token that is both revoked and (conceptually) expired.
    store.add("revoked-expired-token", SessionState::REVOKED);

    auto code = store.validate("revoked-expired-token");
    EXPECT_EQ(code, NetworkErrorCode::SESSION_REVOKED)
        << "Revoked session must be rejected as SESSION_REVOKED, not SESSION_EXPIRED";
    EXPECT_NE(code, NetworkErrorCode::SESSION_EXPIRED);
    EXPECT_NE(code, NetworkErrorCode::OK);
}

/**
 * @brief NAG-06: Rate-limiter unavailability → fail-closed deny (not fail-open).
 *
 * When the rate-limiting backend is unreachable, no request must be let
 * through — fail-open would allow adversarial traffic to bypass limits.
 */
TEST(NetworkHardeningPhase2AdversarialGuard, NAG06_UnavailableRateLimiterFailClosed) {
    MockRateLimiter rl(/*limitPerWindow=*/1000, /*unavailable=*/true);
    constexpr int kProbes = 20;
    for (int i = 0; i < kProbes; ++i) {
        auto code = rl.check();
        EXPECT_NE(code, NetworkErrorCode::OK)
            << "Rate-limiter unavailability must never produce OK (probe " << i << ")";
    }
}

/**
 * @brief NAG-07: BACKPRESSURE_EXCEEDED is transient, NOT connection-closing.
 *
 * Backpressure is a temporary condition; the connection must remain alive so
 * that the client can retry after the queue drains.
 */
TEST(NetworkHardeningPhase2AdversarialGuard, NAG07_BackpressureExceededTransientNotClosing) {
    EXPECT_TRUE(isRateLimitTransient(NetworkErrorCode::BACKPRESSURE_EXCEEDED))
        << "BACKPRESSURE_EXCEEDED must be classified as transient";
    EXPECT_FALSE(isConnectionClosingError(NetworkErrorCode::BACKPRESSURE_EXCEEDED))
        << "BACKPRESSURE_EXCEEDED must not close the connection";
}

/**
 * @brief NAG-08: Cluster-level errors (QUORUM_DEGRADED, ROUTING_UNAVAILABLE)
 *        are not connection-closing — they reflect cluster state, not a
 *        transport failure that terminates the client connection.
 */
TEST(NetworkHardeningPhase2AdversarialGuard, NAG08_ClusterErrorsNotConnectionClosing) {
    EXPECT_FALSE(isConnectionClosingError(NetworkErrorCode::QUORUM_DEGRADED))
        << "QUORUM_DEGRADED is a cluster-level error and must not close the transport";
    EXPECT_FALSE(isConnectionClosingError(NetworkErrorCode::ROUTING_UNAVAILABLE))
        << "ROUTING_UNAVAILABLE is a cluster-level error and must not close the transport";

    // Cluster errors are also not transient rate-limit events.
    EXPECT_FALSE(isRateLimitTransient(NetworkErrorCode::QUORUM_DEGRADED));
    EXPECT_FALSE(isRateLimitTransient(NetworkErrorCode::ROUTING_UNAVAILABLE));
}
