/**
 * @file test_task_scheduler.cpp
 * @brief Unit tests for TaskScheduler
 */

#include <gtest/gtest.h>
#include "scheduler/task_scheduler.h"
#include "query/query_engine.h"
#include "storage/rocksdb_wrapper.h"
#include <filesystem>
#include <thread>
#include <chrono>

using namespace themis;

class TaskSchedulerTest : public ::testing::Test {
protected:
    void SetUp() override {
        test_db_path_ = "./data/test_task_scheduler";
        
        // Clean up any existing test data
        if (std::filesystem::exists(test_db_path_)) {
            std::filesystem::remove_all(test_db_path_);
        }
        std::filesystem::create_directories(test_db_path_);
        
        // Initialize RocksDB and QueryEngine
        rocksdb::Options options;
        options.create_if_missing = true;
        rocksdb::TransactionDBOptions txn_options;
        
        rocksdb::TransactionDB* db;
        rocksdb::Status status = rocksdb::TransactionDB::Open(
            options, txn_options, test_db_path_, &db
        );
        ASSERT_TRUE(status.ok()) << "Failed to open test database: " << status.ToString();
        
        db_wrapper_ = std::make_unique<RocksDBWrapper>(db);
        query_engine_ = std::make_unique<QueryEngine>(db_wrapper_.get());
    }
    
    void TearDown() override {
        query_engine_.reset();
        db_wrapper_.reset();
        
        if (std::filesystem::exists(test_db_path_)) {
            std::filesystem::remove_all(test_db_path_);
        }
    }
    
    std::string test_db_path_;
    std::unique_ptr<RocksDBWrapper> db_wrapper_;
    std::unique_ptr<QueryEngine> query_engine_;
};

TEST_F(TaskSchedulerTest, BasicLifecycle) {
    TaskScheduler::Config config;
    config.check_interval = std::chrono::milliseconds(100);
    
    TaskScheduler scheduler(query_engine_.get(), config);
    
    EXPECT_FALSE(scheduler.isRunning());
    
    scheduler.start();
    EXPECT_TRUE(scheduler.isRunning());
    
    scheduler.stop();
    EXPECT_FALSE(scheduler.isRunning());
}

TEST_F(TaskSchedulerTest, RegisterAndListTasks) {
    TaskScheduler scheduler(query_engine_.get());
    
    ScheduledTask task1;
    task1.name = "Test Task 1";
    task1.description = "A test task";
    task1.type = ScheduledTask::TaskType::AQL_QUERY;
    task1.aql_query = "FOR d IN test RETURN d";
    task1.interval = std::chrono::minutes(5);
    
    std::string id1 = scheduler.registerTask(task1);
    EXPECT_FALSE(id1.empty());
    
    ScheduledTask task2;
    task2.name = "Test Task 2";
    task2.type = ScheduledTask::TaskType::FUNCTION;
    task2.function_name = "test_function";
    task2.interval = std::chrono::minutes(10);
    
    std::string id2 = scheduler.registerTask(task2);
    EXPECT_FALSE(id2.empty());
    EXPECT_NE(id1, id2);
    
    auto tasks = scheduler.listTasks();
    EXPECT_EQ(tasks.size(), 2);
    
    auto stats = scheduler.getStats();
    EXPECT_EQ(stats.registered_tasks, 2);
    EXPECT_EQ(stats.active_tasks, 2);
}

TEST_F(TaskSchedulerTest, EnableDisableTask) {
    TaskScheduler scheduler(query_engine_.get());
    
    ScheduledTask task;
    task.name = "Test Task";
    task.type = ScheduledTask::TaskType::AQL_QUERY;
    task.aql_query = "FOR d IN test RETURN d";
    
    std::string id = scheduler.registerTask(task);
    
    auto stats = scheduler.getStats();
    EXPECT_EQ(stats.active_tasks, 1);
    
    scheduler.disableTask(id);
    stats = scheduler.getStats();
    EXPECT_EQ(stats.active_tasks, 0);
    
    scheduler.enableTask(id);
    stats = scheduler.getStats();
    EXPECT_EQ(stats.active_tasks, 1);
}

