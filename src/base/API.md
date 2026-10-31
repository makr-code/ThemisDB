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
SOT domain: api-contract
Canonical references: `include/base/buffer_pool.h`, `include/base/resource_pool_manager.h`, `include/themis/base/module_loader.h`, `include/themis/base/hot_reload_manager.h`, `include/themis/base/plugin_dependency_graph.h`
Related issues: #7093, #7167

# Base API contract

## Resource-pool APIs

- `BufferPool::acquire(bytes)` returns a `BufferHandle`; handles are move-only and auto-release unless configured otherwise. `statistics()` reports live allocations, slab hits/misses, fallbacks, and per-class counts.
- `AdaptiveConnectionPool::acquire(timeout, slot_id)` returns `bool`; callers must handle timeout/failure and release acquired slot IDs. `shutdown()` and statistics are exposed.
- `AdaptiveConnectionPool` manages generic integer slots, not concrete connection objects. `ResourcePoolManager` owns only the connection-slot and buffer pools. The header's thread-pool/leak-detection design-goal text is not implemented in the inspected class/source: there is no thread-pool member or leak-reclamation path, and `Config::leak_timeout` is not read by `resource_pool_manager.cpp`.

## Module lifecycle APIs

- `ModuleLoader` declares staged load states (`UNLOADED`, verification/validation/staging/activation states, `ACTIVE`, and failure/unload states) plus typed module errors.
- `HotReloadManager` returns a `HotReloadResult` for reload/rollback operations. Callers must inspect `success` and preserve failure/rollback information. The public header contracts an atomic swap, but the implementation later unloads and reloads under the canonical key; record this contract/implementation discrepancy and do not claim uninterrupted availability until reconciled.
- `PluginDependencyGraph` provides dependency/cycle/order analysis; use its result rather than assuming caller-supplied module order is valid.

## Caller obligations and edge cases

- Respect ownership and lifetime of buffer handles and pool slots.
- A timed-out pool acquisition does not grant a usable slot.
- Treat module load, verification, dependency, and reload failures as failures; do not activate or report a candidate solely because a file was found.
- These are separate APIs under `include/base/` and `include/themis/base/`; include the namespace and header that match the surface in use.

`include/base/` currently contains only the two resource-pool headers listed
above; module lifecycle headers live under `include/themis/base/`.

`tests/CMakeLists.txt` discovers module CMake files and adds `tests/base/`.
The base file registers its `test_*.cpp` sources as focused unit tests and a
separate focused soak test with a 7200-second timeout. The registration helper
adds `module:base`, `tier:unit`/`tier:soak`, `kind:focused`, and `focus`; the
base file adds `base` and `soak` labels. The root test file also separately
registers the reload-regression and BaseEntity field tests. These definitions
establish registration, not build or execution success. See `IMPLEMENTATION.md`
and `MODULE_GUIDE.md`.
