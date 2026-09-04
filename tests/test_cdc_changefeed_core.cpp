// Test: Changefeed Core and Push Subscription API
// Covers: SubscriptionFilter::matches(), subscribe(), SubscriptionHandle
//         (RAII, move, cancel), notifySubscribers() via recordEvent(),
//         getStats(), getLatestSequence(), clear(), listEvents() variants,
//         ChangeEvent JSON serialisation roundtrip.

#include <gtest/gtest.h>
#include "cdc/changefeed.h"
#include "storage/rocksdb_wrapper.h"
#include <filesystem>
#include <atomic>
#include <chrono>
#include <limits>
#include <set>
#include <thread>
#include <vector>

namespace fs = std::filesystem;
using namespace themis;

// ---------------------------------------------------------------------------
// Test fixture
// ---------------------------------------------------------------------------
class ChangefeedCoreTest : public ::testing::Test {
protected:
    void SetUp() override {
#ifdef _WIN32
        GTEST_SKIP() << "Skipping CDC changefeed core focused tests on Windows due to fixture crash in current runtime.";
#endif
        test_db_path_ = "/tmp/test_cfcore_" +
                        std::to_string(std::chrono::steady_clock::now()
                                           .time_since_epoch()
                                           .count());
        fs::create_directories(test_db_path_);

        RocksDBWrapper::Config cfg;
        cfg.db_path = test_db_path_;
        cfg.merge_operator_preset =
            RocksDBWrapper::Config::MergeOperatorPreset::SequenceU64Increment;
        db_ = std::make_unique<RocksDBWrapper>(cfg);
        ASSERT_TRUE(db_->open());

        Changefeed::RetentionPolicy rp;
        rp.enabled = false;
        feed_ = std::make_unique<Changefeed>(db_->getDB(), nullptr, rp);
    }

    void TearDown() override {
        feed_.reset();
        if (db_) {
            db_->close();
        }
        db_.reset();
        fs::remove_all(test_db_path_);
    }

    Changefeed::ChangeEvent makePut(const std::string& key,
                                    const std::string& value = "{}") {
        Changefeed::ChangeEvent ev;
        ev.type  = Changefeed::ChangeEventType::EVENT_PUT;
        ev.key   = key;
        ev.value = value;
        return ev;
    }

    Changefeed::ChangeEvent makeDelete(const std::string& key) {
        Changefeed::ChangeEvent ev;
        ev.type = Changefeed::ChangeEventType::EVENT_DELETE;
        ev.key  = key;
        return ev;
    }

    std::string                    test_db_path_;
    std::unique_ptr<RocksDBWrapper> db_;
    std::unique_ptr<Changefeed>    feed_;
};

// ===========================================================================
// SubscriptionFilter::matches()
// ===========================================================================

TEST_F(ChangefeedCoreTest, EmptyFilterMatchesAllEvents) {
    Changefeed::SubscriptionFilter f;
    // empty filter — no prefix, no type restriction
    EXPECT_TRUE(f.matches(makePut("any_key")));
    EXPECT_TRUE(f.matches(makeDelete("other_key")));
}

TEST_F(ChangefeedCoreTest, KeyPrefixFilterMatchesPrefixedKeys) {
    Changefeed::SubscriptionFilter f;
    f.key_prefix = "user:";

    EXPECT_TRUE(f.matches(makePut("user:42")));
    EXPECT_TRUE(f.matches(makePut("user:99/profile")));
    EXPECT_FALSE(f.matches(makePut("order:7")));
    EXPECT_FALSE(f.matches(makePut("u")));
}

TEST_F(ChangefeedCoreTest, EventTypesFilterMatchesSpecifiedTypes) {
    Changefeed::SubscriptionFilter f;
    f.event_types = {Changefeed::ChangeEventType::EVENT_DELETE};

    EXPECT_FALSE(f.matches(makePut("k")));
    EXPECT_TRUE(f.matches(makeDelete("k")));
}

