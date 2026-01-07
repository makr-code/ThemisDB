/**
 * @file test_hybrid_retention_manager.cpp
 * @brief Unit tests for HybridRetentionManager
 */

#include <gtest/gtest.h>
#include "scheduler/hybrid_retention_manager.h"
#include "scheduler/task_scheduler.h"
#include "query/query_engine.h"
#include "storage/rocksdb_wrapper.h"
#include "timeseries/tsstore.h"
#include <filesystem>

using namespace themis;

class HybridRetentionManagerTest : public ::testing::Test {
protected:
    void SetUp() override {
        test_db_path_ = "./data/test_hybrid_retention";
        
        // Clean up any existing test data
        if (std::filesystem::exists(test_db_path_)) {
            std::filesystem::remove_all(test_db_path_);
        }
        std::filesystem::create_directories(test_db_path_);
        
        // Initialize RocksDB
        rocksdb::Options options;
        options.create_if_missing = true;
        rocksdb::TransactionDBOptions txn_options;
        
        rocksdb::TransactionDB* db;
        rocksdb::Status status = rocksdb::TransactionDB::Open(
            options, txn_options, test_db_path_, &db
        );
        ASSERT_TRUE(status.ok()) << "Failed to open test database: " << status.ToString();
        
        db_wrapper_ = std::make_unique<RocksDBWrapper>(db);
        query_engine_ = std::make_unique<QueryEngine>(db_wrapper_.get());
        tsstore_ = std::make_unique<TSStore>(db_wrapper_.get());
        
        // Create task scheduler
        TaskScheduler::Config scheduler_config;
        scheduler_config.check_interval = std::chrono::milliseconds(100);
        scheduler_ = std::make_unique<TaskScheduler>(query_engine_.get(), scheduler_config);
    }
    
    void TearDown() override {
        scheduler_.reset();
        tsstore_.reset();
        query_engine_.reset();
        db_wrapper_.reset();
        
        if (std::filesystem::exists(test_db_path_)) {
            std::filesystem::remove_all(test_db_path_);
        }
    }
    
    std::string test_db_path_;
    std::unique_ptr<RocksDBWrapper> db_wrapper_;
    std::unique_ptr<QueryEngine> query_engine_;
    std::unique_ptr<TSStore> tsstore_;
    std::unique_ptr<TaskScheduler> scheduler_;
};

TEST_F(HybridRetentionManagerTest, BasicLifecycle) {
    HybridRetentionConfig config;
    
    HybridRetentionManager manager(
        query_engine_.get(),
        tsstore_.get(),
        scheduler_.get(),
        config
    );
    
    EXPECT_FALSE(manager.isRunning());
    
    scheduler_->start();
    manager.start();
    EXPECT_TRUE(manager.isRunning());
    
    manager.stop();
    EXPECT_FALSE(manager.isRunning());
    scheduler_->stop();
}

TEST_F(HybridRetentionManagerTest, DefaultConfiguration) {
    HybridRetentionConfig config;
    
    HybridRetentionManager manager(
        query_engine_.get(),
        tsstore_.get(),
        scheduler_.get(),
        config
    );
    
    auto retrieved_config = manager.getConfig();
    
    // Verify default values
    EXPECT_TRUE(retrieved_config.stage1.enabled);
    EXPECT_TRUE(retrieved_config.stage2.enabled);
    EXPECT_TRUE(retrieved_config.stage3.enabled);
    EXPECT_TRUE(retrieved_config.auto_cleanup);
    EXPECT_EQ(retrieved_config.stage2.low_cv_threshold, 5.0);
    EXPECT_EQ(retrieved_config.stage2.medium_cv_threshold, 20.0);
}

TEST_F(HybridRetentionManagerTest, CustomConfiguration) {
    HybridRetentionConfig config;
    config.stage1.duration = std::chrono::hours(24 * 14);  // 14 days
    config.stage2.low_cv_threshold = 3.0;
    config.stage2.medium_cv_threshold = 15.0;
    config.stage3.enabled = false;
    
    HybridRetentionManager manager(
        query_engine_.get(),
        tsstore_.get(),
        scheduler_.get(),
        config
    );
    
    auto retrieved_config = manager.getConfig();
    
    EXPECT_EQ(retrieved_config.stage1.duration.count(), 24 * 14);
    EXPECT_EQ(retrieved_config.stage2.low_cv_threshold, 3.0);
    EXPECT_EQ(retrieved_config.stage2.medium_cv_threshold, 15.0);
    EXPECT_FALSE(retrieved_config.stage3.enabled);
}

