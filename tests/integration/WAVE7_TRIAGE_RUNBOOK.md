# Wave 7 Triage Runbook

<!-- ThemisDB | WAVE7_TRIAGE_RUNBOOK.md | Version: 0.0.1 -->

## Purpose

Step-by-step triage procedures for any Wave 7 test failure in CI or local
development.  Follow this runbook before escalating to the on-call team.

---

## Quick Reference

| Label | Reproduce command | Escalation owner |
|-------|-------------------|-----------------|
| `final_signoff` | `ctest -L final_signoff --output-on-failure -V` | @core-pipeline |
| `recovery_resilience` | `ctest -L recovery_resilience --output-on-failure -V` | @reliability |
| `stability_cert` | `ctest -L stability_cert --output-on-failure -V` | @performance |
| `wave7` (all) | `ctest -L wave7 --output-on-failure -V` | @themisdb-core-team |

---

## General Triage Steps

### Step 1 — Identify the failing test

```bash
# From the build directory
ctest -L wave7 --output-on-failure 2>&1 | grep -E "FAILED|PASSED|Test #"
```

Note the test name (e.g., `w7a_final_journey_signoff_test`) and the test case
(e.g., `FJS01_FullIngestIndexQueryDeleteLifecycleWithInvariantChecks`).

### Step 2 — Re-run in verbose mode

```bash
ctest -R <test_name> --output-on-failure -V
# Example:
ctest -R w7a_final_journey_signoff_test --output-on-failure -V
```

Check `SCOPED_TRACE` messages in the output — every Wave 7 test prints
a trace label at the top of each test case identifying the test ID (e.g.,
`"FJS-01: full lifecycle invariant"`).

### Step 3 — Run the binary directly with GTest filter

```bash
# From the build directory
./tests/integration/pipeline/w7a_final_journey_signoff_test \
    --gtest_filter="FinalJourneySignoffTest.FJS01_*" \
    --gtest_repeat=5
```

Replace `w7a_final_journey_signoff_test` and fixture/test name as appropriate.

### Step 4 — Check for data races (ThreadSanitizer)

If the failure is non-deterministic or only occurs under load (W7B/W7C):

```bash
# Rebuild with TSAN
cmake -DCMAKE_BUILD_TYPE=Debug -DTHEMIS_ENABLE_TSAN=ON ..
cmake --build . --target w7b_recovery_resilience_test
./tests/integration/pipeline/w7b_recovery_resilience_test
```

Look for `WARNING: ThreadSanitizer: data race` in the output.

---

## Test-Specific Triage

### FJS-01 — Full lifecycle invariant

**Symptom**: `storage missing after ingest` or `phantom record found after delete`

**Triage**:
1. Verify `InMemoryPipelineStorage::Write` and `Erase` are thread-safe if the
   test is run in a parallel CTest configuration.
2. Check that `FullLifecyclePipeline::Delete` removes from both `ingested_` and
   `storage_`.

### FJS-02 — Multi-tenant isolation

**Symptom**: `tenant_A can read tenant_B data`

**Triage**:
1. Confirm `MultiTenantPipeline::ProvisionTenant` creates independent storage
   instances — not shared references.
2. Check that `TenantSearch` looks up the correct index by tenant key.

### FJS-03 — Idempotent re-ingestion

**Symptom**: `storage size must be 1` assertion fails (size > 1)

**Triage**:
1. Verify fingerprint key includes both `id` and `payload.size()`.
2. Check for off-by-one in fingerprint comparison.

### FJS-04 — Causal consistency

**Symptom**: `causal order violation at position N`

**Triage**:
1. Ensure `CausalWritePipeline::Append` increments `next_seq_` under the mutex
   before returning.
2. If test is run with multiple threads, verify single-threaded append pattern
   is used in this test.

### FJS-05 — Concurrent consistency

**Symptom**: `concurrent write count mismatch` or `index/storage inconsistency`

**Triage**:
1. Run with ThreadSanitizer (see Step 4 above).
2. Check that `ConcurrentConsistencyPipeline::ConcurrentWrite` holds the mutex
   for both storage and index operations.

### FJS-07 — Maximum payload boundary (1 MiB)

**Symptom**: Test is slow or OOM on CI

**Triage**:
1. Confirm CI runner has ≥ 512 MiB available.
2. If OOM, reduce `kMaxPayloadBytes` to 256 KiB and document in
   `WAVE7_TEST_COVERAGE.md` under Known Risks.

### FJS-08 — Stage failure rollback

**Symptom**: `phantom record found after rollback`

**Triage**:
1. Check `RollbackPipeline::Execute` — storage write at stage 1 must be
   followed by `storage_->Erase(id)` in both failure branches.
