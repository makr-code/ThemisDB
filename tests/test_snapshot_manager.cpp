#include <gtest/gtest.h>
#include "transaction/snapshot_manager.h"
#include "cdc/changefeed.h"
#include "storage/rocksdb_wrapper.h"
#include <filesystem>
#include <chrono>
#include <thread>

using namespace themis;
using namespace themis::transaction;

class SnapshotManagerTest : public ::testing::Test {
protected:
    void SetUp() override {
        test_db_path_ = "./data/themis_snapshot_manager_test";
        if (std::filesystem::exists(test_db_path_)) {
            std::filesystem::remove_all(test_db_path_);
        }
        
        RocksDBWrapper::Config config;
        config.db_path = test_db_path_;
        config.enable_wal = true;
        db_ = std::make_unique<RocksDBWrapper>(config);
        ASSERT_TRUE(db_->open());

        auto* txn_db = db_->getRawDB();
        ASSERT_NE(txn_db, nullptr);

        changefeed_ = std::make_unique<Changefeed>(txn_db);
        snapshot_manager_ = std::make_unique<SnapshotManager>(*db_, *changefeed_);
    }
    
    void TearDown() override {
        snapshot_manager_.reset();
        changefeed_.reset();
        db_.reset();
        
        if (std::filesystem::exists(test_db_path_)) {
            std::filesystem::remove_all(test_db_path_);
        }
    }
    
    // Helper: Record some events in changefeed
    void recordEvents(int count) {
        for (int i = 0; i < count; ++i) {
            Changefeed::ChangeEvent event;
            event.type = Changefeed::ChangeEventType::EVENT_PUT;
            event.key = "test:key" + std::to_string(i);
            event.value = "value" + std::to_string(i);
            auto now = std::chrono::system_clock::now();
            event.timestamp_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                now.time_since_epoch()).count();
            changefeed_->recordEvent(event);
        }
    }
    
    std::string test_db_path_;
    std::unique_ptr<RocksDBWrapper> db_;
    std::unique_ptr<Changefeed> changefeed_;
    std::unique_ptr<SnapshotManager> snapshot_manager_;
};

// Test 1: Create a tag
TEST_F(SnapshotManagerTest, CreateTag) {
    recordEvents(10);
    
    auto snapshot = snapshot_manager_->createTag("v1.0.0", "Release 1.0", "admin");
    
    ASSERT_TRUE(snapshot.has_value());
    EXPECT_EQ(snapshot->tag_name, "v1.0.0");
    EXPECT_EQ(snapshot->description, "Release 1.0");
    EXPECT_EQ(snapshot->created_by, "admin");
    EXPECT_GT(snapshot->sequence_number, 0);
    EXPECT_GT(snapshot->timestamp_ms, 0);
}

// Test 2: Create duplicate tag fails
TEST_F(SnapshotManagerTest, CreateDuplicateTagFails) {
    auto snapshot1 = snapshot_manager_->createTag("v1.0.0", "First", "admin");
    ASSERT_TRUE(snapshot1.has_value());
    
    auto snapshot2 = snapshot_manager_->createTag("v1.0.0", "Duplicate", "admin");
    EXPECT_FALSE(snapshot2.has_value());
}

// Test 3: Get tag
TEST_F(SnapshotManagerTest, GetTag) {
    auto created = snapshot_manager_->createTag("v1.0.0", "Release 1.0", "admin");
    ASSERT_TRUE(created.has_value());
    
    auto retrieved = snapshot_manager_->getTag("v1.0.0");
    ASSERT_TRUE(retrieved.has_value());
    EXPECT_EQ(retrieved->tag_name, "v1.0.0");
    EXPECT_EQ(retrieved->description, "Release 1.0");
    EXPECT_EQ(retrieved->created_by, "admin");
    EXPECT_EQ(retrieved->sequence_number, created->sequence_number);
}

// Test 4: Get non-existent tag
TEST_F(SnapshotManagerTest, GetNonExistentTag) {
    auto retrieved = snapshot_manager_->getTag("nonexistent");
    EXPECT_FALSE(retrieved.has_value());
}

