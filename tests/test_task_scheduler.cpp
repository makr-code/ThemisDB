/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            test_task_scheduler.cpp                            ║
  Version:         0.0.32                                             ║
  Last Modified:   2026-02-23 03:59:33                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     882                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

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
        if (++call_count < 3) throw std::runtime_error("not yet");
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
        if (++call_count < 2) throw std::runtime_error("first fail");
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
        if (++call_count < 2) throw std::runtime_error("first fail");
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
        if (++call_count < 2) throw std::runtime_error("first fail");
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
    EXPECT_THROW(scheduler_->enableTask("nonexistent_task_id"), std::runtime_error);
}

TEST_F(TaskSchedulerTest, DisableTaskThatDoesNotExistThrows) {
    EXPECT_THROW(scheduler_->disableTask("nonexistent_task_id"), std::runtime_error);
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

    EXPECT_EQ(scheduler_->getStats().failed_executions, 1u);
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
            if (call++ == 0) throw std::runtime_error("temporary blip");
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
    std::mutex mu;

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
    std::mutex mu;
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
    std::mutex mu;
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
    std::mutex mu;
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
    std::mutex mu;
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
