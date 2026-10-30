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

# Core Trust Boundaries

> Exact model backend not verifiable from final document text alone.

## Trust model

Core is an in-process infrastructure boundary. Consumers provide configuration, log messages, trace headers, metric operations, adapter implementations, plugin paths, and security-provider settings. Those inputs must not be treated as trusted merely because they enter through a Core interface.

| Boundary | Risk | Control and remaining limit |
|---|---|---|
| Application code → logger | Caller-controlled messages/fields can contain credentials or personal data. | No automatic redaction is established by `ILogger`; sanitize before logging and restrict log access. |
| Request headers → tracing | Untrusted propagation headers may carry malformed or attacker-selected context. | Tracer adapters parse/extract context. Validate at the request boundary; do not use trace IDs as authorization. |
| Configuration → adapter factories | Invalid names/endpoints or unsafe production defaults can disable or misroute concerns. | `ConfigValidator` rejects unsupported adapter names; production checks reject no-op tracing/metrics. Validate deployment configuration separately. |
| Plugin package → host process | A native plugin executes with host process privileges. | ABI checks and optional SHA-256 digest comparison detect some mismatch/tampering. `kTrustAll` is the registry default; a co-located digest is not publisher identity or a signature rooted in a trusted key. |
| Environment/config → secrets and key providers | Environment/config values can expose credentials if copied into logs or command lines. | Use deployment secret injection and least privilege; never log tokens, pins, key material, or serialized secret configuration. |
| Concern → audit sink | `NoOpAuditLog` provides no durable audit trail. | Select a durable sink when compliance requires it; durability/integrity depend on that adapter and its operational controls. |

## Production constraints

`ProductionMode::isEnabled()` is triggered by the exact values listed in [`PRODUCTION_REQUIREMENTS.md`](PRODUCTION_REQUIREMENTS.md). Production `ConcernsContext` construction requires real tracing and metrics. `SecurityLayerBuilder` rejects local/mock key providers and absent or invalid JWT settings; HSM use additionally requires `THEMIS_HSM_ENABLED=1` and valid PKCS#11 configuration.

These controls do not replace deployment authorization, filesystem permissions, network egress controls, secret rotation, or review of the configured adapter and plugin artifacts.

## Plugin trust controls and residual risk

`AdapterRegistry::loadFromPlugin()` loads a shared library through the platform loader. When `AdapterTrustPolicy::kRequireSignature` is selected, it checks the file's SHA-256 digest against the first line of `<path>.sig` before loading. This is an integrity comparison only: if an attacker can replace both files, the check does not authenticate the publisher. The source path does not implement a public-key signature verification or signer identity/rotation policy. Restrict write access to plugin directories and deployment configuration, distribute artifacts through a trusted channel, and do not load untrusted libraries.

`SignedAdapterValidator` validates SHA-256 of adapter metadata (`id:apiVersion:description`); it is distinct from authenticating a plugin binary and must not be described as doing so.

## Incident handling

If logs/traces contain secrets, plugin integrity is in doubt, or an unexpected adapter is loaded:

1. Contain through the owning service's established traffic/process controls; do not rely on Core to unload native code safely at runtime.
2. Preserve redacted diagnostics and the library/configuration hashes. Do not redistribute exposed secret material.
3. Rotate credentials or keys that may have been disclosed and review access to logs, deployment settings, and plugin storage.
4. Restore the last trusted package/configuration using the service rollback process and verify health before resuming traffic.
5. Report suspected vulnerabilities through the project-level [`SECURITY.md`](../../SECURITY.md) process and record the incident actions.

See [`API.md`](API.md) for caller obligations and [`RUNBOOK.md`](RUNBOOK.md) for operational recovery.
