// Integration tests for all merge operators

#include "storage/merge_operators.h"
#include "storage/rocksdb_wrapper.h"
#include <gtest/gtest.h>
#include <rocksdb/db.h>
#include <rocksdb/options.h>
#include <rocksdb/utilities/transaction_db.h>
#include <filesystem>
#include <thread>
#include <vector>

using namespace themis;

class MergeOperatorsIntegrationTest : public ::testing::Test {
protected:
    void SetUp() override {
        test_db_path_ = "./test_merge_integration_db";
        std::filesystem::remove_all(test_db_path_);
    }
    
    void TearDown() override {
        std::filesystem::remove_all(test_db_path_);
    }
    
    std::string test_db_path_;
};

TEST_F(MergeOperatorsIntegrationTest, CounterAndMaxTogether) {
    // Open DB with default merge operator (counter)
    rocksdb::Options options;
    options.create_if_missing = true;
    options.merge_operator = std::make_shared<CounterMergeOperator>();
    
    rocksdb::DB* db = nullptr;
    auto status = rocksdb::DB::Open(options, test_db_path_, &db);
    ASSERT_TRUE(status.ok());
    
    // Use counters
    ASSERT_TRUE(db->Merge(rocksdb::WriteOptions(), "visits", "100").ok());
    ASSERT_TRUE(db->Merge(rocksdb::WriteOptions(), "visits", "50").ok());
    ASSERT_TRUE(db->Merge(rocksdb::WriteOptions(), "visits", "75").ok());
    
    std::string value;
    ASSERT_TRUE(db->Get(rocksdb::ReadOptions(), "visits", &value).ok());
    EXPECT_EQ(value, "225");
    
    delete db;
    std::filesystem::remove_all(test_db_path_);
    
    // Now open with Max operator
    options.merge_operator = std::make_shared<MaxMergeOperator>();
    status = rocksdb::DB::Open(options, test_db_path_, &db);
    ASSERT_TRUE(status.ok());
    
    // Use max tracking
    ASSERT_TRUE(db->Merge(rocksdb::WriteOptions(), "max_temp", "25.5").ok());
    ASSERT_TRUE(db->Merge(rocksdb::WriteOptions(), "max_temp", "30.2").ok());
    ASSERT_TRUE(db->Merge(rocksdb::WriteOptions(), "max_temp", "27.8").ok());
    
    ASSERT_TRUE(db->Get(rocksdb::ReadOptions(), "max_temp", &value).ok());
    EXPECT_NEAR(std::stod(value), 30.2, 0.001);
    
    delete db;
}

TEST_F(MergeOperatorsIntegrationTest, BatchOperations) {
    rocksdb::Options options;
    options.create_if_missing = true;
    options.merge_operator = std::make_shared<CounterMergeOperator>();
    
    rocksdb::DB* db = nullptr;
    auto status = rocksdb::DB::Open(options, test_db_path_, &db);
    ASSERT_TRUE(status.ok());
    
    // Use WriteBatch for atomic operations
    rocksdb::WriteBatch batch;
    batch.Merge("counter1", "10");
    batch.Merge("counter2", "20");
    batch.Merge("counter1", "5");
    batch.Merge("counter2", "15");
    batch.Merge("counter3", "30");
    
    ASSERT_TRUE(db->Write(rocksdb::WriteOptions(), &batch).ok());
    
    // Verify all counters
    std::string value;
    ASSERT_TRUE(db->Get(rocksdb::ReadOptions(), "counter1", &value).ok());
    EXPECT_EQ(value, "15");
    
    ASSERT_TRUE(db->Get(rocksdb::ReadOptions(), "counter2", &value).ok());
    EXPECT_EQ(value, "35");
    
    ASSERT_TRUE(db->Get(rocksdb::ReadOptions(), "counter3", &value).ok());
    EXPECT_EQ(value, "30");
    
    delete db;
}

TEST_F(MergeOperatorsIntegrationTest, ConcurrentMergeOperations) {
    rocksdb::Options options;
    options.create_if_missing = true;
    options.merge_operator = std::make_shared<CounterMergeOperator>();
    
    rocksdb::DB* db = nullptr;
    auto status = rocksdb::DB::Open(options, test_db_path_, &db);
    ASSERT_TRUE(status.ok());
    
    // Multiple threads incrementing the same counter
    const int num_threads = 4;
    const int increments_per_thread = 25;
    
    std::vector<std::thread> threads = {};

    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back([db, increments_per_thread]() {
            for (int j = 0; j < increments_per_thread; ++j) {
                db->Merge(rocksdb::WriteOptions(), "concurrent_counter", "1");
            }
        });
    }
    
    for (auto& t : threads) {
        t.join();
    }
    
    // Should have 100 total increments
    std::string value;
    ASSERT_TRUE(db->Get(rocksdb::ReadOptions(), "concurrent_counter", &value).ok());
    EXPECT_EQ(value, "100");
    
    delete db;
}

TEST_F(MergeOperatorsIntegrationTest, AppendOperatorUsageScenario) {
    rocksdb::Options options;
    options.create_if_missing = true;
    options.merge_operator = std::make_shared<AppendMergeOperator>(",");
    
    rocksdb::DB* db = nullptr;
    auto status = rocksdb::DB::Open(options, test_db_path_, &db);
    ASSERT_TRUE(status.ok());
    
    // Simulate event logging
    ASSERT_TRUE(db->Merge(rocksdb::WriteOptions(), "user:123:events", "login").ok());
    ASSERT_TRUE(db->Merge(rocksdb::WriteOptions(), "user:123:events", "view_page").ok());
    ASSERT_TRUE(db->Merge(rocksdb::WriteOptions(), "user:123:events", "click_button").ok());
    ASSERT_TRUE(db->Merge(rocksdb::WriteOptions(), "user:123:events", "logout").ok());
    
    std::string value;
    ASSERT_TRUE(db->Get(rocksdb::ReadOptions(), "user:123:events", &value).ok());
    EXPECT_EQ(value, "login,view_page,click_button,logout");
    
    delete db;
}

