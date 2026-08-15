# RUNBOOK: Replication Lag & Failover Detection and Recovery

**Audience:** Database Operators, SREs, Replication Team Lead  
**Purpose:** Detect and respond to replication lag spikes and failover events  
**Severity:** Critical (affects data freshness and high-availability)  
**Estimated Duration:** 15 min - 1 hour (diagnosis + recovery)  

---

## Overview

This runbook guides operators through diagnosing replication lag conditions, determining root causes, triggering failover when necessary, and verifying recovery. It covers lag spike detection, metric interpretation, manual override procedures, and post-incident analysis.

**Key Principles:**
- Lag detection must distinguish between acceptable network delay and true replication stall
- Failover decision must consider write-consistency guarantees and data loss tolerance
- Recovery verification must include lag trending and replica catch-up rate
- All failover events require root cause analysis within 24 hours

---

## Prerequisites Checklist

- [ ] Operator familiar with replication lag metric interpretation
- [ ] Monitoring dashboard for `replication_lag_p99` and `replica_catch_up_rate` accessible
- [ ] Failover command-line tool installed and tested
- [ ] Incident response team available for critical failovers
- [ ] Write consistency policy documented (RPO/RTO targets)
- [ ] Replica catch-up procedures tested in staging environment
- [ ] Network diagnostics tools available (traceroute, network latency, bandwidth)

---

## Step-by-Step Procedure

### 1. Lag Detection and Metric Interpretation (5-10 min)

**Objective:** Determine if observed lag is a true replication stall or expected network behavior.

1. **Query current lag metrics:**
   ```bash
   query-metrics \
     --metric replication_lag_p99 \
     --window 1h \
     --group-by replica_id,region
   ```
   Output example:
   ```
   replica-us-west-2-001  us-west    45ms    [ACCEPTABLE]
   replica-eu-north-1-001 eu-north  120ms   [WARNING]
   replica-ap-south-1-001 ap-south  850ms   [CRITICAL]
   ```

2. **Compare to baseline:**
   - **Normal:** Lag < 100ms (same region), < 300ms (cross-region)
   - **Warning:** Lag 100-500ms (same region), 300-800ms (cross-region)
   - **Critical:** Lag > 500ms (same region), > 800ms (cross-region)

3. **Check for trending:**
   ```bash
   query-metrics \
     --metric replication_lag_p99 \
     --window 15m \
     --interval 1m \
     --replica-id <replica-with-lag>
   ```
   - ✅ **Stable lag:** Continues at same level → likely network (proceed with diagnosis)
   - ⚠ **Increasing lag:** Growing over time → possible stall (escalate to Phase 2)
   - ❌ **Diverging lag:** Replica stopped catching up → immediate failover consideration

4. **Decision Point:**
   - ✅ **Lag stable and < warning threshold:** Normal operation, no action
   - ⚠ **Lag in warning zone but stable:** Monitor for 5 min, proceed to Step 2
   - ❌ **Lag increasing or > critical threshold:** Go to Step 2 immediately

### 2. Root Cause Diagnosis (10-20 min)

**Objective:** Identify the cause of lag spike (network, disk I/O, CPU, WAL backlog, etc.).

| Suspected Cause | Diagnostic Command | Interpretation |
|---|---|---|
| **Network Latency** | `network-diagnostics --source leader --dest <replica>` | Baseline latency + jitter |
| **WAL Backlog** | `query-metrics --metric wal_shipping_queue_depth` | Depth > 10MB → backlog present |
| **Replica CPU** | `query-metrics --metric replica_cpu_usage` | CPU > 80% → resource contention |
| **Replica Disk I/O** | `query-metrics --metric replica_disk_write_latency_p99` | Latency > 50ms → disk stall |
| **Network Bandwidth** | `network-diagnostics --bandwidth --source leader --dest <replica>` | Throughput < 100Mbps → congestion |

**Root Cause Decision Tree:**

```
Is lag > 500ms (same-region) or > 800ms (cross-region)?
│
├─ YES, and increasing rapidly
│  │
│  └─ Check WAL queue depth
│     ├─ If depth > 10MB (and growing): WAL BACKLOG → Escalate to WAL team
│     ├─ If depth normal: Check replica CPU
│     │   └─ If CPU > 90%: RESOURCE STALL → Kill heavy processes or scale replica
│     │   └─ If CPU normal: Check disk I/O
│     │       └─ If disk latency p99 > 100ms: DISK STALL → Check storage health
│     │       └─ If disk normal: Check network bandwidth
│     │           └─ If bandwidth < 50Mbps: NETWORK CONGESTION → Call network team
│     │           └─ If bandwidth normal: UNKNOWN → Collect traces and escalate
│     └─ Otherwise: UNKNOWN → Go to Step 3 (Extended Diagnosis)
│
└─ NO, lag stable in warning zone
   └─ Acceptable for cross-region; monitor for 30 min, collect metrics
```

