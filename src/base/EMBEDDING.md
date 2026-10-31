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

# Base Module Embedding and Integration Notes

## Purpose

The base module is embedded into the wider system as the runtime boundary for module lifecycle and safety. It is not modeled as a storage or query subsystem; it behaves as the execution substrate for dynamic extension and safe runtime mutation.

## Controlled embedding points

The module is connected to the rest of ThemisDB through a small set of operational surfaces:

- `ModuleLoader` handles module discovery, verification, activation, and unloading.
- `HotReloadManager` coordinates candidate loading, reload results, and configured rollback. Replacement unloads the old and temporary candidate modules before loading under the canonical key, so availability is interrupted; restoration after final-load failure is best-effort.
- `PluginDependencyGraph` exposes ordering and cycle analysis; callers must invoke/use the analysis where needed.
- `RemoteRegistryClient` supports credentials and artifact validation. HTTP and HTTPS are accepted, with a warning for plaintext HTTP.
- `ModuleSandbox` and `WasmPluginSandbox` expose configurable, platform/runtime-dependent controls; they do not imply universal isolation.
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

1. The caller passes a module path to `ModuleLoader`.
2. The loader's configured verification path checks the artifact/policy; unsigned-load configuration exists, so signature verification is not universal.
3. The loader exposes metadata and ABI compatibility checks. `AbiChecker` is a separate API; do not assume every caller invokes it.
4. The loader reports lifecycle/result state; activation depends on the configured checks and their outcome.

### Hot reload flow

1. `HotReloadManager` validates the registration and current module state.
2. It saves state if configured and emits `BEFORE_UNLOAD` notifications.
3. It loads the candidate under a temporary key before unloading the current version.
4. It unloads the temporary candidate and loads the replacement under the canonical key; failure and emergency restore paths are represented in the result/implementation.
5. Rollback availability depends on configuration and the recorded backup; callers must inspect the result.

### Wasm integration flow

1. `WasmPluginSandbox` validates the binary and host-function allowlist.
2. A runtime is created/injected only if an implementation is registered/selected; without one, the sandbox is validation-only.
3. Configured sandbox limits are applied subject to platform support.
4. Registered host functions define the API available to the runtime.

## Dependency implications

The dependency graph provides analysis for startup/reload integrations, but the
loader does not automatically invoke `PluginDependencyGraph` in every path.
Callers must ensure their integration uses dependency validation as required.

## Operational embedding guidance

When embedding or extending the base module, keep the following rules in mind:

- Keep the trust boundary on the loader or registry side, not in higher-level callers.
- Do not widen the runtime contract by silently accepting incompatible or unverified modules.
- Put execution limits in the sandbox layer rather than as ad hoc per-caller guards.
- Preserve explicit rollback and diagnostics in reload changes.
- Keep dependency ordering deterministic and observable.

## Source-grounded summary

The source shows that the base area offers lifecycle and safety controls. Their
coverage depends on configuration, platform/backend availability, and caller
integration; the documentation does not claim that every control is applied
universally.

## Validation path

For a concrete integration, check the selected header contract, the matching
implementation and build source list, then the test target's parent-CMake
registration. For benchmark claims, use
[`PERFORMANCE_EXPECTATIONS.md`](PERFORMANCE_EXPECTATIONS.md) and attach measured
results; gate definitions alone are not validation evidence. The source-list
and test-registration gaps are summarized in [`AUDIT.md`](AUDIT.md).
