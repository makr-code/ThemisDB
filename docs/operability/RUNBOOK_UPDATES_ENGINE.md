# Runbook: Updates Engine

> **Module**: `src/updates`
> **Wave**: D
> **Version**: 1.0.0
> **Owner**: ThemisDB Platform Engineering
> **See also**: [`src/updates/ROADMAP.md`](../../src/updates/ROADMAP.md),
> [`docs/operability/WAVE_D_ROADMAP.md`](WAVE_D_ROADMAP.md)

---

## Purpose

This runbook provides operator triage and remediation procedures for the
ThemisDB Updates Engine. It covers the five operator-critical failure
scenarios identified during Wave D.

---

## Scenario 1 — Update Batch Failure

### Symptom
Log pattern: `[UPDATES:BatchFailed]`

Write operations to the update batch are failing or being dropped. Throughput
may drop sharply; error rates in metrics will spike.

### Diagnosis
1. Search logs for `[UPDATES:BatchFailed]` entries; note the affected keys
   and error codes.
2. Check the updates engine error metrics for error class
   `PatchApply` or `StateTransition`.
3. Verify that the WAL is not full or corrupted:
   ```
   themis-ctl updates wal-status
   ```

### Remediation
1. If the WAL is full, rotate it and clear acknowledged entries:
   ```
   themis-ctl updates wal-rotate
   ```
2. If patch apply errors are persistent, roll back to the last known-good
   state:
   ```
   themis-ctl updates rollback --to-version <VERSION>
   ```
3. Confirm the batch write success rate recovers in metrics.
4. Monitor for `[UPDATES:BatchFailed]` clearance.

---

## Scenario 2 — Delta Conflict

### Symptom
Log pattern: `[UPDATES:DeltaConflict]`

Concurrent delta operations are conflicting. Stale-write conflicts indicate
an ordering problem between writers.

### Diagnosis
1. Identify the conflicting key range from log entries tagged
   `[UPDATES:DeltaConflict]`.
2. Check whether multiple writers are targeting the same key partition
   without proper sequencing.
3. Review the update coordinator configuration for concurrency limits.

### Remediation
1. Reduce writer concurrency for the affected partition:
   ```
   themis-ctl updates config set max_concurrent_writers=<N>
   ```
2. If the conflict is due to clock skew, re-synchronize NTP on affected nodes.
3. Force a consistency check on the affected key range:
   ```
   themis-ctl updates verify --key-prefix <PREFIX>
   ```
4. Monitor for `[UPDATES:DeltaConflict]` clearance.

---

## Scenario 3 — Performance Baseline Regression

### Symptom
Log pattern: `[UPDATES:BaselineRegression]`

Write throughput variance across measurement windows exceeds the 20%
threshold, or throughput drops below 2 000 ops/sec sustained.

### Diagnosis
1. Run the Wave D soak test to reproduce the measurement:
   ```
   ctest -R test_updates_engine_soak -V
   ```
2. Compare recent benchmark results against the baseline in
   `benchmarks/updates/`.
3. Check for resource contention (CPU, memory, disk I/O) on the host.

### Remediation
1. Identify and resolve resource contention (co-located processes, noisy
   neighbours on shared hardware).
2. Review recent changes to the updates pipeline for accidental regression.
3. Re-run the performance baseline benchmark after remediation:
   ```
   benchmarks/updates/bench_updates_performance_baselines_q1_2027
   ```
4. Update the baseline in `docs/operability/WAVE_D_ACCEPTANCE_CHECKLIST.md`
   once the regression is resolved.

---

## Scenario 4 — Exporter Lag

### Symptom
Log pattern: `[UPDATES:ExporterLag]`

The metrics/event exporter for the updates engine is lagging behind the
write pipeline. Observability data may be delayed or dropped.

### Diagnosis
1. Check the exporter queue depth metric:
   ```
   themis-ctl exporter status --module updates
   ```
2. Look for `[UPDATES:ExporterLag]` log entries and note the lag duration.
3. Verify that the export destination (Prometheus, OTLP endpoint) is
   reachable from the updates node.

### Remediation
1. If the destination is unreachable, restore connectivity and the exporter
   will drain the queue automatically.
2. If the queue is permanently full, increase the exporter buffer size:
   ```
   themis-ctl config set updates.exporter.queue_depth=<N>
   ```
3. If data loss is acceptable for the affected window, flush and reset the
   exporter queue:
   ```
   themis-ctl exporter flush --module updates
   ```
4. Monitor the `exporter_lag_seconds` metric for clearance.

---

## Scenario 5 — Rollback Failure

### Symptom
The rollback mechanism fails during a failed update, leaving the system in a
partially applied state.

### Diagnosis
1. Check update state machine logs for `StateTransition` error class entries.
2. Confirm the rollback target version exists in the WAL:
   ```
   themis-ctl updates wal-list
   ```
3. Identify any migration locks that may be blocking the rollback.

### Remediation
1. Force-release any stale migration locks:
   ```
   themis-ctl updates migration unlock --force
   ```
2. Retry the rollback to the last known-good version:
   ```
   themis-ctl updates rollback --to-version <VERSION> --force
   ```
3. Validate post-rollback state:
   ```
   themis-ctl updates verify --full
   ```
4. Escalate to the platform team if the rollback cannot complete cleanly.

---

## Log Pattern Reference

| Pattern                          | Scenario                            |
|----------------------------------|-------------------------------------|
| `[UPDATES:BatchFailed]`          | Write batch failure or WAL error    |
| `[UPDATES:DeltaConflict]`        | Concurrent delta ordering conflict  |
| `[UPDATES:BaselineRegression]`   | Throughput baseline regression      |
| `[UPDATES:ExporterLag]`          | Metrics exporter queue lag          |

---

## Escalation

If remediation steps do not resolve the incident within 30 minutes, escalate
to the ThemisDB on-call engineer with:
- Relevant log excerpts (tagged with the log patterns above)
- Output of `themis-ctl updates wal-status`
- Output of `themis-ctl updates verify --full`
- Benchmark results if a throughput regression is suspected
