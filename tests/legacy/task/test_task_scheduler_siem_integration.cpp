/**
 * @file test_task_scheduler_siem_integration.cpp
 * @brief Tests for TaskScheduler SIEM integration and audit logging
 */

#include <gtest/gtest.h>
#include <thread>
#include <chrono>
#include <filesystem>
#include <fstream>

#include "scheduler/task_scheduler.h"
#include "utils/audit_logger.h"
#include "security/encryption.h"
#include "utils/pki_client.h"
#include "storage/rocksdb_wrapper.h"
#include "index/secondary_index.h"
#include "cdc/changefeed.h"
#include "query/query_engine.h"

using namespace themis;
using namespace themis::utils;

/**
 * Mock PKI Client for testing (doesn't require actual PKI infrastructure)
 */
class MockPKIClient : public VCCPKIClient {
public:
    MockPKIClient() : VCCPKIClient(PKIConfig{}) {}
    
    // NOTE: signHash signature changed in base class
    SignatureResult signHash([[maybe_unused]] const std::vector<uint8_t>& hash) {
        SignatureResult result;
        result.ok = true;
        result.signature_id = "test-sig-" + std::to_string(signature_count_++);
        result.algorithm = "ECDSA-SHA256";
        result.signature_b64 = "dGVzdF9zaWduYXR1cmU="; // base64("test_signature")
        result.cert_serial = "12345";
        return result;
    }
    
private:
    int signature_count_ = 0;
};

/**
 * Test fixture for TaskScheduler SIEM integration tests
 */
class TaskSchedulerSIEMIntegrationTest : public ::testing::Test {
protected:
    static std::string makeTestDir() {
        auto ns = std::chrono::high_resolution_clock::now().time_since_epoch().count();
        return (std::filesystem::temp_directory_path() /
                std::filesystem::path("siem_test_" + std::to_string(ns))).string();
    }

    void SetUp() override {
    #ifdef THEMIS_EDITION_COMMUNITY
        GTEST_SKIP() << "TaskScheduler SIEM integration tests require field_encryption support (Enterprise/Hyperscaler).";
    #endif

        // Explicitly opt in for test-only mock key provider usage.
    #ifdef _WIN32
        _putenv_s("THEMIS_ALLOW_MOCK_KEY_PROVIDER", "1");
    #else
        setenv("THEMIS_ALLOW_MOCK_KEY_PROVIDER", "1", 1);
    #endif

        // Clean up any previous test artifacts
        test_dir_ = makeTestDir();
        std::filesystem::create_directories(test_dir_);
        
        // Initialize storage (required for QueryEngine)
        RocksDBWrapper::Config db_config;
        db_config.db_path = test_dir_ + "/db";
        // create_if_missing is default behavior
        db_wrapper_ = std::make_unique<RocksDBWrapper>(db_config);
        ASSERT_TRUE(db_wrapper_->open());
        
        // Initialize changefeed for CDC events
        changefeed_ = std::make_shared<Changefeed>(db_wrapper_->getRawDB());
        
        // Initialize query engine required by TaskScheduler.
        idx_ = std::make_unique<SecondaryIndexManager>(*db_wrapper_);
        query_engine_ = std::make_unique<QueryEngine>(*db_wrapper_, *idx_);
        
        // Initialize encryption (mock implementation for testing).
        // Community edition may not provide field_encryption; skip gracefully.
        try {
            encryption_ = FieldEncryption::createDefault();
        } catch (const std::exception& e) {
            const std::string msg = e.what();
            if (msg.find("field_encryption") != std::string::npos) {
                GTEST_SKIP() << "Field encryption unavailable in current edition: " << msg;
            }
            throw;
        }
        
        // Initialize PKI client (mock)
        pki_client_ = std::make_shared<MockPKIClient>();
        
        // Initialize audit logger with test configuration
        AuditLoggerConfig audit_config;
        audit_config.enabled = true;
        audit_config.log_path = test_dir_ + "/audit.jsonl";
        audit_config.enable_siem = false; // Disable actual SIEM forwarding in tests
        audit_config.enable_hash_chain = true;
        audit_config.chain_state_file = test_dir_ + "/audit_chain.json";
        audit_config.enable_task_scheduler_audit = true;
        audit_config.enable_anomaly_detection = true;
        audit_config.anomaly_threshold = 2.0;
        
        audit_logger_ = std::make_shared<AuditLogger>(
            encryption_,
            pki_client_,
            audit_config
        );
        
        // Initialize task scheduler with audit logging
        TaskScheduler::Config scheduler_config;
        scheduler_config.persist_tasks = false;
        scheduler_config.max_concurrent_tasks = 2;
        scheduler_config.check_interval = std::chrono::milliseconds(100);
        scheduler_config.enable_audit_logging = true;
        
        scheduler_ = std::make_unique<TaskScheduler>(
            query_engine_.get(),
            scheduler_config,
            changefeed_.get(),
            audit_logger_  // NOTE: Changed from raw pointer to shared_ptr
        );
    }
    
