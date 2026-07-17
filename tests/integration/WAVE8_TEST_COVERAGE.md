# Wave 8 Test Coverage — Post-Release Reliability Hardening

<!-- ThemisDB | WAVE8_TEST_COVERAGE.md | Version: 0.0.1 -->

## Overview

Wave 8 is the **post-release reliability hardening** wave.  It focuses on
converting known incidents and near-misses into permanent regression guards,
hardening API/schema/contract boundaries, eliminating residual flakiness, and
formalising test operability for ongoing maintenance.

Wave 8 consists of three test sub-waves (W8A, W8B, W8C) and one documentation
deliverable (W8D).

---

## Sub-Wave Summary

| Sub-wave | File | Tests | CTest Labels | Release Gate |
|----------|------|-------|--------------|--------------|
| W8A | `pipeline/w8a_incident_regression_shielding_test.cpp` | IRS-01..IRS-08 | `wave8;w8a;release_critical;incident_regression` | ✅ Blocks release |
| W8B | `pipeline/w8b_contract_compatibility_reliability_test.cpp` | CCR-01..CCR-08 | `wave8;w8b;contract_compat;release_critical` | ✅ Blocks release |
| W8C | `pipeline/w8c_determinism_flake_ci_signal_test.cpp` | DFQ-01..DFQ-08 | `wave8;w8c;release_critical;determinism;flake_burndown;ci_signal` | ✅ Blocks release |
| W8D | `WAVE8_TEST_COVERAGE.md`, `WAVE8_TRIAGE_RUNBOOK.md` | — | — | Documentation |

---

## W8A — Post-Release Incident Regression Shielding (IRS)

| Test ID | Description | Pass Criterion | Owner |
|---------|-------------|----------------|-------|
| IRS-01 | Invalid state transitions are rejected; state unchanged | Backward/self-loop transitions return `ok=false`; state stays unchanged after rejection; terminal state has no successors | @core-pipeline |
| IRS-02 | Idempotent command bus de-duplicates repeated submissions | Duplicate `command_id` returns `executed=false, duplicate=true`; `ExecutedCount` reflects only distinct IDs | @ingestion |
| IRS-03 | Out-of-order command is rejected by ordered queue | Seq-gap and re-delivery both rejected; correct `expected_seq` returned; next valid seq accepted | @storage |
| IRS-04 | Timeout failures are retriable and counted separately | Timeout counter incremented per timeout failure; succeeds on attempt 3 after 2 timeouts; exhausted retries recorded correctly | @reliability |
| IRS-05 | Stage-N timeout does not block stage N+1 | Stage with timeout marked `ok=false`; independent stages proceed; all three results available | @reliability |
| IRS-06 | Revoked token denied immediately; pre-revocation data intact | Post-revocation write returns `ok=false`; no new key in storage; previously-written key unaffected | @security |
| IRS-07 | Failed atomic update leaves no partial state | Mid-crash update returns `false`; all fields unchanged; no phantom record created | @storage |
| IRS-08 | Empty result set is distinguishable from unknown key | Unknown key: `found=false`; registered key with 0 items: `found=true, items.empty()`; these two are distinguishable | @core-pipeline |

### Pass/Fail Logic

All IRS tests carry the `release_critical` label.  The CI gate
`.github/workflows/09-pr-gates_release-critical-tests.yml` runs
`ctest -L release_critical` and will fail the PR if any test does not pass.

---

## W8B — Contract & Compatibility Reliability Layer (CCR)

| Test ID | Description | Pass Criterion | Owner |
|---------|-------------|----------------|-------|
| CCR-01 | Unknown fields in record tolerated (forward compat) | V1 validator passes V2 record with unknown `tags` field; no errors | @api-compat |
| CCR-02 | Absent required field produces structured error | Missing required `name` → `ok=false`; `errors[0] == "missing_required:name"` | @api-compat |
| CCR-03 | V2 reader tolerates V1 records (backward compat) | V2 reader reads V1 record; `tags` is empty vector; no crash or error | @api-compat |
| CCR-04 | V1 reader ignores extra V2 fields (forward compat) | V1 reader reads V2 record with `tags`; reads `name`/`value` correctly; no crash | @api-compat |
| CCR-05 | Numeric field rejects string-typed value | String value for integer field → `ok=false, errors[0]=="type_mismatch:count"` | @api-compat |
| CCR-06 | Repeated API call with same idempotency key returns same result | Second call: `created=false`; all fields identical to first response; resource count stays 1 | @api-contract |
| CCR-07 | Sequential pages are non-overlapping and cover all items | 25 items, page_size=10 → 3 pages; zero overlap; all 25 items covered | @api-contract |
| CCR-08 | All API error paths produce conformant error structures | Every error factory path: non-empty `code`/`message`; valid severity; `ErrorContractChecker::IsConformant` returns true | @api-contract |

