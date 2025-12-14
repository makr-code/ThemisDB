# Analyse: Features externer Bibliotheken für ThemisDB

**Version:** 1.0  
**Datum:** Dezember 2025  
**Autor:** ThemisDB Development Team

## Executive Summary

ThemisDB nutzt eine Reihe leistungsstarker externer Bibliotheken (RocksDB, TBB, CUDA, Apache Arrow, Boost, OpenTelemetry, etc.), die weit mehr Features bieten, als derzeit aktiv genutzt werden. Diese Analyse identifiziert ungenutztes Potenzial und zukünftige Optimierungsmöglichkeiten.

**Wichtigste Erkenntnisse:**
- **RocksDB:** Nur ~30% der verfügbaren Features werden genutzt (z.B. TTL, Backup, Checkpoints fehlen)
- **TBB:** Grundlegende Parallelisierung vorhanden, aber fortgeschrittene Flow Graphs und Algorithmen ungenutzt
- **CUDA/GPU:** Infrastruktur vorhanden, aber nur minimale Nutzung in Vektor-Search
- **Apache Arrow:** Parquet und Compute Features vorhanden, aber nicht voll ausgeschöpft
- **Boost:** Nur wenige Komponenten genutzt (Asio, Beast, System), viele ungenutzte Libraries
- **OpenTelemetry:** Basic Tracing implementiert, aber Metrics und Context Propagation fehlen

## 1. RocksDB - Features & Potenzial

### 1.1 Aktuell genutzte Features

**Kern-Funktionalität:**
- ✅ Basic CRUD Operations (Put, Get, Delete)
- ✅ Iterators für Range-Scans
- ✅ WriteBatch für atomare Multi-Key Operations
- ✅ Transactions (TransactionDB)
- ✅ Column Families für Namespace-Trennung
- ✅ Compression (LZ4, ZSTD)
- ✅ Block Cache (LRU Cache)
- ✅ Bloom Filters für schnelle Point Lookups
- ✅ Statistics für Monitoring
- ✅ Compaction (Level/Universal Style)

**Analyse des aktuellen Codes:**
```cpp
// src/storage/rocksdb_wrapper.cpp
- Basic Options Configuration (Memtable, Block Cache, Compaction)
- TransactionDB für ACID-Garantien
- Column Families für Index-Separation
- Statistics für Performance Monitoring
```

### 1.2 Ungenutztes Potenzial - Kurzfristig (Q1-Q2 2026)

#### 1.2.1 Time-To-Live (TTL)
**Status:** ❌ Nicht genutzt  
**Priorität:** 🔥 Hoch  
**Use Case:** Automatisches Löschen abgelaufener Daten (Time Series, Cache, Session Data)

**Implementierungsvorschlag:**
```cpp
// TTL-basierte Column Family für Time Series
rocksdb::DBWithTTL::Open(options, db_path, &db, ttl_seconds);

// Oder: ColumnFamily-basierte TTL
rocksdb::ColumnFamilyOptions cf_opts;
cf_opts.ttl = 86400; // 24 Stunden
```

**Nutzen für ThemisDB:**
- Automatische Retention für `timeseries/tsstore.cpp`
- Session Management im `server/http_server.cpp`
- Cache Invalidierung in `cache/semantic_cache.cpp`

#### 1.2.2 Backup & Restore
**Status:** ⚠️ Teilweise implementiert (`backup_manager.cpp`)  
**Priorität:** 🔥 Hoch  
**Fehlende Features:**
- Incremental Backups (spart Speicherplatz)
- Backup Rotation & Retention Policies
- Cloud-Storage Integration (S3, Azure Blob)

**Implementierungsvorschlag:**
```cpp
// Incremental Backup
rocksdb::BackupEngine* backup_engine;
rocksdb::BackupEngine::Open(env, backup_opts, &backup_engine);
backup_engine->CreateNewBackupWithMetadata(db, metadata, flush_before_backup);

// Restore mit Backup-Auswahl
backup_engine->RestoreDBFromBackup(backup_id, db_dir, wal_dir);
```

#### 1.2.3 Write-Ahead Log (WAL) Advanced Features
**Status:** ⚠️ Basic WAL genutzt, keine Advanced Features  
**Priorität:** 🔥 Hoch

