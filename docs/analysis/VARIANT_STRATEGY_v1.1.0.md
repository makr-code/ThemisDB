# ThemisDB v1.1.0: Varianten-Strategie und Optimierungsplan

**Version:** 1.3  
**Datum:** Dezember 2025  
**Ziel:** v1.1.0 - Fokus auf bestehende Libraries und 1:1 Performance-Optimierungen  
**Deployment-Szenario:** ThemisDB (CPU/RAM + minimal GPU) + vLLM (GPU/VRAM + minimal CPU) Co-Location

## Executive Summary

Basierend auf Stakeholder-Feedback: **Reduzierung der Komplexität** durch Fokussierung auf:
1. **Kernbestand beibehalten** - Bestehende Libraries besser nutzen
2. **1:1 Austausch** - Nur wo signifikanter Performance-Gewinn
3. **Use-Case-basierte Varianten** - OLTP, OLAP, Hybrid, Embedded
4. **🆕 ThemisDB + vLLM Synergie** - Ressourcen-optimierte Co-Location

**Strategie-Änderung für v1.1.0:**
- ❌ NICHT: 10+ neue Libraries gleichzeitig
- ✅ STATTDESSEN: 3-4 gezielte Optimierungen + bessere Nutzung existierender Libs
- ✅ **NEU:** CUDA als Kernbestand (wenn GPU verfügbar, nicht Enterprise)
- ✅ **NEU:** vLLM Co-Location Optimierung (CPU/RAM ↔ GPU/VRAM Balance)

---

## 🆕 0. ThemisDB + vLLM Co-Location Strategie

### 0.1 Deployment-Szenario

**Typische Server-Konfiguration:**
```
Hardware:
- CPU: 64 Cores (z.B. AMD EPYC / Intel Xeon)
- RAM: 256 GB DDR4/DDR5
- GPU: 4x NVIDIA A100 (80 GB VRAM each) oder H100

Workload-Verteilung:
┌─────────────────────────────────────────────┐
│  ThemisDB                    vLLM           │
│  (CPU/RAM heavy)        (GPU/VRAM heavy)    │
├─────────────────────────────────────────────┤
│  CPU: 50-60 Cores        CPU: 4-14 Cores    │
│  RAM: 200 GB             RAM: 56 GB         │
│  GPU: Minimal (CUDA      GPU: 4x A100       │
│       Streams für              (320 GB      │
│       Vector Search)           VRAM total)  │
└─────────────────────────────────────────────┘
```

**Synergie-Punkte:**
1. **CPU-Allokation:** ThemisDB nutzt Cores, die vLLM nicht braucht
2. **RAM-Allokation:** ThemisDB nutzt RAM für Caching, vLLM minimal
3. **GPU-Sharing:** ThemisDB nutzt GPU nur für spezifische Tasks (Vector Search), vLLM dominiert
4. **Datenaustausch:** ThemisDB speichert Embeddings, vLLM generiert sie

---

### 0.2 Ressourcen-Koordination

#### CPU/RAM Thread-Allokation (ThemisDB-optimiert)
```cpp
// src/main_server.cpp - Resource Coordination
#include <thread>

struct ResourceConfig {
    // Total System Resources
    size_t total_cpu_cores = std::thread::hardware_concurrency(); // 64
    size_t total_ram_gb = 256;
    
    // vLLM Reservation (Tensor Parallel + Pipeline Parallel)
    size_t vllm_cpu_cores = 14;  // vLLM braucht wenig CPU
    size_t vllm_ram_gb = 56;     // Model Loading + KV Cache
    
    // ThemisDB Allocation
    size_t themis_cpu_cores = total_cpu_cores - vllm_cpu_cores;  // 50 Cores
    size_t themis_ram_gb = total_ram_gb - vllm_ram_gb;            // 200 GB
    
    // ThemisDB Internal Allocation
    size_t rocksdb_threads = themis_cpu_cores * 0.3;  // 15 Cores
    size_t tbb_threads = themis_cpu_cores * 0.6;      // 30 Cores
    size_t system_reserve = themis_cpu_cores * 0.1;   // 5 Cores
};

void configureThemisDB(const ResourceConfig& config) {
    // RocksDB Background Jobs
    rocksdb::Options opts;
    opts.max_background_jobs = config.rocksdb_threads;
    
    // TBB Thread Pool
    tbb::global_control tbb_limit(
        tbb::global_control::max_allowed_parallelism,
        config.tbb_threads
    );
    
    // RocksDB Memory Budget (80% of allocated RAM)
    size_t block_cache_mb = (config.themis_ram_gb * 0.8 * 1024) * 0.6; // 60% für Cache
    size_t memtable_mb = (config.themis_ram_gb * 0.8 * 1024) * 0.3;    // 30% für Memtables
    
    opts.write_buffer_size = memtable_mb * 1024 * 1024 / 3;
    rocksdb::BlockBasedTableOptions table_opts;
    table_opts.block_cache = rocksdb::NewLRUCache(block_cache_mb * 1024 * 1024);
}
```

