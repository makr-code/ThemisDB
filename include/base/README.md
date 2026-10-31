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

# Base Resource-Pool Headers

`include/base/` currently contains the public declarations for shared resource
pools; the module lifecycle interfaces are under `include/themis/base/`.

| Header | Public surface |
|---|---|
| [`buffer_pool.h`](buffer_pool.h) | `themis::resource::BufferPool` and move-only `BufferHandle`; six slab size classes through 4 KiB, with allocator fallback for larger requests. |
| [`resource_pool_manager.h`](resource_pool_manager.h) | `themis::resource::AdaptiveConnectionPool` (generic integer slots) and `ResourcePoolManager` (connection and buffer pools). |

`ResourcePoolManager` does not expose a thread-pool member or leak-reclamation
path in the inspected class/implementation. The header's stated thread-pool
facade and leak-detection design goals are not implemented there; its
`Config::leak_timeout` field is unused. `AdaptiveConnectionPool` pools integer
slots, not connection objects.

These are not module-loader or sandbox headers. See
[`../../src/base/API.md`](../../src/base/API.md) for a cross-surface summary
and [`../../src/base/README.md`](../../src/base/README.md) for source/build
registration caveats.

## Build integration status

The matching implementation files exist at `src/base/buffer_pool.cpp` and
`src/base/resource_pool_manager.cpp`, but neither appears in
`THEMIS_BASE_SOURCES`, the explicit source list used to create `themis_base` in
modular mode, or in the monolithic `THEMIS_CORE_SOURCES` list inspected. In
modular mode, `themis_core` is a compatibility interface that links module
targets; it is not the source list for the base library. The headers and source
files therefore establish the declared API and implementation, not that these
symbols are linked into either build mode. The modular library target is named
`themis_base` (underscore), not `themis-base`.

`tests/CMakeLists.txt` discovers module CMake files with `CONFIGURE_DEPENDS`
and adds `tests/base/`. That file registers `test_*.cpp` sources as focused
unit tests and a separate focused soak test with a 7200-second timeout. The
registration helper adds `module:base`, `tier:<unit|soak>`, `kind:focused`, and
`focus` labels; the base file adds `base` and the soak target also adds `soak`.
This confirms CMake registration, not that the targets build or tests pass.
