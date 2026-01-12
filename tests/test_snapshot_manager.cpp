#include <gtest/gtest.h>
#include "transaction/snapshot_manager.h"
#include "storage/rocksdb_wrapper.h"
#include "cdc/changefeed.h"
#include "utils/logger.h"
#include <filesystem>

using namespace themis;

class SnapshotManagerTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create temporary database
        test_db_path_ = "./test_snapshot_db_" + std::to_string(::testing::UnitTest::GetInstance()->random_seed());
        std::filesystem::create_directories(test_db_path_);
        
        // Initialize RocksDB
        RocksDBWrapper::Config config;
        config.db_path = test_db_path_;
        config.enable_wal = false;  // Faster for tests
        config.enable_statistics = false;
        
        db_ = std::make_unique<RocksDBWrapper>(config);
        ASSERT_TRUE(db_->open().ok());
        
        // Initialize Changefeed
        changefeed_ = std::make_unique<Changefeed>(db_->getTransactionDB(), nullptr);
        
        // Initialize SnapshotManager
        snapshot_mgr_ = std::make_unique<SnapshotManager>(*db_, *changefeed_);
    }

    void TearDown() override {
        snapshot_mgr_.reset();
        changefeed_.reset();
        db_->close();
        db_.reset();
        
        // Clean up test database
        std::filesystem::remove_all(test_db_path_);
    }

    // Helper: Create some changefeed events to advance sequence
    void createChangefeedEvents(size_t count) {
        for (size_t i = 0; i < count; ++i) {
            Changefeed::ChangeEvent event;
            event.type = Changefeed::ChangeEventType::EVENT_PUT;
            event.key = "test:key_" + std::to_string(i);
            event.value = "value_" + std::to_string(i);
            event.timestamp_ms = 0;
            changefeed_->recordEvent(event);
        }
    }

    std::string test_db_path_;
    std::unique_ptr<RocksDBWrapper> db_;
    std::unique_ptr<Changefeed> changefeed_;
    std::unique_ptr<SnapshotManager> snapshot_mgr_;
};

// Test: Create and retrieve a snapshot tag
TEST_F(SnapshotManagerTest, CreateAndRetrieveTag) {
    // Create some events first
    createChangefeedEvents(10);
    
    auto status = snapshot_mgr_->createTag("test_tag", "Test snapshot", "test_user");
    ASSERT_TRUE(status.ok) << status.message;
    
    auto snapshot = snapshot_mgr_->getTag("test_tag");
    ASSERT_TRUE(snapshot.has_value());
    EXPECT_EQ(snapshot->tag_name, "test_tag");
    EXPECT_EQ(snapshot->description, "Test snapshot");
    EXPECT_EQ(snapshot->created_by, "test_user");
    EXPECT_EQ(snapshot->sequence_number, 10);
    EXPECT_GT(snapshot->timestamp_ms, 0);
}

// Test: Duplicate tag creation should fail
TEST_F(SnapshotManagerTest, DuplicateTagFails) {
    auto status1 = snapshot_mgr_->createTag("duplicate_tag", "First");
    ASSERT_TRUE(status1.ok);
    
    auto status2 = snapshot_mgr_->createTag("duplicate_tag", "Second");
    EXPECT_FALSE(status2.ok);
    EXPECT_NE(status2.message.find("already exists"), std::string::npos);
}

// Test: Invalid tag names should be rejected
TEST_F(SnapshotManagerTest, InvalidTagNames) {
    // Empty name
    auto status1 = snapshot_mgr_->createTag("", "Test");
    EXPECT_FALSE(status1.ok);
    
    // Name with spaces
    auto status2 = snapshot_mgr_->createTag("invalid tag", "Test");
    EXPECT_FALSE(status2.ok);
    
    // Name with special characters
    auto status3 = snapshot_mgr_->createTag("invalid@tag", "Test");
    EXPECT_FALSE(status3.ok);
    
    // Valid names should work
    auto status4 = snapshot_mgr_->createTag("valid-tag_123", "Test");
    EXPECT_TRUE(status4.ok);
}

