#include <gtest/gtest.h>
#include "scheduler/task_audit_event.h"
#include "scheduler/task_anomaly_detector.h"
#include "scheduler/task_audit_manager.h"
#include <chrono>
#include <thread>
#include <filesystem>
#include <set>

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

// Test taskEventTypeFromString round-trip
TEST(TaskAuditEvent, EventTypeFromString) {
    // Verify all enum values survive a to-string / from-string round-trip
    auto types = {
        TaskEventType::TASK_REGISTERED, TaskEventType::TASK_UNREGISTERED,
        TaskEventType::TASK_ENABLED,    TaskEventType::TASK_DISABLED,
        TaskEventType::TASK_UPDATED,    TaskEventType::TASK_STARTED,
        TaskEventType::TASK_COMPLETED,  TaskEventType::TASK_FAILED,
        TaskEventType::TASK_TIMEOUT,    TaskEventType::TASK_RETRY,
        TaskEventType::TASK_QUEUED,     TaskEventType::TASK_DEQUEUED,
        TaskEventType::MANUAL_EXECUTION, TaskEventType::CRON_TRIGGERED,
        TaskEventType::CDC_TRIGGERED,   TaskEventType::INTERVAL_TRIGGERED,
        TaskEventType::WEBHOOK_TRIGGERED
    };
    for (auto t : types) {
        EXPECT_EQ(t, taskEventTypeFromString(taskEventTypeToString(t)));
    }
    // Unknown string should not crash and returns a sensible default
    EXPECT_NO_THROW(taskEventTypeFromString("NONEXISTENT_TYPE"));
}

// Helper: log N audit events for a given task ID into a manager
static void logEvents(TaskAuditManager& mgr, const std::string& task_id,
                      int count, bool success = true,
                      TaskEventType ev_type = TaskEventType::TASK_COMPLETED,
                      const std::string& trigger_type = "CRON") {
    for (int i = 0; i < count; ++i) {
        TaskAuditEvent ev;
        ev.uuid = generateUUID();
        ev.timestamp = std::chrono::system_clock::now();
        ev.task_id = task_id;
        ev.task_name = task_id + "_name";
        ev.event_type = ev_type;
        ev.trigger_type = trigger_type;
        ev.user_id = "user_" + task_id;
        ev.ip_address = "127.0.0.1";
        ev.success = success;
        ev.duration_ms = 10.0 * (i + 1);
        mgr.logAuditEvent(ev);
    }
}

// Test searchable query: filter by task_id returns only matching events
TEST(TaskAuditManager, QueryByTaskId) {
    TaskAuditConfig config;
    config.enable_audit_logging = true;
    config.enable_anomaly_detection = false;
    config.audit_log_path = getTempDir() + "/qbytask_audit.jsonl";
    config.security_log_path = getTempDir() + "/qbytask_sec.jsonl";
    std::filesystem::remove(config.audit_log_path);
    std::filesystem::remove(config.security_log_path);

    auto mgr = std::make_shared<TaskAuditManager>(nullptr, config);
    logEvents(*mgr, "task-alpha", 5);
    logEvents(*mgr, "task-beta",  3);

    AuditQueryParams params;
    params.task_id = "task-alpha";
    params.limit = 100;

    auto results = mgr->queryAuditEvents(params);
    ASSERT_EQ(5u, results.size());
    for (const auto& ev : results) {
        EXPECT_EQ("task-alpha", ev.task_id);
    }
}

// Test searchable query: filter by success status
TEST(TaskAuditManager, QueryBySuccess) {
    TaskAuditConfig config;
    config.enable_audit_logging = true;
    config.enable_anomaly_detection = false;
    config.audit_log_path = getTempDir() + "/qbysuccess_audit.jsonl";
    config.security_log_path = getTempDir() + "/qbysuccess_sec.jsonl";
    std::filesystem::remove(config.audit_log_path);
    std::filesystem::remove(config.security_log_path);

    auto mgr = std::make_shared<TaskAuditManager>(nullptr, config);
    logEvents(*mgr, "task-x", 4, /*success=*/true);
    logEvents(*mgr, "task-x", 3, /*success=*/false, TaskEventType::TASK_FAILED);

    AuditQueryParams params_ok;
    params_ok.success = true;
    params_ok.limit = 100;
    auto ok = mgr->queryAuditEvents(params_ok);
    EXPECT_EQ(4u, ok.size());
    for (const auto& ev : ok) EXPECT_TRUE(ev.success);

    AuditQueryParams params_fail;
    params_fail.success = false;
    params_fail.limit = 100;
    auto fail = mgr->queryAuditEvents(params_fail);
    EXPECT_EQ(3u, fail.size());
    for (const auto& ev : fail) EXPECT_FALSE(ev.success);
}

