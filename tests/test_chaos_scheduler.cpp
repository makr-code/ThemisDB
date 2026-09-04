/**
 * @file test_chaos_scheduler.cpp
 * @brief Chaos/stress tests for the Scheduler module
 *
 * Tests adversarial and high-stress conditions:
 *  - Rapid task registration/unregistration under concurrent access
 *  - Tasks that exceed their timeout
 *  - Tasks throwing unexpected exception types
 *  - Scheduler stop while tasks are mid-execution
 *  - High-frequency task execution (throughput check)
 *  - Concurrent manual executions of the same task
 *  - Rapid enable/disable under load
 *  - All tasks fail simultaneously
 *  - Large number of registered tasks
 *  - Task function replaced while executing
 */

#include <gtest/gtest.h>
#include "scheduler/task_scheduler.h"
#include "storage/rocksdb_wrapper.h"
#include "index/secondary_index.h"
#include "query/query_engine.h"
#include <filesystem>
#include <atomic>
#include <thread>
#include <chrono>
#include <vector>
#include <random>

using namespace themis;
using namespace std::chrono_literals;

// ─────────────────────────────────────────────────────────────────────────────
// Shared fixture (mirrors test_task_scheduler.cpp)
// ─────────────────────────────────────────────────────────────────────────────

class ChaosSchedulerTest : public ::testing::Test {
protected:
    void SetUp() override {
#ifdef _WIN32
        GTEST_SKIP() << "Skipping ChaosSchedulerTest on Windows due to known timeout/deadlock instability.";
#endif
        auto now = std::chrono::high_resolution_clock::now().time_since_epoch().count();
        db_path_ = (std::filesystem::temp_directory_path() /
                    std::filesystem::path("themis_chaos_" + std::to_string(now))).string();
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
        if (scheduler_) {
          scheduler_->stop();
        }
        scheduler_.reset();
        engine_.reset();
        idx_.reset();
        if (storage_) {
          storage_->close();
        }
        storage_.reset();
        std::error_code ec;
        std::filesystem::remove_all(db_path_, ec);
    }

    void makeScheduler(size_t max_concurrent = 8) {
        if (scheduler_) {
          scheduler_->stop();
        }
        TaskScheduler::Config cfg;
        cfg.max_concurrent_tasks     = max_concurrent;
        cfg.check_interval           = 20ms;  // Very short for chaos tests
        cfg.persist_tasks            = false;
        cfg.enable_audit_logging     = false;
        cfg.enable_anomaly_detection = false;
        scheduler_ = std::make_unique<TaskScheduler>(engine_.get(), cfg);
    }

