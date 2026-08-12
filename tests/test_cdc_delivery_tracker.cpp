// Test: CDC At-Least-Once Delivery Tracker
// Tests for DeliveryTracker: trackDelivery, acknowledge, acknowledgeUpTo,
// getPendingRedelivery, redelivery callback, expiry, and stats.

#include <gtest/gtest.h>
#include "cdc/delivery_tracker.h"

#include <atomic>
#include <chrono>
#include <string>
#include <thread>
#include <vector>

using namespace themis;
using namespace themis::cdc;

namespace {

// Helper: make a ChangeEvent with the given sequence
Changefeed::ChangeEvent makeEvent(uint64_t seq, const std::string& key_prefix = "key") {
    Changefeed::ChangeEvent ev;
    ev.sequence = seq;
    ev.type = Changefeed::ChangeEventType::EVENT_PUT;
    ev.key = key_prefix + ":" + std::to_string(seq);
    ev.value = "val";
    ev.timestamp_ms = static_cast<int64_t>(seq) * 1000;
    return ev;
}

std::vector<Changefeed::ChangeEvent> makeEventRange(uint64_t from, uint64_t to) {
    std::vector<Changefeed::ChangeEvent> evs;
    for (uint64_t s = from; s <= to; ++s) {
        evs.push_back(makeEvent(s));
    }
    return evs;
}

} // anonymous namespace

// ============================================================================
// Basic trackDelivery / acknowledge tests
// ============================================================================

TEST(DeliveryTrackerTest, TrackAndAcknowledge) {
    DeliveryTracker tracker;

    auto events = makeEventRange(1, 3);
    EXPECT_TRUE(tracker.trackDelivery("c1", events));

    auto stats = tracker.getStats("c1");
    ASSERT_TRUE(stats.has_value());
    EXPECT_EQ(stats->pending_count, 3u);
    EXPECT_EQ(stats->total_delivered, 3u);
    EXPECT_EQ(stats->total_acknowledged, 0u);

    EXPECT_TRUE(tracker.acknowledge("c1", 2));
    EXPECT_FALSE(tracker.acknowledge("c1", 2)); // already acked

    stats = tracker.getStats("c1");
    ASSERT_TRUE(stats.has_value());
    EXPECT_EQ(stats->pending_count, 2u);
    EXPECT_EQ(stats->total_acknowledged, 1u);
}

TEST(DeliveryTrackerTest, AcknowledgeUnknownConsumer) {
    DeliveryTracker tracker;
    EXPECT_FALSE(tracker.acknowledge("unknown", 42));
}

TEST(DeliveryTrackerTest, AcknowledgeUnknownSequence) {
    DeliveryTracker tracker;
    tracker.trackDelivery("c1", makeEventRange(1, 2));
    EXPECT_FALSE(tracker.acknowledge("c1", 99)); // seq 99 not tracked
}

TEST(DeliveryTrackerTest, AcknowledgeUpTo) {
    DeliveryTracker tracker;
    tracker.trackDelivery("c1", makeEventRange(1, 5));

    size_t removed = tracker.acknowledgeUpTo("c1", 3);
    EXPECT_EQ(removed, 3u); // seq 1, 2, 3

    auto stats = tracker.getStats("c1");
    ASSERT_TRUE(stats.has_value());
    EXPECT_EQ(stats->pending_count, 2u); // seq 4, 5 remain
    EXPECT_EQ(stats->total_acknowledged, 3u);
}

TEST(DeliveryTrackerTest, AcknowledgeUpToCoversAll) {
    DeliveryTracker tracker;
    tracker.trackDelivery("c1", makeEventRange(10, 15));
    size_t removed = tracker.acknowledgeUpTo("c1", 100);
    EXPECT_EQ(removed, 6u);

    auto stats = tracker.getStats("c1");
    ASSERT_TRUE(stats.has_value());
    EXPECT_EQ(stats->pending_count, 0u);
}

TEST(DeliveryTrackerTest, AcknowledgeUpToUnknownConsumer) {
    DeliveryTracker tracker;
    EXPECT_EQ(tracker.acknowledgeUpTo("ghost", 10), 0u);
}

// ============================================================================
// Consumer management
// ============================================================================

TEST(DeliveryTrackerTest, RemoveConsumer) {
    DeliveryTracker tracker;
    tracker.trackDelivery("c1", makeEventRange(1, 3));
    EXPECT_EQ(tracker.consumerCount(), 1u);

    tracker.removeConsumer("c1");
    EXPECT_EQ(tracker.consumerCount(), 0u);
    EXPECT_FALSE(tracker.getStats("c1").has_value());
}

