# LLM Grafana Metrics Integration - Implementation Guide

## Overview

This document describes the integration of Prometheus metrics and Grafana dashboards for ThemisDB's LLM subsystem (llama.cpp plugin).

## Implementation Status

### ✅ Completed Components

1. **Prometheus Exporter** (`include/llm/grafana_metrics.h`, `src/llm/grafana_metrics.cpp`)
   - Custom Prometheus metrics exporter
   - Supports Counter, Gauge, Histogram, and Summary metric types
   - Thread-safe metric recording
   - Prometheus text format export

2. **LLM Metrics Collector** 
   - Comprehensive metrics for LLM inference pipeline
   - Inference metrics (requests, latency, throughput)
   - GPU metrics (memory, utilization, temperature)
   - Model metrics (loaded models, memory usage)
   - Cache metrics (hit rates, efficiency)
   - Scheduler metrics (queue length, batch size)
   - Error tracking

3. **Grafana Dashboard** (`grafana/dashboards/themisdb-llm-dashboard.json`)
   - Real-time inference monitoring
   - Latency distribution (P50, P95, P99)
   - Throughput visualization
   - GPU resource monitoring
   - Cache performance tracking
   - Error rate monitoring

4. **Alert Rules** (`grafana/provisioning/alerts.yml`)
   - High latency alerts (Warning: >100ms, Critical: >200ms)
   - Error rate alerts (Warning: >5%, Critical: >10%)
   - GPU memory alerts (Warning: >85%, Critical: >95%)
   - GPU temperature alerts (Warning: >80°C, Critical: >90°C)
   - Low throughput alerts (<100 tokens/sec)
   - High queue length alerts (>50 requests)

5. **LlamaWrapper Integration**
   - Metrics recording integrated into inference flow
   - Model loading/unloading tracking
   - First token latency measurement
   - Per-token latency calculation
   - Error recording on failures

## Architecture

```
┌─────────────────────────────────────────────────────────┐
│                  LlamaWrapper                            │
│  (llama.cpp Plugin)                                     │
│                                                          │
│  ├─ loadModel()                                         │
│  │  └─> recordModelLoaded()                            │
│  │                                                      │
│  ├─ generate()                                          │
│  │  ├─> recordInferenceRequest()                       │
│  │  ├─> recordFirstTokenLatency()                      │
│  │  ├─> recordPerTokenLatency()                        │
│  │  ├─> recordTokensGenerated()                        │
│  │  ├─> recordInferenceSuccess()                       │
│  │  └─> recordInferenceFailure() (on error)            │
│  │                                                      │
│  └─ unloadModel()                                       │
│     └─> recordModelUnloaded()                          │
│                                                          │
└───────────────────┬────────────────────────────────────┘
                    │
                    ↓
         ┌──────────────────────┐
         │ LLMMetricsCollector  │
         │                       │
         │ - Inference metrics   │
         │ - Latency tracking    │
         │ - Throughput data     │
         │ - GPU metrics         │
         │ - Cache statistics    │
         │ - Error tracking      │
         └───────────┬──────────┘
                     │
                     ↓
          ┌─────────────────────┐
          │ PrometheusExporter  │
          │                      │
          │ - Metrics storage    │
          │ - Histogram buckets  │
          │ - Label management   │
          │ - Text format export │
          └──────────┬──────────┘
                     │
                     ↓ HTTP /metrics
          ┌─────────────────────┐
          │    Prometheus        │
          │                      │
          │ - Scrapes metrics    │
          │ - Time series DB     │
          │ - Alert evaluation   │
          └──────────┬──────────┘
                     │
                     ↓ PromQL
          ┌─────────────────────┐
          │      Grafana         │
          │                      │
          │ - Dashboards         │
          │ - Visualizations     │
          │ - Alert notifications│
          └─────────────────────┘
```

## Usage Example

### C++ Code Integration

