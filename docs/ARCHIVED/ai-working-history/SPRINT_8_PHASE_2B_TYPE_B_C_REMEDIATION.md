# Sprint 8 Phase 2B: Type B/C Remediation Implementation Plan

**Date:** 2026-07-05  
**Phase:** Type B/C Violation Remediation (Constructor Issues & Complex Scenarios)  
**Target Gaps:** 47-50 gaps across 8 modules  
**Timeline:** Week 2-3 (2026-07-19 to 2026-07-25)

---

## Type B: Move Constructor/Assignment Issues (30-35 gaps)

### Issue Pattern: Members Not Moved or Source Not Cleared

### 1. Sharding Module - Transaction Coordinator (10 gaps)

#### Gap B1: CrossShardTransactionCoordinator State

**File:** `src/sharding/cross_shard_transaction.cpp`

**Issue:** Coordinator state (write_set_, participants_, WAL entries) not fully moved in constructor.

**Remediation:**
```cpp
class CrossShardTransactionCoordinator {
 private:
  std::vector<WriteOperation> write_set_;
  std::set<ShardId> participants_;
  TransactionWALEntry wal_entry_;
  std::unique_ptr<CoordinatorState> state_;

 public:
  // BUG: Only write_set moved, not participants_
  // BEFORE:
  // CrossShardTransactionCoordinator(CrossShardTransactionCoordinator&& other)
  //     : write_set_(std::move(other.write_set_)),
  //       wal_entry_(std::move(other.wal_entry_)) {}

  // AFTER:
  CrossShardTransactionCoordinator(CrossShardTransactionCoordinator&& other) noexcept
      : write_set_(std::move(other.write_set_)),
        participants_(std::move(other.participants_)),
        wal_entry_(std::move(other.wal_entry_)),
        state_(std::move(other.state_)) {
    // Ensure source is cleared
    other.participants_.clear();
    other.state_ = nullptr;
    
    // Validate post-move state
    THEMIS_VALIDATE_MOVE(other);
  }

  CrossShardTransactionCoordinator& operator=(CrossShardTransactionCoordinator&& other) noexcept {
    if (this != &other) {
      write_set_ = std::move(other.write_set_);
      participants_ = std::move(other.participants_);
      wal_entry_ = std::move(other.wal_entry_);
      state_ = std::move(other.state_);
      
      other.participants_.clear();
      other.state_ = nullptr;
      THEMIS_VALIDATE_MOVE(other);
    }
    return *this;
  }

  CrossShardTransactionCoordinator(const CrossShardTransactionCoordinator&) = delete;
  CrossShardTransactionCoordinator& operator=(const CrossShardTransactionCoordinator&) = delete;
};
```

#### Gap B2-B10: Participant & WAL Moves (9 gaps)

**Files:**
- `src/sharding/two_phase_commit_participant.cpp` (3 gaps)
- `src/sharding/transaction_wal.cpp` (2 gaps)
- `src/sharding/transaction_snapshot.cpp` (2 gaps)
- `src/sharding/shard_rpc_client.cpp` (2 gaps)

**Pattern:** Ensure all members (status flags, timestamps, prepared_modifications_, etc.) are moved.

---

### 2. Replication Module - WAL & Slot (7 gaps)

#### Gap B11: ReplicationManager WAL Context

**File:** `src/replication/replication_manager.cpp`

**Issue:** WAL context holds reader/writer state but only some members moved.

**Remediation:**
```cpp
struct WALContext {
  std::unique_ptr<WALReader> reader;
  std::unique_ptr<WALWriter> writer;
  std::string wal_directory;
  std::atomic<uint64_t> current_lsn{0};
  std::vector<WALEntry> pending_entries;

  // BUG: Only reader/writer moved, not pending_entries
  WALContext(WALContext&& other) noexcept
      : reader(std::move(other.reader)),
        writer(std::move(other.writer)),
        wal_directory(std::move(other.wal_directory)),
        current_lsn(other.current_lsn.load()),
        pending_entries(std::move(other.pending_entries)) {
    // Ensure source cleanup
    other.pending_entries.clear();
    
    // Validate
    THEMIS_VALIDATE_MOVE(other);
  }

  WALContext& operator=(WALContext&& other) noexcept {
    if (this != &other) {
      reader = std::move(other.reader);
      writer = std::move(other.writer);
      wal_directory = std::move(other.wal_directory);
      current_lsn.store(other.current_lsn.load());
      pending_entries = std::move(other.pending_entries);
      
      other.pending_entries.clear();
      THEMIS_VALIDATE_MOVE(other);
    }
    return *this;
  }

  WALContext(const WALContext&) = delete;
  WALContext& operator=(const WALContext&) = delete;
};
```

