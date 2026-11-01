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
SOT_Domain: "security"
Canonical_References:
  - "include/search/hybrid_search.h"
  - "include/search/distributed_hybrid_search.h"
  - "include/search/layered_retrieval_orchestrator.h"
  - "src/search/SECURITY.md"
Related_Issue: "#7378"
Related_Milestone: "DOC-WEEKLY-2026-W44"
---

Exact model backend not verifiable from final document text alone.

# Search Module Trust and Safety Notes

This document captures the trust boundaries, operational risks, and control points for the search module. It complements `src/search/SECURITY.md` and should be read alongside the implementation and API contracts.

This note is not an authentication/authorization or adversarial-input assessment.
The inspected search interfaces accept caller-provided queries and injected
backend clients; **TODO:** document identity, authorization, tenant-isolation,
and prompt-injection controls from their owning modules before asserting them.

## Trust boundaries

The search module assumes trust boundaries between these areas:

- caller-supplied query text and vector payloads
- full-text and vector backend services
- remote shard services used in distributed retrieval
- LLM rerankers used as optional ranking services
- internal configuration values such as candidate caps, timeouts, and weights

The search interfaces expose query and backend boundaries, but the inspected
contracts do not establish how callers authenticate/authorize queries or how
LLM prompt content is screened. Treat these as unresolved boundary evidence,
not as controls supplied by this module.

## Risks

### 1. Query amplification and resource misuse

Unbounded candidate counts or large vector dimensions can create expensive search work. The module mitigates this with validation in `HybridSearch` and guardrails in `LayeredRetrievalOrchestrator`.

### 2. Silent degradation

A major risk is a partially failed search silently passing as a full success.
The current hybrid path exposes selected `SearchStats` fields, but does not
populate the declared error-code taxonomy; see `API.md` for the diagnostic gap.

### 3. External dependency failure

The module depends on backends that may fail or become slow: full-text indexes, ANN/vector engines, remote shards, and LLM rerank services. The implementation is explicitly designed to degrade through fallback states rather than crash the query path.

### 4. Ranking drift

LLM reranking adds semantic quality but also may be inconsistent or unavailable. The module handles this by preserving the original ranking when rerank is not feasible.

## Controls in place

- validation of search config values in constructors
- a declared `SearchErrorCode` taxonomy (not currently wired into the inspected search implementation)
- partial-result flags on degraded queries
- shard request timeouts and layered wait deadlines (timed-out layered work is not cancelled)
- candidate count and prompt-size guardrails
- LLM reranker fallback to the original score ordering

## Incident handling expectations

When a search incident is reported, the operating team should determine:

- whether the failure was local, distributed, or external-service related
- whether the result was partial and how that was reported
- whether a configuration or health issue triggered a guardrail or timeout
- whether the failing path changed ranking quality or result completeness

## Review principle

The trust model favors visible degradation, but observability is incomplete:
the hybrid implementation populates only selected stats fields, and the
declared error taxonomy is not consumed by the inspected search source. Do not
assume every failure class is surfaced as a code.

## Related docs

- `src/search/SECURITY.md`
- `src/search/OPERATIONS.md`
- `src/search/RUNBOOK.md`
- `src/search/API.md`
- `src/search/ARCHITECTURE.md`
