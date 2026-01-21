---
name: ⚡ Performance: Graph Bulk Operations
about: Add batch operations for efficient graph manipulation
title: "[GRAPH] Bulk Operations for Nodes and Edges"
labels: priority:P2, type:performance, area:graph, effort:medium, phase:optimization
assignees: ''
---

## ⚡ Performance Enhancement - Graph Operations

**Current Status:** Individual operations only, bulk APIs pending  
**Priority:** P2 (Medium)  
**Effort:** 2 weeks  
**Target Version:** v1.5.0  
**Related Files:**
- `include/index/property_graph.h`
- `src/index/property_graph.cpp`
- `tests/test_property_graph.cpp`

---

## 📋 Problem Description

Currently, `PropertyGraphManager` provides methods for single node/edge operations:
- `addNode()`, `deleteNode()`, `addEdge()`, `deleteEdge()`

While `addNodesBatch()` and `addEdgesBatch()` exist for additions, there are no bulk deletion or update operations. This leads to:
- **Performance bottlenecks** when deleting multiple nodes
- **Excessive RocksDB transactions** (N operations = N WriteBatches)
- **Reduced throughput** for batch graph updates
- **Poor scalability** for large graph modifications

**Performance Impact:** ~100x slower for bulk operations vs optimized batch processing.

---

## 🎯 Requirements

### Must Have (P2)

- [ ] **Bulk Node Operations**
  
  ```cpp
  // Delete multiple nodes atomically
  Status deleteNodesBatch(
      const std::vector<std::string>& pks,
      std::string_view graph_id = "default",
      NodeDeletionMode mode = NodeDeletionMode::CASCADE
  );
  
  // Update multiple node properties
  Status updateNodesBatch(
      const std::vector<std::pair<std::string, BaseEntity>>& updates,
      std::string_view graph_id = "default"
  );
  ```

- [ ] **Bulk Edge Operations**
  
  ```cpp
  // Delete multiple edges atomically
  Status deleteEdgesBatch(
      const std::vector<std::string>& edgeIds,
      std::string_view graph_id = "default"
  );
  
  // Update multiple edge properties
  Status updateEdgesBatch(
      const std::vector<std::pair<std::string, BaseEntity>>& updates,
      std::string_view graph_id = "default"
  );
  ```

- [ ] **Bulk Label Operations**
  
  ```cpp
  // Add label to multiple nodes
  Status addLabelBatch(
      const std::vector<std::string>& pks,
      std::string_view label,
      std::string_view graph_id = "default"
  );
  
  // Remove label from multiple nodes
  Status removeLabelBatch(
      const std::vector<std::string>& pks,
      std::string_view label,
      std::string_view graph_id = "default"
  );
  ```

### Should Have (P3)

- [ ] **Conditional Bulk Operations**
  
  ```cpp
  // Delete nodes matching predicate
  Status deleteNodesWhere(
      std::function<bool(const BaseEntity&)> predicate,
      std::string_view graph_id = "default"
  );
  
  // Update edges matching predicate
  Status updateEdgesWhere(
      std::function<bool(const BaseEntity&)> predicate,
      std::function<void(BaseEntity&)> updater,
      std::string_view graph_id = "default"
  );
  ```

- [ ] **Transaction Statistics**
  
  ```cpp
  struct BulkOperationStats {
      size_t nodes_affected;
      size_t edges_affected;
      size_t labels_modified;
      size_t indices_updated;
      std::chrono::milliseconds duration;
  };
  
  std::pair<Status, BulkOperationStats> deleteNodesBatchWithStats(...);
  ```

### Could Have (P4)

- [ ] **Parallel Execution**
  - Split bulk operations into chunks
  - Process chunks in parallel threads
  - Merge results atomically

- [ ] **Progress Callbacks**
  ```cpp
  using ProgressCallback = std::function<void(size_t completed, size_t total)>;
  
  Status deleteNodesBatch(
      const std::vector<std::string>& pks,
      std::string_view graph_id,
      NodeDeletionMode mode,
      ProgressCallback callback
  );
  ```

---

## 🔧 Implementation Details

### Bulk Deletion Algorithm

```cpp
Status PropertyGraphManager::deleteNodesBatch(
    const std::vector<std::string>& pks,
    std::string_view graph_id,
    NodeDeletionMode mode
) {
    if (pks.empty()) return Status::OK();
    
    auto batch = db_.createWriteBatch();
    if (!batch) {
        return Status::Error("Could not create write batch");
    }
    
    // Collect all edges to delete/bridge
    std::unordered_map<std::string, NodeEdges> nodeEdgeMap;
    for (const auto& pk : pks) {
        collectNodeEdges(pk, graph_id, nodeEdgeMap[pk]);
    }
    
    // Process bridging if needed
    if (mode == NodeDeletionMode::BRIDGE) {
        for (const auto& [pk, edges] : nodeEdgeMap) {
            createBridgingEdges(edges.incoming, edges.outgoing, pk, graph_id, *batch);
        }
    }
    
    // Delete all nodes and their edges atomically
    for (const auto& pk : pks) {
        deleteNodeInternal(pk, graph_id, *batch);
    }
    
    if (!batch->commit()) {
        return Status::Error("Batch commit failed");
    }
    
    return Status::OK();
}
```

---

## 🧪 Test Cases

```cpp
TEST_F(PropertyGraphTest, DeleteNodesBatch_Cascade) {
    // Create 100 nodes with edges
    for (int i = 0; i < 100; i++) {
        createNode("node" + std::to_string(i));
        if (i > 0) createEdge("node" + std::to_string(i-1), 
                              "node" + std::to_string(i));
    }
    
    // Delete first 50 nodes
    std::vector<std::string> toDelete;
    for (int i = 0; i < 50; i++) {
        toDelete.push_back("node" + std::to_string(i));
    }
    
    auto st = pgm_->deleteNodesBatch(toDelete);
    ASSERT_TRUE(st.ok);
    
    // Verify deletions
    for (int i = 0; i < 50; i++) {
        EXPECT_FALSE(nodeExists("node" + std::to_string(i)));
    }
    for (int i = 50; i < 100; i++) {
        EXPECT_TRUE(nodeExists("node" + std::to_string(i)));
    }
}

TEST_F(PropertyGraphTest, BulkOperations_Performance) {
    // Benchmark: 1000 individual vs 1 batch
    auto start = std::chrono::high_resolution_clock::now();
    
    for (int i = 0; i < 1000; i++) {
        pgm_->deleteNode("node" + std::to_string(i));
    }
    
    auto individual_time = std::chrono::high_resolution_clock::now() - start;
    
    // ... batch test ...
    
    EXPECT_LT(batch_time.count(), individual_time.count() / 10);  // 10x faster
}
```

---

## 📊 Performance Targets

- **Throughput:** 10,000+ nodes/sec for bulk deletion
- **Speedup:** 50-100x vs individual operations
- **Memory:** O(N) space for N operations (acceptable for batch)
- **Latency:** <1ms per operation in batch mode

---

## 📚 Documentation Updates

- [ ] API reference for bulk operations
- [ ] Performance comparison guide
- [ ] Migration guide from individual to bulk APIs
- [ ] Best practices for large graph updates

---

## ✅ Acceptance Criteria

- [ ] All bulk operations use single WriteBatch
- [ ] Performance benchmarks show >50x speedup
- [ ] Atomic commit/rollback for all operations
- [ ] Unit tests for success and failure cases
- [ ] Integration tests with 10,000+ nodes
- [ ] Documentation complete
