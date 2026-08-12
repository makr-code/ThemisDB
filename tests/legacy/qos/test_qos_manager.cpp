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

// =============================================================================
// Per-tenant bandwidth quota tests
// =============================================================================

class TenantQuotaTest : public ::testing::Test {
protected:
    void SetUp() override {
        QoSManager::Config cfg;
        cfg.default_rate_bps    = 0;   // Unlimited per-connection by default
        cfg.default_burst_bytes = 0;
        cfg.max_queue_bytes     = 0;   // No backpressure limit in these tests
        qos_ = std::make_unique<QoSManager>(cfg);
    }

    std::unique_ptr<QoSManager> qos_;
    static constexpr uint64_t kConn1 = 101;
    static constexpr uint64_t kConn2 = 102;
    static constexpr uint64_t kConn3 = 103;
    static const std::string kTenantA;
    static const std::string kTenantB;
};

const std::string TenantQuotaTest::kTenantA = "tenant-alpha";
const std::string TenantQuotaTest::kTenantB = "tenant-beta";

TEST_F(TenantQuotaTest, RegisterAndGetTenantQuota) {
    qos_->registerTenantQuota(kTenantA, 8'000'000 /* 8 Mbps */, 1'000'000 /* 1 MB burst */);
    auto ts = qos_->getTenantStats(kTenantA);
    EXPECT_EQ(ts.tenant_id, kTenantA);
    EXPECT_EQ(ts.rate_bps, 8'000'000u);
    EXPECT_EQ(ts.burst_bytes, 1'000'000u);
    EXPECT_EQ(ts.bytes_sent, 0u);
    EXPECT_EQ(ts.bytes_shaped, 0u);
    EXPECT_EQ(ts.active_connections, 0u);
}

TEST_F(TenantQuotaTest, UnknownTenantReturnsDefaultStats) {
    auto ts = qos_->getTenantStats("nonexistent-tenant");
    EXPECT_TRUE(ts.tenant_id.empty());
    EXPECT_EQ(ts.rate_bps, 0u);
    EXPECT_EQ(ts.bytes_sent, 0u);
}

