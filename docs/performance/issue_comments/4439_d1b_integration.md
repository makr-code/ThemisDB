## 🔍 Tiefenanalyse-Update (2026-04-07): Scope auf `TimeSeriesStore` fokussieren

**Richtige Diagnose:** `TSStore` ist bereits an `TSAutoBuffer` angebunden. Das Problem liegt bei der **älteren `TimeSeriesStore`-Klasse** (`src/timeseries/timeseries.cpp`), die der HTTP-Layer und `bench_timeseries_ingestion` verwenden.

**Engpass in `TimeSeriesStore::put()` (timeseries.cpp:87–108):**
```cpp
std::string key = makeKey(metric, entity, point.timestamp_ms);  // ostringstream + setw(20) — ~100ns
nlohmann::json value_json = point.toJson();
std::string value_str = value_json.dump();                        // JSON-Serialisierung — ~300-500ns
rocksdb::Status s = db_->Put(write_opts, cf_, key, value_str);   // kein WriteBatch!
```

**Konkreter Fix-Scope für dieses Issue:**
1. `makeKey()` von `ostringstream` auf `fmt::format` oder `snprintf` umstellen
2. `putDataPoints(batch)` soll `rocksdb::WriteBatch` nutzen (wie `commitBatch()` in `rocksdb_wrapper.cpp:1284`)
3. Optional: `TimeSeriesStore` bekommt einen `set_auto_buffer(TSAutoBuffer*)` Setter

**Benchmark-Ziel:** `bench_timeseries_ingestion/RawDataIngestion` ≥ 500 k pts/s  
(nicht `TimeseriesBench/InsertTimepoints` — das ist ein No-Op-Benchmark)
