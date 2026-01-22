---
name: ✨ Feature: Graph Edge Bridging on Node Deletion
about: Add option to bridge connections when deleting intermediate nodes in graph
title: "[GRAPH] Edge Bridging on Node Deletion"
labels: priority:P2, type:enhancement, area:graph, effort:medium, phase:feature
assignees: ''
---

## ✨ Feature Enhancement - Graph Operations

**Current Status:** Cascade deletion implemented, edge bridging pending  
**Priority:** P2 (Medium)  
**Effort:** 1-2 weeks  
**Target Version:** v1.5.0  
**Related PR:** #XXX (Cascade deletion for graph edges)  
**Related Files:**
- `include/index/property_graph.h`
- `src/index/property_graph.cpp`
- `tests/test_property_graph.cpp`

---

## 📋 Problem Description

Currently, when a node is deleted from the graph using `PropertyGraphManager::deleteNode()`, all connected edges (both incoming and outgoing) are cascade-deleted. This is the correct behavior for cleaning up orphaned edges and maintaining data consistency.

However, in some use cases, users may want to preserve the graph connectivity by **bridging** the connections between parent and child nodes when deleting an intermediate node.

**Example Scenario:**
```
Initial Graph: A → B → C
Delete Node B with cascade: A  C (edges lost)
Delete Node B with bridging: A → C (edge created)
```

**Use Cases:**
- **Organizational hierarchies:** Removing a manager should connect their subordinates to the next level up
- **Process flows:** Removing a step should maintain workflow continuity
- **Knowledge graphs:** Removing an intermediary concept should preserve relationships
- **Supply chains:** Removing a middleman should connect suppliers directly to consumers

---

## 🎯 Requirements

### Must Have (P2)

- [ ] **Extended deleteNode() Method**
  
  Add new parameter to control deletion behavior:
  ```cpp
  enum class NodeDeletionMode {
      CASCADE,  // Current behavior: delete all connected edges
      BRIDGE    // New behavior: create bridging edges
  };
  
  Status deleteNode(
      std::string_view pk,
      std::string_view graph_id = "default",
      NodeDeletionMode mode = NodeDeletionMode::CASCADE
  );
  ```

- [ ] **Edge Bridging Logic**
  
  For each node deletion with BRIDGE mode:
  1. Collect all incoming edges to the deleted node (sources)
  2. Collect all outgoing edges from the deleted node (targets)
  3. For each source-target pair, create a new edge if it doesn't exist
  4. Preserve edge properties (type, metadata) from the longest path
  5. Handle self-loops (node pointing to itself) gracefully
  6. Use atomic WriteBatch for all operations

- [ ] **Edge Property Handling**
  
  When creating bridging edges:
  - Preserve edge type from the longest path
  - Merge or combine edge metadata (configurable strategy)
  - Add `_bridged: true` flag to indicate synthetic edge
  - Store `_bridged_from: <deleted_node_pk>` for audit trail

- [ ] **Duplicate Edge Prevention**
  
  - Check if bridging edge already exists before creating
  - If exists, optionally merge metadata
  - Handle multi-edges (multiple edges between same nodes)

### Should Have (P3)

- [ ] **Configurable Bridging Strategies**
  
  ```cpp
  enum class BridgingStrategy {
      DIRECT,           // Create direct edges (A→B→C becomes A→C)
      PRESERVE_PATHS,   // Preserve all paths (multiple hops)
      SHORTEST_PATH,    // Only create shortest path bridges
      TYPED_ONLY        // Only bridge edges of same type
  };
  ```

- [ ] **Metadata Merge Policies**
  
  ```cpp
  enum class MetadataMergePolicy {
      KEEP_FIRST,       // Use metadata from incoming edge
      KEEP_LAST,        // Use metadata from outgoing edge
      MERGE,            // Combine metadata fields
      CUSTOM            // User-defined merge function
  };
  ```

