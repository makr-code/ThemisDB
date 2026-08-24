# MaintenanceApiHandler Hardening Plan (2026-05-18)

## Scope
- Handler: src/server/maintenance_api_handler.cpp
- Tests: tests/test_database_maintenance_orchestrator.cpp

## Acceptance Criteria
- Reject invalid schedule/job ids before orchestrator calls.
- Reject invalid tenant_id filters in listSchedules().
- Keep existing API shape and error JSON contract.
- Add focused negative tests for the new guards.

## Validation
- Editor diagnostics on touched files.
- Focused build attempt via themis_tests (known global linker blockers may still prevent executable validation).