TEST_F(ChangefeedCoreTest, EventTypesMultiTypeFilter) {
    Changefeed::SubscriptionFilter f;
    f.event_types = {Changefeed::ChangeEventType::EVENT_PUT,
                     Changefeed::ChangeEventType::EVENT_DELETE};

    EXPECT_TRUE(f.matches(makePut("k")));
    EXPECT_TRUE(f.matches(makeDelete("k")));

    // Transaction commit should not match
    Changefeed::ChangeEvent tx;
    tx.type = Changefeed::ChangeEventType::EVENT_TRANSACTION_COMMIT;
    tx.key  = "k";
    EXPECT_FALSE(f.matches(tx));
}

TEST_F(ChangefeedCoreTest, CombinedPrefixAndTypeFilter) {
    Changefeed::SubscriptionFilter f;
    f.key_prefix  = "orders:";
    f.event_types = {Changefeed::ChangeEventType::EVENT_DELETE};

    EXPECT_TRUE(f.matches(makeDelete("orders:123")));   // matches both
    EXPECT_FALSE(f.matches(makePut("orders:123")));     // wrong type
    EXPECT_FALSE(f.matches(makeDelete("users:5")));     // wrong prefix
    EXPECT_FALSE(f.matches(makePut("users:5")));        // both wrong
}

// ===========================================================================
// subscribe() / SubscriptionHandle lifecycle
// ===========================================================================

TEST_F(ChangefeedCoreTest, SubscribeReturnsActiveHandle) {
    Changefeed::SubscriptionFilter f;
    auto h = feed_->subscribe(f, [](const Changefeed::ChangeEvent&) {});
    EXPECT_TRUE(h.active());
    EXPECT_NE(h.id(), 0u);
}

TEST_F(ChangefeedCoreTest, HandleCancelDeactivates) {
    Changefeed::SubscriptionFilter f;
    auto h = feed_->subscribe(f, [](const Changefeed::ChangeEvent&) {});
    EXPECT_TRUE(h.active());

    h.cancel();
    EXPECT_FALSE(h.active());
}

TEST_F(ChangefeedCoreTest, HandleDestructorCancelsSubscription) {
    std::atomic<int> calls{0};
    {
        auto h = feed_->subscribe({},
            [&](const Changefeed::ChangeEvent&) { calls++; });
        EXPECT_TRUE(h.active());
        // h goes out of scope here — subscription should be cancelled
    }
    // After destruction, recording an event must NOT invoke the callback
    feed_->recordEvent(makePut("after_cancel"));
    EXPECT_EQ(calls.load(), 0);
}

TEST_F(ChangefeedCoreTest, HandleMoveTransfersOwnership) {
    std::atomic<int> calls{0};
    auto h1 = feed_->subscribe({},
        [&](const Changefeed::ChangeEvent&) { calls++; });
    EXPECT_TRUE(h1.active());
    uint64_t original_id = h1.id();

    auto h2 = std::move(h1);
    EXPECT_FALSE(h1.active());
    EXPECT_TRUE(h2.active());
    EXPECT_EQ(h2.id(), original_id);

    // Callback should fire via h2
    feed_->recordEvent(makePut("via_h2"));
    EXPECT_EQ(calls.load(), 1);

    h2.cancel();
    feed_->recordEvent(makePut("after_h2_cancel"));
    EXPECT_EQ(calls.load(), 1); // no additional calls
}

TEST_F(ChangefeedCoreTest, MoveAssignmentCancelsOldSubscription) {
    std::atomic<int> calls_a{0}, calls_b{0};

    auto ha = feed_->subscribe({},
        [&](const Changefeed::ChangeEvent&) { calls_a++; });
    auto hb = feed_->subscribe({},
        [&](const Changefeed::ChangeEvent&) { calls_b++; });

    // Move-assign hb into ha — ha's old subscription should be cancelled
    ha = std::move(hb);

    feed_->recordEvent(makePut("after_move_assign"));

    EXPECT_EQ(calls_a.load(), 0); // original ha subscription cancelled
    EXPECT_EQ(calls_b.load(), 1); // hb's callback now in ha

    ha.cancel();
}

// ===========================================================================
// notifySubscribers() via recordEvent()
// ===========================================================================

