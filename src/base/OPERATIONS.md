---
Author: "ThemisDB Contributors"
Created: "2026-10-21"
Last Updated: "2026-10-21"
Status: "active"
AI_Used: true
AI_Models: "GitHub Copilot, MAI-Code-1.1-Flash"
AI_Contributor: "GitHub Copilot"
Human_Review: true
Human_Author: "ThemisDB Contributors"
---

Exact model backend not verifiable from final document text alone.

# Base Module Operations Guide

## Mission

The base module is the runtime operations substrate for execution safety. When a module is failing, the most important question is not “did the process crash,” but “which boundary rejected or destabilized the module, and what should the operator do next?”

## Primary operational surfaces

The relevant runtime artifacts are:

- `ModuleLoader` — lifecycle and verification objective
- `ModuleSandbox` — resource and isolation guardrails
- `HotReloadManager` — in-place change and rollback behavior
- `PluginDependencyGraph` — dependency legality and startup ordering
- `RemoteRegistryClient` — registry-driven artifact acquisition
- `ABTestManager` — rollout and promotion state

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

- module was modified in transit,
- signer is not trusted,
- certificate is revoked or expired.

Relevant codes:

- `BASE_LOADER_SIGNATURE_REJECTED`
- `BASE_LOADER_HEALTH_CHECK_FAILED`

Operator action:

- confirm the artifact was signed by a trusted key,
- inspect whether the module is stale or replaced without approval,
- verify revocation and trust policy settings.

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
- `BASE_SANDBOX_DEGRADED_STATE`

Operator action:

- confirm platform support for cgroups or job objects,
- inspect memory and CPU caps,
- verify the plugin uses only allowed host functions and valid WASM bytecode.

### Dependency or ordering failure

Symptoms:

- module graph contains a cycle,
- order or version constraint impossible,
- required dependency missing.

Relevant codes:

- `BASE_DEPENDENCY_CYCLE`
- `BASE_DEPENDENCY_MISSING_REQUIREMENT`
- `BASE_DEPENDENCY_VERSION_MISMATCH`

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

- inspect the previous version path and restore it if needed,
- check logs for candidate load failures,
- review the callback state and whether `preserveState` is viable for the module.

### Registry or download issues

Symptoms:

- registry auth rejected,
- checksum mismatch,
- network timeout or retry exhaustion.

Relevant codes:

- `BASE_REGISTRY_AUTH_FAILED`
- `BASE_REGISTRY_CHECKSUM_MISMATCH`
- `BASE_REGISTRY_DOWNLOAD_FAILED`

Operator action:

- verify registry credentials and TLS configuration,
- check download path and file integrity,
- review timeout and retry budgets for transient network issues.

## Observability signals

Operationally important signals include:

- load attempt counts and failure counters,
- retry exhaustion and timeout counts in `RequestStats`,
- hot reload stats such as total reloads and failed reloads,
- warning or error spans emitted from reload operations,
- module health check failure details.

These should be used together with the taxonomy to reduce incident time and avoid generic “module failed” triage.

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
