---
Author: "ThemisDB Team"
Created: "2026-10-30"
Last Updated: "2026-10-30"
Status: active
AI_Used: true
AI_Models: unknown
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
Source_Level: level1
SOT_Domain: module-behavior
Canonical_Sources:
  - "src/index/README.md"
  - "src/index/ARCHITECTURE.md"
  - "src/index/ROADMAP.md"
  - "include/index/index_manager.h"
  - "include/index/vector_index.h"
  - "include/index/ann_frontdoor.h"
  - "tests/index/CMakeLists.txt"
Related: "Issue #7125; DOC-WEEKLY-2026-44"
---

Exact model backend not verifiable from final document text alone.

# Index module guide

## Purpose and reader

Use this guide to orient a developer changing or integrating `src/index/`.
It identifies ownership, the safest source-reading path, integration
preconditions, and focused validation. It does not replace public declarations
or prescribe APIs beyond those present in source.

## Module scope

The module owns data structures and lifecycle for vector, secondary, spatial,
graph, and full-text index paths, plus ANN routing, selected acceleration and
compression paths, and index maintenance. It does not own embedding generation,
query-planner policy, storage-engine implementation, user authentication, or
GPU driver/hardware validation.

Public interfaces live under `include/index/`; consult the
[source-checked API reference](API.md) and the declarations themselves.
implementation is under this `src/index/`; focused module tests are under
[`tests/index/`](../../tests/index/README.md). Begin with the module
[README](README.md), [architecture](ARCHITECTURE.md), and
[roadmap](ROADMAP.md), then inspect the relevant header and `.cpp` before
relying on a summary.

## Integration sequence

1. Establish the data contract first: vector dimension, metric, vector field,
   namespace, and (where relevant) tenant scope must agree between producer
   and index.
2. Wire backing storage before creating indexes. `IndexManager` can report
   initialization errors when RocksDB is not configured.
3. Configure optional advanced indexing before `VectorIndexManager::init()`.
   Select a backend only after verifying compile-time and runtime availability.
4. Register ANN and tier backends before exposing concurrent searches.
   `AnnFrontdoor` registration is not safe to race with its read-only search
   methods.
5. At retrieval time, propagate query dimensions, scope, tier/shard hints, and
   correlation context consistently. Inspect route, fallback, and partial
   result metadata before consuming candidates.
6. On update/rebuild, check each returned status and preserve the authoritative
   entity records. Validate the rebuilt result set before switching consumers.

## Choosing the right source and test

| Change area | Primary source | Focused evidence |
|---|---|---|
| ANN plan, route, candidate validation | [`ann_frontdoor.h`](../../include/index/ann_frontdoor.h), [`ann_frontdoor.cpp`](ann_frontdoor.cpp) | [`test_ann_frontdoor.cpp`](../../tests/index/test_ann_frontdoor.cpp), [`test_ann_cpu_parity.cpp`](../../tests/index/test_ann_cpu_parity.cpp) |
| Vector insert/search/reindex | [`vector_index.h`](../../include/index/vector_index.h), [`vector_index.cpp`](vector_index.cpp) | [`test_index_maintenance.cpp`](../../tests/index/test_index_maintenance.cpp), [`test_hnsw_recall_integration.cpp`](../../tests/index/test_hnsw_recall_integration.cpp) |
| Manager/adapters/tenant namespaces | [`index_manager.h`](../../include/index/index_manager.h), [`index_manager.cpp`](index_manager.cpp) | [`test_index_manager_di.cpp`](../../tests/index/test_index_manager_di.cpp) |
| Manifest serialization/history | [`index_manifest_v1.h`](../../include/index/index_manifest_v1.h), [`index_manifest_v1.cpp`](index_manifest_v1.cpp) | [`test_index_manifest_v1.cpp`](../../tests/index/test_index_manifest_v1.cpp) |
| GPU backend fallback | [`gpu_vector_index.h`](../../include/index/gpu_vector_index.h), [`gpu_vector_index.cpp`](gpu_vector_index.cpp) | [`test_gpu_vector_index_llm_paths_focused.cpp`](../../tests/index/test_gpu_vector_index_llm_paths_focused.cpp) |

Focused test registration is in
[`tests/index/CMakeLists.txt`](../../tests/index/CMakeLists.txt). Actual
CTest names follow the test source stem plus `_IndexFocusedTests` for the
focused suite; use names from a configured build's CTest listing rather than
assuming every source is enabled in every build.

## Failure and edge handling

- Check `Status`, `Result<T>`, or boolean return values before publishing an
  index mutation or backend switch.
- Empty results may mean a legitimately empty match, missing/unready backend,
  failure, or fallback limitation; use result/error metadata to disambiguate.
- Distributed results can be partial or fail closed according to configuration.
- ANN frontdoor rejects null/zero-sized queries but delegates dimension
  compatibility to the selected backend.
- `AnnFrontdoor` currently has no storage-backed brute-force implementation;
  a planned flat strategy may therefore return no candidates if no usable
  registered backend exists.
- GPU CPU fallback is controlled explicitly by `allowCPUFallback`; a passing
  CPU fallback test does not prove GPU parity.
- The manifest class is not thread-safe and does not itself implement durable
  RocksDB writes or atomic rollback.

## Operational connection

Index changes affect retrieval completeness, latency, persistent state,
resource use, and potentially tenant data isolation. Review the
[operations guide](OPERATIONS.md), [runbook](RUNBOOK.md), and
[trust boundaries](TRUST.md) before changing lifecycle or fallback behavior.
Current release-level readiness and the remaining hardware-only GPU parity
gate are in the [module roadmap](ROADMAP.md).
