# Updates Module Operator Runbook (Q4 2026)

## Overview

This runbook provides structured guidance for detecting, analyzing, and recovering from the 8 critical failure scenarios identified during Phase 4-5 hardening of the Updates module. Each scenario includes symptom patterns, root cause analysis procedures, recovery steps, and prevention recommendations.

---

## Scenario 1: Coordinator Unreachable

### Symptoms
- Operations fail with `COORDINATION_TIMEOUT` error after 30+ seconds
- Coordinator service not responding to health checks
- Network connectivity failures between update nodes and coordinator
- Error messages: `COORDINATION_PEER_FAILED`, `COORDINATION_QUORUM_LOST`

### Root Cause Analysis

1. **Check coordinator service status:**
   ```bash
   systemctl status themis-coordinator
   ```

2. **Verify network connectivity:**
   ```bash
   netstat -an | grep 5432
   ping <coordinator-ip>
   ```

3. **Check coordinator logs:**
   ```bash
   journalctl -u themis-coordinator -n 100 --no-pager
   ```

4. **Verify DNS resolution (if hostname-based):**
   ```bash
   nslookup <coordinator-hostname>
   ```

### Recovery Procedure

1. **Restart coordinator service (30-60 second downtime)**
   - Command: `systemctl restart themis-coordinator`
   - Wait for readiness probe: `themis_cli health check --target=coordinator`
   - Expected: Service transitions from "down" → "starting" → "healthy"

2. **Verify quorum recovery**
   - List coordinator quorum status: `themis_cli coordinator status`
   - Confirm all quorum members reporting healthy
   - Timeout: 2 minutes

3. **Reconnect all update nodes**
   - Command: `themis_cli updates reconnect-coordinator --all`
   - Verify reconnection: `themis_cli updates list-nodes --status=connected`
   - Expected: 100% of nodes connected

4. **Retry failed operations**
   - Query retry queue: `themis_cli updates list-pending --status=retry_pending`
   - Trigger auto-retry: `themis_cli updates trigger-retry-queue`
   - Monitor: `watch 'themis_cli updates list-pending | grep retry_pending | wc -l'`

### Prevention Tips

- **Monitoring:** Configure alerting on `updates_coordination_timeout_total` (threshold: > 3 in 5 minutes)
- **Redundancy:** Deploy coordinator as HA cluster (min 3 nodes) with automatic failover
- **Network:** Implement dedicated network links for coordinator communication
- **Runbook:** Train operators to restart coordinator within 5 minutes of first timeout

---

## Scenario 2: Partial Migration Failure

### Symptoms
- Operations containing "migrate" fail at specific phases (e.g., "migrate_data", "migrate_indexes")
- Some shards succeed while others fail with `MIGRATION_PHASE_ERROR`
- Partial state inconsistency: some nodes see new state, others see old
- Error messages: `MIGRATION_VALIDATION_FAILED`, `STATE_MISMATCH`

### Root Cause Analysis

1. **Identify failed migration operations:**
   ```bash
   themis_cli updates list-operations --status=failed --filter="*migrate*"
   ```

2. **Check shard-level migration state:**
   ```bash
   themis_cli updates get-migration-state --operation-id=<op_id>
   ```

3. **Review detailed phase logs:**
   ```bash
   journalctl -u themis-updater --grep="migrate_phase" -n 200 --no-pager
   ```

4. **Verify state consistency across cluster:**
   ```bash
   themis_cli consistency check --operation-id=<op_id> --detailed
   ```

### Recovery Procedure

1. **Rollback to pre-migration state (2-5 minutes)**
   - Command: `themis_cli updates rollback --operation-id=<op_id> --mode=full`
   - Verify rollback completion: `themis_cli updates get-operation-status --operation-id=<op_id>`
   - Expected: status transitions to "rolled_back", all shards matching pre-migration state

2. **Fix root cause before retry**
   - Review logs for specific failure mode
   - Address: resource shortage, data corruption, clock skew, etc.
   - Example: `df -h` (disk space), `dmesg | tail -20` (kernel errors)

3. **Retry migration with reduced parallelism**
   - Command: `themis_cli updates retry-operation --operation-id=<op_id> --parallelism=2`
   - Monitor: `themis_cli updates get-operation-progress --operation-id=<op_id> --watch`
   - Timeout: 30 minutes (or custom based on data size)

4. **Verify post-migration state**
   - Validate indexes: `themis_cli indexes validate --all`
   - Check data integrity: `themis_cli data integrity-check --operation-id=<op_id>`
   - Expected: 100% pass rate

