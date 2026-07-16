# Time Series Module

**Stand:** 13. Mai 2026
**Version:** 1.9.x
**Kategorie:** TimeSeries

---

## Übersicht

Das Time-Series-Modul bietet hochperformante Zeitserien-Speicherung für Metriken und Events
mit Gorilla-Kompression, Continuous Aggregates, Retention Policies, automatischem Batching,
Anomalie-Erkennung, Streaming-Cursor und einem Prometheus-Remote-Write-Endpunkt.

## Quell-Code Referenz

| Komponente | Header | Source | Beschreibung |
|------------|--------|--------|--------------|
| TimeSeriesStore | `timeseries.h` | `timeseries.cpp` | Vereinfachte Haupt-API |
| TSStore | `tsstore.h` | `tsstore.cpp` | Primäre Write/Query-API mit Gorilla |
| GorillaEncoder | `gorilla.h` | `gorilla.cpp` | Delta-of-delta + XOR-Float-Kompression |
| GorillaDecoder | `gorilla.h` | `gorilla.cpp` | Dekompression (inkl. Legacy-Chunks) |
| GorillaSIMDDecoder | `gorilla_simd.h` | `gorilla_simd.cpp` | AVX2/NEON-beschleunigter Decoder |
| HeuristicCompressionSelector | `compression_selector.h` | `compression_selector.cpp` | Automatische Codec-Wahl pro Series |
| ContinuousAggMaterializationEngine | `continuous_agg.h` | `continuous_agg.cpp` | TimescaleDB-style Materialized Views |
| ContinuousAggregateManager | `continuous_agg.h` | `continuous_agg.cpp` | Inkrementelles Refresh mit Watermark |
| DistributedAggregateCoordinator | `continuous_agg.h` | `continuous_agg.cpp` | Multi-Shard Aggregation |
| RetentionManager | `retention.h` | `retention.cpp` | Per-Metric TTL mit Audit-Logging |
| TSAutoBuffer | `ts_auto_buffer.h` | `ts_auto_buffer.cpp` | Auto-Batching für Einzelpunkt-Inserts |
| DownsamplingPipeline | `downsampling.h` | `downsampling.cpp` | Multi-Tier Downsampling (raw→1min→1h→1d) |
| TierSelector | `downsampling.h` | `downsampling.cpp` | Query-Routing zur gröbsten passenden Tier |
| Hypertable | `hypertable.h` | `hypertable.cpp` | TimescaleDB-style Zeitchunk-Partitionierung |
| AnomalyDetector | `anomaly_detection.h` | `anomaly_detection.cpp` | Z-Score und IQR Anomalie-Erkennung |
| GapFiller | `gap_fill.h` | `gap_fill.cpp` | Gap-Fill Interpolation (Forward/Linear/Backward) |
| TsStreamCursor | `ts_stream_cursor.h` | `ts_stream_cursor.cpp` | Lazy paginierter Iterator für große Abfragen |
| EncryptedChunkStore | `encrypted_chunk_store.h` | `encrypted_chunk_store.cpp` | AES-256-GCM Verschlüsselung (compress-then-encrypt) |
| TimeSeriesMetrics | `timeseries_metrics.h` | `timeseries_metrics.cpp` | Prometheus Metriken (Ingest, Kompression, Lag) |
| PrometheusRemoteWrite | `prometheus_remote_write.h` | `prometheus_remote_write.cpp` | `POST /api/v1/prom/write` Endpoint |

**Gesamt:** 23 Header, 26 Source-Dateien

## Implementierte Klassen

### TimeSeriesStore (vereinfachte API)

```cpp
#include "timeseries/timeseries.h"

themis::TimeSeriesStore ts(db, cf);

// Metriken schreiben
ts.put("cpu_usage", "server-1", {now_ms(), 75.5, {}});

// Range Query
auto pts = ts.query("cpu_usage", "server-1", {
    .from_ms = start, .to_ms = end, .limit = 1000
});

// Aggregation
auto agg = ts.aggregate("cpu_usage", "server-1", {.from_ms = start, .to_ms = end});
// agg.avg = 76.85, agg.min = 75.5, agg.max = 78.2, agg.sum, agg.count

// Letzter Wert
auto latest = ts.getLatest("cpu_usage", "server-1");
```

### TSStore (primäre API, v1.x+)

