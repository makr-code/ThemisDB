# Replication Troubleshooting Guide

The `replication` module manages data replication across ThemisDB nodes and shards, ensuring high availability, consistency, and durability through Raft-based leader election, WAL streaming, and conflict resolution.

## Quick Diagnostics

| Symptom | Likely Cause | Quick Fix |
|---------|-------------|-----------|
| High replication lag (> 10s) | Network congestion or slow follower | Check follower I/O; tune `replication.batch_size` |
| `ReplicationManager: election timeout` | Leader unreachable from quorum | Check network; restart isolated node |
| WAL checksum mismatch | Disk bit-rot or network corruption | Enable `replication.wal.verify_checksums` |
| Split-brain after partition | Minority formed its own leader | Check Raft quorum; `split_brain_protection: true` |
| Multi-master conflict flood | Write rate exceeds conflict resolver | Enable CRDT for commutative operations |
| Hot-spare promotion fails | Hot spare not fully caught up | Check `hot_spare.catchup_lag_threshold_ms` |
| Follower stuck in `CATCHING_UP` | Log segment gap | Run `themisdb-admin replication resync` |
| Replication CPU overhead high | Compression too expensive | Change `replication.compression` to `lz4` |

## Common Issues

### Issue 1: Replication Lag Exceeds SLA

**Description:** Follower nodes are significantly behind the leader.

**Symptoms:**
- Metric `themisdb_replication_lag_ms > 10000`
- Reads from follower return stale data

**Cause:** Follower I/O is too slow; batch size is too small.

**Solution:**
```yaml
replication:
  batch_size: 1000               # increase from 100
  batch_timeout_ms: 50
  compression: lz4               # faster than zstd for replication
  flow_control:
    enabled: true
    max_in_flight_bytes: 104857600  # 100MB
```
```bash
# Check replication lag per node
themisdb-admin replication lag

# Inspect follower I/O
themisdb-admin replication follower-stats --node replica-02
```

---

### Issue 2: Raft Election Storms

**Description:** Leader elections occur too frequently, causing brief write unavailability.

**Symptoms:**
- Log: `ReplicationManager: new election started (term=142)` repeating every few seconds
- Brief write errors during elections

**Cause:** Election timeout too short; follower falsely times out leader.

**Solution:**
```yaml
replication:
  raft:
    heartbeat_interval_ms: 100
    election_timeout_min_ms: 500    # increase from 150
    election_timeout_max_ms: 1000   # increase from 300
    max_append_entries_size: 65536
```

---

### Issue 3: WAL Checksum Mismatch

**Description:** Replicated WAL entries fail checksum validation on followers.

**Symptoms:**
- Log: `ReplicationManager: WAL checksum mismatch for entry seq=98765 on replica-03`
- Replica falls out of sync

**Cause:** Network packet corruption or disk read error on leader.

**Solution:**
```yaml
replication:
  wal:
    verify_checksums: true
    checksum_algorithm: crc32c     # "crc32c" | "sha256"
    retry_on_checksum_error: true
    retry_max: 3
```
```bash
# Force resync of a corrupt replica
themisdb-admin replication resync \
  --node replica-03 \
  --from-snapshot latest
```

---

### Issue 4: Split-Brain After Network Partition

**Description:** After a network partition, two nodes both believe they are the leader.

**Symptoms:**
- Two nodes write independently; data diverges
- Log: `ReplicationManager: SPLIT-BRAIN DETECTED – stepping down`

**Cause:** Minority partition elected a new leader before the old leader stepped down.

**Solution:**
```yaml
replication:
  raft:
    split_brain_protection: true    # leader steps down if it loses quorum contact
    quorum_check_interval_ms: 1000
    leader_lease_duration_ms: 5000
```
```bash
# After healing partition, merge diverged data
themisdb-admin replication merge-diverged \
  --primary node-01 \
  --secondary node-02 \
  --strategy last_write_wins        # or "manual"
```

---

### Issue 5: Hot-Spare Promotion Failure

**Description:** When the primary node fails, the hot spare cannot be promoted.

**Symptoms:**
- Log: `HotSpare: cannot promote – lag=45000ms exceeds threshold=5000ms`
- Manual failover required

**Cause:** Hot spare is not caught up enough to serve as primary.

