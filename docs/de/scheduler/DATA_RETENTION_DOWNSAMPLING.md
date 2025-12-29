# Automatische Datenaufbewahrung und Downsampling

## Überblick

Dieses Dokument beschreibt, wie TaskScheduler für automatische Datenaufbewahrung (Retention) und Downsampling von Zeitreihendaten verwendet wird. Das System ersetzt hochauflösende Datenpunkte automatisch durch niedrigauflösende Aggregate nach konfigurierbaren Zeiträumen.

## Motivation

### Problem
- **Speicherexplosion**: 1-Sekunden-Datenpunkte verbrauchen enorm viel Speicher
- **Lange Aufbewahrungszeiten**: Historische Daten werden für Analysen benötigt
- **Unnötige Granularität**: Nach einem Jahr ist 1s-Auflösung meist nicht mehr nötig

### Lösung
Automatisches Ersetzen von hochauflösenden Daten durch Aggregate:
- **1s-Daten nach 1 Jahr** → 1h-Aggregate (Mittelwert, Standardabweichung, Min, Max)
- **Auswertungsfähigkeit erhalten**: Statistische Eigenschaften bleiben erhalten
- **Speicherreduktion**: ~99% Einsparung (3600:1 Ratio für 1s→1h)

## Anwendungsfälle

### Use Case 1: Einfaches Downsampling (1s → 1h nach 1 Jahr)

**Szenario**: IoT-Sensoren senden 1s-Daten. Nach 1 Jahr wird die hohe Auflösung nicht mehr benötigt.

**Implementierung**:
```cpp
ScheduledTask downsample_task;
downsample_task.type = ScheduledTask::TaskType::AQL_QUERY;
downsample_task.aql_query = R"(
    FOR d IN timeseries
    FILTER d.resolution == '1s'
    AND d.timestamp < DATE_SUB(NOW(), 1, 'year')
    COLLECT 
        metric = d.metric,
        entity = d.entity,
        hour = DATE_TRUNC(d.timestamp, 'hour')
    AGGREGATE
        avg_value = AVG(d.value),
        stddev_value = STDDEV(d.value),
        min_value = MIN(d.value),
        max_value = MAX(d.value),
        count = COUNT(d)
    INSERT {
        metric: metric,
        entity: entity,
        timestamp: hour,
        resolution: '1h',
        value: avg_value,
        statistics: {
            avg: avg_value,
            stddev: stddev_value,
            min: min_value,
            max: max_value,
            count: count
        }
    } INTO timeseries_aggregates
    RETURN {metric: metric, points_aggregated: count}
)";
downsample_task.interval = std::chrono::hours(24); // Täglich ausführen
scheduler.registerTask(downsample_task);
```

**Speichereinsparung**: 
- Vorher: 31.536.000 Datenpunkte/Jahr (1 pro Sekunde)
- Nachher: 8.760 Datenpunkte/Jahr (1 pro Stunde)
- **Einsparung: 99,97%**

### Use Case 2: Multi-Tier Retention Policy

**Szenario**: Gestaffelte Aufbewahrung mit verschiedenen Auflösungen.

**Policy**:
- **0-7 Tage**: 1s-Auflösung (Echtzeit-Analysen)
- **7 Tage - 3 Monate**: 1m-Auflösung (detaillierte Historie)
- **3 Monate - 1 Jahr**: 1h-Auflösung (Trendanalysen)
- **> 1 Jahr**: 1d-Auflösung (Langzeit-Archiv)

**Speichereinsparung**:
```
Tier 1 (0-7d):     1s   → 100% der Daten
Tier 2 (7d-3m):    1m   → ~1,67% (60:1)
Tier 3 (3m-1y):    1h   → ~0,03% (3600:1)
Tier 4 (>1y):      1d   → ~0,001% (86400:1)
```

**Durchschnittliche Speichereinsparung über 5 Jahre: ~95%**

### Use Case 3: Metrik-spezifische Retention

**Szenario**: Verschiedene Metriken haben unterschiedliche Anforderungen.

**Beispiel**:
- **Temperatur**: 1s → 1h nach 1 Jahr (niedrige Varianz)
- **Druck**: 1s → 15m nach 90 Tagen (höhere Varianz)
- **Vibration**: 1s → 5m nach 30 Tagen (sehr hohe Varianz)
- **Status**: Keine Aggregation (diskrete Werte)

**Konfiguration**:
```cpp
// Temperatur-Policy
ScheduledTask temp_policy;
temp_policy.parameters = {
    {"metric", "temperature"},
    {"source_resolution", "1s"},
    {"target_resolution", "1h"},
    {"retention_days", 365}
};

// Druck-Policy
ScheduledTask pressure_policy;
pressure_policy.parameters = {
    {"metric", "pressure"},
    {"source_resolution", "1s"},
    {"target_resolution", "15m"},
    {"retention_days", 90}
};
```

## Statistiken und Auswertungsfähigkeit

### Erhaltene Metriken

