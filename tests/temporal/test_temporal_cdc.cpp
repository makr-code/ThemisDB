/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            test_temporal_cdc.cpp                              ║
  Version:         0.0.3                                              ║
  Last Modified:   2026-04-13 04:34:26                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     253                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • ab6254146d  2026-03-20  docs(temporal): document Phase 4 components and enrich tests ║
    • f8f5de7b2b  2026-03-20  feat(temporal): add Phase 4 tests, update CMake/CI/ROADMAP ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/**
 * Tests for TemporalCDC
 *
 * Copyright (c) 2025 VCC-URN Project
 * SPDX-License-Identifier: Apache-2.0
 */

#include <atomic>
#include <gtest/gtest.h>
#include "temporal/temporal_cdc.h"

using namespace themisdb::temporal;

class TemporalCDCTest : public ::testing::Test {
protected:
    TemporalCDC cdc;

    ChangeEvent makeEvent(const std::string& table, ChangeType type,
                          const std::string& entity_id,
                          Timestamp ts = 1000) {
        ChangeEvent ev;
        ev.type             = type;
        ev.table_name       = table;
        ev.entity_id        = entity_id;
        ev.after_value      = {{"id", entity_id}};
        ev.transaction_time = ts;
        ev.valid_from       = ts;
        ev.valid_to         = kMaxTimestamp;
        return ev;
    }
};

// ── subscribe / unsubscribe ───────────────────────────────────────────────────

TEST_F(TemporalCDCTest, Subscribe_ReturnsNonEmptySubId) {
    auto id = cdc.subscribeToChanges("orders", [](const ChangeEvent&) {});
    EXPECT_FALSE(id.empty());
}

TEST_F(TemporalCDCTest, SubscriptionCount_AfterSubscribe_IsOne) {
    cdc.subscribeToChanges("orders", [](const ChangeEvent&) {});
    EXPECT_EQ(cdc.subscriptionCount(), 1u);
}

TEST_F(TemporalCDCTest, Unsubscribe_ValidId_ReturnsTrue) {
    auto id = cdc.subscribeToChanges("orders", [](const ChangeEvent&) {});
    EXPECT_TRUE(cdc.unsubscribe(id));
}

TEST_F(TemporalCDCTest, Unsubscribe_InvalidId_ReturnsFalse) {
    EXPECT_FALSE(cdc.unsubscribe("nonexistent-sub-id"));
}

TEST_F(TemporalCDCTest, SubscriptionCount_AfterUnsubscribe_IsZero) {
    auto id = cdc.subscribeToChanges("orders", [](const ChangeEvent&) {});
    cdc.unsubscribe(id);
    EXPECT_EQ(cdc.subscriptionCount(), 0u);
}

// ── publishEvent / delivery ───────────────────────────────────────────────────

TEST_F(TemporalCDCTest, PublishEvent_DeliveresToSubscriber) {
    std::vector<ChangeEvent> received;
    cdc.subscribeToChanges("employees",
        [&received](const ChangeEvent& ev) { received.push_back(ev); });

    cdc.publishEvent(makeEvent("employees", ChangeType::INSERT, "emp1"));
    ASSERT_EQ(received.size(), 1u);
    EXPECT_EQ(received[0].entity_id, "emp1");
}

TEST_F(TemporalCDCTest, PublishEvent_DoesNotDeliverToWrongTable) {
    std::vector<ChangeEvent> received;
    cdc.subscribeToChanges("orders",
        [&received](const ChangeEvent& ev) { received.push_back(ev); });

    cdc.publishEvent(makeEvent("employees", ChangeType::INSERT, "emp1"));
    EXPECT_TRUE(received.empty());
}

TEST_F(TemporalCDCTest, PublishEvent_WildcardSubscriber_ReceivesAll) {
    std::vector<ChangeEvent> received;
    // empty table_name = wildcard (all tables)
    cdc.subscribeToChanges("",
        [&received](const ChangeEvent& ev) { received.push_back(ev); });

    cdc.publishEvent(makeEvent("employees", ChangeType::INSERT, "emp1"));
    cdc.publishEvent(makeEvent("orders",    ChangeType::INSERT, "ord1"));
    EXPECT_EQ(received.size(), 2u);
}

// ── replayChanges ─────────────────────────────────────────────────────────────

TEST_F(TemporalCDCTest, ReplayChanges_ReturnsMatchingEvents) {
    cdc.publishEvent(makeEvent("employees", ChangeType::INSERT, "emp1", 500));
    cdc.publishEvent(makeEvent("employees", ChangeType::UPDATE, "emp1", 1000));
    cdc.publishEvent(makeEvent("orders",    ChangeType::INSERT, "ord1", 1200));

    auto events = cdc.replayChanges("employees", {400, 1500});
    EXPECT_EQ(events.size(), 2u);
}

TEST_F(TemporalCDCTest, ReplayChanges_EmptyRange_ReturnsEmpty) {
    cdc.publishEvent(makeEvent("employees", ChangeType::INSERT, "emp1", 1000));

    auto events = cdc.replayChanges("employees", {2000, 3000});
    EXPECT_TRUE(events.empty());
}

TEST_F(TemporalCDCTest, ReplayChanges_AllTables_ReturnsAll) {
    cdc.publishEvent(makeEvent("employees", ChangeType::INSERT, "emp1", 1000));
    cdc.publishEvent(makeEvent("orders",    ChangeType::INSERT, "ord1", 1100));

    // empty table_name → all tables
    auto events = cdc.replayChanges("", {0, kMaxTimestamp});
    EXPECT_EQ(events.size(), 2u);
}

