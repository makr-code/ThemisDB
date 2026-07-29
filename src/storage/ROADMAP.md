# Storage Module Roadmap

<!-- Status: [ ] open  [~] in progress  [x] done  [I] issue  [P] PR  [?] blocked  [!] unclear -->
<!-- Status: current | validated: 2026-07-19 -->
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
- [x] Freeze storage module API contract — WAL durability, MVCC snapshot isolation, crash-recovery, backup/PITR, tiering transparency, error taxonomy (include/storage/storage_api_contract.h) (Target: Q3 2026)
- [x] Define explicit StorageErrorCode taxonomy (WAL_WRITE_FAILED, CHECKPOINT_FAILED, RECOVERY_INCOMPLETE, PITR_INVALID_TIMESTAMP, COMPACTION_ABORTED, STORAGE_EXHAUSTED, …) (Target: Q3 2026)

### Phase 2: Core Implementation
- [ ] complete hardening for WAL/MVCC and backup/PITR internals (Target: Q4 2026)
- [ ] align tiered/blob/redundancy behavior to bounded runtime contracts (Target: Q4 2026)

### Phase 3: Error Handling and Edge Cases
- [ ] standardize fail-safe behavior for replay faults, storage pressure, and recovery errors (Target: Q4 2026)
- [ ] unify diagnostics across persistence, maintenance, and recovery incident classes (Target: Q4 2026)

### Phase 4: Tests
- [x] Contract-hardening focused tests STR-01..STR-16 covering WAL durability, MVCC isolation, crash-recovery, and PITR invariants (tests/storage/test_storage_contract_hardening_focused.cpp) (Target: Q4 2026)
- [x] Expand focused regressions for replay, PITR, and tiered/blob edge scenarios (Target: Q4 2026)
- [x] Extend deterministic stress fixtures for mixed write + maintenance workloads (Target: Q4 2026)

### Phase 5: Performance and Hardening
- [x] Lock benchmark-backed release gates for storage hot paths: SGRG-01..SGRG-06 in benchmarks/storage/bench_storage_release_gates.cpp (WAL throughput ≥100k ops/s, MVCC read p99≤100µs, MVCC write p99≤500µs, checkpoint p99≤10ms, tiering decision p99≤50µs, compaction check p99≤100µs) (Target: Q4 2026)
- [x] Validate p95/p99 and throughput behavior against release baselines (Target: Q4 2026)

### Phase 6: Documentation and Acceptance
- [x] core storage module docs aligned to source-verifiable behavior
- [x] roadmap/future planning separated from historical changelog entries
- [x] Contract header frozen: include/storage/storage_api_contract.h
- [x] Contract-hardening tests delivered: tests/storage/test_storage_contract_hardening_focused.cpp (STR-01..STR-16)
- [x] Release-gate benchmarks delivered: benchmarks/storage/bench_storage_release_gates.cpp (SGRG-01..SGRG-06)

## Production Readiness Checklist

- [x] core storage surfaces documented and source-verified
- [x] module-level security and failure behavior documented
- [x] benchmark mapping documented in performance expectations
- [x] Contract header frozen: include/storage/storage_api_contract.h (Phase 1)
- [x] Contract-hardening tests: tests/storage/test_storage_contract_hardening_focused.cpp (Phase 4, STR-01..STR-16)
- [x] Release-gate benchmarks: benchmarks/storage/bench_storage_release_gates.cpp (Phase 5, SGRG-01..SGRG-06)
- [x] Benchmark CMakeLists registered: benchmarks/storage/CMakeLists.txt
- [ ] remaining hardening tasks closed for durability/recovery edge paths
- [ ] release benchmark stabilization complete

## Known Issues and Limitations

- runtime behavior depends on storage configuration, backend profile, and workload shape.
- selected replay/recovery/tiering edge scenarios need continued hardening.
- benchmark depth should continue expanding for advanced storage workloads.

## Breaking Changes

No breaking storage contract planned. Any contract-breaking change requires migration notes and changelog entry before merge.