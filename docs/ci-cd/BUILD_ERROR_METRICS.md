# Build Error Metrics Collection

Comprehensive system for collecting, aggregating, and exporting build error metrics in multiple formats including Prometheus and Grafana compatibility.

## Overview

The metrics collection system analyzes error patterns across CI runs to provide insights into:

- **Error Distribution**: By type, severity, platform, and workflow
- **Time Trends**: Error frequency over 24h, 7d, 30d periods
- **Problematic Components**: Files and systems with most errors
- **Resolution Latency**: Average time from error detection to fix
- **Chronic Errors**: Persistent issues requiring attention

## Features

### Supported Export Formats

- **JSON**: Structured metrics for programmatic consumption
- **Prometheus**: OpenMetrics format for Prometheus/Grafana integration
- **CSV**: Tabular format for spreadsheet analysis
- **Markdown**: Human-readable reports for dashboards

### Metrics Tracked

#### Error Counts
- `themis_build_errors_total`: Total error instances captured
- `themis_unique_errors_total`: Number of unique errors (deduplicated)
- `themis_chronic_errors_total`: Errors appearing ≥3 consecutive runs
- `themis_average_error_frequency`: Average occurrences per unique error

#### Error Distribution
- By error type (compiler_error, linker_error, test_failure, etc.)
- By severity (critical, high, medium, low)
- By platform (Linux, Windows, macOS)
- By workflow (CI — Build, Docker Image CI/CD, etc.)

#### Trends
- Last 24 hours
- Last 7 days
- Last 30 days

#### Resolution Latency
- CRITICAL: Average hours from first occurrence to fix
- HIGH: Average hours
- MEDIUM: Average hours
- LOW: Average hours
- P50/P95 percentiles

#### File Analysis
- Top 20 problematic files by error count
- Helps identify modules needing refactoring/testing

## Usage

### In Maintenance Workflow

```yaml
- name: Collect build error metrics
  shell: bash
  env:
    ERROR_ARTIFACTS_DIR: /tmp/error-artifacts
    METRICS_DIR: /tmp/error-metrics
    OUTPUT_FORMAT: json,prometheus,markdown
  run: node .github/scripts/collect-build-metrics.js

- name: Upload metrics
  uses: actions/upload-artifact@v4
  with:
    name: error-metrics
    path: /tmp/error-metrics/
    retention-days: 90
```

### Standalone Usage

```bash
# Collect metrics from error artifacts
export ERROR_ARTIFACTS_DIR="/path/to/error-artifacts"
export METRICS_DIR="/tmp/metrics"
export OUTPUT_FORMAT="json,prometheus,csv,markdown"

node .github/scripts/collect-build-metrics.js
```

## Output Files

### JSON Format (`error-metrics.json`)

```json
{
  "timestamp": "2026-08-18T11:00:00Z",
  "total_unique_errors": 45,
  "total_error_instances": 187,
  "error_types_count": {
    "compiler_error": 85,
    "linker_error": 42,
    "test_failure": 35,
    "sanitizer_error": 15,
    "cmake_error": 10
  },
  "error_severity_count": {
    "critical": 15,
    "high": 95,
    "medium": 52,
    "low": 25
  },
  "error_by_platform": {
    "Linux": 120,
    "Windows": 45,
    "macOS": 22
  },
  "chronic_errors": 8,
  "average_error_frequency": "4.16",
  "error_trends": {
    "last_24h": 187,
    "last_7d": 892,
    "last_30d": 3245
  },
  "top_problematic_files": {
    "src/exporters/huggingface_hub_client.cpp": 12,
    "src/sharding/distributed_query.cpp": 8,
    "tests/integration/test_transaction.cpp": 7
  }
}
```

### Prometheus Format (`error-metrics.prom`)

```prometheus
# HELP themis_build_errors_total Total number of build errors captured
# TYPE themis_build_errors_total counter
themis_build_errors_total{} 187

# HELP themis_unique_errors_total Total number of unique errors
# TYPE themis_unique_errors_total gauge
themis_unique_errors_total{} 45

# HELP themis_chronic_errors_total Number of errors appearing 3+ times
# TYPE themis_chronic_errors_total gauge
themis_chronic_errors_total{} 8

# HELP themis_errors_by_type Number of errors by type
# TYPE themis_errors_by_type gauge
themis_errors_by_type{type="compiler_error"} 85
themis_errors_by_type{type="linker_error"} 42
themis_errors_by_type{type="test_failure"} 35
themis_errors_by_type{type="sanitizer_error"} 15

# ... and more
```

### Markdown Report (`METRICS_REPORT.md`)

