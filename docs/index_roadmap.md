# Index Module – Production Readiness Review & Roadmap

## Overview

This document reviews the production readiness of the `src/index` module (IndexManager and its sub-managers), identifies concrete gaps, and proposes a phased roadmap with prioritized implementation steps. It draws on `src/index/README.md`, `src/index/FUTURE_ENHANCEMENTS.md`, `docs/ERROR_HANDLING_MIGRATION.md`, `docs/QUERYENGINE_MIGRATION_PROGRESS.md`, `docs/BUILD_PERFORMANCE_OPTIMIZATIONS.md`, and related phase-implementation guides.

---

## 1. Current Status Summary

| Component | Maturity | Since | Notes |
|-----------|----------|-------|-------|
| **VectorIndexManager** (HNSW, PQ, IVF+PQ) | ✅ Production | v1.3.0 | GPU acceleration (Vulkan) in Beta |
| **SecondaryIndexManager** (B-tree, range, composite, sparse, geo, TTL) | ✅ Production | v1.0.0 | Full-text search not yet implemented |
| **GraphIndexManager** (BFS/DFS, shortest path, analytics) | ✅ Production | v1.2.0 | Distributed traversal not implemented |
| **SpatialIndexManager** (R-tree, Z-order, GeoJSON) | ⚠️ Beta | v1.4.0 | Exact polygon intersection not implemented |
| **AdaptiveIndexManager** (query pattern analysis, auto-recommendations) | ⚠️ Beta | v1.5.0 | Index registry check stubbed (TODO in adaptive_index.cpp) |
| **GPU Acceleration** (Vulkan, CUDA, HIP) | ⚠️ Beta | v1.3.0 | HIP backend not implemented; CUDA backend not implemented in current PR; serialization not yet implemented |
| **Multi-GPU VectorIndex** | ⚠️ Beta | v1.5.0 | Parallel batch processing across GPUs not implemented (TODO in multi_gpu_vector_index.cpp) |

---

## 2. Known Gaps

### 2.1 Partial / Stub Implementations

- **HIP backend** (`gpu_vector_index.cpp`): Falls back to CPU silently; full ROCm implementation is pending.
- **CUDA backend** (`gpu_vector_index.cpp`): Marked as "not implemented in this PR"; falls back to CPU.
- **GPU index serialization/deserialization** (`gpu_vector_index.cpp`): Two `TODO: Implement serialization` markers; index state cannot be persisted across restarts.
- **Multi-GPU parallel batching** (`multi_gpu_vector_index.cpp`): Parallel batch processing across GPUs stubbed; GPU utilization tracking returns 0.
- **Adaptive index registry check** (`adaptive_index.cpp`): `TODO: Check actual index registry` – recommendations may duplicate existing indexes.
- **BaseEntity string-array support** (`property_graph.cpp`): `TODO: Extend BaseEntity to support string arrays` – property graph multi-value properties unsupported.

### 2.2 Infrastructure-Ready Optimizations Not Yet Enabled

All flags below report status 🟡 "Infrastructure ready, implementation pending" in `docs/BUILD_PERFORMANCE_OPTIMIZATIONS.md`. They exist as CMake options and config flags but activate no additional code paths:

| Optimization | CMake Flag | Expected Gain |
|--------------|-----------|---------------|
| RCU Index (lock-free reads) | `THEMIS_ENABLE_RCU_INDEX` | +200–500% read-heavy workloads |
| LIRS Cache (better eviction) | `THEMIS_ENABLE_LIRS_CACHE` | +30–40% cache hit rate |
| WiscKey (key/value separation) | `THEMIS_ENABLE_WISCKEY` | +40–60% write throughput |
| DiskANN (billion-scale vectors) | `THEMIS_ENABLE_DISKANN` | +300–400% for >100M vectors |
| Bw-Tree (lock-free index) | `THEMIS_ENABLE_BW_TREE` | +100–200% index update throughput |
| Mimalloc allocator | `THEMIS_ENABLE_MIMALLOC` | +10–20% overall |
| Huge Pages | `THEMIS_ENABLE_HUGE_PAGES` | +15–30% memory-intensive |

### 2.3 Missing Graph Algorithm Feature Parity

