---
Author: "ThemisDB Team"
Created: "2026-10-30"
Last Updated: "2026-10-30"
Status: active
AI_Used: true
AI_Models: unknown
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
Source_Level: level1
SOT_Domain: security
Canonical_Sources:
  - "src/index/SECURITY.md"
  - "include/index/index_manager.h"
  - "src/index/index_manager.cpp"
  - "include/index/vector_index.h"
  - "src/index/vector_index.cpp"
  - "include/index/gpu_vector_index.h"
  - "src/index/gpu_vector_index.cpp"
  - "include/index/ann_frontdoor.h"
  - "include/index/index_manifest_v1.h"
  - "src/index/ROADMAP.md"
Related: "Issue #7125; DOC-WEEKLY-2026-44"
---

Exact model backend not verifiable from final document text alone.

# Index trust boundaries

## Purpose and reader

This document describes the index module's trust boundaries and review
obligations for security engineers, service integrators, and operators. It
distinguishes mechanisms implemented in index code from controls that must be
provided by callers, storage, deployment, or hardware infrastructure.

## Trust boundaries

### Callers, vectors, and filters

Treat query vectors, entity vectors, field names, primary keys, filters, and
scope identifiers as caller-controlled unless an upstream contract validates
them. The vector API configures a dimension and reports operation status;
validate dimensions, field shape, and resource limits at the integration
boundary. ANN frontdoor checks null query pointers and zero dimensions, but it
does not constitute comprehensive input validation.

Candidate filters and evaluator callbacks constrain search results; they are
not authorization checks. Enforce access policy before retrieval and ensure
the caller cannot select another tenant/scope merely by changing a filter or
scope string.

### Tenant namespaces and authorization

Tenant-aware `IndexManager` overloads prefix logical names as
`tenant:<tenant_id>:<index_name>` and reject empty, overlong, colon-containing,
or NUL-containing components. This is a namespace boundary only. It does not
authenticate the tenant, authorize operations, or prove that every direct
manager/storage path is tenant-filtered. The service layer must authorize
tenant identity and pass the authorized tenant context into each operation.

### Storage, persistence, and recovery

`VectorIndexManager` persists vector entities through the RocksDB wrapper.
Protect database files, backups, snapshots, and exported vectors according to
their data classification. The optional audit logger records operation
metadata when configured. Its implementation catches `std::exception` around
optional audit-event construction/logging, warns, and continues the vector
operation; other thrown types are not covered by this catch. Optional audit is
therefore not fail-closed and must not be relied on as a required security
control.

`IndexManifestV1` is caller-synchronized metadata serialization, not a durable
storage transaction or authorization mechanism. Its rollback fields are claims
about availability supplied in metadata; they do not prove the prior vector
data is present or restore it.

### Accelerators and fallback

GPU/backend selection depends on compiled support and runtime availability.
`GPUVectorIndex::Config::allowCPUFallback` explicitly permits CPU failover or
requires failure when the requested backend cannot initialize. Choose this
policy according to the system's availability and data-handling requirements;
record any fallback because it changes execution characteristics. GPU
hardware-only parity is still pending in the module roadmap.

### Logs and diagnostics

Use correlation IDs and structured route/fallback metadata for diagnosis.
Avoid logging raw vectors, query text, entity payloads, or secrets in callers
and backend adapters. Review any added diagnostic field for sensitive data
exposure. Error text and backend diagnostics should be handled as potentially
sensitive operational data.

## Failure and abuse cases

| Risk | Required control boundary |
|---|---|
| Cross-tenant index lookup or destructive operation | Authenticate/authorize in the service boundary; use tenant overloads consistently; test direct and indirect access paths. |
| Dimension or malformed-vector input causing resource/error behavior | Validate inputs and enforce deployment resource limits; inspect returned status and fail the ingestion operation on error. |
| Unauthorized filtering or scope selection | Treat filters and `scope_id` as routing/query hints, not access-control predicates. |
| Backend unavailable with unexpected CPU execution | Set and test `allowCPUFallback` explicitly; monitor active backend/fallback state. |
| Rebuild, rollback, or manifest mismatch causing stale/mixed generations | Preserve authoritative data; verify schema/version and actual restore implementation before switching traffic. |
| Diagnostics exposing sensitive data | Keep payloads out of logs; audit new telemetry fields and retention/access policy. |

## Validation and incident response

Review tenant isolation, rejected tenant components, backend fallback, mutation
failure, and audit logger behavior in focused tests and integration tests.
When an incident suggests cross-context exposure or persistent index
corruption, stop risky mutations, preserve relevant storage state and logs,
restrict affected routes through the owning service control plane, and
escalate to security, storage, and index owners. Follow
[RUNBOOK.md](RUNBOOK.md) for non-destructive triage.

## Canonical sources

- [Module security summary](SECURITY.md)
- [Tenant and manager declarations](../../include/index/index_manager.h)
- [Tenant validation and manager implementation](index_manager.cpp)
- [Vector lifecycle and optional audit integration](../../include/index/vector_index.h)
- [GPU fallback policy](../../include/index/gpu_vector_index.h)
- [ANN query/result boundary](../../include/index/ann_frontdoor.h)
- [Current hardware parity status](ROADMAP.md)
