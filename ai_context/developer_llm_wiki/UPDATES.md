---
Author: "ThemisDB Team"
Created: "2026-10-31"
Last Updated: "2026-11-01"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
Source_Level: "level1"
SOT_Domain: "module-behavior"
Canonical_References: "include/updates/update_state_machine.h; include/updates/updates_api_contract.h; include/updates/release_manifest.h; include/updates/delta_update_engine.h; src/updates/README.md; src/updates/ARCHITECTURE.md; src/updates/API.md; src/updates/IMPLEMENTATION.md; src/updates/OPERATIONS.md; src/updates/TRUST.md; tests/updates/CMakeLists.txt"
source: "src/updates/README.md; include/updates/update_state_machine.h; include/updates/delta_update_engine.h; tests/updates/CMakeLists.txt"
source_class: "secondary"
source_weight: 60
---

> Exact model backend not verifiable from final document text alone.

# Updates — Developer LLM Wiki

Curated orientation for coding and review work in `src/updates`. This is a
secondary summary, not a replacement for the public headers, implementation,
or focused tests. Root governance takes precedence over module-local material;
module headers and source establish the runtime contract.

## Module shape

The updates module owns release-manifest handling, update-state tracking,
delta patching, hot reload, schema migration, rollout controllers, tenant and
cluster scheduling surfaces, and structured diagnostics. Public APIs are in
[`include/updates/`](../../include/updates/); implementations are in
[`src/updates/`](../../src/updates/); focused tests are grouped under
[`tests/updates/`](../../tests/updates/).

Use the [module README](../../src/updates/README.md) for navigation and
[`ARCHITECTURE.md`](../../src/updates/ARCHITECTURE.md) for ownership and failure
planes. Detailed engineering references include the
[API guide](../../src/updates/API.md),
[implementation guide](../../src/updates/IMPLEMENTATION.md),
[state and rollout model](../../src/updates/MODELING.md),
[operations guide](../../src/updates/OPERATIONS.md),
[incident runbook](../../src/updates/RUNBOOK.md), and
[trust boundaries](../../src/updates/TRUST.md).

## Contracts and boundaries

- `UpdateStateMachine` guards a defined lifecycle. Invalid edges return false;
  `FAILED` requires explicit reset. Its optional JSON-lines transaction log can
  reveal an interrupted update, but that signal is not an automatic recovery.
  Checkpoint snapshots are not substitutes for installed-file backups.
- `ReleaseManifest` and `ReleaseFile` expose JSON conversion and manifest hash
  calculation. `ManifestDatabase::verifyManifest()` rejects a missing verifier,
  signature, or certificate; it requires a system-trusted certificate chain,
  an issuer allowed by verifier policy, a matching payload hash, and a valid
  signature. Parsed fields and cached verification results are not proof of
  authenticity.
- `DeltaUpdateEngine` checks relative paths and, when supplied, base/target
  hashes and target size. Dependency ordering is applied only when
  `DeltaManifest::enforce_order` is set. The engine is not thread-safe, and
  callers fetch patch assets before apply.
- `HotReloadEngine` owns file-level download/apply and rollback behavior.
  Signature verification is mandatory; `verify_signatures=false` rejects a
  download and no unsigned-update override is supported. Configuration also
  controls backups, dry-run, and post-update health-check rollback. Callers
  must inspect results and retain rollback IDs.
- Canary membership is deterministic from version and node ID, but the
  controller does not coordinate nodes. Blue/green deployment tracks local
  active/standby slots. Multi-node consistency belongs to cluster/coordinated
  managers and their caller-supplied topology.
- The update library does not replace caller-side authentication,
  authorization, or approval policy. Keep credentials out of diagnostics and
  stored runbook evidence.

See the canonical interfaces:
[`update_state_machine.h`](../../include/updates/update_state_machine.h),
[`updates_api_contract.h`](../../include/updates/updates_api_contract.h),
[`release_manifest.h`](../../include/updates/release_manifest.h),
[`delta_update_engine.h`](../../include/updates/delta_update_engine.h),
[`hot_reload_engine.h`](../../include/updates/hot_reload_engine.h),
[`canary_rollout.h`](../../include/updates/canary_rollout.h), and
[`blue_green_deployment.h`](../../include/updates/blue_green_deployment.h).

## Failure and diagnostic model

`DiagnosticErrorCode` reserves 7400–7499 for state, rollback, patch,
coordination, and miscellaneous update failures. `ErrorContext` carries code,
severity, root cause, operation, phase, node, version, and extra context.
`OperatorDiagnostics` classifies contexts and supplies suggested alerting
rules and recovery procedures; those recommendations are not proof that the
underlying system has recovered.

For checksum/signature failures, unsafe paths, partial migration, rollback
ambiguity, quorum loss, or data-loss risk, stop rollout progression and follow
the deployment's control-plane runbook. Do not retry untrusted artifacts or
allow isolated nodes to advance independently. See
[`updates_diagnostics.h`](../../include/updates/updates_diagnostics.h),
[`updates_operator_diagnostics.h`](../../include/updates/updates_operator_diagnostics.h),
and the module [trust guide](../../src/updates/TRUST.md).

## Tests and evaluation

Focused source suites under `tests/updates/` cover contract hardening,
rollback, edge-case determinism, operator diagnostics, cluster scheduling,
and performance. Registration and CTest naming are defined in
[`tests/updates/CMakeLists.txt`](../../tests/updates/CMakeLists.txt); confirm
the configured build tree's targets before running a test. Benchmarks and
their expectations are mapped in
[`PERFORMANCE_EXPECTATIONS.md`](../../src/updates/PERFORMANCE_EXPECTATIONS.md).
Declared thresholds are evaluation criteria, not evidence of a current
representative-hardware run.

## Source priority and freshness

For source-backed decisions, prefer the public header and implementation for
the exact path, then current module `ROADMAP.md`, `README.md`, and architecture
contracts, then this summary. Historical evidence in module audit/evidence
files is not a substitute for a current test or benchmark run. If this page
drifts, correct it from source; never copy roadmap status claims into the
developer wiki as independent release authority.
