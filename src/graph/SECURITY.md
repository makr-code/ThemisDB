# Security - Graph Module

<!-- Status: current | validated: 2026-05-31 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md -->

Report vulnerabilities via project-level SECURITY.md.

## Security Scope

Security in the graph module focuses on constraint-safe traversal, bounded execution behavior, deterministic fallback handling for advanced routes, and prevention of unsafe cross-context graph access patterns.

## Threat Model

| Threat | Current Mitigation Surface |
|---|---|
| unconstrained traversal expansion | explicit path/depth/result constraints and validation gates |
| unsafe semantic or reasoning input effects | ontology/rule validation and bounded reasoning behavior |
| degraded acceleration path correctness risk | deterministic fallback behavior for GPU/distributed routes |
| cross-context data exposure in graph execution | explicit graph/edge/node constraint and policy checks |
| hidden query-planning regressions | explain/rewrite/telemetry surfaces for observable behavior |

## Implemented Security Controls

- path constraints gate traversal breadth/depth and required/forbidden entities.
- planner and execution flows surface explicit outcomes for invalid inputs.
- advanced routes (distributed/GPU) expose bounded fallback behavior.
- semantic and reasoning paths rely on explicit ontology/rule constraints.

## Security Follow-ups

- continue hardening distributed and GPU edge-case behavior under partial capability.
- tighten diagnostics around invalid semantic input and denial paths.
- expand abuse-case coverage for high fan-out and mixed-constraint scenarios.

## Sourcecode Verification (Module: graph/security)

- Verified files:
  - src/graph/path_constraints.cpp
  - src/graph/graph_query_optimizer.cpp
  - src/graph/distributed_graph.cpp
  - src/graph/gpu_traversal.cpp
  - src/graph/knowledge_graph_reasoner.cpp
  - src/graph/ontology_manager.cpp
  - src/graph/explain_plan.cpp
- Verified controls:
  - constraint-gated traversal and bounded planning behavior
  - deterministic fallback handling for advanced execution routes
  - semantic validation and observable error surfaces