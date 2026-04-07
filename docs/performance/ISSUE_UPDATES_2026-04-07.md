# Performance-Issue Update-Vorschläge — 2026-04-07

> Ergebnis der Tiefenanalyse vom 2026-04-07. Jeder Abschnitt enthält den exakten
> Kommentartext, der per `gh issue comment` auf dem entsprechenden GitHub-Issue
> gepostet werden soll.  
> Lokale Ausführung: `gh issue comment <NUM> --repo makr-code/ThemisDB --body-file <datei>`

---

## #4432 — [PERF-D1] Timeseries Write Throughput

**Fehlerhafte Annahmen im aktuellen Issue:**
- Das Issue referenziert `TimeseriesBench/InsertTimepoints` (49 M/s) als Messung.
  Dieser Benchmark ist ein **No-Op** (misst nur `DoNotOptimize(ts); DoNotOptimize(val)`).
- Die beschriebene Ursache ("Async flush recommendations not fully utilized") ist nur ein Teil
  des Problems — die eigentliche Quelle des ~200 k pts/s-Limits liegt tiefer.

**Exakter Kommentartext für #4432:**

```
## 🔍 Tiefenanalyse-Update (2026-04-07)

### Benchmark-Artefakt: InsertTimepoints ist No-Op

`TimeseriesBench/InsertTimepoints` (49 M/s in bench_core_performance.cpp:302–312) misst **kein** persistiertes Schreiben:

```cpp
// bench_core_performance.cpp:221–229
BENCHMARK_F(TimeseriesBench, InsertTimepoints)(benchmark::State& state) {
    for (auto _ : state) {
        int64_t ts = std::chrono::system_clock::now().time_since_epoch().count();
        double val = 50.0 + (i % 20);
        benchmark::DoNotOptimize(ts);   // ← kein TSStore-Aufruf!
        benchmark::DoNotOptimize(val);
    }
}
```

Der echte ~200 k pts/s-Wert stammt aus dem HTTP-API-Benchmark (§38.1 in PERFORMANCE_EXPECTATIONS.md).

### Tatsächliche Bottleneck-Kette in `TimeSeriesStore::put()` (timeseries.cpp:87–108)

Der echte Hot Path bei persistierten Writes ist:

1. **`makeKey()` via `ostringstream` + `setw(20)`** (~100 ns/Aufruf, vs. ~10 ns für `snprintf`)
2. **`nlohmann::json::dump()`** pro Punkt (~300–500 ns für `{timestamp_ms, value}`)
3. **Kein `WriteBatch`** — jeder Punkt ist ein separater `rocksdb::TransactionDB::Put()`-Aufruf

### TSAutoBuffer existiert bereits — ist aber nicht verdrahtet

`TSAutoBuffer` (src/timeseries/ts_auto_buffer.cpp, 611 Zeilen) löst genau dieses Problem für die `TSStore`-API (neu). Die ältere `TimeSeriesStore`-Klasse (src/timeseries/timeseries.cpp) ist davon **nicht** angebunden — sie hat keinen `TSAutoBuffer`.

Der `bench_timeseries_ingestion.cpp`-Benchmark (`RawDataIngestion`) nutzt `TimeSeriesStore` direkt:
```cpp
// bench_timeseries_ingestion.cpp:63
ts_store_ = std::make_unique<TimeSeriesStore>(db_->getRawDB(), nullptr);
```

### Konkrete Fix-Richtung

Die Sub-Issues #4438 und #4439 beschreiben eine `AdaptiveFlushController`-Neuimplementierung — diese Klasse **existiert bereits** als `TSAutoBuffer` (inkl. FlushController, EWMA-Backpressure, Adaptive-Batch-Size, Overdue-Flush-Detection).

**Was tatsächlich fehlt:**
- `TimeSeriesStore::put()` / `putDataPoints()` soll intern `WriteBatch` nutzen (analog zu `commitBatch()` in rocksdb_wrapper.cpp:1284)
- `makeKey()` auf `snprintf` oder `fmt::format` umstellen
- `nlohmann::json::dump()` durch binäre Serialisierung oder kompaktes String-Format ersetzen
- `TimeSeriesStore` optional an `TSAutoBuffer` anbinden (wie `TSStore` es tut)

### Korrektes Akzeptanzkriterium

- `bench_timeseries_ingestion/RawDataIngestion` (1 Thread) ≥ 500 k pts/s — **nicht** `InsertTimepoints`
- Alle geschriebenen Punkte sind bei `query()` zurücklesbar
- P99 Insert-Latenz ≤ 2 µs
```