---

#### GPU/VRAM Sharing-Strategie
```cpp
// src/acceleration/cuda_backend.cpp - GPU Sharing mit vLLM

class CUDAResourceManager {
public:
    void initializeSharedGPU() {
        // vLLM nutzt GPUs 0-3 für Model Inference
        // ThemisDB nutzt GPU 0 mit niedriger Priorität für Vector Search
        
        // CUDA Stream mit niedriger Priorität (non-blocking für vLLM)
        cudaStream_t themis_stream;
        cudaStreamCreateWithPriority(&themis_stream, cudaStreamNonBlocking, -1);
        
        // Minimale VRAM-Allokation (vLLM hat Vorrang)
        size_t themis_vram_mb = 2048; // 2 GB pro GPU (vLLM hat 78 GB)
        
        // Batch-Größe begrenzen, um vLLM nicht zu stören
        size_t max_vector_batch = 1024; // Statt 10k+
    }
    
    // Adaptive GPU-Nutzung: Nur wenn vLLM idle
    bool canUseGPU() {
        // Check GPU Utilization (nvml)
        nvmlDevice_t device;
        nvmlDeviceGetHandleByIndex(0, &device);
        
        nvmlUtilization_t util;
        nvmlDeviceGetUtilizationRates(device, &util);
        
        // Nur GPU nutzen wenn < 80% Auslastung (vLLM idle)
        return util.gpu < 80;
    }
};
```

---

### 0.3 ThemisDB + vLLM Integration Pattern

#### Use Case: RAG (Retrieval-Augmented Generation)
```
Workflow:
1. User Query → vLLM (Embedding-Modell, z.B. BGE-large)
   - GPU: Embedding-Generierung (1-2ms)
   - Output: Query-Vektor [1024 dim]

2. Vector Search → ThemisDB (HNSW Index)
   - CPU: HNSW Traversal (5-10ms) ODER
   - GPU: CUDA Vector Search (1-2ms, wenn verfügbar)
   - Output: Top-K relevante Dokumente

3. Context Augmentation → ThemisDB (RocksDB)
   - CPU: Dokument-Retrieval (1-2ms)
   - RAM: Cache Hit (sub-ms)

4. LLM Generation → vLLM (Llama 3 70B)
   - GPU: Autoregressive Decoding (100-500ms)
   - Output: Generated Answer

Total Latency: ~110-520ms (CPU+GPU optimiert)
```

**ThemisDB-Optimierungen für RAG:**
```cpp
// src/index/vector_index.cpp - vLLM-optimierte Vector Search

class VectorIndex {
    // Hybrid CPU/GPU Search (abhängig von vLLM-Last)
    std::vector<Result> search(const float* query_vec, size_t k) {
        if (cuda_mgr_->canUseGPU()) {
            // vLLM idle → GPU nutzen (1-2ms)
            return searchGPU(query_vec, k);
        } else {
            // vLLM busy → CPU fallback (5-10ms)
            return searchCPU(query_vec, k);
        }
    }
    
    // Prefetch für vLLM-Kontext
    void prefetchForLLM(const std::vector<std::string>& doc_ids) {
        // Dokumente in RAM-Cache laden (bevor vLLM sie braucht)
        tbb::parallel_for_each(doc_ids.begin(), doc_ids.end(), 
            [this](const std::string& id) {
                rocksdb_->Get(read_opts_, id, &cache_[id]);
            }
        );
    }
};
```

---

### 0.4 Build-Konfiguration: ThemisDB + vLLM Co-Location

**CMake Preset für Co-Location:**
```cmake
# CMakeLists.txt
option(THEMIS_VLLM_COLOCATION "Optimize for vLLM co-location" ON)

if(THEMIS_VLLM_COLOCATION)
    # CUDA aktiviert (Kernbestand, NICHT Enterprise!)
    set(THEMIS_ENABLE_CUDA ON CACHE BOOL "" FORCE)
    
    # Resource Limits für GPU-Sharing
    target_compile_definitions(themis_core PRIVATE
        THEMIS_MAX_GPU_VRAM_MB=2048        # 2 GB pro GPU (vLLM hat Rest)
        THEMIS_MAX_VECTOR_BATCH_SIZE=1024  # Kleine Batches
        THEMIS_GPU_LOW_PRIORITY=1          # Niedriger als vLLM
    )
    
    # CPU/RAM Optimierungen
    target_compile_definitions(themis_core PRIVATE
        THEMIS_CPU_CORES_RESERVED=50       # 50 von 64 Cores
        THEMIS_RAM_GB_ALLOCATED=200        # 200 von 256 GB
    )
endif()
```

