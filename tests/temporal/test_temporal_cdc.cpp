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

// ============================================================================
// CDCPersistentLog tests (CDCPL-01 .. CDCPL-08) — v1.8.0
// ============================================================================

#include <filesystem>
#include <cstdlib>
#include <chrono>

namespace {
std::string makeTempDir() {
    namespace fs = std::filesystem;
    const fs::path base = fs::temp_directory_path();
    for (int attempt = 0; attempt < 32; ++attempt) {
        const auto stamp = std::chrono::steady_clock::now().time_since_epoch().count();
        const fs::path candidate =
            base / ("themisdb_wal_" + std::to_string(stamp) + "_" + std::to_string(attempt));
        std::error_code ec = {};
        if (fs::create_directories(candidate, ec)) {
            return candidate.string();
        }
    }
    const fs::path fallback = base / "themisdb_wal_test";
    std::error_code ec = {};
    fs::create_directories(fallback, ec);
    return fallback.string();
}
} // namespace

class CDCPersistentLogTest : public ::testing::Test {
protected:
    std::string tmp_dir_ = {};

    void SetUp() override {
        tmp_dir_ = makeTempDir();
    }
    void TearDown() override {
        std::filesystem::remove_all(tmp_dir_);
    }

    ChangeEvent makeEvent(const std::string& table, const std::string& key) {
        ChangeEvent ev;
        ev.type             = ChangeType::INSERT;
        ev.table_name       = table;
        ev.entity_id        = key;
        ev.after_value      = {{"k", key}};
        ev.transaction_time = 42;
        return ev;
    }
};

TEST_F(CDCPersistentLogTest, CDCPL_01_OpenAndClose) {
    CDCPersistentLog log(tmp_dir_, "test");
    EXPECT_NO_THROW(log.open());
    EXPECT_TRUE(log.isOpen());
    log.close();
    EXPECT_FALSE(log.isOpen());
}

TEST_F(CDCPersistentLogTest, CDCPL_02_AppendAndReplay) {
    CDCPersistentLog log(tmp_dir_, "test");
    log.open();
    log.append(makeEvent("orders", "o1"));
    log.append(makeEvent("orders", "o2"));
    log.close();

    CDCPersistentLog log2(tmp_dir_, "test");
    log2.open();
    auto events = log2.replayAll();
    log2.close();
    EXPECT_EQ(events.size(), 2u);
}

TEST_F(CDCPersistentLogTest, CDCPL_03_TotalEventsCounter) {
    CDCPersistentLog log(tmp_dir_, "test");
    log.open();
    for (int i = 0; i < 5; ++i) {
        log.append(makeEvent("t", std::to_string(i)));
    }
    EXPECT_EQ(log.totalEventsAppended(), 5u);
    log.close();
}

TEST_F(CDCPersistentLogTest, CDCPL_04_TotalBytesWritten) {
    CDCPersistentLog log(tmp_dir_, "test");
    log.open();
    log.append(makeEvent("t", "k1"));
    EXPECT_GT(log.totalBytesWritten(), 0u);
    log.close();
}

TEST_F(CDCPersistentLogTest, CDCPL_05_AppendBeforeOpenThrows) {
    CDCPersistentLog log(tmp_dir_, "test");
    EXPECT_THROW(log.append(makeEvent("t", "k")), std::runtime_error);
}

TEST_F(CDCPersistentLogTest, CDCPL_06_ReplaySegmentByIndex) {
    CDCPersistentLog log(tmp_dir_, "seg");
    log.open();
    log.append(makeEvent("t", "k1"));
    log.close();

    CDCPersistentLog log2(tmp_dir_, "seg");
    log2.open();
    auto events = log2.replaySegment(0);
    log2.close();
    EXPECT_EQ(events.size(), 1u);
    EXPECT_EQ(events[0].entity_id, "k1");
}

TEST_F(CDCPersistentLogTest, CDCPL_07_ReplaySegmentOutOfRangeThrows) {
    CDCPersistentLog log(tmp_dir_, "seg");
    log.open();
    log.close();
    CDCPersistentLog log2(tmp_dir_, "seg");
    log2.open();
    EXPECT_THROW(log2.replaySegment(99), std::out_of_range);
    log2.close();
}

TEST_F(CDCPersistentLogTest, CDCPL_08_IdempotentOpen) {
    CDCPersistentLog log(tmp_dir_, "test");
    log.open();
    EXPECT_NO_THROW(log.open());  // second open should be no-op
    log.append(makeEvent("t", "k1"));
    log.close();
    auto events = CDCPersistentLog(tmp_dir_, "test").replayAll();
    // Can't replay after construction without open — check via open.
    CDCPersistentLog log2(tmp_dir_, "test");
    log2.open();
    auto ev2 = log2.replayAll();
    log2.close();
    EXPECT_EQ(ev2.size(), 1u);
}
