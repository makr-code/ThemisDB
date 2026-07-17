# Wave 9 Test Coverage

<!-- ThemisDB | WAVE9_TEST_COVERAGE.md | Version: 0.0.1 -->

## Overview

Wave 9 is the security hardening, SLA compliance, and chaos fault-tolerance
wave.  It consists of three sub-waves (W9A, W9B, W9C) covering security
hardening & audit trail, SLA compliance & uptime validation, and chaos
engineering & fault tolerance.

---

## Sub-Wave Summary

| Sub-wave | File | Tests | CTest Labels | Release Gate |
|----------|------|-------|--------------|--------------|
| W9A | `pipeline/w9a_security_hardening_audit_test.cpp`   | SHA-01..SHA-08 | `wave9;w9a;release_critical;security_hardening` | ✅ Blocks release |
| W9B | `pipeline/w9b_sla_compliance_uptime_test.cpp`       | SLA-01..SLA-08 | `wave9;w9b;release_critical;sla_compliance`     | ✅ Blocks release |
| W9C | `pipeline/w9c_chaos_fault_tolerance_test.cpp`       | CFT-01..CFT-08 | `wave9;w9c;release_critical;chaos_fault_tolerance` | ✅ Blocks release |

---

## W9A — Security Hardening & Audit Trail (SHA)

| Test ID | Description | Pass Criterion | Owner |
|---------|-------------|----------------|-------|
| SHA-01 | Auth token rate limiting — token bucket enforces rate limit | Token bucket drains to 0 after 5 attempts; resumes after Refill() | @security |
| SHA-02 | Privilege escalation prevention — unprivileged role returns kForbidden | kForbidden returned; store size and contents unchanged after attempt | @security |
| SHA-03 | Input injection resistance — SQL/AQL injection strings rejected | All injection payloads return kRejected; all benign inputs return kOk | @security |
| SHA-04 | Audit log tamper detection — modified entry detected by integrity check | IntegrityCheck() returns true before tamper; false after TamperEntry() | @security |
| SHA-05 | Credential rotation — old rejected, new accepted after rotate | Old cred returns kExpired; new cred returns kOk within same sequence | @security |
| SHA-06 | Audit log completeness under denied escalations — one event per denial | DeniedCount() == kDeniedOps; Count() == kDeniedOps; IntegrityCheck() true | @security |
| SHA-07 | Replay attack prevention — consumed nonce rejected on second use | First ConsumeNonce() = kOk; second = kReplay; third = kReplay | @security |
| SHA-08 | Audit log ordering under concurrency — monotone gap-free sequences | Count() == kTotalExpected; SequenceIsMonotone() true; IntegrityCheck() true | @security |

### Pass/Fail Logic

All SHA tests carry the `release_critical` label.  Any failure blocks the
release-critical CI gate.

---

## W9B — SLA Compliance & Uptime Validation (SLA)

| Test ID | Description | Pass Criterion | Owner |
|---------|-------------|----------------|-------|
| SLA-01 | Availability window — 1000 requests, 1 failure injected | availability ≥ 99.9%; succeeded = 999; failed = 1 | @reliability |
| SLA-02 | p99 latency compliance — 100 synthetic ops within 10 ms budget | p99 of 100 samples ≤ 10 000 µs | @reliability |
| SLA-03 | Graceful degradation under load — queue-full returns kOverloaded | All overflow enqueue attempts return kOverloaded; queue size unchanged; in-flight drainable | @reliability |
| SLA-04 | RTO — recovery within ≤ 3 retry cycles | RecoverWithRetry(3, 2).recovered == true; cycles_used ≤ 3 | @reliability |
| SLA-05 | RPO — data loss ≤ contract bound (N=5) after simulated crash | lost = total - persisted; lost ≤ kRpoContractMaxLoss | @reliability |
| SLA-06 | Uptime accounting — availability computed from up/down event sequence | Availability() == up/(up+down); 999/1000 scenario ≥ 99.9% | @reliability |
| SLA-07 | Degraded-mode throughput floor — 50% workers disabled; ≥ 40% throughput | ThroughputFraction() ≥ 0.40; relative units ≥ 0.40 | @reliability |
| SLA-08 | SLA gate self-check — counter reports 1.0/0.0 correctly | Report(true) → 1.0; Report(false) → 0.0; Report(true) → 1.0 | @reliability |

