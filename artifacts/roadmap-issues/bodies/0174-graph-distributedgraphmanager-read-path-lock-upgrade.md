### Context

This issue implements the roadmap item '`DistributedGraphManager`: Read-Path Lock Upgrade' for the graph domain. It is sourced from the consolidated roadmap under 🟡 Medium Priority — Near-term (v1.5.0 – v1.8.0) and targets milestone v1.8.0.

Primary detail section: `DistributedGraphManager`: Read-Path Lock Upgrade

### Goal

Deliver the scoped changes for `DistributedGraphManager`: Read-Path Lock Upgrade in src/graph/ and complete the linked detail section in a release-ready state for v1.8.0.

### Detailed Scope

### `DistributedGraphManager`: Read-Path Lock Upgrade
**Priority:** Medium
**Target Version:** v1.8.0

`distributed_graph.cpp` uses `std::lock_guard<std::mutex>` for all shard operations including read-only lookups (`getShard` at line 110, `listShards` at line 120, `execute` at line 126). All reader threads serialize unnecessarily.

**Implementation Notes:**
- `[ ]` Replace `std::mutex shards_mutex_` with `std::shared_mutex` in `DistributedGraphManager`; upgrade `getShard`, `listShards`, and `execute` (read path) to `std::shared_lock`.
- `[ ]` Keep `addShard` and `removeShard` (write path) on `std::unique_lock`.
- `[ ]` Add a TSAN-enabled stress test: 8 concurrent `execute()` threads + 1 `addShard()` thread.

---


**Priority:** Medium  
**Target Version:** v1.7.0

Extend PathConstraints with more sophisticated constraint types.

**Features:**
- **Node Property Constraints** ✅ DONE – `addNodePropertyConstraint(key, value)` prunes BFS traversal
- **Weight Constraints** ✅ DONE – `addMaxWeight(threshold)` prunes BFS; `addMinWeight(threshold)` rejects at acceptance
- **Schema-Aware Node Label Hints** ✅ DONE – `QueryConstraints::node_labels` filters BFS/DFS by `_labels` field
- **Excluded Edge Type Hints** ✅ DONE – `QueryConstraints::excluded_edge_types` reduces cost-model fanout estimate
- **Temporal Constraints**: Path valid at specific time ⏳ Planned
- **Probability Constraints**: Min probability for uncertain graphs ⏳ Planned
- **Resource Constraints**: Capacity limits on paths ⏳ Planned
- **Semantic Constraints**: Ontology-based path rules ⏳ Planned
- **Geo-Fence Constraints**: Spatial boundaries for paths ⏳ Planned

**Implemented API:**
```cpp
PathConstraints constraints(&graph_mgr);

// Node property constraint (v1.7.0)
constraints.addNodePropertyConstraint("country", "USA");
// → Only traverse nodes where node.country == "USA"

// Weight constraints (v1.7.0)
constraints.addMaxWeight(100.0);  // Total path weight <= 100 (BFS pruning)
constraints.addMinWeight(10.0);   // Total path weight >= 10 (acceptance check)

auto paths = constraints.findConstrainedPaths("start", "end", 10);
```

**Planned API (not yet implemented):**
```cpp
// Temporal constraint
constraints.addTemporalConstraint(
    start_time_ms,
    end_time_ms,
    TemporalMode::VALID_DURING
);

// Resource constraint
constraints.addResourceCapacity("bandwidth", 1000);

// Geo-fence constraint
constraints.addGeoFence(
    center_lat, center_lon, radius_km,
    GeoFenceMode::MUST_STAY_INSIDE
);
```

**Implementation Notes:**
- `getNodeField(vertexId, fieldName)` added to `GraphIndexManager` (uses `node:<pk>` key format)
- `ConstraintType::MAX_WEIGHT` / `MIN_WEIGHT` added to `PathConstraints::ConstraintType` enum
- `Constraint::double_value` field stores threshold for weight constraints
- BFS pruner checks `MAX_WEIGHT` after each edge weight accumulation
- `validatePath` enforces `NODE_PROPERTY` for all nodes; weight constraints handled by `findConstrainedPaths`

---

### Acceptance Criteria

- [ ] Replace `std::mutex shards_mutex_` with `std::shared_mutex` in `DistributedGraphManager`; upgrade `getShard`, `listShards`, and `execute` (read path) to `std::shared_lock`.
- [ ] Keep `addShard` and `removeShard` (write path) on `std::unique_lock`.
- [ ] Add a TSAN-enabled stress test: 8 concurrent `execute()` threads + 1 `addShard()` thread.
- [ ] **Node Property Constraints** ✅ DONE – `addNodePropertyConstraint(key, value)` prunes BFS traversal
- [ ] **Weight Constraints** ✅ DONE – `addMaxWeight(threshold)` prunes BFS; `addMinWeight(threshold)` rejects at acceptance
- [ ] **Schema-Aware Node Label Hints** ✅ DONE – `QueryConstraints::node_labels` filters BFS/DFS by `_labels` field
- [ ] **Excluded Edge Type Hints** ✅ DONE – `QueryConstraints::excluded_edge_types` reduces cost-model fanout estimate
- [ ] **Temporal Constraints**: Path valid at specific time ⏳ Planned
- [ ] **Probability Constraints**: Min probability for uncertain graphs ⏳ Planned
- [ ] **Resource Constraints**: Capacity limits on paths ⏳ Planned
- [ ] **Semantic Constraints**: Ontology-based path rules ⏳ Planned
- [ ] **Geo-Fence Constraints**: Spatial boundaries for paths ⏳ Planned
- [ ] `getNodeField(vertexId, fieldName)` added to `GraphIndexManager` (uses `node:<pk>` key format)
- [ ] `ConstraintType::MAX_WEIGHT` / `MIN_WEIGHT` added to `PathConstraints::ConstraintType` enum
- [ ] `Constraint::double_value` field stores threshold for weight constraints
- [ ] BFS pruner checks `MAX_WEIGHT` after each edge weight accumulation
- [ ] `validatePath` enforces `NODE_PROPERTY` for all nodes; weight constraints handled by `findConstrainedPaths`

### Relationships

- Roadmap row: #174 (🟡 Medium Priority — Near-term (v1.5.0 – v1.8.0))
- Depends on: none identified during generation.
- Part of: consolidated roadmap delivery tracking.

### References

- src/ROADMAP.md
- src/graph/FUTURE_ENHANCEMENTS.md#distributedgraphmanager-read-path-lock-upgrade
- Source key: roadmap:174:graph:v1.8.0:distributedgraphmanager-read-path-lock-upgrade

Generated from the consolidated source roadmap. Keep the roadmap and issue in sync when scope changes.

<!-- roadmap-source-key: roadmap:174:graph:v1.8.0:distributedgraphmanager-read-path-lock-upgrade -->
<!-- roadmap-ref: row=174;module=graph;target=v1.8.0 -->
<!-- roadmap-detail: src/graph/FUTURE_ENHANCEMENTS.md#distributedgraphmanager-read-path-lock-upgrade -->