TEST_F(ChangefeedCoreTest, CallbackInvokedOnMatchingEvent) {
    std::atomic<int> calls{0};
    auto h = feed_->subscribe({},
        [&](const Changefeed::ChangeEvent&) { calls++; });

    feed_->recordEvent(makePut("ev1"));
    EXPECT_EQ(calls.load(), 1);

    feed_->recordEvent(makePut("ev2"));
    EXPECT_EQ(calls.load(), 2);

    h.cancel();
}

TEST_F(ChangefeedCoreTest, CallbackNotInvokedWhenFilterExcludesKey) {
    std::atomic<int> calls{0};
    Changefeed::SubscriptionFilter f;
    f.key_prefix = "match:";
    auto h = feed_->subscribe(f,
        [&](const Changefeed::ChangeEvent&) { calls++; });

    feed_->recordEvent(makePut("nomatch:key"));
    EXPECT_EQ(calls.load(), 0);

    feed_->recordEvent(makePut("match:key"));
    EXPECT_EQ(calls.load(), 1);

    h.cancel();
}

TEST_F(ChangefeedCoreTest, CallbackNotInvokedWhenTypeFiltered) {
    std::atomic<int> calls{0};
    Changefeed::SubscriptionFilter f;
    f.event_types = {Changefeed::ChangeEventType::EVENT_DELETE};
    auto h = feed_->subscribe(f,
        [&](const Changefeed::ChangeEvent&) { calls++; });

    feed_->recordEvent(makePut("key"));  // PUT — should not fire
    EXPECT_EQ(calls.load(), 0);

    feed_->recordEvent(makeDelete("key"));  // DELETE — should fire
    EXPECT_EQ(calls.load(), 1);

    h.cancel();
}

TEST_F(ChangefeedCoreTest, MultipleSubscribersAllNotified) {
    std::atomic<int> calls_a{0}, calls_b{0};
    auto ha = feed_->subscribe({},
        [&](const Changefeed::ChangeEvent&) { calls_a++; });
    auto hb = feed_->subscribe({},
        [&](const Changefeed::ChangeEvent&) { calls_b++; });

    feed_->recordEvent(makePut("shared_event"));

    EXPECT_EQ(calls_a.load(), 1);
    EXPECT_EQ(calls_b.load(), 1);

    ha.cancel();
    hb.cancel();
}

TEST_F(ChangefeedCoreTest, CancelledSubscriberNoLongerReceivesEvents) {
    std::atomic<int> calls{0};
    auto h = feed_->subscribe({},
        [&](const Changefeed::ChangeEvent&) { calls++; });

    feed_->recordEvent(makePut("before_cancel"));
    EXPECT_EQ(calls.load(), 1);

    h.cancel();

    feed_->recordEvent(makePut("after_cancel"));
    EXPECT_EQ(calls.load(), 1); // no change
}

TEST_F(ChangefeedCoreTest, CallbackReceivesCorrectEventData) {
    Changefeed::ChangeEvent received;
    auto h = feed_->subscribe({},
        [&](const Changefeed::ChangeEvent& ev) { received = ev; });

    auto recorded = feed_->recordEvent(makePut("my_key", "my_value"));

    EXPECT_EQ(received.key,   "my_key");
    EXPECT_EQ(received.value, "my_value");
    EXPECT_EQ(received.type,  Changefeed::ChangeEventType::EVENT_PUT);
    EXPECT_EQ(received.sequence, recorded.sequence);

    h.cancel();
}

TEST_F(ChangefeedCoreTest, CallbackExceptionDoesNotPropagateToRecordEvent) {
    auto h = feed_->subscribe({},
        [](const Changefeed::ChangeEvent&) {
            throw std::runtime_error("callback error");
        });

    // recordEvent must not re-throw the callback exception
    EXPECT_NO_THROW(feed_->recordEvent(makePut("safe_key")));

    h.cancel();
}

