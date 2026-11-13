---
Author: "ThemisDB Contributors"
Created: "2026-09-09"
Last Updated: "2026-11-13"
Status: "active"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Contributors"
---

> Exact model backend not verifiable from final document text alone.

# Architecture - Tensor Module

<!-- Status: current | validated: 2026-10-31 -->
<!-- Links: README.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md -->

## Overview

The tensor module composes tensor index management, bridge-oriented ingestion/runtime behavior, fingerprint graph operations, and tensor-specific structural helper paths into a bounded subsystem.

## Main Execution Planes

### Role in the native database/LLM hybrid (design target; 2026-11-13)

Tensor abstraction is not merely prompt formatting or an additional vector
index. Distinguish:

- **Representation/storage:** TT cores, ranks, dtype/layout and reconstruction
  error for vectors or adapter updates; persist the required metadata and
  lifetimes through the selected backend.
- **Selection/planning:** `TensorMidLayer` normalizes adapter/package/
  fingerprint/shard scopes and queries injected repositories/fingerprint
  graphs. Summaries reduce candidate work, not necessarily prompt tokens.
- **Training/serving bridge:** `AdaLoraTTBridge` exposes adapter conversion,
  while `TensorRAGPipeline` and final-layer interfaces expose routing/handoffs.
  Verify the actual decoder/ggml consumer before claiming compressed
  execution, zero-copy, KV injection or live adapter switching.
- **Decentralized artifacts:** distributed-tensor planners and injected
  fetchers can support summary-first/exact-on-demand design. Planning APIs and
  in-process callback execution are not a built-in distributed transport.

The proposed contract binds tenant/base-model/artifact versions, shape,
dtype/layout, rank/scaling, decomposition profile, measured approximation
error, provenance and storage/mapping ownership. Qualify approximate summaries
with exact artifact/evidence checks where required; similarity is neither
authorization nor adapter compatibility.

Tensor representations do not create an unlimited semantic context window.
The existing RAG `tensor_hybrid`/`tensor_prefix` text compatibility modes are
not direct tensor/KV injection. Native tensor consumption needs separate
backend capability, numerical/quality tests and runtime evidence. See
[the tensor backlog](FUTURE_ENHANCEMENTS.md),
[training](../training/ARCHITECTURE.md), and
[native LLM architecture](../llm/ARCHITECTURE.md).

1. Tensor index and retrieval plane
- `TensorIndexManager::routeFor()` obtains a `TensorRouter` decision from a dimension-derived profile; `createIndex()` currently constructs a flat TT index without applying that decision. `HnswTTBridge` is a separate `ITensorIndex` implementation.

2. Bridge and ingestion plane
- `TensorIngestionBridge` produces TT artifacts; `TensorCoreStorageBridge` writes serialized records through an injected storage backend; `TensorMmapBridge` supplies lifetime-bound core slices. The default manager mapping copies into anonymous mapped memory; direct file mapping requires the separate FD or injected mapper path.

3. Fingerprint and structural plane
- `AdapterRepository` may register TT trains with `TensorFingerprintGraph`. The graph stores first-core fingerprints and registered trains; key-based search uses TT inner products, while raw-fingerprint search scans candidates using approximate cosine similarity. Structural helpers are separate public surfaces.

## Data Flow

1. A caller supplies dimensions and vector count to `routeFor()` for a routing estimate.
2. `createIndex()` creates or restores the manager's flat TT index; `addFlat()`/`searchFlat()` compress flat vectors, while `add()`/`search()` accept `TTTrain` values directly.
3. Ingestion records are serialized and passed to the configured tensor storage backend. Adapter storage uses its own required backend and can optionally wire the fingerprint graph.
4. Similarity requests use either registered-train comparison or the approximate first-core fingerprint path; invalid/missing registered-train state is rejected or skipped, with diagnostics on the corresponding error paths.

## Core Contracts

