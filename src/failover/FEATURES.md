---
Author: ThemisDB Contributors
Created: 2026-10-18
Last Updated: 2026-11-01
Status: active
AI_Used: true
AI_Models: unknown
AI_Contributor: GitHub Copilot
Human_Review: false
Human_Author: ThemisDB Contributors
---

Exact model backend not verifiable from final document text alone.

# Failover Features

This inventory describes implemented module surfaces, not proof of end-to-end
recovery or a production SLO.

| Surface | Source-backed behavior |
|---|---|
| Automatic failover | Health-check outcomes update consecutive-failure tracking. Requests enter a bounded queue; the worker checks quorum and configured fencing, selects a healthy non-witness replica, and asks the replication manager to trigger failover. |
| Health behavior | `health_check_call_timeout_ms` bounds the monitoring thread's wait for a health call; adaptive intervals and GC-grace handling are optional configuration paths. A timed-out health task may continue in the background. |
| Quorum/fencing | Optional local quorum log; append failures block the associated promotion path. Invalid epoch 0 is rejected. Log recovery is not manager-state restoration. |
| Disaster recovery | Plan validation, ordered seven-step coordination, injectable step hooks, per-step results, in-memory result caching for non-empty plan IDs, and aggregate statistics. |
| Topology snapshot | Captures a version and failure map with sorted node IDs; helpers compare node-set membership. |

**Not implemented by built-in handlers:** storage restore, traffic cutover,
metadata writes, and spare activation. Non-dry-run operations without their
required hooks now fail closed; hook implementations must verify actual
effects. See [`IMPLEMENTATION.md`](IMPLEMENTATION.md) for the remaining adapter
work.

## Sources

See [`API.md`](API.md), [`IMPLEMENTATION.md`](IMPLEMENTATION.md), and the public
headers `include/failover/auto_failover_manager.h`,
`include/failover/disaster_recovery_manager.h`,
`include/failover/failover_api_contract.h`,
`include/failover/quorum_log.h`, and
`include/failover/topology_snapshot.h` for exact behavior.
