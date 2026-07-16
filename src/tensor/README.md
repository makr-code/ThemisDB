# ThemisDB Tensor Module

<!-- Status: current | validated: 2026-05-31 -->
<!-- Links: ARCHITECTURE.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md -->

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

- tensor routing/index behavior is bounded by index and bridge configuration.
- fingerprint graph behavior remains explicit and observable under insert/query/export operations.
- dedup and replay-adjacent behavior is diagnosable through dedicated test/benchmark paths.
- advanced tensor paths remain configuration-sensitive and require explicit enablement.

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