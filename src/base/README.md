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

## Scope

In scope: module loading/verification, dependency analysis, optional sandbox/WASM
execution, reload/rollback, remote-registry intake, A/B experiment support, and
generic buffer/connection-slot pool APIs.

Out of scope: plugin-specific business logic, a universal host-side policy,
application-specific management commands/metrics, or unverified production
guarantees.

## Quick start

For callers using the declared resource-pool API:

```cpp
#include "base/buffer_pool.h"

themis::resource::BufferPool pool;
auto buffer = pool.acquire(512);
if (!buffer.valid()) {
    // Handle allocation/shutdown failure.
}
```

`BufferHandle` is move-only and auto-releases by default. This snippet
illustrates the header contract only; the source-list omission below means
linkage through the base module target is not confirmed.

## Source and interface map

| Surface | Current source / interface | Responsibility |
|---|---|---|
| Module loading | `include/themis/base/module_loader.h`; canonical implementation `src/themis/module_loader.cpp` and platform/security companions | Artifact loading, verification, lifecycle state, and errors. `src/base/module_loader.cpp` is a deprecated legacy copy, not the canonical implementation. |
| Sandbox and ABI | `include/themis/base/module_sandbox.h`; `src/base/module_sandbox.cpp` | Configurable OS constraints, ABI checks, warnings, and optional WASM integration. |
| WASM | `include/themis/base/wasm_plugin_sandbox.h`, `wasm_runtime_injector.h`; corresponding `src/base/*.cpp` | WASM validation, allowlisting, runtime selection, and execution when a runtime is registered. |
| Reload and dependencies | `include/themis/base/hot_reload_manager.h`, `plugin_dependency_graph.h`; corresponding `src/base/*.cpp` | Reload/rollback coordination, tracing hooks, dependency graph analysis. |
| Remote registry and experiments | `include/themis/base/remote_registry_client.h`, `ab_test_manager.h`; corresponding `src/base/*.cpp` | Registry intake and A/B test lifecycle support. |
| Resource pools | `include/base/buffer_pool.h`, `resource_pool_manager.h`; `src/base/buffer_pool.cpp`, `resource_pool_manager.cpp` | Slab-backed buffer handles and generic connection-slot/resource-pool coordination. |

`src/base/` currently contains ten `.cpp` files. In modular mode,
`cmake/ModularBuild.cmake` uses `THEMIS_BASE_SOURCES` to create the shared
`themis_base` target. That list includes seven lifecycle, sandbox, registry,
and WASM/dependency implementation files under `src/base/`, plus the canonical
loader at `src/themis/module_loader.cpp` and its platform/security companions.
It omits the deprecated `src/base/module_loader.cpp` and the two resource-pool
`.cpp` files. The monolithic `THEMIS_CORE_SOURCES` list inspected also omits the
resource-pool files. In modular mode `themis_core` is an interface target that
links module targets; it is not the base source list. Resource-pool linkage
remains unconfirmed.

## Runtime flow and limits

1. The loader checks a candidate artifact and its metadata/policy before
   activation; dependency graph analysis is available to callers as a separate
   ordering/checking surface.
2. Sandbox constraints are applied only when configured and supported. Missing
   platform mechanisms may be reported as warnings or degraded state.
3. WASM binaries can be validated without an injected runtime; execution through
   `callExport()` requires a runtime implementation.
4. Hot reload reports a `HotReloadResult` and retains rollback information
   according to its configuration. The public header contracts an atomic swap,
   but the implementation sequence appears inconsistent with that contract;
   uninterrupted availability remains unresolved pending reconciliation.
5. Registry requests accept HTTP or HTTPS; plain HTTP is permitted with a
   warning, so transport encryption is not universal.
6. Buffer-pool requests above 4 KiB use the system allocator; buffer-pool
   sources are not present in the explicit base/monolithic source lists noted
   above.
7. `AdaptiveConnectionPool` represents integer slots rather than live
   connections. `ResourcePoolManager` owns connection and buffer pools only;
   the header's thread-pool/leak-detection design goals are not implemented in
   the inspected class/source (`leak_timeout` is unused).

## Build and test evidence

- `cmake/ModularBuild.cmake` declares the `themis_base` library target in
  modular mode; `cmake/CMakeLists.txt` makes `themis_core` a compatibility
  interface target in that mode. Benchmarks are conditionally registered from
  `benchmarks/CMakeLists.txt`.
- No separate base-only configure/build recipe was found. The root build
  defaults `THEMIS_BUILD_BENCHMARKS` to OFF and defaults
  `THEMIS_BUILD_TESTS` to ON for Debug and OFF otherwise (unless overridden).
  Use the repository's configured build profile, and verify target linkage
  before relying on the resource-pool APIs.
- `tests/CMakeLists.txt` discovers module `*/CMakeLists.txt` files with
  `CONFIGURE_DEPENDS` and adds `tests/base/`; the base file registers its
  `test_*.cpp` sources as focused unit tests and a separate soak test.
- `themis_register_module_focused_test()` adds `module:base`,
  `tier:unit`/`tier:soak`, `kind:focused`, and `focus` labels; the base file
  also adds `base` and, for the soak target, `soak`. The soak timeout is 7200
  seconds (two hours), and it is selectable with `ctest -L soak`.
- The root test CMake file separately registers a hot-reload regression test
  and a BaseEntity field test (the latter exercises the entity/storage path).
- Registration is established by the CMake definitions; no build or test run
  was performed for this documentation audit.

See [API.md](API.md), [ARCHITECTURE.md](ARCHITECTURE.md),
[MODULE_GUIDE.md](MODULE_GUIDE.md), [ROADMAP.md](ROADMAP.md), and
[FUTURE_ENHANCEMENTS.md](FUTURE_ENHANCEMENTS.md). Header contracts are the
primary API evidence; use the build/test files linked above for registration
details.
