# Architecture - Index Module

<!-- Status: current | validated: 2026-09-09 -->
<!-- Links: README.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md -->

Last Updated: 2026-09-09
Module Path: src/index/
Status: Phase B hardening; HNSW + GPU indexing available

## Overview

The index module composes core index structures, acceleration backends, lifecycle controls, and optimization helpers into a bounded retrieval subsystem for ThemisDB.

## Main Execution Planes

1. Core index plane
- vector, secondary, spatial, graph, and full-text index structures
- deterministic insert/update/search behavior and persistence integration

2. Acceleration and compression plane
- GPU-aware vector pathways and backend-specific execution paths
- quantization and compression routines for memory/performance trade-offs

3. Lifecycle and operations plane
- index rebuild, tier migration, distributed and multi-GPU coordination
- workload replay and adaptive recommendation support

4. Advanced retrieval plane
- approximate radius and multi-vector retrieval helpers
- graph and temporal-oriented index extension paths

## Core Contracts

| Contract | Behavior |
|---|---|
| structure contract | deterministic index mutation and lookup semantics |
| acceleration contract | explicit backend-aware performance paths with bounded fallback |
| lifecycle contract | explicit rebuild/tiering/distribution operational behavior |
| optimization contract | bounded adaptation and replay-driven recommendation behavior |

## Failure Semantics

- invalid configuration or unsupported backend paths fail with explicit outcomes.
- degraded acceleration capabilities trigger deterministic fallback behavior.
- rebuild/distributed failures remain observable and non-silent.

## Sourcecode Verification (Module: index/architecture)

- Verified files:
  - src/index/index_manager.cpp
  - src/index/vector_index.cpp
  - src/index/gpu_vector_index.cpp
  - src/index/secondary_index.cpp
  - src/index/spatial_index.cpp
  - src/index/graph_index.cpp
  - src/index/tiered_index_manager.cpp
  - src/index/index_compression.cpp
  - src/index/distributed_vector_index.cpp
- Verified architecture claims:
  - explicit core/acceleration/lifecycle/advanced retrieval planes
  - deterministic fallback and failure boundaries
  - module-local ownership of index orchestration surfaces

---

## Module Dependencies

### Direct Upstream Dependencies (this module uses)

| Module | Interface / File | Purpose |
|--------|-----------------|---------|
| acceleration | `include/acceleration/compute_backend.h`, `include/acceleration/device_manager.h` | GPU-aware vector index execution paths (`gpu_vector_index.cpp`, `cuda_hnsw_graph_traversal.h`) |
| storage | `include/storage/` (base entity) | Index persistence, WAL, and snapshot integration |
| security | `include/security/` | Index-access credential gating and audit events |
| observability | `include/observability/` | Index operation spans, latency histograms |
| llm | `include/llm/lora_framework/lora_orchestrator.h` | LoRA framework integration for learned-index adaptation paths |
| themis/base | `include/themis/base/interfaces/index_interface.h` | `IVectorIndex`, `ISecondaryIndex`, `IGraphIndex`, `IIndexManager` abstract contracts |

### Direct Downstream Consumers (modules that use this module)

| Module | Via | Notes |
|--------|-----|-------|
| search | `include/index/ann_frontdoor.h`, `include/index/vector_index.h` | Primary ANN and vector-candidate provider for search module |
| rag | `include/index/ann_frontdoor.h`, `include/index/vector_index.h` | RAG hybrid retriever uses vector index for dense candidates |
| llm | `include/index/` (various) | LLM module uses vector index surfaces for embedding lookup |

---

## Integration Points

### Critical Integration: IVectorIndex / ISecondaryIndex / IGraphIndex — Base Index Contracts
**Files:** `include/themis/base/interfaces/index_interface.h` ↔ `src/index/vector_index.cpp`, `src/index/secondary_index.cpp`, `src/index/graph_index.cpp`
**Contract:** All index types implement THEMIS_BASE_API abstract interfaces (`IVectorIndex` line 126, `ISecondaryIndex` line 67, `IGraphIndex` line 194). This is the stable ABI boundary consumed by search, RAG, and LLM modules.
**Thread Safety:** Implementations must satisfy thread-safety guarantees declared in their header contract; `IIndexManager` coordinates concurrent access.
**Failure Mode:** Unsupported backend or invalid configuration → explicit status code; deterministic fallback to CPU path.

### Critical Integration: ANNFrontdoor — Stable ABI Entry-Point (Wave B Frozen)
**Files:** `include/index/ann_frontdoor.h` ↔ `src/index/` ANN routing layer
**Contract:** `ANNFrontdoor` is the single stable public entry-point for all ANN queries from external modules. Contract frozen at Wave B. Internally routes to HNSW, GPU-HNSW, or flat-index based on runtime capability.
**Thread Safety:** Thread-safe for concurrent `search()` calls.
**Failure Mode:** GPU unavailable → automatic fallback to CPU HNSW; result quality unchanged, latency increases.

### Critical Integration: GPU Vector Index + CUDA RAII Guards
**Files:** `include/index/gpu_vector_index.h`, `include/gpu/cuda_raii.h` ↔ `src/index/gpu_vector_index.cpp`
**Contract:** `GpuVectorIndex` uses CUDA RAII guards (`cuda_raii.h`) delivered in the current wave to ensure deterministic GPU resource cleanup. CUDA HNSW graph traversal (`cuda_hnsw_graph_traversal.h`) is available; Phase C CUDA reduction gate OPEN.
**Thread Safety:** GPU buffer allocation/deallocation is mutex-protected; no concurrent access to the same GPU buffer.
**Failure Mode:** VRAM exhaustion → eviction of least-recently-used index segments; if persistent, request rejected with `Status::kOutOfMemory`.

### Critical Integration: LoRA Framework — Learned Index Adaptation
**Files:** `include/llm/lora_framework/lora_orchestrator.h` ↔ `src/index/` (adaptive index paths)
**Contract:** Index module consumes LoRA orchestrator for dynamic learned-index adaptation. This is a downstream dependency on the LLM module, creating a cross-module coupling that must be managed via the `ILLMPlugin` lifecycle.
**Thread Safety:** Adapter hot-swap protected by `adapter_lifecycle_mutex_` in LLM module; index reads hold shared reference.
**Failure Mode:** LoRA adapter unavailable → fallback to static index; adaptation disabled until adapter reload.
