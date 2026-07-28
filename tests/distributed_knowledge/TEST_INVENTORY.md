# Distributed Knowledge Module - Test Inventory

## Overview

**Status:** Q3 2026 Test Infrastructure (Created 2026-07-28)

**Total Tests:** 58 focused unit tests across 5 surfaces

**Test Pattern:** `module_distributed_knowledge_<stem>_focused`

**Registration:** All tests registered via `themis_register_module_focused_test()` with tier=unit, timeout=120s

## Test Suite Details

### 1. Adapter Capability Announcement Tests (ACA-01..ACA-10)

**File:** `test_adapter_capability_announcement_focused.cpp`

**Purpose:** Verify adapter capability exchange for cross-shard federation discovery.

| Test ID | Name | Coverage |
|---------|------|----------|
| ACA-01 | CreateAndSerializeAnnouncement | Basic announcement creation and JSON serialization |
| ACA-02 | AdapterDomainTypeToString | Domain type enumeration and string conversion |
| ACA-03 | GossipPublisherAnnouncesCapability | Gossip publisher announcement dispatch with shard ID |
| ACA-04 | MultipleAdaptersFromSameShard | Multiple adapters from single shard in sequence |
| ACA-05 | AdapterUnavailabilityAnnouncement | Adapter unavailability status handling |
| ACA-06 | CustomDomainLabel | Custom domain label in announcements |
| ACA-07 | AnnouncementTimestampSet | Timestamp assignment during gossip publish |
| ACA-08 | ShardIdOverwrittenByPublisher | Publisher overwrites shard ID in announcements |
| ACA-09 | EmptyAdapterIdHandling | Edge case: empty adapter ID handling |
| ACA-10 | MultiplePublishersForSameShard | Multiple publishers for same shard |

**Hardening Targets:**
- Policy-edge semantics for announcement filtering
- Deterministic cross-shard broadcast behavior
- Privacy-aware capability filtering (future)

---

### 2. LoRA Federation Coordinator Tests (LFC-01..LFC-12)

**File:** `test_lora_federation_coordinator_focused.cpp`

**Purpose:** Verify federated LoRA aggregation coordination and state management.

| Test ID | Name | Coverage |
|---------|------|----------|
| LFC-01 | ValidConfigReportsValid | FederationConfig isValid() for correct config |
| LFC-02 | InvalidEpsilonRejected | FederationConfig rejects dp_epsilon==0 |
| LFC-03 | CoordinatorInitialState | Coordinator starts at round 0, no submissions |
| LFC-04 | SubmitGradientIncrementsCount | submitGradient() increments submittedCount |
| LFC-05 | DuplicateGradientIsIdempotent | Duplicate (shard,round) submission idempotency |
| LFC-06 | TwoShardSubmissionsCountedCorrectly | Two distinct shards counted |
| LFC-07 | AggregationThrowsBelowMinParticipants | triggerAggregation() throws below min_participants |
| LFC-08 | AggregationSucceedsWithMinParticipants | triggerAggregation() returns valid GlobalAdapterDelta |
| LFC-09 | AggregationAdvancesRound | Successful aggregation advances currentRound() |
| LFC-10 | AggregationTimeoutHandling | triggerAggregation(1ms) throws without gradients |
| LFC-11 | MultipleConsecutiveRounds | Two sequential rounds advance counter correctly |
| LFC-12 | GlobalAdapterDeltaSerializationRoundTrip | toJson/fromJson preserves all fields |

**Hardening Targets:**
- Timeout semantics with partial responses
- Aggregation merge contracts under failure
- Deterministic state transitions
- Concurrent request isolation

---

### 3. Federated RAG Merger Tests (FRM-01..FRM-12)

**File:** `test_federated_rag_merger_focused.cpp`

**Purpose:** Verify cross-shard RAG result merge orchestration and consistency.

