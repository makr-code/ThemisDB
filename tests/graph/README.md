---
Author: ThemisDB Contributors
Created: 2026-10-31
Last Updated: 2026-10-31
Status: active
AI_Used: true
AI_Models: unknown
AI_Contributor: ThemisDB documentation orchestration specialist
Human_Review: false
Human_Author: ThemisDB Contributors
Source level: level1
SOT domain: module-behavior
Canonical source references:
  - CMakeLists.txt
  - ../cmake/RegisterModuleTests.cmake
  - ../../CMakePresets.json
  - ../../include/graph/
  - ../../src/graph/
Related milestone: DOC-WEEKLY-2026-44
AI_Model_Note: Exact model backend not verifiable from final document text alone.
---

# ThemisDB Graph Module - Tests

<!-- File/category inventory is descriptive; CMake globs are authoritative. -->
<!-- Links: ../../src/graph/README.md · ../../src/graph/ROADMAP.md · ../../src/graph/ARCHITECTURE.md -->

> **Build:** use the explicit test-enabled `community-debug` configure below;
> `linux-release` disables tests and benchmarks.

## Current Status

> This section describes current CMake registration only; no test run or coverage-completeness
> audit was performed.

> **Historical status snapshot (2026-06-25; not current verification):** the prior README said the
> graph gap alert was closed, reported zero verified gaps, and said `explain_plan`,
> `ontology_manager`, and `rotate_completion` follow-up work no longer blocked these tests. Those
> statements reproduce that snapshot only; they were not revalidated and are not current readiness
> or test-result claims.

- Focused graph tests are registered through `tests/graph/CMakeLists.txt` and can be selected with
  the `module:graph` label supplied by the registration helper.

---

## Overview

This directory contains focused C++ tests for ThemisDB's Graph module, including query planning,
traversal, constraint validation, semantic reasoning, and tensor-fingerprint utilities. CMake
registration is authoritative for the discovered test executables and their labels.

## Test Files by Category

> Parenthetical category counts are historical, non-exhaustive labels; some tests appear in more
> than one category. CMake globs are authoritative for the current inventory.

### Query Planning & Optimization (5 tests)
- `test_graph_query_optimizer.cpp` — cost-based planning, algorithm selection, plan caching
- `test_query_explain.cpp` — explain-plan generation and inspection
- `test_graph_query_rewriter.cpp` — query rewrite rules and optimization
- `test_graph_bfs_fix.cpp` — BFS-specific traversal and correctness
- `test_graph_index.cpp` — graph index operations

### Traversal & Execution (8 tests)
- `test_graph_parallel_traversal.cpp` — parallel multi-source traversal
- `test_graph_distributed.cpp` — distributed/cross-shard graph operations
- `test_gpu_traversal.cpp` — GPU-accelerated traversal with CPU fallback
- `test_graph_analytics.cpp` — graph analytics operations
- `test_graph_advanced_features.cpp` — advanced feature combinations
- `test_graph_index_comprehensive.cpp` — comprehensive index scenarios
- `test_path_constraints_semantic.cpp` — semantic constraint validation
- `test_graph_type_filtering.cpp` — type-based graph filtering

### Constraints & Semantics (6 tests)
- `test_path_constraints_semantic.cpp` — path constraint semantics
- `test_graph_query_optimizer.cpp` — constraint-aware planning
- `test_ontology_manager.cpp` — ontology loading and semantic lookups
- `test_knowledge_graph_reasoner.cpp` — graph reasoning and inference
- `test_graph_edge_encryption.cpp` — encrypted edge handling
- `test_graph_edge_empty_fields_qw45.cpp` — edge field validation

### Tensor Graph & Maintenance (6 tests)
- `test_tensor_fingerprint_graph.cpp` — tensor fingerprint similarity graphs
- `test_rotate_completion.cpp` — RotatE link-prediction integration
- `test_scheduled_edge_refresh.cpp` — scheduled graph refresh orchestration
- `test_graph_watermarking.cpp` — watermark and version boundary semantics

### Specialized Tests (5 tests)
- `test_query_explain.cpp` — explain-plan surfaces
- `test_graph_advanced_features.cpp` — feature integration
- `test_graph_analytics.cpp` — analytics operations
- `test_graph_index_comprehensive.cpp` — comprehensive indexing
- `test_graph_edge_empty_fields_qw45.cpp` — edge validation

The current directory also includes focused regression files for parity, exact traversal,
high-cardinality stress, phase-gate orchestration, query caches, plan-cache hardening, resource
pooling, load balancing, error taxonomy, phase-3 optimization, entity-type constraints, and RotatE
acceptance. The complete set is discovered by `test_*.cpp`; this list is not a second registration
source.

## Build & Run

```bash
# Configure using system packages (use a fresh binary directory for a clean build).
# community-debug inherits tests=OFF, so enable tests explicitly; disable benchmarks for a
# test-only build. Requires GoogleTest (GTest CONFIG).
cmake --preset community-debug -DTHEMIS_BUILD_TESTS=ON -DTHEMIS_BUILD_BENCHMARKS=OFF

# Build one focused graph test target
cmake --build --preset community-debug --target module_graph_test_graph_query_optimizer_focused

# Run all graph-focused tests
ctest --preset community-debug -L "module:graph" --output-on-failure

# Run specific test
ctest --preset community-debug -R "test_graph_query_optimizer_GraphFocusedTests" --output-on-failure
```

The system-package preset requires the Linux/macOS compiler, Ninja, and development dependencies
listed in `SETUP.md`. `linux-debug` also inherits tests-off in the current preset chain. Both
`file(GLOB)` blocks lack `CONFIGURE_DEPENDS`, so re-run configure after adding/removing matching
test files.

## Notes

- Test coverage is per-fixture; file presence does not imply integration with every storage or
  distributed subsystem.
- GPU traversal tests cover the code's fallback behavior; a hardware-backed GPU result requires
  the relevant runtime/build support.
- Distributed tests use the executor interfaces/fixtures in their source; do not infer a live
  cluster requirement or consensus coverage from the filename.
- For development/debugging, use `--gtest_filter` to isolate specific test cases.