**Ungenutztes Potenzial:**
- WAL Archive für Point-In-Time Recovery
- WAL Shipping für Replication (bereits in `sharding/wal_shipper.cpp` skizziert)
- WAL Tail Reading für Change Data Capture

**Implementierungsvorschlag:**
```cpp
// WAL Archive für PITR
options.wal_ttl_seconds = 86400;
options.wal_size_limit_mb = 1024;

// WAL Tail Reading für CDC
std::unique_ptr<rocksdb::TransactionLogIterator> iter;
db->GetUpdatesSince(sequence_number, &iter);
```

#### 1.2.4 Merge Operators
**Status:** ❌ Nicht genutzt  
**Priorität:** 🟡 Mittel  
**Use Case:** Atomare Counter, Append-Operations ohne Read-Modify-Write

**Implementierungsvorschlag:**
```cpp
// Custom Merge Operator für Counter
class CounterMergeOperator : public rocksdb::MergeOperator {
  bool FullMerge(...) override {
    // Summiere alle Deltas
  }
};

// Nutzung
db->Merge(write_options, key, delta);
```

**Nutzen für ThemisDB:**
- Atomic Counter für `index/adaptive_index.cpp` (Query-Stats)
- Append-Only Logs für `utils/audit_logger.cpp`
- Aggregation für `analytics/olap.cpp`

### 1.3 Ungenutztes Potenzial - Mittelfristig (Q3-Q4 2026)

#### 1.3.1 RocksDB Replication (Follower Reads)
**Status:** 📋 Geplant (bereits `replication/replication_manager.cpp` vorhanden)  
**Priorität:** 🟡 Mittel

**Features:**
- Secondary Instance Mode (Read-only Follower)
- Automatic WAL Tailing
- Eventual Consistency für Read Scalability

#### 1.3.2 SstFileWriter (Bulk Loading)
**Status:** ❌ Nicht genutzt  
**Priorität:** 🟢 Niedrig  
**Use Case:** Schnelles Bulk-Insert für große Datensätze

**Nutzen für ThemisDB:**
- Import aus `importers/postgres_importer.cpp`
- Initial Data Loading
- Migration von Legacy-Systemen

#### 1.3.3 Rate Limiter (I/O Throttling)
**Status:** ❌ Nicht genutzt  
**Priorität:** 🟢 Niedrig

**Nutzen:**
- Verhinderung von I/O Spikes während Compaction
- QoS für Multi-Tenant Deployments

### 1.4 RocksDB Features - Zusammenfassung

| Feature | Status | Priorität | Nutzen für ThemisDB |
|---------|--------|-----------|---------------------|
| TTL (Time-To-Live) | ❌ | 🔥 Hoch | Auto-Cleanup für Time Series, Cache |
| Incremental Backups | ❌ | 🔥 Hoch | Effiziente Backups, Cloud-Integration |
| WAL Archive/Shipping | ⚠️ | 🔥 Hoch | PITR, Replication, CDC |
| Merge Operators | ❌ | 🟡 Mittel | Atomic Counter, Append-Logs |
| Secondary Instances | 📋 | 🟡 Mittel | Read Scalability |
| SstFileWriter | ❌ | 🟢 Niedrig | Bulk Loading |
| Rate Limiter | ❌ | 🟢 Niedrig | I/O QoS |

---

## 2. Intel TBB (Threading Building Blocks) - Features & Potenzial

### 2.1 Aktuell genutzte Features

**Grundlegende Parallelisierung:**
```cpp
// src/query/query_engine.cpp
tbb::parallel_for(...) - Parallel Query Execution

// src/security/field_encryption.cpp
tbb::parallel_for(...) - Parallel Field Encryption

// src/acceleration/cpu_backend_tbb.cpp
tbb::parallel_reduce(...) - Parallel Vector Distance Calculation
```

**Genutzte TBB Komponenten:**
- ✅ `tbb::parallel_for` - Basic Parallel Loops
- ✅ `tbb::parallel_reduce` - Parallel Reductions
- ✅ `tbb::task_group` - Task-based Parallelism (minimal)