### Pass/Fail Logic

All SLA tests carry the `release_critical` label and block the release PR gate.

---

## W9C — Chaos Engineering & Fault Tolerance (CFT)

| Test ID | Description | Pass Criterion | Owner |
|---------|-------------|----------------|-------|
| CFT-01 | Network partition — messages dropped during partition; resume after heal | DeliveredCount before = 1; DroppedCount during = 5; DeliveredCount after = 1+3 | @chaos |
| CFT-02 | Partial write failure — failed batch rolls back to pre-batch state | committed = false; no batch key in store; sentinel intact; key sets equal | @chaos |
| CFT-03 | Cascading failure containment — fault in A does not propagate to B | fault_a.FaultCount() = 1; subsys_b.FaultCount() = 0; subsys_b.SuccessCount() = 2 | @chaos |
| CFT-04 | Self-healing after node restart — restarted node serves reads correctly | IsInCluster() true after Restart(); all snapshot keys readable with correct values | @chaos |
| CFT-05 | Write storm resilience — 8 threads × 50 writes complete without deadlock | succeeded + failed_inj == total; succeeded > 0; store non-empty | @chaos |
| CFT-06 | Read path degradation — primary degraded; secondary path serves reads | value present; correct value returned; used_secondary == true | @chaos |
| CFT-07 | Timeout enforcement — operation exceeding deadline cancelled, kTimedOut | RunWithDeadline(20,10).status == kTimedOut; steps_executed == kDeadlineSteps | @chaos |
| CFT-08 | Chaos gate self-check — counter reports 1.0/0.0 correctly | Report(true) → 1.0; Report(false) → 0.0; Report(true) → 1.0 | @chaos |

### Pass/Fail Logic

All CFT tests carry the `release_critical` label and block the release PR gate.

---

## Execution Commands

```bash
# Run all Wave 9 tests
ctest -L wave9 --output-on-failure -V

# W9A only (security hardening)
ctest -L w9a --output-on-failure -V

# W9B only (SLA compliance)
ctest -L w9b --output-on-failure -V

# W9C only (chaos fault tolerance)
ctest -L w9c --output-on-failure -V

# All release-critical (includes W7 + W8 + W9)
ctest -L release_critical --output-on-failure -j 1
```

---

## Dependencies

| Component | File | Purpose |
|-----------|------|---------|
| Shared fixture | `tests/integration/test_fixture.h` | `IntegrationTestFixture`, `PipelineAuditLog` |
| Data generator | `tests/integration/test_data_generator.h` | `SeededTestDataGenerator` |
| Guidelines | `tests/integration/INTEGRATION_TEST_GUIDELINES.md` | Authoring rules |
| Triage runbook | `tests/integration/WAVE9_TRIAGE_RUNBOOK.md` | Failure investigation |

---

## Metric Thresholds

| Metric | Threshold | Source |
|--------|-----------|--------|
| SHA-04 integrity check before tamper | true | SHA-04 acceptance criterion |
| SHA-08 sequence monotone | true | SHA-08 acceptance criterion |
| SLA-01 availability | ≥ 99.9% | SLA-01 contract |
| SLA-02 p99 | ≤ 10 000 µs | SLA-02 budget |
| SLA-07 throughput fraction | ≥ 0.40 | SLA-07 degraded-mode floor |
| CFT-02 batch rollback completeness | 0 residual keys | CFT-02 acceptance criterion |
| CFT-07 timeout accuracy | steps_executed == kDeadlineSteps | CFT-07 acceptance criterion |