---

## #4438 — [PERF-D1-A] AdaptiveFlushController: Buffer & Async Flush Implementation

**Kernbefund:** `AdaptiveFlushController` existiert bereits als `TSAutoBuffer` in
`src/timeseries/ts_auto_buffer.cpp` (611 Zeilen, inkl. FlushController, EWMA,
Adaptive-Batch-Größe, Backpressure, Overdue-Flush-Detection).

**Exakter Kommentartext für #4438:**

```
## 🔍 Analyse-Update: Klasse existiert bereits

`AdaptiveFlushController` / `TSAutoBuffer` ist in `src/timeseries/ts_auto_buffer.cpp` (611 Zeilen) und `include/timeseries/ts_auto_buffer.h` bereits vollständig implementiert:

- ✅ Konfigurierbarer Buffer (max_points_per_buffer, max_memory_bytes)
- ✅ Async Flush Worker (flush_thread_, start()/stop())
- ✅ Watermark + Timeout-Trigger (shouldFlushBuffer + shouldFlushGlobal)
- ✅ FlushController EWMA-basiertes Adaptive-Batching
- ✅ Backpressure (high/low watermark, backpressure_cv_.wait_for)
- ✅ Overdue-Flush-Detection (overdue_flush_multiplier, metrics->recordOverdueFlushed)
- ✅ Stats (buffer_size, flush_count, ewma_latency, backpressure_events, dedup_dropped_count)

**Was dieses Issue **nicht** lösen muss:** Neuimplementierung der Klasse.

**Was stattdessen zu tun ist (#4439):** Die bestehende `TSAutoBuffer`/`TSStore`-Kette mit
`TimeSeriesStore` (timeseries.cpp) verdrahten und den `makeKey()`/JSON-Overhead reduzieren.

→ Dieses Issue kann als **obsolete / duplicate of #4439** geschlossen werden.
```

---

## #4439 — [PERF-D1-B] Integration: AdaptiveFlushController in TSStore Write Path

**Exakter Kommentartext für #4439:**

```
## 🔍 Analyse-Update: Verdrahtung existiert für TSStore — fehlt für TimeSeriesStore

**Aktueller Stand:**
- `TSStore` (include/timeseries/tsstore.h) ist bereits an `TSAutoBuffer` angebunden
- `TimeSeriesStore` (src/timeseries/timeseries.cpp) — die ältere Klasse, die der HTTP-Layer
  und `bench_timeseries_ingestion` nutzt — verwendet **keinen** Buffer

**Engpass in `TimeSeriesStore::put()` (timeseries.cpp:87–108):**
```cpp
std::string key = makeKey(metric, entity, point.timestamp_ms);  // ostringstream + setw(20)
nlohmann::json value_json = point.toJson();
std::string value_str = value_json.dump();                        // JSON-Serialisierung
rocksdb::Status s = db_->Put(write_opts, cf_, key, value_str);   // kein WriteBatch!
```

**Konkreter Fix-Scope für dieses Issue:**
1. `makeKey()` von `ostringstream` auf `fmt::format` oder `snprintf` umstellen
2. `putDataPoints(batch)` soll `rocksdb::WriteBatch` nutzen (wie `commitBatch()` in rocksdb_wrapper.cpp:1284)
3. Optional: `TimeSeriesStore` bekommt einen `set_auto_buffer(TSAutoBuffer*)` Setter für den gebufferten Modus
4. `bench_timeseries_ingestion/RawDataIngestion` als Benchmark-Ziel verwenden (nicht `InsertTimepoints`)
```

---

## #4437 — [PERF-D1-C] Tests: Regression & Performance Validation for Adaptive Flush

**Exakter Kommentartext für #4437:**

```
## 🔍 Analyse-Update: Benchmark-Ziel korrigieren

Das Issue referenziert als Ziel "≥500k pts/sec throughput single-threaded". Korrekt —
aber der Benchmark **muss** `bench_timeseries_ingestion/RawDataIngestion` verwenden,
**nicht** `TimeseriesBench/InsertTimepoints`.

`InsertTimepoints` in `bench_core_performance.cpp` ist ein No-Op (ruft nie `TimeSeriesStore::put()` auf)
und misst nur Benchmark-Framework-Overhead (~49 M/s). Ein neuer `bench_timeseries_adaptive_flush`
soll `TimeSeriesStore::putDataPoints()` oder `TSAutoBuffer::add()` direkt messen.

Akzeptanzkriterium bleibt unverändert: ≥500k pts/s, P99 < 100µs — aber gegen die echte
persistierte I/O, nicht gegen einen No-Op.
```