### 3. Extended Diagnosis (if root cause unclear, 10-15 min)

**Objective:** Collect detailed traces and logs for engineering investigation.

```bash
# Collect WAL shipping logs
collect-logs \
  --component wal_shipper \
  --replica-id <replica-with-lag> \
  --duration 10m \
  --output ./wal_shipper_logs.json

# Collect replica processing logs
collect-logs \
  --component replica_apply_engine \
  --replica-id <replica-with-lag> \
  --duration 10m \
  --output ./replica_apply_logs.json

# Collect network trace
collect-network-trace \
  --source leader \
  --dest <replica-with-lag> \
  --duration 30s \
  --output ./network_trace.pcap
```

**Decision Point:**
- Root cause identified → Proceed with targeted recovery (Step 4)
- Root cause unclear → Escalate to replication engineering team with collected logs

### 4. Recovery Actions (15-45 min, depending on root cause)

**Choose recovery path based on root cause:**

#### 4A: WAL Backlog Recovery
**Objective:** Clear WAL queue by increasing replica apply rate.

```bash
# 1. Check current apply rate
query-metrics --metric wal_apply_rate_events_per_sec --replica-id <replica>

# 2. Increase batch size for bulk application
update-replica-config \
  --replica-id <replica> \
  --wal-apply-batch-size 5000  # Increase from default 1000

# 3. Monitor recovery
watch-metric --metric replication_lag_p99 --replica-id <replica> --interval 10s

# 4. Once lag recovers, revert batch size
update-replica-config \
  --replica-id <replica> \
  --wal-apply-batch-size 1000  # Reset to normal
```

**Success Criteria:** Lag returns to < 100ms (same-region) or < 300ms (cross-region) within 15 min

#### 4B: Replica Resource Stall (CPU/Memory)
**Objective:** Free resources for replication apply.

```bash
# 1. Identify heavy processes
query-top-processes --replica-id <replica> --limit 5

# 2. Gracefully terminate non-critical workloads
kill-workload --replica-id <replica> --workload <workload-name> --graceful

# 3. Or scale replica horizontally (CPU/memory increase)
scale-replica --replica-id <replica> --cpu 4 --memory 16G

# 4. Monitor recovery
watch-metric --metric replica_cpu_usage,replication_lag_p99 --interval 30s

# 5. Once stable, evaluate permanent scaling
```

**Success Criteria:** CPU drops below 70%, lag recovers within 10 min

#### 4C: Disk I/O Stall
**Objective:** Diagnose and resolve storage layer issues.

```bash
# 1. Check disk health
health-check-disk --replica-id <replica>

# 2. Check for disk errors in system logs
collect-logs --replica-id <replica> --component disk --duration 5m

# 3. If failing disk: Trigger node failover (see 4D)
# 4. If disk OK but slow: Check for background jobs (compaction, GC)
query-background-jobs --replica-id <replica>

# 5. If compaction in progress: Increase I/O priority
update-compaction-priority --replica-id <replica> --priority high

# 6. Monitor recovery
watch-metric --metric replica_disk_write_latency_p99 --interval 30s
```

**Success Criteria:** Disk write latency p99 < 50ms, lag recovers within 15 min

#### 4D: Network Congestion
**Objective:** Coordinate with network team or switch to fallback path.

```bash
# 1. Contact network operations team
# 2. Document: source, destination, time of issue, baseline vs. current latency

# 3. If available, switch to alternate network path
update-replication-route \
  --replica-id <replica> \
  --use-alternate-path true

# 4. Monitor recovery
watch-metric --metric replication_lag_p99,network_rtt_ms --interval 30s

# 5. Once stable, notify network team for root cause analysis
```

**Success Criteria:** Network latency normalized, lag recovers within 10 min

#### 4E: Stalled Replica (No Recovery via Actions A-D)
**Objective:** If lag continues to increase after 30 min of recovery attempts, consider failover.

Go to **Step 5: Failover Decision**.

### 5. Failover Decision (5-15 min)

**Objective:** Determine if failover to healthy replica is required.

**Failover is REQUIRED if:**
- Lag > 1000ms for > 15 minutes with no recovery trend
- Replica is down/unhealthy and cannot be recovered
- Data loss tolerance (RPO) exceeded

**Failover is OPTIONAL if:**
- Lag recovering but slowly (> 60 min recovery estimate)
- Writes can tolerate increased latency temporarily
- Replica will recover within RPO window

**Pre-Failover Validation:**
```bash
# 1. Verify at least one healthy replica exists
list-replicas --status healthy

# 2. Check data consistency
verify-data-consistency \
  --leader <leader> \
  --healthy-replicas <comma-separated-list>

# 3. Check write quorum
verify-quorum --cluster <cluster-id>

# 4. Confirm RPO/RTO policy allows failover
query-policy --cluster <cluster-id> --policy data_loss_tolerance
```

