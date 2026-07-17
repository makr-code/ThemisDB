# Wave 7 Test Coverage — Final Release Gates

<!-- ThemisDB | WAVE7_TEST_COVERAGE.md | Version: 0.0.1 -->

## Overview

Wave 7 is the final test wave before release certification.  It consists of three
sub-waves (W7A, W7B, W7C) covering critical journey sign-off, recovery/resilience
hardening, and endurance/stability certification.

---

## Sub-Wave Summary

| Sub-wave | File | Tests | CTest Labels | Release Gate |
|----------|------|-------|--------------|--------------|
| W7A | `pipeline/w7a_final_journey_signoff_test.cpp` | FJS-01..FJS-08 | `wave7;w7a;release_critical;final_signoff` | ✅ Blocks release |
| W7B | `pipeline/w7b_recovery_resilience_test.cpp`   | HCR-01..HCR-08 | `wave7;w7b;release_critical;recovery_resilience` | ✅ Blocks release |
| W7C | `pipeline/w7c_endurance_stability_test.cpp`   | ESC-01..ESC-08 | `wave7;w7c;endurance;stability_cert` | ⚠️ Advisory — non-blocking on latency |

---

## W7A — Final Critical Journey Sign-off (FJS)

| Test ID | Description | Pass Criterion | Owner |
|---------|-------------|----------------|-------|
| FJS-01 | Full ingest → index → query → delete lifecycle | All four stages succeed; audit has `ingest/write` + `delete/erase`; no phantom records | @core-pipeline |
| FJS-02 | Multi-tenant isolation across all stages | No tenant can read another tenant's keys or index terms | @security |
| FJS-03 | Idempotent re-ingestion with duplicate detection | Re-ingest of identical payload returns `duplicate=true`; storage size stays 1 | @ingestion |
| FJS-04 | Ordered write sequence (causal consistency) | Write log is strictly monotonically increasing by sequence number | @storage |
| FJS-05 | Cross-component consistency after concurrent writes | All written IDs are present in storage; index consistent with storage | @core-pipeline |
| FJS-06 | Empty payload edge case | Empty payload stored without error; audit records `empty_payload`; index still works | @ingestion |
| FJS-07 | Maximum payload size boundary (1 MiB) | 1 MiB payload ingest/fetch/delete succeeds; fetched size == 1 MiB | @storage |
| FJS-08 | Stage failure with clean rollback | Failed stage leaves no phantom record; rollback count accurate | @core-pipeline |

### Pass/Fail Logic

All FJS tests carry the `release_critical` label.  The CI gate
`.github/workflows/09-pr-gates_release-critical-tests.yml` runs
`ctest -L release_critical` and will fail the PR if any test does not pass.

---

## W7B — High-Confidence Recovery & Resilience (HCR)

| Test ID | Description | Pass Criterion | Owner |
|---------|-------------|----------------|-------|
| HCR-01 | Intermittent auto-retry succeeds within budget | Operation succeeds on attempt 3 after 2 transient failures | @reliability |
| HCR-02 | Timeout cascade containment | Stage 3 not affected by Stage 2 latency; each stage has independent deadline | @reliability |
| HCR-03 | Max retries exhausted → clean failure | `succeeded=false` after `max_retries+1` attempts; no partial state | @reliability |
| HCR-04 | Partial write failure, no phantom records | Items before failure boundary committed; items after failure boundary absent | @storage |
| HCR-05 | WAL replay after abrupt crash | Only committed WAL entries present post-replay; uncommitted entries absent | @storage |
| HCR-06 | State integrity before/during/after recovery | Pre-crash good; during-crash empty; post-recovery correct | @storage |
| HCR-07 | Concurrent recovery requests serialized | All concurrent callers serialized; no data race; all entries present | @reliability |
| HCR-08 | Post-recovery results match pre-failure known-good | Every known-good key/value pair matches post-recovery | @storage |

### Pass/Fail Logic

All HCR tests carry the `release_critical` label and block the release PR gate.

---

## W7C — Endurance/Stability Certification (ESC)

| Test ID | Description | Pass Criterion | Owner |
|---------|-------------|----------------|-------|
| ESC-01 | Sustained throughput (N=500), no memory drift | total_writes == N; peak_size == current_size | @performance |
| ESC-02 | Load spike: idle→burst→idle, latency recovery | Post-burst latency ≤ 10× idle average + 1 ms | @performance |
| ESC-03 | Long-running query workload, cursor leak detection | Open handles == 0 after 100 query cycles | @core-pipeline |
| ESC-04 | Repeated open/close cycles, cleanup completeness | total_opens == total_closes; open_count == 0 | @core-pipeline |
| ESC-05 | Interleaved read/write under load, no data loss | All written keys readable with correct values; 0 mismatches | @storage |
| ESC-06 | Flake detection N=50 | 0/50 repetitions fail | @quality |
| ESC-07 | Diagnostic output: structured error log on anomaly | ERROR event present; all events well-formed (non-empty fields, non-zero timestamp) | @observability |
| ESC-08 | Monotonic drift indicator over sustained run | cumulative_ops series is non-decreasing; final count correct | @performance |

### Pass/Fail Logic

ESC tests carry the `endurance;stability_cert` label.  They are
**advisory** for the PR gate and do not block merge.  However, any failure
must be triaged before the release tag is cut — see `WAVE7_TRIAGE_RUNBOOK.md`.

---

## Known Risks

| Risk | Mitigation |
|------|-----------|
| ESC-02 latency threshold may be environment-sensitive | Threshold set conservatively (10× + 1 ms cushion); in-memory ops are orders of magnitude below this |
| W7B HCR-07 ThreadSanitizer race detection | All shared state guarded by `std::mutex`; TSAN clean in CI |
| FJS-05 concurrent write count | Uses deterministic thread count × ops; no random scheduling dependency |
| FJS-07 1 MiB allocation in CI | Under 10 MiB per test process; well within CI memory budget |

---

## Canonical RNG Seed

All Wave 7 tests use `kCanonicalSeed = 42` for any random data generation,
ensuring full reproducibility across environments.

---

## Release Gate Commands

```bash
# Run all Wave 7 release-critical tests
ctest -L "release_critical" --output-on-failure

# Run all Wave 7 tests (including endurance)
ctest -L "wave7" --output-on-failure

# Run only W7A sign-off suite
ctest -L "final_signoff" --output-on-failure

# Run only W7B recovery/resilience
ctest -L "recovery_resilience" --output-on-failure

# Run only W7C endurance/stability
ctest -L "stability_cert" --output-on-failure
```

---

*Ownership: @themisdb-core-team | Wave: 7 | Status: Final Release Gate*
