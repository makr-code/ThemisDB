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
  - ../../include/graph/path_constraints.h
  - ../../src/graph/path_constraints.cpp
  - ../../include/graph/graph_query_optimizer.h
  - ../../include/index/graph_analytics.h
Related milestone: DOC-WEEKLY-2026-44
AI_Model_Note: Exact model backend not verifiable from final document text alone.
---

# Graph advanced features

This page is a source map, not a feature-readiness or performance claim. The public declarations
and their implementations define the current behavior.

## Path constraints

`themis::graph::PathConstraints` is declared in
[`include/graph/path_constraints.h`](../../include/graph/path_constraints.h) and implemented in
[`path_constraints.cpp`](path_constraints.cpp). The class exposes constraint-building methods,
`validatePath()`, `findConstrainedPaths()`, and constraint inspection. Mutator methods return
`void`; validation failures may leave an attempted constraint unapplied. Validate caller inputs
and inspect `getConstraints()` rather than assuming a rejected value was installed.

Focused evidence: [`test_path_constraints_semantic.cpp`](../../tests/graph/test_path_constraints_semantic.cpp)
and [`test_graph_type_filtering.cpp`](../../tests/graph/test_graph_type_filtering.cpp).

## Optimizer and graph analytics boundaries

`GraphQueryOptimizer` declares planning, traversal, execution-statistics, and cache-control
surfaces in [`graph_query_optimizer.h`](../../include/graph/graph_query_optimizer.h). Graph index
operations are declared by the owning index module; graph analytics such as PageRank and
centrality are not re-described here as graph-module APIs. Consult
[`include/index/graph_analytics.h`](../../include/index/graph_analytics.h) for that contract.

## Source of current details

- [Graph C++ API overview](API.md)
- [Data and algorithm modeling](MODELING.md)
- [Graph implementation and focused-test map](IMPLEMENTATION.md)
- [Graph focused tests and CMake registration](../../tests/graph/README.md)

The former version of this page contained copied API examples, algorithm-complexity estimates,
performance assertions, and implementation-status claims that were not consistently supported
by current declarations or tests. Those examples and claims are intentionally removed rather than
preserved as current guidance.
