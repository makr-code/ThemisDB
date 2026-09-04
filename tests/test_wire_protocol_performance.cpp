/// @file test_wire_protocol_performance.cpp
/// @brief Unit tests for Wire Protocol v1 performance components:
///        WireProtocolMetrics, PayloadBufferPool, CompressionAdvisor

#include <gtest/gtest.h>
#include "network/wire_protocol_performance.h"

#include <chrono>
#include <thread>
#include <vector>

using namespace themis::network;

// =============================================================================
// WireProtocolMetrics
// =============================================================================

TEST(WireProtocolMetrics, DefaultConstruction) {
    WireProtocolMetrics m;
    EXPECT_EQ(m.totalRequests(), 0u);
    EXPECT_EQ(m.totalErrors(),   0u);
}

TEST(WireProtocolMetrics, RecordSingleLatency) {
    WireProtocolMetrics m;
    m.recordLatencyMs(5.0);

    auto snap = m.snapshot();
    EXPECT_EQ(snap.latency.sample_count, 1u);
    EXPECT_DOUBLE_EQ(snap.latency.p50_ms, 5.0);
    EXPECT_DOUBLE_EQ(snap.latency.max_ms, 5.0);
}

TEST(WireProtocolMetrics, PercentilesMonotonicallyIncrease) {
    WireProtocolMetrics m;
    // Insert 100 samples: 1 ms, 2 ms, … 100 ms
    for (int i = 1; i <= 100; ++i)
        m.recordLatencyMs(static_cast<double>(i));

    auto snap = m.snapshot();
    EXPECT_LE(snap.latency.p50_ms,  snap.latency.p75_ms);
    EXPECT_LE(snap.latency.p75_ms,  snap.latency.p95_ms);
    EXPECT_LE(snap.latency.p95_ms,  snap.latency.p99_ms);
    EXPECT_LE(snap.latency.p99_ms,  snap.latency.p999_ms);
    EXPECT_LE(snap.latency.p999_ms, snap.latency.max_ms);
}

TEST(WireProtocolMetrics, RecordLatencyFromTimePoint) {
    WireProtocolMetrics m;
    auto t0 = std::chrono::steady_clock::now();
    std::this_thread::sleep_for(std::chrono::milliseconds(5));
    m.recordLatency(t0);

    auto snap = m.snapshot();
    EXPECT_EQ(snap.latency.sample_count, 1u);
    EXPECT_GE(snap.latency.max_ms, 5.0);
}

TEST(WireProtocolMetrics, RecordBytes) {
    WireProtocolMetrics m;
    m.recordBytes(100, 200);
    m.recordBytes(300, 400);

    auto snap = m.snapshot();
    EXPECT_EQ(snap.throughput.bytes_received_total, 400u);
    EXPECT_EQ(snap.throughput.bytes_sent_total,     600u);
}

TEST(WireProtocolMetrics, RecordCompression) {
    WireProtocolMetrics m;
    m.recordCompression(1000, 400);

    auto snap = m.snapshot();
    EXPECT_EQ(snap.throughput.compressed_payloads, 1u);
    EXPECT_NEAR(snap.throughput.compression_ratio, 0.4, 1e-6);
}

TEST(WireProtocolMetrics, RecordErrors) {
    WireProtocolMetrics m;
    m.recordLatencyMs(1.0); // register one request first
    m.recordError("connection");
    m.recordError("timeout");
    m.recordError("parse");
    m.recordError("auth");

    auto snap = m.snapshot();
    EXPECT_EQ(snap.errors.connection_errors, 1u);
    EXPECT_EQ(snap.errors.timeout_errors,    1u);
    EXPECT_EQ(snap.errors.parse_errors,      1u);
    EXPECT_EQ(snap.errors.auth_errors,       1u);
    EXPECT_EQ(m.totalErrors(), 4u);
    EXPECT_GT(snap.errors.error_rate, 0.0);
    EXPECT_LE(snap.errors.error_rate, 1.0);
}

TEST(WireProtocolMetrics, RecordUnknownErrorKind) {
    WireProtocolMetrics m;
    // Unknown kinds increment total but not any specific counter
    m.recordError("unknown_kind");
    EXPECT_EQ(m.totalErrors(), 1u);
}

