#include <gtest/gtest.h>
#include "transaction/snapshot_manager.h"
#include "cdc/changefeed.h"
#include "storage/rocksdb_wrapper.h"
#include <filesystem>
#include <rocksdb/db.h>
#include <rocksdb/utilities/transaction_db.h>

using namespace themis;

class SnapshotManagerTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create temp directory for test database
        test_dir_ = std::filesystem::temp_directory_path() / "snapshot_manager_test";
        std::filesystem::remove_all(test_dir_);
        std::filesystem::create_directories(test_dir_);

        // Initialize RocksDB
        RocksDBWrapper::Config config;
        config.path = test_dir_.string();
        config.enable_statistics = false;
        
        db_wrapper_ = std::make_unique<RocksDBWrapper>(config);
        ASSERT_TRUE(db_wrapper_->open().ok);

        // Create "tags" column family
        auto* raw_db = db_wrapper_->getRawDB();
        rocksdb::ColumnFamilyHandle* cf_handle = nullptr;
        rocksdb::ColumnFamilyOptions cf_opts;
        auto s = raw_db->CreateColumnFamily(cf_opts, "tags", &cf_handle);
        ASSERT_TRUE(s.ok()) << s.ToString();
        tags_cf_ = cf_handle;

        // Initialize Changefeed
        changefeed_ = std::make_unique<Changefeed>(raw_db, nullptr);

        // Initialize SnapshotManager
        snapshot_mgr_ = std::make_unique<SnapshotManager>(raw_db, tags_cf_, changefeed_.get());
    }

    void TearDown() override {
        snapshot_mgr_.reset();
        changefeed_.reset();
        
        if (tags_cf_) {
            delete tags_cf_;
            tags_cf_ = nullptr;
        }
        
        db_wrapper_.reset();
        std::filesystem::remove_all(test_dir_);
    }

    std::filesystem::path test_dir_;
    std::unique_ptr<RocksDBWrapper> db_wrapper_;
    rocksdb::ColumnFamilyHandle* tags_cf_ = nullptr;
    std::unique_ptr<Changefeed> changefeed_;
    std::unique_ptr<SnapshotManager> snapshot_mgr_;
};

// Test: Create a valid tag
TEST_F(SnapshotManagerTest, CreateValidTag) {
    auto status = snapshot_mgr_->createTag("test_snapshot", "Test snapshot description");
    ASSERT_TRUE(status.ok) << status.message;
}

// Test: Create tag with invalid name (uppercase)
TEST_F(SnapshotManagerTest, CreateTagInvalidName_Uppercase) {
    auto status = snapshot_mgr_->createTag("TestSnapshot", "Description");
    ASSERT_FALSE(status.ok);
    EXPECT_THAT(status.message, ::testing::HasSubstr("Invalid tag name"));
}

// Test: Create tag with invalid name (starts with digit)
TEST_F(SnapshotManagerTest, CreateTagInvalidName_StartsWithDigit) {
    auto status = snapshot_mgr_->createTag("1test", "Description");
    ASSERT_FALSE(status.ok);
    EXPECT_THAT(status.message, ::testing::HasSubstr("Invalid tag name"));
}

// Test: Create tag with invalid name (special characters)
TEST_F(SnapshotManagerTest, CreateTagInvalidName_SpecialChars) {
    auto status = snapshot_mgr_->createTag("test@snapshot", "Description");
    ASSERT_FALSE(status.ok);
    EXPECT_THAT(status.message, ::testing::HasSubstr("Invalid tag name"));
}

// Test: Create tag with empty name
TEST_F(SnapshotManagerTest, CreateTagInvalidName_Empty) {
    auto status = snapshot_mgr_->createTag("", "Description");
    ASSERT_FALSE(status.ok);
    EXPECT_THAT(status.message, ::testing::HasSubstr("Invalid tag name"));
}

// Test: Create tag with too long name
TEST_F(SnapshotManagerTest, CreateTagInvalidName_TooLong) {
    std::string long_name(100, 'a'); // 100 characters
    auto status = snapshot_mgr_->createTag(long_name, "Description");
    ASSERT_FALSE(status.ok);
    EXPECT_THAT(status.message, ::testing::HasSubstr("Invalid tag name"));
}

// Test: Create tag with valid name containing hyphens and underscores
TEST_F(SnapshotManagerTest, CreateTagValidName_WithHyphensUnderscores) {
    auto status = snapshot_mgr_->createTag("test-snapshot_v1", "Description");
    ASSERT_TRUE(status.ok) << status.message;
}

// Test: Create tag with too long description
TEST_F(SnapshotManagerTest, CreateTagInvalidDescription_TooLong) {
    std::string long_desc(600, 'x'); // 600 characters
    auto status = snapshot_mgr_->createTag("test", long_desc);
    ASSERT_FALSE(status.ok);
    EXPECT_THAT(status.message, ::testing::HasSubstr("Invalid description"));
}

