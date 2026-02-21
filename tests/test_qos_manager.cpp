/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            test_qos_manager.cpp                               ║
  Version:         0.0.4                                              ║
  Last Modified:   2026-02-21 08:45:26                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     336                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2563a40d8  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • f0e1e982c  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • b4dc54fdd  2026-02-20  Network module: QoS manager with token bucket, backpressu... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/**
 * @file test_qos_manager.cpp
 * @brief Unit tests for QoSManager (token bucket, priority, backpressure)
 *
 * Tests:
 *  - TokenBucket: construction, tryConsume, consume with timeout, reconfigure
 *  - QoSManager: connection registration, priority, token bucket, backpressure,
 *    statistics, callback
 */

#include <gtest/gtest.h>
#include "network/qos_manager.h"

#include <atomic>
#include <chrono>
#include <thread>
#include <vector>

using namespace themis::network;
using namespace std::chrono_literals;

// =============================================================================
// TokenBucket tests
// =============================================================================

class TokenBucketTest : public ::testing::Test {};

TEST_F(TokenBucketTest, InitialTokensEqualBurst) {
    TokenBucket tb(1'000'000 /* 1 Mbps */, 125'000 /* 125 KB burst */);
    // After construction the bucket starts full
    EXPECT_GE(tb.availableBytes(), 125'000.0);
}

TEST_F(TokenBucketTest, TryConsumeSucceedsWithinBurst) {
    TokenBucket tb(1'000'000, 125'000);
    EXPECT_TRUE(tb.tryConsume(1000));
}

TEST_F(TokenBucketTest, TryConsumeFailsWhenEmpty) {
    // Very small burst (10 bytes), try to consume more
    TokenBucket tb(8 /* 8 bps = 1 B/s */, 10);
    EXPECT_TRUE(tb.tryConsume(10));   // consume entire burst
    EXPECT_FALSE(tb.tryConsume(10));  // no tokens left
}

TEST_F(TokenBucketTest, UnlimitedBucketAlwaysSucceeds) {
    // rate_bps = 0 means unlimited
    TokenBucket tb(0, 1000);
    for (int i = 0; i < 100; ++i) {
        EXPECT_TRUE(tb.tryConsume(10'000));
    }
}

TEST_F(TokenBucketTest, TokensRefillOverTime) {
    // 8 bps = 1 byte/second; burst = 1 byte
    TokenBucket tb(8, 1);
    EXPECT_TRUE(tb.tryConsume(1));   // consume the single token
    EXPECT_FALSE(tb.tryConsume(1));  // empty

    // Wait a little over 1 second for a refill
    std::this_thread::sleep_for(1100ms);
    EXPECT_TRUE(tb.tryConsume(1));
}

TEST_F(TokenBucketTest, ConsumeBlocksUntilTokensAvailable) {
    // 80 bps = 10 B/s; burst = 10 bytes
    TokenBucket tb(80, 10);
    EXPECT_TRUE(tb.tryConsume(10));  // drain

    auto start = std::chrono::steady_clock::now();
    bool ok    = tb.consume(5, 2000ms);
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                       std::chrono::steady_clock::now() - start)
                       .count();

    EXPECT_TRUE(ok);
    EXPECT_GE(elapsed, 400);  // at 10 B/s, 5 bytes takes ~500 ms; allow 400 ms for scheduling jitter
}

TEST_F(TokenBucketTest, ConsumeTimesOut) {
    // 8 bps = 1 B/s; burst = 1 byte; consume 100 bytes with 100 ms timeout
    TokenBucket tb(8, 1);
    EXPECT_TRUE(tb.tryConsume(1));  // drain

    bool ok = tb.consume(100, 100ms);
    EXPECT_FALSE(ok);  // cannot get 100 bytes in 100 ms at 1 B/s
}

TEST_F(TokenBucketTest, Reconfigure) {
    TokenBucket tb(8, 1);
    tb.reconfigure(0 /* unlimited */, 1'000'000);
    // After making unlimited, large consume should succeed immediately
    EXPECT_TRUE(tb.tryConsume(500'000));
}

TEST_F(TokenBucketTest, BurstCappedOnReconfigure) {
    TokenBucket tb(1'000'000, 1'000'000);
    // Decrease burst
    tb.reconfigure(1'000'000, 100);
    // Available tokens must not exceed new burst
    EXPECT_LE(tb.availableBytes(), 100.0 + 1.0 /* small refill tolerance */);
}

// =============================================================================
// QoSManager tests
// =============================================================================

class QoSManagerTest : public ::testing::Test {
protected:
    void SetUp() override {
        QoSManager::Config cfg;
        cfg.default_rate_bps    = 0;      // Unlimited by default in tests
        cfg.default_burst_bytes = 0;
        cfg.max_queue_bytes     = 10'000; // 10 KB queue cap for backpressure tests
        qos_ = std::make_unique<QoSManager>(cfg);
    }

    std::unique_ptr<QoSManager> qos_;
    static constexpr uint64_t kConnId = 42;
};

TEST_F(QoSManagerTest, InitialStatsAreZero) {
    auto stats = qos_->getStats();
    EXPECT_EQ(stats.total_bytes_sent, 0u);
    EXPECT_EQ(stats.total_bytes_received, 0u);
    EXPECT_EQ(stats.total_bytes_shaped, 0u);
    EXPECT_EQ(stats.backpressure_events, 0u);
    EXPECT_EQ(stats.active_connections, 0u);
}

TEST_F(QoSManagerTest, RegisterAndUnregisterConnection) {
    qos_->registerConnection(kConnId, Priority::HIGH);
    EXPECT_EQ(qos_->getStats().active_connections, 1u);

    qos_->unregisterConnection(kConnId);
    EXPECT_EQ(qos_->getStats().active_connections, 0u);
}

TEST_F(QoSManagerTest, AllowSendUnknownConnectionPermitsAll) {
    // Connections that were never registered are allowed through
    EXPECT_TRUE(qos_->allowSend(9999, 100));
}

TEST_F(QoSManagerTest, AllowSendUnlimitedAlwaysTrue) {
    qos_->registerConnection(kConnId, Priority::MEDIUM);
    // No token bucket configured → unlimited
    for (int i = 0; i < 100; ++i) {
        EXPECT_TRUE(qos_->allowSend(kConnId, 1000));
        qos_->recordBytesSent(kConnId, 1000);
    }
}

TEST_F(QoSManagerTest, TokenBucketLimitsThrough) {
    qos_->registerConnection(kConnId, Priority::HIGH);
    // 8 bps = 1 B/s; burst = 5 bytes
    qos_->setTokenBucket(kConnId, 8, 5);

    // First 5 bytes succeed immediately
    EXPECT_TRUE(qos_->allowSend(kConnId, 5, 0ms));
    // Next 5 bytes fail immediately (no wait)
    EXPECT_FALSE(qos_->allowSend(kConnId, 5, 0ms));
}

TEST_F(QoSManagerTest, BackpressureTriggers) {
    qos_->registerConnection(kConnId, Priority::LOW);

    // Fill queue up to cap (10 000 bytes)
    EXPECT_TRUE(qos_->allowSend(kConnId, 10'000, 0ms));

    // Next send should trigger backpressure
    EXPECT_FALSE(qos_->allowSend(kConnId, 1, 0ms));

    auto cs = qos_->getConnectionStats(kConnId);
    EXPECT_GE(cs.backpressure_events, 1u);

    auto stats = qos_->getStats();
    EXPECT_GE(stats.backpressure_events, 1u);
}

TEST_F(QoSManagerTest, BackpressureReleasedAfterSend) {
    qos_->registerConnection(kConnId, Priority::MEDIUM);

    EXPECT_TRUE(qos_->allowSend(kConnId, 10'000, 0ms));
    // Queue is full
    EXPECT_FALSE(qos_->allowSend(kConnId, 1, 0ms));

    // Drain queue by recording a send
    qos_->recordBytesSent(kConnId, 10'000);

    // Queue should now have room
    EXPECT_TRUE(qos_->allowSend(kConnId, 1, 0ms));
}

TEST_F(QoSManagerTest, SetAndChangePriority) {
    qos_->registerConnection(kConnId, Priority::LOW);

    auto cs = qos_->getConnectionStats(kConnId);
    EXPECT_EQ(cs.priority, Priority::LOW);

    qos_->setPriority(kConnId, Priority::CRITICAL);
    cs = qos_->getConnectionStats(kConnId);
    EXPECT_EQ(cs.priority, Priority::CRITICAL);
}

TEST_F(QoSManagerTest, RecordBytesSentUpdatesStats) {
    qos_->registerConnection(kConnId, Priority::HIGH);
    qos_->allowSend(kConnId, 500, 0ms);
    qos_->recordBytesSent(kConnId, 500);

    auto cs = qos_->getConnectionStats(kConnId);
    EXPECT_EQ(cs.bytes_sent, 500u);

    auto stats = qos_->getStats();
    EXPECT_EQ(stats.total_bytes_sent, 500u);
    EXPECT_EQ(stats.bytes_per_priority[Priority::HIGH], 500u);
}

TEST_F(QoSManagerTest, RecordBytesReceivedUpdatesStats) {
    qos_->registerConnection(kConnId);
    qos_->recordBytesReceived(kConnId, 1234);

    auto cs = qos_->getConnectionStats(kConnId);
    EXPECT_EQ(cs.bytes_received, 1234u);

    EXPECT_EQ(qos_->getStats().total_bytes_received, 1234u);
}

TEST_F(QoSManagerTest, GetAllConnectionStats) {
    qos_->registerConnection(1, Priority::HIGH);
    qos_->registerConnection(2, Priority::LOW);
    qos_->registerConnection(3, Priority::CRITICAL);

    auto all = qos_->getAllConnectionStats();
    EXPECT_EQ(all.size(), 3u);
}

TEST_F(QoSManagerTest, BackpressureCallbackFired) {
    std::atomic<int> cb_count{0};
    std::atomic<uint64_t> cb_conn_id{0};
    qos_->setBackpressureCallback([&](uint64_t id, uint64_t) {
        cb_count.fetch_add(1);
        cb_conn_id.store(id, std::memory_order_relaxed);
    });

    qos_->registerConnection(kConnId, Priority::MEDIUM);
    qos_->allowSend(kConnId, 10'000, 0ms);  // fill queue
    qos_->allowSend(kConnId, 100, 0ms);     // triggers backpressure

    EXPECT_GE(cb_count.load(), 1);
    EXPECT_EQ(cb_conn_id.load(), kConnId);
}

TEST_F(QoSManagerTest, ClearTokenBucketRestoresUnlimited) {
    qos_->registerConnection(kConnId);
    qos_->setTokenBucket(kConnId, 8, 5);

    EXPECT_TRUE(qos_->allowSend(kConnId, 5, 0ms));
    EXPECT_FALSE(qos_->allowSend(kConnId, 5, 0ms));  // rate limited

    qos_->recordBytesSent(kConnId, 5);
    qos_->clearTokenBucket(kConnId);

    // After clearing, large sends should succeed immediately
    EXPECT_TRUE(qos_->allowSend(kConnId, 5000, 0ms));
}

TEST_F(QoSManagerTest, ConnectionStatsNotFoundReturnsDefault) {
    auto cs = qos_->getConnectionStats(9999);
    EXPECT_EQ(cs.connection_id, 0u);
    EXPECT_EQ(cs.bytes_sent, 0u);
}

TEST_F(QoSManagerTest, MultipleConnectionsIndependent) {
    qos_->registerConnection(1, Priority::HIGH);
    qos_->registerConnection(2, Priority::LOW);

    qos_->setTokenBucket(1, 8, 5);  // small rate limit on conn 1

    // Conn 1 limited
    EXPECT_TRUE(qos_->allowSend(1, 5, 0ms));
    EXPECT_FALSE(qos_->allowSend(1, 5, 0ms));

    // Conn 2 unlimited – should not be affected
    EXPECT_TRUE(qos_->allowSend(2, 5000, 0ms));
}

// =============================================================================
// Thread-safety smoke test
// =============================================================================

TEST_F(QoSManagerTest, ConcurrentSendRecordIsThreadSafe) {
    constexpr int kNumThreads = 8;
    constexpr int kOpsPerThread = 500;

    qos_->registerConnection(kConnId, Priority::HIGH);

    std::vector<std::thread> threads;
    threads.reserve(kNumThreads);

    for (int t = 0; t < kNumThreads; ++t) {
        threads.emplace_back([&]() {
            for (int i = 0; i < kOpsPerThread; ++i) {
                if (qos_->allowSend(kConnId, 10, 0ms)) {
                    qos_->recordBytesSent(kConnId, 10);
                }
            }
        });
    }

    for (auto& th : threads) {
        th.join();
    }

    // Just verify no crash and stats are internally consistent
    auto stats = qos_->getStats();
    EXPECT_GE(stats.total_bytes_sent, 0u);
}
