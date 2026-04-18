# Observability & Monitoring Overview

**Version:** 1.5.0-dev  
**Last Updated:** April 2026  
**Status:** Production Ready

---

## Table of Contents

- [Overview](#overview)
- [Logging](#logging)
- [Distributed Tracing](#distributed-tracing)
- [Metrics](#metrics)
- [Alerting](#alerting)
- [Gaps & Future Work](#gaps--future-work)
- [Quick Start](#quick-start)
- [Related Documentation](#related-documentation)

---

## Overview

ThemisDB provides comprehensive observability capabilities across four key pillars:

1. **Logging** - Structured application and audit logging via spdlog
2. **Distributed Tracing** - OpenTelemetry-based request tracing with OTLP export
3. **Metrics** - Prometheus-compatible metrics for performance monitoring
4. **Alerting** - Alert management integration (via external systems)

These capabilities enable deep visibility into database operations, performance monitoring, debugging, and production incident response.

---

## Logging

### Overview

ThemisDB uses a comprehensive logging infrastructure built on [spdlog](https://github.com/gabime/spdlog) for high-performance structured logging.

### Key Components

#### Main Logger
- **Header:** [`include/utils/logger.h`](../../include/utils/logger.h)
- **Implementation:** `src/utils/logger.cpp`
- **Features:**
  - Multiple log levels (TRACE, DEBUG, INFO, WARN, ERROR, CRITICAL)
  - Configurable log file output
  - Runtime level adjustment
  - Custom pattern support

#### Specialized Loggers
- **Audit Logger** (`src/utils/audit_logger.cpp`) - Security and compliance event logging
- **SAGA Logger** (`src/utils/saga_logger.cpp`) - Distributed transaction logging
- **Tracing Logger** - Integrated with OpenTelemetry tracing

### Usage Example

```cpp
#include "utils/logger.h"

// Initialize logging
themis::utils::Logger::init("themis.log", themis::utils::Logger::Level::INFO);

// Log messages
THEMIS_INFO("Server starting on port {}", 8080);
THEMIS_DEBUG("Processing query with {} entities", entity_count);
THEMIS_ERROR("Failed to connect to shard: {}", error_msg);

// Runtime level adjustment
themis::utils::Logger::setLevel(themis::utils::Logger::Level::DEBUG);

// Shutdown
themis::utils::Logger::shutdown();
```

### Configuration

Log levels can be set:
- At initialization time via API
- From configuration files
- Via environment variables
- Dynamically at runtime

### Documentation

- [Logger Header](../../include/utils/logger.h) - API reference
- [Utils Module README](../../src/utils/README.md) - Comprehensive utilities documentation
- [Audit Logger Documentation](../de/src/utils/audit_logger.cpp.md) (German)
- [SAGA Logger Documentation](../de/src/utils/saga_logger.cpp.md) (German)

---

## Distributed Tracing

### Overview

ThemisDB implements distributed tracing using **OpenTelemetry** with OTLP (OpenTelemetry Protocol) export to backends like Jaeger, Grafana Tempo, or any OpenTelemetry-compatible collector.

### Key Components

- **Header:** `include/utils/tracing.h`
- **Implementation:** `src/utils/tracing.cpp`
- **Export Protocol:** OTLP over HTTP
- **Backends Supported:** Jaeger, Grafana Tempo, OpenTelemetry Collector

### Instrumented Components

ThemisDB automatically instruments the following components:

- **Query Engine** - Query execution, AQL parsing, optimization, joins, aggregations
- **Storage Engine** - Put/get/delete operations with key/value size tracking
- **Index Manager** - Index creation, lookups, updates with type-specific attributes
- **Plugin Manager** - Plugin discovery, loading, lifecycle events
- **Security Layer** - Authentication, authorization, MFA operations
- **API Layer** - HTTP request/response flows for all endpoints

### Configuration

```yaml
# config.yaml
tracing:
  enabled: true
  service_name: "themis-server"
  otlp_endpoint: "http://localhost:4318"
```

### Usage Example

```cpp
#include "utils/tracing.h"

// Create a span for an operation
auto span = themis::utils::Tracing::startSpan("QueryEngine.executeAndKeys");

// Add contextual attributes
span->setAttribute("query.table", table_name);
span->setAttribute("query.type", "AQL");
span->setAttribute("query.result_count", result_count);

// Record errors if they occur
try {
    // ... operation ...
    span->setStatus(opentelemetry::trace::StatusCode::kOk);
} catch (const std::exception& e) {
    span->recordError(e.what());
    span->setStatus(opentelemetry::trace::StatusCode::kError);
}

// Span ends automatically when destroyed
```

### Performance Considerations

- **Overhead when enabled:** ~100-200ns per span, asynchronous export
- **Overhead when disabled:** Zero - all tracing calls become no-ops at compile time
- **Compile-time control:** Build without `THEMIS_ENABLE_TRACING` for maximum performance

### Documentation

- **[Tracing Configuration Guide](../tracing-configuration.md)** - Complete setup and integration guide
- [Tracing Implementation](../de/src/utils/tracing.cpp.md) (German)
- [OpenTelemetry Documentation](https://opentelemetry.io/docs/)

---

## Metrics

### Overview

ThemisDB exposes Prometheus-compatible metrics across multiple subsystems. Unlike logging and tracing which have centralized infrastructure, **metrics are currently distributed** across various modules without a single unified metrics module.

### Key Metrics Locations

#### LLM Metrics
- **Headers:** 
  - [`include/llm/grafana_metrics.h`](../../include/llm/grafana_metrics.h) - Grafana/Prometheus integration
  - [`include/llm/lora_framework/lora_metrics.h`](../../include/llm/lora_framework/lora_metrics.h) - LoRA adapter metrics
- **Features:**
  - Inference latency and throughput
  - Cache hit/miss rates (response cache, KV cache)
  - Model loading times
  - GPU utilization
  - Token generation metrics

**Documentation:** [LLM Response Cache Metrics](../LLM_RESPONSE_CACHE_METRICS.md)

#### Sharding Metrics
- **Headers:**
  - [`include/sharding/prometheus_metrics.h`](../../include/sharding/prometheus_metrics.h) - Prometheus export
  - [`include/sharding/metrics_registry.h`](../../include/sharding/metrics_registry.h) - Metrics registration
  - [`include/sharding/operational_metrics.h`](../../include/sharding/operational_metrics.h) - Operational metrics
- **Features:**
  - Shard health and availability
  - Rebalancing operations
  - Gossip protocol metrics
  - Cross-shard query latency

#### Performance Metrics
- **Headers:**
  - [`include/performance/lockfree_metrics_buffer.h`](../../include/performance/lockfree_metrics_buffer.h)
  - [`include/performance/cycle_metrics_config.h`](../../include/performance/cycle_metrics_config.h)
- **Features:**
  - Lock-free metrics collection
  - CPU cycle-accurate timing
  - Low-overhead performance tracking

#### Security Metrics
- **Header:** [`include/security/hsm_security_metrics.h`](../../include/security/hsm_security_metrics.h)
- **Features:**
  - HSM operation metrics
  - Encryption/decryption performance
  - Key rotation tracking

#### Compression Metrics
- **Header:** [`include/utils/compression_metrics.h`](../../include/utils/compression_metrics.h)
- **Features:**
  - Compression ratio tracking
  - Compression/decompression latency

#### Plugin Metrics
- **Header:** [`include/plugins/plugin_metrics.h`](../../include/plugins/plugin_metrics.h)
- **Features:**
  - Plugin load times
  - Plugin operation metrics

### Metrics Endpoints

Metrics are typically exposed via HTTP endpoints:

- **Main Metrics Endpoint:** `http://localhost:9091/metrics` (Prometheus format)
- **Sharding Metrics Handler:** `include/server/sharding_metrics_handler.h`

### Usage Example

```cpp
#include "llm/grafana_metrics.h"
#include "sharding/prometheus_metrics.h"

// LLM metrics collection
themis::llm::monitoring::PrometheusExporter exporter;
themis::llm::monitoring::LLMMetricsCollector metrics(&exporter);

// Record inference metrics
metrics.recordInference(latency_ms, tokens_generated);
metrics.recordCacheHit("response_cache");

// Export metrics (typically done via HTTP endpoint)
std::string metrics_output = exporter.exportMetrics();
```

### Available Metrics (Sample)

```prometheus
# LLM Metrics
llm_inference_requests_total
llm_inference_duration_ms
llm_cache_hits_total{cache_type="response_cache"}
llm_cache_misses_total{cache_type="response_cache"}
llm_cache_size_mb{cache_type="response_cache"}

# Tracing Metrics
themis_trace_spans_total
themis_trace_active_spans
trace_span_duration_ms{span="operation_name"}

# Query Metrics
query_execution_duration_ms
query_result_count

# Storage Metrics
storage_operation_duration_ms{operation="put"}
storage_key_size_bytes
storage_value_size_bytes
```

### Integration with Prometheus

1. **Configure Prometheus** to scrape ThemisDB metrics endpoint:

```yaml
# prometheus.yml
scrape_configs:
  - job_name: 'themisdb'
    static_configs:
      - targets: ['localhost:9091']
    scrape_interval: 15s
```

2. **Deploy Grafana dashboards** for visualization (see `grafana/` directory)

### Documentation

- [LLM Response Cache Metrics](../LLM_RESPONSE_CACHE_METRICS.md) - Detailed cache metrics guide
- [Prometheus Integration Complete](../PROMETHEUS_INTEGRATION_COMPLETE.md)
- [Grafana Metrics Complete](../GRAFANA_METRICS_COMPLETE.md)
- [German Observability Docs](../de/observability/) - Comprehensive observability documentation

---

## Alerting

### Overview

ThemisDB supports alerting through integration with external alert management systems like Prometheus Alertmanager.

### Components

#### Alert Management (Stub Implementation)
- **Location:** Part of observability module (GAP-008)
- **Features:**
  - Alert creation and management interface
  - Severity levels (INFO, WARNING, ERROR, CRITICAL)
  - Alert status tracking (FIRING, RESOLVED, SILENCED)

#### Existing Health Check Systems

ThemisDB provides comprehensive health check infrastructure:

1. **`sharding::HealthCheckSystem`** - Shard/cluster health monitoring
2. **`sharding::HealthMonitor`** - Node health with auto-failover
3. **`server::HealthErrorService`** - HTTP health endpoint (default Port 9090)

> **Note:** If running ThemisDB and Prometheus on the same machine, adjust either the ThemisDB health endpoint port or the Prometheus host port mapping to avoid conflicts (see Quick Start section).

### Alert Configuration Example

```yaml
# prometheus-alerts.yml
groups:
  - name: themisdb_alerts
    rules:
      - alert: HighCacheMissRate
        expr: |
          (
            sum(rate(llm_cache_hits_total[5m]))
            /
            (sum(rate(llm_cache_hits_total[5m])) + sum(rate(llm_cache_misses_total[5m])))
          ) < 0.5
        for: 10m
        labels:
          severity: warning
          component: cache
        annotations:
          summary: "Cache hit rate below 50%"
          description: "Cache hit rate is {{ $value | humanizePercentage }}"

      - alert: HighQueryLatency
        expr: histogram_quantile(0.99, query_execution_duration_ms_bucket) > 1000
        for: 5m
        labels:
          severity: critical
          component: query
        annotations:
          summary: "High query latency detected"
          description: "p99 latency is {{ $value }}ms"
```

### Documentation

- [Observability Alerting](../de/observability/observability_alerting.md) (German)
- [Performance Alerting Config](../performance/PERFORMANCE_ALERTING_CONFIG.md)
- [Performance Regression Detection](../performance/PERFORMANCE_REGRESSION_DETECTION.md)

---

## Gaps & Future Work

### Current Limitations

1. **No Unified Metrics Module**
   - Metrics are distributed across various subsystems
   - No single entry point for all metrics collection
   - Each subsystem has its own metrics infrastructure
   - **Location:** Metrics defined in multiple headers across `include/llm/`, `include/sharding/`, `include/performance/`, etc.

2. **Limited Alerting Infrastructure**
   - Alert management is currently a stub implementation (GAP-008)
   - Relies on external systems (Prometheus Alertmanager)
   - No built-in alert routing or notification

3. **Metrics Discovery**
   - No centralized registry for discovering all available metrics
   - Documentation of metrics is scattered across multiple files
   - Requires exploring multiple headers to understand full metrics surface

### Recommended Improvements

1. **Create Unified Metrics Module** (v1.6+)
   - Central `include/observability/metrics.h` header
   - Unified metrics registry
   - Standard metrics collection interface
   - Automatic metrics discovery

2. **Enhanced Alerting** (v1.6+)
   - Built-in alert rule engine
   - Alert notification plugins (email, Slack, PagerDuty)
   - Alert aggregation and deduplication
   - Alert history and audit trail

3. **Improved Documentation** (Ongoing)
   - Central metrics catalog
   - Metrics naming conventions
   - Standard labels and dimensions
   - Example PromQL queries for common scenarios

### Contributing

Contributions to improve ThemisDB's observability capabilities are welcome! See:
- [Contributing Guide](../../CONTRIBUTING.md)
- [Development Documentation](../de/development/DEVELOPMENT_SUMMARY.md)

---

## Quick Start

### 1. Enable Logging

```cpp
#include "utils/logger.h"

// Initialize with INFO level
themis::utils::Logger::init("themis.log", themis::utils::Logger::Level::INFO);

// Use logging macros
THEMIS_INFO("Application started");
```

### 2. Enable Tracing

```yaml
# config.yaml
tracing:
  enabled: true
  service_name: "my-themis-instance"
  otlp_endpoint: "http://localhost:4318"
```

```bash
# Start Jaeger (Docker)
docker run -d --name jaeger \
  -e COLLECTOR_OTLP_ENABLED=true \
  -p 16686:16686 \
  -p 4318:4318 \
  jaegertracing/all-in-one:latest

# Access Jaeger UI at http://localhost:16686
# Visit this URL in your browser
```

### 3. Scrape Metrics with Prometheus

```yaml
# prometheus.yml
scrape_configs:
  - job_name: 'themisdb'
    static_configs:
      - targets: ['localhost:9091']
```

```bash
# Start Prometheus
# Note: Using port 9092 on host to avoid conflict with ThemisDB health endpoint (port 9090)
docker run -d --name prometheus \
  -p 9092:9090 \
  -v $(pwd)/prometheus.yml:/etc/prometheus/prometheus.yml \
  prom/prometheus

# Access Prometheus UI at http://localhost:9092
# Visit this URL in your browser
```

### 4. Visualize with Grafana

```bash
# Start Grafana
docker run -d --name grafana \
  -p 3000:3000 \
  grafana/grafana

# Access Grafana at http://localhost:3000
# Visit this URL in your browser (default credentials: admin/admin)

# Add Prometheus data source
# URL: http://prometheus:9090 (container-to-container)
# Or http://host.docker.internal:9092 (if Prometheus is on host)

# Import ThemisDB dashboards from grafana/ directory
```

---

## Related Documentation

### Core Documentation
- **[Tracing Configuration Guide](../tracing-configuration.md)** - Complete tracing setup
- **[LLM Response Cache Metrics](../LLM_RESPONSE_CACHE_METRICS.md)** - Cache metrics integration
- **[Utils Module README](../../src/utils/README.md)** - Logging and utility components

### German Language Documentation
- **[Observability Overview (DE)](../de/observability/README.md)** - Comprehensive German documentation
- **[Observability Metrics (DE)](../de/observability/observability_metrics.md)**
- **[Observability Tracing (DE)](../de/observability/observability_tracing.md)**
- **[Observability Prometheus (DE)](../de/observability/observability_prometheus.md)**
- **[Observability OpenTelemetry (DE)](../de/observability/observability_opentelemetry.md)**
- **[Observability Alerting (DE)](../de/observability/observability_alerting.md)**

### Integration Guides
- **[Prometheus Integration](../PROMETHEUS_INTEGRATION_COMPLETE.md)**
- **[Grafana Metrics Integration](../GRAFANA_METRICS_COMPLETE.md)**
- **[LLM Grafana Metrics Integration](../LLM_GRAFANA_METRICS_INTEGRATION.md)**

### Performance & Monitoring
- **[Performance Alerting Config](../performance/PERFORMANCE_ALERTING_CONFIG.md)**
- **[Performance Regression Detection](../performance/PERFORMANCE_REGRESSION_DETECTION.md)**
- **[Performance Regression Quick Reference](../performance/PERFORMANCE_REGRESSION_QUICK_REFERENCE.md)**

### API & Integration
- **[Operations Runbook](../de/guides/guides_operations_runbook.md)** - Daily operations guide
- **[Server Module Documentation](../de/server/README.md)** - HTTP server and API handlers

### Source Code References
- **Logger:** [`include/utils/logger.h`](../../include/utils/logger.h)
- **Tracing:** `include/utils/tracing.h`
- **LLM Metrics:** [`include/llm/grafana_metrics.h`](../../include/llm/grafana_metrics.h)
- **Sharding Metrics:** [`include/sharding/prometheus_metrics.h`](../../include/sharding/prometheus_metrics.h)
- **Performance Metrics:** [`include/performance/lockfree_metrics_buffer.h`](../../include/performance/lockfree_metrics_buffer.h)

### Examples
- **[LLM Metrics Example](../../examples/example_llm_metrics.cpp)**
- **[Benchmarks Metrics Collector](../../benchmarks/bench_metrics_collector.cpp)**

---

## Support

For questions or issues related to observability:

- **GitHub Issues:** [ThemisDB Issues](https://github.com/makr-code/ThemisDB/issues)
- **Documentation:** [Main README](../../README.md)
- **Security:** [Security Policy](../../SECURITY.md)

---

**Version:** 1.5.0-dev  
**License:** See [LICENSE](../../LICENSE)  
**Last Updated:** April 2026
