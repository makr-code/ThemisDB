> **Hinweis:** Vage Eintraege ohne messbares Ziel, Interface-Spezifikation oder Teststrategie mit `<!-- TODO: add measurable target, interface spec, test strategy -->` markieren.

<!-- Status: current | validated: 2026-05-31 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md -->

# Transaction Module - Future Enhancements

## Scope
- Reliability and safety hardening of transaction lifecycle, isolation, and distributed coordination paths.
- Operational and observability hardening for SAGA, OCC, and coordinator-driven flows.
- Performance and resilience hardening for high-concurrency transaction workloads.
- **Completed: Stub #279 — Distributed Transaction Manager Phase-2 fail-closed behavior** (Q3 2026) — all remote participants now receive COMMIT/ABORT decisions with fail-fast validation

## Design Constraints
- [ ] Transaction state transitions must remain deterministic and fail-safe under partial failures (Target: ongoing)
- [ ] Isolation and conflict semantics must remain correctness-first under concurrency pressure (Target: ongoing)
- [ ] Distributed coordinator decisions must be durable and recoverable before external commit effects (Target: Q4 2026)
- [ ] Compensation logic must remain idempotent and replay-safe across retries/recovery (Target: Q4 2026)
- [ ] Public transaction APIs remain additive-only in active major versions (Target: ongoing)

## Required Interfaces

| Interface | Consumer | Notes |
|---|---|---|
| `TransactionManager` / `Transaction` APIs | query/server/runtime layers | core lifecycle and isolation behavior |
| `DistributedTransactionManager` | distributed coordinator paths | prepare/commit/abort and recovery semantics |
| `SAGAOrchestrator` / distributed SAGA interfaces | orchestration/business flows | compensation ordering and remote step behavior |
| `LockManager` and conflict paths | concurrency control | lock acquisition and contention safety |
| `TransactionBatcher` | ingest/write-heavy paths | batching throughput and bounded latency |
| `TransactionAuditor` | audit/compliance operators | append/query/export traceability |

## Implementation Notes

### Lifecycle and Isolation Hardening
**Priority:** High
**Target:** Q3-Q4 2026

- Strengthen edge-case coverage for begin/prepare/commit/abort transitions.
- Harden isolation and conflict handling under mixed high-contention workloads.
- Extend deterministic error-path coverage for timeout and rollback behavior.

### Distributed Coordination Hardening
**Priority:** High
**Target:** Q4 2026

- Expand in-doubt, recovery, and participant-liveness regression packs.
- Tighten coordinator durability guarantees and replay safety.
- Improve observability for distributed transition timelines.

### SAGA and Compensation Hardening
**Priority:** Medium
**Target:** Q4 2026

- Validate compensation idempotency under retry storms and partial remote failures.
- Expand orchestration DAG and dependency-failure test coverage.
- Harden manual intervention and recovery paths.

### Throughput and Operations Hardening
**Priority:** Medium
**Target:** Q1 2027

- Re-baseline throughput/tail-latency envelopes for transaction-heavy profiles.
- Keep audit and batching overhead bounded under peak concurrency.
- Expand benchmark evidence for production guardrail tuning.

## Test Strategy
- Focused transaction lifecycle/isolation regression suites.
- Distributed coordinator and failure-injection matrix for prepare/commit/abort/recovery.
- SAGA compensation and replay-safety regression matrix.
- Performance regressions for throughput, tail latency, and contention envelopes.

## Performance Targets
- Maintain stable transaction tail-latency envelopes under representative mixed workloads.
- Keep throughput regressions inside release budget thresholds.
- Keep batching/audit overhead bounded under sustained peak concurrency.

## Security / Reliability
- Fail closed on invalid transaction states and unsafe transition preconditions.
- Preserve deterministic timeout/cancellation/rollback behavior.
- Prevent unbounded memory growth in queues and coordination structures.

## Risk Backlog

### Risk 1: Distributed in-doubt reconciliation drift
**Severity:** High
**Signal:** inconsistent participant/coordinator completion states after faults.
**Mitigation:** stronger recovery validation, replay guards, and transition telemetry.

### Risk 2: Compensation divergence under repeated retries
**Severity:** Medium
**Signal:** repeated compensation attempts produce inconsistent outcomes.
**Mitigation:** stricter idempotency checks and replay-safety regression packs.

### Risk 3: Contention-driven tail-latency spikes
**Severity:** Medium
**Signal:** p99 latency grows sharply in mixed isolation workloads.
**Mitigation:** contention diagnostics, bounded queue behavior, and tuning guardrails.

## Adoption Scenarios

### Scenario A: Correctness-first lane
- Prioritize isolation, state-transition safety, and deterministic recovery behavior.
- Promote only after full lifecycle and failure-injection gate pass.

### Scenario B: Throughput-first lane
- Prioritize batching and coordinator throughput hardening with safety parity.
- Promote only after benchmark and regression gate pass.

### Scenario C: Distributed-first lane
- Prioritize multi-participant reliability and compensation/replay robustness.
- Promote only after distributed fault-matrix gate pass.