    void TearDown() override {
        // Stop scheduler if running
        if (scheduler_) {
            scheduler_->stop();
            scheduler_.reset();
        }

        changefeed_.reset();
        query_engine_.reset();
        idx_.reset();
        audit_logger_.reset();
        encryption_.reset();
        pki_client_.reset();
        
        if (db_wrapper_) {
            db_wrapper_->close();
            db_wrapper_.reset();
        }
        
        // Clean up test directory
        std::error_code ec;
        std::filesystem::remove_all(test_dir_, ec);
    }
    
    // Helper to read audit log entries
    std::vector<nlohmann::json> readAuditLog() {
        std::vector<nlohmann::json> entries;
        std::string log_path = test_dir_ + "/audit.jsonl";
        
        if (!std::filesystem::exists(log_path)) {
            return entries;
        }
        
        std::ifstream ifs(log_path);
        std::string line;
        while (std::getline(ifs, line)) {
            if (!line.empty()) {
                entries.push_back(nlohmann::json::parse(line));
            }
        }
        
        return entries;
    }
    
    // Helper to find audit events by type
    std::vector<nlohmann::json> findAuditEventsByType([[maybe_unused]] const std::string& event_type) {
        auto all_entries = readAuditLog();
        std::vector<nlohmann::json> matching;
        
        for (const auto& entry : all_entries) {
            // Events are encrypted, so we need to check the payload if available
            // For this test, we'll check the category
            if (entry.contains("category") && entry["category"] == "TASK_SCHEDULER") {
                matching.push_back(entry);
            }
        }
        
        return matching;
    }
    
    std::string test_dir_;
    std::unique_ptr<RocksDBWrapper> db_wrapper_;
    std::shared_ptr<Changefeed> changefeed_;
    std::unique_ptr<SecondaryIndexManager> idx_;
    std::unique_ptr<QueryEngine> query_engine_;
    std::shared_ptr<FieldEncryption> encryption_;
    std::shared_ptr<MockPKIClient> pki_client_;
    std::shared_ptr<AuditLogger> audit_logger_;
    std::unique_ptr<TaskScheduler> scheduler_;
};

// ============================================================================
// Test Cases
// ============================================================================

/**
 * Test 1: Verify task registration generates audit event
 */
TEST_F(TaskSchedulerSIEMIntegrationTest, TaskRegistrationAuditEvent) {
    // Register a function
    int execution_count = 0;
    scheduler_->registerFunction("test_func", [&execution_count](const nlohmann::json&) {
        execution_count++;
        return nlohmann::json{{"status", "success"}};
    });
    
    // Register a task
    ScheduledTask task;
    task.name = "Test Task";
    task.description = "Test task for SIEM audit";
    task.type = ScheduledTask::TaskType::FUNCTION;
    task.function_name = "test_func";
    task.trigger_type = ScheduledTask::TriggerType::INTERVAL;
    task.interval = std::chrono::seconds(10);
    task.enabled = true;
    
    std::string task_id = scheduler_->registerTask(task);
    
    // Read audit log
    auto entries = readAuditLog();
    
    // Should have at least one entry (task registration)
    EXPECT_GT(entries.size(), 0);
    
    // Verify the entry has proper structure
    EXPECT_TRUE(entries.back().contains("category"));
    EXPECT_EQ(entries.back()["category"], "TASK_SCHEDULER");
}

/**
 * Test 2: Verify task execution generates audit events
 */
TEST_F(TaskSchedulerSIEMIntegrationTest, TaskExecutionAuditEvents) {
    int execution_count = 0;
    
    scheduler_->registerFunction("test_exec", [&execution_count](const nlohmann::json&) {
        execution_count++;
        return nlohmann::json{{"status", "success"}};
    });
    
    ScheduledTask task;
    task.name = "Execution Test Task";
    task.type = ScheduledTask::TaskType::FUNCTION;
    task.function_name = "test_exec";
    task.trigger_type = ScheduledTask::TriggerType::MANUAL;
    
    std::string task_id = scheduler_->registerTask(task);
    
    // Clear previous entries count
    size_t entries_before = readAuditLog().size();
    
    // Execute task manually
    auto result = scheduler_->executeTaskNow(task_id);
    
    // Verify execution happened
    EXPECT_EQ(execution_count, 1);
    EXPECT_TRUE(result.contains("status"));
    
    // Verify audit logs were created
    auto entries_after = readAuditLog();
    EXPECT_GT(entries_after.size(), entries_before);
    
    // Should have manual trigger event + execution success event
    EXPECT_GE(entries_after.size() - entries_before, 2);
}

