# Operator Runbook: Split-Brain Incident Response

**Module:** failover  
**Scope:** AutoFailoverManager, DisasterRecoveryManager  
**Wave:** D (Operability Hardening)  
**Last Updated:** 2026-08-24

---

## 1. Overview

A **split-brain** occurs when two or more nodes simultaneously believe they are the
authoritative leader for the same epoch. This runbook covers detection, containment,
recovery, and post-mortem procedures.

**Severity:** CRITICAL — write divergence may occur if not immediately contained.

---

## 2. Detection

### 2.1 Automated Detection

ThemisDB's epoch fencing system (`EpochFencingManager`) prevents split-brain by
ensuring only one node holds a valid epoch token at a time. If split-brain is
detected, the following diagnostic is emitted:

```
Failover diagnostic [code=2] node='<node_id>': <detail>
```

`FailoverErrorCode::SPLIT_BRAIN_DETECTED = 2`

Also watch for metric `failover_split_brain_count` incrementing unexpectedly.

### 2.2 Manual Detection Signs

- Two nodes both accepting writes simultaneously
- Diverging replication lag on a single node
- Clients receiving conflicting reads from different cluster members
- Log entries: `"split-brain prevention requires fencing manager"` or `"fencing returned invalid epoch"`

---

## 3. Containment

### 3.1 Immediate Actions (within 5 minutes)

1. **Identify the higher-epoch node:**
   ```bash
   # Query each suspected leader for its current epoch
   themisdb-cli node epoch-status --node <node_id>
   ```
   The node with the **higher epoch** is the authoritative leader.

2. **Fence the lower-epoch node:**
   ```bash
   themisdb-cli fencing force-fence --node <stale_leader_id> --reason "split-brain-containment"
   ```
   This bumps the epoch and invalidates the stale leader's token.

3. **Force the stale leader into follower mode:**
   ```bash
   themisdb-cli node demote --node <stale_leader_id> --force
   ```

4. **Verify single-leader state:**
   ```bash
   themisdb-cli cluster leader-status
   # Expected: exactly one node in Leader role
   ```

### 3.2 If Fencing Is Unavailable

If `EpochFencingManager` is not configured or unavailable:

1. Immediately set the cluster to **read-only mode** via load balancer:
   ```bash
   lb-ctl set-policy cluster=<name> write=deny
   ```
2. Page the on-call engineer for manual investigation.
3. Do **not** restart nodes until write divergence scope is assessed.

---

## 4. Recovery

### 4.1 Assess Write Divergence

```bash
# Compare WAL tails on both suspected leaders
themisdb-cli wal tail --node <node_a> --last 1000
themisdb-cli wal tail --node <node_b> --last 1000
```

Identify the divergence point (first differing sequence number).

### 4.2 Choose Authoritative State

- **Default:** The node with the higher epoch number is authoritative.
- **Exception:** If the higher-epoch node has fewer committed entries, escalate to
  a human decision. Do not automatically prefer the longer WAL without confirming
  epoch ordering.

### 4.3 Reconcile the Divergent Node

```bash
# Truncate the divergent WAL back to the common point
themisdb-cli wal truncate --node <divergent_node> --seq <last_common_seq>
# Re-sync from authoritative leader
themisdb-cli replication resync --node <divergent_node> --leader <authoritative_node>
```

### 4.4 Re-enable Writes

```bash
# After confirming single-leader state and successful re-sync:
lb-ctl set-policy cluster=<name> write=allow
```

---

## 5. Post-Mortem Checklist

- [ ] Root cause identified: fencing bypass, epoch counter reset, network configuration?
- [ ] `EpochFencingManager` confirmed to be configured and reachable in production
- [ ] `enable_split_brain_prevention=true` verified in `AutoFailoverConfig`
- [ ] Quorum log (`quorum_log_path`) persisting decisions on restart
- [ ] Metrics dashboard alert for `failover_split_brain_count > 0` verified active
- [ ] Runbook updated if new failure mode discovered

---

## 6. Escalation

| Scenario | Escalation |
|---|---|
| Split-brain persists after fencing | Page on-call SRE immediately |
| Write divergence spans >60 s | Incident commander + DBA review |
| Fencing manager unreachable | Infrastructure oncall + network team |

---

## 7. Related Runbooks

- `failover_runbook_fencing_override.md` — When and how to override fencing
- `failover_runbook_manual_recovery.md` — Full cluster recovery procedure
- `RUNBOOK_REPLICATION_LAG_FAILOVER.md` — Replication lag and WAL shipping issues