### Pass/Fail Logic

All CCR tests carry the `release_critical` label and block the release PR gate.

---

## W8C — Determinism, Flake Burn-down & CI Signal Quality (DFQ)

| Test ID | Description | Pass Criterion | Owner |
|---------|-------------|----------------|-------|
| DFQ-01 | Identical seed produces identical key sequence | Two producers with seed=42 output byte-identical sequences of 50 keys; different seed ≠ same sequence | @quality |
| DFQ-02 | Independent test environments do not share state | env-A is not leaked on creation; live count correct; reuse after destroy is clean | @quality |
| DFQ-03 | All handles closed after workload; double-close detected | 20 opens → 20 closes; `OpenCount()==0`; `TotalOpens()==TotalCloses()`; double-close returns `false` | @quality |
| DFQ-04 | Validation failures emit precise diagnostic context | Each failure path returns non-empty `error_code` and `expected_detail` including actual values | @quality |
| DFQ-05 | 50 repetitions produce no flaky failure | `Increment(1)` returns 1 across 50 independent iterations; no timing dependency | @quality |
| DFQ-06 | Seed=0 and seed=UINT32\_MAX produce valid distinct sequences | Both boundary seeds produce non-empty keys with correct prefix; seeds produce different sequences | @quality |
| DFQ-07 | Parallel identical increments produce correct total | 4 threads × 100 ops = 400; no race condition or memory ordering bug | @quality |
| DFQ-08 | Anomaly emits well-formed structured diagnostic event | 3 events all well-formed (non-empty source/message, non-zero timestamp); ≥1 ERROR event present; timestamps non-decreasing | @observability |

### Pass/Fail Logic

All DFQ tests carry the `release_critical` label and block the release PR gate.

---

## W8D — Test Operability & Ownership Maturity

Deliverables:

- **`WAVE8_TEST_COVERAGE.md`** (this file) — sub-wave summary, test table, known risks, gate commands, ownership.
- **`WAVE8_TRIAGE_RUNBOOK.md`** — step-by-step triage procedures for every W8 test, minimal repro commands, escalation path.

### Ownership Matrix

| Sub-wave | Primary Owner | Escalation |
|----------|---------------|-----------|
| W8A (IRS) | @core-pipeline, @reliability | @themisdb-core-team |
| W8B (CCR) | @api-compat, @api-contract | @themisdb-core-team |
| W8C (DFQ) | @quality, @observability | @themisdb-core-team |

### Maintenance Policy

| Trigger | Action |
|---------|--------|
| New production incident or near-miss | Add IRS test within 1 sprint of incident close |
| API schema change (add/remove field) | Add/update CCR-01..CCR-05 tests in same PR |
| CI flake rate > 0.5 % on any label | Open P1 issue; add DFQ test within 1 sprint |
| Wave 8 test fails on `develop` | Author fixes before merge; no carry-over |

---

## Known Risks

| Risk | Mitigation |
|------|-----------|
| DFQ-07 concurrent increment: false positive under TSAN | All increments use `std::atomic`; TSAN-clean in CI |
| IRS-05 cascade containment: stage model is in-memory only | Documents the contract pattern; production implementation must replicate independent deadline propagation |
| CCR-07 pagination: depends on stable iteration order | `PaginatedQueryEngine` uses a stable `std::vector`; iteration order is deterministic |
| DFQ-08 timestamp monotonicity: `steady_clock` may resolve to same tick on fast CPUs | Test uses three events with deliberate sequence; same-tick values satisfy `>=` assertion |

---

## Canonical RNG Seed

All Wave 8 tests use `kCanonicalSeed = 42` for any random data generation,
ensuring full reproducibility across environments.

---

## Release Gate Commands

```bash
# Run all Wave 8 release-critical tests
ctest -L "wave8" --output-on-failure

# Run only W8A incident regression shielding
ctest -L "incident_regression" --output-on-failure

# Run only W8B contract/compatibility
ctest -L "contract_compat" --output-on-failure

# Run only W8C determinism/flake/CI signal
ctest -L "flake_burndown" --output-on-failure

# Run all release-critical tests (includes W8A + W8B + W8C)
ctest -L "release_critical" --output-on-failure
```

---

*Ownership: @themisdb-core-team | Wave: 8 | Status: Post-Release Reliability Hardening*
