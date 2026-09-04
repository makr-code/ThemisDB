// Test for SetMergeOperator

#include "storage/merge_operators.h"
#include "storage/rocksdb_wrapper.h"
#include <gtest/gtest.h>
#include <rocksdb/db.h>
#include <rocksdb/options.h>
#include <filesystem>
#include <algorithm>

using namespace themis;

class SetMergeOperatorTest : public ::testing::Test {
protected:
    void SetUp() override {
        test_db_path_ = "./test_set_merge_db";
        std::filesystem::remove_all(test_db_path_);
        
        // Create RocksDB with SetMergeOperator
        rocksdb::Options options;
        options.create_if_missing = true;
        options.merge_operator = std::make_shared<SetMergeOperator>();
        
        auto status = rocksdb::DB::Open(options, test_db_path_, &db_);
        ASSERT_TRUE(status.ok()) << status.ToString();
    }
    
    void TearDown() override {
        delete db_;
        db_ = nullptr;
        std::filesystem::remove_all(test_db_path_);
    }
    
    std::string test_db_path_;
    rocksdb::DB* db_ = nullptr;
};

TEST_F(SetMergeOperatorTest, BasicSetAdd) {
    // First add with no existing value
    auto status = db_->Merge(rocksdb::WriteOptions(), "set1", "id1");
    ASSERT_TRUE(status.ok()) << status.ToString();
    
    // Read back
    std::string value = {};
    status = db_->Get(rocksdb::ReadOptions(), "set1", &value);
    ASSERT_TRUE(status.ok());
    EXPECT_EQ(value, "id1");
}

TEST_F(SetMergeOperatorTest, MultipleUniqueValues) {
    // Add multiple unique values
    ASSERT_TRUE(db_->Merge(rocksdb::WriteOptions(), "set2", "id1").ok());
    ASSERT_TRUE(db_->Merge(rocksdb::WriteOptions(), "set2", "id2").ok());
    ASSERT_TRUE(db_->Merge(rocksdb::WriteOptions(), "set2", "id3").ok());
    
    // Read back - should be sorted and unique
    std::string value = {};
    auto status = db_->Get(rocksdb::ReadOptions(), "set2", &value);
    ASSERT_TRUE(status.ok());
    EXPECT_EQ(value, "id1,id2,id3");
}

TEST_F(SetMergeOperatorTest, DuplicateValues) {
    // Add duplicate values
    ASSERT_TRUE(db_->Merge(rocksdb::WriteOptions(), "set3", "id1").ok());
    ASSERT_TRUE(db_->Merge(rocksdb::WriteOptions(), "set3", "id2").ok());
    ASSERT_TRUE(db_->Merge(rocksdb::WriteOptions(), "set3", "id1").ok()); // duplicate
    ASSERT_TRUE(db_->Merge(rocksdb::WriteOptions(), "set3", "id3").ok());
    ASSERT_TRUE(db_->Merge(rocksdb::WriteOptions(), "set3", "id2").ok()); // duplicate
    
    // Read back - duplicates should be removed
    std::string value = {};
    auto status = db_->Get(rocksdb::ReadOptions(), "set3", &value);
    ASSERT_TRUE(status.ok());
    EXPECT_EQ(value, "id1,id2,id3");
}

TEST_F(SetMergeOperatorTest, BatchAddMultipleValues) {
    // Add multiple values in one merge operation (comma-separated)
    ASSERT_TRUE(db_->Merge(rocksdb::WriteOptions(), "set4", "id1,id2,id3").ok());
    ASSERT_TRUE(db_->Merge(rocksdb::WriteOptions(), "set4", "id4,id5").ok());
    
    std::string value = {};
    auto status = db_->Get(rocksdb::ReadOptions(), "set4", &value);
    ASSERT_TRUE(status.ok());
    EXPECT_EQ(value, "id1,id2,id3,id4,id5");
}

TEST_F(SetMergeOperatorTest, BatchWithDuplicates) {
    // Add batch with duplicates
    ASSERT_TRUE(db_->Merge(rocksdb::WriteOptions(), "set5", "id1,id2,id3").ok());
    ASSERT_TRUE(db_->Merge(rocksdb::WriteOptions(), "set5", "id2,id3,id4").ok());
    
    std::string value = {};
    auto status = db_->Get(rocksdb::ReadOptions(), "set5", &value);
    ASSERT_TRUE(status.ok());
    EXPECT_EQ(value, "id1,id2,id3,id4"); // Should deduplicate
}

TEST_F(SetMergeOperatorTest, EmptyStringHandling) {
    // Add with empty strings in batch
    ASSERT_TRUE(db_->Merge(rocksdb::WriteOptions(), "set6", "id1,,id2").ok());
    ASSERT_TRUE(db_->Merge(rocksdb::WriteOptions(), "set6", ",id3,").ok());
    
    std::string value = {};
    auto status = db_->Get(rocksdb::ReadOptions(), "set6", &value);
    ASSERT_TRUE(status.ok());
    // Empty strings should be filtered out
    EXPECT_EQ(value, "id1,id2,id3");
}

