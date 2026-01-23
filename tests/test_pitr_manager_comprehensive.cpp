/**
 * @file test_pitr_manager_comprehensive.cpp
 * @brief Comprehensive real unit tests for Point-in-Time Recovery (PITR) Manager
 * 
 * Test Intent:
 * - Validate PITR Manager functionality with real changefeed and snapshot operations
 * - Test point-in-time recovery to specific timestamps
 * - Verify changefeed integration and event replay
 * - Test recovery consistency across multiple entities
 * - Validate timeline management and branch handling
 * - Test error conditions and recovery failures
 * 
 * Coverage: Storage layer (PITR, Changefeed, Snapshot Manager)
 * No stubs - all tests use real RocksDB, Changefeed, and SnapshotManager
 */

#include <gtest/gtest.h>
#include "storage/pitr_manager.h"
#include "storage/rocksdb_wrapper.h"
#include "transaction/snapshot_manager.h"
#include "cdc/changefeed.h"
#include <filesystem>
#include <thread>
#include <chrono>

using namespace themis;
namespace fs = std::filesystem;

class PITRManagerComprehensiveTest : public ::testing::Test {
protected:
    void SetUp() override {
        test_dir_ = fs::temp_directory_path() / "pitr_comprehensive_test";
        cleanupTestDir();
        fs::create_directories(test_dir_);
        
        // Initialize RocksDB
        RocksDBWrapper::Config config;
        config.db_path = test_dir_.string();
        config.enable_wal = true;
        config.enable_statistics = false;
        
        db_wrapper_ = std::make_unique<RocksDBWrapper>(config);
        ASSERT_TRUE(db_wrapper_->open());
        
        // Initialize Changefeed
        auto* raw_db = db_wrapper_->getRawDB();
        changefeed_ = std::make_unique<Changefeed>(raw_db, nullptr);
        
        // Initialize SnapshotManager
        snapshot_mgr_ = std::make_unique<transaction::SnapshotManager>(*db_wrapper_, *changefeed_);
        
        // Initialize PITRManager
        pitr_mgr_ = std::make_unique<PITRManager>(
            db_wrapper_.get(), changefeed_.get(), snapshot_mgr_.get()
        );
    }
    
    void TearDown() override {
        pitr_mgr_.reset();
        snapshot_mgr_.reset();
        changefeed_.reset();
        db_wrapper_.reset();
        cleanupTestDir();
    }
    
    void cleanupTestDir() {
        std::error_code ec;
        fs::remove_all(test_dir_, ec);
    }
    
    // Helper to add test events with timestamps
    void addEvent(Changefeed::ChangeEventType type, const std::string& key, 
                  const std::string& value, int64_t timestamp_ms) {
        Changefeed::ChangeEvent event;
        event.type = type;
        event.key = key;
        event.timestamp_ms = timestamp_ms;
        
        if (type == Changefeed::ChangeEventType::EVENT_PUT) {
            event.value = value;
        } else {
            event.value = std::nullopt;
        }
        
        changefeed_->recordEvent(event);
    }
    
    fs::path test_dir_;
    std::unique_ptr<RocksDBWrapper> db_wrapper_;
    std::unique_ptr<Changefeed> changefeed_;
    std::unique_ptr<transaction::SnapshotManager> snapshot_mgr_;
    std::unique_ptr<PITRManager> pitr_mgr_;
};

// ============================================================================
// Basic PITR Functionality Tests
// ============================================================================