---

## #4435 — [PERF-D2] Gorilla Decode SIMD

**Kritischer Fehler im Issue:** "Scalar loop in Gorilla decode — No explicit AVX2/AVX-512"
→ Dies ist **falsch**. Der AVX2-SIMD-Pfad existiert bereits.

**Exakter Kommentartext für #4435:**

```
## 🔍 Tiefenanalyse-Update: Falscher Root Cause — SIMD existiert bereits

### Was das Issue falsch beschreibt

Das Issue behauptet "Scalar loop in Gorilla decode, no explicit AVX2/AVX-512".
Das ist **unzutreffend**. In `src/timeseries/gorilla_simd.cpp` existieren bereits:

- **`prefix_sum_i64()`** (Z. 164–236): vollständiger AVX2-Pfad für Timestamp-Rekonstruktion + NEON-Fallback
- **`prefix_xor_u64()`** (Z. 247–300): vollständiger AVX2-Pfad für Double-Bit-Pattern-Rekonstruktion + NEON-Fallback
- **`GorillaSIMDDecoder::decodeAll()`** (Z. 307–384): 2-Phasen-Architektur (Phase 1: scalar parse → Phase 2: SIMD bulk)

### Tatsächlicher Bottleneck: Phase 1 (`parse_gorilla_chunk`)

Die SIMD-Phase (Phase 2) ist korrekt. Der Engpass liegt in Phase 1 (`parse_gorilla_chunk`, Z. 102–153):

**Problem 1: `BitReader::readBits(N)` — Bit-für-Bit-Schleife**
```cpp
uint64_t BitReader::readBits(int bits) {
    uint64_t v = 0;
    for (int i = 0; i < bits; ++i) {       // 64 Iterationen für readBits(64)
        if (readBit()) v |= (1ULL << i);   // pro Bit: Bounds-Check + Bit-Extract
    }
    return v;
}
```
Für `readBits(64)` sind das 64 Schleifeniterationen. Fix: Byte-orientierter BitReader
(liest 8 Bit auf einmal via direktem `buf_[byte_idx++]`).

**Problem 2: `readVarUInt()` ruft `readBits(8)` statt direktem Byte-Read auf**
Jedes LEB128-Byte kostet 8 Bit-Iterationen statt eines `uint8_t`-Reads.

**Problem 3: Kein `reserve()` auf `dods`/`xorvals` (Z. 98–99)**
```cpp
struct GorillaParsed {
    std::vector<int64_t>  dods;    // kein reserve() vor der while-Schleife
    std::vector<uint64_t> xorvals; // kein reserve() vor der while-Schleife
};
```
Für einen 128-Punkte-Chunk: ~7 Reallokationen (2→4→8→16→32→64→128).

**Problem 4: Unnötige `payload`-Kopie (Z. 330)**
```cpp
const std::vector<uint8_t> payload(payload_ptr, payload_ptr + payload_size);
```
Kopiert die gesamten Chunk-Daten nur um den 3-Byte-Header zu überspringen.
Fix: `BitReader` mit `payload_ptr`-Offset direkt konstruieren.

### Korrigiertes Akzeptanzkriterium

- ✅ Phase 2 (AVX2 prefix-sum/XOR) ist korrekt — kein Änderungsbedarf
- 🔧 Phase 1: Byte-orientierter `BitReader` (kein Bit-für-Bit-Loop)
- 🔧 `reserve(expected_points)` vor der `push_back`-Schleife
- 🔧 Header-Strip via Offset statt Kopie
- Ziel: >2 GB/s bleibt bestehen; erreichbar durch Phase-1-Optimierung allein
```

---

## #4433 — [PERF-D4] Distributed 2PC

**Exakter Kommentartext für #4433:**

