# Architecture - Graph Module

<!-- Status: current | validated: 2026-05-31 -->
<!-- Links: README.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md -->

## Overview

The graph module composes planning, traversal, constraints, and advanced graph-processing features into a bounded execution subsystem for ThemisDB.

## Main Execution Planes

1. Planning and optimization plane
- cost-based query planning and algorithm selection
- rewrite and explain-plan generation surfaces

2. Traversal and execution plane
- BFS/DFS/shortest-path style execution and parallel traversal
- distributed and GPU-assisted traversal routes with fallback controls

3. Semantic and reasoning plane
- ontology-aware constraints and knowledge-graph reasoning behavior
- refresh and watermark flows for evolving graph states

4. Tensor graph utility plane
- tensor-fingerprint graph similarity operations
- deduplication/persistence support for tensor graph data

## Core Contracts

| Contract | Behavior |
|---|---|
| planning contract | deterministic plan generation under explicit constraints |
| traversal contract | bounded traversal semantics across local/parallel/distributed routes |
| semantic contract | explicit ontology/reasoning validation and explainability surfaces |
| tensor utility contract | deterministic fingerprint similarity and dedup behavior |

## Failure Semantics

- invalid constraints or inconsistent semantic input fail with explicit outcomes.
- unsupported/degraded acceleration paths (GPU/distributed) degrade via bounded fallback behavior.
- execution errors surface deterministically rather than silently degrading correctness.

## Sourcecode Verification (Module: graph/architecture)

- Verified files:
  - src/graph/graph_query_optimizer.cpp
  - src/graph/path_constraints.cpp
  - src/graph/parallel_traversal.cpp
  - src/graph/distributed_graph.cpp
  - src/graph/gpu_traversal.cpp
  - src/graph/graph_query_rewriter.cpp
  - src/graph/explain_plan.cpp
  - src/graph/knowledge_graph_reasoner.cpp
  - src/graph/tensor_fingerprint_graph.cpp
- Verified architecture claims:
  - explicit planning/traversal/semantic/tensor planes
  - deterministic failure and fallback behavior boundaries
  - module-local ownership of graph orchestration surfaces