# Sprint 8 Batch D Phase 2-3: Complete Use-After-Move Remediation Plan

**Status:** Phase 2 KICKOFF  
**Date:** 2026-07-03 07:45 UTC  
**Phase 1 Result:** 8/8 gaps CLOSED ✅  
**Phase 2-3 Target:** 89 remaining gaps  
**User Preference:** Larger batches per commit (weiter/"next block")

---

## 🎯 Executive Summary

After successful Phase 1 completion (8 Priority 1 gaps), Sprint 8 Batch D enters Phase 2-3 to remediate **89 remaining use-after-move vulnerabilities (CWE-416)** organized into strategic groups:

### Gap Distribution
- **Priority 2 (Days 2-3):** ~20 gaps (SagaOrchestrator, Cross-shard transaction, Shard placement)
- **Priority 3 (Days 4-7):** ~22 gaps (Transaction coordinator, Partition coordinator, Model manager)
- **Unclassified (Days 8+):** ~47 gaps (architectural, complex refactors)
- **Total:** 89 gaps

### Success Criteria
- [ ] Phase 2: ≥80% Priority 2 gaps (16+ gaps) remediated
- [ ] Phase 3: ≥80% Priority 3 gaps (17+ gaps) remediated
- [ ] All 97 Batch D gaps closed by 2026-07-04 23:59 UTC
- [ ] Zero new gaps introduced
- [ ] 100% test pass rate
- [ ] Production-ready code

---

## 📊 Phase 2: Priority 2 Gaps (Days 2-3) - ~20 Gaps

### Group A: SagaOrchestrator Template Snapshot (Gap A-3 expansion)
**File:** `src/transaction/saga_orchestrator.cpp`  
**Gaps:** 8-12 related template storage scenarios  
**Complexity:** MEDIUM  
**Estimated Time:** 90 min total (8-10 min per gap)

#### Pattern Recognition
```cpp
// Pattern 1: Template moved to storage, original accessed
SagaTemplate tmpl = buildTemplate();
templates_[name] = std::move(tmpl);
tmpl.validate();  // ❌ UB

// Pattern 2: Template definition moved, metadata accessed
SAGADefinition def = loadDefinition();
definitions_.push_back(std::move(def));
def.getName();  // ❌ UB

// Pattern 3: Nested template move in construction
createAndStore(std::move(template_obj));
template_obj.status();  // ❌ UB
```

#### Remediation Strategy (3 sub-patterns)
1. **Pre-Move Snapshot** (5 gaps)
   - Capture template ID, name, version before move
   - Store snapshot in registry
   - Access via snapshot, never original

2. **Copy-Before-Store** (4 gaps)
   - Make local copy of template
   - Store copy with std::move for efficiency
   - Keep original for further use

3. **Handle-Based Access** (3 gaps)
   - Create TemplateHandle (ID + immutable metadata)
   - Move template body, access via handle
   - Handle survives move operation

#### Implementation Checklist
- [ ] Identify all SagaOrchestrator template move points (grep for std::move)
- [ ] Classify by sub-pattern
- [ ] Add pre-move snapshots
- [ ] Add tests for each sub-pattern
- [ ] Verify all template queries use snapshots/handles
- [ ] Integration test: full saga execution

---

### Group B: Cross-Shard Transaction Protocol (Gaps A-4-A-7)
**Files:**
- `src/sharding/cross_shard_transaction.cpp` (3-4 gaps)
- `src/transaction/transaction_executor.cpp` (2-3 gaps)

**Complexity:** HIGH  
**Estimated Time:** 120 min total (20-25 min per gap)

#### Pattern Recognition
```cpp
// Pattern 1: Transaction moved to prepare phase, accessed in commit
Transaction tx = buildTx();
preparePhase(std::move(tx));  // 2PC prepare
commitPhase(tx);  // ❌ UB - tx is moved

// Pattern 2: Coordinator moved through protocol phases
Coordinator coord = create();
phase1Result = runPhase1(std::move(coord));
phase2Result = runPhase2(coord);  // ❌ UB

// Pattern 3: WAL operation reference to moved object
auto record = createWALRecord(tx);
wal_.append(std::move(record));
tx.setWALPtr(record);  // ❌ UB
```