```
## 🔍 Tiefenanalyse-Update: Vollständige Bestätigung + zusätzliche Befunde

Die im Issue beschriebene Ursache ist korrekt. Ergänzungen aus der Code-Analyse:

### Bestätigter Code-Beweis (two_phase_commit_coordinator.cpp)

**Phase 1 unter globalem Lock (Z. 187–197):**
```cpp
std::lock_guard<std::mutex> lock(mutex_);
auto& stored = transactions_.at(transaction_id);
stored.state = CoordinatorTxnState::PREPARING;
all_prepared = runPhase1(stored);  // ← alle shard.onPrepare() SERIELL aufgerufen
```

**Phase 2 unter demselben globalen Lock (Z. 218–222):**
```cpp
std::lock_guard<std::mutex> lock(mutex_);
auto& stored = transactions_.at(transaction_id);
runPhase2(stored, all_prepared);   // ← alle shard.onCommit() SERIELL aufgerufen
```

→ Beide Phasen sind unter `mutex_` serialisiert: Es ist immer **genau eine** Transaktion gleichzeitig im System.

### WAL-Overhead: 4 Writes pro Transaktion

`logToWAL()` wird 4× aufgerufen: BEGIN_TX, COMMIT/ABORT_TX (nach Phase 1), COMMIT/ABORT_TX (nach Phase 2), plus `wal_->flush()` bei `config_.sync_wal_writes=true`.
Jeder `flush()` ist potenziell ein fsync → ~46 ms pro Transaktion = ~21 Transaktionen/s.

### `transactions_`-Map ohne Eviction

Die `transactions_`-Map (std::map in der Impl) hat keinen Eviction-Mechanismus für abgeschlossene Transaktionen. Bei hohem Durchsatz (Tage/Wochen Betrieb) wächst sie unbegrenzt → Lock-Contention steigt.

### Richtiger Source-Pfad

Issue referenziert `src/transaction` — korrekte Datei ist:
`src/sharding/two_phase_commit_coordinator.cpp`

### Empfohlener Fix-Ansatz

1. **Shard-Fan-Out aus dem Lock herausnehmen:** Nur den State-Update unter Lock halten; `runPhase1()` / `runPhase2()` nach Lock-Release mit `std::async` parallelisieren
2. **Phase 1 → Phase 2 überlappen** (Pipelining): Decision nach Phase 1 persistent machen und Phase 2 asynchron starten
3. **WAL Group-Commit:** Mehrere Txn-WAL-Einträge in einem fsync bündeln
4. **Eviction:** Abgeschlossene Transaktionen aus `transactions_` nach Timeout entfernen
```

---

## #4434 — [PERF-D5] 1MB Blob Storage

**Kritischer Befund:** `BatchWriteOptimizer` enthält **keine** Batch-Logik.

**Exakter Kommentartext für #4434:**

```
## 🔍 Tiefenanalyse-Update: BatchWriteOptimizer ist kein echter Optimizer

### Root Cause: BatchWriteOptimizer hat keine Batch-Logik

`src/storage/batch_write_optimizer.cpp` (161 Zeilen) ist ausschließlich ein `WriteOptions`-Konfigurator:
- Keine interne Queue, kein Flush-Scheduling, keine `add(key, value)`-Methode
- `getOptimizedWriteOptions()` gibt nur ein `rocksdb::WriteOptions`-Objekt zurück
- Das erklärt direkt das Benchmark-Ergebnis aus §36.18: **Batch-API ist 4× langsamer als Single-Inserts**,
  weil der "Batch-Koordinator" zusätzlichen Overhead hinzufügt, ohne zu batchen.

### Bereits implementierte Infrastruktur (nicht auf Hot Path verdrahtet)

1. **`zero_copy_blob_transfer.cpp`** (568 Zeilen): sendfile(2), mmap, S3 Multipart-Upload —
   vollständig implementiert, aber nicht auf dem Write-Hot-Path
2. **`commitBatch()` in rocksdb_wrapper.cpp:1284**: Direkt `db_->Write(batch)` ohne MVCC-Overhead —
   dieser Pfad erreicht 500k+ ops/s (§36.6)
3. **RocksDB BlobDB** ist bereits konfiguriert: `enable_blob_files=true`, `min_blob_size=1024`,
   `enable_blob_garbage_collection=true`

### Konkreter Fix-Scope

`BatchWriteOptimizer` muss zu einem echten asynchronen Write-Buffer umgebaut werden:
- Interne Queue + Background-Flush-Thread (analog zu `TSAutoBuffer`)
- `add(key, value)` API statt nur `getOptimizedWriteOptions()`
- `WriteBatch`-basierter Commit via `commitBatch()` (nicht per-write MVCC-Transaktion)
- Für 1MB-Blobs: Chunk-Splitting (64/128 KB) + paralleles NVMe-I/O via `zero_copy_blob_transfer`

### Source-Dateien

- `src/storage/batch_write_optimizer.cpp` (eigentlich leer bzgl. Batching)
- `src/storage/zero_copy_blob_transfer.cpp` (bereit für Integration)
- `src/storage/rocksdb_wrapper.cpp:1284` (commitBatch — schneller Pfad)
```

