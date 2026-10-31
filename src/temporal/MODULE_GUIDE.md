---
Author: "ThemisDB Team"
Created: "2026-10-31"
Last Updated: "2026-10-31"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
---

# Temporal module guide

This guide helps developers orient themselves before changing or integrating
temporal behavior. Start with [`README.md`](README.md) for scope and
[`ARCHITECTURE.md`](ARCHITECTURE.md) for subsystem boundaries.

## Source map

- `include/temporal/` owns the C++ interfaces, including time types,
  system-versioned and bitemporal tables, query/index, lifecycle, CDC, and
  conflict APIs.
- `src/temporal/` implements those interfaces; core files are grouped by
  responsibility in [`IMPLEMENTATION.md`](IMPLEMENTATION.md).
- `tests/temporal/` contains the focused module tests. Its CMake file registers
  each `test_*.cpp` as a focused test target.
- `benchmarks/temporal/` contains query, interval-tree, and dedicated/release
  benchmark sources. See [`PERFORMANCE_EXPECTATIONS.md`](PERFORMANCE_EXPECTATIONS.md)
  for the performance-gate mapping.

## Recommended reading by task

| Task | Read |
|---|---|
| Understand range and version semantics | [`MODELING.md`](MODELING.md), then [`API.md`](API.md) |
| Change query/index behavior | [`ARCHITECTURE.md`](ARCHITECTURE.md), [`IMPLEMENTATION.md`](IMPLEMENTATION.md), the related header and focused tests |
| Change retention or snapshot behavior | [`OPERATIONS.md`](OPERATIONS.md), [`RUNBOOK.md`](RUNBOOK.md), `retention_manager.h`, `snapshot_manager.h` |
| Change CDC or conflicts | `temporal_cdc.h` or `temporal_conflict_resolver.h`, corresponding tests, and [`TRUST.md`](TRUST.md) |
| Add embedding-related fields | [`EMBEDDING.md`](EMBEDDING.md); embedding generation and similarity are outside this module's API |

## Integration boundaries

The architecture document identifies storage and utils as direct upstream
dependencies and describes the storage/temporal tier interface. Keep storage
ownership, query planning, embedding generation, and deployment authentication
in their owning modules. Temporal APIs accept in-process tables, callbacks,
policies, and JSON payloads; the caller owns the lifetime and authorization
context that surrounds those objects.

Before changing time values, verify both the namespace and unit: the runtime
uses `themisdb::temporal` milliseconds and half-open ranges, whereas the
separate `themis::temporal` contract header names nanosecond constants. See
[`MODELING.md`](MODELING.md) for this boundary and its evaluation implications.

Exact model backend not verifiable from final document text alone.
