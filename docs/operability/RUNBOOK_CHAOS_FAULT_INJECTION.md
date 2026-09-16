# RUNBOOK: Chaos Fault Injection — Triage & Recovery

**Author:** ThemisDB Contributors
**Created:** 2026-09-16
**Last Updated:** 2026-09-16
**Status:** active

**Audience:** Database Operators, SREs, Resilience Engineering Team Lead
**Purpose:** Triage and recover from fault injection runaways, scheduler deadlocks, callback queue overflows, process scope violations, and fault-injector registry corruption
**Severity:** Medium-High (chaos module is an auxiliary simulation surface; its failure must not disrupt production document or query paths)
**Estimated Duration:** 5 min - 1 hour (depending on failure class)

---

## Overview

This runbook guides operators through diagnosing and recovering from failures in the ThemisDB chaos fault injection framework (`src/chaos/`). The module has four primary surfaces:

1. **Fault injector** — fault injection and recovery registry (`FaultInjector`, `injectFault`, `recoverFault`)
2. **Chaos scheduler** — timed fault scheduling (`ChaosScheduler`, `scheduleIn`, `start`, `stop`)
3. **Callback dispatcher** — event delivery on fault lifecycle transitions (§ 5 of chaos_contract.h)
4. **Fault descriptor registry** — in-process registry of active fault descriptors

**Key Principles:**
- All chaos operations are process-local and non-persistent (§ 7 of chaos_contract.h)
- All invalid operations fail closed (no-op) per `ChaosFailureClass` contract (§ 4)
- Callbacks are FIFO, non-re-entrant, and bounded (§ 5)
- Scheduler FSM has exactly two states: STOPPED and RUNNING (§ 6); idempotent start/stop
- A chaos module failure must never affect the production document or query data paths

---

## Prerequisites Checklist

Before beginning any intervention, verify:

- [ ] Production document/query paths are unaffected (chaos is an auxiliary simulation layer)
- [ ] Access to server logs with `[CHAOS:*]` tag filtering
- [ ] Process memory metrics accessible (`chaos_active_fault_count`, `chaos_scheduler_state`)
- [ ] Knowledge of which fault injection session or test is currently running
- [ ] Wave D D1 trace spans are available (see §Distributed Tracing below) if Phase 2A is deployed

---

## Failure Scenarios

---

### Scenario 1: Fault Injection Runaway (probability=1.0 Stuck)

**Symptoms:**
- Logs contain `[CHAOS:InjectRunaway]` or `[CHAOS:CapacityExceeded]` tags
- `chaos_active_fault_count` metric is at or near `kMaxActiveFaults` (see `chaos_contract.h` § 7)
- Production-adjacent paths experiencing unexpected failures that correlate with fault patterns
- `recoverFault()` calls are not clearing faults

**Log patterns:**
```
[CHAOS:InjectRunaway] FaultInjector::injectFault: active fault count at capacity: <N>/<MAX>
[CHAOS:CapacityExceeded] FaultInjector::injectFault: CapacityExceeded — fault rejected for node <id>
[CHAOS:RecoverFailed] FaultInjector::recoverFault: node_id <id> not found in active registry
```

#### Step 1: Confirm Runaway State

```bash
# Check active fault count against capacity
grep '\[CHAOS:InjectRunaway\]\|\[CHAOS:CapacityExceeded\]' \
  /var/log/themisdb/themisdb.log | tail -20

# Check rate of inject vs recover calls
grep '\[CHAOS:' /var/log/themisdb/themisdb.log | \
  grep -o '\[CHAOS:[A-Za-z]*\]' | sort | uniq -c | sort -rn | head -10
```

#### Step 2: Diagnose Root Cause

| Symptom | Likely Cause | Action |
|---------|-------------|--------|
| `CapacityExceeded` for all new injects | Caller injecting without paired recovery | Identify and fix the caller; run `clearAllFaults()` |
| `RecoverFailed` for existing faults | Node ID mismatch or registry corrupted | Check node_id consistency; call `clearAllFaults()` as fallback |
| `probability=1.0` faults not recovering | Caller omitted `recoverFault()` call | Audit caller code path; add paired recovery |
| Fault count stable but stuck at MAX | Multiple sessions competing without coordination | Restart the chaos framework session |

#### Step 3: Clear All Active Faults (Recovery)

```bash
# Clear all active faults — this is safe (fail-closed, process-local)
themisdb-admin chaos clear-all-faults

# Verify fault count drops to 0
themisdb-admin chaos status
# Expected: active_fault_count=0, scheduler_state=STOPPED

# Confirm production paths are unaffected
themisdb-admin document-store test-round-trip \
  --key "chaos-recovery-probe-$(date +%s)" \
  --content '{"probe": true}'
```

