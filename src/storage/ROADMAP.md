# Storage Module Roadmap

<!-- Status: [ ] open  [~] in progress  [x] done  [I] issue  [P] PR  [?] blocked  [!] unclear -->
<!-- Status: current | validated: 2026-05-31 -->
<!-- Links: README.md · ARCHITECTURE.md · FUTURE_ENHANCEMENTS.md -->

## Current Status

Production-capable storage runtime exists for durable persistence, MVCC/WAL lifecycle behavior, backup/PITR flows, blob/tiering behavior, and storage audit/integrity surfaces.

## In Progress

- [~] hardening failure-path behavior under sustained write/load and maintenance overlap (Target: Q3 2026)
- [~] improving diagnostics consistency across storage, replay, and recovery stages (Target: Q3 2026)
- [~] stabilizing benchmark-backed release guardrails for storage hot paths (Target: Q3 2026)

## Planned Features

### Short-term (3-6 months)
- [ ] tighten deterministic behavior under heavy WAL replay and compaction pressure (Target: Q4 2026)
- [ ] expand stress coverage for blob/tiering and PITR edge scenarios (Target: Q4 2026)
- [ ] improve operator-facing diagnostics for recovery and maintenance incidents (Target: Q4 2026)

### Mid-term (6-12 months)
- [ ] re-baseline p95/p99 envelopes for write/replay/recovery-sensitive paths (Target: Q1 2027)
- [ ] broaden benchmark depth for mount-latency and storage allocator edge paths (Target: Q1 2027)
- [ ] harden long-run reliability under sustained mixed read/write pressure (Target: Q1 2027)

## Implementation Phases

### Phase 1: Design / API Contract
- [ ] freeze storage wrapper/engine/recovery contracts for current major line (Target: Q3 2026)
- [ ] define explicit error taxonomy for durability and recovery incident classes (Target: Q3 2026)

### Phase 2: Core Implementation
- [ ] complete hardening for WAL/MVCC and backup/PITR internals (Target: Q4 2026)
- [ ] align tiered/blob/redundancy behavior to bounded runtime contracts (Target: Q4 2026)

### Phase 3: Error Handling and Edge Cases
- [ ] standardize fail-safe behavior for replay faults, storage pressure, and recovery errors (Target: Q4 2026)
- [ ] unify diagnostics across persistence, maintenance, and recovery incident classes (Target: Q4 2026)

### Phase 4: Tests
- [ ] expand focused regressions for replay, PITR, and tiered/blob edge scenarios (Target: Q4 2026)
- [ ] extend deterministic stress fixtures for mixed write + maintenance workloads (Target: Q4 2026)

### Phase 5: Performance and Hardening
- [ ] lock benchmark-backed release gates for storage hot paths (Target: Q4 2026)
- [ ] validate p95/p99 and throughput behavior against release baselines (Target: Q4 2026)

### Phase 6: Documentation and Acceptance
- [x] core storage module docs aligned to source-verifiable behavior
- [x] roadmap/future planning separated from historical changelog entries

## Production Readiness Checklist

- [x] core storage surfaces documented and source-verified
- [x] module-level security and failure behavior documented
- [x] benchmark mapping documented in performance expectations
- [ ] remaining hardening tasks closed for durability/recovery edge paths
- [ ] release benchmark stabilization complete

## Known Issues and Limitations

- runtime behavior depends on storage configuration, backend profile, and workload shape.
- selected replay/recovery/tiering edge scenarios need continued hardening.
- benchmark depth should continue expanding for advanced storage workloads.

## Breaking Changes

No breaking storage contract planned. Any contract-breaking change requires migration notes and changelog entry before merge.