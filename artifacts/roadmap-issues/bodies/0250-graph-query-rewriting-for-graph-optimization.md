### Context

This issue implements the roadmap item 'Query Rewriting for Graph Optimization' for the graph domain. It is sourced from the consolidated roadmap under 🟢 Low Priority — Future (v1.9.0+) and targets milestone v1.9.0.

Primary detail section: Query Rewriting for Graph Optimization

### Goal

Deliver the scoped changes for Query Rewriting for Graph Optimization in src/graph/ and complete the linked detail section in a release-ready state for v1.9.0.

### Detailed Scope

### Query Rewriting for Graph Optimization
**Priority:** Medium  
**Target Version:** v1.8.0

Automatically rewrite graph queries for better performance.

**Features:**
- Common subexpression elimination
- Predicate pushdown to graph layer
- Join reordering for graph patterns
- Materialized view utilization
- Query decomposition for parallelism

**Benefits:**
- Improved query performance without user intervention
- Better integration with relational/document queries
- Optimal execution plans for complex queries
- Reduced redundant computation

**Example Rewrite:**
```aql
-- Original query
FOR v1 IN vertices
  FILTER v1.type == "Person"
  FOR v2 IN 1..3 OUTBOUND v1 GRAPH "social"
    FILTER v2.country == "USA"
    RETURN {person: v1, friend: v2}

-- Rewritten query
FOR v1 IN vertices
  FILTER v1.type == "Person"
  FOR v2 IN 1..3 OUTBOUND v1 GRAPH "social"
    PRUNE v2.country != "USA"  // Early pruning
    FILTER v2.country == "USA"
    RETURN {person: v1, friend: v2}
```

**Rewrite Rules:**
- Push predicates into graph traversal (prune early)
- Decompose multi-pattern queries into independent subqueries
- Materialize frequently accessed subgraphs
- Convert repeated traversals to single traversal with caching
- Reorder multi-hop traversals based on selectivity

---

### Acceptance Criteria

- [ ] Common subexpression elimination
- [ ] Predicate pushdown to graph layer
- [ ] Join reordering for graph patterns
- [ ] Materialized view utilization
- [ ] Query decomposition for parallelism
- [ ] Improved query performance without user intervention
- [ ] Better integration with relational/document queries
- [ ] Optimal execution plans for complex queries
- [ ] Reduced redundant computation
- [ ] Push predicates into graph traversal (prune early)
- [ ] Decompose multi-pattern queries into independent subqueries
- [ ] Materialize frequently accessed subgraphs
- [ ] Convert repeated traversals to single traversal with caching
- [ ] Reorder multi-hop traversals based on selectivity

### Relationships

- Roadmap row: #250 (🟢 Low Priority — Future (v1.9.0+))
- Depends on: none identified during generation.
- Part of: consolidated roadmap delivery tracking.

### References

- src/ROADMAP.md
- src/graph/FUTURE_ENHANCEMENTS.md#query-rewriting-for-graph-optimization
- Source key: roadmap:250:graph:v1.9.0:query-rewriting-for-graph-optimization

Generated from the consolidated source roadmap. Keep the roadmap and issue in sync when scope changes.

<!-- roadmap-source-key: roadmap:250:graph:v1.9.0:query-rewriting-for-graph-optimization -->
<!-- roadmap-ref: row=250;module=graph;target=v1.9.0 -->
<!-- roadmap-detail: src/graph/FUTURE_ENHANCEMENTS.md#query-rewriting-for-graph-optimization -->
