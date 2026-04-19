# Temporal Module — Architecture Guide

**Version:** 1.2
**Last Updated:** 2026-04-06
**Module Path:** `src/temporal/`

---

## 1. Overview

The Temporal module implements bitemporal data management for ThemisDB: transaction-time
(when data was stored) and valid-time (when data was real-world valid) tracking, time-travel
queries (`AS OF`, `FROM...TO`), bitemporal period operations (OVERLAPS, CONTAINS, PRECEDES),
and temporal data retention. It complements the timeseries module (which handles metric
streams) by providing ISO SQL:2011-compatible temporal tables for structured records.

---

## 2. Design Principles

- **Bitemporality** – two orthogonal time axes: system time (controlled by ThemisDB) and
  valid time (controlled by the application). Together they fully describe when data was
  known and when it was true.
- **HLC Timestamps** – Hybrid Logical Clocks ensure correct causal ordering across
  distributed nodes without relying on synchronized wall clocks.
- **Non-Destructive Updates** – updates create new versions; old versions are retained
  until retention policy expires them.
- **Temporal Index** – `temporal_index.cpp` maintains efficient period-based B-tree
  indexes to accelerate time-bounded queries.

---

## 3. Component Architecture

### 3.1 Key Components

| File | Role |
|---|---|
| `temporal_query_engine.cpp` | AS OF, FROM...TO, BETWEEN...AND query execution; bitemporal joins; SEQUENCED/NON-SEQUENCED semantics |
| `bi_temporal.cpp` | Bitemporal record management (system + valid time axes) |
| `bitemporal_join.cpp` | Bitemporal join operator (combined transaction-time + valid-time predicates) |
| `system_versioned_table.cpp` | Automatic system-time versioning of all table rows |
| `temporal_index.cpp` | Period-based index for efficient time range queries |
| `temporal_aggregator.cpp` | Temporal aggregations (tumbling and sliding window) |
| `temporal_conflict_resolver.cpp` | HLC-based conflict resolution for concurrent edits |
| `snapshot_manager.cpp` | Temporal snapshot creation and restoration |
| `retention_manager.cpp` | Automated expiry of old versions based on retention policy |
| `interval_tree_index.cpp` | Augmented BST interval tree; O(log n + k) overlap detection for valid-time period predicates |
| `temporal_compressor.cpp` | DELTA/ZSTD/Gorilla/dictionary compression for historical version payloads |
| `temporal_cdc.cpp` | Versioned CDC engine; typed ChangeEvent pub/sub with bounded ring-buffer replay |

### 3.2 Component Diagram

```
┌─────────────────────────────────────────────────────────────────┐
│           AQL: SELECT * FROM orders AS OF '2025-12-31'          │
└──────────────────────────┬──────────────────────────────────────┘
                           │
┌──────────────────────────▼──────────────────────────────────────┐
│               TemporalQueryEngine                                │
│                                                                  │
│  AS OF(t) → filter: sys_start <= t <= sys_end                   │
│  FROM t1 TO t2 → filter: sys_start < t2 AND sys_end > t1        │
│  BETWEEN(t1,t2) → bitemporal period overlap                     │
└──────────────────────────┬──────────────────────────────────────┘
                           │ temporal predicate
┌──────────────────────────▼──────────────────────────────────────┐
│                  TemporalIndex                                   │
│   B-tree on (sys_start, sys_end) → fast period lookup           │
└──────────────────────────┬──────────────────────────────────────┘
                           │
┌──────────────────────────▼──────────────────────────────────────┐
│                 SystemVersionedTable / BiTemporal               │
│  on write: record (sys_start=now, sys_end=∞, val_start, val_end) │
│  on update: close old version (sys_end=now), open new version   │
└──────────────────────────────────────────────────────────────────┘

┌──────────────────────────────────────────────────────────────────┐
│                    Phase 4 Components                            │
│ interval_tree_index.cpp → max-end augmented BST, overlap queries │
│ temporal_compressor.cpp → DELTA/ZSTD/Gorilla/dictionary compress │
│ temporal_cdc.cpp        → ChangeEvent pub/sub, ring-buffer replay│
└──────────────────────────────────────────────────────────────────┘
```

---

## 4. Data Flow

### 4.1 System-Versioned Insert

