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

# Updates Module API

This is the integration-oriented index of the C++ contracts declared in
[`include/updates`](../../include/updates/). Header comments are normative for
signature, ownership, and thread-safety details; this page summarizes the
entry points and edge cases most relevant to callers.

## Core interfaces

| Surface | Contract and caller responsibility |
|---|---|
| `UpdateStateMachine` | `transition()` returns false for an invalid edge. `FAILED` requires explicit `reset()`. Optional transaction and checkpoint logs are configured at construction. State callbacks are invoked after releasing the internal lock. See [`update_state_machine.h`](../../include/updates/update_state_machine.h). |
| `ReleaseManifest` / `ReleaseFile` | Convert between structured fields and JSON with `toJson()` / `fromJson()`; parsing returns `std::optional`. `calculateHash()` excludes signature fields. See [`release_manifest.h`](../../include/updates/release_manifest.h). |
| `DeltaUpdateEngine` | `findDelta()` may return no manifest; `applyDelta()` returns per-file successes/fallbacks and an overall status. Instance is not thread-safe. Callers fetch patch assets before apply. See [`delta_update_engine.h`](../../include/updates/delta_update_engine.h). |
| `HotReloadEngine` | Downloads/verifies releases, applies file updates, exposes rollback IDs, and can invoke a post-update health check. `verify_signatures`, backup, dry-run, and health-check rollback are explicit configuration choices. See [`hot_reload_engine.h`](../../include/updates/hot_reload_engine.h). |
| `CanaryRollout` / `BlueGreenDeployment` | Wrap a hot-reload engine for local progressive or slot-based deployments. Canary membership is deterministic per version/node ID; neither controller supplies cross-node coordination. See their respective headers. |
| `UpdatesConfig` | YAML/JSON conversion for checker, auto-update, hot reload, notifications, canary, and telemetry. A failed YAML load returns defaults after logging; validate configuration at the application boundary. See [`updates_config.h`](../../include/updates/updates_config.h). |
| Diagnostics | `DiagnosticEmitter` distributes structured `ErrorContext`; `OperatorDiagnostics` classifies scenarios and returns alert/recovery descriptions. See [`updates_diagnostic_emitter.h`](../../include/updates/updates_diagnostic_emitter.h) and [`updates_operator_diagnostics.h`](../../include/updates/updates_operator_diagnostics.h). |
| Build identity | `verifyBuildSignature()` returns a cached result; callers must inspect `verified`. `buildChannel()` is descriptive and is not proof of signature validity. See [`build_verifier.h`](../../include/updates/build_verifier.h). |

## Error and compatibility contracts

`UpdatesError` in `updates_api_contract.h` defines stable contract codes
7400–7405 for no rollback target, checksum mismatch, missing artifact,
incompatible patch, switch in progress, and aborted canary. The broader
`DiagnosticErrorCode` taxonomy in `updates_diagnostics.h` also uses the
reserved 7400–7499 range. Do not confuse the small frozen API enum with the
diagnostic taxonomy or the similarly named result structures in other headers.

Manifest parsers and configuration parsers may reject malformed input or
return defaults; update callers must check optional/result status and surface
verification errors. A false result, empty rollback ID, or missing delta is
not a successful update.

## Verification

The focused contract tests are
[`test_updates_contract_hardening_focused.cpp`](../../tests/updates/test_updates_contract_hardening_focused.cpp),
[`test_updates_rollback_hardening_focused.cpp`](../../tests/updates/test_updates_rollback_hardening_focused.cpp),
and [`test_updates_operator_diagnostics_focused.cpp`](../../tests/updates/test_updates_operator_diagnostics_focused.cpp).
Public API changes also require updating the Doxygen declaration comments and
the related integration/runbook contract.

> Exact model backend not verifiable from final document text alone.
