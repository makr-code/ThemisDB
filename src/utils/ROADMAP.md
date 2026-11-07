---
Author: "ThemisDB Team"
Created: "2026-11-07"
Last Updated: "2026-11-07"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot; OpenAI API assistant"
Human_Review: false
Human_Author: "ThemisDB Team"
---

> Exact model backend not verifiable from final document text alone.

# Utils Module Roadmap

<!-- Status: [ ] open  [~] in progress  [x] done  [I] issue  [P] PR  [?] blocked  [!] unclear -->
<!-- Status: review | validated: 2026-11-07 -->
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

## Research-Based Release Evolution: v2.5.0 and v3.0.0

This plan is a module-level proposal, not a release commitment.
[`VERSIONING.md`](../../VERSIONING.md) identifies v2.5.0 as the next planned
minor version; release dates, staffing, and the v3.0.0 schedule remain subject
to project-level approval. Waves 1–6 below are utils release sub-waves and are
separate from the project-wide Wave A–D model above.

### Current maturity and evidence-based gaps

**Assessment:** utils is functionally broad and has meaningful security,
concurrency, and edge-case hardening. Its implementation and focused test
coverage are more mature than its release-grade performance evidence and
consumer/API compatibility inventory. Treat the module as operationally useful,
but not yet performance-qualified across its high-fan-out paths.

- `include/utils/` and `src/utils/` cover observability, privacy, key handling,
  compression/serialization, concurrency, and general helpers. Existing
  focused tests exercise privacy fail-closed behavior, Unicode, malformed
  inputs, serialization bounds, and concurrency stress; see
  [`tests/utils/CMakeLists.txt`](../../tests/utils/CMakeLists.txt) and
  [`tests/utils/test_utils_privacy_hardening.cpp`](../../tests/utils/test_utils_privacy_hardening.cpp).
- Only `bench_utils_release_gates` is registered in
  [`benchmarks/utils/CMakeLists.txt`](../../benchmarks/utils/CMakeLists.txt).
  Its cases measure configuration construction/copy and small error dispatch,
  not production scanner, compression, crypto, or concurrent-runtime paths.
  `BENCHMARK_BASELINE.md` explicitly classifies the older Phase A.3 results as
  simulated historical data, not a release baseline.
- [`PERFORMANCE_EXPECTATIONS.md`](PERFORMANCE_EXPECTATIONS.md) records that
  current utils benchmarks do not establish p95/p99 for those subsystems.
  Therefore no absolute performance target or claimed release regression
  threshold is currently evidence-backed.
- Regex/NER and streaming privacy helpers have bounds and failure tests, but
  the module's own research note identifies accuracy, representative
  performance, and data-set limitations. In particular, embedding-based PII
  detection is not currently instantiable; see
  [`EMBEDDING.md`](EMBEDDING.md). It is not assumed as a release capability in
  this plan.
- The Phase 4.2 completion list names
  `test_utils_pii_stream_scanner_stress_parallel.cpp`, which is not present in
  the current `tests/utils/` file inventory. Confirm whether it was renamed or
  omitted before counting that listed suite as release evidence.
- The Developer LLM Wiki has no curated utils-specific module page, and its
  generated health snapshot is dated 2026-11-02 with a large source delta.
  Current source, tests, and module documentation were used as the evidence
  baseline; the wiki remains an explicit synchronization gap.

### Research findings and implications

1. Google Benchmark supports repeated runs, aggregate statistics, JSON output,
   and randomized interleaving. Its variance guidance identifies CPU-frequency
   changes, scheduling contention, affinity, and NUMA as sources of noise.
   **Implication:** record host/compiler/build context, use repetitions and
   stable runners, and retain raw results. Google Benchmark's repetition
   aggregates are not by themselves request-latency p95/p99; collect those from
   representative operation-level samples where percentile budgets are needed.
2. OWASP's Denial of Service Cheat Sheet recommends inventorying application
   resource-exhaustion risks, applying cheap validation early, and designing
   graceful degradation. **Implication:** test bounded input, queue, timeout,
   memory, and dependency-failure behavior across the shared utilities rather
   than treating unit success as overload evidence.
3. The in-repository utils research and performance notes require labeled
   fixtures for detection behavior and production-path measurements for
   performance claims. **Implication:** keep correctness/false-negative
   validation separate from throughput claims and do not infer performance
   from historical simulations or narrow configuration microbenchmarks.

Research references:

