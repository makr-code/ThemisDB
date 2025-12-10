/**
 * Unit tests for TrueTime Clock
 */

#include <gtest/gtest.h>
#include "sharding/truetime_clock.h"
#include <thread>
#include <chrono>

using namespace themis::sharding;

class TrueTimeClockTest : public ::testing::Test {
protected:
    void SetUp() override {
        config_.node_id = "test_node_1";
        config_.source = ClockSource::SYSTEM_CLOCK;
        config_.base_uncertainty_us = 100;
        config_.max_uncertainty_us = 10000;
        config_.enable_commit_wait = true;
    }
    
    TrueTimeConfig config_;
};

// TrueTimeStamp Tests

TEST_F(TrueTimeClockTest, TimestampMidpoint) {
    TrueTimeStamp ts;
    ts.earliest_us = 1000;
    ts.latest_us = 2000;
    ts.logical = 0;
    ts.node_id = "node1";
    
    EXPECT_EQ(ts.midpoint(), 1500);
}

TEST_F(TrueTimeClockTest, TimestampUncertainty) {
    TrueTimeStamp ts;
    ts.earliest_us = 1000;
    ts.latest_us = 2000;
    
    EXPECT_EQ(ts.uncertainty(), 500);
}

TEST_F(TrueTimeClockTest, TimestampDefinitelyBefore) {
    TrueTimeStamp ts1;
    ts1.earliest_us = 1000;
    ts1.latest_us = 2000;
    
    TrueTimeStamp ts2;
    ts2.earliest_us = 3000;
    ts2.latest_us = 4000;
    
    EXPECT_TRUE(ts1.definitelyBefore(ts2));
    EXPECT_FALSE(ts2.definitelyBefore(ts1));
}

TEST_F(TrueTimeClockTest, TimestampDefinitelyAfter) {
    TrueTimeStamp ts1;
    ts1.earliest_us = 3000;
    ts1.latest_us = 4000;
    
    TrueTimeStamp ts2;
    ts2.earliest_us = 1000;
    ts2.latest_us = 2000;
    
    EXPECT_TRUE(ts1.definitelyAfter(ts2));
    EXPECT_FALSE(ts2.definitelyAfter(ts1));
}

TEST_F(TrueTimeClockTest, TimestampOverlaps) {
    TrueTimeStamp ts1;
    ts1.earliest_us = 1000;
    ts1.latest_us = 3000;
    
    TrueTimeStamp ts2;
    ts2.earliest_us = 2000;
    ts2.latest_us = 4000;
    
    EXPECT_TRUE(ts1.overlaps(ts2));
    EXPECT_TRUE(ts2.overlaps(ts1));
}

TEST_F(TrueTimeClockTest, TimestampCompare) {
    TrueTimeStamp ts1;
    ts1.earliest_us = 1000;
    ts1.latest_us = 2000;
    ts1.logical = 5;
    ts1.node_id = "node1";
    
    TrueTimeStamp ts2;
    ts2.earliest_us = 3000;
    ts2.latest_us = 4000;
    ts2.logical = 10;
    ts2.node_id = "node2";
    
    EXPECT_LT(ts1.compare(ts2), 0);
    EXPECT_GT(ts2.compare(ts1), 0);
    EXPECT_EQ(ts1.compare(ts1), 0);
}

TEST_F(TrueTimeClockTest, TimestampCompareConcurrent) {
    TrueTimeStamp ts1;
    ts1.earliest_us = 1000;
    ts1.latest_us = 3000;
    ts1.logical = 5;
    ts1.node_id = "node1";
    
    TrueTimeStamp ts2;
    ts2.earliest_us = 2000;
    ts2.latest_us = 4000;
    ts2.logical = 10;
    ts2.node_id = "node2";
    
    // Overlapping timestamps - use logical counter
    int result = ts1.compare(ts2);
    EXPECT_LT(result, 0);  // ts1.logical < ts2.logical
}

TEST_F(TrueTimeClockTest, TimestampJsonSerialization) {
    TrueTimeStamp ts;
    ts.earliest_us = 1000;
    ts.latest_us = 2000;
    ts.logical = 42;
    ts.node_id = "test_node";
    
    std::string json = ts.toJson();
    auto ts2_opt = TrueTimeStamp::fromJson(json);
    
    ASSERT_TRUE(ts2_opt.has_value());
    auto ts2 = ts2_opt.value();
    
    EXPECT_EQ(ts.earliest_us, ts2.earliest_us);
    EXPECT_EQ(ts.latest_us, ts2.latest_us);
    EXPECT_EQ(ts.logical, ts2.logical);
    EXPECT_EQ(ts.node_id, ts2.node_id);
}

