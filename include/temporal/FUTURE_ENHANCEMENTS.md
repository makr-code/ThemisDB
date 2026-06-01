> **Build:** `cmake --preset linux-release && cmake --build --preset linux-release`

<!-- Status: current | validated: 2026-06-01 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md · ../../src/temporal/FUTURE_ENHANCEMENTS.md -->

# Temporal Module — Public Header Future Enhancements

**Module Path:** `include/temporal/`
**Canonical implementation enhancements:** [`../../src/temporal/FUTURE_ENHANCEMENTS.md`](../../src/temporal/FUTURE_ENHANCEMENTS.md)

---

## Scope

Planned enhancements to the **public header contract** in `include/temporal/`. Runtime enforcement, tiering, and CDC pipeline work remain tracked in:

→ [`../../src/temporal/FUTURE_ENHANCEMENTS.md`](../../src/temporal/FUTURE_ENHANCEMENTS.md)

---

## Design Constraints

- `[x]` Bi-temporal headers must maintain ISO SQL:2011 period semantics across valid-time and transaction-time axes.
- `[x]` Snapshot and retention interfaces must remain backend-agnostic for cold-store swapability.
- `[x]` CDC headers must model ordered version streams; consumers handle version-ordered delivery.
- `[x]` Conflict-resolution headers must expose deterministic period-overlap semantics.

---

## Required Interfaces (Header Contract)

| Interface | Declared In | Consumer | Status |
|-----------|------------|----------|--------|
| `BiTemporalTable` CRUD / period APIs | `bi_temporal.h` | Query engine, migration services | ✅ Stable |
| `SnapshotManager` point-in-time retrieval | `snapshot_manager.h` | Audit and rollback services | ✅ Stable |
| `TemporalCDC` change-stream subscription | `temporal_cdc.h` | Replication and event processors | ✅ Stable |
| `TemporalQueryEngine` AS OF query | `temporal_query_engine.h` | AQL and SQL query layers | ✅ Stable |
| `RetentionManager` policy API | `retention_manager.h` | Operations and compliance tooling | ✅ Stable |

---

## Planned Enhancements

### Short-Term (Q3 2026)

- Document period-overlap conflict semantics and resolution precedence uniformly across conflict-resolver and bi-temporal headers.
- Clarify hot/warm/cold tier-transition triggers and policy contract in `temporal_tier_manager.h`.
- Add ISO SQL:2011 conformance annotations to bi-temporal and system-versioned table headers.

### Medium-Term (Q4 2026)

- Introduce `temporal_audit_log.h` to expose an immutable audit trail of temporal row-version mutations.
- Expose benchmark-reference latency notes for point-in-time snapshot retrieval and period-range index hot paths.
- Add deprecation guidance for any pre-SQL:2011 temporal APIs and document migration paths.

### Long-Term

- Unify period-aware aggregation and join result types behind a shared temporal-context envelope.
- Add extension hooks for embedders to inject custom cold-store backends beyond the default RocksDB integration.
- Provide temporal-query explain plans via `temporal_query_engine.h` to aid consumer-side optimisation.
