/**
 * Test: Metadata Change Listener
 *
 * Tests for IMetadataChangeListener / RecordingMetadataChangeListener:
 *
 * Acceptance criteria:
 *   AC-CL-1  Initially eventCount() is 0
 *   AC-CL-2  onMetadataChanged() records events in FIFO order
 *   AC-CL-3  lastEvent() returns nullopt when no events have been recorded
 *   AC-CL-4  lastEvent() returns the most recent event after dispatch
 *   AC-CL-5  clear() resets eventCount to 0
 *   AC-CL-6  Callback is invoked synchronously after each event
 *   AC-CL-7  setCallback() replaces the previous callback
 *   AC-CL-8  Concurrent onMetadataChanged() calls are thread-safe
 *   AC-CL-9  Polymorphic usage via IMetadataChangeListener*
 *   AC-CL-10 MetadataChangeEvent::toJSON() emits correct fields
 *
 * Copyright (c) 2026 ThemisDB Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>
#include "metadata/imetadata_change_listener.h"

#include <atomic>
#include <thread>
#include <vector>

using namespace themis::metadata;

// Helper: build a minimal change event
static MetadataChangeEvent makeEvent(MetadataChangeType type,
                                     std::string_view   table) {
    MetadataChangeEvent ev;
    ev.change_type = type;
    ev.table_name  = std::string(table);
    ev.timestamp   = std::chrono::system_clock::now();
    return ev;
}

// ─────────────────────────────────────────────────────────────────────────────
// AC-CL-1 — Initial state
// ─────────────────────────────────────────────────────────────────────────────

TEST(RecordingMetadataChangeListenerTest, InitiallyEmpty) {
    RecordingMetadataChangeListener rec;
    EXPECT_EQ(rec.eventCount(), 0u);
    EXPECT_TRUE(rec.events().empty());
}

// ─────────────────────────────────────────────────────────────────────────────
// AC-CL-2 — FIFO ordering
// ─────────────────────────────────────────────────────────────────────────────

TEST(RecordingMetadataChangeListenerTest, EventsRecordedInFifoOrder) {
    RecordingMetadataChangeListener rec;
    rec.onMetadataChanged(makeEvent(MetadataChangeType::TABLE_CREATED,  "orders"));
    rec.onMetadataChanged(makeEvent(MetadataChangeType::TABLE_MODIFIED, "orders"));
    rec.onMetadataChanged(makeEvent(MetadataChangeType::TABLE_DROPPED,  "orders"));

    auto evs = rec.events();
    ASSERT_EQ(evs.size(), 3u);
    EXPECT_EQ(evs[0].change_type, MetadataChangeType::TABLE_CREATED);
    EXPECT_EQ(evs[1].change_type, MetadataChangeType::TABLE_MODIFIED);
    EXPECT_EQ(evs[2].change_type, MetadataChangeType::TABLE_DROPPED);
}

TEST(RecordingMetadataChangeListenerTest, TableNamesPreserved) {
    RecordingMetadataChangeListener rec;
    rec.onMetadataChanged(makeEvent(MetadataChangeType::TABLE_CREATED, "alpha"));
    rec.onMetadataChanged(makeEvent(MetadataChangeType::TABLE_CREATED, "beta"));

    auto evs = rec.events();
    EXPECT_EQ(evs[0].table_name, "alpha");
    EXPECT_EQ(evs[1].table_name, "beta");
}

// ─────────────────────────────────────────────────────────────────────────────
// AC-CL-3/4 — lastEvent()
// ─────────────────────────────────────────────────────────────────────────────

TEST(RecordingMetadataChangeListenerTest, LastEventNulloptWhenEmpty) {
    RecordingMetadataChangeListener rec;
    EXPECT_FALSE(rec.lastEvent().has_value());
}

TEST(RecordingMetadataChangeListenerTest, LastEventReturnsNewestEvent) {
    RecordingMetadataChangeListener rec;
    rec.onMetadataChanged(makeEvent(MetadataChangeType::TABLE_CREATED,  "t1"));
    rec.onMetadataChanged(makeEvent(MetadataChangeType::STATISTICS_UPDATED, "t2"));

    auto last = rec.lastEvent();
    ASSERT_TRUE(last.has_value());
    EXPECT_EQ(last->change_type, MetadataChangeType::STATISTICS_UPDATED);
    EXPECT_EQ(last->table_name,  "t2");
}

// ─────────────────────────────────────────────────────────────────────────────
// AC-CL-5 — clear()
// ─────────────────────────────────────────────────────────────────────────────

TEST(RecordingMetadataChangeListenerTest, ClearResetsEventCount) {
    RecordingMetadataChangeListener rec;
    rec.onMetadataChanged(makeEvent(MetadataChangeType::TABLE_CREATED, "t"));
    rec.onMetadataChanged(makeEvent(MetadataChangeType::TABLE_MODIFIED, "t"));
    EXPECT_EQ(rec.eventCount(), 2u);

    rec.clear();
    EXPECT_EQ(rec.eventCount(), 0u);
    EXPECT_FALSE(rec.lastEvent().has_value());
}

// ─────────────────────────────────────────────────────────────────────────────
// AC-CL-6 — Callback invoked after each event
// ─────────────────────────────────────────────────────────────────────────────

TEST(RecordingMetadataChangeListenerTest, CallbackInvokedAfterEachEvent) {
    std::atomic<int>      count{0};
    MetadataChangeType    last_type{};

    RecordingMetadataChangeListener rec([&](const MetadataChangeEvent& ev) {
        last_type = ev.change_type;
        ++count;
    });

    rec.onMetadataChanged(makeEvent(MetadataChangeType::TABLE_CREATED,  "t"));
    EXPECT_EQ(count.load(), 1);
    EXPECT_EQ(last_type, MetadataChangeType::TABLE_CREATED);

    rec.onMetadataChanged(makeEvent(MetadataChangeType::CONSTRAINT_ADDED, "t"));
    EXPECT_EQ(count.load(), 2);
    EXPECT_EQ(last_type, MetadataChangeType::CONSTRAINT_ADDED);
}

TEST(RecordingMetadataChangeListenerTest, NoCallbackNoError) {
    RecordingMetadataChangeListener rec;  // no callback
    EXPECT_NO_THROW(rec.onMetadataChanged(makeEvent(MetadataChangeType::TABLE_CREATED, "t")));
    EXPECT_EQ(rec.eventCount(), 1u);
}

// ─────────────────────────────────────────────────────────────────────────────
// AC-CL-7 — setCallback() replaces previous callback
// ─────────────────────────────────────────────────────────────────────────────

TEST(RecordingMetadataChangeListenerTest, SetCallbackReplacesPrevious) {
    std::atomic<int> first_count{0};
    std::atomic<int> second_count{0};

    RecordingMetadataChangeListener rec(
        [&](const MetadataChangeEvent&) { ++first_count; });

    rec.onMetadataChanged(makeEvent(MetadataChangeType::TABLE_CREATED, "t"));
    EXPECT_EQ(first_count.load(), 1);

    rec.setCallback([&](const MetadataChangeEvent&) { ++second_count; });
    rec.onMetadataChanged(makeEvent(MetadataChangeType::TABLE_MODIFIED, "t"));
    EXPECT_EQ(first_count.load(), 1);   // old callback not called again
    EXPECT_EQ(second_count.load(), 1);  // new callback called
}

TEST(RecordingMetadataChangeListenerTest, SetCallbackToNullDisablesCallback) {
    std::atomic<int> count{0};
    RecordingMetadataChangeListener rec(
        [&](const MetadataChangeEvent&) { ++count; });

    rec.setCallback(nullptr);
    rec.onMetadataChanged(makeEvent(MetadataChangeType::TABLE_DROPPED, "t"));
    EXPECT_EQ(count.load(), 0);
    EXPECT_EQ(rec.eventCount(), 1u);  // event still recorded
}

// ─────────────────────────────────────────────────────────────────────────────
// AC-CL-8 — Thread-safety
// ─────────────────────────────────────────────────────────────────────────────

TEST(RecordingMetadataChangeListenerTest, ConcurrentDispatchIsThreadSafe) {
    RecordingMetadataChangeListener rec;
    constexpr int kEventsPerThread = 100;
    constexpr int kThreads         = 4;

    std::vector<std::thread> threads;
    threads.reserve(kThreads);

    for (int t = 0; t < kThreads; ++t) {
        threads.emplace_back([&] {
            for (int i = 0; i < kEventsPerThread; ++i) {
                rec.onMetadataChanged(
                    makeEvent(MetadataChangeType::TABLE_MODIFIED, "shared_table"));
            }
        });
    }

    for (auto& th : threads) th.join();

    EXPECT_EQ(rec.eventCount(), static_cast<std::size_t>(kThreads * kEventsPerThread));
}

// ─────────────────────────────────────────────────────────────────────────────
// AC-CL-9 — Polymorphic usage
// ─────────────────────────────────────────────────────────────────────────────

TEST(MetadataChangeListenerPolymorphismTest, DispatchViaInterface) {
    std::unique_ptr<IMetadataChangeListener> listener =
        std::make_unique<RecordingMetadataChangeListener>();

    listener->onMetadataChanged(makeEvent(MetadataChangeType::TABLE_CREATED, "orders"));

    auto* rec = dynamic_cast<RecordingMetadataChangeListener*>(listener.get());
    ASSERT_NE(rec, nullptr);
    EXPECT_EQ(rec->eventCount(), 1u);
    EXPECT_EQ(rec->lastEvent()->table_name, "orders");
}

// ─────────────────────────────────────────────────────────────────────────────
// AC-CL-10 — MetadataChangeEvent::toJSON()
// ─────────────────────────────────────────────────────────────────────────────

TEST(MetadataChangeEventTest, ToJSONIncludesRequiredFields) {
    MetadataChangeEvent ev;
    ev.change_type = MetadataChangeType::TABLE_CREATED;
    ev.table_name  = "orders";
    ev.actor       = "dba";
    ev.detail      = "initial creation";
    ev.timestamp   = std::chrono::system_clock::now();

    auto j = ev.toJSON();
    EXPECT_TRUE(j.contains("change_type"));
    EXPECT_TRUE(j.contains("table_name"));
    EXPECT_EQ(j["table_name"].get<std::string>(), "orders");
    EXPECT_TRUE(j.contains("actor"));
    EXPECT_EQ(j["actor"].get<std::string>(), "dba");
    EXPECT_TRUE(j.contains("detail"));
    EXPECT_TRUE(j.contains("timestamp_ms"));
}

TEST(MetadataChangeEventTest, ToJSONOmitsAbsentOptionalFields) {
    MetadataChangeEvent ev;
    ev.change_type = MetadataChangeType::STATISTICS_UPDATED;
    ev.table_name  = "users";
    ev.timestamp   = std::chrono::system_clock::now();
    // actor and detail intentionally left empty

    auto j = ev.toJSON();
    EXPECT_FALSE(j.contains("actor"));
    EXPECT_FALSE(j.contains("detail"));
}
