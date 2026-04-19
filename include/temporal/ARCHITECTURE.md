<!-- Status: current | validated: 2026-04-06 -->

# Architecture — include/temporal/

> Public-header interface layer for the ThemisDB Temporal subsystem.
> Implementation details live in [`../../src/temporal/`](../../src/temporal/).

---

## Overview

The `include/temporal/` directory exposes the **public C++ API** for all temporal
data-management capabilities of ThemisDB.  These headers define the contracts
that application code and plugin authors depend on; the concrete implementations
reside in `../../src/temporal/`.

The temporal subsystem provides:
- **Bi-temporal storage** – independent valid-time and transaction-time axes
- **Interval indexing** – augmented-BST interval-tree for O(log n + k) overlap queries
- **Change-data capture** – pub/sub ring-buffer CDC with replay support
- **Temporal compression** – pluggable DELTA / ZSTD / Gorilla / DICTIONARY strategies
- **Conflict resolution** – last-write-wins and custom user-defined strategies
- **Retention & snapshots** – policy-driven data lifecycle management

---

## Design Principles

1. **Separation of interface and implementation** — headers declare stable ABI
   contracts; all implementation symbols are compiled into `libtemporal`.
2. **Composability** — individual components (`IntervalTreeIndex`,
   `TemporalCompressor`, `TemporalCDC`, …) are independently usable; they share
   only the primitive types defined in `temporal_types.h`.
3. **Zero-cost abstractions** — performance-critical paths (overlap query,
   Gorilla encoding) are template-based or inlined; runtime polymorphism is
   limited to strategy/plugin boundaries.
4. **Temporal correctness by design** — the bi-temporal model enforces
   non-overlapping valid-time periods at the API level; the query engine
   transparently handles transaction-time versioning.
5. **Extensibility** — compression strategies, conflict resolvers, and CDC
   listeners are injected via stable virtual interfaces, enabling third-party
   extensions without recompiling the core.

---

## Interface Inventory

| Header | Classes / Interfaces | Purpose |
|--------|---------------------|---------|
| `temporal_types.h` | `TimePoint`, `TimeInterval`, `TransactionTime`, `ValidTime`, `BiTemporalKey` | Primitive temporal types shared by all other headers |
| `bi_temporal.h` | `BiTemporalRecord<T>`, `BiTemporalStore<T>` | Bi-temporal storage model (valid-time + transaction-time axes) |
| `bitemporal_join.h` | `BiTemporalJoin<L,R>`, `JoinStrategy` | Temporal equi-join and overlap-join between bi-temporal relations |
| `interval_tree_index.h` | `IntervalTreeIndex<K,V>`, `OverlapResult<V>` | Augmented-BST interval tree; max-end tracking; O(log n + k) overlap queries |
| `temporal_index.h` | `TemporalIndex<K>`, `TemporalIndexFactory` | Factory and base interface for temporal index variants |
| `temporal_aggregator.h` | `TemporalAggregator<T>`, `WindowSpec`, `AggregateResult<T>` | Sliding/tumbling/session window aggregations over temporal streams |
| `temporal_query_engine.h` | `TemporalQueryEngine`, `TemporalQueryPlan`, `TemporalQueryResult` | High-level query API: AS-OF, BETWEEN, CONTAINED IN, OVERLAPS |
| `temporal_cdc.h` | `TemporalCDC`, `CDCEvent`, `CDCListener`, `CDCRingBuffer` | Change-data capture; pub/sub ring-buffer (capacity 65 536); `replayChanges()` |
| `temporal_compressor.h` | `TemporalCompressor`, `CompressionStrategy`, `DeltaStrategy`, `GorillaStrategy`, `ZstdStrategy`, `DictionaryStrategy` | Pluggable compression for temporal column data |
| `temporal_conflict_resolver.h` | `ConflictResolver`, `LastWriteWinsResolver`, `CustomResolver` | Bi-temporal write-conflict resolution strategies |
| `retention_manager.h` | `RetentionManager`, `RetentionPolicy`, `RetentionRule` | Policy-driven expiry and archival of historical records |
| `snapshot_manager.h` | `SnapshotManager`, `Snapshot`, `SnapshotCatalog` | Point-in-time snapshot creation, enumeration, and restoration |
| `system_versioned_table.h` | `SystemVersionedTable<T>`, `SystemPeriod` | SQL:2011-style system-versioned (transaction-time) tables |

---

## Component Interaction Diagram

```
temporal_types.h  ◄──── shared primitives ──────────────────────────┐
     │                                                                │
     ├─► bi_temporal.h ──────────────────────────► bitemporal_join.h │
     │        │                                                       │
     │        ▼                                                       │
     ├─► system_versioned_table.h                                     │
     │                                                                │
     ├─► interval_tree_index.h ◄─── temporal_index.h                 │
     │        │                                                       │
     │        └──────────────────► temporal_query_engine.h ──────────┘
     │
     ├─► temporal_cdc.h
     ├─► temporal_compressor.h
     ├─► temporal_conflict_resolver.h
     ├─► temporal_aggregator.h
     ├─► retention_manager.h
     └─► snapshot_manager.h
```

---

## Implementation Reference

> All `.cpp` translation units, internal helpers, and benchmark harnesses are
> located in **`../../src/temporal/`**.  Do not include headers from `src/`
> directly; the public API is fully described here.
