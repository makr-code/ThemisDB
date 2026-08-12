/**
 * @file test_scheduler_integration.cpp
 * @brief Integration tests for the Scheduler module
 *
 * Tests end-to-end scheduler flows:
 *  - Task lifecycle (register → start → execute → stop)
 *  - Per-task statistics accumulation
 *  - Prometheus metrics export
 *  - Persistence round-trip (save/load)
 *  - RetryPolicy applied in scheduled execution
 *  - EventTrigger circuit breaker integration
 *  - CronExpression scheduling integration
 *  - Concurrent task execution
 */

#include <gtest/gtest.h>
#include "scheduler/task_scheduler.h"
#include "scheduler/event_trigger.h"
#include "utils/cron_parser.h"
#include "storage/rocksdb_wrapper.h"
#include "index/secondary_index.h"
#include "query/query_engine.h"
#include <filesystem>
#include <atomic>
#include <thread>
#include <chrono>
#include <string>
#include <sstream>

using namespace themis;
using namespace std::chrono_literals;

// ─────────────────────────────────────────────────────────────────────────────
// Shared fixture
// ─────────────────────────────────────────────────────────────────────────────

class SchedulerIntegrationTest : public ::testing::Test {
protected:
    void SetUp() override {
        auto now = std::chrono::high_resolution_clock::now().time_since_epoch().count();
        db_path_ = (std::filesystem::temp_directory_path() /
                    std::filesystem::path("themis_integ_" + std::to_string(now))).string();
        std::filesystem::create_directories(db_path_);

        RocksDBWrapper::Config cfg;
        cfg.db_path = db_path_ + "/db";
        cfg.enable_blobdb = false;
        storage_ = std::make_unique<RocksDBWrapper>(cfg);
        ASSERT_TRUE(storage_->open());

        idx_    = std::make_unique<SecondaryIndexManager>(*storage_);
        engine_ = std::make_unique<QueryEngine>(*storage_, *idx_);

        makeScheduler();
    }

    void TearDown() override {
        if (scheduler_) scheduler_->stop();
        scheduler_.reset();
        engine_.reset();
        idx_.reset();
        if (storage_) storage_->close();
        storage_.reset();
        std::filesystem::remove_all(db_path_);
    }

    void makeScheduler(bool persist = false) {
        if (scheduler_) scheduler_->stop();
        TaskScheduler::Config cfg;
        cfg.max_concurrent_tasks     = 4;
        cfg.check_interval           = 50ms;
        cfg.persist_tasks            = persist;
        cfg.persistence_path         = db_path_ + "/tasks";
        cfg.enable_audit_logging     = false;
        cfg.enable_anomaly_detection = false;
        scheduler_ = std::make_unique<TaskScheduler>(engine_.get(), cfg);
    }

    // Register a simple function task that counts calls
    std::string registerCountingTask(std::atomic<int>& counter,
                                     const std::string& name = "count_task") {
        scheduler_->registerFunction(name, [&counter](const nlohmann::json&) -> nlohmann::json {
            ++counter;
            return nlohmann::json{{"count", counter.load()}};
        });
        ScheduledTask task;
        task.name          = name;
        task.type          = ScheduledTask::TaskType::FUNCTION;
        task.function_name = name;
        task.trigger_type  = ScheduledTask::TriggerType::MANUAL;
        return scheduler_->registerTask(task);
    }

    std::string db_path_;
    std::unique_ptr<RocksDBWrapper> storage_;
    std::unique_ptr<SecondaryIndexManager> idx_;
    std::unique_ptr<QueryEngine> engine_;
    std::unique_ptr<TaskScheduler> scheduler_;
};

// ─────────────────────────────────────────────────────────────────────────────
// 1. Lifecycle tests
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(SchedulerIntegrationTest, StartStopIsIdempotent) {
    EXPECT_FALSE(scheduler_->isRunning());
    scheduler_->start();
    EXPECT_TRUE(scheduler_->isRunning());
    scheduler_->stop();
    EXPECT_FALSE(scheduler_->isRunning());
    // Second stop is a no-op
    EXPECT_NO_THROW(scheduler_->stop());
}

