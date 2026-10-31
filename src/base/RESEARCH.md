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

# Base Module Research Notes

## Scope

This review is grounded in runtime interfaces in `include/themis/base/` and
implementation files under `src/base/`; the loader implementation is in
`src/themis/`, not the legacy copy under `src/base/`:

- `module_loader.h` and canonical `src/themis/module_loader.cpp`
- `module_sandbox.h` and `module_sandbox.cpp`
- `hot_reload_manager.h` and `hot_reload_manager.cpp`
- `plugin_dependency_graph.h` and `plugin_dependency_graph.cpp`
- `remote_registry_client.h` and `remote_registry_client.cpp`
- `wasm_plugin_sandbox.h` / `wasm_plugin_sandbox.cpp`
- `ab_test_manager.h`

The base area is not a domain feature layer; it provides module lifecycle,
verification, optional isolation, and reload APIs. The availability and
guarantees of these controls depend on configuration and runtime integration.

## Problem statement

The central design tension is clear from the source: extension modules need
verification, execution controls, and managed replacement. The APIs separate
these concerns; they do not establish that each is automatically enabled or
that replacement realizes the header's atomic-swap contract or preserves
uninterrupted availability.

The source evidence is explicit:

- `ModuleSecurityVerifier` provides configurable signature/trust-policy checks; an unsigned-load option exists, so signature enforcement is not universal.
- `LoadStage` enumerates a staged lifecycle from `UNLOADED` through `VERIFYING`, `VALIDATED`, `STAGING`, `ACTIVATING`, and `ACTIVE`.
- `AbiChecker` validates ABI compatibility and required symbol presence before switching a module into usage.
- `ModuleSandbox` constrains OS-level execution with memory, CPU, and cgroup-aware protections when the platform supports them.
- `HotReloadManager` loads a candidate before unloading the old module, then performs a second load under the canonical key. The header contracts an atomic swap, but the implementation sequence appears inconsistent; uninterrupted availability remains unresolved.
- `PluginDependencyGraph` can detect cycles and compute an order; caller integration determines whether the analysis gates activation.
- `RemoteRegistryClient` supports configured authentication and checksum validation. It accepts both HTTP and HTTPS, so transport encryption is not universal.

## Research questions

### 1. Why is the loader staged instead of a single direct load?

The staged lifecycle is a control-plane design, not just bookkeeping. It
exposes staged failure states; actual checks and fail-closed behavior depend on
the configured verification path:

- missing artifact or untrusted signature
- ABI mismatch or missing required symbol
- dependency conflict or circular dependency
- runtime health-check failure
- invalid sandbox or degraded backend state

This design matches the explicit `ModuleErrorCode` taxonomy in `module_loader.h` and the base error taxonomy in `base_error_taxonomy.h`.

### 2. Why is sandboxing a separate concern from loading?

The implementation treats trust and execution as different responsibilities. A module can be syntactically valid and still be dangerous if it consumes too much memory, runs unsafely, or escapes the host's resource model. `ModuleSandbox` and `WasmPluginSandbox` make resource isolation an explicit runtime boundary, not an afterthought.

### 3. Why is hot reload designed around rollback?

The header contracts an atomic swap, but the implementation loads a temporary
candidate, unloads the old and temporary modules, then loads under the canonical
name. Treat the difference as an unresolved contract/implementation discrepancy,
not as grounds to discard the API contract or to promise uninterrupted
availability. State callbacks and rollback are configuration- and
outcome-dependent.

## Assumptions and constraints

- The base layer is a runtime infrastructure boundary, not a user-facing data or query layer.
- The loader exposes trust and compatibility checks; enabled checks depend on configuration and caller path.
- Execution limits are best-effort where the platform does not support a mechanism; the code degrades safely rather than silently disabling all constraints.
- The dependency graph exposes rejection/ordering analysis, but automatic use by every loader path is not established.
- Hot reload returns explicit outcomes; success/rollback guarantees must be read from the result and configuration, not assumed.

## Implementation evidence

The following patterns appear directly in the implementation:

- `ModuleLoader::loadModule()` governs module discovery, verification, metadata extraction, and activation states.
- `AbiChecker::checkVersions()` and `checkRequiredSymbols()` reject incompatible or malformed binaries before activation.
- `ModuleSandbox` checks cgroup support, process limits, and optional WASM isolation features when available.
- `WasmRuntimeAdapter` bridges runtime injection to `WasmPluginSandbox`, allowing pluggable runtime implementations behind a stable interface.
- `PluginDependencyGraph::detectCycles()` and `topologicalOrder()` identify dependency ordering constraints that must be respected for safe startup.
- `RemoteRegistryClient` supports configured authentication and checksum checks before handing payloads to the loader; plaintext HTTP is accepted with a warning.
- `ABTestManager` introduces optional version experiments without a restart, which is a deployment-time extension of the same lifecycle model.

## Evaluation criteria (design goals, not validation results)

The design is good when the following remain true:

- invalid/incompatible modules fail closed when the relevant checks are enabled,
- module ordering remains deterministic and cycle-safe when the graph analysis is used,
- configured sandbox limits are enforced when the OS provides the primitives,
- reload reports failure and handles the previous-version/rollback path according to the implementation; the header's atomic-swap contract and implementation sequence need reconciliation before uninterrupted availability can be claimed,
- operators can diagnose failure with explicit error codes and remediation hints.

The relevant measurable surfaces and defined gates are listed in
[`PERFORMANCE_EXPECTATIONS.md`](PERFORMANCE_EXPECTATIONS.md). No benchmark run,
test run, or representative-hardware result was produced for this review; use
the current benchmark targets and record actual results before making
performance/readiness claims.

## Trade-offs

### Safety vs. ease of deployment

The base layer can refuse modules that fail enabled trust or ABI checks. The
verification configuration is not universal, so do not generalize this to
every load path.

### Bounded execution vs. maximum flexibility

When enabled and supported, sandboxing/WASM controls impose limits; they are
not universal guarantees across all platforms or runtime configurations.

### Availability vs. correctness

The header contracts atomic replacement, while the implementation's subsequent
unload/reload under the canonical key appears inconsistent with that contract.
Callers must inspect the result and configured rollback state; uninterrupted
availability remains unresolved pending reconciliation.

## Conclusion

The base area provides APIs for runtime trust, lifecycle, dependency analysis,
optional isolation, and reload. The coverage and guarantees available to an
application depend on configuration, platform/backend support, and caller
integration.
