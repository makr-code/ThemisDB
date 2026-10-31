---
Author: "ThemisDB Contributors"
Created: "2026-10-30"
Last Updated: "2026-10-31"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot coding agent"
Human_Review: false
Human_Author: "ThemisDB Contributors"
Source_Level: "level1"
SOT_Domain: "module-behavior"
Canonical_References:
  - "README.md"
  - "API.md"
  - "../../include/themis/base/module_loader.h"
  - "../../include/themis/base/module_sandbox.h"
  - "../../include/themis/base/hot_reload_manager.h"
  - "../../include/themis/base/plugin_dependency_graph.h"
  - "../../include/base/buffer_pool.h"
  - "../../include/base/resource_pool_manager.h"
  - "../themis/module_loader.cpp"
  - "module_sandbox.cpp"
  - "hot_reload_manager.cpp"
  - "buffer_pool.cpp"
Related_Issue: "#7167 (closes #7091)"
Related_Milestone: "DOC-WEEKLY-2026-W44"
---

Exact model backend not verifiable from final document text alone.

# Base Module Architecture

## Scope, boundaries, and ownership

The base area contains two related foundations: module/plugin lifecycle APIs under `themis::modules` and shared resource-pool utilities under `themis::resource`. It owns module loading/verification, dependency resolution, sandbox/WASM integration, hot reload, and generic buffer/connection-pool support; it does not own plugin-specific business behavior.

**Source ownership caveat:** `src/base/module_loader.cpp` identifies itself as a deprecated legacy copy. The canonical platform-independent loader implementation is [`src/themis/module_loader.cpp`](../themis/module_loader.cpp), with platform-specific sources alongside it. Do not infer current loader behavior from the legacy copy.

## Components and public interfaces

| Component | Public interface | Source-backed responsibility |
|---|---|---|
| Module lifecycle | [`ModuleLoader`](../../include/themis/base/module_loader.h) | Register/load/unload modules and expose verification, lifecycle, and error state. |
| Isolation and ABI | [`ModuleSandbox`](../../include/themis/base/module_sandbox.h), [`WasmPluginSandbox`](../../include/themis/base/wasm_plugin_sandbox.h), [`WasmRuntimeInjector`](../../include/themis/base/wasm_runtime_injector.h) | Apply configured sandbox/ABI controls and select explicitly registered WASM runtimes. |
| Dependency, reload, registry | [`PluginDependencyGraph`](../../include/themis/base/plugin_dependency_graph.h), [`HotReloadManager`](../../include/themis/base/hot_reload_manager.h), [`RemoteRegistryClient`](../../include/themis/base/remote_registry_client.h) | Resolve dependency order, manage reload/rollback, and retrieve/validate registry artifacts. |
| Generic resource pools | [`BufferPool`](../../include/base/buffer_pool.h), [`ResourcePoolManager`](../../include/base/resource_pool_manager.h) | Reuse size-class buffers and generic integer slots; no thread-pool ownership or leak-reclamation path is implemented by `ResourcePoolManager`. |

## Data and event flow

1. A caller passes an artifact path to `ModuleLoader`; the loader checks existence and runs its configured verification, metadata, and activation steps.
2. `PluginDependencyGraph` can analyze a resolver or registry snapshot and report cycles/order; it is a separate API, not evidence that every load path automatically consults it.
3. Sandbox constraints apply only when configured and depend on platform support. WASM execution additionally requires an injected/registered runtime.
4. Hot reload loads a candidate under a temporary key before unloading the current module, but subsequently unloads the candidate and loads it under the canonical key. The public header contracts an atomic swap; this implementation sequence appears inconsistent with that contract. Inspect result and rollback state, and do not claim uninterrupted availability until the discrepancy is reconciled.
5. Remote registry download is a separate intake path. HTTP and HTTPS URLs are accepted; HTTP triggers a warning, so transport encryption is not guaranteed.
6. Independent resource-pool callers can acquire/release buffers or connection slots and inspect statistics; the pool implementation sources are absent from the modular `THEMIS_BASE_SOURCES` and the monolithic `THEMIS_CORE_SOURCES` lists inspected.

Evidence: [`module_loader.cpp`](../themis/module_loader.cpp), [`module_sandbox.cpp`](module_sandbox.cpp), [`hot_reload_manager.cpp`](hot_reload_manager.cpp), [`plugin_dependency_graph.cpp`](plugin_dependency_graph.cpp), [`buffer_pool.cpp`](buffer_pool.cpp), and [`resource_pool_manager.cpp`](resource_pool_manager.cpp).

## Failure domains and edge cases

- Missing/invalid artifacts, signature/hash or ABI checks, and dependency cycles can stop activation before a module is usable.
- Sandbox setup or absent optional WASM runtime limits the corresponding execution path; do not treat configured isolation as universal across all plugins.
- Reload candidate failure can trigger rollback; rollback failure is a distinct error state.
- Registry authentication/download/checksum failure blocks that artifact intake path.
- Buffer requests larger than the fixed slab classes use the system allocator; pool acquisition/configuration failures are separate from module-load failures.
- `ResourcePoolManager::Config::leak_timeout` is declared but not consumed in its implementation; its header's thread-pool and leak-detection design goals are not realized by the inspected class/source.

## Explicit non-goals

- Implementing plugin domain logic, module APIs, or database request execution.
- Guaranteeing every module is dynamically loaded or sandboxed at runtime.
- Treating legacy `src/base/module_loader.cpp` as the canonical loader implementation.
- Claiming every pool design target or performance figure is met without current benchmark evidence.

## Build and test registration evidence

In modular mode, `cmake/ModularBuild.cmake` uses `THEMIS_BASE_SOURCES` to build
`themis_base` from the lifecycle/sandbox/registry/WASM sources under `src/base/`
and the canonical loader at `src/themis/module_loader.cpp`. It omits
`src/base/module_loader.cpp`, `buffer_pool.cpp`, and
`resource_pool_manager.cpp`. In that mode, `cmake/CMakeLists.txt` defines
`themis_core` as an interface target linking the modules. `tests/CMakeLists.txt`
discovers module CMake files using `CONFIGURE_DEPENDS` and adds
`tests/base/CMakeLists.txt`. That file registers `test_*.cpp` as focused unit
tests and adds a focused soak target with a 7200-second timeout. The registration
helper supplies `module:base`, `tier:unit`/`tier:soak`, `kind:focused`, and
`focus` labels; the local file adds `base` and `soak` labels. The root test file
also separately registers the reload-regression and BaseEntity field tests.
These definitions establish registration, not successful build or execution.

## Verified cross-module references

| Boundary | Evidence and link |
|---|---|
| Plugin security | Canonical `src/themis/module_loader.cpp` includes `acceleration/plugin_security.h`; see [`PluginSecurity`](../../include/acceleration/plugin_security.h). |
| Hash verification | The loader includes Themis module hash-verification support; see [`module_hash_verifier.h`](../../include/themis/module_hash_verifier.h). |
| Server consumer | `src/server/http_server.cpp` includes `themis/base/module_loader.h`; see [server module README](../server/README.md). |

## References

- [Module README](README.md) · [API reference](API.md) · [module roadmap](ROADMAP.md)
- [`module_loader.h`](../../include/themis/base/module_loader.h) · [`hot_reload_manager.h`](../../include/themis/base/hot_reload_manager.h) · [`resource_pool_manager.h`](../../include/base/resource_pool_manager.h)
