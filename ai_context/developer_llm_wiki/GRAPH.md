---
Author: "ThemisDB Team"
Created: "2026-10-31"
Last Updated: "2026-10-31"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
Source_Level: "level1"
SOT_Domain: "module-behavior"
Canonical_References: "include/graph/; src/graph/; tests/graph/CMakeLists.txt; benchmarks/graph/README.md; ROADMAP.md"
source: "include/graph/; src/graph/; tests/graph/CMakeLists.txt; benchmarks/graph/"
source_class: "secondary"
source_weight: 60
---

> Exact model backend not verifiable from final document text alone.

# Graph — Developer LLM Wiki

This page is an orientation aid, not an API contract, release decision, or
independent test report. Prefer current public headers for signatures,
implementations and focused tests for behavior, and root governance for
project-level status.

## Module shape and boundaries

The graph module groups query planning and rewriting, path constraints,
traversal, explain output, ontology and knowledge-graph reasoning, distributed
execution, GPU-assisted traversal, and tensor-fingerprint graph utilities.
Public declarations are under [`include/graph/`](../../include/graph/);
implementations are under [`src/graph/`](../../src/graph/). The module does not
by itself establish ownership of graph storage or client visualization.

Useful entry points:

- Planning and explanation: [`graph_query_optimizer.h`](../../include/graph/graph_query_optimizer.h),
  [`graph_query_rewriter.h`](../../include/graph/graph_query_rewriter.h),
  [`explain_plan.h`](../../include/graph/explain_plan.h).
- Constraints and execution: [`path_constraints.h`](../../include/graph/path_constraints.h),
  [`parallel_traversal.h`](../../include/graph/parallel_traversal.h),
  [`distributed_graph.h`](../../include/graph/distributed_graph.h),
  [`gpu_traversal.h`](../../include/graph/gpu_traversal.h).
- Semantic paths: [`ontology_manager.h`](../../include/graph/ontology_manager.h),
  [`knowledge_graph_reasoner.h`](../../include/graph/knowledge_graph_reasoner.h),
  [`rotate_completion.h`](../../include/graph/rotate_completion.h).
- Tensor utilities: [`tensor_fingerprint_graph.h`](../../include/graph/tensor_fingerprint_graph.h),
  [`tensor_deduplication_manager.h`](../../include/graph/tensor_deduplication_manager.h).

Use the module [`README.md`](../../src/graph/README.md),
[`ARCHITECTURE.md`](../../src/graph/ARCHITECTURE.md), and
[`API.md`](../../src/graph/API.md) for additional navigation, while resolving
behavior against source and tests.

## Behavior and evidence boundaries

- Planning, constraint validation, traversal, reasoning, and tensor utility
  paths are separate surfaces; a declaration or source file is not proof of
  production readiness or test success.
- GPU/distributed execution and fallback behavior must be checked in the
  specific implementation and tests. Do not infer universal parity or
  performance from the presence of these interfaces.
- RotatE is implemented in `rotate_completion.cpp`. The acceptance fixture in
  [`test_rotate_completion_acceptance.cpp`](../../tests/graph/test_rotate_completion_acceptance.cpp)
  asserts MRR ≥ 0.10, Hits@10 ≥ 0.40, relative-to-TransE bounds, and top-20
  p99 latency < 50 ms. Those are test thresholds, not captured evaluation
  results. The root [`ROADMAP.md`](../../ROADMAP.md) separately retains higher
  MRR ≥ 0.35 and Hits@10 ≥ 0.55 targets as unverified pending an evaluation
  result; the implemented-test sign-off does not establish those values.
- [`graph_embedding.h`](../../include/graph/graph_embedding.h) declares
  embedding interfaces, but the inspected graph source inventory does not
  contain a provider implementation. Do not treat the declarations as evidence
  that each listed algorithm is implemented.

## Tests and benchmarks

[`tests/graph/CMakeLists.txt`](../../tests/graph/CMakeLists.txt) discovers
`test_*.cpp` files and registers focused targets. For example, the RotatE
acceptance source maps to target
`module_graph_test_rotate_completion_acceptance_focused` and CTest name
`test_rotate_completion_acceptance_GraphFocusedTests`. Configure with
`THEMIS_BUILD_TESTS=ON`; standard presets may disable tests unless explicitly
overridden. Build a target and run its registered CTest by name to keep
validation focused. Test registration is not evidence that a test has run.

Graph benchmarks are gated by `THEMIS_BUILD_BENCHMARKS` and the benchmark
dependency; consult the current
[`benchmarks/graph/README.md`](../../benchmarks/graph/README.md) and
[`CMakeLists.txt`](../../benchmarks/graph/CMakeLists.txt) for registration and
commands. A benchmark source, configured target, or threshold does not prove a
run or representative-hardware result.

## Historical evidence

Two referenced `ai_working/` artifacts were retained in the archive:
[`gap_scanner_verified_graph.json`](../../docs/ARCHIVED/ai-working-history/gap_scanner_verified_graph.json)
and the
[Hybrid Retrieval Rollout Plan](../../docs/ARCHIVED/ai-working-history/HYBRID_RETRIEVAL_ROLLOUT_PLAN.md).
Both are historical snapshots, not current readiness evidence. The raw
`gap_scan_results_verified_L0.5_enhanced.json` was not found in the worktree or
archive; the related archived verifier report is a different artifact and is
not a replacement for that JSON. Do not reconstruct missing evidence from
summary text.

## Source priority

For code behavior, prefer the implementation and focused test for the exact
path, then public headers and module API/architecture documents, then this
summary. Root `ROADMAP.md` and active governance remain authoritative for
project status. The wiki is secondary context and must not silently resolve
conflicting source claims.
