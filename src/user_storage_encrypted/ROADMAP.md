# User Storage Encrypted Module Roadmap

<!-- Status: [ ] open  [~] in progress  [x] done  [I] issue  [P] PR  [?] blocked  [!] unclear -->
<!-- Status: current | validated: 2026-05-31 -->
<!-- Links: README.md · ARCHITECTURE.md · FUTURE_ENHANCEMENTS.md -->

## Current Status

Production-usable encrypted user-storage behavior exists for gocryptfs-backed backend lifecycle, key derivation, rotation scheduling, and multi-level encrypted storage orchestration.

## In Progress

- [~] hardening mount and unmount failure handling across hostile or degraded host environments (Target: Q3 2026)
- [~] tightening scheduler reliability and recovery diagnostics for rotation execution paths (Target: Q3 2026)
- [~] aligning benchmark-backed release expectations to real encrypted storage hot paths (Target: Q3 2026)

## Planned Features

### Short-term (3-6 months)
- [ ] expand integration coverage for end-to-end create, mount, write, unmount, and remount lifecycle behavior (Target: Q4 2026)
- [ ] harden path validation and error reporting for encrypted container operations (Target: Q4 2026)
- [ ] improve operator-facing observability for mount and rotation incidents (Target: Q4 2026)

### Mid-term (6-12 months)
- [ ] extend per-user isolation and quota enforcement behavior without widening failure domains (Target: Q1 2027)
- [ ] re-baseline p95/p99 envelopes for encrypted mount lifecycle workloads (Target: Q1 2027)
- [ ] deepen resilience tests for sustained multi-tier encrypted storage operation (Target: Q1 2027)

## Implementation Phases

### Phase 1: Design / API Contract
- [x] freeze backend, key derivation, and scheduler contracts for the current major line (Target: Q3 2026) — evidence: include/user_storage_encrypted/user_storage_encrypted_api_contract.h
- [x] define explicit error taxonomy for mount, unmount, and rotation failures (Target: Q3 2026) — evidence: include/user_storage_encrypted/user_storage_encrypted_api_contract.h

### Phase 2: Core Implementation
- [ ] complete backend and scheduler hardening for adverse host-environment behavior (Target: Q4 2026)
- [ ] align multi-tier orchestration to bounded per-level failure contracts (Target: Q4 2026)

### Phase 3: Error Handling and Edge Cases
- [ ] standardize fail-safe behavior for unavailable backend, invalid path, and rotation callback failures (Target: Q4 2026)
- [ ] unify diagnostics across backend, key-management, and orchestration incidents (Target: Q4 2026)

### Phase 4: Tests
- [x] expand focused regressions for mount-state, key derivation, and scheduler edge scenarios (Target: Q4 2026) — evidence: tests/user_storage_encrypted/test_user_storage_encrypted_contract_hardening_focused.cpp
- [x] extend integration tests for full encrypted storage lifecycle flows (Target: Q4 2026) — evidence: tests/user_storage_encrypted/test_user_storage_encrypted_contract_hardening_focused.cpp

### Phase 5: Performance and Hardening
- [x] lock benchmark-backed release gates for encrypted mount lifecycle hot paths (Target: Q4 2026) — evidence: benchmarks/user_storage_encrypted/bench_user_storage_encrypted_release_gates.cpp
- [x] validate p95/p99 latency and throughput behavior against release baselines (Target: Q4 2026) — evidence: benchmarks/user_storage_encrypted/bench_user_storage_encrypted_release_gates.cpp

### Phase 6: Documentation and Acceptance
- [x] core user_storage_encrypted module docs aligned to source-verifiable behavior
- [x] roadmap/future planning separated from historical changelog entries

## Production Readiness Checklist

- [x] core encrypted storage surfaces documented and source-verified
- [x] module-level security and failure behavior documented
- [x] benchmark mapping documented in performance expectations
- [x] release benchmark stabilization complete
- [x] end-to-end lifecycle integration coverage broadened

## Known Issues and Limitations

- runtime behavior depends on host gocryptfs and FUSE availability.
- backend and scheduler edge handling still need broader hardening coverage.
- benchmark depth should continue expanding beyond current mount-latency-focused cases.

## Breaking Changes

No breaking contract change planned. Any change to backend, key, or tier-orchestration contracts requires migration notes and changelog entry before merge.