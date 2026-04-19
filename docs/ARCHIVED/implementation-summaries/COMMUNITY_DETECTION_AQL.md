# Community Detection AQL Functions

## Overview

ThemisDB now exposes two community detection algorithms through AQL:
- **LOUVAIN_COMMUNITIES** - Greedy modularity optimization (Blondel et al., 2008)
- **LABEL_PROPAGATION_COMMUNITIES** - Fast label propagation (Raghavan et al., 2007)

Both functions work on edge arrays and return a mapping of node IDs to community IDs.

## Function Signatures

### LOUVAIN_COMMUNITIES

```aql
LOUVAIN_COMMUNITIES(edges [, min_modularity_gain])
```

**Parameters:**
- `edges` (Array) - Array of edge documents with `_from` and `_to` fields
- `min_modularity_gain` (Number, optional) - Minimum modularity gain threshold (default: 0.000001)

**Returns:** Object mapping vertex IDs to community IDs

**Example:**
```aql
FOR doc IN edges_collection
  COLLECT communities = LOUVAIN_COMMUNITIES(doc)
  RETURN communities
```

### LABEL_PROPAGATION_COMMUNITIES

```aql
LABEL_PROPAGATION_COMMUNITIES(edges [, max_iterations])
```

**Parameters:**
- `edges` (Array) - Array of edge documents with `_from` and `_to` fields
- `max_iterations` (Integer, optional) - Maximum propagation iterations (default: 100)

**Returns:** Object mapping vertex IDs to community IDs

**Example:**
```aql
FOR doc IN edges_collection
  COLLECT communities = LABEL_PROPAGATION_COMMUNITIES(doc, 50)
  RETURN communities
```

## Usage Examples

### Basic Community Detection

```aql
// Detect communities in a social network
LET edges = [
  {_from: "user/alice", _to: "user/bob"},
  {_from: "user/bob", _to: "user/charlie"},
  {_from: "user/charlie", _to: "user/alice"},
  {_from: "user/dave", _to: "user/eve"},
  {_from: "user/eve", _to: "user/dave"}
]

LET communities = LOUVAIN_COMMUNITIES(edges)

RETURN communities
// Returns: {"user/alice": 0, "user/bob": 0, "user/charlie": 0, 
//           "user/dave": 1, "user/eve": 1}
```

### Analyze Community Sizes

```aql
LET edges = (FOR e IN social_edges RETURN e)
LET communities = LOUVAIN_COMMUNITIES(edges)

// Group nodes by community
LET community_groups = (
  FOR node, comm IN communities
    COLLECT community_id = comm INTO group
    RETURN {
      community_id: community_id,
      members: group[*].node,
      size: LENGTH(group)
    }
)

// Return largest communities
FOR comm IN community_groups
  FILTER comm.size > 10
  SORT comm.size DESC
  LIMIT 10
  RETURN comm
```

### Compare Algorithms

```aql
LET edges = (FOR e IN network_edges RETURN e)

LET louvain_result = LOUVAIN_COMMUNITIES(edges)
LET label_prop_result = LABEL_PROPAGATION_COMMUNITIES(edges)

RETURN {
  louvain: {
    communities: LENGTH(UNIQUE(VALUES(louvain_result))),
    assignments: louvain_result
  },
  label_propagation: {
    communities: LENGTH(UNIQUE(VALUES(label_prop_result))),
    assignments: label_prop_result
  }
}
```

### Find Communities with Specific Properties

```aql
LET edges = (FOR e IN collaboration_edges RETURN e)
LET communities = LOUVAIN_COMMUNITIES(edges, 0.0001)

// Find all nodes in community 0
LET community_0_nodes = (
  FOR node, comm IN communities
    FILTER comm == 0
    RETURN node
)

RETURN {
  community_id: 0,
  members: community_0_nodes,
  size: LENGTH(community_0_nodes)
}
```

### Iterative Analysis with Label Propagation

```aql
// Use fewer iterations for faster convergence on large graphs
LET edges = (FOR e IN huge_graph_edges LIMIT 100000 RETURN e)
LET communities = LABEL_PROPAGATION_COMMUNITIES(edges, 20)

// Count communities
LET num_communities = LENGTH(UNIQUE(VALUES(communities)))

RETURN {
  total_nodes: LENGTH(KEYS(communities)),
  total_communities: num_communities,
  avg_community_size: LENGTH(KEYS(communities)) / num_communities
}
```

## Performance Considerations

### Louvain Algorithm
- **Complexity:** O(n log n) on average, O(n²) worst case
- **Best for:** Moderate to large graphs (1K-100K nodes)
- **Iterations:** Typically converges in 10-50 iterations
- **Quality:** Generally produces higher modularity communities

### Label Propagation
- **Complexity:** O(m + n) per iteration, where m = edges, n = nodes
- **Best for:** Large graphs requiring fast results (10K-1M+ nodes)
- **Iterations:** Can converge in 5-20 iterations
- **Quality:** Fast but may produce suboptimal communities

### Tuning Parameters

**Louvain:**
- Lower `min_modularity_gain` (e.g., 0.0000001) → More iterations, better quality
- Higher `min_modularity_gain` (e.g., 0.001) → Fewer iterations, faster execution

**Label Propagation:**
- Lower `max_iterations` (e.g., 10) → Faster execution, may not fully converge
- Higher `max_iterations` (e.g., 200) → Better convergence, slower execution

## Algorithm Comparison

| Aspect | Louvain | Label Propagation |
|--------|---------|-------------------|
| Speed | Moderate (O(n log n)) | Fast (O(m + n)) |
| Quality | High modularity | Good, may vary |
| Determinism | Deterministic | Deterministic* |
| Best Use Case | Research, analysis | Production, real-time |
| Memory | Moderate | Low |

*Both implementations use deterministic node ordering for reproducibility.

## Implementation Notes

- Both functions follow the existing AQL graph function pattern
- They accept edge arrays as input (consistent with PAGERANK, SHORTEST_PATH, etc.)
- Return format is a JSON object mapping node IDs to community IDs
- Community IDs are contiguous integers starting from 0
- Isolated nodes are assigned their own community
- Functions are registered in the global function registry

## Source Code

- Implementation: `include/query/functions/graph_functions.h`
- Core algorithms: `src/index/graph_analytics.cpp`
- Tests: `tests/test_community_detection_aql.cpp`

## References

1. **Louvain Algorithm:**
   - Blondel, V. D., et al. (2008). "Fast unfolding of communities in large networks."
   - Journal of Statistical Mechanics: Theory and Experiment.

2. **Label Propagation:**
   - Raghavan, U. N., et al. (2007). "Near linear time algorithm to detect community structures in large-scale networks."
   - Physical Review E.

3. **ThemisDB Documentation:**
   - Graph Functions: https://makr-code.github.io/ThemisDB/aql/graph-functions/
   - AQL Reference: https://makr-code.github.io/ThemisDB/aql/
