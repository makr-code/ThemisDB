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

# Updates Module Guide

This guide helps developers locate the right layer when implementing or
integrating a ThemisDB update workflow. Begin with the module
[`README.md`](README.md), then use the specific contract below rather than
depending on internal implementation details.

## Choose the right component

| Need | Start here |
|---|---|
| Guard update lifecycle and inspect in-flight work | [`update_state_machine.h`](../../include/updates/update_state_machine.h) |
| Describe a release and its files | [`release_manifest.h`](../../include/updates/release_manifest.h) |
| Generate/apply patches between releases | [`delta_update_engine.h`](../../include/updates/delta_update_engine.h) |
| Download, verify, apply, and roll back an update | [`hot_reload_engine.h`](../../include/updates/hot_reload_engine.h) |
| Progressively enable a release on nodes | [`canary_rollout.h`](../../include/updates/canary_rollout.h) |
| Deploy to standby and promote/rollback slots | [`blue_green_deployment.h`](../../include/updates/blue_green_deployment.h) |
| Configure update behavior | [`updates_config.h`](../../include/updates/updates_config.h) |
| Emit or interpret structured update failures | [`updates_diagnostic_emitter.h`](../../include/updates/updates_diagnostic_emitter.h), [`updates_diagnostics.h`](../../include/updates/updates_diagnostics.h), [`updates_operator_diagnostics.h`](../../include/updates/updates_operator_diagnostics.h) |

## Safe integration sequence

1. Select the owning controller and configure directories, verification,
   backup, and approval behavior explicitly for the deployment.
2. Check manifest/download/patch results before applying. Preserve the
   rollback identifier returned by a successful hot reload.
3. Use a single `DeltaUpdateEngine` instance sequentially; its public header
   does not promise thread safety.
4. For canary, set a stable node ID and configure ordered stages ending at
   100%. Do not assume a local controller coordinates peers.
5. For blue/green, deploy to standby and validate it before promotion. Rollback
   requires a usable rollback point.
6. Emit structured context for failed operations. Treat unclassified
   diagnostics as actionable; `OperatorDiagnostics` defaults unknown patterns
   to `DEADLOCK_RACE_CONDITION`, which is a classification fallback, not proof
   of a deadlock.

## Testing and review

The test fixtures are grouped under [`tests/updates`](../../tests/updates/);
module CMake registration is in
[`tests/updates/CMakeLists.txt`](../../tests/updates/CMakeLists.txt). Focus the
test run on the component changed, then cover interactions where a controller
crosses a boundary (for example canary-to-hot-reload or state-machine-to-
rollback). Tests should assert both the success result and the failure result
for invalid input, missing state, and partial operations.

For architecture and failure ownership, see
[`ARCHITECTURE.md`](ARCHITECTURE.md). For operator response, use
[`RUNBOOK.md`](RUNBOOK.md), not assumptions about a CLI that is not part of
these C++ headers.

> Exact model backend not verifiable from final document text alone.
