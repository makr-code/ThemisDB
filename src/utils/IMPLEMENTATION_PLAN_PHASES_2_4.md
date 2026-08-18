# Implementation Plan: Utils Module Phases 2-4 (Production-Ready)

<!-- Status: planning | created: 2026-08-17 | validated: 2026-08-17 -->
<!-- Links: ROADMAP.md · PRODUCTION_READINESS_CHECKLIST.md · PHASE3_ERROR_CONTRACTS.md -->

## Purpose

This document describes the concrete implementation work required to bring the utils module
from its current Phase 1/5/6-complete state to full production readiness (Phases 2-4 complete).

### Current State Summary

| Phase | Status | Gap |
|---|---|---|
| Phase 1: Design / API Contract | ✓ COMPLETE | – |
| Phase 2: Core Implementation | ✓ COMPLETE 2026-08-18 | – |
| Phase 3: Error Handling | ✓ COMPLETE 2026-08-18 | TSAN CI run pending |
| Phase 4: Tests | ✓ COMPLETE 2026-08-18 | TSAN CI run + coverage measurement pending |
| Phase 5: Performance | ✓ COMPLETE | – |
| Phase 6: Documentation | ✓ COMPLETE 2026-08-18 | – |

### error_contracts.h Adoption State (as of 2026-08-18)

All components adopted:
`error_contracts.cpp`, `error_registry.cpp`, `lz4_codec.cpp`, `pii_detection_engine.cpp`,
`pki_client.cpp`, `retention_manager.cpp`, `zstd_codec.cpp`, `serialization.cpp`,
`tracing.cpp`, `saga_logger.cpp`, `pii_detector.cpp`

Phase 3 header @error_contract tables added:
- observability (audit_logger.h, logger.h, tracing.h, saga_logger.h) — 2026-08-17
- crypto (hkdf_helper.h, hkdf_cache.h, pki_client.h, lek_manager.h) — 2026-08-17
- runtime (thread_pool_manager.h, rate_limiter.h) — 2026-08-17
- compression (zstd_codec.h, lz4_codec.h, serialization.h) — 2026-08-18

---

## Phase 2: Core Implementation Hardening

**Target:** Q4 2026  
**Acceptance gate:** All hardening tasks closed; degradation paths tested

### 2.1 Observability Plane Hardening

**Components:** `audit_logger.cpp`, `logger.cpp`, `saga_logger.cpp`, `tracing.cpp`

**Work items:**
- [x] `audit_logger.cpp`: Verify bounded FIFO queue behavior under load; add explicit capacity checks; document fallback (drop-oldest vs. reject) as a configurable choice; eliminate any race conditions in the append path.
- [x] `logger.cpp`: Add explicit sink-unavailable handling; verify that log failure does not propagate to callers silently; bound the log queue.
- [x] `saga_logger.cpp`: Verify saga-event ordering guarantees under concurrent callers; add explicit error on serialization failure.
- [x] `tracing.cpp`: Verify span lifecycle (begin/end symmetry); add explicit handling when tracing backend is unavailable (fail-open with warning vs. fail-closed configurable).

**Degradation contracts required:**
- [x] 2.8a: audit_logger: backend unavailable → fail-closed with operator log
- [x] 2.8b: logger: sink unavailable → fail-open with local stderr fallback, never silent
- [x] 2.8c: tracing: backend unavailable → fail-open, no-op span, latency bounded

### 2.2 Privacy Plane Hardening

**Components:** `pii_detector.cpp`, `pii_stream_scanner.cpp`, `pii_pseudonymizer.cpp`, `regex_detection_engine.cpp`, `ner_detection_engine.cpp`

**Work items:**
- [x] `pii_detector.cpp`: Verify UTF-8 / CJK / RTL input handling; add explicit truncation behavior for oversized inputs; ensure detection errors return conservative result (no false permits).
- [x] `pii_stream_scanner.cpp`: Bound concurrent scan slots; verify scan timeout behavior; add graceful degradation when regex engine unavailable.
- [x] `pii_pseudonymizer.cpp`: Verify pseudonymization determinism under concurrent callers; add explicit failure when pseudonymization key unavailable (fail-closed).
- [x] `regex_detection_engine.cpp`: Bound regex backtracking; add timeout on individual pattern match; handle malformed regex patterns without panic.
- [x] `ner_detection_engine.cpp`: Bound model inference time; handle model-unavailable gracefully; explicit error on corrupt model artifact.

