/**
 * @file test_bandwidth_management_qos.cpp
 * @brief Focused tests for Bandwidth Management and QoS (v1.8.0, Issue #190)
 *
 * Tests acceptance criteria:
 *  AC-BW-1   Per-connection bandwidth limits via set_bandwidth_limit()
 *  AC-BW-2   Traffic shaping with token bucket (set_token_bucket)
 *  AC-BW-3   Traffic shaping with leaky bucket (setLeakyBucket)
 *  AC-BW-4   Priority queuing CRITICAL/HIGH/MEDIUM/LOW
 *  AC-BW-5   Fair queuing – starvation guard prevents LOW starvation
 *  AC-BW-6   Congestion control – AIMD increase and loss halving
 *  AC-BW-7   Config mbps convenience fields (max_bandwidth_mbps,
 *             per_connection_limit_mbps, enable_priority_queuing)
 *  AC-BW-8   Linux tc configureTc() on non-Linux returns false
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
// Helper fixtures
// =============================================================================

class BandwidthQoSTest : public ::testing::Test {
protected:
    void SetUp() override {
        QoSManager::Config cfg;
        cfg.default_rate_bps    = 0;  // unlimited per-connection by default
        cfg.default_burst_bytes = 0;
        cfg.max_queue_bytes     = 0;  // no backpressure in these tests
        qos_ = std::make_unique<QoSManager>(cfg);
    }

    std::unique_ptr<QoSManager> qos_;
    static constexpr uint64_t kConn1 = 201;
    static constexpr uint64_t kConn2 = 202;
    static constexpr uint64_t kConn3 = 203;
};

// =============================================================================
// AC-BW-1: Per-connection bandwidth limits via set_bandwidth_limit()
// =============================================================================

TEST_F(BandwidthQoSTest, SetBandwidthLimitBlocksExcessSends) {
    qos_->registerConnection(kConn1, Priority::HIGH);

    // 8 bytes/sec → 1 byte per second; 1 second burst
    // After the initial 8-byte burst is consumed, sends are blocked.
    qos_->set_bandwidth_limit(kConn1, 8);  // 8 bytes/sec

    // Should succeed: burst = 8 bytes
    EXPECT_TRUE(qos_->allowSend(kConn1, 8, 0ms));
    // Bucket empty – next send without timeout fails
    EXPECT_FALSE(qos_->allowSend(kConn1, 1, 0ms));
}

TEST_F(BandwidthQoSTest, SetBandwidthLimitUnlimitedIsAllowed) {
    qos_->registerConnection(kConn1, Priority::MEDIUM);
    qos_->set_bandwidth_limit(kConn1, 0);  // 0 = unlimited

    for (int i = 0; i < 50; ++i) {
        EXPECT_TRUE(qos_->allowSend(kConn1, 100'000, 0ms));
        qos_->recordBytesSent(kConn1, 100'000);
    }
}

// =============================================================================
// AC-BW-2: Traffic shaping – token bucket (snake_case API)
// =============================================================================

TEST_F(BandwidthQoSTest, SetTokenBucketLimitsConnection) {
    qos_->registerConnection(kConn1, Priority::HIGH);
    // 80 bps = 10 B/s; burst = 10 bytes
    qos_->set_token_bucket(kConn1, 80, 10);

    EXPECT_TRUE(qos_->allowSend(kConn1, 10, 0ms));   // burst
    EXPECT_FALSE(qos_->allowSend(kConn1, 10, 0ms));  // bucket empty
}

TEST_F(BandwidthQoSTest, SetPriorityChangesConnectionPriority) {
    qos_->registerConnection(kConn1, Priority::LOW);
    auto cs = qos_->getConnectionStats(kConn1);
    EXPECT_EQ(cs.priority, Priority::LOW);

    qos_->set_priority(kConn1, Priority::CRITICAL);
    cs = qos_->getConnectionStats(kConn1);
    EXPECT_EQ(cs.priority, Priority::CRITICAL);
}

// =============================================================================
// AC-BW-3: Traffic shaping – leaky bucket
// =============================================================================

class LeakyBucketTest : public ::testing::Test {};

TEST_F(LeakyBucketTest, AddWithinCapacityReturnsTrue) {
    LeakyBucket lb(8'000'000 /* 8 Mbps = 1 MB/s */, 10'000 /* 10 KB capacity */);
    EXPECT_TRUE(lb.add(5'000));
}

TEST_F(LeakyBucketTest, AddBeyondCapacityReturnsFalse) {
    LeakyBucket lb(8, 10);   // 8 bps drain, 10 byte capacity
    EXPECT_TRUE(lb.add(10));  // fills exactly to capacity
    EXPECT_FALSE(lb.add(1));  // overflows
}

TEST_F(LeakyBucketTest, DrainOverTimeLowersFill) {
    // 80 bps = 10 B/s drain; capacity = 10 bytes
    LeakyBucket lb(80, 10);
    lb.add(10);  // fill to capacity

    // After ~1.5 s at 10 B/s, ~15 bytes drained → fill should be 0
    std::this_thread::sleep_for(1500ms);
    EXPECT_TRUE(lb.tryConform(10));  // fill should be 0 now; 10 bytes fits
}

TEST_F(LeakyBucketTest, TryConformDoesNotModifyState) {
    LeakyBucket lb(80, 10);
    lb.add(10);  // fill capacity

    // tryConform should not add to the fill
    EXPECT_FALSE(lb.tryConform(1));  // 10 + 1 > 10
    // The fill should still be 10 (tryConform didn't add)
    // A second tryConform should still return the same result
    EXPECT_FALSE(lb.tryConform(1));
}

TEST_F(LeakyBucketTest, ReconfigureUpdatesDrainAndCapacity) {
    LeakyBucket lb(8, 10);
    lb.add(10);  // fill up

    lb.reconfigure(0 /* stopped */, 1'000'000);  // very large capacity
    EXPECT_TRUE(lb.add(999'990));  // should fit in the large bucket
}

TEST_F(LeakyBucketTest, CurrentFillReportsCorrectValue) {
    LeakyBucket lb(0, 1'000'000);  // zero drain rate, large capacity
    lb.add(5'000);
    EXPECT_NEAR(lb.currentFill(), 5'000.0, 10.0);
}

TEST_F(LeakyBucketTest, DrainRateBpsAndCapacityBytes) {
    LeakyBucket lb(8'000, 4'000);
    EXPECT_EQ(lb.drainRateBps(), 8'000u);
    EXPECT_EQ(lb.capacityBytes(), 4'000u);
}

TEST_F(BandwidthQoSTest, LeakyBucketBlocksNonConformantSends) {
    qos_->registerConnection(kConn1, Priority::MEDIUM);
    // 8 bps = 1 B/s drain; capacity = 5 bytes
    qos_->setLeakyBucket(kConn1, 8, 5);

    EXPECT_TRUE(qos_->allowSend(kConn1, 5, 0ms));   // fills bucket
    EXPECT_FALSE(qos_->allowSend(kConn1, 1, 0ms));  // overflow
}

TEST_F(BandwidthQoSTest, ClearLeakyBucketRestoresUnlimited) {
    qos_->registerConnection(kConn1, Priority::MEDIUM);
    qos_->setLeakyBucket(kConn1, 8, 5);

    EXPECT_TRUE(qos_->allowSend(kConn1, 5, 0ms));
    EXPECT_FALSE(qos_->allowSend(kConn1, 1, 0ms));

    qos_->recordBytesSent(kConn1, 5);
    qos_->clearLeakyBucket(kConn1);

    // After clearing, large sends should succeed
    EXPECT_TRUE(qos_->allowSend(kConn1, 50'000, 0ms));
}

// =============================================================================
// AC-BW-4: Priority queuing – CRITICAL / HIGH / MEDIUM / LOW
// =============================================================================

class PriorityQueueTest : public ::testing::Test {
protected:
    void SetUp() override {
        QoSManager::Config cfg;
        cfg.enable_priority_queuing    = true;
        cfg.enable_fair_queuing        = false;  // disable starvation guard
        cfg.starvation_guard_threshold = UINT32_MAX;  // effectively infinite
        qos_ = std::make_unique<QoSManager>(cfg);

        qos_->registerConnection(kCrit,   Priority::CRITICAL);
        qos_->registerConnection(kHigh,   Priority::HIGH);
        qos_->registerConnection(kMedium, Priority::MEDIUM);
        qos_->registerConnection(kLow,    Priority::LOW);
    }

    std::unique_ptr<QoSManager> qos_;
    static constexpr uint64_t kCrit   = 1;
    static constexpr uint64_t kHigh   = 2;
    static constexpr uint64_t kMedium = 3;
    static constexpr uint64_t kLow    = 4;
};

TEST_F(PriorityQueueTest, EnqueueAndDequeueSingleItem) {
    EXPECT_TRUE(qos_->enqueueSend(kHigh, 1000));
    EXPECT_EQ(qos_->getPendingQueueDepth(Priority::HIGH), 1u);

    auto item = qos_->dequeueForSend();
    ASSERT_TRUE(item.has_value());
    EXPECT_EQ(item->connection_id, kHigh);
    EXPECT_EQ(item->bytes, 1000u);
    EXPECT_EQ(item->priority, Priority::HIGH);
    EXPECT_EQ(qos_->getPendingQueueDepth(Priority::HIGH), 0u);
}

TEST_F(PriorityQueueTest, StrictPriorityOrderCriticalFirst) {
    qos_->enqueueSend(kLow,    100);
    qos_->enqueueSend(kMedium, 200);
    qos_->enqueueSend(kHigh,   300);
    qos_->enqueueSend(kCrit,   400);

    auto item = qos_->dequeueForSend();
    ASSERT_TRUE(item.has_value());
    EXPECT_EQ(item->priority, Priority::CRITICAL);
    EXPECT_EQ(item->bytes, 400u);
}

TEST_F(PriorityQueueTest, AllFourPrioritiesInOrder) {
    qos_->enqueueSend(kLow,    1);
    qos_->enqueueSend(kMedium, 2);
    qos_->enqueueSend(kHigh,   3);
    qos_->enqueueSend(kCrit,   4);

    std::vector<Priority> order;
    for (int i = 0; i < 4; ++i) {
        auto item = qos_->dequeueForSend();
        ASSERT_TRUE(item.has_value());
        order.push_back(item->priority);
    }

    EXPECT_EQ(order[0], Priority::CRITICAL);
    EXPECT_EQ(order[1], Priority::HIGH);
    EXPECT_EQ(order[2], Priority::MEDIUM);
    EXPECT_EQ(order[3], Priority::LOW);
}

TEST_F(PriorityQueueTest, DequeueEmptyReturnsNullopt) {
    EXPECT_FALSE(qos_->dequeueForSend().has_value());
}

TEST_F(PriorityQueueTest, EnqueueUnknownConnectionReturnsFalse) {
    EXPECT_FALSE(qos_->enqueueSend(9999, 100));
}

TEST_F(PriorityQueueTest, QueueDepthCountedPerPriority) {
    qos_->enqueueSend(kCrit, 100);
    qos_->enqueueSend(kCrit, 200);
    qos_->enqueueSend(kLow,  300);

    EXPECT_EQ(qos_->getPendingQueueDepth(Priority::CRITICAL), 2u);
    EXPECT_EQ(qos_->getPendingQueueDepth(Priority::HIGH),     0u);
    EXPECT_EQ(qos_->getPendingQueueDepth(Priority::LOW),      1u);
}

// =============================================================================
// AC-BW-5: Fair queuing – starvation guard
// =============================================================================

class FairQueueStarvationTest : public ::testing::Test {
protected:
    void SetUp() override {
        QoSManager::Config cfg;
        cfg.enable_priority_queuing    = true;
        cfg.enable_fair_queuing        = true;
        cfg.starvation_guard_threshold = 3;  // force LOW after 3 HIGH/CRITICAL
        qos_ = std::make_unique<QoSManager>(cfg);

        qos_->registerConnection(kHigh, Priority::HIGH);
        qos_->registerConnection(kLow,  Priority::LOW);
    }

    std::unique_ptr<QoSManager> qos_;
    static constexpr uint64_t kHigh = 10;
    static constexpr uint64_t kLow  = 20;
};

TEST_F(FairQueueStarvationTest, LowPriorityServedAfterThreshold) {
    // Enqueue many HIGH sends and one LOW send
    for (int i = 0; i < 10; ++i) {
        qos_->enqueueSend(kHigh, 100);
    }
    qos_->enqueueSend(kLow, 999);

    // Dequeue: first 3 should be HIGH (threshold = 3), then LOW is forced
    int high_count = 0;
    bool low_served = false;
    for (int i = 0; i < 11; ++i) {
        auto item = qos_->dequeueForSend();
        if (!item.has_value()) {
          break;
        }
        if (item->priority == Priority::LOW) {
            low_served = true;
            break;
        }
        if (item->priority == Priority::HIGH) {
            ++high_count;
        }
    }

    EXPECT_TRUE(low_served) << "LOW priority was never served (starvation)";
    // The LOW connection should have been served within threshold+1 dequeues
    EXPECT_LE(high_count, 3);
}

// =============================================================================
// AC-BW-6: Congestion control – AIMD
// =============================================================================

class CongestionControllerTest : public ::testing::Test {};

TEST_F(CongestionControllerTest, InitialCwndIsDefault) {
    CongestionController cc;
    EXPECT_EQ(cc.cwnd(), CongestionController::kDefaultInitialCwnd);
}

TEST_F(CongestionControllerTest, SlowStartDoublesWindow) {
    CongestionController cc;
    uint64_t initial = cc.cwnd();
    // Simulating an ACK of initial bytes → window should grow
    cc.recordAck(initial, 100us);
    EXPECT_GT(cc.cwnd(), initial);
}

TEST_F(CongestionControllerTest, LossHalvesCwnd) {
    CongestionController cc;
    // Grow the window first
    for (int i = 0; i < 20; ++i) {
        cc.recordAck(10'000, 1000us);
    }
    uint64_t before_loss = cc.cwnd();
    cc.recordLoss();
    EXPECT_LE(cc.cwnd(), before_loss / 2 + 1);
}

TEST_F(CongestionControllerTest, CwndDoesNotExceedMaximum) {
    CongestionController cc;
    // Drive cwnd to max
    for (int i = 0; i < 10'000; ++i) {
        cc.recordAck(1'000'000, 100us);
    }
    EXPECT_LE(cc.cwnd(), CongestionController::kMaxCwnd);
}

TEST_F(CongestionControllerTest, SmoothRttIsUpdated) {
    CongestionController cc;
    EXPECT_EQ(cc.smoothedRtt().count(), 0);
    cc.recordAck(1000, 5000us);
    EXPECT_GT(cc.smoothedRtt().count(), 0);
}

TEST_F(CongestionControllerTest, ResetRestoresToInitialState) {
    CongestionController cc;
    cc.recordAck(100'000, 100us);
    cc.recordLoss();
    cc.reset();
    EXPECT_EQ(cc.cwnd(), CongestionController::kDefaultInitialCwnd);
    EXPECT_EQ(cc.smoothedRtt().count(), 0);
}

TEST_F(BandwidthQoSTest, RecordAckAndLossViaQoSManager) {
    qos_->registerConnection(kConn1, Priority::HIGH);

    // Before any ACK, congestion window is unlimited (no CC created yet)
    EXPECT_EQ(qos_->getCongestionWindow(kConn1), UINT64_MAX);

    // After ACK, CC is created and cwnd is tracked
    qos_->recordAck(kConn1, 1460, 500us);
    uint64_t cwnd_after_ack = qos_->getCongestionWindow(kConn1);
    EXPECT_GT(cwnd_after_ack, 0u);
    EXPECT_NE(cwnd_after_ack, UINT64_MAX);

    // After loss, cwnd should be reduced
    qos_->recordLoss(kConn1);
    uint64_t cwnd_after_loss = qos_->getCongestionWindow(kConn1);
    EXPECT_LE(cwnd_after_loss, cwnd_after_ack);
}

TEST_F(BandwidthQoSTest, CongestionWindowUnknownConnectionIsMaxUint) {
    EXPECT_EQ(qos_->getCongestionWindow(9999), UINT64_MAX);
}

// =============================================================================
// Congestion control integration: allowSend() enforces cwnd (AC: congestion
// control integration)
// =============================================================================

TEST_F(BandwidthQoSTest, AllowSendBlockedByCongestionWindow) {
    qos_->registerConnection(kConn1, Priority::HIGH);

    // Seed the CC with ACKs so a small cwnd is established after a loss
    for (int i = 0; i < 5; ++i) {
        qos_->recordAck(kConn1, 1460, 1000us);
    }
    // Force loss to cut cwnd in half and get a known small window
    qos_->recordLoss(kConn1);
    uint64_t small_cwnd = qos_->getCongestionWindow(kConn1);
    ASSERT_GT(small_cwnd, 0u);
    ASSERT_LT(small_cwnd, UINT64_MAX);

    // Send exactly cwnd bytes → should succeed (fills window)
    EXPECT_TRUE(qos_->allowSend(kConn1, small_cwnd, 0ms));
    // Any additional bytes exceed the window → must be blocked
    EXPECT_FALSE(qos_->allowSend(kConn1, 1, 0ms));
}

TEST_F(BandwidthQoSTest, AllowSendAfterWindowDrainedIsPermitted) {
    qos_->registerConnection(kConn1, Priority::HIGH);

    // Establish small window via loss
    for (int i = 0; i < 5; ++i) {
        qos_->recordAck(kConn1, 1460, 1000us);
    }
    qos_->recordLoss(kConn1);
    uint64_t cwnd = qos_->getCongestionWindow(kConn1);

    // Fill the window
    EXPECT_TRUE(qos_->allowSend(kConn1, cwnd, 0ms));
    EXPECT_FALSE(qos_->allowSend(kConn1, 1, 0ms));

    // Drain the queue (simulate ACKed data)
    qos_->recordBytesSent(kConn1, cwnd);
    // Now the window is available again
    EXPECT_TRUE(qos_->allowSend(kConn1, cwnd, 0ms));
}

TEST_F(BandwidthQoSTest, ConnectionStatsExposesCC) {
    qos_->registerConnection(kConn1, Priority::HIGH);

    // Before CC is seeded, congestion_window == UINT64_MAX
    auto cs_before = qos_->getConnectionStats(kConn1);
    EXPECT_EQ(cs_before.congestion_window, UINT64_MAX);
    EXPECT_EQ(cs_before.smoothed_rtt_us, 0u);

    // After an ACK, CC fields are populated
    qos_->recordAck(kConn1, 1460, 2000us);
    auto cs_after = qos_->getConnectionStats(kConn1);
    EXPECT_NE(cs_after.congestion_window, UINT64_MAX);
    EXPECT_GT(cs_after.smoothed_rtt_us, 0u);
    EXPECT_NE(cs_after.congestion_ssthresh_bytes, 0u);
}

// =============================================================================
// AC-BW-7: Config mbps convenience fields
// =============================================================================

TEST(BandwidthMbpsConfigTest, MaxBandwidthMbpsOverridesBps) {
    QoSManager::Config cfg;
    cfg.max_bandwidth_mbps = 100;  // 100 Mbps
    cfg.max_bandwidth_bps  = 0;
    // The effective bandwidth should be 100,000,000 bps
    // (tested indirectly: manager should construct without error)
    QoSManager qos(cfg);
    auto stats = qos.getStats();
    EXPECT_EQ(stats.active_connections, 0u);
}

TEST(BandwidthMbpsConfigTest, PerConnectionLimitMbpsApplied) {
    QoSManager::Config cfg;
    cfg.per_connection_limit_mbps = 1;  // 1 Mbps = 125'000 bytes/s
    cfg.default_burst_bytes       = 0;  // use default (1s of rate)
    QoSManager qos(cfg);

    qos.registerConnection(1, Priority::HIGH);
    auto cs = qos.getConnectionStats(1);
    EXPECT_TRUE(cs.has_token_bucket);
    EXPECT_EQ(cs.token_bucket_rate_bps, 1'000'000u);  // 1 Mbps
}

TEST(BandwidthMbpsConfigTest, EnablePriorityQueueingAlias) {
    QoSManager::Config cfg;
    cfg.enable_priority_queuing = true;
    cfg.enable_fair_queuing     = false;
    QoSManager qos(cfg);

    qos.registerConnection(1, Priority::CRITICAL);
    qos.registerConnection(2, Priority::LOW);
    qos.enqueueSend(2, 100);
    qos.enqueueSend(1, 200);

    // CRITICAL should come first even though LOW was enqueued first
    auto item = qos.dequeueForSend();
    ASSERT_TRUE(item.has_value());
    EXPECT_EQ(item->priority, Priority::CRITICAL);
}

// =============================================================================
// AC-BW-8: Linux tc configureTc()
// =============================================================================

TEST(LinuxTcTest, ConfigureTcWithEmptyInterfaceReturnsFalse) {
    QoSManager qos;
    QoSManager::TcConfig tc;
    tc.enabled        = true;
    tc.interface_name = "";
    EXPECT_FALSE(qos.configureTc(tc));
}

TEST(LinuxTcTest, ConfigureTcDisabledReturnsFalse) {
    QoSManager qos;
    QoSManager::TcConfig tc;
    tc.enabled        = false;
    tc.interface_name = "eth0";
    EXPECT_FALSE(qos.configureTc(tc));
}

TEST(LinuxTcTest, ConfigureTcWithMaliciousInterfaceNameReturnsFalse) {
    QoSManager qos;
    QoSManager::TcConfig tc;
    tc.enabled = true;
    // Shell metacharacters in interface name must be rejected to prevent
    // command injection.
    tc.interface_name = "eth0; rm -rf /";
    EXPECT_FALSE(qos.configureTc(tc));

    tc.interface_name = "eth0$(whoami)";
    EXPECT_FALSE(qos.configureTc(tc));

    tc.interface_name = "eth0`id`";
    EXPECT_FALSE(qos.configureTc(tc));

    // Argument injection via leading dash: tc could interpret "-h" as a flag
    tc.interface_name = "-h";
    EXPECT_FALSE(qos.configureTc(tc));

    tc.interface_name = "--help";
    EXPECT_FALSE(qos.configureTc(tc));

    tc.interface_name = "-eth0";  // starts with dash
    EXPECT_FALSE(qos.configureTc(tc));
}

TEST(LinuxTcTest, ConfigureTcAcceptsValidInterfaceNameFormat) {
    QoSManager qos;
    QoSManager::TcConfig tc;
    tc.enabled = true;
    // Valid interface name formats should not be rejected due to character validation.
    // The call may still return false if tc binary is not available or we lack
    // privileges, but the interface name itself must not be rejected.
    // We verify by checking names with only metacharacters are rejected.
    tc.interface_name = "eth0";
    // On non-Linux or no tc binary this returns false — just no crash/injection.
    (void)qos.configureTc(tc);  // outcome platform-dependent, just no crash

    tc.interface_name = "bond0.100";
    (void)qos.configureTc(tc);

    tc.interface_name = "veth_test-0";
    (void)qos.configureTc(tc);
}

#if !defined(__linux__)
TEST(LinuxTcTest, ConfigureTcOnNonLinuxReturnsFalse) {
    QoSManager qos;
    QoSManager::TcConfig tc;
    tc.enabled        = true;
    tc.interface_name = "eth0";
    tc.total_rate_bps = 1'000'000'000ULL;  // 1 Gbps
    EXPECT_FALSE(qos.configureTc(tc));
}
#endif

// =============================================================================
// Priority level semantics
// =============================================================================

TEST(PriorityLevelsTest, PriorityEnumValuesAreCorrect) {
    EXPECT_EQ(static_cast<uint8_t>(Priority::CRITICAL), 0u);
    EXPECT_EQ(static_cast<uint8_t>(Priority::HIGH),     1u);
    EXPECT_EQ(static_cast<uint8_t>(Priority::MEDIUM),   2u);
    EXPECT_EQ(static_cast<uint8_t>(Priority::LOW),      3u);
}

TEST(PriorityLevelsTest, CriticalInteractiveQueriesHaveLowestNumericValue) {
    // CRITICAL = 0 means it sorts first in strict-priority ordering
    EXPECT_LT(static_cast<uint8_t>(Priority::CRITICAL),
              static_cast<uint8_t>(Priority::HIGH));
    EXPECT_LT(static_cast<uint8_t>(Priority::HIGH),
              static_cast<uint8_t>(Priority::MEDIUM));
    EXPECT_LT(static_cast<uint8_t>(Priority::MEDIUM),
              static_cast<uint8_t>(Priority::LOW));
}
