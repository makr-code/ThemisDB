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
  - ../../tests/graph/CMakeLists.txt
  - ../../benchmarks/graph/CMakeLists.txt
Related milestone: DOC-WEEKLY-2026-44
AI_Model_Note: Exact model backend not verifiable from final document text alone.
---

# ThemisDB graph module

The graph code provides C++ graph planning, traversal, path constraints, query rewrite/explain,
reasoning, and graph-related utility surfaces. This summary describes code presence, not a
production-readiness assessment or performance result.

## Code areas

- Planning/execution: `graph_query_optimizer`, `graph_query_rewriter`, and `explain_plan`.
- Traversal: `parallel_traversal`, `gpu_traversal`, and `distributed_graph`.
- Semantic behavior: `path_constraints`, `ontology_manager`, and `knowledge_graph_reasoner`.
- Additional utilities: RotatE completion, scheduled edge refresh, watermarks, and tensor
  fingerprint/deduplication.

Exact APIs are declared under [`include/graph/`](../../include/graph/). Graph-index behavior is
owned by [`include/index/graph_index.h`](../../include/index/graph_index.h); this module does not
define a replacement persistence format. For HTTP/wire behavior, use the server API layer rather
than treating these C++ headers as transport contracts.

## Build and tests

The root `cmake/CMakeLists.txt` includes graph implementation sources in the core build. Focused
tests are discovered from `tests/graph/test_*.cpp` by
[`tests/graph/CMakeLists.txt`](../../tests/graph/CMakeLists.txt). The graph benchmark subdirectory
is gated by `THEMIS_BUILD_BENCHMARKS` and discovers `bench_*.cpp`; see
[`benchmarks/graph/CMakeLists.txt`](../../benchmarks/graph/CMakeLists.txt).

Use the [implementation map](IMPLEMENTATION.md), [focused-test guide](../../tests/graph/README.md),
and [benchmark index](../../benchmarks/graph/README.md) for target names and source mappings.
Presence of a source file or test target is not evidence that it passed in a particular build.
