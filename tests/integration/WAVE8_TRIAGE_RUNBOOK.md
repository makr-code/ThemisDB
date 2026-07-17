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
| `contract_compat` | `ctest -L contract_compat --output-on-failure -V` | @api-compat |
| `flake_burndown` | `ctest -L flake_burndown --output-on-failure -V` | @quality |
| `wave8` (all) | `ctest -L wave8 --output-on-failure -V` | @themisdb-core-team |

---

## General Triage Steps

### Step 1 — Identify the failing test

```bash
# From the build directory
ctest -L wave8 --output-on-failure 2>&1 | grep -E "FAILED|PASSED|Test #"
```

Note the test name (e.g., `w8a_incident_regression_shielding_test`) and the
test case (e.g., `IRS01_InvalidStateTransitionIsRejectedAndStateIsPreserved`).

### Step 2 — Re-run in verbose mode

```bash
ctest -R <test_name> --output-on-failure -V
# Example:
ctest -R w8a_incident_regression_shielding_test --output-on-failure -V
```

Check `SCOPED_TRACE` messages in the output — every Wave 8 test prints
a trace label at the top of each test case identifying the test ID (e.g.,
`"IRS-01: invalid state-transition regression"`).

### Step 3 — Run the binary directly with GTest filter

```bash
# From the build directory
./tests/integration/pipeline/w8a_incident_regression_shielding_test \
    --gtest_filter="IncidentRegressionShieldingTest.IRS01_*" \
    --gtest_repeat=5
```

Replace the binary name and fixture/test name as appropriate.

### Step 4 — Check for data races (ThreadSanitizer)

If the failure is non-deterministic or only occurs under load (DFQ-07):

```bash
# Rebuild with TSAN
cmake -DCMAKE_BUILD_TYPE=Debug -DTHEMIS_ENABLE_TSAN=ON ..
cmake --build . --target w8c_determinism_flake_ci_signal_test
./tests/integration/pipeline/w8c_determinism_flake_ci_signal_test
```

Look for `WARNING: ThreadSanitizer: data race` in the output.

---

## Test-Specific Triage

### IRS-01 — Invalid state-transition regression

**Symptom**: `kIndexed→kNew must be rejected` assertion fails

**Triage**:
1. Verify `StateMachinePipeline::IsValidTransition` covers all invalid pairs:
   backward (kIndexed→kNew), self-loop (kIndexed→kIndexed), post-terminal
   (kDeleted→anything).
2. Check that `Advance` returns the *current* state (not the target) on failure.

---

### IRS-02 — Idempotent command bus

**Symptom**: `duplicate command must not re-execute` fails (executed=true)

**Triage**:
1. Verify `IdempotentCommandBus::Dispatch` checks `seen_` *before* appending to
   `executed_payloads_`.
2. Confirm `seen_.insert` happens under the same mutex lock as the check.

---

### IRS-03 — Out-of-order command rejection

**Symptom**: `seq=5 must be rejected` assertion fails

**Triage**:
1. Verify `OrderedCommandQueue::Enqueue` compares `seq != next_seq_` and returns
   `{false, next_seq_, seq}` on mismatch.
2. Confirm `next_seq_` is only incremented on success.

---

### IRS-04 — Timeout-failure retry

**Symptom**: `must take exactly 3 attempts` fails (different attempt count)

**Triage**:
1. Verify `TimeoutAwarePipeline::Execute` increments `timeout_count` only for
   `FailureKind::kTimeout`, not for `kTransient`.
2. Check the loop boundary: `fail_for=2` means attempts 1 and 2 fail,
   attempt 3 succeeds.

---

### IRS-05 — Cascade timeout containment

**Symptom**: `stage3 must succeed independently` fails

**Triage**:
1. Confirm `CascadeContainmentPipeline::Run` evaluates each stage independently
   using its own bitmask bit.
2. Stage 3 result must not depend on stage 1 or 2 `ok` values.

---

### IRS-06 — Auth revocation regression

**Symptom**: `write must fail after token revocation` fails (ok=true)