#### Gap B12-B17: Logical Replication & Slot Moves (6 gaps)

**Files:**
- `src/replication/logical_replication.cpp` (3 gaps)
- `src/replication/replication_slot.cpp` (2 gaps)
- `src/replication/raft_v2.cpp` (1 gap)

---

### 3. Network Module - Message & Connection (6 gaps)

#### Gap B18: MessageBuffer Move Issue

**File:** `include/network/message_buffer.h`

**Issue:** Buffer holds data and metadata; metadata not cleared after move.

**Remediation:**
```cpp
class MessageBuffer {
 private:
  std::vector<uint8_t> payload_;
  MessageType type_ = MessageType::UNKNOWN;
  uint32_t sequence_id_ = 0;
  std::chrono::system_clock::time_point created_at_;

 public:
  MessageBuffer(MessageBuffer&& other) noexcept
      : payload_(std::move(other.payload_)),
        type_(other.type_),
        sequence_id_(other.sequence_id_),
        created_at_(std::move(other.created_at_)) {
    // IMPORTANT: Clear source metadata
    other.type_ = MessageType::UNKNOWN;
    other.sequence_id_ = 0;
    
    THEMIS_VALIDATE_MOVE(other);
  }

  MessageBuffer& operator=(MessageBuffer&& other) noexcept {
    if (this != &other) {
      payload_ = std::move(other.payload_);
      type_ = other.type_;
      sequence_id_ = other.sequence_id_;
      created_at_ = std::move(other.created_at_);
      
      other.type_ = MessageType::UNKNOWN;
      other.sequence_id_ = 0;
      THEMIS_VALIDATE_MOVE(other);
    }
    return *this;
  }

  MessageBuffer(const MessageBuffer&) = delete;
  MessageBuffer& operator=(const MessageBuffer&) = delete;
};
```

#### Gap B19-B23: Connection & Protocol Moves (5 gaps)

**Files:**
- `src/network/connection_context.cpp` (2 gaps)
- `src/network/protocol_state_machine.cpp` (2 gaps)
- `src/network/socket_pool.cpp` (1 gap)

---

### 4. Distributed Knowledge - Graph Structures (4 gaps)

#### Gap B24: KnowledgeGraphNode Move

**File:** `src/distributed_knowledge/knowledge_graph.cpp`

**Issue:** Node properties not moved in move constructor.

**Remediation:**
```cpp
struct KnowledgeGraphNode {
  std::string node_id;
  std::map<std::string, std::string> properties;
  std::vector<std::string> edge_ids;

  KnowledgeGraphNode(KnowledgeGraphNode&& other) noexcept
      : node_id(std::move(other.node_id)),
        properties(std::move(other.properties)),
        edge_ids(std::move(other.edge_ids)) {
    THEMIS_VALIDATE_MOVE(other);
  }

  KnowledgeGraphNode& operator=(KnowledgeGraphNode&& other) noexcept {
    if (this != &other) {
      node_id = std::move(other.node_id);
      properties = std::move(other.properties);
      edge_ids = std::move(other.edge_ids);
      THEMIS_VALIDATE_MOVE(other);
    }
    return *this;
  }
};
```

#### Gap B25-B27: Edge & Schema Moves (3 gaps)

---

## Type C: Complex Move Scenarios (15-20 gaps)

### Issue Pattern: Polymorphic Types, Template Instantiations, Move Chains

### 1. LLM Module - Polymorphic Adapter Moves (3 gaps)

#### Gap C1: LLMAdapter Hierarchy

**File:** `include/llm/llm_adapter.h`

**Issue:** Virtual move semantics not properly enforced in polymorphic hierarchy.

**Remediation:**
```cpp
class LLMAdapter {
 protected:
  std::string adapter_id_;
  std::unique_ptr<AdapterConfig> config_;
  
 public:
  // Virtual move operations for polymorphic safety
  virtual ~LLMAdapter() = default;
  
  virtual std::unique_ptr<LLMAdapter> clone() const = 0;
  
  // Explicit move support
  LLMAdapter(LLMAdapter&& other) noexcept
      : adapter_id_(std::move(other.adapter_id_)),
        config_(std::move(other.config_)) {}
  
  LLMAdapter& operator=(LLMAdapter&& other) noexcept {
    if (this != &other) {
      adapter_id_ = std::move(other.adapter_id_);
      config_ = std::move(other.config_);
    }
    return *this;
  }

 private:
  LLMAdapter(const LLMAdapter&) = delete;
  LLMAdapter& operator=(const LLMAdapter&) = delete;
};

class OpenAIAdapter : public LLMAdapter {
 private:
  std::string api_key_;
  std::unique_ptr<APIClient> client_;
  
 public:
  OpenAIAdapter(OpenAIAdapter&& other) noexcept
      : LLMAdapter(std::move(other)),
        api_key_(std::move(other.api_key_)),
        client_(std::move(other.client_)) {}

  OpenAIAdapter& operator=(OpenAIAdapter&& other) noexcept {
    LLMAdapter::operator=(std::move(other));
    api_key_ = std::move(other.api_key_);
    client_ = std::move(other.client_);
    return *this;
  }

  std::unique_ptr<LLMAdapter> clone() const override {
    return std::make_unique<OpenAIAdapter>(*this);
  }

 private:
  OpenAIAdapter(const OpenAIAdapter&) = delete;
  OpenAIAdapter& operator=(const OpenAIAdapter&) = delete;
};
```