- **PageRank / Community Detection / Centrality**: Implemented in `GraphAnalytics` but not distributed; single-node only.
- **GPU Graph Analytics (Gunrock)**: Planned for Phase 3 in `docs/PHASE3_IMPLEMENTATION_GUIDE.md`; not yet implemented.
- **Distributed Graph Traversal**: Planned for v1.7.0 per `src/index/README.md`; no scatter-gather for cross-shard BFS/DFS.
- **Cycle Detection**: Limited and expensive on large graphs; no workaround beyond `max_depth`.

### 2.4 Missing Parallel Query Execution

No parallel/concurrent query execution exists within the index module. Searches are single-threaded per request. No SIMD-parallel distance computation is wired into the default build path (AVX-512/NEON optimizations documented in `FUTURE_ENHANCEMENTS.md` but not enabled by default).

### 2.5 Full-Text Search Not Implemented

`SecondaryIndexManager` supports only equality and range queries. A full inverted-index (TF-IDF/BM25) implementation is planned for v1.7.0 per `src/index/FUTURE_ENHANCEMENTS.md` but does not exist yet.

### 2.6 Distributed Index Partitioning / Sharding

No index sharding or scatter-gather exists. `IndexManager::ShardingConfig` and `enableSharding()` are documented in `FUTURE_ENHANCEMENTS.md` as a v1.7.0 target but are not implemented.

### 2.7 HNSW Hard Deletion

Vectors cannot be removed from the HNSW graph; the current workaround is a soft-delete bitmap. Graph repair after deletion and automated rebuild-on-threshold are listed as planned for v1.7.0.

---

## 3. Error-Handling Migration Status

The codebase is migrating from legacy `bool` / `std::pair<Status, T>` patterns to the standardized `Result<T>` / `ErrorCode` infrastructure (see `docs/ERROR_HANDLING_MIGRATION.md`).

| Component | Status |
|-----------|--------|
| `IStorageEngine` interface | ✅ Migrated |
| `StorageEngine` implementation | ✅ Migrated |
| `PluginManager` | ✅ Migrated |
| `IndexManager` interface (`include/index/index_manager.h`) | ✅ Partially migrated – all public methods return `Result<T>` |
| `QueryEngine` (18 methods) | ✅ Migrated (Phase 2, per `docs/QUERYENGINE_MIGRATION_PROGRESS.md`) |
| **`VectorIndexManager`** | ⏳ Pending – still uses custom `Status` struct |
| **`SecondaryIndexManager`** | ⏳ Pending – still uses custom `Status` struct |
| **`GraphIndexManager`** | ⏳ Pending – still uses custom `Status` struct |
| RPC handlers | ⏳ Pending |
| API handlers | ⏳ Pending |

Phase 3 (index manager implementations) and Phase 4 (RPC/API handlers) of the migration are not yet started for the `src/index` code paths.

---

## 4. Observability Gaps

The index module currently exposes no Prometheus counters, OpenTelemetry spans, or structured health endpoints. Specific gaps:

- **No Prometheus metrics**: Query latency histograms, throughput counters, cache hit rates, index size gauges, and error-rate counters for each index manager are absent. The broader `docs/PROMETHEUS_INTEGRATION_COMPLETE.md` and `docs/GRAFANA_METRICS_COMPLETE.md` documents describe infrastructure-level metrics but do not cover per-index-type granularity.
- **No OpenTelemetry tracing**: No distributed spans for HNSW construction or search, R-tree traversal, BFS/DFS, or adaptive recommendation operations.
- **No index heatmaps or usage dashboards**: The `AdaptiveIndexManager` tracks internal query patterns but does not export them as observable signals.
- **No progress metrics for long-running operations**: Index rebuild, reindex, and defragmentation do not emit progress counters or ETAs.
- **No health/readiness probes per index**: No per-manager liveness or readiness signal for use by health-check endpoints.

---

## 5. Admin / Ops Gaps

