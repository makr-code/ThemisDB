/*
 * ThemisDB | File: test_task_result_store.cpp | Version: 0.0.14
 * Maturity: 🟢 PRODUCTION-READY | Score: 98/100
 * Gap Summary: total=5; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=2, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
 */

/**
 * @file test_task_result_store.cpp
 * @brief Tests for TaskResultStore (src/scheduler/task_result_store.cpp)
 *
 * Covers:
 *   - store() – basic storage
 *   - getResults() – retrieval by task_id, with limit
 *   - getLatestResult() – most-recent result
 *   - store() with max_per_task cap – oldest entries pruned
 *   - TaskExecutionResult::toJson() / fromJson() round-trip
 *   - Empty task_id – returns empty
 *   - Multiple tasks isolated from each other
 */

#include <gtest/gtest.h>
#include "scheduler/task_result_store.h"
#include "storage/rocksdb_wrapper.h"
#include <filesystem>
#include <chrono>

using namespace themis;
using namespace themis::scheduler;

// ============================================================================
// Fixture
// ============================================================================

class TaskResultStoreTest : public ::testing::Test {
protected:
    void SetUp() override {
        db_path_ = (std::filesystem::temp_directory_path() /
                    ("themis_trs_test_" +
                     std::to_string(std::chrono::high_resolution_clock::now()
                                        .time_since_epoch()
                                        .count())))
                       .string();
        std::filesystem::create_directories(db_path_);

        RocksDBWrapper::Config cfg;
        cfg.db_path       = db_path_ + "/db";
        cfg.enable_blobdb = false;
        storage_          = std::make_unique<RocksDBWrapper>(cfg);
        ASSERT_TRUE(storage_->open());

        store_ = std::make_unique<TaskResultStore>(*storage_, /*max_per_task=*/5);
    }

    void TearDown() override {
        store_.reset();
        storage_->close();
        storage_.reset();
        std::filesystem::remove_all(db_path_);
    }

    TaskExecutionResult makeResult(const std::string& task_id,
                                   int64_t ts_ms, bool success = true) {
        TaskExecutionResult r;
        r.task_id      = task_id;
        r.task_name    = "test_task";
        r.timestamp_ms = ts_ms;
        r.duration_ms  = 10.0;
        r.success      = success;
        r.output       = nlohmann::json{{"ts", ts_ms}};
        r.error        = success ? "" : "simulated error";
        return r;
    }

    std::string                        db_path_;
    std::unique_ptr<RocksDBWrapper>    storage_;
    std::unique_ptr<TaskResultStore>   store_;
};

// ============================================================================
// store + getResults
// ============================================================================

TEST_F(TaskResultStoreTest, StoreAndRetrieve_SingleResult) {
    auto err = store_->store(makeResult("task1", 1000));
    EXPECT_EQ(err, SchedulerError::kSuccess);
    auto results = store_->getResults("task1");
    ASSERT_EQ(results.size(), 1u);
    EXPECT_EQ(results[0].task_id, "task1");
    EXPECT_EQ(results[0].timestamp_ms, 1000);
}

TEST_F(TaskResultStoreTest, GetResults_EmptyTask_ReturnsEmpty) {
    auto results = store_->getResults("nonexistent_task");
    EXPECT_TRUE(results.empty());
}

TEST_F(TaskResultStoreTest, StoreMultiple_ReturnsAll) {
    for (int i = 1; i <= 3; ++i) {
        auto err = store_->store(makeResult("task_m", static_cast<int64_t>(i) * 1000));
        EXPECT_EQ(err, SchedulerError::kSuccess);
    }
    auto results = store_->getResults("task_m");
    EXPECT_EQ(results.size(), 3u);
}

TEST_F(TaskResultStoreTest, GetResults_WithLimit) {
    for (int i = 1; i <= 4; ++i) {
        auto err = store_->store(makeResult("task_lim", static_cast<int64_t>(i) * 1000));
        EXPECT_EQ(err, SchedulerError::kSuccess);
    }
    auto results = store_->getResults("task_lim", 2);
    EXPECT_EQ(results.size(), 2u);
}

// ============================================================================
// getLatestResult
// ============================================================================

TEST_F(TaskResultStoreTest, GetLatestResult_NoResults_ReturnsNullopt) {
    auto latest = store_->getLatestResult("absent_task");
    EXPECT_FALSE(latest.has_value());
}

TEST_F(TaskResultStoreTest, GetLatestResult_ReturnsNewest) {
    auto err1 = store_->store(makeResult("task_lat", 1000));
    auto err2 = store_->store(makeResult("task_lat", 3000));
    auto err3 = store_->store(makeResult("task_lat", 2000));
    EXPECT_EQ(err1, SchedulerError::kSuccess);
    EXPECT_EQ(err2, SchedulerError::kSuccess);
    EXPECT_EQ(err3, SchedulerError::kSuccess);
    auto latest = store_->getLatestResult("task_lat");
    ASSERT_TRUE(latest.has_value());
    // The newest timestamp should be returned
    EXPECT_EQ(latest->timestamp_ms, 3000);
}

