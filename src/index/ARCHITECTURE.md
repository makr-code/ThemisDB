---
Author: "ThemisDB Team"
Created: "2026-09-09"
Last Updated: "2026-10-31"
Status: active
AI_Used: true
AI_Models: unknown
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
---

Exact model backend not verifiable from final document text alone.

# Architecture - Index Module

Module path: `src/index/`. Public declarations are in `include/index/`.
This page describes component boundaries; it is not an API/ABI guarantee.

## Overview

The index module implements index data structures, routing, and lifecycle
operations used by retrieval and query components. Exact behavior, configuration,
and failure semantics are defined by each public header and implementation.

## Main Execution Planes

1. Core indexes
- vector, secondary, spatial, graph, and full-text index structures
- storage integration is provided through the relevant manager interfaces

2. Retrieval and acceleration
- ANN routing over registered backends
- GPU-aware vector paths, quantization, and compression helpers

3. Lifecycle and coordination
- vector reindexing, tier migration, distributed and multi-GPU helpers
- workload replay and adaptive recommendation components

4. Other retrieval structures
- approximate-radius and multi-vector search helpers
- graph analytics and temporal graph components

## Core Contracts

| Area | Source-defined responsibility |
|---|---|
| Index interfaces | Per-header input, result, and error semantics |
| ANN routing | Backend selection and routing metadata in `AnnFrontdoor` |
| Backend selection | Build/runtime-dependent capabilities and configured fallback |
| Lifecycle | Manager-specific reindex, migration, and persistence behavior |

## Failure Semantics

- Check the return type and error metadata of the specific operation; not every
  component uses the same error model.
- CPU fallback is configuration-dependent for `GPUVectorIndex`; it is not a
  module-wide guarantee.
- ANN frontdoor can return degraded or partial results. Consumers must inspect
  result metadata rather than infer success from a route label.

## Canonical implementation entry points

The following files are useful starting points; consult their matching headers
for public contracts:

| Area | Implementation | Public declaration |
|---|---|---|
| ANN routing | `ann_frontdoor.cpp` | `include/index/ann_frontdoor.h` |
| Vector index | `vector_index.cpp` | `include/index/vector_index.h` |
| Index manager | `index_manager.cpp` | `include/index/index_manager.h` |
| Secondary/full-text index | `secondary_index.cpp`, `inverted_index.cpp` | `include/index/secondary_index.h`, `include/index/inverted_index.h` |
| GPU vector index | `gpu_vector_index.cpp` | `include/index/gpu_vector_index.h` |
| Tiered/distributed vector index | `tiered_index_manager.cpp`, `distributed_vector_index.cpp` | Matching headers under `include/index/` |

## Integration and validation references

- [API behavior and known caveats](API.md)
- [Developer integration guide](MODULE_GUIDE.md)
- [Focused test registration](../../tests/index/CMakeLists.txt)
- [Current readiness and open gates](ROADMAP.md)
