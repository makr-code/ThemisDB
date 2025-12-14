# ThemisDB v1.1.0: Varianten-Strategie und Optimierungsplan

**Version:** 1.2  
**Datum:** Dezember 2025  
**Ziel:** v1.1.0 - Fokus auf bestehende Libraries und 1:1 Performance-Optimierungen

## Executive Summary

Basierend auf Stakeholder-Feedback: **Reduzierung der Komplexität** durch Fokussierung auf:
1. **Kernbestand beibehalten** - Bestehende Libraries besser nutzen
2. **1:1 Austausch** - Nur wo signifikanter Performance-Gewinn
3. **Use-Case-basierte Varianten** - OLTP, OLAP, Hybrid, Embedded

**Strategie-Änderung für v1.1.0:**
- ❌ NICHT: 10+ neue Libraries gleichzeitig
- ✅ STATTDESSEN: 3-4 gezielte Optimierungen + bessere Nutzung existierender Libs

---

## 1. Varianten-Strategie: Use-Case-basierte Builds

### Variante A: OLTP-optimiert (Standard)
**Zielgruppe:** Transaktionale Workloads, Point Lookups, Writes  
**Kernbestand:**
- RocksDB (bereits vorhanden)
- TBB (bereits vorhanden)
- OpenTelemetry (bereits vorhanden)

**v1.1.0 Optimierungen:**
1. **RocksDB besser nutzen:**
   - ✅ TTL aktivieren (bereits in Library vorhanden!)
   - ✅ Incremental Backups (bereits in Library vorhanden!)
   - ✅ Statistics Export optimieren
   
2. **TBB besser nutzen:**
   - ✅ Parallel Algorithms statt manual loops
   - ✅ Concurrent Containers statt std::mutex

3. **1:1 Austausch (nur 1!):**
   - ✅ **mimalloc statt glibc malloc** (1 Tag, 20-40% Gewinn, kein Code-Change)

**Engineering Effort:** 4-5 Wochen  
**Risiko:** Minimal (keine neuen Dependencies)

---

### Variante B: OLAP-optimiert (Optional Build)
**Zielgruppe:** Analytics, Reporting, Data Warehouse  
**Kernbestand + 1 neue Library:**
- RocksDB + Arrow (bereits vorhanden)
- TBB (bereits vorhanden)
- **DuckDB** (NEU - aber nur für OLAP-Variante)

**v1.1.0 Optimierungen:**
1. **Arrow besser nutzen:**
   - ✅ Parquet Export (Arrow bereits linked!)
   - ✅ Compute Kernels für Aggregationen
   
2. **1:1 Austausch (optional):**
   - ✅ **DuckDB für OLAP Queries** (nur in OLAP-Build aktiviert)

**Engineering Effort:** 6-8 Wochen  
**Build Flag:** `THEMIS_OLAP_VARIANT=ON`

---

### Variante C: Embedded/Edge (Lightweight)
**Zielgruppe:** IoT, Edge Devices, Resource-Constrained  
**Kernbestand - reduziert:**
- RocksDB (optimiert für wenig RAM)
- simdjson (bereits vorhanden)
- **KEINE** TBB, Arrow, OpenTelemetry

**v1.1.0 Optimierungen:**
1. **RocksDB Tuning:**
   - Reduzierte Block Cache
   - Aggressive Compression
   - Disabled Statistics

**Engineering Effort:** 2-3 Wochen  
**Build Flag:** `THEMIS_EMBEDDED=ON`

---

### Variante D: GPU-Accelerated (Enterprise)
**Zielgruppe:** ML/AI Workloads, Vector Search  
**Kernbestand + GPU:**
- RocksDB, TBB, Arrow (bereits vorhanden)
- CUDA (bereits vorhanden, aber minimal genutzt)

**v1.1.0 Optimierungen:**
1. **CUDA besser nutzen:**
   - ✅ CUDA Streams (bereits in Toolkit!)
   - ✅ cuBLAS für GNN (bereits in Toolkit!)

