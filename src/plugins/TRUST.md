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

Exact model backend not verifiable from final document text alone.

# Plugin Trust Model

## Purpose and scope

This document identifies the trust boundaries and controls visible in the
plugin runtime source. It is for security reviewers and operators. The
project-level [`SECURITY.md`](../../SECURITY.md) remains the reporting and
incident authority; this page does not certify the module as a sandbox.

## Boundaries and principal risks

| Boundary | Risk | Source-visible control and limit |
|---|---|---|
| Manifest to manager | Malformed names, paths, edition claims, or capability requests influence loading. | Manifest-backed `loadPlugin(name)` validates required fields, restricts names to alphanumeric/underscore/hyphen, checks edition/license policy, and snapshots capabilities. `loadPluginFromPath()` and OCI loading bypass `plugin.json` and do not apply those manifest restrictions. No path makes a plugin implementation safe. |
| Native library to server | A malicious or defective plugin can execute with the host process's privileges or crash/corrupt it. | Library and symbol validation gate activation; the native path has no process sandbox in this module. Use reviewed artifacts and service-level isolation. |
| Manifest `.sig` file to manager | A checksum may be mistaken for publisher authentication. | In `NDEBUG`, `verifyManifestSignature()` compares the first line of `<manifest>.sig` with the manifest SHA-256. That is a hash comparison, not a public-key signature. In development mode unsigned or mismatched manifests are allowed with warnings; `THEMIS_TEST_MODE` skips this check. |
| Signed repository catalog | Untrusted catalogs or keys could introduce unauthorized entries. | `SignedPluginRepository` accepts entries only when the key fingerprint maps to an active pinned Ed25519 key and the signature verifies over canonical manifest JSON. This API is distinct from the manager's `.sig` check; do not assume it is automatically applied to every load path. |
| OCI registry and cache | Registry substitution, credential exposure, interrupted downloads, or corrupt content. | The client enables TLS peer/host verification and verifies the plugin layer's SHA-256 digest before cache placement. Authentication configuration is optional at API level; a digest alone does not prove publisher identity. Protect tokens and cache-directory writes at deployment level. |
| WASM runtime and host imports | Malformed module or excessive host access. | Optional runtime path checks an expected 64-character SHA-256 hash and gates use on `enterprise_plugins`. The module's hash validates content identity only; runtime sandbox settings and host-import permissions must be reviewed for the selected runtime. |
| Edition/private boundary | Restricted plugin or feature becomes available in an unauthorized edition. | Manifest edition and license checks plus focused boundary/edition tests exist. Confirm packaging, manifests, license configuration, and the actual target build; test names alone do not prove release policy is active. |

## Load-path policy distinction

Use the manifest-backed name-based loader when the deployment relies on
`allowed_editions`, `license_feature`, or dependencies. `loadPluginFromPath()`
applies the binary verifier and factory-entry-point checks but receives no
manifest. `loadPluginFromOci()` verifies
the OCI layer digest, then delegates to the same explicit-path loader. Neither
path substitutes for manifest policy or verifies publisher identity from the
OCI digest alone.

## Capability and lifecycle controls

Capabilities declared at registration/load are frozen after initialization.
An explicit escalation check marks the plugin restricted if it later reports a
strict superset. This check is not on every call path. The lifecycle manager
validates state transitions and returns explicit errors for failed load,
reload, and health operations. Reviewers should verify that every integration
path invokes the intended policy before activation.

## Key and secret handling

`OciAuthConfig` contains credentials or a bearer token in memory. Do not place
tokens in manifests, logs, test fixtures, or committed documentation. Supply
them through the deployment's secret-management mechanism and scope each
credential to the registry permissions it needs. Pinned public keys are trust
anchors: provisioning, rotation, revocation, and backup are responsibilities
of the operator/key-management process.

The module source does not establish durable security-audit retention. Preserve
deployment logs and provenance evidence in the owning operations system.

## Incident handling

For a suspected malicious or compromised plugin: stop new discovery or
rollouts, isolate the server if native code may still execute, preserve the
manifest/binary/hash/signature/catalog evidence, and notify the security
response owner. Revoke or rotate a pinned key only through approved key
management, then replace the artifact from a verified source. Do not rely on
the plugin manager's unload operation to contain code already executing in the
host process. Follow [`RUNBOOK.md`](./RUNBOOK.md) for lifecycle triage and
[`OPERATIONS.md`](./OPERATIONS.md) for deployment controls.

## Primary evidence

[`plugin_manager.cpp`](./plugin_manager.cpp),
[`signed_plugin_repository.cpp`](./signed_plugin_repository.cpp),
[`oci_registry_client.cpp`](./oci_registry_client.cpp),
[`wasm_plugin_loader.cpp`](./wasm_plugin_loader.cpp),
[`plugin_interface.h`](../../include/plugins/plugin_interface.h),
[`signed_plugin_repository.h`](../../include/plugins/signed_plugin_repository.h),
and [`wasm_host_api.h`](../../include/plugins/wasm_host_api.h).
