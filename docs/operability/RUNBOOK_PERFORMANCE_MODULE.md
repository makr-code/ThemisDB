# Runbook: Performance Module — Wave D Operability

<!-- Runbook: performance | Wave D | validated: 2026-09-16 -->
<!-- Links: src/performance/ROADMAP.md · docs/operability/README.md -->

## Overview

This runbook covers operator-critical incident scenarios for the `performance`
module. Use it to diagnose and remediate metric overflow, baseline regression,
benchmark harness crashes, distributed timeout incidents, and hardware-baseline
anomalies.

---

## Scenario 1 — Metric Overflow

**Log pattern:** `[PERF:MetricOverflow]`

### Symptoms
- Performance metric collection queue exceeds capacity.
- `[PERF:MetricOverflow]` emitted with `metric_name`, `queue_depth`, and `drop_count` fields.
- Monitoring dashboards may show gaps; performance regression gates may miss data.

### Diagnosis
1. Confirm metric overflow:
   ```
   grep '\[PERF:MetricOverflow\]' /var/log/themisdb/performance.log
   ```
2. Identify `metric_name` and `drop_count`.
3. Review collector queue depth:
   ```
   grep 'collector_queue_depth' /var/log/themisdb/performance.log | tail -10
   ```
4. Review `perf_metric_drop_total` metric in Prometheus.

### Remediation
1. Increase collector queue capacity: `performance.collector.max_queue_depth`.
2. Apply metric sampling if cardinality is too high:
   `performance.collector.sampling_rate=0.1`.
3. Scale out metric consumers if ingestion rate is the bottleneck.
4. Confirm `[PERF:MetricOverflow]` stops and drop count returns to zero.

### Escalation
Escalate to the platform team if metric drop rate exceeds 0.1% for more than 2 minutes.

---

## Scenario 2 — Baseline Regression

**Log pattern:** `[PERF:BaselineRegression]`

### Symptoms
- Performance baseline gate detects regression against stored p95/p99 envelopes.
- `[PERF:BaselineRegression]` emitted with `gate_name`, `observed_p99_ms`, `baseline_p99_ms`, and `delta_pct` fields.
- CI pipeline may block release; service may be performing below SLO.

### Diagnosis
1. Confirm baseline regression:
   ```
   grep '\[PERF:BaselineRegression\]' /var/log/themisdb/performance.log
   ```
2. Identify `gate_name` (e.g., `GATE-PERF-01..12`) and `delta_pct`.
3. Compare current vs. baseline measurements:
   ```
   themisdb-admin perf show-baseline --gate <gate_name>
   ```
4. Check for recent code changes or infrastructure changes that correlate with regression.

### Remediation
1. If regression is caused by a code change: revert or optimize the offending commit.
2. If regression is caused by infrastructure change: restore prior hardware/config profile.
3. If new baseline is intentional: update baseline with approval:
   `themisdb-admin perf update-baseline --gate <gate_name> --approve`.
4. Confirm gate passes after remediation.

### Escalation
Escalate to the performance engineering team if regression exceeds 10% and the root cause
is not identified within 30 minutes.

---

## Scenario 3 — Harness Crash

**Log pattern:** `[PERF:HarnessCrash]`

### Symptoms
- Benchmark harness process terminates unexpectedly.
- `[PERF:HarnessCrash]` emitted with `bench_name`, `exit_code`, and `crash_signal` fields.
- Benchmark results for the affected run are incomplete or missing.

### Diagnosis
1. Confirm harness crash:
   ```
   grep '\[PERF:HarnessCrash\]' /var/log/themisdb/performance.log
   ```
2. Identify `bench_name` and `crash_signal` (SIGSEGV, SIGABRT, OOM).
3. Review core dump if available:
   ```
   ls -la /var/crash/themisdb-perf-*
   ```
4. Check system resource state at time of crash (OOM killer, disk full).

### Remediation
1. For OOM crashes: increase harness memory limit or reduce benchmark parallelism.
2. For SIGSEGV: file a bug report with the crash context; do not ship the affected benchmark.
3. For SIGABRT: check assertion violations in harness initialization code.
4. Re-run benchmark after addressing root cause.

### Escalation
Escalate to the performance engineering team for any harness crash with SIGSEGV or SIGABRT
signals.

---

## Scenario 4 — Distributed Timeout

**Log pattern:** `[PERF:DistributedTimeout]`

### Symptoms
- Distributed performance scenario exceeds configured timeout.
- `[PERF:DistributedTimeout]` emitted with `scenario_id`, `timeout_ms`, and `completed_pct` fields.
- Distributed performance gates may not complete; partial results may be unreliable.

### Diagnosis
1. Confirm distributed timeouts:
   ```
   grep '\[PERF:DistributedTimeout\]' /var/log/themisdb/performance.log
   ```
2. Identify `scenario_id` and `completed_pct` (percentage of scenario that completed).
3. Check network latency between participating nodes.
4. Review `perf_distributed_scenario_timeout_total` metric.

### Remediation
1. Increase scenario timeout: `performance.distributed.scenario_timeout_ms`.
2. Check for network partition or elevated inter-node latency.
3. Reduce scenario parallelism to lower coordination overhead.
4. Confirm distributed scenarios complete within SLO after adjustment.

### Escalation
Escalate to the infrastructure team if inter-node latency is elevated or if network
partition is suspected.

---

## Scenario 5 — Hardware-Baseline Gate Pending

**Log pattern:** `[PERF:BaselineRegression]` with `gate_class=hardware_baseline`

### Symptoms
- Hardware-dependent baseline gate is not yet confirmed.
- p95/p99 benchmark results are available but hardware baseline confirmation is pending.
- Wave D sign-off is blocked on hardware baseline run.

### Diagnosis
1. Check hardware baseline gate status:
   ```
   themisdb-admin perf show-baseline --gate-class hardware_baseline
   ```
2. Determine if the gate has been run on representative hardware (not sandbox).
3. Review `docs/operability/RUNBOOK_PERFORMANCE_MODULE.md` hardware requirements.

### Remediation
1. Schedule hardware baseline run on representative production-class hardware.
2. After run: update gate values with `themisdb-admin perf update-baseline --hardware-confirmed`.
3. Document hardware profile used for baseline in `benchmarks/performance/BASELINE_HARDWARE.md`.
4. Mark hardware-baseline gate items as `[x]` in `src/performance/ROADMAP.md`.

### Escalation
Escalate to the release team if hardware baseline confirmation is blocking Wave D sign-off
and hardware is not available within the release window.
