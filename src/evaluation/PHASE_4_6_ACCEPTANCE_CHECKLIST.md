# Evaluation Module - Phase 4-6 Acceptance Checklist

<!-- Status: 2026-08-08 — Checklist prepared for Phase 4-6 execution -->
<!-- Issue: #5643 (Development Status) -->
<!-- Links: ROADMAP.md · JUSTIFIED_GAP.md · PRODUCTION_REQUIREMENTS.md -->

## Overview

This document defines the concrete acceptance criteria for Phases 4, 5, and 6 of the Evaluation Module hardening. Once the build environment is set up (see JUSTIFIED_GAP.md), all items can be executed sequentially to generate the required executable evidence for issue #5643 closure.

**Phase 3** (Error Handling) is already ✅ **COMPLETE AND VERIFIED** via code audit (2026-08-08). This checklist covers Phases 4, 5, and 6 only.

## Phase 4: Tests - Acceptance Criteria

### 4.1 Focused Test Target Execution

Execute all module-focused tests and capture pass/fail status with timestamp. Tests should be run in the environment matching the build preset (e.g., linux-release or windows-release).

#### Test Target: module_epic2_evaluation_hardware_profile_test_focused

- **Registration:** `tests/epic2_evaluation/CMakeLists.txt` lines 46-77
- **Source:** `tests/epic2_evaluation/hardware_profile_test.cc` (238 lines)
- **Acceptance:**
  - [ ] Target builds without errors
  - [ ] All test cases pass
  - [ ] Test output captured with timestamp in MODULE_EVIDENCE.md
  - [ ] Example evidence format:
    ```
    Test: module_epic2_evaluation_hardware_profile_test_focused
    Date: 2026-08-08T12:34:56Z
    Result: PASS (5/5 tests)
    Duration: 1.23s
    Notes: All hardware profile initialization and state checks passed.
    ```

#### Test Target: query_planner_test

- **Source:** `tests/epic2_evaluation/query_planner_test.cc` (623 lines)
- **Acceptance:**
  - [ ] Target builds without errors
  - [ ] All test cases pass (estimated 20+ tests covering path selection, fallback chains, Category C enforcement)
  - [ ] Test output captured with timestamp

#### Test Target: approximation_rules_test

- **Source:** `tests/epic2_evaluation/approximation_rules_test.cc` (664 lines)
- **Acceptance:**
  - [ ] Target builds without errors
  - [ ] All test cases pass (estimated 15+ tests covering zone mapping, governance decisions, policy versions)
  - [ ] Test output captured with timestamp

#### Test Target: benchmark_matrix_test

- **Source:** `tests/epic2_evaluation/benchmark_matrix_test.cc` (535 lines)
- **Acceptance:**
  - [ ] Target builds without errors
  - [ ] All test cases pass
  - [ ] Test output captured with timestamp

#### Test Target: retrieval_metrics_test

- **Source:** `tests/epic2_evaluation/retrieval_metrics_test.cc` (447 lines)
- **Acceptance:**
  - [ ] Target builds without errors
  - [ ] All test cases pass (estimated 20+ tests covering metric computations, error handling, input validation)
  - [ ] Test output captured with timestamp

#### Test Target: ablation_framework_test

- **Source:** `tests/epic2_evaluation/ablation_framework_test.cc` (310 lines)
- **Acceptance:**
  - [ ] Target builds without errors
  - [ ] All test cases pass
  - [ ] Test output captured with timestamp

#### Test Target: test_query_planner_cache

- **Source:** `tests/epic2_evaluation/test_query_planner_cache.cc` (541 lines)
- **Note:** This is Phase 3 P3-01 focused tests (28 tests for Query Optimizer Hardening)
- **Acceptance:**
  - [ ] Target builds without errors
  - [ ] All 28 test cases pass
  - [ ] Test output captured with timestamp
  - [ ] Evidence of Phase 3 policy/error cases covered

### 4.2 Regression Coverage

