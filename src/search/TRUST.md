---
Author: "ThemisDB Team"
Created: "2026-10-21"
Last Updated: "2026-10-21"
Status: "active"
AI_Used: true
AI_Models: "GitHub Copilot"
AI_Contributor: "GitHub Copilot"
Human_Review: true
Human_Author: "ThemisDB Team"
---

# Search Module Trust and Safety Notes

This document captures the trust boundaries, operational risks, and control points for the search module. It complements `src/search/SECURITY.md` and should be read alongside the implementation and API contracts.

## Trust boundaries

The search module assumes trust boundaries between these areas:

- caller-supplied query text and vector payloads
- full-text and vector backend services
- remote shard services used in distributed retrieval
- LLM rerankers used as optional ranking services
- internal configuration values such as candidate caps, timeouts, and weights

The module treats untrusted input as a query-path concern, not a security boundary itself. The actual trust boundary is the point where backend availability, quality, and configuration validity can degrade the response semantics.

## Risks

### 1. Query amplification and resource misuse

Unbounded candidate counts or large vector dimensions can create expensive search work. The module mitigates this with validation in `HybridSearch` and guardrails in `LayeredRetrievalOrchestrator`.

### 2. Silent degradation

A major risk is a partially failed search silently passing as a full success. This is why the module uses `SearchStats`, `SearchErrorCode`, and explicit partial-result semantics instead of a blind success-only return path.

### 3. External dependency failure

The module depends on backends that may fail or become slow: full-text indexes, ANN/vector engines, remote shards, and LLM rerank services. The implementation is explicitly designed to degrade through fallback states rather than crash the query path.

### 4. Ranking drift

LLM reranking adds semantic quality but also may be inconsistent or unavailable. The module handles this by preserving the original ranking when rerank is not feasible.

## Controls in place

- validation of search config values in constructors
- explicit `SearchErrorCode` taxonomy for diagnostic clarity
- partial-result flags on degraded queries
- timeout handling for shard and layered operations
- candidate count and prompt-size guardrails
- LLM reranker fallback to the original score ordering

## Incident handling expectations

When a search incident is reported, the operating team should determine:

- whether the failure was local, distributed, or external-service related
- whether the result was partial and how that was reported
- whether a configuration or health issue triggered a guardrail or timeout
- whether the failing path changed ranking quality or result completeness

## Review principle

The trust model for this module is simple: query quality is important, but explicit degradation signals are more important than hidden success. That principle is reflected in the implementation throughout the search sources and in the module's error taxonomy.

## Related docs

- `src/search/SECURITY.md`
- `src/search/OPERATIONS.md`
- `src/search/RUNBOOK.md`
- `src/search/API.md`
- `src/search/ARCHITECTURE.md`
