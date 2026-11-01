---
Author: "ThemisDB Team"
Created: "2026-10-21"
Last Updated: "2026-10-31"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot coding agent"
Human_Review: false
Human_Author: "unknown"
Source_Level: "level1"
SOT_Domain: "module-behavior"
Canonical_References:
  - "include/search/hybrid_search.h"
  - "include/search/distributed_hybrid_search.h"
  - "include/search/layered_retrieval_orchestrator.h"
  - "src/search/hybrid_search.cpp"
  - "src/search/distributed_hybrid_search.cpp"
Related_Issue: "#7378"
Related_Milestone: "DOC-WEEKLY-2026-W44"
---

Exact model backend not verifiable from final document text alone.

# Search Module Operations Guide

This document describes source-observable signals and operational questions.
The module does not define a deployment-wide health check or alerting policy;
configure those at the owning service boundary.

## Normal operating posture

The search module is expected to:

- maintain bounded retrieval latency under configured limits
- emit explicit degraded-state diagnostics when a backend is partial or unavailable
- return useful results even when one retrieval source fails
- preserve observability via logs, `SearchStats`, and layer-level decision metadata

## Signals to monitor

Operationally relevant signals are produced from the code paths in the module:

- `HybridSearch::search()` logs backend failures and partial-result conditions
- `DistributedHybridSearch` logs remote shard timeout and failure states
- `LlmReranker` logs backend failures and fallback actions
- `LayeredRetrievalOrchestrator` records per-layer decisions and latency metadata

The hybrid stats struct declares more fields than the current implementation
populates: only backend status, partial-result, and candidate counts are
written by `HybridSearch::search()`. Error-code/fusion/reranker fields are not
currently active signals in that path (**TODO:** wire and test them).

Watch for:

- rising partial-result frequency
- high shard failure counts
- repeated reranker fallback events
- layered retrieval timeout decisions
- increased candidate deficits or result counts below configured `k`

## Recovery patterns

### Partial backend failure

When a single backend fails, the module is expected to degrade gracefully. This is an operationally preferred state as long as the degraded condition remains visible and traceable.

### Cross-shard degradation

When remote shards fail, the distributed path should continue with surviving shards and record the issue in the result stats. A full distributed outage is only acceptable when the operational contract explicitly allows it.

### Reranker outage

When the LLM reranker backend is unavailable, the module should return the original fused ranking. The decision is degrade-but-still-useful rather than block the result set.

## Rollback and fail-safe posture

The module is designed for rollback by configuration rather than by hidden compatibility behavior:

- the caller can disable or reconfigure the reranker
- the vector search path may fall back from ANN to the vector index path
- the layered pipeline may skip expensive stages under guardrails
- invalid configuration is rejected at construction time and is not allowed to degrade silently

## Operational questions to answer during incident response

1. Which retrieval source failed first: lexical, vector, distributed, or reranker?
2. Did the query produce a partial result with a well-defined degraded status?
3. Did the module exceed candidate caps, timeout budgets, or guardrails?
4. Were the downstream systems health-checked before rerouting or scaling?

## Escalation rules

Escalate when:

- search calls start returning empty sets despite healthy candidate sources
- distributed merge continuously reaches failure or underflow states
- layered pipeline latency exceeds the modeled budgets repeatedly
- one backend failure cascades into a broad availability problem across multiple search requests

## Related docs

- `src/search/RUNBOOK.md`
- `src/search/ARCHITECTURE.md`
- `src/search/SECURITY.md`
- `src/search/IMPLEMENTATION.md`
- `src/search/ROADMAP.md`