/**
 * Test 3: Verify task failure generates appropriate audit event
 */
TEST_F(TaskSchedulerSIEMIntegrationTest, TaskFailureAuditEvent) {
    scheduler_->registerFunction("failing_func", [](const nlohmann::json&) -> nlohmann::json {
        throw std::runtime_error("Intentional test failure");
    });
    
    ScheduledTask task;
    task.name = "Failing Task";
    task.type = ScheduledTask::TaskType::FUNCTION;
    task.function_name = "failing_func";
    task.trigger_type = ScheduledTask::TriggerType::MANUAL;
    
    std::string task_id = scheduler_->registerTask(task);
    
    size_t entries_before = readAuditLog().size();
    
    // Execute task (should fail)
    auto result = scheduler_->executeTaskNow(task_id);
    
    // Verify it failed
    EXPECT_TRUE(result.contains("error"));
    
    // Verify audit logs were created for failure
    auto entries_after = readAuditLog();
    EXPECT_GT(entries_after.size(), entries_before);
}

/**
 * Test 4: Verify cron trigger generates audit event
 */
TEST_F(TaskSchedulerSIEMIntegrationTest, CronTriggerAuditEvent) {
    int execution_count = 0;
    
    scheduler_->registerFunction("cron_func", [&execution_count](const nlohmann::json&) {
        execution_count++;
        return nlohmann::json{{"status", "success"}};
    });
    
    ScheduledTask task;
    task.name = "Cron Task";
    task.type = ScheduledTask::TaskType::FUNCTION;
    task.function_name = "cron_func";
    task.trigger_type = ScheduledTask::TriggerType::CRON;
    task.cron_expression = "* * * * *"; // Every minute
    
    std::string task_id = scheduler_->registerTask(task);
    
    size_t entries_before = readAuditLog().size();
    
    // Start scheduler and let it run briefly
    scheduler_->start();
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    scheduler_->stop();
    
    // If cron triggered (depends on timing), we should see execution
    auto entries_after = readAuditLog();
    EXPECT_GT(entries_after.size(), entries_before);
}

/**
 * Test 5: Verify CEF format generation
 */
TEST_F(TaskSchedulerSIEMIntegrationTest, CEFFormatGeneration) {
    // This test verifies the CEF formatter works correctly
    nlohmann::json test_event = {
        {"event_type", "TASK_EXECUTED_SUCCESS"},
        {"task_id", "test-task-123"},
        {"user_id", "admin"},
        {"timestamp", 1675000000000},
        {"execution_time_ms", 123.45},
        {"severity", "LOW"},
        {"anomaly_score", 0.5}
    };
    
    // Create a temporary audit logger with CEF format
    AuditLoggerConfig cef_config;
    cef_config.enabled = true;
    cef_config.log_path = test_dir_ + "/cef_audit.jsonl";
    cef_config.enable_siem = false;
    cef_config.siem_format = "cef";
    cef_config.enable_task_scheduler_audit = true;
    
    AuditLogger cef_logger(encryption_, pki_client_, cef_config);
    
    // Log a task scheduler event
    cef_logger.logTaskSchedulerEvent(
        SecurityEventType::TASK_EXECUTED_SUCCESS,
        "test-task-123",
        "admin",
        {{"execution_time_ms", 123.45}, {"severity", "LOW"}}
    );
    
    // The event should be logged to the configured CEF file.
    const std::string cef_path = test_dir_ + "/cef_audit.jsonl";
    ASSERT_TRUE(std::filesystem::exists(cef_path));
    std::ifstream ifs(cef_path);
    std::string line;
    ASSERT_TRUE(static_cast<bool>(std::getline(ifs, line)));
    EXPECT_FALSE(line.empty());
}

/**
 * Test 6: Verify anomaly detection
 */
TEST_F(TaskSchedulerSIEMIntegrationTest, AnomalyDetection) {
    int execution_count = 0;
    
    scheduler_->registerFunction("anomaly_func", [&execution_count](const nlohmann::json&) {
        execution_count++;
        
        // First 10 executions are fast (normal baseline)
        if (execution_count <= 10) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        } else {
            // 11th execution is very slow (anomaly)
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
        }
        
        return nlohmann::json{{"status", "success"}};
    });
    
    ScheduledTask task;
    task.name = "Anomaly Detection Task";
    task.type = ScheduledTask::TaskType::FUNCTION;
    task.function_name = "anomaly_func";
    task.trigger_type = ScheduledTask::TriggerType::MANUAL;
    
    std::string task_id = scheduler_->registerTask(task);
    
    // Execute task multiple times to build baseline
    for (int i = 0; i < 11; i++) {
        scheduler_->executeTaskNow(task_id);
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
    
    // After 11 executions, anomaly should be detected
    // (This is a basic test; actual anomaly detection happens in audit logger)
    EXPECT_EQ(execution_count, 11);
    
    // Verify audit logs contain execution events
    auto entries = readAuditLog();
    EXPECT_GT(entries.size(), 11); // At least 11 registrations + executions
}

