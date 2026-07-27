// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

/**
 * @file test_cdc_transport_degradation.cpp
 * @brief Phase 2/3 CDC transport-degradation fault-matrix tests.
 *
 * Validates the deterministic, fail-closed behaviour of CDC delivery and
 * consumer-group components under simulated transport-degradation scenarios
 * as required by src/cdc/ROADMAP.md (Phase 2 + Phase 3).
 *
 * ## Test cases
 *
 *   TRD-01  DeliveryTracker: initial trackDelivery records all events as pending.
 *   TRD-02  DeliveryTracker: getPendingRedelivery with zero timeout override
 *           returns all tracked events immediately (simulates instant timeout).
 *   TRD-03  DeliveryTracker: redelivery attempt count increments on successive
 *           timed-out calls (bounded retry tracking).
 *   TRD-04  DeliveryTracker: removeConsumer clears all pending state; subsequent
 *           ack on the removed consumer returns false.
 *   TRD-05  ConsumerGroupManager: partition assignment is deterministic — same
 *           key always maps to the same partition for a given consumer_count.
 *   TRD-06  ConsumerGroupManager: two consumers in the same group handle disjoint
 *           key subsets (partition fan-out).
 *   TRD-07  DeliveryTracker: max_pending_per_consumer enforced — trackDelivery
 *           returns false once the per-consumer cap is reached.
 *   TRD-08  DeliveryTracker: acknowledge returns false for unknown consumer and
 *           for unknown sequence (idempotent rejection).
 *
 * @see include/cdc/cdc_delivery_contract.h — § 2 delivery semantics, § 5 fail-closed
 * @see include/cdc/delivery_tracker.h
 * @see include/cdc/consumer_group.h
 * @see src/cdc/ROADMAP.md Phase 2 / Phase 3 hardening
 */

#include <gtest/gtest.h>

#include "cdc/changefeed.h"
#include "cdc/consumer_group.h"
#include "cdc/cdc_delivery_contract.h"
#include "cdc/delivery_tracker.h"

#include <filesystem>
#include <memory>
#include <rocksdb/utilities/transaction_db.h>
#include <string>
#include <vector>

using namespace themis;
using namespace themis::cdc;
namespace fs = std::filesystem;

// ============================================================================
// Shared fixture — RocksDB + Changefeed + ConsumerGroupManager
// ============================================================================

class TransportDegradationTest : public ::testing::Test {
protected:
    std::string test_db_path;
    std::unique_ptr<rocksdb::TransactionDB> db;
    std::unique_ptr<Changefeed> changefeed;
    std::unique_ptr<ConsumerGroupManager> manager;

    void SetUp() override {
        test_db_path = (fs::temp_directory_path()
                        / ("test_cdc_trd_" + std::to_string(
                               std::chrono::steady_clock::now().time_since_epoch().count())))
                           .string();
        fs::create_directories(test_db_path);

        rocksdb::Options opts;
        opts.create_if_missing = true;
        rocksdb::TransactionDBOptions txn_opts;
        rocksdb::TransactionDB* raw_db = nullptr;
        auto s = rocksdb::TransactionDB::Open(opts, txn_opts, test_db_path, &raw_db);
        ASSERT_TRUE(s.ok()) << s.ToString();
        db.reset(raw_db);

        Changefeed::RetentionPolicy ret;
        ret.enabled = false;
        changefeed = std::make_unique<Changefeed>(db.get(), nullptr, ret);
        manager    = std::make_unique<ConsumerGroupManager>(db.get(), nullptr);
    }

    void TearDown() override {
        manager.reset();
        changefeed.reset();
        db.reset();
        fs::remove_all(test_db_path);
    }

    /// Record @p n events with distinct keys into the changefeed.
    void seedEvents(int n, const std::string& key_prefix = "key") {
        for (int i = 0; i < n; ++i) {
            Changefeed::ChangeEvent ev;
            ev.type         = Changefeed::ChangeEventType::EVENT_PUT;
            ev.key          = key_prefix + ":" + std::to_string(i);
            ev.value        = "v" + std::to_string(i);
            ev.timestamp_ms = 1700000000000LL + i;
            changefeed->recordEvent(ev);
        }
    }