```cpp
#include "timeseries/tsstore.h"

TSStore::Config config;
config.compression = TSStore::CompressionType::Gorilla;
config.chunk_size_hours = 24;
config.late_arrival_window_ms = 5000; // 0 = alle Timestamps akzeptieren

TSStore ts(db, cf, config);

// Einzelpunkt (via Auto-Buffer oder direkt)
ts.putDataPoint(point);

// Batch mit Gorilla-Kompression
ts.putDataPoints(batch);
```

### Gorilla-Kompression (10–20×)

```cpp
#include "timeseries/gorilla.h"

// Kodieren
GorillaEncoder enc;
enc.add(t0, v0);
enc.add(t1, v1);
auto bytes = enc.finish(); // 3-Byte Header (v1+) + Payload

// Dekodieren
GorillaDecoder dec(bytes);
while (auto pt = dec.next()) {
    auto [ts, val] = *pt;
}
if (dec.hasError()) {
    // Truncated oder korrupte Daten; dec.decodedCount() erfolgreiche Punkte
}
```

### ContinuousAggMaterializationEngine (v1.9.x)

```cpp
#include "timeseries/continuous_agg.h"

ContinuousAggMaterializationEngine engine(&store);

// Registrieren (ähnlich wie CREATE MATERIALIZED VIEW)
ContinuousAggDefinition def;
def.name   = "cpu_5min";
def.config = { "cpu_usage", "server-01", AggWindow{std::chrono::minutes(5)} };
engine.createAggregate(def);

// Inkrementelles Refresh
engine.refreshAggregate("cpu_5min", now_ms());
engine.refreshAll(now_ms()); // alle aktiven Aggregates

// Abfrage materialiserter Daten
auto pts = engine.queryMaterialized("cpu_5min", from_ms, to_ms);

// Status
auto s = engine.getAggregateStatus("cpu_5min");
// s->watermark_ms, s->status (ACTIVE/STALE/INACTIVE), s->windows_written
```

### RetentionManager

```cpp
#include "timeseries/retention.h"

RetentionPolicy policy;
policy.per_metric["cpu_usage"] = std::chrono::days(30);
RetentionManager rm(&store, policy);
rm.startAsync(std::chrono::hours(1)); // Background-Bereinigung

// Staged Deletion (Mark → Soft → Hard)
StagedDeletionPolicy staged;
staged.mark_after        = std::chrono::days(25);
staged.soft_delete_after = std::chrono::days(28);
staged.hard_delete_after = std::chrono::days(30);
rm.setStagedDeletion(staged);

// Compliance-Audit-Callback
rm.setAuditCallback([](const RetentionAuditEntry& e) { /* log */ });
```

## Hochleistungs-APIs (v1.9.x)

### TSStore::putBatch — Zero-Copy Batch Write

```cpp
std::vector<themis::TSStore::TSRow> rows = {
    {"cpu_usage", "server-01", now_ms(),     0.72},
    {"cpu_usage", "server-01", now_ms() + 1, 0.74},
};
auto result = ts_store->putBatch(rows);
// result.value().ok_count == 2; alle Rows in einem WriteBatch committet
```

Ziel: ≥ 1 M Zeilen/s bei p99 < 2 ms auf 8-Kern-Host.

### TsStreamCursor — Lazy Streaming Iterator

```cpp
#include "timeseries/ts_stream_cursor.h"

TSStore::QueryOptions opts;
opts.metric = "cpu_usage";
opts.from_timestamp_ms = t0;
opts.to_timestamp_ms   = t1;

auto cursor = themis::timeseries::TsStreamCursor::open(store, opts);
while (cursor->valid()) {
    process(cursor->current());
    cursor->advance();
}
// cursor->rowsConsumed(), cursor->pagesFetched()
```

Standard-Seitengröße: **4 096** Datenpunkte. Ziel: ≥ 500 MB/s auf NVMe.

### TSAutoBuffer — Auto-Batching für IoT/Streaming

```cpp
#include "timeseries/ts_auto_buffer.h"

TSAutoBufferConfig cfg;
cfg.max_points_per_buffer = 500;
cfg.flush_interval        = std::chrono::seconds(5);
cfg.enable_adaptive_flush = true;

TSAutoBuffer buffer(&store, cfg);
store.setAutoBuffer(&buffer);
buffer.start();

// Einzelpunkt — wird gebuffert und als Gorilla-Chunk geflusht
store.putDataPoint(point);

// Backpressure-Status
auto stats = buffer.getStats();
// stats.backpressure_events, stats.points_flushed, stats.flush_count

buffer.stop(); // flusht alle verbleibenden Punkte
```

