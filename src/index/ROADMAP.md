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

## In Progress 🚧
- [x] Full-text inverted index integration (Target: Q2 2026) (Issue: #1433)
- [I] Automated index advisor with workload replay (Target: Q2 2026) (Issue: #1434)
- [P] HNSW incremental re-indexing without full rebuild (Target: Q3 2026) (Issue: #1435)

## Planned Features 📋

### Short-term (Next 3-6 months)
- [I] DiskANN / ScaNN alternative ANN algorithms (Issue: #1865)
- [I] Index statistics export to metadata module (Issue: #1866)
- [I] Partial / filtered indexes on secondary index manager (Issue: #1880)
- [I] Online index rebuild with minimal read impact (Issue: #1868)
- [I] Configurable GPU memory budget per index (Issue: #1869)

### Long-term (6-12 months)
- [I] Distributed vector index across shards (Issue: #1879)
- [I] Learned index structures (ML-based B-tree replacement) (Issue: #1990)
- [I] Multi-tenancy index isolation (Issue: #1872)
- [!] Cold/warm tier index migration (Issue: #2407)
- [I] Index compression using sparse encoding (Issue: #1874)

## Implementation Phases

### Phase 1: Production-Grade Index Infrastructure (Status: Completed ✅)
- [x] HNSW vector similarity index (L2, Cosine, Dot Product) in `index/hnsw_index.cpp`
- [x] GPU-accelerated vector search via GPUVectorIndex (Vulkan, CUDA, HIP)
- [x] Product Quantization (PQ), Binary Quantization, and Residual Quantization
- [x] B-tree, range, sparse, and composite secondary indexes (`index/secondary_index_manager.cpp`)
- [x] R-tree spatial index with Z-order curves (`index/rtree_index.cpp`)
- [x] Graph indexing: adjacency lists, BFS/DFS traversal (`index/graph_index_manager.cpp`)
- [x] Adaptive index recommendations based on query patterns
- [x] IVF+PQ and FAISS integration, multi-vector search
- [x] GNN embeddings, temporal graphs, rotary embeddings
- [x] IndexManager with dependency injection to break circular dependencies
- [x] RocksDB persistence for vector indexes with WriteBatch atomicity
- [x] Audit logging for vector operations

### Phase 2: Full-Text & Automated Advisor (Status: In Progress 🚧)
- [x] Full-text inverted index integration (Target: Q2 2026)
- [~] Automated index advisor with workload replay (Target: Q2 2026)
- [x] HNSW incremental re-indexing without full rebuild (Target: Q3 2026)

### Phase 3: Learned Structures & GPU Build (Status: Planned 📋)
- [I] DiskANN / ScaNN alternative ANN algorithms for on-disk indexes (Issue: #1876)
- [ ] Learned index structures (ML-based B-tree replacement)
- [I] GPU-accelerated index build for large-scale vector datasets (Issue: #1878)
- [ ] Distributed vector index across shards
- [ ] Partial / filtered indexes on secondary index manager
- [ ] Cold/warm tier index migration

## Production Readiness Checklist
- [I] Unit tests coverage > 80% (Issue: #1882)
- [I] Integration tests (HNSW recall@10, spatial correctness) (Issue: #1883)
- [I] Performance benchmarks (QPS, recall, memory) (Issue: #1884)
- [I] Security audit (GPU memory safety, RocksDB key prefix isolation) (Issue: #1885)
- [I] Documentation complete (Issue: #1886)
- [I] API stability guaranteed (Issue: #1887)

## Known Issues & Limitations
- GPU acceleration requires Vulkan/CUDA drivers; falls back to CPU automatically.
- HNSW incremental re-indexing implemented via `VectorIndexManager::incrementalReindex()`.
- Full-text search is implemented via `InvertedIndex` (standalone) and `SecondaryIndexManager` (integrated).

## Breaking Changes
- `IndexManager` factory API (`createDefault()`) is stable from v1.x.
- GPU index configuration struct may gain new fields in v2.0; additive only.
