> ⚠️ **Historischer Auditbericht** – Befunde ohne aktuellen Codebeleg mit `<!-- TODO: add source file evidence -->` markieren. Veraltete Befunde entfernen.

<!-- Status: current | validated: 2026-04-19 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md -->

# Audit Report — Index Module

**Last Audit:** 2026-04-19
**Auditor:** Copilot
**Status:** ⚠️ Pass with findings

## Summary

| Metric | Result |
|--------|--------|
| Build System Registration | ✅ Verified |
| Source Files | 41 |
| Test Coverage | ⚠️ Unit tests present; GPU paths partially covered; integration and benchmark suites open (#1883, #1884) |
| Open TODOs | 0 explicit TODOs; 6 open tracking issues |
| Open Stubs | 0 |
| Security Issues | 1 open — GPU memory safety (#1885); separator injection fixed (#1872, 2026-04-07) |

## Build System

The index module is registered in `CMakeLists.txt` as the `themis_index` static library. Source files are listed explicitly per component group (HNSW core, GPU backends, quantisation, secondary indexes, advisor, full-text). The module links against: RocksDB, Vulkan SDK, CUDA toolkit, ROCm/HIP, DiskANN, ScaNN, and the internal thread-pool and tenant-context libraries. GPU backend compilation is guarded by CMake feature flags (`THEMIS_GPU_VULKAN`, `THEMIS_GPU_CUDA`, `THEMIS_GPU_HIP`).

## Source Files Audited

The 41 source files span the following component groups:

| Component | Key Files | Status |
|-----------|-----------|--------|
| Vector index core | `vector_index.cpp`, `advanced_vector_index.cpp`, `adaptive_index.cpp`, `ann_index.cpp`, `vector_auto_buffer.cpp` | ✅ Reviewed |
| GPU backends | `gpu_vector_index.cpp`, `gpu_vector_index_vulkan.cpp`, `gpu_memory_oversubscription.cpp`, `multi_gpu_vector_index.cpp`, `cuda_hnsw_graph_traversal.cpp` | ⚠️ HIP VRAM clear validation pending (#1878) |
| HNSW optimisation | `hnsw_layer_optimizer.cpp`, `hnsw_parameter_tuner.cpp`, `hnsw_production_defaults.cpp` | ✅ Reviewed |
| Quantisation | `binary_quantizer.cpp`, `product_quantizer.cpp`, `learned_quantizer.cpp`, `residual_quantizer.cpp` | ✅ Reviewed |
| Approximate search | `approximate_radius_search.cpp`, `multi_vector_search.cpp` | ✅ Reviewed |
| Graph indexing | `graph_index.cpp`, `graph_analytics.cpp`, `graph_auto_buffer.cpp`, `property_graph.cpp`, `edge_types.cpp` | ✅ Reviewed |
| Spatial / secondary indexes | `spatial_index.cpp`, `secondary_index.cpp`, `inverted_index.cpp` | ✅ Reviewed |
| Index management | `index_manager.cpp`, `index_compression.cpp`, `tiered_index_manager.cpp` | ✅ Reviewed |
| Embeddings / tensor ops | `rotary_embeddings.cpp`, `rotary_embeddings_gpu_cpu.cpp`, `rotary_embeddings_hip.cpp`, `learnable_rope.cpp`, `lora_rope.cpp`, `matryoshka_truncation.cpp`, `gnn_embeddings.cpp` | ✅ Reviewed |
| Graph network | `process_graph.cpp`, `temporal_graph.cpp`, `distributed_vector_index.cpp` | ⚠️ Incomplete — issue #1878 |
| Workload replay | `workload_replay.cpp` | ✅ Reviewed |

## Test Coverage

- Unit tests in `tests/index/` cover: HNSW correctness (recall@k), PQ/BQ/RQ encoding/decoding, B-tree insert/range query, R-tree spatial query, full-text BM25 ranking, tiered migration promotion/demotion.
- GPU unit tests cover Vulkan and CUDA backends using mocked device buffers; hardware-in-the-loop tests require a physical GPU.
- HIP backend has no hardware-in-the-loop tests in CI (no AMD GPU runner available).
- **Open: integration test suite** (issue #1883) — end-to-end tests covering multi-index concurrent queries are not yet merged.
- **Open: performance benchmark suite** (issue #1884) — recall@k vs. latency benchmarks for HNSW, DiskANN, ScaNN not yet automated in CI.
- Tenant key prefix isolation has targeted unit tests in `tests/index/tenant_isolation_test.cpp`.

## Findings

### Resolved
- **HNSW concurrent insert connectivity** — Fixed in v1.5.0; locking strategy changed to per-layer read-write lock.
- **Vulkan descriptor set leak** — Fixed in v1.3.0; descriptor set lifetime bound to index object.
- **CUDA stream synchronisation race** — Fixed in v1.3.0; explicit stream synchronisation added before stream result copy-back.
- **PQ sub-vector assignment error** — Fixed in v1.4.0; codebook assignment now uses vectorised distance computation.
- **R-tree Z-order MBR split** — Fixed in v1.6.0; Morton code boundary condition corrected for high-dimensional inputs.
- **[#1872] Multi-tenancy key separator injection** — Fixed 2026-04-07: `isValidTenantComponent()` added to `src/index/index_manager.cpp`; all 10 tenant-scoped entry points (`createSecondaryIndex`, `createVectorIndex`, `createGraphIndex`, `getSecondaryIndex`, `getVectorIndex`, `getGraphIndex`, `dropIndex`, `dropTenantIndexes`, `listIndexes`, `getIndexType`) now reject `tenant_id` or `index_name` values that contain `:`, null bytes, are empty, or exceed 512 bytes. 15 new regression tests added to `tests/test_multi_tenant_index.cpp` (`MultiTenantInjectionSecurity` suite).

### Open

#### 🔴 [#1885] Security audit — GPU memory safety and key prefix isolation
- A formal security audit of VRAM secure-clear correctness and RocksDB key prefix isolation has not been completed.
- **Severity:** High (multi-tenancy guarantee is unverified without audit)
- **Action:** Conduct structured security review covering: VRAM clear on all eviction paths (Vulkan, CUDA, HIP), iterator range bounding in RocksDB, and cross-tenant handle leakage in IndexManager.

#### ⚠️ [#1878] GPU build and VRAM clear on HIP/ROCm
- HIP backend VRAM secure-clear has not been validated on ROCm < 5.4.
- Multi-GPU distributed index shard rebalancing has partial implementation.
- **Severity:** Medium
- **Action:** Add ROCm version check at startup; disable HIP VRAM clear optimisation path on unsupported versions.

#### ⚠️ [#1882] Unit test coverage gaps
- `ann_index.cpp` (which implements both ScaNN and DiskANN backends via `include/index/ann_index.h`) has limited unit test coverage; edge cases for empty indexes, single-element indexes, and concurrent access are not tested.
- **Action:** Add parametrised unit tests for boundary conditions.

#### ⚠️ [#1883] Integration test suite not merged
- End-to-end multi-index concurrent query tests are in a feature branch but not merged.
- **Action:** Review and merge; add to CI gate.

#### ⚠️ [#1884] Performance benchmark suite not in CI
- Recall@k vs. latency benchmarks for HNSW, DiskANN, ScaNN are not automated.
- **Action:** Add benchmark harness to CI with performance regression detection.

#### ℹ️ [#1886] Documentation gaps
- DiskANN and ScaNN configuration parameters are not yet documented in the module README.
- **Action:** Document all configurable parameters with defaults and valid ranges.

#### ℹ️ [#1887] API stability not formally declared
- Public IndexManager API has not been marked as stable; downstream callers cannot rely on ABI/API stability guarantees.
- **Action:** Review public API surface, mark stable interfaces, and document breaking-change policy.

## Compliance

| Requirement | Status |
|-------------|--------|
| VRAM secure clear on eviction | ✅ Implemented; ⚠️ HIP path unvalidated |
| Tenant key prefix `tenant:<id>:<index_name>` enforced | ✅ Enforced at IndexManager layer |
| Separator disallow-list for tenant IDs | ✅ Enforced at all 10 entry points; 15 regression tests (#1872 resolved 2026-04-07) |
| Index registry cross-tenant handle isolation | ✅ Enforced |
| Per-query VRAM budget cap | ✅ Enforced |
| Query timeout limits | ✅ Enforced |
| DiskANN/ScaNN on-disk file permissions (0600) | ✅ Enforced |
| Formal security audit | ⚠️ Open (#1885) |
