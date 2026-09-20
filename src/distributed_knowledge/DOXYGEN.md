# DISTRIBUTED_KNOWLEDGE DOXYGEN

Author: ThemisDB Contributors
Created: 2026-09-20
Last Updated: 2026-09-20
Status: active

## Source
- This file is generated from module-scoped Doxygen XML.
- XML index: `C:\Projects\ThemisDB\.tmp\module_doxygen_global_e2e\distributed_knowledge\xml\index.xml`
- Warnings log: `C:\Projects\ThemisDB\.tmp\module_doxygen_global_e2e\distributed_knowledge\doxygen-warnings.log`

## Structure Metrics
- C/C++ Files (scanned): 24
- Compounds: 80
- Classes/Structs: 36
- Namespaces: 11
- File Compounds: 24

## Namespaces
- @054363232047240215020024356020113006022076014047
- @120177125073046366351021120052351177056151134270
- @231127267365321075332374372165027307043130073165
- @347374330310104234153131162260136217322105312256
- std::chrono_literals
- testing
- themis
- themis::bench
- themis::bench::dkrg
- themis::distributed_knowledge
- themis::distributed_knowledge::@130034145074237162130156227037167242006364274302

## Types
### Classes
- AdapterCapabilityAnnouncementTest
- CrossShardFeedbackSyncTest
- FederatedDistillationCoordinatorTest
- FederatedRAGMergerTest
- LoRAFederationCoordinatorTest
- RecordingListener
- ThrowingListener
- themis::distributed_knowledge::AlwaysPermitTrustPolicy
- themis::distributed_knowledge::CrossShardFeedbackSync
- themis::distributed_knowledge::DistributedKnowledgeDiagnosticEmitter
- themis::distributed_knowledge::FederatedDistillationCoordinator
- themis::distributed_knowledge::FederatedRAGMerger
- themis::distributed_knowledge::GossipAdapterPublisher
- themis::distributed_knowledge::IDKDiagnosticListener
- themis::distributed_knowledge::IFederatedDistillationCoordinator
- themis::distributed_knowledge::IFederationTrustPolicy
- themis::distributed_knowledge::ILoRAFederationCoordinator
- themis::distributed_knowledge::LoRAFederationCoordinator

### Structs
- themis::bench::dkrg::BenchEntity
- themis::distributed_knowledge::AdapterCapabilityAnnouncement
- themis::distributed_knowledge::DKDiagnosticEvent
- themis::distributed_knowledge::DistillationBoundedPolicy
- themis::distributed_knowledge::DistillationConfig
- themis::distributed_knowledge::DistillationModelCard
- themis::distributed_knowledge::DistillationRound
- themis::distributed_knowledge::EncryptedGradient
- themis::distributed_knowledge::FederatedRAGMergerConfig
- themis::distributed_knowledge::FederationConfig
- themis::distributed_knowledge::FeedbackSummary
- themis::distributed_knowledge::FeedbackSyncConfig
- themis::distributed_knowledge::GlobalAdapterDelta
- themis::distributed_knowledge::MergeHardeningPolicy
- themis::distributed_knowledge::MergedRAGContext
- themis::distributed_knowledge::RetrievedDocument
- themis::distributed_knowledge::ShardRetrievalResult
- themis::distributed_knowledge::SoftLabel

## Notes
- This is a derived artifact. Do not manually edit semantic content.
- Regenerate via scripts/generate_module_doxygen_xml.py --write-module-markdown.

## Detailed Function Documentation (Soll-Ist)
- Functions extracted: 315

### AdapterCapabilityAnnouncementTest

#### `void SetUp() override`
- Source: `tests/distributed_knowledge/test_adapter_capability_announcement_focused.cpp`:33
- Brief: n/a
- Parameters: none

#### `auto make_gossip_fn()`
- Source: `tests/distributed_knowledge/test_adapter_capability_announcement_focused.cpp`:39
- Brief: n/a
- Parameters: none

### CrossShardFeedbackSyncTest

#### `void SetUp() override`
- Source: `tests/distributed_knowledge/test_cross_shard_feedback_sync_focused.cpp`:34
- Brief: n/a
- Parameters: none

#### `FeedbackSummary makeSummary(const std::string &id, const std::string &type_label="USER_NEGATIVE")`
- Source: `tests/distributed_knowledge/test_cross_shard_feedback_sync_focused.cpp`:49
- Brief: Build a minimal FeedbackSummary with a 4-float embedding.
- Parameters:
  - `id` (const std::string &): n/a
  - `type_label` (const std::string &): n/a

#### `auto make_gossip_fn()`
- Source: `tests/distributed_knowledge/test_cross_shard_feedback_sync_focused.cpp`:44
- Brief: n/a
- Parameters: none

### FederatedDistillationCoordinatorTest

#### `void SetUp() override`
- Source: `tests/distributed_knowledge/test_federated_distillation_coordinator_focused.cpp`:34
- Brief: n/a
- Parameters: none

#### `std::vector< SoftLabel > makeLabels(size_t n, const std::string &teacher_id="t1")`
- Source: `tests/distributed_knowledge/test_federated_distillation_coordinator_focused.cpp`:48
- Brief: Build a minimal set of SoftLabels for testing.
- Parameters:
  - `n` (size_t): n/a
  - `teacher_id` (const std::string &): n/a

### FederatedRAGMergerTest

#### `void SetUp() override`
- Source: `tests/distributed_knowledge/test_federated_rag_merger_focused.cpp`:33
- Brief: n/a
- Parameters: none

#### `ShardRetrievalResult makeShardResult(const std::string &shard_id, size_t n_docs, bool ok=true)`
- Source: `tests/distributed_knowledge/test_federated_rag_merger_focused.cpp`:42
- Brief: Build a ShardRetrievalResult with n documents.
- Parameters:
  - `shard_id` (const std::string &): n/a
  - `n_docs` (size_t): n/a
  - `ok` (bool): n/a

### LoRAFederationCoordinatorTest

#### `void SetUp() override`
- Source: `tests/distributed_knowledge/test_lora_federation_coordinator_focused.cpp`:34
- Brief: n/a
- Parameters: none

#### `EncryptedGradient makeGradient(const std::string &shard_id, uint64_t round, size_t samples=100)`
- Source: `tests/distributed_knowledge/test_lora_federation_coordinator_focused.cpp`:46
- Brief: Build a minimal EncryptedGradient for a given shard/round.
- Parameters:
  - `shard_id` (const std::string &): n/a
  - `round` (uint64_t): n/a
  - `samples` (size_t): n/a

### RecordingListener

#### `void onEvent(const DKDiagnosticEvent &event) override`
- Source: `tests/distributed_knowledge/test_dk_diagnostics_focused.cpp`:58
- Brief: On Event.
- Parameters:
  - `event` (const DKDiagnosticEvent &): Input parameter.
- Details: event Input parameter.

### ThrowingListener

#### `void onEvent(const DKDiagnosticEvent &) override`
- Source: `tests/distributed_knowledge/test_dk_diagnostics_focused.cpp`:67
- Brief: On Event.
- Parameters:
  - `event` (const DKDiagnosticEvent &): Input parameter.
- Details: event Input parameter.

### bench_distributed_knowledge.cpp

