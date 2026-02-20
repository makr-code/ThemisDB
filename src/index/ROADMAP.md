# Index Module Roadmap

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
- [ ] Full-text inverted index integration (Target: Q2 2026)
- [ ] Automated index advisor with workload replay (Target: Q2 2026)
- [ ] HNSW incremental re-indexing without full rebuild (Target: Q3 2026)

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

## Production Readiness Checklist
- [ ] Unit tests coverage > 80%
- [ ] Integration tests (HNSW recall@10, spatial correctness)
- [ ] Performance benchmarks (QPS, recall, memory)
- [ ] Security audit (GPU memory safety, RocksDB key prefix isolation)
- [ ] Documentation complete
- [ ] API stability guaranteed

## Known Issues & Limitations
- GPU acceleration requires Vulkan/CUDA drivers; falls back to CPU automatically.
- HNSW rebuild is currently full-rebuild; incremental updates planned.
- Full-text search is out of scope for the current release.

## Breaking Changes
- `IndexManager` factory API (`createDefault()`) is stable from v1.x.
- GPU index configuration struct may gain new fields in v2.0; additive only.
