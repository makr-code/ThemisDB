# RUNBOOK: Sharding Topology Change & Rebalancing

**Audience:** Database Operators, SREs, Sharding Team Lead  
**Purpose:** Execute sharding topology changes and rebalance operations safely  
**Severity:** High (affects read/write distribution and cluster balance)  
**Estimated Duration:** 30 min - 4 hours (varies with shard count and data size)  

---

## Overview

This runbook guides operators through planning, executing, and validating sharding topology changes (adding/removing shards) and rebalancing operations. It covers prerequisites, orchestrated rebalance procedures, progress monitoring, and rollback if issues occur.

**Key Principles:**
- Topology changes require quorum agreement and consistency validation
- Rebalance must maintain exact-path guarantees and write consistency
- Progress must be monitored with automatic stall detection
- All topology changes must be validated for data correctness post-completion

---

## Prerequisites Checklist (Before Starting)

- [ ] Target topology changes documented (shards to add/remove or rebalance strategy)
- [ ] Cluster quorum verified (all current shards healthy and connected)
- [ ] Backup of current topology and shard assignments taken
- [ ] Monitoring dashboard configured for rebalance progress metrics
- [ ] Estimated rebalance duration calculated and approved
- [ ] Maintenance window communicated to users (if needed)
- [ ] Rollback procedure tested in staging cluster
- [ ] Operator available for full duration of topology change

---

## Pre-Execution Validation (10-15 min)

### 1. Verify Cluster Health

```bash
# Check all shards are connected and healthy
verify-cluster-health --cluster <cluster-id>

# Output should show:
# Shard Status:
#   shard-001: HEALTHY, in_sync=true, lag_ms=0
#   shard-002: HEALTHY, in_sync=true, lag_ms=5
#   ...
# Quorum: ESTABLISHED (majority connected)
# Data Consistency: VERIFIED
```

Proceed only if **all shards HEALTHY and QUORUM ESTABLISHED**.

### 2. Validate Current Shard Assignments

```bash
# List current topology
list-topology --cluster <cluster-id>

# Verify shard-to-key distribution
verify-key-distribution --cluster <cluster-id>

# Output should show:
# Shard Ranges:
#   shard-001: key_range [0x0000, 0x3FFF]
#   shard-002: key_range [0x4000, 0x7FFF]
#   shard-003: key_range [0x8000, 0xBFFF]
#   shard-004: key_range [0xC000, 0xFFFF]
# Total Keys in Cluster: N
# Unassigned Keys: 0 (must be zero!)
```

### 3. Baseline Metrics

```bash
# Record baseline performance
query-metrics \
  --metric shard_write_latency_p99,shard_read_latency_p99 \
  --window 5m \
  --output baseline_metrics.json

# Record shard load distribution
query-metrics \
  --metric shard_load_ratio \
  --window 5m \
  --output baseline_load.json
```

---

## Step-by-Step Topology Change Execution

### Step 1: Plan & Validate New Topology (10 min)

**Objective:** Define the target state and validate it doesn't violate consistency constraints.

1. **Define target topology:**
   ```bash
   # Option A: Add new shard
   cat > new_topology.json << EOF
   {
     "operation": "add_shard",
     "new_shard": {
       "id": "shard-005",
       "host": "db.shard5.internal",
       "port": 5432,
       "region": "us-east-1"
     },
     "rebalance_strategy": "gradual"  # or "immediate"
   }
   EOF
   
   # Option B: Remove shard
   cat > new_topology.json << EOF
   {
     "operation": "remove_shard",
     "target_shard": "shard-004",
     "rebalance_strategy": "gradual"
   }
   EOF
   
   # Option C: Rebalance for load balancing
   cat > new_topology.json << EOF
   {
     "operation": "rebalance",
     "target_distribution": "equal_load",  # or "equal_keys"
     "rebalance_strategy": "gradual"
   }
   EOF
   ```

2. **Validate new topology:**
   ```bash
   validate-topology --topology-plan new_topology.json

   # Output should show:
   # Validation Results:
   #   - Key range coverage: COMPLETE (all keys assigned)
   #   - Consistency constraints: SATISFIED (quorum intact)
   #   - Shard capacity: SUFFICIENT
   #   - Estimated data movement: 15.2 GB
   #   - Estimated duration: 45 minutes
   #   - Rollback risk: LOW
   ```