**Degradation contracts required:**
- [x] 2.8d: pii_stream_scanner: scan timeout → conservative fail-closed (treat as PII present)
- [x] 2.8e: pseudonymizer: key unavailable → reject, never produce plaintext

### 2.3 Key Management Plane Hardening

**Components:** `hkdf_helper.cpp`, `hkdf_cache.cpp`, `lek_manager.cpp`

**Work items:**
- [x] `hkdf_helper.cpp`: Verify key material zeroization on scope exit; add explicit handling for OpenSSL unavailability; document HKDF derivation semantics (salt, info, length contracts) in implementation.
- [x] `hkdf_cache.cpp`: Bound cache size and TTL; verify eviction under memory pressure; ensure no stale key material escapes cache.
- [x] `lek_manager.cpp`: Verify LEK rotation is atomic (no window where two generations are active simultaneously); add explicit error on key store unavailability; bound key-store retry budget.

**Degradation contracts required:**
- [x] 2.8f: hkdf_helper: OpenSSL unavailable → fail-closed (no key derivation)
- [x] 2.8g: lek_manager: key store unavailable → fail-closed, no plaintext fallback

### 2.4 Compression Plane Hardening

**Components:** `zstd_codec.cpp`, `lz4_codec.cpp`, `serialization.cpp`

**Work items:**
- [x] `zstd_codec.cpp`: Verify concurrent encode/decode safety (zstd context reuse); add explicit handling for corrupt compressed data (decompression bomb / checksum failure); bound output buffer growth.
- [x] `lz4_codec.cpp`: Verify output buffer sizing for worst-case input; add explicit error on block-size overflow; handle codec library unavailable.
- [x] `serialization.cpp`: Add explicit error on schema mismatch; bound deserialization depth to prevent stack overflow on crafted input.

**Degradation contracts required:**
- [x] 2.8h: zstd_codec: library unavailable → fail with explicit error, no silent pass-through
- [x] 2.9a: bounded resource check – zstd output buffer max before realloc

### 2.5 Runtime Services Plane Hardening

**Components:** `thread_pool_manager.cpp`, `rate_limiter.cpp`

**Work items:**
- [x] `thread_pool_manager.cpp`: Verify shutdown drains or cancels in-flight tasks (no dangling tasks at exit); add explicit error on task submission to full or stopped pool; verify priority ordering under saturation; bound task queue size.
- [x] `rate_limiter.cpp`: Verify token bucket is concurrency-safe without spin-loops (use `std::atomic` or `std::mutex` consistently); document reject vs. queue behavior as explicit configuration; add explicit handling for clock skew.

**Degradation contracts required:**
- [x] 2.9b: thread_pool: queue full → explicit THREAD_POOL_OVERLOAD error
- [x] 2.9c: rate_limiter: token exhausted → explicit RATE_LIMIT_EXHAUSTED error

### 2.6 Bounded Resource Checks (cross-cutting)

- [x] 2.9d: All high-fan-out helpers must have a documented resource limit (queue depth, buffer max, retry budget) visible at the call site or in the header @note.
- [x] 2.10: Doxygen error contracts updated for all public APIs after hardening (shared with Phase 3).

---

## Phase 3: Error Handling and Edge Cases

**Target:** Q4 2026  
**Acceptance gate:** All components emit typed ErrorCode; fail-safe semantics verified; edge cases tested

### 3.5 Observability Error Contracts

**Files:** `include/utils/audit_logger.h`, `include/utils/logger.h`, `include/utils/tracing.h`, `include/utils/saga_logger.h`

- [x] Add `@error_contract` Doxygen table to all public APIs (template in PHASE3_IMPLEMENTATION_GUIDE.md §1)
- [x] Ensure `logEvent()`, `log()`, `beginSpan()`/`endSpan()` return `ErrorCode` or `std::expected<T, ErrorCode>` on all failure paths
- [x] Verify error codes are in range 9010-9039 (observability taxonomy)

### 3.6 Privacy Error Contracts

**Files:** `include/utils/pii_detector.h`, `include/utils/pii_stream_scanner.h`, `include/utils/pii_pseudonymizer.h`, `include/utils/regex_detection_engine.h`

