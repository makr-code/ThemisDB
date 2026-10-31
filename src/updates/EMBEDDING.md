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

# Updates Module Integration Boundaries

Here, “embedding” means how the updates subsystem is integrated into the
ThemisDB process and adjacent modules. This module does not implement vector
embeddings or an embedding model. This page is for maintainers wiring update
behavior into a server or deployment controller.

## Ownership and dependencies

The implementation lives in `src/updates`; public C++ declarations live in
[`include/updates`](../../include/updates/). The module owns update-state
tracking, release metadata and patch handling, migration orchestration, rollout
controllers, diagnostics, and update configuration. It does not own general
network clients, server request authorization, storage-engine internals, or
cluster transport.

`HotReloadEngine` accepts a manifest database and an update checker through
shared pointers. `CanaryRollout` and `BlueGreenDeployment` wrap the hot-reload
engine, while coordinated and cluster managers own node-level workflows.
Integrators should pass explicit configuration and ensure required dependency
lifetimes outlive their consumers. The class contracts and ownership notes are
in [`hot_reload_engine.h`](../../include/updates/hot_reload_engine.h),
[`canary_rollout.h`](../../include/updates/canary_rollout.h), and
[`blue_green_deployment.h`](../../include/updates/blue_green_deployment.h).

The server directly consumes selected update APIs, including hot reload,
manifest storage, and diagnostic emission; the module architecture records
these downstream relationships in [`ARCHITECTURE.md`](ARCHITECTURE.md).
Avoid reaching into internal `.cpp` details from another module. Add or use a
public header contract when a cross-module integration point is needed.

## Integration sequence

1. Load and review `UpdatesConfig`; its checker, automatic-update,
   hot-reload, notification, canary, and telemetry options are disabled by
   default unless otherwise set.
2. Construct the manifest/update dependencies and `HotReloadEngine` with
   deployment-specific directories and verification/backup policy.
3. For a progressive rollout, construct a canary or blue/green controller;
   supply a stable node ID for canary membership and retain returned rollback
   IDs.
4. Connect diagnostics through `DiagnosticEmitter` listeners or the
   operator-diagnostics API. Include operation, phase, version, and node context
   when available.
5. Define who may initiate, approve, abort, and roll back updates at the
   caller boundary. The module API does not replace server-side authorization.

## Failure and concurrency boundaries

Do not run concurrent updates through one `DeltaUpdateEngine` instance; its
header explicitly declares it not thread-safe. Canary and blue/green state is
local to each controller and does not provide inter-node consensus. A failed
health check may trigger rollback only when the hot-reload configuration
enables it. Invalid state edges, missing rollback targets, incompatible patch
bases, and coordination failures must be surfaced to the caller rather than
converted into success.

Integration tests should exercise the public headers and supplied fakes or
fixtures, checking dependency lifetime, authorization at the calling layer,
rollback-ID handling, and diagnostic propagation. Keep tests focused on the
boundary being changed; the module test targets are registered from
[`tests/updates/CMakeLists.txt`](../../tests/updates/CMakeLists.txt).

> Exact model backend not verifiable from final document text alone.