---

## #4431 — [PERF-D3] Vector Insert

**Exakter Kommentartext für #4431:**

```
## 🔍 Tiefenanalyse-Update: Regression ist Benchmark-Artefakt

### Root Cause: addEntity() vs. addBatch() — unterschiedliche Write-Pfade

```cpp
// addEntity() → rocksdb_wrapper.cpp:746–772
auto txn = beginTransaction();  // neue MVCC-Transaktion
txn->put(key, value);
txn->commit();                  // WAL-Write + fsync pro Insert

// addBatch() → rocksdb_wrapper.cpp:1284–1293
db_->Write(*write_options_, batch);  // direkt, kein MVCC-Overhead
```

`addEntity()` öffnet eine vollständige RocksDB-MVCC-Transaktion **pro Insert**.
`addBatch()` nutzt `WriteBatch` direkt und ist signifikant schneller.

### Regression ist Benchmark-Infrastruktur-Artefakt

Die v1.3.0→v1.3.4 Regression (566k→351k/s) wurde durch per-test temp dirs und
Einzel-Transaktionen pro `put()` verursacht — **keine echte Produktions-Regression**.
Die Produktionsperformance (via `addBatch()`) liegt deutlich über 351k/s.

### Zusätzlicher Overhead in addEntity(): Konfigurations-Read pro Insert

In `src/index/vector_index.cpp:974–983` wird bei jedem `addEntity()`-Aufruf
ein `db_.get("config:vector")` ausgeführt (O(1) RocksDB-Get, ~1–3 µs, aber kumulativ relevant).

### Richtiger Source-Pfad

Issue referenziert `src/acceleration/vec_knn.cpp` — die relevante Datei ist:
`src/index/vector_index.cpp`

### Fix-Richtung

1. **Benchmark-Fix**: `bench_core_performance.cpp` soll `addBatch()` statt per-`addEntity()` nutzen
2. **Produktions-Fix**: `addEntity()` soll intern einen Write-Buffer akkumulieren und via `WriteBatch` flushen
3. Konfigurations-Read cachen (einmal pro Session, nicht pro Insert)
```

---

## #4436 — [PERF-D7] Query Engine vs. ClickHouse

**Kritischster Befund: Das D7-Gap ist ein Benchmark-Artefakt, KEIN echtes Performance-Problem.**

**Exakter Kommentartext für #4436:**

```
## 🔍 Tiefenanalyse-Update: KRITISCH — Benchmark ist No-Op, kein echtes Gap

### SimpleEvaluation misst die Query Engine NICHT

```cpp
// bench_core_performance.cpp:221–229
BENCHMARK_F(QueryEngineBench, SimpleEvaluation)(benchmark::State& state) {
    for (auto _ : state) {
        double val = 42.0;              // ← kein QueryEngine-Aufruf!
        benchmark::DoNotOptimize(val);
    }
    state.SetItemsProcessed(state.iterations());
}
```

`QueryEngineBench/SimpleEvaluation` ruft `QueryEngine::executeAql()` oder ähnliches **nie auf**.
Die 814.5 M items/s messen den Overhead des Benchmark-Frameworks (leere Iteration + DoNotOptimize).

### Folgerungen

1. Der Vergleich "814.5 M/s vs. 1.200 M/s ClickHouse" ist **bedeutungslos** — beide messen unterschiedliche Dinge
2. Die v1.3.0→v1.3.4 Regression (−16%, 968.6→814.5 M/s) ist eine Regression im **Benchmark-Overhead**, nicht in der Query-Performance
3. Der Code-Snippet im Issue (`src/query/executor.cpp`) ist fiktiv — diese Datei und dieser Code existieren nicht

### Echte Query-Performance

`PERFORMANCE_EXPECTATIONS.md §2`:
- `Simple AQL WHERE`: 3.43 M ops/s @ ~0.3 µs ✅ (Ziel: 10.000 Queries/s)
- `Complex WHERE`: 3.35 M ops/s ✅
- `JOIN (Users-Posts)`: 10.2 M ops/s ✅

**Es gibt kein echtes Performance-Gap bei der Query Engine.**

### QueryCompiler JIT ist eine Lambda-Closure, kein nativer Code

`src/query/query_compiler.cpp:308–375`: Die "JIT-Compilation" baut eine `std::function`-Closure,
die den Interpreter mit direktem Capture aufruft. LLVM-MCJIT ist im Code als `THEMIS_HAS_LLVM_JIT`
Extension-Point kommentiert — **nicht implementiert**.

### Empfohlene Maßnahmen

1. **Issue schließen** (kein echtes Gap) ODER umformulieren:
   > "Ersetze `SimpleEvaluation` No-Op-Benchmark durch echten Query-Engine-Benchmark"
2. Neuer Benchmark: `VectorizedExecutionEngine::execute()` auf vorbereiteten `ColumnBatch`-Daten
   gegen ClickHouse-kompatiblen Columnar-Filter messen
3. Für P-8 (§37 Maßnahmenplan): Columnar SIMD Aggregation in `ColumnarExecutionEngine`
   (`src/analytics/columnar_engine.cpp`) optimieren — das ist der echte Kandidat für ClickHouse-Parität
```

