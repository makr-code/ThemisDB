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

# Base Module Embedding and Integration Notes

## Purpose

The base module is embedded into the wider system as the runtime boundary for module lifecycle and safety. It is not modeled as a storage or query subsystem; it behaves as the execution substrate for dynamic extension and safe runtime mutation.

## Controlled embedding points

The module is connected to the rest of ThemisDB through a small set of operational surfaces:

- `ModuleLoader` handles module discovery, verification, activation, and unloading.
- `HotReloadManager` coordinates atomic replacement and rollback during live upgrades.
- `PluginDependencyGraph` exposes ordering and cycle analysis for module dependency constraints.
- `RemoteRegistryClient` pulls plugin artifacts from an authenticated registry and validates them before local installation.
- `ModuleSandbox` and `WasmPluginSandbox` bound execution resources and provide isolation.
- `ABTestManager` supports incremental conversion of a module variant to production without a full restart.

## Integration pattern

The integration pattern is explicit in the headers:

- The loader owns artifact loading and trust checks.
- The sandbox owns execution constraints.
- The dependency graph owns dependency legality and ordering.
- The reload manager owns transition semantics and rollback.
- The registry client owns remote artifact intake and verification.

This separation is deliberate. It ensures that callers do not need to combine security, dependency, execution, and rollout rules ad hoc.

## Important runtime flows

### Module activation flow

1. Module path is resolved by `ModuleLoader`.
2. `ModuleSecurityVerifier` validates the artifact and policy.
3. `ModuleMetadata` is extracted and checked for ABI compatibility.
4. `AbiChecker` verifies required symbols and ABI version compatibility.
5. The module enters the staged lifecycle and is activated only after the checks pass.

### Hot reload flow

1. `HotReloadManager` validates the registration and current module state.
2. It saves state if configured and emits `BEFORE_UNLOAD` notifications.
3. It loads the candidate artifact before unloading the current version.
4. It preserves a rollback path and updates the canonical module name only after the new version is loadable.
5. It emits lifecycle notifications and keeps rollback data available.

### Wasm integration flow

1. `WasmPluginSandbox` validates the binary and the host allowlist.
2. It creates or binds a runtime via `WasmRuntimeInjector`.
3. It applies sandbox resource limits and execution restrictions.
4. It exposes a narrow, explicit host-function boundary to imported WASM calls.

## Dependency implications

The dependency graph is not merely descriptive. It is part of the runtime contract, because Startup and reload ordering are legal only when the dependency topology is valid. A module cannot be safely loaded without considering the dependency graph and ABI compatibility wire.

## Operational embedding guidance

When embedding or extending the base module, keep the following rules in mind:

- Keep the trust boundary on the loader or registry side, not in higher-level callers.
- Do not widen the runtime contract by silently accepting incompatible or unverified modules.
- Put execution limits in the sandbox layer rather than as ad hoc per-caller guards.
- Preserve explicit rollback and diagnostics in reload changes.
- Keep dependency ordering deterministic and observable.

## Source-grounded summary

The source shows that the base module is best understood as an execution substrate with guardrails. It is embedded into the wider ThemisDB runtime by providing a single, conservative lifecycle model: verify first, order safely, isolate execution, activate only under valid conditions, and preserve rollback if a change cannot be proved safe.
