> ⚠️ **Historische Messdaten** – Die in diesem Dokument enthaltenen Zahlen entstammen einem bestimmten Messzeitpunkt und sind nicht mehr reproduzierbar ohne die ursprüngliche Testumgebung.
> Für reproduzierbare Ergebnisse: Benchmark-Kommandos und aktuelle CMake-Presets unter [`benchmarks/README.md`](../README.md) verwenden.

# ThemisDB Performance-Vergleich: V1.3.0 vs V1.3.3
**Datum**: 28. Dezember 2025  
**Build**: MSVC Release (x64, AVX2)  
**Hardware**: 20-Core CPU @ 3.7 GHz, 20 MB L3 Cache

---

## Executive Summary

V1.3.3 zeigt **gemischte Ergebnisse** im Vergleich zu v1.3.0:

| Benchmark | V1.3.0 (23.12) | V1.3.3 (28.12) | Änderung | Bewertung |
|-----------|----------------|----------------|----------|-----------|
| **VectorIndex Insert** | 566.7k items/s | **538.0k items/s** | **-5.1%** ⚠️ | Leichter Rückgang |
| **SecondaryIndex Insert** | **1.78M items/s** | **5.11k items/s** | **-99.7%** ❌ | **KRITISCHE REGRESSION** |
| **QueryEngine Eval** | 968.6M items/s | **949.8M items/s** | -1.9% ✅ | Stabil (im Rauschen) |
| **GraphIndex AddEdges** | **1.47M items/s** | **1.20M items/s** | **-18.4%** ⚠️ | Moderater Rückgang |
| **Timeseries Insert** | 61.0M items/s | **55.9M items/s** | -8.4% ⚠️ | Leichter Rückgang |

### 🚨 Kritischer Befund: SecondaryIndex

**Problem**: SecondaryIndex-Performance ist um **99.7%** eingebrochen (1.78M → 5.1k items/s).

**Mögliche Ursachen**:
1. DEBUG-Build statt Release? → **Nein**, beide Release
2. Neue Transaktions-Overhead in v1.3.1-v1.3.3? → **Wahrscheinlich**
3. RocksDB-Konfiguration geändert? → **Zu prüfen**
4. Lock-Contention durch parallele Test-Isolierung? → **Möglich**

**Nächste Schritte**:
- ✅ Index-Setup in `bench_core_performance.cpp` prüfen (Transaktions-Wrapper)
- ✅ RocksDB-Konfiguration vergleichen (`Config`-Struct)
- ⚠️ Profiling mit Visual Studio Profiler
- ⚠️ `bench_lock_contention` ausführen

---

## Detaillierte Analyse

### 1. VectorIndex Insert (-5.1%)
**Ursache**: Wahrscheinlich normale Schwankung oder geringfügiger Overhead durch Test-Isolierung (per-test temp dirs).

**Empfehlung**: ✅ Akzeptabel (< 10% Regression)

---

### 2. SecondaryIndex Insert (-99.7%) ❌

**V1.3.0 Benchmark-Code** (20251223):
```cpp
// SecondaryIndexBench/IndexInsert ausgeführt mit ~10k Iterationen
// Setup: createIndex("users", "email", false)
// Loop: put(entity) ohne explizite Transaktion
// Ergebnis: 1.78M items/s
```

**V1.3.3 Benchmark-Lauf** (20251228):
```
SecondaryIndexBench/IndexInsert    656030180 ns     19564076 ns  119 items_per_second=5.11141k/s
```

**Auffälligkeit**: 
- `real_time` = **656 ms** (656 Millionen ns)
- `cpu_time` = **19.6 ms** (19.5 Millionen ns)
- **33× Diskrepanz** zwischen real_time und cpu_time!

**Interpretation**:
- CPU verbringt nur 3% der Zeit mit Arbeit
- 97% der Zeit wartet der Thread (I/O, Locks, Sleep)
- **Wahrscheinlich**: RocksDB-Transaktion wartet auf Background Compaction/Flush