TEST(WireProtocolMetrics, HistogramPopulated) {
    WireProtocolMetrics m;
    m.recordLatencyMs(3.0);
    m.recordLatencyMs(10.0);
    m.recordLatencyMs(100.0);

    auto snap = m.snapshot();
    EXPECT_FALSE(snap.latency_histogram.empty());

    // Sum of histogram counts should equal sample count
    uint64_t total = 0;
    for (const auto& [bucket, count] : snap.latency_histogram)
        total += count;
    EXPECT_EQ(total, snap.latency.sample_count);
}

TEST(WireProtocolMetrics, Reset) {
    WireProtocolMetrics m;
    m.recordLatencyMs(5.0);
    m.recordBytes(100, 200);
    m.recordError("connection");

    m.reset();

    EXPECT_EQ(m.totalRequests(), 0u);
    EXPECT_EQ(m.totalErrors(),   0u);
    auto snap = m.snapshot();
    EXPECT_EQ(snap.latency.sample_count, 0u);
    EXPECT_EQ(snap.throughput.bytes_received_total, 0u);
    EXPECT_EQ(snap.errors.connection_errors, 0u);
}

TEST(WireProtocolMetrics, SlidingWindowEvictsOldSamples) {
    WireProtocolMetrics::Config cfg;
    cfg.max_samples = 5;
    WireProtocolMetrics m(cfg);

    // First 5 samples: 100 ms each
    for (int i = 0; i < 5; ++i) {
      m.recordLatencyMs(100.0);
    }
    // Overwrite with 1 ms each
    for (int i = 0; i < 5; ++i) {
      m.recordLatencyMs(1.0);
    }

    auto snap = m.snapshot();
    EXPECT_EQ(snap.latency.sample_count, 5u);
    EXPECT_LE(snap.latency.max_ms, 2.0) << "Old high samples should be evicted";
}

// =============================================================================
// PayloadBufferPool
// =============================================================================

TEST(PayloadBufferPool, DefaultConstruction) {
    PayloadBufferPool pool;
    EXPECT_EQ(pool.slabSize(),  64u * 1024);
    EXPECT_EQ(pool.hitCount(),  0u);
    EXPECT_EQ(pool.missCount(), 0u);
}

TEST(PayloadBufferPool, AcquireReturnsClearedBuffer) {
    PayloadBufferPool pool(1024, 4);
    auto buf = pool.acquire();
    ASSERT_TRUE(static_cast<bool>(buf));
    EXPECT_TRUE(buf->empty());
    EXPECT_GE(buf->capacity(), 1024u);
}

TEST(PayloadBufferPool, FirstAcquireIsMiss) {
    PayloadBufferPool pool(256, 4);
    auto buf = pool.acquire();
    EXPECT_EQ(pool.missCount(), 1u);
    EXPECT_EQ(pool.hitCount(),  0u);
}

TEST(PayloadBufferPool, SecondAcquireIsHit) {
    PayloadBufferPool pool(256, 4);
    {
        auto buf = pool.acquire(); // miss
        (void)buf;
    } // returned to pool
    {
        auto buf2 = pool.acquire(); // hit
        EXPECT_EQ(pool.hitCount(), 1u);
    }
}

TEST(PayloadBufferPool, HitRateAfterReturnAndReuse) {
    PayloadBufferPool pool(256, 4);
    // Acquire and return 4 times
    for (int i = 0; i < 4; ++i) {
        auto buf = pool.acquire();
        (void)buf;
    }
    // Re-acquire 4 times (should all be hits)
    for (int i = 0; i < 4; ++i) {
        auto buf = pool.acquire();
        (void)buf;
    }
    EXPECT_GT(pool.hitRate(), 0.0);
}

TEST(PayloadBufferPool, BufferCanBeResized) {
    PayloadBufferPool pool(512, 4);
    auto buf = pool.acquire();
    buf->resize(128);
    EXPECT_EQ(buf->size(), 128u);
}

TEST(PayloadBufferPool, HandleMoveSemantics) {
    PayloadBufferPool pool(128, 2);
    auto h1 = pool.acquire();
    auto h2 = std::move(h1);
    EXPECT_FALSE(static_cast<bool>(h1)); // h1 should be empty after move
    EXPECT_TRUE(static_cast<bool>(h2));
}

