---
Author: "ThemisDB Contributors"
Created: "2026-05-31"
Last Updated: "2026-10-31"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot coding agent"
Human_Review: false
Human_Author: "ThemisDB Contributors"
---

Exact model backend not verifiable from final document text alone.

# Security - Base Module

Report vulnerabilities via project-level SECURITY.md.

## Security Scope

Security-related base APIs cover module verification, artifact integrity, optional execution controls, and reload behavior. Their presence does not establish process isolation or guarantee that every check is enabled by every caller/build.

## Threat Model

| Threat | Current Mitigation Surface |
|---|---|
| untrusted or tampered modules | configured loader verification and integrity checks; behavior depends on policy/configuration |
| dependency abuse and unsafe activation order | dependency graph analysis is available; callers must use it where required |
| plugin escape or resource abuse | optional, platform-dependent sandbox and WASM controls |
| unsafe remote module intake | URL-scheme validation, optional auth headers, and hash comparison when an expected hash is supplied; HTTP is accepted with a warning |
| reload-induced instability | header contract specifies an atomic swap, but the inspected implementation sequence appears inconsistent; reconcile before promising uninterrupted availability |

## Implemented Security Controls

- loader checks depend on the configured verification path; do not assume every optional check is mandatory in every configuration.
- sandbox/WASM controls are optional and have platform/runtime limitations.
- The normal `ModuleLoader` path delegates through `ModuleSecurityVerifier` to `PluginSecurityVerifier::verifyPlugin()`. It checks file/hash and metadata/signature/trust conditions; `setHashManifest()` can additionally enable SHA-256 comparison for module names present in a loaded manifest. The normal path does not call the CRL/OCSP methods. Those calls appear in the separate `EnhancedPluginSecurityVerifier` full-chain path when requested with certificate metadata and revocation checking enabled. Do not claim revocation enforcement for every module load.
- The separate `PluginBundleLoader` verifies an Ed25519 manifest signature when a public key is configured. Without a key, the implementation rejects by default and permits unsigned bundles only after explicit opt-in. The class overview comment in the header says verification is skipped when no key is configured, which conflicts with the setter documentation and implementation; reconcile that header wording.
- `module_security.cpp` changes a local policy after constructing `PluginSecurityVerifier` with a copy and does not update that copy in the constructor. Effective compile-mode defaults should not be inferred from those local assignments alone.
- The canonical loader defines certificate-revoked/expired error values and verification-failure accounting; these are error/reporting paths, not evidence that the normal load path performs revocation checks.
- `ModuleSignatureVerifier` is a separate platform-signature helper, not evidence that the normal `ModuleLoader` path uses Authenticode/GPG; its Windows implementation sets `WTD_REVOKE_NONE`.
- `PluginDependencyGraph` exposes cycle/order analysis; it is not shown to be automatically consulted by every loader path.
- registry intake parses response/download data and compares SHA-256 when the
  registry entry supplies an expected hash; auth headers are optional and
  plaintext HTTP remains accepted.
- `RegistryConfig` exposes `auth_token` and `api_key` headers, TLS verification (enabled by default), optional CA bundle and public-key pin. HTTP remains accepted, and the base API does not provide credential storage or rotation.

## Security Follow-ups

- continue hardening concrete runtime backends for wasm and platform edges.
- keep reload and dependency failure paths deterministic under load.
- maintain testable diagnostics for module trust and activation failures.

## Sourcecode Verification (Module: base/security)

- Verified files:
  - src/themis/module_loader.cpp (canonical loader)
  - src/themis/module_security.cpp (ModuleLoader verifier adapter)
  - src/acceleration/plugin_security.cpp (basic and enhanced verifier implementation)
  - src/themis/module_signature_verifier.cpp (separate platform-signature helper)
  - src/base/module_loader.cpp (deprecated legacy copy; not canonical)
  - src/base/module_sandbox.cpp
  - src/base/wasm_plugin_sandbox.cpp
  - src/base/wasm_runtime_injector.cpp
  - src/base/plugin_dependency_graph.cpp
  - src/base/remote_registry_client.cpp
  - src/base/hot_reload_manager.cpp
- Verified controls:
  - loader hash/metadata/signature/trust checks, optional hash-manifest comparison, and bundle-signature path; effective policy and revocation enforcement are not universal
  - resource limits are best-effort/platform-dependent and apply to the current process; network/filesystem/syscall restrictions can be skipped with warnings
  - reload and registry intake result/error paths; the reload header's atomic-swap contract and implementation sequence need reconciliation, and TLS depends on configuration