// Test 5: List tags
TEST_F(SnapshotManagerTest, ListTags) {
    snapshot_manager_->createTag("v1.0.0", "Release 1.0", "admin");
    recordEvents(5);
    snapshot_manager_->createTag("v1.1.0", "Release 1.1", "admin");
    recordEvents(5);
    snapshot_manager_->createTag("v2.0.0", "Release 2.0", "admin");
    
    auto tags = snapshot_manager_->listTags();
    
    EXPECT_EQ(tags.size(), 3);
}

// Test 6: List tags with limit
TEST_F(SnapshotManagerTest, ListTagsWithLimit) {
    snapshot_manager_->createTag("v1.0.0", "Release 1.0", "admin");
    snapshot_manager_->createTag("v1.1.0", "Release 1.1", "admin");
    snapshot_manager_->createTag("v2.0.0", "Release 2.0", "admin");
    
    auto tags = snapshot_manager_->listTags(2);
    
    EXPECT_EQ(tags.size(), 2);
}

// Test 7: List tags sorted by timestamp (descending)
TEST_F(SnapshotManagerTest, ListTagsSortedByTimestamp) {
    auto tag1 = snapshot_manager_->createTag("v1.0.0", "Release 1.0", "admin");
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    auto tag2 = snapshot_manager_->createTag("v1.1.0", "Release 1.1", "admin");
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    auto tag3 = snapshot_manager_->createTag("v2.0.0", "Release 2.0", "admin");
    
    auto tags = snapshot_manager_->listTags(0, "timestamp", false);
    
    EXPECT_EQ(tags.size(), 3);
    EXPECT_EQ(tags[0].tag_name, "v2.0.0");
    EXPECT_EQ(tags[1].tag_name, "v1.1.0");
    EXPECT_EQ(tags[2].tag_name, "v1.0.0");
}

// Test 8: List tags sorted by timestamp (ascending)
TEST_F(SnapshotManagerTest, ListTagsSortedByTimestampAscending) {
    auto tag1 = snapshot_manager_->createTag("v1.0.0", "Release 1.0", "admin");
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    auto tag2 = snapshot_manager_->createTag("v1.1.0", "Release 1.1", "admin");
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    auto tag3 = snapshot_manager_->createTag("v2.0.0", "Release 2.0", "admin");
    
    auto tags = snapshot_manager_->listTags(0, "timestamp", true);
    
    EXPECT_EQ(tags.size(), 3);
    EXPECT_EQ(tags[0].tag_name, "v1.0.0");
    EXPECT_EQ(tags[1].tag_name, "v1.1.0");
    EXPECT_EQ(tags[2].tag_name, "v2.0.0");
}

// Test 9: List tags sorted by name
TEST_F(SnapshotManagerTest, ListTagsSortedByName) {
    snapshot_manager_->createTag("v2.0.0", "Release 2.0", "admin");
    snapshot_manager_->createTag("v1.0.0", "Release 1.0", "admin");
    snapshot_manager_->createTag("v1.1.0", "Release 1.1", "admin");
    
    auto tags = snapshot_manager_->listTags(0, "name", true);
    
    EXPECT_EQ(tags.size(), 3);
    EXPECT_EQ(tags[0].tag_name, "v1.0.0");
    EXPECT_EQ(tags[1].tag_name, "v1.1.0");
    EXPECT_EQ(tags[2].tag_name, "v2.0.0");
}

// Test 10: List tags sorted by sequence
TEST_F(SnapshotManagerTest, ListTagsSortedBySequence) {
    auto tag1 = snapshot_manager_->createTag("tag_a", "First", "admin");
    recordEvents(10);
    auto tag2 = snapshot_manager_->createTag("tag_b", "Second", "admin");
    recordEvents(5);
    auto tag3 = snapshot_manager_->createTag("tag_c", "Third", "admin");
    
    auto tags = snapshot_manager_->listTags(0, "sequence", true);
    
    EXPECT_EQ(tags.size(), 3);
    EXPECT_EQ(tags[0].tag_name, "tag_a");
    EXPECT_EQ(tags[1].tag_name, "tag_b");
    EXPECT_EQ(tags[2].tag_name, "tag_c");
}