| Gap | Current State | Impact |
|-----|--------------|--------|
| **Online index migration** | Not implemented | Schema changes require downtime |
| **Rebuild/reindex API with progress** | No progress reporting; no API endpoint | Operators cannot monitor long-running rebuilds |
| **Defragmentation** | No defrag operation for fragmented HNSW or R-tree nodes | Performance degrades over time with many deletes |
| **TTL cleanup automation** | `cleanupExpired()` must be called manually | Expired entries accumulate unless application schedules cleanup |
| **Backup/restore integration** | Index state not included in documented backup procedures | Index data may not be consistently backed up alongside primary data |
| **Admin endpoint: list/stats** | Internal `getStats()` exists per manager; not exposed via HTTP/gRPC admin API | Operators cannot inspect index health from outside the process |

---

## 6. Security / Isolation Gaps

- **No tenant-scoped guards**: All index managers accept arbitrary collection/table names without tenant isolation checks. A multi-tenant deployment has no enforcement boundary at the index layer.
- **No per-index ACL**: No access-control checks on individual index operations (create, drop, search, rebuild).
- **No resource quotas per index/tenant**: No limits on index memory usage, number of vectors per tenant, or query rate per tenant. This is listed as a concern in `docs/TENANT_ISOLATION_GUIDE.md` but not implemented in the index module.
- **No index-operation audit trail**: Vector and graph index operations are not emitted to the audit log system (unlike storage operations which have audit hooks).

---

## 7. Feature Gaps

| Feature | Planned Version | Status |
|---------|----------------|--------|
| Full-text search index (BM25/TF-IDF) | v1.7.0 | Not started; spec in `src/index/FUTURE_ENHANCEMENTS.md` |
| Distributed index partitioning / sharding (scatter-gather) | v1.7.0 | Not started; spec in `src/index/FUTURE_ENHANCEMENTS.md` |
| Parallel query execution (intra-query parallelism) | Not assigned | Not started |
| GPU graph analytics (Gunrock integration) | Phase 3 | Header interface defined; core implementation pending |
| DiskANN (billion-scale vector search) | Phase 3 | Header interface defined; greedy-search and SSD I/O pending |
| Bw-Tree (lock-free secondary index) | Phase 3 | Header interface defined; delta-chain implementation pending |
| HNSW hard deletion with graph repair | v1.7.0 | Not started; workaround: soft-delete bitmap |
| Exact polygon intersection (GEOS) | v1.7.0 | Currently uses MBR approximation only |
| GPU memory oversubscription | v1.6.0 | Not started; VRAM limit workarounds only |
| Adaptive HNSW parameter tuning | v1.6.0 | Parameter tuner exists (`hnsw_parameter_tuner.cpp`) but not wired to runtime monitoring |
| Unified `IIndex` interface for polymorphism | v1.6.0 | Described in `FUTURE_ENHANCEMENTS.md`; not yet refactored |
| Index metadata registry (centralized) | v1.6.0 | `AdaptiveIndexManager` has partial tracking; no unified registry |

---

## 8. Testing / Benchmarks Gaps

- **No fuzz tests** for index operations (vector insert/search, B-tree key encoding, R-tree node splitting, graph traversal with adversarial inputs). The `fuzz/` directory exists at the repository root but does not cover the index module.
- **No chaos / crash-recovery tests**: No tests verifying that partially-written HNSW state, R-tree splits, or secondary-index batches can be recovered after a crash mid-write.
- **No online-migration tests**: No end-to-end test for modifying an index definition while reads and writes are in flight.
- **No large-dataset benchmarks**: Documented performance numbers (HNSW: 0.1–1ms, GPU: 200K+ QPS) lack automated regression benchmarks for large datasets (>10M vectors) in CI.
- **No GPU/CPU fallback correctness tests**: GPU backends falling back to CPU silently; no test verifies result equivalence.
- **No multi-tenant isolation tests**: No tests verifying that data from different tenants cannot be accessed across index boundaries.
- **No SIMD correctness tests**: AVX-512/NEON distance functions (documented in `FUTURE_ENHANCEMENTS.md`) lack cross-platform parity tests.

---

## 9. Prioritized Roadmap

The phases below are ordered by risk reduction and operational impact. Each phase is independent and can be started in parallel by different owners.

### Phase 1 – Stability & Security (Target: next 1–2 months)

**Goal**: Eliminate known data-safety risks and close the most critical security gaps.