// Test: Tag name length validation
TEST_F(SnapshotManagerTest, TagNameLengthValidation) {
    // Too long (> 128 chars)
    std::string long_name(200, 'a');
    auto status = snapshot_mgr_->createTag(long_name, "Test");
    EXPECT_FALSE(status.ok);
    EXPECT_NE(status.message.find("too long"), std::string::npos);
}

// Test: Description length validation
TEST_F(SnapshotManagerTest, DescriptionLengthValidation) {
    // Too long (> 1024 chars)
    std::string long_desc(2000, 'x');
    auto status = snapshot_mgr_->createTag("test_tag", long_desc);
    EXPECT_FALSE(status.ok);
    EXPECT_NE(status.message.find("too long"), std::string::npos);
}

// Test: List all tags
TEST_F(SnapshotManagerTest, ListTags) {
    // Create multiple tags
    snapshot_mgr_->createTag("tag1", "First tag");
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    snapshot_mgr_->createTag("tag2", "Second tag");
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    snapshot_mgr_->createTag("tag3", "Third tag");
    
    auto tags = snapshot_mgr_->listTags();
    ASSERT_EQ(tags.size(), 3);
    
    // Should be sorted by timestamp (newest first)
    EXPECT_EQ(tags[0].tag_name, "tag3");
    EXPECT_EQ(tags[1].tag_name, "tag2");
    EXPECT_EQ(tags[2].tag_name, "tag1");
}

// Test: List tags when empty
TEST_F(SnapshotManagerTest, ListTagsEmpty) {
    auto tags = snapshot_mgr_->listTags();
    EXPECT_TRUE(tags.empty());
}

// Test: Delete a tag
TEST_F(SnapshotManagerTest, DeleteTag) {
    snapshot_mgr_->createTag("to_delete", "Will be deleted");
    ASSERT_TRUE(snapshot_mgr_->tagExists("to_delete"));
    
    auto status = snapshot_mgr_->deleteTag("to_delete");
    ASSERT_TRUE(status.ok) << status.message;
    
    EXPECT_FALSE(snapshot_mgr_->tagExists("to_delete"));
}

// Test: Delete non-existent tag should fail
TEST_F(SnapshotManagerTest, DeleteNonExistentTag) {
    auto status = snapshot_mgr_->deleteTag("nonexistent");
    EXPECT_FALSE(status.ok);
    EXPECT_NE(status.message.find("does not exist"), std::string::npos);
}

// Test: Tag existence check
TEST_F(SnapshotManagerTest, TagExists) {
    EXPECT_FALSE(snapshot_mgr_->tagExists("nonexistent"));
    
    snapshot_mgr_->createTag("exists", "Test");
    EXPECT_TRUE(snapshot_mgr_->tagExists("exists"));
}

// Test: Get statistics
TEST_F(SnapshotManagerTest, GetStats) {
    // Empty stats
    auto stats1 = snapshot_mgr_->getStats();
    EXPECT_EQ(stats1.total_tags, 0);
    EXPECT_EQ(stats1.oldest_sequence, 0);
    EXPECT_EQ(stats1.newest_sequence, 0);
    
    // Create some tags at different sequences
    createChangefeedEvents(10);
    snapshot_mgr_->createTag("tag1", "First");
    
    createChangefeedEvents(10);
    snapshot_mgr_->createTag("tag2", "Second");
    
    createChangefeedEvents(10);
    snapshot_mgr_->createTag("tag3", "Third");
    
    auto stats2 = snapshot_mgr_->getStats();
    EXPECT_EQ(stats2.total_tags, 3);
    EXPECT_EQ(stats2.oldest_sequence, 10);
    EXPECT_EQ(stats2.newest_sequence, 30);
    EXPECT_GT(stats2.newest_timestamp_ms, stats2.oldest_timestamp_ms);
}

