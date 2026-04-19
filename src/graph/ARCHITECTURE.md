> **Architektur-Hinweis:** Klassen/Typen/Namespaces mit aktuellem Sourcecode abgleichen. Symbole, die nicht im Source gefunden werden, mit `<!-- TODO: verify symbol -->` markieren.

# Graph Module — Architecture Guide

**Version:** 1.0
**Last Updated:** 2026-04-06
**Module Path:** `src/graph/`

---

## 1. Overview

The Graph module provides ThemisDB's advanced property graph query capabilities. It layers
a cost-based query optimizer and constrained path-finding engine on top of the
`GraphIndexManager` (from `src/index/`), enabling efficient graph traversals with complex
path constraints, adaptive algorithm selection, and parallel execution.

---

## 2. Design Principles

- **Cost-Based Optimization** – the optimizer selects traversal algorithms (BFS, DFS,
  Dijkstra, A*, Bidirectional) based on estimated query cost from graph statistics.
- **Constraint-First Planning** – path constraints (min/max length, required/forbidden
  nodes, edge predicates) are pushed into the traversal rather than filtered post-hoc.
- **Adaptive Learning** – `query_plan_cache.cpp` tracks actual execution statistics and
  feeds them back into the optimizer's cost model for future queries.
- **Parallelism** – `parallel_traversal.cpp` parallelizes independent frontier expansions
  for BFS and bidirectional search on large graphs.
- **Separation from Storage** – the graph module does not manage graph storage or indexes;
  it delegates to `GraphIndexManager` from the index module.

---

## 3. Component Architecture

### 3.1 Key Components

| File | Role |
|---|---|
| `graph_query_optimizer.cpp` | Cost-based algorithm selection and plan generation |
| `path_constraints.cpp` | Path constraint evaluation: length, required/forbidden nodes, edge predicates |
| `parallel_traversal.cpp` | Parallel BFS / bidirectional search using thread pool |

### 3.2 Component Diagram

```
┌─────────────────────────────────────────────────────────────────┐
│                AQL Graph Query (src/query/)                      │
│   FOR v, e, p IN 1..5 OUTBOUND @start GRAPH 'social'           │
│   FILTER p.edges[0].type == 'FOLLOWS'                            │
└──────────────────────────┬──────────────────────────────────────┘
                           │
┌──────────────────────────▼──────────────────────────────────────┐
│                  GraphQueryOptimizer                             │
│                                                                  │
│  collectStatistics() → {vertex_count, edge_count, avg_degree}   │
│  selectAlgorithm(pattern, constraints, stats) → algorithm        │
│  generatePlan() → GraphQueryPlan                                 │
│  optimizeShortestPath(start, end) → plan                        │
└────────┬──────────────────────────────────────────┬─────────────┘
         │ selected algorithm                        │ plan cache
┌────────▼────────────────────┐        ┌────────────▼──────────────┐
│  PathConstraints            │        │   QueryPlanCache           │
│  validate(path, constraints)│        │   cache_hit? → reuse plan  │
│  during traversal           │        │   update stats after exec  │
└─────────────────────────────┘        └───────────────────────────┘
         │
┌────────▼────────────────────┐
│  Traversal Execution        │
│  BFS | DFS | Dijkstra | A*  │
│  | Bidirectional            │
│  (sequential or parallel)   │
└────────┬────────────────────┘
         │
┌────────▼────────────────────┐
│  GraphIndexManager          │
│  (src/index/)               │
└─────────────────────────────┘
```

---

## 4. Data Flow

### 4.1 Shortest Path Query

```
optimizer.optimizeShortestPath("user_A", "user_B")
    │
    ├─ collectStatistics() → {vertices: 1M, edges: 10M, avg_degree: 10}
    │
    ├─ estimatedDepth <= 5 AND !has_weights → select BFS
    │   OR !has_weights AND large_graph → select Bidirectional BFS
    │   OR has_weights → select Dijkstra
    │   OR has_heuristic → select A*
    │
    ├─ generate GraphQueryPlan {algorithm: BFS, estimated_cost: 1200ms}
    │
    ├─ parallel_traversal (if graph > threshold): split frontier
    │
    └─ return path + PathConstraints validation at each step
```

