---
name: ✨ Feature: Graph Undo/Redo Operations
about: Add transactional undo/redo support for graph modifications
title: "[GRAPH] Undo/Redo Transaction History"
labels: priority:P3, type:enhancement, area:graph, effort:large, phase:feature
assignees: ''
---

## ✨ Feature Enhancement - Graph Transactions

**Current Status:** No undo/redo support, proposal phase  
**Priority:** P3 (Nice to have)  
**Effort:** 3-4 weeks  
**Target Version:** v1.6.0  
**Related Files:**
- `include/index/property_graph.h`
- `include/transaction/transaction_manager.h`
- `src/index/property_graph.cpp`

---

## 📋 Problem Description

When users perform graph operations (adding/deleting nodes and edges), there's no built-in mechanism to:
- **Undo** recent modifications
- **Redo** previously undone operations
- **Audit** historical changes
- **Restore** to a previous graph state

This is problematic for:
- **Interactive graph editing tools** (visual graph editors)
- **Exploratory data analysis** (trial-and-error modifications)
- **Error recovery** (accidental deletions)
- **Collaborative editing** (multiple users working on same graph)

---

## 🎯 Requirements

### Must Have (P3)

- [ ] **Transaction History Tracking**
  
  ```cpp
  class GraphTransactionHistory {
  public:
      // Record operation for undo/redo
      void recordOperation(const GraphOperation& op);
      
      // Undo last N operations
      Status undo(size_t count = 1);
      
      // Redo last N undone operations
      Status redo(size_t count = 1);
      
      // Get operation history
      std::vector<GraphOperation> getHistory(size_t limit = 100);
      
      // Clear history
      void clear();
  };
  ```

- [ ] **GraphOperation Types**
  
  ```cpp
  enum class GraphOperationType {
      ADD_NODE,
      DELETE_NODE,
      UPDATE_NODE,
      ADD_EDGE,
      DELETE_EDGE,
      UPDATE_EDGE,
      ADD_LABEL,
      REMOVE_LABEL
  };
  
  struct GraphOperation {
      GraphOperationType type;
      std::string target_id;      // Node or edge ID
      std::string graph_id;
      nlohmann::json before_state; // For undo
      nlohmann::json after_state;  // For redo
      std::chrono::system_clock::time_point timestamp;
      std::string user_id;         // Who made the change
  };
  ```

- [ ] **Integration with PropertyGraphManager**
  
  ```cpp
  class PropertyGraphManager {
      // Enable/disable history tracking
      void enableHistoryTracking(bool enable = true);
      
      // Get transaction history
      std::shared_ptr<GraphTransactionHistory> getHistory();
      
      // Execute with history tracking
      Status addNodeWithHistory(const BaseEntity& node, ...);
      Status deleteNodeWithHistory(std::string_view pk, ...);
      // ... other operations ...
  };
  ```

### Should Have (P4)

- [ ] **Branching History**
  - Support multiple undo/redo branches
  - Allow switching between history branches
  - Merge history branches

- [ ] **Selective Undo**
  ```cpp
  // Undo specific operation by ID
  Status undoOperation(const std::string& operation_id);
  
  // Undo all operations by user
  Status undoUserOperations(const std::string& user_id);
  ```

- [ ] **Persistent History**
  - Save history to RocksDB
  - Load history on startup
  - Configurable history retention (time/count)

### Could Have (P5)

- [ ] **Visual History Browser**
  - Timeline view of operations
  - Graph diff visualization
  - Operation replay

- [ ] **Collaborative Features**
  - Conflict detection
  - Merge strategies
  - Operation broadcast to other clients

---

## 🔧 Implementation Details

### Operation Recording

```cpp
Status PropertyGraphManager::deleteNode(
    std::string_view pk,
    std::string_view graph_id,
    NodeDeletionMode mode
) {
    // Record before state for undo
    if (history_enabled_) {
        GraphOperation op;
        op.type = GraphOperationType::DELETE_NODE;
        op.target_id = std::string(pk);
        op.graph_id = std::string(graph_id);
        op.before_state = captureNodeState(pk, graph_id);
        op.timestamp = std::chrono::system_clock::now();
        
        // Perform deletion
        auto status = deleteNodeInternal(pk, graph_id, mode);
        
        if (status.ok) {
            op.after_state = nullptr;  // Node deleted
            history_->recordOperation(op);
        }
        
        return status;
    }
    
    return deleteNodeInternal(pk, graph_id, mode);
}
```

### Undo Implementation

