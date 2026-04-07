# Zeitreihen-Speichermethoden (Timeline, IOT)

**Stand:** 6. April 2026  
**Version:** 1.0.0  
**Kategorie:** Technical Documentation

---

## Fragestellung

**Werden Zeitreihenelemente (timeline, IOT) auch als singuläre RocksDB-Entities gespeichert, oder als Batch mit Gorilla-Kompression?**

## Antwort

**Beides ist möglich** – die Speichermethode hängt davon ab, welche API-Methode verwendet wird:

### 1. Singuläre RocksDB-Entities

**Wann:** Bei Verwendung von `putDataPoint()` (Einzelpunkt-Einfügung)

**Eigenschaften:**
- Jeder Datenpunkt wird als **separate RocksDB-Entity** gespeichert
- Key-Format: `ts:{metric}:{entity}:{timestamp_ms}`
- Value-Format: JSON mit vollständigen DataPoint-Informationen
- **Keine Gorilla-Kompression**, auch wenn diese in der Konfiguration aktiviert ist
- Direkter Speicherzugriff ohne Buffering

**Beispiel:**
```cpp
TSStore ts(db, cf);
TSStore::DataPoint point{
    .metric = "temperature",
    .entity = "sensor_001",
    .timestamp_ms = 1700000000000,
    .value = 22.5,
    .tags = {{"location", "room_a"}},
    .metadata = {}
};
ts.putDataPoint(point);  // Wird als ts:temperature:sensor_001:1700000000000 gespeichert
```

**Anwendungsfälle:**
- Echtzeit-IoT-Sensordaten
- Streaming-Metriken
- Timeline-Events
- Einzelne Messwerte

### 2. Batch mit Gorilla-Kompression

**Wann:** Bei Verwendung von `putDataPoints()` (Batch-Einfügung) mit aktivierter Gorilla-Kompression

**Eigenschaften:**
- Datenpunkte werden nach `metric:entity` gruppiert
- Punkte innerhalb einer Gruppe werden nach Timestamp sortiert
- Gruppe wird mit **Gorilla-Codec** komprimiert
- Key-Format: `tsc:{metric}:{entity}:{first_ts}:{last_ts}`
- Value-Format: JSON-Metadaten + binärer Gorilla-Chunk
- **Kompressionsrate: 10-20x** bei etwa +15% CPU-Overhead

**Beispiel:**
```cpp
TSStore::Config config;
config.compression = TSStore::CompressionType::Gorilla;
config.chunk_size_hours = 24;

TSStore ts(db, cf, config);

std::vector<TSStore::DataPoint> points = {
    {.metric="temperature", .entity="sensor_001", .timestamp_ms=1700000000000, .value=22.5},
    {.metric="temperature", .entity="sensor_001", .timestamp_ms=1700000060000, .value=22.8},
    {.metric="temperature", .entity="sensor_001", .timestamp_ms=1700000120000, .value=23.1},
    // ... weitere Punkte
};

ts.putDataPoints(points);  // Wird als tsc:temperature:sensor_001:1700000000000:1700003600000 gespeichert
```

**Anwendungsfälle:**
- Batch-Import historischer Daten
- Bulk-Operationen
- Daten-Migration
- Offline-Analysen

## Technische Details

### Gorilla-Kompression

Der Gorilla-Codec verwendet zwei Techniken:

1. **Timestamp-Kompression:** Delta-of-Delta-Encoding mit ZigZag + Varint
2. **Value-Kompression:** XOR der IEEE-754 Double-Repräsentation mit Leading/Trailing-Zero-Packing

**Implementierung:**
- `include/timeseries/gorilla.h`
- `src/timeseries/gorilla.cpp`

### Schlüssel-Formate

| Speichermethode | Key-Format | Beispiel |
|----------------|-----------|----------|
| Singular | `ts:{metric}:{entity}:{timestamp}` | `ts:cpu:server01:00000001700000000000` |
| Batch (Gorilla) | `tsc:{metric}:{entity}:{first_ts}:{last_ts}` | `tsc:cpu:server01:1700000000000:1700003600000` |

### Performance-Charakteristiken

| Aspekt | Singular | Batch (Gorilla) |
|--------|----------|-----------------|
| Schreiblatenz | Niedrig (~1ms) | Mittel (~10-50ms) |
| Speichereffizienz | 100% (Basis) | 5-10% (10-20x Kompression) |
| CPU-Overhead | Minimal | +15% |
| Geeignet für | Streaming, Echtzeit | Batch, Historisch |

## HTTP-API-Verhalten

### `/ts/put` Endpoint

Der HTTP-Endpoint `/ts/put` verwendet **intern `putDataPoint()`**, daher:
- **Speichermethode:** Singuläre RocksDB-Entities
- **Keine Gorilla-Kompression**, auch wenn konfiguriert
- Geeignet für Echtzeit-Streaming

