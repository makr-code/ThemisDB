<!-- Status: current | validated: 2026-03-22 -->

# Updates Module — Security Reference

## Scope

This document covers security properties exposed through the public headers in
`include/updates/`.  Implementation-layer controls are documented in
`../../src/updates/SECURITY.md`.

## Threat Model

| Threat | Impact | Mitigation |
|---|---|---|
| Malicious update artifact | Code execution / data corruption | `ReleaseManifest` carries SHA-256 digest; `ParallelDownloader` verifies before install |
| Schema migration data loss | Irreversible data destruction | `SchemaMigrationTester` validates on shadow copy; dry-run mandatory before production |
| Canary traffic hijacking | Serving of malicious content | `CanaryConfig` percentage validated; traffic routing signed with cluster key |
| Bypassing preflight gate | Update applied to degraded node | `PreflightHealthCheck` is blocking; `UpdateStateMachine` refuses `APPLY` from `PREFLIGHT_FAILED` state |
| Unauthorised update trigger | Uncontrolled cluster changes | `ClusterUpdateManager` requires authenticated caller token |
| Webhook URL injection | SSRF / data exfiltration | `NotificationWebhook` URL must pass allow-list validation before registration |
| Rollback to vulnerable version | Re-exposure to patched CVE | `UpdateHistoryLogger` records all rollbacks; operator approval required for downgrade |
| Tenant schedule bypass | SLA violation / data leakage | `TenantUpdateScheduler` enforces maintenance windows; cross-tenant schedule mutation rejected |

## Security Controls

- **Artifact integrity** — all artifacts referenced in `ReleaseManifest` carry a
  SHA-256 digest that `ParallelDownloader` verifies after download and before
  any installation step.
- **Shadow migration testing** — `SchemaMigrationTester` runs every migration
  against a transactional shadow copy; the migration is only promoted if the
  tester reports `MigrationResult::SUCCESS`.
- **State machine gating** — `UpdateStateMachine` is the single authoritative FSM;
  callers cannot reach destructive states without passing through all prerequisite
  gates, including `PREFLIGHT_OK`.
- **Append-only audit** — `UpdateHistoryLogger` records are write-once; callers
  cannot update or delete existing entries through the public API.

## Known Limitations

- `NotificationWebhook` URL allow-list validation is caller-supplied; applications
  must configure the allow-list appropriately for their environment.
- `HotReloadEngine` reload of native shared libraries (`.so`/`.dll`) carries
  inherent TOCTOU risk during the file-swap window; callers should ensure the
  update path is writable only by privileged processes.
- `ParallelDownloader` does not currently support end-to-end TLS certificate
  pinning; mutual TLS is recommended at the infrastructure layer for artifact
  delivery.
