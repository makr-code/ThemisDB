# Security - Temporal Module

<!-- Report vulnerabilities through the project-level `SECURITY.md`. -->

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

## Security boundary

The temporal interfaces accept caller-provided keys, timestamps, ranges, table
names, JSON payloads, and policy values. The inspected APIs do not authenticate
callers or authorize access to tables; enforce those controls in the consuming
service. Do not treat temporal versioning, snapshots, or a change log as an
authorization or compliance boundary.

## Data exposure and persistence

- Snapshots, the default CDC ring, conflict history/queues, and the retention
  archive are process-memory state, not durable backups.
- CDC callbacks receive event payloads synchronously. Restrict subscriptions and
  avoid publishing unnecessary sensitive data.
- `CDCPersistentLog` writes serialized event data to filesystem segments. Its
  CRC-32 record check is not encryption or writer authentication. Protect the
  WAL directory, backups, and deletion lifecycle at the deployment boundary.
- Custom merge resolvers and subscriber callbacks execute application code;
  validate inputs and review their exception, concurrency, and privilege
  behavior.
- Retention can physically remove historical versions. Set and review policies
  against the application's legal, audit, and recovery requirements.

## Validation boundary

The module tests exercise temporal behavior but do not by themselves establish a
security review, deployment authorization, audit logging, or production
acceptance. The project security process is defined by the root
[`SECURITY.md`](../../SECURITY.md); module maturity is tracked in the root
[`ROADMAP.md`](../../ROADMAP.md).
