/**
 * @file test_task_scheduler_dynamic_scaling.cpp
 * @brief Unit tests for TaskScheduler dynamic concurrency scaling (Issue #2269).
 *
 * Tests cover:
 *  - Dynamic scaling disabled: concurrency limit stays at Config::max_concurrent_tasks
 *  - Dynamic scaling disabled: getQueueDepth always returns 0
 *  - Dynamic scaling: getDynamicConcurrencyLimit reflects Config value initially
 *  - Dynamic scaling: scale-up when pending queue >= scale_up_queue_depth
 *  - Dynamic scaling: scale-up is capped at max_concurrent_tasks_ceil
 *  - Dynamic scaling: scale-down after scale_down_idle_ticks idle ticks
 *  - Dynamic scaling: scale-down floor is min_concurrent_tasks
 *  - exportMetrics() includes concurrency_limit and queue_depth gauges
 *  - Config defaults for dynamic scaling fields
 *  - getQueueDepth / getDynamicConcurrencyLimit do not require the scheduler to be running
 */

#include <gtest/gtest.h>
#include "scheduler/task_scheduler.h"
#include "storage/rocksdb_wrapper.h"
#include "index/secondary_index.h"
#include "query/query_engine.h"

#include <atomic>
#include <chrono>
#include <filesystem>
#include <string>
#include <thread>

using namespace themis;
using namespace std::chrono_literals;

// ---------------------------------------------------------------------------
// Test fixture
// ---------------------------------------------------------------------------

class DynamicScalingTest : public ::testing::Test {
protected:
    static std::string makeDbPath() {
        auto ns = std::chrono::high_resolution_clock::now().time_since_epoch().count();
        return (std::filesystem::temp_directory_path() /
                std::filesystem::path("themis_dscale_" + std::to_string(ns))).string();
    }

    void SetUp() override {
        db_path_ = makeDbPath();
        std::filesystem::create_directories(db_path_);

        RocksDBWrapper::Config cfg;
        cfg.db_path    = db_path_ + "/db";
        cfg.enable_blobdb = false;
        storage_ = std::make_unique<RocksDBWrapper>(cfg);
        ASSERT_TRUE(storage_->open());
        idx_    = std::make_unique<SecondaryIndexManager>(*storage_);
        engine_ = std::make_unique<QueryEngine>(*storage_, *idx_);
    }

    void TearDown() override {
        if (scheduler_) {
            scheduler_->stop();
            scheduler_.reset();
        }
        engine_.reset();
        idx_.reset();
        storage_->close();
        storage_.reset();
        std::filesystem::remove_all(db_path_);
    }

    /// Create a scheduler with dynamic scaling enabled.
    void makeScheduler(bool enable_scaling,
                       size_t max_concurrent      = 2,
                       size_t min_concurrent      = 1,
                       size_t ceil_concurrent     = 8,
                       size_t scale_up_depth      = 2,
                       size_t scale_down_ticks    = 2,
                       std::chrono::milliseconds tick = 30ms)
    {
        TaskScheduler::Config cfg;
        cfg.max_concurrent_tasks      = max_concurrent;
        cfg.check_interval            = tick;
        cfg.persist_tasks             = false;
        cfg.enable_audit_logging      = false;
        cfg.enable_anomaly_detection  = false;
        cfg.enable_dynamic_scaling    = enable_scaling;
        cfg.min_concurrent_tasks      = min_concurrent;
        cfg.max_concurrent_tasks_ceil = ceil_concurrent;
        cfg.scale_up_queue_depth      = scale_up_depth;
        cfg.scale_down_idle_ticks     = scale_down_ticks;
        scheduler_ = std::make_unique<TaskScheduler>(engine_.get(), cfg);
    }

    /// Build a FUNCTION-type task that blocks for @p hold_ms then returns.
    static ScheduledTask makeBlockingTask(const std::string& id,
                                          const std::string& fn_name)
    {
        ScheduledTask t;
        t.id            = id;
        t.name          = id;
        t.type          = ScheduledTask::TaskType::FUNCTION;
        t.function_name = fn_name;
        // Trigger every second (won't matter in tests where we call executeTaskNow)
        t.interval      = std::chrono::seconds(9999);
        t.enabled       = true;
        return t;
    }

    std::string                        db_path_;
    std::unique_ptr<RocksDBWrapper>    storage_;
    std::unique_ptr<SecondaryIndexManager> idx_;
    std::unique_ptr<QueryEngine>       engine_;
    std::unique_ptr<TaskScheduler>     scheduler_;
};

// ---------------------------------------------------------------------------
// Config defaults
// ---------------------------------------------------------------------------