TEST_F(PITRManagerComprehensiveTest, RecoverToSpecificTimestamp) {
    // Intent: Verify PITR can recover database to a specific point in time
    
    // Event 1: Initial insert at t=1000
    addEvent(Changefeed::ChangeEventType::EVENT_PUT, "users:1", 
             R"({"name":"Alice","age":30})", 1000);
    
    // Event 2: Update at t=2000
    addEvent(Changefeed::ChangeEventType::EVENT_PUT, "users:1", 
             R"({"name":"Alice","age":31})", 2000);
    
    // Event 3: Another insert at t=3000
    addEvent(Changefeed::ChangeEventType::EVENT_PUT, "users:2", 
             R"({"name":"Bob","age":25})", 3000);
    
    // Event 4: Delete at t=4000
    addEvent(Changefeed::ChangeEventType::EVENT_DELETE, "users:1", "", 4000);
    
    // Recover to t=2500 (after update, before new insert)
    auto result = pitr_mgr_->recoverToTimestamp(2500);
    ASSERT_TRUE(result.success) << "Recovery failed: " << result.error_message;
    
    // Verify state at t=2500
    auto val1 = db_wrapper_->get("users:1");
    ASSERT_TRUE(val1.has_value()) << "users:1 should exist at t=2500";
    
    auto val2 = db_wrapper_->get("users:2");
    EXPECT_FALSE(val2.has_value()) << "users:2 should not exist yet at t=2500";
}

TEST_F(PITRManagerComprehensiveTest, RecoverToEarliestPoint) {
    // Intent: Verify recovery to earliest available timestamp
    
    addEvent(Changefeed::ChangeEventType::EVENT_PUT, "key1", "value1", 1000);
    addEvent(Changefeed::ChangeEventType::EVENT_PUT, "key2", "value2", 2000);
    addEvent(Changefeed::ChangeEventType::EVENT_PUT, "key3", "value3", 3000);
    
    // Recover to t=500 (before any events)
    auto result = pitr_mgr_->recoverToTimestamp(500);
    
    // Should either succeed with empty state or indicate earliest point
    if (result.success) {
        auto val = db_wrapper_->get("key1");
        EXPECT_FALSE(val.has_value()) << "No keys should exist before t=1000";
    }
}

TEST_F(PITRManagerComprehensiveTest, RecoverToLatestPoint) {
    // Intent: Verify recovery to latest timestamp restores all changes
    
    addEvent(Changefeed::ChangeEventType::EVENT_PUT, "latest:1", "v1", 1000);
    addEvent(Changefeed::ChangeEventType::EVENT_PUT, "latest:2", "v2", 2000);
    addEvent(Changefeed::ChangeEventType::EVENT_PUT, "latest:3", "v3", 3000);
    
    // Recover to t=9999 (future timestamp)
    auto result = pitr_mgr_->recoverToTimestamp(9999);
    ASSERT_TRUE(result.success);
    
    // All keys should exist
    EXPECT_TRUE(db_wrapper_->get("latest:1").has_value());
    EXPECT_TRUE(db_wrapper_->get("latest:2").has_value());
    EXPECT_TRUE(db_wrapper_->get("latest:3").has_value());
}

// ============================================================================
// Delete Event Recovery Tests
// ============================================================================

TEST_F(PITRManagerComprehensiveTest, RecoverBeforeDelete) {
    // Intent: Verify key exists when recovering before delete event
    
    addEvent(Changefeed::ChangeEventType::EVENT_PUT, "delete:test", "data", 1000);
    addEvent(Changefeed::ChangeEventType::EVENT_DELETE, "delete:test", "", 2000);
    
    // Recover to t=1500 (before delete)
    auto result = pitr_mgr_->recoverToTimestamp(1500);
    ASSERT_TRUE(result.success);
    
    auto val = db_wrapper_->get("delete:test");
    EXPECT_TRUE(val.has_value()) << "Key should exist before delete";
}

TEST_F(PITRManagerComprehensiveTest, RecoverAfterDelete) {
    // Intent: Verify key doesn't exist when recovering after delete event
    
    addEvent(Changefeed::ChangeEventType::EVENT_PUT, "delete:test2", "data", 1000);
    addEvent(Changefeed::ChangeEventType::EVENT_DELETE, "delete:test2", "", 2000);
    
    // Recover to t=2500 (after delete)
    auto result = pitr_mgr_->recoverToTimestamp(2500);
    ASSERT_TRUE(result.success);
    
    auto val = db_wrapper_->get("delete:test2");
    EXPECT_FALSE(val.has_value()) << "Key should not exist after delete";
}