```
INSERT INTO contracts(id, value, valid_from, valid_to) VALUES (...)
    │
    ├─ system_versioned_table.cpp:
    │       write record with sys_start=HLC.now(), sys_end=MAX_TIMESTAMP
    │       val_start=valid_from, val_end=valid_to
    │
    └─ temporal_index: insert period (sys_start, sys_end, pk)
```

### 4.2 Time-Travel Query

```
SELECT * FROM contracts AS OF TIMESTAMP '2025-06-01'
    │
    ├─ temporal_query_engine: resolve target_time = '2025-06-01'
    ├─ temporal_index: range scan → all versions where
    │       sys_start <= target_time AND sys_end >= target_time
    └─ return matching versions
```

### 4.3 Conflict Resolution

```
Two concurrent updates to same record (distributed nodes):
    │
    ├─ temporal_conflict_resolver: compare HLC timestamps
    ├─ policy: LAST_WRITE_WINS → higher HLC wins
    │   or     FIRST_WRITE_WINS → lower HLC wins
    │   or     CRDT_MERGE → merge using CRDT semantics
    └─ winner stored; loser version retained in history (auditable)
```

---

## 5. Integration Points

| Direction | Module | Interface |
|---|---|---|
| **Uses** | `src/storage/` | Version storage via MVCC and `history_manager.cpp` |
| **Uses** | `src/storage/hlc.cpp` | HLC timestamp generation |
| **Called by** | `src/query/` | Temporal AQL operators |
| **Uses** | `src/scheduler/` | Retention policy enforcement (scheduled jobs) |
| **Uses** | `src/observability/` | Temporal query metrics |

---

## 6. Threading & Concurrency Model

- Temporal index reads are lock-free (MVCC snapshot on storage).
- Temporal writes hold a per-record lock to prevent interleaved version creation.
- `retention_manager` runs as a background scheduled job.
- `temporal_conflict_resolver` is stateless per invocation.

---

## 7. Performance Architecture

| Technique | Detail |
|---|---|
| Temporal index | B-tree on period ranges avoids full scan for time-bounded queries |
| HLC monotonicity | Monotonically increasing timestamps; no clock skew handling needed |
| Retention pruning | Expired versions deleted in background; no read-time overhead |
| Interval tree | O(log n + k) overlap queries instead of O(n) full scan for period predicates |
| History compression | TemporalCompressor reduces storage overhead 3–10× for historical version payloads |

---

## 8. Security Considerations

- Temporal history is immutable; retention is the only deletion mechanism.
- All temporal write operations are auditable via `schema_audit_log.cpp`.
- Time-travel queries are subject to the same RBAC and RLS as current-time queries.

---

## 9. Configuration

| Parameter | Default | Description |
|---|---|---|
| `temporal.system_versioned.enabled` | true | Enable system-time versioning |
| `temporal.retention.default_days` | 365 | Default historical retention |
| `temporal.conflict.policy` | "last_write_wins" | Conflict resolution strategy |
| `temporal.index.enabled` | true | Enable temporal period index |

---

## 10. Error Handling

| Error Type | Strategy |
|---|---|
| Invalid AS OF timestamp | Return structured error |
| Conflict detected | Apply resolution policy; log conflict event |
| Version chain overflow | Log warning; trigger retention cleanup |
| Index inconsistency | Rebuild temporal index from version chain |

---

## 11. Known Limitations & Future Work

- SQL `PERIOD FOR` DDL syntax is not yet supported; application-time periods must be managed via the C++ API (`BiTemporalTable`).
- No automatic SQL-level retention syntax (`ALTER TABLE … SET RETENTION_PERIOD`); retention policies must be set programmatically via `RetentionManager::setPolicy()`.
- History table compression is implemented via `TemporalCompressor` (DELTA, ZSTD, Gorilla, DICTIONARY algorithms; see `temporal_compressor.cpp`).
- Temporal CDC is available via `TemporalCDC` (in-process pub/sub with bounded ring-buffer; Kafka integration deferred to Phase 5).
- Temporal foreign keys with period-aware referential integrity are implemented via `TemporalForeignKey::validate()` (C++ layer); CASCADE/RESTRICT at SQL layer deferred to Phase 5.

---

## 12. References

- `src/temporal/README.md` — module overview
- `docs/temporal_data_guide.md` — temporal data guide
- ISO SQL:2011 — Temporal database standard
- `ARCHITECTURE.md` (root) — full system architecture
