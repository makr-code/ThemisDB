> **Roadmap-Hinweis:** Vage Bullets ohne Akzeptanzkriterien in Checkbox-Tasks ueberfuehren. Format: `- [ ] <Task> (Target: <Q/Jahr>)`.

<!-- Status: [ ] open  [~] in progress  [x] done  [I] Issue  [P] PR  [?] blocked  [!] unclear -->

# Transaction Module Roadmap

## Current Status
Production-grade transaction stack with ACID lifecycle management, MVCC integration, savepoints, distributed coordination paths, and audit/batching utilities in active use.

## In Progress
- [~] Transaction hardening wave for distributed safety, timeout semantics, and recovery guarantees (Target: Q3 2026)
  - [ ] Complete remaining cross-shard failure-injection coverage for coordinator and participant transitions (Target: Q3 2026)
  - [ ] Tighten timeout and rollback determinism under sustained contention and mixed workloads (Target: Q3 2026)

## Planned Features

### Short-term (3-6 months)
- [ ] Harden coordinator crash-recovery and in-doubt transaction reconciliation policies (Target: Q4 2026)
- [ ] Expand transaction diagnostics and explainability for lock/queue/latency bottlenecks (Target: Q4 2026)
- [ ] Strengthen SAGA orchestration safeguards for partial remote failures and retries (Target: Q4 2026)

### Mid-term (6-12 months)
- [ ] Advance distributed transaction throughput hardening without weakening safety invariants (Target: Q1 2027)
- [ ] Extend OCC and serializable conflict telemetry to improve operator tuning loops (Target: Q1 2027)
- [ ] Expand audit/export integration hardening for large retention windows (Target: Q1 2027)

## Implementation Phases

### Phase 1: Lifecycle and Isolation Safety
- [ ] Re-validate transaction lifecycle invariants across begin/prepare/commit/abort paths (Target: Q3 2026)
- [ ] Harden isolation-level edge-case behavior under mixed read/write contention (Target: Q3 2026)

### Phase 2: Distributed Coordination Hardening
- [ ] Strengthen distributed coordinator timeout, retry, and participant liveness semantics (Target: Q4 2026)
- [ ] Add deterministic regression packs for in-doubt and recovery flows (Target: Q4 2026)

### Phase 3: SAGA and Compensation Reliability
- [ ] Expand compensation idempotency and ordering verification under fault injection (Target: Q4 2026)
- [ ] Validate remote-step orchestration behavior under partial network degradation (Target: Q4 2026)

### Phase 4: Performance and Operational Hardening
- [ ] Re-baseline transaction throughput and tail-latency envelopes across representative profiles (Target: Q1 2027)
- [ ] Ensure audit and batching overhead remains bounded under peak ingest and concurrency (Target: Q1 2027)

### Phase 5: Documentation and Release Readiness
- [ ] Keep transaction docs source-aligned with explicit sourcecode verification evidence per cycle (Target: ongoing)
- [ ] Keep completed roadmap items exclusively in changelog (Target: ongoing)

## Production Readiness Checklist
- Status: Tracking in progress
- Nachweise: transaction focused tests, distributed transaction tests, SAGA tests, performance suites
- Hinweis: Abgeschlossene Arbeit wird ausschliesslich in CHANGELOG dokumentiert.

## Known Issues and Limitations
- Some distributed failure envelopes still need broader regression evidence for long-running degraded conditions.
- Coordinator and remote orchestration paths require additional benchmark-backed operational limits.
- Advanced throughput optimizations remain gated behind safety-first verification criteria.

## Breaking Changes
- Transaction public APIs in active major lines remain additive-first.
- Any future behavioral changes requiring migration must be versioned and documented in changelog/migration notes.
