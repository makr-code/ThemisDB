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

# Base Module Operations Guide

## Mission

The base module exposes lifecycle, registry, reload, and optional sandbox APIs.
It is not by itself a complete execution-safety boundary: native modules run in
the host process, and several sandbox restrictions are currently skipped with
warnings.

## Primary operational surfaces

The relevant runtime artifacts are:

- `ModuleLoader` — configured lifecycle and verification steps
- `ModuleSandbox` — optional, platform-dependent resource/isolation controls
- `HotReloadManager` — reload result and configured rollback behavior
- `PluginDependencyGraph` — caller-invoked dependency analysis and ordering
- `RemoteRegistryClient` — registry-driven artifact acquisition; HTTP URLs are accepted with a warning
- `ABTestManager` — experiment state and promotion/rollback API

`ModuleSandbox` resource controls act on the current process, not a separate
process per native module. Linux cgroup/`RLIMIT` and Windows Job Object behavior
depends on platform support and setup. Network, filesystem, and syscall
restrictions may be skipped. Check `launchWarnings()` and `lastError()`; a
successful launch alone does not prove that every requested control took effect.
`HotReloadManager::Config::sandboxConfig` is optional; when unset, the manager
does not create its sandbox. Reload loads a candidate under a temporary key,
unloads the old and temporary modules, then reloads under the canonical name.
This creates an availability gap; restoration from the previous path after a
failed final load is best-effort.

The normal `ModuleLoader` verifier path supports a caller-configured hash
manifest; only modules listed in it receive the additional SHA-256 comparison.
`PluginBundleLoader` verifies an Ed25519 manifest when given a public key and
rejects unsigned bundles by default unless unsigned loading is explicitly
enabled. Revocation error values exist, but the normal loader path is not shown
to invoke CRL/OCSP checks.

This guide is a symptom-oriented aid, not proof that each named error occurs in
every production path. See `RUNBOOK.md` and `include/themis/base/` for the
source-defined error taxonomy and API.

## Symptom-driven response model

### Module not found or inaccessible

Symptoms:

- binary path missing,
- permission denied,
- deployment artifact absent.

Relevant codes:

- `BASE_LOADER_PATH_NOT_FOUND`
- `BASE_LOADER_LOAD_FAILED`

Operator action:

- confirm artifact exists at the configured path,
- check file permissions and deployment packaging,
- verify the file matches the expected plugin or module name.

### Signature or trust rejection

Symptoms:

- supplied hash/signature does not match, or the configured trust policy rejects
  the metadata/issuer,
- certificate is outside its validity dates or the configured trust check fails.

The normal `ModuleLoader` verification path does not call the implemented
CRL/OCSP methods. Do not treat this symptom list as evidence that revocation is
checked.

Relevant codes:

- `BASE_LOADER_SIGNATURE_REJECTED`
- `BASE_LOADER_HEALTH_CHECK_FAILED`

Operator action:

- identify the verifier used by the caller and compare the artifact metadata,
  signature/hash, and expected issuer against that verifier's effective policy,
- inspect whether the module is stale or replaced without approval,
- if revocation checking is required, use a verifier confirmed to perform it;
  the normal loader path is not shown to invoke CRL/OCSP checks.

### ABI or symbol mismatch

Symptoms:

- load succeeds but activation fails,
- required exports missing,
- host and module ABI disagree.

Relevant codes:

- `BASE_LOADER_ABI_MISMATCH`
- `BASE_LOADER_INIT_FAILED`

Operator action:

- rebuild or redeploy a matching ABI version,
- compare host major/minor versions against the artifact,
- inspect the initialization function and required symbol list.

### Sandbox or WASM failure

Symptoms:

- launch or runtime bounds failure,
- cgroup unsupported or resource limit hit,
- WASM capability rejection.

Relevant codes:

- `BASE_SANDBOX_LAUNCH_FAILED`
- `BASE_SANDBOX_RESOURCE_LIMIT`
- `BASE_SANDBOX_DEGRADED`

Operator action:

- inspect `ModuleSandbox::launchWarnings()` and `lastError()` to see which
  controls were skipped or failed,
- confirm resource limits at the host-process level; they do not isolate an
  in-process native plugin from ThemisDB,
- verify the plugin uses only allowed host functions and valid WASM bytecode.

### Dependency or ordering failure

Symptoms:

- module graph contains a cycle,
- order or version constraint impossible,
- required dependency missing.

Relevant codes:

- `BASE_DEPENDENCY_CYCLE`
- `BASE_DEPENDENCY_MISSING_REQUIRED`
- `BASE_DEPENDENCY_VERSION_RANGE_MISMATCH`

Operator action:

- inspect the dependency graph export,
- check version constraints and optional/required edges,
- confirm the dependency module is deployed and compatible.

### Hot reload or rollback failure

Symptoms:

- no rollback path available,
- replacement binary failed final activation,
- state restore failed after reload.

Relevant codes:

- `BASE_RELOAD_NO_BACKUP`
- `BASE_RELOAD_CANDIDATE_LOAD_FAILED`
- `BASE_RELOAD_ROLLBACK_FAILED`
- `BASE_RELOAD_STATE_RESTORE_FAILED`

Operator action:

- inspect the previous version path and request rollback only when a backup was
  retained and the result/configuration indicates rollback is available,
- check logs for candidate load failures,
- review the callback state and whether `preserveState` is viable for the module.
  the implementation unloads the old and candidate modules before loading the
  candidate under the canonical name. Plan for an availability gap and treat
  emergency restoration after final-load failure as best-effort.

### Registry or download issues

Symptoms:

- registry auth rejected,
- checksum mismatch,
- network timeout or retry exhaustion.

Relevant codes:

- `BASE_REGISTRY_AUTH_FAILURE`
- `BASE_REGISTRY_CHECKSUM_MISMATCH`
- `BASE_REGISTRY_DOWNLOAD_FAILED`

Operator action:

- verify the configured `RegistryConfig` credential fields and TLS options;
  credential storage/rotation is the owning application's responsibility,
- check download path and file integrity,
- review timeout and retry budgets for transient network issues.

## Observability signals

Operationally important signals include:

- load attempt counts and failure counters,
- retry exhaustion and timeout counts in `RequestStats`,
- hot reload stats such as total reloads and failed reloads,
- warning or error spans emitted from reload operations,
- module health check failure details.

`RequestStats` retry/timeout counts and the registered observability hooks are
source-defined. Names of exported metrics, server/admin commands, alert rules,
and deployment configuration keys are not defined by the base API and must be
treated as deployment-specific examples, not built-in ThemisDB interfaces.

`RegistryConfig` does define `auth_token`, `api_key`, `verify_ssl` (default
true), `ca_bundle_path`, and `pinned_public_key`. Credentials are sent as
request headers; storage/rotation is outside this client. HTTP is accepted with
a warning, and `verify_ssl=false` disables certificate and host verification.

These signals should be used together with the taxonomy to reduce incident time
and avoid generic “module failed” triage.

## Recovery strategy

The recommended recovery pattern is:

1. isolate the failed module or dependency,
2. inspect the code path that generated the taxonomy error,
3. verify artifact validity and dependency compatibility,
4. attempt only the minimum corrective action,
5. re-run the affected load or reload path with observability enabled,
6. keep rollback available until recovery is verified.

## Escalation

Escalate when:

- a module fails trust verification repeatedly,
- a hot reload cannot be rolled back,
- dependency cycles appear in production configuration.
- sandbox or WASM resource caps are triggered under normal load,
- registry or network changes affect multiple modules at once.

These incidents are not routine product issues; they are runtime-safety events and should be treated as such.