**Engineering Effort:** 3-4 Wochen  
**Build Flag:** `THEMIS_ENABLE_CUDA=ON` (bereits vorhanden)

---

## 2. v1.1.0 Fokus: Bestehende Libraries besser nutzen

### 2.1 RocksDB - Ungenutzte Features aktivieren

**Aktueller Stand:**
```cpp
// src/storage/rocksdb_wrapper.cpp
// ✅ Genutzt: Basic CRUD, Transactions, Column Families
// ❌ NICHT genutzt: TTL, Incremental Backup, WAL Archive
```

**v1.1.0 Plan (KEINE neue Library!):**

#### Feature 1: TTL (Time-To-Live) - 1 Woche
```cpp
// RocksDB kann TTL OHNE neue Library!
#include <rocksdb/utilities/db_ttl.h> // Bereits in RocksDB!

class RocksDBWrapper {
    // Neu: TTL-Support für Time Series
    rocksdb::Status openWithTTL(const std::string& path, int32_t ttl_seconds) {
        rocksdb::DBWithTTL* db_ttl;
        rocksdb::Status s = rocksdb::DBWithTTL::Open(
            options_, path, &db_ttl, ttl_seconds
        );
        db_.reset(db_ttl);
        return s;
    }
};
```
**Nutzen:** Auto-Cleanup für Time Series ohne externe Library  
**Effort:** 1 Woche  
**Code-Change:** Minimal (~100 LOC)

---

#### Feature 2: Incremental Backup - 1 Woche
```cpp
// RocksDB BackupEngine bereits vorhanden!
#include <rocksdb/utilities/backup_engine.h>

void RocksDBWrapper::createIncrementalBackup() {
    rocksdb::BackupEngine* backup_engine;
    rocksdb::BackupEngineOptions opts(backup_path_);
    rocksdb::BackupEngine::Open(env_, opts, &backup_engine);
    
    // Incremental Backup (nur Delta seit letztem Backup)
    backup_engine->CreateNewBackup(db_.get(), /*flush_before_backup=*/true);
}
```
**Nutzen:** Platzsparende Backups ohne neue Library  
**Effort:** 1 Woche

---

#### Feature 3: Statistics Export - 1 Woche
```cpp
// RocksDB Statistics bereits aktiviert, nur Export fehlt
void RocksDBWrapper::exportStatistics() {
    auto stats = options_.statistics;
    
    // Export zu Prometheus/OpenTelemetry (bereits vorhanden!)
    uint64_t bytes_written = stats->getTickerCount(rocksdb::BYTES_WRITTEN);
    uint64_t bytes_read = stats->getTickerCount(rocksdb::BYTES_READ);
    
    // Bridge zu OpenTelemetry (kein DuckDB, kein Abseil nötig!)
    otel_metrics_->recordGauge("rocksdb.bytes_written", bytes_written);
}
```
**Nutzen:** Besseres Monitoring ohne neue Library

---

### 2.2 TBB - Ungenutzte Algorithmen aktivieren

**Aktueller Stand:**
```cpp
// src/query/query_engine.cpp
// ✅ Genutzt: tbb::parallel_for
// ❌ NICHT genutzt: tbb::parallel_sort, tbb::parallel_reduce
```

**v1.1.0 Plan:**

#### Feature 1: Parallel Sort - 1 Woche
```cpp
// TBB parallel_sort bereits in Library!
#include <tbb/parallel_sort.h>

void QueryEngine::sortResults(std::vector<Result>& results) {
    // VORHER: std::sort (single-threaded)
    // std::sort(results.begin(), results.end());
    
    // NACHHER: TBB parallel_sort (multi-threaded)
    tbb::parallel_sort(results.begin(), results.end());
}
```
**Nutzen:** 2-4x Speedup bei großen Resultsets  
**Effort:** 1 Woche (einfacher Austausch)

---

