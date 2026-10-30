---
Author: "ThemisDB Contributors"
Created: "2026-10-21"
Last Updated: "2026-10-30"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
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
- `AdaptiveConnectionPool` provides timed acquisition, release, shutdown, scaling, and statistics.
- `module_loader.cpp` and `hot_reload_manager.cpp` implement module lifecycle and reload behavior declared under `include/themis/base/`.

## Invariants and failure cases

- Release buffer handles through their ownership contract; `BufferHandle` is move-only and releases automatically by default.
- `BufferPool::acquire()` is `noexcept`; requests outside slab classes may use the allocator fallback described in its header.
- A connection acquisition can time out (`acquire()` returns false); shutdown and capacity are observable states, not successful acquisition.
- Keep module load/reload stages and result outcomes explicit. Do not claim that a candidate became active when the reload result reports failure.

## Change and validation steps

1. Identify whether the change is allocator, connection pool, module loader, or reload behavior.
2. Check the relevant header under `include/base/` or `include/themis/base/` and trace its `src/base/` implementation.
3. Use `tests/base/test_base_reload_regression.cpp` for reload changes and the closest base interface/pool test.
4. Run the selected target registered by `tests/base/CMakeLists.txt`.

See `API.md`, `MODULE_GUIDE.md`, and `SECURITY.md`.
