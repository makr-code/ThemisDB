# Wave 8 Triage Runbook

<!-- ThemisDB | WAVE8_TRIAGE_RUNBOOK.md | Version: 0.0.1 -->

## Purpose

Step-by-step triage procedures for any Wave 8 test failure in CI or local
development.  Follow this runbook before escalating to the on-call team.

---

## Quick Reference

| Label | Reproduce command | Escalation owner |
|-------|-------------------|-----------------|
| `incident_regression` | `ctest -L incident_regression --output-on-failure -V` | @core-pipeline |
| `contract_compat`     | `ctest -L contract_compat --output-on-failure -V`     | @api |
| `determinism`         | `ctest -L determinism --output-on-failure -V`         | @ci |
| `flake_burndown`      | `ctest -L flake_burndown --output-on-failure -V`      | @ci |
| `ci_signal`           | `ctest -L ci_signal --output-on-failure -V`           | @ci |
| `wave8` (all)         | `ctest -L wave8 --output-on-failure -V`               | @themisdb-core-team |

---

## General Triage Steps

### Step 1 — Identify the failing test

```bash
# From the build directory
ctest -L wave8 --output-on-failure 2>&1 | grep -E "FAILED|PASSED|Test #"
```

### Step 2 — Reproduce locally

```bash
# Reproduce a single failing test (example: W8A)
ctest -R w8a_incident_regression_shielding_test --output-on-failure -V

# Verbose GTest output
./w8a_incident_regression_shielding_test --gtest_filter="*" --gtest_output=xml:/tmp/w8a_results.xml
```

### Step 3 — Check for non-determinism

```bash
# Re-run the same test 5 times and compare pass/fail counts
for i in $(seq 1 5); do
    ctest -R w8c_determinism_flake_ci_signal_test --output-on-failure 2>&1 \
        | grep -c "PASSED\|FAILED"
done
```

If failure rate < 100%, the test is likely a flake — open a `flake` label issue
and reference the DFQ wave-8 category.

### Step 4 — Inspect WAL / audit state (IRS failures)

For IRS-02 (WAL ordering) or IRS-05 (index/storage divergence):

```bash
# Check if uncommitted WAL entries leaked into recovered state
./w8a_incident_regression_shielding_test \
    --gtest_filter="*IRS02*" --gtest_also_run_disabled_tests -v
```

### Step 5 — Check concurrent access failures (IRS-01, CCR-04)

These tests run `std::thread` workers.  If CI reports TSAN violations:

```bash
# Rebuild with ThreadSanitizer
cmake -DCMAKE_CXX_FLAGS="-fsanitize=thread" ...
ctest -R "w8a|w8b" --output-on-failure
```

---

## W8A — Incident Regression Shielding Failures

### IRS-01 (phantom record)

**Symptom:** `phantom records with corrupted values detected: N`  
**Root cause:** Race in Delete() not using the same mutex as Write().  
**Fix:** Ensure all mutating operations in MinimalKVStore hold the same mutex.

### IRS-02 (WAL ordering)

**Symptom:** `committed WAL key missing after replay` or `uncommitted key appeared`  
**Root cause:** WAL::Commit() or Replay() has ordering bug.  
**Fix:** Verify Commit() marks only entries appended before the call; Replay() skips uncommitted.

### IRS-03 (retry storm)

**Symptom:** `total back-off delay exceeds cap`  
**Root cause:** Multiplier or cap applied incorrectly in back-off loop.  
**Fix:** Verify delay capping at `max_delay` per step, not per total.

### IRS-04 (partial batch)

**Symptom:** `rollback failed: key still present: batch_key_N`  
**Root cause:** BatchWriteSimulator does not roll back all staged keys on failure.  
**Fix:** Ensure every staged key is erased in the failure path.

### IRS-07 (large value)

**Symptom:** `N/100 reads returned a corrupted large value`  
**Root cause:** Shared mutable state between InMemoryBackend and test.  
**Fix:** Verify MinimalKVStore::Read() returns a copy, not a reference.

### IRS-08 (audit log)

**Symptom:** `audit log has duplicate or missing keys`  
**Root cause:** AuditLogCapture::Record() not holding the mutex when incrementing the sequence counter.  
**Fix:** Ensure both the push_back and the counter increment are inside the same lock scope.

---

## W8B — Contract Compatibility Failures

### CCR-01 (round-trip)

**Symptom:** Field mismatch after Deserialize(Serialize(r)).  
**Root cause:** Serialization format changed without updating the parser.  
**Fix:** Ensure Serialize and Deserialize use identical delimiter/key conventions.

### CCR-03 (substitution)

**Symptom:** `PrefixedInMemoryBackend` contract failure.  
**Root cause:** Prefix not stripped consistently in Read/Delete.  
**Fix:** Apply prefix in Read/Delete on the lookup key, not on the stored key.

### CCR-04 (reader isolation)

**Symptom:** `corrupt_reads > 0` — value neither v1 nor v2.  
**Root cause:** Non-atomic string swap without mutex in InMemoryBackend.  
**Fix:** Verify all Write/Read paths hold `mu_`.

---

## W8C — Determinism/Flake Failures

### DFQ-01 (seed reproducibility)

**Symptom:** `N key mismatches between two seeded generators`  
**Root cause:** `std::mt19937` state shared across test runs, or incorrect seed.  
**Fix:** Each generator must be constructed with the same seed value independently.

### DFQ-04 (variance budget)

**Symptom:** `CV = X% exceeds 10%`  
**Root cause:** Synthetic distribution parameters changed or PRNG state contaminated.  
**Fix:** Verify the `std::normal_distribution` parameters (mean=100, stddev=3) and that `rng` is freshly seeded.

### DFQ-06 (gate counter)

**Symptom:** `gate counter emits 1.0 / 0.0 incorrectly`  
**Root cause:** `SetPassed` called with wrong boolean.  
**Fix:** Confirm the expected == actual comparison result.

### DFQ-07 (state isolation)

**Symptom:** `state leaked from previous test`  
**Root cause:** `StateSentinel::Reset()` not called in SetUp/TearDown.  
**Fix:** Verify both `SetUp()` and `TearDown()` call `sentinel_.Reset()`.

---

## Escalation Policy

| Failure type | SLA | Action |
|---|---|---|
| IRS (incident regression) | 2 h | Page on-call; block merge; open regression ticket |
| CCR (contract compat) | 4 h | Notify API owner; block merge |
| DFQ (determinism/flake) | 1 business day | Open flake ticket; non-blocking unless > 3 consecutive failures |
| Any `release_critical` failure | Immediate | Block merge via CI gate |