TEST(DeliveryTrackerTest, RemoveUnknownConsumerIsNoop) {
    DeliveryTracker tracker;
    EXPECT_NO_THROW(tracker.removeConsumer("nobody"));
}

TEST(DeliveryTrackerTest, MultipleConsumers) {
    DeliveryTracker tracker;
    tracker.trackDelivery("c1", makeEventRange(1, 2));
    tracker.trackDelivery("c2", makeEventRange(3, 5));

    EXPECT_EQ(tracker.consumerCount(), 2u);

    auto all = tracker.getAllStats();
    EXPECT_EQ(all.size(), 2u);
}

// ============================================================================
// Pending limit
// ============================================================================

TEST(DeliveryTrackerTest, PendingLimitRejection) {
    DeliveryTrackerConfig cfg;
    cfg.max_pending_per_consumer = 3;
    DeliveryTracker tracker(cfg);

    EXPECT_TRUE(tracker.trackDelivery("c1", makeEventRange(1, 3)));
    // Adding 1 more would exceed the limit (3 + 1 > 3)
    EXPECT_FALSE(tracker.trackDelivery("c1", {makeEvent(4)}));

    // After acking one, there is room again
    tracker.acknowledge("c1", 1);
    EXPECT_TRUE(tracker.trackDelivery("c1", {makeEvent(4)}));
}

TEST(DeliveryTrackerTest, EmptyDeliveryAlwaysSucceeds) {
    DeliveryTrackerConfig cfg;
    cfg.max_pending_per_consumer = 1;
    DeliveryTracker tracker(cfg);

    EXPECT_TRUE(tracker.trackDelivery("c1", {}));
    EXPECT_EQ(tracker.consumerCount(), 0u); // consumer not created for empty delivery
}

// ============================================================================
// getPendingRedelivery — manual poll (no background thread)
// ============================================================================

TEST(DeliveryTrackerTest, NoPendingRedeliveryBeforeTimeout) {
    DeliveryTrackerConfig cfg;
    cfg.ack_timeout = std::chrono::milliseconds{60000}; // very long timeout
    DeliveryTracker tracker(cfg);

    tracker.trackDelivery("c1", makeEventRange(1, 3));

    // Should return nothing because timeout hasn't elapsed
    auto pending = tracker.getPendingRedelivery("c1");
    EXPECT_TRUE(pending.empty());
}

TEST(DeliveryTrackerTest, PendingRedeliveryAfterTimeout) {
    DeliveryTrackerConfig cfg;
    cfg.ack_timeout = std::chrono::milliseconds{0}; // immediate timeout for test
    DeliveryTracker tracker(cfg);

    tracker.trackDelivery("c1", makeEventRange(1, 3));

    // With timeout=0s every event is immediately eligible
    auto pending = tracker.getPendingRedelivery("c1");
    EXPECT_EQ(pending.size(), 3u);

    auto stats = tracker.getStats("c1");
    ASSERT_TRUE(stats.has_value());
    EXPECT_EQ(stats->total_redeliveries, 3u);
    EXPECT_EQ(stats->pending_count, 3u); // still pending (not acked)
}

TEST(DeliveryTrackerTest, AckedEventsNotRedelivered) {
    DeliveryTrackerConfig cfg;
    cfg.ack_timeout = std::chrono::milliseconds{0}; // immediate timeout for test
    DeliveryTracker tracker(cfg);

    tracker.trackDelivery("c1", makeEventRange(1, 3));
    tracker.acknowledge("c1", 2); // ack seq 2

    auto pending = tracker.getPendingRedelivery("c1");
    EXPECT_EQ(pending.size(), 2u);
    for (const auto& ev : pending) {
        EXPECT_NE(ev.sequence, 2u); // seq 2 must not appear
    }
}

