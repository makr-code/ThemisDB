/**
 * @file bench_stream_protocol.cpp
 * @brief Stream-Protocol Benchmarks – Wire Protocol V2 (Issue #5, reactivated v1.9.0)
 *
 * Benchmark cases:
 *   SP-1  BM_StreamProtocol_FrameHeaderBuild  – V2FrameHeader construction and hot-path
 *                                               field inspection (is_valid / get_type /
 *                                               has_flag).  Parameterised by logical
 *                                               payload size (128 B – 64 KiB).
 *                                               Target: > 50 M ops/s
 *
 *   SP-2  BM_StreamProtocol_LZ4Roundtrip      – LZ4 compress+decompress a stream payload.
 *                                               Parameterised by payload size
 *                                               (1 KiB – 1 MiB).
 *                                               Target P95: < 1 ms @ 16 KiB
 *
 *   SP-3  BM_StreamProtocol_MetricsSnapshot   – WireProtocolMetrics: record N latency
 *                                               samples, then call snapshot() which
 *                                               computes P95/P99 via sorted-window.
 *                                               Parameterised by sample-window size
 *                                               (100 – 10 000).
 *                                               Target P99 (snapshot call): < 5 ms @ 10k
 *
 *   SP-4  BM_StreamProtocol_BufferPoolRoundtrip – PayloadBufferPool acquire / fill /
 *                                                release cycle.  Parameterised by slab
 *                                                size (1 KiB – 64 KiB).
 *                                                Target: > 1 M ops/s
 *
 * Performance expectations are documented in PERFORMANCE_EXPECTATIONS.md §21 (SP-1..SP-3).
 *
 * Build:
 *   cmake -DTHEMIS_BUILD_BENCHMARKS=ON ... && cmake --build . --target bench_stream_protocol
 * Run:
 *   ./bench_stream_protocol --benchmark_format=json
 */

#include <benchmark/benchmark.h>

#include "network/connection_compression.h"
#include "network/wire_protocol_performance.h"
#include "themis/network/wire_protocol_v2.hpp"

#include <cstdint>
#include <string>
#include <vector>

// ─── helpers ──────────────────────────────────────────────────────────────────

namespace {

/// Build a pseudo-random but compressible payload of @p size bytes.
/// The pattern repeats every 32 bytes with a per-byte deterministic value so
/// that LZ4 / Zstd achieve a real compression gain.
static std::vector<uint8_t> makeCompressiblePayload(size_t size) {
    std::vector<uint8_t> buf(size);
    for (size_t i = 0; i < size; ++i) {
        buf[i] = static_cast<uint8_t>((i % 32) * 7 + (i / 32) % 11);
    }
    return buf;
}

} // namespace

// =============================================================================
// SP-1: V2FrameHeader build + hot-path inspection
// =============================================================================

/**
 * Constructs a DATA frame header, then exercises the three hot-path predicates
 * (is_valid, get_type, has_flag) that every session handler invokes per frame.
 *
 * A HEADERS frame is also built and inspected so that the branch predictor
 * sees non-trivial variety.  The payload_length field is driven by the
 * benchmark parameter so that different sizes are exercised.
 */
static void BM_StreamProtocol_FrameHeaderBuild(benchmark::State& state) {
    const auto payload_len = static_cast<uint32_t>(state.range(0));
    uint32_t stream_id     = 1;

    for (auto _ : state) {
        // DATA frame
        themis::wire::V2FrameHeader data_hdr{};
        data_hdr.magic          = themis::wire::WIRE_V2_MAGIC;
        data_hdr.version        = themis::wire::WIRE_VERSION_2;
        data_hdr.frame_type     = static_cast<uint8_t>(themis::wire::V2FrameType::DATA);
        data_hdr.flags          = static_cast<uint16_t>(themis::wire::V2FrameFlags::END_STREAM);
        data_hdr.stream_id      = stream_id;
        data_hdr.payload_length = payload_len;

        benchmark::DoNotOptimize(data_hdr.is_valid());
        benchmark::DoNotOptimize(data_hdr.get_type());
        benchmark::DoNotOptimize(
            data_hdr.has_flag(themis::wire::V2FrameFlags::END_STREAM));

        // HEADERS frame (new stream open)
        themis::wire::V2FrameHeader hdr_hdr{};
        hdr_hdr.magic          = themis::wire::WIRE_V2_MAGIC;
        hdr_hdr.version        = themis::wire::WIRE_VERSION_2;
        hdr_hdr.frame_type     = static_cast<uint8_t>(themis::wire::V2FrameType::HEADERS);
        hdr_hdr.flags          = static_cast<uint16_t>(themis::wire::V2FrameFlags::END_HEADERS);
        hdr_hdr.stream_id      = stream_id + 2;
        hdr_hdr.payload_length = 64;

        benchmark::DoNotOptimize(hdr_hdr.is_valid());
        benchmark::DoNotOptimize(hdr_hdr.get_type());
        benchmark::DoNotOptimize(
            hdr_hdr.has_flag(themis::wire::V2FrameFlags::END_HEADERS));

        benchmark::ClobberMemory();
        stream_id += 4; // keep client-originated odd stream IDs
    }

    state.SetItemsProcessed(state.iterations() * 2); // 2 headers per iteration
    state.SetLabel("V2FrameHeader DATA+HEADERS inspect");
}
// Range covers typical frame payload sizes: 128 B, 1 KiB, 16 KiB, 64 KiB
BENCHMARK(BM_StreamProtocol_FrameHeaderBuild)->RangeMultiplier(8)->Range(128, 65536);

