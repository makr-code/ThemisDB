# Operator Runbook: Fencing Override Procedure

**Module:** failover  
**Scope:** EpochFencingManager, AutoFailoverManager  
**Wave:** D (Operability Hardening)  
**Last Updated:** 2026-08-24

---

## 1. Overview

Epoch fencing is a safety mechanism that prevents dual-master by ensuring only one
node holds a valid epoch token at any time. In rare operational scenarios (planned
maintenance, manual DR, fencing manager failure), an operator may need to override
the automatic fencing gate.

**⚠️ WARNING:** Overriding fencing bypasses split-brain prevention. Perform these
steps only with explicit awareness of the current cluster state and only after
confirming that no other node believes itself to be leader.

---

## 2. When to Override

| Scenario | Override Allowed | Notes |
|---|---|---|
| Planned leader handover during maintenance window | ✅ Yes | Follow §3 below |
| Fencing manager service is down, cluster is read-only | ✅ Yes (with caution) | Follow §4 below |
| DR failover to recovery site with no quorum at primary | ✅ Yes | Use DR plan with `enforce_epoch_fencing=false` |
| Automated failover is stuck waiting for fencing | ✅ Yes (with human approval) | Follow §5 below |
| Fencing returning epoch=0 for unknown reason | ❌ No — investigate first | Check fencing service health |
| Normal operations — no incident | ❌ Never | Do not override without cause |

---

## 3. Planned Maintenance Override

### Pre-Conditions

- [ ] Confirm current cluster leader: `themisdb-cli cluster leader-status`
- [ ] Confirm no active failovers in progress: `themisdb-cli failover status`
- [ ] Notify team and open a maintenance window ticket

### Steps

1. **Pause automatic failover:**
   ```bash
   themisdb-cli failover pause --reason "planned-maintenance-$(date +%Y%m%d)"
   ```

2. **Manually bump epoch on the new intended leader:**
   ```bash
   themisdb-cli fencing bump-epoch --node <new_leader_id> \
     --reason "planned-maintenance-manual-handover"
   # Record the returned epoch number for post-maintenance verification
   ```

3. **Demote current leader:**
   ```bash
   themisdb-cli node demote --node <current_leader_id>
   ```

4. **Verify new leader is elected:**
   ```bash
   themisdb-cli cluster leader-status
   # Confirm new_leader_id is now Leader with the epoch returned in step 2
   ```

5. **Resume automatic failover:**
   ```bash
   themisdb-cli failover resume
   ```

6. **Record in maintenance log:**
   - Epoch before override, epoch after override
   - Time of override, operator name, ticket reference

---

## 4. Fencing Service Down Override

### Safety Pre-Conditions (ALL must be true)

- [ ] Fencing manager service is confirmed down (not just slow)
- [ ] Cluster has exactly one node that was last known to be leader
- [ ] No competing leadership claims observed in logs within last 60 s
- [ ] A second operator has confirmed the above independently

### Steps

1. **Set `enable_split_brain_prevention=false` in AutoFailoverConfig:**
   This can be done via the admin API or config reload:
   ```bash
   themisdb-admin config set failover.enable_split_brain_prevention=false
   ```

2. **Proceed with required failover** (if a node has actually failed and needs replacement).

3. **Restore fencing as soon as fencing service is back:**
   ```bash
   themisdb-admin config set failover.enable_split_brain_prevention=true
   ```
   **Do not leave prevention disabled longer than necessary.**

4. **Verify split_brain_count metric did not increment during override period.**

---

## 5. Stuck Automated Failover Override

When `preventSplitBrain()` keeps emitting QUORUM_UNAVAILABLE and the failover is
stuck, but a human has verified it is safe to proceed:

1. Check the `quorum_log` file to understand last committed quorum state:
   ```bash
   cat <quorum_log_path>  # path from failover config
   ```

2. If the log shows a completed quorum from a previous run that is blocking the new election:
   - Truncate the log file (after human approval and backup):
     ```bash
     cp <quorum_log_path> <quorum_log_path>.bak.$(date +%Y%m%d_%H%M%S)
     truncate -s 0 <quorum_log_path>
     ```
   - Restart the failover manager process.

3. **Post-override:** Verify the new quorum decision is written to the log immediately.

---

## 6. Post-Override Checklist

- [ ] `enable_split_brain_prevention` restored to `true`
- [ ] `failover_split_brain_count` metric is 0 after override
- [ ] Quorum log contains the new epoch decision
- [ ] Cluster shows exactly 1 Leader node
- [ ] Maintenance log / incident ticket updated with override details and rationale

---

## 7. Related Runbooks

- `failover_runbook_split_brain.md` — Responding to an active split-brain
- `failover_runbook_manual_recovery.md` — Full cluster manual recovery
