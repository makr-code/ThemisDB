# Wave 6 Test Coverage

> **Wave:** 6 — Release-Candidate Hardening, Stress/Soak, Failure Injection & Final Governance  
> **Branch:** `develop`  
> **Date:** 2026-07-16  
> **CTest label filter:** `wave6`

---

## Overview

Wave 6 completes the release-readiness test program for ThemisDB. It delivers four focused test suites
designed to support the final release decision:

| PR   | File                                          | IDs          | Focus                                       |
|------|-----------------------------------------------|--------------|---------------------------------------------|
| W6-A | `w6a_critical_journey_hardening_test.cpp`     | RCJ-01..08   | Release-Candidate critical journey E2E      |
| W6-B | `w6b_stress_soak_stability_test.cpp`          | SSS-01..08   | Stress, soak and stability validation       |
| W6-C | `w6c_failure_injection_recovery_test.cpp`     | FIR-01..08   | Failure injection and recovery proofs       |
| W6-D | `WAVE6_TEST_COVERAGE.md` (this file)          | —            | Final governance, runbook, flake zeroing    |

---

## PR W6-A: Release-Candidate Critical Journey Hardening

### Test Cases

| ID     | Name                                                        | Risk Level |
|--------|-------------------------------------------------------------|------------|
| RCJ-01 | Happy-path: authenticated ingest → index → query → audit   | Critical   |
| RCJ-02 | Idempotent double-ingest produces a single indexed entry    | High       |
| RCJ-03 | Query result consistency after concurrent writes            | High       |
| RCJ-04 | Auth-token rotation mid-session preserves read access       | High       |
| RCJ-05 | Cross-pipeline state is consistent after partial rollback   | High       |
| RCJ-06 | Audit trail completeness across multi-step journey          | Medium     |
| RCJ-07 | Empty-result edge case is handled without error             | Medium     |
| RCJ-08 | State transitions are deterministic under re-execution      | Critical   |

### Acceptance Criteria

- All 8 cases pass deterministically on every run.
- No global state or shared test data between cases.
- Audit events are verifiable per-case (no silent swallowing).

---

## PR W6-B: Stress, Soak & Stability Validation

### Test Cases

| ID     | Name                                                                        | Risk Level |
|--------|-----------------------------------------------------------------------------|------------|
| SSS-01 | Sustained ingest burst — no storage corruption after N batches              | High       |
| SSS-02 | High-concurrency query fan-out — metric counts stay consistent              | High       |
| SSS-03 | Storage size is monotone during soak-style write loop                       | Medium     |
| SSS-04 | Repeated index lookups under concurrent writers find no phantom reads       | High       |
| SSS-05 | Audit log capacity is bounded — no unbounded growth under load              | Medium     |
| SSS-06 | Auth mock under concurrent token checks returns no false positives          | Critical   |
| SSS-07 | Pipeline throughput does not degrade across successive soak cycles          | Medium     |
| SSS-08 | Zero resource leaks — storage and index are fully cleaned between runs      | High       |

### Acceptance Criteria

- All 8 cases pass under the default CTest timeout (300 s).
- SSS-04 reliably detects phantom reads if they occur (assertion is deterministic).
- SSS-07 degradation factor is set conservatively (10×) to avoid environment flakiness.

---

## PR W6-C: Failure Injection & Recovery Proofs

### Test Cases

| ID     | Name                                                                         | Risk Level |
|--------|------------------------------------------------------------------------------|------------|
| FIR-01 | Auth service failure — pipeline returns safe error, no data leak             | Critical   |
| FIR-02 | Storage write failure — ingest fails cleanly; index not updated              | Critical   |
| FIR-03 | Partial ingest failure — already-stored documents remain intact              | High       |
| FIR-04 | LLM backend embedding failure — RAG path returns error, state clean          | High       |
| FIR-05 | Cascading dependency failure — retry succeeds after transient outage         | High       |
| FIR-06 | Timeout simulation — partial writes rolled back to consistent state          | Critical   |
| FIR-07 | Recovery after storage fault — subsequent ingests succeed                    | High       |
| FIR-08 | Data integrity after failure burst — no phantom or corrupted reads           | Critical   |

### Acceptance Criteria

- All failure paths return defined error codes or `nullopt`/`false` (no uncaught exceptions).
- Index and storage are always in a mutually consistent state after any injected failure.
- Recovery (fault-clear followed by success) is confirmed in FIR-05, FIR-07, FIR-08.

