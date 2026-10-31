---
Author: "ThemisDB Contributors"
Created: "2026-10-21"
Last Updated: "2026-10-31"
Status: "active"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot coding agent"
Human_Review: false
Human_Author: "ThemisDB Contributors"
---

Exact model backend not verifiable from final document text alone.

# Base Module Trust and Security Boundaries

## Purpose

The base module provides APIs on the native module load/reload path, but it is
not itself a process-isolation boundary. Native modules are loaded into the
ThemisDB process; the controls below are checks and optional mechanisms, not a
guarantee that extensions cannot affect the host.

## Trust boundaries

The module separates trust into four zones:

1. Artifact trust
   - binary existence and file/hash checks,
   - metadata, signature, and trust-level checks in the normal `ModuleLoader` verifier path,
   - policy-dependent behavior; the normal path does not call the implemented CRL/OCSP methods, so revocation enforcement is not established.

2. Compatibility trust
   - ABI major/minor compatibility,
   - required-symbol presence,
   - declared dependency constraints.

3. Execution controls
   - optional resource controls applied to the current process, not a separate process per native module,
   - WASM runtime limits/host-import policy when a runtime is injected,
   - network, filesystem, and syscall restrictions that may be skipped with warnings.

4. Operational trust
   - reload outcome, rollback state, and a possible contract/implementation discrepancy around atomic replacement,
   - rollback availability only when configured and a backup path is retained,
   - registry authentication/checksum checks; HTTP transport is accepted with a warning.

These boundaries are represented by APIs and error codes; their activation and
coverage depend on caller configuration and runtime/build integration. The
mitigations below are available control surfaces, not guarantees that every
deployment enables or invokes them.

## Key controls

### Module verification

`ModuleLoader::loadModule()` calls `ModuleSecurityVerifier`, which delegates to
`PluginSecurityVerifier::verifyPlugin()`. That path calculates/checks hashes,
loads metadata, verifies a supplied signature, and checks trust level according
to policy. The loader also supports an explicitly loaded SHA-256 manifest;
only modules with a matching manifest entry receive that additional comparison.
The separate `PluginBundleLoader` verifies an Ed25519 manifest signature when a
public key is configured; without a key it rejects bundles unless unsigned
bundles are explicitly allowed. Its header overview says verification is
skipped when no key is set, which conflicts with the setter documentation and
implementation; the source and setter describe fail-closed behavior by default.

`module_security.cpp` mutates a local policy after constructing the verifier
with a copy and does not update that copy in its constructor. The initial
verifier copy therefore retains the `PluginSecurityPolicy` struct defaults; the
later `NDEBUG` assignments do not create the intended debug/release split until
a setter updates the verifier. Do not claim that unsigned modules are accepted
by default in development builds. A caller can still change signature/unsigned
settings through the exposed loader setters.

The normal path does not call `checkCRL()` or `checkOCSP()`. Those calls appear
in the separate `EnhancedPluginSecurityVerifier` path only when a caller
requests `LEVEL_4_FULL_CHAIN`, signing-certificate metadata is available, and
revocation checking is enabled in that verifier's policy. A separate
platform-signature helper also exists; its presence does not establish that
`ModuleLoader` invokes it. The canonical loader has revoked/expired certificate
error values and verification-failure accounting, but those definitions alone
do not establish that the normal path performs CRL/OCSP checks.

### ABI validation

`AbiChecker` can validate configured required-symbol sets and ABI compatibility;
whether a caller invokes it is part of the integration path. This is an important trust control because a module with wrong symbol contracts can destabilize the host or behave unpredictably under execution.

### Dependency validation

`PluginDependencyGraph` analyzes cycle/order constraints. The loader's use of
this separate API is not automatic in every path; callers must ensure graph
validation where their integration requires it.

### Sandbox and WASM constraints

`ModuleSandbox` operates on the current process: Linux cgroup v2 moves the
current PID, fallback `RLIMIT` controls also affect that process, and Windows
uses a Job Object for the current process. This does not isolate a native
plugin from the host or other in-process modules. cgroup/job-object setup and
limits are platform/privilege dependent; fallback limits are not equivalent to
cgroup resource controls. Network, filesystem, and syscall restrictions are
currently skipped with warnings in the inspected implementation. WASM runtime
limits and import policy apply only when WASM isolation is configured and a
runtime is injected.