### Prevention Tips

- **Capacity Planning:** Ensure ≥25% free disk space before large migrations
- **Parallelism Tuning:** Start with parallelism=1 for large datasets, scale up after verification
- **Monitoring:** Alert on migration phase duration exceeding 10× baseline
- **Testing:** Run migration tests in staging environment with prod-like data volumes

---

## Scenario 3: Canary Timeout Cycles

### Symptoms
- Canary deployment phases repeatedly timeout after 30-120 seconds
- Operations stuck in "canary_validating" state
- Canary validation logs show repeated timeout errors
- Error messages: `CANARY_VALIDATION_TIMEOUT`, `CANARY_HEALTH_CHECK_FAILED`

### Root Cause Analysis

1. **Identify stuck canary operations:**
   ```bash
   themis_cli updates list-operations --filter="*canary*" --status=in_progress
   ```

2. **Check canary node health:**
   ```bash
   themis_cli node health --node-ids=<canary_nodes> --detailed
   ```

3. **Review validation timeout configuration:**
   ```bash
   themis_cli config get-setting --key=canary_validation_timeout_ms
   themis_cli config get-setting --key=canary_health_check_interval_ms
   ```

4. **Monitor CPU/memory during canary:**
   ```bash
   ssh <canary_node> 'top -b -n 1 | head -20'
   ssh <canary_node> 'free -h'
   ```

### Recovery Procedure

1. **Increase canary timeout temporarily (5-10 minutes)**
   - Command: `themis_cli config set-setting --key=canary_validation_timeout_ms --value=300000` (300s)
   - Verify setting: `themis_cli config get-setting --key=canary_validation_timeout_ms`

2. **Restart canary validation loop**
   - Command: `themis_cli updates cancel-operation --operation-id=<op_id> --skip-rollback`
   - Retry with increased timeout: `themis_cli updates retry-operation --operation-id=<op_id>`
   - Monitor: `themis_cli updates get-operation-progress --watch`

3. **Scale down canary nodes if resource-constrained**
   - Check resource utilization: `themis_cli cluster resource-utilization --nodes=<canary>`
   - If > 80% CPU/memory: `themis_cli cluster scale --nodes=<canary> --add-replicas=2`
   - Expected: Reduce per-node load by 33-50%

4. **Return timeout to normal after validation succeeds**
   - Once canary validates: `themis_cli config set-setting --key=canary_validation_timeout_ms --value=120000` (120s default)

### Prevention Tips

- **Monitoring:** Alert on canary timeout cycles (2+ timeouts in 10 minutes)
- **Capacity:** Reserve 30% headroom on canary nodes during validation
- **Tuning:** Monitor validation duration trend; adjust timeout = p95_duration + 30s
- **Testing:** Perform canary timing tests under various load scenarios

---

## Scenario 4: Blue-Green Rollback Failure

### Symptoms
- Blue-green deployment fails during rollback phase
- Traffic not switching back to blue environment after validation failure
- Green environment remains live with potentially problematic code
- Error messages: `BLUE_GREEN_SWITCH_FAILED`, `ROLLBACK_INCOMPLETE`, `TRAFFIC_NOT_SWITCHED`
- Specific affected node known (node_id in error context)

### Root Cause Analysis

1. **Identify failed blue-green deployment:**
   ```bash
   themis_cli updates list-operations --filter="*blue_green*" --status=failed
   ```

2. **Check traffic switch state:**
   ```bash
   themis_cli routing get-traffic-split --operation-id=<op_id>
   ```

3. **Verify green environment health:**
   ```bash
   themis_cli node health --node-id=<green_node> --detailed
   ```

4. **Check rollback state on affected node:**
   ```bash
   ssh <affected_node> 'systemctl status themis-updater | grep -A 10 "Active:"'
   ssh <affected_node> 'cat /var/log/themis/updates.log | tail -50'
   ```

### Recovery Procedure

1. **Emergency traffic switch back to blue (1-2 minutes)**
   - Command: `themis_cli routing force-switch --operation-id=<op_id> --target=blue --emergency`
   - Verify switch: `themis_cli routing get-active-version`
   - Expected: 100% traffic on blue version

2. **Wait for in-flight requests to drain from green**
   - Monitor: `themis_cli routing active-connections --environment=green`
   - Timeout: 5 minutes maximum
   - Command: `sleep 300 && themis_cli routing active-connections --environment=green`

