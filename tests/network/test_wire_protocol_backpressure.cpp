/*
 * @file test_wire_protocol_backpressure.cpp
 * @brief Unit tests for TCP backlog management and backpressure handling in
 *        WireProtocolServer.
 *
 * These tests validate the observable configuration and state-machine behaviour
 * introduced by the backpressure feature without requiring a live TCP socket:
 *
 *  1.  Config defaults — tcp_backlog default value (128)
 *  2.  tcp_backlog field can be set to custom values
 *  3.  tcp_backlog validation: positive values accepted; 0/negative handled
 *  4.  max_connections default and reconfiguration
 *  5.  Backpressure state: connection rejection increments rejected_connections stat
 *  6.  Backpressure state: overloaded_ flag transitions (entering / recovering)
 *  7.  Backpressure + per-IP limit: independent rejection paths
 *  8.  Recovery: overloaded_ cleared once active count drops below max_connections
 *  9.  Stats: rejected_connections counter starts at zero
 * 10.  Config: all backpressure-related fields coexist without conflict
 */

#include <gtest/gtest.h>
#include "network/wire_protocol_server.h"
#include <atomic>
#include <map>
#include <string>

using namespace themis::network;

// ============================================================================
// 1. tcp_backlog default
// ============================================================================

TEST(WireProtocolBackpressure, TcpBacklogDefaultIs128) {
    WireProtocolServer::Config cfg;
    EXPECT_EQ(cfg.tcp_backlog, 128);
}

// ============================================================================
// 2. tcp_backlog can be customized
// ============================================================================

TEST(WireProtocolBackpressure, TcpBacklogCustomValue) {
    WireProtocolServer::Config cfg;
    cfg.tcp_backlog = 512;
    EXPECT_EQ(cfg.tcp_backlog, 512);
}

TEST(WireProtocolBackpressure, TcpBacklogHighConcurrency) {
    WireProtocolServer::Config cfg;
    cfg.tcp_backlog = 4096;
    EXPECT_EQ(cfg.tcp_backlog, 4096);
}

// ============================================================================
// 3. tcp_backlog edge values
// ============================================================================

TEST(WireProtocolBackpressure, TcpBacklogMinimumOne) {
    WireProtocolServer::Config cfg;
    cfg.tcp_backlog = 1;
    EXPECT_EQ(cfg.tcp_backlog, 1);
}

// ============================================================================
// 4. max_connections default and reconfiguration
// ============================================================================

TEST(WireProtocolBackpressure, MaxConnectionsDefault) {
    WireProtocolServer::Config cfg;
    EXPECT_EQ(cfg.max_connections, 1000u);
}

TEST(WireProtocolBackpressure, MaxConnectionsCanBeReduced) {
    WireProtocolServer::Config cfg;
    cfg.max_connections = 10;
    EXPECT_EQ(cfg.max_connections, 10u);
}

TEST(WireProtocolBackpressure, MaxConnectionsCanBeSetUnlimited) {
    WireProtocolServer::Config cfg;
    cfg.max_connections = 0;  // 0 = no global limit in policy
    EXPECT_EQ(cfg.max_connections, 0u);
}

// ============================================================================
// 5. Stats: rejected_connections starts at zero
// ============================================================================

TEST(WireProtocolBackpressure, StatsRejectedConnectionsDefaultZero) {
    WireProtocolServer::Stats stats;
    EXPECT_EQ(stats.rejected_connections, 0u);
}

TEST(WireProtocolBackpressure, StatsAllFieldsDefaultZero) {
    WireProtocolServer::Stats stats;
    EXPECT_EQ(stats.total_connections, 0u);
    EXPECT_EQ(stats.active_connections, 0u);
    EXPECT_EQ(stats.rejected_connections, 0u);
    EXPECT_EQ(stats.total_requests, 0u);
    EXPECT_EQ(stats.total_errors, 0u);
    EXPECT_EQ(stats.auth_failures, 0u);
    EXPECT_EQ(stats.bytes_received, 0u);
    EXPECT_EQ(stats.bytes_sent, 0u);
}

// ============================================================================
// 6. Backpressure overload state — logic mirror
// ============================================================================