**Triage**:
1. Verify `MockPipelineAuth::DenyToken` adds the token to `denied_tokens_`.
2. Confirm `Authorize` checks `denied_tokens_` *before* `allowed_tokens_`.

---

### IRS-07 — Partial-update atomicity

**Symptom**: `field_a must remain unchanged` fails (field_a = "UPDATED_A")

**Triage**:
1. Verify `AtomicUpdatePipeline::Update` returns `false` without touching
   `records_` when `fail_mid_update=true`.
2. Check that the staged `Record` copy is never stored.

---

### IRS-08 — Empty vs. null result contract

**Symptom**: `null result and empty result must be distinguishable` fails

**Triage**:
1. Verify `EmptyResultPipeline::Query` returns `{false, {}}` for unregistered keys.
2. Confirm `Register` with an empty vector stores a key with `found=true`.

---

### CCR-01 — Unknown fields tolerated

**Symptom**: V2 record with extra `tags` field fails V1 validation

**Triage**:
1. Verify `SchemaValidator::Validate` only checks fields listed in `schema_`.
   Extra fields in `record` that are *not* in `schema_` must be silently ignored.

---

### CCR-02 — Required-field contract

**Symptom**: `ok` is `true` despite missing required field

**Triage**:
1. Verify `IsConformant` iteration checks `field_def.required` before returning.
2. Confirm `errors` vector is populated with `"missing_required:<name>"`.

---

### CCR-03 — Backward compatibility (V2 reader, V1 record)

**Symptom**: `ReadAsV2` throws or returns `nullopt`

**Triage**:
1. Confirm `ReadAsV2` handles missing `"tags"` key gracefully (no `at()` throw).
2. Check that `VersionedSchemaStore::WriteV1` does not write a `"tags"` entry.

---

### CCR-04 — Forward compatibility (V1 reader, V2 record)

**Symptom**: `ReadAsV1` crashes or returns wrong `value`

**Triage**:
1. Confirm `ReadAsV1` only reads `"name"` and `"value"` fields; `"tags"` is never
   accessed.

---

### CCR-05 — Type contract enforcement

**Symptom**: `type_mismatch:count` not in `errors` despite wrong type

**Triage**:
1. Verify `SchemaValidator` compares `it->second.type != field_def.type`.
2. Confirm `FieldValue::type` is correctly set to `FieldType::kString` in the
   wrong-type record construction.

---

### CCR-06 — API idempotency contract

**Symptom**: second call returns `created=true`

**Triage**:
1. Verify `IdempotentApiEndpoint::Create` looks up `idempotency_map_` *before*
   creating the resource.
2. Check that the mutex covers both the lookup and the insert.

---

### CCR-07 — Pagination contract

**Symptom**: duplicate item across pages, or item count != 25

**Triage**:
1. Verify `PaginatedQueryEngine::Fetch` uses `next_cursor = end` (not `end+1`).
2. Confirm `has_more = (end < items_.size())`.
3. If an item appears in two pages, check the cursor calculation for off-by-one.

---

### CCR-08 — Error contract

**Symptom**: `IsConformant` returns false for one of the factory paths

**Triage**:
1. Check `ApiErrorFactory` for the failing path — ensure `code` and `message`
   are non-empty string literals.
2. Verify no factory uses an uninitialized severity value.

---

### DFQ-01 — Identical seed → identical sequence

**Symptom**: `keys1 != keys2` despite same seed

**Triage**:
1. Confirm both `DeterministicDataProducer` instances are constructed with
   exactly `kCanonicalSeed` and no state is shared between them.

---

### DFQ-02 — Test isolation

**Symptom**: `IsLeaked()` returns true unexpectedly

**Triage**:
1. Confirm `IsolatedTestEnvironment::ResetShared()` is called in `SetUp`.
2. Check that the previous test's env was destroyed (went out of scope).

---

### DFQ-03 — Cleanup completeness

**Symptom**: `OpenCount != 0` or double-close returns true

**Triage**:
1. Verify all elements of `handles` vector are closed in the close loop.
2. Confirm `HandleTracker::Close` erases from `open_handles_` exactly once
   per unique handle ID.