3. **Cleanup failed green environment**
   - Stop green services: `themis_cli cluster stop-services --node-id=<green_node> --service=themis-updater`
   - Clear state: `themis_cli cluster reset-deployment-state --node-id=<green_node>`

4. **Restart green for next deployment attempt**
   - Command: `themis_cli cluster start-services --node-id=<green_node> --service=themis-updater`
   - Verify healthy: `themis_cli node health --node-id=<green_node> --expected-status=healthy`

### Prevention Tips

- **Testing:** Validate rollback scenario in canary with intentional validation failure
- **Monitoring:** Alert on traffic stuck in transition state (> 2 minutes)
- **Redundancy:** Ensure blue environment remains healthy during green deployment
- **Runbook:** Train on manual traffic switch procedure for emergency scenarios

---

## Scenario 5: Resource Exhaustion Under High Update Load

### Symptoms
- Operations fail with `RESOURCE_EXHAUSTED` error
- Updates slow dramatically (< 10 ops/sec vs baseline 2,000 ops/sec)
- Memory pressure visible (`PSI memory some 50%+` on affected nodes)
- Disk I/O bottleneck (queue depth > 32)
- Error messages: `MEMORY_PRESSURE_HIGH`, `DISK_IO_BOTTLENECK`, `CONNECTION_POOL_EXHAUSTED`

### Root Cause Analysis

1. **Check cluster resource utilization:**
   ```bash
   themis_cli cluster resource-utilization --all-nodes --detailed
   ```

2. **Monitor memory pressure:**
   ```bash
   ssh <affected_node> 'cat /proc/pressure/memory'
   ```

3. **Check disk I/O queue depth:**
   ```bash
   ssh <affected_node> 'iostat -x 1 5'
   ```

4. **Review connection pool status:**
   ```bash
   themis_cli stats get-metric --name=connection_pool_active_connections
   themis_cli stats get-metric --name=connection_pool_max_size
   ```

### Recovery Procedure

1. **Reduce incoming update load (immediate, 5 minutes)**
   - Pause non-critical updates: `themis_cli updates pause --selector="priority<high"`
   - Monitor: `themis_cli updates list-operations --status=in_progress | wc -l`
   - Target: Reduce in-flight operations to < 50% peak

2. **Scale cluster horizontally**
   - Add nodes: `themis_cli cluster add-nodes --count=2 --wait-for-rebalance`
   - Verify added nodes healthy: `themis_cli cluster list-nodes --status=healthy`
   - Rebalance: `themis_cli cluster rebalance --targets=all`

3. **Increase resource limits if possible**
   - Increase memory limit: `themis_cli config set-setting --key=max_memory_mb --value=<new_value>`
   - Increase connection pool: `themis_cli config set-setting --key=connection_pool_max_size --value=500`
   - Verify applied: `themis_cli stats get-metric --name=connection_pool_max_size`

4. **Resume update operations gradually**
   - Unpause: `themis_cli updates resume --selector="priority<high"`
   - Monitor throughput: `watch 'themis_cli stats get-metric --name=updates_completed_total | tail -1'`
   - Expected: Throughput ≥ 1,500 ops/sec before resuming paused updates

### Prevention Tips

- **Capacity Planning:** Monitor p95 update load; ensure 3× headroom for spikes
- **Monitoring:** Alert on memory pressure > 30% or disk queue depth > 16
- **Tuning:** Profile high-load scenarios; adjust connection pool, buffer sizes per testing results
- **Load Shedding:** Implement priority queuing to protect critical updates during peak load

---

## Scenario 6: Manifest Corruption

### Symptoms
- Operations fail with `PATCH_CHECKSUM_MISMATCH` or `STATE_HISTORY_CORRUPT` errors
- Manifest files have invalid checksums or corrupted entries
- Rollback fails due to missing/invalid checkpoint data
- Error messages: `MANIFEST_INTEGRITY_FAILED`, `CHECKPOINT_UNREADABLE`

### Root Cause Analysis

1. **Identify corrupted manifest:**
   ```bash
   themis_cli updates get-manifest --operation-id=<op_id>
   ```

2. **Verify checksum integrity:**
   ```bash
   themis_cli updates validate-manifest --operation-id=<op_id> --detailed
   ```

3. **Check disk health (possible hardware failure):**
   ```bash
   ssh <affected_node> 'smartctl -a /dev/sda | grep -A 5 "SMART overall"'
   ssh <affected_node> 'dmesg | grep -i "I/O error"'
   ```