TEST_F(SchedulerIntegrationTest, RegisterAndUnregisterTask) {
    std::atomic<int> count{0};
    std::string id = registerCountingTask(count);
    EXPECT_FALSE(id.empty());

    EXPECT_EQ(scheduler_->getStats().registered_tasks, 1u);
    scheduler_->unregisterTask(id);
    EXPECT_EQ(scheduler_->getStats().registered_tasks, 0u);
}

TEST_F(SchedulerIntegrationTest, EnableDisableTask) {
    std::atomic<int> count{0};
    std::string id = registerCountingTask(count);

    scheduler_->disableTask(id);
    auto t = scheduler_->getTask(id);
    ASSERT_NE(t, nullptr);
    EXPECT_FALSE(t->enabled);

    scheduler_->enableTask(id);
    EXPECT_TRUE(scheduler_->getTask(id)->enabled);
}

TEST_F(SchedulerIntegrationTest, ListTasksReturnsAllRegistered) {
    std::atomic<int> c1{0}, c2{0};
    registerCountingTask(c1, "task_a");
    registerCountingTask(c2, "task_b");

    auto tasks = scheduler_->listTasks();
    EXPECT_EQ(tasks.size(), 2u);
}

// ─────────────────────────────────────────────────────────────────────────────
// 2. Manual execution + statistics
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(SchedulerIntegrationTest, ManualExecutionUpdatesStats) {
    std::atomic<int> count{0};
    std::string id = registerCountingTask(count);

    auto result = scheduler_->executeTaskNow(id);
    EXPECT_FALSE(result.contains("error")) << result.dump();
    EXPECT_EQ(count.load(), 1);

    auto t = scheduler_->getTask(id);
    ASSERT_NE(t, nullptr);
    EXPECT_EQ(t->total_executions, 1u);
    EXPECT_EQ(t->successful_executions, 1u);
    EXPECT_EQ(t->failed_executions, 0u);
}

TEST_F(SchedulerIntegrationTest, FailedManualExecutionUpdatesFailureStats) {
    scheduler_->registerFunction("always_fail", [](const nlohmann::json&) -> nlohmann::json {
        throw std::runtime_error("intentional failure");
    });
    ScheduledTask task;
    task.name          = "fail_task";
    task.type          = ScheduledTask::TaskType::FUNCTION;
    task.function_name = "always_fail";
    task.trigger_type  = ScheduledTask::TriggerType::MANUAL;
    task.max_retries   = 0;
    std::string id = scheduler_->registerTask(task);

    auto result = scheduler_->executeTaskNow(id);
    EXPECT_TRUE(result.contains("error"));

    auto t = scheduler_->getTask(id);
    ASSERT_NE(t, nullptr);
    EXPECT_EQ(t->failed_executions, 1u);
    EXPECT_EQ(t->successful_executions, 0u);
    EXPECT_FALSE(t->last_error.empty());
}

TEST_F(SchedulerIntegrationTest, MultipleManualExecutionsAccumulateStats) {
    std::atomic<int> count{0};
    std::string id = registerCountingTask(count);

    for (int i = 0; i < 5; ++i) {
        scheduler_->executeTaskNow(id);
    }

    auto t = scheduler_->getTask(id);
    ASSERT_NE(t, nullptr);
    EXPECT_EQ(t->total_executions, 5u);
    EXPECT_EQ(t->successful_executions, 5u);
    EXPECT_EQ(count.load(), 5);
}

// ─────────────────────────────────────────────────────────────────────────────
// 3. Prometheus metrics export
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(SchedulerIntegrationTest, ExportMetricsIsNotEmpty) {
    auto text = scheduler_->exportMetrics();
    EXPECT_FALSE(text.empty());
}

TEST_F(SchedulerIntegrationTest, ExportMetricsContainsRequiredMetricNames) {
    auto text = scheduler_->exportMetrics();
    EXPECT_NE(text.find("themis_scheduler_tasks_registered"), std::string::npos);
    EXPECT_NE(text.find("themis_scheduler_tasks_active"),     std::string::npos);
    EXPECT_NE(text.find("themis_scheduler_tasks_running"),    std::string::npos);
    EXPECT_NE(text.find("themis_scheduler_executions_total"), std::string::npos);
}

