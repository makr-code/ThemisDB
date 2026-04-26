# Logging Configuration Guide

**Version:** 1.5.0  
**Last Updated:** 2026-04-06  
**Addresses:** FIND-023 - Centralize and Standardize Logging Configuration  
**Standards:** ISO 27001 A.12.4, BSI C5 LOG-01 to LOG-03

---

## Overview

This document describes the centralized logging configuration for ThemisDB. It provides a unified approach to logging across all components, ensuring consistency, compliance, and operational efficiency.

---

## Logging Architecture

### Centralized Logging Stack

**Components:**
- **Log Collection:** Filebeat / Fluentd
- **Log Processing:** Logstash / Vector
- **Log Storage:** Elasticsearch / OpenSearch
- **Log Visualization:** Kibana / Grafana
- **Log Archive:** S3-compatible object storage

**Architecture:**
```
Application → Local Logs → Log Collector → Log Processor → Storage → Visualization
                    ↓
              Archive (S3)
```

---

## Log Categories

### 1. Application Logs

**Purpose:** Application behavior, errors, warnings

**Location:** `/var/log/themisdb/app.log`

**Format:** JSON structured logging

**Example:**
```json
{
  "timestamp": "2026-02-03T10:00:00.000Z",
  "level": "INFO",
  "component": "query_engine",
  "message": "Query executed successfully",
  "query_id": "q_1234567890",
  "duration_ms": 45,
  "user": "user@example.com",
  "trace_id": "trace_abc123"
}
```

**Retention:** 90 days

### 2. Audit Logs

**Purpose:** Security events, access attempts, data modifications

**Location:** `/var/log/themisdb/audit.log`

**Format:** JSON structured with cryptographic signatures

**Example:**
```json
{
  "timestamp": "2026-02-03T10:00:00.000Z",
  "event_type": "user_login",
  "user": "admin@example.com",
  "source_ip": "192.168.1.100",
  "result": "success",
  "mfa_verified": true,
  "session_id": "sess_xyz789",
  "signature": "sha256:abcd1234..."
}
```

**Retention:** 7 years (compliance requirement)

### 3. Security Logs

**Purpose:** Security events, threats, vulnerabilities

**Location:** `/var/log/themisdb/security.log`

**Format:** JSON structured with threat intelligence

**Example:**
```json
{
  "timestamp": "2026-02-03T10:00:00.000Z",
  "event_type": "authentication_failure",
  "user": "attacker@malicious.com",
  "source_ip": "203.0.113.1",
  "failure_count": 5,
  "threat_level": "medium",
  "blocked": true,
  "rule_triggered": "brute_force_detection"
}
```

**Retention:** 2 years

### 4. Performance Logs

**Purpose:** Performance metrics, slow queries, resource usage

**Location:** `/var/log/themisdb/performance.log`

**Format:** JSON structured with metrics

**Example:**
```json
{
  "timestamp": "2026-02-03T10:00:00.000Z",
  "event_type": "slow_query",
  "query": "SELECT * FROM large_table WHERE...",
  "duration_ms": 5234,
  "threshold_ms": 1000,
  "cpu_usage": 78.5,
  "memory_mb": 1024,
  "user": "user@example.com"
}
```

**Retention:** 30 days

### 5. System Logs

**Purpose:** System events, service status, infrastructure

**Location:** `/var/log/themisdb/system.log`

**Format:** Syslog format

**Example:**
```
Feb  3 10:00:00 themisdb systemd[1]: Started ThemisDB Service
Feb  3 10:00:01 themisdb themisdb-server: Service initialization complete
Feb  3 10:00:02 themisdb themisdb-server: Listening on port 9090
```

**Retention:** 90 days

---

## Centralized Configuration

### Configuration Template

**File:** `config/logging.yaml`

