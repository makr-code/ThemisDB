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
- [I] Full-text inverted index integration (Target: Q2 2026) (Issue: #1433)
- [I] Automated index advisor with workload replay (Target: Q2 2026) (Issue: #1434)
- [P] HNSW incremental re-indexing without full rebuild (Target: Q3 2026) (Issue: #1435)

## Planned Features 📋

### Short-term (Next 3-6 months)
- [ ] DiskANN / ScaNN alternative ANN algorithms
- [ ] Index statistics export to metadata module
- [ ] Partial / filtered indexes on secondary index manager
- [ ] Online index rebuild with minimal read impact
- [ ] Configurable GPU memory budget per index

### Long-term (6-12 months)
- [ ] Distributed vector index across shards
- [ ] Learned index structures (ML-based B-tree replacement)
- [ ] Multi-tenancy index isolation
- [ ] Cold/warm tier index migration
- [ ] Index compression using sparse encoding

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
- [~] Full-text inverted index integration (Target: Q2 2026)
- [~] Automated index advisor with workload replay (Target: Q2 2026)
- [x] HNSW incremental re-indexing without full rebuild (Target: Q3 2026)

### Phase 3: Learned Structures & GPU Build (Status: Planned 📋)
- [ ] DiskANN / ScaNN alternative ANN algorithms for on-disk indexes
- [ ] Learned index structures (ML-based B-tree replacement)
- [ ] GPU-accelerated index build for large-scale vector datasets
- [ ] Distributed vector index across shards
- [ ] Partial / filtered indexes on secondary index manager
- [ ] Cold/warm tier index migration

## Production Readiness Checklist
- [ ] Unit tests coverage > 80%
- [ ] Integration tests (HNSW recall@10, spatial correctness)
- [ ] Performance benchmarks (QPS, recall, memory)
- [ ] Security audit (GPU memory safety, RocksDB key prefix isolation)
- [ ] Documentation complete
- [ ] API stability guaranteed

## Known Issues & Limitations
- GPU acceleration requires Vulkan/CUDA drivers; falls back to CPU automatically.
- HNSW incremental re-indexing implemented via `VectorIndexManager::incrementalReindex()`.
- Full-text search is out of scope for the current release.

## Breaking Changes
- `IndexManager` factory API (`createDefault()`) is stable from v1.x.
- GPU index configuration struct may gain new fields in v2.0; additive only.
