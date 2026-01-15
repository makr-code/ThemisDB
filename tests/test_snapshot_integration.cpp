#include <gtest/gtest.h>
#include "transaction/snapshot_manager.h"
#include "cdc/changefeed.h"
#include "storage/rocksdb_wrapper.h"
#include <filesystem>
#include <thread>
#include <vector>
#include <chrono>
#include <atomic>
#include <future>

using namespace themis;
using namespace themis::transaction;

class SnapshotIntegrationTest : public ::testing::Test {
protected:
    void SetUp() override {
        test_db_path_ = "./data/themis_snapshot_integration_test";
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

// Integration Test 1: Tags survive database restart
TEST_F(SnapshotIntegrationTest, TagsSurviveRestart) {
    // Create several tags
    recordEvents(10);
    snapshot_manager_->createTag("v1.0.0", "Release 1.0", "admin");
    recordEvents(20);
    snapshot_manager_->createTag("v1.1.0", "Release 1.1", "admin");
    recordEvents(15);
    snapshot_manager_->createTag("v2.0.0", "Release 2.0", "admin");
    
    // Verify tags exist
    EXPECT_TRUE(snapshot_manager_->tagExists("v1.0.0"));
    EXPECT_TRUE(snapshot_manager_->tagExists("v1.1.0"));
    EXPECT_TRUE(snapshot_manager_->tagExists("v2.0.0"));
    
    auto tags_before = snapshot_manager_->listTags();
    EXPECT_EQ(tags_before.size(), 3);
    
    // Simulate restart: destroy and recreate snapshot manager
    snapshot_manager_.reset();
    snapshot_manager_ = std::make_unique<SnapshotManager>(*db_, *changefeed_);
    
    // Verify tags still exist after restart
    EXPECT_TRUE(snapshot_manager_->tagExists("v1.0.0"));
    EXPECT_TRUE(snapshot_manager_->tagExists("v1.1.0"));
    EXPECT_TRUE(snapshot_manager_->tagExists("v2.0.0"));
    
    auto tags_after = snapshot_manager_->listTags();
    EXPECT_EQ(tags_after.size(), 3);
    
    // Verify tag details are preserved
    auto tag_v1 = snapshot_manager_->getTag("v1.0.0");
    ASSERT_TRUE(tag_v1.has_value());
    EXPECT_EQ(tag_v1->tag_name, "v1.0.0");
    EXPECT_EQ(tag_v1->description, "Release 1.0");
    EXPECT_EQ(tag_v1->created_by, "admin");
}

// Integration Test 2: Handle large number of tags (1000+)
TEST_F(SnapshotIntegrationTest, HandleLargeNumberOfTags) {
    const int NUM_TAGS = 1000;
    
    // Create 1000 tags
    for (int i = 0; i < NUM_TAGS; ++i) {
        std::string tag_name = "tag_" + std::to_string(i);
        std::string description = "Tag number " + std::to_string(i);
        auto snapshot = snapshot_manager_->createTag(tag_name, description, "integration_test");
        ASSERT_TRUE(snapshot.has_value()) << "Failed to create tag: " << tag_name;
    }
    
    // List all tags
    auto all_tags = snapshot_manager_->listTags();
    EXPECT_EQ(all_tags.size(), NUM_TAGS);
    
    // Verify pagination works with large dataset
    auto first_100 = snapshot_manager_->listTags(100);
    EXPECT_EQ(first_100.size(), 100);
    
    // Verify all tags are accessible
    for (int i = 0; i < NUM_TAGS; i += 100) {
        std::string tag_name = "tag_" + std::to_string(i);
        auto tag = snapshot_manager_->getTag(tag_name);
        EXPECT_TRUE(tag.has_value()) << "Tag not found: " << tag_name;
    }
    
    // Test statistics with large dataset
    auto stats = snapshot_manager_->getStats();
    EXPECT_EQ(stats.total_snapshots, NUM_TAGS);
    EXPECT_GT(stats.newest_sequence, stats.oldest_sequence);
}

// Integration Test 3: Concurrent tag creation from multiple threads
TEST_F(SnapshotIntegrationTest, ConcurrentTagCreation) {
    const int NUM_THREADS = 4;
    const int TAGS_PER_THREAD = 50;
    std::atomic<int> success_count{0};
    std::atomic<int> failure_count{0};
    
    std::vector<std::thread> threads;
    
    for (int t = 0; t < NUM_THREADS; ++t) {
        threads.emplace_back([this, t, &success_count, &failure_count, TAGS_PER_THREAD]() {
            for (int i = 0; i < TAGS_PER_THREAD; ++i) {
                std::string tag_name = "thread_" + std::to_string(t) + "_tag_" + std::to_string(i);
                std::string description = "Created by thread " + std::to_string(t);
                
                auto snapshot = snapshot_manager_->createTag(tag_name, description, "thread_" + std::to_string(t));
                if (snapshot.has_value()) {
                    success_count.fetch_add(1);
                } else {
                    failure_count.fetch_add(1);
                }
            }
        });
    }
    
    // Wait for all threads to complete
    for (auto& thread : threads) {
        thread.join();
    }
    
    // Verify all tags were created successfully
    EXPECT_EQ(success_count.load(), NUM_THREADS * TAGS_PER_THREAD);
    EXPECT_EQ(failure_count.load(), 0);
    
    auto all_tags = snapshot_manager_->listTags();
    EXPECT_EQ(all_tags.size(), NUM_THREADS * TAGS_PER_THREAD);
}

// Integration Test 4: Concurrent tag operations (create, read, delete)
TEST_F(SnapshotIntegrationTest, ConcurrentMixedOperations) {
    // Pre-create some tags
    for (int i = 0; i < 100; ++i) {
        snapshot_manager_->createTag("initial_tag_" + std::to_string(i), "Initial", "setup");
    }
    
    std::atomic<int> create_count{0};
    std::atomic<int> read_count{0};
    std::atomic<int> delete_count{0};
    
    std::vector<std::thread> threads;
    
    // Creator thread
    threads.emplace_back([this, &create_count]() {
        for (int i = 0; i < 50; ++i) {
            auto snapshot = snapshot_manager_->createTag("new_tag_" + std::to_string(i), "New", "creator");
            if (snapshot.has_value()) {
                create_count.fetch_add(1);
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    });
    
    // Reader threads
    for (int t = 0; t < 2; ++t) {
        threads.emplace_back([this, &read_count]() {
            for (int i = 0; i < 100; ++i) {
                auto tags = snapshot_manager_->listTags(10);
                if (!tags.empty()) {
                    read_count.fetch_add(1);
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }
        });
    }
    
    // Deleter thread
    threads.emplace_back([this, &delete_count]() {
        for (int i = 0; i < 50; ++i) {
            if (snapshot_manager_->deleteTag("initial_tag_" + std::to_string(i))) {
                delete_count.fetch_add(1);
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(2));
        }
    });
    
    // Wait for all threads
    for (auto& thread : threads) {
        thread.join();
    }
    
    // Verify operations completed
    EXPECT_GT(create_count.load(), 0);
    EXPECT_GT(read_count.load(), 0);
    EXPECT_GT(delete_count.load(), 0);
    
    // Database should be in consistent state
    auto final_tags = snapshot_manager_->listTags();
    EXPECT_GT(final_tags.size(), 0);
}

// Integration Test 5: Tag creation with changefeed events
TEST_F(SnapshotIntegrationTest, TagCreationWithChangefeedEvents) {
    // Record initial events
    recordEvents(100);
    
    // Create tag at specific point
    auto tag1 = snapshot_manager_->createTag("checkpoint_1", "After 100 events", "test");
    ASSERT_TRUE(tag1.has_value());
    uint64_t seq1 = tag1->sequence_number;
    
    // Record more events
    recordEvents(200);
    
    // Create second tag
    auto tag2 = snapshot_manager_->createTag("checkpoint_2", "After 300 events", "test");
    ASSERT_TRUE(tag2.has_value());
    uint64_t seq2 = tag2->sequence_number;
    
    // Verify sequence numbers are different and ordered
    EXPECT_LT(seq1, seq2);
    
    // Record more events
    recordEvents(150);
    
    // Create third tag
    auto tag3 = snapshot_manager_->createTag("checkpoint_3", "After 450 events", "test");
    ASSERT_TRUE(tag3.has_value());
    uint64_t seq3 = tag3->sequence_number;
    
    EXPECT_LT(seq2, seq3);
    
    // Verify all tags can be retrieved
    EXPECT_TRUE(snapshot_manager_->tagExists("checkpoint_1"));
    EXPECT_TRUE(snapshot_manager_->tagExists("checkpoint_2"));
    EXPECT_TRUE(snapshot_manager_->tagExists("checkpoint_3"));
}

// Integration Test 6: Database restart with tags and changefeed
TEST_F(SnapshotIntegrationTest, RestartWithChangefeedAndTags) {
    // Create scenario with interleaved events and tags
    recordEvents(50);
    auto tag1 = snapshot_manager_->createTag("v1", "Version 1", "admin");
    ASSERT_TRUE(tag1.has_value());
    
    recordEvents(75);
    auto tag2 = snapshot_manager_->createTag("v2", "Version 2", "admin");
    ASSERT_TRUE(tag2.has_value());
    
    recordEvents(100);
    auto tag3 = snapshot_manager_->createTag("v3", "Version 3", "admin");
    ASSERT_TRUE(tag3.has_value());
    
    // Store sequence numbers before restart
    uint64_t seq1_before = tag1->sequence_number;
    uint64_t seq2_before = tag2->sequence_number;
    uint64_t seq3_before = tag3->sequence_number;
    
    // Simulate restart
    snapshot_manager_.reset();
    changefeed_.reset();
    db_.reset();
    
    // Reopen database
    RocksDBWrapper::Config config;
    config.db_path = test_db_path_;
    config.enable_wal = true;
    db_ = std::make_unique<RocksDBWrapper>(config);
    ASSERT_TRUE(db_->open());
    
    auto* txn_db = db_->getRawDB();
    ASSERT_NE(txn_db, nullptr);
    
    changefeed_ = std::make_unique<Changefeed>(txn_db);
    snapshot_manager_ = std::make_unique<SnapshotManager>(*db_, *changefeed_);
    
    // Verify tags still exist with correct sequence numbers
    auto tag1_after = snapshot_manager_->getTag("v1");
    ASSERT_TRUE(tag1_after.has_value());
    EXPECT_EQ(tag1_after->sequence_number, seq1_before);
    
    auto tag2_after = snapshot_manager_->getTag("v2");
    ASSERT_TRUE(tag2_after.has_value());
    EXPECT_EQ(tag2_after->sequence_number, seq2_before);
    
    auto tag3_after = snapshot_manager_->getTag("v3");
    ASSERT_TRUE(tag3_after.has_value());
    EXPECT_EQ(tag3_after->sequence_number, seq3_before);
}

// Integration Test 7: Stress test - rapid tag creation and deletion
TEST_F(SnapshotIntegrationTest, StressTestRapidOperations) {
    const int NUM_ITERATIONS = 100;
    
    for (int i = 0; i < NUM_ITERATIONS; ++i) {
        // Create tag
        std::string tag_name = "stress_tag_" + std::to_string(i);
        auto created = snapshot_manager_->createTag(tag_name, "Stress test", "stress");
        ASSERT_TRUE(created.has_value());
        
        // Immediately read it
        auto retrieved = snapshot_manager_->getTag(tag_name);
        ASSERT_TRUE(retrieved.has_value());
        EXPECT_EQ(retrieved->tag_name, tag_name);
        
        // Delete every other tag
        if (i % 2 == 0 && i > 0) {
            std::string old_tag = "stress_tag_" + std::to_string(i - 1);
            EXPECT_TRUE(snapshot_manager_->deleteTag(old_tag));
        }
    }
    
    // Verify final state
    auto remaining_tags = snapshot_manager_->listTags();
    EXPECT_GT(remaining_tags.size(), 0);
    EXPECT_LT(remaining_tags.size(), NUM_ITERATIONS);
}

// Integration Test 8: Tag operations with empty database
TEST_F(SnapshotIntegrationTest, OperationsOnEmptyDatabase) {
    // No events recorded yet
    
    // Create tag at sequence 0
    auto tag = snapshot_manager_->createTag("initial", "Initial tag", "test");
    ASSERT_TRUE(tag.has_value());
    
    // Verify it can be retrieved
    auto retrieved = snapshot_manager_->getTag("initial");
    ASSERT_TRUE(retrieved.has_value());
    EXPECT_EQ(retrieved->tag_name, "initial");
    
    // List tags should return 1 tag
    auto tags = snapshot_manager_->listTags();
    EXPECT_EQ(tags.size(), 1);
    
    // Statistics should be correct
    auto stats = snapshot_manager_->getStats();
    EXPECT_EQ(stats.total_snapshots, 1);
}