```yaml
logging:
  # Global settings
  global:
    format: "json"                    # json | text
    level: "info"                     # debug | info | warn | error
    timestamp_format: "iso8601"
    include_caller: true
    include_stacktrace_on_error: true
  
  # Output configuration
  outputs:
    # Local file output
    file:
      enabled: true
      path: "/var/log/themisdb"
      rotation:
        enabled: true
        max_size_mb: 100
        max_files: 10
        compress: true
    
    # Centralized logging
    remote:
      enabled: true
      type: "elasticsearch"          # elasticsearch | splunk | loki
      endpoint: "https://logs.example.com:9200"
      index_pattern: "themisdb-%{+YYYY.MM.dd}"
      buffer_size: 1000
      flush_interval_seconds: 10
    
    # Console output (development)
    console:
      enabled: false
      format: "text"
      color: true
  
  # Log categories
  categories:
    application:
      enabled: true
      level: "info"
      file: "app.log"
      retention_days: 90
      
    audit:
      enabled: true
      level: "info"
      file: "audit.log"
      retention_days: 2555  # 7 years
      immutable: true
      cryptographic_signature: true
      
    security:
      enabled: true
      level: "warn"
      file: "security.log"
      retention_days: 730  # 2 years
      alert_on_critical: true
      
    performance:
      enabled: true
      level: "info"
      file: "performance.log"
      retention_days: 30
      slow_query_threshold_ms: 1000
      
    system:
      enabled: true
      level: "info"
      file: "system.log"
      retention_days: 90
  
  # Filtering
  filters:
    # Exclude health check logs
    - type: "exclude"
      field: "path"
      pattern: "/health"
    
    # Redact sensitive data
    - type: "redact"
      fields: ["password", "token", "api_key", "credit_card"]
  
  # Sampling (for high-volume logs)
  sampling:
    enabled: false
    rate: 0.1  # Sample 10% of logs
    exclude_errors: true  # Always log errors
```

### Environment-Specific Configuration

**Development:**
```yaml
# config/logging.dev.yaml
logging:
  global:
    level: "debug"
  outputs:
    console:
      enabled: true
    remote:
      enabled: false
```

**Production:**
```yaml
# config/logging.prod.yaml
logging:
  global:
    level: "info"
  outputs:
    file:
      enabled: true
    remote:
      enabled: true
    console:
      enabled: false
```

---

## Log Rotation

### Automatic Rotation

**Logrotate Configuration:** `/etc/logrotate.d/themisdb`

```bash
/var/log/themisdb/*.log {
    daily
    rotate 90
    compress
    delaycompress
    notifempty
    create 0640 themisdb themisdb
    sharedscripts
    postrotate
        systemctl reload themisdb >/dev/null 2>&1 || true
    endscript
}

# Audit logs - special handling (7 years retention)
/var/log/themisdb/audit.log {
    daily
    rotate 2555
    compress
    delaycompress
    notifempty
    create 0640 themisdb themisdb
    sharedscripts
    postrotate
        # Archive to S3 for long-term storage
        /usr/local/bin/archive-audit-logs.sh
    endscript
}
```

---

## Logging Best Practices

### Structured Logging

**Use JSON format for all logs:**
```json
{
  "timestamp": "ISO8601",
  "level": "INFO|WARN|ERROR",
  "component": "component_name",
  "message": "Human-readable message",
  "context": {
    "key": "value"
  }
}
```

### Log Levels

**DEBUG:** Detailed diagnostic information
- Use: Development and troubleshooting
- Example: Variable values, function entry/exit

**INFO:** General informational messages
- Use: Normal operation events
- Example: Service started, request completed

**WARN:** Warning messages (potential issues)
- Use: Recoverable errors, deprecated features
- Example: Slow query, high memory usage

**ERROR:** Error messages (failures)
- Use: Operation failures, exceptions
- Example: Database connection failed, query error

**CRITICAL:** Critical failures
- Use: System failures, data corruption
- Example: Service crash, data integrity violation

### What to Log

**DO Log:**
- ✅ Authentication attempts (success and failure)
- ✅ Authorization decisions
- ✅ Data access (sensitive data)
- ✅ Configuration changes
- ✅ Administrative actions
- ✅ Errors and exceptions
- ✅ Security events
- ✅ Performance issues

**DO NOT Log:**
- ❌ Passwords or secrets
- ❌ Full credit card numbers
- ❌ Personal health information (PHI)
- ❌ Session tokens
- ❌ Encryption keys

### Data Redaction

**Automatic redaction of sensitive fields:**
```json
{
  "user": "john.doe",
  "password": "[REDACTED]",
  "credit_card": "****-****-****-1234",
  "api_key": "[REDACTED]"
}
```

---

## Monitoring & Alerting

### Log-Based Alerts

