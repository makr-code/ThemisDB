# Service Level Agreements (SLA) and Monitoring

**Version:** 1.0  
**Last Updated:** April 2026  
**Target Audience:** Operations Teams, SREs, Management

## Table of Contents

1. [Service Level Objectives](#service-level-objectives)
2. [SLA Definitions](#sla-definitions)
3. [Monitoring Strategy](#monitoring-strategy)
4. [Prometheus Alerting Rules](#prometheus-alerting-rules)
5. [Grafana Dashboards](#grafana-dashboards)
6. [SLA Reporting](#sla-reporting)
7. [Incident Response](#incident-response)

---

## Service Level Objectives

### SLO Framework

ThemisDB uses the following SLO tiers based on service criticality:

#### Tier 1: Production Services (99.9% - "Three Nines")

**Target Availability:** 99.9%  
**Allowed Downtime:** 43.8 minutes/month (8.76 hours/year)

**Services:**
- Production inference endpoints
- Real-time LLM APIs
- Critical customer-facing services

**Performance Targets:**
- P50 Latency: < 50ms
- P95 Latency: < 100ms
- P99 Latency: < 200ms
- Error Rate: < 0.1%

#### Tier 2: Standard Services (99.5% - "Two Nines Five")

**Target Availability:** 99.5%  
**Allowed Downtime:** 3.6 hours/month (43.8 hours/year)

**Services:**
- Model training workloads
- Batch processing
- Scheduled inference jobs

**Performance Targets:**
- P50 Latency: < 100ms
- P95 Latency: < 500ms
- P99 Latency: < 1000ms
- Error Rate: < 0.5%

#### Tier 3: Best Effort (99.0%)

**Target Availability:** 99.0%  
**Allowed Downtime:** 7.2 hours/month (87.6 hours/year)

**Services:**
- Development environments
- Experimental features
- Non-critical batch jobs

**Performance Targets:**
- P50 Latency: < 500ms
- P95 Latency: < 2000ms
- P99 Latency: < 5000ms
- Error Rate: < 1.0%

---

## SLA Definitions

### Availability SLA

**Measurement Period:** Calendar month (UTC)

**Calculation:**
```
Availability % = ((Total Minutes - Downtime Minutes) / Total Minutes) × 100
```

**Exclusions (not counted as downtime):**
- Scheduled maintenance (with 7 days notice)
- Customer-caused issues
- Force majeure events
- Beta/experimental features

**SLA Credits (for Tier 1):**
- 99.0-99.9%: 10% monthly credit
- 95.0-99.0%: 25% monthly credit
- <95.0%: 50% monthly credit

### Performance SLA

**Inference Latency (P95):**
- Tier 1: < 100ms
- Tier 2: < 500ms
- Tier 3: < 2000ms

**Throughput:**
- Tier 1: > 1000 req/sec per GPU
- Tier 2: > 500 req/sec per GPU
- Tier 3: Best effort

**Training Performance:**
- GPU Utilization: > 85%
- Training Steps: As estimated ± 10%

### Data SLA

**Durability:**
- 99.999999999% (11 nines) - Multi-region replication
- Zero data loss for committed transactions

**Backup SLA:**
- RPO: 5 minutes (Tier 1), 1 hour (Tier 2), 24 hours (Tier 3)
- RTO: 1 hour (Tier 1), 4 hours (Tier 2), 24 hours (Tier 3)
- Backup success rate: > 99.9%

---

## Monitoring Strategy

### Metrics Collection

**Infrastructure Metrics:**
```yaml
# Prometheus scrape configuration
scrape_configs:
  - job_name: 'themisdb-tier1'
    scrape_interval: 10s
    scrape_timeout: 5s
    static_configs:
      - targets: ['themisdb-tier1:9091']
        labels:
          tier: 'tier1'
          service: 'inference'
    
  - job_name: 'themisdb-tier2'
    scrape_interval: 30s
    static_configs:
      - targets: ['themisdb-tier2:9092']
        labels:
          tier: 'tier2'
          service: 'training'
```

**Key Metrics:**

1. **Availability Metrics:**
   - `themisdb_up`: Service health (0/1)
   - `themisdb_http_requests_total`: Total requests
   - `themisdb_http_requests_failed`: Failed requests

2. **Performance Metrics:**
   - `themisdb_request_duration_seconds`: Request latency histogram
   - `themisdb_inference_duration_seconds`: Inference time
   - `themisdb_gpu_utilization`: GPU usage percentage

3. **Resource Metrics:**
   - `themisdb_gpu_memory_used_bytes`: GPU memory usage
   - `themisdb_gpu_temperature_celsius`: GPU temperature
   - `themisdb_disk_usage_bytes`: Disk usage

4. **Business Metrics:**
   - `themisdb_tokens_generated_total`: Tokens generated count
   - `themisdb_models_loaded`: Active models count
   - `themisdb_active_users`: Concurrent users

### SLO Recording Rules

```yaml
# /etc/prometheus/rules/slo.yml
groups:
  - name: slo_rules
    interval: 1m
    rules:
      # Availability SLO
      - record: slo:availability:ratio
        expr: |
          sum(rate(themisdb_http_requests_total[5m]))
          /
          (sum(rate(themisdb_http_requests_total[5m])) + sum(rate(themisdb_http_requests_failed[5m])))
        labels:
          tier: "tier1"
      
      # Latency SLO (P95 < 100ms for Tier 1)
      - record: slo:latency:p95
        expr: histogram_quantile(0.95, rate(themisdb_request_duration_seconds_bucket[5m]))
        labels:
          tier: "tier1"
      
      # Error rate SLO
      - record: slo:error_rate:ratio
        expr: |
          sum(rate(themisdb_http_requests_failed[5m]))
          /
          sum(rate(themisdb_http_requests_total[5m]))
        labels:
          tier: "tier1"
      
      # Error budget remaining (monthly)
      - record: slo:error_budget:remaining
        expr: |
          1 - (
            sum(rate(themisdb_downtime_seconds_total[30d]))
            /
            (30 * 24 * 60 * 60 * 0.001)  # 0.1% error budget
          )
        labels:
          tier: "tier1"
```

---

## Prometheus Alerting Rules

### Critical SLA Alerts

```yaml
# /etc/prometheus/rules/alerts_sla.yml
groups:
  - name: sla_critical
    interval: 1m
    rules:
      # Availability SLA violation
      - alert: SLAAvailabilityTier1Critical
        expr: |
          slo:availability:ratio{tier="tier1"} < 0.999
        for: 5m
        labels:
          severity: critical
          tier: tier1
          category: sla
        annotations:
          summary: "Tier 1 availability SLA violation"
          description: "Availability is {{ $value | humanizePercentage }}, below 99.9% target. Error budget burning."
          runbook: "https://docs.themisdb.io/runbooks/#sla-availability-violation"
      
      # Latency SLA violation
      - alert: SLALatencyTier1Critical
        expr: |
          slo:latency:p95{tier="tier1"} > 0.1  # 100ms
        for: 5m
        labels:
          severity: critical
          tier: tier1
          category: sla
        annotations:
          summary: "Tier 1 P95 latency SLA violation"
          description: "P95 latency is {{ $value | humanizeDuration }}, above 100ms target."
          runbook: "https://docs.themisdb.io/runbooks/#sla-latency-violation"
      
      # Error rate SLA violation
      - alert: SLAErrorRateTier1Critical
        expr: |
          slo:error_rate:ratio{tier="tier1"} > 0.001  # 0.1%
        for: 5m
        labels:
          severity: critical
          tier: tier1
          category: sla
        annotations:
          summary: "Tier 1 error rate SLA violation"
          description: "Error rate is {{ $value | humanizePercentage }}, above 0.1% target."
          runbook: "https://docs.themisdb.io/runbooks/#sla-error-rate-violation"
      
      # Error budget burn rate (fast burn)
      - alert: SLAErrorBudgetFastBurn
        expr: |
          slo:error_budget:remaining{tier="tier1"} < 0.9
          and
          rate(slo:error_budget:remaining{tier="tier1"}[1h]) < -0.01  # Burning >1% per hour
        for: 5m
        labels:
          severity: critical
          tier: tier1
          category: sla
        annotations:
          summary: "Error budget burning rapidly"
          description: "Error budget at {{ $value | humanizePercentage }}, burning fast. Immediate action required."
          runbook: "https://docs.themisdb.io/runbooks/#error-budget-management"

  - name: sla_warning
    interval: 1m
    rules:
      # Error budget warning (50% remaining)
      - alert: SLAErrorBudgetLow
        expr: |
          slo:error_budget:remaining{tier="tier1"} < 0.5
        for: 10m
        labels:
          severity: warning
          tier: tier1
          category: sla
        annotations:
          summary: "Error budget low"
          description: "Error budget at {{ $value | humanizePercentage }}, below 50%. Review incidents and consider freezing deployments."
      
      # GPU utilization below target
      - alert: SLAGPUUtilizationLow
        expr: |
          avg(themisdb_gpu_utilization) < 85
        for: 30m
        labels:
          severity: warning
          category: sla
        annotations:
          summary: "GPU utilization below SLA target"
          description: "Average GPU utilization is {{ $value }}%, below 85% target for training workloads."
      
      # Backup SLA warning
      - alert: SLABackupDelayed
        expr: |
          time() - themisdb_last_backup_timestamp_seconds > 3600  # 1 hour
        for: 5m
        labels:
          severity: warning
          category: sla
          tier: tier1
        annotations:
          summary: "Backup SLA at risk"
          description: "Last backup was {{ $value | humanizeDuration }} ago, exceeding 1 hour RPO target."
          runbook: "https://docs.themisdb.io/runbooks/#backup-failure"

  - name: sla_tier2
    interval: 1m
    rules:
      # Tier 2 availability
      - alert: SLAAvailabilityTier2Warning
        expr: |
          slo:availability:ratio{tier="tier2"} < 0.995
        for: 10m
        labels:
          severity: warning
          tier: tier2
          category: sla
        annotations:
          summary: "Tier 2 availability SLA at risk"
          description: "Availability is {{ $value | humanizePercentage }}, approaching 99.5% target."
```

### GPU-Specific SLA Alerts

```yaml
# /etc/prometheus/rules/alerts_gpu.yml
groups:
  - name: gpu_sla
    interval: 30s
    rules:
      # GPU failure
      - alert: SLAGPUFailure
        expr: |
          themisdb_gpu_health_status == 0
        for: 1m
        labels:
          severity: critical
          category: sla
        annotations:
          summary: "GPU failure detected"
          description: "GPU {{ $labels.gpu_id }} on {{ $labels.instance }} has failed."
          runbook: "https://docs.themisdb.io/runbooks/#gpu-failure-response"
      
      # GPU temperature critical
      - alert: SLAGPUTemperatureCritical
        expr: |
          themisdb_gpu_temperature_celsius > 85
        for: 5m
        labels:
          severity: critical
          category: sla
        annotations:
          summary: "GPU temperature critical"
          description: "GPU {{ $labels.gpu_id }} temperature is {{ $value }}°C, above 85°C threshold."
          runbook: "https://docs.themisdb.io/runbooks/#gpu-thermal-issues"
      
      # GPU memory exhausted
      - alert: SLAGPUMemoryExhausted
        expr: |
          (themisdb_gpu_memory_used_bytes / themisdb_gpu_memory_total_bytes) > 0.95
        for: 5m
        labels:
          severity: warning
          category: sla
        annotations:
          summary: "GPU memory usage critical"
          description: "GPU {{ $labels.gpu_id }} memory usage is {{ $value | humanizePercentage }}, above 95%."
```

---

## Grafana Dashboards

### SLA Overview Dashboard

Create dashboard at `/etc/grafana/provisioning/dashboards/sla-overview.json`:

```json
{
  "dashboard": {
    "title": "SLA Overview",
    "tags": ["sla", "overview"],
    "timezone": "UTC",
    "panels": [
      {
        "id": 1,
        "title": "Tier 1 Availability (Monthly)",
        "type": "stat",
        "targets": [
          {
            "expr": "slo:availability:ratio{tier=\"tier1\"} * 100",
            "legendFormat": "Availability %"
          }
        ],
        "fieldConfig": {
          "defaults": {
            "unit": "percent",
            "thresholds": {
              "steps": [
                {"value": 0, "color": "red"},
                {"value": 99.5, "color": "yellow"},
                {"value": 99.9, "color": "green"}
              ]
            }
          }
        }
      },
      {
        "id": 2,
        "title": "P95 Latency (5m)",
        "type": "graph",
        "targets": [
          {
            "expr": "slo:latency:p95{tier=\"tier1\"} * 1000",
            "legendFormat": "Tier 1"
          },
          {
            "expr": "slo:latency:p95{tier=\"tier2\"} * 1000",
            "legendFormat": "Tier 2"
          }
        ],
        "yaxes": [
          {
            "label": "Latency (ms)",
            "format": "short"
          }
        ]
      },
      {
        "id": 3,
        "title": "Error Budget Remaining",
        "type": "gauge",
        "targets": [
          {
            "expr": "slo:error_budget:remaining{tier=\"tier1\"} * 100",
            "legendFormat": "Tier 1 Budget"
          }
        ],
        "fieldConfig": {
          "defaults": {
            "unit": "percent",
            "thresholds": {
              "steps": [
                {"value": 0, "color": "red"},
                {"value": 25, "color": "orange"},
                {"value": 50, "color": "yellow"},
                {"value": 75, "color": "green"}
              ]
            }
          }
        }
      },
      {
        "id": 4,
        "title": "Request Success Rate",
        "type": "graph",
        "targets": [
          {
            "expr": "sum(rate(themisdb_http_requests_total[5m])) - sum(rate(themisdb_http_requests_failed[5m]))",
            "legendFormat": "Successful Requests/sec"
          },
          {
            "expr": "sum(rate(themisdb_http_requests_failed[5m]))",
            "legendFormat": "Failed Requests/sec"
          }
        ]
      },
      {
        "id": 5,
        "title": "GPU Utilization",
        "type": "graph",
        "targets": [
          {
            "expr": "avg(themisdb_gpu_utilization) by (gpu_id)",
            "legendFormat": "GPU {{ gpu_id }}"
          }
        ],
        "yaxes": [
          {
            "label": "Utilization %",
            "format": "percent",
            "max": 100,
            "min": 0
          }
        ]
      },
      {
        "id": 6,
        "title": "Downtime (Last 30 Days)",
        "type": "stat",
        "targets": [
          {
            "expr": "sum(themisdb_downtime_seconds_total[30d]) / 60",
            "legendFormat": "Downtime (minutes)"
          }
        ],
        "fieldConfig": {
          "defaults": {
            "unit": "m",
            "thresholds": {
              "steps": [
                {"value": 0, "color": "green"},
                {"value": 43.8, "color": "yellow"},
                {"value": 87.6, "color": "red"}
              ]
            }
          }
        }
      }
    ]
  }
}
```

### Custom Dashboards

**Deploy dashboards:**
```bash
# Copy dashboard configurations
sudo cp /path/to/dashboards/*.json /etc/grafana/provisioning/dashboards/

# Restart Grafana
sudo systemctl restart grafana-server
```

**Available Dashboards:**
1. `sla-overview.json` - High-level SLA metrics
2. `sla-detailed.json` - Detailed per-service SLA tracking
3. `error-budget.json` - Error budget burn rate and remaining
4. `gpu-performance.json` - GPU-specific performance metrics
5. `inference-latency.json` - Inference latency breakdown

---

## SLA Reporting

### Automated Monthly Reports

```bash
# /usr/local/bin/generate-sla-report.sh
#!/bin/bash

MONTH=$(date -d "last month" +%Y-%m)
OUTPUT="/reports/sla-report-${MONTH}.pdf"

# Generate report
themisdb-cli sla report \
  --month "${MONTH}" \
  --output "${OUTPUT}" \
  --include-metrics \
  --include-incidents \
  --format pdf

# Email report
mail -s "ThemisDB SLA Report - ${MONTH}" \
  -A "${OUTPUT}" \
  stakeholders@example.com < /dev/null
```

### Report Contents

**Executive Summary:**
- Overall availability percentage
- SLA target vs. actual
- Notable incidents
- Trend analysis

**Detailed Metrics:**
- Availability per tier
- Latency percentiles (P50, P95, P99)
- Error rates
- GPU utilization
- Backup success rates

**Incident Analysis:**
- Number of incidents
- Mean Time To Detect (MTTD)
- Mean Time To Resolve (MTTR)
- Root causes
- Preventive actions

**Error Budget:**
- Starting budget
- Budget consumed
- Remaining budget
- Burn rate trend

### SLA Query Examples

```bash
# Check current month SLA status
themisdb-cli sla status --tier tier1 --month current

# Calculate availability for specific date range
themisdb-cli sla calculate \
  --start "2026-01-01" \
  --end "2026-01-31" \
  --tier tier1

# Export SLA data for analysis
themisdb-cli sla export \
  --month 2026-01 \
  --format csv \
  --output /tmp/sla-data-2026-01.csv
```

---

## Incident Response

### SLA Violation Response

**When SLA alert fires:**

```bash
# 1. Acknowledge alert
themisdb-cli alert ack --alert-id <alert-id>

# 2. Assess impact
themisdb-cli sla impact-assessment

# Expected output:
# Current Availability: 99.7% (target: 99.9%)
# Error Budget: 45% remaining
# Affected Services: Tier 1 inference
# Estimated Users Impacted: 1,234

# 3. Follow incident runbook
# See RUNBOOKS.md for specific procedures

# 4. Track SLA impact during incident
themisdb-cli sla track-incident \
  --incident-id INC-2026-001 \
  --real-time

# 5. After resolution, document SLA impact
themisdb-cli sla incident-report \
  --incident-id INC-2026-001 \
  --output /reports/inc-2026-001-sla-impact.pdf
```

### Error Budget Management

**Error budget policies:**

```yaml
# /etc/themisdb/error-budget-policy.yaml
error_budget_policy:
  tier1:
    freeze_deployments_at: 0.25  # 25% remaining
    high_alert_at: 0.50
    warning_at: 0.75
    
    actions:
      - threshold: 0.25
        action: freeze_all_deployments
        notification: critical
      
      - threshold: 0.50
        action: freeze_risky_deployments
        notification: warning
      
      - threshold: 0.75
        action: increase_monitoring
        notification: info
```

**Check error budget:**
```bash
# View current error budget
themisdb-cli sla error-budget --tier tier1

# Expected output:
# Error Budget Status:
# Target: 99.9% availability (0.1% error budget)
# Current: 99.92% availability
# Budget Remaining: 78%
# Burn Rate: 2.3% per week
# Projected Budget End: 2026-03-15

# If budget low, trigger policy
BUDGET_REMAINING=$(themisdb-cli sla error-budget --tier tier1 --json | jq -r '.remaining')
if (( $(echo "$BUDGET_REMAINING < 0.25" | bc -l) )); then
  themisdb-cli deployment freeze --reason "Error budget exhausted"
fi
```

---

## Appendix

### SLA Metrics Dictionary

| Metric | Definition | Target (Tier 1) |
|--------|------------|-----------------|
| Availability | % of time service is operational | 99.9% |
| P50 Latency | 50th percentile request latency | < 50ms |
| P95 Latency | 95th percentile request latency | < 100ms |
| P99 Latency | 99th percentile request latency | < 200ms |
| Error Rate | % of failed requests | < 0.1% |
| MTTD | Mean Time To Detect | < 5 min |
| MTTR | Mean Time To Resolve | < 30 min |
| GPU Utilization | % of GPU compute used | > 85% |

### Related Documentation

- [Operational Runbooks](RUNBOOKS.md)
- [Monitoring Guide](MONITORING.md)
- [Disaster Recovery](DISASTER_RECOVERY.md)
- [Incident Response Checklist](CHECKLISTS/incident_response.md)

---

**Document Version:** 1.0  
**Last Updated:** April 2026  
**Next Review:** April 2026  
**Owner:** SRE Team