TEST(DeliveryTrackerTest, ExpiredEventsRemovedAfterMaxAttempts) {
    DeliveryTrackerConfig cfg;
    cfg.ack_timeout = std::chrono::milliseconds{0}; // immediate timeout for test
    cfg.max_redelivery_attempts = 2;
    DeliveryTracker tracker(cfg);

    tracker.trackDelivery("c1", {makeEvent(1)});

    // Attempt 1: returns event for redelivery
    auto r1 = tracker.getPendingRedelivery("c1");
    EXPECT_EQ(r1.size(), 1u);

    // Attempt 2 (attempt count == max): event expires and is removed
    auto r2 = tracker.getPendingRedelivery("c1");
    EXPECT_TRUE(r2.empty());

    auto stats = tracker.getStats("c1");
    ASSERT_TRUE(stats.has_value());
    EXPECT_EQ(stats->total_expired, 1u);
    EXPECT_EQ(stats->pending_count, 0u);
}

TEST(DeliveryTrackerTest, PendingRedeliveryUnknownConsumer) {
    DeliveryTracker tracker;
    auto result = tracker.getPendingRedelivery("nobody");
    EXPECT_TRUE(result.empty());
}

// ============================================================================
// Redelivery callback via background thread
// ============================================================================

TEST(DeliveryTrackerTest, RedeliveryCallbackInvokedByBackgroundThread) {
    std::atomic<int> callback_count{0};
    std::atomic<uint64_t> last_sequence{0};

    DeliveryTrackerConfig cfg;
    cfg.ack_timeout = std::chrono::milliseconds{0}; // immediate timeout for test
    cfg.recheck_interval = std::chrono::milliseconds{50};

    DeliveryTracker tracker(cfg, [&]([[maybe_unused]] const std::string& cid,
                                     const std::vector<Changefeed::ChangeEvent>& evts) {
        callback_count.fetch_add(static_cast<int>(evts.size()));
        for (const auto& ev : evts) {
            last_sequence.store(ev.sequence);
        }
    });

    tracker.start();
    tracker.trackDelivery("c1", {makeEvent(42)});

    // Wait up to 1 second for the background thread to fire the callback
    for (int i = 0; i < 100 && callback_count.load() == 0; ++i) {
        std::this_thread::sleep_for(std::chrono::milliseconds{10});
    }

    tracker.stop();

    EXPECT_GE(callback_count.load(), 1);
    EXPECT_EQ(last_sequence.load(), 42u);
}

TEST(DeliveryTrackerTest, StartStopIdempotent) {
    DeliveryTracker tracker;
    tracker.start();
    tracker.start(); // second start is a no-op
    tracker.stop();
    tracker.stop();  // second stop is a no-op
}

// ============================================================================
// Stats
// ============================================================================

TEST(DeliveryTrackerTest, StatsAllConsumers) {
    DeliveryTracker tracker;
    tracker.trackDelivery("c1", makeEventRange(1, 2));
    tracker.trackDelivery("c2", makeEventRange(10, 12));
    tracker.acknowledge("c1", 1);

    auto all = tracker.getAllStats();
    ASSERT_EQ(all.size(), 2u);

    // Find c1 stats
    ConsumerDeliveryStats* c1_stats = nullptr;
    for (auto& s : all) {
        if (s.consumer_id == "c1") c1_stats = &s;
    }
    ASSERT_NE(c1_stats, nullptr);
    EXPECT_EQ(c1_stats->total_delivered, 2u);
    EXPECT_EQ(c1_stats->total_acknowledged, 1u);
    EXPECT_EQ(c1_stats->pending_count, 1u);
}

TEST(DeliveryTrackerTest, StatsUnknownConsumer) {
    DeliveryTracker tracker;
    EXPECT_FALSE(tracker.getStats("nobody").has_value());
}

// ============================================================================
// Thread safety: concurrent trackDelivery + acknowledge
// ============================================================================

TEST(DeliveryTrackerTest, ConcurrentDeliveryAndAck) {
    DeliveryTracker tracker;

    constexpr int N = 200;
    std::atomic<int> ack_count{0};

    // Producer thread: deliver events
    std::thread producer([&] {
        for (int i = 1; i <= N; ++i) {
            tracker.trackDelivery("c1", {makeEvent(static_cast<uint64_t>(i))});
        }
    });

    // Consumer thread: acknowledge events
    std::thread consumer([&] {
        for (int i = 1; i <= N; ++i) {
            if (tracker.acknowledge("c1", static_cast<uint64_t>(i))) {
                ack_count++;
            }
        }
    });

    producer.join();
    consumer.join();

    // Some events may have been acked before they were tracked (race),
    // but the total_delivered should equal N once settled.
    auto stats = tracker.getStats("c1");
    ASSERT_TRUE(stats.has_value());
    EXPECT_EQ(stats->total_delivered, static_cast<uint64_t>(N));
}
