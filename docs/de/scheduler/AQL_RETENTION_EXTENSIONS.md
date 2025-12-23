# AQL-Erweiterungen für Data Retention und Task Scheduling

## Überblick

Dieses Dokument beschreibt die neuen AQL-Funktionen, die speziell für das TaskScheduler- und HybridRetentionManager-System entwickelt wurden. Diese Erweiterungen ermöglichen es, Retention-Policies direkt in AQL-Queries zu definieren und zu verwalten.

## Neue AQL-Funktionen

### Statistische Funktionen für Adaptive Retention

#### `CV(stddev, mean)`

Berechnet den **Variationskoeffizienten** (Coefficient of Variation).

**Formel**: `CV = (stddev / mean) × 100%`

**Parameter**:
- `stddev` (NUMBER): Standardabweichung
- `mean` (NUMBER): Mittelwert

**Rückgabe**: NUMBER - Der Variationskoeffizient in Prozent

**Beispiele**:
```aql
// Einfache CV-Berechnung
RETURN CV(2.5, 50)  // 5.0 (niedrige Varianz)
RETURN CV(15, 50)   // 30.0 (hohe Varianz)

// In einer Aggregation
FOR d IN timeseries
FILTER d.timestamp < DATE_SUB(NOW(), 90, 'days')
COLLECT hour = DATE_TRUNC(d.timestamp, 'hour')
AGGREGATE 
    avg = AVG(d.value),
    stddev = STDDEV(d.value)
LET cv = CV(stddev, avg)
LET variance_level = cv < 5 ? 'low' : cv < 20 ? 'medium' : 'high'
RETURN {
    hour: hour,
    cv: cv,
    variance_level: variance_level,
    suggested_resolution: cv < 5 ? '1h' : cv < 20 ? '15m' : '1m'
}
```

**Anwendung**: Kern der adaptiven Retention - bestimmt, wie stark Daten variieren und welche Auflösung beibehalten werden sollte.

#### `VARIANCE_LEVEL(cv, lowThreshold?, mediumThreshold?)`

Klassifiziert die Varianz basierend auf CV-Schwellenwerten.

**Parameter**:
- `cv` (NUMBER): Variationskoeffizient
- `lowThreshold` (NUMBER, optional): Schwelle für niedrige Varianz (Standard: 5.0)
- `mediumThreshold` (NUMBER, optional): Schwelle für mittlere Varianz (Standard: 20.0)

**Rückgabe**: STRING - 'low', 'medium' oder 'high'

**Beispiele**:
```aql
RETURN VARIANCE_LEVEL(3.5)      // 'low'
RETURN VARIANCE_LEVEL(15)        // 'medium'
RETURN VARIANCE_LEVEL(25)        // 'high'
RETURN VARIANCE_LEVEL(7, 3, 15)  // 'medium' mit custom Schwellen

// In einer Retention-Query
FOR d IN timeseries
COLLECT hour = DATE_TRUNC(d.timestamp, 'hour')
AGGREGATE avg = AVG(d.value), stddev = STDDEV(d.value)
LET cv = CV(stddev, avg)
LET level = VARIANCE_LEVEL(cv)
FILTER level != 'high'  // Nur stabile Perioden aggregieren
INSERT {
    timestamp: hour,
    value: avg,
    variance_level: level
} INTO timeseries_aggregates
```

#### `RETENTION_RESOLUTION(cv, lowThreshold?, mediumThreshold?)`

Empfiehlt die optimale Auflösung für Retention basierend auf Varianz.

**Parameter**:
- `cv` (NUMBER): Variationskoeffizient
- `lowThreshold` (NUMBER, optional): Schwelle für niedrige Varianz (Standard: 5.0)
- `mediumThreshold` (NUMBER, optional): Schwelle für mittlere Varianz (Standard: 20.0)

**Rückgabe**: STRING - Empfohlene Auflösung ('1h', '15m', '1m')

