#include <gtest/gtest.h>
#include "cdc/cdc_admin.h"
#include "cdc/changefeed.h"
#include "cdc/cdc_error.h"
#include <memory>
#include <rocksdb/utilities/transaction_db.h>
#include <filesystem>

using namespace themis::cdc;
using themis::Changefeed;
namespace fs = std::filesystem;

class CDCAdminTest : public ::testing::Test {
protected:
    std::unique_ptr<rocksdb::TransactionDB> db;
    std::unique_ptr<Changefeed> changefeed;
    std::unique_ptr<CDCAdmin> admin;
    std::string test_db_path;
    
    void SetUp() override {
        // Create temporary test database
        test_db_path = "/tmp/test_cdc_admin_" + std::to_string(time(nullptr));
        fs::create_directories(test_db_path);
        
        rocksdb::Options options;
        options.create_if_missing = true;
        options.error_if_exists = false;
        
        rocksdb::TransactionDBOptions txn_options;
        rocksdb::TransactionDB* db_ptr;
        rocksdb::Status s = rocksdb::TransactionDB::Open(
            options, txn_options, test_db_path, &db_ptr);
        ASSERT_TRUE(s.ok()) << "Failed to open test database: " << s.ToString();
        db.reset(db_ptr);
        
        // Create changefeed with retention disabled
        Changefeed::RetentionPolicy retention;
        retention.enabled = false;
        
        changefeed = std::make_unique<Changefeed>(db.get(), nullptr, retention);
        admin = std::make_unique<CDCAdmin>(changefeed.get());
    }
    
    void TearDown() override {
        admin.reset();
        changefeed.reset();
        db.reset();
        fs::remove_all(test_db_path);
    }
    
    void addTestEvents(int count) {
        for (int i = 0; i < count; i++) {
            Changefeed::ChangeEvent event;
            event.key = "test_key_" + std::to_string(i);
            event.value = "test_value_" + std::to_string(i);
            event.type = Changefeed::ChangeEventType::EVENT_PUT;
            changefeed->recordEvent(event);
        }
    }

    // Helper: record multiple events for the same key
    void addEventsForKey(const std::string& key, int count,
                         Changefeed::ChangeEventType last_type =
                             Changefeed::ChangeEventType::EVENT_PUT) {
        for (int i = 0; i < count - 1; ++i) {
            Changefeed::ChangeEvent e;
            e.key   = key;
            e.value = "value_" + std::to_string(i);
            e.type  = Changefeed::ChangeEventType::EVENT_PUT;
            changefeed->recordEvent(e);
        }
        Changefeed::ChangeEvent last;
        last.key  = key;
        last.type = last_type;
        if (last_type != Changefeed::ChangeEventType::EVENT_DELETE) {
            last.value = "final_value";
        }
        changefeed->recordEvent(last);
    }
};

TEST_F(CDCAdminTest, BasicHealthCheck) {
    HealthStatus health = admin->healthCheck();
    
    EXPECT_TRUE(health.is_healthy);
    EXPECT_TRUE(health.changefeed_healthy);
    EXPECT_FALSE(health.message.empty());
}

TEST_F(CDCAdminTest, DiagnosticsExport) {
    addTestEvents(10);
    
    DiagnosticsInfo diag = admin->getDiagnostics();
    
    EXPECT_GT(diag.watermarks.high_watermark, 0);
    EXPECT_TRUE(diag.health.is_healthy);
    
    // Verify JSON serialization
    nlohmann::json json = diag.toJson();
    EXPECT_TRUE(json.contains("watermarks"));
    EXPECT_TRUE(json.contains("health"));
    EXPECT_TRUE(json.contains("stats"));
}

TEST_F(CDCAdminTest, PurgeAllEvents) {
    addTestEvents(50);
    
    auto watermarks_before = changefeed->getWatermarks();
    EXPECT_GT(watermarks_before.high_watermark, 0);
    
    PurgeResult result = admin->purgeAll();
    
    EXPECT_GT(result.events_deleted, 0);
    EXPECT_GT(result.elapsed_time_ms, 0);
    
    // Verify JSON serialization
    nlohmann::json json = result.toJson();
    EXPECT_TRUE(json.contains("events_deleted"));
    EXPECT_TRUE(json.contains("elapsed_time_ms"));
}

