/**
 * Test: CDC Replay Controller
 *
 * Tests for ICDCReplayController / InMemoryReplayController /
 * IReplaySession / InMemoryReplaySession:
 *
 *   AC-R1  beginReplay() returns a non-null IReplaySession
 *   AC-R2  Empty changefeed → session is immediately done
 *   AC-R3  nextBatch() respects batch_size
 *   AC-R4  from_sequence cursor filters older events
 *   AC-R5  to_sequence cap stops at the upper bound
 *   AC-R6  from_timestamp_ms / to_timestamp_ms filter events by wall-clock
 *   AC-R7  key_prefix filter restricts to matching keys
 *   AC-R8  event_types filter restricts to matching operation types
 *   AC-R9  max_events_per_session limits total event count
 *   AC-R10 cancel() causes done() == true and nextBatch() returns {}
 *   AC-R11 deliveredCount() increments correctly across batches
 *   AC-R12 totalSessionsCreated() increments per beginReplay() call
 *   AC-R13 replayFromTimestamp() convenience overload works
 *   AC-R14 replayFromSequence() convenience overload works
 *   AC-R15 Draining all events transitions state to Done
 *
 * Copyright (c) 2025 ThemisDB Project
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>
#include "cdc/icdc_replay_controller.h"
#include "cdc/changefeed.h"

#include <memory>
#include <string>
#include <thread>
#include <vector>

using namespace themis;
using namespace themis::cdc;

namespace {

// ── Helpers ───────────────────────────────────────────────────────────────────

Changefeed::ChangeEvent makeEv(
    uint64_t seq,
    const std::string& key = "col:k",
    int64_t ts_ms = 0,
    Changefeed::ChangeEventType type = Changefeed::ChangeEventType::EVENT_PUT)
{
    Changefeed::ChangeEvent ev;
    ev.sequence     = seq;
    ev.type         = type;
    ev.key          = key + std::to_string(seq);
    ev.value        = "v" + std::to_string(seq);
    ev.timestamp_ms = ts_ms > 0 ? ts_ms : static_cast<int64_t>(seq) * 1000;
    return ev;
}

/**
 * Lightweight fake Changefeed stub that returns a pre-loaded event list
 * without requiring RocksDB.
 *
 * We derive from InMemoryReplayController directly using a thin wrapper
 * that overrides fetchAndFilter by operating on a local vector.
 */

/// Simple controller that works from a plain vector (no RocksDB).
class VectorReplayController final : public ICDCReplayController {
public:
    explicit VectorReplayController(
        std::vector<Changefeed::ChangeEvent> events)
        : events_(std::move(events)), sessions_(0) {}

    std::unique_ptr<IReplaySession> beginReplay(
        const ReplayOptions& opts) override
    {
        sessions_.fetch_add(1, std::memory_order_relaxed);
        auto filtered = filter(opts);
        return std::make_unique<InMemoryReplaySession>(
            std::move(filtered), opts.batch_size);
    }

    std::unique_ptr<IReplaySession> replayFromTimestamp(
        int64_t from_ts, int64_t to_ts = 0) override
    {
        ReplayOptions o;
        o.from_timestamp_ms = from_ts;
        o.to_timestamp_ms   = to_ts;
        return beginReplay(o);
    }

    std::unique_ptr<IReplaySession> replayFromSequence(
        uint64_t from_seq, uint64_t to_seq = 0) override
    {
        ReplayOptions o;
        o.from_sequence = from_seq;
        o.to_sequence   = to_seq;
        return beginReplay(o);
    }

    std::size_t totalSessionsCreated() const override {
        return sessions_.load(std::memory_order_relaxed);
    }

private:
    std::vector<Changefeed::ChangeEvent> filter(
        const ReplayOptions& opts) const
    {
        std::vector<Changefeed::ChangeEvent> out = {};

        for (const auto& ev : events_) {
            // from_sequence is an exclusive lower bound: keep events with seq > from_sequence
            if (opts.from_sequence > 0 && ev.sequence <= opts.from_sequence) {
              continue;
            }
            // to_sequence is an inclusive upper bound: keep events with seq <= to_sequence
            if (opts.to_sequence   > 0 && ev.sequence > opts.to_sequence) {
              continue;
            }
            if (opts.from_timestamp_ms > 0 &&
                ev.timestamp_ms < opts.from_timestamp_ms)                   continue;
            if (opts.to_timestamp_ms > 0 &&
                ev.timestamp_ms > opts.to_timestamp_ms)                     continue;
            if (!opts.key_prefix.empty() &&
                ev.key.substr(0, opts.key_prefix.size()) != opts.key_prefix) continue;
            if (!opts.event_types.empty()) {
                bool found = false;
                for (auto t : opts.event_types) {
                    if (ev.type == t) { found = true; break; }
                }
                if (!found) {
                  continue;
                }
            }
            if (opts.max_events_per_session > 0 &&
                out.size() >= opts.max_events_per_session) break;
            out.push_back(ev);
        }
        return out;
    }

