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

# Cache Trust Boundaries

Exact model backend not verifiable from final document text alone.

## Purpose and trust model

This document identifies what the cache module trusts, what it does not establish, and where operators must enforce controls. It supplements [`SECURITY.md`](./SECURITY.md); security findings must use the project-level security reporting process.

Cache entries are derived data, not an authorization source or canonical record. Cached query results, prompts, parameters, embeddings, metadata, keys, warmup records, and replication messages may contain sensitive or attacker-influenced material. A cache hit must not bypass caller authorization, row/tenant access checks, or source-data freshness rules.

## Boundaries

| Boundary | Trust decision and control |
|---|---|
| Caller → cache API | The cache accepts caller-supplied fingerprints, tenant identifiers, JSON values, vectors, TTLs, and invalidation selectors. Validate at the call boundary; the contract defines size/failure classes but does not authenticate a caller. |
| Tenant → cache namespace | Empty tenant denotes the global namespace. Adaptive tenant isolation is opt-in in `Config`; when enabled, use the same validated tenant context on fingerprint generation, get, put, and invalidation. Do not mix tenant and global keys for sensitive results. |
| Cache → underlying data | Cached results can become stale. The upstream writer/service owns authorization and invalidation ordering; TTL is not proof of freshness. |
| Process → RocksDB | `SemanticCache` borrows caller-owned database/column-family pointers. The caller owns lifetime and database access controls. Embedding/adaptive storage paths depend on configured database locations and filesystem permissions. |
| Node → Redis/remote peer | Coordinator messages can carry keys, tenant identifiers, results, and invalidation selectors. Protect network access and credentials at deployment boundaries. The coordinator config exposes password/authentication and optional HMAC secret fields, but HMAC provides message integrity, not confidentiality; do not infer transport encryption from those fields. |
| Operator/admin surface | Cache inspection, flushing, key listing, and invalidation can expose or destroy data. Authentication and authorization must be enforced by the integrating admin/API layer and audited in production. |

## Security controls and gaps to verify

- Enforce tenant identity outside the cache and enable isolation where required. A cache configured with isolation off shares the empty/global namespace.
- Apply explicit key/value/tenant limits and resource quotas; map errors using `CacheFailureClass` and preserve fail-closed behavior for `DegradedBackend` and `InternalError`.
- Keep cache directories, RocksDB files, snapshots, warmup logs, backups, Redis credentials, and HMAC secrets under least-privilege access and approved retention controls.
- Restrict Redis/peer reachability at the network layer. Where confidentiality is required, use a deployment-approved encrypted transport; the Redis coordinator's shown configuration does not itself specify TLS.
- Avoid logging raw prompts, result payloads, vectors, credentials, or full cache values during diagnosis. Use sanitized keys/correlation identifiers and aggregate metrics.
- Treat partial replication, queue drops, failed invalidation, and unknown cache state as incidents requiring explicit review rather than silently accepting stale data.

These are deployment controls to verify, not a claim that every cache class implements authentication, encryption, or audit logging internally. Refer to [`PRODUCTION_REQUIREMENTS.md`](./PRODUCTION_REQUIREMENTS.md) for the module production checklist.

## Incident handling

For possible cross-tenant access, stale sensitive results, exposed credentials, or failed erasure, preserve sanitized diagnostics, stop using the affected cache path if needed, and escalate through the project security process. Invalidate the narrowest known scope only after coordinating with the data owner; check distributed delivery separately and do not claim backups were purged from a cache invalidation result.

See [RUNBOOK.md](./RUNBOOK.md) for triage and [OPERATIONS.md](./OPERATIONS.md) for routine monitoring.
