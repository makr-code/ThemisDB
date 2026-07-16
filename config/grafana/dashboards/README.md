# ThemisDB Grafana Dashboards

This directory contains pre-configured Grafana dashboards for monitoring ThemisDB modules.

## Dashboards

### 4. ThemisDB Task Scheduler (`themisdb-scheduler-dashboard.json`)

**Purpose**: Scheduler health, concurrency, queue depth, and per-task execution monitoring  
**Update Frequency**: 30 seconds  
**Default Time Range**: Last 3 hours  
**Metrics Prefix**: `themis_scheduler_*`  
**Template Variable**: `$task_name` (multi-select, auto-populated from Prometheus labels)

**Sections**:
- Scheduler Overview (5 stat panels): Registered/Active/Running tasks, Concurrency Limit, Queue Depth
- Concurrency & Queue Depth Over Time (2 timeseries): Limit vs running vs pending, task count trends
- Execution Success & Failure Rates (2 timeseries): Success/failure rate per minute, failure percentage
- Per-Task Metrics (4 panels): Avg execution duration timeseries, per-task failure rate, Top-10 slowest tasks table, enabled/disabled status table
- Last Run Timestamps (1 table): Last execution time per task

**Best For**: Detecting scheduling bottlenecks, SLA monitoring, identifying slow or failing tasks

---

### 1. LoRA Framework Overview (`lora-framework-overview.json`)

**Purpose**: High-level system health and performance monitoring  
**Update Frequency**: 30 seconds  
**Default Time Range**: Last 1 hour

**Sections**:
- Adapter Lifecycle (6 panels): Load duration, rates, active adapters, error tracking
- Cache Performance (4 panels): Hit rate, memory usage, evictions
- Storage I/O (2 panels): Latency percentiles, throughput
- Inference Performance (4 panels): Request rate, latency, queue size, errors
- Resource Utilization (3 panels): Memory, GPU VRAM, CPU usage

**Best For**: Real-time monitoring, production dashboards, NOC displays

### 2. LoRA Training & Performance (`lora-training-performance.json`)

**Purpose**: Training operation analysis and model quality tracking  
**Update Frequency**: 30 seconds  
**Default Time Range**: Last 6 hours

**Sections**:
- Training Operations (6 panels): Duration, throughput, success rates
- Model Quality Metrics (4 panels): Loss curves, accuracy curves, comparisons
- Adapter Performance Comparison (1 table): Multi-metric comparison
- Training Bottlenecks (3 panels): Latency heatmap, mode distribution, throughput bars

**Best For**: Model development, training optimization, performance tuning

### 3. LoRA Operations & Audit (`lora-operations-audit.json`)

**Purpose**: Operations monitoring and compliance tracking  
**Update Frequency**: 30 seconds  
**Default Time Range**: Last 6 hours

**Sections**:
- Orchestrator Operations (2 panels): Operation timing and rates
- CRUD Operations (6 panels): Success rates, distribution, operation counters
- Audit Logging (4 panels): Write/query performance, log size
- Error Tracking (3 panels): Error rates by type, distribution, top errors
- System Health Indicators (3 panels): Success rate, versioning, rollbacks

**Best For**: Troubleshooting, audit compliance, operations analysis

## Quick Start

### 1. Import via Grafana UI

1. Open Grafana at http://localhost:3000
2. Navigate to **Dashboards** → **Import**
3. Click **Upload JSON file**
4. Select one of the JSON files from this directory
5. Choose your Prometheus datasource
6. Click **Import**

### 2. Automatic Provisioning (Recommended)

Add to your Grafana provisioning configuration:

```yaml
# /etc/grafana/provisioning/dashboards/lora.yml
apiVersion: 1

providers:
  - name: 'LoRA Framework'
    orgId: 1
    folder: 'LoRA Monitoring'
    type: file
    disableDeletion: false
    updateIntervalSeconds: 10
    allowUiUpdates: true
    options:
      path: /path/to/ThemisDB/config/grafana/dashboards
```

### 3. Docker Compose