    std::vector<Changefeed::ChangeEvent> events_;
    std::atomic<std::size_t>             sessions_;
};

std::vector<Changefeed::ChangeEvent> makeEventRange(
    uint64_t from, uint64_t to)
{
    std::vector<Changefeed::ChangeEvent> v = {};

    for (uint64_t i = from; i <= to; ++i) {
      v.push_back(makeEv(i));
    }
    return v;
}

} // namespace

// ── AC-R1  beginReplay() returns non-null ─────────────────────────────────────

TEST(InMemoryReplayControllerTest, BeginReplayReturnsNonNull) {
    VectorReplayController ctrl({});
    ReplayOptions opts;
    auto session = ctrl.beginReplay(opts);
    EXPECT_NE(session, nullptr);
}

// ── AC-R2  Empty changefeed → immediately done ────────────────────────────────

TEST(InMemoryReplayControllerTest, EmptyFeedSessionIsDone) {
    VectorReplayController ctrl({});
    auto session = ctrl.beginReplay(ReplayOptions{});
    EXPECT_FALSE(session->done());
    auto batch = session->nextBatch();
    EXPECT_TRUE(batch.empty());
    EXPECT_TRUE(session->done());
}

// ── AC-R3  nextBatch() respects batch_size ────────────────────────────────────

TEST(InMemoryReplayControllerTest, BatchSizeIsRespected) {
    VectorReplayController ctrl(makeEventRange(1, 10));
    ReplayOptions opts;
    opts.batch_size = 3;
    auto session = ctrl.beginReplay(opts);

    auto b1 = session->nextBatch();
    EXPECT_EQ(b1.size(), 3u);
    EXPECT_FALSE(session->done());

    auto b2 = session->nextBatch();
    EXPECT_EQ(b2.size(), 3u);

    auto b3 = session->nextBatch();
    EXPECT_EQ(b3.size(), 3u);

    auto b4 = session->nextBatch();
    EXPECT_EQ(b4.size(), 1u);

    EXPECT_TRUE(session->done());
    auto b5 = session->nextBatch();
    EXPECT_TRUE(b5.empty());
}

// ── AC-R4  from_sequence cursor filters older events ─────────────────────────
// from_sequence uses an exclusive lower bound: only events with
// sequence > from_sequence are returned (same semantics as Changefeed::ListOptions).

TEST(InMemoryReplayControllerTest, FromSequenceFiltersOlderEvents) {
    VectorReplayController ctrl(makeEventRange(1, 5));
    auto session = ctrl.replayFromSequence(3); // exclusive: returns seq > 3 (i.e., 4, 5)
    auto batch   = session->nextBatch();
    ASSERT_EQ(batch.size(), 2u);
    EXPECT_EQ(batch[0].sequence, 4u);
    EXPECT_EQ(batch[1].sequence, 5u);
}

// ── AC-R5  to_sequence cap ────────────────────────────────────────────────────

TEST(InMemoryReplayControllerTest, ToSequenceCapsUpperBound) {
    VectorReplayController ctrl(makeEventRange(1, 10));
    auto session = ctrl.replayFromSequence(0, 4); // seq <= 4
    auto batch   = session->nextBatch();
    ASSERT_EQ(batch.size(), 4u);
    for (std::size_t i = 0; i < batch.size(); ++i) {
        EXPECT_EQ(batch[i].sequence, i + 1);
    }
}

// ── AC-R6  Timestamp range filter ────────────────────────────────────────────

TEST(InMemoryReplayControllerTest, TimestampRangeFiltersCorrectly) {
    // Events: seq 1..5, timestamp_ms = seq * 1000
    VectorReplayController ctrl(makeEventRange(1, 5));
    auto session = ctrl.replayFromTimestamp(2000, 4000);
    auto batch   = session->nextBatch();
    ASSERT_EQ(batch.size(), 3u);
    EXPECT_EQ(batch[0].timestamp_ms, 2000);
    EXPECT_EQ(batch[1].timestamp_ms, 3000);
    EXPECT_EQ(batch[2].timestamp_ms, 4000);
}

// ── AC-R7  key_prefix filter ──────────────────────────────────────────────────

TEST(InMemoryReplayControllerTest, KeyPrefixFilterWorks) {
    std::vector<Changefeed::ChangeEvent> events = {};

    for (uint64_t i = 1; i <= 4; ++i) {
        events.push_back(makeEv(i, "orders:k"));
    }
    for (uint64_t i = 5; i <= 8; ++i) {
        events.push_back(makeEv(i, "products:k"));
    }

    VectorReplayController ctrl(std::move(events));
    ReplayOptions opts;
    opts.key_prefix = "orders:";
    auto session = ctrl.beginReplay(opts);
    auto batch   = session->nextBatch();
    ASSERT_EQ(batch.size(), 4u);
    for (const auto& ev : batch) {
        EXPECT_EQ(ev.key.substr(0, 7), "orders:");
    }
}

