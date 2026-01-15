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

## 38.2 Logging

### Strukturierte Logs

```json
{
  "ts": "2026-01-01T10:00:00Z",
  "level": "INFO",
  "service": "themisdb",
  "component": "aql",
  "event": "query_executed",
  "trace_id": "abc123",
  "span_id": "def456",
  "duration_ms": 32,
  "collection": "users",
  "operation": "READ",
  "status": "ok"
}
```

**Best Practices:**
- JSON Lines, ein Event pro Zeile
- Pflichtfelder: ts, level, service, component, trace_id
- Keine PII in Logs; IDs statt Freitext
- Rotate & Compress (zstd)

### Log-Pipeline

- Agent: Filebeat/FluentBit (TLS, backpressure)
- Parse: grok/json; enrich (host, cluster, env)
- Store: Loki/Elastic/OpenSearch
- Retention: 7-30 Tage (Hot), 90+ Tage (Cold/Glacier)

---

## 38.3 Tracing

### OpenTelemetry Setup (Go API Layer)

```go
// tracing.go
import "go.opentelemetry.io/otel"
import "go.opentelemetry.io/otel/exporters/otlp/otlptrace/otlptracehttp"
import "go.opentelemetry.io/otel/sdk/trace"

func InitTracer(endpoint string) func() {
    exporter, _ := otlptracehttp.New(context.Background(), otlptracehttp.WithEndpoint(endpoint))
    tp := trace.NewTracerProvider(trace.WithBatcher(exporter))
    otel.SetTracerProvider(tp)
    return func() { _ = tp.Shutdown(context.Background()) }
}
```

### AQL Trace-Injektion

- Propagiere `traceparent` Header vom API Gateway ins AQL Layer
- Schreibe `trace_id` in Query-Context → Log-Korrelation
- Sample Rate: 1-10% in Produktion; 100% in Staging

---

## 38.4 Dashboards (Grafana)

### Latenz & Throughput

Panels:
- Query p50/p95/p99 (stacked per operation)
- QPS split by read/write/graph/vector
- Error rate (% per op)

### Replikation

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
