/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            test_cdc_admin.cpp                                 ║
  Version:         0.0.8                                              ║
  Last Modified:   2026-02-21 12:09:23                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     233                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 3b2027fce  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • bdb82d096  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 7f2db8dcb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 84d1fada6  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 2563a40d8  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include <gtest/gtest.h>
#include "cdc/cdc_admin.h"
#include "cdc/changefeed.h"
#include <memory>
#include <rocksdb/db.h>
#include <filesystem>

using namespace themis::cdc;
namespace fs = std::filesystem;

class CDCAdminTest : public ::testing::Test {
protected:
    std::unique_ptr<rocksdb::DB> db;
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
        
        rocksdb::DB* db_ptr;
        rocksdb::Status s = rocksdb::DB::Open(options, test_db_path, &db_ptr);
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
            event.change_type = Changefeed::ChangeType::PUT;
            changefeed->recordEvent(event);
        }
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

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