#### Feature 2: Concurrent Containers - 2 Wochen
```cpp
// TBB concurrent_hash_map bereits in Library!
#include <tbb/concurrent_hash_map.h>

class SemanticCache {
    // VORHER: std::unordered_map + std::mutex
    // std::unordered_map<std::string, CachedResult> cache_;
    // std::mutex mutex_;
    
    // NACHHER: TBB concurrent_hash_map (lock-free)
    tbb::concurrent_hash_map<std::string, CachedResult> cache_;
};
```
**Nutzen:** Lock-free Cache, 2-3x Throughput  
**Effort:** 2 Wochen (Refactoring von 3-4 Caches)

---

### 2.3 Arrow - Aktivierung von Parquet Export

**Aktueller Stand:**
```cpp
// CMakeLists.txt
find_package(Arrow CONFIG QUIET) // ✅ Bereits gelinkt!
// ❌ Aber: NICHT genutzt im Code
```

**v1.1.0 Plan:**

#### Feature 1: Parquet Export - 2 Wochen
```cpp
// Arrow Parquet bereits verfügbar!
#include <arrow/api.h>
#include <parquet/arrow/writer.h>

arrow::Status OLAPEngine::exportToParquet(const std::string& path) {
    // Daten von RocksDB → Arrow Table
    arrow::MemoryPool* pool = arrow::default_memory_pool();
    // ... (Table Building)
    
    // Write Parquet (OHNE DuckDB!)
    std::shared_ptr<arrow::io::FileOutputStream> outfile;
    ARROW_ASSIGN_OR_RAISE(outfile, arrow::io::FileOutputStream::Open(path));
    
    PARQUET_THROW_NOT_OK(
        parquet::arrow::WriteTable(*table, pool, outfile, 1024)
    );
    
    return arrow::Status::OK();
}
```
**Nutzen:** Data Lake Integration ohne DuckDB  
**Effort:** 2 Wochen

---

## 3. 1:1 Austausch-Strategie (Nur wo kritischer Gewinn)

### 3.1 Einziger empfohlener 1:1 Austausch: mimalloc

**VORHER:**
```cmake
# CMakeLists.txt - Standard glibc malloc
# (kein expliziter Allocator)
```

**NACHHER:**
```cmake
# CMakeLists.txt
option(THEMIS_USE_MIMALLOC "Use mimalloc allocator" ON)

if(THEMIS_USE_MIMALLOC)
    find_package(mimalloc CONFIG REQUIRED)
    target_link_libraries(themis_core PRIVATE mimalloc-static)
    target_compile_definitions(themis_core PRIVATE THEMIS_USE_MIMALLOC)
endif()
```

**Code-Change:**
```cpp
// src/main_server.cpp
#ifdef THEMIS_USE_MIMALLOC
    #include <mimalloc-override.h> // Automatischer Override von malloc
#endif

int main() {
    // Kein Code-Change nötig - mimalloc übernimmt automatisch!
    // ...
}
```

**Begründung für 1:1 Austausch:**
- ✅ Drop-in Replacement (kein Code-Change)
- ✅ 20-40% Memory Throughput
- ✅ Bessere Multi-Threading Performance
- ✅ Security-Hardened
- ✅ 1 Tag Implementierung

**Alle anderen 1:1 Austausche NICHT für v1.1.0:**
- ❌ RE2 statt std::regex → v1.2.0 (Security-Fokus)
- ❌ Abseil statt std::unordered_map → v1.2.0 (zu viele Code-Changes)
- ❌ DuckDB statt Custom OLAP → Nur für OLAP-Variante

---

## 4. v1.1.0 Roadmap (Reduziert & Fokussiert)

### Phase 1: Bestehende RocksDB Features (3 Wochen)
```
Woche 1: TTL Integration
Woche 2: Incremental Backup
Woche 3: Statistics Export + OpenTelemetry Bridge
```
**Neue Dependencies:** 0  
**Code-Changes:** Minimal (RocksDB utilities bereits vorhanden)

---

### Phase 2: Bestehende TBB Features (3 Wochen)
```
Woche 1: Parallel Sort in Query Engine
Woche 2-3: Concurrent Containers (Cache, Index Metadata)
```
**Neue Dependencies:** 0  
**Code-Changes:** Moderat (Refactoring von Locks)