// Test 11: Delete tag
TEST_F(SnapshotManagerTest, DeleteTag) {
    snapshot_manager_->createTag("v1.0.0", "Release 1.0", "admin");
    
    EXPECT_TRUE(snapshot_manager_->tagExists("v1.0.0"));
    
    bool deleted = snapshot_manager_->deleteTag("v1.0.0");
    EXPECT_TRUE(deleted);
    
    EXPECT_FALSE(snapshot_manager_->tagExists("v1.0.0"));
}

// Test 12: Delete non-existent tag
TEST_F(SnapshotManagerTest, DeleteNonExistentTag) {
    bool deleted = snapshot_manager_->deleteTag("nonexistent");
    EXPECT_FALSE(deleted);
}

// Test 13: Tag exists
TEST_F(SnapshotManagerTest, TagExists) {
    EXPECT_FALSE(snapshot_manager_->tagExists("v1.0.0"));
    
    snapshot_manager_->createTag("v1.0.0", "Release 1.0", "admin");
    
    EXPECT_TRUE(snapshot_manager_->tagExists("v1.0.0"));
}

// Test 14: Get statistics
TEST_F(SnapshotManagerTest, GetStatistics) {
    recordEvents(5);
    snapshot_manager_->createTag("v1.0.0", "Release 1.0", "admin");
    recordEvents(10);
    snapshot_manager_->createTag("v2.0.0", "Release 2.0", "admin");
    recordEvents(5);
    snapshot_manager_->createTag("v3.0.0", "Release 3.0", "admin");
    
    auto stats = snapshot_manager_->getStats();
    
    EXPECT_EQ(stats.total_snapshots, 3);
    EXPECT_GT(stats.oldest_timestamp_ms, 0);
    EXPECT_GT(stats.newest_timestamp_ms, 0);
    EXPECT_GE(stats.newest_timestamp_ms, stats.oldest_timestamp_ms);
    EXPECT_GT(stats.oldest_sequence, 0);
    EXPECT_GT(stats.newest_sequence, 0);
    EXPECT_GE(stats.newest_sequence, stats.oldest_sequence);
}

// Test 15: Get sequence for tag
TEST_F(SnapshotManagerTest, GetSequenceForTag) {
    recordEvents(10);
    auto created = snapshot_manager_->createTag("v1.0.0", "Release 1.0", "admin");
    ASSERT_TRUE(created.has_value());
    
    auto sequence = snapshot_manager_->getSequenceForTag("v1.0.0");
    ASSERT_TRUE(sequence.has_value());
    EXPECT_EQ(*sequence, created->sequence_number);
}

// Test 16: Get sequence for non-existent tag
TEST_F(SnapshotManagerTest, GetSequenceForNonExistentTag) {
    auto sequence = snapshot_manager_->getSequenceForTag("nonexistent");
    EXPECT_FALSE(sequence.has_value());
}

// Test 17: Get timestamp for tag
TEST_F(SnapshotManagerTest, GetTimestampForTag) {
    auto created = snapshot_manager_->createTag("v1.0.0", "Release 1.0", "admin");
    ASSERT_TRUE(created.has_value());
    
    auto timestamp = snapshot_manager_->getTimestampForTag("v1.0.0");
    ASSERT_TRUE(timestamp.has_value());
    EXPECT_EQ(*timestamp, created->timestamp_ms);
}

// Test 18: Get timestamp for non-existent tag
TEST_F(SnapshotManagerTest, GetTimestampForNonExistentTag) {
    auto timestamp = snapshot_manager_->getTimestampForTag("nonexistent");
    EXPECT_FALSE(timestamp.has_value());
}

// Test 19: Validate tag name - valid names
TEST_F(SnapshotManagerTest, ValidateTagNameValid) {
    EXPECT_TRUE(SnapshotManager::isValidTagName("v1.0.0"));
    EXPECT_TRUE(SnapshotManager::isValidTagName("release-1.0"));
    EXPECT_TRUE(SnapshotManager::isValidTagName("my_tag"));
    EXPECT_TRUE(SnapshotManager::isValidTagName("tag.with.dots"));
    EXPECT_TRUE(SnapshotManager::isValidTagName("Tag123"));
}

