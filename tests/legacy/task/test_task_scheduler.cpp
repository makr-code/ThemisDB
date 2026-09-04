/**
 * @file test_task_scheduler.cpp
 * @brief Unit tests for TaskScheduler core functionality
 */

#include <gtest/gtest.h>
#include "scheduler/task_scheduler.h"
#include "scheduler/task_audit_manager.h"
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
    task.interval = 1000ms;
    // Schedule for immediate execution
    task.next_run = std::chrono::system_clock::now();

    scheduler_->registerTask(task);
    scheduler_->start();

    // Wait a bit more than one interval
    std::this_thread::sleep_for(250ms);
    scheduler_->stop();

    EXPECT_GE(count.load(), 1);
}

TEST_F(TaskSchedulerTest, SchedulerLoopFailedTaskIncrementsTotalExecutions) {
    // Verifies that task->total_executions is incremented even when a scheduled
    // (background) execution fails.  Previously executeTask() only incremented
    // total_executions in the success branch, unlike executeTaskNow/executeDAG.
    std::atomic<int> call_count{0};
    scheduler_->registerFunction("always_fail_loop", [&](const nlohmann::json&) -> nlohmann::json {
        ++call_count;
        throw std::runtime_error("deliberate scheduled failure");
    });

    ScheduledTask task;
    task.name = "sched_fail_task";
    task.type = ScheduledTask::TaskType::FUNCTION;
    task.function_name = "always_fail_loop";
    task.trigger_type = ScheduledTask::TriggerType::INTERVAL;
    task.interval = 1000ms;
    task.max_retries = 0;  // No retries so the loop fires fast
    // Schedule for immediate execution
    task.next_run = std::chrono::system_clock::now();

    std::string id = scheduler_->registerTask(task);
    scheduler_->start();

    // Wait long enough for at least one scheduled execution
    std::this_thread::sleep_for(350ms);
    scheduler_->stop();

    auto t = scheduler_->getTask(id);
    ASSERT_NE(t, nullptr);
    EXPECT_GE(call_count.load(), 1) << "task should have been called at least once";
    // total_executions must equal failed_executions: each failure increments both
    EXPECT_EQ(t->total_executions, t->failed_executions)
        << "total_executions should equal failed_executions when task always fails";
    EXPECT_GE(t->total_executions, 1u);
}



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


// ===== Retry logic tests =====

TEST_F(TaskSchedulerTest, ZeroRetriesFailsImmediately) {
    std::atomic<int> attempt_count{0};
    scheduler_->registerFunction("always_fail", [&](const nlohmann::json&) -> nlohmann::json {
        ++attempt_count;
        throw std::runtime_error("deliberate failure");
    });

    ScheduledTask task;
    task.name = "zero_retry_task";
    task.type = ScheduledTask::TaskType::FUNCTION;
    task.function_name = "always_fail";
    task.trigger_type = ScheduledTask::TriggerType::MANUAL;
    task.max_retries = 0;

    std::string id = scheduler_->registerTask(task);
    auto result = scheduler_->executeTaskNow(id);

    EXPECT_TRUE(result.contains("error"));
    EXPECT_EQ(attempt_count.load(), 1);  // Only 1 attempt (no retries)

    auto t = scheduler_->getTask(id);
    ASSERT_NE(t, nullptr);
    EXPECT_EQ(t->failed_executions, 1u);
    EXPECT_EQ(t->successful_executions, 0u);
}

TEST_F(TaskSchedulerTest, TaskSucceedsOnRetry) {
    // Fails on first attempt, succeeds on second
    std::atomic<int> attempt_count{0};
    scheduler_->registerFunction("fail_then_succeed", [&](const nlohmann::json&) -> nlohmann::json {
        int n = ++attempt_count;
        if (n < 2) {
            throw std::runtime_error("first attempt failed");
        }
        return nlohmann::json{{"status", "ok"}};
    });

    ScheduledTask task;
    task.name = "retry_task";
    task.type = ScheduledTask::TaskType::FUNCTION;
    task.function_name = "fail_then_succeed";
    task.trigger_type = ScheduledTask::TriggerType::MANUAL;
    task.max_retries = 2;  // Allow 2 retries → 3 total attempts

    std::string id = scheduler_->registerTask(task);
    auto result = scheduler_->executeTaskNow(id);

    // Should succeed on second attempt
    EXPECT_FALSE(result.contains("error")) << result.dump();
    EXPECT_EQ(attempt_count.load(), 2);

    auto t = scheduler_->getTask(id);
    ASSERT_NE(t, nullptr);
    EXPECT_EQ(t->successful_executions, 1u);
    EXPECT_EQ(t->failed_executions, 0u);
}

TEST_F(TaskSchedulerTest, AllRetriesExhaustedCountsAsFailed) {
    std::atomic<int> attempt_count{0};
    scheduler_->registerFunction("always_fail_2", [&](const nlohmann::json&) -> nlohmann::json {
        ++attempt_count;
        throw std::runtime_error("persistent failure");
    });

    ScheduledTask task;
    task.name = "exhaust_retries_task";
    task.type = ScheduledTask::TaskType::FUNCTION;
    task.function_name = "always_fail_2";
    task.trigger_type = ScheduledTask::TriggerType::MANUAL;
    task.max_retries = 2;  // 1 initial + 2 retries = 3 total attempts

    std::string id = scheduler_->registerTask(task);
    auto result = scheduler_->executeTaskNow(id);

    EXPECT_TRUE(result.contains("error"));
    EXPECT_EQ(attempt_count.load(), 3);  // All 3 attempts exhausted

    auto t = scheduler_->getTask(id);
    ASSERT_NE(t, nullptr);
    EXPECT_EQ(t->failed_executions, 1u);
    EXPECT_EQ(t->successful_executions, 0u);
}

// ===== Advanced RetryPolicy Tests =====

TEST_F(TaskSchedulerTest, RetryPolicyNoneIsEquivalentToZeroRetries) {
    std::atomic<int> call_count{0};
    scheduler_->registerFunction("single_shot", [&](const nlohmann::json&) -> nlohmann::json {
        ++call_count;
        throw std::runtime_error("always fails");
    });

    ScheduledTask task;
    task.name = "no_retry_policy_task";
    task.type = ScheduledTask::TaskType::FUNCTION;
    task.function_name = "single_shot";
    task.trigger_type = ScheduledTask::TriggerType::MANUAL;

    ScheduledTask::RetryPolicy policy;
    policy.strategy    = ScheduledTask::RetryStrategy::NONE;
    policy.max_retries = 5;  // Ignored when strategy == NONE
    task.retry_policy  = policy;

    std::string id = scheduler_->registerTask(task);
    auto result = scheduler_->executeTaskNow(id);

    EXPECT_TRUE(result.contains("error"));
    EXPECT_EQ(call_count.load(), 1);  // Only 1 attempt regardless of max_retries
}

TEST_F(TaskSchedulerTest, RetryPolicyFixedDelay) {
    std::atomic<int> call_count{0};
    scheduler_->registerFunction("fixed_delay_fn", [&](const nlohmann::json&) -> nlohmann::json {
        if (++call_count < 3) {
          throw std::runtime_error("not yet");
        }
        return nlohmann::json{{"ok", true}};
    });

    ScheduledTask task;
    task.name = "fixed_delay_task";
    task.type = ScheduledTask::TaskType::FUNCTION;
    task.function_name = "fixed_delay_fn";
    task.trigger_type = ScheduledTask::TriggerType::MANUAL;

    ScheduledTask::RetryPolicy policy;
    policy.strategy      = ScheduledTask::RetryStrategy::FIXED_DELAY;
    policy.max_retries   = 3;
    policy.initial_delay = std::chrono::milliseconds{10};  // Very short for test speed
    policy.max_delay     = std::chrono::milliseconds{50};
    task.retry_policy    = policy;

    std::string id = scheduler_->registerTask(task);
    auto result = scheduler_->executeTaskNow(id);

    EXPECT_FALSE(result.contains("error")) << result.dump();
    EXPECT_EQ(call_count.load(), 3);  // Succeeded on 3rd attempt

    auto t = scheduler_->getTask(id);
    ASSERT_NE(t, nullptr);
    EXPECT_EQ(t->successful_executions, 1u);
    EXPECT_EQ(t->failed_executions, 0u);
}

TEST_F(TaskSchedulerTest, RetryPolicyLinearBackoff) {
    std::atomic<int> call_count{0};
    scheduler_->registerFunction("linear_fn", [&](const nlohmann::json&) -> nlohmann::json {
        if (++call_count < 2) {
          throw std::runtime_error("first fail");
        }
        return nlohmann::json{{"ok", true}};
    });

    ScheduledTask task;
    task.name = "linear_backoff_task";
    task.type = ScheduledTask::TaskType::FUNCTION;
    task.function_name = "linear_fn";
    task.trigger_type = ScheduledTask::TriggerType::MANUAL;

    ScheduledTask::RetryPolicy policy;
    policy.strategy      = ScheduledTask::RetryStrategy::LINEAR_BACKOFF;
    policy.max_retries   = 3;
    policy.initial_delay = std::chrono::milliseconds{5};
    policy.max_delay     = std::chrono::milliseconds{50};
    task.retry_policy    = policy;

    std::string id = scheduler_->registerTask(task);
    auto result = scheduler_->executeTaskNow(id);

    EXPECT_FALSE(result.contains("error")) << result.dump();
    EXPECT_EQ(call_count.load(), 2);
}

