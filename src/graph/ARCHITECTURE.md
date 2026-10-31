---
Author: ThemisDB Contributors
Created: 2026-10-31
Last Updated: 2026-10-31
Status: active
AI_Used: true
AI_Models: unknown
AI_Contributor: ThemisDB documentation orchestration specialist
Human_Review: false
Human_Author: unknown
Source level: level1
SOT domain: module-behavior
Canonical source references:
  - ../../include/graph/
  - ../../src/graph/
  - ../../include/index/graph_index.h
  - ../../cmake/CMakeLists.txt
Related milestone: DOC-WEEKLY-2026-44
AI_Model_Note: Exact model backend not verifiable from final document text alone.
---

# Graph module architecture

## Scope and ownership

`include/graph/` declares C++ graph-facing APIs, with implementation sources under `src/graph/`.
The code includes optimizer/planning, rewrite and explain, path constraints, local/parallel and
GPU-oriented traversal, shard-executor coordination, ontology/reasoning, and additional graph
utilities. The exact operations and result types vary by header; see
[`API.md`](API.md) and the headers rather than assuming a common execution contract.

`GraphQueryOptimizer` and related graph components consume `GraphIndexManager` from the index
module. Canonical graph-index and persistence behavior remains with the owning index/storage
interfaces. The graph C++ module is not the HTTP/wire API or the caller-authentication boundary.

## Execution and integration paths

1. A caller supplies graph inputs and constraints to a declared planning, traversal, or path API.
2. The implementation uses graph-index operations where required; algorithm state and result
   types are defined by the particular API.
3. Optional paths include parallel traversal, device-aware traversal, and registered
   `ShardGraphExecutor` implementations. Do not infer universal GPU execution, cross-shard
   completeness, or a specific fallback result from the class names.
4. Ontology/reasoning and tensor-related utilities are separate APIs; their outputs do not
   automatically become canonical persisted graph facts.

## Build and validation wiring

- `cmake/CMakeLists.txt` lists graph implementation sources in the core target; the CUDA
  translation unit is conditional on `THEMIS_ENABLE_CUDA`.
- `tests/graph/CMakeLists.txt` discovers focused tests and gives per-file targets/CTest names.
- `benchmarks/graph/CMakeLists.txt` is guarded by `THEMIS_BUILD_BENCHMARKS` and discovers local
  `bench_*.cpp` sources.

These configuration statements describe registration only, not a successful build or test run.
See the relevant CMakeLists files for current target details.

## Unknowns / boundaries

- This architecture summary does not assert service-level authorization, tenant isolation, storage
  transaction semantics, or cluster health guarantees. Those depend on the owning integration
  layers; verify them there.
- There is no `src/graph/graph_embedding.cpp` in the current source inventory. The declared
  `IGraphEmbeddingProvider` interface alone does not establish an implementation.
- Performance, parity, and release maturity require captured test/benchmark evidence; no such
  result is asserted here.
