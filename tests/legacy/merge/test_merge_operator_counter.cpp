// Test for CounterMergeOperator

#include "storage/merge_operators.h"
#include "storage/rocksdb_wrapper.h"
#include <gtest/gtest.h>
#include <rocksdb/db.h>
#include <rocksdb/options.h>
#include <filesystem>

using namespace themis;

class CounterMergeOperatorTest : public ::testing::Test {
protected:
    void SetUp() override {
        test_db_path_ = "./test_counter_merge_db";
        std::filesystem::remove_all(test_db_path_);
        
        // Create RocksDB with CounterMergeOperator
        rocksdb::Options options;
        options.create_if_missing = true;
        options.merge_operator = std::make_shared<CounterMergeOperator>();
        
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

TEST_F(CounterMergeOperatorTest, BasicIncrement) {
    // Merge operation with no existing value
    auto status = db_->Merge(rocksdb::WriteOptions(), "counter1", "5");
    ASSERT_TRUE(status.ok()) << status.ToString();
    
    // Read back
    std::string value = {};
    status = db_->Get(rocksdb::ReadOptions(), "counter1", &value);
    ASSERT_TRUE(status.ok());
    EXPECT_EQ(value, "5");
}

TEST_F(CounterMergeOperatorTest, MultipleIncrements) {
    // Multiple merge operations
    ASSERT_TRUE(db_->Merge(rocksdb::WriteOptions(), "counter2", "10").ok());
    ASSERT_TRUE(db_->Merge(rocksdb::WriteOptions(), "counter2", "20").ok());
    ASSERT_TRUE(db_->Merge(rocksdb::WriteOptions(), "counter2", "5").ok());
    
    // Read back - should be 35
    std::string value = {};
    auto status = db_->Get(rocksdb::ReadOptions(), "counter2", &value);
    ASSERT_TRUE(status.ok());
    EXPECT_EQ(value, "35");
}

TEST_F(CounterMergeOperatorTest, NegativeIncrements) {
    // Add positive value
    ASSERT_TRUE(db_->Merge(rocksdb::WriteOptions(), "counter3", "100").ok());
    
    // Subtract using negative merge
    ASSERT_TRUE(db_->Merge(rocksdb::WriteOptions(), "counter3", "-30").ok());
    ASSERT_TRUE(db_->Merge(rocksdb::WriteOptions(), "counter3", "-20").ok());
    
    // Should be 50
    std::string value = {};
    auto status = db_->Get(rocksdb::ReadOptions(), "counter3", &value);
    ASSERT_TRUE(status.ok());
    EXPECT_EQ(value, "50");
}

TEST_F(CounterMergeOperatorTest, ZeroIncrement) {
    // Initialize with value
    ASSERT_TRUE(db_->Merge(rocksdb::WriteOptions(), "counter4", "42").ok());
    
    // Add zero
    ASSERT_TRUE(db_->Merge(rocksdb::WriteOptions(), "counter4", "0").ok());
    
    // Should still be 42
    std::string value = {};
    auto status = db_->Get(rocksdb::ReadOptions(), "counter4", &value);
    ASSERT_TRUE(status.ok());
    EXPECT_EQ(value, "42");
}

TEST_F(CounterMergeOperatorTest, LargeNumbers) {
    // Test with large numbers
    ASSERT_TRUE(db_->Merge(rocksdb::WriteOptions(), "counter5", "1000000").ok());
    ASSERT_TRUE(db_->Merge(rocksdb::WriteOptions(), "counter5", "2000000").ok());
    ASSERT_TRUE(db_->Merge(rocksdb::WriteOptions(), "counter5", "3000000").ok());
    
    std::string value = {};
    auto status = db_->Get(rocksdb::ReadOptions(), "counter5", &value);
    ASSERT_TRUE(status.ok());
    EXPECT_EQ(value, "6000000");
}

TEST_F(CounterMergeOperatorTest, MixedWithPut) {
    // Use Put to set initial value
    ASSERT_TRUE(db_->Put(rocksdb::WriteOptions(), "counter6", "100").ok());
    
    // Then use Merge to increment
    ASSERT_TRUE(db_->Merge(rocksdb::WriteOptions(), "counter6", "50").ok());
    ASSERT_TRUE(db_->Merge(rocksdb::WriteOptions(), "counter6", "25").ok());
    
    std::string value = {};
    auto status = db_->Get(rocksdb::ReadOptions(), "counter6", &value);
    ASSERT_TRUE(status.ok());
    EXPECT_EQ(value, "175");
}

TEST_F(CounterMergeOperatorTest, ConcurrentCounters) {
    // Multiple independent counters
    ASSERT_TRUE(db_->Merge(rocksdb::WriteOptions(), "counter_a", "10").ok());
    ASSERT_TRUE(db_->Merge(rocksdb::WriteOptions(), "counter_b", "20").ok());
    ASSERT_TRUE(db_->Merge(rocksdb::WriteOptions(), "counter_a", "5").ok());
    ASSERT_TRUE(db_->Merge(rocksdb::WriteOptions(), "counter_c", "30").ok());
    ASSERT_TRUE(db_->Merge(rocksdb::WriteOptions(), "counter_b", "15").ok());
    
    std::string value = {};
    ASSERT_TRUE(db_->Get(rocksdb::ReadOptions(), "counter_a", &value).ok());
    EXPECT_EQ(value, "15");
    
    ASSERT_TRUE(db_->Get(rocksdb::ReadOptions(), "counter_b", &value).ok());
    EXPECT_EQ(value, "35");
    
    ASSERT_TRUE(db_->Get(rocksdb::ReadOptions(), "counter_c", &value).ok());
    EXPECT_EQ(value, "30");
}

TEST_F(CounterMergeOperatorTest, CounterAfterDelete) {
    // Create counter
    ASSERT_TRUE(db_->Merge(rocksdb::WriteOptions(), "counter7", "100").ok());
    
    // Delete it
    ASSERT_TRUE(db_->Delete(rocksdb::WriteOptions(), "counter7").ok());
    
    // Merge after delete should start fresh
    ASSERT_TRUE(db_->Merge(rocksdb::WriteOptions(), "counter7", "50").ok());
    
    std::string value = {};
    auto status = db_->Get(rocksdb::ReadOptions(), "counter7", &value);
    ASSERT_TRUE(status.ok());
    EXPECT_EQ(value, "50");
}

TEST_F(CounterMergeOperatorTest, PersistenceAfterReopen) {
    // Create and increment counter
    ASSERT_TRUE(db_->Merge(rocksdb::WriteOptions(), "counter8", "100").ok());
    ASSERT_TRUE(db_->Merge(rocksdb::WriteOptions(), "counter8", "200").ok());
    
    // Close and reopen database
    delete db_;
    db_ = nullptr;
    
    rocksdb::Options options;
    options.merge_operator = std::make_shared<CounterMergeOperator>();
    auto status = rocksdb::DB::Open(options, test_db_path_, &db_);
    ASSERT_TRUE(status.ok());
    
    // Verify counter persisted
    std::string value = {};
    ASSERT_TRUE(db_->Get(rocksdb::ReadOptions(), "counter8", &value).ok());
    EXPECT_EQ(value, "300");
    
    // Can still increment
    ASSERT_TRUE(db_->Merge(rocksdb::WriteOptions(), "counter8", "50").ok());
    ASSERT_TRUE(db_->Get(rocksdb::ReadOptions(), "counter8", &value).ok());
    EXPECT_EQ(value, "350");
}
