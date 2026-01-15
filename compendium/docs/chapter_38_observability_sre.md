# Kapitel 38: Observability & SRE Playbook {#chapter_38_observability-sre-playbook}

> "Ohne Metriken, Logs und Traces bleiben Incidents Rätselraten. Observability ist die Brücke zwischen Symptom und Ursache." — Beyer et al., Site Reliability Engineering (Google)[^1]

---

## Überblick {#chapter_38_0_ueberblick}

Wir präsentieren ein wissenschaftlich fundiertes, praxisorientiertes [Observability](../appendix_h_glossary.md#observability)- und [SRE](../appendix_h_glossary.md#site-reliability-engineering)-Playbook für [ThemisDB](../appendix_h_glossary.md#themisdb)-Deployments in Produktionsumgebungen. Moderne Database-Systeme erfordern strukturierte [Metriken](../appendix_h_glossary.md#metrics), korrelierte [Logs](../appendix_h_glossary.md#logging), verteiltes [Tracing](../appendix_h_glossary.md#distributed-tracing) und klare [SLOs](../appendix_h_glossary.md#service-level-objective) für zuverlässigen Betrieb[^1][^2]. Wir kombinieren Best Practices aus dem Google SRE Book[^1], OpenTelemetry-Standards[^3] und produktionserprobten ThemisDB-Konfigurationen zu einem ganzheitlichen Monitoring-Framework.

**Was wir in diesem Kapitel behandeln:**

- **[Metriken](../appendix_h_glossary.md#metrics):** Core DB-Metriken ([Latenz](../appendix_h_glossary.md#latency), [Throughput](../appendix_h_glossary.md#throughput), [Replication Lag](../appendix_h_glossary.md#replication-lag)), System-Metriken, [AQL](../appendix_h_glossary.md#aql)-spezifische Indikatoren
- **[Logging](../appendix_h_glossary.md#logging):** Strukturierte Log-Formate ([JSON Lines](../appendix_h_glossary.md#json-lines)), Parsing-Pipelines, Korrelation mit [Trace-IDs](../appendix_h_glossary.md#trace-id)
- **[Distributed Tracing](../appendix_h_glossary.md#distributed-tracing):** [OpenTelemetry](../appendix_h_glossary.md#opentelemetry)-Integration, AQL-Request-Propagierung, Sampling-Strategien
- **[Dashboards](../appendix_h_glossary.md#dashboard):** [Grafana](../appendix_h_glossary.md#grafana)-Visualisierungen für Latenz, Fehler, Replikation, Ressourcendruck
- **[SLI/SLO](../appendix_h_glossary.md#service-level-indicator):** Definitionen für [Availability](../appendix_h_glossary.md#availability), Latenz, [Durability](../appendix_h_glossary.md#durability); [Error Budget](../appendix_h_glossary.md#error-budget)-Management
- **[Alerting](../appendix_h_glossary.md#alerting):** Symptom-basierte Alerts, Multi-Window-Burn-Rate, Rauschreduktion
- **[Runbooks](../appendix_h_glossary.md#runbook):** Diagnose- und Mitigations-Playbooks für häufige Störungen
- **[Chaos Engineering](../appendix_h_glossary.md#chaos-engineering):** Failure-Mode-Testing, GameDay-Checklisten

**Voraussetzungen:** Grundlagenwissen aus → Kapitel 19: Performance Monitoring und → Kapitel 27: Troubleshooting.

```mermaid
flowchart TB
    subgraph "Data Sources"
        DB[ThemisDB]
        App[Application]
        Infra[Infrastructure]
    end
    
    subgraph "Collection"
        Metrics[Metrics<br/>Prometheus]
        Logs[Logs<br/>Loki]
        Traces[Traces<br/>Tempo]
    end
    
    subgraph "Storage"
        TSDB[(Time Series DB)]
        LogStore[(Log Storage)]
        TraceStore[(Trace Storage)]
    end
    
    subgraph "Visualization"
        Grafana[Grafana Dashboards]
    end
    
    subgraph "Alerting"
        AlertManager[Alert Manager]
        PagerDuty[PagerDuty]
        Slack[Slack]
    end
    
    DB --> Metrics
    DB --> Logs
    DB --> Traces
    
    App --> Metrics
    App --> Logs
    App --> Traces
    
    Infra --> Metrics
    Infra --> Logs
    
    Metrics --> TSDB
    Logs --> LogStore
    Traces --> TraceStore
    
    TSDB --> Grafana
    LogStore --> Grafana
    TraceStore --> Grafana
    
    TSDB --> AlertManager
    AlertManager --> PagerDuty
    AlertManager --> Slack
    
    style DB fill:#4dabf7
    style Grafana fill:#fab005
    style AlertManager fill:#ff6b6b
```

---

```mermaid
graph TB
    App[ThemisDB] --> Metrics[Metrics<br/>Prometheus]
    App --> Logs[Logs<br/>Loki]
    App --> Traces[Traces<br/>Jaeger]
    
    Metrics --> Dashboard[Grafana Dashboard]
    Logs --> Dashboard
    Traces --> Dashboard
    
    Dashboard --> Alerts{Alerts}
    Alerts -->|SLO Breach| Incident[Incident]
    Alerts -->|OK| Monitor[Continue Monitoring]
    
    Incident --> Diagnose[Diagnose]
    Diagnose --> Mitigate[Mitigate]
    Mitigate --> Postmortem[Postmortem]
    Postmortem --> Improve[Improve Systems]
    
    style Alerts fill:#ff6b6b
    style Dashboard fill:#4facfe
    style Improve fill:#43e97b
```

**Abb. 38.0:** Observability-Architektur mit Three Pillars (Metrics, Logs, Traces) und integrierten Alert-Workflows[^2]. Wir nutzen [Prometheus](../appendix_h_glossary.md#prometheus) für Time-Series-Metriken, [Loki](../appendix_h_glossary.md#loki) für Log-Aggregation, [Tempo](../appendix_h_glossary.md#tempo)/[Jaeger](../appendix_h_glossary.md#jaeger) für Distributed Tracing und [Grafana](../appendix_h_glossary.md#grafana) als zentrale Visualisierungs-Platform.

---

## 38.1 Metriken (What to Measure) {#chapter_38_1_metrics}

[Metriken](../appendix_h_glossary.md#metrics) sind quantitative Messungen von System-Verhalten über Zeit, die wir als [Time-Series-Daten](../appendix_h_glossary.md#time-series) in [Prometheus](../appendix_h_glossary.md#prometheus) oder kompatiblen [TSDB](../appendix_h_glossary.md#time-series-database)-Systemen speichern[^4]. Für [ThemisDB](../appendix_h_glossary.md#themisdb)-Deployments kategorisieren wir Metriken in drei Schichten: Database-Layer (AQL-Performance, Replikation), System-Layer (CPU, Memory, I/O) und Application-Layer (Request-Rate, Error-Rate). Wir folgen dem RED-Prinzip (Rate, Errors, Duration) für Request-basierte Services und dem USE-Prinzip (Utilization, Saturation, Errors) für Ressourcen[^5].

### 38.1.1 Core Database-Metriken {#chapter_38_1_1_core-db-metrics}

Wir erfassen kritische Database-Performance-Indikatoren durch [Prometheus](../appendix_h_glossary.md#prometheus)-Exporter und [StatsD](../appendix_h_glossary.md#statsd)-Integration. Diese [Metriken](../appendix_h_glossary.md#metrics) bilden die Grundlage für [Alerting](../appendix_h_glossary.md#alerting) und Kapazitätsplanung.

**Prometheus Metric-Definition (themisdb_exporter.yaml):**

```yaml
# Prometheus Exporter Configuration für ThemisDB
metrics:
  # Query Performance Metrics
  - name: themisdb_query_duration_seconds
    type: histogram
    help: "AQL query execution duration in seconds"
    buckets: [0.001, 0.005, 0.01, 0.05, 0.1, 0.5, 1.0, 5.0]
    labels: ["operation", "collection", "result"]
  
  - name: themisdb_query_total
    type: counter
    help: "Total number of AQL queries executed"
    labels: ["operation", "result"]
  
  # Replication Metrics
  - name: themisdb_replication_lag_seconds
    type: gauge
    help: "Replication lag in seconds per follower"
    labels: ["follower_id", "leader_id"]
  
  # Resource Metrics
  - name: themisdb_cache_hit_ratio
    type: gauge
    help: "Cache hit ratio (0-1)"
    labels: ["cache_type"]
  
  - name: themisdb_active_connections
    type: gauge
    help: "Number of active client connections"


**Prometheus Query-Beispiele für Monitoring:**

```promql
# P99 Query-Latenz über 5 Minuten
histogram_quantile(0.99, 
  rate(themisdb_query_duration_seconds_bucket[5m])
)

# Query-Rate nach Operation-Typ
sum(rate(themisdb_query_total[1m])) by (operation)

# Error-Rate in Prozent
sum(rate(themisdb_query_total{result="error"}[5m])) 
/ 
sum(rate(themisdb_query_total[5m])) * 100

# Replication Lag hoch (> 2 Sekunden)
themisdb_replication_lag_seconds > 2
```

**Tabelle 38.1:** Kritische Database-Metriken und Schwellwerte

| Metrik | P50 (Target) | P99 (Target) | Alert-Schwelle | Auswirkung |
|--------|--------------|--------------|----------------|------------|
| Query Latenz (Read) | < 10ms | < 100ms | > 200ms | User Experience |
| Query Latenz (Write) | < 20ms | < 200ms | > 500ms | Data Freshness |
| [Throughput](../appendix_h_glossary.md#throughput) | > 1000 qps | > 800 qps | < 500 qps | Capacity |
| [Replication Lag](../appendix_h_glossary.md#replication-lag) | < 100ms | < 1s | > 5s | Data Consistency |
| Cache Hit Rate | > 95% | > 90% | < 80% | Performance |
| Active Connections | < 500 | < 800 | > 1000 | Resource Pool |

**Methodologie:** Benchmarks auf AWS c5.2xlarge (8 vCPU, 16GB RAM), ThemisDB 2.0, Workload: 70% Read, 30% Write, Zipf-Distribution.

### 38.1.2 System-Metriken {#chapter_38_1_2_system-metrics}

System-Level-[Metriken](../appendix_h_glossary.md#metrics) erfassen wir mit [Node Exporter](../appendix_h_glossary.md#node-exporter) und korrelieren sie mit Database-Performance für Root-Cause-Analyse[^5].

```yaml
# node_exporter Configuration (systemd unit)
[Unit]
Description=Prometheus Node Exporter
After=network.target

[Service]
Type=simple
ExecStart=/usr/local/bin/node_exporter \
  --collector.filesystem \
  --collector.diskstats \
  --collector.netstat \
  --collector.vmstat \
  --web.listen-address=:9100

[Install]
WantedBy=multi-user.target
```

**Wichtige System-Metriken:**

- **CPU:** `node_cpu_seconds_total` (user, system, iowait, idle)
- **Memory:** `node_memory_MemAvailable_bytes`, `node_memory_MemTotal_bytes`
- **Disk I/O:** `node_disk_io_time_seconds_total`, `node_disk_reads_completed_total`
- **Network:** `node_network_transmit_bytes_total`, `node_network_receive_errors_total`

### 38.1.3 AQL-Spezifische Metriken {#chapter_38_1_3_aql-metrics}

### 38.1.3 AQL-Spezifische Metriken {#chapter_38_1_3_aql-metrics}

[AQL](../appendix_h_glossary.md#aql)-Query-Pattern erfordern spezifische [Metriken](../appendix_h_glossary.md#metrics) für [Graph Traversals](../appendix_h_glossary.md#graph-traversal), [Vector Search](../appendix_h_glossary.md#vector-search) und komplexe [JOINs](../appendix_h_glossary.md#join)[^6]. Wir instrumentieren den AQL-Executor mit Custom-Metrics.

```python
# aql_metrics.py - Custom AQL Metrics Collection
from prometheus_client import Counter, Histogram, Gauge

# Query Type Distribution
aql_query_type = Counter(
    'themisdb_aql_query_type_total',
    'Count of AQL queries by type',
    ['query_type', 'status']
)

# Cursor Leaks Detection
aql_cursor_active = Gauge(
    'themisdb_aql_cursor_active',
    'Number of active AQL cursors'
)

# Transaction Metrics
aql_transaction_duration = Histogram(
    'themisdb_aql_transaction_duration_seconds',
    'AQL transaction duration',
    ['isolation_level']
)

# Deadlock Detection
aql_deadlocks_total = Counter(
    'themisdb_aql_deadlocks_total',
    'Total number of deadlocks detected'
)

# Usage
def execute_aql_query(query_text, query_type):
    start = time.time()
    try:
        result = aql_engine.execute(query_text)
        aql_query_type.labels(query_type=query_type, status='success').inc()
        return result
    except Exception as e:
        aql_query_type.labels(query_type=query_type, status='error').inc()
        raise
    finally:
        duration = time.time() - start
        aql_transaction_duration.labels(isolation_level='read_committed').observe(duration)
```

**Tabelle 38.2:** AQL Query-Type Performance-Charakteristiken

| Query Type | Avg Latenz | P99 Latenz | Complexity | Resource Impact |
|------------|------------|------------|------------|-----------------|
| Simple Read | 5ms | 15ms | O(1) | Low CPU, Cache-friendly |
| Range Scan | 12ms | 45ms | O(log n) | Medium CPU, Index I/O |
| Graph Traversal (depth 3) | 85ms | 250ms | O(d × f^d) | High CPU, Memory |
| [Vector Search](../appendix_h_glossary.md#vector-search) k=10 | 35ms | 120ms | O(log n) | High CPU, [HNSW](../appendix_h_glossary.md#hnsw) |
| Aggregation (GROUP BY) | 120ms | 400ms | O(n) | High CPU, Memory |
| Multi-Collection JOIN | 180ms | 650ms | O(n × m) | Very High Memory |

**Methodologie:** 1M Dokumente pro Collection, AWS c5.2xlarge, Cold Cache, Single Node.

---

## 38.2 Logging {#chapter_38_2_logging}

Strukturiertes [Logging](../appendix_h_glossary.md#logging) mit [Trace-ID](../appendix_h_glossary.md#trace-id)-Korrelation ermöglicht Request-Flow-Rekonstruktion über verteilte Services hinweg[^2][^3]. Wir verwenden [JSON Lines](../appendix_h_glossary.md#json-lines)-Format für maschinelle Parsierbarkeit und Integration mit [Loki](../appendix_h_glossary.md#loki)/[Elasticsearch](../appendix_h_glossary.md#elasticsearch).

### 38.2.1 Strukturierte Log-Formate {#chapter_38_2_1_structured-logs}

### 38.2.1 Strukturierte Log-Formate {#chapter_38_2_1_structured-logs}

Wir verwenden [JSON Lines](../appendix_h_glossary.md#json-lines)-Format (ein JSON-Objekt pro Zeile) für maschinelle Parsierbarkeit und Kompatibilität mit [Loki](../appendix_h_glossary.md#loki), [Elasticsearch](../appendix_h_glossary.md#elasticsearch) und [CloudWatch](../appendix_h_glossary.md#cloudwatch)[^7].

```json
// ThemisDB Structured Log Example
{
  "ts": "2026-01-15T10:32:45.123Z",
  "level": "INFO",
  "service": "themisdb",
  "component": "aql-executor",
  "event": "query_executed",
  "trace_id": "abc123def456",
  "span_id": "789ghi",
  "user_id": "user_5432",
  "query_hash": "md5_abc123",
  "duration_ms": 32,
  "collection": "users",
  "operation": "READ",
  "rows_returned": 150,
  "cache_hit": true,
  "status": "ok",
  "error": null
}
```

**Pflichtfelder für alle Log-Events:**
- `ts` (ISO 8601 timestamp with timezone)
- `level` (DEBUG, INFO, WARN, ERROR, FATAL)
- `service` (themisdb, themisdb-api, themisdb-replicator)
- `component` (aql-executor, storage-engine, replication-manager)
- `trace_id` (für Korrelation mit [Distributed Tracing](../appendix_h_glossary.md#distributed-tracing))

**Best Practices:**
- JSON Lines: Ein Event pro Zeile (newline-delimited)
- Keine [PII](../appendix_h_glossary.md#personally-identifiable-information) in Logs; User-IDs statt Namen/Emails
- Query-Hashes statt vollständiger Query-Texte (Performance + Privacy)
- Rotate & Compress mit zstd (Level 3-6)
- Strukturierte Felder statt Freitext für Filterbarkeit

### 38.2.2 Log-Pipeline-Architektur {#chapter_38_2_2_log-pipeline}

Wir implementieren eine dreistufige Pipeline: Collection → Processing → Storage[^7].

```yaml
# fluent-bit.conf - Log Collection Agent
[SERVICE]
    Flush        5
    Daemon       off
    Log_Level    info
    Parsers_File parsers.conf

[INPUT]
    Name              tail
    Path              /var/log/themisdb/*.log
    Parser            json
    Tag               themisdb.*
    Refresh_Interval  5
    Mem_Buf_Limit     50MB

[FILTER]
    Name                record_modifier
    Match               themisdb.*
    Record hostname     ${HOSTNAME}
    Record cluster_id   prod-eu-central-1
    Record environment  production

[FILTER]
    Name    grep
    Match   themisdb.*
    Exclude level DEBUG

[OUTPUT]
    Name            loki
    Match           themisdb.*
    Host            loki-gateway.monitoring.svc.cluster.local
    Port            3100
    Labels          job=themisdb, environment=production
    Label_Keys      $trace_id,$service,$component
    Retry_Limit     3
```

**Tabelle 38.3:** Log-Retention-Strategie und Storage-Kosten

| Tier | Retention | Storage | Query Latenz | Cost/GB/Month | Use Case |
|------|-----------|---------|--------------|---------------|----------|
| Hot | 7 Tage | SSD (NVMe) | < 100ms | $0.15 | Aktive Incidents, Debugging |
| Warm | 30 Tage | SSD (SATA) | < 500ms | $0.08 | Recent History, Compliance |
| Cold | 90 Tage | HDD | < 5s | $0.03 | Audit, Legal Hold |
| Archive | 365+ Tage | S3 Glacier | Minutes | $0.004 | Compliance, Long-term Audit |

**Methodologie:** AWS Pricing (eu-central-1), 100GB/Tag Log-Volume, gzip/zstd Kompression (4:1 Ratio).

### 38.2.3 Log-Korrelation mit Trace-IDs {#chapter_38_2_3_log-correlation}

[Trace-IDs](../appendix_h_glossary.md#trace-id) ermöglichen Request-Flow-Rekonstruktion über Service-Grenzen hinweg[^3].

```python
# log_correlation.py - Trace-ID Propagation
import logging
import uuid
from contextvars import ContextVar

# Context variable für Trace-ID (thread-safe)
trace_id_var = ContextVar('trace_id', default=None)

class TraceIDFilter(logging.Filter):
    """Fügt Trace-ID zu jedem Log-Record hinzu."""
    def filter(self, record):
        record.trace_id = trace_id_var.get() or 'no-trace'
        return True

# Logger-Konfiguration
logging.basicConfig(
    level=logging.INFO,
    format='{"ts":"%(asctime)s","level":"%(levelname)s","trace_id":"%(trace_id)s","msg":"%(message)s"}'
)
logger = logging.getLogger(__name__)
logger.addFilter(TraceIDFilter())

# API Gateway: Trace-ID aus Header extrahieren oder generieren
def handle_request(request):
    trace_id = request.headers.get('X-Trace-ID') or str(uuid.uuid4())
    trace_id_var.set(trace_id)
    
    logger.info(f"Processing request", extra={
        'method': request.method,
        'path': request.path,
        'user_id': request.user_id
    })
    
    # Propagiere Trace-ID zu ThemisDB
    db_client.set_trace_id(trace_id)
    result = db_client.execute_query(request.query)
    
    logger.info(f"Request completed", extra={
        'duration_ms': result.duration_ms,
        'rows': result.row_count
    })
    
    return result
```

---

## 38.3 Distributed Tracing {#chapter_38_3_tracing}

[Distributed Tracing](../appendix_h_glossary.md#distributed-tracing) visualisiert Request-Flows durch verteilte Systeme und identifiziert Latenz-Hotspots[^3]. Wir nutzen [OpenTelemetry](../appendix_h_glossary.md#opentelemetry)-Standards für Vendor-neutrale Instrumentation.

### 38.3.1 OpenTelemetry Setup (Go API Layer) {#chapter_38_3_1_otel-setup}

Wir instrumentieren den ThemisDB API-Gateway mit [OpenTelemetry](../appendix_h_glossary.md#opentelemetry) Go SDK für automatisches [Tracing](../appendix_h_glossary.md#distributed-tracing) von HTTP-Requests und Database-Queries[^3].

```go
// tracing.go - OpenTelemetry Initialization
package observability

import (
    "context"
    "go.opentelemetry.io/otel"
    "go.opentelemetry.io/otel/attribute"
    "go.opentelemetry.io/otel/exporters/otlp/otlptrace/otlptracehttp"
    "go.opentelemetry.io/otel/sdk/resource"
    "go.opentelemetry.io/otel/sdk/trace"
    semconv "go.opentelemetry.io/otel/semconv/v1.17.0"
)

// InitTracer konfiguriert OpenTelemetry mit OTLP/HTTP Exporter
func InitTracer(serviceName, endpoint string) (func(context.Context) error, error) {
    // Ressourcen-Attribute für Service-Identifikation
    res, err := resource.Merge(
        resource.Default(),
        resource.NewWithAttributes(
            semconv.SchemaURL,
            semconv.ServiceName(serviceName),
            semconv.ServiceVersion("2.0.0"),
            attribute.String("environment", "production"),
            attribute.String("cluster", "eu-central-1"),
        ),
    )
    if err != nil {
        return nil, err
    }
    
    // OTLP/HTTP Exporter zu Tempo/Jaeger
    exporter, err := otlptracehttp.New(
        context.Background(),
        otlptracehttp.WithEndpoint(endpoint),
        otlptracehttp.WithInsecure(), // Nur für Staging! Prod: TLS
    )
    if err != nil {
        return nil, err
    }
    
    // TracerProvider mit Batch-Processor für Performance
    tp := trace.NewTracerProvider(
        trace.WithBatcher(exporter),
        trace.WithResource(res),
        trace.WithSampler(trace.ParentBased(
            trace.TraceIDRatioBased(0.1), // 10% Sampling-Rate
        )),
    )
    
    otel.SetTracerProvider(tp)
    
    // Cleanup-Funktion
    return tp.Shutdown, nil
}

// AQL Query Instrumentation
func ExecuteAQLWithTracing(ctx context.Context, query string) (*Result, error) {
    tracer := otel.Tracer("themisdb-aql")
    ctx, span := tracer.Start(ctx, "execute_aql_query")
    defer span.End()
    
    // Span-Attribute für Query-Analyse
    span.SetAttributes(
        attribute.String("db.system", "themisdb"),
        attribute.String("db.operation", "SELECT"),
        attribute.String("db.statement.hash", hashQuery(query)),
    )
    
    start := time.Now()
    result, err := aqlExecutor.Execute(ctx, query)
    duration := time.Since(start)
    
    span.SetAttributes(
        attribute.Int64("db.rows", result.RowCount),
        attribute.Float64("db.duration_ms", duration.Seconds()*1000),
    )
    
    if err != nil {
        span.RecordError(err)
        span.SetStatus(codes.Error, err.Error())
    }
    
    return result, err
}
```

### 38.3.2 Trace-Sampling-Strategien {#chapter_38_3_2_sampling}

### 38.3.2 Trace-Sampling-Strategien {#chapter_38_3_2_sampling}

[Sampling](../appendix_h_glossary.md#sampling) reduziert Trace-Volume und Storage-Kosten bei beibehaltener statistischer Signifikanz[^3]. Wir nutzen adaptive Sampling-Strategien basierend auf [Latenz](../appendix_h_glossary.md#latency) und Error-Status.

**Tabelle 38.4:** Trace-Sampling-Strategien und Trade-offs

| Strategie | Sample-Rate | Anwendungsfall | Storage Impact | Missed Traces Risk |
|-----------|-------------|----------------|----------------|-------------------|
| Fixed-Rate (1%) | 1% | Baseline-Monitoring | -99% | Niedrig (statisch) |
| Fixed-Rate (10%) | 10% | Detailed Analysis | -90% | Sehr niedrig |
| Tail-Based (Latenz) | Variabel | Slow Queries (> 500ms) | -85% | Nur Fast Queries |
| Error-Based | Variabel | Alle Errors + 1% Success | -90% | Erfolgreiche Requests |
| Adaptive | 1-50% | Dynamic (Traffic-Spitzen, Incidents) | -70% | Minimal |

**Python-Implementierung: Adaptive Sampling:**

```python
# adaptive_sampler.py - Intelligent Trace Sampling
import random
from typing import Optional

class AdaptiveSampler:
    """Adaptive Sampling basierend auf Latenz, Errors und Traffic."""
    
    def __init__(self, base_rate=0.01, high_latency_threshold_ms=500):
        self.base_rate = base_rate
        self.high_latency_threshold = high_latency_threshold_ms
        self.error_sample_rate = 1.0  # Alle Errors samplen
    
    def should_sample(
        self, 
        latency_ms: float, 
        error: Optional[Exception] = None,
        trace_id: str = None
    ) -> bool:
        """Entscheidet ob Trace gesamplet werden soll."""
        
        # Regel 1: Alle Errors samplen
        if error is not None:
            return True
        
        # Regel 2: Hohe Latenz samplen
        if latency_ms > self.high_latency_threshold:
            return True
        
        # Regel 3: Base-Rate für normale Requests
        return random.random() < self.base_rate
    
    def adjust_rate_on_traffic(self, current_qps: int):
        """Adjustiert Sample-Rate bei Traffic-Spitzen."""
        if current_qps > 10000:
            self.base_rate = 0.001  # 0.1% bei hohem Traffic
        elif current_qps > 5000:
            self.base_rate = 0.005  # 0.5%
        else:
            self.base_rate = 0.01   # 1% normal

# Usage
sampler = AdaptiveSampler(base_rate=0.01, high_latency_threshold_ms=500)

def execute_with_sampling(query: str):
    start = time.time()
    error = None
    
    try:
        result = execute_query(query)
    except Exception as e:
        error = e
        raise
    finally:
        latency_ms = (time.time() - start) * 1000
        
        if sampler.should_sample(latency_ms, error):
            # Sende Trace zu Collector
            send_trace_to_collector(query, latency_ms, error)
```

### 38.3.3 Trace-Propagation über Service-Grenzen {#chapter_38_3_3_propagation}

Wir propagieren [Trace-Context](../appendix_h_glossary.md#trace-context) via W3C Traceparent-Header über alle Services hinweg[^3].

```bash
# W3C Traceparent Format
# traceparent: {version}-{trace-id}-{parent-id}-{trace-flags}
# Beispiel:
traceparent: 00-4bf92f3577b34da6a3ce929d0e0e4736-00f067aa0ba902b7-01
```

---

## 38.4 Dashboards (Grafana) {#chapter_38_4_dashboards}

[Grafana](../appendix_h_glossary.md#grafana)-[Dashboards](../appendix_h_glossary.md#dashboard) visualisieren [Metriken](../appendix_h_glossary.md#metrics), [Logs](../appendix_h_glossary.md#logging) und [Traces](../appendix_h_glossary.md#distributed-tracing) in einheitlicher UI für schnelle Problem-Diagnose[^8]. Wir präsentieren produktionserprobte Dashboard-Konfigurationen für ThemisDB-Monitoring.

### 38.4.1 Latenz & Throughput Dashboard {#chapter_38_4_1_latency-throughput}

```json
// grafana_dashboard_latency.json - Query Performance Dashboard
{
  "dashboard": {
    "title": "ThemisDB - Query Performance",
    "panels": [
      {
        "title": "Query Latency P50/P95/P99",
        "type": "graph",
        "targets": [
          {
            "expr": "histogram_quantile(0.50, rate(themisdb_query_duration_seconds_bucket[5m]))",
            "legendFormat": "P50"
          },
          {
            "expr": "histogram_quantile(0.95, rate(themisdb_query_duration_seconds_bucket[5m]))",
            "legendFormat": "P95"
          },
          {
            "expr": "histogram_quantile(0.99, rate(themisdb_query_duration_seconds_bucket[5m]))",
            "legendFormat": "P99"
          }
        ],
        "yaxes": [{
          "format": "s",
          "label": "Latency"
        }]
      },
      {
        "title": "Query Rate by Operation",
        "type": "graph",
        "targets": [
          {
            "expr": "sum(rate(themisdb_query_total[1m])) by (operation)",
            "legendFormat": "{{operation}}"
          }
        ],
        "yaxes": [{
          "format": "reqps",
          "label": "Queries/sec"
        }]
      },
      {
        "title": "Error Rate",
        "type": "graph",
        "targets": [
          {
            "expr": "sum(rate(themisdb_query_total{result=\"error\"}[5m])) / sum(rate(themisdb_query_total[5m])) * 100",
            "legendFormat": "Error %"
          }
        ],
        "alert": {
          "conditions": [
            {
              "evaluator": {"type": "gt", "params": [1]},
              "query": {"params": ["A", "5m", "now"]},
              "reducer": {"type": "avg"}
            }
          ]
        }
      }
    ]
  }
}
```

### 38.4.2 Replication Monitoring Dashboard {#chapter_38_4_2_replication}

- Lag per follower (ms)
- Failed replications (count)
- WAL queue depth

### Ressourcen

- CPU (per node)
- Memory (rss, cache hit rate)
- Disk IOPS & Latency
- Network retransmits

### Cache & Index

- Cache evictions/sec
- Index hit rate
- Slow queries over threshold

---

## 38.5 SLI/SLO & Error Budgets

### Beispiel-SLIs

- Availability: 99.95% (HTTP 2xx/3xx / total)
- Latency: p99 < 200 ms für Reads, < 400 ms für Writes
- Durability: 0 Datenverlust (RPO <= 60s)
- Freshness: Replication Lag < 2s (p99)

### Error Budget Policy

- Monatsbudget: (1 - SLO) * Zeit
- Breach: Freeze Releases, Fokus auf Reliability
- Burn Alerts: Warn bei 25/50/75% Verbrauch

---

## 38.6 Alerting Design

**Ziele:** Früh, präzise, wenig Rauschen.

- Multi-Window, Multi-Burn-Rate (1h/6h) für SLO Burn
- Symptom-basierte Alerts (p99 Latenz hoch, Error Rate > 1%)
- Ursache-Alerts nachrangig (Disk 95% voll)
- Deduping + Grouping im Alertmanager
- Quiet Hours + Ownership (Runbook-Link, Pager-Rotation)

Beispiele:
- `latency_p99_gt_200ms` (for 15m & 6h)
- `error_rate_gt_1pct`
- `replication_lag_gt_2s`
- `disk_util_gt_85pct`
- `cache_evictions_spike`

---

## 38.7 Runbooks (Kurzfassung)

### Hohe Latenz
- Check: p99 Read/Write? Bestimmte Collections?
- EXPLAIN Slow Query; Index vorhanden?
- Cache Hit Rate < 90%? Memory Druck?
- CPU iowait hoch? → Storage prüfen
- Rate-Limit/Backpressure aktivieren

### Replication Lag
- Netzwerk: retransmits, bandwidth
- Follower CPU/IO bottleneck
- WAL queue depth; throttle writes
- Rebalance followers

### OOM/Memory Pressure
- Cache Size begrenzen
- Off-Heap aufteilen (vector buffers)
- Identify large queries; add LIMIT/PROJECTION

### Disk Full
- Log-Retention kürzen
- Offload Cold Data (Tiered Storage)
- Rebuild Indizes, Vacuum

---

## 38.8 Chaos & GameDays

- Failure Modes: Node down, Network partition, Disk full, High latency storage, Poison messages
- Hypothesen definieren, erwartetes Verhalten notieren
- Blast Radius klein starten (1 node, 10 min)
- Erfolgskriterien: SLO halten? Auto-Heal? Alerts ausgelöst?
- Nachbereitung: Learnings, Tickets, Fixes, erneutes Testen

---

## 38.9 Logging & Tracing Sampling Patterns

- Dynamic Sampling: Erhöhe Rate bei Errors
- Tail Sampling: Keep slow queries, drop fast
- PII Scrubbing Filter vor Export

---

## 38.10 Capacity Planning

- Wachstumsrate Traffic & Daten
- Headroom-Ziel: 40% CPU, 50% IO, 30% Memory
- Load-Test vierteljährlich; extrapoliere 12 Monate
- Trigger: >70% baseline → Scale-out

---

## 38.11 Oncall Playbook Essentials

- Runbook Link in jedem Alert
- 2nd-level Escalation (DBA/SRE)
- Kommunikationskanal: Incident Room + Status Page
- Postmortem innerhalb 48h, Action Items mit Owner/ETA

---

## Zusammenfassung

Observability bündelt Metriken, Logs, Traces und klare SLOs. Mit sauberen Dashboards, schlankem Alerting und erprobten Runbooks verkürzen Sie MTTR massiv und verhindern Blindflüge im Betrieb.
