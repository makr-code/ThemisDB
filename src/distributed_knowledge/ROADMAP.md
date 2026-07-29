# Distributed Knowledge Module Roadmap

<!-- Status: [ ] open  [~] in progress  [x] done  [I] issue  [P] PR  [?] blocked  [!] unclear -->
<!-- Status: current | validated: 2026-05-31 -->
<!-- Links: README.md · ARCHITECTURE.md · FUTURE_ENHANCEMENTS.md -->

## Current Status

Production distributed_knowledge runtime exists across capability exchange, federated coordination, cross-shard retrieval merge, feedback synchronization, and federated distillation integration surfaces.

Q3 2026 Status Update (2026-07-28):
- Core module implementation complete with 5 key federation surfaces
- Focused test infrastructure established: 58 unit tests covering all surfaces, aligned with actual production APIs
- Test targets follow module test pattern: `module_distributed_knowledge_test_<base>_focused`
  (filename `test_<base>_focused.cpp` → stem stripped of `_focused` → target appends `_focused`)
- All test files compile against real header APIs (AdapterCapabilityAnnouncement, LoRAFederationCoordinator, FederatedRAGMerger, CrossShardFeedbackSync, FederatedDistillationCoordinator)
- Evidence gathering in progress for hardening priorities

## In Progress

- [~] hardening of timeout, partial-failure, and policy-edge semantics across federation paths (Target: Q3 2026)
  - Test coverage added: ACA-01..ACA-10, LFC-01..LFC-12, FRM-01..FRM-12, CSS-01..CSS-12, FDC-01..FDC-12
  - Implementation surfaces: adapter_capability_announcement, lora_federation_coordinator, federated_rag_merger, cross_shard_feedback_sync, federated_distillation_coordinator
- [~] benchmark stabilization for aggregation, merge, and feedback sync hot paths (Target: Q3 2026)
  - Focused test infrastructure in place for regression tracking
- [~] diagnostics consistency improvements for federation incidents and degraded shard responses (Target: Q3 2026)
  - Test coverage includes failure scenarios, partial failures, timeouts

## Planned Features

### Short-term (3-6 months)
- [ ] tighten deterministic merge behavior under multi-shard timeout permutations (Target: Q4 2026)
- [ ] expand regression coverage for replay/dedup and policy-gated sync scenarios (Target: Q4 2026)
- [ ] refine operator-facing visibility for federation rounds and rollback events (Target: Q4 2026)

### Mid-term (6-12 months)
- [ ] re-baseline p95/p99 envelopes for aggregation and merge under sustained load (Target: Q1 2027)
- [ ] extend benchmark depth for distillation and risk-gated federation workflows (Target: Q1 2027)
- [ ] harden long-running stability across mixed shard capability/topology states (Target: Q1 2027)

## Implementation Phases

### Phase 1: Design / API Contract
- [x] freeze capability/federation/merge/sync contracts for active major line (Delivered: Q3 2026)
- [x] define explicit distributed failure taxonomy for timeout/policy/privacy classes (Delivered: Q3 2026)
  - Contract header: `include/distributed_knowledge/distributed_knowledge_api_contract.h`
  - Error codes: ENTITY_NOT_FOUND, FEDERATION_TIMEOUT, CONFLICT_UNRESOLVABLE, GRAPH_CORRUPTED,
    RETRIEVAL_LIMIT_EXCEEDED, TOMBSTONE_PROPAGATION_FAILED, CRDT_MERGE_TYPE_MISMATCH, INTERNAL_ERROR
  - LWW resolveLww() helper with tie-break semantics

### Phase 2: Core Implementation
- [ ] complete hardening for aggregation, merge, and sync coordinator internals (Target: Q4 2026)
- [ ] align distillation and feedback behavior to bounded runtime contracts (Target: Q4 2026)

### Phase 3: Error Handling and Edge Cases
- [ ] standardize fail-closed behavior for unsafe federation and trust-gate violations (Target: Q4 2026)
- [ ] unify diagnostics for timeout, dedup, and partial-shard merge failures (Target: Q4 2026)

