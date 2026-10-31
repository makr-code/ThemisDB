---
Author: "ThemisDB Contributors"
Created: "2026-10-21"
Last Updated: "2026-10-31"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot coding agent"
Human_Review: false
Human_Author: "ThemisDB Contributors"
---

Exact model backend not verifiable from final document text alone.

Source level: 1
SOT domain: module-behavior
Canonical references: `include/base/buffer_pool.h`, `include/base/resource_pool_manager.h`, `include/themis/base/module_loader.h`, `include/themis/base/hot_reload_manager.h`, `src/base/module_loader.cpp`, `src/base/hot_reload_manager.cpp`, `tests/base/test_base_reload_regression.cpp`
Related issues: #7093, #7167

# Base implementation

## Purpose and boundary

For developers changing shared runtime resources and module lifecycle facilities implemented in `src/base/`. The public surface spans both `include/base/` and `include/themis/base/`; do not assume the latter is the only base API.

## Implementation map

- `BufferPool` provides slab-backed buffers through move-only `BufferHandle`; the declared slab classes range from 128 B through 4 KiB.
- `AdaptiveConnectionPool` manages generic integer slots and provides timed acquisition, release, shutdown, scaling, and statistics; it does not own connection objects.
- `ResourcePoolManager` owns only connection-slot and buffer pools. Although its header describes a thread-pool facade and includes `leak_timeout`, the class has no thread-pool member and the implementation does not read that field or reclaim timed-out borrows.
- The canonical loader is `src/themis/module_loader.cpp`; `src/base/module_loader.cpp` is a deprecated legacy copy. `hot_reload_manager.cpp` implements reload behavior declared under `include/themis/base/`.
- `buffer_pool.cpp` and `resource_pool_manager.cpp` implement the two public headers under `include/base/`. These files are absent from the modular `THEMIS_BASE_SOURCES` list in `cmake/ModularBuild.cmake` and from the monolithic `THEMIS_CORE_SOURCES` list inspected; normal-library integration is unconfirmed.

## Invariants and failure cases

- Release buffer handles through their ownership contract; `BufferHandle` is move-only and releases automatically by default.
- `BufferPool::acquire()` is `noexcept`; requests outside slab classes may use the allocator fallback described in its header.
- A connection acquisition can time out (`acquire()` returns false); shutdown and capacity are observable states, not successful acquisition.
- Keep module load/reload stages and result outcomes explicit. Do not claim that a candidate became active when the reload result reports failure.

## Change and validation steps

1. Identify whether the change is allocator, connection pool, module loader, or reload behavior.
2. Check the relevant header under `include/base/` or `include/themis/base/` and trace its implementation; the canonical loader implementation is under `src/themis/`.
3. `tests/CMakeLists.txt` auto-discovers `tests/base/CMakeLists.txt` and adds the module directory. The module CMake registers the current `test_*.cpp` sources as focused unit tests, with `test_base_soak.cpp` registered separately as a focused soak target. The root file also explicitly registers focused targets for `test_base_reload_regression.cpp` and `test_base_entity_focused.cpp`.
4. This establishes CMake registration only, not successful compilation or test execution. This documentation audit did not build or run targets.

See `API.md`, `MODULE_GUIDE.md`, and `SECURITY.md`.
