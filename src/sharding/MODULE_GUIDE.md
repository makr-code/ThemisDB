---
Author: "ThemisDB Contributors"
Created: "2026-10-19"
Last Updated: "2026-10-19"
Status: "active"
AI_Used: true
AI_Models: "MAI-Code-1.1-Flash"
AI_Contributor: "MAI-Code-1.1-Flash"
Human_Review: true
Human_Author: "ThemisDB Contributors"
---

Exact model backend not verifiable from final document text alone.

# Sharding Module Guide

## Who this module is for

This module is for engineers working on:

- shard placement and route selection,
- cross-shard transaction safety,
- topology change and migration handling,
- health/failover logic,
- operational diagnostics and recovery under degraded topology.

## Entry points

Start here when working in the module:

- `src/sharding/README.md` — module purpose and high-level scope
- `src/sharding/ARCHITECTURE.md` — execution planes and contracts
- `include/sharding/sharding_api_contract.h` — normative contract
- `src/sharding/ROADMAP.md` — current status and phase plan
- `src/sharding/FUTURE_ENHANCEMENTS.md` — future-risk items and required semantics

## Most relevant source files

### Routing and topology

- `src/sharding/consistent_hash.cpp`
- `src/sharding/shard_router.cpp`
- `src/sharding/adaptive_shard_router.cpp`
- `src/sharding/locality_aware_router.cpp`

### Coordination and transactions

- `src/sharding/distributed_coordinator.cpp`
- `src/sharding/cross_shard_transaction.cpp`
- `src/sharding/two_phase_commit_coordinator.cpp`
- `src/sharding/distributed_transaction.cpp`

### Durability and recovery

- `src/sharding/wal_manager.cpp`
- `src/sharding/wal_applier.cpp`
- `src/sharding/data_migrator.cpp`
- `src/sharding/shard_repair_engine.cpp`
- `src/sharding/auto_rebalancer.cpp`

### Health and operations

- `src/sharding/health_monitor.cpp`
- `src/sharding/quorum_manager.cpp`
- `src/sharding/operational_metrics.cpp`
- `src/sharding/prometheus_metrics.cpp`

## Working rules for changes

1. Preserve the contractual error taxonomy in `sharding_api_contract.h`.
2. Treat lost quorum and WAL corruption as fail-closed conditions.
3. Do not silently route around invalid topology.
4. Do not treat migration or rebalance as best-effort background work.
5. Keep health and metrics explicit; do not hide degraded operation behind generic success responses.

## Debugging approach

When investigating a sharding issue:

1. confirm the route decision and topology view,
2. inspect transaction or coordinator state,
3. validate WAL ordering and replay state,
4. check the reason for health/failover transitions,
5. confirm whether migration or repair is in progress before assuming a backend issue.

## Production-readiness checklist

Before shipping a change in this module, verify:

- route semantics stay deterministic,
- transaction states remain explicit,
- quorum checks fail closed,
- WAL replay remains ordered and verifiable,
- migration can rollback or fail explicitly,
- operational metrics remain useful for triage.

## Summary

This module is a distributed safety layer in ThemisDB. The right mental model is not simply “data partitioning,” but “explicit topology-aware correctness under distributed failure.”