TEST_F(TaskSchedulerTest, UnregisterTask) {
    TaskScheduler scheduler(query_engine_.get());
    
    ScheduledTask task;
    task.name = "Test Task";
    task.type = ScheduledTask::TaskType::AQL_QUERY;
    task.aql_query = "FOR d IN test RETURN d";
    
    std::string id = scheduler.registerTask(task);
    EXPECT_EQ(scheduler.listTasks().size(), 1);
    
    scheduler.unregisterTask(id);
    EXPECT_EQ(scheduler.listTasks().size(), 0);
}

TEST_F(TaskSchedulerTest, FunctionRegistration) {
    TaskScheduler scheduler(query_engine_.get());
    
    bool function_called = false;
    scheduler.registerFunction("test_func", [&function_called](const nlohmann::json& params) {
        function_called = true;
        return nlohmann::json{{"result", "success"}};
    });
    
    ScheduledTask task;
    task.name = "Function Task";
    task.type = ScheduledTask::TaskType::FUNCTION;
    task.function_name = "test_func";
    
    std::string id = scheduler.registerTask(task);
    
    auto result = scheduler.executeTaskNow(id);
    EXPECT_TRUE(function_called);
    EXPECT_EQ(result["result"], "success");
}

TEST_F(TaskSchedulerTest, ExecuteTaskNow) {
    TaskScheduler scheduler(query_engine_.get());
    
    // Register a simple function
    int execution_count = 0;
    scheduler.registerFunction("counter", [&execution_count](const nlohmann::json& params) {
        execution_count++;
        return nlohmann::json{{"count", execution_count}};
    });
    
    ScheduledTask task;
    task.name = "Counter Task";
    task.type = ScheduledTask::TaskType::FUNCTION;
    task.function_name = "counter";
    
    std::string id = scheduler.registerTask(task);
    
    EXPECT_EQ(execution_count, 0);
    
    auto result1 = scheduler.executeTaskNow(id);
    EXPECT_EQ(execution_count, 1);
    EXPECT_EQ(result1["count"], 1);
    
    auto result2 = scheduler.executeTaskNow(id);
    EXPECT_EQ(execution_count, 2);
    EXPECT_EQ(result2["count"], 2);
}

TEST_F(TaskSchedulerTest, ScheduledExecution) {
    TaskScheduler::Config config;
    config.check_interval = std::chrono::milliseconds(50);
    
    TaskScheduler scheduler(query_engine_.get(), config);
    
    int execution_count = 0;
    scheduler.registerFunction("counter", [&execution_count](const nlohmann::json& params) {
        execution_count++;
        return nlohmann::json{{"count", execution_count}};
    });
    
    ScheduledTask task;
    task.name = "Scheduled Counter";
    task.type = ScheduledTask::TaskType::FUNCTION;
    task.function_name = "counter";
    task.interval = std::chrono::milliseconds(100);
    
    std::string id = scheduler.registerTask(task);
    
    scheduler.start();
    
    // Wait for at least 2 executions
    std::this_thread::sleep_for(std::chrono::milliseconds(300));
    
    scheduler.stop();
    
    EXPECT_GE(execution_count, 2);
    
    auto task_info = scheduler.getTask(id);
    ASSERT_NE(task_info, nullptr);
    EXPECT_GE(task_info->total_executions, 2);
}

TEST_F(TaskSchedulerTest, UpdateTask) {
    TaskScheduler scheduler(query_engine_.get());
    
    ScheduledTask task;
    task.id = "test_task";
    task.name = "Original Name";
    task.type = ScheduledTask::TaskType::FUNCTION;
    task.function_name = "test_func";
    task.interval = std::chrono::minutes(5);
    
    scheduler.registerTask(task);
    
    auto task_info = scheduler.getTask("test_task");
    ASSERT_NE(task_info, nullptr);
    EXPECT_EQ(task_info->name, "Original Name");
    EXPECT_EQ(task_info->interval, std::chrono::minutes(5));
    
    task.name = "Updated Name";
    task.interval = std::chrono::minutes(10);
    scheduler.updateTask(task);
    
    task_info = scheduler.getTask("test_task");
    ASSERT_NE(task_info, nullptr);
    EXPECT_EQ(task_info->name, "Updated Name");
    EXPECT_EQ(task_info->interval, std::chrono::minutes(10));
}

