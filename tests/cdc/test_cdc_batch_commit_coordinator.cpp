/**
 * Test: CDC Batch Commit Coordinator
 *
 * Tests for ICDCBatchCommitCoordinator / InMemoryBatchCommitCoordinator:
 *
 *   AC-BC1  beginBatch() returns a non-zero BatchId
 *   AC-BC2  beginBatch() while a batch is open returns 0
 *   AC-BC3  addEvent() returns NoBatchOpen when no batch is open
 *   AC-BC4  addEvent() returns Added for events within limit
 *   AC-BC5  addEvent() returns BatchFull when max_batch_size is reached
 *   AC-BC6  commitBatch() commits all staged events
 *   AC-BC7  committedEvents() returns events in commit order
 *   AC-BC8  isCommitted() reflects committed batch IDs
 *   AC-BC9  commitBatch() returns NoBatchOpen when no batch is open
 *   AC-BC10 rollbackBatch() clears staged events; status → RolledBack
 *   AC-BC11 rollbackBatch() returns NoBatchOpen when no batch is open
 *   AC-BC12 After commit, beginBatch() succeeds again (idle → open)
 *   AC-BC13 After rollback, beginBatch() succeeds again
 *   AC-BC14 info() reflects current state accurately
 *   AC-BC15 Commit history FIFO eviction when limit is reached
 *   AC-BC16 Thread-safety: concurrent addEvent calls
 *
 * Copyright (c) 2025 ThemisDB Project
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>
#include "cdc/icdc_batch_commit_coordinator.h"
#include "cdc/changefeed.h"

#include <atomic>
#include <memory>
#include <string>
#include <thread>
#include <vector>

using namespace themis;
using namespace themis::cdc;

namespace {

Changefeed::ChangeEvent makeEv(uint64_t seq, const std::string& key = "k:") {
    Changefeed::ChangeEvent ev;
    ev.sequence     = seq;
    ev.type         = Changefeed::ChangeEventType::EVENT_PUT;
    ev.key          = key + std::to_string(seq);
    ev.value        = "v" + std::to_string(seq);
    ev.timestamp_ms = static_cast<int64_t>(seq) * 1000;
    return ev;
}

} // namespace

// ── AC-BC1  beginBatch() returns non-zero BatchId ────────────────────────────

TEST(InMemoryBatchCommitCoordinatorTest, BeginBatchReturnsNonZeroId) {
    InMemoryBatchCommitCoordinator coord;
    BatchId id = coord.beginBatch();
    EXPECT_NE(id, 0u);
    EXPECT_EQ(coord.status(), BatchStatus::Open);
}

// ── AC-BC2  beginBatch() while open returns 0 ────────────────────────────────

TEST(InMemoryBatchCommitCoordinatorTest, BeginBatchWhileOpenReturnsZero) {
    InMemoryBatchCommitCoordinator coord;
    coord.beginBatch();
    BatchId second = coord.beginBatch();
    EXPECT_EQ(second, 0u);
}

// ── AC-BC3  addEvent() returns NoBatchOpen ────────────────────────────────────

TEST(InMemoryBatchCommitCoordinatorTest, AddEventNoBatchOpen) {
    InMemoryBatchCommitCoordinator coord;
    EXPECT_EQ(coord.addEvent(makeEv(1)), AddEventResult::NoBatchOpen);
}

// ── AC-BC4  addEvent() returns Added ─────────────────────────────────────────

TEST(InMemoryBatchCommitCoordinatorTest, AddEventAdded) {
    InMemoryBatchCommitCoordinator coord;
    coord.beginBatch();
    EXPECT_EQ(coord.addEvent(makeEv(1)), AddEventResult::Added);
    EXPECT_EQ(coord.addEvent(makeEv(2)), AddEventResult::Added);
    EXPECT_EQ(coord.info().pending_event_count, 2u);
}

// ── AC-BC5  addEvent() returns BatchFull ─────────────────────────────────────

TEST(InMemoryBatchCommitCoordinatorTest, AddEventBatchFull) {
    BatchConfig cfg;
    cfg.max_batch_size = 2;
    InMemoryBatchCommitCoordinator coord(cfg);
    coord.beginBatch();
    EXPECT_EQ(coord.addEvent(makeEv(1)), AddEventResult::Added);
    EXPECT_EQ(coord.addEvent(makeEv(2)), AddEventResult::Added);
    EXPECT_EQ(coord.addEvent(makeEv(3)), AddEventResult::BatchFull);
    EXPECT_EQ(coord.info().pending_event_count, 2u);
}

// ── AC-BC6  commitBatch() commits staged events ──────────────────────────────

TEST(InMemoryBatchCommitCoordinatorTest, CommitBatchCommitsEvents) {
    InMemoryBatchCommitCoordinator coord;
    BatchId id = coord.beginBatch();
    coord.addEvent(makeEv(1));
    coord.addEvent(makeEv(2));

    EXPECT_EQ(coord.commitBatch(), CommitResult::Committed);
    EXPECT_EQ(coord.status(), BatchStatus::Committed);
    EXPECT_EQ(coord.info().total_committed_batches, 1u);
    EXPECT_TRUE(coord.isCommitted(id));
}

// ── AC-BC7  committedEvents() returns events in order ────────────────────────

TEST(InMemoryBatchCommitCoordinatorTest, CommittedEventsReturnedInOrder) {
    InMemoryBatchCommitCoordinator coord;
    BatchId id = coord.beginBatch();
    coord.addEvent(makeEv(10));
    coord.addEvent(makeEv(20));
    coord.addEvent(makeEv(30));
    coord.commitBatch();

    auto evs = coord.committedEvents(id);
    ASSERT_EQ(evs.size(), 3u);
    EXPECT_EQ(evs[0].sequence, 10u);
    EXPECT_EQ(evs[1].sequence, 20u);
    EXPECT_EQ(evs[2].sequence, 30u);
}

TEST(InMemoryBatchCommitCoordinatorTest, CommittedEventsUnknownIdReturnsEmpty) {
    InMemoryBatchCommitCoordinator coord;
    EXPECT_TRUE(coord.committedEvents(9999).empty());
}

// ── AC-BC8  isCommitted() reflects committed batch IDs ───────────────────────

TEST(InMemoryBatchCommitCoordinatorTest, IsCommittedReflectsState) {
    InMemoryBatchCommitCoordinator coord;
    BatchId id = coord.beginBatch();
    coord.addEvent(makeEv(1));
    EXPECT_FALSE(coord.isCommitted(id));
    coord.commitBatch();
    EXPECT_TRUE(coord.isCommitted(id));
}

// ── AC-BC9  commitBatch() NoBatchOpen ────────────────────────────────────────

TEST(InMemoryBatchCommitCoordinatorTest, CommitBatchNoBatchOpen) {
    InMemoryBatchCommitCoordinator coord;
    EXPECT_EQ(coord.commitBatch(), CommitResult::NoBatchOpen);
}

// ── AC-BC10 rollbackBatch() ───────────────────────────────────────────────────

TEST(InMemoryBatchCommitCoordinatorTest, RollbackBatchClearsEvents) {
    InMemoryBatchCommitCoordinator coord;
    coord.beginBatch();
    coord.addEvent(makeEv(1));
    EXPECT_EQ(coord.rollbackBatch(), RollbackResult::RolledBack);
    EXPECT_EQ(coord.status(), BatchStatus::RolledBack);
    EXPECT_EQ(coord.info().total_rolled_back, 1u);
    EXPECT_EQ(coord.info().pending_event_count, 0u);
}

// ── AC-BC11 rollbackBatch() NoBatchOpen ──────────────────────────────────────

TEST(InMemoryBatchCommitCoordinatorTest, RollbackBatchNoBatchOpen) {
    InMemoryBatchCommitCoordinator coord;
    EXPECT_EQ(coord.rollbackBatch(), RollbackResult::NoBatchOpen);
}

// ── AC-BC12 After commit, beginBatch() succeeds again ────────────────────────

TEST(InMemoryBatchCommitCoordinatorTest, BeginBatchSucceedsAfterCommit) {
    InMemoryBatchCommitCoordinator coord;
    coord.beginBatch();
    coord.addEvent(makeEv(1));
    coord.commitBatch();

    BatchId id2 = coord.beginBatch();
    EXPECT_NE(id2, 0u);
    EXPECT_EQ(coord.status(), BatchStatus::Open);
}

// ── AC-BC13 After rollback, beginBatch() succeeds again ──────────────────────

TEST(InMemoryBatchCommitCoordinatorTest, BeginBatchSucceedsAfterRollback) {
    InMemoryBatchCommitCoordinator coord;
    coord.beginBatch();
    coord.rollbackBatch();

    BatchId id2 = coord.beginBatch();
    EXPECT_NE(id2, 0u);
    EXPECT_EQ(coord.status(), BatchStatus::Open);
}

// ── AC-BC14 info() reflects state ────────────────────────────────────────────

TEST(InMemoryBatchCommitCoordinatorTest, InfoReflectsCurrentState) {
    InMemoryBatchCommitCoordinator coord;
    auto i0 = coord.info();
    EXPECT_EQ(i0.status, BatchStatus::Idle);
    EXPECT_EQ(i0.total_committed_batches, 0u);
    EXPECT_EQ(i0.total_rolled_back, 0u);

    BatchId id = coord.beginBatch();
    coord.addEvent(makeEv(5));
    coord.addEvent(makeEv(6));

    auto i1 = coord.info();
    EXPECT_EQ(i1.status, BatchStatus::Open);
    EXPECT_EQ(i1.current_batch_id, id);
    EXPECT_EQ(i1.pending_event_count, 2u);

    coord.commitBatch();
    auto i2 = coord.info();
    EXPECT_EQ(i2.status, BatchStatus::Committed);
    EXPECT_EQ(i2.total_committed_batches, 1u);
    EXPECT_EQ(i2.pending_event_count, 0u);
}

// ── AC-BC15 Commit history FIFO eviction ─────────────────────────────────────

TEST(InMemoryBatchCommitCoordinatorTest, CommitHistoryFifoEviction) {
    BatchConfig cfg;
    cfg.commit_history_size = 3; // keep only 3 committed batches
    InMemoryBatchCommitCoordinator coord(cfg);

    std::vector<BatchId> ids;
    for (int i = 0; i < 5; ++i) {
        BatchId id = coord.beginBatch();
        ids.push_back(id);
        coord.addEvent(makeEv(static_cast<uint64_t>(i + 1)));
        coord.commitBatch();
    }

    // Oldest 2 batches should have been evicted
    EXPECT_FALSE(coord.isCommitted(ids[0]));
    EXPECT_FALSE(coord.isCommitted(ids[1]));
    // Most recent 3 should still be present
    EXPECT_TRUE(coord.isCommitted(ids[2]));
    EXPECT_TRUE(coord.isCommitted(ids[3]));
    EXPECT_TRUE(coord.isCommitted(ids[4]));
}

// ── AC-BC16 Thread-safety: concurrent addEvent calls ─────────────────────────

TEST(InMemoryBatchCommitCoordinatorTest, ConcurrentAddEventThreadSafe) {
    InMemoryBatchCommitCoordinator coord;
    coord.beginBatch();

    constexpr int kThreads = 4;
    constexpr int kEventsPerThread = 50;
    std::atomic<int> added{0};

    std::vector<std::thread> threads;
    threads.reserve(kThreads);
    for (int t = 0; t < kThreads; ++t) {
        threads.emplace_back([&, t] {
            for (int i = 0; i < kEventsPerThread; ++i) {
                auto r = coord.addEvent(
                    makeEv(static_cast<uint64_t>(t * 1000 + i)));
                if (r == AddEventResult::Added) {
                    added.fetch_add(1, std::memory_order_relaxed);
                }
            }
        });
    }
    for (auto& th : threads) {
      th.join();
    }

    // No assertion on exact count (batch may be full), but no crash
    EXPECT_GE(added.load(), 1);
    EXPECT_EQ(coord.status(), BatchStatus::Open);

    coord.commitBatch();
    EXPECT_EQ(coord.status(), BatchStatus::Committed);
}

// ── ICDCBatchCommitCoordinator polymorphic usage ──────────────────────────────

TEST(InMemoryBatchCommitCoordinatorTest, PolymorphicUsage) {
    std::unique_ptr<ICDCBatchCommitCoordinator> coord =
        std::make_unique<InMemoryBatchCommitCoordinator>();

    BatchId id = coord->beginBatch();
    EXPECT_NE(id, 0u);
    coord->addEvent(makeEv(42));
    EXPECT_EQ(coord->commitBatch(), CommitResult::Committed);
    EXPECT_TRUE(coord->isCommitted(id));

    auto evs = coord->committedEvents(id);
    ASSERT_EQ(evs.size(), 1u);
    EXPECT_EQ(evs[0].sequence, 42u);
}
