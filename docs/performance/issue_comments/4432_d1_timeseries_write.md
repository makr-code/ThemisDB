## 🔍 Tiefenanalyse-Update (2026-04-07)

### Benchmark-Artefakt: `InsertTimepoints` ist No-Op

`TimeseriesBench/InsertTimepoints` (49 M/s, `bench_core_performance.cpp:302–312`) misst **kein** persistiertes Schreiben:

```cpp
BENCHMARK_F(TimeseriesBench, InsertTimepoints)(benchmark::State& state) {
    for (auto _ : state) {
        int64_t ts = std::chrono::system_clock::now().time_since_epoch().count();
        double val = 50.0 + (i % 20);
        benchmark::DoNotOptimize(ts);   // ← kein TSStore-Aufruf!
        benchmark::DoNotOptimize(val);
    }
}
```

Der echte ~200 k pts/s-Wert stammt aus dem HTTP-API-Benchmark (§38.1, PERFORMANCE_EXPECTATIONS.md).

### Tatsächliche Bottleneck-Kette in `TimeSeriesStore::put()` (timeseries.cpp:87–108)

1. **`makeKey()` via `ostringstream` + `setw(20)`** — ~100 ns/Aufruf (vs. ~10 ns für `snprintf`/`fmt::format`)
2. **`nlohmann::json::dump()`** pro Punkt — ~300–500 ns für `{timestamp_ms, value}`
3. **Kein `WriteBatch`** — jeder Punkt ist ein separater `rocksdb::TransactionDB::Put()`-Aufruf

### `TSAutoBuffer` existiert bereits — aber für die falsche Klasse

`TSAutoBuffer` (`src/timeseries/ts_auto_buffer.cpp`, 611 Zeilen) löst dieses Problem für `TSStore` (neuere API). Die ältere `TimeSeriesStore`-Klasse (`src/timeseries/timeseries.cpp`) ist davon **nicht** angebunden. Der `bench_timeseries_ingestion`-Benchmark nutzt `TimeSeriesStore` direkt:

```cpp
// bench_timeseries_ingestion.cpp:63
ts_store_ = std::make_unique<TimeSeriesStore>(db_->getRawDB(), nullptr);
```

### Fix-Richtung

Die Sub-Issues #4438/#4439 beschreiben eine Neuimplementierung des `AdaptiveFlushController` — diese Funktionalität **existiert bereits vollständig** als `TSAutoBuffer`. Was fehlt:

- `TimeSeriesStore::put()` / `putDataPoints()` auf `WriteBatch` umstellen (`commitBatch()` in `rocksdb_wrapper.cpp:1284`)
- `makeKey()` auf `fmt::format` oder `snprintf` umstellen
- `nlohmann::json::dump()` durch kompakteres Format ersetzen
- Optional: `TimeSeriesStore` an `TSAutoBuffer` anbinden (Setter-Injection)

### Korrektes Akzeptanzkriterium

- **Benchmark:** `bench_timeseries_ingestion/RawDataIngestion` ≥ 500 k pts/s (1 Thread) — **nicht** `InsertTimepoints`
- Alle geschriebenen Punkte bei `query()` zurücklesbar
- P99 Insert-Latenz ≤ 2 µs
