# Task Scheduler Audit Events & Anomaly Detection

## Overview

The Task Scheduler Audit Events & Anomaly Detection system provides comprehensive logging, security monitoring, and behavioral analysis for all task executions in ThemisDB. This implementation meets enterprise governance and compliance requirements including ISO 27001, SOC2, and GDPR.

## Features

### 1. Comprehensive Audit Logging
- **Complete Event Tracking**: Every task execution (including failures and queue operations) is logged with structured data
- **Rich Event Data**: UUID, timestamp, task ID, trigger type, event type, user, IP address, resource usage, success/failure status, errors, and anomaly scores
- **Tamper-Evident Logging**: Integration with ThemisDB's existing AuditLogger for hash-chain based tamper detection
- **GDPR Compliance**: Automatic data masking and redaction for sensitive fields (user IDs, IP addresses, session IDs)

### 2. Security Event Reporting
Separate security event stream for policy violations:
- **Rate Limit Violations**: Excessive task execution attempts
- **Resource Limit Violations**: Tasks exceeding CPU, memory, or timeout limits
- **Injection Detection**: AQL injection and cron expression validation failures
- **Anomaly Detection**: Behavioral deviations flagged as security events

### 3. Anomaly Detection
Multi-dimensional anomaly detection system:
- **Frequency Anomalies**: Sudden spikes or drops in execution frequency
- **Pattern Anomalies**: Deviations from normal execution intervals
- **Resource Anomalies**: Unexpected CPU, memory, or I/O usage
- **Failure Rate Anomalies**: Elevated failure rates indicating system issues

### 4. Query & Export Interface
Flexible querying and export capabilities:
- **Query API**: Filter events by time range, task ID, user, event type, success/failure, anomaly status
- **Export Formats**: JSON, JSONL, CEF (Common Event Format), CSV
- **SIEM Integration**: CEF, Splunk HEC, and Elastic ECS format support
- **Pagination**: Efficient handling of large result sets

## Architecture

```
┌──────────────────┐
│  TaskScheduler   │
└────────┬─────────┘
         │
         │ integrates
         ▼
┌──────────────────────────┐
│   TaskAuditManager       │
│  - Event logging         │
│  - Security reporting    │
│  - Query interface       │
└──────┬──────────┬────────┘
       │          │
       │          │
┌──────▼─────┐   ┌▼──────────────────┐
│  AuditLogger│  │ TaskAnomalyDetector│
│  (existing) │  │  - Frequency       │
│  - Tamper   │  │  - Pattern         │
│    evident  │  │  - Resource        │
│  - Hash     │  │  - Failure rate    │
│    chain    │  └───────────────────┘
└─────────────┘
```

## Components

### TaskAuditEvent
Comprehensive audit event structure containing:
```cpp
struct TaskAuditEvent {
    std::string uuid;                           // Unique event ID
    std::chrono::system_clock::time_point timestamp;
    std::string task_id;
    std::string task_name;
    TaskEventType event_type;                   // STARTED, COMPLETED, FAILED, etc.
    std::string trigger_type;                   // CRON, CDC, INTERVAL, MANUAL
    std::string user_id;                        // Can be masked
    std::string ip_address;                     // Can be masked
    bool success;
    std::optional<std::string> error_message;
    TaskResourceUsage resource_usage;           // CPU, memory, I/O metrics
    AnomalyMetrics anomaly_metrics;             // Anomaly scores
    // ... additional fields
};
```

### TaskSecurityEvent
Security-focused event for violations:
```cpp
struct TaskSecurityEvent {
    std::string uuid;
    std::chrono::system_clock::time_point timestamp;
    TaskSecurityEventType event_type;           // RATE_LIMIT_EXCEEDED, etc.
    std::string severity;                       // LOW, MEDIUM, HIGH, CRITICAL
    std::string user_id;
    std::string violation_type;
    std::string description;
    bool blocked;                               // Was action blocked?
    std::string action_taken;
    // ... additional fields
};
```

### TaskAnomalyDetector
Statistical anomaly detection engine:
```cpp
class TaskAnomalyDetector {
    AnomalyMetrics recordExecution(const TaskAuditEvent& event);
    bool hasBaseline(const std::string& task_id);
    std::optional<TaskStatistics> getTaskStatistics(const std::string& task_id);
    // ... configuration and export methods
};
```