---

## Zusammenfassung: Issue-Korrektheit nach Tiefenanalyse

| Issue | Titel (kurz) | Root Cause korrekt? | Empfehlung |
|-------|-------------|---------------------|------------|
| #4432 | PERF-D1 Timeseries Write | ⚠️ Teilweise | Benchmark-Artefakt + echter Bottleneck ergänzen |
| #4438 | PERF-D1-A AdaptiveFlush | ❌ Obsolet | AdaptiveFlush = TSAutoBuffer existiert bereits → schließen |
| #4439 | PERF-D1-B Integration | ✅ Richtig | Scope auf TimeSeriesStore fokussieren |
| #4437 | PERF-D1-C Tests | ⚠️ Falsch Benchmark | InsertTimepoints → RawDataIngestion |
| #4435 | PERF-D2 Gorilla Decode | ❌ Falsch | SIMD existiert; Bottleneck ist Phase-1 BitReader |
| #4433 | PERF-D4 2PC | ✅ Korrekt | Quelldatei-Pfad korrigieren + unbounded Map ergänzen |
| #4434 | PERF-D5 Blob Write | ❌ Unvollständig | BatchWriteOptimizer ist kein Optimizer → Kernproblem |
| #4431 | PERF-D3 Vector Insert | ⚠️ Unvollständig | Regression ist Artefakt; MVCC-Overhead benennen |
| #4436 | PERF-D7 Query Engine | ❌ Falsch | Kein echtes Gap; Benchmark ist No-Op → schließen |

---

## gh CLI Befehle zum Posten der Kommentare

```bash
# Voraussetzung: gh auth login
# Dann für jedes Issue die entsprechende Datei erstellen und posten:

gh issue comment 4432 --repo makr-code/ThemisDB --body "$(cat /pfad/zum/kommentar_4432.md)"
gh issue comment 4435 --repo makr-code/ThemisDB --body "$(cat /pfad/zum/kommentar_4435.md)"
gh issue comment 4433 --repo makr-code/ThemisDB --body "$(cat /pfad/zum/kommentar_4433.md)"
gh issue comment 4434 --repo makr-code/ThemisDB --body "$(cat /pfad/zum/kommentar_4434.md)"
gh issue comment 4431 --repo makr-code/ThemisDB --body "$(cat /pfad/zum/kommentar_4431.md)"
gh issue comment 4436 --repo makr-code/ThemisDB --body "$(cat /pfad/zum/kommentar_4436.md)"
gh issue comment 4438 --repo makr-code/ThemisDB --body "$(cat /pfad/zum/kommentar_4438.md)"
gh issue comment 4439 --repo makr-code/ThemisDB --body "$(cat /pfad/zum/kommentar_4439.md)"
gh issue comment 4437 --repo makr-code/ThemisDB --body "$(cat /pfad/zum/kommentar_4437.md)"

# Für D7 Issue schließen (kein echtes Gap):
gh issue close 4436 --repo makr-code/ThemisDB --comment "No real performance gap. See analysis comment above."

# Für D1-A Issue schließen (Klasse existiert bereits):
gh issue close 4438 --repo makr-code/ThemisDB --comment "TSAutoBuffer (ts_auto_buffer.cpp) bereits implementiert. See analysis comment above."
```