TEST_F(DynamicScalingTest, ConfigDefaults_DynamicScalingDisabled) {
    TaskScheduler::Config cfg;
    EXPECT_FALSE(cfg.enable_dynamic_scaling);
    EXPECT_EQ(1u,  cfg.min_concurrent_tasks);
    EXPECT_EQ(16u, cfg.max_concurrent_tasks_ceil);
    EXPECT_EQ(2u,  cfg.scale_up_queue_depth);
    EXPECT_EQ(3u,  cfg.scale_down_idle_ticks);
}

// ---------------------------------------------------------------------------
// When scaling disabled: limit is fixed; queue depth always 0
// ---------------------------------------------------------------------------

TEST_F(DynamicScalingTest, ScalingDisabled_LimitFixed) {
    makeScheduler(false, /*max*/ 4);
    EXPECT_EQ(4u, scheduler_->getDynamicConcurrencyLimit());
}

TEST_F(DynamicScalingTest, ScalingDisabled_QueueDepthAlwaysZero) {
    makeScheduler(false, /*max*/ 4);
    scheduler_->start();
    std::this_thread::sleep_for(100ms);
    EXPECT_EQ(0u, scheduler_->getQueueDepth());
}

// ---------------------------------------------------------------------------
// Initial limit equals Config::max_concurrent_tasks
// ---------------------------------------------------------------------------

TEST_F(DynamicScalingTest, ScalingEnabled_InitialLimitFromConfig) {
    makeScheduler(true, /*max*/ 3);
    EXPECT_EQ(3u, scheduler_->getDynamicConcurrencyLimit());
}

// ---------------------------------------------------------------------------
// Scale-up: when tasks are pending the limit grows
// ---------------------------------------------------------------------------

TEST_F(DynamicScalingTest, ScaleUp_LimitIncreasesWhenTasksPending) {
    // max=1 so only 1 task can run; scale-up depth=1; ceil=8
    makeScheduler(true,
                  /*max*/           1,
                  /*min*/           1,
                  /*ceil*/          8,
                  /*scale_up*/      1,
                  /*scale_down*/    10,
                  /*tick*/          30ms);

    std::atomic<int> blocker_latch{0};
    std::atomic<int> running_count{0};

    // Register a slow function that blocks until we release the latch
    scheduler_->registerFunction("slow_fn", [&](const nlohmann::json&) -> nlohmann::json {
        ++running_count;
        while (blocker_latch.load() == 0) {
            std::this_thread::sleep_for(5ms);
        }
        --running_count;
        return {};
    });

    // Register more tasks than the initial limit of 1
    for (int i = 0; i < 4; ++i) {
        auto t = makeBlockingTask("task_" + std::to_string(i), "slow_fn");
        t.interval = std::chrono::seconds(1);  // fire immediately on first tick
        t.next_run = std::chrono::system_clock::now();
        scheduler_->registerTask(t);
    }

    scheduler_->start();

    // Wait a few ticks for the scheduler to observe the pending queue and scale up.
    std::this_thread::sleep_for(300ms);

    size_t new_limit = scheduler_->getDynamicConcurrencyLimit();

    // Release tasks so the scheduler can clean up
    blocker_latch.store(1);
    std::this_thread::sleep_for(100ms);

    EXPECT_GT(new_limit, 1u) << "Limit should have grown above initial 1";
    EXPECT_LE(new_limit, 8u) << "Limit must not exceed ceil";
}

// ---------------------------------------------------------------------------
// Scale-up is capped at max_concurrent_tasks_ceil
// ---------------------------------------------------------------------------

TEST_F(DynamicScalingTest, ScaleUp_CappedAtCeiling) {
    // ceil=2, start=1, depth=1, ticks=100 (we only run a few ticks)
    makeScheduler(true,
                  /*max*/           1,
                  /*min*/           1,
                  /*ceil*/          2,
                  /*scale_up*/      1,
                  /*scale_down*/    100,
                  /*tick*/          20ms);

    std::atomic<int> blocker{0};
    scheduler_->registerFunction("slow_fn", [&](const nlohmann::json&) -> nlohmann::json {
        while (blocker.load() == 0) std::this_thread::sleep_for(5ms);
        return {};
    });

    for (int i = 0; i < 6; ++i) {
        ScheduledTask t = makeBlockingTask("t_" + std::to_string(i), "slow_fn");
        t.interval = std::chrono::seconds(1);
        t.next_run = std::chrono::system_clock::now();
        scheduler_->registerTask(t);
    }

    scheduler_->start();
    std::this_thread::sleep_for(400ms);

    size_t limit = scheduler_->getDynamicConcurrencyLimit();
    blocker.store(1);
    std::this_thread::sleep_for(100ms);

    EXPECT_LE(limit, 2u) << "Limit must not exceed ceiling of 2";
}