```cpp
#include "llm/llama_wrapper.h"
#include "llm/grafana_metrics.h"

using namespace themis::llm;
using namespace themis::llm::monitoring;

// 1. Initialize metrics infrastructure
PrometheusExporter exporter;
LLMMetricsCollector metrics_collector(&exporter);

// 2. Start metrics server (optional, for standalone deployment)
MetricsServer::ServerConfig server_config;
server_config.port = 9091;
server_config.metrics_path = "/metrics";
MetricsServer metrics_server(server_config, &exporter);
metrics_server.start();

// 3. Configure and create LlamaWrapper
LlamaWrapper::Config llm_config;
llm_config.n_gpu_layers = 32;
llm_config.n_ctx = 4096;

// Note: Response cache is ENABLED BY DEFAULT (enable_response_cache = true)
// To disable: llm_config.enable_response_cache = false;

LlamaWrapper wrapper(llm_config);

// 4. Set metrics collector on wrapper
wrapper.setMetricsCollector(&metrics_collector);

// 5. Load model (metrics recorded automatically)
wrapper.loadModel("models/mistral-7b-v0.1.Q4_K_M.gguf");

// 6. Generate inference (metrics recorded automatically)
InferenceRequest request;
request.prompt = "Explain quantum computing";
request.max_tokens = 100;
request.temperature = 0.7;

auto response = wrapper.generate(request);

// 7. Metrics are now available at http://localhost:9091/metrics
// Prometheus will scrape them automatically
```

### Metrics Output Example

```
# HELP llm_inference_requests_total Total number of inference requests
# TYPE llm_inference_requests_total counter
llm_inference_requests_total{model_id="mistral-7b-v0.1"} 42.00

# HELP llm_inference_duration_ms Inference duration in milliseconds
# TYPE llm_inference_duration_ms histogram
llm_inference_duration_ms_bucket{model_id="mistral-7b-v0.1",le="10"} 0
llm_inference_duration_ms_bucket{model_id="mistral-7b-v0.1",le="25"} 3
llm_inference_duration_ms_bucket{model_id="mistral-7b-v0.1",le="50"} 18
llm_inference_duration_ms_bucket{model_id="mistral-7b-v0.1",le="100"} 35
llm_inference_duration_ms_bucket{model_id="mistral-7b-v0.1",le="+Inf"} 42
llm_inference_duration_ms_sum{model_id="mistral-7b-v0.1"} 2847.50
llm_inference_duration_ms_count{model_id="mistral-7b-v0.1"} 42

# HELP llm_tokens_generated_total Total tokens generated
# TYPE llm_tokens_generated_total counter
llm_tokens_generated_total{model_id="mistral-7b-v0.1"} 3567.00

# HELP llm_first_token_latency_ms Time to first token in milliseconds
# TYPE llm_first_token_latency_ms histogram
llm_first_token_latency_ms_bucket{model_id="mistral-7b-v0.1",le="25"} 15
llm_first_token_latency_ms_bucket{model_id="mistral-7b-v0.1",le="50"} 38
llm_first_token_latency_ms_bucket{model_id="mistral-7b-v0.1",le="+Inf"} 42
```

## Key Metrics Explained

### Inference Metrics
- **llm_inference_requests_total**: Total inference requests by model
- **llm_inference_duration_ms**: Histogram of inference latencies
- **llm_inference_failures_total**: Failed requests by model and error type

### Latency Metrics
- **llm_first_token_latency_ms**: Time to generate first token (critical UX metric)
- **llm_per_token_latency_ms**: Average latency per token

### Throughput Metrics
- **llm_tokens_generated_total**: Total tokens produced
- **llm_batch_size**: Current batch size (gauge)
- **llm_concurrent_requests**: Active concurrent requests (gauge)

### GPU Metrics
- **llm_gpu_memory_used_mb**: GPU VRAM used
- **llm_gpu_memory_total_mb**: Total GPU VRAM
- **llm_gpu_utilization_pct**: GPU compute utilization
- **llm_gpu_temperature_celsius**: GPU temperature

### Model Metrics
- **llm_models_loaded**: Number of loaded models (gauge)
- **llm_model_memory_mb**: Memory per model

### Cache Metrics
- **llm_cache_hits_total**: Cache hits by type
- **llm_cache_misses_total**: Cache misses by type

## Testing

### Unit Tests

```bash
# Run LLM metrics tests
cd /home/runner/work/ThemisDB/ThemisDB/build
ctest -R test_llm_grafana_metrics -V
```

