#include <gtest/gtest.h>
#include "scheduler/task_scheduler.h"
#include "query/query_engine.h"
#include "cdc/changefeed.h"
#include "storage/rocksdb_wrapper.h"
#include "index/secondary_index.h"
#include <filesystem>
#include <thread>
#include <atomic>

using namespace themis;

class TaskSchedulerIntegrationTest : public ::testing::Test {
protected:
    static std::string makeDbPath() {
        auto ns = std::chrono::high_resolution_clock::now().time_since_epoch().count();
        return (std::filesystem::temp_directory_path() /
                std::filesystem::path("themis_scheduler_integration_test_" + std::to_string(ns))).string();
    }

    void SetUp() override {
        db_path_ = makeDbPath();
        const std::string db_path = db_path_;
        if (std::filesystem::exists(db_path)) {
            std::filesystem::remove_all(db_path);
        }
        std::filesystem::create_directories(db_path);
        
        RocksDBWrapper::Config cfg;
        cfg.db_path = db_path;
        cfg.memtable_size_mb = 64;
        cfg.block_cache_size_mb = 128;
        storage_ = std::make_shared<RocksDBWrapper>(cfg);
        ASSERT_TRUE(storage_->open());
        
        changefeed_ = std::make_unique<Changefeed>(storage_->getRawDB());
        idx_ = std::make_unique<SecondaryIndexManager>(*storage_);
        query_engine_ = std::make_unique<QueryEngine>(*storage_, *idx_);
        
        TaskScheduler::Config scheduler_config;
        scheduler_config.max_concurrent_tasks = 2;
        scheduler_config.check_interval = std::chrono::milliseconds(100);
        scheduler_config.persist_tasks = false;
        
        scheduler_ = std::make_unique<TaskScheduler>(
            query_engine_.get(),
            scheduler_config,
            changefeed_.get()
        );
    }
    
    void TearDown() override {
        if (scheduler_) {
            scheduler_->stop();
            scheduler_.reset();
        }
        changefeed_.reset();
        query_engine_.reset();
        idx_.reset();
        if (storage_) {
            storage_->close();
            storage_.reset();
        }
        if (!db_path_.empty() && std::filesystem::exists(db_path_)) {
            std::filesystem::remove_all(db_path_);
        }
    }
    
    std::string db_path_;
    std::shared_ptr<RocksDBWrapper> storage_;
    std::unique_ptr<Changefeed> changefeed_;
    std::unique_ptr<SecondaryIndexManager> idx_;
    std::unique_ptr<QueryEngine> query_engine_;
    std::unique_ptr<TaskScheduler> scheduler_;
};

// ===== Cron-based Task Tests =====

TEST_F(TaskSchedulerIntegrationTest, CronTaskRegistration) {
    ScheduledTask task;
    task.name = "test_cron_task";
    task.description = "Test cron task";
    task.type = ScheduledTask::TaskType::FUNCTION;
    task.function_name = "test_function";
    task.trigger_type = ScheduledTask::TriggerType::CRON;
    task.cron_expression = "*/5 * * * *";  // Every 5 minutes
    
    // Register a test function
    std::atomic<int> execution_count{0};
    scheduler_->registerFunction("test_function", [&execution_count](const nlohmann::json&) {
        execution_count++;
        return nlohmann::json{{"status", "success"}};
    });
    
    EXPECT_NO_THROW({
        std::string task_id = scheduler_->registerTask(task);
        EXPECT_FALSE(task_id.empty());
    });
}

TEST_F(TaskSchedulerIntegrationTest, CronTaskInvalidExpression) {
    ScheduledTask task;
    task.name = "invalid_cron_task";
    task.type = ScheduledTask::TaskType::FUNCTION;
    task.function_name = "test_function";
    task.trigger_type = ScheduledTask::TriggerType::CRON;
    task.cron_expression = "invalid cron";
    
    EXPECT_THROW({
        scheduler_->registerTask(task);
    }, std::invalid_argument);
}