Bei Aggregation werden folgende Statistiken berechnet und gespeichert:

```json
{
  "timestamp": "2024-01-01T12:00:00Z",
  "resolution": "1h",
  "value": 23.5,  // Durchschnittswert
  "statistics": {
    "avg": 23.5,
    "stddev": 1.2,    // Standardabweichung
    "min": 21.0,      // Minimum
    "max": 25.8,      // Maximum
    "count": 3600,    // Anzahl Originaldatenpunkte
    "sum": 84600.0    // Summe (für weitere Aggregation)
  }
}
```

### Mögliche Analysen

**Mit Aggregaten möglich**:
✅ Trendanalysen (Moving Averages)
✅ Anomalie-Detektion (Outliers via Stddev)
✅ Min/Max-Analysen
✅ Volumenanalysen (Count, Sum)
✅ Histogramme (mit Stddev-Schätzung)
✅ Korrelationsanalysen

**Eingeschränkt**:
⚠️ Exakte Perzentile (nur Schätzung möglich)
⚠️ Exakte Median-Berechnung
⚠️ Detailanalysen auf Sekundenebene

### Genauigkeitsverlust

| Aggregation | Datenverlust | Analysefähigkeit |
|-------------|--------------|------------------|
| 1s → 1m | Minimal | 99% erhalten |
| 1s → 1h | Gering | 95% erhalten |
| 1s → 1d | Moderat | 85% erhalten |

**Empfehlung**: Für langfristige Archive ist 1h-Auflösung ein guter Kompromiss zwischen Speicher und Analysefähigkeit.

## Konfiguration

### Retention Policy Konfiguration

```yaml
# config.yaml
retention_policies:
  # Globale Einstellungen
  enabled: true
  check_interval_hours: 24
  
  # Policy-Definitionen
  policies:
    - name: "temperature_yearly"
      metric_pattern: "temperature_*"
      source_resolution: "1s"
      target_resolution: "1h"
      retention_days: 365
      statistics: ["avg", "stddev", "min", "max"]
      
    - name: "pressure_quarterly"
      metric_pattern: "pressure_*"
      source_resolution: "1s"
      target_resolution: "15m"
      retention_days: 90
      statistics: ["avg", "stddev", "min", "max"]
      
    - name: "vibration_monthly"
      metric_pattern: "vibration_*"
      source_resolution: "1s"
      target_resolution: "5m"
      retention_days: 30
      statistics: ["avg", "stddev", "min", "max", "p95"]
      
  # Multi-Tier Policy
  multi_tier:
    enabled: true
    tiers:
      - resolution: "1s"
        retention_days: 7
      - resolution: "1m"
        retention_days: 90
      - resolution: "1h"
        retention_days: 365
      - resolution: "1d"
        retention_days: -1  # Forever
```

### Programmatische Konfiguration

```cpp
// Retention-Policy aus Config laden
struct RetentionConfig {
    std::string name;
    std::string metric_pattern;
    std::string source_resolution;
    std::string target_resolution;
    int retention_days;
    std::vector<std::string> statistics;
};

void configureRetentionPolicies(
    TaskScheduler& scheduler,
    const std::vector<RetentionConfig>& policies
) {
    for (const auto& policy : policies) {
        ScheduledTask task;
        task.name = policy.name;
        task.type = ScheduledTask::TaskType::FUNCTION;
        task.function_name = "apply_retention_policy";
        task.parameters = {
            {"metric", policy.metric_pattern},
            {"source_resolution", policy.source_resolution},
            {"target_resolution", policy.target_resolution},
            {"retention_days", policy.retention_days},
            {"statistics", policy.statistics}
        };
        task.interval = std::chrono::hours(24);
        
        scheduler.registerTask(task);
    }
}
```

## Monitoring

### Storage Savings Dashboard

**Metriken**:
- `retention_raw_records_total` - Anzahl Rohdatenpunkte
- `retention_aggregated_records_total` - Anzahl Aggregate
- `retention_storage_savings_bytes` - Eingesparter Speicher
- `retention_compression_ratio` - Kompressionsverhältnis
- `retention_policy_executions_total` - Policy-Ausführungen
- `retention_policy_execution_duration_seconds` - Ausführungszeit

**Grafana Dashboard**:
```json
{
  "panels": [
    {
      "title": "Storage Savings Over Time",
      "targets": [
        {
          "expr": "retention_storage_savings_bytes / 1024 / 1024 / 1024"
        }
      ]
    },
    {
      "title": "Compression Ratio by Resolution",
      "targets": [
        {
          "expr": "retention_compression_ratio"
        }
      ]
    }
  ]
}
```

### Beispiel-Report

```
=== Storage Savings Report ===
Period: Last 365 days

Raw Data (1s):
  - Records: 31,536,000
  - Storage: 3,000 MB

Aggregated Data (1h):
  - Records: 8,760
  - Storage: 1.3 MB

Savings:
  - Compression Ratio: 3600:1
  - Storage Saved: 2,998.7 MB (99.96%)
  - Cost Saved: ~$30/month (cloud storage)
```

