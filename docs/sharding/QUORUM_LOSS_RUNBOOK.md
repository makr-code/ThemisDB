# Recovering from Quorum Loss — Operator Runbook

**Status**: Phase 3 Production Ready (Delivered 2026-08-17)  
**Target Audience**: ThemisDB cluster operators, on-call engineers  
**Severity**: **CRITICAL** — Quorum loss halts all read and write operations  
**Recovery Time Objective (RTO)**: 5–15 minutes (depending on failure root cause)  
**Recovery Point Objective (RPO)**: 0 seconds (no data loss if recovery successful)

## Table of Contents

1. [Detection and Diagnosis](#detection-and-diagnosis)
2. [Root Cause Analysis](#root-cause-analysis)
3. [Recovery Procedures](#recovery-procedures)
4. [Advanced Troubleshooting](#advanced-troubleshooting)
5. [Prevention and Monitoring](#prevention-and-monitoring)

---

## Detection and Diagnosis

### Recognizing Quorum Loss

Quorum loss occurs when fewer than `n/2 + 1` shards in the cluster are reachable.
For a 5-node cluster, quorum loss occurs when **≤ 2 nodes** are available.

#### Symptoms

1. **Immediate**: All write operations fail with `ShardingErrorCode::QUORUM_LOST`
2. **Logs**: `spdlog` output contains:
   ```
   [SHARDING] QUORUM_LOST: Available shards (2) < required quorum (3)
   ```
3. **Metrics**: Prometheus `sharding_quorum_health_status` = 0
4. **API Response**: HTTP 503 or gRPC `UNAVAILABLE`; client receives:
   ```json
   {
     "error_code": "QUORUM_LOST",
     "message": "Quorum of required shards is unavailable (< n/2+1 reachable)"
   }
   ```

#### Checking Cluster Health

Run the cluster health diagnostic command:

```bash
# Via ThemisDB CLI
themisdb-cli --cluster-health

# Expected output on HEALTHY cluster:
# Cluster Status: HEALTHY
# Reachable Shards: 5/5 (quorum=3)
# Consensus Layer: RAID-Paxos (2 layers)

# Expected output on QUORUM_LOST:
# Cluster Status: QUORUM_LOST
# Reachable Shards: 2/5 (quorum=3) <-- BELOW QUORUM
# Consensus Layer: UNHEALTHY
```

---

## Root Cause Analysis

### Determine What Went Wrong

Before recovering, identify the root cause to prevent recurrence.

#### Step 1: Check Node Status

```bash
# List all cluster nodes and their status
themisdb-cli --list-nodes

# Expected output format:
# Node ID       Status      Last Heartbeat    CPU    Memory   Disk
# --------      ------      --------------    ---    ------   ----
# node-001      UP          1s ago            15%    42%      67%
# node-002      DOWN        5m ago            --     --       --  <-- This node is down
# node-003      UP          500ms ago         12%    38%      65%
# node-004      DOWN        10m ago           --     --       --  <-- Also down
# node-005      UP          600ms ago         18%    45%      70%
```

#### Step 2: Check Consensus Layer Status

```bash
# For each node, check RAID-Paxos consensus state
themisdb-cli --node=node-001 --consensus-status

# Output:
# Consensus Type: RAID-Paxos
# Storage Layer: UP (Paxos quorum: 3/3)
# Cache Layer: UP (Paxos quorum: 3/3)
# Replication Factor: 5
```

#### Step 3: Inspect System Logs

```bash
# Check OS-level logs for network/disk issues
journalctl -u themisdb -n 100 --since "10 minutes ago"

# Look for:
# - Network unreachability errors
# - Disk space exhaustion
# - OOM (Out of Memory) kills
# - Hardware failures (smartctl, dmesg)
```

### Common Root Causes

| Cause | Indicators | Recovery Path |
|-------|-----------|-----------------|
| **Network Partition** | Multiple nodes unreachable; logs show connection timeouts | [Restore Network](#restore-network-connectivity) |
| **Node Hardware Failure** | Node dead; doesn't respond to ping; logs empty | [Replace Failed Node](#replace-failed-node) |
| **Disk Full on Node** | `ENOSPC` errors in logs; disk usage 100% | [Free Disk Space](#free-disk-space) |
| **Process Crash** | Node status DOWN; error logs with stack trace | [Restart Node](#restart-failed-node) |
| **Power/Hypervisor Issue** | Node completely offline; hypervisor reports DOWN | [Restore Power/Hypervisor](#restore-powercontext-hypervisor) |

---

## Recovery Procedures

### Restore Network Connectivity

**When**: Network partition has isolated ≥ 3 nodes  
**Duration**: 2–5 minutes  
**Data Loss**: None (no data written during partition)

#### Procedure

```bash
# 1. Verify partition exists (node can ping itself but not others)
ssh node-002
ping node-001  # Expected: unreachable
ping node-003  # Expected: unreachable

# 2. Check network interface status
ip link show
ip addr show

# 3. Check for firewall/security group rules blocking traffic
# On AWS:
aws ec2 describe-security-groups --group-ids <sg-id>
# Verify inbound rule on port 7000 (sharding RPC) is open

# 4. Restore network (example: AWS security group fix)
aws ec2 authorize-security-group-ingress \
  --group-id <sg-id> \
  --protocol tcp \
  --port 7000 \
  --source-type cidr \
  --cidr 0.0.0.0/0

# 5. Verify cluster recovery (this should re-establish quorum)
themisdb-cli --cluster-health

# Expected: "Cluster Status: HEALTHY" and "Reachable Shards: 5/5"
```

**Recovery Success Check**:
```bash
# If 3+ nodes are now reachable, quorum is restored
# Operations should resume immediately
curl -X POST http://localhost:7001/api/v1/query \
  -H "Content-Type: application/json" \
  -d '{"sql": "SELECT COUNT(*) FROM test"}'
# Expected: HTTP 200 (not 503)
```

### Replace Failed Node

**When**: Hardware failure on 1 node (1 node down, 4 up = quorum OK)  
**Duration**: 10–20 minutes  
**Data Loss**: None (replicas on other nodes)  
**Precondition**: Cluster must have quorum before starting (not used for quorum loss recovery)

#### Procedure

```bash
# 1. Identify failed node
themisdb-cli --cluster-health  # Shows which node is DOWN

# 2. Mark node for replacement (removes from shard ownership)
themisdb-cli --node=node-002 --mark-for-removal

# This initiates automatic shard migration OFF the failed node
# Check migration progress:
themisdb-cli --migration-status  # Should show progress rebalancing

# 3. Wait for migration to complete (typically < 5 minutes for small shards)
# Status: "Pending: 0/X shards" indicates complete

# 4. Remove node from cluster
themisdb-cli --node=node-002 --remove

# 5. Replace hardware (or provision new VM)
# ...provision replacement node...
ssh new-node-ip

# 6. Add new node to cluster
themisdb-cli --add-node \
  --node-id=node-002-replacement \
  --endpoint=new-node-ip:7000

# 7. Rebalance shards onto new node
themisdb-cli --rebalance --policy=round-robin

# Status: Watch `--migration-status` until complete

# 8. Verify cluster health
themisdb-cli --cluster-health
# Expected: All nodes UP, quorum=3/5
```

### Free Disk Space

**When**: Node runs out of disk space (ENOSPC error)  
**Duration**: 5–10 minutes  
**Data Loss**: None (data is replicated)

#### Procedure

```bash
# 1. Identify node with full disk
ssh node-with-full-disk
df -h  # Shows which filesystem is 100%

# 2. Identify and remove temporary files
du -sh /var/lib/themisdb/wal/* | sort -h  # Show WAL segments by size
ls -lh /var/lib/themisdb/snapshot/  # Show snapshots

# 3. Delete old WAL segments (safe if quorum is OK and replicas are up-to-date)
# WARNING: Only delete WAL segments that are older than checkpoint LSN!
themisdb-cli --node=node-XXX --check-wal-retention-safe
# Returns: "Safe to delete segments: /var/lib/themisdb/wal/seg-001 through seg-050"

rm /var/lib/themisdb/wal/seg-001 through seg-050

# 4. Compact state machine (if applicable)
themisdb-cli --node=node-XXX --compact-state-machine
# This reduces in-memory and on-disk state

# 5. Verify free space
df -h  # Should now show available space

# 6. Resume operations
themisdb-cli --cluster-health  # Should show HEALTHY
```

### Restart Failed Node

**When**: Node process crashed but hardware is intact  
**Duration**: 2–3 minutes  
**Data Loss**: None (WAL replay recovers state)

#### Procedure

```bash
# 1. SSH to failed node
ssh node-002

# 2. Check if ThemisDB process is running
systemctl status themisdb

# 3. Start node if stopped
systemctl start themisdb

# 4. Watch startup logs for WAL recovery
journalctl -u themisdb -f  # Follow logs

# Expected log sequence:
# [INFO] ThemisDB starting...
# [INFO] Loading WAL from /var/lib/themisdb/wal
# [INFO] Replaying WAL entries (LSN 1000-5000)... [██████████] 100%
# [INFO] Consensus layer initialized
# [INFO] Node ready to serve traffic

# 5. Verify node is healthy
curl http://node-002:7001/health
# Expected: HTTP 200, "status": "UP"

# 6. Verify cluster health
themisdb-cli --cluster-health
# Expected: "Reachable Shards: 5/5"
```

### Restore Power/Hypervisor Context

**When**: Physical node offline (power failure, hypervisor restart)  
**Duration**: 5–10 minutes  
**Data Loss**: None (data persisted to disk)

#### Procedure

```bash
# 1. Verify physical status
# Check data center console / hypervisor dashboard:
# - Node should show as DOWN in hypervisor
# - Verify no hardware fault LEDs

# 2. Power on or boot VM
# - Physical power button (data center) or
# - AWS EC2: Instance > Instance State > Start

# 3. Wait for OS boot (typically 1–2 minutes)
# Monitor:
ping node-002  # Should start responding

# 4. Verify ThemisDB auto-starts
ssh node-002
systemctl status themisdb
# If not running: systemctl start themisdb

# 5. Watch WAL recovery
journalctl -u themisdb -n 50
# Should show WAL replay (same as [Restart Failed Node](#restart-failed-node))

# 6. Verify cluster health
themisdb-cli --cluster-health
# Quorum should restore once node fully boots
```

---

## Advanced Troubleshooting

### Scenario: Only 1 Node Up (Complete Quorum Loss)

**Situation**: 4 nodes down, 1 remaining (or worse)  
**Option 1: Wait for Node Recovery (Recommended)**
```bash
# This is a true systemic failure
# Best approach: restore failed nodes

# Monitor node recovery
watch -n 2 'themisdb-cli --cluster-health'

# Once any node comes back up:
# Cluster should resolve (if quorum becomes achievable)
```

**Option 2: Force-Recovery (DANGEROUS — Last Resort)**

⚠️ **WARNING**: This procedure can cause data loss if the 1 remaining node
has stale data. Only use after exhausting all other options.

```bash
# 1. Ensure you have taken backups of all nodes
# Backup data before proceeding:
themisdb-cli --backup --output-dir=/backup/complete_recovery_20260817

# 2. Identify which node has most recent LSN
# (Shows which node has most complete transaction log)
for node in node-001 node-002 node-003 node-004 node-005; do
  echo "Node $node:"
  ssh $node "themisdb-cli --show-wal-tail" 2>/dev/null || echo "  (unreachable)"
done

# Note: The node with HIGHEST LSN has most recent data

# 3. Force-elect the most advanced node as single leader
themisdb-cli --force-single-leader --node=node-001

# WARNING: This may abandon uncommitted transactions on failed nodes!
# Verify data integrity post-recovery:
themisdb-cli --verify-data-consistency --shard=all

# 4. Once recovered, gradually add failed nodes back
# See [Replace Failed Node](#replace-failed-node)
```

### Checking Transaction Durability During Recovery

```bash
# After partial recovery, verify no committed transactions were lost
themisdb-cli --verify-wal-integrity

# Expected:
# WAL Integrity Check
# ─────────────────
# Total Entries: 5000
# CRC Errors: 0
# Missing LSNs: 0
# Status: OK

# If errors found, triggers automatic recovery (see next section)
```

### Automatic WAL Corruption Handling

If WAL corruption is detected during recovery:

```bash
# 1. System will halt replay at first corruption point
# Error in logs:
# [ERROR] WAL_CORRUPTION: Entry at LSN 1500/256 has CRC mismatch
# [INFO] Replay halted at LSN 1500/255

# 2. Manual intervention required:
# Option A: Truncate corrupted segment and continue
themisdb-cli --truncate-wal-at-lsn=1500/255 --confirm

# Option B: Restore from backup
themisdb-cli --restore-from-backup --backup-id=backup-20260817-120000

# 3. Restart node
systemctl restart themisdb

# 4. Verify recovery
journalctl -u themisdb | grep "WAL replay"
# Should show: "Replay complete, LSN 1500/255"
```

---

## Prevention and Monitoring

### Monitoring for Quorum Issues

**Set up alerts for**:

1. **Shard Health (Target: ≥ n/2 + 1 UP)**
   ```
   Alert: sharding_quorum_health_status < 1 for > 5 minutes
   Action: Page on-call engineer immediately
   ```

2. **Individual Node Latency**
   ```
   Alert: shard_rpc_latency_p95_ms > 1000 for > 2 minutes
   Action: Investigate network; may indicate upcoming partition
   ```

3. **Disk Space**
   ```
   Alert: node_disk_available_bytes < 5GB for any node
   Action: Trigger cleanup (see [Free Disk Space](#free-disk-space))
   ```

4. **Consensus Timeouts**
   ```
   Alert: sharding_consensus_timeout_total > 10/min
   Action: Investigate; may precede partition or quorum loss
   ```

### Redundancy Best Practices

1. **Minimum Cluster Size**: Always maintain ≥ 5 nodes
   - 3 nodes = no fault tolerance (1 failure → quorum loss)
   - 5 nodes = 2-node fault tolerance ✅
   - 7 nodes = 3-node fault tolerance ✅

2. **Geographic Distribution**: Spread nodes across ≥ 2 failure domains
   - Multi-AZ in cloud (us-east-1a, 1b, 1c)
   - Multi-DC on-prem (rack 1, rack 2, UPS system)

3. **Regular Backup and Recovery Drills**
   - Monthly: Test restore from backup
   - Quarterly: Simulate single-node failure; verify recovery

### Runbook Updates

This runbook is versioned with Phase 3 (2026-08-17). When cluster topology
or recovery procedures change:

1. Update procedures in this file
2. Add new scenarios to [Root Cause Analysis](#root-cause-analysis) table
3. Test all procedures in staging environment
4. Bump version and add CHANGELOG entry

---

## Quick Reference Card

**Print this for on-call duty**:

```
╔═══════════════════════════════════════════════════════════════╗
║                  QUORUM LOSS QUICK CHECKLIST                  ║
╚═══════════════════════════════════════════════════════════════╝

1. DETECT:
   □ Run: themisdb-cli --cluster-health
   □ Check: "Reachable Shards" < "quorum"
   □ Logs: grep "QUORUM_LOST" /var/log/themisdb/*.log

2. ROOT CAUSE:
   □ Network partition?   → Restore network
   □ Node crash?          → Restart node
   □ Disk full?           → Free space
   □ Hardware failure?    → Replace node
   □ Power loss?          → Power on

3. RECOVER:
   □ Follow matching procedure above
   □ Verify: themisdb-cli --cluster-health (all nodes UP)
   □ Test: POST /api/query should return 200 OK

4. ALERT:
   □ Notify: #incident-response Slack channel
   □ Create: Post-mortem 24 hours later
   □ Review: Prevention measures (see section 5)

EMERGENCY CONTACT:
  On-Call Lead: @oncall-sharding-lead
  Slack: #themisdb-incidents
  Escalation: CTO on-call (see wiki)
```

---

## Additional Resources

- **ROADMAP**: `src/sharding/ROADMAP.md` — Phase 3 design rationale
- **Error Taxonomy**: `include/sharding/sharding_api_contract.h` § 5
- **Recovery Strategies**: `include/sharding/sharding_error_recovery.h`
- **Test Suite**: `tests/sharding/test_sharding_phase3_edgecases.cpp`

---

**Document Version**: Phase 3 (2026-08-17)  
**Last Updated**: 2026-08-17  
**Maintained By**: ThemisDB Sharding Team  
**Review Frequency**: Quarterly or after production incident