3. **Decision Point:**
   - ✅ **All validations pass:** Proceed to Step 2
   - ⚠ **Minor warnings:** Review with sharding lead, may proceed
   - ❌ **Validation failures:** Fix topology, re-validate before proceeding

### Step 2: Execute Topology Change (Orchestrated)

**Objective:** Perform the topology change with automatic progress tracking and stall detection.

```bash
# Execute topology change with monitoring
execute-topology-change \
  --cluster <cluster-id> \
  --topology-plan new_topology.json \
  --strategy orchestrated \
  --batch-size 2 \  # Move 2 key ranges in parallel
  --max-stall-duration 10m \
  --auto-rollback-on-stall true
```

**What happens during execution:**
1. **Phase 1: Validation (2-3 min)**
   - Verify all shards ready to participate
   - Allocate new key ranges (if adding shard)
   - Validate exact-path routing rules

2. **Phase 2: Data Movement (bulk of time)**
   - Move key ranges from source → destination shard
   - Verify data integrity during transfer
   - Build indexes on destination shard in parallel
   - Monitor:
     ```bash
     watch-topology-progress --cluster <cluster-id>
     
     # Output: [##########......] 65% complete
     # Moved: 15.2 GB / 23.5 GB
     # Duration: 25 min / 45 min (est.)
     # Active key ranges: 2
     # Last progress: 30 seconds ago
     ```

3. **Phase 3: Cutover (5-10 min)**
   - Switch routing to new topology
   - Verify all clients see new topology
   - Drain in-flight operations from old routes

### Step 3: Monitor for Stalls (Continuous during execution)

**Objective:** Detect and resolve rebalance stalls immediately.

```bash
# Monitor for stalls (automatic via orchestrator, or manual)
watch-metric \
  --metric topology_change_progress_bytes \
  --interval 30s \
  --stall-threshold 300s  # No progress for 5 min = stall

# If stall detected:
# - Orchestrator will pause and investigate
# - Check logs for blocked operations
# - Possible causes:
#   - Destination shard full (increase capacity)
#   - Network bandwidth exhausted (reduce batch size)
#   - Long-running transaction blocking writes
#   - Compaction or GC on destination shard
```