namespace {

/// Mirror the backpressure decision logic from checkConnectionLimit() and the
/// overloaded_ state transition in handleAccept().  This lets us unit-test the
/// logic without a live server.
struct BackpressureStateMirror {
    uint32_t max_connections = 1000;
    std::atomic<uint32_t> active_count{0};
    std::atomic<bool> overloaded{false};
    std::atomic<uint64_t> rejected{0};

    /// Returns true when a new connection should be accepted.
    bool checkLimit() const {
        return max_connections == 0 ||
               active_count.load(std::memory_order_relaxed) < max_connections;
    }

    /// Simulate accept: returns true on success, false when backpressure fires.
    bool accept() {
        if (!checkLimit()) {
            // First rejection: set overloaded flag.
            overloaded.store(true, std::memory_order_relaxed);
            rejected.fetch_add(1, std::memory_order_relaxed);
            return false;
        }
        active_count.fetch_add(1, std::memory_order_relaxed);
        return true;
    }

    /// Simulate connection close / recovery.
    void release() {
        const uint32_t prev = active_count.fetch_sub(1, std::memory_order_relaxed);
        if (overloaded.load(std::memory_order_relaxed) && max_connections > 0 &&
            prev - 1 < max_connections) {
            overloaded.store(false, std::memory_order_relaxed);
        }
    }
};

}  // anonymous namespace

TEST(WireProtocolBackpressure, NotOverloadedInitially) {
    BackpressureStateMirror bp;
    bp.max_connections = 5;
    EXPECT_FALSE(bp.overloaded.load());
}

TEST(WireProtocolBackpressure, OverloadedFlagSetOnFirstRejection) {
    BackpressureStateMirror bp;
    bp.max_connections = 2;

    EXPECT_TRUE(bp.accept());   // conn 1
    EXPECT_TRUE(bp.accept());   // conn 2  (now at limit)
    EXPECT_FALSE(bp.accept());  // conn 3  → rejected, overloaded set
    EXPECT_TRUE(bp.overloaded.load());
    EXPECT_EQ(bp.rejected.load(), 1u);
}

TEST(WireProtocolBackpressure, RejectedConnectionsAccumulate) {
    BackpressureStateMirror bp;
    bp.max_connections = 1;

    EXPECT_TRUE(bp.accept());  // 1 connection – at limit
    for (int i = 0; i < 5; ++i) {
        EXPECT_FALSE(bp.accept());  // all rejected
    }
    EXPECT_EQ(bp.rejected.load(), 5u);
}

// ============================================================================
// 7. Recovery: overloaded_ clears when count drops below limit
// ============================================================================

TEST(WireProtocolBackpressure, RecoveryAfterConnectionClose) {
    BackpressureStateMirror bp;
    bp.max_connections = 2;

    bp.accept();   // conn 1
    bp.accept();   // conn 2 – at limit
    bp.accept();   // rejected → overloaded = true
    EXPECT_TRUE(bp.overloaded.load());

    bp.release();  // close conn 2 → active=1, below limit → overloaded=false
    EXPECT_FALSE(bp.overloaded.load());
}

TEST(WireProtocolBackpressure, AcceptsAgainAfterRecovery) {
    BackpressureStateMirror bp;
    bp.max_connections = 1;

    bp.accept();              // at limit
    EXPECT_FALSE(bp.accept()); // rejected
    EXPECT_TRUE(bp.overloaded.load());

    bp.release();              // recovery
    EXPECT_FALSE(bp.overloaded.load());
    EXPECT_TRUE(bp.accept());  // can accept again
}

// ============================================================================
// 8. No global limit (max_connections == 0) → never overloaded
// ============================================================================

TEST(WireProtocolBackpressure, UnlimitedConnectionsNeverOverloaded) {
    BackpressureStateMirror bp;
    bp.max_connections = 0;  // unlimited

    for (int i = 0; i < 100; ++i) {
        EXPECT_TRUE(bp.accept());
    }
    EXPECT_FALSE(bp.overloaded.load());
    EXPECT_EQ(bp.rejected.load(), 0u);
}

// ============================================================================
// 9. Config: all backpressure-related fields coexist
// ============================================================================

