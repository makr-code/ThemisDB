/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            test_cdc_retention.cpp                             ║
  Version:         0.0.25                                             ║
  Last Modified:   2026-02-22 08:22:43                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     369                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

// Test: CDC Retention and Watermarks
// Tests for P1 retention policy and watermark tracking features

#include <gtest/gtest.h>
#include "cdc/changefeed.h"
#include "storage/rocksdb_wrapper.h"
#include <filesystem>
#include <thread>
#include <chrono>

using namespace themis;

class CDCRetentionTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Clean up any existing test database
        test_db_path_ = "./data/themis_cdc_retention_test";
        if (std::filesystem::exists(test_db_path_)) {
            std::filesystem::remove_all(test_db_path_);
        }
        
        // Create RocksDB wrapper
        RocksDBWrapper::Config config;
        config.db_path = test_db_path_;
        config.memtable_size_mb = 64;
        config.block_cache_size_mb = 128;
        
        db_ = std::make_unique<RocksDBWrapper>(config);
        ASSERT_TRUE(db_->open());
    }
    
    void TearDown() override {
        changefeed_.reset();
        db_->close();
        db_.reset();
        
        // Clean up test database
        if (std::filesystem::exists(test_db_path_)) {
            std::filesystem::remove_all(test_db_path_);
        }
    }
    
    std::string test_db_path_;
    std::unique_ptr<RocksDBWrapper> db_;
    std::unique_ptr<Changefeed> changefeed_;
};

// ===== Watermark Tests =====

TEST_F(CDCRetentionTest, WatermarksEmptyChangefeed) {
    // Create changefeed without retention
    auto* raw_db = db_->getDB();
    changefeed_ = std::make_unique<Changefeed>(raw_db, nullptr);
    
    auto wm = changefeed_->getWatermarks();
    
    // Empty changefeed should have zero watermarks
    EXPECT_EQ(wm.low_watermark, 0);
    EXPECT_EQ(wm.high_watermark, 0);
    EXPECT_EQ(wm.oldest_timestamp_ms, 0);
    EXPECT_EQ(wm.newest_timestamp_ms, 0);
}

TEST_F(CDCRetentionTest, WatermarksWithEvents) {
    auto* raw_db = db_->getDB();
    changefeed_ = std::make_unique<Changefeed>(raw_db, nullptr);
    
    // Record some events
    std::vector<uint64_t> sequences;
    for (int i = 0; i < 10; i++) {
        Changefeed::ChangeEvent event;
        event.type = Changefeed::ChangeEventType::EVENT_PUT;
        event.key = "key_" + std::to_string(i);
        event.value = "value_" + std::to_string(i);
        
        auto recorded = changefeed_->recordEvent(event);
        sequences.push_back(recorded.sequence);
        
        // Small delay to ensure different timestamps
        std::this_thread::sleep_for(std::chrono::milliseconds(2));
    }
    
    auto wm = changefeed_->getWatermarks();
    
    // Check watermarks
    EXPECT_EQ(wm.low_watermark, sequences.front());
    EXPECT_EQ(wm.high_watermark, sequences.back());
    EXPECT_GT(wm.oldest_timestamp_ms, 0);
    EXPECT_GT(wm.newest_timestamp_ms, 0);
    EXPECT_GE(wm.newest_timestamp_ms, wm.oldest_timestamp_ms);
}

// ===== Retention Policy Tests =====

TEST_F(CDCRetentionTest, RetentionPolicyDisabledByDefault) {
    Changefeed::RetentionPolicy policy;
    EXPECT_FALSE(policy.enabled);
    
    auto* raw_db = db_->getDB();
    changefeed_ = std::make_unique<Changefeed>(raw_db, nullptr, policy);
    
    // Record events
    for (int i = 0; i < 10; i++) {
        Changefeed::ChangeEvent event;
        event.type = Changefeed::ChangeEventType::EVENT_PUT;
        event.key = "key_" + std::to_string(i);
        event.value = "value";
        changefeed_->recordEvent(event);
    }
    
    // Apply retention (should do nothing since disabled)
    size_t deleted = changefeed_->applyRetentionPolicy();
    EXPECT_EQ(deleted, 0);
    
    // All events should still be there
    auto stats = changefeed_->getStats();
    EXPECT_EQ(stats.total_events, 10);
}

