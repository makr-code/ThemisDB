# SIEM Integration Guide for ThemisDB

**Version:** 1.4.0  
**Last Updated:** 2026-04-06  
**Target Audience:** Security Operations Center (SOC) Engineers, SIEM Administrators, Security Architects, Compliance Officers

---

## Table of Contents

1. [Overview](#overview)
2. [SIEM-Relevant Metrics](#siem-relevant-metrics)
3. [Dashboard Setup](#dashboard-setup)
4. [Alert Configuration](#alert-configuration)
5. [Integration Examples](#integration-examples)
6. [Compliance Mapping](#compliance-mapping)
7. [Export Formats](#export-formats)
8. [Best Practices](#best-practices)

---

## Overview

ThemisDB provides comprehensive security monitoring capabilities designed for Security Information and Event Management (SIEM) systems. This guide covers the integration of ThemisDB metrics into your SIEM infrastructure for real-time security monitoring, threat detection, and compliance reporting.

### Key Capabilities

- **Real-time Security Monitoring**: Continuous monitoring of authentication, authorization, and audit events
- **Anomaly Detection**: Baseline deviations and unusual patterns detection
- **Compliance Reporting**: SOC2, GDPR, HIPAA-aligned metrics and alerts
- **Threat Detection**: Brute force attacks, privilege escalation, data exfiltration
- **Audit Trail**: Complete audit logging for forensic analysis

### Architecture

```
┌─────────────┐
│  ThemisDB   │
│   Metrics   │──┐
└─────────────┘  │
                 │    ┌───────────────┐
                 ├───▶│  Prometheus   │
                 │    └───────┬───────┘
┌─────────────┐  │            │
│  ThemisDB   │  │            │    ┌──────────────┐
│  Audit Logs │──┼────────────┼───▶│   Grafana    │
└─────────────┘  │            │    │  Dashboard   │
                 │            │    └──────────────┘
┌─────────────┐  │            │
│  ThemisDB   │  │            │    ┌──────────────┐
│  Events     │──┘            └───▶│ SIEM Systems │
└─────────────┘                    │ (Splunk/ELK) │
                                   └──────────────┘
```

---

## SIEM-Relevant Metrics

### Authentication & Authorization Metrics

These metrics track user authentication, authorization attempts, and session management - critical for detecting credential attacks and unauthorized access attempts.

#### Authentication Attempts

**Metric**: `themis_auth_attempts_total{method, status}`

- **Type**: Counter
- **Labels**:
  - `method`: Authentication method (password, token, oauth, certificate)
  - `status`: Result (success, failure)
  - `user`: Username attempting authentication
  - `source_ip`: Source IP address
- **SIEM Relevance**: Detect brute force attacks, credential stuffing, password spraying
- **Compliance**: SOC2 CC6.1, GDPR Article 32, HIPAA Access Control

**Example PromQL**:
```promql
# Failed login attempts per user/IP in last 5 minutes
sum by (user, source_ip) (increase(themis_auth_attempts_total{status="failure"}[5m]))

# Authentication success rate
(sum(rate(themis_auth_attempts_total{status="success"}[5m])) 
 / 
 sum(rate(themis_auth_attempts_total[5m]))) * 100
```

**Alert Threshold**: >5 failed attempts from same IP in 2 minutes

#### Failed Authentication Events

**Metric**: `themis_auth_failures_total{user, source_ip, reason}`

- **Type**: Counter
- **Labels**:
  - `user`: Username
  - `source_ip`: Source IP
  - `reason`: Failure reason (invalid_password, user_not_found, account_locked, expired_credentials)
- **SIEM Relevance**: Identify targeted attacks, account enumeration
- **Compliance**: SOC2 CC6.1

**Example PromQL**:
```promql
# Top failed login sources
topk(10, sum by (source_ip) (increase(themis_auth_failures_total[1h])))

# Failed logins by reason
sum by (reason) (rate(themis_auth_failures_total[5m]))
```

For complete metric documentation, see the full guide in the repository.

---

## Dashboard Setup

### Quick Start

1. **Import SIEM Dashboard**:
   ```bash
   cp grafana/siem-security-monitoring.json /etc/grafana/provisioning/dashboards/
   systemctl restart grafana-server
   ```

2. **Configure Prometheus**:
   ```yaml
   # prometheus.yml
   scrape_configs:
     - job_name: 'themisdb'
       static_configs:
         - targets: ['localhost:9091']
   
   rule_files:
     - 'alerts/siem_security_alerts.yaml'
   ```

3. **Access Dashboard**:
   - URL: http://localhost:3000
   - Navigate to: Dashboards → ThemisDB SIEM Security Monitoring

---

## Alert Configuration

### Critical Security Alerts

The following alerts are pre-configured for SIEM integration:

- **BruteForceAttackDetected**: Multiple failed authentication attempts
- **PrivilegeEscalationDetected**: Unauthorized privilege changes
- **UnauthorizedDataExport**: Data exfiltration attempts
- **AuditLogTamperingAttempt**: Integrity violations
- **BackupFailure**: Compliance-critical backup issues

See `grafana/alerts/siem_security_alerts.yaml` for complete alert definitions.

---

## Integration Examples

### Splunk Integration

```bash
# Install Prometheus exporter for Splunk
pip install prometheus-splunk-exporter

# Configure queries
cat > config.yml <<EOF
splunk:
  host: splunk.example.com
  token: your-hec-token
  index: themisdb_metrics

queries:
  - name: auth_failures
    query: 'sum by (user, source_ip) (increase(themis_auth_failures_total[5m]))'
    interval: 5m
EOF
```

### ELK Stack Integration

```ruby
# Logstash configuration
input {
  http_poller {
    urls => {
      prometheus => "http://localhost:9090/api/v1/query?query=themis_auth_failures_total"
    }
    schedule => { every => "30s" }
  }
}

output {
  elasticsearch {
    hosts => ["localhost:9200"]
    index => "themisdb-security-%{+YYYY.MM.dd}"
  }
}
```

### Syslog Integration

```yaml
# themisdb.yaml
logging:
  syslog:
    enabled: true
    server: syslog.example.com
    port: 514
    format: rfc5424
```

---

## Compliance Mapping

### SOC2 Trust Service Criteria

| Criteria | Control | ThemisDB Metrics | Alert |
|----------|---------|------------------|-------|
| CC6.1 | Logical Access | `themis_auth_attempts_total` | BruteForceAttackDetected |
| CC6.2 | Privileged Access | `themis_privilege_escalation_total` | PrivilegeEscalationDetected |
| CC7.2 | System Monitoring | `themis_replication_lag_seconds` | BackupFailure |

### GDPR Articles

| Article | Requirement | ThemisDB Metrics | Alert |
|---------|-------------|------------------|-------|
| Article 5 | Purpose Limitation | `themis_data_access_total` | SensitiveDataAccessWithoutJustification |
| Article 32 | Security of Processing | `themis_auth_failures_total` | BruteForceAttackDetected |
| Article 33 | Breach Notification | `themis_security_incidents_total` | UnauthorizedDataExport |

### HIPAA Security Rule

| Standard | ThemisDB Metrics | Alert |
|----------|------------------|-------|
| 164.308(a)(1) | `themis_security_incidents_total` | SecurityPolicyViolation |
| 164.312(a)(1) | `themis_auth_attempts_total` | FailedAuthenticationAfterSuccess |
| 164.312(b) | `themis_audit_events_total` | AuditLogTamperingAttempt |

---

## Export Formats

### JSON Export Example

```json
{
  "timestamp": "2026-01-27T10:30:00Z",
  "event_type": "authentication_failure",
  "severity": "high",
  "user": "admin",
  "source_ip": "192.168.1.100",
  "failed_attempts": 15,
  "threat_category": "brute_force",
  "compliance": ["soc2", "gdpr"],
  "action_required": "Block IP, review authentication logs"
}
```

### Syslog Format (RFC 5424)

```
<134>1 2026-01-27T10:30:00Z themisdb-prod themisdb - SECURITY [themis@32473 event_type="auth_failure" user="admin" source_ip="192.168.1.100"] Brute force attack detected
```

---

## Best Practices

### Metric Collection

✅ **DO**:
- Scrape metrics at 10-30 second intervals
- Retain security metrics for 15+ days
- Use labels for categorization
- Enable metric cardinality limits

❌ **DON'T**:
- Store PII in metric labels
- Scrape too frequently (< 5 seconds)
- Use high-cardinality values as labels

### Alert Configuration

✅ **DO**:
- Set appropriate `for` durations
- Include actionable annotations
- Test alerts regularly
- Document remediation procedures

❌ **DON'T**:
- Alert on everything
- Ignore alert fatigue
- Forget to update contacts

### Security

✅ **DO**:
- Encrypt communications (TLS)
- Implement RBAC
- Audit monitoring access
- Backup configurations

❌ **DON'T**:
- Expose metrics publicly
- Store secrets in dashboards
- Share admin credentials

---

## Support and Resources

- **GitHub Repository**: https://github.com/makr-code/ThemisDB
- **Documentation**: Located in `docs/` directory
- **Grafana Dashboards**: `grafana/` directory
- **Alert Rules**: `grafana/alerts/` directory

---

**Document Version**: 1.0  
**Last Review**: 2026-01-27  
**Owner**: Security Team
