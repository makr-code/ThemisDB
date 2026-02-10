---
name: GAP-006 Path Constraints Implementation
about: Implement real path constraints algorithm for complex graph traversal
title: '[GAP-006] Implement PathConstraints Algorithm'
labels: ['type:feature', 'area:graph', 'gap:006', 'priority:P2', 'status:ready']
assignees: ''
---

## Implementation Task
Implement the PathConstraints algorithm to enable complex path finding with multiple constraint types in graph traversals.

**Current Status:** Stub implementation (returns NOT_IMPLEMENTED)  
**Target Release:** Q2 2026  
**Priority:** P2 - High  
**Effort:** Large (2-3 weeks)

## Background

PathConstraints provides an interface for finding paths in graphs that satisfy complex requirements beyond simple shortest-path queries. The stub interface is defined in:
- `include/graph/path_constraints.h` - Complete interface definition
- `src/graph/path_constraints.cpp` - Stub implementation (needs real algorithm)

## Constraint Types to Support

The implementation must handle these 12 constraint types:
- [ ] **MIN_LENGTH** - Minimum path length (number of hops)
- [ ] **MAX_LENGTH** - Maximum path length (number of hops)
- [ ] **NODE_PROPERTY** - Node must have specific properties
- [ ] **EDGE_PROPERTY** - Edge must have specific properties
- [ ] **FORBIDDEN_NODE** - Path cannot include specific nodes
- [ ] **REQUIRED_NODE** - Path must include specific nodes
- [ ] **FORBIDDEN_EDGE** - Path cannot include specific edges
- [ ] **REQUIRED_EDGE** - Path must include specific edges
- [ ] **NO_CYCLES** - Path must be acyclic
- [ ] **UNIQUE_NODES** - All nodes in path must be unique
- [ ] **UNIQUE_EDGES** - All edges in path must be unique
- [ ] **CUSTOM_PREDICATE** - Custom validation function

## Required Implementation

### 1. Core Algorithm - Constrained BFS/DFS

**File:** `src/graph/path_constraints.cpp`

Implement `findConstrainedPaths()` method using modified graph traversal:

```cpp
Result<std::vector<PathResult>> PathConstraints::findConstrainedPaths(
    std::string_view start_node,
    std::string_view end_node,
    int max_results) const {
    
    // IMPLEMENTATION STEPS:
    
    // 1. Initialize data structures
    //    - Priority queue or stack for traversal
    //    - Visited set (if UNIQUE_NODES constraint)
    //    - Path tracking structure
    //    - Results vector
    
    // 2. Validate constraints compatibility
    //    - Check for contradictory constraints
    //    - Validate min_length <= max_length
    //    - Check if required nodes/edges are not forbidden
    
    // 3. Choose traversal strategy based on constraints:
    //    - Use BFS if MIN_LENGTH or shortest paths preferred
    //    - Use DFS if exploring deep paths or MAX_LENGTH is primary
    //    - Use bidirectional search for long-distance paths
    
    // 4. Main traversal loop:
    //    while (not exhausted and results < max_results):
    //      - Get next path/node to explore
    //      - Check early termination conditions (MAX_LENGTH, etc.)
    //      - For each neighbor:
    //        * Check FORBIDDEN_NODE/EDGE constraints
    //        * Check UNIQUE_NODES/EDGES constraints
    //        * Check cycle constraints
    //        * Validate against custom predicates
    //        * If valid, add to exploration queue
    //      - If target reached and all constraints satisfied:
    //        * Verify REQUIRED_NODE/EDGE constraints
    //        * Verify length constraints
    //        * Add to results
    
    // 5. Post-processing:
    //    - Sort results by path length or cost
    //    - Limit to max_results
    //    - Return
}
```

### 2. Enhanced Validation

Improve `validatePath()` to check all constraints comprehensively:

```cpp
Result<bool> PathConstraints::validatePath(
    const std::vector<std::string>& nodes,
    const std::vector<std::string>& edges) const {
    
    // Check all constraint types:
    // - Length constraints (MIN/MAX)
    // - Forbidden nodes/edges (none in path)
    // - Required nodes/edges (all present in path)
    // - Uniqueness (no duplicates if required)
    // - Acyclic (no node appears twice)
    // - Custom predicates (all return true)
    // - Node/edge properties (query GraphIndexManager)
}
```

### 3. Integration with GraphIndexManager

**Dependencies:**
```cpp
#include "index/graph_index.h"

// Use existing methods:
// - graphMgr_.outNeighbors(node) - Get outgoing edges
// - graphMgr_.inNeighbors(node) - Get incoming edges
// - graphMgr_.getEdge(from, to) - Get edge data
// - graphMgr_.getVertex(id) - Get node data
```

### 4. Integration with GraphQueryOptimizer

**File:** `include/graph/graph_query_optimizer.h`

Add support for PathConstraints in query optimization:

```cpp
// In GraphQueryOptimizer class, add:
Result<OptimizationPlan> optimizeConstrainedPath(
    std::string_view start_vertex,
    std::string_view end_vertex,
    const PathConstraints& constraints
);
```

