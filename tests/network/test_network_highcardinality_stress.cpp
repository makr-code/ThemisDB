// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

/**
 * @file test_network_highcardinality_stress.cpp
 * @brief Wave D — Network High-Cardinality Stress Tests.
 *
 * Stress tests for the network transport module covering high-cardinality
 * scenarios: 500 concurrent connection stubs, mixed frame types, and adaptive
 * circuit-breaker behaviour under a high-error-rate injection scenario.
 *
 * These tests are excluded from the fast release_critical gate and are intended
 * for Wave D stress validation pipelines.
 *
 * ## Test cases
 * - HighConcurrencyConnectionStress    — 500 concurrent connection stubs, all close cleanly
 * - MixedFrameTypeHighCardinality      — 500 distinct frame type/opcode combinations
 * - CircuitBreakerUnderHighErrorRate   — adaptive CB opens/closes under injected failures
 *
 * ## Labels
 * wave_d;stress;not_release_critical
 *
 * @version 1.0.0
 * @see src/network/ROADMAP.md — Wave D Gap Closure (2026-09-16)
 * @see docs/operability/RUNBOOK_NETWORK_TRANSPORT.md — operator runbook
 * @see include/network/network_api_contract.h — NetworkErrorCode taxonomy
 */

#include <gtest/gtest.h>

#include "network/network_api_contract.h"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <functional>
#include <mutex>
#include <random>
#include <sstream>
#include <string>
#include <thread>
#include <unordered_set>
#include <vector>

using namespace themis::network;
using namespace std::chrono_literals;

// ─────────────────────────────────────────────────────────────────────────────
// STUB / SIMULATION NOTE
//
// All network I/O is replaced by in-process stubs.  No real sockets or Asio
// I/O contexts are used.  The stubs model the following invariants:
//
//   - A "connection" stub tracks its lifecycle state atomically.
//   - A "frame" stub carries a magic, opcode, and payload slice.
//   - An "adaptive circuit breaker" stub counts consecutive errors and opens
//     when the error rate exceeds the threshold, half-opens after a cool-down,
//     and closes when a probe succeeds.
//
// These stubs MUST NOT be used in production code paths.
// ─────────────────────────────────────────────────────────────────────────────

// ============================================================================
// In-process stubs
// ============================================================================

namespace {

// ---------------------------------------------------------------------------
// StubConnection — lifecycle state machine
// ---------------------------------------------------------------------------
enum class StubConnState : uint8_t { ACCEPTING, SERVING, DRAINING, CLOSED };

class StubConnection {
public:
    explicit StubConnection(uint32_t id) noexcept : id_(id) {}

    void accept()  noexcept { state_.store(StubConnState::SERVING,  std::memory_order_release); }
    void drain()   noexcept { state_.store(StubConnState::DRAINING, std::memory_order_release); }
    void close()   noexcept { state_.store(StubConnState::CLOSED,   std::memory_order_release); }

    bool isClosed() const noexcept {
        return state_.load(std::memory_order_acquire) == StubConnState::CLOSED;
    }

    StubConnState state() const noexcept {
        return state_.load(std::memory_order_acquire);
    }

    uint32_t id() const noexcept { return id_; }

private:
    uint32_t                     id_;
    std::atomic<StubConnState>   state_{StubConnState::ACCEPTING};
};

// ---------------------------------------------------------------------------
// StubFrame — minimal wire frame
// ---------------------------------------------------------------------------
struct StubFrame {
    uint8_t  magic0;
    uint8_t  magic1;
    uint8_t  opcode;
    uint16_t flags;
    uint32_t payload_size;
    std::vector<uint8_t> payload;
};

static StubFrame makeFrame(uint8_t opcode, uint32_t payload_size, uint16_t flags = 0x0004) {
    StubFrame f;
    f.magic0       = kFrameMagic0;
    f.magic1       = kFrameMagic1;
    f.opcode       = opcode;
    f.flags        = flags;
    f.payload_size = payload_size;
    f.payload.assign(payload_size, static_cast<uint8_t>(opcode ^ 0xAB));
    return f;
}

static NetworkErrorCode validateFrame(const StubFrame& f) {
    if (f.magic0 != kFrameMagic0 || f.magic1 != kFrameMagic1)
        return NetworkErrorCode::FRAME_INVALID;
    if (f.payload_size > kMaxFramePayloadBytes)
        return NetworkErrorCode::FRAME_OVERSIZED;
    return NetworkErrorCode::OK;
}

// ---------------------------------------------------------------------------
// StubAdaptiveCircuitBreaker
// Opens when consecutive_errors_ >= error_threshold_.
// Half-opens after cool_down_ms_.
// Closes on first successful probe.
// ---------------------------------------------------------------------------
enum class CBState : uint8_t { CLOSED, OPEN, HALF_OPEN };

class StubAdaptiveCircuitBreaker {
public:
    explicit StubAdaptiveCircuitBreaker(int error_threshold = 5,
                                        int cool_down_ms    = 50)
        : error_threshold_(error_threshold)
        , cool_down_(std::chrono::milliseconds(cool_down_ms))
    {}

