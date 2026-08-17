# Utils Module Roadmap

<!-- Status: [ ] open  [~] in progress  [x] done  [I] issue  [P] PR  [?] blocked  [!] unclear -->
<!-- Status: current | validated: 2026-08-17 -->
<!-- Links: README.md · ARCHITECTURE.md · FUTURE_ENHANCEMENTS.md -->

## Current Status

Production-usable shared utility behavior exists for observability, privacy processing, key helpers, compression, concurrency, tracing, and general reusable support code used across ThemisDB.

## In Progress

- [~] hardening privacy and audit helper behavior for edge-case and overload scenarios (Target: Q3 2026)
- [~] tightening diagnostics consistency across shared helper failures and degradation paths (Target: Q3 2026)
- [~] aligning benchmark-backed release expectations to the broad utils hot-path surface (Target: Q3 2026)

## Planned Features

### Short-term (3-6 months)
- [ ] expand targeted regressions for privacy, audit, and runtime helper edge cases (Target: Q4 2026)
- [ ] improve operator-visible diagnostics for shared helper overload and fallback conditions (Target: Q4 2026)
- [ ] tighten lifecycle and documentation around crypto and key-management helper contracts (Target: Q4 2026)

### Mid-term (6-12 months)
- [ ] broaden benchmark depth for additional utility hot paths where justified by release risk (Target: Q1 2027)
- [ ] extend shared-helper resilience validation under sustained concurrent load (Target: Q1 2027)
- [ ] refine compatibility guarantees for widely consumed utility APIs (Target: Q1 2027)

## Implementation Phases

### Phase 1: Design / API Contract
- [x] freeze widely consumed helper contracts for current major line consumers (Target: Q3 2026)
- [x] define explicit error taxonomy for audit, privacy, and runtime utility failure classes (Target: Q3 2026)

### Phase 2: Core Implementation
- [~] complete remaining hardening for shared utility hotspots with broad module fan-out (Target: Q4 2026)
  - [x] 2.1: Error taxonomy definition (64 codes, 7300-7363) - COMPLETE
  - [x] 2.2: Error registry implementation (audit, privacy, key, compression, runtime) - COMPLETE  
  - [~] 2.3: Observability plane hardening (audit_logger, logger, tracing, saga_logger) - IN PROGRESS
    - [x] 2.3a: audit_logger RAII for POSIX file descriptors (FdGuard, eliminates raw close leaks) - COMPLETE 2026-08-17
  - [ ] 2.4: Privacy & Key plane hardening (pii, hkdf, pki) - QUEUED
  - [~] 2.5: Compression & Runtime plane hardening (codecs, thread_pool, rate_limiter) - IN PROGRESS
    - [x] 2.5a: thread_pool_manager bounded-shutdown joins (joinThreadWithin) - COMPLETE 2026-08-17
    - [x] 2.5b: thread_pool_manager getStatistics() data-race guard - COMPLETE 2026-08-17
    - [x] 2.5c: rate_limiter acquire() cv_.wait_for replacing explicit unlock/lock - COMPLETE 2026-08-17
    - [x] 2.5d: rate_limiter try_acquire_for() timed acquisition added - COMPLETE 2026-08-17
    - [x] 2.5e: http_client_pool bounded-shutdown joins (joinThreadWithin) - COMPLETE 2026-08-17
    - [x] 2.5f: grpc_channel_pool explicit lock.lock() replaced with exception-safe scope guard - COMPLETE 2026-08-17
  - [ ] 2.6: Documentation and acceptance gates - QUEUED
- [~] align degradation paths to predictable, module-safe contracts (Target: Q4 2026)
  - [~] 2.7: Explicit error codes for all hotspots (7300-7363) - IN PROGRESS
  - [ ] 2.8: Graceful degradation for external service failures - QUEUED
  - [ ] 2.9: Bounded resource checks for all high-fan-out helpers - QUEUED
  - [ ] 2.10: Doxygen error contracts for public APIs - IN PROGRESS