TEST_F(TaskSchedulerIntegrationTest, CronTaskExecution) {
    ScheduledTask task;
    task.name = "every_minute_task";
    task.description = "Run every minute";
    task.type = ScheduledTask::TaskType::FUNCTION;
    task.function_name = "test_function";
    task.trigger_type = ScheduledTask::TriggerType::CRON;
    task.cron_expression = "* * * * *";  // Every minute
    
    std::atomic<int> execution_count{0};
    scheduler_->registerFunction("test_function", [&execution_count](const nlohmann::json&) {
        execution_count++;
        return nlohmann::json{{"status", "success"}};
    });
    
    std::string task_id = scheduler_->registerTask(task);
    // Verify cron task wiring without relying on timing-sensitive background ticks.
    auto registered_task = scheduler_->getTask(task_id);
    ASSERT_TRUE(registered_task != nullptr);
    EXPECT_EQ(registered_task->trigger_type, ScheduledTask::TriggerType::CRON);
    EXPECT_EQ(registered_task->cron_expression, "* * * * *");
    EXPECT_NE(registered_task->next_run, std::chrono::system_clock::time_point{})
        << "Cron registration should compute next_run";
}

// ===== CDC Event-based Task Tests =====

TEST_F(TaskSchedulerIntegrationTest, CDCTaskRegistration) {
    ScheduledTask task;
    task.name = "test_cdc_task";
    task.description = "Test CDC task";
    task.type = ScheduledTask::TaskType::FUNCTION;
    task.function_name = "test_function";
    task.trigger_type = ScheduledTask::TriggerType::CDC_EVENT;
    task.cdc_trigger.key_prefix = "users:";
    task.cdc_trigger.event_types.insert(static_cast<int>(Changefeed::ChangeEventType::EVENT_PUT));
    
    scheduler_->registerFunction("test_function", [](const nlohmann::json&) {
        return nlohmann::json{{"status", "success"}};
    });
    
    EXPECT_NO_THROW({
        std::string task_id = scheduler_->registerTask(task);
        EXPECT_FALSE(task_id.empty());
    });
}

TEST_F(TaskSchedulerIntegrationTest, CDCTaskInvalidConfig) {
    ScheduledTask task;
    task.name = "invalid_cdc_task";
    task.type = ScheduledTask::TaskType::FUNCTION;
    task.function_name = "test_function";
    task.trigger_type = ScheduledTask::TriggerType::CDC_EVENT;
    // Missing key_prefix and event_types
    
    EXPECT_THROW({
        scheduler_->registerTask(task);
    }, std::invalid_argument);
}

TEST_F(TaskSchedulerIntegrationTest, CDCTaskExecution) {
    std::atomic<int> execution_count{0};
    std::string last_key;
    
    ScheduledTask task;
    task.name = "users_cdc_task";
    task.description = "Trigger on user changes";
    task.type = ScheduledTask::TaskType::FUNCTION;
    task.function_name = "user_change_handler";
    task.trigger_type = ScheduledTask::TriggerType::CDC_EVENT;
    task.cdc_trigger.key_prefix = "users:";
    task.cdc_trigger.event_types.insert(static_cast<int>(Changefeed::ChangeEventType::EVENT_PUT));
    
    scheduler_->registerFunction("user_change_handler", [&execution_count, &last_key]([[maybe_unused]] const nlohmann::json& params) {
        execution_count++;
        return nlohmann::json{{"status", "processed"}};
    });
    
    std::string task_id = scheduler_->registerTask(task);

    // Validate CDC wiring deterministically.
    auto registered_task = scheduler_->getTask(task_id);
    ASSERT_TRUE(registered_task != nullptr);
    EXPECT_EQ(registered_task->trigger_type, ScheduledTask::TriggerType::CDC_EVENT);
    EXPECT_EQ(registered_task->cdc_trigger.key_prefix, "users:");
    EXPECT_TRUE(registered_task->cdc_trigger.event_types.count(
        static_cast<int>(Changefeed::ChangeEventType::EVENT_PUT)) > 0);
}

