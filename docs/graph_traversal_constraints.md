# Graph Traversal Constraints

This document describes extended path constraints for AQL graph traversals.

## Syntax

After the `GRAPH "<name>"` clause of a traversal, optional constraint keywords can follow in any order:

- `NO_BACKTRACK`: Prevents immediate return to the previous vertex (avoids A→B→A steps).
- `EDGE_LABEL_WHITELIST ["L1", "L2"]`: Only edges with one of these labels/types are allowed.
- `EDGE_LABEL_BLACKLIST ["Lx"]`: Exclude edges with these labels/types.
- `NODE_LABEL_WHITELIST ["Person"]`: Only nodes with one of these labels/types are allowed.
- `NODE_LABEL_BLACKLIST ["Bot"]`: Exclude nodes with these labels/types.

Labels are resolved in this order:
1. `labels` array on the entity (if present)
2. `_type` string field (fallback)
3. `type` string field (fallback)

## Examples

- Outbound traversal with edge filter and no-backtrack:

```
FOR v IN 1..3 OUTBOUND "user/123" TYPE "friend_of" GRAPH "social"
  NO_BACKTRACK EDGE_LABEL_WHITELIST ["friend_of"]
RETURN v
```

- Inbound traversal with node blacklist:

```
FOR v IN 1..2 INBOUND "user/123" GRAPH "social"
  NODE_LABEL_BLACKLIST ["Deactivated"]
RETURN v
```

- Shortest path with node+edge constraints:

```
FOR v IN 1..5 OUTBOUND "user/123" GRAPH "social"
  SHORTEST_PATH TO "user/789"
  NODE_LABEL_WHITELIST ["Person"] EDGE_LABEL_BLACKLIST ["blocked"]
RETURN v
```

## Engine Behavior

- Constraints are enforced in the QueryEngine for both traversal (BFS) and shortest-path results.
- For plain outbound traversals without constraints, an optimized index BFS is used.
- If constraints or non-outbound directions are used, QueryEngine performs a constraint-aware BFS.
- For shortest path, constraints are validated on the returned path.

## Direction

- Directions: `OUTBOUND`, `INBOUND`, `ANY`.
- Outbound uses outgoing edges; Inbound uses incoming edges; Any considers both.
- Shortest-path currently follows the graph's directed edges; explicit directional shortest-path is not yet exposed in GraphIndex.