### Anomalie-Erkennung

```cpp
#include "timeseries/anomaly_detection.h"

AnomalyConfig cfg;
cfg.method           = AnomalyMethod::ZScore;
cfg.zscore_threshold = 3.0;

AnomalyDetector detector(cfg);
auto anomalies = detector.detect(data_points);
for (const auto& a : anomalies) {
    // a.timestamp_ms, a.value, a.score, a.method
}
```

### Gap-Fill Interpolation

```cpp
#include "timeseries/gap_fill.h"

GapFiller filler(GapFillMethod::LinearInterpolation);
auto filled = filler.fill(sparse_points, from_ms, to_ms, interval_ms);
```

## Performance

| Metrik | Aktuell | Ziel |
|--------|---------|------|
| Write-Durchsatz (putBatch) | ~500k pts/s | ≥ 1 M pts/s |
| Gorilla-Kompression | 10–20× | ≥ 6× (mit Multi-Tier) |
| Gorilla-Decode-Throughput (SIMD) | >1,2 GB/s | >2 GB/s |
| Range-Scan (1M pts, float64) | ~300 ms | <50 ms |
| Continuous Agg Refresh-Latenz | ~5 s | <500 ms |
| Verschlüsselungs-Overhead (AES-NI) | N/A | <5% Write-Throughput |

## Troubleshooting

| Problem | Ursache / Lösung |
|---------|-----------------|
| Hohe Schreiblatenz | `backpressure_events` prüfen; `enable_adaptive_flush = true` setzen oder `max_memory_bytes` erhöhen |
| Punkte fehlen in Abfragen | TSAutoBuffer hat noch nicht geflusht; `buffer.flush()` aufrufen |
| Gorilla-Dekodierfehler | `GorillaDecoder::hasError()`: Daten korrupt oder unsupported Version |
| Continuous Agg veraltet | `getAggregateStatus()` → STALE; `refreshAggregate()` aufrufen |
| Out-of-Order-Punkte abgelehnt | `late_arrival_window_ms` erhöhen oder auf 0 setzen |
| Retention läuft nicht | `RetentionManager::startAsync()` wurde nicht aufgerufen |
| Prometheus-Write 400 | Header `Content-Encoding: snappy` und `Content-Type: application/x-protobuf` prüfen |

## Verwandte Dokumentation

| Dokument | Pfad |
|----------|------|
| Öffentliche API-Referenz | [`../../../include/timeseries/README.md`](../../../include/timeseries/README.md) |
| Implementierungs-README | [`../../../src/timeseries/README.md`](../../../src/timeseries/README.md) |
| Architektur-Guide | [`../../../src/timeseries/ARCHITECTURE.md`](../../../src/timeseries/ARCHITECTURE.md) |
| Roadmap | [`../../../src/timeseries/ROADMAP.md`](../../../src/timeseries/ROADMAP.md) |
| Future Enhancements | [`../../../src/timeseries/FUTURE_ENHANCEMENTS.md`](../../../src/timeseries/FUTURE_ENHANCEMENTS.md) |
| Changelog | [`../../../src/timeseries/CHANGELOG.md`](../../../src/timeseries/CHANGELOG.md) |
| Security | [`../../../src/timeseries/SECURITY.md`](../../../src/timeseries/SECURITY.md) |
| Speichermethoden | [`STORAGE_METHODS.md`](STORAGE_METHODS.md) |
| Auto-Buffer | [`AUTO_BUFFER.md`](AUTO_BUFFER.md) |
| Konfiguration | [`../../../docs/timeseries/CONFIG_GUIDE.md`](../../../docs/timeseries/CONFIG_GUIDE.md) |
| Englische Übersicht | [`../../../docs/timeseries/README.md`](../../../docs/timeseries/README.md) |
| Features: Time Series | [`../features/features_time_series.md`](../features/features_time_series.md) |

## Installation

Das Timeseries-Modul ist Teil von ThemisDB und wird als Bibliothek gebaut.
CMake-Konfiguration: `cmake --preset linux-release && cmake --build --preset linux-release`

Für den C++-Zugriff auf die öffentlichen Header:

```cmake
target_include_directories(your_target PRIVATE ${THEMISDB_INCLUDE_DIR})
```

Include-Pfad in Source-Dateien: `#include "timeseries/<header>.h"`