#### Gap C2-C3: Anthropic & Other Adapter Moves (2 gaps)

---

### 2. Graph Module - Node/Edge Move Chains (3 gaps)

#### Gap C4: GraphNode Complex Move

**File:** `src/graph/graph_node.cpp`

**Issue:** Node contains edges, properties, and type constraints; moves may leak state.

**Remediation:**
```cpp
class GraphNode {
 private:
  NodeId node_id_;
  std::vector<EdgeId> outgoing_edges_;
  std::vector<EdgeId> incoming_edges_;
  NodeProperties properties_;
  std::unique_ptr<TypeConstraint> constraint_;
  MoveChainTracker move_tracker_;

 public:
  GraphNode(GraphNode&& other) noexcept
      : node_id_(other.node_id_),
        outgoing_edges_(std::move(other.outgoing_edges_)),
        incoming_edges_(std::move(other.incoming_edges_)),
        properties_(std::move(other.properties_)),
        constraint_(std::move(other.constraint_)) {
    
    move_tracker_.onMoveBegin();
    try {
      other.outgoing_edges_.clear();
      other.incoming_edges_.clear();
      move_tracker_.onMoveEnd();
    } catch (...) {
      move_tracker_.onMoveEnd();
      throw;
    }
  }

  GraphNode& operator=(GraphNode&& other) noexcept {
    if (this != &other) {
      move_tracker_.onMoveBegin();
      try {
        node_id_ = other.node_id_;
        outgoing_edges_ = std::move(other.outgoing_edges_);
        incoming_edges_ = std::move(other.incoming_edges_);
        properties_ = std::move(other.properties_);
        constraint_ = std::move(other.constraint_);
        
        other.outgoing_edges_.clear();
        other.incoming_edges_.clear();
        move_tracker_.onMoveEnd();
      } catch (...) {
        move_tracker_.onMoveEnd();
        throw;
      }
    }
    return *this;
  }

  GraphNode(const GraphNode&) = delete;
  GraphNode& operator=(const GraphNode&) = delete;
};
```

#### Gap C5-C6: Edge & Traversal Moves (2 gaps)

---

### 3. Server Module - Connection Context Moves (3 gaps)

#### Gap C7: ConnectionContext Move Chain

**File:** `src/server/connection_context.cpp`

**Issue:** Complex connection state with handler registrations; cleanup needed on move.

**Remediation:**
```cpp
class ConnectionContext {
 private:
  ConnectionId conn_id_;
  std::unique_ptr<SocketWrapper> socket_;
  std::map<EventType, std::vector<EventHandler>> handlers_;
  AuthenticationInfo auth_info_;
  std::deque<Message> pending_messages_;

 public:
  ConnectionContext(ConnectionContext&& other) noexcept
      : conn_id_(other.conn_id_),
        socket_(std::move(other.socket_)),
        handlers_(std::move(other.handlers_)),
        auth_info_(std::move(other.auth_info_)),
        pending_messages_(std::move(other.pending_messages_)) {
    
    // Validate move completed
    assert(handlers_.empty() || other.handlers_.empty());
    other.pending_messages_.clear();
  }

  ConnectionContext& operator=(ConnectionContext&& other) noexcept {
    if (this != &other) {
      conn_id_ = other.conn_id_;
      socket_ = std::move(other.socket_);
      handlers_ = std::move(other.handlers_);
      auth_info_ = std::move(other.auth_info_);
      pending_messages_ = std::move(other.pending_messages_);
      
      other.pending_messages_.clear();
    }
    return *this;
  }

  ConnectionContext(const ConnectionContext&) = delete;
  ConnectionContext& operator=(const ConnectionContext&) = delete;
};
```

#### Gap C8-C9: Session & Protocol Handler Moves (2 gaps)

---

### 4. Temporal Module - Time-Series Moves (2 gaps)

#### Gap C10: TimeSeries Move

**File:** `src/temporal/time_series.cpp`

**Issue:** Time-series data with indices; index not moved correctly.

