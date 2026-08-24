# Operator Runbook: Manual Failover Recovery Procedure

**Module:** failover  
**Scope:** AutoFailoverManager, DisasterRecoveryManager  
**Wave:** D (Operability Hardening)  
**Last Updated:** 2026-08-24

---

## 1. Overview

This runbook covers **manual cluster recovery** when automatic failover has failed
or is insufficient — for example, after a primary site disaster, after a multi-node
failure, or when the automatic failover manager itself is unavailable.

---

## 2. Pre-Recovery Assessment

### 2.1 Collect Cluster State

```bash
# Cluster overview
themisdb-cli cluster status

# Node health details
themisdb-cli node list --format=health

# Current leader and epoch
themisdb-cli cluster leader-status

# Recent failover events
themisdb-cli failover history --last 20

# Quorum log
cat <quorum_log_path>  # path from config
```

### 2.2 Classify the Failure

| Scenario | Recovery Path |
|---|---|
| Single leader node failed, replicas healthy | §3: Standard replica promotion |
| All nodes failed, backup available | §4: DR plan execution |
| Leader + majority of replicas failed | §5: Reduced-quorum recovery |
| Network partition resolved | §6: Post-partition reconciliation |
| Disaster recovery site failover | §7: DR execution |

---

## 3. Standard Replica Promotion

**Use when:** The primary leader has failed but at least one healthy replica exists.

1. **Verify no automatic failover is in progress:**
   ```bash
   themisdb-cli failover status
   # If in-progress, wait up to 30s; if stuck, proceed manually
   ```

2. **Identify the best replica candidate:**
   ```bash
   themisdb-cli replication replicas --format=detail
   # Choose the replica with: HEALTHY status, lowest replication lag, not WITNESS role
   ```

3. **Trigger manual promotion:**
   ```bash
   themisdb-cli failover trigger \
     --failed-node <failed_leader_id> \
     --promote <candidate_replica_id>
   ```

4. **Monitor promotion progress:**
   ```bash
   themisdb-cli failover watch --timeout=60s
   ```

5. **Verify new leader:**
   ```bash
   themisdb-cli cluster leader-status
   themisdb-cli node status --node <promoted_id>
   # Expected: new node in Leader role, epoch incremented
   ```

6. **Update DNS/VIP** (if applicable):
   ```bash
   dns-ctl update --record <cluster_write_endpoint> --target <promoted_id>
   ```

---

## 4. Full Recovery from Backup

**Use when:** All nodes have failed and you must restore from a snapshot.

1. **Select the most recent valid snapshot:**
   ```bash
   themisdb-cli snapshots list --sort=timestamp --limit=5
   # Record snapshot_id
   ```

2. **Create and execute a DR plan:**
   ```bash
   themisdb-cli dr plan create \
     --plan-id "manual-recovery-$(date +%Y%m%d-%H%M)" \
     --primary-site <primary> \
     --recovery-site <recovery> \
     --snapshot-id <snapshot_id>
   ```

3. **Run a dry-run first:**
   ```bash
   themisdb-cli dr plan execute --plan-id <plan_id> --dry-run
   # All steps should PASS in dry-run before real execution
   ```

4. **Execute the plan:**
   ```bash
   themisdb-cli dr plan execute --plan-id <plan_id>
   # Monitor step-by-step progress
   ```

5. **Verify recovery:**
   ```bash
   themisdb-cli cluster status
   themisdb-cli replication health
   ```

---

## 5. Reduced-Quorum Recovery

**Use when:** Leader + majority of replicas failed; quorum cannot be established.

⚠️ **This requires human approval.** Proceeding without quorum risks data loss.

1. **Document the data loss window:**
   - Last successful checkpoint timestamp
   - Last replicated sequence number on surviving node(s)

2. **Force-promote the surviving node with the most complete WAL:**
   ```bash
   themisdb-cli node force-promote --node <survivor_id> \
     --reason "reduced-quorum-emergency" \
     --acknowledge-data-loss=true
   ```
   This bypasses quorum checks. Document the decision with ticket reference.

3. **Rebuild replica set** from the promoted node once available.

---

## 6. Post-Partition Reconciliation

After a network partition is resolved:

1. **Identify which partition had the authoritative leader** (higher epoch):
   ```bash
   themisdb-cli node epoch-status --all
   ```

2. **Fence all nodes in the minority partition:**
   ```bash
   for node in <minority_partition_nodes>; do
     themisdb-cli fencing force-fence --node $node --reason "post-partition-reconciliation"
   done
   ```

3. **Force minority nodes to rejoin as followers:**
   ```bash
   themisdb-cli replication rejoin --nodes <minority_nodes> --leader <authoritative_leader>
   ```

4. **Monitor WAL sync completion:**
   ```bash
   themisdb-cli replication lag --all
   # Wait until all nodes show lag ≤ 100ms
   ```

---

## 7. DR Site Failover

**Use when:** The primary site is completely unavailable.

1. Confirm primary site is unreachable (do not failover on transient partition).
2. Follow **§4** above using the recovery site as `--recovery-site`.
3. After recovery: update all client endpoints to point to recovery site.
4. When primary site comes back: resync as replica before re-promoting.

---

## 8. Post-Recovery Checklist

- [ ] Cluster has exactly 1 Leader node
- [ ] All replica nodes show HEALTHY status
- [ ] Replication lag ≤ 100ms on all replicas
- [ ] Quorum log contains new epoch entry
- [ ] `failover_split_brain_count` metric is 0
- [ ] Client endpoints (DNS/VIP) point to new leader
- [ ] Health-check monitoring alerts cleared
- [ ] Incident ticket updated with recovery timeline, root cause, and action items

---

## 9. Escalation

| Scenario | Escalation |
|---|---|
| Recovery fails after 3 attempts | Page on-call SRE + database team |
| Data loss suspected | Freeze all writes, escalate to incident commander |
| DR plan execution fails at EPOCH_FENCING step | See `failover_runbook_fencing_override.md` |
| Recovery site unavailable | Business continuity plan |

---

## 10. Related Runbooks

- `failover_runbook_split_brain.md` — Active split-brain response
- `failover_runbook_fencing_override.md` — Fencing bypass procedure
- `RUNBOOK_REPLICATION_LAG_FAILOVER.md` — Replication lag issues
- `RUNBOOK_SHARDING_TOPOLOGY_CHANGE.md` — Topology repair/rebalance
