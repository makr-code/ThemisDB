# REPLICATION DOXYGEN

Author: ThemisDB Contributors
Created: 2026-09-20
Last Updated: 2026-09-20
Status: active

## Source
- This file is generated from module-scoped Doxygen XML.
- XML index: `C:\Projects\ThemisDB\.tmp\module_doxygen_global_e2e\replication\xml\index.xml`
- Warnings log: `C:\Projects\ThemisDB\.tmp\module_doxygen_global_e2e\replication\doxygen-warnings.log`

## Structure Metrics
- C/C++ Files (scanned): 53
- Compounds: 303
- Classes/Structs: 211
- Namespaces: 31
- File Compounds: 53

## Namespaces
- @205126211214073101101150065172303340110247374174
- @212345002217151070223151232003161261171270150203
- @217311333162210365072041365035313260355347372131
- @264161173061050203010173331334171017160076312132
- @311320147230175333302033267022212101344206004335
- @312022346272341366023321346230351174353154312042
- benchmark
- std
- std::chrono_literals
- testing
- themis
- themis::bench
- themis::bench::rmd
- themis::bench::rrg
- themis::bench::wave_d
- themis::bench::wave_d::@160117142105152031175005050126170326157273066250
- themis::replication
- themis::replication::test
- themis::server
- themis::sharding
- themis::test
- themisdb
- themisdb::replication
- themisdb::replication::@005330206161360251167046317022110211210342353114
- themisdb::replication::@073011252212221063103360154262233364233253054375
- themisdb::replication::@176301166312177324042016047117236254335132322351
- themisdb::replication::@262003162167317076054137006175316350063012200242
- themisdb::replication::@271347326313165332142325020050227347224131235070
- themisdb::replication::@346330024233145367130007374234033035220334131171
- themisdb::replication::crdt
- themisdb::replication::test

## Types
### Classes
- AsyncWalShipperTest
- CRDTResolverTest
- CrossClusterPublicationTest
- CrossClusterSubscriptionTest
- EventStreamTest
- FieldLevelMergeTest
- GeoPlacementTest
- LWWResolverTest
- LeaderElectionTest
- MCMFollowerTest
- MCMTest
- ObservabilityTest
- ParallelReplicationWorkerTest
- PersistentStateTest
- PublicationFilterTest
- RaftV2ConfigBasicTest
- RaftV2JointConsensusAddTest
- RaftV2JointConsensusRemoveTest
- RaftV2JointQuorumTest
- ReplicationConfigTest
- ReplicationCoordinatorTest
- ReplicationManagerAddReplicaTest
- ReplicationPolicyTest
- ReplicationSlotTest
- ReplicationStreamCompressionTest
- ReplicationTopologyApiHandlerNoReplTest
- ReplicationTopologyApiHandlerWithReplTest
- StressBPQueue
- StressCDCPublisher
- StressReplicationSlot
- ThreeWayMergeTest
- WALChecksumTest
- WalBenchFixture
- WitnessNodeTest
- themis::replication::IKafkaChangeStreamExporter
- themis::test::MockReplica
- themis::test::MockReplicationSystem
- themis::test::ReplicationChaosTest
- themisdb::replication::AdvancedConflictResolver
- themisdb::replication::AsyncWalShipper
- themisdb::replication::BatchedAckTracker
- themisdb::replication::BidirectionalReplicationManager
- themisdb::replication::CDCManager
- themisdb::replication::CRDTConflictResolver
- themisdb::replication::CRDTMergeResolver
- themisdb::replication::CompressedReplicationStream
- themisdb::replication::ConflictResolver
- themisdb::replication::CrossClusterPublication
- themisdb::replication::CrossClusterSubscription
- themisdb::replication::CustomResolver
- themisdb::replication::FieldLevelMergeResolver
- themisdb::replication::GeoReplicaPlacementManager
- themisdb::replication::GeoReplicationManager
- themisdb::replication::HybridLogicalClock
- themisdb::replication::IArchivalBackend
- themisdb::replication::IConflictResolver
- themisdb::replication::IReplicationListener
- themisdb::replication::LWWConflictResolver
- themisdb::replication::LagAlertManager
- themisdb::replication::LagBasedReadRouter
- themisdb::replication::LastWriteWinsResolver
- themisdb::replication::LeaderElection
- themisdb::replication::LogicalReplicationManager
- themisdb::replication::MembershipChangeManager
- themisdb::replication::MultiMasterReplicationManager
- themisdb::replication::MultiRegionActiveActiveManager
- themisdb::replication::MultiTierReplicationManager
- themisdb::replication::ParallelReplicationWorker
- themisdb::replication::PersistentReplicationState
- themisdb::replication::QuorumReadManager
- themisdb::replication::RaftV2ClusterConfig
- themisdb::replication::ReplicationAnalytics
- themisdb::replication::ReplicationBenchmark
- themisdb::replication::ReplicationEventStream
- themisdb::replication::ReplicationEventStream::Subscription
- themisdb::replication::ReplicationManager
- themisdb::replication::ReplicationObserver
- themisdb::replication::ReplicationPolicy
- themisdb::replication::ReplicationSlot
- themisdb::replication::ReplicationSlotManager
- themisdb::replication::ReplicationStream
- themisdb::replication::SchemaAwareCDCBridge
- themisdb::replication::ThreeWayMergeResolver
- themisdb::replication::VectorClock
- themisdb::replication::WALArchivalManager
- themisdb::replication::WALManager
- themisdb::replication::crdt::DisableWinsFlag
- themisdb::replication::crdt::EnableWinsFlag
- themisdb::replication::crdt::GrowOnlyCounter
- themisdb::replication::crdt::GrowOnlySet
- themisdb::replication::crdt::LWWMap
- themisdb::replication::crdt::LWWRegister
- themisdb::replication::crdt::MVRegister
- themisdb::replication::crdt::ORSet
- themisdb::replication::crdt::PNCounter
- themisdb::replication::crdt::RGArray
- themisdb::replication::crdt::TwoPSet
- themisdb::replication::test::ConflictContextTest
- themisdb::replication::test::DeterministicConflictResolutionTest
- themisdb::replication::test::DiagnosticsConsistencyTest
- themisdb::replication::test::EdgeCaseConflictResolutionTest
- themisdb::replication::test::FieldLevelMergeResolverTest
- themisdb::replication::test::ThreeWayMergeResolverTest

### Structs
- BPQueue
- TempWALDir
- themis::bench::rrg::LwwVersion
- themis::replication::KafkaChangeStreamConfig
- themis::replication::KafkaProducerConfig
- themis::replication::KafkaStreamStats
- themis::replication::test::MockGCounter
- themis::replication::test::MockVersion
- themisdb::replication::AdvancedConflictResolver::ResolutionContext
- themisdb::replication::AlertEvent
- themisdb::replication::BatchedAckTracker::AckBatch
- themisdb::replication::BatchedAckTracker::AckBatchConfig
- themisdb::replication::BatchedAckTracker::Stats
- themisdb::replication::BidirectionalReplicationManager::BidiConfig
- themisdb::replication::BidirectionalReplicationManager::BidiConflictRecord
- themisdb::replication::BidirectionalReplicationManager::BidiWriteEntry
- themisdb::replication::BidirectionalReplicationManager::OriginInfo
- themisdb::replication::BidirectionalReplicationManager::SyncStatus
- themisdb::replication::CDCManager::Subscription
- themisdb::replication::CollectionAccessStats
- themisdb::replication::CompressedReplicationStream::CompressionConfig
- themisdb::replication::CompressedReplicationStream::CompressionStats
- themisdb::replication::ConflictRecord
- themisdb::replication::ConsensusHealthDiagnostic
- themisdb::replication::CrossClusterPublication::RemoteSubscriber
- themisdb::replication::FailoverCandidateDiagnostic
- themisdb::replication::FailoverCandidateDiagnostic::EvaluationSteps
- themisdb::replication::FailoverExecutionDiagnostic
- themisdb::replication::FailoverExecutionDiagnostic::Event
- themisdb::replication::GeoReplicationManager::GeoConfig
- themisdb::replication::HybridLogicalClock::Timestamp
- themisdb::replication::LagAlertManager::ReplicaState
- themisdb::replication::LagBasedReadRouter::RouterConfig
- themisdb::replication::LagBasedReadRouter::RoutingDecision
- themisdb::replication::LogicalChange
- themisdb::replication::LogicalReplicationManager::Config
- themisdb::replication::LogicalReplicationManager::LogicalReplicationSlot
- themisdb::replication::LogicalReplicationManager::ReplicationFilter
- themisdb::replication::LogicalReplicationManager::SlotRuntime
- themisdb::replication::LogicalReplicationManager::Stats
- themisdb::replication::MMPeerInfo
- themisdb::replication::MMReplicationConfig
- themisdb::replication::MMWriteEntry
- themisdb::replication::MembershipChangeEntry
- themisdb::replication::MultiMasterReplicationManager::ReadResult
- themisdb::replication::MultiMasterReplicationManager::Stats
- themisdb::replication::MultiMasterReplicationManager::TopologyEdge
- themisdb::replication::MultiMasterReplicationManager::TopologyNode
- themisdb::replication::MultiMasterReplicationManager::TopologySnapshot
- themisdb::replication::MultiRegionActiveActiveConfig
- themisdb::replication::MultiRegionActiveActiveManager::ReadResult
- themisdb::replication::MultiRegionActiveActiveManager::WriteResult
- themisdb::replication::MultiTierConfig
- themisdb::replication::MultiTierStats
- themisdb::replication::ParallelReplicationWorker::ParallelConfig
- themisdb::replication::ParallelReplicationWorker::Stats
- themisdb::replication::ParallelReplicationWorker::WorkItem
- themisdb::replication::PersistentReplicationState::State
- themisdb::replication::PlacementConstraints
- themisdb::replication::PlacementValidationResult
- themisdb::replication::PromotionEligibilityAnalysis
- themisdb::replication::PromotionEligibilityAnalysis::Criterion
- themisdb::replication::PublicationFilter
- themisdb::replication::QuorumReadManager::QuorumReadConfig
- themisdb::replication::QuorumReadManager::QuorumReadResult
- themisdb::replication::QuorumReadManager::ReplicaResponse
- themisdb::replication::RaftV2State
- themisdb::replication::RegionStalenessInfo
- themisdb::replication::ReplicaHealthTransitionDiagnostic
- themisdb::replication::ReplicaInfo
- themisdb::replication::ReplicaLag
- themisdb::replication::ReplicationAnalytics::AnalyticsConfig
- themisdb::replication::ReplicationAnalytics::Bottleneck
- themisdb::replication::ReplicationAnalytics::Insight
- themisdb::replication::ReplicationAnalytics::LagDataPoint
- themisdb::replication::ReplicationAnalytics::LagHistory
- themisdb::replication::ReplicationBenchmark::BenchmarkConfig
- themisdb::replication::ReplicationBenchmark::BenchmarkResult
- themisdb::replication::ReplicationConfig
- themisdb::replication::ReplicationEventStream::Event
- themisdb::replication::ReplicationEventStream::SubscriptionRecord
- themisdb::replication::ReplicationEventStreamConfig
- themisdb::replication::ReplicationManager::LeaseReadResult
- themisdb::replication::ReplicationObserver::Bottleneck
- themisdb::replication::ReplicationObserver::HealthScore
- themisdb::replication::ReplicationObserver::LagSnapshot
- themisdb::replication::ReplicationObserver::TopologyNode
- themisdb::replication::ReplicationObserverConfig
- themisdb::replication::ReplicationPolicy::Policy
- themisdb::replication::ReplicationPolicy::ValidationResult
- themisdb::replication::ReplicationSlot::SlotState
- themisdb::replication::ReplicationSlotManager::ManagerConfig
- themisdb::replication::ReplicationStats
- themisdb::replication::SLOThresholds
- themisdb::replication::SchemaAwareCDCBridge::Stats
- themisdb::replication::SchemaAwareCDCBridge::Subscription
- themisdb::replication::SchemaEncodedEvent
- themisdb::replication::TierConfig
- themisdb::replication::WALArchivalManager::ArchivalConfig
- themisdb::replication::WALArchivalManager::ArchivedSegment
- themisdb::replication::WALEntry
- themisdb::replication::WalSegment
- themisdb::replication::WalShippingConfig
- themisdb::replication::WalShippingStats
- themisdb::replication::crdt::Dot
- themisdb::replication::crdt::LWWMap::Slot
- themisdb::replication::crdt::RGArray::Entry
- themisdb::replication::test::StubWAL

## Notes
- This is a derived artifact. Do not manually edit semantic content.
- Regenerate via scripts/generate_module_doxygen_xml.py --write-module-markdown.

## Detailed Function Documentation (Soll-Ist)
- Functions extracted: 1365

### AsyncWalShipperTest

#### `WalShippingConfig defaultConfig()`
- Source: `tests/replication/test_replication_geo_placement_wal_shipping_focused.cpp`:273
- Brief: n/a
- Parameters: none

### BPQueue

#### `bool pop(uint64_t &v)`
- Source: `benchmarks/replication/bench_replication_dedicated_gates.cpp`:69
- Brief: n/a
- Parameters:
  - `v` (uint64_t &): n/a

#### `bool push(uint64_t v)`
- Source: `benchmarks/replication/bench_replication_dedicated_gates.cpp`:63
- Brief: n/a
- Parameters:
  - `v` (uint64_t): n/a

### EventStreamTest

#### `void SetUp() override`
- Source: `tests/replication/test_replication_new_features.cpp`:400
- Brief: n/a
- Parameters: none

### LeaderElectionTest

#### `void SetUp() override`
- Source: `tests/replication/test_replication_ha.cpp`:349
- Brief: n/a
- Parameters: none

#### `void TearDown() override`
- Source: `tests/replication/test_replication_ha.cpp`:356
- Brief: n/a
- Parameters: none

### MCMFollowerTest

#### `void SetUp() override`
- Source: `tests/replication/test_replication_raft_v2.cpp`:349
- Brief: n/a
- Parameters: none

### MCMTest

#### `void SetUp() override`
- Source: `tests/replication/test_replication_raft_v2.cpp`:271
- Brief: n/a
- Parameters: none

### ObservabilityTest

#### `void SetUp() override`
- Source: `tests/replication/test_replication_new_features.cpp`:112
- Brief: n/a
- Parameters: none

#### `void TearDown() override`
- Source: `tests/replication/test_replication_new_features.cpp`:119
- Brief: n/a
- Parameters: none

### ParallelReplicationWorkerTest

#### `ParallelReplicationWorker::ParallelConfig defaultConfig()`
- Source: `tests/replication/test_replication_new_features.cpp`:783
- Brief: n/a
- Parameters: none

### PersistentStateTest

#### `void SetUp() override`
- Source: `tests/replication/test_replication_ha.cpp`:2188
- Brief: n/a
- Parameters: none

#### `void TearDown() override`
- Source: `tests/replication/test_replication_ha.cpp`:2200
- Brief: n/a
- Parameters: none

#### `void cleanupPath()`
- Source: `tests/replication/test_replication_ha.cpp`:2203
- Brief: n/a
- Parameters: none

### RaftV2JointQuorumTest

#### `void SetUp() override`
- Source: `tests/replication/test_replication_raft_v2.cpp`:182
- Brief: n/a
- Parameters: none

### ReplicationCoordinatorTest

#### `void SetUp() override`
- Source: `tests/replication/test_replication_coordinator_focused.cpp`:15
- Brief: n/a
- Parameters: none

### ReplicationManagerAddReplicaTest

#### `void SetUp() override`
- Source: `tests/replication/test_replication_manager_addReplica_simple.cpp`:21
- Brief: n/a
- Parameters: none

#### `void TearDown() override`
- Source: `tests/replication/test_replication_manager_addReplica_simple.cpp`:28
- Brief: n/a
- Parameters: none

### ReplicationPolicyTest

#### `void SetUp() override`
- Source: `tests/replication/test_replication_new_features.cpp`:504
- Brief: n/a
- Parameters: none

#### `void TearDown() override`
- Source: `tests/replication/test_replication_new_features.cpp`:511
- Brief: n/a
- Parameters: none

### ReplicationSlotTest

#### `void SetUp() override`
- Source: `tests/replication/test_replication_new_features.cpp`:590
- Brief: n/a
- Parameters: none

### ReplicationStreamCompressionTest

#### `void SetUp() override`
- Source: `tests/replication/test_replication_ha.cpp`:2623
- Brief: n/a
- Parameters: none

#### `void TearDown() override`
- Source: `tests/replication/test_replication_ha.cpp`:2636
- Brief: n/a
- Parameters: none

### ReplicationTopologyApiHandlerNoReplTest

#### `void SetUp() override`
- Source: `tests/replication/test_replication_topology_api_handler.cpp`:40
- Brief: n/a
- Parameters: none

#### `http::request< http::string_body > makeGet(const std::string &target)`
- Source: `tests/replication/test_replication_topology_api_handler.cpp`:46
- Brief: n/a
- Parameters:
  - `target` (const std::string &): n/a

### ReplicationTopologyApiHandlerWithReplTest

#### `void SetUp() override`
- Source: `tests/replication/test_replication_topology_api_handler.cpp`:93
- Brief: n/a
- Parameters: none

#### `http::request< http::string_body > makeGet(const std::string &target)`
- Source: `tests/replication/test_replication_topology_api_handler.cpp`:98
- Brief: n/a
- Parameters:
  - `target` (const std::string &): n/a

### StressBPQueue

#### `bool pop(uint64_t &out)`
- Source: `tests/replication/test_replication_highcardinality_stress.cpp`:116
- Brief: n/a
- Parameters:
  - `out` (uint64_t &): n/a

#### `bool push(uint64_t v)`
- Source: `tests/replication/test_replication_highcardinality_stress.cpp`:110
- Brief: n/a
- Parameters:
  - `v` (uint64_t): n/a

#### `std::size_t size()`
- Source: `tests/replication/test_replication_highcardinality_stress.cpp`:122
- Brief: n/a
- Parameters: none

### StressCDCPublisher

#### `uint64_t outOfOrder() const noexcept`
- Source: `tests/replication/test_replication_highcardinality_stress.cpp`:92
- Brief: n/a
- Parameters: none

#### `void publish(uint32_t slot_id, uint64_t seq)`
- Source: `tests/replication/test_replication_highcardinality_stress.cpp`:81
- Brief: n/a
- Parameters:
  - `slot_id` (uint32_t): n/a
  - `seq` (uint64_t): n/a

#### `uint64_t total() const noexcept`
- Source: `tests/replication/test_replication_highcardinality_stress.cpp`:95
- Brief: n/a
- Parameters: none

### StressReplicationSlot

#### `StressReplicationSlot(StressReplicationSlot &&other) noexcept`
- Source: `tests/replication/test_replication_highcardinality_stress.cpp`:49
- Brief: n/a
- Parameters:
  - `other` (StressReplicationSlot &&): n/a

#### `StressReplicationSlot(const StressReplicationSlot &)=delete`
- Source: `tests/replication/test_replication_highcardinality_stress.cpp`:46
- Brief: n/a
- Parameters:
  - `<unnamed>` (const StressReplicationSlot &): n/a

#### `StressReplicationSlot(uint64_t start_lsn)`
- Source: `tests/replication/test_replication_highcardinality_stress.cpp`:43
- Brief: n/a
- Parameters:
  - `start_lsn` (uint64_t): n/a

#### `void ack(uint64_t lsn) noexcept`
- Source: `tests/replication/test_replication_highcardinality_stress.cpp`:65
- Brief: n/a
- Parameters:
  - `lsn` (uint64_t): n/a

#### `uint64_t acked() const noexcept`
- Source: `tests/replication/test_replication_highcardinality_stress.cpp`:72
- Brief: n/a
- Parameters: none

#### `uint64_t advance() noexcept`
- Source: `tests/replication/test_replication_highcardinality_stress.cpp`:61
- Brief: n/a
- Parameters: none

#### `uint64_t lsn() const noexcept`
- Source: `tests/replication/test_replication_highcardinality_stress.cpp`:71
- Brief: n/a
- Parameters: none

#### `StressReplicationSlot & operator=(StressReplicationSlot &&other) noexcept`
- Source: `tests/replication/test_replication_highcardinality_stress.cpp`:53
- Brief: n/a
- Parameters:
  - `other` (StressReplicationSlot &&): n/a

#### `StressReplicationSlot & operator=(const StressReplicationSlot &)=delete`
- Source: `tests/replication/test_replication_highcardinality_stress.cpp`:47
- Brief: n/a
- Parameters:
  - `<unnamed>` (const StressReplicationSlot &): n/a

### TempWALDir

#### `TempWALDir(const std::string &p)`
- Source: `tests/replication/test_replication_ha.cpp`:64
- Brief: n/a
- Parameters:
  - `p` (const std::string &): n/a

#### `TempWALDir(const std::string &p)`
- Source: `tests/replication/test_replication_new_features.cpp`:62
- Brief: n/a
- Parameters:
  - `p` (const std::string &): n/a

#### `TempWALDir(const std::string &p)`
- Source: `tests/replication/test_replication_raft_v2.cpp`:54
- Brief: n/a
- Parameters:
  - `p` (const std::string &): n/a

#### `~TempWALDir()`
- Source: `tests/replication/test_replication_ha.cpp`:78
- Brief: n/a
- Parameters: none

#### `~TempWALDir()`
- Source: `tests/replication/test_replication_new_features.cpp`:76
- Brief: n/a
- Parameters: none

#### `~TempWALDir()`
- Source: `tests/replication/test_replication_raft_v2.cpp`:58
- Brief: n/a
- Parameters: none

### WalBenchFixture

#### `void SetUp(const ::benchmark::State &) override`
- Source: `benchmarks/replication/bench_replication_throughput.cpp`:88
- Brief: n/a
- Parameters:
  - `<unnamed>` (const ::benchmark::State &): n/a

#### `void TearDown(const ::benchmark::State &) override`
- Source: `benchmarks/replication/bench_replication_throughput.cpp`:98
- Brief: n/a
- Parameters:
  - `<unnamed>` (const ::benchmark::State &): n/a

### WitnessNodeTest

#### `ReplicationConfig makeWitnessConfig(const std::string &wal_dir)`
- Source: `tests/replication/test_replication_ha.cpp`:4761
- Brief: n/a
- Parameters:
  - `wal_dir` (const std::string &): n/a

### bench_replication_dedicated_gates.cpp

#### `void bench_replication_bp_enqueue_p95()`
- Source: `benchmarks/replication/bench_replication_dedicated_gates.cpp`:98
- Brief: n/a
- Parameters: none

#### `void bench_replication_cdc_publish_throughput()`
- Source: `benchmarks/replication/bench_replication_dedicated_gates.cpp`:165
- Brief: n/a
- Parameters: none

#### `void bench_replication_slot_advance_p95()`
- Source: `benchmarks/replication/bench_replication_dedicated_gates.cpp`:135
- Brief: n/a
- Parameters: none

#### `void bench_replication_wal_enqueue_throughput()`
- Source: `benchmarks/replication/bench_replication_dedicated_gates.cpp`:190
- Brief: n/a
- Parameters: none

#### `int main()`
- Source: `benchmarks/replication/bench_replication_dedicated_gates.cpp`:215
- Brief: n/a
- Parameters: none

#### `ns_t now_ns()`
- Source: `benchmarks/replication/bench_replication_dedicated_gates.cpp`:40
- Brief: n/a
- Parameters: none

#### `double percentile(std::vector< ns_t > &samples, double p)`
- Source: `benchmarks/replication/bench_replication_dedicated_gates.cpp`:46
- Brief: n/a
- Parameters:
  - `samples` (std::vector< ns_t > &): n/a
  - `p` (double): n/a

#### `void stub_cdc_publish(uint32_t) noexcept`
- Source: `benchmarks/replication/bench_replication_dedicated_gates.cpp`:85
- Brief: n/a
- Parameters:
  - `<unnamed>` (uint32_t): n/a

#### `uint64_t stub_slot_advance() noexcept`
- Source: `benchmarks/replication/bench_replication_dedicated_gates.cpp`:79
- Brief: n/a
- Parameters: none

#### `void stub_wal_enqueue(uint64_t) noexcept`
- Source: `benchmarks/replication/bench_replication_dedicated_gates.cpp`:91
- Brief: n/a
- Parameters:
  - `<unnamed>` (uint64_t): n/a

### bench_replication_geo_wal_baselines.cpp

#### `int main()`
- Source: `tests/replication/bench_replication_geo_wal_baselines.cpp`:14
- Brief: n/a
- Parameters: none

### bench_replication_multi_dc_multi_writer.cpp

#### `BENCHMARK_MAIN()`
- Source: `benchmarks/replication/bench_replication_multi_dc_multi_writer.cpp`:448
- Brief: n/a
- Parameters: none

### bench_replication_release_gates.cpp

#### `BENCHMARK_MAIN()`
- Source: `benchmarks/replication/bench_replication_release_gates.cpp`:310
- Brief: n/a
- Parameters: none

### bench_replication_throughput.cpp

#### `Arg(0) -> Arg(50) ->Arg(200) ->Unit(benchmark::kNanosecond)`
- Source: `benchmarks/replication/bench_replication_throughput.cpp`:210
- Brief: n/a
- Parameters:
  - `<unnamed>` (0): n/a

#### `Arg(100) -> Arg(500) ->Arg(1000) ->Unit(benchmark::kMillisecond) ->Iterations(200)`
- Source: `benchmarks/replication/bench_replication_throughput.cpp`:152
- Brief: n/a
- Parameters:
  - `<unnamed>` (100): n/a

#### `Arg(2) -> Arg(8) ->Arg(32) ->Unit(benchmark::kNanosecond)`
- Source: `benchmarks/replication/bench_replication_throughput.cpp`:246
- Brief: n/a
- Parameters:
  - `<unnamed>` (2): n/a

#### `BENCHMARK(BM_WALEntry_Deserialize) -> Unit(benchmark::kNanosecond)`
- Source: `benchmarks/replication/bench_replication_throughput.cpp`:184
- Brief: n/a
- Parameters:
  - `<unnamed>` (BM_WALEntry_Deserialize): n/a

#### `BENCHMARK(BM_WALEntry_Serialize) -> Unit(benchmark::kNanosecond)`
- Source: `benchmarks/replication/bench_replication_throughput.cpp`:171
- Brief: n/a
- Parameters:
  - `<unnamed>` (BM_WALEntry_Serialize): n/a

#### `BENCHMARK_DEFINE_F(WalBenchFixture, Append)(benchmark`
- Source: `benchmarks/replication/bench_replication_throughput.cpp`:111
- Brief: n/a
- Parameters:
  - `<unnamed>` (WalBenchFixture): n/a
  - `<unnamed>` (Append): n/a

#### `BENCHMARK_DEFINE_F(WalBenchFixture, ReadFrom)(benchmark`
- Source: `benchmarks/replication/bench_replication_throughput.cpp`:130
- Brief: n/a
- Parameters:
  - `<unnamed>` (WalBenchFixture): n/a
  - `<unnamed>` (ReadFrom): n/a

#### `BENCHMARK_MAIN()`
- Source: `benchmarks/replication/bench_replication_throughput.cpp`:329
- Brief: n/a
- Parameters: none

#### `void BM_CRDTMerge(benchmark::State &state)`
- Source: `benchmarks/replication/bench_replication_throughput.cpp`:219
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_HLCConflictDetection(benchmark::State &state)`
- Source: `benchmarks/replication/bench_replication_throughput.cpp`:190
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_ReplicationManager_Initialize(benchmark::State &state)`
- Source: `benchmarks/replication/bench_replication_throughput.cpp`:255
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_ReplicationManager_PromoteToLeader(benchmark::State &state)`
- Source: `benchmarks/replication/bench_replication_throughput.cpp`:284
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_WALEntry_Deserialize(benchmark::State &state)`
- Source: `benchmarks/replication/bench_replication_throughput.cpp`:173
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_WALEntry_Serialize(benchmark::State &state)`
- Source: `benchmarks/replication/bench_replication_throughput.cpp`:162
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `Unit(benchmark::kMicrosecond) -> Iterations(10000)`
- Source: `benchmarks/replication/bench_replication_throughput.cpp`:123
- Brief: n/a
- Parameters:
  - `<unnamed>` (benchmark::kMicrosecond): n/a

#### `Unit(benchmark::kMillisecond) -> Iterations(50)`
- Source: `benchmarks/replication/bench_replication_throughput.cpp`:277
- Brief: n/a
- Parameters:
  - `<unnamed>` (benchmark::kMillisecond): n/a

### test_replication_async_wal_lag_alerts.cpp

#### `TEST(AsyncWalShipperContract, ConstructAndStatsAreAccessible)`
- Source: `tests/replication/test_replication_async_wal_lag_alerts.cpp`:12
- Brief: n/a
- Parameters:
  - `<unnamed>` (AsyncWalShipperContract): n/a
  - `<unnamed>` (ConstructAndStatsAreAccessible): n/a

#### `TEST(AsyncWalShipperContract, EnqueueSegmentIncrementsCounter)`
- Source: `tests/replication/test_replication_async_wal_lag_alerts.cpp`:23
- Brief: n/a
- Parameters:
  - `<unnamed>` (AsyncWalShipperContract): n/a
  - `<unnamed>` (EnqueueSegmentIncrementsCounter): n/a

### test_replication_chaos_failover_focused.cpp

#### `TEST(GeoPlacementContract, SelectLeaderCandidateHonorsAvailability)`
- Source: `tests/replication/test_replication_chaos_failover_focused.cpp`:39
- Brief: n/a
- Parameters:
  - `<unnamed>` (GeoPlacementContract): n/a
  - `<unnamed>` (SelectLeaderCandidateHonorsAvailability): n/a

#### `TEST(GeoPlacementContract, ValidatePlacementReturnsStructuredResult)`
- Source: `tests/replication/test_replication_chaos_failover_focused.cpp`:29
- Brief: n/a
- Parameters:
  - `<unnamed>` (GeoPlacementContract): n/a
  - `<unnamed>` (ValidatePlacementReturnsStructuredResult): n/a

### test_replication_coordinator_focused.cpp

#### `TEST_F(ReplicationCoordinatorTest, RecordAcknowledgmentAcceptsValidReplicaId)`
- Source: `tests/replication/test_replication_coordinator_focused.cpp`:34
- Brief: n/a
- Parameters:
  - `<unnamed>` (ReplicationCoordinatorTest): n/a
  - `<unnamed>` (RecordAcknowledgmentAcceptsValidReplicaId): n/a

#### `TEST_F(ReplicationCoordinatorTest, RecordAcknowledgmentFailsClosedForEmptyReplicaId)`
- Source: `tests/replication/test_replication_coordinator_focused.cpp`:22
- Brief: n/a
- Parameters:
  - `<unnamed>` (ReplicationCoordinatorTest): n/a
  - `<unnamed>` (RecordAcknowledgmentFailsClosedForEmptyReplicaId): n/a

### test_replication_crdt_types.cpp

#### `TEST(DisableWinsFlagTest, ConcurrentDisableWins)`
- Source: `tests/replication/test_replication_crdt_types.cpp`:456
- Brief: n/a
- Parameters:
  - `<unnamed>` (DisableWinsFlagTest): n/a
  - `<unnamed>` (ConcurrentDisableWins): n/a

#### `TEST(DisableWinsFlagTest, DisableSetsFalse)`
- Source: `tests/replication/test_replication_crdt_types.cpp`:449
- Brief: n/a
- Parameters:
  - `<unnamed>` (DisableWinsFlagTest): n/a
  - `<unnamed>` (DisableSetsFalse): n/a

#### `TEST(DisableWinsFlagTest, EnableSetsTrue)`
- Source: `tests/replication/test_replication_crdt_types.cpp`:443
- Brief: n/a
- Parameters:
  - `<unnamed>` (DisableWinsFlagTest): n/a
  - `<unnamed>` (EnableSetsTrue): n/a

#### `TEST(EnableWinsFlagTest, ConcurrentEnableWins)`
- Source: `tests/replication/test_replication_crdt_types.cpp`:421
- Brief: n/a
- Parameters:
  - `<unnamed>` (EnableWinsFlagTest): n/a
  - `<unnamed>` (ConcurrentEnableWins): n/a

#### `TEST(EnableWinsFlagTest, DisableSetsFalse)`
- Source: `tests/replication/test_replication_crdt_types.cpp`:414
- Brief: n/a
- Parameters:
  - `<unnamed>` (EnableWinsFlagTest): n/a
  - `<unnamed>` (DisableSetsFalse): n/a

#### `TEST(EnableWinsFlagTest, DisabledByDefault)`
- Source: `tests/replication/test_replication_crdt_types.cpp`:401
- Brief: n/a
- Parameters:
  - `<unnamed>` (EnableWinsFlagTest): n/a
  - `<unnamed>` (DisabledByDefault): n/a

#### `TEST(EnableWinsFlagTest, EnableSetsTrue)`
- Source: `tests/replication/test_replication_crdt_types.cpp`:408
- Brief: n/a
- Parameters:
  - `<unnamed>` (EnableWinsFlagTest): n/a
  - `<unnamed>` (EnableSetsTrue): n/a

#### `TEST(EnableWinsFlagTest, MergeIdempotent)`
- Source: `tests/replication/test_replication_crdt_types.cpp`:430
- Brief: n/a
- Parameters:
  - `<unnamed>` (EnableWinsFlagTest): n/a
  - `<unnamed>` (MergeIdempotent): n/a

#### `TEST(GrowOnlyCounterTest, IncrementIncreases)`
- Source: `tests/replication/test_replication_crdt_types.cpp`:47
- Brief: n/a
- Parameters:
  - `<unnamed>` (GrowOnlyCounterTest): n/a
  - `<unnamed>` (IncrementIncreases): n/a

#### `TEST(GrowOnlyCounterTest, InitialValueIsZero)`
- Source: `tests/replication/test_replication_crdt_types.cpp`:42
- Brief: n/a
- Parameters:
  - `<unnamed>` (GrowOnlyCounterTest): n/a
  - `<unnamed>` (InitialValueIsZero): n/a

#### `TEST(GrowOnlyCounterTest, MergeCommutativity)`
- Source: `tests/replication/test_replication_crdt_types.cpp`:53
- Brief: n/a
- Parameters:
  - `<unnamed>` (GrowOnlyCounterTest): n/a
  - `<unnamed>` (MergeCommutativity): n/a

#### `TEST(GrowOnlyCounterTest, MergeIdempotency)`
- Source: `tests/replication/test_replication_crdt_types.cpp`:68
- Brief: n/a
- Parameters:
  - `<unnamed>` (GrowOnlyCounterTest): n/a
  - `<unnamed>` (MergeIdempotency): n/a

#### `TEST(GrowOnlyCounterTest, MergeTakesPerNodeMax)`
- Source: `tests/replication/test_replication_crdt_types.cpp`:77
- Brief: n/a
- Parameters:
  - `<unnamed>` (GrowOnlyCounterTest): n/a
  - `<unnamed>` (MergeTakesPerNodeMax): n/a

#### `TEST(GrowOnlySetTest, AddAndContains)`
- Source: `tests/replication/test_replication_crdt_types.cpp`:202
- Brief: n/a
- Parameters:
  - `<unnamed>` (GrowOnlySetTest): n/a
  - `<unnamed>` (AddAndContains): n/a

#### `TEST(GrowOnlySetTest, MergeUnion)`
- Source: `tests/replication/test_replication_crdt_types.cpp`:209
- Brief: n/a
- Parameters:
  - `<unnamed>` (GrowOnlySetTest): n/a
  - `<unnamed>` (MergeUnion): n/a

#### `TEST(LWWMapTest, LaterTimestampWinsPerKey)`
- Source: `tests/replication/test_replication_crdt_types.cpp`:320
- Brief: n/a
- Parameters:
  - `<unnamed>` (LWWMapTest): n/a
  - `<unnamed>` (LaterTimestampWinsPerKey): n/a

#### `TEST(LWWMapTest, MergeCommutativity)`
- Source: `tests/replication/test_replication_crdt_types.cpp`:342
- Brief: n/a
- Parameters:
  - `<unnamed>` (LWWMapTest): n/a
  - `<unnamed>` (MergeCommutativity): n/a

#### `TEST(LWWMapTest, MergePerKeyLWW)`
- Source: `tests/replication/test_replication_crdt_types.cpp`:334
- Brief: n/a
- Parameters:
  - `<unnamed>` (LWWMapTest): n/a
  - `<unnamed>` (MergePerKeyLWW): n/a

#### `TEST(LWWMapTest, PutAndGet)`
- Source: `tests/replication/test_replication_crdt_types.cpp`:312
- Brief: n/a
- Parameters:
  - `<unnamed>` (LWWMapTest): n/a
  - `<unnamed>` (PutAndGet): n/a

#### `TEST(LWWMapTest, RemoveKey)`
- Source: `tests/replication/test_replication_crdt_types.cpp`:327
- Brief: n/a
- Parameters:
  - `<unnamed>` (LWWMapTest): n/a
  - `<unnamed>` (RemoveKey): n/a

#### `TEST(LWWRegisterTest, EarlierTimestampLoses)`
- Source: `tests/replication/test_replication_crdt_types.cpp`:135
- Brief: n/a
- Parameters:
  - `<unnamed>` (LWWRegisterTest): n/a
  - `<unnamed>` (EarlierTimestampLoses): n/a

#### `TEST(LWWRegisterTest, LaterTimestampWins)`
- Source: `tests/replication/test_replication_crdt_types.cpp`:128
- Brief: n/a
- Parameters:
  - `<unnamed>` (LWWRegisterTest): n/a
  - `<unnamed>` (LaterTimestampWins): n/a

#### `TEST(LWWRegisterTest, MergePicksHigherTimestamp)`
- Source: `tests/replication/test_replication_crdt_types.cpp`:142
- Brief: n/a
- Parameters:
  - `<unnamed>` (LWWRegisterTest): n/a
  - `<unnamed>` (MergePicksHigherTimestamp): n/a

#### `TEST(LWWRegisterTest, TieBreakByNodeId)`
- Source: `tests/replication/test_replication_crdt_types.cpp`:150
- Brief: n/a
- Parameters:
  - `<unnamed>` (LWWRegisterTest): n/a
  - `<unnamed>` (TieBreakByNodeId): n/a

#### `TEST(LWWRegisterTest, WriteAndRead)`
- Source: `tests/replication/test_replication_crdt_types.cpp`:121
- Brief: n/a
- Parameters:
  - `<unnamed>` (LWWRegisterTest): n/a
  - `<unnamed>` (WriteAndRead): n/a

#### `TEST(MVRegisterTest, MergeIdempotent)`
- Source: `tests/replication/test_replication_crdt_types.cpp`:189
- Brief: n/a
- Parameters:
  - `<unnamed>` (MVRegisterTest): n/a
  - `<unnamed>` (MergeIdempotent): n/a

#### `TEST(MVRegisterTest, MergeUnionsConcurrentValues)`
- Source: `tests/replication/test_replication_crdt_types.cpp`:180
- Brief: n/a
- Parameters:
  - `<unnamed>` (MVRegisterTest): n/a
  - `<unnamed>` (MergeUnionsConcurrentValues): n/a

#### `TEST(MVRegisterTest, WriteAndReadSingleValue)`
- Source: `tests/replication/test_replication_crdt_types.cpp`:163
- Brief: n/a
- Parameters:
  - `<unnamed>` (MVRegisterTest): n/a
  - `<unnamed>` (WriteAndReadSingleValue): n/a

#### `TEST(MVRegisterTest, WriteOverwrites)`
- Source: `tests/replication/test_replication_crdt_types.cpp`:171
- Brief: n/a
- Parameters:
  - `<unnamed>` (MVRegisterTest): n/a
  - `<unnamed>` (WriteOverwrites): n/a

#### `TEST(ORSetTest, AddAndContains)`
- Source: `tests/replication/test_replication_crdt_types.cpp`:267
- Brief: n/a
- Parameters:
  - `<unnamed>` (ORSetTest): n/a
  - `<unnamed>` (AddAndContains): n/a

#### `TEST(ORSetTest, ConcurrentAddSurvivesConcurrentRemove)`
- Source: `tests/replication/test_replication_crdt_types.cpp`:288
- Brief: n/a
- Parameters:
  - `<unnamed>` (ORSetTest): n/a
  - `<unnamed>` (ConcurrentAddSurvivesConcurrentRemove): n/a

#### `TEST(ORSetTest, ElementsReturnsLiveItems)`
- Source: `tests/replication/test_replication_crdt_types.cpp`:298
- Brief: n/a
- Parameters:
  - `<unnamed>` (ORSetTest): n/a
  - `<unnamed>` (ElementsReturnsLiveItems): n/a

#### `TEST(ORSetTest, ReAddAfterRemove)`
- Source: `tests/replication/test_replication_crdt_types.cpp`:280
- Brief: n/a
- Parameters:
  - `<unnamed>` (ORSetTest): n/a
  - `<unnamed>` (ReAddAfterRemove): n/a

#### `TEST(ORSetTest, RemoveAfterAdd)`
- Source: `tests/replication/test_replication_crdt_types.cpp`:273
- Brief: n/a
- Parameters:
  - `<unnamed>` (ORSetTest): n/a
  - `<unnamed>` (RemoveAfterAdd): n/a

#### `TEST(PNCounterTest, CanGoNegative)`
- Source: `tests/replication/test_replication_crdt_types.cpp`:96
- Brief: n/a
- Parameters:
  - `<unnamed>` (PNCounterTest): n/a
  - `<unnamed>` (CanGoNegative): n/a

#### `TEST(PNCounterTest, IncrementAndDecrement)`
- Source: `tests/replication/test_replication_crdt_types.cpp`:89
- Brief: n/a
- Parameters:
  - `<unnamed>` (PNCounterTest): n/a
  - `<unnamed>` (IncrementAndDecrement): n/a

#### `TEST(PNCounterTest, MergeCommutativity)`
- Source: `tests/replication/test_replication_crdt_types.cpp`:102
- Brief: n/a
- Parameters:
  - `<unnamed>` (PNCounterTest): n/a
  - `<unnamed>` (MergeCommutativity): n/a

#### `TEST(RGArrayTest, AppendAndRead)`
- Source: `tests/replication/test_replication_crdt_types.cpp`:359
- Brief: n/a
- Parameters:
  - `<unnamed>` (RGArrayTest): n/a
  - `<unnamed>` (AppendAndRead): n/a

#### `TEST(RGArrayTest, MergeUnion)`
- Source: `tests/replication/test_replication_crdt_types.cpp`:388
- Brief: n/a
- Parameters:
  - `<unnamed>` (RGArrayTest): n/a
  - `<unnamed>` (MergeUnion): n/a

#### `TEST(RGArrayTest, RemoveTombstones)`
- Source: `tests/replication/test_replication_crdt_types.cpp`:369
- Brief: n/a
- Parameters:
  - `<unnamed>` (RGArrayTest): n/a
  - `<unnamed>` (RemoveTombstones): n/a

#### `TEST(RGArrayTest, SizeCountsLiveElements)`
- Source: `tests/replication/test_replication_crdt_types.cpp`:379
- Brief: n/a
- Parameters:
  - `<unnamed>` (RGArrayTest): n/a
  - `<unnamed>` (SizeCountsLiveElements): n/a

#### `TEST(TwoPSetTest, AddAndContains)`
- Source: `tests/replication/test_replication_crdt_types.cpp`:224
- Brief: n/a
- Parameters:
  - `<unnamed>` (TwoPSetTest): n/a
  - `<unnamed>` (AddAndContains): n/a

#### `TEST(TwoPSetTest, CannotReAddAfterRemove)`
- Source: `tests/replication/test_replication_crdt_types.cpp`:237
- Brief: n/a
- Parameters:
  - `<unnamed>` (TwoPSetTest): n/a
  - `<unnamed>` (CannotReAddAfterRemove): n/a

#### `TEST(TwoPSetTest, MergeUnionsBothSets)`
- Source: `tests/replication/test_replication_crdt_types.cpp`:254
- Brief: n/a
- Parameters:
  - `<unnamed>` (TwoPSetTest): n/a
  - `<unnamed>` (MergeUnionsBothSets): n/a

#### `TEST(TwoPSetTest, RemoveNeverAddedIsNoop)`
- Source: `tests/replication/test_replication_crdt_types.cpp`:247
- Brief: n/a
- Parameters:
  - `<unnamed>` (TwoPSetTest): n/a
  - `<unnamed>` (RemoveNeverAddedIsNoop): n/a

#### `TEST(TwoPSetTest, RemoveTombstones)`
- Source: `tests/replication/test_replication_crdt_types.cpp`:230
- Brief: n/a
- Parameters:
  - `<unnamed>` (TwoPSetTest): n/a
  - `<unnamed>` (RemoveTombstones): n/a

### test_replication_geo_placement_wal_shipping_focused.cpp

#### `TEST_F(AsyncWalShipperTest, WAL01_SegmentAcceptedAndShipped)`
- Source: `tests/replication/test_replication_geo_placement_wal_shipping_focused.cpp`:287
- Brief: n/a
- Parameters:
  - `<unnamed>` (AsyncWalShipperTest): n/a
  - `<unnamed>` (WAL01_SegmentAcceptedAndShipped): n/a

#### `TEST_F(AsyncWalShipperTest, WAL02_BackPressureOnFullQueue)`
- Source: `tests/replication/test_replication_geo_placement_wal_shipping_focused.cpp`:313
- Brief: n/a
- Parameters:
  - `<unnamed>` (AsyncWalShipperTest): n/a
  - `<unnamed>` (WAL02_BackPressureOnFullQueue): n/a

#### `TEST_F(AsyncWalShipperTest, WAL03_LagAlertFires)`
- Source: `tests/replication/test_replication_geo_placement_wal_shipping_focused.cpp`:345
- Brief: n/a
- Parameters:
  - `<unnamed>` (AsyncWalShipperTest): n/a
  - `<unnamed>` (WAL03_LagAlertFires): n/a

#### `TEST_F(AsyncWalShipperTest, WAL04_NoAlertWithinLagLimit)`
- Source: `tests/replication/test_replication_geo_placement_wal_shipping_focused.cpp`:375
- Brief: n/a
- Parameters:
  - `<unnamed>` (AsyncWalShipperTest): n/a
  - `<unnamed>` (WAL04_NoAlertWithinLagLimit): n/a

#### `TEST_F(AsyncWalShipperTest, WAL05_StatsAccounting)`
- Source: `tests/replication/test_replication_geo_placement_wal_shipping_focused.cpp`:397
- Brief: n/a
- Parameters:
  - `<unnamed>` (AsyncWalShipperTest): n/a
  - `<unnamed>` (WAL05_StatsAccounting): n/a

#### `TEST_F(AsyncWalShipperTest, WAL06_PrometheusMetricsPresent)`
- Source: `tests/replication/test_replication_geo_placement_wal_shipping_focused.cpp`:423
- Brief: n/a
- Parameters:
  - `<unnamed>` (AsyncWalShipperTest): n/a
  - `<unnamed>` (WAL06_PrometheusMetricsPresent): n/a

#### `TEST_F(AsyncWalShipperTest, WAL07_CurrentLagMsZeroOnEmptyQueue)`
- Source: `tests/replication/test_replication_geo_placement_wal_shipping_focused.cpp`:449
- Brief: n/a
- Parameters:
  - `<unnamed>` (AsyncWalShipperTest): n/a
  - `<unnamed>` (WAL07_CurrentLagMsZeroOnEmptyQueue): n/a

#### `TEST_F(AsyncWalShipperTest, WAL08_GracefulStopAndDoubleSafe)`
- Source: `tests/replication/test_replication_geo_placement_wal_shipping_focused.cpp`:459
- Brief: n/a
- Parameters:
  - `<unnamed>` (AsyncWalShipperTest): n/a
  - `<unnamed>` (WAL08_GracefulStopAndDoubleSafe): n/a

#### `TEST_F(GeoPlacementTest, GEO01_PreferredDCRespected)`
- Source: `tests/replication/test_replication_geo_placement_wal_shipping_focused.cpp`:104
- Brief: n/a
- Parameters:
  - `<unnamed>` (GeoPlacementTest): n/a
  - `<unnamed>` (GEO01_PreferredDCRespected): n/a

#### `TEST_F(GeoPlacementTest, GEO02_ForbiddenDCExcluded)`
- Source: `tests/replication/test_replication_geo_placement_wal_shipping_focused.cpp`:123
- Brief: n/a
- Parameters:
  - `<unnamed>` (GeoPlacementTest): n/a
  - `<unnamed>` (GEO02_ForbiddenDCExcluded): n/a

#### `TEST_F(GeoPlacementTest, GEO03_FallbackWhenPreferredDCUnhealthy)`
- Source: `tests/replication/test_replication_geo_placement_wal_shipping_focused.cpp`:141
- Brief: n/a
- Parameters:
  - `<unnamed>` (GeoPlacementTest): n/a
  - `<unnamed>` (GEO03_FallbackWhenPreferredDCUnhealthy): n/a

#### `TEST_F(GeoPlacementTest, GEO04_ZoneAffinityRespected)`
- Source: `tests/replication/test_replication_geo_placement_wal_shipping_focused.cpp`:161
- Brief: n/a
- Parameters:
  - `<unnamed>` (GeoPlacementTest): n/a
  - `<unnamed>` (GEO04_ZoneAffinityRespected): n/a

#### `TEST_F(GeoPlacementTest, GEO05_FailoverExcludesFailedNode)`
- Source: `tests/replication/test_replication_geo_placement_wal_shipping_focused.cpp`:182
- Brief: n/a
- Parameters:
  - `<unnamed>` (GeoPlacementTest): n/a
  - `<unnamed>` (GEO05_FailoverExcludesFailedNode): n/a

#### `TEST_F(GeoPlacementTest, GEO06_FailoverRespectsDCConstraint)`
- Source: `tests/replication/test_replication_geo_placement_wal_shipping_focused.cpp`:203
- Brief: n/a
- Parameters:
  - `<unnamed>` (GeoPlacementTest): n/a
  - `<unnamed>` (GEO06_FailoverRespectsDCConstraint): n/a

#### `TEST_F(GeoPlacementTest, GEO07_ValidatePlacement_RequiredDCMissing)`
- Source: `tests/replication/test_replication_geo_placement_wal_shipping_focused.cpp`:224
- Brief: n/a
- Parameters:
  - `<unnamed>` (GeoPlacementTest): n/a
  - `<unnamed>` (GEO07_ValidatePlacement_RequiredDCMissing): n/a

#### `TEST_F(GeoPlacementTest, GEO08_ValidatePlacement_MinCopiesPerDCViolation)`
- Source: `tests/replication/test_replication_geo_placement_wal_shipping_focused.cpp`:246
- Brief: n/a
- Parameters:
  - `<unnamed>` (GeoPlacementTest): n/a
  - `<unnamed>` (GEO08_ValidatePlacement_MinCopiesPerDCViolation): n/a

### test_replication_ha.cpp

#### `TEST(BatchedAckTrackerTest, DefaultConstructorWorks)`
- Source: `tests/replication/test_replication_ha.cpp`:2782
- Brief: n/a
- Parameters:
  - `<unnamed>` (BatchedAckTrackerTest): n/a
  - `<unnamed>` (DefaultConstructorWorks): n/a

#### `TEST(BatchedAckTrackerTest, DestructorJoinsCleanly)`
- Source: `tests/replication/test_replication_ha.cpp`:2788
- Brief: n/a
- Parameters:
  - `<unnamed>` (BatchedAckTrackerTest): n/a
  - `<unnamed>` (DestructorJoinsCleanly): n/a

#### `TEST(BatchedAckTrackerTest, ForceFlushDrainsBuffer)`
- Source: `tests/replication/test_replication_ha.cpp`:2750
- Brief: n/a
- Parameters:
  - `<unnamed>` (BatchedAckTrackerTest): n/a
  - `<unnamed>` (ForceFlushDrainsBuffer): n/a

#### `TEST(BatchedAckTrackerTest, HighestAckedTracked)`
- Source: `tests/replication/test_replication_ha.cpp`:2737
- Brief: n/a
- Parameters:
  - `<unnamed>` (BatchedAckTrackerTest): n/a
  - `<unnamed>` (HighestAckedTracked): n/a

#### `TEST(BatchedAckTrackerTest, RecordAndDequeue)`
- Source: `tests/replication/test_replication_ha.cpp`:2719
- Brief: n/a
- Parameters:
  - `<unnamed>` (BatchedAckTrackerTest): n/a
  - `<unnamed>` (RecordAndDequeue): n/a

#### `TEST(BatchedAckTrackerTest, StatsBatchSizeIsAccurate)`
- Source: `tests/replication/test_replication_ha.cpp`:2766
- Brief: n/a
- Parameters:
  - `<unnamed>` (BatchedAckTrackerTest): n/a
  - `<unnamed>` (StatsBatchSizeIsAccurate): n/a

#### `TEST(BidirectionalReplicationTest, BidirectionalSyncFalseBlocksIncomingWrites)`
- Source: `tests/replication/test_replication_ha.cpp`:5375
- Brief: n/a
- Parameters:
  - `<unnamed>` (BidirectionalReplicationTest): n/a
  - `<unnamed>` (BidirectionalSyncFalseBlocksIncomingWrites): n/a

#### `TEST(BidirectionalReplicationTest, CollectionStrategyOverridesDefault)`
- Source: `tests/replication/test_replication_ha.cpp`:5158
- Brief: n/a
- Parameters:
  - `<unnamed>` (BidirectionalReplicationTest): n/a
  - `<unnamed>` (CollectionStrategyOverridesDefault): n/a

#### `TEST(BidirectionalReplicationTest, ConcurrentWritesDetectedAsConflict)`
- Source: `tests/replication/test_replication_ha.cpp`:5119
- Brief: n/a
- Parameters:
  - `<unnamed>` (BidirectionalReplicationTest): n/a
  - `<unnamed>` (ConcurrentWritesDetectedAsConflict): n/a

#### `TEST(BidirectionalReplicationTest, ConflictsLastHourCountedInSyncStatus)`
- Source: `tests/replication/test_replication_ha.cpp`:5426
- Brief: n/a
- Parameters:
  - `<unnamed>` (BidirectionalReplicationTest): n/a
  - `<unnamed>` (ConflictsLastHourCountedInSyncStatus): n/a

#### `TEST(BidirectionalReplicationTest, CustomStrategyProducesPendingConflict)`
- Source: `tests/replication/test_replication_ha.cpp`:5197
- Brief: n/a
- Parameters:
  - `<unnamed>` (BidirectionalReplicationTest): n/a
  - `<unnamed>` (CustomStrategyProducesPendingConflict): n/a

#### `TEST(BidirectionalReplicationTest, DDLConflictIsRecordedAsDDLConflict)`
- Source: `tests/replication/test_replication_ha.cpp`:5298
- Brief: n/a
- Parameters:
  - `<unnamed>` (BidirectionalReplicationTest): n/a
  - `<unnamed>` (DDLConflictIsRecordedAsDDLConflict): n/a

#### `TEST(BidirectionalReplicationTest, DDLReplicationAcceptedAndTracked)`
- Source: `tests/replication/test_replication_ha.cpp`:5283
- Brief: n/a
- Parameters:
  - `<unnamed>` (BidirectionalReplicationTest): n/a
  - `<unnamed>` (DDLReplicationAcceptedAndTracked): n/a

#### `TEST(BidirectionalReplicationTest, DoubleStartIsIdempotent)`
- Source: `tests/replication/test_replication_ha.cpp`:5010
- Brief: n/a
- Parameters:
  - `<unnamed>` (BidirectionalReplicationTest): n/a
  - `<unnamed>` (DoubleStartIsIdempotent): n/a

#### `TEST(BidirectionalReplicationTest, ManualResolveConflictPicksWinner)`
- Source: `tests/replication/test_replication_ha.cpp`:5225
- Brief: n/a
- Parameters:
  - `<unnamed>` (BidirectionalReplicationTest): n/a
  - `<unnamed>` (ManualResolveConflictPicksWinner): n/a

#### `TEST(BidirectionalReplicationTest, ManualResolveReturnsFalseForUnknownNode)`
- Source: `tests/replication/test_replication_ha.cpp`:5256
- Brief: n/a
- Parameters:
  - `<unnamed>` (BidirectionalReplicationTest): n/a
  - `<unnamed>` (ManualResolveReturnsFalseForUnknownNode): n/a

#### `TEST(BidirectionalReplicationTest, OriginTrackingAcceptsPeerChanges)`
- Source: `tests/replication/test_replication_ha.cpp`:5065
- Brief: n/a
- Parameters:
  - `<unnamed>` (BidirectionalReplicationTest): n/a
  - `<unnamed>` (OriginTrackingAcceptsPeerChanges): n/a

#### `TEST(BidirectionalReplicationTest, OriginTrackingRejectsOwnChangeBouncing)`
- Source: `tests/replication/test_replication_ha.cpp`:5043
- Brief: n/a
- Parameters:
  - `<unnamed>` (BidirectionalReplicationTest): n/a
  - `<unnamed>` (OriginTrackingRejectsOwnChangeBouncing): n/a

#### `TEST(BidirectionalReplicationTest, OriginTrackingRejectsStaleOrDuplicateRemoteSequence)`
- Source: `tests/replication/test_replication_ha.cpp`:5085
- Brief: n/a
- Parameters:
  - `<unnamed>` (BidirectionalReplicationTest): n/a
  - `<unnamed>` (OriginTrackingRejectsStaleOrDuplicateRemoteSequence): n/a

#### `TEST(BidirectionalReplicationTest, ReplicateDDLFalseBlocksDDLApply)`
- Source: `tests/replication/test_replication_ha.cpp`:5403
- Brief: n/a
- Parameters:
  - `<unnamed>` (BidirectionalReplicationTest): n/a
  - `<unnamed>` (ReplicateDDLFalseBlocksDDLApply): n/a

#### `TEST(BidirectionalReplicationTest, StartFailsWhenLocalEqualsRemote)`
- Source: `tests/replication/test_replication_ha.cpp`:4996
- Brief: n/a
- Parameters:
  - `<unnamed>` (BidirectionalReplicationTest): n/a
  - `<unnamed>` (StartFailsWhenLocalEqualsRemote): n/a

#### `TEST(BidirectionalReplicationTest, StartFailsWhenNodeIdEmpty)`
- Source: `tests/replication/test_replication_ha.cpp`:5002
- Brief: n/a
- Parameters:
  - `<unnamed>` (BidirectionalReplicationTest): n/a
  - `<unnamed>` (StartFailsWhenNodeIdEmpty): n/a

#### `TEST(BidirectionalReplicationTest, StartSucceedsWithValidConfig)`
- Source: `tests/replication/test_replication_ha.cpp`:4988
- Brief: n/a
- Parameters:
  - `<unnamed>` (BidirectionalReplicationTest): n/a
  - `<unnamed>` (StartSucceedsWithValidConfig): n/a

#### `TEST(BidirectionalReplicationTest, SubmitWriteAdvancesLocalSequence)`
- Source: `tests/replication/test_replication_ha.cpp`:5020
- Brief: n/a
- Parameters:
  - `<unnamed>` (BidirectionalReplicationTest): n/a
  - `<unnamed>` (SubmitWriteAdvancesLocalSequence): n/a

#### `TEST(BidirectionalReplicationTest, SubmitWriteReturnsZeroWhenStopped)`
- Source: `tests/replication/test_replication_ha.cpp`:5034
- Brief: n/a
- Parameters:
  - `<unnamed>` (BidirectionalReplicationTest): n/a
  - `<unnamed>` (SubmitWriteReturnsZeroWhenStopped): n/a

#### `TEST(BidirectionalReplicationTest, SyncStatusIsSynchronizedWhenNoPendingAndLowLag)`
- Source: `tests/replication/test_replication_ha.cpp`:5343
- Brief: n/a
- Parameters:
  - `<unnamed>` (BidirectionalReplicationTest): n/a
  - `<unnamed>` (SyncStatusIsSynchronizedWhenNoPendingAndLowLag): n/a

#### `TEST(BidirectionalReplicationTest, SyncStatusNotSynchronizedWhenHighLag)`
- Source: `tests/replication/test_replication_ha.cpp`:5359
- Brief: n/a
- Parameters:
  - `<unnamed>` (BidirectionalReplicationTest): n/a
  - `<unnamed>` (SyncStatusNotSynchronizedWhenHighLag): n/a

#### `TEST(BidirectionalReplicationTest, SyncStatusReflectsLagFromUpdateRemoteSequence)`
- Source: `tests/replication/test_replication_ha.cpp`:5330
- Brief: n/a
- Parameters:
  - `<unnamed>` (BidirectionalReplicationTest): n/a
  - `<unnamed>` (SyncStatusReflectsLagFromUpdateRemoteSequence): n/a

#### `TEST(CDCManagerTest, MultipleSubscribersReceiveAll)`
- Source: `tests/replication/test_replication_ha.cpp`:3085
- Brief: n/a
- Parameters:
  - `<unnamed>` (CDCManagerTest): n/a
  - `<unnamed>` (MultipleSubscribersReceiveAll): n/a

#### `TEST(CDCManagerTest, ReplicationManagerDeliversCDCEvents)`
- Source: `tests/replication/test_replication_ha.cpp`:3122
- Brief: n/a
- Parameters:
  - `<unnamed>` (CDCManagerTest): n/a
  - `<unnamed>` (ReplicationManagerDeliversCDCEvents): n/a

#### `TEST(CDCManagerTest, SubscribeAndReceiveWildcard)`
- Source: `tests/replication/test_replication_ha.cpp`:3028
- Brief: n/a
- Parameters:
  - `<unnamed>` (CDCManagerTest): n/a
  - `<unnamed>` (SubscribeAndReceiveWildcard): n/a

#### `TEST(CDCManagerTest, SubscribeFiltersByCollection)`
- Source: `tests/replication/test_replication_ha.cpp`:3053
- Brief: n/a
- Parameters:
  - `<unnamed>` (CDCManagerTest): n/a
  - `<unnamed>` (SubscribeFiltersByCollection): n/a

#### `TEST(CDCManagerTest, SubscriptionCountAccurate)`
- Source: `tests/replication/test_replication_ha.cpp`:3100
- Brief: n/a
- Parameters:
  - `<unnamed>` (CDCManagerTest): n/a
  - `<unnamed>` (SubscriptionCountAccurate): n/a

#### `TEST(CDCManagerTest, ThrowingCallbackDoesNotCrash)`
- Source: `tests/replication/test_replication_ha.cpp`:3112
- Brief: n/a
- Parameters:
  - `<unnamed>` (CDCManagerTest): n/a
  - `<unnamed>` (ThrowingCallbackDoesNotCrash): n/a

#### `TEST(CDCManagerTest, UnsubscribeStopsDelivery)`
- Source: `tests/replication/test_replication_ha.cpp`:3070
- Brief: n/a
- Parameters:
  - `<unnamed>` (CDCManagerTest): n/a
  - `<unnamed>` (UnsubscribeStopsDelivery): n/a

#### `TEST(CompressedStreamTest, AdaptiveFalseAlwaysCompressesInAutoMode)`
- Source: `tests/replication/test_replication_ha.cpp`:2598
- Brief: n/a
- Parameters:
  - `<unnamed>` (CompressedStreamTest): n/a
  - `<unnamed>` (AdaptiveFalseAlwaysCompressesInAutoMode): n/a

#### `TEST(CompressedStreamTest, AlreadyCompressedDataHasMinimalBenefit)`
- Source: `tests/replication/test_replication_ha.cpp`:2514
- Brief: n/a
- Parameters:
  - `<unnamed>` (CompressedStreamTest): n/a
  - `<unnamed>` (AlreadyCompressedDataHasMinimalBenefit): n/a

#### `TEST(CompressedStreamTest, AutoSkipsCompressionForSmallBatches)`
- Source: `tests/replication/test_replication_ha.cpp`:2397
- Brief: n/a
- Parameters:
  - `<unnamed>` (CompressedStreamTest): n/a
  - `<unnamed>` (AutoSkipsCompressionForSmallBatches): n/a

#### `TEST(CompressedStreamTest, AutoUsesZstdForLargeBatches)`
- Source: `tests/replication/test_replication_ha.cpp`:2414
- Brief: n/a
- Parameters:
  - `<unnamed>` (CompressedStreamTest): n/a
  - `<unnamed>` (AutoUsesZstdForLargeBatches): n/a

#### `TEST(CompressedStreamTest, DefaultConstructorWorks)`
- Source: `tests/replication/test_replication_ha.cpp`:2469
- Brief: n/a
- Parameters:
  - `<unnamed>` (CompressedStreamTest): n/a
  - `<unnamed>` (DefaultConstructorWorks): n/a

#### `TEST(CompressedStreamTest, EmptyBatchReturnsTrue)`
- Source: `tests/replication/test_replication_ha.cpp`:2475
- Brief: n/a
- Parameters:
  - `<unnamed>` (CompressedStreamTest): n/a
  - `<unnamed>` (EmptyBatchReturnsTrue): n/a

#### `TEST(CompressedStreamTest, LZ4CompressesAndTracksStats)`
- Source: `tests/replication/test_replication_ha.cpp`:2364
- Brief: n/a
- Parameters:
  - `<unnamed>` (CompressedStreamTest): n/a
  - `<unnamed>` (LZ4CompressesAndTracksStats): n/a

#### `TEST(CompressedStreamTest, LZ4RoundTrip)`
- Source: `tests/replication/test_replication_ha.cpp`:2554
- Brief: n/a
- Parameters:
  - `<unnamed>` (CompressedStreamTest): n/a
  - `<unnamed>` (LZ4RoundTrip): n/a

#### `TEST(CompressedStreamTest, NoneAlgorithmReturnsSameSize)`
- Source: `tests/replication/test_replication_ha.cpp`:2330
- Brief: n/a
- Parameters:
  - `<unnamed>` (CompressedStreamTest): n/a
  - `<unnamed>` (NoneAlgorithmReturnsSameSize): n/a

#### `TEST(CompressedStreamTest, ResetStatsWorks)`
- Source: `tests/replication/test_replication_ha.cpp`:2429
- Brief: n/a
- Parameters:
  - `<unnamed>` (CompressedStreamTest): n/a
  - `<unnamed>` (ResetStatsWorks): n/a

#### `TEST(CompressedStreamTest, SnappyCompressesAndTracksStats)`
- Source: `tests/replication/test_replication_ha.cpp`:2381
- Brief: n/a
- Parameters:
  - `<unnamed>` (CompressedStreamTest): n/a
  - `<unnamed>` (SnappyCompressesAndTracksStats): n/a

#### `TEST(CompressedStreamTest, SnappyRoundTrip)`
- Source: `tests/replication/test_replication_ha.cpp`:2578
- Brief: n/a
- Parameters:
  - `<unnamed>` (CompressedStreamTest): n/a
  - `<unnamed>` (SnappyRoundTrip): n/a

#### `TEST(CompressedStreamTest, ZstdAchievesHighRatioOnJsonLikeData)`
- Source: `tests/replication/test_replication_ha.cpp`:2481
- Brief: n/a
- Parameters:
  - `<unnamed>` (CompressedStreamTest): n/a
  - `<unnamed>` (ZstdAchievesHighRatioOnJsonLikeData): n/a

#### `TEST(CompressedStreamTest, ZstdCompressesRepeatedData)`
- Source: `tests/replication/test_replication_ha.cpp`:2346
- Brief: n/a
- Parameters:
  - `<unnamed>` (CompressedStreamTest): n/a
  - `<unnamed>` (ZstdCompressesRepeatedData): n/a

#### `TEST(CompressedStreamTest, ZstdRoundTrip)`
- Source: `tests/replication/test_replication_ha.cpp`:2442
- Brief: n/a
- Parameters:
  - `<unnamed>` (CompressedStreamTest): n/a
  - `<unnamed>` (ZstdRoundTrip): n/a

#### `TEST(CompressedStreamThroughputPerfTest, ZstdBatchUnder50ms)`
- Source: `tests/replication/test_replication_ha.cpp`:5940
- Brief: CompressedReplicationStream Zstd throughput proxy.
- Parameters:
  - `<unnamed>` (CompressedStreamThroughputPerfTest): n/a
  - `<unnamed>` (ZstdBatchUnder50ms): n/a
- Details: Validates that the in-process Zstd compression path can sustain the serialise+compress throughput needed for the 500 MB/s WAL shipping goal (Design Constraint #2). Each batch of 1,000 × 512-byte WAL entries amounts to ~512 KB; the test asserts the round-trip completes within 50 ms (≥ 10 MB/s — conservative floor that rules out algorithmic regressions without requiring network infrastructure). Set THEMIS_RUN_PERF_TESTS=1 to enable.

#### `TEST(CrossClusterE2ETest, FilteredPublicationDeliversOnlyMatchingEntries)`
- Source: `tests/replication/test_replication_ha.cpp`:4331
- Brief: n/a
- Parameters:
  - `<unnamed>` (CrossClusterE2ETest): n/a
  - `<unnamed>` (FilteredPublicationDeliversOnlyMatchingEntries): n/a

#### `TEST(CrossClusterE2ETest, MultipleSubscriptionsReceiveIndependently)`
- Source: `tests/replication/test_replication_ha.cpp`:4359
- Brief: n/a
- Parameters:
  - `<unnamed>` (CrossClusterE2ETest): n/a
  - `<unnamed>` (MultipleSubscriptionsReceiveIndependently): n/a

#### `TEST(CrossClusterE2ETest, WALEntryAppliedIntegrationPath)`
- Source: `tests/replication/test_replication_ha.cpp`:4383
- Brief: n/a
- Parameters:
  - `<unnamed>` (CrossClusterE2ETest): n/a
  - `<unnamed>` (WALEntryAppliedIntegrationPath): n/a

#### `TEST(CrossClusterIntegrationTest, FilterDropsNonMatchingEntriesViaReplicationManager)`
- Source: `tests/replication/test_replication_ha.cpp`:4460
- Brief: n/a
- Parameters:
  - `<unnamed>` (CrossClusterIntegrationTest): n/a
  - `<unnamed>` (FilterDropsNonMatchingEntriesViaReplicationManager): n/a

#### `TEST(CrossClusterIntegrationTest, PublicationReceivesEntriesViaReplicationManager)`
- Source: `tests/replication/test_replication_ha.cpp`:4414
- Brief: n/a
- Parameters:
  - `<unnamed>` (CrossClusterIntegrationTest): n/a
  - `<unnamed>` (PublicationReceivesEntriesViaReplicationManager): n/a

#### `TEST(CrossClusterPrometheusTest, PublicationMetricsCorrect)`
- Source: `tests/replication/test_replication_ha.cpp`:4524
- Brief: n/a
- Parameters:
  - `<unnamed>` (CrossClusterPrometheusTest): n/a
  - `<unnamed>` (PublicationMetricsCorrect): n/a

#### `TEST(CrossClusterPrometheusTest, SubscriptionMetricsCorrect)`
- Source: `tests/replication/test_replication_ha.cpp`:4543
- Brief: n/a
- Parameters:
  - `<unnamed>` (CrossClusterPrometheusTest): n/a
  - `<unnamed>` (SubscriptionMetricsCorrect): n/a

#### `TEST(CrossClusterPrometheusTest, SubscriptionMetricsReflectErrors)`
- Source: `tests/replication/test_replication_ha.cpp`:4562
- Brief: n/a
- Parameters:
  - `<unnamed>` (CrossClusterPrometheusTest): n/a
  - `<unnamed>` (SubscriptionMetricsReflectErrors): n/a

#### `TEST(ElectionLoopTest, HeartbeatResetsElectionTimer)`
- Source: `tests/replication/test_replication_ha.cpp`:653
- Brief: n/a
- Parameters:
  - `<unnamed>` (ElectionLoopTest): n/a
  - `<unnamed>` (HeartbeatResetsElectionTimer): n/a

#### `TEST(ElectionLoopTest, StopDoesNotHang)`
- Source: `tests/replication/test_replication_ha.cpp`:676
- Brief: n/a
- Parameters:
  - `<unnamed>` (ElectionLoopTest): n/a
  - `<unnamed>` (StopDoesNotHang): n/a

#### `TEST(ElectionLoopTest, TimeoutTriggersElectionOnSingleNode)`
- Source: `tests/replication/test_replication_ha.cpp`:630
- Brief: n/a
- Parameters:
  - `<unnamed>` (ElectionLoopTest): n/a
  - `<unnamed>` (TimeoutTriggersElectionOnSingleNode): n/a

#### `TEST(HLCPerfTest, NowCallUnder5us)`
- Source: `tests/replication/test_replication_ha.cpp`:5842
- Brief: HybridLogicalClock::now() latency < 5 µs per call.
- Parameters:
  - `<unnamed>` (HLCPerfTest): n/a
  - `<unnamed>` (NowCallUnder5us): n/a
- Details: Design Constraint #4 (FUTURE_ENHANCEMENTS.md): "Vector clock comparison and HLC conflict detection must add < 5 µs per write operation." Set THEMIS_RUN_PERF_TESTS=1 to enable.

#### `TEST(HLCTest, CurrentReturnsLastGeneratedTimestamp)`
- Source: `tests/replication/test_replication_ha.cpp`:821
- Brief: n/a
- Parameters:
  - `<unnamed>` (HLCTest): n/a
  - `<unnamed>` (CurrentReturnsLastGeneratedTimestamp): n/a

#### `TEST(HLCTest, NowReturnsMonotonicTimestamps)`
- Source: `tests/replication/test_replication_ha.cpp`:796
- Brief: n/a
- Parameters:
  - `<unnamed>` (HLCTest): n/a
  - `<unnamed>` (NowReturnsMonotonicTimestamps): n/a

#### `TEST(HLCTest, ReceiveAdvancesClockBeyondSender)`
- Source: `tests/replication/test_replication_ha.cpp`:807
- Brief: n/a
- Parameters:
  - `<unnamed>` (HLCTest): n/a
  - `<unnamed>` (ReceiveAdvancesClockBeyondSender): n/a

#### `TEST(HLCTest, TimestampOrdering)`
- Source: `tests/replication/test_replication_ha.cpp`:830
- Brief: n/a
- Parameters:
  - `<unnamed>` (HLCTest): n/a
  - `<unnamed>` (TimestampOrdering): n/a

#### `TEST(HLCTest, TimestampToString)`
- Source: `tests/replication/test_replication_ha.cpp`:840
- Brief: n/a
- Parameters:
  - `<unnamed>` (HLCTest): n/a
  - `<unnamed>` (TimestampToString): n/a

#### `TEST(LagBasedReadRouterTest, EligibleReplicaCountIsCorrect)`
- Source: `tests/replication/test_replication_ha.cpp`:4713
- Brief: n/a
- Parameters:
  - `<unnamed>` (LagBasedReadRouterTest): n/a
  - `<unnamed>` (EligibleReplicaCountIsCorrect): n/a

#### `TEST(LagBasedReadRouterTest, ExcludesFailedReplicas)`
- Source: `tests/replication/test_replication_ha.cpp`:4684
- Brief: n/a
- Parameters:
  - `<unnamed>` (LagBasedReadRouterTest): n/a
  - `<unnamed>` (ExcludesFailedReplicas): n/a

#### `TEST(LagBasedReadRouterTest, ExcludesHighLagReplicaAndFallsBackToPrimary)`
- Source: `tests/replication/test_replication_ha.cpp`:4650
- Brief: n/a
- Parameters:
  - `<unnamed>` (LagBasedReadRouterTest): n/a
  - `<unnamed>` (ExcludesHighLagReplicaAndFallsBackToPrimary): n/a

#### `TEST(LagBasedReadRouterTest, FallsBackToPrimaryWhenNoReplicas)`
- Source: `tests/replication/test_replication_ha.cpp`:4609
- Brief: n/a
- Parameters:
  - `<unnamed>` (LagBasedReadRouterTest): n/a
  - `<unnamed>` (FallsBackToPrimaryWhenNoReplicas): n/a

#### `TEST(LagBasedReadRouterTest, PrimaryPreferenceAlwaysReturnsPrimary)`
- Source: `tests/replication/test_replication_ha.cpp`:4625
- Brief: n/a
- Parameters:
  - `<unnamed>` (LagBasedReadRouterTest): n/a
  - `<unnamed>` (PrimaryPreferenceAlwaysReturnsPrimary): n/a

#### `TEST(LagBasedReadRouterTest, PrometheusMetricsContainExpectedKeys)`
- Source: `tests/replication/test_replication_ha.cpp`:4729
- Brief: n/a
- Parameters:
  - `<unnamed>` (LagBasedReadRouterTest): n/a
  - `<unnamed>` (PrometheusMetricsContainExpectedKeys): n/a

#### `TEST(LagBasedReadRouterTest, ReplicationManagerSelectReadReplicaReturnsPrimaryWhenNoReplicas)`
- Source: `tests/replication/test_replication_ha.cpp`:4740
- Brief: n/a
- Parameters:
  - `<unnamed>` (LagBasedReadRouterTest): n/a
  - `<unnamed>` (ReplicationManagerSelectReadReplicaReturnsPrimaryWhenNoReplicas): n/a

#### `TEST(LagBasedReadRouterTest, SecondaryPreferenceReturnsEmptyWhenNoEligible)`
- Source: `tests/replication/test_replication_ha.cpp`:4700
- Brief: n/a
- Parameters:
  - `<unnamed>` (LagBasedReadRouterTest): n/a
  - `<unnamed>` (SecondaryPreferenceReturnsEmptyWhenNoEligible): n/a

#### `TEST(LagBasedReadRouterTest, SelectsEligibleReplicaOverPrimary)`
- Source: `tests/replication/test_replication_ha.cpp`:4635
- Brief: n/a
- Parameters:
  - `<unnamed>` (LagBasedReadRouterTest): n/a
  - `<unnamed>` (SelectsEligibleReplicaOverPrimary): n/a

#### `TEST(LagBasedReadRouterTest, SelectsLowestLagReplica)`
- Source: `tests/replication/test_replication_ha.cpp`:4667
- Brief: n/a
- Parameters:
  - `<unnamed>` (LagBasedReadRouterTest): n/a
  - `<unnamed>` (SelectsLowestLagReplica): n/a

#### `TEST(LeaderLeaseTest, DirectLeaseApiRenewAndExpire)`
- Source: `tests/replication/test_replication_ha.cpp`:3915
- Brief: n/a
- Parameters:
  - `<unnamed>` (LeaderLeaseTest): n/a
  - `<unnamed>` (DirectLeaseApiRenewAndExpire): n/a

#### `TEST(LeaderLeaseTest, LeaseExpiresWithoutRenewal)`
- Source: `tests/replication/test_replication_ha.cpp`:3875
- Brief: n/a
- Parameters:
  - `<unnamed>` (LeaderLeaseTest): n/a
  - `<unnamed>` (LeaseExpiresWithoutRenewal): n/a

#### `TEST(LeaderLeaseTest, LeaseReadFailsWhenLeaseDisabled)`
- Source: `tests/replication/test_replication_ha.cpp`:3854
- Brief: n/a
- Parameters:
  - `<unnamed>` (LeaderLeaseTest): n/a
  - `<unnamed>` (LeaseReadFailsWhenLeaseDisabled): n/a

#### `TEST(LeaderLeaseTest, LeaseReadOnUninitialisedManagerFails)`
- Source: `tests/replication/test_replication_ha.cpp`:3900
- Brief: n/a
- Parameters:
  - `<unnamed>` (LeaderLeaseTest): n/a
  - `<unnamed>` (LeaseReadOnUninitialisedManagerFails): n/a

#### `TEST(LeaderLeaseTest, LeaseReadSucceedsOnLeaderWithValidLease)`
- Source: `tests/replication/test_replication_ha.cpp`:3831
- Brief: n/a
- Parameters:
  - `<unnamed>` (LeaderLeaseTest): n/a
  - `<unnamed>` (LeaseReadSucceedsOnLeaderWithValidLease): n/a

#### `TEST(LeaderLeaseTest, PrometheusMetricsContainLeaseCounters)`
- Source: `tests/replication/test_replication_ha.cpp`:3945
- Brief: n/a
- Parameters:
  - `<unnamed>` (LeaderLeaseTest): n/a
  - `<unnamed>` (PrometheusMetricsContainLeaseCounters): n/a

#### `TEST(LeaderLeaseTest, SingleNodeAcquiresLeaseAfterElection)`
- Source: `tests/replication/test_replication_ha.cpp`:3812
- Brief: n/a
- Parameters:
  - `<unnamed>` (LeaderLeaseTest): n/a
  - `<unnamed>` (SingleNodeAcquiresLeaseAfterElection): n/a

#### `TEST(MMCRDTResolverTest, FlagDWConcurrentEnableDisableDisableWins)`
- Source: `tests/replication/test_replication_ha.cpp`:1216
- Brief: n/a
- Parameters:
  - `<unnamed>` (MMCRDTResolverTest): n/a
  - `<unnamed>` (FlagDWConcurrentEnableDisableDisableWins): n/a

#### `TEST(MMCRDTResolverTest, FlagDWDisabledWhenNoEnableTags)`
- Source: `tests/replication/test_replication_ha.cpp`:1236
- Brief: n/a
- Parameters:
  - `<unnamed>` (MMCRDTResolverTest): n/a
  - `<unnamed>` (FlagDWDisabledWhenNoEnableTags): n/a

#### `TEST(MMCRDTResolverTest, FlagDWEnabledWhenNoDisableTags)`
- Source: `tests/replication/test_replication_ha.cpp`:1204
- Brief: n/a
- Parameters:
  - `<unnamed>` (MMCRDTResolverTest): n/a
  - `<unnamed>` (FlagDWEnabledWhenNoDisableTags): n/a

#### `TEST(MMCRDTResolverTest, FlagDWStrategyName)`
- Source: `tests/replication/test_replication_ha.cpp`:1199
- Brief: n/a
- Parameters:
  - `<unnamed>` (MMCRDTResolverTest): n/a
  - `<unnamed>` (FlagDWStrategyName): n/a

#### `TEST(MMCRDTResolverTest, FlagEWConcurrentEnableDisableEnableWins)`
- Source: `tests/replication/test_replication_ha.cpp`:1178
- Brief: n/a
- Parameters:
  - `<unnamed>` (MMCRDTResolverTest): n/a
  - `<unnamed>` (FlagEWConcurrentEnableDisableEnableWins): n/a

#### `TEST(MMCRDTResolverTest, FlagEWDisabledWhenAllTagsTombstoned)`
- Source: `tests/replication/test_replication_ha.cpp`:1160
- Brief: n/a
- Parameters:
  - `<unnamed>` (MMCRDTResolverTest): n/a
  - `<unnamed>` (FlagEWDisabledWhenAllTagsTombstoned): n/a

#### `TEST(MMCRDTResolverTest, FlagEWEnabledWhenLiveTagExists)`
- Source: `tests/replication/test_replication_ha.cpp`:1147
- Brief: n/a
- Parameters:
  - `<unnamed>` (MMCRDTResolverTest): n/a
  - `<unnamed>` (FlagEWEnabledWhenLiveTagExists): n/a

#### `TEST(MMCRDTResolverTest, FlagEWStrategyName)`
- Source: `tests/replication/test_replication_ha.cpp`:1142
- Brief: n/a
- Parameters:
  - `<unnamed>` (MMCRDTResolverTest): n/a
  - `<unnamed>` (FlagEWStrategyName): n/a

#### `TEST(MMCRDTResolverTest, GCounterMergesMaxPerKey)`
- Source: `tests/replication/test_replication_ha.cpp`:892
- Brief: n/a
- Parameters:
  - `<unnamed>` (MMCRDTResolverTest): n/a
  - `<unnamed>` (GCounterMergesMaxPerKey): n/a

#### `TEST(MMCRDTResolverTest, GSetUnionOfValues)`
- Source: `tests/replication/test_replication_ha.cpp`:909
- Brief: n/a
- Parameters:
  - `<unnamed>` (MMCRDTResolverTest): n/a
  - `<unnamed>` (GSetUnionOfValues): n/a

#### `TEST(MMCRDTResolverTest, LWWMapPicksLatestValuePerKey)`
- Source: `tests/replication/test_replication_ha.cpp`:1111
- Brief: n/a
- Parameters:
  - `<unnamed>` (MMCRDTResolverTest): n/a
  - `<unnamed>` (LWWMapPicksLatestValuePerKey): n/a

#### `TEST(MMCRDTResolverTest, LWWRegisterPicksLatest)`
- Source: `tests/replication/test_replication_ha.cpp`:877
- Brief: n/a
- Parameters:
  - `<unnamed>` (MMCRDTResolverTest): n/a
  - `<unnamed>` (LWWRegisterPicksLatest): n/a

#### `TEST(MMCRDTResolverTest, MVRegisterReturnsAllConcurrentValues)`
- Source: `tests/replication/test_replication_ha.cpp`:1092
- Brief: n/a
- Parameters:
  - `<unnamed>` (MMCRDTResolverTest): n/a
  - `<unnamed>` (MVRegisterReturnsAllConcurrentValues): n/a

#### `TEST(MMCRDTResolverTest, ORSetKeepsElementAddedOnBothNodes)`
- Source: `tests/replication/test_replication_ha.cpp`:990
- Brief: n/a
- Parameters:
  - `<unnamed>` (MMCRDTResolverTest): n/a
  - `<unnamed>` (ORSetKeepsElementAddedOnBothNodes): n/a

#### `TEST(MMCRDTResolverTest, ORSetRemovesTombstonedElements)`
- Source: `tests/replication/test_replication_ha.cpp`:968
- Brief: n/a
- Parameters:
  - `<unnamed>` (MMCRDTResolverTest): n/a
  - `<unnamed>` (ORSetRemovesTombstonedElements): n/a

#### `TEST(MMCRDTResolverTest, PNCounterMergesPositiveAndNegativeSeparately)`
- Source: `tests/replication/test_replication_ha.cpp`:941
- Brief: n/a
- Parameters:
  - `<unnamed>` (MMCRDTResolverTest): n/a
  - `<unnamed>` (PNCounterMergesPositiveAndNegativeSeparately): n/a

#### `TEST(MMCRDTResolverTest, RGAMergesElementsById)`
- Source: `tests/replication/test_replication_ha.cpp`:1045
- Brief: n/a
- Parameters:
  - `<unnamed>` (MMCRDTResolverTest): n/a
  - `<unnamed>` (RGAMergesElementsById): n/a

#### `TEST(MMCRDTResolverTest, RGATombstoneIrrevocable)`
- Source: `tests/replication/test_replication_ha.cpp`:1071
- Brief: n/a
- Parameters:
  - `<unnamed>` (MMCRDTResolverTest): n/a
  - `<unnamed>` (RGATombstoneIrrevocable): n/a

#### `TEST(MMCRDTResolverTest, StrategyNameLWWMapAndMVRegister)`
- Source: `tests/replication/test_replication_ha.cpp`:1130
- Brief: n/a
- Parameters:
  - `<unnamed>` (MMCRDTResolverTest): n/a
  - `<unnamed>` (StrategyNameLWWMapAndMVRegister): n/a

#### `TEST(MMCRDTResolverTest, StrategyNameMatchesType)`
- Source: `tests/replication/test_replication_ha.cpp`:927
- Brief: n/a
- Parameters:
  - `<unnamed>` (MMCRDTResolverTest): n/a
  - `<unnamed>` (StrategyNameMatchesType): n/a

#### `TEST(MMCRDTResolverTest, TwoPSetEmptyRemoveReturnsAllAdded)`
- Source: `tests/replication/test_replication_ha.cpp`:1031
- Brief: n/a
- Parameters:
  - `<unnamed>` (MMCRDTResolverTest): n/a
  - `<unnamed>` (TwoPSetEmptyRemoveReturnsAllAdded): n/a

#### `TEST(MMCRDTResolverTest, TwoPSetExcludesRemovedElements)`
- Source: `tests/replication/test_replication_ha.cpp`:1009
- Brief: n/a
- Parameters:
  - `<unnamed>` (MMCRDTResolverTest): n/a
  - `<unnamed>` (TwoPSetExcludesRemovedElements): n/a

#### `TEST(MMLastWriteWinsTest, EmptyInputReturnsDefault)`
- Source: `tests/replication/test_replication_ha.cpp`:871
- Brief: n/a
- Parameters:
  - `<unnamed>` (MMLastWriteWinsTest): n/a
  - `<unnamed>` (EmptyInputReturnsDefault): n/a

#### `TEST(MMLastWriteWinsTest, SelectsLatestHLCTimestamp)`
- Source: `tests/replication/test_replication_ha.cpp`:857
- Brief: n/a
- Parameters:
  - `<unnamed>` (MMLastWriteWinsTest): n/a
  - `<unnamed>` (SelectsLatestHLCTimestamp): n/a

#### `TEST(MMReplicationManagerTest, AddAndRemovePeer)`
- Source: `tests/replication/test_replication_ha.cpp`:1376
- Brief: n/a
- Parameters:
  - `<unnamed>` (MMReplicationManagerTest): n/a
  - `<unnamed>` (AddAndRemovePeer): n/a

#### `TEST(MMReplicationManagerTest, ConcurrentWritesAreThreadSafe)`
- Source: `tests/replication/test_replication_ha.cpp`:1550
- Brief: n/a
- Parameters:
  - `<unnamed>` (MMReplicationManagerTest): n/a
  - `<unnamed>` (ConcurrentWritesAreThreadSafe): n/a

#### `TEST(MMReplicationManagerTest, DoubleStartIsIdempotent)`
- Source: `tests/replication/test_replication_ha.cpp`:1330
- Brief: n/a
- Parameters:
  - `<unnamed>` (MMReplicationManagerTest): n/a
  - `<unnamed>` (DoubleStartIsIdempotent): n/a

#### `TEST(MMReplicationManagerTest, GetLocalInfo)`
- Source: `tests/replication/test_replication_ha.cpp`:1406
- Brief: n/a
- Parameters:
  - `<unnamed>` (MMReplicationManagerTest): n/a
  - `<unnamed>` (GetLocalInfo): n/a

#### `TEST(MMReplicationManagerTest, GetUnresolvedConflictsInitiallyEmpty)`
- Source: `tests/replication/test_replication_ha.cpp`:1483
- Brief: n/a
- Parameters:
  - `<unnamed>` (MMReplicationManagerTest): n/a
  - `<unnamed>` (GetUnresolvedConflictsInitiallyEmpty): n/a

#### `TEST(MMReplicationManagerTest, PrometheusMetricsContainNodeId)`
- Source: `tests/replication/test_replication_ha.cpp`:1464
- Brief: n/a
- Parameters:
  - `<unnamed>` (MMReplicationManagerTest): n/a
  - `<unnamed>` (PrometheusMetricsContainNodeId): n/a

#### `TEST(MMReplicationManagerTest, ReplicationLagZeroWhenNoPeers)`
- Source: `tests/replication/test_replication_ha.cpp`:1493
- Brief: n/a
- Parameters:
  - `<unnamed>` (MMReplicationManagerTest): n/a
  - `<unnamed>` (ReplicationLagZeroWhenNoPeers): n/a

#### `TEST(MMReplicationManagerTest, SetAndTriggerConflictCallback)`
- Source: `tests/replication/test_replication_ha.cpp`:1420
- Brief: n/a
- Parameters:
  - `<unnamed>` (MMReplicationManagerTest): n/a
  - `<unnamed>` (SetAndTriggerConflictCallback): n/a

#### `TEST(MMReplicationManagerTest, SetConflictResolverPerCollection)`
- Source: `tests/replication/test_replication_ha.cpp`:1445
- Brief: n/a
- Parameters:
  - `<unnamed>` (MMReplicationManagerTest): n/a
  - `<unnamed>` (SetConflictResolverPerCollection): n/a

#### `TEST(MMReplicationManagerTest, StartAndStop)`
- Source: `tests/replication/test_replication_ha.cpp`:1321
- Brief: n/a
- Parameters:
  - `<unnamed>` (MMReplicationManagerTest): n/a
  - `<unnamed>` (StartAndStop): n/a

#### `TEST(MMReplicationManagerTest, StatsTrackWrites)`
- Source: `tests/replication/test_replication_ha.cpp`:1360
- Brief: n/a
- Parameters:
  - `<unnamed>` (MMReplicationManagerTest): n/a
  - `<unnamed>` (StatsTrackWrites): n/a

#### `TEST(MMReplicationManagerTest, TopologySnapshotLocalNodeOnly)`
- Source: `tests/replication/test_replication_ha.cpp`:1591
- Brief: n/a
- Parameters:
  - `<unnamed>` (MMReplicationManagerTest): n/a
  - `<unnamed>` (TopologySnapshotLocalNodeOnly): n/a

#### `TEST(MMReplicationManagerTest, TopologySnapshotReportsOfflineWhenStopped)`
- Source: `tests/replication/test_replication_ha.cpp`:1667
- Brief: n/a
- Parameters:
  - `<unnamed>` (MMReplicationManagerTest): n/a
  - `<unnamed>` (TopologySnapshotReportsOfflineWhenStopped): n/a

#### `TEST(MMReplicationManagerTest, TopologySnapshotWithPeer)`
- Source: `tests/replication/test_replication_ha.cpp`:1611
- Brief: n/a
- Parameters:
  - `<unnamed>` (MMReplicationManagerTest): n/a
  - `<unnamed>` (TopologySnapshotWithPeer): n/a

#### `TEST(MMReplicationManagerTest, TriggerSyncDoesNotCrash)`
- Source: `tests/replication/test_replication_ha.cpp`:1500
- Brief: n/a
- Parameters:
  - `<unnamed>` (MMReplicationManagerTest): n/a
  - `<unnamed>` (TriggerSyncDoesNotCrash): n/a

#### `TEST(MMReplicationManagerTest, WriteCallbackInvokedAfterReplication)`
- Source: `tests/replication/test_replication_ha.cpp`:1532
- Brief: n/a
- Parameters:
  - `<unnamed>` (MMReplicationManagerTest): n/a
  - `<unnamed>` (WriteCallbackInvokedAfterReplication): n/a

#### `TEST(MMReplicationManagerTest, WriteReturnsWriteId)`
- Source: `tests/replication/test_replication_ha.cpp`:1338
- Brief: n/a
- Parameters:
  - `<unnamed>` (MMReplicationManagerTest): n/a
  - `<unnamed>` (WriteReturnsWriteId): n/a

#### `TEST(MMReplicationManagerTest, WriteSyncCompletesWithinTimeout)`
- Source: `tests/replication/test_replication_ha.cpp`:1349
- Brief: n/a
- Parameters:
  - `<unnamed>` (MMReplicationManagerTest): n/a
  - `<unnamed>` (WriteSyncCompletesWithinTimeout): n/a

#### `TEST(MMReplicationManagerTest, WriteSyncFailsClosedWhenQuorumZeroWithActivePeer)`
- Source: `tests/replication/test_replication_ha.cpp`:1508
- Brief: n/a
- Parameters:
  - `<unnamed>` (MMReplicationManagerTest): n/a
  - `<unnamed>` (WriteSyncFailsClosedWhenQuorumZeroWithActivePeer): n/a

#### `TEST(MMWriteEntryTest, SerializeDeserializeRoundTrip)`
- Source: `tests/replication/test_replication_ha.cpp`:1251
- Brief: n/a
- Parameters:
  - `<unnamed>` (MMWriteEntryTest): n/a
  - `<unnamed>` (SerializeDeserializeRoundTrip): n/a

#### `TEST(MultiTierReplicationTest, AssignTierOverridesExistingAssignment)`
- Source: `tests/replication/test_replication_ha.cpp`:5527
- Brief: n/a
- Parameters:
  - `<unnamed>` (MultiTierReplicationTest): n/a
  - `<unnamed>` (AssignTierOverridesExistingAssignment): n/a

#### `TEST(MultiTierReplicationTest, AssignTierPersistsAndIsRetrievable)`
- Source: `tests/replication/test_replication_ha.cpp`:5507
- Brief: n/a
- Parameters:
  - `<unnamed>` (MultiTierReplicationTest): n/a
  - `<unnamed>` (AssignTierPersistsAndIsRetrievable): n/a

#### `TEST(MultiTierReplicationTest, AutoTieringDisabledByDefault)`
- Source: `tests/replication/test_replication_ha.cpp`:5596
- Brief: n/a
- Parameters:
  - `<unnamed>` (MultiTierReplicationTest): n/a
  - `<unnamed>` (AutoTieringDisabledByDefault): n/a

#### `TEST(MultiTierReplicationTest, ColdCollectionDemotedToTier3ByAutoTiering)`
- Source: `tests/replication/test_replication_ha.cpp`:5650
- Brief: n/a
- Parameters:
  - `<unnamed>` (MultiTierReplicationTest): n/a
  - `<unnamed>` (ColdCollectionDemotedToTier3ByAutoTiering): n/a

#### `TEST(MultiTierReplicationTest, CustomTierConfigOverridesBuiltinDefaults)`
- Source: `tests/replication/test_replication_ha.cpp`:5577
- Brief: n/a
- Parameters:
  - `<unnamed>` (MultiTierReplicationTest): n/a
  - `<unnamed>` (CustomTierConfigOverridesBuiltinDefaults): n/a

#### `TEST(MultiTierReplicationTest, EnableAutoTieringToggleWorks)`
- Source: `tests/replication/test_replication_ha.cpp`:5601
- Brief: n/a
- Parameters:
  - `<unnamed>` (MultiTierReplicationTest): n/a
  - `<unnamed>` (EnableAutoTieringToggleWorks): n/a

#### `TEST(MultiTierReplicationTest, EvaluateTierNoChangeWhenAutoTieringDisabled)`
- Source: `tests/replication/test_replication_ha.cpp`:5689
- Brief: n/a
- Parameters:
  - `<unnamed>` (MultiTierReplicationTest): n/a
  - `<unnamed>` (EvaluateTierNoChangeWhenAutoTieringDisabled): n/a

#### `TEST(MultiTierReplicationTest, GetCollectionStatsIncludesAllTrackedCollections)`
- Source: `tests/replication/test_replication_ha.cpp`:5740
- Brief: n/a
- Parameters:
  - `<unnamed>` (MultiTierReplicationTest): n/a
  - `<unnamed>` (GetCollectionStatsIncludesAllTrackedCollections): n/a

#### `TEST(MultiTierReplicationTest, GetCollectionsForTierReturnsCorrectSubset)`
- Source: `tests/replication/test_replication_ha.cpp`:5558
- Brief: n/a
- Parameters:
  - `<unnamed>` (MultiTierReplicationTest): n/a
  - `<unnamed>` (GetCollectionsForTierReturnsCorrectSubset): n/a

#### `TEST(MultiTierReplicationTest, GetStatsCountsPromotionsAndDemotions)`
- Source: `tests/replication/test_replication_ha.cpp`:5714
- Brief: n/a
- Parameters:
  - `<unnamed>` (MultiTierReplicationTest): n/a
  - `<unnamed>` (GetStatsCountsPromotionsAndDemotions): n/a

#### `TEST(MultiTierReplicationTest, GetStatsReflectsAssignments)`
- Source: `tests/replication/test_replication_ha.cpp`:5700
- Brief: n/a
- Parameters:
  - `<unnamed>` (MultiTierReplicationTest): n/a
  - `<unnamed>` (GetStatsReflectsAssignments): n/a

#### `TEST(MultiTierReplicationTest, GetTierConfigReflectsAssignedTier)`
- Source: `tests/replication/test_replication_ha.cpp`:5548
- Brief: n/a
- Parameters:
  - `<unnamed>` (MultiTierReplicationTest): n/a
  - `<unnamed>` (GetTierConfigReflectsAssignedTier): n/a

#### `TEST(MultiTierReplicationTest, HotCollectionPromotedToTier1ByAutoTiering)`
- Source: `tests/replication/test_replication_ha.cpp`:5630
- Brief: n/a
- Parameters:
  - `<unnamed>` (MultiTierReplicationTest): n/a
  - `<unnamed>` (HotCollectionPromotedToTier1ByAutoTiering): n/a

#### `TEST(MultiTierReplicationTest, ModerateAccessCollectionNormalisedToTier2)`
- Source: `tests/replication/test_replication_ha.cpp`:5670
- Brief: n/a
- Parameters:
  - `<unnamed>` (MultiTierReplicationTest): n/a
  - `<unnamed>` (ModerateAccessCollectionNormalisedToTier2): n/a

#### `TEST(MultiTierReplicationTest, RecordAccessHasNoEffectWhenAutoTieringDisabled)`
- Source: `tests/replication/test_replication_ha.cpp`:5609
- Brief: n/a
- Parameters:
  - `<unnamed>` (MultiTierReplicationTest): n/a
  - `<unnamed>` (RecordAccessHasNoEffectWhenAutoTieringDisabled): n/a

#### `TEST(MultiTierReplicationTest, RemoveTierFallsBackToDefault)`
- Source: `tests/replication/test_replication_ha.cpp`:5536
- Brief: n/a
- Parameters:
  - `<unnamed>` (MultiTierReplicationTest): n/a
  - `<unnamed>` (RemoveTierFallsBackToDefault): n/a

#### `TEST(MultiTierReplicationTest, Tier1DefaultConfigHasStrongConsistency)`
- Source: `tests/replication/test_replication_ha.cpp`:5472
- Brief: n/a
- Parameters:
  - `<unnamed>` (MultiTierReplicationTest): n/a
  - `<unnamed>` (Tier1DefaultConfigHasStrongConsistency): n/a

#### `TEST(MultiTierReplicationTest, Tier2DefaultConfigHasModerateConsistency)`
- Source: `tests/replication/test_replication_ha.cpp`:5484
- Brief: n/a
- Parameters:
  - `<unnamed>` (MultiTierReplicationTest): n/a
  - `<unnamed>` (Tier2DefaultConfigHasModerateConsistency): n/a

#### `TEST(MultiTierReplicationTest, Tier3DefaultConfigHasAsyncBestEffort)`
- Source: `tests/replication/test_replication_ha.cpp`:5496
- Brief: n/a
- Parameters:
  - `<unnamed>` (MultiTierReplicationTest): n/a
  - `<unnamed>` (Tier3DefaultConfigHasAsyncBestEffort): n/a

#### `TEST(MultiTierReplicationTest, UnassignedCollectionReturnsDefaultTier)`
- Source: `tests/replication/test_replication_ha.cpp`:5519
- Brief: n/a
- Parameters:
  - `<unnamed>` (MultiTierReplicationTest): n/a
  - `<unnamed>` (UnassignedCollectionReturnsDefaultTier): n/a

#### `TEST(ParallelReplicationWorkerTest, DependencyTrackingSerialisesPerDocument)`
- Source: `tests/replication/test_replication_ha.cpp`:1710
- Brief: n/a
- Parameters:
  - `<unnamed>` (ParallelReplicationWorkerTest): n/a
  - `<unnamed>` (DependencyTrackingSerialisesPerDocument): n/a

#### `TEST(ParallelReplicationWorkerTest, StatsReflectParallelism)`
- Source: `tests/replication/test_replication_ha.cpp`:1753
- Brief: n/a
- Parameters:
  - `<unnamed>` (ParallelReplicationWorkerTest): n/a
  - `<unnamed>` (StatsReflectParallelism): n/a

#### `TEST(ParallelReplicationWorkerTest, StopJoinsCleanly)`
- Source: `tests/replication/test_replication_ha.cpp`:1739
- Brief: n/a
- Parameters:
  - `<unnamed>` (ParallelReplicationWorkerTest): n/a
  - `<unnamed>` (StopJoinsCleanly): n/a

#### `TEST(ParallelReplicationWorkerTest, SubmitAndSync)`
- Source: `tests/replication/test_replication_ha.cpp`:1685
- Brief: n/a
- Parameters:
  - `<unnamed>` (ParallelReplicationWorkerTest): n/a
  - `<unnamed>` (SubmitAndSync): n/a

#### `TEST(QuorumReadManagerTest, DetectsConflictsOnDivergence)`
- Source: `tests/replication/test_replication_ha.cpp`:1834
- Brief: n/a
- Parameters:
  - `<unnamed>` (QuorumReadManagerTest): n/a
  - `<unnamed>` (DetectsConflictsOnDivergence): n/a

#### `TEST(QuorumReadManagerTest, FailsWhenNoReplicasAvailable)`
- Source: `tests/replication/test_replication_ha.cpp`:1851
- Brief: n/a
- Parameters:
  - `<unnamed>` (QuorumReadManagerTest): n/a
  - `<unnamed>` (FailsWhenNoReplicasAvailable): n/a

#### `TEST(QuorumReadManagerTest, PicksHighestVersion)`
- Source: `tests/replication/test_replication_ha.cpp`:1815
- Brief: n/a
- Parameters:
  - `<unnamed>` (QuorumReadManagerTest): n/a
  - `<unnamed>` (PicksHighestVersion): n/a

#### `TEST(QuorumReadManagerTest, RepairOnRead_ConflictsDetectedAndFlagged)`
- Source: `tests/replication/test_replication_ha.cpp`:2021
- Brief: n/a
- Parameters:
  - `<unnamed>` (QuorumReadManagerTest): n/a
  - `<unnamed>` (RepairOnRead_ConflictsDetectedAndFlagged): n/a

#### `TEST(QuorumReadManagerTest, SessionConsistency_FailsWhenReplicasBelowRequiredVersion)`
- Source: `tests/replication/test_replication_ha.cpp`:1946
- Brief: n/a
- Parameters:
  - `<unnamed>` (QuorumReadManagerTest): n/a
  - `<unnamed>` (SessionConsistency_FailsWhenReplicasBelowRequiredVersion): n/a

#### `TEST(QuorumReadManagerTest, SessionConsistency_FreshReplicasLateInIterationOrder)`
- Source: `tests/replication/test_replication_ha.cpp`:2150
- Brief: n/a
- Parameters:
  - `<unnamed>` (QuorumReadManagerTest): n/a
  - `<unnamed>` (SessionConsistency_FreshReplicasLateInIterationOrder): n/a

#### `TEST(QuorumReadManagerTest, SessionConsistency_MalformedTokenTreatedAsNoRequirement)`
- Source: `tests/replication/test_replication_ha.cpp`:2126
- Brief: n/a
- Parameters:
  - `<unnamed>` (QuorumReadManagerTest): n/a
  - `<unnamed>` (SessionConsistency_MalformedTokenTreatedAsNoRequirement): n/a

#### `TEST(QuorumReadManagerTest, SessionConsistency_PartialVersionSatisfactionFails)`
- Source: `tests/replication/test_replication_ha.cpp`:1972
- Brief: n/a
- Parameters:
  - `<unnamed>` (QuorumReadManagerTest): n/a
  - `<unnamed>` (SessionConsistency_PartialVersionSatisfactionFails): n/a

#### `TEST(QuorumReadManagerTest, SessionConsistency_QuorumSatisfiedByFreshReplicas)`
- Source: `tests/replication/test_replication_ha.cpp`:1995
- Brief: n/a
- Parameters:
  - `<unnamed>` (QuorumReadManagerTest): n/a
  - `<unnamed>` (SessionConsistency_QuorumSatisfiedByFreshReplicas): n/a

#### `TEST(QuorumReadManagerTest, SessionConsistency_TokenFromReadUsedInNextRead)`
- Source: `tests/replication/test_replication_ha.cpp`:1917
- Brief: n/a
- Parameters:
  - `<unnamed>` (QuorumReadManagerTest): n/a
  - `<unnamed>` (SessionConsistency_TokenFromReadUsedInNextRead): n/a

#### `TEST(QuorumReadManagerTest, SessionToken_ReturnedOnSuccessfulRead)`
- Source: `tests/replication/test_replication_ha.cpp`:1891
- Brief: n/a
- Parameters:
  - `<unnamed>` (QuorumReadManagerTest): n/a
  - `<unnamed>` (SessionToken_ReturnedOnSuccessfulRead): n/a

#### `TEST(QuorumReadManagerTest, SetReplicasUpdatesTopology)`
- Source: `tests/replication/test_replication_ha.cpp`:1879
- Brief: n/a
- Parameters:
  - `<unnamed>` (QuorumReadManagerTest): n/a
  - `<unnamed>` (SetReplicasUpdatesTopology): n/a

#### `TEST(QuorumReadManagerTest, SingleNodeLocalFetch_ClearingRevertsToStub)`
- Source: `tests/replication/test_replication_ha.cpp`:2084
- Brief: n/a
- Parameters:
  - `<unnamed>` (QuorumReadManagerTest): n/a
  - `<unnamed>` (SingleNodeLocalFetch_ClearingRevertsToStub): n/a

#### `TEST(QuorumReadManagerTest, SingleNodeLocalFetch_InjectedFnCalled)`
- Source: `tests/replication/test_replication_ha.cpp`:2059
- Brief: n/a
- Parameters:
  - `<unnamed>` (QuorumReadManagerTest): n/a
  - `<unnamed>` (SingleNodeLocalFetch_InjectedFnCalled): n/a

#### `TEST(QuorumReadManagerTest, SingleNodeLocalFetch_ThrowingFnDegradesGracefully)`
- Source: `tests/replication/test_replication_ha.cpp`:2108
- Brief: n/a
- Parameters:
  - `<unnamed>` (QuorumReadManagerTest): n/a
  - `<unnamed>` (SingleNodeLocalFetch_ThrowingFnDegradesGracefully): n/a

#### `TEST(QuorumReadManagerTest, SingleNodeModeSucceeds)`
- Source: `tests/replication/test_replication_ha.cpp`:1869
- Brief: n/a
- Parameters:
  - `<unnamed>` (QuorumReadManagerTest): n/a
  - `<unnamed>` (SingleNodeModeSucceeds): n/a

#### `TEST(QuorumReadManagerTest, SingleNodeMode_ReturnsSessionToken)`
- Source: `tests/replication/test_replication_ha.cpp`:2044
- Brief: n/a
- Parameters:
  - `<unnamed>` (QuorumReadManagerTest): n/a
  - `<unnamed>` (SingleNodeMode_ReturnsSessionToken): n/a

#### `TEST(QuorumReadManagerTest, SucceedsWithQuorumReplicas)`
- Source: `tests/replication/test_replication_ha.cpp`:1797
- Brief: n/a
- Parameters:
  - `<unnamed>` (QuorumReadManagerTest): n/a
  - `<unnamed>` (SucceedsWithQuorumReplicas): n/a

#### `TEST(ReplicationAnalyticsTest, BottleneckDetectionNetwork)`
- Source: `tests/replication/test_replication_ha.cpp`:2855
- Brief: n/a
- Parameters:
  - `<unnamed>` (ReplicationAnalyticsTest): n/a
  - `<unnamed>` (BottleneckDetectionNetwork): n/a

#### `TEST(ReplicationAnalyticsTest, ConcurrentRecordIsThreadSafe)`
- Source: `tests/replication/test_replication_ha.cpp`:2892
- Brief: n/a
- Parameters:
  - `<unnamed>` (ReplicationAnalyticsTest): n/a
  - `<unnamed>` (ConcurrentRecordIsThreadSafe): n/a

#### `TEST(ReplicationAnalyticsTest, LagSpikeInsightGenerated)`
- Source: `tests/replication/test_replication_ha.cpp`:2816
- Brief: n/a
- Parameters:
  - `<unnamed>` (ReplicationAnalyticsTest): n/a
  - `<unnamed>` (LagSpikeInsightGenerated): n/a

#### `TEST(ReplicationAnalyticsTest, NoInsightForNormalLag)`
- Source: `tests/replication/test_replication_ha.cpp`:2831
- Brief: n/a
- Parameters:
  - `<unnamed>` (ReplicationAnalyticsTest): n/a
  - `<unnamed>` (NoInsightForNormalLag): n/a

#### `TEST(ReplicationAnalyticsTest, PrometheusExportContainsLag)`
- Source: `tests/replication/test_replication_ha.cpp`:2875
- Brief: n/a
- Parameters:
  - `<unnamed>` (ReplicationAnalyticsTest): n/a
  - `<unnamed>` (PrometheusExportContainsLag): n/a

#### `TEST(ReplicationAnalyticsTest, RecordAndGetLagHistory)`
- Source: `tests/replication/test_replication_ha.cpp`:2803
- Brief: n/a
- Parameters:
  - `<unnamed>` (ReplicationAnalyticsTest): n/a
  - `<unnamed>` (RecordAndGetLagHistory): n/a

#### `TEST(ReplicationAnalyticsTest, SlowReplicaInsightGenerated)`
- Source: `tests/replication/test_replication_ha.cpp`:2839
- Brief: n/a
- Parameters:
  - `<unnamed>` (ReplicationAnalyticsTest): n/a
  - `<unnamed>` (SlowReplicaInsightGenerated): n/a

#### `TEST(ReplicationAnalyticsTest, UnknownReplicaReturnsEmptyHistory)`
- Source: `tests/replication/test_replication_ha.cpp`:2885
- Brief: n/a
- Parameters:
  - `<unnamed>` (ReplicationAnalyticsTest): n/a
  - `<unnamed>` (UnknownReplicaReturnsEmptyHistory): n/a

#### `TEST(ReplicationBenchmarkTest, DefaultConstructorWorks)`
- Source: `tests/replication/test_replication_ha.cpp`:2954
- Brief: n/a
- Parameters:
  - `<unnamed>` (ReplicationBenchmarkTest): n/a
  - `<unnamed>` (DefaultConstructorWorks): n/a

#### `TEST(ReplicationBenchmarkTest, FormatContainsKeyFields)`
- Source: `tests/replication/test_replication_ha.cpp`:2978
- Brief: n/a
- Parameters:
  - `<unnamed>` (ReplicationBenchmarkTest): n/a
  - `<unnamed>` (FormatContainsKeyFields): n/a

#### `TEST(ReplicationBenchmarkTest, LatencyPercentilesAreSorted)`
- Source: `tests/replication/test_replication_ha.cpp`:2996
- Brief: n/a
- Parameters:
  - `<unnamed>` (ReplicationBenchmarkTest): n/a
  - `<unnamed>` (LatencyPercentilesAreSorted): n/a

#### `TEST(ReplicationBenchmarkTest, RunProducesPositiveThroughput)`
- Source: `tests/replication/test_replication_ha.cpp`:2920
- Brief: n/a
- Parameters:
  - `<unnamed>` (ReplicationBenchmarkTest): n/a
  - `<unnamed>` (RunProducesPositiveThroughput): n/a

#### `TEST(ReplicationManagerErrorHandling, ReplicateAsFollowerFails)`
- Source: `tests/replication/test_replication_ha.cpp`:560
- Brief: n/a
- Parameters:
  - `<unnamed>` (ReplicationManagerErrorHandling): n/a
  - `<unnamed>` (ReplicateAsFollowerFails): n/a

#### `TEST(ReplicationManagerErrorHandling, ReplicateBeforeInitFails)`
- Source: `tests/replication/test_replication_ha.cpp`:543
- Brief: n/a
- Parameters:
  - `<unnamed>` (ReplicationManagerErrorHandling): n/a
  - `<unnamed>` (ReplicateBeforeInitFails): n/a

#### `TEST(ReplicationManagerErrorHandling, SemiSyncImpossibleQuorumFailsClosed)`
- Source: `tests/replication/test_replication_ha.cpp`:604
- Brief: n/a
- Parameters:
  - `<unnamed>` (ReplicationManagerErrorHandling): n/a
  - `<unnamed>` (SemiSyncImpossibleQuorumFailsClosed): n/a

#### `TEST(ReplicationManagerErrorHandling, SyncModeWithoutReplicaStreamsFailsClosed)`
- Source: `tests/replication/test_replication_ha.cpp`:583
- Brief: n/a
- Parameters:
  - `<unnamed>` (ReplicationManagerErrorHandling): n/a
  - `<unnamed>` (SyncModeWithoutReplicaStreamsFailsClosed): n/a

#### `TEST(ReplicationManagerThreadSafety, ConcurrentAddRemoveReplicas)`
- Source: `tests/replication/test_replication_ha.cpp`:488
- Brief: n/a
- Parameters:
  - `<unnamed>` (ReplicationManagerThreadSafety): n/a
  - `<unnamed>` (ConcurrentAddRemoveReplicas): n/a

#### `TEST(ReplicationStreamTest, InitialBackoffIsZero)`
- Source: `tests/replication/test_replication_ha.cpp`:1290
- Brief: n/a
- Parameters:
  - `<unnamed>` (ReplicationStreamTest): n/a
  - `<unnamed>` (InitialBackoffIsZero): n/a

#### `TEST(VectorClockPerfTest, IncrementAndCompareSingleOpUnder5us)`
- Source: `tests/replication/test_replication_ha.cpp`:5790
- Brief: VectorClock increment + compare combined latency < 5 µs.
- Parameters:
  - `<unnamed>` (VectorClockPerfTest): n/a
  - `<unnamed>` (IncrementAndCompareSingleOpUnder5us): n/a
- Details: Design Constraint #4 (FUTURE_ENHANCEMENTS.md): "Vector clock comparison and HLC conflict detection must add < 5 µs per write operation." This test measures the median cost of one VectorClock::increment() followed by one VectorClock::compare() — i.e. the overhead added to every write on the multi-master write path. Set THEMIS_RUN_PERF_TESTS=1 to enable.

#### `TEST(VectorClockTest, Compare_Concurrent)`
- Source: `tests/replication/test_replication_ha.cpp`:744
- Brief: n/a
- Parameters:
  - `<unnamed>` (VectorClockTest): n/a
  - `<unnamed>` (Compare_Concurrent): n/a

#### `TEST(VectorClockTest, Compare_EqualIsConcurrent)`
- Source: `tests/replication/test_replication_ha.cpp`:756
- Brief: n/a
- Parameters:
  - `<unnamed>` (VectorClockTest): n/a
  - `<unnamed>` (Compare_EqualIsConcurrent): n/a

#### `TEST(VectorClockTest, Compare_HappensBefore)`
- Source: `tests/replication/test_replication_ha.cpp`:731
- Brief: n/a
- Parameters:
  - `<unnamed>` (VectorClockTest): n/a
  - `<unnamed>` (Compare_HappensBefore): n/a

#### `TEST(VectorClockTest, CopyConstructor)`
- Source: `tests/replication/test_replication_ha.cpp`:778
- Brief: n/a
- Parameters:
  - `<unnamed>` (VectorClockTest): n/a
  - `<unnamed>` (CopyConstructor): n/a

#### `TEST(VectorClockTest, IncrementAndGet)`
- Source: `tests/replication/test_replication_ha.cpp`:702
- Brief: n/a
- Parameters:
  - `<unnamed>` (VectorClockTest): n/a
  - `<unnamed>` (IncrementAndGet): n/a

#### `TEST(VectorClockTest, MergeInHAContext)`
- Source: `tests/replication/test_replication_ha.cpp`:716
- Brief: n/a
- Parameters:
  - `<unnamed>` (VectorClockTest): n/a
  - `<unnamed>` (MergeInHAContext): n/a

#### `TEST(VectorClockTest, ToJsonRoundTrip)`
- Source: `tests/replication/test_replication_ha.cpp`:764
- Brief: n/a
- Parameters:
  - `<unnamed>` (VectorClockTest): n/a
  - `<unnamed>` (ToJsonRoundTrip): n/a

#### `TEST(WALAppendThroughputPerfTest, Over50kEntriesPerSecond)`
- Source: `tests/replication/test_replication_ha.cpp`:5889
- Brief: WAL append throughput > 50,000 entries/s (prerequisite for Design Constraint #1: replication lag p99 ≤ 50 ms at 10,000 write/s).
- Parameters:
  - `<unnamed>` (WALAppendThroughputPerfTest): n/a
  - `<unnamed>` (Over50kEntriesPerSecond): n/a
- Details: Set THEMIS_RUN_PERF_TESTS=1 to enable.

#### `TEST(WALArchivalTest, ArchiveSingleSegmentAndRetrieve)`
- Source: `tests/replication/test_replication_ha.cpp`:3174
- Brief: n/a
- Parameters:
  - `<unnamed>` (WALArchivalTest): n/a
  - `<unnamed>` (ArchiveSingleSegmentAndRetrieve): n/a

#### `TEST(WALArchivalTest, ArchiveWithoutCompressionRoundTrip)`
- Source: `tests/replication/test_replication_ha.cpp`:3207
- Brief: n/a
- Parameters:
  - `<unnamed>` (WALArchivalTest): n/a
  - `<unnamed>` (ArchiveWithoutCompressionRoundTrip): n/a

#### `TEST(WALArchivalTest, BackendInjection_ArchiveAndRetrieveViaBackend)`
- Source: `tests/replication/test_replication_ha.cpp`:3685
- Brief: n/a
- Parameters:
  - `<unnamed>` (WALArchivalTest): n/a
  - `<unnamed>` (BackendInjection_ArchiveAndRetrieveViaBackend): n/a

#### `TEST(WALArchivalTest, BackendInjection_PurgeDeletesViaBackend)`
- Source: `tests/replication/test_replication_ha.cpp`:3726
- Brief: n/a
- Parameters:
  - `<unnamed>` (WALArchivalTest): n/a
  - `<unnamed>` (BackendInjection_PurgeDeletesViaBackend): n/a

#### `TEST(WALArchivalTest, BackendInjection_TransitionTierNotifiesBackend)`
- Source: `tests/replication/test_replication_ha.cpp`:3756
- Brief: n/a
- Parameters:
  - `<unnamed>` (WALArchivalTest): n/a
  - `<unnamed>` (BackendInjection_TransitionTierNotifiesBackend): n/a

#### `TEST(WALArchivalTest, EncryptionAtRest_EmptyKey_RejectsArchival)`
- Source: `tests/replication/test_replication_ha.cpp`:3616
- Brief: n/a
- Parameters:
  - `<unnamed>` (WALArchivalTest): n/a
  - `<unnamed>` (EncryptionAtRest_EmptyKey_RejectsArchival): n/a

#### `TEST(WALArchivalTest, EncryptionAtRest_InvalidKey_RejectsArchival)`
- Source: `tests/replication/test_replication_ha.cpp`:3423
- Brief: n/a
- Parameters:
  - `<unnamed>` (WALArchivalTest): n/a
  - `<unnamed>` (EncryptionAtRest_InvalidKey_RejectsArchival): n/a

#### `TEST(WALArchivalTest, EncryptionAtRest_RoundTrip)`
- Source: `tests/replication/test_replication_ha.cpp`:3343
- Brief: n/a
- Parameters:
  - `<unnamed>` (WALArchivalTest): n/a
  - `<unnamed>` (EncryptionAtRest_RoundTrip): n/a

#### `TEST(WALArchivalTest, EncryptionAtRest_WithCompression_RoundTrip)`
- Source: `tests/replication/test_replication_ha.cpp`:3388
- Brief: n/a
- Parameters:
  - `<unnamed>` (WALArchivalTest): n/a
  - `<unnamed>` (EncryptionAtRest_WithCompression_RoundTrip): n/a

#### `TEST(WALArchivalTest, IndexPersistence_TierAndEncryptionFields)`
- Source: `tests/replication/test_replication_ha.cpp`:3572
- Brief: n/a
- Parameters:
  - `<unnamed>` (WALArchivalTest): n/a
  - `<unnamed>` (IndexPersistence_TierAndEncryptionFields): n/a

#### `TEST(WALArchivalTest, ListArchivedSortedBySegmentId)`
- Source: `tests/replication/test_replication_ha.cpp`:3251
- Brief: n/a
- Parameters:
  - `<unnamed>` (WALArchivalTest): n/a
  - `<unnamed>` (ListArchivedSortedBySegmentId): n/a

#### `TEST(WALArchivalTest, MissingSegmentReturnsNullopt)`
- Source: `tests/replication/test_replication_ha.cpp`:3236
- Brief: n/a
- Parameters:
  - `<unnamed>` (WALArchivalTest): n/a
  - `<unnamed>` (MissingSegmentReturnsNullopt): n/a

#### `TEST(WALArchivalTest, PurgeExpiredRemovesOldSegments)`
- Source: `tests/replication/test_replication_ha.cpp`:3278
- Brief: n/a
- Parameters:
  - `<unnamed>` (WALArchivalTest): n/a
  - `<unnamed>` (PurgeExpiredRemovesOldSegments): n/a

#### `TEST(WALArchivalTest, RunArchivalCycleArchivesOldSegments)`
- Source: `tests/replication/test_replication_ha.cpp`:3305
- Brief: n/a
- Parameters:
  - `<unnamed>` (WALArchivalTest): n/a
  - `<unnamed>` (RunArchivalCycleArchivesOldSegments): n/a

#### `TEST(WALArchivalTest, StorageTier_DefaultIsStandard)`
- Source: `tests/replication/test_replication_ha.cpp`:3448
- Brief: n/a
- Parameters:
  - `<unnamed>` (WALArchivalTest): n/a
  - `<unnamed>` (StorageTier_DefaultIsStandard): n/a

#### `TEST(WALArchivalTest, TransitionStorageTiers_DisabledWhenZero)`
- Source: `tests/replication/test_replication_ha.cpp`:3472
- Brief: n/a
- Parameters:
  - `<unnamed>` (WALArchivalTest): n/a
  - `<unnamed>` (TransitionStorageTiers_DisabledWhenZero): n/a

#### `TEST(WALArchivalTest, TransitionStorageTiers_MovesToCold)`
- Source: `tests/replication/test_replication_ha.cpp`:3497
- Brief: n/a
- Parameters:
  - `<unnamed>` (WALArchivalTest): n/a
  - `<unnamed>` (TransitionStorageTiers_MovesToCold): n/a

#### `TEST(WALArchivalTest, TransitionStorageTiers_MovesToGlacier)`
- Source: `tests/replication/test_replication_ha.cpp`:3538
- Brief: n/a
- Parameters:
  - `<unnamed>` (WALArchivalTest): n/a
  - `<unnamed>` (TransitionStorageTiers_MovesToGlacier): n/a

#### `TEST_F(CRDTResolverTest, EmptyLocalReturnsRemote)`
- Source: `tests/replication/test_replication_ha.cpp`:333
- Brief: n/a
- Parameters:
  - `<unnamed>` (CRDTResolverTest): n/a
  - `<unnamed>` (EmptyLocalReturnsRemote): n/a

#### `TEST_F(CRDTResolverTest, EmptyRemoteReturnsLocal)`
- Source: `tests/replication/test_replication_ha.cpp`:338
- Brief: n/a
- Parameters:
  - `<unnamed>` (CRDTResolverTest): n/a
  - `<unnamed>` (EmptyRemoteReturnsLocal): n/a

#### `TEST_F(CRDTResolverTest, MergeKeepsMaxNumericValue)`
- Source: `tests/replication/test_replication_ha.cpp`:320
- Brief: n/a
- Parameters:
  - `<unnamed>` (CRDTResolverTest): n/a
  - `<unnamed>` (MergeKeepsMaxNumericValue): n/a

#### `TEST_F(CrossClusterPublicationTest, AddAndRemoveRemoteSubscriber)`
- Source: `tests/replication/test_replication_ha.cpp`:4079
- Brief: n/a
- Parameters:
  - `<unnamed>` (CrossClusterPublicationTest): n/a
  - `<unnamed>` (AddAndRemoveRemoteSubscriber): n/a

#### `TEST_F(CrossClusterPublicationTest, FilteredPublishDropsNonMatchingEntries)`
- Source: `tests/replication/test_replication_ha.cpp`:4110
- Brief: n/a
- Parameters:
  - `<unnamed>` (CrossClusterPublicationTest): n/a
  - `<unnamed>` (FilteredPublishDropsNonMatchingEntries): n/a

#### `TEST_F(CrossClusterPublicationTest, InitialStateHasNoSubscribersAndZeroPublished)`
- Source: `tests/replication/test_replication_ha.cpp`:4073
- Brief: n/a
- Parameters:
  - `<unnamed>` (CrossClusterPublicationTest): n/a
  - `<unnamed>` (InitialStateHasNoSubscribersAndZeroPublished): n/a

#### `TEST_F(CrossClusterPublicationTest, NameIsPreserved)`
- Source: `tests/replication/test_replication_ha.cpp`:4068
- Brief: n/a
- Parameters:
  - `<unnamed>` (CrossClusterPublicationTest): n/a
  - `<unnamed>` (NameIsPreserved): n/a

#### `TEST_F(CrossClusterPublicationTest, OnWALEntryAppliedFeedsPublish)`
- Source: `tests/replication/test_replication_ha.cpp`:4140
- Brief: n/a
- Parameters:
  - `<unnamed>` (CrossClusterPublicationTest): n/a
  - `<unnamed>` (OnWALEntryAppliedFeedsPublish): n/a

#### `TEST_F(CrossClusterPublicationTest, PublishDeliversEntryToSubscriber)`
- Source: `tests/replication/test_replication_ha.cpp`:4095
- Brief: n/a
- Parameters:
  - `<unnamed>` (CrossClusterPublicationTest): n/a
  - `<unnamed>` (PublishDeliversEntryToSubscriber): n/a

#### `TEST_F(CrossClusterPublicationTest, PublishDeliversToMultipleSubscribers)`
- Source: `tests/replication/test_replication_ha.cpp`:4127
- Brief: n/a
- Parameters:
  - `<unnamed>` (CrossClusterPublicationTest): n/a
  - `<unnamed>` (PublishDeliversToMultipleSubscribers): n/a

#### `TEST_F(CrossClusterPublicationTest, SetFilterThreadSafe)`
- Source: `tests/replication/test_replication_ha.cpp`:4154
- Brief: n/a
- Parameters:
  - `<unnamed>` (CrossClusterPublicationTest): n/a
  - `<unnamed>` (SetFilterThreadSafe): n/a

#### `TEST_F(CrossClusterSubscriptionTest, ApplyErrorCountedAndDoesNotStop)`
- Source: `tests/replication/test_replication_ha.cpp`:4257
- Brief: n/a
- Parameters:
  - `<unnamed>` (CrossClusterSubscriptionTest): n/a
  - `<unnamed>` (ApplyErrorCountedAndDoesNotStop): n/a

#### `TEST_F(CrossClusterSubscriptionTest, DestructorAutoDisables)`
- Source: `tests/replication/test_replication_ha.cpp`:4291
- Brief: n/a
- Parameters:
  - `<unnamed>` (CrossClusterSubscriptionTest): n/a
  - `<unnamed>` (DestructorAutoDisables): n/a

#### `TEST_F(CrossClusterSubscriptionTest, DisableIdempotent)`
- Source: `tests/replication/test_replication_ha.cpp`:4316
- Brief: n/a
- Parameters:
  - `<unnamed>` (CrossClusterSubscriptionTest): n/a
  - `<unnamed>` (DisableIdempotent): n/a

#### `TEST_F(CrossClusterSubscriptionTest, DisabledSubscriptionReceivesNothing)`
- Source: `tests/replication/test_replication_ha.cpp`:4244
- Brief: n/a
- Parameters:
  - `<unnamed>` (CrossClusterSubscriptionTest): n/a
  - `<unnamed>` (DisabledSubscriptionReceivesNothing): n/a

#### `TEST_F(CrossClusterSubscriptionTest, EnableIdempotent)`
- Source: `tests/replication/test_replication_ha.cpp`:4303
- Brief: n/a
- Parameters:
  - `<unnamed>` (CrossClusterSubscriptionTest): n/a
  - `<unnamed>` (EnableIdempotent): n/a

#### `TEST_F(CrossClusterSubscriptionTest, EnableRegistersWithPublication)`
- Source: `tests/replication/test_replication_ha.cpp`:4211
- Brief: n/a
- Parameters:
  - `<unnamed>` (CrossClusterSubscriptionTest): n/a
  - `<unnamed>` (EnableRegistersWithPublication): n/a

#### `TEST_F(CrossClusterSubscriptionTest, EnabledSubscriptionReceivesEntries)`
- Source: `tests/replication/test_replication_ha.cpp`:4226
- Brief: n/a
- Parameters:
  - `<unnamed>` (CrossClusterSubscriptionTest): n/a
  - `<unnamed>` (EnabledSubscriptionReceivesEntries): n/a

#### `TEST_F(CrossClusterSubscriptionTest, InitiallyDisabled)`
- Source: `tests/replication/test_replication_ha.cpp`:4203
- Brief: n/a
- Parameters:
  - `<unnamed>` (CrossClusterSubscriptionTest): n/a
  - `<unnamed>` (InitiallyDisabled): n/a

#### `TEST_F(CrossClusterSubscriptionTest, LastAppliedSequenceTracksHighestApplied)`
- Source: `tests/replication/test_replication_ha.cpp`:4278
- Brief: n/a
- Parameters:
  - `<unnamed>` (CrossClusterSubscriptionTest): n/a
  - `<unnamed>` (LastAppliedSequenceTracksHighestApplied): n/a

#### `TEST_F(CrossClusterSubscriptionTest, NameIsPreserved)`
- Source: `tests/replication/test_replication_ha.cpp`:4197
- Brief: n/a
- Parameters:
  - `<unnamed>` (CrossClusterSubscriptionTest): n/a
  - `<unnamed>` (NameIsPreserved): n/a

#### `TEST_F(LWWResolverTest, ChoosesRemoteOnTie)`
- Source: `tests/replication/test_replication_ha.cpp`:302
- Brief: n/a
- Parameters:
  - `<unnamed>` (LWWResolverTest): n/a
  - `<unnamed>` (ChoosesRemoteOnTie): n/a

#### `TEST_F(LWWResolverTest, FallsBackToRemoteWhenNoTimestamp)`
- Source: `tests/replication/test_replication_ha.cpp`:293
- Brief: n/a
- Parameters:
  - `<unnamed>` (LWWResolverTest): n/a
  - `<unnamed>` (FallsBackToRemoteWhenNoTimestamp): n/a

#### `TEST_F(LWWResolverTest, KeepsLocalWhenLocalIsNewer)`
- Source: `tests/replication/test_replication_ha.cpp`:285
- Brief: n/a
- Parameters:
  - `<unnamed>` (LWWResolverTest): n/a
  - `<unnamed>` (KeepsLocalWhenLocalIsNewer): n/a

#### `TEST_F(LWWResolverTest, SelectsNewerTimestamp)`
- Source: `tests/replication/test_replication_ha.cpp`:277
- Brief: n/a
- Parameters:
  - `<unnamed>` (LWWResolverTest): n/a
  - `<unnamed>` (SelectsNewerTimestamp): n/a

#### `TEST_F(LeaderElectionTest, CommitIndexNeverGoesBackward)`
- Source: `tests/replication/test_replication_ha.cpp`:441
- Brief: n/a
- Parameters:
  - `<unnamed>` (LeaderElectionTest): n/a
  - `<unnamed>` (CommitIndexNeverGoesBackward): n/a

#### `TEST_F(LeaderElectionTest, HeartbeatAdvancesCommitIndex)`
- Source: `tests/replication/test_replication_ha.cpp`:418
- Brief: n/a
- Parameters:
  - `<unnamed>` (LeaderElectionTest): n/a
  - `<unnamed>` (HeartbeatAdvancesCommitIndex): n/a

#### `TEST_F(LeaderElectionTest, HeartbeatConvertsCandidateToFollower)`
- Source: `tests/replication/test_replication_ha.cpp`:405
- Brief: n/a
- Parameters:
  - `<unnamed>` (LeaderElectionTest): n/a
  - `<unnamed>` (HeartbeatConvertsCandidateToFollower): n/a

#### `TEST_F(LeaderElectionTest, RequestVoteGrantedWhenLogIsUpToDate)`
- Source: `tests/replication/test_replication_ha.cpp`:463
- Brief: n/a
- Parameters:
  - `<unnamed>` (LeaderElectionTest): n/a
  - `<unnamed>` (RequestVoteGrantedWhenLogIsUpToDate): n/a

#### `TEST_F(LeaderElectionTest, RequestVoteRejectedForStaleTerm)`
- Source: `tests/replication/test_replication_ha.cpp`:473
- Brief: n/a
- Parameters:
  - `<unnamed>` (LeaderElectionTest): n/a
  - `<unnamed>` (RequestVoteRejectedForStaleTerm): n/a

#### `TEST_F(LeaderElectionTest, SingleNodeWinsElectionImmediately)`
- Source: `tests/replication/test_replication_ha.cpp`:365
- Brief: n/a
- Parameters:
  - `<unnamed>` (LeaderElectionTest): n/a
  - `<unnamed>` (SingleNodeWinsElectionImmediately): n/a

#### `TEST_F(LeaderElectionTest, StaleVoteIsIgnored)`
- Source: `tests/replication/test_replication_ha.cpp`:388
- Brief: n/a
- Parameters:
  - `<unnamed>` (LeaderElectionTest): n/a
  - `<unnamed>` (StaleVoteIsIgnored): n/a

#### `TEST_F(LeaderElectionTest, ThreeNodeClusterRequiresQuorum)`
- Source: `tests/replication/test_replication_ha.cpp`:373
- Brief: n/a
- Parameters:
  - `<unnamed>` (LeaderElectionTest): n/a
  - `<unnamed>` (ThreeNodeClusterRequiresQuorum): n/a

#### `TEST_F(PersistentStateTest, ConcurrentPersistIsThreadSafe)`
- Source: `tests/replication/test_replication_ha.cpp`:2278
- Brief: n/a
- Parameters:
  - `<unnamed>` (PersistentStateTest): n/a
  - `<unnamed>` (ConcurrentPersistIsThreadSafe): n/a

#### `TEST_F(PersistentStateTest, FileDoesNotExistInitially)`
- Source: `tests/replication/test_replication_ha.cpp`:2215
- Brief: n/a
- Parameters:
  - `<unnamed>` (PersistentStateTest): n/a
  - `<unnamed>` (FileDoesNotExistInitially): n/a

#### `TEST_F(PersistentStateTest, LoadAfterRestart)`
- Source: `tests/replication/test_replication_ha.cpp`:2305
- Brief: n/a
- Parameters:
  - `<unnamed>` (PersistentStateTest): n/a
  - `<unnamed>` (LoadAfterRestart): n/a

#### `TEST_F(PersistentStateTest, LoadReturnsDefaultWhenFileAbsent)`
- Source: `tests/replication/test_replication_ha.cpp`:2240
- Brief: n/a
- Parameters:
  - `<unnamed>` (PersistentStateTest): n/a
  - `<unnamed>` (LoadReturnsDefaultWhenFileAbsent): n/a

#### `TEST_F(PersistentStateTest, PersistAndLoad)`
- Source: `tests/replication/test_replication_ha.cpp`:2220
- Brief: n/a
- Parameters:
  - `<unnamed>` (PersistentStateTest): n/a
  - `<unnamed>` (PersistAndLoad): n/a

#### `TEST_F(PersistentStateTest, PersistOverwritesPreviousState)`
- Source: `tests/replication/test_replication_ha.cpp`:2248
- Brief: n/a
- Parameters:
  - `<unnamed>` (PersistentStateTest): n/a
  - `<unnamed>` (PersistOverwritesPreviousState): n/a

#### `TEST_F(PersistentStateTest, RemoveDeletesFile)`
- Source: `tests/replication/test_replication_ha.cpp`:2266
- Brief: n/a
- Parameters:
  - `<unnamed>` (PersistentStateTest): n/a
  - `<unnamed>` (RemoveDeletesFile): n/a

#### `TEST_F(PublicationFilterTest, CollectionAndOperationFilterCombined)`
- Source: `tests/replication/test_replication_ha.cpp`:4052
- Brief: n/a
- Parameters:
  - `<unnamed>` (PublicationFilterTest): n/a
  - `<unnamed>` (CollectionAndOperationFilterCombined): n/a

#### `TEST_F(PublicationFilterTest, CollectionFilterIncludesOnly)`
- Source: `tests/replication/test_replication_ha.cpp`:4026
- Brief: n/a
- Parameters:
  - `<unnamed>` (PublicationFilterTest): n/a
  - `<unnamed>` (CollectionFilterIncludesOnly): n/a

#### `TEST_F(PublicationFilterTest, EmptyFilterMatchesAll)`
- Source: `tests/replication/test_replication_ha.cpp`:4019
- Brief: n/a
- Parameters:
  - `<unnamed>` (PublicationFilterTest): n/a
  - `<unnamed>` (EmptyFilterMatchesAll): n/a

#### `TEST_F(PublicationFilterTest, MultipleCollectionsFilter)`
- Source: `tests/replication/test_replication_ha.cpp`:4034
- Brief: n/a
- Parameters:
  - `<unnamed>` (PublicationFilterTest): n/a
  - `<unnamed>` (MultipleCollectionsFilter): n/a

#### `TEST_F(PublicationFilterTest, OperationFilterIncludesOnly)`
- Source: `tests/replication/test_replication_ha.cpp`:4043
- Brief: n/a
- Parameters:
  - `<unnamed>` (PublicationFilterTest): n/a
  - `<unnamed>` (OperationFilterIncludesOnly): n/a

#### `TEST_F(ReplicationConfigTest, ElectionTimeoutMinGEMaxFails)`
- Source: `tests/replication/test_replication_ha.cpp`:123
- Brief: n/a
- Parameters:
  - `<unnamed>` (ReplicationConfigTest): n/a
  - `<unnamed>` (ElectionTimeoutMinGEMaxFails): n/a

#### `TEST_F(ReplicationConfigTest, InvalidBatchSizeTooLargeFails)`
- Source: `tests/replication/test_replication_ha.cpp`:105
- Brief: n/a
- Parameters:
  - `<unnamed>` (ReplicationConfigTest): n/a
  - `<unnamed>` (InvalidBatchSizeTooLargeFails): n/a

#### `TEST_F(ReplicationConfigTest, InvalidBatchSizeZeroFails)`
- Source: `tests/replication/test_replication_ha.cpp`:96
- Brief: n/a
- Parameters:
  - `<unnamed>` (ReplicationConfigTest): n/a
  - `<unnamed>` (InvalidBatchSizeZeroFails): n/a

#### `TEST_F(ReplicationConfigTest, InvalidHeartbeatIntervalFails)`
- Source: `tests/replication/test_replication_ha.cpp`:114
- Brief: n/a
- Parameters:
  - `<unnamed>` (ReplicationConfigTest): n/a
  - `<unnamed>` (InvalidHeartbeatIntervalFails): n/a

#### `TEST_F(ReplicationConfigTest, InvalidWALCompressionLevelFails)`
- Source: `tests/replication/test_replication_ha.cpp`:181
- Brief: n/a
- Parameters:
  - `<unnamed>` (ReplicationConfigTest): n/a
  - `<unnamed>` (InvalidWALCompressionLevelFails): n/a

#### `TEST_F(ReplicationConfigTest, LeaseDisabledIgnoresLeaseDuration)`
- Source: `tests/replication/test_replication_ha.cpp`:158
- Brief: n/a
- Parameters:
  - `<unnamed>` (ReplicationConfigTest): n/a
  - `<unnamed>` (LeaseDisabledIgnoresLeaseDuration): n/a

#### `TEST_F(ReplicationConfigTest, LeaseDurationGEElectionTimeoutFails)`
- Source: `tests/replication/test_replication_ha.cpp`:133
- Brief: n/a
- Parameters:
  - `<unnamed>` (ReplicationConfigTest): n/a
  - `<unnamed>` (LeaseDurationGEElectionTimeoutFails): n/a

#### `TEST_F(ReplicationConfigTest, LeaseDurationLTElectionTimeoutSucceeds)`
- Source: `tests/replication/test_replication_ha.cpp`:145
- Brief: n/a
- Parameters:
  - `<unnamed>` (ReplicationConfigTest): n/a
  - `<unnamed>` (LeaseDurationLTElectionTimeoutSucceeds): n/a

#### `TEST_F(ReplicationConfigTest, ValidConfigInitializesSuccessfully)`
- Source: `tests/replication/test_replication_ha.cpp`:172
- Brief: n/a
- Parameters:
  - `<unnamed>` (ReplicationConfigTest): n/a
  - `<unnamed>` (ValidConfigInitializesSuccessfully): n/a

#### `TEST_F(ReplicationConfigTest, ValidWALCompressionLevelSucceeds)`
- Source: `tests/replication/test_replication_ha.cpp`:191
- Brief: n/a
- Parameters:
  - `<unnamed>` (ReplicationConfigTest): n/a
  - `<unnamed>` (ValidWALCompressionLevelSucceeds): n/a

#### `TEST_F(ReplicationStreamCompressionTest, AutoAlgorithmConfig)`
- Source: `tests/replication/test_replication_ha.cpp`:2696
- Brief: n/a
- Parameters:
  - `<unnamed>` (ReplicationStreamCompressionTest): n/a
  - `<unnamed>` (AutoAlgorithmConfig): n/a

#### `TEST_F(ReplicationStreamCompressionTest, CompressionDisabledByDefault)`
- Source: `tests/replication/test_replication_ha.cpp`:2649
- Brief: n/a
- Parameters:
  - `<unnamed>` (ReplicationStreamCompressionTest): n/a
  - `<unnamed>` (CompressionDisabledByDefault): n/a

#### `TEST_F(ReplicationStreamCompressionTest, CompressionDisabledConstructsStream)`
- Source: `tests/replication/test_replication_ha.cpp`:2678
- Brief: n/a
- Parameters:
  - `<unnamed>` (ReplicationStreamCompressionTest): n/a
  - `<unnamed>` (CompressionDisabledConstructsStream): n/a

#### `TEST_F(ReplicationStreamCompressionTest, LZ4ConfigConstructsStream)`
- Source: `tests/replication/test_replication_ha.cpp`:2668
- Brief: n/a
- Parameters:
  - `<unnamed>` (ReplicationStreamCompressionTest): n/a
  - `<unnamed>` (LZ4ConfigConstructsStream): n/a

#### `TEST_F(ReplicationStreamCompressionTest, SnappyAlgorithmConfig)`
- Source: `tests/replication/test_replication_ha.cpp`:2706
- Brief: n/a
- Parameters:
  - `<unnamed>` (ReplicationStreamCompressionTest): n/a
  - `<unnamed>` (SnappyAlgorithmConfig): n/a

#### `TEST_F(ReplicationStreamCompressionTest, UnknownAlgorithmDefaultsToZstd)`
- Source: `tests/replication/test_replication_ha.cpp`:2686
- Brief: n/a
- Parameters:
  - `<unnamed>` (ReplicationStreamCompressionTest): n/a
  - `<unnamed>` (UnknownAlgorithmDefaultsToZstd): n/a

#### `TEST_F(ReplicationStreamCompressionTest, ZstdConfigConstructsStream)`
- Source: `tests/replication/test_replication_ha.cpp`:2657
- Brief: n/a
- Parameters:
  - `<unnamed>` (ReplicationStreamCompressionTest): n/a
  - `<unnamed>` (ZstdConfigConstructsStream): n/a

#### `TEST_F(WALChecksumTest, AppendAndReadBackVerifiesChecksum)`
- Source: `tests/replication/test_replication_ha.cpp`:208
- Brief: n/a
- Parameters:
  - `<unnamed>` (WALChecksumTest): n/a
  - `<unnamed>` (AppendAndReadBackVerifiesChecksum): n/a

#### `TEST_F(WALChecksumTest, CorruptChecksumEntryIsDropped)`
- Source: `tests/replication/test_replication_ha.cpp`:230
- Brief: n/a
- Parameters:
  - `<unnamed>` (WALChecksumTest): n/a
  - `<unnamed>` (CorruptChecksumEntryIsDropped): n/a

#### `TEST_F(WitnessNodeTest, AddReplicaWithWitnessRoleSkipsStream)`
- Source: `tests/replication/test_replication_ha.cpp`:4788
- Brief: n/a
- Parameters:
  - `<unnamed>` (WitnessNodeTest): n/a
  - `<unnamed>` (AddReplicaWithWitnessRoleSkipsStream): n/a

#### `TEST_F(WitnessNodeTest, RemoveWitnessNodeDeregistersIt)`
- Source: `tests/replication/test_replication_ha.cpp`:4874
- Brief: n/a
- Parameters:
  - `<unnamed>` (WitnessNodeTest): n/a
  - `<unnamed>` (RemoveWitnessNodeDeregistersIt): n/a

#### `TEST_F(WitnessNodeTest, TwoNodeClusterWithWitnessHasQuorum)`
- Source: `tests/replication/test_replication_ha.cpp`:4815
- Brief: n/a
- Parameters:
  - `<unnamed>` (WitnessNodeTest): n/a
  - `<unnamed>` (TwoNodeClusterWithWitnessHasQuorum): n/a

#### `TEST_F(WitnessNodeTest, WitnessCountsForQuorumWhenDataFollowerFailed)`
- Source: `tests/replication/test_replication_ha.cpp`:4894
- Brief: n/a
- Parameters:
  - `<unnamed>` (WitnessNodeTest): n/a
  - `<unnamed>` (WitnessCountsForQuorumWhenDataFollowerFailed): n/a

#### `TEST_F(WitnessNodeTest, WitnessNodeNotSelectedForReads)`
- Source: `tests/replication/test_replication_ha.cpp`:4854
- Brief: n/a
- Parameters:
  - `<unnamed>` (WitnessNodeTest): n/a
  - `<unnamed>` (WitnessNodeNotSelectedForReads): n/a

#### `TEST_F(WitnessNodeTest, WitnessNotSelectedAsLeaderCandidate)`
- Source: `tests/replication/test_replication_ha.cpp`:4931
- Brief: n/a
- Parameters:
  - `<unnamed>` (WitnessNodeTest): n/a
  - `<unnamed>` (WitnessNotSelectedAsLeaderCandidate): n/a

#### `TEST_F(WitnessNodeTest, WitnessRoleIsVotingMember)`
- Source: `tests/replication/test_replication_ha.cpp`:4769
- Brief: n/a
- Parameters:
  - `<unnamed>` (WitnessNodeTest): n/a
  - `<unnamed>` (WitnessRoleIsVotingMember): n/a

#### `BidirectionalReplicationManager::BidiConfig makeBidiConfig(const std::string &local="node-west", const std::string &remote="node-east")`
- Source: `tests/replication/test_replication_ha.cpp`:4971
- Brief: n/a
- Parameters:
  - `local` (const std::string &): n/a
  - `remote` (const std::string &): n/a

#### `ReplicationConfig makeConfig(const std::string &wal_dir="/tmp/themis_repl_test_wal")`
- Source: `tests/replication/test_replication_ha.cpp`:40
- Brief: n/a
- Parameters:
  - `wal_dir` (const std::string &): n/a

#### `HybridLogicalClock::Timestamp makeHLCTimestamp(uint64_t phys, uint32_t log, const std::string &node)`
- Source: `tests/replication/test_replication_ha.cpp`:852
- Brief: n/a
- Parameters:
  - `phys` (uint64_t): n/a
  - `log` (uint32_t): n/a
  - `node` (const std::string &): n/a

#### `ReplicationConfig makeLeaseConfig(const std::string &wal_dir)`
- Source: `tests/replication/test_replication_ha.cpp`:3798
- Brief: n/a
- Parameters:
  - `wal_dir` (const std::string &): n/a

#### `MMReplicationConfig makeMMConfig(const std::string &node_id="mm-node-1")`
- Source: `tests/replication/test_replication_ha.cpp`:1302
- Brief: n/a
- Parameters:
  - `node_id` (const std::string &): n/a

#### `ReplicaInfo makeReplica(const std::string &ep, uint64_t seq, HealthStatus hs=HealthStatus::HEALTHY)`
- Source: `tests/replication/test_replication_ha.cpp`:1785
- Brief: n/a
- Parameters:
  - `ep` (const std::string &): n/a
  - `seq` (uint64_t): n/a
  - `hs` (HealthStatus): n/a

#### `WALEntry makeWALEntry(uint64_t seq, const std::string &collection, const std::string &op, const std::string &doc_id="doc1")`
- Source: `tests/replication/test_replication_ha.cpp`:4002
- Brief: n/a
- Parameters:
  - `seq` (uint64_t): n/a
  - `collection` (const std::string &): n/a
  - `op` (const std::string &): n/a
  - `doc_id` (const std::string &): n/a

### test_replication_highcardinality_stress.cpp

#### `TEST(ReplicationHighCardinalityStress, BackpressureEdgeCases)`
- Source: `tests/replication/test_replication_highcardinality_stress.cpp`:221
- Brief: n/a
- Parameters:
  - `<unnamed>` (ReplicationHighCardinalityStress): n/a
  - `<unnamed>` (BackpressureEdgeCases): n/a

#### `TEST(ReplicationHighCardinalityStress, ConcurrentCDCStress)`
- Source: `tests/replication/test_replication_highcardinality_stress.cpp`:183
- Brief: n/a
- Parameters:
  - `<unnamed>` (ReplicationHighCardinalityStress): n/a
  - `<unnamed>` (ConcurrentCDCStress): n/a

#### `TEST(ReplicationHighCardinalityStress, HighCardinalityReplicationSlot)`
- Source: `tests/replication/test_replication_highcardinality_stress.cpp`:135
- Brief: n/a
- Parameters:
  - `<unnamed>` (ReplicationHighCardinalityStress): n/a
  - `<unnamed>` (HighCardinalityReplicationSlot): n/a

### test_replication_manager_addReplica_simple.cpp

#### `TEST_F(ReplicationManagerAddReplicaTest, AcceptsValidReplicaInfo)`
- Source: `tests/replication/test_replication_manager_addReplica_simple.cpp`:83
- Brief: n/a
- Parameters:
  - `<unnamed>` (ReplicationManagerAddReplicaTest): n/a
  - `<unnamed>` (AcceptsValidReplicaInfo): n/a
- Details: Test 3: Accept valid node_id and endpoint

#### `TEST_F(ReplicationManagerAddReplicaTest, FailClosedGuardsAreIndependent)`
- Source: `tests/replication/test_replication_manager_addReplica_simple.cpp`:143
- Brief: n/a
- Parameters:
  - `<unnamed>` (ReplicationManagerAddReplicaTest): n/a
  - `<unnamed>` (FailClosedGuardsAreIndependent): n/a
- Details: Test 5: Guard is independent across multiple calls

#### `TEST_F(ReplicationManagerAddReplicaTest, MultipleReplicasCanBeAdded)`
- Source: `tests/replication/test_replication_manager_addReplica_simple.cpp`:115
- Brief: n/a
- Parameters:
  - `<unnamed>` (ReplicationManagerAddReplicaTest): n/a
  - `<unnamed>` (MultipleReplicasCanBeAdded): n/a
- Details: Test 4: Multiple valid replicas can be added

#### `TEST_F(ReplicationManagerAddReplicaTest, RejectsEmptyEndpoint)`
- Source: `tests/replication/test_replication_manager_addReplica_simple.cpp`:62
- Brief: n/a
- Parameters:
  - `<unnamed>` (ReplicationManagerAddReplicaTest): n/a
  - `<unnamed>` (RejectsEmptyEndpoint): n/a
- Details: Test 2: Reject empty endpoint

#### `TEST_F(ReplicationManagerAddReplicaTest, RejectsEmptyNodeId)`
- Source: `tests/replication/test_replication_manager_addReplica_simple.cpp`:38
- Brief: n/a
- Parameters:
  - `<unnamed>` (ReplicationManagerAddReplicaTest): n/a
  - `<unnamed>` (RejectsEmptyNodeId): n/a
- Details: Test 1: Reject empty node_id

### test_replication_new_features.cpp

#### `TEST_F(EventStreamTest, FilteredSubscriptionOnlyReceivesMatchingType)`
- Source: `tests/replication/test_replication_new_features.cpp`:416
- Brief: n/a
- Parameters:
  - `<unnamed>` (EventStreamTest): n/a
  - `<unnamed>` (FilteredSubscriptionOnlyReceivesMatchingType): n/a

#### `TEST_F(EventStreamTest, HistoricalQueryFilterByType)`
- Source: `tests/replication/test_replication_new_features.cpp`:458
- Brief: n/a
- Parameters:
  - `<unnamed>` (EventStreamTest): n/a
  - `<unnamed>` (HistoricalQueryFilterByType): n/a

#### `TEST_F(EventStreamTest, HistoricalQueryReturnsBufferedEvents)`
- Source: `tests/replication/test_replication_new_features.cpp`:448
- Brief: n/a
- Parameters:
  - `<unnamed>` (EventStreamTest): n/a
  - `<unnamed>` (HistoricalQueryReturnsBufferedEvents): n/a

#### `TEST_F(EventStreamTest, RAIIHandleUnsubscribesOnDestruction)`
- Source: `tests/replication/test_replication_new_features.cpp`:435
- Brief: n/a
- Parameters:
  - `<unnamed>` (EventStreamTest): n/a
  - `<unnamed>` (RAIIHandleUnsubscribesOnDestruction): n/a

#### `TEST_F(EventStreamTest, RingBufferDropsOldestWhenFull)`
- Source: `tests/replication/test_replication_new_features.cpp`:471
- Brief: n/a
- Parameters:
  - `<unnamed>` (EventStreamTest): n/a
  - `<unnamed>` (RingBufferDropsOldestWhenFull): n/a

#### `TEST_F(EventStreamTest, SubscribeAndReceiveEvent)`
- Source: `tests/replication/test_replication_new_features.cpp`:406
- Brief: n/a
- Parameters:
  - `<unnamed>` (EventStreamTest): n/a
  - `<unnamed>` (SubscribeAndReceiveEvent): n/a

#### `TEST_F(EventStreamTest, WALEntryAppliedEmitsWriteReplicatedEvent)`
- Source: `tests/replication/test_replication_new_features.cpp`:483
- Brief: n/a
- Parameters:
  - `<unnamed>` (EventStreamTest): n/a
  - `<unnamed>` (WALEntryAppliedEmitsWriteReplicatedEvent): n/a

#### `TEST_F(FieldLevelMergeTest, EmptyConflictSetFailsClosed)`
- Source: `tests/replication/test_replication_new_features.cpp`:355
- Brief: n/a
- Parameters:
  - `<unnamed>` (FieldLevelMergeTest): n/a
  - `<unnamed>` (EmptyConflictSetFailsClosed): n/a

#### `TEST_F(FieldLevelMergeTest, IntersectOnlyCommonFields)`
- Source: `tests/replication/test_replication_new_features.cpp`:304
- Brief: n/a
- Parameters:
  - `<unnamed>` (FieldLevelMergeTest): n/a
  - `<unnamed>` (IntersectOnlyCommonFields): n/a

#### `TEST_F(FieldLevelMergeTest, LeftBiasPreferFirstEntry)`
- Source: `tests/replication/test_replication_new_features.cpp`:320
- Brief: n/a
- Parameters:
  - `<unnamed>` (FieldLevelMergeTest): n/a
  - `<unnamed>` (LeftBiasPreferFirstEntry): n/a

#### `TEST_F(FieldLevelMergeTest, RightBiasPreferLastEntry)`
- Source: `tests/replication/test_replication_new_features.cpp`:332
- Brief: n/a
- Parameters:
  - `<unnamed>` (FieldLevelMergeTest): n/a
  - `<unnamed>` (RightBiasPreferLastEntry): n/a

#### `TEST_F(FieldLevelMergeTest, StrategyNamesAreCorrect)`
- Source: `tests/replication/test_replication_new_features.cpp`:344
- Brief: n/a
- Parameters:
  - `<unnamed>` (FieldLevelMergeTest): n/a
  - `<unnamed>` (StrategyNamesAreCorrect): n/a

#### `TEST_F(FieldLevelMergeTest, UnionIncludesAllFields)`
- Source: `tests/replication/test_replication_new_features.cpp`:289
- Brief: n/a
- Parameters:
  - `<unnamed>` (FieldLevelMergeTest): n/a
  - `<unnamed>` (UnionIncludesAllFields): n/a

#### `TEST_F(FieldLevelMergeTest, WinnerCarriesMergedCausalMetadata)`
- Source: `tests/replication/test_replication_new_features.cpp`:365
- Brief: n/a
- Parameters:
  - `<unnamed>` (FieldLevelMergeTest): n/a
  - `<unnamed>` (WinnerCarriesMergedCausalMetadata): n/a

#### `TEST_F(ObservabilityTest, CustomCriticalLagThresholdApplied)`
- Source: `tests/replication/test_replication_new_features.cpp`:163
- Brief: n/a
- Parameters:
  - `<unnamed>` (ObservabilityTest): n/a
  - `<unnamed>` (CustomCriticalLagThresholdApplied): n/a

#### `TEST_F(ObservabilityTest, DetectBottlenecksReturnsEmptyForHealthyCluster)`
- Source: `tests/replication/test_replication_new_features.cpp`:157
- Brief: n/a
- Parameters:
  - `<unnamed>` (ObservabilityTest): n/a
  - `<unnamed>` (DetectBottlenecksReturnsEmptyForHealthyCluster): n/a

#### `TEST_F(ObservabilityTest, HealthScoreReturns100ForEmptyCluster)`
- Source: `tests/replication/test_replication_new_features.cpp`:148
- Brief: n/a
- Parameters:
  - `<unnamed>` (ObservabilityTest): n/a
  - `<unnamed>` (HealthScoreReturns100ForEmptyCluster): n/a

#### `TEST_F(ObservabilityTest, LagSnapshotsEmptyWhenNoReplicas)`
- Source: `tests/replication/test_replication_new_features.cpp`:127
- Brief: n/a
- Parameters:
  - `<unnamed>` (ObservabilityTest): n/a
  - `<unnamed>` (LagSnapshotsEmptyWhenNoReplicas): n/a

#### `TEST_F(ObservabilityTest, TopologyContainsLocalNode)`
- Source: `tests/replication/test_replication_new_features.cpp`:134
- Brief: n/a
- Parameters:
  - `<unnamed>` (ObservabilityTest): n/a
  - `<unnamed>` (TopologyContainsLocalNode): n/a

#### `TEST_F(ParallelReplicationWorkerTest, AverageLatencyPopulatedAfterWork)`
- Source: `tests/replication/test_replication_new_features.cpp`:966
- Brief: n/a
- Parameters:
  - `<unnamed>` (ParallelReplicationWorkerTest): n/a
  - `<unnamed>` (AverageLatencyPopulatedAfterWork): n/a

#### `TEST_F(ParallelReplicationWorkerTest, AverageLatencyZeroBeforeSubmit)`
- Source: `tests/replication/test_replication_new_features.cpp`:980
- Brief: n/a
- Parameters:
  - `<unnamed>` (ParallelReplicationWorkerTest): n/a
  - `<unnamed>` (AverageLatencyZeroBeforeSubmit): n/a

#### `TEST_F(ParallelReplicationWorkerTest, ConstructAndDestruct)`
- Source: `tests/replication/test_replication_new_features.cpp`:793
- Brief: n/a
- Parameters:
  - `<unnamed>` (ParallelReplicationWorkerTest): n/a
  - `<unnamed>` (ConstructAndDestruct): n/a

#### `TEST_F(ParallelReplicationWorkerTest, DependencyTrackingDetectsConflicts)`
- Source: `tests/replication/test_replication_new_features.cpp`:820
- Brief: n/a
- Parameters:
  - `<unnamed>` (ParallelReplicationWorkerTest): n/a
  - `<unnamed>` (DependencyTrackingDetectsConflicts): n/a

#### `TEST_F(ParallelReplicationWorkerTest, GetStatsBeforeSubmitReturnsZero)`
- Source: `tests/replication/test_replication_new_features.cpp`:876
- Brief: n/a
- Parameters:
  - `<unnamed>` (ParallelReplicationWorkerTest): n/a
  - `<unnamed>` (GetStatsBeforeSubmitReturnsZero): n/a

#### `TEST_F(ParallelReplicationWorkerTest, GroupTransactionsDisabled)`
- Source: `tests/replication/test_replication_new_features.cpp`:947
- Brief: n/a
- Parameters:
  - `<unnamed>` (ParallelReplicationWorkerTest): n/a
  - `<unnamed>` (GroupTransactionsDisabled): n/a

#### `TEST_F(ParallelReplicationWorkerTest, GroupTransactionsEnabled)`
- Source: `tests/replication/test_replication_new_features.cpp`:919
- Brief: n/a
- Parameters:
  - `<unnamed>` (ParallelReplicationWorkerTest): n/a
  - `<unnamed>` (GroupTransactionsEnabled): n/a

#### `TEST_F(ParallelReplicationWorkerTest, LargeSubmitBatch)`
- Source: `tests/replication/test_replication_new_features.cpp`:986
- Brief: n/a
- Parameters:
  - `<unnamed>` (ParallelReplicationWorkerTest): n/a
  - `<unnamed>` (LargeSubmitBatch): n/a

#### `TEST_F(ParallelReplicationWorkerTest, MixedDocumentsPartialDependencies)`
- Source: `tests/replication/test_replication_new_features.cpp`:850
- Brief: n/a
- Parameters:
  - `<unnamed>` (ParallelReplicationWorkerTest): n/a
  - `<unnamed>` (MixedDocumentsPartialDependencies): n/a

#### `TEST_F(ParallelReplicationWorkerTest, NoDependencyTrackingSkipsDeps)`
- Source: `tests/replication/test_replication_new_features.cpp`:835
- Brief: n/a
- Parameters:
  - `<unnamed>` (ParallelReplicationWorkerTest): n/a
  - `<unnamed>` (NoDependencyTrackingSkipsDeps): n/a

#### `TEST_F(ParallelReplicationWorkerTest, SingleThreadedWorker)`
- Source: `tests/replication/test_replication_new_features.cpp`:885
- Brief: n/a
- Parameters:
  - `<unnamed>` (ParallelReplicationWorkerTest): n/a
  - `<unnamed>` (SingleThreadedWorker): n/a

#### `TEST_F(ParallelReplicationWorkerTest, SixteenThreadWorker)`
- Source: `tests/replication/test_replication_new_features.cpp`:898
- Brief: n/a
- Parameters:
  - `<unnamed>` (ParallelReplicationWorkerTest): n/a
  - `<unnamed>` (SixteenThreadWorker): n/a

#### `TEST_F(ParallelReplicationWorkerTest, StatsParallelismFactorNonNegative)`
- Source: `tests/replication/test_replication_new_features.cpp`:865
- Brief: n/a
- Parameters:
  - `<unnamed>` (ParallelReplicationWorkerTest): n/a
  - `<unnamed>` (StatsParallelismFactorNonNegative): n/a

#### `TEST_F(ParallelReplicationWorkerTest, SubmitMultipleIndependentEntries)`
- Source: `tests/replication/test_replication_new_features.cpp`:808
- Brief: n/a
- Parameters:
  - `<unnamed>` (ParallelReplicationWorkerTest): n/a
  - `<unnamed>` (SubmitMultipleIndependentEntries): n/a

#### `TEST_F(ParallelReplicationWorkerTest, SubmitSingleEntryAndSync)`
- Source: `tests/replication/test_replication_new_features.cpp`:798
- Brief: n/a
- Parameters:
  - `<unnamed>` (ParallelReplicationWorkerTest): n/a
  - `<unnamed>` (SubmitSingleEntryAndSync): n/a

#### `TEST_F(ParallelReplicationWorkerTest, SyncOnEmptyQueueReturnsImmediately)`
- Source: `tests/replication/test_replication_new_features.cpp`:911
- Brief: n/a
- Parameters:
  - `<unnamed>` (ParallelReplicationWorkerTest): n/a
  - `<unnamed>` (SyncOnEmptyQueueReturnsImmediately): n/a

#### `TEST_F(ReplicationPolicyTest, AssignNonExistentPolicyReturnsFalse)`
- Source: `tests/replication/test_replication_new_features.cpp`:533
- Brief: n/a
- Parameters:
  - `<unnamed>` (ReplicationPolicyTest): n/a
  - `<unnamed>` (AssignNonExistentPolicyReturnsFalse): n/a

#### `TEST_F(ReplicationPolicyTest, DefineThenGetPolicy)`
- Source: `tests/replication/test_replication_new_features.cpp`:518
- Brief: n/a
- Parameters:
  - `<unnamed>` (ReplicationPolicyTest): n/a
  - `<unnamed>` (DefineThenGetPolicy): n/a

#### `TEST_F(ReplicationPolicyTest, GetPolicyForUnassignedCollectionReturnsDefault)`
- Source: `tests/replication/test_replication_new_features.cpp`:537
- Brief: n/a
- Parameters:
  - `<unnamed>` (ReplicationPolicyTest): n/a
  - `<unnamed>` (GetPolicyForUnassignedCollectionReturnsDefault): n/a

#### `TEST_F(ReplicationPolicyTest, ListPoliciesAfterDefine)`
- Source: `tests/replication/test_replication_new_features.cpp`:550
- Brief: n/a
- Parameters:
  - `<unnamed>` (ReplicationPolicyTest): n/a
  - `<unnamed>` (ListPoliciesAfterDefine): n/a

#### `TEST_F(ReplicationPolicyTest, RemovePolicyReturnsTrueIfExists)`
- Source: `tests/replication/test_replication_new_features.cpp`:542
- Brief: n/a
- Parameters:
  - `<unnamed>` (ReplicationPolicyTest): n/a
  - `<unnamed>` (RemovePolicyReturnsTrueIfExists): n/a

#### `TEST_F(ReplicationPolicyTest, ValidatePolicySucceedsForLenientPolicy)`
- Source: `tests/replication/test_replication_new_features.cpp`:571
- Brief: n/a
- Parameters:
  - `<unnamed>` (ReplicationPolicyTest): n/a
  - `<unnamed>` (ValidatePolicySucceedsForLenientPolicy): n/a

#### `TEST_F(ReplicationPolicyTest, ValidatePolicyViolatesMinReplicasOnSingleNode)`
- Source: `tests/replication/test_replication_new_features.cpp`:561
- Brief: n/a
- Parameters:
  - `<unnamed>` (ReplicationPolicyTest): n/a
  - `<unnamed>` (ValidatePolicyViolatesMinReplicasOnSingleNode): n/a

#### `TEST_F(ReplicationSlotTest, AdvanceOnPausedSlotFails)`
- Source: `tests/replication/test_replication_new_features.cpp`:654
- Brief: n/a
- Parameters:
  - `<unnamed>` (ReplicationSlotTest): n/a
  - `<unnamed>` (AdvanceOnPausedSlotFails): n/a

#### `TEST_F(ReplicationSlotTest, AdvanceUpdatesLsn)`
- Source: `tests/replication/test_replication_new_features.cpp`:638
- Brief: n/a
- Parameters:
  - `<unnamed>` (ReplicationSlotTest): n/a
  - `<unnamed>` (AdvanceUpdatesLsn): n/a

#### `TEST_F(ReplicationSlotTest, CreateSlotIsActive)`
- Source: `tests/replication/test_replication_new_features.cpp`:600
- Brief: n/a
- Parameters:
  - `<unnamed>` (ReplicationSlotTest): n/a
  - `<unnamed>` (CreateSlotIsActive): n/a

#### `TEST_F(ReplicationSlotTest, DropSlot)`
- Source: `tests/replication/test_replication_new_features.cpp`:666
- Brief: n/a
- Parameters:
  - `<unnamed>` (ReplicationSlotTest): n/a
  - `<unnamed>` (DropSlot): n/a

#### `TEST_F(ReplicationSlotTest, DuplicateSlotNameReturnsNull)`
- Source: `tests/replication/test_replication_new_features.cpp`:611
- Brief: n/a
- Parameters:
  - `<unnamed>` (ReplicationSlotTest): n/a
  - `<unnamed>` (DuplicateSlotNameReturnsNull): n/a

#### `TEST_F(ReplicationSlotTest, LagCalculation)`
- Source: `tests/replication/test_replication_new_features.cpp`:677
- Brief: n/a
- Parameters:
  - `<unnamed>` (ReplicationSlotTest): n/a
  - `<unnamed>` (LagCalculation): n/a

#### `TEST_F(ReplicationSlotTest, ListSlotsReturnsAllRegistered)`
- Source: `tests/replication/test_replication_new_features.cpp`:704
- Brief: n/a
- Parameters:
  - `<unnamed>` (ReplicationSlotTest): n/a
  - `<unnamed>` (ListSlotsReturnsAllRegistered): n/a

#### `TEST_F(ReplicationSlotTest, MinConfirmedLsnAcrossSlots)`
- Source: `tests/replication/test_replication_new_features.cpp`:717
- Brief: n/a
- Parameters:
  - `<unnamed>` (ReplicationSlotTest): n/a
  - `<unnamed>` (MinConfirmedLsnAcrossSlots): n/a

#### `TEST_F(ReplicationSlotTest, PauseAndResume)`
- Source: `tests/replication/test_replication_new_features.cpp`:621
- Brief: n/a
- Parameters:
  - `<unnamed>` (ReplicationSlotTest): n/a
  - `<unnamed>` (PauseAndResume): n/a

#### `TEST_F(ReplicationSlotTest, StatePersistenceRoundTrip)`
- Source: `tests/replication/test_replication_new_features.cpp`:731
- Brief: n/a
- Parameters:
  - `<unnamed>` (ReplicationSlotTest): n/a
  - `<unnamed>` (StatePersistenceRoundTrip): n/a

#### `TEST_F(ThreeWayMergeTest, EmptyConflictSetFailsClosed)`
- Source: `tests/replication/test_replication_new_features.cpp`:233
- Brief: n/a
- Parameters:
  - `<unnamed>` (ThreeWayMergeTest): n/a
  - `<unnamed>` (EmptyConflictSetFailsClosed): n/a

#### `TEST_F(ThreeWayMergeTest, MergesNonConflictingFields)`
- Source: `tests/replication/test_replication_new_features.cpp`:192
- Brief: n/a
- Parameters:
  - `<unnamed>` (ThreeWayMergeTest): n/a
  - `<unnamed>` (MergesNonConflictingFields): n/a

#### `TEST_F(ThreeWayMergeTest, SingleWriteReturnedUnchanged)`
- Source: `tests/replication/test_replication_new_features.cpp`:181
- Brief: n/a
- Parameters:
  - `<unnamed>` (ThreeWayMergeTest): n/a
  - `<unnamed>` (SingleWriteReturnedUnchanged): n/a

#### `TEST_F(ThreeWayMergeTest, StrategyNameIsThreeWayMerge)`
- Source: `tests/replication/test_replication_new_features.cpp`:228
- Brief: n/a
- Parameters:
  - `<unnamed>` (ThreeWayMergeTest): n/a
  - `<unnamed>` (StrategyNameIsThreeWayMerge): n/a

#### `TEST_F(ThreeWayMergeTest, WinnerCarriesMergedCausalMetadata)`
- Source: `tests/replication/test_replication_new_features.cpp`:244
- Brief: n/a
- Parameters:
  - `<unnamed>` (ThreeWayMergeTest): n/a
  - `<unnamed>` (WinnerCarriesMergedCausalMetadata): n/a

#### `ReplicationConfig makeConfig(const std::string &wal_dir="/tmp/themis_newfeature_wal")`
- Source: `tests/replication/test_replication_new_features.cpp`:40
- Brief: n/a
- Parameters:
  - `wal_dir` (const std::string &): n/a

#### `MMWriteEntry makeMMEntry(const std::string &doc_id, const std::string &data, uint64_t hlc_physical, uint32_t hlc_logical=0)`
- Source: `tests/replication/test_replication_new_features.cpp`:88
- Brief: n/a
- Parameters:
  - `doc_id` (const std::string &): n/a
  - `data` (const std::string &): n/a
  - `hlc_physical` (uint64_t): n/a
  - `hlc_logical` (uint32_t): n/a

### test_replication_raft_v2.cpp

#### `TEST(RaftV2CommitTransitionTest, CommitCollapsesToNewConfig)`
- Source: `tests/replication/test_replication_raft_v2.cpp`:207
- Brief: n/a
- Parameters:
  - `<unnamed>` (RaftV2CommitTransitionTest): n/a
  - `<unnamed>` (CommitCollapsesToNewConfig): n/a

#### `TEST(RaftV2CommitTransitionTest, CommitWithoutTransitionThrows)`
- Source: `tests/replication/test_replication_raft_v2.cpp`:217
- Brief: n/a
- Parameters:
  - `<unnamed>` (RaftV2CommitTransitionTest): n/a
  - `<unnamed>` (CommitWithoutTransitionThrows): n/a

#### `TEST(RaftV2ErrorTest, CannotRemoveLastMember)`
- Source: `tests/replication/test_replication_raft_v2.cpp`:256
- Brief: n/a
- Parameters:
  - `<unnamed>` (RaftV2ErrorTest): n/a
  - `<unnamed>` (CannotRemoveLastMember): n/a

#### `TEST(RaftV2ErrorTest, ConcurrentChangeRejected)`
- Source: `tests/replication/test_replication_raft_v2.cpp`:245
- Brief: n/a
- Parameters:
  - `<unnamed>` (RaftV2ErrorTest): n/a
  - `<unnamed>` (ConcurrentChangeRejected): n/a

#### `TEST(RaftV2RollbackTransitionTest, RollbackRestoresOldConfig)`
- Source: `tests/replication/test_replication_raft_v2.cpp`:226
- Brief: n/a
- Parameters:
  - `<unnamed>` (RaftV2RollbackTransitionTest): n/a
  - `<unnamed>` (RollbackRestoresOldConfig): n/a

#### `TEST(RaftV2RollbackTransitionTest, RollbackWithoutTransitionThrows)`
- Source: `tests/replication/test_replication_raft_v2.cpp`:236
- Brief: n/a
- Parameters:
  - `<unnamed>` (RaftV2RollbackTransitionTest): n/a
  - `<unnamed>` (RollbackWithoutTransitionThrows): n/a

#### `TEST(RaftV2StateTest, DefaultStateIsZero)`
- Source: `tests/replication/test_replication_raft_v2.cpp`:461
- Brief: n/a
- Parameters:
  - `<unnamed>` (RaftV2StateTest): n/a
  - `<unnamed>` (DefaultStateIsZero): n/a

#### `TEST(RaftV2StateTest, HasVotedReturnsFalseWhenVotedForEmpty)`
- Source: `tests/replication/test_replication_raft_v2.cpp`:450
- Brief: n/a
- Parameters:
  - `<unnamed>` (RaftV2StateTest): n/a
  - `<unnamed>` (HasVotedReturnsFalseWhenVotedForEmpty): n/a

#### `TEST(RaftV2StateTest, HasVotedReturnsTrueAfterVote)`
- Source: `tests/replication/test_replication_raft_v2.cpp`:455
- Brief: n/a
- Parameters:
  - `<unnamed>` (RaftV2StateTest): n/a
  - `<unnamed>` (HasVotedReturnsTrueAfterVote): n/a

#### `TEST_F(MCMFollowerTest, ApplyCommitEntryFinalisesTransition)`
- Source: `tests/replication/test_replication_raft_v2.cpp`:379
- Brief: n/a
- Parameters:
  - `<unnamed>` (MCMFollowerTest): n/a
  - `<unnamed>` (ApplyCommitEntryFinalisesTransition): n/a

#### `TEST_F(MCMFollowerTest, ApplyEntryIdempotentWhenAlreadyInJoint)`
- Source: `tests/replication/test_replication_raft_v2.cpp`:422
- Brief: n/a
- Parameters:
  - `<unnamed>` (MCMFollowerTest): n/a
  - `<unnamed>` (ApplyEntryIdempotentWhenAlreadyInJoint): n/a

#### `TEST_F(MCMFollowerTest, ApplyJointEntryActivatesJointConsensus)`
- Source: `tests/replication/test_replication_raft_v2.cpp`:358
- Brief: n/a
- Parameters:
  - `<unnamed>` (MCMFollowerTest): n/a
  - `<unnamed>` (ApplyJointEntryActivatesJointConsensus): n/a

#### `TEST_F(MCMFollowerTest, ApplyJointEntryForRemove)`
- Source: `tests/replication/test_replication_raft_v2.cpp`:405
- Brief: n/a
- Parameters:
  - `<unnamed>` (MCMFollowerTest): n/a
  - `<unnamed>` (ApplyJointEntryForRemove): n/a

#### `TEST_F(MCMTest, ConcurrentProposalRejected)`
- Source: `tests/replication/test_replication_raft_v2.cpp`:440
- Brief: n/a
- Parameters:
  - `<unnamed>` (MCMTest): n/a
  - `<unnamed>` (ConcurrentProposalRejected): n/a

#### `TEST_F(MCMTest, OnJointCommittedIgnoresMismatch)`
- Source: `tests/replication/test_replication_raft_v2.cpp`:306
- Brief: n/a
- Parameters:
  - `<unnamed>` (MCMTest): n/a
  - `<unnamed>` (OnJointCommittedIgnoresMismatch): n/a

#### `TEST_F(MCMTest, OnJointCommittedWritesCommitEntry)`
- Source: `tests/replication/test_replication_raft_v2.cpp`:297
- Brief: n/a
- Parameters:
  - `<unnamed>` (MCMTest): n/a
  - `<unnamed>` (OnJointCommittedWritesCommitEntry): n/a

#### `TEST_F(MCMTest, OnNewConfigCommittedFinalisesTransition)`
- Source: `tests/replication/test_replication_raft_v2.cpp`:318
- Brief: n/a
- Parameters:
  - `<unnamed>` (MCMTest): n/a
  - `<unnamed>` (OnNewConfigCommittedFinalisesTransition): n/a

#### `TEST_F(MCMTest, ProposeAddActivatesJointConsensus)`
- Source: `tests/replication/test_replication_raft_v2.cpp`:288
- Brief: n/a
- Parameters:
  - `<unnamed>` (MCMTest): n/a
  - `<unnamed>` (ProposeAddActivatesJointConsensus): n/a

#### `TEST_F(MCMTest, ProposeAddCreatesJointEntry)`
- Source: `tests/replication/test_replication_raft_v2.cpp`:280
- Brief: n/a
- Parameters:
  - `<unnamed>` (MCMTest): n/a
  - `<unnamed>` (ProposeAddCreatesJointEntry): n/a

#### `TEST_F(MCMTest, ProposeRemoveCreatesJointEntry)`
- Source: `tests/replication/test_replication_raft_v2.cpp`:332
- Brief: n/a
- Parameters:
  - `<unnamed>` (MCMTest): n/a
  - `<unnamed>` (ProposeRemoveCreatesJointEntry): n/a

#### `TEST_F(RaftV2ConfigBasicTest, ConstructionSetsMembers)`
- Source: `tests/replication/test_replication_raft_v2.cpp`:73
- Brief: n/a
- Parameters:
  - `<unnamed>` (RaftV2ConfigBasicTest): n/a
  - `<unnamed>` (ConstructionSetsMembers): n/a

#### `TEST_F(RaftV2ConfigBasicTest, EmptyConfigQuorumIsOne)`
- Source: `tests/replication/test_replication_raft_v2.cpp`:102
- Brief: n/a
- Parameters:
  - `<unnamed>` (RaftV2ConfigBasicTest): n/a
  - `<unnamed>` (EmptyConfigQuorumIsOne): n/a

#### `TEST_F(RaftV2ConfigBasicTest, QuorumSizeFiveNode)`
- Source: `tests/replication/test_replication_raft_v2.cpp`:87
- Brief: n/a
- Parameters:
  - `<unnamed>` (RaftV2ConfigBasicTest): n/a
  - `<unnamed>` (QuorumSizeFiveNode): n/a

#### `TEST_F(RaftV2ConfigBasicTest, QuorumSizeThreeNode)`
- Source: `tests/replication/test_replication_raft_v2.cpp`:82
- Brief: n/a
- Parameters:
  - `<unnamed>` (RaftV2ConfigBasicTest): n/a
  - `<unnamed>` (QuorumSizeThreeNode): n/a

#### `TEST_F(RaftV2ConfigBasicTest, StableQuorumDeniedWhenMinorityVotes)`
- Source: `tests/replication/test_replication_raft_v2.cpp`:97
- Brief: n/a
- Parameters:
  - `<unnamed>` (RaftV2ConfigBasicTest): n/a
  - `<unnamed>` (StableQuorumDeniedWhenMinorityVotes): n/a

#### `TEST_F(RaftV2ConfigBasicTest, StableQuorumGrantedWhenMajorityVotes)`
- Source: `tests/replication/test_replication_raft_v2.cpp`:92
- Brief: n/a
- Parameters:
  - `<unnamed>` (RaftV2ConfigBasicTest): n/a
  - `<unnamed>` (StableQuorumGrantedWhenMajorityVotes): n/a

#### `TEST_F(RaftV2JointConsensusAddTest, AllMembersUnionDuringJoint)`
- Source: `tests/replication/test_replication_raft_v2.cpp`:137
- Brief: n/a
- Parameters:
  - `<unnamed>` (RaftV2JointConsensusAddTest): n/a
  - `<unnamed>` (AllMembersUnionDuringJoint): n/a

#### `TEST_F(RaftV2JointConsensusAddTest, BeginAddEntersJointConsensus)`
- Source: `tests/replication/test_replication_raft_v2.cpp`:117
- Brief: n/a
- Parameters:
  - `<unnamed>` (RaftV2JointConsensusAddTest): n/a
  - `<unnamed>` (BeginAddEntersJointConsensus): n/a

#### `TEST_F(RaftV2JointConsensusAddTest, NewMemberIsVisible)`
- Source: `tests/replication/test_replication_raft_v2.cpp`:122
- Brief: n/a
- Parameters:
  - `<unnamed>` (RaftV2JointConsensusAddTest): n/a
  - `<unnamed>` (NewMemberIsVisible): n/a

#### `TEST_F(RaftV2JointConsensusAddTest, OldMembersPreservedDuringTransition)`
- Source: `tests/replication/test_replication_raft_v2.cpp`:128
- Brief: n/a
- Parameters:
  - `<unnamed>` (RaftV2JointConsensusAddTest): n/a
  - `<unnamed>` (OldMembersPreservedDuringTransition): n/a

#### `TEST_F(RaftV2JointConsensusRemoveTest, BeginRemoveEntersJointConsensus)`
- Source: `tests/replication/test_replication_raft_v2.cpp`:152
- Brief: n/a
- Parameters:
  - `<unnamed>` (RaftV2JointConsensusRemoveTest): n/a
  - `<unnamed>` (BeginRemoveEntersJointConsensus): n/a

#### `TEST_F(RaftV2JointConsensusRemoveTest, RemovedMemberAbsentFromNewConfig)`
- Source: `tests/replication/test_replication_raft_v2.cpp`:157
- Brief: n/a
- Parameters:
  - `<unnamed>` (RaftV2JointConsensusRemoveTest): n/a
  - `<unnamed>` (RemovedMemberAbsentFromNewConfig): n/a

#### `TEST_F(RaftV2JointConsensusRemoveTest, RemovedMemberStillInOldConfig)`
- Source: `tests/replication/test_replication_raft_v2.cpp`:163
- Brief: n/a
- Parameters:
  - `<unnamed>` (RaftV2JointConsensusRemoveTest): n/a
  - `<unnamed>` (RemovedMemberStillInOldConfig): n/a

#### `TEST_F(RaftV2JointConsensusRemoveTest, RemovedMemberStillReachableViaMember)`
- Source: `tests/replication/test_replication_raft_v2.cpp`:168
- Brief: n/a
- Parameters:
  - `<unnamed>` (RaftV2JointConsensusRemoveTest): n/a
  - `<unnamed>` (RemovedMemberStillReachableViaMember): n/a

#### `TEST_F(RaftV2JointQuorumTest, QuorumDeniedWithNewMajorityMissingOldMajority)`
- Source: `tests/replication/test_replication_raft_v2.cpp`:198
- Brief: n/a
- Parameters:
  - `<unnamed>` (RaftV2JointQuorumTest): n/a
  - `<unnamed>` (QuorumDeniedWithNewMajorityMissingOldMajority): n/a

#### `TEST_F(RaftV2JointQuorumTest, QuorumDeniedWithOldMajorityMissingNewMajority)`
- Source: `tests/replication/test_replication_raft_v2.cpp`:193
- Brief: n/a
- Parameters:
  - `<unnamed>` (RaftV2JointQuorumTest): n/a
  - `<unnamed>` (QuorumDeniedWithOldMajorityMissingNewMajority): n/a

#### `TEST_F(RaftV2JointQuorumTest, QuorumGrantedWithBothMajorities)`
- Source: `tests/replication/test_replication_raft_v2.cpp`:188
- Brief: n/a
- Parameters:
  - `<unnamed>` (RaftV2JointQuorumTest): n/a
  - `<unnamed>` (QuorumGrantedWithBothMajorities): n/a

#### `std::set< std::string > makeMembers(std::initializer_list< const char * > ids)`
- Source: `tests/replication/test_replication_raft_v2.cpp`:41
- Brief: n/a
- Parameters:
  - `ids` (std::initializer_list< const char * >): n/a

#### `std::shared_ptr< WALManager > makeWAL(const std::string &dir)`
- Source: `tests/replication/test_replication_raft_v2.cpp`:61
- Brief: n/a
- Parameters:
  - `dir` (const std::string &): n/a

### test_replication_topology_api_handler.cpp

#### `TEST(ReplicationCoordinatorTopologyTest, GetReplicaInfoReturnsEmptyWithoutShipper)`
- Source: `tests/replication/test_replication_topology_api_handler.cpp`:158
- Brief: n/a
- Parameters:
  - `<unnamed>` (ReplicationCoordinatorTopologyTest): n/a
  - `<unnamed>` (GetReplicaInfoReturnsEmptyWithoutShipper): n/a

#### `TEST(ReplicationCoordinatorTopologyTest, GetShipperStatsReturnsZeroWithoutShipper)`
- Source: `tests/replication/test_replication_topology_api_handler.cpp`:172
- Brief: n/a
- Parameters:
  - `<unnamed>` (ReplicationCoordinatorTopologyTest): n/a
  - `<unnamed>` (GetShipperStatsReturnsZeroWithoutShipper): n/a

#### `TEST_F(ReplicationTopologyApiHandlerNoReplTest, HealthReturns503WhenNoCoordinator)`
- Source: `tests/replication/test_replication_topology_api_handler.cpp`:61
- Brief: n/a
- Parameters:
  - `<unnamed>` (ReplicationTopologyApiHandlerNoReplTest): n/a
  - `<unnamed>` (HealthReturns503WhenNoCoordinator): n/a

#### `TEST_F(ReplicationTopologyApiHandlerNoReplTest, TopologyReturns503WhenNoCoordinator)`
- Source: `tests/replication/test_replication_topology_api_handler.cpp`:53
- Brief: n/a
- Parameters:
  - `<unnamed>` (ReplicationTopologyApiHandlerNoReplTest): n/a
  - `<unnamed>` (TopologyReturns503WhenNoCoordinator): n/a

#### `TEST_F(ReplicationTopologyApiHandlerNoReplTest, UiAlwaysServes200)`
- Source: `tests/replication/test_replication_topology_api_handler.cpp`:69
- Brief: n/a
- Parameters:
  - `<unnamed>` (ReplicationTopologyApiHandlerNoReplTest): n/a
  - `<unnamed>` (UiAlwaysServes200): n/a

#### `TEST_F(ReplicationTopologyApiHandlerWithReplTest, UiPageContainsExpectedElements)`
- Source: `tests/replication/test_replication_topology_api_handler.cpp`:109
- Brief: n/a
- Parameters:
  - `<unnamed>` (ReplicationTopologyApiHandlerWithReplTest): n/a
  - `<unnamed>` (UiPageContainsExpectedElements): n/a

#### `TEST_F(ReplicationTopologyApiHandlerWithReplTest, UiPageInjectsApiBaseFromHostHeader)`
- Source: `tests/replication/test_replication_topology_api_handler.cpp`:130
- Brief: n/a
- Parameters:
  - `<unnamed>` (ReplicationTopologyApiHandlerWithReplTest): n/a
  - `<unnamed>` (UiPageInjectsApiBaseFromHostHeader): n/a

#### `TEST_F(ReplicationTopologyApiHandlerWithReplTest, UiRejectsInvalidApiBasePrefix)`
- Source: `tests/replication/test_replication_topology_api_handler.cpp`:142
- Brief: n/a
- Parameters:
  - `<unnamed>` (ReplicationTopologyApiHandlerWithReplTest): n/a
  - `<unnamed>` (UiRejectsInvalidApiBasePrefix): n/a

### themis::bench::rmd

#### `void BM_RMD01_GeoLeaderSelection(benchmark::State &state)`
- Source: `benchmarks/replication/bench_replication_multi_dc_multi_writer.cpp`:151
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_RMD02_GeoFailoverSelection(benchmark::State &state)`
- Source: `benchmarks/replication/bench_replication_multi_dc_multi_writer.cpp`:173
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_RMD03_GeoValidatePlacement(benchmark::State &state)`
- Source: `benchmarks/replication/bench_replication_multi_dc_multi_writer.cpp`:198
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_RMD04_WalEnqueueThroughput(benchmark::State &state)`
- Source: `benchmarks/replication/bench_replication_multi_dc_multi_writer.cpp`:220
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_RMD05_WalLagCheck(benchmark::State &state)`
- Source: `benchmarks/replication/bench_replication_multi_dc_multi_writer.cpp`:266
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_RMD06_MultiWriterConflictRate(benchmark::State &state)`
- Source: `benchmarks/replication/bench_replication_multi_dc_multi_writer.cpp`:294
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_RMD07_MultiDCWriteThroughput(benchmark::State &state)`
- Source: `benchmarks/replication/bench_replication_multi_dc_multi_writer.cpp`:348
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_RMD08_LagAlertFireLatency(benchmark::State &state)`
- Source: `benchmarks/replication/bench_replication_multi_dc_multi_writer.cpp`:395
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `Iterations(10) -> Unit(benchmark::kMillisecond)`
- Source: `benchmarks/replication/bench_replication_multi_dc_multi_writer.cpp`:441
- Brief: n/a
- Parameters:
  - `<unnamed>` (10): n/a

#### `Repetitions(3) -> ReportAggregatesOnly(false) ->Unit(benchmark::kMicrosecond)`
- Source: `benchmarks/replication/bench_replication_multi_dc_multi_writer.cpp`:340
- Brief: n/a
- Parameters:
  - `<unnamed>` (3): n/a

#### `Repetitions(5) -> ReportAggregatesOnly(true) ->Unit(benchmark::kMicrosecond)`
- Source: `benchmarks/replication/bench_replication_multi_dc_multi_writer.cpp`:165
- Brief: n/a
- Parameters:
  - `<unnamed>` (5): n/a

#### `UseRealTime() -> Repetitions(3) ->ReportAggregatesOnly(true) ->Unit(benchmark::kMicrosecond)`
- Source: `benchmarks/replication/bench_replication_multi_dc_multi_writer.cpp`:257
- Brief: n/a
- Parameters: none

#### `std::vector< ReplicaInfo > build3DCTopology()`
- Source: `benchmarks/replication/bench_replication_multi_dc_multi_writer.cpp`:113
- Brief: Build a 3-DC topology with kReplicasPerDC healthy voting replicas per DC.
- Parameters: none

#### `PlacementConstraints buildConstraints()`
- Source: `benchmarks/replication/bench_replication_multi_dc_multi_writer.cpp`:136
- Brief: Constraints that prefer the first DC and forbid the third.
- Parameters: none

### themis::bench::rrg

#### `void BM_RRG01_GCounterMerge(benchmark::State &state)`
- Source: `benchmarks/replication/bench_replication_release_gates.cpp`:143
- Brief: RRG-01: G-Counter merge throughput over 1k merge operations.
- Parameters:
  - `state` (benchmark::State &): n/a
- Details: GATE-RRG-01: ≥ 500k ops/s.

#### `void BM_RRG02_LwwResolve(benchmark::State &state)`
- Source: `benchmarks/replication/bench_replication_release_gates.cpp`:170
- Brief: RRG-02: LWW resolution latency for a single conflicting pair.
- Parameters:
  - `state` (benchmark::State &): n/a
- Details: GATE-RRG-02: p99 ≤ 50 µs.

#### `void BM_RRG03_EventSerialize(benchmark::State &state)`
- Source: `benchmarks/replication/bench_replication_release_gates.cpp`:199
- Brief: RRG-03: Serialise a 128-byte change event payload.
- Parameters:
  - `state` (benchmark::State &): n/a
- Details: GATE-RRG-03: p99 ≤ 100 µs.

#### `void BM_RRG04_WalApply(benchmark::State &state)`
- Source: `benchmarks/replication/bench_replication_release_gates.cpp`:225
- Brief: RRG-04: In-memory WAL event apply latency.
- Parameters:
  - `state` (benchmark::State &): n/a
- Details: GATE-RRG-04: p99 ≤ 500 µs.

#### `void BM_RRG05_PartitionOffsetLookup(benchmark::State &state)`
- Source: `benchmarks/replication/bench_replication_release_gates.cpp`:254
- Brief: RRG-05: Partition offset lookup in an in-memory hash map.
- Parameters:
  - `state` (benchmark::State &): n/a
- Details: GATE-RRG-05: p99 ≤ 100 µs.

#### `void BM_RRG06_LagCheck(benchmark::State &state)`
- Source: `benchmarks/replication/bench_replication_release_gates.cpp`:286
- Brief: RRG-06: Replication lag evaluation (two LSN comparison).
- Parameters:
  - `state` (benchmark::State &): n/a
- Details: GATE-RRG-06: p99 ≤ 50 µs.

#### `Repetitions(kRepetitions) -> ReportAggregatesOnly(true)`
- Source: `benchmarks/replication/bench_replication_release_gates.cpp`:158
- Brief: n/a
- Parameters:
  - `<unnamed>` (kRepetitions): n/a

#### `GCounter gCounterMerge(const GCounter &a, const GCounter &b)`
- Source: `benchmarks/replication/bench_replication_release_gates.cpp`:83
- Brief: n/a
- Parameters:
  - `a` (const GCounter &): n/a
  - `b` (const GCounter &): n/a

#### `bool isLagExceeded(std::int64_t applied_lsn, std::int64_t upstream_lsn, std::int64_t max_gap)`
- Source: `benchmarks/replication/bench_replication_release_gates.cpp`:129
- Brief: Replication lag check: compare two LSN values.
- Parameters:
  - `applied_lsn` (std::int64_t): n/a
  - `upstream_lsn` (std::int64_t): n/a
  - `max_gap` (std::int64_t): n/a

#### `LwwVersion lwwResolve(const LwwVersion &a, const LwwVersion &b)`
- Source: `benchmarks/replication/bench_replication_release_gates.cpp`:97
- Brief: n/a
- Parameters:
  - `a` (const LwwVersion &): n/a
  - `b` (const LwwVersion &): n/a

#### `std::int64_t partitionOffsetLookup(const std::unordered_map< int, std::int64_t > &offsets, int partition_id)`
- Source: `benchmarks/replication/bench_replication_release_gates.cpp`:122
- Brief: Partition offset lookup: hash map read.
- Parameters:
  - `offsets` (const std::unordered_map< int, std::int64_t > &): n/a
  - `partition_id` (int): n/a

#### `std::array< std::uint8_t, 128 > serializeEvent(std::int64_t lsn, const char *payload)`
- Source: `benchmarks/replication/bench_replication_release_gates.cpp`:105
- Brief: Simulated 128-byte change event payload serialisation (copy into array).
- Parameters:
  - `lsn` (std::int64_t): n/a
  - `payload` (const char *): n/a

#### `bool walApply(std::vector< std::int64_t > &applied, std::int64_t lsn)`
- Source: `benchmarks/replication/bench_replication_release_gates.cpp`:116
- Brief: In-memory WAL apply: append LSN to applied list.
- Parameters:
  - `applied` (std::vector< std::int64_t > &): n/a
  - `lsn` (std::int64_t): n/a

### themis::bench::wave_d

#### `void GEO_BENCH_01_LeaderElectionWithConstraints(benchmark::State &state)`
- Source: `benchmarks/replication/bench_geo_placement_wal_shipping.cpp`:125
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void GEO_BENCH_02_FailoverWithConstraint(benchmark::State &state)`
- Source: `benchmarks/replication/bench_geo_placement_wal_shipping.cpp`:153
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void GEO_BENCH_03_PlacementValidation(benchmark::State &state)`
- Source: `benchmarks/replication/bench_geo_placement_wal_shipping.cpp`:181
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `Iterations(100) -> Name("GEO-BENCH-01: Leader Election (3-DC)") ->Unit(benchmark::kMillisecond) ->Repetitions(kRepetitions) ->ReportAggregatesOnly(true)`
- Source: `benchmarks/replication/bench_geo_placement_wal_shipping.cpp`:143
- Brief: n/a
- Parameters:
  - `<unnamed>` (100): n/a

#### `Iterations(1000) -> Name("WAL-BENCH-03: Stats Accuracy (load test)") ->Unit(benchmark::kMillisecond) ->Repetitions(kRepetitions) ->ReportAggregatesOnly(true)`
- Source: `benchmarks/replication/bench_geo_placement_wal_shipping.cpp`:362
- Brief: n/a
- Parameters:
  - `<unnamed>` (1000): n/a

#### `Iterations(10000) -> Name("WAL-BENCH-01: Shipping Throughput (1 KB segs)") ->Unit(benchmark::kMillisecond) ->Repetitions(kRepetitions) ->ReportAggregatesOnly(true)`
- Source: `benchmarks/replication/bench_geo_placement_wal_shipping.cpp`:256
- Brief: n/a
- Parameters:
  - `<unnamed>` (10000): n/a

#### `Iterations(500) -> Name("WAL-BENCH-02: Lag Alert Latency (backpressure)") ->Unit(benchmark::kMillisecond) ->Repetitions(kRepetitions) ->ReportAggregatesOnly(true)`
- Source: `benchmarks/replication/bench_geo_placement_wal_shipping.cpp`:319
- Brief: n/a
- Parameters:
  - `<unnamed>` (500): n/a

#### `void WAL_BENCH_01_ShippingThroughput(benchmark::State &state)`
- Source: `benchmarks/replication/bench_geo_placement_wal_shipping.cpp`:208
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void WAL_BENCH_02_LagAlertLatency(benchmark::State &state)`
- Source: `benchmarks/replication/bench_geo_placement_wal_shipping.cpp`:266
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void WAL_BENCH_03_StatsAccuracy(benchmark::State &state)`
- Source: `benchmarks/replication/bench_geo_placement_wal_shipping.cpp`:329
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void WAL_BENCH_04_MetricsExport(benchmark::State &state)`
- Source: `benchmarks/replication/bench_geo_placement_wal_shipping.cpp`:372
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

### themis::replication

#### `bool isHardReplicationError(ReplicationErrorCode code) noexcept`
- Source: `include/replication/replication_api_contract.h`:168
- Brief: Returns true when the error code indicates a non-retryable hard error.
- Parameters:
  - `code` (ReplicationErrorCode): n/a
- Details: Hard replication errors must be reported to the operator immediately; the pipeline must not silently continue in a degraded state.

#### `bool isLagError(ReplicationErrorCode code) noexcept`
- Source: `include/replication/replication_api_contract.h`:180
- Brief: Returns true when the error code indicates a recoverable lag / flow control condition.
- Parameters:
  - `code` (ReplicationErrorCode): n/a

### themis::replication::IKafkaChangeStreamExporter

#### `bool configure(const KafkaChangeStreamConfig &config)=0`
- Source: `include/replication/kafka_change_stream.h`:64
- Brief: n/a
- Parameters:
  - `config` (const KafkaChangeStreamConfig &): n/a

#### `bool flush(std::chrono::milliseconds timeout=std::chrono::milliseconds(5000))=0`
- Source: `include/replication/kafka_change_stream.h`:69
- Brief: n/a
- Parameters:
  - `timeout` (std::chrono::milliseconds): n/a

#### `KafkaStreamStats getStats() const =0`
- Source: `include/replication/kafka_change_stream.h`:68
- Brief: n/a
- Parameters: none

#### `bool isRunning() const =0`
- Source: `include/replication/kafka_change_stream.h`:67
- Brief: n/a
- Parameters: none

#### `bool start()=0`
- Source: `include/replication/kafka_change_stream.h`:65
- Brief: n/a
- Parameters: none

#### `bool stop()=0`
- Source: `include/replication/kafka_change_stream.h`:66
- Brief: n/a
- Parameters: none

#### `~IKafkaChangeStreamExporter()=default`
- Source: `include/replication/kafka_change_stream.h`:63
- Brief: n/a
- Parameters: none

### themis::replication::test

#### `TEST(ReplicationContractHardening, RCH01_GCounterMergeCommutative)`
- Source: `tests/replication/test_replication_contract_hardening_focused.cpp`:163
- Brief: RCH-01: GCounter merge is commutative.
- Parameters:
  - `<unnamed>` (ReplicationContractHardening): n/a
  - `<unnamed>` (RCH01_GCounterMergeCommutative): n/a

#### `TEST(ReplicationContractHardening, RCH02_GCounterMergeIdempotent)`
- Source: `tests/replication/test_replication_contract_hardening_focused.cpp`:178
- Brief: RCH-02: GCounter merge is idempotent.
- Parameters:
  - `<unnamed>` (ReplicationContractHardening): n/a
  - `<unnamed>` (RCH02_GCounterMergeIdempotent): n/a

#### `TEST(ReplicationContractHardening, RCH03_GCounterMergeAssociative)`
- Source: `tests/replication/test_replication_contract_hardening_focused.cpp`:188
- Brief: RCH-03: GCounter merge is associative.
- Parameters:
  - `<unnamed>` (ReplicationContractHardening): n/a
  - `<unnamed>` (RCH03_GCounterMergeAssociative): n/a

#### `TEST(ReplicationContractHardening, RCH04_CrdtTypeMismatch)`
- Source: `tests/replication/test_replication_contract_hardening_focused.cpp`:201
- Brief: RCH-04: Merging incompatible CRDT types raises CRDT_TYPE_MISMATCH.
- Parameters:
  - `<unnamed>` (ReplicationContractHardening): n/a
  - `<unnamed>` (RCH04_CrdtTypeMismatch): n/a

#### `TEST(ReplicationContractHardening, RCH05_LwwDeterministic)`
- Source: `tests/replication/test_replication_contract_hardening_focused.cpp`:213
- Brief: RCH-05: LWW resolution is deterministic — same inputs always same winner.
- Parameters:
  - `<unnamed>` (ReplicationContractHardening): n/a
  - `<unnamed>` (RCH05_LwwDeterministic): n/a

#### `TEST(ReplicationContractHardening, RCH06_TieBreakByNodeId)`
- Source: `tests/replication/test_replication_contract_hardening_focused.cpp`:225
- Brief: RCH-06: Tie-break by node-ID is consistent.
- Parameters:
  - `<unnamed>` (ReplicationContractHardening): n/a
  - `<unnamed>` (RCH06_TieBreakByNodeId): n/a

#### `TEST(ReplicationContractHardening, RCH07_TombstoneWins)`
- Source: `tests/replication/test_replication_contract_hardening_focused.cpp`:235
- Brief: RCH-07: Tombstone wins over concurrent update at same timestamp.
- Parameters:
  - `<unnamed>` (ReplicationContractHardening): n/a
  - `<unnamed>` (RCH07_TombstoneWins): n/a

#### `TEST(ReplicationContractHardening, RCH08_LwwWinnerHasGreaterTimestamp)`
- Source: `tests/replication/test_replication_contract_hardening_focused.cpp`:244
- Brief: RCH-08: LWW winner always has the greater timestamp.
- Parameters:
  - `<unnamed>` (ReplicationContractHardening): n/a
  - `<unnamed>` (RCH08_LwwWinnerHasGreaterTimestamp): n/a

#### `TEST(ReplicationContractHardening, RCH09_PartitionOrderPreserved)`
- Source: `tests/replication/test_replication_contract_hardening_focused.cpp`:265
- Brief: RCH-09: Events within a partition are delivered in commit order.
- Parameters:
  - `<unnamed>` (ReplicationContractHardening): n/a
  - `<unnamed>` (RCH09_PartitionOrderPreserved): n/a

#### `TEST(ReplicationContractHardening, RCH10_AtLeastOnceDelivery)`
- Source: `tests/replication/test_replication_contract_hardening_focused.cpp`:272
- Brief: RCH-10: At-least-once: a committed event appears in the delivered stream.
- Parameters:
  - `<unnamed>` (ReplicationContractHardening): n/a
  - `<unnamed>` (RCH10_AtLeastOnceDelivery): n/a

#### `TEST(ReplicationContractHardening, RCH11_RedeliveryIdempotent)`
- Source: `tests/replication/test_replication_contract_hardening_focused.cpp`:283
- Brief: RCH-11: Redelivery is idempotent — applying same LSN twice is a no-op.
- Parameters:
  - `<unnamed>` (ReplicationContractHardening): n/a
  - `<unnamed>` (RCH11_RedeliveryIdempotent): n/a

#### `TEST(ReplicationContractHardening, RCH12_StreamOffsetError)`
- Source: `tests/replication/test_replication_contract_hardening_focused.cpp`:295
- Brief: RCH-12: Invalid offset raises STREAM_OFFSET_ERROR.
- Parameters:
  - `<unnamed>` (ReplicationContractHardening): n/a
  - `<unnamed>` (RCH12_StreamOffsetError): n/a

#### `TEST(ReplicationContractHardening, RCH13_LsnMonotonic)`
- Source: `tests/replication/test_replication_contract_hardening_focused.cpp`:314
- Brief: RCH-13: LSNs are monotonically increasing in a generated sequence.
- Parameters:
  - `<unnamed>` (ReplicationContractHardening): n/a
  - `<unnamed>` (RCH13_LsnMonotonic): n/a

#### `TEST(ReplicationContractHardening, RCH14_LsnGapDetected)`
- Source: `tests/replication/test_replication_contract_hardening_focused.cpp`:327
- Brief: RCH-14: LSN gap (jump by > 1) triggers REPLICATION_LAG_EXCEEDED.
- Parameters:
  - `<unnamed>` (ReplicationContractHardening): n/a
  - `<unnamed>` (RCH14_LsnGapDetected): n/a

#### `TEST(ReplicationContractHardening, RCH15_RetryIdempotent)`
- Source: `tests/replication/test_replication_contract_hardening_focused.cpp`:339
- Brief: RCH-15: Idempotent re-apply of same LSN is safe.
- Parameters:
  - `<unnamed>` (ReplicationContractHardening): n/a
  - `<unnamed>` (RCH15_RetryIdempotent): n/a

#### `TEST(ReplicationContractHardening, RCH16_MaxPendingWalEventsReasonable)`
- Source: `tests/replication/test_replication_contract_hardening_focused.cpp`:355
- Brief: RCH-16: kMaxPendingWalEvents is a positive, finite constant.
- Parameters:
  - `<unnamed>` (ReplicationContractHardening): n/a
  - `<unnamed>` (RCH16_MaxPendingWalEventsReasonable): n/a

#### `bool mockApplyIdempotent(std::vector< std::int64_t > &applied_lsns, std::int64_t lsn)`
- Source: `tests/replication/test_replication_contract_hardening_focused.cpp`:149
- Brief: Simulates idempotent apply (tracks applied LSNs in a set).
- Parameters:
  - `applied_lsns` (std::vector< std::int64_t > &): n/a
  - `lsn` (std::int64_t): n/a

#### `std::optional< ReplicationErrorCode > mockCheckLsnGap(std::int64_t prev_lsn, std::int64_t next_lsn)`
- Source: `tests/replication/test_replication_contract_hardening_focused.cpp`:140
- Brief: Simulates LSN gap detection.
- Parameters:
  - `prev_lsn` (std::int64_t): n/a
  - `next_lsn` (std::int64_t): n/a

#### `std::optional< ReplicationErrorCode > mockCrdtMergeTypes(MockCrdtType a, MockCrdtType b)`
- Source: `tests/replication/test_replication_contract_hardening_focused.cpp`:93
- Brief: Simulates type-mismatch detection.
- Parameters:
  - `a` (MockCrdtType): n/a
  - `b` (MockCrdtType): n/a

#### `MockVersion mockLwwResolve(const MockVersion &v1, const MockVersion &v2)`
- Source: `tests/replication/test_replication_contract_hardening_focused.cpp`:109
- Brief: n/a
- Parameters:
  - `v1` (const MockVersion &): n/a
  - `v2` (const MockVersion &): n/a

#### `std::vector< int > mockPartitionStream(std::vector< int > events)`
- Source: `tests/replication/test_replication_contract_hardening_focused.cpp`:125
- Brief: Simulates partition-ordered event stream.
- Parameters:
  - `events` (std::vector< int >): n/a

#### `std::optional< ReplicationErrorCode > mockValidateOffset(std::int64_t offset, std::int64_t current_committed)`
- Source: `tests/replication/test_replication_contract_hardening_focused.cpp`:131
- Brief: Simulates offset validation.
- Parameters:
  - `offset` (std::int64_t): n/a
  - `current_committed` (std::int64_t): n/a

### themis::replication::test::MockGCounter

#### `MockGCounter merge(const MockGCounter &a, const MockGCounter &b)`
- Source: `tests/replication/test_replication_contract_hardening_focused.cpp`:77
- Brief: n/a
- Parameters:
  - `a` (const MockGCounter &): n/a
  - `b` (const MockGCounter &): n/a

#### `bool operator==(const MockGCounter &other) const`
- Source: `tests/replication/test_replication_contract_hardening_focused.cpp`:85
- Brief: n/a
- Parameters:
  - `other` (const MockGCounter &): n/a

#### `std::int64_t value() const`
- Source: `tests/replication/test_replication_contract_hardening_focused.cpp`:69
- Brief: n/a
- Parameters: none

### themis::test

#### `TEST_F(ReplicationChaosTest, GeographicPartitionRecovery)`
- Source: `tests/replication/test_replication_chaos.cpp`:263
- Brief: Test: Geographic partition recovery.
- Parameters:
  - `<unnamed>` (ReplicationChaosTest): n/a
  - `<unnamed>` (GeographicPartitionRecovery): n/a
- Details: Partition one region, trigger failover to local region, verify recovery.

#### `TEST_F(ReplicationChaosTest, LagInjectionAndFailover)`
- Source: `tests/replication/test_replication_chaos.cpp`:200
- Brief: Test: Lag injection and failover.
- Parameters:
  - `<unnamed>` (ReplicationChaosTest): n/a
  - `<unnamed>` (LagInjectionAndFailover): n/a
- Details: Inject high lag on replica, trigger failover, verify consistency.

#### `TEST_F(ReplicationChaosTest, QuorumRecoveryAfterPartition)`
- Source: `tests/replication/test_replication_chaos.cpp`:325
- Brief: Test: Quorum recovery after partition.
- Parameters:
  - `<unnamed>` (ReplicationChaosTest): n/a
  - `<unnamed>` (QuorumRecoveryAfterPartition): n/a
- Details: Partition partitions cluster, verify quorum is lost then recovered.

#### `TEST_F(ReplicationChaosTest, ReplicaResyncAfterCrash)`
- Source: `tests/replication/test_replication_chaos.cpp`:294
- Brief: Test: Replica resync after crash.
- Parameters:
  - `<unnamed>` (ReplicationChaosTest): n/a
  - `<unnamed>` (ReplicaResyncAfterCrash): n/a
- Details: Replica crashes, reboots, catches up via resync.

#### `TEST_F(ReplicationChaosTest, ReplicationLagAlertAndAction)`
- Source: `tests/replication/test_replication_chaos.cpp`:352
- Brief: Test: Replication lag alert and action.
- Parameters:
  - `<unnamed>` (ReplicationChaosTest): n/a
  - `<unnamed>` (ReplicationLagAlertAndAction): n/a
- Details: Monitor lag, trigger alerts at thresholds, trigger failover after prolonged lag.

#### `TEST_F(ReplicationChaosTest, WALShippingUnderPacketLoss)`
- Source: `tests/replication/test_replication_chaos.cpp`:230
- Brief: Test: WAL shipping under packet loss.
- Parameters:
  - `<unnamed>` (ReplicationChaosTest): n/a
  - `<unnamed>` (WALShippingUnderPacketLoss): n/a
- Details: Inject packet loss, verify WAL shipping retries and succeeds.

### themis::test::MockReplica

#### `MockReplica(int replica_id, const std::string &region)`
- Source: `tests/replication/test_replication_chaos.cpp`:44
- Brief: n/a
- Parameters:
  - `replica_id` (int): n/a
  - `region` (const std::string &): n/a

#### `bool canAcceptWAL() const`
- Source: `tests/replication/test_replication_chaos.cpp`:66
- Brief: n/a
- Parameters: none

#### `int getId() const`
- Source: `tests/replication/test_replication_chaos.cpp`:52
- Brief: n/a
- Parameters: none

#### `int getLagMs() const`
- Source: `tests/replication/test_replication_chaos.cpp`:57
- Brief: n/a
- Parameters: none

#### `const std::string & getRegion() const`
- Source: `tests/replication/test_replication_chaos.cpp`:53
- Brief: n/a
- Parameters: none

#### `State getState() const`
- Source: `tests/replication/test_replication_chaos.cpp`:54
- Brief: n/a
- Parameters: none

#### `int getSyncCount() const`
- Source: `tests/replication/test_replication_chaos.cpp`:63
- Brief: n/a
- Parameters: none

#### `uint64_t getWalPosition() const`
- Source: `tests/replication/test_replication_chaos.cpp`:60
- Brief: n/a
- Parameters: none

#### `void incrementSyncCount()`
- Source: `tests/replication/test_replication_chaos.cpp`:64
- Brief: n/a
- Parameters: none

#### `bool isHealthy() const`
- Source: `tests/replication/test_replication_chaos.cpp`:70
- Brief: n/a
- Parameters: none

#### `void setLagMs(int lag)`
- Source: `tests/replication/test_replication_chaos.cpp`:58
- Brief: n/a
- Parameters:
  - `lag` (int): n/a

#### `void setState(State s)`
- Source: `tests/replication/test_replication_chaos.cpp`:55
- Brief: n/a
- Parameters:
  - `s` (State): n/a

#### `void setWalPosition(uint64_t pos)`
- Source: `tests/replication/test_replication_chaos.cpp`:61
- Brief: n/a
- Parameters:
  - `pos` (uint64_t): n/a

#### `void simulateWALShip(int wal_entry_count)`
- Source: `tests/replication/test_replication_chaos.cpp`:75
- Brief: n/a
- Parameters:
  - `wal_entry_count` (int): n/a

### themis::test::MockReplicationSystem

#### `MockReplicationSystem(int replica_count=3)`
- Source: `tests/replication/test_replication_chaos.cpp`:95
- Brief: n/a
- Parameters:
  - `replica_count` (int): n/a

#### `bool failover()`
- Source: `tests/replication/test_replication_chaos.cpp`:160
- Brief: n/a
- Parameters: none

#### `int getHealthyReplicaCount() const`
- Source: `tests/replication/test_replication_chaos.cpp`:125
- Brief: n/a
- Parameters: none

#### `int getLeaderId() const`
- Source: `tests/replication/test_replication_chaos.cpp`:116
- Brief: n/a
- Parameters: none

#### `MockReplica * getReplica(int id)`
- Source: `tests/replication/test_replication_chaos.cpp`:109
- Brief: n/a
- Parameters:
  - `id` (int): n/a

#### `int getReplicaCount() const`
- Source: `tests/replication/test_replication_chaos.cpp`:107
- Brief: n/a
- Parameters: none

#### `bool hasQuorum() const`
- Source: `tests/replication/test_replication_chaos.cpp`:135
- Brief: n/a
- Parameters: none

#### `bool shipWAL(int wal_entry_count)`
- Source: `tests/replication/test_replication_chaos.cpp`:137
- Brief: n/a
- Parameters:
  - `wal_entry_count` (int): n/a

#### `void updateReplicaLag(int replica_id, int lag_ms)`
- Source: `tests/replication/test_replication_chaos.cpp`:148
- Brief: n/a
- Parameters:
  - `replica_id` (int): n/a
  - `lag_ms` (int): n/a

### themis::test::ReplicationChaosTest

#### `void SetUp() override`
- Source: `tests/replication/test_replication_chaos.cpp`:188
- Brief: n/a
- Parameters: none

#### `void TearDown() override`
- Source: `tests/replication/test_replication_chaos.cpp`:190
- Brief: n/a
- Parameters: none

### themisdb::replication

#### `TEST(ReplicationWave1Contract, LogicalReplicationConfigConstructible)`
- Source: `tests/replication/test_replication_wave1_critical_fixes.cpp`:25
- Brief: n/a
- Parameters:
  - `<unnamed>` (ReplicationWave1Contract): n/a
  - `<unnamed>` (LogicalReplicationConfigConstructible): n/a

#### `TEST(ReplicationWave1Contract, ObservabilityConfigAccessible)`
- Source: `tests/replication/test_replication_wave1_critical_fixes.cpp`:15
- Brief: n/a
- Parameters:
  - `<unnamed>` (ReplicationWave1Contract): n/a
  - `<unnamed>` (ObservabilityConfigAccessible): n/a

#### `TEST(ReplicationWave1Contract, ReplicationConfigConstructible)`
- Source: `tests/replication/test_replication_wave1_critical_fixes.cpp`:20
- Brief: n/a
- Parameters:
  - `<unnamed>` (ReplicationWave1Contract): n/a
  - `<unnamed>` (ReplicationConfigConstructible): n/a

#### `std::vector< PromotionEligibilityAnalysis > analyzeAllReplicasPromotion()`
- Source: `include/replication/replication_failover_diagnostics.h`:293
- Brief: Analyze promotion eligibility for all replicas.
- Parameters: none
- Return: Vector of eligibility analyses, sorted by node ID.
- Details: Vector of eligibility analyses, sorted by node ID.

#### `PromotionEligibilityAnalysis analyzePromotionEligibility(const std::string &node_id)`
- Source: `include/replication/replication_failover_diagnostics.h`:285
- Brief: Analyze promotion eligibility for a specific replica.
- Parameters:
  - `node_id` (const std::string &): Replica node ID to analyze.
- Return: Detailed eligibility analysis.
- Details: node_id Replica node ID to analyze. Detailed eligibility analysis. Thread-safe; returns a snapshot at the time of the call.

#### `std::set< std::string > extractJsonArrayStrings(const std::string &arr)`
- Source: `src/replication/replication_manager.cpp`:3273
- Brief: Helper: extract quoted string tokens from a JSON array e.
- Parameters:
  - `arr` (const std::string &): Input parameter.
- Return: Return value.
- Details: arr Input parameter. Return value. g. ["a","b"] → {"a","b"} BATCH C FIX: Add bounds checking and container stability guards to prevent iterator invalidation and out-of-bounds access during iteration. Calls: empty(), size(), find(), insert(), substr().

#### `std::map< std::string, int64_t > extractJsonInts(const std::string &doc)`
- Source: `src/replication/replication_manager.cpp`:3204
- Brief: n/a
- Parameters:
  - `doc` (const std::string &): n/a

#### `std::string extractSubArray(const std::string &doc, const std::string &key)`
- Source: `src/replication/replication_manager.cpp`:3314
- Brief: Helper: find the raw JSON array string for a named key, e.
- Parameters:
  - `doc` (const std::string &): Input parameter.
  - `key` (const std::string &): Input parameter.
- Return: Return value.
- Details: doc Input parameter. key Input parameter. Return value. g. extractSubArray(doc,"add") Calls: find(), size(), substr().

#### `std::string extractSubObject(const std::string &doc, const std::string &key)`
- Source: `src/replication/replication_manager.cpp`:3247
- Brief: Helper: extract the sub-object string for a named key, e.
- Parameters:
  - `doc` (const std::string &): Input parameter.
  - `key` (const std::string &): Input parameter.
- Return: Return value.
- Details: doc Input parameter. key Input parameter. Return value. g. extractSubObject(doc,"P") returns the raw content between the outermost braces of "P": { ... } Calls: find(), size(), substr().

#### `std::string generateWriteId(const std::string &node_id)`
- Source: `src/replication/replication_manager.cpp`:3923
- Brief: WAVE1-FIX [no_timeout:3331]: generateWriteId is O(1) — no blocking I/O or wait.
- Parameters:
  - `node_id` (const std::string &): Identifier of the node.
- Return: Return value.
- Details: node_id Identifier of the node. Return value. The system_clock::now() call is non-blocking (kernel vDSO); seq.fetch_add is lock-free atomic. No deadline is required. Documented here to close the gap. Calls: std::chrono::system_clock::now(), time_since_epoch(), count(), std::to_string(), fetch_add().

#### `std::vector< ReplicaHealthTransitionDiagnostic > getClusterHealthHistory(size_t max_entries=50)`
- Source: `include/replication/replication_failover_diagnostics.h`:240
- Brief: Get recent health state transitions for all replicas.
- Parameters:
  - `max_entries` (size_t): Maximum total transitions to return.
- Return: Vector of transitions for all replicas, ordered by timestamp.
- Details: max_entries Maximum total transitions to return. Vector of transitions for all replicas, ordered by timestamp.

#### `ConsensusHealthDiagnostic getConsensusHealthDiagnostic()`
- Source: `include/replication/replication_failover_diagnostics.h`:344
- Brief: Get diagnostic information about consensus health.
- Parameters: none
- Return: Current consensus health state.
- Details: Current consensus health state. Thread-safe; returns a snapshot.

#### `std::vector< FailoverCandidateDiagnostic > getFailoverCandidateDiagnostics(const std::string &exclude_node_id)`
- Source: `include/replication/replication_failover_diagnostics.h`:96
- Brief: Get diagnostic information about all failover candidates.
- Parameters:
  - `exclude_node_id` (const std::string &): Node ID to exclude from candidate list (typically the failed leader).
- Return: Vector of diagnostic information for each candidate, sorted by ranking score (best first).
- Details: Returns the evaluation steps and ranking for each candidate, enabling operators to understand the failover selection process. exclude_node_id Node ID to exclude from candidate list (typically the failed leader). Vector of diagnostic information for each candidate, sorted by ranking score (best first). Thread-safe; safe to call concurrently from any thread. Captures a snapshot at the time of the call.

#### `std::optional< FailoverExecutionDiagnostic > getFailoverDiagnostic(const std::string &failover_id)`
- Source: `include/replication/replication_failover_diagnostics.h`:171
- Brief: Get diagnostic information about a specific failover operation.
- Parameters:
  - `failover_id` (const std::string &): ID of the failover operation.
- Return: Diagnostic information, or std::nullopt if not found.
- Details: failover_id ID of the failover operation. Diagnostic information, or std::nullopt if not found.

#### `std::vector< FailoverExecutionDiagnostic > getFailoverHistory(size_t max_entries=0)`
- Source: `include/replication/replication_failover_diagnostics.h`:182
- Brief: Get history of recent failover operations.
- Parameters:
  - `max_entries` (size_t): Maximum number of entries to return (0 = all available).
- Return: Vector of failover diagnostics, ordered by timestamp (newest first).
- Details: max_entries Maximum number of entries to return (0 = all available). Vector of failover diagnostics, ordered by timestamp (newest first). Useful for trend analysis (e.g., are failovers becoming slower?)

#### `std::optional< FailoverExecutionDiagnostic > getLastFailoverDiagnostic()`
- Source: `include/replication/replication_failover_diagnostics.h`:163
- Brief: Get diagnostic information about the last failover operation.
- Parameters: none
- Return: Diagnostic information, or std::nullopt if no failover has occurred.
- Details: Returns detailed information about the most recent failover (whether successful or failed), enabling operators to investigate failover issues. Diagnostic information, or std::nullopt if no failover has occurred. Thread-safe; returns a snapshot of the last operation.

#### `std::vector< ReplicaHealthTransitionDiagnostic > getReplicaHealthHistory(const std::string &node_id, size_t max_entries=10)`
- Source: `include/replication/replication_failover_diagnostics.h`:232
- Brief: Get recent health state transitions for a specific replica.
- Parameters:
  - `node_id` (const std::string &): Replica node ID.
  - `max_entries` (size_t): Maximum number of transitions to return.
- Return: Vector of transitions, ordered by timestamp (newest first).
- Details: node_id Replica node ID. max_entries Maximum number of transitions to return. Vector of transitions, ordered by timestamp (newest first).

#### `std::string healthToString(HealthStatus s)`
- Source: `src/replication/event_stream.cpp`:236
- Brief: Health To String.
- Parameters:
  - `s` (HealthStatus): Input parameter.
- Return: Return value.
- Details: s Input parameter. Return value. Implements healthToString without additional internal calls.

#### `std::string roleToString(ReplicationRole role)`
- Source: `src/replication/event_stream.cpp`:219
- Brief: Role To String.
- Parameters:
  - `role` (ReplicationRole): Input parameter.
- Return: Return value.
- Details: role Input parameter. Return value. Implements roleToString without additional internal calls.

### themisdb::replication::AdvancedConflictResolver

#### `MMWriteEntry resolve(const std::string &document_id, const std::vector< MMWriteEntry > &conflicting_writes, const ResolutionContext &context)=0`
- Source: `include/replication/conflict_resolution.h`:93
- Brief: n/a
- Parameters:
  - `document_id` (const std::string &): Identifier of the document in conflict.
  - `conflicting_writes` (const std::vector< MMWriteEntry > &): Two or more concurrent MMWriteEntry values.
  - `context` (const ResolutionContext &): Additional context for resolution logic.
- Return: The winning write entry.
- Details: Select the winning write from a set of concurrent conflicting writes. document_id Identifier of the document in conflict. conflicting_writes Two or more concurrent MMWriteEntry values. context Additional context for resolution logic. The winning write entry. Constraints: Must be thread-safe and idempotent. Must return one of the entries from conflicting_writes (no fabrication). conflicting_writes.size() >= 2 is guaranteed by the caller.

#### `std::string strategyName() const =0`
- Source: `include/replication/conflict_resolution.h`:100
- Brief: n/a
- Parameters: none
- Details: Human-readable name of this resolver strategy.

#### `~AdvancedConflictResolver()=default`
- Source: `include/replication/conflict_resolution.h`:78
- Brief: n/a
- Parameters: none

### themisdb::replication::AsyncWalShipper

#### `AsyncWalShipper(AsyncWalShipper &&)=delete`
- Source: `include/replication/async_wal_shipper.h`:204
- Brief: n/a
- Parameters:
  - `<unnamed>` (AsyncWalShipper &&): n/a

#### `AsyncWalShipper(WalShippingConfig config)`
- Source: `include/replication/async_wal_shipper.h`:186
- Brief: Construct and start the background shipping thread.
- Parameters:
  - `config` (WalShippingConfig): WalShippingConfig; copied on construction.
- Details: config WalShippingConfig; copied on construction. Thread safety: Construction is not thread-safe; ensure no concurrent access to this object until construction completes. The background worker thread is started immediately and will begin processing queued segments.

#### `AsyncWalShipper(const AsyncWalShipper &)=delete`
- Source: `include/replication/async_wal_shipper.h`:202
- Brief: n/a
- Parameters:
  - `<unnamed>` (const AsyncWalShipper &): n/a

#### `std::vector< double > buildHistogramBounds(uint32_t max_lag_ms, uint32_t buckets)`
- Source: `include/replication/async_wal_shipper.h`:439
- Brief: Build histogram bucket bounds for the given config.
- Parameters:
  - `max_lag_ms` (uint32_t): Input parameter.
  - `buckets` (uint32_t): Input parameter.
- Return: Vector of bucket upper bounds (size == buckets).
- Details: static Generates a geometric series of bucket upper bounds from 1 ms to 10× max_lag_ms, divided into buckets intervals. max_lag_ms Configuration maximum lag in milliseconds. buckets Number of histogram buckets to generate. Vector of bucket upper bounds (size == buckets). Bucket +Inf is implicit (not stored); samples above the highest bucket are placed in the last bucket. max_lag_ms Input parameter. buckets Input parameter. Return value.

#### `int64_t currentLagMs() const`
- Source: `include/replication/async_wal_shipper.h`:299
- Brief: Return the current shipping lag in milliseconds.
- Parameters: none
- Return: Current lag in milliseconds (0 if queue is empty).
- Details: Defined as the wall-clock time between the moment the oldest segment was enqueued (WalSegment::enqueue_time) and now. When the queue is empty, returns 0. This is the primary lag value monitored against max_lag_ms. Current lag in milliseconds (0 if queue is empty). Thread safety: Acquires queue_mutex_; safe to call concurrently. Timing: Lag is calculated using std::chrono::steady_clock for monotonic timing (immune to system clock adjustments).

#### `void dispatchSegment(const WalSegment &seg)`
- Source: `include/replication/async_wal_shipper.h`:413
- Brief: Dispatch a single segment: invoke ship handler, update stats/histogram.
- Parameters:
  - `seg` (const WalSegment &): Input parameter.
- Details: Dispatch Segment. seg Segment to dispatch (const ref; not modified). Exception safety: Strong (no state corruption on exception from callback; callback exceptions are caught and suppressed). Lock order: histogram (4) → stats (3) (respects lock hierarchy). seg Input parameter.

#### `bool enqueueSegment(WalSegment segment)`
- Source: `include/replication/async_wal_shipper.h`:266
- Brief: Enqueue a WAL segment for async shipping.
- Parameters:
  - `segment` (WalSegment): Input parameter.
- Return: true when the segment was accepted; false when the queue is at capacity (back-pressure signal to the caller).
- Details: Enqueue Segment. Implements backpressure control: when the queue reaches max_queue_depth, this returns false to signal the caller to back off (drop segment, wait, or retry). segment Segment to ship; moved into the internal queue. true when the segment was accepted; false when the queue is at capacity (back-pressure signal to the caller). Thread safety: Acquires queue_mutex_; safe to call concurrently. Condition variable is notified after enqueue. Backpressure: When false is returned, caller must implement backpressure strategy (e.g., wait, retry, or drop). Segment ownership: Moved into queue; original segment object left in moved-from state (safe to destroy). segment Input parameter. True when the operation succeeds.

#### `std::string exportPrometheusMetrics() const`
- Source: `include/replication/async_wal_shipper.h`:326
- Brief: Export Prometheus-format metrics text.
- Parameters: none
- Return: Prometheus-format string (valid metric text).
- Details: Exposes all WAL shipping metrics as Prometheus 0.0.4 format text. Includes histogram (replication_wal_lag_ms), counters, and gauge metrics. Metrics exported: replication_wal_lag_ms histogram (buckets at geometric intervals) replication_wal_segments_enqueued_total counter replication_wal_segments_shipped_total counter replication_wal_segments_dropped_total counter replication_wal_lag_alerts_total counter replication_wal_bytes_shipped_total counter Labels per metric: local_dc: local datacenter identifier remote_dc: remote datacenter endpoint Prometheus-format string (valid metric text). Thread safety: Acquires stats_mutex_ and histogram_mutex_; safe to call concurrently. Format: Follows Prometheus v0.0.4 text exposition format.

#### `AsyncWalShipper & operator=(AsyncWalShipper &&)=delete`
- Source: `include/replication/async_wal_shipper.h`:205
- Brief: n/a
- Parameters:
  - `<unnamed>` (AsyncWalShipper &&): n/a

#### `AsyncWalShipper & operator=(const AsyncWalShipper &)=delete`
- Source: `include/replication/async_wal_shipper.h`:203
- Brief: n/a
- Parameters:
  - `<unnamed>` (const AsyncWalShipper &): n/a

#### `void recordLagSample(int64_t lag_ms)`
- Source: `include/replication/async_wal_shipper.h`:424
- Brief: Record one lag sample into the histogram.
- Parameters:
  - `lag_ms` (int64_t): Input parameter.
- Details: Record Lag Sample. Updates histogram buckets and sum. Negative lags are clamped to 0. lag_ms Lag in milliseconds. Thread safety: Acquires histogram_mutex_; safe to call concurrently. lag_ms Input parameter.

#### `void setAlertCallback(AlertCallback cb)`
- Source: `include/replication/async_wal_shipper.h`:222
- Brief: Set the alert callback invoked when lag exceeds max_lag_ms.
- Parameters:
  - `cb` (AlertCallback): Input parameter.
- Details: Set Alert Callback. Thread-safe; replaces any previously registered callback. cb Callback function; nullptr to unregister. Callback receives lag in milliseconds. Callback exceptions are caught and suppressed. Thread safety: Acquires callback_mutex_; safe to call concurrently. cb Input parameter.

#### `void setShipHandler(ShipHandler handler)`
- Source: `include/replication/async_wal_shipper.h`:240
- Brief: Set the transport handler for WAL segment shipping.
- Parameters:
  - `handler` (ShipHandler): Input parameter.
- Details: Set Ship Handler. Must be called before the first enqueueSegment() call if a real transport is required (default is no-op counting handler). handler Transport handler function. Receives WalSegment by const ref. Returns true on success, false on transient failure. Exceptions from handler are NOT caught; handler must be exception-safe. Thread safety: Acquires callback_mutex_; safe to call concurrently. Lifetime: Handler is copied; caller retains ownership of any captured state. handler Input parameter.

#### `WalShippingStats stats() const`
- Source: `include/replication/async_wal_shipper.h`:283
- Brief: Return current stats snapshot.
- Parameters: none
- Return: WalShippingStats containing segments_enqueued, segments_shipped, segments_dropped, lag_alerts_fired, current and max lag, and bytes counters.
- Details: Thread-safe; returns a consistent snapshot of current statistics. WalShippingStats containing segments_enqueued, segments_shipped, segments_dropped, lag_alerts_fired, current and max lag, and bytes counters. Thread safety: Acquires stats_mutex_; safe to call concurrently.

#### `void stop()`
- Source: `include/replication/async_wal_shipper.h`:346
- Brief: Gracefully stop the background thread.
- Parameters: none
- Details: Stop. Sets stop_requested flag and notifies the worker thread. Drains the remaining queue before stopping (best-effort; does not block indefinitely). Safe to call multiple times. Thread safety: Acquires queue_mutex_; safe to call concurrently. Graceful shutdown: Worker thread will process all remaining queued segments before exiting (no forced termination). Blocking: May block briefly while joining worker thread.

#### `void workerLoop()`
- Source: `include/replication/async_wal_shipper.h`:401
- Brief: Background thread main loop.
- Parameters: none
- Details: Worker Loop. Runs in dedicated thread; dequeues segments and calls dispatchSegment(). Exits when stop_requested is true and queue is empty.

#### `~AsyncWalShipper()`
- Source: `include/replication/async_wal_shipper.h`:199
- Brief: Stop the background thread and release resources.
- Parameters: none
- Details: Blocks until the background thread has exited gracefully (best-effort). Attempts to drain remaining queue segments before stopping. Thread safety: This method acquires queue_mutex_ and may be called from any thread. Exception safety: Noexcept.

### themisdb::replication::BatchedAckTracker

#### `BatchedAckTracker()`
- Source: `include/replication/replication_manager.h`:1392
- Brief: n/a
- Parameters: none

#### `BatchedAckTracker(const AckBatchConfig &config)`
- Source: `include/replication/replication_manager.h`:1393
- Brief: n/a
- Parameters:
  - `config` (const AckBatchConfig &): n/a

#### `std::optional< AckBatch > dequeuePendingAcks()`
- Source: `include/replication/replication_manager.h`:1401
- Brief: Dequeue Pending Acks.
- Parameters: none
- Return: Return value.
- Details: Return value. Calls: lock(), empty(), std::move(), front(), pop().

#### `void flushLoop()`
- Source: `include/replication/replication_manager.h`:1434
- Brief: Flush Loop.
- Parameters: none
- Details: Calls: load(), lock(), wait_for(), std::chrono::milliseconds(), empty(), flushPending().

#### `void flushPending()`
- Source: `include/replication/replication_manager.h`:1435
- Brief: Flush Pending.
- Parameters: none
- Details: Calls: empty(), std::move(), std::chrono::system_clock::now(), fetch_add(), size(), rlock(), push().

#### `void forceFlush()`
- Source: `include/replication/replication_manager.h`:1404
- Brief: Force Flush.
- Parameters: none
- Details: Calls: lock(), flushPending().

#### `uint64_t getHighestAcked() const`
- Source: `include/replication/replication_manager.h`:1407
- Brief: n/a
- Parameters: none

#### `Stats getStats() const`
- Source: `include/replication/replication_manager.h`:1414
- Brief: n/a
- Parameters: none

#### `void recordApplied(uint64_t sequence_number)`
- Source: `include/replication/replication_manager.h`:1397
- Brief: Record Applied.
- Parameters:
  - `sequence_number` (uint64_t): Input parameter.
- Details: sequence_number Input parameter. Calls: lock(), push_back(), load(), store(), size(), flushPending(), notify_one().

#### `~BatchedAckTracker()`
- Source: `include/replication/replication_manager.h`:1394
- Brief: n/a
- Parameters: none

### themisdb::replication::BidirectionalReplicationManager

#### `BidirectionalReplicationManager(const BidiConfig &config)`
- Source: `include/replication/replication_manager.h`:2313
- Brief: n/a
- Parameters:
  - `config` (const BidiConfig &): n/a

#### `BidirectionalReplicationManager(const BidirectionalReplicationManager &)=delete`
- Source: `include/replication/replication_manager.h`:2317
- Brief: n/a
- Parameters:
  - `<unnamed>` (const BidirectionalReplicationManager &): n/a

#### `bool applyRemoteDDL(const std::string &ddl_statement, const std::string &schema_version, uint64_t origin_seq)`
- Source: `include/replication/replication_manager.h`:2423
- Brief: Apply Remote DDL.
- Parameters:
  - `ddl_statement` (const std::string &): Input parameter.
  - `schema_version` (const std::string &): Input parameter.
  - `origin_seq` (uint64_t): Input parameter.
- Return: True when the operation succeeds.
- Details: Simulate an incoming DDL event from the peer. Delegates to applyRemoteWrite() with is_ddl=true. ddl_statement Input parameter. schema_version Input parameter. origin_seq Input parameter. True when the operation succeeds.

#### `bool applyRemoteWrite(const BidiWriteEntry &entry)`
- Source: `include/replication/replication_manager.h`:2369
- Brief: Apply a remote write in bidirectional (multi-master) scenario Key consistency guarantees: 1.
- Parameters:
  - `entry` (const BidiWriteEntry &): Input parameter.
- Return: True when the operation succeeds.
- Details: Apply an incoming write that was received from the peer. Origin tracking: if the entry's origin_node equals remote_node_id and replicate_foreign_changes is false (default), the change is applied locally but NOT re-forwarded back to the peer, breaking the loop. Conflict detection: if a pending local write targets the same (collection, document_id), handleConflict() is called to resolve it. Fail-closed invariants: When origin tracking is enabled, incoming writes must carry non-empty origin_node and origin_seq > 0. Stale/duplicate writes from the same origin (origin_seq <= last seen sequence for the same document) are rejected. Returns true when the entry was accepted and applied. entry Input parameter. True when the operation succeeds. Loop prevention: Track origin node and sequence to avoid re-applying local writes 2. Causal ordering: Use origin_sequence to detect causal relationships 3. Conflict detection: Identify concurrent writes via timestamps/clocks 4. Conflict resolution: Apply configured strategy (LWW, CRDT, etc.) to merge state This ensures strong eventual consistency: all replicas converge to same state when all writes have been exchanged and conflicts resolved. Calls: empty(), makeDocKey(), lk(), find(), end(), detectConflict(), handleConflict(), erase().

#### `bool detectConflict(const BidiWriteEntry &incoming, const BidiWriteEntry &existing) const`
- Source: `include/replication/replication_manager.h`:2452
- Brief: n/a
- Parameters:
  - `incoming` (const BidiWriteEntry &): n/a
  - `existing` (const BidiWriteEntry &): n/a
- Details: Detect whether two entries targeting the same (collection, document_id) constitute a conflict. Two writes conflict when both have been submitted since the last known-good sync point (i.e. their sequence numbers are both ahead of the last acknowledged remote sequence).

#### `std::vector< BidiConflictRecord > getConflictHistory() const`
- Source: `include/replication/replication_manager.h`:2380
- Brief: n/a
- Parameters: none
- Details: Return all conflict records (both auto-resolved and pending manual resolution).

#### `ConflictResolution getEffectiveStrategy(const std::string &collection) const`
- Source: `include/replication/replication_manager.h`:2409
- Brief: n/a
- Parameters:
  - `collection` (const std::string &): n/a
- Details: Read back the effective strategy for a collection.

#### `OriginInfo getOrigin(const std::string &document_id) const`
- Source: `include/replication/replication_manager.h`:2435
- Brief: n/a
- Parameters:
  - `document_id` (const std::string &): n/a

#### `std::vector< BidiConflictRecord > getPendingConflicts() const`
- Source: `include/replication/replication_manager.h`:2386
- Brief: n/a
- Parameters: none
- Details: Return only the conflict records that are awaiting manual resolution (i.e. strategy == CUSTOM and no manual resolution has been applied yet).

#### `SyncStatus getSyncStatus() const`
- Source: `include/replication/replication_manager.h`:2374
- Brief: n/a
- Parameters: none
- Details: Current synchronisation status snapshot.

#### `void handleConflict(const BidiWriteEntry &local_write, const BidiWriteEntry &remote_write, bool is_ddl)`
- Source: `include/replication/replication_manager.h`:2458
- Brief: Handle Conflict.
- Parameters:
  - `local_write` (const BidiWriteEntry &): Input parameter.
  - `remote_write` (const BidiWriteEntry &): Input parameter.
  - `is_ddl` (bool): Input parameter.
- Details: Record a conflict and apply the configured strategy. local_write Input parameter. remote_write Input parameter. is_ddl Input parameter.

#### `bool isLocalOrigin(const OriginInfo &origin) const`
- Source: `include/replication/replication_manager.h`:2436
- Brief: n/a
- Parameters:
  - `origin` (const OriginInfo &): n/a

#### `std::string makeDocKey(const std::string &collection, const std::string &document_id) const`
- Source: `include/replication/replication_manager.h`:2485
- Brief: n/a
- Parameters:
  - `collection` (const std::string &): n/a
  - `document_id` (const std::string &): n/a

#### `BidirectionalReplicationManager & operator=(const BidirectionalReplicationManager &)=delete`
- Source: `include/replication/replication_manager.h`:2318
- Brief: n/a
- Parameters:
  - `<unnamed>` (const BidirectionalReplicationManager &): n/a

#### `bool resolveConflict(const std::string &document_id, const std::string &winner_node)`
- Source: `include/replication/replication_manager.h`:2399
- Brief: ── Conflict resolution ───────────────────────────────────────────────────────
- Parameters:
  - `document_id` (const std::string &): Identifier of the document.
  - `winner_node` (const std::string &): Input parameter.
- Return: True when the operation succeeds.
- Details: Manually resolve a conflict by nominating which node's write wins. Locates the conflict record by document_id (most recent conflict for that document), marks it resolved, and updates resolved_write to the nominated node's write. winner_node must be either local_node_id or remote_node_id. Returns true when a matching unresolved conflict was found and resolved. document_id Identifier of the document. winner_node Input parameter. True when the operation succeeds.

#### `BidiWriteEntry resolveWrite(const BidiWriteEntry &local, const BidiWriteEntry &remote, ConflictResolution strategy) const`
- Source: `include/replication/replication_manager.h`:2442
- Brief: n/a
- Parameters:
  - `local` (const BidiWriteEntry &): n/a
  - `remote` (const BidiWriteEntry &): n/a
  - `strategy` (ConflictResolution): n/a
- Details: Apply the configured resolution strategy and return the winning entry.

#### `void setCollectionStrategy(const std::string &collection, ConflictResolution strategy)`
- Source: `include/replication/replication_manager.h`:2405
- Brief: ── Configuration helpers ─────────────────────────────────────────────────────
- Parameters:
  - `collection` (const std::string &): Input parameter.
  - `strategy` (ConflictResolution): Input parameter.
- Details: Update the conflict resolution strategy for a specific collection. collection Input parameter. strategy Input parameter.

#### `bool start()`
- Source: `include/replication/replication_manager.h`:2325
- Brief: ── Lifecycle ────────────────────────────────────────────────────────────────
- Parameters: none
- Return: True when the operation succeeds.
- Details: Activate bidirectional replication. Returns true on success, false when the manager is already running or the configuration is invalid (e.g. local_node_id == remote_node_id). True when the operation succeeds. Calls: empty(), compare_exchange_strong().

#### `void stop()`
- Source: `include/replication/replication_manager.h`:2331
- Brief: Stop.
- Parameters: none
- Details: Gracefully stop replication and release all resources. Safe to call even if start() was never called. Calls: store().

#### `uint64_t submitWrite(const std::string &document_id, const std::string &collection, const std::string &operation, const std::string &data, bool is_ddl=false)`
- Source: `include/replication/replication_manager.h`:2345
- Brief: ── Write path ───────────────────────────────────────────────────────────────
- Parameters:
  - `document_id` (const std::string &): Identifier of the document.
  - `collection` (const std::string &): Input parameter.
  - `operation` (const std::string &): Input parameter.
  - `data` (const std::string &): Input parameter.
  - `is_ddl` (bool): Input parameter.
- Return: Return value.
- Details: Submit a local write for bidirectional replication. If track_origin is enabled, the entry is tagged with local_node_id. The write is enqueued for forwarding to the peer on the next sync cycle (or immediately if sync_interval_ms == 0). Returns the assigned local sequence number. Returns 0 when stop() has been called. document_id Identifier of the document. collection Input parameter. operation Input parameter. data Input parameter. is_ddl Input parameter. Return value.

#### `void updateRemoteSequence(uint64_t remote_seq, int64_t lag_ms=0)`
- Source: `include/replication/replication_manager.h`:2417
- Brief: Update Remote Sequence.
- Parameters:
  - `remote_seq` (uint64_t): Input parameter.
  - `lag_ms` (int64_t): Input parameter.
- Details: Inject a remote sequence number directly (used by tests and integration harnesses that do not run a real network layer). remote_seq Input parameter. lag_ms Input parameter.

#### `~BidirectionalReplicationManager()`
- Source: `include/replication/replication_manager.h`:2314
- Brief: n/a
- Parameters: none

### themisdb::replication::CDCManager

#### `CDCManager()=default`
- Source: `include/replication/replication_manager.h`:1583
- Brief: n/a
- Parameters: none

#### `void onConflictDetected(const std::string &) override`
- Source: `include/replication/replication_manager.h`:1601
- Brief: n/a
- Parameters:
  - `<unnamed>` (const std::string &): n/a

#### `void onFailoverCompleted(const std::string &, bool) override`
- Source: `include/replication/replication_manager.h`:1605
- Brief: n/a
- Parameters:
  - `<unnamed>` (const std::string &): n/a
  - `<unnamed>` (bool): n/a

#### `void onFailoverStarted(const std::string &, const std::string &) override`
- Source: `include/replication/replication_manager.h`:1604
- Brief: n/a
- Parameters:
  - `<unnamed>` (const std::string &): n/a
  - `<unnamed>` (const std::string &): n/a

#### `void onLeaderElected(const std::string &) override`
- Source: `include/replication/replication_manager.h`:1598
- Brief: n/a
- Parameters:
  - `<unnamed>` (const std::string &): n/a

#### `void onNetworkPartitionDetected(const std::vector< std::string > &) override`
- Source: `include/replication/replication_manager.h`:1606
- Brief: n/a
- Parameters:
  - `<unnamed>` (const std::vector< std::string > &): n/a

#### `void onReplicaAdded(const ReplicaInfo &) override`
- Source: `include/replication/replication_manager.h`:1599
- Brief: n/a
- Parameters:
  - `<unnamed>` (const ReplicaInfo &): n/a

#### `void onReplicaHealthChanged(const std::string &, HealthStatus, HealthStatus) override`
- Source: `include/replication/replication_manager.h`:1603
- Brief: n/a
- Parameters:
  - `<unnamed>` (const std::string &): n/a
  - `<unnamed>` (HealthStatus): n/a
  - `<unnamed>` (HealthStatus): n/a

#### `void onReplicaRemoved(const std::string &) override`
- Source: `include/replication/replication_manager.h`:1600
- Brief: n/a
- Parameters:
  - `<unnamed>` (const std::string &): n/a

#### `void onReplicationLagWarning(int64_t) override`
- Source: `include/replication/replication_manager.h`:1602
- Brief: n/a
- Parameters:
  - `<unnamed>` (int64_t): n/a

#### `void onRoleChange(ReplicationRole, ReplicationRole) override`
- Source: `include/replication/replication_manager.h`:1597
- Brief: n/a
- Parameters:
  - `<unnamed>` (ReplicationRole): n/a
  - `<unnamed>` (ReplicationRole): n/a

#### `void onWALEntryApplied(const WALEntry &entry) override`
- Source: `include/replication/replication_manager.h`:1609
- Brief: On WALEntry Applied.
- Parameters:
  - `entry` (const WALEntry &): Input parameter.
- Details: entry Input parameter. Calls: lock(), empty(), callback(), THEMIS_ERROR(), what().

#### `uint64_t subscribe(const std::string &collection, CDCCallback callback)`
- Source: `include/replication/replication_manager.h`:1586
- Brief: ============================================================================ CDCManager Implementation (v1.
- Parameters:
  - `collection` (const std::string &): Input parameter.
  - `callback` (CDCCallback): Input parameter.
- Return: Return value.
- Details: collection Input parameter. callback Input parameter. Return value. 6.0) ============================================================================ Calls: fetch_add(), lock(), push_back(), std::move().

#### `size_t subscriptionCount() const`
- Source: `include/replication/replication_manager.h`:1592
- Brief: n/a
- Parameters: none

#### `void unsubscribe(uint64_t subscription_id)`
- Source: `include/replication/replication_manager.h`:1589
- Brief: Unsubscribe.
- Parameters:
  - `subscription_id` (uint64_t): Identifier of the subscription.
- Details: subscription_id Identifier of the subscription. Calls: lock(), erase(), std::remove_if(), begin(), end().

### themisdb::replication::CRDTConflictResolver

#### `std::string resolve(const std::string &local, const std::string &remote, const std::string &collection, const std::string &document_id) override`
- Source: `include/replication/replication_manager.h`:373
- Brief: Resolve.
- Parameters:
  - `local` (const std::string &): Input parameter.
  - `remote` (const std::string &): Input parameter.
  - `collection` (const std::string &): Input parameter.
  - `document_id` (const std::string &): Identifier of the document.
- Return: Return value.
- Details: local Input parameter. remote Input parameter. collection Input parameter. document_id Identifier of the document. Return value.

### themisdb::replication::CRDTMergeResolver

#### `CRDTMergeResolver(CRDTType type)`
- Source: `include/replication/multi_master_replication.h`:280
- Brief: n/a
- Parameters:
  - `type` (CRDTType): n/a

#### `std::string mergeFlagDW(const std::vector< MMWriteEntry > &writes)`
- Source: `include/replication/multi_master_replication.h`:303
- Brief: Merge Flag DW.
- Parameters:
  - `writes` (const std::vector< MMWriteEntry > &): Input parameter.
- Return: Return value.
- Details: writes Input parameter. Return value. Calls: extractSubArray(), extractJsonArrayStrings(), insert(), empty(), str().

#### `std::string mergeFlagEW(const std::vector< MMWriteEntry > &writes)`
- Source: `include/replication/multi_master_replication.h`:302
- Brief: Merge Flag EW.
- Parameters:
  - `writes` (const std::vector< MMWriteEntry > &): Input parameter.
- Return: Return value.
- Details: writes Input parameter. Return value. Calls: extractSubArray(), extractJsonArrayStrings(), insert(), find(), end(), str().

#### `std::string mergeGCounter(const std::vector< MMWriteEntry > &writes)`
- Source: `include/replication/multi_master_replication.h`:295
- Brief: Merge GCounter.
- Parameters:
  - `writes` (const std::vector< MMWriteEntry > &): Input parameter.
- Return: Return value.
- Details: writes Input parameter. Return value. Calls: extractJsonInts(), std::max(), str().

#### `std::string mergeGSet(const std::vector< MMWriteEntry > &writes)`
- Source: `include/replication/multi_master_replication.h`:297
- Brief: Merge GSet.
- Parameters:
  - `writes` (const std::vector< MMWriteEntry > &): Input parameter.
- Return: Return value.
- Details: writes Input parameter. Return value. Calls: size(), find(), insert(), substr(), str().

#### `std::string mergeLWWMap(const std::vector< MMWriteEntry > &writes)`
- Source: `include/replication/multi_master_replication.h`:299
- Brief: Merge LWWMap.
- Parameters:
  - `writes` (const std::vector< MMWriteEntry > &): Input parameter.
- Return: Return value.
- Details: writes Input parameter. Return value. Calls: extractJsonInts(), find(), end(), std::to_string(), str().

#### `std::string mergeLWWRegister(const std::vector< MMWriteEntry > &writes)`
- Source: `include/replication/multi_master_replication.h`:293
- Brief: CRDT merge strategies implement distributed consistency semantics: - LWW_REGISTER: Last-write-wins based on HLC timestamps (totally ordered) - MV_REGISTER: Multi-value register preserving all concurrent writes - G_COUNTER/PN_COUNTER: Grow-only/positive-negative counters (monotonic) - G_SET/OR_SET/TWO_P_SET: Set-based CRDTs (add/remove semantics) - LWW_MAP/RGA: Ordered map/sequence CRDTs - FLAG_EW/FLAG_DW: Enabled-wins/disabled-wins flags All implement strong eventual consistency (SEC) guarantees.
- Parameters:
  - `writes` (const std::vector< MMWriteEntry > &): Input parameter.
- Return: Return value.
- Details: writes Input parameter. Return value. Implements mergeLWWRegister without additional internal calls.

#### `std::string mergeMVRegister(const std::vector< MMWriteEntry > &writes)`
- Source: `include/replication/multi_master_replication.h`:294
- Brief: Merge MVRegister.
- Parameters:
  - `writes` (const std::vector< MMWriteEntry > &): Input parameter.
- Return: Return value.
- Details: writes Input parameter. Return value. Calls: str().

#### `std::string mergeORSet(const std::vector< MMWriteEntry > &writes)`
- Source: `include/replication/multi_master_replication.h`:298
- Brief: Merge ORSet.
- Parameters:
  - `writes` (const std::vector< MMWriteEntry > &): Input parameter.
- Return: Return value.
- Details: writes Input parameter. Return value. Calls: extractSubArray(), extractJsonArrayStrings(), insert(), size(), find(), substr(), begin(), emplace_back().

#### `std::string mergePNCounter(const std::vector< MMWriteEntry > &writes)`
- Source: `include/replication/multi_master_replication.h`:296
- Brief: Merge PNCounter.
- Parameters:
  - `writes` (const std::vector< MMWriteEntry > &): Input parameter.
- Return: Return value.
- Details: writes Input parameter. Return value. Calls: extractSubObject(), empty(), extractJsonInts(), std::max(), str(), serializeMap().

#### `std::string mergeRGA(const std::vector< MMWriteEntry > &writes)`
- Source: `include/replication/multi_master_replication.h`:301
- Brief: Merge RGA.
- Parameters:
  - `writes` (const std::vector< MMWriteEntry > &): Input parameter.
- Return: Return value.
- Details: writes Input parameter. Return value. Calls: size(), find(), substr(), empty(), end(), std::move(), str().

#### `std::string mergeTwoPSet(const std::vector< MMWriteEntry > &writes)`
- Source: `include/replication/multi_master_replication.h`:300
- Brief: Merge Two PSet.
- Parameters:
  - `writes` (const std::vector< MMWriteEntry > &): Input parameter.
- Return: Return value.
- Details: writes Input parameter. Return value. Calls: extractSubArray(), extractJsonArrayStrings(), insert(), count(), str().

#### `MMWriteEntry resolve(const std::string &document_id, const std::vector< MMWriteEntry > &conflicting_writes) override`
- Source: `include/replication/multi_master_replication.h`:282
- Brief: Resolve.
- Parameters:
  - `document_id` (const std::string &): Identifier of the document.
  - `conflicting_writes` (const std::vector< MMWriteEntry > &): Input parameter.
- Return: Return value.
- Details: document_id Identifier of the document. conflicting_writes Input parameter. Return value.

#### `std::string strategyName() const override`
- Source: `include/replication/multi_master_replication.h`:287
- Brief: n/a
- Parameters: none

### themisdb::replication::CompressedReplicationStream

#### `CompressedReplicationStream(const std::string &endpoint)`
- Source: `include/replication/replication_manager.h`:1329
- Brief: n/a
- Parameters:
  - `endpoint` (const std::string &): n/a

#### `CompressedReplicationStream(const std::string &endpoint, const CompressionConfig &config)`
- Source: `include/replication/replication_manager.h`:1323
- Brief: n/a
- Parameters:
  - `endpoint` (const std::string &): n/a
  - `config` (const CompressionConfig &): n/a

#### `std::string algorithmName(CompressionAlgorithm algo)`
- Source: `include/replication/replication_manager.h`:1361
- Brief: Algorithm Name.
- Parameters:
  - `algo` (CompressionAlgorithm): Input parameter.
- Return: Return value.
- Details: algo Input parameter. Return value. Implements algorithmName without additional internal calls.

#### `std::vector< uint8_t > compress(const std::vector< uint8_t > &data, CompressionAlgorithm algo) const`
- Source: `include/replication/replication_manager.h`:1355
- Brief: n/a
- Parameters:
  - `data` (const std::vector< uint8_t > &): n/a
  - `algo` (CompressionAlgorithm): n/a

#### `std::vector< uint8_t > decompress(const std::vector< uint8_t > &compressed, CompressionAlgorithm algo) const`
- Source: `include/replication/replication_manager.h`:1336
- Brief: n/a
- Parameters:
  - `compressed` (const std::vector< uint8_t > &): n/a
  - `algo` (CompressionAlgorithm): n/a

#### `CompressionStats getStats() const`
- Source: `include/replication/replication_manager.h`:1339
- Brief: n/a
- Parameters: none

#### `void resetStats()`
- Source: `include/replication/replication_manager.h`:1342
- Brief: Reset Stats.
- Parameters: none
- Details: Calls: lock().

#### `CompressionAlgorithm selectAlgorithm(size_t payload_bytes) const`
- Source: `include/replication/replication_manager.h`:1359
- Brief: n/a
- Parameters:
  - `payload_bytes` (size_t): n/a

#### `bool sendBatch(const std::vector< WALEntry > &entries)`
- Source: `include/replication/replication_manager.h`:1332
- Brief: Send Batch.
- Parameters:
  - `entries` (const std::vector< WALEntry > &): Input parameter.
- Return: True when the operation succeeds.
- Details: entries Input parameter. True when the operation succeeds. Calls: empty(), serializeEntries(), size(), selectAlgorithm(), compress(), lock(), algorithmName().

#### `std::vector< uint8_t > serializeEntries(const std::vector< WALEntry > &entries) const`
- Source: `include/replication/replication_manager.h`:1352
- Brief: n/a
- Parameters:
  - `entries` (const std::vector< WALEntry > &): n/a

### themisdb::replication::ConflictResolver

#### `MMWriteEntry resolve(const std::string &document_id, const std::vector< MMWriteEntry > &conflicting_writes)=0`
- Source: `include/replication/multi_master_replication.h`:237
- Brief: n/a
- Parameters:
  - `document_id` (const std::string &): n/a
  - `conflicting_writes` (const std::vector< MMWriteEntry > &): n/a

#### `std::string strategyName() const =0`
- Source: `include/replication/multi_master_replication.h`:243
- Brief: n/a
- Parameters: none

#### `~ConflictResolver()=default`
- Source: `include/replication/multi_master_replication.h`:234
- Brief: n/a
- Parameters: none

### themisdb::replication::CrossClusterPublication

#### `CrossClusterPublication(const std::string &name)`
- Source: `include/replication/replication_manager.h`:1659
- Brief: n/a
- Parameters:
  - `name` (const std::string &): n/a

#### `uint64_t addRemoteSubscriber(RemoteSubscriberCallback callback)`
- Source: `include/replication/replication_manager.h`:1669
- Brief: Add Remote Subscriber.
- Parameters:
  - `callback` (RemoteSubscriberCallback): Input parameter.
- Return: Return value.
- Details: callback Input parameter. Return value. Calls: fetch_add(), lock(), push_back(), std::move().

#### `std::string exportPrometheusMetrics() const`
- Source: `include/replication/replication_manager.h`:1684
- Brief: n/a
- Parameters: none

#### `PublicationFilter getFilter() const`
- Source: `include/replication/replication_manager.h`:1666
- Brief: n/a
- Parameters: none

#### `const std::string & name() const`
- Source: `include/replication/replication_manager.h`:1662
- Brief: n/a
- Parameters: none

#### `void onConflictDetected(const std::string &) override`
- Source: `include/replication/replication_manager.h`:1694
- Brief: n/a
- Parameters:
  - `<unnamed>` (const std::string &): n/a

#### `void onFailoverCompleted(const std::string &, bool) override`
- Source: `include/replication/replication_manager.h`:1698
- Brief: n/a
- Parameters:
  - `<unnamed>` (const std::string &): n/a
  - `<unnamed>` (bool): n/a

#### `void onFailoverStarted(const std::string &, const std::string &) override`
- Source: `include/replication/replication_manager.h`:1697
- Brief: n/a
- Parameters:
  - `<unnamed>` (const std::string &): n/a
  - `<unnamed>` (const std::string &): n/a

#### `void onLeaderElected(const std::string &) override`
- Source: `include/replication/replication_manager.h`:1691
- Brief: n/a
- Parameters:
  - `<unnamed>` (const std::string &): n/a

#### `void onNetworkPartitionDetected(const std::vector< std::string > &) override`
- Source: `include/replication/replication_manager.h`:1699
- Brief: n/a
- Parameters:
  - `<unnamed>` (const std::vector< std::string > &): n/a

#### `void onReplicaAdded(const ReplicaInfo &) override`
- Source: `include/replication/replication_manager.h`:1692
- Brief: n/a
- Parameters:
  - `<unnamed>` (const ReplicaInfo &): n/a

#### `void onReplicaHealthChanged(const std::string &, HealthStatus, HealthStatus) override`
- Source: `include/replication/replication_manager.h`:1696
- Brief: n/a
- Parameters:
  - `<unnamed>` (const std::string &): n/a
  - `<unnamed>` (HealthStatus): n/a
  - `<unnamed>` (HealthStatus): n/a

#### `void onReplicaRemoved(const std::string &) override`
- Source: `include/replication/replication_manager.h`:1693
- Brief: n/a
- Parameters:
  - `<unnamed>` (const std::string &): n/a

#### `void onReplicationLagWarning(int64_t) override`
- Source: `include/replication/replication_manager.h`:1695
- Brief: n/a
- Parameters:
  - `<unnamed>` (int64_t): n/a

#### `void onRoleChange(ReplicationRole, ReplicationRole) override`
- Source: `include/replication/replication_manager.h`:1690
- Brief: n/a
- Parameters:
  - `<unnamed>` (ReplicationRole): n/a
  - `<unnamed>` (ReplicationRole): n/a

#### `void onWALEntryApplied(const WALEntry &entry) override`
- Source: `include/replication/replication_manager.h`:1689
- Brief: On WALEntry Applied.
- Parameters:
  - `entry` (const WALEntry &): Input parameter.
- Details: entry Input parameter. Calls: publish().

#### `void publish(const WALEntry &entry)`
- Source: `include/replication/replication_manager.h`:1681
- Brief: Publish.
- Parameters:
  - `entry` (const WALEntry &): Input parameter.
- Details: entry Input parameter. Calls: lock(), matches(), fetch_add(), callback(), THEMIS_ERROR(), what().

#### `uint64_t publishedCount() const`
- Source: `include/replication/replication_manager.h`:1678
- Brief: n/a
- Parameters: none

#### `void removeRemoteSubscriber(uint64_t subscriber_id)`
- Source: `include/replication/replication_manager.h`:1672
- Brief: Remove Remote Subscriber.
- Parameters:
  - `subscriber_id` (uint64_t): Identifier of the subscriber.
- Details: subscriber_id Identifier of the subscriber. Calls: lock(), erase(), std::remove_if(), begin(), end().

#### `void setFilter(const PublicationFilter &filter)`
- Source: `include/replication/replication_manager.h`:1665
- Brief: Set Filter.
- Parameters:
  - `filter` (const PublicationFilter &): Input parameter.
- Details: filter Input parameter. Calls: lock().

#### `size_t subscriberCount() const`
- Source: `include/replication/replication_manager.h`:1675
- Brief: n/a
- Parameters: none

### themisdb::replication::CrossClusterSubscription

#### `CrossClusterSubscription(const std::string &name, std::shared_ptr< CrossClusterPublication > publication, ApplyCallback on_apply)`
- Source: `include/replication/replication_manager.h`:1734
- Brief: n/a
- Parameters:
  - `name` (const std::string &): n/a
  - `publication` (std::shared_ptr< CrossClusterPublication >): n/a
  - `on_apply` (ApplyCallback): n/a

#### `uint64_t appliedCount() const`
- Source: `include/replication/replication_manager.h`:1754
- Brief: n/a
- Parameters: none

#### `void disable()`
- Source: `include/replication/replication_manager.h`:1748
- Brief: Disable.
- Parameters: none
- Details: Calls: lock(), load(), removeRemoteSubscriber(), store().

#### `void enable()`
- Source: `include/replication/replication_manager.h`:1745
- Brief: Enable.
- Parameters: none
- Details: Calls: lock(), load(), addRemoteSubscriber(), on_apply_(), fetch_add(), compare_exchange_weak(), THEMIS_WARN(), store().

#### `uint64_t errorCount() const`
- Source: `include/replication/replication_manager.h`:1760
- Brief: n/a
- Parameters: none

#### `std::string exportPrometheusMetrics() const`
- Source: `include/replication/replication_manager.h`:1763
- Brief: n/a
- Parameters: none

#### `bool isEnabled() const`
- Source: `include/replication/replication_manager.h`:1751
- Brief: n/a
- Parameters: none

#### `uint64_t lastAppliedSequence() const`
- Source: `include/replication/replication_manager.h`:1757
- Brief: n/a
- Parameters: none

#### `const std::string & name() const`
- Source: `include/replication/replication_manager.h`:1742
- Brief: n/a
- Parameters: none

#### `~CrossClusterSubscription()`
- Source: `include/replication/replication_manager.h`:1739
- Brief: n/a
- Parameters: none

### themisdb::replication::CustomResolver

#### `CustomResolver(ResolverFunc resolver)`
- Source: `include/replication/multi_master_replication.h`:317
- Brief: n/a
- Parameters:
  - `resolver` (ResolverFunc): n/a

#### `MMWriteEntry resolve(const std::string &document_id, const std::vector< MMWriteEntry > &conflicting_writes) override`
- Source: `include/replication/multi_master_replication.h`:319
- Brief: Resolve.
- Parameters:
  - `document_id` (const std::string &): Identifier of the document.
  - `conflicting_writes` (const std::vector< MMWriteEntry > &): Input parameter.
- Return: Return value.
- Details: document_id Identifier of the document. conflicting_writes Input parameter. Return value.

#### `std::string strategyName() const override`
- Source: `include/replication/multi_master_replication.h`:324
- Brief: n/a
- Parameters: none

### themisdb::replication::FieldLevelMergeResolver

#### `FieldLevelMergeResolver(MergeStrategy strategy=MergeStrategy::UNION)`
- Source: `include/replication/conflict_resolution.h`:180
- Brief: n/a
- Parameters:
  - `strategy` (MergeStrategy): n/a

#### `std::string mergeFields(const std::vector< MMWriteEntry > &writes) const`
- Source: `include/replication/conflict_resolution.h`:197
- Brief: n/a
- Parameters:
  - `writes` (const std::vector< MMWriteEntry > &): n/a
- Details: Merge a collection of (field_name → value_string) maps. Returns a JSON object string.

#### `MMWriteEntry resolve(const std::string &document_id, const std::vector< MMWriteEntry > &conflicting_writes, const ResolutionContext &context) override`
- Source: `include/replication/conflict_resolution.h`:182
- Brief: Resolve.
- Parameters:
  - `document_id` (const std::string &): n/a
  - `conflicting_writes` (const std::vector< MMWriteEntry > &): Input parameter.
  - `context` (const ResolutionContext &): n/a
- Return: Return value.
- Details: param Input parameter. conflicting_writes Input parameter. param Input parameter. Return value.

#### `std::string strategyName() const override`
- Source: `include/replication/conflict_resolution.h`:188
- Brief: n/a
- Parameters: none
- Details: Human-readable name of this resolver strategy.

### themisdb::replication::GeoReplicaPlacementManager

#### `GeoReplicaPlacementManager()=default`
- Source: `include/replication/geo_placement.h`:161
- Brief: n/a
- Parameters: none

#### `GeoReplicaPlacementManager(GeoReplicaPlacementManager &&) noexcept=default`
- Source: `include/replication/geo_placement.h`:167
- Brief: n/a
- Parameters:
  - `<unnamed>` (GeoReplicaPlacementManager &&): n/a

#### `GeoReplicaPlacementManager(const GeoReplicaPlacementManager &)=delete`
- Source: `include/replication/geo_placement.h`:165
- Brief: n/a
- Parameters:
  - `<unnamed>` (const GeoReplicaPlacementManager &): n/a

#### `int dcPreferenceScore(const std::string &datacenter, const PlacementConstraints &constraints) const`
- Source: `include/replication/geo_placement.h`:252
- Brief: n/a
- Parameters:
  - `datacenter` (const std::string &): n/a
  - `constraints` (const PlacementConstraints &): n/a
- Details: Rank score for a candidate given its DC's position in the preference list. Lower index → higher score. DC not in list → lowest score.

#### `std::unordered_map< std::string, uint32_t > healthyCountPerDC(const std::vector< ReplicaInfo > &replicas) const`
- Source: `include/replication/geo_placement.h`:240
- Brief: Return a per-DC count of healthy replicas.
- Parameters:
  - `replicas` (const std::vector< ReplicaInfo > &): Current topology snapshot.
- Return: Map of datacenter ID → healthy replica count.
- Details: replicas Current topology snapshot. Map of datacenter ID → healthy replica count.

#### `bool isEligible(const ReplicaInfo &candidate, const PlacementConstraints &constraints) const`
- Source: `include/replication/geo_placement.h`:245
- Brief: Returns true when the candidate is eligible under the given constraints.
- Parameters:
  - `candidate` (const ReplicaInfo &): n/a
  - `constraints` (const PlacementConstraints &): n/a

#### `GeoReplicaPlacementManager & operator=(GeoReplicaPlacementManager &&) noexcept=default`
- Source: `include/replication/geo_placement.h`:168
- Brief: n/a
- Parameters:
  - `<unnamed>` (GeoReplicaPlacementManager &&): n/a

#### `GeoReplicaPlacementManager & operator=(const GeoReplicaPlacementManager &)=delete`
- Source: `include/replication/geo_placement.h`:166
- Brief: n/a
- Parameters:
  - `<unnamed>` (const GeoReplicaPlacementManager &): n/a

#### `std::optional< ReplicaInfo > selectFailoverCandidate(const std::vector< ReplicaInfo > &replicas, const PlacementConstraints &constraints, const std::string &failed_node_id) const`
- Source: `include/replication/geo_placement.h`:203
- Brief: Select the best failover candidate excluding a failed node.
- Parameters:
  - `replicas` (const std::vector< ReplicaInfo > &): Current replica topology snapshot.
  - `constraints` (const PlacementConstraints &): Placement constraints to apply.
  - `failed_node_id` (const std::string &): Node ID of the failed leader to exclude.
- Return: The best eligible candidate, or std::nullopt when no candidate satisfies all constraints.
- Details: Identical ranking logic as selectLeaderCandidate() but additionally skips the node identified by failed_node_id. replicas Current replica topology snapshot. constraints Placement constraints to apply. failed_node_id Node ID of the failed leader to exclude. The best eligible candidate, or std::nullopt when no candidate satisfies all constraints.

#### `std::optional< ReplicaInfo > selectLeaderCandidate(const std::vector< ReplicaInfo > &replicas, const PlacementConstraints &constraints) const`
- Source: `include/replication/geo_placement.h`:187
- Brief: Select the best leader candidate from a list of replicas.
- Parameters:
  - `replicas` (const std::vector< ReplicaInfo > &): Current replica topology snapshot.
  - `constraints` (const PlacementConstraints &): Placement constraints to apply.
- Return: The best eligible candidate, or std::nullopt when no candidate satisfies all constraints.
- Details: Ranking order (highest priority first): Candidate is in a preferred DC (ordered by preference list index). Candidate has the highest priority (ReplicaInfo::priority). Candidate has the lowest replication lag (last_applied_sequence desc). replicas Current replica topology snapshot. constraints Placement constraints to apply. The best eligible candidate, or std::nullopt when no candidate satisfies all constraints.

#### `PlacementValidationResult validatePlacement(const std::vector< ReplicaInfo > &replicas, const PlacementConstraints &constraints) const`
- Source: `include/replication/geo_placement.h`:226
- Brief: Validate whether the current topology satisfies the constraints.
- Parameters:
  - `replicas` (const std::vector< ReplicaInfo > &): Current topology snapshot.
  - `constraints` (const PlacementConstraints &): Constraints to evaluate.
- Return: PlacementValidationResult with violations and recommendations.
- Details: Checks performed: Every required DC has at least one healthy replica. Every DC meets min_copies_per_dc healthy replica count. No forbidden DC hosts a healthy replica (recommendation only — not a hard violation since placement is advisory). At least one eligible candidate exists for leader election. replicas Current topology snapshot. constraints Constraints to evaluate. PlacementValidationResult with violations and recommendations.

#### `~GeoReplicaPlacementManager()=default`
- Source: `include/replication/geo_placement.h`:162
- Brief: n/a
- Parameters: none

### themisdb::replication::GeoReplicationManager

#### `GeoReplicationManager(const GeoConfig &config)`
- Source: `include/replication/replication_manager.h`:2534
- Brief: n/a
- Parameters:
  - `config` (const GeoConfig &): n/a

#### `std::string exportPrometheusMetrics() const`
- Source: `include/replication/replication_manager.h`:2605
- Brief: n/a
- Parameters: none
- Details: Prometheus-format metrics snapshot.

#### `std::string generateSessionToken(uint64_t sequence) const`
- Source: `include/replication/replication_manager.h`:2626
- Brief: n/a
- Parameters:
  - `sequence` (uint64_t): n/a

#### `std::string getSessionToken() const`
- Source: `include/replication/replication_manager.h`:2573
- Brief: n/a
- Parameters: none
- Details: Return a fresh session token embedding the current local sequence. Pass this token to subsequent read() calls to obtain read-your-writes (SESSION consistency).

#### `std::chrono::milliseconds getStaleness(const std::string &region) const`
- Source: `include/replication/replication_manager.h`:2579
- Brief: n/a
- Parameters:
  - `region` (const std::string &): n/a
- Details: Return the estimated replication lag for a given region. Returns chrono::milliseconds::max() for unknown regions.

#### `uint64_t parseSessionToken(const std::string &token) const`
- Source: `include/replication/replication_manager.h`:2602
- Brief: n/a
- Parameters:
  - `token` (const std::string &): n/a
- Details: Validate a session token and return the sequence it encodes. Returns 0 for malformed or expired tokens.

#### `std::optional< std::string > read(const std::string &key, ConsistencyLevel consistency=ConsistencyLevel::SESSION, const std::string &session_token="")`
- Source: `include/replication/replication_manager.h`:2562
- Brief: ── Read ──────────────────────────────────────────────────────────────────────
- Parameters:
  - `key` (const std::string &): Input parameter.
  - `consistency` (ConsistencyLevel): Input parameter.
  - `session_token` (const std::string &): Input parameter.
- Return: Return value.
- Details: Read a value with the specified consistency level. Automatic routing rules: STRONG – served only if local staleness == 0. BOUNDED_STALENESS – served only if local staleness <= max_staleness_ms. SESSION – served only if local sequence >= token sequence. EVENTUAL – always served. Returns std::nullopt when the consistency constraint cannot be satisfied by the local region (caller should retry or relax the level). key Input parameter. consistency Input parameter. session_token Input parameter. Return value.

#### `std::string selectReadRegion(ConsistencyLevel consistency, const std::string &session_token="") const`
- Source: `include/replication/replication_manager.h`:2593
- Brief: n/a
- Parameters:
  - `consistency` (ConsistencyLevel): n/a
  - `session_token` (const std::string &): n/a
- Details: Select the best read region for the given consistency level and optional session token. Returns an empty string when no eligible region exists.

#### `void updateRegionStaleness(const std::string &region, int64_t staleness_ms, uint64_t last_applied_sequence=0)`
- Source: `include/replication/replication_manager.h`:2585
- Brief: ── Staleness management ──────────────────────────────────────────────────────
- Parameters:
  - `region` (const std::string &): Input parameter.
  - `staleness_ms` (int64_t): Input parameter.
  - `last_applied_sequence` (uint64_t): Input parameter.
- Details: Feed new staleness information from the replication layer. Called on every WAL acknowledgement or heartbeat from a remote region. region Input parameter. staleness_ms Input parameter. last_applied_sequence Input parameter.

#### `bool write(const std::string &key, const std::string &value, ConsistencyLevel consistency=ConsistencyLevel::SESSION)`
- Source: `include/replication/replication_manager.h`:2544
- Brief: ── Write ─────────────────────────────────────────────────────────────────────
- Parameters:
  - `key` (const std::string &): Input parameter.
  - `value` (const std::string &): Input parameter.
  - `consistency` (ConsistencyLevel): Input parameter.
- Return: True when the operation succeeds.
- Details: Write a key/value pair with the specified consistency level. Returns false only when consistency == STRONG and the local replica is not fully caught up (staleness > 0). All other levels always succeed locally. The returned session_token encodes the new sequence number for subsequent SESSION reads. key Input parameter. value Input parameter. consistency Input parameter. True when the operation succeeds.

### themisdb::replication::HybridLogicalClock

#### `HybridLogicalClock(const std::string &node_id)`
- Source: `include/replication/multi_master_replication.h`:158
- Brief: n/a
- Parameters:
  - `node_id` (const std::string &): n/a

#### `Timestamp current() const`
- Source: `include/replication/multi_master_replication.h`:167
- Brief: n/a
- Parameters: none

#### `Timestamp now()`
- Source: `include/replication/multi_master_replication.h`:161
- Brief: Now.
- Parameters: none
- Return: Return value.
- Details: Return value. Calls: lock(), time_since_epoch(), count(), load(), store().

#### `Timestamp receive(const Timestamp &received)`
- Source: `include/replication/multi_master_replication.h`:164
- Brief: Receive.
- Parameters:
  - `received` (const Timestamp &): Input parameter.
- Return: Return value.
- Details: received Input parameter. Return value. Calls: lock(), std::chrono::system_clock::now(), time_since_epoch(), count(), load(), std::max(), store().

### themisdb::replication::HybridLogicalClock::Timestamp

#### `bool operator<(const Timestamp &other) const`
- Source: `include/replication/multi_master_replication.h`:137
- Brief: n/a
- Parameters:
  - `other` (const Timestamp &): n/a

#### `bool operator==(const Timestamp &other) const`
- Source: `include/replication/multi_master_replication.h`:146
- Brief: n/a
- Parameters:
  - `other` (const Timestamp &): n/a

#### `std::string toString() const`
- Source: `include/replication/multi_master_replication.h`:151
- Brief: n/a
- Parameters: none

### themisdb::replication::IArchivalBackend

#### `bool deleteObject(const std::string &key)=0`
- Source: `include/replication/replication_manager.h`:1803
- Brief: n/a
- Parameters:
  - `key` (const std::string &): n/a

#### `std::optional< std::vector< uint8_t > > getObject(const std::string &key) const =0`
- Source: `include/replication/replication_manager.h`:1799
- Brief: n/a
- Parameters:
  - `key` (const std::string &): n/a

#### `bool putObject(const std::string &key, const std::vector< uint8_t > &data)=0`
- Source: `include/replication/replication_manager.h`:1795
- Brief: n/a
- Parameters:
  - `key` (const std::string &): n/a
  - `data` (const std::vector< uint8_t > &): n/a

#### `void setStorageTier(const std::string &key, const std::string &tier)=0`
- Source: `include/replication/replication_manager.h`:1807
- Brief: n/a
- Parameters:
  - `key` (const std::string &): n/a
  - `tier` (const std::string &): n/a

#### `~IArchivalBackend()=default`
- Source: `include/replication/replication_manager.h`:1792
- Brief: n/a
- Parameters: none

### themisdb::replication::IConflictResolver

#### `std::string resolve(const std::string &local, const std::string &remote, const std::string &collection, const std::string &document_id)=0`
- Source: `include/replication/replication_manager.h`:338
- Brief: n/a
- Parameters:
  - `local` (const std::string &): Local version
  - `remote` (const std::string &): Remote version
  - `collection` (const std::string &): n/a
  - `document_id` (const std::string &): n/a
- Return: Resolved document (merged or selected)
- Details: Resolve conflict between two versions of a document local Local version remote Remote version Resolved document (merged or selected)

#### `~IConflictResolver()=default`
- Source: `include/replication/replication_manager.h`:330
- Brief: n/a
- Parameters: none

### themisdb::replication::IReplicationListener

#### `void onConflictDetected(const std::string &document_id)=0`
- Source: `include/replication/replication_manager.h`:392
- Brief: n/a
- Parameters:
  - `document_id` (const std::string &): n/a

#### `void onFailoverCompleted(const std::string &new_leader_id, bool success)=0`
- Source: `include/replication/replication_manager.h`:396
- Brief: n/a
- Parameters:
  - `new_leader_id` (const std::string &): n/a
  - `success` (bool): n/a

#### `void onFailoverStarted(const std::string &failed_leader_id, const std::string &new_leader_id)=0`
- Source: `include/replication/replication_manager.h`:395
- Brief: n/a
- Parameters:
  - `failed_leader_id` (const std::string &): n/a
  - `new_leader_id` (const std::string &): n/a

#### `void onLeaderElected(const std::string &leader_id)=0`
- Source: `include/replication/replication_manager.h`:389
- Brief: n/a
- Parameters:
  - `leader_id` (const std::string &): n/a

#### `void onNetworkPartitionDetected(const std::vector< std::string > &unreachable_nodes)=0`
- Source: `include/replication/replication_manager.h`:397
- Brief: n/a
- Parameters:
  - `unreachable_nodes` (const std::vector< std::string > &): n/a

#### `void onReplicaAdded(const ReplicaInfo &replica)=0`
- Source: `include/replication/replication_manager.h`:390
- Brief: n/a
- Parameters:
  - `replica` (const ReplicaInfo &): n/a

#### `void onReplicaHealthChanged(const std::string &node_id, HealthStatus old_status, HealthStatus new_status)=0`
- Source: `include/replication/replication_manager.h`:394
- Brief: n/a
- Parameters:
  - `node_id` (const std::string &): n/a
  - `old_status` (HealthStatus): n/a
  - `new_status` (HealthStatus): n/a

#### `void onReplicaRemoved(const std::string &node_id)=0`
- Source: `include/replication/replication_manager.h`:391
- Brief: n/a
- Parameters:
  - `node_id` (const std::string &): n/a

#### `void onReplicationLagWarning(int64_t lag_ms)=0`
- Source: `include/replication/replication_manager.h`:393
- Brief: n/a
- Parameters:
  - `lag_ms` (int64_t): n/a

#### `void onRoleChange(ReplicationRole old_role, ReplicationRole new_role)=0`
- Source: `include/replication/replication_manager.h`:388
- Brief: n/a
- Parameters:
  - `old_role` (ReplicationRole): n/a
  - `new_role` (ReplicationRole): n/a

#### `void onWALEntryApplied(const WALEntry &)`
- Source: `include/replication/replication_manager.h`:400
- Brief: n/a
- Parameters:
  - `<unnamed>` (const WALEntry &): n/a

#### `~IReplicationListener()=default`
- Source: `include/replication/replication_manager.h`:386
- Brief: n/a
- Parameters: none

### themisdb::replication::LWWConflictResolver

#### `int64_t extractTimestamp(const std::string &json_doc)`
- Source: `include/replication/replication_manager.h`:363
- Brief: Extract the "updated_at" field from a minimal JSON payload.
- Parameters:
  - `json_doc` (const std::string &): Input parameter.
- Return: Return value.
- Details: json_doc Input parameter. Return value. We intentionally avoid a full JSON parser dependency; we just scan for the first occurrence of "updated_at":<number> pattern. BATCH B ANNOTATION: Version Tracking: This method extracts a monotonic timestamp that serves as a version vector component for causality tracking. The extracted timestamp represents the write's logical time in the system and should be propagated to all conflict resolution decision points. Consensus Expectation: All replicas must produce deterministic timestamp extraction from the same JSON payload to ensure convergence. Calls: find(), size(), std::stoll(), substr(), THEMIS_DEBUG().

#### `std::string resolve(const std::string &local, const std::string &remote, const std::string &collection, const std::string &document_id) override`
- Source: `include/replication/replication_manager.h`:353
- Brief: Resolve.
- Parameters:
  - `local` (const std::string &): Input parameter.
  - `remote` (const std::string &): Input parameter.
  - `collection` (const std::string &): n/a
  - `document_id` (const std::string &): n/a
- Return: Return value.
- Details: local Input parameter. remote Input parameter. param Input parameter. param Input parameter. Return value.

### themisdb::replication::LagAlertManager

#### `LagAlertManager()`
- Source: `include/replication/lag_alert_manager.h`:193
- Brief: Construct an alert manager with default SLO thresholds.
- Parameters: none
- Details: Default thresholds: alert_threshold_ms: 10 s critical_threshold_ms: 30 s failover_threshold_ms: 60 s failover_duration_ms: 5 min

#### `LagAlertManager(LagAlertManager &&)=default`
- Source: `include/replication/lag_alert_manager.h`:207
- Brief: n/a
- Parameters:
  - `<unnamed>` (LagAlertManager &&): n/a

#### `LagAlertManager(const LagAlertManager &)=delete`
- Source: `include/replication/lag_alert_manager.h`:205
- Brief: n/a
- Parameters:
  - `<unnamed>` (const LagAlertManager &): n/a

#### `std::map< std::string, int64_t > allReplicaLags() const`
- Source: `include/replication/lag_alert_manager.h`:403
- Brief: Get current lag for all tracked replicas.
- Parameters: none
- Return: Map of replica_id → current lag_ms.
- Details: Thread-safe snapshot. Map of replica_id → current lag_ms. Thread safety: Acquires state_mutex_; returns copy.

#### `bool checkAndAlertLagViolations()`
- Source: `include/replication/lag_alert_manager.h`:338
- Brief: Check all replicas for lag violations and emit alerts if needed.
- Parameters: none
- Return: true if any alerts were fired; false otherwise.
- Details: Check And Alert Lag Violations. Should be called periodically (e.g., every 5-10 seconds) to evaluate lag thresholds and trigger alerts. May invoke the registered callback zero or more times depending on how many thresholds are crossed. Thread-safe. Blocks briefly to acquire internal locks. true if any alerts were fired; false otherwise. Thread safety: Acquires state_mutex_; callback invoked OUTSIDE lock. Alert firing: Each replica transitions are checked; alert callback may be invoked multiple times if multiple replicas cross thresholds. True when the operation succeeds.

#### `bool checkReplicaLag(const std::string &replica_id)`
- Source: `include/replication/lag_alert_manager.h`:350
- Brief: Check a specific replica for lag violations.
- Parameters:
  - `replica_id` (const std::string &): Identifier of the replica.
- Return: true if any alert was fired for this replica; false otherwise.
- Details: Check Replica Lag. Thread-safe. replica_id Replica to check. true if any alert was fired for this replica; false otherwise. Thread safety: Acquires state_mutex_; callback invoked OUTSIDE lock. replica_id Identifier of the replica. True when the operation succeeds.

#### `void clearAllReplicas()`
- Source: `include/replication/lag_alert_manager.h`:317
- Brief: Clear all tracked replicas.
- Parameters: none
- Details: Clear All Replicas. Thread-safe. Removes all replicas and their associated state.

#### `int64_t currentTimeMs()`
- Source: `include/replication/lag_alert_manager.h`:498
- Brief: Get current time in milliseconds since Unix epoch.
- Parameters: none
- Return: Unix epoch milliseconds (system_clock).
- Details: Current Time Ms. Unix epoch milliseconds (system_clock). Thread safety: No lock required (clock operation). Return value.

#### `void emitAlert(const AlertEvent &event)`
- Source: `include/replication/lag_alert_manager.h`:489
- Brief: Emit an alert event if a callback is registered.
- Parameters:
  - `event` (const AlertEvent &): Input parameter.
- Details: Emit Alert. Invokes callback OUTSIDE the state_mutex_ to prevent deadlock if callback tries to update replica lag. Callback exceptions are caught. event AlertEvent to emit. Lock requirement: Caller must hold state_mutex_ upon entry. Method releases lock before invoking callback, then re-acquires. Exception safety: Callback exceptions are caught and suppressed. event Input parameter.

#### `void evaluateReplicaAlerts(ReplicaState &state, const std::string &replica_id)`
- Source: `include/replication/lag_alert_manager.h`:475
- Brief: Check if a replica is transitioning into or currently in alert state.
- Parameters:
  - `state` (ReplicaState &): Input/output parameter.
  - `replica_id` (const std::string &): Identifier of the replica.
- Details: Evaluate Replica Alerts. Called under state_mutex_ lock. May invoke alert callback (outside lock). Updates replica state (in_alert, in_critical, alert_count, critical_count). state Replica state to evaluate and update. replica_id Replica identifier (for alert events). Lock requirement: Caller must NOT hold state_mutex_ (method acquires). state Input/output parameter. replica_id Identifier of the replica.

#### `std::string exportPrometheusMetrics() const`
- Source: `include/replication/lag_alert_manager.h`:418
- Brief: Export Prometheus-format metrics text.
- Parameters: none
- Details: Exposes per-replica metrics: replication_lag_ms{replica_id} — Current lag (gauge) lag_alert_triggered_total{replica_id} — Total alert transitions lag_critical_triggered_total{replica_id} — Total critical transitions failover_initiated_total{replica_id} — Total failover initiations Thread-safe. Returns Prometheus 0.0.4 format text. Thread safety: Acquires state_mutex_; returns copy.

#### `std::tuple< uint64_t, uint64_t, uint64_t > getAlertStats(const std::string &replica_id) const`
- Source: `include/replication/lag_alert_manager.h`:435
- Brief: Get alert statistics for a replica.
- Parameters:
  - `replica_id` (const std::string &): Replica to query.
- Return: Tuple of (alert_count, critical_count, failover_count), or all zeros if replica not tracked.
- Details: Thread-safe. replica_id Replica to query. Tuple of (alert_count, critical_count, failover_count), or all zeros if replica not tracked. Thread safety: Acquires state_mutex_; safe to call concurrently.

#### `int64_t getReplicaLag(const std::string &replica_id) const`
- Source: `include/replication/lag_alert_manager.h`:298
- Brief: Get the current lag for a specific replica.
- Parameters:
  - `replica_id` (const std::string &): Replica to query.
- Return: Current lag in milliseconds, or 0 if replica not tracked.
- Details: Thread-safe. replica_id Replica to query. Current lag in milliseconds, or 0 if replica not tracked. Thread safety: Acquires state_mutex_; safe to call concurrently.

#### `LagAlertManager & operator=(LagAlertManager &&)=default`
- Source: `include/replication/lag_alert_manager.h`:208
- Brief: n/a
- Parameters:
  - `<unnamed>` (LagAlertManager &&): n/a

#### `LagAlertManager & operator=(const LagAlertManager &)=delete`
- Source: `include/replication/lag_alert_manager.h`:206
- Brief: n/a
- Parameters:
  - `<unnamed>` (const LagAlertManager &): n/a

#### `void removeReplica(const std::string &replica_id)`
- Source: `include/replication/lag_alert_manager.h`:310
- Brief: Remove a replica from lag tracking.
- Parameters:
  - `replica_id` (const std::string &): Identifier of the replica.
- Details: Remove Replica. Called when a replica is removed from the cluster. Thread-safe. Also clears any alert state for the removed replica. replica_id Replica to remove. Thread safety: Acquires state_mutex_; safe to call concurrently. replica_id Identifier of the replica.

#### `std::vector< std::string > replicasEligibleForFailover() const`
- Source: `include/replication/lag_alert_manager.h`:388
- Brief: Return list of replicas currently with sustained critical lag.
- Parameters: none
- Return: Vector of replica IDs eligible for failover due to lag.
- Details: These are replicas that have been above critical_threshold_ms for longer than failover_duration_ms and are eligible for automatic failover. Thread-safe snapshot. Vector of replica IDs eligible for failover due to lag. Thread safety: Acquires state_mutex_; returns copy.

#### `std::vector< std::string > replicasInAlert() const`
- Source: `include/replication/lag_alert_manager.h`:363
- Brief: Return list of replicas currently exceeding alert threshold.
- Parameters: none
- Return: Vector of replica IDs with lag > alert_threshold_ms.
- Details: Thread-safe snapshot; the list may change after the call returns. Does NOT include replicas in critical state (use replicasInCritical() for those). Vector of replica IDs with lag > alert_threshold_ms. Thread safety: Acquires state_mutex_; returns copy.

#### `std::vector< std::string > replicasInCritical() const`
- Source: `include/replication/lag_alert_manager.h`:374
- Brief: Return list of replicas currently at critical lag level.
- Parameters: none
- Return: Vector of replica IDs with lag > critical_threshold_ms.
- Details: Thread-safe snapshot. Vector of replica IDs with lag > critical_threshold_ms. Thread safety: Acquires state_mutex_; returns copy.

#### `void setAlertCallback(AlertCallback callback)`
- Source: `include/replication/lag_alert_manager.h`:253
- Brief: Register a callback to be invoked on alert events.
- Parameters:
  - `callback` (AlertCallback): Input parameter.
- Details: Set Alert Callback. Thread-safe. The callback receives AlertEvent by const reference. Implementations should avoid blocking operations and exception handling. callback Function to invoke; nullptr to clear. Callback is invoked OUTSIDE the internal lock to prevent deadlock if callback updates replica lag. Callback exceptions are caught and suppressed. Thread safety: Acquires state_mutex_; safe to call concurrently. Callback lifetime: Callback is copied; caller retains ownership. callback Input parameter.

#### `void setThresholds(const SLOThresholds &thresholds)`
- Source: `include/replication/lag_alert_manager.h`:226
- Brief: Set the SLO thresholds for all replicas.
- Parameters:
  - `thresholds` (const SLOThresholds &): Input parameter.
- Details: Set Thresholds. Thread-safe. Changes apply immediately to future lag checks. New thresholds do NOT retroactively reset alert state; ongoing alerts continue until lag drops below the alert threshold. thresholds New SLO thresholds (copied internally). Thread safety: Acquires state_mutex_; safe to call concurrently. A threshold of 0 disables that alert level. thresholds Input parameter.

#### `SLOThresholds thresholds() const`
- Source: `include/replication/lag_alert_manager.h`:237
- Brief: Return the current SLO thresholds.
- Parameters: none
- Return: Current SLO thresholds (snapshot).
- Details: Thread-safe. Current SLO thresholds (snapshot). Thread safety: Acquires state_mutex_; safe to call concurrently.

#### `void updateReplicaLag(const std::string &replica_id, int64_t lag_ms)`
- Source: `include/replication/lag_alert_manager.h`:272
- Brief: Update the current lag for a replica.
- Parameters:
  - `replica_id` (const std::string &): Identifier of the replica.
  - `lag_ms` (int64_t): Input parameter.
- Details: Update Replica Lag. Thread-safe. Typically called from replication heartbeat handlers or periodic lag measurement tasks. Updates timestamp automatically. Creates the replica entry if it doesn't exist (auto-vivification). replica_id Unique replica identifier. lag_ms Current lag in milliseconds. Thread safety: Acquires state_mutex_; safe to call concurrently. Side effect: Tracks maximum lag since last alert; useful for diagnostics. replica_id Identifier of the replica. lag_ms Input parameter.

#### `void updateReplicaLags(const std::map< std::string, int64_t > &lags)`
- Source: `include/replication/lag_alert_manager.h`:286
- Brief: Update lag for multiple replicas at once (atomic batch).
- Parameters:
  - `lags` (const std::map< std::string, int64_t > &): Map of replica_id → lag_ms.
- Details: More efficient than calling updateReplicaLag() multiple times. All replicas are updated under a single lock acquisition. Thread-safe. lags Map of replica_id → lag_ms. Thread safety: Acquires state_mutex_ once for all updates. Atomicity: All updates are applied together; no interleaving.

#### `~LagAlertManager()=default`
- Source: `include/replication/lag_alert_manager.h`:202
- Brief: Destructor.
- Parameters: none
- Details: Thread-safe; any callbacks still pending are NOT invoked. Exception safety: Noexcept.

### themisdb::replication::LagBasedReadRouter

#### `LagBasedReadRouter()`
- Source: `include/replication/replication_manager.h`:272
- Brief: Uses default RouterConfig (lag_threshold_ms = 10000).
- Parameters: none

#### `LagBasedReadRouter(const RouterConfig &config)`
- Source: `include/replication/replication_manager.h`:273
- Brief: n/a
- Parameters:
  - `config` (const RouterConfig &): n/a

#### `size_t eligibleReplicaCount(const std::vector< ReplicaInfo > &replicas) const`
- Source: `include/replication/replication_manager.h`:289
- Brief: Returns the number of replicas currently below the lag threshold.
- Parameters:
  - `replicas` (const std::vector< ReplicaInfo > &): n/a

#### `std::string exportPrometheusMetrics(const std::vector< ReplicaInfo > &replicas) const`
- Source: `include/replication/replication_manager.h`:292
- Brief: Export current routing state as Prometheus metrics.
- Parameters:
  - `replicas` (const std::vector< ReplicaInfo > &): n/a

#### `const RouterConfig & getConfig() const`
- Source: `include/replication/replication_manager.h`:296
- Brief: n/a
- Parameters: none

#### `RoutingDecision selectReplica(ReadPreference preference, const std::vector< ReplicaInfo > &replicas, const std::string &primary_node_id) const`
- Source: `include/replication/replication_manager.h`:283
- Brief: n/a
- Parameters:
  - `preference` (ReadPreference): Caller's read preference.
  - `replicas` (const std::vector< ReplicaInfo > &): Current snapshot of all replica infos.
  - `primary_node_id` (const std::string &): Node ID of the current primary.
- Return: RoutingDecision describing which node to use and why.
- Details: Select the best node to serve a read request. preference Caller's read preference. replicas Current snapshot of all replica infos. primary_node_id Node ID of the current primary. RoutingDecision describing which node to use and why.

#### `void setConfig(const RouterConfig &config)`
- Source: `include/replication/replication_manager.h`:295
- Brief: Set Config.
- Parameters:
  - `config` (const RouterConfig &): Input parameter.
- Details: config Input parameter. Implements setConfig without additional internal calls.

### themisdb::replication::LastWriteWinsResolver

#### `MMWriteEntry resolve(const std::string &document_id, const std::vector< MMWriteEntry > &conflicting_writes) override`
- Source: `include/replication/multi_master_replication.h`:252
- Brief: ============================================================================ Multi-master ConflictResolver implementations (MMWriteEntry variants) ============================================================================ NOTE: All conflict resolution operations implement proper causal ordering through: - Vector clocks for happened-before relationships - Hybrid logical clocks (HLC) for total ordering of concurrent writes - Explicit dependency tracking for write-before constraints - LWW (Last-Write-Wins) semantics based on HLC timestamp These mechanisms ensure strong eventual consistency in multi-master scenarios.
- Parameters:
  - `document_id` (const std::string &): n/a
  - `conflicting_writes` (const std::vector< MMWriteEntry > &): Input parameter.
- Return: Return value.
- Details: param Input parameter. conflicting_writes Input parameter. Return value.

#### `std::string strategyName() const override`
- Source: `include/replication/multi_master_replication.h`:257
- Brief: n/a
- Parameters: none

### themisdb::replication::LeaderElection

#### `LeaderElection(const std::string &node_id, const ReplicationConfig &config, std::shared_ptr< WALManager > wal)`
- Source: `include/replication/replication_manager.h`:455
- Brief: n/a
- Parameters:
  - `node_id` (const std::string &): n/a
  - `config` (const ReplicationConfig &): n/a
  - `wal` (std::shared_ptr< WALManager >): n/a

#### `void becomeFollower(uint64_t term, const std::string &leader_id)`
- Source: `include/replication/replication_manager.h`:556
- Brief: Become Follower.
- Parameters:
  - `term` (uint64_t): Input parameter.
  - `leader_id` (const std::string &): Identifier of the leader.
- Details: term Input parameter. leader_id Identifier of the leader. Calls: store(), clear(), std::chrono::steady_clock::now().

#### `void becomeLeader()`
- Source: `include/replication/replication_manager.h`:555
- Brief: Become Leader.
- Parameters: none
- Details: Calls: store(), notify_all().

#### `void electionLoop()`
- Source: `include/replication/replication_manager.h`:554
- Brief: Election Loop.
- Parameters: none
- Details: Calls: gen(), rd(), load(), lock(), wait_for(), std::chrono::milliseconds(), std::chrono::steady_clock::now(), count().

#### `uint64_t getCommitIndex() const`
- Source: `include/replication/replication_manager.h`:507
- Brief: n/a
- Parameters: none
- Details: Returns the highest WAL sequence that this follower knows to be committed by the leader quorum. Updated by receiveHeartbeat(). Leaders always have commit_index == their own getCurrentSequence.

#### `uint64_t getCurrentTerm() const`
- Source: `include/replication/replication_manager.h`:500
- Brief: n/a
- Parameters: none

#### `std::string getLeaderId() const`
- Source: `include/replication/replication_manager.h`:484
- Brief: n/a
- Parameters: none

#### `ReplicationRole getRole() const`
- Source: `include/replication/replication_manager.h`:481
- Brief: n/a
- Parameters: none

#### `void grantVote(uint64_t term)`
- Source: `include/replication/replication_manager.h`:494
- Brief: Grant Vote.
- Parameters:
  - `term` (uint64_t): Input parameter.
- Details: term Input parameter. Calls: lock(), load(), THEMIS_INFO(), becomeLeader().

#### `bool hasValidLease() const`
- Source: `include/replication/replication_manager.h`:523
- Brief: n/a
- Parameters: none
- Details: Returns true if this node holds a valid (non-expired) leader lease. A valid lease guarantees that no other node can have been elected leader since the lease was last renewed.

#### `bool isLeader() const`
- Source: `include/replication/replication_manager.h`:487
- Brief: n/a
- Parameters: none

#### `std::chrono::steady_clock::time_point leaseExpiresAt() const`
- Source: `include/replication/replication_manager.h`:529
- Brief: n/a
- Parameters: none
- Details: Returns the absolute time at which the current lease expires. Returns a past time-point when no lease is held.

#### `void receiveHeartbeat(uint64_t term, const std::string &leader_id, uint64_t leader_commit)`
- Source: `include/replication/replication_manager.h`:474
- Brief: Receive Heartbeat.
- Parameters:
  - `term` (uint64_t): Input parameter.
  - `leader_id` (const std::string &): Identifier of the leader.
  - `leader_commit` (uint64_t): Input parameter.
- Details: term Input parameter. leader_id Identifier of the leader. leader_commit Input parameter. Calls: lock(), load(), std::chrono::steady_clock::now(), becomeFollower(), notify_one(), getCurrentSequence(), std::min(), compare_exchange_weak().

#### `void renewLease(uint32_t duration_ms)`
- Source: `include/replication/replication_manager.h`:516
- Brief: Renew Lease.
- Parameters:
  - `duration_ms` (uint32_t): Input parameter.
- Details: Renew the leader lease for duration_ms milliseconds from now. Must only be called by the leader after successfully broadcasting a heartbeat to the quorum. duration_ms Input parameter. Calls: isLeader(), lock(), std::chrono::steady_clock::now(), std::chrono::milliseconds(), THEMIS_DEBUG().

#### `bool requestVote(uint64_t term, const std::string &candidate_id, uint64_t last_log_sequence, uint64_t last_log_term)`
- Source: `include/replication/replication_manager.h`:466
- Brief: Request Vote.
- Parameters:
  - `term` (uint64_t): Input parameter.
  - `candidate_id` (const std::string &): Identifier of the candidate.
  - `last_log_sequence` (uint64_t): Input parameter.
  - `last_log_term` (uint64_t): Input parameter.
- Return: True when the operation succeeds.
- Details: term Input parameter. candidate_id Identifier of the candidate. last_log_sequence Input parameter. last_log_term Input parameter. True when the operation succeeds. Calls: lock(), store(), clear(), empty(), getCurrentSequence(), getCurrentTerm().

#### `void setClusterSize(uint32_t size)`
- Source: `include/replication/replication_manager.h`:490
- Brief: n/a
- Parameters:
  - `size` (uint32_t): n/a

#### `void start()`
- Source: `include/replication/replication_manager.h`:497
- Brief: Start.
- Parameters: none
- Details: Calls: store(), std::chrono::steady_clock::now(), std::thread().

#### `void startElection()`
- Source: `include/replication/replication_manager.h`:463
- Brief: Start Election.
- Parameters: none
- Details: Calls: lock(), load(), incrementTerm(), store(), std::chrono::steady_clock::now(), THEMIS_INFO(), becomeLeader().

#### `~LeaderElection()`
- Source: `include/replication/replication_manager.h`:460
- Brief: n/a
- Parameters: none

### themisdb::replication::LogicalReplicationManager

#### `LogicalReplicationManager(std::shared_ptr< WALManager > wal)`
- Source: `include/replication/logical_replication.h`:111
- Brief: n/a
- Parameters:
  - `wal` (std::shared_ptr< WALManager >): Shared pointer to the WAL manager; may be nullptr (WAL-dependent operations such as restart_lsn will default to 0).
- Details: Construct a LogicalReplicationManager with default configuration. wal Shared pointer to the WAL manager; may be nullptr (WAL-dependent operations such as restart_lsn will default to 0). Persisted slots are loaded from the configured wal_directory (if set).

#### `LogicalReplicationManager(std::shared_ptr< WALManager > wal, Config config)`
- Source: `include/replication/logical_replication.h`:120
- Brief: n/a
- Parameters:
  - `wal` (std::shared_ptr< WALManager >): Shared pointer to the WAL manager; may be nullptr.
  - `config` (Config): Custom configuration (WAL directory, schema versions, decoding mode).
- Details: Construct a LogicalReplicationManager with custom configuration. wal Shared pointer to the WAL manager; may be nullptr. config Custom configuration (WAL directory, schema versions, decoding mode). Persisted slots are loaded from config.wal_directory (if non-empty).

#### `void advanceSlot(const std::string &slot_name, uint64_t lsn)`
- Source: `include/replication/logical_replication.h`:207
- Brief: Advance Slot.
- Parameters:
  - `slot_name` (const std::string &): Name of the slot.
  - `lsn` (uint64_t): Input parameter.
- Details: Advance the confirmed flush LSN for a slot after processing changes. Call this after successfully processing changes from readChanges() to prevent redelivery on restart. LSN must monotonically increase. slot_name Identifier of the slot to advance. lsn New confirmed flush LSN. Silently returns if slot does not exist or if lsn is less than the current confirmed_flush_lsn (backwards LSN is ignored). slot_name Name of the slot. lsn Input parameter. Calls: lock(), find(), end(), clear(), persistSlot().

#### `void applyTransform(LogicalChange &change) const`
- Source: `include/replication/logical_replication.h`:378
- Brief: n/a
- Parameters:
  - `change` (LogicalChange &): n/a

#### `std::string collectionKey(const std::string &collection, const std::string &document_id)`
- Source: `include/replication/logical_replication.h`:381
- Brief: Collection Key.
- Parameters:
  - `collection` (const std::string &): Input parameter.
  - `document_id` (const std::string &): Identifier of the document.
- Return: Return value.
- Details: collection Input parameter. document_id Identifier of the document. Return value. Calls: empty(), THEMIS_WARN().

#### `LogicalReplicationSlot createSlot(const std::string &slot_name, const std::string &output_plugin)`
- Source: `include/replication/logical_replication.h`:141
- Brief: Create Slot.
- Parameters:
  - `slot_name` (const std::string &): Name of the slot.
  - `output_plugin` (const std::string &): Input parameter.
- Return: LogicalReplicationSlot with restart_lsn set to current WAL position.
- Throws:
  - std::runtime_error: if a slot with slot_name already exists.
- Details: Create a logical replication slot with default filter settings. A slot represents a persistent subscription to WAL changes. Multiple slots can coexist, each with independent restart LSN and filter settings. The slot is persisted to disk for recovery after restart. slot_name Unique identifier for the slot. output_plugin Plugin name (e.g., "test_decoding", "wal2json"). LogicalReplicationSlot with restart_lsn set to current WAL position. std::runtime_error if a slot with slot_name already exists. Default filter allows all collections and operations. slot_name Name of the slot. output_plugin Input parameter. Return value. Implements createSlot without additional internal calls.

#### `LogicalReplicationSlot createSlot(const std::string &slot_name, const std::string &output_plugin, const ReplicationFilter &filter)`
- Source: `include/replication/logical_replication.h`:153
- Brief: Create Slot.
- Parameters:
  - `slot_name` (const std::string &): Name of the slot.
  - `output_plugin` (const std::string &): Input parameter.
  - `filter` (const ReplicationFilter &): Input parameter.
- Return: LogicalReplicationSlot configured with the given filter.
- Details: Create a logical replication slot with a custom filter. slot_name Unique identifier for the slot. output_plugin Plugin name for change decoding. filter Filtering rules (collections, DDL/DML, row predicates). LogicalReplicationSlot configured with the given filter. slot_name Name of the slot. output_plugin Input parameter. filter Input parameter. Return value. Implements createSlot without additional internal calls.

#### `LogicalReplicationSlot createSlot(const std::string &slot_name, const std::string &output_plugin, const ReplicationFilter &filter, bool perform_initial_sync)`
- Source: `include/replication/logical_replication.h`:169
- Brief: Create Slot.
- Parameters:
  - `slot_name` (const std::string &): Name of the slot.
  - `output_plugin` (const std::string &): Input parameter.
  - `filter` (const ReplicationFilter &): Input parameter.
  - `perform_initial_sync` (bool): Input parameter.
- Return: LogicalReplicationSlot with initial sync pending if requested.
- Details: Create a logical replication slot with initial sync option. slot_name Unique identifier for the slot. output_plugin Plugin name for change decoding. filter Filtering rules. perform_initial_sync If true, initial SNAPSHOT changes are provided at slot creation; if false, slot starts at current WAL position. LogicalReplicationSlot with initial sync pending if requested. slot_name Name of the slot. output_plugin Input parameter. filter Input parameter. perform_initial_sync Input parameter. Return value. Implements createSlot without additional internal calls.

#### `LogicalReplicationSlot createSlot(const std::string &slot_name, const std::string &output_plugin, const ReplicationFilter &filter, bool perform_initial_sync, std::vector< LogicalChange > initial_snapshot)`
- Source: `include/replication/logical_replication.h`:188
- Brief: Create Slot.
- Parameters:
  - `slot_name` (const std::string &): Name of the slot.
  - `output_plugin` (const std::string &): Input parameter.
  - `filter` (const ReplicationFilter &): Input parameter.
  - `perform_initial_sync` (bool): Input parameter.
  - `initial_snapshot` (std::vector< LogicalChange >): Input parameter.
- Return: LogicalReplicationSlot with custom snapshot enqueued.
- Throws:
  - std::invalid_argument: if an error occurs.
  - std::runtime_error: if an error occurs.
- Details: Create a logical replication slot with custom initial snapshot. Used when the initial sync should include only specific data or a subset of the current snapshot. slot_name Unique identifier for the slot. output_plugin Plugin name for change decoding. filter Filtering rules. perform_initial_sync If true, initial_snapshot changes are delivered first. initial_snapshot LogicalChange list to deliver before starting incremental. LogicalReplicationSlot with custom snapshot enqueued. slot_name Name of the slot. output_plugin Input parameter. filter Input parameter. perform_initial_sync Input parameter. initial_snapshot Input parameter. Return value. std::invalid_argument if an error occurs. std::runtime_error if an error occurs. Calls: empty(), trimCopy(), isSupportedRowFilter(), getCurrentSequence(), documentIdFromChange(), push_back(), std::move(), collectionKey().

#### `std::string documentIdFromChange(const LogicalChange &change) const`
- Source: `include/replication/logical_replication.h`:379
- Brief: n/a
- Parameters:
  - `change` (const LogicalChange &): n/a

#### `bool evaluateRowFilter(const std::string &expression, const nlohmann::json &payload) const`
- Source: `include/replication/logical_replication.h`:376
- Brief: n/a
- Parameters:
  - `expression` (const std::string &): n/a
  - `payload` (const nlohmann::json &): n/a

#### `Stats getStats() const`
- Source: `include/replication/logical_replication.h`:348
- Brief: n/a
- Parameters: none

#### `bool hasSlot(const std::string &slot_name) const`
- Source: `include/replication/logical_replication.h`:223
- Brief: n/a
- Parameters:
  - `slot_name` (const std::string &): Slot identifier.
- Return: true if slot exists; false otherwise.
- Details: Check if a slot exists by name. slot_name Slot identifier. true if slot exists; false otherwise.

#### `std::vector< LogicalReplicationSlot > listSlots() const`
- Source: `include/replication/logical_replication.h`:215
- Brief: n/a
- Parameters: none
- Return: Vector of LogicalReplicationSlot structures for all slots in the manager.
- Details: List all existing logical replication slots. Vector of LogicalReplicationSlot structures for all slots in the manager.

#### `void loadPersistedSlots()`
- Source: `include/replication/logical_replication.h`:370
- Brief: Load Persisted Slots.
- Parameters: none
- Details: Calls: slotStatePath(), empty(), dir(), fs::exists(), fs::create_directories(), THEMIS_WARN(), string(), message().

#### `LogicalChange makeLogicalChange(const WALEntry &entry) const`
- Source: `include/replication/logical_replication.h`:377
- Brief: n/a
- Parameters:
  - `entry` (const WALEntry &): n/a

#### `bool matchesFilter(const LogicalChange &change, const ReplicationFilter &filter) const`
- Source: `include/replication/logical_replication.h`:375
- Brief: n/a
- Parameters:
  - `change` (const LogicalChange &): n/a
  - `filter` (const ReplicationFilter &): n/a

#### `void onConflictDetected(const std::string &document_id) override`
- Source: `include/replication/logical_replication.h`:305
- Brief: On Conflict Detected.
- Parameters:
  - `document_id` (const std::string &): Identifier of the document involved in the conflict.
- Details: Callback: invoked when a write conflict is detected. (Inherited from IReplicationListener) document_id Identifier of the document involved in the conflict. param Input parameter. Implements onConflictDetected without additional internal calls.

#### `void onFailoverCompleted(const std::string &new_leader_id, bool success) override`
- Source: `include/replication/logical_replication.h`:344
- Brief: On Failover Completed.
- Parameters:
  - `new_leader_id` (const std::string &): Node ID of the newly elected leader.
  - `success` (bool): true if failover succeeded; false if aborted.
- Details: Callback: invoked when failover completes. (Inherited from IReplicationListener) new_leader_id Node ID of the newly elected leader. success true if failover succeeded; false if aborted. param Input parameter. bool Input parameter. Implements onFailoverCompleted without additional internal calls.

#### `void onFailoverStarted(const std::string &failed_leader_id, const std::string &new_leader_id) override`
- Source: `include/replication/logical_replication.h`:334
- Brief: On Failover Started.
- Parameters:
  - `failed_leader_id` (const std::string &): Node ID of the current leader being failed over.
  - `new_leader_id` (const std::string &): Node ID of the candidate new leader.
- Details: Callback: invoked when failover is initiated. (Inherited from IReplicationListener) failed_leader_id Node ID of the current leader being failed over. new_leader_id Node ID of the candidate new leader. param Input parameter. param Input parameter. Implements onFailoverStarted without additional internal calls.

#### `void onLeaderElected(const std::string &leader_id) override`
- Source: `include/replication/logical_replication.h`:281
- Brief: On Leader Elected.
- Parameters:
  - `leader_id` (const std::string &): Node ID of the newly elected leader.
- Details: Callback: invoked when a new leader is elected. (Inherited from IReplicationListener) leader_id Node ID of the newly elected leader. param Input parameter. Implements onLeaderElected without additional internal calls.

#### `void onNetworkPartitionDetected(const std::vector< std::string > &unreachable_nodes) override`
- Source: `include/replication/logical_replication.h`:345
- Brief: On Network Partition Detected.
- Parameters:
  - `unreachable_nodes` (const std::vector< std::string > &): n/a
- Details: param Input parameter. Implements onNetworkPartitionDetected without additional internal calls.

#### `void onReplicaAdded(const ReplicaInfo &replica) override`
- Source: `include/replication/logical_replication.h`:289
- Brief: On Replica Added.
- Parameters:
  - `replica` (const ReplicaInfo &): Information about the added replica.
- Details: Callback: invoked when a replica is added to the replication group. (Inherited from IReplicationListener) replica Information about the added replica. param Input parameter. Implements onReplicaAdded without additional internal calls.

#### `void onReplicaHealthChanged(const std::string &node_id, HealthStatus old_status, HealthStatus new_status) override`
- Source: `include/replication/logical_replication.h`:323
- Brief: On Replica Health Changed.
- Parameters:
  - `node_id` (const std::string &): Node ID of the replica.
  - `old_status` (HealthStatus): Previous health status.
  - `new_status` (HealthStatus): New health status (HEALTHY, DEGRADED, FAILED, UNKNOWN).
- Details: Callback: invoked when a replica's health status changes. (Inherited from IReplicationListener) node_id Node ID of the replica. old_status Previous health status. new_status New health status (HEALTHY, DEGRADED, FAILED, UNKNOWN). param Input parameter. HealthStatus Input parameter. HealthStatus Input parameter. Implements onReplicaHealthChanged without additional internal calls.

#### `void onReplicaRemoved(const std::string &node_id) override`
- Source: `include/replication/logical_replication.h`:297
- Brief: On Replica Removed.
- Parameters:
  - `node_id` (const std::string &): Node ID of the removed replica.
- Details: Callback: invoked when a replica is removed from the replication group. (Inherited from IReplicationListener) node_id Node ID of the removed replica. param Input parameter. Implements onReplicaRemoved without additional internal calls.

#### `void onReplicationLagWarning(int64_t lag_ms) override`
- Source: `include/replication/logical_replication.h`:313
- Brief: On Replication Lag Warning.
- Parameters:
  - `lag_ms` (int64_t): Replication lag in milliseconds.
- Details: Callback: invoked when replication lag exceeds a threshold. (Inherited from IReplicationListener) lag_ms Replication lag in milliseconds. int64_t Input parameter. Implements onReplicationLagWarning without additional internal calls.

#### `void onRoleChange(ReplicationRole old_role, ReplicationRole new_role) override`
- Source: `include/replication/logical_replication.h`:273
- Brief: On Role Change.
- Parameters:
  - `old_role` (ReplicationRole): Previous role.
  - `new_role` (ReplicationRole): New role (LEADER, FOLLOWER, CANDIDATE, etc.).
- Details: Callback: invoked when this node changes replication roles. (Inherited from IReplicationListener) old_role Previous role. new_role New role (LEADER, FOLLOWER, CANDIDATE, etc.). ReplicationRole Input parameter. ReplicationRole Input parameter. Implements onRoleChange without additional internal calls.

#### `void onWALEntryApplied(const WALEntry &entry) override`
- Source: `include/replication/logical_replication.h`:346
- Brief: On WALEntry Applied.
- Parameters:
  - `entry` (const WALEntry &): Input parameter.
- Details: entry Input parameter. Calls: makeLogicalChange(), lock(), push_back(), empty(), documentIdFromChange(), count(), collectionKey(), matchesFilter().

#### `void persistSlot(const SlotRuntime &slot) const`
- Source: `include/replication/logical_replication.h`:371
- Brief: n/a
- Parameters:
  - `slot` (const SlotRuntime &): n/a

#### `std::vector< LogicalChange > readChanges(const std::string &slot_name, uint32_t max_changes=1000)`
- Source: `include/replication/logical_replication.h`:246
- Brief: Read Changes.
- Parameters:
  - `slot_name` (const std::string &): Name of the slot.
  - `max_changes` (uint32_t): Input parameter.
- Return: Vector of LogicalChange entries; empty if slot does not exist or has no buffered changes.
- Details: Read logical changes from a specific slot. Retrieves up to max_changes changes that have not yet been confirmed flushed (i.e., changes with LSN >= confirmed_flush_lsn). Changes are filtered according to the slot's ReplicationFilter (collection, DDL/DML, row predicates). slot_name Identifier of the slot. max_changes Maximum number of changes to return (default: 1000). Vector of LogicalChange entries; empty if slot does not exist or has no buffered changes. Changes are removed from the in-memory buffer immediately upon return. Call advanceSlot() after processing to persist the confirmed_flush_lsn so that changes are not re-delivered after a restart. slot_name Name of the slot. max_changes Input parameter. Return value. Calls: lock(), find(), end(), size(), reserve(), push_back(), std::move(), front().

#### `void recordDDLChange(const std::string &ddl_statement, const std::string &schema_version="", uint64_t lsn=0)`
- Source: `include/replication/logical_replication.h`:258
- Brief: Record DDLChange.
- Parameters:
  - `ddl_statement` (const std::string &): Input parameter.
  - `schema_version` (const std::string &): Input parameter.
  - `lsn` (uint64_t): Input parameter.
- Details: Record a DDL change (schema modification) into the logical stream. DDL changes are captured as DDLCHANGE_TYPE logical changes and delivered to all slots with replicate_ddl=true. ddl_statement The DDL SQL or DDL description (e.g., "ALTER TABLE ..."). schema_version Optional schema version label (e.g., "v1.2.3"). lsn Optional specific WAL LSN to associate; if 0, uses current WAL position. ddl_statement Input parameter. schema_version Input parameter. lsn Input parameter. Calls: empty(), getCurrentSequence(), std::chrono::system_clock::now(), lock(), reserve(), size(), push_back(), slog().

#### `std::string slotStatePath(const std::string &slot_name) const`
- Source: `include/replication/logical_replication.h`:372
- Brief: n/a
- Parameters:
  - `slot_name` (const std::string &): n/a

#### `~LogicalReplicationManager() override=default`
- Source: `include/replication/logical_replication.h`:121
- Brief: n/a
- Parameters: none

### themisdb::replication::MMWriteEntry

#### `std::optional< MMWriteEntry > deserialize(const std::vector< uint8_t > &data)`
- Source: `include/replication/multi_master_replication.h`:194
- Brief: Deserialize.
- Parameters:
  - `data` (const std::vector< uint8_t > &): n/a
- Return: Return value.
- Details: raw Input parameter. Return value. Calls: size(), THEMIS_WARN(), readUint32(), s(), begin(), readString(), VectorClock::fromJson(), readUint64().

#### `std::vector< uint8_t > serialize() const`
- Source: `include/replication/multi_master_replication.h`:193
- Brief: n/a
- Parameters: none

### themisdb::replication::MembershipChangeEntry

#### `bool isJointPhase() const`
- Source: `include/replication/raft_v2.h`:149
- Brief: Returns true when this is a JOINT-phase entry.
- Parameters: none

### themisdb::replication::MembershipChangeManager

#### `MembershipChangeManager(std::shared_ptr< RaftV2ClusterConfig > config, const std::string &node_id, std::shared_ptr< WALManager > wal)`
- Source: `include/replication/raft_v2.h`:200
- Brief: Construct the manager with the current cluster configuration.
- Parameters:
  - `config` (std::shared_ptr< RaftV2ClusterConfig >): Shared cluster configuration (modified in-place).
  - `node_id` (const std::string &): ID of the local node (must be the current leader).
  - `wal` (std::shared_ptr< WALManager >): WAL manager used to persist configuration entries.
- Details: config Shared cluster configuration (modified in-place). node_id ID of the local node (must be the current leader). wal WAL manager used to persist configuration entries.

#### `void applyEntry(const MembershipChangeEntry &entry)`
- Source: `include/replication/raft_v2.h`:253
- Brief: Apply an incoming configuration entry from a leader (follower path).
- Parameters:
  - `entry` (const MembershipChangeEntry &): Input parameter.
- Details: Apply Entry. Followers must apply configuration entries as soon as they are written to the local log (not only after commit), per Raft §4.1. entry Input parameter. Calls: lock(), isInJointConsensus(), empty(), count(), beginAddMember(), beginRemoveMember(), commitTransition(), reset().

#### `std::shared_ptr< const RaftV2ClusterConfig > currentConfig() const`
- Source: `include/replication/raft_v2.h`:268
- Brief: Returns the current cluster configuration (read-only view).
- Parameters: none

#### `bool isChangeInProgress() const`
- Source: `include/replication/raft_v2.h`:258
- Brief: True while a membership change is in flight.
- Parameters: none

#### `void onJointCommitted(uint64_t log_index)`
- Source: `include/replication/raft_v2.h`:237
- Brief: Called by the leader once the JOINT entry at log_index is committed (majority of both C_old and C_new have acked it).
- Parameters:
  - `log_index` (uint64_t): Input parameter.
- Details: On Joint Committed. Writes the COMMIT-phase entry to the WAL. log_index Input parameter. Calls: lock(), writeEntry(), str(), append(), getCurrentSequence().

#### `void onNewConfigCommitted()`
- Source: `include/replication/raft_v2.h`:245
- Brief: Called once the COMMIT entry has been replicated to C_new quorum.
- Parameters: none
- Details: On New Config Committed. Finalises the transition: C_new becomes the active configuration and C_old is discarded. After this call isChangeInProgress() returns false. Calls: lock(), commitTransition(), reset().

#### `std::optional< MembershipChangeEntry > pendingEntry() const`
- Source: `include/replication/raft_v2.h`:263
- Brief: Returns the pending change entry, if any.
- Parameters: none

#### `MembershipChangeEntry proposeAdd(const std::string &node_id)`
- Source: `include/replication/raft_v2.h`:217
- Brief: Propose adding a new voting member.
- Parameters:
  - `node_id` (const std::string &): n/a
- Return: Return value.
- Throws:
  - std::runtime_error: if a change is already in progress.
- Details: Propose Add. Writes a JOINT-phase configuration entry to the WAL and begins joint consensus. The caller must replicate the returned entry and call onJointCommitted() once a quorum of both C_old and C_new has acknowledged it. std::runtime_error if a change is already in progress. new_node_id Identifier of the new node. Return value.

#### `MembershipChangeEntry proposeRemove(const std::string &node_id)`
- Source: `include/replication/raft_v2.h`:227
- Brief: Propose removing an existing voting member.
- Parameters:
  - `node_id` (const std::string &): n/a
- Return: Return value.
- Throws:
  - std::runtime_error: if a change is already in progress or the resulting cluster would be empty.
- Details: Propose Remove. Same semantics as proposeAdd() but for removal. std::runtime_error if a change is already in progress or the resulting cluster would be empty. target_node_id Identifier of the target node. Return value.

#### `MembershipChangeEntry writeEntry(MembershipChangeEntry::Phase phase, const std::set< std::string > &old_members, const std::set< std::string > &new_members)`
- Source: `include/replication/raft_v2.h`:277
- Brief: Write Entry.
- Parameters:
  - `phase` (MembershipChangeEntry::Phase): Input parameter.
  - `old_members` (const std::set< std::string > &): Input parameter.
  - `new_members` (const std::set< std::string > &): Input parameter.
- Return: Return value.
- Details: phase Input parameter. old_members Input parameter. new_members Input parameter. Return value.

### themisdb::replication::MultiMasterReplicationManager

#### `MultiMasterReplicationManager(const MMReplicationConfig &config)`
- Source: `include/replication/multi_master_replication.h`:375
- Brief: n/a
- Parameters:
  - `config` (const MMReplicationConfig &): n/a

#### `void addPeer(const MMPeerInfo &peer)`
- Source: `include/replication/multi_master_replication.h`:417
- Brief: Add Peer.
- Parameters:
  - `peer` (const MMPeerInfo &): Input parameter.
- Details: peer Input parameter. Calls: lock(), THEMIS_INFO().

#### `void antiEntropySync(const std::string &peer_id)`
- Source: `include/replication/multi_master_replication.h`:542
- Brief: ---------------------- Internal: Anti-Entropy ----------------------
- Parameters:
  - `peer_id` (const std::string &): Identifier of the peer.
- Details: peer_id Identifier of the peer. Calls: lock(), find(), end(), getMissingWrites(), sendToPeer().

#### `bool detectConflict(const MMWriteEntry &incoming, const MMWriteEntry &existing)`
- Source: `include/replication/multi_master_replication.h`:538
- Brief: ---------------------- Internal: Conflict Detection & Resolution ----------------------
- Parameters:
  - `incoming` (const MMWriteEntry &): Input parameter.
  - `existing` (const MMWriteEntry &): Input parameter.
- Return: True when the operation succeeds.
- Details: incoming Input parameter. existing Input parameter. True when the operation succeeds.

#### `std::string exportPrometheusMetrics() const`
- Source: `include/replication/multi_master_replication.h`:482
- Brief: n/a
- Parameters: none

#### `MMPeerInfo getLocalInfo() const`
- Source: `include/replication/multi_master_replication.h`:420
- Brief: n/a
- Parameters: none

#### `std::vector< MMWriteEntry > getMissingWrites(const VectorClock &peer_clock)`
- Source: `include/replication/multi_master_replication.h`:543
- Brief: Get Missing Writes.
- Parameters:
  - `peer_clock` (const VectorClock &): Input parameter.
- Return: Return value.
- Details: peer_clock Input parameter. Return value.

#### `std::vector< MMPeerInfo > getPeers() const`
- Source: `include/replication/multi_master_replication.h`:419
- Brief: n/a
- Parameters: none

#### `uint64_t getReplicationLag() const`
- Source: `include/replication/multi_master_replication.h`:433
- Brief: n/a
- Parameters: none

#### `Stats getStats() const`
- Source: `include/replication/multi_master_replication.h`:447
- Brief: n/a
- Parameters: none

#### `TopologySnapshot getTopologySnapshot() const`
- Source: `include/replication/multi_master_replication.h`:479
- Brief: n/a
- Parameters: none
- Details: Build a topology snapshot for visualization (web UI / REST API). Returns the local node and all known peers with their current state and estimated replication lag.

#### `std::vector< ConflictRecord > getUnresolvedConflicts() const`
- Source: `include/replication/multi_master_replication.h`:428
- Brief: n/a
- Parameters: none

#### `void handleConflict(const std::string &document_id, const std::vector< MMWriteEntry > &conflicting_writes)`
- Source: `include/replication/multi_master_replication.h`:539
- Brief: Handle Conflict.
- Parameters:
  - `document_id` (const std::string &): Identifier of the document.
  - `conflicting_writes` (const std::vector< MMWriteEntry > &): Input parameter.
- Details: document_id Identifier of the document. conflicting_writes Input parameter.

#### `void heartbeatLoop()`
- Source: `include/replication/multi_master_replication.h`:531
- Brief: Heartbeat Loop.
- Parameters: none
- Details: Calls: load(), std::this_thread::sleep_for(), std::chrono::milliseconds(), now(), lock().

#### `bool isRunning() const`
- Source: `include/replication/multi_master_replication.h`:381
- Brief: n/a
- Parameters: none

#### `ReadResult read(const std::string &collection, const std::string &document_id, uint32_t read_quorum=0)`
- Source: `include/replication/multi_master_replication.h`:410
- Brief: Read.
- Parameters:
  - `collection` (const std::string &): n/a
  - `document_id` (const std::string &): n/a
  - `read_quorum` (uint32_t): Input parameter.
- Return: Return value.
- Details: param Input parameter. param Input parameter. read_quorum Input parameter. Return value.

#### `void receiveFromPeer(const std::string &node_id, const MMWriteEntry &entry)`
- Source: `include/replication/multi_master_replication.h`:536
- Brief: Receive From Peer.
- Parameters:
  - `node_id` (const std::string &): Identifier of the node.
  - `entry` (const MMWriteEntry &): n/a
- Details: node_id Identifier of the node. incoming Input parameter.

#### `void registerConflictCallback(ConflictCallback callback)`
- Source: `include/replication/multi_master_replication.h`:423
- Brief: Register Conflict Callback.
- Parameters:
  - `callback` (ConflictCallback): Input parameter.
- Details: callback Input parameter. Calls: lock(), push_back(), std::move().

#### `void removePeer(const std::string &node_id)`
- Source: `include/replication/multi_master_replication.h`:418
- Brief: Remove Peer.
- Parameters:
  - `node_id` (const std::string &): Identifier of the node.
- Details: node_id Identifier of the node. Calls: lock(), erase(), THEMIS_INFO().

#### `bool replicateWrite(const MMWriteEntry &entry)`
- Source: `include/replication/multi_master_replication.h`:534
- Brief: Multi-master consensus: write is committed only after quorum acknowledges This implements quorum-based distributed consensus for concurrent writes.
- Parameters:
  - `entry` (const MMWriteEntry &): Input parameter.
- Return: True when the operation succeeds.
- Details: entry Input parameter. True when the operation succeeds. The quorum size is configurable (typically ceil((n_nodes+1)/2) for majority quorum). All writes carry vector clocks and HLC timestamps to maintain causal ordering. Calls: lock(), empty(), THEMIS_ERROR(), THEMIS_WARN(), sendToPeer().

#### `void replicationLoop()`
- Source: `include/replication/multi_master_replication.h`:530
- Brief: Replication Loop.
- Parameters: none
- Details: Calls: load(), lock(), wait_for(), std::chrono::milliseconds(), empty(), std::move(), front(), pop().

#### `bool resolveConflict(const std::string &conflict_id, const std::string &winning_write_id)`
- Source: `include/replication/multi_master_replication.h`:429
- Brief: Resolve Conflict.
- Parameters:
  - `conflict_id` (const std::string &): Identifier of the conflict.
  - `winning_write_id` (const std::string &): Identifier of the winning write.
- Return: True when the operation succeeds.
- Details: conflict_id Identifier of the conflict. winning_write_id Identifier of the winning write. True when the operation succeeds.

#### `bool sendToPeer(const std::string &node_id, const MMWriteEntry &entry)`
- Source: `include/replication/multi_master_replication.h`:535
- Brief: Send To Peer.
- Parameters:
  - `node_id` (const std::string &): Identifier of the node.
  - `entry` (const MMWriteEntry &): Input parameter.
- Return: True when the operation succeeds.
- Details: node_id Identifier of the node. entry Input parameter. True when the operation succeeds.

#### `void setConflictResolver(const std::string &collection, std::shared_ptr< ConflictResolver > resolver)`
- Source: `include/replication/multi_master_replication.h`:424
- Brief: Set Conflict Resolver.
- Parameters:
  - `collection` (const std::string &): Input parameter.
  - `resolver` (std::shared_ptr< ConflictResolver >): Input parameter.
- Details: collection Input parameter. resolver Input parameter.

#### `bool start()`
- Source: `include/replication/multi_master_replication.h`:379
- Brief: Start.
- Parameters: none
- Return: True when the operation succeeds.
- Details: True when the operation succeeds. Calls: exchange(), std::thread(), THEMIS_INFO().

#### `void stop()`
- Source: `include/replication/multi_master_replication.h`:380
- Brief: Stop.
- Parameters: none
- Details: Calls: exchange(), notify_all(), timedJoin(), THEMIS_INFO().

#### `void syncLoop()`
- Source: `include/replication/multi_master_replication.h`:532
- Brief: Sync Loop.
- Parameters: none
- Details: Calls: load(), std::this_thread::sleep_for(), std::chrono::milliseconds(), lock(), push_back(), antiEntropySync(), fetch_add().

#### `void triggerSync()`
- Source: `include/replication/multi_master_replication.h`:432
- Brief: Trigger Sync.
- Parameters: none
- Details: Calls: notify_all().

#### `std::string write(const std::string &collection, const std::string &document_id, const std::string &operation, const std::string &data, WriteCallback callback=nullptr)`
- Source: `include/replication/multi_master_replication.h`:385
- Brief: Write.
- Parameters:
  - `collection` (const std::string &): Input parameter.
  - `document_id` (const std::string &): Identifier of the document.
  - `operation` (const std::string &): Input parameter.
  - `data` (const std::string &): Input parameter.
  - `callback` (WriteCallback): Input parameter.
- Return: Return value.
- Details: collection Input parameter. document_id Identifier of the document. operation Input parameter. data Input parameter. callback Input parameter. Return value.

#### `bool writeSync(const std::string &collection, const std::string &document_id, const std::string &operation, const std::string &data, std::chrono::milliseconds timeout=std::chrono::milliseconds(5000))`
- Source: `include/replication/multi_master_replication.h`:394
- Brief: Write Sync.
- Parameters:
  - `collection` (const std::string &): Input parameter.
  - `document_id` (const std::string &): Identifier of the document.
  - `operation` (const std::string &): Input parameter.
  - `data` (const std::string &): Input parameter.
  - `timeout` (std::chrono::milliseconds): Input parameter.
- Return: True when the operation succeeds.
- Details: collection Input parameter. document_id Identifier of the document. operation Input parameter. data Input parameter. timeout Input parameter. True when the operation succeeds.

#### `~MultiMasterReplicationManager()`
- Source: `include/replication/multi_master_replication.h`:376
- Brief: n/a
- Parameters: none

### themisdb::replication::MultiRegionActiveActiveManager

#### `MultiRegionActiveActiveManager(const MultiRegionActiveActiveConfig &config)`
- Source: `include/replication/replication_manager.h`:2060
- Brief: n/a
- Parameters:
  - `config` (const MultiRegionActiveActiveConfig &): n/a

#### `std::string createSessionToken() const`
- Source: `include/replication/replication_manager.h`:2123
- Brief: n/a
- Parameters: none
- Details: Create a new session token embedding the current local sequence. The token is an opaque string that encodes the sequence number and an expiry timestamp; it is intentionally human-readable for debuggability.

#### `std::string exportPrometheusMetrics() const`
- Source: `include/replication/replication_manager.h`:2157
- Brief: n/a
- Parameters: none
- Details: Prometheus-format metrics snapshot.

#### `std::string generateSessionToken(uint64_t sequence) const`
- Source: `include/replication/replication_manager.h`:2207
- Brief: n/a
- Parameters:
  - `sequence` (uint64_t): n/a

#### `std::string generateWriteId(uint64_t sequence) const`
- Source: `include/replication/replication_manager.h`:2206
- Brief: n/a
- Parameters:
  - `sequence` (uint64_t): n/a

#### `std::vector< RegionStalenessInfo > getAllRegionStaleness() const`
- Source: `include/replication/replication_manager.h`:2146
- Brief: n/a
- Parameters: none
- Details: Snapshot of staleness for every tracked region.

#### `ConsistencyLevel getEffectiveConsistency(const std::string &collection) const`
- Source: `include/replication/replication_manager.h`:2169
- Brief: n/a
- Parameters:
  - `collection` (const std::string &): Collection (table) name to look up.
- Return: The effective ConsistencyLevel for that collection.
- Details: Return the effective consistency level for a collection. If the collection has an entry in MultiRegionActiveActiveConfig::collection_consistency_overrides, that level is returned. Otherwise config_.default_consistency is used. collection Collection (table) name to look up. The effective ConsistencyLevel for that collection.

#### `std::chrono::milliseconds getStaleness(const std::string &region_id) const`
- Source: `include/replication/replication_manager.h`:2136
- Brief: n/a
- Parameters:
  - `region_id` (const std::string &): n/a
- Details: Return the current estimated staleness for a given region. Returns max duration when the region is unknown.

#### `bool isSplitBrain() const`
- Source: `include/replication/replication_manager.h`:2184
- Brief: n/a
- Parameters: none
- Details: Return true when a split-brain condition is suspected. A split-brain is indicated when split_brain_detection_enabled is set in the configuration and every configured peer region reports an unhealthy staleness (i.e. no peer has been heard from within the max_staleness_ms * 2 window). Returns false when detection is disabled or when at least one peer is healthy. This is a heuristic based on the last staleness feed; a false negative is possible if staleness updates are delayed.

#### `bool isWithinStalenessBound(const std::string &region_id) const`
- Source: `include/replication/replication_manager.h`:2141
- Brief: n/a
- Parameters:
  - `region_id` (const std::string &): n/a
- Details: Returns true when the local region's staleness is within max_staleness_ms.

#### `uint64_t parseSessionToken(const std::string &token) const`
- Source: `include/replication/replication_manager.h`:2208
- Brief: Returns 0 on error.
- Parameters:
  - `token` (const std::string &): n/a

#### `ReadResult read(const std::string &collection, const std::string &document_id, ConsistencyLevel consistency=ConsistencyLevel::BOUNDED_STALENESS, const std::string &session_token="")`
- Source: `include/replication/replication_manager.h`:2111
- Brief: n/a
- Parameters:
  - `collection` (const std::string &): Collection (table) name.
  - `document_id` (const std::string &): Unique document identifier.
  - `consistency` (ConsistencyLevel): Requested consistency level (may be overridden per collection).
  - `session_token` (const std::string &): Optional session token for SESSION consistency (read-your-writes).
- Return: ReadResult with success=true when the consistency requirement is met.
- Details: Attempt a read at the requested consistency level. If a collection-level override exists in MultiRegionActiveActiveConfig::collection_consistency_overrides, it replaces the caller-supplied consistency parameter. Returns success=false when: STRONG: local staleness > 0 (replica is not fully caught up) BOUNDED_STALENESS: local staleness > max_staleness_ms SESSION: the local replica has not yet applied the sequence in the token collection Collection (table) name. document_id Unique document identifier. consistency Requested consistency level (may be overridden per collection). session_token Optional session token for SESSION consistency (read-your-writes). ReadResult with success=true when the consistency requirement is met.

#### `void updateRegionStaleness(const std::string &region_id, int64_t staleness_ms, uint64_t last_applied_sequence)`
- Source: `include/replication/replication_manager.h`:2152
- Brief: Update Region Staleness.
- Parameters:
  - `region_id` (const std::string &): Identifier of the region.
  - `staleness_ms` (int64_t): Input parameter.
  - `last_applied_sequence` (uint64_t): Input parameter.
- Details: Called by the replication layer whenever new WAL progress is learned for a remote region (e.g. on heartbeat or WAL ACK). region_id Identifier of the region. staleness_ms Input parameter. last_applied_sequence Input parameter.

#### `bool validateSessionToken(const std::string &token, uint64_t required_sequence) const`
- Source: `include/replication/replication_manager.h`:2129
- Brief: n/a
- Parameters:
  - `token` (const std::string &): n/a
  - `required_sequence` (uint64_t): n/a
- Details: Validate a session token and check whether the local replica has applied at least required_sequence. Returns false for malformed or expired tokens.

#### `WriteResult write(const std::string &collection, const std::string &document_id, const std::string &operation, const std::string &data, ConsistencyLevel consistency=ConsistencyLevel::SESSION, const std::string &session_token="")`
- Source: `include/replication/replication_manager.h`:2084
- Brief: n/a
- Parameters:
  - `collection` (const std::string &): Collection (table) name.
  - `document_id` (const std::string &): Unique document identifier within the collection.
  - `operation` (const std::string &): Operation type string (e.g. "INSERT", "UPDATE", "DELETE").
  - `data` (const std::string &): Serialized document payload.
  - `consistency` (ConsistencyLevel): Requested consistency level (may be overridden per collection).
  - `session_token` (const std::string &): Optional caller session token (currently unused by write).
- Return: WriteResult with success=true on acceptance, or success=false when rejected due to leader-region fencing.
- Details: Record a write locally and return a WriteResult that includes a session token embedding the new sequence number. If a collection-level override exists in MultiRegionActiveActiveConfig::collection_consistency_overrides, it replaces the caller-supplied consistency parameter. When MultiRegionActiveActiveConfig::leader_region_id is non-empty and the effective consistency is STRONG, the write is rejected (success=false) if the local region is not the designated leader. This prevents split-brain lost-update scenarios for critical writes. collection Collection (table) name. document_id Unique document identifier within the collection. operation Operation type string (e.g. "INSERT", "UPDATE", "DELETE"). data Serialized document payload. consistency Requested consistency level (may be overridden per collection). session_token Optional caller session token (currently unused by write). WriteResult with success=true on acceptance, or success=false when rejected due to leader-region fencing.

### themisdb::replication::MultiTierReplicationManager

#### `MultiTierReplicationManager(const MultiTierConfig &config={})`
- Source: `include/replication/multi_tier_replication.h`:174
- Brief: n/a
- Parameters:
  - `config` (const MultiTierConfig &): n/a

#### `MultiTierReplicationManager(const MultiTierReplicationManager &)=delete`
- Source: `include/replication/multi_tier_replication.h`:178
- Brief: n/a
- Parameters:
  - `<unnamed>` (const MultiTierReplicationManager &): n/a

#### `void applyTierChange(const std::string &collection, ReplicationTier old_tier, ReplicationTier new_tier)`
- Source: `include/replication/multi_tier_replication.h`:286
- Brief: Apply Tier Change.
- Parameters:
  - `collection` (const std::string &): Input parameter.
  - `old_tier` (ReplicationTier): Input parameter.
  - `new_tier` (ReplicationTier): Input parameter.
- Details: Apply a tier change and update promotion/demotion counters. Acquires assignments_mutex_ and stats_mutex_ internally. Must NOT be called while holding either lock. collection Input parameter. old_tier Input parameter. new_tier Input parameter.

#### `void assignTier(const std::string &collection, ReplicationTier tier)`
- Source: `include/replication/multi_tier_replication.h`:189
- Brief: Assign Tier.
- Parameters:
  - `collection` (const std::string &): Input parameter.
  - `tier` (ReplicationTier): Input parameter.
- Details: Assign a collection to the specified replication tier. If the collection is already assigned to a different tier the assignment is updated in-place. collection Input parameter. tier Input parameter.

#### `void enableAutoTiering(bool enabled)`
- Source: `include/replication/multi_tier_replication.h`:225
- Brief: Enable Auto Tiering.
- Parameters:
  - `enabled` (bool): Input parameter.
- Details: Enable or disable automatic tier promotion/demotion based on access patterns. When enabled, recordAccess() updates per-collection counters and evaluateTierPromotion() applies the promotion/demotion rules. enabled Input parameter.

#### `ReplicationTier evaluateTierPromotion(const std::string &collection)`
- Source: `include/replication/multi_tier_replication.h`:248
- Brief: Evaluate Tier Promotion.
- Parameters:
  - `collection` (const std::string &): Input parameter.
- Return: Return value.
- Details: Evaluate and (if necessary) change the tier for the given collection based on accumulated access statistics. Promotion rule: access_rate >= hot_access_threshold → TIER_1_CRITICAL Demotion rule: access_rate < cold_access_threshold → TIER_3_ARCHIVAL Otherwise: no change (or revert to TIER_2_STANDARD if currently 1 or 3) Returns the new tier (which may be the same as the current tier). Returns the current tier unchanged when auto-tiering is disabled. collection Input parameter. Return value.

#### `std::vector< CollectionAccessStats > getCollectionStats() const`
- Source: `include/replication/multi_tier_replication.h`:261
- Brief: n/a
- Parameters: none
- Details: Return per-collection access statistics. Only collections that have had at least one access recorded or an explicit tier assignment are included.

#### `std::vector< std::string > getCollectionsForTier(ReplicationTier tier) const`
- Source: `include/replication/multi_tier_replication.h`:269
- Brief: n/a
- Parameters:
  - `tier` (ReplicationTier): n/a
- Details: Return all collections currently assigned to the given tier. Collections relying on the default tier are NOT included unless they have been explicitly assigned.

#### `TierConfig getDefaultTierConfig(ReplicationTier tier) const`
- Source: `include/replication/multi_tier_replication.h`:214
- Brief: n/a
- Parameters:
  - `tier` (ReplicationTier): n/a
- Details: Return the built-in (or overridden) TierConfig for the given tier.

#### `MultiTierStats getStats() const`
- Source: `include/replication/multi_tier_replication.h`:253
- Brief: n/a
- Parameters: none
- Details: Return aggregate tier statistics.

#### `ReplicationTier getTier(const std::string &collection) const`
- Source: `include/replication/multi_tier_replication.h`:204
- Brief: n/a
- Parameters:
  - `collection` (const std::string &): n/a
- Details: Get the current replication tier for a collection. Returns the explicitly assigned tier if one exists, otherwise the default_tier from the MultiTierConfig.

#### `TierConfig getTierConfig(const std::string &collection) const`
- Source: `include/replication/multi_tier_replication.h`:209
- Brief: n/a
- Parameters:
  - `collection` (const std::string &): n/a
- Details: Get the full TierConfig for a collection's current tier.

#### `bool isAutoTieringEnabled() const`
- Source: `include/replication/multi_tier_replication.h`:228
- Brief: n/a
- Parameters: none
- Details: Returns true when auto-tiering is currently enabled.

#### `MultiTierReplicationManager & operator=(const MultiTierReplicationManager &)=delete`
- Source: `include/replication/multi_tier_replication.h`:179
- Brief: n/a
- Parameters:
  - `<unnamed>` (const MultiTierReplicationManager &): n/a

#### `void recordAccess(const std::string &collection)`
- Source: `include/replication/multi_tier_replication.h`:235
- Brief: Record Access.
- Parameters:
  - `collection` (const std::string &): Input parameter.
- Details: Record a single access to the collection. Has no effect when auto-tiering is disabled. Thread-safe. collection Input parameter.

#### `void refreshAccessRate(CollectionAccessStats &stats) const`
- Source: `include/replication/multi_tier_replication.h`:279
- Brief: n/a
- Parameters:
  - `stats` (CollectionAccessStats &): n/a
- Details: Expire old timestamps outside the rolling window and recompute access_rate_per_min and recent_accesses. Must be called while holding the stats write lock.

#### `void removeTier(const std::string &collection)`
- Source: `include/replication/multi_tier_replication.h`:196
- Brief: Remove Tier.
- Parameters:
  - `collection` (const std::string &): Input parameter.
- Details: Remove a collection's explicit tier assignment. After removal getTier() returns the default_tier from the config. collection Input parameter.

#### `~MultiTierReplicationManager()=default`
- Source: `include/replication/multi_tier_replication.h`:175
- Brief: n/a
- Parameters: none

### themisdb::replication::ParallelReplicationWorker

#### `ParallelReplicationWorker(const ParallelConfig &config)`
- Source: `include/replication/replication_manager.h`:1097
- Brief: n/a
- Parameters:
  - `config` (const ParallelConfig &): n/a

#### `Stats getStats() const`
- Source: `include/replication/replication_manager.h`:1106
- Brief: n/a
- Parameters: none

#### `void submit(const WALEntry &entry)`
- Source: `include/replication/replication_manager.h`:1101
- Brief: Submit.
- Parameters:
  - `entry` (const WALEntry &): Input parameter.
- Details: entry Input parameter. Calls: std::chrono::steady_clock::now(), dep_lock(), find(), end(), push_back(), fetch_add(), q_lock(), size().

#### `void sync()`
- Source: `include/replication/replication_manager.h`:1104
- Brief: Sync.
- Parameters: none
- Details: Calls: lock(), empty(), load(), std::this_thread::sleep_for(), std::chrono::milliseconds().

#### `void workerLoop()`
- Source: `include/replication/replication_manager.h`:1141
- Brief: Worker Loop.
- Parameters: none
- Details: Calls: load(), lock(), wait_for(), std::chrono::milliseconds(), empty(), push_back(), std::move(), front().

#### `~ParallelReplicationWorker()`
- Source: `include/replication/replication_manager.h`:1098
- Brief: n/a
- Parameters: none

### themisdb::replication::PersistentReplicationState

#### `PersistentReplicationState(const std::string &state_file_path)`
- Source: `include/replication/replication_manager.h`:1269
- Brief: n/a
- Parameters:
  - `state_file_path` (const std::string &): n/a

#### `bool exists() const`
- Source: `include/replication/replication_manager.h`:1278
- Brief: n/a
- Parameters: none

#### `State load() const`
- Source: `include/replication/replication_manager.h`:1275
- Brief: n/a
- Parameters: none

#### `bool persist(const State &state)`
- Source: `include/replication/replication_manager.h`:1272
- Brief: Persist.
- Parameters:
  - `state` (const State &): Input parameter.
- Return: True when the operation succeeds.
- Details: state Input parameter. True when the operation succeeds. Calls: lock(), ofs(), is_open(), THEMIS_ERROR(), time_since_epoch(), count(), flush(), good().

#### `void remove()`
- Source: `include/replication/replication_manager.h`:1281
- Brief: Remove.
- Parameters: none
- Details: Calls: lock().

### themisdb::replication::PublicationFilter

#### `bool matches(const WALEntry &entry) const`
- Source: `include/replication/replication_manager.h`:1638
- Brief: n/a
- Parameters:
  - `entry` (const WALEntry &): n/a

### themisdb::replication::QuorumReadManager

#### `QuorumReadManager(const QuorumReadConfig &config, const std::vector< ReplicaInfo > &replicas)`
- Source: `include/replication/replication_manager.h`:1173
- Brief: n/a
- Parameters:
  - `config` (const QuorumReadConfig &): n/a
  - `replicas` (const std::vector< ReplicaInfo > &): n/a

#### `std::string generateSessionToken(uint64_t version) const`
- Source: `include/replication/replication_manager.h`:1242
- Brief: Generate an opaque session token encoding version and an expiry timestamp.
- Parameters:
  - `version` (uint64_t): n/a

#### `uint64_t parseSessionToken(const std::string &token) const`
- Source: `include/replication/replication_manager.h`:1245
- Brief: Parse token and return the embedded version (0 on error or expiry).
- Parameters:
  - `token` (const std::string &): n/a

#### `ReplicaResponse queryReplica(const ReplicaInfo &replica, const std::string &collection, const std::string &document_id) const`
- Source: `include/replication/replication_manager.h`:1235
- Brief: n/a
- Parameters:
  - `replica` (const ReplicaInfo &): n/a
  - `collection` (const std::string &): n/a
  - `document_id` (const std::string &): n/a

#### `QuorumReadResult read(const std::string &collection, const std::string &document_id, uint32_t quorum=0, const std::string &session_token="")`
- Source: `include/replication/replication_manager.h`:1178
- Brief: Read.
- Parameters:
  - `collection` (const std::string &): Input parameter.
  - `document_id` (const std::string &): Identifier of the document.
  - `quorum` (uint32_t): Input parameter.
  - `session_token` (const std::string &): Input parameter.
- Return: Return value.
- Details: collection Input parameter. document_id Identifier of the document. quorum Input parameter. session_token Input parameter. Return value.

#### `void setDocumentFetchCallback(DocumentFetchFn fn)`
- Source: `include/replication/replication_manager.h`:1201
- Brief: Set Document Fetch Callback.
- Parameters:
  - `fn` (DocumentFetchFn): Input parameter.
- Details: Inject a data-fetch function so that queryReplica() can return real document content. The storage / RPC layer sets this at startup; tests inject a local-memory lookup. Without a callback the data field of every ReplicaResponse remains empty (original behaviour). fn Input parameter. Calls: lock(), std::move().

#### `void setLocalDocumentFetchFn(LocalDocumentFetchFn fn)`
- Source: `include/replication/replication_manager.h`:1218
- Brief: Set Local Document Fetch Fn.
- Parameters:
  - `fn` (LocalDocumentFetchFn): Input parameter.
- Details: Inject a local-storage read function used by read() when the replica list is empty (single-node deployments). Without a callback the data field remains empty and version=0 (original behaviour). fn Input parameter. Calls: lock(), std::move().

#### `void setReplicas(const std::vector< ReplicaInfo > &replicas)`
- Source: `include/replication/replication_manager.h`:1186
- Brief: Set Replicas.
- Parameters:
  - `replicas` (const std::vector< ReplicaInfo > &): Input parameter.
- Details: replicas Input parameter. Calls: lock().

### themisdb::replication::RaftV2ClusterConfig

#### `RaftV2ClusterConfig(const std::set< std::string > &members={})`
- Source: `include/replication/raft_v2.h`:51
- Brief: Create a stable (non-transitional) configuration.
- Parameters:
  - `members` (const std::set< std::string > &): Initial set of voting member node IDs.
- Details: members Initial set of voting member node IDs.

#### `void beginAddMember(const std::string &node_id)`
- Source: `include/replication/raft_v2.h`:59
- Brief: Begin a joint-consensus transition to add node_id.
- Parameters:
  - `node_id` (const std::string &): Identifier of the node.
- Throws:
  - std::runtime_error: if a membership change is already in flight.
  - std::runtime_error: if an error occurs.
- Details: Begin Add Member. std::runtime_error if a membership change is already in flight. node_id Identifier of the node. std::runtime_error if an error occurs. Calls: lock(), insert().

#### `void beginRemoveMember(const std::string &node_id)`
- Source: `include/replication/raft_v2.h`:66
- Brief: Begin a joint-consensus transition to remove node_id.
- Parameters:
  - `node_id` (const std::string &): Identifier of the node.
- Throws:
  - std::runtime_error: if a membership change is already in flight or if the resulting cluster would have fewer than 1 member.
  - std::runtime_error: if an error occurs.
- Details: Begin Remove Member. std::runtime_error if a membership change is already in flight or if the resulting cluster would have fewer than 1 member. node_id Identifier of the node. std::runtime_error if an error occurs. Calls: lock(), size(), erase().

#### `void commitTransition()`
- Source: `include/replication/raft_v2.h`:72
- Brief: Commit the pending transition, activating C_new.
- Parameters: none
- Throws:
  - std::runtime_error: if not in joint consensus.
  - std::runtime_error: if an error occurs.
- Details: Commit Transition. std::runtime_error if not in joint consensus. std::runtime_error if an error occurs. Calls: lock(), clear().

#### `std::set< std::string > getAllMembers() const`
- Source: `include/replication/raft_v2.h`:95
- Brief: Returns the union of old and new members during joint consensus, or the single active set otherwise.
- Parameters: none

#### `std::set< std::string > getNewMembers() const`
- Source: `include/replication/raft_v2.h`:98
- Brief: Current (new/target) member set.
- Parameters: none

#### `std::set< std::string > getOldMembers() const`
- Source: `include/replication/raft_v2.h`:101
- Brief: Previous member set (empty when not in transition).
- Parameters: none

#### `bool hasQuorum(const std::set< std::string > &votes) const`
- Source: `include/replication/raft_v2.h`:109
- Brief: Returns true when votes satisfy quorum requirements.
- Parameters:
  - `votes` (const std::set< std::string > &): n/a
- Details: During joint consensus both C_old and C_new must separately have majority votes. Outside joint consensus only C_new is checked.

#### `bool isInJointConsensus() const`
- Source: `include/replication/raft_v2.h`:83
- Brief: True while a joint-consensus transition is in flight.
- Parameters: none

#### `bool isMember(const std::string &node_id) const`
- Source: `include/replication/raft_v2.h`:89
- Brief: Returns true if node_id is a voting member in either C_old or C_new (or both).
- Parameters:
  - `node_id` (const std::string &): n/a

#### `size_t majority(size_t n)`
- Source: `include/replication/raft_v2.h`:123
- Brief: n/a
- Parameters:
  - `n` (size_t): n/a

#### `size_t quorumSize() const`
- Source: `include/replication/raft_v2.h`:115
- Brief: Minimum number of votes required for a simple-majority quorum in the current (non-transitional) configuration.
- Parameters: none

#### `void rollbackTransition()`
- Source: `include/replication/raft_v2.h`:78
- Brief: Roll back the pending transition, restoring C_old.
- Parameters: none
- Throws:
  - std::runtime_error: if not in joint consensus.
  - std::runtime_error: if an error occurs.
- Details: Rollback Transition. std::runtime_error if not in joint consensus. std::runtime_error if an error occurs. Calls: lock(), clear().

### themisdb::replication::RaftV2State

#### `bool hasVoted() const`
- Source: `include/replication/raft_v2.h`:169
- Brief: Returns true when this node has cast a vote in the current term.
- Parameters: none

### themisdb::replication::ReplicaInfo

#### `bool isHealthy() const`
- Source: `include/replication/replication_manager.h`:146
- Brief: n/a
- Parameters: none

#### `bool isHealthyWithTimeout(uint32_t timeout_ms) const`
- Source: `include/replication/replication_manager.h`:150
- Brief: n/a
- Parameters:
  - `timeout_ms` (uint32_t): n/a

#### `int64_t replicationLagMs() const`
- Source: `include/replication/replication_manager.h`:158
- Brief: n/a
- Parameters: none

#### `void updateHealthStatus(uint32_t heartbeat_timeout_ms, uint32_t degraded_lag_threshold_ms)`
- Source: `include/replication/replication_manager.h`:160
- Brief: Update Health Status.
- Parameters:
  - `heartbeat_timeout_ms` (uint32_t): Input parameter.
  - `degraded_lag_threshold_ms` (uint32_t): Input parameter.
- Details: heartbeat_timeout_ms Input parameter. degraded_lag_threshold_ms Input parameter. Calls: std::chrono::system_clock::now(), count().

### themisdb::replication::ReplicationAnalytics

#### `ReplicationAnalytics()`
- Source: `include/replication/replication_manager.h`:1479
- Brief: n/a
- Parameters: none

#### `std::vector< Bottleneck > detectBottlenecks() const`
- Source: `include/replication/replication_manager.h`:1492
- Brief: n/a
- Parameters: none

#### `std::string exportPrometheusMetrics() const`
- Source: `include/replication/replication_manager.h`:1495
- Brief: n/a
- Parameters: none

#### `std::vector< Insight > getInsights() const`
- Source: `include/replication/replication_manager.h`:1485
- Brief: n/a
- Parameters: none

#### `LagHistory getLagHistory(const std::string &replica_id, std::chrono::hours duration) const`
- Source: `include/replication/replication_manager.h`:1488
- Brief: n/a
- Parameters:
  - `replica_id` (const std::string &): n/a
  - `duration` (std::chrono::hours): n/a

#### `int64_t percentile(const std::vector< int64_t > &sorted, double p)`
- Source: `include/replication/replication_manager.h`:1513
- Brief: Percentile.
- Parameters:
  - `sorted` (const std::vector< int64_t > &): Input parameter.
  - `p` (double): Input parameter.
- Return: Return value.
- Details: sorted Input parameter. p Input parameter. Return value. Calls: empty(), size(), std::min().

#### `void recordLag(const std::string &replica_id, int64_t lag_ms)`
- Source: `include/replication/replication_manager.h`:1482
- Brief: Record Lag.
- Parameters:
  - `replica_id` (const std::string &): Identifier of the replica.
  - `lag_ms` (int64_t): Input parameter.
- Details: replica_id Identifier of the replica. lag_ms Input parameter. Calls: lock(), push_back(), std::chrono::system_clock::now(), size(), pop_front().

#### `void setConfig(const AnalyticsConfig &config)`
- Source: `include/replication/replication_manager.h`:1503
- Brief: Set Config.
- Parameters:
  - `config` (const AnalyticsConfig &): Input parameter.
- Details: config Input parameter. Implements setConfig without additional internal calls.

### themisdb::replication::ReplicationBenchmark

#### `ReplicationBenchmark(std::shared_ptr< WALManager > wal)`
- Source: `include/replication/replication_manager.h`:1549
- Brief: n/a
- Parameters:
  - `wal` (std::shared_ptr< WALManager >): n/a

#### `ReplicationBenchmark(std::shared_ptr< WALManager > wal, const BenchmarkConfig &config)`
- Source: `include/replication/replication_manager.h`:1547
- Brief: n/a
- Parameters:
  - `wal` (std::shared_ptr< WALManager >): n/a
  - `config` (const BenchmarkConfig &): n/a

#### `std::string format(const BenchmarkResult &result)`
- Source: `include/replication/replication_manager.h`:1555
- Brief: Format.
- Parameters:
  - `result` (const BenchmarkResult &): n/a
- Return: Return value.
- Details: r Input parameter. Return value. Calls: str().

#### `BenchmarkResult run()`
- Source: `include/replication/replication_manager.h`:1552
- Brief: Run.
- Parameters: none
- Return: Return value.
- Details: Return value. Calls: payload(), std::to_string(), append(), reserve(), std::chrono::high_resolution_clock::now(), push_back(), count(), std::sort().

### themisdb::replication::ReplicationEventStream

#### `ReplicationEventStream(const ReplicationEventStream &)=delete`
- Source: `include/replication/event_stream.h`:169
- Brief: n/a
- Parameters:
  - `<unnamed>` (const ReplicationEventStream &): n/a

#### `ReplicationEventStream(const StreamConfig &config=StreamConfig{})`
- Source: `include/replication/event_stream.h`:166
- Brief: n/a
- Parameters:
  - `config` (const StreamConfig &): n/a

#### `size_t bufferedEventCount() const`
- Source: `include/replication/event_stream.h`:214
- Brief: n/a
- Parameters: none
- Details: Number of events currently held in the ring buffer.

#### `void emit(Event ev)`
- Source: `include/replication/event_stream.h`:254
- Brief: Emit.
- Parameters:
  - `ev` (Event): Input parameter.
- Details: Emit an event: append to ring buffer and invoke matching callbacks. ev Input parameter.

#### `std::vector< Event > getEvents(std::chrono::system_clock::time_point start, std::chrono::system_clock::time_point end, std::optional< EventType > filter=std::nullopt) const`
- Source: `include/replication/event_stream.h`:207
- Brief: n/a
- Parameters:
  - `start` (std::chrono::system_clock::time_point): n/a
  - `end` (std::chrono::system_clock::time_point): n/a
  - `filter` (std::optional< EventType >): n/a
- Details: Return events from the in-memory ring buffer within [start, end). If filter is set only events of that type are returned. Events are returned in chronological order.

#### `void onConflictDetected(const std::string &document_id) override`
- Source: `include/replication/event_stream.h`:223
- Brief: On Conflict Detected.
- Parameters:
  - `document_id` (const std::string &): Identifier of the document.
- Details: document_id Identifier of the document.

#### `void onFailoverCompleted(const std::string &new_leader, bool success) override`
- Source: `include/replication/event_stream.h`:230
- Brief: On Failover Completed.
- Parameters:
  - `new_leader` (const std::string &): Input parameter.
  - `success` (bool): Input parameter.
- Details: new_leader Input parameter. success Input parameter.

#### `void onFailoverStarted(const std::string &failed_node, const std::string &new_leader) override`
- Source: `include/replication/event_stream.h`:228
- Brief: On Failover Started.
- Parameters:
  - `failed_node` (const std::string &): Input parameter.
  - `new_leader` (const std::string &): Input parameter.
- Details: failed_node Input parameter. new_leader Input parameter.

#### `void onLeaderElected(const std::string &leader_id) override`
- Source: `include/replication/event_stream.h`:220
- Brief: On Leader Elected.
- Parameters:
  - `leader_id` (const std::string &): Identifier of the leader.
- Details: leader_id Identifier of the leader.

#### `void onNetworkPartitionDetected(const std::vector< std::string > &affected) override`
- Source: `include/replication/event_stream.h`:232
- Brief: On Network Partition Detected.
- Parameters:
  - `affected` (const std::vector< std::string > &): Input parameter.
- Details: affected Input parameter.

#### `void onReplicaAdded(const ReplicaInfo &replica) override`
- Source: `include/replication/event_stream.h`:221
- Brief: On Replica Added.
- Parameters:
  - `replica` (const ReplicaInfo &): Input parameter.
- Details: replica Input parameter.

#### `void onReplicaHealthChanged(const std::string &node_id, HealthStatus old_status, HealthStatus new_status) override`
- Source: `include/replication/event_stream.h`:225
- Brief: On Replica Health Changed.
- Parameters:
  - `node_id` (const std::string &): Identifier of the node.
  - `old_status` (HealthStatus): Input parameter.
  - `new_status` (HealthStatus): Input parameter.
- Details: node_id Identifier of the node. old_status Input parameter. new_status Input parameter.

#### `void onReplicaRemoved(const std::string &node_id) override`
- Source: `include/replication/event_stream.h`:222
- Brief: On Replica Removed.
- Parameters:
  - `node_id` (const std::string &): Identifier of the node.
- Details: node_id Identifier of the node.

#### `void onReplicationLagWarning(int64_t lag_ms) override`
- Source: `include/replication/event_stream.h`:224
- Brief: On Replication Lag Warning.
- Parameters:
  - `lag_ms` (int64_t): Input parameter.
- Details: lag_ms Input parameter.

#### `void onRoleChange(ReplicationRole from, ReplicationRole to) override`
- Source: `include/replication/event_stream.h`:219
- Brief: On Role Change.
- Parameters:
  - `from` (ReplicationRole): Input parameter.
  - `to` (ReplicationRole): Input parameter.
- Details: from Input parameter. to Input parameter.

#### `void onWALEntryApplied(const WALEntry &entry) override`
- Source: `include/replication/event_stream.h`:233
- Brief: On WALEntry Applied.
- Parameters:
  - `entry` (const WALEntry &): Input parameter.
- Details: entry Input parameter.

#### `ReplicationEventStream & operator=(const ReplicationEventStream &)=delete`
- Source: `include/replication/event_stream.h`:170
- Brief: n/a
- Parameters:
  - `<unnamed>` (const ReplicationEventStream &): n/a

#### `Subscription subscribe(EventType type, EventCallback callback)`
- Source: `include/replication/event_stream.h`:184
- Brief: n/a
- Parameters:
  - `type` (EventType): Only events of this type are delivered to callback.
  - `callback` (EventCallback): Invoked synchronously on the emitting thread; must not block.
- Details: Subscribe to a specific event type. Returns a Subscription handle; the subscription is active until the handle is destroyed or cancel() is called. type Only events of this type are delivered to callback. callback Invoked synchronously on the emitting thread; must not block.

#### `Subscription subscribeAll(EventCallback callback)`
- Source: `include/replication/event_stream.h`:190
- Brief: n/a
- Parameters:
  - `callback` (EventCallback): Invoked for every event regardless of type.
- Details: Subscribe to all event types. callback Invoked for every event regardless of type.

#### `void unsubscribe(uint64_t subscription_id)`
- Source: `include/replication/event_stream.h`:196
- Brief: Unsubscribe.
- Parameters:
  - `subscription_id` (uint64_t): Identifier of the subscription.
- Details: Manually unsubscribe a previously registered handler. Safe to call after the stream is destroyed (no-op). subscription_id Identifier of the subscription.

#### `~ReplicationEventStream() override=default`
- Source: `include/replication/event_stream.h`:167
- Brief: n/a
- Parameters: none

### themisdb::replication::ReplicationEventStream::Subscription

#### `Subscription()=default`
- Source: `include/replication/event_stream.h`:132
- Brief: n/a
- Parameters: none

#### `Subscription(Subscription &&) noexcept=default`
- Source: `include/replication/event_stream.h`:138
- Brief: n/a
- Parameters:
  - `<unnamed>` (Subscription &&): n/a

#### `Subscription(const Subscription &)=delete`
- Source: `include/replication/event_stream.h`:140
- Brief: n/a
- Parameters:
  - `<unnamed>` (const Subscription &): n/a

#### `Subscription(std::shared_ptr< ReplicationEventStream > stream, uint64_t id)`
- Source: `include/replication/event_stream.h`:133
- Brief: n/a
- Parameters:
  - `stream` (std::shared_ptr< ReplicationEventStream >): n/a
  - `id` (uint64_t): n/a

#### `void cancel()`
- Source: `include/replication/event_stream.h`:144
- Brief: n/a
- Parameters: none
- Details: Manually cancel this subscription before destruction.

#### `uint64_t id() const`
- Source: `include/replication/event_stream.h`:151
- Brief: n/a
- Parameters: none

#### `Subscription & operator=(Subscription &&) noexcept=default`
- Source: `include/replication/event_stream.h`:139
- Brief: n/a
- Parameters:
  - `<unnamed>` (Subscription &&): n/a

#### `Subscription & operator=(const Subscription &)=delete`
- Source: `include/replication/event_stream.h`:141
- Brief: n/a
- Parameters:
  - `<unnamed>` (const Subscription &): n/a

#### `~Subscription()`
- Source: `include/replication/event_stream.h`:135
- Brief: n/a
- Parameters: none

### themisdb::replication::ReplicationManager

#### `ReplicationManager(const ReplicationConfig &config)`
- Source: `include/replication/replication_manager.h`:628
- Brief: n/a
- Parameters:
  - `config` (const ReplicationConfig &): Configuration parameters for replication behavior (mode, failover settings, WAL, conflict resolution, etc.)
- Details: Construct a ReplicationManager with the given configuration. config Configuration parameters for replication behavior (mode, failover settings, WAL, conflict resolution, etc.) Object is not yet active; call initialize() to start replication threads.

#### `void addListener(std::shared_ptr< IReplicationListener > listener)`
- Source: `include/replication/replication_manager.h`:785
- Brief: Add Listener.
- Parameters:
  - `listener` (std::shared_ptr< IReplicationListener >): Input parameter.
- Details: Register a listener for replication lifecycle events. The listener will be called on the replication background thread for each event (apply, failover, conflict, lag, health change, etc.). listener Shared pointer to an IReplicationListener implementation. Listeners must not block for more than 1 ms or spawn I/O. Multiple listeners can be registered; all are called for each event. listener Input parameter. Calls: lock(), push_back().

#### `void addReplica(const ReplicaInfo &replica)`
- Source: `include/replication/replication_manager.h`:735
- Brief: Add Replica.
- Parameters:
  - `replica` (const ReplicaInfo &): Input parameter.
- Throws:
  - std::invalid_argument: if node_id or endpoint is empty.
- Details: Add a replica to the replication group. The new replica will receive a full snapshot followed by incremental WAL entries. Adding a replica during active write traffic may incur lag until the replica catches up. replica Replica information (node_id and endpoint must be non-empty). std::invalid_argument if node_id or endpoint is empty. Rejects empty node_id or endpoint fail-closed to prevent silent replica registration failures. replica Input parameter. Calls: empty(), spdlog::error(), lock(), load(), spdlog::warn(), push_back(), isLeader(), start().

#### `void addWitnessNode(const std::string &node_id, const std::string &endpoint)`
- Source: `include/replication/replication_manager.h`:759
- Brief: Add Witness Node.
- Parameters:
  - `node_id` (const std::string &): Identifier of the node.
  - `endpoint` (const std::string &): Input parameter.
- Details: Add a witness node to the cluster. A witness node participates in leader-election voting (and therefore contributes to quorum) but does NOT receive WAL data. This allows a 2-node data cluster to maintain quorum without requiring a third full data replica. node_id Unique identifier for the witness node. endpoint Network address (hostname:port) of the witness node. node_id Identifier of the node. endpoint Input parameter. Calls: std::chrono::system_clock::now(), addReplica().

#### `void attemptAutomaticFailover(const std::string &failed_node_id)`
- Source: `include/replication/replication_manager.h`:1060
- Brief: Attempt Automatic Failover.
- Parameters:
  - `failed_node_id` (const std::string &): Identifier of the failed node.
- Details: failed_node_id Identifier of the failed node. Calls: hasQuorum(), electNewLeader(), getLeaderId(), notifyListeners(), onFailoverStarted(), onFailoverCompleted().

#### `void compactionLoop()`
- Source: `include/replication/replication_manager.h`:1057
- Brief: Compaction Loop.
- Parameters: none
- Details: Calls: load(), getCurrentSequence(), lock(), std::min(), getLastAckedSequence(), truncateBefore(), std::this_thread::sleep_for(), std::chrono::seconds().

#### `bool demoteToFollower()`
- Source: `include/replication/replication_manager.h`:824
- Brief: Demote To Follower.
- Parameters: none
- Return: true on success; false if this node is not the leader or if the new leader cannot be elected.
- Details: Demote this leader node to follower (used during planned maintenance). Stops accepting new writes and voluntarily steps down from leadership. Another replica will be elected as the new leader. true on success; false if this node is not the leader or if the new leader cannot be elected. True when the operation succeeds. Implements demoteToFollower without additional internal calls.

#### `bool detectNetworkPartition() const`
- Source: `include/replication/replication_manager.h`:917
- Brief: n/a
- Parameters: none
- Return: true if a partition is detected; false if cluster is connected or this node is isolated but retains quorum.
- Details: Detect if there is a network partition in the cluster. A partition is detected when: This node and some replicas are unable to communicate, and Neither partition can achieve quorum, or Quorum has been lost. true if a partition is detected; false if cluster is connected or this node is isolated but retains quorum. In leader mode: leader remains available for writes if it has quorum. In follower mode: no writes allowed; read traffic routed to healthy replicas or primary.

#### `bool electNewLeader()`
- Source: `include/replication/replication_manager.h`:1061
- Brief: Elect New Leader.
- Parameters: none
- Return: True when the operation succeeds.
- Details: True when the operation succeeds. Calls: lock(), size(), startElection(), isLeader().

#### `bool enableMultiRegion(const std::string &region_id, const std::vector< std::string > &peer_regions)`
- Source: `include/replication/replication_manager.h`:832
- Brief: Enable Multi Region.
- Parameters:
  - `region_id` (const std::string &): Identifier of the region.
  - `peer_regions` (const std::vector< std::string > &): Input parameter.
- Return: true on success
- Details: Enable multi-region replication region_id Identifier for this region peer_regions List of peer region endpoints true on success region_id Identifier of the region. peer_regions Input parameter. True when the operation succeeds. Calls: lock(), THEMIS_INFO(), std::chrono::system_clock::now(), push_back(), isLeader(), start(), std::move(), setClusterSize().

#### `std::string exportPrometheusMetrics() const`
- Source: `include/replication/replication_manager.h`:868
- Brief: n/a
- Parameters: none
- Return: Prometheus-formatted metrics string
- Details: Export metrics in Prometheus format Prometheus-formatted metrics string

#### `std::map< std::string, bool > getClusterHealth() const`
- Source: `include/replication/replication_manager.h`:862
- Brief: n/a
- Parameters: none
- Return: Health status map (node_id -> is_healthy)
- Details: Check cluster health Health status map (node_id -> is_healthy)

#### `std::string getLeaderEndpoint() const`
- Source: `include/replication/replication_manager.h`:705
- Brief: n/a
- Parameters: none
- Return: Empty string if this node is the leader; otherwise the "hostname:port" endpoint of the current leader.
- Details: Get the network endpoint of the current leader. Empty string if this node is the leader; otherwise the "hostname:port" endpoint of the current leader.

#### `const PlacementConstraints & getPlacementPolicy() const`
- Source: `include/replication/replication_manager.h`:1014
- Brief: n/a
- Parameters: none
- Return: Reference to the active PlacementConstraints, or empty constraints if no policy is set.
- Details: Get the currently active geographic replica placement constraints. Reference to the active PlacementConstraints, or empty constraints if no policy is set.

#### `ReadPreference getReadPreference() const`
- Source: `include/replication/replication_manager.h`:925
- Brief: n/a
- Parameters: none
- Return: Configured ReadPreference (PRIMARY, SECONDARY, PRIMARY_PREFERRED, etc.).
- Details: Get the read preference configuration for query routing. Configured ReadPreference (PRIMARY, SECONDARY, PRIMARY_PREFERRED, etc.). setReadPreference() to change the default.

#### `std::vector< std::pair< std::string, HealthStatus > > getReplicaHealthStatus() const`
- Source: `include/replication/replication_manager.h`:876
- Brief: n/a
- Parameters: none
- Return: Vector of (node_id, HealthStatus) pairs for all replicas, indicating whether each is HEALTHY, DEGRADED, FAILED, or UNKNOWN.
- Details: Get health status of all replicas. Vector of (node_id, HealthStatus) pairs for all replicas, indicating whether each is HEALTHY, DEGRADED, FAILED, or UNKNOWN. HealthStatus enum for interpretation of each status.

#### `std::vector< ReplicaInfo > getReplicas() const`
- Source: `include/replication/replication_manager.h`:713
- Brief: n/a
- Parameters: none
- Return: Vector of ReplicaInfo structs for all known replicas, including role, health status, and replication lag.
- Details: Get information about all configured replicas. Vector of ReplicaInfo structs for all known replicas, including role, health status, and replication lag.

#### `int64_t getReplicationLag(const std::string &replica_id) const`
- Source: `include/replication/replication_manager.h`:856
- Brief: n/a
- Parameters:
  - `replica_id` (const std::string &): Node ID of replica
- Return: Lag in milliseconds
- Details: Get replication lag for specific replica replica_id Node ID of replica Lag in milliseconds

#### `ReplicationRole getRole() const`
- Source: `include/replication/replication_manager.h`:697
- Brief: n/a
- Parameters: none
- Return: ReplicationRole::LEADER if this node is the leader; otherwise FOLLOWER, CANDIDATE, OBSERVER, or WITNESS.
- Details: Get the current replication role of this node. ReplicationRole::LEADER if this node is the leader; otherwise FOLLOWER, CANDIDATE, OBSERVER, or WITNESS.

#### `const ReplicationStats & getStats() const`
- Source: `include/replication/replication_manager.h`:721
- Brief: n/a
- Parameters: none
- Return: Const reference to the current ReplicationStats (entries replicated, failures, latencies, etc.).
- Details: Get replication runtime statistics. Const reference to the current ReplicationStats (entries replicated, failures, latencies, etc.).

#### `bool hasLeaderLease() const`
- Source: `include/replication/replication_manager.h`:987
- Brief: n/a
- Parameters: none
- Details: Returns true when this node is the leader AND its leader lease is currently valid. Can be used by routing layers to decide whether to serve a read locally.

#### `bool hasQuorum() const`
- Source: `include/replication/replication_manager.h`:888
- Brief: n/a
- Parameters: none
- Return: true if quorum is achieved; false if partition or insufficient healthy replicas.
- Details: Check if the cluster currently has quorum for write operations. Quorum is defined as: for a cluster with N voting members, at least ceil(N/2)+1 members must be responsive (HEALTHY or DEGRADED status). true if quorum is achieved; false if partition or insufficient healthy replicas. This check is fast (O(1)) as quorum state is maintained continuously.

#### `void healthMonitorLoop()`
- Source: `include/replication/replication_manager.h`:1058
- Brief: Health Monitor Loop.
- Parameters: none
- Details: Calls: load(), performHealthCheck(), isLeader(), getLeaderId(), lock(), empty(), fetch_add(), attemptAutomaticFailover().

#### `void heartbeatLoop()`
- Source: `include/replication/replication_manager.h`:1056
- Brief: Heartbeat Loop.
- Parameters: none
- Details: Calls: load(), isLeader(), getCurrentTerm(), lock(), receiveHeartbeat(), getCurrentSequence(), renewLease(), std::this_thread::sleep_for().

#### `bool initialize()`
- Source: `include/replication/replication_manager.h`:644
- Brief: Initialize.
- Parameters: none
- Return: true on success or if already initialized; false if configuration validation (validateConfig()) fails.
- Details: Initialize replication subsystem and start background threads. Initializes WAL, replicas, leader election state, and background replication threads. If already initialized, returns true immediately (idempotent). true on success or if already initialized; false if configuration validation (validateConfig()) fails. On success, background replication threads are running and this node has joined the configured replica group. True when the operation succeeds. Calls: load(), lock(), validateConfig(), reserve(), size(), std::chrono::system_clock::now(), push_back(), setClusterSize().

#### `LeaseReadResult leaseRead(const std::string &collection, const std::string &document_id) const`
- Source: `include/replication/replication_manager.h`:979
- Brief: n/a
- Parameters:
  - `collection` (const std::string &): Collection name (informational; not used for storage lookup within this module).
  - `document_id` (const std::string &): Document identifier (informational).
- Return: LeaseReadResult describing whether the read was served and how.
- Details: Attempt a linearizable read via the leader lease mechanism. If this node is the current Raft leader and its lease is still valid, the read is served locally with linearizability guarantees (no quorum round-trip required). Otherwise the call returns success=false and the caller should redirect to the primary or use a quorum read. collection Collection name (informational; not used for storage lookup within this module). document_id Document identifier (informational). LeaseReadResult describing whether the read was served and how.

#### `void notifyListeners(std::function< void(IReplicationListener &)> callback)`
- Source: `include/replication/replication_manager.h`:1059
- Brief: n/a
- Parameters:
  - `callback` (std::function< void(IReplicationListener &)>): n/a

#### `void performHealthCheck()`
- Source: `include/replication/replication_manager.h`:900
- Brief: Perform Health Check.
- Parameters: none
- Details: Trigger a health check on all configured replicas. Sends heartbeat/ping messages to all replicas and updates their health status based on responses. Called automatically at heartbeat_interval_ms but can also be called explicitly to force immediate health assessment. Non-blocking; results are available via getReplicaHealthStatus() after heartbeat_interval_ms or on next check call. Calls: lock(), reserve(), size(), updateReplicaHealth(), push_back(), notifyListeners(), onReplicaHealthChanged().

#### `bool promoteReplica(const std::string &replica_id)`
- Source: `include/replication/replication_manager.h`:840
- Brief: Promote Replica.
- Parameters:
  - `replica_id` (const std::string &): Identifier of the replica.
- Return: true on success
- Details: Promote a read replica to primary replica_id Node ID of replica to promote true on success replica_id Identifier of the replica. True when the operation succeeds. Calls: lock(), THEMIS_INFO(), std::find_if(), begin(), end(), THEMIS_ERROR(), getCurrentSequence(), THEMIS_WARN().

#### `bool promoteToLeader()`
- Source: `include/replication/replication_manager.h`:813
- Brief: Promote To Leader.
- Parameters: none
- Return: true on success; false if this node is already the leader, unable to communicate with other replicas, or the quorum is not achieved.
- Details: Promote this follower node to leader (for planned maintenance or deliberate topology change). true on success; false if this node is already the leader, unable to communicate with other replicas, or the quorum is not achieved. Blocking; may take up to election_timeout_max_ms to complete. True when the operation succeeds. Calls: startElection(), isLeader().

#### `void removeReplica(const std::string &node_id)`
- Source: `include/replication/replication_manager.h`:746
- Brief: Remove Replica.
- Parameters:
  - `node_id` (const std::string &): Identifier of the node.
- Details: Remove a replica from the replication group. Existing WAL entries are not deleted; the replica simply stops receiving new replication updates. node_id Unique identifier of the replica to remove. If the removed replica is the current leader, failover is triggered. node_id Identifier of the node. Calls: lock(), erase(), std::remove_if(), begin(), end(), setClusterSize(), size(), notifyListeners().

#### `bool replicate(const WALEntry &entry)`
- Source: `include/replication/replication_manager.h`:672
- Brief: Replicate.
- Parameters:
  - `entry` (const WALEntry &): Input parameter.
- Return: true if entry was appended to WAL; false if this node is not the leader, WAL append failed, or replication is not initialized.
- Details: Replicate a write operation to all configured replicas. Appends the WAL entry to this node's Write-Ahead Log and schedules replication to followers according to the configured replication mode (SYNC, SEMI_SYNC, or ASYNC). entry WAL entry containing operation, document, data, and checksum. true if entry was appended to WAL; false if this node is not the leader, WAL append failed, or replication is not initialized. In ASYNC mode, returns true immediately without waiting for replicas. In SEMI_SYNC mode, waits for min_sync_replicas to acknowledge. In SYNC mode, waits for all voting replicas. entry Input parameter. True when the operation succeeds. Calls: load(), THEMIS_ERROR(), isLeader(), append(), size(), notifyListeners(), onWALEntryApplied(), waitForReplication().

#### `LagBasedReadRouter::RoutingDecision selectReadReplica(std::optional< ReadPreference > preference=std::nullopt) const`
- Source: `include/replication/replication_manager.h`:949
- Brief: n/a
- Parameters:
  - `preference` (std::optional< ReadPreference >): Override read preference (uses configured default when not provided).
- Return: RoutingDecision describing which node was chosen and why.
- Details: Select the best node for a read request using automated lag-based traffic shifting. Replicas whose replication lag exceeds config_.max_replication_lag_ms are excluded from read routing; if none are eligible the primary node is returned. preference Override read preference (uses configured default when not provided). RoutingDecision describing which node was chosen and why.

#### `void setConflictResolver(std::shared_ptr< IConflictResolver > resolver)`
- Source: `include/replication/replication_manager.h`:773
- Brief: Set Conflict Resolver.
- Parameters:
  - `resolver` (std::shared_ptr< IConflictResolver >): Input parameter.
- Details: Set a custom conflict resolver for multi-master replication. The resolver will be called whenever two or more writes conflict on the same document. The resolver must be thread-safe and idempotent. resolver Implementation of IConflictResolver; if nullptr, the default Last-Write-Wins strategy is used. This is only meaningful in multi-master (CRDT) replication mode. In leader-follower mode, write ordering prevents most conflicts. resolver Input parameter. Implements setConflictResolver without additional internal calls.

#### `void setPlacementPolicy(const PlacementConstraints &constraints)`
- Source: `include/replication/replication_manager.h`:1006
- Brief: ============================================================================ Geographic replica placement policies (v1.
- Parameters:
  - `constraints` (const PlacementConstraints &): Input parameter.
- Details: Set the geographic replica placement constraints for leader election and failover candidate selection. When set, the ReplicationManager will use these constraints to guide: selectLeaderCandidate() for initial leader election selectFailoverCandidate() when the current leader fails constraints Placement constraints (preferred/forbidden DCs, zones, etc.) An empty constraints object disables geographic placement policy. include/replication/geo_placement.h — PlacementConstraints constraints Input parameter. 8.0+) ============================================================================ Calls: lock(), THEMIS_INFO(), size().

#### `void setReadPreference(ReadPreference preference)`
- Source: `include/replication/replication_manager.h`:935
- Brief: Set Read Preference.
- Parameters:
  - `preference` (ReadPreference): Input parameter.
- Details: Set the read preference for query routing. preference New read preference strategy. Changes take effect immediately for new read requests. In-flight reads are not affected. preference Input parameter. Implements setReadPreference without additional internal calls.

#### `bool setupCascadingReplication(const std::string &source_replica, const std::vector< std::string > &target_replicas)`
- Source: `include/replication/replication_manager.h`:848
- Brief: Setup Cascading Replication.
- Parameters:
  - `source_replica` (const std::string &): Input parameter.
  - `target_replicas` (const std::vector< std::string > &): Input parameter.
- Return: true on success
- Details: Setup cascading replication (replica replicating to other replicas) source_replica Source replica node ID target_replicas Target replica node IDs true on success source_replica Input parameter. target_replicas Input parameter. True when the operation succeeds. Calls: THEMIS_INFO(), size().

#### `void shutdown()`
- Source: `include/replication/replication_manager.h`:655
- Brief: Shutdown.
- Parameters: none
- Details: Shutdown replication subsystem and wait for background threads to exit. Gracefully stops replication threads, flushes any pending WAL entries, and releases all replica connections. Safe to call from any thread. Blocking; may take up to heartbeat_interval_ms + election_timeout_max_ms to complete in worst case (if leader election is in progress). Calls: load(), store(), timedJoin(), stop(), clear().

#### `bool triggerFailover(const std::string &target_node_id)`
- Source: `include/replication/replication_manager.h`:802
- Brief: Trigger Failover.
- Parameters:
  - `target_node_id` (const std::string &): Identifier of the target node.
- Return: true on success; false if target replica is unreachable or unable to assume leadership.
- Details: Initiate a manual failover to a specific target replica. Attempts to promote the target replica to leader and demote this node (or the current leader) to follower. Useful for maintenance or deliberate topology changes. target_node_id Node ID of the replica to promote. true on success; false if target replica is unreachable or unable to assume leadership. Blocking; may take up to election_timeout_max_ms to complete. If this node is not the current leader, the call is relayed to the leader for execution. target_node_id Identifier of the target node. True when the operation succeeds. Calls: lock(), THEMIS_WARN(), startElection(), isLeader(), notifyListeners(), onFailoverCompleted().

#### `void updateReplicaHealth(ReplicaInfo &replica)`
- Source: `include/replication/replication_manager.h`:1062
- Brief: Update Replica Health.
- Parameters:
  - `replica` (ReplicaInfo &): Input/output parameter.
- Details: replica Input/output parameter. Calls: updateHealthStatus().

#### `bool validateConfig()`
- Source: `include/replication/replication_manager.h`:1055
- Brief: Validate Config.
- Parameters: none
- Return: True when the operation succeeds.
- Details: True when the operation succeeds. Calls: THEMIS_ERROR(), empty(), THEMIS_WARN().

#### `PlacementValidationResult validatePlacementPolicy() const`
- Source: `include/replication/replication_manager.h`:1023
- Brief: n/a
- Parameters: none
- Return: PlacementValidationResult with any violations or recommendations. An empty violations list means the topology satisfies the policy.
- Details: Validate whether the current replica topology satisfies the active geographic placement policy. PlacementValidationResult with any violations or recommendations. An empty violations list means the topology satisfies the policy.

#### `bool waitForReplication(uint64_t sequence, uint32_t timeout_ms=0)`
- Source: `include/replication/replication_manager.h`:689
- Brief: Wait for replication consensus acknowledgment from replica set - SYNC mode: requires all replicas to acknowledge - SEMI_SYNC mode: requires min_sync_replicas to acknowledge (quorum) - ASYNC mode: no waiting (checked in replicate() above) This implements the replicated state machine consensus pattern.
- Parameters:
  - `sequence` (uint64_t): Input parameter.
  - `timeout_ms` (uint32_t): Input parameter.
- Return: true if replication completed within timeout; false on timeout or error.
- Details: Wait for a specific WAL entry to be replicated to a sufficient number of replicas. Blocks until either: The entry (identified by sequence number) has been acknowledged by enough replicas to satisfy the configured replication mode, or timeout_ms milliseconds have elapsed (0 = no timeout). sequence WAL sequence number to wait for. timeout_ms Maximum time to wait in milliseconds; 0 means use the configured replication_timeout_ms from ReplicationConfig. true if replication completed within timeout; false on timeout or error. Unused in ASYNC mode (returns immediately). sequence Input parameter. timeout_ms Input parameter. True when the operation succeeds. Calls: std::chrono::steady_clock::now(), std::chrono::milliseconds(), lock(), size(), THEMIS_ERROR(), getLastAckedSequence(), std::this_thread::sleep_for().

#### `~ReplicationManager()`
- Source: `include/replication/replication_manager.h`:630
- Brief: n/a
- Parameters: none

### themisdb::replication::ReplicationObserver

#### `ReplicationObserver(ReplicationObserver &&) noexcept=default`
- Source: `include/replication/observability.h`:146
- Brief: n/a
- Parameters:
  - `<unnamed>` (ReplicationObserver &&): n/a

#### `ReplicationObserver(const ReplicationObserver &)=delete`
- Source: `include/replication/observability.h`:144
- Brief: n/a
- Parameters:
  - `<unnamed>` (const ReplicationObserver &): n/a

#### `ReplicationObserver(std::shared_ptr< ReplicationManager > manager, const ObserverConfig &config=ObserverConfig{})`
- Source: `include/replication/observability.h`:136
- Brief: n/a
- Parameters:
  - `manager` (std::shared_ptr< ReplicationManager >): n/a
  - `config` (const ObserverConfig &): n/a
- Details: Construct an observer backed by the given ReplicationManager. The observer holds a shared_ptr to the manager so it remains valid for the observer's lifetime.

#### `HealthScore calculateHealthScore() const`
- Source: `include/replication/observability.h`:187
- Brief: n/a
- Parameters: none
- Details: Compute an overall health score for the replication cluster. Scores degrade based on: replica lag, failed health checks, and recent failover events. Performance: O(N), ≤ 5 ms.

#### `std::vector< Bottleneck > detectBottlenecks() const`
- Source: `include/replication/observability.h`:178
- Brief: n/a
- Parameters: none
- Details: Analyse replica lag and health metrics to identify bottlenecks. Returns an empty vector when the cluster is healthy. Performance: O(N), ≤ 5 ms.

#### `std::vector< LagSnapshot > getLagSnapshots(std::chrono::seconds window=std::chrono::seconds(60)) const`
- Source: `include/replication/observability.h`:160
- Brief: n/a
- Parameters:
  - `window` (std::chrono::seconds): n/a
- Details: Return a lag snapshot for every known replica. The window parameter is currently informational; lag is measured at the time of the call from the heartbeat timestamps in ReplicaInfo. Performance: O(N) in the number of replicas, ≤ 1 ms.

#### `std::vector< TopologyNode > getTopology() const`
- Source: `include/replication/observability.h`:170
- Brief: n/a
- Parameters: none
- Details: Return the current replication topology as a list of TopologyNode records, one per known cluster member (including the local node). Performance: O(N), ≤ 1 ms.

#### `ReplicationObserver & operator=(ReplicationObserver &&) noexcept=default`
- Source: `include/replication/observability.h`:147
- Brief: n/a
- Parameters:
  - `<unnamed>` (ReplicationObserver &&): n/a

#### `ReplicationObserver & operator=(const ReplicationObserver &)=delete`
- Source: `include/replication/observability.h`:145
- Brief: n/a
- Parameters:
  - `<unnamed>` (const ReplicationObserver &): n/a

#### `~ReplicationObserver()=default`
- Source: `include/replication/observability.h`:141
- Brief: n/a
- Parameters: none

### themisdb::replication::ReplicationPolicy

#### `ReplicationPolicy(ReplicationPolicy &&) noexcept=default`
- Source: `include/replication/policy.h`:131
- Brief: n/a
- Parameters:
  - `<unnamed>` (ReplicationPolicy &&): n/a

#### `ReplicationPolicy(const ReplicationPolicy &)=delete`
- Source: `include/replication/policy.h`:129
- Brief: n/a
- Parameters:
  - `<unnamed>` (const ReplicationPolicy &): n/a

#### `ReplicationPolicy(std::shared_ptr< ReplicationManager > manager)`
- Source: `include/replication/policy.h`:126
- Brief: n/a
- Parameters:
  - `manager` (std::shared_ptr< ReplicationManager >): n/a
- Details: Construct a ReplicationPolicy manager backed by the given ReplicationManager (used to query the current cluster topology during validation).

#### `bool assignPolicy(const std::string &collection, const std::string &policy_name)`
- Source: `include/replication/policy.h`:156
- Brief: Assign Policy.
- Parameters:
  - `collection` (const std::string &): Input parameter.
  - `policy_name` (const std::string &): Name of the retention policy.
- Return: True when the operation succeeds.
- Details: Assign a previously defined policy to a collection. Overwrites any existing assignment for that collection. Returns false if policy_name was not previously defined. collection Input parameter. policy_name Name of the retention policy. True when the operation succeeds.

#### `Policy defaultPolicy()`
- Source: `include/replication/policy.h`:194
- Brief: n/a
- Parameters: none

#### `void definePolicy(const std::string &policy_name, const Policy &policy)`
- Source: `include/replication/policy.h`:142
- Brief: Define Policy.
- Parameters:
  - `policy_name` (const std::string &): Name of the retention policy.
  - `policy` (const Policy &): Input parameter.
- Details: Define (or replace) a named policy. Thread-safe; policy is visible to subsequent calls immediately. policy_name Name of the retention policy. policy Input parameter.

#### `Policy getPolicy(const std::string &collection) const`
- Source: `include/replication/policy.h`:163
- Brief: n/a
- Parameters:
  - `collection` (const std::string &): n/a
- Details: Get the effective policy for a collection. Returns the default policy when no assignment exists.

#### `std::vector< std::string > listPolicies() const`
- Source: `include/replication/policy.h`:166
- Brief: n/a
- Parameters: none
- Details: List all defined policy names.

#### `ReplicationPolicy & operator=(ReplicationPolicy &&) noexcept=default`
- Source: `include/replication/policy.h`:132
- Brief: n/a
- Parameters:
  - `<unnamed>` (ReplicationPolicy &&): n/a

#### `ReplicationPolicy & operator=(const ReplicationPolicy &)=delete`
- Source: `include/replication/policy.h`:130
- Brief: n/a
- Parameters:
  - `<unnamed>` (const ReplicationPolicy &): n/a

#### `bool removePolicy(const std::string &policy_name)`
- Source: `include/replication/policy.h`:149
- Brief: Remove a retention policy by name.
- Parameters:
  - `policy_name` (const std::string &): Name of the retention policy to remove.
- Return: True when the policy existed and was removed.
- Details: Remove a named policy. Assignments referencing it remain but will return the default policy until reassigned. Returns false if the policy_name did not exist. policy_name Name of the retention policy to remove. True when the policy existed and was removed.

#### `ValidationResult validatePolicy(const Policy &policy) const`
- Source: `include/replication/policy.h`:185
- Brief: n/a
- Parameters:
  - `policy` (const Policy &): n/a
- Return: ValidationResult with is_valid = true when all checks pass.
- Details: Check whether the given policy is currently achievable given the cluster topology reported by the ReplicationManager. Checks performed: Enough healthy replicas to meet desired_replicas. Enough distinct datacenters to meet min_datacenters. required_datacenters each have at least one healthy replica. write_quorum ≤ current healthy replica count. SYNC mode is feasible only when all replicas are healthy. ValidationResult with is_valid = true when all checks pass.

### themisdb::replication::ReplicationSlot

#### `ReplicationSlot(ReplicationSlot &&) noexcept=default`
- Source: `include/replication/replication_slot.h`:140
- Brief: n/a
- Parameters:
  - `<unnamed>` (ReplicationSlot &&): n/a

#### `ReplicationSlot(const ReplicationSlot &)=delete`
- Source: `include/replication/replication_slot.h`:138
- Brief: n/a
- Parameters:
  - `<unnamed>` (const ReplicationSlot &): n/a

#### `ReplicationSlot(const std::string &name, const std::string &plugin_name, const std::string &downstream_node_id, std::shared_ptr< WALManager > wal_manager, const std::string &state_file_path)`
- Source: `include/replication/replication_slot.h`:127
- Brief: n/a
- Parameters:
  - `name` (const std::string &): Unique slot identifier within this node.
  - `plugin_name` (const std::string &): Output plugin (e.g., "physical", "json").
  - `downstream_node_id` (const std::string &): Identifier of the replica or consumer.
  - `wal_manager` (std::shared_ptr< WALManager >): WAL manager used to query sequence ranges.
  - `state_file_path` (const std::string &): Path where slot state is persisted.
- Details: Create a new slot. name Unique slot identifier within this node. plugin_name Output plugin (e.g., "physical", "json"). downstream_node_id Identifier of the replica or consumer. wal_manager WAL manager used to query sequence ranges. state_file_path Path where slot state is persisted.

#### `bool advance(uint64_t confirmed_lsn)`
- Source: `include/replication/replication_slot.h`:178
- Brief: Advance an iterator within the validated range.
- Parameters:
  - `confirmed_lsn` (uint64_t): Input parameter.
- Return: None.
- Details: Advance the confirmed LSN for this slot (called by the consumer on ack). Persists the new LSN to the state file. Returns false when the slot is paused or dropped. confirmed_lsn Input parameter. None.

#### `bool drop()`
- Source: `include/replication/replication_slot.h`:167
- Brief: Drop.
- Parameters: none
- Return: True when the operation succeeds.
- Details: Drop this slot permanently. WAL retention for this slot's LSN range is released. After calling drop() all subsequent control calls return false. True when the operation succeeds.

#### `uint64_t lag() const`
- Source: `include/replication/replication_slot.h`:189
- Brief: n/a
- Parameters: none
- Details: Lag in number of WAL sequences between the leader and this slot.

#### `void loadState()`
- Source: `include/replication/replication_slot.h`:199
- Brief: Load State.
- Parameters: none

#### `const std::string & name() const`
- Source: `include/replication/replication_slot.h`:184
- Brief: n/a
- Parameters: none

#### `ReplicationSlot & operator=(ReplicationSlot &&) noexcept=default`
- Source: `include/replication/replication_slot.h`:141
- Brief: n/a
- Parameters:
  - `<unnamed>` (ReplicationSlot &&): n/a

#### `ReplicationSlot & operator=(const ReplicationSlot &)=delete`
- Source: `include/replication/replication_slot.h`:139
- Brief: n/a
- Parameters:
  - `<unnamed>` (const ReplicationSlot &): n/a

#### `bool pause()`
- Source: `include/replication/replication_slot.h`:154
- Brief: Lock Hierarchy Note: This method acquires state_mutex_ (Level 2), copies state within the lock, then releases the lock before calling persistStateImpl() for blocking I/O.
- Parameters: none
- Return: True when the operation succeeds.
- Details: Pause this slot. WAL segments beyond confirmed_lsn will be retained until the slot is resumed or dropped. Returns false if the slot is already paused or has been dropped. True when the operation succeeds. This ensures lock-free blocking operations and prevents deadlocks.

#### `void persistState() const`
- Source: `include/replication/replication_slot.h`:197
- Brief: n/a
- Parameters: none

#### `void persistStateImpl(const SlotState &state) const`
- Source: `include/replication/replication_slot.h`:198
- Brief: n/a
- Parameters:
  - `state` (const SlotState &): n/a

#### `bool resume()`
- Source: `include/replication/replication_slot.h`:160
- Brief: Resume.
- Parameters: none
- Return: True when the operation succeeds.
- Details: Resume a paused slot. Returns false if the slot is ACTIVE or DROPPED. True when the operation succeeds.

#### `SlotState state() const`
- Source: `include/replication/replication_slot.h`:186
- Brief: n/a
- Parameters: none

#### `SlotStatus status() const`
- Source: `include/replication/replication_slot.h`:185
- Brief: n/a
- Parameters: none

#### `~ReplicationSlot()=default`
- Source: `include/replication/replication_slot.h`:135
- Brief: n/a
- Parameters: none

### themisdb::replication::ReplicationSlotManager

#### `ReplicationSlotManager(const ManagerConfig &config, std::shared_ptr< WALManager > wal_manager)`
- Source: `include/replication/replication_slot.h`:250
- Brief: n/a
- Parameters:
  - `config` (const ManagerConfig &): n/a
  - `wal_manager` (std::shared_ptr< WALManager >): n/a

#### `std::shared_ptr< ReplicationSlot > createSlot(const std::string &name, const std::string &plugin_name="physical", const std::string &downstream_node_id="")`
- Source: `include/replication/replication_slot.h`:263
- Brief: n/a
- Parameters:
  - `name` (const std::string &): n/a
  - `plugin_name` (const std::string &): n/a
  - `downstream_node_id` (const std::string &): n/a
- Details: Create and register a new slot. Returns nullptr if a slot with this name already exists.

#### `bool dropSlot(const std::string &name)`
- Source: `include/replication/replication_slot.h`:279
- Brief: Drop Slot.
- Parameters:
  - `name` (const std::string &): Input parameter.
- Return: True when the operation succeeds.
- Details: Drop and remove a slot by name. Returns false if the slot does not exist. name Input parameter. True when the operation succeeds.

#### `std::shared_ptr< ReplicationSlot > getSlot(const std::string &name) const`
- Source: `include/replication/replication_slot.h`:273
- Brief: n/a
- Parameters:
  - `name` (const std::string &): n/a
- Details: Look up an existing slot by name. Returns nullptr if not found.

#### `std::vector< ReplicationSlot::SlotState > listSlots() const`
- Source: `include/replication/replication_slot.h`:286
- Brief: n/a
- Parameters: none
- Details: List states of all registered slots (snapshot).

#### `void loadPersistedSlots()`
- Source: `include/replication/replication_slot.h`:301
- Brief: Load Persisted Slots.
- Parameters: none
- Details: Reload slot states from disk (call on startup to restore persisted slots).

#### `uint64_t minConfirmedLsn() const`
- Source: `include/replication/replication_slot.h`:296
- Brief: n/a
- Parameters: none
- Details: Minimum confirmed LSN across all active slots. WAL segments before this LSN may be safely purged. Returns 0 if there are no active slots.

#### `size_t slotCount() const`
- Source: `include/replication/replication_slot.h`:289
- Brief: n/a
- Parameters: none
- Details: Number of currently registered (non-dropped) slots.

### themisdb::replication::ReplicationStats

#### `std::string toPrometheusFormat() const`
- Source: `include/replication/replication_manager.h`:322
- Brief: n/a
- Parameters: none

### themisdb::replication::ReplicationStream

#### `ReplicationStream(const std::string &follower_endpoint, std::shared_ptr< WALManager > wal, const ReplicationConfig &config)`
- Source: `include/replication/replication_manager.h`:564
- Brief: n/a
- Parameters:
  - `follower_endpoint` (const std::string &): n/a
  - `wal` (std::shared_ptr< WALManager >): n/a
  - `config` (const ReplicationConfig &): n/a

#### `uint32_t computeBackoffMs() const`
- Source: `include/replication/replication_manager.h`:612
- Brief: n/a
- Parameters: none

#### `uint32_t getConsecutiveFailures() const`
- Source: `include/replication/replication_manager.h`:587
- Brief: n/a
- Parameters: none

#### `const ReplicaInfo & getFollowerInfo() const`
- Source: `include/replication/replication_manager.h`:578
- Brief: n/a
- Parameters: none

#### `uint64_t getLastAckedSequence() const`
- Source: `include/replication/replication_manager.h`:581
- Brief: n/a
- Parameters: none

#### `bool isHealthy() const`
- Source: `include/replication/replication_manager.h`:584
- Brief: n/a
- Parameters: none

#### `bool sendBatch(const std::vector< WALEntry > &entries)`
- Source: `include/replication/replication_manager.h`:611
- Brief: Send Batch.
- Parameters:
  - `entries` (const std::vector< WALEntry > &): Input parameter.
- Return: True when the operation succeeds.
- Details: entries Input parameter. True when the operation succeeds. Implements sendBatch without additional internal calls.

#### `void start()`
- Source: `include/replication/replication_manager.h`:572
- Brief: Start.
- Parameters: none
- Details: Calls: store(), std::thread().

#### `void stop()`
- Source: `include/replication/replication_manager.h`:575
- Brief: Stop.
- Parameters: none
- Details: Calls: store(), notify_all(), joinable(), timedJoin().

#### `void streamLoop()`
- Source: `include/replication/replication_manager.h`:610
- Brief: Stream Loop.
- Parameters: none
- Details: Calls: load(), computeBackoffMs(), lk(), wait_for(), std::chrono::milliseconds(), readFrom(), empty(), sendBatch().

#### `~ReplicationStream()`
- Source: `include/replication/replication_manager.h`:569
- Brief: n/a
- Parameters: none

### themisdb::replication::SchemaAwareCDCBridge

#### `SchemaAwareCDCBridge(std::shared_ptr< ReplicationManager > repl_mgr, const themis::cdc::SchemaRegistryConfig &reg_cfg)`
- Source: `include/replication/schema_cdc.h`:74
- Brief: Construct the bridge.
- Parameters:
  - `repl_mgr` (std::shared_ptr< ReplicationManager >): The replication manager whose CDC events to capture.
  - `reg_cfg` (const themis::cdc::SchemaRegistryConfig &): Schema registry configuration (URL, format, auth …).
- Details: repl_mgr The replication manager whose CDC events to capture. reg_cfg Schema registry configuration (URL, format, auth …).

#### `void deregisterCollection(const std::string &collection)`
- Source: `include/replication/schema_cdc.h`:100
- Brief: Deregister a collection. Future WAL entries for that collection are silently dropped.
- Parameters:
  - `collection` (const std::string &): Input parameter.
- Details: Deregister Collection. collection Input parameter. Calls: lock(), erase().

#### `void dispatch(const SchemaEncodedEvent &ev)`
- Source: `include/replication/schema_cdc.h`:182
- Brief: Dispatch.
- Parameters:
  - `ev` (const SchemaEncodedEvent &): Input parameter.
- Details: ev Input parameter. Calls: lock(), empty(), callback().

#### `Stats getStats() const`
- Source: `include/replication/schema_cdc.h`:137
- Brief: n/a
- Parameters: none

#### `void onConflictDetected(const std::string &) override`
- Source: `include/replication/schema_cdc.h`:146
- Brief: n/a
- Parameters:
  - `<unnamed>` (const std::string &): n/a

#### `void onFailoverCompleted(const std::string &, bool) override`
- Source: `include/replication/schema_cdc.h`:152
- Brief: n/a
- Parameters:
  - `<unnamed>` (const std::string &): n/a
  - `<unnamed>` (bool): n/a

#### `void onFailoverStarted(const std::string &, const std::string &) override`
- Source: `include/replication/schema_cdc.h`:150
- Brief: n/a
- Parameters:
  - `<unnamed>` (const std::string &): n/a
  - `<unnamed>` (const std::string &): n/a

#### `void onLeaderElected(const std::string &) override`
- Source: `include/replication/schema_cdc.h`:143
- Brief: n/a
- Parameters:
  - `<unnamed>` (const std::string &): n/a

#### `void onNetworkPartitionDetected(const std::vector< std::string > &) override`
- Source: `include/replication/schema_cdc.h`:153
- Brief: n/a
- Parameters:
  - `<unnamed>` (const std::vector< std::string > &): n/a

#### `void onReplicaAdded(const ReplicaInfo &) override`
- Source: `include/replication/schema_cdc.h`:144
- Brief: n/a
- Parameters:
  - `<unnamed>` (const ReplicaInfo &): n/a

#### `void onReplicaHealthChanged(const std::string &, HealthStatus, HealthStatus) override`
- Source: `include/replication/schema_cdc.h`:148
- Brief: n/a
- Parameters:
  - `<unnamed>` (const std::string &): n/a
  - `<unnamed>` (HealthStatus): n/a
  - `<unnamed>` (HealthStatus): n/a

#### `void onReplicaRemoved(const std::string &) override`
- Source: `include/replication/schema_cdc.h`:145
- Brief: n/a
- Parameters:
  - `<unnamed>` (const std::string &): n/a

#### `void onReplicationLagWarning(int64_t) override`
- Source: `include/replication/schema_cdc.h`:147
- Brief: n/a
- Parameters:
  - `<unnamed>` (int64_t): n/a

#### `void onRoleChange(ReplicationRole, ReplicationRole) override`
- Source: `include/replication/schema_cdc.h`:142
- Brief: n/a
- Parameters:
  - `<unnamed>` (ReplicationRole): n/a
  - `<unnamed>` (ReplicationRole): n/a

#### `void onWALEntryApplied(const WALEntry &entry) override`
- Source: `include/replication/schema_cdc.h`:141
- Brief: On WALEntry Applied.
- Parameters:
  - `entry` (const WALEntry &): n/a
- Details: wal_entry Input parameter. Calls: lock(), find(), end(), fetch_add(), std::chrono::system_clock::now(), time_since_epoch(), count(), encoder().

#### `uint32_t registerCollection(const std::string &collection, const std::string &schema_def="")`
- Source: `include/replication/schema_cdc.h`:92
- Brief: Register a collection for schema-encoded CDC output.
- Parameters:
  - `collection` (const std::string &): Input parameter.
  - `schema_def` (const std::string &): Input parameter.
- Return: Registered schema ID.
- Details: Register Collection. If schema_def is empty the encoder uses the default Avro/JSON/Protobuf template from CdcSchemaEncoder::defaultAvroSchema() etc. collection Collection name to watch. schema_def Optional explicit schema definition string. Registered schema ID. collection Input parameter. schema_def Input parameter. Return value.

#### `void start()`
- Source: `include/replication/schema_cdc.h`:122
- Brief: Register this bridge as a listener on the replication manager.
- Parameters: none
- Details: Start. Calls: lock(), addListener(), shared_from_this().

#### `void stop()`
- Source: `include/replication/schema_cdc.h`:127
- Brief: Deregister from the replication manager.
- Parameters: none
- Details: Stop. Calls: lock().

#### `uint64_t subscribe(const std::string &collection, EncodedCallback callback)`
- Source: `include/replication/schema_cdc.h`:111
- Brief: Subscribe to encoded CDC events for collection.
- Parameters:
  - `collection` (const std::string &): Input parameter.
  - `callback` (EncodedCallback): Input parameter.
- Return: Subscription ID that can be passed to unsubscribe().
- Details: Subscribe. Use an empty string to subscribe to all registered collections. Subscription ID that can be passed to unsubscribe(). collection Input parameter. callback Input parameter. Return value.

#### `void unsubscribe(uint64_t subscription_id)`
- Source: `include/replication/schema_cdc.h`:115
- Brief: Unsubscribe a previously registered callback.
- Parameters:
  - `subscription_id` (uint64_t): Identifier of the subscription.
- Details: Unsubscribe. subscription_id Identifier of the subscription. Calls: lock(), erase(), std::remove_if(), begin(), end().

#### `~SchemaAwareCDCBridge()`
- Source: `include/replication/schema_cdc.h`:78
- Brief: n/a
- Parameters: none

### themisdb::replication::ThreeWayMergeResolver

#### `ThreeWayMergeResolver()=default`
- Source: `include/replication/conflict_resolution.h`:127
- Brief: n/a
- Parameters: none

#### `std::string mergeJson(const std::string &base, const std::string &left, const std::string &right) const`
- Source: `include/replication/conflict_resolution.h`:145
- Brief: n/a
- Parameters:
  - `base` (const std::string &): n/a
  - `left` (const std::string &): n/a
  - `right` (const std::string &): n/a
- Details: Perform the actual three-way merge of serialised JSON objects. Returns merged JSON string; falls back to left on parse error.

#### `MMWriteEntry resolve(const std::string &document_id, const std::vector< MMWriteEntry > &conflicting_writes, const ResolutionContext &context) override`
- Source: `include/replication/conflict_resolution.h`:129
- Brief: Resolve.
- Parameters:
  - `document_id` (const std::string &): n/a
  - `conflicting_writes` (const std::vector< MMWriteEntry > &): Input parameter.
  - `context` (const ResolutionContext &): n/a
- Return: Return value.
- Details: param Input parameter. conflicting_writes Input parameter. param Input parameter. Return value.

#### `MMWriteEntry selectBase(const std::vector< MMWriteEntry > &writes) const`
- Source: `include/replication/conflict_resolution.h`:139
- Brief: n/a
- Parameters:
  - `writes` (const std::vector< MMWriteEntry > &): n/a
- Details: Select the best common-ancestor candidate from the write set.

#### `std::string strategyName() const override`
- Source: `include/replication/conflict_resolution.h`:135
- Brief: n/a
- Parameters: none
- Details: Human-readable name of this resolver strategy.

### themisdb::replication::VectorClock

#### `VectorClock()=default`
- Source: `include/replication/multi_master_replication.h`:86
- Brief: n/a
- Parameters: none

#### `VectorClock(VectorClock &&other) noexcept`
- Source: `include/replication/multi_master_replication.h`:91
- Brief: n/a
- Parameters:
  - `other` (VectorClock &&): n/a

#### `VectorClock(const VectorClock &other)`
- Source: `include/replication/multi_master_replication.h`:90
- Brief: n/a
- Parameters:
  - `other` (const VectorClock &): n/a

#### `VectorClock(const std::string &node_id)`
- Source: `include/replication/multi_master_replication.h`:87
- Brief: n/a
- Parameters:
  - `node_id` (const std::string &): n/a

#### `int compare(const VectorClock &other) const`
- Source: `include/replication/multi_master_replication.h`:106
- Brief: n/a
- Parameters:
  - `other` (const VectorClock &): n/a

#### `VectorClock fromJson(const std::string &json)`
- Source: `include/replication/multi_master_replication.h`:118
- Brief: From Json.
- Parameters:
  - `json` (const std::string &): Input parameter.
- Return: Return value.
- Details: json Input parameter. Return value. Calls: size(), find(), substr(), std::isdigit(), std::stoull().

#### `uint64_t get(const std::string &node_id) const`
- Source: `include/replication/multi_master_replication.h`:102
- Brief: n/a
- Parameters:
  - `node_id` (const std::string &): n/a

#### `bool happensBefore(const VectorClock &other) const`
- Source: `include/replication/multi_master_replication.h`:109
- Brief: n/a
- Parameters:
  - `other` (const VectorClock &): n/a

#### `void increment(const std::string &node_id)`
- Source: `include/replication/multi_master_replication.h`:96
- Brief: Increment.
- Parameters:
  - `node_id` (const std::string &): Identifier of the node.
- Details: node_id Identifier of the node. Calls: lock().

#### `bool isConcurrent(const VectorClock &other) const`
- Source: `include/replication/multi_master_replication.h`:112
- Brief: n/a
- Parameters:
  - `other` (const VectorClock &): n/a

#### `void merge(const VectorClock &other)`
- Source: `include/replication/multi_master_replication.h`:99
- Brief: Merge.
- Parameters:
  - `other` (const VectorClock &): Input parameter.
- Details: other Input parameter. Calls: lk_lo(), lk_hi().

#### `VectorClock & operator=(VectorClock &&other) noexcept`
- Source: `include/replication/multi_master_replication.h`:93
- Brief: n/a
- Parameters:
  - `other` (VectorClock &&): n/a

#### `VectorClock & operator=(const VectorClock &other)`
- Source: `include/replication/multi_master_replication.h`:92
- Brief: n/a
- Parameters:
  - `other` (const VectorClock &): n/a

#### `std::string toJson() const`
- Source: `include/replication/multi_master_replication.h`:115
- Brief: n/a
- Parameters: none

### themisdb::replication::WALArchivalManager

#### `WALArchivalManager(const ArchivalConfig &config, std::shared_ptr< IArchivalBackend > backend=nullptr)`
- Source: `include/replication/replication_manager.h`:1869
- Brief: n/a
- Parameters:
  - `config` (const ArchivalConfig &): n/a
  - `backend` (std::shared_ptr< IArchivalBackend >): n/a

#### `std::string archivePath(uint64_t segment_id) const`
- Source: `include/replication/replication_manager.h`:1903
- Brief: n/a
- Parameters:
  - `segment_id` (uint64_t): n/a

#### `uint32_t archiveSegments(const std::vector< std::string > &segment_paths)`
- Source: `include/replication/replication_manager.h`:1874
- Brief: Archive Segments.
- Parameters:
  - `segment_paths` (const std::vector< std::string > &): Input parameter.
- Return: Return value.
- Details: segment_paths Input parameter. Return value. Calls: lock(), src(), THEMIS_WARN(), raw(), rfind(), substr(), empty(), std::all_of().

#### `std::vector< uint8_t > compressData(const std::vector< uint8_t > &data)`
- Source: `include/replication/replication_manager.h`:1906
- Brief: n/a
- Parameters:
  - `data` (const std::vector< uint8_t > &): n/a

#### `std::optional< std::vector< uint8_t > > decryptAesGcm(const std::vector< uint8_t > &data, const std::vector< uint8_t > &key)`
- Source: `include/replication/replication_manager.h`:1910
- Brief: n/a
- Parameters:
  - `data` (const std::vector< uint8_t > &): n/a
  - `key` (const std::vector< uint8_t > &): n/a

#### `std::vector< uint8_t > encryptAesGcm(const std::vector< uint8_t > &data, const std::vector< uint8_t > &key)`
- Source: `include/replication/replication_manager.h`:1908
- Brief: n/a
- Parameters:
  - `data` (const std::vector< uint8_t > &): n/a
  - `key` (const std::vector< uint8_t > &): n/a

#### `std::vector< uint8_t > hexToBytes(const std::string &hex)`
- Source: `include/replication/replication_manager.h`:1913
- Brief: n/a
- Parameters:
  - `hex` (const std::string &): n/a

#### `std::vector< ArchivedSegment > listArchived() const`
- Source: `include/replication/replication_manager.h`:1881
- Brief: n/a
- Parameters: none

#### `void loadIndex()`
- Source: `include/replication/replication_manager.h`:1905
- Brief: Load Index.
- Parameters: none
- Details: Calls: f(), std::getline(), empty(), iss(), THEMIS_WARN(), std::chrono::system_clock::time_point(), std::chrono::seconds(), push_back().

#### `uint32_t purgeExpired()`
- Source: `include/replication/replication_manager.h`:1884
- Brief: Purge Expired.
- Parameters: none
- Return: Return value.
- Details: Return value. Calls: std::chrono::system_clock::time_point::max(), std::chrono::system_clock::now(), std::chrono::hours(), lock(), begin(), end(), deleteObject(), std::filesystem::remove().

#### `std::optional< std::vector< uint8_t > > retrieveSegment(uint64_t segment_id) const`
- Source: `include/replication/replication_manager.h`:1878
- Brief: n/a
- Parameters:
  - `segment_id` (uint64_t): n/a

#### `uint32_t runArchivalCycle()`
- Source: `include/replication/replication_manager.h`:1892
- Brief: Run Archival Cycle.
- Parameters: none
- Return: Return value.
- Details: Return value. Calls: std::chrono::steady_clock::now(), std::chrono::milliseconds(), std::chrono::steady_clock::time_point::max(), std::filesystem::exists(), std::filesystem::directory_iterator(), THEMIS_ERROR(), is_regular_file(), push_back().

#### `void saveIndex() const`
- Source: `include/replication/replication_manager.h`:1904
- Brief: n/a
- Parameters: none

#### `uint32_t transitionStorageTiers()`
- Source: `include/replication/replication_manager.h`:1889
- Brief: Transition Storage Tiers.
- Parameters: none
- Return: Return value.
- Details: Return value. Calls: std::chrono::system_clock::now(), std::chrono::hours(), lock(), THEMIS_INFO(), setStorageTier(), saveIndex().

### themisdb::replication::WALEntry

#### `std::optional< WALEntry > deserialize(const std::vector< uint8_t > &data)`
- Source: `include/replication/replication_manager.h`:126
- Brief: Deserialize.
- Parameters:
  - `data` (const std::vector< uint8_t > &): Input parameter.
- Return: Return value.
- Throws:
  - std::out_of_range: if an error occurs.
- Details: data Input parameter. Return value. std::out_of_range if an error occurs. Calls: size(), THEMIS_DEBUG(), THEMIS_ERROR(), s(), begin(), readUint64(), std::chrono::system_clock::time_point(), std::chrono::milliseconds().

#### `std::vector< uint8_t > serialize() const`
- Source: `include/replication/replication_manager.h`:123
- Brief: n/a
- Parameters: none

### themisdb::replication::WALManager

#### `WALManager(const ReplicationConfig &config)`
- Source: `include/replication/replication_manager.h`:408
- Brief: n/a
- Parameters:
  - `config` (const ReplicationConfig &): n/a

#### `uint64_t append(const WALEntry &entry)`
- Source: `include/replication/replication_manager.h`:412
- Brief: Append.
- Parameters:
  - `entry` (const WALEntry &): Input parameter.
- Return: Return value.
- Details: entry Input parameter. Return value. Calls: lock(), load(), empty(), SHA256(), c_str(), size(), std::setw(), std::setfill().

#### `uint64_t getCurrentSequence() const`
- Source: `include/replication/replication_manager.h`:418
- Brief: n/a
- Parameters: none

#### `uint64_t getCurrentTerm() const`
- Source: `include/replication/replication_manager.h`:421
- Brief: n/a
- Parameters: none

#### `std::mutex & getMutex()`
- Source: `include/replication/replication_manager.h`:437
- Brief: n/a
- Parameters: none

#### `uint64_t getSize() const`
- Source: `include/replication/replication_manager.h`:433
- Brief: n/a
- Parameters: none

#### `uint64_t incrementTerm()`
- Source: `include/replication/replication_manager.h`:424
- Brief: Increment Term.
- Parameters: none
- Return: Return value.
- Details: Return value. Implements incrementTerm without additional internal calls.

#### `void loadFromDisk()`
- Source: `include/replication/replication_manager.h`:447
- Brief: Load From Disk.
- Parameters: none
- Details: Calls: std::filesystem::directory_iterator(), path(), extension(), ifs(), read(), eof(), data(), WALEntry::deserialize().

#### `std::vector< WALEntry > readFrom(uint64_t start_sequence, uint32_t limit=1000)`
- Source: `include/replication/replication_manager.h`:415
- Brief: Read From.
- Parameters:
  - `start_sequence` (uint64_t): Input parameter.
  - `limit` (uint32_t): Input parameter.
- Return: Return value.
- Details: start_sequence Input parameter. limit Input parameter. Return value. Calls: lock(), reserve(), size(), std::to_string(), std::filesystem::exists(), ifs(), THEMIS_ERROR(), read().

#### `void rotateSegment()`
- Source: `include/replication/replication_manager.h`:446
- Brief: n/a
- Parameters: none

#### `void sync()`
- Source: `include/replication/replication_manager.h`:430
- Brief: Sync.
- Parameters: none
- Details: Calls: dir(), empty(), fs::exists(), THEMIS_WARN(), message(), fs::directory_iterator(), path(), extension().

#### `void truncateBefore(uint64_t sequence)`
- Source: `include/replication/replication_manager.h`:427
- Brief: Truncate Before.
- Parameters:
  - `sequence` (uint64_t): Input parameter.
- Details: sequence Input parameter. Calls: lock(), std::to_string(), std::filesystem::remove().

#### `~WALManager()`
- Source: `include/replication/replication_manager.h`:409
- Brief: n/a
- Parameters: none

### themisdb::replication::crdt::DisableWinsFlag

#### `void disable()`
- Source: `include/replication/crdt_types.h`:667
- Brief: n/a
- Parameters: none

#### `void enable()`
- Source: `include/replication/crdt_types.h`:666
- Brief: n/a
- Parameters: none

#### `void merge(const DisableWinsFlag &other)`
- Source: `include/replication/crdt_types.h`:674
- Brief: n/a
- Parameters:
  - `other` (const DisableWinsFlag &): n/a

#### `bool value() const`
- Source: `include/replication/crdt_types.h`:669
- Brief: n/a
- Parameters: none

### themisdb::replication::crdt::Dot

#### `bool operator<(const Dot &o) const noexcept`
- Source: `include/replication/crdt_types.h`:40
- Brief: n/a
- Parameters:
  - `o` (const Dot &): n/a

#### `bool operator==(const Dot &o) const noexcept`
- Source: `include/replication/crdt_types.h`:37
- Brief: n/a
- Parameters:
  - `o` (const Dot &): n/a

### themisdb::replication::crdt::EnableWinsFlag

#### `void disable()`
- Source: `include/replication/crdt_types.h`:630
- Brief: n/a
- Parameters: none

#### `void enable()`
- Source: `include/replication/crdt_types.h`:629
- Brief: n/a
- Parameters: none

#### `void merge(const EnableWinsFlag &other)`
- Source: `include/replication/crdt_types.h`:637
- Brief: n/a
- Parameters:
  - `other` (const EnableWinsFlag &): n/a

#### `bool value() const`
- Source: `include/replication/crdt_types.h`:632
- Brief: n/a
- Parameters: none

### themisdb::replication::crdt::GrowOnlyCounter

#### `GrowOnlyCounter(const std::string &node_id)`
- Source: `include/replication/crdt_types.h`:61
- Brief: n/a
- Parameters:
  - `node_id` (const std::string &): n/a

#### `void increment(uint64_t delta=1)`
- Source: `include/replication/crdt_types.h`:65
- Brief: Increment the local node's count by delta (default 1).
- Parameters:
  - `delta` (uint64_t): n/a

#### `void merge(const GrowOnlyCounter &other)`
- Source: `include/replication/crdt_types.h`:77
- Brief: Merge another G-Counter state into this one (take per-node max).
- Parameters:
  - `other` (const GrowOnlyCounter &): n/a

#### `const std::unordered_map< std::string, uint64_t > & state() const`
- Source: `include/replication/crdt_types.h`:87
- Brief: Raw per-node state (for serialisation / testing).
- Parameters: none

#### `uint64_t value() const`
- Source: `include/replication/crdt_types.h`:70
- Brief: Returns the total counter value across all nodes.
- Parameters: none

### themisdb::replication::crdt::GrowOnlySet

#### `void add(const T &element)`
- Source: `include/replication/crdt_types.h`:257
- Brief: n/a
- Parameters:
  - `element` (const T &): n/a

#### `bool contains(const T &element) const`
- Source: `include/replication/crdt_types.h`:259
- Brief: n/a
- Parameters:
  - `element` (const T &): n/a

#### `const std::set< T > & elements() const`
- Source: `include/replication/crdt_types.h`:263
- Brief: n/a
- Parameters: none

#### `void merge(const GrowOnlySet< T > &other)`
- Source: `include/replication/crdt_types.h`:265
- Brief: n/a
- Parameters:
  - `other` (const GrowOnlySet< T > &): n/a

### themisdb::replication::crdt::LWWMap

#### `LWWMap(const std::string &node_id)`
- Source: `include/replication/crdt_types.h`:434
- Brief: n/a
- Parameters:
  - `node_id` (const std::string &): n/a

#### `bool contains(const K &key) const`
- Source: `include/replication/crdt_types.h`:467
- Brief: n/a
- Parameters:
  - `key` (const K &): n/a

#### `std::optional< V > get(const K &key) const`
- Source: `include/replication/crdt_types.h`:461
- Brief: Returns the value for key, or empty optional if absent/removed.
- Parameters:
  - `key` (const K &): n/a

#### `void merge(const LWWMap< K, V > &other)`
- Source: `include/replication/crdt_types.h`:472
- Brief: n/a
- Parameters:
  - `other` (const LWWMap< K, V > &): n/a

#### `void put(const K &key, V value, uint64_t timestamp_us)`
- Source: `include/replication/crdt_types.h`:437
- Brief: Set key to value with timestamp_us.
- Parameters:
  - `key` (const K &): n/a
  - `value` (V): n/a
  - `timestamp_us` (uint64_t): n/a

#### `void remove(const K &key, uint64_t timestamp_us)`
- Source: `include/replication/crdt_types.h`:449
- Brief: Mark key as removed with timestamp_us.
- Parameters:
  - `key` (const K &): n/a
  - `timestamp_us` (uint64_t): n/a

### themisdb::replication::crdt::LWWRegister

#### `LWWRegister()=default`
- Source: `include/replication/crdt_types.h`:145
- Brief: n/a
- Parameters: none

#### `LWWRegister(const std::string &node_id)`
- Source: `include/replication/crdt_types.h`:146
- Brief: n/a
- Parameters:
  - `node_id` (const std::string &): n/a

#### `void merge(const LWWRegister< T > &other)`
- Source: `include/replication/crdt_types.h`:167
- Brief: n/a
- Parameters:
  - `other` (const LWWRegister< T > &): n/a

#### `const std::optional< T > & read() const`
- Source: `include/replication/crdt_types.h`:162
- Brief: Returns the current value (empty optional if never written).
- Parameters: none

#### `uint64_t timestamp() const`
- Source: `include/replication/crdt_types.h`:165
- Brief: Returns the timestamp of the winning write.
- Parameters: none

#### `void write(T value, uint64_t timestamp_us)`
- Source: `include/replication/crdt_types.h`:152
- Brief: Write value with the given timestamp_us (microseconds since epoch).
- Parameters:
  - `value` (T): n/a
  - `timestamp_us` (uint64_t): n/a

### themisdb::replication::crdt::MVRegister

#### `void merge(const MVRegister< T > &other)`
- Source: `include/replication/crdt_types.h`:229
- Brief: Merge another MVRegister into this one (union of entries).
- Parameters:
  - `other` (const MVRegister< T > &): n/a

#### `std::vector< T > read() const`
- Source: `include/replication/crdt_types.h`:218
- Brief: Returns all concurrent values currently in the register.
- Parameters: none

#### `void write(T value, const Dot &dot)`
- Source: `include/replication/crdt_types.h`:210
- Brief: Write value tagged with dot.
- Parameters:
  - `value` (T): n/a
  - `dot` (const Dot &): n/a
- Details: Clears all previous values (simulating causally-dominating write).

### themisdb::replication::crdt::ORSet

#### `ORSet(const std::string &node_id)`
- Source: `include/replication/crdt_types.h`:330
- Brief: n/a
- Parameters:
  - `node_id` (const std::string &): n/a

#### `void add(const T &element)`
- Source: `include/replication/crdt_types.h`:334
- Brief: Add element, generating a fresh dot.
- Parameters:
  - `element` (const T &): n/a

#### `bool contains(const T &element) const`
- Source: `include/replication/crdt_types.h`:354
- Brief: n/a
- Parameters:
  - `element` (const T &): n/a

#### `std::set< T > elements() const`
- Source: `include/replication/crdt_types.h`:367
- Brief: n/a
- Parameters: none

#### `void merge(const ORSet< T > &other)`
- Source: `include/replication/crdt_types.h`:378
- Brief: n/a
- Parameters:
  - `other` (const ORSet< T > &): n/a

#### `void remove(const T &element)`
- Source: `include/replication/crdt_types.h`:343
- Brief: Remove element by tombstoning all currently-observed dots.
- Parameters:
  - `element` (const T &): n/a

### themisdb::replication::crdt::PNCounter

#### `PNCounter(const std::string &node_id)`
- Source: `include/replication/crdt_types.h`:108
- Brief: n/a
- Parameters:
  - `node_id` (const std::string &): n/a

#### `void decrement(uint64_t delta=1)`
- Source: `include/replication/crdt_types.h`:112
- Brief: n/a
- Parameters:
  - `delta` (uint64_t): n/a

#### `void increment(uint64_t delta=1)`
- Source: `include/replication/crdt_types.h`:111
- Brief: n/a
- Parameters:
  - `delta` (uint64_t): n/a

#### `void merge(const PNCounter &other)`
- Source: `include/replication/crdt_types.h`:120
- Brief: n/a
- Parameters:
  - `other` (const PNCounter &): n/a

#### `int64_t value() const`
- Source: `include/replication/crdt_types.h`:115
- Brief: Returns the signed counter value (may be negative).
- Parameters: none

### themisdb::replication::crdt::RGArray

#### `RGArray(const std::string &node_id)`
- Source: `include/replication/crdt_types.h`:513
- Brief: n/a
- Parameters:
  - `node_id` (const std::string &): n/a

#### `Dot append(T value)`
- Source: `include/replication/crdt_types.h`:540
- Brief: Append value at the end of the sequence.
- Parameters:
  - `value` (T): n/a

#### `It findDot(const Dot &dot)`
- Source: `include/replication/crdt_types.h`:603
- Brief: n/a
- Parameters:
  - `dot` (const Dot &): n/a

#### `Dot insertAfter(const Dot &after, T value)`
- Source: `include/replication/crdt_types.h`:521
- Brief: Insert value after the element with Dot after.
- Parameters:
  - `after` (const Dot &): n/a
  - `value` (T): n/a
- Details: Use an empty-string Dot{} to insert at the beginning.

#### `void merge(const RGArray< T > &other)`
- Source: `include/replication/crdt_types.h`:569
- Brief: Merge another RGArray state (union of entries, order by dot).
- Parameters:
  - `other` (const RGArray< T > &): n/a

#### `std::vector< T > read() const`
- Source: `include/replication/crdt_types.h`:555
- Brief: Returns all live (non-tombstoned) elements in order.
- Parameters: none

#### `void remove(const Dot &dot)`
- Source: `include/replication/crdt_types.h`:547
- Brief: Delete the element identified by dot.
- Parameters:
  - `dot` (const Dot &): n/a

#### `size_t size() const`
- Source: `include/replication/crdt_types.h`:584
- Brief: n/a
- Parameters: none

### themisdb::replication::crdt::TwoPSet

#### `void add(const T &element)`
- Source: `include/replication/crdt_types.h`:289
- Brief: n/a
- Parameters:
  - `element` (const T &): n/a

#### `bool contains(const T &element) const`
- Source: `include/replication/crdt_types.h`:299
- Brief: n/a
- Parameters:
  - `element` (const T &): n/a

#### `void merge(const TwoPSet< T > &other)`
- Source: `include/replication/crdt_types.h`:303
- Brief: n/a
- Parameters:
  - `other` (const TwoPSet< T > &): n/a

#### `void remove(const T &element)`
- Source: `include/replication/crdt_types.h`:295
- Brief: Remove element (tombstone). Has no effect if the element was never added.
- Parameters:
  - `element` (const T &): n/a

### themisdb::replication::test

#### `TEST(ReplicationFailClosedContract, AcceptsWriteWhenWalAppendSucceeds)`
- Source: `tests/replication/test_replication_fail_closed_behavior.cpp`:33
- Brief: n/a
- Parameters:
  - `<unnamed>` (ReplicationFailClosedContract): n/a
  - `<unnamed>` (AcceptsWriteWhenWalAppendSucceeds): n/a

#### `TEST(ReplicationFailClosedContract, RejectsWriteWhenWalAppendFails)`
- Source: `tests/replication/test_replication_fail_closed_behavior.cpp`:26
- Brief: n/a
- Parameters:
  - `<unnamed>` (ReplicationFailClosedContract): n/a
  - `<unnamed>` (RejectsWriteWhenWalAppendFails): n/a

#### `TEST_F(ConflictContextTest, EmptyUserRolesHandled)`
- Source: `tests/replication/test_replication_conflict_focused.cpp`:326
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConflictContextTest): n/a
  - `<unnamed>` (EmptyUserRolesHandled): n/a
- Details: RCS-03.2: Empty user roles are handled correctly.

#### `TEST_F(ConflictContextTest, ResolutionContextRespected)`
- Source: `tests/replication/test_replication_conflict_focused.cpp`:303
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConflictContextTest): n/a
  - `<unnamed>` (ResolutionContextRespected): n/a
- Details: RCS-03.1: Resolution context with metadata and roles is respected.

#### `TEST_F(DeterministicConflictResolutionTest, IdempotentResolution)`
- Source: `tests/replication/test_replication_conflict_focused.cpp`:381
- Brief: n/a
- Parameters:
  - `<unnamed>` (DeterministicConflictResolutionTest): n/a
  - `<unnamed>` (IdempotentResolution): n/a
- Details: RCS-04.1: Same input always produces same winner (idempotence).

#### `TEST_F(DeterministicConflictResolutionTest, SequentialResolutionStateIntegrity)`
- Source: `tests/replication/test_replication_conflict_focused.cpp`:395
- Brief: n/a
- Parameters:
  - `<unnamed>` (DeterministicConflictResolutionTest): n/a
  - `<unnamed>` (SequentialResolutionStateIntegrity): n/a
- Details: RCS-04.2: Multiple sequential calls don't corrupt resolver state.

#### `TEST_F(DiagnosticsConsistencyTest, DiagnosticMetadataAvailable)`
- Source: `tests/replication/test_replication_conflict_focused.cpp`:555
- Brief: n/a
- Parameters:
  - `<unnamed>` (DiagnosticsConsistencyTest): n/a
  - `<unnamed>` (DiagnosticMetadataAvailable): n/a
- Details: RCS-06.2: Resolution diagnostics include metadata about decision path.

#### `TEST_F(DiagnosticsConsistencyTest, StrategyNamesConsistent)`
- Source: `tests/replication/test_replication_conflict_focused.cpp`:539
- Brief: n/a
- Parameters:
  - `<unnamed>` (DiagnosticsConsistencyTest): n/a
  - `<unnamed>` (StrategyNamesConsistent): n/a
- Details: RCS-06.1: Strategy names are consistent and identifiable.

#### `TEST_F(EdgeCaseConflictResolutionTest, LargeConflictSetDeterministic)`
- Source: `tests/replication/test_replication_conflict_focused.cpp`:477
- Brief: n/a
- Parameters:
  - `<unnamed>` (EdgeCaseConflictResolutionTest): n/a
  - `<unnamed>` (LargeConflictSetDeterministic): n/a
- Details: RCS-05.2: Large conflict set resolves deterministically.

#### `TEST_F(EdgeCaseConflictResolutionTest, MinimalConflictSetResolved)`
- Source: `tests/replication/test_replication_conflict_focused.cpp`:454
- Brief: n/a
- Parameters:
  - `<unnamed>` (EdgeCaseConflictResolutionTest): n/a
  - `<unnamed>` (MinimalConflictSetResolved): n/a
- Details: RCS-05.1: Minimum conflict set (2 writes) resolves correctly.

#### `TEST_F(FieldLevelMergeResolverTest, IntersectStrategyOnlyCommonFields)`
- Source: `tests/replication/test_replication_conflict_focused.cpp`:183
- Brief: n/a
- Parameters:
  - `<unnamed>` (FieldLevelMergeResolverTest): n/a
  - `<unnamed>` (IntersectStrategyOnlyCommonFields): n/a
- Details: RCS-02.2: INTERSECT strategy includes only common fields.

#### `TEST_F(FieldLevelMergeResolverTest, LeftBiasPreferFirstEntry)`
- Source: `tests/replication/test_replication_conflict_focused.cpp`:204
- Brief: n/a
- Parameters:
  - `<unnamed>` (FieldLevelMergeResolverTest): n/a
  - `<unnamed>` (LeftBiasPreferFirstEntry): n/a
- Details: RCS-02.3: LEFT_BIAS strategy resolves without error and returns valid winner. Note: LEFT_BIAS applies at field-value level for conflicting fields; the base winner is always the latest-HLC write. This test verifies the resolver runs cleanly and returns a valid entry, not a specific write_id.

#### `TEST_F(FieldLevelMergeResolverTest, RightBiasPreferLastEntry)`
- Source: `tests/replication/test_replication_conflict_focused.cpp`:227
- Brief: n/a
- Parameters:
  - `<unnamed>` (FieldLevelMergeResolverTest): n/a
  - `<unnamed>` (RightBiasPreferLastEntry): n/a
- Details: RCS-02.4: RIGHT_BIAS strategy resolves without error and returns valid winner. Note: RIGHT_BIAS applies at field-value level for conflicting fields; the base winner is always the latest-HLC write. This test verifies the resolver runs cleanly and returns a valid entry, not a specific write_id.

#### `TEST_F(FieldLevelMergeResolverTest, StrategyNamesCorrect)`
- Source: `tests/replication/test_replication_conflict_focused.cpp`:246
- Brief: n/a
- Parameters:
  - `<unnamed>` (FieldLevelMergeResolverTest): n/a
  - `<unnamed>` (StrategyNamesCorrect): n/a
- Details: RCS-02.5: Strategy names are correct and deterministic.

#### `TEST_F(FieldLevelMergeResolverTest, UnionStrategyIncludesAllFields)`
- Source: `tests/replication/test_replication_conflict_focused.cpp`:166
- Brief: n/a
- Parameters:
  - `<unnamed>` (FieldLevelMergeResolverTest): n/a
  - `<unnamed>` (UnionStrategyIncludesAllFields): n/a
- Details: RCS-02.1: UNION strategy includes all fields.

#### `TEST_F(ThreeWayMergeResolverTest, EmptyConflictSetFailsClosed)`
- Source: `tests/replication/test_replication_conflict_focused.cpp`:103
- Brief: n/a
- Parameters:
  - `<unnamed>` (ThreeWayMergeResolverTest): n/a
  - `<unnamed>` (EmptyConflictSetFailsClosed): n/a
- Details: RCS-01.3: Empty conflict set fails closed.

#### `TEST_F(ThreeWayMergeResolverTest, MultipleWritesResolved)`
- Source: `tests/replication/test_replication_conflict_focused.cpp`:86
- Brief: n/a
- Parameters:
  - `<unnamed>` (ThreeWayMergeResolverTest): n/a
  - `<unnamed>` (MultipleWritesResolved): n/a
- Details: RCS-01.2: Multiple writes are resolved to one winner.

#### `TEST_F(ThreeWayMergeResolverTest, SingleWriteReturnedUnchanged)`
- Source: `tests/replication/test_replication_conflict_focused.cpp`:72
- Brief: n/a
- Parameters:
  - `<unnamed>` (ThreeWayMergeResolverTest): n/a
  - `<unnamed>` (SingleWriteReturnedUnchanged): n/a
- Details: RCS-01.1: Single write in conflict set is returned unchanged.

#### `TEST_F(ThreeWayMergeResolverTest, StrategyNameConsistent)`
- Source: `tests/replication/test_replication_conflict_focused.cpp`:116
- Brief: n/a
- Parameters:
  - `<unnamed>` (ThreeWayMergeResolverTest): n/a
  - `<unnamed>` (StrategyNameConsistent): n/a
- Details: RCS-01.4: Strategy name is consistent.

### themisdb::replication::test::ConflictContextTest

#### `void SetUp() override`
- Source: `tests/replication/test_replication_conflict_focused.cpp`:280
- Brief: n/a
- Parameters: none

#### `MMWriteEntry createTestWrite(const std::string &write_id, const std::string &doc_id, const std::string &collection)`
- Source: `tests/replication/test_replication_conflict_focused.cpp`:284
- Brief: n/a
- Parameters:
  - `write_id` (const std::string &): n/a
  - `doc_id` (const std::string &): n/a
  - `collection` (const std::string &): n/a

### themisdb::replication::test::DeterministicConflictResolutionTest

#### `void SetUp() override`
- Source: `tests/replication/test_replication_conflict_focused.cpp`:356
- Brief: n/a
- Parameters: none

#### `MMWriteEntry createTestWrite(const std::string &write_id, const std::string &doc_id, const std::string &collection)`
- Source: `tests/replication/test_replication_conflict_focused.cpp`:362
- Brief: n/a
- Parameters:
  - `write_id` (const std::string &): n/a
  - `doc_id` (const std::string &): n/a
  - `collection` (const std::string &): n/a

### themisdb::replication::test::DiagnosticsConsistencyTest

#### `void SetUp() override`
- Source: `tests/replication/test_replication_conflict_focused.cpp`:513
- Brief: n/a
- Parameters: none

#### `MMWriteEntry createTestWrite(const std::string &write_id, const std::string &doc_id, const std::string &collection)`
- Source: `tests/replication/test_replication_conflict_focused.cpp`:520
- Brief: n/a
- Parameters:
  - `write_id` (const std::string &): n/a
  - `doc_id` (const std::string &): n/a
  - `collection` (const std::string &): n/a

### themisdb::replication::test::EdgeCaseConflictResolutionTest

#### `void SetUp() override`
- Source: `tests/replication/test_replication_conflict_focused.cpp`:429
- Brief: n/a
- Parameters: none

#### `MMWriteEntry createTestWrite(const std::string &write_id, const std::string &doc_id, const std::string &collection)`
- Source: `tests/replication/test_replication_conflict_focused.cpp`:435
- Brief: n/a
- Parameters:
  - `write_id` (const std::string &): n/a
  - `doc_id` (const std::string &): n/a
  - `collection` (const std::string &): n/a

### themisdb::replication::test::FieldLevelMergeResolverTest

#### `void SetUp() override`
- Source: `tests/replication/test_replication_conflict_focused.cpp`:138
- Brief: n/a
- Parameters: none

#### `MMWriteEntry createTestWrite(const std::string &write_id, const std::string &doc_id, const std::string &collection)`
- Source: `tests/replication/test_replication_conflict_focused.cpp`:146
- Brief: n/a
- Parameters:
  - `write_id` (const std::string &): n/a
  - `doc_id` (const std::string &): n/a
  - `collection` (const std::string &): n/a

### themisdb::replication::test::StubWAL

#### `bool append(const std::string &entry)`
- Source: `tests/replication/test_replication_fail_closed_behavior.cpp`:17
- Brief: n/a
- Parameters:
  - `entry` (const std::string &): n/a

### themisdb::replication::test::ThreeWayMergeResolverTest

#### `void SetUp() override`
- Source: `tests/replication/test_replication_conflict_focused.cpp`:39
- Brief: n/a
- Parameters: none

#### `MMWriteEntry createTestWrite(const std::string &write_id, const std::string &doc_id, const std::string &collection, const std::string &operation="WRITE")`
- Source: `tests/replication/test_replication_conflict_focused.cpp`:51
- Brief: n/a
- Parameters:
  - `write_id` (const std::string &): n/a
  - `doc_id` (const std::string &): n/a
  - `collection` (const std::string &): n/a
  - `operation` (const std::string &): n/a
- Details: Helper to create a test MMWriteEntry.

