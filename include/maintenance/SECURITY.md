<!-- Status: current | validated: 2026-04-06 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md -->

# Security — Maintenance Module (Public Headers)

> For reporting security vulnerabilities, see the project-level [SECURITY.md](../../../SECURITY.md).

## Threat Model

| Threat | Mitigation |
|--------|------------|
| Unauthorised schedule creation / modification | Schedule CRUD requires `admin:maintenance:write` scope |
| Runaway maintenance tasks consuming resources | `OrchestratorJob` enforces configurable time windows; tasks cancelled on timeout |
| Compaction triggering data loss | `StorageCompactionHandler` validates that compaction is safe before triggering |
| Health report spoofing | `ModuleHealthSignal` signals are aggregated server-side; not accepted from external sources |
| Force-run abuse (planned) | Force-run flag will require `admin:maintenance:admin` elevated scope |
| Task handler injection via `FunctionMaintenanceTaskHandler` | Callable registration restricted to internal server startup; not exposed via HTTP API |

## Known Limitations

- Schedule persistence is in-memory (v1.0.0); restart loses schedules until RocksDB persistence lands in v1.1.0.
- `STORAGE_COMPACTION` handler is not yet wired to `CompactionManager`; stub in current release.
- Implementation-level security details: `../../src/maintenance/SECURITY.md`.
