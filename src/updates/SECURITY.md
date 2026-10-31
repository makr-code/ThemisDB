---
Author: "ThemisDB Team"
Created: "2026-10-31"
Last Updated: "2026-10-31"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
---

> Exact model backend not verifiable from final document text alone.

# Security - Updates Module

<!-- Status: current | source checked: 2026-10-31 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md -->

Report vulnerabilities via project-level SECURITY.md.

## Security Scope

Security in the updates module focuses on deterministic update-state behavior, explicit manifest and patch validation, bounded migration and rollout control, and observable rollback/preflight failure signaling.

## Threat Model

| Threat | Current Mitigation Surface |
|---|---|
| unsafe or inconsistent state transitions | explicit update state machine and rollback behavior |
| tampered or malformed manifests/patches | signature verification is configurable in `HotReloadEngine`; delta application checks relative paths and supplied hashes/sizes |
| opaque migration failure during rollout | diagnosable migration and preflight health-check behavior |
| hidden rollout regression | explicit canary/blue-green and history logging behavior |

## Implemented Security Controls

- `HotReloadEngine::Config` exposes signature-verification and backup settings;
  configuration alone does not establish that the caller uses a trusted key or
  enables verification.
- Delta application validates relative paths and checks base/target hashes and
  target size when these values are supplied by the manifest.
- Build-signature verification is a separate `BuildVerifier` path. A channel
  name is not proof of verification, and unavailable signature material or
  OpenSSL support does not yield a verified result.
- Caller authentication, authorization, approval policy, filesystem
  permissions, and deployment-wide coordination remain integrator
  responsibilities.

## Security Follow-ups

- broaden fault-injection coverage for rollback, manifest, and patch edge cases.
- deepen stress coverage for coordinated cluster and tenant update scenarios.
- tighten diagnostics taxonomy across update, migration, and rollout incidents.

## Source references checked (2026-10-31)

- Verified files:
  - src/updates/update_state_machine.cpp
  - src/updates/release_manifest.cpp
  - src/updates/delta_update_engine.cpp
  - src/updates/preflight_health_check.cpp
  - src/updates/canary_rollout.cpp
  - src/updates/update_history_logger.cpp
- This is a source-backed threat/control summary, not an independent security
  assessment or proof that a configured deployment enables these controls.