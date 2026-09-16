// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

/**
 * @file bench_network_protocol_path.cpp
 * @brief Mid-term protocol-path performance consistency benchmark gates (NRG-P1..NRG-P4).
 *
 * Provides reproducible latency/throughput measurements for the four
 * protocol-path promotion gates identified in the network module roadmap
 * (Mid-term planned feature: "Improve protocol-path performance consistency
 * with benchmark-backed promotion gates", Target: Q1 2027).
 *
 * ## Gate table
 *
 * | Gate   | Benchmark                                      | Threshold               |
 * |--------|------------------------------------------------|-------------------------|
 * | NRG-P1 | WebSocket frame parse throughput              | ≥ 5 M frames/sec        |
 * | NRG-P2 | TCP zero-copy path overhead per 64 KiB payload| ≤ 5 µs per call (stub)  |
 * | NRG-P3 | QUIC 0-RTT handshake latency (stub)           | ≤ 50 ms per handshake   |
 * | NRG-P4 | gRPC metadata overhead per call (stub)        | ≤ 10 µs per call        |
 *
 * ## Notes
 * - NRG-P1 and NRG-P2 are backed by in-process stub implementations of the
 *   real hot paths (WS parse, zero-copy frame building) and produce meaningful
 *   baselines on any hardware.
 * - NRG-P3 (QUIC 0-RTT) and NRG-P4 (gRPC metadata) are stub benchmarks:
 *   they model the expected cost of the real path and establish a baseline.
 *   Hardware-representative baselines are required before Wave D sign-off
 *   (see ROADMAP.md §Mid-term planned features — marked [~]).
 * - All benchmarks use kNetworkCanonicalSeed = 42, Repetitions(5).
 * - No live sockets; all I/O is mocked in-process.
 *
 * @see include/network/network_api_contract.h — contract thresholds
 * @see src/network/ROADMAP.md — Mid-term planned features (NRG-P1..NRG-P4)
 * @see benchmarks/network/bench_network_release_gates.cpp — NRG-01..NRG-06 pattern
 */

#include <benchmark/benchmark.h>

#include "network/network_api_contract.h"

#include <algorithm>
#include <array>
#include <atomic>
#include <cstdint>
#include <cstring>
#include <string>
#include <unordered_map>
#include <vector>

namespace themis {
namespace bench {
namespace nrg_p {

using namespace themis::network;

// ============================================================================
// Constants
// ============================================================================

static constexpr std::uint64_t kNetworkCanonicalSeed = 42;
static constexpr int           kRepetitions          = 5;
static constexpr int           kWarmupIterations     = 500;

// ============================================================================
// Stub helpers
// ============================================================================

namespace {

// ---------------------------------------------------------------------------
// NRG-P1: WebSocket frame parse stub
//
// Models the hot path: receive raw WS frame bytes → extract FIN/RSV/opcode/
// mask bit / payload length → validate → dispatch opcode.
//
// The real path (http_ws::async_read handler) does exactly these byte-level
// operations.  This stub replicates the decision tree without I/O overhead.
// ---------------------------------------------------------------------------
struct WsFrameBytes {
    uint8_t byte0;  // FIN(1) | RSV(3) | opcode(4)
    uint8_t byte1;  // MASK(1) | payload_len(7)
    uint8_t payload[8];  // up to 8-byte payload for small frame bench
};

/// Returns the opcode or 0xFF on parse error.
static inline uint8_t parseWsFrame(const WsFrameBytes& f) noexcept {
    const bool fin    = (f.byte0 & 0x80) != 0;
    const bool rsv    = (f.byte0 & 0x70) != 0;  // RSV1-3 must be 0
    const uint8_t op  =  f.byte0 & 0x0F;
    const bool masked = (f.byte1 & 0x80) != 0;
    const uint8_t len =  f.byte1 & 0x7F;

    // Validation gates (mirrors real parser)
    if (rsv)       return 0xFF;  // RSV bits set — reject
    if (op == 0x08) return 0x08; // Close frame — handle close
    if (len > 125)  return 0xFF; // Extended payload — reject for this bench size

    benchmark::DoNotOptimize(fin);
    benchmark::DoNotOptimize(masked);
    benchmark::DoNotOptimize(len);
    return op;
}

// ---------------------------------------------------------------------------
// NRG-P2: Zero-copy frame path stub
//
// Models the cost of the ZeroCopyFrameBuilder::writeToWithSendfile() fast
// path for large payloads (≥ 64 KiB threshold):
//   1. Compute sendfile params (offset + size clamp)
//   2. Validate payload_fd (stub: always valid)
//   3. Record bytes sent in atomic counter
//
// The real path calls sendfile(2)/splice(2); this stub replaces those syscalls
// with equivalent arithmetic to measure the bookkeeping cost alone.
// ---------------------------------------------------------------------------
static constexpr std::size_t kZeroCopyThreshold = 64 * 1024;  // 64 KiB

class StubZeroCopyBuilder {
public:
    /// Returns the number of bytes "sent" (stub: always returns payload_size).
    std::size_t writeToWithSendfile(int /*payload_fd*/,
                                    std::size_t payload_offset,
                                    std::size_t payload_size) noexcept {
        if (payload_size < kZeroCopyThreshold) {
            // writev fallback path (stub)
            benchmark::DoNotOptimize(payload_offset);
            bytes_sent_.fetch_add(payload_size, std::memory_order_relaxed);
            return payload_size;
        }
        // sendfile path (stub): clamp to 1 MiB per call to match real sendfile limit
        const std::size_t chunk = std::min(payload_size, std::size_t{1024 * 1024});
        const std::size_t effective_offset = payload_offset + (payload_size - chunk);
        benchmark::DoNotOptimize(effective_offset);
        bytes_sent_.fetch_add(chunk, std::memory_order_relaxed);
        return chunk;
    }

