> **Roadmap-Hinweis:** Vage Bullets ohne Akzeptanzkriterien in Checkbox-Tasks überführen. Format: `- [ ] <Task> (Target: <Q/Jahr>)`.

# Index Module Roadmap

<!-- Status: [ ] open  [~] in progress  [x] done  [I] Issue  [P] PR  [?] blocked  [!] unclear -->

## Current Status
v1.x – Production-grade indexing infrastructure. HNSW vector indexing, B-tree/range secondary indexes, R-tree spatial indexes, and graph adjacency indexes are all implemented with optional GPU acceleration.

## Completed ✅
- [x] HNSW vector similarity index (L2, Cosine, Dot Product)
- [x] GPU-accelerated vector search (Vulkan, CUDA, HIP via GPUVectorIndex)
- [x] Product Quantization (PQ), Binary Quantization, Residual Quantization
- [x] B-tree, range, sparse, and composite secondary indexes
- [x] R-tree spatial index with Z-order curves
- [x] Graph indexing (adjacency lists, BFS/DFS traversal)
- [x] Adaptive index recommendations based on query patterns
- [x] IVF+PQ and FAISS integration
- [x] Multi-vector search
- [x] GNN embeddings, temporal graphs, rotary embeddings
- [x] IndexManager with dependency injection (breaks circular deps)
- [x] RocksDB persistence for vector indexes with WriteBatch atomicity
- [x] Audit logging for vector operations
- [x] Full-text inverted index integration (Target: Q2 2026) (Issue: #1433)
- [x] Automated index advisor with workload replay (Target: Q2 2026) (Issue: #1434)
- [x] HNSW incremental re-indexing without full rebuild (Target: Q3 2026) (Issue: #1435)

## In Progress 🚧
- [~] GPU memory oversubscription — paging/streaming for datasets > VRAM (Target: v1.7.0)
  - [x] `GPUMemoryOversubscriptionManager` with LRU eviction, streaming, prefetch (NONE/LRU/MRU/SEQUENTIAL)
  - [x] `GPUVectorIndex::Config` extended with `enable_oversubscription`, `vram_budget_mb`, `prefetch_strategy`
  - [x] `GPUVectorIndex::getOversubscriptionStats()` for monitoring
  - [x] Unified-memory integration via `GPUUnifiedMemoryAllocator` (CUDA/HIP with CPU fallback)
  - [x] Focused test suite in `tests/index/test_gpu_memory_oversubscription.cpp` (26 tests)
  - [x] CI workflow `.github/workflows/gpu-memory-oversubscription-ci.yml`

## Planned Features 📋

### Short-term (Next 3-6 months)
- [x] Index statistics export to metadata module (Issue: #1866)
- [P] Online index rebuild with minimal read impact (Issue: #1868)
- [P] Configurable GPU memory budget per index (Issue: #1869)

### Long-term (6-12 months)
- [P] Multi-tenancy index isolation (Issue: #1872)

## Implementation Phases

### Phase 1: Production-Grade Index Infrastructure (Status: Completed ✅)
- [x] HNSW vector similarity index (L2, Cosine, Dot Product) in `index/vector_index.cpp`
- [x] GPU-accelerated vector search via GPUVectorIndex (Vulkan, CUDA, HIP)
- [x] Product Quantization (PQ), Binary Quantization, and Residual Quantization
- [x] B-tree, range, sparse, and composite secondary indexes (`index/secondary_index.cpp`)
- [x] R-tree spatial index with Z-order curves (`index/spatial_index.cpp`)
- [x] Graph indexing: adjacency lists, BFS/DFS traversal (`index/graph_index.cpp`)
- [x] Adaptive index recommendations based on query patterns
- [x] IVF+PQ and FAISS integration, multi-vector search
- [x] GNN embeddings, temporal graphs, rotary embeddings
- [x] IndexManager with dependency injection to break circular dependencies
- [x] RocksDB persistence for vector indexes with WriteBatch atomicity
- [x] Audit logging for vector operations

### Phase 2: Full-Text & Automated Advisor (Status: Completed ✅)
- [x] Full-text inverted index integration (Target: Q2 2026)
- [x] Automated index advisor with workload replay (Target: Q2 2026)
- [x] HNSW incremental re-indexing without full rebuild (Target: Q3 2026)

### Phase 3: Learned Structures & GPU Build (Status: In Progress 🚧)
- [x] DiskANN / ScaNN alternative ANN algorithms for on-disk indexes (Issue: #1876)
- [x] Matryoshka Representation Learning (MRL) truncation for multi-stage retrieval (v1.8.0)
  - Files: `include/index/matryoshka_truncation.h`, `src/index/matryoshka_truncation.cpp`
  - Backend: transparent `IAnnIndex` decorator (`MatryoshkaTruncatedIndex`) wrapping any ANN backend
  - Granularities: `kMRL_64/128/256/512/768/1024/1536` (OpenAI text-embedding-3, Nomic Embed v1.5, BGE-M3)
  - Tests: `tests/index/test_matryoshka_truncation.cpp` — 25 unit tests, AC-1 through AC-25
  - CI: `.github/workflows/matryoshka-truncation-ci.yml`
  - Performance: O(trunc_dim) truncation + normalisation; no index-build overhead beyond backend
- [x] Learned index structures (ML-based B-tree replacement)
- [I] GPU-accelerated index build for large-scale vector datasets (Issue: #1878)
- [x] Parallel batch search across GPUs in MultiGPUVectorIndex
- [x] GPU utilization tracking in MultiGPUVectorIndex statistics
- [x] IndexSuggestionEngine::indexExists backed by in-memory index registry
- [x] Distributed vector index across shards
- [x] Partial / filtered indexes on secondary index manager
- [x] Cold/warm tier index migration (Issue: #2407) (Target: Q3 2026)
- [P] Multi-tenancy index isolation via RocksDB key-prefix scoping (Issue: #1872)

## Production Readiness Checklist
- [I] Unit tests coverage > 80% (Issue: #1882)
- [P] Integration tests (HNSW recall@10, spatial correctness) (Issue: #1883)
- [P] Performance benchmarks (QPS, recall, memory) (Issue: #1884)
- [I] Security audit (GPU memory safety, RocksDB key prefix isolation) (Issue: #1885)
- [I] Documentation complete (Issue: #1886)
- [I] API stability guaranteed (Issue: #1887)

## Known Issues & Limitations
- GPU acceleration requires Vulkan/CUDA drivers; falls back to CPU automatically.
- HNSW incremental re-indexing implemented via `VectorIndexManager::incrementalReindex()`.
- Full-text search is implemented via `InvertedIndex` (standalone) and `SecondaryIndexManager` (integrated).
- Multi-tenancy index isolation uses RocksDB key-prefix format `tenant:<id>:<index_name>`;
  index registry isolation is enforced at the `IndexManager` layer.  Both `IVectorIndex`
  (`VectorIndexAdapter`) and `ISecondaryIndex` (`SecondaryIndexAdapter`) adapters are fully
  implemented and returned by `IndexManager::createVectorIndex()` / `createSecondaryIndex()`.
  Partial (filtered) indexes are created by passing `config = "partial:<predicate>"` to
  `createSecondaryIndex()`.
- `ProcessGraphManager` multi-model query functions (v2026-04-13): 13 previously stubbed
  methods are now fully implemented:
  - Relational: `queryTasksByFormData`, `joinWithCollection`, `aggregateByField`
  - Vector: `findSimilarProcesses` (cosine similarity), `findSimilarTasks`, `semanticSearchProcesses` (keyword scoring)
  - Anomaly: `detectAnomalies` (duration z-score + path deviation)
  - Geo: `findTasksInArea` (Haversine radius), `findTasksInGeofence` (WKT ray-casting),
    `optimizeTaskRoute` (nearest-neighbor TSP), `validateLocationConstraint`, `getRegionalParameters`
  - Cross-model: `executeMultiModelQuery` (BFS graph + relational + vector + geo combined)

## Breaking Changes
- `IndexManager` factory API (`createDefault()`) is stable from v1.x.
- GPU index configuration struct may gain new fields in v2.0; additive only.