### 4.2 Pattern Match Query

```
optimizer.optimizePatternMatch(pattern)
    │
    ├─ classify as PATTERN_MATCH
    ├─ select DFS (depth-first for backtracking)
    │
    └─ DFS with PathConstraints:
           required_nodes? → check at each expansion
           forbidden_nodes? → prune branch immediately
           edge_predicate? → filter during edge iteration
```

---

## 5. Integration Points

| Direction | Module | Interface |
|---|---|---|
| **Delegates to** | `src/index/` | `GraphIndexManager` for actual graph storage/traversal |
| **Called by** | `src/query/` | Graph query plan execution |
| **Uses** | `src/analytics/` | Graph analytics (PageRank, community detection) |
| **Provides to** | `src/query/` | Optimized execution plans and results |

---

## 6. Threading & Concurrency Model

- `GraphQueryOptimizer` is stateless per query; safe for concurrent invocations.
- `parallel_traversal.cpp` uses a shared thread pool (configurable size); frontier
  partitions run in parallel with a join at each BFS level.
- Query plan cache uses a read-write lock (many concurrent readers, one writer).
- Graph statistics collection holds a read lock on `GraphIndexManager`.

---

## 7. Performance Architecture

| Technique | Detail |
|---|---|
| Algorithm selection | BFS for shallow unweighted; A* for heuristic-guided; Bidirectional for long paths |
| Parallel BFS | Frontier partitioned across threads at each hop level |
| Plan caching | Repeated queries reuse compiled plans |
| Constraint pruning | Path constraints prune dead branches early (not post-filter) |

### Algorithm Complexity

| Algorithm | Time | Best Use Case |
|---|---|---|
| BFS | O(V + E) | Shallow unweighted shortest path, k-hop |
| DFS | O(V + E) | Pattern matching (backtracking) |
| Dijkstra | O((V+E) log V) | Weighted shortest path |
| A* | O((V+E) log V) | Heuristic-guided (domain-specific) |
| Bidirectional BFS | O(b^(d/2)) | Long-distance paths in large graphs |

---

## 8. Security Considerations

- Traversal depth and result count are bounded (configurable) to prevent DoS via
  unbounded graph traversals.
- User-supplied node/edge IDs are validated against the schema before traversal.

---

## 9. Configuration

| Parameter | Default | Description |
|---|---|---|
| `graph.max_traversal_depth` | 100 | Max path length |
| `graph.max_result_paths` | 1000 | Max paths returned |
| `graph.parallel_threshold_vertices` | 100000 | Min graph size for parallel traversal |
| `graph.plan_cache_size` | 1000 | Max cached query plans |
| `graph.bidi_threshold_depth` | 5 | Min depth to use bidirectional search |

---

## 10. Error Handling

| Error Type | Strategy |
|---|---|
| Traversal depth exceeded | Return partial results with `max_depth_reached` flag |
| Result count exceeded | Return top-N with `results_truncated` flag |
| Invalid start/end node | Return structured error |
| Graph statistics unavailable | Fall back to default algorithm heuristics |

---

## 11. Known Limitations & Future Work

- A* algorithm requires a user-supplied heuristic function; no built-in heuristic.
- Parallel traversal is experimental; correctness on concurrent graph mutations is not guaranteed.
- Subgraph isomorphism (exact pattern matching) is NP-complete; large patterns may be slow.
- Graph analytics (PageRank, centrality) delegates to `src/analytics/`; not in this module.

---

## 12. References

- `src/graph/README.md` — module overview
- `src/graph/ADVANCED_FEATURES_README.md` — advanced features
- `src/index/README.md` — GraphIndexManager documentation
- `docs/graph_roadmap.md` — graph roadmap
- `ARCHITECTURE.md` (root) — full system architecture