**Solution:**
```yaml
replication:
  hot_spare:
    enabled: true
    catchup_lag_threshold_ms: 10000   # relax promotion threshold
    promote_on_leader_loss: true
    catchup_timeout_ms: 120000
    verify_data_integrity: true
```
```bash
# Check hot spare readiness
themisdb-admin replication hot-spare status

# Force promote despite lag (emergency)
themisdb-admin replication hot-spare promote --force
```

---

### Issue 6: Follower Stuck in CATCHING_UP State

**Description:** A follower cannot catch up with the leader after a long outage.

**Symptoms:**
- Log: `ReplicationManager: replica-04 stuck in CATCHING_UP (elapsed: 30m)`
- Follower never transitions to `FOLLOWER` state

**Cause:** Log segment gap; follower needs entries that the leader has already compacted.

**Solution:**
```bash
# Trigger full resync from snapshot
themisdb-admin replication resync \
  --node replica-04 \
  --from-snapshot latest \
  --force
```
```yaml
replication:
  raft:
    log_compaction_threshold: 100000   # keep more log entries before snapshot
    install_snapshot_timeout_ms: 600000
```

---

### Issue 7: Multi-Master Conflict Flood

**Description:** Conflict resolver is overwhelmed with write conflicts in multi-master setup.

**Symptoms:**
- Log: `ReplicationManager: 10000 conflicts/s – conflict resolver queue full`
- Write latency degrades significantly

**Cause:** Application writes to the same documents from multiple masters simultaneously.

**Solution:**
```yaml
replication:
  multi_master:
    conflict_resolution: lww         # "lww" (last-write-wins) | "crdt" | "manual"
    crdt_types:
      - counter
      - set
    conflict_queue_size: 100000
    batch_conflict_resolution: true
```

---

### Issue 8: High CPU from Replication Compression

**Description:** Replication compresses every WAL batch, consuming significant CPU.

**Symptoms:**
- `top` shows `themisdb` at 80% CPU even with low query load
- Log: `ReplicationManager: compression ratio=1.1 (not worth it)`

**Cause:** `zstd` compression is too expensive for already-compressed data.

**Solution:**
```yaml
replication:
  compression: lz4                  # switch from zstd to lz4
  compress_threshold_bytes: 16384   # skip compression for small batches
  compress_level: 1                 # lower compression level
```

## Diagnostic Commands

```bash
# Replication status and lag
themisdb-admin replication status

# Per-node lag
themisdb-admin replication lag

# Raft leader info
themisdb-admin replication raft-info

# Hot spare readiness
themisdb-admin replication hot-spare status

# Conflict statistics
themisdb-admin replication conflicts --last 1h

# Live metrics
curl -s http://localhost:9100/metrics | grep themisdb_replication

# Tail replication logs
journalctl -u themisdb -f | grep -E "replication|raft|wal|hot.spare|lag"
```

## Configuration Reference

```yaml
replication:
  enabled: true
  replication_factor: 3
  consistency_level: quorum        # "one" | "quorum" | "all"
  batch_size: 500
  compression: lz4
  raft:
    heartbeat_interval_ms: 100
    election_timeout_min_ms: 500
    election_timeout_max_ms: 1000
    split_brain_protection: true
  hot_spare:
    enabled: false
    catchup_lag_threshold_ms: 5000
    promote_on_leader_loss: true
  wal:
    verify_checksums: true
    checksum_algorithm: crc32c
```

**Common misconfigurations:**

| Key | Wrong | Correct |
|-----|-------|---------|
| `replication_factor` | `1` | `3` for production |
| `raft.election_timeout_min_ms` | `50` | `500` |
| `wal.verify_checksums` | `false` | `true` |
| `consistency_level` | `one` | `quorum` |

## Known Limitations

- Replication does not support geo-distributed deployments with WAN latency > 200ms without tuning.
- Multi-master conflict resolution with `manual` strategy requires custom application logic.
- Log-based replication cannot replicate schema changes atomically with data changes.
- Hot spare promotion requires at least one full snapshot to be available on the spare node.

## Related Documentation

- [Replication Implementation Status](../REPLICATION_IMPLEMENTATION_STATUS.md)
- [HA Replication Guide](../replication/replication-ha-guide.md)
- [Raft Consensus Design](../architecture/RAFT_CONSENSUS_DESIGN.md)
- [Hot Spare Management](../hot_spare_management.md)
- [WAL & gRPC mTLS Configuration](../architecture/WAL_GRPC_MTLS_CONFIGURATION.md)
