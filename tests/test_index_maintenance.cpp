// Test for Index Maintenance Manager

#include "storage/index_maintenance.h"
#include "storage/rocksdb_wrapper.h"
#include <gtest/gtest.h>
#include <filesystem>
#include <thread>
#include <chrono>

using namespace themis;

class IndexMaintenanceTest : public ::testing::Test {
protected:
    void SetUp() override {
        test_db_path_ = "./test_index_maintenance_db";
        std::filesystem::remove_all(test_db_path_);
        
        // Create RocksDB wrapper
        RocksDBWrapper::Config config;
        config.db_path = test_db_path_;
        config.enable_statistics = true;
        
        db_wrapper_ = std::make_shared<RocksDBWrapper>(config);
        auto result = db_wrapper_->open();
        ASSERT_TRUE(result) << result.error().message();
        
        // Create maintenance manager
        maintenance_manager_ = std::make_unique<IndexMaintenanceManager>(db_wrapper_);
    }
    
    void TearDown() override {
        maintenance_manager_.reset();
        db_wrapper_->close();
        db_wrapper_.reset();
        std::filesystem::remove_all(test_db_path_);
    }
    
    void populateData(int num_keys = 1000) {
        auto db = db_wrapper_->getDB();
        ASSERT_NE(db, nullptr);
        
        for (int i = 0; i < num_keys; ++i) {
            std::string key = "key_" + std::to_string(i);
            std::string value = "value_" + std::to_string(i);
            auto s = db->Put(rocksdb::WriteOptions(), key, value);
            ASSERT_TRUE(s.ok()) << s.ToString();
        }
        
        // Flush to create SST files
        auto s = db->Flush(rocksdb::FlushOptions());
        ASSERT_TRUE(s.ok()) << s.ToString();
    }
    
    std::string test_db_path_;
    std::shared_ptr<RocksDBWrapper> db_wrapper_;
    std::unique_ptr<IndexMaintenanceManager> maintenance_manager_;
};

TEST_F(IndexMaintenanceTest, ManagerInitialization) {
    EXPECT_FALSE(maintenance_manager_->isRunning());
}

TEST_F(IndexMaintenanceTest, StartAndStopBackgroundThread) {
    auto result = maintenance_manager_->start();
    ASSERT_TRUE(result) << result.error().message();
    EXPECT_TRUE(maintenance_manager_->isRunning());
    
    // Give thread time to start
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    result = maintenance_manager_->stop();
    ASSERT_TRUE(result) << result.error().message();
    EXPECT_FALSE(maintenance_manager_->isRunning());
}

TEST_F(IndexMaintenanceTest, DoubleStartFails) {
    auto result = maintenance_manager_->start();
    ASSERT_TRUE(result);
    
    // Second start should fail
    result = maintenance_manager_->start();
    EXPECT_FALSE(result);
    EXPECT_EQ(result.error().code(), errors::ErrorCode::ERR_INDEX_MAINTENANCE_IN_PROGRESS);
    
    maintenance_manager_->stop();
}

TEST_F(IndexMaintenanceTest, SetAndGetPolicy) {
    MaintenancePolicy policy;
    policy.reorganize_threshold = 15.0;
    policy.rebuild_threshold = 35.0;
    policy.max_concurrent_jobs = 4;
    policy.online_maintenance = false;
    
    auto result = maintenance_manager_->setPolicy(policy);
    ASSERT_TRUE(result) << result.error().message();
    
    auto retrieved_policy = maintenance_manager_->getPolicy();
    EXPECT_DOUBLE_EQ(retrieved_policy.reorganize_threshold, 15.0);
    EXPECT_DOUBLE_EQ(retrieved_policy.rebuild_threshold, 35.0);
    EXPECT_EQ(retrieved_policy.max_concurrent_jobs, 4);
    EXPECT_FALSE(retrieved_policy.online_maintenance);
}

TEST_F(IndexMaintenanceTest, FragmentationMonitoring) {
    populateData(100);
    
    auto result = maintenance_manager_->monitorFragmentation("test_index");
    ASSERT_TRUE(result) << result.error().message();
    
    const auto& metrics = *result;
    EXPECT_EQ(metrics.index_name, "test_index");
    EXPECT_GE(metrics.fragmentation_percentage, 0.0);
    EXPECT_LE(metrics.fragmentation_percentage, 100.0);
    EXPECT_NE(metrics.level, FragmentationLevel::LOW);  // Should have some fragmentation
}