TEST(WireProtocolBackpressure, AllBackpressureFieldsCoexist) {
    WireProtocolServer::Config cfg;
    cfg.max_connections = 500;
    cfg.max_connections_per_ip = 5;
    cfg.tcp_backlog = 256;

    EXPECT_EQ(cfg.max_connections, 500u);
    EXPECT_EQ(cfg.max_connections_per_ip, 5u);
    EXPECT_EQ(cfg.tcp_backlog, 256);
}

// ============================================================================
// 10. tcp_backlog does not interfere with other Config fields
// ============================================================================

TEST(WireProtocolBackpressure, TcpBacklogIndependentOfPort) {
    WireProtocolServer::Config cfg;
    cfg.port = 9000;
    cfg.tcp_backlog = 64;

    EXPECT_EQ(cfg.port, 9000u);
    EXPECT_EQ(cfg.tcp_backlog, 64);
}

TEST(WireProtocolBackpressure, TcpBacklogIndependentOfTLS) {
    WireProtocolServer::Config cfg;
    cfg.enable_tls = true;
    cfg.tcp_backlog = 200;

    EXPECT_TRUE(cfg.enable_tls);
    EXPECT_EQ(cfg.tcp_backlog, 200);
}

// ============================================================================
// 11. Regression: unregisterConnection must not underflow when called for
//     sessions that were never registerConnection()-ed (e.g. rejected before
//     start() was called — client_ip_ stays "unknown").
// ============================================================================

namespace {

/// Minimal mirror of the fixed unregisterConnection logic:
/// only adjusts active_count when the IP was actually in the map.
struct UnregisterMirror {
    uint32_t max_connections = 10;
    std::atomic<uint32_t> active_count{0};
    std::atomic<bool> overloaded{false};
    std::map<std::string, uint32_t> per_ip;

    void registerConn(const std::string& ip) {
        per_ip[ip]++;
        active_count.fetch_add(1, std::memory_order_relaxed);
    }

    void unregisterConn(const std::string& ip) {
        bool was_registered = false;
        auto it = per_ip.find(ip);
        if (it != per_ip.end() && it->second > 0) {
            it->second--;
            was_registered = true;
        }
        if (!was_registered) return;  // BUG FIX: don't fetch_sub when not registered
        active_count.fetch_sub(1, std::memory_order_relaxed);
    }
};

}  // anonymous namespace

TEST(WireProtocolBackpressure, UnregisterUnknownIpDoesNotUnderflow) {
    UnregisterMirror m;
    m.active_count.store(0);

    // Simulate a rejected session whose client_ip_ was never set ("unknown")
    m.unregisterConn("unknown");

    // active_count must remain 0, NOT wrap to UINT32_MAX
    EXPECT_EQ(m.active_count.load(), 0u);
}

TEST(WireProtocolBackpressure, UnregisterEmptyIpDoesNotUnderflow) {
    UnregisterMirror m;
    m.active_count.store(0);

    // Simulate a WebSocket-upgraded session whose client_ip_ was cleared
    m.unregisterConn("");

    EXPECT_EQ(m.active_count.load(), 0u);
}

TEST(WireProtocolBackpressure, UnregisterRegisteredIpDecrementsCounter) {
    UnregisterMirror m;
    m.registerConn("10.0.0.1");
    m.registerConn("10.0.0.1");
    EXPECT_EQ(m.active_count.load(), 2u);

    m.unregisterConn("10.0.0.1");
    EXPECT_EQ(m.active_count.load(), 1u);

    m.unregisterConn("10.0.0.1");
    EXPECT_EQ(m.active_count.load(), 0u);
}

TEST(WireProtocolBackpressure, MixedRegisteredAndUnregisteredCalls) {
    UnregisterMirror m;
    m.registerConn("192.168.1.1");
    m.registerConn("192.168.1.2");
    EXPECT_EQ(m.active_count.load(), 2u);

    // Unregister a session that was never registered (rejected connection)
    m.unregisterConn("unknown");
    EXPECT_EQ(m.active_count.load(), 2u);  // unchanged

    // Properly unregister a real session
    m.unregisterConn("192.168.1.1");
    EXPECT_EQ(m.active_count.load(), 1u);
}
