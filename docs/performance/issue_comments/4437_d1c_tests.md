## 🔍 Tiefenanalyse-Update (2026-04-07): Falsches Benchmark-Ziel

**`TimeseriesBench/InsertTimepoints` ist ein No-Op** — ruft nie `TimeSeriesStore::put()` auf.

Der Benchmark-Body (`bench_core_performance.cpp:302–312`) macht nur:
```cpp
benchmark::DoNotOptimize(ts);
benchmark::DoNotOptimize(val);
```

**Korrektes Benchmark-Ziel für Regressionstests:**
- `bench_timeseries_ingestion/RawDataIngestion` (persistiertes Schreiben, 1 Thread) ≥ 500 k pts/s
- Für Batch-Tests: `bench_timeseries_ingestion/BatchIngestion` mit Batch-Size 1000

**Neuer Benchmark `bench_timeseries_adaptive_flush`** soll `TimeSeriesStore::putDataPoints()` oder `TSAutoBuffer::add()` direkt gegen persistierten RocksDB-I/O messen.

Alle anderen Akzeptanzkriterien (P99 < 100 µs, kein Datenverlust, >90% Coverage) bleiben unverändert.