| # | Task | Reference | Owner |
|---|------|-----------|-------|
| 1.1 | Implement GPU index serialization/deserialization | `gpu_vector_index.cpp` TODOs | TBD |
| 1.2 | Migrate `VectorIndexManager`, `SecondaryIndexManager`, `GraphIndexManager` to `Result<T>` (Error Handling Phase 3) | `docs/ERROR_HANDLING_MIGRATION.md` | TBD |
| 1.3 | Add per-index ACL checks and tenant-scope guards to `IndexManager` | `docs/TENANT_ISOLATION_GUIDE.md` | TBD |
| 1.4 | Wire audit-log hooks into vector/graph index operations | `docs/TASK_AUDIT_EVENTS.md` | TBD |
| 1.5 | Fix adaptive index registry check to avoid duplicate-index recommendations | `adaptive_index.cpp` TODO | TBD |

### Phase 2 – Observability & Ops (Target: 2–4 months)

**Goal**: Enable operators to monitor index health and perform administrative operations safely.

| # | Task | Reference | Owner |
|---|------|-----------|-------|
| 2.1 | Expose Prometheus metrics: per-manager latency histograms, throughput counters, cache hit rates, error rates, index size gauges | `docs/PROMETHEUS_INTEGRATION_COMPLETE.md` | TBD |
| 2.2 | Add OpenTelemetry spans for HNSW search/construction, R-tree traversal, BFS/DFS | `docs/tracing-configuration.md` | TBD |
| 2.3 | Add admin HTTP/gRPC endpoints: list indexes, get stats, trigger rebuild, get rebuild progress | — | TBD |
| 2.4 | Automate TTL cleanup via the task scheduler (`SecondaryIndexManager::cleanupExpired`) | `docs/TASK_SCHEDULER_CRON_CDC.md` | TBD |
| 2.5 | Emit progress metrics (current/total items, ETA) for rebuild, reindex, and defragmentation operations | — | TBD |
| 2.6 | Add per-index liveness/readiness probes for health-check endpoints | — | TBD |

### Phase 3 – Performance & Scale (Target: 4–9 months)

**Goal**: Activate infrastructure-ready optimizations and enable large-scale deployments.

| # | Task | Reference | Owner |
|---|------|-----------|-------|
| 3.1 | Implement RCU Index (`THEMIS_ENABLE_RCU_INDEX`) for lock-free read paths | `docs/BUILD_PERFORMANCE_OPTIMIZATIONS.md`, `docs/PHASE1_RCU_INDEX_IMPLEMENTATION.md` | TBD |
| 3.2 | Implement LIRS Cache (`THEMIS_ENABLE_LIRS_CACHE`) for better eviction policy | `docs/BUILD_PERFORMANCE_OPTIMIZATIONS.md` | TBD |
| 3.3 | Implement WiscKey (`THEMIS_ENABLE_WISCKEY`) for large-value separation | `docs/BUILD_PERFORMANCE_OPTIMIZATIONS.md` | TBD |
| 3.4 | Implement DiskANN (`THEMIS_ENABLE_DISKANN`) – greedy search, SSD I/O, LRU cache for billion-scale vectors | `docs/PHASE3_IMPLEMENTATION_GUIDE.md` | TBD |
| 3.5 | Implement Bw-Tree (`THEMIS_ENABLE_BW_TREE`) – lock-free delta-chain B-tree for secondary indexes | `docs/PHASE3_IMPLEMENTATION_GUIDE.md` | TBD |
| 3.6 | Implement HNSW hard deletion with graph repair and auto-rebuild-on-threshold | `src/index/FUTURE_ENHANCEMENTS.md` | TBD |
| 3.7 | Implement CUDA and HIP GPU backends for `GPUVectorIndex` | `gpu_vector_index.cpp` | TBD |
| 3.8 | Implement parallel batch processing across GPUs in `MultiGPUVectorIndex` | `multi_gpu_vector_index.cpp` TODO | TBD |
| 3.9 | Enable default SIMD (AVX-512/NEON) distance computation for CPU search path | `src/index/FUTURE_ENHANCEMENTS.md` | TBD |

### Phase 4 – Feature Delivery (Target: 6–12 months)

**Goal**: Deliver planned features that expand index capabilities.

