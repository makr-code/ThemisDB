---
Author: "ThemisDB Contributors"
Created: "2026-10-19"
Last Updated: "2026-11-01"
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

> **Scope limitation:** This is a conceptual trust-boundary map, not a security assessment or proof
> that any listed control is enforced on every path. Only repository source and contract surfaces
> were inspected; no deployed component, configuration, or runtime telemetry was available.
> **TODO:** validate each boundary against a named deployment and its enabled runtime components
> before using this page as deployment assurance.

## Trust boundary

The sharding module is intended to be a trust boundary in ThemisDB. Its components map logical requests to shard ownership, coordinate distributed decisions, and interact with persistence and failure handling. This document does not establish that deployed components enforce those boundaries.

## Key trust boundaries

### Routing boundary

`shard_router.cpp` and `consistent_hash.cpp` contain route-ownership logic. Expected behavior for invalid rings and unavailable shards is component- and configuration-specific and was not validated against a running deployment.

### Transaction boundary

`cross_shard_transaction.cpp` and the public two-phase commit contract describe distributed-write state handling. This review did not establish that deployed transaction paths enforce the contract under quorum loss, coordinator failure, or in-doubt recovery.

### Durability boundary

`wal_manager.cpp` and its documented WAL semantics are relevant to recovered-state integrity. Corruption handling, durability, and recovery behavior must be checked against the selected storage backend and deployment; no runtime recovery was exercised.

### Migration and repair boundary

Replication, migration, and rebalancer implementations are relevant to partial-failure handling. The error taxonomy includes migration states, but this inspection did not validate every implementation path or confirm that deployed operators can observe and recover each failure safely.

## Security and reliability controls

The repository describes these intended/component-level controls; deployment enforcement is unverified:

- ring-based routing under a valid topology,
- component/contract-level fail-closed paths for quorum and WAL failures,
- commit/abort state handling for distributed transactions,
- migration and rollback checks,
- health and failover signals distinct from business outcomes.

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

The repository contains sharding ownership, coordination, WAL, migration, and recovery surfaces.
Their presence does not establish deployed safety or recoverability. Such assurance requires
deployment-specific configuration review and execution evidence for the actual runtime components.
