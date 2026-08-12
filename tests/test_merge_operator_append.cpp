// Test for AppendMergeOperator

#include "storage/merge_operators.h"
#include "storage/rocksdb_wrapper.h"
#include <gtest/gtest.h>
#include <rocksdb/db.h>
#include <rocksdb/options.h>
#include <filesystem>

using namespace themis;

class AppendMergeOperatorTest : public ::testing::Test {
protected:
    void SetUp() override {
        test_db_path_ = "./test_append_merge_db";
        std::filesystem::remove_all(test_db_path_);
        
        // Create RocksDB with AppendMergeOperator (default delimiter |)
        rocksdb::Options options;
        options.create_if_missing = true;
        options.merge_operator = std::make_shared<AppendMergeOperator>();
        
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

TEST_F(AppendMergeOperatorTest, BasicAppend) {
    // First append with no existing value
    auto status = db_->Merge(rocksdb::WriteOptions(), "log1", "event1");
    ASSERT_TRUE(status.ok()) << status.ToString();
    
    // Read back
    std::string value;
    status = db_->Get(rocksdb::ReadOptions(), "log1", &value);
    ASSERT_TRUE(status.ok());
    EXPECT_EQ(value, "event1");
}

TEST_F(AppendMergeOperatorTest, MultipleAppends) {
    // Multiple append operations
    ASSERT_TRUE(db_->Merge(rocksdb::WriteOptions(), "log2", "event1").ok());
    ASSERT_TRUE(db_->Merge(rocksdb::WriteOptions(), "log2", "event2").ok());
    ASSERT_TRUE(db_->Merge(rocksdb::WriteOptions(), "log2", "event3").ok());
    
    // Read back - should be delimited
    std::string value;
    auto status = db_->Get(rocksdb::ReadOptions(), "log2", &value);
    ASSERT_TRUE(status.ok());
    EXPECT_EQ(value, "event1|event2|event3");
}

TEST_F(AppendMergeOperatorTest, EmptyStringAppend) {
    // Append empty string
    ASSERT_TRUE(db_->Merge(rocksdb::WriteOptions(), "log3", "start").ok());
    ASSERT_TRUE(db_->Merge(rocksdb::WriteOptions(), "log3", "").ok());
    ASSERT_TRUE(db_->Merge(rocksdb::WriteOptions(), "log3", "end").ok());
    
    std::string value;
    auto status = db_->Get(rocksdb::ReadOptions(), "log3", &value);
    ASSERT_TRUE(status.ok());
    EXPECT_EQ(value, "start||end");
}

TEST_F(AppendMergeOperatorTest, AppendWithSpecialCharacters) {
    // Test with special characters
    ASSERT_TRUE(db_->Merge(rocksdb::WriteOptions(), "log4", "msg:info").ok());
    ASSERT_TRUE(db_->Merge(rocksdb::WriteOptions(), "log4", "msg:warn").ok());
    ASSERT_TRUE(db_->Merge(rocksdb::WriteOptions(), "log4", "msg:error").ok());
    
    std::string value;
    auto status = db_->Get(rocksdb::ReadOptions(), "log4", &value);
    ASSERT_TRUE(status.ok());
    EXPECT_EQ(value, "msg:info|msg:warn|msg:error");
}

TEST_F(AppendMergeOperatorTest, LongStrings) {
    // Test with longer strings
    std::string long_event1(1000, 'A');
    std::string long_event2(1000, 'B');
    
    ASSERT_TRUE(db_->Merge(rocksdb::WriteOptions(), "log5", long_event1).ok());
    ASSERT_TRUE(db_->Merge(rocksdb::WriteOptions(), "log5", long_event2).ok());
    
    std::string value;
    auto status = db_->Get(rocksdb::ReadOptions(), "log5", &value);
    ASSERT_TRUE(status.ok());
    EXPECT_EQ(value.size(), 2001u); // 1000 + 1 (delimiter) + 1000
    EXPECT_EQ(value, long_event1 + "|" + long_event2);
}

TEST_F(AppendMergeOperatorTest, MixedWithPut) {
    // Use Put to set initial value
    ASSERT_TRUE(db_->Put(rocksdb::WriteOptions(), "log6", "initial").ok());
    
    // Then use Merge to append
    ASSERT_TRUE(db_->Merge(rocksdb::WriteOptions(), "log6", "appended1").ok());
    ASSERT_TRUE(db_->Merge(rocksdb::WriteOptions(), "log6", "appended2").ok());
    
    std::string value;
    auto status = db_->Get(rocksdb::ReadOptions(), "log6", &value);
    ASSERT_TRUE(status.ok());
    EXPECT_EQ(value, "initial|appended1|appended2");
}

TEST_F(AppendMergeOperatorTest, ConcurrentLogs) {
    // Multiple independent logs
    ASSERT_TRUE(db_->Merge(rocksdb::WriteOptions(), "log_a", "a1").ok());
    ASSERT_TRUE(db_->Merge(rocksdb::WriteOptions(), "log_b", "b1").ok());
    ASSERT_TRUE(db_->Merge(rocksdb::WriteOptions(), "log_a", "a2").ok());
    ASSERT_TRUE(db_->Merge(rocksdb::WriteOptions(), "log_c", "c1").ok());
    ASSERT_TRUE(db_->Merge(rocksdb::WriteOptions(), "log_b", "b2").ok());
    
    std::string value;
    ASSERT_TRUE(db_->Get(rocksdb::ReadOptions(), "log_a", &value).ok());
    EXPECT_EQ(value, "a1|a2");
    
    ASSERT_TRUE(db_->Get(rocksdb::ReadOptions(), "log_b", &value).ok());
    EXPECT_EQ(value, "b1|b2");
    
    ASSERT_TRUE(db_->Get(rocksdb::ReadOptions(), "log_c", &value).ok());
    EXPECT_EQ(value, "c1");
}

TEST_F(AppendMergeOperatorTest, AppendAfterDelete) {
    // Create log
    ASSERT_TRUE(db_->Merge(rocksdb::WriteOptions(), "log7", "old1").ok());
    ASSERT_TRUE(db_->Merge(rocksdb::WriteOptions(), "log7", "old2").ok());
    
    // Delete it
    ASSERT_TRUE(db_->Delete(rocksdb::WriteOptions(), "log7").ok());
    
    // Append after delete should start fresh
    ASSERT_TRUE(db_->Merge(rocksdb::WriteOptions(), "log7", "new1").ok());
    ASSERT_TRUE(db_->Merge(rocksdb::WriteOptions(), "log7", "new2").ok());
    
    std::string value;
    auto status = db_->Get(rocksdb::ReadOptions(), "log7", &value);
    ASSERT_TRUE(status.ok());
    EXPECT_EQ(value, "new1|new2");
}

TEST_F(AppendMergeOperatorTest, PersistenceAfterReopen) {
    // Create and append to log
    ASSERT_TRUE(db_->Merge(rocksdb::WriteOptions(), "log8", "persistent1").ok());
    ASSERT_TRUE(db_->Merge(rocksdb::WriteOptions(), "log8", "persistent2").ok());
    
    // Close and reopen database
    delete db_;
    db_ = nullptr;
    
    rocksdb::Options options;
    options.merge_operator = std::make_shared<AppendMergeOperator>();
    auto status = rocksdb::DB::Open(options, test_db_path_, &db_);
    ASSERT_TRUE(status.ok());
    
    // Verify log persisted
    std::string value;
    ASSERT_TRUE(db_->Get(rocksdb::ReadOptions(), "log8", &value).ok());
    EXPECT_EQ(value, "persistent1|persistent2");
    
    // Can still append
    ASSERT_TRUE(db_->Merge(rocksdb::WriteOptions(), "log8", "persistent3").ok());
    ASSERT_TRUE(db_->Get(rocksdb::ReadOptions(), "log8", &value).ok());
    EXPECT_EQ(value, "persistent1|persistent2|persistent3");
}

TEST_F(AppendMergeOperatorTest, CustomDelimiterTest) {
    // Close existing DB
    delete db_;
    db_ = nullptr;
    std::filesystem::remove_all(test_db_path_);
    
    // Create with custom delimiter
    rocksdb::Options options;
    options.create_if_missing = true;
    options.merge_operator = std::make_shared<AppendMergeOperator>(",");
    
    auto status = rocksdb::DB::Open(options, test_db_path_, &db_);
    ASSERT_TRUE(status.ok());
    
    // Test with comma delimiter
    ASSERT_TRUE(db_->Merge(rocksdb::WriteOptions(), "log9", "item1").ok());
    ASSERT_TRUE(db_->Merge(rocksdb::WriteOptions(), "log9", "item2").ok());
    ASSERT_TRUE(db_->Merge(rocksdb::WriteOptions(), "log9", "item3").ok());
    
    std::string value;
    ASSERT_TRUE(db_->Get(rocksdb::ReadOptions(), "log9", &value).ok());
    EXPECT_EQ(value, "item1,item2,item3");
}