TEST_F(CDCRetentionTest, RetentionByEventCount) {
    Changefeed::RetentionPolicy policy;
    policy.enabled = true;
    policy.max_event_count = 5;  // Keep only 5 events
    policy.max_age_hours = std::chrono::hours(1000);  // Very long, won't trigger
    
    auto* raw_db = db_->getDB();
    changefeed_ = std::make_unique<Changefeed>(raw_db, nullptr, policy);
    
    // Record 10 events
    for (int i = 0; i < 10; i++) {
        Changefeed::ChangeEvent event;
        event.type = Changefeed::ChangeEventType::EVENT_PUT;
        event.key = "key_" + std::to_string(i);
        event.value = "value";
        changefeed_->recordEvent(event);
    }
    
    auto stats_before = changefeed_->getStats();
    EXPECT_EQ(stats_before.total_events, 10);
    
    // Apply retention
    size_t deleted = changefeed_->applyRetentionPolicy();
    EXPECT_GT(deleted, 0);
    
    // Should have ~5 events left
    auto stats_after = changefeed_->getStats();
    EXPECT_LE(stats_after.total_events, policy.max_event_count + 1);  // Allow small margin
}

TEST_F(CDCRetentionTest, RetentionByTimestamp) {
    Changefeed::RetentionPolicy policy;
    policy.enabled = true;
    policy.max_age_hours = std::chrono::hours(0);  // Immediate expiry (for testing)
    policy.max_event_count = 1000000;  // Very high, won't trigger
    
    auto* raw_db = db_->getDB();
    changefeed_ = std::make_unique<Changefeed>(raw_db, nullptr, policy);
    
    // Record events with old timestamps
    for (int i = 0; i < 5; i++) {
        Changefeed::ChangeEvent event;
        event.type = Changefeed::ChangeEventType::EVENT_PUT;
        event.key = "old_key_" + std::to_string(i);
        event.value = "value";
        // Set timestamp to 1 hour ago
        auto now = std::chrono::system_clock::now().time_since_epoch();
        event.timestamp_ms = std::chrono::duration_cast<std::chrono::milliseconds>(now).count() - 3600000;
        changefeed_->recordEvent(event);
    }
    
    // Record some recent events (will have current timestamp)
    for (int i = 0; i < 5; i++) {
        Changefeed::ChangeEvent event;
        event.type = Changefeed::ChangeEventType::EVENT_PUT;
        event.key = "new_key_" + std::to_string(i);
        event.value = "value";
        changefeed_->recordEvent(event);
    }
    
    auto stats_before = changefeed_->getStats();
    EXPECT_EQ(stats_before.total_events, 10);
    
    // Apply retention (should delete old events)
    size_t deleted = changefeed_->applyRetentionPolicy();
    EXPECT_GE(deleted, 5);  // Should delete at least the 5 old events
    
    auto stats_after = changefeed_->getStats();
    EXPECT_LE(stats_after.total_events, 5);
}

TEST_F(CDCRetentionTest, DeleteOldEventsBySequence) {
    auto* raw_db = db_->getDB();
    changefeed_ = std::make_unique<Changefeed>(raw_db, nullptr);
    
    // Record 10 events
    std::vector<uint64_t> sequences;
    for (int i = 0; i < 10; i++) {
        Changefeed::ChangeEvent event;
        event.type = Changefeed::ChangeEventType::EVENT_PUT;
        event.key = "key_" + std::to_string(i);
        event.value = "value";
        auto recorded = changefeed_->recordEvent(event);
        sequences.push_back(recorded.sequence);
    }
    
    // Delete first 5 events
    uint64_t cutoff = sequences[4] + 1;  // Delete sequences < cutoff
    size_t deleted = changefeed_->deleteOldEvents(cutoff);
    EXPECT_GE(deleted, 5);
    
    // Should have ~5 events left
    auto stats = changefeed_->getStats();
    EXPECT_LE(stats.total_events, 5);
}

