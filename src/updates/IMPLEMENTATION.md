---
Author: "ThemisDB Team"
Created: "2026-10-31"
Last Updated: "2026-10-31"
Status: active
AI_Used: true
AI_Models: unknown
AI_Contributor: GitHub Copilot
Human_Review: false
Human_Author: "ThemisDB Team"
---

# Updates Module Implementation Notes

For contributors changing update behavior. Public interfaces are in
[`include/updates`](../../include/updates/); implementation is in this
directory. Preserve the subsystem boundaries in
[`ARCHITECTURE.md`](ARCHITECTURE.md).

## Execution path

An update controller obtains release information through the manifest/update
checker boundary, validates release and file data, optionally downloads
patches, applies the update through `HotReloadEngine`, and records state,
history, or diagnostics. Higher-level canary, blue/green, cluster, and tenant
controllers decide when to apply and how to react to health or coordination
results. These paths are separate contracts; changing one does not implicitly
change the others.

`UpdateStateMachine::transition` is the authoritative guard for the state
sequence. It validates the edge under a mutex, records a transaction, updates
the current version, and invokes registered callbacks after unlocking. Keep
callbacks outside the critical section so callback code can query the machine
without deadlocking. An update failure must remain distinguishable from a
valid transition; do not bypass the state guard to force progress.

## Artifact and patch invariants

- Treat manifest fields and patch paths as input. Delta application rejects
  unsafe relative paths and checks base/target hashes and target size when
  supplied by the manifest.
- Patch dependencies are topologically ordered only when
  `DeltaManifest::enforce_order` is enabled. Missing dependencies and cycles
  fail that ordered apply; without the flag, manifest order is used.
- A per-file delta failure may be reported for full-file fallback; callers must
  inspect the aggregate `DeltaApplyResult` and not report success without
  confirming all required files are present.
- Replacement uses an atomic-write helper. Do not weaken path validation,
  integrity checks, or failure propagation as an optimization.

See [`delta_update_engine.h`](../../include/updates/delta_update_engine.h) and
[`delta_update_engine.cpp`](delta_update_engine.cpp).

## Configuration and error handling

`UpdatesConfig` loads YAML or JSON and has disabled-by-default update,
hot-reload, notification, canary, and telemetry toggles. A YAML load exception
is logged and returns a default configuration; callers must not interpret that
fallback as successful loading of their intended production settings.
Telemetry configuration clamps its send interval to at least 86,400 seconds.

Use the structured `DiagnosticErrorCode`, severity, root-cause, and
`ErrorContext` types for failures crossing component boundaries. Error codes in
the 7400–7499 range are reserved for this module. Preserve operation, phase,
version, and node context where known; avoid placing credentials or
unnecessary sensitive data into messages or `extra_context`.

## Tests and changes

Add regression coverage alongside the affected behavior:

- transitions and restart-history cases: `test_updates_determinism_edge_cases_focused.cpp`;
- delta and migration cases: `test_updates_phase2_phase3_hardening_focused.cpp`;
- rollback and coordination: `test_updates_rollback_hardening_focused.cpp`;
- diagnostic contracts: `test_updates_operator_diagnostics_focused.cpp`.

These are source paths, not a guarantee of current build artifacts. Confirm
the configured test target and run the narrow focused suite after a code
change. When modifying a public header, update [`API.md`](API.md) and document
ownership, threading, return values, and failure behavior in the declaration.

> Exact model backend not verifiable from final document text alone.
