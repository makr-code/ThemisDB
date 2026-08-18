# Utils Module Roadmap

<!-- Status: [ ] open  [~] in progress  [x] done  [I] issue  [P] PR  [?] blocked  [!] unclear -->
<!-- Status: current | validated: 2026-08-17 -->
<!-- Links: README.md · ARCHITECTURE.md · FUTURE_ENHANCEMENTS.md · IMPLEMENTATION_PLAN_PHASES_2_4.md -->

## Current Status

Production-usable shared utility behavior exists for observability, privacy processing, key helpers, compression, concurrency, tracing, and general reusable support code used across ThemisDB.

## In Progress

- [~] broader release benchmark stabilization complete (Target: Q1 2027)

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
- [x] complete remaining hardening for shared utility hotspots with broad module fan-out (Target: Q4 2026)
  - [x] 2.1: Error taxonomy definition (64 codes, 7300-7363) - COMPLETE
  - [x] 2.2: Error registry implementation (audit, privacy, key, compression, runtime) - COMPLETE  
  - [x] 2.3: Observability plane hardening (audit_logger, logger, tracing, saga_logger) - COMPLETE 2026-08-17
    - [x] 2.3a: audit_logger RAII for POSIX file descriptors (FdGuard, eliminates raw close leaks) - COMPLETE 2026-08-17
  - [x] 2.2c: Privacy plane NER & Regex detection hardening - COMPLETE 2026-08-18
    - [x] 2.2c-1: ner_detection_engine timeout bounds (5000ms), gazetteer validation, fail-closed on unavailable
    - [x] 2.2c-2: regex_detection_engine ReDoS detection, per-pattern timeout, malformed pattern exception handling
  - [x] 2.4: Privacy & Key plane hardening (pii, hkdf, pki) - COMPLETE 2026-08-17
    - [x] 2.4a: pki_client cert pinning fail-closed enforcement via `CURLOPT_PINNEDPUBLICKEY` (hex + sha256// inputs) - COMPLETE 2026-08-17
    - [x] 2.4b: pki_client password callback bounds hardening (`buf`/`size` validation, bounded copy) - COMPLETE 2026-08-17
  - [x] 2.4a (compression): zstd_codec decompression bomb detection (1024x ratio), concurrent safety documented - COMPLETE 2026-08-18
  - [x] 2.4b (compression): lz4_codec worst-case buffer sizing, block-size overflow, library unavailable fallback - COMPLETE 2026-08-18
  - [x] 2.4c (compression): serialization MAX_NESTING_DEPTH=32 stack overflow protection, schema mismatch errors - COMPLETE 2026-08-18
  - [x] 2.5: Compression & Runtime plane hardening (codecs, thread_pool, rate_limiter) - COMPLETE 2026-08-17
    - [x] 2.5a: thread_pool_manager bounded-shutdown joins (joinThreadWithin) - COMPLETE 2026-08-17
    - [x] 2.5b: thread_pool_manager getStatistics() data-race guard - COMPLETE 2026-08-17
    - [x] 2.5c: rate_limiter acquire() cv_.wait_for replacing explicit unlock/lock - COMPLETE 2026-08-17
    - [x] 2.5d: rate_limiter try_acquire_for() timed acquisition added - COMPLETE 2026-08-17
    - [x] 2.5e: http_client_pool bounded-shutdown joins (joinThreadWithin) - COMPLETE 2026-08-17
    - [x] 2.5f: grpc_channel_pool explicit lock.lock() replaced with exception-safe scope guard - COMPLETE 2026-08-17
  - [x] 2.6: Documentation and acceptance gates - COMPLETE 2026-08-17
- [x] align degradation paths to predictable, module-safe contracts (Target: Q4 2026)
  - [x] 2.7: Explicit error codes for all hotspots (7300-7363) - COMPLETE 2026-08-17
  - [x] 2.8: Graceful degradation for external service failures - COMPLETE 2026-08-17
  - [x] 2.9: Bounded resource checks for all high-fan-out helpers - COMPLETE 2026-08-18
    - [x] 2.9a: error_registry concurrent read/write synchronization via `std::shared_mutex` - COMPLETE 2026-08-17
    - [x] 2.9d: High-fan-out helper resource limit documentation (9 headers updated with @note tags) - COMPLETE 2026-08-18
  - [x] 2.10: Doxygen error contracts for public APIs - COMPLETE 2026-08-18

### Phase 3: Error Handling and Edge Cases
- [x] standardize fail-safe behavior across privacy, crypto, compression, and observability helpers (Target: Q4 2026)
  - [x] 3.1: Unified error contract framework (error_contracts.h/cpp) - COMPLETE
  - [x] 3.2: Error code taxonomy (9000-9099, 90 codes across 9 subsystems) - COMPLETE
  - [x] 3.3: Incident categorization for operators (15 categories) - COMPLETE
  - [x] 3.4: ErrorContext and diagnostic logging helpers - COMPLETE
  - [x] 3.5: Apply error contracts to observability components (audit_logger, logger, tracing, saga_logger) - COMPLETE 2026-08-17
    - [x] 3.5a: tracing.cpp logErrorWithContext at initialization failure path - COMPLETE 2026-08-17
    - [x] 3.5b: saga_logger.cpp logErrorWithContext at hash-mismatch and signature-invalid paths - COMPLETE 2026-08-17
  - [x] 3.6: Apply error contracts to privacy components (pii_detector, detection engines) - COMPLETE 2026-08-17
    - [x] 3.6a: pii_detector reload failure emits structured ErrorContext - COMPLETE 2026-08-17
    - [x] 3.6b: pii_detector engine exception emits structured ErrorContext - COMPLETE 2026-08-17
    - [x] 3.6c: pii_detector default engine load failure emits structured ErrorContext - COMPLETE 2026-08-17
  - [x] 3.7: Apply error contracts to crypto components (hkdf_helper, hkdf_cache, pki_client) - COMPLETE 2026-08-17
    - [x] 3.7a: pki_client pinning misconfiguration now fails closed with explicit diagnostics - COMPLETE 2026-08-17
  - [x] 3.8: Apply error contracts to compression components (zstd_codec, lz4_codec, serialization) - COMPLETE 2026-08-17
    - [x] 3.8a: zstd_codec compress/decompress failure emits structured ErrorContext - COMPLETE 2026-08-17
    - [x] 3.8b: lz4_codec compress/decompress failure emits structured ErrorContext - COMPLETE 2026-08-17
    - [x] 3.8c: serialization Decoder bounds violation emits structured ErrorContext - COMPLETE 2026-08-17
  - [x] 3.9: Apply error contracts to runtime services (thread_pool_manager, rate_limiter, connection pools) - COMPLETE 2026-08-17
    - [x] 3.9a: error_registry read/write paths hardened for concurrent access in high-fan-out call paths - COMPLETE 2026-08-17
- [x] unify diagnostics and incident categorization for shared-helper failures (Target: Q4 2026)
  - [x] 3.10: Incident categorizer with 15 operator-visible categories - COMPLETE
  - [x] 3.11: Structured logging with ErrorContext - COMPLETE
  - [x] 3.12: Update component implementations to use new diagnostics - COMPLETE 2026-08-17
    - tracing.cpp, saga_logger.cpp, pii_detector.cpp, zstd_codec.cpp, lz4_codec.cpp, serialization.cpp - COMPLETE 2026-08-17

### Phase 4: Tests
- [x] expand focused regressions for benchmark-mapped utility hotspots and edge scenarios (Target: Q4 2026)
- [x] extend concurrency and stress validation for shared helper fan-out (Target: Q4 2026)
  - [x] Phase 4 stress/concurrency test suite added (UTL-CONC-01..10): ThreadPool, RateLimiter, ErrorRegistry, error_contracts, zstd_codec, lz4_codec - COMPLETE 2026-08-17
- [x] Phase 4.2 new test coverage complete (2026-08-18):
  - test_utils_pii_unicode_edge_cases.cpp: CJK/RTL/combining marks/truncated UTF-8
  - test_utils_lek_rotation_atomic.cpp: dual-generation window verification
  - test_utils_audit_logger_stress_concurrent_writers.cpp: N=8/32/128 concurrent writers
  - test_utils_thread_pool_stress_saturation.cpp: submission saturation + priority ordering
  - test_utils_pii_stream_scanner_stress_parallel.cpp: parallel scan slot stress
  - test_tsan_stress_concurrent.cpp: TSAN concurrency verification for mutex-heavy components
- [x] Phase 4.3 unregistered tests registered in CMakeLists (2026-08-18):
  - test_ner_detection_engine.cpp → NERDetectionEngineFocusedTests
  - test_regex_detection_engine.cpp → RegexDetectionEngineFocusedTests
  - test_serialization.cpp → SerializationFocusedTests
  - test_tsan_stress_concurrent.cpp → TSANConcurrencyStressTests
  - test_phase1_resource_management.cpp → Phase1ResourceManagementTests

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
- [x] gap remediation follow-up: Phase 3.5/3.6/3.8/3.12 error-contract integration + Phase 4 stress test suite (2026-08-17)
  - tracing.cpp: logErrorWithContext at OTel initialization failure
  - saga_logger.cpp: logErrorWithContext at hash-mismatch and PKI signature-invalid paths
  - pii_detector.cpp: logErrorWithContext at reload failure, engine exception, engine load failure
  - zstd_codec.cpp: logErrorWithContext at compress/decompress failure; error_contracts.h added
  - lz4_codec.cpp: logErrorWithContext at compress/decompress failure; error_contracts.h added
  - serialization.cpp: logErrorWithContext at Decoder bounds violation; error_contracts.h added
  - tests/utils/test_utils_stress_concurrency.cpp: Phase 4 stress suite UTL-CONC-01..10 added
- [x] gap remediation: pki_client + error_registry critical/high closure (2026-08-17)
  - pki_client.cpp: cert pinning fail-closed enforcement + password callback bounds hardening
  - error_registry.cpp/h: shared_mutex synchronization for concurrent read/write safety
- [x] Phase 4 gap closure: 5 unregistered test files added to CMakeLists (2026-08-18)
  - test_ner_detection_engine.cpp, test_regex_detection_engine.cpp, test_serialization.cpp
  - test_tsan_stress_concurrent.cpp, test_phase1_resource_management.cpp
  - IMPLEMENTATION_PLAN_PHASES_2_4.md Phase 4 items marked complete

## Production Readiness Checklist

- [x] core shared helper surfaces documented and source-verified
- [x] module-level security and failure behavior documented
- [x] benchmark mapping documented in performance expectations
- [x] remaining hardening tasks closed for high-fan-out helpers (Phase 2.2c and 2.4a-c hardening complete 2026-08-18; total 11 critical/high gap clusters closed across 2026-08-17 and 2026-08-18)
- [x] Phase 2 hardening fully complete (Privacy plane: NER/Regex; Compression plane: ZSTD/LZ4/Serialization; Cross-cutting resource limits documented)
- [~] broader release benchmark stabilization complete (Target: Q1 2027)

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