// ---------------------------------------------------------------------------
// Scale-down: after idle ticks the limit drops back toward min
// ---------------------------------------------------------------------------

TEST_F(DynamicScalingTest, ScaleDown_LimitDecreasesAfterIdleTicks) {
    // max=1 → scale-up to ≥2 → then stop registering tasks → scale-down after 2 idle ticks
    makeScheduler(true,
                  /*max*/           1,
                  /*min*/           1,
                  /*ceil*/          8,
                  /*scale_up*/      1,
                  /*scale_down*/    2,   // scale down after just 2 idle ticks
                  /*tick*/          30ms);

    std::atomic<bool> release{false};
    scheduler_->registerFunction("slow_fn", [&](const nlohmann::json&) -> nlohmann::json {
        while (!release.load()) std::this_thread::sleep_for(5ms);
        return {};
    });

    // Register 4 tasks to trigger scale-up
    for (int i = 0; i < 4; ++i) {
        ScheduledTask t = makeBlockingTask("t_" + std::to_string(i), "slow_fn");
        t.interval = std::chrono::seconds(1);
        t.next_run = std::chrono::system_clock::now();
        scheduler_->registerTask(t);
    }

    scheduler_->start();
    // Allow scale-up with bounded polling to avoid timing flakiness on slower CI hosts.
    size_t limit_after_scaleup = scheduler_->getDynamicConcurrencyLimit();
    for (int i = 0; i < 40 && limit_after_scaleup <= 1u; ++i) {
        std::this_thread::sleep_for(25ms);
        limit_after_scaleup = scheduler_->getDynamicConcurrencyLimit();
    }

    // Release all tasks and remove them so queue empties
    release.store(true);
    std::this_thread::sleep_for(100ms);

    // Disable all tasks so the pending count stays at 0
    for (int i = 0; i < 4; ++i) {
        scheduler_->disableTask("t_" + std::to_string(i));
    }

    // Wait for scale-down with bounded polling.
    size_t limit_after_scaledown = scheduler_->getDynamicConcurrencyLimit();
    for (int i = 0; i < 40 && limit_after_scaledown >= limit_after_scaleup; ++i) {
        std::this_thread::sleep_for(25ms);
        limit_after_scaledown = scheduler_->getDynamicConcurrencyLimit();
    }

    EXPECT_GT(limit_after_scaleup,  1u) << "Should have scaled up first";
    EXPECT_LT(limit_after_scaledown, limit_after_scaleup)
        << "Limit should have decreased after idle ticks";
    EXPECT_GE(limit_after_scaledown, 1u) << "Limit must not fall below min_concurrent_tasks=1";
}

// ---------------------------------------------------------------------------
// Scale-down floor is min_concurrent_tasks
// ---------------------------------------------------------------------------

TEST_F(DynamicScalingTest, ScaleDown_FloorIsMinConcurrentTasks) {
    makeScheduler(true,
                  /*max*/           2,
                  /*min*/           2,   // min == max, so no scale-down possible
                  /*ceil*/          8,
                  /*scale_up*/      2,
                  /*scale_down*/    1,
                  /*tick*/          20ms);

    scheduler_->start();
    // Many idle ticks
    std::this_thread::sleep_for(200ms);

    size_t limit = scheduler_->getDynamicConcurrencyLimit();
    EXPECT_GE(limit, 2u) << "Limit must not fall below min_concurrent_tasks=2";
}

// ---------------------------------------------------------------------------
// exportMetrics() includes scaling gauges
// ---------------------------------------------------------------------------

TEST_F(DynamicScalingTest, ExportMetrics_ContainsScalingGauges) {
    makeScheduler(false, /*max*/ 5);
    scheduler_->start();
    std::this_thread::sleep_for(60ms);

    std::string metrics = scheduler_->exportMetrics();
    EXPECT_NE(std::string::npos, metrics.find("themis_scheduler_concurrency_limit"))
        << "Expected themis_scheduler_concurrency_limit gauge";
    EXPECT_NE(std::string::npos, metrics.find("themis_scheduler_queue_depth"))
        << "Expected themis_scheduler_queue_depth gauge";
    // The static limit should be emitted as 5
    EXPECT_NE(std::string::npos, metrics.find("themis_scheduler_concurrency_limit 5"))
        << "Concurrency limit gauge should equal Config::max_concurrent_tasks when scaling disabled";
}

// ---------------------------------------------------------------------------
// Accessors available before start()
// ---------------------------------------------------------------------------

TEST_F(DynamicScalingTest, Accessors_WorkBeforeStart) {
    makeScheduler(true, /*max*/ 3, /*min*/ 1, /*ceil*/ 10);
    EXPECT_EQ(3u, scheduler_->getDynamicConcurrencyLimit());
    EXPECT_EQ(0u, scheduler_->getQueueDepth());
}