- [ ] **Bulk Operations Support**
  
  ```cpp
  Status deleteNodesBatch(
      const std::vector<std::string>& pks,
      std::string_view graph_id = "default",
      NodeDeletionMode mode = NodeDeletionMode::CASCADE
  );
  ```

### Could Have (P4)

- [ ] **Conditional Bridging**
  
  - Bridge only if path length <= N
  - Bridge only for specific edge types
  - Bridge based on node labels or properties

- [ ] **Analytics and Reporting**
  
  - Count of bridged edges created
  - Report on removed vs bridged paths
  - Graph connectivity metrics before/after

---

## 🔧 Implementation Details

### Core Algorithm (BRIDGE Mode)

```cpp
Status PropertyGraphManager::deleteNode(
    std::string_view pk,
    std::string_view graph_id,
    NodeDeletionMode mode
) {
    // ... existing validation ...
    
    if (mode == NodeDeletionMode::BRIDGE) {
        // Step 1: Collect incoming edges (predecessors)
        std::vector<EdgeInfo> incomingEdges;
        collectIncomingEdges(pk, graph_id, incomingEdges);
        
        // Step 2: Collect outgoing edges (successors)
        std::vector<EdgeInfo> outgoingEdges;
        collectOutgoingEdges(pk, graph_id, outgoingEdges);
        
        // Step 3: Create bridging edges
        for (const auto& inEdge : incomingEdges) {
            for (const auto& outEdge : outgoingEdges) {
                // Skip self-loops
                if (inEdge.fromPk == outEdge.toPk) continue;
                
                // Check if edge already exists
                if (edgeExists(inEdge.fromPk, outEdge.toPk, graph_id)) continue;
                
                // Create bridging edge
                createBridgingEdge(inEdge, outEdge, pk, graph_id, batch);
            }
        }
    }
    
    // Continue with cascade deletion (edges already processed)
    // ... existing deletion logic ...
}
```

### Edge Creation

```cpp
void createBridgingEdge(
    const EdgeInfo& inEdge,
    const EdgeInfo& outEdge,
    std::string_view deletedNodePk,
    std::string_view graph_id,
    WriteBatchWrapper& batch
) {
    // Generate new edge ID
    std::string newEdgeId = generateBridgeEdgeId(
        inEdge.edgeId, outEdge.edgeId
    );
    
    // Create edge entity
    BaseEntity bridgeEdge(newEdgeId);
    bridgeEdge.setField("_from", inEdge.fromPk);
    bridgeEdge.setField("_to", outEdge.toPk);
    bridgeEdge.setField("_type", selectEdgeType(inEdge.type, outEdge.type));
    bridgeEdge.setField("_bridged", true);
    bridgeEdge.setField("_bridged_from", std::string(deletedNodePk));
    bridgeEdge.setField("_created_at", currentTimestamp());
    
    // Merge metadata
    mergeEdgeMetadata(bridgeEdge, inEdge, outEdge);
    
    // Add to batch
    addEdge(bridgeEdge, graph_id, batch);
}
```

---

## 🧪 Test Cases

### Unit Tests

