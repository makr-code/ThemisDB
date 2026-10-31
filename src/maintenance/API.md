---
Author: "ThemisDB Team"
Created: "2026-10-30"
Last Updated: "2026-10-30"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
---

Exact model backend not verifiable from final document text alone.

# Maintenance API

## Purpose and contract owners

This page is for API integrators and maintainers. The typed C++ contract is
`include/maintenance/database_maintenance_orchestrator.h` and related schedule,
task, and health headers. HTTP routes are registered and authorized in
`src/server/http_server.cpp`; `MaintenanceApiHandler` translates JSON requests
to orchestrator calls. HTTP authorization is not implemented by the handler
class itself.

## HTTP routes

| Method and path | Access scope | Behavior |
|---|---|---|
| `POST /api/v1/maintenance/schedules` | `maintenance:write` | Create from a JSON `MaintenanceScheduleEntry`; successful response is `201 Created`. |
| `GET /api/v1/maintenance/schedules` | `maintenance:read` | List schedules. The current HTTP route does not pass a tenant filter to the handler. |
| `GET /api/v1/maintenance/schedules/{id}` | `maintenance:read` | Return one schedule. |
| `PUT /api/v1/maintenance/schedules/{id}` | `maintenance:write` | Replace mutable schedule fields. |
| `PATCH /api/v1/maintenance/schedules/{id}` | `maintenance:write` | Apply supplied fields; unspecified schedule fields are retained. |
| `DELETE /api/v1/maintenance/schedules/{id}` | `maintenance:write` | Remove the schedule. A job already started by it may finish. |
| `POST /api/v1/maintenance/schedules/{id}/run` | `maintenance:write`; additionally `maintenance:admin` when forced | Start an asynchronous run. Optional JSON body field `force` must be boolean. A successful trigger responds `202 Accepted`. |
| `GET /api/v1/maintenance/jobs` | `maintenance:read` | List retained jobs. |
| `GET /api/v1/maintenance/jobs/{id}` | `maintenance:read` | Return one job. |
| `POST /api/v1/maintenance/jobs/{id}/cancel` | `maintenance:write` | Request cancellation of a running job; completed task effects are not rolled back. |
| `GET /api/v1/maintenance/status` | `maintenance:read` | Return orchestrator status. |
| `GET /api/v1/maintenance/health` | `maintenance:read` | Return the aggregate health report and recent dispatch outcomes. |
| `GET /api/v1/maintenance/task-handlers` | `maintenance:read` | List registered task types and handler names. |

These scopes are enforced by the HTTP server route layer. A direct in-process
caller of `MaintenanceApiHandler` does not pass through those route checks and
must apply its own access policy.

## Schedule payload and validation

The schedule payload is the JSON representation of
[`MaintenanceScheduleEntry`](../../include/maintenance/maintenance_schedule.h).
It includes a name, frequency/cron, task list, optional task dependencies,
window settings, enablement, tenant ID, and failure/lock policy. The
orchestrator assigns the schedule ID and timestamps on create.

Create and update require a non-empty name, at least one task, valid window
hours (`0` through `23`), and a cron expression for custom frequency. Declared
dependency tasks must be present in the task list and the graph must be acyclic.
Invalid JSON, invalid IDs, invalid schedule data, and missing schedule/job IDs
produce error responses; storage errors are propagated rather than represented
as successful mutations.

`PUT` is full replacement of mutable fields. `PATCH` applies only fields
present in the JSON object. The HTTP run route accepts `force` in the request
body, not as a query parameter; non-boolean values are rejected. Forced runs
bypass the UTC window only and require both write and admin authorization.

## C++ integration interfaces

- `DatabaseMaintenanceOrchestrator`: lifecycle, CRUD, job control, health,
  tenant policy, lock injection, and handler registration.
- `MaintenanceScheduleEntry`: schedule data, JSON conversion, and partial patch.
- `IMaintenanceTaskHandler`: task-specific execution returning
  `Result<std::string>`; implementations are expected to be thread-safe.
- `IDistributedLock`: optional TTL lock strategy. A null lock disables
  cross-node coordination.
- `MaintenanceHealthReport`: aggregate signals and bounded dispatch history.

Use the concrete declarations and Doxygen comments in
[`include/maintenance/`](../../include/maintenance/) as the authoritative
contract. The HTTP server may map internal errors to HTTP status codes; callers
should not infer persistence or task transaction semantics from an HTTP status
alone.

## Sources and tests

- [`database_maintenance_orchestrator.h`](../../include/maintenance/database_maintenance_orchestrator.h)
- [`maintenance_api_handler.h`](../../include/server/maintenance_api_handler.h)
- [`maintenance_api_handler.cpp`](../../src/server/maintenance_api_handler.cpp)
- [`http_server.cpp`](../../src/server/http_server.cpp)
- [`test_database_maintenance_orchestrator.cpp`](../../tests/test_database_maintenance_orchestrator.cpp)
- [Operations](OPERATIONS.md) · [Runbook](RUNBOOK.md)