#### Remediation Strategy
1. **Transaction State Handle** (3 gaps)
   - Extract transaction ID + state before move
   - Create TransactionHandle(id, state, metadata)
   - Pass handle through protocol phases
   - Move only transaction body, not handle

2. **Protocol State Wrapper** (2-3 gaps)
   - Create ProtocolState struct (id, phase, status)
   - Snapshot state at each phase boundary
   - Move transaction between phases via state handle
   - Reconstruct transaction reference from state

3. **WAL Reference Fix** (1-2 gaps)
   - Store transaction ID in WAL record, not moved transaction
   - Lookup transaction by ID when needed
   - Eliminate moved reference access

#### Implementation Checklist
- [ ] Analyze cross_shard_transaction.cpp move points
- [ ] Design TransactionHandle + ProtocolState structs
- [ ] Update buildTransaction() to capture ID before move
- [ ] Update 2PC prepare/commit to accept handles
- [ ] Update WAL operations to use IDs
- [ ] Add integration tests for full 2PC flow
- [ ] Stress test with concurrent transactions

---

### Group C: Shard Placement State Capture (Gaps B-2-B-5)
**File:** `src/distributed/shard_placement.cpp`  
**Gaps:** 4-5 related placement decision scenarios  
**Complexity:** MEDIUM  
**Estimated Time:** 60 min total (12-15 min per gap)

#### Pattern Recognition
```cpp
// Pattern 1: Coordinator moved in placement decision, state re-accessed
Coordinator coord = buildCoordinator();
decisions_.push_back(std::move(coord));
coord.getPlacementDecision();  // ❌ UB

// Pattern 2: Shard state moved, then queried
ShardState state = analyzeLoad();
selectedShards_.push_back(std::move(state));
state.load();  // ❌ UB

// Pattern 3: Placement context moved through pipeline
PlacementContext ctx = createContext();
runOptimizer(std::move(ctx));
ctx.getResult();  // ❌ UB
```

#### Remediation Strategy
1. **Placement Decision Handle** (2 gaps)
   - Capture placement ID + decision vector before move
   - Store decision snapshot in map
   - Access via ID, never moved object

2. **Shard State Registry** (2 gaps)
   - Create ShardStateHandle (shard_id, load, replica_count)
   - Move full ShardState to registry
   - Query via handle/ID always

3. **Context Result Capture** (1 gap)
   - Extract optimization result before move
   - Store result in shared registry
   - Move context only, access result by ID

#### Implementation Checklist
- [ ] Identify all shard_placement.cpp move points
- [ ] Create placement data handles
- [ ] Add pre-move state capture
- [ ] Update placement queries to use handles
- [ ] Add tests for placement accuracy
- [ ] Verify no placement decisions lost after move

---

## 📊 Phase 3: Priority 3 Gaps (Days 4-7) - ~22 Gaps

### Group D: Transaction Coordinator Suite (Gaps A-8-A-15)
**Files:**
- `src/transaction/distributed_transaction_manager.cpp` (2-3 gaps)
- `src/transaction/transaction_coordinator.cpp` (3-4 gaps)
- `src/transaction/two_phase_commit_coordinator.cpp` (2-3 gaps)

**Complexity:** HIGH  
**Estimated Time:** 180 min total (20-25 min per gap)

#### Patterns
- Coordinator moved through async execution pipeline
- Coordinator state needed after move for monitoring
- Multiple coordinators in flight, moved without tracking

#### Remediation Strategy
1. **Coordinator ID Registry** (4 gaps)
   - All coordinator state keyed by ID
   - Move coordinator to async queue by ID
   - Monitor/query via ID, never moved object

2. **Coordinator Snapshot** (3 gaps)
   - Snapshot coordinator state (participants, phase, status)
   - Move coordinator, keep snapshot in registry
   - All monitoring uses snapshot

3. **Weak References** (1 gap)
   - Use weak_ptr to coordinator
   - Check validity before access
   - Guard against use-after-move with validity check