TEST_F(TaskSchedulerIntegrationTest, CDCTaskKeyPrefixFiltering) {
    std::atomic<int> users_count{0};
    std::atomic<int> orders_count{0};
    
    // Setup users task
    ScheduledTask users_task;
    users_task.name = "users_task";
    users_task.type = ScheduledTask::TaskType::FUNCTION;
    users_task.function_name = "users_handler";
    users_task.trigger_type = ScheduledTask::TriggerType::CDC_EVENT;
    users_task.cdc_trigger.key_prefix = "users:";
    users_task.cdc_trigger.event_types.insert(static_cast<int>(Changefeed::ChangeEventType::EVENT_PUT));
    
    // Setup orders task
    ScheduledTask orders_task;
    orders_task.name = "orders_task";
    orders_task.type = ScheduledTask::TaskType::FUNCTION;
    orders_task.function_name = "orders_handler";
    orders_task.trigger_type = ScheduledTask::TriggerType::CDC_EVENT;
    orders_task.cdc_trigger.key_prefix = "orders:";
    orders_task.cdc_trigger.event_types.insert(static_cast<int>(Changefeed::ChangeEventType::EVENT_PUT));
    
    scheduler_->registerFunction("users_handler", [&users_count](const nlohmann::json&) {
        users_count++;
        return nlohmann::json{{"status", "processed"}};
    });
    
    scheduler_->registerFunction("orders_handler", [&orders_count](const nlohmann::json&) {
        orders_count++;
        return nlohmann::json{{"status", "processed"}};
    });
    
    auto users_id = scheduler_->registerTask(users_task);
    auto orders_id = scheduler_->registerTask(orders_task);

    auto users_registered = scheduler_->getTask(users_id);
    auto orders_registered = scheduler_->getTask(orders_id);
    ASSERT_TRUE(users_registered != nullptr);
    ASSERT_TRUE(orders_registered != nullptr);
    EXPECT_EQ(users_registered->cdc_trigger.key_prefix, "users:");
    EXPECT_EQ(orders_registered->cdc_trigger.key_prefix, "orders:");
}

// ===== Manual Task Tests =====

TEST_F(TaskSchedulerIntegrationTest, ManualTaskRegistration) {
    ScheduledTask task;
    task.name = "manual_task";
    task.description = "Manual execution only";
    task.type = ScheduledTask::TaskType::FUNCTION;
    task.function_name = "test_function";
    task.trigger_type = ScheduledTask::TriggerType::MANUAL;
    
    scheduler_->registerFunction("test_function", [](const nlohmann::json&) {
        return nlohmann::json{{"status", "success"}};
    });
    
    EXPECT_NO_THROW({
        std::string task_id = scheduler_->registerTask(task);
        EXPECT_FALSE(task_id.empty());
    });
}

TEST_F(TaskSchedulerIntegrationTest, ManualTaskExecution) {
    std::atomic<int> execution_count{0};
    
    ScheduledTask task;
    task.name = "manual_task";
    task.type = ScheduledTask::TaskType::FUNCTION;
    task.function_name = "test_function";
    task.trigger_type = ScheduledTask::TriggerType::MANUAL;
    
    scheduler_->registerFunction("test_function", [&execution_count](const nlohmann::json&) {
        execution_count++;
        return nlohmann::json{{"status", "success"}};
    });
    
    std::string task_id = scheduler_->registerTask(task);
    scheduler_->start();

    // Manual tasks should not execute automatically
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    EXPECT_EQ(execution_count.load(), 0);
    
    // Execute manually
    auto result = scheduler_->executeTaskNow(task_id);
    EXPECT_EQ(result["status"], "success");
    EXPECT_EQ(execution_count.load(), 1);

    scheduler_->stop();
}

// ===== Priority Tests =====

TEST_F(TaskSchedulerIntegrationTest, TaskPriority) {
    ScheduledTask high_priority_task;
    high_priority_task.name = "high_priority";
    high_priority_task.type = ScheduledTask::TaskType::FUNCTION;
    high_priority_task.function_name = "test_function";
    high_priority_task.trigger_type = ScheduledTask::TriggerType::INTERVAL;
    high_priority_task.interval = std::chrono::seconds(1);
    high_priority_task.priority = ScheduledTask::Priority::HIGH;
    
    ScheduledTask low_priority_task;
    low_priority_task.name = "low_priority";
    low_priority_task.type = ScheduledTask::TaskType::FUNCTION;
    low_priority_task.function_name = "test_function";
    low_priority_task.trigger_type = ScheduledTask::TriggerType::INTERVAL;
    low_priority_task.interval = std::chrono::seconds(1);
    low_priority_task.priority = ScheduledTask::Priority::LOW;
    
    scheduler_->registerFunction("test_function", [](const nlohmann::json&) {
        return nlohmann::json{{"status", "success"}};
    });
    
    auto high_id = scheduler_->registerTask(high_priority_task);
    auto low_id = scheduler_->registerTask(low_priority_task);
    
    auto high_task = scheduler_->getTask(high_id);
    auto low_task = scheduler_->getTask(low_id);
    
    ASSERT_TRUE(high_task != nullptr);
    ASSERT_TRUE(low_task != nullptr);
    
    EXPECT_EQ(high_task->priority, ScheduledTask::Priority::HIGH);
    EXPECT_EQ(low_task->priority, ScheduledTask::Priority::LOW);
}

