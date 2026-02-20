/**
 * @file test_task_scheduler.cpp
 * @brief Unit tests for TaskScheduler core functionality
 */

#include <gtest/gtest.h>
#include "scheduler/task_scheduler.h"
#include "utils/cron_parser.h"
#include "storage/rocksdb_wrapper.h"
#include "index/secondary_index.h"
#include "query/query_engine.h"
#include <filesystem>
#include <atomic>
#include <thread>
#include <chrono>

using namespace themis;
using namespace std::chrono_literals;

// ===== Test fixture =====

class TaskSchedulerTest : public ::testing::Test {
protected:
    static std::string makeDbPath() {
        auto now = std::chrono::high_resolution_clock::now().time_since_epoch().count();
        return (std::filesystem::temp_directory_path() /
                std::filesystem::path("themis_sched_test_" + std::to_string(now))).string();
    }

    void SetUp() override {
        db_path_ = makeDbPath();
        std::filesystem::create_directories(db_path_);

        RocksDBWrapper::Config cfg;
        cfg.db_path = db_path_ + "/db";
        cfg.enable_blobdb = false;
        storage_ = std::make_unique<RocksDBWrapper>(cfg);
        ASSERT_TRUE(storage_->open());

        idx_ = std::make_unique<SecondaryIndexManager>(*storage_);
        engine_ = std::make_unique<QueryEngine>(*storage_, *idx_);

        TaskScheduler::Config sched_cfg;
        sched_cfg.max_concurrent_tasks = 4;
        sched_cfg.check_interval = 50ms;
        sched_cfg.persist_tasks = false;
        sched_cfg.enable_audit_logging = false;
        sched_cfg.enable_anomaly_detection = false;

        scheduler_ = std::make_unique<TaskScheduler>(engine_.get(), sched_cfg);
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

    std::string db_path_;
    std::unique_ptr<RocksDBWrapper> storage_;
    std::unique_ptr<SecondaryIndexManager> idx_;
    std::unique_ptr<QueryEngine> engine_;
    std::unique_ptr<TaskScheduler> scheduler_;
};

// ===== Lifecycle tests =====

TEST_F(TaskSchedulerTest, StartStop) {
    EXPECT_FALSE(scheduler_->isRunning());
    scheduler_->start();
    EXPECT_TRUE(scheduler_->isRunning());
    scheduler_->stop();
    EXPECT_FALSE(scheduler_->isRunning());
}

TEST_F(TaskSchedulerTest, DoubleStartIsNoop) {
    scheduler_->start();
    EXPECT_NO_THROW(scheduler_->start());  // Second start should be a no-op
    EXPECT_TRUE(scheduler_->isRunning());
}

// ===== Function registration =====

TEST_F(TaskSchedulerTest, RegisterAndUnregisterFunction) {
    int call_count = 0;
    scheduler_->registerFunction("test_fn", [&](const nlohmann::json&) {
        ++call_count;
        return nlohmann::json{{"ok", true}};
    });
    scheduler_->unregisterFunction("test_fn");
    // After unregistration, executeTaskNow with a function task should fail
    ScheduledTask t;
    t.id = "fn_task";
    t.name = "fn_task";
    t.type = ScheduledTask::TaskType::FUNCTION;
    t.function_name = "test_fn";
    t.trigger_type = ScheduledTask::TriggerType::MANUAL;
    scheduler_->registerTask(t);
    auto result = scheduler_->executeTaskNow("fn_task");
    EXPECT_TRUE(result.contains("error"));
}

// ===== Task registration =====

TEST_F(TaskSchedulerTest, RegisterIntervalTask) {
    ScheduledTask task;
    task.name = "interval_task";
    task.type = ScheduledTask::TaskType::FUNCTION;
    task.function_name = "noop";
    task.trigger_type = ScheduledTask::TriggerType::INTERVAL;
    task.interval = 5min;

    std::string id = scheduler_->registerTask(task);
    EXPECT_FALSE(id.empty());

    auto registered = scheduler_->getTask(id);
    ASSERT_NE(registered, nullptr);
    EXPECT_EQ(registered->name, "interval_task");
}

TEST_F(TaskSchedulerTest, RegisterCronTask) {
    ScheduledTask task;
    task.name = "cron_task";
    task.type = ScheduledTask::TaskType::FUNCTION;
    task.function_name = "noop";
    task.trigger_type = ScheduledTask::TriggerType::CRON;
    task.cron_expression = "*/5 * * * *";

    EXPECT_NO_THROW({
        std::string id = scheduler_->registerTask(task);
        EXPECT_FALSE(id.empty());
    });
}

TEST_F(TaskSchedulerTest, RegisterCronSpecialExpression) {
    ScheduledTask task;
    task.name = "daily_task";
    task.type = ScheduledTask::TaskType::FUNCTION;
    task.function_name = "noop";
    task.trigger_type = ScheduledTask::TriggerType::CRON;
    task.cron_expression = "@daily";

    EXPECT_NO_THROW({
        std::string id = scheduler_->registerTask(task);
        EXPECT_FALSE(id.empty());
    });
}

TEST_F(TaskSchedulerTest, RegisterCronInvalidExpressionThrows) {
    ScheduledTask task;
    task.name = "bad_cron";
    task.type = ScheduledTask::TaskType::FUNCTION;
    task.function_name = "noop";
    task.trigger_type = ScheduledTask::TriggerType::CRON;
    task.cron_expression = "not a valid cron";

    EXPECT_THROW(scheduler_->registerTask(task), std::invalid_argument);
}

TEST_F(TaskSchedulerTest, UnregisterTask) {
    ScheduledTask task;
    task.name = "task_to_remove";
    task.type = ScheduledTask::TaskType::FUNCTION;
    task.function_name = "noop";
    task.trigger_type = ScheduledTask::TriggerType::MANUAL;

    std::string id = scheduler_->registerTask(task);
    ASSERT_FALSE(id.empty());

    scheduler_->unregisterTask(id);
    EXPECT_EQ(scheduler_->getTask(id), nullptr);
}

TEST_F(TaskSchedulerTest, EnableDisableTask) {
    ScheduledTask task;
    task.name = "toggle_task";
    task.type = ScheduledTask::TaskType::FUNCTION;
    task.function_name = "noop";
    task.trigger_type = ScheduledTask::TriggerType::MANUAL;

    std::string id = scheduler_->registerTask(task);

    scheduler_->disableTask(id);
    auto t = scheduler_->getTask(id);
    ASSERT_NE(t, nullptr);
    EXPECT_FALSE(t->enabled);

    scheduler_->enableTask(id);
    t = scheduler_->getTask(id);
    ASSERT_NE(t, nullptr);
    EXPECT_TRUE(t->enabled);
}

// ===== Task execution =====

TEST_F(TaskSchedulerTest, ExecuteTaskNow) {
    std::atomic<int> call_count{0};
    scheduler_->registerFunction("count_fn", [&](const nlohmann::json&) {
        ++call_count;
        return nlohmann::json{{"status", "ok"}};
    });

    ScheduledTask task;
    task.name = "exec_task";
    task.type = ScheduledTask::TaskType::FUNCTION;
    task.function_name = "count_fn";
    task.trigger_type = ScheduledTask::TriggerType::MANUAL;

    std::string id = scheduler_->registerTask(task);

    auto result = scheduler_->executeTaskNow(id);
    EXPECT_FALSE(result.contains("error"));
    EXPECT_EQ(call_count.load(), 1);
}

TEST_F(TaskSchedulerTest, ExecuteNonExistentTaskReturnsError) {
    auto result = scheduler_->executeTaskNow("does_not_exist");
    EXPECT_TRUE(result.contains("error"));
}

TEST_F(TaskSchedulerTest, ExecuteTaskNowUpdatesCounts) {
    scheduler_->registerFunction("noop", [](const nlohmann::json&) {
        return nlohmann::json{{"ok", true}};
    });

    ScheduledTask task;
    task.name = "stats_task";
    task.type = ScheduledTask::TaskType::FUNCTION;
    task.function_name = "noop";
    task.trigger_type = ScheduledTask::TriggerType::MANUAL;

    std::string id = scheduler_->registerTask(task);
    scheduler_->executeTaskNow(id);
    scheduler_->executeTaskNow(id);

    auto t = scheduler_->getTask(id);
    ASSERT_NE(t, nullptr);
    EXPECT_EQ(t->total_executions, 2u);
    EXPECT_EQ(t->successful_executions, 2u);
}

// ===== Statistics =====

TEST_F(TaskSchedulerTest, GetStatsReflectsRegisteredTasks) {
    auto stats_before = scheduler_->getStats();
    EXPECT_EQ(stats_before.registered_tasks, 0u);

    ScheduledTask task;
    task.name = "t1";
    task.type = ScheduledTask::TaskType::FUNCTION;
    task.function_name = "noop";
    task.trigger_type = ScheduledTask::TriggerType::MANUAL;
    scheduler_->registerTask(task);

    auto stats_after = scheduler_->getStats();
    EXPECT_EQ(stats_after.registered_tasks, 1u);
}

TEST_F(TaskSchedulerTest, ListTasksReturnsAll) {
    ScheduledTask t1;
    t1.name = "task_a";
    t1.type = ScheduledTask::TaskType::FUNCTION;
    t1.function_name = "noop";
    t1.trigger_type = ScheduledTask::TriggerType::MANUAL;

    ScheduledTask t2;
    t2.name = "task_b";
    t2.type = ScheduledTask::TaskType::FUNCTION;
    t2.function_name = "noop";
    t2.trigger_type = ScheduledTask::TriggerType::MANUAL;

    scheduler_->registerTask(t1);
    scheduler_->registerTask(t2);

    auto tasks = scheduler_->listTasks();
    EXPECT_EQ(tasks.size(), 2u);
}

// ===== Automatic execution via scheduler loop =====

TEST_F(TaskSchedulerTest, IntervalTaskExecutesAfterInterval) {
    std::atomic<int> count{0};
    scheduler_->registerFunction("increment", [&](const nlohmann::json&) {
        ++count;
        return nlohmann::json{};
    });

    ScheduledTask task;
    task.name = "auto_task";
    task.type = ScheduledTask::TaskType::FUNCTION;
    task.function_name = "increment";
    task.trigger_type = ScheduledTask::TriggerType::INTERVAL;
    task.interval = 80ms;
    // Schedule for immediate execution
    task.next_run = std::chrono::system_clock::now();

    scheduler_->registerTask(task);
    scheduler_->start();

    // Wait a bit more than one interval
    std::this_thread::sleep_for(250ms);
    scheduler_->stop();

    EXPECT_GE(count.load(), 1);
}

// ===== Security validation =====

TEST_F(TaskSchedulerTest, EmptyAqlQueryThrows) {
    ScheduledTask task;
    task.name = "empty_aql";
    task.type = ScheduledTask::TaskType::AQL_QUERY;
    task.aql_query = "";
    task.trigger_type = ScheduledTask::TriggerType::MANUAL;

    EXPECT_THROW(scheduler_->registerTask(task), std::invalid_argument);
}

TEST_F(TaskSchedulerTest, ExcessiveTimeoutThrows) {
    ScheduledTask task;
    task.name = "long_timeout";
    task.type = ScheduledTask::TaskType::FUNCTION;
    task.function_name = "noop";
    task.trigger_type = ScheduledTask::TriggerType::MANUAL;
    task.timeout = std::chrono::hours(48);  // Exceeds 24h limit

    EXPECT_THROW(scheduler_->registerTask(task), std::invalid_argument);
}