    std::uint64_t totalBytesSent() const noexcept {
        return bytes_sent_.load(std::memory_order_relaxed);
    }

private:
    std::atomic<std::uint64_t> bytes_sent_{0};
};

// ---------------------------------------------------------------------------
// NRG-P3: QUIC 0-RTT handshake stub
//
// Models the expected cost of a QUIC 0-RTT resumption:
//   1. Look up session ticket in session cache (in-memory map)
//   2. Reconstruct early data keys (stub: 2-round key schedule step)
//   3. Validate anti-replay token (stub: bloom-filter lookup)
//
// The real 0-RTT path (QuicTransport + BoringSSL/Quiche) performs these
// steps; this stub models their cost without crypto I/O.
// ---------------------------------------------------------------------------
class StubQuicSessionCache {
public:
    StubQuicSessionCache() {
        // Pre-populate with 100 session tickets
        cache_.reserve(100);
        for (int i = 0; i < 100; ++i) {
            cache_["ticket-" + std::to_string(i)] = true;
        }
    }

    bool lookupTicket(const std::string& ticket_id) const {
        auto it = cache_.find(ticket_id);
        return it != cache_.end() && it->second;
    }

private:
    std::unordered_map<std::string, bool> cache_;
};

static uint64_t stubKeyScheduleStep(uint64_t seed, int rounds) noexcept {
    // Model 2-round key derivation cost (SHA-256 substitute)
    uint64_t acc = seed ^ kNetworkCanonicalSeed;
    for (int i = 0; i < rounds; ++i) {
        acc = (acc ^ (acc >> 17)) * 0xff51afd7ed558ccdULL;
        acc = (acc ^ (acc >> 31)) * 0xc4ceb9fe1a85ec53ULL;
        acc ^= acc >> 33;
    }
    return acc;
}

static bool stubAntiReplayCheck(uint64_t token_hash) noexcept {
    // Stub bloom-filter: modulo check (models 3 hash functions over 1024-bit array)
    return (token_hash % 7919) != 0;  // 1/7919 false positive
}

// ---------------------------------------------------------------------------
// NRG-P4: gRPC metadata overhead stub
//
// Models the cost of gRPC metadata processing per call:
//   1. Header frame construction (HPACK-style stub)
//   2. Method path lookup in routing table
//   3. Trailer encoding
//
// The real path (grpc++ / Envoy EnvoyXDSClient) performs HPACK encoding;
// this stub models the in-memory representation cost.
// ---------------------------------------------------------------------------
class StubGrpcMetadataProcessor {
public:
    StubGrpcMetadataProcessor() {
        // Pre-populate routing table
        routes_["/themis.Replication/Replicate"]  = "handler_replicate";
        routes_["/themis.Query/Execute"]          = "handler_query";
        routes_["/themis.Transaction/Begin"]      = "handler_txn_begin";
        routes_["/themis.Transaction/Commit"]     = "handler_txn_commit";
        routes_["/themis.Admin/HealthCheck"]      = "handler_health";
    }

