# Wave 8 Test Coverage

<!-- ThemisDB | WAVE8_TEST_COVERAGE.md | Version: 0.0.1 -->

## Overview

Wave 8 is the post-release hardening wave.  It consists of three sub-waves
(W8A, W8B, W8C) covering incident regression shielding, contract compatibility
& reliability, and determinism/flake/CI signal quality.

---

## Sub-Wave Summary

| Sub-wave | File | Tests | CTest Labels | Release Gate |
|----------|------|-------|--------------|--------------|
| W8A | `pipeline/w8a_incident_regression_shielding_test.cpp`        | IRS-01..IRS-08 | `wave8;w8a;release_critical;incident_regression` | ✅ Blocks release |
| W8B | `pipeline/w8b_contract_compatibility_reliability_test.cpp`   | CCR-01..CCR-08 | `wave8;w8b;release_critical;contract_compat`     | ✅ Blocks release |
| W8C | `pipeline/w8c_determinism_flake_ci_signal_test.cpp`          | DFQ-01..DFQ-08 | `wave8;w8c;release_critical;determinism;flake_burndown;ci_signal` | ✅ Blocks release |

---

## W8A — Incident Regression Shielding (IRS)

| Test ID | Description | Pass Criterion | Owner |
|---------|-------------|----------------|-------|
| IRS-01 | Concurrent ingest/delete race — no phantom record | No corrupted values after concurrent write + delete on the same key | @core-pipeline |
| IRS-02 | WAL flush ordering — committed entries visible after replay | Committed WAL entries present; uncommitted entries absent | @storage |
| IRS-03 | Retry storm prevention — total delay bounded under back-off | Total back-off delay ≤ geometric cap; attempt count ≤ max_retries+1 | @reliability |
| IRS-04 | Partial batch rollback — no partial state | Failed mid-batch write leaves storage identical to pre-batch state | @storage |
| IRS-05 | Index/storage divergence after restart | Index and storage agree on key set post-replay | @core-pipeline |
| IRS-06 | Double-delete idempotency | Second delete of absent key returns `false`, not an error | @storage |
| IRS-07 | Large-value read stability (512 KiB, 100 reads) | All 100 reads return bytes identical to written value | @storage |
| IRS-08 | Audit log completeness under concurrent writes | All write events recorded; no duplicate sequence numbers | @observability |

### Pass/Fail Logic

All IRS tests carry the `release_critical` label.  Any failure blocks the
release-critical CI gate (`.github/workflows/09-pr-gates_release-critical-tests.yml`).

---

## W8B — Contract Compatibility & Reliability (CCR)

| Test ID | Description | Pass Criterion | Owner |
|---------|-------------|----------------|-------|
| CCR-01 | Serialization round-trip | Deserialized record == original on all fields | @core-pipeline |
| CCR-02 | Schema evolution (additive) | v1 reader on v2 payload extracts all core fields without error | @api |
| CCR-03 | Interface substitution | Both `InMemoryBackend` and `PrefixedInMemoryBackend` satisfy the same read/write/delete contract | @api |
| CCR-04 | Concurrent reader isolation | No reader observes a value that is neither the pre-write nor post-write state | @storage |
| CCR-05 | Error contract on closed/invalid resource | Operations on absent keys return well-defined sentinels, not exceptions | @api |
| CCR-06 | Empty-collection contracts | Read/delete/query on empty stores return well-defined empty results | @api |
| CCR-07 | Idempotent-write contract | Writing key/value twice produces identical observable state to writing once | @storage |
| CCR-08 | Cross-module pipeline contract (ingest→query) | All ingested records retrievable via query with no transformation loss | @core-pipeline |

### Pass/Fail Logic

All CCR tests carry the `release_critical` label and block the release PR gate.

---

## W8C — Determinism, Flake & CI Signal (DFQ)

| Test ID | Description | Pass Criterion | Owner |
|---------|-------------|----------------|-------|
| DFQ-01 | Seed reproducibility | Two generators with the same seed produce 100 identical keys | @ci |
| DFQ-02 | Operation count determinism under concurrency | Concurrent atomic counter matches expected total after join | @ci |
| DFQ-03 | Output stability under retries | Deterministic computation produces identical output on 20 retries | @ci |
| DFQ-04 | Variance budget (CV < 10%) | Coefficient of variation of synthetic latency samples < 10% | @performance |
| DFQ-05 | Flake categorisation | All timing-dependent flakes tagged and suppressed in deterministic mode | @ci |
| DFQ-06 | CI gate assertion self-check | Gate counter emits 1.0 on pass, 0.0 on fail | @ci |
| DFQ-07 | State isolation between tests | State sentinel is clean at test start; dirty after mutation | @ci |
| DFQ-08 | Monotonic CI signal | Sorted synthetic sample sequence is non-decreasing | @ci |

### Pass/Fail Logic

All DFQ tests carry the `release_critical` label and block the release PR gate.
DFQ-04 failure additionally generates a flake investigation ticket.

---

## Execution Commands

```bash
# Run all Wave 8 tests
ctest -L wave8 --output-on-failure -V

# W8A only (incident regression shielding)
ctest -L w8a --output-on-failure -V

# W8B only (contract compat)
ctest -L w8b --output-on-failure -V

# W8C only (determinism/flake)
ctest -L w8c --output-on-failure -V

# All release-critical (includes W7 + W8)
ctest -L release_critical --output-on-failure -j 1
```

---

## Dependencies

| Component | File | Purpose |
|-----------|------|---------|
| Shared fixture | `tests/integration/test_fixture.h` | `IntegrationTestFixture`, `PipelineAuditLog` |
| Data generator | `tests/integration/test_data_generator.h` | `SeededTestDataGenerator` |
| Guidelines | `tests/integration/INTEGRATION_TEST_GUIDELINES.md` | Authoring rules |
| Triage runbook | `tests/integration/WAVE8_TRIAGE_RUNBOOK.md` | Failure investigation |

---

## Metric Thresholds

| Metric | Threshold | Source |
|--------|-----------|--------|
| IRS-07 large-value mismatch count | = 0 | IRS-07 acceptance criterion |
| CCR-04 corrupt-read count | = 0 | CCR-04 acceptance criterion |
| DFQ-04 CV | < 10% | DFQ-04 variance budget |
| DFQ-01 key mismatches | = 0 | DFQ-01 seed reproducibility |