## Best Practices

### 1. Backup vor Deletion

```cpp
// Immer erst aggregieren, dann löschen
ScheduledTask aggregate_task;
aggregate_task.id = "aggregate_old_data";
// ... Aggregation logic ...

ScheduledTask cleanup_task;
cleanup_task.id = "cleanup_aggregated_data";
cleanup_task.aql_query = R"(
    // Nur löschen wenn Aggregate existieren
    FOR d IN timeseries
    FILTER d.resolution == '1s'
    LET aggregate_exists = (
        FOR a IN timeseries_aggregates
        FILTER a.timestamp == DATE_TRUNC(d.timestamp, 'hour')
        LIMIT 1
        RETURN 1
    )
    FILTER LENGTH(aggregate_exists) > 0
    REMOVE d IN timeseries
)";
```

### 2. Schrittweise Aggregation

```cpp
// Besser: Stufenweise aggregieren (1s → 1m → 1h)
// Schlechteres: Direkt (1s → 1h)
// Vorteil: Bessere Zwischenstufen für Analysen
```

### 3. Retention-Windows

```cpp
// Grace Period: 1 Tag extra behalten für Sicherheit
FILTER d.timestamp < DATE_SUB(NOW(), 366, 'days')  // 1 Jahr + 1 Tag
```

### 4. Inkrementelle Verarbeitung

```cpp
// Nur einen Tag pro Run verarbeiten (für Performance)
FILTER d.timestamp BETWEEN 
    DATE_SUB(NOW(), 366, 'days') AND 
    DATE_SUB(NOW(), 365, 'days')
```

## Performance-Optimierung

### Batch-Größe optimieren

```cpp
// Option 1: Zeitbasierte Batches (empfohlen)
FOR d IN timeseries
FILTER d.timestamp BETWEEN '2023-01-01' AND '2023-01-02'
// Verarbeite 1 Tag pro Task

// Option 2: Count-basierte Batches
FOR d IN timeseries
LIMIT 100000
// Verarbeite max 100k Datenpunkte pro Task
```

### Index-Nutzung

```sql
-- Erstelle Indices für effiziente Queries
CREATE INDEX idx_ts_resolution ON timeseries(resolution, timestamp);
CREATE INDEX idx_ts_metric ON timeseries(metric, timestamp);
```

### Parallel-Ausführung

```cpp
TaskScheduler::Config config;
config.max_concurrent_tasks = 8;  // Mehrere Policies parallel
```

## Kosten-Nutzen-Analyse

### Beispiel: 100 IoT-Sensoren

**Annahmen**:
- 100 Sensoren
- 1 Datenpunkt/Sekunde/Sensor
- 100 Bytes/Datenpunkt
- 5 Jahre Aufbewahrung

**Ohne Retention**:
```
100 Sensoren × 31.536.000 Punkte/Jahr × 5 Jahre × 100 Bytes
= 1.576.800.000.000 Bytes = 1.468 TB
Cloud-Kosten: ~$30.000/Jahr
```

**Mit Multi-Tier Retention**:
```
Tier 1 (0-7d):    100 × 604.800 × 100 Bytes = 5,8 GB
Tier 2 (7d-3m):   100 × 119.520 × 100 Bytes = 1,1 GB
Tier 3 (3m-1y):   100 × 7.884 × 100 Bytes = 75 MB
Tier 4 (>1y):     100 × 1.460 × 100 Bytes × 4 = 58 MB
Total: ~7 GB
Cloud-Kosten: ~$150/Jahr
```

**Einsparung: 99,5% ($29.850/Jahr)**

## Zusammenfassung

### Vorteile ✅
- **Massive Speichereinsparung**: 95-99% für langfristige Daten
- **Auswertungsfähigkeit erhalten**: Statistische Eigenschaften bleiben
- **Automatisiert**: Keine manuelle Intervention nötig
- **Konfigurierbar**: Flexible Policies pro Metrik
- **Skalierbar**: Multi-Tier-Ansatz für große Datenmengen

### Empfehlungen 📋
1. **Start konservativ**: 1s → 1h nach 1 Jahr für alle Metriken
2. **Monitoring aufsetzen**: Storage Savings Dashboard
3. **Schrittweise optimieren**: Metrik-spezifische Policies hinzufügen
4. **Backups**: Immer vor Deletion testen
5. **Grace Period**: 1 Tag extra Puffer

### Nächste Schritte 🚀
1. Retention-Policies in `config.yaml` definieren
2. TaskScheduler mit Policies starten
3. Monitoring-Dashboard einrichten
4. Nach 1 Monat: Savings analysieren und optimieren
5. Nach 3 Monaten: Multi-Tier-Policy evaluieren

## Referenzen

- Beispiel-Code: `examples/data_retention_downsampling_example.cpp`
- TaskScheduler-Doku: `docs/de/scheduler/TASK_SCHEDULER.md`
- Konfiguration: `config.yaml` (retention_policies Sektion)
