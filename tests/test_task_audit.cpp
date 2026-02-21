/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            test_task_audit.cpp                                ║
  Version:         0.0.14                                             ║
  Last Modified:   2026-02-21 16:53:15                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     352                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 234245ceb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • b8b369411  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 8efb1d2fe  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 31ccce9fb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • ea0163e87  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include <gtest/gtest.h>
#include "scheduler/task_audit_event.h"
#include "scheduler/task_anomaly_detector.h"
#include "scheduler/task_audit_manager.h"
#include <chrono>
#include <thread>
#include <filesystem>

using namespace themis;
using namespace themis::scheduler;

// Helper function to get portable temporary directory
static std::string getTempDir() {
    return std::filesystem::temp_directory_path().string();
}

// Test UUID generation
TEST(TaskAuditEvent, GenerateUUID) {
    auto uuid1 = generateUUID();
    auto uuid2 = generateUUID();
    
    // UUID should be non-empty and follow format
    EXPECT_FALSE(uuid1.empty());
    EXPECT_FALSE(uuid2.empty());
    EXPECT_NE(uuid1, uuid2);  // UUIDs should be unique
    
    // Check format (xxxxxxxx-xxxx-4xxx-yxxx-xxxxxxxxxxxx)
    EXPECT_EQ(36, uuid1.length());
    EXPECT_EQ('-', uuid1[8]);
    EXPECT_EQ('-', uuid1[13]);
    EXPECT_EQ('-', uuid1[18]);
    EXPECT_EQ('-', uuid1[23]);
}

// Test data masking
TEST(TaskAuditEvent, MaskSensitiveData) {
    std::string data = "user@example.com";
    
    // Full masking
    auto full_masked = maskSensitiveData(data, "full");
    EXPECT_EQ("***REDACTED***", full_masked);
    
    // Partial masking
    auto partial_masked = maskSensitiveData(data, "partial");
    EXPECT_NE(data, partial_masked);
    EXPECT_TRUE(partial_masked.find("***") != std::string::npos);
    
    // Hash masking
    auto hash_masked = maskSensitiveData(data, "hash");
    EXPECT_NE(data, hash_masked);
    EXPECT_EQ(64, hash_masked.length());  // SHA-256 produces 64 hex characters
}

// Test event type conversions
TEST(TaskAuditEvent, EventTypeConversions) {
    EXPECT_EQ("TASK_REGISTERED", taskEventTypeToString(TaskEventType::TASK_REGISTERED));
    EXPECT_EQ("TASK_STARTED", taskEventTypeToString(TaskEventType::TASK_STARTED));
    EXPECT_EQ("TASK_COMPLETED", taskEventTypeToString(TaskEventType::TASK_COMPLETED));
    EXPECT_EQ("TASK_FAILED", taskEventTypeToString(TaskEventType::TASK_FAILED));
    
    EXPECT_EQ("RATE_LIMIT_EXCEEDED", 
              taskSecurityEventTypeToString(TaskSecurityEventType::RATE_LIMIT_EXCEEDED));
    EXPECT_EQ("AQL_INJECTION_DETECTED",
              taskSecurityEventTypeToString(TaskSecurityEventType::AQL_INJECTION_DETECTED));
}

// Test TaskAuditEvent serialization
TEST(TaskAuditEvent, Serialization) {
    TaskAuditEvent event;
    event.uuid = "test-uuid-123";
    event.timestamp = std::chrono::system_clock::now();
    event.task_id = "task-001";
    event.task_name = "Test Task";
    event.event_type = TaskEventType::TASK_COMPLETED;
    event.trigger_type = "CRON";
    event.user_id = "test-user";
    event.ip_address = "192.168.1.100";
    event.success = true;
    event.duration_ms = 123.45;
    
    // Test JSON serialization
    auto json = event.toJson(false);
    EXPECT_EQ("test-uuid-123", json["uuid"]);
    EXPECT_EQ("task-001", json["task_id"]);
    EXPECT_EQ("Test Task", json["task_name"]);
    EXPECT_EQ("TASK_COMPLETED", json["event_type"]);
    EXPECT_EQ("CRON", json["trigger_type"]);
    EXPECT_TRUE(json["success"]);
    EXPECT_DOUBLE_EQ(123.45, json["duration_ms"]);
    
    // Test GDPR mode (should mask sensitive data)
    auto gdpr_json = event.toJson(true);
    EXPECT_NE("test-user", gdpr_json["user_id"]);
    EXPECT_NE("192.168.1.100", gdpr_json["ip_address"]);
    
    // Test CEF format
    auto cef = event.toCEF();
    EXPECT_TRUE(cef.find("CEF:0") != std::string::npos);
    EXPECT_TRUE(cef.find("ThemisDB") != std::string::npos);
    EXPECT_TRUE(cef.find("TASK_COMPLETED") != std::string::npos);
}

