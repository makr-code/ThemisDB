// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

/**
 * @file test_cdc_replay_ack_hardening.cpp
 * @brief Phase 3/4 CDC replay/acknowledgement permutation hardening tests.
 *
 * Validates deterministic and auditable state transitions for the replay,
 * acknowledgement, and delivery-timeout pathways as required by
 * src/cdc/ROADMAP.md (Phase 3 + Phase 4).
 *
 * ## Test cases
 *
 *   RAH-01  InMemoryReplayController returns events in sequence order from a
 *           given from_sequence offset.
 *   RAH-02  InMemoryReplaySession drains completely; done() is true after the
 *           last nextBatch() call and subsequent calls return empty vector.
 *   RAH-03  InMemoryReplaySession cancel() terminates the session before drain;
 *           done() returns true and nextBatch() returns {} afterward.
 *   RAH-04  Changefeed listEvents respects from_sequence exclusive lower bound.
 *   RAH-05  DeliveryTracker acknowledgeUpTo removes all sequences ≤ boundary.
 *   RAH-06  Changefeed listEvents respects to_sequence inclusive upper bound.
 *   RAH-07  DeliveryTracker: getPendingRedelivery with non-zero timeout_override
 *           holds events that were just tracked (override > elapsed time).
 *   RAH-08  InMemoryReplayController::totalSessionsCreated increments correctly.
 *
 * @see include/cdc/cdc_delivery_contract.h — § 3 replay contract, § 2 delivery
 * @see include/cdc/icdc_replay_controller.h
 * @see include/cdc/delivery_tracker.h
 * @see src/cdc/ROADMAP.md Phase 3 / Phase 4
 */

#include <gtest/gtest.h>

#include "cdc/changefeed.h"
#include "cdc/cdc_delivery_contract.h"
#include "cdc/delivery_tracker.h"
#include "cdc/icdc_replay_controller.h"

#include <filesystem>
#include <memory>
#include <rocksdb/utilities/transaction_db.h>
#include <string>
#include <vector>

using namespace themis;
using namespace themis::cdc;
namespace fs = std::filesystem;

// ============================================================================
// Shared fixture — RocksDB + Changefeed
// ============================================================================

class ReplayAckHardeningTest : public ::testing::Test {
protected:
    std::string db_path;
    std::unique_ptr<rocksdb::TransactionDB> db;
    std::unique_ptr<Changefeed> changefeed;

    void SetUp() override {
        db_path = "/tmp/test_cdc_rah_" + std::to_string(::time(nullptr));
        fs::create_directories(db_path);

        rocksdb::Options opts;
        opts.create_if_missing = true;
        rocksdb::TransactionDBOptions txn_opts;
        rocksdb::TransactionDB* raw_db = nullptr;
        auto s = rocksdb::TransactionDB::Open(opts, txn_opts, db_path, &raw_db);
        ASSERT_TRUE(s.ok()) << s.ToString();
        db.reset(raw_db);

        Changefeed::RetentionPolicy ret;
        ret.enabled = false;
        changefeed = std::make_unique<Changefeed>(db.get(), nullptr, ret);
    }

    void TearDown() override {
        changefeed.reset();
        db.reset();
        fs::remove_all(db_path);
    }

    /// Record @p n events and return the list of assigned sequences.
    std::vector<uint64_t> seedEvents(int n, const std::string& prefix = "key") {
        std::vector<uint64_t> seqs;
        for (int i = 0; i < n; ++i) {
            Changefeed::ChangeEvent ev;
            ev.type         = Changefeed::ChangeEventType::EVENT_PUT;
            ev.key          = prefix + ":" + std::to_string(i);
            ev.value        = "v" + std::to_string(i);
            ev.timestamp_ms = 1700000000000LL + i;
            changefeed->recordEvent(ev);
            seqs.push_back(changefeed->getLatestSequence());
        }
        return seqs;
    }

    /// Build synthetic ChangeEvents with sequences [from, from+count).
    static std::vector<Changefeed::ChangeEvent> makeEvents(
            uint64_t from, int count) {
        std::vector<Changefeed::ChangeEvent> evs;
        for (int i = 0; i < count; ++i) {
            Changefeed::ChangeEvent ev;
            ev.sequence     = from + static_cast<uint64_t>(i);
            ev.type         = Changefeed::ChangeEventType::EVENT_PUT;
            ev.key          = "k:" + std::to_string(i);
            ev.value        = "val";
            ev.timestamp_ms = static_cast<int64_t>(from) * 1000 + i;
            evs.push_back(ev);
        }
        return evs;
    }
};

// ============================================================================
// RAH-01: InMemoryReplayController returns events in sequence order from offset
// ============================================================================

TEST_F(ReplayAckHardeningTest, RAH01_ReplayFromSequenceReturnsOrderedEvents) {
    seedEvents(10);

    InMemoryReplayController ctrl(changefeed.get());
    auto session = ctrl.replayFromSequence(3, 7);
    ASSERT_NE(session, nullptr);

    std::vector<Changefeed::ChangeEvent> all;
    while (!session->done()) {
        auto batch = session->nextBatch();
        all.insert(all.end(), batch.begin(), batch.end());
    }

    EXPECT_FALSE(all.empty()) << "Replay from seq 3 to 7 must return events";
    for (std::size_t i = 1; i < all.size(); ++i) {
        EXPECT_LT(all[i - 1].sequence, all[i].sequence)
            << "Replay events must be in ascending sequence order";
    }
    // Sequence bounds respected.
    if (!all.empty()) {
        EXPECT_GT(all.front().sequence, 3u);
        EXPECT_LE(all.back().sequence, 7u);
    }
}

// ============================================================================
// RAH-02: InMemoryReplaySession drains completely and done() becomes true
// ============================================================================