- [ ] All Phase 3 policy/error cases are covered by tests
  - Tensor staleness detection tests
  - Hardware/profile mismatch handling tests
  - Distributed manifest absence fallback tests
  - Category C fail-closed enforcement tests
- [ ] No silent test failures or skip markers
- [ ] All test assertions validate expected behavior (not just "no crash")

### 4.3 Test Evidence Documentation

- [ ] Create entry in MODULE_EVIDENCE.md under "Phase 4 Test Execution Evidence"
- [ ] Include:
  - Date and time of test run
  - Build preset used (e.g., linux-release)
  - Test environment details (compiler version, OS, GTest version)
  - Pass/fail status for each test target
  - Total test count and pass rate
  - Any test failures or anomalies with investigation notes

#### Template for evidence entry:

```markdown
### Phase 4 Test Execution Evidence (DATE)

**Environment:**
- Preset: linux-release
- Compiler: GCC 11.x
- GTest version: 1.x.x
- OS: Linux (Ubuntu 22.04 LTS)
- Build type: Debug

**Test Results:**
- [ ] module_epic2_evaluation_hardware_profile_test_focused: PASS (5/5 tests, 1.2s)
- [ ] query_planner_test: PASS (N/M tests, X.Xs)
- [ ] approximation_rules_test: PASS (N/M tests, X.Xs)
- [ ] benchmark_matrix_test: PASS (N/M tests, X.Xs)
- [ ] retrieval_metrics_test: PASS (N/M tests, X.Xs)
- [ ] ablation_framework_test: PASS (N/M tests, X.Xs)
- [ ] test_query_planner_cache: PASS (28/28 tests, X.Xs)

**Summary:** All 7 test targets passed with 100% test case success.

**Notes:**
- All Phase 3 error handling tests passed (Category C enforcement, fallback semantics, metric validation).
- No test failures or anomalies.
```

---

## Phase 5: Performance / Hardening - Acceptance Criteria

### 5.1 Benchmark Execution

Execute all benchmark targets and capture measured baselines. Benchmarks should be run on a consistent machine to establish reproducible gates.

#### Benchmark Target: planner_decision_bench

- **Source:** `benchmarks/epic2_evaluation/planner_decision_bench.cc`
- **Description:** EPIC 2.5 query planner decision overhead measurement
- **Acceptance:**
  - [ ] Benchmark builds without errors
  - [ ] Benchmark executes to completion without crashes
  - [ ] Measurement: query path selection latency (end-to-end: from eligibility check to decision + observer callback)
  - [ ] Baseline captured (e.g., "median latency: 42.3µs, p95: 156µs, p99: 201µs")
  - [ ] Evidence: full benchmark output captured with timestamp

#### Benchmark Target: benchmark_matrix_bench

- **Source:** `benchmarks/epic2_evaluation/benchmark_matrix_bench.cc`
- **Description:** EPIC 2.2 benchmark matrix construction and lookup throughput
- **Acceptance:**
  - [ ] Benchmark builds without errors
  - [ ] Benchmark executes to completion
  - [ ] Measurement: throughput (e.g., "queries/second") for HNSW/DiskANN/Tensor/Graph/LLM-LoRA matrix
  - [ ] Baseline captured (e.g., "HNSW: 50K ops/s, DiskANN: 25K ops/s")
  - [ ] Evidence: full benchmark output captured with timestamp

#### Benchmark Target: planner_decision_bench (artifact staleness variant)

- **Source:** `benchmarks/epic2_evaluation/artifact_staleness_bench.cc`
- **Description:** Artifact staleness detection latency measurement
- **Acceptance:**
  - [ ] Benchmark builds without errors
  - [ ] Measurement: staleness detection time (e.g., "age check, residual check, delta lag check")
  - [ ] Baseline captured
  - [ ] Evidence: full benchmark output captured with timestamp

#### Benchmark Target: storage_strategy_bench

- **Source:** `benchmarks/epic2_evaluation/storage_strategy_bench.cc`
- **Description:** Storage strategy selection measurement
- **Acceptance:**
  - [ ] Benchmark builds without errors
  - [ ] Measurement: strategy selection overhead and downstream follow-up measurement
  - [ ] Baseline captured
  - [ ] Evidence: full benchmark output captured with timestamp

