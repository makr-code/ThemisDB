> **Build:** `cmake --preset linux-release && cmake --build --preset linux-release`

<!-- Status: current | validated: 2026-06-01 -->
<!-- Links: README.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md · ../../src/temporal/ARCHITECTURE.md -->

# Temporal Module — Public Header Architecture

**Module Path:** `include/temporal/`
**Implementation:** `../../src/temporal/`
**Canonical architecture doc:** [`../../src/temporal/ARCHITECTURE.md`](../../src/temporal/ARCHITECTURE.md)

---

## 1. Overview

`include/temporal/` defines the **public bi-temporal, system-versioned, and time-travel API contract** for ThemisDB. The 16 headers cover bi-temporal table management, bitemporal joins, interval-tree indexing, snapshot and retention management, temporal CDC, cold-store tiering, compression, conflict resolution, migration, and query-engine integration.

For runtime composition — temporal-index internals, retention policy enforcement, CDC pipelines, and tiering mechanics — see:
→ [`../../src/temporal/ARCHITECTURE.md`](../../src/temporal/ARCHITECTURE.md)

---

## 2. Header Groups

### 2.1 Bi-Temporal Table Contract

| Header | Public Type | Purpose |
|--------|------------|---------|
| `bi_temporal.h` | `BiTemporalTable` | Bi-temporal table with transaction-time and valid-time axes |
| `system_versioned_table.h` | `SystemVersionedTable` | SQL:2011 system-versioned table abstraction |
| `temporal_types.h` | — | Shared temporal type aliases (`ValidTime`, `TransactionTime`, `Period`) |

### 2.2 Joins and Indexing

| Header | Public Type | Purpose |
|--------|------------|---------|
| `bitemporal_join.h` | `BiTemporalJoin` | Aligned bi-temporal join with period-overlap semantics |
| `interval_tree_index.h` | `IntervalTreeIndex` | In-memory interval-tree index for temporal-range queries |
| `temporal_index.h` | `TemporalIndex` | Persistent temporal range index |

### 2.3 Snapshots and Retention

| Header | Public Type | Purpose |
|--------|------------|---------|
| `snapshot_manager.h` | `SnapshotManager` | Point-in-time snapshot creation and retrieval |
| `retention_manager.h` | `RetentionManager` | Policy-driven data-retention enforcement |
| `temporal_cold_store.h` | `TemporalColdStore` | Cold-tier storage backend for aged temporal data |
| `temporal_tier_manager.h` | `TemporalTierManager` | Hot/warm/cold tier orchestration |

### 2.4 Change Data Capture and Aggregation

| Header | Public Type | Purpose |
|--------|------------|---------|
| `temporal_cdc.h` | `TemporalCDC` | CDC stream of temporal row-version changes |
| `temporal_aggregator.h` | `TemporalAggregator` | Period-aware aggregations over bi-temporal ranges |

### 2.5 Compression, Conflict, and Migration

| Header | Public Type | Purpose |
|--------|------------|---------|
| `temporal_compressor.h` | `TemporalCompressor` | Delta/RLE compression for temporal history columns |
| `temporal_conflict_resolver.h` | `TemporalConflictResolver` | Conflict detection and resolution for overlapping periods |
| `temporal_migrator.h` | `TemporalMigrator` | Schema-evolution migration for versioned tables |

### 2.6 Query Engine

| Header | Public Type | Purpose |
|--------|------------|---------|
| `temporal_query_engine.h` | `TemporalQueryEngine` | AS OF / FROM … TO … / BETWEEN temporal query surface |

---

## 3. Namespace Layout

| Namespace | Scope |
|-----------|-------|
| `themis::temporal` | All bi-temporal, versioned-table, and time-travel types |

---

## 4. Public Contract Notes

- `temporal_types.h` defines the canonical `Period`, `ValidTime`, and `TransactionTime` aliases used across all temporal headers.
- Bi-temporal headers must maintain ISO SQL:2011 period semantics; overlapping period handling is defined in `temporal_conflict_resolver.h`.
- Snapshot and retention interfaces expose stable contracts; backend tiering and cold-store implementations are swappable.
- CDC headers model ordered change streams; consumers must handle version-ordered delivery.
- Compression and migration headers define transformation contracts for embedders managing long-lived temporal stores.
