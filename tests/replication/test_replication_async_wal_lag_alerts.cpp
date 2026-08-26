/**
 * @file test_replication_async_wal_lag_alerts.cpp
 * @brief Contract tests for AsyncWalShipper lag-alert API.
 */

#include <gtest/gtest.h>

#include "replication/async_wal_shipper.h"

using namespace themisdb::replication;

TEST(AsyncWalShipperContract, ConstructAndStatsAreAccessible) {
    WalShippingConfig cfg;
    cfg.remote_dc_endpoint = "dc-b:9000";
    cfg.local_dc_id = "dc-a";
    cfg.max_lag_ms = 1000;

    AsyncWalShipper shipper(cfg);
    const auto s = shipper.stats();
    EXPECT_EQ(s.segments_enqueued, 0u);
}

TEST(AsyncWalShipperContract, EnqueueSegmentIncrementsCounter) {
    WalShippingConfig cfg;
    cfg.remote_dc_endpoint = "dc-b:9000";
    cfg.local_dc_id = "dc-a";

    AsyncWalShipper shipper(cfg);

    WalSegment seg;
    seg.sequence_number = 1;
    seg.data = "abc";
    seg.enqueue_time = std::chrono::steady_clock::now();
    seg.target_dc = "dc-b";

    EXPECT_TRUE(shipper.enqueueSegment(std::move(seg)));
    EXPECT_GE(shipper.stats().segments_enqueued, 1u);
}
