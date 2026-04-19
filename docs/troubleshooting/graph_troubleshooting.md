# Graph Troubleshooting Guide

The `graph` module provides graph database functionality for ThemisDB, including parallel BFS/DFS traversal, path constraint filtering, query optimization for graph patterns, and integration with the graph index.

## Quick Diagnostics

| Symptom | Likely Cause | Quick Fix |
|---------|-------------|-----------|
| Graph traversal times out | No depth limit on traversal | Set `OPTIONS { maxDepth: 10 }` |
| Cyclic graph causes infinite loop | Visited-set not enabled | Enable `graph.traversal.track_visited` |
| Duplicate paths in result | Result deduplication disabled | Enable `graph.traversal.deduplicate_paths` |
| `GraphQueryOptimizer: no index found` | Graph index not created | Create graph index on edge collection |
| BFS memory spike | Too many reachable nodes | Limit `max_vertices` in traversal options |
| Parallel traversal slower than single | Too much thread contention | Reduce `graph.parallel.threads` |
| Missing edges after insert | Wrong edge `_from`/`_to` format | Use `collection/key` format for vertex IDs |
| Path constraints filter too aggressively | Edge attribute type mismatch | Ensure consistent data types in edge attributes |
| `SubgraphFan-out: OOM` | High-degree vertex with no limit | Set `max_vertex_out_degree` filter |
| Forbidden edge still returned | Edge filter cache stale | Flush graph query cache |

## Common Issues

### Issue 1: Traversal Timeout on Deep Graphs

**Description:** Graph traversals on deep or dense graphs time out.

**Symptoms:**
- Error: `ParallelTraversal: traversal aborted: timeout after 30000ms`
- AQL TRAVERSAL query hangs

**Cause:** No depth limit; the graph contains cycles or very high branching factor.

**Solution:**
```sql
-- Always set explicit depth limits
FOR vertex, edge, path IN 1..6   -- depth 1 to 6
    OUTBOUND @start_vertex @@edge_collection
    OPTIONS { maxDepth: 6, order: "bfs", uniqueVertices: "global" }
    RETURN vertex
```
```yaml
graph:
  traversal:
    default_max_depth: 10
    timeout_ms: 30000
    track_visited: true
```

---

### Issue 2: Traversal Does Not Terminate on Cyclic Graph

**Description:** A traversal loops forever because the graph contains cycles.

**Symptoms:**
- Log: `ParallelTraversal: visited count exceeds max_vertices limit`
- Query never returns

**Cause:** `uniqueVertices: "none"` allows revisiting the same vertex.

**Solution:**
```sql
FOR v, e, p IN 1..100
    OUTBOUND @start @@edges
    OPTIONS { uniqueVertices: "global", uniqueEdges: "path" }
    RETURN v
```
```yaml
graph:
  traversal:
    track_visited: true
    default_unique_vertices: global   # "none" | "path" | "global"
    max_vertices_per_traversal: 100000
```

---

### Issue 3: Graph Query Optimizer Chooses Full Scan

**Description:** Graph pattern queries perform full collection scans instead of index-assisted traversal.

**Symptoms:**
- EXPLAIN shows `CollectionScan` on vertex collection
- Traversal is slow even for small result sets

**Cause:** No persistent graph definition or graph index is missing.

**Solution:**
```bash
# Create graph definition
themisdb-admin graph create \
  --name social_graph \
  --edge-collection follows \
  --from-collection users \
  --to-collection users

# Create index on edge _from/_to fields
themisdb-admin index create \
  --collection follows \
  --type edge \
  --fields [_from, _to]
```

---

### Issue 4: BFS Memory Spike on Large Graphs

**Description:** BFS traversal consumes excessive memory on graphs with millions of reachable nodes.

**Symptoms:**
- Process OOM-killed during graph traversal
- Log: `ParallelTraversal: BFS queue size=500000 – memory pressure`

**Cause:** BFS queue holds all discovered but unvisited vertices; unbounded on large graphs.

**Solution:**
```sql
-- Limit BFS result size
FOR v IN 1..5 OUTBOUND @start @@edges
    OPTIONS { order: "bfs", maxVertices: 10000 }
    LIMIT 1000
    RETURN v
```
```yaml
graph:
  traversal:
    max_vertices_per_traversal: 50000
    bfs_queue_limit: 100000
    enable_early_termination: true
```

---

### Issue 5: Duplicate Paths in Result

**Description:** The traversal result contains the same path multiple times.

**Symptoms:**
- Result set has duplicate `_key` values for vertices
- Log: `ParallelTraversal: duplicate path detected`

**Cause:** Multi-threaded traversal finds same paths via different routes when deduplication is off.

