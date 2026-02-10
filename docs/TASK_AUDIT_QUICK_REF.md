# Task Scheduler Audit Events - Quick Reference

## Enable Audit Logging

```cpp
// Create TaskScheduler with audit logging
TaskScheduler::Config config;
config.enable_audit_logging = true;
config.enable_anomaly_detection = true;
config.enable_gdpr_mode = false;  // true for GDPR compliance

TaskScheduler scheduler(query_engine, config, changefeed, audit_logger);
```

## Query Recent Failures

```cpp
auto audit_manager = scheduler.getAuditManager();

scheduler::AuditQueryParams params;
params.start_time = std::chrono::system_clock::now() - std::chrono::hours(24);
params.success = false;
params.limit = 100;

auto events = audit_manager->queryAuditEvents(params);
```

## Query Anomalous Tasks

```cpp
scheduler::AuditQueryParams params;
params.anomalous_only = true;
params.sort_by = scheduler::AuditQueryParams::SortBy::ANOMALY_SCORE_DESC;

auto events = audit_manager->queryAuditEvents(params);
```

## Export to CEF/SIEM

```cpp
audit_manager->exportAuditEvents(
    params,
    "/var/log/themis/audit.cef",
    scheduler::ExportFormat::CEF
);
```

## Monitor Task Statistics

```cpp
auto stats = audit_manager->getTaskStatistics("task-id");
if (stats) {
    std::cout << "Executions: " << stats->total_executions << std::endl;
    std::cout << "Failures: " << stats->total_failures << std::endl;
    std::cout << "Failure rate: " << (stats->failure_rate * 100) << "%" << std::endl;
}
```

## Event Callbacks

```cpp
TaskAuditConfig config;
config.on_audit_event = [](const TaskAuditEvent& event) {
    // Handle audit events
};
config.on_security_event = [](const TaskSecurityEvent& event) {
    // Handle security violations
};
config.on_anomaly_detected = [](const std::string& task_id, const AnomalyMetrics& metrics) {
    // Handle anomalies
};
```

## Key Features

✅ **Comprehensive Logging**: Every task execution logged with full context
✅ **Security Events**: Separate stream for violations and attacks
✅ **Anomaly Detection**: 4 dimensions (frequency, pattern, resource, failures)
✅ **GDPR Compliant**: Automatic data masking
✅ **Tamper-Evident**: Hash chain integration
✅ **SIEM Ready**: CEF, Splunk, Elastic formats
✅ **Query & Export**: Flexible filtering and multiple formats

## Log Locations

- Audit events: `data/logs/task_audit.jsonl`
- Security events: `data/logs/task_security.jsonl`
- Main audit log: `data/logs/audit.jsonl` (tamper-evident)

## Event Types

**Audit**: TASK_REGISTERED, TASK_STARTED, TASK_COMPLETED, TASK_FAILED, MANUAL_EXECUTION, CRON_TRIGGERED, CDC_TRIGGERED

**Security**: RATE_LIMIT_EXCEEDED, RESOURCE_LIMIT_EXCEEDED, AQL_INJECTION_DETECTED, EXCESSIVE_FAILURES, ANOMALY_DETECTED

## Anomaly Scores

- **0.0-0.3**: Normal behavior
- **0.3-0.7**: Minor deviation
- **0.7-1.0**: Anomalous (alert triggered)

Combines:
- Frequency score (execution rate)
- Pattern score (timing regularity)
- Resource score (CPU/memory usage)
- Failure rate score (error frequency)
