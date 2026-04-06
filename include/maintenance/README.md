<!-- Status: current | validated: 2026-04-06 -->

# Maintenance Module — Public Headers

Scheduled database maintenance orchestration for ThemisDB: compaction, replica validation, MVCC cleanup, and health reporting.

## Header Listing

| Header | Purpose |
|--------|---------|
| `database_maintenance_orchestrator.h` | Central maintenance coordinator |
| `i_maintenance_task_handler.h` | Task handler base interface |
| `maintenance_task_handler_impls.h` | Built-in handlers (compaction, replica, MVCC, function) |
| `maintenance_task.h` | Task and dependency types |
| `maintenance_schedule.h` | Cron-based schedule entry |
| `maintenance_schedule_store.h` | Schedule CRUD |
| `maintenance_health_report.h` | Aggregated health reporting |

## Links

- Implementation: `../../src/maintenance/`
- ARCHITECTURE.md · AUDIT.md · CHANGELOG.md · ROADMAP.md · SECURITY.md · FUTURE_ENHANCEMENTS.md
