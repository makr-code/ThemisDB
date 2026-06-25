> **Build:** `cmake --preset linux-ninja-release && cmake --build --preset linux-ninja-release`

# ThemisDB Graph Module - Tests

<!-- Status: current | validated: 2026-06-25 -->
<!-- Links: ../src/graph/README.md · ../src/graph/ROADMAP.md · ../src/graph/ARCHITECTURE.md -->

## ⚠️ L0 Risk Alert (2026-06-25)

**Status: CRITICAL GAPS DETECTED IN BACKING IMPLEMENTATIONS**

- **Total Gaps**: 9 (8 critical, 1 high) — affects source module, not tests
- **Blocked Tests**: Some integration scenarios may be blocked waiting for `explain_plan` and `ontology_manager` implementations
- **Backing Files at Risk**:
  - `src/graph/explain_plan.cpp` (2 critical)
  - `src/graph/ontology_manager.cpp` (2 critical)
  - `src/graph/rotate_completion.cpp` (3 critical)
- **Action**: See `src/graph/README.md` and `src/graph/ROADMAP.md` for mitigation and implementation timeline.

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
# Build all graph tests
cmake --build --preset windows-release --target module_graph_test_*

# Run all graph tests
ctest --preset windows-release -R "^test_graph" --output-on-failure

# Run specific test
ctest --preset windows-release -R "test_graph_query_optimizer" --output-on-failure
```

## Notes

- All tests integrate with ThemisDB's ACID transaction and distributed coordination layers.
- GPU tests (test_gpu_traversal.cpp) require CUDA or HIP backend; fall back to CPU if unavailable.
- Distributed tests assume access to sharding and consensus layers.
- For development/debugging, use `--gtest_filter` to isolate specific test cases.
