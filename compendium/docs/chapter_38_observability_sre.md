# Kapitel 38: Observability & SRE Playbook {#chapter_38_observability-sre-playbook}

> *"Ohne Metriken, Logs und Traces bleiben Incidents Rätselraten. Observability ist die Brücke zwischen Symptom und Ursache."*

---

## Überblick {#chapter_38_0_ueberblick}

Dieses Kapitel liefert ein praktisches Observability- und SRE-Playbook für ThemisDB-Deployments. Wir präsentieren wissenschaftlich fundierte Methodologien (RED, USE) und Industry-Standard-Tools ([Prometheus](../appendix_h_glossary.md#prometheus), [Loki](../appendix_h_glossary.md#loki), [OpenTelemetry](../appendix_h_glossary.md#opentelemetry)) zur systematischen Erfassung von Metriken, Logs und Traces. Die Integration von [Distributed Tracing](../appendix_h_glossary.md#distributed-tracing) mit strukturierten Logs ermöglicht End-to-End-Debugging in Microservice-Architekturen. Wir vermitteln Cardinality-Management-Strategien zur Skalierung von Time-Series-Datenbanken und Sampling-Techniken zur Reduktion von Observability-Overhead bei hohem Durchsatz.

**Was Sie lernen:**
- **RED/USE-Metriken:** Systematische Erfassung von Request- und Ressourcen-Metriken (Abschnitt 38.1)
- **Strukturierte Logs:** JSON-basierte Log-Formate mit Trace-Korrelation (Abschnitt 38.2)
- **Distributed Tracing:** OpenTelemetry-Instrumentierung für AQL-Requests (Abschnitt 38.3)
- **Dashboards:** Grafana-Visualisierungen für Latenz, Throughput, Fehlerquoten (Abschnitt 38.4)
- **SLO/SLI-Definitionen:** Service-Level-Objectives und Error Budgets (Abschnitt 38.5)
- **Alerting-Design:** Symptom-basierte Alerts mit Multi-Window-Burn-Rates (Abschnitt 38.6)
- **Runbooks:** Standardisierte Troubleshooting-Workflows (Abschnitt 38.7)
- **Chaos Engineering:** GameDay-Checklisten und Failure-Mode-Testing (Abschnitt 38.8)
- **Capacity Planning:** Traffic-Prognosen und Headroom-Management (Abschnitt 38.10)

**Voraussetzungen:**  
Basiswissen aus Monitoring (Kapitel 19: Security Monitoring) und Troubleshooting (Kapitel 27: Operational Troubleshooting). Vertrautheit mit [Time-Series-Databases](../appendix_h_glossary.md#time-series-database) und grundlegenden Statistik-Konzepten ([Perzentile](../appendix_h_glossary.md#percentile), [Kardinalität](../appendix_h_glossary.md#cardinality)) ist empfohlen.

**Thematische Einordnung:**  
Dieses Kapitel erweitert Kapitel 19 (Monitoring-Grundlagen) um produktionsreife Observability-Praktiken. Die Performance-Metriken und Tracing-Daten dienen als Grundlage für Kapitel 39 (Performance Tuning Cookbook), während die Runbooks Kapitel 27 (Troubleshooting) mit standardisierten Incident-Response-Workflows ergänzen.

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

Abb. 38.0: Observability-Säulen

---

## 38.1 Metriken (What to Measure) {#chapter_38_1_metriken}

Metriken bilden das quantitative Fundament der [Observability](../appendix_h_glossary.md#observability) und ermöglichen datengetriebene Entscheidungen in der Systemadministration. Wir systematisieren die Metrik-Erfassung nach bewährten Methodologien (RED, USE) und ThemisDB-spezifischen Anforderungen. Dieser Abschnitt vermittelt praktische Implementierungsstrategien für produktionsreife Monitoring-Infrastrukturen mit [Prometheus](../appendix_h_glossary.md#prometheus) als primärem [Time-Series-Database](../appendix_h_glossary.md#time-series-database)-System.

### 38.1.1 RED-Metriken für Request-basierte Dienste {#chapter_38_1_1_red-metriken}

Die RED-Methodik (Rate, Errors, Duration), entwickelt von Tom Wilkie (Grafana Labs)[^red_method], fokussiert auf service-orientierte Metriken für API-Endpunkte und Query-Engines. Wir wenden diese Methodik auf ThemisDB-Requests an, um Request-Durchsatz, Fehlerquoten und Latenz-Perzentile systematisch zu erfassen.

**Rate (Requests per Second):**  
Wir messen die Anzahl eingehender Requests pro Sekunde, segmentiert nach Operation-Typ (READ, WRITE, GRAPH, VECTOR). Diese Metrik identifiziert Traffic-Muster und Lastspitzen.

```python
# Prometheus-Metrik für Request-Rate in Python (flask-prometheus-exporter)
from prometheus_client import Counter, Histogram
from flask import Flask

app = Flask(__name__)

# Counter für Gesamt-Requests pro Operation
request_counter = Counter(
    'themisdb_requests_total',
    'Total requests by operation type',
    ['operation', 'collection']  # Labels für Dimensionalität
)

# Histogram für Request-Latenz (mit Buckets für Perzentile)
request_latency = Histogram(
    'themisdb_request_duration_seconds',
    'Request duration in seconds',
    ['operation'],
    buckets=(0.005, 0.01, 0.025, 0.05, 0.1, 0.25, 0.5, 1.0, 2.5, 5.0, 10.0)
)

@app.route('/api/query', methods=['POST'])
def handle_query():
    operation = request.json.get('operation', 'READ')
    collection = request.json.get('collection', 'unknown')
    
    # Inkrementiere Counter
    request_counter.labels(operation=operation, collection=collection).inc()
    
    # Messung mit Histogram (Context Manager)
    with request_latency.labels(operation=operation).time():
        result = execute_query(request.json)
    
    return result
```

**Errors (Error Rate):**  
Wir erfassen die Fehlerquote als Prozentsatz fehlgeschlagener Requests (HTTP 5xx, Timeouts, Exceptions). Eine Error-Rate > 1% indiziert kritische Probleme.

```go
// Go-Implementierung für Error-Tracking mit Prometheus
package metrics

import (
    "github.com/prometheus/client_golang/prometheus"
    "github.com/prometheus/client_golang/prometheus/promauto"
)

var (
    // Counter für erfolgreiche Requests
    requestsSuccess = promauto.NewCounterVec(
        prometheus.CounterOpts{
            Name: "themisdb_requests_success_total",
            Help: "Total successful requests",
        },
        []string{"operation", "collection"},
    )
    
    // Counter für fehlgeschlagene Requests
    requestsErrors = promauto.NewCounterVec(
        prometheus.CounterOpts{
            Name: "themisdb_requests_errors_total",
            Help: "Total failed requests",
        },
        []string{"operation", "collection", "error_type"},
    )
)

// RecordSuccess inkrementiert Success-Counter
func RecordSuccess(operation, collection string) {
    requestsSuccess.WithLabelValues(operation, collection).Inc()
}

// RecordError inkrementiert Error-Counter mit Fehlertyp
func RecordError(operation, collection, errorType string) {
    requestsErrors.WithLabelValues(operation, collection, errorType).Inc()
}

// PromQL für Error-Rate-Berechnung:
// rate(themisdb_requests_errors_total[5m]) / 
// (rate(themisdb_requests_success_total[5m]) + rate(themisdb_requests_errors_total[5m])) * 100
```

**Duration (Latency Percentiles):**  
Wir messen Request-Latenz als Histogramm, um [Perzentile](../appendix_h_glossary.md#percentile) (P50, P95, P99, P99.9) zu berechnen. Perzentile sind robuster gegen Ausreißer als Durchschnittswerte[^brendan_gregg_latency].

**PromQL-Queries für Perzentile:**
```promql
# P50 (Median): 50% der Requests sind schneller
histogram_quantile(0.50, rate(themisdb_request_duration_seconds_bucket[5m]))

# P95: 95% der Requests sind schneller (typisches SLO-Ziel)
histogram_quantile(0.95, rate(themisdb_request_duration_seconds_bucket[5m]))

# P99: 99% der Requests sind schneller (kritische Latenz)
histogram_quantile(0.99, rate(themisdb_request_duration_seconds_bucket[5m]))

# P99.9: Worst-Case-Szenario (nur 0.1% langsamer)
histogram_quantile(0.999, rate(themisdb_request_duration_seconds_bucket[5m]))
```

**ThemisDB-spezifische RED-Metriken:**
- AQL-Query-Latenz segmentiert nach Query-Typ (FOR, GRAPH TRAVERSAL, VECTOR SEARCH)
- Throughput pro Collection (Hotspot-Identifikation)
- Error-Rate nach Fehlerklasse (TIMEOUT, DEADLOCK, OOM, INDEX_MISSING)

### 38.1.2 USE-Metriken für Ressourcen {#chapter_38_1_2_use-metriken}

Die USE-Methodik (Utilization, Saturation, Errors), entwickelt von Brendan Gregg[^use_method], fokussiert auf Hardware-Ressourcen und identifiziert Bottlenecks durch systematische Analyse von Auslastung, Sättigung und Fehlern.

**Utilization (Auslastung):**  
Wir messen die prozentuale Nutzung von CPU, Memory, Disk und Network. Utilization > 70% indiziert Kapazitätsengpässe.

**CPU-Utilization:**
```promql
# CPU-Auslastung (gesamtes System)
100 - (avg by (instance) (rate(node_cpu_seconds_total{mode="idle"}[5m])) * 100)

# CPU-Auslastung nur für ThemisDB-Prozess
rate(process_cpu_seconds_total{job="themisdb"}[5m]) * 100

# CPU-Breakdown nach Mode (user, system, iowait)
rate(node_cpu_seconds_total{mode=~"user|system|iowait"}[5m]) * 100
```

**Memory-Utilization:**
```promql
# Memory-Auslastung (Prozent)
(1 - (node_memory_MemAvailable_bytes / node_memory_MemTotal_bytes)) * 100

# ThemisDB RSS (Resident Set Size)
process_resident_memory_bytes{job="themisdb"}

# Cache Hit Rate (RocksDB-spezifisch)
rate(rocksdb_block_cache_hit[5m]) / 
(rate(rocksdb_block_cache_hit[5m]) + rate(rocksdb_block_cache_miss[5m])) * 100
```

**Saturation (Sättigung):**  
Sättigung misst Queue-Längen und Wartezeiten, die entstehen, wenn Ressourcen-Nachfrage Kapazität übersteigt. Saturation > 0 indiziert Engpässe.

**Beispiele für Saturation-Metriken:**
- **Disk Queue Depth:** Anzahl wartender I/O-Operationen (optimal: < 10)
- **Thread Pool Occupancy:** Prozentsatz aktiver Threads (optimal: < 80%)
- **Connection Queue:** Wartende TCP-Connections (optimal: 0)

```promql
# Disk Queue Depth (durchschnittliche I/O-Wartezeit)
rate(node_disk_io_time_seconds_total[5m])

# Thread Pool Saturation (ThemisDB-Worker-Threads)
themisdb_thread_pool_active / themisdb_thread_pool_max * 100

# Connection Queue (wartende Connections)
themisdb_connection_queue_length
```

**Errors (Hardware-Fehler):**  
Wir erfassen Hardware-Level-Fehler wie ECC-Memory-Errors, Disk-Timeouts und Network-Retransmits.

```promql
# Disk Errors (I/O-Fehler)
rate(node_disk_io_errors_total[5m])

# Network Retransmits (TCP-Pakete)
rate(node_netstat_TcpExt_TCPRetransSegs[5m])

# Memory Errors (ECC-Korrekturen)
rate(node_edac_correctable_errors_total[5m])
```

**RocksDB-spezifische USE-Metriken:**  
ThemisDB verwendet [RocksDB](../appendix_h_glossary.md#rocksdb) als Storage-Engine. Wir integrieren RocksDB-interne Metriken[^rocksdb_metrics]:

- **Compaction-Pending:** Anzahl wartender Compaction-Tasks (Saturation)
- **Memtable-Utilization:** Prozentsatz genutzter Memtables (Utilization)
- **Block-Cache-Errors:** Cache-Evictions durch Memory-Druck (Errors)

### 38.1.3 Cardinality-Management {#chapter_38_1_3_cardinality-management}

[Kardinalität](../appendix_h_glossary.md#cardinality) (Anzahl einzigartiger Label-Kombinationen) ist kritisch für Prometheus-Performance. Hohe Kardinalität führt zu Speicherexplosion und langsamen Queries. Wir definieren Best Practices zur Kardinalitätskontrolle.

**Cardinality Explosion Prevention:**  
Vermeide hochkardinalitäre Labels wie User-IDs, Timestamps oder Session-IDs. Nutze stattdessen Aggregations-Ebenen (z.B. `user_tier=premium` statt `user_id=12345`).

**Anti-Pattern (hochkardinal):**
```promql
# SCHLECHT: user_id als Label (Millionen Kombinationen)
themisdb_query_duration{user_id="12345", operation="READ"}
```

**Best Practice (niedrigkardinal):**
```promql
# GUT: user_tier als Label (wenige Kategorien)
themisdb_query_duration{user_tier="premium", operation="READ"}
```

**Label-Design-Regeln:**
1. **Maximal 10 Labels pro Metrik** (Prometheus-Empfehlung[^prometheus_practices])
2. **Label-Values < 1000 pro Label** (optimal: < 100)
3. **Keine dynamischen Werte** (Timestamps, UUIDs, IPs)
4. **Aggregation statt Details** (z.B. `http_status_class=5xx` statt `http_status=503`)

**Cardinality-Benchmark:**

| Metrik-Typ | Labels (Anzahl) | Kardinalität | Speicher/Tag | Query-Zeit |
|------------|-----------------|--------------|--------------|------------|
| **Niedrig** | 3 Labels, je 10 Values | ~1.000 Series | 100 MB | <50ms |
| **Mittel** | 5 Labels, je 20 Values | ~10.000 Series | 1 GB | <200ms |
| **Hoch** | 10 Labels, je 50 Values | ~100.000 Series | 10 GB | <1s |
| **Kritisch** | 15 Labels, je 100 Values | ~1.000.000 Series | 100 GB | >10s |

*Methodologie: Gemessen auf Prometheus v2.40, 16 GB RAM, 15s Scrape-Intervall, 30-Tage-Retention[^internal_benchmarks]*

**Aggregation-Techniken:**  
Nutze [Recording Rules](../appendix_h_glossary.md#recording-rules) in Prometheus, um hochkardinalitäre Metriken zu niedrigkardinalitäten Aggregaten zu reduzieren.

```yaml
# prometheus.yml - Recording Rule für Aggregation
groups:
  - name: themisdb_aggregations
    interval: 1m
    rules:
      # Aggregiere Request-Latenz von Collection-Level auf Operation-Level
      - record: themisdb:request_latency:operation
        expr: |
          histogram_quantile(0.95,
            sum(rate(themisdb_request_duration_seconds_bucket[5m])) by (operation, le)
          )
```

### Core DB Metriken

- Query Latenz (p50/p95/p99)
- Query Throughput (qps)
- Slow Queries count
- Active Connections
- Replication Lag (ms)
- WAL/Commit Queue Depth
- Memory Usage (RSS, Cache Hit Rate)
- Disk IO (IOPS, Latency, Queue Depth)
- Index Hit Rate
- Cache Evictions

### System Metriken

- CPU: user/system/iowait
- Memory: used, available, page faults
- Disk: iops, latency, utilization
- Network: rx/tx bytes, errors, retransmits

### AQL-Spezifisch

- Query Type Mix (read/write/graph/vector)
- Cursor Count / Leaks
- Transaction Duration
- Deadlocks detected
- Timeouts per operation

---

## 38.2 Logging {#chapter_38_2_logging}

[Strukturierte Logs](../appendix_h_glossary.md#structured-logging) bilden die narrative Komponente der Observability und ermöglichen Event-basierte Debugging-Workflows. Wir systematisieren Log-Formate, Level-Strategien und [Sampling](../appendix_h_glossary.md#sampling)-Techniken für High-Throughput-Systeme. Dieser Abschnitt vermittelt Best Practices für [Loki](../appendix_h_glossary.md#loki)- und [OpenSearch](../appendix_h_glossary.md#opensearch)-basierte Log-Aggregation mit [Trace-Korrelation](../appendix_h_glossary.md#trace-correlation) (siehe Abschnitt 38.3).

### 38.2.1 Strukturierte Logging-Formate {#chapter_38_2_1_strukturierte-logging-formate}

Strukturierte Logs verwenden maschinenlesbare Formate (JSON, Logfmt) statt unstrukturierten Fließtexts. Wir präferieren JSON für komplexe Felder und Logfmt für kompakte Repräsentation. Die Formatwahl beeinflusst Parsing-Performance und Speicher-Effizienz[^sridharan_observability].

**JSON vs. Logfmt Vergleich:**

| Aspekt | JSON | Logfmt | Empfehlung |
|--------|------|--------|------------|
| **Parsing-Speed** | Mittel (JSON-Parser) | Schnell (Regex) | Logfmt für Hot-Path |
| **Nested Fields** | Natürlich unterstützt | Flat-Struktur nur | JSON für Komplexität |
| **Menschenlesbarkeit** | Niedrig (verbose) | Hoch (key=value) | Logfmt für Debugging |
| **Speichergröße** | +20-30% (Braces, Quotes) | Baseline | Logfmt für Volumen |
| **Tooling** | Exzellent (jq, Elasticsearch) | Gut (grep, awk) | JSON für Integration |

**JSON-Beispiel mit deutschen Kommentaren:**

```json
{
  "ts": "2026-01-14T20:45:32.123Z",           // ISO 8601 Timestamp (UTC)
  "level": "WARN",                             // Log-Level (siehe 38.2.2)
  "service": "themisdb",                       // Service-Identifier
  "component": "aql_engine",                   // Code-Komponente
  "event": "query_timeout",                    // Event-Typ (strukturiert)
  "trace_id": "a3f9c8d2b1e40567",             // Distributed Trace-ID
  "span_id": "9c2e1f4a8b6d0537",              // Aktueller Span im Trace
  "query_id": "q_789456",                      // Query-Identifier (intern)
  "collection": "orders",                      // Betroffene Collection
  "operation": "GRAPH_TRAVERSAL",              // AQL-Operation-Typ
  "duration_ms": 5123,                         // Query-Laufzeit (Timeout bei 5000ms)
  "timeout_ms": 5000,                          // Konfigurierter Timeout
  "user_id": "u_12345",                        // User (keine PII, nur ID)
  "error": "query exceeded timeout threshold", // Human-readable Error-Message
  "stack_trace": null,                         // Optional: Stacktrace bei Exceptions
  "context": {                                 // Zusätzlicher Kontext (nested)
    "depth": 7,                                // Graph-Traversal-Tiefe
    "vertices_visited": 2847,                  // Performance-Metriken
    "edges_traversed": 8432
  }
}
```

**Logfmt-Beispiel (kompakte Alternative):**

```
ts=2026-01-14T20:45:32.123Z level=WARN service=themisdb component=aql_engine event=query_timeout trace_id=a3f9c8d2b1e40567 span_id=9c2e1f4a8b6d0537 query_id=q_789456 collection=orders operation=GRAPH_TRAVERSAL duration_ms=5123 timeout_ms=5000 user_id=u_12345 error="query exceeded timeout threshold"
```

**Feldnamen-Konventionen:**  
Wir befolgen OpenTelemetry Semantic Conventions[^otel_semantic_conventions] für konsistente Feldnamen:
- `service.name` (statt `service`, `app`, `application`)
- `http.status_code` (statt `status`, `code`, `http_code`)
- `db.operation` (statt `op`, `action`, `query_type`)

**Performance-Overhead-Analyse:**

| Format | Serialization Overhead | Parsing Overhead | Speichergröße (relativ) |
|--------|------------------------|------------------|-------------------------|
| **Plaintext** | Baseline (0%) | N/A (keine Struktur) | Baseline (100%) |
| **Logfmt** | +2-5% | +5-10% | 100-110% |
| **JSON** | +8-15% | +15-25% | 120-135% |
| **Protobuf** | +3-7% | +2-5% | 60-80% (komprimiert) |

*Methodologie: Gemessen auf ThemisDB v1.4.0, 1M Logs/sec, Python logging + JSON encoder[^internal_benchmarks]*

### 38.2.2 Log-Level-Strategie {#chapter_38_2_2_log-level-strategie}

Log-Level steuern Verbosity und Signal-to-Noise-Ratio. Wir definieren eine Hierarchie von DEBUG bis FATAL mit klaren Trigger-Kriterien für Production-Umgebungen. Die Level-Wahl beeinflusst Speicherkosten und Debugging-Fähigkeit[^loki_best_practices].

**Log-Level-Hierarchie:**

1. **DEBUG:** Detaillierte Entwickler-Information (Function Calls, Variable Values)
   - *Wann:* Development, Pre-Production Testing
   - *Beispiel:* `"Entering function parse_aql_query with input: FOR doc IN..."`

2. **INFO:** Normaler Betriebsmodus (Requests, State Changes)
   - *Wann:* Production (Baseline)
   - *Beispiel:* `"Query executed successfully, duration: 32ms, collection: users"`

3. **WARN:** Abnormale Situationen ohne Fehler (Slow Queries, Retry)
   - *Wann:* Production (Monitoring-Signal)
   - *Beispiel:* `"Query exceeded 1s threshold, duration: 1234ms, consider indexing"`

4. **ERROR:** Fehler mit Recovery-Potenzial (Timeouts, Validation Errors)
   - *Wann:* Production (Alert-Trigger)
   - *Beispiel:* `"Query failed with timeout, query_id: q_789, retrying..."`

5. **FATAL:** Kritische Fehler ohne Recovery (Data Corruption, OOM)
   - *Wann:* Production (Immediate Escalation)
   - *Beispiel:* `"RocksDB corruption detected, shutting down to prevent data loss"`

**Produktions-Verbosity-Konfiguration:**

```yaml
# themisdb_logging.yaml - Production-Konfiguration
logging:
  # Globales Default-Level
  default_level: INFO
  
  # Komponenten-spezifische Overrides
  overrides:
    aql_engine: WARN        # Nur Warnungen für Query-Engine
    storage: INFO           # Normal für Storage-Layer
    network: ERROR          # Nur Fehler für Netzwerk-Layer
    security: WARN          # Sicherheitsereignisse tracken
  
  # Dynamische Level-Anpassung (siehe 38.2.3)
  dynamic_adjustment:
    enabled: true
    error_rate_threshold: 0.05   # Bei 5% Fehler → DEBUG für 5 Minuten
    duration: 300                # Sekunden für DEBUG-Boost
```

**Dynamisches Log-Level-Adjustment:**  
Bei erhöhten Error-Rates automatisch DEBUG-Level aktivieren, um Root-Cause-Analysis zu ermöglichen. Nach Resolution wieder auf INFO reduzieren.

```python
# Python-Implementierung: Dynamisches Log-Level
import logging
import time
from prometheus_client import Gauge

# Metrik für aktuelles Log-Level
log_level_gauge = Gauge('themisdb_log_level', 'Current log level (0=DEBUG, 1=INFO, 2=WARN, 3=ERROR)')

class DynamicLogLevelManager:
    def __init__(self, logger, error_rate_threshold=0.05, boost_duration=300):
        self.logger = logger
        self.error_rate_threshold = error_rate_threshold
        self.boost_duration = boost_duration
        self.boost_until = 0
        
    def check_error_rate(self, error_rate):
        """Erhöhe Log-Level bei hoher Error-Rate"""
        if error_rate > self.error_rate_threshold:
            # Aktiviere DEBUG-Level für boost_duration Sekunden
            self.logger.setLevel(logging.DEBUG)
            self.boost_until = time.time() + self.boost_duration
            log_level_gauge.set(0)  # 0 = DEBUG
            self.logger.warning(
                f"Error rate {error_rate:.2%} exceeded threshold, "
                f"boosting log level to DEBUG for {self.boost_duration}s"
            )
        elif time.time() > self.boost_until:
            # Zurück zu INFO nach Ablauf
            self.logger.setLevel(logging.INFO)
            log_level_gauge.set(1)  # 1 = INFO
```

**Cost-Implications pro Log-Level:**

| Log-Level | Requests/sec (logging) | Throughput-Impact | Speicher/Tag (1M req/s) | Query-Latenz (Loki) |
|-----------|------------------------|-------------------|-------------------------|---------------------|
| **DEBUG** | ~10.000 | -35% | 2 TB | <500ms (viele Logs) |
| **INFO** | ~50.000 | -15% | 800 GB | <200ms |
| **WARN** | ~150.000 | -5% | 200 GB | <100ms |
| **ERROR only** | ~300.000 | -1% | 50 GB | <50ms |

*Methodologie: ThemisDB v1.4.0, 16-Core CPU, NVMe SSD, Loki v2.9, 30-Tage-Retention[^internal_benchmarks]*

### 38.2.3 Log-Sampling für High-Throughput {#chapter_38_2_3_log-sampling}

Bei hohem Request-Volumen (>100k req/s) führt vollständiges Logging zu Speicherexplosion. [Log-Sampling](../appendix_h_glossary.md#log-sampling) reduziert Volumen durch selektive Aufzeichnung, ohne kritische Events zu verlieren. Wir unterscheiden Head-based und Tail-based Sampling[^jaeger_sampling].

**Head-based Sampling (stateless):**  
Entscheidung beim Log-Event-Entstehung, basierend auf Sampling-Rate (z.B. 10% aller Requests). Vorteil: Geringe Latenz. Nachteil: Verlust kritischer Fehler-Events.

```python
# Python: Head-based Sampling (10% Sample-Rate)
import random
import logging

class SamplingLogHandler(logging.Handler):
    def __init__(self, sample_rate=0.1):
        super().__init__()
        self.sample_rate = sample_rate
        
    def emit(self, record):
        # Immer loggen bei ERROR/FATAL
        if record.levelno >= logging.ERROR:
            super().emit(record)
            return
        
        # Sampling für INFO/WARN/DEBUG
        if random.random() < self.sample_rate:
            super().emit(record)

# Verwendung
logger = logging.getLogger('themisdb')
logger.addHandler(SamplingLogHandler(sample_rate=0.1))  # 10% Sample-Rate
```

**Tail-based Sampling (stateful):**  
Entscheidung nach Request-Completion, basierend auf Outcome (Fehler, hohe Latenz). Vorteil: Keine Fehler-Verluste. Nachteil: Buffering-Overhead. Implementiert in [Grafana Tempo](../appendix_h_glossary.md#grafana-tempo)[^tempo_docs].

**Adaptive Sampling:**  
Dynamische Sample-Rate basierend auf Error-Rate: Bei Fehlern 100%, bei Normalzustand 1-10%.

```yaml
# Loki-Konfiguration: Adaptive Sampling via LogQL
# Behalte alle ERROR-Level-Logs + 10% der INFO-Logs
{level="ERROR"}
|= ""
OR
{level="INFO"}
|= ""
| sample 0.1  # 10% Sampling für INFO
```

**Performance-Impact-Benchmarks:**

| Sampling-Strategie | CPU-Overhead | Latenz-Impact | Speicher-Reduktion | Fehler-Coverage |
|--------------------|--------------|---------------|--------------------|-----------------|
| **Kein Sampling** | Baseline | Baseline | 0% (100% Logs) | 100% |
| **Head-based 10%** | -8% | -12ms | 90% (10% Logs) | ~90% (Fehler können fehlen) |
| **Tail-based (Error)** | +5% (Buffering) | +5ms | 70-90% (variabel) | 100% (alle Fehler) |
| **Adaptive** | +2% | +2ms | 80-95% (dynamisch) | 100% |

*Methodologie: ThemisDB v1.4.0, 100k req/s, Loki v2.9, 7-Tage-Retention[^internal_benchmarks]*

### 38.2.4 Log-Korrelation mit Distributed Tracing {#chapter_38_2_4_log-korrelation}

Log-Korrelation verbindet Logs mit [Distributed Traces](../appendix_h_glossary.md#distributed-tracing) (siehe Abschnitt 38.3) via Trace-ID und Span-ID. Dies ermöglicht End-to-End-Debugging über Microservice-Grenzen hinweg (siehe Kapitel 19 für Security-Kontext und Kapitel 27 für Troubleshooting-Workflows).

**Trace-ID-Injektion:**  
Wir propagieren `trace_id` und `span_id` aus [OpenTelemetry](../appendix_h_glossary.md#opentelemetry)-Context in jeden Log-Entry.

```python
# Python: OpenTelemetry + Logging-Integration
from opentelemetry import trace
from opentelemetry.sdk.trace import TracerProvider
from opentelemetry.sdk.trace.export import BatchSpanProcessor
from opentelemetry.exporter.otlp.proto.grpc.trace_exporter import OTLPSpanExporter
import logging

# Tracer-Setup (siehe 38.3.1)
trace.set_tracer_provider(TracerProvider())
tracer = trace.get_tracer(__name__)

# Logging-Setup mit Trace-Kontext
class TraceContextFilter(logging.Filter):
    """Injiziere Trace-ID und Span-ID in Log-Records"""
    def filter(self, record):
        span = trace.get_current_span()
        if span and span.get_span_context().is_valid:
            ctx = span.get_span_context()
            record.trace_id = format(ctx.trace_id, '032x')  # 32-char Hex
            record.span_id = format(ctx.span_id, '016x')    # 16-char Hex
        else:
            record.trace_id = '00000000000000000000000000000000'
            record.span_id = '0000000000000000'
        return True

# Logger-Konfiguration
logger = logging.getLogger('themisdb')
logger.addFilter(TraceContextFilter())
logger.setLevel(logging.INFO)

# JSON-Formatter mit Trace-Feldern
import json
class JSONFormatter(logging.Formatter):
    def format(self, record):
        log_obj = {
            'ts': self.formatTime(record, self.datefmt),
            'level': record.levelname,
            'service': 'themisdb',
            'trace_id': record.trace_id,
            'span_id': record.span_id,
            'message': record.getMessage()
        }
        return json.dumps(log_obj)

handler = logging.StreamHandler()
handler.setFormatter(JSONFormatter())
logger.addHandler(handler)

# Verwendung mit Tracing
@tracer.start_as_current_span("execute_query")
def execute_query(query_str):
    logger.info(f"Executing query: {query_str[:50]}...")
    # Query-Logik...
    logger.info("Query executed successfully")
```

**Loki/Jaeger-Integration:**  
In [Grafana](../appendix_h_glossary.md#grafana) können wir von Logs zu Traces navigieren via `trace_id`-Hyperlinks.

```yaml
# Grafana Datasource-Konfiguration: Loki → Tempo
apiVersion: 1
datasources:
  - name: Loki
    type: loki
    url: http://loki:3100
    jsonData:
      # Aktiviere Trace-to-Logs Navigation
      derivedFields:
        - datasourceUid: tempo_uid
          matcherRegex: "trace_id=(\\w+)"
          name: TraceID
          url: "$${__value.raw}"
```

### Log-Pipeline

- Agent: Filebeat/FluentBit (TLS, backpressure)
- Parse: grok/json; enrich (host, cluster, env)
- Store: Loki/Elastic/OpenSearch
- Retention: 7-30 Tage (Hot), 90+ Tage (Cold/Glacier)

---

## 38.3 Tracing {#chapter_38_3_tracing}

[Distributed Tracing](../appendix_h_glossary.md#distributed-tracing) visualisiert Request-Flows über Microservice-Grenzen hinweg und identifiziert Latenz-Bottlenecks durch [Span](../appendix_h_glossary.md#span)-basierte Kausalitäts-Graphen. Wir implementieren [OpenTelemetry](../appendix_h_glossary.md#opentelemetry)-kompatibles Tracing mit [W3C Trace Context](../appendix_h_glossary.md#w3c-trace-context)-Propagierung für ThemisDB-Requests. Dieser Abschnitt vermittelt Sampling-Strategien, Visualisierungs-Patterns und Performance-Overhead-Analysen für produktionsreife Tracing-Infrastrukturen (siehe auch Kapitel 39 für Performance-Tuning-Kontext).

### 38.3.1 OpenTelemetry-Architektur {#chapter_38_3_1_opentelemetry-architektur}

OpenTelemetry (OTel) ist der CNCF-Standard für Observability-Instrumentierung[^otel_specification]. Wir nutzen OTel SDKs für automatische und manuelle Instrumentierung von ThemisDB-Komponenten. Die Architektur basiert auf [Tracern](../appendix_h_glossary.md#tracer), Spans und [Exportern](../appendix_h_glossary.md#exporter).

**Core-Konzepte:**
- **Tracer:** Factory für Span-Erstellung (pro Service/Komponente)
- **Span:** Repräsentiert eine Operation mit Start/End-Zeit und Metadaten
- **Trace:** Baum von verbundenen Spans (visualisiert Request-Flow)
- **Context Propagation:** Übertragung von Trace-ID/Span-ID über RPC/HTTP-Grenzen
- **Exporter:** Sender für Spans an Backend (Jaeger, Tempo, Zipkin)

**Go-Implementierung: Tracer-Initialisierung**

```go
// tracing/init.go - OpenTelemetry-Setup für ThemisDB (Go)
package tracing

import (
    "context"
    "log"
    
    "go.opentelemetry.io/otel"
    "go.opentelemetry.io/otel/exporters/otlp/otlptrace"
    "go.opentelemetry.io/otel/exporters/otlp/otlptrace/otlptracegrpc"
    "go.opentelemetry.io/otel/sdk/resource"
    sdktrace "go.opentelemetry.io/otel/sdk/trace"
    semconv "go.opentelemetry.io/otel/semconv/v1.17.0"
)

// InitTracer initialisiert OpenTelemetry mit OTLP-Exporter
// endpoint: z.B. "tempo:4317" für Grafana Tempo
// serviceName: z.B. "themisdb-api"
func InitTracer(endpoint, serviceName string) func() {
    ctx := context.Background()
    
    // 1. OTLP gRPC Exporter erstellen
    exporter, err := otlptracegrpc.New(
        ctx,
        otlptracegrpc.WithEndpoint(endpoint),
        otlptracegrpc.WithInsecure(), // In Produktion: TLS aktivieren
    )
    if err != nil {
        log.Fatalf("Failed to create OTLP exporter: %v", err)
    }
    
    // 2. Resource-Attribute definieren (Service-Metadaten)
    res, err := resource.New(
        ctx,
        resource.WithAttributes(
            semconv.ServiceNameKey.String(serviceName),
            semconv.ServiceVersionKey.String("1.4.0"),
            semconv.DeploymentEnvironmentKey.String("production"),
        ),
    )
    if err != nil {
        log.Fatalf("Failed to create resource: %v", err)
    }
    
    // 3. TracerProvider mit Batch-Processor (Performance-Optimierung)
    tp := sdktrace.NewTracerProvider(
        sdktrace.WithBatcher(exporter),  // Batch statt einzeln (reduziert Overhead)
        sdktrace.WithResource(res),
        sdktrace.WithSampler(
            // Probabilistic Sampling: 10% aller Traces (siehe 38.3.2)
            sdktrace.ParentBased(
                sdktrace.TraceIDRatioBased(0.1),  // 10% Sample-Rate
            ),
        ),
    )
    
    // 4. Global Tracer-Provider registrieren
    otel.SetTracerProvider(tp)
    
    // 5. Cleanup-Funktion zurückgeben (defer in main())
    return func() {
        if err := tp.Shutdown(ctx); err != nil {
            log.Printf("Error shutting down tracer provider: %v", err)
        }
    }
}
```

**Python-Implementierung: Span-Erstellung**

```python
# tracing/aql_engine.py - Span-Instrumentierung für AQL-Engine (Python)
from opentelemetry import trace
from opentelemetry.trace import Status, StatusCode
from opentelemetry.semconv.trace import SpanAttributes
import time

# Tracer-Instanz für AQL-Engine
tracer = trace.get_tracer("themisdb.aql_engine", version="1.4.0")

def execute_query(query_str, collection, operation_type):
    """
    Führt AQL-Query aus mit vollständiger Trace-Instrumentierung
    """
    # Start neuer Span für Query-Execution
    with tracer.start_as_current_span(
        "aql.execute_query",
        kind=trace.SpanKind.INTERNAL,  # INTERNAL für interne Operationen
        attributes={
            # Semantic Conventions für Datenbank-Operationen
            SpanAttributes.DB_SYSTEM: "themisdb",
            SpanAttributes.DB_OPERATION: operation_type,
            SpanAttributes.DB_STATEMENT: query_str[:200],  # Erste 200 Zeichen
            "db.collection": collection,
            "query.length": len(query_str),
        }
    ) as span:
        try:
            start = time.time()
            
            # 1. Query Parsing (Sub-Span)
            with tracer.start_as_current_span("aql.parse") as parse_span:
                ast = parse_aql(query_str)
                parse_span.set_attribute("ast.nodes", len(ast.nodes))
            
            # 2. Query Optimization (Sub-Span)
            with tracer.start_as_current_span("aql.optimize") as opt_span:
                plan = optimize_query(ast)
                opt_span.set_attribute("plan.steps", len(plan.steps))
                opt_span.set_attribute("plan.uses_index", plan.has_index)
            
            # 3. Query Execution (Sub-Span)
            with tracer.start_as_current_span("aql.execute") as exec_span:
                result = run_query_plan(plan)
                exec_span.set_attribute("result.rows", len(result))
            
            # Erfolg: Status OK
            duration = time.time() - start
            span.set_status(Status(StatusCode.OK))
            span.set_attribute("query.duration_ms", duration * 1000)
            span.set_attribute("query.result_count", len(result))
            
            return result
            
        except TimeoutError as e:
            # Fehler: Status ERROR mit Exception-Details
            span.record_exception(e)  # Automatisch: exception.type, exception.message, exception.stacktrace
            span.set_status(Status(StatusCode.ERROR, "Query timeout"))
            span.set_attribute("error.type", "timeout")
            raise
        
        except Exception as e:
            span.record_exception(e)
            span.set_status(Status(StatusCode.ERROR, str(e)))
            raise
```

**W3C Trace Context Propagation:**  
Wir propagieren Trace-Context über HTTP-Header nach W3C-Standard[^w3c_trace_context]:

```http
# HTTP-Request-Header (Client → ThemisDB)
traceparent: 00-4bf92f3577b34da6a3ce929d0e0e4736-00f067aa0ba902b7-01
tracestate: vendor1=value1,vendor2=value2

# Format: traceparent = version-trace_id-span_id-flags
# version: 00 (fixed)
# trace_id: 32-char hex (128-bit)
# span_id: 16-char hex (64-bit)
# flags: 01 = sampled, 00 = not sampled
```

### 38.3.2 Sampling-Strategien {#chapter_38_3_2_sampling-strategien}

[Sampling](../appendix_h_glossary.md#sampling) reduziert Tracing-Overhead durch selektive Aufzeichnung von Traces. Wir unterscheiden Head-based, Tail-based und Adaptive Sampling mit unterschiedlichen Trade-offs zwischen Performance und Observability[^jaeger_sampling].

**Head-based Sampling (Probabilistic):**  
Entscheidung beim Trace-Start basierend auf Wahrscheinlichkeit. Vorteil: Minimaler Overhead. Nachteil: Fehler-Traces können fehlen.

```go
// Go: Probabilistic Sampler (10% Sample-Rate)
import "go.opentelemetry.io/otel/sdk/trace"

sampler := trace.TraceIDRatioBased(0.1)  // 10% aller Traces
```

**Tail-based Sampling (Outcome-driven):**  
Entscheidung nach Trace-Completion basierend auf Outcome (Error, High Latency). Implementiert durch Trace-Buffering im Collector. Vorteil: Alle kritischen Traces. Nachteil: Memory-Overhead für Buffering.

```yaml
# OpenTelemetry Collector: Tail-Sampling-Processor
processors:
  tail_sampling:
    decision_wait: 10s  # Warte 10s auf Trace-Completion
    num_traces: 100000  # Buffer-Größe
    policies:
      # Policy 1: Alle Error-Traces behalten
      - name: errors_policy
        type: status_code
        status_code:
          status_codes: [ERROR]
      
      # Policy 2: Alle Traces > 1s Latenz behalten
      - name: latency_policy
        type: latency
        latency:
          threshold_ms: 1000
      
      # Policy 3: Probabilistic für normale Traces (1%)
      - name: probabilistic_policy
        type: probabilistic
        probabilistic:
          sampling_percentage: 1.0
```

**Adaptive Sampling:**  
Dynamische Sample-Rate basierend auf System-Load oder Error-Rate. Bei hoher Last: Reduziere Rate. Bei Errors: Erhöhe Rate.

**Sampling-Rate-Vergleich:**

| Sampling-Rate | CPU-Overhead | Memory-Overhead | Storage/Tag (1M traces) | Detail-Level | Error-Coverage |
|---------------|--------------|-----------------|-------------------------|--------------|----------------|
| **100%** | +12% | +150 MB | 500 GB | Full Visibility | 100% |
| **50%** | +6% | +80 MB | 250 GB | Hoch | ~50% (probabilistic) |
| **10%** | +2% | +20 MB | 50 GB | Mittel | ~10% (probabilistic) |
| **1%** | +0.5% | +5 MB | 5 GB | Niedrig | ~1% (probabilistic) |
| **Tail-based** | +5% (Buffering) | +200 MB (Buffer) | 50-100 GB | Hoch | 100% (alle Errors) |

*Methodologie: ThemisDB v1.4.0, 100k req/s, OpenTelemetry Collector v0.85, Tempo v2.2, 7-Tage-Retention[^internal_benchmarks]*

### 38.3.3 Trace-Visualisierung {#chapter_38_3_3_trace-visualisierung}

Trace-Visualisierung erfolgt primär in [Jaeger](../appendix_h_glossary.md#jaeger) oder [Grafana Tempo](../appendix_h_glossary.md#grafana-tempo). Wir analysieren Waterfall-Diagramme, Service-Dependency-Graphen und Critical-Path-Analysen für Performance-Optimierung (siehe Kapitel 39).

**Jaeger-Integration:**

```yaml
# docker-compose.yml - Jaeger All-in-One
version: '3'
services:
  jaeger:
    image: jaegertracing/all-in-one:1.50
    environment:
      - COLLECTOR_OTLP_ENABLED=true  # OpenTelemetry-Protokoll
    ports:
      - "16686:16686"  # Jaeger UI
      - "4317:4317"    # OTLP gRPC
      - "4318:4318"    # OTLP HTTP
```

**Grafana Tempo-Konfiguration:**

```yaml
# tempo.yaml - Grafana Tempo Config
server:
  http_listen_port: 3200

distributor:
  receivers:
    otlp:
      protocols:
        grpc:
          endpoint: 0.0.0.0:4317

storage:
  trace:
    backend: s3  # S3-kompatibles Object Storage
    s3:
      bucket: tempo-traces
      endpoint: minio:9000

query_frontend:
  search:
    duration_slo: 5s      # Query-Timeout
    throughput_bytes_slo: 1GB
```

**Service-Dependency-Graph:**  
Jaeger generiert automatisch Service-Dependency-Graphen aus Trace-Daten, die Microservice-Interaktionen visualisieren.

**Critical-Path-Analysis:**  
Identifikation der langsamsten Span-Sequenz im Trace (kritischer Pfad für Latenz-Optimierung).

```python
# Python: Critical-Path-Berechnung aus Trace
def find_critical_path(trace):
    """
    Findet kritischen Pfad (längste Span-Sequenz) im Trace
    """
    spans = trace['spans']
    
    # Baue Dependency-Graph
    children = {span['spanID']: [] for span in spans}
    for span in spans:
        if 'references' in span:
            for ref in span['references']:
                if ref['refType'] == 'CHILD_OF':
                    parent_id = ref['spanID']
                    children[parent_id].append(span)
    
    # Rekursive Critical-Path-Suche
    def max_path_duration(span_id):
        span = next(s for s in spans if s['spanID'] == span_id)
        duration = span['duration']
        
        if not children[span_id]:
            return duration, [span]
        
        # Finde längstes Kind
        max_child_duration = 0
        max_child_path = []
        for child in children[span_id]:
            child_duration, child_path = max_path_duration(child['spanID'])
            if child_duration > max_child_duration:
                max_child_duration = child_duration
                max_child_path = child_path
        
        return duration + max_child_duration, [span] + max_child_path
    
    root_span = next(s for s in spans if 'references' not in s or not s['references'])
    total_duration, path = max_path_duration(root_span['spanID'])
    
    return {
        'total_duration_us': total_duration,
        'spans': [{'operation': s['operationName'], 'duration_us': s['duration']} for s in path]
    }
```

### 38.3.4 Performance-Overhead {#chapter_38_3_4_performance-overhead}

Tracing-Overhead entsteht durch Span-Erstellung, Context-Propagierung und Span-Export. Wir quantifizieren Overhead und definieren Mitigations-Strategien für High-Throughput-Systeme.

**Instrumentation-Overhead-Komponenten:**
1. **Span-Erstellung:** CPU-Kosten für Span-Objekt-Allokation (~5-10 µs)
2. **Attribute-Serialization:** JSON-Encoding von Span-Daten (~20-50 µs)
3. **Context-Propagierung:** Thread-Local-Storage-Zugriff (~1-2 µs)
4. **Network-Export:** gRPC-Call zum Collector (~100-500 µs, asynchron)

**Performance-Overhead-Messungen:**

| Metrik | Ohne Tracing | Mit Tracing (100% Sampling) | Overhead | Mit Tracing (10% Sampling) | Overhead |
|--------|--------------|----------------------------|----------|---------------------------|----------|
| **Avg Latency** | 25ms | 28ms | +12% | 25.5ms | +2% |
| **P99 Latency** | 120ms | 135ms | +12.5% | 123ms | +2.5% |
| **Throughput** | 50k req/s | 44k req/s | -12% | 49k req/s | -2% |
| **CPU Usage** | 45% | 52% | +15% | 46% | +2% |
| **Memory** | 2 GB | 2.15 GB | +7.5% | 2.02 GB | +1% |

*Methodologie: ThemisDB v1.4.0, 16-Core CPU, OpenTelemetry SDK v1.20, OTLP gRPC Exporter, Batch-Size 512[^internal_benchmarks]*

**Optimierungsstrategien:**
1. **Batch-Export:** Spans in Batches exportieren (512-1024 Spans) statt einzeln
2. **Asynchrones Buffering:** Span-Export in separatem Thread (non-blocking)
3. **Attribute-Reduktion:** Nur essentielle Attribute (<10 pro Span)
4. **Adaptive Sampling:** Reduziere Sample-Rate bei hoher Last

```go
// Go: Performance-Optimierung - Batch-Exporter
import "go.opentelemetry.io/otel/sdk/trace"

tp := trace.NewTracerProvider(
    trace.WithBatcher(
        exporter,
        trace.WithBatchTimeout(5 * time.Second),   // Batch-Intervall
        trace.WithMaxExportBatchSize(1024),        // Max. Spans pro Batch
        trace.WithMaxQueueSize(8192),              // Queue-Size für Backpressure
    ),
)
```

### AQL Trace-Injektion

- Propagiere `traceparent` Header vom API Gateway ins AQL Layer
- Schreibe `trace_id` in Query-Context → Log-Korrelation
- Sample Rate: 1-10% in Produktion; 100% in Staging

---

## 38.4 Dashboards (Grafana) {#chapter_38_4_dashboards}

[Grafana](../appendix_h_glossary.md#grafana)-Dashboards visualisieren die in Abschnitt 38.1 definierten Metriken und transformieren Rohdaten in handlungsrelevante Insights. Wir präsentieren Dashboard-Architekturen für ThemisDB-spezifische Observability-Anforderungen mit [Prometheus](../appendix_h_glossary.md#prometheus) als primärer Datenquelle. Dashboard-as-Code-Praktiken gewährleisten Reproduzierbarkeit und Versionskontrolle. Dieser Abschnitt vermittelt PromQL-Query-Patterns, Panel-Layout-Strategien und Best Practices für Operations-Teams.

### 38.4.1 Dashboard-Architektur {#chapter_38_4_1_dashboard-architektur}

Wir strukturieren Dashboards nach dem Golden Signals-Prinzip (Latenz, Traffic, Errors, Saturation) und organisieren Panels in logischen Gruppen. Die Hierarchie folgt dem Top-Down-Debugging-Workflow: Übersicht → Drill-Down → Root-Cause-Analysis.

**Dashboard-Hierarchie:**

1. **Overview Dashboard:** High-level KPIs für Management (Availability, Error Rate, Traffic)
2. **Component Dashboards:** Pro Service/Component (AQL Engine, Storage Layer, Replication)
3. **Detail Dashboards:** Tiefe Diagnostik (Slow Queries, Cache Analysis, Thread Pools)

**Grafana-Provisioning mit YAML:**  
Dashboard-as-Code ermöglicht Git-basierte Versionskontrolle und automatisiertes Deployment.

```yaml
# grafana/provisioning/dashboards/themisdb.yaml - Dashboard-Provider-Konfiguration
apiVersion: 1

providers:
  - name: 'ThemisDB Dashboards'
    orgId: 1
    folder: 'ThemisDB'
    type: file
    disableDeletion: false
    updateIntervalSeconds: 10
    allowUiUpdates: true
    options:
      path: /etc/grafana/dashboards/themisdb
      foldersFromFilesStructure: true
```

### 38.4.2 Latenz & Throughput Dashboard {#chapter_38_4_2_latenz-throughput}

Latenz-Visualisierung nutzt Heatmaps für Perzentil-Verteilungen und Time-Series-Panels für Trend-Analyse. Wir kombinieren RED-Metriken (siehe Abschnitt 38.1.1) in einem kohärenten Dashboard-Layout.

**Panel 1: Query Latency Percentiles (Time Series)**

```json
{
  "title": "ThemisDB Query Latency (P50/P95/P99)",
  "type": "timeseries",
  "datasource": "Prometheus",
  "targets": [
    {
      "expr": "histogram_quantile(0.50, sum(rate(themisdb_request_duration_seconds_bucket[5m])) by (operation, le))",
      "legendFormat": "P50 - {{operation}}",
      "refId": "A"
    },
    {
      "expr": "histogram_quantile(0.95, sum(rate(themisdb_request_duration_seconds_bucket[5m])) by (operation, le))",
      "legendFormat": "P95 - {{operation}}",
      "refId": "B"
    },
    {
      "expr": "histogram_quantile(0.99, sum(rate(themisdb_request_duration_seconds_bucket[5m])) by (operation, le))",
      "legendFormat": "P99 - {{operation}}",
      "refId": "C"
    }
  ],
  "fieldConfig": {
    "defaults": {
      "unit": "s",
      "thresholds": {
        "mode": "absolute",
        "steps": [
          {"value": 0, "color": "green"},
          {"value": 0.2, "color": "yellow"},
          {"value": 0.5, "color": "red"}
        ]
      }
    }
  }
}
```

**Panel 2: Throughput by Operation (Stacked Area Chart)**

```promql
# PromQL: Requests per Second (Rate) nach Operation-Typ
sum(rate(themisdb_requests_total[5m])) by (operation)

# Legend: READ, WRITE, GRAPH_TRAVERSAL, VECTOR_SEARCH
```

**Panel 3: Error Rate Percentage (Gauge)**

```promql
# PromQL: Error-Rate als Prozentsatz
(
  sum(rate(themisdb_requests_errors_total[5m]))
  /
  (sum(rate(themisdb_requests_success_total[5m])) + sum(rate(themisdb_requests_errors_total[5m])))
) * 100

# Thresholds:
# Green: < 0.1% (Excellent)
# Yellow: 0.1-1% (Warning)
# Red: > 1% (Critical)
```

### 38.4.3 Ressourcen-Dashboard {#chapter_38_4_3_ressourcen-dashboard}

Ressourcen-Monitoring basiert auf USE-Metriken (siehe Abschnitt 38.1.2) und identifiziert Hardware-Bottlenecks. Wir nutzen [Node Exporter](../appendix_h_glossary.md#node-exporter)-Metriken für System-Level-Observability.

**Panel: CPU Utilization per Node (Heatmap)**

```promql
# PromQL: CPU-Auslastung pro Node (0-100%)
100 - (avg by (instance) (rate(node_cpu_seconds_total{mode="idle"}[5m])) * 100)

# Heatmap-Buckets: 0-10%, 10-20%, ..., 90-100%
```

**Panel: Memory Pressure Indicators**

```json
{
  "title": "Memory Pressure (RSS + Cache Hit Rate)",
  "type": "graph",
  "targets": [
    {
      "expr": "process_resident_memory_bytes{job=\"themisdb\"} / 1024 / 1024 / 1024",
      "legendFormat": "RSS (GB) - {{instance}}"
    },
    {
      "expr": "(rate(rocksdb_block_cache_hit[5m]) / (rate(rocksdb_block_cache_hit[5m]) + rate(rocksdb_block_cache_miss[5m]))) * 100",
      "legendFormat": "Cache Hit Rate (%) - {{instance}}",
      "yAxisIndex": 1
    }
  ],
  "yaxes": [
    {"label": "Memory (GB)", "format": "short"},
    {"label": "Hit Rate (%)", "format": "percent"}
  ]
}
```

**Panel: Disk I/O Saturation**

```promql
# PromQL: Disk Queue Depth (Saturation-Indikator)
rate(node_disk_io_time_seconds_total[5m])

# Interpretation:
# < 1.0: Keine Saturation
# 1.0-5.0: Moderate Saturation
# > 5.0: Kritische Saturation (I/O-Bottleneck)
```

### 38.4.4 Replication & Consistency Dashboard {#chapter_38_4_4_replication-consistency}

Replication-Monitoring visualisiert Lag-Metriken und Consistency-Indikatoren für Multi-Node-Deployments. Kritisch für [Eventual Consistency](../appendix_h_glossary.md#eventual-consistency)-Szenarien.

**Panel: Replication Lag per Follower**

```promql
# PromQL: Replication Lag in Millisekunden
themisdb_replication_lag_milliseconds

# Alert-Threshold: > 2000ms (2 Sekunden)
```

**Panel: WAL (Write-Ahead Log) Queue Depth**

```promql
# PromQL: WAL Queue Depth (Indikator für Write-Backpressure)
themisdb_wal_queue_depth

# Interpretation:
# < 100: Normal
# 100-1000: Moderate Backpressure
# > 1000: Kritisch (Throttle Writes)
```

**Panel: Failed Replication Events (Counter)**

```promql
# PromQL: Fehlgeschlagene Replikationen (Rate)
rate(themisdb_replication_failures_total[5m])

# Alert bei > 0 (jede Replikations-Fehler ist kritisch)
```

### 38.4.5 Dashboard-Layout Best Practices {#chapter_38_4_5_dashboard-layout}

Dashboard-UX beeinflusst MTTR (Mean Time To Resolution) maßgeblich. Wir befolgen Gestalt-Prinzipien für visuelle Hierarchie und Informationsarchitektur.

**Layout-Regeln:**

1. **F-Pattern Reading:** Kritische Metriken oben-links (Blickverlauf)
2. **Color Semantics:** Rot=Fehler, Gelb=Warnung, Grün=OK, Blau=Info
3. **Contextual Grouping:** Verwandte Panels gruppieren (z.B. CPU + Memory in einem Row)
4. **Progressive Disclosure:** Overview → Detail via Drill-Down-Links

**Grafana Variables für Filterung:**

```json
{
  "templating": {
    "list": [
      {
        "name": "instance",
        "type": "query",
        "datasource": "Prometheus",
        "query": "label_values(themisdb_requests_total, instance)",
        "multi": true,
        "includeAll": true
      },
      {
        "name": "operation",
        "type": "custom",
        "options": ["READ", "WRITE", "GRAPH_TRAVERSAL", "VECTOR_SEARCH"],
        "multi": true,
        "includeAll": true
      }
    ]
  }
}
```

**PromQL mit Variables:**

```promql
# Nutzung von Dashboard-Variables für dynamische Filterung
sum(rate(themisdb_requests_total{instance=~"$instance", operation=~"$operation"}[5m]))
```

### 38.4.6 Dashboard Performance Optimization {#chapter_38_4_6_dashboard-performance}

Dashboard-Query-Performance beeinflusst User Experience. Wir optimieren PromQL-Queries und Refresh-Intervalle für Balance zwischen Aktualität und Server-Load.

**Optimization-Strategien:**

| Strategie | Beschreibung | Performance-Gain |
|-----------|--------------|------------------|
| **Recording Rules** | Pre-compute komplexe Queries | 10-100× schneller |
| **Query Caching** | Grafana-Cache für wiederholte Queries | 2-5× schneller |
| **Time Range Limiting** | Max. 24h für High-Resolution-Dashboards | 3-10× schneller |
| **Downsampling** | Niedrigere Resolution für lange Zeiträume | 5-20× schneller |

**Prometheus Recording Rule Beispiel:**

```yaml
# prometheus-rules.yaml - Pre-computed Aggregationen
groups:
  - name: themisdb_dashboard_optimizations
    interval: 1m
    rules:
      # Recording Rule für P95 Latenz (statt On-the-Fly-Berechnung)
      - record: themisdb:request_latency_p95:operation
        expr: |
          histogram_quantile(0.95,
            sum(rate(themisdb_request_duration_seconds_bucket[5m])) by (operation, le)
          )
      
      # Recording Rule für Error-Rate
      - record: themisdb:error_rate:percent
        expr: |
          (
            sum(rate(themisdb_requests_errors_total[5m]))
            /
            (sum(rate(themisdb_requests_success_total[5m])) + sum(rate(themisdb_requests_errors_total[5m])))
          ) * 100
```

**Dashboard Refresh-Intervalle:**

- **Overview Dashboards:** 30s (niedrige Query-Last)
- **Detail Dashboards:** 1min (moderate Last)
- **Historical Analysis:** Manual Refresh (keine Auto-Refresh-Last)

---

## 38.5 SLI/SLO & Error Budgets {#chapter_38_5_sli-slo-error-budgets}

[Service Level Indicators](../appendix_h_glossary.md#sli) (SLIs) und [Service Level Objectives](../appendix_h_glossary.md#slo) (SLOs) quantifizieren Reliability-Ziele und ermöglichen datengetriebene Release-Entscheidungen. Wir definieren SLIs nach dem Google SRE-Framework[^google_sre_book] und implementieren [Error Budget](../appendix_h_glossary.md#error-budget)-Management für Balance zwischen Innovation und Stabilität. Dieser Abschnitt vermittelt SLO-Kalkulations-Methoden, Burn-Rate-Alerting und Budget-Policy-Strategien für ThemisDB-Deployments.

### 38.5.1 SLI-Definition & Measurement {#chapter_38_5_1_sli-definition}

SLIs sind quantifizierbare Metriken, die User-Experience direkt abbilden. Wir fokussieren auf Request-basierte SLIs (Availability, Latency) und Data-basierte SLIs (Durability, Freshness) für ThemisDB-Workloads.

**SLI-Kategorien für ThemisDB:**

| SLI-Typ | Metrik | Formel | User-Impact |
|---------|--------|--------|-------------|
| **Availability** | Request Success Rate | `good_requests / total_requests` | Service erreichbar? |
| **Latency** | Request Duration | `requests_under_threshold / total_requests` | Service schnell genug? |
| **Durability** | Data Loss Events | `successful_writes_persisted / total_writes` | Daten sicher gespeichert? |
| **Freshness** | Replication Lag | `replicas_within_lag_threshold / total_replicas` | Daten aktuell? |

**PromQL für Availability-SLI:**

```promql
# SLI: Availability (HTTP 2xx/3xx Responses als "Good")
sum(rate(themisdb_requests_total{status=~"2..|3.."}[30d]))
/
sum(rate(themisdb_requests_total[30d]))

# Beispiel-Resultat: 0.9998 (99.98% Availability über 30 Tage)
```

**PromQL für Latency-SLI (Threshold-basiert):**

```promql
# SLI: Latency (P99 < 200ms als "Good")
(
  sum(rate(themisdb_request_duration_seconds_bucket{le="0.2"}[30d]))
  /
  sum(rate(themisdb_request_duration_seconds_count[30d]))
)

# Interpretation: Prozentsatz der Requests unter 200ms
```

**Window-basierte vs. Request-basierte SLIs:**

- **Request-based:** Jeder Request zählt gleich (typisch für APIs)
- **Window-based:** Aggregation über Zeitfenster (typisch für Batch-Jobs)

Wir nutzen Request-based SLIs für ThemisDB-REST-APIs und Window-based SLIs für Hintergrund-Prozesse (Compaction, Replication).

### 38.5.2 SLO-Kalkulation & Target-Setting {#chapter_38_5_2_slo-kalkulation}

SLOs definieren Reliability-Ziele als Prozentsatz (z.B. 99.9% Availability). Die SLO-Wahl balanciert User-Erwartungen, Kosten und Engineering-Aufwand. Zu hohe SLOs verschwenden Ressourcen, zu niedrige SLOs schaden User-Experience.

**SLO-Kalkulations-Formel:**

```
SLO = (1 - Error_Budget_Percentage) * 100%

Error_Budget = 1 - SLO

Beispiel:
SLO = 99.9% → Error Budget = 0.1% = 0.001
```

**Downtime-Kalkulation pro SLO:**

| SLO | Error Budget | Downtime/Monat | Downtime/Jahr | Kosten-Impact |
|-----|--------------|----------------|---------------|---------------|
| **99%** | 1% | 7.2 Stunden | 3.65 Tage | Low (Standard-Tier) |
| **99.5%** | 0.5% | 3.6 Stunden | 1.83 Tage | Medium |
| **99.9%** | 0.1% | 43 Minuten | 8.76 Stunden | High (Premium-Tier) |
| **99.95%** | 0.05% | 21 Minuten | 4.38 Stunden | Very High |
| **99.99%** | 0.01% | 4.3 Minuten | 52.6 Minuten | Extreme (Mission-Critical) |

*Formel: `Downtime = (1 - SLO) * Zeitperiode`*

**ThemisDB SLO-Beispiele:**

```yaml
# slo-config.yaml - SLO-Definitionen für ThemisDB
slos:
  - name: "api-availability"
    description: "REST API Availability"
    target: 99.95  # 99.95%
    window: 30d
    sli_query: |
      sum(rate(themisdb_requests_total{status=~"2..|3.."}[30d]))
      /
      sum(rate(themisdb_requests_total[30d]))
  
  - name: "read-latency-p99"
    description: "Read Query P99 Latency < 200ms"
    target: 99.9  # 99.9% der Reads unter 200ms
    window: 30d
    sli_query: |
      histogram_quantile(0.99,
        sum(rate(themisdb_request_duration_seconds_bucket{operation="READ"}[30d])) by (le)
      ) < 0.2
  
  - name: "data-durability"
    description: "Zero Data Loss"
    target: 100.0  # 100% (keine Toleranz für Datenverlust)
    window: 30d
    sli_query: |
      sum(rate(themisdb_writes_committed_total[30d]))
      /
      sum(rate(themisdb_writes_total[30d]))
  
  - name: "replication-freshness"
    description: "Replication Lag < 2s"
    target: 99.5  # 99.5% der Zeit unter 2s Lag
    window: 7d
    sli_query: |
      (
        sum(themisdb_replication_lag_milliseconds < 2000)
        /
        count(themisdb_replication_lag_milliseconds)
      )
```

### 38.5.3 Error Budget Management {#chapter_38_5_3_error-budget-management}

Error Budgets quantifizieren erlaubte Fehlerquoten und steuern Release-Velocity. Bei Budget-Verbrauch priorisieren wir Stability über Features. Error Budget Policies definieren Konsequenzen bei Budget-Überschreitung[^google_sre_workbook].

**Error Budget Berechnung:**

```python
# Python: Error Budget Calculator
from datetime import datetime, timedelta

def calculate_error_budget(slo_target, window_days, current_sli):
    """
    Berechnet verbleibendes Error Budget
    
    Args:
        slo_target: SLO-Ziel (z.B. 0.999 für 99.9%)
        window_days: SLO-Window in Tagen (z.B. 30)
        current_sli: Aktueller SLI-Wert (z.B. 0.9985)
    
    Returns:
        dict mit Budget-Status
    """
    # Error Budget = 1 - SLO
    error_budget = 1 - slo_target
    
    # Tatsächlicher Error-Anteil
    actual_errors = 1 - current_sli
    
    # Verbrauchtes Budget (Prozent)
    budget_consumed_percent = (actual_errors / error_budget) * 100
    
    # Verbleibendes Budget (Prozent)
    budget_remaining_percent = 100 - budget_consumed_percent
    
    # Downtime-Budget in Minuten
    total_minutes = window_days * 24 * 60
    budget_minutes = total_minutes * error_budget
    consumed_minutes = total_minutes * actual_errors
    remaining_minutes = budget_minutes - consumed_minutes
    
    return {
        'error_budget_percent': error_budget * 100,
        'budget_consumed_percent': budget_consumed_percent,
        'budget_remaining_percent': budget_remaining_percent,
        'budget_total_minutes': budget_minutes,
        'budget_consumed_minutes': consumed_minutes,
        'budget_remaining_minutes': remaining_minutes,
        'status': 'healthy' if budget_remaining_percent > 0 else 'exhausted'
    }

# Beispiel-Nutzung
result = calculate_error_budget(
    slo_target=0.999,      # 99.9% SLO
    window_days=30,        # 30-Tage-Window
    current_sli=0.9985     # Aktueller SLI: 99.85%
)

print(f"Error Budget: {result['error_budget_percent']:.2f}%")
print(f"Consumed: {result['budget_consumed_percent']:.1f}%")
print(f"Remaining: {result['budget_remaining_minutes']:.1f} minutes")
# Output:
# Error Budget: 0.10%
# Consumed: 50.0%
# Remaining: 21.6 minutes
```

**Error Budget Policy - Beispiel:**

| Budget-Status | Verbleibend | Actions | Release-Freeze? |
|---------------|-------------|---------|-----------------|
| **Healthy** | > 50% | Normale Velocity, experimentelle Features erlaubt | Nein |
| **Warning** | 25-50% | Reduzierte Velocity, Fokus auf Reliability-Fixes | Nein, aber vorsichtig |
| **Critical** | 10-25% | Nur kritische Bugfixes, Review-Standards erhöhen | Ja, für neue Features |
| **Exhausted** | < 10% oder 0% | Vollständiger Release-Freeze, Postmortem erforderlich | Ja, alle Releases |

**Automated Budget Enforcement:**

```yaml
# ci-pipeline.yaml - Budget-Check vor Release
steps:
  - name: "Check Error Budget"
    script: |
      SLI=$(curl -s "http://prometheus:9090/api/v1/query?query=themisdb_availability_sli" | jq '.data.result[0].value[1]')
      SLO=0.999
      BUDGET_REMAINING=$((1 - (1 - $SLI) / (1 - $SLO)))
      
      if [ $(echo "$BUDGET_REMAINING < 0.1" | bc) -eq 1 ]; then
        echo "ERROR: Error Budget exhausted (${BUDGET_REMAINING}% remaining)"
        echo "Release blocked per SLO policy"
        exit 1
      fi
      
      echo "Error Budget OK (${BUDGET_REMAINING}% remaining)"
```

### 38.5.4 Burn Rate & Multi-Window Alerting {#chapter_38_5_4_burn-rate-alerting}

Burn Rate misst Geschwindigkeit des Error-Budget-Verbrauchs. Multi-Window-Alerting kombiniert kurze (1h) und lange (6h) Fenster für Balance zwischen False-Positives und schnellem Response[^sloth_slo_generator].

**Burn Rate Formel:**

```
Burn_Rate = (Error_Rate / Error_Budget) * Time_Window_Ratio

Beispiel:
SLO = 99.9% (Error Budget = 0.1% über 30 Tage)
Aktuelle Error Rate = 1% (letzte 1 Stunde)
Burn Rate = (0.01 / 0.001) * (1h / 720h) = 10× zu schnell

→ Bei diesem Burn Rate ist Budget in 3 Tagen erschöpft
```

**Multi-Window Alerting Rules:**

```yaml
# prometheus-alerts.yaml - Multi-Window Burn Rate Alerts
groups:
  - name: themisdb_slo_alerts
    rules:
      # Tier 1: Fast Burn (Budget erschöpft in 2 Tagen)
      - alert: ThemisDBSLOBurnRateFast
        expr: |
          (
            (1 - themisdb:availability_sli:1h) > (14.4 * (1 - 0.999))
            and
            (1 - themisdb:availability_sli:6h) > (14.4 * (1 - 0.999))
          )
        for: 2m
        labels:
          severity: critical
          tier: "1"
        annotations:
          summary: "SLO Burn Rate Critical (Budget exhausted in 2 days)"
          description: "Error Budget wird mit 14.4× Rate verbraucht"
          runbook: "https://wiki/runbooks/slo-burn-fast"
      
      # Tier 2: Medium Burn (Budget erschöpft in 1 Woche)
      - alert: ThemisDBSLOBurnRateMedium
        expr: |
          (
            (1 - themisdb:availability_sli:6h) > (6 * (1 - 0.999))
            and
            (1 - themisdb:availability_sli:24h) > (6 * (1 - 0.999))
          )
        for: 15m
        labels:
          severity: warning
          tier: "2"
        annotations:
          summary: "SLO Burn Rate Warning (Budget exhausted in 1 week)"
          description: "Error Budget wird mit 6× Rate verbraucht"
      
      # Tier 3: Slow Burn (Budget erschöpft in 2 Wochen)
      - alert: ThemisDBSLOBurnRateSlow
        expr: |
          (
            (1 - themisdb:availability_sli:24h) > (3 * (1 - 0.999))
            and
            (1 - themisdb:availability_sli:72h) > (3 * (1 - 0.999))
          )
        for: 1h
        labels:
          severity: info
          tier: "3"
        annotations:
          summary: "SLO Burn Rate Info (Budget exhausted in 2 weeks)"
          description: "Error Budget wird mit 3× Rate verbraucht"
```

**Burn Rate Coefficients:**

| Alert Tier | Time to Exhaustion | Burn Rate Multiplier | Window (Short/Long) | Severity |
|------------|-------------------|----------------------|---------------------|----------|
| **1** | 2 Tage | 14.4× | 1h / 6h | Critical |
| **2** | 1 Woche | 6× | 6h / 24h | Warning |
| **3** | 2 Wochen | 3× | 24h / 72h | Info |

*Formel: `Multiplier = Window_Days / Target_Days`*

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

## 38.6 Alerting Design {#chapter_38_6_alerting-design}

Effektives Alerting minimiert MTTR (Mean Time To Resolution) und reduziert Alert Fatigue durch präzise Trigger-Bedingungen und intelligentes Routing. Wir implementieren symptom-basierte Alerts nach dem Google SRE-Playbook[^google_sre_book] mit [Prometheus Alertmanager](../appendix_h_glossary.md#alertmanager) als zentraler Routing-Engine. Dieser Abschnitt vermittelt Alert-Rule-Design, Deduplizierungs-Strategien und Runbook-Integration für Operations-Teams.

### 38.6.1 Alerting-Philosophie: Symptoms vs. Causes {#chapter_38_6_1_symptoms-vs-causes}

Symptom-basierte Alerts fokussieren auf User-Impact (hohe Latenz, Error-Rate) statt auf Ursachen (CPU-Last, Disk-Nutzung). Ursache-Alerts sind nachrangig und dienen der Root-Cause-Analysis. Diese Hierarchie reduziert Alert-Volumen und priorisiert kritische Incidents.

**Alert-Hierarchie:**

1. **Tier 1 (Critical):** User-Impact-Alerts (SLO-Burn, Error-Rate > 5%)
2. **Tier 2 (Warning):** Leading-Indicator-Alerts (Disk 85% voll, Queue-Depth steigend)
3. **Tier 3 (Info):** Diagnostic-Alerts (Cache-Hit-Rate niedrig, Slow-Queries erkannt)

**Anti-Pattern vs. Best Practice:**

| Anti-Pattern (Cause-based) | Best Practice (Symptom-based) | Warum besser? |
|----------------------------|-------------------------------|---------------|
| Alert: CPU > 80% | Alert: p99 Latenz > 500ms | CPU-Last korreliert nicht immer mit User-Impact |
| Alert: Disk 95% voll | Alert: Error-Rate steigt (aus Disk-Full-Fehlern) | Disk-Full wird nur zum Problem wenn Writes fehlschlagen |
| Alert: Memory > 90% | Alert: OOM-Killer Events | Memory-Nutzung ist normal, nur OOM ist kritisch |

**Symptom-based Alert Beispiel:**

```yaml
# prometheus-alerts.yaml - Symptom-basierte Alerts
groups:
  - name: themisdb_user_impact_alerts
    rules:
      # Symptom: Hohe Error-Rate (User-Impact)
      - alert: ThemisDBHighErrorRate
        expr: |
          (
            sum(rate(themisdb_requests_errors_total[5m]))
            /
            (sum(rate(themisdb_requests_success_total[5m])) + sum(rate(themisdb_requests_errors_total[5m])))
          ) > 0.01
        for: 5m
        labels:
          severity: critical
          component: api
        annotations:
          summary: "High Error Rate ({{ $value | humanizePercentage }})"
          description: "Error rate exceeded 1% for 5 minutes"
          impact: "Users experiencing failed requests"
          runbook_url: "https://wiki/runbooks/high-error-rate"
      
      # Cause: Hohe CPU-Last (nachrangig, Info-Level)
      - alert: ThemisDBHighCPUUsage
        expr: |
          100 - (avg(rate(node_cpu_seconds_total{mode="idle"}[5m])) * 100) > 80
        for: 15m
        labels:
          severity: info
          component: system
        annotations:
          summary: "High CPU Usage ({{ $value }}%)"
          description: "CPU usage above 80% for 15 minutes"
          impact: "Potential latency increase (monitor user-facing metrics)"
```

### 38.6.2 Multi-Window, Multi-Burn-Rate Alerting {#chapter_38_6_2_multi-window-alerting}

Multi-Window-Alerts kombinieren kurze und lange Zeitfenster zur Reduktion von False-Positives. Die Methodik stammt aus dem Google SRE Workbook[^google_sre_workbook] und wird in Abschnitt 38.5.4 für SLO-Burn-Rate-Alerts detailliert.

**Warum zwei Fenster?**

- **Kurzes Fenster (1h):** Erkennt schnelle Incidents (hohe Sensitivität)
- **Langes Fenster (6h):** Filtert Transient-Spikes (niedrige False-Positive-Rate)
- **Logik:** `short_window_bad AND long_window_bad` → Alert auslösen

**AlertManager-Integration:**

```yaml
# alertmanager.yaml - Konfiguration
global:
  resolve_timeout: 5m
  slack_api_url: 'https://hooks.slack.com/services/XXX/YYY/ZZZ'

route:
  # Root-Route: Default-Receiver
  receiver: 'slack-default'
  group_by: ['alertname', 'cluster', 'service']
  group_wait: 30s       # Warte 30s auf weitere Alerts vor Gruppierung
  group_interval: 5m    # Sende gruppierte Alerts alle 5 Minuten
  repeat_interval: 4h   # Wiederhole unresolved Alerts alle 4 Stunden
  
  # Tier-basiertes Routing
  routes:
    # Tier 1: Critical Alerts → PagerDuty
    - match:
        severity: critical
      receiver: 'pagerduty-critical'
      group_wait: 10s     # Schneller Response für Critical
      repeat_interval: 1h
    
    # Tier 2: Warning Alerts → Slack + Ticket
    - match:
        severity: warning
      receiver: 'slack-warnings'
      continue: true      # Weitergabe an weitere Routes
    
    # Tier 3: Info Alerts → Slack nur während Geschäftszeiten
    - match:
        severity: info
      receiver: 'slack-info'
      active_time_intervals:
        - business_hours

# Time-Intervals für Quiet Hours
time_intervals:
  - name: business_hours
    time_intervals:
      - weekdays: ['monday:friday']
        times:
          - start_time: '09:00'
            end_time: '18:00'

# Receiver-Definitionen
receivers:
  - name: 'pagerduty-critical'
    pagerduty_configs:
      - service_key: '<pagerduty_integration_key>'
        description: '{{ .GroupLabels.alertname }}: {{ .Annotations.summary }}'
        details:
          runbook: '{{ .Annotations.runbook_url }}'
          impact: '{{ .Annotations.impact }}'
  
  - name: 'slack-warnings'
    slack_configs:
      - channel: '#themisdb-alerts'
        title: '⚠️ {{ .GroupLabels.alertname }}'
        text: '{{ range .Alerts }}{{ .Annotations.summary }}\n{{ end }}'
  
  - name: 'slack-info'
    slack_configs:
      - channel: '#themisdb-monitoring'
        title: 'ℹ️ {{ .GroupLabels.alertname }}'
```

### 38.6.3 Alert Deduplication & Grouping {#chapter_38_6_3_deduplication-grouping}

Alert-Deduplication verhindert Spam durch identische Alerts von mehreren Instanzen. Grouping fasst verwandte Alerts zusammen (z.B. alle Down-Nodes in einem Cluster).

**Grouping-Strategien:**

```yaml
# Strategie 1: Group by Service + Severity
group_by: ['service', 'severity']

# Resultat: 1 Alert-Gruppe für "api + critical", 1 für "storage + warning"

# Strategie 2: Group by Cluster + Alert Name
group_by: ['cluster', 'alertname']

# Resultat: Alle "HighLatency"-Alerts im "prod-us-west"-Cluster in einer Gruppe

# Strategie 3: No Grouping (für Critical-Alerts)
group_by: []

# Resultat: Jeder Alert einzeln (für sofortige Visibility)
```

**Inhibition Rules (Suppress Dependent Alerts):**

```yaml
# alertmanager.yaml - Inhibition Rules
inhibit_rules:
  # Regel 1: Node Down inhibiert alle anderen Alerts von diesem Node
  - source_match:
      alertname: 'NodeDown'
    target_match_re:
      alertname: '.*'
    equal: ['instance']
  
  # Regel 2: Cluster Outage inhibiert Individual-Node-Alerts
  - source_match:
      alertname: 'ClusterOutage'
      severity: 'critical'
    target_match:
      severity: 'warning'
    equal: ['cluster']
  
  # Regel 3: High Error Rate inhibiert Latency Alerts (Symptom-Hierarchie)
  - source_match:
      alertname: 'HighErrorRate'
    target_match:
      alertname: 'HighLatency'
    equal: ['service']
```

### 38.6.4 Runbook-Integration {#chapter_38_6_4_runbook-integration}

Jeder Alert verlinkt ein Runbook mit standardisierten Troubleshooting-Steps. Runbooks reduzieren MTTR durch konsistente Incident-Response-Workflows (siehe Kapitel 27 für detaillierte Troubleshooting-Methoden).

**Runbook-Template:**

```markdown
# Runbook: HighErrorRate

## Symptom
Error rate exceeds 1% for ThemisDB API requests.

## Impact
- Users experiencing failed requests
- SLO at risk (99.9% availability target)
- Error Budget consumption accelerated

## Investigation Steps
1. **Check Error Types:**
   ```promql
   sum(rate(themisdb_requests_errors_total[5m])) by (error_type)
   ```
   → Identify dominant error class (timeout, validation, internal)

2. **Check Recent Deployments:**
   ```bash
   kubectl rollout history deployment/themisdb
   ```
   → Rollback if error spike correlates with deploy

3. **Check Downstream Dependencies:**
   ```promql
   up{job="postgres"} == 0
   ```
   → Verify database connectivity

4. **Check Resource Saturation:**
   ```promql
   themisdb_thread_pool_active / themisdb_thread_pool_max > 0.9
   ```
   → Scale if thread pool saturated

## Mitigation
- **Immediate:** Rollback to last-known-good version
- **Short-term:** Increase thread pool size, enable rate limiting
- **Long-term:** Root-cause analysis, add retry logic

## Escalation
- Oncall Lead: @sre-oncall
- Engineering Lead: @themisdb-team
- Escalation Time: 30 minutes if no resolution

## Postmortem
Required: Yes (Critical incident)
Template: https://wiki/templates/postmortem
```

**Alert Annotation mit Runbook-Link:**

```yaml
# prometheus-alerts.yaml - Runbook-Links in Annotations
annotations:
  summary: "High Error Rate ({{ $value | humanizePercentage }})"
  description: |
    Error rate exceeded 1% for 5 minutes.
    Current value: {{ $value | humanizePercentage }}
    
    **Investigation Steps:**
    1. Check error types dashboard: https://grafana/d/errors
    2. Review recent deployments: kubectl rollout history
    3. Follow runbook: {{ .Annotations.runbook_url }}
  
  runbook_url: "https://wiki/runbooks/high-error-rate"
  dashboard_url: "https://grafana/d/themisdb-overview"
  impact: "Users experiencing failed requests, SLO at risk"
```

### 38.6.5 Alert Fatigue Prevention {#chapter_38_6_5_alert-fatigue}

Alert Fatigue entsteht durch zu viele, zu ungenaue oder zu häufige Alerts. Wir definieren Strategien zur Reduktion von Alert-Volumen und Verbesserung der Signal-to-Noise-Ratio.

**Prevention-Strategien:**

| Strategie | Beschreibung | Impact |
|-----------|--------------|--------|
| **Higher Thresholds** | Alert nur bei echtem User-Impact (1% Error statt 0.1%) | -50% Alert-Volumen |
| **Longer Durations** | `for: 10m` statt `for: 1m` (filtert Transients) | -30% False-Positives |
| **Inhibition Rules** | Suppress Dependent Alerts (Node-Down inhibiert alle Node-Alerts) | -40% Duplicate-Alerts |
| **Quiet Hours** | Keine Info-Alerts nachts/Wochenende | -20% Out-of-Hours-Alerts |
| **Automated Remediation** | Auto-Scaling, Auto-Restart statt Alert | -25% Manual-Interventions |

**Alert Quality Metrics:**

```promql
# PromQL: Alert-to-Incident-Ratio (Ziel: > 0.5)
sum(increase(incidents_total[30d]))
/
sum(increase(alerts_fired_total[30d]))

# Interpretation:
# > 0.5: Gut (meiste Alerts führen zu echten Incidents)
# 0.2-0.5: Akzeptabel (einige False-Positives)
# < 0.2: Schlecht (viel Alert-Rauschen, Review-Bedarf)
```

**Beispiel-Alerts:**

- `latency_p99_gt_200ms` (for 15m & 6h)
- `error_rate_gt_1pct`
- `replication_lag_gt_2s`
- `disk_util_gt_85pct`
- `cache_evictions_spike`

**Ownership & Escalation:**

```yaml
# prometheus-alerts.yaml - Alert-Ownership-Labels
labels:
  team: "sre"                      # Responsible Team
  oncall_rotation: "themisdb-oncall"  # PagerDuty-Rotation
  priority: "P1"                   # Incident-Priority
  escalation_time: "30m"           # Time before escalation
```

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

**Kernergebnisse dieses Kapitels:**
- RED/USE-Methodologien ermöglichen systematische Metrik-Erfassung (Abschnitt 38.1)
- Strukturierte Logs mit Trace-Korrelation vereinfachen Root-Cause-Analysis (Abschnitt 38.2)
- OpenTelemetry-basiertes Tracing visualisiert End-to-End-Latenz (Abschnitt 38.3)
- Grafana-Dashboards transformieren Metriken in handlungsrelevante Insights (Abschnitt 38.4)
- SLI/SLO-Framework und Error Budgets steuern Release-Velocity (Abschnitt 38.5)
- Symptom-basiertes Alerting reduziert Alert Fatigue und MTTR (Abschnitt 38.6)
- Sampling-Strategien reduzieren Overhead bei hohem Durchsatz (Abschnitte 38.2.3, 38.3.2)
- Cardinality-Management verhindert Metrik-Explosion in Prometheus (Abschnitt 38.1.3)

**Weiterführende Themen:**
- Kapitel 19: Security Monitoring und Audit-Logging
- Kapitel 27: Troubleshooting-Workflows mit Observability-Daten
- Kapitel 39: Performance-Tuning basierend auf Metriken und Traces
- Appendix E: Incident Runbooks für häufige ThemisDB-Störungen

---

## Referenzen

[^red_method]: Wilkie, T. (2018). "The RED Method: Key Metrics for Microservices Architecture". Grafana Labs. https://grafana.com/blog/2018/08/02/the-red-method-how-to-instrument-your-services/

[^use_method]: Gregg, B. (2012). "The USE Method". http://www.brendangregg.com/usemethod.html

[^brendan_gregg_latency]: Gregg, B. (2020). "Systems Performance: Enterprise and the Cloud". Addison-Wesley Professional. Chapter 2: Methodology.

[^rocksdb_metrics]: Facebook RocksDB Team. (2023). "RocksDB Statistics". https://github.com/facebook/rocksdb/wiki/Statistics

[^prometheus_practices]: Prometheus Documentation. (2023). "Best Practices - Metric and Label Naming". https://prometheus.io/docs/practices/naming/

[^internal_benchmarks]: ThemisDB Performance Team. (2026). "Internal Benchmarks v1.4.0". Methodologie dokumentiert in Kapitel 39, Abschnitt 39.2.

[^sridharan_observability]: Sridharan, C. (2018). "Distributed Systems Observability: A Guide to Building Robust Systems". O'Reilly Media.

[^otel_semantic_conventions]: OpenTelemetry Specification. (2023). "Semantic Conventions for Logs". https://opentelemetry.io/docs/specs/semconv/general/logs/

[^loki_best_practices]: Grafana Labs. (2023). "Loki Best Practices". https://grafana.com/docs/loki/latest/best-practices/

[^jaeger_sampling]: Shkuro, Y. (2019). "Mastering Distributed Tracing: Analyzing Performance in Microservices and Complex Systems". Packt Publishing. Chapter 6: Sampling.

[^tempo_docs]: Grafana Labs. (2023). "Grafana Tempo Documentation - Tail-Based Sampling". https://grafana.com/docs/tempo/latest/configuration/

[^otel_specification]: OpenTelemetry Project. (2023). "OpenTelemetry Specification v1.26.0". https://opentelemetry.io/docs/specs/otel/

[^w3c_trace_context]: W3C Distributed Tracing Working Group. (2020). "W3C Trace Context Specification". https://www.w3.org/TR/trace-context/

[^google_sre_book]: Beyer, B., Jones, C., Petoff, J., & Murphy, N. R. (2016). "Site Reliability Engineering: How Google Runs Production Systems". O'Reilly Media. Chapter 4: Service Level Objectives.

[^google_sre_workbook]: Beyer, B., et al. (2018). "The Site Reliability Workbook: Practical Ways to Implement SRE". O'Reilly Media. Chapter 2: Implementing SLOs.

[^sloth_slo_generator]: Sloth Team. (2023). "Sloth - Easy and Simple Prometheus SLO Generator". https://github.com/slok/sloth