- [x] Add `@error_contract` Doxygen tables for `detect()`, `scan()`, `pseudonymize()`
- [x] Verify all detection errors return conservative result (no false-permit on error)
- [x] Verify error codes are in range 9040-9049 (privacy detection taxonomy)

### 3.7 Crypto Error Contracts

**Files:** `include/utils/hkdf_helper.h`, `include/utils/hkdf_cache.h`, `include/utils/pki_client.h`, `include/utils/lek_manager.h`

- [x] Add `@error_contract` Doxygen tables for `derive()`, `get()`, `encrypt()`/`decrypt()`
- [x] Verify key derivation never silently produces a weak or default key
- [x] Verify error codes are in range 9050-9059 (cryptography taxonomy)

### 3.8 Compression Error Contracts

**Files:** `include/utils/zstd_codec.h`, `include/utils/lz4_codec.h`, `include/utils/serialization.h`

- [x] Add `@error_contract` Doxygen tables for `compress()`, `decompress()`, `serialize()`/`deserialize()` — COMPLETE 2026-08-18
- [x] Verify compression errors are propagated, not silently skipped
- [x] Verify error codes are in range 9060-9069 (compression taxonomy)

### 3.9 Runtime Services Error Contracts

**Files:** `include/utils/thread_pool_manager.h`, `include/utils/rate_limiter.h`

- [x] Add `@error_contract` Doxygen tables for `submit()`, `tryAcquire()`
- [x] Verify error codes are in range 9070-9079 (concurrency taxonomy)

### 3.10–3.12 Diagnostics and Incident Categorization

- [x] 3.10: `IncidentCategorizer` with 15 operator-visible categories – COMPLETE (in error_contracts.h)
- [x] 3.11: Structured logging with `ErrorContext` – COMPLETE
- [x] 3.12: Update component implementations to call `logErrorWithContext()` / `categorizeIncident()` on non-trivial failures; operator-visible diagnostic messages must be actionable (not "unknown error") — COMPLETE 2026-08-17/18

### Edge Case Coverage Checklist

- [x] Unicode: CJK, RTL, combining marks in PII detection (pii_detector, regex_detection_engine)
- [x] Malformed input: truncated, invalid UTF-8, oversized buffers
- [x] Overload: queue full (audit_logger, thread_pool), buffer full (codecs), memory pressure
- [x] Resource exhaustion: file handles, thread count, OpenSSL context limits
- [x] External service unavailable: gRPC backend, HTTP endpoint, key store

---

## Phase 4: Tests

**Target:** Q4 2026  
**Acceptance gate:** 100% test pass rate; >= 80% public API coverage; all benchmarks pass in release profile

### 4.1 Test Execution and Verification

- [ ] Run all existing tests in the utils suite and verify 100% pass rate:
  - `test_utils_interfaces.cpp` (36 cases)
  - `test_utils_contract_hardening_focused.cpp` (16 cases)
  - `test_utils_privacy_audit_hardening.cpp` (8 cases)
  - `test_utils_rate_limiter.cpp` (7 cases)
  - `test_utils_standalone.cpp` (19 cases)
  - `test_phase1_resource_management.cpp` (15 cases)
  - `test_utils_future_interfaces.cpp` (22 cases)
- [ ] Run all benchmarks in release profile and verify they complete without errors:
  - `bench_pii_stream_scanner.cpp`
  - `bench_simd_distance.cpp`
  - `bench_thread_pool_saturation.cpp`
  - `bench_encryption.cpp`
  - `bench_compression.cpp`
  - `bench_compliance_security_governance.cpp`
- [ ] Measure code coverage for public APIs (lcov or equivalent); target >= 80%

### 4.2 New Tests for Phase 2-3 Hardening

**Observability:**
- [x] `test_audit_logger_bounded_queue.cpp`: verify queue-full behavior (reject vs. drop-oldest); no silent discard → implemented as `test_utils_audit_logger_hardening.cpp`
- [x] `test_logger_sink_unavailable.cpp`: verify fallback to stderr; no silent failure → covered in `test_utils_observability_error_contracts.cpp`

**Privacy:**
- [x] `test_pii_unicode_edge_cases.cpp`: CJK, RTL, combining marks, null bytes, truncated UTF-8 — implemented as `test_utils_pii_unicode_edge_cases.cpp`
- [x] `test_pii_scanner_timeout.cpp`: verify conservative fail-closed on scan timeout → implemented as `test_utils_privacy_hardening.cpp`

