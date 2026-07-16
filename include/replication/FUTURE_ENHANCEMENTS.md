> **Build:** `cmake --preset linux-release && cmake --build --preset linux-release`

<!-- Status: current | validated: 2026-06-01 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md · ../../src/replication/FUTURE_ENHANCEMENTS.md -->

# Replication Module — Public Header Future Enhancements

**Module Path:** `include/replication/`
**Canonical implementation enhancements:** [`../../src/replication/FUTURE_ENHANCEMENTS.md`](../../src/replication/FUTURE_ENHANCEMENTS.md)

---

## Scope

Planned enhancements to the **public header contract** in `include/replication/`. Runtime hardening and benchmark work remain tracked in:

→ [`../../src/replication/FUTURE_ENHANCEMENTS.md`](../../src/replication/FUTURE_ENHANCEMENTS.md)

---

## Design Constraints

- `[x]` Promotion, failover, and conflict outcomes must remain explicit and deterministic.
- `[x]` CDC and logical-stream headers must preserve backend-neutral contracts.
- `[x]` Observability headers must keep lag/topology degradation visible to operators.
- `[x]` Conflict-resolution and CRDT contracts must remain extensible for deployer-specific strategies.

---

## Required Interfaces (Header Contract)

| Interface | Declared In | Consumer | Status |
|-----------|------------|----------|--------|
| `ReplicationManager` lifecycle APIs | `replication_manager.h` | Cluster orchestration | ✅ Stable |
| `LogicalReplication` / `ReplicationSlot` | `logical_replication.h`, `replication_slot.h` | CDC and WAL consumers | ✅ Stable |
| `ConflictResolver` | `conflict_resolution.h` | Multi-writer replication paths | ✅ Stable |
| `ReplicationObservability` | `observability.h` | Operations and admin APIs | ✅ Stable |

---

## Planned Enhancements

### Short-Term (Q3 2026)

- Publish clearer state-transition and degraded-lag guidance across public orchestration headers.
- Standardize naming for replication events, stream cursors, and conflict-result DTOs.
- Add public documentation for slot-fault and backpressure-reporting expectations.

### Medium-Term (Q4 2026)

- Introduce `replication_transition_event.h` and `replication_backpressure_hint.h` for shared event/diagnostic payloads.
- Document benchmark-reference expectations for promotion, conflict, and CDC-sensitive hot paths.
- Align multi-tier and multi-master headers around shared topology terminology.

### Long-Term

- Add extension hooks for embedders to plug in custom replication transports without replacing lifecycle contracts.
- Unify logical-stream and event-stream metadata envelopes under a single replication event schema.