4. **Review checkpoint history:**
   ```bash
   themis_cli updates list-checkpoints --operation-id=<op_id>
   ```

### Recovery Procedure

1. **Rollback to last valid checkpoint (5-10 minutes)**
   - Find last valid checkpoint: `themis_cli updates get-last-valid-checkpoint --operation-id=<op_id>`
   - Rollback: `themis_cli updates rollback --checkpoint-id=<valid_checkpoint_id>`
   - Verify: `themis_cli updates validate-manifest --operation-id=<op_id>`

2. **Repair or replace affected storage**
   - If hardware failure: `systemctl stop themis-updater && systemctl stop themis-storage`
   - Replace disk: `hot-swap procedure for <affected_node>`
   - Restart services: `systemctl start themis-storage && systemctl start themis-updater`

3. **Rebuild manifest from transaction log**
   - Command: `themis_cli updates rebuild-manifest --operation-id=<op_id> --source=transaction_log`
   - Validate: `themis_cli updates validate-manifest --operation-id=<op_id>`
   - Monitor: `watch 'themis_cli updates get-operation-status --operation-id=<op_id>'` (wait for completion)

4. **Retry operation**
   - Command: `themis_cli updates retry-operation --operation-id=<op_id>`
   - Expected: Operation completes with valid checksum

### Prevention Tips

- **Monitoring:** Alert on checksum mismatches (> 0 in any 1-hour window)
- **Hardware:** Run periodic SMART diagnostics; replace drives with high error rates
- **Backup:** Maintain backup copies of critical manifests; test recovery procedures quarterly
- **Journaling:** Enable transaction journaling to enable manifest rebuild from logs

---

## Scenario 7: Cluster Partition

### Symptoms
- Operations fail with `NETWORK_PARTITION` error
- Cluster split into 2+ isolated partitions with no cross-partition communication
- Some nodes see different state than others (split-brain risk)
- Error messages: `QUORUM_LOST`, `MAJORITY_PARTITION_REQUIRED`, `SPLIT_BRAIN_PREVENTION_ACTIVE`

### Root Cause Analysis

1. **Identify partition topology:**
   ```bash
   themis_cli cluster topology --detailed
   ```

2. **Check inter-node network connectivity:**
   ```bash
   themis_cli cluster connectivity-check --all-pairs
   ```

3. **Review network interface status on affected nodes:**
   ```bash
   ssh <node1> 'ip addr | grep inet'
   ssh <node1> 'ip route | grep default'
   ```

4. **Check network device errors:**
   ```bash
   ssh <affected_node> 'ethtool -S eth0 | grep -i error'
   ```

### Recovery Procedure

1. **Identify and isolate minority partition (do NOT execute until confirmed)**
   - Majority partition: nodes with > 50% cluster membership
   - Minority partition: remaining nodes
   - Confirm via: `themis_cli cluster get-partition-info --all`

2. **Halt minority partition to prevent split-brain**
   - Command: `themis_cli cluster halt-partition --partition-id=<minority_partition>`
   - Verify halted: `themis_cli cluster get-partition-status --partition-id=<minority_partition>`
   - **CRITICAL:** Only proceed after verification to prevent dual-authority scenario

3. **Restore network connectivity**
   - Investigate root cause: switch misconfiguration, fiber cut, firewall rules, etc.
   - Physical inspection: check cables, optical connections
   - Network team: verify firewall rules, load balancer configuration
   - Example: `sudo ufw allow from <other_partition_subnet>`

4. **Rejoin minority partition**
   - Command: `themis_cli cluster rejoin-partition --partition-id=<minority_partition> --wait-for-sync`
   - Verify sync completion: `themis_cli cluster check-consistency --all`
   - Expected: All nodes report consistent state

### Prevention Tips

- **Network Design:** Use redundant network paths (dual NICs, multiple switches)
- **Monitoring:** Alert on inter-node ping failures (> 100ms latency or packet loss)
- **Testing:** Conduct chaos engineering tests: simulate network partitions, verify fail-closed behavior
- **Documentation:** Maintain current cluster topology diagram and partition recovery procedures

---

## Scenario 8: Deadlock/Race Conditions

### Symptoms
- Operations hang indefinitely (no timeout, no error message)
- Operations fail with generic timeout after 30+ minutes
- State machine transitions appear stuck
- Conflicting operations attempting to modify same resources
- Error messages: `OPERATION_TIMEOUT`, `CIRCULAR_DEPENDENCY_DETECTED`, `LOCK_CONTENTION_HIGH`

### Root Cause Analysis

