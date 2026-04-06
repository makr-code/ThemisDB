# ThemisDB Complex Event Processing (CEP) - Streaming Analytics

**Stand:** 6. April 2026  
**Version:** v1.3.0  
**Kategorie:** 🔍 Observability

---

## 📑 Inhaltsverzeichnis

- [Übersicht](#übersicht)
- [Architecture](#architecture)
- [Implementation](#implementation)

---

## Übersicht

ThemisDB CEP ist eine vollständige Streaming Analytics Engine für Echtzeit-Ereignisverarbeitung. Sie ermöglicht:

- **Pattern Matching**: Erkennung komplexer Ereignismuster über Zeit
- **Window Management**: Zeit- und anzahlbasierte Aggregationen
- **EPL (Event Processing Language)**: SQL-ähnliche Regelsprache
- **Change Data Capture (CDC)**: Automatische Events bei Datenbankänderungen
- **Backpressure Integration**: Load-aware Event Processing

## Architektur

```
┌─────────────────────────────────────────────────────────────────┐
│                       CEPEngine (Singleton)                      │
│  - Verwaltet alle Streams und Regeln                            │
│  - Koordiniert Pattern Matching                                 │
│  - Prometheus Metrics Export                                    │
└─────────────────────────────────────────────────────────────────┘
                               │
          ┌────────────────────┼────────────────────┐
          ▼                    ▼                    ▼
┌──────────────────┐  ┌──────────────────┐  ┌──────────────────┐
│   EventStream    │  │  PatternMatcher  │  │  WindowManager   │
│  - Ring Buffer   │  │  - NFA-basiert   │  │  - TUMBLING      │
│  - Partitioned   │  │  - SEQUENCE      │  │  - SLIDING       │
│  - Backpressure  │  │  - AND/OR/NOT    │  │  - SESSION       │
└──────────────────┘  └──────────────────┘  └──────────────────┘
                               │
                               ▼
┌─────────────────────────────────────────────────────────────────┐
│                         RuleEngine                               │
│  - EPL Parser                                                   │
│  - Hot Reload von Regeln                                        │
│  - Action Triggers (Alert, Webhook, DB Write)                   │
└─────────────────────────────────────────────────────────────────┘
```

## Event Types

### Database Events (CDC)

| Event | Beschreibung |
|-------|--------------|
| `DOCUMENT_INSERT` | Neues Dokument eingefügt |
| `DOCUMENT_UPDATE` | Dokument aktualisiert |
| `DOCUMENT_DELETE` | Dokument gelöscht |
| `COLLECTION_CREATE` | Neue Collection erstellt |
| `COLLECTION_DROP` | Collection gelöscht |
| `INDEX_CREATE` | Index erstellt |
| `INDEX_DROP` | Index gelöscht |

### Graph Events

| Event | Beschreibung |
|-------|--------------|
| `VERTEX_CREATE` | Neuer Vertex erstellt |
| `VERTEX_UPDATE` | Vertex aktualisiert |
| `VERTEX_DELETE` | Vertex gelöscht |
| `EDGE_CREATE` | Neue Kante erstellt |
| `EDGE_DELETE` | Kante gelöscht |
| `GRAPH_TRAVERSAL` | Graph-Traversierung ausgeführt |

### Security Events

| Event | Beschreibung |
|-------|--------------|
| `AUTH_SUCCESS` | Erfolgreiche Authentifizierung |
| `AUTH_FAILURE` | Fehlgeschlagene Authentifizierung |
| `PERMISSION_DENIED` | Zugriff verweigert |
| `TOKEN_REFRESH` | Token erneuert |

### System Events

| Event | Beschreibung |
|-------|--------------|
| `SHARD_JOIN` | Neuer Shard beigetreten |
| `SHARD_LEAVE` | Shard verlassen |
| `REBALANCE_START` | Rebalancing gestartet |
| `REBALANCE_COMPLETE` | Rebalancing abgeschlossen |
| `MIGRATION_START` | Migration gestartet |
| `MIGRATION_COMPLETE` | Migration abgeschlossen |

## Window Types

### TUMBLING Window

Feste, nicht-überlappende Zeitfenster.

```sql
WINDOW TUMBLING(5 MINUTES)
```

```
Time:    |----1----|----2----|----3----|
Events:  [A B C]   [D E]     [F G H I]
Output:  {3}       {2}       {4}
```

### SLIDING Window

Überlappende Fenster mit konfigurierbarem Slide.

```sql
WINDOW SLIDING(5 MINUTES, 1 MINUTE)
```

```
Time:    |----1----|----2----|----3----|
Window 1:[A B C D]
Window 2:    [B C D E]
Window 3:        [C D E F]
```

### SESSION Window

Gap-basierte Fenster für User-Sessions.

```sql
WINDOW SESSION(30 MINUTES)
```

```
Time:    |--A--B--C----------D--E--|
Session: |----Session 1----|  |Ses 2|
         (30min gap closes session)
```

### COUNT Window

Anzahlbasierte Fenster.

```sql
WINDOW COUNT(100 EVENTS)
```

## Pattern Matching

### SEQUENCE Pattern

Events müssen in Reihenfolge auftreten.

```sql
PATTERN SEQUENCE(LoginEvent, ViewEvent, PurchaseEvent)
WITHIN 1 HOUR
```

### CONJUNCTION (AND) Pattern

Events müssen innerhalb einer Toleranz auftreten.

```sql
PATTERN AND(ClickEvent, ImpressionEvent)
TOLERANCE 1 SECOND
```

### DISJUNCTION (OR) Pattern

Eines der Events muss auftreten.

```sql
PATTERN OR(ErrorEvent, WarningEvent)
```

### NEGATION (NOT) Pattern

Event A ohne Event B innerhalb eines Zeitfensters.

```sql
PATTERN NOT(PaymentEvent) AFTER OrderEvent
WITHIN 5 MINUTES
```

### REPETITION Pattern

Wiederholte Events.

```sql
PATTERN REPEAT(FailedLoginEvent, 3, 10)
WITHIN 1 MINUTE
```

## EPL (Event Processing Language)

### Syntax

```sql
CREATE RULE <rule_name> AS
SELECT <projections>
FROM <streams>
[JOIN <stream> ON <condition>]
[WHERE <filter>]
[PATTERN <pattern_expression>]
[WINDOW <window_type>(<params>)]
[GROUP BY <fields>]
[HAVING <condition>]
ACTION <action_type>(<params>);
```

### Beispiele

#### 1. Fraud Detection

```sql
CREATE RULE fraud_detection AS
SELECT userId, SUM(amount) as total, COUNT(*) as tx_count
FROM PaymentEvents
WHERE amount > 100
WINDOW TUMBLING(1 HOUR)
GROUP BY userId
HAVING SUM(amount) > 10000 OR COUNT(*) > 20
ACTION webhook('https://fraud.api/alert', '{"user": "${userId}", "total": ${total}}');
```

#### 2. Brute Force Detection

```sql
CREATE RULE brute_force_detection AS
SELECT userId, COUNT(*) as attempts
FROM AuthEvents
WHERE success = false
WINDOW TUMBLING(5 MINUTES)
GROUP BY userId
HAVING COUNT(*) >= 5
ACTION alert('security', 'critical', 'Brute force attack detected for user ${userId}');
```

#### 3. Session Timeout

```sql
CREATE RULE session_timeout AS
SELECT sessionId, userId, FIRST(timestamp) as session_start
FROM UserActivity
WINDOW SESSION(30 MINUTES)
GROUP BY sessionId
ACTION db_write('expired_sessions', '{"sessionId": "${sessionId}", "duration": ${window_duration}}');
```

#### 4. Real-Time Aggregation

```sql
CREATE RULE hourly_metrics AS
SELECT 
    collection,
    COUNT(*) as operations,
    AVG(latency_ms) as avg_latency,
    PERCENTILE(latency_ms, 99) as p99_latency
FROM QueryEvents
WINDOW TUMBLING(1 HOUR)
GROUP BY collection
ACTION db_write('metrics_hourly');
```

#### 5. Komplexes Pattern

```sql
CREATE RULE suspicious_behavior AS
PATTERN SEQUENCE(
    LoginEvent[location != user.home_location],
    ViewEvent[page = 'account_settings'],
    ChangePasswordEvent
)
WITHIN 10 MINUTES
GROUP BY userId
ACTION alert('security', 'warning', 'Suspicious account activity for ${userId}');
```

## Aggregationen

| Funktion | Beschreibung | Beispiel |
|----------|--------------|----------|
| `COUNT(*)` | Anzahl Events | `COUNT(*) as total` |
| `COUNT(DISTINCT field)` | Eindeutige Werte | `COUNT(DISTINCT userId)` |
| `SUM(field)` | Summe | `SUM(amount)` |
| `AVG(field)` | Durchschnitt | `AVG(latency_ms)` |
| `MIN(field)` | Minimum | `MIN(price)` |
| `MAX(field)` | Maximum | `MAX(response_time)` |
| `FIRST(field)` | Erster Wert | `FIRST(timestamp)` |
| `LAST(field)` | Letzter Wert | `LAST(status)` |
| `STDDEV(field)` | Standardabweichung | `STDDEV(values)` |
| `PERCENTILE(field, p)` | Perzentil | `PERCENTILE(latency, 99)` |
| `COLLECT(field)` | Array sammeln | `COLLECT(eventId)` |

## Actions

### Alert

Generiert einen Alert in der internen Queue.

```sql
ACTION alert('category', 'severity', 'message template');
```

### Webhook

Sendet HTTP POST an URL.

```sql
ACTION webhook('https://api.example.com/webhook', '${json_payload}');
```

### Database Write

Schreibt Ergebnis in ThemisDB Collection.

```sql
ACTION db_write('collection_name');
```

### Email

Sendet E-Mail (erfordert SMTP-Konfiguration).

```sql
ACTION email('alerts@company.com', 'Subject', 'Body template');
```

### Slack

Sendet Nachricht an Slack Webhook.

```sql
ACTION slack('https://hooks.slack.com/...', 'Channel alert: ${message}');
```

## Integration mit ThemisDB

### CDC (Change Data Capture)

CDC ist automatisch aktiviert. Jede Änderung an Collections generiert Events:

```cpp
// Automatisch bei document.insert()
Event {
    type: DOCUMENT_INSERT,
    collection_name: "users",
    document_id: "user_123",
    fields: {
        "name": "John Doe",
        "email": "john@example.com"
    }
}
```

### Aktivierung

```yaml
# config/themisdb.yaml
cep:
  enabled: true
  cdc:
    enabled: true
    collections: ["*"]  # Alle Collections
    # collections: ["users", "orders"]  # Spezifische Collections
```

### Graph Events

```cpp
// Bei graph.createVertex()
Event {
    type: VERTEX_CREATE,
    fields: {
        "graph": "social",
        "vertex_id": "v_123",
        "label": "Person"
    }
}
```

### Vector Search Events

```cpp
// Bei vectorSearch()
Event {
    type: QUERY_COMPLETE,
    fields: {
        "query_type": "vector_search",
        "collection": "embeddings",
        "top_k": 10,
        "latency_ms": 5
    }
}
```

## Konfiguration

### YAML-Konfiguration

```yaml
# config/cep.yaml
cep:
  enabled: true
  
  # Threading
  worker_threads: 4
  io_threads: 2
  
  # Checkpointing
  checkpointing:
    enabled: true
    path: /var/lib/themisdb/cep/checkpoints
    interval_ms: 10000
  
  # Backpressure
  backpressure:
    enabled: true
    threshold: 0.9
  
  # Default Stream
  default_stream:
    buffer_size: 1048576
    partitions: 16
    retention_ms: 3600000

# Regeln
rules:
  - rule_id: failed_logins
    rule_name: "Failed Login Detection"
    enabled: true
    streams: ["security_events"]
    filter: "type = 'AUTH_FAILURE'"
    window:
      type: TUMBLING
      size_ms: 300000
    aggregations:
      - name: attempts
        type: COUNT
    group_by: ["userId"]
    having: "attempts >= 5"
    actions:
      - type: ALERT
        target: "security"
        template: "Multiple failed logins for user ${userId}: ${attempts} attempts"
```

## Prometheus Metriken

```
# Events
themisdb_cep_events_received_total{stream="default"}
themisdb_cep_events_processed_total{stream="default"}
themisdb_cep_events_dropped_total{stream="default",reason="backpressure"}

# Pattern Matching
themisdb_cep_pattern_matches_total{pattern="sequence_login"}
themisdb_cep_partial_matches_active{pattern="sequence_login"}

# Rules
themisdb_cep_rules_active
themisdb_cep_rules_triggered_total{rule="fraud_detection"}

# Windows
themisdb_cep_window_events{window_type="tumbling"}
themisdb_cep_window_closed_total{window_type="tumbling"}

# Aggregations
themisdb_cep_aggregation_results{aggregation="count_by_user"}

# Performance
themisdb_cep_processing_latency_seconds{quantile="0.5"}
themisdb_cep_processing_latency_seconds{quantile="0.99"}
themisdb_cep_throughput_events_per_second

# State
themisdb_cep_state_size_bytes
themisdb_cep_checkpoint_latency_seconds
themisdb_cep_checkpoint_size_bytes
```

## API

### C++ API

```cpp
#include <themisdb/analytics/cep_engine.h>

using namespace themisdb::analytics;

// Initialize
CEPEngine::getInstance().initialize(CEPConfig{
    .enabled = true,
    .worker_threads = 4
});

// Create stream
auto stream = CEPEngine::getInstance().createStream(StreamConfig{
    .stream_id = "events",
    .buffer_size = 1024 * 1024
});

// Add rule from EPL
CEPEngine::getInstance().addRuleFromEPL(R"(
    CREATE RULE my_rule AS
    SELECT userId, COUNT(*) as count
    FROM events
    WINDOW TUMBLING(5 MINUTES)
    GROUP BY userId
    HAVING COUNT(*) > 10
    ACTION alert('monitoring', 'High activity for ${userId}');
)");

// Submit event
CEPEngine::getInstance().submitEvent(Event{
    .type = EventType::CUSTOM,
    .event_name = "user_action",
    .fields = {{"userId", "user_123"}, {"action", "click"}}
});

// Get alerts
auto alerts = CEPEngine::getInstance().getAlerts(100, true);
```

### REST API

```bash
# Submit event
POST /api/v1/cep/events
{
    "type": "CUSTOM",
    "event_name": "user_action",
    "fields": {
        "userId": "user_123",
        "action": "click"
    }
}

# Add rule
POST /api/v1/cep/rules
{
    "rule_id": "my_rule",
    "epl": "CREATE RULE my_rule AS SELECT ..."
}

# Get alerts
GET /api/v1/cep/alerts?limit=100&unacknowledged=true

# Acknowledge alert
POST /api/v1/cep/alerts/{alert_id}/acknowledge
```

## Best Practices

### 1. Partitionierung

Verwenden Sie `partition_key` für parallele Verarbeitung:

```cpp
Event event;
event.partition_key = userId;  // Events pro User werden zusammen verarbeitet
```

### 2. Backpressure

Konfigurieren Sie Backpressure für Lastspitzen:

```yaml
backpressure:
  enabled: true
  threshold: 0.8  # Ab 80% Füllstand drosseln
```

### 3. Checkpointing

Für Fault Tolerance:

```yaml
checkpointing:
  enabled: true
  interval_ms: 10000  # Alle 10 Sekunden
```

### 4. Window-Größe

- Kleine Windows = Niedrige Latenz, mehr Overhead
- Große Windows = Höhere Latenz, weniger Overhead

### 5. Pattern Matching

- `WITHIN` immer setzen um Memory-Leaks zu vermeiden
- `GROUP BY` für parallele Pattern-Evaluation

## Troubleshooting

### Events werden gedroppt

1. Prüfen Sie Backpressure: `themisdb_cep_events_dropped_total{reason="backpressure"}`
2. Erhöhen Sie `buffer_size` oder `worker_threads`

### Hohe Latenz

1. Prüfen Sie Window-Größen
2. Reduzieren Sie Aggregations-Komplexität
3. Erhöhen Sie Parallelität

### Pattern matched nicht

1. Prüfen Sie `WITHIN` Constraint
2. Prüfen Sie Event-Reihenfolge (für SEQUENCE)
3. Aktivieren Sie Debug-Logging

## Vergleich mit anderen CEP-Engines

| Feature | ThemisDB CEP | Esper | Apache Flink CEP |
|---------|--------------|-------|------------------|
| Sprache | C++ | Java | Java/Scala |
| EPL | ✅ | ✅ | ✅ (Pattern API) |
| Windows | TUMBLING, SLIDING, SESSION | Alle | Alle |
| State Backend | RocksDB | Memory | RocksDB, Memory |
| Backpressure | ✅ Native | ❌ | ✅ |
| Checkpointing | ✅ | ❌ | ✅ |
| CDC Integration | ✅ Native | ❌ | Connector |