#### Step 4: Validate Recovery

```bash
# Confirm [CHAOS:InjectRunaway] messages stop
grep '\[CHAOS:InjectRunaway\]' /var/log/themisdb/themisdb.log | \
  awk '{print $1, $2}' | tail -5

# Confirm chaos_active_fault_count metric returns to 0
query-metrics --metric chaos_active_fault_count --range 10m --window 1m
```

**Decision Point:**
- ✅ **Fault count at 0, no runaway logs:** Recovery confirmed → incident closed
- ❌ **Faults still accumulating:** Identify and terminate the runaway fault injection session; restart chaos framework

---

### Scenario 2: Scheduler Deadlock

**Symptoms:**
- Logs contain `[CHAOS:SchedulerDeadlock]` or `[CHAOS:StopTimeout]` tags
- `chaos_scheduler_state` metric stuck in `RUNNING` despite `stop()` being called
- Chaos scheduler thread unresponsive; `stop()` call blocks indefinitely

**Log patterns:**
```
[CHAOS:SchedulerDeadlock] ChaosScheduler::stop: stop() blocked for >5s; possible deadlock
[CHAOS:StopTimeout] ChaosScheduler::stop: scheduler thread did not join within timeout
[CHAOS:WakeStrategyError] ChaosScheduler: wake strategy CONDVAR failed to signal
```

#### Step 1: Confirm Deadlock

```bash
# Check for scheduler deadlock events
grep '\[CHAOS:SchedulerDeadlock\]\|\[CHAOS:StopTimeout\]' \
  /var/log/themisdb/themisdb.log | tail -20

# Check scheduler FSM state
themisdb-admin chaos scheduler-state
# Expected output: RUNNING (but stop() was called — confirms stuck state)
```

#### Step 2: Force Stop the Scheduler

```bash
# Option A: Force stop with timeout override
themisdb-admin chaos scheduler-stop --force --timeout 10s

# Option B: Restart the chaos framework (drops all state — safe, process-local)
themisdb-admin chaos restart --clear-all

# Verify scheduler returns to STOPPED
themisdb-admin chaos scheduler-state
# Expected: STOPPED
```

#### Step 3: Diagnose Wake Strategy

```bash
# Check which wake strategy was in use when deadlock occurred
grep '\[CHAOS:WakeStrategyError\]' /var/log/themisdb/themisdb.log | tail -5

# If CONDVAR strategy deadlocked, switch to FIXED_TICK for reliability
themisdb-admin chaos configure --wake-strategy FIXED_TICK --tick-ms 50
```

**Decision Point:**
- ✅ **Scheduler in STOPPED state:** Recovery confirmed → close incident; review wake strategy config
- ❌ **Scheduler still unresponsive:** Restart the chaos service process; file issue with chaos module team

---

### Scenario 3: Callback Queue Overflow

**Symptoms:**
- Logs contain `[CHAOS:CallbackQueueOverflow]` or `[CHAOS:CallbackDropped]` tags
- Fault lifecycle events (inject/recover) are not triggering the expected callbacks
- Dashboard: `chaos_callback_drop_total` rising

**Log patterns:**
```
[CHAOS:CallbackQueueOverflow] ChaosScheduler: callback queue full (depth=<N>); dropping event
[CHAOS:CallbackDropped] FaultInjector: callback dispatch skipped — queue at capacity
[CHAOS:CallbackTimeout] FaultInjector: callback dispatch timed out after <N>ms
```

#### Step 1: Identify the Queue Overflow Rate

```bash
# Check callback overflow events
grep '\[CHAOS:CallbackQueueOverflow\]\|\[CHAOS:CallbackDropped\]' \
  /var/log/themisdb/themisdb.log | tail -30

# Count drops per minute
grep '\[CHAOS:CallbackDropped\]' /var/log/themisdb/themisdb.log | \
  awk '{print $1, $2}' | cut -d: -f1,2 | sort | uniq -c | tail -10
```

#### Step 2: Diagnose Cause

| Symptom | Likely Cause | Resolution |
|---|---|---|
| Queue full immediately after inject burst | Callback handler is slow (blocking I/O in callback) | Make callback handler non-blocking; defer I/O |
| Queue fills gradually | Callback consumer thread is behind the producer | Increase callback queue depth or add consumer threads |
| Drops only under CONDVAR wake strategy | Wake strategy has high jitter | Switch to FIXED_TICK wake strategy |
| Callbacks dropping during `clearAllFaults()` | Mass-recover triggers burst of callbacks | Expected behavior; suppress callbacks during bulk clear |