| # | Task | Reference | Owner |
|---|------|-----------|-------|
| 4.1 | Implement full-text search index (BM25/TF-IDF, tokenizer, stemmer) | `src/index/FUTURE_ENHANCEMENTS.md` | TBD |
| 4.2 | Implement distributed index partitioning and scatter-gather query execution | `src/index/FUTURE_ENHANCEMENTS.md` | TBD |
| 4.3 | Implement intra-query parallelism (thread pool dispatch for HNSW search, R-tree scan) | — | TBD |
| 4.4 | Implement GPU graph analytics (Gunrock integration) | `docs/PHASE3_IMPLEMENTATION_GUIDE.md` | TBD |
| 4.5 | Implement exact polygon intersection via GEOS library | `src/index/FUTURE_ENHANCEMENTS.md` | TBD |
| 4.6 | Implement `BaseEntity` string-array support for multi-value property graph edges | `property_graph.cpp` TODO | TBD |
| 4.7 | Implement unified `IIndex` interface and centralized index metadata registry | `src/index/FUTURE_ENHANCEMENTS.md` | TBD |
| 4.8 | Implement GPU memory oversubscription (CUDA Unified Memory paging, LRU eviction) | `src/index/FUTURE_ENHANCEMENTS.md` | TBD |

### Phase 5 – Tests & CI (Ongoing, begin immediately alongside Phase 1)

**Goal**: Establish safety nets that prevent regressions and validate new implementations.

| # | Task | Reference | Owner |
|---|------|-----------|-------|
| 5.1 | Add fuzz tests for index key encoding, HNSW insert/search, R-tree split, graph traversal | `fuzz/` | TBD |
| 5.2 | Add crash-recovery tests: verify index consistency after mid-write process kill | — | TBD |
| 5.3 | Add online-migration tests: concurrent reads/writes while index definition changes | — | TBD |
| 5.4 | Add large-dataset benchmarks (>10M vectors) to CI with regression gates | `docs/BENCHMARK_RUNBOOK.md` | TBD |
| 5.5 | Add GPU/CPU fallback correctness tests (result equivalence across backends) | — | TBD |
| 5.6 | Add multi-tenant isolation tests for index operations | `docs/TESTING_AND_BENCHMARKING_GUIDE.md` | TBD |
| 5.7 | Add SIMD correctness tests (AVX-512 vs generic path) | — | TBD |
| 5.8 | Achieve >80% line coverage for `index_manager.cpp`, `vector_index.cpp`, `graph_index.cpp`, `secondary_index.cpp` | — | TBD |

---

## 10. References

| Document | Path | Relevance |
|----------|------|-----------|
| Index module README | `src/index/README.md` | Component status, known limitations, performance characteristics |
| Index future enhancements | `src/index/FUTURE_ENHANCEMENTS.md` | Planned features: full-text, distributed partitioning, GPU oversubscription |
| Error handling migration guide | `docs/ERROR_HANDLING_MIGRATION.md` | `Result<T>` patterns, migration checklist, pending phases |
| Error handling migration summary | `docs/ERROR_HANDLING_MIGRATION_SUMMARY.md` | Completed and in-progress migrations |
| QueryEngine migration progress | `docs/QUERYENGINE_MIGRATION_PROGRESS.md` | Phase 2 (QueryEngine) completed; Phase 3 (index managers) pending |
| Performance optimizations build guide | `docs/BUILD_PERFORMANCE_OPTIMIZATIONS.md` | RCU, LIRS, WiscKey, DiskANN, Bw-Tree status and flags |
| Phase 3 implementation guide | `docs/PHASE3_IMPLEMENTATION_GUIDE.md` | DiskANN, Bw-Tree, GPU graph analytics implementation plans |
| Tenant isolation guide | `docs/TENANT_ISOLATION_GUIDE.md` | Multi-tenant security requirements |
| Benchmark runbook | `docs/BENCHMARK_RUNBOOK.md` | Performance testing procedures |
| Prometheus integration | `docs/PROMETHEUS_INTEGRATION_COMPLETE.md` | Existing metrics infrastructure |
| Tracing configuration | `docs/tracing-configuration.md` | OpenTelemetry setup reference |
| Task scheduler | `docs/TASK_SCHEDULER_CRON_CDC.md` | Automation hooks for TTL cleanup |