TEST_F(ReplayAckHardeningTest, RAH02_ReplaySessionDrainsCompletely) {
    seedEvents(5);

    InMemoryReplayController ctrl(changefeed.get());
    // Use a batch size of 2 to force multiple nextBatch() calls.
    ReplayOptions opts;
    opts.from_sequence = 0;
    opts.batch_size    = 2;
    auto session = ctrl.beginReplay(opts);
    ASSERT_NE(session, nullptr);
    EXPECT_FALSE(session->done());

    int batch_count = 0;
    while (!session->done()) {
        auto batch = session->nextBatch();
        ++batch_count;
        if (batch.empty()) break; // safety guard
    }

    EXPECT_TRUE(session->done());
    EXPECT_GT(batch_count, 0);

    // After done, nextBatch must return empty.
    auto post = session->nextBatch();
    EXPECT_TRUE(post.empty())
        << "nextBatch after done() must return empty vector (contract § 3)";
}

// ============================================================================
// RAH-03: cancel() terminates the session; done() is true and nextBatch returns {}
// ============================================================================

TEST_F(ReplayAckHardeningTest, RAH03_CancelTerminatesSession) {
    seedEvents(10);

    InMemoryReplayController ctrl(changefeed.get());
    auto session = ctrl.replayFromSequence(0);
    ASSERT_NE(session, nullptr);
    EXPECT_FALSE(session->done());

    session->cancel();

    EXPECT_TRUE(session->done())
        << "done() must return true after cancel() (contract § 3)";
    EXPECT_EQ(session->state(), ReplaySessionState::Cancelled);

    auto after = session->nextBatch();
    EXPECT_TRUE(after.empty())
        << "nextBatch after cancel() must return {} (contract § 3)";
}

// ============================================================================
// RAH-04: Changefeed listEvents respects from_sequence exclusive lower bound
// ============================================================================

TEST_F(ReplayAckHardeningTest, RAH04_ListEventsRespectsFromSequenceBound) {
    auto seqs = seedEvents(8);
    // seqs[3] is the 4th assigned sequence.
    uint64_t pivot = seqs[3];

    Changefeed::ListOptions lo;
    lo.from_sequence = pivot;
    lo.limit = 100;
    auto events = changefeed->listEvents(lo);

    for (const auto& ev : events) {
        EXPECT_GT(ev.sequence, pivot)
            << "listEvents(from_sequence=" << pivot
            << ") must return only events with sequence > pivot";
    }
}

// ============================================================================
// RAH-05: DeliveryTracker acknowledgeUpTo removes all sequences ≤ boundary
// ============================================================================

TEST_F(ReplayAckHardeningTest, RAH05_AcknowledgeUpToCumulativeAck) {
    DeliveryTracker tracker;
    auto evs = makeEvents(1, 6);
    tracker.trackDelivery("consumer_rah", evs);

    // Cumulative ack up to sequence 4 (events 1,2,3,4 acknowledged).
    std::size_t removed = tracker.acknowledgeUpTo("consumer_rah", 4u);
    EXPECT_EQ(removed, 4u);

    auto stats = tracker.getStats("consumer_rah");
    ASSERT_TRUE(stats.has_value());
    EXPECT_EQ(stats->pending_count, 2u)
        << "Events with sequence 5 and 6 must remain pending";
    EXPECT_EQ(stats->total_acknowledged, 4u);
}

// ============================================================================
// RAH-06: Changefeed listEvents respects to_sequence inclusive upper bound
// ============================================================================

TEST_F(ReplayAckHardeningTest, RAH06_ListEventsRespectsToSequenceBound) {
    auto seqs = seedEvents(10);
    uint64_t upper = seqs[4]; // 5th event sequence

    Changefeed::ListOptions lo;
    lo.from_sequence = 0;
    lo.to_sequence   = upper;
    lo.limit         = 100;
    auto events = changefeed->listEvents(lo);

    for (const auto& ev : events) {
        EXPECT_LE(ev.sequence, upper)
            << "listEvents(to_sequence=" << upper
            << ") must not return events beyond upper bound";
    }
    EXPECT_FALSE(events.empty())
        << "listEvents with valid to_sequence must return at least some events";
}

// ============================================================================
// RAH-07: getPendingRedelivery with large timeout override holds events
// ============================================================================

TEST_F(ReplayAckHardeningTest, RAH07_LargeTimeoutOverrideHoldsTrackedEvents) {
    DeliveryTracker tracker;
    auto evs = makeEvents(100, 4);
    tracker.trackDelivery("consumer_rah7", evs);

    // With a very large override, events should NOT be returned for redelivery
    // because they were just tracked (no actual time elapsed).
    auto pending = tracker.getPendingRedelivery(
        "consumer_rah7", std::chrono::hours{24});

    EXPECT_TRUE(pending.empty())
        << "Freshly tracked events must not appear for redelivery under a long timeout";
}

// ============================================================================
// RAH-08: totalSessionsCreated increments per beginReplay call
// ============================================================================

TEST_F(ReplayAckHardeningTest, RAH08_TotalSessionsCreatedIncrements) {
    seedEvents(3);

    InMemoryReplayController ctrl(changefeed.get());
    EXPECT_EQ(ctrl.totalSessionsCreated(), 0u);

    auto s1 = ctrl.replayFromSequence(0);
    EXPECT_EQ(ctrl.totalSessionsCreated(), 1u);

    auto s2 = ctrl.replayFromSequence(0);
    EXPECT_EQ(ctrl.totalSessionsCreated(), 2u);

    ReplayOptions opts;
    opts.from_sequence = 0;
    auto s3 = ctrl.beginReplay(opts);
    EXPECT_EQ(ctrl.totalSessionsCreated(), 3u);
}