#### Step 3: Drain the Callback Queue

```bash
# Check current queue depth
themisdb-admin chaos callback-queue-status

# Drain by pausing inject activity (no new events to enqueue)
themisdb-admin chaos pause-inject

# Wait for queue to drain
sleep 5
themisdb-admin chaos callback-queue-status
# Expected: queue_depth=0

# Resume inject activity
themisdb-admin chaos resume-inject
```

**Decision Point:**
- ✅ **Queue drained, drop rate at 0:** Recovery confirmed → review callback handler performance
- ❌ **Queue refilling immediately:** Callback handler is blocking; fix callback implementation

---

### Scenario 4: Process Scope Exceeded

**Symptoms:**
- Logs contain `[CHAOS:ScopeViolation]` or process-level blast radius warnings
- Chaos operations affecting processes or resources outside the intended scope
- `chaos_active_fault_count` exceeds `kMaxActiveFaults` per-process constant (§ 7)

**Log patterns:**
```
[CHAOS:ScopeViolation] FaultInjector: blast-radius violation — fault scope exceeds process boundary
[CHAOS:CapacityExceeded] FaultInjector: kMaxActiveFaults exceeded: <N> active faults
[CHAOS:ProcessScopeWarning] FaultInjector: fault_id <id> targets resource outside allowed scope
```

#### Step 1: Identify the Scope Violation

```bash
# Check scope violation events
grep '\[CHAOS:ScopeViolation\]\|\[CHAOS:ProcessScopeWarning\]' \
  /var/log/themisdb/themisdb.log | tail -20

# Identify which fault IDs are violating scope
grep '\[CHAOS:ScopeViolation\]' /var/log/themisdb/themisdb.log | \
  grep -o 'fault_id [^ ]*' | sort | uniq -c
```

#### Step 2: Restrict Fault Scope

```bash
# Clear all faults immediately to stop the violation
themisdb-admin chaos clear-all-faults

# Review the fault descriptors being injected — ensure they target only
# in-process node IDs and not external resources
themisdb-admin chaos list-active-faults

# Reconfigure the fault injector with explicit scope constraints
themisdb-admin chaos configure --max-active-faults 100 --scope-mode process_local
```

#### Step 3: Validate Scope Compliance

```bash
# Confirm no scope violations after reconfiguration
grep '\[CHAOS:ScopeViolation\]' /var/log/themisdb/themisdb.log | \
  awk '{print $1, $2}' | tail -5

# Verify production paths unaffected
themisdb-admin document-store test-round-trip \
  --key "scope-probe-$(date +%s)" --content '{"probe": true}'
```

**Decision Point:**
- ✅ **No new scope violations:** Recovery confirmed → add scope guard to fault injection callers
- ❌ **Violations continue:** Stop all chaos sessions; review fault descriptor construction in code

---

### Scenario 5: Fault-Injector Registry Corruption

**Symptoms:**
- Logs contain `[CHAOS:RegistryCorruption]` or `[CHAOS:InternalError]` tags
- `isFaultActive()` returns inconsistent results for known-active faults
- `clearAllFaults()` does not reduce active fault count to 0

**Log patterns:**
```
[CHAOS:RegistryCorruption] FaultInjector: registry invariant violated: active_count=<N> != map.size()=<M>
[CHAOS:InternalError] FaultInjector::recoverFault: InternalError — unexpected state for node <id>
[CHAOS:SnapshotMismatch] FaultInjector: snapshot count <N> does not match live registry count <M>
```

#### Step 1: Confirm Registry Corruption

```bash
# Check for registry corruption events
grep '\[CHAOS:RegistryCorruption\]\|\[CHAOS:InternalError\]' \
  /var/log/themisdb/themisdb.log | tail -20

# Get live registry state
themisdb-admin chaos status --verbose
# Look for: active_count vs registry_map_size discrepancy
```

#### Step 2: Full Registry Reset (Safe — Process-Local Only)

```bash
# Registry corruption is always process-local and non-persistent.
# A full reset has zero impact on production data.

# Reset the fault injector registry
themisdb-admin chaos reset-registry --force

# Verify registry is clean
themisdb-admin chaos status
# Expected: active_fault_count=0, registry_consistent=true
```

#### Step 3: Root Cause Analysis