TEST_F(PITRManagerComprehensiveTest, RecoverDeleteThenReinsert) {
    // Intent: Verify recovery handles delete followed by re-insert
    
    addEvent(Changefeed::ChangeEventType::EVENT_PUT, "reinsert", "v1", 1000);
    addEvent(Changefeed::ChangeEventType::EVENT_DELETE, "reinsert", "", 2000);
    addEvent(Changefeed::ChangeEventType::EVENT_PUT, "reinsert", "v2", 3000);
    
    // Recover to t=2500 (after delete, before re-insert)
    auto result = pitr_mgr_->recoverToTimestamp(2500);
    ASSERT_TRUE(result.success);
    
    auto val = db_wrapper_->get("reinsert");
    EXPECT_FALSE(val.has_value());
    
    // Recover to t=3500 (after re-insert)
    result = pitr_mgr_->recoverToTimestamp(3500);
    ASSERT_TRUE(result.success);
    
    val = db_wrapper_->get("reinsert");
    ASSERT_TRUE(val.has_value());
    std::string str(val->begin(), val->end());
    EXPECT_EQ(str, "v2");
}

// ============================================================================
// Multi-Key Recovery Tests
// ============================================================================

TEST_F(PITRManagerComprehensiveTest, RecoverMultipleEntities) {
    // Intent: Verify PITR recovers consistent state across multiple entities
    
    // User events
    addEvent(Changefeed::ChangeEventType::EVENT_PUT, "users:1", 
             R"({"name":"Alice"})", 1000);
    addEvent(Changefeed::ChangeEventType::EVENT_PUT, "users:2", 
             R"({"name":"Bob"})", 1500);
    
    // Product events
    addEvent(Changefeed::ChangeEventType::EVENT_PUT, "products:1", 
             R"({"name":"Widget"})", 2000);
    addEvent(Changefeed::ChangeEventType::EVENT_PUT, "products:2", 
             R"({"name":"Gadget"})", 2500);
    
    // Order events
    addEvent(Changefeed::ChangeEventType::EVENT_PUT, "orders:1", 
             R"({"user":"1","product":"1"})", 3000);
    
    // Recover to t=2200 (2 users, 1 product, no orders)
    auto result = pitr_mgr_->recoverToTimestamp(2200);
    ASSERT_TRUE(result.success);
    
    EXPECT_TRUE(db_wrapper_->get("users:1").has_value());
    EXPECT_TRUE(db_wrapper_->get("users:2").has_value());
    EXPECT_TRUE(db_wrapper_->get("products:1").has_value());
    EXPECT_FALSE(db_wrapper_->get("products:2").has_value());
    EXPECT_FALSE(db_wrapper_->get("orders:1").has_value());
}

TEST_F(PITRManagerComprehensiveTest, RecoverHighVolumeChanges) {
    // Intent: Verify PITR handles large number of changefeed events
    
    const int num_events = 1000;
    for (int i = 0; i < num_events; ++i) {
        std::string key = "bulk:key" + std::to_string(i);
        std::string value = "value" + std::to_string(i);
        addEvent(Changefeed::ChangeEventType::EVENT_PUT, key, value, 
                 1000 + i);
    }
    
    // Recover to t=1500 (first 500 events)
    auto result = pitr_mgr_->recoverToTimestamp(1500);
    ASSERT_TRUE(result.success);
    
    // Verify first 500 keys exist
    for (int i = 0; i < 500; ++i) {
        auto val = db_wrapper_->get("bulk:key" + std::to_string(i));
        EXPECT_TRUE(val.has_value()) << "Key " << i << " should exist";
    }
    
    // Verify remaining keys don't exist
    for (int i = 500; i < num_events; ++i) {
        auto val = db_wrapper_->get("bulk:key" + std::to_string(i));
        EXPECT_FALSE(val.has_value()) << "Key " << i << " should not exist yet";
    }
}

// ============================================================================
// Update Event Recovery Tests
// ============================================================================