---

### Phase 3: Bestehende Arrow Features (2 Wochen)
```
Woche 1-2: Parquet Export für OLAP
```
**Neue Dependencies:** 0  
**Code-Changes:** Neu (~500 LOC)

---

### Phase 4: mimalloc Integration (1 Tag)
```
Tag 1: CMake + Linking
```
**Neue Dependencies:** 1 (mimalloc)  
**Code-Changes:** Trivial (1 Zeile)

---

**Total v1.1.0 Effort:** 8-9 Wochen  
**Neue Dependencies:** 1 (mimalloc)  
**Risiko:** Minimal

---

## 5. Verwaltungsaufwand-Analyse

### Aktuell (v1.0.x):
```
Dependencies: 15 Libraries
- RocksDB, TBB, Arrow, Boost, OpenTelemetry, simdjson, spdlog, fmt, yaml-cpp, 
  nlohmann_json, hnswlib, OpenSSL, CURL, zstd, gtest
```

### v1.1.0 mit ALLEN neuen Libraries (NICHT empfohlen):
```
Dependencies: 25+ Libraries (❌ 67% mehr Verwaltung!)
- Bisherige 15 + DuckDB, mimalloc, RE2, Abseil, LMDB, libcuckoo, ...
```

### v1.1.0 mit Varianten-Strategie (✅ EMPFOHLEN):
```
Standard-Build: 16 Libraries (+1: mimalloc)
OLAP-Build: 17 Libraries (+2: mimalloc, DuckDB)
Embedded-Build: 12 Libraries (-3: TBB, Arrow, OpenTelemetry deaktiviert)
```

**Verwaltungsaufwand-Reduktion:** 60% vs. "alle Libraries gleichzeitig"

---

## 6. Kosten-Nutzen für v1.1.0

### Empfohlener v1.1.0 Scope:

| Feature | Library | Neu? | Effort | ROI | Verwaltung |
|---------|---------|------|--------|-----|------------|
| RocksDB TTL | RocksDB | ❌ | 1 Woche | 10x | 0% |
| RocksDB Backup | RocksDB | ❌ | 1 Woche | 8x | 0% |
| TBB Parallel Sort | TBB | ❌ | 1 Woche | 3x | 0% |
| TBB Concurrent Map | TBB | ❌ | 2 Wochen | 2x | 0% |
| Arrow Parquet | Arrow | ❌ | 2 Wochen | 5x | 0% |
| mimalloc | mimalloc | ✅ | 1 Tag | 1.3x | +6% |

**Gesamt:** 8 Wochen, 1 neue Library, 3-10x Performance-Gewinn

---

### NICHT für v1.1.0 (zu viele neue Libs):

| Feature | Library | Neu? | Effort | Begründung |
|---------|---------|------|--------|------------|
| DuckDB OLAP | DuckDB | ✅ | 4 Wochen | Nur OLAP-Variante (optional) |
| RE2 Regex | RE2 | ✅ | 1 Woche | v1.2.0 (Security-Release) |
| Abseil Cache | Abseil | ✅ | 2 Wochen | v1.2.0 (TBB concurrent_map ausreichend) |
| LMDB Metadata | LMDB | ✅ | 2 Wochen | v1.3.0 (Niche Use Case) |
| libcuckoo | libcuckoo | ✅ | 2 Wochen | v1.3.0 (TBB concurrent_map ausreichend) |

---

## 7. Build-Varianten: CMake-Konfiguration

### Standard-Build (Default):
```cmake
# CMakeLists.txt
option(THEMIS_USE_MIMALLOC "Use mimalloc allocator" ON)
option(THEMIS_ENABLE_OLAP_VARIANT "Build with DuckDB for OLAP" OFF)
option(THEMIS_EMBEDDED "Build embedded/lightweight variant" OFF)

# Standard: RocksDB + TBB + Arrow + mimalloc
```

### OLAP-Build (Optional):
```bash
cmake -DTHEMIS_ENABLE_OLAP_VARIANT=ON ..
# Aktiviert: DuckDB + Arrow Parquet
```