// Test: Get sequence for tag
TEST_F(SnapshotManagerTest, GetSequenceForTag) {
    createChangefeedEvents(5);
    snapshot_mgr_->createTag("tag_at_5", "At sequence 5");
    
    auto seq = snapshot_mgr_->getSequenceForTag("tag_at_5");
    ASSERT_TRUE(seq.has_value());
    EXPECT_EQ(*seq, 5);
    
    // Non-existent tag
    auto seq2 = snapshot_mgr_->getSequenceForTag("nonexistent");
    EXPECT_FALSE(seq2.has_value());
}

// Test: Snapshot JSON serialization
TEST_F(SnapshotManagerTest, SnapshotSerialization) {
    SnapshotManager::Snapshot original;
    original.tag_name = "test";
    original.sequence_number = 123;
    original.timestamp_ms = 1234567890123;
    original.description = "Test description";
    original.created_by = "user";
    
    // Serialize to JSON
    auto json = original.toJson();
    EXPECT_EQ(json["tag_name"], "test");
    EXPECT_EQ(json["sequence_number"], 123);
    EXPECT_EQ(json["timestamp_ms"], 1234567890123);
    EXPECT_EQ(json["description"], "Test description");
    EXPECT_EQ(json["created_by"], "user");
    
    // Deserialize from JSON
    auto restored = SnapshotManager::Snapshot::fromJson(json);
    EXPECT_EQ(restored.tag_name, original.tag_name);
    EXPECT_EQ(restored.sequence_number, original.sequence_number);
    EXPECT_EQ(restored.timestamp_ms, original.timestamp_ms);
    EXPECT_EQ(restored.description, original.description);
    EXPECT_EQ(restored.created_by, original.created_by);
}

// Test: Concurrent tag creation (thread safety)
TEST_F(SnapshotManagerTest, ConcurrentTagCreation) {
    const int num_threads = 10;
    std::vector<std::thread> threads;
    std::atomic<int> success_count{0};
    
    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back([this, i, &success_count]() {
            auto status = snapshot_mgr_->createTag(
                "concurrent_tag_" + std::to_string(i),
                "Created by thread " + std::to_string(i)
            );
            if (status.ok) {
                success_count++;
            }
        });
    }
    
    for (auto& thread : threads) {
        thread.join();
    }
    
    EXPECT_EQ(success_count, num_threads);
    
    auto tags = snapshot_mgr_->listTags();
    EXPECT_EQ(tags.size(), num_threads);
}

// Test: Default values
TEST_F(SnapshotManagerTest, DefaultValues) {
    auto status = snapshot_mgr_->createTag("default_test");
    ASSERT_TRUE(status.ok);
    
    auto snapshot = snapshot_mgr_->getTag("default_test");
    ASSERT_TRUE(snapshot.has_value());
    EXPECT_EQ(snapshot->description, "");
    EXPECT_EQ(snapshot->created_by, "system");
}

// Test: Tag name validation function
TEST_F(SnapshotManagerTest, ValidateTagNameFunction) {
    EXPECT_TRUE(SnapshotManager::validateTagName("valid_tag").ok);
    EXPECT_TRUE(SnapshotManager::validateTagName("valid-tag").ok);
    EXPECT_TRUE(SnapshotManager::validateTagName("valid123").ok);
    EXPECT_TRUE(SnapshotManager::validateTagName("Valid_Tag-123").ok);
    
    EXPECT_FALSE(SnapshotManager::validateTagName("").ok);
    EXPECT_FALSE(SnapshotManager::validateTagName("invalid tag").ok);
    EXPECT_FALSE(SnapshotManager::validateTagName("invalid@tag").ok);
    EXPECT_FALSE(SnapshotManager::validateTagName("invalid.tag").ok);
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    Logger::instance().initialize();
    return RUN_ALL_TESTS();
}