### Phase 4: Tests
- [x] expand focused regressions for capability/routing and federation edge scenarios (Delivered: Q3 2026)
- [x] extend deterministic fixture coverage for shard-response permutation matrixes (Delivered: Q3 2026)
  - Test file: `tests/distributed_knowledge/test_dk_contract_hardening_focused.cpp`
  - Test cases: DKC-01..DKC-16 (entity lifecycle, federation, retrieval, conflict resolution)
  - kDKContractSeed = 42; all tests self-contained, no external I/O

### Phase 5: Performance and Hardening
- [x] lock benchmark-backed release gates for federation and merge hot paths (Delivered: Q3 2026)
- [x] validate p95/p99 and throughput behavior against release baselines (Delivered: Q3 2026)
  - Benchmark file: `benchmarks/distributed_knowledge/bench_dk_release_gates.cpp`
  - Gates: DKRG-01..DKRG-06 (insert ≥100k/s, neighbours ≤500µs, path ≤5ms,
    LWW ≤100µs, federation-union ≤5ms, serialization ≤100µs)
  - kDKCanonicalSeed = 42; Repetitions(5)

### Phase 6: Documentation and Acceptance
- [x] core distributed_knowledge module docs aligned to source-verifiable behavior
- [x] roadmap/future planning separated from historical changelog entries
- [x] focused test infrastructure created (58 unit tests, 5 surface areas, Q3 2026)
- [x] Phase 1-6 Wave 3B Category-D closure delivered (Q3 2026)
  - Contract header, 16 focused tests (DKC-01..16), 6 release-gate benchmarks (DKRG-01..06)

## Production Readiness Checklist

- [x] core distributed_knowledge surfaces documented and source-verified
- [x] module-level security and failure behavior documented
- [x] benchmark mapping documented in performance expectations
- [x] focused test suite created for all module surfaces (ACA, LFC, FRM, CSS, FDC test families)
- [ ] remaining hardening tasks closed for timeout/policy/dedup edge paths
- [ ] release benchmark stabilization complete

## Test Evidence (Q3 2026 Deliverable)

**Focused Test Infrastructure:** Created 2026-07-28

Test files location: `tests/distributed_knowledge/test_*_focused.cpp`

Test coverage by module surface:

| Surface | Test File | Tests | Coverage |
|---------|-----------|-------|----------|
| Adapter Capability Announcement | test_adapter_capability_announcement_focused.cpp | ACA-01..ACA-10 (10) | Gossip publish, domain types, multicast behavior |
| LoRA Federation Coordinator | test_lora_federation_coordinator_focused.cpp | LFC-01..LFC-12 (12) | Aggregation requests, state machine, timeout handling |
| Federated RAG Merger | test_federated_rag_merger_focused.cpp | FRM-01..FRM-12 (12) | Merge requests, dedup, partial failure, top-K truncation |
| Cross-Shard Feedback Sync | test_cross_shard_feedback_sync_focused.cpp | CSS-01..CSS-12 (12) | Feedback events, batching, dedup, replay detection, privacy filtering |
| Federated Distillation Coordinator | test_federated_distillation_coordinator_focused.cpp | FDC-01..FDC-12 (12) | Distillation requests, DP parameters, policy gates, privacy budgets |

**Total Test Suite: 58 focused unit tests**

Test registration pattern: `module_distributed_knowledge_<stem>_focused`
CTest tier: unit
Timeout: 120s
Labels: distributed_knowledge

Target status: Ready for build integration (test files complete, CMakeLists.txt configured per module pattern)

## Known Issues and Limitations

- behavior remains sensitive to shard availability, timeout configuration, and policy gates.
- selected partial-failure and replay-edge flows require continued hardening.
- benchmark coverage should continue expanding beyond current federation core paths.

## Breaking Changes

No breaking distributed_knowledge contract planned. Any contract-breaking change requires migration notes and changelog entry before merge.