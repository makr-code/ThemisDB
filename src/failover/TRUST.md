---
Author: "ThemisDB Team"
Created: "2026-10-30"
Last Updated: "2026-10-30"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
---

Exact model backend not verifiable from final document text alone.

# Failover Trust Boundaries

## Purpose

This document identifies which failover inputs are trusted and which safeguards
the module enforces. It is for security reviewers, maintainers, and operators.
The module's security contract is defined by its public headers and actual
implementation; this page does not claim that failover provides authentication.

## Trust boundaries

| Boundary | Assumption | Control and residual risk |
|---|---|---|
| Caller to manager | The caller is authorized by its owning service/control plane. | `AutoFailoverManager` and `DisasterRecoveryManager` do not authenticate or authorize callers. Keep them behind the deployment's authenticated admin boundary. |
| Replication, health, and fencing managers | Injected managers return truthful, current results and enforce their own contracts. | Quorum checks and fencing tokens gate promotion. A compromised or stale dependency can invalidate those decisions. |
| DR plan fields and step hooks | The caller supplies intended site/snapshot identifiers and trusted hook implementations. | Validation checks required strings, not identity, authorization, snapshot existence, or adapter safety. Hooks replace built-in steps and may perform arbitrary side effects. |
| Quorum log file | The configured path and filesystem permissions are controlled by the deployment. | Delimited fields reject pipe characters and records carry CRC32. CRC32 is not keyed or authenticated; a writer with file access can alter or forge records. Recovery skips malformed or CRC-invalid lines. |
| Logs and callbacks | Node IDs, plan IDs, and error details can be exposed to configured log/callback consumers. | Restrict access to operational logs and callback sinks; do not put credentials or secrets in identifiers or detail strings. |

## Fail-closed safeguards

- Promotion requires replication-manager quorum; configured split-brain
  prevention requires a fencing manager and rejects epoch 0.
- A configured quorum log append failure blocks the corresponding quorum or
  promotion decision.
- Invalid DR plan fields, missing required managers, failed steps, and concurrent
  uncached execution return explicit failure outcomes.
- The runtime does not provide a general rollback for actions completed before a
  later DR or failover step fails.

These controls reduce unsafe transitions; they do not prove that downstream
storage restore, traffic movement, spare activation, or metadata updates
completed. The current built-in DR restore/traffic methods and failover spare/
metadata methods do not perform those external operations. See
[IMPLEMENTATION.md](IMPLEMENTATION.md).

## Incident handling

Treat uncertain quorum, missing/invalid fencing, quorum-log write or integrity
concerns, and incomplete recovery as stop conditions. Preserve logs and quorum
records, keep the candidate from serving writes while leader state is ambiguous,
and escalate to failover plus replication/storage owners. Follow
[RUNBOOK.md](RUNBOOK.md); do not disable fencing or treat CRC32 as proof of
record authenticity.

## Related documentation

- [SECURITY.md](SECURITY.md)
- [OPERATIONS.md](OPERATIONS.md)
- [API.md](API.md)
- `include/failover/failover_api_contract.h`
- `src/failover/quorum_log.cpp`