```yaml
services:
  grafana:
    image: grafana/grafana:latest
    volumes:
      - ./config/grafana/dashboards:/etc/grafana/provisioning/dashboards/lora:ro
```

## Variables

All dashboards support the following template variables:

- **DS_PROMETHEUS**: Prometheus datasource (auto-configured)
- **adapter_id**: Filter by adapter ID (multi-select, supports "All")
- **training_mode**: Filter by training mode (Training dashboard only)
- **operation**: Filter by operation type (Operations dashboard only)

## Dashboard Features

### Time Controls
- **Refresh intervals**: 30s, 1m, 5m, 15m, 30m, 1h
- **Time picker**: Quick ranges (1h, 6h, 24h, 7d, 30d) and custom ranges
- **Auto-refresh**: Enabled by default at 30s

### Visualization Types
- **Time Series**: Trend analysis over time
- **Gauge**: Single-value metrics with thresholds
- **Stat**: Key performance indicators
- **Table**: Detailed comparisons and rankings
- **Bar Gauge**: Category-based comparisons
- **Pie Chart**: Distribution analysis
- **Heatmap**: Latency distribution visualization

### Alert Integration
Panels include visual alert indicators:
- 🟢 Green: Healthy state
- 🟡 Yellow: Warning threshold
- 🔴 Red: Critical threshold

## Customization

### Modifying Thresholds

1. Click panel title → **Edit**
2. Navigate to **Thresholds** in right sidebar
3. Adjust values and colors
4. Click **Apply** and **Save dashboard**

**Default Threshold Values**:
- **Cache Hit Rate**: 60% (warning), 80% (good)
- **Adapter Load Duration**: 300ms (warning), 500ms (critical)
- **Training Accuracy**: 80% (warning), 90% (good)
- **Error Rate**: 1% (warning), 5% (critical)
- **Audit Log Size**: 1GB (warning), 5GB (critical)

These thresholds should be adjusted based on your specific Service Level Objectives (SLOs).

### Adding Panels

1. Click **Add panel** button (top right)
2. Select visualization type
3. Configure Prometheus query
4. Set display options
5. Click **Apply**

### Adjusting Queries

All queries use PromQL syntax. Common patterns:

```promql
# Rate calculation
rate(metric_total[5m])

# Percentile
histogram_quantile(0.95, rate(metric_bucket[5m]))

# Aggregation
sum by (label) (metric)
```

## Prerequisites

1. **Grafana**: Version 8.0 or higher
2. **Prometheus**: Data source configured in Grafana
3. **ThemisDB**: Metrics endpoint enabled (default: :9091/metrics)
4. **Prometheus Scrape**: ThemisDB metrics being collected

Verify metrics collection:
```bash
curl http://localhost:9090/api/v1/label/__name__/values | grep themis_lora
```

## Troubleshooting

### No Data Displayed

1. Check Prometheus connection in Grafana
2. Verify ThemisDB metrics endpoint: `curl http://localhost:9091/metrics`
3. Check Prometheus scrape targets: http://localhost:9090/targets

### Variables Not Populating

1. Ensure metrics with labels exist
2. Refresh dashboard variables: Settings → Variables → Refresh
3. Check query syntax in variable configuration

### Performance Issues

1. Reduce time range (e.g., last 1h instead of 24h)
2. Use recording rules for complex queries
3. Limit multi-select variables to fewer items

## Documentation

For detailed documentation, see:
- **Metrics Guide**: `LORA_TESTING_AND_METRICS_GUIDE.md`
- **Metric Definitions**: `include/llm/lora_framework/lora_metrics.h`
- **Alert Rules**: `LORA_TESTING_AND_METRICS_GUIDE.md` (Alerting Rules section)

## Support

For issues or questions:
1. Check the Troubleshooting section in `LORA_TESTING_AND_METRICS_GUIDE.md`
2. Review Grafana logs: `/var/log/grafana/grafana.log`
3. Verify Prometheus metrics collection
4. Open an issue in the ThemisDB repository

## Version Compatibility

- **Grafana**: 8.0+
- **Prometheus**: 2.30+
- **ThemisDB**: v1.3.0+

## License

These dashboards are part of ThemisDB and follow the same license terms.
