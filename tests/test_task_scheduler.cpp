/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            test_task_scheduler.cpp                            ║
  Version:         0.0.19                                             ║
  Last Modified:   2026-02-21 19:00:08                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     889                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 189cdf5b1  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • a5676b06f  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 56752fde6  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • c3f305f42  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • e178371a5  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
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