**Key Management:**
- [x] `test_hkdf_zeroization.cpp`: verify key material zeroized after use → implemented as `test_utils_crypto_hardening.cpp`
- [x] `test_lek_rotation_atomic.cpp`: verify no dual-generation window during rotation — implemented as `test_utils_lek_rotation_atomic.cpp`

**Compression:**
- [x] `test_codec_corrupt_input.cpp`: verify deterministic error on corrupt zstd/lz4 data → implemented as `test_utils_compression_hardening.cpp`
- [x] `test_codec_concurrent.cpp`: verify safe concurrent encode/decode → covered in `test_utils_compression_hardening.cpp`

**Runtime Services:**
- [x] `test_thread_pool_shutdown.cpp`: verify no dangling tasks on graceful and forced shutdown → implemented as `test_utils_runtime_hardening.cpp`
- [x] `test_rate_limiter_concurrency.cpp`: verify no spin-loop, no data race under TSAN → implemented as `test_utils_runtime_hardening.cpp`

### 4.3 Concurrency and Stress

- [x] Concurrency stress test for `audit_logger` under N concurrent writers (N = 8, 32, 128) — implemented as `test_utils_audit_logger_stress_concurrent_writers.cpp`
- [x] Concurrency stress test for `thread_pool_manager` submission saturation and priority ordering — implemented as `test_utils_thread_pool_stress_saturation.cpp`
- [x] Concurrency stress test for `pii_stream_scanner` parallel scan slots — implemented as `test_utils_pii_stream_scanner_stress_parallel.cpp`
- [~] Verify under TSAN (thread sanitizer) for all components with internal mutex usage (pending CI TSAN run)

---

## Doxygen Function-Level Audit (Phase 6 Completion)

This item closes the Phase 6 "partial" gap identified in the documentation pass of 2026-08-17.

- [ ] Run `doxygen Doxyfile` and capture Doxygen warnings
- [ ] For each public header in `include/utils/`: verify `@param`, `@return`, `@throws`/`@error_contract` are present for non-trivial APIs
- [ ] Target: zero undocumented public function parameters in `doxygen --warn-if-undocumented` output
- [ ] After clean Doxygen build: update PRODUCTION_READINESS_CHECKLIST.md Phase 6 Doxygen item to `[x]`

---

## Delivery Sequence and Dependencies

```
Phase 2 hardening (2.1-2.5) ──► Phase 3 error contracts (3.5-3.12) ──► Phase 4 tests (4.1-4.3)
                                                                         │
                                                             ◄── Doxygen audit (Phase 6 gap) ◄──
```

Phase 3 error contract work is partially blocked on Phase 2 hardening because
the error paths being documented need to exist in the implementation first.
Phase 4 test verification is blocked on both Phase 2 and Phase 3.

### Recommended Sequence per Sprint

| Sprint | Work |
|---|---|
| S1 | Phase 2.1 (audit_logger, logger) + Phase 3.5 Doxygen update |
| S2 | Phase 2.2 (privacy plane) + Phase 3.6 Doxygen update |
| S3 | Phase 2.3 (key management) + Phase 3.7 Doxygen update |
| S4 | Phase 2.4 (compression) + Phase 2.5 (runtime) + Phase 3.8-3.9 Doxygen |
| S5 | Phase 3.12 (logErrorWithContext adoption) + cross-cutting edge cases |
| S6 | Phase 4 test execution, coverage measurement, new tests (4.2-4.3) |
| S7 | Doxygen function-level audit; final PRODUCTION_READINESS_CHECKLIST sign-off |

---

## Sign-Off Gates

Each phase requires the following sign-off before marking complete:

**Phase 2 gate:**
- All hardening tasks `[x]` in PRODUCTION_READINESS_CHECKLIST.md
- Degradation contracts tested in at least one integration test per component

**Phase 3 gate:**
- All `@error_contract` / `@throws` Doxygen blocks in headers (3.5-3.9)
- `logErrorWithContext()` called on all non-trivial error paths (3.12)
- Edge case list in this document fully addressed

**Phase 4 gate:**
- 100% test pass rate (verified CI run output)
- Code coverage >= 80% (coverage report attached)
- All benchmarks passing in release profile (CI log attached)

**Combined Phase 2-4 sign-off:**
- Core Implementation Lead: ___________
- Error Handling Lead: ___________
- Test Lead: ___________
- Date: ___________
