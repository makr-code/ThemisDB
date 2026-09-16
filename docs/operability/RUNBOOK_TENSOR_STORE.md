# Runbook: Tensor Store — Operator Incident Triage

<!-- Wave D deliverable | Module: tensor | Validated: 2026-09-16 -->
<!-- Related: src/tensor/ROADMAP.md · ARCHITECTURE.md · FUTURE_ENHANCEMENTS.md -->

## Overview

This runbook covers the five most operator-critical incident classes for the
Tensor Store.  Each scenario includes detection signals, log-pattern anchors,
immediate mitigation steps, and escalation guidance.

---

## Scenario 1 — Graph Export Failure

**Log pattern**: `[TENSOR:GraphExportFailed]`

### Symptoms
- Graph export jobs terminate with a non-zero exit code.
- Exported tensor graph files are missing or truncated.
- Downstream replay pipelines report missing graph snapshots.

### Detection
```
grep '\[TENSOR:GraphExportFailed\]' /var/log/themis/tensor.log | tail -50
```

### Immediate mitigation
1. Confirm storage quota on the export destination: `df -h <export_path>`.
2. Retry the export with a reduced batch size:
   ```
   themis-admin tensor export --batch-size 1000 --retry 3
   ```
3. If the failure persists, check the tensor index manager for locked shards:
   ```
   themis-admin tensor status --shard-locks
   ```
4. Release any stale locks with:
   ```
   themis-admin tensor unlock --force --confirm
   ```

### Escalation
Open a P1 incident if exports fail for > 15 minutes or if data loss is
suspected.  Attach `tensor.log` (last 1 000 lines) and the output of
`themis-admin tensor status`.

---

## Scenario 2 — Bridge Routing Stall

**Log pattern**: `[TENSOR:BridgeRoutingStall]`

### Symptoms
- Bridge routing latency p99 exceeds configured SLO threshold.
- Routing queue depth metric `tensor_bridge_queue_depth` grows monotonically.
- Downstream consumers report stale tensor deliveries.

### Detection
```
grep '\[TENSOR:BridgeRoutingStall\]' /var/log/themis/tensor.log | tail -50
```
Check Prometheus:
```
tensor_bridge_routing_latency_p99 > 500   # ms
tensor_bridge_queue_depth > 10000
```

### Immediate mitigation
1. Identify the blocked routing worker:
   ```
   themis-admin tensor bridge status --verbose
   ```
2. Restart the bridge routing pool (zero-downtime rolling restart):
   ```
   themis-admin tensor bridge restart --rolling
   ```
3. If backpressure is caused by a slow downstream, throttle inbound routing:
   ```
   themis-admin tensor bridge throttle --rate 5000
   ```

### Escalation
Escalate to on-call infra if the queue does not drain within 5 minutes of
the rolling restart.

---

## Scenario 3 — Fingerprint Dedup Collision

**Log pattern**: `[TENSOR:DedupCollision]`

### Symptoms
- Tensor dedup pipeline emits `DedupCollision` events at an elevated rate.
- Duplicate tensors appear in the index despite dedup guards.
- Storage utilisation grows faster than expected for the current ingest rate.

### Detection
```
grep '\[TENSOR:DedupCollision\]' /var/log/themis/tensor.log | tail -50
```
Check metric:
```
tensor_dedup_collision_rate > 0.01   # > 1 % collision rate
```

### Immediate mitigation
1. Identify the affected fingerprint range:
   ```
   themis-admin tensor dedup report --window 1h
   ```
2. Invalidate the collision window and trigger re-dedup:
   ```
   themis-admin tensor dedup reindex --window 1h --confirm
   ```
3. If collisions persist, check the hash-function configuration:
   ```
   themis-admin tensor config get dedup.hash_function
   ```
   Upgrade to a stronger hash if the configured one is deprecated.

### Escalation
Escalate if the collision rate exceeds 5 % or if re-dedup does not resolve
within 30 minutes.

---

## Scenario 4 — Replay Lag

**Log pattern**: `[TENSOR:ReplayLag]`

### Symptoms
- Tensor replay pipeline falls behind the ingest rate.
- Replay offset metric `tensor_replay_lag_ms` grows beyond threshold.
- Graph state visible to queries is stale.

### Detection
```
grep '\[TENSOR:ReplayLag\]' /var/log/themis/tensor.log | tail -50
```
Check metric:
```
tensor_replay_lag_ms > 30000   # 30 s lag threshold
```

### Immediate mitigation
1. Check replay worker health:
   ```
   themis-admin tensor replay status
   ```
2. Scale out replay workers (if horizontal scaling is configured):
   ```
   themis-admin tensor replay scale --workers 8
   ```
3. Pause non-critical ingest paths to let replay catch up:
   ```
   themis-admin tensor ingest pause --priority low
   ```
4. Resume ingest once `tensor_replay_lag_ms` drops below 5 000 ms.

### Escalation
Escalate if lag exceeds 5 minutes after scaling; attach replay lag trend chart.

---

## Scenario 5 — Memory Pressure

**Log pattern**: `[TENSOR:MemoryPressure]`

### Symptoms
- Process RSS for `themis-tensor` rises sharply.
- OS reports low available memory on tensor nodes.
- OOM-kill events appear in `/var/log/syslog` for the tensor process.

### Detection
```
grep '\[TENSOR:MemoryPressure\]' /var/log/themis/tensor.log | tail -50
```
Check metric:
```
process_resident_memory_bytes{job="themis-tensor"} > 8589934592  # 8 GiB
```

### Immediate mitigation
1. Trigger an explicit GC / memory reclaim:
   ```
   themis-admin tensor gc --force
   ```
2. Lower the in-memory tensor index budget:
   ```
   themis-admin tensor config set index.memory_budget_mb 2048
   ```
3. Offload least-recently-used tensors to cold storage:
   ```
   themis-admin tensor evict --lru --count 10000
   ```
4. If RSS does not drop within 2 minutes, perform a graceful rolling restart:
   ```
   themis-admin tensor restart --rolling
   ```

### Escalation
Escalate immediately if OOM-kill events occur; this is a P0 incident.
Attach `themis-admin tensor stats --verbose` output.

---

## Reference

| Log Pattern                     | Metric                               | Owner          |
|---------------------------------|--------------------------------------|----------------|
| `[TENSOR:GraphExportFailed]`    | `tensor_export_error_total`          | tensor-infra   |
| `[TENSOR:BridgeRoutingStall]`   | `tensor_bridge_queue_depth`          | tensor-runtime |
| `[TENSOR:DedupCollision]`       | `tensor_dedup_collision_rate`        | tensor-data    |
| `[TENSOR:ReplayLag]`            | `tensor_replay_lag_ms`               | tensor-runtime |
| `[TENSOR:MemoryPressure]`       | `process_resident_memory_bytes`      | tensor-infra   |

---

*Last updated: 2026-09-16 — Wave D operability pass*