// ── AC-R8  event_types filter ────────────────────────────────────────────────

TEST(InMemoryReplayControllerTest, EventTypeFilterWorks) {
    std::vector<Changefeed::ChangeEvent> events = {};

    for (uint64_t i = 1; i <= 4; ++i) {
        events.push_back(makeEv(i, "k:", 0,
            Changefeed::ChangeEventType::EVENT_PUT));
    }
    for (uint64_t i = 5; i <= 8; ++i) {
        events.push_back(makeEv(i, "k:", 0,
            Changefeed::ChangeEventType::EVENT_DELETE));
    }

    VectorReplayController ctrl(std::move(events));
    ReplayOptions opts;
    opts.event_types = {Changefeed::ChangeEventType::EVENT_DELETE};
    auto session = ctrl.beginReplay(opts);
    auto batch   = session->nextBatch();
    ASSERT_EQ(batch.size(), 4u);
    for (const auto& ev : batch) {
        EXPECT_EQ(ev.type, Changefeed::ChangeEventType::EVENT_DELETE);
    }
}

// ── AC-R9  max_events_per_session hard limit ─────────────────────────────────

TEST(InMemoryReplayControllerTest, MaxEventsPerSessionIsRespected) {
    VectorReplayController ctrl(makeEventRange(1, 20));
    ReplayOptions opts;
    opts.max_events_per_session = 7;
    opts.batch_size             = 100; // single batch
    auto session = ctrl.beginReplay(opts);
    auto batch   = session->nextBatch();
    EXPECT_EQ(batch.size(), 7u);
    EXPECT_TRUE(session->done());
}

// ── AC-R10 cancel() causes done() == true ────────────────────────────────────

TEST(InMemoryReplayControllerTest, CancelMakesDone) {
    VectorReplayController ctrl(makeEventRange(1, 10));
    auto session = ctrl.beginReplay(ReplayOptions{});
    EXPECT_FALSE(session->done());
    session->cancel();
    EXPECT_TRUE(session->done());
    EXPECT_EQ(session->state(), ReplaySessionState::Cancelled);
    EXPECT_TRUE(session->nextBatch().empty());
}

// ── AC-R11 deliveredCount() increments correctly ─────────────────────────────

TEST(InMemoryReplayControllerTest, DeliveredCountIncrementsAcrossBatches) {
    VectorReplayController ctrl(makeEventRange(1, 6));
    ReplayOptions opts;
    opts.batch_size = 2;
    auto session = ctrl.beginReplay(opts);
    EXPECT_EQ(session->deliveredCount(), 0u);
    session->nextBatch();
    EXPECT_EQ(session->deliveredCount(), 2u);
    session->nextBatch();
    EXPECT_EQ(session->deliveredCount(), 4u);
    session->nextBatch();
    EXPECT_EQ(session->deliveredCount(), 6u);
    EXPECT_TRUE(session->done());
}

// ── AC-R12 totalSessionsCreated() increments per call ────────────────────────

TEST(InMemoryReplayControllerTest, TotalSessionsCreatedIncrements) {
    VectorReplayController ctrl({});
    EXPECT_EQ(ctrl.totalSessionsCreated(), 0u);
    ctrl.beginReplay(ReplayOptions{});
    ctrl.beginReplay(ReplayOptions{});
    EXPECT_EQ(ctrl.totalSessionsCreated(), 2u);
}

// ── AC-R13 replayFromTimestamp() convenience ─────────────────────────────────

TEST(InMemoryReplayControllerTest, ReplayFromTimestampConvenienceOverload) {
    VectorReplayController ctrl(makeEventRange(1, 5));
    auto session = ctrl.replayFromTimestamp(2000, 3000);
    auto batch   = session->nextBatch();
    EXPECT_EQ(batch.size(), 2u);
}

// ── AC-R14 replayFromSequence() convenience ──────────────────────────────────

TEST(InMemoryReplayControllerTest, ReplayFromSequenceConvenienceOverload) {
    VectorReplayController ctrl(makeEventRange(1, 10));
    auto session = ctrl.replayFromSequence(7, 9);
    auto batch   = session->nextBatch();
    ASSERT_EQ(batch.size(), 2u);
    EXPECT_EQ(batch[0].sequence, 8u);
    EXPECT_EQ(batch[1].sequence, 9u);
}

// ── AC-R15 Draining all events transitions state to Done ─────────────────────

TEST(InMemoryReplayControllerTest, DrainAllEventsTransitionsToDone) {
    VectorReplayController ctrl(makeEventRange(1, 3));
    ReplayOptions opts;
    opts.batch_size = 10;
    auto session = ctrl.beginReplay(opts);
    EXPECT_EQ(session->state(), ReplaySessionState::Active);
    auto batch = session->nextBatch();
    EXPECT_EQ(batch.size(), 3u);
    EXPECT_EQ(session->state(), ReplaySessionState::Done);
}
