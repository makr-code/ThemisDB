# Alerting und Monitoring für ThemisDB

**Version:** 1.0.0  
**Stand:** 6. April 2026  
**Kategorie:** Observability  
**Status:** ✅ Produktionsreif

---

## 📑 Inhaltsverzeichnis

- [Übersicht](#übersicht)
- [Prometheus Alerting](#prometheus-alerting)
- [Grafana Alerts](#grafana-alerts)
- [Alert Rules](#alert-rules)
- [Notification Channels](#notification-channels)
- [Runbooks](#runbooks)
- [Best Practices](#best-practices)

---

## Übersicht

ThemisDB bietet umfassendes Alerting für proaktives Monitoring kritischer Metriken. Alerts können über Prometheus AlertManager, Grafana oder externe Dienste versendet werden.

### Alert Kategorien

| Kategorie | Severity | Beispiele |
|-----------|----------|-----------|
| **Critical** | 🔴 P0 | Database down, Data corruption |
| **High** | 🟠 P1 | High error rate, Memory exhaustion |
| **Medium** | 🟡 P2 | Slow queries, High latency |
| **Low** | 🟢 P3 | Disk space warning, Cache misses |

---

## Prometheus Alerting

### AlertManager Setup

```yaml
# alertmanager.yml
global:
  resolve_timeout: 5m
  smtp_smarthost: 'smtp.gmail.com:587'
  smtp_from: 'alerts@company.com'
  smtp_auth_username: 'alerts@company.com'
  smtp_auth_password: 'password'

route:
  group_by: ['alertname', 'cluster', 'service']
  group_wait: 10s
  group_interval: 10s
  repeat_interval: 12h
  receiver: 'default'
  
  routes:
    # Critical alerts → PagerDuty
    - match:
        severity: critical
      receiver: 'pagerduty'
      continue: true
    
    # High priority → Slack
    - match:
        severity: high
      receiver: 'slack'
    
    # Medium/Low → Email
    - match_re:
        severity: medium|low
      receiver: 'email'

receivers:
  - name: 'default'
    email_configs:
      - to: 'ops-team@company.com'
  
  - name: 'pagerduty'
    pagerduty_configs:
      - service_key: 'YOUR_PAGERDUTY_KEY'
        severity: '{{ .CommonLabels.severity }}'
        description: '{{ .CommonAnnotations.summary }}'
  
  - name: 'slack'
    slack_configs:
      - api_url: 'https://hooks.slack.com/services/YOUR/WEBHOOK/URL'
        channel: '#themis-alerts'
        title: '{{ .GroupLabels.alertname }}'
        text: '{{ .CommonAnnotations.description }}'
  
  - name: 'email'
    email_configs:
      - to: 'dev-team@company.com'
        headers:
          Subject: '[ThemisDB] {{ .GroupLabels.alertname }}'
```

### Alert Rules

```yaml
# prometheus-alerts.yml
groups:
  - name: themisdb_critical
    interval: 30s
    rules:
      # Database Down
      - alert: ThemisDBDown
        expr: up{job="themisdb"} == 0
        for: 1m
        labels:
          severity: critical
          component: server
        annotations:
          summary: "ThemisDB instance is down"
          description: "ThemisDB instance {{ $labels.instance }} has been down for more than 1 minute."
          runbook: "https://docs.themisdb.com/runbooks/database-down"
      
      # High Error Rate
      - alert: HighErrorRate
        expr: |
          rate(themis_server_errors_total[5m]) > 10
        for: 5m
        labels:
          severity: critical
          component: server
        annotations:
          summary: "High error rate detected"
          description: "Error rate is {{ $value }} errors/sec on {{ $labels.instance }}."
          runbook: "https://docs.themisdb.com/runbooks/high-error-rate"
      
      # Memory Exhaustion
      - alert: MemoryExhaustion
        expr: |
          (themis_memory_used_bytes / themis_memory_total_bytes) > 0.95
        for: 5m
        labels:
          severity: critical
          component: memory
        annotations:
          summary: "Memory usage critically high"
          description: "Memory usage is {{ $value | humanizePercentage }} on {{ $labels.instance }}."
          runbook: "https://docs.themisdb.com/runbooks/memory-exhaustion"

  - name: themisdb_high
    interval: 1m
    rules:
      # Slow Queries
      - alert: SlowQueries
        expr: |
          histogram_quantile(0.95, 
            rate(themis_query_duration_seconds_bucket[5m])
          ) > 1
        for: 10m
        labels:
          severity: high
          component: query
        annotations:
          summary: "Slow queries detected"
          description: "95th percentile query duration is {{ $value }}s on {{ $labels.instance }}."
          runbook: "https://docs.themisdb.com/runbooks/slow-queries"
      
      # High CPU Usage
      - alert: HighCPUUsage
        expr: |
          rate(process_cpu_seconds_total{job="themisdb"}[5m]) * 100 > 80
        for: 10m
        labels:
          severity: high
          component: cpu
        annotations:
          summary: "High CPU usage"
          description: "CPU usage is {{ $value | humanizePercentage }} on {{ $labels.instance }}."
      
      # Connection Pool Exhaustion
      - alert: ConnectionPoolExhaustion
        expr: |
          (themis_connection_pool_active / themis_connection_pool_max) > 0.9
        for: 5m
        labels:
          severity: high
          component: connections
        annotations:
          summary: "Connection pool nearly exhausted"
          description: "Connection pool usage is {{ $value | humanizePercentage }} on {{ $labels.instance }}."

  - name: themisdb_medium
    interval: 5m
    rules:
      # Disk Space Warning
      - alert: DiskSpaceWarning
        expr: |
          (themis_storage_size_bytes / themis_storage_capacity_bytes) > 0.80
        for: 30m
        labels:
          severity: medium
          component: storage
        annotations:
          summary: "Disk space running low"
          description: "Disk usage is {{ $value | humanizePercentage }} on {{ $labels.instance }}."
      
      # High Cache Miss Rate
      - alert: HighCacheMissRate
        expr: |
          (
            rate(themis_cache_misses_total[10m]) /
            rate(themis_cache_requests_total[10m])
          ) > 0.5
        for: 30m
        labels:
          severity: medium
          component: cache
        annotations:
          summary: "High cache miss rate"
          description: "Cache miss rate is {{ $value | humanizePercentage }} on {{ $labels.instance }}."
      
      # Long Running Queries
      - alert: LongRunningQueries
        expr: |
          themis_query_active_count{duration=">1h"} > 0
        for: 15m
        labels:
          severity: medium
          component: query
        annotations:
          summary: "Long running queries detected"
          description: "{{ $value }} queries running for more than 1 hour on {{ $labels.instance }}."

  - name: themisdb_low
    interval: 10m
    rules:
      # Replication Lag
      - alert: ReplicationLag
        expr: |
          themis_replication_lag_seconds > 300
        for: 1h
        labels:
          severity: low
          component: replication
        annotations:
          summary: "Replication lag detected"
          description: "Replication lag is {{ $value }}s on {{ $labels.instance }}."
      
      # Index Size Growth
      - alert: IndexSizeGrowth
        expr: |
          rate(themis_index_size_bytes[24h]) > 1073741824  # 1GB per day
        for: 24h
        labels:
          severity: low
          component: index
        annotations:
          summary: "Rapid index growth"
          description: "Index growing at {{ $value | humanize }}B/day on {{ $labels.instance }}."
```

---

## Grafana Alerts

### Alert Creation

```json
{
  "dashboard": "ThemisDB Overview",
  "panelId": 1,
  "name": "High Query Latency",
  "message": "Query latency exceeds threshold",
  "severity": "warning",
  "frequency": "1m",
  "for": "5m",
  "conditions": [
    {
      "type": "query",
      "query": {
        "params": ["A", "5m", "now"]
      },
      "reducer": {
        "type": "avg"
      },
      "evaluator": {
        "type": "gt",
        "params": [1000]
      }
    }
  ],
  "notifications": [
    {
      "uid": "slack-notifications"
    }
  ]
}
```

### Grafana Alert Rules (YAML)

```yaml
# grafana-alerts.yml
apiVersion: 1

groups:
  - orgId: 1
    name: themisdb_alerts
    folder: ThemisDB
    interval: 1m
    rules:
      - uid: themis_query_latency
        title: High Query Latency
        condition: A
        data:
          - refId: A
            datasourceUid: prometheus
            model:
              expr: |
                histogram_quantile(0.95,
                  rate(themis_query_duration_seconds_bucket[5m])
                )
              intervalMs: 60000
        noDataState: NoData
        execErrState: Alerting
        for: 5m
        annotations:
          description: "95th percentile query latency is {{ $values.A }} seconds"
          runbook_url: "https://docs.themisdb.com/runbooks/high-latency"
        labels:
          severity: high
          component: query
        
      - uid: themis_error_spike
        title: Error Spike Detected
        condition: A
        data:
          - refId: A
            datasourceUid: prometheus
            model:
              expr: |
                rate(themis_server_errors_total[5m]) > 10
              intervalMs: 60000
        for: 2m
        annotations:
          description: "Error rate is {{ $values.A }} errors/sec"
        labels:
          severity: critical
```

---

## Alert Rules

### SLO-based Alerts

```yaml
# SLO: 99.9% availability (43.2 minutes downtime/month)
- alert: SLOViolation
  expr: |
    (
      1 - (
        sum(rate(themis_server_requests_success_total[30d]))
        /
        sum(rate(themis_server_requests_total[30d]))
      )
    ) > 0.001  # 0.1% error budget
  for: 5m
  labels:
    severity: critical
    slo: availability
  annotations:
    summary: "SLO violation: availability below 99.9%"
    description: "Current availability: {{ $value | humanizePercentage }}"
```

### Composite Alerts

```yaml
# Alert nur wenn mehrere Bedingungen erfüllt
- alert: ServiceDegraded
  expr: |
    (
      rate(themis_query_duration_seconds_sum[5m])
      /
      rate(themis_query_duration_seconds_count[5m])
      > 0.5
    )
    and
    (
      rate(themis_server_errors_total[5m]) > 1
    )
    and
    (
      themis_server_active_connections > 100
    )
  for: 10m
  labels:
    severity: high
  annotations:
    summary: "Service degraded: high latency + errors + high load"
```

---

## Notification Channels

### Slack Integration

```yaml
# Slack Webhook
receivers:
  - name: 'slack'
    slack_configs:
      - api_url: 'https://hooks.slack.com/services/YOUR/WEBHOOK/URL'
        channel: '#themis-alerts'
        username: 'AlertManager'
        title: '{{ .GroupLabels.alertname }}'
        text: |
          {{ range .Alerts }}
          *Alert:* {{ .Labels.alertname }}
          *Severity:* {{ .Labels.severity }}
          *Instance:* {{ .Labels.instance }}
          *Summary:* {{ .Annotations.summary }}
          *Description:* {{ .Annotations.description }}
          *Runbook:* {{ .Annotations.runbook }}
          {{ end }}
        color: '{{ if eq .Status "firing" }}danger{{ else }}good{{ end }}'
```

### PagerDuty Integration

```yaml
receivers:
  - name: 'pagerduty'
    pagerduty_configs:
      - service_key: 'YOUR_PAGERDUTY_INTEGRATION_KEY'
        description: '{{ .CommonAnnotations.summary }}'
        severity: '{{ .CommonLabels.severity }}'
        details:
          firing: '{{ .Alerts.Firing | len }}'
          resolved: '{{ .Alerts.Resolved | len }}'
          instance: '{{ .CommonLabels.instance }}'
```

### Microsoft Teams

```yaml
receivers:
  - name: 'teams'
    webhook_configs:
      - url: 'https://outlook.office.com/webhook/YOUR_WEBHOOK_URL'
        send_resolved: true
```

### Email with Template

```yaml
receivers:
  - name: 'email'
    email_configs:
      - to: 'ops@company.com'
        from: 'alertmanager@company.com'
        smarthost: 'smtp.gmail.com:587'
        auth_username: 'alerts@company.com'
        auth_password: 'password'
        headers:
          Subject: '[{{ .Status | toUpper }}] {{ .GroupLabels.alertname }}'
        html: |
          <!DOCTYPE html>
          <html>
          <body>
            <h2 style="color: {{ if eq .Status "firing" }}red{{ else }}green{{ end }};">
              {{ .GroupLabels.alertname }}
            </h2>
            <table>
              <tr>
                <td><b>Status:</b></td>
                <td>{{ .Status }}</td>
              </tr>
              <tr>
                <td><b>Severity:</b></td>
                <td>{{ .CommonLabels.severity }}</td>
              </tr>
              <tr>
                <td><b>Instance:</b></td>
                <td>{{ .CommonLabels.instance }}</td>
              </tr>
              <tr>
                <td><b>Summary:</b></td>
                <td>{{ .CommonAnnotations.summary }}</td>
              </tr>
            </table>
            <p>{{ .CommonAnnotations.description }}</p>
            <a href="{{ .CommonAnnotations.runbook }}">Runbook</a>
          </body>
          </html>
```

---

## Runbooks

### Database Down

**Alert:** `ThemisDBDown`  
**Severity:** Critical

**Diagnosis:**
```bash
# 1. Check if process is running
ps aux | grep themis

# 2. Check system resources
df -h
free -m
top

# 3. Check logs
tail -n 100 /var/log/themisdb/server.log

# 4. Check network
netstat -tulpn | grep 8765
```

**Resolution:**
```bash
# 1. Restart service
systemctl restart themisdb

# 2. If won't start, check config
themisdb --validate-config

# 3. Check for port conflicts
lsof -i :8765

# 4. Start in debug mode
themisdb --debug --foreground
```

### High Error Rate

**Alert:** `HighErrorRate`  
**Severity:** Critical

**Diagnosis:**
```bash
# 1. Check error types
curl http://localhost:8765/metrics | grep themis_errors

# 2. Check recent logs
tail -n 1000 /var/log/themisdb/server.log | grep ERROR

# 3. Check slow queries
curl http://localhost:8765/api/v1/admin/slow-queries
```

**Resolution:**
```bash
# 1. Kill long-running queries
curl -X DELETE http://localhost:8765/api/v1/admin/queries/{query_id}

# 2. Clear cache if corruption suspected
curl -X POST http://localhost:8765/api/v1/admin/cache/clear

# 3. Restart if persistent
systemctl restart themisdb
```

### Memory Exhaustion

**Alert:** `MemoryExhaustion`  
**Severity:** Critical

**Diagnosis:**
```bash
# 1. Check memory distribution
curl http://localhost:8765/api/v1/admin/memory/stats

# 2. Check for memory leaks
valgrind --leak-check=full themisdb

# 3. Identify large objects
curl http://localhost:8765/api/v1/admin/memory/top-consumers
```

**Resolution:**
```bash
# 1. Clear caches
curl -X POST http://localhost:8765/api/v1/admin/cache/clear

# 2. Kill memory-intensive queries
curl http://localhost:8765/api/v1/admin/queries | jq '.[] | select(.memory_mb > 1000)'

# 3. Adjust memory limits
themisdb --max-memory 32GB

# 4. Restart as last resort
systemctl restart themisdb
```

---

## Best Practices

### 1. Alert Hierarchy

```yaml
# Klare Severity-Levels
critical:   Requires immediate action (< 5 min response)
high:       Requires action within 1 hour
medium:     Requires action within 1 day
low:        Nice to know, no immediate action

# Eskalation
1. Critical → PagerDuty → On-call engineer
2. High → Slack → Team channel
3. Medium → Email → Dev team
4. Low → Dashboard → Review in standup
```

### 2. Alert Fatigue vermeiden

```yaml
# ✅ Gut: Gruppierte Alerts
group_by: ['alertname', 'cluster']
group_interval: 5m

# ❌ Schlecht: Einzelne Alerts
group_by: []
group_interval: 0s

# ✅ Gut: Sinnvolle Thresholds
expr: rate(errors[5m]) > 10  # >10 errors/sec

# ❌ Schlecht: Zu sensitiv
expr: errors > 0  # Jeder Fehler = Alert!
```

### 3. Runbook-Links

```yaml
annotations:
  summary: "High error rate"
  description: "{{ $value }} errors/sec"
  runbook: "https://docs.company.com/runbooks/high-errors"
  dashboard: "https://grafana.company.com/d/themis-overview"
```

### 4. Testing Alerts

```bash
# Alert Rule testen
promtool check rules prometheus-alerts.yml

# Alert simulieren
curl -X POST http://localhost:9093/api/v1/alerts \
  -d '[{
    "labels": {
      "alertname": "TestAlert",
      "severity": "critical"
    },
    "annotations": {
      "summary": "Test alert"
    }
  }]'
```

---

## Siehe auch

- [OpenTelemetry Integration](observability_opentelemetry.md)
- [Prometheus Metrics](observability_prometheus.md)
- [Grafana Dashboards](../tools/operations/GRAFANA_DASHBOARDS.md)
- [Production Monitoring](../production/PRODUCTION_MONITORING.md)