### TaskAuditManager
Central management interface:
```cpp
class TaskAuditManager {
    AnomalyMetrics logAuditEvent(const TaskAuditEvent& event);
    void logSecurityEvent(const TaskSecurityEvent& event);
    std::vector<TaskAuditEvent> queryAuditEvents(const AuditQueryParams& params);
    size_t exportAuditEvents(const AuditQueryParams& params, 
                            const std::string& output_path, 
                            ExportFormat format);
    // ... statistics and configuration methods
};
```

## Usage

### Basic Integration

```cpp
#include "scheduler/task_scheduler.h"
#include "utils/audit_logger.h"

// Create audit logger with tamper-evident logging
auto encryption = std::make_shared<FieldEncryption>(...);
auto pki = std::make_shared<VCCPKIClient>(...);
utils::AuditLoggerConfig audit_config;
audit_config.enable_hash_chain = true;
auto audit_logger = std::make_shared<utils::AuditLogger>(encryption, pki, audit_config);

// Create task scheduler with audit logging enabled
TaskScheduler::Config config;
config.enable_audit_logging = true;
config.enable_anomaly_detection = true;
config.enable_gdpr_mode = false;  // Set to true for GDPR compliance

TaskScheduler scheduler(query_engine, config, changefeed, audit_logger);
scheduler.start();
```

### Querying Audit Events

```cpp
// Get audit manager from scheduler
auto audit_manager = scheduler.getAuditManager();

// Query recent failed tasks
scheduler::AuditQueryParams params;
params.start_time = std::chrono::system_clock::now() - std::chrono::hours(24);
params.success = false;  // Only failed tasks
params.limit = 100;
params.sort_by = scheduler::AuditQueryParams::SortBy::TIMESTAMP_DESC;

auto events = audit_manager->queryAuditEvents(params);

// Process events
for (const auto& event : events) {
    std::cout << "Failed task: " << event.task_name 
              << " at " << event.timestamp 
              << " error: " << (event.error_message ? *event.error_message : "unknown")
              << std::endl;
}
```

### Querying Anomalous Events

```cpp
// Query tasks with anomalous behavior
scheduler::AuditQueryParams params;
params.anomalous_only = true;
params.start_time = std::chrono::system_clock::now() - std::chrono::hours(1);
params.sort_by = scheduler::AuditQueryParams::SortBy::ANOMALY_SCORE_DESC;

auto anomalous_events = audit_manager->queryAuditEvents(params);

for (const auto& event : anomalous_events) {
    std::cout << "Anomaly detected in task: " << event.task_name 
              << " score: " << event.anomaly_metrics.overall_score
              << " reason: " << event.anomaly_metrics.description
              << std::endl;
}
```

### Exporting Audit Logs

```cpp
// Export to CEF format for SIEM
scheduler::AuditQueryParams params;
params.start_time = std::chrono::system_clock::now() - std::chrono::hours(24);

size_t count = audit_manager->exportAuditEvents(
    params,
    "/var/log/themis/task_audit_export.cef",
    scheduler::ExportFormat::CEF
);

std::cout << "Exported " << count << " events to CEF format" << std::endl;

// Export to CSV for analysis
count = audit_manager->exportAuditEvents(
    params,
    "/var/log/themis/task_audit_export.csv",
    scheduler::ExportFormat::CSV
);
```

### Monitoring Task Statistics

```cpp
// Get statistics for a specific task
auto stats = audit_manager->getTaskStatistics("my-task-id");
if (stats) {
    std::cout << "Task Statistics:" << std::endl;
    std::cout << "  Total executions: " << stats->total_executions << std::endl;
    std::cout << "  Total failures: " << stats->total_failures << std::endl;
    std::cout << "  Failure rate: " << (stats->failure_rate * 100) << "%" << std::endl;
    std::cout << "  Mean execution time: " << stats->mean_execution_time_ms << "ms" << std::endl;
    std::cout << "  Executions per hour: " << stats->executions_per_hour << std::endl;
}

// Check if task has anomalies
if (audit_manager->hasAnomalies("my-task-id")) {
    std::cout << "Warning: Task exhibits anomalous behavior" << std::endl;
}
```

### Custom Event Callbacks