#### Implementation Checklist
- [ ] Analyze all coordinator move patterns
- [ ] Design CoordinatorHandle (ID + immutable state)
- [ ] Refactor coordinator storage to use ID registry
- [ ] Update async executor to track coordinator ID
- [ ] Add comprehensive monitoring tests
- [ ] Stress test: 1000+ concurrent coordinators

---

### Group E: Partition Coordinator Suite (Gaps B-6-B-12)
**Files:**
- `src/distributed/partition_coordinator.cpp` (4-5 gaps)
- `src/distributed/partition_manager.cpp` (2-3 gaps)

**Complexity:** HIGH  
**Estimated Time:** 150 min total (18-22 min per gap)

#### Patterns
- Partition coordinator moved through topology changes
- Partition state queried after move
- Multiple partitions being rebalanced simultaneously

#### Remediation Strategy
1. **Partition Handle** (3 gaps)
   - Create PartitionHandle (partition_id, replica_set, state)
   - Move partition coordinator via handle
   - Topology queries use handle

2. **Rebalancing State Snapshot** (2-3 gaps)
   - Snapshot partition state before rebalancing move
   - Store in rebalancing registry
   - Monitor via registry, never moved object

3. **Topology Consistency** (1-2 gaps)
   - Atomic partition state updates
   - Version-based consistency
   - Guard move operations with version checks

#### Implementation Checklist
- [ ] Analyze partition_coordinator.cpp move patterns
- [ ] Design PartitionHandle struct
- [ ] Implement rebalancing state registry
- [ ] Update topology manager to use handles
- [ ] Add partition consistency tests
- [ ] Rebalancing stress test: multi-node topology changes

---

### Group F: Model Manager Ownership Model (Gaps C-2-C-8)
**Files:**
- `src/rag/model_manager.cpp` (3-4 gaps)
- `src/rag/llm_integration.cpp` (2-3 gaps)
- `src/rag/prompt_adapter.cpp` (1-2 gaps)

**Complexity:** MEDIUM-HIGH  
**Estimated Time:** 120 min total (15-18 min per gap)

#### Patterns
- Model moved between pipeline stages
- Multiple stages need model access
- Model state (weights cache, stats) needs consistency

#### Remediation Strategy
1. **Unified Model Registry** (3 gaps)
   - All models stored in central registry keyed by model_id
   - Pipeline stages access via ID, not moved reference
   - Automatic cleanup when last reference drops

2. **Shared Ownership Completion** (2 gaps)
   - Convert unique_ptr to shared_ptr for models
   - Ensure all pipeline stages use shared_ptr
   - Eliminate move semantics, use reference counting

3. **Model State Snapshot** (2 gaps)
   - For inference results, capture immediately
   - Store in result object, not moved model
   - Model can be moved/unloaded without affecting results

#### Implementation Checklist
- [ ] Analyze model_manager.cpp move patterns
- [ ] Design ModelHandle (id, config, metadata)
- [ ] Convert to shared_ptr for all models
- [ ] Update pipeline stages to use registry
- [ ] Add model lifecycle tests
- [ ] Inference accuracy tests (weights unchanged after move)

---

## 📊 Phase 3+: Unclassified Gaps (~47 Gaps)

**Strategy:** Post-Phase 2-3 analysis
- Run AST scanner on entire codebase after Phase 2-3
- Identify architectural patterns
- Batch remaining gaps into coherent groups
- Estimated 2-3 additional priority groups

---

## 🧪 Testing Strategy (All Phases)

### Unit Tests: Per-Gap Verification
```cpp
// Example: SagaOrchestrator template move
TEST(MoveSemantics, SagaTemplateMovedState) {
  SagaOrchestrator orchestrator;
  
  // Capture ID before move
  auto tmpl_id = template.getId();
  
  // Add and move
  orchestrator.addTemplate(std::move(template));
  
  // Verify access via ID
  auto retrieved = orchestrator.getTemplate(tmpl_id);
  EXPECT_TRUE(retrieved.isValid());
  EXPECT_EQ(retrieved.getId(), tmpl_id);
}
```