**Critical Alerts:**
```yaml
# High error rate
alert: HighErrorRate
expr: rate(log_messages{level="ERROR"}[5m]) > 10
for: 5m
annotations:
  summary: "High error rate detected"

# Authentication failures
alert: AuthenticationFailures
expr: rate(log_messages{event_type="auth_failure"}[5m]) > 5
for: 1m
annotations:
  summary: "Multiple authentication failures"

# Security events
alert: SecurityEvent
expr: log_messages{level="CRITICAL", category="security"} > 0
for: 0m
annotations:
  summary: "Critical security event detected"
```

### Log Dashboards

**Grafana Dashboards:**
1. **Application Health**
   - Error rate
   - Warning rate
   - Log volume

2. **Security Overview**
   - Authentication failures
   - Authorization denials
   - Security events

3. **Performance Monitoring**
   - Slow queries
   - Resource usage
   - Request latency

4. **Audit Trail**
   - User actions
   - Data modifications
   - Configuration changes

---

## Compliance Requirements

### ISO 27001 A.12.4 - Logging and Monitoring

**Requirements:**
- ✅ Event logs recording user activities
- ✅ Exceptions and security events logged
- ✅ Administrator and operator activities logged
- ✅ Logging facilities protected against tampering
- ✅ Log retention according to policy

### BSI C5 Logging Requirements

**LOG-01: Logging Configuration**
- ✅ Centralized logging configuration
- ✅ Standardized log format
- ✅ Appropriate retention periods

**LOG-02: Log Protection**
- ✅ Logs protected from unauthorized access
- ✅ Cryptographic integrity (audit logs)
- ✅ Immutable audit trail

**LOG-03: Log Analysis**
- ✅ Regular log review
- ✅ Automated analysis
- ✅ Security event detection

### GDPR Compliance

**Article 32 - Security of Processing:**
- ✅ Ability to ensure confidentiality
- ✅ Ability to restore availability
- ✅ Regular testing and evaluation
- ✅ Logging of processing activities

---

## Configuration Management

### Configuration Validation Script

**Script:** `scripts/operations/logging-config.sh`

```bash
#!/bin/bash
# Logging configuration management script

# Validate configuration
validate_config() {
    themisdb-admin config validate --config logging.yaml
}

# Apply configuration
apply_config() {
    themisdb-admin config apply --config logging.yaml --restart-required
}

# Test logging
test_logging() {
    themisdb-admin logging test --all-categories
}

# Export configuration
export_config() {
    themisdb-admin config export --format yaml > logging-backup.yaml
}

# Usage
case "$1" in
    validate)
        validate_config
        ;;
    apply)
        apply_config
        ;;
    test)
        test_logging
        ;;
    export)
        export_config
        ;;
    *)
        echo "Usage: $0 {validate|apply|test|export}"
        exit 1
        ;;
esac
```

---

## Troubleshooting

### Common Issues

**Issue:** Logs not appearing in centralized system
```bash
# Check log collector status
systemctl status filebeat

# Verify connectivity
curl -X GET https://logs.example.com:9200/_cluster/health

# Check log collector logs
tail -f /var/log/filebeat/filebeat
```

**Issue:** Disk space filling up with logs
```bash
# Check log directory size
du -sh /var/log/themisdb/

# Manually trigger log rotation
logrotate -f /etc/logrotate.d/themisdb

# Adjust retention in config
vim config/logging.yaml
```

**Issue:** Missing logs from specific component
```bash
# Check component log level
themisdb-admin logging get-level --component query_engine

# Temporarily increase log level
themisdb-admin logging set-level --component query_engine --level debug

# Verify logs are being written
tail -f /var/log/themisdb/app.log | grep query_engine
```

---

## Related Documentation

- [Operations Handbook](../OPERATIONS_HANDBOOK.md)
- [Monitoring Setup Guide](../MONITORING_SETUP_GUIDE.md)
- [Security Deployment Guide](../../security/SECURITY_DEPLOYMENT_GUIDE.md)
- [Audit Findings Report](../../../audit-reports/v1.4.1/FINDINGS_AND_RISKS.md)

---

**Document Version:** 1.5.0  
**Compliance:** ISO 27001 A.12.4, BSI C5 LOG-01 to LOG-03  
**Last Reviewed:** 2026-02-03
