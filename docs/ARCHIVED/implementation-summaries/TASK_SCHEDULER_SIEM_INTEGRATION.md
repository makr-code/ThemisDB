# Task Scheduler SIEM Integration Guide

**Version:** 1.5.0  
**Last Updated:** 2026-04-06  
**Target Audience:** Security Engineers, DevOps, System Administrators

---

## Table of Contents

1. [Overview](#overview)
2. [Features](#features)
3. [Architecture](#architecture)
4. [Configuration](#configuration)
5. [Event Types](#event-types)
6. [SIEM Integration Examples](#siem-integration-examples)
7. [Anomaly Detection](#anomaly-detection)
8. [Compliance & Security](#compliance--security)
9. [Troubleshooting](#troubleshooting)

---

## Overview

ThemisDB's TaskScheduler now provides comprehensive SIEM (Security Information and Event Management) integration, enabling real-time monitoring, threat detection, and compliance reporting for scheduled tasks, cron jobs, and CDC (Change Data Capture) events.

### Key Benefits

- **Real-time Security Monitoring**: Track all task operations with detailed audit logs
- **Anomaly Detection**: Automatically detect unusual task behavior (execution time, frequency, timing)
- **Compliance**: GDPR Article 32 and ISO 27001 compliant audit logging
- **Multi-Format Support**: JSON, CEF (Common Event Format), and Syslog formats
- **SIEM Compatibility**: Works with Splunk, Elastic SIEM, syslog-ng, and more

---

## Features

### Audit Logging Capabilities

1. **Task Lifecycle Events**
   - Task registration/unregistration
   - Task enable/disable operations
   - Task configuration updates

2. **Execution Events**
   - Task execution (success/failure)
   - Cron trigger activations
   - CDC event trigger activations
   - Manual task executions

3. **Security Events**
   - Timeout detection
   - Resource limit exceeded
   - Anomaly detection alerts

4. **Rich Event Context**
   - Timestamp (ISO 8601)
   - Event type
   - Task ID and name
   - User/service account
   - Source IP (if available)
   - Execution metrics (time, resource usage)
   - Anomaly score
   - Error messages (on failure)

---

## Architecture

```
┌─────────────────┐
│  TaskScheduler  │
│                 │
│  ┌──────────┐  │      ┌──────────────┐
│  │  Cron    │  │─────▶│ AuditLogger  │
│  │ Triggers │  │      └──────┬───────┘
│  └──────────┘  │             │
│                 │             │   ┌──────────────┐
│  ┌──────────┐  │             ├──▶│ JSON Format  │
│  │   CDC    │  │             │   └──────────────┘
│  │ Triggers │  │             │
│  └──────────┘  │             │   ┌──────────────┐
│                 │             ├──▶│  CEF Format  │
│  ┌──────────┐  │             │   └──────────────┘
│  │ Manual   │  │             │
│  │Execution │  │             │   ┌──────────────┐
│  └──────────┘  │             └──▶│Syslog Format │
└─────────────────┘                 └──────┬───────┘
                                           │
                                           ▼
                          ┌────────────────────────────┐
                          │   SIEM Systems             │
                          │                            │
                          │  • Splunk                  │
                          │  • Elastic SIEM / ELK      │
                          │  • Syslog-ng / rsyslog     │
                          │  • IBM QRadar              │
                          │  • ArcSight                │
                          └────────────────────────────┘
```

---

## Configuration

### Basic Configuration

Edit `config/audit.yaml`:

```yaml
# Enable audit logging
audit_logging:
  enabled: true
  log_path: "data/logs/audit.jsonl"

# Enable SIEM integration
siem_integration:
  enable_siem: true
  siem_type: "syslog"      # Options: syslog, splunk, elastic
  siem_format: "json"       # Options: json, cef, syslog
  siem_host: "siem.example.com"
  siem_port: 514

# Task scheduler audit
task_scheduler_audit:
  enabled: true
  log_task_registration: true
  log_task_execution: true
  log_cron_triggers: true
  log_cdc_triggers: true
  include_resource_metrics: true

# Anomaly detection
anomaly_detection:
  enabled: true
  threshold: 2.0            # Standard deviations
  min_baseline_samples: 10
```

### Advanced Configuration

```yaml
# GDPR compliance
compliance:
  gdpr:
    enabled: true
    retention_days: 365
    auto_archive: true
    pseudonymize_users: false

# Authorization
authorization:
  siem_config_roles:
    - "system_admin"
    - "security_admin"
  
  require_mfa:
    siem_config_changes: true
    audit_log_export: true
```

---

## Event Types

### Task Scheduler Event Types

| Event Type | Description | Severity |
|------------|-------------|----------|
| `TASK_REGISTERED` | Task registered in scheduler | LOW |
| `TASK_UNREGISTERED` | Task removed from scheduler | MEDIUM |
| `TASK_ENABLED` | Task enabled for execution | LOW |
| `TASK_DISABLED` | Task disabled | MEDIUM |
| `TASK_UPDATED` | Task configuration updated | LOW |
| `TASK_EXECUTED_SUCCESS` | Task executed successfully | LOW |
| `TASK_EXECUTED_FAILURE` | Task execution failed | HIGH |
| `TASK_CRON_TRIGGERED` | Cron schedule triggered task | LOW |
| `TASK_CDC_TRIGGERED` | CDC event triggered task | LOW |
| `TASK_MANUAL_TRIGGERED` | Manual execution initiated | MEDIUM |
| `TASK_TIMEOUT` | Task exceeded timeout limit | HIGH |
| `TASK_RESOURCE_LIMIT_EXCEEDED` | Resource limits exceeded | HIGH |
| `TASK_ANOMALY_DETECTED` | Anomalous behavior detected | HIGH |

### Event Field Reference

#### Common Fields (All Events)

```json
{
  "timestamp": 1707561600000,
  "event_type": "TASK_EXECUTED_SUCCESS",
  "category": "TASK_SCHEDULER",
  "task_id": "backup-task-001",
  "task_name": "Daily Backup",
  "user_id": "system",
  "severity": "LOW"
}
```

#### Execution Success Event

```json
{
  "timestamp": 1707561600000,
  "event_type": "TASK_EXECUTED_SUCCESS",
  "task_id": "backup-task-001",
  "task_name": "Daily Backup",
  "trigger_type": 0,
  "execution_time_ms": 1234.56,
  "total_executions": 150,
  "avg_execution_time_ms": 1180.23,
  "anomaly_score": 0.45,
  "success": true,
  "severity": "LOW"
}
```

#### Execution Failure Event

```json
{
  "timestamp": 1707561600000,
  "event_type": "TASK_EXECUTED_FAILURE",
  "task_id": "data-sync-task",
  "task_name": "Data Synchronization",
  "trigger_type": 1,
  "execution_time_ms": 5678.90,
  "error_message": "Connection timeout to remote server",
  "failed_executions": 3,
  "success": false,
  "severity": "HIGH"
}
```

#### Cron Trigger Event

```json
{
  "timestamp": 1707561600000,
  "event_type": "TASK_CRON_TRIGGERED",
  "task_id": "report-generation",
  "task_name": "Weekly Report Generation",
  "trigger_type": "CRON",
  "cron_expression": "0 9 * * 1",
  "severity": "LOW"
}
```

#### CDC Trigger Event

```json
{
  "timestamp": 1707561600000,
  "event_type": "TASK_CDC_TRIGGERED",
  "task_id": "order-processor",
  "task_name": "Order Processing",
  "trigger_type": "CDC_EVENT",
  "cdc_key": "orders:12345",
  "cdc_event_type": 0,
  "cdc_key_prefix": "orders:",
  "severity": "LOW"
}
```

#### Anomaly Detection Event

```json
{
  "timestamp": 1707561600000,
  "event_type": "TASK_ANOMALY_DETECTED",
  "task_id": "data-import",
  "task_name": "Data Import Job",
  "execution_time_ms": 15234.56,
  "anomaly_score": 3.8,
  "threshold": 2.0,
  "details": "Execution time significantly exceeds baseline",
  "severity": "HIGH"
}
```

---

## SIEM Integration Examples

### Splunk Integration

#### Configuration

```yaml
siem_integration:
  enable_siem: true
  siem_type: "splunk"
  siem_format: "json"
  splunk:
    hec_token: "your-splunk-hec-token-here"
    hec_url: "https://splunk.example.com:8088/services/collector/event"
    verify_ssl: true
```

#### Splunk Search Queries

**Failed task executions in the last hour:**
```spl
index=themisdb_audit sourcetype=themisdb:task_scheduler event_type="TASK_EXECUTED_FAILURE"
| stats count by task_id, task_name, error_message
| sort -count
```

**Anomaly detection alerts:**
```spl
index=themisdb_audit sourcetype=themisdb:task_scheduler event_type="TASK_ANOMALY_DETECTED"
| where anomaly_score > 2.0
| table _time, task_id, task_name, anomaly_score, execution_time_ms
```

**Task execution performance dashboard:**
```spl
index=themisdb_audit sourcetype=themisdb:task_scheduler event_type="TASK_EXECUTED_SUCCESS"
| stats avg(execution_time_ms) as avg_time, max(execution_time_ms) as max_time, count by task_name
| sort -avg_time
```

### Elastic Stack (ELK) Integration

#### Configuration

```yaml
siem_integration:
  enable_siem: true
  siem_type: "elastic"
  siem_format: "json"
  siem_host: "elasticsearch.example.com"
  siem_port: 9200
  elasticsearch:
    index: "themisdb-audit"
    index_rotation: "daily"
    username: "themisdb"
    password: "your-password"
```

#### Logstash Configuration

```ruby
input {
  file {
    path => "/var/log/themisdb/audit.jsonl"
    codec => "json"
    type => "themisdb_audit"
  }
}

filter {
  if [category] == "TASK_SCHEDULER" {
    mutate {
      add_field => { "[@metadata][target_index]" => "themisdb-task-scheduler-%{+YYYY.MM.dd}" }
    }
  }
}

output {
  elasticsearch {
    hosts => ["localhost:9200"]
    index => "%{[@metadata][target_index]}"
    user => "themisdb"
    password => "${ELASTIC_PASSWORD}"
  }
}
```

#### Kibana Queries

**Failed tasks visualization:**
```json
{
  "query": {
    "bool": {
      "must": [
        { "term": { "event_type": "TASK_EXECUTED_FAILURE" } },
        { "range": { "timestamp": { "gte": "now-1h" } } }
      ]
    }
  },
  "aggs": {
    "failures_by_task": {
      "terms": { "field": "task_name.keyword" }
    }
  }
}
```

### Syslog-ng Integration

#### Configuration

```yaml
siem_integration:
  enable_siem: true
  siem_type: "syslog"
  siem_format: "cef"          # CEF format works well with syslog
  siem_host: "syslog.example.com"
  siem_port: 514
```

#### Syslog-ng Server Configuration

```
source s_themisdb {
    udp(
        ip(0.0.0.0)
        port(514)
    );
};

filter f_themisdb_task_scheduler {
    message("themisdb" type(string) flags(prefix));
    message("TASK_" type(string) flags(substring));
};

destination d_themisdb_tasks {
    file("/var/log/themisdb/task_scheduler.log"
         template("${ISODATE} ${HOST} ${MESSAGE}\n")
    );
};

log {
    source(s_themisdb);
    filter(f_themisdb_task_scheduler);
    destination(d_themisdb_tasks);
};
```

#### CEF Format Example

ThemisDB generates CEF-compliant messages:

```
CEF:0|ThemisDB|TaskScheduler|1.5.0|TASK_EXECUTED_FAILURE|Task Executed Failure|8|taskId=backup-task-001 suser=system rt=1707561600000 executionTime=5678.90 msg=Connection timeout to remote server
```

---

## Anomaly Detection

### How It Works

ThemisDB uses statistical analysis to detect anomalous task behavior:

1. **Baseline Learning**: First 10 executions establish a baseline
2. **Metrics Tracked**:
   - Execution time
   - Execution frequency
   - Resource consumption
3. **Z-Score Calculation**: Compares current execution to baseline
4. **Threshold**: Configurable (default: 2.0 standard deviations)

### Anomaly Types

#### Execution Time Anomaly

Task takes significantly longer than usual:

```
Normal: ~1000ms ± 100ms
Anomalous: 5000ms
Anomaly Score: 3.8 (exceeds threshold of 2.0)
```

#### Frequency Anomaly

Task runs much more or less frequently than expected:

```
Normal: Every 5 minutes ± 30 seconds
Anomalous: Every 30 seconds
Anomaly Score: 4.2
```

#### Resource Anomaly

Task consumes excessive CPU, memory, or I/O:

```
Normal: 2% CPU, 100MB memory
Anomalous: 80% CPU, 2GB memory
Anomaly Score: 5.1
```

### Configuration

```yaml
anomaly_detection:
  enabled: true
  threshold: 2.0              # Alert when score > 2.0
  min_baseline_samples: 10    # Samples needed for baseline
  
  dimensions:
    execution_time: true
    execution_frequency: true
    execution_timing: true
    resource_usage: true
  
  actions:
    log_event: true
    auto_disable_task: false   # Auto-disable on anomaly
    send_siem_alert: true      # High-priority SIEM alert
```

### Responding to Anomalies

When an anomaly is detected:

1. **Review Logs**: Check audit logs for anomaly details
2. **Investigate Cause**: Determine if anomaly is:
   - Expected (e.g., large dataset)
   - Infrastructure issue (slow network, high load)
   - Security incident (malicious activity)
3. **Take Action**:
   - Adjust task configuration if expected
   - Investigate security if suspicious
   - Tune anomaly threshold if false positive

---

## Compliance & Security

### GDPR Compliance (Article 32)

ThemisDB's audit logging meets GDPR requirements:

- ✅ **Security of Processing**: Encrypted audit logs
- ✅ **Integrity and Confidentiality**: Hash chain tamper protection
- ✅ **Data Retention**: Configurable retention with auto-archiving
- ✅ **Access Controls**: Role-based access to audit logs

### ISO 27001 Compliance

- ✅ **A.12.4.1 Event Logging**: Comprehensive audit trail
- ✅ **A.12.4.2 Protection of Log Information**: Encrypted storage
- ✅ **A.12.4.3 Administrator Logs**: All privileged operations logged
- ✅ **A.12.4.4 Clock Synchronization**: Timestamps in UTC

### Security Best Practices

1. **Enable Encryption**: Always use `encrypt_then_sign: true`
2. **Hash Chain**: Enable `enable_hash_chain: true` for tamper detection
3. **Access Control**: Restrict audit log access to authorized users only
4. **Regular Review**: Review audit logs regularly for suspicious activity
5. **Secure SIEM**: Use TLS/SSL for SIEM connections
6. **Backup Logs**: Regularly backup and archive audit logs

### Authorization Controls

```yaml
authorization:
  # Only these roles can modify SIEM settings
  siem_config_roles:
    - "system_admin"
    - "security_admin"
  
  # Only these roles can view audit logs
  audit_view_roles:
    - "system_admin"
    - "security_admin"
    - "compliance_officer"
    - "auditor"
  
  # Require MFA for sensitive operations
  require_mfa:
    siem_config_changes: true
    audit_log_export: true
```

---

## Troubleshooting

### Common Issues

#### 1. No Audit Logs Generated

**Symptom**: Audit log file is empty or not created

**Solutions**:
```yaml
# Check configuration
audit_logging:
  enabled: true  # Must be true
  
task_scheduler_audit:
  enabled: true  # Must be true

# Verify TaskScheduler has audit logger
TaskScheduler scheduler(
    query_engine,
    config,
    changefeed,
    audit_logger  // Must not be nullptr
);
```

#### 2. SIEM Not Receiving Events

**Symptom**: Events logged locally but not in SIEM

**Solutions**:
- Check network connectivity to SIEM server
- Verify SIEM credentials (Splunk HEC token, Elastic auth)
- Check firewall rules (syslog port 514, Splunk port 8088)
- Enable debug logging:
  ```yaml
  operational:
    async_siem_forwarding: false  # Synchronous for debugging
  ```

#### 3. High Anomaly False Positives

**Symptom**: Many anomaly alerts for normal behavior

**Solutions**:
- Increase threshold:
  ```yaml
  anomaly_detection:
    threshold: 3.0  # Default is 2.0
  ```
- Increase baseline samples:
  ```yaml
  anomaly_detection:
    min_baseline_samples: 20  # Default is 10
  ```

#### 4. Audit Log Growing Too Large

**Symptom**: Disk space issues from large audit logs

**Solutions**:
- Enable log rotation:
  ```yaml
  operational:
    max_log_file_size_mb: 100
    max_rotated_files: 10
  ```
- Enable auto-archiving:
  ```yaml
  compliance:
    gdpr:
      auto_archive: true
      retention_days: 365
  ```

### Debug Logging

Enable verbose logging:

```yaml
logging:
  level: "debug"
  categories:
    - "audit"
    - "task_scheduler"
    - "siem"
```

### Verifying Hash Chain Integrity

Programmatically check audit log integrity:

```cpp
bool integrity_ok = audit_logger->verifyChainIntegrity();
if (!integrity_ok) {
    // Possible tampering detected!
    THEMIS_ALERT("Audit log integrity check FAILED");
}
```

---

## Support and Resources

- **GitHub Repository**: https://github.com/makr-code/ThemisDB
- **Documentation**: https://makr-code.github.io/ThemisDB/
- **Issue Tracker**: https://github.com/makr-code/ThemisDB/issues
- **Configuration Examples**: `config/audit.yaml`

---

**Document Version**: 1.0  
**Last Review**: 2026-02-10  
**Owner**: Security Team