### 2.2 Ungenutztes Potenzial - Kurzfristig (Q1-Q2 2026)

#### 2.2.1 TBB Flow Graph
**Status:** ❌ Nicht genutzt  
**Priorität:** 🔥 Hoch  
**Use Case:** Pipeline-Processing für Query Engine, Content Processing

**Implementierungsvorschlag:**
```cpp
// Flow Graph für Query Pipeline
tbb::flow::graph g;

// Nodes: Parse -> Optimize -> Execute -> Format
tbb::flow::function_node<Query, ParsedQuery> parse_node(g, ...);
tbb::flow::function_node<ParsedQuery, OptimizedQuery> optimize_node(g, ...);
tbb::flow::function_node<OptimizedQuery, Result> execute_node(g, ...);

tbb::flow::make_edge(parse_node, optimize_node);
tbb::flow::make_edge(optimize_node, execute_node);
```

**Nutzen für ThemisDB:**
- **Query Pipeline:** `query/query_engine.cpp` - Parse -> Optimize -> Execute
- **Content Processing:** `content/content_manager.cpp` - Extract -> Transform -> Index
- **Replication Pipeline:** `replication/replication_manager.cpp` - Capture -> Transform -> Apply

#### 2.2.2 Concurrent Data Structures
**Status:** ❌ TBB Containers nicht genutzt  
**Priorität:** 🔥 Hoch

**Verfügbare TBB Containers (besser als `std::mutex` + `std::map`):**
- `tbb::concurrent_hash_map` - Lock-free Hash Map
- `tbb::concurrent_queue` - Lock-free Queue
- `tbb::concurrent_vector` - Thread-safe Vector

**Implementierungsvorschlag:**
```cpp
// Ersetze std::map + std::mutex in caches
tbb::concurrent_hash_map<std::string, CachedResult> semantic_cache_;

// Query Queue für Load Balancing
tbb::concurrent_queue<Query> query_queue_;
```

**Nutzen für ThemisDB:**
- **Semantic Cache:** `cache/semantic_cache.cpp` - Lock-free Cache
- **Query Queue:** `server/http_server.cpp` - Thread-safe Request Queue
- **Connection Pool:** `utils/http_client_pool.cpp` - Lock-free Pool

#### 2.2.3 TBB Parallel Algorithms
**Status:** ⚠️ Nur wenige genutzt  
**Priorität:** 🟡 Mittel

**Ungenutztes Potenzial:**
- `tbb::parallel_sort` - Parallel Sorting für Index Building
- `tbb::parallel_scan` - Parallel Prefix Sum für Aggregates
- `tbb::parallel_pipeline` - Multi-Stage Pipelines

**Implementierungsvorschlag:**
```cpp
// Parallel Sort für Index Rebuild
tbb::parallel_sort(index_entries.begin(), index_entries.end());

// Parallel Pipeline für Batch Processing
tbb::parallel_pipeline(
  num_threads,
  tbb::make_filter<void, InputItem>(tbb::filter::serial_in_order, read_items),
  tbb::make_filter<InputItem, OutputItem>(tbb::filter::parallel, process_items),
  tbb::make_filter<OutputItem, void>(tbb::filter::serial_in_order, write_results)
);
```

### 2.3 Ungenutztes Potenzial - Mittelfristig (Q3-Q4 2026)

#### 2.3.1 TBB Task Scheduler Observer
**Status:** ❌ Nicht genutzt  
**Priorität:** 🟢 Niedrig  
**Use Case:** Thread-Local Initialization, Profiling

**Nutzen:**
- Custom Thread Pools für RocksDB Background Jobs
- Thread-Local RocksDB Transactions

#### 2.3.2 TBB Memory Allocators
**Status:** ❌ Nicht genutzt  
**Priorität:** 🟢 Niedrig

**Features:**
- `scalable_allocator` - NUMA-aware Allocator
- `cache_aligned_allocator` - False-Sharing Prevention

### 2.4 TBB Features - Zusammenfassung

