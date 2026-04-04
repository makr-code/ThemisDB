# Task Scheduler für ThemisDB - Post-Processing System

## Überblick

Das Task-Scheduler-System ermöglicht die periodische Ausführung von AQL-Queries und benutzerdefinierten Funktionen für Post-Processing-Operationen nach der Speicherung in RocksDB.

## Motivation und Anwendungsfall

### Problem
ThemisDB verarbeitet IoT-Daten und komprimiert diese mit Gorilla-Kompression. Wenn einzelne Datensätze ankommen, werden sie einzeln in RocksDB gespeichert (Gorilla kommt nicht zum Einsatz). Ein Batch-System wurde etabliert, um mehrere Daten zu sammeln und dann als Batch mit Gorilla-Kompression zu speichern.

### Lösung
Ein cron-ähnliches Task-System für ThemisDB, das Anweisungen für Post-Processing definiert (nach Ablage in RocksDB) mittels Funktionen + AQL.

## Architektur

### Komponenten

1. **TaskScheduler** (`include/scheduler/task_scheduler.h`)
   - Kern-Scheduler mit periodischer Ausführung
   - Unterstützt AQL-Queries und Custom Functions
   - Konfigurierbare Intervalle (cron-like)
   - Parallele Task-Ausführung mit Ressourcen-Limits

2. **ScheduledTask** (Struktur)
   - Task-Definition mit ID, Name, Beschreibung
   - Task-Typ: AQL_QUERY oder FUNCTION
   - Scheduling-Konfiguration (Intervall, Next-Run)
   - Statistiken (Erfolge, Fehler, Laufzeit)
   - Ressourcen-Limits (Timeout, Retries)

3. **API Handler** (`include/server/task_scheduler_api_handler.h`)
   - RESTful API für Task-Management
   - Endpunkte für CRUD-Operationen
   - Task-Ausführung und Monitoring

### Integration mit bestehenden Systemen

```
┌─────────────────────────────────────────────────────────────┐
│                     ThemisDB Server                         │
├─────────────────────────────────────────────────────────────┤
│                                                             │
│  ┌──────────────┐      ┌──────────────┐                   │
│  │  IoT Data    │─────>│ TSAutoBuffer │                   │
│  │  Ingestion   │      │  (Batching)  │                   │
│  └──────────────┘      └──────┬───────┘                   │
│                               │                            │
│                               v                            │
│                        ┌──────────────┐                    │
│                        │   RocksDB    │                    │
│                        │   Storage    │                    │
│                        └──────┬───────┘                    │
│                               │                            │
│                               │  Post-Processing Trigger   │
│                               v                            │
│                        ┌──────────────┐                    │
│                        │ TaskScheduler│<──── Cron-like     │
│                        │   (NEW!)     │      Scheduling    │
│                        └──────┬───────┘                    │
│                               │                            │
│           ┌───────────────────┼───────────────────┐        │
│           │                   │                   │        │
│           v                   v                   v        │
│    ┌─────────────┐    ┌─────────────┐    ┌─────────────┐ │
│    │ AQL Query   │    │  Custom     │    │  Gorilla    │ │
│    │ Execution   │    │  Functions  │    │  Compress   │ │
│    └─────────────┘    └─────────────┘    └─────────────┘ │
│                                                             │
└─────────────────────────────────────────────────────────────┘
```

## ⚠️ SICHERHEITSRISIKEN

### Kritische Risiken

#### 1. **Arbitrary Code Execution**
- **Risiko**: AQL-Queries und Custom Functions können beliebigen Code ausführen
- **Mitigation**: 
  - Strikte RBAC - nur Administratoren dürfen Tasks registrieren
  - Query-Validierung und Sanitization
  - Sandboxed Execution Environment
  - Resource Limits (CPU, Memory, I/O)

