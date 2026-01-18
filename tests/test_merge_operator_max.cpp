// Test for MaxMergeOperator

#include "storage/merge_operators.h"
#include "storage/rocksdb_wrapper.h"
#include <gtest/gtest.h>
#include <rocksdb/db.h>
#include <rocksdb/options.h>
#include <filesystem>
#include <cmath>

using namespace themis;

class MaxMergeOperatorTest : public ::testing::Test {
protected:
    void SetUp() override {
        test_db_path_ = "./test_max_merge_db";
        std::filesystem::remove_all(test_db_path_);
        
        // Create RocksDB with MaxMergeOperator
        rocksdb::Options options;
        options.create_if_missing = true;
        options.merge_operator = std::make_shared<MaxMergeOperator>();
        
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

TEST_F(MaxMergeOperatorTest, BasicMax) {
    // First merge with no existing value
    auto status = db_->Merge(rocksdb::WriteOptions(), "max1", "25.5");
    ASSERT_TRUE(status.ok()) << status.ToString();
    
    // Read back
    std::string value;
    status = db_->Get(rocksdb::ReadOptions(), "max1", &value);
    ASSERT_TRUE(status.ok());
    EXPECT_EQ(std::stod(value), 25.5);
}

TEST_F(MaxMergeOperatorTest, IncreasingValues) {
    // Merge increasing values
    ASSERT_TRUE(db_->Merge(rocksdb::WriteOptions(), "max2", "10.5").ok());
    ASSERT_TRUE(db_->Merge(rocksdb::WriteOptions(), "max2", "20.3").ok());
    ASSERT_TRUE(db_->Merge(rocksdb::WriteOptions(), "max2", "30.7").ok());
    
    // Should keep the maximum
    std::string value;
    auto status = db_->Get(rocksdb::ReadOptions(), "max2", &value);
    ASSERT_TRUE(status.ok());
    EXPECT_NEAR(std::stod(value), 30.7, 0.0001);
}

TEST_F(MaxMergeOperatorTest, DecreasingValues) {
    // Merge decreasing values
    ASSERT_TRUE(db_->Merge(rocksdb::WriteOptions(), "max3", "100.0").ok());
    ASSERT_TRUE(db_->Merge(rocksdb::WriteOptions(), "max3", "50.0").ok());
    ASSERT_TRUE(db_->Merge(rocksdb::WriteOptions(), "max3", "25.0").ok());
    
    // Should keep the maximum (first value)
    std::string value;
    auto status = db_->Get(rocksdb::ReadOptions(), "max3", &value);
    ASSERT_TRUE(status.ok());
    EXPECT_NEAR(std::stod(value), 100.0, 0.0001);
}

TEST_F(MaxMergeOperatorTest, MixedValues) {
    // Mix of values
    ASSERT_TRUE(db_->Merge(rocksdb::WriteOptions(), "max4", "50.0").ok());
    ASSERT_TRUE(db_->Merge(rocksdb::WriteOptions(), "max4", "75.0").ok());
    ASSERT_TRUE(db_->Merge(rocksdb::WriteOptions(), "max4", "30.0").ok());
    ASSERT_TRUE(db_->Merge(rocksdb::WriteOptions(), "max4", "90.0").ok());
    ASSERT_TRUE(db_->Merge(rocksdb::WriteOptions(), "max4", "60.0").ok());
    
    std::string value;
    auto status = db_->Get(rocksdb::ReadOptions(), "max4", &value);
    ASSERT_TRUE(status.ok());
    EXPECT_NEAR(std::stod(value), 90.0, 0.0001);
}

TEST_F(MaxMergeOperatorTest, IntegerValues) {
    // Test with integer values
    ASSERT_TRUE(db_->Merge(rocksdb::WriteOptions(), "max5", "100").ok());
    ASSERT_TRUE(db_->Merge(rocksdb::WriteOptions(), "max5", "200").ok());
    ASSERT_TRUE(db_->Merge(rocksdb::WriteOptions(), "max5", "150").ok());
    
    std::string value;
    auto status = db_->Get(rocksdb::ReadOptions(), "max5", &value);
    ASSERT_TRUE(status.ok());
    EXPECT_NEAR(std::stod(value), 200.0, 0.0001);
}

TEST_F(MaxMergeOperatorTest, NegativeValues) {
    // Test with negative values
    ASSERT_TRUE(db_->Merge(rocksdb::WriteOptions(), "max6", "-50.0").ok());
    ASSERT_TRUE(db_->Merge(rocksdb::WriteOptions(), "max6", "-20.0").ok());
    ASSERT_TRUE(db_->Merge(rocksdb::WriteOptions(), "max6", "-100.0").ok());
    
    // Maximum should be -20.0
    std::string value;
    auto status = db_->Get(rocksdb::ReadOptions(), "max6", &value);
    ASSERT_TRUE(status.ok());
    EXPECT_NEAR(std::stod(value), -20.0, 0.0001);
}

TEST_F(MaxMergeOperatorTest, ZeroValue) {
    // Test with zero
    ASSERT_TRUE(db_->Merge(rocksdb::WriteOptions(), "max7", "-10.0").ok());
    ASSERT_TRUE(db_->Merge(rocksdb::WriteOptions(), "max7", "0.0").ok());
    ASSERT_TRUE(db_->Merge(rocksdb::WriteOptions(), "max7", "-5.0").ok());
    
    std::string value;
    auto status = db_->Get(rocksdb::ReadOptions(), "max7", &value);
    ASSERT_TRUE(status.ok());
    EXPECT_NEAR(std::stod(value), 0.0, 0.0001);
}

TEST_F(MaxMergeOperatorTest, MixedWithPut) {
    // Use Put to set initial value
    ASSERT_TRUE(db_->Put(rocksdb::WriteOptions(), "max8", "50.0").ok());
    
    // Merge higher value
    ASSERT_TRUE(db_->Merge(rocksdb::WriteOptions(), "max8", "75.0").ok());
    
    std::string value;
    auto status = db_->Get(rocksdb::ReadOptions(), "max8", &value);
    ASSERT_TRUE(status.ok());
    EXPECT_NEAR(std::stod(value), 75.0, 0.0001);
    
    // Merge lower value - should not change
    ASSERT_TRUE(db_->Merge(rocksdb::WriteOptions(), "max8", "25.0").ok());
    ASSERT_TRUE(db_->Get(rocksdb::ReadOptions(), "max8", &value).ok());
    EXPECT_NEAR(std::stod(value), 75.0, 0.0001);
}

TEST_F(MaxMergeOperatorTest, ConcurrentMaxTracking) {
    // Multiple independent max trackers
    ASSERT_TRUE(db_->Merge(rocksdb::WriteOptions(), "temp_sensor1", "25.5").ok());
    ASSERT_TRUE(db_->Merge(rocksdb::WriteOptions(), "temp_sensor2", "30.0").ok());
    ASSERT_TRUE(db_->Merge(rocksdb::WriteOptions(), "temp_sensor1", "26.3").ok());
    ASSERT_TRUE(db_->Merge(rocksdb::WriteOptions(), "temp_sensor3", "22.8").ok());
    ASSERT_TRUE(db_->Merge(rocksdb::WriteOptions(), "temp_sensor2", "28.5").ok());
    ASSERT_TRUE(db_->Merge(rocksdb::WriteOptions(), "temp_sensor1", "24.0").ok());
    
    std::string value;
    ASSERT_TRUE(db_->Get(rocksdb::ReadOptions(), "temp_sensor1", &value).ok());
    EXPECT_NEAR(std::stod(value), 26.3, 0.0001);
    
    ASSERT_TRUE(db_->Get(rocksdb::ReadOptions(), "temp_sensor2", &value).ok());
    EXPECT_NEAR(std::stod(value), 30.0, 0.0001);
    
    ASSERT_TRUE(db_->Get(rocksdb::ReadOptions(), "temp_sensor3", &value).ok());
    EXPECT_NEAR(std::stod(value), 22.8, 0.0001);
}

TEST_F(MaxMergeOperatorTest, MaxAfterDelete) {
    // Create max tracker
    ASSERT_TRUE(db_->Merge(rocksdb::WriteOptions(), "max9", "100.0").ok());
    
    // Delete it
    ASSERT_TRUE(db_->Delete(rocksdb::WriteOptions(), "max9").ok());
    
    // Merge after delete should start fresh
    ASSERT_TRUE(db_->Merge(rocksdb::WriteOptions(), "max9", "50.0").ok());
    
    std::string value;
    auto status = db_->Get(rocksdb::ReadOptions(), "max9", &value);
    ASSERT_TRUE(status.ok());
    EXPECT_NEAR(std::stod(value), 50.0, 0.0001);
}

TEST_F(MaxMergeOperatorTest, PersistenceAfterReopen) {
    // Create and track max
    ASSERT_TRUE(db_->Merge(rocksdb::WriteOptions(), "max10", "100.5").ok());
    ASSERT_TRUE(db_->Merge(rocksdb::WriteOptions(), "max10", "150.3").ok());
    ASSERT_TRUE(db_->Merge(rocksdb::WriteOptions(), "max10", "120.0").ok());
    
    // Close and reopen database
    delete db_;
    db_ = nullptr;
    
    rocksdb::Options options;
    options.merge_operator = std::make_shared<MaxMergeOperator>();
    auto status = rocksdb::DB::Open(options, test_db_path_, &db_);
    ASSERT_TRUE(status.ok());
    
    // Verify max persisted
    std::string value;
    ASSERT_TRUE(db_->Get(rocksdb::ReadOptions(), "max10", &value).ok());
    EXPECT_NEAR(std::stod(value), 150.3, 0.0001);
    
    // Can still track max
    ASSERT_TRUE(db_->Merge(rocksdb::WriteOptions(), "max10", "175.0").ok());
    ASSERT_TRUE(db_->Get(rocksdb::ReadOptions(), "max10", &value).ok());
    EXPECT_NEAR(std::stod(value), 175.0, 0.0001);
}

TEST_F(MaxMergeOperatorTest, LargeNumbers) {
    // Test with large numbers
    ASSERT_TRUE(db_->Merge(rocksdb::WriteOptions(), "max11", "1000000.5").ok());
    ASSERT_TRUE(db_->Merge(rocksdb::WriteOptions(), "max11", "2000000.3").ok());
    ASSERT_TRUE(db_->Merge(rocksdb::WriteOptions(), "max11", "1500000.0").ok());
    
    std::string value;
    auto status = db_->Get(rocksdb::ReadOptions(), "max11", &value);
    ASSERT_TRUE(status.ok());
    EXPECT_NEAR(std::stod(value), 2000000.3, 0.1);
}