```cpp
Status GraphTransactionHistory::undo(size_t count) {
    if (undo_stack_.size() < count) {
        return Status::Error("Not enough operations to undo");
    }
    
    auto batch = db_.createWriteBatch();
    
    for (size_t i = 0; i < count; i++) {
        auto op = undo_stack_.back();
        undo_stack_.pop_back();
        
        // Reverse the operation
        switch (op.type) {
            case GraphOperationType::DELETE_NODE:
                // Restore deleted node
                restoreNodeFromState(op.before_state, *batch);
                break;
            case GraphOperationType::ADD_NODE:
                // Delete added node
                deleteNodeInternal(op.target_id, op.graph_id, *batch);
                break;
            // ... other cases ...
        }
        
        // Move to redo stack
        redo_stack_.push_back(op);
    }
    
    return batch->commit() ? Status::OK() : Status::Error("Undo failed");
}
```

---

## 🧪 Test Cases

```cpp
TEST_F(GraphHistoryTest, UndoDeleteNode) {
    pgm_->enableHistoryTracking(true);
    
    // Add and delete node
    BaseEntity node("A");
    node.setField("name", "Alice");
    pgm_->addNodeWithHistory(node);
    pgm_->deleteNodeWithHistory("A");
    
    // Undo deletion
    auto history = pgm_->getHistory();
    ASSERT_TRUE(history->undo(1).ok);
    
    // Verify node restored
    auto restored = pgm_->getNode("A");
    ASSERT_TRUE(restored.has_value());
    EXPECT_EQ(restored->getFieldAsString("name"), "Alice");
}

TEST_F(GraphHistoryTest, RedoAddNode) {
    pgm_->enableHistoryTracking(true);
    
    BaseEntity node("B");
    pgm_->addNodeWithHistory(node);
    
    auto history = pgm_->getHistory();
    history->undo(1);
    EXPECT_FALSE(pgm_->nodeExists("B"));
    
    history->redo(1);
    EXPECT_TRUE(pgm_->nodeExists("B"));
}

TEST_F(GraphHistoryTest, UndoComplexOperation) {
    pgm_->enableHistoryTracking(true);
    
    // Create graph: A → B → C
    pgm_->addNodeWithHistory(createNode("A"));
    pgm_->addNodeWithHistory(createNode("B"));
    pgm_->addNodeWithHistory(createNode("C"));
    pgm_->addEdgeWithHistory(createEdge("A", "B", "e1"));
    pgm_->addEdgeWithHistory(createEdge("B", "C", "e2"));
    
    // Delete B with bridging
    pgm_->deleteNodeWithHistory("B", "default", NodeDeletionMode::BRIDGE);
    
    // Verify A → C bridge created
    EXPECT_TRUE(pgm_->edgeExists("A", "C"));
    
    // Undo deletion - should restore B and remove bridge
    auto history = pgm_->getHistory();
    history->undo(1);
    
    EXPECT_TRUE(pgm_->nodeExists("B"));
    EXPECT_TRUE(pgm_->edgeExists("A", "B"));
    EXPECT_TRUE(pgm_->edgeExists("B", "C"));
    EXPECT_FALSE(pgm_->edgeExists("A", "C"));  // Bridge removed
}
```

---

## 📊 Performance Considerations

**Memory Overhead:**
- Each operation stores before/after state (JSON)
- History limit: default 1000 operations
- Configurable retention: time-based or count-based

**Storage Overhead:**
- Optional persistent history in RocksDB
- Compression of historical states
- Incremental diffs vs full snapshots

**Performance Impact:**
- ~5-10% overhead when history tracking enabled
- Negligible when disabled (default)

---

## 🔒 Security & Compliance

- [ ] Audit trail for all graph modifications
- [ ] User attribution for operations
- [ ] Compliance with data retention policies
- [ ] Secure deletion of history (GDPR right to be forgotten)

---

## 📚 Documentation Updates

- [ ] User guide for undo/redo operations
- [ ] API reference for GraphTransactionHistory
- [ ] Performance tuning guide
- [ ] Security and compliance considerations

---

## ✅ Acceptance Criteria

- [ ] Undo/redo for all graph operations
- [ ] History tracking can be enabled/disabled
- [ ] Operations can be undone in reverse order
- [ ] Redone operations restore exact state
- [ ] Memory usage stays within limits
- [ ] Performance overhead <10%
- [ ] Unit tests for all operation types
- [ ] Integration tests for complex scenarios
- [ ] Documentation complete