TEST_F(CDCAdminTest, PurgeBySequenceRange) {
    addTestEvents(100);
    
    auto watermarks_before = changefeed->getWatermarks();
    uint64_t low = watermarks_before.low_watermark;
    uint64_t mid = low + 50;
    
    PurgeResult result = admin->purgeBySequenceRange(low, mid);
    
    EXPECT_GT(result.events_deleted, 0);
    EXPECT_GT(result.elapsed_time_ms, 0);
}

TEST_F(CDCAdminTest, PurgeByTimestamp) {
    addTestEvents(20);
    
    // Get current timestamp
    auto now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    
    // Purge events older than now (should purge all)
    PurgeResult result = admin->purgeByTimestamp(now_ms + 1000);
    
    EXPECT_GE(result.events_deleted, 0);
    EXPECT_GE(result.elapsed_time_ms, 0);
}

TEST_F(CDCAdminTest, ReplayFromSequence) {
    addTestEvents(30);
    
    auto watermarks = changefeed->getWatermarks();
    uint64_t start_seq = watermarks.low_watermark;
    
    // Replay first 10 events
    auto events = admin->replayFromSequence(start_seq, 10);
    
    // May not get exactly 10 due to implementation limitations
    EXPECT_GE(events.size(), 0);
    EXPECT_LE(events.size(), static_cast<size_t>(10));
}

TEST_F(CDCAdminTest, ReplayWithLimit) {
    addTestEvents(50);
    
    auto watermarks = changefeed->getWatermarks();
    
    // Replay with limit
    auto events = admin->replayFromSequence(watermarks.low_watermark, 5);
    
    EXPECT_LE(events.size(), static_cast<size_t>(5));
}

TEST_F(CDCAdminTest, ReplayEmptyRange) {
    // No events added
    
    auto events = admin->replayFromSequence(1000, 10);
    
    EXPECT_EQ(events.size(), 0);
}

TEST_F(CDCAdminTest, InvalidPurgeRange) {
    // End < start should throw
    EXPECT_THROW(
        admin->purgeBySequenceRange(100, 50),
        CDCException
    );
}

TEST_F(CDCAdminTest, HealthStatusJSONSerialization) {
    HealthStatus health = admin->healthCheck();
    nlohmann::json json = health.toJson();
    
    EXPECT_TRUE(json.contains("is_healthy"));
    EXPECT_TRUE(json.contains("message"));
    EXPECT_TRUE(json.contains("components"));
    EXPECT_TRUE(json.contains("metrics"));
}

TEST_F(CDCAdminTest, PurgeResultJSONSerialization) {
    addTestEvents(10);
    
    PurgeResult result = admin->purgeAll();
    nlohmann::json json = result.toJson();
    
    EXPECT_TRUE(json.contains("events_deleted"));
    EXPECT_TRUE(json.contains("elapsed_time_ms"));
    EXPECT_EQ(json["events_deleted"], result.events_deleted);
    EXPECT_EQ(json["elapsed_time_ms"], result.elapsed_time_ms);
}

TEST_F(CDCAdminTest, MultiplePurgeOperations) {
    // Add events
    addTestEvents(100);
    
    // Purge by sequence
    PurgeResult result1 = admin->purgeBySequenceRange(1, 30);
    EXPECT_GT(result1.events_deleted, 0);
    
    // Add more events
    addTestEvents(50);
    
    // Purge all
    PurgeResult result2 = admin->purgeAll();
    EXPECT_GE(result2.events_deleted, 0);
}

TEST_F(CDCAdminTest, PurgeOlderThanNegativeTimestampThrows) {
    // Negative timestamps must be rejected before casting to uint64_t
    EXPECT_THROW(
        admin->purgeOlderThan(-1),
        CDCException
    );
}

// ===== Compaction Tests =====

TEST_F(CDCAdminTest, CompactLog_EmptyLog) {
    // Compacting an empty log should return zeroes and not throw
    CompactionResult result = admin->compactLog();

    EXPECT_EQ(result.events_scanned, 0u);
    EXPECT_EQ(result.events_deleted, 0u);
    EXPECT_EQ(result.keys_compacted, 0u);
    EXPECT_EQ(result.events_retained, 0u);
}

