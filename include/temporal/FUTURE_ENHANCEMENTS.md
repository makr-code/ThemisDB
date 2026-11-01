# Temporal public-header enhancement proposals

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

> These are unapproved discussion topics, not scheduled work or promises about
> API stability. The canonical module roadmap is
> [`../../src/temporal/ROADMAP.md`](../../src/temporal/ROADMAP.md).

## Candidate topics

- Clarify how the separate `themis::temporal` contract header relates—or does
  not relate—to the `themisdb::temporal` runtime before proposing a unified
  contract.
- Improve header comments for boundary behavior, errors, persistence, object
  lifetime, and concurrency where an implementation and tests support a
  stronger statement.
- Add runtime-backed benchmark cases or acceptance configuration if the project
  adopts temporal-specific performance gates.

No evidence in the inspected headers establishes a `temporal_audit_log.h`, SQL
conformance guarantees, a stable ABI promise, an explain-plan API, or a dated
delivery commitment. **TODO:** do not convert such proposals into commitments
without an approved issue and source-backed contract.