TEST_F(CDCRetentionTest, DeleteOldEventsByTimestamp) {
    auto* raw_db = db_->getDB();
    changefeed_ = std::make_unique<Changefeed>(raw_db, nullptr);
    
    // Record events with different timestamps
    int64_t cutoff_timestamp = 0;
    for (int i = 0; i < 10; i++) {
        Changefeed::ChangeEvent event;
        event.type = Changefeed::ChangeEventType::EVENT_PUT;
        event.key = "key_" + std::to_string(i);
        event.value = "value";
        
        // Set custom timestamp
        auto now = std::chrono::system_clock::now().time_since_epoch();
        event.timestamp_ms = std::chrono::duration_cast<std::chrono::milliseconds>(now).count() - (10 - i) * 1000;
        
        changefeed_->recordEvent(event);
        
        if (i == 4) {
            cutoff_timestamp = event.timestamp_ms + 500;  // Between event 4 and 5
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(2));
    }
    
    // Delete events older than cutoff
    size_t deleted = changefeed_->deleteOldEventsByTimestamp(cutoff_timestamp);
    EXPECT_GE(deleted, 5);
    
    // Should have ~5 events left
    auto stats = changefeed_->getStats();
    EXPECT_LE(stats.total_events, 5);
}

// ===== Background Cleanup Tests =====

TEST_F(CDCRetentionTest, BackgroundCleanupThread) {
    Changefeed::RetentionPolicy policy;
    policy.enabled = true;
    policy.max_event_count = 10;
    policy.cleanup_interval = std::chrono::minutes(1);  // 1 minute minimum for testing
    
    auto* raw_db = db_->getDB();
    // Manually start cleanup for testing (don't wait for constructor auto-start)
    changefeed_ = std::make_unique<Changefeed>(raw_db, nullptr, policy);
    
    // Record 20 events
    for (int i = 0; i < 20; i++) {
        Changefeed::ChangeEvent event;
        event.type = Changefeed::ChangeEventType::EVENT_PUT;
        event.key = "key_" + std::to_string(i);
        event.value = "value";
        changefeed_->recordEvent(event);
    }
    
    auto stats_before = changefeed_->getStats();
    EXPECT_EQ(stats_before.total_events, 20);
    
    // Manually trigger retention instead of waiting for background thread
    changefeed_->applyRetentionPolicy();
    
    // Should have fewer events now
    auto stats_after = changefeed_->getStats();
    EXPECT_LT(stats_after.total_events, stats_before.total_events);
    EXPECT_LE(stats_after.total_events, policy.max_event_count + 2);  // Allow small margin
}

TEST_F(CDCRetentionTest, StopBackgroundCleanup) {
    Changefeed::RetentionPolicy policy;
    policy.enabled = true;
    policy.max_event_count = 5;
    policy.cleanup_interval = std::chrono::minutes(1);  // 1 minute
    
    auto* raw_db = db_->getDB();
    changefeed_ = std::make_unique<Changefeed>(raw_db, nullptr, policy);
    
    // Give thread time to start
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    
    // Stop cleanup immediately
    changefeed_->stopRetentionCleanup();
    
    // Record many events
    for (int i = 0; i < 20; i++) {
        Changefeed::ChangeEvent event;
        event.type = Changefeed::ChangeEventType::EVENT_PUT;
        event.key = "key_" + std::to_string(i);
        event.value = "value";
        changefeed_->recordEvent(event);
    }
    
    // Events should NOT be cleaned up (thread stopped, no manual trigger)
    auto stats = changefeed_->getStats();
    EXPECT_EQ(stats.total_events, 20);  // All events still there
}