1. **Identify hung operations:**
   ```bash
   themis_cli updates list-operations --status=in_progress --age=>5m
   ```

2. **Analyze operation dependencies:**
   ```bash
   themis_cli updates get-dependency-graph --operation-id=<op_id> --detailed
   ```

3. **Check lock state and contention:**
   ```bash
   themis_cli stats get-metric --name=lock_wait_total
   themis_cli stats get-metric --name=lock_contention_high
   ```

4. **Review detailed operation state machine logs:**
   ```bash
   journalctl -u themis-updater --grep="state_transition\|lock_acquired\|lock_released" -n 500 --no-pager
   ```

### Recovery Procedure

1. **Force cancel hung operation (5-10 minutes)**
   - **WARNING:** Only if operation has been hung > 10 minutes
   - Command: `themis_cli updates cancel-operation --operation-id=<op_id> --mode=force`
   - Verify cancelled: `themis_cli updates get-operation-status --operation-id=<op_id> | grep state`

2. **Rollback to pre-operation state**
   - Command: `themis_cli updates rollback --operation-id=<op_id> --mode=full`
   - Verify rollback: `themis_cli consistency check --detailed | grep -i mismatch`

3. **Investigate root cause**
   - Review state machine logs for circular dependencies
   - Analyze dependency graph for unexpected lock ordering
   - Reproduce in test environment with detailed tracing enabled

4. **Retry with reduced concurrency**
   - Command: `themis_cli updates retry-operation --operation-id=<op_id> --concurrency=1 --enable-trace`
   - Monitor with detailed tracing: `journalctl -u themis-updater -f --grep="lock"` (in separate terminal)
   - Expected: Operation completes with lock tracing visible in logs

### Prevention Tips

- **Code Review:** Inspect lock acquisition order for potential circular dependencies
- **Testing:** Run deadlock detection tests with stress scenarios (100+ concurrent operations)
- **Tracing:** Enable detailed lock tracing in test/canary environments; review patterns
- **Monitoring:** Alert on operation duration > p99 baseline; investigate all anomalies
- **Timeout:** Set operation timeout < maximum acceptable service latency (e.g., 5 minutes vs 30 minutes default)

---

## Cross-Scenario Troubleshooting Guide

### Initial Diagnostics (First 30 seconds)

1. **Identify affected operations:**
   ```bash
   themis_cli updates list-operations --status=failed,in_progress --age=>2m
   ```

2. **Check error category:**
   ```bash
   themis_cli updates get-operation-status --operation-id=<op_id> | grep -E "error_code|error_class"
   ```

3. **Generate diagnostic bundle:**
   ```bash
   themis_cli diagnostics collect-bundle --operation-id=<op_id> --output=/tmp/updates_diag.tar.gz
   ```

### Escalation Procedures

| Duration | Action | Threshold |
|----------|--------|-----------|
| 2-5 min | Acknowledge incident; start diagnostics | Any single operation failure |
| 5-10 min | Attempt basic recovery (restart coordinator, rollback, etc.) | 2+ operations affected OR critical priority |
| 10-30 min | Escalate to on-call engineer; consider emergency mitigations | 10+ operations affected OR cascading failures |
| 30+ min | Page senior engineer; prepare for rollback of last deployment | Cluster-wide impact OR split-brain risk |

### Tools and Commands Quick Reference

```bash
# List operations by various filters
themis_cli updates list-operations --status=failed --age=>5m
themis_cli updates list-operations --filter="*migrate*"
themis_cli updates list-operations --priority=critical

# Get detailed operation state
themis_cli updates get-operation-status --operation-id=<op_id> --detailed
themis_cli updates get-operation-progress --operation-id=<op_id> --watch

# Recovery operations
themis_cli updates rollback --operation-id=<op_id> --mode=full
themis_cli updates retry-operation --operation-id=<op_id>
themis_cli updates cancel-operation --operation-id=<op_id>

# Diagnostics and validation
themis_cli diagnostics collect-bundle --operation-id=<op_id>
themis_cli consistency check --detailed
themis_cli cluster topology --detailed
```

### Runbook Maintenance

- **Review Frequency:** Quarterly (or after each major incident)
- **Update Triggers:** New failure scenarios identified, procedure changes, tool updates
- **Owner:** Updates module team lead
- **Distribution:** All on-call operators + operations team

---

**Runbook Version:** 1.0 (Phase 6 Implementation, Oct 2026)  
**Last Updated:** 2026-08-08  
**Next Review:** 2026-11-08