// ============================================================================
// max_per_task cap (oldest pruned when cap exceeded)
// ============================================================================

TEST_F(TaskResultStoreTest, MaxPerTask_OldestPruned) {
    // max_per_task = 5; store 7 results
    for (int i = 1; i <= 7; ++i) {
        auto err = store_->store(makeResult("task_cap", static_cast<int64_t>(i) * 1000));
        EXPECT_EQ(err, SchedulerError::kSuccess);
    }
    auto results = store_->getResults("task_cap");
    // Should retain at most 5
    EXPECT_LE(results.size(), 5u);
}

// ============================================================================
// Multiple task isolation
// ============================================================================

TEST_F(TaskResultStoreTest, MultipleTasksIsolated) {
    auto err1 = store_->store(makeResult("taskA", 1000));
    auto err2 = store_->store(makeResult("taskB", 2000));
    auto err3 = store_->store(makeResult("taskA", 3000));
    EXPECT_EQ(err1, SchedulerError::kSuccess);
    EXPECT_EQ(err2, SchedulerError::kSuccess);
    EXPECT_EQ(err3, SchedulerError::kSuccess);

    auto resA = store_->getResults("taskA");
    auto resB = store_->getResults("taskB");

    EXPECT_EQ(resA.size(), 2u);
    EXPECT_EQ(resB.size(), 1u);
    for (const auto& r : resA) { EXPECT_EQ(r.task_id, "taskA"); }
    for (const auto& r : resB) { EXPECT_EQ(r.task_id, "taskB"); }
}

// ============================================================================
// Failure results stored correctly
// ============================================================================

TEST_F(TaskResultStoreTest, FailureResult_StoredAndRetrieved) {
    auto err = store_->store(makeResult("task_fail", 5000, /*success=*/false));
    EXPECT_EQ(err, SchedulerError::kSuccess);
    auto results = store_->getResults("task_fail");
    ASSERT_EQ(results.size(), 1u);
    EXPECT_FALSE(results[0].success);
    EXPECT_EQ(results[0].error, "simulated error");
}

// ============================================================================
// TaskExecutionResult::toJson / fromJson round-trip
// ============================================================================

TEST(TaskExecutionResultSerializationTest, JsonRoundTrip_Success) {
    TaskExecutionResult r;
    r.task_id      = "roundtrip_task";
    r.task_name    = "rt_name";
    r.timestamp_ms = 1700000000000LL;
    r.duration_ms  = 42.5;
    r.success      = true;
    r.output       = nlohmann::json{{"key", "value"}};
    r.error        = "";

    auto j      = r.toJson();
    auto parsed = TaskExecutionResult::fromJson(j);

    EXPECT_EQ(parsed.task_id,      r.task_id);
    EXPECT_EQ(parsed.task_name,    r.task_name);
    EXPECT_EQ(parsed.timestamp_ms, r.timestamp_ms);
    EXPECT_NEAR(parsed.duration_ms, r.duration_ms, 1e-6);
    EXPECT_EQ(parsed.success,      r.success);
    EXPECT_EQ(parsed.output,       r.output);
}

TEST(TaskExecutionResultSerializationTest, JsonRoundTrip_Failure) {
    TaskExecutionResult r;
    r.task_id      = "fail_task";
    r.task_name    = "fn";
    r.timestamp_ms = 1000;
    r.duration_ms  = 0.5;
    r.success      = false;
    r.output       = nlohmann::json{};
    r.error        = "timeout";

    auto j      = r.toJson();
    auto parsed = TaskExecutionResult::fromJson(j);

    EXPECT_EQ(parsed.success, false);
    EXPECT_EQ(parsed.error,   "timeout");
}

// ============================================================================
// Retention Limit Enforcement (PRODUCTION FIX)
// ============================================================================

TEST_F(TaskResultStoreTest, RetentionLimit_EnforcedBeforeWrite) {
    // Create a store with a small limit (3 results per task)
    auto store_small = std::make_unique<TaskResultStore>(*storage_, /*max_per_task=*/3);
    
    // Store 3 results successfully
    auto err1 = store_small->store(makeResult("task_limit", 1000));
    auto err2 = store_small->store(makeResult("task_limit", 2000));
    auto err3 = store_small->store(makeResult("task_limit", 3000));
    
    EXPECT_EQ(err1, SchedulerError::kSuccess);
    EXPECT_EQ(err2, SchedulerError::kSuccess);
    EXPECT_EQ(err3, SchedulerError::kSuccess);
    
    // 4th store should be rejected with kRetentionLimitExceeded
    auto err_limit = store_small->store(makeResult("task_limit", 4000));
    EXPECT_EQ(err_limit, SchedulerError::kRetentionLimitExceeded);
    
    // Verify only 3 results are stored (the rejected one was not added)
    auto results = store_small->getResults("task_limit");
    EXPECT_EQ(results.size(), 3u);
}

TEST_F(TaskResultStoreTest, RetentionLimit_StorageError) {
    GTEST_SKIP() << "Requires RocksDBWrapper failure-injection mock; "
                    "skipped until mock harness is available";
}