**Docker Compose Beispiel:**
```yaml
# docker-compose.yml - ThemisDB + vLLM
version: '3.8'
services:
  themisdb:
    image: themisdb/themisdb:v1.1.0-cuda
    environment:
      - THEMIS_CPU_CORES=50
      - THEMIS_RAM_GB=200
      - THEMIS_GPU_VRAM_MB=2048
    deploy:
      resources:
        limits:
          cpus: '50'
          memory: 200G
        reservations:
          devices:
            - driver: nvidia
              count: 1
              capabilities: [gpu]
    
  vllm:
    image: vllm/vllm-openai:latest
    command: >
      --model meta-llama/Llama-3-70b-chat-hf
      --tensor-parallel-size 4
      --gpu-memory-utilization 0.95
    deploy:
      resources:
        limits:
          cpus: '14'
          memory: 56G
        reservations:
          devices:
            - driver: nvidia
              count: 4
              capabilities: [gpu]
```

---

## 1. Varianten-Strategie: Use-Case-basierte Builds (Aktualisiert)

### Variante A: OLTP-optimiert (Standard)
**Zielgruppe:** Transaktionale Workloads, Point Lookups, Writes  
**Kernbestand:**
- RocksDB (bereits vorhanden)
- TBB (bereits vorhanden)
- OpenTelemetry (bereits vorhanden)
- **CUDA (bereits vorhanden) - KERNBESTAND wenn GPU verfügbar!**

**v1.1.0 Optimierungen:**
1. **RocksDB besser nutzen:**
   - ✅ TTL aktivieren (bereits in Library vorhanden!)
   - ✅ Incremental Backups (bereits in Library vorhanden!)
   - ✅ Statistics Export optimieren
   
2. **TBB besser nutzen:**
   - ✅ Parallel Algorithms statt manual loops
   - ✅ Concurrent Containers statt std::mutex

3. **CUDA besser nutzen (wenn GPU verfügbar):**
   - ✅ CUDA Streams für Vector Search
   - ✅ Adaptive GPU-Nutzung (vLLM Co-Location)

4. **1:1 Austausch (nur 1!):**
   - ✅ **mimalloc statt glibc malloc** (1 Tag, 20-40% Gewinn, kein Code-Change)

**Engineering Effort:** 4-5 Wochen  
**Risiko:** Minimal (keine neuen Dependencies)

---

### Variante B: OLAP-optimiert (Optional Build)
**Zielgruppe:** Analytics, Reporting, Data Warehouse  
**Kernbestand + 1 neue Library:**
- RocksDB + Arrow (bereits vorhanden)
- TBB (bereits vorhanden)
- CUDA (optional, für Parquet-Processing)
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
- **KEINE** TBB, Arrow, OpenTelemetry, CUDA

**v1.1.0 Optimierungen:**
1. **RocksDB Tuning:**
   - Reduzierte Block Cache
   - Aggressive Compression
   - Disabled Statistics

**Engineering Effort:** 2-3 Wochen  
**Build Flag:** `THEMIS_EMBEDDED=ON`

---

### Variante D: vLLM Co-Location (🆕 EMPFOHLEN für AI/ML)
**Zielgruppe:** RAG, Semantic Search, AI Workloads  
**Kernbestand + vLLM-Optimierungen:**
- RocksDB, TBB, Arrow (bereits vorhanden)
- **CUDA (Kernbestand!)** - Adaptive GPU-Nutzung mit vLLM
- mimalloc (Memory-Effizienz)

**v1.1.0 Optimierungen:**
1. **CUDA besser nutzen:**
   - ✅ CUDA Streams mit niedriger Priorität
   - ✅ Adaptive GPU-Nutzung (nur wenn vLLM < 80% Last)
   - ✅ VRAM-Limit (2 GB, Rest für vLLM)
   
2. **CPU/RAM Koordination:**
   - ✅ CPU-Allokation: 50 von 64 Cores
   - ✅ RAM-Allokation: 200 von 256 GB
   - ✅ Thread-Pool Tuning (RocksDB 30%, TBB 60%)

