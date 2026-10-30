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
SOT domain: api-contract
Canonical references: `include/base/buffer_pool.h`, `include/base/resource_pool_manager.h`, `include/themis/base/module_loader.h`, `include/themis/base/hot_reload_manager.h`, `include/themis/base/plugin_dependency_graph.h`
Related issues: #7093, #7167

# Base API contract

## Resource-pool APIs

- `BufferPool::acquire(bytes)` returns a `BufferHandle`; handles are move-only and auto-release unless configured otherwise. `statistics()` reports live allocations, slab hits/misses, fallbacks, and per-class counts.
- `AdaptiveConnectionPool::acquire(timeout, slot_id)` returns `bool`; callers must handle timeout/failure and release acquired slot IDs. `shutdown()` and statistics are exposed.

## Module lifecycle APIs

- `ModuleLoader` declares staged load states (`UNLOADED`, verification/validation/staging/activation states, `ACTIVE`, and failure/unload states) plus typed module errors.
- `HotReloadManager` returns a `HotReloadResult` for reload/rollback operations. Callers must inspect `success` and preserve failure/rollback information.
- `PluginDependencyGraph` provides dependency/cycle/order analysis; use its result rather than assuming caller-supplied module order is valid.

## Caller obligations and edge cases

- Respect ownership and lifetime of buffer handles and pool slots.
- A timed-out pool acquisition does not grant a usable slot.
- Treat module load, verification, dependency, and reload failures as failures; do not activate or report a candidate solely because a file was found.
- These are separate APIs under `include/base/` and `include/themis/base/`; include the namespace and header that match the surface in use.

Test related behavior through the targets registered in `tests/base/CMakeLists.txt`; see `IMPLEMENTATION.md` and `MODULE_GUIDE.md`.