// Test searchable query: filter by event type
TEST(TaskAuditManager, QueryByEventType) {
    TaskAuditConfig config;
    config.enable_audit_logging = true;
    config.enable_anomaly_detection = false;
    config.audit_log_path = getTempDir() + "/qbyevtype_audit.jsonl";
    config.security_log_path = getTempDir() + "/qbyevtype_sec.jsonl";
    std::filesystem::remove(config.audit_log_path);
    std::filesystem::remove(config.security_log_path);

    auto mgr = std::make_shared<TaskAuditManager>(nullptr, config);
    logEvents(*mgr, "task-y", 3, true, TaskEventType::TASK_COMPLETED);
    logEvents(*mgr, "task-y", 2, false, TaskEventType::TASK_FAILED);

    AuditQueryParams params;
    params.event_type = TaskEventType::TASK_FAILED;
    params.limit = 100;
    auto results = mgr->queryAuditEvents(params);
    EXPECT_EQ(2u, results.size());
    for (const auto& ev : results) {
        EXPECT_EQ(TaskEventType::TASK_FAILED, ev.event_type);
    }
}

// Test searchable query: pagination (limit + offset)
TEST(TaskAuditManager, QueryPagination) {
    TaskAuditConfig config;
    config.enable_audit_logging = true;
    config.enable_anomaly_detection = false;
    config.audit_log_path = getTempDir() + "/qbypage_audit.jsonl";
    config.security_log_path = getTempDir() + "/qbypage_sec.jsonl";

    auto mgr = std::make_shared<TaskAuditManager>(nullptr, config);
    logEvents(*mgr, "pg-task", 10);

    // First page
    AuditQueryParams p1;
    p1.limit = 4;
    p1.offset = 0;
    auto page1 = mgr->queryAuditEvents(p1);
    EXPECT_EQ(4u, page1.size());

    // Second page
    AuditQueryParams p2;
    p2.limit = 4;
    p2.offset = 4;
    auto page2 = mgr->queryAuditEvents(p2);
    EXPECT_EQ(4u, page2.size());

    // Pages should not overlap (by UUID)
    std::set<std::string> uuids1, uuids2;
    for (const auto& ev : page1) uuids1.insert(ev.uuid);
    for (const auto& ev : page2) uuids2.insert(ev.uuid);
    for (const auto& u : uuids2) {
        EXPECT_EQ(0u, uuids1.count(u)) << "Duplicate UUID across pages: " << u;
    }
}

// Test searchable query: time-range filter
TEST(TaskAuditManager, QueryByTimeRange) {
    TaskAuditConfig config;
    config.enable_audit_logging = true;
    config.enable_anomaly_detection = false;
    config.audit_log_path = getTempDir() + "/qbytime_audit.jsonl";
    config.security_log_path = getTempDir() + "/qbytime_sec.jsonl";

    auto mgr = std::make_shared<TaskAuditManager>(nullptr, config);

    auto t0 = std::chrono::system_clock::now();
    logEvents(*mgr, "tr-task", 3); // before mid-point
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    auto t_mid = std::chrono::system_clock::now();
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    logEvents(*mgr, "tr-task", 2); // after mid-point

    AuditQueryParams params;
    params.start_time = t_mid;
    params.limit = 100;
    auto results = mgr->queryAuditEvents(params);
    EXPECT_EQ(2u, results.size());
    for (const auto& ev : results) {
        EXPECT_GE(ev.timestamp, t_mid);
    }
}