3. **RAG-Optimierungen:**
   - ✅ Vector Search Prefetching
   - ✅ Document Cache Warming
   - ✅ Hybrid CPU/GPU Search

**Engineering Effort:** 5-6 Wochen  
**Build Flag:** `THEMIS_VLLM_COLOCATION=ON` (automatisch aktiviert CUDA)

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
Standard-Build (OLTP): 16 Libraries (+1: mimalloc)
  - CUDA wenn verfügbar (Kernbestand, nicht Enterprise!)
  
OLAP-Build: 17 Libraries (+2: mimalloc, DuckDB)

Embedded-Build: 12 Libraries (-3: TBB, Arrow, OpenTelemetry deaktiviert, kein CUDA)

vLLM Co-Location Build: 16 Libraries (+1: mimalloc)
  - CUDA IMMER aktiviert (Kernbestand!)
  - Optimiert für GPU-Sharing mit vLLM
  - CPU/RAM Koordination (50 Cores, 200 GB)
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
| CUDA Streams | CUDA | ❌ | 1 Woche | 2x | 0% (Kernbestand!) |
| vLLM Koordination | - | ❌ | 1 Woche | 3x | 0% (Config-only) |
| mimalloc | mimalloc | ✅ | 1 Tag | 1.3x | +6% |

**Gesamt:** 9 Wochen, 1 neue Library, 3-10x Performance-Gewinn  
**🆕 vLLM Co-Location:** +1 Woche für Ressourcen-Koordination
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
option(THEMIS_VLLM_COLOCATION "Optimize for vLLM co-location" OFF)

# Standard: RocksDB + TBB + Arrow + mimalloc
# CUDA: Automatisch aktiviert wenn Hardware erkannt (Kernbestand!)
```

### vLLM Co-Location Build (🆕 EMPFOHLEN für AI/ML):
```bash
cmake -DTHEMIS_VLLM_COLOCATION=ON ..
# Aktiviert: CUDA (forced), GPU-Sharing, CPU/RAM Koordination
# Setzt automatisch: THEMIS_ENABLE_CUDA=ON
```

### OLAP-Build (Optional):
```bash
cmake -DTHEMIS_ENABLE_OLAP_VARIANT=ON ..
# Aktiviert: DuckDB + Arrow Parquet
```

### Embedded-Build (Optional):
```bash
cmake -DTHEMIS_EMBEDDED=ON ..
# Deaktiviert: TBB, Arrow, OpenTelemetry, CUDA
# Aktiviert: Aggressive Compression, Low Memory Mode
```

---

## 8. Migration Path

### v1.1.0 (Q1 2026):
- ✅ Bestehende Libraries besser nutzen (RocksDB, TBB, Arrow)
- ✅ CUDA als Kernbestand (nicht Enterprise) - wenn GPU verfügbar
- ✅ mimalloc als einziger 1:1 Austausch
- ✅ Varianten-basierte Builds
- ✅ 🆕 vLLM Co-Location Optimierung

### v1.2.0 (Q2 2026):
- ✅ RE2 (Security-Fokus)
- ✅ TBB Flow Graph (wenn Performance noch nicht ausreicht)
- ✅ Erweiterte vLLM Integration (Embedding Cache Warming)

### v1.3.0 (Q3 2026):
- ✅ Abseil oder LMDB (falls Bedarf entsteht)
- ✅ Multi-vLLM Load Balancing

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
4. **CUDA Streams** (1 Woche) - 0 neue Libs (Kernbestand!)
5. **🆕 vLLM Co-Location** (1 Woche) - 0 neue Libs (Konfiguration)
6. **mimalloc** (1 Tag) - 1 neue Lib (Drop-in)

**Total:** 9 Wochen, 1 neue Library, 3-10x Performance

---

### ❌ NICHT für v1.1.0:
- DuckDB → Optional OLAP-Build (separate Variante)
- RE2 → v1.2.0 (Security-Release)
- Abseil, LMDB, libcuckoo → v1.3.0 (wenn Bedarf)

---

### 🎯 Fokus v1.1.0:
**"Bestehende Libraries ausreizen, bevor neue hinzufügen"**

**🆕 Zusätzlicher Fokus:**
**"ThemisDB + vLLM Synergie für maximale Ressourcen-Effizienz"**

**Erfolgsmetrik:** 
- < 5% mehr Dependencies (✅ nur mimalloc)
- > 3x Performance-Gewinn (✅ RocksDB + TBB + CUDA)
- < 10 Wochen Implementierung (✅ 9 Wochen)
- 🆕 Optimale GPU-Sharing mit vLLM (< 80% GPU-Auslastung für ThemisDB)
- 🆕 CPU/RAM Balance (50 Cores, 200 GB für ThemisDB)

---

## Anhang A: Varianten-Vergleich

| Variante | Dependencies | Build Time | Binary Size | Use Case | CUDA |
|----------|--------------|------------|-------------|----------|------|
| Standard (OLTP) | 16 (+1) | 20 min | 50 MB | OLTP, General Purpose | Optional¹ |
| OLAP | 17 (+2) | 25 min | 80 MB | Analytics, Reporting | Optional |
| Embedded | 12 (-3) | 10 min | 20 MB | IoT, Edge | ❌ |
| 🆕 vLLM Co-Location | 16 (+1) | 25 min | 55 MB | RAG, AI/ML Workloads | ✅ Kernbestand |

¹ CUDA wird automatisch aktiviert wenn GPU erkannt (Kernbestand, nicht Enterprise!)

---

## Anhang B: v1.1.0 Checkliste

### Kern-Features (8 Wochen):
- [ ] RocksDB TTL Integration (1 Woche)
- [ ] RocksDB Incremental Backup (1 Woche)
- [ ] RocksDB Statistics Export (1 Woche)
- [ ] TBB Parallel Sort (1 Woche)
- [ ] TBB Concurrent Hash Map (2 Wochen)
- [ ] Arrow Parquet Export (2 Wochen)

### GPU & vLLM (2 Wochen):
- [ ] 🆕 CUDA Streams für Vector Search (1 Woche)
- [ ] 🆕 vLLM Co-Location Ressourcen-Koordination (1 Woche)
  - [ ] CPU/RAM Allokations-Logik
  - [ ] GPU-Sharing mit Priorität
  - [ ] Adaptive CUDA-Nutzung (nvml Monitoring)

### Performance & Build (1 Woche):
- [ ] mimalloc Integration (1 Tag)
- [ ] Build-Varianten Testing (3 Tage)
- [ ] Docker Compose ThemisDB+vLLM (2 Tage)
- [ ] Documentation Update (1 Tag)

**Total:** 11 Wochen (inkl. vLLM-Integration, Testing & Docs)

---

## 🆕 Anhang C: vLLM Co-Location Best Practices

### Hardware-Empfehlung:
```
Minimum:
- CPU: 32 Cores (20 ThemisDB, 10 vLLM, 2 System)
- RAM: 128 GB (90 ThemisDB, 35 vLLM, 3 System)
- GPU: 2x NVIDIA A100 40GB (vLLM primär, ThemisDB gelegentlich)