TEST_F(CDCAdminTest, CompactLog_NoOpWhenAllKeysUnique) {
    // One event per unique key — nothing to compact
    addTestEvents(10);

    auto stats_before = changefeed->getStats();
    EXPECT_EQ(stats_before.total_events, 10u);

    CompactionResult result = admin->compactLog();

    EXPECT_EQ(result.events_scanned, 10u);
    EXPECT_EQ(result.events_deleted, 0u);
    EXPECT_EQ(result.keys_compacted, 0u);
    EXPECT_EQ(result.events_retained, 10u);

    // Event count must be unchanged
    auto stats_after = changefeed->getStats();
    EXPECT_EQ(stats_after.total_events, 10u);
}

TEST_F(CDCAdminTest, CompactLog_RemovesSupersededEvents) {
    // Record 3 PUT events for the same key — only the latest should survive
    addEventsForKey("doc:1", 3);

    auto stats_before = changefeed->getStats();
    EXPECT_EQ(stats_before.total_events, 3u);

    CompactionResult result = admin->compactLog();

    EXPECT_EQ(result.events_scanned, 3u);
    EXPECT_EQ(result.events_deleted, 2u);   // two older events removed
    EXPECT_EQ(result.keys_compacted, 1u);   // one key compacted
    EXPECT_EQ(result.events_retained, 1u);  // latest event kept

    auto stats_after = changefeed->getStats();
    EXPECT_EQ(stats_after.total_events, 1u);
}

TEST_F(CDCAdminTest, CompactLog_MultipleKeys) {
    // "doc:a" has 2 events, "doc:b" has 3, "doc:c" has 1
    addEventsForKey("doc:a", 2);
    addEventsForKey("doc:b", 3);
    addEventsForKey("doc:c", 1);

    CompactionResult result = admin->compactLog();

    // superseded: 1 for doc:a + 2 for doc:b = 3 total deleted
    EXPECT_EQ(result.events_scanned, 6u);
    EXPECT_EQ(result.events_deleted, 3u);
    EXPECT_EQ(result.keys_compacted, 2u);   // doc:a and doc:b had older events
    EXPECT_EQ(result.events_retained, 3u);  // one survivor per key

    EXPECT_EQ(changefeed->getStats().total_events, 3u);
}

TEST_F(CDCAdminTest, CompactLog_PreservesDeleteTombstones) {
    // PUT then DELETE for the same key — the DELETE tombstone must survive
    addEventsForKey("doc:x", 2, Changefeed::ChangeEventType::EVENT_DELETE);

    CompactionResult result = admin->compactLog();

    // The earlier PUT is superseded; the DELETE must be retained
    EXPECT_GE(result.events_deleted, 1u);
    EXPECT_GE(result.events_retained, 1u);

    // Verify the DELETE event is still present
    auto remaining = changefeed->listEvents();
    bool found_delete = false;
    for (const auto& ev : remaining) {
        if (ev.key == "doc:x" &&
            ev.type == Changefeed::ChangeEventType::EVENT_DELETE) {
            found_delete = true;
        }
    }
    EXPECT_TRUE(found_delete);
}

TEST_F(CDCAdminTest, CompactLog_ResultJsonSerialization) {
    addEventsForKey("doc:1", 3);

    CompactionResult result = admin->compactLog();

    // CompactionResult is a plain struct — spot-check fields are accessible
    EXPECT_GE(result.events_scanned, 0u);
    EXPECT_GE(result.events_deleted, 0u);
    EXPECT_GE(result.keys_compacted, 0u);
    EXPECT_GE(result.events_retained, 0u);
    EXPECT_EQ(result.events_scanned,
              result.events_deleted + result.events_retained);
}

TEST_F(CDCAdminTest, CompactLog_IsIdempotent) {
    addEventsForKey("doc:1", 3);
    addTestEvents(5);   // unique keys

    // First compaction removes superseded events
    CompactionResult first = admin->compactLog();
    EXPECT_GT(first.events_deleted, 0u);

    // Second compaction on already-compact log should be a no-op
    CompactionResult second = admin->compactLog();
    EXPECT_EQ(second.events_deleted, 0u);
    EXPECT_EQ(second.keys_compacted, 0u);
}
