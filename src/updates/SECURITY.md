# Security - Updates Module

<!-- Status: current | validated: 2026-05-31 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md -->

Report vulnerabilities via project-level SECURITY.md.

## Security Scope

Security in the updates module focuses on deterministic update-state behavior, explicit manifest and patch validation, bounded migration and rollout control, and observable rollback/preflight failure signaling.

## Threat Model

| Threat | Current Mitigation Surface |
|---|---|
| unsafe or inconsistent state transitions | explicit update state machine and rollback behavior |
| tampered or malformed manifests/patches | deterministic manifest round-trip and delta pipeline behavior |
| opaque migration failure during rollout | diagnosable migration and preflight health-check behavior |
| hidden rollout regression | explicit canary/blue-green and history logging behavior |

## Implemented Security Controls

- update-state and rollback paths expose explicit outcomes.
- manifest and patch pipeline behavior remains observable.
- migration and dependency faults remain diagnosable.
- rollout/preflight failures are explicit and bounded.

## Security Follow-ups

- broaden fault-injection coverage for rollback, manifest, and patch edge cases.
- deepen stress coverage for coordinated cluster and tenant update scenarios.
- tighten diagnostics taxonomy across update, migration, and rollout incidents.

## Sourcecode Verification (Module: updates/security)

- Verified files:
  - src/updates/update_state_machine.cpp
  - src/updates/release_manifest.cpp
  - src/updates/delta_update_engine.cpp
  - src/updates/preflight_health_check.cpp
  - src/updates/canary_rollout.cpp
  - src/updates/update_history_logger.cpp
- Verified controls:
  - explicit state/rollback fault signaling
  - observable manifest and patch behavior
  - deterministic rollout and preflight outcomes