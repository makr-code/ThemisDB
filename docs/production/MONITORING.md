# Monitoring & Observability Guide

**Version:** 1.8.0-rc1  
**Last Updated:** April 2026  
**Target Audience:** DevOps Engineers, SREs

## Table of Contents

1. [Overview](#overview)
2. [Key Metrics](#key-metrics)
3. [Prometheus Integration](#prometheus-integration)
4. [Grafana Dashboards](#grafana-dashboards)
5. [Alerting Rules](#alerting-rules)
6. [Log Aggregation](#log-aggregation)
7. [Tracing](#tracing)
8. [Custom Metrics](#custom-metrics)

---

## Overview

Comprehensive monitoring is essential for production GPU workloads. This guide covers metrics collection, visualization, and alerting for ThemisDB GPU training and inference operations.

### Monitoring Stack

```
ThemisDB → Prometheus → Grafana → Alertmanager
              ↓
         Long-term Storage (optional)
```

---

## Key Metrics

### GPU Utilization Metrics

**Essential GPU Metrics:**

```yaml
# GPU utilization percentage (target: >85%)
themis_gpu_utilization{device="0"}

# GPU memory usage in MB
themis_gpu_memory_used{device="0"}
themis_gpu_memory_total{device="0"}
themis_gpu_memory_free{device="0"}

# GPU temperature in Celsius
themis_gpu_temperature{device="0"}

# GPU power usage in Watts
themis_gpu_power_usage{device="0"}
themis_gpu_power_limit{device="0"}

# GPU clock speeds in MHz
themis_gpu_sm_clock{device="0"}
themis_gpu_memory_clock{device="0"}
```

### Training Metrics

```yaml
# Training throughput
themis_training_samples_per_second
themis_training_tokens_per_second

# Training loss
themis_training_loss{model="llama-2-7b"}
themis_training_perplexity{model="llama-2-7b"}

# Learning rate
themis_training_learning_rate

# Gradient norms
themis_training_gradient_norm

# Training progress
themis_training_steps_completed
themis_training_epochs_completed
themis_training_time_elapsed_seconds
```

### Inference Metrics

```yaml
# Request latency (milliseconds)
themis_inference_latency_ms{quantile="0.5"}  # P50
themis_inference_latency_ms{quantile="0.95"} # P95
themis_inference_latency_ms{quantile="0.99"} # P99

# First token latency
themis_inference_first_token_latency_ms{quantile="0.95"}

# Throughput
themis_inference_requests_per_second
themis_inference_tokens_per_second

# Queue depth
themis_inference_queue_length
themis_inference_queue_wait_time_ms
```

### Memory Metrics

```yaml
# VRAM usage breakdown
themis_vram_model_weights_mb
themis_vram_activations_mb
themis_vram_gradients_mb
themis_vram_optimizer_mb
themis_vram_kv_cache_mb

# System memory
themis_system_memory_used_mb
themis_system_memory_total_mb
```

### Multi-GPU Metrics

```yaml
# Per-GPU metrics
themis_multigpu_utilization{device="0"}
themis_multigpu_utilization{device="1"}

# Inter-GPU communication
themis_nccl_bandwidth_gbps{src="0",dst="1"}
themis_nccl_latency_us{src="0",dst="1"}

# Load balancing
themis_multigpu_samples_processed{device="0"}
```

---

## Prometheus Integration

### Installation

```bash
# Install Prometheus
wget https://github.com/prometheus/prometheus/releases/download/v2.48.0/prometheus-2.48.0.linux-amd64.tar.gz
tar xvf prometheus-2.48.0.linux-amd64.tar.gz
cd prometheus-2.48.0.linux-amd64
```

### Configuration

Create `prometheus.yml`:

```yaml
global:
  scrape_interval: 15s
  evaluation_interval: 15s
  external_labels:
    cluster: 'themisdb-production'

scrape_configs:
  # ThemisDB metrics
  - job_name: 'themisdb'
    static_configs:
      - targets: ['localhost:4318']
        labels:
          instance: 'themisdb-node-1'
    
    metric_relabel_configs:
      # Drop high-cardinality labels if needed
      - source_labels: [__name__]
        regex: 'themis_debug_.*'
        action: drop

  # NVIDIA DCGM exporter (detailed GPU metrics)
  - job_name: 'nvidia-dcgm'
    static_configs:
      - targets: ['localhost:9400']
    
  # Node exporter (system metrics)
  - job_name: 'node'
    static_configs:
      - targets: ['localhost:9100']

# Alerting
alerting:
  alertmanagers:
    - static_configs:
        - targets: ['localhost:9093']

rule_files:
  - 'alerts.yml'

# Remote storage (optional)
remote_write:
  - url: 'http://victoriametrics:8428/api/v1/write'
```

### Enable Metrics in ThemisDB

Edit `config.yaml`:

```yaml
metrics:
  enabled: true
  port: 4318
  path: /metrics
  
  # Export format
  format: prometheus  # or: opentelemetry
  
  # Metrics collection intervals
  gpu_metrics_interval: 1s
  training_metrics_interval: 1s
  inference_metrics_interval: 100ms
  
  # Additional exporters
  exporters:
    - type: prometheus
      endpoint: http://localhost:4318/metrics
    
    - type: statsd
      endpoint: localhost:8125
```

### Start Prometheus

```bash
./prometheus --config.file=prometheus.yml &

# Verify
curl http://localhost:9090/api/v1/targets
```

### NVIDIA DCGM Exporter

For detailed GPU metrics:

```bash
# Docker deployment
docker run -d \
  --name dcgm-exporter \
  --gpus all \
  --rm \
  -p 9400:9400 \
  nvcr.io/nvidia/k8s/dcgm-exporter:3.1.8-3.1.5-ubuntu20.04

# Kubernetes deployment
kubectl apply -f https://raw.githubusercontent.com/NVIDIA/dcgm-exporter/main/dcgm-exporter.yaml
```

---

## Grafana Dashboards

### Installation

```bash
# Install Grafana
sudo apt-get install -y adduser libfontconfig1
wget https://dl.grafana.com/oss/release/grafana_10.2.3_amd64.deb
sudo dpkg -i grafana_10.2.3_amd64.deb

# Start service
sudo systemctl enable grafana-server
sudo systemctl start grafana-server

# Access at http://localhost:3000
# Default credentials: admin/admin
```

### Add Prometheus Data Source

1. Navigate to **Configuration** → **Data Sources**
2. Click **Add data source**
3. Select **Prometheus**
4. Set URL: `http://localhost:9090`
5. Click **Save & Test**

### Pre-built Dashboards

#### GPU Overview Dashboard

Create new dashboard with following panels:

**Panel 1: GPU Utilization**

```promql
# Query
themis_gpu_utilization{device=~"$device"}

# Panel settings
- Type: Time series
- Unit: Percent (0-100)
- Min: 0, Max: 100
- Thresholds: Yellow at 50, Red at 90
```

**Panel 2: GPU Memory Usage**

```promql
# Query
100 * (themis_gpu_memory_used / themis_gpu_memory_total)

# Panel settings
- Type: Gauge
- Unit: Percent (0-100)
- Thresholds: Green 0-70, Yellow 70-85, Red 85-100
```

**Panel 3: GPU Temperature**

```promql
# Query
themis_gpu_temperature{device=~"$device"}

# Panel settings
- Type: Time series
- Unit: Celsius
- Thresholds: Green <75, Yellow 75-85, Red >85
```

**Panel 4: Training Throughput**

```promql
# Query
rate(themis_training_samples_per_second[5m])

# Panel settings
- Type: Stat
- Unit: samples/sec
```

#### Training Dashboard

**Panel 1: Training Loss**

```promql
themis_training_loss{model="$model"}
```

**Panel 2: Learning Rate**

```promql
themis_training_learning_rate
```

**Panel 3: Gradient Norm**

```promql
themis_training_gradient_norm
```

**Panel 4: Training Progress**

```promql
themis_training_epochs_completed
```

#### Inference Dashboard

**Panel 1: Request Latency**

```promql
# P50, P95, P99
histogram_quantile(0.50, rate(themis_inference_latency_ms_bucket[5m]))
histogram_quantile(0.95, rate(themis_inference_latency_ms_bucket[5m]))
histogram_quantile(0.99, rate(themis_inference_latency_ms_bucket[5m]))
```

**Panel 2: Throughput**

```promql
rate(themis_inference_requests_total[5m])
```

**Panel 3: Queue Length**

```promql
themis_inference_queue_length
```

### Import Dashboard Templates

```bash
# Download dashboard JSON
curl -o themisdb-gpu-dashboard.json \
  https://raw.githubusercontent.com/makr-code/ThemisDB/main/grafana/dashboards/gpu-overview.json

# Import via Grafana UI or API
curl -X POST http://admin:admin@localhost:3000/api/dashboards/db \
  -H "Content-Type: application/json" \
  -d @themisdb-gpu-dashboard.json
```

---

## Alerting Rules

Create `alerts.yml`:

```yaml
groups:
  - name: gpu_alerts
    interval: 30s
    rules:
      # Critical: GPU down
      - alert: GPUDown
        expr: up{job="themisdb"} == 0
        for: 1m
        labels:
          severity: critical
        annotations:
          summary: "GPU node {{ $labels.instance }} is down"
          description: "GPU monitoring for {{ $labels.instance }} has been down for more than 1 minute"

      # Critical: GPU utilization too low
      - alert: GPUUtilizationLow
        expr: themis_gpu_utilization < 20
        for: 5m
        labels:
          severity: warning
        annotations:
          summary: "Low GPU utilization on {{ $labels.device }}"
          description: "GPU {{ $labels.device }} utilization is {{ $value }}% (< 20%)"

      # Critical: GPU memory exhausted
      - alert: GPUMemoryHigh
        expr: (themis_gpu_memory_used / themis_gpu_memory_total) > 0.95
        for: 2m
        labels:
          severity: critical
        annotations:
          summary: "GPU {{ $labels.device }} memory critical"
          description: "GPU memory usage is {{ $value | humanizePercentage }}"

      # Warning: GPU temperature high
      - alert: GPUTemperatureHigh
        expr: themis_gpu_temperature > 85
        for: 5m
        labels:
          severity: warning
        annotations:
          summary: "GPU {{ $labels.device }} temperature high"
          description: "Temperature is {{ $value }}°C (threshold: 85°C)"

      # Critical: GPU temperature critical
      - alert: GPUTemperatureCritical
        expr: themis_gpu_temperature > 90
        for: 2m
        labels:
          severity: critical
        annotations:
          summary: "GPU {{ $labels.device }} overheating"
          description: "Temperature is {{ $value }}°C (critical threshold: 90°C)"

  - name: training_alerts
    interval: 30s
    rules:
      # Warning: Training loss not decreasing
      - alert: TrainingLossStagnant
        expr: |
          (themis_training_loss - themis_training_loss offset 10m) > -0.01
        for: 30m
        labels:
          severity: warning
        annotations:
          summary: "Training loss not decreasing"
          description: "Loss has been stagnant for 30 minutes"

      # Critical: Training loss is NaN
      - alert: TrainingLossNaN
        expr: themis_training_loss != themis_training_loss
        for: 1m
        labels:
          severity: critical
        annotations:
          summary: "Training loss is NaN"
          description: "Training has diverged - immediate action required"

      # Warning: Low throughput
      - alert: TrainingThroughputLow
        expr: themis_training_samples_per_second < 100
        for: 10m
        labels:
          severity: warning
        annotations:
          summary: "Training throughput low"
          description: "Only {{ $value }} samples/sec (expected: >100)"

  - name: inference_alerts
    interval: 30s
    rules:
      # Warning: High latency
      - alert: InferenceLatencyHigh
        expr: |
          histogram_quantile(0.95, rate(themis_inference_latency_ms_bucket[5m])) > 500
        for: 5m
        labels:
          severity: warning
        annotations:
          summary: "Inference P95 latency high"
          description: "P95 latency is {{ $value }}ms (threshold: 500ms)"

      # Warning: Queue building up
      - alert: InferenceQueueHigh
        expr: themis_inference_queue_length > 100
        for: 5m
        labels:
          severity: warning
        annotations:
          summary: "Inference queue growing"
          description: "Queue length is {{ $value }} (threshold: 100)"

      # Critical: Inference errors
      - alert: InferenceErrorRateHigh
        expr: |
          rate(themis_inference_errors_total[5m]) / rate(themis_inference_requests_total[5m]) > 0.05
        for: 5m
        labels:
          severity: critical
        annotations:
          summary: "High inference error rate"
          description: "Error rate is {{ $value | humanizePercentage }}"
```

### Alertmanager Configuration

Create `alertmanager.yml`:

```yaml
global:
  resolve_timeout: 5m
  
  # Slack integration
  slack_api_url: 'https://hooks.slack.com/services/YOUR/WEBHOOK/URL'
  
  # PagerDuty integration
  pagerduty_url: 'https://events.pagerduty.com/v2/enqueue'

route:
  group_by: ['alertname', 'cluster']
  group_wait: 10s
  group_interval: 10s
  repeat_interval: 12h
  receiver: 'default'
  
  routes:
    # Critical alerts to PagerDuty
    - match:
        severity: critical
      receiver: 'pagerduty'
      continue: true
    
    # All alerts to Slack
    - match_re:
        severity: (warning|critical)
      receiver: 'slack'

receivers:
  - name: 'default'
    email_configs:
      - to: 'ops@example.com'
        from: 'alertmanager@example.com'
        smarthost: 'smtp.example.com:587'
  
  - name: 'slack'
    slack_configs:
      - channel: '#themisdb-alerts'
        title: 'ThemisDB Alert'
        text: '{{ range .Alerts }}{{ .Annotations.description }}{{ end }}'
  
  - name: 'pagerduty'
    pagerduty_configs:
      - service_key: 'YOUR_PAGERDUTY_KEY'
        description: '{{ .GroupLabels.alertname }}'
```

---

## Log Aggregation

### Structured Logging

Configure structured logs in `config.yaml`:

```yaml
logging:
  format: json
  level: info
  output: /var/log/themisdb/app.log
  
  fields:
    service: themisdb
    environment: production
    version: 1.4.0
  
  log_sampling:
    enabled: true
    rate: 0.1  # Sample 10% of debug logs
```

### ELK Stack Integration

**Filebeat Configuration** (`filebeat.yml`):

```yaml
filebeat.inputs:
  - type: log
    enabled: true
    paths:
      - /var/log/themisdb/*.log
    json.keys_under_root: true
    json.add_error_key: true

output.elasticsearch:
  hosts: ["localhost:9200"]
  index: "themisdb-%{+yyyy.MM.dd}"

processors:
  - add_host_metadata: ~
  - add_cloud_metadata: ~
```

### Loki Integration

**Promtail Configuration** (`promtail.yml`):

```yaml
server:
  http_listen_port: 9080

positions:
  filename: /tmp/positions.yaml

clients:
  - url: http://localhost:3100/loki/api/v1/push

scrape_configs:
  - job_name: themisdb
    static_configs:
      - targets:
          - localhost
        labels:
          job: themisdb
          __path__: /var/log/themisdb/*.log
```

---

## Tracing

### OpenTelemetry Integration

```yaml
tracing:
  enabled: true
  provider: opentelemetry
  
  exporters:
    - type: jaeger
      endpoint: localhost:14268
    
    - type: zipkin
      endpoint: http://localhost:9411/api/v2/spans
  
  sampling:
    rate: 0.1  # Sample 10% of requests
    
  trace_gpu_operations: true
```

### Jaeger Deployment

```bash
# Docker
docker run -d \
  --name jaeger \
  -p 16686:16686 \
  -p 14268:14268 \
  jaegertracing/all-in-one:latest

# Access UI at http://localhost:16686
```

---

## Custom Metrics

### Application-Level Metrics

```python
from themisdb import metrics

# Counter
metrics.counter('custom_operations_total', labels={'operation': 'lora_merge'})

# Gauge
metrics.gauge('custom_queue_size', value=42)

# Histogram
metrics.histogram('custom_processing_duration_ms', value=123.45)

# Timer context
with metrics.timer('custom_operation_duration'):
    # Your code here
    pass
```

### Export Custom Metrics

```yaml
metrics:
  custom:
    enabled: true
    prefix: themis_custom_
    
    # Define custom metrics
    definitions:
      - name: lora_adapters_loaded
        type: gauge
        help: "Number of LoRA adapters currently loaded"
      
      - name: checkpoint_save_duration_seconds
        type: histogram
        help: "Time taken to save checkpoints"
        buckets: [1, 5, 10, 30, 60, 120]
```

---

## Best Practices

1. **Set up baseline metrics** before optimization
2. **Monitor GPU utilization** - target >85% for training
3. **Track memory trends** to prevent OOM
4. **Alert on anomalies**, not just thresholds
5. **Use distributed tracing** for multi-GPU setups
6. **Archive metrics** for long-term analysis
7. **Test alert rules** before deploying to production

---

## Next Steps

- **Troubleshooting**: Use metrics to debug issues ([TROUBLESHOOTING.md](TROUBLESHOOTING.md))
- **Performance**: Optimize based on metrics ([PERFORMANCE_TUNING.md](PERFORMANCE_TUNING.md))
- **Runbooks**: Create operational procedures ([RUNBOOKS.md](RUNBOOKS.md))

---

**Document Version:** 1.0  
**Last Updated:** April 2026  
**Next Review:** April 2026