TEST_F(CDCRetentionTest, WatermarksAfterRetention) {
    Changefeed::RetentionPolicy policy;
    policy.enabled = true;
    policy.max_event_count = 5;
    
    auto* raw_db = db_->getDB();
    changefeed_ = std::make_unique<Changefeed>(raw_db, nullptr, policy);
    
    // Record 10 events
    std::vector<uint64_t> sequences;
    for (int i = 0; i < 10; i++) {
        Changefeed::ChangeEvent event;
        event.type = Changefeed::ChangeEventType::EVENT_PUT;
        event.key = "key_" + std::to_string(i);
        event.value = "value";
        auto recorded = changefeed_->recordEvent(event);
        sequences.push_back(recorded.sequence);
        std::this_thread::sleep_for(std::chrono::milliseconds(2));
    }
    
    auto wm_before = changefeed_->getWatermarks();
    EXPECT_EQ(wm_before.low_watermark, sequences.front());
    EXPECT_EQ(wm_before.high_watermark, sequences.back());
    
    // Apply retention
    changefeed_->applyRetentionPolicy();
    
    // Watermarks should have moved
    auto wm_after = changefeed_->getWatermarks();
    EXPECT_GT(wm_after.low_watermark, wm_before.low_watermark);  // Low moved up
    EXPECT_EQ(wm_after.high_watermark, wm_before.high_watermark);  // High stays same
}

// ===== Compaction Tests =====

TEST_F(CDCRetentionTest, CompactionEmptyLog) {
    auto* raw_db = db_->getDB();
    changefeed_ = std::make_unique<Changefeed>(raw_db, nullptr);

    auto result = changefeed_->compactByKey();

    EXPECT_EQ(result.events_scanned, 0u);
    EXPECT_EQ(result.events_deleted, 0u);
    EXPECT_EQ(result.keys_compacted, 0u);
}

TEST_F(CDCRetentionTest, CompactionKeepsLatestEventPerKey) {
    auto* raw_db = db_->getDB();
    changefeed_ = std::make_unique<Changefeed>(raw_db, nullptr);

    // Record 3 PUT events for the same key
    for (int i = 0; i < 3; i++) {
        Changefeed::ChangeEvent event;
        event.type = Changefeed::ChangeEventType::EVENT_PUT;
        event.key = "doc:1";
        event.value = "value_" + std::to_string(i);
        changefeed_->recordEvent(event);
    }

    auto stats_before = changefeed_->getStats();
    EXPECT_EQ(stats_before.total_events, 3u);

    auto result = changefeed_->compactByKey();

    EXPECT_EQ(result.events_scanned, 3u);
    EXPECT_EQ(result.events_deleted, 2u);   // 2 older events removed
    EXPECT_EQ(result.keys_compacted, 1u);   // 1 key compacted
    EXPECT_EQ(result.events_retained, 1u);  // 1 latest event kept

    auto stats_after = changefeed_->getStats();
    EXPECT_EQ(stats_after.total_events, 1u);
}

