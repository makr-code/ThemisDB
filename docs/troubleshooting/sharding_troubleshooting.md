# Sharding Troubleshooting Guide

The `sharding` module implements distributed data partitioning for ThemisDB, including adaptive shard routing, consistent hashing, consensus protocols (Raft/Paxos/Gossip), cross-shard transactions, auto-rebalancing, and circuit breakers for fault isolation.

## Quick Diagnostics

| Symptom | Likely Cause | Quick Fix |
|---------|-------------|-----------|
| `CircuitBreaker: shard-03 OPEN` | Shard consistently failing health checks | Check shard health; reset breaker after fix |
| Uneven data distribution (hot shard) | Poor shard key choice | Change shard key or enable virtual nodes |
| Cross-shard transaction hangs | 2PC coordinator node down | Check coordinator; enable `auto_abort_timeout_ms` |
| `ConsensusFactory: no leader elected` | Split network or quorum loss | Check network partition; restart minority nodes |
| Auto-rebalancer moves data continuously | Load skew threshold too sensitive | Increase `rebalance.trigger.load_skew_threshold_pct` |
| `GossipProtocol: node-05 marked SUSPECTED` | Node slow to respond, not actually down | Increase `gossip.suspect_timeout_ms` |
| Shard migration fails midway | Source shard ran out of disk | Free disk space before migration |
| `CapabilityMatcher: no shard found for query` | Query requires capability not available | Check shard capability assignments |
| Admin API returns `503 Cluster not ready` | Cluster is in bootstrap phase | Wait for bootstrap; check `min_shards` setting |
| Data loss after shard crash | Replication factor too low | Set `replication_factor >= 3` |

## Common Issues

### Issue 1: Circuit Breaker Opens on Healthy Shard

**Description:** A shard's circuit breaker trips even though the shard is healthy.

**Symptoms:**
- Log: `CircuitBreaker: shard-03 tripped OPEN after 5 consecutive failures`
- Queries to shard-03 return `503` even though the shard node is running

**Cause:** Network jitter causes health-check timeouts; `cb_failure_threshold` is too low.

**Solution:**
```yaml
sharding:
  circuit_breaker:
    failure_threshold: 10          # increase from default 5
    success_threshold: 3
    timeout_ms: 5000               # health-check timeout
    half_open_requests: 3          # requests to test recovery
    reset_timeout_ms: 30000
```
```bash
# Manually reset a circuit breaker
themisdb-admin sharding circuit-breaker reset --shard shard-03

# Check all circuit breaker states
themisdb-admin sharding circuit-breaker status
```

---

### Issue 2: Hot Shard Due to Poor Shard Key

**Description:** One shard receives disproportionately more requests than others.

**Symptoms:**
- `themisdb-admin sharding stats` shows shard-01 handling 80% of requests
- Latency on shard-01 is 5× higher than other shards

**Cause:** Shard key has low cardinality (e.g., `status` field with 3 values) or a monotonically increasing key (timestamps) with range sharding.

**Solution:**
```bash
# Analyse shard distribution
themisdb-admin sharding distribution --collection orders

# Recommend a better shard key
themisdb-admin sharding recommend-key --collection orders --sample 100000
```
```yaml
sharding:
  collections:
    orders:
      shard_key: ["_key"]           # use composite or high-cardinality key
      number_of_shards: 16
      replication_factor: 3
      strategy: consistent_hash     # "consistent_hash" | "range"
      virtual_nodes: 150            # improve distribution
```

---

### Issue 3: Cross-Shard Transaction Hangs

**Description:** A transaction spanning multiple shards never commits or aborts.

**Symptoms:**
- Log: `CrossShardTransaction: 2PC prepare phase waiting for shard-07 (elapsed: 120s)`
- Transaction remains `PREPARED` state indefinitely

**Cause:** 2PC coordinator node crashed after sending `PREPARE` but before receiving all `ACK`s; recovery is stalled.

**Solution:**
```bash
# List stuck transactions
themisdb-admin transaction list --state PREPARED --older-than 60s

# Force-abort a stuck transaction (data-safe: no partial commits)
themisdb-admin transaction abort --txn-id <txn_id>

# Check coordinator health
themisdb-admin sharding coordinator status
```
```yaml
sharding:
  cross_shard_transaction:
    auto_abort_timeout_ms: 30000    # auto-abort after 30s in PREPARED state
    coordinator_heartbeat_ms: 5000
    recovery_enabled: true
```

---

### Issue 4: No Raft Leader Elected

**Description:** The Raft consensus group loses its leader and cannot elect a new one.

**Symptoms:**
- Log: `ConsensusFactory: raft group 'shard-04' has no leader (term=42)`
- Writes to affected shards return `503 No Leader`

**Cause:** Network partition isolated the majority; fewer than `(N/2)+1` nodes can communicate.

**Solution:**
```bash
# Check Raft group state
themisdb-admin sharding raft status --group shard-04

# Check network connectivity between nodes
themisdb-admin sharding network-check

# If a node is isolated, restart it to trigger re-election
systemctl restart themisdb-shard@shard-04
```
```yaml
sharding:
  raft:
    election_timeout_min_ms: 150
    election_timeout_max_ms: 300
    heartbeat_interval_ms: 50
    snapshot_threshold: 10000
```

---

### Issue 5: Auto-Rebalancer Causes Continuous Data Movement

**Description:** The auto-rebalancer keeps migrating data even after distribution appears even.

**Symptoms:**
- Log: `AutoRebalancer: triggered rebalance (load skew: 15%)`
- Background I/O is constantly elevated
- Rebalancer never reaches a stable state

