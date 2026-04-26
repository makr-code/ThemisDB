> **Hinweis:** Inhalt mit aktuellem Modulcode und -stand abgleichen.

# Ethics Monitoring Dashboard

> **Historischer Stand:** 2026-01-31 — Inhalte nicht gegen aktuelle Quellen geprüft.

## Overview

The `ethics_monitoring_dashboard.py` module provides comprehensive real-time monitoring and visualization capabilities for the ethical AI system. It tracks ethics evaluation metrics over time, detects anomalies, and provides multiple output formats for dashboards and monitoring systems.

## Features

### 1. MetricsAggregator
Collects and aggregates metrics from the ethics evaluation system:
- **Time-series data management**: Efficient storage using deques with configurable size limits
- **Rolling windows**: Hourly, daily, weekly, monthly aggregation
- **Trend analysis**: Automatic detection of increasing/decreasing/stable trends
- **Historical tracking**: Query metrics across any time range
- **Detailed metrics**: Tracks 24+ individual metrics across 5 ethics dimensions

### 2. DashboardRenderer
Visualize metrics in multiple formats:
- **Terminal Dashboard**: ASCII visualization with progress bars and trend indicators
- **JSON Export**: Structured data for web dashboards
- **Prometheus Format**: Industry-standard metrics format
- **Grafana Integration**: Compatible JSON API with annotations support

### 3. AnomalyDetector
Statistical analysis and alerting:
- **Statistical Outliers**: Z-score based detection (configurable threshold)
- **Threshold Breaches**: Alert when metrics fall below quality thresholds
- **Quality Degradation**: Detect decreasing trends over time
- **Bias Detection**: Monitor fairness metrics for potential bias
- **Alert History**: Track all alerts with severity levels

## Quick Start

```python
from ethics_evaluation_metrics import quick_evaluate
from ethics_monitoring_dashboard import create_monitoring_system

# Initialize monitoring system
aggregator, detector, renderer = create_monitoring_system()

# In your evaluation loop
decision = {
    'decision': 'Your ethical decision text',
    'primary_philosophy': 'utilitarianism',
    'confidence': 0.85
}

result = quick_evaluate(decision)
aggregator.ingest_evaluation(result)

# Display dashboard
print(renderer.render_terminal(aggregator, detector))

# Check for critical alerts
alerts = detector.detect_anomalies(aggregator)
for alert in alerts:
    if alert.severity == AlertSeverity.CRITICAL:
        print(f"CRITICAL: {alert.message}")
```

## Architecture

### Data Flow
```
Ethics Evaluation → MetricsAggregator → Time Series Storage
                                      ↓
                              AnomalyDetector → Alerts
                                      ↓
                              DashboardRenderer → Multiple Formats
                                                   ├─ Terminal
                                                   ├─ JSON
                                                   ├─ Prometheus
                                                   └─ Grafana
```

### Key Classes

#### MetricsAggregator
```python
aggregator = MetricsAggregator(max_data_points=10000)
aggregator.ingest_evaluation(ethics_result)
metrics = aggregator.get_aggregated_metrics(TimeWindow.HOURLY)
history = aggregator.get_metric_history('ethics_fairness_score')
```

#### AnomalyDetector
```python
detector = AnomalyDetector(
    outlier_threshold=3.0,     # Z-score threshold
    quality_threshold=0.6,     # Minimum quality
    bias_threshold=0.15        # Maximum bias
)
alerts = detector.detect_anomalies(aggregator, TimeWindow.HOURLY)
recent = detector.get_recent_alerts(severity=AlertSeverity.CRITICAL, hours=24)
```

#### DashboardRenderer
```python
renderer = DashboardRenderer(width=80)
terminal_view = renderer.render_terminal(aggregator, detector)
json_data = renderer.export_json(aggregator, detector)
prometheus = renderer.export_prometheus(aggregator)
grafana = renderer.export_grafana_json(aggregator, detector)
```

## Metrics Tracked

### Primary Dimensions
- **Decision Quality**: Outcome satisfaction, ethical alignment, feasibility, impact
- **Consistency**: Intra-case, inter-case, philosophy, temporal
- **Fairness**: Demographic parity, equalized odds, individual fairness
- **Alignment**: Principle adherence, constitutional compliance, value alignment, constraints
- **Transparency**: Explanation completeness, reasoning clarity, justification robustness

