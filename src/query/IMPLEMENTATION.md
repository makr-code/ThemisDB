---
Author: "ThemisDB Contributors"
Created: "2026-10-21"
Last Updated: "2026-10-21"
Status: "active"
AI_Used: true
AI_Models: "MAI-Code-1.1-Flash"
AI_Contributor: "MAI-Code-1.1-Flash"
Human_Review: true
Human_Author: "ThemisDB Contributors"
---

Exact model backend not verifiable from final document text alone.

# Query Module Implementation Notes

## Purpose

This document describes the implementation realities of the Query module as they exist in the source tree. It is meant to answer: how does the module behave at execution time, what invariants matter, and which edge cases must remain explicit.

## Core implementation structure

The Query module is currently organized around a few concrete responsibilities:

- parsing and validation (`aql_parser.cpp`, `aql_parser_json.cpp`, `aql_safety_validator.cpp`),
- translation to execution structures (`aql_translator.cpp`),
- planner and optimizer behavior (`query_optimizer.cpp`, `adaptive_optimizer.cpp`, `optimizer_cost_model.cpp`),
- execution runtime (`query_engine.cpp`, `result_stream.cpp`, `let_evaluator.cpp`, `window_evaluator.cpp`),
- cache and semantic reuse (`query_cache.cpp`, `query_cache_manager.cpp`, `semantic_cache.cpp`),
- federation and distributed execution (`query_federation.cpp`, `cross_cluster_federation.cpp`),
- runtime controls (`query_canceller.cpp`, `continuous_query_engine.cpp`).

## Invariants

The implementation is strongest when these invariants hold:

1. Query input is parsed before execution.
   - The parser is the canonical syntax boundary. Malformed input should be rejected early rather than processed implicitly.

2. Planning happens before execution.
   - The optimizer chooses a strategy, but the execution layer still owns the actual call path and cancellation semantics.

3. Access checks are explicit.
   - In `QueryEngine`, collection access checks are enforced via callback logic when configured. Hidden access bypasses are treated as a correctness violation.

4. Runtime work stays bounded.
   - Query result streaming, cancellation, TTLs, memory caps, and federation timeouts are not optional quality-of-life features; they are part of the execution contract.

5. Cache reuse remains safe.
   - Exact caches operate under dependency and TTL semantics. Semantic caches require similarity thresholds and valid index state to be considered safe.

## Edge cases that matter

### Malformed AQL or invalid syntax

The parser should reject malformed expressions, unclosed calls, invalid tokens, and bad operator sequences before execution. Failures should remain deterministic and explainable to both developers and operators.

### Null or empty query input

The cache and parser surfaces explicitly reject empty or invalid query contents. This is a major guardrail because empty input often hides upstream validation bugs or malformed client requests.

### Oversized result sets

The exact-cache layer enforces result-size and memory-bound checks before inserting entries. This prevents a large result set from damaging cache stability or causing memory pressure across the query runtime.

### Expired or invalid cache entries

TTL and dependency invalidation are required behaviors for cache correctness. Expired entries should be evicted cleanly rather than reused as valid results.

### Federated failure and partial results

Distributed execution is a separate failure domain from local execution. Timeout, retry metadata, and merge behavior need to be explicit so that partial or degraded results are not mistaken for full correctness.

### Cancellation and long-running queries

Continuous queries and query cancellation must operate cooperatively. A canceled or timed-out query should not leave runtime state behind in a partially updated or inconsistent shape.

## Implementation guidance

When changing the Query module, the safest approach is to preserve these boundaries:

- keep parsing and validation separate from execution,
- keep planner and runtime concerns separated,
- keep cache behavior explicit and measurable,
- keep federation and local execution boundaries visible,
- keep cancellation and timeout behavior documented with the code path that changes.

Any change that makes the query path harder to explain or harder to test should be treated as a design risk, not just a refactor.
