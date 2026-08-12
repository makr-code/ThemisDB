> **⚠️ ARCHIVIERUNGSHINWEIS:** Diese Datei ist ein Duplikat die bereits unter `docs/ARCHIVED/implementation-summaries/` archiviert wurde. Der Inhalt hier dient nur als Referenz. Bitte nutze die archivierte Version als kanonische Quelle.
>
> **Status: archive-candidate** | Inventar: [DOCS_INVENTORY_2026-Q3.md](Audit/DOCS_INVENTORY_2026-Q3.md)

---

# General Graph Traversal Feature

## Overview

ThemisDB now supports general graph traversal for depth-limited exploration. This feature allows you to explore graph neighborhoods without requiring shortest path computation.

## Feature Implementation

### What's New

1. **General Traversal Method**: `QueryEngine::executeGeneralTraversal()` implements BFS traversal with depth filtering
2. **Direction Support**: OUTBOUND, INBOUND, and ANY traversal directions
3. **Depth Range Filtering**: Specify minDepth and maxDepth to control result set
4. **Path Tracking**: Full path from start vertex to each result vertex
5. **Edge Tracking**: List of edge IDs traversed to reach each vertex

### AQL Syntax

```aql
FOR v IN minDepth..maxDepth OUTBOUND|INBOUND|ANY startVertex GRAPH graphName
RETURN v
```

### Examples

#### Example 1: Find All Friends Within 2 Hops

```aql
FOR user IN 1..2 OUTBOUND "users/alice" GRAPH "social"
RETURN user
```

This finds all users reachable from Alice within 1-2 hops following outbound edges.

#### Example 2: Community Detection (2-3 Hops)

```aql
FOR entity IN 2..3 OUTBOUND "company/acme" GRAPH "business"
RETURN entity
```

Finds entities 2-3 hops away from a company, useful for community detection.

#### Example 3: Reverse Connection Discovery

```aql
FOR influencer IN 1..2 INBOUND "users/celebrity" GRAPH "social"
RETURN influencer
```

Finds users who follow the celebrity (reverse direction).

#### Example 4: Bidirectional Exploration

```aql
FOR neighbor IN 1..1 ANY "users/bob" GRAPH "social"
RETURN neighbor
```

Finds all immediate neighbors in any direction (both followers and following).

#### Example 5: Include Starting Vertex

```aql
FOR v IN 0..2 OUTBOUND "users/alice" GRAPH "social"
RETURN v
```

Setting minDepth=0 includes the starting vertex in results.

## Result Format

Each traversal result includes:

```json
{
  "type": "traversal",
  "results": [
    {
      "vertex": "users/bob",
      "depth": 1,
      "path": ["users/alice", "users/bob"],
      "edges": ["e1"],
      "data": {
        "_key": "users/bob",
        "name": "Bob",
        "age": 30
      }
    }
  ]
}
```

### Fields

- **vertex**: Primary key of the reached vertex
- **depth**: Number of hops from start vertex
- **path**: Complete path from start to this vertex (array of vertex PKs)
- **edges**: Edge IDs traversed to reach this vertex
- **data**: Full vertex entity data loaded from storage

## Implementation Details

### Algorithm

- **Traversal Method**: Breadth-First Search (BFS)
- **Cycle Prevention**: Visited set prevents infinite loops
- **Depth Tracking**: Each node tracks its depth from start
- **Path Reconstruction**: Full path stored with each node

### Performance

- **Time Complexity**: O(V + E) where V is reachable vertices, E is edges
- **Space Complexity**: O(V) for visited set and queue
- **Graph Size**: Tested up to 10K vertices, 50K edges
- **Query Latency**: < 100ms for depth 3 traversal (p50)

### Optimizations

1. **Early Termination**: Stops expanding at maxDepth
2. **Visited Tracking**: Prevents revisiting nodes
3. **Direction Filtering**: Only explores relevant edges
4. **Graph ID Scoping**: Filters by graph namespace

## API Changes

### Header Files

**include/query/query_engine.h**:
- Added `TraversalDirection` enum
- Added `TraversalResult` struct
- Added `executeGeneralTraversal()` method

**src/query/query_engine.cpp**:
- Implemented `executeGeneralTraversal()` with BFS algorithm

**src/query/aql_runner.cpp**:
- Updated traversal dispatch to call general traversal for non-shortest-path queries
- Converts AQL direction enum to TraversalDirection

## Testing

### Unit Tests

**tests/test_general_traversal.cpp**:
- BasicOutboundTraversal
- MinDepthFiltering
- InboundDirection
- AnyDirection
- PathTracking
- EdgeTracking
- DiamondGraphMultiplePaths
- InvalidDepthRange
- EmptyStartVertex

### Integration Tests

**tests/test_aql_general_traversal.cpp**:
- BasicOutboundTraversal (via AQL)
- MinDepthFiltering (via AQL)
- InboundDirection (via AQL)
- AnyDirection (via AQL)
- PathAndEdgeTracking (via AQL)
- DepthZeroIncludesStart (via AQL)

## Migration Guide

### Before (Not Working)

```aql
FOR v IN 1..3 OUTBOUND "start" GRAPH "g"
RETURN v
```

**Result**: Error - "Traversal dispatch (non-shortest) not implemented"

### After (Working)

```aql
FOR v IN 1..3 OUTBOUND "start" GRAPH "g"
RETURN v
```

**Result**: Returns all vertices reachable at depths 1-3 with full path information

## Limitations

1. **Edge Type Filtering**: Not yet exposed in AQL syntax (internal support exists)
2. **Filter Expressions**: FILTER clauses on traversal not yet implemented
3. **Multiple Paths**: Only returns one path per vertex (first discovered via BFS)
4. **Temporal Constraints**: Temporal filtering not yet integrated with general traversal

## Future Enhancements

1. **K-Shortest-Paths**: Implement Yen's algorithm for multiple paths (P2)
2. **Edge Type Filters**: Expose edge type filtering in AQL syntax
3. **Filter Support**: Add FILTER clause support during traversal
4. **Community Detection**: Implement Louvain algorithm (P2)
5. **PageRank**: Add graph analytics algorithms (P3)
6. **Temporal Support**: Integrate temporal constraints (P3)

## Related Documentation

- **AQL Reference**: docs/de/aql/aql_query_engine.md
- **Graph Functions**: include/query/functions/graph_functions.h
- **Development Status**: docs/de/development/stub_simulation_audit_2025-11.md

## References

- ArangoDB AQL Graphs: https://www.arangodb.com/docs/stable/aql/graphs.html
- BFS Algorithm: O(V+E) complexity
- Graph Traversal Patterns: https://neo4j.com/docs/cypher-manual/current/clauses/match/
