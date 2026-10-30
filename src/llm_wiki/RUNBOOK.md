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

# LLM Wiki Incident Runbook

## Scope and safety

Use this runbook to diagnose failures in the module's plugin, policy, workspace
state, and optional RocksDB paths. It describes source-available checks; it
does not assume a particular deployment's service manager, database path, or
monitoring stack. Preserve the workspace before repair. Do not delete state or
logs as a first response.

## Triage sequence

1. Record the deployment edition/build configuration, plugin library version,
   last known successful request, operation, workspace root, policy path, and
   relevant error message.
2. Check plugin initialization and edition gating. The current gate depends
   on `THEMISDB_LLM_WIKI_ENTERPRISE_ENABLED`; a denied initialization is not a
   retrieval failure. Compare the deployed build's compile definitions and
   plugin artifacts.
3. Check policy loading if `process_policy_path` is configured. Confirm the
   YAML file exists and is readable, the policy schema is discoverable, and
   the YAML satisfies both schema and semantic requirements. Do not bypass a
   failed validation by disabling the validation stage or setting a second
   planner.
4. Check workspace filesystem access and capacity. Confirm the service
   identity can read/write the configured root and inspect `wiki/state.json`
   and `wiki/state.log` without editing either file.
5. Separate query screening from retrieval. An unsafe-pattern flag, empty
   workspace, uninitialized plugin, policy reload failure, and no matching
   candidates may all produce an empty candidate set in current code.
6. If RocksDB is expected, verify the build has the optional RocksDB path
   enabled and the store opened successfully. A build without that path does
   not provide RocksDB-backed page storage.

## Symptom-to-action

| Symptom | First checks | Safe response |
|---|---|---|
| Initialization denied | Edition compile define, loaded library, plugin factory | Restore the intended supported build/package; do not bypass the gate |
| Initialization fails with policy error | Policy path, schema resolution, parse and validation message | Restore a known-valid policy; preserve the rejected file for review |
| Hot reload fails | Policy file timestamp/readability and validation message | Keep the prior valid policy active; repair candidate policy and retry through the configured reload path |
| State JSON parse/checksum error | Preserve both state and log; inspect copies/read-only diagnostics | Follow the workspace recovery procedure below; do not hand-edit state |
| Empty query candidates | Initialization, policy status, unsafe-query flag, workspace content, implementation limits | Capture the full result and logs; reproduce with focused tests before changing stored data |
| RocksDB open or write error | Build feature, DB path permissions, free disk, RocksDB status | Stop writes if persistence health is uncertain; preserve the DB directory before repair |

## Workspace state recovery

`WorkspaceStateManager::load()` attempts reconstruction from the last valid
JSON entry in `wiki/state.log` after checksum mismatch or a state
deserialization failure. A JSON parse error returns `InvalidJson` directly and
does not automatically invoke log recovery. Before recovery:

1. Stop or quiesce writers so the state file and log are not changing.
2. Take a filesystem-level backup of the complete `wiki/` directory.
3. For an automatic recovery case, run the state-manager load path in a
   controlled environment using the backup or a copy. For invalid JSON, use
   the explicit `recoverFromLog()` operation only after review; do not treat
   that manual step as automatic load behavior.
4. Check the returned status and compare recovered version, links, and tasks
   with the last known good backup or audit evidence.
5. Restore only after maintainer review; retain the original files for
   incident analysis.

The stored state checksum is currently computed with 64-bit FNV-1a despite
the `sha256:` label in source. It is not a cryptographic integrity guarantee.
If integrity or tampering is suspected, do not treat successful checksum
validation as sufficient evidence.

## Rollback and escalation

- Roll back a policy change by restoring the last reviewed policy file and
  reloading it through the normal validation path. The implementation retains
  the last in-memory policy when hot reload rejects a candidate.
- Roll back a plugin release using the deployment's normal artifact rollback
  procedure; preserve workspace data and verify schema compatibility before
  reusing it.
- Escalate suspected cross-workspace exposure, policy bypass, corrupted
  persisted data, or discrepancy between interface promises and deployed
  implementation to the LLM Wiki maintainer and security/on-call owner.
- Attach build identity, sanitized logs, policy version, workspace backup
  location, and focused reproduction. Exclude credentials and sensitive wiki
  content from tickets.

## Related documents

- [`OPERATIONS.md`](OPERATIONS.md) — routine health and maintenance
- [`TRUST.md`](TRUST.md) — security assumptions and incident boundaries
- [`IMPLEMENTATION.md`](IMPLEMENTATION.md) — source-level failure behavior

> Exact model backend not verifiable from final document text alone.
