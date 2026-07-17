# Wave 9 Triage Runbook

<!-- ThemisDB | WAVE9_TRIAGE_RUNBOOK.md | Version: 0.0.1 -->

## Purpose

Step-by-step triage procedures for any Wave 9 test failure in CI or local
development.  Follow this runbook before escalating to the on-call team.

---

## Quick Reference

| Label | Reproduce command | Escalation owner |
|-------|-------------------|-----------------|
| `security_hardening`      | `ctest -L security_hardening --output-on-failure -V`       | @security |
| `sla_compliance`          | `ctest -L sla_compliance --output-on-failure -V`           | @reliability |
| `chaos_fault_tolerance`   | `ctest -L chaos_fault_tolerance --output-on-failure -V`    | @chaos |
| `wave9` (all)             | `ctest -L wave9 --output-on-failure -V`                    | @themisdb-core-team |

---

## General Triage Steps

### Step 1 — Identify the failing test

```bash
# From the build directory
ctest -L wave9 --output-on-failure 2>&1 | grep -E "FAILED|PASSED|Test #"
```

### Step 2 — Reproduce locally

```bash
# Reproduce a single failing test (example: W9A)
ctest -R w9a_security_hardening_audit_test --output-on-failure -V

# Verbose GTest output
./w9a_security_hardening_audit_test --gtest_filter="*" \
    --gtest_output=xml:/tmp/w9a_results.xml
```

### Step 3 — Check for non-determinism

```bash
# Re-run the same test 5 times and compare pass/fail counts
for i in $(seq 1 5); do
    ctest -R w9c_chaos_fault_tolerance_test --output-on-failure 2>&1 \
        | grep -c "PASSED\|FAILED"
done
```

If failure rate < 100%, the test is likely a flake — open a `flake` label issue
and reference the W9 category.

### Step 4 — Inspect audit state (SHA-04, SHA-06, SHA-08 failures)

```bash
./w9a_security_hardening_audit_test \
    --gtest_filter="*SHA04*|*SHA06*|*SHA08*" --gtest_also_run_disabled_tests -v
```

### Step 5 — Check concurrent access failures (SHA-08, CFT-05)

```bash
# Rebuild with ThreadSanitizer
cmake -DCMAKE_CXX_FLAGS="-fsanitize=thread" ...
ctest -R "w9a|w9c" --output-on-failure
```

---

## W9A — Security Hardening & Audit Failures

### SHA-01 (token bucket)

**Symptom:** `token consumption must succeed after bucket refill` fails  
**Root cause:** Refill() not restoring tokens to positive value.  
**Fix:** Verify `tokens_ = std::min(tokens_ + refill_amt_, capacity_)` is correct.

### SHA-02 (privilege escalation)

**Symptom:** `unprivileged write must return kForbidden` fails  
**Root cause:** Role check not evaluated before data mutation.  
**Fix:** Ensure role is checked before any write to `data_` map.

### SHA-03 (injection resistance)

**Symptom:** An injection payload returns kOk instead of kRejected  
**Root cause:** Missing injection pattern or case-insensitive match not applied.  
**Fix:** Verify ToUpper() is applied to both input and pattern before comparison.

### SHA-04 (tamper detection)

**Symptom:** `audit log integrity check must detect the tampered entry` fails  
**Root cause:** `hash_at_write` updated even after TamperEntry() call.  
**Fix:** Confirm TamperEntry() modifies only the entry data, not `hash_at_write`.

### SHA-07 (replay prevention)

**Symptom:** Second ConsumeNonce() returns kOk instead of kReplay  
**Root cause:** Nonce not inserted into `used_` set on first consumption.  
**Fix:** Confirm the insert + check uses the return value of `std::unordered_set::insert`.

### SHA-08 (audit ordering)

**Symptom:** `SequenceIsMonotone()` returns false  
**Root cause:** Sequence counter (`seq_`) and `push_back()` not under the same lock.  
**Fix:** Both the `fetch_add` counter and vector push must be inside the same mutex scope.

---

## W9B — SLA Compliance Failures

### SLA-01 (availability)

**Symptom:** `availability N must be ≥ 0.999` fails  
**Root cause:** More than one failure injected or succeeded/total computation incorrect.  
**Fix:** Verify exactly one element in `fail_at` and that Availability() = succeeded/total.

### SLA-03 (graceful degradation)

**Symptom:** `queue size must not increase on overloaded attempts` fails  
**Root cause:** Enqueue() inserts before checking capacity.  
**Fix:** Check `queue_.size() >= capacity_` before inserting.

### SLA-04 (RTO)

**Symptom:** `system must recover within the RTO budget` fails  
**Root cause:** succeed_on_cycle > max_cycles causes loop to exit without recovery.  
**Fix:** Verify RecoverWithRetry() sets recovered = true when c >= succeed_on_cycle.

### SLA-07 (degraded throughput)

**Symptom:** `degraded throughput fraction N must be ≥ floor 0.40` fails  
**Root cause:** Disabled workers count exceeds total, resulting in 0 active workers.  
**Fix:** Ensure active = max(0, total - disabled); verify 50% disabled leaves 50% active.

---

## W9C — Chaos Fault Tolerance Failures

### CFT-01 (partition)

**Symptom:** `message N must be dropped during partition` fails  
**Root cause:** Partition flag not checked atomically in Send().  
**Fix:** Ensure Send() reads `partitioned_` under the mutex or atomically.

### CFT-02 (partial write rollback)

**Symptom:** `rollback failed: key still present: batch_key_N` fails  
**Root cause:** ExecuteBatch() commits staging even when fail_at is triggered.  
**Fix:** Confirm staging map is discarded (not committed) when fail_at is hit.

### CFT-03 (bulkhead)

**Symptom:** `subsystem B must remain operational after a fault in A` fails  
**Root cause:** Exception from A's Execute() propagates to B's call site.  
**Fix:** Confirm BulkheadSubsystem::Execute() catches all exceptions internally.

### CFT-04 (node restart)

**Symptom:** `restarted node must serve key: key_X` fails  
**Root cause:** Restart() not copying the snapshot into `local_`.  
**Fix:** Verify `local_ = snapshot` in the Restart() method.

### CFT-07 (timeout)

**Symptom:** `timed-out operation must not run all steps` fails  
**Root cause:** Loop body does not check `s >= deadline_steps` before incrementing.  
**Fix:** Ensure the deadline check fires before `++executed` for steps beyond deadline.

---

## Escalation Policy

| Failure type | SLA | Action |
|---|---|---|
| SHA (security hardening) | 2 h | Page @security; block merge; open security incident |
| SLA (compliance) | 4 h | Notify @reliability; block merge |
| CFT (chaos tolerance) | 2 h | Page @chaos; block merge; run TSAN |
| Any `release_critical` failure | Immediate | Block merge via CI gate |