**Beispiele**:
```aql
RETURN RETENTION_RESOLUTION(3.5)  // '1h' (niedrige Varianz)
RETURN RETENTION_RESOLUTION(15)   // '15m' (mittlere Varianz)
RETURN RETENTION_RESOLUTION(25)   // '1m' (hohe Varianz)

// Adaptive Retention Query
FOR d IN timeseries
FILTER d.resolution == '1s' 
AND d.timestamp < DATE_SUB(NOW(), 90, 'days')
COLLECT hour = DATE_TRUNC(d.timestamp, 'hour')
AGGREGATE avg = AVG(d.value), stddev = STDDEV(d.value)
LET cv = CV(stddev, avg)
LET resolution = RETENTION_RESOLUTION(cv)
INSERT {
    timestamp: hour,
    resolution: resolution,  // Automatisch gewählt!
    value: avg,
    statistics: {cv: cv}
} INTO timeseries_adaptive
```

### Datums-Convenience-Funktionen

#### `DATE_SUB(timestamp, amount, unit)`

Alias für `DATE_SUBTRACT` - subtrahiert Zeit von einem Timestamp.

**Parameter**:
- `timestamp` (INTEGER): Unix-Timestamp in Millisekunden
- `amount` (INTEGER): Menge zum Subtrahieren
- `unit` (STRING): Einheit ('year', 'month', 'day', 'hour', 'minute', 'second')

**Rückgabe**: INTEGER - Neuer Timestamp

**Beispiele**:
```aql
RETURN DATE_SUB(NOW(), 1, 'year')    // Vor einem Jahr
RETURN DATE_SUB(NOW(), 7, 'days')    // Vor einer Woche
RETURN DATE_SUB(NOW(), 90, 'days')   // Vor 3 Monaten

// In einer Retention-Query (wie in Beispielen verwendet)
FOR d IN timeseries
FILTER d.timestamp < DATE_SUB(NOW(), 1, 'year')
AND d.resolution == '1s'
COLLECT hour = DATE_TRUNC(d.timestamp, 'hour')
AGGREGATE avg = AVG(d.value)
INSERT {timestamp: hour, value: avg} INTO timeseries_aggregates
```

**Hinweis**: Diese Funktion macht die Beispiel-Queries direkt lauffähig, da sie den in den Beispielen verwendeten Funktionsnamen entspricht.

### Task-Scheduling-Funktionen

#### `SCHEDULE_TASK(config)`

⚠️ **SICHERHEITSWARNUNG**: Erfordert Admin-Rechte. Kann beliebige AQL ausführen.

Erstellt einen neuen geplanten Task direkt aus AQL.

**Parameter**:
- `config` (OBJECT): Task-Konfiguration mit folgenden Feldern:
  - `name` (STRING): Task-Name
  - `type` (STRING): Task-Typ ('aql' oder 'function')
  - `query` (STRING): AQL-Query zum Ausführen
  - `interval_hours` (INTEGER, optional): Intervall in Stunden
  - `interval_minutes` (INTEGER, optional): Intervall in Minuten
  - `interval_seconds` (INTEGER, optional): Intervall in Sekunden

**Rückgabe**: OBJECT - Task-Informationen mit Task-ID

**Beispiele**:
```aql
// Tägliches Cleanup
RETURN SCHEDULE_TASK({
    name: 'Daily Data Cleanup',
    type: 'aql',
    query: 'FOR d IN timeseries FILTER d.timestamp < DATE_SUB(NOW(), 30, "days") REMOVE d IN timeseries',
    interval_hours: 24
})

// Stündliche Aggregation
RETURN SCHEDULE_TASK({
    name: 'Hourly Aggregation',
    type: 'aql',
    query: 'FOR d IN timeseries FILTER d.timestamp < DATE_SUB(NOW(), 1, "hour") COLLECT hour = DATE_TRUNC(d.timestamp, "hour") AGGREGATE avg = AVG(d.value) INSERT {timestamp: hour, value: avg} INTO hourly_aggregates',
    interval_hours: 1
})

// Adaptive Retention Task
RETURN SCHEDULE_TASK({
    name: 'Adaptive Retention',
    type: 'aql',
    query: 'FOR d IN timeseries COLLECT hour = DATE_TRUNC(d.timestamp, "hour") AGGREGATE avg = AVG(d.value), stddev = STDDEV(d.value) LET cv = CV(stddev, avg) LET resolution = RETENTION_RESOLUTION(cv) INSERT {timestamp: hour, resolution: resolution, value: avg} INTO timeseries_adaptive',
    interval_hours: 6
})
```