| Feature | Status | Priorität | Nutzen für ThemisDB |
|---------|--------|-----------|---------------------|
| Flow Graph | ❌ | 🔥 Hoch | Query/Content Pipelines |
| Concurrent Containers | ❌ | 🔥 Hoch | Lock-free Caches, Queues |
| Parallel Algorithms | ⚠️ | 🟡 Mittel | Sort, Scan, Pipeline |
| Task Scheduler Observer | ❌ | 🟢 Niedrig | Thread-Local Init |
| Memory Allocators | ❌ | 🟢 Niedrig | NUMA-aware Allocation |

---

## 3. CUDA & GPU Acceleration - Features & Potenzial

### 3.1 Aktuell genutzte Features

**Minimal-Implementierung:**
```cpp
// src/acceleration/cuda_backend.cpp
- Basic CUDA Kernel für Vector Distance Berechnung
- cudaMalloc/cudaMemcpy für Device Memory Management

// CMakeLists.txt
- CUDA Toolkit Integration
- Optional Build Flag (THEMIS_ENABLE_CUDA)
```

**Genutzte CUDA Features:**
- ✅ Basic CUDA Kernels (`__global__` functions)
- ✅ Device Memory Management
- ✅ CUDA Runtime API

### 3.2 Ungenutztes Potenzial - Kurzfristig (Q1-Q2 2026)

#### 3.2.1 CUDA Streams (Parallel Kernel Execution)
**Status:** ❌ Nicht genutzt  
**Priorität:** 🔥 Hoch  
**Use Case:** Overlap von Compute und Memory Transfer

**Implementierungsvorschlag:**
```cpp
// Multiple CUDA Streams für parallele Batch-Verarbeitung
cudaStream_t streams[NUM_BATCHES];
for (int i = 0; i < NUM_BATCHES; i++) {
  cudaStreamCreate(&streams[i]);
  cudaMemcpyAsync(d_input[i], h_input[i], size, cudaMemcpyHostToDevice, streams[i]);
  vector_similarity_kernel<<<grid, block, 0, streams[i]>>>(d_input[i], d_query, d_output[i]);
  cudaMemcpyAsync(h_output[i], d_output[i], size, cudaMemcpyDeviceToHost, streams[i]);
}
cudaDeviceSynchronize();
```

**Nutzen für ThemisDB:**
- **Vector Search:** `index/vector_index.cpp` - Parallel Batch Query Processing
- **Graph Analytics:** `index/graph_analytics.cpp` - PageRank, Centrality Berechnung
- **Geo Queries:** `geo/gpu_backend_stub.cpp` - Spatial Joins

#### 3.2.2 cuBLAS (BLAS auf GPU)
**Status:** ❌ Nicht genutzt  
**Priorität:** 🔥 Hoch  
**Use Case:** Matrix-Operationen für GNN Embeddings, OLAP

**Implementierungsvorschlag:**
```cpp
// cuBLAS für Matrix-Multiplikation (GNN Layer)
cublasHandle_t handle;
cublasCreate(&handle);
cublasSgemm(handle, CUBLAS_OP_N, CUBLAS_OP_N, m, n, k, &alpha, d_A, lda, d_B, ldb, &beta, d_C, ldc);
```

**Nutzen für ThemisDB:**
- **GNN Embeddings:** `index/gnn_embeddings.cpp` - Graph Neural Network Inference
- **OLAP:** `analytics/olap.cpp` - Matrix Aggregations
- **Recommendation:** Future Feature - Collaborative Filtering

#### 3.2.3 Thrust Library (CUDA C++ Template Library)
**Status:** ❌ Nicht genutzt  
**Priorität:** 🟡 Mittel  
**Use Case:** High-Level GPU Algorithms (Sort, Reduce, Scan)

**Implementierungsvorschlag:**
```cpp
// Thrust für Parallel Sort auf GPU
thrust::device_vector<float> d_vec(h_vec.begin(), h_vec.end());
thrust::sort(d_vec.begin(), d_vec.end());

// Thrust für Parallel Reduce
float sum = thrust::reduce(d_vec.begin(), d_vec.end(), 0.0f, thrust::plus<float>());
```

**Nutzen für ThemisDB:**
- **Index Building:** Parallel Sort für Vector Index
- **Aggregates:** GPU-accelerated SUM, COUNT, AVG