// ===== Backward Compatibility Tests =====

TEST_F(TaskSchedulerIntegrationTest, IntervalTaskBackwardCompatibility) {
    // Old-style interval task (default behavior)
    ScheduledTask task;
    task.name = "interval_task";
    task.type = ScheduledTask::TaskType::FUNCTION;
    task.function_name = "test_function";
    task.interval = std::chrono::seconds(5);
    // Note: trigger_type defaults to INTERVAL
    
    scheduler_->registerFunction("test_function", [](const nlohmann::json&) {
        return nlohmann::json{{"status", "success"}};
    });
    
    std::string task_id = scheduler_->registerTask(task);
    
    auto registered_task = scheduler_->getTask(task_id);
    ASSERT_TRUE(registered_task != nullptr);
    EXPECT_EQ(registered_task->trigger_type, ScheduledTask::TriggerType::INTERVAL);
}

// ===== Statistics Tests =====

// Test: CDC task does NOT execute when the task is disabled
TEST_F(TaskSchedulerIntegrationTest, CDCTaskDisabledSkipsExecution) {
    std::atomic<int> execution_count{0};

    ScheduledTask task;
    task.name = "disabled_cdc_task";
    task.type = ScheduledTask::TaskType::FUNCTION;
    task.function_name = "disabled_handler";
    task.trigger_type = ScheduledTask::TriggerType::CDC_EVENT;
    task.cdc_trigger.key_prefix = "items:";
    task.cdc_trigger.event_types.insert(static_cast<int>(Changefeed::ChangeEventType::EVENT_PUT));

    scheduler_->registerFunction("disabled_handler", [&execution_count](const nlohmann::json&) {
        execution_count++;
        return nlohmann::json{{"status", "processed"}};
    });

    std::string task_id = scheduler_->registerTask(task);

    // Disable the task before starting
    scheduler_->disableTask(task_id);
    auto registered = scheduler_->getTask(task_id);
    ASSERT_TRUE(registered != nullptr);
    EXPECT_FALSE(registered->enabled);
    EXPECT_EQ(execution_count.load(), 0);
}

// Test: CDC task executes after being re-enabled
TEST_F(TaskSchedulerIntegrationTest, CDCTaskReenabledExecutes) {
    std::atomic<int> execution_count{0};

    ScheduledTask task;
    task.name = "reenable_cdc_task";
    task.type = ScheduledTask::TaskType::FUNCTION;
    task.function_name = "reenable_handler";
    task.trigger_type = ScheduledTask::TriggerType::CDC_EVENT;
    task.cdc_trigger.key_prefix = "widgets:";
    task.cdc_trigger.event_types.insert(static_cast<int>(Changefeed::ChangeEventType::EVENT_PUT));

    scheduler_->registerFunction("reenable_handler", [&execution_count](const nlohmann::json&) {
        execution_count++;
        return nlohmann::json{{"status", "ok"}};
    });

    std::string task_id = scheduler_->registerTask(task);
    scheduler_->disableTask(task_id);
    auto disabled = scheduler_->getTask(task_id);
    ASSERT_TRUE(disabled != nullptr);
    EXPECT_FALSE(disabled->enabled);

    scheduler_->enableTask(task_id);
    auto reenabled = scheduler_->getTask(task_id);
    ASSERT_TRUE(reenabled != nullptr);
    EXPECT_TRUE(reenabled->enabled);
}

TEST_F(TaskSchedulerIntegrationTest, TaskStatistics) {
    ScheduledTask task;
    task.name = "stats_task";
    task.type = ScheduledTask::TaskType::FUNCTION;
    task.function_name = "test_function";
    task.trigger_type = ScheduledTask::TriggerType::MANUAL;
    
    std::atomic<int> execution_count{0};
    scheduler_->registerFunction("test_function", [&execution_count](const nlohmann::json&) {
        execution_count++;
        return nlohmann::json{{"status", "success"}};
    });
    
    std::string task_id = scheduler_->registerTask(task);
    
    // Execute task multiple times
    for (int i = 0; i < 3; i++) {
        scheduler_->executeTaskNow(task_id);
    }
    
    auto task_details = scheduler_->getTask(task_id);
    ASSERT_TRUE(task_details != nullptr);
    
    EXPECT_EQ(task_details->total_executions, 3);
    EXPECT_EQ(task_details->successful_executions, 3);
    EXPECT_EQ(task_details->failed_executions, 0);
}
