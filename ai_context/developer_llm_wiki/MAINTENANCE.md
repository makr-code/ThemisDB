---
Author: "ThemisDB Team"
Created: "2026-11-01"
Last Updated: "2026-11-01"
Status: "active"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
Source_Level: "level1"
SOT_Domain: "module-behavior"
Canonical_References: "include/maintenance/database_maintenance_orchestrator.h; include/maintenance/maintenance_schedule.h; include/maintenance/maintenance_health_report.h; src/maintenance/README.md; src/maintenance/ROADMAP.md"
---

> Exact model backend not verifiable from final document text alone.

# Maintenance — Developer LLM Wiki

This page is secondary coding context. Public headers define the runtime API;
the implementation and production orchestrator tests establish current behavior.
Use the module [README](../../src/maintenance/README.md),
[architecture](../../src/maintenance/ARCHITECTURE.md),
[roadmap](../../src/maintenance/ROADMAP.md), and
[performance expectations](../../src/maintenance/PERFORMANCE_EXPECTATIONS.md)
for the detailed module documentation.

## Runtime API and boundaries

- [`DatabaseMaintenanceOrchestrator`](../../include/maintenance/database_maintenance_orchestrator.h)
  owns schedule CRUD, dispatch coordination, optional persistence, and job/
  health reporting. Schedule operations return repository `Result<T>` and
  `Error` values.
- [`MaintenanceScheduleEntry`](../../include/maintenance/maintenance_schedule.h)
  is the schedule type accepted by the orchestrator.
- [`MaintenanceHealthReport`](../../include/maintenance/maintenance_health_report.h)
  is the production health-report surface.
- [`maintenance_api_contract.h`](../../include/maintenance/maintenance_api_contract.h)
  contains compatibility helper types for focused tests and synthetic
  benchmarks. Its `MaintenanceError`, descriptor/snapshot types, fail-closed
  predicate, limits, and error semantics are not wired into runtime behavior.
- Optional schedule persistence is implemented by
  [`MaintenanceScheduleStore`](../../include/maintenance/maintenance_schedule_store.h).
  Schedule mutations save/remove durable state before publishing their matching
  in-memory change. With no store configured, schedules remain process-local.

## Schedule mutation churn limit

`max_schedule_changes_per_interval == 0` disables the limit. A non-zero value
limits validated create/update/patch/delete attempts for a schedule in a fixed
60-second window, anchored at its first counted attempt. Existing-schedule
attempts consume a slot before persistence, including when a later storage write
fails. Rejections do not increase the count. The window uses a monotonic clock;
the counter is in-memory and is reset by an orchestrator restart.

Create uses the submitted entry's limit and counts the create attempt. Update,
patch, and delete use the existing schedule's limit; changing the limit in an
update/patch takes effect after that mutation. Existing-schedule attempts
consume a slot even if persistence later fails; failed creates discard their
uncommitted counter, and successful deletes clear it. A capped operation returns
the repository error `ERR_RATE_LIMIT_EXCEEDED` with the schedule ID, configured
limit, and interval in its context. This is a mutation rate limit, not a
fail-closed dispatch-health policy.

## Persistence and failure handling

Schedule records are JSON values stored under the `maint_sched::` key prefix.
On load, malformed records are skipped while valid records are retained; a
prefix-scan failure is returned to the caller. A schedule mutation returns a
storage error without changing the corresponding in-memory schedule if its
write/remove fails, though its already-validated churn attempt has been counted.
Task side effects are not rolled back when a later task fails.

## Tests and benchmarks

- Production CRUD, persistence, dispatch, and health coverage is in
  [`tests/database/test_database_maintenance_orchestrator.cpp`](../../tests/database/test_database_maintenance_orchestrator.cpp).
  The new mutation-limit cases exercise actual orchestrator CRUD paths.
- Model-focused tests are in
  [`tests/maintenance/`](../../tests/maintenance/); do not treat their local
  helper models as runtime integration evidence.
- The test build discovers module `CMakeLists.txt` files under
  [`tests/CMakeLists.txt`](../../tests/CMakeLists.txt); verify the current
  root test CMake file before relying on a generated target name.
- Maintenance benchmark targets are registered in
  [`benchmarks/maintenance/CMakeLists.txt`](../../benchmarks/maintenance/CMakeLists.txt).
  They are synthetic/in-memory proxies. Their Google Benchmark aggregates do
  not calculate per-operation p99, and no external percentile processor tied to
  these targets was verified. All release-manifest p99 gates therefore remain
  `UNVERIFIED`; source labels and target registration are not measured evidence.

## Source priority

For code changes, consult the production headers and implementation first, then
the production tests and benchmark sources. This wiki is navigation and summary
context; it does not override the module source or canonical
[documentation governance](../../DOCUMENTATION_GOVERNANCE.md).
