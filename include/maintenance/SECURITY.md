<!-- Status: current | validated: 2026-04-15 | Commit: e963d4e9ba -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md -->

# Security — Maintenance Module (Public Headers)

> For reporting security vulnerabilities, see the project-level [SECURITY.md](../../../SECURITY.md).

## Threat Model

| Threat | Mitigation |
|--------|------------|
| Unauthorised schedule creation / modification | Schedule CRUD requires `maintenance:write` scope; force-run requires `maintenance:admin` |
| Runaway maintenance tasks consuming resources | `OrchestratorJob` enforces configurable time windows; tasks cancelled on timeout |
| Compaction triggering data loss | `StorageCompactionHandler` validates that compaction is safe before triggering |
| Health report spoofing | `ModuleHealthSignal` signals are aggregated server-side; not accepted from external sources |
| Force-run abuse | Force-run flag requires `maintenance:admin` elevated scope; always audit-logged |
| Task handler injection via `FunctionMaintenanceTaskHandler` | Callable registration restricted to internal server startup; not exposed via HTTP API |
| Distributed lock token abuse | `InProcessDistributedLock` uses TTL expiry; production Raft-backed implementation must enforce per-node token identity |
| Cross-tenant schedule interference | Per-tenant window and quota enforced inside `executeSchedule()`; tenant_id populated from server-side schedule, not from client request |

## Known Limitations

- Raft-backed `IDistributedLock` not yet available; production multi-node deployments should inject a custom Raft-backed implementation until v2.1.0.
- `REPLICA_VALIDATION` wiring to sharding module pending; task currently succeeds via no-op unregistered-handler path.
- Implementation-level security details: `../../src/maintenance/SECURITY.md`.