### 3.3 Ungenutztes Potenzial - Mittelfristig (Q3-Q4 2026)

#### 3.3.1 CUDA Unified Memory
**Status:** ❌ Nicht genutzt  
**Priorität:** 🟡 Mittel  
**Use Case:** Vereinfachtes Memory Management (kein manuelles cudaMemcpy)

**Vorteil:**
- Automatische Page Migration zwischen Host/Device
- Reduzierter Boilerplate Code

#### 3.3.2 NVGRAPH (Graph Analytics auf GPU)
**Status:** ❌ Nicht genutzt (deprecated, aber Nachfolger: cuGraph)  
**Priorität:** 🟢 Niedrig  
**Use Case:** PageRank, BFS, SSSP auf GPU

#### 3.3.3 TensorRT (Deep Learning Inference)
**Status:** ❌ Nicht genutzt  
**Priorität:** 🟢 Niedrig (außer GNN Feature wird erweitert)  
**Use Case:** Optimierte GNN Inference, CLIP Embeddings

### 3.4 CUDA Features - Zusammenfassung

| Feature | Status | Priorität | Nutzen für ThemisDB |
|---------|--------|-----------|---------------------|
| CUDA Streams | ❌ | 🔥 Hoch | Parallel Batch Processing |
| cuBLAS | ❌ | 🔥 Hoch | GNN, OLAP Matrix Ops |
| Thrust Library | ❌ | 🟡 Mittel | GPU Algorithms |
| Unified Memory | ❌ | 🟡 Mittel | Simplified Memory Mgmt |
| cuGraph | ❌ | 🟢 Niedrig | Graph Analytics |
| TensorRT | ❌ | 🟢 Niedrig | DL Inference |

---

## 4. Apache Arrow - Features & Potenzial

### 4.1 Aktuell genutzte Features

**Minimal-Integration:**
```cpp
// CMakeLists.txt
find_package(Arrow CONFIG QUIET)
target_link_libraries(themis_core PUBLIC Arrow::arrow_shared)

// Features in vcpkg.json
"arrow", "features": ["parquet", "compute"]
```

**Status:**
- ⚠️ Arrow dependency vorhanden, aber **Code-Nutzung unklar**
- ⚠️ Parquet Feature installiert, aber keine `#include <arrow/parquet/...>` gefunden
- ⚠️ Compute Feature installiert, aber keine Arrow Compute Kernels genutzt

### 4.2 Ungenutztes Potenzial - Kurzfristig (Q1-Q2 2026)

#### 4.2.1 Parquet File Format
**Status:** ❌ Installiert, aber nicht genutzt  
**Priorität:** 🔥 Hoch  
**Use Case:** Effiziente Spalten-orientierte Speicherung für OLAP

**Implementierungsvorschlag:**
```cpp
// Export OLAP Query Results zu Parquet
#include <arrow/io/file.h>
#include <parquet/arrow/writer.h>

std::shared_ptr<arrow::Table> table = ...;
std::shared_ptr<arrow::io::FileOutputStream> outfile;
PARQUET_ASSIGN_OR_THROW(outfile, arrow::io::FileOutputStream::Open("output.parquet"));

parquet::WriterProperties::Builder builder;
builder.compression(parquet::Compression::SNAPPY);

parquet::arrow::WriteTable(*table, arrow::default_memory_pool(), outfile, /*chunk_size=*/1024);
```

**Nutzen für ThemisDB:**
- **OLAP Export:** `analytics/olap.cpp` - Export für Data Warehouses
- **Backup Format:** Alternative zu RocksDB Backup (Spalten-Format)
- **Data Lake Integration:** Export zu S3/HDFS

#### 4.2.2 Arrow Compute Kernels
**Status:** ❌ Installiert, aber nicht genutzt  
**Priorität:** 🔥 Hoch  
**Use Case:** SIMD-optimierte Aggregationen

**Implementierungsvorschlag:**
```cpp
#include <arrow/compute/api.h>

// Arrow Compute für GROUP BY Aggregation
arrow::compute::ExecContext ctx;
arrow::Result<arrow::Datum> result = arrow::compute::CallFunction(
  "sum", {arrow::Datum(array)}, nullptr, &ctx
);
```

