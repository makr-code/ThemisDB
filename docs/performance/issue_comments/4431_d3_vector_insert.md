## 🔍 Tiefenanalyse-Update (2026-04-07): Regression ist Benchmark-Artefakt

### Root Cause: `addEntity()` vs. `addBatch()` — unterschiedliche Write-Pfade

```cpp
// addEntity() → rocksdb_wrapper.cpp:746–772
auto txn = beginTransaction();  // neue MVCC-Transaktion pro Insert
txn->put(key, value);
txn->commit();                  // WAL-Write pro Insert

// addBatch() → rocksdb_wrapper.cpp:1284–1293
db_->Write(*write_options_, batch);  // direkt, kein MVCC-Overhead
```

`addEntity()` öffnet eine vollständige RocksDB-MVCC-Transaktion **pro Insert**. `addBatch()` nutzt `WriteBatch` direkt und ist signifikant schneller.

### Die v1.3.0→v1.3.4 Regression (566k→351k/s) ist ein Benchmark-Artefakt

`PERFORMANCE_EXPECTATIONS.md §36.1` vermerkt explizit:
> *"Mehrere Regressionen (insb. SecondaryIndex, VectorIndex, Graph) sind auf geänderte Test-Infrastruktur zurückzuführen (per-test temp dirs, einzelne RocksDB-Transaktionen pro `put()`), nicht auf Produktions-Regressions"*

Der Benchmark nutzt `addEntity()` pro Insert → jeder Insert öffnet eine separate MVCC-Transaktion. Die tatsächliche Produktionsperformance via `addBatch()` liegt deutlich über 351k/s.

### Zusätzlicher Overhead: Konfigurations-Read pro Insert

In `src/index/vector_index.cpp:974–983` wird bei jedem `addEntity()`-Aufruf ein `db_.get("config:vector")` ausgeführt (~1–3 µs pro Insert, O(1) RocksDB-Get, aber kumulativ relevant).

### Source-Dateien (Korrektur)

Issue referenziert `src/acceleration/vec_knn.cpp` → korrekte Datei:
**`src/index/vector_index.cpp`**

### Fix-Richtung

1. **Benchmark-Fix:** `bench_core_performance.cpp` soll `addBatch()` statt per-`addEntity()` nutzen
2. **Produktions-Fix:** `addEntity()` soll intern einen Write-Buffer akkumulieren und via `WriteBatch` flushen (wie verschlüsselte Vektoren es via `encBatch_` bereits tun)
3. Konfigurations-Read in `addEntity()` cachen (einmal beim Konstruktor/ersten Aufruf)
