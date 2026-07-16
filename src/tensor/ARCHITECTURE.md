# Architecture - Tensor Module

<!-- Status: current | validated: 2026-05-31 -->
<!-- Links: README.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md -->

## Overview

The tensor module composes tensor index management, bridge-oriented ingestion/runtime behavior, fingerprint graph operations, and tensor-specific structural helper paths into a bounded subsystem.

## Main Execution Planes

1. Tensor index and retrieval plane
- tensor index, manager, and hybrid index bridge behavior

2. Bridge and ingestion plane
- tensor core/ingestion/mmap bridges and conversion behavior

3. Fingerprint and structural plane
- tensor fingerprint graph, dedup-adjacent, and structural helper behavior

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