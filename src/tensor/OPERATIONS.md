---
Author: "ThemisDB Contributors"
Created: "2026-10-21"
Last Updated: "2026-10-21"
Status: "active"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "unknown"
Human_Review: true
Human_Author: "ThemisDB Contributors"
---

Exact model backend not verifiable from final document text alone.

# Tensor Module Operations Guide

## Operational responsibilities

The tensor module is expected to operate as a bounded internal service for compact tensor storage, retrieval, and similarity queries. Operationally, the key obligations are:

- maintain route correctness for TT vs hybrid vs HNSW paths
- keep index lifecycle and cleanup deterministic
- preserve fingerprint query correctness under high turnover
- surface diagnostics for invalid or malformed tensor data

## Metrics and signals

The implementation exposes operational signals from the code itself:

- `TensorIndexManager::pending_operations_` tracks in-flight create operations
- `TensorFingerprintGraph::stats()` tracks total adapters, query calls, and comparison counts
- `legacy_bridge_cache_` gives a direct signal of bridge state lifecycle and cleanup pressure
- `dropIndex()` emits warnings when file removal fails

This means operators can quickly determine whether the module is operating under a transient spike, a stale persisted index, or a malformed data issue.

## Operational checks

### Daily checks

- confirm route results match expected tenant / field workloads
- confirm no abnormal accumulation of duplicate or stale adapter entries
- verify graph stats remain consistent across query and write operations
- inspect warning logs for persistent file removal or restore failures

### During incident investigation

- compare the adapter key against the expected tenant + domain + model tuple
- inspect the candidate graph state before and after a failed query burst
- confirm `findSimilar()` and `findSimilarByFingerprint()` do not diverge on valid workloads
- verify `createIndex()` and `dropIndex()` are completing within expected bounds

## SLO-style expectations

The module’s design assumptions are bounded-latency and bounded-logic operations, not best-effort heuristics. The projected runtime envelope is consistent with the module’s use of compact graphs and cached norms, but it still depends on input size, data profile, and route selection.

Operators should therefore treat route mismatch, repeated invalid self-product warnings, and cache churn as operational signals rather than harmless noise.

## Recovery checklist

1. identify the data profile and route decision for the affected workload
2. validate the installed adapter and graph entries
3. remove stale or malformed state
4. rebuild the index or graph from a trusted source
5. confirm that diagnostics are no longer repeating after the fix

## Escalation

Escalate when:

- the same malformed tensor state is repeated across multiple tenants or collections
- a route decision remains wrong after reconfiguration
- persisted state and in-memory graph diverge across restart or recovery
- adapter or fingerprint invariants cannot be restored without broad data rebuilds