**Solution:**
```yaml
graph:
  traversal:
    deduplicate_paths: true
    deduplicate_vertices: true
```
```sql
-- Use DISTINCT in AQL
FOR v IN 1..5 OUTBOUND @start @@edges
    OPTIONS { uniqueVertices: "global" }
    RETURN DISTINCT v._key
```

---

### Issue 6: Missing Edges After Bulk Insert

**Description:** After inserting many edges, graph traversal does not find all of them.

**Symptoms:**
- `edge_collection` has N documents but traversal finds fewer edges
- Log: `GraphIndex: skipping edge with invalid _from format`

**Cause:** Edges inserted with wrong `_from`/`_to` format (should be `collection/key`).

**Solution:**
```bash
# Find edges with invalid format
themisdb-admin query run \
  --query "FOR e IN follows FILTER e._from !~ '^[a-z_]+/' RETURN e._key" \
  --output /tmp/invalid_edges.json

# Fix with migration
themisdb-admin graph repair-edges \
  --collection follows \
  --from-collection users \
  --to-collection users
```

---

### Issue 7: Path Constraints Return Wrong Results

**Description:** Path attribute filters return unexpected results.

**Symptoms:**
- AQL `FILTER path.edges[*].weight ALL > 5` returns paths with weight ≤ 5
- Type mismatch: some edges store weight as string

**Cause:** Edge attribute has inconsistent types across documents.

**Solution:**
```sql
-- Defensive type coercion in path constraints
FOR v, e, path IN 1..6 OUTBOUND @start @@edges
    FILTER path.edges[*].weight ALL > 5
    FILTER path.edges[*].active ALL == true
    LET weights = (
      FOR edge IN path.edges
        RETURN TO_NUMBER(edge.weight)   -- coerce to number
    )
    FILTER MIN(weights) > 5
    RETURN path
```

---

### Issue 8: Parallel Traversal Slower Than Single-Threaded

**Description:** Enabling parallel traversal actually slows down graph queries.

**Symptoms:**
- P99 latency increases after setting `graph.parallel.threads: 8`
- CPU usage is high but throughput drops

**Cause:** Thread contention on visited-set mutex; graph is too small to benefit from parallelism.

**Solution:**
```yaml
graph:
  parallel:
    enabled: true
    threads: 4
    min_vertices_for_parallel: 10000   # only use parallel for large graphs
    contention_limit: 0.30             # fall back to serial if > 30% threads contend
```

## Diagnostic Commands

```bash
# List defined graphs
themisdb-admin graph list

# Graph statistics
themisdb-admin graph stats --graph social_graph

# Check index usage for graph
themisdb-admin query explain \
  --query "FOR v IN 1..5 OUTBOUND 'users/alice' GRAPH 'social_graph' RETURN v"

# Validate edge integrity
themisdb-admin graph validate --graph social_graph

# Live graph metrics
curl -s http://localhost:9100/metrics | grep themisdb_graph

# Tail graph logs
journalctl -u themisdb -f | grep -E "graph|traversal|path|BFS|DFS"
```

## Configuration Reference

```yaml
graph:
  traversal:
    default_max_depth: 10
    track_visited: true
    max_vertices_per_traversal: 100000
    timeout_ms: 60000
    deduplicate_paths: true
    default_unique_vertices: global
  parallel:
    enabled: true
    threads: 4
    min_vertices_for_parallel: 5000
  query_optimizer:
    use_graph_index: true
    prefer_edge_index: true
```

**Common misconfigurations:**

| Key | Wrong | Correct |
|-----|-------|---------|
| `default_unique_vertices` | `none` | `global` or `path` |
| `default_max_depth` | unset | `10` |
| `max_vertices_per_traversal` | unset | `100000` |
| `parallel.threads` | `16` on 4-core server | Match CPU core count |

## Known Limitations

- Parallel traversal does not yet support weighted shortest-path algorithms (Dijkstra/A*).
- Graph traversal across shards requires cross-shard queries; performance may degrade for deeply distributed graphs.
- Path deduplication in `global` mode uses a hash set in memory; very large traversals may exhaust heap.
- Graph index does not store edge weights; range queries on edge weights require full scan.

## Related Documentation

- [Graph Module ROADMAP](../../src/graph/ROADMAP.md)
- [Graph Roadmap](../de/roadmap/graph_roadmap.md)
- [General Traversal Feature](../ARCHIVED/implementation-summaries/GENERAL_TRAVERSAL_FEATURE.md)
- [Community Detection AQL](../ARCHIVED/implementation-summaries/COMMUNITY_DETECTION_AQL.md)
- [AQL Roadmap](../de/roadmap/aql_roadmap.md)
