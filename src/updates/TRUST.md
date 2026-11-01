---
Author: "ThemisDB Team"
Created: "2026-10-31"
Last Updated: "2026-11-01"
Status: active
AI_Used: true
AI_Models: unknown
AI_Contributor: GitHub Copilot
Human_Review: false
Human_Author: "ThemisDB Team"
---

# Updates Module Trust Boundaries

This page describes which update inputs and decisions must be trusted, where
integrators remain responsible, and how to respond to integrity incidents.
It complements [`SECURITY.md`](SECURITY.md); public interface details are in
[`API.md`](API.md).

## Trust boundaries

| Boundary | Trust assumption and control |
|---|---|
| Release metadata and file assets | Treat downloaded manifest fields, paths, hashes, signatures, and patch bytes as untrusted. `ManifestDatabase::verifyManifest()` requires a matching hash, configured verifier, non-empty signature and certificate, a certificate chain accepted by the system trust store, an issuer DN allowed by `PluginSecurityPolicy::trustedIssuers`, and a valid signature. Missing verifier/trust material or any mismatch rejects the manifest. |
| Delta patch application | `DeltaUpdateEngine` validates relative paths and checks base/target hashes and output size when present. Require complete, trusted manifest values; preserve atomic replacement and reject malformed paths or mismatched content. |
| Build identity | `verifyBuildSignature()` verifies the embedded compile-time Ed25519 signature when signature material and OpenSSL support are available. Community builds have no embedded signature; a missing signature or unavailable OpenSSL yields `verified == false`. `buildChannel()` alone is not verification. |
| Update initiators and policy | Callers own authentication, authorization, approval, tenant isolation, and admission control. The update library does not make an authenticated API request an authorized deployment decision. |
| Local files and logs | Install, staging, backup, manifest, state-history, and checkpoint paths require filesystem permissions and retention protections appropriate to their effect. Update records may contain versions, paths, node IDs, and failure context. |
| External services/configuration | API tokens, webhook URLs, proxies, and telemetry endpoints are configuration inputs. Restrict configuration access, redact secrets from diagnostics, and allow outbound telemetry only under the deployment's consent/network policy. |

## Security controls and limits

The module offers state-transition guards, mandatory manifest signature
verification, file hash checks, path checks for deltas, backup/rollback hooks,
and structured failure reporting. Unsigned-update overrides are unsupported;
`HotReloadEngine::Config::verify_signatures=false` fails the download rather
than bypassing verification. The signature cache is not consulted to authorize
manifests. Do not treat a parsed manifest, a `community` channel label, an
operator-diagnostics classification, or a successful state transition as
standalone proof of artifact authenticity.

The frozen contract in `updates_api_contract.h` uses a different
`VerificationResult` type from the hot-reload header and from
`BuildVerificationResult`; evaluate the specific verifier used by the code
path rather than assuming these result types share semantics.

No trust boundary should suppress a verification error or turn a missing
rollback target into success. Preserve enough sanitized evidence to reconstruct
the affected version and transition sequence without logging credentials or
unnecessary payload data.

## Incident handling

For a checksum, signature, unsafe-path, or manifest-integrity failure, stop
the rollout, quarantine the suspect artifact, preserve redacted logs, and
obtain release files again from the deployment's approved source. Do not retry
the same bytes or disable verification as a workaround. For an authorization
or secret-exposure incident, revoke/rotate the affected credential at its
owning service and review access to update configuration and logs.

Keep affected nodes isolated if update state or rollback integrity is
uncertain. Escalate data-loss risk, signature mismatch, corrupted state
history, or partial migration to the security and platform on-call owners.
Record the artifact/version, verifier result, affected nodes, logs, containment,
and recovery verification while excluding secret values.

Canonical implementation references:
[`build_verifier.cpp`](build_verifier.cpp),
[`delta_update_engine.cpp`](delta_update_engine.cpp),
[`manifest_database.cpp`](manifest_database.cpp),
[`hot_reload_engine.cpp`](hot_reload_engine.cpp),
[`updates_config.cpp`](updates_config.cpp),
[`update_state_machine.cpp`](update_state_machine.cpp), and
[`PRODUCTION_REQUIREMENTS.md`](PRODUCTION_REQUIREMENTS.md).

> Exact model backend not verifiable from final document text alone.