### Integration Tests: Phase-Boundary Verification
- Full transaction lifecycle (build → prepare → commit → log)
- Multi-shard transaction with coordinator moves
- Model pipeline with stage transitions
- Partition rebalancing with topology changes

### Regression Tests
- All existing transaction tests pass
- All existing distributed tests pass
- All existing RAG tests pass
- Performance baseline maintained

### Stress Tests
- 1000+ concurrent transactions
- 100+ shard placement operations
- 50+ model pipeline inference cycles
- Partition rebalancing with 10+ nodes

---

## 📈 Execution Timeline

| Phase | Duration | Target Gaps | Estimated Completion |
|-------|----------|-------------|----------------------|
| **Phase 2** | ~4-6 hours | 16+ Priority 2 | 2026-07-03 12:00 UTC |
| **Phase 3** | ~8-10 hours | 17+ Priority 3 | 2026-07-03 22:00 UTC |
| **Unclassified** | ~12 hours | 30+ remaining | 2026-07-04 12:00 UTC |
| **Integration** | ~4 hours | All 97 | 2026-07-04 23:59 UTC |
| **TOTAL** | **28 hours** | **97 gaps** | **2026-07-04 23:59 UTC** |

---

## ✅ Delivery Checklist

### Per-Gap Checklist (Repeat for each gap)
- [ ] Identify exact move pattern (pre-grep analysis)
- [ ] Classify remediation type
- [ ] Implement fix (snapshot/handle/registry)
- [ ] Add unit test
- [ ] Run affected module tests
- [ ] Verify no new gaps introduced
- [ ] Code review approval
- [ ] Merge to develop

### Phase 2 Completion Checklist
- [ ] All Group A gaps (SagaOrchestrator): 8-12 gaps
- [ ] All Group B gaps (Cross-shard): 4-5 gaps
- [ ] All Group C gaps (Shard placement): 4-5 gaps
- [ ] Phase 2 integration test passing
- [ ] No performance regression
- [ ] Update progress: 8 + ~20 = 28 gaps closed (28% total)

### Phase 3 Completion Checklist
- [ ] All Group D gaps (Transaction coordinator): 8+ gaps
- [ ] All Group E gaps (Partition coordinator): 7+ gaps
- [ ] All Group F gaps (Model manager): 7+ gaps
- [ ] Phase 3 integration test passing
- [ ] All existing tests still passing
- [ ] Update progress: 28 + 22 = 50 gaps closed (50% total)

### Final Delivery (All Phases)
- [ ] All 97 Batch D gaps remediated
- [ ] Comprehensive test suite (50+ tests)
- [ ] Documentation updated
- [ ] Performance verified (no regression)
- [ ] Ready for v1.5.0 release

---

## 🚀 Next Immediate Actions

### Right Now (Hour 1)
1. **Phase 2 Analysis** (30 min)
   - Extract exact move patterns for Groups A, B, C
   - Verify line numbers and context
   - Confirm remediation strategy with leads

2. **Test Infrastructure** (30 min)
   - Set up Phase 2 test suite
   - Create test templates for each pattern
   - Ensure build environment ready

### Hour 2-6 (Phase 2 Execution)
1. **Group A: SagaOrchestrator** (90 min)
   - Implement 8-12 template snapshot fixes
   - Add unit tests
   - Commit batch

2. **Group B: Cross-Shard** (120 min)
   - Implement TransactionHandle pattern
   - Update 2PC protocol
   - Add integration tests
   - Commit batch

3. **Group C: Shard Placement** (60 min)
   - Implement PlacementHandle pattern
   - Add placement decision tests
   - Commit batch

---

## 📞 Support & Escalation

**Pattern Questions:** Refer to `SPRINT_8_BATCH_D_GAP_ANALYSIS.md`  
**Tool Issues:** Check `.github/instructions/cpp-best-practices.instructions.md`  
**Architectural:** Reference `ARCHITECTURE.md` (Target Architecture)

---

**Status:** READY FOR PHASE 2 EXECUTION ✅  
**Created:** 2026-07-03 07:45 UTC  
**Owner:** Core Infrastructure Team  
**Reviewer:** Architecture lead  
**Timeline:** 2026-07-03 to 2026-07-04 23:59 UTC