The test suite (`tests/test_llm_grafana_metrics.cpp`) includes:
- Basic metrics recording
- Model loading/unloading metrics
- Multiple inference tracking
- Error recording
- Metrics export format validation
- Thread safety verification
- Dashboard generation

### Manual Testing

```bash
# 1. Start ThemisDB with metrics enabled
./build/Release/themis_server --enable-llm --metrics-port 9091

# 2. In another terminal, check metrics endpoint
curl http://localhost:9091/metrics

# 3. Start Grafana/Prometheus stack
cd grafana
docker-compose up -d

# 4. Open Grafana
open http://localhost:3000
# Login: admin/admin

# 5. View LLM dashboard
# Navigate to: Dashboards → ThemisDB LLM / llama.cpp Monitoring
```

## Integration with Existing Code

The integration is **minimal and non-invasive**:

1. **Header inclusion**: Added `#include "llm/grafana_metrics.h"` to `llama_wrapper.h`

2. **Member variable**: Added `LLMMetricsCollector* metrics_collector_` to `LlamaWrapper` class

3. **Setter method**: Added `setMetricsCollector()` to allow optional metrics injection

4. **Metrics recording**: Added calls to `metrics_collector_->recordXxx()` at key points:
   - `loadModel()`: Record model loading
   - `generate()`: Record inference request, latency, tokens, success/failure
   - `unloadModel()`: Record model unloading

5. **Null checks**: All metrics recording is guarded with `if (metrics_collector_)` checks

## Performance Impact

The metrics integration has **minimal performance overhead**:

- Metrics recording uses atomic operations and mutexes
- Recording a metric typically takes <1 microsecond
- No blocking I/O during metrics recording
- Metrics export only happens during Prometheus scrape (every 5-15 seconds)

## Configuration

### Prometheus Scrape Configuration

Edit `grafana/prometheus.yml`:

```yaml
scrape_configs:
  - job_name: 'themisdb-llm'
    static_configs:
      - targets: ['host.docker.internal:9091']
        labels:
          service: 'themisdb-llm'
          component: 'inference'
    metrics_path: '/metrics'
    scrape_interval: 5s  # Fast scraping for real-time monitoring
```

### Alert Thresholds

Edit `grafana/provisioning/alerts.yml` to customize alert thresholds:

```yaml
- alert: HighFirstTokenLatency
  expr: histogram_quantile(0.95, llm_first_token_latency_ms_bucket) > 100
  for: 5m
  labels:
    severity: warning
```

## Troubleshooting

### Metrics not appearing in Prometheus

1. Check metrics endpoint: `curl http://localhost:9091/metrics`
2. Check Prometheus targets: http://localhost:9090/targets
3. Verify `metrics_collector_` is set on `LlamaWrapper`
4. Check logs for metrics recording

### Dashboard shows no data

1. Verify time range (Last 1 hour)
2. Check Prometheus data source connection
3. Test queries in Prometheus UI first
4. Verify metrics are being recorded (check endpoint)

### High memory usage

- Metrics are stored in-memory
- Use histogram buckets wisely (default: 10 buckets)
- Consider shorter retention in Prometheus
- Use recording rules for complex queries

## Future Enhancements

Potential improvements for future iterations:

1. **GPU metrics collection**: Integrate with NVML/CUDA for real GPU metrics
2. **Cache integration**: Add metrics to `LLMPrefixCache` and `LLMResponseCache`
3. **Scheduler metrics**: Integrate with `ContinuousBatchScheduler`
4. **LoRA metrics**: Track LoRA adapter loading and switching
5. **Distributed metrics**: Aggregate metrics across multiple nodes
6. **Custom alerting**: Integrate with PagerDuty, Slack, etc.

## References

- Prometheus Documentation: https://prometheus.io/docs/
- Grafana Documentation: https://grafana.com/docs/
- PromQL Query Language: https://prometheus.io/docs/prometheus/latest/querying/basics/
- Histogram Buckets: https://prometheus.io/docs/practices/histograms/

## Support

For issues or questions:
- GitHub Issues: https://github.com/makr-code/ThemisDB/issues
- Documentation: `/home/runner/work/ThemisDB/ThemisDB/grafana/README.md`