TEST_F(SetMergeOperatorTest, MixedWithPut) {
    // Use Put to set initial value
    ASSERT_TRUE(db_->Put(rocksdb::WriteOptions(), "set7", "id1,id2").ok());
    
    // Then use Merge to add
    ASSERT_TRUE(db_->Merge(rocksdb::WriteOptions(), "set7", "id3").ok());
    ASSERT_TRUE(db_->Merge(rocksdb::WriteOptions(), "set7", "id1").ok()); // duplicate
    
    std::string value = {};
    auto status = db_->Get(rocksdb::ReadOptions(), "set7", &value);
    ASSERT_TRUE(status.ok());
    EXPECT_EQ(value, "id1,id2,id3");
}

TEST_F(SetMergeOperatorTest, ConcurrentSets) {
    // Multiple independent sets
    ASSERT_TRUE(db_->Merge(rocksdb::WriteOptions(), "set_a", "a1,a2").ok());
    ASSERT_TRUE(db_->Merge(rocksdb::WriteOptions(), "set_b", "b1").ok());
    ASSERT_TRUE(db_->Merge(rocksdb::WriteOptions(), "set_a", "a3").ok());
    ASSERT_TRUE(db_->Merge(rocksdb::WriteOptions(), "set_c", "c1,c2,c3").ok());
    ASSERT_TRUE(db_->Merge(rocksdb::WriteOptions(), "set_b", "b2,b3").ok());
    
    std::string value = {};
    ASSERT_TRUE(db_->Get(rocksdb::ReadOptions(), "set_a", &value).ok());
    EXPECT_EQ(value, "a1,a2,a3");
    
    ASSERT_TRUE(db_->Get(rocksdb::ReadOptions(), "set_b", &value).ok());
    EXPECT_EQ(value, "b1,b2,b3");
    
    ASSERT_TRUE(db_->Get(rocksdb::ReadOptions(), "set_c", &value).ok());
    EXPECT_EQ(value, "c1,c2,c3");
}

TEST_F(SetMergeOperatorTest, SetAfterDelete) {
    // Create set
    ASSERT_TRUE(db_->Merge(rocksdb::WriteOptions(), "set8", "old1,old2").ok());
    
    // Delete it
    ASSERT_TRUE(db_->Delete(rocksdb::WriteOptions(), "set8").ok());
    
    // Add after delete should start fresh
    ASSERT_TRUE(db_->Merge(rocksdb::WriteOptions(), "set8", "new1,new2").ok());
    
    std::string value = {};
    auto status = db_->Get(rocksdb::ReadOptions(), "set8", &value);
    ASSERT_TRUE(status.ok());
    EXPECT_EQ(value, "new1,new2");
}

TEST_F(SetMergeOperatorTest, PersistenceAfterReopen) {
    // Create and add to set
    ASSERT_TRUE(db_->Merge(rocksdb::WriteOptions(), "set9", "id1,id2").ok());
    ASSERT_TRUE(db_->Merge(rocksdb::WriteOptions(), "set9", "id3").ok());
    
    // Close and reopen database
    delete db_;
    db_ = nullptr;
    
    rocksdb::Options options;
    options.merge_operator = std::make_shared<SetMergeOperator>();
    auto status = rocksdb::DB::Open(options, test_db_path_, &db_);
    ASSERT_TRUE(status.ok());
    
    // Verify set persisted
    std::string value = {};
    ASSERT_TRUE(db_->Get(rocksdb::ReadOptions(), "set9", &value).ok());
    EXPECT_EQ(value, "id1,id2,id3");
    
    // Can still add
    ASSERT_TRUE(db_->Merge(rocksdb::WriteOptions(), "set9", "id4,id2").ok()); // id2 is duplicate
    ASSERT_TRUE(db_->Get(rocksdb::ReadOptions(), "set9", &value).ok());
    EXPECT_EQ(value, "id1,id2,id3,id4");
}

TEST_F(SetMergeOperatorTest, AlphanumericValues) {
    // Test with various alphanumeric values
    ASSERT_TRUE(db_->Merge(rocksdb::WriteOptions(), "set10", "user123").ok());
    ASSERT_TRUE(db_->Merge(rocksdb::WriteOptions(), "set10", "user456").ok());
    ASSERT_TRUE(db_->Merge(rocksdb::WriteOptions(), "set10", "user789").ok());
    ASSERT_TRUE(db_->Merge(rocksdb::WriteOptions(), "set10", "user123").ok()); // duplicate
    
    std::string value = {};
    auto status = db_->Get(rocksdb::ReadOptions(), "set10", &value);
    ASSERT_TRUE(status.ok());
    EXPECT_EQ(value, "user123,user456,user789");
}