**Beispiel:**
```bash
curl -X POST http://localhost:8080/ts/put \
  -H "Content-Type: application/json" \
  -d '{
    "metric": "temperature",
    "entity": "sensor_001",
    "timestamp_ms": 1700000000000,
    "value": 22.5,
    "tags": {"location": "room_a"}
  }'
```

### Batch-API (C++ direkt)

Für Batch-Operationen mit Kompression ist direkter C++-Zugriff erforderlich:

```cpp
TSStore ts(db, cf, {.compression = TSStore::CompressionType::Gorilla});
std::vector<TSStore::DataPoint> batch = loadBatchData();
ts.putDataPoints(batch);  // Mit Gorilla-Kompression
```

## Limitierungen und Roadmap

### Aktuelle Limitierungen

1. **Einzelpunkt-Kompression:** `putDataPoint()` unterstützt keine Gorilla-Kompression
2. **HTTP-API:** Kein Batch-Endpoint für komprimierte Speicherung
3. **Buffering:** Kein automatisches Buffering von Einzelpunkten zu Chunks

### Geplante Verbesserungen

1. ✅ **Auto-Buffering:** Automatische Pufferung von Einzelpunkten für Batch-Kompression
   - **Implementiert:** `TSAutoBuffer` (siehe [AUTO_BUFFER.md](./AUTO_BUFFER.md))
   - Konfigurierbare Buffer-Größe und Flush-Intervalle
   - Thread-safe, mit Zeit- und Größen-basierten Flush-Strategien
   - Nutzt bestehende Patterns (CEP Engine, Backpressure Protocol)
   - Keine neuen Dependencies erforderlich

2. **Batch-HTTP-API:** Endpoint für Batch-Einfügungen
   - `POST /ts/put/batch` mit Array von DataPoints
   - Automatische Gorilla-Kompression wenn konfiguriert
   - Integration mit TSAutoBuffer geplant

3. **Adaptive Kompression:** Automatische Wahl zwischen Singular/Batch basierend auf Datenmustern

## Empfehlungen

### Wann Singular verwenden

✅ **Verwenden Sie `putDataPoint()` (Singular) für:**
- Echtzeit-IoT-Sensordaten
- Streaming-Metriken
- Timeline-Events
- Niedrige Latenz-Anforderungen
- HTTP-API-Integration

### Wann Batch mit Gorilla verwenden

✅ **Verwenden Sie `putDataPoints()` (Batch + Gorilla) für:**
- Historische Daten-Importe
- Batch-Migration
- Offline-Analysen
- Speicheroptimierung (bei großen Datenmengen)
- C++-basierte Bulk-Operationen

✅ **ODER verwenden Sie `TSAutoBuffer` für automatisches Batching:**
- Automatische Pufferung von Einzelpunkten
- Konfigurierbare Flush-Strategien (Zeit/Größe)
- Transparent für Echtzeit-Anwendungen
- Siehe [AUTO_BUFFER.md](./AUTO_BUFFER.md) für Details

### Hybride Strategie

Für optimale Ergebnisse kombinieren Sie beide Ansätze:

```cpp
// Echtzeit-Streaming (Singular)
while (sensor.hasData()) {
    auto point = sensor.readPoint();
    ts.putDataPoint(point);
}

// Periodisches Batch-Komprimieren (z.B. alle 24h)
void compactOldData() {
    auto old_points = ts.query({
        .metric = "temperature",
        .from_timestamp_ms = now() - 24h,
        .to_timestamp_ms = now()
    });
    
    // Lösche alte Singular-Einträge
    ts.deleteOldData(now() - 24h);
    
    // Speichere als Gorilla-Chunk
    ts.putDataPoints(old_points.second);
}
```

## Zusammenfassung

| Frage | Antwort |
|-------|---------|
| **Werden Zeitreihen als singuläre RocksDB-Entities gespeichert?** | **Ja**, bei Verwendung von `putDataPoint()` |
| **Werden Zeitreihen als Batch mit Gorilla-Kompression gespeichert?** | **Ja**, bei Verwendung von `putDataPoints()` mit aktivierter Kompression |
| **Welche Methode ist Standard?** | Singular (via HTTP-API `/ts/put`) |
| **Welche Methode ist effizienter?** | Batch + Gorilla (10-20x Kompression) |

**Fazit:** ThemisDB unterstützt **beide Speichermethoden**. Die Wahl hängt von Ihrem Anwendungsfall ab (Echtzeit vs. Batch) und der verwendeten API-Methode.

## Siehe auch

- [Time-Series Engine (TSStore)](../features/features_time_series.md)
- [Gorilla Compression](../src/timeseries/gorilla.cpp.md)
- [TSStore Implementation](../src/timeseries/tsstore.cpp.md)
- [HTTP API Reference](../apis/HTTP_API_REFERENCE.md)

## Referenzen

- **Gorilla Paper:** "Gorilla: A Fast, Scalable, In-Memory Time Series Database" (Facebook, 2015)
- **RocksDB:** https://rocksdb.org/
- **ThemisDB Source:** `include/timeseries/tsstore.h`, `src/timeseries/tsstore.cpp`
