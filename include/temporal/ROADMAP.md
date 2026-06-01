> **Build:** `cmake --preset linux-release && cmake --build --preset linux-release`

<!-- Status: current | validated: 2026-06-01 -->
<!-- Links: README.md · ARCHITECTURE.md · FUTURE_ENHANCEMENTS.md · ../../src/temporal/ROADMAP.md -->

# Temporal Module — Public Header Roadmap

**Module Path:** `include/temporal/`
**Canonical implementation roadmap:** [`../../src/temporal/ROADMAP.md`](../../src/temporal/ROADMAP.md)

---

## Overview

Tracks public temporal API contract stability, header coverage, and future public entry points. Runtime enforcement, CDC pipeline, and tiering work remain in:

→ [`../../src/temporal/ROADMAP.md`](../../src/temporal/ROADMAP.md)

---

## Current Status

All 16 temporal headers are present. Public entry points exist for bi-temporal tables, bitemporal joins, interval-tree and persistent indexing, snapshots, retention, cold-store tiering, CDC, period-aware aggregation, compression, conflict resolution, migration, and temporal query.

---

## Completed ✅

- [x] `bi_temporal.h`, `system_versioned_table.h`, `temporal_types.h` — core bi-temporal and SQL:2011 table contract
- [x] `bitemporal_join.h`, `interval_tree_index.h`, `temporal_index.h` — join and index surfaces
- [x] `snapshot_manager.h`, `retention_manager.h`, `temporal_cold_store.h`, `temporal_tier_manager.h` — snapshot, retention, and tiering
- [x] `temporal_cdc.h`, `temporal_aggregator.h` — CDC and period-aware aggregation
- [x] `temporal_compressor.h`, `temporal_conflict_resolver.h`, `temporal_migrator.h` — compression, conflict, and migration
- [x] `temporal_query_engine.h` — AS OF / period-range query surface

---

## In Progress

- [ ] Document period-overlap conflict semantics and resolution precedence across `temporal_conflict_resolver.h` and `bi_temporal.h` (Target: 2026-Q3)
- [ ] Clarify tier-transition triggers between hot/warm/cold in `temporal_tier_manager.h` docs (Target: 2026-Q3)

---

## Planned

- [ ] `temporal_audit_log.h` — immutable audit trail for temporal row-version changes (Target: 2026-Q4)
- [ ] Add ISO SQL:2011 conformance annotations to bi-temporal and system-versioned headers (Target: 2026-Q4)
- [ ] Expose benchmark latency targets for point-in-time snapshot retrieval hot paths (Target: 2026-Q4)

---

## Breaking Change History

None in v1.x. Temporal headers maintain backward compatibility within the active major line; period-semantics changes require migration notes and changelog updates.