TEST(PayloadBufferPool, PoolDepthBounded) {
    const size_t depth = 2;
    PayloadBufferPool pool(128, depth);

    // Acquire 5 buffers; all returned on scope exit
    {
        auto b1 = pool.acquire();
        auto b2 = pool.acquire();
        auto b3 = pool.acquire();
        auto b4 = pool.acquire();
        auto b5 = pool.acquire();
        (void)b1; (void)b2; (void)b3; (void)b4; (void)b5;
    }
    // Pool should hold at most `depth` slabs
    EXPECT_LE(pool.poolDepth(), depth);
}

// =============================================================================
// CompressionAdvisor
// =============================================================================

TEST(CompressionAdvisor, SmallPayloadSkipped) {
    CompressionAdvisor advisor;
    auto d = advisor.advise(100); // < 512 bytes
    EXPECT_EQ(d, CompressionAdvisor::Decision::SKIP);
}

TEST(CompressionAdvisor, MediumPayloadFast) {
    CompressionAdvisor advisor;
    // 1 KiB, above 512 B threshold, below 64 KiB
    auto d = advisor.advise(1024);
    EXPECT_EQ(d, CompressionAdvisor::Decision::LZ4_FAST);
}

TEST(CompressionAdvisor, LargePayloadHC) {
    CompressionAdvisor advisor;
    auto d = advisor.advise(128 * 1024); // > 64 KiB
    EXPECT_EQ(d, CompressionAdvisor::Decision::LZ4_HC);
}

TEST(CompressionAdvisor, SpeedModeSelectsFastX) {
    CompressionAdvisor::Config cfg;
    cfg.prefer_speed = true;
    CompressionAdvisor advisor(cfg);

    auto d_med   = advisor.advise(1024);
    auto d_large = advisor.advise(128 * 1024);
    EXPECT_EQ(d_med,   CompressionAdvisor::Decision::LZ4_FAST_X);
    EXPECT_EQ(d_large, CompressionAdvisor::Decision::LZ4_FAST_X);
}

TEST(CompressionAdvisor, Lz4AccelerationFastPositive) {
    CompressionAdvisor advisor;
    int acc = advisor.lz4Acceleration(CompressionAdvisor::Decision::LZ4_FAST);
    EXPECT_GT(acc, 0);
}

TEST(CompressionAdvisor, Lz4AccelerationFastXHigherThanFast) {
    CompressionAdvisor advisor;
    int fast   = advisor.lz4Acceleration(CompressionAdvisor::Decision::LZ4_FAST);
    int fast_x = advisor.lz4Acceleration(CompressionAdvisor::Decision::LZ4_FAST_X);
    EXPECT_GT(fast_x, fast);
}

TEST(CompressionAdvisor, Lz4AccelerationHcIsZero) {
    CompressionAdvisor advisor;
    // HC uses level, not acceleration
    EXPECT_EQ(advisor.lz4Acceleration(CompressionAdvisor::Decision::LZ4_HC), 0);
    EXPECT_EQ(advisor.lz4Acceleration(CompressionAdvisor::Decision::SKIP),   0);
}

TEST(CompressionAdvisor, HcLevelPositive) {
    CompressionAdvisor advisor;
    EXPECT_GT(advisor.lz4HcLevel(), 0);
}

TEST(CompressionAdvisor, CustomThresholds) {
    CompressionAdvisor::Config cfg;
    cfg.min_compressible_bytes = 2048;
    cfg.lz4_fast_threshold     = 1024 * 1024;
    CompressionAdvisor advisor(cfg);

    EXPECT_EQ(advisor.advise(1000),  CompressionAdvisor::Decision::SKIP);
    EXPECT_EQ(advisor.advise(4096),  CompressionAdvisor::Decision::LZ4_FAST);
    EXPECT_EQ(advisor.advise(2 * 1024 * 1024), CompressionAdvisor::Decision::LZ4_HC);
}

TEST(CompressionAdvisor, BoundaryAtThreshold) {
    // Exactly at min_compressible_bytes boundary
    CompressionAdvisor advisor;
    const auto& cfg = advisor.config();

    EXPECT_EQ(advisor.advise(cfg.min_compressible_bytes - 1),
              CompressionAdvisor::Decision::SKIP);
    EXPECT_EQ(advisor.advise(cfg.min_compressible_bytes),
              CompressionAdvisor::Decision::LZ4_FAST);
}