TEST_F(PITRManagerComprehensiveTest, RecoverBetweenUpdates) {
    // Intent: Verify recovery captures correct version between updates
    
    addEvent(Changefeed::ChangeEventType::EVENT_PUT, "update:key", "v1", 1000);
    addEvent(Changefeed::ChangeEventType::EVENT_PUT, "update:key", "v2", 2000);
    addEvent(Changefeed::ChangeEventType::EVENT_PUT, "update:key", "v3", 3000);
    addEvent(Changefeed::ChangeEventType::EVENT_PUT, "update:key", "v4", 4000);
    
    // Recover to t=2500 (should have v2)
    auto result = pitr_mgr_->recoverToTimestamp(2500);
    ASSERT_TRUE(result.success);
    
    auto val = db_wrapper_->get("update:key");
    ASSERT_TRUE(val.has_value());
    std::string str(val->begin(), val->end());
    EXPECT_EQ(str, "v2");
}

TEST_F(PITRManagerComprehensiveTest, RecoverRapidUpdates) {
    // Intent: Verify PITR handles rapid successive updates correctly
    
    for (int i = 0; i < 100; ++i) {
        std::string value = "version" + std::to_string(i);
        addEvent(Changefeed::ChangeEventType::EVENT_PUT, "rapid:key", 
                 value, 1000 + i * 10);
    }
    
    // Recover to t=1555 (should have version55)
    auto result = pitr_mgr_->recoverToTimestamp(1555);
    ASSERT_TRUE(result.success);
    
    auto val = db_wrapper_->get("rapid:key");
    ASSERT_TRUE(val.has_value());
    std::string str(val->begin(), val->end());
    EXPECT_EQ(str, "version55");
}

// ============================================================================
// Snapshot Integration Tests
// ============================================================================

TEST_F(PITRManagerComprehensiveTest, CreateRecoverySnapshot) {
    // Intent: Verify PITR can create snapshots for recovery points
    
    addEvent(Changefeed::ChangeEventType::EVENT_PUT, "snap:1", "data1", 1000);
    addEvent(Changefeed::ChangeEventType::EVENT_PUT, "snap:2", "data2", 2000);
    
    // Create snapshot at t=1500
    auto snapshot_id = pitr_mgr_->createRecoverySnapshot(1500);
    EXPECT_FALSE(snapshot_id.empty());
    
    // Add more events
    addEvent(Changefeed::ChangeEventType::EVENT_PUT, "snap:3", "data3", 3000);
    
    // Recover using snapshot should restore to t=1500 state
    auto result = pitr_mgr_->recoverFromSnapshot(snapshot_id);
    ASSERT_TRUE(result.success);
    
    EXPECT_TRUE(db_wrapper_->get("snap:1").has_value());
    EXPECT_FALSE(db_wrapper_->get("snap:2").has_value());
    EXPECT_FALSE(db_wrapper_->get("snap:3").has_value());
}

// ============================================================================
// Error Handling Tests
// ============================================================================

TEST_F(PITRManagerComprehensiveTest, RecoverWithNoChangefeedData) {
    // Intent: Verify PITR handles empty changefeed gracefully
    
    auto result = pitr_mgr_->recoverToTimestamp(1000);
    
    // Should either succeed with empty state or indicate no data
    EXPECT_TRUE(result.success || !result.error_message.empty());
}

TEST_F(PITRManagerComprehensiveTest, RecoverWithInvalidTimestamp) {
    // Intent: Verify PITR handles invalid timestamps gracefully
    
    addEvent(Changefeed::ChangeEventType::EVENT_PUT, "key", "value", 1000);
    
    // Try to recover to negative timestamp
    auto result = pitr_mgr_->recoverToTimestamp(-1000);
    
    // Should fail gracefully
    EXPECT_FALSE(result.success || result.error_message.empty());
}

TEST_F(PITRManagerComprehensiveTest, RecoverWithCorruptedChangefeed) {
    // Intent: Verify PITR handles corrupted changefeed entries
    
    addEvent(Changefeed::ChangeEventType::EVENT_PUT, "key1", "value1", 1000);
    
    // Add event with invalid JSON
    Changefeed::ChangeEvent corrupt_event;
    corrupt_event.type = Changefeed::ChangeEventType::EVENT_PUT;
    corrupt_event.key = "corrupt:key";
    corrupt_event.value = "{invalid json}";
    corrupt_event.timestamp_ms = 2000;
    changefeed_->recordEvent(corrupt_event);
    
    addEvent(Changefeed::ChangeEventType::EVENT_PUT, "key2", "value2", 3000);
    
    // Recovery should handle corruption gracefully
    auto result = pitr_mgr_->recoverToTimestamp(2500);
    
    // Should either skip corrupted event or fail with clear error
    if (!result.success) {
        EXPECT_FALSE(result.error_message.empty());
    }
}