    /// Build a vector of synthetic ChangeEvents with sequences [from, from+count).
    static std::vector<Changefeed::ChangeEvent> makeEvents(
            uint64_t from, int count, const std::string& key_prefix = "trd") {
        std::vector<Changefeed::ChangeEvent> evs;
        evs.reserve(static_cast<std::size_t>(count));
        for (int i = 0; i < count; ++i) {
            Changefeed::ChangeEvent ev;
            ev.sequence     = from + static_cast<uint64_t>(i);
            ev.type         = Changefeed::ChangeEventType::EVENT_PUT;
            ev.key          = key_prefix + ":" + std::to_string(i);
            ev.value        = "val";
            ev.timestamp_ms = static_cast<int64_t>(from) * 1000 + i;
            evs.push_back(ev);
        }
        return evs;
    }
};

// ============================================================================
// TRD-01: trackDelivery records all events as pending
// ============================================================================

TEST_F(TransportDegradationTest, TRD01_TrackDeliveryRecordsPending) {
    DeliveryTracker tracker;
    auto evs = makeEvents(1, 5);
    EXPECT_TRUE(tracker.trackDelivery("consumer_a", evs));

    auto stats = tracker.getStats("consumer_a");
    ASSERT_TRUE(stats.has_value());
    EXPECT_EQ(stats->pending_count,   5u);
    EXPECT_EQ(stats->total_delivered, 5u);
    EXPECT_EQ(stats->total_acknowledged, 0u);
}

// ============================================================================
// TRD-02: getPendingRedelivery with zero-duration override returns events
//         immediately — simulates instant ack-timeout expiry
// ============================================================================

TEST_F(TransportDegradationTest, TRD02_ZeroTimeoutOverrideReturnsAllPending) {
    DeliveryTracker tracker;
    auto evs = makeEvents(10, 3);
    tracker.trackDelivery("consumer_b", evs);

    // Passing timeout_override = 0ms forces all pending events to appear
    // timed-out and eligible for redelivery.
    auto pending = tracker.getPendingRedelivery(
        "consumer_b", std::chrono::milliseconds{0});

    EXPECT_EQ(pending.size(), 3u)
        << "All tracked events must be returned with zero timeout override";
}

// ============================================================================
// TRD-03: Redelivery attempt count increments across successive calls
// ============================================================================

TEST_F(TransportDegradationTest, TRD03_RedeliveryAttemptCountIncrementsOnTimeout) {
    // Configure with max_redelivery_attempts=3 so we can observe the counter.
    DeliveryTrackerConfig cfg;
    cfg.max_redelivery_attempts = 3;
    DeliveryTracker tracker(cfg);

    auto evs = makeEvents(20, 2);
    tracker.trackDelivery("consumer_c", evs);

    // First redelivery query (instant timeout).
    auto first = tracker.getPendingRedelivery(
        "consumer_c", std::chrono::milliseconds{0});
    EXPECT_EQ(first.size(), 2u);

    // Second redelivery query — attempt count should now be 2.
    auto second = tracker.getPendingRedelivery(
        "consumer_c", std::chrono::milliseconds{0});
    EXPECT_EQ(second.size(), 2u);

    auto stats = tracker.getStats("consumer_c");
    ASSERT_TRUE(stats.has_value());
    EXPECT_GE(stats->total_redeliveries, 2u)
        << "Redelivery counter must have incremented";
}

// ============================================================================
// TRD-04: removeConsumer clears all pending state
// ============================================================================

TEST_F(TransportDegradationTest, TRD04_RemoveConsumerClearsPendingState) {
    DeliveryTracker tracker;
    auto evs = makeEvents(30, 4);
    tracker.trackDelivery("consumer_d", evs);

    tracker.removeConsumer("consumer_d");

    // Stats should be gone.
    EXPECT_FALSE(tracker.getStats("consumer_d").has_value())
        << "Stats for removed consumer must be absent";

    // Ack on removed consumer must return false.
    EXPECT_FALSE(tracker.acknowledge("consumer_d", 30u));
}

