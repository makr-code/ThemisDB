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

# Base Module Research Notes

## Scope

This review is grounded in the concrete runtime interfaces in `include/themis/base/` and the implementation files under `src/base/`:

- `module_loader.h` and `module_loader.cpp`
- `module_sandbox.h` and `module_sandbox.cpp`
- `hot_reload_manager.h` and `hot_reload_manager.cpp`
- `plugin_dependency_graph.h` and `plugin_dependency_graph.cpp`
- `remote_registry_client.h` and `remote_registry_client.cpp`
- `wasm_plugin_sandbox.h` / `wasm_plugin_sandbox.cpp`
- `ab_test_manager.h`

The base module is not a domain feature layer; it is the runtime contract that allows ThemisDB to load, verify, isolate, and swap executable extension modules without destabilizing the host process.

## Problem statement

The central design tension is clear from the source: extension modules must be safe to load, safe to execute, and safe to replace while the DB remains online. The design solves this by separating verification, dependency analysis, isolation, and hot-swap legality into distinct stages instead of letting load behavior become a monolithic single-step action.

The source evidence is explicit:

- `ModuleSecurityVerifier` enforces signature and trust policy checks before activation.
- `LoadStage` enumerates a staged lifecycle from `UNLOADED` through `VERIFYING`, `VALIDATED`, `STAGING`, `ACTIVATING`, and `ACTIVE`.
- `AbiChecker` validates ABI compatibility and required symbol presence before switching a module into usage.
- `ModuleSandbox` constrains OS-level execution with memory, CPU, and cgroup-aware protections when the platform supports them.
- `HotReloadManager` loads a replacement candidate before unloading the old module, then keeps rollback state available until the new version stabilizes.
- `PluginDependencyGraph` detects cycles and computes a safe load order before modules are activated or re-ordered.
- `RemoteRegistryClient` verifies registry metadata, checksum, and auth before a module artifact is stored locally.

## Research questions

### 1. Why is the loader staged instead of a single direct load?

The staged lifecycle is a control-plane design, not just bookkeeping. It allows the system to fail closed at each boundary:

- missing artifact or untrusted signature
- ABI mismatch or missing required symbol
- dependency conflict or circular dependency
- runtime health-check failure
- invalid sandbox or degraded backend state

This design matches the explicit `ModuleErrorCode` taxonomy in `module_loader.h` and the base error taxonomy in `base_error_taxonomy.h`.

### 2. Why is sandboxing a separate concern from loading?

The implementation treats trust and execution as different responsibilities. A module can be syntactically valid and still be dangerous if it consumes too much memory, runs unsafely, or escapes the host's resource model. `ModuleSandbox` and `WasmPluginSandbox` make resource isolation an explicit runtime boundary, not an afterthought.

### 3. Why is hot reload designed around rollback?

The `HotReloadManager` contract prefers atomic replacement with a rollback slot. Source comments in `hot_reload_manager.h` explicitly describe atomic swap semantics and state-preserving reload behavior. This is a practical answer to the operational reality that in-place replacement must preserve availability while still allowing a failed upgrade to be reversed without a process restart.

## Assumptions and constraints

- The base layer is a runtime infrastructure boundary, not a user-facing data or query layer.
- Every module is expected to pass trust and compatibility checks before activation.
- Execution limits are best-effort where the platform does not support a mechanism; the code degrades safely rather than silently disabling all constraints.
- The registry and dependency system are designed to reject unsafe or incompatible combinations instead of trying to force them into a running state.
- Hot reload is intentionally operationally conservative: success requires safe activation and explicit rollback state.

## Implementation evidence

The following patterns appear directly in the implementation:

- `ModuleLoader::loadModule()` governs module discovery, verification, metadata extraction, and activation states.
- `AbiChecker::checkVersions()` and `checkRequiredSymbols()` reject incompatible or malformed binaries before activation.
- `ModuleSandbox` checks cgroup support, process limits, and optional WASM isolation features when available.
- `WasmRuntimeAdapter` bridges runtime injection to `WasmPluginSandbox`, allowing pluggable runtime implementations behind a stable interface.
- `PluginDependencyGraph::detectCycles()` and `topologicalOrder()` identify dependency ordering constraints that must be respected for safe startup.
- `RemoteRegistryClient` authenticates, downloads, and checksum-verifies plugin payloads before handing them to the loader.
- `ABTestManager` introduces optional version experiments without a restart, which is a deployment-time extension of the same lifecycle model.

## Evaluation criteria

The design is good when the following remain true:

- invalid or incompatible modules fail closed,
- module ordering remains deterministic and cycle-safe,
- sandbox limits are enforced when the OS provides the primitives,
- reload succeeds without breaking the previous version before the new one is proven safe,
- operators can diagnose failure with explicit error codes and remediation hints.

## Trade-offs

### Safety vs. ease of deployment

The base layer intentionally refuses modules that fail trust or ABI checks. This increases operational discipline but reduces “best effort” module loading and prevents hidden platform instability.

### Bounded execution vs. maximum flexibility

Sandboxing and WASM isolation impose limits. The trade-off is a tighter operational envelope in exchange for predictable failures and reduced blast radius.

### Availability vs. correctness

The reload design preserves availability by loading a new candidate before dropping the old binary, but this requires more coordination and state management. The implementation chooses explicit validation and rollback over optimistic replacement.

## Conclusion

The base module is best understood as ThemisDB’s runtime trust and lifecycle foundation. It exists to make module change safe, inspectable, and reversible. That is why the implementation separates validation, dependency modeling, resource isolation, and reload semantics rather than collapsing them into a single loosely-typed plugin load path.