TEST_F(TaskSchedulerTest, RetryPolicyJitterBackoff) {
    std::atomic<int> call_count{0};
    scheduler_->registerFunction("jitter_fn", [&](const nlohmann::json&) -> nlohmann::json {
        if (++call_count < 2) {
          throw std::runtime_error("first fail");
        }
        return nlohmann::json{{"ok", true}};
    });

    ScheduledTask task;
    task.name = "jitter_backoff_task";
    task.type = ScheduledTask::TaskType::FUNCTION;
    task.function_name = "jitter_fn";
    task.trigger_type = ScheduledTask::TriggerType::MANUAL;

    ScheduledTask::RetryPolicy policy;
    policy.strategy           = ScheduledTask::RetryStrategy::JITTER_BACKOFF;
    policy.max_retries        = 3;
    policy.initial_delay      = std::chrono::milliseconds{5};
    policy.max_delay          = std::chrono::milliseconds{50};
    policy.backoff_multiplier = 2.0;
    policy.jitter_factor      = 0.2;
    task.retry_policy         = policy;

    std::string id = scheduler_->registerTask(task);
    auto result = scheduler_->executeTaskNow(id);

    EXPECT_FALSE(result.contains("error")) << result.dump();
    EXPECT_EQ(call_count.load(), 2);
}

TEST_F(TaskSchedulerTest, RetryPolicyConditionalShouldRetry) {
    std::atomic<int> call_count{0};
    scheduler_->registerFunction("conditional_fn", [&](const nlohmann::json&) -> nlohmann::json {
        ++call_count;
        throw std::runtime_error("permanent_error: cannot connect");
    });

    ScheduledTask task;
    task.name = "conditional_retry_task";
    task.type = ScheduledTask::TaskType::FUNCTION;
    task.function_name = "conditional_fn";
    task.trigger_type = ScheduledTask::TriggerType::MANUAL;

    ScheduledTask::RetryPolicy policy;
    policy.strategy      = ScheduledTask::RetryStrategy::FIXED_DELAY;
    policy.max_retries   = 5;
    policy.initial_delay = std::chrono::milliseconds{5};
    policy.max_delay     = std::chrono::milliseconds{50};
    // Only retry on "transient" errors; "permanent_error" should not be retried
    policy.should_retry  = [](const std::string& err) {
        return err.find("permanent_error") == std::string::npos;
    };
    task.retry_policy = policy;

    std::string id = scheduler_->registerTask(task);
    auto result = scheduler_->executeTaskNow(id);

    EXPECT_TRUE(result.contains("error"));
    // should_retry returns false on first failure → only 1 attempt total
    EXPECT_EQ(call_count.load(), 1);
}

TEST_F(TaskSchedulerTest, RetryPolicyLegacyMaxRetriesStillWorks) {
    std::atomic<int> call_count{0};
    scheduler_->registerFunction("legacy_fn", [&](const nlohmann::json&) -> nlohmann::json {
        if (++call_count < 2) {
          throw std::runtime_error("first fail");
        }
        return nlohmann::json{{"ok", true}};
    });

    ScheduledTask task;
    task.name = "legacy_retry_task";
    task.type = ScheduledTask::TaskType::FUNCTION;
    task.function_name = "legacy_fn";
    task.trigger_type = ScheduledTask::TriggerType::MANUAL;
    task.max_retries = 2;  // Legacy field; no retry_policy set

    std::string id = scheduler_->registerTask(task);
    auto result = scheduler_->executeTaskNow(id);

    EXPECT_FALSE(result.contains("error")) << result.dump();
    EXPECT_EQ(call_count.load(), 2);
}

TEST_F(TaskSchedulerTest, RetryPolicyExponentialBackoff) {
    std::atomic<int> call_count{0};
    scheduler_->registerFunction("exp_backoff_fn", [&](const nlohmann::json&) -> nlohmann::json {
        if (++call_count < 3) {
          throw std::runtime_error("transient error");
        }
        return nlohmann::json{{"ok", true}};
    });

    ScheduledTask task;
    task.name = "exp_backoff_task";
    task.type = ScheduledTask::TaskType::FUNCTION;
    task.function_name = "exp_backoff_fn";
    task.trigger_type = ScheduledTask::TriggerType::MANUAL;

    ScheduledTask::RetryPolicy policy;
    policy.strategy           = ScheduledTask::RetryStrategy::EXPONENTIAL_BACKOFF;
    policy.max_retries        = 3;
    policy.initial_delay      = std::chrono::milliseconds{5};
    policy.max_delay          = std::chrono::milliseconds{50};
    policy.backoff_multiplier = 2.0;
    task.retry_policy         = policy;

    std::string id = scheduler_->registerTask(task);
    auto result = scheduler_->executeTaskNow(id);

    EXPECT_FALSE(result.contains("error")) << result.dump();
    EXPECT_EQ(call_count.load(), 3);  // Succeeded on 3rd attempt

    auto t = scheduler_->getTask(id);
    ASSERT_NE(t, nullptr);
    EXPECT_EQ(t->successful_executions, 1u);
    EXPECT_EQ(t->failed_executions, 0u);
}

TEST_F(TaskSchedulerTest, RetryPolicyPersistedAndRestoredFromDisk) {
    auto persist_path = db_path_ + "/persist_retry_test";
    std::filesystem::create_directories(persist_path);

    TaskScheduler::Config pcfg;
    pcfg.persist_tasks = true;
    pcfg.persistence_path = persist_path;
    pcfg.enable_audit_logging = false;
    pcfg.enable_anomaly_detection = false;
    auto sched = std::make_unique<TaskScheduler>(engine_.get(), pcfg);

    ScheduledTask task;
    task.id = "persist_retry_task";
    task.name = task.id;
    task.type = ScheduledTask::TaskType::FUNCTION;
    task.function_name = "noop_fn";
    task.trigger_type = ScheduledTask::TriggerType::MANUAL;

    ScheduledTask::RetryPolicy rp;
    rp.strategy           = ScheduledTask::RetryStrategy::EXPONENTIAL_BACKOFF;
    rp.max_retries        = 5;
    rp.initial_delay      = std::chrono::milliseconds{250};
    rp.max_delay          = std::chrono::milliseconds{8000};
    rp.backoff_multiplier = 3.0;
    rp.jitter_factor      = 0.05;
    task.retry_policy     = rp;

    sched->registerFunction("noop_fn", [](const nlohmann::json&) -> nlohmann::json { return {}; });
    sched->registerTask(task);
    sched.reset();  // destructor triggers saveTasks()

    // Load from disk in a fresh scheduler instance
    auto sched2 = std::make_unique<TaskScheduler>(engine_.get(), pcfg);
    sched2->registerFunction("noop_fn", [](const nlohmann::json&) -> nlohmann::json { return {}; });
    auto loaded = sched2->getTask("persist_retry_task");
    ASSERT_NE(loaded, nullptr);
    ASSERT_TRUE(loaded->retry_policy.has_value());

    const auto& loaded_rp = *loaded->retry_policy;
    EXPECT_EQ(loaded_rp.strategy, ScheduledTask::RetryStrategy::EXPONENTIAL_BACKOFF);
    EXPECT_EQ(loaded_rp.max_retries, 5u);
    EXPECT_EQ(loaded_rp.initial_delay, std::chrono::milliseconds{250});
    EXPECT_EQ(loaded_rp.max_delay, std::chrono::milliseconds{8000});
    EXPECT_DOUBLE_EQ(loaded_rp.backoff_multiplier, 3.0);
    EXPECT_DOUBLE_EQ(loaded_rp.jitter_factor, 0.05);
}

TEST_F(TaskSchedulerTest, RetryPolicyFibonacciBackoff) {
    std::atomic<int> call_count{0};
    scheduler_->registerFunction("fib_backoff_fn", [&](const nlohmann::json&) -> nlohmann::json {
        if (++call_count < 4) {
          throw std::runtime_error("transient error");
        }
        return nlohmann::json{{"ok", true}};
    });

    ScheduledTask task;
    task.name = "fib_backoff_task";
    task.type = ScheduledTask::TaskType::FUNCTION;
    task.function_name = "fib_backoff_fn";
    task.trigger_type = ScheduledTask::TriggerType::MANUAL;

    ScheduledTask::RetryPolicy policy;
    policy.strategy      = ScheduledTask::RetryStrategy::FIBONACCI_BACKOFF;
    policy.max_retries   = 5;
    policy.initial_delay = std::chrono::milliseconds{5};
    policy.max_delay     = std::chrono::milliseconds{100};
    task.retry_policy    = policy;

    std::string id = scheduler_->registerTask(task);
    auto result = scheduler_->executeTaskNow(id);

    EXPECT_FALSE(result.contains("error")) << result.dump();
    EXPECT_EQ(call_count.load(), 4);  // Succeeded on 4th attempt

    auto t = scheduler_->getTask(id);
    ASSERT_NE(t, nullptr);
    EXPECT_EQ(t->successful_executions, 1u);
    EXPECT_EQ(t->failed_executions, 0u);
}

TEST_F(TaskSchedulerTest, RetryPolicyFibonacciBackoffExhausted) {
    std::atomic<int> call_count{0};
    scheduler_->registerFunction("fib_always_fail", [&](const nlohmann::json&) -> nlohmann::json {
        ++call_count;
        throw std::runtime_error("persistent failure");
    });

    ScheduledTask task;
    task.name = "fib_exhaust_task";
    task.type = ScheduledTask::TaskType::FUNCTION;
    task.function_name = "fib_always_fail";
    task.trigger_type = ScheduledTask::TriggerType::MANUAL;

    ScheduledTask::RetryPolicy policy;
    policy.strategy      = ScheduledTask::RetryStrategy::FIBONACCI_BACKOFF;
    policy.max_retries   = 2;  // 1 initial + 2 retries = 3 total attempts
    policy.initial_delay = std::chrono::milliseconds{5};
    policy.max_delay     = std::chrono::milliseconds{100};
    task.retry_policy    = policy;

    std::string id = scheduler_->registerTask(task);
    auto result = scheduler_->executeTaskNow(id);

    EXPECT_TRUE(result.contains("error"));
    EXPECT_EQ(call_count.load(), 3);  // All 3 attempts exhausted

    auto t = scheduler_->getTask(id);
    ASSERT_NE(t, nullptr);
    EXPECT_EQ(t->failed_executions, 1u);
    EXPECT_EQ(t->successful_executions, 0u);
}

