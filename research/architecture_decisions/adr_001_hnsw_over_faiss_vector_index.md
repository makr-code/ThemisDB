# ADR-001: HNSW over FAISS for ANN Vector Index

**Status:** Accepted  
**Date:** 2023-06-01  
**Deciders:** @themisdb-core-team  
**Modules Affected:** `src/index/`  
**Related Research:** [HNSW: Efficient ANN (2020)](../papers/hnsw_efficient_ann_2020.md)

---

## Context

ThemisDB requires an approximate nearest-neighbor (ANN) index to power vector similarity search across embedding workloads. Target requirements at decision time were:

- Support ≥ 1 million 768-dimensional float32 vectors in a single index.
- Recall@10 ≥ 0.95 across standard benchmark corpora (SIFT-1M, GloVe-100).
- p99 query latency < 10 ms on commodity CPU hardware (no GPU required at runtime).
- Incremental insert/delete without requiring a full index rebuild.
- Open-source license compatible with ThemisDB's Apache 2.0 license.
- Must integrate with the RocksDB persistence layer (ADR-002) for serialization.

The vector index feeds the RAG pipeline (`src/rag/`) and multi-model query executor (`src/query/`), so API stability and testability were secondary drivers.

## Decision Drivers

- **Recall target:** Recall@10 ≥ 0.95 at production scale (≥ 1M 768-dim vectors).
- **Latency target:** p99 < 10 ms on x86-64 CPU with AVX2; no GPU dependency at query time.
- **Incremental updates:** Index must accept insert and delete operations without full rebuild to support real-time ingestion.
- **No BLAS/LAPACK dependency:** ThemisDB's build system uses vcpkg; heavy BLAS dependencies complicate cross-compilation and container images.
- **License:** Apache 2.0 or MIT to avoid GPL/commercial-restriction issues.
- **Thread safety:** Concurrent reads with serialized writes must be supported for the multi-threaded query executor.

## Considered Options

| Option | Pros | Cons |
|--------|------|------|
| **HNSW (hnswlib)** | Sub-ms single-query latency; incremental insert/delete; header-only C++ (easy vcpkg integration); Apache 2.0; excellent recall@10 | Higher memory footprint than flat indexes (graph edges stored per node); no GPU path |
| **FAISS (Facebook AI)** | Battle-tested; IVF variants reduce memory; GPU acceleration via CUDA | Requires BLAS/LAPACK; IVF flat-index must be fully rebuilt after significant additions; GPU path needed for best perf; FAISS license (MIT) but GPU extension is separate |
| **ScaNN (Google)** | Highest throughput on large batches; strong recall | C++ only, limited public vcpkg support; Apache 2.0 but complex build; no incremental updates |
| **Annoy (Spotify)** | Simple API; memory-mapped files (read-only sharing) | Read-only after build (no inserts/deletes); lower recall at high dimensions than HNSW; Python-first API |

## Decision

**Chosen: HNSW via hnswlib**

hnswlib satisfies every decision driver:

1. **Recall:** Achieves Recall@10 ≥ 0.96 on SIFT-1M and ≥ 0.97 on GloVe-100-angular with `M=16, ef_construction=200, ef_search=100` — exceeding the 0.95 requirement.
2. **Latency:** Single-query p99 measured at 2–4 ms on an AMD EPYC 7443 with 768-dim vectors and 1M nodes, well below the 10 ms budget.
3. **Incremental updates:** `hnsw_index->addPoint()` and `hnsw_index->markDeleted()` operate without rebuild; periodic compaction is scheduled offline.
4. **No BLAS:** hnswlib is a header-only library with no external linear algebra dependencies beyond AVX2 SIMD intrinsics.
5. **License:** Apache 2.0 — fully compatible.
6. **Persistence:** The index is serialized via `hnsw_index->saveIndex()` into a RocksDB value blob keyed by `index_id`, enabling MVCC snapshots alongside other data.

FAISS was rejected primarily because its IVF variants require a full `train()` pass when the data distribution shifts significantly, and its optimal performance depends on CUDA (violating the no-GPU-at-query-time driver). ScaNN's build complexity and absence from vcpkg registries made it impractical for the CI pipeline.

## Consequences

### Positive
- Sub-millisecond ANN queries on commodity hardware enable low-latency RAG pipelines.
- Header-only integration keeps the build system simple and cross-platform (Linux/macOS/Windows CI all pass).
- Incremental inserts support real-time vector ingestion without index downtime.
- Thread-safe concurrent reads allow the query executor to saturate CPU cores during batch retrieval.

### Negative / Trade-offs
- **Memory overhead:** HNSW stores up to `M * 2` neighbor pointers per node. A 1M-vector index with `M=16` consumes ~1 GB of graph structure plus the raw vectors. *Mitigation: configurable `M` per index; large indexes use a dedicated memory-mapped arena.*
- **No GPU acceleration:** For batch workloads > 10K queries/sec, CPU-only HNSW may become a bottleneck. *Accepted because: GPU path can be added via a separate FAISS-GPU adapter behind the same `IVectorIndex` interface without changing the decision.*
- **Soft deletes only:** `markDeleted()` does not reclaim memory until a rebuild. *Mitigation: scheduled offline compaction rebuilds indexes when deleted fraction exceeds 20 %.*

### Neutral
- The `IVectorIndex` interface in `src/index/` abstracts over the HNSW implementation, allowing future substitution of backends (e.g., FAISS for GPU nodes) without API changes.
- Index parameters (`M`, `ef_construction`, `ef_search`) are runtime-configurable via `config/index.toml`.

## Validation

- [x] Prototype built and benchmarked against SIFT-1M and GloVe-100
- [x] Recall@10 ≥ 0.95 verified on both corpora
- [x] p99 < 10 ms verified on EPYC 7443 single-socket
- [x] Incremental insert/delete tested with 100K random ops
- [x] RocksDB serialization round-trip tested (save → reload → query parity)
- [ ] Integration tests for concurrent read + write scenarios (tracked: `tests/index/`)
- [ ] Module README in `src/index/` updated with parameter tuning guide

## Follow-up Actions

- [ ] Add HNSW parameter auto-tuning based on index size and recall feedback (`src/index/hnsw_tuner.cpp`).
- [ ] Implement background compaction thread to reclaim soft-deleted node memory.
- [ ] Expose `IVectorIndex` mock in test fixtures for downstream unit tests (`tests/mocks/`).
- [ ] Benchmark ScaNN as a future GPU-optional alternative when the batch-query load exceeds 10K QPS.

## Related Decisions

- [ADR-002: RocksDB as Primary Persistent Storage Backend](adr_002_rocksdb_storage_backend.md)
- [ADR-004: Native Multi-Model Data Model](adr_004_multi_model_data_model.md)

---
**Last Updated:** 2026-04-06