#### `Arg(10 '000) -> Arg(50 '000) ->Iterations(3) ->Unit(benchmark::kMillisecond)`
- Source: `benchmarks/distributed_knowledge/bench_distributed_knowledge.cpp`:302
- Brief: n/a
- Parameters:
  - `000` (10 '): n/a

#### `Args({16, 50}) -> Args({8, 50}) ->Args({4, 50}) ->Iterations(100) ->Unit(benchmark::kMillisecond)`
- Source: `benchmarks/distributed_knowledge/bench_distributed_knowledge.cpp`:256
- Brief: n/a
- Parameters:
  - `<unnamed>` ({16, 50}): n/a

#### `Args({64, 100}) -> Args({16, 100}) ->Args({4, 100}) ->Iterations(10) ->Unit(benchmark::kMillisecond)`
- Source: `benchmarks/distributed_knowledge/bench_distributed_knowledge.cpp`:220
- Brief: n/a
- Parameters:
  - `<unnamed>` ({64, 100}): n/a

#### `BENCHMARK_MAIN()`
- Source: `benchmarks/distributed_knowledge/bench_distributed_knowledge.cpp`:342
- Brief: n/a
- Parameters: none

#### `void BM_FederatedRAGMerge_N16x50(benchmark::State &state)`
- Source: `benchmarks/distributed_knowledge/bench_distributed_knowledge.cpp`:230
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_FeedbackDedup_Throughput(benchmark::State &state)`
- Source: `benchmarks/distributed_knowledge/bench_distributed_knowledge.cpp`:266
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_PublishFeedback_Latency(benchmark::State &state)`
- Source: `benchmarks/distributed_knowledge/bench_distributed_knowledge.cpp`:311
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_TriggerAggregation_N64(benchmark::State &state)`
- Source: `benchmarks/distributed_knowledge/bench_distributed_knowledge.cpp`:173
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `Iterations(10 '000) -> Unit(benchmark::kMicrosecond)`
- Source: `benchmarks/distributed_knowledge/bench_distributed_knowledge.cpp`:339
- Brief: n/a
- Parameters:
  - `000` (10 '): n/a

### bench_distributed_knowledge_or.cpp

#### `Args({8, 0}) -> Args({8, 50}) ->Args({16, 25})`
- Source: `benchmarks/distributed_knowledge/bench_distributed_knowledge_or.cpp`:164
- Brief: n/a
- Parameters:
  - `<unnamed>` ({8, 0}): n/a

#### `BENCHMARK(BM_Erase_FederationCoordinator) -> Arg(4) ->Arg(16) ->Arg(64)`
- Source: `benchmarks/distributed_knowledge/bench_distributed_knowledge_or.cpp`:190
- Brief: n/a
- Parameters:
  - `<unnamed>` (BM_Erase_FederationCoordinator): n/a

#### `BENCHMARK(BM_FeedbackPublish_BackpressureSkip)`
- Source: `benchmarks/distributed_knowledge/bench_distributed_knowledge_or.cpp`:129
- Brief: n/a
- Parameters:
  - `<unnamed>` (BM_FeedbackPublish_BackpressureSkip): n/a

#### `BENCHMARK(BM_TriggerAggregation_NoTimeout) -> Arg(4) ->Arg(8) ->Arg(16)`
- Source: `benchmarks/distributed_knowledge/bench_distributed_knowledge_or.cpp`:107
- Brief: n/a
- Parameters:
  - `<unnamed>` (BM_TriggerAggregation_NoTimeout): n/a

#### `BENCHMARK(BM_ZeroTrust_LowRiskPath)`
- Source: `benchmarks/distributed_knowledge/bench_distributed_knowledge_or.cpp`:217
- Brief: n/a
- Parameters:
  - `<unnamed>` (BM_ZeroTrust_LowRiskPath): n/a

#### `BENCHMARK_MAIN()`
- Source: `benchmarks/distributed_knowledge/bench_distributed_knowledge_or.cpp`:219
- Brief: n/a
- Parameters: none

#### `void BM_Erase_FederationCoordinator(benchmark::State &state)`
- Source: `benchmarks/distributed_knowledge/bench_distributed_knowledge_or.cpp`:173
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_FeedbackPublish_BackpressureSkip(benchmark::State &state)`
- Source: `benchmarks/distributed_knowledge/bench_distributed_knowledge_or.cpp`:114
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_MergeWithTimedOutShards(benchmark::State &state)`
- Source: `benchmarks/distributed_knowledge/bench_distributed_knowledge_or.cpp`:136
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_TriggerAggregation_NoTimeout(benchmark::State &state)`
- Source: `benchmarks/distributed_knowledge/bench_distributed_knowledge_or.cpp`:90
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_ZeroTrust_LowRiskPath(benchmark::State &state)`
- Source: `benchmarks/distributed_knowledge/bench_distributed_knowledge_or.cpp`:197
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

### bench_dk_release_gates.cpp

#### `BENCHMARK_MAIN()`
- Source: `benchmarks/distributed_knowledge/bench_dk_release_gates.cpp`:326
- Brief: n/a
- Parameters: none

### test_adapter_capability_announcement_focused.cpp

#### `TEST(FederationTrustPolicy, AlwaysPermitAllowsAll)`
- Source: `tests/distributed_knowledge/test_adapter_capability_announcement_focused.cpp`:294
- Brief: n/a
- Parameters:
  - `<unnamed>` (FederationTrustPolicy): n/a
  - `<unnamed>` (AlwaysPermitAllowsAll): n/a
- Details: TestACA-TRUST-01: AlwaysPermitTrustPolicy permits every announcement.

#### `TEST(FederationTrustPolicy, CustomPolicyCanRejectAnnouncement)`
- Source: `tests/distributed_knowledge/test_adapter_capability_announcement_focused.cpp`:308
- Brief: n/a
- Parameters:
  - `<unnamed>` (FederationTrustPolicy): n/a
  - `<unnamed>` (CustomPolicyCanRejectAnnouncement): n/a
- Details: TestACA-TRUST-02: A custom reject policy returns REJECT for flagged adapters. Implements IFederationTrustPolicy inline to simulate a real domain blocklist.

#### `TEST(FederationTrustPolicy, PolymorphicDispatchThroughBasePointer)`
- Source: `tests/distributed_knowledge/test_adapter_capability_announcement_focused.cpp`:357
- Brief: n/a
- Parameters:
  - `<unnamed>` (FederationTrustPolicy): n/a
  - `<unnamed>` (PolymorphicDispatchThroughBasePointer): n/a
- Details: TestACA-TRUST-05: AlwaysPermitTrustPolicy polymorphic dispatch works correctly. Accesses the policy through the base-class pointer to confirm vtable dispatch.

#### `TEST(FederationTrustPolicy, TrustDecisionEnumValuesAreDistinct)`
- Source: `tests/distributed_knowledge/test_adapter_capability_announcement_focused.cpp`:335
- Brief: n/a
- Parameters:
  - `<unnamed>` (FederationTrustPolicy): n/a
  - `<unnamed>` (TrustDecisionEnumValuesAreDistinct): n/a
- Details: TestACA-TRUST-03: TrustDecision enum values are distinct.

#### `TEST(FederationTrustPolicy, TrustGateRejectedErrorCodeIsUnique)`
- Source: `tests/distributed_knowledge/test_adapter_capability_announcement_focused.cpp`:343
- Brief: n/a
- Parameters:
  - `<unnamed>` (FederationTrustPolicy): n/a
  - `<unnamed>` (TrustGateRejectedErrorCodeIsUnique): n/a
- Details: TestACA-TRUST-04: DKErrorCode::TRUST_GATE_REJECTED is defined and distinct from all other codes.

#### `TEST_F(AdapterCapabilityAnnouncementTest, AdapterDomainTypeToString)`
- Source: `tests/distributed_knowledge/test_adapter_capability_announcement_focused.cpp`:77
- Brief: n/a
- Parameters:
  - `<unnamed>` (AdapterCapabilityAnnouncementTest): n/a
  - `<unnamed>` (AdapterDomainTypeToString): n/a
- Details: TestACA-02: AdapterDomainType to string conversion. Verifies that all domain types convert to human-readable strings correctly.

#### `TEST_F(AdapterCapabilityAnnouncementTest, AdapterWithdrawalAnnouncement)`
- Source: `tests/distributed_knowledge/test_adapter_capability_announcement_focused.cpp`:148
- Brief: n/a
- Parameters:
  - `<unnamed>` (AdapterCapabilityAnnouncementTest): n/a
  - `<unnamed>` (AdapterWithdrawalAnnouncement): n/a
- Details: TestACA-05: Adapter withdrawal announcement. Verifies that announcements correctly reflect adapter withdrawal status (is_withdrawal = true) for adapter downtime or maintenance scenarios.

#### `TEST_F(AdapterCapabilityAnnouncementTest, AnnouncementTimestampSet)`
- Source: `tests/distributed_knowledge/test_adapter_capability_announcement_focused.cpp`:194
- Brief: n/a
- Parameters:
  - `<unnamed>` (AdapterCapabilityAnnouncementTest): n/a
  - `<unnamed>` (AnnouncementTimestampSet): n/a
- Details: TestACA-07: Announcement timestamp is set during gossip publish. Verifies that the publisher sets the announced_at timestamp when publishing an announcement (even if not set by caller).

#### `TEST_F(AdapterCapabilityAnnouncementTest, CreateAndSerializeAnnouncement)`
- Source: `tests/distributed_knowledge/test_adapter_capability_announcement_focused.cpp`:56
- Brief: n/a
- Parameters:
  - `<unnamed>` (AdapterCapabilityAnnouncementTest): n/a
  - `<unnamed>` (CreateAndSerializeAnnouncement): n/a
- Details: TestACA-01: Adapter capability announcement creation and JSON serialization. Verifies that an AdapterCapabilityAnnouncement can be created, populated, and serialized to JSON with all required fields present.

#### `TEST_F(AdapterCapabilityAnnouncementTest, CustomDomainLabel)`
- Source: `tests/distributed_knowledge/test_adapter_capability_announcement_focused.cpp`:169
- Brief: n/a
- Parameters:
  - `<unnamed>` (AdapterCapabilityAnnouncementTest): n/a
  - `<unnamed>` (CustomDomainLabel): n/a
- Details: TestACA-06: Custom domain label in announcement. Verifies that custom domain labels are properly included in announcements when the domain type is CUSTOM.

#### `TEST_F(AdapterCapabilityAnnouncementTest, EmptyAdapterIdHandling)`
- Source: `tests/distributed_knowledge/test_adapter_capability_announcement_focused.cpp`:242
- Brief: n/a
- Parameters:
  - `<unnamed>` (AdapterCapabilityAnnouncementTest): n/a
  - `<unnamed>` (EmptyAdapterIdHandling): n/a
- Details: TestACA-09: Empty adapter ID handling (defensive). Verifies that announcements with empty adapter_id can still be serialized (though would likely be rejected by policy in practice).

#### `TEST_F(AdapterCapabilityAnnouncementTest, GossipPublisherAnnouncesCapability)`
- Source: `tests/distributed_knowledge/test_adapter_capability_announcement_focused.cpp`:98
- Brief: n/a
- Parameters:
  - `<unnamed>` (AdapterCapabilityAnnouncementTest): n/a
  - `<unnamed>` (GossipPublisherAnnouncesCapability): n/a
- Details: TestACA-03: GossipAdapterPublisher initialization and announcement dispatch. Verifies that the gossip publisher correctly initializes and can dispatch announcements to the gossip channel with proper shard ID and timestamp.

#### `TEST_F(AdapterCapabilityAnnouncementTest, MultipleAdaptersFromSameShard)`
- Source: `tests/distributed_knowledge/test_adapter_capability_announcement_focused.cpp`:124
- Brief: n/a
- Parameters:
  - `<unnamed>` (AdapterCapabilityAnnouncementTest): n/a
  - `<unnamed>` (MultipleAdaptersFromSameShard): n/a
- Details: TestACA-04: Multiple adapter announcements from single shard. Verifies that a publisher can announce multiple distinct adapters in sequence and each generates a separate gossip message.

#### `TEST_F(AdapterCapabilityAnnouncementTest, MultiplePublishersForSameShard)`
- Source: `tests/distributed_knowledge/test_adapter_capability_announcement_focused.cpp`:262
- Brief: n/a
- Parameters:
  - `<unnamed>` (AdapterCapabilityAnnouncementTest): n/a
  - `<unnamed>` (MultiplePublishersForSameShard): n/a
- Details: TestACA-10: Multiple publishers for same shard. Verifies that multiple publisher instances for the same shard can independently publish announcements without interference.

#### `TEST_F(AdapterCapabilityAnnouncementTest, ShardIdOverwrittenByPublisher)`
- Source: `tests/distributed_knowledge/test_adapter_capability_announcement_focused.cpp`:216
- Brief: n/a
- Parameters:
  - `<unnamed>` (AdapterCapabilityAnnouncementTest): n/a
  - `<unnamed>` (ShardIdOverwrittenByPublisher): n/a
- Details: TestACA-08: Shard ID is overwritten by publisher. Verifies that the publisher always sets its own shard_id in announcements, even if the announcement already contained a different shard_id.

### test_cross_shard_feedback_sync_focused.cpp

#### `TEST_F(CrossShardFeedbackSyncTest, CreateFeedbackSummary)`
- Source: `tests/distributed_knowledge/test_cross_shard_feedback_sync_focused.cpp`:70
- Brief: n/a
- Parameters:
  - `<unnamed>` (CrossShardFeedbackSyncTest): n/a
  - `<unnamed>` (CreateFeedbackSummary): n/a
- Details: TestCSS-01: FeedbackSummary creation with required fields. Verifies that a FeedbackSummary can be constructed and its fields are accessible as documented.

#### `TEST_F(CrossShardFeedbackSyncTest, DuplicateSummaryIsDeduped)`
- Source: `tests/distributed_knowledge/test_cross_shard_feedback_sync_focused.cpp`:175
- Brief: n/a
- Parameters:
  - `<unnamed>` (CrossShardFeedbackSyncTest): n/a
  - `<unnamed>` (DuplicateSummaryIsDeduped): n/a
- Details: TestCSS-07: Duplicate inbound summary_id is deduplicated. Verifies that sending the same payload twice results in receivedCount()==1 and deduplicatedCount()==1.

#### `TEST_F(CrossShardFeedbackSyncTest, FeedbackSummaryDefaultShardOriginIsAnon)`
- Source: `tests/distributed_knowledge/test_cross_shard_feedback_sync_focused.cpp`:105
- Brief: n/a
- Parameters:
  - `<unnamed>` (CrossShardFeedbackSyncTest): n/a
  - `<unnamed>` (FeedbackSummaryDefaultShardOriginIsAnon): n/a
- Details: TestCSS-03: FeedbackSummary shard_origin defaults to ANON. Verifies that the default value for shard_origin is "ANON" as required by the privacy contract.

#### `TEST_F(CrossShardFeedbackSyncTest, FeedbackSummaryJsonRoundTrip)`
- Source: `tests/distributed_knowledge/test_cross_shard_feedback_sync_focused.cpp`:84
- Brief: n/a
- Parameters:
  - `<unnamed>` (CrossShardFeedbackSyncTest): n/a
  - `<unnamed>` (FeedbackSummaryJsonRoundTrip): n/a
- Details: TestCSS-02: FeedbackSummary JSON serialization round-trip. Verifies that toJson() / fromJson() preserve all fields.

#### `TEST_F(CrossShardFeedbackSyncTest, FeedbackSyncConfigValidation)`
- Source: `tests/distributed_knowledge/test_cross_shard_feedback_sync_focused.cpp`:270
- Brief: n/a
- Parameters:
  - `<unnamed>` (CrossShardFeedbackSyncTest): n/a
  - `<unnamed>` (FeedbackSyncConfigValidation): n/a
- Details: TestCSS-12: FeedbackSyncConfig validation. Verifies that a correctly constructed FeedbackSyncConfig reports isValid() == true, and that degenerate configs (dim=0) are rejected.

#### `TEST_F(CrossShardFeedbackSyncTest, GetStatsReturnsJsonWithCounters)`
- Source: `tests/distributed_knowledge/test_cross_shard_feedback_sync_focused.cpp`:254
- Brief: n/a
- Parameters:
  - `<unnamed>` (CrossShardFeedbackSyncTest): n/a
  - `<unnamed>` (GetStatsReturnsJsonWithCounters): n/a
- Details: TestCSS-11: getStats() returns a JSON object with expected counters. Verifies that getStats() returns a JSON object containing at minimum "published" and "received" after some operations.

#### `TEST_F(CrossShardFeedbackSyncTest, HandleInboundSummaryInvokesCallback)`
- Source: `tests/distributed_knowledge/test_cross_shard_feedback_sync_focused.cpp`:135
- Brief: n/a
- Parameters:
  - `<unnamed>` (CrossShardFeedbackSyncTest): n/a
  - `<unnamed>` (HandleInboundSummaryInvokesCallback): n/a
- Details: TestCSS-05: handleInboundSummary() invokes the registered callback. Verifies that after setting a feedback callback, processing an inbound summary payload calls the callback with the correct summary_id.

#### `TEST_F(CrossShardFeedbackSyncTest, InboundPolicyCheckRejectsSummary)`
- Source: `tests/distributed_knowledge/test_cross_shard_feedback_sync_focused.cpp`:216
- Brief: n/a
- Parameters:
  - `<unnamed>` (CrossShardFeedbackSyncTest): n/a
  - `<unnamed>` (InboundPolicyCheckRejectsSummary): n/a
- Details: TestCSS-09: Inbound policy check rejects summaries. Verifies that a policy check returning false causes the summary to be silently dropped and rejectedByPolicyCount() is incremented.

#### `TEST_F(CrossShardFeedbackSyncTest, MultipleDistinctSummariesAllReceived)`
- Source: `tests/distributed_knowledge/test_cross_shard_feedback_sync_focused.cpp`:194
- Brief: n/a
- Parameters:
  - `<unnamed>` (CrossShardFeedbackSyncTest): n/a
  - `<unnamed>` (MultipleDistinctSummariesAllReceived): n/a
- Details: TestCSS-08: Multiple distinct summaries all received. Verifies that five distinct inbound summaries are all processed (no false-positive deduplication).

#### `TEST_F(CrossShardFeedbackSyncTest, PublishFeedbackDispatchesGossipMessage)`
- Source: `tests/distributed_knowledge/test_cross_shard_feedback_sync_focused.cpp`:120
- Brief: n/a
- Parameters:
  - `<unnamed>` (CrossShardFeedbackSyncTest): n/a
  - `<unnamed>` (PublishFeedbackDispatchesGossipMessage): n/a
- Details: TestCSS-04: publishFeedback() dispatches a gossip message. Verifies that publishFeedback() calls the gossip function exactly once and increments publishedCount().

#### `TEST_F(CrossShardFeedbackSyncTest, PublishFeedbackRejectsWrongEmbeddingDim)`
- Source: `tests/distributed_knowledge/test_cross_shard_feedback_sync_focused.cpp`:156
- Brief: n/a
- Parameters:
  - `<unnamed>` (CrossShardFeedbackSyncTest): n/a
  - `<unnamed>` (PublishFeedbackRejectsWrongEmbeddingDim): n/a
- Details: TestCSS-06: publishFeedback() throws when embedding dimension is wrong. Verifies that the embedding dimension validator rejects a summary whose reason_embedding length differs from max_embedding_dim.

#### `TEST_F(CrossShardFeedbackSyncTest, ZeroTrustEnforcerThrowsOnHighRisk)`
- Source: `tests/distributed_knowledge/test_cross_shard_feedback_sync_focused.cpp`:236
- Brief: n/a
- Parameters:
  - `<unnamed>` (CrossShardFeedbackSyncTest): n/a
  - `<unnamed>` (ZeroTrustEnforcerThrowsOnHighRisk): n/a
- Details: TestCSS-10: ZeroTrust enforcer returning false throws on high-risk summary. Verifies that when the ZeroTrust enforcer returns false, handleInboundSummary() throws std::runtime_error for the high-risk summary.

### test_distributed_knowledge_highcardinality_stress.cpp

#### `TEST(WaveD_DistributedKnowledgeStress, ConcurrentMergeStress)`
- Source: `tests/distributed_knowledge/test_distributed_knowledge_highcardinality_stress.cpp`:118
- Brief: n/a
- Parameters:
  - `<unnamed>` (WaveD_DistributedKnowledgeStress): n/a
  - `<unnamed>` (ConcurrentMergeStress): n/a

#### `TEST(WaveD_DistributedKnowledgeStress, HighCardinalityFederationRound)`
- Source: `tests/distributed_knowledge/test_distributed_knowledge_highcardinality_stress.cpp`:96
- Brief: n/a
- Parameters:
  - `<unnamed>` (WaveD_DistributedKnowledgeStress): n/a
  - `<unnamed>` (HighCardinalityFederationRound): n/a

#### `TEST(WaveD_DistributedKnowledgeStress, PolicyGatedSyncStress)`
- Source: `tests/distributed_knowledge/test_distributed_knowledge_highcardinality_stress.cpp`:140
- Brief: n/a
- Parameters:
  - `<unnamed>` (WaveD_DistributedKnowledgeStress): n/a
  - `<unnamed>` (PolicyGatedSyncStress): n/a

### test_distributed_knowledge_llm_focused.cpp

#### `TEST(DistributedKnowledgeLlmFocused, DK1_ConstructValidConfig_NoThrow)`
- Source: `tests/distributed_knowledge/test_distributed_knowledge_llm_focused.cpp`:16
- Brief: n/a
- Parameters:
  - `<unnamed>` (DistributedKnowledgeLlmFocused): n/a
  - `<unnamed>` (DK1_ConstructValidConfig_NoThrow): n/a

#### `TEST(DistributedKnowledgeLlmFocused, DK2_ZeroTopK_ThrowsInvalidArgument)`
- Source: `tests/distributed_knowledge/test_distributed_knowledge_llm_focused.cpp`:23
- Brief: n/a
- Parameters:
  - `<unnamed>` (DistributedKnowledgeLlmFocused): n/a
  - `<unnamed>` (DK2_ZeroTopK_ThrowsInvalidArgument): n/a

#### `TEST(DistributedKnowledgeLlmFocused, DK3_MergeEmpty_ReturnsEmptyResult)`
- Source: `tests/distributed_knowledge/test_distributed_knowledge_llm_focused.cpp`:30
- Brief: n/a
- Parameters:
  - `<unnamed>` (DistributedKnowledgeLlmFocused): n/a
  - `<unnamed>` (DK3_MergeEmpty_ReturnsEmptyResult): n/a

#### `TEST(DistributedKnowledgeLlmFocused, DK4_MergeSingleShard_ReturnsDocuments)`
- Source: `tests/distributed_knowledge/test_distributed_knowledge_llm_focused.cpp`:41
- Brief: n/a
- Parameters:
  - `<unnamed>` (DistributedKnowledgeLlmFocused): n/a
  - `<unnamed>` (DK4_MergeSingleShard_ReturnsDocuments): n/a

#### `TEST(DistributedKnowledgeLlmFocused, DK5_BuildPromptContext_ReturnsNonEmpty)`
- Source: `tests/distributed_knowledge/test_distributed_knowledge_llm_focused.cpp`:62
- Brief: n/a
- Parameters:
  - `<unnamed>` (DistributedKnowledgeLlmFocused): n/a
  - `<unnamed>` (DK5_BuildPromptContext_ReturnsNonEmpty): n/a

#### `TEST(DistributedKnowledgeLlmFocused, DK6_TopKLimit_Respected)`
- Source: `tests/distributed_knowledge/test_distributed_knowledge_llm_focused.cpp`:82
- Brief: n/a
- Parameters:
  - `<unnamed>` (DistributedKnowledgeLlmFocused): n/a
  - `<unnamed>` (DK6_TopKLimit_Respected): n/a

#### `TEST(DistributedKnowledgeLlmFocused, DK7_TimedOutShard_GracefulDegradation)`
- Source: `tests/distributed_knowledge/test_distributed_knowledge_llm_focused.cpp`:103
- Brief: n/a
- Parameters:
  - `<unnamed>` (DistributedKnowledgeLlmFocused): n/a
  - `<unnamed>` (DK7_TimedOutShard_GracefulDegradation): n/a

### test_dk_contract_hardening_focused.cpp

#### `TEST(DKContractHardeningDKC01, EntityCreateReturnsValidId)`
- Source: `tests/distributed_knowledge/test_dk_contract_hardening_focused.cpp`:170
- Brief: n/a
- Parameters:
  - `<unnamed>` (DKContractHardeningDKC01): n/a
  - `<unnamed>` (EntityCreateReturnsValidId): n/a

#### `TEST(DKContractHardeningDKC02, EntityReadReturnsCommittedState)`
- Source: `tests/distributed_knowledge/test_dk_contract_hardening_focused.cpp`:185
- Brief: n/a
- Parameters:
  - `<unnamed>` (DKContractHardeningDKC02): n/a
  - `<unnamed>` (EntityReadReturnsCommittedState): n/a

#### `TEST(DKContractHardeningDKC03, EntityUpdateProducesNewVersion)`
- Source: `tests/distributed_knowledge/test_dk_contract_hardening_focused.cpp`:201
- Brief: n/a
- Parameters:
  - `<unnamed>` (DKContractHardeningDKC03): n/a
  - `<unnamed>` (EntityUpdateProducesNewVersion): n/a

#### `TEST(DKContractHardeningDKC04, TombstonedEntityInvisible)`
- Source: `tests/distributed_knowledge/test_dk_contract_hardening_focused.cpp`:219
- Brief: n/a
- Parameters:
  - `<unnamed>` (DKContractHardeningDKC04): n/a
  - `<unnamed>` (TombstonedEntityInvisible): n/a

#### `TEST(DKContractHardeningDKC05, FederationResultIsUnionNoDuplicates)`
- Source: `tests/distributed_knowledge/test_dk_contract_hardening_focused.cpp`:234
- Brief: n/a
- Parameters:
  - `<unnamed>` (DKContractHardeningDKC05): n/a
  - `<unnamed>` (FederationResultIsUnionNoDuplicates): n/a

#### `TEST(DKContractHardeningDKC06, FederationTimeoutSurfaced)`
- Source: `tests/distributed_knowledge/test_dk_contract_hardening_focused.cpp`:259
- Brief: n/a
- Parameters:
  - `<unnamed>` (DKContractHardeningDKC06): n/a
  - `<unnamed>` (FederationTimeoutSurfaced): n/a

#### `TEST(DKContractHardeningDKC07, PartialResultReturnedOnTimeout)`
- Source: `tests/distributed_knowledge/test_dk_contract_hardening_focused.cpp`:273
- Brief: n/a
- Parameters:
  - `<unnamed>` (DKContractHardeningDKC07): n/a
  - `<unnamed>` (PartialResultReturnedOnTimeout): n/a

#### `TEST(DKContractHardeningDKC08, FederationResultLimitEnforced)`
- Source: `tests/distributed_knowledge/test_dk_contract_hardening_focused.cpp`:289
- Brief: n/a
- Parameters:
  - `<unnamed>` (DKContractHardeningDKC08): n/a
  - `<unnamed>` (FederationResultLimitEnforced): n/a

#### `TEST(DKContractHardeningDKC09, NeighboursCompleteness)`
- Source: `tests/distributed_knowledge/test_dk_contract_hardening_focused.cpp`:305
- Brief: n/a
- Parameters:
  - `<unnamed>` (DKContractHardeningDKC09): n/a
  - `<unnamed>` (NeighboursCompleteness): n/a

#### `TEST(DKContractHardeningDKC10, PathQueryOrderedByDepth)`
- Source: `tests/distributed_knowledge/test_dk_contract_hardening_focused.cpp`:322
- Brief: n/a
- Parameters:
  - `<unnamed>` (DKContractHardeningDKC10): n/a
  - `<unnamed>` (PathQueryOrderedByDepth): n/a

#### `TEST(DKContractHardeningDKC11, NeighboursLimitEnforced)`
- Source: `tests/distributed_knowledge/test_dk_contract_hardening_focused.cpp`:340
- Brief: n/a
- Parameters:
  - `<unnamed>` (DKContractHardeningDKC11): n/a
  - `<unnamed>` (NeighboursLimitEnforced): n/a

#### `TEST(DKContractHardeningDKC12, PathQueryDepthEnforced)`
- Source: `tests/distributed_knowledge/test_dk_contract_hardening_focused.cpp`:355
- Brief: n/a
- Parameters:
  - `<unnamed>` (DKContractHardeningDKC12): n/a
  - `<unnamed>` (PathQueryDepthEnforced): n/a

#### `TEST(DKContractHardeningDKC13, LwwHigherTimestampWins)`
- Source: `tests/distributed_knowledge/test_dk_contract_hardening_focused.cpp`:376
- Brief: n/a
- Parameters:
  - `<unnamed>` (DKContractHardeningDKC13): n/a
  - `<unnamed>` (LwwHigherTimestampWins): n/a

#### `TEST(DKContractHardeningDKC14, LwwTieBreakByNodeId)`
- Source: `tests/distributed_knowledge/test_dk_contract_hardening_focused.cpp`:390
- Brief: n/a
- Parameters:
  - `<unnamed>` (DKContractHardeningDKC14): n/a
  - `<unnamed>` (LwwTieBreakByNodeId): n/a

#### `TEST(DKContractHardeningDKC15, TombstonePropagationFailureSurfaced)`
- Source: `tests/distributed_knowledge/test_dk_contract_hardening_focused.cpp`:409
- Brief: n/a
- Parameters:
  - `<unnamed>` (DKContractHardeningDKC15): n/a
  - `<unnamed>` (TombstonePropagationFailureSurfaced): n/a

#### `TEST(DKContractHardeningDKC16, CrdtMergeTypeMismatchSurfaced)`
- Source: `tests/distributed_knowledge/test_dk_contract_hardening_focused.cpp`:423
- Brief: n/a
- Parameters:
  - `<unnamed>` (DKContractHardeningDKC16): n/a
  - `<unnamed>` (CrdtMergeTypeMismatchSurfaced): n/a

### test_dk_diagnostics_focused.cpp

#### `TEST(DKDiagnosticsEmitter, AddNullListenerIsNoOp)`
- Source: `tests/distributed_knowledge/test_dk_diagnostics_focused.cpp`:90
- Brief: n/a
- Parameters:
  - `<unnamed>` (DKDiagnosticsEmitter): n/a
  - `<unnamed>` (AddNullListenerIsNoOp): n/a
- Details: TestDKD-02: addListener(nullptr) is silently ignored.

#### `TEST(DKDiagnosticsEmitter, ClearListenersRemovesAll)`
- Source: `tests/distributed_knowledge/test_dk_diagnostics_focused.cpp`:99
- Brief: n/a
- Parameters:
  - `<unnamed>` (DKDiagnosticsEmitter): n/a
  - `<unnamed>` (ClearListenersRemovesAll): n/a
- Details: TestDKD-03: clearListeners() removes all registered listeners.

#### `TEST(DKDiagnosticsEmitter, EmitDedupCollisionPopulatesFieldsAndMetadata)`
- Source: `tests/distributed_knowledge/test_dk_diagnostics_focused.cpp`:235
- Brief: n/a
- Parameters:
  - `<unnamed>` (DKDiagnosticsEmitter): n/a
  - `<unnamed>` (EmitDedupCollisionPopulatesFieldsAndMetadata): n/a
- Details: TestDKD-10: emitDedupCollision populates fields and metadata.key.

#### `TEST(DKDiagnosticsEmitter, EmitFederationRollbackPopulatesFields)`
- Source: `tests/distributed_knowledge/test_dk_diagnostics_focused.cpp`:288
- Brief: n/a
- Parameters:
  - `<unnamed>` (DKDiagnosticsEmitter): n/a
  - `<unnamed>` (EmitFederationRollbackPopulatesFields): n/a
- Details: TestDKD-13: emitFederationRollback populates correct fields.

#### `TEST(DKDiagnosticsEmitter, EmitMergeTimeoutPopulatesFields)`
- Source: `tests/distributed_knowledge/test_dk_diagnostics_focused.cpp`:216
- Brief: n/a
- Parameters:
  - `<unnamed>` (DKDiagnosticsEmitter): n/a
  - `<unnamed>` (EmitMergeTimeoutPopulatesFields): n/a
- Details: TestDKD-09: emitMergeTimeout fills type, shard_id, operation_id, cause.

#### `TEST(DKDiagnosticsEmitter, EmitPartialShardMergePopulatesMetadata)`
- Source: `tests/distributed_knowledge/test_dk_diagnostics_focused.cpp`:271
- Brief: n/a
- Parameters:
  - `<unnamed>` (DKDiagnosticsEmitter): n/a
  - `<unnamed>` (EmitPartialShardMergePopulatesMetadata): n/a
- Details: TestDKD-12: emitPartialShardMerge populates responding/total shard metadata.

#### `TEST(DKDiagnosticsEmitter, EmitTrustGateRejectHasErrorSeverity)`
- Source: `tests/distributed_knowledge/test_dk_diagnostics_focused.cpp`:255
- Brief: n/a
- Parameters:
  - `<unnamed>` (DKDiagnosticsEmitter): n/a
  - `<unnamed>` (EmitTrustGateRejectHasErrorSeverity): n/a
- Details: TestDKD-11: emitTrustGateReject sets severity to ERROR.

#### `TEST(DKDiagnosticsEmitter, EmitWithNoListenersDoesNotThrow)`
- Source: `tests/distributed_knowledge/test_dk_diagnostics_focused.cpp`:79
- Brief: n/a
- Parameters:
  - `<unnamed>` (DKDiagnosticsEmitter): n/a
  - `<unnamed>` (EmitWithNoListenersDoesNotThrow): n/a
- Details: TestDKD-01: emit() with no listeners does not throw.

#### `TEST(DKDiagnosticsEmitter, ListenerCountIsAccurate)`
- Source: `tests/distributed_knowledge/test_dk_diagnostics_focused.cpp`:111
- Brief: n/a
- Parameters:
  - `<unnamed>` (DKDiagnosticsEmitter): n/a
  - `<unnamed>` (ListenerCountIsAccurate): n/a
- Details: TestDKD-04: listenerCount() reflects add and clear operations.

#### `TEST(DKDiagnosticsEmitter, MultipleListenersAllReceiveEvent)`
- Source: `tests/distributed_knowledge/test_dk_diagnostics_focused.cpp`:151
- Brief: n/a
- Parameters:
  - `<unnamed>` (DKDiagnosticsEmitter): n/a
  - `<unnamed>` (MultipleListenersAllReceiveEvent): n/a
- Details: TestDKD-06: Multiple listeners all receive the same event.

#### `TEST(DKDiagnosticsEmitter, SingleListenerReceivesEvent)`
- Source: `tests/distributed_knowledge/test_dk_diagnostics_focused.cpp`:129
- Brief: n/a
- Parameters:
  - `<unnamed>` (DKDiagnosticsEmitter): n/a
  - `<unnamed>` (SingleListenerReceivesEvent): n/a
- Details: TestDKD-05: A single registered listener receives the emitted event.

#### `TEST(DKDiagnosticsEmitter, ThrowingListenerDoesNotBlockOthers)`
- Source: `tests/distributed_knowledge/test_dk_diagnostics_focused.cpp`:174
- Brief: n/a
- Parameters:
  - `<unnamed>` (DKDiagnosticsEmitter): n/a
  - `<unnamed>` (ThrowingListenerDoesNotBlockOthers): n/a
- Details: TestDKD-07: A throwing listener does not prevent subsequent listeners from receiving the event.

#### `TEST(DKDiagnosticsEmitter, TimestampAutoFilled)`
- Source: `tests/distributed_knowledge/test_dk_diagnostics_focused.cpp`:194
- Brief: n/a
- Parameters:
  - `<unnamed>` (DKDiagnosticsEmitter): n/a
  - `<unnamed>` (TimestampAutoFilled): n/a
- Details: TestDKD-08: timestamp_utc is populated automatically when left empty.

#### `TEST(MergeHardeningPolicy, IsConstrainedSemantics)`
- Source: `tests/distributed_knowledge/test_dk_diagnostics_focused.cpp`:315
- Brief: n/a
- Parameters:
  - `<unnamed>` (MergeHardeningPolicy): n/a
  - `<unnamed>` (IsConstrainedSemantics): n/a
- Details: TestDKD-14: MergeHardeningPolicy::isConstrained() semantics. Verifies all four constraint scenarios: Unconstrained policy (all zeros, BEST_EFFORT) → false Constrained by max_merge_latency_ms → true Constrained by dedup_window_size → true Constrained by FAIL_CLOSED timeout_behavior → true

### test_federated_distillation_coordinator_focused.cpp

#### `TEST_F(FederatedDistillationCoordinatorTest, BoundedPolicyMaxRoundsEnforced)`
- Source: `tests/distributed_knowledge/test_federated_distillation_coordinator_focused.cpp`:279
- Brief: n/a
- Parameters:
  - `<unnamed>` (FederatedDistillationCoordinatorTest): n/a
  - `<unnamed>` (BoundedPolicyMaxRoundsEnforced): n/a

#### `TEST_F(FederatedDistillationCoordinatorTest, BoundedPolicyPrivacyBudgetEnforced)`
- Source: `tests/distributed_knowledge/test_federated_distillation_coordinator_focused.cpp`:297
- Brief: n/a
- Parameters:
  - `<unnamed>` (FederatedDistillationCoordinatorTest): n/a
  - `<unnamed>` (BoundedPolicyPrivacyBudgetEnforced): n/a

#### `TEST_F(FederatedDistillationCoordinatorTest, BoundedPolicyUnconstrainedAllowsBroadcast)`
- Source: `tests/distributed_knowledge/test_federated_distillation_coordinator_focused.cpp`:313
- Brief: n/a
- Parameters:
  - `<unnamed>` (FederatedDistillationCoordinatorTest): n/a
  - `<unnamed>` (BoundedPolicyUnconstrainedAllowsBroadcast): n/a

#### `TEST_F(FederatedDistillationCoordinatorTest, BroadcastWithoutSubmitThrows)`
- Source: `tests/distributed_knowledge/test_federated_distillation_coordinator_focused.cpp`:152
- Brief: n/a
- Parameters:
  - `<unnamed>` (FederatedDistillationCoordinatorTest): n/a
  - `<unnamed>` (BroadcastWithoutSubmitThrows): n/a
- Details: TestFDC-06: broadcastToStudents() throws when no labels submitted. Verifies that calling broadcastToStudents() without prior submitSoftLabels() throws std::runtime_error.

#### `TEST_F(FederatedDistillationCoordinatorTest, CoordinatorInitialState)`
- Source: `tests/distributed_knowledge/test_federated_distillation_coordinator_focused.cpp`:122
- Brief: n/a
- Parameters:
  - `<unnamed>` (FederatedDistillationCoordinatorTest): n/a
  - `<unnamed>` (CoordinatorInitialState): n/a
- Details: TestFDC-04: Coordinator initial state — round 0, no submissions. Verifies that a freshly constructed coordinator has currentRound()==0, submittedCount()==0, and no last round.

#### `TEST_F(FederatedDistillationCoordinatorTest, CreateSoftLabel)`
- Source: `tests/distributed_knowledge/test_federated_distillation_coordinator_focused.cpp`:73
- Brief: n/a
- Parameters:
  - `<unnamed>` (FederatedDistillationCoordinatorTest): n/a
  - `<unnamed>` (CreateSoftLabel): n/a
- Details: TestFDC-01: SoftLabel creation with required fields. Verifies that a SoftLabel can be constructed and all documented fields are accessible.

#### `TEST_F(FederatedDistillationCoordinatorTest, GenerateModelCardAfterBroadcast)`
- Source: `tests/distributed_knowledge/test_federated_distillation_coordinator_focused.cpp`:266
- Brief: n/a
- Parameters:
  - `<unnamed>` (FederatedDistillationCoordinatorTest): n/a
  - `<unnamed>` (GenerateModelCardAfterBroadcast): n/a
- Details: TestFDC-12: generateModelCard() captures round and DP metadata. Verifies that after one broadcast, generateModelCard() returns a snapshot with rounds_completed == 1 and the correct dp_epsilon_per_round.

#### `TEST_F(FederatedDistillationCoordinatorTest, MultipleStudentsAllReceiveBroadcast)`
- Source: `tests/distributed_knowledge/test_federated_distillation_coordinator_focused.cpp`:239
- Brief: n/a
- Parameters:
  - `<unnamed>` (FederatedDistillationCoordinatorTest): n/a
  - `<unnamed>` (MultipleStudentsAllReceiveBroadcast): n/a
- Details: TestFDC-11: Multiple students all receive the broadcast. Verifies that when three students are registered, all three receive the broadcast round.

#### `TEST_F(FederatedDistillationCoordinatorTest, PolicyGateBlocksBroadcast)`
- Source: `tests/distributed_knowledge/test_federated_distillation_coordinator_focused.cpp`:207
- Brief: n/a
- Parameters:
  - `<unnamed>` (FederatedDistillationCoordinatorTest): n/a
  - `<unnamed>` (PolicyGateBlocksBroadcast): n/a
- Details: TestFDC-09: Policy gate blocks broadcast. Verifies that a policy gate returning false causes broadcastToStudents() to throw std::runtime_error.

#### `TEST_F(FederatedDistillationCoordinatorTest, PrivacyBudgetRemainingUnlimited)`
- Source: `tests/distributed_knowledge/test_federated_distillation_coordinator_focused.cpp`:225
- Brief: n/a
- Parameters:
  - `<unnamed>` (FederatedDistillationCoordinatorTest): n/a
  - `<unnamed>` (PrivacyBudgetRemainingUnlimited): n/a
- Details: TestFDC-10: privacyBudgetRemaining() returns max for unlimited rounds. Verifies that when max_rounds == 0, privacyBudgetRemaining() returns std::numeric_limits<double>::max().

#### `TEST_F(FederatedDistillationCoordinatorTest, PrivacyBudgetVerificationUnlimited)`
- Source: `tests/distributed_knowledge/test_federated_distillation_coordinator_focused.cpp`:188
- Brief: n/a
- Parameters:
  - `<unnamed>` (FederatedDistillationCoordinatorTest): n/a
  - `<unnamed>` (PrivacyBudgetVerificationUnlimited): n/a
- Details: TestFDC-08: DP budget verification allows broadcast within budget. Verifies that verifyPrivacyBudget() returns true after one round when max_rounds == 0 (unlimited budget).

#### `TEST_F(FederatedDistillationCoordinatorTest, RegisteredStudentReceivesBroadcast)`
- Source: `tests/distributed_knowledge/test_federated_distillation_coordinator_focused.cpp`:168
- Brief: n/a
- Parameters:
  - `<unnamed>` (FederatedDistillationCoordinatorTest): n/a
  - `<unnamed>` (RegisteredStudentReceivesBroadcast): n/a
- Details: TestFDC-07: Registered student callback receives the broadcast round. Verifies that registerStudent() works and the callback is invoked with the correct round number during broadcastToStudents().

#### `TEST_F(FederatedDistillationCoordinatorTest, SoftLabelJsonRoundTrip)`
- Source: `tests/distributed_knowledge/test_federated_distillation_coordinator_focused.cpp`:96
- Brief: n/a
- Parameters:
  - `<unnamed>` (FederatedDistillationCoordinatorTest): n/a
  - `<unnamed>` (SoftLabelJsonRoundTrip): n/a
- Details: TestFDC-03: SoftLabel JSON serialization round-trip. Verifies that SoftLabel::toJson() and SoftLabel::fromJson() preserve all fields.

#### `TEST_F(FederatedDistillationCoordinatorTest, SubmitAndBroadcastAdvancesRound)`
- Source: `tests/distributed_knowledge/test_federated_distillation_coordinator_focused.cpp`:136
- Brief: n/a
- Parameters:
  - `<unnamed>` (FederatedDistillationCoordinatorTest): n/a
  - `<unnamed>` (SubmitAndBroadcastAdvancesRound): n/a
- Details: TestFDC-05: submitSoftLabels() + broadcastToStudents() advances round. Verifies that after submitting labels and broadcasting, currentRound()==1 and lastRound() has a value.

#### `TEST_F(FederatedDistillationCoordinatorTest, ValidConfigReportsValid)`
- Source: `tests/distributed_knowledge/test_federated_distillation_coordinator_focused.cpp`:87
- Brief: n/a
- Parameters:
  - `<unnamed>` (FederatedDistillationCoordinatorTest): n/a
  - `<unnamed>` (ValidConfigReportsValid): n/a
- Details: TestFDC-02: DistillationConfig validation — valid config. Verifies that a properly constructed DistillationConfig reports isValid().

### test_federated_rag_merger_focused.cpp

#### `TEST_F(FederatedRAGMergerTest, AllShardsTimedOutThrows)`
- Source: `tests/distributed_knowledge/test_federated_rag_merger_focused.cpp`:154
- Brief: n/a
- Parameters:
  - `<unnamed>` (FederatedRAGMergerTest): n/a
  - `<unnamed>` (AllShardsTimedOutThrows): n/a
- Details: TestFRM-06: merge() with shard_timeout_ms == 0 throws when all shards time out. Verifies the documented contract: when shard_timeout_ms == 0, merge() throws "all shards timed out" immediately.

#### `TEST_F(FederatedRAGMergerTest, DeduplicationRemovesDuplicateDocIds)`
- Source: `tests/distributed_knowledge/test_federated_rag_merger_focused.cpp`:178
- Brief: n/a
- Parameters:
  - `<unnamed>` (FederatedRAGMergerTest): n/a
  - `<unnamed>` (DeduplicationRemovesDuplicateDocIds): n/a
- Details: TestFRM-07: Deduplication removes documents with identical doc_ids. Verifies that when two shards return the same doc_id, only one copy appears in the merged output.

#### `TEST_F(FederatedRAGMergerTest, InvalidConfigTopKZeroRejected)`
- Source: `tests/distributed_knowledge/test_federated_rag_merger_focused.cpp`:95
- Brief: n/a
- Parameters:
  - `<unnamed>` (FederatedRAGMergerTest): n/a
  - `<unnamed>` (InvalidConfigTopKZeroRejected): n/a
- Details: TestFRM-03: Invalid config (top_k == 0) is rejected. Verifies that a config with top_k == 0 is rejected by isValid().

#### `TEST_F(FederatedRAGMergerTest, MergeEmptyShardListReturnsEmptyContext)`
- Source: `tests/distributed_knowledge/test_federated_rag_merger_focused.cpp`:231
- Brief: n/a
- Parameters:
  - `<unnamed>` (FederatedRAGMergerTest): n/a
  - `<unnamed>` (MergeEmptyShardListReturnsEmptyContext): n/a
- Details: TestFRM-10: merge() with empty shard result list returns empty context. Verifies that an empty input produces a zero-document MergedRAGContext.

#### `TEST_F(FederatedRAGMergerTest, MergeStrategyEnumeration)`
- Source: `tests/distributed_knowledge/test_federated_rag_merger_focused.cpp`:80
- Brief: n/a
- Parameters:
  - `<unnamed>` (FederatedRAGMergerTest): n/a
  - `<unnamed>` (MergeStrategyEnumeration): n/a
- Details: TestFRM-02: MergeStrategy enumeration values. Verifies that all three merge strategy values are accessible.

#### `TEST_F(FederatedRAGMergerTest, MergeWithTwoHealthyShardsCorrectCounts)`
- Source: `tests/distributed_knowledge/test_federated_rag_merger_focused.cpp`:110
- Brief: n/a
- Parameters:
  - `<unnamed>` (FederatedRAGMergerTest): n/a
  - `<unnamed>` (MergeWithTwoHealthyShardsCorrectCounts): n/a
- Details: TestFRM-04: merge() with two healthy shards returns correct document counts. Verifies that merging results from two shards (5 docs each) produces a MergedRAGContext with the expected shards_queried and shards_responded values.

#### `TEST_F(FederatedRAGMergerTest, MergerExposesConfig)`
- Source: `tests/distributed_knowledge/test_federated_rag_merger_focused.cpp`:220
- Brief: n/a
- Parameters:
  - `<unnamed>` (FederatedRAGMergerTest): n/a
  - `<unnamed>` (MergerExposesConfig): n/a
- Details: TestFRM-09: FederatedRAGMerger exposes its config correctly. Verifies that the config passed to the constructor is accessible via config().

#### `TEST_F(FederatedRAGMergerTest, MergerIsStateless)`
- Source: `tests/distributed_knowledge/test_federated_rag_merger_focused.cpp`:246
- Brief: n/a
- Parameters:
  - `<unnamed>` (FederatedRAGMergerTest): n/a
  - `<unnamed>` (MergerIsStateless): n/a
- Details: TestFRM-11: Merger is stateless — successive calls are independent. Verifies that calling merge() twice on the same merger instance produces independent contexts (stateless contract).

#### `TEST_F(FederatedRAGMergerTest, PartialShardFailureSkipsFailedShard)`
- Source: `tests/distributed_knowledge/test_federated_rag_merger_focused.cpp`:130
- Brief: n/a
- Parameters:
  - `<unnamed>` (FederatedRAGMergerTest): n/a
  - `<unnamed>` (PartialShardFailureSkipsFailedShard): n/a
- Details: TestFRM-05: merge() skips failed shard (ok == false) gracefully. Verifies that a shard with ok == false is excluded from the merged context but the merge succeeds with the remaining responding shards.

#### `TEST_F(FederatedRAGMergerTest, RetrievedDocumentSerialization)`
- Source: `tests/distributed_knowledge/test_federated_rag_merger_focused.cpp`:265
- Brief: n/a
- Parameters:
  - `<unnamed>` (FederatedRAGMergerTest): n/a
  - `<unnamed>` (RetrievedDocumentSerialization): n/a
- Details: TestFRM-12: RetrievedDocument JSON serialization. Verifies that a RetrievedDocument serialises to JSON with the expected fields.

#### `TEST_F(FederatedRAGMergerTest, TopKTruncationRespected)`
- Source: `tests/distributed_knowledge/test_federated_rag_merger_focused.cpp`:198
- Brief: n/a
- Parameters:
  - `<unnamed>` (FederatedRAGMergerTest): n/a
  - `<unnamed>` (TopKTruncationRespected): n/a
- Details: TestFRM-08: Top-K truncation is respected. Verifies that the merged output never exceeds top_k documents.

#### `TEST_F(FederatedRAGMergerTest, ValidConfigReportsValid)`
- Source: `tests/distributed_knowledge/test_federated_rag_merger_focused.cpp`:71
- Brief: n/a
- Parameters:
  - `<unnamed>` (FederatedRAGMergerTest): n/a
  - `<unnamed>` (ValidConfigReportsValid): n/a
- Details: TestFRM-01: FederatedRAGMergerConfig validation — valid config. Verifies that a properly constructed FederatedRAGMergerConfig reports isValid() == true.

### test_lora_federation_coordinator_focused.cpp

#### `TEST_F(LoRAFederationCoordinatorTest, AggregationAdvancesRound)`
- Source: `tests/distributed_knowledge/test_lora_federation_coordinator_focused.cpp`:178
- Brief: n/a
- Parameters:
  - `<unnamed>` (LoRAFederationCoordinatorTest): n/a
  - `<unnamed>` (AggregationAdvancesRound): n/a
- Details: TestLFC-09: Successful aggregation advances the round counter. Verifies that after triggerAggregation() succeeds, currentRound() is 1.

#### `TEST_F(LoRAFederationCoordinatorTest, AggregationSucceedsWithMinParticipants)`
- Source: `tests/distributed_knowledge/test_lora_federation_coordinator_focused.cpp`:161
- Brief: n/a
- Parameters:
  - `<unnamed>` (LoRAFederationCoordinatorTest): n/a
  - `<unnamed>` (AggregationSucceedsWithMinParticipants): n/a
- Details: TestLFC-08: triggerAggregation() succeeds with min_participants met. Verifies that with two gradients submitted (== min_participants), triggerAggregation() returns a valid GlobalAdapterDelta.

#### `TEST_F(LoRAFederationCoordinatorTest, AggregationThrowsBelowMinParticipants)`
- Source: `tests/distributed_knowledge/test_lora_federation_coordinator_focused.cpp`:148
- Brief: n/a
- Parameters:
  - `<unnamed>` (LoRAFederationCoordinatorTest): n/a
  - `<unnamed>` (AggregationThrowsBelowMinParticipants): n/a
- Details: TestLFC-07: triggerAggregation() throws when below min_participants. Verifies that calling triggerAggregation() with only one gradient submitted (min_participants == 2) throws std::runtime_error.

#### `TEST_F(LoRAFederationCoordinatorTest, AggregationTimeoutHandling)`
- Source: `tests/distributed_knowledge/test_lora_federation_coordinator_focused.cpp`:194
- Brief: n/a
- Parameters:
  - `<unnamed>` (LoRAFederationCoordinatorTest): n/a
  - `<unnamed>` (AggregationTimeoutHandling): n/a
- Details: TestLFC-10: triggerAggregation() with timeout_ms=1 throws on zero-gradient round. Verifies timeout error handling: when no gradients are submitted and a 1 ms timeout is requested, a std::runtime_error is thrown.

#### `TEST_F(LoRAFederationCoordinatorTest, CoordinatorInitialState)`
- Source: `tests/distributed_knowledge/test_lora_federation_coordinator_focused.cpp`:86
- Brief: n/a
- Parameters:
  - `<unnamed>` (LoRAFederationCoordinatorTest): n/a
  - `<unnamed>` (CoordinatorInitialState): n/a
- Details: TestLFC-03: Coordinator initial state — round 0, no submissions. Verifies that a freshly constructed coordinator starts at round 0 with no pending gradients and no completed delta.

#### `TEST_F(LoRAFederationCoordinatorTest, DuplicateGradientIsIdempotent)`
- Source: `tests/distributed_knowledge/test_lora_federation_coordinator_focused.cpp`:116
- Brief: n/a
- Parameters:
  - `<unnamed>` (LoRAFederationCoordinatorTest): n/a
  - `<unnamed>` (DuplicateGradientIsIdempotent): n/a
- Details: TestLFC-05: Duplicate gradient submission is idempotent. Verifies that submitting two gradients from the same shard for the same round only counts once (idempotency contract).

#### `TEST_F(LoRAFederationCoordinatorTest, GlobalAdapterDeltaSerializationRoundTrip)`
- Source: `tests/distributed_knowledge/test_lora_federation_coordinator_focused.cpp`:228
- Brief: n/a
- Parameters:
  - `<unnamed>` (LoRAFederationCoordinatorTest): n/a
  - `<unnamed>` (GlobalAdapterDeltaSerializationRoundTrip): n/a
- Details: TestLFC-12: GlobalAdapterDelta JSON serialization round-trip. Verifies that GlobalAdapterDelta serialises to JSON and deserialises back with consistent field values.

#### `TEST_F(LoRAFederationCoordinatorTest, InvalidEpsilonRejected)`
- Source: `tests/distributed_knowledge/test_lora_federation_coordinator_focused.cpp`:75
- Brief: n/a
- Parameters:
  - `<unnamed>` (LoRAFederationCoordinatorTest): n/a
  - `<unnamed>` (InvalidEpsilonRejected): n/a
- Details: TestLFC-02: FederationConfig validation — invalid epsilon rejected. Verifies that a config with dp_epsilon == 0 is rejected by isValid().

#### `TEST_F(LoRAFederationCoordinatorTest, MultipleConsecutiveRounds)`
- Source: `tests/distributed_knowledge/test_lora_federation_coordinator_focused.cpp`:210
- Brief: n/a
- Parameters:
  - `<unnamed>` (LoRAFederationCoordinatorTest): n/a
  - `<unnamed>` (MultipleConsecutiveRounds): n/a
- Details: TestLFC-11: Multiple consecutive aggregation rounds. Verifies that the coordinator correctly handles two sequential rounds, advancing round counter with each successful aggregation.

#### `TEST_F(LoRAFederationCoordinatorTest, SubmitGradientIncrementsCount)`
- Source: `tests/distributed_knowledge/test_lora_federation_coordinator_focused.cpp`:103
- Brief: n/a
- Parameters:
  - `<unnamed>` (LoRAFederationCoordinatorTest): n/a
  - `<unnamed>` (SubmitGradientIncrementsCount): n/a
- Details: TestLFC-04: submitGradient() increments submittedCount. Verifies that submitting a gradient for round 0 increments submittedCount().

#### `TEST_F(LoRAFederationCoordinatorTest, TwoShardSubmissionsCountedCorrectly)`
- Source: `tests/distributed_knowledge/test_lora_federation_coordinator_focused.cpp`:130
- Brief: n/a
- Parameters:
  - `<unnamed>` (LoRAFederationCoordinatorTest): n/a
  - `<unnamed>` (TwoShardSubmissionsCountedCorrectly): n/a
- Details: TestLFC-06: Multiple distinct shard submissions are counted correctly. Verifies that two distinct shards contributing gradients results in submittedCount() == 2.

#### `TEST_F(LoRAFederationCoordinatorTest, ValidConfigReportsValid)`
- Source: `tests/distributed_knowledge/test_lora_federation_coordinator_focused.cpp`:66
- Brief: n/a
- Parameters:
  - `<unnamed>` (LoRAFederationCoordinatorTest): n/a
  - `<unnamed>` (ValidConfigReportsValid): n/a
- Details: TestLFC-01: FederationConfig validation — valid config. Verifies that a properly constructed FederationConfig reports isValid() == true.

### themis::bench::dkrg

#### `void BM_DKRG01_EntityInsert(benchmark::State &state)`
- Source: `benchmarks/distributed_knowledge/bench_dk_release_gates.cpp`:162
- Brief: DKRG-01: unordered_map insert for BenchEntity. GATE-DKRG-01: ≥ 100k inserts/s.
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_DKRG02_NeighboursLookup(benchmark::State &state)`
- Source: `benchmarks/distributed_knowledge/bench_dk_release_gates.cpp`:193
- Brief: DKRG-02: getNeighbours() for a node with 10 direct edges. GATE-DKRG-02: p99 ≤ 500 µs.
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_DKRG03_PathQueryDepth3(benchmark::State &state)`
- Source: `benchmarks/distributed_knowledge/bench_dk_release_gates.cpp`:217
- Brief: DKRG-03: BFS path query to depth 3. GATE-DKRG-03: p99 ≤ 5 ms.
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_DKRG04_EntityMergeLww(benchmark::State &state)`
- Source: `benchmarks/distributed_knowledge/bench_dk_release_gates.cpp`:241
- Brief: DKRG-04: resolveLww() — LWW conflict resolution decision. GATE-DKRG-04: p99 ≤ 100 µs.
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_DKRG05_FederationResultUnion(benchmark::State &state)`
- Source: `benchmarks/distributed_knowledge/bench_dk_release_gates.cpp`:268
- Brief: DKRG-05: federation result union for 2×100 entity ID sets. UseRealTime() because set operations involve memory allocation. GATE-DKRG-05: p99 ≤ 5 ms.
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_DKRG06_EntitySerialization(benchmark::State &state)`
- Source: `benchmarks/distributed_knowledge/bench_dk_release_gates.cpp`:298
- Brief: DKRG-06: Serialize a BenchEntity with a 128-byte payload to string. GATE-DKRG-06: p99 ≤ 100 µs.
- Parameters:
  - `state` (benchmark::State &): n/a

#### `Repetitions(kRepetitions) -> ReportAggregatesOnly(true)`
- Source: `benchmarks/distributed_knowledge/bench_dk_release_gates.cpp`:182
- Brief: n/a
- Parameters:
  - `<unnamed>` (kRepetitions): n/a

#### `UseRealTime() -> Repetitions(kRepetitions) ->ReportAggregatesOnly(true)`
- Source: `benchmarks/distributed_knowledge/bench_dk_release_gates.cpp`:286
- Brief: n/a
- Parameters: none

#### `std::set< std::string > bfsDepth3(const BenchGraph &g, const std::string &start)`
- Source: `benchmarks/distributed_knowledge/bench_dk_release_gates.cpp`:123
- Brief: n/a
- Parameters:
  - `g` (const BenchGraph &): n/a
  - `start` (const std::string &): n/a

#### `std::set< std::string > federationUnion(const std::vector< std::string > &a, const std::vector< std::string > &b)`
- Source: `benchmarks/distributed_knowledge/bench_dk_release_gates.cpp`:146
- Brief: n/a
- Parameters:
  - `a` (const std::vector< std::string > &): n/a
  - `b` (const std::vector< std::string > &): n/a

#### `std::vector< std::string > getNeighbours(const BenchGraph &g, const std::string &node)`
- Source: `benchmarks/distributed_knowledge/bench_dk_release_gates.cpp`:117
- Brief: n/a
- Parameters:
  - `g` (const BenchGraph &): n/a
  - `node` (const std::string &): n/a

#### `BenchEntity makeEntity(int idx, const char *node_id) noexcept`
- Source: `benchmarks/distributed_knowledge/bench_dk_release_gates.cpp`:90
- Brief: n/a
- Parameters:
  - `idx` (int): n/a
  - `node_id` (const char *): n/a

#### `BenchGraph makeGraph(int n_nodes, int edges_per_node)`
- Source: `benchmarks/distributed_knowledge/bench_dk_release_gates.cpp`:105
- Brief: n/a
- Parameters:
  - `n_nodes` (int): n/a
  - `edges_per_node` (int): n/a

### themis::distributed_knowledge

#### `std::string adapterDomainTypeToString(AdapterDomainType t)`
- Source: `include/distributed_knowledge/adapter_capability_announcement.h`:47
- Brief: Adapter Domain Type To String.
- Parameters:
  - `t` (AdapterDomainType): Input parameter.
- Return: Return value.
- Details: t Input parameter. Return value. Implements adapterDomainTypeToString without additional internal calls.

#### `bool isRetryableCode(DKErrorCode code) noexcept`
- Source: `include/distributed_knowledge/distributed_knowledge_api_contract.h`:138
- Brief: n/a
- Parameters:
  - `code` (DKErrorCode): n/a

#### `LwwDecision resolveLww(std::int64_t local_ts, std::int64_t remote_ts, const std::string &local_node, const std::string &remote_node) noexcept`
- Source: `include/distributed_knowledge/distributed_knowledge_api_contract.h`:93
- Brief: n/a
- Parameters:
  - `local_ts` (std::int64_t): n/a
  - `remote_ts` (std::int64_t): n/a
  - `local_node` (const std::string &): n/a
  - `remote_node` (const std::string &): n/a

### themis::distributed_knowledge::AdapterCapabilityAnnouncement

#### `AdapterCapabilityAnnouncement fromJson(const nlohmann::json &j)`
- Source: `include/distributed_knowledge/adapter_capability_announcement.h`:115
- Brief: n/a
- Parameters:
  - `j` (const nlohmann::json &): n/a

#### `nlohmann::json toJson() const`
- Source: `include/distributed_knowledge/adapter_capability_announcement.h`:95
- Brief: n/a
- Parameters: none

### themis::distributed_knowledge::AlwaysPermitTrustPolicy

#### `TrustDecision evaluateTrustGate(const AdapterCapabilityAnnouncement &) const noexcept override`
- Source: `include/distributed_knowledge/distributed_knowledge_api_contract.h`:277
- Brief: n/a
- Parameters:
  - `<unnamed>` (const AdapterCapabilityAnnouncement &): n/a

### themis::distributed_knowledge::CrossShardFeedbackSync

#### `CrossShardFeedbackSync(CrossShardFeedbackSync &&) noexcept=default`
- Source: `include/distributed_knowledge/cross_shard_feedback_sync.h`:99
- Brief: n/a
- Parameters:
  - `<unnamed>` (CrossShardFeedbackSync &&): n/a

#### `CrossShardFeedbackSync(FeedbackSyncConfig config, std::string local_shard_id, std::function< void(nlohmann::json)> gossip_message_fn)`
- Source: `include/distributed_knowledge/cross_shard_feedback_sync.h`:90
- Brief: n/a
- Parameters:
  - `config` (FeedbackSyncConfig): n/a
  - `local_shard_id` (std::string): n/a
  - `gossip_message_fn` (std::function< void(nlohmann::json)>): n/a

#### `CrossShardFeedbackSync(const CrossShardFeedbackSync &)=delete`
- Source: `include/distributed_knowledge/cross_shard_feedback_sync.h`:97
- Brief: n/a
- Parameters:
  - `<unnamed>` (const CrossShardFeedbackSync &): n/a

#### `size_t deduplicatedCount() const`
- Source: `include/distributed_knowledge/cross_shard_feedback_sync.h`:161
- Brief: n/a
- Parameters: none

#### `void emitFeedbackDecisionRecord(const std::string &direction, const FeedbackSummary &summary) const`
- Source: `include/distributed_knowledge/cross_shard_feedback_sync.h`:197
- Brief: Emit Feedback Decision Record.
- Parameters:
  - `direction` (const std::string &): Input parameter.
  - `summary` (const FeedbackSummary &): Input parameter.
- Details: direction Input parameter. summary Input parameter.

#### `themis::governance::StoreErasureResult erase(const std::string &subject_id="", themis::governance::Regulation regulation=themis::governance::Regulation::GDPR)`
- Source: `include/distributed_knowledge/cross_shard_feedback_sync.h`:149
- Brief: Erase.
- Parameters:
  - `subject_id` (const std::string &): n/a
  - `regulation` (themis::governance::Regulation): n/a
- Return: Return value.
- Details: param Input parameter. Regulation Input parameter. Return value.

#### `size_t eraseCount() const`
- Source: `include/distributed_knowledge/cross_shard_feedback_sync.h`:153
- Brief: n/a
- Parameters: none

#### `std::string generateSummaryId()`
- Source: `include/distributed_knowledge/cross_shard_feedback_sync.h`:191
- Brief: Generate Summary Id.
- Parameters: none
- Return: Return value.
- Details: Return value. Calls: std::chrono::system_clock::now(), time_since_epoch(), count(), rng(), dist(), str().

#### `size_t getSkippedPublishCount() const`
- Source: `include/distributed_knowledge/cross_shard_feedback_sync.h`:147
- Brief: n/a
- Parameters: none

#### `nlohmann::json getStats() const`
- Source: `include/distributed_knowledge/cross_shard_feedback_sync.h`:165
- Brief: n/a
- Parameters: none

#### `void handleInboundSummary(const nlohmann::json &payload)`
- Source: `include/distributed_knowledge/cross_shard_feedback_sync.h`:114
- Brief: ── Receiving ────────────────────────────────────────────────────────────
- Parameters:
  - `payload` (const nlohmann::json &): Input parameter.
- Throws:
  - std::runtime_error: if an error occurs.
- Details: Handle Inbound Summary. payload Input parameter. payload Input parameter. std::runtime_error if an error occurs. Calls: FeedbackSummary::fromJson(), lk(), count(), zero_trust_enforcer_(), policy_check_(), size(), clear(), insert().

#### `CrossShardFeedbackSync & operator=(CrossShardFeedbackSync &&) noexcept`
- Source: `include/distributed_knowledge/cross_shard_feedback_sync.h`:100
- Brief: n/a
- Parameters:
  - `<unnamed>` (CrossShardFeedbackSync &&): n/a

#### `CrossShardFeedbackSync & operator=(const CrossShardFeedbackSync &)=delete`
- Source: `include/distributed_knowledge/cross_shard_feedback_sync.h`:98
- Brief: n/a
- Parameters:
  - `<unnamed>` (const CrossShardFeedbackSync &): n/a

#### `void publishFeedback(FeedbackSummary summary)`
- Source: `include/distributed_knowledge/cross_shard_feedback_sync.h`:107
- Brief: ── Publishing ───────────────────────────────────────────────────────────
- Parameters:
  - `summary` (FeedbackSummary): Input parameter.
- Throws:
  - std::invalid_argument: if an error occurs.
- Details: Publish Feedback. summary Input parameter. summary Input parameter. std::invalid_argument if an error occurs. Calls: empty(), size(), std::to_string(), generateSummaryId(), std::chrono::system_clock::now(), toJson(), lk(), emitFeedbackDecisionRecord().

#### `size_t publishedCount() const`
- Source: `include/distributed_knowledge/cross_shard_feedback_sync.h`:157
- Brief: n/a
- Parameters: none

#### `size_t receivedCount() const`
- Source: `include/distributed_knowledge/cross_shard_feedback_sync.h`:159
- Brief: n/a
- Parameters: none

#### `size_t rejectedByPolicyCount() const`
- Source: `include/distributed_knowledge/cross_shard_feedback_sync.h`:163
- Brief: n/a
- Parameters: none

#### `void setDecisionRecordProcessor(std::shared_ptr< themis::llm::DecisionRecordYamlProcessor > processor)`
- Source: `include/distributed_knowledge/cross_shard_feedback_sync.h`:134
- Brief: Set Decision Record Processor.
- Parameters:
  - `processor` (std::shared_ptr< themis::llm::DecisionRecordYamlProcessor >): Input parameter.
- Details: processor Input parameter.

#### `void setFeedbackCallback(FeedbackCallback cb)`
- Source: `include/distributed_knowledge/cross_shard_feedback_sync.h`:120
- Brief: Set Feedback Callback.
- Parameters:
  - `cb` (FeedbackCallback): Input parameter.
- Details: cb Input parameter. cb Input parameter. Calls: lk(), std::move().

#### `void setInboundPolicyCheck(InboundPolicyCheck check)`
- Source: `include/distributed_knowledge/cross_shard_feedback_sync.h`:128
- Brief: Set Inbound Policy Check.
- Parameters:
  - `check` (InboundPolicyCheck): Input parameter.
- Details: check Input parameter. check Input parameter. Calls: lk(), std::move().

#### `void setZeroTrustEnforcer(ZeroTrustEnforcer enforcer)`
- Source: `include/distributed_knowledge/cross_shard_feedback_sync.h`:145
- Brief: Set Zero Trust Enforcer.
- Parameters:
  - `enforcer` (ZeroTrustEnforcer): Input parameter.
- Details: ───────────────────────────────────────────────────────────────────────────── DK-OR: ZeroTrust setter, skipped-publish counter, erase ───────────────────────────────────────────────────────────────────────────── enforcer Input parameter. enforcer Input parameter. Calls: lk(), std::move().

#### `~CrossShardFeedbackSync() noexcept`
- Source: `include/distributed_knowledge/cross_shard_feedback_sync.h`:95
- Brief: n/a
- Parameters: none

### themis::distributed_knowledge::DistillationBoundedPolicy

#### `bool isConstrained() const noexcept`
- Source: `include/distributed_knowledge/distributed_knowledge_api_contract.h`:245
- Brief: n/a
- Parameters: none

### themis::distributed_knowledge::DistillationConfig

#### `bool isValid() const`
- Source: `include/distributed_knowledge/federated_distillation_coordinator.h`:143
- Brief: n/a
- Parameters: none

### themis::distributed_knowledge::DistillationModelCard

#### `nlohmann::json toJson() const`
- Source: `include/distributed_knowledge/federated_distillation_coordinator.h`:106
- Brief: n/a
- Parameters: none

### themis::distributed_knowledge::DistillationRound

#### `nlohmann::json toJson() const`
- Source: `include/distributed_knowledge/federated_distillation_coordinator.h`:67
- Brief: n/a
- Parameters: none

### themis::distributed_knowledge::DistributedKnowledgeDiagnosticEmitter

#### `DistributedKnowledgeDiagnosticEmitter()=default`
- Source: `include/distributed_knowledge/dk_diagnostic_emitter.h`:75
- Brief: n/a
- Parameters: none

#### `DistributedKnowledgeDiagnosticEmitter(DistributedKnowledgeDiagnosticEmitter &&) noexcept=default`
- Source: `include/distributed_knowledge/dk_diagnostic_emitter.h`:81
- Brief: n/a
- Parameters:
  - `<unnamed>` (DistributedKnowledgeDiagnosticEmitter &&): n/a

#### `DistributedKnowledgeDiagnosticEmitter(const DistributedKnowledgeDiagnosticEmitter &)=delete`
- Source: `include/distributed_knowledge/dk_diagnostic_emitter.h`:79
- Brief: n/a
- Parameters:
  - `<unnamed>` (const DistributedKnowledgeDiagnosticEmitter &): n/a

#### `void addListener(std::shared_ptr< IDKDiagnosticListener > listener)`
- Source: `include/distributed_knowledge/dk_diagnostic_emitter.h`:93
- Brief: Add Listener.
- Parameters:
  - `listener` (std::shared_ptr< IDKDiagnosticListener >): Input parameter.
- Details: listener Input parameter. Calls: lock(), push_back(), std::move().

#### `void clearListeners()`
- Source: `include/distributed_knowledge/dk_diagnostic_emitter.h`:103
- Brief: Clear Listeners.
- Parameters: none
- Details: Calls: lock(), clear().

#### `void emit(DKDiagnosticEvent event)`
- Source: `include/distributed_knowledge/dk_diagnostic_emitter.h`:127
- Brief: Emit.
- Parameters:
  - `event` (DKDiagnosticEvent): Input parameter.
- Details: event Input parameter. Calls: lock(), empty(), utcNow(), onEvent().

#### `void emitDedupCollision(const std::string &shard_id, const std::string &operation_id, const std::string &duplicate_key)`
- Source: `include/distributed_knowledge/dk_diagnostic_emitter.h`:163
- Brief: Emit Dedup Collision.
- Parameters:
  - `shard_id` (const std::string &): Identifier of the shard.
  - `operation_id` (const std::string &): Identifier of the operation.
  - `duplicate_key` (const std::string &): Input parameter.
- Details: shard_id Identifier of the shard. operation_id Identifier of the operation. duplicate_key Input parameter. Calls: emit(), std::move().

#### `void emitFederationRollback(const std::string &operation_id, const std::string &cause, DKDiagnosticSeverity severity=DKDiagnosticSeverity::ERROR)`
- Source: `include/distributed_knowledge/dk_diagnostic_emitter.h`:216
- Brief: n/a
- Parameters:
  - `operation_id` (const std::string &): n/a
  - `cause` (const std::string &): n/a
  - `severity` (DKDiagnosticSeverity): n/a

#### `void emitMergeTimeout(const std::string &shard_id, const std::string &operation_id, const std::string &cause, DKDiagnosticSeverity severity=DKDiagnosticSeverity::WARNING)`
- Source: `include/distributed_knowledge/dk_diagnostic_emitter.h`:143
- Brief: n/a
- Parameters:
  - `shard_id` (const std::string &): n/a
  - `operation_id` (const std::string &): n/a
  - `cause` (const std::string &): n/a
  - `severity` (DKDiagnosticSeverity): n/a

#### `void emitPartialShardMerge(const std::string &operation_id, std::size_t responding_shards, std::size_t total_shards)`
- Source: `include/distributed_knowledge/dk_diagnostic_emitter.h`:202
- Brief: Emit Partial Shard Merge.
- Parameters:
  - `operation_id` (const std::string &): Identifier of the operation.
  - `responding_shards` (std::size_t): Input parameter.
  - `total_shards` (std::size_t): Input parameter.
- Details: operation_id Identifier of the operation. responding_shards Input parameter. total_shards Input parameter. Calls: std::to_string(), emit(), std::move().

#### `void emitTrustGateReject(const std::string &shard_id, const std::string &operation_id, const std::string &cause)`
- Source: `include/distributed_knowledge/dk_diagnostic_emitter.h`:183
- Brief: Emit Trust Gate Reject.
- Parameters:
  - `shard_id` (const std::string &): Identifier of the shard.
  - `operation_id` (const std::string &): Identifier of the operation.
  - `cause` (const std::string &): Input parameter.
- Details: shard_id Identifier of the shard. operation_id Identifier of the operation. cause Input parameter. Calls: emit(), std::move().

#### `std::size_t listenerCount() const noexcept`
- Source: `include/distributed_knowledge/dk_diagnostic_emitter.h`:108
- Brief: n/a
- Parameters: none

#### `DistributedKnowledgeDiagnosticEmitter & operator=(DistributedKnowledgeDiagnosticEmitter &&) noexcept=default`
- Source: `include/distributed_knowledge/dk_diagnostic_emitter.h`:82
- Brief: n/a
- Parameters:
  - `<unnamed>` (DistributedKnowledgeDiagnosticEmitter &&): n/a

#### `DistributedKnowledgeDiagnosticEmitter & operator=(const DistributedKnowledgeDiagnosticEmitter &)=delete`
- Source: `include/distributed_knowledge/dk_diagnostic_emitter.h`:80
- Brief: n/a
- Parameters:
  - `<unnamed>` (const DistributedKnowledgeDiagnosticEmitter &): n/a

#### `std::string utcNow()`
- Source: `include/distributed_knowledge/dk_diagnostic_emitter.h`:231
- Brief: n/a
- Parameters: none

#### `~DistributedKnowledgeDiagnosticEmitter()=default`
- Source: `include/distributed_knowledge/dk_diagnostic_emitter.h`:76
- Brief: n/a
- Parameters: none

### themis::distributed_knowledge::EncryptedGradient

#### `EncryptedGradient fromJson(const nlohmann::json &j)`
- Source: `include/distributed_knowledge/lora_federation_coordinator.h`:48
- Brief: n/a
- Parameters:
  - `j` (const nlohmann::json &): n/a

#### `nlohmann::json toJson() const`
- Source: `include/distributed_knowledge/lora_federation_coordinator.h`:41
- Brief: n/a
- Parameters: none

### themis::distributed_knowledge::FederatedDistillationCoordinator

#### `FederatedDistillationCoordinator(DistillationConfig cfg={})`
- Source: `include/distributed_knowledge/federated_distillation_coordinator.h`:202
- Brief: n/a
- Parameters:
  - `cfg` (DistillationConfig): n/a

#### `FederatedDistillationCoordinator(FederatedDistillationCoordinator &&) noexcept=default`
- Source: `include/distributed_knowledge/federated_distillation_coordinator.h`:207
- Brief: n/a
- Parameters:
  - `<unnamed>` (FederatedDistillationCoordinator &&): n/a

#### `FederatedDistillationCoordinator(const FederatedDistillationCoordinator &)=delete`
- Source: `include/distributed_knowledge/federated_distillation_coordinator.h`:205
- Brief: n/a
- Parameters:
  - `<unnamed>` (const FederatedDistillationCoordinator &): n/a

#### `void applyDPNoise(std::vector< SoftLabel > &labels) const`
- Source: `include/distributed_knowledge/federated_distillation_coordinator.h`:282
- Brief: ── Implementation ─────────────────────────────────────────────────────────
- Parameters:
  - `labels` (std::vector< SoftLabel > &): Input/output parameter.
- Details: labels Input/output parameter.

#### `DistillationRound broadcastToStudents() override`
- Source: `include/distributed_knowledge/federated_distillation_coordinator.h`:215
- Brief: Broadcast To Students.
- Parameters: none
- Return: Return value.
- Throws:
  - std::runtime_error: if an error occurs.
- Details: Return value. std::runtime_error if an error occurs. Calls: lock(), verifyPrivacyBudget(), isConstrained(), policy_gate_(), applyDPNoise(), std::move(), size(), cb().

#### `const DistillationConfig & config() const`
- Source: `include/distributed_knowledge/federated_distillation_coordinator.h`:274
- Brief: n/a
- Parameters: none

#### `uint64_t currentRound() const override`
- Source: `include/distributed_knowledge/federated_distillation_coordinator.h`:217
- Brief: n/a
- Parameters: none

#### `DistillationModelCard generateModelCard(const std::string &coordinator_id="") const`
- Source: `include/distributed_knowledge/federated_distillation_coordinator.h`:271
- Brief: n/a
- Parameters:
  - `coordinator_id` (const std::string &): n/a

#### `nlohmann::json getStats() const override`
- Source: `include/distributed_knowledge/federated_distillation_coordinator.h`:220
- Brief: n/a
- Parameters: none

#### `std::optional< DistillationRound > lastRound() const override`
- Source: `include/distributed_knowledge/federated_distillation_coordinator.h`:219
- Brief: n/a
- Parameters: none

#### `FederatedDistillationCoordinator & operator=(FederatedDistillationCoordinator &&) noexcept`
- Source: `include/distributed_knowledge/federated_distillation_coordinator.h`:208
- Brief: n/a
- Parameters:
  - `<unnamed>` (FederatedDistillationCoordinator &&): n/a

#### `FederatedDistillationCoordinator & operator=(const FederatedDistillationCoordinator &)=delete`
- Source: `include/distributed_knowledge/federated_distillation_coordinator.h`:206
- Brief: n/a
- Parameters:
  - `<unnamed>` (const FederatedDistillationCoordinator &): n/a

#### `double privacyBudgetRemaining() const`
- Source: `include/distributed_knowledge/federated_distillation_coordinator.h`:262
- Brief: n/a
- Parameters: none

#### `void registerStudent(const std::string &student_id, std::function< void(const DistillationRound &)> cb) override`
- Source: `include/distributed_knowledge/federated_distillation_coordinator.h`:222
- Brief: n/a
- Parameters:
  - `student_id` (const std::string &): n/a
  - `cb` (std::function< void(const DistillationRound &)>): n/a

#### `void reportStudentUtility(const std::string &student_id, double utility)`
- Source: `include/distributed_knowledge/federated_distillation_coordinator.h`:258
- Brief: ── Utility reporting ──────────────────────────────────────────────────────
- Parameters:
  - `student_id` (const std::string &): Identifier of the student.
  - `utility` (double): Input parameter.
- Details: Report Student Utility. student_id Identifier of the student. utility Input parameter. param Input parameter. utility Input parameter. Calls: lock(), rollback_trigger_().

#### `void reset()`
- Source: `include/distributed_knowledge/federated_distillation_coordinator.h`:269
- Brief: Reset the modification detection flag.
- Parameters: none
- Details: Calls: lock(), clear().

#### `void setAuditCallback(std::function< void(const nlohmann::json &)> cb)`
- Source: `include/distributed_knowledge/federated_distillation_coordinator.h`:248
- Brief: n/a
- Parameters:
  - `cb` (std::function< void(const nlohmann::json &)>): n/a

#### `void setBoundedPolicy(DistillationBoundedPolicy policy)`
- Source: `include/distributed_knowledge/federated_distillation_coordinator.h`:246
- Brief: Set Bounded Policy.
- Parameters:
  - `policy` (DistillationBoundedPolicy): Input parameter.
- Details: policy Input parameter. policy Input parameter. Calls: lock().

#### `void setNoiseGeneratorFn(NoiseGeneratorFn fn)`
- Source: `include/distributed_knowledge/federated_distillation_coordinator.h`:234
- Brief: Set Noise Generator Fn.
- Parameters:
  - `fn` (NoiseGeneratorFn): Input parameter.
- Details: fn Input parameter. fn Input parameter. Calls: lock(), std::move().

#### `void setPolicyGate(PolicyGate gate)`
- Source: `include/distributed_knowledge/federated_distillation_coordinator.h`:240
- Brief: Set Policy Gate.
- Parameters:
  - `gate` (PolicyGate): Input parameter.
- Details: gate Input parameter. gate Input parameter. Calls: lock(), std::move().

#### `void setRollbackTrigger(std::function< void(uint64_t, double)> cb)`
- Source: `include/distributed_knowledge/federated_distillation_coordinator.h`:250
- Brief: n/a
- Parameters:
  - `cb` (std::function< void(uint64_t, double)>): n/a

#### `void submitSoftLabels(const std::string &teacher_id, std::vector< SoftLabel > labels) override`
- Source: `include/distributed_knowledge/federated_distillation_coordinator.h`:212
- Brief: Submit Soft Labels.
- Parameters:
  - `teacher_id` (const std::string &): Identifier of the teacher.
  - `labels` (std::vector< SoftLabel >): Input parameter.
- Throws:
  - std::invalid_argument: if an error occurs.
  - std::runtime_error: if an error occurs.
- Details: teacher_id Identifier of the teacher. labels Input parameter. std::invalid_argument if an error occurs. std::runtime_error if an error occurs. Calls: lock(), empty(), has_value(), std::move().

#### `size_t submittedCount() const override`
- Source: `include/distributed_knowledge/federated_distillation_coordinator.h`:218
- Brief: n/a
- Parameters: none

#### `bool verifyPrivacyBudget() const`
- Source: `include/distributed_knowledge/federated_distillation_coordinator.h`:264
- Brief: n/a
- Parameters: none

#### `~FederatedDistillationCoordinator() noexcept override`
- Source: `include/distributed_knowledge/federated_distillation_coordinator.h`:203
- Brief: n/a
- Parameters: none

### themis::distributed_knowledge::FederatedRAGMerger

#### `FederatedRAGMerger(FederatedRAGMergerConfig config={})`
- Source: `include/distributed_knowledge/federated_rag_merger.h`:290
- Brief: n/a
- Parameters:
  - `config` (FederatedRAGMergerConfig): n/a

#### `const FederatedRAGMergerConfig & config() const`
- Source: `include/distributed_knowledge/federated_rag_merger.h`:364
- Brief: n/a
- Parameters: none

#### `std::vector< RetrievedDocument > deduplicate(std::vector< RetrievedDocument > docs) const`
- Source: `include/distributed_knowledge/federated_rag_merger.h`:508
- Brief: Remove documents with duplicate doc_id, preserving order.
- Parameters:
  - `docs` (std::vector< RetrievedDocument >): Input ranked document list (may contain duplicates by doc_id)
- Return: Deduplicated document list, order preserved, size ≤ input size Deterministic: same input → same output, reproducible across runs
- Details: Deduplication Semantics: Iterates input in order For each doc_id: keeps first occurrence, discards later ones Result size ≤ input size (never grows) Order preserved: output documents in input order Determinism Contract: Uses std::set (ordered) instead of std::unordered_set to ensure DETERMINISTIC iteration order Identical input always produces identical output Enables reproducible testing and reliable debugging in distributed systems Performance: O(n log n) worst-case; negligible for typical dedup sets (<100 docs) See DKRG-01..06 benchmarks for performance validation (<5% regression expected <1%) Conflict Resolution: When same doc_id appears multiple times (merged from different shards), the first (highest-ranked) version is kept Content from duplicate is discarded (shard_id, metadata unchanged from first) Use Case: After RRF/weighted/round-robin merge, removes same document from different shards Before top_k truncation (dedup may reduce count below top_k) docs Input ranked document list (may contain duplicates by doc_id) Deduplicated document list, order preserved, size ≤ input size Deterministic: same input → same output, reproducible across runs

#### `themis::governance::StoreErasureResult erase(const std::string &subject_id="", themis::governance::Regulation regulation=themis::governance::Regulation::GDPR)`
- Source: `include/distributed_knowledge/federated_rag_merger.h`:373
- Brief: Clear any cached merge context (DK-OR-H-2).
- Parameters:
  - `subject_id` (const std::string &): n/a
  - `regulation` (themis::governance::Regulation): n/a
- Return: Return value.
- Details: ───────────────────────────────────────────────────────────────────────────── DK-OR: GDPR erase (clears cached merge context) ───────────────────────────────────────────────────────────────────────────── Clears internal state and increments erase_count_. param Input parameter. Regulation Input parameter. Return value. Implements erase without additional internal calls.

#### `size_t eraseCount() const`
- Source: `include/distributed_knowledge/federated_rag_merger.h`:377
- Brief: n/a
- Parameters: none

#### `MergedRAGContext merge(const std::vector< ShardRetrievalResult > &shard_results) const`
- Source: `include/distributed_knowledge/federated_rag_merger.h`:343
- Brief: Merge retrieval results from multiple shards using configured strategy.
- Parameters:
  - `shard_results` (const std::vector< ShardRetrievalResult > &): Per-shard retrieval responses. sr.ok == false → silently skipped (shard failure) sr.timed_out == true → silently skipped (shard timeout) sr.documents → ranked list per shard sr.adapter_accuracy_delta → shard specialisation weight (for RRF/weighted)
- Return: MergedRAGContext with: documents: ranked merged list (size ≤ top_k) total_candidate_count: sum of doc counts from responding shards unique_doc_count: size after dedup (if enabled) shards_queried: \|shard_results\| shards_responded: count with ok == true && timed_out == false strategy_used: which algorithm was applied
- Details: Orchestrates merge conflict resolution by: Filtering timed-out or failed shards Applying selected merge strategy (RRF, score-weighted, or round-robin) Optional deduplication by doc_id Truncation to top_k Consistency Level: EVENTUAL Operation is stateless; no coordinator synchronization required Shard responses may be from different temporal snapshots (stale OK) Merge tolerates heterogeneous freshness: RRF/weighted/round-robin robust to rank shifts No version vector; timestamp in ShardRetrievalResult is informational only Rationale: Merge correctness depends only on input ranking, not absolute freshness Version Tracking: ShardRetrievalResult.latency_ms documents response age (informational) No causal ordering enforced; shards may respond in any order Deterministic: same inputs always produce same output Replication Lag: Max acceptable lag: unbounded (design intent: eventual consistency OK) Policy: Stale rankings accepted by merge strategies Correctness: Cross-shard score/rank bias naturally bounds stale-data impact Safe-read: Timestamp available but not enforced as guard Exception Semantics: std::runtime_error("all shards timed out"): when shard_timeout_ms == 0 or all shards report timed_out == true std::invalid_argument: when config_ is invalid (checked in ctor) Strong exception safety: merge either completes or throws; no partial state change Idempotency: Same shard_results input always produces identical output Safe to retry; no internal state mutation beyond GDPR erase_count_ shard_results Per-shard retrieval responses. sr.ok == false → silently skipped (shard failure) sr.timed_out == true → silently skipped (shard timeout) sr.documents → ranked list per shard sr.adapter_accuracy_delta → shard specialisation weight (for RRF/weighted) MergedRAGContext with: documents: ranked merged list (size ≤ top_k) total_candidate_count: sum of doc counts from responding shards unique_doc_count: size after dedup (if enabled) shards_queried: \|shard_results\| shards_responded: count with ok == true && timed_out == false strategy_used: which algorithm was applied

#### `std::string mergeAndBuildContext(const std::vector< ShardRetrievalResult > &shard_results, size_t max_docs=10, size_t max_chars=0) const`
- Source: `include/distributed_knowledge/federated_rag_merger.h`:359
- Brief: Convenience overload: merge and directly build prompt context.
- Parameters:
  - `shard_results` (const std::vector< ShardRetrievalResult > &): Per-shard results (see merge() for details).
  - `max_docs` (size_t): Maximum documents to include in prompt (0 = all).
  - `max_chars` (size_t): Character budget (0 = unlimited). If specified, last document is truncated to fit budget.
- Return: Formatted prompt context string with "[Shard: X] content" per document.
- Throws:
  - std::runtime_error: (see merge() exception contract)
- Details: Equivalent to: merge(shard_results).buildPromptContext(max_docs, max_chars) shard_results Per-shard results (see merge() for details). max_docs Maximum documents to include in prompt (0 = all). max_chars Character budget (0 = unlimited). If specified, last document is truncated to fit budget. Formatted prompt context string with "[Shard: X] content" per document. std::runtime_error (see merge() exception contract)

#### `std::vector< RetrievedDocument > mergeRRF(const std::vector< ShardRetrievalResult > &results) const`
- Source: `include/distributed_knowledge/federated_rag_merger.h`:416
- Brief: Merge using Reciprocal Rank Fusion (RRF).
- Parameters:
  - `results` (const std::vector< ShardRetrievalResult > &): Pre-filtered shard results (may include timed-out or failed entries)
- Return: Ranked document list by RRF score (ties stable-sorted by input order)
- Details: Algorithm: For each shard sr with sr.ok && !sr.timed_out: For each document doc at rank r in sr.documents: rrf_score += 1.0 / (rrf_constant + r) Optionally boost score if sr.adapter_accuracy_delta > 0 Sort by rrf_score descending (stable sort for ties) Conflict Resolution (multiple shards return same doc_id): RRF scores are accumulated: if shard1 ranks doc_id at position 3 and shard2 ranks same doc_id at position 5, their RRF contributions add Result: duplicates automatically ranked higher (due to accumulated score) Tie-Breaking (identical RRF scores): Stable sort preserves input order Later occurrences stay later in output (when scores equal) Specialisation Boost: If adapter_accuracy_delta > 0 (specialised shard), multiply RRF by specialisation_boost Default boost = 1.2 (20% advantage for specialized shards) Failure Handling: Skips shards with sr.ok == false (malformed response, partial timeout) Skips shards with sr.timed_out == true (exceeded DK-OR deadline) Returns partial merge if some shards failed results Pre-filtered shard results (may include timed-out or failed entries) Ranked document list by RRF score (ties stable-sorted by input order)

#### `std::vector< RetrievedDocument > mergeRoundRobin(const std::vector< ShardRetrievalResult > &results) const`
- Source: `include/distributed_knowledge/federated_rag_merger.h`:475
- Brief: Merge using round-robin interleaving for diversity.
- Parameters:
  - `results` (const std::vector< ShardRetrievalResult > &): Pre-filtered shard results
- Return: Ranked document list (interleaved by position across shards)
- Details: Algorithm: Collect all non-empty, non-timed-out shard result lists pos = 0 While (any shard has doc at pos): For each shard in order: If shard.documents[pos] exists: append to merged ++pos Limit total size to ~top_k * 2 (before dedup/trim) Conflict Resolution: Multiple shards at same position → both included (interleaved) No score computation; rank order from merge determines final ranking Tie-Breaking: Shard order in results determines interleave order Deterministic: same input always produces same output Failure Handling: Skips timed-out or failed shards (see mergeRRF) results Pre-filtered shard results Ranked document list (interleaved by position across shards)

#### `std::vector< RetrievedDocument > mergeScoreWeighted(const std::vector< ShardRetrievalResult > &results) const`
- Source: `include/distributed_knowledge/federated_rag_merger.h`:446
- Brief: Merge using score-weighted accumulation.
- Parameters:
  - `results` (const std::vector< ShardRetrievalResult > &): Pre-filtered shard results
- Return: Ranked document list by weighted score
- Details: Algorithm: For each shard sr with sr.ok && !sr.timed_out: weight = 1.0 + (boost_specialised ? sr.adapter_accuracy_delta : 0.0) For each document doc: weighted_score += doc.relevance_score * max(0.01, weight) Sort by weighted_score descending (stable sort for ties) Conflict Resolution (multiple shards return same doc_id): Scores are weighted-summed: doc's final score = Σ_i (score_i × weight_i) Multi-occurrence docs automatically ranked higher Weights reflect shard accuracy (adapter_accuracy_delta from capability announcement) Tie-Breaking: When weighted scores equal, stable sort preserves input order Weight Clamping: Weight has minimum 0.01 to avoid score collapse for untrusted shards Failure Handling: Skips timed-out or failed shards (see mergeRRF) results Pre-filtered shard results Ranked document list by weighted score

### themis::distributed_knowledge::FederatedRAGMergerConfig

#### `bool isValid() const`
- Source: `include/distributed_knowledge/federated_rag_merger.h`:217
- Brief: Validate configuration for constructor.
- Parameters: none
- Details: Returns true iff: top_k > 0 rrf_constant > 0.0 specialisation_boost >= 1.0

### themis::distributed_knowledge::FederationConfig

#### `bool isValid() const`
- Source: `include/distributed_knowledge/lora_federation_coordinator.h`:118
- Brief: n/a
- Parameters: none

### themis::distributed_knowledge::FeedbackSummary

#### `FeedbackSummary fromJson(const nlohmann::json &j)`
- Source: `include/distributed_knowledge/cross_shard_feedback_sync.h`:52
- Brief: n/a
- Parameters:
  - `j` (const nlohmann::json &): n/a

#### `nlohmann::json toJson() const`
- Source: `include/distributed_knowledge/cross_shard_feedback_sync.h`:39
- Brief: n/a
- Parameters: none

### themis::distributed_knowledge::FeedbackSyncConfig

#### `bool isValid() const`
- Source: `include/distributed_knowledge/cross_shard_feedback_sync.h`:77
- Brief: n/a
- Parameters: none

### themis::distributed_knowledge::GlobalAdapterDelta

#### `GlobalAdapterDelta fromJson(const nlohmann::json &j)`
- Source: `include/distributed_knowledge/lora_federation_coordinator.h`:77
- Brief: n/a
- Parameters:
  - `j` (const nlohmann::json &): n/a

#### `nlohmann::json toJson() const`
- Source: `include/distributed_knowledge/lora_federation_coordinator.h`:68
- Brief: n/a
- Parameters: none

### themis::distributed_knowledge::GossipAdapterPublisher

#### `GossipAdapterPublisher(GossipAdapterPublisher &&) noexcept=default`
- Source: `include/distributed_knowledge/adapter_capability_announcement.h`:168
- Brief: n/a
- Parameters:
  - `<unnamed>` (GossipAdapterPublisher &&): n/a

#### `GossipAdapterPublisher(const GossipAdapterPublisher &)=delete`
- Source: `include/distributed_knowledge/adapter_capability_announcement.h`:166
- Brief: n/a
- Parameters:
  - `<unnamed>` (const GossipAdapterPublisher &): n/a

#### `GossipAdapterPublisher(std::string local_shard_id, std::function< void(nlohmann::json)> gossip_message_fn)`
- Source: `include/distributed_knowledge/adapter_capability_announcement.h`:160
- Brief: n/a
- Parameters:
  - `local_shard_id` (std::string): n/a
  - `gossip_message_fn` (std::function< void(nlohmann::json)>): n/a

#### `void announce(AdapterCapabilityAnnouncement announcement)`
- Source: `include/distributed_knowledge/adapter_capability_announcement.h`:175
- Brief: Announce.
- Parameters:
  - `announcement` (AdapterCapabilityAnnouncement): Input parameter.
- Details: announcement Input parameter. announcement Input parameter. Calls: std::chrono::system_clock::now(), toJson(), lk(), gossip_message_fn_(), std::move().

#### `themis::governance::StoreErasureResult erase(const std::string &subject_id="", themis::governance::Regulation regulation=themis::governance::Regulation::GDPR)`
- Source: `include/distributed_knowledge/adapter_capability_announcement.h`:191
- Brief: ───────────────────────────────────────────────────────────────────────────── DK-OR: GDPR erase ─────────────────────────────────────────────────────────────────────────────
- Parameters:
  - `subject_id` (const std::string &): n/a
  - `regulation` (themis::governance::Regulation): n/a
- Return: Return value.
- Details: param Input parameter. Regulation Input parameter. Return value.

#### `size_t eraseCount() const`
- Source: `include/distributed_knowledge/adapter_capability_announcement.h`:195
- Brief: n/a
- Parameters: none

#### `void handleInboundMessage(const nlohmann::json &payload)`
- Source: `include/distributed_knowledge/adapter_capability_announcement.h`:181
- Brief: Handle Inbound Message.
- Parameters:
  - `payload` (const nlohmann::json &): Input parameter.
- Details: payload Input parameter. payload Input parameter. Calls: AdapterCapabilityAnnouncement::fromJson(), lk(), cb().

#### `std::optional< AdapterCapabilityAnnouncement > lastAnnouncement() const`
- Source: `include/distributed_knowledge/adapter_capability_announcement.h`:189
- Brief: n/a
- Parameters: none

#### `GossipAdapterPublisher & operator=(GossipAdapterPublisher &&) noexcept=default`
- Source: `include/distributed_knowledge/adapter_capability_announcement.h`:169
- Brief: n/a
- Parameters:
  - `<unnamed>` (GossipAdapterPublisher &&): n/a

#### `GossipAdapterPublisher & operator=(const GossipAdapterPublisher &)=delete`
- Source: `include/distributed_knowledge/adapter_capability_announcement.h`:167
- Brief: n/a
- Parameters:
  - `<unnamed>` (const GossipAdapterPublisher &): n/a

#### `void setAnnouncementCallback(AnnouncementCallback cb)`
- Source: `include/distributed_knowledge/adapter_capability_announcement.h`:187
- Brief: Set Announcement Callback.
- Parameters:
  - `cb` (AnnouncementCallback): Input parameter.
- Details: cb Input parameter. cb Input parameter. Calls: lk(), std::move().

#### `~GossipAdapterPublisher()`
- Source: `include/distributed_knowledge/adapter_capability_announcement.h`:164
- Brief: n/a
- Parameters: none

### themis::distributed_knowledge::IDKDiagnosticListener

#### `void onEvent(const DKDiagnosticEvent &event)=0`
- Source: `include/distributed_knowledge/dk_diagnostic_emitter.h`:66
- Brief: On Event.
- Parameters:
  - `event` (const DKDiagnosticEvent &): Input parameter.
- Details: event Input parameter.

#### `~IDKDiagnosticListener()=default`
- Source: `include/distributed_knowledge/dk_diagnostic_emitter.h`:60
- Brief: IDKDiagnostic Listener.
- Parameters: none
- Return: Return value.
- Details: Return value.

### themis::distributed_knowledge::IFederatedDistillationCoordinator

#### `DistillationRound broadcastToStudents()=0`
- Source: `include/distributed_knowledge/federated_distillation_coordinator.h`:178
- Brief: Broadcast To Students.
- Parameters: none
- Return: Return value.
- Details: Return value.

#### `uint64_t currentRound() const =0`
- Source: `include/distributed_knowledge/federated_distillation_coordinator.h`:180
- Brief: n/a
- Parameters: none

#### `nlohmann::json getStats() const =0`
- Source: `include/distributed_knowledge/federated_distillation_coordinator.h`:186
- Brief: n/a
- Parameters: none

#### `std::optional< DistillationRound > lastRound() const =0`
- Source: `include/distributed_knowledge/federated_distillation_coordinator.h`:184
- Brief: n/a
- Parameters: none

#### `void registerStudent(const std::string &student_id, std::function< void(const DistillationRound &)> cb)=0`
- Source: `include/distributed_knowledge/federated_distillation_coordinator.h`:188
- Brief: n/a
- Parameters:
  - `student_id` (const std::string &): n/a
  - `cb` (std::function< void(const DistillationRound &)>): n/a

#### `void submitSoftLabels(const std::string &teacher_id, std::vector< SoftLabel > labels)=0`
- Source: `include/distributed_knowledge/federated_distillation_coordinator.h`:171
- Brief: Submit Soft Labels.
- Parameters:
  - `teacher_id` (const std::string &): Identifier of the teacher.
  - `labels` (std::vector< SoftLabel >): Input parameter.
- Details: teacher_id Identifier of the teacher. labels Input parameter.

#### `size_t submittedCount() const =0`
- Source: `include/distributed_knowledge/federated_distillation_coordinator.h`:182
- Brief: n/a
- Parameters: none

#### `~IFederatedDistillationCoordinator() noexcept=default`
- Source: `include/distributed_knowledge/federated_distillation_coordinator.h`:164
- Brief: IFederated Distillation Coordinator.
- Parameters: none
- Return: Return value.
- Details: Return value. Exception safety: noexcept.

### themis::distributed_knowledge::IFederationTrustPolicy

#### `TrustDecision evaluateTrustGate(const AdapterCapabilityAnnouncement &announcement) const noexcept=0`
- Source: `include/distributed_knowledge/distributed_knowledge_api_contract.h`:271
- Brief: n/a
- Parameters:
  - `announcement` (const AdapterCapabilityAnnouncement &): n/a

#### `~IFederationTrustPolicy()=default`
- Source: `include/distributed_knowledge/distributed_knowledge_api_contract.h`:269
- Brief: IFederation Trust Policy.
- Parameters: none
- Return: Return value.
- Details: Return value.

### themis::distributed_knowledge::ILoRAFederationCoordinator

#### `uint64_t currentRound() const =0`
- Source: `include/distributed_knowledge/lora_federation_coordinator.h`:161
- Brief: n/a
- Parameters: none

#### `nlohmann::json getStats() const =0`
- Source: `include/distributed_knowledge/lora_federation_coordinator.h`:167
- Brief: n/a
- Parameters: none

#### `std::optional< GlobalAdapterDelta > lastDelta() const =0`
- Source: `include/distributed_knowledge/lora_federation_coordinator.h`:165
- Brief: n/a
- Parameters: none

#### `void setGlobalDeltaCallback(std::function< void(const GlobalAdapterDelta &)> cb)=0`
- Source: `include/distributed_knowledge/lora_federation_coordinator.h`:158
- Brief: n/a
- Parameters:
  - `cb` (std::function< void(const GlobalAdapterDelta &)>): n/a

#### `void submitGradient(const EncryptedGradient &gradient)=0`
- Source: `include/distributed_knowledge/lora_federation_coordinator.h`:143
- Brief: Submit Gradient.
- Parameters:
  - `gradient` (const EncryptedGradient &): Input parameter.
- Details: gradient Input parameter.

#### `size_t submittedCount() const =0`
- Source: `include/distributed_knowledge/lora_federation_coordinator.h`:163
- Brief: n/a
- Parameters: none

#### `GlobalAdapterDelta triggerAggregation()=0`
- Source: `include/distributed_knowledge/lora_federation_coordinator.h`:149
- Brief: Trigger Aggregation.
- Parameters: none
- Return: Return value.
- Details: Return value.

#### `GlobalAdapterDelta triggerAggregation(size_t timeout_ms)=0`
- Source: `include/distributed_knowledge/lora_federation_coordinator.h`:156
- Brief: Trigger Aggregation.
- Parameters:
  - `timeout_ms` (size_t): Input parameter.
- Return: Return value.
- Details: timeout_ms Input parameter. Return value.

#### `~ILoRAFederationCoordinator()=default`
- Source: `include/distributed_knowledge/lora_federation_coordinator.h`:137
- Brief: ILo RAFederation Coordinator.
- Parameters: none
- Return: Return value.
- Details: Return value.

### themis::distributed_knowledge::LoRAFederationCoordinator

#### `LoRAFederationCoordinator(FederationConfig config={})`
- Source: `include/distributed_knowledge/lora_federation_coordinator.h`:176
- Brief: n/a
- Parameters:
  - `config` (FederationConfig): n/a

#### `LoRAFederationCoordinator(LoRAFederationCoordinator &&) noexcept`
- Source: `include/distributed_knowledge/lora_federation_coordinator.h`:181
- Brief: n/a
- Parameters:
  - `other` (LoRAFederationCoordinator &&): n/a

#### `LoRAFederationCoordinator(const LoRAFederationCoordinator &)=delete`
- Source: `include/distributed_knowledge/lora_federation_coordinator.h`:179
- Brief: n/a
- Parameters:
  - `<unnamed>` (const LoRAFederationCoordinator &): n/a

#### `void advanceRound()`
- Source: `include/distributed_knowledge/lora_federation_coordinator.h`:201
- Brief: ── Extra: manual round control ──────────────────────────────────────────
- Parameters: none
- Details: Advance Round. Calls: lk(), clear().

#### `nlohmann::json applyDifferentialPrivacy(const nlohmann::json &aggregated) const`
- Source: `include/distributed_knowledge/lora_federation_coordinator.h`:293
- Brief: n/a
- Parameters:
  - `aggregated` (const nlohmann::json &): n/a

#### `const FederationConfig & config() const`
- Source: `include/distributed_knowledge/lora_federation_coordinator.h`:210
- Brief: n/a
- Parameters: none

#### `uint64_t currentRound() const override`
- Source: `include/distributed_knowledge/lora_federation_coordinator.h`:192
- Brief: n/a
- Parameters: none

#### `GlobalAdapterDelta doAggregation()`
- Source: `include/distributed_knowledge/lora_federation_coordinator.h`:292
- Brief: ───────────────────────────────────────────────────────────────────────────── doAggregation (internal, caller holds mutex_) ─────────────────────────────────────────────────────────────────────────────
- Parameters: none
- Return: Return value.
- Throws:
  - std::runtime_error: if an error occurs.
- Details: Return value. std::runtime_error if an error occurs. Calls: begin(), end(), gradient_outlier_filter_(), erase(), size(), std::to_string(), is_object(), items().

#### `void emitFederationDecisionRecord(const GlobalAdapterDelta &delta, const std::string &outcome) const`
- Source: `include/distributed_knowledge/lora_federation_coordinator.h`:301
- Brief: Emit Federation Decision Record.
- Parameters:
  - `delta` (const GlobalAdapterDelta &): Input parameter.
  - `outcome` (const std::string &): Input parameter.
- Details: delta Input parameter. outcome Input parameter.

#### `themis::governance::StoreErasureResult erase(const std::string &subject_id="", themis::governance::Regulation regulation=themis::governance::Regulation::GDPR)`
- Source: `include/distributed_knowledge/lora_federation_coordinator.h`:220
- Brief: Erase.
- Parameters:
  - `subject_id` (const std::string &): n/a
  - `regulation` (themis::governance::Regulation): n/a
- Return: Return value.
- Details: param Input parameter. Regulation Input parameter. Return value. Calls: lk(), clear().

#### `size_t eraseCount() const`
- Source: `include/distributed_knowledge/lora_federation_coordinator.h`:224
- Brief: n/a
- Parameters: none

#### `size_t filteredGradientsCount() const`
- Source: `include/distributed_knowledge/lora_federation_coordinator.h`:254
- Brief: n/a
- Parameters: none

#### `nlohmann::json getStats() const override`
- Source: `include/distributed_knowledge/lora_federation_coordinator.h`:195
- Brief: n/a
- Parameters: none

#### `std::optional< GlobalAdapterDelta > lastDelta() const override`
- Source: `include/distributed_knowledge/lora_federation_coordinator.h`:194
- Brief: n/a
- Parameters: none

#### `GradientOutlierFilter makeL2NormOutlierFilter(double z_threshold=2.5)`
- Source: `include/distributed_knowledge/lora_federation_coordinator.h`:256
- Brief: n/a
- Parameters:
  - `z_threshold` (double): n/a

#### `std::string nextDeltaVersion() const`
- Source: `include/distributed_knowledge/lora_federation_coordinator.h`:295
- Brief: n/a
- Parameters: none

#### `LoRAFederationCoordinator & operator=(LoRAFederationCoordinator &&) noexcept`
- Source: `include/distributed_knowledge/lora_federation_coordinator.h`:182
- Brief: n/a
- Parameters:
  - `other` (LoRAFederationCoordinator &&): n/a

#### `LoRAFederationCoordinator & operator=(const LoRAFederationCoordinator &)=delete`
- Source: `include/distributed_knowledge/lora_federation_coordinator.h`:180
- Brief: n/a
- Parameters:
  - `<unnamed>` (const LoRAFederationCoordinator &): n/a

#### `double privacyBudgetRemaining() const`
- Source: `include/distributed_knowledge/lora_federation_coordinator.h`:214
- Brief: n/a
- Parameters: none

#### `void setAuditRecordCallback(std::function< void(const nlohmann::json &)> callback)`
- Source: `include/distributed_knowledge/lora_federation_coordinator.h`:236
- Brief: n/a
- Parameters:
  - `callback` (std::function< void(const nlohmann::json &)>): n/a

#### `void setCrossBorderPolicy(std::shared_ptr< themis::governance::CrossBorderTransferPolicy > policy)`
- Source: `include/distributed_knowledge/lora_federation_coordinator.h`:231
- Brief: ── DK-7: Admin, GDPR, and audit hooks ──────────────────────────────────
- Parameters:
  - `policy` (std::shared_ptr< themis::governance::CrossBorderTransferPolicy >): Input parameter.
- Details: ───────────────────────────────────────────────────────────────────────────── DK-7: GDPR + Audit + SphincsPlus DI setters ───────────────────────────────────────────────────────────────────────────── policy Input parameter. policy Input parameter. Calls: lk(), std::move().

#### `void setDecisionRecordProcessor(std::shared_ptr< themis::llm::DecisionRecordYamlProcessor > processor)`
- Source: `include/distributed_knowledge/lora_federation_coordinator.h`:207
- Brief: Set Decision Record Processor.
- Parameters:
  - `processor` (std::shared_ptr< themis::llm::DecisionRecordYamlProcessor >): Input parameter.
- Details: processor Input parameter. processor Input parameter. Calls: lk(), std::move().

#### `void setGlobalDeltaCallback(std::function< void(const GlobalAdapterDelta &)> cb) override`
- Source: `include/distributed_knowledge/lora_federation_coordinator.h`:189
- Brief: n/a
- Parameters:
  - `cb` (std::function< void(const GlobalAdapterDelta &)>): n/a

#### `void setGradientOutlierFilter(GradientOutlierFilter filter)`
- Source: `include/distributed_knowledge/lora_federation_coordinator.h`:252
- Brief: Set Gradient Outlier Filter.
- Parameters:
  - `filter` (GradientOutlierFilter): Input parameter.
- Details: ───────────────────────────────────────────────────────────────────────────── FPD: Gradient Outlier / Poisoning Detection ───────────────────────────────────────────────────────────────────────────── filter Input parameter. filter Input parameter. Calls: lk(), std::move().

#### `void setShardLocations(std::map< std::string, std::string > locations)`
- Source: `include/distributed_knowledge/lora_federation_coordinator.h`:234
- Brief: n/a
- Parameters:
  - `locations` (std::map< std::string, std::string >): n/a

#### `void setSigningCallback(std::function< std::string(const nlohmann::json &)> signing_fn)`
- Source: `include/distributed_knowledge/lora_federation_coordinator.h`:239
- Brief: n/a
- Parameters:
  - `signing_fn` (std::function< std::string(const nlohmann::json &)>): n/a

#### `void submitGradient(const EncryptedGradient &gradient) override`
- Source: `include/distributed_knowledge/lora_federation_coordinator.h`:186
- Brief: Submit Gradient.
- Parameters:
  - `gradient` (const EncryptedGradient &): Input parameter.
- Details: gradient Input parameter. Calls: lk(), count(), size(), doAggregation(), std::max(), has_value(), std::move().

#### `size_t submittedCount() const override`
- Source: `include/distributed_knowledge/lora_federation_coordinator.h`:193
- Brief: n/a
- Parameters: none

#### `GlobalAdapterDelta triggerAggregation() override`
- Source: `include/distributed_knowledge/lora_federation_coordinator.h`:187
- Brief: Trigger Aggregation.
- Parameters: none
- Return: Return value.
- Throws:
  - std::runtime_error: if an error occurs.
- Details: Return value. std::runtime_error if an error occurs. Calls: lk(), std::to_string(), empty(), has_value(), find(), end(), checkTransfer(), size().

#### `GlobalAdapterDelta triggerAggregation(size_t timeout_ms) override`
- Source: `include/distributed_knowledge/lora_federation_coordinator.h`:188
- Brief: ───────────────────────────────────────────────────────────────────────────── DK-OR: Operational Resilience — timeout overload, erase ─────────────────────────────────────────────────────────────────────────────
- Parameters:
  - `timeout_ms` (size_t): Input parameter.
- Return: Return value.
- Throws:
  - std::runtime_error: if an error occurs.
- Details: timeout_ms Input parameter. Return value. std::runtime_error if an error occurs. Calls: std::async(), wait_for(), std::chrono::milliseconds(), std::to_string(), get().

#### `bool verifyPrivacyBudget() const`
- Source: `include/distributed_knowledge/lora_federation_coordinator.h`:216
- Brief: n/a
- Parameters: none

#### `~LoRAFederationCoordinator() noexcept override`
- Source: `include/distributed_knowledge/lora_federation_coordinator.h`:177
- Brief: n/a
- Parameters: none

### themis::distributed_knowledge::MergeHardeningPolicy

#### `bool isConstrained() const noexcept`
- Source: `include/distributed_knowledge/distributed_knowledge_api_contract.h`:219
- Brief: n/a
- Parameters: none

### themis::distributed_knowledge::MergedRAGContext

#### `std::string buildPromptContext(size_t max_docs=0, size_t max_chars=0) const`
- Source: `include/distributed_knowledge/federated_rag_merger.h`:130
- Brief: Build a compact text context suitable for LLM prompt injection.
- Parameters:
  - `max_docs` (size_t): Maximum documents to include (default: 0 = all). If 0, include all documents (subject to max_chars).
  - `max_chars` (size_t): Approximate character budget (default: 0 = unlimited). If > 0, truncate to stay within budget; last doc may be shortened with "..." suffix.
- Return: Formatted prompt context string, ready for LLM injection. Empty string if documents list is empty.
- Details: Formatting: Each document formatted as: "[Shard: <shard_id>] <content>\n Entities: <entities>\n\n" Entities appended if metadata["_entities"] is non-empty Joins documents with "\n\n" separator Truncation Contract: If max_docs > 0: stop after including max_docs documents If max_chars > 0: stop if appending next doc exceeds budget Last document is truncated (if necessary) to fit max_chars, append "..." If remaining budget < 20 chars: skip last document entirely Determinism: Output order matches input documents ranking Identical inputs → identical output (idempotent) Use Case: After merge(), call buildPromptContext() to create RAG context for LLM Example: buildPromptContext(10, 4000) → top-10 docs, max 4KB max_docs Maximum documents to include (default: 0 = all). If 0, include all documents (subject to max_chars). max_chars Approximate character budget (default: 0 = unlimited). If > 0, truncate to stay within budget; last doc may be shortened with "..." suffix. Formatted prompt context string, ready for LLM injection. Empty string if documents list is empty.

### themis::distributed_knowledge::RetrievedDocument

#### `nlohmann::json toJson() const`
- Source: `include/distributed_knowledge/federated_rag_merger.h`:53
- Brief: n/a
- Parameters: none

### themis::distributed_knowledge::SoftLabel

#### `SoftLabel fromJson(const nlohmann::json &j)`
- Source: `include/distributed_knowledge/federated_distillation_coordinator.h`:45
- Brief: n/a
- Parameters:
  - `j` (const nlohmann::json &): n/a

#### `nlohmann::json toJson() const`
- Source: `include/distributed_knowledge/federated_distillation_coordinator.h`:38
- Brief: n/a
- Parameters: none