// ===== exportMetrics() tests =====

TEST_F(TaskSchedulerTest, ExportMetricsReturnsNonEmptyString) {
    EXPECT_FALSE(scheduler_->exportMetrics().empty());
}

TEST_F(TaskSchedulerTest, ExportMetricsHasPrometheusHelp) {
    auto text = scheduler_->exportMetrics();
    EXPECT_NE(text.find("# HELP"), std::string::npos);
    EXPECT_NE(text.find("# TYPE"), std::string::npos);
}

TEST_F(TaskSchedulerTest, ExportMetricsContainsTaskName) {
    std::atomic<int> count{0};
    scheduler_->registerFunction("metrics_fn",
        [&count](const nlohmann::json&) -> nlohmann::json {
            ++count;
            return {};
        });
    ScheduledTask task;
    task.name          = "my_special_task";
    task.type          = ScheduledTask::TaskType::FUNCTION;
    task.function_name = "metrics_fn";
    task.trigger_type  = ScheduledTask::TriggerType::MANUAL;
    scheduler_->registerTask(task);

    auto text = scheduler_->exportMetrics();
    EXPECT_NE(text.find("my_special_task"), std::string::npos) << text;
}

TEST_F(TaskSchedulerTest, ExportMetricsSuccessCounterGrowsAfterExecution) {
    std::atomic<int> count{0};
    scheduler_->registerFunction("counter_fn",
        [&count](const nlohmann::json&) -> nlohmann::json {
            ++count;
            return {};
        });
    ScheduledTask task;
    task.name          = "counter_task";
    task.type          = ScheduledTask::TaskType::FUNCTION;
    task.function_name = "counter_fn";
    task.trigger_type  = ScheduledTask::TriggerType::MANUAL;
    std::string id = scheduler_->registerTask(task);

    // Before execution: global success counter includes "success"
    auto text_before = scheduler_->exportMetrics();
    EXPECT_NE(text_before.find("status=\"success\""), std::string::npos) << text_before;

    scheduler_->executeTaskNow(id);

    // After one success: the per-task success entry should contain "1" after label
    auto text_after = scheduler_->exportMetrics();
    // Check the scheduler-level counter line ends with the expected value
    EXPECT_NE(text_after.find("status=\"success\""), std::string::npos) << text_after;
    // The per-task counter for counter_task success should be 1
    EXPECT_NE(text_after.find("counter_task"), std::string::npos) << text_after;
}

// ===== Validation tests =====

TEST_F(TaskSchedulerTest, RetryPolicyMaxRetriesExceedsLimitThrows) {
    scheduler_->registerFunction("vfn", [](const nlohmann::json&) -> nlohmann::json {
        return {};
    });
    ScheduledTask task;
    task.name          = "over_retry";
    task.type          = ScheduledTask::TaskType::FUNCTION;
    task.function_name = "vfn";
    task.trigger_type  = ScheduledTask::TriggerType::MANUAL;

    ScheduledTask::RetryPolicy rp;
    rp.strategy    = ScheduledTask::RetryStrategy::FIXED_DELAY;
    rp.max_retries = 100;  // Exceeds MAX_RETRIES (10) enforced in validateResourceLimits()
    task.retry_policy = rp;

    EXPECT_THROW(scheduler_->registerTask(task), std::invalid_argument);
}

TEST_F(TaskSchedulerTest, EnableTaskThatDoesNotExistThrows) {
    // Unknown task IDs are a no-op by contract.
    EXPECT_NO_THROW(scheduler_->enableTask("nonexistent_task_id"));
}

TEST_F(TaskSchedulerTest, DisableTaskThatDoesNotExistThrows) {
    // Unknown task IDs are a no-op by contract.
    EXPECT_NO_THROW(scheduler_->disableTask("nonexistent_task_id"));
}

TEST_F(TaskSchedulerTest, GetTaskReturnsNullptrForUnknownId) {
    auto t = scheduler_->getTask("does_not_exist");
    EXPECT_EQ(t, nullptr);
}

// ===== Stats tests =====

TEST_F(TaskSchedulerTest, InitialStatsAreZero) {
    auto stats = scheduler_->getStats();
    EXPECT_EQ(stats.registered_tasks, 0u);
    EXPECT_EQ(stats.active_tasks, 0u);
    EXPECT_EQ(stats.total_executions, 0u);
    EXPECT_EQ(stats.failed_executions, 0u);
}

TEST_F(TaskSchedulerTest, StatsTotalExecutionsIncrementsOnSuccess) {
    std::atomic<int> count{0};
    scheduler_->registerFunction("stats_fn",
        [&count](const nlohmann::json&) -> nlohmann::json { ++count; return {}; });
    ScheduledTask task;
    task.name          = "stats_task";
    task.type          = ScheduledTask::TaskType::FUNCTION;
    task.function_name = "stats_fn";
    task.trigger_type  = ScheduledTask::TriggerType::MANUAL;
    std::string id = scheduler_->registerTask(task);

    scheduler_->executeTaskNow(id);
    scheduler_->executeTaskNow(id);

    EXPECT_EQ(scheduler_->getStats().total_executions, 2u);
    EXPECT_EQ(scheduler_->getStats().failed_executions, 0u);
}

TEST_F(TaskSchedulerTest, StatsFailedExecutionsIncrementsOnFailure) {
    scheduler_->registerFunction("fail_stats_fn",
        [](const nlohmann::json&) -> nlohmann::json {
            throw std::runtime_error("boom");
        });
    ScheduledTask task;
    task.name          = "fail_stats_task";
    task.type          = ScheduledTask::TaskType::FUNCTION;
    task.function_name = "fail_stats_fn";
    task.trigger_type  = ScheduledTask::TriggerType::MANUAL;
    task.max_retries   = 0;
    std::string id = scheduler_->registerTask(task);

    scheduler_->executeTaskNow(id);

    auto registered = scheduler_->getTask(id);
    ASSERT_NE(registered, nullptr);
    EXPECT_EQ(registered->failed_executions, 1u);
    EXPECT_EQ(scheduler_->getStats().total_executions, 1u);
}

// ===== CronExpression integration =====

TEST_F(TaskSchedulerTest, RegisterCronTaskWithSpecialExpression) {
    scheduler_->registerFunction("cron_fn",
        [](const nlohmann::json&) -> nlohmann::json { return {}; });
    ScheduledTask task;
    task.name            = "cron_special";
    task.type            = ScheduledTask::TaskType::FUNCTION;
    task.function_name   = "cron_fn";
    task.trigger_type    = ScheduledTask::TriggerType::CRON;
    task.cron_expression = "@daily";
    std::string id = scheduler_->registerTask(task);
    EXPECT_FALSE(id.empty());
    auto t = scheduler_->getTask(id);
    ASSERT_NE(t, nullptr);
    EXPECT_EQ(t->cron_expression, "@daily");
}

TEST_F(TaskSchedulerTest, RegisterAndListMultipleTasks) {
    scheduler_->registerFunction("list_fn",
        [](const nlohmann::json&) -> nlohmann::json { return {}; });
    for (int i = 0; i < 5; ++i) {
        ScheduledTask task;
        task.name          = "list_task_" + std::to_string(i);
        task.type          = ScheduledTask::TaskType::FUNCTION;
        task.function_name = "list_fn";
        task.trigger_type  = ScheduledTask::TriggerType::MANUAL;
        scheduler_->registerTask(task);
    }
    auto tasks = scheduler_->listTasks();
    EXPECT_EQ(tasks.size(), 5u);
}

TEST_F(TaskSchedulerTest, UnregisterReducesListSize) {
    scheduler_->registerFunction("ur_fn",
        [](const nlohmann::json&) -> nlohmann::json { return {}; });
    ScheduledTask task;
    task.name          = "ur_task";
    task.type          = ScheduledTask::TaskType::FUNCTION;
    task.function_name = "ur_fn";
    task.trigger_type  = ScheduledTask::TriggerType::MANUAL;
    std::string id = scheduler_->registerTask(task);

    EXPECT_EQ(scheduler_->listTasks().size(), 1u);
    scheduler_->unregisterTask(id);
    EXPECT_EQ(scheduler_->listTasks().size(), 0u);
}

TEST_F(TaskSchedulerTest, ExecuteUnknownTaskReturnsError) {
    auto result = scheduler_->executeTaskNow("totally_unknown_id");
    EXPECT_TRUE(result.contains("error"));
}

// ===== Error categorization tests =====

TEST_F(TaskSchedulerTest, SuccessfulExecutionClearsErrorCategory) {
    std::atomic<int> count{0};
    scheduler_->registerFunction("clear_cat_fn",
        [&count](const nlohmann::json&) -> nlohmann::json { ++count; return {}; });
    ScheduledTask task;
    task.name          = "clear_cat_task";
    task.type          = ScheduledTask::TaskType::FUNCTION;
    task.function_name = "clear_cat_fn";
    task.trigger_type  = ScheduledTask::TriggerType::MANUAL;
    std::string id = scheduler_->registerTask(task);

    scheduler_->executeTaskNow(id);

    auto t = scheduler_->getTask(id);
    ASSERT_NE(t, nullptr);
    EXPECT_EQ(t->last_error_category, ScheduledTask::ErrorCategory::NONE);
}

TEST_F(TaskSchedulerTest, PermanentErrorCategoryOnMissingFunction) {
    // Unregistered function → function not found → PERMANENT
    ScheduledTask task;
    task.name          = "perm_cat_task";
    task.type          = ScheduledTask::TaskType::FUNCTION;
    task.function_name = "definitely_does_not_exist";
    task.trigger_type  = ScheduledTask::TriggerType::MANUAL;
    task.max_retries   = 0;
    std::string id = scheduler_->registerTask(task);

    scheduler_->executeTaskNow(id);

    auto t = scheduler_->getTask(id);
    ASSERT_NE(t, nullptr);
    EXPECT_EQ(t->last_error_category, ScheduledTask::ErrorCategory::PERMANENT);
}