// Test: Create duplicate tag
TEST_F(SnapshotManagerTest, CreateDuplicateTag) {
    auto status1 = snapshot_mgr_->createTag("duplicate", "First");
    ASSERT_TRUE(status1.ok) << status1.message;

    auto status2 = snapshot_mgr_->createTag("duplicate", "Second");
    ASSERT_FALSE(status2.ok);
    EXPECT_THAT(status2.message, ::testing::HasSubstr("already exists"));
}

// Test: Get existing tag
TEST_F(SnapshotManagerTest, GetExistingTag) {
    auto status = snapshot_mgr_->createTag("my_tag", "My description", "testuser");
    ASSERT_TRUE(status.ok) << status.message;

    auto snapshot = snapshot_mgr_->getTag("my_tag");
    ASSERT_TRUE(snapshot.has_value());
    EXPECT_EQ(snapshot->tag_name, "my_tag");
    EXPECT_EQ(snapshot->description, "My description");
    EXPECT_EQ(snapshot->created_by, "testuser");
    EXPECT_GT(snapshot->timestamp_ms, 0);
}

// Test: Get non-existent tag
TEST_F(SnapshotManagerTest, GetNonExistentTag) {
    auto snapshot = snapshot_mgr_->getTag("nonexistent");
    ASSERT_FALSE(snapshot.has_value());
}

// Test: List empty snapshots
TEST_F(SnapshotManagerTest, ListEmptySnapshots) {
    auto snapshots = snapshot_mgr_->listTags();
    EXPECT_EQ(snapshots.size(), 0);
}

// Test: List multiple snapshots sorted by time
TEST_F(SnapshotManagerTest, ListSnapshotsSortedByTime) {
    // Create snapshots with small delays
    snapshot_mgr_->createTag("first", "First snapshot");
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    
    snapshot_mgr_->createTag("second", "Second snapshot");
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    
    snapshot_mgr_->createTag("third", "Third snapshot");

    auto snapshots = snapshot_mgr_->listTags(true); // Sort by time
    ASSERT_EQ(snapshots.size(), 3);
    
    // Newest first
    EXPECT_EQ(snapshots[0].tag_name, "third");
    EXPECT_EQ(snapshots[1].tag_name, "second");
    EXPECT_EQ(snapshots[2].tag_name, "first");
}

// Test: List snapshots sorted by name
TEST_F(SnapshotManagerTest, ListSnapshotsSortedByName) {
    snapshot_mgr_->createTag("zebra", "Zebra");
    snapshot_mgr_->createTag("alpha", "Alpha");
    snapshot_mgr_->createTag("beta", "Beta");

    auto snapshots = snapshot_mgr_->listTags(false); // Sort by name
    ASSERT_EQ(snapshots.size(), 3);
    
    EXPECT_EQ(snapshots[0].tag_name, "alpha");
    EXPECT_EQ(snapshots[1].tag_name, "beta");
    EXPECT_EQ(snapshots[2].tag_name, "zebra");
}

// Test: Delete existing tag
TEST_F(SnapshotManagerTest, DeleteExistingTag) {
    snapshot_mgr_->createTag("to_delete", "Will be deleted");
    
    auto snapshot = snapshot_mgr_->getTag("to_delete");
    ASSERT_TRUE(snapshot.has_value());

    auto status = snapshot_mgr_->deleteTag("to_delete");
    ASSERT_TRUE(status.ok) << status.message;

    snapshot = snapshot_mgr_->getTag("to_delete");
    EXPECT_FALSE(snapshot.has_value());
}

// Test: Delete non-existent tag
TEST_F(SnapshotManagerTest, DeleteNonExistentTag) {
    auto status = snapshot_mgr_->deleteTag("nonexistent");
    ASSERT_FALSE(status.ok);
    EXPECT_THAT(status.message, ::testing::HasSubstr("not found"));
}

// Test: Get statistics with no snapshots
TEST_F(SnapshotManagerTest, GetStatsEmpty) {
    auto stats = snapshot_mgr_->getStats();
    EXPECT_EQ(stats.total_snapshots, 0);
    EXPECT_EQ(stats.total_size_bytes, 0);
    EXPECT_EQ(stats.oldest_timestamp_ms, 0);
    EXPECT_EQ(stats.newest_timestamp_ms, 0);
}

// Test: Get statistics with multiple snapshots
TEST_F(SnapshotManagerTest, GetStatsWithSnapshots) {
    snapshot_mgr_->createTag("tag1", "Description 1");
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    snapshot_mgr_->createTag("tag2", "Description 2");
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    snapshot_mgr_->createTag("tag3", "Description 3");

    auto stats = snapshot_mgr_->getStats();
    EXPECT_EQ(stats.total_snapshots, 3);
    EXPECT_GT(stats.total_size_bytes, 0);
    EXPECT_GT(stats.oldest_timestamp_ms, 0);
    EXPECT_GT(stats.newest_timestamp_ms, 0);
    EXPECT_LE(stats.oldest_timestamp_ms, stats.newest_timestamp_ms);
}