// Test 20: Validate tag name - invalid names
TEST_F(SnapshotManagerTest, ValidateTagNameInvalid) {
    EXPECT_FALSE(SnapshotManager::isValidTagName(""));
    EXPECT_FALSE(SnapshotManager::isValidTagName("tag with spaces"));
    EXPECT_FALSE(SnapshotManager::isValidTagName("tag/with/slash"));
    EXPECT_FALSE(SnapshotManager::isValidTagName("tag@special"));
    EXPECT_FALSE(SnapshotManager::isValidTagName(std::string(129, 'a'))); // Too long
}

// Test 21: Invalid tag name fails creation
TEST_F(SnapshotManagerTest, InvalidTagNameFailsCreation) {
    auto snapshot = snapshot_manager_->createTag("invalid tag name", "Description", "admin");
    EXPECT_FALSE(snapshot.has_value());
}

// Test 22: JSON serialization
TEST_F(SnapshotManagerTest, JsonSerialization) {
    auto created = snapshot_manager_->createTag("v1.0.0", "Release 1.0", "admin");
    ASSERT_TRUE(created.has_value());
    
    auto json = created->toJson();
    
    EXPECT_TRUE(json.contains("tag_name"));
    EXPECT_TRUE(json.contains("sequence_number"));
    EXPECT_TRUE(json.contains("timestamp_ms"));
    EXPECT_TRUE(json.contains("description"));
    EXPECT_TRUE(json.contains("created_by"));
    
    EXPECT_EQ(json["tag_name"], "v1.0.0");
    EXPECT_EQ(json["description"], "Release 1.0");
    EXPECT_EQ(json["created_by"], "admin");
}

// Test 23: JSON deserialization
TEST_F(SnapshotManagerTest, JsonDeserialization) {
    json j;
    j["tag_name"] = "v1.0.0";
    j["sequence_number"] = 100;
    j["timestamp_ms"] = 1234567890;
    j["description"] = "Test";
    j["created_by"] = "admin";
    
    auto snapshot = SnapshotManager::Snapshot::fromJson(j);
    
    EXPECT_EQ(snapshot.tag_name, "v1.0.0");
    EXPECT_EQ(snapshot.sequence_number, 100);
    EXPECT_EQ(snapshot.timestamp_ms, 1234567890);
    EXPECT_EQ(snapshot.description, "Test");
    EXPECT_EQ(snapshot.created_by, "admin");
}

// Test 24: Persistence - tags survive restart
TEST_F(SnapshotManagerTest, PersistenceAcrossRestarts) {
    snapshot_manager_->createTag("v1.0.0", "Release 1.0", "admin");
    snapshot_manager_->createTag("v2.0.0", "Release 2.0", "admin");
    
    // Destroy and recreate snapshot manager
    snapshot_manager_.reset();
    snapshot_manager_ = std::make_unique<SnapshotManager>(*db_, *changefeed_);
    
    auto tags = snapshot_manager_->listTags();
    EXPECT_EQ(tags.size(), 2);
    
    auto tag1 = snapshot_manager_->getTag("v1.0.0");
    ASSERT_TRUE(tag1.has_value());
    EXPECT_EQ(tag1->description, "Release 1.0");
    
    auto tag2 = snapshot_manager_->getTag("v2.0.0");
    ASSERT_TRUE(tag2.has_value());
    EXPECT_EQ(tag2->description, "Release 2.0");
}

// Test 25: Empty list when no tags
TEST_F(SnapshotManagerTest, EmptyListWhenNoTags) {
    auto tags = snapshot_manager_->listTags();
    EXPECT_EQ(tags.size(), 0);
}