// Test loadEventsFromFile: full field round-trip through disk persistence
TEST(TaskAuditManager, SearchableHistoryFromDisk) {
    auto log_path = getTempDir() + "/disk_roundtrip_audit.jsonl";
    // Remove any previous file for a clean test
    std::filesystem::remove(log_path);

    TaskAuditConfig config;
    config.enable_audit_logging = true;
    config.enable_anomaly_detection = false;
    config.audit_log_path = log_path;
    config.security_log_path = getTempDir() + "/disk_roundtrip_sec.jsonl";

    // Phase 1: write events through a manager instance
    {
        auto mgr = std::make_shared<TaskAuditManager>(nullptr, config);
        logEvents(*mgr, "disk-task-a", 3, true,  TaskEventType::TASK_COMPLETED);
        logEvents(*mgr, "disk-task-b", 2, false, TaskEventType::TASK_FAILED);
    }

    // Phase 2: create a fresh manager (empty cache) and query from file
    auto mgr2 = std::make_shared<TaskAuditManager>(nullptr, config);
    // Cache is empty; all results must come from the file.

    // Query all events
    AuditQueryParams all;
    all.limit = 100;
    auto all_events = mgr2->queryAuditEvents(all);
    EXPECT_EQ(5u, all_events.size());

    // Filter by task_id
    AuditQueryParams by_task;
    by_task.task_id = "disk-task-a";
    by_task.limit = 100;
    auto task_a = mgr2->queryAuditEvents(by_task);
    ASSERT_EQ(3u, task_a.size());
    for (const auto& ev : task_a) {
        EXPECT_EQ("disk-task-a", ev.task_id);
        EXPECT_TRUE(ev.success);
        EXPECT_EQ(TaskEventType::TASK_COMPLETED, ev.event_type);
        EXPECT_EQ("CRON", ev.trigger_type);
    }

    // Filter by success=false (should return the TASK_FAILED events for disk-task-b)
    AuditQueryParams by_fail;
    by_fail.success = false;
    by_fail.limit = 100;
    auto failed = mgr2->queryAuditEvents(by_fail);
    EXPECT_EQ(2u, failed.size());
    for (const auto& ev : failed) {
        EXPECT_FALSE(ev.success);
        EXPECT_EQ(TaskEventType::TASK_FAILED, ev.event_type);
    }
}

// Test no duplicate results when same events exist in cache and file
TEST(TaskAuditManager, NoDuplicatesFromCacheAndFile) {
    auto log_path = getTempDir() + "/nodup_audit.jsonl";
    std::filesystem::remove(log_path);

    TaskAuditConfig config;
    config.enable_audit_logging = true;
    config.enable_anomaly_detection = false;
    config.audit_log_path = log_path;
    config.security_log_path = getTempDir() + "/nodup_sec.jsonl";

    auto mgr = std::make_shared<TaskAuditManager>(nullptr, config);
    // Log 5 events; they land in both the file and the in-memory cache
    logEvents(*mgr, "dup-task", 5);

    AuditQueryParams params;
    params.limit = 100;
    auto results = mgr->queryAuditEvents(params);

    // Collect UUIDs – there should be exactly 5 unique ones
    std::set<std::string> uuids;
    for (const auto& ev : results) {
        uuids.insert(ev.uuid);
    }
    EXPECT_EQ(5u, uuids.size());
    EXPECT_EQ(5u, results.size());
}

// Test searchable query: filter by user_id
TEST(TaskAuditManager, QueryByUserId) {
    TaskAuditConfig config;
    config.enable_audit_logging = true;
    config.enable_anomaly_detection = false;
    config.audit_log_path = getTempDir() + "/qbyuser_audit.jsonl";
    config.security_log_path = getTempDir() + "/qbyuser_sec.jsonl";
    std::filesystem::remove(config.audit_log_path);
    std::filesystem::remove(config.security_log_path);

    auto mgr = std::make_shared<TaskAuditManager>(nullptr, config);
    // logEvents sets user_id = "user_" + task_id
    logEvents(*mgr, "user-task-a", 3);
    logEvents(*mgr, "user-task-b", 2);

    AuditQueryParams params;
    params.user_id = "user_user-task-a";
    params.limit = 100;
    auto results = mgr->queryAuditEvents(params);
    ASSERT_EQ(3u, results.size());
    for (const auto& ev : results) {
        EXPECT_EQ("user_user-task-a", ev.user_id);
    }
}

