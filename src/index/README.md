---
Author: "ThemisDB Team"
Created: "2026-10-18"
Last Updated: "2026-10-31"
Status: active
AI_Used: true
AI_Models: unknown
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
---

Exact model backend not verifiable from final document text alone.

# ThemisDB Index Module

## Purpose and boundaries

`src/index/` implements index data structures and lifecycle operations used by
retrieval and query components. The directory includes vector and ANN routing,
secondary and inverted indexes, spatial and graph indexes, GPU-aware paths,
quantizers/compression, tiering, and distributed-index helpers. Build and
runtime availability vary by backend; this overview does not assert production
readiness or hardware parity.

The index module owns index behavior and its exposed lifecycle interfaces. It
does not own embedding generation, query parsing/planning, the storage engine,
or caller authorization. Integrators must enforce authorization outside index
filters and tenant-qualified names.

## Source entry points

| Area | Primary implementation |
|---|---|
| ANN routing and candidate metadata | `ann_frontdoor.cpp` |
| Vector index lifecycle/search | `vector_index.cpp` |
| Index manager and adapters | `index_manager.cpp` |
| Secondary and full-text indexes | `secondary_index.cpp`, `inverted_index.cpp` |
| Spatial and graph indexes | `spatial_index.cpp`, `graph_index.cpp` |
| GPU vector paths | `gpu_vector_index.cpp`, `gpu_vector_index_vulkan.cpp` |
| Tiered and distributed vector paths | `tiered_index_manager.cpp`, `distributed_vector_index.cpp` |

These are navigation pointers, not a complete file inventory. Public
declarations are under [`../../include/index/`](../../include/index/README.md).

## Developer references

- [Architecture and ownership](ARCHITECTURE.md)
- [Current API behavior and caveats](API.md)
- [Integration guide](MODULE_GUIDE.md)
- [Implementation notes](IMPLEMENTATION.md)
- [Operational checks](OPERATIONS.md) and [incident runbook](RUNBOOK.md)
- [Current module status and open gates](ROADMAP.md)
- [Focused tests and registration](../../tests/index/README.md)
- [Index rebuild benchmark](../../benchmarks/index/README.md)

## Build and validation

Index tests are registered from [`../../tests/index/CMakeLists.txt`](../../tests/index/CMakeLists.txt).
The benchmark in `benchmarks/index/` is conditional on the repository benchmark
build option; see its README for the configured preset and target. Select a
backend-specific test/build configuration before treating CPU-only test results
as evidence for GPU behavior.

## Status boundaries

The [module roadmap](ROADMAP.md) is the source for current delivery status and
open validation gates. In particular, CPU tests do not establish numerical
parity on representative GPU hardware. Historical readiness claims in older
module notes are not current status evidence.