TEST_F(MergeOperatorsIntegrationTest, SetOperatorUsageScenario) {
    rocksdb::Options options;
    options.create_if_missing = true;
    options.merge_operator = std::make_shared<SetMergeOperator>();
    
    rocksdb::DB* db = nullptr;
    auto status = rocksdb::DB::Open(options, test_db_path_, &db);
    ASSERT_TRUE(status.ok());
    
    // Simulate tag aggregation
    ASSERT_TRUE(db->Merge(rocksdb::WriteOptions(), "post:456:tags", "cpp").ok());
    ASSERT_TRUE(db->Merge(rocksdb::WriteOptions(), "post:456:tags", "database").ok());
    ASSERT_TRUE(db->Merge(rocksdb::WriteOptions(), "post:456:tags", "rocksdb").ok());
    ASSERT_TRUE(db->Merge(rocksdb::WriteOptions(), "post:456:tags", "cpp").ok()); // duplicate
    ASSERT_TRUE(db->Merge(rocksdb::WriteOptions(), "post:456:tags", "performance").ok());
    
    std::string value;
    ASSERT_TRUE(db->Get(rocksdb::ReadOptions(), "post:456:tags", &value).ok());
    EXPECT_EQ(value, "cpp,database,performance,rocksdb");
    
    delete db;
}

TEST_F(MergeOperatorsIntegrationTest, MaxOperatorUsageScenario) {
    rocksdb::Options options;
    options.create_if_missing = true;
    options.merge_operator = std::make_shared<MaxMergeOperator>();
    
    rocksdb::DB* db = nullptr;
    auto status = rocksdb::DB::Open(options, test_db_path_, &db);
    ASSERT_TRUE(status.ok());
    
    // Simulate temperature monitoring
    ASSERT_TRUE(db->Merge(rocksdb::WriteOptions(), "sensor:1:max_temp", "20.5").ok());
    ASSERT_TRUE(db->Merge(rocksdb::WriteOptions(), "sensor:1:max_temp", "22.3").ok());
    ASSERT_TRUE(db->Merge(rocksdb::WriteOptions(), "sensor:1:max_temp", "21.8").ok());
    ASSERT_TRUE(db->Merge(rocksdb::WriteOptions(), "sensor:1:max_temp", "25.7").ok());
    ASSERT_TRUE(db->Merge(rocksdb::WriteOptions(), "sensor:1:max_temp", "23.1").ok());
    
    std::string value;
    ASSERT_TRUE(db->Get(rocksdb::ReadOptions(), "sensor:1:max_temp", &value).ok());
    EXPECT_NEAR(std::stod(value), 25.7, 0.001);
    
    delete db;
}

TEST_F(MergeOperatorsIntegrationTest, MergeWithCompaction) {
    rocksdb::Options options;
    options.create_if_missing = true;
    options.merge_operator = std::make_shared<CounterMergeOperator>();
    options.level0_file_num_compaction_trigger = 2; // Trigger compaction early
    
    rocksdb::DB* db = nullptr;
    auto status = rocksdb::DB::Open(options, test_db_path_, &db);
    ASSERT_TRUE(status.ok());
    
    // Create many merge operations to trigger compaction
    for (int i = 0; i < 100; ++i) {
        ASSERT_TRUE(db->Merge(rocksdb::WriteOptions(), "compact_counter", "1").ok());
        if (i % 10 == 0) {
            db->Flush(rocksdb::FlushOptions());
        }
    }
    
    // Force compaction
    ASSERT_TRUE(db->CompactRange(rocksdb::CompactRangeOptions(), nullptr, nullptr).ok());
    
    // Verify counter is correct after compaction
    std::string value;
    ASSERT_TRUE(db->Get(rocksdb::ReadOptions(), "compact_counter", &value).ok());
    EXPECT_EQ(value, "100");
    
    delete db;
}

TEST_F(MergeOperatorsIntegrationTest, MergeWithSnapshot) {
    rocksdb::Options options;
    options.create_if_missing = true;
    options.merge_operator = std::make_shared<CounterMergeOperator>();
    
    rocksdb::DB* db = nullptr;
    auto status = rocksdb::DB::Open(options, test_db_path_, &db);
    ASSERT_TRUE(status.ok());
    
    // Initial merge
    ASSERT_TRUE(db->Merge(rocksdb::WriteOptions(), "snap_counter", "10").ok());
    
    // Take snapshot
    const rocksdb::Snapshot* snapshot = db->GetSnapshot();
    
    // More merges after snapshot
    ASSERT_TRUE(db->Merge(rocksdb::WriteOptions(), "snap_counter", "20").ok());
    ASSERT_TRUE(db->Merge(rocksdb::WriteOptions(), "snap_counter", "30").ok());
    
    // Read with snapshot (should see old value)
    rocksdb::ReadOptions read_opts;
    read_opts.snapshot = snapshot;
    std::string value;
    ASSERT_TRUE(db->Get(read_opts, "snap_counter", &value).ok());
    EXPECT_EQ(value, "10");
    
    // Read without snapshot (should see new value)
    ASSERT_TRUE(db->Get(rocksdb::ReadOptions(), "snap_counter", &value).ok());
    EXPECT_EQ(value, "60");
    
    db->ReleaseSnapshot(snapshot);
    delete db;
}