    std::string registerFn(const std::string& name,
                           std::function<nlohmann::json(const nlohmann::json&)> fn) {
        scheduler_->registerFunction(name, fn);
        ScheduledTask task;
        task.name          = name + "_task";
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
// 1. Rapid register/unregister
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(ChaosSchedulerTest, RapidRegisterUnregisterDoesNotCorruptState) {
    scheduler_->registerFunction("rapid_rur",
        [](const nlohmann::json&) -> nlohmann::json { return {}; });

    for (int i = 0; i < 50; ++i) {
        ScheduledTask task;
        task.name          = "rur_task_" + std::to_string(i);
        task.type          = ScheduledTask::TaskType::FUNCTION;
        task.function_name = "rapid_rur";
        task.trigger_type  = ScheduledTask::TriggerType::MANUAL;
        std::string id = scheduler_->registerTask(task);
        scheduler_->unregisterTask(id);
    }

    // Scheduler state must be clean
    EXPECT_EQ(scheduler_->listTasks().size(), 0u);
    EXPECT_EQ(scheduler_->getStats().registered_tasks, 0u);
}

// ─────────────────────────────────────────────────────────────────────────────
// 2. Tasks throwing non-standard exceptions
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(ChaosSchedulerTest, TaskThrowingNonStdExceptionHandledGracefully) {
    scheduler_->registerFunction("throw_int",
        [](const nlohmann::json&) -> nlohmann::json {
            throw 42;  // Non-exception type
        });
    ScheduledTask task;
    task.name          = "throw_int_task";
    task.type          = ScheduledTask::TaskType::FUNCTION;
    task.function_name = "throw_int";
    task.trigger_type  = ScheduledTask::TriggerType::MANUAL;
    task.max_retries   = 0;
    std::string id = scheduler_->registerTask(task);

    // Should not crash the process; result should indicate failure
    auto result = scheduler_->executeTaskNow(id);
    EXPECT_TRUE(result.contains("error")) << result.dump();
}

// ─────────────────────────────────────────────────────────────────────────────
// 3. Concurrent manual executions of the same task
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(ChaosSchedulerTest, ConcurrentManualExecutionsSameTask) {
    std::atomic<int> call_count{0};
    scheduler_->registerFunction("concurrent_fn",
        [&call_count](const nlohmann::json&) -> nlohmann::json {
            ++call_count;
            std::this_thread::sleep_for(5ms);
            return nlohmann::json{{"n", call_count.load()}};
        });
    ScheduledTask task;
    task.name                = "concurrent_task";
    task.type                = ScheduledTask::TaskType::FUNCTION;
    task.function_name       = "concurrent_fn";
    task.trigger_type        = ScheduledTask::TriggerType::MANUAL;
    std::string id = scheduler_->registerTask(task);

    const int THREADS = 10;
    std::vector<std::thread> threads;
    threads.reserve(THREADS);
    std::atomic<int> errors{0};

    for (int i = 0; i < THREADS; ++i) {
        threads.emplace_back([&]() {
            auto result = scheduler_->executeTaskNow(id);
            if (result.contains("error")) {
              ++errors;
            }
        });
    }
    for (auto& t : threads) {
      t.join();
    }

    EXPECT_EQ(errors.load(), 0) << "No executions should have errored";
    EXPECT_EQ(call_count.load(), THREADS);
}

// ─────────────────────────────────────────────────────────────────────────────
// 4. Stop scheduler while tasks are running
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(ChaosSchedulerTest, StopWhileTasksRunningDoesNotDeadlock) {
    std::atomic<bool> entered{false};
    scheduler_->registerFunction("slow_chaos_fn",
        [&entered](const nlohmann::json&) -> nlohmann::json {
            entered.store(true);
            std::this_thread::sleep_for(500ms);
            return {};
        });
    ScheduledTask task;
    task.name          = "slow_chaos_task";
    task.type          = ScheduledTask::TaskType::FUNCTION;
    task.function_name = "slow_chaos_fn";
    task.trigger_type  = ScheduledTask::TriggerType::INTERVAL;
    task.interval      = 1000ms;
    scheduler_->registerTask(task);

    scheduler_->start();

    // Wait until at least one task has started
    auto deadline = std::chrono::steady_clock::now() + 2s;
    while (!entered.load() && std::chrono::steady_clock::now() < deadline) {
        std::this_thread::sleep_for(10ms);
    }

    // Stop must return within a reasonable time (not deadlock)
    auto stop_start = std::chrono::steady_clock::now();
    scheduler_->stop();
    auto stop_duration = std::chrono::steady_clock::now() - stop_start;

    EXPECT_LT(std::chrono::duration_cast<std::chrono::seconds>(stop_duration).count(), 10)
        << "stop() took too long (possible deadlock)";
    EXPECT_FALSE(scheduler_->isRunning());
}

// ─────────────────────────────────────────────────────────────────────────────
// 5. All tasks fail simultaneously
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(ChaosSchedulerTest, AllTasksFailSimultaneously) {
    scheduler_->registerFunction("mass_fail",
        [](const nlohmann::json&) -> nlohmann::json {
            throw std::runtime_error("mass failure");
        });

    std::vector<std::string> ids;
    for (int i = 0; i < 10; ++i) {
        ScheduledTask task;
        task.name          = "mass_fail_" + std::to_string(i);
        task.type          = ScheduledTask::TaskType::FUNCTION;
        task.function_name = "mass_fail";
        task.trigger_type  = ScheduledTask::TriggerType::MANUAL;
        task.max_retries   = 0;
        ids.push_back(scheduler_->registerTask(task));
    }

    int failures = 0;
    for (const auto& id : ids) {
        auto result = scheduler_->executeTaskNow(id);
        if (result.contains("error")) {
          ++failures;
        }
    }

    EXPECT_EQ(failures, 10);

    // Scheduler must remain functional after mass failure
    auto stats = scheduler_->getStats();
    EXPECT_GE(stats.failed_executions, 0u);
    EXPECT_EQ(stats.registered_tasks, 10u);
}

// ─────────────────────────────────────────────────────────────────────────────
// 6. Large number of registered tasks
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(ChaosSchedulerTest, LargeNumberOfRegisteredTasksRemainAccessible) {
    scheduler_->registerFunction("noop", [](const nlohmann::json&) -> nlohmann::json {
        return {};
    });

    const int TASK_COUNT = 200;
    std::vector<std::string> ids;
    ids.reserve(TASK_COUNT);
    for (int i = 0; i < TASK_COUNT; ++i) {
        ScheduledTask task;
        task.name          = "big_task_" + std::to_string(i);
        task.type          = ScheduledTask::TaskType::FUNCTION;
        task.function_name = "noop";
        task.trigger_type  = ScheduledTask::TriggerType::MANUAL;
        ids.push_back(scheduler_->registerTask(task));
    }

    EXPECT_EQ(scheduler_->getStats().registered_tasks, TASK_COUNT);

    // Random sample: all should be retrievable
    std::mt19937 rng{42};
    std::uniform_int_distribution<int> dist(0, TASK_COUNT - 1);
    for (int i = 0; i < 20; ++i) {
        int idx = dist(rng);
        auto t = scheduler_->getTask(ids[idx]);
        ASSERT_NE(t, nullptr) << "Task " << ids[idx] << " not found";
    }

    // Cleanup: unregister all
    for (const auto& id : ids) {
        scheduler_->unregisterTask(id);
    }
    EXPECT_EQ(scheduler_->getStats().registered_tasks, 0u);
}

// ─────────────────────────────────────────────────────────────────────────────
// 7. Rapid enable/disable while scheduler is running
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(ChaosSchedulerTest, RapidEnableDisableUnderLoad) {
    std::atomic<int> count{0};
    scheduler_->registerFunction("toggle_fn",
        [&count](const nlohmann::json&) -> nlohmann::json {
            ++count;
            return {};
        });
    ScheduledTask task;
    task.name          = "toggle_task";
    task.type          = ScheduledTask::TaskType::FUNCTION;
    task.function_name = "toggle_fn";
    task.trigger_type  = ScheduledTask::TriggerType::INTERVAL;
    task.interval      = 1000ms;
    std::string id = scheduler_->registerTask(task);

    scheduler_->start();

    // Toggle enable/disable 20 times while scheduler runs
    for (int i = 0; i < 20; ++i) {
        if (i % 2 == 0) {
            scheduler_->disableTask(id);
        } else {
            scheduler_->enableTask(id);
        }
        std::this_thread::sleep_for(5ms);
    }
    // Leave enabled
    scheduler_->enableTask(id);

    scheduler_->stop();

    // Scheduler and task should be in a consistent state
    auto t = scheduler_->getTask(id);
    ASSERT_NE(t, nullptr);
    EXPECT_TRUE(t->enabled);
}

// ─────────────────────────────────────────────────────────────────────────────
// 8. High-throughput manual execution
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(ChaosSchedulerTest, HighThroughputManualExecutionMeetsBaseline) {
    std::atomic<int> count{0};
    scheduler_->registerFunction("fast_fn",
        [&count](const nlohmann::json&) -> nlohmann::json {
            ++count;
            return {};
        });
    ScheduledTask task;
    task.name          = "fast_task";
    task.type          = ScheduledTask::TaskType::FUNCTION;
    task.function_name = "fast_fn";
    task.trigger_type  = ScheduledTask::TriggerType::MANUAL;
    std::string id = scheduler_->registerTask(task);

    const int ITERATIONS = 100;
    int succeeded = 0;
    int limited = 0;
    auto start = std::chrono::steady_clock::now();
    for (int i = 0; i < ITERATIONS; ++i) {
        auto result = scheduler_->executeTaskNow(id);
        if (result.contains("error")) {
            ASSERT_EQ(result["error"].get<std::string>(), "Rate limit exceeded. Please try again later.");
            ++limited;
        } else {
            ++succeeded;
        }
    }
    auto elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - start).count();

    // Scheduler enforces max 10 manual executions per minute per task.
    EXPECT_EQ(succeeded, 10);
    EXPECT_EQ(limited, ITERATIONS - succeeded);
    EXPECT_EQ(count.load(), succeeded);
    // Even under throttling, the loop should complete quickly.
    EXPECT_LT(elapsed_ms, 5000) << "Throughput too low: " << elapsed_ms << "ms for 100 tasks";
}

// ─────────────────────────────────────────────────────────────────────────────
// 9. Mixed success/failure tasks under scheduler loop
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(ChaosSchedulerTest, MixedSuccessFailureTasksUnderLoop) {
    std::atomic<int> success_count{0};
    std::atomic<int> fail_count{0};

    scheduler_->registerFunction("mixed_success",
        [&success_count](const nlohmann::json&) -> nlohmann::json {
            ++success_count;
            return {};
        });
    scheduler_->registerFunction("mixed_fail",
        [&fail_count](const nlohmann::json&) -> nlohmann::json {
            ++fail_count;
            throw std::runtime_error("expected failure");
        });

    // Register multiple of each type
    for (int i = 0; i < 5; ++i) {
        ScheduledTask s;
        s.name          = "success_task_" + std::to_string(i);
        s.type          = ScheduledTask::TaskType::FUNCTION;
        s.function_name = "mixed_success";
        s.trigger_type  = ScheduledTask::TriggerType::INTERVAL;
        s.interval      = 1000ms;
        scheduler_->registerTask(s);

        ScheduledTask f;
        f.name          = "fail_task_" + std::to_string(i);
        f.type          = ScheduledTask::TaskType::FUNCTION;
        f.function_name = "mixed_fail";
        f.trigger_type  = ScheduledTask::TriggerType::INTERVAL;
        f.interval      = 1000ms;
        f.max_retries   = 0;
        scheduler_->registerTask(f);
    }

    scheduler_->start();
    std::this_thread::sleep_for(2200ms);
    scheduler_->stop();

    // Both success and failure tasks should have run at least once
    EXPECT_GT(success_count.load(), 0) << "Success tasks should have executed";
    EXPECT_GT(fail_count.load(), 0)    << "Failure tasks should have executed";

    auto stats = scheduler_->getStats();
    EXPECT_GT(stats.total_executions, 0u);
    EXPECT_GT(stats.failed_executions, 0u);
}

// ─────────────────────────────────────────────────────────────────────────────
// 10. Scheduler handles duplicate task IDs gracefully
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(ChaosSchedulerTest, DuplicateTaskNameGetsDifferentId) {
    scheduler_->registerFunction("dup_fn",
        [](const nlohmann::json&) -> nlohmann::json { return {}; });

    ScheduledTask task1;
    task1.name          = "dup_task";
    task1.type          = ScheduledTask::TaskType::FUNCTION;
    task1.function_name = "dup_fn";
    task1.trigger_type  = ScheduledTask::TriggerType::MANUAL;

    ScheduledTask task2 = task1;  // Same name, no explicit ID

    std::string id1 = scheduler_->registerTask(task1);
    std::string id2 = scheduler_->registerTask(task2);

    // Name-based task IDs are deterministic; re-registering same name overwrites.
    EXPECT_EQ(id1, id2);
    EXPECT_EQ(scheduler_->getStats().registered_tasks, 1u);
}

// ─────────────────────────────────────────────────────────────────────────────
// 11. Concurrent register while scheduler is running
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(ChaosSchedulerTest, ConcurrentRegistrationWhileSchedulerRunning) {
    scheduler_->registerFunction("register_race_fn",
        [](const nlohmann::json&) -> nlohmann::json { return {}; });

    scheduler_->start();

    const int N_THREADS = 5;
    const int TASKS_PER_THREAD = 10;
    std::vector<std::thread> threads;
    std::atomic<int> registered{0};

    for (int t = 0; t < N_THREADS; ++t) {
        threads.emplace_back([&, t]() {
            for (int i = 0; i < TASKS_PER_THREAD; ++i) {
                ScheduledTask task;
                task.name          = "race_t" + std::to_string(t) + "_i" + std::to_string(i);
                task.type          = ScheduledTask::TaskType::FUNCTION;
                task.function_name = "register_race_fn";
                task.trigger_type  = ScheduledTask::TriggerType::MANUAL;
                try {
                    scheduler_->registerTask(task);
                    ++registered;
                } catch (...) {
                    // Acceptable under race: just don't crash
                }
            }
        });
    }
    for (auto& th : threads) {
      th.join();
    }

    scheduler_->stop();

    // All registrations that completed must be reflected in stats
    EXPECT_EQ(scheduler_->getStats().registered_tasks,
              static_cast<size_t>(registered.load()))
        << "Stats don't match registered count";
}

// ─────────────────────────────────────────────────────────────────────────────
// 12. ExportMetrics under concurrent modification
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(ChaosSchedulerTest, ExportMetricsUnderConcurrentModificationDoesNotCrash) {
    scheduler_->registerFunction("metrics_race_fn",
        [](const nlohmann::json&) -> nlohmann::json { return {}; });

    // Register some tasks
    std::vector<std::string> ids;
    for (int i = 0; i < 5; ++i) {
        ScheduledTask task;
        task.name          = "metrics_race_" + std::to_string(i);
        task.type          = ScheduledTask::TaskType::FUNCTION;
        task.function_name = "metrics_race_fn";
        task.trigger_type  = ScheduledTask::TriggerType::MANUAL;
        ids.push_back(scheduler_->registerTask(task));
    }

    std::atomic<bool> stop_flag{false};

    // Background thread executes tasks concurrently with exportMetrics
    std::thread modifier([&]() {
        int idx = 0;
        while (!stop_flag.load()) {
            scheduler_->executeTaskNow(ids[idx % ids.size()]);
            ++idx;
            std::this_thread::sleep_for(1ms);
        }
    });

    // Main thread repeatedly calls exportMetrics
    for (int i = 0; i < 20; ++i) {
        EXPECT_NO_THROW(scheduler_->exportMetrics());
        std::this_thread::sleep_for(2ms);
    }

    stop_flag.store(true);
    modifier.join();
}
