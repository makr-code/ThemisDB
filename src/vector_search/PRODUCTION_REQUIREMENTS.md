# Vector Search Module - Production Requirements

<!-- Status: current | validated: 2026-09-22 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md · SECURITY.md -->

## Purpose and Scope

This document defines **mandatory production requirements** for the vector search module. It specifies operational constraints, integration boundaries, performance gates, and deployment prerequisites.

## Document Boundaries (Canonical Split)

- **`src/vector_search/PRODUCTION_REQUIREMENTS.md` (this document):** Mandatory production constraints (MUST/MUST NOT), operational limits, deployment gates, current evidence.
- **`src/vector_search/README.md`:** Module overview, scope, interfaces, quickstart.
- **`src/vector_search/ROADMAP.md`:** Implementation phases 1-6, completion status, production readiness.
- **`src/vector_search/FUTURE_ENHANCEMENTS.md`:** Planned features, performance targets, research directions.
- **`src/vector_search/ARCHITECTURE.md`:** Design principles, components, data flow, concurrency model.
- **`src/vector_search/CHANGELOG.md`:** Version history and delivered artefacts.

## Mandatory Vector Search Production Requirements

### 1. Index Correctness & Validation

**MUST:** All vectors ingested into the index must pass dimension and value validation.
- Evidence: `tests/vector_search/test_vector_search_highcardinality_stress.cpp` — dimension mismatch tests
- Status: ✅ ENFORCED
- Enforced via: E5400 (dimension mismatch), E5401 (NaN/inf detection)

**MUST NOT:** Allow NaN, infinity, or out-of-range floating-point values in vectors.
- Evidence: Vector validation in index construction paths
- Status: ✅ ENFORCED
- Error Code: E5401

**MUST:** Index dimension is fixed at creation time; all inserted vectors must match.
- Evidence: Index constructor sets immutable dimension; search validates dimension
- Status: ✅ ENFORCED
- Error Code: E5400 on mismatch

### 2. Search Accuracy & Recall

**MUST:** Approximate nearest neighbor (ANN) results must achieve ≥ 0.9 recall against brute-force baseline.
- Evidence: `tests/integration/test_vector_search_soak.cpp` — concurrent recall gate
- Status: ✅ VERIFIED
- Gate: `VectorSearchSoak_ConcurrentSearchReliability` (≥ 0.9)

**MUST:** Distance scores must be consistent and deterministic for identical queries on stable indices.
- Evidence: Benchmark reproducibility across multiple runs
- Status: ✅ VERIFIED
- Implication: No randomized distance computation; deterministic rounding behavior

**MUST NOT:** Return empty results without explicit E5403 error or valid empty-index condition.
- Status: ✅ ENFORCED
- Behavior: Return E5402 if index is empty; E5403 if search completed but found no results

### 3. Performance Gates

**MUST:** Insertion throughput must remain ≥ 1 000 vectors/sec (P95).
- Evidence: `benchmarks/vector_search/bench_vector_search_dedicated_gates.cpp` — VS-BM-01
- Status: ✅ VERIFIED
- Hardware: P95 on release-profile hardware

**MUST:** Query latency (k=10 neighbors, 128-dim) must remain ≤ 10 ms (P99).
- Evidence: `benchmarks/vector_search/bench_vector_search_dedicated_gates.cpp` — VS-BM-02
- Status: ✅ VERIFIED
- Condition: Single-threaded search on 1M-vector index

**MUST:** Concurrent search (4 threads) must maintain ≥ 500 ops/sec (P95) throughput.
- Evidence: `benchmarks/vector_search/bench_vector_search_dedicated_gates.cpp` — VS-BM-04
- Status: ✅ VERIFIED
- Regression tolerance: ≤ 10% vs. baseline

**Memory overhead must remain < 40% of raw vector storage.**
- Evidence: Benchmark memory profiling
- Status: ✅ VERIFIED
- Implication: 1M 128-dim vectors (~512 MB) + index ≤ ~717 MB (512 MB base + 40% ≈ 205 MB overhead)

### 4. Concurrency & Thread Safety

**MUST:** Multiple concurrent search operations must not corrupt index state.
- Evidence: `tests/integration/test_vector_search_soak.cpp` — concurrent stability test
- Status: ✅ VERIFIED
- Guarantee: Read-write mutex protection on all mutable state

**MUST NOT:** Concurrent modification (add/delete) and search must serialize; no parallel updates.
- Status: ✅ ENFORCED
- Behavior: Modifications acquire exclusive write lock; searches use shared read lock

**MUST:** Concurrent queries must not introduce deadlocks.
- Status: ✅ VERIFIED
- Locking Strategy: Consistent lock acquisition order; no nested write locks

### 5. Error Handling & Graceful Degradation

