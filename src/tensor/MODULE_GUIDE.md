---
Author: "ThemisDB Contributors"
Created: "2026-10-21"
Last Updated: "2026-10-21"
Status: "active"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "unknown"
Human_Review: true
Human_Author: "ThemisDB Contributors"
---

Exact model backend not verifiable from final document text alone.

# Tensor Module Guide

## Purpose

The tensor module is the compact, TT-oriented representation layer for ThemisDB. It owns the data structures and runtime paths that turn adapter or vector workloads into compressed, searchable, and diagnosable tensor representations.

This module is the correct place for:

- TT decomposition and reuse
- tensor route selection and index lifecycle
- adapter registry and loading logic
- fingerprint-based similarity search
- mmap and bridge integration for tensor-heavy memory paths

This module is not the correct place for:

- broad public auth logic
- generic query planner semantics
- unrelated storage engine ownership outside tensor-specific storage contracts

## Primary files

- `include/tensor/tensor_index_manager.h` — main lifecycle manager
- `include/tensor/tensor_fingerprint_graph.h` — similarity graph contract
- `include/tensor/adapter_repository.h` — adapter persistence and retrieval
- `include/storage/tensor_router.h` — routing policy and decision surface
- `include/storage/tensor_train_decomposer.h` — TT-core and TT-train contract
- `src/tensor/tensor_index_manager.cpp` — runtime lifecycle implementation
- `src/tensor/tensor_fingerprint_graph.cpp` — fingerprint scoring and diagnostics
- `src/tensor/adapter_repository.cpp` — repository write/read and storage hooks
- `src/tensor/tensor_mmap_bridge.cpp` — mmap-backed bridge implementation

## Working conventions

### Keep the data contract explicit

Any change in TT-core layout, route threshold, or fingerprint similarity rules should be visible in the code and in the relevant docs. This module values explicit contracts more than hidden compatibility behavior.

### Prefer fail-safe behaviour over silent degradation

The module repeatedly emits diagnostics and early returns for invalid state. That should remain the default approach in future edits.

### Keep graph and storage responsibilities separate

The `AdapterRepository` and `TensorFingerprintGraph` are distinct responsibilities. Storage owns data persistence; graph owns similarity and ranking views. A good change preserves that separation.

## Development checklist

Before landing a tensor change, verify:

- route selection still matches the `TensorRouter` thresholds and data profile assumptions
- TT core validation still rejects invalid or empty core shapes
- fingerprint search remains deterministic with bounded returns
- cleanup and persistence paths remain paired when indexes or adapters are removed
- diagnostics remain observable for malformed states

## Validation guidance

Target the closest tensor tests and benchmarks rather than broad integration runs. The most relevant validation path is the module’s existing contract-hardening and benchmark suites in `tests/tensor` and the module-level benchmark gates.
