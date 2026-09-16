# Runbook: Process Manager — Operator Remediation Guide

<!-- Status: current | Created: 2026-09-16 | Wave D delivery -->
<!-- Links: src/process/ROADMAP.md · ARCHITECTURE.md · FUTURE_ENHANCEMENTS.md -->

## Overview

This runbook provides operator procedures for the five most critical incident
classes in the ThemisDB **Process Manager** (process model lifecycle, spawn,
signal handling, and resource management).  Each scenario includes detection
signals, root-cause analysis steps, and remediation actions.

**Module error codes:** E7600–E7699 (process module range)  
**Log prefix format:** `[PROCESS:<IncidentClass>]`

---

## Scenario 1 — Spawn Failure

**Log pattern:** `[PROCESS:SpawnFailed]`

### Detection

```
[PROCESS:SpawnFailed] process_id=<ID> model_id=<MID> reason=<MSG> error_code=E76xx
```

- Alert fires when the `themis_process_spawn_failure_total` counter
  increments more than 5 times in 60 s.
- Spawn success rate drops below 99 % on the Grafana panel
  *Process Manager / Spawn Rate*.

### Root Cause Analysis

1. Check system `ulimit -u` (max user processes) — `fork()` fails when the
   kernel process table is exhausted.
2. Confirm cgroup `pids.max` limit:
   ```bash
   cat /sys/fs/cgroup/pids/themisdb/pids.max
   ```
3. Look for preceding `[PROCESS:ZombieAccumulation]` events — zombie buildup
   consumes PID slots.
4. Check for invalid model descriptors: `[PROCESS:SpawnFailed] reason=invalid_model`.

### Remediation

| Step | Action |
|------|--------|
| 1 | Increase `ulimit -u` or `pids.max` cgroup limit and restart ThemisDB. |
| 2 | If zombies are the root cause, apply **Scenario 2** remediation first. |
| 3 | For invalid models, reject and quarantine the model via `themis-admin process quarantine <MID>`. |
| 4 | If spawn failures continue: reduce `process.max_concurrent_spawns` to back-pressure upstream. |

### Escalation

Escalate if spawn failure rate stays above 1 % after step 4 for > 10 min.
Provide `themis-diag dump process` output and `dmesg | tail -50`.

---

## Scenario 2 — Zombie Process Accumulation

**Log pattern:** `[PROCESS:ZombieAccumulation]`

### Detection

```
[PROCESS:ZombieAccumulation] zombie_count=<N> threshold=<T> oldest_pid=<PID>
```

- Alert fires when `themis_process_zombie_count > 10` for > 30 s.
- Confirmed via: `ps aux | awk '$8=="Z" {print}' | wc -l`

### Root Cause Analysis

1. Verify the SIGCHLD handler is registered:
   ```bash
   grep SIGCHLD /proc/<themisdb_pid>/status
   ```
2. Check if `waitpid()` calls are non-blocking and called frequently enough
   in the reap loop.
3. Identify the zombie PIDs and their parent: `ps -eo pid,ppid,stat | grep Z`.
4. Correlate with high spawn rate periods — reap loop may be starved.

### Remediation

| Step | Action |
|------|--------|
| 1 | Trigger an immediate reap cycle: `themis-admin process reap-zombies`. |
| 2 | Increase reap frequency via `process.reap_interval_ms` config (lower value = more frequent). |
| 3 | If zombie count > 100 and rising: perform a rolling restart of the process manager pod. |
| 4 | Long-term: audit SIGCHLD handler for race conditions after high-volume spawn bursts. |

---

## Scenario 3 — Signal Handler Crash

**Log pattern:** `[PROCESS:SignalHandlerCrash]`

### Detection

```
[PROCESS:SignalHandlerCrash] signal=<SIGNUM> handler=<HANDLER_NAME> fault_addr=<ADDR>
```

- Alert fires when `themis_process_signal_handler_fault_total` increments.
- Typically surfaces as a SIGSEGV inside a signal handler (double-fault).

### Root Cause Analysis

1. Capture crash context from the core dump (if enabled).
2. Identify the offending signal handler via `addr2line`:
   ```bash
   addr2line -e themisdb <FAULT_ADDR>
   ```