### 5. Performance Optimizations

Implement these optimizations:

1. **Early Termination**
   - Stop exploring paths that exceed MAX_LENGTH
   - Skip branches with forbidden nodes early
   - Use heuristics to prioritize promising paths

2. **Caching**
   - Cache adjacency information
   - Cache property lookups
   - Consider memoization for subpaths

3. **Parallel Exploration** (Optional)
   - Explore multiple paths concurrently
   - Use thread pool for path validation
   - Careful with shared state

4. **Index Utilization**
   - Use graph indices for neighbor lookups
   - Leverage edge indices when filtering by edge properties

## Testing Requirements

### Unit Tests

**File:** `tests/test_path_constraints.cpp` (create new or expand existing)

```cpp
// Test each constraint type individually
TEST(PathConstraintsTest, MinMaxLengthConstraints) {
    // Create small test graph
    // Add min/max length constraints
    // Verify paths satisfy constraints
}

TEST(PathConstraintsTest, ForbiddenRequiredNodes) {
    // Test forbidden and required node constraints
}

TEST(PathConstraintsTest, UniquenessConstraints) {
    // Test UNIQUE_NODES and UNIQUE_EDGES
}

TEST(PathConstraintsTest, AcyclicConstraint) {
    // Test NO_CYCLES constraint
}

TEST(PathConstraintsTest, CombinedConstraints) {
    // Test multiple constraints together
    // Test constraint conflicts
}

TEST(PathConstraintsTest, PerformanceLargeGraph) {
    // Test with 10K+ nodes
    // Verify reasonable performance
}
```

### Integration Tests

**File:** `tests/integration/test_path_constraints_integration.cpp`

```cpp
// Test integration with GraphIndexManager
TEST(PathConstraintsIntegrationTest, RealGraphData) {
    // Load actual graph data
    // Apply various constraints
    // Verify correctness
}

// Test integration with GraphQueryOptimizer
TEST(PathConstraintsIntegrationTest, QueryOptimization) {
    // Verify query optimizer uses constraints effectively
}
```

## Algorithm Complexity

**Target Complexity:**
- **Time:** O(V + E) × f(constraints) where f is constraint checking overhead
  - Best case: O(E) for simple paths
  - Worst case: O(V!) for exploring all paths (use MAX_RESULTS to limit)
  
- **Space:** O(V) for visited set + O(k × L) for storing k paths of length L

**Optimization Goals:**
- Find paths in < 100ms for graphs with 100K nodes
- Support at least 5 simultaneous constraints efficiently
- Scale to graphs with 1M+ nodes for simple constraints

## Implementation Checklist

### Phase 1: Core Algorithm (Week 1)
- [ ] Implement basic constrained BFS
- [ ] Support MIN_LENGTH, MAX_LENGTH constraints
- [ ] Support FORBIDDEN_NODE, REQUIRED_NODE
- [ ] Basic unit tests

### Phase 2: Extended Constraints (Week 2)
- [ ] Implement UNIQUE_NODES, UNIQUE_EDGES
- [ ] Implement NO_CYCLES
- [ ] Support FORBIDDEN_EDGE, REQUIRED_EDGE
- [ ] Support CUSTOM_PREDICATE
- [ ] Comprehensive unit tests

### Phase 3: Optimization & Integration (Week 3)
- [ ] Optimize with early termination
- [ ] Add caching mechanisms
- [ ] Integrate with GraphQueryOptimizer
- [ ] Integration tests
- [ ] Performance benchmarks
- [ ] Documentation updates

### Phase 4: Polish (Additional Days)
- [ ] Edge case handling
- [ ] Error message improvements
- [ ] Code review and refactoring
- [ ] Update examples in README

## Success Criteria

- [ ] All 12 constraint types implemented and tested
- [ ] Unit test coverage > 90%
- [ ] Passes all integration tests
- [ ] Performance targets met (< 100ms for 100K node graphs)
- [ ] No memory leaks (Valgrind clean)
- [ ] Documentation complete
- [ ] Examples added to `src/graph/ADVANCED_FEATURES_README.md`

## References

### Academic Papers
1. Yen, J. Y. (1971). "Finding the k shortest loopless paths in a network"
2. Roditty, L., & Zwick, U. (2005). "Improved dynamic reachability algorithms for directed graphs"

### Existing Code to Study
- `src/index/graph_analytics.cpp` - K-Shortest Paths implementation (Yen's algorithm)
- `include/graph/graph_query_optimizer.h` - Query optimization patterns
- `tests/test_graph_query_optimizer.cpp` - Testing patterns

### Similar Implementations
- Neo4j Cypher: Path pattern matching with constraints
- ArangoDB AQL: Graph traversal with filters
- JanusGraph: Complex path queries

## Notes

- Keep interface backward compatible (stub users should work with real implementation)
- Consider memory usage for storing multiple paths
- Document complexity trade-offs in README
- Add configuration options for tuning (max iterations, timeout, etc.)

## Related Issues

- GAP-006 Main tracking issue
- Graph Query Optimizer enhancements
- GraphIndexManager performance optimization