**Cause:** `load_skew_threshold_pct` is too low (e.g., 5%); minor fluctuations trigger constant moves.

**Solution:**
```yaml
sharding:
  rebalancer:
    enabled: true
    trigger:
      load_skew_threshold_pct: 25    # increase from 5%
      min_data_size_gb: 10           # don't rebalance tiny datasets
    schedule:
      window_start: "02:00"          # rebalance only during off-peak
      window_end: "06:00"
    max_concurrent_migrations: 2
    migration_rate_limit_mbps: 100
```
```bash
# Pause rebalancer temporarily
themisdb-admin sharding rebalancer pause --duration 1h

# Check rebalancer plan
themisdb-admin sharding rebalancer plan
```

---

### Issue 6: Gossip Protocol Marks Live Node as Suspected

**Description:** A healthy node is incorrectly marked as `SUSPECTED` by the gossip protocol.

**Symptoms:**
- Log: `GossipProtocol: node-05 status=SUSPECTED (no heartbeat for 3000ms)`
- Routing avoids node-05 unnecessarily

**Cause:** The node is under heavy GC or I/O pressure, delaying gossip heartbeats beyond the suspect timeout.

**Solution:**
```yaml
sharding:
  gossip:
    heartbeat_interval_ms: 1000
    suspect_timeout_ms: 10000       # increase from 3000
    dead_timeout_ms: 30000
    fanout: 3
    port: 7946
```

---

### Issue 7: Shard Migration Fails with Disk Full

**Description:** Data migration from one shard to another fails because the target disk is full.

**Symptoms:**
- Log: `DataMigrator: migration job failed: ENOSPC on target node-08`
- Partially migrated data left on target

**Cause:** Target node does not have enough free disk space for the migrated shard.

**Solution:**
```bash
# Check free disk space on all nodes
themisdb-admin sharding nodes disk-usage

# Cancel and clean up the failed migration
themisdb-admin sharding migration cancel --job-id <job_id>
themisdb-admin sharding migration cleanup --job-id <job_id>

# Free disk space on target and retry
df -h /var/lib/themisdb/
```

---

### Issue 8: Consistent Hash Ring Imbalanced After Node Addition

**Description:** After adding a new shard node, data distribution is not updated immediately.

**Symptoms:**
- New node is in cluster but has 0 documents
- `themisdb-admin sharding distribution` shows old nodes still hold all data

**Cause:** Virtual node tokens for the new node have not been assigned; rebalancer is disabled or not triggered.

**Solution:**
```bash
# Assign virtual node tokens to new node
themisdb-admin sharding assign-tokens \
  --node shard-09 \
  --virtual-nodes 150

# Trigger immediate rebalance
themisdb-admin sharding rebalancer trigger --now
```

## Diagnostic Commands

```bash
# Cluster overview
themisdb-admin sharding status

# Shard distribution per collection
themisdb-admin sharding distribution --collection orders

# Circuit breaker status
themisdb-admin sharding circuit-breaker status

# Raft group leaders
themisdb-admin sharding raft leaders

# Pending migrations
themisdb-admin sharding migration list

# Live sharding metrics
curl -s http://localhost:9100/metrics | grep themisdb_sharding

# Gossip membership view
themisdb-admin sharding gossip members

# Tail sharding logs
journalctl -u themisdb -f | grep -E "shard|gossip|raft|rebalance|circuit"
```

## Configuration Reference

```yaml
sharding:
  enabled: true
  number_of_shards: 16
  replication_factor: 3
  strategy: consistent_hash        # "consistent_hash" | "range"
  virtual_nodes: 150
  circuit_breaker:
    failure_threshold: 10
    reset_timeout_ms: 30000
  raft:
    election_timeout_min_ms: 150
    election_timeout_max_ms: 300
    heartbeat_interval_ms: 50
  gossip:
    port: 7946
    suspect_timeout_ms: 10000
    fanout: 3
  rebalancer:
    enabled: true
    trigger:
      load_skew_threshold_pct: 25
    max_concurrent_migrations: 2
  cross_shard_transaction:
    auto_abort_timeout_ms: 30000
    recovery_enabled: true
```

**Common misconfigurations:**

| Key | Wrong | Correct |
|-----|-------|---------|
| `replication_factor` | `1` | `3` for production |
| `rebalancer.trigger.load_skew_threshold_pct` | `5` | `20–30` |
| `circuit_breaker.failure_threshold` | `3` | `10` in production |
| `gossip.suspect_timeout_ms` | `1000` | `5000–10000` |

## Known Limitations

- Cross-shard transactions using 2PC add significant latency overhead; use single-shard transactions where possible.
- Consistent hash ring rebalancing requires full data movement; there is no incremental delta-migration.
- Gossip-based consensus does not provide linearizable reads; use Raft for strong consistency requirements.
- Auto-rebalancer does not account for network topology (rack/zone awareness) by default.
- Maximum supported shards per cluster is 1024; beyond this, internal routing tables exceed memory limits.

## Related Documentation

- [Sharding Module ROADMAP](../../src/sharding/ROADMAP.md)
- [Adaptive Shard Routing](../ARCHIVED/implementation-summaries/ADAPTIVE_SHARD_ROUTING.md)
- [Distributed Transactions](../DISTRIBUTED_TRANSACTIONS.md)
- [Cross-Shard Testing](../ARCHIVED/implementation-summaries/CROSS_SHARD_TESTING.md)
- [Raft Consensus Design](../architecture/RAFT_CONSENSUS_DESIGN.md)
- [Hot Spare Management](../hot_spare_management.md)
