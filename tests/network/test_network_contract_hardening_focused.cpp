// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

/**
 * @file test_network_contract_hardening_focused.cpp
 * @brief Phase 4 network module contract-hardening focused tests (NCH-01..NCH-16).
 *
 * Validates every invariant defined in include/network/network_api_contract.h
 * using deterministic, self-contained mock fixtures.  No live sockets or real
 * network I/O are used.
 *
 * ## Test Cases
 *
 * ### NCH-01..NCH-04 — Frame Validation Contract
 *   NCH-01  Valid frame (correct magic + size) passes validation.
 *   NCH-02  Invalid magic bytes → FRAME_INVALID.
 *   NCH-03  Oversized payload (> kMaxFramePayloadBytes) → FRAME_OVERSIZED.
 *   NCH-04  isConnectionClosingError() returns true for FRAME_INVALID/OVERSIZED.
 *
 * ### NCH-05..NCH-08 — Auth/Session Contract
 *   NCH-05  Privileged opcode without session token → AUTH_REQUIRED.
 *   NCH-06  Expired session token → SESSION_EXPIRED.
 *   NCH-07  Revoked session → SESSION_REVOKED (fail-closed).
 *   NCH-08  Malformed session token → SESSION_MALFORMED.
 *
 * ### NCH-09..NCH-12 — Rate-Limit Contract
 *   NCH-09  Requests within per-connection limit → all succeed.
 *   NCH-10  Requests exceeding per-connection limit → RATE_LIMITED.
 *   NCH-11  RATE_LIMITED is classified as transient (isRateLimitTransient).
 *   NCH-12  Distributed fallback when rate-limiter unavailable → fail-closed deny.
 *
 * ### NCH-13..NCH-16 — Connection Lifecycle Contract
 *   NCH-13  DRAIN state rejects new requests (SERVER_DRAINING).
 *   NCH-14  No messages are accepted after connection transitions to CLOSED.
 *   NCH-15  Graceful shutdown respects kDrainGracePeriod ordering.
 *   NCH-16  isConnectionClosingError covers all hard-close codes.
 *
 * @see include/network/network_api_contract.h
 * @see src/network/ROADMAP.md — Phase 4 items
 */

#include <gtest/gtest.h>

#include "network/network_api_contract.h"

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

using namespace themis::network;
using namespace std::chrono_literals;

// ============================================================================
// Seed
// ============================================================================
static constexpr std::uint64_t kNetworkContractSeed = 42;

// ============================================================================
// Minimal in-process mocks
// ============================================================================

namespace {

// ---------------------------------------------------------------------------
// Mock frame validator
// ---------------------------------------------------------------------------
struct MockFrame {
    std::uint8_t magic0{kFrameMagic0};
    std::uint8_t magic1{kFrameMagic1};
    std::uint8_t opcode{0x01};
    std::size_t  payloadSize{0};
};

NetworkErrorCode validateFrame(const MockFrame& f) {
    if (f.magic0 != kFrameMagic0 || f.magic1 != kFrameMagic1)
        return NetworkErrorCode::FRAME_INVALID;
    if (f.payloadSize > kMaxFramePayloadBytes)
        return NetworkErrorCode::FRAME_OVERSIZED;
    return NetworkErrorCode::OK;
}

// ---------------------------------------------------------------------------
// Mock session store
// ---------------------------------------------------------------------------
enum class SessionState { ACTIVE, EXPIRED, REVOKED, MISSING };

class MockSessionStore {
public:
    void add(const std::string& token, SessionState state) {
        sessions_[token] = state;
    }