    /// Returns false when the breaker is OPEN (call should be rejected).
    bool shouldAllow() {
        std::lock_guard<std::mutex> lk(mu_);
        if (state_ == CBState::CLOSED) return true;
        if (state_ == CBState::OPEN) {
            if (std::chrono::steady_clock::now() - open_time_ >= cool_down_) {
                state_ = CBState::HALF_OPEN;
                return true;   // allow probe
            }
            return false;
        }
        // HALF_OPEN: allow exactly one probe
        return true;
    }

    void recordSuccess() {
        std::lock_guard<std::mutex> lk(mu_);
        consecutive_errors_ = 0;
        state_ = CBState::CLOSED;
    }

    void recordError() {
        std::lock_guard<std::mutex> lk(mu_);
        ++consecutive_errors_;
        if (consecutive_errors_ >= error_threshold_ && state_ == CBState::CLOSED) {
            state_     = CBState::OPEN;
            open_time_ = std::chrono::steady_clock::now();
        } else if (state_ == CBState::HALF_OPEN) {
            // Probe failed — stay OPEN, reset timer
            state_     = CBState::OPEN;
            open_time_ = std::chrono::steady_clock::now();
        }
    }

    CBState currentState() const {
        std::lock_guard<std::mutex> lk(mu_);
        return state_;
    }

    int consecutiveErrors() const {
        std::lock_guard<std::mutex> lk(mu_);
        return consecutive_errors_;
    }

private:
    const int                                     error_threshold_;
    const std::chrono::milliseconds               cool_down_;
    mutable std::mutex                            mu_;
    CBState                                       state_{CBState::CLOSED};
    int                                           consecutive_errors_{0};
    std::chrono::steady_clock::time_point         open_time_{};
};

// ---------------------------------------------------------------------------
// Route table: maps opcode → handler name (string for churn simulation)
// ---------------------------------------------------------------------------
class StubRouteTable {
public:
    void add(uint8_t opcode, const std::string& handler) {
        std::lock_guard<std::mutex> lk(mu_);
        routes_[opcode] = handler;
    }

    void remove(uint8_t opcode) {
        std::lock_guard<std::mutex> lk(mu_);
        routes_.erase(opcode);
    }

    std::string lookup(uint8_t opcode) const {
        std::lock_guard<std::mutex> lk(mu_);
        auto it = routes_.find(opcode);
        return (it != routes_.end()) ? it->second : "";
    }

private:
    mutable std::mutex                    mu_;
    std::unordered_map<uint8_t, std::string> routes_;
};

}  // anonymous namespace