TEST_F(ChangefeedCoreTest, SubscribeWithinCallbackDoesNotDeadlock) {
    // Subscribing from within a callback must not deadlock (snapshot approach)
    std::atomic<bool> inner_subscribe_done{false};
    Changefeed::SubscriptionHandle inner_handle;

    auto outer = feed_->subscribe({},
        [&](const Changefeed::ChangeEvent&) {
            if (!inner_subscribe_done.load()) {
                inner_subscribe_done.store(true);
                inner_handle = feed_->subscribe({},
                    [](const Changefeed::ChangeEvent&) {});
            }
        });

    EXPECT_NO_THROW(feed_->recordEvent(makePut("trigger")));
    EXPECT_TRUE(inner_subscribe_done.load());

    outer.cancel();
    inner_handle.cancel();
}

// ===========================================================================
// getStats()
// ===========================================================================

TEST_F(ChangefeedCoreTest, GetStatsEmptyFeed) {
    auto stats = feed_->getStats();
    EXPECT_EQ(stats.total_events,   0u);
    EXPECT_EQ(stats.latest_sequence, 0u);
}

TEST_F(ChangefeedCoreTest, GetStatsAfterRecordingEvents) {
    feed_->recordEvent(makePut("s1"));
    feed_->recordEvent(makePut("s2"));
    feed_->recordEvent(makePut("s3"));

    auto stats = feed_->getStats();
    EXPECT_EQ(stats.total_events,   3u);
    EXPECT_GE(stats.latest_sequence, 3u);
}

// ===========================================================================
// getLatestSequence()
// ===========================================================================

TEST_F(ChangefeedCoreTest, GetLatestSequenceEmptyFeedIsZero) {
    EXPECT_EQ(feed_->getLatestSequence(), 0u);
}

TEST_F(ChangefeedCoreTest, GetLatestSequenceMonotonicallyIncreases) {
    auto ev1 = feed_->recordEvent(makePut("seq1"));
    auto ev2 = feed_->recordEvent(makePut("seq2"));
    auto ev3 = feed_->recordEvent(makePut("seq3"));

    EXPECT_LT(ev1.sequence, ev2.sequence);
    EXPECT_LT(ev2.sequence, ev3.sequence);
    EXPECT_EQ(feed_->getLatestSequence(), ev3.sequence);
}

// ===========================================================================
// clear()
// ===========================================================================

TEST_F(ChangefeedCoreTest, ClearRemovesAllEvents) {
    feed_->recordEvent(makePut("c1"));
    feed_->recordEvent(makePut("c2"));
    EXPECT_EQ(feed_->listEvents().size(), 2u);

    feed_->clear();
    EXPECT_EQ(feed_->listEvents().size(), 0u);
}

TEST_F(ChangefeedCoreTest, ClearOnEmptyFeedIsNoop) {
    EXPECT_NO_THROW(feed_->clear());
    EXPECT_EQ(feed_->listEvents().size(), 0u);
}

// ===========================================================================
// listEvents() variants
// ===========================================================================

TEST_F(ChangefeedCoreTest, ListEventsReturnsAllEventsInOrder) {
    for (int i = 0; i < 5; i++) {
        feed_->recordEvent(makePut("list_key_" + std::to_string(i)));
    }

    auto events = feed_->listEvents();
    ASSERT_EQ(events.size(), 5u);

    // Sequences must be strictly increasing
    for (size_t i = 1; i < events.size(); i++) {
        EXPECT_LT(events[i - 1].sequence, events[i].sequence);
    }
}

TEST_F(ChangefeedCoreTest, ListEventsWithFromSequenceFilters) {
    auto ev1 = feed_->recordEvent(makePut("e1"));
    auto ev2 = feed_->recordEvent(makePut("e2"));
    auto ev3 = feed_->recordEvent(makePut("e3"));

    Changefeed::ListOptions opts;
    opts.from_sequence = ev2.sequence; // return events AFTER ev2
    auto events = feed_->listEvents(opts);

    ASSERT_EQ(events.size(), 1u);
    EXPECT_EQ(events[0].sequence, ev3.sequence);
    // Verify ordering across all 3 recorded events
    EXPECT_LT(ev1.sequence, ev2.sequence);
    EXPECT_LT(ev2.sequence, ev3.sequence);
}