---

### DFQ-04 — Precise assertion messages

**Symptom**: `error_code` is empty on failure

**Triage**:
1. Check `PreciseAssertionPipeline::Write` returns an early `{false, "code", "detail"}`
   before the storage write for each validation failure path.

---

### DFQ-05 — Flake burn-down (N=50)

**Symptom**: One or more repetitions fail

**Triage**:
1. A failure here means a real regression — `FlakeProbeOperation::Increment`
   must always return `delta` after `Reset()`.
2. Run `--gtest_repeat=200` to confirm reproducibility.
3. Escalate immediately to @quality if consistently reproducible.

---

### DFQ-06 — Seed boundary

**Symptom**: seed=0 or seed=UINT32\_MAX produces empty keys

**Triage**:
1. Verify `std::mt19937` accepts `uint32_t` seeds at boundaries without
   undefined behaviour.
2. Confirm `snprintf` format `"key-%08x"` always produces non-empty output.

---

### DFQ-07 — Concurrent determinism

**Symptom**: Final counter != 400 (kThreads × kPerThread)

**Triage**:
1. Run with ThreadSanitizer (see Step 4 above).
2. Confirm `FlakeProbeOperation::Increment` uses
   `fetch_add(delta, std::memory_order_relaxed)` — sufficient for correctness
   in this single-variable scenario.

---

### DFQ-08 — Structured diagnostic output

**Symptom**: `AllWellFormed()` returns false or timestamp non-monotonic

**Triage**:
1. Check `DiagnosticEventSink::Emit` — confirm `timestamp_us` is obtained from
   `std::chrono::steady_clock` and cast to `uint64_t`.
2. If timestamps are equal, the `>=` assertion still passes — only strict
   decrease is a bug.

---

## Flake Policy

| Condition | Action |
|-----------|--------|
| DFQ-05 fails once | Open P0 issue; quarantine test; root-cause within 24 h |
| Any Wave 8 test is flaky > 1 in 200 runs | Open P1; fix or quarantine within 1 sprint |
| TSAN report on DFQ-07 | Treat as P0 data-race; fix before next merge |

---

## Minimal Repro Scripts

### W8A — Single incident regression test

```bash
./tests/integration/pipeline/w8a_incident_regression_shielding_test \
    --gtest_filter="IncidentRegressionShieldingTest.*" \
    --gtest_repeat=3 \
    --gtest_output=xml:/tmp/w8a_repro.xml
```

### W8B — Single contract/compat test

```bash
./tests/integration/pipeline/w8b_contract_compatibility_reliability_test \
    --gtest_filter="ContractCompatibilityReliabilityTest.*" \
    --gtest_repeat=3
```

### W8C — Full determinism/flake suite

```bash
./tests/integration/pipeline/w8c_determinism_flake_ci_signal_test \
    --gtest_filter="DeterminismFlakeCiSignalTest.*" \
    --gtest_repeat=5
```

### Full Wave 8 repro

```bash
#!/usr/bin/env bash
# Wave 8 full repro — run from build directory
set -euo pipefail

echo "=== Wave 8A: Incident Regression Shielding ==="
ctest -R w8a_incident_regression_shielding_test --output-on-failure -V

echo "=== Wave 8B: Contract & Compatibility Reliability ==="
ctest -R w8b_contract_compatibility_reliability_test --output-on-failure -V

echo "=== Wave 8C: Determinism, Flake Burn-down & CI Signal ==="
ctest -R w8c_determinism_flake_ci_signal_test --output-on-failure -V

echo "=== All Wave 8 tests completed ==="
```

---

## Escalation Path

| Severity | Action |
|----------|--------|
| `release_critical` failure on `develop` | Page on-call; block merge; open P0 issue |
| `release_critical` failure on feature branch | Fix before merge; author responsible |
| Flaky test (DFQ-05) | Open P0 issue; quarantine until root cause found |
| CCR schema-compat failure | Author + @api-compat; fix same sprint |

---

*Ownership: @themisdb-core-team | Wave: 8 | Status: Operational*
