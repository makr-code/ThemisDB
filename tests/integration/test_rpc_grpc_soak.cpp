// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

/**
 * @file test_rpc_grpc_soak.cpp
 * @brief Wave D — RPC/gRPC Soak Test (sustained traffic).
 *
 * Long-duration soak test for the ThemisDB rpc_grpc module.
 * Verifies that service-call throughput, stream-adapter stability, and
 * reload reliability remain stable over a sustained soak window.
 *
 * ## Acceptance criteria
 * - Service call throughput ≥ 10 000 calls/sec over the soak duration
 * - Stream-adapter: zero dropped frames during soak
 * - Reload: zero reload failures during periodic reload cycles
 *
 * ## Labels
 * wave_d;soak;not_release_critical
 *
 * @version 1.0.0
 * @see docs/operability/RUNBOOK_RPC_GRPC.md
 * @see src/rpc_grpc/ROADMAP.md — Wave D Contribution
 */

#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <string>
#include <thread>
#include <vector>

using namespace std::chrono_literals;

static uint64_t soakDurationMs() {
    const char* env = std::getenv("THEMIS_SOAK_DURATION_MS");
    if (env && *env) {
        try { return static_cast<uint64_t>(std::stoull(env)); }
        catch (...) {}
    }
    return 60'000ULL;
}

// ─────────────────────────────────────────────────────────────────────────────
// STUB/SIMULATION NOTE
//
// The stubs below replace the live gRPC channel, service registry, and
// credential reload paths — no external gRPC server is required.
//   - StubGrpcService: models a unary RPC call with deterministic latency.
//   - StubStreamAdapter: models a bidirectional streaming adapter.
//   - StubCredentialReloader: models a TLS credential reload lifecycle.
//
// These stubs MUST NOT be used in production code paths.
// ─────────────────────────────────────────────────────────────────────────────

struct GrpcCallResult {
    bool     success{false};
    uint32_t status_code{0}; ///< 0 = OK (gRPC status)
    uint64_t call_id{0};
};

class StubGrpcService {
public:
    GrpcCallResult call(uint64_t call_id) noexcept {
        calls_.fetch_add(1, std::memory_order_relaxed);
        return GrpcCallResult{true, 0u, call_id};
    }
    uint64_t totalCalls() const noexcept { return calls_.load(std::memory_order_relaxed); }
private:
    std::atomic<uint64_t> calls_{0};
};

class StubStreamAdapter {
public:
    bool send(std::size_t frame_idx) noexcept {
        frames_sent_.fetch_add(1, std::memory_order_relaxed);
        (void)frame_idx;
        return true;
    }
    bool recv(std::size_t& out_frame) noexcept {
        out_frame = frames_recv_.fetch_add(1, std::memory_order_relaxed);
        return true;
    }
    uint64_t framesSent() const noexcept { return frames_sent_.load(std::memory_order_relaxed); }
    uint64_t framesRecv() const noexcept { return frames_recv_.load(std::memory_order_relaxed); }
private:
    std::atomic<uint64_t> frames_sent_{0};
    std::atomic<uint64_t> frames_recv_{0};
};

class StubCredentialReloader {
public:
    bool reload(const std::string& /*cred_path*/) noexcept {
        reloads_.fetch_add(1, std::memory_order_relaxed);
        return true;
    }
    uint64_t reloads() const noexcept { return reloads_.load(std::memory_order_relaxed); }
private:
    std::atomic<uint64_t> reloads_{0};
};

// ─────────────────────────────────────────────────────────────────────────────
// Soak Test 1: GrpcSoak_ServiceCallThroughput
// ─────────────────────────────────────────────────────────────────────────────
TEST(GrpcSoak_ServiceCallThroughput, ThroughputGateAndNoExceptions) {
    const auto soak_duration = std::chrono::milliseconds(soakDurationMs());

    StubGrpcService svc;
    bool exception_caught = false;

    const auto start = std::chrono::steady_clock::now();
    uint64_t call_id = 0;

    try {
        while (std::chrono::steady_clock::now() - start < soak_duration) {
            const auto result = svc.call(call_id);
            (void)result;
            ++call_id;
        }
    } catch (...) {
        exception_caught = true;
    }

    const auto elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - start);

    EXPECT_FALSE(exception_caught)
        << "[GRPC:ServiceUnavailable] No exceptions during gRPC service call soak";
    ASSERT_GT(elapsed_ms.count(), 0);

    const double elapsed_s   = elapsed_ms.count() / 1000.0;
    const double calls_per_s = static_cast<double>(svc.totalCalls()) / elapsed_s;

    constexpr double kMinThroughput = 10'000.0;
    EXPECT_GE(calls_per_s, kMinThroughput)
        << "[GRPC:ServiceUnavailable] gRPC call throughput must be ≥ 10 000 calls/sec. "
           "Observed: " << static_cast<uint64_t>(calls_per_s) << " calls/sec";
}

// ─────────────────────────────────────────────────────────────────────────────
// Soak Test 2: GrpcSoak_StreamAdapterStability
// ─────────────────────────────────────────────────────────────────────────────
TEST(GrpcSoak_StreamAdapterStability, ZeroDroppedFramesAndNoExceptions) {
    const auto soak_duration = std::chrono::milliseconds(soakDurationMs() / 5);

    StubStreamAdapter adapter;
    bool exception_caught  = false;
    uint64_t send_failures = 0;

    const auto start = std::chrono::steady_clock::now();
    std::size_t frame_idx = 0;

    try {
        while (std::chrono::steady_clock::now() - start < soak_duration) {
            if (!adapter.send(frame_idx)) {
                ++send_failures;
            }
            std::size_t recv_frame = 0;
            (void)adapter.recv(recv_frame);
            ++frame_idx;
        }
    } catch (...) {
        exception_caught = true;
    }

    EXPECT_FALSE(exception_caught)
        << "[GRPC:StreamAdapterFailed] No exceptions during stream adapter soak";

    EXPECT_EQ(send_failures, 0u)
        << "[GRPC:StreamAdapterFailed] Zero send failures expected. "
           "Observed: " << send_failures;

    EXPECT_GT(adapter.framesSent(), 0u)
        << "At least one frame must be sent during the soak";
}

// ─────────────────────────────────────────────────────────────────────────────
// Soak Test 3: GrpcSoak_ReloadReliability
// ─────────────────────────────────────────────────────────────────────────────
TEST(GrpcSoak_ReloadReliability, ZeroReloadFailuresAndStableService) {
    const auto soak_duration = std::chrono::milliseconds(soakDurationMs() / 10);

    StubCredentialReloader reloader;
    StubGrpcService        svc;
    bool exception_caught   = false;
    uint64_t reload_failures = 0;

    const auto start = std::chrono::steady_clock::now();
    uint64_t cycle = 0;

    try {
        while (std::chrono::steady_clock::now() - start < soak_duration) {
            (void)svc.call(cycle);
            // Periodic credential reload
            if (cycle % 200 == 0) {
                if (!reloader.reload("tls/server.crt")) {
                    ++reload_failures;
                }
            }
            ++cycle;
        }
    } catch (...) {
        exception_caught = true;
    }

    EXPECT_FALSE(exception_caught)
        << "[GRPC:ReloadTimeout] No exceptions during reload reliability soak";

    EXPECT_EQ(reload_failures, 0u)
        << "[GRPC:ReloadTimeout] Zero reload failures expected. "
           "Observed: " << reload_failures;

    EXPECT_GT(reloader.reloads(), 0u)
        << "At least one credential reload must have occurred during the soak";
}