TEST_F(TrueTimeClockTest, TimestampOperators) {
    TrueTimeStamp ts1;
    ts1.earliest_us = 1000;
    ts1.latest_us = 2000;
    ts1.logical = 5;
    ts1.node_id = "node1";
    
    TrueTimeStamp ts2;
    ts2.earliest_us = 3000;
    ts2.latest_us = 4000;
    ts2.logical = 10;
    ts2.node_id = "node2";
    
    EXPECT_TRUE(ts1 < ts2);
    EXPECT_TRUE(ts2 > ts1);
    EXPECT_TRUE(ts1 == ts1);
    EXPECT_FALSE(ts1 == ts2);
}

// TrueTimeClock Tests

TEST_F(TrueTimeClockTest, ClockConstruction) {
    TrueTimeClock clock(config_);
    EXPECT_NO_THROW(clock.start());
    EXPECT_NO_THROW(clock.stop());
}

TEST_F(TrueTimeClockTest, ClockNow) {
    TrueTimeClock clock(config_);
    clock.start();
    
    auto ts = clock.now();
    
    EXPECT_GT(ts.earliest_us, 0);
    EXPECT_GT(ts.latest_us, ts.earliest_us);
    EXPECT_EQ(ts.node_id, config_.node_id);
    EXPECT_GE(ts.uncertainty(), config_.base_uncertainty_us);
    
    clock.stop();
}

TEST_F(TrueTimeClockTest, ClockMonotonicIncreasing) {
    TrueTimeClock clock(config_);
    clock.start();
    
    auto ts1 = clock.now();
    std::this_thread::sleep_for(std::chrono::microseconds(10));
    auto ts2 = clock.now();
    
    // ts2 should be after ts1 (or at least not before)
    EXPECT_FALSE(ts2.definitelyBefore(ts1));
    
    clock.stop();
}

TEST_F(TrueTimeClockTest, ClockLogicalCounter) {
    TrueTimeClock clock(config_);
    clock.start();
    
    auto ts1 = clock.now();
    auto ts2 = clock.now();  // Immediate second call
    
    // If called within same microsecond, logical counter should increment
    if (ts1.midpoint() == ts2.midpoint()) {
        EXPECT_GT(ts2.logical, ts1.logical);
    }
    
    clock.stop();
}

TEST_F(TrueTimeClockTest, ClockAfter) {
    TrueTimeClock clock(config_);
    clock.start();
    
    uint64_t target_time = clock.now().midpoint() + 1000;
    auto ts = clock.after(target_time);
    
    EXPECT_GE(ts.earliest_us, target_time);
    
    clock.stop();
}

TEST_F(TrueTimeClockTest, ClockReceive) {
    TrueTimeClock clock(config_);
    clock.start();
    
    // Create a timestamp from the future
    auto now_ts = clock.now();
    TrueTimeStamp future_ts;
    future_ts.earliest_us = now_ts.latest_us + 1000;
    future_ts.latest_us = future_ts.earliest_us + 200;
    future_ts.logical = 10;
    future_ts.node_id = "remote_node";
    
    // Receive the future timestamp
    auto updated_ts = clock.receive(future_ts);
    
    // Clock should have advanced
    EXPECT_GE(updated_ts.midpoint(), future_ts.midpoint());
    EXPECT_GE(updated_ts.logical, future_ts.logical);
    
    clock.stop();
}

TEST_F(TrueTimeClockTest, ClockSyncNow) {
    TrueTimeClock clock(config_);
    clock.start();
    
    bool success = clock.syncNow();
    EXPECT_TRUE(success);
    
    auto stats = clock.getStats();
    EXPECT_GT(stats.sync_count, 0);
    
    clock.stop();
}

TEST_F(TrueTimeClockTest, ClockUncertainty) {
    TrueTimeClock clock(config_);
    clock.start();
    
    uint64_t uncertainty = clock.getCurrentUncertainty();
    EXPECT_GE(uncertainty, config_.base_uncertainty_us);
    EXPECT_LE(uncertainty, config_.max_uncertainty_us);
    
    clock.stop();
}

TEST_F(TrueTimeClockTest, ClockStats) {
    TrueTimeClock clock(config_);
    clock.start();
    
    // Trigger a sync
    clock.syncNow();
    
    auto stats = clock.getStats();
    EXPECT_GT(stats.sync_count, 0);
    EXPECT_GE(stats.current_uncertainty_us, config_.base_uncertainty_us);
    EXPECT_EQ(stats.sync_source, "SYSTEM_CLOCK");
    
    clock.stop();
}

TEST_F(TrueTimeClockTest, ClockPrometheusMetrics) {
    TrueTimeClock clock(config_);
    clock.start();
    
    std::string metrics = clock.exportPrometheusMetrics();
    
    EXPECT_NE(metrics.find("themis_truetime_sync_count"), std::string::npos);
    EXPECT_NE(metrics.find("themis_truetime_uncertainty_us"), std::string::npos);
    EXPECT_NE(metrics.find(config_.node_id), std::string::npos);
    
    clock.stop();
}

// Commit-Wait Tests