// Test 26: Statistics when no tags
TEST_F(SnapshotManagerTest, StatisticsWhenNoTags) {
    auto stats = snapshot_manager_->getStats();
    
    EXPECT_EQ(stats.total_snapshots, 0);
    EXPECT_EQ(stats.oldest_timestamp_ms, 0);
    EXPECT_EQ(stats.newest_timestamp_ms, 0);
    EXPECT_EQ(stats.oldest_sequence, 0);
    EXPECT_EQ(stats.newest_sequence, 0);
}

// Test 27: Default created_by value
TEST_F(SnapshotManagerTest, DefaultCreatedByValue) {
    auto snapshot = snapshot_manager_->createTag("v1.0.0", "Release 1.0");
    ASSERT_TRUE(snapshot.has_value());
    EXPECT_EQ(snapshot->created_by, "system");
}

// Tests for Phase 7 additions: GC / Retention Policy & Snapshot Restore

// Test 28: Retention policy prunes old snapshots
TEST_F(SnapshotManagerTest, RetentionPolicyMaxSnapshots) {
    snapshot_manager_->createTag("old1", "Old 1", "admin");
    std::this_thread::sleep_for(std::chrono::milliseconds(5));
    snapshot_manager_->createTag("old2", "Old 2", "admin");
    std::this_thread::sleep_for(std::chrono::milliseconds(5));
    snapshot_manager_->createTag("keep", "Newest", "admin");

    SnapshotManager::RetentionPolicy pol;
    pol.max_snapshots = 1;
    pol.protect_latest = true;
    snapshot_manager_->setRetentionPolicy(pol);

    size_t pruned = snapshot_manager_->pruneOldSnapshots();

    EXPECT_EQ(pruned, 2u);
    // The newest must survive
    EXPECT_TRUE(snapshot_manager_->tagExists("keep"));
}

// Test 29: Consistency check returns 0 on healthy data
TEST_F(SnapshotManagerTest, ConsistencyCheckHealthy) {
    snapshot_manager_->createTag("healthy", "Healthy snapshot", "admin");
    EXPECT_EQ(snapshot_manager_->checkConsistency(), 0u);
}

// Test 30: restoreToTag returns failure for non-existent tag
TEST_F(SnapshotManagerTest, RestoreToTagNonExistent) {
    auto result = snapshot_manager_->restoreToTag("does_not_exist");
    EXPECT_FALSE(result.success);
    EXPECT_FALSE(result.message.empty());
}

// Test 31: restoreToTag succeeds for valid tag
TEST_F(SnapshotManagerTest, RestoreToTagSuccess) {
    recordEvents(5);
    auto created = snapshot_manager_->createTag("restore_me", "Before restore", "admin");
    ASSERT_TRUE(created.has_value());

    auto result = snapshot_manager_->restoreToTag("restore_me", "admin");
    EXPECT_TRUE(result.success);
    EXPECT_EQ(result.tag_name, "restore_me");
    EXPECT_EQ(result.target_sequence, created->sequence_number);
    EXPECT_GT(result.timestamp_ms, 0);
    EXPECT_FALSE(result.message.empty());
}

// Test 32: restoreToTag creates an audit restore-point tag
TEST_F(SnapshotManagerTest, RestoreToTagCreatesAuditTag) {
    snapshot_manager_->createTag("target", "Target", "admin");
    snapshot_manager_->restoreToTag("target", "admin");

    // A "restore-of-target-..." tag should now exist
    auto tags = snapshot_manager_->listTags();
    bool found = false;
    for (const auto& t : tags) {
        if (t.tag_name.find("restore-of-target-") == 0) {
            found = true;
            break;
        }
    }
    EXPECT_TRUE(found) << "Expected audit restore-point tag to be created";
}

// Test 33: RestoreResult JSON serialization
TEST_F(SnapshotManagerTest, RestoreResultToJson) {
    snapshot_manager_->createTag("json_restore", "JSON", "admin");
    auto result = snapshot_manager_->restoreToTag("json_restore");

    auto j = result.toJson();
    EXPECT_TRUE(j.contains("success"));
    EXPECT_TRUE(j.contains("tag_name"));
    EXPECT_TRUE(j.contains("target_sequence"));
    EXPECT_TRUE(j.contains("timestamp_ms"));
    EXPECT_TRUE(j.contains("message"));
}