```cpp
// Set up callbacks for real-time monitoring
TaskAuditConfig config;

// Callback for all audit events
config.on_audit_event = [](const TaskAuditEvent& event) {
    if (!event.success) {
        // Send alert for failed tasks
        sendAlert("Task " + event.task_name + " failed: " + 
                 (event.error_message ? *event.error_message : "unknown"));
    }
};

// Callback for security events
config.on_security_event = [](const TaskSecurityEvent& event) {
    // Log to security monitoring system
    logToSIEM(event.toJson());
    
    if (event.severity == "CRITICAL") {
        // Immediate escalation
        escalateToSecurityTeam(event);
    }
};

// Callback for anomaly detection
config.on_anomaly_detected = [](const std::string& task_id, const AnomalyMetrics& metrics) {
    std::cout << "Anomaly detected in task " << task_id 
              << ": " << metrics.description 
              << " (score: " << metrics.overall_score << ")" << std::endl;
};

auto audit_manager = std::make_shared<TaskAuditManager>(audit_logger, config);
```

## Event Types

### Audit Event Types
- `TASK_REGISTERED` - New task registered
- `TASK_UNREGISTERED` - Task removed
- `TASK_ENABLED` - Task enabled
- `TASK_DISABLED` - Task disabled
- `TASK_UPDATED` - Task configuration updated
- `TASK_STARTED` - Task execution started
- `TASK_COMPLETED` - Task execution completed successfully
- `TASK_FAILED` - Task execution failed
- `TASK_TIMEOUT` - Task execution timed out
- `TASK_RETRY` - Task retry attempt
- `TASK_QUEUED` - Task queued for execution
- `MANUAL_EXECUTION` - Manual task execution triggered
- `CRON_TRIGGERED` - Cron schedule triggered task
- `CDC_TRIGGERED` - CDC event triggered task
- `INTERVAL_TRIGGERED` - Interval timer triggered task

### Security Event Types
- `RATE_LIMIT_EXCEEDED` - Task execution rate limit exceeded
- `RESOURCE_LIMIT_EXCEEDED` - Resource limit exceeded
- `CRON_INJECTION_DETECTED` - Potential cron injection attack
- `AQL_INJECTION_DETECTED` - Potential AQL injection attack
- `UNAUTHORIZED_ACCESS` - Unauthorized task access attempt
- `INVALID_CONFIGURATION` - Invalid or suspicious task configuration
- `EXCESSIVE_FAILURES` - Abnormal failure rate detected
- `ANOMALY_DETECTED` - Behavioral anomaly detected
- `PRIVILEGE_ESCALATION` - Privilege escalation attempt
- `SUSPICIOUS_PATTERN` - Suspicious execution pattern

## Configuration

### Anomaly Detection Configuration
```cpp
AnomalyDetectorConfig config;
config.frequency_threshold = 0.7;       // Alert threshold for frequency score
config.pattern_threshold = 0.7;         // Alert threshold for pattern score
config.resource_threshold = 0.8;        // Alert threshold for resource score
config.failure_rate_threshold = 0.6;    // Alert threshold for failure rate
config.overall_threshold = 0.7;         // Overall anomaly score threshold
config.min_samples = 30;                // Min samples for baseline
config.max_history_size = 1000;         // Max history entries per task
config.baseline_window = std::chrono::hours(24);  // Baseline calculation window
config.frequency_spike_factor = 3.0;    // Spike if frequency > mean * factor
config.resource_spike_factor = 2.5;     // Spike if resource > mean * factor
config.failure_rate_spike = 0.3;        // Alert if failure rate > 30%
```

### Audit Manager Configuration
```cpp
TaskAuditConfig config;
config.enable_audit_logging = true;
config.audit_log_path = "data/logs/task_audit.jsonl";
config.enable_security_logging = true;
config.security_log_path = "data/logs/task_security.jsonl";
config.enable_anomaly_detection = true;
config.anomaly_config = anomaly_detector_config;
config.enable_gdpr_mode = false;  // Enable for GDPR compliance
config.max_query_results = 1000;  // Max results per query
config.enable_export_api = true;
```

## Log File Formats