**Stall Recovery (if automatic recovery doesn't work):**

```bash
# 1. Investigate stalled key range
list-stalled-key-ranges --cluster <cluster-id>

# 2. Check destination shard
check-shard-health --shard-id <destination-shard>

# 3. Increase parallelism or reduce batch size
update-topology-change \
  --batch-size 1 \  # Reduce to single range at a time
  --max-data-transfer-rate 100MB/s  # Cap bandwidth

# 4. Resume
resume-topology-change --cluster <cluster-id>
```

### Step 4: Completion Validation (10-20 min)

**Objective:** Verify the topology change completed successfully and data is consistent.

```bash
# 1. Verify new topology in effect
list-topology --cluster <cluster-id>

# 2. Validate all keys properly assigned
verify-key-distribution --cluster <cluster-id>

# 3. Check data consistency
verify-data-consistency --cluster <cluster-id>

# 4. Validate exact-path guarantees
verify-exact-path-routing --cluster <cluster-id>

# 5. Benchmark read/write performance post-change
run-benchmark --suite topology-post-change-validation \
  --duration 5m \
  --metric shard_write_latency_p99,shard_read_latency_p99

# 6. Compare to baseline
compare-metrics \
  --before baseline_metrics.json \
  --after current_metrics.json \
  --threshold latency_increase_pct:10%
```

**Success Criteria:**
- [ ] All keys assigned and verified
- [ ] Data consistency check passes
- [ ] Exact-path routing works for all key ranges
- [ ] Write latency p99 within 10% of baseline
- [ ] Read latency p99 within 10% of baseline
- [ ] No orphaned key ranges

**Decision:**
- ✅ **All validations pass:** Topology change complete! Proceed to Step 5
- ⚠ **Minor latency increase (5-10%):** Monitor for 30 min; within tolerance
- ❌ **Consistency failures or orphaned keys:** Initiate Rollback (Step 6)

### Step 5: Post-Change Monitoring (30 min - ongoing)

**Objective:** Prove the topology is stable under production load.

```bash
# 1. Monitor for 30 minutes post-change
watch-metric \
  --metric shard_write_latency_p99,shard_read_latency_p99,replication_lag_p99 \
  --window 30m \
  --interval 1m

# 2. Check load distribution
query-metrics --metric shard_load_ratio --window 30m

# 3. Watch for cascading issues
watch-error-logs --cluster <cluster-id> --window 30m

# 4. If load imbalanced, trigger rebalance
query-shard-loads --cluster <cluster-id>
# Example output:
#   shard-001: 45% load
#   shard-002: 38% load
#   shard-003: 52% load  <- imbalanced
#   shard-004: 41% load
#   shard-005: 24% load  <- underutilized (new shard)

# If imbalanced:
execute-topology-change \
  --cluster <cluster-id> \
  --operation rebalance \
  --target-distribution equal_load
```

---

## Rollback Procedure (If Issues Detected)

### When to Rollback

Rollback is **immediate** if:
- Data consistency check fails
- Orphaned key ranges detected
- Write latency p99 > 200% of baseline
- Replication lag exceeds 1000ms
- Cascading failures detected (circuit breaker trips)

### Step 6: Rollback Execution

```bash
# 1. Issue rollback command
initiate-topology-rollback \
  --cluster <cluster-id> \
  --from-topology new_topology.json \
  --to-topology baseline_topology.json

# 2. Monitor rollback progress
watch-topology-progress --cluster <cluster-id>

# 3. Validate rollback completion
verify-topology --cluster <cluster-id> --expected baseline_topology.json

# 4. Verify data consistency
verify-data-consistency --cluster <cluster-id>

# 5. Post-incident actions
# - File incident report (template below)
# - Escalate to sharding team lead for RCA
# - Block re-attempt until root cause fixed and tested in staging
```

---

## Troubleshooting Table

| Symptom | Likely Cause | Investigation | Resolution |
|---------|--------------|---|----------|
| Topology change stalls at 30% progress | Destination shard I/O bottleneck | Check destination disk latency | Increase disk speed or reduce batch size |
| "Orphaned key range" error during validation | Bug in key range migration logic | Review migration logs and key boundaries | Rollback and escalate to team lead |
| Write latency increases 3x post-change | Load imbalance (new shard underutilized) | Query shard load distribution | Run rebalance with equal_load strategy |
| Reads fail with "key not found" intermittently | Routing table not updated on all shards | Check routing table propagation | Force routing table refresh and verify |
| Replication lag spikes after topology change | Replica didn't receive key range changes | Check replica WAL shipping | Restart replica to re-sync topology |

---

## Incident Report Template

```markdown
# Topology Change Incident Report

## Change Details
- **Cluster:** [cluster-id]
- **Change Type:** [add_shard/remove_shard/rebalance]
- **Start Time:** YYYY-MM-DD HH:MM:SS UTC
- **End Time:** YYYY-MM-DD HH:MM:SS UTC
- **Rollback Time:** YYYY-MM-DD HH:MM:SS UTC (if applicable)

## Issue Description
[What went wrong]

## Impact
- **Keys Affected:** [number or percentage]
- **Write Operations Blocked:** [duration]
- **Data Loss:** [yes/no]

## Root Cause Analysis (Sharding Team)
[To be completed within 24 hours]

## Prevention
[What will prevent similar incidents]

## Sign-Off
- **Operator:** [name]
- **Sharding Lead:** [name]
- **Date:** YYYY-MM-DD
```

---

## Quick Reference

```bash
# Verify health before starting
verify-cluster-health --cluster <cluster-id>

# Plan topology change
validate-topology --topology-plan new_topology.json

# Execute (orchestrated with auto-stall detection)
execute-topology-change --cluster <cluster-id> --topology-plan new_topology.json

# Monitor progress
watch-topology-progress --cluster <cluster-id>

# Validate completion
verify-data-consistency --cluster <cluster-id>

# If issues, rollback
initiate-topology-rollback --cluster <cluster-id> --to-topology baseline_topology.json
```

---

**Runbook Version:** 1.0  
**Last Updated:** 2026-08-15  
**Owner:** Sharding Team  
**Next Review:** 2026-12-15