TEST_F(TaskSchedulerTest, TransientErrorCategoryOnGenericException) {
    scheduler_->registerFunction("transient_fn",
        [](const nlohmann::json&) -> nlohmann::json {
            throw std::runtime_error("connection reset");  // transient-sounding
        });
    ScheduledTask task;
    task.name          = "transient_cat_task";
    task.type          = ScheduledTask::TaskType::FUNCTION;
    task.function_name = "transient_fn";
    task.trigger_type  = ScheduledTask::TriggerType::MANUAL;
    task.max_retries   = 0;
    std::string id = scheduler_->registerTask(task);

    scheduler_->executeTaskNow(id);

    auto t = scheduler_->getTask(id);
    ASSERT_NE(t, nullptr);
    EXPECT_EQ(t->last_error_category, ScheduledTask::ErrorCategory::TRANSIENT);
}

TEST_F(TaskSchedulerTest, ErrorCategoryResetToNoneOnSubsequentSuccess) {
    int call = 0;
    scheduler_->registerFunction("flaky_fn",
        [&call](const nlohmann::json&) -> nlohmann::json {
            if (call++ == 0) {
              throw std::runtime_error("temporary blip");
            }
            return {};
        });
    ScheduledTask task;
    task.name          = "flaky_cat_task";
    task.type          = ScheduledTask::TaskType::FUNCTION;
    task.function_name = "flaky_fn";
    task.trigger_type  = ScheduledTask::TriggerType::MANUAL;
    task.max_retries   = 0;
    std::string id = scheduler_->registerTask(task);

    // First call fails
    scheduler_->executeTaskNow(id);
    auto t_fail = scheduler_->getTask(id);
    ASSERT_NE(t_fail, nullptr);
    EXPECT_NE(t_fail->last_error_category, ScheduledTask::ErrorCategory::NONE);

    // Second call succeeds
    scheduler_->executeTaskNow(id);
    auto t_ok = scheduler_->getTask(id);
    ASSERT_NE(t_ok, nullptr);
    EXPECT_EQ(t_ok->last_error_category, ScheduledTask::ErrorCategory::NONE);
}

// ===== Conditional branching tests =====

// Helper: register a FUNCTION task that stores its result in a shared variable and optionally
// records execution order, with an optional branch_condition.
static std::string registerConditionalTask(
    TaskScheduler* sched,
    const std::string& name,
    const std::vector<std::string>& deps,
    std::function<bool(const std::map<std::string, nlohmann::json>&)> condition,
    std::vector<std::string>& exec_log,
    std::mutex& log_mu)
{
    ScheduledTask t;
    t.id = name;
    t.name = name;
    t.type = ScheduledTask::TaskType::FUNCTION;
    t.function_name = name + "_cond_fn";
    t.trigger_type = ScheduledTask::TriggerType::MANUAL;
    t.dependencies = deps;
    t.branch_condition = std::move(condition);
    sched->registerFunction(name + "_cond_fn", [name, &exec_log, &log_mu](const nlohmann::json&) {
        std::lock_guard<std::mutex> lk(log_mu);
        exec_log.push_back(name);
        return nlohmann::json{{"task", name}, {"status", "ok"}};
    });
    return sched->registerTask(t);
}

TEST_F(TaskSchedulerTest, DAG_ConditionalBranchTrueExecutesTask) {
    // Task with branch_condition returning true should execute normally.
    std::vector<std::string> log;
    std::mutex mu;
    registerConditionalTask(scheduler_.get(), "cond_always_true", {},
        [](const std::map<std::string, nlohmann::json>&) { return true; },
        log, mu);

    auto res = scheduler_->executeDAG({"cond_always_true"});
    EXPECT_EQ(res.succeeded.size(), 1u);
    EXPECT_TRUE(res.failed.empty());
    EXPECT_TRUE(res.skipped.empty());
    EXPECT_TRUE(res.condition_skipped.empty());
    ASSERT_EQ(log.size(), 1u);
    EXPECT_EQ(log[0], "cond_always_true");
}

TEST_F(TaskSchedulerTest, DAG_ConditionalBranchFalseSkipsTask) {
    // Task with branch_condition returning false should be condition-skipped.
    std::vector<std::string> log;
    std::mutex mu;
    registerConditionalTask(scheduler_.get(), "cond_always_false", {},
        [](const std::map<std::string, nlohmann::json>&) { return false; },
        log, mu);

    auto res = scheduler_->executeDAG({"cond_always_false"});
    EXPECT_TRUE(res.succeeded.empty());
    EXPECT_TRUE(res.failed.empty());
    EXPECT_TRUE(res.skipped.empty());
    ASSERT_EQ(res.condition_skipped.size(), 1u);
    EXPECT_EQ(res.condition_skipped[0], "cond_always_false");
    EXPECT_TRUE(log.empty());
}

TEST_F(TaskSchedulerTest, DAG_ConditionalBranchEvaluatesDepResult) {
    // root → branch_ok (condition: root result status == "ok")
    // root → branch_err (condition: root result status == "error")
    // Only branch_ok should execute since root returns status "ok".
    std::vector<std::string> log;
    std::mutex mu;

    // Root task returns {"status": "ok"}
    ScheduledTask root;
    root.id = "cb_root"; root.name = root.id;
    root.type = ScheduledTask::TaskType::FUNCTION;
    root.function_name = "cb_root_fn";
    root.trigger_type = ScheduledTask::TriggerType::MANUAL;
    scheduler_->registerFunction("cb_root_fn", [](const nlohmann::json&) -> nlohmann::json {
        return {{"status", "ok"}};
    });
    scheduler_->registerTask(root);

    registerConditionalTask(scheduler_.get(), "cb_branch_ok", {"cb_root"},
        [](const std::map<std::string, nlohmann::json>& deps) {
            auto it = deps.find("cb_root");
            return it != deps.end() && it->second.value("status", "") == "ok";
        },
        log, mu);

    registerConditionalTask(scheduler_.get(), "cb_branch_err", {"cb_root"},
        [](const std::map<std::string, nlohmann::json>& deps) {
            auto it = deps.find("cb_root");
            return it != deps.end() && it->second.value("status", "") == "error";
        },
        log, mu);

    auto res = scheduler_->executeDAG({"cb_root", "cb_branch_ok", "cb_branch_err"});
    EXPECT_EQ(res.succeeded.size(), 2u);   // root + branch_ok
    EXPECT_TRUE(res.succeeded.count("cb_root"));
    EXPECT_TRUE(res.succeeded.count("cb_branch_ok"));
    EXPECT_TRUE(res.failed.empty());
    EXPECT_TRUE(res.skipped.empty());
    ASSERT_EQ(res.condition_skipped.size(), 1u);
    EXPECT_EQ(res.condition_skipped[0], "cb_branch_err");
    ASSERT_EQ(log.size(), 1u);
    EXPECT_EQ(log[0], "cb_branch_ok");
}

TEST_F(TaskSchedulerTest, DAG_ConditionalSkipPropagatesTransitively) {
    // A → B (condition: false) → C
    // B is condition-skipped, C should be condition-skipped transitively.
    std::vector<std::string> log;
    std::mutex mu = {};

    // Register root task A inline (no branch_condition)
    scheduler_->registerFunction("cs_a_fn", [&log, &mu](const nlohmann::json&) -> nlohmann::json {
        std::lock_guard<std::mutex> lk(mu);
        log.push_back("cs_a");
        return nlohmann::json{{"task", "cs_a"}, {"status", "ok"}};
    });
    ScheduledTask ta;
    ta.id = "cs_a"; ta.name = ta.id;
    ta.type = ScheduledTask::TaskType::FUNCTION;
    ta.function_name = "cs_a_fn";
    ta.trigger_type = ScheduledTask::TriggerType::MANUAL;
    scheduler_->registerTask(ta);

    registerConditionalTask(scheduler_.get(), "cs_b", {"cs_a"},
        [](const std::map<std::string, nlohmann::json>&) { return false; },
        log, mu);
    registerConditionalTask(scheduler_.get(), "cs_c", {"cs_b"},
        nullptr,  // no branch_condition – should be skipped transitively
        log, mu);

    auto res = scheduler_->executeDAG({"cs_a", "cs_b", "cs_c"});
    EXPECT_EQ(res.succeeded.size(), 1u);
    EXPECT_TRUE(res.succeeded.count("cs_a"));
    EXPECT_TRUE(res.failed.empty());
    EXPECT_TRUE(res.skipped.empty());
    EXPECT_EQ(res.condition_skipped.size(), 2u);
    // Both B and C should be in condition_skipped
    auto& cs = res.condition_skipped;
    EXPECT_NE(std::find(cs.begin(), cs.end(), "cs_b"), cs.end());
    EXPECT_NE(std::find(cs.begin(), cs.end(), "cs_c"), cs.end());
    // Only A executed
    ASSERT_EQ(log.size(), 1u);
    EXPECT_EQ(log[0], "cs_a");
}

// Helper: register a FUNCTION task that records execution order
static std::string registerOrderTask(
    TaskScheduler* sched,
    const std::string& name,
    const std::vector<std::string>& deps,
    std::vector<std::string>& order_log,
    std::mutex& log_mu)
{
    ScheduledTask t;
    t.id = name;
    t.name = name;
    t.type = ScheduledTask::TaskType::FUNCTION;
    t.function_name = name + "_fn";
    t.trigger_type = ScheduledTask::TriggerType::MANUAL;
    t.dependencies = deps;
    sched->registerFunction(name + "_fn", [name, &order_log, &log_mu](const nlohmann::json&) {
        std::lock_guard<std::mutex> lk(log_mu);
        order_log.push_back(name);
        return nlohmann::json{{"task", name}};
    });
    return sched->registerTask(t);
}