TEST_F(TrueTimeClockTest, CommitWaitPastTimestamp) {
    config_.enable_commit_wait = true;
    TrueTimeClock clock(config_);
    clock.start();
    
    // Create a timestamp from the past
    auto now_ts = clock.now();
    TrueTimeStamp past_ts;
    past_ts.earliest_us = now_ts.earliest_us - 10000;
    past_ts.latest_us = now_ts.earliest_us - 5000;
    past_ts.logical = 0;
    past_ts.node_id = "node1";
    
    // Should not need to wait
    bool success = clock.waitUntilPast(past_ts);
    EXPECT_TRUE(success);
    
    clock.stop();
}

TEST_F(TrueTimeClockTest, CommitWaitFutureTimestamp) {
    config_.enable_commit_wait = true;
    TrueTimeClock clock(config_);
    clock.start();
    
    // Create a timestamp slightly in the future
    auto now_ts = clock.now();
    TrueTimeStamp future_ts;
    future_ts.earliest_us = now_ts.latest_us + 100;
    future_ts.latest_us = future_ts.earliest_us + 200;
    future_ts.logical = 0;
    future_ts.node_id = "node1";
    
    auto start = std::chrono::steady_clock::now();
    bool success = clock.waitUntilPast(future_ts);
    auto duration = std::chrono::steady_clock::now() - start;
    
    EXPECT_TRUE(success);
    // Should have waited at least the uncertainty
    EXPECT_GT(std::chrono::duration_cast<std::chrono::microseconds>(duration).count(), 100);
    
    clock.stop();
}

TEST_F(TrueTimeClockTest, CommitWaitDisabled) {
    config_.enable_commit_wait = false;
    TrueTimeClock clock(config_);
    clock.start();
    
    auto future_ts = clock.now();
    future_ts.latest_us += 100000;  // Far future
    
    // Should return immediately when commit-wait is disabled
    auto start = std::chrono::steady_clock::now();
    bool success = clock.waitUntilPast(future_ts);
    auto duration = std::chrono::steady_clock::now() - start;
    
    EXPECT_TRUE(success);
    EXPECT_LT(std::chrono::duration_cast<std::chrono::microseconds>(duration).count(), 1000);
    
    clock.stop();
}

TEST_F(TrueTimeClockTest, CommitWaitHelperCalculateWait) {
    TrueTimeStamp commit_ts;
    commit_ts.earliest_us = 10000;
    commit_ts.latest_us = 11000;
    
    TrueTimeStamp now_ts;
    now_ts.earliest_us = 5000;
    now_ts.latest_us = 6000;
    
    uint64_t wait_us = CommitWaitHelper::calculateWaitDuration(commit_ts, now_ts);
    
    // Should wait until commit_ts.latest is past now_ts.earliest
    EXPECT_GT(wait_us, 0);
    EXPECT_GE(wait_us, commit_ts.latest_us - now_ts.earliest_us);
}

// Multiple Nodes Test

TEST_F(TrueTimeClockTest, MultipleNodesOrdering) {
    TrueTimeConfig config1 = config_;
    config1.node_id = "node1";
    TrueTimeConfig config2 = config_;
    config2.node_id = "node2";
    
    TrueTimeClock clock1(config1);
    TrueTimeClock clock2(config2);
    
    clock1.start();
    clock2.start();
    
    // Node 1 creates a timestamp
    auto ts1 = clock1.now();
    
    // Simulate sending to node 2
    std::this_thread::sleep_for(std::chrono::microseconds(100));
    auto ts2 = clock2.receive(ts1);
    
    // Node 2's timestamp should be after node 1's
    EXPECT_FALSE(ts2.definitelyBefore(ts1));
    
    // Node 2 creates its own timestamp
    auto ts3 = clock2.now();
    
    // Should be ordered
    EXPECT_FALSE(ts3.definitelyBefore(ts1));
    
    clock1.stop();
    clock2.stop();
}

// Edge Cases

TEST_F(TrueTimeClockTest, MaxUncertaintyCap) {
    config_.base_uncertainty_us = 100;
    config_.max_uncertainty_us = 1000;
    config_.drift_rate_ppm = 10000;  // Very high drift
    
    TrueTimeClock clock(config_);
    clock.start();
    
    // Wait to accumulate drift
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    auto ts = clock.now();
    EXPECT_LE(ts.uncertainty(), config_.max_uncertainty_us);
    
    clock.stop();
}

TEST_F(TrueTimeClockTest, ConcurrentAccess) {
    TrueTimeClock clock(config_);
    clock.start();
    
    // Multiple threads calling now() concurrently
    std::vector<std::thread> threads;
    std::vector<TrueTimeStamp> timestamps(10);
    
    for (int i = 0; i < 10; ++i) {
        threads.emplace_back([&clock, &timestamps, i]() {
            timestamps[i] = clock.now();
        });
    }
    
    for (auto& t : threads) {
        t.join();
    }
    
    // All timestamps should be valid
    for (const auto& ts : timestamps) {
        EXPECT_GT(ts.latest_us, ts.earliest_us);
        EXPECT_EQ(ts.node_id, config_.node_id);
    }
    
    clock.stop();
}