#### 2. **SQL Injection-ähnliche Angriffe**
- **Risiko**: Bösartige AQL-Queries können Daten lesen, ändern oder löschen
- **Mitigation**:
  - Input Validation für alle Task-Parameter
  - Prepared Statements / Parametrisierte Queries
  - Query Complexity Analysis
  - Read-only Execution Mode für nicht-privilegierte Tasks

#### 3. **Resource Exhaustion (DoS)**
- **Risiko**: Bösartige Tasks können System-Ressourcen erschöpfen
- **Mitigation**:
  - Timeout-Limits pro Task (default: 10 Minuten)
  - Max Concurrent Tasks (default: 4)
  - CPU/Memory Quotas
  - Query Complexity Limits
  - Rate Limiting

#### 4. **Privilege Escalation**
- **Risiko**: Tasks laufen mit System-Berechtigungen
- **Mitigation**:
  - Task-spezifische User Contexts
  - Least-Privilege Principle
  - Mandatory Access Control (MAC)
  - Audit Logging aller Task-Operationen

#### 5. **Data Exfiltration**
- **Risiko**: Tasks können sensitive Daten nach außen senden
- **Mitigation**:
  - Network Isolation für Task-Ausführung
  - Outbound Connection Filtering
  - Data Loss Prevention (DLP)
  - Comprehensive Audit Logs

#### 6. **Sensitive Data in Task Definitions**
- **Risiko**: Task-Definitionen (Queries, Parameter) enthalten sensitive Daten
- **Mitigation**:
  - Encryption at Rest für Task-Definitionen
  - Secure Key Management
  - Proper File Permissions (600)
  - Redaction in Logs

### Sicherheits-Checkliste für Produktion

- [ ] **Authentication**: Alle API-Endpunkte mit starker Auth (JWT, mTLS)
- [ ] **Authorization**: RBAC - nur Admins können Tasks verwalten
- [ ] **Input Validation**: Alle Inputs sanitizen (AQL, Parameter, Intervals)
- [ ] **Audit Logging**: Alle Task-Operationen loggen (Create, Update, Execute, Delete)
- [ ] **Rate Limiting**: API-Endpunkte rate-limiten (besonders /execute)
- [ ] **Resource Limits**: CPU, Memory, I/O Quotas pro Task
- [ ] **Query Validation**: AQL-Queries auf schädliche Patterns prüfen
- [ ] **Encryption**: Task-Definitionen at-rest verschlüsseln
- [ ] **Network Isolation**: Task-Ausführung in isoliertem Netzwerk
- [ ] **Monitoring**: Alerting bei ungewöhnlichen Task-Aktivitäten
- [ ] **Sandboxing**: Tasks in isolierten Prozessen/Containern ausführen
- [ ] **HTTPS Only**: Nur verschlüsselte Verbindungen zulassen

## Verwendung

### 1. Grundlegende Task-Registrierung

```cpp
// Task Scheduler erstellen
TaskScheduler scheduler(query_engine);

// Task für Daten-Kompression definieren
ScheduledTask compression_task;
compression_task.id = "compress_old_iot_data";
compression_task.name = "IoT Data Gorilla Compression";
compression_task.description = "Compress old IoT time series data with Gorilla";
compression_task.type = ScheduledTask::TaskType::AQL_QUERY;

// AQL Query für Batch-Verarbeitung
compression_task.aql_query = R"(
    FOR d IN timeseries
    FILTER d.timestamp < DATE_SUB(NOW(), 1, 'hour')
    AND d.compressed == false
    COLLECT metric = d.metric, entity = d.entity INTO batch
    LET compressed_data = COMPRESS_GORILLA(batch)
    UPDATE compressed_data IN timeseries
)";

compression_task.interval = std::chrono::minutes(10);
compression_task.timeout = std::chrono::minutes(5);

// Task registrieren
scheduler.registerTask(compression_task);
scheduler.start();
```

### 2. Custom Function für Post-Processing

