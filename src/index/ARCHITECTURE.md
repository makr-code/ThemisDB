# Architecture - Index Module

<!-- Status: current | validated: 2026-05-31 -->
<!-- Links: README.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md -->

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