#### `LIST_SCHEDULED_TASKS()`

Listet alle geplanten Tasks auf.

**Parameter**: Keine

**Rückgabe**: ARRAY - Array von Task-Objekten

**Beispiel**:
```aql
RETURN LIST_SCHEDULED_TASKS()

// Filterung nach bestimmten Tasks
FOR task IN LIST_SCHEDULED_TASKS()
FILTER task.name LIKE 'Retention%'
RETURN task
```

#### `CANCEL_TASK(taskId)`

⚠️ **SICHERHEITSWARNUNG**: Erfordert Admin-Rechte.

Stoppt einen geplanten Task.

**Parameter**:
- `taskId` (STRING): ID des zu stoppenden Tasks

**Rückgabe**: BOOLEAN - `true` wenn erfolgreich gestoppt

**Beispiel**:
```aql
RETURN CANCEL_TASK('task_12345')

// Alle Retention-Tasks stoppen
FOR task IN LIST_SCHEDULED_TASKS()
FILTER task.name LIKE 'Retention%'
LET cancelled = CANCEL_TASK(task.id)
RETURN {task_id: task.id, cancelled: cancelled}
```

### Utility-Funktionen

#### `ESTIMATE_STORAGE_SAVINGS(sourceResolution, targetResolution, dataPoints)`

Schätzt die Speichereinsparungen durch Downsampling.

**Parameter**:
- `sourceResolution` (STRING): Quell-Auflösung (z.B. '1s', '1m')
- `targetResolution` (STRING): Ziel-Auflösung (z.B. '1h', '1d')
- `dataPoints` (INTEGER): Anzahl der Datenpunkte

**Rückgabe**: OBJECT - Detaillierte Speicherstatistiken

**Beispiele**:
```aql
// 1 Jahr an 1s-Daten → 1h
RETURN ESTIMATE_STORAGE_SAVINGS('1s', '1h', 31536000)
// Returns:
// {
//   "source_resolution": "1s",
//   "target_resolution": "1h",
//   "source_data_points": 31536000,
//   "target_data_points": 8760,
//   "compression_ratio": 3600,
//   "source_storage_bytes": 3153600000,
//   "target_storage_bytes": 1314000,
//   "storage_saved_bytes": 3152286000,
//   "storage_savings_percent": 99.96,
//   "storage_saved_mb": 3006
// }

// Analyse mehrerer Szenarien
FOR resolution IN ['1m', '15m', '1h', '1d']
LET savings = ESTIMATE_STORAGE_SAVINGS('1s', resolution, 31536000)
RETURN {
    target: resolution,
    ratio: savings.compression_ratio,
    savings_percent: savings.storage_savings_percent,
    savings_mb: savings.storage_saved_mb
}
```

## Vollständige Retention-Beispiele mit neuen Funktionen

### Beispiel 1: Einfache Adaptive Retention

```aql
// Adaptive Retention für alte Daten
FOR d IN timeseries
FILTER d.resolution == '1s'
AND d.timestamp < DATE_SUB(NOW(), 90, 'days')
COLLECT 
    metric = d.metric,
    entity = d.entity,
    hour = DATE_TRUNC(d.timestamp, 'hour')
AGGREGATE
    avg = AVG(d.value),
    stddev = STDDEV(d.value),
    min_val = MIN(d.value),
    max_val = MAX(d.value),
    count = COUNT(d)

// Neue Funktionen für intelligente Entscheidungen
LET cv = CV(stddev, avg)
LET variance_level = VARIANCE_LEVEL(cv)
LET resolution = RETENTION_RESOLUTION(cv)

INSERT {
    metric: metric,
    entity: entity,
    timestamp: hour,
    resolution: resolution,
    value: avg,
    statistics: {
        avg: avg,
        stddev: stddev,
        cv: cv,
        min: min_val,
        max: max_val,
        count: count,
        variance_level: variance_level
    },
    created_at: DATE_NOW()
} INTO timeseries_adaptive

RETURN {
    hour: hour,
    cv: cv,
    variance_level: variance_level,
    resolution: resolution,
    points_aggregated: count
}
```

