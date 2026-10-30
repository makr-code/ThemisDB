---
Author: "ThemisDB Contributors"
Created: "2026-10-30"
Last Updated: "2026-10-30"
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
| Generic resource pools | [`BufferPool`](../../include/base/buffer_pool.h), [`ResourcePoolManager`](../../include/base/resource_pool_manager.h) | Reuse size-class buffers and coordinate generic connection/buffer/thread resource pools. |

## Data and event flow

1. A module artifact is registered/resolved and checked for path, metadata, compatibility, and security requirements before activation.
2. Dependency resolution produces a load order; configured sandbox and runtime hooks apply at the module boundary.
3. Hot reload tracks candidate activation and rollback state; remote registry download is a separate intake path before loader hand-off.
4. Independent resource-pool callers acquire/release pooled buffers or connection slots and can inspect pool statistics.

Evidence: [`module_loader.cpp`](../themis/module_loader.cpp), [`module_sandbox.cpp`](module_sandbox.cpp), [`hot_reload_manager.cpp`](hot_reload_manager.cpp), [`plugin_dependency_graph.cpp`](plugin_dependency_graph.cpp), [`buffer_pool.cpp`](buffer_pool.cpp), and [`resource_pool_manager.cpp`](resource_pool_manager.cpp).

## Failure domains and edge cases

- Missing/invalid artifacts, signature/hash or ABI checks, and dependency cycles can stop activation before a module is usable.
- Sandbox setup or absent optional WASM runtime limits the corresponding execution path; do not treat configured isolation as universal across all plugins.
- Reload candidate failure can trigger rollback; rollback failure is a distinct error state.
- Registry authentication/download/checksum failure blocks that artifact intake path.
- Buffer requests larger than the fixed slab classes use the system allocator; pool acquisition/configuration failures are separate from module-load failures.

## Explicit non-goals

- Implementing plugin domain logic, module APIs, or database request execution.
- Guaranteeing every module is dynamically loaded or sandboxed at runtime.
- Treating legacy `src/base/module_loader.cpp` as the canonical loader implementation.
- Claiming every pool design target or performance figure is met without current benchmark evidence.

## Verified cross-module references

| Boundary | Evidence and link |
|---|---|
| Plugin security | Canonical `src/themis/module_loader.cpp` includes `acceleration/plugin_security.h`; see [`PluginSecurity`](../../include/acceleration/plugin_security.h). |
| Hash verification | The loader includes Themis module hash-verification support; see [`module_hash_verifier.h`](../../include/themis/module_hash_verifier.h). |
| Server consumer | `src/server/http_server.cpp` includes `themis/base/module_loader.h`; see [server module README](../server/README.md). |

## References

- [Module README](README.md) · [API reference](API.md) · [module roadmap](ROADMAP.md)
- [`module_loader.h`](../../include/themis/base/module_loader.h) · [`hot_reload_manager.h`](../../include/themis/base/hot_reload_manager.h) · [`resource_pool_manager.h`](../../include/base/resource_pool_manager.h)
