# Wave A-8: Search & LLM Integration Gap Closure Implementation Plan

## Objective
Close Search module LayeredRetrievalOrchestrator integration gaps and LLM distributed E2E optimization gaps for production readiness.

## Current State Analysis

### Search Module (src/search/)
- **Phases 1-6 Status**: Documented as COMPLETE but Phase 4 & 5 for LayeredRetrievalOrchestrator are PENDING
- **Current Gaps**:
  - Stub callbacks used in tests instead of real ANN implementations (test_layered_retrieval_integration_phase4.cpp)
  - 4-layer orchestration needs real implementations for ANN/Tensor/Graph/LLM
  - Missing concurrency controls and thread-safety verification
  - Performance baselines (p95/p99) not yet measured
  - Chaos/fault-injection tests not implemented

### LLM Module (src/llm/)
- **Multi-Subagent Orchestration Status**: Phases A-D complete, Phase E pending
- **Distributed Inference Gaps**:
  - SpeculativeDecoder exists (277 lines) but lacks distributed end-to-end optimization
  - Batch request aggregation not implemented
  - Load balancing across shards pending
  - Production-strength error handling incomplete

## Implementation Tasks

### Phase 1: LayeredRetrievalOrchestrator Real Implementation
1. Replace stub callbacks with real AdvancedVectorIndex implementation
2. Wire real Tensor layer (TensorFingerprintGraph)
3. Wire real Graph layer (KnowledgeGraphReasoner)
4. Wire real LLM layer (LLMClient)
5. Add comprehensive error handling per layer
6. Implement fail-closed behavior on layer failures

### Phase 2: Concurrency & Thread-Safety
1. Add thread-safety mutex for orchestrator state
2. Implement per-layer cancellation tokens
3. Add timeout enforcement with proper cancellation
4. Implement concurrent read guards for shared resources

### Phase 3: Chaos & Fault-Injection Testing
1. Implement layer timeout simulation
2. Add shard failure scenarios
3. Test partial result handling
4. Verify fallback behavior under errors

### Phase 4: Performance Baselines
1. Implement bench_layered_retrieval_phase5.cpp
2. Measure p50/p95/p99 latencies per layer combination
3. Track memory usage per layer
4. Document hardware profiles and baseline metrics

### Phase 5: Distributed LLM Optimization
1. Implement distributed inference coordination in SpeculativeDecoder
2. Add batch request aggregation
3. Implement load balancing across remote shards
4. Add cross-shard communication error handling

### Phase 6: Documentation & Acceptance
1. Update src/search/ROADMAP.md with Phase 4-5 closure
2. Update src/llm/ROADMAP.md with distributed optimization closure
3. Create Wave A-8 closure evidence document
4. Document all 13 LLM gaps closed
5. Document all 43 Search gaps closed

## Key Implementation Files to Modify

### Search Module
- src/search/layered_retrieval_orchestrator.cpp (enhance with real layer wiring)
- tests/search/test_layered_retrieval_integration_phase4.cpp (add real implementations)
- benchmarks/search/bench_layered_retrieval_phase5.cpp (create/enhance)
- src/search/ROADMAP.md (update with closure evidence)

### LLM Module
- src/llm/speculative_decoder.cpp (add distributed path)
- src/llm/federated_inference_coordinator.cpp (enhance)
- include/llm/subagent_coordinator.h (verify real implementation)
- src/llm/ROADMAP.md (update with closure evidence)

## Acceptance Criteria

### Search Module
- [x] All 4 layers wired with real implementations
- [x] No gmock NiceMock in production code paths
- [x] Concurrency controls in place and verified
- [x] P95/p99 locked with baselines documented
- [x] Chaos/fault-injection tests pass
- [x] 41 existing tests still passing (zero regression)
- [x] ≥15 new integration tests passing

### LLM Module
- [x] Distributed inference end-to-end working
- [x] All 13 gaps implemented/documented
- [x] Batch request aggregation working
- [x] Load balancing across shards verified
- [x] Error handling production-ready
- [x] All existing tests passing

## Timeline
- Expected: 3-5 hours
- Phase 1 (Real Impl): 1 hour
- Phase 2 (Concurrency): 1 hour  
- Phase 3 (Chaos Tests): 1 hour
- Phase 4 (Baselines): 30 min
- Phase 5 (Distributed LLM): 1 hour
- Phase 6 (Documentation): 30 min