**Nutzen für ThemisDB:**
- **OLAP Aggregates:** `analytics/olap.cpp` - SIMD-optimierte SUM, AVG, COUNT
- **Query Engine:** `query/query_engine.cpp` - Filter/Project Operations
- **Time Series:** `timeseries/continuous_agg.cpp` - Windowed Aggregations

#### 4.2.3 Arrow Flight (RPC Framework)
**Status:** ❌ Nicht installiert  
**Priorität:** 🟡 Mittel  
**Use Case:** High-Performance RPC für Shard-to-Shard Communication

**Nutzen für ThemisDB:**
- **Sharding:** `sharding/remote_executor.cpp` - Ersatz für HTTP/REST
- **Replication:** `replication/replication_manager.cpp` - WAL Streaming

### 4.3 Ungenutztes Potenzial - Mittelfristig (Q3-Q4 2026)

#### 4.3.1 Arrow Dataset API
**Status:** ❌ Nicht genutzt  
**Priorität:** 🟢 Niedrig  
**Use Case:** Partitioned Data Management (ähnlich Hive/Spark)

#### 4.3.2 Arrow Gandiva (LLVM-based Expression Compiler)
**Status:** ❌ Nicht genutzt  
**Priorität:** 🟢 Niedrig  
**Use Case:** JIT-kompilierte Filter Expressions

### 4.4 Arrow Features - Zusammenfassung

| Feature | Status | Priorität | Nutzen für ThemisDB |
|---------|--------|-----------|---------------------|
| Parquet Format | ❌ | 🔥 Hoch | OLAP Export, Backup |
| Compute Kernels | ❌ | 🔥 Hoch | SIMD Aggregations |
| Arrow Flight | ❌ | 🟡 Mittel | Shard RPC |
| Dataset API | ❌ | 🟢 Niedrig | Partitioned Data |
| Gandiva | ❌ | 🟢 Niedrig | JIT Filters |

---

## 5. Boost Libraries - Features & Potenzial

### 5.1 Aktuell genutzte Features

**Genutzte Boost Komponenten:**
```cpp
// vcpkg.json
"boost-system", "boost-asio", "boost-beast", "boost-optional"

// CMakeLists.txt
find_package(Boost REQUIRED COMPONENTS system)

// Code
Boost::asio - HTTP Server (server/http_server.cpp)
Boost::beast - HTTP Protocol (server/http_server.cpp)
Boost::system - System Errors
```

### 5.2 Ungenutztes Potenzial - Kurzfristig (Q1-Q2 2026)

#### 5.2.1 Boost.Geometry (bereits erwähnt im Code!)
**Status:** ⚠️ Teilweise genutzt (`geo/boost_cpu_exact_backend.cpp`)  
**Priorität:** 🟡 Mittel  
**Use Case:** Erweiterte Geometrie-Operationen

**Ungenutztes Potenzial:**
- Boost.Geometry Spatial Index (R-Tree)
- Buffer/Envelope Operations
- Advanced Predicates (Touches, Crosses, Overlaps)

#### 5.2.2 Boost.Interprocess
**Status:** ❌ Nicht genutzt  
**Priorität:** 🟡 Mittel  
**Use Case:** Shared Memory für Multi-Process Architecture

**Nutzen für ThemisDB:**
- Shared Memory Cache zwischen Prozessen
- Zero-Copy Data Sharing (z.B. für Plugin-System)

#### 5.2.3 Boost.Serialization
**Status:** ❌ Nicht genutzt (stattdessen: `nlohmann::json`)  
**Priorität:** 🟢 Niedrig  
**Use Case:** Binäre Serialisierung (effizienter als JSON)

### 5.3 Boost Features - Zusammenfassung

| Feature | Status | Priorität | Nutzen für ThemisDB |
|---------|--------|-----------|---------------------|
| Geometry R-Tree | ⚠️ | 🟡 Mittel | Spatial Index |
| Interprocess | ❌ | 🟡 Mittel | Shared Memory Cache |
| Serialization | ❌ | 🟢 Niedrig | Binary Serialization |

---

## 6. OpenTelemetry - Features & Potenzial

