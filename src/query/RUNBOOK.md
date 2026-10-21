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

# Query Module Runbook

## Purpose

This runbook covers the failure modes and recovery actions that matter most for ThemisDB's Query module when a production query begins failing, timing out, degrading, or producing ambiguous results.

## High-risk symptoms

Operators should treat the following as operational escalation triggers:

- repeated 429/timeout responses on query endpoints,
- planner path changes that sharply increase latency,
- cache hit rate dropping unexpectedly after a deployment,
- distributed query failures that affect multiple shards or clusters,
- continuous query loops failing to drain or cancel cleanly,
- large result sets or semantic cache misses causing memory pressure.

## Recovery sequence

### 1. Confirm the failure surface

Determine whether the issue is in:

- parser/syntax validation,
- optimizer or plan selection,
- execution runtime, 
- cache reuse,
- federation or shard communication,
- cancellation or timeout enforcement.

The key question is not only whether the query fails, but which part of the pipeline is creating the failure.

### 2. Reduce blast radius

If the root issue is unclear, temporarily reduce query concurrency or disable the more expensive or approximate cache path before attempting a deeper change. This keeps the system stable while the root cause is isolated.

### 3. Identify cache and planner side effects

Before changing a query path, inspect whether the failure is related to:

- cache pollution,
- dependency invalidation failure,
- optimizer regression,
- skewed cardinality or selectivity estimates,
- a federated path returning partial results.

### 4. Roll back the smallest effective change

When a recent change is suspected, revert or disable the smallest planner, cache, or federation change all the way down to the boundary that introduced the regression. Do not roll back unrelated storage or network changes unless they have been directly implicated.

## Common failure modes

### Parser or syntax failure

Symptoms:

- malformed AQL or invalid expression tokenization,
- parse rejection for statements that used to work.

Response:

- validate query syntax at the boundary,
- confirm whether compatibility or language grammar changes introduced the break,
- review the parser and safety validator contract before changing execution behavior.

### Optimizer regression

Symptoms:

- plan cost and selection behavior shift unexpectedly,
- previously efficient queries become significantly slower,
- distributed query plans become unstable or high latency.

Response:

- compare plan choice against the prior expected strategy,
- check cardinality and selectivity assumptions,
- reduce further optimizer churn until the regression is isolated.

### Cache degradation

Symptoms:

- hit rate drops sharply,
- TTL or invalidation behavior becomes inconsistent,
- memory usage spikes when large results are cached.

Response:

- clear the affected cache layer,
- validate dependency and TTL behavior,
- inspect memory capacity and result-size policies before re-enabling the cache.

### Federation timeout or shard failure

Symptoms:

- remote shard query times out or returns partial results,
- retries or merge paths become noisy,
- result set integrity is unclear across shards.

Response:

- reduce concurrency or isolate offending shards,
- verify timeout and retry metadata,
- inspect merge behavior and result validation before re-enabling broad federation.

## Escalation policy

Escalate to the query or distributed runtime owner when:

- repeated failures affect multiple workloads,
- the root cause spans planner, cache, and federation layers,
- there is a data correctness or partial-result risk,
- a fix requires changing the public query contract or runtime safety boundary.

## Post-incident expectations

After recovery, capture:

- the failing query pattern,
- the execution boundary involved,
- the mitigations used,
- the lasting code or configuration change required to prevent recurrence.

The Query module should be treated as tightly coupled to operational evidence. Recovery is not complete until the root cause and valid fallback are both explained.