TEST_F(HybridRetentionManagerTest, UpdateConfiguration) {
    HybridRetentionConfig initial_config;
    initial_config.stage2.low_cv_threshold = 5.0;
    
    HybridRetentionManager manager(
        query_engine_.get(),
        tsstore_.get(),
        scheduler_.get(),
        initial_config
    );
    
    // Update config
    HybridRetentionConfig new_config;
    new_config.stage2.low_cv_threshold = 3.0;
    new_config.stage2.medium_cv_threshold = 12.0;
    
    manager.updateConfig(new_config);
    
    auto retrieved_config = manager.getConfig();
    EXPECT_EQ(retrieved_config.stage2.low_cv_threshold, 3.0);
    EXPECT_EQ(retrieved_config.stage2.medium_cv_threshold, 12.0);
}

TEST_F(HybridRetentionManagerTest, ManualExecution) {
    HybridRetentionConfig config;
    
    HybridRetentionManager manager(
        query_engine_.get(),
        tsstore_.get(),
        scheduler_.get(),
        config
    );
    
    scheduler_->start();
    manager.start();
    
    // Manual execution should not throw
    EXPECT_NO_THROW(manager.executeStage1());
    EXPECT_NO_THROW(manager.executeStage2());
    EXPECT_NO_THROW(manager.executeStage3());
    EXPECT_NO_THROW(manager.executeAll());
    
    manager.stop();
    scheduler_->stop();
}

TEST_F(HybridRetentionManagerTest, Statistics) {
    HybridRetentionConfig config;
    
    HybridRetentionManager manager(
        query_engine_.get(),
        tsstore_.get(),
        scheduler_.get(),
        config
    );
    
    auto initial_stats = manager.getStats();
    
    // Initially all stats should be zero
    EXPECT_EQ(initial_stats.stage1.compressions_total, 0);
    EXPECT_EQ(initial_stats.stage2.aggregations_total, 0);
    EXPECT_EQ(initial_stats.stage3.aggregations_total, 0);
    
    // Reset stats should not throw
    EXPECT_NO_THROW(manager.resetStats());
    
    auto reset_stats = manager.getStats();
    EXPECT_EQ(reset_stats.stage1.compressions_total, 0);
}

TEST_F(HybridRetentionManagerTest, StatusReport) {
    HybridRetentionConfig config;
    
    HybridRetentionManager manager(
        query_engine_.get(),
        tsstore_.get(),
        scheduler_.get(),
        config
    );
    
    auto report = manager.getStatusReport();
    
    // Verify report structure
    EXPECT_TRUE(report.contains("running"));
    EXPECT_TRUE(report.contains("config"));
    EXPECT_TRUE(report.contains("stats"));
    
    EXPECT_FALSE(report["running"].get<bool>());  // Not running yet
    
    scheduler_->start();
    manager.start();
    
    report = manager.getStatusReport();
    EXPECT_TRUE(report["running"].get<bool>());  // Now running
    
    manager.stop();
    scheduler_->stop();
}

TEST_F(HybridRetentionManagerTest, DisabledStages) {
    HybridRetentionConfig config;
    config.stage1.enabled = false;
    config.stage3.enabled = false;
    // Only stage 2 enabled
    
    HybridRetentionManager manager(
        query_engine_.get(),
        tsstore_.get(),
        scheduler_.get(),
        config
    );
    
    scheduler_->start();
    manager.start();
    
    // Should start successfully even with some stages disabled
    EXPECT_TRUE(manager.isRunning());
    
    manager.stop();
    scheduler_->stop();
}

TEST_F(HybridRetentionManagerTest, MultipleManagers) {
    HybridRetentionConfig config1;
    config1.stage1.metric_pattern = "temperature_*";
    
    HybridRetentionConfig config2;
    config2.stage1.metric_pattern = "pressure_*";
    
    HybridRetentionManager manager1(
        query_engine_.get(),
        tsstore_.get(),
        scheduler_.get(),
        config1
    );
    
    HybridRetentionManager manager2(
        query_engine_.get(),
        tsstore_.get(),
        scheduler_.get(),
        config2
    );
    
    scheduler_->start();
    
    // Both should start successfully
    EXPECT_NO_THROW(manager1.start());
    EXPECT_NO_THROW(manager2.start());
    
    EXPECT_TRUE(manager1.isRunning());
    EXPECT_TRUE(manager2.isRunning());
    
    manager1.stop();
    manager2.stop();
    scheduler_->stop();
}

TEST_F(HybridRetentionManagerTest, InvalidConstruction) {
    HybridRetentionConfig config;
    
    // Null query_engine should throw
    EXPECT_THROW(
        HybridRetentionManager manager(nullptr, tsstore_.get(), scheduler_.get(), config),
        std::invalid_argument
    );
    
    // Null tsstore should throw
    EXPECT_THROW(
        HybridRetentionManager manager(query_engine_.get(), nullptr, scheduler_.get(), config),
        std::invalid_argument
    );
    
    // Null scheduler should throw
    EXPECT_THROW(
        HybridRetentionManager manager(query_engine_.get(), tsstore_.get(), nullptr, config),
        std::invalid_argument
    );
}
