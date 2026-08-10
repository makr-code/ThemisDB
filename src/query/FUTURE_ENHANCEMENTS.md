> **Hinweis:** Vage Eintraege ohne messbares Ziel, Interface-Spezifikation oder Teststrategie mit `<!-- TODO: add measurable target, interface spec, test strategy -->` markieren.

<!-- Status: current | validated: 2026-05-31 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md -->

# Query Module - Future Enhancements

## Scope
- Reliability and performance hardening of parser, optimizer, execution, and federation paths.
- Safety and governance improvements for multi-model query processing.
- Operational hardening for long-running and distributed query workloads.

## Design Constraints
- [ ] Query execution must remain fail-safe under malformed input and partial dependency failures (Target: ongoing)
- [ ] Resource limits must be enforced deterministically without data corruption (Target: Q4 2026)
- [ ] Optimization and JIT paths must preserve semantic equivalence to interpreter execution (Target: ongoing)
- [ ] Federation must maintain bounded memory and timeout behavior under degraded peers (Target: Q4 2026)
- [ ] Public query APIs and compatibility layers remain additive-only in active major versions (Target: ongoing)

## Required Interfaces

| Interface | Consumer | Notes |
|---|---|---|
| `AQLParser` | query frontends | parsing safety and bounded depth behavior |
| `QueryOptimizer` | planner | statistics-aware plan generation with stable fallbacks |
| `QueryEngine` execute entry points | server/runtime | access checks and resource limit enforcement |
| `QueryFederation` / `CrossClusterFederation` | distributed execution | bounded fan-out and partial-failure handling |
| `VectorizedExecutionEngine` | analytical execution paths | predictable memory and throughput envelopes |
| `QueryCompiler` | hot-query optimization | correctness-preserving JIT fallback behavior |

## Implementation Notes

### Safety and Validation Hardening
**Priority:** High
**Target:** Q3-Q4 2026

- Continue parser/translator edge-case hardening and maintain bounded-depth guarantees.
- Keep execute entry-point access checks and validation behavior consistent.
- Strengthen error-path observability without exposing sensitive internals.

### Optimizer and Runtime Hardening
**Priority:** High
**Target:** Q4 2026

- Improve resilience of plan selection under partial/stale stats.
- Expand deterministic regression coverage for rewrite, adaptive, and runtime re-optimization paths.
- Tighten JIT/interpreter equivalence and fallback telemetry.

### Block A — Buffer / Cache Telemetry And Profiling
**Priority:** High
**Target:** Q3 2026

- Use `BufferPool::Statistics` as the canonical telemetry surface for query-side cache profiling.
- Track and gate hit rate, miss rate, retained free buffers, release evictions, and pressure during scan/range workloads.
- Restrict cache-aware layout or prefetch changes to measured query paths with before/after latency evidence.

### Block B — Thread-Pool Consolidation
**Priority:** High
**Target:** Q3 2026

- Use `WorkStealingThreadPool` as the canonical query-adjacent execution pool unless a stronger module-local justification exists.
- Track submission volume, rejection/backpressure, queue high-watermark, queue pressure, and scale-up events.
- Expand convergence work from telemetry hardening to adoption across query, federation, and nearby runtime paths.

### Block C — Connection-Pool Rebalancing
**Priority:** High
**Target:** Q3 2026

- Use `WireProtocolConnectionPool` rebalance telemetry for shard/federation hotspot analysis.
- Require deterministic rebalance behavior with cooldown/tolerance guardrails to avoid resize thrash.
- Add uneven-load, failover, and recovery proofs before counting connection-pool rebalancing as production-ready.

### Block D — Cross-Feature Regression Matrix
**Priority:** High
**Target:** Q4 2026

- Start with parser/planner-level cross-feature matrices, then expand into executor/federation/runtime coverage.
- Required matrix lanes: AQL + Geo, AQL + FTS, query + federation, query + limits, query + continuous query, query + retry/timeout/fault injection.
- Grow the matrix toward the roadmap target of 1,000+ tests with explicit zero-v1.x-regression proof.

### Federation Hardening
**Priority:** Medium
**Target:** Q4 2026

- Expand degraded-peer handling (timeouts, partial results, retries) with bounded resource usage.
- Validate shard routing and pruning behavior under failure injection.
- Harden protocol and payload guards for cross-cluster calls.

### Advanced Query Feature Hardening
**Priority:** Medium
**Target:** Q1 2027

- Mature approximate query processing for broader production scenarios.
- Strengthen ML-assisted optimization with strict fallback contracts.
- Expand continuous-query backpressure and persistence safety guarantees.

## Test Strategy
- Focused security/reliability regressions for parser, translator, and execute paths.
- Federation and cross-cluster fault-injection matrix with bounded-memory assertions.
- Performance regressions for vectorized execution, optimizer latency, and JIT hot paths.
- Equivalence tests comparing optimized/JIT outputs with interpreter baseline.
- Resource-pooling regressions must cover hit-rate telemetry, queue pressure, adaptive scale behavior, and rebalance control surfaces.
- Cross-feature query regressions must keep a lightweight parser/planner bootstrap suite green while broader executor/federation coverage scales out.

## Performance Targets
- Maintain stable planner and execution latency envelopes under representative workloads.
- Keep vectorized/federated regressions inside release budget thresholds.
- Keep optimization overhead bounded under high query concurrency.
- Hold query-side buffer telemetry to evidence-backed tuning only; no layout/prefetch changes without baseline comparison.
- Keep thread-pool queue pressure and connection-pool rebalance adjustments observable enough to explain Wave-7 throughput claims.

## Security / Reliability
- Fail closed on invalid critical query state and unsafe execution preconditions.
- Preserve deterministic cancellation/timeout behavior.
- Prevent unbounded growth in long-running and distributed execution paths.
- Require rebalance cooldown/tolerance guardrails so adaptive pool behavior stays predictable under degraded or noisy peers.

## Risk Backlog

### Risk 1: Optimizer drift under stale statistics
**Severity:** High
**Signal:** Plan quality degrades under changing data distributions.
**Mitigation:** stronger fallback rules, regression packs, and telemetry gates.

### Risk 2: Federation degradation under unstable peers
**Severity:** Medium
**Signal:** Partial failures cause latency spikes or memory pressure.
**Mitigation:** tighter timeouts, bounded accumulation, and retry policy hardening.

### Risk 3: Long-running query resource pressure
**Severity:** Medium
**Signal:** sustained streaming/continuous workloads push queue and memory limits.
**Mitigation:** backpressure controls, bounded queues, and persistence safeguards.

## Adoption Scenarios

### Scenario A: Safety-first lane
- Prioritize parser/execute-path safety and access-control invariants.
- Promote only with full regression pass for critical safety controls.

### Scenario B: Performance-first lane
- Prioritize optimizer/vectorized/JIT throughput and latency hardening.
- Promote only with benchmark and equivalence gate pass.

### Scenario C: Federation-first lane
- Prioritize distributed query resilience and bounded-failure behavior.
- Promote only with fault-injection gate pass.