- [Google Benchmark User Guide](https://github.com/google/benchmark/blob/main/docs/user_guide.md)
- [Google Benchmark: Reducing Variance](https://github.com/google/benchmark/blob/main/docs/reducing_variance.md)
- [OWASP Denial of Service Cheat Sheet](https://github.com/OWASP/CheatSheetSeries/blob/master/cheatsheets/Denial_of_Service_Cheat_Sheet.md)
- Local evidence: [`RESEARCH.md`](RESEARCH.md), [`PERFORMANCE_EXPECTATIONS.md`](PERFORMANCE_EXPECTATIONS.md), [`SECURITY.md`](SECURITY.md), and [`BENCHMARK_BASELINE.md`](../../benchmarks/utils/BENCHMARK_BASELINE.md).

### v2.5.0 — compatible evidence and hardening

The v2.5.0 scope is additive and backward-compatible within the v2 major line.
Prioritize evidence and bounded behavior before optimization; do not introduce
compatibility shims or claim a release gate before its production baseline is
measured.

#### Wave 1 (P1) — Representative performance baseline (estimate: 2–3 person-weeks)

- **Goal:** Replace synthetic/narrow-only evidence with reproducible measurements
  of selected production utility paths.
**Tasks:**
- [ ] Register production-path benchmark cases for the PII stream scanner, compression/serialization, and one runtime contention path (Target: TBD)
- [ ] Define representative input sizes, warm/cold state, concurrency, and machine-readable output (Target: TBD)
- [ ] Record CPU, OS, compiler, build mode, dependency versions, and benchmark revision; add operation-sample latency reporting for p95/p99 budgets (Target: TBD)
- **Dependencies:** Dedicated or otherwise controlled representative hardware;
  module owners select workload distributions before the first baseline is
  accepted.
- **Risks / trade-offs:** Shared CI hosts add noise; realistic workloads cost
  runtime and may expose environment-specific results. Do not trade correctness
  or safe bounds for a microbenchmark win.
- **Success metrics:** 100% of selected cases call production implementations;
  each case emits machine-readable results and environment metadata; five
  repetitions produce a coefficient of variation at or below 5% on the
  controlled runner, or the case is marked noisy and not gated. Establish
  regression thresholds only after the baseline is reviewed.
- **Validation:** Run the benchmark target in release mode with repetitions and
  randomized interleaving; inspect raw JSON, rerun on the same host, and compare
  output to the checked-in baseline. Keep percentile measurement separate from
  Google Benchmark mean/standard-deviation aggregates.

#### Wave 2 (P1) — Security and overload proof (estimate: 2–4 person-weeks)

- **Goal:** Verify that high-fan-out utilities remain bounded and report
  incomplete/degraded results explicitly under malformed input, overload, and
  dependency loss.
**Tasks:**
- [ ] Add accepted/rejected boundary tests for regex, stream chunking, serialization, and compression limits (Target: TBD)
- [ ] Add deterministic queue saturation/shutdown, rate-limiter timeout, audit-write failure, and PKI pinning/configuration tests (Target: TBD)
- [ ] Add fault-injection and sustained-concurrency cases using existing test harnesses (Target: TBD)
- **Dependencies:** Wave 1 selects tested workload sizes; security and utility
  owners agree on safe resource/time limits for each API.
- **Risks / trade-offs:** Limits that are too strict can reject legitimate
  workloads; flaky timing-based tests can hide real failures. Prefer injected
  clocks/faults and deterministic assertions over wall-clock-only tests.
- **Success metrics:** Every selected boundary has a test for both accepted and
  rejected/failure behavior; no unbounded wait or silent partial-success result
  remains undocumented; zero findings in the applicable ASan/UBSan/TSan runs.
- **Validation:** Run the focused utils test targets, applicable sanitizers,
  repeated stress tests, and failure-injection tests; verify each failure has
  an explicit result/error contract and bounded cleanup.

#### Wave 3 (P1) — Consumer/API and operator readiness (estimate: 1–3 person-weeks)

- **Goal:** Make compatibility risk and operational handling reviewable before
  the v2.5.0 release candidate.
**Tasks:**
- [ ] Inventory public utility entry points and high-fan-out consumers (Target: TBD)
- [ ] Link each critical contract to tests and failure semantics (Target: TBD)
- [ ] Review runbook coverage for audit, privacy, key, tracing, compression, and pool incidents; correct unsupported fallback or performance claims (Target: TBD)
- **Dependencies:** Wave 2 establishes validated failure behavior; module
  maintainers identify consumers and operator-owned scenarios.
- **Risks / trade-offs:** Broad helper usage makes exhaustive consumer inventory
  expensive; scope the first pass to public headers and release-critical
  surfaces, then track the remainder as an explicit gap.
- **Success metrics:** 100% of identified release-critical public surfaces have
  an owner, documented failure/compatibility contract, and at least one linked
  focused test; every operator-critical scenario has a runbook or a named
  follow-up.
- **Validation:** Review header/API docs against implementation and tests; run
  public-header compile checks where available; verify internal links and
  release checklist evidence before v2.5.0 RC.

### v3.0.0 — deliberate contract and architecture evolution

The v3.0.0 scope is strategic and may include approved source/API breaks. It
must not be treated as permission to add hidden legacy paths: any migration
must use the canonical v3 contract, with explicit human approval for any
compatibility bridge and a documented removal milestone.

#### Wave 4 (P2) — API domains and compatibility design (estimate: 2–4 person-weeks)

- **Goal:** Define a smaller, clearer contract boundary for the broad shared
  utility surface without prematurely splitting the implementation target.
**Tasks:**
- [ ] Classify every public header by observability, privacy/key, compression/serialization, concurrency, or general support (Target: TBD)
- [ ] Map direct dependencies and consumer fan-out; define ownership, thread-safety, errors, resource limits, and platform/compiler support (Target: TBD)
- [ ] Publish a v2-to-v3 migration table for proposed breaking changes (Target: TBD)
- **Dependencies:** Wave 3's consumer inventory and v2.5.0 failure contracts.
- **Risks / trade-offs:** Over-segmentation can create coupling and maintenance
  overhead; retain shared implementation where measurement and ownership do
  not justify a separate target.
- **Success metrics:** Every public header has one documented owner/domain and
  compatibility status; every proposed break has a replacement, migration
  example, affected-consumer list, and test plan before implementation begins.
- **Validation:** API/architecture review with downstream module owners;
  compatibility builds against representative consumers and supported
  toolchain/platform matrix.

#### Wave 5 (P2) — Measured usability and performance improvements (estimate: 3–6 person-weeks)

- **Goal:** Improve only utility paths shown by Wave 1 data to constrain real
  workloads, while preserving detection and failure semantics.
**Tasks:**
- [ ] Select the highest-value measured bottlenecks and evaluate scanner, codec, or runtime-service changes against the approved contract (Target: TBD)
- [ ] Publish labeled PII evaluation methodology and precision/recall or false-negative results before proposing detector expansion (Target: TBD)
- [ ] Reassess embedding detection only through a separate design with implementation, trust/resource contract, dataset, and measurable evaluation (Target: TBD)
- **Dependencies:** Stable Wave 1 baseline, Wave 2 correctness/security
  coverage, and Wave 4 approved v3 contract.
- **Risks / trade-offs:** Hardware-specific wins may regress other targets;
  detector recall cannot be traded for throughput without explicit product
  approval. If no bottleneck meets the evidence bar, defer optimization.
- **Success metrics:** For each selected hot path, achieve at least 20%
  improvement in its predeclared primary workload metric on reference hardware,
  with no correctness/security regression and no more than 5% regression in
  other declared workloads; otherwise document the measured result and do not
  claim improvement.
- **Validation:** Run parity/property tests and sanitizer/fault suites, repeat
  Wave 1 benchmarks on identical hardware/configuration, and report raw
  results, variance, workload, memory use, and any cross-platform regression.

#### Wave 6 (P2) — v3 release operations and sustained resilience (estimate: 2–4 person-weeks)

- **Goal:** Demonstrate the new contracts are operable and stable under
  sustained load before v3.0.0 GA.
**Tasks:**
- [ ] Align diagnostics and resource limits with the approved contracts (Target: TBD)
- [ ] Exercise restart, dependency-loss, saturation, and recovery paths (Target: TBD)
- [ ] Update runbooks, migration notes, dependency/security review, and release evidence (Target: TBD)
- **Dependencies:** Waves 4–5 complete; supported platform matrix and v3 release
  candidate are available.
- **Risks / trade-offs:** Soak tests are resource-intensive and cannot replace
  production telemetry; platform availability may delay final evidence.
- **Success metrics:** A 24-hour representative soak completes with no
  unbounded queue growth, deadlock, sanitizer finding, unexplained error, or
  resource leak; all v3 breaking changes have migration and rollback guidance;
  all release-critical benchmarks remain within approved budgets.
- **Validation:** Run focused and integration suites, sanitizers, fault
  injection, 24-hour soak, public API/consumer builds, and release checklist
  review on supported platforms.

### Cross-wave KPIs, risks, and open assumptions

| Area | KPI / acceptance threshold | Evidence source |
|---|---|---|
| Performance | All selected release cases execute production paths; controlled-runner CV ≤5%; p95/p99 reported from operation samples where gated | Benchmark JSON, host/compiler metadata, locked baseline |
| Correctness and security | All selected bounds/failure cases asserted; zero applicable sanitizer findings; no silent partial result | Focused tests, fault-injection and sanitizer logs |
| Compatibility | All critical v2 public APIs classified before v3 changes; all breaks have consumer list and migration test | Header/consumer inventory and compatibility builds |
| Operations | Critical utility failures have actionable diagnostics, bounded cleanup, and a runbook or tracked owner | Failure tests, runbooks, release evidence |
| Governance | No performance/security claim without reproducible source/test/benchmark evidence; no unapproved legacy paths | PR evidence and module-owner review |

**Primary trade-off:** prioritize evidence and predictable bounds over broad
feature expansion. The v2.5.0 line preserves consumer contracts; v3.0.0 may
restructure contracts only after consumer mapping and migration proof.

**Open assumptions / knowledge gaps:**

- Representative reference hardware, workload distributions, and release
  benchmark runners have not been nominated; thresholds are provisional until
  owners approve them.
- Consumer fan-out is described as broad but has not been exhaustively counted.
- No versioned, labeled PII evaluation corpus or cross-language quality target
  is identified in current module docs.
- Supported compiler/platform compatibility for every public helper needs
  confirmation against current release configurations.
- Person-week estimates exclude cross-module owner review, CI queue time, and
  hardware procurement; release scheduling and staffing are not confirmed.
- The Developer LLM Wiki's utils-specific curation and freshness remain open;
  synchronize it after the evidence baseline is approved.
