> **Build:** `cmake --preset linux-release && cmake --build --preset linux-release`

<!-- Status: current | validated: 2026-06-01 -->
<!-- Links: README.md · ARCHITECTURE.md · ../../src/temporal/ROADMAP.md -->

# TEMPORAL Module — Public Header Roadmap

**Module Path:** `include/temporal/`
**Canonical implementation roadmap:** [`../../src/temporal/ROADMAP.md`](../../src/temporal/ROADMAP.md)

---

## Overview

This document tracks public API contract stability, planned header additions, and header-level breaking changes for `include/temporal/`. For feature roadmap items that affect both implementation and headers see the canonical roadmap:

→ [`../../src/temporal/ROADMAP.md`](../../src/temporal/ROADMAP.md)

---

## Current Status

production temporal query engine with bitemporal tables, system-versioned tables, snapshot/retention, and interval indexing. All production-required public headers are present and `#pragma once` guarded.

The header API surface is **stable** for all types introduced in v1.x.

---

## Completed ✅

- [x] `temporal_query_engine.h` — query and version semantics contract
- [x] `bi_temporal.h` — query and version semantics contract
- [x] `bitemporal_join.h` — query and version semantics contract
- [x] `system_versioned_table.h` — query and version semantics contract
- [x] `temporal_types.h` — query and version semantics contract
- [x] `snapshot_manager.h` — lifecycle and consistency contract
- [x] `retention_manager.h` — lifecycle and consistency contract
- [x] `temporal_conflict_resolver.h` — lifecycle and consistency contract
- [x] `temporal_migrator.h` — lifecycle and consistency contract
- [x] `temporal_tier_manager.h` — lifecycle and consistency contract
- [x] `temporal_cold_store.h` — lifecycle and consistency contract
- [x] `temporal_index.h` — indexing and throughput contract
- [x] `interval_tree_index.h` — indexing and throughput contract
- [x] `temporal_aggregator.h` — indexing and throughput contract
- [x] `temporal_cdc.h` — indexing and throughput contract
- [x] `temporal_compressor.h` — indexing and throughput contract

---

## In Progress 🚧

- [I] Header-level unit test coverage for all public interfaces (tracked via module issue backlog)

---

## Planned Features 📋

### Short-term (Next 3–6 months)

- [ ] Audit all headers for missing `[[nodiscard]]` on factory and error-returning methods (Target: Q3 2026)
- [ ] Verify `#pragma once` guard consistency across all headers in a CI step (Target: Q3 2026)

### Medium-term (6–12 months)

- [ ] Align header-level type documentation with OpenAPI spec where applicable (Target: Q4 2026)
- [ ] Consolidate deprecated symbol annotations with `[[deprecated("...")]]` where needed (Target: Q4 2026)

---

## Production Readiness Checklist

- [x] All headers have `#pragma once` guard
- [x] All public factory methods marked `[[nodiscard]]`
- [x] Build conditionals documented in `README.md` and `ARCHITECTURE.md`
- [P] Header-level unit tests (tracked in module issue backlog)

---

## References

- Canonical implementation roadmap: [`../../src/temporal/ROADMAP.md`](../../src/temporal/ROADMAP.md)
- Architecture: [`ARCHITECTURE.md`](ARCHITECTURE.md)
- Future enhancements: [`FUTURE_ENHANCEMENTS.md`](FUTURE_ENHANCEMENTS.md)
- Module overview: [`README.md`](README.md)