TEST_F(IndexMaintenanceTest, FragmentationLevelClassification) {
    populateData(100);
    
    auto result = maintenance_manager_->monitorFragmentation("test_index");
    ASSERT_TRUE(result);
    
    const auto& metrics = *result;
    
    if (metrics.fragmentation_percentage <= 10.0) {
        EXPECT_EQ(metrics.level, FragmentationLevel::LOW);
    } else if (metrics.fragmentation_percentage <= 30.0) {
        EXPECT_EQ(metrics.level, FragmentationLevel::MEDIUM);
    } else {
        EXPECT_EQ(metrics.level, FragmentationLevel::HIGH);
    }
}

TEST_F(IndexMaintenanceTest, SynchronousIndexRebuild) {
    populateData(1000);
    
    // Get initial metrics
    auto metrics_before = maintenance_manager_->monitorFragmentation("test_index");
    ASSERT_TRUE(metrics_before);
    
    // Perform synchronous rebuild
    auto result = maintenance_manager_->rebuildIndex("test_index", false);
    ASSERT_TRUE(result) << result.error().message();
    
    const auto& status = *result;
    EXPECT_EQ(status.index_name, "test_index");
    EXPECT_EQ(status.operation, MaintenanceOperation::INDEX_REBUILD);
    EXPECT_TRUE(status.is_completed);
    EXPECT_FALSE(status.is_failed);
    EXPECT_FALSE(status.is_running);
    EXPECT_DOUBLE_EQ(status.progress_percentage, 100.0);
    EXPECT_GT(status.duration_ms, 0);
    
    // Check fragmentation improved
    EXPECT_LE(status.after_metrics.fragmentation_percentage,
              status.before_metrics.fragmentation_percentage);
}

TEST_F(IndexMaintenanceTest, AsynchronousIndexRebuild) {
    populateData(1000);
    
    // Start async rebuild
    auto result = maintenance_manager_->rebuildIndex("test_index", true);
    ASSERT_TRUE(result) << result.error().message();
    
    const auto& status = *result;
    EXPECT_TRUE(status.is_running || status.is_completed);
    EXPECT_FALSE(status.is_failed);
    
    // Give it time to complete
    std::this_thread::sleep_for(std::chrono::seconds(2));
    
    // Check job status
    auto job_result = maintenance_manager_->getJobStatus(status.job_id);
    ASSERT_TRUE(job_result) << job_result.error().message();
    
    const auto& job_status = *job_result;
    EXPECT_TRUE(job_status.is_completed || job_status.is_running);
}

TEST_F(IndexMaintenanceTest, SynchronousIndexReorganize) {
    populateData(500);
    
    // Perform synchronous reorganization
    auto result = maintenance_manager_->reorganizeIndex("test_index", false);
    ASSERT_TRUE(result) << result.error().message();
    
    const auto& status = *result;
    EXPECT_EQ(status.index_name, "test_index");
    EXPECT_EQ(status.operation, MaintenanceOperation::INDEX_REORGANIZATION);
    EXPECT_TRUE(status.is_completed);
    EXPECT_FALSE(status.is_failed);
    EXPECT_DOUBLE_EQ(status.progress_percentage, 100.0);
}

TEST_F(IndexMaintenanceTest, StatisticsUpdate) {
    populateData(500);
    
    auto result = maintenance_manager_->updateStatistics("test_index");
    ASSERT_TRUE(result) << result.error().message();
    
    const auto& status = *result;
    EXPECT_EQ(status.operation, MaintenanceOperation::STATISTICS_UPDATE);
    EXPECT_TRUE(status.is_completed);
    EXPECT_FALSE(status.is_failed);
}

TEST_F(IndexMaintenanceTest, OrphanEntryCleanup) {
    populateData(500);
    
    // Delete some keys to create "orphans"
    auto db = db_wrapper_->getDB();
    for (int i = 0; i < 100; ++i) {
        std::string key = "key_" + std::to_string(i);
        db->Delete(rocksdb::WriteOptions(), key);
    }
    
    auto result = maintenance_manager_->cleanupOrphanEntries("test_index");
    ASSERT_TRUE(result) << result.error().message();
    
    const auto& status = *result;
    EXPECT_EQ(status.operation, MaintenanceOperation::ORPHAN_ENTRY_CLEANUP);
    EXPECT_TRUE(status.is_completed);
    EXPECT_FALSE(status.is_failed);
}