**Decision:**
- ✅ **Healthy replica available, consistency verified:** Proceed to Step 6 (Failover Execution)
- ⚠ **Marginal quorum or consistency issues:** Contact replication lead before failover
- ❌ **No healthy replica, quorum lost:** Critical incident — escalate to incident commander

### 6. Failover Execution (10-20 min)

**Objective:** Perform controlled failover to healthy replica.

```bash
# 1. Notify all stakeholders (teams, customers if applicable)
notify-stakeholders --event failover --cluster <cluster-id>

# 2. Trigger failover with safety checks
initiate-failover \
  --cluster <cluster-id> \
  --from-replica <stalled-replica> \
  --to-replica <healthy-replica> \
  --verify-quorum true \
  --verify-consistency true

# 3. Monitor failover progress
watch-failover-progress --cluster <cluster-id> --interval 5s

# 4. Once complete, verify new topology
verify-topology --cluster <cluster-id>

# 5. Validate connectivity from clients
test-client-connectivity --cluster <cluster-id>
```

**Success Indicators:**
- [ ] Failover completes in < 20 min
- [ ] New leader establishes quorum
- [ ] All healthy replicas connected to new leader
- [ ] Replication lag returns to < 100ms (same-region)
- [ ] Client applications reconnect successfully

### 7. Post-Failover Recovery & Cleanup (15-30 min)

**Objective:** Restore old replica to cluster after issue is resolved.

```bash
# 1. Wait 5 minutes for new topology to stabilize
wait 300s

# 2. Diagnose and fix the original replica issue
# (Based on root cause from Step 2)

# 3. Once fixed, rejoin to cluster as new replica
rejoin-replica \
  --replica-id <formerly-stalled-replica> \
  --cluster <cluster-id> \
  --catch-up-from <new-leader>

# 4. Monitor catch-up progress
watch-metric \
  --metric replica_catch_up_rate,replication_lag_p99 \
  --replica-id <formerly-stalled-replica> \
  --interval 10s

# 5. Once caught up, return to normal operations
complete-recovery --replica-id <formerly-stalled-replica>
```

---

## Troubleshooting Table

| Symptom | Likely Cause | Investigation | Resolution |
|---------|--------------|---|----------|
| Lag spikes but recovers within 5 min | Normal network jitter | Network baseline comparison | Monitor; no action needed |
| Lag steadily increasing over hours | WAL backlog or replica stall | Check WAL depth, replica CPU/disk | Clear WAL backlog or scale replica |
| Lag at critical level but replica appears healthy | Network congestion between leader-replica | Run network diagnostics | Switch alternate network path |
| Multiple replicas show lag simultaneously | Leader processing bottleneck or cascading | Check leader CPU/disk, WAL generation rate | Scale leader or reduce client load |
| Failover succeeded but new leader is also slow | Systemic issue (all replicas affected) | Check cluster-wide metrics (network, disk, CPU) | Address underlying cluster health issue |
| Replica shows lag but catch-up rate is zero | Replica crashed or network disconnected | Check replica connectivity, process logs | Reconnect or restart replica |

---

## Incident Report Template

```markdown
# Replication Lag Incident Report

## Incident Details
- **Affected Cluster:** [cluster-id]
- **Affected Replica:** [replica-id]
- **Start Time:** YYYY-MM-DD HH:MM:SS UTC
- **End Time / Recovery Time:** YYYY-MM-DD HH:MM:SS UTC
- **Duration:** [minutes]
- **Root Cause:** [from Step 2 diagnosis]

## Impact Assessment
- **Max Lag Observed:** [milliseconds]
- **Failover Triggered:** [yes/no]
- **Data Loss:** [yes/no, if any]
- **Client Connections Affected:** [number/percentage]

## Timeline
1. [HH:MM] Lag detected; operators notified
2. [HH:MM] Root cause diagnosis began
3. [HH:MM] Recovery action initiated
4. [HH:MM] Lag recovered to normal
5. [HH:MM] Incident closed

## Root Cause Analysis (Replication Team)
[To be completed within 24 hours]

## Prevention
[What will prevent similar incidents]

## Sign-Off
- **Operator:** [name]
- **Replication Lead:** [name]
- **Date:** YYYY-MM-DD
```

---

## Quick Reference

```bash
# Check lag
query-metrics --metric replication_lag_p99 --window 1h

# Diagnose root cause
network-diagnostics --source leader --dest <replica>
query-metrics --metric wal_shipping_queue_depth

# Recovery (WAL backlog)
update-replica-config --replica-id <replica> --wal-apply-batch-size 5000

# Failover
initiate-failover --cluster <cluster> --from-replica <stalled> --to-replica <healthy>

# Monitor recovery
watch-metric --metric replication_lag_p99 --interval 10s
```

---

**Runbook Version:** 1.0  
**Last Updated:** 2026-08-15  
**Owner:** Replication Team  
**Next Review:** 2026-12-15