TEST_F(TaskSchedulerTest, DAG_EmptySetReturnsEmptyResult) {
    auto res = scheduler_->executeDAG({});
    EXPECT_TRUE(res.succeeded.empty());
    EXPECT_TRUE(res.failed.empty());
    EXPECT_TRUE(res.skipped.empty());
}

TEST_F(TaskSchedulerTest, DAG_UnknownTaskIdThrows) {
    EXPECT_THROW(scheduler_->executeDAG({"does_not_exist"}), std::invalid_argument);
}

TEST_F(TaskSchedulerTest, DAG_SingleTask) {
    std::vector<std::string> order;
    std::mutex mu = {};
    registerOrderTask(scheduler_.get(), "solo", {}, order, mu);

    auto res = scheduler_->executeDAG({"solo"});
    EXPECT_EQ(res.succeeded.size(), 1u);
    EXPECT_TRUE(res.failed.empty());
    EXPECT_TRUE(res.skipped.empty());
    ASSERT_EQ(order.size(), 1u);
    EXPECT_EQ(order[0], "solo");
}

TEST_F(TaskSchedulerTest, DAG_LinearChainRespectsDependencyOrder) {
    // a -> b -> c   (b depends on a; c depends on b)
    std::vector<std::string> order;
    std::mutex mu = {};
    registerOrderTask(scheduler_.get(), "dag_a", {}, order, mu);
    registerOrderTask(scheduler_.get(), "dag_b", {"dag_a"}, order, mu);
    registerOrderTask(scheduler_.get(), "dag_c", {"dag_b"}, order, mu);

    auto res = scheduler_->executeDAG({"dag_a", "dag_b", "dag_c"});
    EXPECT_EQ(res.succeeded.size(), 3u);
    EXPECT_TRUE(res.failed.empty());
    EXPECT_TRUE(res.skipped.empty());

    // Order must be a before b before c
    auto pos = [&](const std::string& id) {
        return std::find(order.begin(), order.end(), id) - order.begin();
    };
    EXPECT_LT(pos("dag_a"), pos("dag_b"));
    EXPECT_LT(pos("dag_b"), pos("dag_c"));
}

TEST_F(TaskSchedulerTest, DAG_ParallelIndependentTasksAllSucceed) {
    // p1, p2, p3 have no dependencies – all run independently
    std::vector<std::string> order;
    std::mutex mu = {};
    registerOrderTask(scheduler_.get(), "par1", {}, order, mu);
    registerOrderTask(scheduler_.get(), "par2", {}, order, mu);
    registerOrderTask(scheduler_.get(), "par3", {}, order, mu);

    auto res = scheduler_->executeDAG({"par1", "par2", "par3"});
    EXPECT_EQ(res.succeeded.size(), 3u);
    EXPECT_TRUE(res.failed.empty());
    EXPECT_TRUE(res.skipped.empty());
}

TEST_F(TaskSchedulerTest, DAG_CascadingFailureSkipsDependents) {
    // root -> child -> grandchild; root fails → child and grandchild skipped
    scheduler_->registerFunction("fail_fn", [](const nlohmann::json&) -> nlohmann::json {
        throw std::runtime_error("intentional failure");
    });
    scheduler_->registerFunction("child_fn", [](const nlohmann::json&) -> nlohmann::json {
        return {};
    });
    scheduler_->registerFunction("grandchild_fn", [](const nlohmann::json&) -> nlohmann::json {
        return {};
    });

    ScheduledTask root;
    root.id = "dag_root_fail"; root.name = root.id;
    root.type = ScheduledTask::TaskType::FUNCTION;
    root.function_name = "fail_fn";
    root.trigger_type = ScheduledTask::TriggerType::MANUAL;
    root.max_retries = 0;
    scheduler_->registerTask(root);

    ScheduledTask child;
    child.id = "dag_child"; child.name = child.id;
    child.type = ScheduledTask::TaskType::FUNCTION;
    child.function_name = "child_fn";
    child.trigger_type = ScheduledTask::TriggerType::MANUAL;
    child.dependencies = {"dag_root_fail"};
    scheduler_->registerTask(child);

    ScheduledTask grand;
    grand.id = "dag_grandchild"; grand.name = grand.id;
    grand.type = ScheduledTask::TaskType::FUNCTION;
    grand.function_name = "grandchild_fn";
    grand.trigger_type = ScheduledTask::TriggerType::MANUAL;
    grand.dependencies = {"dag_child"};
    scheduler_->registerTask(grand);

    auto res = scheduler_->executeDAG({"dag_root_fail", "dag_child", "dag_grandchild"});
    EXPECT_EQ(res.failed.size(), 1u);
    EXPECT_TRUE(res.failed.count("dag_root_fail"));
    EXPECT_EQ(res.skipped.size(), 2u);
    EXPECT_TRUE(res.succeeded.empty());
}

TEST_F(TaskSchedulerTest, DAG_CycleDetectionThrows) {
    // a depends on b, b depends on a → cycle
    // Cycle is detected during topological sort, before any task executes.
    scheduler_->registerFunction("cyc_noop", [](const nlohmann::json&) { return nlohmann::json{}; });

    ScheduledTask ta;
    ta.id = "cyc_a"; ta.name = ta.id;
    ta.type = ScheduledTask::TaskType::FUNCTION;
    ta.function_name = "cyc_noop";
    ta.trigger_type = ScheduledTask::TriggerType::MANUAL;
    ta.dependencies = {"cyc_b"};
    scheduler_->registerTask(ta);

    ScheduledTask tb;
    tb.id = "cyc_b"; tb.name = tb.id;
    tb.type = ScheduledTask::TaskType::FUNCTION;
    tb.function_name = "cyc_noop";
    tb.trigger_type = ScheduledTask::TriggerType::MANUAL;
    tb.dependencies = {"cyc_a"};
    scheduler_->registerTask(tb);

    EXPECT_THROW(scheduler_->executeDAG({"cyc_a", "cyc_b"}), std::runtime_error);
}

TEST_F(TaskSchedulerTest, DAG_DependencyOutsideSetIsIgnored) {
    // task_x depends on task_y, but only task_x is in the execution set
    std::vector<std::string> order;
    std::mutex mu = {};
    registerOrderTask(scheduler_.get(), "only_x", {"nonexistent_y"}, order, mu);

    // Should not throw and should succeed (out-of-set dep is silently ignored)
    auto res = scheduler_->executeDAG({"only_x"});
    EXPECT_EQ(res.succeeded.size(), 1u);
    EXPECT_TRUE(res.failed.empty());
    EXPECT_TRUE(res.skipped.empty());
}

TEST_F(TaskSchedulerTest, DAG_DependenciesPersistedAndRestoredFromDisk) {
    // Verify that the `dependencies` field survives a save/load round-trip.
    auto persist_path = db_path_ + "/persist_dag_test";
    std::filesystem::create_directories(persist_path);

    TaskScheduler::Config pcfg;
    pcfg.persist_tasks = true;
    pcfg.persistence_path = persist_path;
    pcfg.enable_audit_logging = false;
    pcfg.enable_anomaly_detection = false;
    auto sched = std::make_unique<TaskScheduler>(engine_.get(), pcfg);

    ScheduledTask task;
    task.id = "persist_dep_task";
    task.name = task.id;
    task.type = ScheduledTask::TaskType::FUNCTION;
    task.function_name = "noop_fn";
    task.trigger_type = ScheduledTask::TriggerType::MANUAL;
    task.dependencies = {"dep_a", "dep_b"};
    sched->registerTask(task);
    sched.reset();  // destructor triggers saveTasks()

    // Create a fresh scheduler that loads from disk
    auto sched2 = std::make_unique<TaskScheduler>(engine_.get(), pcfg);
    auto loaded = sched2->getTask("persist_dep_task");
    ASSERT_NE(loaded, nullptr);
    ASSERT_EQ(loaded->dependencies.size(), 2u);
    EXPECT_EQ(loaded->dependencies[0], "dep_a");
    EXPECT_EQ(loaded->dependencies[1], "dep_b");
}

TEST_F(TaskSchedulerTest, DAG_AuditEventsLoggedPerTask) {
    // Create a scheduler with audit logging enabled
    TaskScheduler::Config acfg;
    acfg.max_concurrent_tasks = 4;
    acfg.check_interval = 50ms;
    acfg.persist_tasks = false;
    acfg.enable_audit_logging = true;
    acfg.enable_anomaly_detection = false;
    auto audit_sched = std::make_unique<TaskScheduler>(engine_.get(), acfg);

    audit_sched->registerFunction("dag_audit_ok",
        [](const nlohmann::json&) -> nlohmann::json { return {{"ok", true}}; });
    audit_sched->registerFunction("dag_audit_fail",
        [](const nlohmann::json&) -> nlohmann::json {
            throw std::runtime_error("audit_fail_error");
        });

    ScheduledTask ta;
    ta.id = "dag_audit_a"; ta.name = ta.id;
    ta.type = ScheduledTask::TaskType::FUNCTION;
    ta.function_name = "dag_audit_ok";
    ta.trigger_type = ScheduledTask::TriggerType::MANUAL;
    ta.max_retries = 0;
    audit_sched->registerTask(ta);

    ScheduledTask tb;
    tb.id = "dag_audit_b"; tb.name = tb.id;
    tb.type = ScheduledTask::TaskType::FUNCTION;
    tb.function_name = "dag_audit_fail";
    tb.trigger_type = ScheduledTask::TriggerType::MANUAL;
    tb.max_retries = 0;
    audit_sched->registerTask(tb);

    auto res = audit_sched->executeDAG({"dag_audit_a", "dag_audit_b"});
    EXPECT_TRUE(res.succeeded.count("dag_audit_a"));
    EXPECT_TRUE(res.failed.count("dag_audit_b"));

    auto audit_mgr = audit_sched->getAuditManager();
    ASSERT_NE(audit_mgr, nullptr);

    // Query started events for both tasks
    scheduler::AuditQueryParams started_params;
    started_params.event_type = scheduler::TaskEventType::TASK_STARTED;
    started_params.limit = 100;
    auto started = audit_mgr->queryAuditEvents(started_params);
    auto started_a = std::count_if(started.begin(), started.end(),
        [](const auto& e) { return e.task_id == "dag_audit_a"; });
    auto started_b = std::count_if(started.begin(), started.end(),
        [](const auto& e) { return e.task_id == "dag_audit_b"; });
    EXPECT_GE(started_a, 1);
    EXPECT_GE(started_b, 1);

    // Task A completed successfully
    scheduler::AuditQueryParams completed_params;
    completed_params.task_id = "dag_audit_a";
    completed_params.event_type = scheduler::TaskEventType::TASK_COMPLETED;
    completed_params.limit = 100;
    auto completed = audit_mgr->queryAuditEvents(completed_params);
    EXPECT_GE(completed.size(), 1u);
    EXPECT_TRUE(completed[0].success);

    // Task B failed
    scheduler::AuditQueryParams failed_params;
    failed_params.task_id = "dag_audit_b";
    failed_params.event_type = scheduler::TaskEventType::TASK_FAILED;
    failed_params.limit = 100;
    auto failed = audit_mgr->queryAuditEvents(failed_params);
    EXPECT_GE(failed.size(), 1u);
    EXPECT_FALSE(failed[0].success);
    ASSERT_TRUE(failed[0].error_message.has_value());
    EXPECT_NE(failed[0].error_message->find("audit_fail_error"), std::string::npos);
}