// =============================================================================
// SP-2: LZ4 stream-payload compression roundtrip
// =============================================================================

/**
 * Compresses a realistically patterned payload with LZ4 then immediately
 * decompresses it.  The parameterised size range covers the most common
 * stream frame sizes seen in practice (1 KiB → 1 MiB).
 *
 * SetBytesProcessed reports uncompressed throughput (MB/s) so results can be
 * compared against the NET-7 target in PERFORMANCE_EXPECTATIONS.md.
 */
static void BM_StreamProtocol_LZ4Roundtrip(benchmark::State& state) {
    const size_t payload_size = static_cast<size_t>(state.range(0));
    const std::vector<uint8_t> original = makeCompressiblePayload(payload_size);

    for (auto _ : state) {
        auto compressed = themis::network::compressLZ4(original, /*min_size=*/256);
        benchmark::DoNotOptimize(compressed);

        if (!compressed.empty()) {
            auto restored = themis::network::decompressLZ4(compressed);
            benchmark::DoNotOptimize(restored.data());
        }

        benchmark::ClobberMemory();
    }

    state.SetBytesProcessed(
        state.iterations() * static_cast<int64_t>(payload_size));
    state.SetLabel("LZ4 compress+decompress");
}
BENCHMARK(BM_StreamProtocol_LZ4Roundtrip)
    ->RangeMultiplier(16)
    ->Range(1024, 1024 * 1024);

// =============================================================================
// SP-3: WireProtocolMetrics snapshot (P95/P99 computation)
// =============================================================================

/**
 * Records N latency samples into a WireProtocolMetrics instance (simulating
 * per-stream telemetry recording), then benchmarks the snapshot() call that
 * sorts the window and computes P50/P95/P99/P999.
 *
 * The benchmark loop only measures snapshot(), which is the latency-critical
 * reporting path; the pre-fill is done in the setup phase outside the loop.
 */
static void BM_StreamProtocol_MetricsSnapshot(benchmark::State& state) {
    const int n_samples = static_cast<int>(state.range(0));

    themis::network::WireProtocolMetrics::Config cfg;
    cfg.max_samples = static_cast<size_t>(n_samples);
    themis::network::WireProtocolMetrics metrics(cfg);

    // Pre-fill with a realistic latency distribution (0.1 ms – 2.6 ms)
    for (int i = 0; i < n_samples; ++i) {
        double ms = 0.1 + static_cast<double>(i % 50) * 0.05;
        metrics.recordLatencyMs(ms);
        metrics.recordBytes(/*rx=*/static_cast<uint64_t>(128 + (i % 4) * 128),
                            /*tx=*/64);
    }

    for (auto _ : state) {
        auto snap = metrics.snapshot();
        benchmark::DoNotOptimize(snap.latency.p95_ms);
        benchmark::DoNotOptimize(snap.latency.p99_ms);
        benchmark::DoNotOptimize(snap.throughput.requests_total);
    }

    // Surface the measured P99 as a label for the benchmark report
    auto final_snap = metrics.snapshot();
    state.SetLabel(
        "snapshot p95=" + std::to_string(final_snap.latency.p95_ms) +
        "ms p99=" + std::to_string(final_snap.latency.p99_ms) + "ms");
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_StreamProtocol_MetricsSnapshot)
    ->RangeMultiplier(10)
    ->Range(100, 10000);

// =============================================================================
// SP-4: PayloadBufferPool acquire / fill / release
// =============================================================================

/**
 * Exercises the slab-allocator hot path: acquire a buffer from the pool,
 * write a pattern into it (simulating frame serialisation), then release it
 * back to the pool via RAII Handle destruction.
 *
 * After warmup iterations the pool should reach a steady-state hit rate of
 * ~100 %, so the benchmark reflects pooled-buffer throughput rather than
 * heap-allocation cost.
 */
static void BM_StreamProtocol_BufferPoolRoundtrip(benchmark::State& state) {
    const size_t slab_size  = static_cast<size_t>(state.range(0));
    const size_t pool_depth = 32;

    themis::network::PayloadBufferPool pool(slab_size, pool_depth);

    // Warm up the pool so all iterations use pooled slabs
    for (size_t w = 0; w < pool_depth; ++w) {
        auto h = pool.acquire();
        h->resize(slab_size);
        benchmark::DoNotOptimize(h->data());
    } // all handles return to pool here

    for (auto _ : state) {
        auto handle = pool.acquire();
        auto& buf   = *handle;
        buf.resize(slab_size);
        // Simulate filling a frame payload
        for (size_t i = 0; i < buf.size(); ++i) {
            buf[i] = static_cast<uint8_t>(i & 0xFFu);
        }
        benchmark::DoNotOptimize(buf.data());
        benchmark::ClobberMemory();
        // handle destructor returns buf to pool
    }

    state.SetItemsProcessed(state.iterations());
    state.SetLabel("acquire+fill+release slab=" + std::to_string(slab_size));
}
BENCHMARK(BM_StreamProtocol_BufferPoolRoundtrip)
    ->RangeMultiplier(4)
    ->Range(1024, 65536);