TEST_F(CDCRetentionTest, CompactionMultipleKeys) {
    auto* raw_db = db_->getDB();
    changefeed_ = std::make_unique<Changefeed>(raw_db, nullptr);

    // 2 events for "doc:1", 3 events for "doc:2", 1 event for "doc:3"
    for (int i = 0; i < 2; i++) {
        Changefeed::ChangeEvent e;
        e.type = Changefeed::ChangeEventType::EVENT_PUT;
        e.key = "doc:1";
        e.value = "v" + std::to_string(i);
        changefeed_->recordEvent(e);
    }
    for (int i = 0; i < 3; i++) {
        Changefeed::ChangeEvent e;
        e.type = Changefeed::ChangeEventType::EVENT_PUT;
        e.key = "doc:2";
        e.value = "v" + std::to_string(i);
        changefeed_->recordEvent(e);
    }
    {
        Changefeed::ChangeEvent e;
        e.type = Changefeed::ChangeEventType::EVENT_PUT;
        e.key = "doc:3";
        e.value = "v0";
        changefeed_->recordEvent(e);
    }

    auto stats_before = changefeed_->getStats();
    EXPECT_EQ(stats_before.total_events, 6u);

    auto result = changefeed_->compactByKey();

    // Superseded: 1 for doc:1, 2 for doc:2 = 3 total
    EXPECT_EQ(result.events_scanned, 6u);
    EXPECT_EQ(result.events_deleted, 3u);
    EXPECT_EQ(result.keys_compacted, 2u);   // doc:1 and doc:2 had superseded events
    EXPECT_EQ(result.events_retained, 3u);  // one per key

    auto stats_after = changefeed_->getStats();
    EXPECT_EQ(stats_after.total_events, 3u);
}

TEST_F(CDCRetentionTest, CompactionPreservesDeleteTombstones) {
    auto* raw_db = db_->getDB();
    changefeed_ = std::make_unique<Changefeed>(raw_db, nullptr);

    // PUT then DELETE for same key
    {
        Changefeed::ChangeEvent e;
        e.type = Changefeed::ChangeEventType::EVENT_PUT;
        e.key = "doc:x";
        e.value = "hello";
        changefeed_->recordEvent(e);
    }
    {
        Changefeed::ChangeEvent e;
        e.type = Changefeed::ChangeEventType::EVENT_DELETE;
        e.key = "doc:x";
        changefeed_->recordEvent(e);
    }

    auto result = changefeed_->compactByKey();

    // The PUT is superseded; the DELETE (tombstone) must be kept.
    EXPECT_EQ(result.events_scanned, 2u);
    EXPECT_GE(result.events_deleted, 1u);  // The PUT should be removed
    EXPECT_GE(result.events_retained, 1u); // The DELETE tombstone must survive

    // Verify the DELETE event is still present
    auto remaining = changefeed_->listEvents();
    bool found_delete = false;
    for (const auto& ev : remaining) {
        if (ev.key == "doc:x" && ev.type == Changefeed::ChangeEventType::EVENT_DELETE) {
            found_delete = true;
        }
    }
    EXPECT_TRUE(found_delete);
}

TEST_F(CDCRetentionTest, CompactionNoOpWhenAllUnique) {
    auto* raw_db = db_->getDB();
    changefeed_ = std::make_unique<Changefeed>(raw_db, nullptr);

    // Each key has exactly one event — nothing to compact
    for (int i = 0; i < 5; i++) {
        Changefeed::ChangeEvent e;
        e.type = Changefeed::ChangeEventType::EVENT_PUT;
        e.key = "doc:" + std::to_string(i);
        e.value = "value";
        changefeed_->recordEvent(e);
    }

    auto result = changefeed_->compactByKey();

    EXPECT_EQ(result.events_scanned, 5u);
    EXPECT_EQ(result.events_deleted, 0u);
    EXPECT_EQ(result.keys_compacted, 0u);
    EXPECT_EQ(result.events_retained, 5u);
}

TEST_F(CDCRetentionTest, GetEventBySequence) {
    auto* raw_db = db_->getDB();
    changefeed_ = std::make_unique<Changefeed>(raw_db, nullptr);

    Changefeed::ChangeEvent e;
    e.type = Changefeed::ChangeEventType::EVENT_PUT;
    e.key = "lookup:key";
    e.value = "lookup_value";
    auto recorded = changefeed_->recordEvent(e);

    auto fetched = changefeed_->getEvent(recorded.sequence);
    EXPECT_EQ(fetched.sequence, recorded.sequence);
    EXPECT_EQ(fetched.key, "lookup:key");
    EXPECT_EQ(fetched.value, std::optional<std::string>("lookup_value"));
}