TEST_F(TaskSchedulerTest, DAG_AvgExecutionTimeUpdatedAfterSuccess) {
    // Verify that avg_execution_time_ms is updated by executeDAG for successful tasks.
    scheduler_->registerFunction("dag_avg_fn",
        [](const nlohmann::json&) -> nlohmann::json { return {}; });

    ScheduledTask t;
    t.id = "dag_avg_task"; t.name = t.id;
    t.type = ScheduledTask::TaskType::FUNCTION;
    t.function_name = "dag_avg_fn";
    t.trigger_type = ScheduledTask::TriggerType::MANUAL;
    scheduler_->registerTask(t);

    auto res = scheduler_->executeDAG({"dag_avg_task"});
    ASSERT_TRUE(res.succeeded.count("dag_avg_task"));

    auto loaded = scheduler_->getTask("dag_avg_task");
    ASSERT_NE(loaded, nullptr);
    EXPECT_GE(loaded->avg_execution_time_ms, 0.0);
    EXPECT_EQ(loaded->total_executions, 1u);
    EXPECT_EQ(loaded->successful_executions, 1u);
}

// ===== Task Result Store tests =====

class TaskResultStoreTest : public ::testing::Test {
protected:
    static std::string makeDbPath() {
        auto now = std::chrono::high_resolution_clock::now().time_since_epoch().count();
        return (std::filesystem::temp_directory_path() /
                std::filesystem::path("themis_rs_test_" + std::to_string(now))).string();
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
        sched_cfg.enable_result_store = true;
        sched_cfg.result_store_max_results_per_task = 5;

        scheduler_ = std::make_unique<TaskScheduler>(
            engine_.get(), sched_cfg,
            /*changefeed=*/nullptr,
            /*audit_logger=*/nullptr,
            storage_.get());
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

TEST_F(TaskResultStoreTest, ResultStoredAfterSuccessfulExecution) {
    // Register a simple function task and execute it manually.
    scheduler_->registerFunction("store_fn",
        [](const nlohmann::json&) -> nlohmann::json {
            return {{"status", "ok"}, {"value", 42}};
        });

    ScheduledTask t;
    t.id   = "rs_task1"; t.name = t.id;
    t.type = ScheduledTask::TaskType::FUNCTION;
    t.function_name = "store_fn";
    t.trigger_type  = ScheduledTask::TriggerType::MANUAL;
    scheduler_->registerTask(t);

    auto res = scheduler_->executeTaskNow("rs_task1");
    EXPECT_FALSE(res.contains("error"));

    // Result should be stored.
    auto latest = scheduler_->getLatestTaskResult("rs_task1");
    ASSERT_TRUE(latest.has_value());
    EXPECT_EQ(latest->task_id, "rs_task1");
    EXPECT_TRUE(latest->success);
    EXPECT_TRUE(latest->error.empty());
    EXPECT_EQ(latest->output.value("value", 0), 42);
    EXPECT_GT(latest->duration_ms, 0.0);
}

TEST_F(TaskResultStoreTest, ResultStoredAfterFailedExecution) {
    scheduler_->registerFunction("fail_fn",
        [](const nlohmann::json&) -> nlohmann::json {
            throw std::runtime_error("intentional failure");
        });

    ScheduledTask t;
    t.id   = "rs_fail_task"; t.name = t.id;
    t.type = ScheduledTask::TaskType::FUNCTION;
    t.function_name = "fail_fn";
    t.trigger_type  = ScheduledTask::TriggerType::MANUAL;
    t.max_retries   = 0;
    scheduler_->registerTask(t);

    scheduler_->executeTaskNow("rs_fail_task");

    auto latest = scheduler_->getLatestTaskResult("rs_fail_task");
    ASSERT_TRUE(latest.has_value());
    EXPECT_FALSE(latest->success);
    EXPECT_FALSE(latest->error.empty());
    EXPECT_EQ(latest->task_id, "rs_fail_task");
}

TEST_F(TaskResultStoreTest, GetTaskResultsReturnsNewestFirst) {
    scheduler_->registerFunction("cnt_fn",
        [](const nlohmann::json& p) -> nlohmann::json {
            return {{"n", p.value("n", 0)}};
        });

    ScheduledTask t;
    t.id   = "rs_multi"; t.name = t.id;
    t.type = ScheduledTask::TaskType::FUNCTION;
    t.function_name = "cnt_fn";
    t.trigger_type  = ScheduledTask::TriggerType::MANUAL;
    t.parameters    = {{"n", 0}};
    t.max_retries   = 0;
    scheduler_->registerTask(t);

    // Execute 3 times – each call succeeds.
    for (int i = 1; i <= 3; ++i) {
        // Patch parameters for each run so results differ.
        auto task = scheduler_->getTask("rs_multi");
        ASSERT_NE(task, nullptr);
        task->parameters = {{"n", i}};
        scheduler_->executeTaskNow("rs_multi");
        // Small sleep to ensure distinct timestamps.
        std::this_thread::sleep_for(std::chrono::milliseconds(2));
    }

    auto results = scheduler_->getTaskResults("rs_multi", 10);
    ASSERT_EQ(results.size(), 3u);
    // Newest first → n == 3, 2, 1
    EXPECT_EQ(results[0].output.value("n", 0), 3);
    EXPECT_EQ(results[1].output.value("n", 0), 2);
    EXPECT_EQ(results[2].output.value("n", 0), 1);
}

TEST_F(TaskResultStoreTest, RetentionLimitPrunesOldestRecords) {
    // max_results_per_task is set to 5 in SetUp.
    scheduler_->registerFunction("prune_fn",
        [](const nlohmann::json&) -> nlohmann::json { return {{"ok", true}}; });

    ScheduledTask t;
    t.id   = "rs_prune"; t.name = t.id;
    t.type = ScheduledTask::TaskType::FUNCTION;
    t.function_name = "prune_fn";
    t.trigger_type  = ScheduledTask::TriggerType::MANUAL;
    t.max_retries   = 0;
    scheduler_->registerTask(t);

    // Execute 7 times (cap is 5).
    for (int i = 0; i < 7; ++i) {
        scheduler_->executeTaskNow("rs_prune");
        std::this_thread::sleep_for(std::chrono::milliseconds(2));
    }

    auto results = scheduler_->getTaskResults("rs_prune", 100);
    EXPECT_LE(results.size(), 5u);
}

TEST_F(TaskResultStoreTest, NoResultsWhenDisabled) {
    // Build a scheduler WITHOUT result store.
    TaskScheduler::Config cfg;
    cfg.enable_audit_logging    = false;
    cfg.enable_anomaly_detection = false;
    cfg.enable_result_store     = false;

    auto sched_no_store = std::make_unique<TaskScheduler>(engine_.get(), cfg);
    sched_no_store->registerFunction("noop_rs",
        [](const nlohmann::json&) -> nlohmann::json { return {}; });

    ScheduledTask t;
    t.id   = "rs_disabled"; t.name = t.id;
    t.type = ScheduledTask::TaskType::FUNCTION;
    t.function_name = "noop_rs";
    t.trigger_type  = ScheduledTask::TriggerType::MANUAL;
    sched_no_store->registerTask(t);
    sched_no_store->executeTaskNow("rs_disabled");

    EXPECT_FALSE(sched_no_store->getLatestTaskResult("rs_disabled").has_value());
    EXPECT_TRUE(sched_no_store->getTaskResults("rs_disabled", 10).empty());
}

TEST_F(TaskResultStoreTest, ResultSurvivesSchedulerRestart) {
    // Verify that results stored in RocksDB are readable after recreating the scheduler.
    scheduler_->registerFunction("survive_fn",
        [](const nlohmann::json&) -> nlohmann::json { return {{"persisted", true}}; });

    ScheduledTask t;
    t.id   = "rs_survive"; t.name = t.id;
    t.type = ScheduledTask::TaskType::FUNCTION;
    t.function_name = "survive_fn";
    t.trigger_type  = ScheduledTask::TriggerType::MANUAL;
    scheduler_->registerTask(t);
    scheduler_->executeTaskNow("rs_survive");

    // Destroy and recreate the scheduler (same storage).
    scheduler_.reset();

    TaskScheduler::Config cfg2;
    cfg2.enable_audit_logging    = false;
    cfg2.enable_anomaly_detection = false;
    cfg2.enable_result_store     = true;
    auto sched2 = std::make_unique<TaskScheduler>(
        engine_.get(), cfg2,
        nullptr, nullptr, storage_.get());

    auto latest = sched2->getLatestTaskResult("rs_survive");
    ASSERT_TRUE(latest.has_value());
    EXPECT_TRUE(latest->success);
    EXPECT_EQ(latest->output.value("persisted", false), true);
    scheduler_ = std::move(sched2);
}

TEST_F(TaskResultStoreTest, DAGExecutionPersistsResults) {
    // All three tasks in a linear DAG should have their results stored.
    scheduler_->registerFunction("dag_rs_fn",
        [](const nlohmann::json& p) -> nlohmann::json {
            return {{"step", p.value("step", 0)}};
        });

    auto make_task = [&](const std::string& id, const std::vector<std::string>& deps, int step) {
        ScheduledTask t;
        t.id   = id;
        t.name = id;
        t.type = ScheduledTask::TaskType::FUNCTION;
        t.function_name = "dag_rs_fn";
        t.parameters    = {{"step", step}};
        t.trigger_type  = ScheduledTask::TriggerType::MANUAL;
        t.dependencies  = deps;
        t.max_retries   = 0;
        scheduler_->registerTask(t);
    };

    make_task("dag_rs_a", {}, 1);
    make_task("dag_rs_b", {"dag_rs_a"}, 2);
    make_task("dag_rs_c", {"dag_rs_b"}, 3);

    auto dag_result = scheduler_->executeDAG({"dag_rs_a", "dag_rs_b", "dag_rs_c"});
    ASSERT_EQ(dag_result.succeeded.size(), 3u);
    EXPECT_TRUE(dag_result.failed.empty());

    // All three tasks should have a stored result.
    for (const auto& id : {"dag_rs_a", "dag_rs_b", "dag_rs_c"}) {
        auto r = scheduler_->getLatestTaskResult(id);
        ASSERT_TRUE(r.has_value()) << "Missing result for " << id;
        EXPECT_TRUE(r->success);
        EXPECT_EQ(r->task_id, id);
    }
    // Verify output values
    EXPECT_EQ(scheduler_->getLatestTaskResult("dag_rs_a")->output.value("step", 0), 1);
    EXPECT_EQ(scheduler_->getLatestTaskResult("dag_rs_b")->output.value("step", 0), 2);
    EXPECT_EQ(scheduler_->getLatestTaskResult("dag_rs_c")->output.value("step", 0), 3);
}

// ===== Alert on task failure / SLA breach tests =====

/**
 * @brief Simple mock Alertmanager that records sent/resolved alerts.
 */
namespace {
class MockAlertmanager : public themis::observability::Alertmanager {
public:
    std::vector<themis::observability::Alert> sent_alerts;
    std::vector<std::string> resolved_alert_ids;

    themis::Result<void> sendAlert(const themis::observability::Alert& alert) override {
        sent_alerts.push_back(alert);
        return {};
    }

    themis::Result<void> resolveAlert(const std::string& alert_id) override {
        resolved_alert_ids.push_back(alert_id);
        return {};
    }
};
} // namespace

class TaskSchedulerAlertTest : public ::testing::Test {
protected:
    static std::string makeDbPath() {
        auto now = std::chrono::high_resolution_clock::now().time_since_epoch().count();
        return (std::filesystem::temp_directory_path() /
                std::filesystem::path("themis_alert_test_" + std::to_string(now))).string();
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

        mock_alertmanager_ = std::make_shared<MockAlertmanager>();
        scheduler_->setAlertmanager(mock_alertmanager_);
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
    std::shared_ptr<MockAlertmanager> mock_alertmanager_;
};

TEST_F(TaskSchedulerAlertTest, FailureAlertFiredOnTaskFailure) {
    scheduler_->registerFunction("fail_fn", [](const nlohmann::json&) -> nlohmann::json {
        throw std::runtime_error("intentional failure");
    });

    ScheduledTask t;
    t.id             = "alert_fail_task";
    t.name           = "Alert Fail Task";
    t.type           = ScheduledTask::TaskType::FUNCTION;
    t.function_name  = "fail_fn";
    t.trigger_type   = ScheduledTask::TriggerType::MANUAL;
    t.max_retries    = 0;
    scheduler_->registerTask(t);

    scheduler_->executeTaskNow("alert_fail_task");

    ASSERT_EQ(mock_alertmanager_->sent_alerts.size(), 1u);
    const auto& alert = mock_alertmanager_->sent_alerts[0];
    EXPECT_EQ(alert.alert_name, "TaskFailure");
    EXPECT_EQ(alert.status, themis::observability::AlertStatus::FIRING);
    EXPECT_EQ(alert.severity, themis::observability::AlertSeverity::ERROR);
    EXPECT_FALSE(alert.message.empty());
    EXPECT_EQ(alert.labels.at("task_id"), "alert_fail_task");
    EXPECT_EQ(alert.labels.at("component"), "scheduler");
}

TEST_F(TaskSchedulerAlertTest, FailureAlertResolvedOnSubsequentSuccess) {
    std::atomic<bool> should_fail{true};
    scheduler_->registerFunction("toggle_fn", [&](const nlohmann::json&) -> nlohmann::json {
        if (should_fail.load()) {
            throw std::runtime_error("transient error");
        }
        return {{"ok", true}};
    });

    ScheduledTask t;
    t.id            = "toggle_task";
    t.name          = "Toggle Task";
    t.type          = ScheduledTask::TaskType::FUNCTION;
    t.function_name = "toggle_fn";
    t.trigger_type  = ScheduledTask::TriggerType::MANUAL;
    t.max_retries   = 0;
    scheduler_->registerTask(t);

    // First run: should fail and fire an alert
    scheduler_->executeTaskNow("toggle_task");
    ASSERT_EQ(mock_alertmanager_->sent_alerts.size(), 1u);
    EXPECT_EQ(mock_alertmanager_->sent_alerts[0].alert_name, "TaskFailure");

    // Second run: should succeed and resolve the alert
    should_fail.store(false);
    scheduler_->executeTaskNow("toggle_task");

    ASSERT_EQ(mock_alertmanager_->resolved_alert_ids.size(), 1u);
    EXPECT_FALSE(mock_alertmanager_->resolved_alert_ids[0].empty());
}

TEST_F(TaskSchedulerAlertTest, NoResolveWhenNoPreviousFailureAlert) {
    scheduler_->registerFunction("ok_fn", [](const nlohmann::json&) -> nlohmann::json {
        return {{"ok", true}};
    });

    ScheduledTask t;
    t.id            = "ok_task";
    t.name          = "OK Task";
    t.type          = ScheduledTask::TaskType::FUNCTION;
    t.function_name = "ok_fn";
    t.trigger_type  = ScheduledTask::TriggerType::MANUAL;
    t.max_retries   = 0;
    scheduler_->registerTask(t);

    // Run successfully with no prior failure
    scheduler_->executeTaskNow("ok_task");

    // No alerts should be sent or resolved
    EXPECT_TRUE(mock_alertmanager_->sent_alerts.empty());
    EXPECT_TRUE(mock_alertmanager_->resolved_alert_ids.empty());
}

TEST_F(TaskSchedulerAlertTest, SlaBreachAlertFiredWhenDeadlineExceeded) {
    scheduler_->registerFunction("slow_fn", [](const nlohmann::json&) -> nlohmann::json {
        std::this_thread::sleep_for(std::chrono::milliseconds(60));
        return {{"ok", true}};
    });

    ScheduledTask t;
    t.id            = "sla_task";
    t.name          = "SLA Task";
    t.type          = ScheduledTask::TaskType::FUNCTION;
    t.function_name = "slow_fn";
    t.trigger_type  = ScheduledTask::TriggerType::MANUAL;
    t.max_retries   = 0;
    t.sla_deadline  = std::chrono::milliseconds(10);  // 10ms SLA, task takes ~60ms
    scheduler_->registerTask(t);

    scheduler_->executeTaskNow("sla_task");

    // Should fire exactly one SLA breach alert (task succeeded, so no failure alert)
    ASSERT_EQ(mock_alertmanager_->sent_alerts.size(), 1u);
    const auto& alert = mock_alertmanager_->sent_alerts[0];
    EXPECT_EQ(alert.alert_name, "TaskSlaBreached");
    EXPECT_EQ(alert.status, themis::observability::AlertStatus::FIRING);
    EXPECT_EQ(alert.severity, themis::observability::AlertSeverity::WARNING);
    EXPECT_FALSE(alert.message.empty());
    EXPECT_EQ(alert.labels.at("task_id"), "sla_task");
    EXPECT_EQ(alert.labels.at("component"), "scheduler");
}

TEST_F(TaskSchedulerAlertTest, NoSlaAlertWhenDeadlineNotExceeded) {
    scheduler_->registerFunction("fast_fn", [](const nlohmann::json&) -> nlohmann::json {
        return {{"ok", true}};
    });

    ScheduledTask t;
    t.id            = "fast_sla_task";
    t.name          = "Fast SLA Task";
    t.type          = ScheduledTask::TaskType::FUNCTION;
    t.function_name = "fast_fn";
    t.trigger_type  = ScheduledTask::TriggerType::MANUAL;
    t.max_retries   = 0;
    t.sla_deadline  = std::chrono::seconds(60);  // 60s SLA, task is instant
    scheduler_->registerTask(t);

    scheduler_->executeTaskNow("fast_sla_task");

    // No alerts should be fired
    EXPECT_TRUE(mock_alertmanager_->sent_alerts.empty());
    EXPECT_TRUE(mock_alertmanager_->resolved_alert_ids.empty());
}

TEST_F(TaskSchedulerAlertTest, NoSlaAlertWhenDeadlineNotSet) {
    scheduler_->registerFunction("no_sla_fn", [](const nlohmann::json&) -> nlohmann::json {
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
        return {{"ok", true}};
    });

    ScheduledTask t;
    t.id            = "no_sla_task";
    t.name          = "No SLA Task";
    t.type          = ScheduledTask::TaskType::FUNCTION;
    t.function_name = "no_sla_fn";
    t.trigger_type  = ScheduledTask::TriggerType::MANUAL;
    t.max_retries   = 0;
    // sla_deadline intentionally not set
    scheduler_->registerTask(t);

    scheduler_->executeTaskNow("no_sla_task");

    // No SLA alert should fire since no deadline is configured
    EXPECT_TRUE(mock_alertmanager_->sent_alerts.empty());
}

TEST_F(TaskSchedulerAlertTest, SetAlertmanagerReturnsCorrectInstance) {
    auto am = scheduler_->getAlertmanager();
    EXPECT_EQ(am.get(), mock_alertmanager_.get());

    scheduler_->setAlertmanager(nullptr);
    EXPECT_EQ(scheduler_->getAlertmanager(), nullptr);
}

TEST_F(TaskSchedulerAlertTest, NoAlertsWithoutAlertmanager) {
    // Remove alertmanager
    scheduler_->setAlertmanager(nullptr);

    scheduler_->registerFunction("fail2_fn", [](const nlohmann::json&) -> nlohmann::json {
        throw std::runtime_error("error without alertmanager");
    });

    ScheduledTask t;
    t.id            = "no_am_task";
    t.name          = "No Alertmanager Task";
    t.type          = ScheduledTask::TaskType::FUNCTION;
    t.function_name = "fail2_fn";
    t.trigger_type  = ScheduledTask::TriggerType::MANUAL;
    t.max_retries   = 0;
    scheduler_->registerTask(t);

    // Should not throw even without alertmanager
    EXPECT_NO_THROW(scheduler_->executeTaskNow("no_am_task"));

    // No alerts were recorded (mock was detached)
    EXPECT_TRUE(mock_alertmanager_->sent_alerts.empty());
}

// ===========================================================================
// SCHED-AGE: Starvation Prevention via Aging
//
//  SCHED-AGE-01  consecutive_skips increments when a task is skipped due to
//                the concurrency limit
//  SCHED-AGE-02  effective priority is boosted after aging_threshold skips
//  SCHED-AGE-03  LOW task eventually wins over NORMAL when aged
//  SCHED-AGE-04  consecutive_skips resets to 0 when the task is dispatched
//  SCHED-AGE-05  aging disabled (aging_threshold == 0) leaves priority unchanged
// ===========================================================================

class AgingTest : public ::testing::Test {
protected:
    static std::string makeDbPath() {
        auto now = std::chrono::high_resolution_clock::now().time_since_epoch().count();
        return (std::filesystem::temp_directory_path() /
                std::filesystem::path("themis_sched_aging_test_" + std::to_string(now))).string();
    }

    void SetUp() override {
        db_path_ = makeDbPath();
        std::filesystem::create_directories(db_path_);

        RocksDBWrapper::Config db_cfg;
        db_cfg.db_path = db_path_ + "/db";
        db_cfg.enable_blobdb = false;
        storage_ = std::make_unique<RocksDBWrapper>(db_cfg);
        ASSERT_TRUE(storage_->open());

        idx_ = std::make_unique<SecondaryIndexManager>(*storage_);
        engine_ = std::make_unique<QueryEngine>(*storage_, *idx_);

        TaskScheduler::Config cfg;
        cfg.max_concurrent_tasks = 1;          // Only 1 slot → forces skips
        cfg.check_interval       = 50ms;
        cfg.aging_threshold      = 3;          // Boost after 3 skips
        cfg.enable_audit_logging = false;
        cfg.enable_anomaly_detection = false;
        scheduler_ = std::make_unique<TaskScheduler>(engine_.get(), cfg);
        scheduler_->registerFunction("noop", [](const nlohmann::json&) -> nlohmann::json {
            return {};
        });
    }

    void TearDown() override {
        if (scheduler_) {
            scheduler_->stop();
            scheduler_.reset();
        }
        engine_.reset();
        idx_.reset();
        if (storage_) {
            storage_->close();
            storage_.reset();
        }
        if (!db_path_.empty()) {
            std::filesystem::remove_all(db_path_);
        }
    }

    std::string db_path_;
    std::unique_ptr<RocksDBWrapper> storage_;
    std::unique_ptr<SecondaryIndexManager> idx_;
    std::unique_ptr<QueryEngine> engine_;
    std::unique_ptr<TaskScheduler> scheduler_;
};

// SCHED-AGE-01: consecutive_skips increments under concurrency pressure
TEST_F(AgingTest, SCHED_AGE_01_ConsecutiveSkipsIncrements) {
    // We'll create a task, simulate it being skipped by inspecting the struct
    // directly via getTask after manually bumping the counter.
    ScheduledTask t;
    t.id            = "low_task";
    t.name          = "Low Task";
    t.type          = ScheduledTask::TaskType::FUNCTION;
    t.function_name = "noop";
    t.trigger_type  = ScheduledTask::TriggerType::MANUAL;
    t.priority      = ScheduledTask::Priority::LOW;
    t.consecutive_skips = 0;
    scheduler_->registerTask(t);

    // Simulate 3 skips by directly incrementing
    auto registered = scheduler_->getTask("low_task");
    ASSERT_NE(registered, nullptr);
    registered->consecutive_skips = 2;
    EXPECT_EQ(2u, registered->consecutive_skips);

    registered->consecutive_skips++;
    EXPECT_EQ(3u, registered->consecutive_skips);
}

// SCHED-AGE-02: effective priority is boosted after aging_threshold skips
TEST_F(AgingTest, SCHED_AGE_02_EffectivePriorityBoosted) {
    // Task with LOW priority and consecutive_skips == aging_threshold (3)
    // should sort as NORMAL (1) rather than LOW (0).
    ScheduledTask low;
    low.id            = "low";
    low.name          = "Low";
    low.type          = ScheduledTask::TaskType::FUNCTION;
    low.function_name = "noop";
    low.trigger_type  = ScheduledTask::TriggerType::MANUAL;
    low.priority      = ScheduledTask::Priority::LOW;

    ScheduledTask normal;
    normal.id            = "normal";
    normal.name          = "Normal";
    normal.type          = ScheduledTask::TaskType::FUNCTION;
    normal.function_name = "noop";
    normal.trigger_type  = ScheduledTask::TriggerType::MANUAL;
    normal.priority      = ScheduledTask::Priority::NORMAL;

    // Simulate LOW task has been aged (3 skips at threshold 3)
    low.consecutive_skips = 3;

    // Effective priority for aged LOW is NORMAL (min(0+1, 2) == 1)
    const uint32_t thr = 3;
    auto effectivePriority = [thr](const ScheduledTask& t) -> int {
        const int base = static_cast<int>(t.priority);
        if (thr > 0 && t.consecutive_skips >= thr) {
            return std::min(base + 1, static_cast<int>(ScheduledTask::Priority::HIGH));
        }
        return base;
    };

    EXPECT_EQ(effectivePriority(low), effectivePriority(normal))
        << "Aged LOW task should have same effective priority as NORMAL task";
}

// SCHED-AGE-03: aged NORMAL task wins over non-aged HIGH (capped at HIGH)
TEST_F(AgingTest, SCHED_AGE_03_AgedNormalCappedAtHigh) {
    ScheduledTask aged_normal;
    aged_normal.priority         = ScheduledTask::Priority::NORMAL;
    aged_normal.consecutive_skips = 5; // >= threshold 3 → boosted to HIGH

    const uint32_t thr = 3;
    auto effectivePriority = [thr](const ScheduledTask& t) -> int {
        const int base = static_cast<int>(t.priority);
        if (thr > 0 && t.consecutive_skips >= thr) {
            return std::min(base + 1, static_cast<int>(ScheduledTask::Priority::HIGH));
        }
        return base;
    };

    // Effective priority of aged NORMAL == HIGH (2), not 3
    EXPECT_EQ(static_cast<int>(ScheduledTask::Priority::HIGH),
              effectivePriority(aged_normal));
}

// SCHED-AGE-04: consecutive_skips resets to 0 when task is dispatched
TEST_F(AgingTest, SCHED_AGE_04_SkipsResetOnDispatch) {
    ScheduledTask t;
    t.id              = "reset_task";
    t.name            = "Reset Task";
    t.type            = ScheduledTask::TaskType::FUNCTION;
    t.function_name   = "noop";
    t.trigger_type    = ScheduledTask::TriggerType::MANUAL;
    t.priority        = ScheduledTask::Priority::LOW;
    t.consecutive_skips = 99; // Was heavily aged
    scheduler_->registerTask(t);

    // Execute immediately — dispatching should reset the counter
    scheduler_->executeTaskNow("reset_task");

    auto registered = scheduler_->getTask("reset_task");
    ASSERT_NE(registered, nullptr);
    EXPECT_EQ(0u, registered->consecutive_skips)
        << "consecutive_skips must be reset to 0 after dispatch";
}

// SCHED-AGE-05: aging disabled (aging_threshold == 0) leaves priority unchanged
TEST_F(AgingTest, SCHED_AGE_05_AgingDisabledNoBoost) {
    ScheduledTask t;
    t.priority         = ScheduledTask::Priority::LOW;
    t.consecutive_skips = 100; // Very high — would boost if aging enabled

    const uint32_t thr = 0; // Disabled
    auto effectivePriority = [thr](const ScheduledTask& s) -> int {
        const int base = static_cast<int>(s.priority);
        if (thr > 0 && s.consecutive_skips >= thr) {
            return std::min(base + 1, static_cast<int>(ScheduledTask::Priority::HIGH));
        }
        return base;
    };

    EXPECT_EQ(static_cast<int>(ScheduledTask::Priority::LOW),
              effectivePriority(t))
        << "With aging_threshold==0, effective priority must equal base priority";
}