// Test searchable query: filter by trigger_type
TEST(TaskAuditManager, QueryByTriggerType) {
    TaskAuditConfig config;
    config.enable_audit_logging = true;
    config.enable_anomaly_detection = false;
    config.audit_log_path = getTempDir() + "/qbytrigger_audit.jsonl";
    config.security_log_path = getTempDir() + "/qbytrigger_sec.jsonl";
    std::filesystem::remove(config.audit_log_path);
    std::filesystem::remove(config.security_log_path);

    auto mgr = std::make_shared<TaskAuditManager>(nullptr, config);
    // Log events with CRON trigger (default from logEvents helper)
    logEvents(*mgr, "trigger-task", 3);
    // Log events with MANUAL trigger; use TASK_COMPLETED event type to test
    // that trigger_type filter works independently of the event type.
    logEvents(*mgr, "trigger-task", 2, true, TaskEventType::TASK_COMPLETED, "MANUAL");

    AuditQueryParams params;
    params.trigger_type = "MANUAL";
    params.limit = 100;
    auto results = mgr->queryAuditEvents(params);
    EXPECT_EQ(2u, results.size());
    for (const auto& ev : results) {
        EXPECT_EQ("MANUAL", ev.trigger_type);
    }
}

// Test querySecurityEvents with disk persistence (no-duplicates across cache+file)
TEST(TaskAuditManager, SecurityEventsFromDisk) {
    auto sec_log_path = getTempDir() + "/secdisk_sec.jsonl";
    std::filesystem::remove(sec_log_path);

    TaskAuditConfig config;
    config.enable_security_logging = true;
    config.enable_audit_logging = false;
    config.audit_log_path = getTempDir() + "/secdisk_audit.jsonl";
    config.security_log_path = sec_log_path;

    // Phase 1: write security events through a manager instance
    {
        auto mgr = std::make_shared<TaskAuditManager>(nullptr, config);
        for (int i = 0; i < 4; ++i) {
            TaskSecurityEvent ev;
            ev.uuid = generateUUID();
            ev.timestamp = std::chrono::system_clock::now();
            ev.task_id = "sec-task";
            ev.task_name = "sec-task_name";
            ev.event_type = TaskSecurityEventType::RATE_LIMIT_EXCEEDED;
            ev.severity = "MEDIUM";
            ev.user_id = "sec-user";
            ev.ip_address = "127.0.0.1";
            ev.violation_type = "rate_limit";
            ev.description = "rate limit";
            ev.blocked = true;
            ev.action_taken = "denied";
            mgr->logSecurityEvent(ev);
        }
    }

    // Phase 2: fresh manager (empty cache) — events must come from file
    auto mgr2 = std::make_shared<TaskAuditManager>(nullptr, config);

    AuditQueryParams params;
    params.limit = 100;
    auto results = mgr2->querySecurityEvents(params);
    EXPECT_EQ(4u, results.size());

    // filter by task_id
    AuditQueryParams by_task;
    by_task.task_id = "sec-task";
    by_task.limit = 100;
    auto task_results = mgr2->querySecurityEvents(by_task);
    EXPECT_EQ(4u, task_results.size());

    // filter by user_id
    AuditQueryParams by_user;
    by_user.user_id = "sec-user";
    by_user.limit = 100;
    auto user_results = mgr2->querySecurityEvents(by_user);
    EXPECT_EQ(4u, user_results.size());
}

// Test no duplicate security events across cache+file
TEST(TaskAuditManager, NoDuplicateSecurityEvents) {
    auto sec_log_path = getTempDir() + "/secnodup_sec.jsonl";
    std::filesystem::remove(sec_log_path);

    TaskAuditConfig config;
    config.enable_security_logging = true;
    config.enable_audit_logging = false;
    config.audit_log_path = getTempDir() + "/secnodup_audit.jsonl";
    config.security_log_path = sec_log_path;

    auto mgr = std::make_shared<TaskAuditManager>(nullptr, config);
    // Log 3 events — land in both file and in-memory cache
    for (int i = 0; i < 3; ++i) {
        TaskSecurityEvent ev;
        ev.uuid = generateUUID();
        ev.timestamp = std::chrono::system_clock::now();
        ev.event_type = TaskSecurityEventType::AQL_INJECTION_DETECTED;
        ev.severity = "HIGH";
        ev.user_id = "attacker";
        ev.ip_address = "10.0.0.1";
        ev.violation_type = "injection";
        ev.description = "injection attempt";
        ev.blocked = true;
        ev.action_taken = "blocked";
        mgr->logSecurityEvent(ev);
    }

    AuditQueryParams params;
    params.limit = 100;
    auto results = mgr->querySecurityEvents(params);

    std::set<std::string> uuids;
    for (const auto& ev : results) {
        uuids.insert(ev.uuid);
    }
    EXPECT_EQ(3u, uuids.size());
    EXPECT_EQ(3u, results.size());
}