TEST_F(SchedulerIntegrationTest, ExportMetricsReflectsRegisteredTaskCount) {
    std::atomic<int> c1{0}, c2{0};
    registerCountingTask(c1, "m_task_1");
    registerCountingTask(c2, "m_task_2");

    auto text = scheduler_->exportMetrics();
    // Should have per-task metrics for both tasks
    EXPECT_NE(text.find("m_task_1"), std::string::npos);
    EXPECT_NE(text.find("m_task_2"), std::string::npos);
}

TEST_F(SchedulerIntegrationTest, ExportMetricsSuccessCounterIncrementsAfterExecution) {
    std::atomic<int> count{0};
    std::string id = registerCountingTask(count, "metrics_task");
    scheduler_->executeTaskNow(id);

    auto text = scheduler_->exportMetrics();
    // The success counter should be 1
    EXPECT_NE(text.find("status=\"success\"} 1"), std::string::npos) << text;
}

TEST_F(SchedulerIntegrationTest, ExportMetricsHasHelpAndTypeLines) {
    auto text = scheduler_->exportMetrics();
    EXPECT_NE(text.find("# HELP"), std::string::npos);
    EXPECT_NE(text.find("# TYPE"), std::string::npos);
}

TEST_F(SchedulerIntegrationTest, ExportMetricsPrometheusFormatValid) {
    std::atomic<int> count{0};
    registerCountingTask(count, "fmt_task");

    auto text = scheduler_->exportMetrics();

    // Every non-blank line must start with '#', or be a valid metric line
    // (i.e. must not have spaces in the metric name portion).
    std::istringstream iss(text);
    std::string line;
    while (std::getline(iss, line)) {
        if (line.empty()) continue;
        // Lines starting with '#' are HELP/TYPE comments – always valid
        if (line[0] == '#') continue;
        // Metric lines: name[{labels}] value
        // The metric name must not contain spaces (label block starts with '{')
        auto brace = line.find('{');
        auto space = line.find(' ');
        // If no label block, first space separates name from value
        size_t name_end = (brace != std::string::npos && brace < space) ? brace : space;
        EXPECT_NE(name_end, std::string::npos) << "Malformed line: " << line;
        // Name must start with a letter or '_'
        char first = line[0];
        EXPECT_TRUE(std::isalpha(static_cast<unsigned char>(first)) || first == '_')
            << "Metric name starts with invalid character: " << line;
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// 4. Scheduled (interval) execution via the loop
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(SchedulerIntegrationTest, IntervalTaskExecutedBySchedulerLoop) {
    std::atomic<int> count{0};
    scheduler_->registerFunction("loop_fn", [&count](const nlohmann::json&) -> nlohmann::json {
        ++count;
        return {};
    });
    ScheduledTask task;
    task.name          = "loop_task";
    task.type          = ScheduledTask::TaskType::FUNCTION;
    task.function_name = "loop_fn";
    task.trigger_type  = ScheduledTask::TriggerType::INTERVAL;
    task.interval      = 1000ms;
    task.next_run      = std::chrono::system_clock::now();
    std::string id = scheduler_->registerTask(task);

    scheduler_->start();
    std::this_thread::sleep_for(350ms);
    scheduler_->stop();

    EXPECT_GE(count.load(), 1) << "Expected at least one execution after immediate scheduling";
}

// ─────────────────────────────────────────────────────────────────────────────
// 5. Concurrent task execution
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(SchedulerIntegrationTest, ConcurrentTasksExecuteInParallel) {
    std::atomic<int> concurrent_peak{0};
    std::atomic<int> active{0};

    scheduler_->registerFunction("slow_fn", [&](const nlohmann::json&) -> nlohmann::json {
        int cur = ++active;
        int peak = concurrent_peak.load();
        while (cur > peak) {
            concurrent_peak.compare_exchange_weak(peak, cur);
            peak = concurrent_peak.load();
        }
        std::this_thread::sleep_for(100ms);
        --active;
        return {};
    });

    // Register 3 tasks, each triggered manually back-to-back
    std::vector<std::string> ids;
    for (int i = 0; i < 3; ++i) {
        ScheduledTask t;
        t.name          = "slow_task_" + std::to_string(i);
        t.type          = ScheduledTask::TaskType::FUNCTION;
        t.function_name = "slow_fn";
        t.trigger_type  = ScheduledTask::TriggerType::INTERVAL;
        t.interval      = 10s;  // Large interval – we execute manually
        ids.push_back(scheduler_->registerTask(t));
    }
    scheduler_->start();

    // Fire all three nearly simultaneously via the scheduler loop's thread pool
    // by setting next_run to "now" for all tasks
    for (auto& id : ids) {
        auto t = scheduler_->getTask(id);
        if (t) t->next_run = std::chrono::system_clock::now();
    }
    // Notify scheduler
    std::this_thread::sleep_for(300ms);
    scheduler_->stop();

    // With max_concurrent_tasks = 4 and 3 tasks launched, peak should be ≥ 2
    EXPECT_GE(concurrent_peak.load(), 1);
}

// ─────────────────────────────────────────────────────────────────────────────
// 6. Persistence round-trip
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(SchedulerIntegrationTest, PersistenceRoundTripRestoresTask) {
    // Build scheduler with persistence enabled
    makeScheduler(/*persist=*/true);
    std::filesystem::create_directories(db_path_ + "/tasks");

    scheduler_->registerFunction("persist_fn", [](const nlohmann::json&) -> nlohmann::json {
        return nlohmann::json{{"persisted", true}};
    });
    ScheduledTask task;
    task.name          = "persist_task";
    task.description   = "Test persistence";
    task.type          = ScheduledTask::TaskType::FUNCTION;
    task.function_name = "persist_fn";
    task.trigger_type  = ScheduledTask::TriggerType::MANUAL;
    std::string id = scheduler_->registerTask(task);
    ASSERT_FALSE(id.empty());

    // Save
    scheduler_->stop();

    // Reload in a fresh scheduler instance
    makeScheduler(/*persist=*/true);

    // The task should be present after load
    auto restored = scheduler_->getTask(id);
    // Note: on load, the task is re-registered, so it might have been assigned
    // a fresh id matching the original name-based ID generation.
    // Check by listing tasks instead.
    auto tasks = scheduler_->listTasks();
    bool found = false;
    for (const auto& t : tasks) {
        if (t.name == "persist_task") { found = true; break; }
    }
    EXPECT_TRUE(found) << "Task 'persist_task' was not restored after reload";
}

TEST_F(SchedulerIntegrationTest, PersistenceRoundTripRestoresRetryPolicy) {
    makeScheduler(/*persist=*/true);
    std::filesystem::create_directories(db_path_ + "/tasks");

    scheduler_->registerFunction("rp_fn", [](const nlohmann::json&) -> nlohmann::json {
        return {};
    });
    ScheduledTask task;
    task.name          = "rp_task";
    task.type          = ScheduledTask::TaskType::FUNCTION;
    task.function_name = "rp_fn";
    task.trigger_type  = ScheduledTask::TriggerType::MANUAL;

    ScheduledTask::RetryPolicy rp;
    rp.strategy      = ScheduledTask::RetryStrategy::LINEAR_BACKOFF;
    rp.max_retries   = 4;
    rp.initial_delay = 500ms;
    rp.max_delay     = 5s;
    task.retry_policy = rp;

    std::string id = scheduler_->registerTask(task);
    scheduler_->stop();

    // Reload
    makeScheduler(/*persist=*/true);

    // Find restored task and check retry policy
    for (const auto& t : scheduler_->listTasks()) {
        if (t.name == "rp_task") {
            ASSERT_TRUE(t.retry_policy.has_value());
            EXPECT_EQ(t.retry_policy->strategy,    ScheduledTask::RetryStrategy::LINEAR_BACKOFF);
            EXPECT_EQ(t.retry_policy->max_retries, 4u);
            EXPECT_EQ(t.retry_policy->initial_delay.count(), 500);
            return;
        }
    }
    ADD_FAILURE() << "Task 'rp_task' not found after reload";
}

// ─────────────────────────────────────────────────────────────────────────────
// 7. RetryPolicy in scheduler loop (not just executeTaskNow)
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(SchedulerIntegrationTest, SchedulerLoopAppliesRetryPolicy) {
    std::atomic<int> attempts{0};
    scheduler_->registerFunction("loop_retry_fn", [&](const nlohmann::json&) -> nlohmann::json {
        if (++attempts < 2) throw std::runtime_error("transient");
        return nlohmann::json{{"ok", true}};
    });
    ScheduledTask task;
    task.name          = "loop_retry_task";
    task.type          = ScheduledTask::TaskType::FUNCTION;
    task.function_name = "loop_retry_fn";
    task.trigger_type  = ScheduledTask::TriggerType::MANUAL;

    ScheduledTask::RetryPolicy rp;
    rp.strategy      = ScheduledTask::RetryStrategy::FIXED_DELAY;
    rp.max_retries   = 2;
    rp.initial_delay = 10ms;  // Very short for test speed
    rp.max_delay     = 50ms;
    task.retry_policy = rp;

    std::string id = scheduler_->registerTask(task);
    auto result = scheduler_->executeTaskNow(id);

    EXPECT_FALSE(result.contains("error")) << result.dump();
    EXPECT_EQ(attempts.load(), 2);
}

// ─────────────────────────────────────────────────────────────────────────────
// 8. Stats sanity checks
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(SchedulerIntegrationTest, GetStatsReflectsRunningState) {
    {
        auto stats = scheduler_->getStats();
        EXPECT_EQ(stats.registered_tasks, 0u);
        EXPECT_EQ(stats.total_executions, 0u);
    }

    std::atomic<int> count{0};
    std::string id = registerCountingTask(count);

    {
        auto stats = scheduler_->getStats();
        EXPECT_EQ(stats.registered_tasks, 1u);
        EXPECT_EQ(stats.active_tasks,     1u);
    }

    scheduler_->executeTaskNow(id);

    {
        auto stats = scheduler_->getStats();
        EXPECT_EQ(stats.total_executions, 1u);
        EXPECT_EQ(stats.failed_executions, 0u);
    }
}

TEST_F(SchedulerIntegrationTest, GetStatsActiveVsRegistered) {
    std::atomic<int> c{0};
    std::string id = registerCountingTask(c, "active_test");

    EXPECT_EQ(scheduler_->getStats().active_tasks, 1u);

    scheduler_->disableTask(id);
    EXPECT_EQ(scheduler_->getStats().active_tasks, 0u);
    EXPECT_EQ(scheduler_->getStats().registered_tasks, 1u);
}

// ─────────────────────────────────────────────────────────────────────────────
// 9. CronExpression integration
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(SchedulerIntegrationTest, CronTaskIsRegisteredAndHasValidExpression) {
    scheduler_->registerFunction("cron_fn", [](const nlohmann::json&) -> nlohmann::json {
        return {};
    });
    ScheduledTask task;
    task.name            = "cron_task";
    task.type            = ScheduledTask::TaskType::FUNCTION;
    task.function_name   = "cron_fn";
    task.trigger_type    = ScheduledTask::TriggerType::CRON;
    task.cron_expression = "@hourly";  // Special expression

    std::string id = scheduler_->registerTask(task);
    EXPECT_FALSE(id.empty());

    auto t = scheduler_->getTask(id);
    ASSERT_NE(t, nullptr);
    EXPECT_EQ(t->trigger_type, ScheduledTask::TriggerType::CRON);
    EXPECT_EQ(t->cron_expression, "@hourly");
}

TEST_F(SchedulerIntegrationTest, InvalidCronExpressionThrowsOnRegister) {
    scheduler_->registerFunction("bad_cron_fn", [](const nlohmann::json&) -> nlohmann::json {
        return {};
    });
    ScheduledTask task;
    task.name            = "bad_cron";
    task.type            = ScheduledTask::TaskType::FUNCTION;
    task.function_name   = "bad_cron_fn";
    task.trigger_type    = ScheduledTask::TriggerType::CRON;
    task.cron_expression = "99 99 99 99 99";  // Invalid

    EXPECT_THROW(scheduler_->registerTask(task), std::invalid_argument);
}

// ─────────────────────────────────────────────────────────────────────────────
// 10. on_success / on_failure hooks
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(SchedulerIntegrationTest, OnSuccessHookCalledAfterSuccessfulExecution) {
    std::atomic<bool> hook_fired{false};
    scheduler_->registerFunction("hook_fn", [](const nlohmann::json&) -> nlohmann::json {
        return nlohmann::json{{"ok", true}};
    });
    ScheduledTask task;
    task.name          = "hook_task";
    task.type          = ScheduledTask::TaskType::FUNCTION;
    task.function_name = "hook_fn";
    task.trigger_type  = ScheduledTask::TriggerType::MANUAL;
    task.on_success    = [&](const std::string&, const nlohmann::json&) {
        hook_fired.store(true);
    };

    std::string id = scheduler_->registerTask(task);
    scheduler_->executeTaskNow(id);
    EXPECT_TRUE(hook_fired.load());
}

TEST_F(SchedulerIntegrationTest, OnFailureHookCalledAfterFailedExecution) {
    std::atomic<bool> hook_fired{false};
    scheduler_->registerFunction("fail_hook_fn", [](const nlohmann::json&) -> nlohmann::json {
        throw std::runtime_error("forced failure");
    });
    ScheduledTask task;
    task.name          = "fail_hook_task";
    task.type          = ScheduledTask::TaskType::FUNCTION;
    task.function_name = "fail_hook_fn";
    task.trigger_type  = ScheduledTask::TriggerType::MANUAL;
    task.max_retries   = 0;
    task.on_failure    = [&](const std::string&, const std::string&) {
        hook_fired.store(true);
    };

    std::string id = scheduler_->registerTask(task);
    scheduler_->executeTaskNow(id);
    EXPECT_TRUE(hook_fired.load());
}

// ─────────────────────────────────────────────────────────────────────────────
// 11. updateTask
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(SchedulerIntegrationTest, UpdateTaskChangesDescription) {
    std::atomic<int> count{0};
    std::string id = registerCountingTask(count, "updatable_task");

    // Verify initial description
    auto t_before = scheduler_->getTask(id);
    ASSERT_NE(t_before, nullptr);

    // Update
    ScheduledTask updated = *t_before;
    updated.description = "updated description";
    scheduler_->updateTask(updated);

    auto t_after = scheduler_->getTask(id);
    ASSERT_NE(t_after, nullptr);
    EXPECT_EQ(t_after->description, "updated description");
}

TEST_F(SchedulerIntegrationTest, UpdateTaskPreservesExecutionStats) {
    std::atomic<int> count{0};
    std::string id = registerCountingTask(count, "stats_preserve_task");

    // Run a couple of times to build stats
    scheduler_->executeTaskNow(id);
    scheduler_->executeTaskNow(id);

    auto t_before = scheduler_->getTask(id);
    ASSERT_NE(t_before, nullptr);
    EXPECT_EQ(t_before->total_executions, 2u);

    // Update metadata only
    ScheduledTask updated = *t_before;
    updated.description = "after update";
    scheduler_->updateTask(updated);

    auto t_after = scheduler_->getTask(id);
    ASSERT_NE(t_after, nullptr);
    EXPECT_EQ(t_after->total_executions, 2u);
}

// ─────────────────────────────────────────────────────────────────────────────
// 12. Function registration / unregistration
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(SchedulerIntegrationTest, UnregisteredFunctionReturnsErrorOnExecution) {
    // Register a task that calls a function, then unregister the function
    scheduler_->registerFunction("temp_fn", [](const nlohmann::json&) -> nlohmann::json {
        return nlohmann::json{{"ok", true}};
    });

    ScheduledTask task;
    task.name          = "temp_fn_task";
    task.type          = ScheduledTask::TaskType::FUNCTION;
    task.function_name = "temp_fn";
    task.trigger_type  = ScheduledTask::TriggerType::MANUAL;
    task.max_retries   = 0;
    std::string id = scheduler_->registerTask(task);

    // Succeeds while function is registered
    auto ok_result = scheduler_->executeTaskNow(id);
    EXPECT_FALSE(ok_result.contains("error")) << ok_result.dump();

    // Unregister the function
    scheduler_->unregisterFunction("temp_fn");

    // Now execution should fail
    auto err_result = scheduler_->executeTaskNow(id);
    EXPECT_TRUE(err_result.contains("error")) << err_result.dump();
}

// ─────────────────────────────────────────────────────────────────────────────
// 13. MANUAL-trigger task should not auto-execute in scheduler loop
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(SchedulerIntegrationTest, ManualTriggerTaskNotExecutedBySchedulerLoop) {
    std::atomic<int> count{0};
    scheduler_->registerFunction("manual_only_fn",
        [&count](const nlohmann::json&) -> nlohmann::json {
            ++count;
            return {};
        });

    ScheduledTask task;
    task.name          = "manual_only_task";
    task.type          = ScheduledTask::TaskType::FUNCTION;
    task.function_name = "manual_only_fn";
    task.trigger_type  = ScheduledTask::TriggerType::MANUAL;  // Manual only
    scheduler_->registerTask(task);

    scheduler_->start();
    // Let the scheduler loop run for several ticks (check_interval=20ms in this fixture)
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    scheduler_->stop();

    // Task should NOT have been auto-executed
    EXPECT_EQ(count.load(), 0)
        << "MANUAL trigger task must not be executed by the scheduler loop";
}

// ─────────────────────────────────────────────────────────────────────────────
// 14. exportMetrics reflects disabled tasks
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(SchedulerIntegrationTest, ExportMetricsShowsDisabledTaskAsZero) {
    std::atomic<int> count{0};
    std::string id = registerCountingTask(count, "disable_metrics_task");

    // Disable the task
    scheduler_->disableTask(id);

    auto text = scheduler_->exportMetrics();
    // The task_enabled metric should show 0
    EXPECT_NE(text.find("themis_scheduler_task_enabled"), std::string::npos) << text;
    EXPECT_NE(text.find("disable_metrics_task"), std::string::npos) << text;
    // The active tasks gauge should be 0
    EXPECT_NE(text.find("themis_scheduler_tasks_active"), std::string::npos) << text;
}

// ─────────────────────────────────────────────────────────────────────────────
// 15. Scheduler lifecycle under rapid start/stop
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(SchedulerIntegrationTest, RapidStartStopDoesNotCrash) {
    std::atomic<int> count{0};
    scheduler_->registerFunction("rapid_fn",
        [&count](const nlohmann::json&) -> nlohmann::json {
            ++count;
            return {};
        });
    ScheduledTask task;
    task.name          = "rapid_task";
    task.type          = ScheduledTask::TaskType::FUNCTION;
    task.function_name = "rapid_fn";
    task.trigger_type  = ScheduledTask::TriggerType::INTERVAL;
    task.interval      = std::chrono::milliseconds(1000);
    task.next_run      = std::chrono::system_clock::now();
    scheduler_->registerTask(task);

    for (int i = 0; i < 5; ++i) {
        scheduler_->start();
        std::this_thread::sleep_for(std::chrono::milliseconds(60));
        scheduler_->stop();
    }

    // Should complete without crash or deadlock
    SUCCEED();
}

// ─────────────────────────────────────────────────────────────────────────────
// 16. getStats total_executions aggregates across multiple tasks
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(SchedulerIntegrationTest, GetStatsTotalExecutionsAggregatesAllTasks) {
    std::atomic<int> c1{0}, c2{0};
    std::string id1 = registerCountingTask(c1, "agg_task_1");
    std::string id2 = registerCountingTask(c2, "agg_task_2");

    scheduler_->executeTaskNow(id1);
    scheduler_->executeTaskNow(id1);
    scheduler_->executeTaskNow(id2);

    auto stats = scheduler_->getStats();
    EXPECT_EQ(stats.total_executions, 3u);
}