### Audit Log (JSONL)
```json
{
  "uuid": "550e8400-e29b-41d4-a716-446655440000",
  "timestamp": 1707565200000,
  "task_id": "compress-old-data",
  "task_name": "Compress Old Time Series Data",
  "event_type": "TASK_COMPLETED",
  "trigger_type": "CRON",
  "user_id": "system",
  "ip_address": "localhost",
  "success": true,
  "duration_ms": 1234.56,
  "resource_usage": {
    "cpu_time_ms": 1200.0,
    "memory_bytes": 52428800,
    "execution_time_ms": 1234.56,
    "result_rows": 150000,
    "affected_rows": 150000
  },
  "anomaly_metrics": {
    "frequency_score": 0.1,
    "pattern_score": 0.05,
    "resource_score": 0.2,
    "failure_rate_score": 0.0,
    "overall_score": 0.0875,
    "is_anomalous": false
  }
}
```

### Security Log (JSONL)
```json
{
  "uuid": "650e8400-e29b-41d4-a716-446655440001",
  "timestamp": 1707565300000,
  "task_id": "suspicious-task",
  "event_type": "RATE_LIMIT_EXCEEDED",
  "severity": "MEDIUM",
  "user_id": "attacker",
  "ip_address": "192.168.1.100",
  "violation_type": "manual_execution_rate_limit",
  "description": "Task execution rate limit exceeded",
  "details": {
    "executions_in_window": 15,
    "max_executions": 10,
    "window_minutes": 1
  },
  "blocked": true,
  "action_taken": "execution_denied"
}
```

### CEF Format
```
CEF:0|ThemisDB|TaskScheduler|1.5.0|TASK_COMPLETED|Task Execution Event|3|taskId=compress-old-data taskName=Compress Old Time Series Data triggerType=CRON outcome=success durationMs=1234.56 cpuTimeMs=1200.0 memoryBytes=52428800
```

## Security Considerations

### Tamper-Evident Logging
- All events are logged through the existing AuditLogger with hash-chain verification
- Each event's hash includes the previous event's hash, creating an immutable chain
- Any tampering with historical events will break the chain and be detected

### GDPR Compliance
- Enable `enable_gdpr_mode` to automatically mask sensitive personal data
- User IDs are partially masked (e.g., "us***er")
- IP addresses are partially masked (e.g., "19***00")
- Session IDs are hashed using SHA-256
- Full data available for authorized security investigations

### Data Retention
- Configure retention policies via existing AuditLogger mechanisms
- Archive old events to cold storage
- Purge events after retention period expires
- Maintain compliance with data protection regulations

## Performance Impact

- **Memory**: ~1KB per cached audit event (max 10,000 cached)
- **CPU**: Minimal (<1% overhead for audit logging)
- **I/O**: Asynchronous log writing to minimize impact
- **Storage**: ~500 bytes per audit event in JSONL format
- **Anomaly Detection**: Statistical calculations performed in O(n) time where n is history size

## Testing

Comprehensive test suite included:
- UUID generation and uniqueness
- Data masking (full, partial, hash)
- Event serialization (JSON, CEF, Splunk, Elastic)
- Anomaly detection (frequency, pattern, resource, failure rate)
- Audit manager integration
- Security event logging
- Query and export functionality

Run tests:
```bash
./test_task_audit
```

## Future Enhancements

1. **Machine Learning**: ML-based anomaly detection using historical patterns
2. **Real-time Alerting**: Integration with notification services (email, Slack, PagerDuty)
3. **Dashboard**: Web-based dashboard for audit event visualization
4. **Advanced Queries**: Support for complex query filters and aggregations
5. **Distributed Tracing**: Integration with OpenTelemetry for distributed task execution
6. **Automated Response**: Automatic task disabling on security violations
7. **Compliance Reports**: Pre-built compliance report templates (ISO 27001, SOC2, GDPR)

## References

- ISO 27001: Information Security Management
- SOC2: Service Organization Control 2
- GDPR: General Data Protection Regulation
- CEF: Common Event Format (ArcSight)
- ECS: Elastic Common Schema
- Splunk HEC: HTTP Event Collector

## Support

For questions or issues, please refer to:
- ThemisDB Documentation: https://makr-code.github.io/ThemisDB/
- GitHub Issues: https://github.com/makr-code/ThemisDB/issues
- Security Contact: security@themisdb.io