### Time Windows
- **Real-time**: Last 5 minutes
- **Hourly**: Last 60 minutes
- **Daily**: Last 24 hours
- **Weekly**: Last 7 days
- **Monthly**: Last 30 days

## Export Formats

### Terminal Dashboard
```
================================================================================
                          ETHICS MONITORING DASHBOARD                           
================================================================================

Time: 2026-01-28 22:43:39
Window: hourly (50 evaluations)

────────────────────────────────────────────────────────────────────────────────
OVERALL ETHICS SCORE
────────────────────────────────────────────────────────────────────────────────
  [▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓················] 0.740
```

### Prometheus Format
```
# HELP ethics_overall_score Overall ethics evaluation score
# TYPE ethics_overall_score gauge
ethics_overall_score{window="hourly"} 0.740 1737585819000

# HELP ethics_dimension_score Ethics score by dimension
# TYPE ethics_dimension_score gauge
ethics_dimension_score{dimension="fairness",window="hourly"} 0.726 1737585819000
```

### JSON Export
```json
{
  "timestamp": "2026-01-28T22:43:39",
  "windows": {
    "hourly": {
      "overall_score": 0.740,
      "dimension_scores": {
        "decision_quality": 0.743,
        "fairness": 0.726
      },
      "trends": {
        "fairness": "stable"
      }
    }
  }
}
```

## Anomaly Types

1. **Statistical Outlier**: Metric value significantly deviates from historical mean
2. **Threshold Breach**: Metric falls below acceptable quality threshold
3. **Quality Degradation**: Metric shows declining trend over time
4. **Bias Detection**: Fairness metrics indicate potential bias
5. **Consistency Drop**: Large drop in consistency metrics

## Alert Severity

- **INFO**: Informational, no action required
- **WARNING**: Attention needed, monitor closely
- **CRITICAL**: Immediate action required

## Integration with Existing Systems

### Grafana
1. Export metrics using `export_grafana_json()`
2. Configure Grafana to poll the JSON endpoint
3. Create dashboards using the time series data
4. View alerts as annotations on graphs

### Prometheus
1. Export metrics using `export_prometheus()`
2. Configure Prometheus to scrape the endpoint
3. Set up alerting rules based on thresholds
4. Visualize in Grafana or other tools

### Custom Dashboards
1. Use `export_json()` for structured data
2. Build custom web dashboards
3. Integrate with existing monitoring systems
4. Create custom visualizations

## Configuration

```python
# Custom thresholds
aggregator, detector, renderer = create_monitoring_system(
    outlier_threshold=4.0,      # More lenient outlier detection
    quality_threshold=0.7,      # Higher quality requirement
    bias_threshold=0.10         # Stricter bias tolerance
)

# Custom data retention
aggregator = MetricsAggregator(max_data_points=50000)

# Custom dashboard width
renderer = DashboardRenderer(width=120)
```

## Best Practices

1. **Regular Monitoring**: Check dashboard at least hourly during active use
2. **Alert Response**: Establish procedures for each severity level
3. **Trend Analysis**: Review weekly trends to identify long-term patterns
4. **Threshold Tuning**: Adjust thresholds based on your specific use case
5. **Data Retention**: Balance memory usage with historical depth needs

## Performance Considerations

- **Memory Usage**: ~100 bytes per data point × metrics tracked
- **Storage**: Deque-based storage is memory-efficient
- **CPU**: Minimal overhead for ingestion and aggregation
- **Export**: JSON/Prometheus exports are fast (< 10ms typically)

## Testing

Run the included validation test:
```bash
python3 ethics_monitoring_dashboard.py
```

This demonstrates:
- Mock data generation
- Dashboard rendering
- Multiple export formats
- Anomaly detection
- Trend analysis

## Dependencies

- Python 3.8+
- Standard library only (no external dependencies)
- Integrates with `ethics_evaluation_metrics.py`

## License

MIT License - Same as ThemisDB project

## Author

ThemisDB Ethics AI Framework Team