**Remediation:**
```cpp
class TimeSeries {
 private:
  std::vector<TimePoint> timestamps_;
  std::vector<double> values_;
  std::unique_ptr<TimeIndex> index_;

 public:
  TimeSeries(TimeSeries&& other) noexcept
      : timestamps_(std::move(other.timestamps_)),
        values_(std::move(other.values_)),
        index_(std::move(other.index_)) {
    THEMIS_VALIDATE_MOVE(other);
  }

  TimeSeries& operator=(TimeSeries&& other) noexcept {
    if (this != &other) {
      timestamps_ = std::move(other.timestamps_);
      values_ = std::move(other.values_);
      index_ = std::move(other.index_);
      THEMIS_VALIDATE_MOVE(other);
    }
    return *this;
  }

  TimeSeries(const TimeSeries&) = delete;
  TimeSeries& operator=(const TimeSeries&) = delete;
};
```

#### Gap C11: TimeIndex Move (1 gap)

---

### 5. Miscellaneous Modules - Edge Cases (9+ gaps)

**Pattern:** Apply same remediation to:
- analytics: AnalyticsContext moves (2 gaps)
- chimera: ChimeraState moves (1 gap)
- governance: PolicyContext moves (1 gap)
- importers: ImportSession moves (1 gap)
- performance: PerformanceContext moves (1 gap)
- process: ProcessContext moves (1 gap)
- scraper: ScraperSession moves (1 gap)
- scheduler: ScheduleTask moves (1 gap)
- Others: Remaining edge cases (1 gap)

---

## Implementation Checklist: Phase 2B

### Type B Remediation (30-35 gaps):

#### Sharding (10 gaps)
- [ ] CrossShardTransactionCoordinator (all members)
- [ ] TwoPhaseCommitParticipant (state machine)
- [ ] TransactionWAL entries (prepared_, committed_)
- [ ] TransactionSnapshot (recovery state)
- [ ] ShardRpcClient (pending requests)

#### Replication (7 gaps)
- [ ] WALContext (pending_entries)
- [ ] ReplicationManager (wal_context)
- [ ] LogicalReplication (slot state)
- [ ] ReplicationSlot (position tracking)
- [ ] Raft membership (joint consensus)

#### Network (6 gaps)
- [ ] MessageBuffer (type_, sequence_id_)
- [ ] ConnectionContext (metadata)
- [ ] ProtocolStateMachine (state flags)
- [ ] SocketPool (handles)

#### Distributed Knowledge (4 gaps)
- [ ] KnowledgeGraphNode (all members)
- [ ] KnowledgeGraphEdge (all members)
- [ ] GraphSchema (type definitions)

### Type C Remediation (15-20 gaps):

#### LLM (3 gaps)
- [ ] LLMAdapter polymorphic moves
- [ ] OpenAIAdapter (derived)
- [ ] AnthropicAdapter (derived)

#### Graph (3 gaps)
- [ ] GraphNode move chains
- [ ] GraphEdge moves
- [ ] Traversal context

#### Server (3 gaps)
- [ ] ConnectionContext
- [ ] SessionContext
- [ ] ProtocolHandler

#### Temporal (2 gaps)
- [ ] TimeSeries moves
- [ ] TimeIndex moves

#### Miscellaneous (9+ gaps)
- [ ] Analytics contexts
- [ ] Chimera state
- [ ] Governance policies
- [ ] Importer sessions
- [ ] Performance tracking
- [ ] Process contexts
- [ ] Scraper sessions
- [ ] Scheduler tasks
- [ ] Other edge cases

---

## Testing & Validation

### Test Structure:

```cpp
TEST_SUITE(TypeBRemediationTests) {
  TEST(MoveConstruction_<Component>) { ... }
  TEST(MoveAssignment_<Component>) { ... }
  TEST(AllMembersMoved_<Component>) { ... }
  TEST(SourceClearedAfterMove_<Component>) { ... }
  TEST(ExceptionSafety_<Component>) { ... }
}

TEST_SUITE(TypeCComplexMoveTests) {
  TEST(PolymorphicMove_<Adapter>) { ... }
  TEST(MoveChain_<Path>) { ... }
  TEST(CrossModuleMoveChain_<Path>) { ... }
}
```

---

## Effort & Timeline

| Component | Gaps | Est. Hours | Target |
|-----------|------|-----------|--------|
| Phase 2B Type B | 30-35 | 15-20 | 2026-07-22 |
| Phase 2B Type C | 15-20 | 10-15 | 2026-07-25 |
| **Total Phase 2B** | **47-50** | **25-35** | **2026-07-25** |

---

## Next Phase

- Phase 3: Complete Testing & Integration (Week 3)
- Phase 4: Documentation & Merge Preparation
- Phase 5: Release to develop (Target: 2026-07-26)