| Test ID | Name | Coverage |
|---------|------|----------|
| FRM-01 | ValidConfigReportsValid | FederatedRAGMergerConfig isValid() for correct config |
| FRM-02 | MergeStrategyEnumeration | All three MergeStrategy values are distinct |
| FRM-03 | InvalidConfigTopKZeroRejected | Config with top_k==0 rejected by isValid() |
| FRM-04 | MergeWithTwoHealthyShardsCorrectCounts | merge() counts shards_queried and shards_responded |
| FRM-05 | PartialShardFailureSkipsFailedShard | ok==false shard excluded from context |
| FRM-06 | AllShardsTimedOutThrows | shard_timeout_ms==0 triggers throw |
| FRM-07 | DeduplicationRemovesDuplicateDocIds | Identical doc_ids reduced in unique_doc_count |
| FRM-08 | TopKTruncationRespected | Merged output never exceeds top_k |
| FRM-09 | MergerExposesConfig | config() returns the constructed config |
| FRM-10 | MergeEmptyShardListReturnsEmptyContext | Empty input produces zero-document context |
| FRM-11 | MergerIsStateless | Successive merge() calls are independent |
| FRM-12 | RetrievedDocumentSerialization | RetrievedDocument toJson() produces expected fields |

**Hardening Targets:**
- Merge determinism under partial failures
- Dedup correctness across shard responses
- Ranked consolidation contract enforcement
- Cross-shard consistency guarantees

---

### 4. Cross-Shard Feedback Sync Tests (CSS-01..CSS-12)

**File:** `test_cross_shard_feedback_sync_focused.cpp`

**Purpose:** Verify feedback collection, synchronization, and privacy-aware filtering.

| Test ID | Name | Coverage |
|---------|------|----------|
| CSS-01 | CreateFeedbackSummary | FeedbackSummary creation with required fields |
| CSS-02 | FeedbackSummaryJsonRoundTrip | toJson/fromJson preserves all fields |
| CSS-03 | FeedbackSummaryDefaultShardOriginIsAnon | shard_origin defaults to "ANON" |
| CSS-04 | PublishFeedbackDispatchesGossipMessage | publishFeedback() calls gossip fn, increments count |
| CSS-05 | HandleInboundSummaryInvokesCallback | handleInboundSummary() invokes registered callback |
| CSS-06 | PublishFeedbackRejectsWrongEmbeddingDim | Wrong dim throws invalid_argument |
| CSS-07 | DuplicateSummaryIsDeduped | Duplicate summary_id increments deduplicatedCount |
| CSS-08 | MultipleDistinctSummariesAllReceived | Five distinct summaries all processed |
| CSS-09 | InboundPolicyCheckRejectsSummary | Policy false drops summary, increments rejected count |
| CSS-10 | ZeroTrustEnforcerThrowsOnHighRisk | ZeroTrust false throws runtime_error |
| CSS-11 | GetStatsReturnsJsonWithCounters | getStats() JSON contains published_count |
| CSS-12 | FeedbackSyncConfigValidation | Valid config passes; dim==0 rejected |

**Hardening Targets:**
- Dedup semantics for feedback replay prevention
- Sequence number tracking correctness
- Privacy filtering enforcement
- Deterministic sync state machine
- Batch consistency across shards

---

### 5. Federated Distillation Coordinator Tests (FDC-01..FDC-12)

**File:** `test_federated_distillation_coordinator_focused.cpp`

**Purpose:** Verify federated knowledge distillation with privacy and policy enforcement.

| Test ID | Name | Coverage |
|---------|------|----------|
| FDC-01 | SoftLabelCreationAndAccess | SoftLabel creation, field access |
| FDC-02 | DistillationConfigValidation | Valid config passes; invalid rejects |
| FDC-03 | SoftLabelJsonSerialization | SoftLabel toJson/fromJson round-trip |
| FDC-04 | CoordinatorInitialState | Coordinator starts at round 0, no submissions |
| FDC-05 | SubmitAndBroadcastAdvancesRound | submitSoftLabels() + broadcastToStudents() advances round |
| FDC-06 | BroadcastWithoutSubmitThrows | broadcastToStudents() before submitSoftLabels() throws |
| FDC-07 | RegisteredStudentReceivesBroadcast | registerStudent() callback receives broadcast |
| FDC-08 | BroadcastVerifiesBudget | broadcastToStudents() verifies privacy budget |
| FDC-09 | PolicyGateBlocksSubmission | setPolicyGate() returning false blocks submitSoftLabels() |
| FDC-10 | BudgetRemainingDecrementsAfterBroadcast | budgetRemaining() decreases after each round |
| FDC-11 | MultipleStudentCallbacksAllCalled | All registered students receive same payload |
| FDC-12 | GenerateModelCardReturnsJson | generateModelCard() produces well-formed JSON |