3. Check for async-signal-unsafe function calls inside the handler (e.g.,
   `malloc`, `printf`, `mutex lock`).

### Remediation

| Step | Action |
|------|--------|
| 1 | Restart immediately: `systemctl restart themisdb` — signal handler crashes are unrecoverable. |
| 2 | Disable the affected signal extension temporarily via `process.signal_extensions_enabled: false`. |
| 3 | File a P1 bug with the fault address, signal number, and core dump. |
| 4 | Apply the async-signal-safety fix in a hotfix release; re-enable extension after verification. |

### Escalation

Always escalate — signal handler crashes indicate a code defect.  P1 SLA.

---

## Scenario 4 — File Descriptor Exhaustion

**Log pattern:** `[PROCESS:FdExhaustion]`

### Detection

```
[PROCESS:FdExhaustion] fd_count=<N> fd_limit=<L> process_id=<PID>
```

- Alert fires when `themis_process_open_fd_count / fd_limit > 0.90`.
- Confirmed via: `ls /proc/<themisdb_pid>/fd | wc -l`

### Root Cause Analysis

1. Identify which subsystem holds the most fds:
   ```bash
   ls -la /proc/<PID>/fd | awk '{print $NF}' | sort | uniq -c | sort -rn | head -20
   ```
2. Look for fd leaks correlated with `[PROCESS:SpawnFailed]` events — a
   failed spawn may leave a pipe fd open.
3. Check for leaked eventfd/epoll descriptors in the signal-delivery path.

### Remediation

| Step | Action |
|------|--------|
| 1 | Increase the fd limit: `ulimit -n 65536` and set in `/etc/security/limits.conf`. |
| 2 | Trigger a controlled gc cycle: `themis-admin process gc-fds`. |
| 3 | If fd count is still growing: restart ThemisDB to reclaim all leaked fds. |
| 4 | Deploy the fd-leak fix as a hotfix after root-cause identification. |

---

## Scenario 5 — Cgroup Limit Breach

**Log pattern:** `[PROCESS:CgroupBreach]`

### Detection

```
[PROCESS:CgroupBreach] resource=<cpu|memory|pids> usage=<U> limit=<L> action=throttled
```

- Alert fires when the cgroup controller reports throttling on any resource.
- Metrics: `container_cpu_cfs_throttled_seconds_total`,
  `container_memory_usage_bytes`, `container_processes`.

### Root Cause Analysis

1. Identify the breached resource from the log (`cpu`, `memory`, or `pids`).
2. **CPU breach:** check for runaway process model evaluation tasks (high CPU
   usage with no progress in `themis_process_model_eval_latency_p99`).
3. **Memory breach:** see `[PROCESS:SpawnFailed]` — each spawned model
   instance holds memory until terminated.
4. **PID breach:** correlate with `[PROCESS:ZombieAccumulation]` and
   `[PROCESS:SpawnFailed]`.

### Remediation

| Step | Action |
|------|--------|
| 1 | **CPU:** lower `process.max_concurrent_evaluations` to shed CPU-intensive tasks. |
| 2 | **Memory:** reduce `process.max_model_instance_memory_mb` and evict stale instances via `themis-admin process evict-stale`. |
| 3 | **PID:** apply Scenario 2 (zombie reap) immediately. |
| 4 | Increase the cgroup limit if hardware capacity permits and the limit is below the production baseline. |
| 5 | If breach persists: add a replica node and redistribute spawn load. |

---

## Reference

| Item | Value |
|------|-------|
| Log prefix | `[PROCESS:<IncidentClass>]` |
| Config file | `config/themisdb.yaml` — `process:` block |
| Metrics prefix | `themis_process_*` |
| Admin CLI | `themis-admin process` |
| Source | `src/process/` |
| Architecture | `src/process/ARCHITECTURE.md` |
| Wave D soak test | `tests/integration/test_process_manager_soak.cpp` |
| Stress tests | `tests/process/test_process_highcardinality_stress.cpp` |
| Benchmark gates | `benchmarks/process/bench_process_dedicated_gates.cpp` |