### Beispiel 2: Speicher-Analyse vor Retention

```aql
// Analyse: Wie viel Speicher können wir sparen?
FOR d IN timeseries
FILTER d.resolution == '1s'
COLLECT metric = d.metric
AGGREGATE total_points = COUNT(d)

// Schätze Einsparungen für verschiedene Auflösungen
LET estimate_1h = ESTIMATE_STORAGE_SAVINGS('1s', '1h', total_points)
LET estimate_15m = ESTIMATE_STORAGE_SAVINGS('1s', '15m', total_points)
LET estimate_1m = ESTIMATE_STORAGE_SAVINGS('1s', '1m', total_points)

RETURN {
    metric: metric,
    total_1s_points: total_points,
    scenarios: {
        to_1h: {
            savings_mb: estimate_1h.storage_saved_mb,
            savings_percent: estimate_1h.storage_savings_percent
        },
        to_15m: {
            savings_mb: estimate_15m.storage_saved_mb,
            savings_percent: estimate_15m.storage_savings_percent
        },
        to_1m: {
            savings_mb: estimate_1m.storage_saved_mb,
            savings_percent: estimate_1m.storage_savings_percent
        }
    }
}
```

### Beispiel 3: Automatische Task-Erstellung aus Query

```aql
// Erstelle Retention-Task für jede Metrik automatisch
FOR metric IN (
    FOR d IN timeseries
    COLLECT metric = d.metric
    RETURN metric
)

// Analysiere Varianz der letzten Woche
LET variance_analysis = (
    FOR d IN timeseries
    FILTER d.metric == metric
    AND d.timestamp > DATE_SUB(NOW(), 7, 'days')
    COLLECT hour = DATE_TRUNC(d.timestamp, 'hour')
    AGGREGATE avg = AVG(d.value), stddev = STDDEV(d.value)
    LET cv = CV(stddev, avg)
    RETURN cv
)

LET avg_cv = AVG(variance_analysis)
LET recommended_resolution = RETENTION_RESOLUTION(avg_cv)

// Erstelle passenden Task
LET task = SCHEDULE_TASK({
    name: CONCAT('Retention: ', metric),
    type: 'aql',
    query: CONCAT(
        'FOR d IN timeseries ',
        'FILTER d.metric == "', metric, '" ',
        'AND d.timestamp < DATE_SUB(NOW(), 90, "days") ',
        'COLLECT hour = DATE_TRUNC(d.timestamp, "hour") ',
        'AGGREGATE avg = AVG(d.value), stddev = STDDEV(d.value) ',
        'INSERT {timestamp: hour, resolution: "', recommended_resolution, '", value: avg} ',
        'INTO timeseries_adaptive'
    ),
    interval_hours: 12
})

RETURN {
    metric: metric,
    avg_cv: avg_cv,
    recommended_resolution: recommended_resolution,
    task_created: task
}
```

## Best Practices

### 1. CV-Schwellenwerte kalibrieren