// ============================================================================
// TRD-05: Partition assignment is deterministic for same key + consumer_count
// ============================================================================

TEST_F(TransportDegradationTest, TRD05_PartitionAssignmentIsDeterministic) {
    // Contract § 6: same (key, consumer_count) always yields same partition.
    const uint32_t consumer_count = 4;
    const std::vector<std::string> keys = {
        "user:alice", "user:bob", "order:123", "order:456", "event:xyz"
    };

    for (const auto& key : keys) {
        uint32_t p1 = ConsumerGroupManager::partitionForKey(key, consumer_count);
        uint32_t p2 = ConsumerGroupManager::partitionForKey(key, consumer_count);
        EXPECT_EQ(p1, p2) << "Partition for key '" << key << "' must be deterministic";
        EXPECT_LT(p1, consumer_count) << "Partition index must be in [0, consumer_count)";
    }
}

// ============================================================================
// TRD-06: Two consumers in the same group handle disjoint key subsets
// ============================================================================

TEST_F(TransportDegradationTest, TRD06_PartitionFanOutIsDisjoint) {
    // Contract § 6b: for consumer_count=2 and 10 distinct keys, each key
    // belongs to exactly one partition — no key should be assigned to both
    // consumer 0 and consumer 1.
    const uint32_t consumer_count = 2;
    std::vector<std::string> keys;
    for (int i = 0; i < 10; ++i) {
        keys.push_back("key:" + std::to_string(i));
    }

    for (const auto& key : keys) {
        uint32_t part = ConsumerGroupManager::partitionForKey(key, consumer_count);
        // part must be either 0 or 1, not both.
        EXPECT_LT(part, consumer_count);

        // Verify the other partition index does NOT own this key.
        uint32_t other = (part == 0u) ? 1u : 0u;
        EXPECT_NE(ConsumerGroupManager::partitionForKey(key, consumer_count), other)
            << "Key '" << key << "' must not belong to both partitions";
    }
}

// ============================================================================
// TRD-07: max_pending_per_consumer enforced
// ============================================================================

TEST_F(TransportDegradationTest, TRD07_MaxPendingLimitEnforcesBackpressure) {
    // Contract § 2: trackDelivery returns false when per-consumer limit reached.
    DeliveryTrackerConfig cfg;
    cfg.max_pending_per_consumer = 3;
    DeliveryTracker tracker(cfg);

    auto first_batch = makeEvents(100, 3);
    EXPECT_TRUE(tracker.trackDelivery("consumer_e", first_batch))
        << "First batch at capacity must succeed";

    auto overflow = makeEvents(200, 1);
    EXPECT_FALSE(tracker.trackDelivery("consumer_e", overflow))
        << "Delivery beyond per-consumer limit must be rejected (back-pressure)";
}

// ============================================================================
// TRD-08: acknowledge returns false for unknown consumer or unknown sequence
// ============================================================================

TEST_F(TransportDegradationTest, TRD08_AcknowledgeReturnsFalseForUnknownContext) {
    DeliveryTracker tracker;

    // Unknown consumer.
    EXPECT_FALSE(tracker.acknowledge("nonexistent_consumer", 1u))
        << "Ack on unknown consumer must return false";

    // Known consumer, unknown sequence.
    auto evs = makeEvents(50, 2);
    tracker.trackDelivery("consumer_f", evs);

    EXPECT_FALSE(tracker.acknowledge("consumer_f", 999u))
        << "Ack on unknown sequence must return false";

    // Valid ack.
    EXPECT_TRUE(tracker.acknowledge("consumer_f", 50u));

    // Duplicate ack must be idempotent (false).
    EXPECT_FALSE(tracker.acknowledge("consumer_f", 50u))
        << "Duplicate ack must return false";
}
