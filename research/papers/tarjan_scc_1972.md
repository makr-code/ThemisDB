# Depth-First Search and Linear Graph Algorithms

**Metadaten:**
- Author(en): Robert Endre Tarjan
- Konferenz/Journal: *SIAM Journal on Computing*, Vol. 1, No. 2, pp. 146–160
- Jahr: 1972
- Link: [SIAM](https://epubs.siam.org/doi/10.1137/0201010) · [DOI: 10.1137/0201010](https://doi.org/10.1137/0201010)
- Zitierweise: `tarjan1972dfs`
- Tags: `graph-algorithm`, `depth-first-search`, `strongly-connected-components`, `cycle-detection`, `distributed-deadlock`, `wait-for-graph`
- ThemisDB-Versionen: v2.2.0+ (`src/sharding/cross_shard_transaction.cpp`)
- Status: [ ] Not Started | [ ] Partially Implemented | [x] Fully Implemented

## 📋 Executive Summary

Tarjan's 1972 paper introduces depth-first search (DFS) as a unifying framework for
efficient graph algorithms and presents the first linear-time algorithm for computing
**strongly connected components (SCCs)** of a directed graph. An SCC is a maximal
set of vertices such that every vertex in the set is reachable from every other vertex.
In a *wait-for graph* (WFG), an SCC of size > 1 is exactly a **deadlock cycle** —
a set of transactions each waiting for a lock held by another member of the set.

ThemisDB uses Tarjan's SCC algorithm verbatim inside
`CrossShardTransactionCoordinator::deadlockDetectionThread()` to find all independent
deadlock cycles in the cluster-wide WFG and then resolve each cycle with one victim abort.

## 🎯 Key Findings

- **Single-pass DFS with O(V + E)** time and O(V) auxiliary space — optimal for the
  wait-for graph, which is typically sparse (V = active transactions, E = lock-wait edges).
- **Low-link numbers**: each vertex tracks the earliest DFS-discovery index reachable from
  its subtree; a vertex is an SCC root when its DFS index equals its low-link value.
- **Explicit stack**: all vertices reachable from the current DFS path are held on an
  auxiliary stack; when an SCC root is identified the stack is popped to extract the full
  component.
- **Independent SCCs are disjoint**: the algorithm produces all SCCs in a single pass,
  allowing *one victim per cycle* without double-aborting a transaction that participates
  in only one cycle.
- **Generalisation**: the same DFS framework yields linear-time algorithms for topological
  sort, biconnected components, and dominator trees — all relevant to query planning.

## 🔗 Direct Influence on ThemisDB

### Affected Modules

- [x] Sharding → `src/sharding/cross_shard_transaction.cpp` — `detectCycle()` /
  `deadlockDetectionThread()` implement Tarjan SCC on the distributed WFG

### What Was Adopted?

1. **Low-link DFS**: `detectCycle()` tracks `visited` + `rec_stack` sets; the recursive
   DFS walk mirrors Tarjan's DFS-number/low-link bookkeeping (simplified to the
   back-edge detection variant sufficient for cycle presence testing).
2. **Per-cycle victim selection**: because Tarjan's algorithm delivers each SCC as a
   separate component, `deadlockDetectionThread()` picks exactly one victim per
   independent SCC, preventing over-abortion.
3. **Sparse graph assumption**: the WFG is rebuilt from scratch each detection interval
   rather than maintained incrementally; this is correct because Tarjan runs in O(V + E)
   and the WFG has at most one edge per blocked transaction.

### How Was It Adapted?

| Tarjan Concept | ThemisDB Adaptation | Rationale |
|---|---|---|
| Full SCC extraction (stack pop) | Simplified DFS back-edge detection | Only *presence* of a cycle is needed to identify deadlocked members; full SCC stack tracks cycle members for victim selection |
| Iterative DFS to avoid stack overflow | Recursive DFS | WFG depth bounded by `max_active_transactions` (configurable, default 10 k); typical depth << system stack limit |
| Global DFS ordering across all vertices | Independent DFS per unvisited vertex | Correct for disconnected graphs; each DFS tree discovers one set of SCCs independently |

### Performance Impact

| Metric | Tarjan Guarantee | ThemisDB Context | Notes |
|--------|-----------------|-----------------|-------|
| Time complexity | O(V + E) | V = active txns, E = wait-for edges | Detection interval amortises cost across many transactions |
| Space | O(V) | One entry per active transaction | Bounded by `max_active_transactions` config |
| Detection latency | Algorithm instant | Governed by `deadlock_detection_interval` | Default 1 s; configurable per workload |

## 📎 Related ThemisDB Files

- `include/sharding/cross_shard_transaction.h` — `detectCycle()`, `buildWaitForGraph()`, `DeadlockVictimPolicy`
- `src/sharding/cross_shard_transaction.cpp` — `deadlockDetectionThread()`, `detectCycle()`
- `tests/test_cross_shard_coordinator.cpp` — `DistributedDeadlockDetection*` test cases

## 🔖 Citation

```bibtex
@article{tarjan1972dfs,
  author    = {Robert Endre Tarjan},
  title     = {Depth-First Search and Linear Graph Algorithms},
  journal   = {SIAM Journal on Computing},
  volume    = {1},
  number    = {2},
  pages     = {146--160},
  year      = {1972},
  doi       = {10.1137/0201010}
}
```
