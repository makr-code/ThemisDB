# Runbook: Distributed Knowledge Module

<!-- Status: current | validated: 2026-09-16 | Wave D operability deliverable -->

## Purpose

Operator remediation guide for the ThemisDB `distributed_knowledge` module.
Covers the five most critical incident classes, their log patterns, triage
steps, and recommended remediation actions.

---

## Scenario 1 — Federation Timeout

**Log pattern:** `[DIST_KG:FederationTimeout]`

**Symptoms**
- A federated knowledge aggregation round fails to complete within the timeout.
- Log lines contain `[DIST_KG:FederationTimeout]` with round ID and peer node list.
- Federated model updates stall; downstream RAG and reasoning pipelines see stale knowledge.

**Triage**
1. Identify the failing round ID and the list of non-responsive peer nodes.
2. Check peer node connectivity: `themis_admin dist_kg peer status`.
3. Verify network routes to all federation peers are healthy.
4. Review the federation round timeout configuration against current network latency.

**Remediation**
1. Retry the federation round excluding unreachable peers:
   `themis_admin dist_kg federation retry --round <id> --skip-unavailable`.
2. Increase the federation round timeout if network latency has increased:
   set `distributed_knowledge.federation.round_timeout_ms`.
3. Mark unhealthy peers as temporarily inactive: `themis_admin dist_kg peer disable <node_id>`.
4. Re-enable peers once connectivity is restored.

**Escalation**
Persistent federation timeouts across multiple rounds indicate a network or
peer availability issue — escalate to the infrastructure team with peer status,
network latency metrics, and the round failure log.

---

## Scenario 2 — Merge Conflict

**Log pattern:** `[DIST_KG:MergeConflict]`

**Symptoms**
- Concurrent knowledge graph merge operations produce conflicting updates.
- Log lines contain `[DIST_KG:MergeConflict]` with conflicting entity IDs and sources.
- Knowledge graph consistency checks fail after merge.

**Triage**
1. Identify the conflicting entity IDs and their source nodes from the log.
2. Inspect the merge conflict details: `themis_admin dist_kg merge status`.
3. Determine the conflict resolution policy in effect: `distributed_knowledge.merge.conflict_policy`.
4. Check whether the conflict is due to concurrent writes from multiple federation rounds.

**Remediation**
1. Apply the configured conflict resolution policy:
   `themis_admin dist_kg merge resolve --policy <last_write_wins|source_priority>`.
2. If automatic resolution fails, manually resolve the conflict:
   `themis_admin dist_kg merge manual-resolve <entity_id> --keep-source <node_id>`.
3. Verify knowledge graph consistency after resolution:
   `themis_admin dist_kg graph verify`.
4. Reduce concurrent merge parallelism: set `distributed_knowledge.merge.max_concurrent_merges`.

**Escalation**
Merge conflicts that cannot be automatically resolved require intervention from
the knowledge graph team — provide entity IDs, conflict details, and source
node metadata.

---

## Scenario 3 — Replay Divergence

**Log pattern:** `[DIST_KG:ReplayDivergence]`

**Symptoms**
- Knowledge graph replay produces divergent state across nodes.
- Log lines contain `[DIST_KG:ReplayDivergence]` with diverging node IDs and sequence offset.
- Federated queries return inconsistent results from different nodes.

**Triage**
1. Identify the diverging nodes and the sequence offset from the log.
2. Compare graph state checksums across nodes:
   `themis_admin dist_kg graph checksum --all-nodes`.
3. Identify the replay event where divergence started.
4. Check for missing events in the replay log on one or more nodes.

**Remediation**
1. Re-synchronise the diverging node from the canonical source:
   `themis_admin dist_kg node resync <node_id>`.
2. Replay missing events from the event log:
   `themis_admin dist_kg replay --from-sequence <seq_id> --node <node_id>`.
3. Quarantine the diverging node from federation until sync is verified.
4. Verify consistency after resync: `themis_admin dist_kg graph verify --all-nodes`.

**Escalation**
Graph divergence that cannot be resolved by resync requires escalation to the
distributed systems team with checksum comparison output and the replay event
log.

---

## Scenario 4 — Policy Gate Failed

**Log pattern:** `[DIST_KG:PolicyGateFailed]`

**Symptoms**
- A knowledge sync operation is blocked by a policy gate.
- Log lines contain `[DIST_KG:PolicyGateFailed]` with policy ID and operation context.
- Cross-border or cross-tenant knowledge transfers are rejected.

**Triage**
1. Identify the failing policy ID and the operation that triggered it.
2. Review the policy definition: `themis_admin dist_kg policy show <id>`.
3. Determine whether the gate failure is intentional (compliance block) or a misconfiguration.
4. Check whether the policy was recently updated or a new data classification was applied.

**Remediation**
1. If the gate is a misconfiguration, update the policy:
   `themis_admin dist_kg policy update <id>`.
2. If the gate is a legitimate compliance block, route the sync through an approved
   alternative channel or apply data anonymisation before sync.
3. Request a policy exemption through the governance workflow if appropriate.
4. Monitor for repeated gate failures that may indicate a policy regression.

**Escalation**
Policy gate failures with compliance implications must be escalated to the
governance and compliance team — do not bypass policy gates without explicit
approval.

---

## Scenario 5 — Federation Round Stall

**Log pattern:** `[DIST_KG:FederationTimeout]` (repeated, same round)

**Symptoms**
- The same federation round ID appears in repeated timeout logs.
- The federation coordinator is stuck waiting for a quorum that cannot be reached.
- Knowledge updates are completely blocked.

**Triage**
1. Identify the stuck round ID and how many peers have responded.
2. Check the quorum configuration: `distributed_knowledge.federation.quorum_fraction`.
3. Determine whether the quorum fraction can be reduced temporarily.
4. Review network partition indicators across the federation cluster.

**Remediation**
1. Abort the stuck round and start a new one with available peers:
   `themis_admin dist_kg federation abort-round <id>`.
2. Temporarily reduce the quorum fraction: set `distributed_knowledge.federation.quorum_fraction: 0.5`.
3. Restart the federation coordinator if it is unresponsive:
   `themis_admin dist_kg coordinator restart`.
4. Restore the quorum fraction once all peers are back online.

**Escalation**
A federation stall that cannot be resolved by aborting the round and reducing
quorum indicates a cluster-level partition — escalate to the infrastructure
team with peer status, quorum config, and coordinator logs.

---

## Reference

| Log Pattern                      | Severity | SLO Impact | Owner              |
|----------------------------------|----------|------------|--------------------|
| `[DIST_KG:FederationTimeout]`    | High     | Yes        | distributed_knowledge |
| `[DIST_KG:MergeConflict]`        | Medium   | Partial    | distributed_knowledge |
| `[DIST_KG:ReplayDivergence]`     | High     | Yes        | distributed_knowledge |
| `[DIST_KG:PolicyGateFailed]`     | Medium   | Partial    | distributed_knowledge |

---

*Wave D operability deliverable — see `src/distributed_knowledge/ROADMAP.md`.*
