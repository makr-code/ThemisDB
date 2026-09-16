# RUNBOOK: Sharding Module — Operator Incident Triage

<!-- Status: current | validated: 2026-09-16 -->
<!-- Module: sharding | Wave: D -->

## Overview

This runbook covers the five most common sharding incident classes encountered
in production ThemisDB clusters.  Each scenario includes detection signals,
log patterns, triage steps, mitigation, and escalation criteria.

---

## Scenario 1 — Shard Split Failure

### Description
A shard split operation (triggered by hot-shard overload or rebalance policy)
fails to complete, leaving the shard topology in a partially-split state.

### Detection
- Log pattern: `[SHARDING:SplitFailed]`
- Prometheus: `sharding_split_failures_total` counter increments
- Alert: `ShardSplitFailureAlert` fires after 2 consecutive failures

### Triage Steps
1. Check coordinator logs for `[SHARDING:SplitFailed]` entries.
2. Identify the shard ID and target split point in the log context.
3. Verify WAL integrity for the affected shard:
   ```
   themis-admin sharding wal-verify --shard <id>
   ```
4. Check quorum status: `themis-admin sharding quorum-status`
5. If quorum is lost, do NOT retry the split; escalate immediately.

### Mitigation
- If WAL is intact and quorum is healthy: retry the split with
  `themis-admin sharding split --shard <id> --force-retry`.
- If WAL shows corruption: follow **Scenario 5 (Routing Table Corruption)**.

### Escalation
Escalate if: split fails 3× consecutively, quorum is degraded, or data
checksums diverge between the original and target shards.

---

## Scenario 2 — Migration Stall

### Description
A key-range migration between shards stalls — the migration coordinator does
not advance the migration cursor for > 30 seconds.

### Detection
- Log pattern: `[SHARDING:MigrationStall]`
- Prometheus: `sharding_migration_stall_seconds` gauge exceeds threshold
- Alert: `ShardMigrationStallAlert`

### Triage Steps
1. Identify the stalled migration ID from `[SHARDING:MigrationStall]` log.
2. Check source and destination shard health:
   ```
   themis-admin sharding shard-health --shard <src> --shard <dst>
   ```
3. Look for replication lag on the destination shard — a lagging replica
   blocks migration cursor advancement.
4. Check for WAL backpressure on the source shard.

### Mitigation
- Reduce write pressure on the source shard during migration.
- If destination lag is the cause, wait for replica catch-up or
  temporarily reduce migration batch size:
  ```
  themis-admin sharding migration set-batch-size --migration-id <id> --size 100
  ```
- If stall persists > 5 min: abort and reschedule:
  ```
  themis-admin sharding migration abort --migration-id <id>
  ```

### Escalation
Escalate if the abort command itself times out, or if data loss is suspected
(checksum mismatch between source and destination).

---

## Scenario 3 — Anti-Entropy Divergence

### Description
Anti-entropy reconciliation detects persistent divergence between shard
replicas that does not resolve within the configured convergence window.

### Detection
- Log pattern: `[SHARDING:AntiEntropyDivergence]`
- Prometheus: `sharding_anti_entropy_divergent_pairs` gauge > 0
- Alert: `ShardAntiEntropyDivergenceAlert`

### Triage Steps
1. Check which shard pair is diverging from `[SHARDING:AntiEntropyDivergence]` logs.
2. Compare checksums:
   ```
   themis-admin sharding checksum --shard <id> --replica <replica-id>
   ```
3. Check WAL gaps on the divergent replica:
   ```
   themis-admin sharding wal-gaps --shard <id> --replica <replica-id>
   ```
4. Verify network connectivity between the primary and the divergent replica.

### Mitigation
- If WAL gap detected: trigger full resync:
  ```
  themis-admin sharding resync --shard <id> --replica <replica-id>
  ```
- If network issue: resolve connectivity, then allow anti-entropy to converge
  automatically within the next 2 reconciliation cycles.

### Escalation
Escalate if divergence persists across 3 anti-entropy cycles post-resync, or
if key-count deltas suggest data was permanently lost.

---

## Scenario 4 — Hot-Shard Overload

### Description
A single shard receives disproportionately high write/read traffic, causing
latency spikes and potentially triggering queue saturation.

### Detection
- Log pattern: `[SHARDING:HotShard]`
- Prometheus: `sharding_hot_shard_ops_per_sec` exceeds 3× the cluster mean
- Alert: `ShardHotShardAlert`

### Triage Steps
1. Identify the hot shard ID and client key-space pattern from logs.
2. Check routing table for uneven distribution:
   ```
   themis-admin sharding routing-stats --top 10
   ```
3. Determine if the hot shard is caused by a sequential key pattern or
   a specific client workload.
4. Verify the hot shard is not also a migration source (compound stress).

### Mitigation
- For sequential key hot spots: enable virtual nodes to spread load:
  ```
  themis-admin sharding rebalance --strategy virtual-nodes --factor 8
  ```
- For client workload hot spots: apply client-side key hashing guidance.
- Emergency: temporarily route reads for the hot key range to a replica.

### Escalation
Escalate if queue depth on the hot shard exceeds kMaxDepth for > 60 s,
or if write latency p99 exceeds 10× the normal baseline.

---

## Scenario 5 — Routing Table Corruption

### Description
The routing table on one or more coordinator nodes becomes inconsistent,
causing requests to be misrouted or rejected with `SHARD_UNAVAILABLE` errors.

### Detection
- Log pattern: `[SHARDING:RoutingCorruption]`
- Prometheus: `sharding_routing_errors_total` spikes
- Alert: `ShardRoutingCorruptionAlert`

### Triage Steps
1. Confirm the scope: how many coordinator nodes report `[SHARDING:RoutingCorruption]`?
2. Dump the routing table from a healthy coordinator:
   ```
   themis-admin sharding routing-dump --coordinator <healthy-id>
   ```
3. Compare against the corrupted coordinator's routing table.
4. Check if a recent topology change (node add/remove) coincides with the event.

### Mitigation
- Single-node corruption: force routing table resync from the leader:
  ```
  themis-admin sharding routing-resync --coordinator <corrupted-id>
  ```
- Multi-node corruption: pause ingestion, elect a new leader, and rebuild
  the routing table from the WAL:
  ```
  themis-admin sharding routing-rebuild --from-wal
  ```

### Escalation
Escalate if routing table rebuild fails, or if client-visible misrouting
has caused cross-shard data mixing (requires forensic audit).

---

## Quick Reference — Log Patterns

| Pattern                            | Scenario              |
|------------------------------------|-----------------------|
| `[SHARDING:SplitFailed]`           | Shard split failure   |
| `[SHARDING:MigrationStall]`        | Migration stall       |
| `[SHARDING:AntiEntropyDivergence]` | Anti-entropy diverge  |
| `[SHARDING:HotShard]`              | Hot-shard overload    |
| `[SHARDING:RoutingCorruption]`     | Routing table corrupt |

## Related Documents
- `src/sharding/ROADMAP.md` — module roadmap
- `docs/operability/WAVE_D_ROADMAP.md` — Wave D operability plan
- `docs/operability/WAVE_D_SIGN_OFF.md` — sign-off evidence
- `include/sharding/sharding_api_contract.h` — error taxonomy