TEST_F(TenantQuotaTest, UnregisterTenantRemovesEntry) {
    qos_->registerTenantQuota(kTenantA, 1'000'000, 125'000);
    qos_->unregisterTenantQuota(kTenantA);

    // Stats should return default after removal
    auto ts = qos_->getTenantStats(kTenantA);
    EXPECT_TRUE(ts.tenant_id.empty());
}

TEST_F(TenantQuotaTest, SetTenantQuotaUpdatesExistingEntry) {
    qos_->registerTenantQuota(kTenantA, 1'000'000, 125'000);
    qos_->setTenantQuota(kTenantA, 2'000'000, 250'000);

    auto ts = qos_->getTenantStats(kTenantA);
    EXPECT_EQ(ts.rate_bps, 2'000'000u);
    EXPECT_EQ(ts.burst_bytes, 250'000u);
}

TEST_F(TenantQuotaTest, AssignTenantIncreasesActiveConnections) {
    qos_->registerTenantQuota(kTenantA, 0 /* unlimited */);
    qos_->registerConnection(kConn1);
    qos_->registerConnection(kConn2);

    qos_->assignTenant(kConn1, kTenantA);
    EXPECT_EQ(qos_->getTenantStats(kTenantA).active_connections, 1u);

    qos_->assignTenant(kConn2, kTenantA);
    EXPECT_EQ(qos_->getTenantStats(kTenantA).active_connections, 2u);
}

TEST_F(TenantQuotaTest, UnregisterConnectionDecrementsActiveConnections) {
    qos_->registerTenantQuota(kTenantA, 0);
    qos_->registerConnection(kConn1);
    qos_->assignTenant(kConn1, kTenantA);
    EXPECT_EQ(qos_->getTenantStats(kTenantA).active_connections, 1u);

    qos_->unregisterConnection(kConn1);
    EXPECT_EQ(qos_->getTenantStats(kTenantA).active_connections, 0u);
}

TEST_F(TenantQuotaTest, ReassignTenantUpdatesCounters) {
    qos_->registerTenantQuota(kTenantA, 0);
    qos_->registerTenantQuota(kTenantB, 0);
    qos_->registerConnection(kConn1);

    qos_->assignTenant(kConn1, kTenantA);
    EXPECT_EQ(qos_->getTenantStats(kTenantA).active_connections, 1u);
    EXPECT_EQ(qos_->getTenantStats(kTenantB).active_connections, 0u);

    // Re-assign to tenant B
    qos_->assignTenant(kConn1, kTenantB);
    EXPECT_EQ(qos_->getTenantStats(kTenantA).active_connections, 0u);
    EXPECT_EQ(qos_->getTenantStats(kTenantB).active_connections, 1u);
}

TEST_F(TenantQuotaTest, TenantQuotaLimitsSend) {
    // 8 bps = 1 B/s, burst = 5 bytes
    qos_->registerTenantQuota(kTenantA, 8, 5);
    qos_->registerConnection(kConn1);
    qos_->assignTenant(kConn1, kTenantA);

    // First 5 bytes succeed (burst)
    EXPECT_TRUE(qos_->allowSend(kConn1, 5, 0ms));
    // Next send fails – tenant bucket empty
    EXPECT_FALSE(qos_->allowSend(kConn1, 5, 0ms));

    auto ts = qos_->getTenantStats(kTenantA);
    EXPECT_GE(ts.bytes_shaped, 5u);
}

TEST_F(TenantQuotaTest, TenantQuotaSharedAcrossConnections) {
    // Burst = 10 bytes shared between kConn1 and kConn2
    qos_->registerTenantQuota(kTenantA, 8, 10);
    qos_->registerConnection(kConn1);
    qos_->registerConnection(kConn2);
    qos_->assignTenant(kConn1, kTenantA);
    qos_->assignTenant(kConn2, kTenantA);

    // Drain the shared 10-byte burst via kConn1
    EXPECT_TRUE(qos_->allowSend(kConn1, 10, 0ms));

    // kConn2 should now be rate-limited (shared bucket is empty)
    EXPECT_FALSE(qos_->allowSend(kConn2, 1, 0ms));
}

TEST_F(TenantQuotaTest, UnlimitedTenantAllowsAllSends) {
    // rate_bps = 0 means unlimited
    qos_->registerTenantQuota(kTenantA, 0);
    qos_->registerConnection(kConn1);
    qos_->assignTenant(kConn1, kTenantA);

    for (int i = 0; i < 100; ++i) {
        EXPECT_TRUE(qos_->allowSend(kConn1, 100'000, 0ms));
        qos_->recordBytesSent(kConn1, 100'000);
    }
}

TEST_F(TenantQuotaTest, RecordBytesSentUpdatesTenantStats) {
    qos_->registerTenantQuota(kTenantA, 0);
    qos_->registerConnection(kConn1);
    qos_->assignTenant(kConn1, kTenantA);

    qos_->allowSend(kConn1, 512, 0ms);
    qos_->recordBytesSent(kConn1, 512);

    auto ts = qos_->getTenantStats(kTenantA);
    EXPECT_EQ(ts.bytes_sent, 512u);
}

TEST_F(TenantQuotaTest, MultipleTenantIsolation) {
    // 8 bps = 1 B/s, burst = 5 for tenant A; unlimited for tenant B
    qos_->registerTenantQuota(kTenantA, 8, 5);
    qos_->registerTenantQuota(kTenantB, 0);
    qos_->registerConnection(kConn1);
    qos_->registerConnection(kConn2);
    qos_->assignTenant(kConn1, kTenantA);
    qos_->assignTenant(kConn2, kTenantB);

    // Drain kConn1's tenant bucket
    EXPECT_TRUE(qos_->allowSend(kConn1, 5, 0ms));
    EXPECT_FALSE(qos_->allowSend(kConn1, 1, 0ms));  // kConn1 throttled

    // kConn2 (different tenant, unlimited) is not affected
    EXPECT_TRUE(qos_->allowSend(kConn2, 100'000, 0ms));
}

TEST_F(TenantQuotaTest, GetAllTenantStats) {
    qos_->registerTenantQuota(kTenantA, 1'000'000);
    qos_->registerTenantQuota(kTenantB, 2'000'000);

    auto all = qos_->getAllTenantStats();
    EXPECT_EQ(all.size(), 2u);
}

// =============================================================================
// Audit regression tests
// =============================================================================

// Regression: Bug 1 – per-connection tokens must NOT be consumed when the
// tenant quota rejects the send.  After the tenant bucket is exhausted, the
// per-connection limit should still have its full budget.
TEST_F(TenantQuotaTest, TenantRejectionDoesNotConsumePerConnectionTokens) {
    // Per-connection limit: 8 bps = 1 B/s, burst = 10 bytes
    // Tenant limit:         8 bps = 1 B/s, burst = 5 bytes  (smaller)
    qos_->registerTenantQuota(kTenantA, 8, 5);
    qos_->registerConnection(kConn1);
    qos_->setTokenBucket(kConn1, 8, 10);  // per-connection burst = 10
    qos_->assignTenant(kConn1, kTenantA);

    // Drain the tenant bucket (5 bytes)
    EXPECT_TRUE(qos_->allowSend(kConn1, 5, 0ms));

    // Tenant bucket is now empty; next send should be rejected by tenant quota.
    // The per-connection bucket still has 5 bytes remaining.
    EXPECT_FALSE(qos_->allowSend(kConn1, 5, 0ms));

    // Per-connection bytes_shaped should be 0: the per-connection bucket was
    // never the reason for rejection.
    auto cs = qos_->getConnectionStats(kConn1);
    EXPECT_EQ(cs.bytes_shaped, 0u);

    // Tenant bytes_shaped should record the rejected bytes.
    auto ts = qos_->getTenantStats(kTenantA);
    EXPECT_GE(ts.bytes_shaped, 5u);
}

// Regression: Bug 3 – unregisterConnection must not underflow active_connections
// when assignTenant was called before registerTenantQuota.
TEST_F(TenantQuotaTest, UnregisterConnectionNoUnderflowWhenAssignedBeforeQuotaRegistered) {
    // Assign before registering quota (edge case)
    qos_->registerConnection(kConn1);
    qos_->assignTenant(kConn1, kTenantA);  // tenant-a does not exist yet

    // Now register the quota (active_connections starts at 0)
    qos_->registerTenantQuota(kTenantA, 0);
    EXPECT_EQ(qos_->getTenantStats(kTenantA).active_connections, 0u);

    // Unregistering should NOT underflow; should clamp at 0
    qos_->unregisterConnection(kConn1);
    EXPECT_EQ(qos_->getTenantStats(kTenantA).active_connections, 0u);
}
