## 🔍 Tiefenanalyse-Update (2026-04-07): Diese Klasse existiert bereits

`AdaptiveFlushController` = `TSAutoBuffer` in `src/timeseries/ts_auto_buffer.cpp` (611 Zeilen).

**Was bereits implementiert ist:**
- ✅ Konfigurierbarer Buffer (`max_points_per_buffer`, `max_memory_bytes`, `max_total_points`)
- ✅ Async Flush Worker (`flush_thread_`, `start()` / `stop()`)
- ✅ Watermark + Timeout-Trigger (`shouldFlushBuffer()` + `shouldFlushGlobal()`)
- ✅ `FlushController` mit EWMA-basiertem Adaptive-Batching (`ewma_latency_ms`, `current_batch_size`)
- ✅ Backpressure (`backpressure_high/low_watermark`, `backpressure_cv_.wait_for()`, `isBackpressured()`)
- ✅ Overdue-Flush-Detection (`overdue_flush_multiplier`, `metrics->recordOverdueFlushed()`)
- ✅ Vollständige Stats (`buffer_size`, `flush_count`, `ewma_latency`, `backpressure_events`, `dedup_dropped_count`)

**Was dieses Issue beschreibt, ist vollständig in `TSAutoBuffer` implementiert.**

### Empfehlung: Issue schließen (duplicate / already implemented)

Das tatsächlich fehlende Stück ist die **Anbindung von `TimeSeriesStore`** (ältere Klasse in `timeseries.cpp`) an `TSAutoBuffer` — das ist Scope von #4439.