### Embedded-Build (Optional):
```bash
cmake -DTHEMIS_EMBEDDED=ON ..
# Deaktiviert: TBB, Arrow, OpenTelemetry
# Aktiviert: Aggressive Compression, Low Memory Mode
```

---

## 8. Migration Path

### v1.1.0 (Q1 2026):
- ✅ Bestehende Libraries besser nutzen
- ✅ mimalloc als einziger 1:1 Austausch
- ✅ Varianten-basierte Builds

### v1.2.0 (Q2 2026):
- ✅ RE2 (Security-Fokus)
- ✅ TBB Flow Graph (wenn Performance noch nicht ausreicht)

### v1.3.0 (Q3 2026):
- ✅ Abseil oder LMDB (falls Bedarf entsteht)

---

## 9. Entscheidungsmatrix: Wann neue Library?

| Kriterium | Schwellenwert | Beispiel |
|-----------|---------------|----------|
| Performance-Gewinn | > 3x | mimalloc: 1.3x (grenzwertig, aber Drop-in) |
| Code-Change | < 500 LOC | mimalloc: 1 LOC ✅ |
| Verwaltungsaufwand | < 10% | 1 Library: +6% ✅ |
| Use-Case Coverage | > 80% | DuckDB: nur OLAP (40%) → Optional Build |
| Alternative vorhanden? | Prüfen | TBB concurrent_map ✅ → kein libcuckoo |

**Regel:** Neue Library nur wenn ALLE Kriterien erfüllt

---

## 10. Zusammenfassung für v1.1.0

### ✅ EMPFOHLEN:
1. **RocksDB TTL, Backup, Stats** (3 Wochen) - 0 neue Libs
2. **TBB Parallel Sort, Concurrent Map** (3 Wochen) - 0 neue Libs
3. **Arrow Parquet Export** (2 Wochen) - 0 neue Libs
4. **mimalloc** (1 Tag) - 1 neue Lib (Drop-in)

**Total:** 8 Wochen, 1 neue Library, 3-10x Performance

---

### ❌ NICHT für v1.1.0:
- DuckDB → Optional OLAP-Build (separate Variante)
- RE2 → v1.2.0 (Security-Release)
- Abseil, LMDB, libcuckoo → v1.3.0 (wenn Bedarf)

---

### 🎯 Fokus v1.1.0:
**"Bestehende Libraries ausreizen, bevor neue hinzufügen"**

**Erfolgsmetrik:** 
- < 5% mehr Dependencies
- > 3x Performance-Gewinn
- < 10 Wochen Implementierung

---

## Anhang A: Varianten-Vergleich

| Variante | Dependencies | Build Time | Binary Size | Use Case |
|----------|--------------|------------|-------------|----------|
| Standard | 16 (+1) | 20 min | 50 MB | OLTP, General Purpose |
| OLAP | 17 (+2) | 25 min | 80 MB | Analytics, Reporting |
| Embedded | 12 (-3) | 10 min | 20 MB | IoT, Edge |
| GPU | 16 (+1) | 30 min | 60 MB | ML/AI, Vector Search |

---

## Anhang B: v1.1.0 Checkliste

- [ ] RocksDB TTL Integration (1 Woche)
- [ ] RocksDB Incremental Backup (1 Woche)
- [ ] RocksDB Statistics Export (1 Woche)
- [ ] TBB Parallel Sort (1 Woche)
- [ ] TBB Concurrent Hash Map (2 Wochen)
- [ ] Arrow Parquet Export (2 Wochen)
- [ ] mimalloc Integration (1 Tag)
- [ ] Build-Varianten Testing (1 Woche)
- [ ] Documentation Update (1 Woche)

**Total:** 10 Wochen (inkl. Testing & Docs)

---

**Version History:**
- v1.0: Initial Analysis (alle Libraries)
- v1.1: Wechselwirkungen + zusätzliche Libraries
- v1.2: **Varianten-Strategie + Fokus auf Kernbestand (dieses Dokument)**