// Test: Snapshot persistence across restarts
TEST_F(SnapshotManagerTest, SnapshotPersistence) {
    // Create a snapshot
    snapshot_mgr_->createTag("persistent", "Persistent snapshot", "testuser");
    
    // Get and verify
    auto snapshot1 = snapshot_mgr_->getTag("persistent");
    ASSERT_TRUE(snapshot1.has_value());
    uint64_t seq1 = snapshot1->sequence_number;
    int64_t ts1 = snapshot1->timestamp_ms;

    // Simulate restart by recreating SnapshotManager
    snapshot_mgr_.reset();
    snapshot_mgr_ = std::make_unique<SnapshotManager>(
        db_wrapper_->getRawDB(), tags_cf_, changefeed_.get()
    );

    // Verify snapshot still exists with same data
    auto snapshot2 = snapshot_mgr_->getTag("persistent");
    ASSERT_TRUE(snapshot2.has_value());
    EXPECT_EQ(snapshot2->tag_name, "persistent");
    EXPECT_EQ(snapshot2->description, "Persistent snapshot");
    EXPECT_EQ(snapshot2->created_by, "testuser");
    EXPECT_EQ(snapshot2->sequence_number, seq1);
    EXPECT_EQ(snapshot2->timestamp_ms, ts1);
}

// Test: Concurrent tag creation (race condition check)
TEST_F(SnapshotManagerTest, ConcurrentTagCreation) {
    const int num_threads = 10;
    std::vector<std::thread> threads;
    std::atomic<int> success_count{0};

    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back([this, i, &success_count]() {
            std::string tag_name = "concurrent_" + std::to_string(i);
            auto status = snapshot_mgr_->createTag(tag_name, "Concurrent test");
            if (status.ok) {
                success_count++;
            }
        });
    }

    for (auto& thread : threads) {
        thread.join();
    }

    EXPECT_EQ(success_count, num_threads);
    
    auto snapshots = snapshot_mgr_->listTags();
    EXPECT_EQ(snapshots.size(), num_threads);
}

// Test: Tag name validation static method
TEST_F(SnapshotManagerTest, IsValidTagName) {
    EXPECT_TRUE(SnapshotManager::isValidTagName("valid"));
    EXPECT_TRUE(SnapshotManager::isValidTagName("valid-tag"));
    EXPECT_TRUE(SnapshotManager::isValidTagName("valid_tag"));
    EXPECT_TRUE(SnapshotManager::isValidTagName("valid123"));
    EXPECT_TRUE(SnapshotManager::isValidTagName("v"));
    
    EXPECT_FALSE(SnapshotManager::isValidTagName(""));
    EXPECT_FALSE(SnapshotManager::isValidTagName("Invalid")); // uppercase
    EXPECT_FALSE(SnapshotManager::isValidTagName("1invalid")); // starts with digit
    EXPECT_FALSE(SnapshotManager::isValidTagName("invalid tag")); // space
    EXPECT_FALSE(SnapshotManager::isValidTagName("invalid@tag")); // special char
    EXPECT_FALSE(SnapshotManager::isValidTagName(std::string(100, 'a'))); // too long
}

// Test: Description validation static method
TEST_F(SnapshotManagerTest, IsValidDescription) {
    EXPECT_TRUE(SnapshotManager::isValidDescription(""));
    EXPECT_TRUE(SnapshotManager::isValidDescription("Short description"));
    EXPECT_TRUE(SnapshotManager::isValidDescription(std::string(500, 'x'))); // exactly 500
    
    EXPECT_FALSE(SnapshotManager::isValidDescription(std::string(501, 'x'))); // 501 chars
}

// Test: Snapshot captures current sequence number
TEST_F(SnapshotManagerTest, SnapshotCapturesSequence) {
    // Record some changefeed events to advance sequence
    Changefeed::ChangeEvent event;
    event.type = Changefeed::ChangeEventType::EVENT_PUT;
    event.key = "test:key1";
    event.value = "value1";
    event.timestamp_ms = 1000;
    
    changefeed_->recordEvent(event);
    changefeed_->recordEvent(event);
    changefeed_->recordEvent(event);
    
    uint64_t current_seq = changefeed_->getLatestSequence();
    EXPECT_GT(current_seq, 0);

    // Create snapshot
    snapshot_mgr_->createTag("with_sequence", "Snapshot with sequence");
    
    // Verify snapshot captured current sequence
    auto snapshot = snapshot_mgr_->getTag("with_sequence");
    ASSERT_TRUE(snapshot.has_value());
    EXPECT_EQ(snapshot->sequence_number, current_seq);
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