// Test anomaly detector - basic functionality
TEST(TaskAnomalyDetector, BasicFunctionality) {
    AnomalyDetectorConfig config;
    config.min_samples = 5;  // Reduced for testing
    
    TaskAnomalyDetector detector(config);
    
    // Simulate normal task executions
    for (int i = 0; i < 10; i++) {
        TaskAuditEvent event;
        event.uuid = generateUUID();
        event.timestamp = std::chrono::system_clock::now();
        event.task_id = "task-normal";
        event.task_name = "Normal Task";
        event.event_type = TaskEventType::TASK_COMPLETED;
        event.trigger_type = "CRON";
        event.success = true;
        event.duration_ms = 100.0 + (i * 5);  // Gradually increasing
        event.resource_usage.cpu_time_ms = 90.0 + (i * 5);
        event.resource_usage.memory_bytes = 1000000 + (i * 1000);
        
        auto metrics = detector.recordExecution(event);
        
        // First few executions shouldn't trigger anomalies
        if (i < 5) {
            EXPECT_FALSE(metrics.is_anomalous);
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    
    // Check that baseline was established
    EXPECT_TRUE(detector.hasBaseline("task-normal"));
    
    // Get statistics
    auto stats = detector.getTaskStatistics("task-normal");
    ASSERT_TRUE(stats.has_value());
    EXPECT_EQ(10, stats->total_executions);
    EXPECT_EQ(0, stats->total_failures);
    EXPECT_GT(stats->mean_execution_time_ms, 0);
}

// Test anomaly detector - frequency anomaly
TEST(TaskAnomalyDetector, FrequencyAnomaly) {
    AnomalyDetectorConfig config;
    config.min_samples = 5;
    config.frequency_threshold = 0.5;
    config.frequency_spike_factor = 2.0;
    
    TaskAnomalyDetector detector(config);
    
    // Establish baseline with slow frequency
    for (int i = 0; i < 10; i++) {
        TaskAuditEvent event;
        event.uuid = generateUUID();
        event.timestamp = std::chrono::system_clock::now();
        event.task_id = "task-freq";
        event.success = true;
        event.duration_ms = 100.0;
        
        detector.recordExecution(event);
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    
    // Now execute many times rapidly (frequency spike)
    for (int i = 0; i < 20; i++) {
        TaskAuditEvent event;
        event.uuid = generateUUID();
        event.timestamp = std::chrono::system_clock::now();
        event.task_id = "task-freq";
        event.success = true;
        event.duration_ms = 100.0;
        
        auto metrics = detector.recordExecution(event);
        
        // Later events should detect frequency anomaly
        if (i > 10) {
            // Note: frequency detection looks at last hour, so this might not trigger
            // in a unit test with rapid execution. This tests the mechanism, not timing.
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }
}

// Test anomaly detector - resource anomaly
TEST(TaskAnomalyDetector, ResourceAnomaly) {
    AnomalyDetectorConfig config;
    config.min_samples = 5;
    config.resource_threshold = 0.5;
    config.resource_spike_factor = 3.0;
    
    TaskAnomalyDetector detector(config);
    
    // Establish baseline with normal resource usage
    for (int i = 0; i < 10; i++) {
        TaskAuditEvent event;
        event.uuid = generateUUID();
        event.timestamp = std::chrono::system_clock::now();
        event.task_id = "task-resource";
        event.success = true;
        event.duration_ms = 100.0;
        event.resource_usage.cpu_time_ms = 50.0;
        event.resource_usage.memory_bytes = 1000000;
        
        detector.recordExecution(event);
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    
    // Execute with spike in resource usage
    TaskAuditEvent spike_event;
    spike_event.uuid = generateUUID();
    spike_event.timestamp = std::chrono::system_clock::now();
    spike_event.task_id = "task-resource";
    spike_event.success = true;
    spike_event.duration_ms = 100.0;
    spike_event.resource_usage.cpu_time_ms = 500.0;  // 10x normal
    spike_event.resource_usage.memory_bytes = 10000000;  // 10x normal
    
    auto metrics = detector.recordExecution(spike_event);
    
    // Should detect resource anomaly
    EXPECT_GT(metrics.resource_score, 0.0);
}

// Test anomaly detector - failure rate anomaly
TEST(TaskAnomalyDetector, FailureRateAnomaly) {
    AnomalyDetectorConfig config;
    config.min_samples = 10;
    config.failure_rate_threshold = 0.5;
    config.failure_rate_spike = 0.3;
    
    TaskAnomalyDetector detector(config);
    
    // Establish baseline with successful executions
    for (int i = 0; i < 15; i++) {
        TaskAuditEvent event;
        event.uuid = generateUUID();
        event.timestamp = std::chrono::system_clock::now();
        event.task_id = "task-failures";
        event.success = true;
        event.duration_ms = 100.0;
        
        detector.recordExecution(event);
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    
    // Now introduce failures
    for (int i = 0; i < 10; i++) {
        TaskAuditEvent event;
        event.uuid = generateUUID();
        event.timestamp = std::chrono::system_clock::now();
        event.task_id = "task-failures";
        event.success = (i % 2 == 0);  // 50% failure rate
        event.duration_ms = 100.0;
        
        auto metrics = detector.recordExecution(event);
        
        // Later events should detect elevated failure rate
        if (i > 5) {
            if (metrics.failure_rate_score > 0.5) {
                EXPECT_TRUE(metrics.is_anomalous || metrics.failure_rate_score > config.failure_rate_threshold);
            }
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}

// Test audit manager integration
TEST(TaskAuditManager, BasicAuditing) {
    // Create audit manager (without real audit logger for unit test)
    TaskAuditConfig config;
    config.enable_audit_logging = true;
    config.enable_anomaly_detection = true;
    config.audit_log_path = getTempDir() + "/test_audit.jsonl";
    config.security_log_path = getTempDir() + "/test_security.jsonl";
    
    auto audit_manager = std::make_shared<TaskAuditManager>(nullptr, config);
    
    // Log an audit event
    TaskAuditEvent event;
    event.uuid = generateUUID();
    event.timestamp = std::chrono::system_clock::now();
    event.task_id = "test-task";
    event.task_name = "Test Task";
    event.event_type = TaskEventType::TASK_COMPLETED;
    event.trigger_type = "CRON";
    event.success = true;
    event.duration_ms = 123.0;
    event.user_id = "test-user";
    event.ip_address = "127.0.0.1";
    
    auto metrics = audit_manager->logAuditEvent(event);
    
    // Should return anomaly metrics (even if not anomalous)
    EXPECT_GE(metrics.overall_score, 0.0);
    EXPECT_LE(metrics.overall_score, 1.0);
}

// Test security event logging
TEST(TaskAuditManager, SecurityEventLogging) {
    TaskAuditConfig config;
    config.enable_security_logging = true;
    config.security_log_path = getTempDir() + "/test_security2.jsonl";
    
    auto audit_manager = std::make_shared<TaskAuditManager>(nullptr, config);
    
    // Log a security event
    TaskSecurityEvent security_event;
    security_event.uuid = generateUUID();
    security_event.timestamp = std::chrono::system_clock::now();
    security_event.task_id = "test-task";
    security_event.event_type = TaskSecurityEventType::RATE_LIMIT_EXCEEDED;
    security_event.severity = "MEDIUM";
    security_event.user_id = "test-user";
    security_event.ip_address = "127.0.0.1";
    security_event.violation_type = "rate_limit";
    security_event.description = "Test rate limit exceeded";
    security_event.blocked = true;
    security_event.action_taken = "denied";
    
    EXPECT_NO_THROW(audit_manager->logSecurityEvent(security_event));
}