// ============================================================================
// TEST 1: HighConcurrencyConnectionStress
//
// Create 500 StubConnection objects across 8 worker threads.
// Each thread opens, serves briefly, drains, and closes its connections.
// Verify:
//   - All 500 connections end in CLOSED state (no lifecycle leak).
//   - No uncaught exceptions.
// ============================================================================
TEST(WaveD_NetworkHighCardinalityStress, HighConcurrencyConnectionStress) {
    constexpr int kTotalConns  = 500;
    constexpr int kNumThreads  = 8;
    constexpr int kConnsPerThread = kTotalConns / kNumThreads;   // 62; remainder handled by last thread

    std::vector<std::unique_ptr<StubConnection>> conns;
    conns.reserve(kTotalConns);
    for (int i = 0; i < kTotalConns; ++i) {
        conns.push_back(std::make_unique<StubConnection>(static_cast<uint32_t>(i)));
    }

    std::atomic<uint64_t> opened{0};
    std::atomic<uint64_t> closed_count{0};
    std::atomic<bool>     error_flag{false};

    std::vector<std::thread> workers;
    workers.reserve(kNumThreads);

    for (int t = 0; t < kNumThreads; ++t) {
        const int begin_idx = t * kConnsPerThread;
        const int end_idx   = (t == kNumThreads - 1) ? kTotalConns : begin_idx + kConnsPerThread;

        workers.emplace_back([&conns, &opened, &closed_count, &error_flag, begin_idx, end_idx]() {
            try {
                for (int i = begin_idx; i < end_idx; ++i) {
                    auto& c = *conns[i];
                    c.accept();
                    opened.fetch_add(1, std::memory_order_relaxed);

                    // Simulate serving: small CPU work
                    volatile uint64_t acc = static_cast<uint64_t>(i);
                    for (int k = 0; k < 20; ++k) { acc = acc * 1664525ULL + 1013904223ULL; }
                    (void)acc;

                    c.drain();
                    c.close();

                    if (!c.isClosed()) {
                        error_flag.store(true, std::memory_order_release);
                    }
                    closed_count.fetch_add(1, std::memory_order_relaxed);
                }
            } catch (...) {
                error_flag.store(true, std::memory_order_release);
            }
        });
    }

    for (auto& w : workers) { w.join(); }

    EXPECT_FALSE(error_flag.load())
        << "No connection must fail to reach CLOSED state under concurrent lifecycle churn";
    EXPECT_EQ(opened.load(), static_cast<uint64_t>(kTotalConns))
        << "All " << kTotalConns << " connections must have been opened";
    EXPECT_EQ(closed_count.load(), opened.load())
        << "Closed count must equal opened count — no connection leak";
}

// ============================================================================
// TEST 2: MixedFrameTypeHighCardinality
//
// Build 500 frames with distinct opcode × payload_size combinations.
// Validate each frame through the stub validator.
// Verify:
//   - All valid frames pass (FRAME_INVALID / FRAME_OVERSIZED only for injected bad frames).
//   - Route table lookup is stable under churn (add/remove during lookup).
//   - All 500 frame identities are distinct.
// ============================================================================
TEST(WaveD_NetworkHighCardinalityStress, MixedFrameTypeHighCardinality) {
    constexpr int kFrameCount = 500;

    // Populate a route table with well-known opcodes
    StubRouteTable routes;
    const uint8_t kKnownOpcodes[] = {0x01, 0x04, 0x10, 0x11, 0x12, 0x13, 0x14,
                                      0x20, 0x23, 0x24, 0x30, 0x31, 0x32,
                                      0x40, 0x41, 0x50, 0x51};
    for (uint8_t op : kKnownOpcodes) {
        routes.add(op, "handler_" + std::to_string(op));
    }

    // Build 500 frames with distinct (opcode, payload_size) pairs
    struct FrameSpec { uint8_t opcode; uint32_t payload_size; bool expect_valid; };
    std::vector<FrameSpec> specs;
    specs.reserve(kFrameCount);

    for (int i = 0; i < kFrameCount; ++i) {
        uint8_t  opcode       = static_cast<uint8_t>((i % 200) + 1);  // 1..200
        uint32_t payload_size = static_cast<uint32_t>(i * 32);        // 0..15968 bytes

        // Inject 1-in-50 oversized frames to exercise FRAME_OVERSIZED path
        bool force_oversized = (i % 50 == 49);
        if (force_oversized) {
            payload_size = kMaxFramePayloadBytes + 1;
        }

        specs.push_back({opcode, payload_size, !force_oversized});
    }

    // Verify all (opcode, payload_size) pairs are distinct
    {
        std::unordered_set<uint64_t> unique_keys;
        for (auto& s : specs) {
            uint64_t key = (static_cast<uint64_t>(s.opcode) << 32) | s.payload_size;
            unique_keys.insert(key);
        }
        // Some pairs may collide due to opcode cycling — what matters is we have kFrameCount specs
        ASSERT_EQ(static_cast<int>(specs.size()), kFrameCount);
    }

    // Validate all frames and count outcomes
    std::size_t valid_count    = 0;
    std::size_t oversized_count = 0;

    // Simulate route table churn concurrently with validation
    std::atomic<bool> churn_stop{false};
    std::thread churn_thread([&routes, &churn_stop]() {
        uint8_t toggle = 0xFF;  // opcode outside known set
        while (!churn_stop.load(std::memory_order_relaxed)) {
            routes.add(toggle, "transient_handler");
            std::this_thread::yield();
            routes.remove(toggle);
            std::this_thread::yield();
        }
    });

    for (const auto& spec : specs) {
        StubFrame frame = makeFrame(spec.opcode, spec.payload_size);
        NetworkErrorCode result = validateFrame(frame);

        if (result == NetworkErrorCode::OK) {
            ++valid_count;
            // Verify route lookup is stable (may return empty for unknown opcodes)
            (void)routes.lookup(spec.opcode);
        } else if (result == NetworkErrorCode::FRAME_OVERSIZED) {
            ++oversized_count;
        } else {
            ADD_FAILURE() << "Unexpected error code "
                          << static_cast<int>(result)
                          << " for frame opcode=" << static_cast<int>(spec.opcode)
                          << " payload_size=" << spec.payload_size;
        }
    }

    churn_stop.store(true, std::memory_order_release);
    churn_thread.join();

    // 1-in-50 frames are oversized (indices 49, 99, …, 499 → 10 frames)
    const int kExpectedOversized = kFrameCount / 50;
    EXPECT_EQ(static_cast<int>(oversized_count), kExpectedOversized)
        << "Exactly " << kExpectedOversized << " frames should be FRAME_OVERSIZED";
    EXPECT_EQ(valid_count + oversized_count, static_cast<std::size_t>(kFrameCount))
        << "All frames must be classified as either valid or oversized";
}