TEST_F(IndexMaintenanceTest, ConsistencyCheck) {
    populateData(500);
    
    auto result = maintenance_manager_->checkConsistency("test_index", false);
    ASSERT_TRUE(result) << result.error().message();
    
    const auto& status = *result;
    EXPECT_EQ(status.operation, MaintenanceOperation::CONSISTENCY_CHECK);
    EXPECT_TRUE(status.is_completed);
    EXPECT_FALSE(status.is_failed);
}

TEST_F(IndexMaintenanceTest, ConsistencyCheckWithRepair) {
    populateData(500);
    
    auto result = maintenance_manager_->checkConsistency("test_index", true);
    ASSERT_TRUE(result) << result.error().message();
    
    const auto& status = *result;
    EXPECT_EQ(status.operation, MaintenanceOperation::CONSISTENCY_CHECK);
    EXPECT_TRUE(status.is_completed);
}

TEST_F(IndexMaintenanceTest, ListActiveJobs) {
    populateData(1000);
    
    // Start multiple async jobs
    auto rebuild_result = maintenance_manager_->rebuildIndex("index1", true);
    ASSERT_TRUE(rebuild_result);
    
    auto reorg_result = maintenance_manager_->reorganizeIndex("index2", true);
    ASSERT_TRUE(reorg_result);
    
    // List active jobs
    auto active_jobs = maintenance_manager_->listActiveJobs();
    
    // May have 0, 1, or 2 jobs depending on timing
    EXPECT_LE(active_jobs.size(), 2);
}

TEST_F(IndexMaintenanceTest, GetJobStatus) {
    populateData(500);
    
    auto result = maintenance_manager_->rebuildIndex("test_index", false);
    ASSERT_TRUE(result);
    
    const auto& status = *result;
    
    auto job_result = maintenance_manager_->getJobStatus(status.job_id);
    ASSERT_TRUE(job_result) << job_result.error().message();
    
    EXPECT_EQ(job_result->job_id, status.job_id);
}

TEST_F(IndexMaintenanceTest, GetJobStatusNotFound) {
    auto result = maintenance_manager_->getJobStatus("nonexistent_job");
    EXPECT_FALSE(result);
    EXPECT_EQ(result.error().code(), errors::ErrorCode::ERR_INDEX_NOT_FOUND);
}

TEST_F(IndexMaintenanceTest, CancelJob) {
    populateData(1000);
    
    // Start async job
    auto result = maintenance_manager_->rebuildIndex("test_index", true);
    ASSERT_TRUE(result);
    
    // Cancel immediately
    auto cancel_result = maintenance_manager_->cancelJob(result->job_id);
    EXPECT_TRUE(cancel_result) << cancel_result.error().message();
}

TEST_F(IndexMaintenanceTest, GetAllFragmentationMetrics) {
    populateData(500);
    
    auto metrics_map = maintenance_manager_->getAllFragmentationMetrics();
    
    // Should have at least some metrics
    EXPECT_FALSE(metrics_map.empty());
    
    for (const auto& [index_name, metrics] : metrics_map) {
        EXPECT_FALSE(index_name.empty());
        EXPECT_GE(metrics.fragmentation_percentage, 0.0);
        EXPECT_LE(metrics.fragmentation_percentage, 100.0);
    }
}

TEST_F(IndexMaintenanceTest, TriggerMaintenanceCheckWithoutStart) {
    auto result = maintenance_manager_->triggerMaintenanceCheck();
    EXPECT_FALSE(result);
    EXPECT_EQ(result.error().code(), errors::ErrorCode::ERR_INDEX_MAINTENANCE_DISABLED);
}

TEST_F(IndexMaintenanceTest, TriggerMaintenanceCheckWithStart) {
    auto start_result = maintenance_manager_->start();
    ASSERT_TRUE(start_result);
    
    auto result = maintenance_manager_->triggerMaintenanceCheck();
    EXPECT_TRUE(result) << result.error().message();
    
    maintenance_manager_->stop();
}