2. Confirm `rollback_count_++` is incremented in both `fail_at_index` and
   `fail_at_audit` branches.

---

### HCR-01 — Auto-retry within budget

**Symptom**: `must take exactly 3 attempts` fails

**Triage**:
1. Confirm `fail_until_attempt=2` means attempts 1 and 2 fail, attempt 3
   succeeds — check loop boundary in `RetryableOperationRunner::Run`.

### HCR-03 — Max retries exhausted

**Symptom**: `succeeded` is `true` unexpectedly

**Triage**:
1. Verify `fail_until_attempt=999` is larger than `max_retries+1=4`.
2. Check that the loop exits after `max_retries+1` iterations without calling
   success path.

### HCR-05 — WAL replay

**Symptom**: `k2` (uncommitted) appears after replay

**Triage**:
1. Ensure `WalJournalStore::Replay` checks `entry.committed == true` before
   copying to `data_`.
2. Verify `CommitWal` is not called for `idx2` in the test body.

### HCR-07 — Concurrent recovery serialization

**Symptom**: Data race or incorrect `RecoveryInvocations` count

**Triage**:
1. Run with ThreadSanitizer.
2. Confirm `SerializedRecoveryManager::Recover` holds `recovery_mutex_` for
   the entire body including `++recovery_invocations_`.

---

### ESC-01 — No memory drift

**Symptom**: `peak_size != current_size`

**Triage**:
1. Check for duplicate keys being written (overwritten keys still count toward
   peak_size but not current_size).
2. Ensure key generation is unique across iterations (`esc01_k_0`..`esc01_k_499`).

### ESC-02 — Latency recovery

**Symptom**: `post-burst latency has not recovered`

**Triage**:
1. This test has a wide 10× + 1 ms cushion.  If it fails, the CI machine is
   under extreme load — re-run at a quieter time.
2. If it consistently fails, increase the multiplier from 10× to 50× and
   document in `WAVE7_TEST_COVERAGE.md`.

### ESC-06 — Flake detection N=50

**Symptom**: One or more repetitions fail

**Triage**:
1. A genuine failure here means a real flakiness regression — escalate
   immediately to @quality.
2. Run the failing repetition in isolation with `--gtest_repeat=200` to
   confirm reproducibility.

### ESC-07 — Structured diagnostic log

**Symptom**: `diagnostic log contains malformed events`

**Triage**:
1. Check `DiagnosticEventLog::Emit` — confirm `timestamp_us` is set from
   `std::chrono::steady_clock` and is non-zero.
2. Confirm all three string fields are non-empty for every `Emit` call in
   the test body.

### ESC-08 — Monotonic drift

**Symptom**: `cumulative_ops series is not monotonically non-decreasing`

**Triage**:
1. `running_total` is incremented by `kOpsPhase` each phase — it can only
   increase, so a failure here indicates a logic error in the metric
   recording, not a real drift.
2. Check that `cumulative_ops.Record(running_total)` is called after the
   phase loop, not inside it.

---

## Monitoring Recommendations

### CI Dashboards

- Check the `wave7` label run in the PR check summary.
- All `release_critical` failures will show as blocking checks.
- `endurance;stability_cert` failures show as advisory (green check with warning).

### Soak Alerts

Set an alert if `ESC-01` or `ESC-08` runtime exceeds 30 seconds — this
indicates the in-memory pipeline is unexpectedly slow in the CI environment.

### TSAN in CI

Add `ctest -L wave7 -E "w7c_endurance"` to the TSAN build to avoid false
positives from the intentionally-interleaved ESC-05 load test.

---

## Escalation Path

| Severity | Action |
|----------|--------|
| `release_critical` failure on `develop` | Page on-call; block merge; open P0 issue |
| `release_critical` failure on feature branch | Fix before merge; author responsible |
| `stability_cert` failure | Open P1 issue; triage within 24 h; not blocking |
| Flaky test (ESC-06) | Open P0 issue; quarantine test until root cause found |

---

## Repro Script

```bash
#!/usr/bin/env bash
# Wave 7 full repro — run from build directory
set -euo pipefail

echo "=== Wave 7A: Final Journey Sign-off ==="
ctest -R w7a_final_journey_signoff_test --output-on-failure -V

echo "=== Wave 7B: Recovery & Resilience ==="
ctest -R w7b_recovery_resilience_test --output-on-failure -V

echo "=== Wave 7C: Endurance/Stability ==="
ctest -R w7c_endurance_stability_test --output-on-failure -V

echo "=== All Wave 7 tests completed ==="
```

---

*Ownership: @themisdb-core-team | Wave: 7 | Status: Operational*
