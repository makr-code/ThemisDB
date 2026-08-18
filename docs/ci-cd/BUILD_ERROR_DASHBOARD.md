# Build Error Monitoring Dashboard

## Overview

The Build Error Monitoring Dashboard provides real-time visibility into error patterns, trends, and resolution metrics across ThemisDB CI/CD workflows. This system enables team members to quickly identify chronic failures, prioritize remediation work, and track improvement progress.

## Features

### 📊 Dashboard Types

#### 1. **Markdown Dashboard**
- Embedded in GitHub issues for easy access
- Automatic updates after each workflow run
- Responsive layout with summary metrics
- Problem area highlighting
- Historical trend visualization
- **Frequency**: Updated automatically

#### 2. **Grafana Dashboard**
- Real-time metrics from Prometheus
- Time-series analysis of error patterns
- Multi-panel drill-down views
- Custom alerting rules
- SLA tracking
- **Requires**: Prometheus metrics export

#### 3. **CSV Export**
- Spreadsheet-compatible error data
- Suitable for external analysis tools
- Historical trend tracking
- Error lifecycle tracking (opened → resolved)

## Setup Instructions

### Basic Setup (Markdown Dashboard)

The markdown dashboard is auto-generated and requires no additional setup:

```yaml
- uses: ./.github/actions/publish-error-dashboard@main
  with:
    metrics-artifact: build-error-metrics
    create-issue: 'true'
    dashboard-title: 'Build Error Monitoring Dashboard'
```

**Result**: A GitHub issue is created/updated with the dashboard every run.

### Grafana Integration Setup

#### 1. **Set up Prometheus Data Source**

In Grafana UI:
1. Go to **Configuration → Data Sources**
2. Click **Add data source** → Select **Prometheus**
3. Configure:
   - **URL**: `http://prometheus:9090` (or your Prometheus endpoint)
   - **Access**: Browser
   - **Auth**: Enable Basic Auth if secured
4. Click **Save & Test**

#### 2. **Import Dashboard JSON**

```bash
# Export dashboard JSON from workflow artifact
curl -L https://github.com/$OWNER/$REPO/raw/$BRANCH/dashboards/grafana-dashboard.json \
  -o dashboard.json

# Import via Grafana CLI
grafana-cli dashboard import dashboard.json --datasource Prometheus
```

Or via UI:
1. Go to **Dashboards → Import**
2. Upload `grafana-dashboard.json`
3. Select Prometheus data source
4. Click **Import**

#### 3. **Configure Alert Rules** (Optional)

Example Prometheus alert rule:

```yaml
groups:
  - name: themis_build_errors
    rules:
      - alert: CriticalBuildErrors
        expr: themis_error_severity_count{severity="critical"} > 5
        for: 15m
        labels:
          severity: critical
        annotations:
          summary: "Critical build errors detected ({{ $value }})"
          
      - alert: ChronicBuildFailures
        expr: themis_chronic_errors_total > 3
        for: 1h
        labels:
          severity: warning
```

### Slack Notifications

To enable Slack dashboard notifications:

```yaml
- uses: ./.github/actions/publish-error-dashboard@main
  with:
    metrics-artifact: build-error-metrics
    create-issue: 'true'
    send-slack: 'true'
    slack-webhook: ${{ secrets.SLACK_BUILD_ALERTS }}
```

**Setup Slack Webhook**:
1. Go to Slack workspace settings
2. Create incoming webhook
3. Store URL in repository secret `SLACK_BUILD_ALERTS`

## Dashboard Metrics Explained

### Summary Metrics

| Metric | Description | Action |
|--------|-------------|--------|
| **Total Errors (24h)** | Error count in last 24 hours | 🟡 > 50 = warning |
| **Unique Errors** | Count of distinct error types | 🔴 > 30 = investigate |
| **Chronic Errors** | Errors in 3+ consecutive runs | 🔴 Any = critical |
| **Critical Issues** | Severity=CRITICAL errors | 🔴 Any = immediate action |

### Error Distribution

Shows breakdown by:
- **Severity**: CRITICAL / HIGH / MEDIUM / LOW
- **Type**: compiler_error / linker_error / cmake_error / test_failure / etc.
- **Platform**: Linux / macOS / Windows / Docker
- **Workflow**: ci-build / docker-image / security-scanning / etc.

### Resolution Metrics

Tracks time from error first appearance to fix, compared against SLAs:

| Severity | SLA | Status |
|----------|-----|--------|
| CRITICAL | 2 hours | ✅ Met / 🟡 Exceeded / 🔴 Missed |
| HIGH | 8 hours | ✅ Met / 🟡 Exceeded / 🔴 Missed |
| MEDIUM | 24 hours | ✅ Met / 🟡 Exceeded / 🔴 Missed |
| LOW | 72 hours | ✅ Met / 🟡 Exceeded / 🔴 Missed |

