# Temporal production integration requirements

Author: ThemisDB Contributors
Created: 2026-10-31
Last Updated: 2026-10-31
Status: review
AI_Used: true
AI_Models: unknown
AI_Contributor: GitHub Copilot
Human_Review: false
Human_Author: ThemisDB Contributors

Exact model backend not verifiable from final document text alone.

## Scope

This page records integration constraints visible in the temporal C++ APIs. It
does not define a repository-wide production policy, deployment configuration,
or release approval. The root [`ROADMAP.md`](../../ROADMAP.md) is authoritative
for module maturity and release readiness.

## Source-backed operating constraints

- **Retention:** choose a policy that matches the required history horizon.
  Enforcement may remove historical versions; changing a policy does not restore
  data already purged. The in-memory archive is not durable backup storage.
- **Scheduled tables:** `RetentionManager::scheduleTable` retains a reference to
  a `SystemVersionedTable`. Keep that object alive until it is unscheduled or
  the scheduler is stopped.
- **Snapshots:** `TemporalSnapshotManager` captures table scans in process
  memory. Release handles or use its garbage-collection methods to bound
  retention; snapshots are not process-restart recovery points.
- **CDC:** the default in-memory log is bounded and may overwrite old events.
  Check overflow before treating replay as complete. `CDCPersistentLog` is a
  separate filesystem WAL API; callers own its lifecycle and filesystem
  protection.
- **Payloads and callbacks:** temporal records are JSON payloads; CDC events can
  include before/after values. Restrict subscriber access and review
  application-provided callbacks at the service boundary.

## Configuration and assurance boundary

The inspected temporal headers do not define a module configuration namespace,
`THEMIS_PRODUCTION_MODE`/`THEMIS_ENVIRONMENT` switch, authentication or
authorization control, audit-log policy, or deployment metric exporter.
**TODO:** document these only when the owning service or canonical operations
source provides the actual configuration contract.

Build and test registration are described in the repository CMake files; this
page does not claim that a build, test suite, benchmark, or production
acceptance gate was run.
