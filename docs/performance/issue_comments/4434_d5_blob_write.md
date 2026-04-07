## 🔍 Tiefenanalyse-Update (2026-04-07): `BatchWriteOptimizer` hat keine Batch-Logik

### Root Cause: `BatchWriteOptimizer` ist kein echter Optimizer

`src/storage/batch_write_optimizer.cpp` ist ausschließlich ein `WriteOptions`-Konfigurator:
- **Keine** interne Queue
- **Keine** Flush-Scheduling-Logik
- **Keine** `add(key, value)`-Methode
- `getOptimizedWriteOptions()` gibt nur ein `rocksdb::WriteOptions`-Objekt zurück

Das erklärt direkt das Benchmark-Ergebnis aus `PERFORMANCE_EXPECTATIONS.md §36.18`:
> **Batch-API ist 4× langsamer als Single-Inserts** in Items/s — weil der "Batch-Koordinator" Overhead hinzufügt, ohne tatsächlich zu batchen.

### Bereits implementierte Infrastruktur (noch nicht auf dem Hot Path verdrahtet)

| Komponente | Datei | Status |
|-----------|-------|--------|
| `ZeroCopyBlobTransfer` (sendfile, mmap, S3 Multipart) | `src/storage/zero_copy_blob_transfer.cpp` (568 Zeilen) | ✅ Implementiert, nicht verdrahtet |
| `commitBatch()` — direkt, MVCC-frei (~500k ops/s) | `src/storage/rocksdb_wrapper.cpp:1284` | ✅ Verfügbar |
| BlobDB (`enable_blob_files=true`, `min_blob_size=1024`) | `src/storage/rocksdb_wrapper.cpp:463–468` | ✅ Konfiguriert |

### Konkreter Fix-Scope

`BatchWriteOptimizer` zu einem echten asynchronen Write-Buffer umbauen (analog zu `TSAutoBuffer`):

1. Interne Queue + Background-Flush-Thread
2. `add(key, value)` API + `flush()` / `getStats()` Methoden
3. `WriteBatch`-basierter Commit via `commitBatch()` — **nicht** per-write MVCC-Transaktion
4. Für 1MB-Blobs: Chunk-Splitting (64/128 KB) + parallele NVMe-Writes via `ZeroCopyBlobTransfer`

Die `ZeroCopyBlobTransfer`-Klasse muss nur in den Write-Path von `BatchWriteOptimizer` eingebunden werden — sie ist fertig implementiert.