```bash
# Look for concurrent access patterns around corruption events
grep '\[CHAOS:' /var/log/themisdb/themisdb.log | \
  awk -v ts="$(grep -m1 '\[CHAOS:RegistryCorruption\]' /var/log/themisdb/themisdb.log | awk '{print $1}')" \
  '$1 >= ts' | head -50

# Check if any inject/recover calls came from outside the expected thread
grep '\[CHAOS:' /var/log/themisdb/themisdb.log | grep 'thread_id' | \
  awk '{print $NF}' | sort | uniq -c
```

**Decision Point:**
- ✅ **Registry reset, no new corruption:** Recovery confirmed → add thread-safety audit item
- ❌ **Corruption recurs immediately:** Restart the chaos framework process; file a critical bug with chaos module team

---

## Distributed Tracing — Wave D D1 Span Cross-Links

> **Note:** Distributed tracing spans are planned in the Wave D Phase 2A tracing framework
> planned in `docs/operability/WAVE_D_ROADMAP.md` §2A. Until Phase 2A implementation completes
> (Target: Q1 2027), the listed span names are reference identifiers for future instrumentation.

### Chaos Framework D1 Trace Spans

When the Phase 2A tracing SDK is available, the following operator actions map to trace spans:

| Module Surface | D1 Span Name | Baggage Keys | Notes |
|---|---|---|---|
| Fault inject | `chaos.fault.inject` | `node_id`, `fault_type`, `probability`, `active_count` | Status ERROR on CapacityExceeded |
| Fault recover | `chaos.fault.recover` | `node_id`, `was_active`, `active_count_after` | |
| Scheduler start | `chaos.scheduler.start` | `wake_strategy`, `tick_ms` | |
| Scheduler stop | `chaos.scheduler.stop` | `pending_entries`, `drain_duration_ms` | Status ERROR on timeout |
| Callback dispatch | `chaos.callback.dispatch` | `event_type`, `queue_depth`, `handler_duration_ms` | Status ERROR on overflow |
| Registry clear | `chaos.registry.clear_all` | `cleared_count`, `duration_ms` | |
| Scope violation | `chaos.scope.violation` | `fault_id`, `scope_type`, `target_resource` | Status ERROR always |

### Querying Chaos Trace Spans (Phase 2A onwards)

```bash
# Find all fault injection events for a specific node
otel-query --service chaos_framework --operation chaos.fault.inject \
  --baggage "node_id=<node_id>" --range 1h

# Find all scheduler stop events with duration > 1 s (potential deadlock precursors)
otel-query --service chaos_framework --operation chaos.scheduler.stop \
  --baggage "drain_duration_ms>1000" --range 24h

# Find all capacity-exceeded injection failures
otel-query --service chaos_framework --operation chaos.fault.inject \
  --status ERROR --range 24h --include-baggage

# Cross-reference callback drops with queue depth
otel-query --service chaos_framework --operation chaos.callback.dispatch \
  --status ERROR --range 2h --include-baggage
```

### Phase 2A Instrumentation Targets

Once Phase 2A is implemented, add trace points in:

- `src/chaos/chaos_framework.cpp`: Wrap `injectFault()` and `recoverFault()` in `DistributedTraceSpan`
  with baggage `node_id`, `fault_type`, `active_count`
- `src/chaos/chaos_framework.cpp` (scheduler): Wrap `start()` and `stop()` in spans with baggage
  `wake_strategy`, `pending_entries`, `drain_duration_ms`
- Callback dispatch path: Add span events for `FIFO_dispatch`, `overflow_drop`, and `reentry_guard`

---

## Alert Reference

| Alert Name | Threshold | Runbook Step |
|---|---|---|
| `ChaosInjectRunaway` | `chaos_active_fault_count >= kMaxActiveFaults` for > 2 min | Scenario 1 |
| `ChaosSchedulerDeadlock` | `chaos_scheduler_state=RUNNING` for > 30 s after stop() | Scenario 2 |
| `ChaosCallbackQueueOverflow` | Any `[CHAOS:CallbackQueueOverflow]` event | Scenario 3 |
| `ChaosScopeViolation` | Any `[CHAOS:ScopeViolation]` event | Scenario 4 |
| `ChaosRegistryCorruption` | Any `[CHAOS:RegistryCorruption]` event | Scenario 5 |

---

## Escalation Path

1. **First responder (operator):** Follow the applicable Scenario steps above
2. **Chaos/resilience engineering team:** Escalate for registry corruption, scheduler deadlock, or scope violations
3. **Security team:** Escalate immediately if `ScopeViolation` pattern appears to be externally triggered

---

**Runbook Version:** 1.0
**Last Updated:** 2026-09-16
**Owner:** Resilience Engineering Team, Operations Team
**Next Review:** 2027-03-01 (post-Wave D Phase 2A delivery)