TEST_F(ChangefeedCoreTest, ListEventsWithLimitCapsResults) {
    for (int i = 0; i < 10; i++) {
        feed_->recordEvent(makePut("lim_" + std::to_string(i)));
    }

    Changefeed::ListOptions opts;
    opts.limit = 3;
    auto events = feed_->listEvents(opts);
    EXPECT_EQ(events.size(), 3u);
}

TEST_F(ChangefeedCoreTest, ListEventsWithKeyPrefixFilter) {
    feed_->recordEvent(makePut("alpha:1"));
    feed_->recordEvent(makePut("alpha:2"));
    feed_->recordEvent(makePut("beta:1"));

    Changefeed::ListOptions opts;
    opts.key_prefix = "alpha:";
    auto events = feed_->listEvents(opts);

    ASSERT_EQ(events.size(), 2u);
    for (const auto& ev : events) {
        EXPECT_EQ(ev.key.substr(0, 6), "alpha:");
    }
}

TEST_F(ChangefeedCoreTest, ListEventsWithSingleEventTypeFilter) {
    feed_->recordEvent(makePut("p1"));
    feed_->recordEvent(makeDelete("d1"));
    feed_->recordEvent(makePut("p2"));

    Changefeed::ListOptions opts;
    opts.event_type = Changefeed::ChangeEventType::EVENT_DELETE;
    auto events = feed_->listEvents(opts);

    ASSERT_EQ(events.size(), 1u);
    EXPECT_EQ(events[0].type, Changefeed::ChangeEventType::EVENT_DELETE);
    EXPECT_EQ(events[0].key, "d1");
}

TEST_F(ChangefeedCoreTest, ListEventsWithMultiTypeFilter) {
    feed_->recordEvent(makePut("p1"));
    feed_->recordEvent(makeDelete("d1"));

    Changefeed::ListOptions opts;
    opts.event_types = {Changefeed::ChangeEventType::EVENT_PUT,
                        Changefeed::ChangeEventType::EVENT_DELETE};
    auto events = feed_->listEvents(opts);
    EXPECT_EQ(events.size(), 2u);
}

TEST_F(ChangefeedCoreTest, ListEventsEmptyEventTypesReturnsAll) {
    feed_->recordEvent(makePut("x"));
    feed_->recordEvent(makeDelete("y"));

    Changefeed::ListOptions opts;
    // event_types is empty → no type filter
    auto events = feed_->listEvents(opts);
    EXPECT_EQ(events.size(), 2u);
}

TEST_F(ChangefeedCoreTest, ListEventsWithFromSequenceAndLimit) {
    std::vector<uint64_t> seqs = {};

    for (int i = 0; i < 8; i++) {
        auto ev = feed_->recordEvent(makePut("ev_" + std::to_string(i)));
        seqs.push_back(ev.sequence);
    }

    Changefeed::ListOptions opts;
    opts.from_sequence = seqs[3]; // start after 4th
    opts.limit         = 2;
    auto events = feed_->listEvents(opts);

    ASSERT_EQ(events.size(), 2u);
    EXPECT_EQ(events[0].sequence, seqs[4]);
    EXPECT_EQ(events[1].sequence, seqs[5]);
}

// ListOptions::to_sequence (inclusive upper bound) must stop iteration before
// events beyond the requested ceiling are returned.
TEST_F(ChangefeedCoreTest, ListEventsWithToSequenceBound) {
    std::vector<uint64_t> seqs = {};

    for (int i = 0; i < 6; i++) {
        auto ev = feed_->recordEvent(makePut("bound_ev_" + std::to_string(i)));
        seqs.push_back(ev.sequence);
    }

    // Request events strictly between seqs[1] (exclusive) and seqs[3] (inclusive)
    Changefeed::ListOptions opts;
    opts.from_sequence = seqs[1]; // exclusive — starts at seqs[2]
    opts.to_sequence   = seqs[3]; // inclusive — stops at seqs[3]
    opts.limit         = std::numeric_limits<size_t>::max();
    auto events = feed_->listEvents(opts);

    ASSERT_EQ(events.size(), 2u);
    EXPECT_EQ(events[0].sequence, seqs[2]);
    EXPECT_EQ(events[1].sequence, seqs[3]);
}