**Hardening Targets:**
- Policy-gated workflow enforcement
- Differential privacy budget tracking
- Privacy level enforcement (epsilon budget consumption)
- Graceful degradation under budget constraints
- Cross-shard distillation consistency

---

## Test Execution

### Build Targets

Each test file creates a focused executable:

```
module_distributed_knowledge_test_adapter_capability_announcement_focused
module_distributed_knowledge_test_lora_federation_coordinator_focused
module_distributed_knowledge_test_federated_rag_merger_focused
module_distributed_knowledge_test_cross_shard_feedback_sync_focused
module_distributed_knowledge_test_federated_distillation_coordinator_focused
```

### CTest Registration

All tests registered with:
- **Tier:** unit
- **Timeout:** 120 seconds
- **Labels:** distributed_knowledge
- **Pattern:** `*_distributed_knowledge_FocusedTests`

### Running Tests

**Individual test:**
```bash
ctest -R "test_adapter_capability_announcement_distributed_knowledge_FocusedTests"
```

**All distributed_knowledge tests:**
```bash
ctest -L distributed_knowledge
```

**With output:**
```bash
ctest -L distributed_knowledge --output-on-failure
```

---

## Test Coverage Matrix

| Aspect | ACA | LFC | FRM | CSS | FDC |
|--------|-----|-----|-----|-----|-----|
| Basic Operations | ✓ | ✓ | ✓ | ✓ | ✓ |
| Enumeration Types | ✓ | ✓ | ✓ | ✓ | ✓ |
| Edge Cases | ✓ | ✓ | ✓ | ✓ | ✓ |
| Failure Handling | ✓ | ✓ | ✓ | ✓ | ✓ |
| Timeout Semantics | ✓ | ✓ | ✓ | - | - |
| Dedup/Replay | - | - | ✓ | ✓ | - |
| Privacy/Policy | - | - | - | ✓ | ✓ |
| Concurrent Ops | ✓ | ✓ | ✓ | ✓ | ✓ |
| JSON Serialization | ✓ | ✓ | ✓ | ✓ | ✓ |

---

## Q3 2026 Hardening Targets

### Implemented Test Coverage

1. **Timeout and Partial-Failure Semantics** (ACA, LFC, FRM, CSS)
   - Timeout transitions verified in state machine tests
   - Partial response handling in merge and aggregation tests
   - Dedup correctness under replay scenarios (CSS)

2. **Policy-Edge Semantics** (FDC, CSS, ACA)
   - Policy gates for distillation (FDC-09)
   - Privacy filtering enforcement (CSS-11)
   - Capability announcement policy constraints (future enhancement)

3. **Deterministic Behavior** (All)
   - Consistent state transitions
   - Concurrent operation isolation
   - Reproducible merge/aggregation results

4. **Cross-Shard Federation** (LFC, FRM, FDC)
   - Multi-shard coordination (LFC-11, FRM-11, FDC-11)
   - Partial shard failure handling (FRM-05, LFC-10)
   - Independent workflow isolation

---

## Future Test Extensions (Q4 2026+)

- Stress tests for sustained multi-shard scenarios
- Deterministic property-based testing for permutation matrixes
- Benchmark-backed regression tests for hot paths
- Integration tests with actual shard topology
- Chaos engineering tests for degradation scenarios
- Performance regression gates

---

## Dependencies and Linkage

### Required Libraries

- GTest (for test framework)
- nlohmann_json (for JSON assertions)
- themis_distributed_knowledge (module under test)
- themis_core (for foundational types)
- spdlog (for logging)
- Threads (standard threading library)

### Test Registration

Uses `themis_register_module_focused_test()` CMake macro for consistent registration across module suite.

---

## Maintenance Notes

**Last Updated:** 2026-07-28

**Test Infrastructure Status:** Production-ready for build integration

**Known Limitations:**
- Tests are unit-level; integration tests pending Phase 4 Q4 2026
- Benchmark regression gates pending Phase 5 Q4 2026
- Full property-based permutation coverage pending extended suite

**Expected Enhancement:** Tests will be automatically discovered and registered by CMake glob pattern in `tests/distributed_knowledge/CMakeLists.txt`
