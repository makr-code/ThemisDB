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

# Plugin production controls: implementation facts and policy gaps

This page records controls visible in the plugin source. It does **not** create
or certify a binding production policy. The source does not establish a
module-wide startup configuration schema, a required production environment
variable, or a complete deployment security checklist. Project-level policy
and release posture remain governed by [`../../SECURITY.md`](../../SECURITY.md)
and [`../../ROADMAP.md`](../../ROADMAP.md).

## Source-visible behavior

- **Native loading:** native libraries are loaded into the ThemisDB process.
  The plugin module does not provide process isolation.
- **Binary verification:** `PluginManager` configures the acceleration
  `PluginSecurityVerifier` to require signatures when `NDEBUG` is defined and
  permits unsigned plugins in non-`NDEBUG` builds. Verify the verifier's
  signature semantics and deployment policy independently.
- **Manifest `.sig`:** in `NDEBUG` builds, the manager requires
  `<manifest>.sig` and compares its first line to the SHA-256 hash of the
  manifest. This is a hash comparison, not public-key publisher authentication.
  `THEMIS_TEST_MODE` skips this check. See [`TRUST.md`](./TRUST.md).
- **OCI:** registry credentials are optional in the client API; anonymous
  access is possible. The client enables TLS peer/hostname checks and verifies
  the downloaded plugin-layer digest. A digest does not establish publisher
  identity. Its default request timeout is 30 seconds; the interface does not
  define a separate connect-timeout default.
- **Signed catalog:** `SignedPluginRepository` uses active pinned Ed25519 keys,
  but it is a separate API and is not automatically applied to every manager
  load path.
- **WASM:** the loader checks an expected SHA-256 and the `enterprise_plugins`
  license gate. The host can register a `WasmPluginLoadFn`; built-in runtime
  code depends on build flags. These facts do not establish that every runtime
  executes with a sandbox or equivalent isolation.
- **Observability:** metrics and health state exposed by these classes are
  process-local interfaces; they do not establish durable audit retention.

## Deployment decisions not established by this module

The following requirements need an approved owner/policy and verification at
the system or deployment layer; they are not claimed as enforced by this
module:

- mandatory authenticated OCI access and credential provisioning;
- publisher identity and artifact provenance across every load path;
- native-plugin isolation and resource limits;
- persistent security audit retention and log handling;
- production-specific startup variables, retry policies, and resource quotas.

**TODO / blocking policy gap:** identify the canonical owner and approved
production acceptance criteria for these items before describing them as
mandatory or implemented. Do not infer settings such as
`THEMIS_PRODUCTION_MODE` or `THEMIS_ENVIRONMENT` from this module; their use
was not verified in the plugin implementation.

## Primary evidence

[`plugin_manager.cpp`](./plugin_manager.cpp),
[`oci_registry_client.cpp`](./oci_registry_client.cpp),
[`signed_plugin_repository.cpp`](./signed_plugin_repository.cpp),
[`wasm_plugin_loader.cpp`](./wasm_plugin_loader.cpp),
[`plugin_manager.h`](../../include/plugins/plugin_manager.h), and
[`oci_registry_client.h`](../../include/plugins/oci_registry_client.h).