### 5.2 Guardrail Definition

Define explicit performance guardrails based on measured baselines. Guardrails should be conservative (allow some margin above baseline).

#### Guardrail Template:

```markdown
### Planner Decision Latency Guardrail (Phase 5 P5-01)

**Baseline (from 2026-08-08 run on [hardware]):**
- Median latency: 42.3µs
- P95 latency: 156µs
- P99 latency: 201µs

**Guardrail (20% margin):**
- Median ≤ 50µs
- P95 ≤ 187µs
- P99 ≤ 241µs

**Reproducibility assumptions:**
- Hardware profile class: Intel x86-64 (modern CPU, no power throttling)
- Query type: 100-dimensional ANN with tensor and graph layers
- Shard count: 1 (single-shard, no cross-shard fan-out)
- Fallback amplification: no GPU errors (baseline path)

**Measurement hygiene:**
- Warm-up runs: 100 iterations before measurement
- Measurement runs: 10,000 iterations (steady state)
- Clock source: high-resolution steady_clock
- No concurrent system activity during measurement
```

### 5.3 Hardening Documentation

- [ ] Create entry in PERFORMANCE_EXPECTATIONS.md with measured baselines and guardrails
- [ ] Document for each benchmark:
  - Baseline values (median, P95, P99 if applicable)
  - Guardrail thresholds
  - Reproducibility assumptions (hardware, query type, shard count, concurrency)
  - Measurement date and environment details
  - Any anomalies or notes on variability

### 5.4 Benchmark Evidence Documentation

- [ ] Create entry in MODULE_EVIDENCE.md under "Phase 5 Benchmark Execution Evidence"
- [ ] Include:
  - Date and time of benchmark run
  - Build preset used
  - Benchmark environment details (CPU, RAM, GPU if applicable)
  - Measured baselines for each benchmark target
  - Guardrails established
  - Any anomalies or variability notes

#### Template for evidence entry:

```markdown
### Phase 5 Benchmark Execution Evidence (DATE)

**Environment:**
- Preset: linux-release
- Compiler: GCC 11.x
- Benchmark library: Google Benchmark
- Hardware: [CPU model, RAM, GPU if applicable]
- Build type: Release

**Baseline Measurements:**
- planner_decision_bench: median 42.3µs, P95 156µs, P99 201µs
- benchmark_matrix_bench: HNSW 50K ops/s, DiskANN 25K ops/s
- artifact_staleness_bench: median 8.5µs
- storage_strategy_bench: median 12.1µs

**Guardrails Established:**
- Planner decision: ≤ 50µs (median), ≤ 187µs (P95)
- Benchmark matrix: ≥ 40K ops/s (HNSW minimum)
- Artifact staleness: ≤ 10.2µs (median)
- Storage strategy: ≤ 14.5µs (median)

**Reproducibility:**
- All benchmarks run on consistent hardware (see environment above)
- Warm-up iterations: 100; measurement runs: 10,000
- No concurrent activity; high-resolution steady_clock
```

---

## Phase 6: Documentation & Acceptance - Acceptance Criteria

### 6.1 Evidence Synchronization

- [ ] Update AUDIT.md to reflect Phase 4-6 completion:
  - EVAL-AUD-01: ✅ RESOLVED — Phase 3 code verified complete; Phase 4-6 evidence captured
  - EVAL-AUD-02: ✅ RESOLVED — Current-cycle executable build/test evidence appended to MODULE_EVIDENCE.md
  - EVAL-AUD-03: ✅ RESOLVED — Benchmark baselines captured and guardrails documented
- [ ] Update MODULE_EVIDENCE.md with Phase 4-6 evidence entries (see templates above)
- [ ] Update PRODUCTION_REQUIREMENTS.md acceptance checklist:
  - [ ] Phase-3 Fehler- und Fallback-Semantik dokumentiert und verifiziert ✅
  - [ ] Fokus-Tests wurden erfolgreich gebaut und ausgefuehrt ✅
  - [ ] Relevante Benchmarks wurden ausgefuehrt und Guardrails dokumentiert ✅
  - [ ] Advisory-only Tensor-Semantik und Graph-Truth-Finalisierung unveraendert ✅
  - [ ] Default-Workflow-Integration bleibt deaktiviert ✅