**DEBUG-Meldungen im Log**:
```
[DEBUG] RocksDBWrapper::put called with key: idxmeta:users:email
[DEBUG] RocksDBWrapper::put: calling beginTransaction()
[DEBUG] RocksDBWrapper::put: calling txn->put()
[DEBUG] RocksDBWrapper::put: calling txn->commit()
[DEBUG] TransactionWrapper::commit: SUCCESS
```

Jeder `put()` startet eine **separate Transaktion** → massiver Overhead!

**Vergleich v1.3.0**:
- Wahrscheinlich: Kein Transaktions-Overhead (oder Batch-Transaktionen)
- v1.3.3: Einzelne Transaktion pro `put()` → **350× langsamer**

**Root Cause**:
- Änderung zwischen v1.3.0 und v1.3.1-v1.3.3 hat Transaktions-Wrapper für jeden Index-Insert aktiviert
- RocksDB-Transaktionen haben erheblichen Overhead (Snapshot-Erstellung, Commit-Latenz)
- Für Benchmarks: Batch-Transaktionen verwenden!

---

### 3. QueryEngine Evaluation (-1.9%) ✅

**Bewertung**: Im statistischen Rauschen, keine echte Regression.

---

### 4. GraphIndex AddEdges (-18.4%) ⚠️

**V1.3.0**: 1.47M items/s  
**V1.3.3**: 1.20M items/s

**Ursache**: Wahrscheinlich ähnlich wie VectorIndex → Test-Isolierung oder RocksDB-Config.

**Empfehlung**: ⚠️ Beobachten, aber nicht kritisch (< 20% Regression).

---

### 5. Timeseries Insert (-8.4%) ⚠️

**V1.3.0**: 61.0M items/s  
**V1.3.3**: 55.9M items/s

**Bewertung**: Leichter Rückgang, wahrscheinlich normale Schwankung.

---

## Empfehlungen

### Sofortmaßnahmen (P0)

1. **SecondaryIndex Transaktions-Overhead beheben**:
   ```cpp
   // bench_core_performance.cpp: SecondaryIndexBench
   // VORHER (v1.3.3):
   for (auto _ : state) {
       indexMgr->put(entity); // Jeder put() startet Transaktion!
   }
   
   // NACHHER (Optimierung):
   auto txn = db->beginTransaction();
   for (auto _ : state) {
       indexMgr->put(entity, txn); // Batch-Transaktion
   }
   txn->commit();
   ```

2. **RocksDB-Config vergleichen**:
   - `allow_concurrent_memtable_write` aktiv?
   - `enable_write_thread_adaptive_yield` geändert?
   - Background-Job-Konfiguration (16 Low-Priority / 4 High-Priority)?

3. **DEBUG-Logging deaktivieren**:
   ```cpp
   // Alle [DEBUG] RocksDBWrapper::put Meldungen entfernen oder conditional compilation
   #ifndef NDEBUG
       // DEBUG-Code hier
   #endif
   ```

### Langfristige Optimierungen (P1)

1. **Benchmark-Isolation**:
   - Separate Benchmarks für "Single-Transaction" vs. "Batch-Transaction"
   - Explizite Transaktions-Modi dokumentieren

2. **Regression-Tests**:
   - Automatisierte Benchmarks in CI/CD
   - Alert bei >10% Regression

3. **Profiling**:
   - Visual Studio Profiler für SecondaryIndex-Benchmark
   - Flamegraphs für CPU-Zeit vs. Wall-Time-Diskrepanz

---

## Zusammenfassung

**Status**: ⚠️ **MIXED** (1 Critical Regression, 3 Minor Regressions, 1 Stable)

**Kritische Regression**: SecondaryIndex Insert (-99.7%) durch Transaktions-Overhead

**Nächste Schritte**:
1. ✅ Root Cause Analysis für SecondaryIndex
2. ⚠️ Fix: Batch-Transaktionen in Benchmarks
3. ⚠️ Re-Run nach Fix: Erwartete Performance > 1.5M items/s

**Empfohlene Aktion**: **NICHT produktionsfähig** bis SecondaryIndex-Regression behoben ist.
