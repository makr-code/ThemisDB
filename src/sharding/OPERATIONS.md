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
SOT_Domain: module-behavior
Canonical_References:
  - ../../include/sharding/operational_metrics.h
  - ../../include/sharding/health_monitor.h
  - ../../tests/sharding/CMakeLists.txt
Related_Issue: "#7381"
Related_Milestone: DOC-WEEKLY-2026-W44
---

Exact model backend not verifiable from final document text alone.

# Sharding Operations Guide

> The checks below are operator recommendations, not guaranteed built-in commands or automatic
> remediation behavior. Signals and available actions depend on the selected components and
> deployment. **TODO:** attach deployment-specific metrics names, alert thresholds, and recovery
> procedures before using this as an operational runbook.

## Operational mission

The sharding module is responsible for safe routing, durable distributed coordination, and bounded recovery under topology churn. Operationally, it is the layer that decides whether a request is mapped to the right shard and whether the cluster remains correct while rebalancing or healing.

## Primary operational signals

The most useful signals in this module are:

- routing and fallback decision counts,
- shard health transitions,
- quorum and coordinator state changes,
- 2PC terminal state transitions,
- migration progress and rollback outcomes,
- WAL replay and corruption events,
- per-shard latency, backlog, and repair activity.

The implementation surfaces these through `operational_metrics.cpp`, `prometheus_metrics.cpp`, and the health/rebalance state machinery.

## Daily operating checks

### Topology health

- verify that each shard remains mapped to its expected ownership range,
- check that node membership changes are reflected in routing state,
- confirm health transitions are consistent with failover and replacement logic.

### Quorum status

- verify the active coordinator quorum and replica set,
- confirm that lost quorum does not continue to accept writes,
- inspect any repeated failover or recovery loops for a misconfiguration.

### Transaction health

- inspect the number of in-doubt transactions,
- confirm that commit and abort outcomes are terminal and replayable,
- watch for coordinator or participant timeouts as early evidence of instability.

### Migration and repair health

- confirm the rebalancer is not blocking indefinitely,
- verify that migration conflict and rollback states are being resolved on time,
- review progress metrics to ensure the system does not drift into long partial-migration states.

## Recommended diagnostics sequence

When a sharding incident occurs, use this order:

1. check route status and topology validity,
2. inspect quorum and health transitions,
3. inspect transaction states and WAL diagnostics,
4. inspect migration, repair, and rebalance progress,
5. decide whether the issue is a configuration bug, a transient dependency failure, or a correctness violation.

This sequence keeps the investigation anchored in the layer that failed rather than in the downstream symptom.

## Good operating practice

- keep route topology and health decisions visible,
- return explicit error outcomes for failed writes and migrations,
- treat `QUORUM_LOST`, `WAL_CORRUPTION`, and `COORDINATOR_FAILURE` as operator- and safety-critical events,
- do not hide migration faults behind generic retry loops,
- maintain recovery checkpoints and logs sufficient to reconstruct the last safe shard state.

## Change management guidance

Any change involving route selection, consensus logic, migration, or WAL durability should be treated as a release-critical change. These are not isolated implementation details; they affect the correctness and safety of the distributed system.

## Summary

The sharding module is operationally healthy when topology, quorum, transaction, and migration state all remain explicit, bounded, and observable. In practice, the best operations discipline is to treat these as safety signals and act quickly when they drift away from the contract.