Optimal:
- CPU: 64 Cores (50 ThemisDB, 12 vLLM, 2 System)
- RAM: 256 GB (200 ThemisDB, 50 vLLM, 6 System)
- GPU: 4x NVIDIA A100 80GB (vLLM primär, ThemisDB adaptiv)
```

### Monitoring-Metriken:
```
ThemisDB:
- CPU Utilization (Target: 70-80%)
- RAM Usage (Target: < 200 GB)
- GPU Utilization (Target: < 20% wenn vLLM aktiv)
- Vector Search Latency (Target: < 10ms)

vLLM:
- GPU Utilization (Target: 70-90%)
- VRAM Usage (Target: 70-80 GB pro GPU)
- Token Generation Latency (Target: < 50ms/token)
- Concurrent Requests (Target: 10-50)

System:
- GPU Memory Contention (Target: 0 OOM errors)
- CPU Context Switches (Target: < 100k/s)
- Network Throughput ThemisDB↔vLLM (Target: < 1 Gbps)
```

### Fallback-Strategien:
```
Wenn vLLM GPU-Last > 90%:
→ ThemisDB nutzt CPU-only Vector Search (HNSW)
→ Latenz: 5-10ms statt 1-2ms (akzeptabel)

Wenn RAM < 20 GB frei:
→ ThemisDB reduziert Block Cache (aggressive eviction)
→ Leichte Performance-Degradation, aber kein OOM

Wenn CPU-Last > 90%:
→ ThemisDB aktiviert Rate Limiting
→ vLLM bleibt unbeeinflusst (höhere Priorität)
```

---

**Version History:**
- v1.0: Initial Analysis (alle Libraries)
- v1.1: Wechselwirkungen + zusätzliche Libraries
- v1.2: Varianten-Strategie + Fokus auf Kernbestand
- v1.3: **🆕 vLLM Co-Location + CUDA als Kernbestand (dieses Dokument)**