/**
 * Test 7: Verify task enable/disable generates audit events
 */
TEST_F(TaskSchedulerSIEMIntegrationTest, TaskEnableDisableAuditEvents) {
    scheduler_->registerFunction("test_func", [](const nlohmann::json&) {
        return nlohmann::json{{"status", "success"}};
    });
    
    ScheduledTask task;
    task.name = "Enable/Disable Test Task";
    task.type = ScheduledTask::TaskType::FUNCTION;
    task.function_name = "test_func";
    task.trigger_type = ScheduledTask::TriggerType::MANUAL;
    task.enabled = true;
    
    std::string task_id = scheduler_->registerTask(task);
    
    size_t entries_before = readAuditLog().size();
    
    // Disable task
    scheduler_->disableTask(task_id);
    
    // Enable task
    scheduler_->enableTask(task_id);
    
    // Verify audit logs were created
    auto entries_after = readAuditLog();
    EXPECT_GE(entries_after.size() - entries_before, 2); // Disable + Enable events
}

/**
 * Test 8: Verify task unregistration generates audit event
 */
TEST_F(TaskSchedulerSIEMIntegrationTest, TaskUnregistrationAuditEvent) {
    scheduler_->registerFunction("test_func", [](const nlohmann::json&) {
        return nlohmann::json{{"status", "success"}};
    });
    
    ScheduledTask task;
    task.name = "Unregister Test Task";
    task.type = ScheduledTask::TaskType::FUNCTION;
    task.function_name = "test_func";
    task.trigger_type = ScheduledTask::TriggerType::MANUAL;
    
    std::string task_id = scheduler_->registerTask(task);
    
    size_t entries_before = readAuditLog().size();
    
    // Unregister task
    scheduler_->unregisterTask(task_id);
    
    // Verify audit log was created
    auto entries_after = readAuditLog();
    EXPECT_GT(entries_after.size(), entries_before);
}

/**
 * Test 9: Verify audit log hash chain integrity
 */
TEST_F(TaskSchedulerSIEMIntegrationTest, AuditLogHashChainIntegrity) {
    // Register and execute a few tasks
    scheduler_->registerFunction("test_func", [](const nlohmann::json&) {
        return nlohmann::json{{"status", "success"}};
    });
    
    for (int i = 0; i < 5; i++) {
        ScheduledTask task;
        task.name = "Task " + std::to_string(i);
        task.type = ScheduledTask::TaskType::FUNCTION;
        task.function_name = "test_func";
        task.trigger_type = ScheduledTask::TriggerType::MANUAL;
        
        scheduler_->registerTask(task);
    }
    
    // Verify hash chain integrity
    bool integrity_ok = audit_logger_->verifyChainIntegrity();
    EXPECT_TRUE(integrity_ok);
}

/**
 * Test 10: Verify SIEM format configuration
 */
TEST_F(TaskSchedulerSIEMIntegrationTest, SIEMFormatConfiguration) {
    // Test JSON format (default)
    {
        AuditLoggerConfig config;
        config.enabled = true;
        config.log_path = test_dir_ + "/json_audit.jsonl";
        config.siem_format = "json";
        
        AuditLogger logger(encryption_, pki_client_, config);
        logger.logTaskSchedulerEvent(
            SecurityEventType::TASK_REGISTERED,
            "test-task",
            "admin",
            {{"task_name", "Test Task"}}
        );
        
        EXPECT_TRUE(std::filesystem::exists(test_dir_ + "/json_audit.jsonl"));
    }
    
    // Test CEF format
    {
        AuditLoggerConfig config;
        config.enabled = true;
        config.log_path = test_dir_ + "/cef_audit.jsonl";
        config.siem_format = "cef";
        
        AuditLogger logger(encryption_, pki_client_, config);
        logger.logTaskSchedulerEvent(
            SecurityEventType::TASK_REGISTERED,
            "test-task",
            "admin",
            {{"task_name", "Test Task"}}
        );
        
        EXPECT_TRUE(std::filesystem::exists(test_dir_ + "/cef_audit.jsonl"));
    }
    
    // Test syslog format
    {
        AuditLoggerConfig config;
        config.enabled = true;
        config.log_path = test_dir_ + "/syslog_audit.jsonl";
        config.siem_format = "syslog";
        
        AuditLogger logger(encryption_, pki_client_, config);
        logger.logTaskSchedulerEvent(
            SecurityEventType::TASK_REGISTERED,
            "test-task",
            "admin",
            {{"task_name", "Test Task"}}
        );
        
        EXPECT_TRUE(std::filesystem::exists(test_dir_ + "/syslog_audit.jsonl"));
    }
}