### 6.2 ROADMAP.md Phase Completion Updates

- [ ] Phase 4 (Tests):
  - [x] All focused source-level tests built and executed
  - [x] Refresh executable run evidence for focused targets (evidence appended to MODULE_EVIDENCE.md)
  - [x] Expand regression coverage for Phase 3 policy/error cases
  - [x] All tests pass (100% pass rate)
- [ ] Phase 5 (Performance/Hardening):
  - [x] Define explicit guardrails and capture measured baselines
  - [x] Record reproducibility assumptions (hardware profile, shards, fallback)
  - [x] All guardrails met (baselines documented)
- [ ] Phase 6 (Documentation):
  - [x] Publish acceptance evidence from successful build/test/benchmark runs
  - [x] Keep AUDIT.md, MODULE_EVIDENCE.md, PRODUCTION_REQUIREMENTS.md synchronized
  - [x] Ready to close issue #5643

### 6.3 Issue #5643 Closure Preparation

- [ ] All three audit findings (EVAL-AUD-01, EVAL-AUD-02, EVAL-AUD-03) are resolved with evidence
- [ ] ROADMAP.md shows Phases 3-6 ✅ COMPLETE
- [ ] PRODUCTION_REQUIREMENTS.md acceptance checklist is 100% checked
- [ ] JUSTIFIED_GAP.md evidence closure path has been executed
- [ ] Prepare GitHub issue comment summarizing:
  - Phase 3 code audit results (complete and verified)
  - Phase 4 test execution results (all pass)
  - Phase 5 benchmark baselines and guardrails (documented)
  - Phase 6 documentation synchronization (complete)
  - Ready for closure pending parent-epic traceability refresh
- [ ] Verify parent EPIC 2 dependencies and status labels are updated

### 6.4 Acceptance Documentation Template

Create a comprehensive acceptance summary in a new file `src/evaluation/PHASE_6_ACCEPTANCE_EVIDENCE.md`:

```markdown
# Evaluation Module - Phase 6 Acceptance Evidence

<!-- Status: 2026-08-08T12:34:56Z — Phases 3-6 COMPLETE -->
<!-- Issue: #5643 (Development Status) -->

## Executive Summary

The Evaluation Module (EPIC 2) has successfully completed all Phase 3-6 hardening criteria as of [DATE].

- Phase 3 (Error Handling): ✅ CODE AUDIT VERIFIED COMPLETE (2026-08-08)
- Phase 4 (Tests): ✅ EXECUTABLE EVIDENCE CAPTURED
  - 7 test targets executed: [PASS/FAIL status]
  - 100% test case pass rate
  - Execution date: [DATE]
- Phase 5 (Performance): ✅ BENCHMARKS EXECUTED, GUARDRAILS DOCUMENTED
  - 4 benchmark targets executed
  - Baselines established for planner decision, matrix ops, staleness detection, storage strategy
  - Execution date: [DATE]
- Phase 6 (Documentation): ✅ SYNCHRONIZED WITH MEASURED EVIDENCE
  - AUDIT.md: findings resolved
  - MODULE_EVIDENCE.md: Phase 4-6 evidence appended
  - ROADMAP.md: Phase 3-6 marked complete
  - PRODUCTION_REQUIREMENTS.md: acceptance checklist 100% complete

## Audit Findings Resolution

All three audit findings are RESOLVED with evidence:

1. EVAL-AUD-01: Phase 3 runtime policy/error hardening
   - Status: ✅ CODE VERIFIED COMPLETE + TEST EVIDENCE CAPTURED
   - Evidence: Code audit (query_planner.cc, retrieval_metrics.cc, approximation_rules.cc, artifact_lifecycle.cc) + test passes

2. EVAL-AUD-02: Current-cycle executable build/test evidence
   - Status: ✅ EVIDENCE APPENDED TO MODULE_EVIDENCE.md
   - Evidence: [TEST RESULTS TIMESTAMP AND SUMMARY]

3. EVAL-AUD-03: Benchmark gate baselines
   - Status: ✅ BASELINES CAPTURED, GUARDRAILS DOCUMENTED
   - Evidence: [BENCHMARK RESULTS TIMESTAMP, BASELINES, GUARDRAILS]

## Production Readiness

✅ All PRODUCTION_REQUIREMENTS.md criteria satisfied:
- [x] Planner-, approximation-, and artifact-fallbacks are machine-readable and explainable
- [x] Missing/stale manifests trigger fail-closed behavior
- [x] Advisory-only tensor semantics and CPU-only graph-truth maintained
- [x] All releases backed by benchmark and test evidence
- [x] Default workflow integration remains disabled until gates pass
- [x] No summary-only truth results introduced

## Ready for Issue Closure

Issue #5643 is ready for closure pending:
- Parent EPIC 2 traceability review
- Status label refresh ([EPIC 2] Evaluation Module → PHASE 6 COMPLETE / RELEASE CANDIDATE)
```