```cpp
// Benutzerdefinierte Funktion registrieren
scheduler.registerFunction("batch_compress_iot", 
    [&](const nlohmann::json& params) -> nlohmann::json {
        std::string metric = params["metric"];
        std::string entity = params["entity"];
        
        // Batch-Daten sammeln
        auto batch = collectTimeSeries(metric, entity);
        
        // Gorilla-Kompression anwenden
        auto compressed = gorillaCompress(batch);
        
        // In RocksDB speichern
        storeCompressed(compressed);
        
        return nlohmann::json{
            {"status", "success"},
            {"compressed_points", batch.size()},
            {"compression_ratio", calculateRatio(batch, compressed)}
        };
    }
);

// Task mit Custom Function
ScheduledTask func_task;
func_task.name = "Batch Compression Task";
func_task.type = ScheduledTask::TaskType::FUNCTION;
func_task.function_name = "batch_compress_iot";
func_task.parameters = {
    {"metric", "temperature"},
    {"entity", "sensor_001"}
};
func_task.interval = std::chrono::minutes(5);

scheduler.registerTask(func_task);
```

## Anwendungsfälle für IoT-Daten

### 1. Periodische Gorilla-Kompression
Komprimiert alte IoT-Daten alle 10 Minuten mit Gorilla-Algorithmus für bessere Speichereffizienz.

### 2. Aggregation und Downsampling  
Aggregiert hochfrequente Rohdaten zu niedrigeren Auflösungen (z.B. 1s → 1m → 1h).

### 3. Daten-Cleanup
Löscht alte Rohdaten, wenn Aggregate existieren (Data Lifecycle Management).

### 4. Anomalie-Detektion
Erkennt statistische Anomalien in Echtzeit-IoT-Daten.

## System-Auswirkungen

### Performance

**Positive Effekte:**
- Bessere Kompression durch Batch-Verarbeitung
- Reduzierte Schreiblast durch Batching
- Optimierte Speichernutzung

**Overhead:**
- Scheduler-Loop: ~1-5% CPU
- Memory: ~1KB pro Task + Execution-Memory
- I/O: Abhängig von ausgeführten Tasks

### Ressourcen-Management

```cpp
TaskScheduler::Config config;
config.max_concurrent_tasks = 4;
config.check_interval = std::chrono::seconds(10);
config.persist_tasks = true;
```

## Testing

Siehe `tests/test_task_scheduler.cpp` für umfassende Unit-Tests.

## Task-Abhängigkeiten und DAG-Ausführung

Ab v1.7.0 unterstützt der TaskScheduler Aufgaben-Abhängigkeiten (Dependency Graph).

### Konfiguration

```cpp
ScheduledTask schritt_b;
schritt_b.id = "schritt_b";
schritt_b.dependencies = {"schritt_a"};   // schritt_b läuft erst nach schritt_a
```

### DAG-Ausführung

```cpp
auto result = scheduler.executeDAG({"step_a", "step_b", "step_c"});

// result.succeeded  — map<task_id, json_result>
// result.failed     — map<task_id, error_message>
// result.skipped    — vector<task_id>  (Abhängige fehlgeschlagener Tasks)
```

### Verhalten
- Topologische Sortierung (Kahns Algorithmus) bestimmt die Ausführungsreihenfolge.
- Tasks ohne ausstehende Abhängigkeiten laufen **parallel** innerhalb jeder Welle.
- Schlägt ein Task fehl, werden alle transitiv abhängigen Tasks **übersprungen** (Kaskadenfehler-Schutz).
- Ein Zyklus im Abhängigkeitsgraphen wirft `std::runtime_error`.
- Eine unbekannte Task-ID wirft `std::invalid_argument`.
- Die `dependencies`-Liste wird in `tasks.json` **persistiert** und beim Neustart vollständig wiederhergestellt.

## Referenzen

- `include/scheduler/task_scheduler.h` - API Dokumentation
- `src/scheduler/task_scheduler.cpp` - Implementation
- `tests/test_task_scheduler.cpp` - Unit Tests
- `include/server/task_scheduler_api_handler.h` - REST API Handler