**MUST:** All error conditions return explicit error codes (E5400–E5499).
- Error Taxonomy:
  - E5400: Invalid vector dimension
  - E5401: Vector contains NaN or inf
  - E5402: Index is empty
  - E5403: Search returned no results
  - E5404: Index corruption detected
  - E5405: Invalid distance metric
  - **Note:** Out-of-memory conditions preserve the index and return caller-specific allocation error codes (not E5404).
- Status: ✅ DOCUMENTED

**MUST NOT:** Silently ignore errors or proceed with corrupted state.
- Status: ✅ ENFORCED
- Implication: Errors logged; callers notified; index preserved on failure

**MUST:** Out-of-memory conditions must not corrupt index; existing index remains valid.
- Status: ✅ ENFORCED via bounds checking
- Fallback: Reject new insertions; preserve existing index

### 6. Operational Bounds

**Index Size Limits:**
- Maximum vectors per index: Limited by available system memory
- Maximum vector dimension: 4096 (practical limit; runtime enforcement planned Phase 6)
  - **Note:** Currently no hard runtime check; relies on pre-validation. See MODULE_GAPS.md for tracking.
- Minimum vector dimension: 1
- Typical memory per vector: 0.5–2 KB (HNSW with M=16), 0.5–1 KB (IVF)
  - Example: 1M 128-dim float32 vectors (~512 MB base) + HNSW overhead (~40% = ~717 MB total)

**Search Operation Limits:**
- Maximum k (neighbors requested): 10 000 (validated at query time)
- Maximum query concurrency: ≥ 100 queries with < 5% latency overhead
- Maximum batch insert size: Depends on available memory; recommended ≤ 100k vectors per batch

**Index Rebuild Limits:**
- HNSW rebuild time: ~1 ms per 1 000 vectors (hardware-dependent)
- IVF rebuild time: ~5 ms per 1 000 vectors (includes k-means)
- Concurrent queries blocked during rebuild (exclusive write lock)

### 7. Integration Boundaries

**RAG Module Integration:**
- RAG queries are passed to vector search as embedding vectors
- Search results (document IDs, distances) returned to RAG
- **Boundary:** Vector search does not persist or cache RAG-specific metadata
- **Status:** ✅ ENFORCED via interface contracts

**Server Integration:**
- HTTP endpoints invoke vector search with validated distance metrics
- Query parameters (k, metric) validated before index access
- **Boundary:** Server is responsible for input sanitization
- **Status:** ✅ ENFORCED via error codes on invalid inputs

**Index Module Integration:**
- HNSW/IVF algorithm implementations live in `include/index/`
- Vector search acts as facade for high-level index operations
- **Boundary:** Algorithm details abstracted from consumers
- **Status:** ✅ ENFORCED via public API contracts

### 8. Deployment Prerequisites

**Required Dependencies:**
- C++ compiler with C++17 support (gcc 7+, clang 5+, MSVC 2017+)
- Optional: SIMD-capable processor (AVX2 recommended for performance)
- Optional: RocksDB (for index persistence; Phase 6+)

**Hardware Recommendations:**
- Minimum RAM: 1 GB (for small indices)
- Recommended RAM: ≥ 16 GB (for production indices)
- CPU: Multi-core (for concurrent search scaling)

**Configuration Requirements:**
- Index dimension must match embedding model output dimension
- Distance metric must match embedding similarity semantics (usually COSINE)
- HNSW M and ef parameters should be tuned per workload (defaults: M=16, ef_construction=200)

### 9. Monitoring & Observability

**MUST Track:**
- Insert/search operation latencies (P50, P95, P99)
- Index memory consumption (size and overhead)
- Cache hit/miss rates (if caching enabled)
- Error rates and error codes (E5400–E5499)

**MUST Alert On:**
- Error rate surge (> 1% of operations)
- Latency regression (> 10% vs. baseline)
- Memory pressure (> 80% of limit)
- Index corruption detection (E5404)

**Optional But Recommended:**
- Concurrent query count (for scaling analysis)
- Rebuild frequency and duration (for optimization tuning)
- Cache eviction rates (for capacity planning)

## Wave Model Integration

Vector search module is a **contributing module** in Wave A → B → C → D execution model.

**Wave D Status:** ✓ COMPLETE (2026-09-22)
- Soak tests delivered and passing
- Benchmark gates defined and measurable
- Production readiness checklist signed off

See [ROADMAP.md](ROADMAP.md) and [`../../ROADMAP.md`](../../ROADMAP.md) for full wave model details.

## Deployment Checklist

- [ ] Index dimension configured to match embedding model
- [ ] Distance metric selected (COSINE for typical embeddings)
- [ ] HNSW/IVF algorithm selected per use case
- [ ] Performance targets validated on production hardware
- [ ] Monitoring and alerting configured
- [ ] Concurrent load testing completed
- [ ] Graceful degradation tested (out-of-memory, invalid vectors)
- [ ] Error handling and logging verified
- [ ] Backup and recovery procedures documented

---

**Status:** Production-ready for deployment after checklist completion and maintainer sign-off.
