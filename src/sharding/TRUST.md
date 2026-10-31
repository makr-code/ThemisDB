---
Author: "ThemisDB Contributors"
Created: "2026-10-19"
Last Updated: "2026-10-31"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Contributors"
Source_Level: level1
SOT_Domain: security
Canonical_References: `../../include/sharding/sharding_api_contract.h`, `../../SECURITY.md`
Related_Issue: "#7381"
Related_Milestone: DOC-WEEKLY-2026-W44
---

Exact model backend not verifiable from final document text alone.

# Sharding Trust and Security Boundaries

> **Scope limitation:** This is a conceptual trust-boundary map, not a security assessment or
> proof that every listed control is enforced on every path. Statements about quorum, transport,
> WAL, and migration are contract- or component-specific. **TODO:** perform a path-by-path security
> review before using this page as a deployment assurance.

## Trust boundary

The sharding module is a critical trust boundary in ThemisDB. It decides how logical requests are mapped to shard ownership, when a distributed decision is accepted, when writes are durable, and when the system must fail closed instead of proceeding with uncertain state.

## Key trust boundaries

### Routing boundary

`shard_router.cpp` and `consistent_hash.cpp` define the route ownership model. If the ring is invalid or the target shard is unavailable under a configured strategy, the system should surface a precise failure rather than silently redirect the request.

### Transaction boundary

`cross_shard_transaction.cpp` and the two-phase commit contract define the safety envelope for distributed writes. This is where trust is at risk: an unsafe commit path can create hidden divergence across shards. The module therefore insists on explicit terminal states and fail-closed conditions for quorum and in-doubt transactions.

### Durability boundary

`wal_manager.cpp` and the associated WAL semantics are the integrity boundary for recovered state. A corrupt or partial WAL is not recoverable by optimistic fallback. This is a deliberate security and reliability control: silent recovery would mask integrity problems.

### Migration and repair boundary

Replication, migration, and rebalancer code must not silently ignore partial failures. The sharding layer explicitly distinguishes migration conflict, migration fault, and partial repair states so operators can inspect the exact fault and recover safely.

## Security and reliability controls

The module relies on a small set of strong controls:

- deterministic ring-based routing under a valid topology,
- explicit fail-closed behavior on lost quorum and corrupted WAL,
- idempotent commit/abort semantics for distributed transactions,
- strict migration and rollback checks,
- health and failover signals that remain separate from business success signals.

## Risk model

The highest-risk failure conditions in this module are:

- a wrong route because of invalid topology,
- a commit decision made without quorum or coordinator confidence,
- a WAL replay or migration path that proceeds when the state is uncertain,
- silent fallback around health or repair signals.

Those conditions are treated as security and correctness problems, not minor operational nuisance.

## Incident handling

If a sharding incident is suspected:

1. classify the failure as routing, quorum, transaction, WAL, migration, or health-related,
2. validate the topology and route ownership before changing behavior,
3. inspect whether the issue is a partial or in-doubt transaction,
4. confirm whether a migration or rebalancer is mid-flight,
5. apply the smallest safe corrective action and document the exact state transition.

## Summary

The sharding module is trusted to preserve ownership, safety, and recoverability in a distributed environment. That trust is preserved only when the module remains explicit about failure, silent fallback is avoided, and recovery paths are bounded by the contract rather than by convenience.