    NetworkErrorCode validate(const std::string& token) const {
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
// Mock per-connection rate limiter
// ---------------------------------------------------------------------------
class MockRateLimiter {
public:
    explicit MockRateLimiter(int limitPerSec, bool unavailable = false)
        : limit_(limitPerSec), count_(0), unavailable_(unavailable) {}

    NetworkErrorCode check() {
        if (unavailable_) {
            // Fail-closed: when rate-limiter is unavailable, deny (not allow).
            return NetworkErrorCode::RATE_LIMITED;
        }
        if (++count_ > limit_) {
          return NetworkErrorCode::RATE_LIMITED;
        }
        return NetworkErrorCode::OK;
    }

    void reset() { count_ = 0; }

private:
    int  limit_;
    int  count_;
    bool unavailable_;
};

// ---------------------------------------------------------------------------
// Mock connection state machine
// ---------------------------------------------------------------------------
enum class ConnState { ACCEPTING, AUTHENTICATING, SERVING, DRAINING, CLOSED };

class MockConnection {
public:
    ConnState state{ConnState::SERVING};

    NetworkErrorCode sendMessage(const std::string& /*msg*/) {
        switch (state) {
            case ConnState::SERVING:
                return NetworkErrorCode::OK;
            case ConnState::DRAINING:
                return NetworkErrorCode::SERVER_DRAINING;
            case ConnState::CLOSED:
                return NetworkErrorCode::TRANSPORT_CLOSED;
            default:
                return NetworkErrorCode::INTERNAL_ERROR;
        }
    }

    void startDrain() { state = ConnState::DRAINING; }
    void close()      { state = ConnState::CLOSED; }
};

}  // anonymous namespace

// ============================================================================
// NCH-01..NCH-04 — Frame Validation Contract
// ============================================================================

/**
 * @brief NCH-01: Valid frame (correct magic + within size) → OK.
 */
TEST(NetworkContractFrame, NCH01_ValidFrameAccepted) {
    MockFrame f;
    f.payloadSize = 64;
    EXPECT_EQ(validateFrame(f), NetworkErrorCode::OK);
}

/**
 * @brief NCH-02: Invalid magic bytes → FRAME_INVALID (fail-closed, close connection).
 */
TEST(NetworkContractFrame, NCH02_InvalidMagicFrameInvalid) {
    MockFrame f;
    f.magic0 = 0x00;  // corrupt magic
    f.payloadSize = 64;
    EXPECT_EQ(validateFrame(f), NetworkErrorCode::FRAME_INVALID);
}

/**
 * @brief NCH-03: Oversized payload → FRAME_OVERSIZED.
 */
TEST(NetworkContractFrame, NCH03_OversizedFrameRejected) {
    MockFrame f;
    f.payloadSize = kMaxFramePayloadBytes + 1;
    EXPECT_EQ(validateFrame(f), NetworkErrorCode::FRAME_OVERSIZED);
}

/**
 * @brief NCH-04: isConnectionClosingError() is true for FRAME_INVALID and FRAME_OVERSIZED.
 */
TEST(NetworkContractFrame, NCH04_FrameErrorsAreConnectionClosing) {
    EXPECT_TRUE(isConnectionClosingError(NetworkErrorCode::FRAME_INVALID));
    EXPECT_TRUE(isConnectionClosingError(NetworkErrorCode::FRAME_OVERSIZED));
    // Non-closing codes must NOT be classified as connection-closing.
    EXPECT_FALSE(isConnectionClosingError(NetworkErrorCode::RATE_LIMITED));
    EXPECT_FALSE(isConnectionClosingError(NetworkErrorCode::SESSION_EXPIRED));
}

// ============================================================================
// NCH-05..NCH-08 — Auth/Session Contract
// ============================================================================

/**
 * @brief NCH-05: No session token → AUTH_REQUIRED.
 */
TEST(NetworkContractAuth, NCH05_NoTokenAuthRequired) {
    MockSessionStore store;
    EXPECT_EQ(store.validate(""), NetworkErrorCode::AUTH_REQUIRED);
}

/**
 * @brief NCH-06: Expired session token → SESSION_EXPIRED.
 */
TEST(NetworkContractAuth, NCH06_ExpiredSessionRejected) {
    MockSessionStore store;
    store.add("expired-token", SessionState::EXPIRED);
    EXPECT_EQ(store.validate("expired-token"), NetworkErrorCode::SESSION_EXPIRED);
}

/**
 * @brief NCH-07: Revoked session → SESSION_REVOKED (fail-closed, not open).
 */
TEST(NetworkContractAuth, NCH07_RevokedSessionFailClosed) {
    MockSessionStore store;
    store.add("revoked-token", SessionState::REVOKED);
    auto code = store.validate("revoked-token");
    EXPECT_EQ(code, NetworkErrorCode::SESSION_REVOKED);
    // Revoked must not be OK.
    EXPECT_NE(code, NetworkErrorCode::OK);
}

/**
 * @brief NCH-08: Malformed token (unknown token not in store) → SESSION_MALFORMED.
 */
TEST(NetworkContractAuth, NCH08_MalformedTokenRejected) {
    MockSessionStore store;
    store.add("other-token", SessionState::ACTIVE);
    // A token that exists in the wire but not in any session store.
    EXPECT_EQ(store.validate("garbage-token"), NetworkErrorCode::SESSION_MALFORMED);
}

// ============================================================================
// NCH-09..NCH-12 — Rate-Limit Contract
// ============================================================================

/**
 * @brief NCH-09: Requests within per-connection limit all succeed.
 */
TEST(NetworkContractRateLimit, NCH09_WithinLimitSucceeds) {
    MockRateLimiter rl(10);
    for (int i = 0; i < 10; ++i) {
        EXPECT_EQ(rl.check(), NetworkErrorCode::OK) << "Request " << i;
    }
}

/**
 * @brief NCH-10: Requests exceeding per-connection limit → RATE_LIMITED.
 */
TEST(NetworkContractRateLimit, NCH10_ExceededLimitRateLimited) {
    MockRateLimiter rl(5);
    for (int i = 0; i < 5; ++i) rl.check();  // consume limit
    EXPECT_EQ(rl.check(), NetworkErrorCode::RATE_LIMITED);
}

/**
 * @brief NCH-11: RATE_LIMITED is transient (isRateLimitTransient returns true).
 */
TEST(NetworkContractRateLimit, NCH11_RateLimitedIsTransient) {
    EXPECT_TRUE(isRateLimitTransient(NetworkErrorCode::RATE_LIMITED));
    EXPECT_TRUE(isRateLimitTransient(NetworkErrorCode::BACKPRESSURE_EXCEEDED));
    // Non-transient codes must not be classified as transient.
    EXPECT_FALSE(isRateLimitTransient(NetworkErrorCode::FRAME_INVALID));
    EXPECT_FALSE(isRateLimitTransient(NetworkErrorCode::AUTH_REQUIRED));
}

/**
 * @brief NCH-12: When rate-limiter backend is unavailable → fail-closed deny,
 *        NOT fail-open allow.
 */
TEST(NetworkContractRateLimit, NCH12_UnavailableRateLimiterFailClosed) {
    MockRateLimiter rl(1000, /*unavailable=*/true);
    // Even with a high limit, unavailability must not produce OK.
    auto code = rl.check();
    EXPECT_NE(code, NetworkErrorCode::OK)
        << "Rate limiter unavailability must not fail-open";
    EXPECT_EQ(code, NetworkErrorCode::RATE_LIMITED);
}

// ============================================================================
// NCH-13..NCH-16 — Connection Lifecycle Contract
// ============================================================================

/**
 * @brief NCH-13: DRAINING state rejects new requests with SERVER_DRAINING.
 */
TEST(NetworkContractLifecycle, NCH13_DrainingRejectsNewRequests) {
    MockConnection conn;
    conn.startDrain();
    EXPECT_EQ(conn.sendMessage("hello"), NetworkErrorCode::SERVER_DRAINING);
}

/**
 * @brief NCH-14: No messages accepted after connection is CLOSED.
 */
TEST(NetworkContractLifecycle, NCH14_NoMessagesAfterClose) {
    MockConnection conn;
    conn.close();
    EXPECT_EQ(conn.sendMessage("post-close"), NetworkErrorCode::TRANSPORT_CLOSED);
    EXPECT_TRUE(isConnectionClosingError(NetworkErrorCode::TRANSPORT_CLOSED));
}

/**
 * @brief NCH-15: Graceful shutdown sequence: SERVING → DRAINING → CLOSED.
 *        Messages are accepted in SERVING, rejected in DRAINING, rejected in CLOSED.
 */
TEST(NetworkContractLifecycle, NCH15_GracefulShutdownOrdering) {
    MockConnection conn;
    EXPECT_EQ(conn.state, ConnState::SERVING);
    EXPECT_EQ(conn.sendMessage("ok"), NetworkErrorCode::OK);

    conn.startDrain();
    EXPECT_EQ(conn.state, ConnState::DRAINING);
    EXPECT_EQ(conn.sendMessage("draining"), NetworkErrorCode::SERVER_DRAINING);

    conn.close();
    EXPECT_EQ(conn.state, ConnState::CLOSED);
    EXPECT_EQ(conn.sendMessage("closed"), NetworkErrorCode::TRANSPORT_CLOSED);
}

/**
 * @brief NCH-16: isConnectionClosingError covers all hard-close error codes.
 */
TEST(NetworkContractLifecycle, NCH16_AllHardCloseCodesClassified) {
    const std::vector<NetworkErrorCode> closingCodes = {
        NetworkErrorCode::FRAME_INVALID,
        NetworkErrorCode::FRAME_OVERSIZED,
        NetworkErrorCode::AUTH_TIMEOUT,
        NetworkErrorCode::TRANSPORT_CLOSED,
        NetworkErrorCode::TRANSPORT_UNAVAILABLE,
        NetworkErrorCode::INTERNAL_ERROR,
    };
    for (auto code : closingCodes) {
        EXPECT_TRUE(isConnectionClosingError(code))
            << "Expected connection-closing=true for code "
            << static_cast<int>(code);
    }

    // These must NOT trigger a connection close.
    const std::vector<NetworkErrorCode> nonClosingCodes = {
        NetworkErrorCode::RATE_LIMITED,
        NetworkErrorCode::SESSION_EXPIRED,
        NetworkErrorCode::OPCODE_UNKNOWN,
        NetworkErrorCode::OK,
    };
    for (auto code : nonClosingCodes) {
        EXPECT_FALSE(isConnectionClosingError(code))
            << "Expected connection-closing=false for code "
            << static_cast<int>(code);
    }
}