---

## Execution Sequence

To complete Phases 4-6, execute in this order once the build environment is set up (see JUSTIFIED_GAP.md):

1. **Setup Build Environment** (one-time)
   ```bash
   # Option A: Bootstrap vcpkg
   cd vcpkg && ./bootstrap-vcpkg.sh
   cmake --preset linux-release
   
   # Option B: Install system packages
   sudo apt-get install libgtest-dev google-benchmark-dev
   cmake -B build-local -DTHEMIS_BUILD_TESTS=ON -DTHEMIS_BUILD_BENCHMARKS=ON
   ```

2. **Phase 4: Build & Execute Tests**
   ```bash
   cmake --build build-gcc-linux-release --target module_epic2_evaluation_*_focused
   cmake --build build-gcc-linux-release --target query_planner_test approximation_rules_test ...
   ctest --preset linux-release -R "epic2_evaluation" -V
   # Capture evidence (see Phase 4 section)
   ```

3. **Phase 5: Execute Benchmarks**
   ```bash
   cmake --build build-gcc-linux-release --target planner_decision_bench benchmark_matrix_bench ...
   ./build-gcc-linux-release/bin/planner_decision_bench --benchmark_out=planner_decision_results.json
   # Capture baselines (see Phase 5 section)
   ```

4. **Phase 6: Document Acceptance**
   - Append Phase 4 test evidence to MODULE_EVIDENCE.md
   - Append Phase 5 benchmark evidence to MODULE_EVIDENCE.md
   - Update PRODUCTION_REQUIREMENTS.md acceptance checklist
   - Update ROADMAP.md Phase 3-6 completion status
   - Create PHASE_6_ACCEPTANCE_EVIDENCE.md summary
   - Comment on issue #5643 with resolution summary

5. **Close Issue #5643**
   - Verify all audit findings are resolved
   - Confirm parent EPIC 2 status
   - Close issue with reference to acceptance evidence

---

## Success Criteria

Phases 4-6 are **COMPLETE** when:
- ✅ All 7 test targets build and execute successfully with 100% pass rate
- ✅ All 4 benchmark targets execute successfully and baselines are captured
- ✅ All guardrails are met (baselines within established thresholds)
- ✅ All audit findings (EVAL-AUD-01, EVAL-AUD-02, EVAL-AUD-03) are documented as RESOLVED
- ✅ ROADMAP.md, AUDIT.md, MODULE_EVIDENCE.md, PRODUCTION_REQUIREMENTS.md are all synchronized
- ✅ Issue #5643 is ready for closure

---

**Status:** Checklist prepared (2026-08-08). Execution blocked by build environment. See JUSTIFIED_GAP.md for setup instructions.

**Next Action:** Once build environment is available, follow "Execution Sequence" above to generate all Phase 4-6 evidence.
