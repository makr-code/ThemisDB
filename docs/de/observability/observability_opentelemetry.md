# OpenTelemetry Integration für ThemisDB

**Version:** 1.0.0  
**Stand:** 6. April 2026  
**Kategorie:** Observability  
**Status:** ✅ Produktionsreif

---

## 📑 Inhaltsverzeichnis

- [Übersicht](#übersicht)
- [Installation und Setup](#installation-und-setup)
- [Traces](#traces)
- [Metrics](#metrics)
- [Logs](#logs)
- [Backends](#backends)
- [Best Practices](#best-practices)
- [Troubleshooting](#troubleshooting)

---

## Übersicht

ThemisDB bietet native Integration mit OpenTelemetry für umfassendes Observability. Dies ermöglicht einheitliches Monitoring, Tracing und Logging über alle Komponenten hinweg.

### Unterstützte Signale

| Signal | Status | Protokoll | Beschreibung |
|--------|--------|-----------|--------------|
| **Traces** | ✅ Vollständig | OTLP/gRPC, OTLP/HTTP | Distributed Tracing |
| **Metrics** | ✅ Vollständig | OTLP/gRPC, Prometheus | Performance Metrics |
| **Logs** | ✅ Vollständig | OTLP/gRPC, OTLP/HTTP | Structured Logging |

### Unterstützte Backends

- **Jaeger** - Distributed Tracing
- **Zipkin** - Distributed Tracing
- **Grafana Tempo** - Distributed Tracing
- **Prometheus** - Metrics Collection
- **Grafana Loki** - Log Aggregation
- **Elastic APM** - Full Observability Stack
- **Datadog** - Commercial APM
- **New Relic** - Commercial APM

---

## Installation und Setup

### OpenTelemetry Collector

```yaml
# otel-collector-config.yaml
receivers:
  otlp:
    protocols:
      grpc:
        endpoint: 0.0.0.0:4317
      http:
        endpoint: 0.0.0.0:4318

processors:
  batch:
    timeout: 10s
    send_batch_size: 1024
  
  resource:
    attributes:
      - key: service.name
        value: themisdb
        action: upsert
      - key: deployment.environment
        value: production
        action: upsert

exporters:
  # Jaeger für Traces
  jaeger:
    endpoint: jaeger:14250
    tls:
      insecure: true
  
  # Prometheus für Metrics
  prometheus:
    endpoint: 0.0.0.0:8889
  
  # Loki für Logs
  loki:
    endpoint: http://loki:3100/loki/api/v1/push

service:
  pipelines:
    traces:
      receivers: [otlp]
      processors: [batch, resource]
      exporters: [jaeger]
    
    metrics:
      receivers: [otlp]
      processors: [batch, resource]
      exporters: [prometheus]
    
    logs:
      receivers: [otlp]
      processors: [batch, resource]
      exporters: [loki]
```

### ThemisDB Konfiguration

```yaml
# themis-config.yaml
observability:
  enabled: true
  
  opentelemetry:
    endpoint: "otel-collector:4317"
    protocol: "grpc"
    
    # Traces
    traces:
      enabled: true
      sampling_rate: 1.0  # 100% für Entwicklung, 0.1 für Produktion
      
    # Metrics
    metrics:
      enabled: true
      export_interval: 30s
      
    # Logs
    logs:
      enabled: true
      level: "info"
      format: "json"
  
  # Service Metadata
  service:
    name: "themisdb"
    version: "1.4.0"
    environment: "production"
    instance_id: "${HOSTNAME}"
```

### Docker Compose Setup

```yaml
version: '3.8'

services:
  themisdb:
    image: themisdb/themisdb:1.4.0
    environment:
      - OTEL_EXPORTER_OTLP_ENDPOINT=http://otel-collector:4317
      - OTEL_SERVICE_NAME=themisdb
      - OTEL_TRACES_SAMPLER=parentbased_traceidratio
      - OTEL_TRACES_SAMPLER_ARG=0.1
    depends_on:
      - otel-collector
  
  otel-collector:
    image: otel/opentelemetry-collector-contrib:0.91.0
    command: ["--config=/etc/otel-collector-config.yaml"]
    volumes:
      - ./otel-collector-config.yaml:/etc/otel-collector-config.yaml
    ports:
      - "4317:4317"   # OTLP gRPC
      - "4318:4318"   # OTLP HTTP
      - "8889:8889"   # Prometheus metrics
  
  jaeger:
    image: jaegertracing/all-in-one:1.52
    ports:
      - "16686:16686"  # UI
      - "14250:14250"  # gRPC
  
  prometheus:
    image: prom/prometheus:v2.48.0
    volumes:
      - ./prometheus.yml:/etc/prometheus/prometheus.yml
    ports:
      - "9090:9090"
  
  grafana:
    image: grafana/grafana:10.2.2
    ports:
      - "3000:3000"
    environment:
      - GF_AUTH_ANONYMOUS_ENABLED=true
      - GF_AUTH_ANONYMOUS_ORG_ROLE=Admin
```

---

## Traces

### Automatisches Tracing

ThemisDB instrumentiert automatisch folgende Operationen:

```cpp
// Automatisch instrumentierte Operationen
// 1. HTTP Requests
//    Span: http.request
//    Attributes: http.method, http.url, http.status_code

// 2. AQL Queries
//    Span: aql.query
//    Attributes: aql.query, aql.collection, aql.execution_time

// 3. Storage Operations
//    Span: storage.operation
//    Attributes: storage.operation, storage.key, storage.size

// 4. Index Operations
//    Span: index.operation
//    Attributes: index.type, index.field, index.operation
```

### Custom Spans

```cpp
// C++ API für Custom Spans
#include <themis/observability/tracing.h>

void myCustomOperation() {
    auto span = themis::tracing::StartSpan("custom.operation");
    span.SetAttribute("custom.attribute", "value");
    
    try {
        // Operation ausführen
        doSomething();
        span.SetStatus(themis::tracing::StatusCode::Ok);
    } catch (const std::exception& e) {
        span.SetStatus(themis::tracing::StatusCode::Error, e.what());
        span.RecordException(e);
    }
    
    span.End();
}
```

### Query Tracing

```aql
// AQL Query mit Tracing
/* trace:enabled=true */
FOR doc IN collection
  FILTER doc.value > 100
  RETURN doc
```

**Erzeugte Spans:**
```
aql.query (parent)
├── aql.parse (child)
├── aql.optimize (child)
├── aql.execute (child)
│   ├── index.lookup (grandchild)
│   ├── filter.apply (grandchild)
│   └── result.materialize (grandchild)
└── aql.finalize (child)
```

### Distributed Tracing

```python
# Python Client mit Tracing
from themisdb import ThemisDB
from opentelemetry import trace
from opentelemetry.exporter.otlp.proto.grpc.trace_exporter import OTLPSpanExporter
from opentelemetry.sdk.trace import TracerProvider
from opentelemetry.sdk.trace.export import BatchSpanProcessor

# Setup OpenTelemetry
provider = TracerProvider()
processor = BatchSpanProcessor(OTLPSpanExporter(endpoint="otel-collector:4317"))
provider.add_span_processor(processor)
trace.set_tracer_provider(provider)

# Client mit Tracing
db = ThemisDB("http://themis:8765", enable_tracing=True)

# Trace über mehrere Services
tracer = trace.get_tracer(__name__)

with tracer.start_as_current_span("api.request"):
    # ThemisDB Query (automatisch traced)
    result = db.query("FOR doc IN users FILTER doc.age > 25 RETURN doc")
    
    # Weitere Operations
    with tracer.start_as_current_span("process.results"):
        processed = process_data(result.entities)
```

---

## Metrics

### Automatische Metrics

ThemisDB exportiert automatisch folgende Metriken:

#### Server Metrics

| Metric | Type | Description |
|--------|------|-------------|
| `themis_server_requests_total` | Counter | Total HTTP requests |
| `themis_server_request_duration_seconds` | Histogram | Request duration |
| `themis_server_active_connections` | Gauge | Active client connections |

#### Query Metrics

| Metric | Type | Description |
|--------|------|-------------|
| `themis_query_executions_total` | Counter | Total query executions |
| `themis_query_duration_seconds` | Histogram | Query execution time |
| `themis_query_errors_total` | Counter | Query errors |
| `themis_query_cache_hits_total` | Counter | Query cache hits |

#### Storage Metrics

| Metric | Type | Description |
|--------|------|-------------|
| `themis_storage_operations_total` | Counter | Storage operations |
| `themis_storage_bytes_read` | Counter | Bytes read from storage |
| `themis_storage_bytes_written` | Counter | Bytes written to storage |
| `themis_storage_size_bytes` | Gauge | Total storage size |

#### Index Metrics

| Metric | Type | Description |
|--------|------|-------------|
| `themis_index_lookups_total` | Counter | Index lookup operations |
| `themis_index_size_bytes` | Gauge | Index memory usage |
| `themis_index_build_duration_seconds` | Histogram | Index build time |

### Custom Metrics

```cpp
// C++ API für Custom Metrics
#include <themis/observability/metrics.h>

// Counter
auto request_counter = themis::metrics::Counter(
    "my_service_requests_total",
    "Total requests to my service"
);
request_counter.Increment();

// Gauge
auto active_users = themis::metrics::Gauge(
    "my_service_active_users",
    "Number of active users"
);
active_users.Set(42);

// Histogram
auto response_time = themis::metrics::Histogram(
    "my_service_response_time_seconds",
    "Response time in seconds",
    {0.001, 0.005, 0.01, 0.05, 0.1, 0.5, 1.0, 5.0}
);
response_time.Observe(0.123);
```

### Prometheus Scraping

```yaml
# prometheus.yml
scrape_configs:
  - job_name: 'themisdb'
    scrape_interval: 15s
    static_configs:
      - targets: ['themis:8765']
    
  - job_name: 'otel-collector'
    scrape_interval: 15s
    static_configs:
      - targets: ['otel-collector:8889']
```

---

## Logs

### Structured Logging

```json
{
  "timestamp": "2026-01-24T14:30:45.222Z",
  "level": "info",
  "service": "themisdb",
  "version": "1.4.0",
  "trace_id": "a1b2c3d4e5f6g7h8",
  "span_id": "1234567890abcdef",
  "message": "Query executed successfully",
  "attributes": {
    "query.duration_ms": 123,
    "query.collection": "users",
    "query.results": 42
  }
}
```

### Log Levels

| Level | Verwendung | Beispiel |
|-------|-----------|----------|
| `ERROR` | Fehler, die Attention benötigen | Datenbank-Verbindungsfehler |
| `WARN` | Warnungen, nicht kritisch | Langsame Query (>1s) |
| `INFO` | Informative Events | Query erfolgreich ausgeführt |
| `DEBUG` | Debug-Informationen | Query-Plan Details |
| `TRACE` | Sehr detaillierte Logs | Function Entry/Exit |

### Log Correlation

```cpp
// Logs mit Trace Context
themis::logging::Info("Processing request", {
    {"trace_id", current_trace_id},
    {"span_id", current_span_id},
    {"user_id", user_id},
    {"operation", "query"}
});
```

---

## Backends

### Jaeger Setup

```yaml
# Jaeger All-in-One
services:
  jaeger:
    image: jaegertracing/all-in-one:1.52
    environment:
      - COLLECTOR_OTLP_ENABLED=true
    ports:
      - "16686:16686"  # UI
      - "4317:4317"    # OTLP gRPC
      - "4318:4318"    # OTLP HTTP
```

**Access:** http://localhost:16686

### Grafana Stack

```yaml
# Grafana + Tempo + Loki + Prometheus
services:
  grafana:
    image: grafana/grafana:10.2.2
    volumes:
      - ./grafana/datasources.yaml:/etc/grafana/provisioning/datasources/datasources.yaml
    ports:
      - "3000:3000"
  
  tempo:
    image: grafana/tempo:2.3.1
    command: ["-config.file=/etc/tempo.yaml"]
    volumes:
      - ./tempo.yaml:/etc/tempo.yaml
    ports:
      - "3200:3200"   # Tempo HTTP
      - "4317:4317"   # OTLP gRPC
  
  loki:
    image: grafana/loki:2.9.3
    ports:
      - "3100:3100"
  
  prometheus:
    image: prom/prometheus:v2.48.0
    volumes:
      - ./prometheus.yml:/etc/prometheus/prometheus.yml
    ports:
      - "9090:9090"
```

**Grafana Datasources:**
```yaml
# grafana/datasources.yaml
apiVersion: 1

datasources:
  - name: Prometheus
    type: prometheus
    access: proxy
    url: http://prometheus:9090
    
  - name: Tempo
    type: tempo
    access: proxy
    url: http://tempo:3200
    
  - name: Loki
    type: loki
    access: proxy
    url: http://loki:3100
```

---

## Best Practices

### 1. Sampling Strategy

```yaml
# Adaptive Sampling
traces:
  sampling:
    # Production: Sample 10%
    default: 0.1
    
    # Immer samplen bei Errors
    always_on_error: true
    
    # Rate Limiting
    max_traces_per_second: 100
    
    # Regelbasiert
    rules:
      - name: slow_queries
        condition: "duration > 1000ms"
        sample_rate: 1.0
      
      - name: errors
        condition: "status_code >= 500"
        sample_rate: 1.0
```

### 2. Resource Attributes

```yaml
# Consistent Resource Attributes
resource:
  attributes:
    service.name: "themisdb"
    service.version: "1.4.0"
    service.namespace: "production"
    service.instance.id: "${HOSTNAME}"
    deployment.environment: "production"
    host.name: "${HOSTNAME}"
    cloud.provider: "aws"
    cloud.region: "eu-central-1"
```

### 3. Metric Cardinality

```cpp
// ✅ Gut: Low Cardinality
metrics::Counter("http_requests_total", {
    {"method", "GET"},
    {"status", "200"}
});

// ❌ Schlecht: High Cardinality
metrics::Counter("http_requests_total", {
    {"user_id", "12345"},  // Zu viele unique Values!
    {"request_id", "abc"}  // Zu viele unique Values!
});
```

### 4. Log Aggregation

```python
# Log Context Management
import logging
from contextvars import ContextVar

trace_id_var = ContextVar('trace_id', default=None)

class TraceContextFilter(logging.Filter):
    def filter(self, record):
        record.trace_id = trace_id_var.get()
        return True

# Logger Setup
logger = logging.getLogger(__name__)
logger.addFilter(TraceContextFilter())
```

---

## Troubleshooting

### Keine Traces sichtbar

**Problem:** Traces werden nicht in Jaeger angezeigt.

**Lösung:**
```bash
# 1. OTLP Collector prüfen
curl http://otel-collector:13133/

# 2. ThemisDB Traces prüfen
curl http://themis:8765/debug/traces

# 3. Sampling Rate erhöhen
export OTEL_TRACES_SAMPLER_ARG=1.0  # 100% sampling

# 4. Logs prüfen
docker logs otel-collector
docker logs themisdb
```

### Hohe Latenz bei Tracing

**Problem:** Tracing verursacht Performance-Probleme.

**Lösung:**
```yaml
# Batch Processing optimieren
processors:
  batch:
    timeout: 5s
    send_batch_size: 2048
    send_batch_max_size: 4096

# Sampling reduzieren
traces:
  sampling_rate: 0.01  # 1%
```

### Metriken fehlen

**Problem:** Prometheus scraped keine Metriken.

**Lösung:**
```bash
# Prometheus targets prüfen
curl http://prometheus:9090/api/v1/targets

# ThemisDB metrics endpoint prüfen
curl http://themis:8765/metrics

# OTLP Collector metrics prüfen
curl http://otel-collector:8889/metrics
```

---

## Siehe auch

- [Prometheus Integration](observability_prometheus.md)
- [Grafana Dashboards](../tools/operations/GRAFANA_DASHBOARDS.md)
- [Alerting Configuration](observability_alerting.md)
- [Production Monitoring Guide](../production/PRODUCTION_MONITORING.md)
