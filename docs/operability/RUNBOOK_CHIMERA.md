# Runbook: Chimera Hybrid Adapter

<!-- Wave D operability deliverable — chimera module -->
<!-- Source: src/chimera/ROADMAP.md § Wave D Contribution -->

**Module:** `src/chimera/`  
**Version:** 1.0.0 (Wave D, 2026-Q1)  
**Owner:** ThemisDB Chimera Team  
**Labels:** `wave_d;operability;runbook`

---

## Overview

This runbook covers operator response procedures for the five most critical
Chimera hybrid-adapter incident classes. Each scenario includes detection
signals (log patterns and metrics), immediate mitigations, and escalation paths.

---

## Scenario 1 — Hybrid Router Failure

**Log pattern:** `[CHIMERA:RouterFailed]`

### Detection

- Log line: `[CHIMERA:RouterFailed] query_id=<ID> backend=<B> reason=<R>`
- Metric: `chimera_router_failures_total` rising
- Alert: `ChimeraRouterErrorRate` fires when failure rate > 0.1 %

### Immediate actions

1. **Check backend availability.** Verify the targeted backend (Mongo/Neo4j/Qdrant)
   is reachable from the Chimera adapter.
2. **Enable fallback routing.** Set `chimera.fallback_on_router_failure=true` in
   adapter configuration to redirect to the in-process simulation path.
3. **Check compile-time guards.** Ensure the required `THEMIS_CHIMERA_*` build flag
   is set if a real driver is expected.
4. **Review DISPATCH_FAILED (14) error codes** in the adapter logs; these indicate
   a capability-path mismatch.

### Escalation

Router failures affecting > 1 % of queries for > 5 minutes are P2.

---

## Scenario 2 — Fallback Path OOM

**Log pattern:** `[CHIMERA:FallbackOOM]`

### Detection

- Log line: `[CHIMERA:FallbackOOM] fallback_heap_mb=<N> threshold_mb=<T>`
- Metric: `chimera_fallback_heap_mb` approaching container memory limit
- Alert: `ChimeraFallbackHeapHigh`

### Immediate actions

1. **Reduce concurrency on the fallback path.** Lower
   `chimera.fallback_max_concurrent_queries` to shed load.
2. **Profile fallback allocation.** Large per-query allocations in the simulation
   path are the most common cause; check for unbound result-set sizes.
3. **Increase container memory limit** as a temporary measure while a fix is
   prepared.
4. **Disable the fallback path** (`chimera.fallback_enabled=false`) if OOM risk
   threatens the primary adapter.

### Escalation

OOM conditions on the fallback path that cannot be resolved within 10 minutes
require P1 escalation.

---

## Scenario 3 — Query Plan Conflict

**Log pattern:** `[CHIMERA:PlanConflict]`

### Detection

- Log line: `[CHIMERA:PlanConflict] query_id=<ID> plan_a=<A> plan_b=<B>`
- Metric: `chimera_plan_conflict_count_total` nonzero
- Alert: `ChimeraPlanConflict`

### Immediate actions

1. **Identify conflicting plan sources.** The log includes `plan_a` and `plan_b`
   fields showing which planner outputs disagree.
2. **Lock plan selection to primary planner.** Set
   `chimera.force_primary_plan=true` to bypass secondary plan negotiation.
3. **Inspect CAPABILITY_MISMATCH (15) codes** in surrounding log lines — these
   often precede plan conflicts.
4. **Replay the conflicting query** in isolation using the chimera test fixtures.

### Escalation

Recurring plan conflicts for the same query pattern indicate a planner regression
and require a code review before the next release gate.

---

## Scenario 4 — Backend Timeout Cascade

**Log pattern:** `[CHIMERA:BackendTimeout]`

### Detection

- Log line: `[CHIMERA:BackendTimeout] backend=<B> timeout_ms=<T> cascade_depth=<D>`
- Metric: `chimera_backend_timeout_count_total` rising
- Alert: `ChimeraBackendTimeoutCascade`

### Immediate actions

1. **Check the backend endpoint health.** Run connectivity tests against the
   affected backend.
2. **Enable circuit breaker.** Set `chimera.circuit_breaker_enabled=true` and
   configure `chimera.circuit_breaker_threshold_ms` to limit cascade depth.
3. **Reduce query concurrency** to lessen the load on the degraded backend.
4. **Route traffic to an alternate backend** if configured.

### Escalation

Cascade depth > 3 with no recovery is a P1 incident requiring backend on-call
involvement.

---

## Scenario 5 — Result Merge Stall

**Log pattern:** `[CHIMERA:MergeStall]`

### Detection

- Log line: `[CHIMERA:MergeStall] query_id=<ID> stalled_ms=<T> backend=<B>`
- Metric: `chimera_merge_stall_duration_ms` p99 above threshold
- Alert: `ChimeraMergeStallHigh`

### Immediate actions

1. **Check partial result buffers.** Inspect `chimera_pending_merge_count` metric.
2. **Cancel and retry stalled queries.** Use `themis_admin chimera cancel-query <ID>`.
3. **Reduce per-query result-set size limits.** Lower
   `chimera.max_result_rows_per_backend` to prevent unbounded merge buffers.
4. **Verify that all backends returned results.** A missing partial result from
   one backend causes the merge to stall indefinitely without a timeout guard.

### Escalation

Merge stalls persisting > 30 seconds for interactive queries are P2.

---

## Related resources

- `src/chimera/ROADMAP.md` — module roadmap and Wave D closure
- `tests/integration/test_chimera_soak.cpp` — soak test
- `tests/chimera/test_chimera_highcardinality_stress.cpp` — stress test
- `benchmarks/chimera/bench_chimera_dedicated_gates.cpp` — CH-BM-01..04
- `src/chimera/CHIMERA_ADAPTER_CONTRACT.md` — adapter error taxonomy