```markdown
# Build Error Metrics Report

**Generated**: 2026-08-18T11:00:00Z

## Summary Statistics

- **Total Error Instances**: 187
- **Unique Errors**: 45
- **Chronic Errors** (≥3x): 8
- **Average Frequency**: 4.16x per unique error

## Error Distribution

### By Type

- **compiler_error**: 85 (45.5%)
- **linker_error**: 42 (22.5%)
- **test_failure**: 35 (18.7%)
- **sanitizer_error**: 15 (8.0%)
- **cmake_error**: 10 (5.3%)

### By Severity

- **critical**: 15 (8.0%)
- **high**: 95 (50.8%)
- **medium**: 52 (27.8%)
- **low**: 25 (13.4%)

### By Platform

- **Linux**: 120 (64.2%)
- **Windows**: 45 (24.1%)
- **macOS**: 22 (11.8%)

## Time Trends

- **Last 24h**: 187 errors
- **Last 7d**: 892 errors
- **Last 30d**: 3245 errors

## Top Problematic Files

1. **src/exporters/huggingface_hub_client.cpp**: 12 errors
2. **src/sharding/distributed_query.cpp**: 8 errors
3. **tests/integration/test_transaction.cpp**: 7 errors
...

## Error Resolution Latency

Average time from error first occurrence to fix:

- **CRITICAL**: 2.5h
- **HIGH**: 8.0h
- **MEDIUM**: 24.0h
- **LOW**: 72.0h
```

### CSV Format (`error-metrics.csv`)

```csv
timestamp,error_type,error_message,severity,platform,workflow,frequency
2026-08-18T10:15:00Z,compiler_error,"undefined reference to symbol",high,Linux,CI — Build,5
2026-08-18T10:30:00Z,sanitizer_error,"AddressSanitizer: use-after-free",critical,Linux,CI — Build,3
2026-08-18T11:00:00Z,linker_error,"multiple definition of symbol",high,Windows,CI — Build,2
```

## Prometheus/Grafana Integration

### Setting Up Prometheus Scraper

1. Download metrics artifact from GitHub Actions
2. Configure Prometheus scrape job:

```yaml
scrape_configs:
  - job_name: 'themis_build_errors'
    static_configs:
      - targets: ['metrics-server:9090']
    metrics_path: '/error-metrics.prom'
```

3. Query metrics in Grafana:

```promql
# Recent error rate
rate(themis_build_errors_total[1h])

# Errors by severity
themis_errors_by_severity{severity="critical"}

# Chronic error trend
themis_chronic_errors_total

# Platform comparison
themis_errors_by_platform
```

### Dashboard Queries

**Top Error Types (24h)**:
```promql
topk(5, themis_errors_by_type)
```

**Error Severity Distribution**:
```promql
themis_errors_by_severity
```

**Platform Stability**:
```promql
themis_errors_by_platform
```

**Chronic Error Count Trend**:
```promql
themis_chronic_errors_total
```

## Historical Tracking

Metrics are stored as artifacts with 90-day retention. To track trends:

1. Download metrics artifacts from past runs
2. Combine JSON files into time-series data
3. Upload to external metrics platform (InfluxDB, Prometheus, etc.)

Example trend analysis script:

```bash
# Collect metrics from last 30 runs
for run_id in $(gh run list --limit 30 --json databaseId); do
  gh run download $run_id -n error-metrics -D /tmp/metrics/$run_id
done

# Combine into time-series
jq -s 'group_by(.timestamp) | map({timestamp: .[0].timestamp, errors: map(.total_error_instances) | add})' \
  /tmp/metrics/*/error-metrics.json > combined-trends.json
```

## Configuration

### Environment Variables

- `ERROR_ARTIFACTS_DIR`: Directory containing error-*.json files (default: `/tmp/error-artifacts`)
- `METRICS_DIR`: Output directory for metrics files (default: `/tmp/error-metrics`)
- `OUTPUT_FORMAT`: Comma-separated formats: json,prometheus,csv,markdown (default: `json,prometheus`)

### Customization

Modify `collect-build-metrics.js` to:
- Add custom metrics
- Change time-period calculations
- Customize Prometheus labels
- Add machine learning trend predictions

## Integrations

### GitHub Issues

Link metrics in maintenance issues:

```markdown
## Metrics Dashboard

- [Error Metrics Report](https://github.com/.../.../actions/runs/12345/attempts/1/artifact-link)
- [Raw JSON](https://github.com/.../.../actions/runs/12345/attempts/1/json)
- [Prometheus Format](https://github.com/.../.../actions/runs/12345/attempts/1/prom)
```

### Slack Notifications

Include metrics in Slack alerts:

```
Error Summary (Last 24h)
├─ Total: 187 errors
├─ Unique: 45 errors
├─ Chronic: 8 errors
└─ Top Platform: Linux (120)
```

### External Dashboards

Upload to:
- **Grafana Cloud**: Via remote storage adapter
- **Prometheus**: Via remote write config
- **InfluxDB**: Via Telegraf/Prometheus plugin
- **Datadog**: Via custom agent

## Troubleshooting

### No Metrics Generated

1. Check `ERROR_ARTIFACTS_DIR` contains error-*.json files
2. Verify error files are valid JSON format
3. Check script permissions and Node.js version

### Prometheus Format Issues

1. Validate metrics with `promtool check metrics error-metrics.prom`
2. Ensure all labels are properly quoted
3. Check for duplicate metric definitions

### Missing Time-Series Data

- Metrics are calculated for current run only
- Historical data requires multi-run collection
- Implement artifact retention policy to preserve history

## License

Same as repository.
