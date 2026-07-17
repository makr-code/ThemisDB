# ThemisDB Graph Module - Tests

<!-- Status: current | validated: 2026-07-17 -->
<!-- Links: ../src/graph/README.md · ../src/graph/ROADMAP.md · ../src/graph/ARCHITECTURE.md -->

> **Build:** `cmake --preset linux-release && cmake --build --preset linux-release`

## Current Status

- The earlier graph gap alert was closed on 2026-06-25: canonical source verification in
  `src/graph/README.md` and `src/graph/ROADMAP.md` reports **0 verified gaps** in the backing
  implementation.
- The tests in this directory are no longer blocked by `explain_plan`, `ontology_manager`, or
  `rotate_completion` follow-up work.
- Focused graph tests are registered through `tests/graph/CMakeLists.txt` and can be selected with
  the label `module:graph`.

---

## Overview

This directory contains comprehensive test suites for ThemisDB's Graph module, including unit tests, integration tests, and benchmark fixtures for all graph execution planes: query planning, traversal (parallel/distributed/GPU), constraint validation, semantic reasoning, and tensor-fingerprint graph utilities.

## Test Files by Category

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

## Build & Run

```bash
# Build one focused graph test target (repeat with other `module_graph_*` targets as needed)
cmake --build --preset linux-release --target module_graph_test_graph_query_optimizer_focused

# Run all graph-focused tests
ctest --preset linux-release -L "module:graph" --output-on-failure

# Run specific test
ctest --preset linux-release -R "test_graph_query_optimizer_GraphFocusedTests" --output-on-failure
```

## Notes

- All tests integrate with ThemisDB's ACID transaction and distributed coordination layers.
- GPU tests (test_gpu_traversal.cpp) require CUDA or HIP backend; fall back to CPU if unavailable.
- Distributed tests assume access to sharding and consensus layers.
- For development/debugging, use `--gtest_filter` to isolate specific test cases.
