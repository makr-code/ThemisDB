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

# Base Module API Reference

## Scope

This API summary is based on the public headers under `include/themis/base/` and the runtime behavior in the corresponding `src/base` implementation files. It covers the contract surfaces used by ThemisDB to load, verify, isolate, and replace modules safely.

## Core API groups

### 1. Module loading and lifecycle

`ModuleLoader` in `include/themis/base/module_loader.h` is the primary module runtime surface.

Key types:

- `LoadStage`
- `HealthCheckResult`
- `ModuleDependency`
- `ModuleMetadata`
- `ModuleFailureHistory`
- `ModuleMetrics`
- `ModuleErrorCode`
- `ErrorCategory`

Key behavior:

- verify and stage module load operations,
- maintain load state and failure history,
- enforce structured error codes,
- allow health checks before activation,
- support module unload and lifecycle transitions.

Critical invariants:

- a module must be verified before activation,
- missing or invalid module artifacts fail closed,
- dependency and compatibility mismatches are not silently ignored,
- health checks are part of activation correctness, not optional decoration.

### 2. ABI and compatibility validation

`AbiChecker` is the compatibility contract for runtime safety.

Representative API:

- `addRequiredSymbol()`
- `addDeprecatedSymbol()`
- `useDefaultLists()`
- `check()`
- `checkVersions()`
- `checkRequiredSymbols()`
- `checkDeprecatedSymbols()`

Important guarantees:

- major ABI mismatch is a hard failure,
- forward minor-version incompatibility is rejected,
- required exported symbols must exist,
- deprecated symbol presence is flagged but not always fatal,
- metadata is part of the compatibility contract.

### 3. Sandbox and execution constraints

`ModuleSandbox` and `WasmPluginSandbox` define the runtime bound for untrusted execution.

Key concepts:

- `SandboxStats`
- `ModuleSandbox::Config`
- `WasmPluginSandbox::Config`
- `WasmRuntime`
- `WasmHostFunction`
- `WasmCallResult`

Critical contracts:

- the sandbox is resource-limited and platform-aware,
- linear memory and allowlisted functions are required for WASM execution,
- runtime injection is optional and must be explicit,
- sandbox capability gaps should degrade predictably and remain observable.

### 4. Dependency graph and ordering

`PluginDependencyGraph` provides the ordering model used for dependency-safe activation.

Key methods:

- `addModule()`
- `addDependency()`
- `buildFromResolver()`
- `buildFromRegistry()`
- `detectCycles()`
- `topologicalOrder()`
- `exportTo()`

The graph is a safe ordering model: it prevents undefined module activation order from creeping into runtime behavior.

### 5. Hot reload and rollback

`HotReloadManager` is the live-update and rollback API.

Key types:

- `ModuleVersion`
- `HotReloadResult`
- `ReloadPhase`
- `ReloadCallback`
- `StateSaveCallback`
- `StateRestoreCallback`
- `HotReloadManager::Config`

Key behavior:

- registers and unregisters modules,
- reloads a module from a new binary path,
- preserves state if configured,
- ensures the old module remains live if the new one fails,
- retains rollback and emits lifecycle callbacks.

The most important API contract is: do not mutate a live module without a known-good target and an explicit rollback path.

### 6. Remote registry intake

`RemoteRegistryClient` and related config types enable trusted remote artifact intake.

Key types:

- `RegistryConfig`
- `RegistryPluginEntry`
- `PluginDownloadResult`
- `RequestStats`

Behavior:

- authenticates, downloads, verifies checksums,
- records retry/timeout metrics,
- passes integrity-checked module paths to the loader,
- fails closed if auth or checksum validation fails.

### 7. A/B rollout control

`ABTestManager` provides a rollout experiment surface layered on top of the base runtime model.

Key behavior:

- manage parallel test variants,
- split traffic between control and treatment variants,
- evaluate outcome significance,
- support promotion or rollback without full process restart when using a `HotReloadManager`.

## Error model

The error model is standardized in `include/themis/base/base_error_taxonomy.h`.

Examples:

- `BASE_LOADER_PATH_NOT_FOUND`
- `BASE_LOADER_SIGNATURE_REJECTED`
- `BASE_LOADER_ABI_MISMATCH`
- `BASE_LOADER_LOAD_FAILED`
- `BASE_SANDBOX_LAUNCH_FAILED`
- `BASE_RELOAD_ROLLBACK_FAILED`
- `BASE_DEPENDENCY_CYCLE`
- `BASE_REGISTRY_DOWNLOAD_FAILED`

These codes are not just logs; they are the operational contract for failure diagnosis.

## Example lifecycle usage

```cpp
using namespace themis::modules;

ModuleLoader loader;
HotReloadManager reload_manager;
PluginDependencyGraph graph;

loader.registerModule("themis_analytics");

AbiChecker checker;
checker.useDefaultLists();

ModuleMetadata meta = loader.getModuleInfo("themis_analytics").value().metadata;
AbiCheckResult abi = checker.check(handle, meta, 1, 0);
if (!abi.compatible) {
    // fail closed before activation
}

auto result = reload_manager.reloadModule("themis_analytics", "/opt/themis/modules/themis_analytics_v2.so");
if (!result.success) {
    // inspect result.errorMessage and rollback status
}
```

## Contract expectations

Callers should assume:

- the module interface requires explicit validation before activation,
- reload and dependency operations are stateful and operationally sensitive,
- runtime limits and compatibility are not optional,
- failure codes should be surfaced to operators and not flattened into generic “module failed” messages.