### 6.1 Aktuell genutzte Features

**Basic Tracing:**
```cpp
// src/utils/tracing.cpp
- Span Creation
- Span Attributes
- OTLP HTTP Exporter

// CMakeLists.txt
opentelemetry-cpp::trace
opentelemetry-cpp::otlp_http_exporter
```

### 6.2 Ungenutztes Potenzial - Kurzfristig (Q1-Q2 2026)

#### 6.2.1 Metrics API
**Status:** ❌ Nicht genutzt  
**Priorität:** 🔥 Hoch  
**Use Case:** Application Metrics (Request Rate, Latency, Errors)

**Implementierungsvorschlag:**
```cpp
#include <opentelemetry/metrics/provider.h>

auto meter = metrics::Provider::GetMeterProvider()->GetMeter("themis");
auto counter = meter->CreateUInt64Counter("requests_total", "Total requests");
counter->Add(1, {{"endpoint", "/query"}, {"status", "200"}});
```

**Nutzen für ThemisDB:**
- **HTTP Metrics:** `server/http_server.cpp` - Request Rate, Latency
- **Query Metrics:** `query/query_engine.cpp` - Query Duration, Cache Hit Rate
- **Storage Metrics:** `storage/rocksdb_wrapper.cpp` - Read/Write Throughput

#### 6.2.2 Context Propagation
**Status:** ❌ Nicht genutzt  
**Priorität:** 🔥 Hoch  
**Use Case:** Distributed Tracing über Shard-Grenzen

**Implementierungsvorschlag:**
```cpp
// HTTP Header Injection für Shard-to-Shard Calls
#include <opentelemetry/trace/propagation/http_trace_context.h>

HttpTextMapCarrier carrier;
auto propagator = trace::propagation::HttpTraceContext();
propagator.Inject(carrier, context);
// Send carrier in HTTP headers
```

#### 6.2.3 Resource Attributes
**Status:** ⚠️ Minimal genutzt  
**Priorität:** 🟡 Mittel

**Fehlende Attributes:**
- `service.instance.id` - Shard ID
- `deployment.environment` - prod/staging/dev
- `host.name`, `host.arch` - Infrastructure Info

### 6.3 OpenTelemetry Features - Zusammenfassung

| Feature | Status | Priorität | Nutzen für ThemisDB |
|---------|--------|-----------|---------------------|
| Metrics API | ❌ | 🔥 Hoch | Request/Query Metrics |
| Context Propagation | ❌ | 🔥 Hoch | Distributed Tracing |
| Resource Attributes | ⚠️ | 🟡 Mittel | Service Identification |

---

## 7. Weitere Bibliotheken - Kurzanalyse

### 7.1 simdjson
**Aktuell:** ✅ JSON Parsing  
**Ungenutztes Potenzial:**
- On-Demand API (lazy parsing für große JSON)
- Schema Validation

### 7.2 hnswlib
**Aktuell:** ✅ Vector Index (HNSW)  
**Ungenutztes Potenzial:**
- Persistence (Save/Load Index zu Disk) - **bereits teilweise implementiert**
- Dynamic Index Updates (Add/Delete Vectors)

### 7.3 spdlog
**Aktuell:** ✅ Logging  
**Ungenutztes Potenzial:**
- Async Logging (non-blocking)
- Log Rotation Policies

### 7.4 yaml-cpp
**Aktuell:** ✅ Config Parsing  
**Ungenutztes Potenzial:**
- Schema Validation
- YAML Emitter für Config Export

---

## 8. Priorisierte Roadmap

### Phase 1: Kurzfristig (Q1-Q2 2026) - Quick Wins

**RocksDB:**
1. ✅ TTL für Time Series & Cache (2 Wochen)
2. ✅ Incremental Backups (1 Woche)
3. ✅ WAL Archive für PITR (2 Wochen)
4. ✅ Merge Operators für Counters (1 Woche)

**TBB:**
1. ✅ Flow Graph für Query Pipeline (3 Wochen)
2. ✅ Concurrent Containers (2 Wochen)
3. ✅ Parallel Algorithms (1 Woche)

**OpenTelemetry:**
1. ✅ Metrics API (2 Wochen)
2. ✅ Context Propagation (1 Woche)