---

## PR W6-D: Final Governance, Flake Zeroing & Operability

### Quality Gates

| Gate                     | Description                                               | Blocker? |
|--------------------------|-----------------------------------------------------------|----------|
| `release_candidate` label | All RCJ tests must pass before release sign-off           | Yes      |
| `wave6` label             | Full Wave 6 suite must be green on `develop`              | Yes      |
| `failure_injection` label | All FIR tests must pass (data integrity proven)           | Yes      |
| `stress_soak` label       | All SSS tests must pass (stability proven)                | Yes      |
| Flake rate ≤ 0            | Any flaky test is treated as a blocker until fixed        | Yes      |

### Flake-Zero Strategy

1. **Deterministic inputs on assertion paths** — Wave 6 assertions use fixed IDs, fixed terms,
   and explicit payloads; they do not depend on `TestDataGenerator` randomness.
2. **No sleep-based synchronisation** — all concurrency tests use `std::atomic` + join/yield.
3. **Isolated fixtures** — each test creates its own auth/storage/index/audit instances.
4. **Conservative timing guard** — SSS-07 uses a 10× degradation factor to avoid CI noise.
5. **One-shot fault gates** — `FaultInjectionStorage` uses atomic state; no racy flag resets.

### Triage Runbook

#### Step 1 — Identify the failing label

```bash
ctest --preset linux-release -L wave6 --output-on-failure
```

#### Step 2 — Isolate the failing suite

```bash
# Run only W6-A
ctest --preset linux-release -R w6a_critical_journey_hardening_test --output-on-failure

# Run only W6-B
ctest --preset linux-release -R w6b_stress_soak_stability_test --output-on-failure

# Run only W6-C
ctest --preset linux-release -R w6c_failure_injection_recovery_test --output-on-failure
```

#### Step 3 — Reproduce locally

```bash
# Build
cmake --preset linux-release
cmake --build --preset linux-release --target w6c_failure_injection_recovery_test

# Run single test binary with verbose output
./build/linux-release/tests/integration/w6c_failure_injection_recovery_test \
    --gtest_output=xml:/tmp/fir_results.xml \
    --gtest_filter="*FIR08*"
```

#### Step 4 — Common failure patterns

| Symptom                              | Likely Root Cause                         | Action                                     |
|--------------------------------------|-------------------------------------------|--------------------------------------------|
| RCJ-03 concurrency count mismatch    | Race in mock index `IndexDocument`        | Verify `MockPipelineIndex` mutex coverage  |
| SSS-07 ratio > 10 on slow CI host    | Overloaded runner                         | Widen `kDegradationFactor` or skip timing  |
| FIR-02 index not empty after failure | `Ingest()` updated index before storage   | Fix ordering in `FailureInjectablePipeline`|
| FIR-06 StoredCount > 0 after burst   | `FaultInjectionStorage.fail_count_` race  | Verify atomic decrement logic              |

#### Step 5 — Escalation

If a test is confirmed flaky after 3 re-runs on a clean runner, open an issue tagged
`flaky-test` + `wave6` and disable the test with `GTEST_SKIP()` + a ticket reference
until the root cause is fixed.

### Ownership

| Suite | Owner Role       | Escalation Path                    |
|-------|------------------|------------------------------------|
| W6-A  | Release Engineer | → Platform Lead → Release Manager  |
| W6-B  | SRE / Performance| → Platform Lead                    |
| W6-C  | Backend Engineer | → Database Lead                    |
| W6-D  | QA Lead          | → Release Manager                  |

### Known Residual Risks & Follow-ups

| Risk                                            | Severity | Recommended Follow-up                                          |
|-------------------------------------------------|----------|----------------------------------------------------------------|
| SSS-07 wall-clock check is environment-sensitive | Low      | Replace with operation-count throughput metric post-release    |
| FaultInjectionStorage simulates transient faults only | Medium | Add persistent fault + WAL-level injection in Q4 cycle        |
| RCJ tests use in-process mocks, not real RocksDB | Medium  | Add smoke test against real DB backend before GA               |
| No network-level fault injection (gRPC drops)   | Medium   | Planned for post-Wave 6 resilience hardening sprint            |
| Concurrent phantom test (SSS-04) uses yield loop | Low     | Replace with condition_variable for tighter synchronisation    |
