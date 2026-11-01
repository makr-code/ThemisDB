---
Author: "ThemisDB Team"
Created: "2026-10-21"
Last Updated: "2026-10-31"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot coding agent"
Human_Review: false
Human_Author: "unknown"
---

Exact model backend not verifiable from final document text alone.

# Security — Vector Search

## Scope and trust boundary

The vector-search runtime is implemented by the shared index subsystem. The
manager receives vectors and query parameters from its caller, performs
dimension and operation checks, and reports failures through its `Status`
contract. Authentication and authorization are not provided by this module;
the integrating service is responsible for access control.

## Source-supported controls

- Index initialization requires a non-empty namespace and positive dimension.
- `searchKnn()` validates query dimension. Batch and MVCC entity-write overloads
  validate vector dimension; direct-commit `addEntity()` currently does not.
- The source does not establish that NaN/Inf input values are rejected.
- Optional index encryption controls and an audit-logger hook are exposed by
  `VectorIndexManager`.
- Save/load and configuration methods expose status failures for their
  documented error cases; optional-backend shutdown failures may be logged
  without being propagated, and unsupported backends can follow fallback paths.

These interfaces do not establish that encryption is enabled in a deployment,
that key management is configured securely, or that audit events are retained.
Those are deployment responsibilities and require separate evidence.

## Risks and required handling

- **Malformed or mismatched vectors:** check operation status and reject
  failed writes or searches. Until the direct-commit dimension gap is resolved,
  validate vector lengths in the caller or use a dimension-checking overload.
- **Optional backend mismatch:** confirm build support and configuration before
  enabling advanced indexing.
- **Persisted index state:** validate save/load and recovery behavior for the
  selected deployment; do not treat an index file as trusted solely because it
  exists.
- **Sensitive data and keys:** do not place vector payloads, key material, or
  credentials in module documentation or logs. Follow the repository security
  policy for access to persisted data and encryption keys.

## Validation boundary

The in-process flat-index stress/soak fixtures do not validate production
encryption, authorization, persistence, or backend security behavior. A security
review and deployment-specific validation are required before making security
assurance claims.

## References

- [`VectorIndexManager`](../../include/index/vector_index.h)
- [Production requirements](PRODUCTION_REQUIREMENTS.md)
- [Runbook](RUNBOOK.md)