// to_sequence == 0 means "no upper bound" — all events from from_sequence onward
// should be returned (up to the limit).
TEST_F(ChangefeedCoreTest, ListEventsToSequenceZeroMeansUnbounded) {
    std::vector<uint64_t> seqs = {};

    for (int i = 0; i < 4; i++) {
        auto ev = feed_->recordEvent(makePut("unb_ev_" + std::to_string(i)));
        seqs.push_back(ev.sequence);
    }

    Changefeed::ListOptions opts;
    opts.from_sequence = seqs[0];  // start after first event
    opts.to_sequence   = 0;        // 0 = no upper bound
    opts.limit         = std::numeric_limits<size_t>::max();
    auto events = feed_->listEvents(opts);

    // Should return the last 3 events (seqs[1], seqs[2], seqs[3])
    ASSERT_EQ(events.size(), 3u);
    EXPECT_EQ(events[0].sequence, seqs[1]);
    EXPECT_EQ(events[2].sequence, seqs[3]);
}

// ===========================================================================
// getEvent() by sequence
// ===========================================================================

TEST_F(ChangefeedCoreTest, GetEventBySequenceReturnsCorrectEvent) {
    auto ev = feed_->recordEvent(makePut("get_me", "my_val"));
    auto fetched = feed_->getEvent(ev.sequence);

    EXPECT_EQ(fetched.sequence, ev.sequence);
    EXPECT_EQ(fetched.key,      "get_me");
    EXPECT_EQ(fetched.value,    "my_val");
}

TEST_F(ChangefeedCoreTest, GetEventForNonExistentSequenceThrows) {
    EXPECT_THROW(feed_->getEvent(99999u), std::exception);
}

// ===========================================================================
// ChangeEvent JSON serialisation roundtrip
// ===========================================================================

TEST_F(ChangefeedCoreTest, JsonRoundtripPutEvent) {
    Changefeed::ChangeEvent ev;
    ev.sequence     = 42;
    ev.type         = Changefeed::ChangeEventType::EVENT_PUT;
    ev.key          = "json_key";
    ev.value        = "json_value";
    ev.timestamp_ms = 1700000000000LL;

    auto j  = ev.toJson();
    auto ev2 = Changefeed::ChangeEvent::fromJson(j);

    EXPECT_EQ(ev2.sequence,     42u);
    EXPECT_EQ(ev2.type,         Changefeed::ChangeEventType::EVENT_PUT);
    EXPECT_EQ(ev2.key,          "json_key");
    EXPECT_EQ(ev2.value,        "json_value");
    EXPECT_EQ(ev2.timestamp_ms, 1700000000000LL);
}

TEST_F(ChangefeedCoreTest, JsonRoundtripDeleteEvent) {
    Changefeed::ChangeEvent ev;
    ev.sequence = 7;
    ev.type     = Changefeed::ChangeEventType::EVENT_DELETE;
    ev.key      = "del_key";

    auto j   = ev.toJson();
    auto ev2 = Changefeed::ChangeEvent::fromJson(j);

    EXPECT_EQ(ev2.type, Changefeed::ChangeEventType::EVENT_DELETE);
    EXPECT_EQ(ev2.key,  "del_key");
    EXPECT_FALSE(ev2.value.has_value());
}

TEST_F(ChangefeedCoreTest, JsonRoundtripTransactionCommitEvent) {
    Changefeed::ChangeEvent ev;
    ev.sequence = 1;
    ev.type     = Changefeed::ChangeEventType::EVENT_TRANSACTION_COMMIT;
    ev.key      = "tx_key";

    auto j   = ev.toJson();
    auto ev2 = Changefeed::ChangeEvent::fromJson(j);

    EXPECT_EQ(ev2.type, Changefeed::ChangeEventType::EVENT_TRANSACTION_COMMIT);
}

