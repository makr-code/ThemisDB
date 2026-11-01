---
Author: "ThemisDB Team"
Created: "2026-10-30"
Last Updated: "2026-11-01"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
---

Exact model backend not verifiable from final document text alone.

# Failover Security Notes

Report vulnerabilities through the project-level `SECURITY.md`.

## Security scope

Failover security is primarily about quorum and epoch-fencing gates, safe
handling of recovery plans, and the trustworthiness of injected managers and
local quorum-log storage. This module does not authenticate callers or provide
authorization; the server/control-plane boundary must do so.

## Threats and controls

| Threat | Control in the current implementation | Residual risk |
|---|---|---|
| Promotion without quorum or valid fencing | Quorum check and non-zero epoch checks gate guarded promotion | Correctness depends on the injected replication and fencing managers. If split-brain prevention is disabled and no fencing manager is configured, the promotion path can proceed without that guard. |
| Malformed or corrupted quorum-log records | Pipe characters are rejected on append; recovery skips malformed, unparseable, and CRC-mismatched records | CRC32 is not keyed or authenticated; a user who can write the log can forge a record. |
| Invalid DR plan fields | Required plan and site IDs, plus non-dry-run snapshot ID, must be non-empty | The implementation does not authenticate site IDs or verify that the named snapshot exists. |
| Partial recovery or failover | The first failing DR step returns an explicit failure and later steps do not run | Earlier side effects are not rolled back; production hook correctness is not independently verified by the manager, and metadata persistence/spare activation remain unavailable. |
| Operational blind spots | State, statistics, callbacks, and `spdlog` messages are available | The current managers do not emit named Prometheus metrics or trace spans; external monitoring must not assume these exist. |

## Operational handling

Treat uncertain quorum, missing or invalid required fencing, quorum-log append
failure, and incomplete recovery as stop conditions. Keep the candidate from
serving writes while leader state is ambiguous. Preserve relevant log and
quorum-log evidence, and involve failover, replication, and storage owners before
resuming writes. Do not bypass split-brain controls to clear an alert.

For details on manager trust assumptions and log integrity, see
[TRUST.md](TRUST.md). For incident response, use [RUNBOOK.md](RUNBOOK.md) and
[OPERATIONS.md](OPERATIONS.md).