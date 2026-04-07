<!-- Status: current | validated: 2026-04-06 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md -->

# Security — Maintenance Module

> Report vulnerabilities via the project-level [SECURITY.md](../../../SECURITY.md).

## Threat Model

| Threat | Mitigation |
|--------|-----------|
| Unauthorized schedule creation/modification | `maintenance:write` RBAC scope required; enforced by auth middleware |
| Privilege escalation via scheduled tasks | Tasks execute under server process identity; no privilege elevation |
| DoS via excessive schedule creation | Input validation; schedule count limits enforced |
| Maintenance window bypass | Window enforcement is server-side; not bypassable via client |
| Audit log manipulation | Audit log is append-only hash-chain via `AuditLogger` |
| Cron injection | Cron expressions validated strictly before registration with `TaskScheduler` |
| Health probe poisoning | Health probes registered only by trusted internal modules at startup |

## Security Controls

- RBAC enforcement: `maintenance:read`, `maintenance:write`, `maintenance:admin`
- All state-changing operations audit-logged with caller identity and timestamp
- Input validation on schedule name, tasks list, cron expression, and window hours
- Maintenance windows are UTC-based and enforced server-side
- Job cancellation requires `maintenance:admin` scope

## Data Handling

- Schedule metadata (names, task types, window hours) is non-sensitive
- Audit log entries may reference sensitive operation context — stored encrypted at rest
- No user data is processed by the maintenance orchestrator directly

## Known Limitations

- Schedules currently in memory only; lost on restart until RocksDB persistence is implemented (v1.1.0)
- Force-run override (planned v1.1.0) will bypass window enforcement — requires `maintenance:admin` scope and will be audit-logged
