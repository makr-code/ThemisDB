# ThemisDB Temporal Module

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

## Purpose and current posture

The temporal module provides C++ APIs for system-versioned and bitemporal data,
time-range queries, joins/aggregation, snapshots, retention, CDC, conflict
handling, history compression, migration, and tier/cold-store operations. This
feature inventory is not a claim that all surfaces are connected to SQL/AQL or
that every release gate has passed.

The root [`ROADMAP.md`](../../ROADMAP.md) currently classifies temporal as
`PRODUCTION_CANDIDATE` and is authoritative for maturity, blockers, and release
readiness. Do not infer production approval from the presence of code or tests.

## Runtime boundaries

- Public runtime declarations are under [`include/temporal/`](../../include/temporal/).
- Implementations are under `src/temporal/`; see
  [`IMPLEMENTATION.md`](IMPLEMENTATION.md) for the source map.
- Runtime `themisdb::temporal::Timestamp` values are milliseconds since epoch,
  and `TimeRange` is half-open `[start, end)`.
- `include/temporal/temporal_api_contract.h` declares a distinct
  `themis::temporal` contract surface. Its constants and written rules are not
  interchangeable with the runtime types or evidence that those rules are
  implemented by the runtime.
- The temporal API does not provide service authorization, a deployment CLI, or
  a general SQL/AQL parser contract. See [`SECURITY.md`](SECURITY.md) and
  [`PRODUCTION_REQUIREMENTS.md`](PRODUCTION_REQUIREMENTS.md).

## Developer entry points

- [API index](API.md) · [Architecture](ARCHITECTURE.md) · [Data model](MODELING.md)
- [Implementation guide](IMPLEMENTATION.md) · [Module guide](MODULE_GUIDE.md)
- [Operations](OPERATIONS.md) · [Incident runbook](RUNBOOK.md)
- [Performance mapping](PERFORMANCE_EXPECTATIONS.md) · [Security boundary](SECURITY.md)
- [Module roadmap](ROADMAP.md) · [Future enhancements](FUTURE_ENHANCEMENTS.md)