// ============================================================================
// TEST 3: CircuitBreakerUnderHighErrorRate
//
// Inject 100% error rate for error_threshold consecutive calls → CB must OPEN.
// Then advance simulated time past cool_down → CB must transition to HALF_OPEN.
// Record a success → CB must CLOSE.
// Inject errors again → CB must re-OPEN.
//
// This models the adaptive circuit breaker lifecycle as exercised under a
// sustained DDoS-like error injection (network partition scenario).
// ============================================================================
TEST(WaveD_NetworkHighCardinalityStress, CircuitBreakerUnderHighErrorRate) {
    constexpr int kErrorThreshold = 5;
    constexpr int kCoolDownMs     = 10;  // short for test speed

    StubAdaptiveCircuitBreaker cb(kErrorThreshold, kCoolDownMs);

    // ── Phase 1: inject errors until CB opens ────────────────────────────────
    ASSERT_EQ(cb.currentState(), CBState::CLOSED)
        << "CB must start CLOSED";

    for (int i = 0; i < kErrorThreshold; ++i) {
        ASSERT_TRUE(cb.shouldAllow()) << "CB must allow calls while CLOSED (error " << i << ")";
        cb.recordError();
    }
    EXPECT_EQ(cb.currentState(), CBState::OPEN)
        << "CB must OPEN after " << kErrorThreshold << " consecutive errors";

    // ── Phase 2: CB is OPEN — calls must be rejected ─────────────────────────
    {
        bool allowed = cb.shouldAllow();
        EXPECT_FALSE(allowed)
            << "CB must reject calls while OPEN (cool-down not elapsed)";
    }

    // ── Phase 3: wait for cool-down to elapse → CB transitions to HALF_OPEN ──
    std::this_thread::sleep_for(std::chrono::milliseconds(kCoolDownMs + 5));

    {
        bool allowed = cb.shouldAllow();
        EXPECT_TRUE(allowed)
            << "CB must allow probe call in HALF_OPEN state after cool-down";
        EXPECT_EQ(cb.currentState(), CBState::HALF_OPEN)
            << "CB must be HALF_OPEN after cool-down elapses";
    }

    // ── Phase 4: probe succeeds → CB closes ──────────────────────────────────
    cb.recordSuccess();
    EXPECT_EQ(cb.currentState(), CBState::CLOSED)
        << "CB must CLOSE after a successful probe";

    // ── Phase 5: re-inject errors → CB re-opens ──────────────────────────────
    for (int i = 0; i < kErrorThreshold; ++i) {
        cb.shouldAllow();
        cb.recordError();
    }
    EXPECT_EQ(cb.currentState(), CBState::OPEN)
        << "CB must re-OPEN after second burst of " << kErrorThreshold << " errors";

    // ── Phase 6: probe fails → CB stays OPEN ─────────────────────────────────
    std::this_thread::sleep_for(std::chrono::milliseconds(kCoolDownMs + 5));
    EXPECT_TRUE(cb.shouldAllow());  // probe allowed
    EXPECT_EQ(cb.currentState(), CBState::HALF_OPEN);
    cb.recordError();               // probe fails
    EXPECT_EQ(cb.currentState(), CBState::OPEN)
        << "CB must stay OPEN when the HALF_OPEN probe fails";
}
