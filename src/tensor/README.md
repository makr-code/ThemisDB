---
Author: "ThemisDB Contributors"
Created: "2026-05-31"
Last Updated: "2026-10-31"
Status: "active"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Contributors"
---

> Exact model backend not verifiable from final document text alone.

# ThemisDB Tensor Module

<!-- Status: current | validated: 2026-10-31 -->
<!-- Links: ARCHITECTURE.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md · THEMIS_BRAIN_MEMORY_COMPARATIVE_PAPER.md -->

## Module Purpose

The tensor module provides tensor-aware indexing, retrieval, and graph/fingerprint behavior for ThemisDB, including tensor index management, tensor fingerprint graph operations, and tensor deduplication-related behavior.

## Relevant Interfaces

| Interface / File | Role |
|---|---|
| tensor_index.cpp | tensor index behavior |
| tensor_index_manager.cpp | tensor index lifecycle and routing behavior |
| tensor_core_bridge.cpp | tensor core bridge behavior |
| tensor_ingestion_bridge.cpp | tensor ingestion bridge behavior |
| tensor_mmap_bridge.cpp | tensor mmap bridge behavior |
| hnsw_tt_bridge.cpp | hybrid HNSW-TT bridge behavior |
| ht_index.cpp | hierarchical tensor index behavior |
| tensor_fingerprint_graph.cpp | tensor fingerprint graph behavior |
| adapter_repository.cpp | tensor adapter repository behavior |
| hyper_index_builder.cpp | tensor hyper-index builder behavior |
| hiss_structural_search.cpp | structural search behavior for tensor data |
| tnsr_task.cpp | tensor network structural task behavior |
| utr_converter.cpp | UTR conversion behavior |
| tensor_butterfly_operator.cpp | tensor butterfly operator behavior |

## Scope

In scope:
- tensor-aware indexing and retrieval behavior
- tensor fingerprint graph and dedup-related runtime behavior
- tensor bridges, ingestion, and structural helper behavior

Out of scope:
- non-tensor ANN internals owned by other modules
- external model-serving orchestration outside tensor boundaries

## Runtime Behavior and Limits

- `TensorIndexManager::routeFor()` derives a `TensorRouter::DataProfile` from the dimension and vector count, estimates compressibility from dimension, and returns a route. The current `createIndex()` implementation creates a flat TT index without consuming that route; see [API contract caveat](./API.md).
- fingerprint graph behavior remains explicit and observable under insert/query/export operations.
- `findSimilar()` ranks registered TT trains using TT inner products; `findSimilarByFingerprint()` scans matching entries using approximate cosine similarity with zero-padding for unequal fingerprint lengths. Neither path uses LSH in the current implementation.
- `TensorCoreStorageBridge` uses an injected storage backend or registered factory; without either, it falls back to `InMemoryTensorBackend` and does not persist across process restarts.
- `TensorIndexManager::mapCores()` defaults to anonymous mapped memory populated by copying TT core data. File-backed `MAP_SHARED` mapping is a separate `buildFromFd()`/injected-mapper path, not the manager's default.

## Installation

This is an in-tree module guide; use the main ThemisDB build configuration rather
than treating `themis_tensor` as a separately packaged installation.

## Usage

Include the relevant public header from `include/tensor/` and follow its current
contract. The [public header inventory](../../include/tensor/README.md) and
[API notes](./API.md) identify runtime behavior and known discrepancies.

## Build and Test

The module is a C++17 static library target. From an existing configured build directory:

```sh
cmake --build <build-dir> --target themis_tensor
cmake --build <build-dir> --target module_tensor_test_tensor_contract_hardening_focused
ctest --test-dir <build-dir> -L tensor --output-on-failure
```

Module tests are discovered from `tests/tensor/test_*.cpp`; the focused CTest name is
`<source-stem>_tensor_FocusedTests`. The stress target has a 300-second timeout.
Tensor benchmarks are discovered from `benchmarks/tensor/bench_*.cpp` only when
`THEMIS_BUILD_BENCHMARKS` is enabled. CMake registration is not evidence that a
test or benchmark has built or passed.

## Sourcecode Verification (Module: tensor/readme)

- Verified files:
  - src/tensor/tensor_index.cpp
  - src/tensor/tensor_index_manager.cpp
  - src/tensor/tensor_core_bridge.cpp
  - src/tensor/tensor_ingestion_bridge.cpp
  - src/tensor/tensor_mmap_bridge.cpp
  - src/tensor/hnsw_tt_bridge.cpp
  - src/tensor/ht_index.cpp
  - src/tensor/tensor_fingerprint_graph.cpp
  - src/tensor/adapter_repository.cpp
  - src/tensor/hyper_index_builder.cpp
  - src/tensor/hiss_structural_search.cpp
  - src/tensor/tnsr_task.cpp
  - src/tensor/utr_converter.cpp
  - src/tensor/tensor_butterfly_operator.cpp
- Verified behavior surfaces:
  - tensor indexing/bridge/fingerprint/dedup-oriented runtime paths
- Note:
  - forward planning is tracked in ROADMAP.md and FUTURE_ENHANCEMENTS.md
  - historical entries remain in CHANGELOG.md