// ── log metrics ───────────────────────────────────────────────────────────────

TEST_F(TemporalCDCTest, LogSize_AfterPublish_Increases) {
    EXPECT_EQ(cdc.logSize(), 0u);
    cdc.publishEvent(makeEvent("orders", ChangeType::INSERT, "ord1"));
    EXPECT_EQ(cdc.logSize(), 1u);
}

TEST_F(TemporalCDCTest, TotalPublished_Increments) {
    EXPECT_EQ(cdc.totalPublished(), 0u);
    cdc.publishEvent(makeEvent("orders", ChangeType::INSERT, "ord1"));
    cdc.publishEvent(makeEvent("orders", ChangeType::INSERT, "ord2"));
    EXPECT_EQ(cdc.totalPublished(), 2u);
}

TEST_F(TemporalCDCTest, ClearLog_EmptiesLog) {
    cdc.publishEvent(makeEvent("orders", ChangeType::INSERT, "ord1"));
    cdc.publishEvent(makeEvent("orders", ChangeType::INSERT, "ord2"));
    EXPECT_EQ(cdc.logSize(), 2u);
    cdc.clearLog();
    EXPECT_EQ(cdc.logSize(), 0u);
}

// ── ChangeEvent serialization round-trip ─────────────────────────────────────

TEST_F(TemporalCDCTest, ChangeEvent_ToJson_FromJson_RoundTrip) {
    ChangeEvent original;
    original.type             = ChangeType::UPDATE;
    original.table_name       = "employees";
    original.entity_id        = "emp42";
    original.before_value     = {{"name", "Alice"}, {"age", 30}};
    original.after_value      = {{"name", "Alice"}, {"age", 31}};
    original.transaction_time = 123456789;
    original.valid_from       = 100000;
    original.valid_to         = kMaxTimestamp;
    original.user_id          = "admin";

    auto j        = original.toJson();
    auto restored = ChangeEvent::fromJson(j);

    EXPECT_EQ(restored.table_name,       original.table_name);
    EXPECT_EQ(restored.entity_id,        original.entity_id);
    EXPECT_EQ(restored.transaction_time, original.transaction_time);
    EXPECT_EQ(restored.before_value,     original.before_value);
    EXPECT_EQ(restored.after_value,      original.after_value);
    EXPECT_EQ(restored.user_id,          original.user_id);
}

// ── ring-buffer overflow ──────────────────────────────────────────────────────

TEST_F(TemporalCDCTest, RingBuffer_Overflow_OldestEvicted) {
    TemporalCDC small_cdc{4};

    for (int i = 0; i < 9; ++i) {
        small_cdc.publishEvent(
            makeEvent("tbl", ChangeType::INSERT,
                      "entity" + std::to_string(i),
                      static_cast<Timestamp>(1000 + i)));
    }

    EXPECT_EQ(small_cdc.logSize(), 4u);
    EXPECT_EQ(small_cdc.totalPublished(), 9u);
}

// ── Additional edge-case tests ────────────────────────────────────────────────

TEST_F(TemporalCDCTest, SubscriptionCount_MultipleSubscriptions_IsCorrect) {
    cdc.subscribeToChanges("tbl", [](const ChangeEvent&) {});
    cdc.subscribeToChanges("tbl", [](const ChangeEvent&) {});
    cdc.subscribeToChanges("tbl", [](const ChangeEvent&) {});
    EXPECT_EQ(cdc.subscriptionCount(), 3u);
}

TEST_F(TemporalCDCTest, Unsubscribe_ReducesCount) {
    auto id1 = cdc.subscribeToChanges("tbl", [](const ChangeEvent&) {});
    cdc.subscribeToChanges("tbl", [](const ChangeEvent&) {});
    cdc.subscribeToChanges("tbl", [](const ChangeEvent&) {});
    ASSERT_EQ(cdc.subscriptionCount(), 3u);
    cdc.unsubscribe(id1);
    EXPECT_EQ(cdc.subscriptionCount(), 2u);
}

TEST_F(TemporalCDCTest, PublishEvent_MultipleSubscribers_AllReceive) {
    std::atomic<int> counter{0};
    cdc.subscribeToChanges("orders", [&](const ChangeEvent&) { ++counter; });
    cdc.subscribeToChanges("orders", [&](const ChangeEvent&) { ++counter; });
    cdc.subscribeToChanges("orders", [&](const ChangeEvent&) { ++counter; });

    cdc.publishEvent(makeEvent("orders", ChangeType::INSERT, "ord1", 1000));
    EXPECT_EQ(counter.load(), 3);
}

TEST_F(TemporalCDCTest, ReplayChanges_TimeRangeFilter_ExcludesOutOfRange) {
    cdc.publishEvent(makeEvent("employees", ChangeType::INSERT, "emp1", 100));
    auto events = cdc.replayChanges("employees", {200, 300});
    EXPECT_TRUE(events.empty());
}

TEST_F(TemporalCDCTest, ClearLog_SubscriptionsIntact) {
    int delivered = 0;
    cdc.subscribeToChanges("tbl", [&](const ChangeEvent&) { ++delivered; });
    cdc.publishEvent(makeEvent("tbl", ChangeType::INSERT, "e1", 1000));
    ASSERT_EQ(cdc.logSize(), 1u);

    cdc.clearLog();
    EXPECT_EQ(cdc.logSize(), 0u);

    // Subscription must still be active
    cdc.publishEvent(makeEvent("tbl", ChangeType::INSERT, "e2", 2000));
    EXPECT_EQ(delivered, 2);
    EXPECT_EQ(cdc.logSize(), 1u);
}