## Integration with CI/CD

### In `maintenance-build-issues.yml`

```yaml
- name: Publish Dashboard
  uses: ./.github/actions/publish-error-dashboard@main
  with:
    metrics-artifact: build-error-metrics
    create-issue: ${{ needs.aggregate.outputs.has-errors == 'true' }}
    dashboard-title: 'Build Error Dashboard — ${{ github.run_number }}'
    send-slack: ${{ secrets.SLACK_BUILD_ALERTS != '' }}
    slack-webhook: ${{ secrets.SLACK_BUILD_ALERTS }}
```

### Storing Metrics for Analysis

Dashboards persist metrics artifacts for 90 days, enabling:
- Historical trend analysis
- Error recurrence detection
- Seasonal pattern identification
- Baseline tracking for improvement projects

**Access historical data**:
```bash
# Download metrics from specific workflow run
gh run download <run-id> -n error-dashboard -D ./metrics

# Analyze error trends over time
node -e "
  const fs = require('fs');
  const metrics = JSON.parse(fs.readFileSync('./metrics/error-metrics.json'));
  console.log(metrics.error_trends);
"
```

## Performance & Scalability

### Data Retention

- **Metrics JSON**: 90 days (GitHub artifact retention)
- **Prometheus**: Configure retention in prometheus.yml (default 15 days)
- **Dashboard Issues**: Preserved indefinitely
- **Slack Messages**: Workspace retention policy

### Dashboard Generation Performance

- Markdown generation: < 1 second
- Grafana JSON export: < 500ms
- Full pipeline (metrics + dashboard): < 30 seconds

### Scaling Considerations

For large error volumes:
1. **Increase artifact retention** only if needed (GitHub costs)
2. **Archive old metrics** to external storage (S3, GCS)
3. **Set up Prometheus remote storage** for unlimited history
4. **Implement metric downsample** for > 1000 unique errors/day

## Troubleshooting

### Dashboard Not Updating

**Check**:
1. Verify `build-error-metrics` artifact is generated
2. Confirm `generate-dashboards.js` executes without errors
3. Check GitHub Actions logs for permission issues

**Workaround**:
```bash
# Manually regenerate dashboard
export METRICS_DIR=./metrics
export DASHBOARD_DIR=./dashboards
node .github/scripts/generate-dashboards.js
```

### Prometheus Metrics Not Appearing

**Check**:
1. Metrics artifact contains `error-metrics.prom`
2. Prometheus scrape interval configured (default 15s)
3. Data source URL accessible from Grafana

**Verify metrics format**:
```bash
promtool check metrics error-metrics.prom
```

### Slack Webhook Failures

**Check**:
1. Webhook URL is valid (contains `hooks.slack.com`)
2. Workspace channel exists and is accessible
3. Bot permissions include message posting

**Test webhook**:
```bash
curl -X POST "$SLACK_WEBHOOK" \
  -H 'Content-Type: application/json' \
  -d '{"text":"Test message"}'
```

## Examples

### Query Error Trends in Prometheus

```promql
# Total errors over time
rate(themis_build_errors_total[1d])

# Errors by severity
sum(themis_errors_by_severity) by (severity)

# Chronic errors (appearing in 3+ runs)
themis_chronic_errors_total

# Error resolution latency
histogram_quantile(0.95, themis_error_resolution_seconds)
```

### Export Metrics for Analysis

```bash
# Download metrics CSV
gh run download <run-id> -n error-dashboard -D .

# Process with Python
python3 -c "
import csv
with open('error-metrics.csv') as f:
    reader = csv.DictReader(f)
    for row in reader:
        if int(row['severity']) >= 2:  # CRITICAL/HIGH
            print(f\"{row['error_type']}: {row['frequency']}\")
"
```

## Contributing

To extend dashboard functionality:

1. **Add new metric types**: Update `collect-build-metrics.js`
2. **Customize dashboard layout**: Modify `generate-dashboards.js`
3. **Add new panels**: Update `generateGrafanaDashboard()` method
4. **Change alert thresholds**: Update action inputs or edit `maintenance-build-issues.yml`

## Related Documentation

- [Build Error Metrics Guide](./BUILD_ERROR_METRICS.md)
- [Error Aggregation System](../../.github/scripts/aggregate-build-errors.js)
- [Notification Setup](../../.github/actions/notify-chronic-errors/README.md)
- [Prometheus Documentation](https://prometheus.io/docs/)
- [Grafana Documentation](https://grafana.com/docs/)

---

**Last Updated**: 2026-08-18  
**Maintained By**: CI/CD Team  
**Related Issues**: [build errors](https://github.com/$GITHUB_REPOSITORY/labels/ci/failure)
