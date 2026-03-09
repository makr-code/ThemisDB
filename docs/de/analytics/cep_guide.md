# Complex Event Processing (CEP) Guide

**Version:** v1.7.0
**Status:** 🟢 Production-Ready
**Last Updated:** 2026-03-09

---

## Übersicht

ThemisDB's CEP Engine (`cep_engine.cpp`) ist eine vollständige Produktionsimplementierung für
Echtzeit-Mustererkennung in Ereignisströmen. Die Engine verwendet einen NFA-basierten
(Nichtdeterministischer endlicher Automat) Pattern-Matcher und eine EPL (Event Processing Language)
zur deklarativen Regelspezifikation.

**Header:** `include/analytics/cep_engine.h`
**Implementierung:** `src/analytics/cep_engine.cpp`

---

## Kernfunktionen

| Feature | Beschreibung |
| --- | --- |
| NFA Pattern Matching | Zustandsbasierte Mustererkennung über Ereignisfolgen |
| EPL Parser | `CREATE RULE … AS SELECT … FROM … WINDOW … ACTION` |
| Window Management | Tumbling, Sliding, Session, Hopping Windows |
| Aggregationen | COUNT, SUM, AVG, MIN, MAX, FIRST, LAST, STDDEV, PERCENTILE, TOPN |
| Alert Dispatch | webhook, Slack, Kafka, E-Mail, db_write, log |
| CDC Integration | Change-Data-Capture-Ereignisse als CEP-Input |
| Stateful Checkpointing | Persistenz partieller Musterzustände über Neustarts |
| Backpressure | Konfigurierbare Queue-Tiefe und Drop-Policy |
| Prometheus Metriken | Engine-Metriken für Monitoring |

---

## EPL-Regelformat

### Grundstruktur

```sql
CREATE RULE <rule_name> AS
  SELECT <aggregations>
  FROM <source>
  [WHERE <conditions>]
  [GROUP BY <fields>]
  WINDOW <window_type> <duration>
  [HAVING <conditions>]
  [PATTERN <pattern> WITHIN <duration>]
  ACTION <action>(<params>)
```

### Window-Typen

| Typ | Syntax | Beschreibung |
| --- | --- | --- |
| Tumbling | `WINDOW TUMBLING 60s` | Nicht-überlappende Zeitfenster |
| Sliding | `WINDOW SLIDING 5m STEP 30s` | Überlappende Fenster mit Schrittuweite |
| Session | `WINDOW SESSION TIMEOUT 30s` | Aktivitätsbasierte Sitzungsfenster |
| Hopping | `WINDOW HOPPING 10m ADVANCE 1m` | Gleichmäßig vorwärts rückende Fenster |

Zeiteinheiten: `ms`, `s`, `minutes`, `hours`, `days`

### Action-Typen

```sql
ACTION alert("alert-name")
ACTION webhook("https://hooks.example.com/notify")
ACTION slack("#channel", "message")
ACTION kafka("topic-name")
ACTION db_write("collection", "field")
ACTION log("INFO")
```

---

## Verwendungsbeispiele

### Einfache Fehler-Rate-Regel

```cpp
#include "analytics/cep_engine.h"
using namespace themisdb::analytics;

CEPEngine engine;

std::string epl = R"(
  CREATE RULE high_error_rate AS
  SELECT COUNT(*) AS cnt
  FROM app_events
  WINDOW TUMBLING 60s
  WHERE level = 'ERROR'
  HAVING cnt > 100
  ACTION alert("high-error-rate")
)";

engine.loadRule(epl);

// Ereignisse zuführen
Event evt;
evt.source = "app_events";
evt.fields["level"] = "ERROR";
evt.fields["message"] = "Connection timeout";
evt.timestamp_ms = getCurrentTimeMs();
engine.ingest(evt);
```

### Aggregation mit GROUP BY

```cpp
std::string epl = R"(
  CREATE RULE revenue_spike AS
  SELECT region, SUM(amount) AS total, COUNT(*) AS txn_count
  FROM transactions
  GROUP BY region
  WINDOW TUMBLING 5m
  HAVING total > 10000
  ACTION webhook("https://alerts.example.com/revenue")
)";

engine.loadRule(epl);
```

### Mustererkennung (PATTERN)

```cpp
std::string epl = R"(
  CREATE RULE login_brute_force AS
  SELECT user_id, COUNT(*) AS attempts
  FROM auth_events
  WHERE status = 'FAILED'
  PATTERN (FAILED -> FAILED -> FAILED) WITHIN 60s
  HAVING attempts >= 3
  ACTION alert("brute-force-detected")
)";

engine.loadRule(epl);
```

### TOPN-Aggregation

```cpp
std::string epl = R"(
  CREATE RULE top_queries AS
  SELECT TOPN(query_type, 5) AS top_queries, COUNT(*) AS total
  FROM query_log
  WINDOW TUMBLING 10m
  ACTION log("INFO")
)";
```

---

## Stateful Checkpointing

Die CEP Engine unterstützt die Persistenz partieller Musterzustände (partial matches) über
Neustarts hinweg:

```cpp
// Zustand serialisieren
std::string state = engine.serializeMatcherStates();

// Zustand wiederherstellen
engine.restoreMatcherStates(state);
```

Dies garantiert, dass laufende Mustererkennung nach einem Neustart nahtlos fortgesetzt wird.

---

## Backpressure-Konfiguration

```cpp
CEPConfig cfg;
cfg.max_queue_depth = 10000;       // Max. Ereignisse in der Queue
cfg.backpressure_threshold = 0.8;  // 80% → Backpressure-Signal
cfg.drop_policy = DropPolicy::OldestFirst;
cfg.window_thread_pool_size = 4;

CEPEngine engine(cfg);
```

Prometheus-Metriken werden automatisch unter `themisdb_cep_*` exponiert.

---

## Thread-Sicherheit

- `CEPEngine::ingest()` — **thread-safe**; Events können von mehreren Threads gesendet werden
- `CEPEngine::loadRule()` — **nicht** thread-safe während des Ingests; Regeln vor Start laden
- Window-Aggregationen laufen in dedizierten Hintergrundthreads (Pool-Größe konfigurierbar)

---

## Bekannte Einschränkungen

- Windows-Build: Stub-Implementierung (kein NFA-Matching auf Windows)
- `PATTERN`-Syntax unterstützt derzeit sequentielle Muster (`A -> B -> C`); keine Verzweigungen
- Alert-Dispatch-Handler sind synchron; für Hochlast-Szenarien asynchrone Handler verwenden

---

## Verwandte Dokumentation

- [Analytics Docs Hub](./README.md)
- [Streaming Windows Guide](./streaming_window_guide.md)
- [Anomaly Detection Guide](./anomaly_detection_guide.md)
- [API Reference](../../../include/analytics/cep_engine.h)
- [Implementierung](../../../src/analytics/cep_engine.cpp)
- [Roadmap](../../../src/analytics/ROADMAP.md)

---

**Last Updated:** 2026-03-09
**Version:** v1.7.0
**Status:** 🟢 Production-Ready