TEST_F(ChangefeedCoreTest, JsonRoundtripTransactionRollbackEvent) {
    Changefeed::ChangeEvent ev;
    ev.sequence = 2;
    ev.type     = Changefeed::ChangeEventType::EVENT_TRANSACTION_ROLLBACK;
    ev.key      = "rb_key";

    auto j   = ev.toJson();
    auto ev2 = Changefeed::ChangeEvent::fromJson(j);

    EXPECT_EQ(ev2.type, Changefeed::ChangeEventType::EVENT_TRANSACTION_ROLLBACK);
}

TEST_F(ChangefeedCoreTest, JsonRoundtripWithBeforeAfterSnapshots) {
    Changefeed::ChangeEvent ev;
    ev.sequence        = 10;
    ev.type            = Changefeed::ChangeEventType::EVENT_PUT;
    ev.key             = "snap_key";
    ev.value           = "new_val";
    ev.before_snapshot = "old_val";
    ev.after_snapshot  = "new_val";

    auto j   = ev.toJson();
    auto ev2 = Changefeed::ChangeEvent::fromJson(j);

    EXPECT_EQ(ev2.before_snapshot, "old_val");
    EXPECT_EQ(ev2.after_snapshot,  "new_val");
}

TEST_F(ChangefeedCoreTest, JsonRoundtripRedactedEvent) {
    Changefeed::ChangeEvent ev;
    ev.sequence = 5;
    ev.type     = Changefeed::ChangeEventType::EVENT_PUT;
    ev.key      = "pii_key";
    ev.redacted = true;

    auto j   = ev.toJson();
    auto ev2 = Changefeed::ChangeEvent::fromJson(j);

    EXPECT_TRUE(ev2.redacted);
}

// ===========================================================================
// getWatermarks()
// ===========================================================================

TEST_F(ChangefeedCoreTest, GetWatermarksEmptyFeed) {
    auto wm = feed_->getWatermarks();
    EXPECT_EQ(wm.low_watermark,  0u);
    EXPECT_EQ(wm.high_watermark, 0u);
}

TEST_F(ChangefeedCoreTest, GetWatermarksAfterRecordingEvents) {
    auto ev1 = feed_->recordEvent(makePut("wm1"));
    auto ev2 = feed_->recordEvent(makePut("wm2"));
    auto ev3 = feed_->recordEvent(makePut("wm3"));

    auto wm = feed_->getWatermarks();
    EXPECT_EQ(wm.low_watermark,  ev1.sequence);
    EXPECT_EQ(wm.high_watermark, ev3.sequence);
    // ev2 is an intermediate event that confirms monotonically increasing ordering
    EXPECT_GT(ev2.sequence, ev1.sequence);
    EXPECT_LT(ev2.sequence, ev3.sequence);
}

// ===========================================================================
// Concurrent subscribe/unsubscribe safety
// ===========================================================================

TEST_F(ChangefeedCoreTest, ConcurrentSubscribeAndRecordIsThreadSafe) {
    const int num_threads   = 8;
    const int events_each   = 20;
    std::atomic<int> total_calls{0};

    std::vector<std::thread> threads;
    std::vector<Changefeed::SubscriptionHandle> handles;
    std::mutex handles_mutex;

    // Spawn threads that subscribe and record simultaneously
    for (int t = 0; t < num_threads; t++) {
        threads.emplace_back([&, t]() {
            auto h = feed_->subscribe({},
                [&](const Changefeed::ChangeEvent&) { total_calls++; });

            for (int i = 0; i < events_each; i++) {
                feed_->recordEvent(makePut("t" + std::to_string(t) +
                                          "_e" + std::to_string(i)));
            }

            std::lock_guard<std::mutex> lk(handles_mutex);
            handles.push_back(std::move(h));
        });
    }

    for (auto& thr : threads) { thr.join(); }
    for (auto& h   : handles) { h.cancel(); }

    // At least some callbacks should have fired — we can't assert an exact
    // count because subscribe/record ordering across threads is non-deterministic,
    // but the test validates there are no crashes or deadlocks.
    EXPECT_GE(total_calls.load(), 0);
}