TEST_F(TaskSchedulerTest, MaxConcurrentTasks) {
    TaskScheduler::Config config;
    config.max_concurrent_tasks = 2;
    config.check_interval = std::chrono::milliseconds(50);
    
    TaskScheduler scheduler(query_engine_.get(), config);
    
    std::atomic<int> running_count{0};
    std::atomic<int> max_concurrent{0};
    
    scheduler.registerFunction("slow_task", [&running_count, &max_concurrent](const nlohmann::json& params) {
        running_count++;
        int current = running_count.load();
        int max_val = max_concurrent.load();
        while (current > max_val && !max_concurrent.compare_exchange_weak(max_val, current)) {
            max_val = max_concurrent.load();
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        running_count--;
        return nlohmann::json{{"result", "done"}};
    });
    
    // Register 4 tasks with short intervals
    for (int i = 0; i < 4; i++) {
        ScheduledTask task;
        task.name = "Task " + std::to_string(i);
        task.type = ScheduledTask::TaskType::FUNCTION;
        task.function_name = "slow_task";
        task.interval = std::chrono::milliseconds(50);
        scheduler.registerTask(task);
    }
    
    scheduler.start();
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    scheduler.stop();
    
    // Should never run more than 2 tasks concurrently
    EXPECT_LE(max_concurrent.load(), 2);
}

TEST_F(TaskSchedulerTest, TaskStatistics) {
    TaskScheduler scheduler(query_engine_.get());
    
    int execution_count = 0;
    scheduler.registerFunction("counter", [&execution_count](const nlohmann::json& params) {
        execution_count++;
        if (execution_count == 2) {
            throw std::runtime_error("Simulated failure");
        }
        return nlohmann::json{{"count", execution_count}};
    });
    
    ScheduledTask task;
    task.name = "Stats Task";
    task.type = ScheduledTask::TaskType::FUNCTION;
    task.function_name = "counter";
    
    std::string id = scheduler.registerTask(task);
    
    // Execute 3 times, second one will fail
    scheduler.executeTaskNow(id);
    scheduler.executeTaskNow(id);
    scheduler.executeTaskNow(id);
    
    auto task_info = scheduler.getTask(id);
    ASSERT_NE(task_info, nullptr);
    EXPECT_EQ(task_info->total_executions, 3);
    EXPECT_EQ(task_info->successful_executions, 2);
    EXPECT_EQ(task_info->failed_executions, 1);
    EXPECT_FALSE(task_info->last_error.empty());
}

// ===== Security & Validation Tests =====

TEST_F(TaskSchedulerTest, ValidateAqlQueryInjectionProtection) {
    TaskScheduler scheduler(query_engine_.get());
    
    // Test SQL injection patterns (these should be rejected)
    std::vector<std::string> malicious_queries = {
        "FOR d IN test RETURN d'; DROP TABLE users; --",
        "FOR d IN test RETURN d; DROP COLLECTION test",
        "FOR d IN test FILTER d.value = '\\x00' RETURN d",
        "FOR d IN test RETURN SYSTEM('rm -rf /')",
        "FOR d IN test RETURN d; EXEC('malicious_command')"
    };
    
    for (const auto& query : malicious_queries) {
        ScheduledTask task;
        task.name = "Malicious Task";
        task.type = ScheduledTask::TaskType::AQL_QUERY;
        task.aql_query = query;
        
        EXPECT_THROW({
            scheduler.registerTask(task);
        }, std::invalid_argument) << "Failed to detect malicious pattern in: " << query;
    }
}

TEST_F(TaskSchedulerTest, ValidateAqlQueryLength) {
    TaskScheduler scheduler(query_engine_.get());
    
    // Test extremely long query (potential DoS)
    std::string long_query(200000, 'A'); // 200KB query
    
    ScheduledTask task;
    task.name = "Long Query Task";
    task.type = ScheduledTask::TaskType::AQL_QUERY;
    task.aql_query = long_query;
    
    EXPECT_THROW({
        scheduler.registerTask(task);
    }, std::invalid_argument);
}

TEST_F(TaskSchedulerTest, ValidateResourceLimits) {
    TaskScheduler scheduler(query_engine_.get());
    
    // Test excessive timeout
    {
        ScheduledTask task;
        task.name = "Excessive Timeout Task";
        task.type = ScheduledTask::TaskType::FUNCTION;
        task.function_name = "test_func";
        task.timeout = std::chrono::hours(48); // 48 hours (too long)
        
        EXPECT_THROW({
            scheduler.registerTask(task);
        }, std::invalid_argument);
    }
    
    // Test excessive retries
    {
        ScheduledTask task;
        task.name = "Excessive Retries Task";
        task.type = ScheduledTask::TaskType::FUNCTION;
        task.function_name = "test_func";
        task.max_retries = 100; // Too many retries
        
        EXPECT_THROW({
            scheduler.registerTask(task);
        }, std::invalid_argument);
    }
    
    // Test interval too short
    {
        ScheduledTask task;
        task.name = "Fast Task";
        task.type = ScheduledTask::TaskType::FUNCTION;
        task.function_name = "test_func";
        task.interval = std::chrono::milliseconds(100); // Too fast
        
        EXPECT_THROW({
            scheduler.registerTask(task);
        }, std::invalid_argument);
    }
}

TEST_F(TaskSchedulerTest, SanitizeTaskInputs) {
    TaskScheduler scheduler(query_engine_.get());
    
    // Register function to test with
    scheduler.registerFunction("test_func", [](const nlohmann::json& params) {
        return nlohmann::json{{"result", "ok"}};
    });
    
    // Test with control characters and null bytes
    ScheduledTask task;
    task.name = "Test\x00Task\x01With\x02Control"; // Includes null bytes and control chars
    task.description = "Description\nWith\x00Null\x03Bytes";
    task.type = ScheduledTask::TaskType::FUNCTION;
    task.function_name = "test_func";
    
    std::string task_id = scheduler.registerTask(task);
    
    auto registered_task = scheduler.getTask(task_id);
    ASSERT_NE(registered_task, nullptr);
    
    // Verify control characters were removed (except newline/tab)
    EXPECT_EQ(registered_task->name.find('\x00'), std::string::npos);
    EXPECT_EQ(registered_task->name.find('\x01'), std::string::npos);
    EXPECT_EQ(registered_task->description.find('\x00'), std::string::npos);
    
    // Newline should be preserved
    EXPECT_NE(registered_task->description.find('\n'), std::string::npos);
}

TEST_F(TaskSchedulerTest, QueryComplexityLimits) {
    TaskScheduler scheduler(query_engine_.get());
    
    // Test excessive nesting
    {
        std::string deeply_nested = "FOR d IN test FILTER ((((((((((((((((((((d.value > 0)))))))))))))))))))) RETURN d";
        
        ScheduledTask task;
        task.name = "Deeply Nested Task";
        task.type = ScheduledTask::TaskType::AQL_QUERY;
        task.aql_query = deeply_nested;
        
        EXPECT_THROW({
            scheduler.registerTask(task);
        }, std::invalid_argument);
    }
    
    // Test too many FOR loops
    {
        std::string query = "FOR a IN c1 FOR b IN c2 FOR c IN c3 FOR d IN c4 FOR e IN c5 FOR f IN c6 RETURN a";
        
        ScheduledTask task;
        task.name = "Many Loops Task";
        task.type = ScheduledTask::TaskType::AQL_QUERY;
        task.aql_query = query;
        
        EXPECT_THROW({
            scheduler.registerTask(task);
        }, std::invalid_argument);
    }
}

TEST_F(TaskSchedulerTest, RateLimiting) {
    TaskScheduler scheduler(query_engine_.get());
    
    int execution_count = 0;
    scheduler.registerFunction("rate_test", [&execution_count](const nlohmann::json& params) {
        execution_count++;
        return nlohmann::json{{"count", execution_count}};
    });
    
    ScheduledTask task;
    task.name = "Rate Test Task";
    task.type = ScheduledTask::TaskType::FUNCTION;
    task.function_name = "rate_test";
    
    std::string task_id = scheduler.registerTask(task);
    
    // Execute task multiple times rapidly
    int successful_executions = 0;
    int rate_limited = 0;
    
    for (int i = 0; i < 15; i++) {
        auto result = scheduler.executeTaskNow(task_id);
        
        if (result.contains("error") && 
            result["error"].get<std::string>().find("Rate limit") != std::string::npos) {
            rate_limited++;
        } else {
            successful_executions++;
        }
    }
    
    // Should have hit rate limit at least once (max 10 per minute)
    EXPECT_GT(rate_limited, 0) << "Rate limiting did not trigger";
    EXPECT_LE(successful_executions, 10) << "Too many executions allowed";
}

TEST_F(TaskSchedulerTest, SecureTaskPersistence) {
    TaskScheduler::Config config;
    config.persist_tasks = true;
    config.persistence_path = "./data/test_task_scheduler_secure";
    
    // Clean up any existing test data
    std::string test_path = "./data/test_task_scheduler_secure";
    if (std::filesystem::exists(test_path)) {
        std::filesystem::remove_all(test_path);
    }
    std::filesystem::create_directories(test_path);
    
    {
        TaskScheduler scheduler(query_engine_.get(), config);
        
        scheduler.registerFunction("test_func", [](const nlohmann::json& params) {
            return nlohmann::json{{"result", "ok"}};
        });
        
        ScheduledTask task;
        task.name = "Persistent Task";
        task.type = ScheduledTask::TaskType::FUNCTION;
        task.function_name = "test_func";
        
        scheduler.registerTask(task);
        
        // Scheduler destructor should save tasks
    }
    
    // Verify file was created
    std::string tasks_file = test_path + "/tasks.json";
    EXPECT_TRUE(std::filesystem::exists(tasks_file));
    
    // Check file permissions on Unix systems
    #ifndef _WIN32
    struct stat st;
    if (stat(tasks_file.c_str(), &st) == 0) {
        // Check that only owner has read/write, no group/other permissions
        mode_t expected = S_IRUSR | S_IWUSR;
        mode_t actual = st.st_mode & 0777;
        EXPECT_EQ(actual, expected) << "File permissions not set correctly";
    }
    #endif
    
    // Clean up
    std::filesystem::remove_all(test_path);
}

TEST_F(TaskSchedulerTest, ValidAqlQueryAccepted) {
    TaskScheduler scheduler(query_engine_.get());
    
    // Test that valid queries are accepted (including REMOVE which is valid AQL)
    std::vector<std::string> valid_queries = {
        "FOR d IN test RETURN d",
        "FOR d IN test FILTER d.value > 10 RETURN d",
        "FOR d IN test COLLECT metric = d.metric RETURN metric",
        "INSERT { name: 'test', value: 42 } INTO test",
        "FOR d IN timeseries FILTER d.timestamp < DATE_SUB(NOW(), 30, 'days') REMOVE d IN timeseries"
    };
    
    for (const auto& query : valid_queries) {
        ScheduledTask task;
        task.name = "Valid Task";
        task.type = ScheduledTask::TaskType::AQL_QUERY;
        task.aql_query = query;
        
        EXPECT_NO_THROW({
            std::string id = scheduler.registerTask(task);
            EXPECT_FALSE(id.empty());
            scheduler.unregisterTask(id);
        }) << "Valid query rejected: " << query;
    }
}