### Phase 3: Error Handling and Edge Cases
- [~] standardize fail-safe behavior across privacy, crypto, compression, and observability helpers (Target: Q4 2026)
  - [x] 3.1: Unified error contract framework (error_contracts.h/cpp) - COMPLETE
  - [x] 3.2: Error code taxonomy (9000-9099, 90 codes across 9 subsystems) - COMPLETE
  - [x] 3.3: Incident categorization for operators (15 categories) - COMPLETE
  - [x] 3.4: ErrorContext and diagnostic logging helpers - COMPLETE
  - [ ] 3.5: Apply error contracts to observability components (audit_logger, logger, tracing, saga_logger)
  - [ ] 3.6: Apply error contracts to privacy components (pii_detector, detection engines)
  - [ ] 3.7: Apply error contracts to crypto components (hkdf_helper, hkdf_cache, pki_client)
  - [ ] 3.8: Apply error contracts to compression components (zstd_codec, lz4_codec, serialization)
  - [ ] 3.9: Apply error contracts to runtime services (thread_pool_manager, rate_limiter, connection pools)
- [ ] unify diagnostics and incident categorization for shared-helper failures (Target: Q4 2026)
  - [x] 3.10: Incident categorizer with 15 operator-visible categories - COMPLETE
  - [x] 3.11: Structured logging with ErrorContext - COMPLETE
  - [ ] 3.12: Update component implementations to use new diagnostics

### Phase 4: Tests
- [x] expand focused regressions for benchmark-mapped utility hotspots and edge scenarios (Target: Q4 2026)
- [ ] extend concurrency and stress validation for shared helper fan-out (Target: Q4 2026)

### Phase 5: Performance and Hardening
- [x] lock benchmark-backed release gates for mapped utility hotspots (Target: Q4 2026)
- [x] validate p95/p99 and throughput behavior against release baselines (Target: Q4 2026)

### Phase 6: Documentation and Acceptance
- [x] core utils module docs aligned to source-verifiable behavior
- [x] roadmap/future planning separated from historical changelog entries
- [x] gap remediation run documented: 6 critical scanner findings resolved across 5 source files (2026-08-17)
  - thread_pool_manager.cpp: bounded joins + data-race guard
  - http_client_pool.cpp: bounded joins in destructor
  - grpc_channel_pool.cpp: exception-safe scope re-lock
  - rate_limiter.cpp: cv_.wait_for + try_acquire_for()
  - audit_logger.cpp: FdGuard RAII wrapper for POSIX fds

## Production Readiness Checklist

- [x] core shared helper surfaces documented and source-verified
- [x] module-level security and failure behavior documented
- [x] benchmark mapping documented in performance expectations
- [~] remaining hardening tasks closed for high-fan-out helpers (6 critical gaps closed 2026-08-17; Phase 2.4/3.5-3.9 open)
- [ ] broader release benchmark stabilization complete

## Known Issues and Limitations

- the utils module covers a broad surface and requires continued curation of benchmark depth.
- some helpers depend on host libraries or external services and therefore need explicit degradation coverage.
- shared helper misuse can amplify impact across consumers if failure contracts drift.

## Breaking Changes

No breaking shared-helper contract change planned. Any consumer-visible API change requires migration notes and changelog entry before merge.

## Program Execution Model — Wave Context

This module is a **contributing module** in the program-level Wave A → B → C → D execution model.
It does not own a primary wave deliverable but must remain `release_critical`-green throughout all waves
and must deliver Wave D operability improvements in Q1 2027.
See [`../../ROADMAP.md`](../../ROADMAP.md) for the full wave model and exit criteria.

### Wave D Contribution for `utils`
- [ ] Deliver or validate distributed tracing, high-cardinality stress coverage, exporter reliability, and operator remediation hints as applicable to this module (Target: Q1 2027)
- [ ] Contribute to or validate long-duration soak test coverage for this module's primary paths (Target: Q1 2027)
- [ ] Ensure runbook coverage for operator-critical scenarios in this module (Target: Q1 2027)

### Cross-Wave Requirements
- `release_critical` CI must remain green on `develop` throughout all waves (Target: ongoing)
- p95/p99 benchmarks must be refreshed on representative hardware before Wave D sign-off (Target: Q1 2027)
- No behavioral regression may be introduced into modules in Wave A/B/C scope from changes in this module.

### Program-Level Success Criteria (contribution)
- [ ] This module's distributed/acceleration paths fail closed (Target: Q1 2027)
- [ ] Benchmark-backed p95/p99 baselines exist on representative hardware (Target: Q1 2027)
- [ ] Operator-critical paths have diagnostics, alerts, and runbooks (Target: Q1 2027)
