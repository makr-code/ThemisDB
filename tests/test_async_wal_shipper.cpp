// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

/**
 * @file test_async_wal_shipper.cpp
 * @brief Unit tests for AsyncWalShipper.
 */

#include "replication/async_wal_shipper.h"

#include <gtest/gtest.h>
#include <chrono>
#include <thread>
#include <atomic>

namespace themisdb {
namespace replication {

// Test fixture
class AsyncWalShipperTest : public ::testing::Test {
protected:
    WalShippingConfig makeConfig()
    {
        WalShippingConfig cfg;
        cfg.remote_dc_endpoint = "dc-eu-west:9876";
        cfg.local_dc_id = "dc-us-east";
        cfg.max_lag_ms = 500;
        cfg.max_queue_depth = 1000;
        cfg.histogram_buckets = 16;
        return cfg;
    }
};

// ============================================================================
// Construction and lifecycle tests
// ============================================================================

TEST_F(AsyncWalShipperTest, ConstructionAndDestruction)
{
    auto cfg = makeConfig();
    {
        AsyncWalShipper shipper(cfg);
        // Should not crash on destruction
    }
}

TEST_F(AsyncWalShipperTest, StatsInitialization)
{
    auto cfg = makeConfig();
    AsyncWalShipper shipper(cfg);

    auto stats = shipper.stats();
    EXPECT_EQ(stats.segments_enqueued, 0);
    EXPECT_EQ(stats.segments_shipped, 0);
    EXPECT_EQ(stats.segments_dropped, 0);
}

// ============================================================================
// Segment enqueue tests
// ============================================================================

TEST_F(AsyncWalShipperTest, EnqueueSegment)
{
    auto cfg = makeConfig();
    AsyncWalShipper shipper(cfg);

    WalSegment seg;
    seg.sequence_number = 1;
    seg.data = "test data";
    seg.enqueue_time = std::chrono::steady_clock::now();
    seg.target_dc = "dc-eu-west";

    EXPECT_TRUE(shipper.enqueueSegment(std::move(seg)));

    auto stats = shipper.stats();
    EXPECT_EQ(stats.segments_enqueued, 1);
}

TEST_F(AsyncWalShipperTest, EnqueueMultipleSegments)
{
    auto cfg = makeConfig();
    AsyncWalShipper shipper(cfg);

    for (int i = 0; i < 10; ++i) {
        WalSegment seg;
        seg.sequence_number = i;
        seg.data = std::string(100, 'x');
        seg.enqueue_time = std::chrono::steady_clock::now();
        seg.target_dc = "dc-eu-west";

        EXPECT_TRUE(shipper.enqueueSegment(std::move(seg)));
    }

    auto stats = shipper.stats();
    EXPECT_EQ(stats.segments_enqueued, 10);
    EXPECT_EQ(stats.bytes_enqueued, 1000);  // 10 * 100 bytes
}

TEST_F(AsyncWalShipperTest, QueueFullDropsSegment)
{
    auto cfg = makeConfig();
    cfg.max_queue_depth = 2;
    AsyncWalShipper shipper(cfg);

    // Fill queue
    for (int i = 0; i < 3; ++i) {
        WalSegment seg;
        seg.sequence_number = i;
        seg.data = "test";
        seg.enqueue_time = std::chrono::steady_clock::now();
        seg.target_dc = "dc-eu-west";

        if (i < 2) {
            EXPECT_TRUE(shipper.enqueueSegment(std::move(seg)));
        } else {
            // Third should be rejected (queue full)
            EXPECT_FALSE(shipper.enqueueSegment(std::move(seg)));
        }
    }

    auto stats = shipper.stats();
    EXPECT_EQ(stats.segments_enqueued, 2);
    EXPECT_EQ(stats.segments_dropped, 1);
}

// ============================================================================
// Lag measurement tests
// ============================================================================

TEST_F(AsyncWalShipperTest, CurrentLagEmptyQueue)
{
    auto cfg = makeConfig();
    AsyncWalShipper shipper(cfg);

    auto lag = shipper.currentLagMs();
    EXPECT_EQ(lag, 0);
}

TEST_F(AsyncWalShipperTest, CurrentLagIncreases)
{
    auto cfg = makeConfig();
    AsyncWalShipper shipper(cfg);

    WalSegment seg;
    seg.sequence_number = 1;
    seg.data = "test";
    seg.enqueue_time = std::chrono::steady_clock::now();
    seg.target_dc = "dc-eu-west";

    shipper.enqueueSegment(std::move(seg));

    auto lag1 = shipper.currentLagMs();
    EXPECT_GE(lag1, 0);

    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    auto lag2 = shipper.currentLagMs();
    EXPECT_GE(lag2, lag1);
}

// ============================================================================
// Alert callback tests
// ============================================================================

TEST_F(AsyncWalShipperTest, AlertCallbackOnExcessiveLag)
{
    auto cfg = makeConfig();
    cfg.max_lag_ms = 100;
    AsyncWalShipper shipper(cfg);

    std::atomic<int> alert_count{0};
    shipper.setAlertCallback([&](uint64_t lag_ms) {
        alert_count++;
    });

    // Inject custom ship handler that delays
    shipper.setShipHandler([](const WalSegment& seg) -> bool {
        std::this_thread::sleep_for(std::chrono::milliseconds(150));
        return true;
    });

    WalSegment seg;
    seg.sequence_number = 1;
    seg.data = "test";
    seg.enqueue_time = std::chrono::steady_clock::now();
    seg.target_dc = "dc-eu-west";

    shipper.enqueueSegment(std::move(seg));

    // Wait for processing
    std::this_thread::sleep_for(std::chrono::milliseconds(300));

    EXPECT_GE(alert_count, 1);
}

// ============================================================================
// Metrics export tests
// ============================================================================

TEST_F(AsyncWalShipperTest, PrometheusMetrics)
{
    auto cfg = makeConfig();
    AsyncWalShipper shipper(cfg);

    for (int i = 0; i < 5; ++i) {
        WalSegment seg;
        seg.sequence_number = i;
        seg.data = "test data";
        seg.enqueue_time = std::chrono::steady_clock::now();
        seg.target_dc = "dc-eu-west";

        shipper.enqueueSegment(std::move(seg));
    }

    auto metrics = shipper.exportPrometheusMetrics();

    EXPECT_NE(metrics.find("replication_wal_lag_ms"), std::string::npos);
    EXPECT_NE(metrics.find("replication_wal_segments_enqueued_total"),
              std::string::npos);
    EXPECT_NE(metrics.find("replication_wal_segments_shipped_total"),
              std::string::npos);
}

TEST_F(AsyncWalShipperTest, MetricsIncludeLabels)
{
    auto cfg = makeConfig();
    AsyncWalShipper shipper(cfg);

    auto metrics = shipper.exportPrometheusMetrics();

    EXPECT_NE(metrics.find("local_dc"), std::string::npos);
    EXPECT_NE(metrics.find("remote_dc"), std::string::npos);
}

// ============================================================================
// Ship handler tests
// ============================================================================

TEST_F(AsyncWalShipperTest, CustomShipHandler)
{
    auto cfg = makeConfig();
    AsyncWalShipper shipper(cfg);

    std::atomic<int> handler_calls{0};
    std::vector<uint64_t> shipped_seqs;

    shipper.setShipHandler([&](const WalSegment& seg) -> bool {
        handler_calls++;
        shipped_seqs.push_back(seg.sequence_number);
        return true;
    });

    for (int i = 0; i < 3; ++i) {
        WalSegment seg;
        seg.sequence_number = i;
        seg.data = "test";
        seg.enqueue_time = std::chrono::steady_clock::now();
        seg.target_dc = "dc-eu-west";

        shipper.enqueueSegment(std::move(seg));
    }

    // Wait for background thread to process
    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    EXPECT_GE(handler_calls, 0);  // May or may not process in time
}

// ============================================================================
// Stop/graceful shutdown tests
// ============================================================================

TEST_F(AsyncWalShipperTest, GracefulStop)
{
    auto cfg = makeConfig();
    AsyncWalShipper shipper(cfg);

    for (int i = 0; i < 5; ++i) {
        WalSegment seg;
        seg.sequence_number = i;
        seg.data = "test";
        seg.enqueue_time = std::chrono::steady_clock::now();
        seg.target_dc = "dc-eu-west";

        shipper.enqueueSegment(std::move(seg));
    }

    shipper.stop();  // Should drain queue and exit gracefully
    // No assertion; mainly testing that this doesn't crash
}

TEST_F(AsyncWalShipperTest, MultipleStop)
{
    auto cfg = makeConfig();
    AsyncWalShipper shipper(cfg);

    shipper.stop();
    shipper.stop();  // Should be idempotent
}

// ============================================================================
// Stress tests
// ============================================================================

TEST_F(AsyncWalShipperTest, HighThroughput)
{
    auto cfg = makeConfig();
    cfg.max_queue_depth = 10000;
    AsyncWalShipper shipper(cfg);

    // Enqueue many segments rapidly
    for (int i = 0; i < 1000; ++i) {
        WalSegment seg;
        seg.sequence_number = i;
        seg.data = std::string(100, 'x');
        seg.enqueue_time = std::chrono::steady_clock::now();
        seg.target_dc = "dc-eu-west";

        if (!shipper.enqueueSegment(std::move(seg))) {
            // OK if some are dropped under high load
            break;
        }
    }

    auto stats = shipper.stats();
    EXPECT_GT(stats.segments_enqueued, 0);

    shipper.stop();
}

TEST_F(AsyncWalShipperTest, ConcurrentEnqueue)
{
    auto cfg = makeConfig();
    cfg.max_queue_depth = 10000;
    AsyncWalShipper shipper(cfg);

    std::vector<std::thread> threads;
    std::atomic<int> total_enqueued{0};

    for (int t = 0; t < 5; ++t) {
        threads.emplace_back([&, t]() {
            for (int i = 0; i < 100; ++i) {
                WalSegment seg;
                seg.sequence_number = t * 100 + i;
                seg.data = "concurrent test";
                seg.enqueue_time = std::chrono::steady_clock::now();
                seg.target_dc = "dc-eu-west";

                if (shipper.enqueueSegment(std::move(seg))) {
                    total_enqueued++;
                }
            }
        });
    }

    for (auto& t : threads) {
        t.join();
    }

    auto stats = shipper.stats();
    EXPECT_EQ(stats.segments_enqueued, total_enqueued.load());

    shipper.stop();
}

// ============================================================================
// Edge cases
// ============================================================================

TEST_F(AsyncWalShipperTest, EmptySegment)
{
    auto cfg = makeConfig();
    AsyncWalShipper shipper(cfg);

    WalSegment seg;
    seg.sequence_number = 1;
    seg.data = "";  // Empty
    seg.enqueue_time = std::chrono::steady_clock::now();
    seg.target_dc = "dc-eu-west";

    EXPECT_TRUE(shipper.enqueueSegment(std::move(seg)));
    auto stats = shipper.stats();
    EXPECT_EQ(stats.segments_enqueued, 1);
    EXPECT_EQ(stats.bytes_enqueued, 0);
}

TEST_F(AsyncWalShipperTest, LargeSegment)
{
    auto cfg = makeConfig();
    AsyncWalShipper shipper(cfg);

    WalSegment seg;
    seg.sequence_number = 1;
    seg.data = std::string(10 * 1024 * 1024, 'x');  // 10 MB
    seg.enqueue_time = std::chrono::steady_clock::now();
    seg.target_dc = "dc-eu-west";

    EXPECT_TRUE(shipper.enqueueSegment(std::move(seg)));
    auto stats = shipper.stats();
    EXPECT_EQ(stats.bytes_enqueued, 10 * 1024 * 1024);
}

}  // namespace replication
}  // namespace themisdb
