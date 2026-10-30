---
Author: "ThemisDB Team"
Created: "2026-10-30"
Last Updated: "2026-10-30"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
---

# Access Model Module Guide

## Who owns what

`src/access_model/` owns coordinator and policy implementation. Public C++
contracts live in `include/access_model/`. Cache owns its cache tiers and
eviction signals; storage owns persistent tiers and hot-access signals. The
access model receives those signals through `EvictionListener` and
`PromotionListener`; it is not the owner of the cache or storage backends.

Start with [`ARCHITECTURE.md`](./ARCHITECTURE.md) for boundaries,
[`API.md`](./API.md) for signatures, and [`IMPLEMENTATION.md`](./IMPLEMENTATION.md)
for current behavior and limitations.

## Developer document map

- [`RESEARCH.md`](./RESEARCH.md) — assumptions, method, evaluation, and limits
- [`MODELING.md`](./MODELING.md) — tier/event model and policy rules
- [`EMBEDDING.md`](./EMBEDDING.md) — embedding boundary and validation ownership
- [`RUNBOOK.md`](./RUNBOOK.md) — incident diagnosis and safe response
- [`OPERATIONS.md`](./OPERATIONS.md) — telemetry, capacity limits, and lifecycle
- [`TRUST.md`](./TRUST.md) — trust boundaries, exposures, and controls

## Integrating an adapter

1. Implement `AccessTier` for the tier operations and health/capacity queries
   required by the interface. Use `CacheTier` or `StorageTier` when the adapter
   also emits the corresponding notification.
2. Keep the coordinator in an owning `shared_ptr` for at least as long as any
   registered listener can call it. Cache/storage listener setters use raw
   listener pointers; unregister them before the coordinator is destroyed.
3. Construct the coordinator with a worker count and explicit queue limit for
   the deployment, build a map keyed by each adapter's `TierLevel`, and call
   `initialize()`. Initialization rejects unhealthy, mismatched, or incomplete
   cache/storage registries; it does not call adapter `initialize()`.
4. Set an `AgeBasedPolicy`; invalid policies throw `std::invalid_argument`.
   Call `start()` only after initialization succeeds.
5. Register the coordinator as each adapter's listener. Confirm which callbacks
   the adapter actually emits; listener registration alone does not move data.
6. Observe coordinator counters, transition records, and structured logs. Verify
   the adapter's actual read/write effects independently.
7. Unregister listeners, then call `shutdown()` before releasing the coordinator.
   Shutdown rejects new queued work, drains accepted tasks, and joins workers.
   A blocked backend adapter may delay shutdown.

The existing cache and storage modules expose listener setters in
`include/cache/adaptive_query_cache.h` and `include/storage/tiered_storage.h`.
There is no access-model feature-flag option defined in the module's CMake file
or public headers; integration is enabled by wiring the listener in the owning
application/module.

## Payload and policy guidance

Keys and values are strings at the common tier boundary. The coordinator sees
event metadata (tier, access count, age/window, size, and reason), not payload
semantics. `L3_SEMANTIC` is a tier identity and does not imply embedding
generation or similarity search; see [`EMBEDDING.md`](./EMBEDDING.md).

The event handlers use only part of `AgeBasedPolicy`. The task queue and pending
plan registry are bounded by the configured queue limit (default 10,000), but
that default is not a deployment capacity recommendation. Use the evidence
procedure in
[`WAVE_B_BENCHMARK_EVIDENCE_BUNDLE.md`](./WAVE_B_BENCHMARK_EVIDENCE_BUNDLE.md).

## Test path

`tests/access_model/` contains focused policy/coordinator, integration,
concurrency, observability, and end-to-end tests. Use
`ctest -R 'module_access_model_.*_focused' --output-on-failure` in a configured
build. A mock tier validates interface orchestration only; add adapter-level
tests for backend persistence and failure behavior.

---

**AI provenance:** Exact model backend not verifiable from final document text alone.