`HotReloadManager::Config::sandboxConfig` is optional and empty by default; the
manager does not create its `ModuleSandbox` unless the caller supplies it.

### Registry integrity

`RemoteRegistryClient` exposes `auth_token`/`api_key`, `verify_ssl` (default
true), optional CA-bundle and public-key-pin settings, and SHA-256 checks for a
provided registry hash (the comparison is conditional on the entry supplying
that hash). Credentials are sent in request headers; the client
does not provide credential storage or rotation. Its URL validator permits
HTTPS and plaintext HTTP (with a warning); setting `verify_ssl` false disables
certificate/host verification. Transport protection is therefore not
guaranteed by the API.

## Risks and mitigations

### Risk: untrusted or tampered artifact

Mitigation:

- use the active `ModuleLoader` verification path and its configured hash,
  metadata, signature, and trust-level checks;
- treat revocation checking as unverified for the normal loader path; CRL/OCSP
  calls exist in the separate enhanced full-chain verifier;
- do not infer GPG/Authenticode or revocation enforcement from the existence of
  a separate signature helper.

### Risk: incompatible runtime contract

Mitigation:

- ABI and required symbol checks,
- version mismatch rejection,
- dependency graph and ordering validation.

### Risk: dependency poisoning or cycle

Mitigation:

- cycle detection,
- required/optional dependency distinction,
- deterministic topological ordering.

### Risk: resource exhaustion or runaway plugin behavior

Mitigation:

- optional memory/CPU controls may apply to the current host process through
  cgroups, `RLIMIT`, or a Job Object; inspect warnings and do not treat them as
  per-plugin process isolation;
- WASM limits/import restrictions apply only when the corresponding runtime
  path is configured and available;
- network, filesystem, and syscall restrictions are not enforced by the
  inspected `ModuleSandbox` methods; they warn and skip.

### Risk: unsafe live upgrade

Mitigation:

- The public header contracts an atomic swap: the old handle remains until the
  new module is initialized, and failure keeps the old module running. The
  inspected implementation first loads a candidate, then unloads the old
  module, unloads the candidate entry, and loads the candidate again under the
  canonical name. The final-load failure path attempts to restore the prior
  path. Treat the difference as a contract/implementation discrepancy; do not
  promise uninterrupted availability until it is reconciled;
- rollback is available only when enabled and a backup path was retained.
  `rollback()` unloads the current module before loading the backup;
- candidates pass through the registered `ModuleLoader`; the
  `HotReloadManager::Config::verifySignature` field is not read in the
  inspected implementation and is not an independent verification guarantee.

## Incident handling

A trust incident in the base module should be treated as a runtime safety event:

1. identify which boundary failed — artifact, compatibility, dependency, sandbox, or rollout,
2. map the failure to the taxonomy code,
3. use the deployment's supported disable/quarantine mechanism if the module
   cannot be safely revalidated (no universal operator CLI is defined here),
4. request rollback only when the result/configuration indicates a retained
   backup; rollback itself unloads before reload,
5. inspect registry, trust policy, and dependency configuration before re-enabling the artifact.

## Canonical sources

- [`module_loader.h`](../../include/themis/base/module_loader.h),
  [`module_loader.cpp`](../themis/module_loader.cpp), and
  [`module_security.cpp`](../themis/module_security.cpp)
- [`plugin_security.h`](../../include/acceleration/plugin_security.h) and
  [`plugin_security.cpp`](../../src/acceleration/plugin_security.cpp)
- [`module_signature_verifier.cpp`](../themis/module_signature_verifier.cpp)
- [`module_sandbox.h`](../../include/themis/base/module_sandbox.h) and
  [`module_sandbox.cpp`](module_sandbox.cpp)
- [`hot_reload_manager.h`](../../include/themis/base/hot_reload_manager.h) and
  [`hot_reload_manager.cpp`](hot_reload_manager.cpp)
- [`remote_registry_client.h`](../../include/themis/base/remote_registry_client.h)
  and [`remote_registry_client.cpp`](remote_registry_client.cpp)

## Security posture

The source exposes explicit validation and error paths, but does not support a
universal “fail closed” or rollback guarantee across all optional configuration
and platform paths. Deployment owners must verify the enabled controls and
transport policy for their build.