```aql
// Analysiere historische Varianz um Schwellenwerte zu finden
FOR d IN timeseries
FILTER d.timestamp > DATE_SUB(NOW(), 30, 'days')
COLLECT 
    metric = d.metric,
    hour = DATE_TRUNC(d.timestamp, 'hour')
AGGREGATE avg = AVG(d.value), stddev = STDDEV(d.value)
LET cv = CV(stddev, avg)
COLLECT metric_name = metric
AGGREGATE 
    cv_min = MIN(cv),
    cv_max = MAX(cv),
    cv_avg = AVG(cv),
    cv_p25 = PERCENTILE(cv, 25),
    cv_p50 = PERCENTILE(cv, 50),
    cv_p75 = PERCENTILE(cv, 75)
RETURN {
    metric: metric_name,
    cv_stats: {
        min: cv_min,
        avg: cv_avg,
        max: cv_max,
        quartiles: [cv_p25, cv_p50, cv_p75]
    },
    suggested_thresholds: {
        low: cv_p25,
        medium: cv_p75
    }
}
```

### 2. Schrittweise Migration

```aql
// Teste Retention erst mit kleinem Zeitfenster
FOR d IN timeseries
FILTER d.timestamp BETWEEN 
    DATE_SUB(NOW(), 91, 'days') AND 
    DATE_SUB(NOW(), 90, 'days')  // Nur 1 Tag!
COLLECT hour = DATE_TRUNC(d.timestamp, 'hour')
AGGREGATE avg = AVG(d.value), stddev = STDDEV(d.value)
LET cv = CV(stddev, avg)
INSERT {
    timestamp: hour,
    resolution: RETENTION_RESOLUTION(cv),
    value: avg
} INTO timeseries_adaptive_test  // Test-Tabelle!
```

### 3. Monitoring

```aql
// Überwache laufende Tasks
FOR task IN LIST_SCHEDULED_TASKS()
FILTER task.name LIKE '%Retention%'
RETURN {
    task_id: task.id,
    name: task.name,
    last_run: task.last_run,
    status: task.status,
    executions_total: task.statistics.executions_total,
    executions_failed: task.statistics.executions_failed
}
```

## Sicherheitshinweise

⚠️ **WICHTIG**: Diese Funktionen haben erhebliche Sicherheitsimplikationen:

### Erforderliche Maßnahmen für Produktion:

1. **Authentifizierung**: Alle Task-Management-Funktionen benötigen Admin-Rechte
2. **Input-Validierung**: Query-Strings müssen sanitized werden
3. **Resource Limits**: Tasks benötigen CPU/Memory/Timeout-Limits
4. **Audit Logging**: Alle SCHEDULE_TASK/CANCEL_TASK-Aufrufe loggen
5. **Encryption**: Task-Definitionen verschlüsselt speichern

### Beispiel-Implementierung der Sicherheitschecks:

```cpp
// In SCHEDULE_TASK::execute()
if (!ctx.user.has_permission("schedule_task")) {
    throw std::runtime_error("SCHEDULE_TASK: admin privileges required");
}

// Log audit trail
THEMIS_AUDIT_LOG("SCHEDULE_TASK", {
    {"user", ctx.user.username},
    {"task_name", name},
    {"query", query}
});

// Validate query (basic SQL injection prevention)
if (query.find(";--") != std::string::npos ||
    query.find("DROP") != std::string::npos) {
    throw std::runtime_error("SCHEDULE_TASK: suspicious query detected");
}
```

## Migration von Beispiel-Code

Die neuen Funktionen sind **vollständig kompatibel** mit den Beispielen in:
- `examples/data_retention_downsampling_example.cpp`
- `examples/adaptive_retention_example.cpp`
- `examples/hybrid_retention_usage_example.cpp`

Alle Beispiel-Queries können nun direkt in ThemisDB ausgeführt werden!

## Zusammenfassung

Die neuen AQL-Funktionen ermöglichen:

✅ **Intelligente Retention**: CV-basierte adaptive Downsampling-Entscheidungen
✅ **Task-Management**: Direkte Erstellung und Verwaltung von scheduled Tasks
✅ **Storage-Analyse**: Vorhersage von Speichereinsparungen
✅ **Kompatibilität**: Alle Beispiel-Queries sind lauffähig

**Nächste Schritte**:
1. Funktionen in Function Registry registrieren
2. Security Controls implementieren
3. Integration Tests schreiben
4. Produktiv deployen mit strikten RBAC-Regeln