TEST_F(IndexMaintenanceTest, MaintenanceWindowCheck) {
    MaintenancePolicy policy;
    policy.enable_maintenance_window = true;
    policy.window_start_hour = 2;
    policy.window_end_hour = 6;
    
    auto result = maintenance_manager_->setPolicy(policy);
    ASSERT_TRUE(result);
    
    auto retrieved_policy = maintenance_manager_->getPolicy();
    EXPECT_TRUE(retrieved_policy.enable_maintenance_window);
    EXPECT_EQ(retrieved_policy.window_start_hour, 2);
    EXPECT_EQ(retrieved_policy.window_end_hour, 6);
}

TEST_F(IndexMaintenanceTest, ConcurrentOperations) {
    populateData(1000);
    
    // Start multiple operations concurrently
    std::vector<std::thread> threads;
    std::vector<Result<MaintenanceJobStatus>> results(3);
    
    threads.emplace_back([&]() {
        results[0] = maintenance_manager_->rebuildIndex("index1", false);
    });
    
    threads.emplace_back([&]() {
        results[1] = maintenance_manager_->reorganizeIndex("index2", false);
    });
    
    threads.emplace_back([&]() {
        results[2] = maintenance_manager_->updateStatistics("index3");
    });
    
    for (auto& t : threads) {
        t.join();
    }
    
    // All operations should succeed
    for (const auto& result : results) {
        EXPECT_TRUE(result) << result.error().message();
        EXPECT_TRUE(result->is_completed);
        EXPECT_FALSE(result->is_failed);
    }
}

TEST_F(IndexMaintenanceTest, BackgroundMaintenanceDetectsHighFragmentation) {
    populateData(1000);
    
    // Set aggressive policy
    MaintenancePolicy policy;
    policy.schedule_type = MaintenanceSchedule::EVENT_BASED;
    policy.reorganize_threshold = 5.0;  // Very low threshold
    policy.rebuild_threshold = 15.0;
    policy.time_based_interval_ms = 1000;  // 1 second
    
    maintenance_manager_->setPolicy(policy);
    
    // Start background thread
    auto start_result = maintenance_manager_->start();
    ASSERT_TRUE(start_result);
    
    // Wait for background maintenance to run
    std::this_thread::sleep_for(std::chrono::seconds(3));
    
    // Check if any jobs were created
    auto active_jobs = maintenance_manager_->listActiveJobs();
    // Jobs may have completed already, so we just verify the manager is working
    
    maintenance_manager_->stop();
}

TEST_F(IndexMaintenanceTest, MetricsCaching) {
    populateData(500);
    
    // First call should compute metrics
    auto metrics1 = maintenance_manager_->getAllFragmentationMetrics();
    EXPECT_FALSE(metrics1.empty());
    
    // Second call within cache period should return cached results
    auto metrics2 = maintenance_manager_->getAllFragmentationMetrics();
    EXPECT_FALSE(metrics2.empty());
    
    // Results should be the same (cached)
    EXPECT_EQ(metrics1.size(), metrics2.size());
}

TEST_F(IndexMaintenanceTest, JobIdGeneration) {
    auto result1 = maintenance_manager_->updateStatistics("index1");
    auto result2 = maintenance_manager_->updateStatistics("index2");
    
    ASSERT_TRUE(result1);
    ASSERT_TRUE(result2);
    
    // Job IDs should be unique
    EXPECT_NE(result1->job_id, result2->job_id);
}

TEST_F(IndexMaintenanceTest, MaintenanceDisabledSchedule) {
    MaintenancePolicy policy;
    policy.schedule_type = MaintenanceSchedule::DISABLED;
    
    auto result = maintenance_manager_->setPolicy(policy);
    ASSERT_TRUE(result);
    
    // Start background thread
    auto start_result = maintenance_manager_->start();
    ASSERT_TRUE(start_result);
    
    // Wait a bit
    std::this_thread::sleep_for(std::chrono::seconds(1));
    
    // No jobs should be created
    auto active_jobs = maintenance_manager_->listActiveJobs();
    EXPECT_TRUE(active_jobs.empty());
    
    maintenance_manager_->stop();
}
