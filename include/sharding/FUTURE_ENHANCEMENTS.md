> **Build:** `cmake --preset linux-release && cmake --build --preset linux-release`

<!-- Status: current | validated: 2026-06-01 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md · ../../src/sharding/FUTURE_ENHANCEMENTS.md -->

# Sharding Module — Public Header Future Enhancements

**Module Path:** `include/sharding/`
**Canonical implementation enhancements:** [`../../src/sharding/FUTURE_ENHANCEMENTS.md`](../../src/sharding/FUTURE_ENHANCEMENTS.md)

---

## Scope

Planned enhancements to the **public header contract** in `include/sharding/`. Distributed-runtime hardening and performance work remain tracked in:

→ [`../../src/sharding/FUTURE_ENHANCEMENTS.md`](../../src/sharding/FUTURE_ENHANCEMENTS.md)

---

## Design Constraints

- `[x]` Routing and transaction outcomes must remain explicit and deterministic.
- `[x]` Quorum-loss, migration, and repair degradation must remain observable through public types.
- `[x]` Secure transport contracts must stay independent of a single deployment backend.
- `[x]` Consensus-specific headers must keep factory/abstraction boundaries for embedders.

---

## Required Interfaces (Header Contract)

| Interface | Declared In | Consumer | Status |
|-----------|------------|----------|--------|
| `ShardRouter` routing APIs | `shard_router.h` | Query/server dispatch | ✅ Stable |
| `CrossShardTransactionCoordinator` | `cross_shard_transaction.h` | Distributed transaction layer | ✅ Stable |
| `QuorumManager` | `quorum_manager.h` | Coordination and failover flows | ✅ Stable |
| `WALManager` family | `wal_manager.h`, `wal_applier.h`, `wal_shipper.h` | Durability and replication hooks | ✅ Stable |
| `HealthMonitor` / `OperationalMetrics` | `health_monitor.h`, `operational_metrics.h` | SRE and admin surfaces | ✅ Stable |

---

## Planned Enhancements

### Short-Term (Q3 2026)

- Publish clearer header-level guidance for degraded quorum, partial migration, and repair retry scenarios.
- Standardize event/result DTO naming across routing, repair, and recovery headers.
- Expose contract notes for distributed wait-for and deadlock-detection data produced by `cross_shard_transaction.h`.

### Medium-Term (Q4 2026)

- Add `topology_change_event.h` and `distributed_deadlock_snapshot.h` as shared diagnostics/event contracts.
- Introduce stronger deprecation notes for legacy router helpers once adaptive routing is the default path.
- Document benchmark-reference expectations for routing, commit, and migration-sensitive hot paths.

### Long-Term

- Unify transport-security headers around a backend-neutral channel/security profile abstraction.
- Add extension hooks for embedders to plug in custom topology or placement policies without replacing coordinator contracts.