| Contract | Behavior |
|---|---|
| index contract | deterministic tensor index lifecycle and lookup behavior |
| bridge contract | explicit ingestion/core/mmap bridge outcomes |
| fingerprint contract | observable insert/query/neighbour/export behavior |
| structural contract | bounded behavior for tensor helper and transformation paths |

## Performance Expectations

- `TensorFingerprintGraph::findSimilar` targets bounded query latency under candidate growth by
  caching per-adapter self inner-products and avoiding repeated norm recomputation.
- `TensorFingerprintGraph::findSimilarByFingerprint` uses cached fingerprint squared norms and
  overlap-only dot products with explicit zero-padding semantics for dimension mismatch.
- `AdapterRepository::store` overwrite path updates aggregate stats in O(1) and must not inflate
  `total_adapters` on idempotent key replacement.
- Baseline SLO envelope (release profile, reference CI hardware):
  - key-based similarity at 10k candidates: p95 <= 80 ms, p99 <= 140 ms
  - fingerprint similarity at 10k candidates: p95 <= 15 ms, p99 <= 30 ms

## Failure Semantics

- tensor index lifecycle and route mismatches are explicit.
- bridge ingestion/persistence faults remain diagnosable.
- fingerprint graph path failures surface deterministic outcomes.
- advanced structural path failures remain observable and non-silent.

## Sourcecode Verification (Module: tensor/architecture)

- Verified files:
  - src/tensor/tensor_index.cpp
  - src/tensor/tensor_index_manager.cpp
  - src/tensor/tensor_core_bridge.cpp
  - src/tensor/tensor_ingestion_bridge.cpp
  - src/tensor/hnsw_tt_bridge.cpp
  - src/tensor/tensor_fingerprint_graph.cpp
  - src/tensor/adapter_repository.cpp
  - src/tensor/hyper_index_builder.cpp
  - src/tensor/hiss_structural_search.cpp
  - src/tensor/tnsr_task.cpp
- Verified architecture claims:
  - index/retrieval + bridge/ingestion + fingerprint/structural plane split
  - explicit failure boundaries for index, bridge, and graph-operation faults
  - module-local ownership of tensor-domain behavior

## Module Dependencies

### Direct Upstream Dependencies (this module uses)
| Module | Interface / File | Purpose |
|--------|-----------------|---------|
| CMake target | Linkage | Purpose |
|--------|-----------------|---------|
| `Threads::Threads` | public | Threading used by the library interface |
| `spdlog::spdlog` | private | Internal logging |
| `themis_core`, `themis_index`, `themis_rag`, `themis_query` | private, when each target exists | Optional integration with targets already defined by the build |
| storage headers | source-level | TT representation/decomposition, routing, and tensor storage backend contracts |

`themis_tensor` is a static library target requiring C++17. The source list and
conditional links are defined in `src/tensor/CMakeLists.txt`; test and benchmark
registration are defined in `tests/tensor/CMakeLists.txt` and
`benchmarks/tensor/CMakeLists.txt`. There is no single tensor-module runtime
configuration file: behavior is controlled by API arguments, injected backends,
and optional compile-time guards.

## Integration Points

### Critical Integration: Storage Tensor Network Engine
**Files:** `src/tensor/tensor_core_bridge.cpp` ↔ `storage/tensor_network_storage_engine.h`
**Contract:** `TensorCoreStorageBridge` stores serialized `TensorCoreRecord` bytes through `ITensorStorageBackend`. A backend or default factory can be injected; absent either, the implementation uses an in-memory backend. The bridge does not add a separate signature-validation or durable-storage guarantee.

### Integration: Ingestion contract
**Files:** `src/tensor/tensor_core_bridge.cpp` ↔ `include/ingestion/ingestion_sinks.h`
**Contract:** `TensorCoreStorageBridge` implements `ingestion::ITensorCoreBridge`; it validates tenant and record fields and reports backend write failures through its result type.