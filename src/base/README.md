---
Author: "ThemisDB Contributors"
Created: "2026-05-31"
Last Updated: "2026-10-31"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot coding agent"
Human_Review: false
Human_Author: "ThemisDB Contributors"
---

Exact model backend not verifiable from final document text alone.

# ThemisDB Base Module

## Purpose and boundary

The base area groups module lifecycle and sandbox facilities with general
resource-pool utilities. It does not own plugin-specific business behavior.
Runtime capabilities depend on the configured platform and registered optional
backends; this documentation does not imply that every loaded module is
sandboxed or that all resource-pool sources are linked into the main library.

## Source and interface map

| Surface | Current source / interface | Responsibility |
|---|---|---|
| Module loading | `include/themis/base/module_loader.h`; canonical implementation `src/themis/module_loader.cpp` and platform/security companions | Artifact loading, verification, lifecycle state, and errors. `src/base/module_loader.cpp` is a deprecated legacy copy, not the canonical implementation. |
| Sandbox and ABI | `include/themis/base/module_sandbox.h`; `src/base/module_sandbox.cpp` | Configurable OS constraints, ABI checks, warnings, and optional WASM integration. |
| WASM | `include/themis/base/wasm_plugin_sandbox.h`, `wasm_runtime_injector.h`; corresponding `src/base/*.cpp` | WASM validation, allowlisting, runtime selection, and execution when a runtime is registered. |
| Reload and dependencies | `include/themis/base/hot_reload_manager.h`, `plugin_dependency_graph.h`; corresponding `src/base/*.cpp` | Reload/rollback coordination, tracing hooks, dependency graph analysis. |
| Remote registry and experiments | `include/themis/base/remote_registry_client.h`, `ab_test_manager.h`; corresponding `src/base/*.cpp` | Registry intake and A/B test lifecycle support. |
| Resource pools | `include/base/buffer_pool.h`, `resource_pool_manager.h`; `src/base/buffer_pool.cpp`, `resource_pool_manager.cpp` | Slab-backed buffer handles and generic connection-slot/resource-pool coordination. |

`src/base/` currently contains ten `.cpp` files. The explicit `themis_core`
source list in `cmake/ModularBuild.cmake` includes the seven lifecycle, sandbox,
registry, and WASM/dependency implementation files under `src/base/`, plus the
canonical loader under `src/themis/`. It does not list the legacy
`src/base/module_loader.cpp` or the two resource-pool `.cpp` files. Their
intended build integration is not established by that source list and remains
an open verification item.

## Runtime flow and limits

1. The loader checks a candidate artifact and its metadata/policy before
   activation; dependency graph analysis is available to callers as a separate
   ordering/checking surface.
2. Sandbox constraints are applied only when configured and supported. Missing
   platform mechanisms may be reported as warnings or degraded state.
3. WASM binaries can be validated without an injected runtime; execution through
   `callExport()` requires a runtime implementation.
4. Hot reload reports a `HotReloadResult` and retains rollback information
   according to its configuration. Do not treat this as a guarantee of
   uninterrupted atomic replacement.
5. Registry requests accept HTTP or HTTPS; plain HTTP is permitted with a
   warning, so transport encryption is not universal.
6. Buffer-pool requests above 4 KiB use the system allocator; buffer-pool
   sources are not present in the explicit `themis_core` source list noted
   above.

## Build and test evidence

- Module sources are listed in `cmake/ModularBuild.cmake`; benchmarks are
  conditionally registered from `benchmarks/CMakeLists.txt`.
- `tests/base/CMakeLists.txt` describes focused test executables and a
  separately labelled soak test. No inclusion of this module-local CMake file
  was found in the repository's `tests/CMakeLists.txt`.
- The root test CMake file explicitly registers a hot-reload regression test
  and a BaseEntity field test (the latter exercises the entity/storage path).
  This does not establish that every test file under `tests/base/` is part of
  the standard build.
- No build or test run was performed for this documentation audit.

See [API.md](API.md), [ARCHITECTURE.md](ARCHITECTURE.md),
[MODULE_GUIDE.md](MODULE_GUIDE.md), [ROADMAP.md](ROADMAP.md), and
[FUTURE_ENHANCEMENTS.md](FUTURE_ENHANCEMENTS.md). Header contracts are the
primary API evidence; use the build/test files linked above for registration
details.
