# Runbook: Timeseries Store — Operator Incident Triage

<!-- Wave D deliverable | Module: timeseries | Validated: 2026-09-16 -->
<!-- Related: src/timeseries/ROADMAP.md · ARCHITECTURE.md · FUTURE_ENHANCEMENTS.md -->

## Overview

This runbook covers the five most operator-critical incident classes for the
Timeseries Store.  Each scenario includes detection signals, log-pattern
anchors, immediate mitigation steps, and escalation guidance.

---

## Scenario 1 — Flush Stall

**Log pattern**: `[TIMESERIES:FlushStall]`

### Symptoms
- Adaptive flush controller stops issuing flushes for > 30 s.
- In-memory buffer metric `timeseries_buffer_bytes` grows unbounded.
- Write latency degrades as the buffer nears its capacity limit.

### Detection
```
grep '\[TIMESERIES:FlushStall\]' /var/log/themis/timeseries.log | tail -50
```
Check metric:
```
timeseries_buffer_bytes > 2147483648     # 2 GiB
rate(timeseries_flush_total[5m]) == 0   # no flushes in last 5 min
```

### Immediate mitigation
1. Inspect the flush controller state:
   ```
   themis-admin timeseries flush status
   ```
2. Trigger a manual flush:
   ```
   themis-admin timeseries flush now --confirm
   ```
3. If the flush controller is deadlocked, restart it without restarting the
   ingest path:
   ```
   themis-admin timeseries flush restart
   ```
4. Lower the auto-flush threshold to trigger more frequent flushes:
   ```
   themis-admin timeseries config set flush.buffer_threshold_mb 512
   ```

### Escalation
Escalate if manual flush fails or if buffer utilisation exceeds 80 % of
configured capacity.  Attach flush controller log and buffer metrics.

---

## Scenario 2 — Retention Enforcement Failure

**Log pattern**: `[TIMESERIES:RetentionFailed]`

### Symptoms
- Chunks older than the configured retention window are not being purged.
- Disk utilisation grows faster than the ingest rate would suggest.
- Retention run reports zero deletions for several consecutive cycles.

### Detection
```
grep '\[TIMESERIES:RetentionFailed\]' /var/log/themis/timeseries.log | tail -50
```
Check metric:
```
timeseries_retention_delete_total{window="24h"} == 0   # no deletions
```

### Immediate mitigation
1. Check the retention policy configuration:
   ```
   themis-admin timeseries retention list
   ```
2. Force an immediate retention run:
   ```
   themis-admin timeseries retention run --force --confirm
   ```
3. If the retention engine reports permission errors on the storage backend:
   ```
   themis-admin timeseries storage verify-permissions
   ```
4. Remount / re-provision storage with correct permissions and re-run.

### Escalation
Escalate if disk utilisation exceeds 85 % or if forced retention fails.
Attach `themis-admin timeseries retention report --verbose`.

---

## Scenario 3 — Remote-Write Buffer Overflow

**Log pattern**: `[TIMESERIES:RemoteWriteOverflow]`

### Symptoms
- Remote-write send queue is full; new samples are being dropped.
- Metric `timeseries_remote_write_dropped_total` is non-zero and rising.
- Downstream remote-write target reports gaps in received data.

### Detection
```
grep '\[TIMESERIES:RemoteWriteOverflow\]' /var/log/themis/timeseries.log | tail -50
```
Check metric:
```
timeseries_remote_write_dropped_total > 0
timeseries_remote_write_queue_depth > 10000
```

### Immediate mitigation
1. Inspect the remote-write queue health:
   ```
   themis-admin timeseries remote-write status
   ```
2. Temporarily reduce the remote-write batch size to relieve backpressure:
   ```
   themis-admin timeseries config set remote_write.batch_size 100
   ```
3. Scale out remote-write senders if the downstream can absorb higher
   parallelism:
   ```
   themis-admin timeseries remote-write scale --senders 4
   ```
4. If the downstream is unavailable, pause remote-write and drain locally:
   ```
   themis-admin timeseries remote-write pause
   ```

### Escalation
Escalate immediately if sample drops exceed 0.1 % of ingest volume.  Attach
remote-write queue depth chart and downstream connectivity diagnostics.

---

## Scenario 4 — Encrypted Chunk Corruption

**Log pattern**: `[TIMESERIES:ChunkCorruption]`

### Symptoms
- Decryption of one or more chunks fails with integrity or padding errors.
- Range queries over affected time windows return partial or empty results.
- Chunk validation utility reports CRC/MAC mismatches.

### Detection
```
grep '\[TIMESERIES:ChunkCorruption\]' /var/log/themis/timeseries.log | tail -50
```
Run chunk validation:
```
themis-admin timeseries chunk validate --window 24h
```

### Immediate mitigation
1. Identify the corrupted chunk range:
   ```
   themis-admin timeseries chunk list --corrupt
   ```
2. Attempt recovery from the nearest replica (if replication is enabled):
   ```
   themis-admin timeseries chunk recover --from-replica --confirm
   ```
3. If recovery is not possible, quarantine the corrupted chunks to prevent
   cascading query failures:
   ```
   themis-admin timeseries chunk quarantine --ids <chunk_ids> --confirm
   ```
4. Re-ingest from the source WAL or upstream remote-write feed if available.

### Escalation
Chunk corruption is a P0 data-integrity incident.  Page the on-call data
engineer immediately.  Preserve the corrupted chunks before quarantine for
forensic analysis.

---

## Scenario 5 — Query Timeout

**Log pattern**: `[TIMESERIES:QueryTimeout]`

### Symptoms
- Range queries return `DEADLINE_EXCEEDED` errors.
- p99 query latency metric exceeds the configured SLO threshold.
- Client applications report stale dashboards or failed panels.

### Detection
```
grep '\[TIMESERIES:QueryTimeout\]' /var/log/themis/timeseries.log | tail -50
```
Check metric:
```
timeseries_query_latency_p99_ms > 5000   # 5 s threshold
```

### Immediate mitigation
1. Identify the slow query patterns:
   ```
   themis-admin timeseries query slow-log --threshold 2000ms
   ```
2. Kill runaway queries that are holding shared resources:
   ```
   themis-admin timeseries query kill --older-than 30s --confirm
   ```
3. Temporarily lower the global query timeout to shed load:
   ```
   themis-admin timeseries config set query.timeout_ms 2000
   ```
4. If a compaction is running concurrently, suspend it to free I/O bandwidth:
   ```
   themis-admin timeseries compaction pause
   ```
5. Resume compaction once p99 latency drops below 1 000 ms.

### Escalation
Escalate if p99 does not recover within 10 minutes.  Attach the slow-log
output and a Prometheus latency chart.

---

## Reference

| Log Pattern                          | Metric                                        | Owner              |
|--------------------------------------|-----------------------------------------------|--------------------|
| `[TIMESERIES:FlushStall]`            | `timeseries_buffer_bytes`                     | timeseries-storage |
| `[TIMESERIES:RetentionFailed]`       | `timeseries_retention_delete_total`           | timeseries-storage |
| `[TIMESERIES:RemoteWriteOverflow]`   | `timeseries_remote_write_dropped_total`       | timeseries-network |
| `[TIMESERIES:ChunkCorruption]`       | `timeseries_chunk_integrity_errors_total`     | timeseries-data    |
| `[TIMESERIES:QueryTimeout]`          | `timeseries_query_latency_p99_ms`             | timeseries-query   |

---

*Last updated: 2026-09-16 — Wave D operability pass*