**Apache Arrow:**
1. ✅ Parquet Export (2 Wochen)
2. ✅ Compute Kernels für OLAP (3 Wochen)

**Gesamt:** ~15-20 Wochen Engineering Effort

### Phase 2: Mittelfristig (Q3-Q4 2026)

**CUDA:**
1. ✅ CUDA Streams (2 Wochen)
2. ✅ cuBLAS Integration (2 Wochen)
3. ✅ Thrust Library (1 Woche)

**Arrow:**
1. ✅ Arrow Flight für Sharding RPC (4 Wochen)

**Boost:**
1. ✅ Geometry R-Tree (2 Wochen)
2. ✅ Interprocess Shared Memory (2 Wochen)

**Gesamt:** ~13 Wochen Engineering Effort

---

## 9. Kosten-Nutzen-Analyse

### High-Impact Features (ROI > 5x)

| Feature | Engineering Effort | Nutzen | ROI |
|---------|-------------------|--------|-----|
| RocksDB TTL | 2 Wochen | Auto-Cleanup, reduzierter Ops-Aufwand | 10x |
| TBB Flow Graph | 3 Wochen | 2-3x Query Throughput | 8x |
| OpenTelemetry Metrics | 2 Wochen | Observability, Debugging | 7x |
| Arrow Compute | 3 Wochen | 5-10x OLAP Performance | 6x |
| TBB Concurrent Containers | 2 Wochen | Reduzierte Lock Contention | 5x |

### Medium-Impact Features (ROI 2-5x)

| Feature | Engineering Effort | Nutzen | ROI |
|---------|-------------------|--------|-----|
| CUDA Streams | 2 Wochen | 2x GPU Throughput | 4x |
| Arrow Parquet | 2 Wochen | Data Lake Integration | 3x |
| RocksDB Merge Operators | 1 Woche | Atomic Counters | 3x |

### Low-Impact Features (ROI < 2x)

- Boost.Serialization (JSON ausreichend)
- CUDA Unified Memory (Komplexität vs. Nutzen)
- Arrow Dataset API (derzeit kein Use Case)

---

## 10. Empfehlungen

### Sofort umsetzen (Q1 2026):
1. **RocksDB TTL** - Kritisch für Production
2. **OpenTelemetry Metrics** - Unverzichtbar für Monitoring
3. **TBB Flow Graph** - Höchster Performance-Impact

### Nächste Schritte (Q2 2026):
4. **Arrow Compute Kernels** - OLAP Performance
5. **TBB Concurrent Containers** - Scalability
6. **RocksDB Incremental Backups** - Operational Excellence

### Später evaluieren (Q3-Q4 2026):
7. **CUDA Advanced Features** - Nur wenn GPU-Adoption steigt
8. **Arrow Flight** - Nur wenn Sharding im Prod-Einsatz ist

---

## 11. Risiken & Mitigationen

### Risiko 1: Komplexität-Explosion
**Mitigation:** Inkrementelle Integration, Feature Flags

### Risiko 2: Performance-Regressionen
**Mitigation:** Benchmarks vor/nach jeder Integration

### Risiko 3: Breaking Changes in Dependencies
**Mitigation:** Version Pinning, Automated Tests

---

## Fazit

ThemisDB nutzt derzeit **~30-40% des Potenzials** der integrierten Bibliotheken. Die größten Optimierungschancen liegen in:

1. **RocksDB Advanced Features** (TTL, WAL Archive, Merge Ops)
2. **TBB Flow Graphs & Concurrent Containers**
3. **Arrow Parquet & Compute**
4. **OpenTelemetry Metrics & Context Propagation**

**Geschätzter ROI:** 5-10x Performance-Gewinn bei 20-30 Wochen Engineering Effort.

---

**Nächste Schritte:**
1. Priorisierung mit Stakeholdern
2. Spike-Tests für Top 3 Features
3. Integration Roadmap in 2026 Planung

**Anhänge:**
- A: RocksDB Feature Matrix (Vollständige Liste)
- B: TBB Performance Benchmarks
- C: Arrow Compute Kernel Vergleich
- D: Code-Beispiele & Tutorials