```cpp
TEST_F(PropertyGraphTest, DeleteNode_BridgeMode_SimpleChain) {
    // A → B → C
    createNode("A"); createNode("B"); createNode("C");
    createEdge("A", "B", "e1"); createEdge("B", "C", "e2");
    
    deleteNode("B", "default", NodeDeletionMode::BRIDGE);
    
    // Verify A → C edge exists
    EXPECT_TRUE(edgeExists("A", "C"));
    // Verify bridged flag
    auto edge = getEdge("A", "C");
    EXPECT_TRUE(edge.getField("_bridged").value_or(false));
}

TEST_F(PropertyGraphTest, DeleteNode_BridgeMode_Diamond) {
    // A → B → D
    //   ↘ C ↗
    createNode("A"); createNode("B"); createNode("C"); createNode("D");
    createEdge("A", "B"); createEdge("A", "C");
    createEdge("B", "D"); createEdge("C", "D");
    
    deleteNode("B", "default", NodeDeletionMode::BRIDGE);
    
    // Verify A → D edge exists
    EXPECT_TRUE(edgeExists("A", "D"));
    // Verify C → D still exists
    EXPECT_TRUE(edgeExists("C", "D"));
}

TEST_F(PropertyGraphTest, DeleteNode_BridgeMode_SelfLoopPrevention) {
    // A → B → A (cycle)
    createNode("A"); createNode("B");
    createEdge("A", "B"); createEdge("B", "A");
    
    deleteNode("B", "default", NodeDeletionMode::BRIDGE);
    
    // Should not create A → A self-loop
    EXPECT_FALSE(edgeExists("A", "A"));
}

TEST_F(PropertyGraphTest, DeleteNode_BridgeMode_DuplicatePrevention) {
    // A → B → C, A → C already exists
    createNode("A"); createNode("B"); createNode("C");
    createEdge("A", "B"); createEdge("B", "C");
    createEdge("A", "C", "existing");
    
    deleteNode("B", "default", NodeDeletionMode::BRIDGE);
    
    // Should not create duplicate A → C
    auto edges = getEdgesBetween("A", "C");
    EXPECT_EQ(edges.size(), 1);
    EXPECT_EQ(edges[0].edgeId, "existing");
}
```

### Integration Tests

- Delete node in 3-level hierarchy
- Delete multiple nodes with bridging
- Bridge with different edge types
- Bridge with metadata preservation
- Performance: Bridge in graph with 1000+ nodes

---

## 📊 Performance Considerations

**Time Complexity:**
- Cascade mode: O(E_node) where E_node = edges connected to node
- Bridge mode: O(E_in × E_out) where E_in = incoming edges, E_out = outgoing edges
- Worst case: Node with 100 in + 100 out = 10,000 potential bridges

**Optimization Strategies:**
- Limit max bridging paths per node (configurable)
- Batch edge creation for better RocksDB performance
- Early termination on self-loops
- Use edge existence cache to prevent duplicate lookups

**Memory Impact:**
- Additional edges created = E_in × E_out (minus existing)
- Metadata storage overhead for bridging flags

---

## 🔒 Security & Data Integrity

- [ ] **Atomic Operations:** All bridging and deletions in single WriteBatch
- [ ] **Rollback on Failure:** If bridging fails, rollback node deletion
- [ ] **Audit Trail:** Log all bridged edges with deleted node reference
- [ ] **Permission Checks:** Verify user has rights to create bridging edges
- [ ] **Graph Validation:** Ensure no cycles or invalid states created

---

## 📚 Documentation Updates

- [ ] Update `include/index/property_graph.h` with new enums and parameters
- [ ] Add usage examples to documentation
- [ ] Update CHANGELOG.md with breaking changes (parameter signature)
- [ ] Add "Graph Operations" guide with bridging examples
- [ ] Update AQL syntax if exposing via query language

---

## 🔗 Related Issues

- Original cascade deletion PR: #XXX
- Graph traversal optimization: #XXX
- Temporal graph support: #XXX

---

## ✅ Acceptance Criteria

- [ ] `deleteNode()` supports `NodeDeletionMode::BRIDGE` parameter
- [ ] Bridging creates edges between all incoming and outgoing node pairs
- [ ] No self-loops created
- [ ] No duplicate edges created
- [ ] Edge metadata preserved with bridging flags
- [ ] All operations atomic within WriteBatch
- [ ] Unit tests for simple, diamond, and cycle scenarios
- [ ] Integration tests for large graphs
- [ ] Performance benchmarks show acceptable overhead
- [ ] Documentation updated

---

## 💬 Discussion

**Open Questions:**
1. Should bridging be the default behavior in future versions?
2. Should we support transitive bridging (multi-hop path preservation)?
3. How to handle edge weights/distances in bridging?
4. Should there be a limit on max bridging edges per operation?

**Alternative Approaches:**
- Soft delete: Mark node as deleted but keep edges
- Versioned graphs: Maintain history of topology changes
- External bridging service: Decouple from delete operation