// ============================================================================
// Timeline and Consistency Tests
// ============================================================================

TEST_F(PITRManagerComprehensiveTest, VerifyTimelineConsistency) {
    // Intent: Verify recovered state maintains referential integrity
    
    // Create related entities with foreign key relationships
    addEvent(Changefeed::ChangeEventType::EVENT_PUT, "departments:1", 
             R"({"name":"Engineering"})", 1000);
    addEvent(Changefeed::ChangeEventType::EVENT_PUT, "employees:1", 
             R"({"name":"Alice","dept_id":"1"})", 2000);
    addEvent(Changefeed::ChangeEventType::EVENT_PUT, "employees:2", 
             R"({"name":"Bob","dept_id":"1"})", 3000);
    
    // Recover to t=2500 (dept and one employee)
    auto result = pitr_mgr_->recoverToTimestamp(2500);
    ASSERT_TRUE(result.success);
    
    // Verify consistency: employee references existing department
    EXPECT_TRUE(db_wrapper_->get("departments:1").has_value());
    EXPECT_TRUE(db_wrapper_->get("employees:1").has_value());
    EXPECT_FALSE(db_wrapper_->get("employees:2").has_value());
}

TEST_F(PITRManagerComprehensiveTest, RecoverMultipleTimes) {
    // Intent: Verify PITR can recover to different points multiple times
    
    addEvent(Changefeed::ChangeEventType::EVENT_PUT, "multi:key", "v1", 1000);
    addEvent(Changefeed::ChangeEventType::EVENT_PUT, "multi:key", "v2", 2000);
    addEvent(Changefeed::ChangeEventType::EVENT_PUT, "multi:key", "v3", 3000);
    
    // First recovery to t=1500
    auto result1 = pitr_mgr_->recoverToTimestamp(1500);
    ASSERT_TRUE(result1.success);
    auto val1 = db_wrapper_->get("multi:key");
    ASSERT_TRUE(val1.has_value());
    std::string str1(val1->begin(), val1->end());
    EXPECT_EQ(str1, "v1");
    
    // Second recovery to t=2500
    auto result2 = pitr_mgr_->recoverToTimestamp(2500);
    ASSERT_TRUE(result2.success);
    auto val2 = db_wrapper_->get("multi:key");
    ASSERT_TRUE(val2.has_value());
    std::string str2(val2->begin(), val2->end());
    EXPECT_EQ(str2, "v2");
    
    // Third recovery to t=3500
    auto result3 = pitr_mgr_->recoverToTimestamp(3500);
    ASSERT_TRUE(result3.success);
    auto val3 = db_wrapper_->get("multi:key");
    ASSERT_TRUE(val3.has_value());
    std::string str3(val3->begin(), val3->end());
    EXPECT_EQ(str3, "v3");
}

// ============================================================================
// Performance Tests
// ============================================================================

TEST_F(PITRManagerComprehensiveTest, RecoveryPerformance) {
    // Intent: Verify PITR recovery completes in reasonable time
    
    const int num_events = 5000;
    for (int i = 0; i < num_events; ++i) {
        std::string key = "perf:key" + std::to_string(i);
        std::string value = "value" + std::to_string(i);
        addEvent(Changefeed::ChangeEventType::EVENT_PUT, key, value, 
                 1000 + i);
    }
    
    auto start = std::chrono::high_resolution_clock::now();
    auto result = pitr_mgr_->recoverToTimestamp(3500);
    auto end = std::chrono::high_resolution_clock::now();
    
    ASSERT_TRUE(result.success);
    
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    // Recovery should complete in reasonable time (< 5 seconds for 5000 events)
    EXPECT_LT(duration.count(), 5000) << "Recovery took " << duration.count() << "ms";
}