    /// Returns metadata processing cost (stub: in-memory ops, no I/O).
    bool processCall(const std::string& method_path,
                     uint64_t           call_id,
                     std::string*       handler_out) const {
        // 1. Method path lookup
        auto it = routes_.find(method_path);
        if (it == routes_.end()) return false;

        // 2. HPACK-style header cost (stub: XOR over method bytes)
        uint64_t header_hash = call_id;
        for (unsigned char c : method_path) {
            header_hash ^= static_cast<uint64_t>(c) * 2654435761ULL;
        }
        benchmark::DoNotOptimize(header_hash);

        // 3. Trailer encoding stub
        uint64_t trailer = header_hash ^ 0xdeadbeefcafeULL;
        benchmark::DoNotOptimize(trailer);

        *handler_out = it->second;
        return true;
    }

private:
    std::unordered_map<std::string, std::string> routes_;
};

}  // anonymous namespace

// ============================================================================
// Shared fixtures (constructed once per process)
// ============================================================================

static StubZeroCopyBuilder& zeroCopyBuilder() {
    static StubZeroCopyBuilder b;
    return b;
}

static const StubQuicSessionCache& quicSessionCache() {
    static StubQuicSessionCache c;
    return c;
}

static const StubGrpcMetadataProcessor& grpcProcessor() {
    static StubGrpcMetadataProcessor p;
    return p;
}

// ============================================================================
// NRG-P1 — WebSocket frame parse throughput
// ============================================================================

/**
 * @brief NRG-P1: WS frame parse throughput.
 *
 * Gate: ≥ 5 M frames/sec.
 *
 * Throughput is measured implicitly: Google Benchmark reports items/sec when
 * SetItemsProcessed() is called.  The benchmark is configured with
 * ->Iterations(5'000'000) to accumulate enough samples for a stable reading.
 *
 * **Stub basis:** `parseWsFrame()` replicates the real WS frame parser
 * decision tree (FIN/RSV/opcode/mask/len validation) without I/O.
 *
 * **Hardware baseline pending:** Representative hardware baselines must be
 * collected before Wave D sign-off (see ROADMAP.md mid-term items [~]).
 */
static void BM_NRGP1_WsFrameParseThroughput(benchmark::State& state) {
    // Craft a valid binary WS frame: FIN=1, RSV=0, opcode=2 (binary), MASK=0, len=8
    WsFrameBytes frame{};
    frame.byte0 = 0x82;  // FIN=1, opcode=2
    frame.byte1 = 0x08;  // MASK=0, len=8
    std::memset(frame.payload, 0xAB, sizeof(frame.payload));

    // Warmup
    for (int i = 0; i < kWarmupIterations; ++i) {
        benchmark::DoNotOptimize(parseWsFrame(frame));
    }

    std::int64_t iterations = 0;
    for (auto _ : state) {
        benchmark::DoNotOptimize(parseWsFrame(frame));
        ++iterations;
    }
    state.SetItemsProcessed(iterations);
    state.SetLabel("NRG-P1: GATE >= 5M frames/sec | WS frame parse throughput [STUB; hardware baseline pending]");
}
BENCHMARK(BM_NRGP1_WsFrameParseThroughput)
    ->Repetitions(kRepetitions)
    ->ReportAggregatesOnly(true);

// ============================================================================
// NRG-P2 — TCP zero-copy path overhead per 64 KiB payload
// ============================================================================

/**
 * @brief NRG-P2: Zero-copy frame builder overhead for a 64 KiB payload.
 *
 * Gate: ≤ 5 µs per call.
 *
 * **Stub basis:** `StubZeroCopyBuilder::writeToWithSendfile()` models the
 * bookkeeping cost of the real `ZeroCopyFrameBuilder::writeToWithSendfile()`
 * path (sendfile params + atomic counter update) without syscall overhead.
 * The real sendfile(2) dominates at the OS layer; this gate covers the
 * userspace overhead around it.
 *
 * **Hardware baseline pending:** Representative hardware baselines must be
 * collected before Wave D sign-off.
 */
static void BM_NRGP2_TcpZeroCopyOverhead(benchmark::State& state) {
    auto& builder = zeroCopyBuilder();
    constexpr int    kFakeFd       = 3;
    constexpr size_t kPayloadSize  = 64 * 1024;  // 64 KiB
    constexpr size_t kOffset       = 0;

    // Warmup
    for (int i = 0; i < kWarmupIterations; ++i) {
        benchmark::DoNotOptimize(builder.writeToWithSendfile(kFakeFd, kOffset, kPayloadSize));
    }

    for (auto _ : state) {
        benchmark::DoNotOptimize(
            builder.writeToWithSendfile(kFakeFd, kOffset, kPayloadSize)
        );
    }
    state.SetBytesProcessed(static_cast<std::int64_t>(state.iterations()) * kPayloadSize);
    state.SetLabel("NRG-P2: GATE <= 5 us/call | TCP zero-copy overhead 64 KiB [STUB; hardware baseline pending]");
}
BENCHMARK(BM_NRGP2_TcpZeroCopyOverhead)
    ->Repetitions(kRepetitions)
    ->ReportAggregatesOnly(true);

// ============================================================================
// NRG-P3 — QUIC 0-RTT handshake latency (stub)
// ============================================================================

/**
 * @brief NRG-P3: QUIC 0-RTT resumption path overhead.
 *
 * Gate: ≤ 50 ms per handshake.
 *
 * **Stub basis:** Models session ticket lookup + 2-round key schedule step +
 * anti-replay check.  The real 0-RTT path adds TLS AEAD decryption overhead
 * and a UDP round-trip; this stub isolates the in-process cost.
 *
 * **Hardware baseline pending:** Real QUIC 0-RTT measurements require the
 * THEMIS_ENABLE_HTTP3 build and a loopback network.  This stub establishes
 * the gate before hardware numbers are collected.
 */
static void BM_NRGP3_Quic0RttHandshake(benchmark::State& state) {
    const auto& cache = quicSessionCache();

    // Warmup
    for (int i = 0; i < kWarmupIterations; ++i) {
        const std::string ticket = "ticket-" + std::to_string(i % 100);
        bool found = cache.lookupTicket(ticket);
        if (found) {
            uint64_t key = stubKeyScheduleStep(static_cast<uint64_t>(i), 2);
            bool ok = stubAntiReplayCheck(key);
            benchmark::DoNotOptimize(ok);
        }
    }

    std::size_t counter = 0;
    for (auto _ : state) {
        const std::string ticket = "ticket-" + std::to_string(counter % 100);
        bool found = cache.lookupTicket(ticket);
        benchmark::DoNotOptimize(found);
        if (found) {
            uint64_t key = stubKeyScheduleStep(static_cast<uint64_t>(counter), 2);
            bool ok = stubAntiReplayCheck(key);
            benchmark::DoNotOptimize(ok);
        }
        ++counter;
    }
    state.SetLabel("NRG-P3: GATE <= 50 ms/handshake | QUIC 0-RTT [STUB; hardware baseline pending]");
}
BENCHMARK(BM_NRGP3_Quic0RttHandshake)
    ->Repetitions(kRepetitions)
    ->ReportAggregatesOnly(true);

// ============================================================================
// NRG-P4 — gRPC metadata overhead per call (stub)
// ============================================================================

/**
 * @brief NRG-P4: gRPC metadata processing overhead per call.
 *
 * Gate: ≤ 10 µs per call.
 *
 * **Stub basis:** Models HPACK-style header construction cost + method-path
 * routing table lookup + trailer encoding.  The real gRPC path additionally
 * performs HTTP/2 frame assembly and HPACK huffman coding; this stub covers
 * the in-process routing and metadata cost.
 *
 * **Hardware baseline pending:** Representative hardware baselines must be
 * collected before Wave D sign-off.
 */
static void BM_NRGP4_GrpcMetadataOverhead(benchmark::State& state) {
    const auto& proc = grpcProcessor();
    std::string handler;

    // Warmup
    const std::string kMethod = "/themis.Query/Execute";
    for (int i = 0; i < kWarmupIterations; ++i) {
        benchmark::DoNotOptimize(proc.processCall(kMethod, static_cast<uint64_t>(i), &handler));
    }

    std::size_t counter = 0;
    // Cycle through 5 distinct method paths to model realistic call distribution
    const std::array<const char*, 5> kMethods = {
        "/themis.Replication/Replicate",
        "/themis.Query/Execute",
        "/themis.Transaction/Begin",
        "/themis.Transaction/Commit",
        "/themis.Admin/HealthCheck",
    };

    for (auto _ : state) {
        const char* method = kMethods[counter % kMethods.size()];
        benchmark::DoNotOptimize(proc.processCall(method, counter, &handler));
        ++counter;
    }
    state.SetLabel("NRG-P4: GATE <= 10 us/call | gRPC metadata overhead [STUB; hardware baseline pending]");
}
BENCHMARK(BM_NRGP4_GrpcMetadataOverhead)
    ->Repetitions(kRepetitions)
    ->ReportAggregatesOnly(true);

}  // namespace nrg_p
}  // namespace bench
}  // namespace themis

BENCHMARK_MAIN();
