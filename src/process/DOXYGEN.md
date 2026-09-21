# PROCESS DOXYGEN

Author: ThemisDB Contributors
Created: 2026-09-20
Last Updated: 2026-09-20
Status: active

## Source
- This file is generated from module-scoped Doxygen XML.
- XML index: `C:\Projects\ThemisDB\.tmp\module_doxygen_global_e2e\process\xml\index.xml`
- Warnings log: `C:\Projects\ThemisDB\.tmp\module_doxygen_global_e2e\process\doxygen-warnings.log`

## Structure Metrics
- C/C++ Files (scanned): 105
- Compounds: 432
- Classes/Structs: 295
- Namespaces: 23
- File Compounds: 105

## Namespaces
- @121000070047173024116253346226370370174022111133
- benchmark
- testing
- themis
- themis::observability
- themis::plugins
- themis::process
- themis::process::@045142215342152137151157226322323360206336064142
- themis::process::@052334046007344144131353102223047345153122313251
- themis::process::@060010226313366341241335206040232266126170022036
- themis::process::@062131313221143340177314215130273073127130100131
- themis::process::@202227203165341211275015114217313057177153136147
- themis::process::@277375122307341351111005074371343223204014233015
- themis::process::@310336202046306144247213161275161315277147331250
- themis::process::@323336153047151046357215210142300330303303374223
- themis::process::@324110172174170360105150334004344366077301322074
- themis::process::@364244260117115300334262004274137230300341177003
- themis::process::benchmark
- themis::process::test
- themisdb
- themisdb::analytics
- themisdb::process
- themisdb::process::stress_scenarios

## Types
### Classes
- BenchStubProcessManager
- BenchStubSignalBus
- BottleneckAnalyticsTest
- BpmnSerializerTest
- ConcurrencyChurnTest
- DeterminismConflictTest
- DiagnosticsIncidentTest
- DmnEvaluatorTest
- EdgeTypeRegistryTest
- EpkSerializerTest
- LinkerEdgeTest
- LlmDescriptorTest
- ObjectCentricTracerTest
- OcelExporterTest
- ParserEdgeTest
- PprTest
- ProcessCommunityDetectorTest
- ProcessGraphRagTest
- ProcessGraphTest
- ProcessHighCardinalityStress
- ProcessLightRetrieverTest
- ProcessManagerFixture
- ProcessMiningTest
- ProcessModelGeneratorTest
- ProcessModelHardeningTest
- ProcessModuleTest
- RetrieverEdgeTest
- RetrieverResilienceTest
- RetrieverResilienceTest::BoundedCache
- SlaMonitoringTest
- StressChurnTest
- StubProcessManager
- StubSignalBus
- VccVpbImporterTest
- themis::process::ApplicationCustomStrategy
- themis::process::ArisXmlManagerTest
- themis::process::BpmnSerializer
- themis::process::BpmnValidator
- themis::process::CmmnSerializer
- themis::process::CmmnValidator
- themis::process::ConflictResolutionRegistry
- themis::process::ConflictResolverManager
- themis::process::DeltaPatchCodec
- themis::process::DiagnosticContext
- themis::process::DiagnosticMetricsCollector
- themis::process::DiagnosticRecord
- themis::process::DistributedSpan
- themis::process::DmnEvaluator
- themis::process::DmnEvaluatorThreadSafetyTest
- themis::process::DmnValidator
- themis::process::EpkArisXmlImporter
- themis::process::EpkSerializer
- themis::process::EpkValidator
- themis::process::EpochManager
- themis::process::ExtendedDiagnosticRecord
- themis::process::ExtendedProcessDiagnostics
- themis::process::FederationConsensusManager
- themis::process::FederationConsensusManagerImpl
- themis::process::FederationReplicaManager
- themis::process::FederationReplicaManagerImpl
- themis::process::FimImporter
- themis::process::FirstWriteWinsResolver
- themis::process::FirstWriteWinsStrategy
- themis::process::IConflictResolutionPlugin
- themis::process::IConflictResolutionStrategy
- themis::process::ILLMProcessAdapter
- themis::process::IProcessTracer
- themis::process::ISpan
- themis::process::IXpdlImporter
- themis::process::LastWriteWinsResolver
- themis::process::LastWriteWinsStrategy
- themis::process::LlmProcessDescriptor
- themis::process::LockFreeBatchQueue
- themis::process::LockFreeHashTable
- themis::process::ObjectCentricTracer
- themis::process::OcelExporter
- themis::process::ParserStateTracker
- themis::process::ProcessAgenticRag
- themis::process::ProcessAuditLogger
- themis::process::ProcessAuditLoggerImpl
- themis::process::ProcessCommunityDetector
- themis::process::ProcessConflictResolver
- themis::process::ProcessConflictResolverCallback
- themis::process::ProcessConflictResolverImpl
- themis::process::ProcessDiagnostics
- themis::process::ProcessGraphRag
- themis::process::ProcessLightRetriever
- themis::process::ProcessLinker
- themis::process::ProcessLinker::LinkOperationGuard
- themis::process::ProcessModelGenerator
- themis::process::ProcessModelManager
- themis::process::ProcessModelManager::TransactionGuard
- themis::process::ProcessTelemetryIntegration
- themis::process::ProcessTelemetryIntegrationImpl
- themis::process::SerializerInputValidator
- themis::process::VccVpbImporter
- themis::process::VccVpbImporterResourceTest
- themis::process::benchmark::BaselineIncidentClassifier
- themis::process::benchmark::BpmnParser
- themis::process::benchmark::CmmnParser
- themis::process::benchmark::CommunityDetector
- themis::process::benchmark::ConcurrentModelStore
- themis::process::benchmark::ConflictResolver
- themis::process::benchmark::ConformanceChecker
- themis::process::benchmark::DmnParser
- themis::process::benchmark::EnhancedIncidentClassifier
- themis::process::benchmark::EpkParser
- themis::process::benchmark::FimParser
- themis::process::benchmark::LlmProcessDescriptor
- themis::process::benchmark::OcelParser
- themis::process::benchmark::ProcessLinker
- themis::process::benchmark::ProcessMiningEngine
- themis::process::benchmark::ProcessModelRetriever
- themis::process::benchmark::RevisionStore
- themis::process::benchmark::TransactionSerializer
- themis::process::benchmark::VariantAnalyzer
- themis::process::benchmark::VccVpbParser
- themis::process::test::ProcessAgenticRagTest
- themis::process::test::VccVpbImporterTest
- themisdb::process::FederationConsensusManager
- themisdb::process::ProcessAuditLogger
- themisdb::process::stress_scenarios::AuditTrailHighChurnScenario
- themisdb::process::stress_scenarios::ByzantineConsensusScenario
- themisdb::process::stress_scenarios::CallbackUnderHighChurnScenario
- themisdb::process::stress_scenarios::CascadingFailuresScenario
- themisdb::process::stress_scenarios::CorrelationIdPropagationScenario
- themisdb::process::stress_scenarios::FederationStressScenario
- themisdb::process::stress_scenarios::MultiModelConflictScenario
- themisdb::process::stress_scenarios::NetworkPartitionHealingScenario
- themisdb::process::stress_scenarios::PartitionDetectionLatencyScenario
- themisdb::process::stress_scenarios::RejoinLogSyncScenario
- themisdb::process::stress_scenarios::SplitBrainPreventionScenario
- themisdb::process::stress_scenarios::TemporalReconstructionScenario
- themisdb::process::stress_scenarios::TraceCompletenessPartitionScenario

### Structs
- DsgvoViolation
- ParserEdgeTest::ParseResult
- RetrieverEdgeTest::MockRetrievalResult
- RetrieverResilienceTest::MockRetriever
- StressChurnTest::OperationStats
- StubProcess
- themis::process::AuditEntry
- themis::process::AuditLoggerConfig
- themis::process::AuditLoggerStats
- themis::process::AuditRecord
- themis::process::AuditRetentionPolicy
- themis::process::AuditTrailEntry
- themis::process::BatchQueueEntry
- themis::process::BpmnParsingDeterminismSpec
- themis::process::BpmnSerializer::ImportResult
- themis::process::BpmnSerializerConcurrencyContract
- themis::process::BulkLinkCreationStress
- themis::process::CircularReferenceDetectionStress
- themis::process::CmmnParsingDeterminismSpec
- themis::process::CmmnSerializer::ImportResult
- themis::process::CmmnSerializerConcurrencyContract
- themis::process::CommunityDetectionTimeoutStress
- themis::process::ConcurrentQueryChurnStress
- themis::process::ConflictAnalysis
- themis::process::ConflictContext
- themis::process::ConflictInfo
- themis::process::ConflictMetadata
- themis::process::ConflictResolutionConfig
- themis::process::ConflictResolutionResult
- themis::process::ConflictResolverConfig
- themis::process::ConflictResolverStats
- themis::process::ConsensusLogEntry
- themis::process::ConsensusSnapshot
- themis::process::DecisionTable
- themis::process::DeepNestingStress
- themis::process::DmnRule
- themis::process::EmptyGraphQueryStress
- themis::process::EpkArisXmlImporter::ImportResult
- themis::process::EpkSerializer::ImportResult
- themis::process::FederationConsensusConfig
- themis::process::FederationReplicaConfig
- themis::process::FimModelResult
- themis::process::GossipMessage
- themis::process::GossipVersionVector
- themis::process::HighChurnScenarioGuidelines
- themis::process::HistorySnapshot
- themis::process::LLMDescriptorConfig
- themis::process::LWWResolutionStrategy
- themis::process::LargeContextSizeStress
- themis::process::LargeElementCountStress
- themis::process::LightRetrievalResult
- themis::process::LinkAttributeMutationStress
- themis::process::LinkConsistencyDeterminismSpec
- themis::process::LlmProcessDescriptor::Config
- themis::process::LockFreeLinkEntry
- themis::process::LockFreeLinkerConfig
- themis::process::LockFreeLinkerMetrics
- themis::process::MalformedXmlRecoveryStress
- themis::process::ModelDelta
- themis::process::ModelHistoryConfig
- themis::process::ModelUpdateDeterminismSpec
- themis::process::ModelVersion
- themis::process::ObjectCentricTracer::ConvergenceDivergenceResult
- themis::process::OcelEvent
- themis::process::OperationLogQuery
- themis::process::OperationLogQueryResult
- themis::process::OrphanedLinkResolutionStress
- themis::process::PaxosAcceptRequest
- themis::process::PaxosAcceptResponse
- themis::process::PaxosPrepareRequest
- themis::process::PaxosPrepareResponse
- themis::process::PprConfig
- themis::process::ProcessAgenticConfig
- themis::process::ProcessAgenticResult
- themis::process::ProcessAgenticResult::IterationSummary
- themis::process::ProcessAttachment
- themis::process::ProcessCommunity
- themis::process::ProcessDescriptor
- themis::process::ProcessGraphRag::ComplianceCheckResult
- themis::process::ProcessGraphRag::NodeDwellStats
- themis::process::ProcessGraphRag::ProcessKnowledgeGraph
- themis::process::ProcessGraphRag::SimilarCase
- themis::process::ProcessGraphRag::SlaAlert
- themis::process::ProcessGraphRag::SlaRuleEntry
- themis::process::ProcessLightRetriever::ResourceLimits
- themis::process::ProcessLightRetrieverConcurrencyContract
- themis::process::ProcessLink
- themis::process::ProcessLinker::ConflictRecord
- themis::process::ProcessLinkerConcurrencyContract
- themis::process::ProcessModelGenerator::Config
- themis::process::ProcessModelGenerator::ValidationResult
- themis::process::ProcessModelManager::TransactionContext
- themis::process::ProcessModelManagerConcurrencyContract
- themis::process::ProcessModelRecord
- themis::process::ProcessModelResult
- themis::process::ProcessRagConfig
- themis::process::ProcessRagContext
- themis::process::ProcessStressScenarios
- themis::process::RaftAppendEntriesRequest
- themis::process::RaftAppendEntriesResponse
- themis::process::RaftVoteRequest
- themis::process::RaftVoteResponse
- themis::process::RecoverModelRequest
- themis::process::RecoverModelResult
- themis::process::ReplicaStats
- themis::process::ReproducibilityGuidelines
- themis::process::RetrievalConsistencyDeterminismSpec
- themis::process::SerializerValidationResult
- themis::process::Snapshot
- themis::process::SpanAttributes
- themis::process::SpanEvent
- themis::process::SplitBrainConfig
- themis::process::StateTransitionDeterminismSpec
- themis::process::StressScenarioResult
- themis::process::TelemetryConfig
- themis::process::TelemetryStats
- themis::process::TemporalQuery
- themis::process::TemporalQueryResult
- themis::process::ThreeWayMergeStrategy
- themis::process::TraceContext
- themis::process::TracingConfig
- themis::process::UnsupportedGatewayStress
- themis::process::VccVpbImporter::ImportResult
- themis::process::VersionInfo
- themis::process::VersionedLinkState
- themis::process::XpdlImportResult
- themis::process::XpdlPackage
- themis::process::benchmark::BpmnParser::BpmnModel
- themis::process::benchmark::CmmnParser::CmmnModel
- themis::process::benchmark::CommunityDetector::Community
- themis::process::benchmark::DiagnosticEvent
- themis::process::benchmark::DirectlyFollowsGraph
- themis::process::benchmark::DmnParser::DmnModel
- themis::process::benchmark::EnhancedIncidentClassifier::ClassificationRule
- themis::process::benchmark::EpkParser::EpkModel
- themis::process::benchmark::Event
- themis::process::benchmark::EventLog
- themis::process::benchmark::FimParser::FimModel
- themis::process::benchmark::GraphLink
- themis::process::benchmark::GraphNode
- themis::process::benchmark::LinkRecord
- themis::process::benchmark::OcelParser::OcelLog
- themis::process::benchmark::ProcessConflict
- themis::process::benchmark::ProcessIncident
- themis::process::benchmark::ProcessLink
- themis::process::benchmark::ProcessModelRecord
- themis::process::benchmark::ProcessModelRevision
- themis::process::benchmark::QueryResult
- themis::process::benchmark::SimProcessModel
- themis::process::benchmark::Transaction
- themis::process::benchmark::VariantAnalyzer::Variant
- themis::process::benchmark::VccVpbParser::VccVpbModel
- themis::process::benchmark::VersionClock
- themisdb::process::AuditTrailEntry
- themisdb::process::CrashFaultToleranceGates
- themisdb::process::PartitionDetectionResult
- themisdb::process::ProcessFederationConfig
- themisdb::process::ReplicationStatus
- themisdb::process::SnapshotIndex
- themisdb::process::TemporalQueryResult
- themisdb::process::ThreadSafetyGuarantee

## Notes
- This is a derived artifact. Do not manually edit semantic content.
- Regenerate via scripts/generate_module_doxygen_xml.py --write-module-markdown.

## Detailed Function Documentation (Soll-Ist)
- Functions extracted: 1318

### BenchStubProcessManager

#### `BenchStubProcessManager()=default`
- Source: `benchmarks/process/bench_process_dedicated_gates.cpp`:43
- Brief: n/a
- Parameters: none

#### `int64_t active() const`
- Source: `benchmarks/process/bench_process_dedicated_gates.cpp`:69
- Brief: n/a
- Parameters: none

#### `void reset()`
- Source: `benchmarks/process/bench_process_dedicated_gates.cpp`:62
- Brief: n/a
- Parameters: none

#### `uint64_t spawn()`
- Source: `benchmarks/process/bench_process_dedicated_gates.cpp`:45
- Brief: n/a
- Parameters: none

#### `void terminate(uint64_t pid)`
- Source: `benchmarks/process/bench_process_dedicated_gates.cpp`:53
- Brief: n/a
- Parameters:
  - `pid` (uint64_t): n/a

### BenchStubSignalBus

#### `void reset()`
- Source: `benchmarks/process/bench_process_dedicated_gates.cpp`:89
- Brief: n/a
- Parameters: none

#### `void send(uint64_t pid, int signum)`
- Source: `benchmarks/process/bench_process_dedicated_gates.cpp`:83
- Brief: n/a
- Parameters:
  - `pid` (uint64_t): n/a
  - `signum` (int): n/a

#### `uint64_t sent() const`
- Source: `benchmarks/process/bench_process_dedicated_gates.cpp`:88
- Brief: n/a
- Parameters: none

### BottleneckAnalyticsTest

#### `void SetUp() override`
- Source: `tests/process/test_bottleneck_analytics.cpp`:31
- Brief: n/a
- Parameters: none

#### `void TearDown() override`
- Source: `tests/process/test_bottleneck_analytics.cpp`:54
- Brief: n/a
- Parameters: none

### DeterminismConflictTest

#### `void SetUp() override`
- Source: `tests/process/test_process_determinism_conflict_focused.cpp`:35
- Brief: n/a
- Parameters: none

### DiagnosticsIncidentTest

#### `int64_t capture_time_ms() const`
- Source: `tests/process/test_process_diagnostics_incident_focused.cpp`:25
- Brief: n/a
- Parameters: none

### EdgeTypeRegistryTest

#### `void SetUp() override`
- Source: `tests/process/test_process_graph.cpp`:16
- Brief: n/a
- Parameters: none

### LinkerEdgeTest

#### `void SetUp() override`
- Source: `tests/process/test_process_linker_edge_focused.cpp`:33
- Brief: n/a
- Parameters: none

#### `bool create_attachment(const ProcessAttachment &attach, ProcError &out_error)`
- Source: `tests/process/test_process_linker_edge_focused.cpp`:50
- Brief: n/a
- Parameters:
  - `attach` (const ProcessAttachment &): n/a
  - `out_error` (ProcError &): n/a

#### `bool create_link(const ProcessLink &link, ProcError &out_error)`
- Source: `tests/process/test_process_linker_edge_focused.cpp`:39
- Brief: n/a
- Parameters:
  - `link` (const ProcessLink &): n/a
  - `out_error` (ProcError &): n/a

### LlmDescriptorTest

#### `themis::process::ProcessModelRecord makeRecord()`
- Source: `tests/process/test_process_module.cpp`:240
- Brief: n/a
- Parameters: none

### ObjectCentricTracerTest

#### `void SetUp() override`
- Source: `tests/process/test_object_centric_tracer.cpp`:30
- Brief: n/a
- Parameters: none

#### `void TearDown() override`
- Source: `tests/process/test_object_centric_tracer.cpp`:49
- Brief: n/a
- Parameters: none

#### `void saveModel(const std::string &model_id, const json &nodes, const json &edges)`
- Source: `tests/process/test_object_centric_tracer.cpp`:58
- Brief: Persist a model with given nodes/edges.
- Parameters:
  - `model_id` (const std::string &): n/a
  - `nodes` (const json &): n/a
  - `edges` (const json &): n/a

### ParserEdgeTest

#### `ParseResult mock_parse_xml(std::string_view xml)`
- Source: `tests/process/test_process_parser_edge_focused.cpp`:30
- Brief: n/a
- Parameters:
  - `xml` (std::string_view): n/a

### ProcessCommunityDetectorTest

#### `void SetUp() override`
- Source: `tests/process/test_process_community_detector.cpp`:30
- Brief: n/a
- Parameters: none

#### `void TearDown() override`
- Source: `tests/process/test_process_community_detector.cpp`:48
- Brief: n/a
- Parameters: none

#### `void saveModel(const std::string &model_id, const json &nodes, const json &edges)`
- Source: `tests/process/test_process_community_detector.cpp`:56
- Brief: Persist a model with given nodes/edges in normalized form.
- Parameters:
  - `model_id` (const std::string &): n/a
  - `nodes` (const json &): n/a
  - `edges` (const json &): n/a

### ProcessGraphRagTest

#### `std::string importBauantragModel()`
- Source: `tests/process/test_process_module.cpp`:529
- Brief: n/a
- Parameters: none

#### `std::string startInstance(const std::string &model_id)`
- Source: `tests/process/test_process_module.cpp`:537
- Brief: n/a
- Parameters:
  - `model_id` (const std::string &): n/a

### ProcessGraphTest

#### `void SetUp() override`
- Source: `tests/process/test_process_graph.cpp`:166
- Brief: n/a
- Parameters: none

#### `void TearDown() override`
- Source: `tests/process/test_process_graph.cpp`:186
- Brief: n/a
- Parameters: none

### ProcessLightRetrieverTest

#### `void SetUp() override`
- Source: `tests/process/test_process_light_retriever.cpp`:31
- Brief: n/a
- Parameters: none

#### `void TearDown() override`
- Source: `tests/process/test_process_light_retriever.cpp`:55
- Brief: n/a
- Parameters: none

#### `std::string saveMinimalModel(const std::string &model_id)`
- Source: `tests/process/test_process_light_retriever.cpp`:67
- Brief: Persist a minimal model and return its model_id.
- Parameters:
  - `model_id` (const std::string &): n/a

#### `void storeInstance(const std::string &instance_id, const std::string &model_id)`
- Source: `tests/process/test_process_light_retriever.cpp`:83
- Brief: Write an instance record to RocksDB so ProcessLightRetriever can resolve model_id.
- Parameters:
  - `instance_id` (const std::string &): n/a
  - `model_id` (const std::string &): n/a

### ProcessManagerFixture

#### `void SetUp(const benchmark::State &) override`
- Source: `benchmarks/process/bench_process_import_retrieval.cpp`:164
- Brief: n/a
- Parameters:
  - `<unnamed>` (const benchmark::State &): n/a

#### `void TearDown(const benchmark::State &) override`
- Source: `benchmarks/process/bench_process_import_retrieval.cpp`:179
- Brief: n/a
- Parameters:
  - `<unnamed>` (const benchmark::State &): n/a

### ProcessMiningTest

#### `void SetUp() override`
- Source: `tests/process/test_process_mining_v2.cpp`:102
- Brief: n/a
- Parameters: none

#### `void TearDown() override`
- Source: `tests/process/test_process_mining_v2.cpp`:111
- Brief: n/a
- Parameters: none

### ProcessModelHardeningTest

#### `void SetUp() override`
- Source: `tests/process/test_process_contract_hardening_focused.cpp`:103
- Brief: n/a
- Parameters: none

#### `void TearDown() override`
- Source: `tests/process/test_process_contract_hardening_focused.cpp`:116
- Brief: n/a
- Parameters: none

### ProcessModuleTest

#### `void SetUp() override`
- Source: `tests/process/test_process_module.cpp`:48
- Brief: n/a
- Parameters: none

#### `void TearDown() override`
- Source: `tests/process/test_process_module.cpp`:71
- Brief: n/a
- Parameters: none

### RetrieverEdgeTest

#### `MockRetrievalResult mock_retrieve_empty_graph()`
- Source: `tests/process/test_process_retriever_edge_focused.cpp`:43
- Brief: n/a
- Parameters: none

#### `MockRetrievalResult mock_retrieve_large_context(size_t target_size)`
- Source: `tests/process/test_process_retriever_edge_focused.cpp`:52
- Brief: n/a
- Parameters:
  - `target_size` (size_t): n/a

#### `MockRetrievalResult mock_retrieve_malformed_context()`
- Source: `tests/process/test_process_retriever_edge_focused.cpp`:96
- Brief: n/a
- Parameters: none

#### `MockRetrievalResult mock_retrieve_stale_link()`
- Source: `tests/process/test_process_retriever_edge_focused.cpp`:86
- Brief: n/a
- Parameters: none

#### `MockRetrievalResult mock_retrieve_with_timeout(int64_t simulated_latency)`
- Source: `tests/process/test_process_retriever_edge_focused.cpp`:68
- Brief: n/a
- Parameters:
  - `simulated_latency` (int64_t): n/a

### RetrieverResilienceTest::BoundedCache

#### `BoundedCache(size_t max_size_)`
- Source: `tests/process/test_process_retriever_resilience_focused.cpp`:36
- Brief: n/a
- Parameters:
  - `max_size_` (size_t): n/a

#### `void clear()`
- Source: `tests/process/test_process_retriever_resilience_focused.cpp`:56
- Brief: n/a
- Parameters: none

#### `bool get(const K &key, V &out_value)`
- Source: `tests/process/test_process_retriever_resilience_focused.cpp`:38
- Brief: n/a
- Parameters:
  - `key` (const K &): n/a
  - `out_value` (V &): n/a

#### `bool put(const K &key, const V &value)`
- Source: `tests/process/test_process_retriever_resilience_focused.cpp`:47
- Brief: n/a
- Parameters:
  - `key` (const K &): n/a
  - `value` (const V &): n/a

#### `size_t size() const`
- Source: `tests/process/test_process_retriever_resilience_focused.cpp`:55
- Brief: n/a
- Parameters: none

### RetrieverResilienceTest::MockRetriever

#### `bool retrieve_with_resource_check(const std::string &instance_id, const std::string &query, std::string &out_context)`
- Source: `tests/process/test_process_retriever_resilience_focused.cpp`:66
- Brief: n/a
- Parameters:
  - `instance_id` (const std::string &): n/a
  - `query` (const std::string &): n/a
  - `out_context` (std::string &): n/a

### SlaMonitoringTest

#### `void SetUp() override`
- Source: `tests/process/test_sla_monitoring.cpp`:30
- Brief: n/a
- Parameters: none

#### `void TearDown() override`
- Source: `tests/process/test_sla_monitoring.cpp`:55
- Brief: n/a
- Parameters: none

#### `CEPEngine & initializedCep()`
- Source: `tests/process/test_sla_monitoring.cpp`:69
- Brief: Build an initialized CEPEngine for tests that need it.
- Parameters: none

### StressChurnTest

#### `void SetUp() override`
- Source: `tests/process/test_process_stress_churn_focused.cpp`:36
- Brief: n/a
- Parameters: none

### StressChurnTest::OperationStats

#### `double ops_per_sec() const`
- Source: `tests/process/test_process_stress_churn_focused.cpp`:45
- Brief: n/a
- Parameters: none

### StubProcessManager

#### `StubProcessManager(uint64_t max_processes)`
- Source: `tests/process/test_process_highcardinality_stress.cpp`:48
- Brief: n/a
- Parameters:
  - `max_processes` (uint64_t): n/a

#### `int64_t activeCount() const`
- Source: `tests/process/test_process_highcardinality_stress.cpp`:80
- Brief: n/a
- Parameters: none

#### `int64_t fdCount() const`
- Source: `tests/process/test_process_highcardinality_stress.cpp`:81
- Brief: n/a
- Parameters: none

#### `uint64_t spawn()`
- Source: `tests/process/test_process_highcardinality_stress.cpp`:52
- Brief: Returns allocated pid, or 0 on failure (capacity exceeded).
- Parameters: none

#### `uint64_t spawnCount() const`
- Source: `tests/process/test_process_highcardinality_stress.cpp`:77
- Brief: n/a
- Parameters: none

#### `uint64_t spawnFailures() const`
- Source: `tests/process/test_process_highcardinality_stress.cpp`:79
- Brief: n/a
- Parameters: none

#### `bool terminate(uint64_t pid)`
- Source: `tests/process/test_process_highcardinality_stress.cpp`:66
- Brief: n/a
- Parameters:
  - `pid` (uint64_t): n/a

#### `uint64_t terminateCount() const`
- Source: `tests/process/test_process_highcardinality_stress.cpp`:78
- Brief: n/a
- Parameters: none

### StubSignalBus

#### `uint64_t delivered() const`
- Source: `tests/process/test_process_highcardinality_stress.cpp`:106
- Brief: n/a
- Parameters: none

#### `void send(uint64_t pid, int signum)`
- Source: `tests/process/test_process_highcardinality_stress.cpp`:100
- Brief: n/a
- Parameters:
  - `pid` (uint64_t): n/a
  - `signum` (int): n/a

#### `uint64_t sent() const`
- Source: `tests/process/test_process_highcardinality_stress.cpp`:105
- Brief: n/a
- Parameters: none

### bench_cross_functional_end_to_end.cpp

#### `Arg(10) -> Arg(50) ->Arg(100)`
- Source: `benchmarks/process/bench_cross_functional_end_to_end.cpp`:265
- Brief: n/a
- Parameters:
  - `<unnamed>` (10): n/a

#### `BENCHMARK(BM_CompletePluginWorkflowWithMetrics)`
- Source: `benchmarks/process/bench_cross_functional_end_to_end.cpp`:512
- Brief: n/a
- Parameters:
  - `<unnamed>` (BM_CompletePluginWorkflowWithMetrics): n/a

#### `BENCHMARK(BM_ConcurrentMultiComponentWorkload)`
- Source: `benchmarks/process/bench_cross_functional_end_to_end.cpp`:574
- Brief: n/a
- Parameters:
  - `<unnamed>` (BM_ConcurrentMultiComponentWorkload): n/a

#### `BENCHMARK(BM_PluginInfoCacheWithMetrics)`
- Source: `benchmarks/process/bench_cross_functional_end_to_end.cpp`:380
- Brief: n/a
- Parameters:
  - `<unnamed>` (BM_PluginInfoCacheWithMetrics): n/a

#### `BENCHMARK(BM_PluginQueryWithMetrics)`
- Source: `benchmarks/process/bench_cross_functional_end_to_end.cpp`:327
- Brief: n/a
- Parameters:
  - `<unnamed>` (BM_PluginQueryWithMetrics): n/a

#### `BENCHMARK_MAIN()`
- Source: `benchmarks/process/bench_cross_functional_end_to_end.cpp`:580
- Brief: n/a
- Parameters: none

#### `void BM_CompletePluginWorkflowWithMetrics(benchmark::State &state)`
- Source: `benchmarks/process/bench_cross_functional_end_to_end.cpp`:440
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_ConcurrentMultiComponentWorkload(benchmark::State &state)`
- Source: `benchmarks/process/bench_cross_functional_end_to_end.cpp`:514
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_PluginDiscoveryWithMetrics(benchmark::State &state)`
- Source: `benchmarks/process/bench_cross_functional_end_to_end.cpp`:226
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_PluginInfoCacheWithMetrics(benchmark::State &state)`
- Source: `benchmarks/process/bench_cross_functional_end_to_end.cpp`:329
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_PluginQueryWithMetrics(benchmark::State &state)`
- Source: `benchmarks/process/bench_cross_functional_end_to_end.cpp`:269
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void createPluginManifest(const std::string &dir, const std::string &name, PluginType type)`
- Source: `benchmarks/process/bench_cross_functional_end_to_end.cpp`:58
- Brief: n/a
- Parameters:
  - `dir` (const std::string &): n/a
  - `name` (const std::string &): n/a
  - `type` (PluginType): n/a

#### `std::vector< uint8_t > generateMockAudio(size_t size)`
- Source: `benchmarks/process/bench_cross_functional_end_to_end.cpp`:49
- Brief: n/a
- Parameters:
  - `size` (size_t): n/a

### bench_process_advanced_workflows.cpp

#### `BENCHMARK_MAIN()`
- Source: `benchmarks/process/bench_process_advanced_workflows.cpp`:679
- Brief: n/a
- Parameters: none

### bench_process_concurrency_gates.cpp

#### `BENCHMARK_MAIN()`
- Source: `benchmarks/process/bench_process_concurrency_gates.cpp`:588
- Brief: n/a
- Parameters: none

### bench_process_dedicated_gates.cpp

#### `BENCHMARK_MAIN()`
- Source: `benchmarks/process/bench_process_dedicated_gates.cpp`:214
- Brief: n/a
- Parameters: none

#### `void BM_PM_BM_01_SpawnLatencyP95(benchmark::State &state)`
- Source: `benchmarks/process/bench_process_dedicated_gates.cpp`:101
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_PM_BM_02_TerminateLatencyP95(benchmark::State &state)`
- Source: `benchmarks/process/bench_process_dedicated_gates.cpp`:122
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_PM_BM_03_SignalDeliverP99(benchmark::State &state)`
- Source: `benchmarks/process/bench_process_dedicated_gates.cpp`:156
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_PM_BM_04_ConcurrentSpawnThroughput(benchmark::State &state)`
- Source: `benchmarks/process/bench_process_dedicated_gates.cpp`:178
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `Name("PM-BM-01/SpawnLatencyP95") -> Repetitions(5) ->ReportAggregatesOnly(true) ->Unit(benchmark::kMicrosecond)`
- Source: `benchmarks/process/bench_process_dedicated_gates.cpp`:111
- Brief: n/a
- Parameters:
  - `<unnamed>` ("PM-BM-01/SpawnLatencyP95"): n/a

#### `Name("PM-BM-02/TerminateLatencyP95") -> Repetitions(5) ->ReportAggregatesOnly(true) ->Unit(benchmark::kMicrosecond)`
- Source: `benchmarks/process/bench_process_dedicated_gates.cpp`:146
- Brief: n/a
- Parameters:
  - `<unnamed>` ("PM-BM-02/TerminateLatencyP95"): n/a

#### `Name("PM-BM-03/SignalDeliverP99") -> Repetitions(5) ->ReportAggregatesOnly(true) ->Unit(benchmark::kNanosecond)`
- Source: `benchmarks/process/bench_process_dedicated_gates.cpp`:167
- Brief: n/a
- Parameters:
  - `<unnamed>` ("PM-BM-03/SignalDeliverP99"): n/a

#### `Name("PM-BM-04/ConcurrentSpawnThroughput") -> Arg(500) ->Repetitions(3) ->ReportAggregatesOnly(true) ->Unit(benchmark::kMillisecond)`
- Source: `benchmarks/process/bench_process_dedicated_gates.cpp`:208
- Brief: n/a
- Parameters:
  - `<unnamed>` ("PM-BM-04/ConcurrentSpawnThroughput"): n/a

### bench_process_determinism_gates.cpp

#### `BENCHMARK_MAIN()`
- Source: `benchmarks/process/bench_process_determinism_gates.cpp`:562
- Brief: n/a
- Parameters: none

### bench_process_diagnostics_overhead.cpp

#### `BENCHMARK_MAIN()`
- Source: `benchmarks/process/bench_process_diagnostics_overhead.cpp`:732
- Brief: n/a
- Parameters: none

### bench_process_import_retrieval.cpp

#### `Arg(10) -> Arg(100) ->Arg(500) ->Unit(benchmark::kMillisecond)`
- Source: `benchmarks/process/bench_process_import_retrieval.cpp`:235
- Brief: n/a
- Parameters:
  - `<unnamed>` (10): n/a

#### `Arg(5) -> Arg(20) ->Arg(50) ->Arg(100) ->Arg(200) ->Unit(benchmark::kMicrosecond)`
- Source: `benchmarks/process/bench_process_import_retrieval.cpp`:116
- Brief: n/a
- Parameters:
  - `<unnamed>` (5): n/a

#### `BENCHMARK(BM_BpmnExport) -> Arg(5) ->Arg(20) ->Arg(100) ->Unit(benchmark::kMicrosecond)`
- Source: `benchmarks/process/bench_process_import_retrieval.cpp`:140
- Brief: n/a
- Parameters:
  - `<unnamed>` (BM_BpmnExport): n/a

#### `BENCHMARK(BM_BuildConformancePrompt) -> Arg(5) ->Arg(20) ->Arg(50)`
- Source: `benchmarks/process/bench_process_import_retrieval.cpp`:289
- Brief: n/a
- Parameters:
  - `<unnamed>` (BM_BuildConformancePrompt): n/a

#### `BENCHMARK(BM_BuildSystemPrompt) -> Arg(5) ->Arg(20) ->Arg(100)`
- Source: `benchmarks/process/bench_process_import_retrieval.cpp`:268
- Brief: n/a
- Parameters:
  - `<unnamed>` (BM_BuildSystemPrompt): n/a

#### `BENCHMARK(BM_LlmDescriptor_Generate) -> Arg(5) ->Arg(20) ->Arg(50) ->Arg(100)`
- Source: `benchmarks/process/bench_process_import_retrieval.cpp`:252
- Brief: n/a
- Parameters:
  - `<unnamed>` (BM_LlmDescriptor_Generate): n/a

#### `BENCHMARK(BM_SummarizeList) -> Arg(1) ->Arg(10) ->Arg(50) ->Arg(100) ->Unit(benchmark::kMicrosecond)`
- Source: `benchmarks/process/bench_process_import_retrieval.cpp`:309
- Brief: n/a
- Parameters:
  - `<unnamed>` (BM_SummarizeList): n/a

#### `BENCHMARK_F(ProcessManagerFixture, List_Scan)(benchmark`
- Source: `benchmarks/process/bench_process_import_retrieval.cpp`:218
- Brief: n/a
- Parameters:
  - `<unnamed>` (ProcessManagerFixture): n/a
  - `<unnamed>` (List_Scan): n/a

#### `BENCHMARK_F(ProcessManagerFixture, SaveLoad_RoundTrip)(benchmark`
- Source: `benchmarks/process/bench_process_import_retrieval.cpp`:191
- Brief: n/a
- Parameters:
  - `<unnamed>` (ProcessManagerFixture): n/a
  - `<unnamed>` (SaveLoad_RoundTrip): n/a

#### `BENCHMARK_F(ProcessManagerFixture, Save_Throughput)(benchmark`
- Source: `benchmarks/process/bench_process_import_retrieval.cpp`:205
- Brief: n/a
- Parameters:
  - `<unnamed>` (ProcessManagerFixture): n/a
  - `<unnamed>` (Save_Throughput): n/a

#### `void BM_BpmnExport(benchmark::State &state)`
- Source: `benchmarks/process/bench_process_import_retrieval.cpp`:121
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_BpmnImport_NodeCount(benchmark::State &state)`
- Source: `benchmarks/process/bench_process_import_retrieval.cpp`:103
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_BuildConformancePrompt(benchmark::State &state)`
- Source: `benchmarks/process/bench_process_import_retrieval.cpp`:272
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_BuildSystemPrompt(benchmark::State &state)`
- Source: `benchmarks/process/bench_process_import_retrieval.cpp`:256
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_EpkImport_EventCount(benchmark::State &state)`
- Source: `benchmarks/process/bench_process_import_retrieval.cpp`:144
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_LlmDescriptor_Generate(benchmark::State &state)`
- Source: `benchmarks/process/bench_process_import_retrieval.cpp`:240
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_SummarizeList(benchmark::State &state)`
- Source: `benchmarks/process/bench_process_import_retrieval.cpp`:293
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `std::string makeBpmnXml(int model_idx, int node_count)`
- Source: `benchmarks/process/bench_process_import_retrieval.cpp`:42
- Brief: Generate a minimal, well-formed BPMN 2.0 XML fragment.
- Parameters:
  - `model_idx` (int): n/a
  - `node_count` (int): n/a

#### `std::string makeEpkText(int model_idx, int event_count)`
- Source: `benchmarks/process/bench_process_import_retrieval.cpp`:72
- Brief: Generate a minimal EPK text fragment (simple line-based notation).
- Parameters:
  - `model_idx` (int): n/a
  - `event_count` (int): n/a

#### `ProcessModelRecord makeRecord(int idx, int node_count=5)`
- Source: `benchmarks/process/bench_process_import_retrieval.cpp`:85
- Brief: Build a synthetic ProcessModelRecord for testing descriptor generation.
- Parameters:
  - `idx` (int): n/a
  - `node_count` (int): n/a

### bench_process_linker_gates.cpp

#### `BENCHMARK_MAIN()`
- Source: `benchmarks/process/bench_process_linker_gates.cpp`:583
- Brief: n/a
- Parameters: none

### bench_process_mining.cpp

#### `Arg(10) -> Arg(50) ->Arg(100) ->Arg(500) ->Unit(benchmark::kMillisecond)`
- Source: `benchmarks/process/bench_process_mining.cpp`:101
- Brief: n/a
- Parameters:
  - `<unnamed>` (10): n/a

#### `Arg(100) -> Arg(1000) ->Arg(10000) ->Unit(benchmark::kMillisecond)`
- Source: `benchmarks/process/bench_process_mining.cpp`:196
- Brief: n/a
- Parameters:
  - `<unnamed>` (100): n/a

#### `Arg(50) -> Arg(100) ->Arg(500) ->Arg(1000) ->Unit(benchmark::kMillisecond)`
- Source: `benchmarks/process/bench_process_mining.cpp`:302
- Brief: n/a
- Parameters:
  - `<unnamed>` (50): n/a

#### `Args({100, 10}) -> Args({500, 20}) ->Args({1000, 10}) ->Unit(benchmark::kMillisecond)`
- Source: `benchmarks/process/bench_process_mining.cpp`:221
- Brief: n/a
- Parameters:
  - `<unnamed>` ({100, 10}): n/a

#### `BENCHMARK(BM_ProcessMining_BPMNExport) -> Unit(benchmark::kMicrosecond)`
- Source: `benchmarks/process/bench_process_mining.cpp`:394
- Brief: n/a
- Parameters:
  - `<unnamed>` (BM_ProcessMining_BPMNExport): n/a

#### `BENCHMARK(BM_ProcessMining_PNMLExport) -> Unit(benchmark::kMicrosecond)`
- Source: `benchmarks/process/bench_process_mining.cpp`:415
- Brief: n/a
- Parameters:
  - `<unnamed>` (BM_ProcessMining_PNMLExport): n/a

#### `BENCHMARK_MAIN()`
- Source: `benchmarks/process/bench_process_mining.cpp`:426
- Brief: n/a
- Parameters: none

#### `void BM_ProcessMining_AlphaMiner(benchmark::State &state)`
- Source: `benchmarks/process/bench_process_mining.cpp`:84
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a
- Details: @benchmark Alpha Miner performance

#### `void BM_ProcessMining_BPMNExport(benchmark::State &state)`
- Source: `benchmarks/process/bench_process_mining.cpp`:374
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a
- Details: @benchmark BPMN export performance

#### `void BM_ProcessMining_ConformanceChecking(benchmark::State &state)`
- Source: `benchmarks/process/bench_process_mining.cpp`:339
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a
- Details: @benchmark Token replay conformance checking

#### `void BM_ProcessMining_DFGCreation(benchmark::State &state)`
- Source: `benchmarks/process/bench_process_mining.cpp`:233
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a
- Details: @benchmark DFG creation performance

#### `void BM_ProcessMining_DFGWithPerformance(benchmark::State &state)`
- Source: `benchmarks/process/bench_process_mining.cpp`:258
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a
- Details: @benchmark DFG with performance metrics

#### `void BM_ProcessMining_EventLogExtraction(benchmark::State &state)`
- Source: `benchmarks/process/bench_process_mining.cpp`:166
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a
- Details: @benchmark Event log extraction with varying log sizes

#### `void BM_ProcessMining_HeuristicMiner(benchmark::State &state)`
- Source: `benchmarks/process/bench_process_mining.cpp`:110
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a
- Details: @benchmark Heuristic Miner performance

#### `void BM_ProcessMining_InductiveMiner(benchmark::State &state)`
- Source: `benchmarks/process/bench_process_mining.cpp`:136
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a
- Details: @benchmark Inductive Miner performance

#### `void BM_ProcessMining_LargeLogProcessing(benchmark::State &state)`
- Source: `benchmarks/process/bench_process_mining.cpp`:204
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a
- Details: @benchmark Processing large event logs with many traces

#### `void BM_ProcessMining_PNMLExport(benchmark::State &state)`
- Source: `benchmarks/process/bench_process_mining.cpp`:399
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a
- Details: @benchmark Petri Net (PNML) export performance

#### `void BM_ProcessMining_VariantAnalysis(benchmark::State &state)`
- Source: `benchmarks/process/bench_process_mining.cpp`:286
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a
- Details: @benchmark Variant analysis performance

#### `void BM_ProcessMining_VariantClustering(benchmark::State &state)`
- Source: `benchmarks/process/bench_process_mining.cpp`:311
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a
- Details: @benchmark Variant clustering performance

#### `EventLog createSyntheticEventLog(size_t num_cases, size_t events_per_case)`
- Source: `benchmarks/process/bench_process_mining.cpp`:29
- Brief: Create a synthetic event log for benchmarking.
- Parameters:
  - `num_cases` (size_t): n/a
  - `events_per_case` (size_t): n/a

#### `std::unique_ptr< RocksDBWrapper > setupDatabase(const std::string &path)`
- Source: `benchmarks/process/bench_process_mining.cpp`:69
- Brief: Setup database for benchmarking.
- Parameters:
  - `path` (const std::string &): n/a

### bench_process_parser_gates.cpp

#### `BENCHMARK_MAIN()`
- Source: `benchmarks/process/bench_process_parser_gates.cpp`:680
- Brief: n/a
- Parameters: none

### bench_process_release_gates.cpp

#### `BENCHMARK(BM_ProcError_BatchCast) -> Arg(1000) ->Repetitions(5) ->ReportAggregatesOnly(true)`
- Source: `benchmarks/process/bench_process_release_gates.cpp`:85
- Brief: n/a
- Parameters:
  - `<unnamed>` (BM_ProcError_BatchCast): n/a

#### `BENCHMARK(BM_ProcError_Cast) -> Repetitions(5) ->ReportAggregatesOnly(true)`
- Source: `benchmarks/process/bench_process_release_gates.cpp`:24
- Brief: n/a
- Parameters:
  - `<unnamed>` (BM_ProcError_Cast): n/a

#### `BENCHMARK(BM_ProcError_RangeCheck) -> Repetitions(5) ->ReportAggregatesOnly(true)`
- Source: `benchmarks/process/bench_process_release_gates.cpp`:65
- Brief: n/a
- Parameters:
  - `<unnamed>` (BM_ProcError_RangeCheck): n/a

#### `BENCHMARK(BM_ProcError_SwitchDispatch) -> Repetitions(5) ->ReportAggregatesOnly(true)`
- Source: `benchmarks/process/bench_process_release_gates.cpp`:48
- Brief: n/a
- Parameters:
  - `<unnamed>` (BM_ProcError_SwitchDispatch): n/a

#### `BENCHMARK_MAIN()`
- Source: `benchmarks/process/bench_process_release_gates.cpp`:87
- Brief: n/a
- Parameters: none

#### `void BM_ProcError_BatchCast(benchmark::State &state)`
- Source: `benchmarks/process/bench_process_release_gates.cpp`:67
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_ProcError_Cast(benchmark::State &state)`
- Source: `benchmarks/process/bench_process_release_gates.cpp`:10
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_ProcError_RangeCheck(benchmark::State &state)`
- Source: `benchmarks/process/bench_process_release_gates.cpp`:50
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_ProcError_SwitchDispatch(benchmark::State &state)`
- Source: `benchmarks/process/bench_process_release_gates.cpp`:26
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

### bench_process_retrieval.cpp

#### `Arg(10) -> Arg(50) ->Arg(200) ->Unit(benchmark::kMicrosecond)`
- Source: `benchmarks/process/bench_process_retrieval.cpp`:252
- Brief: n/a
- Parameters:
  - `<unnamed>` (10): n/a

#### `Arg(256) -> Arg(1024) ->Arg(4096) ->Unit(benchmark::kMicrosecond)`
- Source: `benchmarks/process/bench_process_retrieval.cpp`:288
- Brief: n/a
- Parameters:
  - `<unnamed>` (256): n/a

#### `Arg(64) -> Arg(256) ->Arg(1024) ->Unit(benchmark::kMicrosecond)`
- Source: `benchmarks/process/bench_process_retrieval.cpp`:342
- Brief: n/a
- Parameters:
  - `<unnamed>` (64): n/a

#### `void BM_ProcessEmbeddingGenerate(benchmark::State &state)`
- Source: `benchmarks/process/bench_process_retrieval.cpp`:266
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_ProcessEmbeddingPersist(benchmark::State &state)`
- Source: `benchmarks/process/bench_process_retrieval.cpp`:302
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_ProcessFullTextSearch(benchmark::State &state)`
- Source: `benchmarks/process/bench_process_retrieval.cpp`:355
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_ProcessHnswRetrieve(benchmark::State &state)`
- Source: `benchmarks/process/bench_process_retrieval.cpp`:402
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_ProcessModelImport(benchmark::State &state)`
- Source: `benchmarks/process/bench_process_retrieval.cpp`:226
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_ProcessStateChangeEmbed(benchmark::State &state)`
- Source: `benchmarks/process/bench_process_retrieval.cpp`:432
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

### bench_process_retriever_gates.cpp

#### `BENCHMARK_MAIN()`
- Source: `benchmarks/process/bench_process_retriever_gates.cpp`:622
- Brief: n/a
- Parameters: none

### bench_video_processor.cpp

#### `BENCHMARK(BM_VideoProcessor_Disabled)`
- Source: `benchmarks/process/bench_video_processor.cpp`:11
- Brief: n/a
- Parameters:
  - `<unnamed>` (BM_VideoProcessor_Disabled): n/a

#### `BENCHMARK_MAIN()`
- Source: `benchmarks/process/bench_video_processor.cpp`:12
- Brief: n/a
- Parameters: none

#### `void BM_VideoProcessor_Disabled(benchmark::State &state)`
- Source: `benchmarks/process/bench_video_processor.cpp`:4
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

### test_bottleneck_analytics.cpp

#### `TEST_F(BottleneckAnalyticsTest, BOT01_NoDataReturnsEmpty)`
- Source: `tests/process/test_bottleneck_analytics.cpp`:75
- Brief: n/a
- Parameters:
  - `<unnamed>` (BottleneckAnalyticsTest): n/a
  - `<unnamed>` (BOT01_NoDataReturnsEmpty): n/a

#### `TEST_F(BottleneckAnalyticsTest, BOT02_SingleRecordCorrectAvg)`
- Source: `tests/process/test_bottleneck_analytics.cpp`:84
- Brief: n/a
- Parameters:
  - `<unnamed>` (BottleneckAnalyticsTest): n/a
  - `<unnamed>` (BOT02_SingleRecordCorrectAvg): n/a

#### `TEST_F(BottleneckAnalyticsTest, BOT03_MultipleRecordsCorrectAvg)`
- Source: `tests/process/test_bottleneck_analytics.cpp`:97
- Brief: n/a
- Parameters:
  - `<unnamed>` (BottleneckAnalyticsTest): n/a
  - `<unnamed>` (BOT03_MultipleRecordsCorrectAvg): n/a

#### `TEST_F(BottleneckAnalyticsTest, BOT04_MultiplNodesDescendingOrder)`
- Source: `tests/process/test_bottleneck_analytics.cpp`:113
- Brief: n/a
- Parameters:
  - `<unnamed>` (BottleneckAnalyticsTest): n/a
  - `<unnamed>` (BOT04_MultiplNodesDescendingOrder): n/a

#### `TEST_F(BottleneckAnalyticsTest, BOT05_TopNLimitsResults)`
- Source: `tests/process/test_bottleneck_analytics.cpp`:130
- Brief: n/a
- Parameters:
  - `<unnamed>` (BottleneckAnalyticsTest): n/a
  - `<unnamed>` (BOT05_TopNLimitsResults): n/a

#### `TEST_F(BottleneckAnalyticsTest, BOT06_P95Correct)`
- Source: `tests/process/test_bottleneck_analytics.cpp`:148
- Brief: n/a
- Parameters:
  - `<unnamed>` (BottleneckAnalyticsTest): n/a
  - `<unnamed>` (BOT06_P95Correct): n/a

#### `TEST_F(BottleneckAnalyticsTest, BOT07_RollingBufferOver200)`
- Source: `tests/process/test_bottleneck_analytics.cpp`:165
- Brief: n/a
- Parameters:
  - `<unnamed>` (BottleneckAnalyticsTest): n/a
  - `<unnamed>` (BOT07_RollingBufferOver200): n/a

#### `TEST_F(BottleneckAnalyticsTest, BOT08_NoCrossContamination)`
- Source: `tests/process/test_bottleneck_analytics.cpp`:184
- Brief: n/a
- Parameters:
  - `<unnamed>` (BottleneckAnalyticsTest): n/a
  - `<unnamed>` (BOT08_NoCrossContamination): n/a

### test_bpmn_s.cpp

#### `TEST(BpmnS, BMS01_AnnotationParsed)`
- Source: `tests/process/test_bpmn_s.cpp`:176
- Brief: n/a
- Parameters:
  - `<unnamed>` (BpmnS): n/a
  - `<unnamed>` (BMS01_AnnotationParsed): n/a

#### `TEST(BpmnS, BMS02_NoAnnotationIsNull)`
- Source: `tests/process/test_bpmn_s.cpp`:196
- Brief: n/a
- Parameters:
  - `<unnamed>` (BpmnS): n/a
  - `<unnamed>` (BMS02_NoAnnotationIsNull): n/a

#### `TEST(BpmnS, BMS03_PersonalDataNoLegalBasisIsViolation)`
- Source: `tests/process/test_bpmn_s.cpp`:207
- Brief: n/a
- Parameters:
  - `<unnamed>` (BpmnS): n/a
  - `<unnamed>` (BMS03_PersonalDataNoLegalBasisIsViolation): n/a

#### `TEST(BpmnS, BMS04_SensitiveDataWithLegalBasisNoViolation)`
- Source: `tests/process/test_bpmn_s.cpp`:226
- Brief: n/a
- Parameters:
  - `<unnamed>` (BpmnS): n/a
  - `<unnamed>` (BMS04_SensitiveDataWithLegalBasisNoViolation): n/a

#### `TEST(BpmnS, BMS05_ConsentRequiredWithoutArt6aIsViolation)`
- Source: `tests/process/test_bpmn_s.cpp`:236
- Brief: n/a
- Parameters:
  - `<unnamed>` (BpmnS): n/a
  - `<unnamed>` (BMS05_ConsentRequiredWithoutArt6aIsViolation): n/a

#### `TEST(BpmnS, BMS06_ConsentRequiredWithArt6aNoViolation)`
- Source: `tests/process/test_bpmn_s.cpp`:255
- Brief: n/a
- Parameters:
  - `<unnamed>` (BpmnS): n/a
  - `<unnamed>` (BMS06_ConsentRequiredWithArt6aNoViolation): n/a

#### `TEST(BpmnS, BMS07_RetentionDaysParsed)`
- Source: `tests/process/test_bpmn_s.cpp`:273
- Brief: n/a
- Parameters:
  - `<unnamed>` (BpmnS): n/a
  - `<unnamed>` (BMS07_RetentionDaysParsed): n/a

#### `TEST(BpmnS, BMS08_ExportRoundTrip)`
- Source: `tests/process/test_bpmn_s.cpp`:290
- Brief: n/a
- Parameters:
  - `<unnamed>` (BpmnS): n/a
  - `<unnamed>` (BMS08_ExportRoundTrip): n/a

#### `std::vector< DsgvoViolation > checkDsgvoNodes(const std::vector< ProcessNodeInfo > &nodes)`
- Source: `tests/process/test_bpmn_s.cpp`:41
- Brief: n/a
- Parameters:
  - `nodes` (const std::vector< ProcessNodeInfo > &): n/a

### test_cmmn_serializer.cpp

#### `TEST(CmmnSerializer, CMN01_ImportValid)`
- Source: `tests/process/test_cmmn_serializer.cpp`:88
- Brief: n/a
- Parameters:
  - `<unnamed>` (CmmnSerializer): n/a
  - `<unnamed>` (CMN01_ImportValid): n/a

#### `TEST(CmmnSerializer, CMN02_ImportEmpty)`
- Source: `tests/process/test_cmmn_serializer.cpp`:97
- Brief: n/a
- Parameters:
  - `<unnamed>` (CmmnSerializer): n/a
  - `<unnamed>` (CMN02_ImportEmpty): n/a

#### `TEST(CmmnSerializer, CMN03_TaskNodeTypes)`
- Source: `tests/process/test_cmmn_serializer.cpp`:104
- Brief: n/a
- Parameters:
  - `<unnamed>` (CmmnSerializer): n/a
  - `<unnamed>` (CMN03_TaskNodeTypes): n/a

#### `TEST(CmmnSerializer, CMN04_SentryCreatesEdge)`
- Source: `tests/process/test_cmmn_serializer.cpp`:130
- Brief: n/a
- Parameters:
  - `<unnamed>` (CmmnSerializer): n/a
  - `<unnamed>` (CMN04_SentryCreatesEdge): n/a

#### `TEST(CmmnSerializer, CMN05_StageNodeType)`
- Source: `tests/process/test_cmmn_serializer.cpp`:150
- Brief: n/a
- Parameters:
  - `<unnamed>` (CmmnSerializer): n/a
  - `<unnamed>` (CMN05_StageNodeType): n/a

#### `TEST(CmmnSerializer, CMN06_MilestoneNodeType)`
- Source: `tests/process/test_cmmn_serializer.cpp`:162
- Brief: n/a
- Parameters:
  - `<unnamed>` (CmmnSerializer): n/a
  - `<unnamed>` (CMN06_MilestoneNodeType): n/a

#### `TEST(CmmnSerializer, CMN07_ExportRoundTrip)`
- Source: `tests/process/test_cmmn_serializer.cpp`:174
- Brief: n/a
- Parameters:
  - `<unnamed>` (CmmnSerializer): n/a
  - `<unnamed>` (CMN07_ExportRoundTrip): n/a

#### `const ProcessNodeInfo * findNode(const std::vector< ProcessNodeInfo > &nodes, const std::string &id)`
- Source: `tests/process/test_cmmn_serializer.cpp`:71
- Brief: n/a
- Parameters:
  - `nodes` (const std::vector< ProcessNodeInfo > &): n/a
  - `id` (const std::string &): n/a

### test_fim_importer.cpp

#### `TEST(FimImporter, FIM01_SingleModelValid)`
- Source: `tests/process/test_fim_importer.cpp`:72
- Brief: n/a
- Parameters:
  - `<unnamed>` (FimImporter): n/a
  - `<unnamed>` (FIM01_SingleModelValid): n/a

#### `TEST(FimImporter, FIM02_SingleModelEmptyXml)`
- Source: `tests/process/test_fim_importer.cpp`:97
- Brief: n/a
- Parameters:
  - `<unnamed>` (FimImporter): n/a
  - `<unnamed>` (FIM02_SingleModelEmptyXml): n/a

#### `TEST(FimImporter, FIM03_SingleModelInvalidXml)`
- Source: `tests/process/test_fim_importer.cpp`:105
- Brief: n/a
- Parameters:
  - `<unnamed>` (FimImporter): n/a
  - `<unnamed>` (FIM03_SingleModelInvalidXml): n/a

#### `TEST(FimImporter, FIM04_CatalogueImport)`
- Source: `tests/process/test_fim_importer.cpp`:112
- Brief: n/a
- Parameters:
  - `<unnamed>` (FimImporter): n/a
  - `<unnamed>` (FIM04_CatalogueImport): n/a

#### `TEST(FimImporter, FIM05_LeikaKeyTagSet)`
- Source: `tests/process/test_fim_importer.cpp`:126
- Brief: n/a
- Parameters:
  - `<unnamed>` (FimImporter): n/a
  - `<unnamed>` (FIM05_LeikaKeyTagSet): n/a

#### `TEST(FimImporter, FIM06_PlainBpmnAsCatalogue)`
- Source: `tests/process/test_fim_importer.cpp`:147
- Brief: n/a
- Parameters:
  - `<unnamed>` (FimImporter): n/a
  - `<unnamed>` (FIM06_PlainBpmnAsCatalogue): n/a

#### `TEST(FimImporter, FIM07_FitkoApiReturnsError)`
- Source: `tests/process/test_fim_importer.cpp`:157
- Brief: n/a
- Parameters:
  - `<unnamed>` (FimImporter): n/a
  - `<unnamed>` (FIM07_FitkoApiReturnsError): n/a

#### `TEST(FimImporter, FIM08_FitkoApiWithInjectedFetchFn)`
- Source: `tests/process/test_fim_importer.cpp`:170
- Brief: n/a
- Parameters:
  - `<unnamed>` (FimImporter): n/a
  - `<unnamed>` (FIM08_FitkoApiWithInjectedFetchFn): n/a

### test_object_centric_tracer.cpp

#### `TEST_F(ObjectCentricTracerTest, OCT01_EmptyAttachments)`
- Source: `tests/process/test_object_centric_tracer.cpp`:78
- Brief: n/a
- Parameters:
  - `<unnamed>` (ObjectCentricTracerTest): n/a
  - `<unnamed>` (OCT01_EmptyAttachments): n/a

#### `TEST_F(ObjectCentricTracerTest, OCT02_ThreeAttachments)`
- Source: `tests/process/test_object_centric_tracer.cpp`:92
- Brief: n/a
- Parameters:
  - `<unnamed>` (ObjectCentricTracerTest): n/a
  - `<unnamed>` (OCT02_ThreeAttachments): n/a

#### `TEST_F(ObjectCentricTracerTest, OCT03_RequiredKeys)`
- Source: `tests/process/test_object_centric_tracer.cpp`:117
- Brief: n/a
- Parameters:
  - `<unnamed>` (ObjectCentricTracerTest): n/a
  - `<unnamed>` (OCT03_RequiredKeys): n/a

#### `TEST_F(ObjectCentricTracerTest, OCT04_EmptyModelEmptyArcs)`
- Source: `tests/process/test_object_centric_tracer.cpp`:130
- Brief: n/a
- Parameters:
  - `<unnamed>` (ObjectCentricTracerTest): n/a
  - `<unnamed>` (OCT04_EmptyModelEmptyArcs): n/a

#### `TEST_F(ObjectCentricTracerTest, OCT05_ThreeNodeLinearModel)`
- Source: `tests/process/test_object_centric_tracer.cpp`:140
- Brief: n/a
- Parameters:
  - `<unnamed>` (ObjectCentricTracerTest): n/a
  - `<unnamed>` (OCT05_ThreeNodeLinearModel): n/a

#### `TEST_F(ObjectCentricTracerTest, OCT06_NoBranchingNoDivergence)`
- Source: `tests/process/test_object_centric_tracer.cpp`:161
- Brief: n/a
- Parameters:
  - `<unnamed>` (ObjectCentricTracerTest): n/a
  - `<unnamed>` (OCT06_NoBranchingNoDivergence): n/a

#### `TEST_F(ObjectCentricTracerTest, OCT07_ConvergenceNode)`
- Source: `tests/process/test_object_centric_tracer.cpp`:180
- Brief: n/a
- Parameters:
  - `<unnamed>` (ObjectCentricTracerTest): n/a
  - `<unnamed>` (OCT07_ConvergenceNode): n/a

#### `TEST_F(ObjectCentricTracerTest, OCT08_DivergenceNode)`
- Source: `tests/process/test_object_centric_tracer.cpp`:204
- Brief: n/a
- Parameters:
  - `<unnamed>` (ObjectCentricTracerTest): n/a
  - `<unnamed>` (OCT08_DivergenceNode): n/a

#### `TEST_F(ObjectCentricTracerTest, OCT09_EventFieldsPreserved)`
- Source: `tests/process/test_object_centric_tracer.cpp`:228
- Brief: n/a
- Parameters:
  - `<unnamed>` (ObjectCentricTracerTest): n/a
  - `<unnamed>` (OCT09_EventFieldsPreserved): n/a

#### `TEST_F(ObjectCentricTracerTest, OCT10_UniqueArcs)`
- Source: `tests/process/test_object_centric_tracer.cpp`:248
- Brief: n/a
- Parameters:
  - `<unnamed>` (ObjectCentricTracerTest): n/a
  - `<unnamed>` (OCT10_UniqueArcs): n/a

### test_process_community_detector.cpp

#### `TEST_F(ProcessCommunityDetectorTest, LCD01_EmptyGraph)`
- Source: `tests/process/test_process_community_detector.cpp`:76
- Brief: n/a
- Parameters:
  - `<unnamed>` (ProcessCommunityDetectorTest): n/a
  - `<unnamed>` (LCD01_EmptyGraph): n/a

#### `TEST_F(ProcessCommunityDetectorTest, LCD02_SingleNode)`
- Source: `tests/process/test_process_community_detector.cpp`:86
- Brief: n/a
- Parameters:
  - `<unnamed>` (ProcessCommunityDetectorTest): n/a
  - `<unnamed>` (LCD02_SingleNode): n/a

#### `TEST_F(ProcessCommunityDetectorTest, LCD03_TwoDisconnectedNodes)`
- Source: `tests/process/test_process_community_detector.cpp`:99
- Brief: n/a
- Parameters:
  - `<unnamed>` (ProcessCommunityDetectorTest): n/a
  - `<unnamed>` (LCD03_TwoDisconnectedNodes): n/a

#### `TEST_F(ProcessCommunityDetectorTest, LCD04_LinearChain)`
- Source: `tests/process/test_process_community_detector.cpp`:112
- Brief: n/a
- Parameters:
  - `<unnamed>` (ProcessCommunityDetectorTest): n/a
  - `<unnamed>` (LCD04_LinearChain): n/a

#### `TEST_F(ProcessCommunityDetectorTest, LCD05_TwoCliques)`
- Source: `tests/process/test_process_community_detector.cpp`:133
- Brief: n/a
- Parameters:
  - `<unnamed>` (ProcessCommunityDetectorTest): n/a
  - `<unnamed>` (LCD05_TwoCliques): n/a

#### `TEST_F(ProcessCommunityDetectorTest, LCD06_HigherResolutionMoreCommunities)`
- Source: `tests/process/test_process_community_detector.cpp`:158
- Brief: n/a
- Parameters:
  - `<unnamed>` (ProcessCommunityDetectorTest): n/a
  - `<unnamed>` (LCD06_HigherResolutionMoreCommunities): n/a

#### `TEST_F(ProcessCommunityDetectorTest, LCD07_PersistAndLoad)`
- Source: `tests/process/test_process_community_detector.cpp`:183
- Brief: n/a
- Parameters:
  - `<unnamed>` (ProcessCommunityDetectorTest): n/a
  - `<unnamed>` (LCD07_PersistAndLoad): n/a

#### `TEST_F(ProcessCommunityDetectorTest, LCD08_CommunityIdsUnique)`
- Source: `tests/process/test_process_community_detector.cpp`:206
- Brief: n/a
- Parameters:
  - `<unnamed>` (ProcessCommunityDetectorTest): n/a
  - `<unnamed>` (LCD08_CommunityIdsUnique): n/a

#### `TEST_F(ProcessCommunityDetectorTest, LCD09_GenerateReportNonEmpty)`
- Source: `tests/process/test_process_community_detector.cpp`:229
- Brief: n/a
- Parameters:
  - `<unnamed>` (ProcessCommunityDetectorTest): n/a
  - `<unnamed>` (LCD09_GenerateReportNonEmpty): n/a

#### `TEST_F(ProcessCommunityDetectorTest, LCD10_PerformanceTwentyNodes)`
- Source: `tests/process/test_process_community_detector.cpp`:242
- Brief: n/a
- Parameters:
  - `<unnamed>` (ProcessCommunityDetectorTest): n/a
  - `<unnamed>` (LCD10_PerformanceTwentyNodes): n/a

#### `TEST_F(ProcessCommunityDetectorTest, LCD11_GenerateReport_IncludesModularityAndCount)`
- Source: `tests/process/test_process_community_detector.cpp`:267
- Brief: n/a
- Parameters:
  - `<unnamed>` (ProcessCommunityDetectorTest): n/a
  - `<unnamed>` (LCD11_GenerateReport_IncludesModularityAndCount): n/a

#### `TEST_F(ProcessCommunityDetectorTest, LCD12_GenerateReport_EllipsisForLargeCommunit)`
- Source: `tests/process/test_process_community_detector.cpp`:294
- Brief: n/a
- Parameters:
  - `<unnamed>` (ProcessCommunityDetectorTest): n/a
  - `<unnamed>` (LCD12_GenerateReport_EllipsisForLargeCommunit): n/a

### test_process_concurrency_churn_focused.cpp

#### `TEST_F(ConcurrencyChurnTest, C01_ConcurrentAtomicIncrement)`
- Source: `tests/process/test_process_concurrency_churn_focused.cpp`:35
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConcurrencyChurnTest): n/a
  - `<unnamed>` (C01_ConcurrentAtomicIncrement): n/a

#### `TEST_F(ConcurrencyChurnTest, C02_ConcurrentLinkCreation)`
- Source: `tests/process/test_process_concurrency_churn_focused.cpp`:64
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConcurrencyChurnTest): n/a
  - `<unnamed>` (C02_ConcurrentLinkCreation): n/a

#### `TEST_F(ConcurrencyChurnTest, C03_ConcurrentAttachmentCreation)`
- Source: `tests/process/test_process_concurrency_churn_focused.cpp`:112
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConcurrencyChurnTest): n/a
  - `<unnamed>` (C03_ConcurrentAttachmentCreation): n/a

#### `TEST_F(ConcurrencyChurnTest, C04_ReadHeavyConcurrentAccess)`
- Source: `tests/process/test_process_concurrency_churn_focused.cpp`:162
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConcurrencyChurnTest): n/a
  - `<unnamed>` (C04_ReadHeavyConcurrentAccess): n/a

#### `TEST_F(ConcurrencyChurnTest, C05_SingleWriterMultipleReaders)`
- Source: `tests/process/test_process_concurrency_churn_focused.cpp`:209
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConcurrencyChurnTest): n/a
  - `<unnamed>` (C05_SingleWriterMultipleReaders): n/a

#### `TEST_F(ConcurrencyChurnTest, C06_ConcurrentErrorEnumAccess)`
- Source: `tests/process/test_process_concurrency_churn_focused.cpp`:262
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConcurrencyChurnTest): n/a
  - `<unnamed>` (C06_ConcurrentErrorEnumAccess): n/a

#### `TEST_F(ConcurrencyChurnTest, C07_BarrierSynchronization)`
- Source: `tests/process/test_process_concurrency_churn_focused.cpp`:307
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConcurrencyChurnTest): n/a
  - `<unnamed>` (C07_BarrierSynchronization): n/a

#### `TEST_F(ConcurrencyChurnTest, C08_HighContentionLinkOperations)`
- Source: `tests/process/test_process_concurrency_churn_focused.cpp`:359
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConcurrencyChurnTest): n/a
  - `<unnamed>` (C08_HighContentionLinkOperations): n/a

#### `TEST_F(ConcurrencyChurnTest, C09_ConcurrentReadModifyWrite)`
- Source: `tests/process/test_process_concurrency_churn_focused.cpp`:418
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConcurrencyChurnTest): n/a
  - `<unnamed>` (C09_ConcurrentReadModifyWrite): n/a

#### `TEST_F(ConcurrencyChurnTest, C10_TryLockWithFallback)`
- Source: `tests/process/test_process_concurrency_churn_focused.cpp`:461
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConcurrencyChurnTest): n/a
  - `<unnamed>` (C10_TryLockWithFallback): n/a

#### `TEST_F(ConcurrencyChurnTest, C11_OrderedLockAcquisitionDeadlockPrevention)`
- Source: `tests/process/test_process_concurrency_churn_focused.cpp`:508
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConcurrencyChurnTest): n/a
  - `<unnamed>` (C11_OrderedLockAcquisitionDeadlockPrevention): n/a

#### `TEST_F(ConcurrencyChurnTest, C12_HighContentionLockCycles)`
- Source: `tests/process/test_process_concurrency_churn_focused.cpp`:548
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConcurrencyChurnTest): n/a
  - `<unnamed>` (C12_HighContentionLockCycles): n/a

### test_process_contract_hardening_focused.cpp

#### `TEST(BpmnImportErrorHandlingTest, P3_EmptyInputReturnsEmptyInputError)`
- Source: `tests/process/test_process_contract_hardening_focused.cpp`:431
- Brief: n/a
- Parameters:
  - `<unnamed>` (BpmnImportErrorHandlingTest): n/a
  - `<unnamed>` (P3_EmptyInputReturnsEmptyInputError): n/a

#### `TEST(BpmnImportErrorHandlingTest, P3_FileNotFoundReturnsFileReadError)`
- Source: `tests/process/test_process_contract_hardening_focused.cpp`:470
- Brief: n/a
- Parameters:
  - `<unnamed>` (BpmnImportErrorHandlingTest): n/a
  - `<unnamed>` (P3_FileNotFoundReturnsFileReadError): n/a

#### `TEST(BpmnImportErrorHandlingTest, P3_MalformedXmlReturnsMalformedInputError)`
- Source: `tests/process/test_process_contract_hardening_focused.cpp`:454
- Brief: n/a
- Parameters:
  - `<unnamed>` (BpmnImportErrorHandlingTest): n/a
  - `<unnamed>` (P3_MalformedXmlReturnsMalformedInputError): n/a

#### `TEST(BpmnImportErrorHandlingTest, P3_NoElementsReturnsEmptyInputError)`
- Source: `tests/process/test_process_contract_hardening_focused.cpp`:462
- Brief: n/a
- Parameters:
  - `<unnamed>` (BpmnImportErrorHandlingTest): n/a
  - `<unnamed>` (P3_NoElementsReturnsEmptyInputError): n/a

#### `TEST(BpmnImportErrorHandlingTest, P3_NoSilentFailures)`
- Source: `tests/process/test_process_contract_hardening_focused.cpp`:487
- Brief: n/a
- Parameters:
  - `<unnamed>` (BpmnImportErrorHandlingTest): n/a
  - `<unnamed>` (P3_NoSilentFailures): n/a

#### `TEST(BpmnImportErrorHandlingTest, P3_OversizedInputReturnsInputTooLargeError)`
- Source: `tests/process/test_process_contract_hardening_focused.cpp`:441
- Brief: n/a
- Parameters:
  - `<unnamed>` (BpmnImportErrorHandlingTest): n/a
  - `<unnamed>` (P3_OversizedInputReturnsInputTooLargeError): n/a

#### `TEST(BpmnImportErrorHandlingTest, P3_SuccessfulImportReturnsValidResult)`
- Source: `tests/process/test_process_contract_hardening_focused.cpp`:477
- Brief: n/a
- Parameters:
  - `<unnamed>` (BpmnImportErrorHandlingTest): n/a
  - `<unnamed>` (P3_SuccessfulImportReturnsValidResult): n/a

#### `TEST(BpmnSerializerHardeningTest, PRC30_ValidateEmptyNodes)`
- Source: `tests/process/test_process_contract_hardening_focused.cpp`:260
- Brief: n/a
- Parameters:
  - `<unnamed>` (BpmnSerializerHardeningTest): n/a
  - `<unnamed>` (PRC30_ValidateEmptyNodes): n/a

#### `TEST(BpmnSerializerHardeningTest, PRC31_ValidateNodeWithEmptyId)`
- Source: `tests/process/test_process_contract_hardening_focused.cpp`:268
- Brief: n/a
- Parameters:
  - `<unnamed>` (BpmnSerializerHardeningTest): n/a
  - `<unnamed>` (PRC31_ValidateNodeWithEmptyId): n/a

#### `TEST(BpmnSerializerHardeningTest, PRC32_ValidateDuplicateNodeIds)`
- Source: `tests/process/test_process_contract_hardening_focused.cpp`:281
- Brief: n/a
- Parameters:
  - `<unnamed>` (BpmnSerializerHardeningTest): n/a
  - `<unnamed>` (PRC32_ValidateDuplicateNodeIds): n/a

#### `TEST(BpmnSerializerHardeningTest, PRC33_ValidateDanglingEdge)`
- Source: `tests/process/test_process_contract_hardening_focused.cpp`:297
- Brief: n/a
- Parameters:
  - `<unnamed>` (BpmnSerializerHardeningTest): n/a
  - `<unnamed>` (PRC33_ValidateDanglingEdge): n/a

#### `TEST(BpmnSerializerHardeningTest, PRC34_ValidateExcessiveNodes)`
- Source: `tests/process/test_process_contract_hardening_focused.cpp`:316
- Brief: n/a
- Parameters:
  - `<unnamed>` (BpmnSerializerHardeningTest): n/a
  - `<unnamed>` (PRC34_ValidateExcessiveNodes): n/a

#### `TEST(BpmnSerializerHardeningTest, PRC35_ValidateValidStructure)`
- Source: `tests/process/test_process_contract_hardening_focused.cpp`:331
- Brief: n/a
- Parameters:
  - `<unnamed>` (BpmnSerializerHardeningTest): n/a
  - `<unnamed>` (PRC35_ValidateValidStructure): n/a

#### `TEST(ProcessContractTest, PRC01_ErrorCodesUnique)`
- Source: `tests/process/test_process_contract_hardening_focused.cpp`:34
- Brief: n/a
- Parameters:
  - `<unnamed>` (ProcessContractTest): n/a
  - `<unnamed>` (PRC01_ErrorCodesUnique): n/a

#### `TEST(ProcessContractTest, PRC02_ErrorCodesInRange)`
- Source: `tests/process/test_process_contract_hardening_focused.cpp`:46
- Brief: n/a
- Parameters:
  - `<unnamed>` (ProcessContractTest): n/a
  - `<unnamed>` (PRC02_ErrorCodesInRange): n/a

#### `TEST(ProcessContractTest, PRC03_UnsupportedDistinctFromInvalidTransition)`
- Source: `tests/process/test_process_contract_hardening_focused.cpp`:58
- Brief: n/a
- Parameters:
  - `<unnamed>` (ProcessContractTest): n/a
  - `<unnamed>` (PRC03_UnsupportedDistinctFromInvalidTransition): n/a

#### `TEST(ProcessContractTest, PRC04_SerialiserDistinctFromDeserialiser)`
- Source: `tests/process/test_process_contract_hardening_focused.cpp`:63
- Brief: n/a
- Parameters:
  - `<unnamed>` (ProcessContractTest): n/a
  - `<unnamed>` (PRC04_SerialiserDistinctFromDeserialiser): n/a

#### `TEST(ProcessContractTest, PRC05_ExecutionTimeoutIsHighestCode)`
- Source: `tests/process/test_process_contract_hardening_focused.cpp`:68
- Brief: n/a
- Parameters:
  - `<unnamed>` (ProcessContractTest): n/a
  - `<unnamed>` (PRC05_ExecutionTimeoutIsHighestCode): n/a

#### `TEST(ProcessContractTest, PRC06_ErrorSwitchDispatch)`
- Source: `tests/process/test_process_contract_hardening_focused.cpp`:74
- Brief: n/a
- Parameters:
  - `<unnamed>` (ProcessContractTest): n/a
  - `<unnamed>` (PRC06_ErrorSwitchDispatch): n/a

#### `TEST(ProcessContractTest, PRC07_UnsupportedElementLowestCode)`
- Source: `tests/process/test_process_contract_hardening_focused.cpp`:87
- Brief: n/a
- Parameters:
  - `<unnamed>` (ProcessContractTest): n/a
  - `<unnamed>` (PRC07_UnsupportedElementLowestCode): n/a

#### `TEST(ProcessContractTest, PRC08_ExecutionTimeoutCode)`
- Source: `tests/process/test_process_contract_hardening_focused.cpp`:92
- Brief: n/a
- Parameters:
  - `<unnamed>` (ProcessContractTest): n/a
  - `<unnamed>` (PRC08_ExecutionTimeoutCode): n/a

#### `TEST(ProcessDiagnosticsTest, P3_DiagnosticsAreActionable)`
- Source: `tests/process/test_process_contract_hardening_focused.cpp`:524
- Brief: n/a
- Parameters:
  - `<unnamed>` (ProcessDiagnosticsTest): n/a
  - `<unnamed>` (P3_DiagnosticsAreActionable): n/a

#### `TEST(ProcessDiagnosticsTest, P3_DiagnosticsIncludeActionableContext)`
- Source: `tests/process/test_process_contract_hardening_focused.cpp`:506
- Brief: n/a
- Parameters:
  - `<unnamed>` (ProcessDiagnosticsTest): n/a
  - `<unnamed>` (P3_DiagnosticsIncludeActionableContext): n/a

#### `TEST(ProcessErrorTaxonomyTest, P3_DiagnosticMessageFormatting)`
- Source: `tests/process/test_process_contract_hardening_focused.cpp`:406
- Brief: n/a
- Parameters:
  - `<unnamed>` (ProcessErrorTaxonomyTest): n/a
  - `<unnamed>` (P3_DiagnosticMessageFormatting): n/a

#### `TEST(ProcessErrorTaxonomyTest, P3_DiagnosticMessageWithoutDetail)`
- Source: `tests/process/test_process_contract_hardening_focused.cpp`:419
- Brief: n/a
- Parameters:
  - `<unnamed>` (ProcessErrorTaxonomyTest): n/a
  - `<unnamed>` (P3_DiagnosticMessageWithoutDetail): n/a

#### `TEST(ProcessErrorTaxonomyTest, P3_ErrorCodeCategoryMapping)`
- Source: `tests/process/test_process_contract_hardening_focused.cpp`:367
- Brief: n/a
- Parameters:
  - `<unnamed>` (ProcessErrorTaxonomyTest): n/a
  - `<unnamed>` (P3_ErrorCodeCategoryMapping): n/a

#### `TEST(ProcessErrorTaxonomyTest, P3_ErrorCodeRanges)`
- Source: `tests/process/test_process_contract_hardening_focused.cpp`:386
- Brief: n/a
- Parameters:
  - `<unnamed>` (ProcessErrorTaxonomyTest): n/a
  - `<unnamed>` (P3_ErrorCodeRanges): n/a

#### `TEST(ProcessErrorTaxonomyTest, P3_ErrorCodeToStringMapping)`
- Source: `tests/process/test_process_contract_hardening_focused.cpp`:358
- Brief: n/a
- Parameters:
  - `<unnamed>` (ProcessErrorTaxonomyTest): n/a
  - `<unnamed>` (P3_ErrorCodeToStringMapping): n/a

#### `TEST_F(ProcessModelHardeningTest, PRC20_ValidateEmptyId)`
- Source: `tests/process/test_process_contract_hardening_focused.cpp`:127
- Brief: n/a
- Parameters:
  - `<unnamed>` (ProcessModelHardeningTest): n/a
  - `<unnamed>` (PRC20_ValidateEmptyId): n/a

#### `TEST_F(ProcessModelHardeningTest, PRC21_ValidateEmptyName)`
- Source: `tests/process/test_process_contract_hardening_focused.cpp`:141
- Brief: n/a
- Parameters:
  - `<unnamed>` (ProcessModelHardeningTest): n/a
  - `<unnamed>` (PRC21_ValidateEmptyName): n/a

#### `TEST_F(ProcessModelHardeningTest, PRC22_ValidateValidModel)`
- Source: `tests/process/test_process_contract_hardening_focused.cpp`:155
- Brief: n/a
- Parameters:
  - `<unnamed>` (ProcessModelHardeningTest): n/a
  - `<unnamed>` (PRC22_ValidateValidModel): n/a

#### `TEST_F(ProcessModelHardeningTest, PRC23_ValidateDuplicateNodeIds)`
- Source: `tests/process/test_process_contract_hardening_focused.cpp`:180
- Brief: n/a
- Parameters:
  - `<unnamed>` (ProcessModelHardeningTest): n/a
  - `<unnamed>` (PRC23_ValidateDuplicateNodeIds): n/a

#### `TEST_F(ProcessModelHardeningTest, PRC24_ValidateDanglingEdge)`
- Source: `tests/process/test_process_contract_hardening_focused.cpp`:199
- Brief: n/a
- Parameters:
  - `<unnamed>` (ProcessModelHardeningTest): n/a
  - `<unnamed>` (PRC24_ValidateDanglingEdge): n/a

#### `TEST_F(ProcessModelHardeningTest, PRC25_ValidateNameLengthLimit)`
- Source: `tests/process/test_process_contract_hardening_focused.cpp`:221
- Brief: n/a
- Parameters:
  - `<unnamed>` (ProcessModelHardeningTest): n/a
  - `<unnamed>` (PRC25_ValidateNameLengthLimit): n/a

#### `TEST_F(ProcessModelHardeningTest, PRC26_ConsistencyDiagnostics)`
- Source: `tests/process/test_process_contract_hardening_focused.cpp`:235
- Brief: n/a
- Parameters:
  - `<unnamed>` (ProcessModelHardeningTest): n/a
  - `<unnamed>` (PRC26_ConsistencyDiagnostics): n/a

### test_process_determinism_conflict_focused.cpp

#### `TEST_F(DeterminismConflictTest, D01_IdenticalRngSequences)`
- Source: `tests/process/test_process_determinism_conflict_focused.cpp`:44
- Brief: n/a
- Parameters:
  - `<unnamed>` (DeterminismConflictTest): n/a
  - `<unnamed>` (D01_IdenticalRngSequences): n/a

#### `TEST_F(DeterminismConflictTest, D02_DeterministicLinkOrdering)`
- Source: `tests/process/test_process_determinism_conflict_focused.cpp`:68
- Brief: n/a
- Parameters:
  - `<unnamed>` (DeterminismConflictTest): n/a
  - `<unnamed>` (D02_DeterministicLinkOrdering): n/a

#### `TEST_F(DeterminismConflictTest, D03_SortedCollectionsDeterministic)`
- Source: `tests/process/test_process_determinism_conflict_focused.cpp`:99
- Brief: n/a
- Parameters:
  - `<unnamed>` (DeterminismConflictTest): n/a
  - `<unnamed>` (D03_SortedCollectionsDeterministic): n/a

#### `TEST_F(DeterminismConflictTest, D04_ConflictResolutionDeterministic)`
- Source: `tests/process/test_process_determinism_conflict_focused.cpp`:122
- Brief: n/a
- Parameters:
  - `<unnamed>` (DeterminismConflictTest): n/a
  - `<unnamed>` (D04_ConflictResolutionDeterministic): n/a

#### `TEST_F(DeterminismConflictTest, D05_DeterministicTimestampSequences)`
- Source: `tests/process/test_process_determinism_conflict_focused.cpp`:158
- Brief: n/a
- Parameters:
  - `<unnamed>` (DeterminismConflictTest): n/a
  - `<unnamed>` (D05_DeterministicTimestampSequences): n/a

#### `TEST_F(DeterminismConflictTest, D06_ModelStateTransitionsDeterministic)`
- Source: `tests/process/test_process_determinism_conflict_focused.cpp`:182
- Brief: n/a
- Parameters:
  - `<unnamed>` (DeterminismConflictTest): n/a
  - `<unnamed>` (D06_ModelStateTransitionsDeterministic): n/a

#### `TEST_F(DeterminismConflictTest, D07_PartialStateRecoveryDeterministic)`
- Source: `tests/process/test_process_determinism_conflict_focused.cpp`:236
- Brief: n/a
- Parameters:
  - `<unnamed>` (DeterminismConflictTest): n/a
  - `<unnamed>` (D07_PartialStateRecoveryDeterministic): n/a

#### `TEST_F(DeterminismConflictTest, D08_HashBasedSelectionDeterministic)`
- Source: `tests/process/test_process_determinism_conflict_focused.cpp`:284
- Brief: n/a
- Parameters:
  - `<unnamed>` (DeterminismConflictTest): n/a
  - `<unnamed>` (D08_HashBasedSelectionDeterministic): n/a

#### `TEST_F(DeterminismConflictTest, D09_DeterministicEventOrdering)`
- Source: `tests/process/test_process_determinism_conflict_focused.cpp`:316
- Brief: n/a
- Parameters:
  - `<unnamed>` (DeterminismConflictTest): n/a
  - `<unnamed>` (D09_DeterministicEventOrdering): n/a

#### `TEST_F(DeterminismConflictTest, D10_ReproducibleAttachmentLifecycle)`
- Source: `tests/process/test_process_determinism_conflict_focused.cpp`:370
- Brief: n/a
- Parameters:
  - `<unnamed>` (DeterminismConflictTest): n/a
  - `<unnamed>` (D10_ReproducibleAttachmentLifecycle): n/a

#### `TEST_F(DeterminismConflictTest, D11_DeterministicVersionClock)`
- Source: `tests/process/test_process_determinism_conflict_focused.cpp`:426
- Brief: n/a
- Parameters:
  - `<unnamed>` (DeterminismConflictTest): n/a
  - `<unnamed>` (D11_DeterministicVersionClock): n/a

#### `TEST_F(DeterminismConflictTest, D12_DeterministicConflictWinnerSelection)`
- Source: `tests/process/test_process_determinism_conflict_focused.cpp`:466
- Brief: n/a
- Parameters:
  - `<unnamed>` (DeterminismConflictTest): n/a
  - `<unnamed>` (D12_DeterministicConflictWinnerSelection): n/a

### test_process_diagnostics_incident_focused.cpp

#### `TEST_F(DiagnosticsIncidentTest, G01_DiagnosticRecordIncidentType)`
- Source: `tests/process/test_process_diagnostics_incident_focused.cpp`:36
- Brief: n/a
- Parameters:
  - `<unnamed>` (DiagnosticsIncidentTest): n/a
  - `<unnamed>` (G01_DiagnosticRecordIncidentType): n/a

#### `TEST_F(DiagnosticsIncidentTest, G02_DiagnosticRecordOperationAndInput)`
- Source: `tests/process/test_process_diagnostics_incident_focused.cpp`:53
- Brief: n/a
- Parameters:
  - `<unnamed>` (DiagnosticsIncidentTest): n/a
  - `<unnamed>` (G02_DiagnosticRecordOperationAndInput): n/a

#### `TEST_F(DiagnosticsIncidentTest, G03_DiagnosticRecordActionableMessage)`
- Source: `tests/process/test_process_diagnostics_incident_focused.cpp`:70
- Brief: n/a
- Parameters:
  - `<unnamed>` (DiagnosticsIncidentTest): n/a
  - `<unnamed>` (G03_DiagnosticRecordActionableMessage): n/a

#### `TEST_F(DiagnosticsIncidentTest, G04_DiagnosticRecordTimestamp)`
- Source: `tests/process/test_process_diagnostics_incident_focused.cpp`:88
- Brief: n/a
- Parameters:
  - `<unnamed>` (DiagnosticsIncidentTest): n/a
  - `<unnamed>` (G04_DiagnosticRecordTimestamp): n/a

#### `TEST_F(DiagnosticsIncidentTest, G05_DistinctIncidentTypes)`
- Source: `tests/process/test_process_diagnostics_incident_focused.cpp`:108
- Brief: n/a
- Parameters:
  - `<unnamed>` (DiagnosticsIncidentTest): n/a
  - `<unnamed>` (G05_DistinctIncidentTypes): n/a

#### `TEST_F(DiagnosticsIncidentTest, G06_ErrorCodesOrthogonalToIncidentTypes)`
- Source: `tests/process/test_process_diagnostics_incident_focused.cpp`:153
- Brief: n/a
- Parameters:
  - `<unnamed>` (DiagnosticsIncidentTest): n/a
  - `<unnamed>` (G06_ErrorCodesOrthogonalToIncidentTypes): n/a

#### `TEST_F(DiagnosticsIncidentTest, G07_FormattedMessageContainsKeyFields)`
- Source: `tests/process/test_process_diagnostics_incident_focused.cpp`:185
- Brief: n/a
- Parameters:
  - `<unnamed>` (DiagnosticsIncidentTest): n/a
  - `<unnamed>` (G07_FormattedMessageContainsKeyFields): n/a

#### `TEST_F(DiagnosticsIncidentTest, G08_ResourceIncidentCoverageAllLimits)`
- Source: `tests/process/test_process_diagnostics_incident_focused.cpp`:207
- Brief: n/a
- Parameters:
  - `<unnamed>` (DiagnosticsIncidentTest): n/a
  - `<unnamed>` (G08_ResourceIncidentCoverageAllLimits): n/a

### test_process_graph.cpp

#### `TEST_F(EdgeTypeRegistryTest, BuiltinTypesRegistered)`
- Source: `tests/process/test_process_graph.cpp`:22
- Brief: n/a
- Parameters:
  - `<unnamed>` (EdgeTypeRegistryTest): n/a
  - `<unnamed>` (BuiltinTypesRegistered): n/a

#### `TEST_F(EdgeTypeRegistryTest, CategoryConversion)`
- Source: `tests/process/test_process_graph.cpp`:128
- Brief: n/a
- Parameters:
  - `<unnamed>` (EdgeTypeRegistryTest): n/a
  - `<unnamed>` (CategoryConversion): n/a

#### `TEST_F(EdgeTypeRegistryTest, GetInverseType)`
- Source: `tests/process/test_process_graph.cpp`:69
- Brief: n/a
- Parameters:
  - `<unnamed>` (EdgeTypeRegistryTest): n/a
  - `<unnamed>` (GetInverseType): n/a

#### `TEST_F(EdgeTypeRegistryTest, GetTypeInfo)`
- Source: `tests/process/test_process_graph.cpp`:57
- Brief: n/a
- Parameters:
  - `<unnamed>` (EdgeTypeRegistryTest): n/a
  - `<unnamed>` (GetTypeInfo): n/a

#### `TEST_F(EdgeTypeRegistryTest, GetTypesByCategory)`
- Source: `tests/process/test_process_graph.cpp`:87
- Brief: n/a
- Parameters:
  - `<unnamed>` (EdgeTypeRegistryTest): n/a
  - `<unnamed>` (GetTypesByCategory): n/a

#### `TEST_F(EdgeTypeRegistryTest, HelperFunctions)`
- Source: `tests/process/test_process_graph.cpp`:140
- Brief: n/a
- Parameters:
  - `<unnamed>` (EdgeTypeRegistryTest): n/a
  - `<unnamed>` (HelperFunctions): n/a

#### `TEST_F(EdgeTypeRegistryTest, RegisterCustomType)`
- Source: `tests/process/test_process_graph.cpp`:104
- Brief: n/a
- Parameters:
  - `<unnamed>` (EdgeTypeRegistryTest): n/a
  - `<unnamed>` (RegisterCustomType): n/a

#### `TEST_F(ProcessGraphTest, AddBPMNNodes)`
- Source: `tests/process/test_process_graph.cpp`:202
- Brief: n/a
- Parameters:
  - `<unnamed>` (ProcessGraphTest): n/a
  - `<unnamed>` (AddBPMNNodes): n/a

#### `TEST_F(ProcessGraphTest, AddEPKNodes)`
- Source: `tests/process/test_process_graph.cpp`:237
- Brief: n/a
- Parameters:
  - `<unnamed>` (ProcessGraphTest): n/a
  - `<unnamed>` (AddEPKNodes): n/a

#### `TEST_F(ProcessGraphTest, AddHyperedge)`
- Source: `tests/process/test_process_graph.cpp`:303
- Brief: n/a
- Parameters:
  - `<unnamed>` (ProcessGraphTest): n/a
  - `<unnamed>` (AddHyperedge): n/a

#### `TEST_F(ProcessGraphTest, AddProcessEdges)`
- Source: `tests/process/test_process_graph.cpp`:271
- Brief: n/a
- Parameters:
  - `<unnamed>` (ProcessGraphTest): n/a
  - `<unnamed>` (AddProcessEdges): n/a

#### `TEST_F(ProcessGraphTest, AdvanceToken)`
- Source: `tests/process/test_process_graph.cpp`:418
- Brief: n/a
- Parameters:
  - `<unnamed>` (ProcessGraphTest): n/a
  - `<unnamed>` (AdvanceToken): n/a

#### `TEST_F(ProcessGraphTest, AqlQueryExecutorInjection_aggregateByField)`
- Source: `tests/process/test_process_graph.cpp`:1161
- Brief: n/a
- Parameters:
  - `<unnamed>` (ProcessGraphTest): n/a
  - `<unnamed>` (AqlQueryExecutorInjection_aggregateByField): n/a

#### `TEST_F(ProcessGraphTest, AqlQueryExecutorInjection_queryTasksByFormData)`
- Source: `tests/process/test_process_graph.cpp`:1124
- Brief: n/a
- Parameters:
  - `<unnamed>` (ProcessGraphTest): n/a
  - `<unnamed>` (AqlQueryExecutorInjection_queryTasksByFormData): n/a

#### `TEST_F(ProcessGraphTest, ConditionEvaluation)`
- Source: `tests/process/test_process_graph.cpp`:528
- Brief: n/a
- Parameters:
  - `<unnamed>` (ProcessGraphTest): n/a
  - `<unnamed>` (ConditionEvaluation): n/a

#### `TEST_F(ProcessGraphTest, CriticalPath)`
- Source: `tests/process/test_process_graph.cpp`:851
- Brief: n/a
- Parameters:
  - `<unnamed>` (ProcessGraphTest): n/a
  - `<unnamed>` (CriticalPath): n/a

#### `TEST_F(ProcessGraphTest, EventHandling)`
- Source: `tests/process/test_process_graph.cpp`:613
- Brief: n/a
- Parameters:
  - `<unnamed>` (ProcessGraphTest): n/a
  - `<unnamed>` (EventHandling): n/a

#### `TEST_F(ProcessGraphTest, GetVisitTimestampReturnsNulloptForUnvisitedNode)`
- Source: `tests/process/test_process_graph.cpp`:1038
- Brief: n/a
- Parameters:
  - `<unnamed>` (ProcessGraphTest): n/a
  - `<unnamed>` (GetVisitTimestampReturnsNulloptForUnvisitedNode): n/a

#### `TEST_F(ProcessGraphTest, HyperedgeReadiness)`
- Source: `tests/process/test_process_graph.cpp`:909
- Brief: n/a
- Parameters:
  - `<unnamed>` (ProcessGraphTest): n/a
  - `<unnamed>` (HyperedgeReadiness): n/a

#### `TEST_F(ProcessGraphTest, HyperedgeStatus)`
- Source: `tests/process/test_process_graph.cpp`:879
- Brief: n/a
- Parameters:
  - `<unnamed>` (ProcessGraphTest): n/a
  - `<unnamed>` (HyperedgeStatus): n/a

#### `TEST_F(ProcessGraphTest, NodeHistory)`
- Source: `tests/process/test_process_graph.cpp`:756
- Brief: n/a
- Parameters:
  - `<unnamed>` (ProcessGraphTest): n/a
  - `<unnamed>` (NodeHistory): n/a

#### `TEST_F(ProcessGraphTest, ProcessEdgeTypesRegistered)`
- Source: `tests/process/test_process_graph.cpp`:503
- Brief: n/a
- Parameters:
  - `<unnamed>` (ProcessGraphTest): n/a
  - `<unnamed>` (ProcessEdgeTypesRegistered): n/a

#### `TEST_F(ProcessGraphTest, ProcessMetrics)`
- Source: `tests/process/test_process_graph.cpp`:796
- Brief: n/a
- Parameters:
  - `<unnamed>` (ProcessGraphTest): n/a
  - `<unnamed>` (ProcessMetrics): n/a

#### `TEST_F(ProcessGraphTest, RegisterProcess)`
- Source: `tests/process/test_process_graph.cpp`:197
- Brief: n/a
- Parameters:
  - `<unnamed>` (ProcessGraphTest): n/a
  - `<unnamed>` (RegisterProcess): n/a

#### `TEST_F(ProcessGraphTest, StartAndGetProcessInstance)`
- Source: `tests/process/test_process_graph.cpp`:382
- Brief: n/a
- Parameters:
  - `<unnamed>` (ProcessGraphTest): n/a
  - `<unnamed>` (StartAndGetProcessInstance): n/a

#### `TEST_F(ProcessGraphTest, SuspendResumeProcess)`
- Source: `tests/process/test_process_graph.cpp`:456
- Brief: n/a
- Parameters:
  - `<unnamed>` (ProcessGraphTest): n/a
  - `<unnamed>` (SuspendResumeProcess): n/a

#### `TEST_F(ProcessGraphTest, TaskAssignmentQueries)`
- Source: `tests/process/test_process_graph.cpp`:696
- Brief: n/a
- Parameters:
  - `<unnamed>` (ProcessGraphTest): n/a
  - `<unnamed>` (TaskAssignmentQueries): n/a

#### `TEST_F(ProcessGraphTest, TerminateProcess)`
- Source: `tests/process/test_process_graph.cpp`:482
- Brief: n/a
- Parameters:
  - `<unnamed>` (ProcessGraphTest): n/a
  - `<unnamed>` (TerminateProcess): n/a

#### `TEST_F(ProcessGraphTest, ValidateProcess_MissingStart)`
- Source: `tests/process/test_process_graph.cpp`:353
- Brief: n/a
- Parameters:
  - `<unnamed>` (ProcessGraphTest): n/a
  - `<unnamed>` (ValidateProcess_MissingStart): n/a

#### `TEST_F(ProcessGraphTest, ValidateProcess_ValidProcess)`
- Source: `tests/process/test_process_graph.cpp`:330
- Brief: n/a
- Parameters:
  - `<unnamed>` (ProcessGraphTest): n/a
  - `<unnamed>` (ValidateProcess_ValidProcess): n/a

#### `TEST_F(ProcessGraphTest, VisitTimestampPopulatedOnStartProcess)`
- Source: `tests/process/test_process_graph.cpp`:942
- Brief: n/a
- Parameters:
  - `<unnamed>` (ProcessGraphTest): n/a
  - `<unnamed>` (VisitTimestampPopulatedOnStartProcess): n/a

#### `TEST_F(ProcessGraphTest, VisitTimestampsPersistedAfterTokenCompletion)`
- Source: `tests/process/test_process_graph.cpp`:1054
- Brief: n/a
- Parameters:
  - `<unnamed>` (ProcessGraphTest): n/a
  - `<unnamed>` (VisitTimestampsPersistedAfterTokenCompletion): n/a

#### `TEST_F(ProcessGraphTest, VisitTimestampsPopulatedAndOrderedAcrossMultiHopTraversal)`
- Source: `tests/process/test_process_graph.cpp`:961
- Brief: n/a
- Parameters:
  - `<unnamed>` (ProcessGraphTest): n/a
  - `<unnamed>` (VisitTimestampsPopulatedAndOrderedAcrossMultiHopTraversal): n/a

### test_process_highcardinality_stress.cpp

#### `TEST_F(ProcessHighCardinalityStress, ConcurrentSignalStress)`
- Source: `tests/process/test_process_highcardinality_stress.cpp`:164
- Brief: n/a
- Parameters:
  - `<unnamed>` (ProcessHighCardinalityStress): n/a
  - `<unnamed>` (ConcurrentSignalStress): n/a

#### `TEST_F(ProcessHighCardinalityStress, HighCardinalityProcessSpawn)`
- Source: `tests/process/test_process_highcardinality_stress.cpp`:128
- Brief: n/a
- Parameters:
  - `<unnamed>` (ProcessHighCardinalityStress): n/a
  - `<unnamed>` (HighCardinalityProcessSpawn): n/a

#### `TEST_F(ProcessHighCardinalityStress, ResourceExhaustionBehavior)`
- Source: `tests/process/test_process_highcardinality_stress.cpp`:210
- Brief: n/a
- Parameters:
  - `<unnamed>` (ProcessHighCardinalityStress): n/a
  - `<unnamed>` (ResourceExhaustionBehavior): n/a

### test_process_light_retriever.cpp

#### `TEST_F(ProcessLightRetrieverTest, PLR01_AutoGlobalKeyword)`
- Source: `tests/process/test_process_light_retriever.cpp`:104
- Brief: n/a
- Parameters:
  - `<unnamed>` (ProcessLightRetrieverTest): n/a
  - `<unnamed>` (PLR01_AutoGlobalKeyword): n/a

#### `TEST_F(ProcessLightRetrieverTest, PLR02_AutoLocalKeyword)`
- Source: `tests/process/test_process_light_retriever.cpp`:128
- Brief: n/a
- Parameters:
  - `<unnamed>` (ProcessLightRetrieverTest): n/a
  - `<unnamed>` (PLR02_AutoLocalKeyword): n/a

#### `TEST_F(ProcessLightRetrieverTest, PLR03_GlobalWithPersistedCommunities)`
- Source: `tests/process/test_process_light_retriever.cpp`:144
- Brief: n/a
- Parameters:
  - `<unnamed>` (ProcessLightRetrieverTest): n/a
  - `<unnamed>` (PLR03_GlobalWithPersistedCommunities): n/a

#### `TEST_F(ProcessLightRetrieverTest, PLR04_GlobalFallbackToLocal)`
- Source: `tests/process/test_process_light_retriever.cpp`:170
- Brief: n/a
- Parameters:
  - `<unnamed>` (ProcessLightRetrieverTest): n/a
  - `<unnamed>` (PLR04_GlobalFallbackToLocal): n/a

#### `TEST_F(ProcessLightRetrieverTest, PLR05_LocalModeExplicit)`
- Source: `tests/process/test_process_light_retriever.cpp`:188
- Brief: n/a
- Parameters:
  - `<unnamed>` (ProcessLightRetrieverTest): n/a
  - `<unnamed>` (PLR05_LocalModeExplicit): n/a

#### `TEST_F(ProcessLightRetrieverTest, PLR06_NonEmptyLlmContext)`
- Source: `tests/process/test_process_light_retriever.cpp`:205
- Brief: n/a
- Parameters:
  - `<unnamed>` (ProcessLightRetrieverTest): n/a
  - `<unnamed>` (PLR06_NonEmptyLlmContext): n/a

#### `TEST_F(ProcessLightRetrieverTest, PLR07_UsedModeMatchesRequested)`
- Source: `tests/process/test_process_light_retriever.cpp`:223
- Brief: n/a
- Parameters:
  - `<unnamed>` (ProcessLightRetrieverTest): n/a
  - `<unnamed>` (PLR07_UsedModeMatchesRequested): n/a

#### `TEST_F(ProcessLightRetrieverTest, PLR08_ClassifyQueryCaseInsensitive)`
- Source: `tests/process/test_process_light_retriever.cpp`:251
- Brief: n/a
- Parameters:
  - `<unnamed>` (ProcessLightRetrieverTest): n/a
  - `<unnamed>` (PLR08_ClassifyQueryCaseInsensitive): n/a

### test_process_linker_edge_focused.cpp

#### `TEST_F(LinkerEdgeTest, L01_CyclicLinkDetection)`
- Source: `tests/process/test_process_linker_edge_focused.cpp`:65
- Brief: n/a
- Parameters:
  - `<unnamed>` (LinkerEdgeTest): n/a
  - `<unnamed>` (L01_CyclicLinkDetection): n/a

#### `TEST_F(LinkerEdgeTest, L02_MissingTargetInLinkReference)`
- Source: `tests/process/test_process_linker_edge_focused.cpp`:127
- Brief: n/a
- Parameters:
  - `<unnamed>` (LinkerEdgeTest): n/a
  - `<unnamed>` (L02_MissingTargetInLinkReference): n/a

#### `TEST_F(LinkerEdgeTest, L03_MissingSourceInLinkReference)`
- Source: `tests/process/test_process_linker_edge_focused.cpp`:151
- Brief: n/a
- Parameters:
  - `<unnamed>` (LinkerEdgeTest): n/a
  - `<unnamed>` (L03_MissingSourceInLinkReference): n/a

#### `TEST_F(LinkerEdgeTest, L04_PartialLinkStateRecovery)`
- Source: `tests/process/test_process_linker_edge_focused.cpp`:175
- Brief: n/a
- Parameters:
  - `<unnamed>` (LinkerEdgeTest): n/a
  - `<unnamed>` (L04_PartialLinkStateRecovery): n/a

#### `TEST_F(LinkerEdgeTest, L05_AttachmentWithoutMatchingInstance)`
- Source: `tests/process/test_process_linker_edge_focused.cpp`:221
- Brief: n/a
- Parameters:
  - `<unnamed>` (LinkerEdgeTest): n/a
  - `<unnamed>` (L05_AttachmentWithoutMatchingInstance): n/a

#### `TEST_F(LinkerEdgeTest, L06_ValidLinkTypeEnumeration)`
- Source: `tests/process/test_process_linker_edge_focused.cpp`:245
- Brief: n/a
- Parameters:
  - `<unnamed>` (LinkerEdgeTest): n/a
  - `<unnamed>` (L06_ValidLinkTypeEnumeration): n/a

#### `TEST_F(LinkerEdgeTest, L07_BrokenLinkChain)`
- Source: `tests/process/test_process_linker_edge_focused.cpp`:272
- Brief: n/a
- Parameters:
  - `<unnamed>` (LinkerEdgeTest): n/a
  - `<unnamed>` (L07_BrokenLinkChain): n/a

#### `TEST_F(LinkerEdgeTest, L08_MultipleTypedLinksBetweenEntities)`
- Source: `tests/process/test_process_linker_edge_focused.cpp`:308
- Brief: n/a
- Parameters:
  - `<unnamed>` (LinkerEdgeTest): n/a
  - `<unnamed>` (L08_MultipleTypedLinksBetweenEntities): n/a

### test_process_mining_v2.cpp

#### `TEST(ProcessMiningStructsTest, PM2_01_EventLogStatistics)`
- Source: `tests/process/test_process_mining_v2.cpp`:125
- Brief: n/a
- Parameters:
  - `<unnamed>` (ProcessMiningStructsTest): n/a
  - `<unnamed>` (PM2_01_EventLogStatistics): n/a

#### `TEST(ProcessMiningStructsTest, PM2_16_StatusHelpers)`
- Source: `tests/process/test_process_mining_v2.cpp`:373
- Brief: n/a
- Parameters:
  - `<unnamed>` (ProcessMiningStructsTest): n/a
  - `<unnamed>` (PM2_16_StatusHelpers): n/a

#### `TEST_F(ProcessMiningTest, PM2_02_CreateDFG_Activities_Present)`
- Source: `tests/process/test_process_mining_v2.cpp`:136
- Brief: n/a
- Parameters:
  - `<unnamed>` (ProcessMiningTest): n/a
  - `<unnamed>` (PM2_02_CreateDFG_Activities_Present): n/a

#### `TEST_F(ProcessMiningTest, PM2_03_CreateDFG_SelfLoop)`
- Source: `tests/process/test_process_mining_v2.cpp`:160
- Brief: n/a
- Parameters:
  - `<unnamed>` (ProcessMiningTest): n/a
  - `<unnamed>` (PM2_03_CreateDFG_SelfLoop): n/a

#### `TEST_F(ProcessMiningTest, PM2_04_DiscoverProcess_Alpha)`
- Source: `tests/process/test_process_mining_v2.cpp`:192
- Brief: n/a
- Parameters:
  - `<unnamed>` (ProcessMiningTest): n/a
  - `<unnamed>` (PM2_04_DiscoverProcess_Alpha): n/a

#### `TEST_F(ProcessMiningTest, PM2_05_DiscoverProcess_Heuristic)`
- Source: `tests/process/test_process_mining_v2.cpp`:203
- Brief: n/a
- Parameters:
  - `<unnamed>` (ProcessMiningTest): n/a
  - `<unnamed>` (PM2_05_DiscoverProcess_Heuristic): n/a

#### `TEST_F(ProcessMiningTest, PM2_06_DiscoverProcess_Inductive)`
- Source: `tests/process/test_process_mining_v2.cpp`:215
- Brief: n/a
- Parameters:
  - `<unnamed>` (ProcessMiningTest): n/a
  - `<unnamed>` (PM2_06_DiscoverProcess_Inductive): n/a

#### `TEST_F(ProcessMiningTest, PM2_07_AnalyzeVariants_TwoVariants)`
- Source: `tests/process/test_process_mining_v2.cpp`:226
- Brief: n/a
- Parameters:
  - `<unnamed>` (ProcessMiningTest): n/a
  - `<unnamed>` (PM2_07_AnalyzeVariants_TwoVariants): n/a

#### `TEST_F(ProcessMiningTest, PM2_08_AnalyzeVariants_TopN)`
- Source: `tests/process/test_process_mining_v2.cpp`:242
- Brief: n/a
- Parameters:
  - `<unnamed>` (ProcessMiningTest): n/a
  - `<unnamed>` (PM2_08_AnalyzeVariants_TopN): n/a

#### `TEST_F(ProcessMiningTest, PM2_09_ClusterVariants)`
- Source: `tests/process/test_process_mining_v2.cpp`:252
- Brief: n/a
- Parameters:
  - `<unnamed>` (ProcessMiningTest): n/a
  - `<unnamed>` (PM2_09_ClusterVariants): n/a

#### `TEST_F(ProcessMiningTest, PM2_10_Conformance_ExactReplay)`
- Source: `tests/process/test_process_mining_v2.cpp`:263
- Brief: n/a
- Parameters:
  - `<unnamed>` (ProcessMiningTest): n/a
  - `<unnamed>` (PM2_10_Conformance_ExactReplay): n/a

#### `TEST_F(ProcessMiningTest, PM2_11_Conformance_Deviations)`
- Source: `tests/process/test_process_mining_v2.cpp`:278
- Brief: n/a
- Parameters:
  - `<unnamed>` (ProcessMiningTest): n/a
  - `<unnamed>` (PM2_11_Conformance_Deviations): n/a

#### `TEST_F(ProcessMiningTest, PM2_12_Enhance_NodeStats)`
- Source: `tests/process/test_process_mining_v2.cpp`:311
- Brief: n/a
- Parameters:
  - `<unnamed>` (ProcessMiningTest): n/a
  - `<unnamed>` (PM2_12_Enhance_NodeStats): n/a

#### `TEST_F(ProcessMiningTest, PM2_13_DetectBottlenecks)`
- Source: `tests/process/test_process_mining_v2.cpp`:326
- Brief: n/a
- Parameters:
  - `<unnamed>` (ProcessMiningTest): n/a
  - `<unnamed>` (PM2_13_DetectBottlenecks): n/a

#### `TEST_F(ProcessMiningTest, PM2_14_ExportToBPMN)`
- Source: `tests/process/test_process_mining_v2.cpp`:342
- Brief: n/a
- Parameters:
  - `<unnamed>` (ProcessMiningTest): n/a
  - `<unnamed>` (PM2_14_ExportToBPMN): n/a

#### `TEST_F(ProcessMiningTest, PM2_15_ExportToPNML)`
- Source: `tests/process/test_process_mining_v2.cpp`:356
- Brief: n/a
- Parameters:
  - `<unnamed>` (ProcessMiningTest): n/a
  - `<unnamed>` (PM2_15_ExportToPNML): n/a

#### `EventLog makeSimpleLog()`
- Source: `tests/process/test_process_mining_v2.cpp`:41
- Brief: n/a
- Parameters: none

#### `EventLog makeTwoVariantLog()`
- Source: `tests/process/test_process_mining_v2.cpp`:66
- Brief: n/a
- Parameters: none

### test_process_module.cpp

#### `TEST(ProcessEnumsTest, ProcessDomainRoundTrip)`
- Source: `tests/process/test_process_module.cpp`:710
- Brief: n/a
- Parameters:
  - `<unnamed>` (ProcessEnumsTest): n/a
  - `<unnamed>` (ProcessDomainRoundTrip): n/a

#### `TEST(ProcessEnumsTest, ProcessLinkTypeRoundTrip)`
- Source: `tests/process/test_process_module.cpp`:674
- Brief: n/a
- Parameters:
  - `<unnamed>` (ProcessEnumsTest): n/a
  - `<unnamed>` (ProcessLinkTypeRoundTrip): n/a

#### `TEST(ProcessEnumsTest, ProcessNotationRoundTrip)`
- Source: `tests/process/test_process_module.cpp`:693
- Brief: n/a
- Parameters:
  - `<unnamed>` (ProcessEnumsTest): n/a
  - `<unnamed>` (ProcessNotationRoundTrip): n/a

#### `TEST_F(BpmnSerializerTest, ExportProducesXml)`
- Source: `tests/process/test_process_module.cpp`:110
- Brief: n/a
- Parameters:
  - `<unnamed>` (BpmnSerializerTest): n/a
  - `<unnamed>` (ExportProducesXml): n/a

#### `TEST_F(BpmnSerializerTest, ImportAllGatewayTypes)`
- Source: `tests/process/test_process_module.cpp`:798
- Brief: n/a
- Parameters:
  - `<unnamed>` (BpmnSerializerTest): n/a
  - `<unnamed>` (ImportAllGatewayTypes): n/a

#### `TEST_F(BpmnSerializerTest, ImportConditionExpressionChild)`
- Source: `tests/process/test_process_module.cpp`:758
- Brief: n/a
- Parameters:
  - `<unnamed>` (BpmnSerializerTest): n/a
  - `<unnamed>` (ImportConditionExpressionChild): n/a

#### `TEST_F(BpmnSerializerTest, ImportDeeplyNestedSubProcesses)`
- Source: `tests/process/test_process_module.cpp`:787
- Brief: n/a
- Parameters:
  - `<unnamed>` (BpmnSerializerTest): n/a
  - `<unnamed>` (ImportDeeplyNestedSubProcesses): n/a

#### `TEST_F(BpmnSerializerTest, ImportEmptyReturnsFalse)`
- Source: `tests/process/test_process_module.cpp`:105
- Brief: n/a
- Parameters:
  - `<unnamed>` (BpmnSerializerTest): n/a
  - `<unnamed>` (ImportEmptyReturnsFalse): n/a

#### `TEST_F(BpmnSerializerTest, ImportMessageFlow)`
- Source: `tests/process/test_process_module.cpp`:855
- Brief: n/a
- Parameters:
  - `<unnamed>` (BpmnSerializerTest): n/a
  - `<unnamed>` (ImportMessageFlow): n/a

#### `TEST_F(BpmnSerializerTest, ImportMinimalBpmn)`
- Source: `tests/process/test_process_module.cpp`:94
- Brief: n/a
- Parameters:
  - `<unnamed>` (BpmnSerializerTest): n/a
  - `<unnamed>` (ImportMinimalBpmn): n/a

#### `TEST_F(BpmnSerializerTest, ImportNamespacePrefixedBpmn)`
- Source: `tests/process/test_process_module.cpp`:734
- Brief: n/a
- Parameters:
  - `<unnamed>` (BpmnSerializerTest): n/a
  - `<unnamed>` (ImportNamespacePrefixedBpmn): n/a

#### `TEST_F(BpmnSerializerTest, ImportNestedSubProcess)`
- Source: `tests/process/test_process_module.cpp`:745
- Brief: n/a
- Parameters:
  - `<unnamed>` (BpmnSerializerTest): n/a
  - `<unnamed>` (ImportNestedSubProcess): n/a

#### `TEST_F(BpmnSerializerTest, ImportWithCommentsAndPIs)`
- Source: `tests/process/test_process_module.cpp`:776
- Brief: n/a
- Parameters:
  - `<unnamed>` (BpmnSerializerTest): n/a
  - `<unnamed>` (ImportWithCommentsAndPIs): n/a

#### `TEST_F(BpmnSerializerTest, PerformanceLargeBpmnImport)`
- Source: `tests/process/test_process_module.cpp`:871
- Brief: n/a
- Parameters:
  - `<unnamed>` (BpmnSerializerTest): n/a
  - `<unnamed>` (PerformanceLargeBpmnImport): n/a

#### `TEST_F(BpmnSerializerTest, RoundTrip)`
- Source: `tests/process/test_process_module.cpp`:138
- Brief: n/a
- Parameters:
  - `<unnamed>` (BpmnSerializerTest): n/a
  - `<unnamed>` (RoundTrip): n/a

#### `TEST_F(BpmnSerializerTest, SecurityCdataStrippedFromAttributes)`
- Source: `tests/process/test_process_module.cpp`:837
- Brief: n/a
- Parameters:
  - `<unnamed>` (BpmnSerializerTest): n/a
  - `<unnamed>` (SecurityCdataStrippedFromAttributes): n/a

#### `TEST_F(BpmnSerializerTest, SecurityMalformedXmlGraceful)`
- Source: `tests/process/test_process_module.cpp`:817
- Brief: n/a
- Parameters:
  - `<unnamed>` (BpmnSerializerTest): n/a
  - `<unnamed>` (SecurityMalformedXmlGraceful): n/a

#### `TEST_F(BpmnSerializerTest, SecurityOversizedInputRejected)`
- Source: `tests/process/test_process_module.cpp`:808
- Brief: n/a
- Parameters:
  - `<unnamed>` (BpmnSerializerTest): n/a
  - `<unnamed>` (SecurityOversizedInputRejected): n/a

#### `TEST_F(DmnEvaluatorTest, CollectHitPolicyReturnsAll)`
- Source: `tests/process/test_process_module.cpp`:1444
- Brief: n/a
- Parameters:
  - `<unnamed>` (DmnEvaluatorTest): n/a
  - `<unnamed>` (CollectHitPolicyReturnsAll): n/a

#### `TEST_F(DmnEvaluatorTest, FeelNullChecks)`
- Source: `tests/process/test_process_module.cpp`:1399
- Brief: n/a
- Parameters:
  - `<unnamed>` (DmnEvaluatorTest): n/a
  - `<unnamed>` (FeelNullChecks): n/a

#### `TEST_F(DmnEvaluatorTest, FeelNumericComparisons)`
- Source: `tests/process/test_process_module.cpp`:1370
- Brief: n/a
- Parameters:
  - `<unnamed>` (DmnEvaluatorTest): n/a
  - `<unnamed>` (FeelNumericComparisons): n/a

#### `TEST_F(DmnEvaluatorTest, FeelRangeExpressions)`
- Source: `tests/process/test_process_module.cpp`:1381
- Brief: n/a
- Parameters:
  - `<unnamed>` (DmnEvaluatorTest): n/a
  - `<unnamed>` (FeelRangeExpressions): n/a

#### `TEST_F(DmnEvaluatorTest, FeelStringEquality)`
- Source: `tests/process/test_process_module.cpp`:1392
- Brief: n/a
- Parameters:
  - `<unnamed>` (DmnEvaluatorTest): n/a
  - `<unnamed>` (FeelStringEquality): n/a

#### `TEST_F(DmnEvaluatorTest, FeelWildcardAlwaysMatches)`
- Source: `tests/process/test_process_module.cpp`:1363
- Brief: n/a
- Parameters:
  - `<unnamed>` (DmnEvaluatorTest): n/a
  - `<unnamed>` (FeelWildcardAlwaysMatches): n/a

#### `TEST_F(DmnEvaluatorTest, ListDecisionsReturnsAllIds)`
- Source: `tests/process/test_process_module.cpp`:1497
- Brief: n/a
- Parameters:
  - `<unnamed>` (DmnEvaluatorTest): n/a
  - `<unnamed>` (ListDecisionsReturnsAllIds): n/a

#### `TEST_F(DmnEvaluatorTest, LoadFromJsonUniqueHitPolicy)`
- Source: `tests/process/test_process_module.cpp`:1407
- Brief: n/a
- Parameters:
  - `<unnamed>` (DmnEvaluatorTest): n/a
  - `<unnamed>` (LoadFromJsonUniqueHitPolicy): n/a

#### `TEST_F(DmnEvaluatorTest, LoadFromXmlParsesDecisionTable)`
- Source: `tests/process/test_process_module.cpp`:1487
- Brief: n/a
- Parameters:
  - `<unnamed>` (DmnEvaluatorTest): n/a
  - `<unnamed>` (LoadFromXmlParsesDecisionTable): n/a

#### `TEST_F(DmnEvaluatorTest, NoMatchReturnsEmpty)`
- Source: `tests/process/test_process_module.cpp`:1467
- Brief: n/a
- Parameters:
  - `<unnamed>` (DmnEvaluatorTest): n/a
  - `<unnamed>` (NoMatchReturnsEmpty): n/a

#### `TEST_F(EpkSerializerTest, ImportEmptyReturnsFalse)`
- Source: `tests/process/test_process_module.cpp`:179
- Brief: n/a
- Parameters:
  - `<unnamed>` (EpkSerializerTest): n/a
  - `<unnamed>` (ImportEmptyReturnsFalse): n/a

#### `TEST_F(EpkSerializerTest, ImportExportFromJson)`
- Source: `tests/process/test_process_module.cpp`:184
- Brief: n/a
- Parameters:
  - `<unnamed>` (EpkSerializerTest): n/a
  - `<unnamed>` (ImportExportFromJson): n/a

#### `TEST_F(EpkSerializerTest, ImportMinimalEpk)`
- Source: `tests/process/test_process_module.cpp`:170
- Brief: n/a
- Parameters:
  - `<unnamed>` (EpkSerializerTest): n/a
  - `<unnamed>` (ImportMinimalEpk): n/a

#### `TEST_F(LlmDescriptorTest, BuildConformancePrompt)`
- Source: `tests/process/test_process_module.cpp`:293
- Brief: n/a
- Parameters:
  - `<unnamed>` (LlmDescriptorTest): n/a
  - `<unnamed>` (BuildConformancePrompt): n/a

#### `TEST_F(LlmDescriptorTest, GenerateDescriptor)`
- Source: `tests/process/test_process_module.cpp`:261
- Brief: n/a
- Parameters:
  - `<unnamed>` (LlmDescriptorTest): n/a
  - `<unnamed>` (GenerateDescriptor): n/a

#### `TEST_F(LlmDescriptorTest, GenerateSystemPrompt)`
- Source: `tests/process/test_process_module.cpp`:276
- Brief: n/a
- Parameters:
  - `<unnamed>` (LlmDescriptorTest): n/a
  - `<unnamed>` (GenerateSystemPrompt): n/a

#### `TEST_F(LlmDescriptorTest, GenerateSystemPromptEnglish)`
- Source: `tests/process/test_process_module.cpp`:285
- Brief: n/a
- Parameters:
  - `<unnamed>` (LlmDescriptorTest): n/a
  - `<unnamed>` (GenerateSystemPromptEnglish): n/a

#### `TEST_F(OcelExporterTest, ExportInstanceIncludesAttachments)`
- Source: `tests/process/test_process_module.cpp`:1560
- Brief: n/a
- Parameters:
  - `<unnamed>` (OcelExporterTest): n/a
  - `<unnamed>` (ExportInstanceIncludesAttachments): n/a

#### `TEST_F(OcelExporterTest, ExportInstanceProducesOcel2Structure)`
- Source: `tests/process/test_process_module.cpp`:1528
- Brief: n/a
- Parameters:
  - `<unnamed>` (OcelExporterTest): n/a
  - `<unnamed>` (ExportInstanceProducesOcel2Structure): n/a

#### `TEST_F(OcelExporterTest, ExportNonExistentInstanceReturnsEmpty)`
- Source: `tests/process/test_process_module.cpp`:1521
- Brief: n/a
- Parameters:
  - `<unnamed>` (OcelExporterTest): n/a
  - `<unnamed>` (ExportNonExistentInstanceReturnsEmpty): n/a

#### `TEST_F(OcelExporterTest, ObjectTypesAreDerived)`
- Source: `tests/process/test_process_module.cpp`:1587
- Brief: n/a
- Parameters:
  - `<unnamed>` (OcelExporterTest): n/a
  - `<unnamed>` (ObjectTypesAreDerived): n/a

#### `TEST_F(PprTest, EmptyGraphReturnsEmpty)`
- Source: `tests/process/test_process_module.cpp`:1002
- Brief: n/a
- Parameters:
  - `<unnamed>` (PprTest): n/a
  - `<unnamed>` (EmptyGraphReturnsEmpty): n/a

#### `TEST_F(PprTest, LinearChainSeedHasHighScore)`
- Source: `tests/process/test_process_module.cpp`:1030
- Brief: n/a
- Parameters:
  - `<unnamed>` (PprTest): n/a
  - `<unnamed>` (LinearChainSeedHasHighScore): n/a

#### `TEST_F(PprTest, PerformanceLargeGraph)`
- Source: `tests/process/test_process_module.cpp`:1174
- Brief: n/a
- Parameters:
  - `<unnamed>` (PprTest): n/a
  - `<unnamed>` (PerformanceLargeGraph): n/a

#### `TEST_F(PprTest, TopKRespected)`
- Source: `tests/process/test_process_module.cpp`:1099
- Brief: n/a
- Parameters:
  - `<unnamed>` (PprTest): n/a
  - `<unnamed>` (TopKRespected): n/a

#### `TEST_F(PprTest, UnknownSeedFallsBackToUniform)`
- Source: `tests/process/test_process_module.cpp`:1139
- Brief: n/a
- Parameters:
  - `<unnamed>` (PprTest): n/a
  - `<unnamed>` (UnknownSeedFallsBackToUniform): n/a

#### `TEST_F(ProcessGraphRagTest, BuildAdminProcessingPrompt)`
- Source: `tests/process/test_process_module.cpp`:638
- Brief: n/a
- Parameters:
  - `<unnamed>` (ProcessGraphRagTest): n/a
  - `<unnamed>` (BuildAdminProcessingPrompt): n/a

#### `TEST_F(ProcessGraphRagTest, BuildKnowledgeGraphFromModel)`
- Source: `tests/process/test_process_module.cpp`:587
- Brief: n/a
- Parameters:
  - `<unnamed>` (ProcessGraphRagTest): n/a
  - `<unnamed>` (BuildKnowledgeGraphFromModel): n/a

#### `TEST_F(ProcessGraphRagTest, BuildQueryPrompt)`
- Source: `tests/process/test_process_module.cpp`:659
- Brief: n/a
- Parameters:
  - `<unnamed>` (ProcessGraphRagTest): n/a
  - `<unnamed>` (BuildQueryPrompt): n/a

#### `TEST_F(ProcessGraphRagTest, CheckComplianceNonExistentInstance)`
- Source: `tests/process/test_process_module.cpp`:621
- Brief: n/a
- Parameters:
  - `<unnamed>` (ProcessGraphRagTest): n/a
  - `<unnamed>` (CheckComplianceNonExistentInstance): n/a

#### `TEST_F(ProcessGraphRagTest, ExtractSubgraph)`
- Source: `tests/process/test_process_module.cpp`:601
- Brief: n/a
- Parameters:
  - `<unnamed>` (ProcessGraphRagTest): n/a
  - `<unnamed>` (ExtractSubgraph): n/a

#### `TEST_F(ProcessGraphRagTest, RetrieveNonExistentInstance)`
- Source: `tests/process/test_process_module.cpp`:627
- Brief: n/a
- Parameters:
  - `<unnamed>` (ProcessGraphRagTest): n/a
  - `<unnamed>` (RetrieveNonExistentInstance): n/a

#### `TEST_F(ProcessGraphRagTest, SummarizeNonExistentInstance)`
- Source: `tests/process/test_process_module.cpp`:614
- Brief: n/a
- Parameters:
  - `<unnamed>` (ProcessGraphRagTest): n/a
  - `<unnamed>` (SummarizeNonExistentInstance): n/a

#### `TEST_F(ProcessModelGeneratorTest, FromLlmJsonProducesRecord)`
- Source: `tests/process/test_process_module.cpp`:1288
- Brief: n/a
- Parameters:
  - `<unnamed>` (ProcessModelGeneratorTest): n/a
  - `<unnamed>` (FromLlmJsonProducesRecord): n/a

#### `TEST_F(ProcessModelGeneratorTest, MockBackendGeneratesValidModel)`
- Source: `tests/process/test_process_module.cpp`:1336
- Brief: n/a
- Parameters:
  - `<unnamed>` (ProcessModelGeneratorTest): n/a
  - `<unnamed>` (MockBackendGeneratesValidModel): n/a

#### `TEST_F(ProcessModelGeneratorTest, NoBackendReturnsFalse)`
- Source: `tests/process/test_process_module.cpp`:1328
- Brief: n/a
- Parameters:
  - `<unnamed>` (ProcessModelGeneratorTest): n/a
  - `<unnamed>` (NoBackendReturnsFalse): n/a

#### `TEST_F(ProcessModelGeneratorTest, ValidateFailsIsolatedNode)`
- Source: `tests/process/test_process_module.cpp`:1270
- Brief: n/a
- Parameters:
  - `<unnamed>` (ProcessModelGeneratorTest): n/a
  - `<unnamed>` (ValidateFailsIsolatedNode): n/a

#### `TEST_F(ProcessModelGeneratorTest, ValidateFailsNoEnd)`
- Source: `tests/process/test_process_module.cpp`:1257
- Brief: n/a
- Parameters:
  - `<unnamed>` (ProcessModelGeneratorTest): n/a
  - `<unnamed>` (ValidateFailsNoEnd): n/a

#### `TEST_F(ProcessModelGeneratorTest, ValidateFailsNoStart)`
- Source: `tests/process/test_process_module.cpp`:1243
- Brief: n/a
- Parameters:
  - `<unnamed>` (ProcessModelGeneratorTest): n/a
  - `<unnamed>` (ValidateFailsNoStart): n/a

#### `TEST_F(ProcessModelGeneratorTest, ValidatePassesForMinimalGraph)`
- Source: `tests/process/test_process_module.cpp`:1226
- Brief: n/a
- Parameters:
  - `<unnamed>` (ProcessModelGeneratorTest): n/a
  - `<unnamed>` (ValidatePassesForMinimalGraph): n/a

#### `TEST_F(ProcessModuleTest, AttachAndRetrieve)`
- Source: `tests/process/test_process_module.cpp`:389
- Brief: n/a
- Parameters:
  - `<unnamed>` (ProcessModuleTest): n/a
  - `<unnamed>` (AttachAndRetrieve): n/a

#### `TEST_F(ProcessModuleTest, AttachFilterByType)`
- Source: `tests/process/test_process_module.cpp`:408
- Brief: n/a
- Parameters:
  - `<unnamed>` (ProcessModuleTest): n/a
  - `<unnamed>` (AttachFilterByType): n/a

#### `TEST_F(ProcessModuleTest, DeleteModel)`
- Source: `tests/process/test_process_module.cpp`:347
- Brief: n/a
- Parameters:
  - `<unnamed>` (ProcessModuleTest): n/a
  - `<unnamed>` (DeleteModel): n/a

#### `TEST_F(ProcessModuleTest, DetachObject)`
- Source: `tests/process/test_process_module.cpp`:423
- Brief: n/a
- Parameters:
  - `<unnamed>` (ProcessModuleTest): n/a
  - `<unnamed>` (DetachObject): n/a

#### `TEST_F(ProcessModuleTest, DetachObjectHardDelete)`
- Source: `tests/process/test_process_module.cpp`:910
- Brief: n/a
- Parameters:
  - `<unnamed>` (ProcessModuleTest): n/a
  - `<unnamed>` (DetachObjectHardDelete): n/a

#### `TEST_F(ProcessModuleTest, DetachObjectNonExistentReturnsFalse)`
- Source: `tests/process/test_process_module.cpp`:929
- Brief: n/a
- Parameters:
  - `<unnamed>` (ProcessModuleTest): n/a
  - `<unnamed>` (DetachObjectNonExistentReturnsFalse): n/a

#### `TEST_F(ProcessModuleTest, FindInstancesMultipleCollections)`
- Source: `tests/process/test_process_module.cpp`:962
- Brief: n/a
- Parameters:
  - `<unnamed>` (ProcessModuleTest): n/a
  - `<unnamed>` (FindInstancesMultipleCollections): n/a

#### `TEST_F(ProcessModuleTest, FindInstancesSecondaryIndexRoundTrip)`
- Source: `tests/process/test_process_module.cpp`:934
- Brief: n/a
- Parameters:
  - `<unnamed>` (ProcessModuleTest): n/a
  - `<unnamed>` (FindInstancesSecondaryIndexRoundTrip): n/a

#### `TEST_F(ProcessModuleTest, FindInstancesWithObject)`
- Source: `tests/process/test_process_module.cpp`:451
- Brief: n/a
- Parameters:
  - `<unnamed>` (ProcessModuleTest): n/a
  - `<unnamed>` (FindInstancesWithObject): n/a

#### `TEST_F(ProcessModuleTest, FindInstancesWithObjectEmpty)`
- Source: `tests/process/test_process_module.cpp`:956
- Brief: n/a
- Parameters:
  - `<unnamed>` (ProcessModuleTest): n/a
  - `<unnamed>` (FindInstancesWithObjectEmpty): n/a

#### `TEST_F(ProcessModuleTest, GetAttachmentsAfterHardDelete)`
- Source: `tests/process/test_process_module.cpp`:978
- Brief: n/a
- Parameters:
  - `<unnamed>` (ProcessModuleTest): n/a
  - `<unnamed>` (GetAttachmentsAfterHardDelete): n/a

#### `TEST_F(ProcessModuleTest, ImportBpmnViaManager)`
- Source: `tests/process/test_process_module.cpp`:362
- Brief: n/a
- Parameters:
  - `<unnamed>` (ProcessModuleTest): n/a
  - `<unnamed>` (ImportBpmnViaManager): n/a

#### `TEST_F(ProcessModuleTest, ImportVccVpbViaManager)`
- Source: `tests/process/test_process_module.cpp`:373
- Brief: n/a
- Parameters:
  - `<unnamed>` (ProcessModuleTest): n/a
  - `<unnamed>` (ImportVccVpbViaManager): n/a

#### `TEST_F(ProcessModuleTest, LinkProcesses)`
- Source: `tests/process/test_process_module.cpp`:465
- Brief: n/a
- Parameters:
  - `<unnamed>` (ProcessModuleTest): n/a
  - `<unnamed>` (LinkProcesses): n/a

#### `TEST_F(ProcessModuleTest, ListModels)`
- Source: `tests/process/test_process_module.cpp`:334
- Brief: n/a
- Parameters:
  - `<unnamed>` (ProcessModuleTest): n/a
  - `<unnamed>` (ListModels): n/a

#### `TEST_F(ProcessModuleTest, LoadNonExistentReturnsNullopt)`
- Source: `tests/process/test_process_module.cpp`:329
- Brief: n/a
- Parameters:
  - `<unnamed>` (ProcessModuleTest): n/a
  - `<unnamed>` (LoadNonExistentReturnsNullopt): n/a

#### `TEST_F(ProcessModuleTest, MissingDocumentsDetection)`
- Source: `tests/process/test_process_module.cpp`:492
- Brief: n/a
- Parameters:
  - `<unnamed>` (ProcessModuleTest): n/a
  - `<unnamed>` (MissingDocumentsDetection): n/a

#### `TEST_F(ProcessModuleTest, NoMissingDocumentsWhenAllPresent)`
- Source: `tests/process/test_process_module.cpp`:510
- Brief: n/a
- Parameters:
  - `<unnamed>` (ProcessModuleTest): n/a
  - `<unnamed>` (NoMissingDocumentsWhenAllPresent): n/a

#### `TEST_F(ProcessModuleTest, NodeAttachments)`
- Source: `tests/process/test_process_module.cpp`:436
- Brief: n/a
- Parameters:
  - `<unnamed>` (ProcessModuleTest): n/a
  - `<unnamed>` (NodeAttachments): n/a

#### `TEST_F(ProcessModuleTest, RequiredDocumentRegistry)`
- Source: `tests/process/test_process_module.cpp`:480
- Brief: n/a
- Parameters:
  - `<unnamed>` (ProcessModuleTest): n/a
  - `<unnamed>` (RequiredDocumentRegistry): n/a

#### `TEST_F(ProcessModuleTest, SaveAndLoad)`
- Source: `tests/process/test_process_module.cpp`:306
- Brief: n/a
- Parameters:
  - `<unnamed>` (ProcessModuleTest): n/a
  - `<unnamed>` (SaveAndLoad): n/a

#### `TEST_F(VccVpbImporterTest, ImportEmptyReturnsFalse)`
- Source: `tests/process/test_process_module.cpp`:229
- Brief: n/a
- Parameters:
  - `<unnamed>` (VccVpbImporterTest): n/a
  - `<unnamed>` (ImportEmptyReturnsFalse): n/a

#### `TEST_F(VccVpbImporterTest, ImportSingleModel)`
- Source: `tests/process/test_process_module.cpp`:217
- Brief: n/a
- Parameters:
  - `<unnamed>` (VccVpbImporterTest): n/a
  - `<unnamed>` (ImportSingleModel): n/a

### test_process_parser_edge_focused.cpp

#### `TEST_F(ParserEdgeTest, P01_EmptyBpmnInput)`
- Source: `tests/process/test_process_parser_edge_focused.cpp`:66
- Brief: n/a
- Parameters:
  - `<unnamed>` (ParserEdgeTest): n/a
  - `<unnamed>` (P01_EmptyBpmnInput): n/a

#### `TEST_F(ParserEdgeTest, P02_TruncatedBpmnFile)`
- Source: `tests/process/test_process_parser_edge_focused.cpp`:76
- Brief: n/a
- Parameters:
  - `<unnamed>` (ParserEdgeTest): n/a
  - `<unnamed>` (P02_TruncatedBpmnFile): n/a

#### `TEST_F(ParserEdgeTest, P03_MismatchedXmlTags)`
- Source: `tests/process/test_process_parser_edge_focused.cpp`:88
- Brief: n/a
- Parameters:
  - `<unnamed>` (ParserEdgeTest): n/a
  - `<unnamed>` (P03_MismatchedXmlTags): n/a

#### `TEST_F(ParserEdgeTest, P04_InvalidReference)`
- Source: `tests/process/test_process_parser_edge_focused.cpp`:100
- Brief: n/a
- Parameters:
  - `<unnamed>` (ParserEdgeTest): n/a
  - `<unnamed>` (P04_InvalidReference): n/a

#### `TEST_F(ParserEdgeTest, P05_CircularReferenceCycleDetection)`
- Source: `tests/process/test_process_parser_edge_focused.cpp`:127
- Brief: n/a
- Parameters:
  - `<unnamed>` (ParserEdgeTest): n/a
  - `<unnamed>` (P05_CircularReferenceCycleDetection): n/a

#### `TEST_F(ParserEdgeTest, P06_SelfLoopDetection)`
- Source: `tests/process/test_process_parser_edge_focused.cpp`:185
- Brief: n/a
- Parameters:
  - `<unnamed>` (ParserEdgeTest): n/a
  - `<unnamed>` (P06_SelfLoopDetection): n/a

#### `TEST_F(ParserEdgeTest, P07_DeeplyNestedXml)`
- Source: `tests/process/test_process_parser_edge_focused.cpp`:204
- Brief: n/a
- Parameters:
  - `<unnamed>` (ParserEdgeTest): n/a
  - `<unnamed>` (P07_DeeplyNestedXml): n/a

#### `TEST_F(ParserEdgeTest, P08_ExcessiveElementCount)`
- Source: `tests/process/test_process_parser_edge_focused.cpp`:257
- Brief: n/a
- Parameters:
  - `<unnamed>` (ParserEdgeTest): n/a
  - `<unnamed>` (P08_ExcessiveElementCount): n/a

#### `TEST_F(ParserEdgeTest, P09_InvalidCharacterEncoding)`
- Source: `tests/process/test_process_parser_edge_focused.cpp`:289
- Brief: n/a
- Parameters:
  - `<unnamed>` (ParserEdgeTest): n/a
  - `<unnamed>` (P09_InvalidCharacterEncoding): n/a

#### `TEST_F(ParserEdgeTest, P10_MissingRequiredAttributes)`
- Source: `tests/process/test_process_parser_edge_focused.cpp`:324
- Brief: n/a
- Parameters:
  - `<unnamed>` (ParserEdgeTest): n/a
  - `<unnamed>` (P10_MissingRequiredAttributes): n/a

#### `TEST_F(ParserEdgeTest, P11_InvalidAttributeDataType)`
- Source: `tests/process/test_process_parser_edge_focused.cpp`:358
- Brief: n/a
- Parameters:
  - `<unnamed>` (ParserEdgeTest): n/a
  - `<unnamed>` (P11_InvalidAttributeDataType): n/a

#### `TEST_F(ParserEdgeTest, P12_UnsupportedElementType)`
- Source: `tests/process/test_process_parser_edge_focused.cpp`:395
- Brief: n/a
- Parameters:
  - `<unnamed>` (ParserEdgeTest): n/a
  - `<unnamed>` (P12_UnsupportedElementType): n/a

#### `TEST_F(ParserEdgeTest, P13_ConflictingElementDefinitions)`
- Source: `tests/process/test_process_parser_edge_focused.cpp`:427
- Brief: n/a
- Parameters:
  - `<unnamed>` (ParserEdgeTest): n/a
  - `<unnamed>` (P13_ConflictingElementDefinitions): n/a

#### `TEST_F(ParserEdgeTest, P14_ProtocolVersionMismatch)`
- Source: `tests/process/test_process_parser_edge_focused.cpp`:470
- Brief: n/a
- Parameters:
  - `<unnamed>` (ParserEdgeTest): n/a
  - `<unnamed>` (P14_ProtocolVersionMismatch): n/a

#### `TEST_F(ParserEdgeTest, P15_NullMissingProcessDefinition)`
- Source: `tests/process/test_process_parser_edge_focused.cpp`:504
- Brief: n/a
- Parameters:
  - `<unnamed>` (ParserEdgeTest): n/a
  - `<unnamed>` (P15_NullMissingProcessDefinition): n/a

#### `TEST_F(ParserEdgeTest, P16_ParserTimeoutOnComplexInput)`
- Source: `tests/process/test_process_parser_edge_focused.cpp`:540
- Brief: n/a
- Parameters:
  - `<unnamed>` (ParserEdgeTest): n/a
  - `<unnamed>` (P16_ParserTimeoutOnComplexInput): n/a

### test_process_retriever_edge_focused.cpp

#### `TEST_F(RetrieverEdgeTest, R01_EmptyResultHandling)`
- Source: `tests/process/test_process_retriever_edge_focused.cpp`:110
- Brief: n/a
- Parameters:
  - `<unnamed>` (RetrieverEdgeTest): n/a
  - `<unnamed>` (R01_EmptyResultHandling): n/a

#### `TEST_F(RetrieverEdgeTest, R02_LargeGraphTraversalWithinBounds)`
- Source: `tests/process/test_process_retriever_edge_focused.cpp`:122
- Brief: n/a
- Parameters:
  - `<unnamed>` (RetrieverEdgeTest): n/a
  - `<unnamed>` (R02_LargeGraphTraversalWithinBounds): n/a

#### `TEST_F(RetrieverEdgeTest, R03_LargeContextTruncation)`
- Source: `tests/process/test_process_retriever_edge_focused.cpp`:136
- Brief: n/a
- Parameters:
  - `<unnamed>` (RetrieverEdgeTest): n/a
  - `<unnamed>` (R03_LargeContextTruncation): n/a

#### `TEST_F(RetrieverEdgeTest, R04_QueryTimeoutHandlingFast)`
- Source: `tests/process/test_process_retriever_edge_focused.cpp`:151
- Brief: n/a
- Parameters:
  - `<unnamed>` (RetrieverEdgeTest): n/a
  - `<unnamed>` (R04_QueryTimeoutHandlingFast): n/a

#### `TEST_F(RetrieverEdgeTest, R05_QueryTimeoutExceeded)`
- Source: `tests/process/test_process_retriever_edge_focused.cpp`:164
- Brief: n/a
- Parameters:
  - `<unnamed>` (RetrieverEdgeTest): n/a
  - `<unnamed>` (R05_QueryTimeoutExceeded): n/a

#### `TEST_F(RetrieverEdgeTest, R06_StaleLinkDetection)`
- Source: `tests/process/test_process_retriever_edge_focused.cpp`:179
- Brief: n/a
- Parameters:
  - `<unnamed>` (RetrieverEdgeTest): n/a
  - `<unnamed>` (R06_StaleLinkDetection): n/a

#### `TEST_F(RetrieverEdgeTest, R07_MalformedContextRejection)`
- Source: `tests/process/test_process_retriever_edge_focused.cpp`:193
- Brief: n/a
- Parameters:
  - `<unnamed>` (RetrieverEdgeTest): n/a
  - `<unnamed>` (R07_MalformedContextRejection): n/a

#### `TEST_F(RetrieverEdgeTest, R08_ConcurrentQueryIsolation)`
- Source: `tests/process/test_process_retriever_edge_focused.cpp`:205
- Brief: n/a
- Parameters:
  - `<unnamed>` (RetrieverEdgeTest): n/a
  - `<unnamed>` (R08_ConcurrentQueryIsolation): n/a

#### `TEST_F(RetrieverEdgeTest, R09_EmptySubgraphCommunityDetection)`
- Source: `tests/process/test_process_retriever_edge_focused.cpp`:243
- Brief: n/a
- Parameters:
  - `<unnamed>` (RetrieverEdgeTest): n/a
  - `<unnamed>` (R09_EmptySubgraphCommunityDetection): n/a

#### `TEST_F(RetrieverEdgeTest, R10_LocalModeEntityTraversal)`
- Source: `tests/process/test_process_retriever_edge_focused.cpp`:257
- Brief: n/a
- Parameters:
  - `<unnamed>` (RetrieverEdgeTest): n/a
  - `<unnamed>` (R10_LocalModeEntityTraversal): n/a

#### `TEST_F(RetrieverEdgeTest, R11_GlobalModeCommunityReport)`
- Source: `tests/process/test_process_retriever_edge_focused.cpp`:271
- Brief: n/a
- Parameters:
  - `<unnamed>` (RetrieverEdgeTest): n/a
  - `<unnamed>` (R11_GlobalModeCommunityReport): n/a

#### `TEST_F(RetrieverEdgeTest, R12_ContextTruncationBoundary)`
- Source: `tests/process/test_process_retriever_edge_focused.cpp`:285
- Brief: n/a
- Parameters:
  - `<unnamed>` (RetrieverEdgeTest): n/a
  - `<unnamed>` (R12_ContextTruncationBoundary): n/a

#### `TEST_F(RetrieverEdgeTest, R13_AutoRoutingKeywordClassification)`
- Source: `tests/process/test_process_retriever_edge_focused.cpp`:298
- Brief: n/a
- Parameters:
  - `<unnamed>` (RetrieverEdgeTest): n/a
  - `<unnamed>` (R13_AutoRoutingKeywordClassification): n/a

#### `TEST_F(RetrieverEdgeTest, R14_ResourceLimitCascadingDegradation)`
- Source: `tests/process/test_process_retriever_edge_focused.cpp`:320
- Brief: n/a
- Parameters:
  - `<unnamed>` (RetrieverEdgeTest): n/a
  - `<unnamed>` (R14_ResourceLimitCascadingDegradation): n/a

#### `TEST_F(RetrieverEdgeTest, R15_RetrievalLatencyConsistency)`
- Source: `tests/process/test_process_retriever_edge_focused.cpp`:340
- Brief: n/a
- Parameters:
  - `<unnamed>` (RetrieverEdgeTest): n/a
  - `<unnamed>` (R15_RetrievalLatencyConsistency): n/a

#### `TEST_F(RetrieverEdgeTest, R16_GracefulDegradationNoSilentFailure)`
- Source: `tests/process/test_process_retriever_edge_focused.cpp`:359
- Brief: n/a
- Parameters:
  - `<unnamed>` (RetrieverEdgeTest): n/a
  - `<unnamed>` (R16_GracefulDegradationNoSilentFailure): n/a

### test_process_retriever_resilience_focused.cpp

#### `TEST_F(RetrieverResilienceTest, R01_CacheMissHandling)`
- Source: `tests/process/test_process_retriever_resilience_focused.cpp`:96
- Brief: n/a
- Parameters:
  - `<unnamed>` (RetrieverResilienceTest): n/a
  - `<unnamed>` (R01_CacheMissHandling): n/a

#### `TEST_F(RetrieverResilienceTest, R02_CacheHitPerformance)`
- Source: `tests/process/test_process_retriever_resilience_focused.cpp`:110
- Brief: n/a
- Parameters:
  - `<unnamed>` (RetrieverResilienceTest): n/a
  - `<unnamed>` (R02_CacheHitPerformance): n/a

#### `TEST_F(RetrieverResilienceTest, R03_CacheOverflowRejection)`
- Source: `tests/process/test_process_retriever_resilience_focused.cpp`:126
- Brief: n/a
- Parameters:
  - `<unnamed>` (RetrieverResilienceTest): n/a
  - `<unnamed>` (R03_CacheOverflowRejection): n/a

#### `TEST_F(RetrieverResilienceTest, R04_MemoryExhaustionDetection)`
- Source: `tests/process/test_process_retriever_resilience_focused.cpp`:148
- Brief: n/a
- Parameters:
  - `<unnamed>` (RetrieverResilienceTest): n/a
  - `<unnamed>` (R04_MemoryExhaustionDetection): n/a

#### `TEST_F(RetrieverResilienceTest, R05_TimeoutDuringContextAssembly)`
- Source: `tests/process/test_process_retriever_resilience_focused.cpp`:169
- Brief: n/a
- Parameters:
  - `<unnamed>` (RetrieverResilienceTest): n/a
  - `<unnamed>` (R05_TimeoutDuringContextAssembly): n/a

#### `TEST_F(RetrieverResilienceTest, R06_GracefulDegradationNoCacheAvailable)`
- Source: `tests/process/test_process_retriever_resilience_focused.cpp`:198
- Brief: n/a
- Parameters:
  - `<unnamed>` (RetrieverResilienceTest): n/a
  - `<unnamed>` (R06_GracefulDegradationNoCacheAvailable): n/a

#### `TEST_F(RetrieverResilienceTest, R07_ConcurrentCacheReadHeavyAccess)`
- Source: `tests/process/test_process_retriever_resilience_focused.cpp`:239
- Brief: n/a
- Parameters:
  - `<unnamed>` (RetrieverResilienceTest): n/a
  - `<unnamed>` (R07_ConcurrentCacheReadHeavyAccess): n/a

#### `TEST_F(RetrieverResilienceTest, R08_CacheCoherencyUnderConcurrentUpdates)`
- Source: `tests/process/test_process_retriever_resilience_focused.cpp`:280
- Brief: n/a
- Parameters:
  - `<unnamed>` (RetrieverResilienceTest): n/a
  - `<unnamed>` (R08_CacheCoherencyUnderConcurrentUpdates): n/a

#### `TEST_F(RetrieverResilienceTest, R09_LruEvictionBehavior)`
- Source: `tests/process/test_process_retriever_resilience_focused.cpp`:321
- Brief: n/a
- Parameters:
  - `<unnamed>` (RetrieverResilienceTest): n/a
  - `<unnamed>` (R09_LruEvictionBehavior): n/a

#### `TEST_F(RetrieverResilienceTest, R10_StaleWhileRevalidateBehavior)`
- Source: `tests/process/test_process_retriever_resilience_focused.cpp`:390
- Brief: n/a
- Parameters:
  - `<unnamed>` (RetrieverResilienceTest): n/a
  - `<unnamed>` (R10_StaleWhileRevalidateBehavior): n/a

#### `TEST_F(RetrieverResilienceTest, R11_TimeoutWithFallbackStrategy)`
- Source: `tests/process/test_process_retriever_resilience_focused.cpp`:444
- Brief: n/a
- Parameters:
  - `<unnamed>` (RetrieverResilienceTest): n/a
  - `<unnamed>` (R11_TimeoutWithFallbackStrategy): n/a

#### `TEST_F(RetrieverResilienceTest, R12_CircuitBreakerPattern)`
- Source: `tests/process/test_process_retriever_resilience_focused.cpp`:483
- Brief: n/a
- Parameters:
  - `<unnamed>` (RetrieverResilienceTest): n/a
  - `<unnamed>` (R12_CircuitBreakerPattern): n/a

#### `TEST_F(RetrieverResilienceTest, R13_BoundedContextSizeEnforcement)`
- Source: `tests/process/test_process_retriever_resilience_focused.cpp`:541
- Brief: n/a
- Parameters:
  - `<unnamed>` (RetrieverResilienceTest): n/a
  - `<unnamed>` (R13_BoundedContextSizeEnforcement): n/a

#### `TEST_F(RetrieverResilienceTest, R14_RetryWithExponentialBackoff)`
- Source: `tests/process/test_process_retriever_resilience_focused.cpp`:563
- Brief: n/a
- Parameters:
  - `<unnamed>` (RetrieverResilienceTest): n/a
  - `<unnamed>` (R14_RetryWithExponentialBackoff): n/a

#### `TEST_F(RetrieverResilienceTest, R15_PartialContextTruncation)`
- Source: `tests/process/test_process_retriever_resilience_focused.cpp`:610
- Brief: n/a
- Parameters:
  - `<unnamed>` (RetrieverResilienceTest): n/a
  - `<unnamed>` (R15_PartialContextTruncation): n/a

#### `TEST_F(RetrieverResilienceTest, R16_HealthCheckAndRecoveryStatus)`
- Source: `tests/process/test_process_retriever_resilience_focused.cpp`:634
- Brief: n/a
- Parameters:
  - `<unnamed>` (RetrieverResilienceTest): n/a
  - `<unnamed>` (R16_HealthCheckAndRecoveryStatus): n/a

### test_process_stress_churn_focused.cpp

#### `TEST_F(StressChurnTest, S01_HighVolumeLinkCreationSustained)`
- Source: `tests/process/test_process_stress_churn_focused.cpp`:58
- Brief: n/a
- Parameters:
  - `<unnamed>` (StressChurnTest): n/a
  - `<unnamed>` (S01_HighVolumeLinkCreationSustained): n/a

#### `TEST_F(StressChurnTest, S02_SustainedAttachmentCreationDeterministic)`
- Source: `tests/process/test_process_stress_churn_focused.cpp`:96
- Brief: n/a
- Parameters:
  - `<unnamed>` (StressChurnTest): n/a
  - `<unnamed>` (S02_SustainedAttachmentCreationDeterministic): n/a

#### `TEST_F(StressChurnTest, S03_RepeatedModelValidationCycles)`
- Source: `tests/process/test_process_stress_churn_focused.cpp`:130
- Brief: n/a
- Parameters:
  - `<unnamed>` (StressChurnTest): n/a
  - `<unnamed>` (S03_RepeatedModelValidationCycles): n/a

#### `TEST_F(StressChurnTest, S04_ConcurrentLinkCreationUnderLockContention)`
- Source: `tests/process/test_process_stress_churn_focused.cpp`:163
- Brief: n/a
- Parameters:
  - `<unnamed>` (StressChurnTest): n/a
  - `<unnamed>` (S04_ConcurrentLinkCreationUnderLockContention): n/a

#### `TEST_F(StressChurnTest, S05_ErrorCodeCycleUnderHighStress)`
- Source: `tests/process/test_process_stress_churn_focused.cpp`:208
- Brief: n/a
- Parameters:
  - `<unnamed>` (StressChurnTest): n/a
  - `<unnamed>` (S05_ErrorCodeCycleUnderHighStress): n/a

#### `TEST_F(StressChurnTest, S06_LongRunningLinkTraversal)`
- Source: `tests/process/test_process_stress_churn_focused.cpp`:242
- Brief: n/a
- Parameters:
  - `<unnamed>` (StressChurnTest): n/a
  - `<unnamed>` (S06_LongRunningLinkTraversal): n/a

#### `TEST_F(StressChurnTest, S07_MemoryStabilityUnderAllocationCycles)`
- Source: `tests/process/test_process_stress_churn_focused.cpp`:276
- Brief: n/a
- Parameters:
  - `<unnamed>` (StressChurnTest): n/a
  - `<unnamed>` (S07_MemoryStabilityUnderAllocationCycles): n/a

#### `TEST_F(StressChurnTest, S08_ReproducibleStressRunWithCanonicalSeed)`
- Source: `tests/process/test_process_stress_churn_focused.cpp`:317
- Brief: n/a
- Parameters:
  - `<unnamed>` (StressChurnTest): n/a
  - `<unnamed>` (S08_ReproducibleStressRunWithCanonicalSeed): n/a

#### `TEST_F(StressChurnTest, S09_EmptyGraphQueryStress)`
- Source: `tests/process/test_process_stress_churn_focused.cpp`:351
- Brief: n/a
- Parameters:
  - `<unnamed>` (StressChurnTest): n/a
  - `<unnamed>` (S09_EmptyGraphQueryStress): n/a

#### `TEST_F(StressChurnTest, S10_LargeContextSizeStress)`
- Source: `tests/process/test_process_stress_churn_focused.cpp`:400
- Brief: n/a
- Parameters:
  - `<unnamed>` (StressChurnTest): n/a
  - `<unnamed>` (S10_LargeContextSizeStress): n/a

#### `TEST_F(StressChurnTest, S11_CommunityDetectionTimeoutStress)`
- Source: `tests/process/test_process_stress_churn_focused.cpp`:455
- Brief: n/a
- Parameters:
  - `<unnamed>` (StressChurnTest): n/a
  - `<unnamed>` (S11_CommunityDetectionTimeoutStress): n/a

#### `TEST_F(StressChurnTest, S12_ConcurrentQueryChurnStress)`
- Source: `tests/process/test_process_stress_churn_focused.cpp`:506
- Brief: n/a
- Parameters:
  - `<unnamed>` (StressChurnTest): n/a
  - `<unnamed>` (S12_ConcurrentQueryChurnStress): n/a

### test_sla_monitoring.cpp

#### `TEST_F(SlaMonitoringTest, SLA01_NotInitializedNoCrash)`
- Source: `tests/process/test_sla_monitoring.cpp`:94
- Brief: n/a
- Parameters:
  - `<unnamed>` (SlaMonitoringTest): n/a
  - `<unnamed>` (SLA01_NotInitializedNoCrash): n/a

#### `TEST_F(SlaMonitoringTest, SLA02_ZeroSlaNoCrash)`
- Source: `tests/process/test_sla_monitoring.cpp`:108
- Brief: n/a
- Parameters:
  - `<unnamed>` (SlaMonitoringTest): n/a
  - `<unnamed>` (SLA02_ZeroSlaNoCrash): n/a

#### `TEST_F(SlaMonitoringTest, SLA03_RegisterThenDeregister)`
- Source: `tests/process/test_sla_monitoring.cpp`:118
- Brief: n/a
- Parameters:
  - `<unnamed>` (SlaMonitoringTest): n/a
  - `<unnamed>` (SLA03_RegisterThenDeregister): n/a

#### `TEST_F(SlaMonitoringTest, SLA04_DeregisterNonExistentNoCrash)`
- Source: `tests/process/test_sla_monitoring.cpp`:134
- Brief: n/a
- Parameters:
  - `<unnamed>` (SlaMonitoringTest): n/a
  - `<unnamed>` (SLA04_DeregisterNonExistentNoCrash): n/a

#### `TEST_F(SlaMonitoringTest, SLA05_AtRiskRuleIdCorrect)`
- Source: `tests/process/test_sla_monitoring.cpp`:143
- Brief: n/a
- Parameters:
  - `<unnamed>` (SlaMonitoringTest): n/a
  - `<unnamed>` (SLA05_AtRiskRuleIdCorrect): n/a

#### `TEST_F(SlaMonitoringTest, SLA06_OverdueRuleIdCorrect)`
- Source: `tests/process/test_sla_monitoring.cpp`:158
- Brief: n/a
- Parameters:
  - `<unnamed>` (SlaMonitoringTest): n/a
  - `<unnamed>` (SLA06_OverdueRuleIdCorrect): n/a

#### `TEST_F(SlaMonitoringTest, SLA07_CallbackStored)`
- Source: `tests/process/test_sla_monitoring.cpp`:173
- Brief: n/a
- Parameters:
  - `<unnamed>` (SlaMonitoringTest): n/a
  - `<unnamed>` (SLA07_CallbackStored): n/a

#### `TEST_F(SlaMonitoringTest, SLA08_DoubleRegisterReplaces)`
- Source: `tests/process/test_sla_monitoring.cpp`:199
- Brief: n/a
- Parameters:
  - `<unnamed>` (SlaMonitoringTest): n/a
  - `<unnamed>` (SLA08_DoubleRegisterReplaces): n/a

### themis::process

#### `TEST(EpkArisXmlImporterTest, EAX01_MinimalAml_Succeeds)`
- Source: `tests/process/test_process_aris_xml.cpp`:98
- Brief: n/a
- Parameters:
  - `<unnamed>` (EpkArisXmlImporterTest): n/a
  - `<unnamed>` (EAX01_MinimalAml_Succeeds): n/a

#### `TEST(EpkArisXmlImporterTest, EAX02_NodeTypesFromTypeNum)`
- Source: `tests/process/test_process_aris_xml.cpp`:111
- Brief: n/a
- Parameters:
  - `<unnamed>` (EpkArisXmlImporterTest): n/a
  - `<unnamed>` (EAX02_NodeTypesFromTypeNum): n/a

#### `TEST(EpkArisXmlImporterTest, EAX03_EdgeConnectivity)`
- Source: `tests/process/test_process_aris_xml.cpp`:137
- Brief: n/a
- Parameters:
  - `<unnamed>` (EpkArisXmlImporterTest): n/a
  - `<unnamed>` (EAX03_EdgeConnectivity): n/a

#### `TEST(EpkArisXmlImporterTest, EAX04_ImportAll_FiltersNonEpk)`
- Source: `tests/process/test_process_aris_xml.cpp`:157
- Brief: n/a
- Parameters:
  - `<unnamed>` (EpkArisXmlImporterTest): n/a
  - `<unnamed>` (EAX04_ImportAll_FiltersNonEpk): n/a

#### `TEST(EpkArisXmlImporterTest, EAX05_ConnectorNodeTypes)`
- Source: `tests/process/test_process_aris_xml.cpp`:173
- Brief: n/a
- Parameters:
  - `<unnamed>` (EpkArisXmlImporterTest): n/a
  - `<unnamed>` (EAX05_ConnectorNodeTypes): n/a

#### `TEST(EpkArisXmlImporterTest, EAX06_TypeNumRoundTrip)`
- Source: `tests/process/test_process_aris_xml.cpp`:203
- Brief: n/a
- Parameters:
  - `<unnamed>` (EpkArisXmlImporterTest): n/a
  - `<unnamed>` (EAX06_TypeNumRoundTrip): n/a

#### `TEST(EpkArisXmlImporterTest, EAX07_EmptyInput_Fails)`
- Source: `tests/process/test_process_aris_xml.cpp`:221
- Brief: n/a
- Parameters:
  - `<unnamed>` (EpkArisXmlImporterTest): n/a
  - `<unnamed>` (EAX07_EmptyInput_Fails): n/a

#### `TEST(EpkArisXmlImporterTest, EAX08_NoEpkModel_Fails)`
- Source: `tests/process/test_process_aris_xml.cpp`:231
- Brief: n/a
- Parameters:
  - `<unnamed>` (EpkArisXmlImporterTest): n/a
  - `<unnamed>` (EAX08_NoEpkModel_Fails): n/a

#### `TEST(EpkArisXmlImporterTest, EAX09_NodesWithoutEdges)`
- Source: `tests/process/test_process_aris_xml.cpp`:241
- Brief: n/a
- Parameters:
  - `<unnamed>` (EpkArisXmlImporterTest): n/a
  - `<unnamed>` (EAX09_NodesWithoutEdges): n/a

#### `TEST(ProcessAgenticRagTest, PAR01_DefaultConfig)`
- Source: `tests/process/test_process_aris_xml.cpp`:305
- Brief: n/a
- Parameters:
  - `<unnamed>` (ProcessAgenticRagTest): n/a
  - `<unnamed>` (PAR01_DefaultConfig): n/a

#### `TEST(ProcessAgenticRagTest, PAR02_ResultDefaults)`
- Source: `tests/process/test_process_aris_xml.cpp`:317
- Brief: n/a
- Parameters:
  - `<unnamed>` (ProcessAgenticRagTest): n/a
  - `<unnamed>` (PAR02_ResultDefaults): n/a

#### `TEST(ProcessAgenticRagTest, PAR03_TypeNumLabels)`
- Source: `tests/process/test_process_aris_xml.cpp`:330
- Brief: n/a
- Parameters:
  - `<unnamed>` (ProcessAgenticRagTest): n/a
  - `<unnamed>` (PAR03_TypeNumLabels): n/a

#### `TEST(ProcessAgenticRagTest, PAR04_ImportAllEmpty)`
- Source: `tests/process/test_process_aris_xml.cpp`:341
- Brief: n/a
- Parameters:
  - `<unnamed>` (ProcessAgenticRagTest): n/a
  - `<unnamed>` (PAR04_ImportAllEmpty): n/a

#### `TEST(ProcessAgenticRagTest, PAR05_EntityEscapedNames)`
- Source: `tests/process/test_process_aris_xml.cpp`:350
- Brief: n/a
- Parameters:
  - `<unnamed>` (ProcessAgenticRagTest): n/a
  - `<unnamed>` (PAR05_EntityEscapedNames): n/a

#### `TEST(ProcessGraphRagBoundsTest, P23_03_PropertiesMapAccess)`
- Source: `tests/process/test_critical_batch1_fixes.cpp`:38
- Brief: n/a
- Parameters:
  - `<unnamed>` (ProcessGraphRagBoundsTest): n/a
  - `<unnamed>` (P23_03_PropertiesMapAccess): n/a
- Details: TestP23-03: Verify properties map access is bounds-safe Tests that accessing att_node.properties["collection"] (line 244-245) is properly handled without bounds violations. The KGNode properties map should handle dynamic key creation safely.

#### `TEST_F(ArisXmlManagerTest, EAX10_ImportArisXml_StoresModel)`
- Source: `tests/process/test_process_aris_xml.cpp`:282
- Brief: n/a
- Parameters:
  - `<unnamed>` (ArisXmlManagerTest): n/a
  - `<unnamed>` (EAX10_ImportArisXml_StoresModel): n/a

#### `TEST_F(ArisXmlManagerTest, PAR06_ImportArisXmlWithMetaOverride)`
- Source: `tests/process/test_process_aris_xml.cpp`:381
- Brief: n/a
- Parameters:
  - `<unnamed>` (ArisXmlManagerTest): n/a
  - `<unnamed>` (PAR06_ImportArisXmlWithMetaOverride): n/a

#### `TEST_F(DmnEvaluatorThreadSafetyTest, P23_04_ConcurrentLoadsSafe)`
- Source: `tests/process/test_critical_batch1_fixes.cpp`:103
- Brief: n/a
- Parameters:
  - `<unnamed>` (DmnEvaluatorThreadSafetyTest): n/a
  - `<unnamed>` (P23_04_ConcurrentLoadsSafe): n/a
- Details: TestP23-04: Verify concurrent loads don't cause data races Tests that multiple threads can safely call loadFromJson/loadFromXml without causing data races. The fix adds mutex protection to tables_.

#### `TEST_F(DmnEvaluatorThreadSafetyTest, P23_05_ConcurrentEvaluateLoadSafe)`
- Source: `tests/process/test_critical_batch1_fixes.cpp`:145
- Brief: n/a
- Parameters:
  - `<unnamed>` (DmnEvaluatorThreadSafetyTest): n/a
  - `<unnamed>` (P23_05_ConcurrentEvaluateLoadSafe): n/a
- Details: TestP23-05: Verify concurrent evaluation and load calls are safe Tests that evaluate() and loadFromJson() can be called concurrently without causing data races. The mutex protects both read and write access.

#### `TEST_F(VccVpbImporterResourceTest, P23_06_YamlImportExceptionSafety)`
- Source: `tests/process/test_critical_batch1_fixes.cpp`:238
- Brief: n/a
- Parameters:
  - `<unnamed>` (VccVpbImporterResourceTest): n/a
  - `<unnamed>` (P23_06_YamlImportExceptionSafety): n/a
- Details: TestP23-06 Extended: Verify exception safety in YAML parsing Tests that even with large/malformed YAML, resources are properly cleaned up and no leaks occur. All string and vector allocations are RAII-managed.

#### `TEST_F(VccVpbImporterResourceTest, P23_06_YamlImportResourceSafety)`
- Source: `tests/process/test_critical_batch1_fixes.cpp`:218
- Brief: n/a
- Parameters:
  - `<unnamed>` (VccVpbImporterResourceTest): n/a
  - `<unnamed>` (P23_06_YamlImportResourceSafety): n/a
- Details: TestP23-06: Verify YAML list import has proper resource management Tests that importYamlList() properly manages string/vector allocations and doesn't leak resources even when exceptions occur. All allocations use RAII (std::string, std::vector).

#### `ProcessDomain domainFromString(std::string_view s)`
- Source: `src/process/process_model_manager.cpp`:133
- Brief: Domain From String.
- Parameters:
  - `s` (std::string_view): Input parameter.
- Return: Return value.
- Details: s Input parameter. Return value. Implements domainFromString without additional internal calls.

#### `std::string errorCodeCategory(ProcessErrorCode code) noexcept`
- Source: `src/process/process_common.cpp`:102
- Brief: Get the error category for an error code.
- Parameters:
  - `code` (ProcessErrorCode): Error code.
- Return: Category name (e.g., "IMPORT", "RETRIEVAL", "SYSTEM").
- Details: code Error code. Category name (e.g., "IMPORT", "RETRIEVAL", "SYSTEM").

#### `std::string errorCodeToString(ProcessErrorCode code) noexcept`
- Source: `src/process/process_common.cpp`:20
- Brief: Convert ProcessErrorCode to a human-readable string.
- Parameters:
  - `code` (ProcessErrorCode): Error code to convert.
- Return: Short error name (e.g., "MALFORMED_INPUT").
- Details: code Error code to convert. Short error name (e.g., "MALFORMED_INPUT").

#### `std::string formatDiagnostic(ProcessErrorCode code, std::string_view context, std::string_view detail="") noexcept`
- Source: `src/process/process_common.cpp`:126
- Brief: Diagnostic message helper for structured error reporting.
- Parameters:
  - `code` (ProcessErrorCode): Error code.
  - `context` (std::string_view): Brief operational context (e.g., "import BPMN from file").
  - `detail` (std::string_view): Additional detail (e.g., "line 42: unexpected token").
- Return: Formatted diagnostic message.
- Details: Constructs an actionable diagnostic message suitable for operator triage. code Error code. context Brief operational context (e.g., "import BPMN from file"). detail Additional detail (e.g., "line 42: unexpected token"). Formatted diagnostic message.

#### `bool hasOperationTimedOut(int64_t start_time_ms)`
- Source: `include/process/process_common.h`:258
- Brief: Check if an operation has exceeded the timeout window.
- Parameters:
  - `start_time_ms` (int64_t): Time when the operation started (from nowMs()).
- Return: true if the operation has exceeded the timeout.
- Details: start_time_ms Time when the operation started (from nowMs()). true if the operation has exceeded the timeout.

#### `bool isContextSizeValid(size_t context_size)`
- Source: `include/process/process_common.h`:238
- Brief: Validates that context size does not exceed retrieval limits.
- Parameters:
  - `context_size` (size_t): Current context size in bytes.
- Return: true if context size is within limits.
- Details: context_size Current context size in bytes. true if context size is within limits.

#### `bool isDeterminismSufficient(DeterminismClass required_determinism, DeterminismClass actual_determinism)`
- Source: `include/process/process_determinism_spec.h`:328
- Brief: Validate determinism classification for a use case.
- Parameters:
  - `required_determinism` (DeterminismClass): The required determinism level.
  - `actual_determinism` (DeterminismClass): The actual determinism level provided by operation.
- Return: true if actual determinism satisfies requirement.
- Details: required_determinism The required determinism level. actual_determinism The actual determinism level provided by operation. true if actual determinism satisfies requirement.

#### `bool isElementCountValid(int32_t element_count)`
- Source: `include/process/process_common.h`:228
- Brief: Validates that element count is within the safe limit.
- Parameters:
  - `element_count` (int32_t): Number of elements in the model.
- Return: true if element count is within limits.
- Details: element_count Number of elements in the model. true if element count is within limits.

#### `bool isInputSizeValid(size_t input_size)`
- Source: `include/process/process_common.h`:208
- Brief: Validates that input size does not exceed the maximum model input size.
- Parameters:
  - `input_size` (size_t): Size of the input in bytes.
- Return: true if the input size is within limits.
- Details: input_size Size of the input in bytes. true if the input size is within limits.

#### `bool isNestingDepthValid(int32_t depth)`
- Source: `include/process/process_common.h`:218
- Brief: Validates that nesting depth is within the safe limit.
- Parameters:
  - `depth` (int32_t): Current nesting depth.
- Return: true if depth is within limits.
- Details: depth Current nesting depth. true if depth is within limits.

#### `bool isPatternSuitable(ConcurrencyPattern pattern, bool requires_atomicity, bool is_high_churn)`
- Source: `include/process/process_concurrency_contract.h`:282
- Brief: Validate that a concurrency pattern is compatible with a given use case.
- Parameters:
  - `pattern` (ConcurrencyPattern): The concurrency pattern to validate.
  - `requires_atomicity` (bool): Whether the use case requires atomic operations.
  - `is_high_churn` (bool): Whether the use case involves high model churn.
- Return: true if the pattern is suitable for the use case.
- Details: pattern The concurrency pattern to validate. requires_atomicity Whether the use case requires atomic operations. is_high_churn Whether the use case involves high model churn. true if the pattern is suitable for the use case.

#### `bool isRetrievalDepthValid(int32_t depth)`
- Source: `include/process/process_common.h`:248
- Brief: Validates that traversal depth is within the safe retrieval limit.
- Parameters:
  - `depth` (int32_t): Current traversal depth.
- Return: true if depth is within limits.
- Details: depth Current traversal depth. true if depth is within limits.

#### `ProcessNotation notationFromString(std::string_view s)`
- Source: `src/process/process_model_manager.cpp`:108
- Brief: Notation From String.
- Parameters:
  - `s` (std::string_view): Input parameter.
- Return: Return value.
- Details: s Input parameter. Return value. Implements notationFromString without additional internal calls.

#### `int64_t nowMs()`
- Source: `include/process/process_common.h`:22
- Brief: Get current wall-clock time in milliseconds since Unix epoch.
- Parameters: none
- Return: Current time in milliseconds
- Details: Current time in milliseconds

#### `ProcessLinkType processLinkTypeFromString(std::string_view s)`
- Source: `src/process/process_linker.cpp`:69
- Brief: Parse a ProcessLinkType from its string representation.
- Parameters:
  - `s` (std::string_view): Input parameter.
- Return: Return value.
- Details: Process Link Type From String. s Input parameter. Return value. Implements processLinkTypeFromString without additional internal calls.

#### `ProcessModelState stateFromString(std::string_view s)`
- Source: `src/process/process_model_manager.cpp`:161
- Brief: State From String.
- Parameters:
  - `s` (std::string_view): Input parameter.
- Return: Return value.
- Details: s Input parameter. Return value. Implements stateFromString without additional internal calls.

#### `std::string_view toString(DiagnosticIncidentType t)`
- Source: `src/process/process_diagnostics.cpp`:31
- Brief: Convert incident type to human-readable name.
- Parameters:
  - `t` (DiagnosticIncidentType): Input parameter.
- Return: String representation.
- Details: To String. t The incident type. String representation. t Input parameter. Return value. Implements toString without additional internal calls.

#### `std::string_view toString(IncidentContext c)`
- Source: `include/process/process_diagnostics_api.h`:371
- Brief: Convert IncidentContext to human-readable name.
- Parameters:
  - `c` (IncidentContext): The incident context.
- Return: String representation.
- Details: c The incident context. String representation.

#### `std::string_view toString(ProcessDomain d)`
- Source: `src/process/process_model_manager.cpp`:73
- Brief: To String.
- Parameters:
  - `d` (ProcessDomain): Input parameter.
- Return: Return value.
- Details: d Input parameter. Return value. Implements toString without additional internal calls.

#### `std::string_view toString(ProcessLinkType t)`
- Source: `src/process/process_linker.cpp`:49
- Brief: Human-readable name for t.
- Parameters:
  - `t` (ProcessLinkType): Input parameter.
- Return: Return value.
- Details: To String. t Input parameter. Return value. Implements toString without additional internal calls.

#### `std::string_view toString(ProcessModelState s)`
- Source: `src/process/process_model_manager.cpp`:92
- Brief: To String.
- Parameters:
  - `s` (ProcessModelState): Input parameter.
- Return: Return value.
- Details: s Input parameter. Return value. Implements toString without additional internal calls.

#### `std::string_view toString(ProcessNotation n)`
- Source: `src/process/process_model_manager.cpp`:56
- Brief: To String.
- Parameters:
  - `n` (ProcessNotation): Input parameter.
- Return: Return value.
- Details: n Input parameter. Return value. Implements toString without additional internal calls.

### themis::process::ApplicationCustomStrategy

#### `ApplicationCustomStrategy(std::shared_ptr< ProcessConflictResolverCallback > callback)`
- Source: `src/process/process_conflict_resolver.cpp`:122
- Brief: Application Custom Strategy.
- Parameters:
  - `callback` (std::shared_ptr< ProcessConflictResolverCallback >): Input parameter.
- Return: Return value.
- Details: callback Input parameter. Return value.

#### `std::string ResolveConflict(const ConflictMetadata &metadata) override`
- Source: `src/process/process_conflict_resolver.cpp`:126
- Brief: Resolve Conflict.
- Parameters:
  - `metadata` (const ConflictMetadata &): Input parameter.
- Return: Return value.
- Details: metadata Input parameter. Return value.

### themis::process::ArisXmlManagerTest

#### `void SetUp() override`
- Source: `tests/process/test_process_aris_xml.cpp`:260
- Brief: n/a
- Parameters: none

#### `void TearDown() override`
- Source: `tests/process/test_process_aris_xml.cpp`:273
- Brief: n/a
- Parameters: none

### themis::process::AuditRecord

#### `nlohmann::json toJson() const`
- Source: `include/process/model_history_contract.h`:170
- Brief: Serialize audit record to JSON.
- Parameters: none
- Return: JSON representation
- Details: JSON representation

### themis::process::BpmnParsingDeterminismSpec

#### `std::string_view describe()`
- Source: `include/process/process_determinism_spec.h`:144
- Brief: Expected behavior when parsing the same BPMN twice.
- Parameters: none

#### `std::string_view uuid_namespace()`
- Source: `include/process/process_determinism_spec.h`:139
- Brief: UUID v5 namespace for stable model ID generation.
- Parameters: none

### themis::process::BpmnSerializer

#### `std::string escapeXml_(std::string_view s)`
- Source: `include/process/bpmn_serializer.h`:189
- Brief: ------------------------------------------------------------------------ BpmnSerializer::escapeXml_ ------------------------------------------------------------------------
- Parameters:
  - `s` (std::string_view): Input parameter.
- Return: Return value.
- Details: s Input parameter. Return value. Calls: reserve(), size().

#### `std::string exportFromJson(const nlohmann::json &normalized_graph)`
- Source: `include/process/bpmn_serializer.h`:163
- Brief: Export from an intermediate normalised JSON graph.
- Parameters:
  - `normalized_graph` (const nlohmann::json &): JSON structure with "nodes" and "edges" arrays.
- Return: Well-formed BPMN 2.0 XML string.
- Throws:
  - std::invalid_argument: if normalized_graph is malformed.
  - std::bad_alloc: if memory allocation fails.
- Details: ------------------------------------------------------------------------ BpmnSerializer::exportFromJson ------------------------------------------------------------------------ Accepts the normalized JSON field of a ProcessModelRecord. normalized_graph JSON structure with "nodes" and "edges" arrays. Well-formed BPMN 2.0 XML string. std::invalid_argument if normalized_graph is malformed. std::bad_alloc if memory allocation fails. g Input parameter. Return value. Calls: is_null(), value(), contains(), xmlTagToNodeType_(), push_back(), std::move(), empty(), exportXml().

#### `std::string exportXml(std::string_view process_id, std::string_view process_name, const std::vector< ProcessNodeInfo > &nodes, const std::vector< ProcessEdgeInfo > &edges)`
- Source: `include/process/bpmn_serializer.h`:145
- Brief: Export a list of process nodes and edges to BPMN 2.0 XML.
- Parameters:
  - `process_id` (std::string_view): Identifier of the process.
  - `process_name` (std::string_view): Name of the process.
  - `nodes` (const std::vector< ProcessNodeInfo > &): Input parameter.
  - `edges` (const std::vector< ProcessEdgeInfo > &): Input parameter.
- Return: Well-formed BPMN 2.0 XML string.
- Throws:
  - std::bad_alloc: if memory allocation fails.
- Details: ------------------------------------------------------------------------ BpmnSerializer::exportXml ------------------------------------------------------------------------ process_id The BPMN <process id=…>> attribute value. process_name The BPMN <process name=…>> attribute value. nodes Ordered list of process nodes. edges List of connecting edges. Well-formed BPMN 2.0 XML string. std::bad_alloc if memory allocation fails. process_id Identifier of the process. process_name Name of the process. nodes Input parameter. edges Input parameter. Return value.

#### `ImportResult importFile(std::string_view file_path)`
- Source: `include/process/bpmn_serializer.h`:128
- Brief: Import a BPMN 2.0 XML file from disk.
- Parameters:
  - `file_path` (std::string_view): Path to the file.
- Return: ImportResult with error_code set on failure.
- Details: ------------------------------------------------------------------------ BpmnSerializer::importFile ------------------------------------------------------------------------ Errors include FILE_READ_ERROR if file cannot be opened, plus all errors from importXml() for the file content. file_path Path to BPMN XML file. ImportResult with error_code set on failure. file_path Path to the file. Return value. Calls: std::string(), is_open(), ImportResult::failure(), content(), importXml().

#### `ImportResult importXml(std::string_view bpmn_xml)`
- Source: `include/process/bpmn_serializer.h`:117
- Brief: Parse a BPMN 2.0 XML string into ProcessNodeInfo / ProcessEdgeInfo objects that can be registered with ProcessGraphManager.
- Parameters:
  - `bpmn_xml` (std::string_view): Input parameter.
- Return: ImportResult with error_code set on failure.
- Details: ------------------------------------------------------------------------ BpmnSerializer::importXml ------------------------------------------------------------------------ All errors are explicit. There are no silent failures. importXml() returns EMPTY_INPUT, INPUT_TOO_LARGE, MALFORMED_INPUT, or SEMANTIC_VIOLATION depending on the failure mode. bpmn_xml Full BPMN 2.0 XML document. ImportResult with error_code set on failure. bpmn_xml Input parameter. Return value. Calls: empty(), ImportResult::failure(), SerializerInputValidator::validateInput(), size(), parser_tracker(), hasTimedOut(), SPDLOG_WARN(), recordElement().

#### `std::string nodeTypeToXmlTag_(BPMNNodeType t)`
- Source: `include/process/bpmn_serializer.h`:190
- Brief: ------------------------------------------------------------------------ BpmnSerializer::nodeTypeToXmlTag_ ------------------------------------------------------------------------
- Parameters:
  - `t` (BPMNNodeType): Input parameter.
- Return: Return value.
- Details: t Input parameter. Return value. Implements nodeTypeToXmlTag_ without additional internal calls.

#### `std::string validateStructure(const std::vector< ProcessNodeInfo > &nodes, const std::vector< ProcessEdgeInfo > &edges)`
- Source: `include/process/bpmn_serializer.h`:182
- Brief: Validate BPMN structure and bounds before processing.
- Parameters:
  - `nodes` (const std::vector< ProcessNodeInfo > &): Input parameter.
  - `edges` (const std::vector< ProcessEdgeInfo > &): Input parameter.
- Return: Error message if invalid; empty string if valid
- Details: ------------------------------------------------------------------------ BpmnSerializer::validateStructure ------------------------------------------------------------------------ Checks: Node and edge counts within limits All referenced nodes exist Deterministic element ordering No malformed attributes nodes The nodes to validate edges The edges to validate Error message if invalid; empty string if valid nodes Input parameter. edges Input parameter. Return value.

#### `BPMNNodeType xmlTagToNodeType_(std::string_view tag)`
- Source: `include/process/bpmn_serializer.h`:191
- Brief: ------------------------------------------------------------------------ BpmnSerializer::xmlTagToNodeType_ ------------------------------------------------------------------------
- Parameters:
  - `tag` (std::string_view): Input parameter.
- Return: Return value.
- Details: tag Input parameter. Return value. Calls: find().

### themis::process::BpmnSerializer::ImportResult

#### `ImportResult failure(ProcessErrorCode code, std::string_view context, std::string_view detail="")`
- Source: `include/process/bpmn_serializer.h`:99
- Brief: Create a failure result with error code and diagnostic message.
- Parameters:
  - `code` (ProcessErrorCode): Input parameter.
  - `context` (std::string_view): Input parameter.
  - `detail` (std::string_view): Input parameter.
- Return: Return value.
- Details: Failure. code Input parameter. context Input parameter. detail Input parameter. Return value. Calls: formatDiagnostic().

#### `ImportResult success(std::string_view pid, std::string_view pname, std::vector< ProcessNodeInfo > n, std::vector< ProcessEdgeInfo > e)`
- Source: `include/process/bpmn_serializer.h`:89
- Brief: Create a successful import result.
- Parameters:
  - `pid` (std::string_view): Input parameter.
  - `pname` (std::string_view): Input parameter.
  - `n` (std::vector< ProcessNodeInfo >): Input parameter.
  - `e` (std::vector< ProcessEdgeInfo >): Input parameter.
- Return: Return value.
- Details: ------------------------------------------------------------------------ BpmnSerializer::ImportResult helper methods (Phase 3) ------------------------------------------------------------------------ pid Input parameter. pname Input parameter. n Input parameter. e Input parameter. Return value. Calls: std::string(), std::move().

### themis::process::BpmnSerializerConcurrencyContract

#### `std::string_view describe()`
- Source: `include/process/process_concurrency_contract.h`:146
- Brief: Human-readable description of this contract.
- Parameters: none

### themis::process::BpmnValidator

#### `SerializerValidationResult validateBpmnConstraints(const std::vector< std::string > &element_ids)`
- Source: `include/process/serializer_hardening.h`:219
- Brief: Validate BPMN-specific constraints.
- Parameters:
  - `element_ids` (const std::vector< std::string > &): Input parameter.
- Return: Validation result.
- Details: Validate Bpmn Constraints. Process ID must be non-empty Element IDs must be unique Source/target references must be resolvable No cycles in flow (except in loop conditions) elements List of BPMN element descriptions. Validation result. element_ids Input parameter. Return value. Calls: empty(), SerializerValidationResult::failure(), count(), insert(), SerializerValidationResult::success().

### themis::process::BulkLinkCreationStress

#### `std::string_view describe() const`
- Source: `include/process/process_stress_scenarios.h`:295
- Brief: n/a
- Parameters: none

#### `std::string generateTestInput() const`
- Source: `include/process/process_stress_scenarios.h`:290
- Brief: Generate a test input with bulk links.
- Parameters: none

#### `bool isSuccess(const StressScenarioResult &result) const`
- Source: `include/process/process_stress_scenarios.h`:293
- Brief: Check if result is success for this scenario.
- Parameters:
  - `result` (const StressScenarioResult &): n/a

### themis::process::CircularReferenceDetectionStress

#### `std::string_view describe() const`
- Source: `include/process/process_stress_scenarios.h`:264
- Brief: n/a
- Parameters: none

#### `std::string generateTestInput() const`
- Source: `include/process/process_stress_scenarios.h`:259
- Brief: Generate a test input with circular references.
- Parameters: none

#### `bool isSuccess(const StressScenarioResult &result) const`
- Source: `include/process/process_stress_scenarios.h`:262
- Brief: Check if result is success (no deadlock, warning logged).
- Parameters:
  - `result` (const StressScenarioResult &): n/a

### themis::process::CmmnParsingDeterminismSpec

#### `std::string_view describe()`
- Source: `include/process/process_determinism_spec.h`:166
- Brief: n/a
- Parameters: none

#### `std::string_view uuid_namespace()`
- Source: `include/process/process_determinism_spec.h`:162
- Brief: n/a
- Parameters: none

### themis::process::CmmnSerializer

#### `std::string escapeXml_(std::string_view s)`
- Source: `include/process/cmmn_serializer.h`:119
- Brief: ───────────────────────────────────────────────────────────────────────────── CmmnSerializer::escapeXml_ ─────────────────────────────────────────────────────────────────────────────
- Parameters:
  - `s` (std::string_view): Input parameter.
- Return: Return value.
- Details: s Input parameter. Return value. Calls: reserve(), size().

#### `std::string exportXml(std::string_view case_id, std::string_view case_name, const std::vector< ProcessNodeInfo > &nodes, const std::vector< ProcessEdgeInfo > &edges)`
- Source: `include/process/cmmn_serializer.h`:112
- Brief: Export nodes and edges to a minimal CMMN 1.1 XML document.
- Parameters:
  - `case_id` (std::string_view): Identifier of the case.
  - `case_name` (std::string_view): Name of the case.
  - `nodes` (const std::vector< ProcessNodeInfo > &): Input parameter.
  - `edges` (const std::vector< ProcessEdgeInfo > &): Input parameter.
- Return: Well-formed CMMN 1.1 XML string.
- Details: ───────────────────────────────────────────────────────────────────────────── CmmnSerializer::exportXml ───────────────────────────────────────────────────────────────────────────── Only nodes with CMMN-compatible subtypes (HUMAN_TASK, PROCESS_TASK, CASE_TASK, STAGE, MILESTONE, CASE_PLAN) are exported. EPK nodes and pure BPMN gateways are skipped. case_id The CMMN <case id=…>> attribute value. case_name The CMMN <case name=…>> attribute value. nodes Process nodes to export. edges Edges (used to emit sentry/onPart where applicable). Well-formed CMMN 1.1 XML string. case_id Identifier of the case. case_name Name of the case. nodes Input parameter. edges Input parameter. Return value.

#### `ImportResult importFile(std::string_view file_path)`
- Source: `include/process/cmmn_serializer.h`:93
- Brief: Import a CMMN 1.1 XML file from disk.
- Parameters:
  - `file_path` (std::string_view): Path to the file.
- Return: Return value.
- Details: ───────────────────────────────────────────────────────────────────────────── CmmnSerializer::importFile ───────────────────────────────────────────────────────────────────────────── file_path Path to the file. Return value. Calls: std::string(), is_open(), ProcessDiagnostics::createImportIncident(), SPDLOG_WARN(), toFormattedMessage(), content(), importXml().

#### `ImportResult importXml(std::string_view cmmn_xml)`
- Source: `include/process/cmmn_serializer.h`:88
- Brief: Parse a CMMN 1.1 XML string into ProcessNodeInfo / ProcessEdgeInfo.
- Parameters:
  - `cmmn_xml` (std::string_view): Input parameter.
- Return: ImportResult with nodes and edges on success.
- Details: ───────────────────────────────────────────────────────────────────────────── CmmnSerializer::importXml ───────────────────────────────────────────────────────────────────────────── cmmn_xml Full CMMN 1.1 XML document. ImportResult with nodes and edges on success. cmmn_xml Input parameter. Return value. Calls: SerializerInputValidator::validateInput(), ProcessDiagnostics::createMalformedInputIncident(), SPDLOG_WARN(), toFormattedMessage(), parser_state(), clear(), count(), find().

### themis::process::CmmnSerializerConcurrencyContract

#### `std::string_view describe()`
- Source: `include/process/process_concurrency_contract.h`:165
- Brief: n/a
- Parameters: none

### themis::process::CmmnValidator

#### `SerializerValidationResult validateCmmnConstraints(std::string_view case_id, const std::vector< std::string > &item_ids)`
- Source: `include/process/serializer_hardening.h`:257
- Brief: Validate CMMN-specific constraints.
- Parameters:
  - `case_id` (std::string_view): Identifier of the case.
  - `item_ids` (const std::vector< std::string > &): Input parameter.
- Return: Validation result.
- Details: Validate Cmmn Constraints. Case model ID is non-empty Case plan items form a directed acyclic graph (or valid loop structure) case_id The case model ID. item_ids List of case plan item IDs. Validation result. case_id Identifier of the case. item_ids Input parameter. Return value. Calls: empty(), SerializerValidationResult::failure(), count(), insert(), SerializerValidationResult::success().

### themis::process::CommunityDetectionTimeoutStress

#### `std::string_view describe() const`
- Source: `include/process/process_stress_scenarios.h`:400
- Brief: n/a
- Parameters: none

#### `std::string generateTestInput() const`
- Source: `include/process/process_stress_scenarios.h`:395
- Brief: Generate a test input with large graph.
- Parameters: none

#### `bool isSuccess(const StressScenarioResult &result) const`
- Source: `include/process/process_stress_scenarios.h`:398
- Brief: Check if result is success (fallback applied).
- Parameters:
  - `result` (const StressScenarioResult &): n/a

### themis::process::ConcurrentQueryChurnStress

#### `std::string_view describe() const`
- Source: `include/process/process_stress_scenarios.h`:434
- Brief: n/a
- Parameters: none

#### `std::string generateTestInput() const`
- Source: `include/process/process_stress_scenarios.h`:429
- Brief: Generate a test input with concurrent queries.
- Parameters: none

#### `bool isSuccess(const StressScenarioResult &result) const`
- Source: `include/process/process_stress_scenarios.h`:432
- Brief: Check if result is success for this scenario.
- Parameters:
  - `result` (const StressScenarioResult &): n/a

### themis::process::ConflictAnalysis

#### `std::string format() const`
- Source: `include/process/process_diagnostics_api.h`:159
- Brief: Format conflict analysis as a string.
- Parameters: none
- Return: "conflicts=N, retries=M, version=V" or empty string if no data
- Details: "conflicts=N, retries=M, version=V" or empty string if no data

### themis::process::ConflictResolutionRegistry

#### `ConflictResolutionRegistry()=default`
- Source: `include/process/conflict_resolution_plugin.h`:394
- Brief: n/a
- Parameters: none

#### `std::unique_ptr< IConflictResolutionPlugin > getPlugin(const std::string &name)`
- Source: `include/process/conflict_resolution_plugin.h`:376
- Brief: Get a plugin by name.
- Parameters:
  - `name` (const std::string &): Plugin name
- Return: Newly created plugin instance, or nullptr if not registered
- Details: name Plugin name Newly created plugin instance, or nullptr if not registered

#### `bool hasPlugin(const std::string &name) const`
- Source: `include/process/conflict_resolution_plugin.h`:384
- Brief: Check if a plugin is registered.
- Parameters:
  - `name` (const std::string &): Plugin name
- Return: true if plugin is registered
- Details: name Plugin name true if plugin is registered

#### `ConflictResolutionRegistry & instance()`
- Source: `include/process/conflict_resolution_plugin.h`:346
- Brief: Get singleton instance.
- Parameters: none
- Return: Reference to global registry
- Details: Reference to global registry

#### `std::vector< std::string > listPlugins() const`
- Source: `include/process/conflict_resolution_plugin.h`:391
- Brief: Get list of all registered plugin names.
- Parameters: none
- Return: Vector of plugin names
- Details: Vector of plugin names

#### `bool registerPlugin(const std::string &name, ConflictResolutionPluginFactory factory)`
- Source: `include/process/conflict_resolution_plugin.h`:355
- Brief: Register a conflict resolution plugin.
- Parameters:
  - `name` (const std::string &): Unique plugin name (e.g., "custom_merge_v1")
  - `factory` (ConflictResolutionPluginFactory): Factory function to create instances
- Return: true if registered successfully; false if name already registered
- Details: name Unique plugin name (e.g., "custom_merge_v1") factory Factory function to create instances true if registered successfully; false if name already registered

#### `bool unregisterPlugin(const std::string &name)`
- Source: `include/process/conflict_resolution_plugin.h`:368
- Brief: Unregister a conflict resolution plugin.
- Parameters:
  - `name` (const std::string &): Plugin name to unregister
- Return: true if unregistered; false if plugin not found
- Details: In-flight operations continue with fallback; no new invocations after unregister. name Plugin name to unregister true if unregistered; false if plugin not found

#### `~ConflictResolutionRegistry()=default`
- Source: `include/process/conflict_resolution_plugin.h`:395
- Brief: n/a
- Parameters: none

### themis::process::ConflictResolverManager

#### `ConflictResolutionStrategy GetCurrentStrategy() const =0`
- Source: `include/process/process_conflict_resolution_callback.h`:207
- Brief: Get current resolution strategy.
- Parameters: none
- Return: Active strategy (LWW, FWW, ApplicationCallback, Unconfigured)
- Details: Active strategy (LWW, FWW, ApplicationCallback, Unconfigured)

#### `void RegisterResolver(std::shared_ptr< ProcessConflictResolverCallback > resolver)=0`
- Source: `include/process/process_conflict_resolution_callback.h`:188
- Brief: Register application-provided conflict resolver.
- Parameters:
  - `resolver` (std::shared_ptr< ProcessConflictResolverCallback >): Resolver implementation
- Details: resolver Resolver implementation Only one resolver active at a time; new registration replaces old

#### `std::string ResolveConflict(const ConflictMetadata &metadata)=0`
- Source: `include/process/process_conflict_resolution_callback.h`:201
- Brief: Resolve conflict using registered resolver or LWW fallback.
- Parameters:
  - `metadata` (const ConflictMetadata &): Conflict metadata
- Return: ID of winning version
- Details: metadata Conflict metadata ID of winning version If no resolver registered, uses LWW fallback If resolver throws exception, uses LWW fallback (incident emitted) If resolver timeout (5s), uses LWW fallback (incident emitted)

#### `void UnregisterResolver()=0`
- Source: `include/process/process_conflict_resolution_callback.h`:212
- Brief: Unregister resolver; fall back to LWW.
- Parameters: none

#### `~ConflictResolverManager()=default`
- Source: `include/process/process_conflict_resolution_callback.h`:214
- Brief: n/a
- Parameters: none

### themis::process::ConsensusLogEntry

#### `nlohmann::json toJson() const`
- Source: `include/process/federated_consensus_contract.h`:203
- Brief: Serialize to JSON for RPC transmission.
- Parameters: none
- Return: JSON representation of this log entry.
- Details: JSON representation of this log entry.

### themis::process::ConsensusSnapshot

#### `bool isValid(const std::string &computed_hash) const noexcept`
- Source: `include/process/federated_consensus_contract.h`:250
- Brief: Check if this snapshot is valid (hash matches state).
- Parameters:
  - `computed_hash` (const std::string &): Hash computed by verifier
- Return: true if hashes match
- Details: computed_hash Hash computed by verifier true if hashes match

### themis::process::DeepNestingStress

#### `std::string_view describe() const`
- Source: `include/process/process_stress_scenarios.h`:134
- Brief: Get scenario description.
- Parameters: none

#### `std::string generateTestInput() const`
- Source: `include/process/process_stress_scenarios.h`:128
- Brief: Generate a test input with nested sub-processes.
- Parameters: none

#### `bool isSuccess(const StressScenarioResult &result) const`
- Source: `include/process/process_stress_scenarios.h`:131
- Brief: Check if result is success for this scenario.
- Parameters:
  - `result` (const StressScenarioResult &): n/a

### themis::process::DeltaPatchCodec

#### `std::string Decode(const std::string &base, const std::string &patch)`
- Source: `src/process/process_audit_logger.cpp`:88
- Brief: Decode.
- Parameters:
  - `base` (const std::string &): Input parameter.
  - `patch` (const std::string &): Input parameter.
- Return: Return value.
- Details: base Input parameter. patch Input parameter. Return value. Implements Decode without additional internal calls.

#### `std::string Encode(const std::string &before, const std::string &after)`
- Source: `src/process/process_audit_logger.cpp`:70
- Brief: Encode.
- Parameters:
  - `before` (const std::string &): Input parameter.
  - `after` (const std::string &): Input parameter.
- Return: Return value.
- Details: before Input parameter. after Input parameter. Return value. Calls: std::to_string(), size().

#### `bool VerifyEquivalence(const std::string &original, const std::string &replayed)`
- Source: `src/process/process_audit_logger.cpp`:102
- Brief: Verify Equivalence.
- Parameters:
  - `original` (const std::string &): Input parameter.
  - `replayed` (const std::string &): Input parameter.
- Return: True when the operation succeeds.
- Details: original Input parameter. replayed Input parameter. True when the operation succeeds. Implements VerifyEquivalence without additional internal calls.

### themis::process::DiagnosticContext

#### `std::string getRemediationSummary() const`
- Source: `include/process/process_diagnostics.h`:350
- Brief: Get human-readable remediation summary.
- Parameters: none
- Return: Formatted string with suggestions for the operator.
- Details: Formatted string with suggestions for the operator.

#### `void recordConflictingOperation(uint64_t operation_id, std::string_view conflicting_key)`
- Source: `include/process/process_diagnostics.h`:338
- Brief: Record a conflicting operation that contributed to the incident.
- Parameters:
  - `operation_id` (uint64_t): Identifier of the operation.
  - `conflicting_key` (std::string_view): Input parameter.
- Details: Record Conflicting Operation. operation_id The operation ID that caused the conflict. conflicting_key The key/resource that was in conflict. operation_id Identifier of the operation. conflicting_key Input parameter.

#### `void recordLimitExceeded(std::string_view limit_name, int64_t limit_value, int64_t actual_value)`
- Source: `include/process/process_diagnostics.h`:325
- Brief: Record a resource limit that was exceeded.
- Parameters:
  - `limit_name` (std::string_view): Name of the limit.
  - `limit_value` (int64_t): Input parameter.
  - `actual_value` (int64_t): Input parameter.
- Details: Record Limit Exceeded. limit_name The name of the limit (e.g., "max_depth"). limit_value The configured limit value. actual_value The actual value that exceeded the limit. limit_name Name of the limit. limit_value Input parameter. actual_value Input parameter.

#### `void recordResourceMetric(std::string_view metric_name, int64_t value)`
- Source: `include/process/process_diagnostics.h`:317
- Brief: Record a resource metric at incident time.
- Parameters:
  - `metric_name` (std::string_view): Name of the metric.
  - `value` (int64_t): Input parameter.
- Details: Record Resource Metric. metric_name The name of the metric (e.g., "parser_depth"). value The current value of the metric. metric_name Name of the metric. value Input parameter. Calls: std::string().

#### `void setRemediationSuggestion(std::string_view suggestion)`
- Source: `include/process/process_diagnostics.h`:331
- Brief: Set a remediation suggestion for the operator.
- Parameters:
  - `suggestion` (std::string_view): Input parameter.
- Details: Set Remediation Suggestion. suggestion An actionable suggestion to resolve the incident. suggestion Input parameter. Calls: std::string().

#### `nlohmann::json toJson() const`
- Source: `include/process/process_diagnostics.h`:344
- Brief: Get the full diagnostic context as JSON for logging.
- Parameters: none
- Return: JSON object with all captured context.
- Details: JSON object with all captured context.

### themis::process::DiagnosticMetricsCollector

#### `uint64_t getIncidentCount(DiagnosticIncidentType incident_type) const`
- Source: `include/process/process_diagnostics.h`:377
- Brief: Get count of incidents by type.
- Parameters:
  - `incident_type` (DiagnosticIncidentType): The incident type to query.
- Return: Count of incidents of this type since collector creation.
- Details: incident_type The incident type to query. Count of incidents of this type since collector creation.

#### `uint64_t getTotalIncidentCount() const`
- Source: `include/process/process_diagnostics.h`:383
- Brief: Get total incident count across all types.
- Parameters: none
- Return: Total count.
- Details: Total count.

#### `void recordIncident(DiagnosticIncidentType incident_type)`
- Source: `include/process/process_diagnostics.h`:370
- Brief: Record an incident occurrence.
- Parameters:
  - `incident_type` (DiagnosticIncidentType): Input parameter.
- Details: Record Incident. incident_type The type of incident. incident_type Input parameter. Calls: lock().

#### `void reset()`
- Source: `include/process/process_diagnostics.h`:388
- Brief: Reset all metrics.
- Parameters: none
- Details: Reset the modification detection flag. Calls: lock(), clear().

#### `nlohmann::json toJson() const`
- Source: `include/process/process_diagnostics.h`:394
- Brief: Get metrics as JSON.
- Parameters: none
- Return: JSON object with per-incident-type counts.
- Details: JSON object with per-incident-type counts.

### themis::process::DiagnosticRecord

#### `DiagnosticRecord(DiagnosticIncidentType incident_type, ProcError error_code, std::string_view operation, std::string_view input_identifier, std::string_view actionable_message)`
- Source: `include/process/process_diagnostics.h`:114
- Brief: Construct a diagnostic record.
- Parameters:
  - `incident_type` (DiagnosticIncidentType): Classification of the incident.
  - `error_code` (ProcError): ProcError from process_api_contract.h.
  - `operation` (std::string_view): Human-readable operation name.
  - `input_identifier` (std::string_view): Input identifier (filename, ID, etc.).
  - `actionable_message` (std::string_view): Actionable message for operator.
- Details: incident_type Classification of the incident. error_code ProcError from process_api_contract.h. operation Human-readable operation name. input_identifier Input identifier (filename, ID, etc.). actionable_message Actionable message for operator.

#### `std::string_view getActionableMessage() const`
- Source: `include/process/process_diagnostics.h`:141
- Brief: Get the actionable message (operator-facing).
- Parameters: none
- Return: The actionable message string.
- Details: The actionable message string.

#### `ProcError getErrorCode() const`
- Source: `include/process/process_diagnostics.h`:149
- Brief: Get the error code.
- Parameters: none
- Return: The ProcError code.
- Details: The ProcError code.

#### `DiagnosticIncidentType getIncidentType() const`
- Source: `include/process/process_diagnostics.h`:157
- Brief: Get the incident type.
- Parameters: none
- Return: The DiagnosticIncidentType.
- Details: The DiagnosticIncidentType.

#### `std::string toFormattedMessage() const`
- Source: `include/process/process_diagnostics.h`:135
- Brief: n/a
- Parameters: none

#### `~DiagnosticRecord()=default`
- Source: `include/process/process_diagnostics.h`:133
- Brief: Format the diagnostic record as a structured log message.
- Parameters: none
- Return: A formatted string suitable for logging.
- Details: A formatted string suitable for logging. Example output: [IMPORT_INCIDENT]model_v1.bpmn(error=7603,ts=2026-08-05T17:53:26Z) Operation:deserialize_bpmn Message:Invalidgatewaytype:COMPLEX_ANDnotsupportedinv2.0

### themis::process::DistributedSpan

#### `DistributedSpan(const std::string &operation_name, const TraceContext &context, const std::string &node_id)`
- Source: `src/process/process_telemetry_integration.cpp`:146
- Brief: n/a
- Parameters:
  - `operation_name` (const std::string &): n/a
  - `context` (const TraceContext &): n/a
  - `node_id` (const std::string &): n/a

#### `void End()`
- Source: `src/process/process_telemetry_integration.cpp`:204
- Brief: End.
- Parameters: none
- Details: Calls: std::chrono::high_resolution_clock::now(), count(), utils::Logger::Debug(), c_str().

#### `const std::map< std::string, std::string > & GetAttributes() const`
- Source: `src/process/process_telemetry_integration.cpp`:227
- Brief: n/a
- Parameters: none

#### `const TraceContext & GetContext() const`
- Source: `src/process/process_telemetry_integration.cpp`:225
- Brief: n/a
- Parameters: none

#### `uint64_t GetLatencyMs() const`
- Source: `src/process/process_telemetry_integration.cpp`:226
- Brief: n/a
- Parameters: none

#### `const std::string & GetOperationName() const`
- Source: `src/process/process_telemetry_integration.cpp`:224
- Brief: n/a
- Parameters: none

#### `void RecordEvent(const std::string &event_name)`
- Source: `src/process/process_telemetry_integration.cpp`:195
- Brief: Record Event.
- Parameters:
  - `event_name` (const std::string &): Name of the event.
- Details: event_name Name of the event. Calls: push_back(), std::chrono::high_resolution_clock::now().

#### `void SetAttribute(const std::string &key, const std::string &value)`
- Source: `src/process/process_telemetry_integration.cpp`:166
- Brief: Set Attribute.
- Parameters:
  - `key` (const std::string &): Input parameter.
  - `value` (const std::string &): Input parameter.
- Details: key Input parameter. value Input parameter. Implements SetAttribute without additional internal calls.

#### `void SetAttribute(const std::string &key, double value)`
- Source: `src/process/process_telemetry_integration.cpp`:186
- Brief: Set Attribute.
- Parameters:
  - `key` (const std::string &): Input parameter.
  - `value` (double): Input parameter.
- Details: key Input parameter. value Input parameter. Calls: std::to_string().

#### `void SetAttribute(const std::string &key, uint64_t value)`
- Source: `src/process/process_telemetry_integration.cpp`:176
- Brief: Set Attribute.
- Parameters:
  - `key` (const std::string &): Input parameter.
  - `value` (uint64_t): Input parameter.
- Details: key Input parameter. value Input parameter. Calls: std::to_string().

#### `~DistributedSpan()`
- Source: `src/process/process_telemetry_integration.cpp`:158
- Brief: n/a
- Parameters: none

### themis::process::DmnEvaluator

#### `DmnEvaluator()=default`
- Source: `include/process/dmn_evaluator.h`:107
- Brief: n/a
- Parameters: none

#### `nlohmann::json evaluate(std::string_view decision_id, const nlohmann::json &input_context) const`
- Source: `include/process/dmn_evaluator.h`:160
- Brief: Evaluate a named decision table against an input context.
- Parameters:
  - `decision_id` (std::string_view): ID of the decision table to evaluate.
  - `input_context` (const nlohmann::json &): JSON object mapping input column names to values.
- Return: JSON object with output column values on match; JSON array of objects for COLLECT hit policy; empty object {} when no rule matches.
- Details: decision_id ID of the decision table to evaluate. input_context JSON object mapping input column names to values. JSON object with output column values on match; JSON array of objects for COLLECT hit policy; empty object {} when no rule matches.

#### `bool evaluateFeel(std::string_view feel_expr, const nlohmann::json &value)`
- Source: `include/process/dmn_evaluator.h`:172
- Brief: Evaluate a single FEEL expression against a JSON value.
- Parameters:
  - `feel_expr` (std::string_view): FEEL expression string (see class doc for supported subset).
  - `value` (const nlohmann::json &): The value to test the expression against.
- Return: true if the expression matches the value.
- Details: feel_expr FEEL expression string (see class doc for supported subset). value The value to test the expression against. true if the expression matches the value.

#### `std::optional< DecisionTable > getDecision(std::string_view decision_id) const`
- Source: `include/process/dmn_evaluator.h`:188
- Brief: Return the loaded decision table with the given ID, or std::nullopt if not found.
- Parameters:
  - `decision_id` (std::string_view): n/a

#### `std::vector< std::string > listDecisions() const`
- Source: `include/process/dmn_evaluator.h`:182
- Brief: Return all loaded decision table IDs.
- Parameters: none

#### `bool loadFromJson(const nlohmann::json &dmn_json)`
- Source: `include/process/dmn_evaluator.h`:136
- Brief: Load a decision table from a JSON object.
- Parameters:
  - `dmn_json` (const nlohmann::json &): Input parameter.
- Return: true on success; false on schema error.
- Details: Load From Json. Expected JSON schema: { "id":"risk_assessment", "name":"RiskAssessment", "hit_policy":"UNIQUE", "input_columns":["amount","type"], "output_columns":["risk_level","action"], "rules":[ {"id":"r1", "inputs":[">1000","\"credit\""], "outputs":{"risk_level":"HIGH","action":"manual_review"}}, {"id":"r2", "inputs":["[100..1000]","-"], "outputs":{"risk_level":"MEDIUM","action":"auto_approve"}} ] } dmn_json JSON object with the schema above. true on success; false on schema error. dmn_json Input parameter. True when the operation succeeds. Calls: value(), contains(), is_array(), push_back(), is_string(), dump(), is_object(), std::move().

#### `bool loadFromXml(std::string_view dmn_xml)`
- Source: `include/process/dmn_evaluator.h`:147
- Brief: Load a decision table from a simplified DMN 1.5 XML string.
- Parameters:
  - `dmn_xml` (std::string_view): Input parameter.
- Return: true on success; false on parse error.
- Details: ───────────────────────────────────────────────────────────────────────────── loadFromXml — simplified state-machine XML parser for DMN 1. Parses the <decision> / <decisionTable> elements using the same state-machine tokenizer as BpmnSerializer. dmn_xml DMN 1.5 XML string. true on success; false on parse error. dmn_xml Input parameter. True when the operation succeeds. 5 ───────────────────────────────────────────────────────────────────────────── Calls: empty(), size(), SPDLOG_ERROR(), find(), substr(), std::tolower(), toLower(), std::string().

#### `bool matchRule_(const DmnRule &rule, const std::vector< std::string > &input_columns, const nlohmann::json &input_context)`
- Source: `include/process/dmn_evaluator.h`:197
- Brief: Evaluate a rule row against the input context.
- Parameters:
  - `rule` (const DmnRule &): n/a
  - `input_columns` (const std::vector< std::string > &): n/a
  - `input_context` (const nlohmann::json &): n/a

### themis::process::DmnEvaluatorThreadSafetyTest

#### `void SetUp() override`
- Source: `tests/process/test_critical_batch1_fixes.cpp`:69
- Brief: n/a
- Parameters: none

### themis::process::DmnValidator

#### `SerializerValidationResult validateDmnConstraints(std::string_view decision_id, int32_t rule_count)`
- Source: `include/process/serializer_hardening.h`:277
- Brief: Validate DMN-specific constraints.
- Parameters:
  - `decision_id` (std::string_view): Identifier of the decision.
  - `rule_count` (int32_t): Input parameter.
- Return: Validation result.
- Details: Validate Dmn Constraints. Decision table has at least one rule Input/output clauses are well-formed Hit policy is recognized decision_id The decision model ID. rule_count Number of decision rules. Validation result. decision_id Identifier of the decision. rule_count Input parameter. Return value. Calls: empty(), SerializerValidationResult::failure(), SerializerValidationResult::success().

### themis::process::EmptyGraphQueryStress

#### `std::string_view describe() const`
- Source: `include/process/process_stress_scenarios.h`:343
- Brief: n/a
- Parameters: none

#### `std::string generateTestInput() const`
- Source: `include/process/process_stress_scenarios.h`:338
- Brief: Generate a test input (empty model).
- Parameters: none

#### `bool isSuccess(const StressScenarioResult &result) const`
- Source: `include/process/process_stress_scenarios.h`:341
- Brief: Check if result is success for this scenario.
- Parameters:
  - `result` (const StressScenarioResult &): n/a

### themis::process::EpkArisXmlImporter

#### `std::vector< ImportResult > importAllAml(std::string_view aml_xml)`
- Source: `include/process/epk_aris_xml_importer.h`:128
- Brief: Import all EPK models from an AML document.
- Parameters:
  - `aml_xml` (std::string_view): Full AML XML string.
- Return: Vector of ImportResult (one per EPK model). Empty on parse error or when no EPK model is present.
- Details: Each entry in the returned vector corresponds to one <Model> of type EPK in the AML file. aml_xml Full AML XML string. Vector of ImportResult (one per EPK model). Empty on parse error or when no EPK model is present.

#### `ImportResult importAml(std::string_view aml_xml)`
- Source: `include/process/epk_aris_xml_importer.h`:116
- Brief: Import the first EPK model found in an AML XML document.
- Parameters:
  - `aml_xml` (std::string_view): Full AML XML string.
- Return: ImportResult. ok is true on success; message contains an error description on failure.
- Details: Scans <Group> hierarchy depth-first for the first <Model> whose Model.Type attribute equals "EPK" (case-insensitive). aml_xml Full AML XML string. ImportResult. ok is true on success; message contains an error description on failure.

#### `EPKNodeType typeNumToEpkNodeType(int type_num)`
- Source: `include/process/epk_aris_xml_importer.h`:139
- Brief: Map an ARIS TypeNum to the corresponding EPKNodeType.
- Parameters:
  - `type_num` (int): Input parameter.
- Return: Return value.
- Details: Type Num To Epk Node Type. Returns EPKNodeType::FUNCTION for any unrecognised TypeNum. type_num Input parameter. Return value. Implements typeNumToEpkNodeType without additional internal calls.

#### `std::string_view typeNumToLabel(int type_num)`
- Source: `include/process/epk_aris_xml_importer.h`:144
- Brief: Return a human-readable German label for an ARIS TypeNum.
- Parameters:
  - `type_num` (int): Input parameter.
- Return: Return value.
- Details: Type Num To Label. type_num Input parameter. Return value. Implements typeNumToLabel without additional internal calls.

### themis::process::EpkSerializer

#### `std::string epkNodeTypeToLabel_(EPKNodeType t)`
- Source: `include/process/epk_serializer.h`:119
- Brief: Epk Node Type To Label.
- Parameters:
  - `t` (EPKNodeType): Input parameter.
- Return: Return value.
- Details: t Input parameter. Return value. Implements epkNodeTypeToLabel_ without additional internal calls.

#### `nlohmann::json exportJson(std::string_view process_id, std::string_view process_name, const std::vector< ProcessNodeInfo > &nodes, const std::vector< ProcessEdgeInfo > &edges)`
- Source: `include/process/epk_serializer.h`:111
- Brief: Export EPK to a structured JSON representation.
- Parameters:
  - `process_id` (std::string_view): Identifier of the process.
  - `process_name` (std::string_view): Name of the process.
  - `nodes` (const std::vector< ProcessNodeInfo > &): Input parameter.
  - `edges` (const std::vector< ProcessEdgeInfo > &): Input parameter.
- Return: Return value.
- Details: Export Json. Useful for programmatic processing and LLM context generation. process_id Identifier of the process. process_name Name of the process. nodes Input parameter. edges Input parameter. Return value.

#### `std::string exportText(std::string_view process_name, const std::vector< ProcessNodeInfo > &nodes, const std::vector< ProcessEdgeInfo > &edges)`
- Source: `include/process/epk_serializer.h`:100
- Brief: Export EPK nodes and edges to the simple text notation.
- Parameters:
  - `process_name` (std::string_view): Name of the process.
  - `nodes` (const std::vector< ProcessNodeInfo > &): Input parameter.
  - `edges` (const std::vector< ProcessEdgeInfo > &): Input parameter.
- Return: Return value.
- Details: Export Text. process_name Name of the process. nodes Input parameter. edges Input parameter. Return value.

#### `ImportResult importJson(const nlohmann::json &epk_json)`
- Source: `include/process/epk_serializer.h`:91
- Brief: Parse an EPK JSON array (nodes + edges format).
- Parameters:
  - `epk_json` (const nlohmann::json &): Input parameter.
- Return: Return value.
- Details: Import Json. epk_json Input parameter. Return value. Calls: is_null(), empty(), ProcessDiagnostics::createMalformedInputIncident(), SPDLOG_WARN(), toFormattedMessage(), is_array(), is_object(), contains().

#### `ImportResult importText(std::string_view epk_text, std::string_view process_id="", std::string_view process_name="")`
- Source: `include/process/epk_serializer.h`:82
- Brief: Parse an EPK text definition into ProcessNodeInfo / ProcessEdgeInfo.
- Parameters:
  - `epk_text` (std::string_view): Input parameter.
  - `process_id` (std::string_view): Identifier of the process.
  - `process_name` (std::string_view): Name of the process.
- Return: Return value.
- Details: Import Text. Accepts both the simple line-based text notation and a JSON array format. epk_text EPK definition string. process_id Optional override for process ID. process_name Optional override for process name. epk_text Input parameter. process_id Identifier of the process. process_name Name of the process. Return value.

#### `EPKNodeType labelToEpkNodeType_(std::string_view label)`
- Source: `include/process/epk_serializer.h`:120
- Brief: Label To Epk Node Type.
- Parameters:
  - `label` (std::string_view): Input parameter.
- Return: Return value.
- Details: label Input parameter. Return value. Implements labelToEpkNodeType_ without additional internal calls.

### themis::process::EpkValidator

#### `SerializerValidationResult validateEpkConstraints(const std::vector< std::string > &nodes, const std::vector< std::pair< std::string, std::string > > &edges)`
- Source: `include/process/serializer_hardening.h`:238
- Brief: Validate EPK-specific constraints.
- Parameters:
  - `nodes` (const std::vector< std::string > &): List of EPK node IDs.
  - `edges` (const std::vector< std::pair< std::string, std::string > > &): List of edge source→target pairs.
- Return: Validation result.
- Details: Events, functions, and connectors properly sequenced No duplicate node IDs Edges reference existing nodes nodes List of EPK node IDs. edges List of edge source→target pairs. Validation result.

### themis::process::EpochManager

#### `EpochManager()=default`
- Source: `include/process/lock_free_linker_contract.h`:307
- Brief: n/a
- Parameters: none

#### `void advanceEpoch() noexcept`
- Source: `include/process/lock_free_linker_contract.h`:292
- Brief: Advance global epoch and trigger reclamation if safe.
- Parameters: none
- Details: Called periodically (every 10-100ms); checks if all threads past an epoch. If safe, reclaims memory deferred from that epoch.

#### `uint64_t currentEpoch() const noexcept`
- Source: `include/process/lock_free_linker_contract.h`:253
- Brief: Get current global epoch.
- Parameters: none
- Return: Monotonically increasing epoch number
- Details: Monotonically increasing epoch number

#### `void deferDeletion(uint64_t epoch, Deleter deleter)`
- Source: `include/process/lock_free_linker_contract.h`:304
- Brief: Defer object deletion until epoch is safe.
- Parameters:
  - `epoch` (uint64_t): Epoch to defer deletion until
  - `deleter` (Deleter): Callable to delete object (e.g., lambda or std::function)
- Details: Object is not deleted immediately; instead queued for deletion when all threads have left the current epoch. epoch Epoch to defer deletion until deleter Callable to delete object (e.g., lambda or std::function)

#### `void enterEpoch(uint64_t epoch) noexcept`
- Source: `include/process/lock_free_linker_contract.h`:265
- Brief: Announce that current thread is using this epoch.
- Parameters:
  - `epoch` (uint64_t): Epoch to enter
- Details: Thread must call this before dereferencing any lock-free data. Call leaveEpoch() when done. epoch Epoch to enter

#### `EpochManager & instance()`
- Source: `include/process/lock_free_linker_contract.h`:247
- Brief: Get singleton instance.
- Parameters: none
- Return: Reference to global epoch manager
- Details: Reference to global epoch manager

#### `void leaveEpoch() noexcept`
- Source: `include/process/lock_free_linker_contract.h`:272
- Brief: Announce that current thread is done with current epoch.
- Parameters: none
- Details: Thread may now advance to next epoch.

#### `bool waitForEpoch(uint64_t epoch, uint32_t timeout_ms=0) noexcept`
- Source: `include/process/lock_free_linker_contract.h`:284
- Brief: Wait for all threads to leave a given epoch.
- Parameters:
  - `epoch` (uint64_t): Epoch to synchronize on
  - `timeout_ms` (uint32_t): Maximum time to wait (0 = no limit)
- Return: true if all threads left epoch; false if timeout
- Details: Blocks until all threads have called leaveEpoch() for epoch. Used before reclaiming memory associated with epoch. epoch Epoch to synchronize on timeout_ms Maximum time to wait (0 = no limit) true if all threads left epoch; false if timeout

#### `~EpochManager()=default`
- Source: `include/process/lock_free_linker_contract.h`:308
- Brief: n/a
- Parameters: none

### themis::process::ExtendedDiagnosticRecord

#### `ExtendedDiagnosticRecord(DiagnosticIncidentType incident_type, ProcError error_code, std::string_view operation, std::string_view input_identifier, std::string_view actionable_message, const TraceContext &trace_context=TraceContext{}, std::optional< IncidentContext > incident_context=std::nullopt, std::optional< int32_t > churn_metric=std::nullopt, const ConflictAnalysis &conflict_analysis=ConflictAnalysis{})`
- Source: `include/process/process_diagnostics_api.h`:187
- Brief: Construct an extended diagnostic record.
- Parameters:
  - `incident_type` (DiagnosticIncidentType): Classification from DiagnosticIncidentType
  - `error_code` (ProcError): ProcError from process_api_contract.h
  - `operation` (std::string_view): Operation name (e.g., "update_model")
  - `input_identifier` (std::string_view): Input identifier (model ID, instance ID)
  - `actionable_message` (std::string_view): Message for the operator
  - `trace_context` (const TraceContext &): Optional trace context for distributed tracing
  - `incident_context` (std::optional< IncidentContext >): Optional finer-grained context (CHURN_DETECTION, etc.)
  - `churn_metric` (std::optional< int32_t >): Optional concurrent operation count at time of incident
  - `conflict_analysis` (const ConflictAnalysis &): Optional conflict tracking data
- Details: incident_type Classification from DiagnosticIncidentType error_code ProcError from process_api_contract.h operation Operation name (e.g., "update_model") input_identifier Input identifier (model ID, instance ID) actionable_message Message for the operator trace_context Optional trace context for distributed tracing incident_context Optional finer-grained context (CHURN_DETECTION, etc.) churn_metric Optional concurrent operation count at time of incident conflict_analysis Optional conflict tracking data

#### `bool isConflictIncident() const`
- Source: `include/process/process_diagnostics_api.h`:236
- Brief: Check if this is a conflict-related incident.
- Parameters: none
- Return: true if incident_context == CONFLICT_DETECTED
- Details: true if incident_context == CONFLICT_DETECTED

#### `bool isHighChurn() const`
- Source: `include/process/process_diagnostics_api.h`:228
- Brief: Check if this is a high-churn incident.
- Parameters: none
- Return: true if churn_metric > 500
- Details: true if churn_metric > 500

#### `std::string toFormattedMessage() const override`
- Source: `include/process/process_diagnostics_api.h`:222
- Brief: Format the extended diagnostic record as a structured log message.
- Parameters: none
- Return: A formatted string with all available context layers
- Details: A formatted string with all available context layers Example output: [IMPORT_INCIDENT]model_v1.bpmn(error=7603,ts=2026-08-06T17:53:26Z) Context:CHURN_DETECTION[churn=625ops/sec] Trace:trace-2026-08-06-t1234567890/span-xyz Operation:deserialize_bpmn Message:Highmodelchurn(>500ops/sec).Retrywithexponentialbackoff. Conflicts:3detected,2retries

### themis::process::ExtendedProcessDiagnostics

#### `ExtendedProcessDiagnostics()=delete`
- Source: `include/process/process_diagnostics_api.h`:368
- Brief: n/a
- Parameters: none

#### `ExtendedDiagnosticRecord createChurnIncident(ProcError error, std::string_view input_id, std::string_view message, int32_t concurrent_ops, std::optional< std::string_view > trace_id=std::nullopt)`
- Source: `include/process/process_diagnostics_api.h`:274
- Brief: Create a churn-detection incident diagnostic.
- Parameters:
  - `error` (ProcError): ProcError code (typically kValidationFailed)
  - `input_id` (std::string_view): Input identifier (model ID, instance ID)
  - `message` (std::string_view): Actionable message (recommend backoff, batching, etc.)
  - `concurrent_ops` (int32_t): Concurrent operation count at time of incident
  - `trace_id` (std::optional< std::string_view >): Optional trace ID for correlation
- Return: ExtendedDiagnosticRecord with CHURN_DETECTION context
- Details: Use when high model update rate is detected or causes operation failure. error ProcError code (typically kValidationFailed) input_id Input identifier (model ID, instance ID) message Actionable message (recommend backoff, batching, etc.) concurrent_ops Concurrent operation count at time of incident trace_id Optional trace ID for correlation ExtendedDiagnosticRecord with CHURN_DETECTION context Example: autodiag=ExtendedProcessDiagnostics::createChurnIncident( ProcError::kValidationFailed, "model_large_v2", "Modelupdatefailed:highconcurrentchurn(625ops/sec)." "Retrywithexponentialbackoff(2^nseconds).", concurrent_ops=625, trace_id="trace-2026-08-06-xyz" );

#### `ExtendedDiagnosticRecord createConflictIncident(ProcError error, std::string_view input_id, std::string_view message, int32_t conflict_count, int32_t retry_count, std::optional< std::string_view > trace_id=std::nullopt)`
- Source: `include/process/process_diagnostics_api.h`:307
- Brief: Create a conflict-detection incident diagnostic.
- Parameters:
  - `error` (ProcError): ProcError code (typically kValidationFailed or kLinkingFailed)
  - `input_id` (std::string_view): Input identifier (model ID, link ID)
  - `message` (std::string_view): Actionable message (recommend retry with latest version)
  - `conflict_count` (int32_t): Number of conflicts detected
  - `retry_count` (int32_t): Number of retry attempts before failure
  - `trace_id` (std::optional< std::string_view >): Optional trace ID for correlation
- Return: ExtendedDiagnosticRecord with CONFLICT_DETECTED context
- Details: Use when write-write conflicts, link staleness, or version mismatches occur. error ProcError code (typically kValidationFailed or kLinkingFailed) input_id Input identifier (model ID, link ID) message Actionable message (recommend retry with latest version) conflict_count Number of conflicts detected retry_count Number of retry attempts before failure trace_id Optional trace ID for correlation ExtendedDiagnosticRecord with CONFLICT_DETECTED context Example: autodiag=ExtendedProcessDiagnostics::createConflictIncident( ProcError::kValidationFailed, "model_v1", "Write-writeconflictdetected.Fetchlatestversionandretry.", conflict_count=3, retry_count=2, trace_id="trace-conflict-xyz" );

#### `ExtendedDiagnosticRecord createResourceExhaustionIncident(ProcError error, std::string_view input_id, std::string_view message, std::string_view resource_type, std::optional< std::string_view > trace_id=std::nullopt)`
- Source: `include/process/process_diagnostics_api.h`:359
- Brief: Create a resource-exhaustion incident diagnostic.
- Parameters:
  - `error` (ProcError): ProcError code (typically kExecutionTimeout, kMaxDepthExceeded, etc.)
  - `input_id` (std::string_view): Input identifier (model ID, instance ID)
  - `message` (std::string_view): Actionable message describing the limit
  - `resource_type` (std::string_view): Name of resource that was exhausted ("memory", "timeout", "depth")
  - `trace_id` (std::optional< std::string_view >): Optional trace ID for correlation
- Return: ExtendedDiagnosticRecord with RESOURCE_EXHAUSTION context
- Details: Use when memory, file handles, or timeout limits are exceeded. error ProcError code (typically kExecutionTimeout, kMaxDepthExceeded, etc.) input_id Input identifier (model ID, instance ID) message Actionable message describing the limit resource_type Name of resource that was exhausted ("memory", "timeout", "depth") trace_id Optional trace ID for correlation ExtendedDiagnosticRecord with RESOURCE_EXHAUSTION context

#### `ExtendedDiagnosticRecord createTracedIncident(const DiagnosticRecord &base_incident, std::string_view trace_id, std::string_view span_id)`
- Source: `include/process/process_diagnostics_api.h`:341
- Brief: Create a traced incident diagnostic.
- Parameters:
  - `base_incident` (const DiagnosticRecord &): Base DiagnosticRecord (from ProcessDiagnostics factory)
  - `trace_id` (std::string_view): Trace identifier (OpenTelemetry format recommended)
  - `span_id` (std::string_view): Span identifier within the trace
- Return: ExtendedDiagnosticRecord with trace context attached
- Details: Use when an incident occurs within a distributed trace context. Attaches trace_id and span_id for correlation. base_incident Base DiagnosticRecord (from ProcessDiagnostics factory) trace_id Trace identifier (OpenTelemetry format recommended) span_id Span identifier within the trace ExtendedDiagnosticRecord with trace context attached Example: autobase=ProcessDiagnostics::createImportIncident( ProcError::kDeserialiserFailed, "model.bpmn", "Invalidgatewaytype" ); autotraced=ExtendedProcessDiagnostics::createTracedIncident( base, "4bf92f3577b34da6a3ce929d0e0e4736",//trace_id "00f067aa0ba902b7"//span_id );

### themis::process::FederationConsensusManager

#### `bool AppendEntries(const std::string &leader_id, uint64_t leader_term, uint64_t prev_log_index, uint64_t prev_log_term, const std::vector< std::string > &entries, uint64_t leader_commit)`
- Source: `include/process/federation_consensus_manager.h`:94
- Brief: Append entries RPC (heartbeat or replication).
- Parameters:
  - `leader_id` (const std::string &): Identifier of the leader.
  - `leader_term` (uint64_t): Input parameter.
  - `prev_log_index` (uint64_t): Input parameter.
  - `prev_log_term` (uint64_t): Input parameter.
  - `entries` (const std::vector< std::string > &): Input parameter.
  - `leader_commit` (uint64_t): Input parameter.
- Return: True when the operation succeeds.
- Details: Append Entries. leader_id Identifier of the leader. leader_term Input parameter. prev_log_index Input parameter. prev_log_term Input parameter. entries Input parameter. leader_commit Input parameter. True when the operation succeeds. Calls: FederationConsensusManagerImpl::ComputeCrc32(), push_back().

#### `uint64_t AppendEntry(const std::string &data)`
- Source: `include/process/federation_consensus_manager.h`:72
- Brief: Append entry to consensus log.
- Parameters:
  - `data` (const std::string &): Input parameter.
- Return: Return value.
- Details: Append Entry. data Input parameter. Return value. Implements AppendEntry without additional internal calls.

#### `std::unique_ptr< FederationConsensusManager > Create(const FederationConsensusConfig &config, const std::string &node_id, size_t quorum_size)`
- Source: `include/process/federation_consensus_manager.h`:53
- Brief: Factory method to create consensus manager.
- Parameters:
  - `config` (const FederationConsensusConfig &): n/a
  - `node_id` (const std::string &): n/a
  - `quorum_size` (size_t): n/a

#### `FederationConsensusManager(std::unique_ptr< FederationConsensusManagerImpl > impl)`
- Source: `include/process/federation_consensus_manager.h`:61
- Brief: Constructor.
- Parameters:
  - `impl` (std::unique_ptr< FederationConsensusManagerImpl >): n/a

#### `std::string GetLeader() const`
- Source: `include/process/federation_consensus_manager.h`:82
- Brief: Get current leader ID.
- Parameters: none

#### `bool IsLeader() const`
- Source: `include/process/federation_consensus_manager.h`:77
- Brief: Check if this node is leader.
- Parameters: none

#### `bool RequestVote(const std::string &candidate_id, uint64_t candidate_term, uint64_t candidate_last_log_index, uint64_t candidate_last_log_term)`
- Source: `include/process/federation_consensus_manager.h`:87
- Brief: Request vote (called by remote candidate).
- Parameters:
  - `candidate_id` (const std::string &): Identifier of the candidate.
  - `candidate_term` (uint64_t): Input parameter.
  - `candidate_last_log_index` (uint64_t): Input parameter.
  - `candidate_last_log_term` (uint64_t): Input parameter.
- Return: True when the operation succeeds.
- Details: Request Vote. candidate_id Identifier of the candidate. candidate_term Input parameter. candidate_last_log_index Input parameter. candidate_last_log_term Input parameter. True when the operation succeeds. Implements RequestVote without additional internal calls.

#### `void Tick()`
- Source: `include/process/federation_consensus_manager.h`:102
- Brief: Tick state machine (called periodically).
- Parameters: none
- Details: Tick. Implements Tick without additional internal calls.

#### `~FederationConsensusManager()`
- Source: `include/process/federation_consensus_manager.h`:67
- Brief: Destructor.
- Parameters: none

### themis::process::FederationConsensusManagerImpl

#### `bool AppendEntries(const std::string &leader_id, uint64_t leader_term, uint64_t prev_log_index, uint64_t prev_log_term, const std::vector< ConsensusLogEntry > &entries, uint64_t leader_commit)`
- Source: `src/process/federation_consensus_manager.cpp`:150
- Brief: Append Entries.
- Parameters:
  - `leader_id` (const std::string &): Identifier of the leader.
  - `leader_term` (uint64_t): Input parameter.
  - `prev_log_index` (uint64_t): Input parameter.
  - `prev_log_term` (uint64_t): Input parameter.
  - `entries` (const std::vector< ConsensusLogEntry > &): Input parameter.
  - `leader_commit` (uint64_t): Input parameter.
- Return: True when the operation succeeds.
- Details: leader_id Identifier of the leader. leader_term Input parameter. prev_log_index Input parameter. prev_log_term Input parameter. entries Input parameter. leader_commit Input parameter. True when the operation succeeds. leader_id Identifier of the leader. leader_term Input parameter. prev_log_index Input parameter. prev_log_term Input parameter. entries Input parameter. leader_commit Input parameter. True when the operation succeeds. Calls: lock(), GetCurrentTimeMs(), GetLogEntry(), utils::Logger::Warn(), c_str(), push_back(), replica_lock(), std::max().

#### `uint64_t AppendEntry(const std::string &data)`
- Source: `src/process/federation_consensus_manager.cpp`:113
- Brief: Append Entry.
- Parameters:
  - `data` (const std::string &): Input parameter.
- Return: Return value.
- Details: data Input parameter. Return value. data Input parameter. Return value. Calls: consensus_lock(), log_lock(), utils::Logger::Warn(), c_str(), empty(), back(), GetCurrentTimeMs(), ComputeCrc32().

#### `void BecomeCandidate()`
- Source: `src/process/federation_consensus_manager.cpp`:175
- Brief: Become Candidate.
- Parameters: none
- Details: Calls: GetRandomMs(), utils::Logger::Info(), c_str().

#### `void BecomeFollower(uint64_t new_term)`
- Source: `src/process/federation_consensus_manager.cpp`:186
- Brief: Become Follower.
- Parameters:
  - `new_term` (uint64_t): Input parameter.
- Details: new_term Input parameter. new_term Input parameter. Implements BecomeFollower without additional internal calls.

#### `void BecomeLeader()`
- Source: `src/process/federation_consensus_manager.cpp`:180
- Brief: Become Leader.
- Parameters: none
- Details: Calls: clear(), utils::Logger::Info(), c_str().

#### `void BroadcastHeartbeat()`
- Source: `src/process/federation_consensus_manager.cpp`:191
- Brief: Broadcast Heartbeat.
- Parameters: none
- Details: Calls: GetCurrentTimeMs().

#### `uint32_t ComputeCrc32(const std::string &data)`
- Source: `src/process/federation_consensus_manager.cpp`:232
- Brief: Compute Crc32.
- Parameters:
  - `data` (const std::string &): Input parameter.
- Return: Return value.
- Details: data Input parameter. Return value. data Input parameter. Return value. Implements ComputeCrc32 without additional internal calls.

#### `FederationConsensusManagerImpl(const FederationConsensusConfig &config, const std::string &node_id, size_t quorum_size)`
- Source: `src/process/federation_consensus_manager.cpp`:80
- Brief: n/a
- Parameters:
  - `config` (const FederationConsensusConfig &): n/a
  - `node_id` (const std::string &): n/a
  - `quorum_size` (size_t): n/a

#### `uint64_t GetCurrentTimeMs()`
- Source: `src/process/federation_consensus_manager.cpp`:217
- Brief: Get Current Time Ms.
- Parameters: none
- Return: Return value.
- Details: Return value. Return value. Calls: std::chrono::system_clock::now(), time_since_epoch(), count().

#### `std::string GetLeader() const`
- Source: `src/process/federation_consensus_manager.cpp`:125
- Brief: Get Leader.
- Parameters: none
- Return: Return value.
- Details: Return value.

#### `const ConsensusLogEntry * GetLogEntry(uint64_t index) const`
- Source: `src/process/federation_consensus_manager.cpp`:211
- Brief: Get Log Entry.
- Parameters:
  - `index` (uint64_t): Input parameter.
- Return: Pointer to the result.
- Details: index Input parameter. Pointer to the result.

#### `uint64_t GetRandomMs(uint64_t min_ms, uint64_t max_ms)`
- Source: `src/process/federation_consensus_manager.cpp`:225
- Brief: Get Random Ms.
- Parameters:
  - `min_ms` (uint64_t): Input parameter.
  - `max_ms` (uint64_t): Input parameter.
- Return: Return value.
- Details: min_ms Input parameter. max_ms Input parameter. Return value. min_ms Input parameter. max_ms Input parameter. Return value. Calls: rng(), std::this_thread::get_id(), dist().

#### `bool IsLeader() const`
- Source: `src/process/federation_consensus_manager.cpp`:119
- Brief: Is Leader.
- Parameters: none
- Return: True when the operation succeeds.
- Details: True when the operation succeeds.

#### `bool IsLogUpToDate(uint64_t candidate_last_index, uint64_t candidate_last_term) const`
- Source: `src/process/federation_consensus_manager.cpp`:204
- Brief: Is Log Up To Date.
- Parameters:
  - `candidate_last_index` (uint64_t): Input parameter.
  - `candidate_last_term` (uint64_t): Input parameter.
- Return: True when the operation succeeds.
- Details: candidate_last_index Input parameter. candidate_last_term Input parameter. True when the operation succeeds.

#### `void ReplicateLogEntries()`
- Source: `src/process/federation_consensus_manager.cpp`:196
- Brief: Replicate Log Entries.
- Parameters: none
- Details: Implements ReplicateLogEntries without additional internal calls.

#### `bool RequestVote(const std::string &candidate_id, uint64_t candidate_term, uint64_t candidate_last_log_index, uint64_t candidate_last_log_term)`
- Source: `src/process/federation_consensus_manager.cpp`:135
- Brief: Request Vote.
- Parameters:
  - `candidate_id` (const std::string &): Identifier of the candidate.
  - `candidate_term` (uint64_t): Input parameter.
  - `candidate_last_log_index` (uint64_t): Input parameter.
  - `candidate_last_log_term` (uint64_t): Input parameter.
- Return: True when the operation succeeds.
- Details: candidate_id Identifier of the candidate. candidate_term Input parameter. candidate_last_log_index Input parameter. candidate_last_log_term Input parameter. True when the operation succeeds. candidate_id Identifier of the candidate. candidate_term Input parameter. candidate_last_log_index Input parameter. candidate_last_log_term Input parameter. True when the operation succeeds. Calls: lock(), empty(), IsLogUpToDate(), utils::Logger::Debug(), c_str().

#### `void Tick()`
- Source: `src/process/federation_consensus_manager.cpp`:160
- Brief: Tick.
- Parameters: none
- Details: Calls: lock(), GetCurrentTimeMs(), utils::Logger::Info(), c_str(), BecomeCandidate(), BroadcastHeartbeat().

#### `~FederationConsensusManagerImpl()=default`
- Source: `src/process/federation_consensus_manager.cpp`:102
- Brief: n/a
- Parameters: none

### themis::process::FederationReplicaManager

#### `std::string ApplyEntry(uint64_t log_index, uint64_t log_term, const std::string &data)`
- Source: `include/process/federation_replica_manager.h`:83
- Brief: Apply committed log entry to state machine.
- Parameters:
  - `log_index` (uint64_t): Input parameter.
  - `log_term` (uint64_t): Input parameter.
  - `data` (const std::string &): Input parameter.
- Return: Return value.
- Details: Apply Entry. log_index Input parameter. log_term Input parameter. data Input parameter. Return value. Implements ApplyEntry without additional internal calls.

#### `std::unique_ptr< FederationReplicaManager > Create(const FederationReplicaConfig &config, const std::string &node_id)`
- Source: `include/process/federation_replica_manager.h`:65
- Brief: Factory method to create replica manager.
- Parameters:
  - `config` (const FederationReplicaConfig &): n/a
  - `node_id` (const std::string &): n/a

#### `FederationReplicaManager(std::unique_ptr< FederationReplicaManagerImpl > impl)`
- Source: `include/process/federation_replica_manager.h`:72
- Brief: Constructor.
- Parameters:
  - `impl` (std::unique_ptr< FederationReplicaManagerImpl >): n/a

#### `uint64_t GetLastApplied() const`
- Source: `include/process/federation_replica_manager.h`:111
- Brief: Get last applied log index.
- Parameters: none

#### `std::string GetStateHash() const`
- Source: `include/process/federation_replica_manager.h`:106
- Brief: Get current state hash.
- Parameters: none

#### `ReplicaStats GetStats() const`
- Source: `include/process/federation_replica_manager.h`:116
- Brief: Get replica statistics.
- Parameters: none

#### `bool RestoreFromSnapshot(const Snapshot *snapshot, const std::vector< std::string > &tail_entries)`
- Source: `include/process/federation_replica_manager.h`:100
- Brief: Restore from snapshot and replay tail entries.
- Parameters:
  - `snapshot` (const Snapshot *): Input parameter.
  - `tail_entries` (const std::vector< std::string > &): Input parameter.
- Return: True when the operation succeeds.
- Details: Restore From Snapshot. snapshot Input parameter. tail_entries Input parameter. True when the operation succeeds. Implements RestoreFromSnapshot without additional internal calls.

#### `std::shared_ptr< Snapshot > TakeSnapshot()`
- Source: `include/process/federation_replica_manager.h`:95
- Brief: Take snapshot of current state.
- Parameters: none
- Return: Return value.
- Details: Take Snapshot. Return value. Implements TakeSnapshot without additional internal calls.

#### `bool VerifyConsistency(const std::string &expected_state_hash, uint64_t at_log_index) const`
- Source: `include/process/federation_replica_manager.h`:89
- Brief: Verify replica consistency via state hash.
- Parameters:
  - `expected_state_hash` (const std::string &): n/a
  - `at_log_index` (uint64_t): n/a

#### `~FederationReplicaManager()`
- Source: `include/process/federation_replica_manager.h`:78
- Brief: Destructor.
- Parameters: none

### themis::process::FederationReplicaManagerImpl

#### `std::string ApplyEntry(uint64_t log_index, uint64_t log_term, const std::string &data)`
- Source: `src/process/federation_replica_manager.cpp`:82
- Brief: ======================================================================== PUBLIC API - STATE MACHINE EXECUTION ========================================================================
- Parameters:
  - `log_index` (uint64_t): Input parameter.
  - `log_term` (uint64_t): Input parameter.
  - `data` (const std::string &): Input parameter.
- Return: Return value.
- Details: Apply Entry. log_index Input parameter. log_term Input parameter. data Input parameter. Return value. log_index Input parameter. log_term Input parameter. data Input parameter. Return value. Calls: lock(), utils::Logger::Warn(), c_str(), ApplyEntryLocked().

#### `std::string ApplyEntryLocked(uint64_t log_index, uint64_t log_term, const std::string &data)`
- Source: `src/process/federation_replica_manager.cpp`:142
- Brief: Apply Entry Locked.
- Parameters:
  - `log_index` (uint64_t): Input parameter.
  - `log_term` (uint64_t): Input parameter.
  - `data` (const std::string &): Input parameter.
- Return: Return value.
- Details: log_index Input parameter. log_term Input parameter. data Input parameter. Return value. log_index Input parameter. log_term Input parameter. data Input parameter. Return value. Calls: std::to_string(), utils::Logger::Debug(), c_str(), size(), ComputeStateHash().

#### `std::string ComputeStateHash() const`
- Source: `src/process/federation_replica_manager.cpp`:149
- Brief: Compute State Hash.
- Parameters: none
- Return: Return value.
- Details: Return value.

#### `FederationReplicaManagerImpl(const FederationReplicaConfig &config, const std::string &node_id)`
- Source: `src/process/federation_replica_manager.cpp`:61
- Brief: n/a
- Parameters:
  - `config` (const FederationReplicaConfig &): n/a
  - `node_id` (const std::string &): n/a

#### `uint64_t GetLastApplied() const`
- Source: `src/process/federation_replica_manager.cpp`:119
- Brief: Get Last Applied.
- Parameters: none
- Return: Return value.
- Details: Return value.

#### `std::string GetStateHash() const`
- Source: `src/process/federation_replica_manager.cpp`:113
- Brief: Get State Hash.
- Parameters: none
- Return: Return value.
- Details: Return value.

#### `ReplicaStats GetStats() const`
- Source: `src/process/federation_replica_manager.cpp`:125
- Brief: Get Stats.
- Parameters: none
- Return: Return value.
- Details: Return value.

#### `bool ReplayEntries(const std::vector< std::string > &entries)`
- Source: `src/process/federation_replica_manager.cpp`:156
- Brief: Replay Entries.
- Parameters:
  - `entries` (const std::vector< std::string > &): Input parameter.
- Return: True when the operation succeeds.
- Details: entries Input parameter. True when the operation succeeds. entries Input parameter. True when the operation succeeds. Calls: size(), ApplyEntryLocked().

#### `bool RestoreFromSnapshot(const Snapshot *snapshot, const std::vector< std::string > &tail_entries)`
- Source: `src/process/federation_replica_manager.cpp`:106
- Brief: Restore From Snapshot.
- Parameters:
  - `snapshot` (const Snapshot *): Input parameter.
  - `tail_entries` (const std::vector< std::string > &): Input parameter.
- Return: True when the operation succeeds.
- Details: snapshot Input parameter. tail_entries Input parameter. True when the operation succeeds. snapshot Input parameter. tail_entries Input parameter. True when the operation succeeds. Calls: lock(), clear(), ReplayEntries(), utils::Logger::Error(), c_str(), ValidateState(), utils::Logger::Info().

#### `std::shared_ptr< Snapshot > TakeSnapshot()`
- Source: `src/process/federation_replica_manager.cpp`:98
- Brief: Take Snapshot.
- Parameters: none
- Return: Return value.
- Details: Return value. Return value. Calls: lock(), std::chrono::high_resolution_clock::now(), size(), std::chrono::system_clock::now(), time_since_epoch(), count(), utils::Logger::Info(), c_str().

#### `bool ValidateState() const`
- Source: `src/process/federation_replica_manager.cpp`:162
- Brief: Validate State.
- Parameters: none
- Return: True when the operation succeeds.
- Details: True when the operation succeeds.

#### `bool VerifyConsistency(const std::string &expected_state_hash, uint64_t at_log_index) const`
- Source: `src/process/federation_replica_manager.cpp`:91
- Brief: Verify Consistency.
- Parameters:
  - `expected_state_hash` (const std::string &): Input parameter.
  - `at_log_index` (uint64_t): Input parameter.
- Return: True when the operation succeeds.
- Details: expected_state_hash Input parameter. at_log_index Input parameter. True when the operation succeeds.

#### `~FederationReplicaManagerImpl()=default`
- Source: `src/process/federation_replica_manager.cpp`:72
- Brief: n/a
- Parameters: none

### themis::process::FimImporter

#### `FimImporter()=default`
- Source: `include/process/fim_importer.h`:77
- Brief: n/a
- Parameters: none

#### `ProcessModelRecord buildRecord_(const BpmnSerializer::ImportResult &ir, ProcessDomain domain)`
- Source: `include/process/fim_importer.h`:164
- Brief: n/a
- Parameters:
  - `ir` (const BpmnSerializer::ImportResult &): n/a
  - `domain` (ProcessDomain): n/a

#### `std::vector< FimModelResult > importFimCatalogue(std::string_view catalogue_xml, ProcessDomain domain=ProcessDomain::ADMINISTRATION)`
- Source: `include/process/fim_importer.h`:121
- Brief: Parse a FIM Prozessbibliothek XML catalogue document.
- Parameters:
  - `catalogue_xml` (std::string_view): Input parameter.
  - `domain` (ProcessDomain): Input parameter.
- Return: One FimModelResult per <prozess> element found.
- Details: ───────────────────────────────────────────────────────────────────────────── FimImporter::importFimCatalogue ───────────────────────────────────────────────────────────────────────────── The catalogue may contain multiple <prozess> elements. Each element is converted to a separate ProcessModelRecord with notation BPMN_2_0 and the supplied domain classification. catalogue_xml Raw catalogue XML string. domain Domain to assign to imported models. One FimModelResult per <prozess> element found. catalogue_xml Input parameter. domain Input parameter. Return value.

#### `std::vector< FimModelResult > importFromFitkoApi(std::string_view api_base_url, ProcessDomain domain=ProcessDomain::ADMINISTRATION)`
- Source: `include/process/fim_importer.h`:156
- Brief: Fetch and import process models from the FITKO REST API.
- Parameters:
  - `api_base_url` (std::string_view): Input parameter.
  - `domain` (ProcessDomain): Input parameter.
- Return: One FimModelResult per model retrieved.
- Details: ───────────────────────────────────────────────────────────────────────────── FimImporter::importFromFitkoApi ───────────────────────────────────────────────────────────────────────────── When an HttpFetchFn has been injected via setHttpFetchFn(), calls api_base_url + "/prozesse" and parses the JSON response envelope (expected: {"items": [{"bpmnXml": "..."}]}), importing each BPMN payload via importSingleModel(). Without an injected fetch function, returns a single error result explaining that HTTP transport is not configured. api_base_url Base URL of the FITKO FIM API (without trailing slash). domain Domain classification. One FimModelResult per model retrieved. This function emits SPDLOG_WARN entries for non-fatal API errors (e.g. individual model payloads that fail to parse). api_base_url Input parameter. domain Input parameter. Return value.

#### `FimModelResult importSingleModel(std::string_view bpmn_xml, ProcessDomain domain=ProcessDomain::ADMINISTRATION)`
- Source: `include/process/fim_importer.h`:135
- Brief: Import a single FIM BPMN 2.0 XML document.
- Parameters:
  - `bpmn_xml` (std::string_view): Input parameter.
  - `domain` (ProcessDomain): Input parameter.
- Return: FimModelResult with populated record on success.
- Details: ───────────────────────────────────────────────────────────────────────────── FimImporter::importSingleModel ───────────────────────────────────────────────────────────────────────────── Delegates BPMN parsing to BpmnSerializer and wraps the result in a ProcessModelRecord. bpmn_xml BPMN 2.0 XML string (FIM-compliant). domain Domain classification for the model. FimModelResult with populated record on success. bpmn_xml Input parameter. domain Input parameter. Return value.

#### `HttpFetchFn makeCurlHttpFetchFn()`
- Source: `include/process/fim_importer.h`:108
- Brief: Create a libcurl-backed HttpFetchFn for use with setHttpFetchFn().
- Parameters: none
- Return: A ready-to-use HttpFetchFn or an empty function when libcurl is unavailable.
- Details: ───────────────────────────────────────────────────────────────────────────── Factory: real libcurl-backed HttpFetchFn Guarded by THEMIS_FIM_HAS_CURL (set when THEMIS_HAS_CURL or THEMIS_ENABLE_CURL is defined and libcurl is linked). When built with THEMIS_HAS_CURL defined (libcurl linked), returns a concrete HttpFetchFn that performs a real HTTP GET using libcurl with a 10-second timeout and TLS peer verification enabled. The returned fn is safe to use from multiple threads (each call owns its own CURL handle). When built without libcurl, returns an empty std::function so that importFromFitkoApi() falls through to the "not implemented" fallback. Usage at startup: importer.setHttpFetchFn(FimImporter::makeCurlHttpFetchFn()); A ready-to-use HttpFetchFn or an empty function when libcurl is unavailable. Return value. Returns an empty function otherwise. ─────────────────────────────────────────────────────────────────────────────

#### `void setHttpFetchFn(HttpFetchFn fn)`
- Source: `include/process/fim_importer.h`:87
- Brief: Inject an HTTP fetch backend for importFromFitkoApi().
- Parameters:
  - `fn` (HttpFetchFn): Callable that performs the HTTP GET and returns the response body.
- Details: When set, importFromFitkoApi() delegates the HTTP GET call to fn instead of returning the "not yet implemented" error result. fn Callable that performs the HTTP GET and returns the response body.

### themis::process::FirstWriteWinsResolver

#### `std::string GetName() const override`
- Source: `include/process/process_conflict_resolution_callback.h`:275
- Brief: Optional: Get resolver name for logging/diagnostics.
- Parameters: none
- Return: Resolver name (e.g., "MyApplicationResolver")
- Details: Resolver name (e.g., "MyApplicationResolver")

#### `std::string Resolve(const ConflictMetadata &metadata) override`
- Source: `include/process/process_conflict_resolution_callback.h`:274
- Brief: Resolve conflict between two model versions.
- Parameters:
  - `metadata` (const ConflictMetadata &): Conflict metadata (versions, timestamps, senders, etc.)
- Return: ID of winning version (must be either metadata.v1_id or metadata.v2_id)
- Throws:
  - std::exception: If resolution fails; caught by consensus layer (LWW fallback)
- Details: metadata Conflict metadata (versions, timestamps, senders, etc.) ID of winning version (must be either metadata.v1_id or metadata.v2_id) std::exception If resolution fails; caught by consensus layer (LWW fallback) Must complete within 5 seconds; timeout triggers LWW fallback Result must be deterministic (same metadata → same result) Returned ID must equal either metadata.v1_id or metadata.v2_id

### themis::process::FirstWriteWinsStrategy

#### `std::string ResolveConflict(const ConflictMetadata &metadata) override`
- Source: `src/process/process_conflict_resolver.cpp`:100
- Brief: Resolve Conflict.
- Parameters:
  - `metadata` (const ConflictMetadata &): Input parameter.
- Return: Return value.
- Details: metadata Input parameter. Return value.

### themis::process::GossipVersionVector

#### `bool happensBefore(const GossipVersionVector &other) const`
- Source: `include/process/federated_consensus_contract.h`:461
- Brief: Check if this version vector has seen all events in another.
- Parameters:
  - `other` (const GossipVersionVector &): Version vector to compare
- Return: true if this >= other (elementwise)
- Details: other Version vector to compare true if this >= other (elementwise)

#### `void increment(const std::string &shard_id)`
- Source: `include/process/federated_consensus_contract.h`:434
- Brief: Increment clock for a given shard.
- Parameters:
  - `shard_id` (const std::string &): Shard to increment
- Details: shard_id Shard to increment

#### `void merge(const GossipVersionVector &other)`
- Source: `include/process/federated_consensus_contract.h`:446
- Brief: Merge with another version vector (take maximum per shard).
- Parameters:
  - `other` (const GossipVersionVector &): Version vector to merge
- Details: other Version vector to merge

### themis::process::IConflictResolutionPlugin

#### `std::string name() const noexcept=0`
- Source: `include/process/conflict_resolution_plugin.h`:306
- Brief: Get human-readable name of this plugin.
- Parameters: none
- Return: Plugin name (for logging and debugging)
- Details: Plugin name (for logging and debugging)

#### `ConflictResolutionResult resolve(const ModelVersion &local, const ModelVersion &remote, const ConflictContext &ctx)=0`
- Source: `include/process/conflict_resolution_plugin.h`:296
- Brief: Resolve a conflict between two model versions.
- Parameters:
  - `local` (const ModelVersion &): Local model version
  - `remote` (const ModelVersion &): Remote model version
  - `ctx` (const ConflictContext &): Conflict context (includes base version if available)
- Return: Conflict resolution result with chosen strategy
- Details: Called when process module detects a conflict during federation sync or replication. Must complete within < 10 ms; timeout triggers fallback to LWW. local Local model version remote Remote model version ctx Conflict context (includes base version if available) Conflict resolution result with chosen strategy

#### `bool validateMerge(const ModelVersion &merged) const noexcept`
- Source: `include/process/conflict_resolution_plugin.h`:316
- Brief: Validate that a merged model is consistent after merge.
- Parameters:
  - `merged` (const ModelVersion &): Merged model to validate
- Return: true if model is valid (passes schema, no cycles, etc.)
- Details: Optional; called by process module after merge to ensure result is valid. merged Merged model to validate true if model is valid (passes schema, no cycles, etc.)

#### `~IConflictResolutionPlugin()=default`
- Source: `include/process/conflict_resolution_plugin.h`:283
- Brief: n/a
- Parameters: none

### themis::process::IConflictResolutionStrategy

#### `std::string ResolveConflict(const ConflictMetadata &metadata)=0`
- Source: `src/process/process_conflict_resolver.cpp`:77
- Brief: Resolve Conflict.
- Parameters:
  - `metadata` (const ConflictMetadata &): Input parameter.
- Return: Return value.
- Details: metadata Input parameter. Return value.

#### `~IConflictResolutionStrategy()=default`
- Source: `src/process/process_conflict_resolver.cpp`:70
- Brief: IConflict Resolution Strategy.
- Parameters: none
- Return: Return value.
- Details: Return value.

### themis::process::ILLMProcessAdapter

#### `LLMDescriptorBackend activeBackend() const =0`
- Source: `include/process/llm_process_adapter.h`:53
- Brief: n/a
- Parameters: none

#### `std::vector< ProcessDescriptor > generateBatch(const std::vector< std::pair< std::string, std::string > > &activity_name_pairs, const std::string &process_context="")=0`
- Source: `include/process/llm_process_adapter.h`:50
- Brief: n/a
- Parameters:
  - `activity_name_pairs` (const std::vector< std::pair< std::string, std::string > > &): n/a
  - `process_context` (const std::string &): n/a

#### `ProcessDescriptor generateDescriptor(const std::string &activity_id, const std::string &activity_name, const std::string &process_context="")=0`
- Source: `include/process/llm_process_adapter.h`:46
- Brief: n/a
- Parameters:
  - `activity_id` (const std::string &): n/a
  - `activity_name` (const std::string &): n/a
  - `process_context` (const std::string &): n/a

#### `bool isAvailable() const =0`
- Source: `include/process/llm_process_adapter.h`:54
- Brief: n/a
- Parameters: none

#### `~ILLMProcessAdapter()=default`
- Source: `include/process/llm_process_adapter.h`:45
- Brief: n/a
- Parameters: none

### themis::process::IProcessTracer

#### `std::unique_ptr< ISpan > createConflictResolutionSpan(const std::string &conflict_type, const std::string &local_version, const std::string &remote_version, const std::string &plugin_name, const TraceContext &parent_context)=0`
- Source: `include/process/federated_span_contract.h`:466
- Brief: Create conflict resolution span.
- Parameters:
  - `conflict_type` (const std::string &): Type of conflict
  - `local_version` (const std::string &): Local version being resolved
  - `remote_version` (const std::string &): Remote version being resolved
  - `plugin_name` (const std::string &): Conflict resolution plugin name (if used)
  - `parent_context` (const TraceContext &): Parent trace context (from federation sync)
- Return: Span for conflict resolution
- Details: conflict_type Type of conflict local_version Local version being resolved remote_version Remote version being resolved plugin_name Conflict resolution plugin name (if used) parent_context Parent trace context (from federation sync) Span for conflict resolution

#### `std::unique_ptr< ISpan > createConsensusRpcSpan(const std::string &rpc_type, const std::string &remote_shard_id, const TraceContext &parent_context)=0`
- Source: `include/process/federated_span_contract.h`:482
- Brief: Create consensus RPC span.
- Parameters:
  - `rpc_type` (const std::string &): RPC type ("request_vote", "append_entries", "prepare", "accept")
  - `remote_shard_id` (const std::string &): Destination shard ID
  - `parent_context` (const TraceContext &): Parent trace context (from federation sync)
- Return: Span for RPC call
- Details: rpc_type RPC type ("request_vote", "append_entries", "prepare", "accept") remote_shard_id Destination shard ID parent_context Parent trace context (from federation sync) Span for RPC call

#### `std::unique_ptr< ISpan > createFederationSyncSpan(const std::string &sync_id, const std::string &participating_shards, const std::string &principal_id, const std::string &consensus_type)=0`
- Source: `include/process/federated_span_contract.h`:449
- Brief: Create federation sync span.
- Parameters:
  - `sync_id` (const std::string &): Unique sync operation ID
  - `participating_shards` (const std::string &): CSV of shard IDs participating in sync
  - `principal_id` (const std::string &): Principal triggering sync
  - `consensus_type` (const std::string &): Consensus protocol ("raft", "paxos", "gossip")
- Return: Span for federation sync
- Details: sync_id Unique sync operation ID participating_shards CSV of shard IDs participating in sync principal_id Principal triggering sync consensus_type Consensus protocol ("raft", "paxos", "gossip") Span for federation sync

#### `std::unique_ptr< ISpan > createImportSpan(const std::string &model_id, const std::string &shard_id, const std::string &principal_id, const std::optional< TraceContext > &parent_context=std::nullopt)=0`
- Source: `include/process/federated_span_contract.h`:415
- Brief: Create import span.
- Parameters:
  - `model_id` (const std::string &): Model being imported
  - `shard_id` (const std::string &): Local shard ID
  - `principal_id` (const std::string &): Principal performing import
  - `parent_context` (const std::optional< TraceContext > &): Optional parent trace context
- Return: Span for import operation
- Details: model_id Model being imported shard_id Local shard ID principal_id Principal performing import parent_context Optional parent trace context Span for import operation

#### `std::unique_ptr< ISpan > createLinkSpan(const std::string &model_id, const std::string &target_id, const std::string &shard_id, const std::string &principal_id, const std::optional< TraceContext > &parent_context=std::nullopt)=0`
- Source: `include/process/federated_span_contract.h`:432
- Brief: Create link span.
- Parameters:
  - `model_id` (const std::string &): Source model ID
  - `target_id` (const std::string &): Target entity ID
  - `shard_id` (const std::string &): Local shard ID
  - `principal_id` (const std::string &): Principal creating link
  - `parent_context` (const std::optional< TraceContext > &): Optional parent trace context
- Return: Span for link creation
- Details: model_id Source model ID target_id Target entity ID shard_id Local shard ID principal_id Principal creating link parent_context Optional parent trace context Span for link creation

#### `std::unique_ptr< ISpan > createRecoverySpan(const std::string &model_id, int64_t target_timestamp_ns, const std::string &shard_id, const std::string &principal_id)=0`
- Source: `include/process/federated_span_contract.h`:497
- Brief: Create recovery span.
- Parameters:
  - `model_id` (const std::string &): Model being recovered
  - `target_timestamp_ns` (int64_t): Target timestamp for recovery
  - `shard_id` (const std::string &): Local shard ID
  - `principal_id` (const std::string &): Principal performing recovery
- Return: Span for point-in-time recovery
- Details: model_id Model being recovered target_timestamp_ns Target timestamp for recovery shard_id Local shard ID principal_id Principal performing recovery Span for point-in-time recovery

#### `IProcessTracer & instance()`
- Source: `include/process/federated_span_contract.h`:508
- Brief: Get singleton instance of process tracer.
- Parameters: none
- Return: Reference to global process tracer
- Details: Reference to global process tracer

#### `~IProcessTracer()=default`
- Source: `include/process/federated_span_contract.h`:404
- Brief: n/a
- Parameters: none

### themis::process::ISpan

#### `void addEvent(const SpanEvent &event)=0`
- Source: `include/process/federated_span_contract.h`:376
- Brief: Add an event (milestone) to this span.
- Parameters:
  - `event` (const SpanEvent &): SpanEvent to record
- Details: event SpanEvent to record

#### `void end()=0`
- Source: `include/process/federated_span_contract.h`:390
- Brief: End this span (mark as complete).
- Parameters: none
- Details: After calling end(), span is immutable. No further modifications are allowed.

#### `void setAttribute(const std::string &key, const std::string &value)=0`
- Source: `include/process/federated_span_contract.h`:370
- Brief: Add an attribute to this span.
- Parameters:
  - `key` (const std::string &): Attribute name
  - `value` (const std::string &): Attribute value
- Details: key Attribute name value Attribute value

#### `void setStatus(SpanStatus status, const std::string &description="")=0`
- Source: `include/process/federated_span_contract.h`:383
- Brief: Set span status (success, error, cancelled).
- Parameters:
  - `status` (SpanStatus): SpanStatus
  - `description` (const std::string &): Optional error description
- Details: status SpanStatus description Optional error description

#### `TraceContext traceContext() const noexcept=0`
- Source: `include/process/federated_span_contract.h`:363
- Brief: Get trace context (for RPC propagation).
- Parameters: none
- Return: TraceContext containing trace_id and span_id
- Details: TraceContext containing trace_id and span_id

#### `~ISpan()=default`
- Source: `include/process/federated_span_contract.h`:357
- Brief: n/a
- Parameters: none

### themis::process::IXpdlImporter

#### `std::string exportToXpdl(const std::string &package_id)=0`
- Source: `include/process/xpdl_importer.h`:43
- Brief: n/a
- Parameters:
  - `package_id` (const std::string &): n/a

#### `XpdlImportResult importFromFile(const std::string &xpdl_path)=0`
- Source: `include/process/xpdl_importer.h`:41
- Brief: n/a
- Parameters:
  - `xpdl_path` (const std::string &): n/a

#### `XpdlImportResult importFromString(const std::string &xpdl_xml)=0`
- Source: `include/process/xpdl_importer.h`:42
- Brief: n/a
- Parameters:
  - `xpdl_xml` (const std::string &): n/a

#### `bool validateXpdl(const std::string &xpdl_xml, std::vector< std::string > &errors)=0`
- Source: `include/process/xpdl_importer.h`:44
- Brief: n/a
- Parameters:
  - `xpdl_xml` (const std::string &): n/a
  - `errors` (std::vector< std::string > &): n/a

#### `std::string xpdlVersion() const`
- Source: `include/process/xpdl_importer.h`:45
- Brief: n/a
- Parameters: none

#### `~IXpdlImporter()=default`
- Source: `include/process/xpdl_importer.h`:40
- Brief: n/a
- Parameters: none

### themis::process::LWWResolutionStrategy

#### `ConflictResolutionResult resolve(const ModelVersion &local, const ModelVersion &remote) noexcept`
- Source: `include/process/conflict_resolution_plugin.h`:245
- Brief: Resolve conflict using Last-Write-Wins.
- Parameters:
  - `local` (const ModelVersion &): Local model version
  - `remote` (const ModelVersion &): Remote model version
- Return: Result with LOCAL_WINS or REMOTE_WINS
- Details: local Local model version remote Remote model version Result with LOCAL_WINS or REMOTE_WINS

### themis::process::LargeContextSizeStress

#### `std::string_view describe() const`
- Source: `include/process/process_stress_scenarios.h`:370
- Brief: n/a
- Parameters: none

#### `std::string generateTestInput() const`
- Source: `include/process/process_stress_scenarios.h`:365
- Brief: Generate a test input with large graph.
- Parameters: none

#### `bool isSuccess(const StressScenarioResult &result) const`
- Source: `include/process/process_stress_scenarios.h`:368
- Brief: Check if result is success (truncated gracefully).
- Parameters:
  - `result` (const StressScenarioResult &): n/a

### themis::process::LargeElementCountStress

#### `std::string_view describe() const`
- Source: `include/process/process_stress_scenarios.h`:165
- Brief: n/a
- Parameters: none

#### `std::string generateTestInput() const`
- Source: `include/process/process_stress_scenarios.h`:160
- Brief: Generate a test input with large element count.
- Parameters: none

#### `bool isSuccess(const StressScenarioResult &result) const`
- Source: `include/process/process_stress_scenarios.h`:163
- Brief: Check if result is success for this scenario.
- Parameters:
  - `result` (const StressScenarioResult &): n/a

### themis::process::LastWriteWinsResolver

#### `std::string GetName() const override`
- Source: `include/process/process_conflict_resolution_callback.h`:264
- Brief: Optional: Get resolver name for logging/diagnostics.
- Parameters: none
- Return: Resolver name (e.g., "MyApplicationResolver")
- Details: Resolver name (e.g., "MyApplicationResolver")

#### `std::string Resolve(const ConflictMetadata &metadata) override`
- Source: `include/process/process_conflict_resolution_callback.h`:263
- Brief: Resolve conflict between two model versions.
- Parameters:
  - `metadata` (const ConflictMetadata &): Conflict metadata (versions, timestamps, senders, etc.)
- Return: ID of winning version (must be either metadata.v1_id or metadata.v2_id)
- Throws:
  - std::exception: If resolution fails; caught by consensus layer (LWW fallback)
- Details: metadata Conflict metadata (versions, timestamps, senders, etc.) ID of winning version (must be either metadata.v1_id or metadata.v2_id) std::exception If resolution fails; caught by consensus layer (LWW fallback) Must complete within 5 seconds; timeout triggers LWW fallback Result must be deterministic (same metadata → same result) Returned ID must equal either metadata.v1_id or metadata.v2_id

### themis::process::LastWriteWinsStrategy

#### `std::string ResolveConflict(const ConflictMetadata &metadata) override`
- Source: `src/process/process_conflict_resolver.cpp`:83
- Brief: Resolve Conflict.
- Parameters:
  - `metadata` (const ConflictMetadata &): Input parameter.
- Return: Return value.
- Details: metadata Input parameter. Return value.

### themis::process::LinkAttributeMutationStress

#### `std::string_view describe() const`
- Source: `include/process/process_stress_scenarios.h`:319
- Brief: n/a
- Parameters: none

#### `std::string generateTestInput() const`
- Source: `include/process/process_stress_scenarios.h`:314
- Brief: Generate a test input with concurrent mutations.
- Parameters: none

#### `bool isSuccess(const StressScenarioResult &result) const`
- Source: `include/process/process_stress_scenarios.h`:317
- Brief: Check if result is success (LWW resolution applied).
- Parameters:
  - `result` (const StressScenarioResult &): n/a

### themis::process::LinkConsistencyDeterminismSpec

#### `std::string_view describe()`
- Source: `include/process/process_determinism_spec.h`:253
- Brief: n/a
- Parameters: none

### themis::process::LlmProcessDescriptor

#### `std::string buildConformancePrompt(const nlohmann::json &descriptor, const nlohmann::json &observed_trace)`
- Source: `include/process/llm_process_descriptor.h`:153
- Brief: Build a conformance-checking prompt for an LLM.
- Parameters:
  - `descriptor` (const nlohmann::json &): Input parameter.
  - `observed_trace` (const nlohmann::json &): Input parameter.
- Return: Prompt string ready to send to an LLM.
- Details: Build Conformance Prompt. Combines a process model descriptor with an observed execution trace and asks the LLM to identify deviations, compliance violations, and SLA breaches. descriptor Output of generate(). observed_trace JSON array of executed activity IDs in order. Prompt string ready to send to an LLM. descriptor Input parameter. observed_trace Input parameter. Return value.

#### `std::string buildSystemPrompt(const nlohmann::json &descriptor)`
- Source: `include/process/llm_process_descriptor.h`:126
- Brief: Build a compact system-prompt string from a descriptor.
- Parameters:
  - `descriptor` (const nlohmann::json &): Input parameter.
- Return: Plain-text context block.
- Details: Build System Prompt. Produces a condensed text block (< 2000 tokens) that can be prepended to any LLM prompt to give the model full context about a process. descriptor Output of generate(). Plain-text context block. descriptor Input parameter. Return value. Calls: value(), empty(), contains(), str().

#### `nlohmann::json edgeToJson_(const nlohmann::json &edge_doc)`
- Source: `include/process/llm_process_descriptor.h`:164
- Brief: Edge To Json.
- Parameters:
  - `edge_doc` (const nlohmann::json &): Input parameter.
- Return: Return value.
- Details: edge_doc Input parameter. Return value. Calls: value(), empty().

#### `nlohmann::json generate(const ProcessModelRecord &record)`
- Source: `include/process/llm_process_descriptor.h`:101
- Brief: Generate a full LLM descriptor for a process model record.
- Parameters:
  - `record` (const ProcessModelRecord &): Input parameter.
- Return: JSON descriptor object ready for LLM consumption.
- Details: Generate. record The process model record (from ProcessModelManager::load). JSON descriptor object ready for LLM consumption. record Input parameter. Return value.

#### `nlohmann::json generate(const ProcessModelRecord &record, const Config &cfg)`
- Source: `include/process/llm_process_descriptor.h`:112
- Brief: Generate a full LLM descriptor for a process model record.
- Parameters:
  - `record` (const ProcessModelRecord &): Input parameter.
  - `cfg` (const Config &): Input parameter.
- Return: JSON descriptor object ready for LLM consumption.
- Details: Generate. record The process model record (from ProcessModelManager::load). cfg Optional generation configuration. JSON descriptor object ready for LLM consumption. record Input parameter. cfg Input parameter. Return value.

#### `nlohmann::json nodeToJson_(const nlohmann::json &node_doc, const Config &cfg)`
- Source: `include/process/llm_process_descriptor.h`:160
- Brief: Node To Json.
- Parameters:
  - `node_doc` (const nlohmann::json &): Input parameter.
  - `cfg` (const Config &): Input parameter.
- Return: Return value.
- Details: node_doc Input parameter. cfg Input parameter. Return value. Calls: value(), truncate_(), contains().

#### `nlohmann::json summarizeList(const std::vector< ProcessModelRecord > &records, std::string_view language="de")`
- Source: `include/process/llm_process_descriptor.h`:137
- Brief: Summarise multiple process models for a list/comparison prompt.
- Parameters:
  - `records` (const std::vector< ProcessModelRecord > &): Input parameter.
  - `language` (std::string_view): Input parameter.
- Return: JSON array of compact summary objects.
- Details: Summarize List. Generates a compact summary of all provided records, sorted by domain. records Collection of process model records. language ISO 639-1 language code ("de" or "en"). JSON array of compact summary objects. records Input parameter. language Input parameter. Return value.

#### `std::string truncate_(std::string_view s, size_t max_chars)`
- Source: `include/process/llm_process_descriptor.h`:159
- Brief: Truncate.
- Parameters:
  - `s` (std::string_view): Input parameter.
  - `max_chars` (size_t): Input parameter.
- Return: Return value.
- Details: s Input parameter. max_chars Input parameter. Return value. Calls: size(), std::string(), substr().

### themis::process::LockFreeBatchQueue

#### `LockFreeBatchQueue(size_t capacity)`
- Source: `include/process/lock_free_linker_contract.h`:407
- Brief: Create batch queue with capacity.
- Parameters:
  - `capacity` (size_t): Maximum entries before blocking (or backoff)
- Details: capacity Maximum entries before blocking (or backoff)

#### `size_t capacity() const noexcept`
- Source: `include/process/lock_free_linker_contract.h`:437
- Brief: Get queue capacity.
- Parameters: none
- Return: Maximum entries before blocking
- Details: Maximum entries before blocking

#### `size_t depth() const noexcept`
- Source: `include/process/lock_free_linker_contract.h`:431
- Brief: Get approximate queue depth (may be stale).
- Parameters: none
- Return: Approximate number of buffered entries
- Details: Approximate number of buffered entries

#### `std::vector< BatchQueueEntry > drain() noexcept`
- Source: `include/process/lock_free_linker_contract.h`:425
- Brief: Dequeue all buffered entries (atomic swap).
- Parameters: none
- Return: Vector of entries to process
- Details: Called by consumer thread; atomically swaps current buffer with new empty buffer. Returns snapshot of entries; consumer can process without blocking producers. Vector of entries to process

#### `bool tryEnqueue(const BatchQueueEntry &entry) noexcept`
- Source: `include/process/lock_free_linker_contract.h`:415
- Brief: Try to enqueue an entry (non-blocking).
- Parameters:
  - `entry` (const BatchQueueEntry &): Entry to enqueue
- Return: true if enqueued; false if queue full (caller should retry/backoff)
- Details: entry Entry to enqueue true if enqueued; false if queue full (caller should retry/backoff)

### themis::process::LockFreeHashTable

#### `bool erase(const Key &key)`
- Source: `include/process/lock_free_linker_contract.h`:355
- Brief: Delete a key-value pair.
- Parameters:
  - `key` (const Key &): Key to delete
- Return: true if deleted; false if not found
- Details: Marks entry for deletion; actual reclamation deferred to epoch-safe point. key Key to delete true if deleted; false if not found

#### `void forEach(Visitor visitor) const`
- Source: `include/process/lock_free_linker_contract.h`:371
- Brief: Iterate over all entries (snapshot).
- Parameters:
  - `visitor` (Visitor): Callable(const Key&, const Value&) called for each entry
- Details: Note: concurrent modifications may not be visible. visitor Callable(const Key&, const Value&) called for each entry

#### `bool insert(const Key &key, const Value &value)`
- Source: `include/process/lock_free_linker_contract.h`:335
- Brief: Insert a key-value pair (or update if exists).
- Parameters:
  - `key` (const Key &): Key to insert
  - `value` (const Value &): Value to insert
- Return: true if inserted; false if key already existed (value updated)
- Details: Atomic operation; concurrent calls are serialized at key level. key Key to insert value Value to insert true if inserted; false if key already existed (value updated)

#### `std::optional< Value > lookup(const Key &key) const`
- Source: `include/process/lock_free_linker_contract.h`:345
- Brief: Lookup a value by key.
- Parameters:
  - `key` (const Key &): Key to lookup
- Return: Value if found; std::nullopt otherwise
- Details: Lock-free read; may return stale value briefly during concurrent writes. key Key to lookup Value if found; std::nullopt otherwise

#### `size_t size() const noexcept`
- Source: `include/process/lock_free_linker_contract.h`:361
- Brief: Get number of entries (approximate; may be stale).
- Parameters: none
- Return: Approximate entry count
- Details: Approximate entry count

### themis::process::LockFreeLinkEntry

#### `void acquireRef() noexcept`
- Source: `include/process/lock_free_linker_contract.h`:211
- Brief: Increment reference count (prevents reclamation while referenced).
- Parameters: none

#### `uint32_t getRefCount() const noexcept`
- Source: `include/process/lock_free_linker_contract.h`:226
- Brief: Get current reference count.
- Parameters: none
- Return: Reference count
- Details: Reference count

#### `void releaseRef() noexcept`
- Source: `include/process/lock_free_linker_contract.h`:218
- Brief: Decrement reference count; may trigger reclamation.
- Parameters: none

### themis::process::LockFreeLinkerMetrics

#### `nlohmann::json toJson() const`
- Source: `include/process/lock_free_linker_contract.h`:521
- Brief: Get metrics snapshot (for reporting).
- Parameters: none
- Return: JSON representation of metrics
- Details: JSON representation of metrics

### themis::process::MalformedXmlRecoveryStress

#### `std::string_view describe() const`
- Source: `include/process/process_stress_scenarios.h`:192
- Brief: n/a
- Parameters: none

#### `std::string generateTestInputBrokenNesting() const`
- Source: `include/process/process_stress_scenarios.h`:186
- Brief: n/a
- Parameters: none

#### `std::string generateTestInputInvalidAttribute() const`
- Source: `include/process/process_stress_scenarios.h`:187
- Brief: n/a
- Parameters: none

#### `std::string generateTestInputMissingAttribute() const`
- Source: `include/process/process_stress_scenarios.h`:185
- Brief: Generate a test input with specific malformation type.
- Parameters: none

#### `bool isSuccess(const StressScenarioResult &result) const`
- Source: `include/process/process_stress_scenarios.h`:190
- Brief: Check if result is success (graceful failure with error) for this scenario.
- Parameters:
  - `result` (const StressScenarioResult &): n/a

### themis::process::ModelDelta

#### `size_t estimatedSize() const noexcept`
- Source: `include/process/model_history_contract.h`:232
- Brief: Estimate size of delta in bytes.
- Parameters: none
- Return: Approximate byte size
- Details: Approximate byte size

### themis::process::ModelUpdateDeterminismSpec

#### `std::string_view describe()`
- Source: `include/process/process_determinism_spec.h`:224
- Brief: n/a
- Parameters: none

### themis::process::ObjectCentricTracer

#### `ObjectCentricTracer(ProcessLinker &linker, ProcessModelManager &model_manager)`
- Source: `include/process/object_centric_tracer.h`:82
- Brief: n/a
- Parameters:
  - `linker` (ProcessLinker &): ProcessLinker to retrieve attachments.
  - `model_manager` (ProcessModelManager &): ProcessModelManager to load model definitions.
- Details: linker ProcessLinker to retrieve attachments. model_manager ProcessModelManager to load model definitions.

#### `ConvergenceDivergenceResult analyze(std::string_view model_id) const`
- Source: `include/process/object_centric_tracer.h`:158
- Brief: Analyse a process model for convergence / divergence patterns.
- Parameters:
  - `model_id` (std::string_view): Process model identifier.
- Return: ConvergenceDivergenceResult with classified nodes.
- Details: model_id Process model identifier. ConvergenceDivergenceResult with classified nodes.

#### `nlohmann::json buildOcelLog(std::string_view instance_id) const`
- Source: `include/process/object_centric_tracer.h`:116
- Brief: Build an OCEL 2.0 compatible JSON log for a process instance.
- Parameters:
  - `instance_id` (std::string_view): Process instance identifier.
- Return: OCEL 2.0 JSON object.
- Details: Format: { "ocel:global-log":{ "ocel:attribute-names":[], "ocel:object-types":["documents","metadata",...] }, "ocel:events":[ { "ocel:id":"...", "ocel:activity":"HAS_DOCUMENT", "ocel:timestamp":1234567890, "ocel:omap":{"documents":["doc-1"]}, "ocel:vmap":{} },... ], "ocel:objects":{ "doc-1":{"ocel:type":"documents"}, ... } } instance_id Process instance identifier. OCEL 2.0 JSON object.

#### `nlohmann::json computeDfmg(std::string_view model_id, std::string_view object_type) const`
- Source: `include/process/object_centric_tracer.h`:136
- Brief: Compute the Directly-Follows Multigraph (DFMG) for a given object type within a process model.
- Parameters:
  - `model_id` (std::string_view): Process model identifier.
  - `object_type` (std::string_view): Object type (collection name) to trace.
- Return: JSON: { "object_type":"documents", "nodes":["node-A","node-B",...], "arcs":[{"from":"node-A","to":"node-B","frequency":3},...] }
- Details: Loads the process model, iterates all edges, and accumulates arcs with their frequency. Runs in O(n) over the edge list. model_id Process model identifier. object_type Object type (collection name) to trace. JSON: { "object_type":"documents", "nodes":["node-A","node-B",...], "arcs":[{"from":"node-A","to":"node-B","frequency":3},...] }

### themis::process::OcelExporter

#### `OcelExporter(RocksDBWrapper &db, ProcessGraphManager &engine, ProcessModelManager &models, ProcessLinker &linker)`
- Source: `include/process/ocel_exporter.h`:70
- Brief: n/a
- Parameters:
  - `db` (RocksDBWrapper &): n/a
  - `engine` (ProcessGraphManager &): n/a
  - `models` (ProcessModelManager &): n/a
  - `linker` (ProcessLinker &): n/a

#### `nlohmann::json buildEvents_(const ProcessInstance &inst) const`
- Source: `include/process/ocel_exporter.h`:127
- Brief: Build the OCEL 2.0 event list from a process instance.
- Parameters:
  - `inst` (const ProcessInstance &): n/a

#### `nlohmann::json buildObjects_(std::string_view instance_id) const`
- Source: `include/process/ocel_exporter.h`:132
- Brief: Build the OCEL 2.0 object list from instance attachments.
- Parameters:
  - `instance_id` (std::string_view): n/a

#### `nlohmann::json deriveEventTypes_(const nlohmann::json &events)`
- Source: `include/process/ocel_exporter.h`:142
- Brief: Derive the set of eventTypes from all events.
- Parameters:
  - `events` (const nlohmann::json &): n/a

#### `nlohmann::json deriveObjectTypes_(const nlohmann::json &objects)`
- Source: `include/process/ocel_exporter.h`:137
- Brief: Derive the set of objectTypes from all objects.
- Parameters:
  - `objects` (const nlohmann::json &): n/a

#### `nlohmann::json exportFiltered(std::string_view model_id, int64_t from_ms, int64_t to_ms) const`
- Source: `include/process/ocel_exporter.h`:114
- Brief: Export instances of a process model within a time window.
- Parameters:
  - `model_id` (std::string_view): Process model identifier.
  - `from_ms` (int64_t): Start of time window (epoch milliseconds, inclusive).
  - `to_ms` (int64_t): End of time window (epoch milliseconds, inclusive).
- Return: OCEL 2.0 JSON; empty {} if no matching instances.
- Details: model_id Process model identifier. from_ms Start of time window (epoch milliseconds, inclusive). to_ms End of time window (epoch milliseconds, inclusive). OCEL 2.0 JSON; empty {} if no matching instances.

#### `nlohmann::json exportInstance(std::string_view instance_id) const`
- Source: `include/process/ocel_exporter.h`:88
- Brief: Export a single process instance as an OCEL 2.0 JSON log.
- Parameters:
  - `instance_id` (std::string_view): Process instance identifier.
- Return: OCEL 2.0 JSON object; empty {} on error.
- Details: The returned document contains all events derived from token traversal history and all attached objects as OCEL objects. instance_id Process instance identifier. OCEL 2.0 JSON object; empty {} on error.

#### `nlohmann::json exportModel(std::string_view model_id) const`
- Source: `include/process/ocel_exporter.h`:102
- Brief: Export all instances of a process model as a combined OCEL 2.0 JSON log.
- Parameters:
  - `model_id` (std::string_view): Process model / definition identifier.
- Return: OCEL 2.0 JSON; empty {} if no instances found.
- Details: All instances that reference model_id are included in a single event log, enabling cross-case analytics. model_id Process model / definition identifier. OCEL 2.0 JSON; empty {} if no instances found.

#### `std::string toIso8601_(int64_t epoch_ms)`
- Source: `include/process/ocel_exporter.h`:147
- Brief: Convert epoch ms to ISO-8601 timestamp string.
- Parameters:
  - `epoch_ms` (int64_t): n/a

### themis::process::OrphanedLinkResolutionStress

#### `std::string_view describe() const`
- Source: `include/process/process_stress_scenarios.h`:240
- Brief: n/a
- Parameters: none

#### `std::string generateTestInput() const`
- Source: `include/process/process_stress_scenarios.h`:235
- Brief: Generate a test input with orphaned links.
- Parameters: none

#### `bool isSuccess(const StressScenarioResult &result) const`
- Source: `include/process/process_stress_scenarios.h`:238
- Brief: Check if result is success (link stale but detectable) for this scenario.
- Parameters:
  - `result` (const StressScenarioResult &): n/a

### themis::process::ParserStateTracker

#### `ParserStateTracker(int32_t max_depth=kMaxModelNestingDepth, int32_t max_elements=kMaxModelElements, int64_t timeout_ms=kMaxOperationTimeoutMs)`
- Source: `include/process/serializer_hardening.h`:132
- Brief: Create a new parser state tracker.
- Parameters:
  - `max_depth` (int32_t): Maximum allowed nesting depth (kMaxModelNestingDepth).
  - `max_elements` (int32_t): Maximum allowed total elements (kMaxModelElements).
  - `timeout_ms` (int64_t): Maximum operation duration in milliseconds.
- Details: max_depth Maximum allowed nesting depth (kMaxModelNestingDepth). max_elements Maximum allowed total elements (kMaxModelElements). timeout_ms Maximum operation duration in milliseconds.

#### `bool enterScope()`
- Source: `include/process/serializer_hardening.h`:143
- Brief: Signal entry into a nested scope (sub-process, choice, etc.).
- Parameters: none
- Return: true if within depth limits, false if max depth exceeded.
- Details: Enter Scope. true if within depth limits, false if max depth exceeded. True when the operation succeeds. Implements enterScope without additional internal calls.

#### `bool exitScope()`
- Source: `include/process/serializer_hardening.h`:150
- Brief: Signal exit from a nested scope.
- Parameters: none
- Return: true if scope was properly nested, false on underflow.
- Details: Exit Scope. true if scope was properly nested, false on underflow. True when the operation succeeds. Implements exitScope without additional internal calls.

#### `int32_t getCurrentDepth() const`
- Source: `include/process/serializer_hardening.h`:171
- Brief: Get the current nesting depth.
- Parameters: none
- Return: Current depth (0 = root).
- Details: Current depth (0 = root).

#### `std::string getDiagnosticMessage() const`
- Source: `include/process/serializer_hardening.h`:192
- Brief: Get a diagnostic message describing the current state.
- Parameters: none
- Return: Human-readable state description.
- Details: Human-readable state description.

#### `int64_t getElapsedMs() const`
- Source: `include/process/serializer_hardening.h`:185
- Brief: Get elapsed time in milliseconds since creation.
- Parameters: none
- Return: Milliseconds elapsed.
- Details: Milliseconds elapsed.

#### `int32_t getElementCount() const`
- Source: `include/process/serializer_hardening.h`:178
- Brief: Get the total elements recorded so far.
- Parameters: none
- Return: Total element count.
- Details: Total element count.

#### `bool hasTimedOut() const`
- Source: `include/process/serializer_hardening.h`:164
- Brief: Check if the operation has exceeded the timeout window.
- Parameters: none
- Return: true if operation has timed out, false otherwise.
- Details: true if operation has timed out, false otherwise.

#### `bool recordElement()`
- Source: `include/process/serializer_hardening.h`:157
- Brief: Record the creation of a new element (node, edge, etc.).
- Parameters: none
- Return: true if within element limits, false if limit exceeded.
- Details: Record Element. true if within element limits, false if limit exceeded. True when the operation succeeds. Implements recordElement without additional internal calls.

### themis::process::ProcessAgenticRag

#### `ProcessAgenticRag(ProcessGraphRag &rag)`
- Source: `include/process/process_agentic_rag.h`:140
- Brief: Construct with default configuration.
- Parameters:
  - `rag` (ProcessGraphRag &): ProcessGraphRag reference (must outlive this object).
- Details: rag ProcessGraphRag reference (must outlive this object).

#### `ProcessAgenticRag(ProcessGraphRag &rag, const ProcessAgenticConfig &config)`
- Source: `include/process/process_agentic_rag.h`:147
- Brief: Construct with custom configuration.
- Parameters:
  - `rag` (ProcessGraphRag &): ProcessGraphRag reference.
  - `config` (const ProcessAgenticConfig &): Tuning parameters.
- Details: rag ProcessGraphRag reference. config Tuning parameters.

#### `ProcessAgenticRag(const ProcessAgenticRag &)=delete`
- Source: `include/process/process_agentic_rag.h`:152
- Brief: n/a
- Parameters:
  - `<unnamed>` (const ProcessAgenticRag &): n/a

#### `const ProcessAgenticConfig & config() const`
- Source: `include/process/process_agentic_rag.h`:197
- Brief: n/a
- Parameters: none

#### `std::vector< rag::judge::RetrievedDocument > encodeContext(const ProcessRagContext &ctx)`
- Source: `include/process/process_agentic_rag.h`:212
- Brief: n/a
- Parameters:
  - `ctx` (const ProcessRagContext &): n/a
- Details: Encode a ProcessRagContext into a vector of RetrievedDocument. This method safely encodes all context fields (prompt, subgraph, attachments, similar cases, missing documents) into RetrievedDocument objects. Safety notes: String concatenation using std::string::operator+ is bounds-safe (C++ standard) std::to_string() produces valid output strings with proper bounds checking std::move() on vectors is safe and transfers ownership without copying JSON dump(2) produces bounded output from the JSON serializer Complexity: O(n) where n = total fields in context

#### `ProcessAgenticResult iterativeQuery(std::string_view instance_id, std::string_view question)`
- Source: `include/process/process_agentic_rag.h`:172
- Brief: Run the iterative agentic Q&A loop for a process instance.
- Parameters:
  - `instance_id` (std::string_view): Identifier of the instance.
  - `question` (std::string_view): Input parameter.
- Return: ProcessAgenticResult with the final LLM prompt.
- Details: Iterative Query. The loop: Retrieves an initial process context via ProcessGraphRag::retrieve(). Encodes the context as RetrievedDocument objects. Runs the AgenticRAG loop to refine coverage. Returns the final enriched context and LLM prompt. instance_id Process instance ID. question Natural-language question (DE or EN). ProcessAgenticResult with the final LLM prompt. instance_id Identifier of the instance. question Input parameter. Return value.

#### `ProcessAgenticResult iterativeQueryForNode(std::string_view instance_id, std::string_view node_id, std::string_view question)`
- Source: `include/process/process_agentic_rag.h`:187
- Brief: Run for a specific node within a process instance.
- Parameters:
  - `instance_id` (std::string_view): Identifier of the instance.
  - `node_id` (std::string_view): Identifier of the node.
  - `question` (std::string_view): Input parameter.
- Return: Return value.
- Details: Iterative Query For Node. Like iterativeQuery() but seeds the initial retrieval from ProcessGraphRag::retrieveForNode(). instance_id Process instance ID. node_id Seed node ID. question Natural-language question. instance_id Identifier of the instance. node_id Identifier of the node. question Input parameter. Return value.

#### `ProcessRagContext mergeDocuments(ProcessRagContext ctx, const std::vector< rag::judge::RetrievedDocument > &extra_docs)`
- Source: `include/process/process_agentic_rag.h`:230
- Brief: ------------------------------------------------------------------------ mergeDocuments – incorporate extra docs back into context ------------------------------------------------------------------------
- Parameters:
  - `ctx` (ProcessRagContext): Input parameter.
  - `extra_docs` (const std::vector< rag::judge::RetrievedDocument > &): Input parameter.
- Return: Return value.
- Details: Merge additional documents back into a ProcessRagContext. Intelligently incorporates RetrievedDocument objects back into the context, avoiding duplicates and preserving structured fields. Implementation note: Uses O(n) set-based deduplication: Builds an unordered_set of existing IDs: O(n) For each new document, O(1) set lookup to check if duplicate Overall complexity: O(n + m) where n = existing docs, m = new docs The metadata map lookup uses std::map::find() which is O(log k) where k is the size of metadata per document (typically 4-6 fields). Safety: Uses RAII containers (vector, set, map) — no manual memory management. ctx Input parameter. extra_docs Input parameter. Return value.

#### `ProcessAgenticRag & operator=(const ProcessAgenticRag &)=delete`
- Source: `include/process/process_agentic_rag.h`:153
- Brief: n/a
- Parameters:
  - `<unnamed>` (const ProcessAgenticRag &): n/a

#### `ProcessAgenticResult runLoop(std::string_view instance_id, std::string_view question, ProcessRagContext initial_ctx)`
- Source: `include/process/process_agentic_rag.h`:240
- Brief: Run the full agentic loop given the initial context.
- Parameters:
  - `instance_id` (std::string_view): Identifier of the instance.
  - `question` (std::string_view): Input parameter.
  - `initial_ctx` (ProcessRagContext): Input parameter.
- Return: Return value.
- Details: Run Loop. instance_id Identifier of the instance. question Input parameter. initial_ctx Input parameter. Return value.

#### `void setConfig(const ProcessAgenticConfig &cfg)`
- Source: `include/process/process_agentic_rag.h`:198
- Brief: n/a
- Parameters:
  - `cfg` (const ProcessAgenticConfig &): n/a

#### `~ProcessAgenticRag()=default`
- Source: `include/process/process_agentic_rag.h`:149
- Brief: n/a
- Parameters: none

### themis::process::ProcessAttachment

#### `ProcessAttachment fromDocument(const nlohmann::json &doc)`
- Source: `include/process/process_linker.h`:95
- Brief: From Document.
- Parameters:
  - `doc` (const nlohmann::json &): Input parameter.
- Return: Return value.
- Details: doc Input parameter. Return value. Calls: value(), processLinkTypeFromString(), json::object(), contains(), is_null().

#### `nlohmann::json toDocument() const`
- Source: `include/process/process_linker.h`:94
- Brief: n/a
- Parameters: none

### themis::process::ProcessAuditLogger

#### `uint64_t AppendEntry(const std::string &model_id, const std::string &operation, const std::string &before_state, const std::string &after_state)`
- Source: `include/process/process_audit_logger.h`:83
- Brief: Append immutable entry to audit trail.
- Parameters:
  - `model_id` (const std::string &): Identifier of the model.
  - `operation` (const std::string &): Input parameter.
  - `before_state` (const std::string &): Input parameter.
  - `after_state` (const std::string &): Input parameter.
- Return: Return value.
- Details: Append Entry. model_id Identifier of the model. operation Input parameter. before_state Input parameter. after_state Input parameter. Return value. Implements AppendEntry without additional internal calls.

#### `std::unique_ptr< ProcessAuditLogger > Create(const AuditLoggerConfig &config)`
- Source: `include/process/process_audit_logger.h`:66
- Brief: Factory method to create audit logger.
- Parameters:
  - `config` (const AuditLoggerConfig &): Input parameter.
- Return: Return value.
- Details: Create. config Input parameter. Return value. Implements Create without additional internal calls.

#### `uint64_t CreateSnapshot()`
- Source: `include/process/process_audit_logger.h`:118
- Brief: Create snapshot of current audit trail state.
- Parameters: none
- Return: Return value.
- Details: Create Snapshot. Return value. Implements CreateSnapshot without additional internal calls.

#### `std::string GetModelStateAt(const std::string &model_id, uint64_t timestamp_ms) const`
- Source: `include/process/process_audit_logger.h`:107
- Brief: Reconstruct model state at specific point in time.
- Parameters:
  - `model_id` (const std::string &): n/a
  - `timestamp_ms` (uint64_t): n/a

#### `AuditLoggerStats GetStats() const`
- Source: `include/process/process_audit_logger.h`:113
- Brief: Get audit logger statistics.
- Parameters: none

#### `ProcessAuditLogger(std::unique_ptr< ProcessAuditLoggerImpl > impl)`
- Source: `include/process/process_audit_logger.h`:72
- Brief: Constructor.
- Parameters:
  - `impl` (std::unique_ptr< ProcessAuditLoggerImpl >): n/a

#### `std::vector< AuditTrailEntry > QueryByModelId(const std::string &model_id) const`
- Source: `include/process/process_audit_logger.h`:96
- Brief: Query audit trail for entries of a specific model.
- Parameters:
  - `model_id` (const std::string &): n/a

#### `std::vector< AuditTrailEntry > QueryByTimeRange(uint64_t start_ms, uint64_t end_ms) const`
- Source: `include/process/process_audit_logger.h`:101
- Brief: Query audit trail for entries in a time range.
- Parameters:
  - `start_ms` (uint64_t): n/a
  - `end_ms` (uint64_t): n/a

#### `bool VerifyIntegrity() const`
- Source: `include/process/process_audit_logger.h`:91
- Brief: Verify audit trail integrity via CRC32 chain.
- Parameters: none

#### `~ProcessAuditLogger()`
- Source: `include/process/process_audit_logger.h`:78
- Brief: Destructor.
- Parameters: none

### themis::process::ProcessAuditLoggerImpl

#### `uint64_t AppendEntry(const std::string &model_id, const std::string &operation, const std::string &before_state, const std::string &after_state)`
- Source: `src/process/process_audit_logger.cpp`:159
- Brief: ======================================================================== PUBLIC API - AUDIT TRAIL OPERATIONS ========================================================================
- Parameters:
  - `model_id` (const std::string &): Identifier of the model.
  - `operation` (const std::string &): Input parameter.
  - `before_state` (const std::string &): Input parameter.
  - `after_state` (const std::string &): Input parameter.
- Return: Return value.
- Throws:
  - std::runtime_error: if an error occurs.
- Details: Append Entry. model_id Identifier of the model. operation Input parameter. before_state Input parameter. after_state Input parameter. Return value. model_id Identifier of the model. operation Input parameter. before_state Input parameter. after_state Input parameter. Return value. std::runtime_error if an error occurs. Calls: lock(), DeltaPatchCodec::Encode(), std::to_string(), ComputeCrc32(), std::chrono::system_clock::now(), time_since_epoch(), count(), PersistEntry().

#### `uint32_t ComputeCrc32(const std::string &data)`
- Source: `src/process/process_audit_logger.cpp`:217
- Brief: Compute Crc32.
- Parameters:
  - `data` (const std::string &): Input parameter.
- Return: Return value.
- Details: data Input parameter. Return value. data Input parameter. Return value. Implements ComputeCrc32 without additional internal calls.

#### `uint64_t CreateSnapshot()`
- Source: `src/process/process_audit_logger.cpp`:205
- Brief: Create Snapshot.
- Parameters: none
- Return: Return value.
- Details: Return value. Return value. Calls: lock(), snapshot_lock(), empty(), back(), push_back(), utils::Logger::Info().

#### `std::string GetModelStateAt(const std::string &model_id, uint64_t timestamp_ms) const`
- Source: `src/process/process_audit_logger.cpp`:192
- Brief: Get Model State At.
- Parameters:
  - `model_id` (const std::string &): Identifier of the model.
  - `timestamp_ms` (uint64_t): Input parameter.
- Return: Return value.
- Details: model_id Identifier of the model. timestamp_ms Input parameter. Return value.

#### `AuditLoggerStats GetStats() const`
- Source: `src/process/process_audit_logger.cpp`:199
- Brief: Get Stats.
- Parameters: none
- Return: Return value.
- Details: Return value.

#### `bool LoadFromStorage()`
- Source: `src/process/process_audit_logger.cpp`:230
- Brief: Load From Storage.
- Parameters: none
- Return: True when the operation succeeds.
- Details: True when the operation succeeds. True when the operation succeeds. Implements LoadFromStorage without additional internal calls.

#### `bool PersistEntry(const AuditEntry &entry)`
- Source: `src/process/process_audit_logger.cpp`:224
- Brief: Persist Entry.
- Parameters:
  - `entry` (const AuditEntry &): Input parameter.
- Return: True when the operation succeeds.
- Details: entry Input parameter. True when the operation succeeds. entry Input parameter. True when the operation succeeds. Implements PersistEntry without additional internal calls.

#### `ProcessAuditLoggerImpl(const AuditLoggerConfig &config)`
- Source: `src/process/process_audit_logger.cpp`:137
- Brief: Process Audit Logger Impl.
- Parameters:
  - `config` (const AuditLoggerConfig &): Input parameter.
- Return: Return value.
- Details: config Input parameter. Return value.

#### `std::vector< AuditTrailEntry > QueryByModelId(const std::string &model_id) const`
- Source: `src/process/process_audit_logger.cpp`:175
- Brief: Query By Model Id.
- Parameters:
  - `model_id` (const std::string &): Identifier of the model.
- Return: Return value.
- Details: model_id Identifier of the model. Return value.

#### `std::vector< AuditTrailEntry > QueryByTimeRange(uint64_t start_ms, uint64_t end_ms) const`
- Source: `src/process/process_audit_logger.cpp`:183
- Brief: Query By Time Range.
- Parameters:
  - `start_ms` (uint64_t): Input parameter.
  - `end_ms` (uint64_t): Input parameter.
- Return: Return value.
- Details: start_ms Input parameter. end_ms Input parameter. Return value.

#### `bool VerifyIntegrity() const`
- Source: `src/process/process_audit_logger.cpp`:168
- Brief: Verify Integrity.
- Parameters: none
- Return: True when the operation succeeds.
- Details: True when the operation succeeds.

#### `~ProcessAuditLoggerImpl()=default`
- Source: `src/process/process_audit_logger.cpp`:148
- Brief: n/a
- Parameters: none

### themis::process::ProcessCommunityDetector

#### `ProcessCommunityDetector(RocksDBWrapper &db)`
- Source: `include/process/process_community_detector.h`:94
- Brief: Construct a ProcessCommunityDetector with a RocksDB backend.
- Parameters:
  - `db` (RocksDBWrapper &): Reference to a RocksDBWrapper instance that will be used for storing and retrieving detected community data. The ProcessCommunityDetector does not own this reference; the caller is responsible for keeping the RocksDBWrapper alive for the entire lifetime of this detector instance.
- Details: db Reference to a RocksDBWrapper instance that will be used for storing and retrieving detected community data. The ProcessCommunityDetector does not own this reference; the caller is responsible for keeping the RocksDBWrapper alive for the entire lifetime of this detector instance. Thread-safe: Multiple ProcessCommunityDetector instances can be created with the same RocksDB backend, and their operations will properly synchronize via RocksDB's internal locking mechanisms. The constructor does not perform any I/O; community detection is triggered lazily when detect() is called. detect() to run community detection on a process model generateReport() to create human-readable community descriptions

#### `std::vector< ProcessCommunity > detect(std::string_view model_id, float resolution=1.0f) const`
- Source: `include/process/process_community_detector.h`:108
- Brief: Run greedy modularity-based community detection on a process model.
- Parameters:
  - `model_id` (std::string_view): Process model identifier.
  - `resolution` (float): Resolution parameter (default 1.0). Values > 1.0 favour smaller communities; < 1.0 favour larger ones.
- Return: Detected communities sorted by size descending.
- Details: Loads the ProcessModelRecord from RocksDB (key proc:model: <model_id>), extracts its normalized["nodes"] and normalized["edges"], builds an adjacency list, and runs the Louvain-style algorithm. model_id Process model identifier. resolution Resolution parameter (default 1.0). Values > 1.0 favour smaller communities; < 1.0 favour larger ones. Detected communities sorted by size descending.

#### `std::string generateReport(const ProcessCommunity &community, std::string_view model_id, std::string_view llm_endpoint, std::string_view language="de") const`
- Source: `include/process/process_community_detector.h`:132
- Brief: Generate a text report for a community.
- Parameters:
  - `community` (const ProcessCommunity &): The community to describe.
  - `model_id` (std::string_view): Model the community belongs to.
  - `llm_endpoint` (std::string_view): Ignored in the stub – reserved for LLM URL.
  - `language` (std::string_view): Output language ("de" or "en").
- Return: Text report built from node names/descriptions.
- Details: //STUB/SIMULATIONNOTE: //Purpose:GeneratescommunityreportwithoutarealLLMcall //Activation:Always(LLMendpointintegrationnotyetimplemented) //ProductionDelta:Realimplementationcallsllm_endpointwithcommunity //nodedescriptions //RemovalPlan:ReplacewithHTTPLLMcallwhenLLMintegrationiswired(Q42026) community The community to describe. model_id Model the community belongs to. llm_endpoint Ignored in the stub – reserved for LLM URL. language Output language ("de" or "en"). Text report built from node names/descriptions.

#### `std::vector< ProcessCommunity > loadCommunities(std::string_view model_id) const`
- Source: `include/process/process_community_detector.h`:159
- Brief: Load all communities for a model from RocksDB.
- Parameters:
  - `model_id` (std::string_view): n/a
- Return: All communities for model_id; empty vector if none found.
- Details: All communities for model_id; empty vector if none found.

#### `bool persistCommunities(std::string_view model_id, const std::vector< ProcessCommunity > &communities)`
- Source: `include/process/process_community_detector.h`:149
- Brief: Persist a list of communities to RocksDB.
- Parameters:
  - `model_id` (std::string_view): Identifier of the model.
  - `communities` (const std::vector< ProcessCommunity > &): Input parameter.
- Return: true if all communities were stored successfully.
- Details: Persist Communities. Each community is stored under: proc:community:<model_id>:<community_id> true if all communities were stored successfully. model_id Identifier of the model. communities Input parameter. True when the operation succeeds.

### themis::process::ProcessConflictResolver

#### `std::unique_ptr< ProcessConflictResolver > Create(const ConflictResolverConfig &config)`
- Source: `include/process/process_conflict_resolver.h`:71
- Brief: Factory method to create conflict resolver.
- Parameters:
  - `config` (const ConflictResolverConfig &): n/a

#### `std::vector< ConflictInfo > DetectConflictsBatch(const std::map< std::string, std::vector< std::string > > &versions)`
- Source: `include/process/process_conflict_resolver.h`:104
- Brief: Detect conflicts in a batch of model versions.
- Parameters:
  - `versions` (const std::map< std::string, std::vector< std::string > > &): n/a

#### `ConflictResolverStats GetStats() const`
- Source: `include/process/process_conflict_resolver.h`:110
- Brief: Get conflict resolver statistics.
- Parameters: none

#### `ProcessConflictResolver(std::unique_ptr< ProcessConflictResolverImpl > impl)`
- Source: `include/process/process_conflict_resolver.h`:77
- Brief: Constructor.
- Parameters:
  - `impl` (std::unique_ptr< ProcessConflictResolverImpl >): n/a

#### `void RegisterResolver(std::shared_ptr< ProcessConflictResolverCallback > resolver)`
- Source: `include/process/process_conflict_resolver.h`:99
- Brief: Register application-provided resolver callback.
- Parameters:
  - `resolver` (std::shared_ptr< ProcessConflictResolverCallback >): Input parameter.
- Details: Register Resolver. resolver Input parameter. Implements RegisterResolver without additional internal calls.

#### `std::string ResolveConflict(const std::string &model_id, const std::string &v1_id, uint64_t v1_timestamp, const std::string &v1_sender, const std::string &v2_id, uint64_t v2_timestamp, const std::string &v2_sender)`
- Source: `include/process/process_conflict_resolver.h`:88
- Brief: Detect and resolve conflict between two versions.
- Parameters:
  - `model_id` (const std::string &): Identifier of the model.
  - `v1_id` (const std::string &): Identifier of the v1.
  - `v1_timestamp` (uint64_t): Input parameter.
  - `v1_sender` (const std::string &): Input parameter.
  - `v2_id` (const std::string &): Identifier of the v2.
  - `v2_timestamp` (uint64_t): Input parameter.
  - `v2_sender` (const std::string &): Input parameter.
- Return: Return value.
- Details: Resolve Conflict. model_id Identifier of the model. v1_id Identifier of the v1. v1_timestamp Input parameter. v1_sender Input parameter. v2_id Identifier of the v2. v2_timestamp Input parameter. v2_sender Input parameter. Return value. Implements ResolveConflict without additional internal calls.

#### `~ProcessConflictResolver()`
- Source: `include/process/process_conflict_resolver.h`:83
- Brief: Destructor.
- Parameters: none

### themis::process::ProcessConflictResolverCallback

#### `std::string GetName() const`
- Source: `include/process/process_conflict_resolution_callback.h`:164
- Brief: Optional: Get resolver name for logging/diagnostics.
- Parameters: none
- Return: Resolver name (e.g., "MyApplicationResolver")
- Details: Resolver name (e.g., "MyApplicationResolver")

#### `std::string Resolve(const ConflictMetadata &metadata)=0`
- Source: `include/process/process_conflict_resolution_callback.h`:158
- Brief: Resolve conflict between two model versions.
- Parameters:
  - `metadata` (const ConflictMetadata &): Conflict metadata (versions, timestamps, senders, etc.)
- Return: ID of winning version (must be either metadata.v1_id or metadata.v2_id)
- Throws:
  - std::exception: If resolution fails; caught by consensus layer (LWW fallback)
- Details: metadata Conflict metadata (versions, timestamps, senders, etc.) ID of winning version (must be either metadata.v1_id or metadata.v2_id) std::exception If resolution fails; caught by consensus layer (LWW fallback) Must complete within 5 seconds; timeout triggers LWW fallback Result must be deterministic (same metadata → same result) Returned ID must equal either metadata.v1_id or metadata.v2_id

#### `~ProcessConflictResolverCallback()=default`
- Source: `include/process/process_conflict_resolution_callback.h`:166
- Brief: n/a
- Parameters: none

### themis::process::ProcessConflictResolverImpl

#### `bool AreInConflict(const VersionInfo &v1, const VersionInfo &v2)`
- Source: `src/process/process_conflict_resolver.cpp`:246
- Brief: Are In Conflict.
- Parameters:
  - `v1` (const VersionInfo &): Input parameter.
  - `v2` (const VersionInfo &): Input parameter.
- Return: True when the operation succeeds.
- Details: v1 Input parameter. v2 Input parameter. True when the operation succeeds. Implements AreInConflict without additional internal calls.

#### `std::unique_ptr< IConflictResolutionStrategy > CreateStrategy(const std::string &strategy_name, std::shared_ptr< ProcessConflictResolverCallback > callback)`
- Source: `src/process/process_conflict_resolver.cpp`:225
- Brief: Create Strategy.
- Parameters:
  - `strategy_name` (const std::string &): Name of the strategy.
  - `callback` (std::shared_ptr< ProcessConflictResolverCallback >): Input parameter.
- Return: Return value.
- Details: strategy_name Name of the strategy. callback Input parameter. Return value. Calls: std::move().

#### `std::vector< ConflictInfo > DetectConflictsBatch(const std::map< std::string, std::vector< std::string > > &versions)`
- Source: `src/process/process_conflict_resolver.cpp`:204
- Brief: n/a
- Parameters:
  - `versions` (const std::map< std::string, std::vector< std::string > > &): n/a

#### `ConflictResolverStats GetStats() const`
- Source: `src/process/process_conflict_resolver.cpp`:211
- Brief: Get Stats.
- Parameters: none
- Return: Return value.
- Details: Return value.

#### `ProcessConflictResolverImpl(const ConflictResolverConfig &config)`
- Source: `src/process/process_conflict_resolver.cpp`:169
- Brief: Process Conflict Resolver Impl.
- Parameters:
  - `config` (const ConflictResolverConfig &): Input parameter.
- Return: Return value.
- Details: config Input parameter. Return value.

#### `void RegisterResolver(std::shared_ptr< ProcessConflictResolverCallback > resolver)`
- Source: `src/process/process_conflict_resolver.cpp`:202
- Brief: Register Resolver.
- Parameters:
  - `resolver` (std::shared_ptr< ProcessConflictResolverCallback >): Input parameter.
- Details: resolver Input parameter. Calls: lock(), CreateStrategy(), utils::Logger::Info().

#### `std::string ResolveConflict(const std::string &model_id, const std::string &v1_id, uint64_t v1_timestamp, const std::string &v1_sender, const std::string &v2_id, uint64_t v2_timestamp, const std::string &v2_sender)`
- Source: `src/process/process_conflict_resolver.cpp`:196
- Brief: Resolve Conflict.
- Parameters:
  - `model_id` (const std::string &): Identifier of the model.
  - `v1_id` (const std::string &): Identifier of the v1.
  - `v1_timestamp` (uint64_t): Input parameter.
  - `v1_sender` (const std::string &): Input parameter.
  - `v2_id` (const std::string &): Identifier of the v2.
  - `v2_timestamp` (uint64_t): Input parameter.
  - `v2_sender` (const std::string &): Input parameter.
- Return: Return value.
- Details: model_id Identifier of the model. v1_id Identifier of the v1. v1_timestamp Input parameter. v1_sender Input parameter. v2_id Identifier of the v2. v2_timestamp Input parameter. v2_sender Input parameter. Return value. model_id Identifier of the model. v1_id Identifier of the v1. v1_timestamp Input parameter. v1_sender Input parameter. v2_id Identifier of the v2. v2_timestamp Input parameter. v2_sender Input parameter. Return value. Calls: std::chrono::high_resolution_clock::now(), std::chrono::milliseconds(), lock(), count(), utils::Logger::Info(), c_str().

#### `~ProcessConflictResolverImpl()=default`
- Source: `src/process/process_conflict_resolver.cpp`:179
- Brief: n/a
- Parameters: none

### themis::process::ProcessDiagnostics

#### `ProcessDiagnostics()=delete`
- Source: `include/process/process_diagnostics.h`:292
- Brief: n/a
- Parameters: none

#### `DiagnosticRecord createConcurrencyIncident(ProcError error, std::string_view input_id, std::string_view message)`
- Source: `include/process/process_diagnostics.h`:246
- Brief: Create a concurrency incident diagnostic.
- Parameters:
  - `error` (ProcError): Input parameter.
  - `input_id` (std::string_view): Identifier of the input.
  - `message` (std::string_view): Input parameter.
- Return: DiagnosticRecord with CONCURRENCY_INCIDENT classification.
- Details: Create Concurrency Incident. error ProcError code (typically kInvalidTransition). input_id Input identifier (model ID, instance ID). message Actionable message describing the concurrent update conflict. DiagnosticRecord with CONCURRENCY_INCIDENT classification. error Input parameter. input_id Identifier of the input. message Input parameter. Return value. Calls: DiagnosticRecord().

#### `DiagnosticRecord createCycleIncident(ProcError error, std::string_view input_id, std::string_view message)`
- Source: `include/process/process_diagnostics.h`:259
- Brief: Create a cyclic dependency incident diagnostic.
- Parameters:
  - `error` (ProcError): Input parameter.
  - `input_id` (std::string_view): Identifier of the input.
  - `message` (std::string_view): Input parameter.
- Return: DiagnosticRecord with CYCLE_INCIDENT classification.
- Details: Create Cycle Incident. error ProcError code (typically kInvalidTransition). input_id Input identifier (link ID, path). message Actionable message describing the cycle. DiagnosticRecord with CYCLE_INCIDENT classification. error Input parameter. input_id Identifier of the input. message Input parameter. Return value. Calls: DiagnosticRecord().

#### `DiagnosticRecord createImportIncident(ProcError error, std::string_view input_id, std::string_view message)`
- Source: `include/process/process_diagnostics.h`:177
- Brief: Create an import incident diagnostic.
- Parameters:
  - `error` (ProcError): Input parameter.
  - `input_id` (std::string_view): Identifier of the input.
  - `message` (std::string_view): Input parameter.
- Return: DiagnosticRecord with IMPORT_INCIDENT classification.
- Details: Create Import Incident. error ProcError code (typically kDeserialiserFailed). input_id Input identifier (filename, model ID). message Actionable message for the operator. DiagnosticRecord with IMPORT_INCIDENT classification. error Input parameter. input_id Identifier of the input. message Input parameter. Return value. Calls: DiagnosticRecord().

#### `DiagnosticRecord createLinkingIncident(ProcError error, std::string_view input_id, std::string_view message)`
- Source: `include/process/process_diagnostics.h`:216
- Brief: Create a linking incident diagnostic.
- Parameters:
  - `error` (ProcError): Input parameter.
  - `input_id` (std::string_view): Identifier of the input.
  - `message` (std::string_view): Input parameter.
- Return: DiagnosticRecord with LINKING_INCIDENT classification.
- Details: Create Linking Incident. error ProcError code (typically kInvalidTransition). input_id Input identifier (link ID, instance ID). message Actionable message for the operator. DiagnosticRecord with LINKING_INCIDENT classification. error Input parameter. input_id Identifier of the input. message Input parameter. Return value. Calls: DiagnosticRecord().

#### `DiagnosticRecord createMalformedInputIncident(ProcError error, std::string_view input_id, std::string_view message)`
- Source: `include/process/process_diagnostics.h`:272
- Brief: Create a malformed input incident diagnostic.
- Parameters:
  - `error` (ProcError): Input parameter.
  - `input_id` (std::string_view): Identifier of the input.
  - `message` (std::string_view): Input parameter.
- Return: DiagnosticRecord with MALFORMED_INPUT_INCIDENT classification.
- Details: Create Malformed Input Incident. error ProcError code (typically kDeserialiserFailed). input_id Input identifier (filename, model ID). message Actionable message describing the malformation. DiagnosticRecord with MALFORMED_INPUT_INCIDENT classification. error Input parameter. input_id Identifier of the input. message Input parameter. Return value. Calls: DiagnosticRecord().

#### `DiagnosticRecord createMissingTargetIncident(ProcError error, std::string_view input_id, std::string_view message)`
- Source: `include/process/process_diagnostics.h`:285
- Brief: Create a missing target incident diagnostic.
- Parameters:
  - `error` (ProcError): Input parameter.
  - `input_id` (std::string_view): Identifier of the input.
  - `message` (std::string_view): Input parameter.
- Return: DiagnosticRecord with MISSING_TARGET_INCIDENT classification.
- Details: Create Missing Target Incident. error ProcError code (typically kInvalidTransition). input_id Input identifier (link ID, reference). message Actionable message describing the missing target. DiagnosticRecord with MISSING_TARGET_INCIDENT classification. error Input parameter. input_id Identifier of the input. message Input parameter. Return value. Calls: DiagnosticRecord().

#### `DiagnosticRecord createResourceIncident(ProcError error, std::string_view input_id, std::string_view message)`
- Source: `include/process/process_diagnostics.h`:233
- Brief: Create a resource limit incident diagnostic.
- Parameters:
  - `error` (ProcError): Input parameter.
  - `input_id` (std::string_view): Identifier of the input.
  - `message` (std::string_view): Input parameter.
- Return: DiagnosticRecord with RESOURCE_INCIDENT classification.
- Details: Create Resource Incident. error ProcError code (typically kExecutionTimeout). input_id Input identifier. message Actionable message describing which limit was exceeded. DiagnosticRecord with RESOURCE_INCIDENT classification. Example message: "Max nesting depth (100) exceeded in sub-process definitions. " "Reduce nesting or split model into separate definitions." error Input parameter. input_id Identifier of the input. message Input parameter. Return value. Calls: DiagnosticRecord().

#### `DiagnosticRecord createRetrievalIncident(ProcError error, std::string_view input_id, std::string_view message)`
- Source: `include/process/process_diagnostics.h`:203
- Brief: Create a retrieval incident diagnostic.
- Parameters:
  - `error` (ProcError): Input parameter.
  - `input_id` (std::string_view): Identifier of the input.
  - `message` (std::string_view): Input parameter.
- Return: DiagnosticRecord with RETRIEVAL_INCIDENT classification.
- Details: Create Retrieval Incident. error ProcError code. input_id Input identifier (instance ID, model ID). message Actionable message for the operator. DiagnosticRecord with RETRIEVAL_INCIDENT classification. error Input parameter. input_id Identifier of the input. message Input parameter. Return value. Calls: DiagnosticRecord().

#### `DiagnosticRecord createValidationIncident(ProcError error, std::string_view input_id, std::string_view message)`
- Source: `include/process/process_diagnostics.h`:190
- Brief: Create a validation incident diagnostic.
- Parameters:
  - `error` (ProcError): Input parameter.
  - `input_id` (std::string_view): Identifier of the input.
  - `message` (std::string_view): Input parameter.
- Return: DiagnosticRecord with VALIDATION_INCIDENT classification.
- Details: Create Validation Incident. error ProcError code (typically kSerialiserFailed or kInvalidTransition). input_id Input identifier. message Actionable message for the operator. DiagnosticRecord with VALIDATION_INCIDENT classification. error Input parameter. input_id Identifier of the input. message Input parameter. Return value. Calls: DiagnosticRecord().

### themis::process::ProcessGraphRag

#### `ProcessGraphRag(RocksDBWrapper &db, ProcessGraphManager &engine, ProcessModelManager &models, ProcessLinker &linker)`
- Source: `include/process/process_graph_rag.h`:151
- Brief: n/a
- Parameters:
  - `db` (RocksDBWrapper &): n/a
  - `engine` (ProcessGraphManager &): n/a
  - `models` (ProcessModelManager &): n/a
  - `linker` (ProcessLinker &): n/a

#### `std::vector< NodeDwellStats > analyzeBottlenecks(std::string_view model_id, int top_n=5) const`
- Source: `include/process/process_graph_rag.h`:404
- Brief: n/a
- Parameters:
  - `model_id` (std::string_view): n/a
  - `top_n` (int): n/a
- Details: Return the top-top_n bottleneck nodes for model_id, sorted descending by avg_dwell_ms. Returns empty vector if no data is available.

#### `std::string assemblePrompt_(const ProcessRagContext &ctx, const ProcessRagConfig &config) const`
- Source: `include/process/process_graph_rag.h`:440
- Brief: Assemble the final LLM prompt string from context and config.
- Parameters:
  - `ctx` (const ProcessRagContext &): n/a
  - `config` (const ProcessRagConfig &): n/a

#### `std::string buildAdminProcessingPrompt(const ProcessRagContext &ctx) const`
- Source: `include/process/process_graph_rag.h`:278
- Brief: Build a German (or English) system prompt for an LLM agent managing a Verwaltungsvorgang.
- Parameters:
  - `ctx` (const ProcessRagContext &): n/a
- Details: The prompt includes: process state, active tasks, attached documents, missing documents, compliance tags, SLA status, subgraph description, and similar historical cases.

#### `ProcessKnowledgeGraph buildInstanceKnowledgeGraph(std::string_view instance_id, const ProcessRagConfig &config={}) const`
- Source: `include/process/process_graph_rag.h`:221
- Brief: Build a KnowledgeGraph from a live process instance.
- Parameters:
  - `instance_id` (std::string_view): n/a
  - `config` (const ProcessRagConfig &): n/a
- Details: Includes instance state nodes (active tasks, variables) and attachment nodes so the LLM can traverse relationships.

#### `ProcessKnowledgeGraph buildKnowledgeGraph(std::string_view model_id) const`
- Source: `include/process/process_graph_rag.h`:211
- Brief: Convert a stored process model definition into a KnowledgeGraph-compatible node/edge set.
- Parameters:
  - `model_id` (std::string_view): n/a
- Details: Process nodes become KGNode objects (type CONCEPT). Sequence flows become KGEdge objects (relation CAUSES or RELATED_TO).

#### `std::string buildQueryPrompt(const ProcessRagContext &ctx) const`
- Source: `include/process/process_graph_rag.h`:285
- Brief: Build a query-specific user prompt (e.g. for "Was fehlt noch?").
- Parameters:
  - `ctx` (const ProcessRagContext &): n/a

#### `ComplianceCheckResult checkCompliance(std::string_view instance_id) const`
- Source: `include/process/process_graph_rag.h`:323
- Brief: n/a
- Parameters:
  - `instance_id` (std::string_view): n/a

#### `std::vector< std::pair< std::string, float > > computePpr(const nlohmann::json &normalized_graph, const std::vector< std::string > &seed_node_ids, const PprConfig &config={}) const`
- Source: `include/process/process_graph_rag.h`:262
- Brief: Compute Personalized PageRank (PPR) scores for all nodes in a process model graph, seeded from seed_node_ids.
- Parameters:
  - `normalized_graph` (const nlohmann::json &): Process model normalized graph JSON ({"nodes", "edges"}).
  - `seed_node_ids` (const std::vector< std::string > &): Starting nodes (personalisation vector is uniform over these nodes).
  - `config` (const PprConfig &): PPR tuning parameters.
- Return: Top-k (node_id, ppr_score) pairs sorted descending by score.
- Details: PPR handles multi-hop queries better than BFS by propagating relevance across the entire graph with exponential distance decay. Uses power iteration: r=α*A^T*r+(1-α)*personalization Terminates when \|\|r_new − r_old\|\|₁ < PprConfig::convergence_epsilon or after PprConfig::max_iterations steps. normalized_graph Process model normalized graph JSON ({"nodes", "edges"}). seed_node_ids Starting nodes (personalisation vector is uniform over these nodes). config PPR tuning parameters. Top-k (node_id, ppr_score) pairs sorted descending by score.

#### `void deregisterSlaRule(std::string_view instance_id, themisdb::analytics::CEPEngine &cep)`
- Source: `include/process/process_graph_rag.h`:376
- Brief: Deregister Sla Rule.
- Parameters:
  - `instance_id` (std::string_view): Identifier of the instance.
  - `cep` (themisdb::analytics::CEPEngine &): Input/output parameter.
- Details: Deregister the SLA CEP rules for instance_id. Safe to call if no rule was registered. instance_id Identifier of the instance. cep Input/output parameter.

#### `nlohmann::json extractSubgraph(std::string_view model_id, const std::vector< std::string > &seed_node_ids, int max_depth=2) const`
- Source: `include/process/process_graph_rag.h`:235
- Brief: Extract the subgraph around seed_node_ids using BFS up to max_depth hops.
- Parameters:
  - `model_id` (std::string_view): n/a
  - `seed_node_ids` (const std::vector< std::string > &): n/a
  - `max_depth` (int): n/a
- Return: JSON with keys "nodes" (array of node objects) and "edges" (array of edge objects).
- Details: JSON with keys "nodes" (array of node objects) and "edges" (array of edge objects).

#### `std::vector< SimilarCase > findSimilarCases(std::string_view instance_id, int k=5, float min_similarity=0.6f) const`
- Source: `include/process/process_graph_rag.h`:342
- Brief: n/a
- Parameters:
  - `instance_id` (std::string_view): n/a
  - `k` (int): n/a
  - `min_similarity` (float): n/a

#### `void fireSlaAlert_(const std::string &instance_id, const std::string &process_name, int64_t sla_ms, int64_t elapsed_ms, const std::string &status)`
- Source: `include/process/process_graph_rag.h`:426
- Brief: Fire Sla Alert.
- Parameters:
  - `instance_id` (const std::string &): Identifier of the instance.
  - `process_name` (const std::string &): Name of the process.
  - `sla_ms` (int64_t): Input parameter.
  - `elapsed_ms` (int64_t): Input parameter.
  - `status` (const std::string &): Input parameter.
- Details: Fire an SLA alert to the registered callback (if any) for instance_id. Exceptions from the callback are caught and logged. instance_id Identifier of the instance. process_name Name of the process. sla_ms Input parameter. elapsed_ms Input parameter. status Input parameter.

#### `void recordNodeCompletion(std::string_view model_id, std::string_view node_id, std::string_view node_name, int64_t dwell_ms)`
- Source: `include/process/process_graph_rag.h`:396
- Brief: ───────────────────────────────────────────────────────────────────────────── Cross-Case Bottleneck Analytics (Q4 2026) ─────────────────────────────────────────────────────────────────────────────
- Parameters:
  - `model_id` (std::string_view): Identifier of the model.
  - `node_id` (std::string_view): Identifier of the node.
  - `node_name` (std::string_view): Name of the node.
  - `dwell_ms` (int64_t): Input parameter.
- Details: Record the completion of a node to update the cross-case aggregate. Call this after each task/activity completes in an instance. model_id Process model identifier. node_id Node identifier within the model. node_name Human-readable node name (for display). dwell_ms Time spent at this node in milliseconds. model_id Identifier of the model. node_id Identifier of the node. node_name Name of the node. dwell_ms Input parameter.

#### `void registerSlaRule(std::string_view instance_id, int64_t sla_ms, std::string_view process_name, themisdb::analytics::CEPEngine &cep, SlaAlertCallback on_alert=nullptr)`
- Source: `include/process/process_graph_rag.h`:368
- Brief: Register Sla Rule.
- Parameters:
  - `instance_id` (std::string_view): Identifier of the instance.
  - `sla_ms` (int64_t): Input parameter.
  - `process_name` (std::string_view): Name of the process.
  - `cep` (themisdb::analytics::CEPEngine &): Input/output parameter.
  - `on_alert` (SlaAlertCallback): Input parameter.
- Details: Register an SLA CEP rule for instance_id. instance_id Active process instance. sla_ms SLA deadline in milliseconds from process start. process_name Human-readable name for alert messages. cep CEP engine to register the rule with. on_alert Optional callback invoked when alert fires (may be null). instance_id Identifier of the instance. sla_ms Input parameter. process_name Name of the process. cep Input/output parameter. on_alert Input parameter.

#### `ProcessRagContext retrieve(std::string_view instance_id, std::string_view query, const ProcessRagConfig &config={}) const`
- Source: `include/process/process_graph_rag.h`:172
- Brief: Build the full retrieval context for a process instance and a free-text query.
- Parameters:
  - `instance_id` (std::string_view): n/a
  - `query` (std::string_view): n/a
  - `config` (const ProcessRagConfig &): n/a
- Details: Orchestration sequence: Load instance state and active tokens. Extract subgraph around active nodes. Collect attachments. Check for missing required documents. Find similar historical cases. Assemble the LLM prompt.

#### `ProcessRagContext retrieveForNode(std::string_view instance_id, std::string_view node_id, std::string_view query, const ProcessRagConfig &config={}) const`
- Source: `include/process/process_graph_rag.h`:185
- Brief: Build retrieval context focused on a specific node within an instance.
- Parameters:
  - `instance_id` (std::string_view): n/a
  - `node_id` (std::string_view): n/a
  - `query` (std::string_view): n/a
  - `config` (const ProcessRagConfig &): n/a
- Details: Used when the LLM needs to answer questions about a particular task step (e.g., "Which documents are required at Schritt 3?").

#### `float scoreNodeRelevance_(const nlohmann::json &node_doc, std::string_view query, const std::vector< std::string > &active_nodes) const`
- Source: `include/process/process_graph_rag.h`:433
- Brief: Score how relevant node_doc is to query given active_nodes.
- Parameters:
  - `node_doc` (const nlohmann::json &): n/a
  - `query` (std::string_view): n/a
  - `active_nodes` (const std::vector< std::string > &): n/a

#### `nlohmann::json summarizeVerwaltungsvorgang(std::string_view instance_id) const`
- Source: `include/process/process_graph_rag.h`:310
- Brief: Summarise a Verwaltungsvorgang for an officer or a case-management UI.
- Parameters:
  - `instance_id` (std::string_view): n/a
- Return: JSON: { "instance_id":"…", "process_name":"…", "state":"RUNNING", "current_tasks":["…"], "progress_pct":42.5, "missing_documents":["Bauzeichnung"], "compliance_status":"ok", "sla_status":"on_time", "attachments_count":3, "variables":{…} }
- Details: JSON: { "instance_id":"…", "process_name":"…", "state":"RUNNING", "current_tasks":["…"], "progress_pct":42.5, "missing_documents":["Bauzeichnung"], "compliance_status":"ok", "sla_status":"on_time", "attachments_count":3, "variables":{…} }

### themis::process::ProcessLightRetriever

#### `ProcessLightRetriever(RocksDBWrapper &db, ProcessGraphRag &graph_rag, ProcessCommunityDetector &community_detector)`
- Source: `include/process/process_light_retriever.h`:138
- Brief: n/a
- Parameters:
  - `db` (RocksDBWrapper &): RocksDB instance used to resolve instance→model_id.
  - `graph_rag` (ProcessGraphRag &): Existing GraphRAG engine (LOCAL retrieval).
  - `community_detector` (ProcessCommunityDetector &): Community detector (GLOBAL retrieval).
- Details: db RocksDB instance used to resolve instance→model_id. graph_rag Existing GraphRAG engine (LOCAL retrieval). community_detector Community detector (GLOBAL retrieval).

#### `RetrievalMode classifyQuery(std::string_view query) const`
- Source: `include/process/process_light_retriever.h`:182
- Brief: Classify a query as LOCAL or GLOBAL using keyword heuristics.
- Parameters:
  - `query` (std::string_view): n/a
- Details: Comparison is case-insensitive. Returns RetrievalMode::GLOBAL when any global keyword is found; RetrievalMode::LOCAL otherwise.

#### `LightRetrievalResult createDegradedResult(std::string_view reason) const`
- Source: `include/process/process_light_retriever.h`:211
- Brief: Gracefully degrade retrieval result when resources are exhausted.
- Parameters:
  - `reason` (std::string_view): Description of why degradation occurred.
- Return: LightRetrievalResult marked as degraded with partial context.
- Details: reason Description of why degradation occurred. LightRetrievalResult marked as degraded with partial context.

#### `const ResourceLimits & getResourceLimits() const`
- Source: `include/process/process_light_retriever.h`:154
- Brief: Get the current resource limits.
- Parameters: none
- Return: The active ResourceLimits.
- Details: The active ResourceLimits.

#### `bool isWithinDepthBudget(size_t current_depth) const`
- Source: `include/process/process_light_retriever.h`:204
- Brief: Check if traversal depth is within limits.
- Parameters:
  - `current_depth` (size_t): The current traversal depth.
- Return: true if within limits; false if exceeded.
- Details: current_depth The current traversal depth. true if within limits; false if exceeded.

#### `bool isWithinSizeBudget(size_t current_size_bytes) const`
- Source: `include/process/process_light_retriever.h`:197
- Brief: Check if accumulated context size is within limits.
- Parameters:
  - `current_size_bytes` (size_t): Current accumulated size.
- Return: true if within limits; false if exceeded.
- Details: current_size_bytes Current accumulated size. true if within limits; false if exceeded.

#### `bool isWithinTimeoutBudget(int64_t start_time_ms) const`
- Source: `include/process/process_light_retriever.h`:190
- Brief: Check if current time is within the configured timeout window.
- Parameters:
  - `start_time_ms` (int64_t): The time retrieval started.
- Return: true if within timeout; false if exceeded.
- Details: start_time_ms The time retrieval started. true if within timeout; false if exceeded.

#### `LightRetrievalResult retrieve(std::string_view query, std::string_view instance_id, RetrievalMode mode=RetrievalMode::AUTO, const ProcessRagConfig &config={}) const`
- Source: `include/process/process_light_retriever.h`:168
- Brief: Retrieve LLM context for query using the selected mode.
- Parameters:
  - `query` (std::string_view): Free-text query string.
  - `instance_id` (std::string_view): Process instance ID (used for LOCAL retrieval and model_id resolution).
  - `mode` (RetrievalMode): Retrieval mode (default: AUTO).
  - `config` (const ProcessRagConfig &): GraphRAG config forwarded to LOCAL retrieval.
- Return: LightRetrievalResult with assembled llm_context.
- Details: query Free-text query string. instance_id Process instance ID (used for LOCAL retrieval and model_id resolution). mode Retrieval mode (default: AUTO). config GraphRAG config forwarded to LOCAL retrieval. LightRetrievalResult with assembled llm_context.

#### `void setResourceLimits(const ResourceLimits &limits)`
- Source: `include/process/process_light_retriever.h`:148
- Brief: Set custom resource limits for stress scenario handling.
- Parameters:
  - `limits` (const ResourceLimits &): Input parameter.
- Details: ───────────────────────────────────────────────────────────────────────────── Phase 2: Stress Scenario Hardening Implementation ───────────────────────────────────────────────────────────────────────────── limits The resource limits to enforce. limits Input parameter. Implements setResourceLimits without additional internal calls.

### themis::process::ProcessLightRetrieverConcurrencyContract

#### `std::string_view describe()`
- Source: `include/process/process_concurrency_contract.h`:243
- Brief: n/a
- Parameters: none

### themis::process::ProcessLink

#### `ProcessLink fromDocument(const nlohmann::json &doc)`
- Source: `include/process/process_linker.h`:117
- Brief: From Document.
- Parameters:
  - `doc` (const nlohmann::json &): Input parameter.
- Return: Return value.
- Details: doc Input parameter. Return value. Calls: value(), processLinkTypeFromString(), json::object().

#### `nlohmann::json toDocument() const`
- Source: `include/process/process_linker.h`:116
- Brief: n/a
- Parameters: none

### themis::process::ProcessLinker

#### `ProcessLinker(RocksDBWrapper &db)`
- Source: `include/process/process_linker.h`:180
- Brief: Construct a ProcessLinker with a RocksDB backend.
- Parameters:
  - `db` (RocksDBWrapper &): Reference to a RocksDBWrapper instance that will be used for all link storage and retrieval operations. The ProcessLinker does not own this reference; the caller is responsible for keeping the RocksDBWrapper alive for the entire lifetime of this linker instance.
- Details: db Reference to a RocksDBWrapper instance that will be used for all link storage and retrieval operations. The ProcessLinker does not own this reference; the caller is responsible for keeping the RocksDBWrapper alive for the entire lifetime of this linker instance. Thread-safe: Multiple ProcessLinker instances can be created with the same RocksDB backend. Link operations are protected with per-link fine-grained locking to maximize concurrency while ensuring consistency. The constructor does not perform any I/O; initialization is lazy. attachObject() for how to create and manage links detectStaleLinkAtReadTime() for stale link detection

#### `std::pair< bool, std::string > attachObject(std::string_view instance_id, std::string_view object_id, std::string_view object_collection, ProcessLinkType link_type, std::optional< std::string_view > node_id=std::nullopt, nlohmann::json metadata={}, std::string_view attached_by="")`
- Source: `include/process/process_linker.h`:198
- Brief: Attach any object (document, metadata, case file) to a process instance.
- Parameters:
  - `instance_id` (std::string_view): Target process instance ID.
  - `object_id` (std::string_view): ID of the object to attach.
  - `object_collection` (std::string_view): Collection the object lives in ("documents", …).
  - `link_type` (ProcessLinkType): Semantic relationship type.
  - `node_id` (std::optional< std::string_view >): Optional: which process node this attachment belongs to.
  - `metadata` (nlohmann::json): Optional: additional link properties.
  - `attached_by` (std::string_view): Actor performing the attachment.
- Return: {true, attachment_id} on success; {false, error_message} on failure.
- Details: instance_id Target process instance ID. object_id ID of the object to attach. object_collection Collection the object lives in ("documents", …). link_type Semantic relationship type. node_id Optional: which process node this attachment belongs to. metadata Optional: additional link properties. attached_by Actor performing the attachment. {true, attachment_id} on success; {false, error_message} on failure.

#### `std::pair< int32_t, std::string > cleanupOrphanedLinks(const std::vector< std::string > &link_ids)`
- Source: `include/process/process_linker.h`:382
- Brief: Manually remove stale links (orphaned references).
- Parameters:
  - `link_ids` (const std::vector< std::string > &): List of link IDs to remove.
- Return: Pair of (count_removed, error_message). error_message is empty on success.
- Details: Removes links that no longer have valid targets. This is a manual operation requiring operator intervention to prevent accidental data loss. link_ids List of link IDs to remove. Pair of (count_removed, error_message). error_message is empty on success.

#### `bool detachObject(std::string_view attachment_id)`
- Source: `include/process/process_linker.h`:212
- Brief: Detach an object from a process instance by attachment ID.
- Parameters:
  - `attachment_id` (std::string_view): Identifier of the attachment.
- Return: true if the attachment existed and was removed.
- Details: Detach Object. true if the attachment existed and was removed. attachment_id Identifier of the attachment. True when the operation succeeds. Calls: sid(), size(), substr(), get(), SPDLOG_WARN(), json::parse(), value(), empty().

#### `bool detectLinkingConflict_(std::string_view key, std::optional< uint64_t > expected_version=std::nullopt) const`
- Source: `include/process/process_linker.h`:485
- Brief: Check if a link operation conflicts with concurrent modifications.
- Parameters:
  - `key` (std::string_view): The key being accessed.
  - `expected_version` (std::optional< uint64_t >): Expected version if known.
- Return: true if conflict detected.
- Details: key The key being accessed. expected_version Expected version if known. true if conflict detected.

#### `DiagnosticRecord detectStaleLinkAtReadTime(std::string_view link_id) const`
- Source: `include/process/process_linker.h`:359
- Brief: Detect if a link has a missing target (stale link detection).
- Parameters:
  - `link_id` (std::string_view): ID of the link to check.
- Return: DiagnosticRecord with MISSING_TARGET_INCIDENT if target is missing, or with incident_type LINKING_INCIDENT (not missing) on success.
- Details: Performs read-time validation to check if the target of a link still exists. This is a lazy detection mechanism that does not require automatic cleanup. link_id ID of the link to check. DiagnosticRecord with MISSING_TARGET_INCIDENT if target is missing, or with incident_type LINKING_INCIDENT (not missing) on success.

#### `std::vector< std::string > findInstancesWithObject(std::string_view object_id, std::string_view object_collection) const`
- Source: `include/process/process_linker.h`:239
- Brief: Find all instance IDs that have a specific object attached.
- Parameters:
  - `object_id` (std::string_view): n/a
  - `object_collection` (std::string_view): n/a
- Details: Performs a full scan over the attachment prefix; use with care on large datasets.

#### `std::vector< std::string > findStaleLinkReferences() const`
- Source: `include/process/process_linker.h`:371
- Brief: Find all links that reference a deleted or missing target.
- Parameters: none
- Return: Vector of link IDs that have stale targets.
- Details: Scans all links to identify stale references. This is an expensive operation and should be run offline or as part of maintenance procedures. Vector of link IDs that have stale targets.

#### `std::vector< ProcessAttachment > getAttachments(std::string_view instance_id, std::optional< ProcessLinkType > filter_type=std::nullopt) const`
- Source: `include/process/process_linker.h`:220
- Brief: Get all attachments for a process instance.
- Parameters:
  - `instance_id` (std::string_view): The process instance.
  - `filter_type` (std::optional< ProcessLinkType >): Optional: only return attachments of this link type.
- Details: instance_id The process instance. filter_type Optional: only return attachments of this link type.

#### `std::vector< ProcessLink > getLinks(std::string_view process_id, std::optional< ProcessLinkType > filter_type=std::nullopt) const`
- Source: `include/process/process_linker.h`:264
- Brief: Get all outgoing links from a process instance (or model).
- Parameters:
  - `process_id` (std::string_view): Source process instance/model ID.
  - `filter_type` (std::optional< ProcessLinkType >): Optional: only return links of this type.
- Details: process_id Source process instance/model ID. filter_type Optional: only return links of this type.

#### `std::vector< std::string > getMissingDocuments(std::string_view instance_id, std::string_view node_id, std::string_view model_id) const`
- Source: `include/process/process_linker.h`:309
- Brief: Determine which required documents are missing for an instance at a given node.
- Parameters:
  - `instance_id` (std::string_view): n/a
  - `node_id` (std::string_view): n/a
  - `model_id` (std::string_view): n/a
- Return: List of doc_type strings for missing mandatory documents.
- Details: Cross-references the required-document registry (model-level) against the actual attachments (instance-level, filtered by node_id). List of doc_type strings for missing mandatory documents.

#### `std::vector< ProcessAttachment > getNodeAttachments(std::string_view instance_id, std::string_view node_id) const`
- Source: `include/process/process_linker.h`:228
- Brief: Get all attachments for a specific process node within an instance.
- Parameters:
  - `instance_id` (std::string_view): n/a
  - `node_id` (std::string_view): n/a

#### `std::vector< nlohmann::json > getRequiredDocuments(std::string_view model_id, std::string_view node_id) const`
- Source: `include/process/process_linker.h`:295
- Brief: Get all required document descriptors for a node in a model.
- Parameters:
  - `model_id` (std::string_view): n/a
  - `node_id` (std::string_view): n/a
- Details: Each JSON object contains at minimum: doc_type, mandatory, and optionally schema.

#### `bool hasCyclePath_(std::string_view source, std::string_view target, std::set< std::string > &visited, int32_t depth, int32_t max_depth) const`
- Source: `include/process/process_linker.h`:428
- Brief: DFS-based cycle detection: check if there is a path from target back to source in the link graph.
- Parameters:
  - `source` (std::string_view): The source entity in the proposed link.
  - `target` (std::string_view): The target entity in the proposed link.
  - `visited` (std::set< std::string > &): Set of entities already visited in this traversal.
  - `depth` (int32_t): Current traversal depth.
  - `max_depth` (int32_t): Maximum depth before giving up (safety limit).
- Return: true if a path exists from target → source.
- Details: source The source entity in the proposed link. target The target entity in the proposed link. visited Set of entities already visited in this traversal. depth Current traversal depth. max_depth Maximum depth before giving up (safety limit). true if a path exists from target → source.

#### `bool isLinkTargetValid(std::string_view target_id) const`
- Source: `include/process/process_linker.h`:345
- Brief: Validate that a link target actually exists.
- Parameters:
  - `target_id` (std::string_view): The target entity to validate.
- Return: true if the target exists or is a valid system ID, false otherwise.
- Details: Checks if the target entity is stored in the database or is a recognized system identifier. This prevents dangling references. target_id The target entity to validate. true if the target exists or is a valid system ID, false otherwise.

#### `std::pair< bool, std::string > linkProcesses(std::string_view source_id, std::string_view target_id, ProcessLinkType link_type, nlohmann::json properties={})`
- Source: `include/process/process_linker.h`:251
- Brief: Create a typed link between two process instances (or models).
- Parameters:
  - `source_id` (std::string_view): n/a
  - `target_id` (std::string_view): n/a
  - `link_type` (ProcessLinkType): n/a
  - `properties` (nlohmann::json): n/a
- Return: {true, link_id} on success; {false, error_message} on failure.
- Details: {true, link_id} on success; {false, error_message} on failure.

#### `std::string makeAttachKey_(std::string_view instance_id, std::string_view object_id) const`
- Source: `include/process/process_linker.h`:464
- Brief: n/a
- Parameters:
  - `instance_id` (std::string_view): n/a
  - `object_id` (std::string_view): n/a

#### `std::string makeLinkKey_(std::string_view source_id, std::string_view target_id, ProcessLinkType link_type) const`
- Source: `include/process/process_linker.h`:471
- Brief: n/a
- Parameters:
  - `source_id` (std::string_view): n/a
  - `target_id` (std::string_view): n/a
  - `link_type` (ProcessLinkType): n/a

#### `std::string makeObjIdxKey_(std::string_view object_id, std::string_view collection, std::string_view instance_id) const`
- Source: `include/process/process_linker.h`:468
- Brief: n/a
- Parameters:
  - `object_id` (std::string_view): n/a
  - `collection` (std::string_view): n/a
  - `instance_id` (std::string_view): n/a
- Details: Reverse-lookup key for findInstancesWithObject(): proc:obj_idx:<object_id>:<collection>:<instance_id>

#### `std::string makeReqDocKey_(std::string_view model_id, std::string_view node_id, std::string_view doc_type) const`
- Source: `include/process/process_linker.h`:474
- Brief: n/a
- Parameters:
  - `model_id` (std::string_view): n/a
  - `node_id` (std::string_view): n/a
  - `doc_type` (std::string_view): n/a

#### `bool registerRequiredDocument(std::string_view model_id, std::string_view node_id, std::string_view doc_type, bool mandatory, nlohmann::json schema={})`
- Source: `include/process/process_linker.h`:281
- Brief: Register a required document type for a process node in a model.
- Parameters:
  - `model_id` (std::string_view): Identifier of the model.
  - `node_id` (std::string_view): Identifier of the node.
  - `doc_type` (std::string_view): Input parameter.
  - `mandatory` (bool): Input parameter.
  - `schema` (nlohmann::json): Input parameter.
- Return: true on success.
- Details: Register Required Document. model_id Process model definition ID. node_id Node within the model that requires the document. doc_type Human-readable document type, e.g. "Bauzeichnung". mandatory Whether the document is strictly required. schema Optional JSON Schema for document validation. true on success. model_id Identifier of the model. node_id Identifier of the node. doc_type Input parameter. mandatory Input parameter. schema Input parameter. True when the operation succeeds.

#### `void rollbackLinkOperation_(uint64_t operation_id)`
- Source: `include/process/process_linker.h`:491
- Brief: Rollback a link operation that encountered a conflict.
- Parameters:
  - `operation_id` (uint64_t): Identifier of the operation.
- Details: Rollback Link Operation. operation_id ID of the operation to roll back. operation_id Identifier of the operation. Calls: lock(), find(), end(), SPDLOG_WARN(), rbegin(), rend(), put(), del().

#### `std::pair< int32_t, int32_t > verifyLinkIntegrity() const`
- Source: `include/process/process_linker.h`:393
- Brief: Verify the integrity of all links in the database.
- Parameters: none
- Return: Pair of (total_links_checked, count_with_issues).
- Details: Checks that all links reference valid targets and reports any consistency issues. Pair of (total_links_checked, count_with_issues).

#### `bool wouldCreateCycle(std::string_view source_id, std::string_view target_id, int32_t max_depth=0) const`
- Source: `include/process/process_linker.h`:330
- Brief: Detect if creating a link would introduce a cycle.
- Parameters:
  - `source_id` (std::string_view): The process entity creating the link (source).
  - `target_id` (std::string_view): The process entity being linked to (target).
  - `max_depth` (int32_t): Maximum depth to search (prevents infinite traversal). If 0, uses kMaxRetrievalDepth.
- Return: true if a cycle would be created, false if link is safe.
- Details: Uses depth-first search to check if there is already a path from target_id back to source_id. If so, adding the link would create a cycle. source_id The process entity creating the link (source). target_id The process entity being linked to (target). max_depth Maximum depth to search (prevents infinite traversal). If 0, uses kMaxRetrievalDepth. true if a cycle would be created, false if link is safe.

### themis::process::ProcessLinker::LinkOperationGuard

#### `LinkOperationGuard(ProcessLinker &linker, std::string_view operation_name)`
- Source: `include/process/process_linker.h`:442
- Brief: n/a
- Parameters:
  - `linker` (ProcessLinker &): n/a
  - `operation_name` (std::string_view): n/a

#### `LinkOperationGuard(const LinkOperationGuard &)=delete`
- Source: `include/process/process_linker.h`:453
- Brief: n/a
- Parameters:
  - `<unnamed>` (const LinkOperationGuard &): n/a

#### `uint64_t getOperationId() const`
- Source: `include/process/process_linker.h`:450
- Brief: n/a
- Parameters: none

#### `void markFailed()`
- Source: `include/process/process_linker.h`:449
- Brief: n/a
- Parameters: none

#### `LinkOperationGuard & operator=(const LinkOperationGuard &)=delete`
- Source: `include/process/process_linker.h`:454
- Brief: n/a
- Parameters:
  - `<unnamed>` (const LinkOperationGuard &): n/a

#### `void recordModification(std::string_view key)`
- Source: `include/process/process_linker.h`:448
- Brief: Record Modification.
- Parameters:
  - `key` (std::string_view): Input parameter.
- Details: key Input parameter. Calls: std::chrono::system_clock::now(), time_since_epoch(), count(), get(), std::string(), std::move(), push_back(), lock().

#### `~LinkOperationGuard()`
- Source: `include/process/process_linker.h`:446
- Brief: n/a
- Parameters: none

### themis::process::ProcessLinkerConcurrencyContract

#### `std::string_view describe()`
- Source: `include/process/process_concurrency_contract.h`:218
- Brief: n/a
- Parameters: none

### themis::process::ProcessModelGenerator

#### `ProcessModelGenerator()=default`
- Source: `include/process/process_model_generator.h`:109
- Brief: n/a
- Parameters: none

#### `std::string buildFixPrompt_(const nlohmann::json &current_json, const std::vector< std::string > &errors, std::string_view language)`
- Source: `include/process/process_model_generator.h`:208
- Brief: Build a fix/correction prompt given the current model JSON and errors.
- Parameters:
  - `current_json` (const nlohmann::json &): n/a
  - `errors` (const std::vector< std::string > &): n/a
  - `language` (std::string_view): n/a

#### `std::string buildGenerationPrompt_(std::string_view description, const Config &cfg) const`
- Source: `include/process/process_model_generator.h`:202
- Brief: Build the initial generation prompt.
- Parameters:
  - `description` (std::string_view): n/a
  - `cfg` (const Config &): n/a

#### `nlohmann::json extractJson_(const std::string &llm_text)`
- Source: `include/process/process_model_generator.h`:215
- Brief: Parse the LLM text response, extracting the JSON block.
- Parameters:
  - `llm_text` (const std::string &): n/a

#### `ProcessModelRecord fromLlmJson(const nlohmann::json &llm_json, ProcessDomain domain=ProcessDomain::BUSINESS)`
- Source: `include/process/process_model_generator.h`:193
- Brief: Convert an LLM JSON response to a ProcessModelRecord.
- Parameters:
  - `llm_json` (const nlohmann::json &): Parsed LLM JSON response.
  - `domain` (ProcessDomain): Default domain if not present in JSON.
- Return: Populated ProcessModelRecord.
- Details: Expected LLM JSON schema: { "id":"proc_id", "name":"Prozessname", "domain":"ADMINISTRATION", "activities":[{"id":"a1","name":"Schritt1","type":"userTask","sla_hours":24}], "gateways":[{"id":"g1","name":"Entscheidung","type":"exclusiveGateway"}], "events":[{"id":"s1","type":"startEvent"},{"id":"e1","type":"endEvent"}], "edges":[{"from":"s1","to":"a1","type":"sequenceFlow"}] } llm_json Parsed LLM JSON response. domain Default domain if not present in JSON. Populated ProcessModelRecord.

#### `std::pair< bool, ProcessModelRecord > generateFromDescription(std::string_view description, const Config &cfg={}) const`
- Source: `include/process/process_model_generator.h`:131
- Brief: Generate a ProcessModelRecord from a free-text description.
- Parameters:
  - `description` (std::string_view): Natural language process description (DE or EN).
  - `cfg` (const Config &): Generation configuration.
- Return: {true, record} on success; {false, {}} on failure.
- Details: Calls the LLM backend up to Config::max_retries times, validating the result after each attempt and feeding errors back to the LLM. description Natural language process description (DE or EN). cfg Generation configuration. {true, record} on success; {false, {}} on failure.

#### `std::pair< bool, ProcessModelRecord > refine(const ProcessModelRecord &existing, std::string_view feedback, const Config &cfg={}) const`
- Source: `include/process/process_model_generator.h`:149
- Brief: Refine an existing ProcessModelRecord based on textual feedback (e.g. from a user review).
- Parameters:
  - `existing` (const ProcessModelRecord &): Current model to refine.
  - `feedback` (std::string_view): Natural language correction instructions.
  - `cfg` (const Config &): Generation configuration.
- Return: {true, refined_record} on success; {false, existing} on failure (original model unchanged).
- Details: Sends the current model definition together with the feedback to the LLM and returns the updated model after validation. existing Current model to refine. feedback Natural language correction instructions. cfg Generation configuration. {true, refined_record} on success; {false, existing} on failure (original model unchanged).

#### `void setLlmBackend(LlmBackend backend)`
- Source: `include/process/process_model_generator.h`:117
- Brief: Register the LLM backend callable.
- Parameters:
  - `backend` (LlmBackend): Input parameter.
- Details: Set Llm Backend. Must be called before generateFromDescription() or refine(). If not set, both methods return {false, {}}. backend Input parameter. Calls: std::move().

#### `ValidationResult validate(const nlohmann::json &normalized_graph)`
- Source: `include/process/process_model_generator.h`:169
- Brief: Validate BPMN semantic constraints on a normalised process graph.
- Parameters:
  - `normalized_graph` (const nlohmann::json &): JSON object with "nodes" and "edges".
- Return: ValidationResult with error list.
- Details: Rules checked: At least one startEvent node. At least one endEvent node. No isolated nodes (every node must have ≥1 edge). Every gateway node has ≥1 outgoing edge. normalized_graph JSON object with "nodes" and "edges". ValidationResult with error list.

### themis::process::ProcessModelManager

#### `ProcessModelManager(::themis::RocksDBWrapper &db)`
- Source: `include/process/process_model_manager.h`:195
- Brief: Construct a ProcessModelManager with a RocksDB backend.
- Parameters:
  - `db` (::themis::RocksDBWrapper &): Reference to a RocksDBWrapper instance that will be used for all model storage and retrieval operations. The ProcessModelManager does not own this reference; the caller is responsible for keeping the RocksDBWrapper alive for the entire lifetime of this manager instance.
- Details: db Reference to a RocksDBWrapper instance that will be used for all model storage and retrieval operations. The ProcessModelManager does not own this reference; the caller is responsible for keeping the RocksDBWrapper alive for the entire lifetime of this manager instance. Thread-safe: Multiple ProcessModelManager instances can be created with the same RocksDB backend, and their operations will properly synchronize via RocksDB's internal locking mechanisms. The constructor does not perform any I/O; initialization is lazy (performed when the first import/retrieval operation is called). ~ProcessModelManager() for cleanup semantics

#### `ProcessModelManager(const ProcessModelManager &)=delete`
- Source: `include/process/process_model_manager.h`:199
- Brief: n/a
- Parameters:
  - `<unnamed>` (const ProcessModelManager &): n/a

#### `nlohmann::json buildNormalizedGraph_(const std::vector< ProcessNodeInfo > &nodes, const std::vector< ProcessEdgeInfo > &edges, const ProcessModelRecord &meta)`
- Source: `include/process/process_model_manager.h`:514
- Brief: - buildNormalizedGraph_ -----------------------------------------------
- Parameters:
  - `nodes` (const std::vector< ProcessNodeInfo > &): Input parameter.
  - `edges` (const std::vector< ProcessEdgeInfo > &): Input parameter.
  - `meta` (const ProcessModelRecord &): Input parameter.
- Return: Return value.
- Details: nodes Input parameter. edges Input parameter. meta Input parameter. Return value.

#### `TransactionContext createTransaction_(std::string_view model_id)`
- Source: `include/process/process_model_manager.h`:540
- Brief: Create a new transaction context for a multi-step operation.
- Parameters:
  - `model_id` (std::string_view): Identifier of the model.
- Return: TransactionContext with unique ID and timestamp.
- Details: Create Transaction. model_id The model being modified. TransactionContext with unique ID and timestamp. model_id Identifier of the model. Return value.

#### `ProcessModelResult deployToEngine(std::string_view model_id, ProcessGraphManager &engine) const`
- Source: `include/process/process_model_manager.h`:371
- Brief: Deploy a process model to the ProcessGraphManager for execution.
- Parameters:
  - `model_id` (std::string_view): The model to deploy.
  - `engine` (ProcessGraphManager &): Target execution engine.
- Details: Converts the stored model to ProcessNodeInfo / ProcessEdgeInfo objects and registers them with the given engine. model_id The model to deploy. engine Target execution engine.

#### `bool detectConflict_(std::string_view model_id, int expected_revision) const`
- Source: `include/process/process_model_manager.h`:527
- Brief: Detect if another operation modified the model during our transaction.
- Parameters:
  - `model_id` (std::string_view): The model being tracked.
  - `expected_revision` (int): The revision we expect.
- Return: true if conflict detected.
- Details: model_id The model being tracked. expected_revision The revision we expect. true if conflict detected.

#### `std::string exportBpmn(std::string_view model_id) const`
- Source: `include/process/process_model_manager.h`:342
- Brief: Export a stored model to BPMN 2.0 XML.
- Parameters:
  - `model_id` (std::string_view): n/a
- Return: BPMN XML string, or empty on failure.
- Details: BPMN XML string, or empty on failure.

#### `std::string exportEpk(std::string_view model_id) const`
- Source: `include/process/process_model_manager.h`:347
- Brief: Export a stored model to EPK text format.
- Parameters:
  - `model_id` (std::string_view): n/a

#### `std::vector< std::pair< ProcessModelRecord, float > > findSimilar(const std::vector< float > &query_embedding, size_t k=10, float min_similarity=0.7f) const`
- Source: `include/process/process_model_manager.h`:327
- Brief: Vector-similarity search: find models semantically similar to a natural-language query or another model's embedding.
- Parameters:
  - `query_embedding` (const std::vector< float > &): Embedding vector for the query.
  - `k` (size_t): Number of nearest neighbours to return.
  - `min_similarity` (float): Minimum cosine similarity threshold [0, 1].
- Details: query_embedding Embedding vector for the query. k Number of nearest neighbours to return. min_similarity Minimum cosine similarity threshold [0, 1].

#### `nlohmann::json generateLlmDescriptor(std::string_view model_id) const`
- Source: `include/process/process_model_manager.h`:356
- Brief: Generate an LLM-optimised JSON descriptor for a model.
- Parameters:
  - `model_id` (std::string_view): n/a
- Details: The descriptor is designed to be injected into an LLM system prompt or RAG context. It includes structured node/edge descriptions, compliance tags, SLA information, and a natural-language summary.

#### `nlohmann::json getConsistencyDiagnostics() const`
- Source: `include/process/process_model_manager.h`:414
- Brief: Get internal state consistency checks for debugging.
- Parameters: none
- Return: JSON diagnostic object
- Details: Returns diagnostic information about: Total models loaded Index coherency status (FTS/Vector) Orphaned or corrupted records Revision chain integrity JSON diagnostic object

#### `ProcessModelResult importArisXml(std::string_view aml_xml, const ProcessModelRecord &meta={})`
- Source: `include/process/process_model_manager.h`:259
- Brief: Import an EPK model from an ARIS Markup Language (AML) XML document.
- Parameters:
  - `aml_xml` (std::string_view): Input parameter.
  - `meta` (const ProcessModelRecord &): Input parameter.
- Return: ProcessModelResult with the assigned model_id on success.
- Details: Import Aris Xml. Parses the first EPK <Model> found in the AML file produced by ARIS Designer 9.x / 10.x. ARIS TypeNum values are mapped to the corresponding EPKNodeType values (see EpkArisXmlImporter for the full mapping table). aml_xml Full AML XML string. meta Optional metadata overrides (name, domain, owner, …). ProcessModelResult with the assigned model_id on success. aml_xml Input parameter. meta Input parameter. Return value.

#### `ProcessModelResult importBpmn(std::string_view bpmn_xml, const ProcessModelRecord &meta={})`
- Source: `include/process/process_model_manager.h`:213
- Brief: Import a BPMN 2.0 XML document and store it as a process model.
- Parameters:
  - `bpmn_xml` (std::string_view): Input parameter.
  - `meta` (const ProcessModelRecord &): Input parameter.
- Return: ProcessModelResult with the assigned model_id on success.
- Details: - Import -------------------------------------------------------------- bpmn_xml Full BPMN 2.0 XML string. meta Optional metadata overrides (name, domain, owner, …). ProcessModelResult with the assigned model_id on success. bpmn_xml Input parameter. meta Input parameter. Return value.

#### `ProcessModelResult importEpk(std::string_view epk_text, const ProcessModelRecord &meta={})`
- Source: `include/process/process_model_manager.h`:227
- Brief: Import an EPK (Ereignisgesteuerte Prozesskette) definition.
- Parameters:
  - `epk_text` (std::string_view): Input parameter.
  - `meta` (const ProcessModelRecord &): Input parameter.
- Return: Return value.
- Details: Import Epk. Accepts a lightweight text-based EPK description (one node per line, edges indicated by -> notation, or JSON format). epk_text EPK definition string. meta Metadata overrides. epk_text Input parameter. meta Input parameter. Return value.

#### `ProcessModelResult importVccVpb(std::string_view yaml_text, const ProcessModelRecord &meta={})`
- Source: `include/process/process_model_manager.h`:242
- Brief: Import a VCC-VPB YAML process definition.
- Parameters:
  - `yaml_text` (std::string_view): Input parameter.
  - `meta` (const ProcessModelRecord &): Input parameter.
- Return: Return value.
- Details: Import Vcc Vpb. VCC-VPB is the native format used by the Visual Change Control – Visual Process Builder tool. The YAML schema is documented in config/process_models/README.md. yaml_text Raw YAML content. meta Metadata overrides. yaml_text Input parameter. meta Input parameter. Return value.

#### `std::vector< ProcessModelRecord > list(std::optional< ProcessDomain > domain=std::nullopt, std::optional< ProcessModelState > state=std::nullopt, size_t limit=0) const`
- Source: `include/process/process_model_manager.h`:300
- Brief: List all process models, optionally filtered by domain and state.
- Parameters:
  - `domain` (std::optional< ProcessDomain >): If set, only models of this domain are returned.
  - `state` (std::optional< ProcessModelState >): If set, only models with this lifecycle state are returned.
  - `limit` (size_t): Maximum number of results (0 = unlimited).
- Details: domain If set, only models of this domain are returned. state If set, only models with this lifecycle state are returned. limit Maximum number of results (0 = unlimited).

#### `std::optional< ProcessModelRecord > load(std::string_view model_id) const`
- Source: `include/process/process_model_manager.h`:282
- Brief: Load a process model by ID.
- Parameters:
  - `model_id` (std::string_view): The unique model identifier.
- Return: The record, or std::nullopt when not found.
- Details: model_id The unique model identifier. The record, or std::nullopt when not found.

#### `std::string makeKey_(std::string_view model_id) const`
- Source: `include/process/process_model_manager.h`:510
- Brief: n/a
- Parameters:
  - `model_id` (std::string_view): n/a

#### `std::string makeVersionedKey_(std::string_view model_id, int revision) const`
- Source: `include/process/process_model_manager.h`:511
- Brief: n/a
- Parameters:
  - `model_id` (std::string_view): n/a
  - `revision` (int): n/a

#### `ProcessModelManager & operator=(const ProcessModelManager &)=delete`
- Source: `include/process/process_model_manager.h`:200
- Brief: n/a
- Parameters:
  - `<unnamed>` (const ProcessModelManager &): n/a

#### `ProcessModelResult remove(std::string_view model_id)`
- Source: `include/process/process_model_manager.h`:287
- Brief: Delete a process model (soft-delete: marks state as ARCHIVED).
- Parameters:
  - `model_id` (std::string_view): Identifier of the model.
- Return: Return value.
- Details: Remove. model_id Identifier of the model. Return value. Calls: load(), ProcessModelResult::failure(), std::string(), deindex(), removeByPk(), save().

#### `void rollbackTransaction_(const TransactionContext &txn)`
- Source: `include/process/process_model_manager.h`:533
- Brief: Rollback modifications to a model in case of conflict.
- Parameters:
  - `txn` (const TransactionContext &): Input parameter.
- Details: Rollback Transaction. txn Transaction context describing what was modified. txn Input parameter. Calls: lock(), makeKey_(), makeVersionedKey_(), get(), SPDLOG_WARN(), put(), SPDLOG_INFO(), del().

#### `ProcessModelResult save(const ProcessModelRecord &record)`
- Source: `include/process/process_model_manager.h`:274
- Brief: Save or update a ProcessModelRecord in the DB.
- Parameters:
  - `record` (const ProcessModelRecord &): Input parameter.
- Return: Return value.
- Details: - CRUD ---------------------------------------------------------------- If a record with the same id already exists, a new revision is written and the old one is kept under its versioned key (audit trail). record Input parameter. Return value. Calls: empty(), ProcessModelResult::failure(), validateModelConsistency(), SPDLOG_WARN(), load(), makeVersionedKey_(), toDocument(), dump().

#### `std::vector< ProcessModelRecord > search(std::string_view query, size_t limit=20) const`
- Source: `include/process/process_model_manager.h`:314
- Brief: Full-text search across model names and descriptions.
- Parameters:
  - `query` (std::string_view): Keyword or phrase.
  - `limit` (size_t): Maximum results.
- Details: Simple keyword matching — for semantic search use findSimilar(). query Keyword or phrase. limit Maximum results.

#### `void setEmbedder(std::function< std::vector< float >(std::string_view)> embedder)`
- Source: `include/process/process_model_manager.h`:430
- Brief: Wire a text-embedding function.
- Parameters:
  - `embedder` (std::function< std::vector< float >(std::string_view)>): Callable (std::string_view text) → std::vector<float>. Pass an empty function to disable.
- Details: When set, embedder is called automatically inside save() whenever the saved record has an empty embedding vector. The concatenation of name + " " + description + " " + long_description is used as input. embedder Callable (std::string_view text) → std::vector<float>. Pass an empty function to disable.

#### `void setInvertedIndex(std::shared_ptr< InvertedIndex > fts)`
- Source: `include/process/process_model_manager.h`:445
- Brief: Wire an InvertedIndex for BM25 full-text search.
- Parameters:
  - `fts` (std::shared_ptr< InvertedIndex >): Input parameter.
- Details: Set Inverted Index. When set, save() automatically indexes the model name and description fields, and remove() removes the posting entries. The search() method uses the BM25 index instead of the linear keyword scan. The index must be created for the logical table "process_definitions" and the column "text" before the first save() call. fts Shared pointer to an InvertedIndex instance (may be null to disable). fts Input parameter.

#### `void setVectorIndex(std::shared_ptr< VectorIndexManager > vi)`
- Source: `include/process/process_model_manager.h`:460
- Brief: Wire a VectorIndexManager for HNSW-based findSimilar().
- Parameters:
  - `vi` (std::shared_ptr< VectorIndexManager >): Input parameter.
- Details: Set Vector Index. When set, findSimilar() delegates to the HNSW index for O(log n) approximate nearest-neighbour search instead of a linear cosine scan. save() upserts the model embedding; remove() deletes it from the index. The index must be initialised for the object name "process_models" with the correct embedding dimension before the first save() call. vi Shared pointer to an initialised VectorIndexManager (may be null to disable). vi Input parameter.

#### `ProcessModelResult undeployFromEngine(std::string_view model_id, ProcessGraphManager &engine) const`
- Source: `include/process/process_model_manager.h`:379
- Brief: Undeploy (unregister) a model from the execution engine.
- Parameters:
  - `model_id` (std::string_view): n/a
  - `engine` (ProcessGraphManager &): n/a

#### `ProcessModelResult validateModelConsistency(const ProcessModelRecord &record) const`
- Source: `include/process/process_model_manager.h`:401
- Brief: Validate process model consistency and integrity.
- Parameters:
  - `record` (const ProcessModelRecord &): The record to validate
- Return: ProcessModelResult with detailed validation errors on failure
- Details: Checks: Required fields are non-empty (id, name, version) All referenced nodes in edges exist No dangling references Best-effort bounded cycle diagnostics (warnings only, non-fatal) Resource limits respected (max nodes/edges/depth) record The record to validate ProcessModelResult with detailed validation errors on failure

#### `~ProcessModelManager()`
- Source: `include/process/process_model_manager.h`:196
- Brief: n/a
- Parameters: none

### themis::process::ProcessModelManager::TransactionGuard

#### `TransactionGuard(ProcessModelManager &mgr, const TransactionContext &ctx)`
- Source: `include/process/process_model_manager.h`:492
- Brief: n/a
- Parameters:
  - `mgr` (ProcessModelManager &): n/a
  - `ctx` (const TransactionContext &): n/a

#### `TransactionGuard(const TransactionGuard &)=delete`
- Source: `include/process/process_model_manager.h`:500
- Brief: n/a
- Parameters:
  - `<unnamed>` (const TransactionGuard &): n/a

#### `const TransactionContext & getContext() const`
- Source: `include/process/process_model_manager.h`:497
- Brief: n/a
- Parameters: none

#### `void markFailed()`
- Source: `include/process/process_model_manager.h`:496
- Brief: n/a
- Parameters: none

#### `TransactionGuard & operator=(const TransactionGuard &)=delete`
- Source: `include/process/process_model_manager.h`:501
- Brief: n/a
- Parameters:
  - `<unnamed>` (const TransactionGuard &): n/a

#### `~TransactionGuard()`
- Source: `include/process/process_model_manager.h`:494
- Brief: n/a
- Parameters: none

### themis::process::ProcessModelManagerConcurrencyContract

#### `std::string_view describe()`
- Source: `include/process/process_concurrency_contract.h`:191
- Brief: n/a
- Parameters: none

### themis::process::ProcessModelRecord

#### `ProcessModelRecord fromDocument(const nlohmann::json &doc)`
- Source: `include/process/process_model_manager.h`:123
- Brief: Deserialize a JSON document back into a ProcessModelRecord.
- Parameters:
  - `doc` (const nlohmann::json &): Input parameter.
- Return: Return value.
- Details: From Document. doc Input parameter. Return value. Calls: contains(), is_string(), getStr(), value(), notationFromString(), domainFromString(), stateFromString(), is_array().

#### `nlohmann::json toDocument() const`
- Source: `include/process/process_model_manager.h`:118
- Brief: Serialize the record to a BaseEntity-compatible JSON document.
- Parameters: none
- Details: Uses reserved ThemisDB field names so that the document is indexable by the process graph engine.

### themis::process::ProcessModelResult

#### `ProcessModelResult failure(std::string_view msg)`
- Source: `include/process/process_model_manager.h`:139
- Brief: Failure.
- Parameters:
  - `msg` (std::string_view): Input parameter.
- Return: Return value.
- Details: msg Input parameter. Return value. Calls: std::string().

#### `ProcessModelResult success(std::string_view id="")`
- Source: `include/process/process_model_manager.h`:138
- Brief: Success.
- Parameters:
  - `id` (std::string_view): Input parameter.
- Return: Return value.
- Details: id Input parameter. Return value. Calls: std::string().

### themis::process::ProcessStressScenarios

#### `const BulkLinkCreationStress & bulkLinkCreation()`
- Source: `include/process/process_stress_scenarios.h`:454
- Brief: n/a
- Parameters: none

#### `const CircularReferenceDetectionStress & circularReferenceDetection()`
- Source: `include/process/process_stress_scenarios.h`:453
- Brief: n/a
- Parameters: none

#### `const CommunityDetectionTimeoutStress & communityDetectionTimeout()`
- Source: `include/process/process_stress_scenarios.h`:460
- Brief: n/a
- Parameters: none

#### `const ConcurrentQueryChurnStress & concurrentQueryChurn()`
- Source: `include/process/process_stress_scenarios.h`:461
- Brief: n/a
- Parameters: none

#### `const DeepNestingStress & deepNesting()`
- Source: `include/process/process_stress_scenarios.h`:446
- Brief: n/a
- Parameters: none

#### `const EmptyGraphQueryStress & emptyGraphQuery()`
- Source: `include/process/process_stress_scenarios.h`:458
- Brief: n/a
- Parameters: none

#### `std::string_view getScenarioName(int32_t index)`
- Source: `include/process/process_stress_scenarios.h`:467
- Brief: Get scenario by index (0-based).
- Parameters:
  - `index` (int32_t): n/a

#### `const LargeContextSizeStress & largeContextSize()`
- Source: `include/process/process_stress_scenarios.h`:459
- Brief: n/a
- Parameters: none

#### `const LargeElementCountStress & largeElementCount()`
- Source: `include/process/process_stress_scenarios.h`:447
- Brief: n/a
- Parameters: none

#### `const LinkAttributeMutationStress & linkAttributeMutation()`
- Source: `include/process/process_stress_scenarios.h`:455
- Brief: n/a
- Parameters: none

#### `const MalformedXmlRecoveryStress & malformedXmlRecovery()`
- Source: `include/process/process_stress_scenarios.h`:448
- Brief: n/a
- Parameters: none

#### `const OrphanedLinkResolutionStress & orphanedLinkResolution()`
- Source: `include/process/process_stress_scenarios.h`:452
- Brief: n/a
- Parameters: none

#### `int32_t totalScenarioCount()`
- Source: `include/process/process_stress_scenarios.h`:464
- Brief: Count of all defined scenarios.
- Parameters: none

#### `const UnsupportedGatewayStress & unsupportedGateway()`
- Source: `include/process/process_stress_scenarios.h`:449
- Brief: n/a
- Parameters: none

### themis::process::ProcessTelemetryIntegration

#### `std::unique_ptr< ProcessTelemetryIntegration > Create(const TelemetryConfig &config, const std::string &node_id)`
- Source: `include/process/process_telemetry_integration.h`:67
- Brief: Factory method to create telemetry integration.
- Parameters:
  - `config` (const TelemetryConfig &): n/a
  - `node_id` (const std::string &): n/a

#### `std::shared_ptr< DistributedSpan > CreateSpan(const std::string &operation_name)`
- Source: `include/process/process_telemetry_integration.h`:85
- Brief: Create a new distributed trace span.
- Parameters:
  - `operation_name` (const std::string &): n/a

#### `bool ExportSpans()`
- Source: `include/process/process_telemetry_integration.h`:117
- Brief: Export all recorded spans to OpenTelemetry collector.
- Parameters: none
- Return: True when the operation succeeds.
- Details: Export Spans. True when the operation succeeds. Implements ExportSpans without additional internal calls.

#### `TraceContext GetCurrentTraceContext() const`
- Source: `include/process/process_telemetry_integration.h`:95
- Brief: Get current trace context (for header propagation).
- Parameters: none

#### `TelemetryStats GetStats() const`
- Source: `include/process/process_telemetry_integration.h`:112
- Brief: Get telemetry statistics.
- Parameters: none

#### `ProcessTelemetryIntegration(std::unique_ptr< ProcessTelemetryIntegrationImpl > impl)`
- Source: `include/process/process_telemetry_integration.h`:74
- Brief: Constructor.
- Parameters:
  - `impl` (std::unique_ptr< ProcessTelemetryIntegrationImpl >): n/a

#### `void RecordSpan(const std::shared_ptr< DistributedSpan > &span)`
- Source: `include/process/process_telemetry_integration.h`:100
- Brief: Record span in history and export to OTel collector.
- Parameters:
  - `span` (const std::shared_ptr< DistributedSpan > &): Input parameter.
- Details: Record Span. span Input parameter. Implements RecordSpan without additional internal calls.

#### `void SetCurrentTraceContext(const TraceContext &context)`
- Source: `include/process/process_telemetry_integration.h`:90
- Brief: Set current trace context (for parent span linking).
- Parameters:
  - `context` (const TraceContext &): Input parameter.
- Details: Set Current Trace Context. context Input parameter. Implements SetCurrentTraceContext without additional internal calls.

#### `bool ValidateOverheadBudget(const std::string &operation_name, uint64_t operation_latency_ms, uint64_t tracing_overhead_ms)`
- Source: `include/process/process_telemetry_integration.h`:105
- Brief: Validate tracing overhead budget (< 5% of operation latency).
- Parameters:
  - `operation_name` (const std::string &): Name of the operation.
  - `operation_latency_ms` (uint64_t): Input parameter.
  - `tracing_overhead_ms` (uint64_t): Input parameter.
- Return: True when the operation succeeds.
- Details: Validate Overhead Budget. operation_name Name of the operation. operation_latency_ms Input parameter. tracing_overhead_ms Input parameter. True when the operation succeeds. Implements ValidateOverheadBudget without additional internal calls.

#### `~ProcessTelemetryIntegration()`
- Source: `include/process/process_telemetry_integration.h`:80
- Brief: Destructor.
- Parameters: none

### themis::process::ProcessTelemetryIntegrationImpl

#### `std::shared_ptr< DistributedSpan > CreateSpan(const std::string &operation_name)`
- Source: `src/process/process_telemetry_integration.cpp`:274
- Brief: Create Span.
- Parameters:
  - `operation_name` (const std::string &): Name of the operation.
- Return: Return value.
- Details: operation_name Name of the operation. Return value.

#### `bool ExportSpans()`
- Source: `src/process/process_telemetry_integration.cpp`:315
- Brief: Export Spans.
- Parameters: none
- Return: True when the operation succeeds.
- Details: True when the operation succeeds. True when the operation succeeds. Calls: lock(), empty(), push_back(), SerializeSpanOtlp(), SendSpansToCollector(), metrics_lock(), size(), clear().

#### `TraceContext GetCurrentTraceContext() const`
- Source: `src/process/process_telemetry_integration.cpp`:286
- Brief: Get Current Trace Context.
- Parameters: none
- Return: Return value.
- Details: Return value.

#### `TelemetryStats GetStats() const`
- Source: `src/process/process_telemetry_integration.cpp`:309
- Brief: Get Stats.
- Parameters: none
- Return: Return value.
- Details: Return value.

#### `ProcessTelemetryIntegrationImpl(const TelemetryConfig &config, const std::string &node_id)`
- Source: `src/process/process_telemetry_integration.cpp`:250
- Brief: n/a
- Parameters:
  - `config` (const TelemetryConfig &): n/a
  - `node_id` (const std::string &): n/a

#### `void RecordSpan(const std::shared_ptr< DistributedSpan > &span)`
- Source: `src/process/process_telemetry_integration.cpp`:292
- Brief: Record Span.
- Parameters:
  - `span` (const std::shared_ptr< DistributedSpan > &): Input parameter.
- Details: span Input parameter. span Input parameter. Calls: lock(), push_back(), size(), erase(), begin(), utils::Logger::Debug(), GetOperationName(), c_str().

#### `bool SendSpansToCollector(const std::vector< std::string > &span_jsons)`
- Source: `src/process/process_telemetry_integration.cpp`:335
- Brief: Send Spans To Collector.
- Parameters:
  - `span_jsons` (const std::vector< std::string > &): Input parameter.
- Return: True when the operation succeeds.
- Details: span_jsons Input parameter. True when the operation succeeds. span_jsons Input parameter. True when the operation succeeds. Calls: utils::Logger::Info(), size(), c_str().

#### `std::string SerializeSpanOtlp(const std::shared_ptr< DistributedSpan > &span)`
- Source: `src/process/process_telemetry_integration.cpp`:327
- Brief: Serialize Span Otlp.
- Parameters:
  - `span` (const std::shared_ptr< DistributedSpan > &): Input parameter.
- Return: Return value.
- Details: span Input parameter. Return value. span Input parameter. Return value. Calls: GetOperationName(), GetContext(), GetLatencyMs(), str().

#### `void SetCurrentTraceContext(const TraceContext &context)`
- Source: `src/process/process_telemetry_integration.cpp`:280
- Brief: Set Current Trace Context.
- Parameters:
  - `context` (const TraceContext &): Input parameter.
- Details: context Input parameter. context Input parameter. Calls: lock().

#### `bool ValidateOverheadBudget(const std::string &operation_name, uint64_t operation_latency_ms, uint64_t tracing_overhead_ms)`
- Source: `src/process/process_telemetry_integration.cpp`:301
- Brief: Validate Overhead Budget.
- Parameters:
  - `operation_name` (const std::string &): Name of the operation.
  - `operation_latency_ms` (uint64_t): Input parameter.
  - `tracing_overhead_ms` (uint64_t): Input parameter.
- Return: True when the operation succeeds.
- Details: operation_name Name of the operation. operation_latency_ms Input parameter. tracing_overhead_ms Input parameter. True when the operation succeeds. operation_name Name of the operation. operation_latency_ms Input parameter. tracing_overhead_ms Input parameter. True when the operation succeeds. Calls: lock(), utils::Logger::Warn(), c_str(), std::max().

#### `~ProcessTelemetryIntegrationImpl()=default`
- Source: `src/process/process_telemetry_integration.cpp`:263
- Brief: n/a
- Parameters: none

### themis::process::ReproducibilityGuidelines

#### `std::string_view avoid_subprocess_ordering_hint()`
- Source: `include/process/process_determinism_spec.h`:303
- Brief: Avoid dependency on subprocess execution order in test assertions.
- Parameters: none

#### `std::string_view capture_snapshots_at_known_points_hint()`
- Source: `include/process/process_determinism_spec.h`:315
- Brief: Capture snapshots at known points to ensure consistent retrieval.
- Parameters: none

#### `std::string_view use_fixed_uuids_hint()`
- Source: `include/process/process_determinism_spec.h`:297
- Brief: Use fixed UUIDs (RFC 4122 v5) for models to ensure consistent IDs across test runs.
- Parameters: none

#### `std::string_view use_version_clocks_hint()`
- Source: `include/process/process_determinism_spec.h`:309
- Brief: Use version numbers to ensure model consistency in multi-threaded tests.
- Parameters: none

### themis::process::RetrievalConsistencyDeterminismSpec

#### `std::string_view describe()`
- Source: `include/process/process_determinism_spec.h`:283
- Brief: n/a
- Parameters: none

### themis::process::SerializerInputValidator

#### `std::pair< int32_t, int32_t > countXmlTags(std::string_view xml)`
- Source: `include/process/serializer_hardening.h`:105
- Brief: Count opening and closing XML tags to detect truncation.
- Parameters:
  - `xml` (std::string_view): The XML input.
- Return: Tuple of (opening_count, closing_count).
- Details: xml The XML input. Tuple of (opening_count, closing_count).

#### `std::string extractXmlVersion(std::string_view xml)`
- Source: `include/process/serializer_hardening.h`:97
- Brief: Extract format version from a BPMN/CMMN/DMN document header.
- Parameters:
  - `xml` (std::string_view): Input parameter.
- Return: Version string (e.g., "2.0"), or empty if not found.
- Details: Extract Xml Version. xml The XML document. Version string (e.g., "2.0"), or empty if not found. xml Input parameter. Return value. Calls: find(), substr(), find_first_of(), std::string().

#### `bool isAsciiControlChar(unsigned char c)`
- Source: `include/process/serializer_hardening.h`:109
- Brief: Is Ascii Control Char.
- Parameters:
  - `c` (unsigned char): Input parameter.
- Return: True when the operation succeeds.
- Details: c Input parameter. True when the operation succeeds. Implements isAsciiControlChar without additional internal calls.

#### `bool isValidUtf8(std::string_view s)`
- Source: `include/process/serializer_hardening.h`:89
- Brief: Validate that a string is well-formed UTF-8.
- Parameters:
  - `s` (std::string_view): Input parameter.
- Return: true if the string is valid UTF-8, false otherwise.
- Details: Is Valid Utf8. s The string to check. true if the string is valid UTF-8, false otherwise. s Input parameter. True when the operation succeeds. Calls: data(), size(), isValidUtf8Sequence().

#### `bool isValidUtf8Sequence(const unsigned char *data, size_t remaining_bytes, size_t &sequence_length)`
- Source: `include/process/serializer_hardening.h`:110
- Brief: Is Valid Utf8 Sequence.
- Parameters:
  - `data` (const unsigned char *): Input parameter.
  - `remaining_bytes` (size_t): Input parameter.
  - `sequence_length` (size_t &): Input/output parameter.
- Return: True when the operation succeeds.
- Details: data Input parameter. remaining_bytes Input parameter. sequence_length Input/output parameter. True when the operation succeeds. Implements isValidUtf8Sequence without additional internal calls.

#### `bool isXmlTruncated(std::string_view xml)`
- Source: `include/process/serializer_hardening.h`:81
- Brief: Check for truncation indicators in XML-based formats.
- Parameters:
  - `xml` (std::string_view): Input parameter.
- Return: true if the XML appears to be truncated (missing closing tags), false if structure appears complete.
- Details: Is Xml Truncated. xml The XML input to check. true if the XML appears to be truncated (missing closing tags), false if structure appears complete. xml Input parameter. True when the operation succeeds. Calls: countXmlTags().

#### `SerializerValidationResult validateInput(std::string_view input, std::string_view format_name="Process Model")`
- Source: `include/process/serializer_hardening.h`:69
- Brief: Validate generic serializer input before parsing.
- Parameters:
  - `input` (std::string_view): Input parameter.
  - `format_name` (std::string_view): Name of the format.
- Return: Validation result with error details if validation fails.
- Details: Validate Input. input The input data to validate. format_name Human-readable format name (e.g., "BPMN 2.0", "EPK"). Validation result with error details if validation fails. input Input parameter. format_name Name of the format. Return value. Calls: empty(), SerializerValidationResult::failure(), isInputSizeValid(), size(), str(), isValidUtf8(), find(), isXmlTruncated().

### themis::process::SerializerValidationResult

#### `SerializerValidationResult failure(std::string_view msg, DiagnosticIncidentType incident=DiagnosticIncidentType::MALFORMED_INPUT_INCIDENT)`
- Source: `include/process/serializer_hardening.h`:38
- Brief: n/a
- Parameters:
  - `msg` (std::string_view): n/a
  - `incident` (DiagnosticIncidentType): n/a

#### `SerializerValidationResult success()`
- Source: `include/process/serializer_hardening.h`:32
- Brief: n/a
- Parameters: none

### themis::process::SpanAttributes

#### `void setAttribute(const std::string &key, const std::string &value)`
- Source: `include/process/federated_span_contract.h`:310
- Brief: Add a generic attribute.
- Parameters:
  - `key` (const std::string &): Attribute key
  - `value` (const std::string &): Attribute value
- Details: key Attribute key value Attribute value

### themis::process::SpanEvent

#### `SpanEvent now(const std::string &event_name)`
- Source: `include/process/federated_span_contract.h`:337
- Brief: Create event with current timestamp.
- Parameters:
  - `event_name` (const std::string &): Event name
- Return: SpanEvent with current timestamp
- Details: event_name Event name SpanEvent with current timestamp

### themis::process::StateTransitionDeterminismSpec

#### `std::string_view describe()`
- Source: `include/process/process_determinism_spec.h`:192
- Brief: n/a
- Parameters: none

### themis::process::StressScenarioResult

#### `bool isFatal() const`
- Source: `include/process/process_stress_scenarios.h`:95
- Brief: Check if this result is a fatal failure.
- Parameters: none
- Return: true if result indicates unrecoverable failure or silent data loss
- Details: true if result indicates unrecoverable failure or silent data loss

#### `bool isWithinPerformanceEnvelope(int32_t max_latency_ms) const`
- Source: `include/process/process_stress_scenarios.h`:102
- Brief: Check if this result is within performance envelope.
- Parameters:
  - `max_latency_ms` (int32_t): Maximum acceptable latency
- Return: true if latency_ms <= max_latency_ms
- Details: max_latency_ms Maximum acceptable latency true if latency_ms <= max_latency_ms

### themis::process::ThreeWayMergeStrategy

#### `ConflictResolutionResult resolve(const ModelVersion &local, const ModelVersion &remote, const ModelVersion &base) noexcept`
- Source: `include/process/conflict_resolution_plugin.h`:265
- Brief: Resolve conflict using 3-way merge.
- Parameters:
  - `local` (const ModelVersion &): Local model version
  - `remote` (const ModelVersion &): Remote model version
  - `base` (const ModelVersion &): Base model version (common ancestor)
- Return: Result with MERGED strategy and merged_model, or UNRESOLVED if merge impossible
- Details: local Local model version remote Remote model version base Base model version (common ancestor) Result with MERGED strategy and merged_model, or UNRESOLVED if merge impossible

### themis::process::TraceContext

#### `TraceContext FromHeader(const std::string &header)`
- Source: `src/process/process_telemetry_integration.cpp`:129
- Brief: From Header.
- Parameters:
  - `header` (const std::string &): Input parameter.
- Return: Return value.
- Details: header Input parameter. Return value. Calls: size(), substr().

#### `std::string GenerateSpanId()`
- Source: `src/process/process_telemetry_integration.cpp`:108
- Brief: Generate Span Id.
- Parameters: none
- Return: Return value.
- Details: Return value. Calls: rng(), std::setw(), std::setfill(), str().

#### `std::string GenerateTraceId()`
- Source: `src/process/process_telemetry_integration.cpp`:93
- Brief: Generate Trace Id.
- Parameters: none
- Return: Return value.
- Details: Return value. Calls: rng(), std::setw(), std::setfill(), str().

#### `std::string ToHeader() const`
- Source: `src/process/process_telemetry_integration.cpp`:115
- Brief: n/a
- Parameters: none

#### `std::string format() const`
- Source: `include/process/process_diagnostics_api.h`:126
- Brief: Format trace context as a string for logging.
- Parameters: none
- Return: "[trace_id=X, span_id=Y]" or empty string if no context
- Details: "[trace_id=X, span_id=Y]" or empty string if no context

#### `std::optional< TraceContext > fromHeaders(const std::map< std::string, std::string > &headers)`
- Source: `include/process/federated_span_contract.h`:258
- Brief: Parse from W3C Trace Context headers.
- Parameters:
  - `headers` (const std::map< std::string, std::string > &): Map of header_name -> header_value
- Return: Parsed TraceContext, or std::nullopt if invalid
- Details: headers Map of header_name -> header_value Parsed TraceContext, or std::nullopt if invalid

#### `bool isPresent() const`
- Source: `include/process/process_diagnostics_api.h`:132
- Brief: Check if trace context is present.
- Parameters: none
- Return: true if either trace_id or span_id is set
- Details: true if either trace_id or span_id is set

#### `bool isValid() const noexcept`
- Source: `include/process/federated_span_contract.h`:266
- Brief: Check if this context is valid (non-empty trace_id and span_id).
- Parameters: none
- Return: true if valid
- Details: true if valid

#### `std::map< std::string, std::string > toHeaders() const`
- Source: `include/process/federated_span_contract.h`:245
- Brief: Serialize to W3C Trace Context headers.
- Parameters: none
- Return: Map of header_name -> header_value
- Details: Map of header_name -> header_value

### themis::process::UnsupportedGatewayStress

#### `std::string_view describe() const`
- Source: `include/process/process_stress_scenarios.h`:216
- Brief: n/a
- Parameters: none

#### `std::string generateTestInput() const`
- Source: `include/process/process_stress_scenarios.h`:211
- Brief: Generate a test input with unsupported gateway.
- Parameters: none

#### `bool isSuccess(const StressScenarioResult &result) const`
- Source: `include/process/process_stress_scenarios.h`:214
- Brief: Check if result is success (explicit error) for this scenario.
- Parameters:
  - `result` (const StressScenarioResult &): n/a

### themis::process::VccVpbImporter

#### `BPMNNodeType activityTypeToNodeType_(std::string_view type_str)`
- Source: `include/process/vcc_vpb_importer.h`:148
- Brief: Activity Type To Node Type.
- Parameters:
  - `type_str` (std::string_view): Input parameter.
- Return: Return value.
- Details: type_str Input parameter. Return value. Calls: t(), std::transform(), begin(), end().

#### `ProcessDomain domainStringToEnum_(std::string_view domain)`
- Source: `include/process/vcc_vpb_importer.h`:154
- Brief: Domain String To Enum.
- Parameters:
  - `domain` (std::string_view): Input parameter.
- Return: Return value.
- Details: domain Input parameter. Return value. Calls: d(), std::transform(), begin(), end().

#### `ProcessEdgeType edgeTypeToProcessEdgeType_(std::string_view type_str)`
- Source: `include/process/vcc_vpb_importer.h`:151
- Brief: Edge Type To Process Edge Type.
- Parameters:
  - `type_str` (std::string_view): Input parameter.
- Return: Return value.
- Details: type_str Input parameter. Return value. Calls: t(), std::transform(), begin(), end().

#### `std::vector< ImportResult > importDirectory(std::string_view directory_path, const ProcessModelRecord &meta_defaults={})`
- Source: `include/process/vcc_vpb_importer.h`:135
- Brief: Import all *.yaml files from a directory.
- Parameters:
  - `directory_path` (std::string_view): Path to the directory.
  - `meta_defaults` (const ProcessModelRecord &): Input parameter.
- Return: Return value.
- Details: Import Directory. Recursively discovers YAML files and imports them using the standard VCC-VPB schema. Errors per file are collected and returned individually. directory_path Filesystem path to the directory. meta_defaults Metadata defaults applied to every imported model. directory_path Path to the directory. meta_defaults Input parameter. Return value.

#### `ImportResult importYaml(std::string_view yaml_text, const ProcessModelRecord &meta={})`
- Source: `include/process/vcc_vpb_importer.h`:99
- Brief: Parse a VCC-VPB YAML string and produce a ProcessModelRecord.
- Parameters:
  - `yaml_text` (std::string_view): Input parameter.
  - `meta` (const ProcessModelRecord &): Optional metadata overrides (owner, version, state).
- Return: ImportResult with a fully populated record on success.
- Details: Import Yaml. yaml_text Raw YAML content (UTF-8). meta Optional metadata overrides (owner, version, state). ImportResult with a fully populated record on success. Implementation notes: Uses RAII containers for safe memory management (std::string, std::vector, json) All string operations are bounds-safe (std::string uses checked access patterns) Regexes are pre-compiled as static const to avoid repeated compilation Input YAML size is validated before parsing Complexity: O(n log n) where n = length of YAML text (dominated by regex matching) yaml_text Input parameter. meta_defaults Input parameter. Return value.

#### `std::vector< ImportResult > importYamlList(std::string_view yaml_text, std::string_view list_key="administrative_models", const ProcessModelRecord &meta_defaults={})`
- Source: `include/process/vcc_vpb_importer.h`:120
- Brief: Batch-import multiple VCC-VPB models from a single YAML file that uses a top-level list key (e.g. administrative_models:).
- Parameters:
  - `yaml_text` (std::string_view): Input parameter.
  - `list_key` (std::string_view): Input parameter.
  - `meta_defaults` (const ProcessModelRecord &): Input parameter.
- Return: Vector of ImportResult — one per model in the list.
- Details: Import Yaml List. yaml_text Raw YAML content with a top-level list. list_key The top-level YAML key that holds the array of models. meta_defaults Metadata defaults applied to every imported model. Vector of ImportResult — one per model in the list. Implementation notes: Uses RAII containers (std::string, std::vector) for safe memory management Bounds-checks all array/string access via size() before indexing Limits total line count to 100,000 to prevent DoS (malformed input) Pre-compiles all regexes as static const to avoid repeated compilation Complexity: O(n) where n = length of input YAML text yaml_text Input parameter. list_key Input parameter. meta_defaults Input parameter. Return value.

#### `ImportResult parseModelNode_(const nlohmann::json &yaml_as_json, const ProcessModelRecord &meta_defaults)`
- Source: `include/process/vcc_vpb_importer.h`:142
- Brief: Parse Model Node.
- Parameters:
  - `yaml_as_json` (const nlohmann::json &): n/a
  - `meta_defaults` (const ProcessModelRecord &): Input parameter.
- Return: Return value.
- Details: doc Input parameter. meta_defaults Input parameter. Return value.

### themis::process::VersionedLinkState

#### `bool tryTransition(LinkState expected_state, uint32_t expected_version, LinkState new_state) noexcept`
- Source: `include/process/lock_free_linker_contract.h`:171
- Brief: Attempt state transition via CAS.
- Parameters:
  - `expected_state` (LinkState): Expected current state
  - `expected_version` (uint32_t): Expected current version
  - `new_state` (LinkState): New state to transition to
- Return: true if transition succeeded; false if state/version mismatch
- Details: expected_state Expected current state expected_version Expected current version new_state New state to transition to true if transition succeeded; false if state/version mismatch

### themis::process::benchmark

#### `BENCHMARK(BM_PP01_BpmnParse_Small) -> Iterations(10) ->ReportAggregatesOnly(true) ->UseRealTime()`
- Source: `benchmarks/process/bench_process_parser_gates.cpp`:493
- Brief: n/a
- Parameters:
  - `<unnamed>` (BM_PP01_BpmnParse_Small): n/a

#### `BENCHMARK(BM_PP02_BpmnParse_Medium) -> Iterations(5) ->ReportAggregatesOnly(true) ->UseRealTime()`
- Source: `benchmarks/process/bench_process_parser_gates.cpp`:520
- Brief: n/a
- Parameters:
  - `<unnamed>` (BM_PP02_BpmnParse_Medium): n/a

#### `BENCHMARK(BM_PP03_EpkParse) -> Iterations(10) ->ReportAggregatesOnly(true) ->UseRealTime()`
- Source: `benchmarks/process/bench_process_parser_gates.cpp`:546
- Brief: n/a
- Parameters:
  - `<unnamed>` (BM_PP03_EpkParse): n/a

#### `BENCHMARK(BM_PP04_CmmnParse) -> Iterations(10) ->ReportAggregatesOnly(true) ->UseRealTime()`
- Source: `benchmarks/process/bench_process_parser_gates.cpp`:572
- Brief: n/a
- Parameters:
  - `<unnamed>` (BM_PP04_CmmnParse): n/a

#### `BENCHMARK(BM_PP05_DmnParse) -> Iterations(10) ->ReportAggregatesOnly(true) ->UseRealTime()`
- Source: `benchmarks/process/bench_process_parser_gates.cpp`:598
- Brief: n/a
- Parameters:
  - `<unnamed>` (BM_PP05_DmnParse): n/a

#### `BENCHMARK(BM_PP06_OcelParse) -> Iterations(10) ->ReportAggregatesOnly(true) ->UseRealTime()`
- Source: `benchmarks/process/bench_process_parser_gates.cpp`:624
- Brief: n/a
- Parameters:
  - `<unnamed>` (BM_PP06_OcelParse): n/a

#### `BENCHMARK(BM_PP07_VccVpbParse) -> Iterations(10) ->ReportAggregatesOnly(true) ->UseRealTime()`
- Source: `benchmarks/process/bench_process_parser_gates.cpp`:650
- Brief: n/a
- Parameters:
  - `<unnamed>` (BM_PP07_VccVpbParse): n/a

#### `BENCHMARK(BM_PP08_FimParse) -> Iterations(10) ->ReportAggregatesOnly(true) ->UseRealTime()`
- Source: `benchmarks/process/bench_process_parser_gates.cpp`:676
- Brief: n/a
- Parameters:
  - `<unnamed>` (BM_PP08_FimParse): n/a

#### `BENCHMARK(BM_RP01_SimpleQuery) -> Iterations(10) ->ReportAggregatesOnly(true) ->UseRealTime()`
- Source: `benchmarks/process/bench_process_retriever_gates.cpp`:366
- Brief: n/a
- Parameters:
  - `<unnamed>` (BM_RP01_SimpleQuery): n/a

#### `BENCHMARK(BM_RP02_ComplexQuery) -> Iterations(10) ->ReportAggregatesOnly(true) ->UseRealTime()`
- Source: `benchmarks/process/bench_process_retriever_gates.cpp`:395
- Brief: n/a
- Parameters:
  - `<unnamed>` (BM_RP02_ComplexQuery): n/a

#### `BENCHMARK(BM_RP03_FullTextSearch) -> Iterations(10) ->ReportAggregatesOnly(true) ->UseRealTime()`
- Source: `benchmarks/process/bench_process_retriever_gates.cpp`:427
- Brief: n/a
- Parameters:
  - `<unnamed>` (BM_RP03_FullTextSearch): n/a

#### `BENCHMARK(BM_RP04_EmbeddingSimilarity) -> Iterations(10) ->ReportAggregatesOnly(true) ->UseRealTime()`
- Source: `benchmarks/process/bench_process_retriever_gates.cpp`:457
- Brief: n/a
- Parameters:
  - `<unnamed>` (BM_RP04_EmbeddingSimilarity): n/a

#### `BENCHMARK(BM_RP05_PaginationQuery) -> Iterations(5) ->ReportAggregatesOnly(true) ->UseRealTime()`
- Source: `benchmarks/process/bench_process_retriever_gates.cpp`:484
- Brief: n/a
- Parameters:
  - `<unnamed>` (BM_RP05_PaginationQuery): n/a

#### `BENCHMARK(BM_RP06_ConcurrentQuery) -> Iterations(5) ->ReportAggregatesOnly(true) ->UseRealTime()`
- Source: `benchmarks/process/bench_process_retriever_gates.cpp`:532
- Brief: n/a
- Parameters:
  - `<unnamed>` (BM_RP06_ConcurrentQuery): n/a

#### `BENCHMARK(BM_RP07_QueryUnderChurn) -> Iterations(5) ->ReportAggregatesOnly(true) ->UseRealTime()`
- Source: `benchmarks/process/bench_process_retriever_gates.cpp`:593
- Brief: n/a
- Parameters:
  - `<unnamed>` (BM_RP07_QueryUnderChurn): n/a

#### `BENCHMARK(BM_RP08_RankingAndSorting) -> Iterations(10) ->ReportAggregatesOnly(true) ->UseRealTime()`
- Source: `benchmarks/process/bench_process_retriever_gates.cpp`:618
- Brief: n/a
- Parameters:
  - `<unnamed>` (BM_RP08_RankingAndSorting): n/a

#### `void BM_BE01_MultiFormatImport(benchmark::State &state)`
- Source: `benchmarks/process/bench_process_advanced_workflows.cpp`:380
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a
- Details: BE-01: Multi-Format Import (5 formats, 100 files each)

#### `void BM_BE02_AlphaMiner(benchmark::State &state)`
- Source: `benchmarks/process/bench_process_advanced_workflows.cpp`:410
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a
- Details: BE-02: Process Mining Alpha Algorithm (1k event log)

#### `void BM_BE03_HeuristicMiner(benchmark::State &state)`
- Source: `benchmarks/process/bench_process_advanced_workflows.cpp`:432
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a
- Details: BE-03: Process Mining Heuristic Algorithm (1k event log)

#### `void BM_BE04_InductiveMiner(benchmark::State &state)`
- Source: `benchmarks/process/bench_process_advanced_workflows.cpp`:454
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a
- Details: BE-04: Process Mining Inductive Algorithm (1k event log)

#### `void BM_BE05_ConformanceChecking(benchmark::State &state)`
- Source: `benchmarks/process/bench_process_advanced_workflows.cpp`:476
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a
- Details: BE-05: Conformance Checking (DFG vs log, 1k events)

#### `void BM_BE06_VariantAnalysis(benchmark::State &state)`
- Source: `benchmarks/process/bench_process_advanced_workflows.cpp`:499
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a
- Details: BE-06: Variant Analysis (event clustering, 1k events)

#### `void BM_BE07_LlmDescriptor(benchmark::State &state)`
- Source: `benchmarks/process/bench_process_advanced_workflows.cpp`:521
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a
- Details: BE-07: LLM Process Descriptor (100 models)

#### `void BM_BE08_BpmnToDfgConversion(benchmark::State &state)`
- Source: `benchmarks/process/bench_process_advanced_workflows.cpp`:543
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a
- Details: BE-08: BPMN to DFG Conversion (100 models)

#### `void BM_BE09_CommunityDetection(benchmark::State &state)`
- Source: `benchmarks/process/bench_process_advanced_workflows.cpp`:565
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a
- Details: BE-09: Process Community Detection (1k model graph)

#### `void BM_BE10_RagRetrieval(benchmark::State &state)`
- Source: `benchmarks/process/bench_process_advanced_workflows.cpp`:584
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a
- Details: BE-10: RAG Knowledge Retrieval (1k models + queries)

#### `void BM_BE11_EndToEndScenario(benchmark::State &state)`
- Source: `benchmarks/process/bench_process_advanced_workflows.cpp`:610
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a
- Details: BE-11: Combined End-to-End Scenario (import + mining + retrieval)

#### `void BM_BE12_StressTest(benchmark::State &state)`
- Source: `benchmarks/process/bench_process_advanced_workflows.cpp`:648
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a
- Details: BE-12: Sustained Load Stress Test

#### `void BM_CP01_ConcurrentCrud_Small(benchmark::State &state)`
- Source: `benchmarks/process/bench_process_concurrency_gates.cpp`:161
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_CP02_ConcurrentCrud_Medium(benchmark::State &state)`
- Source: `benchmarks/process/bench_process_concurrency_gates.cpp`:254
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_CP03_ConcurrentImport_Bpmn(benchmark::State &state)`
- Source: `benchmarks/process/bench_process_concurrency_gates.cpp`:345
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_CP04_ConcurrentExport(benchmark::State &state)`
- Source: `benchmarks/process/bench_process_concurrency_gates.cpp`:392
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_CP05_ConcurrentLinking(benchmark::State &state)`
- Source: `benchmarks/process/bench_process_concurrency_gates.cpp`:451
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_CP06_ConcurrentRetrieval_Medium(benchmark::State &state)`
- Source: `benchmarks/process/bench_process_concurrency_gates.cpp`:520
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_DP01_ConflictResolution(benchmark::State &state)`
- Source: `benchmarks/process/bench_process_determinism_gates.cpp`:276
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_DP02_RollbackSingle(benchmark::State &state)`
- Source: `benchmarks/process/bench_process_determinism_gates.cpp`:314
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_DP03_RollbackBatch(benchmark::State &state)`
- Source: `benchmarks/process/bench_process_determinism_gates.cpp`:350
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_DP04_TransactionSerialization(benchmark::State &state)`
- Source: `benchmarks/process/bench_process_determinism_gates.cpp`:395
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_DP05_DeterministicOutputVerification(benchmark::State &state)`
- Source: `benchmarks/process/bench_process_determinism_gates.cpp`:457
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_DP06_VersionClockOperations(benchmark::State &state)`
- Source: `benchmarks/process/bench_process_determinism_gates.cpp`:525
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_GO01_ClassificationBaseline(benchmark::State &state)`
- Source: `benchmarks/process/bench_process_diagnostics_overhead.cpp`:279
- Brief: Measure baseline classification performance.
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_GO01_ClassificationEnhanced(benchmark::State &state)`
- Source: `benchmarks/process/bench_process_diagnostics_overhead.cpp`:306
- Brief: Measure enhanced classification performance (with diagnostics).
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_GO01_ClassificationOverhead(benchmark::State &state)`
- Source: `benchmarks/process/bench_process_diagnostics_overhead.cpp`:333
- Brief: Measure classification overhead ratio (Enhanced/Baseline).
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_GO02_LinkTraversalLatency(benchmark::State &state)`
- Source: `benchmarks/process/bench_process_diagnostics_overhead.cpp`:412
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_GO03_GraphConstructionTime(benchmark::State &state)`
- Source: `benchmarks/process/bench_process_diagnostics_overhead.cpp`:463
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_GO04_CycleDetectionPerformance(benchmark::State &state)`
- Source: `benchmarks/process/bench_process_diagnostics_overhead.cpp`:515
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_GO05_CommunityDetection(benchmark::State &state)`
- Source: `benchmarks/process/bench_process_diagnostics_overhead.cpp`:590
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_GO06_ComplexGraphP95Latency(benchmark::State &state)`
- Source: `benchmarks/process/bench_process_diagnostics_overhead.cpp`:668
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_LP01_LinkingLatency(benchmark::State &state)`
- Source: `benchmarks/process/bench_process_linker_gates.cpp`:253
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_LP02_CyclicDependencyDetection(benchmark::State &state)`
- Source: `benchmarks/process/bench_process_linker_gates.cpp`:297
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_LP03_LinkValidation(benchmark::State &state)`
- Source: `benchmarks/process/bench_process_linker_gates.cpp`:349
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_LP04_GraphTraversal(benchmark::State &state)`
- Source: `benchmarks/process/bench_process_linker_gates.cpp`:392
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_LP05_StaleLinkDetection(benchmark::State &state)`
- Source: `benchmarks/process/bench_process_linker_gates.cpp`:460
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_LP06_BatchLinkOperations(benchmark::State &state)`
- Source: `benchmarks/process/bench_process_linker_gates.cpp`:523
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_PP01_BpmnParse_Small(benchmark::State &state)`
- Source: `benchmarks/process/bench_process_parser_gates.cpp`:456
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a
- Details: PP-01: BPMN Parse (100 files)

#### `void BM_PP02_BpmnParse_Medium(benchmark::State &state)`
- Source: `benchmarks/process/bench_process_parser_gates.cpp`:498
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a
- Details: PP-02: BPMN Parse (1k files)

#### `void BM_PP03_EpkParse(benchmark::State &state)`
- Source: `benchmarks/process/bench_process_parser_gates.cpp`:525
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a
- Details: PP-03: EPK Parse (100 files)

#### `void BM_PP04_CmmnParse(benchmark::State &state)`
- Source: `benchmarks/process/bench_process_parser_gates.cpp`:551
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a
- Details: PP-04: CMMN Parse (100 files)

#### `void BM_PP05_DmnParse(benchmark::State &state)`
- Source: `benchmarks/process/bench_process_parser_gates.cpp`:577
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a
- Details: PP-05: DMN Parse (100 files)

#### `void BM_PP06_OcelParse(benchmark::State &state)`
- Source: `benchmarks/process/bench_process_parser_gates.cpp`:603
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a
- Details: PP-06: OCEL Parse (100 logs)

#### `void BM_PP07_VccVpbParse(benchmark::State &state)`
- Source: `benchmarks/process/bench_process_parser_gates.cpp`:629
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a
- Details: PP-07: VCC/VPB Parse (100 files)

#### `void BM_PP08_FimParse(benchmark::State &state)`
- Source: `benchmarks/process/bench_process_parser_gates.cpp`:655
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a
- Details: PP-08: FIM Parse (100 files)

#### `void BM_RP01_SimpleQuery(benchmark::State &state)`
- Source: `benchmarks/process/bench_process_retriever_gates.cpp`:326
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a
- Details: RP-01: Simple Query (1k models)

#### `void BM_RP02_ComplexQuery(benchmark::State &state)`
- Source: `benchmarks/process/bench_process_retriever_gates.cpp`:371
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a
- Details: RP-02: Complex Query (1k models)

#### `void BM_RP03_FullTextSearch(benchmark::State &state)`
- Source: `benchmarks/process/bench_process_retriever_gates.cpp`:400
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a
- Details: RP-03: Full-Text Search (1k models)

#### `void BM_RP04_EmbeddingSimilarity(benchmark::State &state)`
- Source: `benchmarks/process/bench_process_retriever_gates.cpp`:432
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a
- Details: RP-04: Embedding Similarity Search (1k models)

#### `void BM_RP05_PaginationQuery(benchmark::State &state)`
- Source: `benchmarks/process/bench_process_retriever_gates.cpp`:462
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a
- Details: RP-05: Pagination Query (10k models)

#### `void BM_RP06_ConcurrentQuery(benchmark::State &state)`
- Source: `benchmarks/process/bench_process_retriever_gates.cpp`:489
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a
- Details: RP-06: Concurrent Query (1k models, 4 threads)

#### `void BM_RP07_QueryUnderChurn(benchmark::State &state)`
- Source: `benchmarks/process/bench_process_retriever_gates.cpp`:537
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a
- Details: RP-07: Query Under Churn (1k -> 10k models)

#### `void BM_RP08_RankingAndSorting(benchmark::State &state)`
- Source: `benchmarks/process/bench_process_retriever_gates.cpp`:598
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a
- Details: RP-08: Ranking/Sorting (1k results)

#### `Iterations(10) -> ReportAggregatesOnly(true) ->UseRealTime()`
- Source: `benchmarks/process/bench_process_advanced_workflows.cpp`:425
- Brief: n/a
- Parameters:
  - `<unnamed>` (10): n/a

#### `Iterations(20) -> ReportAggregatesOnly(true) ->UseRealTime()`
- Source: `benchmarks/process/bench_process_determinism_gates.cpp`:342
- Brief: n/a
- Parameters:
  - `<unnamed>` (20): n/a

#### `Iterations(3) -> ReportAggregatesOnly(true) ->UseRealTime()`
- Source: `benchmarks/process/bench_process_diagnostics_overhead.cpp`:726
- Brief: n/a
- Parameters:
  - `<unnamed>` (3): n/a

#### `Iterations(5) -> ReportAggregatesOnly(true) ->UseRealTime()`
- Source: `benchmarks/process/bench_process_advanced_workflows.cpp`:403
- Brief: n/a
- Parameters:
  - `<unnamed>` (5): n/a

#### `std::string generateBpmnXml(const std::string &model_name)`
- Source: `benchmarks/process/bench_process_concurrency_gates.cpp`:121
- Brief: Generate realistic BPMN XML content.
- Parameters:
  - `model_name` (const std::string &): n/a

#### `std::string generateBpmnXml(int model_idx)`
- Source: `benchmarks/process/bench_process_parser_gates.cpp`:302
- Brief: Generate realistic BPMN XML content.
- Parameters:
  - `model_idx` (int): n/a

#### `std::string generateCmmnXml(int model_idx)`
- Source: `benchmarks/process/bench_process_parser_gates.cpp`:352
- Brief: Generate CMMN XML content.
- Parameters:
  - `model_idx` (int): n/a

#### `std::vector< ProcessConflict > generateConflicts(int count)`
- Source: `benchmarks/process/bench_process_determinism_gates.cpp`:221
- Brief: Generate synthetic conflicts.
- Parameters:
  - `count` (int): n/a

#### `DirectlyFollowsGraph generateDfg(const EventLog &log)`
- Source: `benchmarks/process/bench_process_advanced_workflows.cpp`:347
- Brief: Generate synthetic DFG.
- Parameters:
  - `log` (const EventLog &): n/a

#### `std::vector< DiagnosticEvent > generateDiagnosticEvents(int count)`
- Source: `benchmarks/process/bench_process_diagnostics_overhead.cpp`:201
- Brief: Generate synthetic diagnostic events.
- Parameters:
  - `count` (int): n/a

#### `std::string generateDmnXml(int model_idx)`
- Source: `benchmarks/process/bench_process_parser_gates.cpp`:370
- Brief: Generate DMN XML content.
- Parameters:
  - `model_idx` (int): n/a

#### `std::vector< float > generateEmbedding(const std::string &text)`
- Source: `benchmarks/process/bench_process_retriever_gates.cpp`:250
- Brief: Generate synthetic embedding vector.
- Parameters:
  - `text` (const std::string &): n/a

#### `std::string generateEpkXml(int model_idx)`
- Source: `benchmarks/process/bench_process_parser_gates.cpp`:330
- Brief: Generate EPK XML content.
- Parameters:
  - `model_idx` (int): n/a

#### `EventLog generateEventLog(int num_cases, int events_per_case)`
- Source: `benchmarks/process/bench_process_advanced_workflows.cpp`:309
- Brief: Generate synthetic event log.
- Parameters:
  - `num_cases` (int): n/a
  - `events_per_case` (int): n/a

#### `std::string generateFimXml(int model_idx)`
- Source: `benchmarks/process/bench_process_parser_gates.cpp`:435
- Brief: Generate FIM content.
- Parameters:
  - `model_idx` (int): n/a

#### `std::vector< ProcessIncident > generateIncidents(int count)`
- Source: `benchmarks/process/bench_process_diagnostics_overhead.cpp`:245
- Brief: Generate synthetic incidents.
- Parameters:
  - `count` (int): n/a

#### `std::vector< std::pair< int, int > > generateLinkChain(int length)`
- Source: `benchmarks/process/bench_process_linker_gates.cpp`:241
- Brief: Generate a link chain (for cycle testing).
- Parameters:
  - `length` (int): n/a

#### `std::vector< ProcessModelRecord > generateModelCollection(int count)`
- Source: `benchmarks/process/bench_process_retriever_gates.cpp`:283
- Brief: Generate synthetic process model collection.
- Parameters:
  - `count` (int): n/a

#### `std::string generateOcelJson(int log_idx)`
- Source: `benchmarks/process/bench_process_parser_gates.cpp`:387
- Brief: Generate OCEL JSON content.
- Parameters:
  - `log_idx` (int): n/a

#### `std::vector< std::string > generateProcessModels(int count)`
- Source: `benchmarks/process/bench_process_linker_gates.cpp`:206
- Brief: Generate random process models for linking.
- Parameters:
  - `count` (int): n/a

#### `std::vector< std::pair< int, int > > generateRandomLinks(int model_count, int link_count)`
- Source: `benchmarks/process/bench_process_linker_gates.cpp`:218
- Brief: Generate random links between models.
- Parameters:
  - `model_count` (int): n/a
  - `link_count` (int): n/a

#### `std::vector< ProcessModelRevision > generateRevisionHistory(const std::string &model_id, int num_revisions)`
- Source: `benchmarks/process/bench_process_determinism_gates.cpp`:254
- Brief: Generate revision history.
- Parameters:
  - `model_id` (const std::string &): n/a
  - `num_revisions` (int): n/a

#### `std::string generateVccVpbXml(int model_idx)`
- Source: `benchmarks/process/bench_process_parser_gates.cpp`:418
- Brief: Generate VCC/VPB content.
- Parameters:
  - `model_idx` (int): n/a

#### `std::string simulateBpmnExport(const SimProcessModel &model)`
- Source: `benchmarks/process/bench_process_concurrency_gates.cpp`:144
- Brief: Simulate BPMN export operation.
- Parameters:
  - `model` (const SimProcessModel &): n/a

#### `SimProcessModel simulateBpmnImport(const std::string &xml_content, int64_t now_ms)`
- Source: `benchmarks/process/bench_process_concurrency_gates.cpp`:128
- Brief: Simulate BPMN import operation.
- Parameters:
  - `xml_content` (const std::string &): n/a
  - `now_ms` (int64_t): n/a

#### `bool simulateLink(const SimProcessModel &source, const SimProcessModel &target)`
- Source: `benchmarks/process/bench_process_concurrency_gates.cpp`:151
- Brief: Simulate linking operation.
- Parameters:
  - `source` (const SimProcessModel &): n/a
  - `target` (const SimProcessModel &): n/a

### themis::process::benchmark::BaselineIncidentClassifier

#### `BaselineIncidentClassifier()=default`
- Source: `benchmarks/process/bench_process_diagnostics_overhead.cpp`:66
- Brief: n/a
- Parameters: none

#### `IncidentType classifyBaseline(const ProcessIncident &incident)`
- Source: `benchmarks/process/bench_process_diagnostics_overhead.cpp`:71
- Brief: Classify incident without detailed diagnostics.
- Parameters:
  - `incident` (const ProcessIncident &): n/a

### themis::process::benchmark::BpmnParser

#### `BpmnModel parse(const std::string &xml_content)`
- Source: `benchmarks/process/bench_process_parser_gates.cpp`:37
- Brief: n/a
- Parameters:
  - `xml_content` (const std::string &): n/a

### themis::process::benchmark::CmmnParser

#### `CmmnModel parse(const std::string &xml_content)`
- Source: `benchmarks/process/bench_process_parser_gates.cpp`:144
- Brief: n/a
- Parameters:
  - `xml_content` (const std::string &): n/a

### themis::process::benchmark::CommunityDetector

#### `std::vector< Community > detectCommunities(int num_models)`
- Source: `benchmarks/process/bench_process_advanced_workflows.cpp`:278
- Brief: Detect communities in process model graph (louvain simulation).
- Parameters:
  - `num_models` (int): n/a

### themis::process::benchmark::ConcurrentModelStore

#### `ConcurrentModelStore()=default`
- Source: `benchmarks/process/bench_process_concurrency_gates.cpp`:49
- Brief: n/a
- Parameters: none

#### `void clear()`
- Source: `benchmarks/process/bench_process_concurrency_gates.cpp`:111
- Brief: n/a
- Parameters: none

#### `bool create(const SimProcessModel &model)`
- Source: `benchmarks/process/bench_process_concurrency_gates.cpp`:53
- Brief: n/a
- Parameters:
  - `model` (const SimProcessModel &): n/a

#### `bool delete_model(const std::string &id)`
- Source: `benchmarks/process/bench_process_concurrency_gates.cpp`:89
- Brief: n/a
- Parameters:
  - `id` (const std::string &): n/a

#### `std::vector< SimProcessModel > list()`
- Source: `benchmarks/process/bench_process_concurrency_gates.cpp`:102
- Brief: n/a
- Parameters: none

#### `int64_t op_counter() const`
- Source: `benchmarks/process/bench_process_concurrency_gates.cpp`:108
- Brief: n/a
- Parameters: none

#### `bool read(const std::string &id, SimProcessModel &out)`
- Source: `benchmarks/process/bench_process_concurrency_gates.cpp`:61
- Brief: n/a
- Parameters:
  - `id` (const std::string &): n/a
  - `out` (SimProcessModel &): n/a

#### `size_t size() const`
- Source: `benchmarks/process/bench_process_concurrency_gates.cpp`:109
- Brief: n/a
- Parameters: none

#### `bool update(const std::string &id, const SimProcessModel &model)`
- Source: `benchmarks/process/bench_process_concurrency_gates.cpp`:74
- Brief: n/a
- Parameters:
  - `id` (const std::string &): n/a
  - `model` (const SimProcessModel &): n/a

#### `~ConcurrentModelStore()=default`
- Source: `benchmarks/process/bench_process_concurrency_gates.cpp`:50
- Brief: n/a
- Parameters: none

### themis::process::benchmark::ConflictResolver

#### `ConflictResolver()=default`
- Source: `benchmarks/process/bench_process_determinism_gates.cpp`:50
- Brief: n/a
- Parameters: none

#### `bool resolveConflict(const ProcessConflict &conflict)`
- Source: `benchmarks/process/bench_process_determinism_gates.cpp`:55
- Brief: Simulate conflict detection and resolution.
- Parameters:
  - `conflict` (const ProcessConflict &): n/a

#### `size_t resolvedCount() const`
- Source: `benchmarks/process/bench_process_determinism_gates.cpp`:88
- Brief: Get resolution statistics.
- Parameters: none

### themis::process::benchmark::ConformanceChecker

#### `double checkConformance(const EventLog &log, const DirectlyFollowsGraph &dfg)`
- Source: `benchmarks/process/bench_process_advanced_workflows.cpp`:146
- Brief: Check log conformance against DFG.
- Parameters:
  - `log` (const EventLog &): n/a
  - `dfg` (const DirectlyFollowsGraph &): n/a

### themis::process::benchmark::DmnParser

#### `DmnModel parse(const std::string &xml_content)`
- Source: `benchmarks/process/bench_process_parser_gates.cpp`:186
- Brief: n/a
- Parameters:
  - `xml_content` (const std::string &): n/a

### themis::process::benchmark::EnhancedIncidentClassifier

#### `EnhancedIncidentClassifier()`
- Source: `benchmarks/process/bench_process_diagnostics_overhead.cpp`:107
- Brief: n/a
- Parameters: none

#### `void analyzeIncidentDetails(const ProcessIncident &incident)`
- Source: `benchmarks/process/bench_process_diagnostics_overhead.cpp`:162
- Brief: Simulate detailed incident analysis (the overhead).
- Parameters:
  - `incident` (const ProcessIncident &): n/a

#### `IncidentType classifyWithDiagnostics(const ProcessIncident &incident)`
- Source: `benchmarks/process/bench_process_diagnostics_overhead.cpp`:124
- Brief: Classify incident with full diagnostics.
- Parameters:
  - `incident` (const ProcessIncident &): n/a

### themis::process::benchmark::EpkParser

#### `EpkModel parse(const std::string &xml_content)`
- Source: `benchmarks/process/bench_process_parser_gates.cpp`:102
- Brief: n/a
- Parameters:
  - `xml_content` (const std::string &): n/a

### themis::process::benchmark::FimParser

#### `FimModel parse(const std::string &content)`
- Source: `benchmarks/process/bench_process_parser_gates.cpp`:277
- Brief: n/a
- Parameters:
  - `content` (const std::string &): n/a

### themis::process::benchmark::LlmProcessDescriptor

#### `std::string generateDescription(const EventLog &log)`
- Source: `benchmarks/process/bench_process_advanced_workflows.cpp`:237
- Brief: Generate natural language description of process.
- Parameters:
  - `log` (const EventLog &): n/a

### themis::process::benchmark::OcelParser

#### `OcelLog parse(const std::string &json_content)`
- Source: `benchmarks/process/bench_process_parser_gates.cpp`:216
- Brief: n/a
- Parameters:
  - `json_content` (const std::string &): n/a

### themis::process::benchmark::ProcessLinker

#### `ProcessLinker()=default`
- Source: `benchmarks/process/bench_process_linker_gates.cpp`:58
- Brief: n/a
- Parameters: none

#### `bool createLink(const std::string &source_id, const std::string &target_id, const std::string &link_type)`
- Source: `benchmarks/process/bench_process_linker_gates.cpp`:63
- Brief: Create a link between two models.
- Parameters:
  - `source_id` (const std::string &): n/a
  - `target_id` (const std::string &): n/a
  - `link_type` (const std::string &): n/a

#### `std::vector< std::string > getReachableNodes(const std::string &start_id)`
- Source: `benchmarks/process/bench_process_linker_gates.cpp`:129
- Brief: Get all nodes reachable from a starting node (graph traversal).
- Parameters:
  - `start_id` (const std::string &): n/a

#### `bool hasCycle()`
- Source: `benchmarks/process/bench_process_linker_gates.cpp`:111
- Brief: Detect cycles using DFS.
- Parameters: none

#### `bool hasCycleDFS(const std::string &node_id)`
- Source: `benchmarks/process/bench_process_linker_gates.cpp`:176
- Brief: DFS helper for cycle detection.
- Parameters:
  - `node_id` (const std::string &): n/a

#### `size_t linkCount() const`
- Source: `benchmarks/process/bench_process_linker_gates.cpp`:169
- Brief: n/a
- Parameters: none

#### `size_t nodeCount() const`
- Source: `benchmarks/process/bench_process_linker_gates.cpp`:170
- Brief: n/a
- Parameters: none

#### `int validateAllLinks()`
- Source: `benchmarks/process/bench_process_linker_gates.cpp`:159
- Brief: Validate all links in the graph.
- Parameters: none

#### `bool validateLink(const std::string &source_id, const std::string &target_id)`
- Source: `benchmarks/process/bench_process_linker_gates.cpp`:98
- Brief: Validate a link exists and is correct.
- Parameters:
  - `source_id` (const std::string &): n/a
  - `target_id` (const std::string &): n/a

### themis::process::benchmark::ProcessMiningEngine

#### `EventLog alphaMiner(const EventLog &input_log)`
- Source: `benchmarks/process/bench_process_advanced_workflows.cpp`:51
- Brief: Alpha Miner algorithm simulation Discovers process model from event log using footprint matrix.
- Parameters:
  - `input_log` (const EventLog &): n/a

#### `EventLog heuristicMiner(const EventLog &input_log)`
- Source: `benchmarks/process/bench_process_advanced_workflows.cpp`:81
- Brief: Heuristic Miner algorithm simulation.
- Parameters:
  - `input_log` (const EventLog &): n/a

#### `EventLog inductiveMiner(const EventLog &input_log)`
- Source: `benchmarks/process/bench_process_advanced_workflows.cpp`:111
- Brief: Inductive Miner algorithm simulation.
- Parameters:
  - `input_log` (const EventLog &): n/a

### themis::process::benchmark::ProcessModelRetriever

#### `ProcessModelRetriever()=default`
- Source: `benchmarks/process/bench_process_retriever_gates.cpp`:59
- Brief: n/a
- Parameters: none

#### `void addModel(const ProcessModelRecord &model)`
- Source: `benchmarks/process/bench_process_retriever_gates.cpp`:64
- Brief: Add a process model to the store.
- Parameters:
  - `model` (const ProcessModelRecord &): n/a

#### `QueryResult complexQuery(const std::string &state, const std::string &name_pattern, int min_revision, int limit=10)`
- Source: `benchmarks/process/bench_process_retriever_gates.cpp`:91
- Brief: Complex query with multiple filters.
- Parameters:
  - `state` (const std::string &): n/a
  - `name_pattern` (const std::string &): n/a
  - `min_revision` (int): n/a
  - `limit` (int): n/a

#### `float cosineSimilarity(const std::vector< float > &a, const std::vector< float > &b)`
- Source: `benchmarks/process/bench_process_retriever_gates.cpp`:217
- Brief: Calculate cosine similarity between two embedding vectors.
- Parameters:
  - `a` (const std::vector< float > &): n/a
  - `b` (const std::vector< float > &): n/a

#### `QueryResult embeddingSimilaritySearch(const std::vector< float > &query_embedding, float min_similarity=0.7f, int limit=10)`
- Source: `benchmarks/process/bench_process_retriever_gates.cpp`:134
- Brief: Embedding similarity search (cosine distance).
- Parameters:
  - `query_embedding` (const std::vector< float > &): n/a
  - `min_similarity` (float): n/a
  - `limit` (int): n/a

#### `QueryResult fullTextSearch(const std::string &query_term, int limit=10)`
- Source: `benchmarks/process/bench_process_retriever_gates.cpp`:115
- Brief: Full-text search on description.
- Parameters:
  - `query_term` (const std::string &): n/a
  - `limit` (int): n/a

#### `int64_t lastModifiedMs() const`
- Source: `benchmarks/process/bench_process_retriever_gates.cpp`:211
- Brief: n/a
- Parameters: none

#### `size_t modelCount() const`
- Source: `benchmarks/process/bench_process_retriever_gates.cpp`:210
- Brief: n/a
- Parameters: none

#### `QueryResult paginatedQuery(const std::string &state, int page, int page_size=20)`
- Source: `benchmarks/process/bench_process_retriever_gates.cpp`:164
- Brief: Paginated query.
- Parameters:
  - `state` (const std::string &): n/a
  - `page` (int): n/a
  - `page_size` (int): n/a

#### `QueryResult rankAndSort(const std::vector< ProcessModelRecord > &candidates, int limit=10)`
- Source: `benchmarks/process/bench_process_retriever_gates.cpp`:192
- Brief: Retrieve and rank models by relevance.
- Parameters:
  - `candidates` (const std::vector< ProcessModelRecord > &): n/a
  - `limit` (int): n/a

#### `QueryResult simpleQuery(const std::string &state_filter, int limit=10)`
- Source: `benchmarks/process/bench_process_retriever_gates.cpp`:72
- Brief: Simple query by name/state filter.
- Parameters:
  - `state_filter` (const std::string &): n/a
  - `limit` (int): n/a

### themis::process::benchmark::RevisionStore

#### `RevisionStore()=default`
- Source: `benchmarks/process/bench_process_determinism_gates.cpp`:99
- Brief: n/a
- Parameters: none

#### `void addRevision(const ProcessModelRevision &rev)`
- Source: `benchmarks/process/bench_process_determinism_gates.cpp`:104
- Brief: Add a new revision.
- Parameters:
  - `rev` (const ProcessModelRevision &): n/a

#### `int batchRollback(const std::vector< std::string > &model_ids, int target_revision)`
- Source: `benchmarks/process/bench_process_determinism_gates.cpp`:150
- Brief: Simulate batch rollback.
- Parameters:
  - `model_ids` (const std::vector< std::string > &): n/a
  - `target_revision` (int): n/a

#### `std::vector< ProcessModelRevision > getRevisions(const std::string &model_id) const`
- Source: `benchmarks/process/bench_process_determinism_gates.cpp`:111
- Brief: Get all revisions for a model.
- Parameters:
  - `model_id` (const std::string &): n/a

#### `bool rollbackToRevision(const std::string &model_id, int target_revision)`
- Source: `benchmarks/process/bench_process_determinism_gates.cpp`:129
- Brief: Simulate single-model rollback.
- Parameters:
  - `model_id` (const std::string &): n/a
  - `target_revision` (int): n/a

#### `size_t totalRevisions() const`
- Source: `benchmarks/process/bench_process_determinism_gates.cpp`:160
- Brief: n/a
- Parameters: none

### themis::process::benchmark::TransactionSerializer

#### `TransactionSerializer()=default`
- Source: `benchmarks/process/bench_process_determinism_gates.cpp`:181
- Brief: n/a
- Parameters: none

#### `bool commitTransaction(const Transaction &txn)`
- Source: `benchmarks/process/bench_process_determinism_gates.cpp`:186
- Brief: Serialize and commit a transaction.
- Parameters:
  - `txn` (const Transaction &): n/a

#### `size_t committedCount() const`
- Source: `benchmarks/process/bench_process_determinism_gates.cpp`:211
- Brief: n/a
- Parameters: none

### themis::process::benchmark::VariantAnalyzer

#### `std::vector< Variant > analyzeVariants(const EventLog &log)`
- Source: `benchmarks/process/bench_process_advanced_workflows.cpp`:191
- Brief: Analyze event log variants (case traces).
- Parameters:
  - `log` (const EventLog &): n/a

### themis::process::benchmark::VccVpbParser

#### `VccVpbModel parse(const std::string &content)`
- Source: `benchmarks/process/bench_process_parser_gates.cpp`:250
- Brief: n/a
- Parameters:
  - `content` (const std::string &): n/a

### themis::process::benchmark::VersionClock

#### `void increment()`
- Source: `benchmarks/process/bench_process_determinism_gates.cpp`:512
- Brief: n/a
- Parameters: none

#### `bool isAfter(const VersionClock &other) const`
- Source: `benchmarks/process/bench_process_determinism_gates.cpp`:517
- Brief: n/a
- Parameters:
  - `other` (const VersionClock &): n/a

### themis::process::test

#### `TEST_F(ProcessAgenticRagTest, EXS01_EncodeContextBasicPrompt)`
- Source: `tests/process/test_process_high_batch_2a.cpp`:72
- Brief: n/a
- Parameters:
  - `<unnamed>` (ProcessAgenticRagTest): n/a
  - `<unnamed>` (EXS01_EncodeContextBasicPrompt): n/a

#### `TEST_F(ProcessAgenticRagTest, EXS02_EncodeContextWithSubgraph)`
- Source: `tests/process/test_process_high_batch_2a.cpp`:86
- Brief: n/a
- Parameters:
  - `<unnamed>` (ProcessAgenticRagTest): n/a
  - `<unnamed>` (EXS02_EncodeContextWithSubgraph): n/a

#### `TEST_F(ProcessAgenticRagTest, EXS03_EncodeContextAttachments)`
- Source: `tests/process/test_process_high_batch_2a.cpp`:108
- Brief: n/a
- Parameters:
  - `<unnamed>` (ProcessAgenticRagTest): n/a
  - `<unnamed>` (EXS03_EncodeContextAttachments): n/a

#### `TEST_F(ProcessAgenticRagTest, EXS04_EncodeContextSimilarCases)`
- Source: `tests/process/test_process_high_batch_2a.cpp`:128
- Brief: n/a
- Parameters:
  - `<unnamed>` (ProcessAgenticRagTest): n/a
  - `<unnamed>` (EXS04_EncodeContextSimilarCases): n/a

#### `TEST_F(ProcessAgenticRagTest, EXS05_EncodeContextMissingDocuments)`
- Source: `tests/process/test_process_high_batch_2a.cpp`:147
- Brief: n/a
- Parameters:
  - `<unnamed>` (ProcessAgenticRagTest): n/a
  - `<unnamed>` (EXS05_EncodeContextMissingDocuments): n/a

#### `TEST_F(ProcessAgenticRagTest, EXS06_EncodeContextEmpty)`
- Source: `tests/process/test_process_high_batch_2a.cpp`:167
- Brief: n/a
- Parameters:
  - `<unnamed>` (ProcessAgenticRagTest): n/a
  - `<unnamed>` (EXS06_EncodeContextEmpty): n/a

#### `TEST_F(ProcessAgenticRagTest, EXS07_MergeDocumentsEmpty)`
- Source: `tests/process/test_process_high_batch_2a.cpp`:179
- Brief: n/a
- Parameters:
  - `<unnamed>` (ProcessAgenticRagTest): n/a
  - `<unnamed>` (EXS07_MergeDocumentsEmpty): n/a

#### `TEST_F(ProcessAgenticRagTest, EXS08_MergeDocumentsDuplicateDetection)`
- Source: `tests/process/test_process_high_batch_2a.cpp`:190
- Brief: n/a
- Parameters:
  - `<unnamed>` (ProcessAgenticRagTest): n/a
  - `<unnamed>` (EXS08_MergeDocumentsDuplicateDetection): n/a

#### `TEST_F(ProcessAgenticRagTest, EXS09_MergeDocumentsAddition)`
- Source: `tests/process/test_process_high_batch_2a.cpp`:208
- Brief: n/a
- Parameters:
  - `<unnamed>` (ProcessAgenticRagTest): n/a
  - `<unnamed>` (EXS09_MergeDocumentsAddition): n/a

#### `TEST_F(ProcessAgenticRagTest, EXS10_EncodeContextLargeIndices)`
- Source: `tests/process/test_process_high_batch_2a.cpp`:225
- Brief: n/a
- Parameters:
  - `<unnamed>` (ProcessAgenticRagTest): n/a
  - `<unnamed>` (EXS10_EncodeContextLargeIndices): n/a

#### `TEST_F(VccVpbImporterTest, EXS11_ImportYamlBasic)`
- Source: `tests/process/test_process_high_batch_2a.cpp`:282
- Brief: n/a
- Parameters:
  - `<unnamed>` (VccVpbImporterTest): n/a
  - `<unnamed>` (EXS11_ImportYamlBasic): n/a

#### `TEST_F(VccVpbImporterTest, EXS12_ImportYamlCompliance)`
- Source: `tests/process/test_process_high_batch_2a.cpp`:292
- Brief: n/a
- Parameters:
  - `<unnamed>` (VccVpbImporterTest): n/a
  - `<unnamed>` (EXS12_ImportYamlCompliance): n/a

#### `TEST_F(VccVpbImporterTest, EXS13_ImportYamlActivities)`
- Source: `tests/process/test_process_high_batch_2a.cpp`:302
- Brief: n/a
- Parameters:
  - `<unnamed>` (VccVpbImporterTest): n/a
  - `<unnamed>` (EXS13_ImportYamlActivities): n/a

#### `TEST_F(VccVpbImporterTest, EXS14_ImportYamlEdges)`
- Source: `tests/process/test_process_high_batch_2a.cpp`:314
- Brief: n/a
- Parameters:
  - `<unnamed>` (VccVpbImporterTest): n/a
  - `<unnamed>` (EXS14_ImportYamlEdges): n/a

#### `TEST_F(VccVpbImporterTest, EXS15_ImportYamlListBasic)`
- Source: `tests/process/test_process_high_batch_2a.cpp`:324
- Brief: n/a
- Parameters:
  - `<unnamed>` (VccVpbImporterTest): n/a
  - `<unnamed>` (EXS15_ImportYamlListBasic): n/a

#### `TEST_F(VccVpbImporterTest, EXS16_ImportYamlEmpty)`
- Source: `tests/process/test_process_high_batch_2a.cpp`:335
- Brief: n/a
- Parameters:
  - `<unnamed>` (VccVpbImporterTest): n/a
  - `<unnamed>` (EXS16_ImportYamlEmpty): n/a

#### `TEST_F(VccVpbImporterTest, EXS17_ImportYamlMalformed)`
- Source: `tests/process/test_process_high_batch_2a.cpp`:342
- Brief: n/a
- Parameters:
  - `<unnamed>` (VccVpbImporterTest): n/a
  - `<unnamed>` (EXS17_ImportYamlMalformed): n/a

#### `TEST_F(VccVpbImporterTest, EXS18_ImportYamlLargeCompliance)`
- Source: `tests/process/test_process_high_batch_2a.cpp`:351
- Brief: n/a
- Parameters:
  - `<unnamed>` (VccVpbImporterTest): n/a
  - `<unnamed>` (EXS18_ImportYamlLargeCompliance): n/a

#### `TEST_F(VccVpbImporterTest, EXS19_RegexSafetyCompliance)`
- Source: `tests/process/test_process_high_batch_2a.cpp`:366
- Brief: n/a
- Parameters:
  - `<unnamed>` (VccVpbImporterTest): n/a
  - `<unnamed>` (EXS19_RegexSafetyCompliance): n/a

#### `TEST_F(VccVpbImporterTest, EXS20_StringBoundsChecking)`
- Source: `tests/process/test_process_high_batch_2a.cpp`:379
- Brief: n/a
- Parameters:
  - `<unnamed>` (VccVpbImporterTest): n/a
  - `<unnamed>` (EXS20_StringBoundsChecking): n/a

### themis::process::test::ProcessAgenticRagTest

#### `ProcessRagContext createTestContext()`
- Source: `tests/process/test_process_high_batch_2a.cpp`:27
- Brief: n/a
- Parameters: none

### themis::process::test::VccVpbImporterTest

#### `std::string createSimpleYaml()`
- Source: `tests/process/test_process_high_batch_2a.cpp`:253
- Brief: n/a
- Parameters: none

### themisdb::process::FederationConsensusManager

#### `std::string AttemptWrite(const std::string &data, ConsistencyLevel consistency)=0`
- Source: `include/process/process_federation_contract.h`:355
- Brief: n/a
- Parameters:
  - `data` (const std::string &): Operation data (Phase 2 will specify format)
  - `consistency` (ConsistencyLevel): Consistency level (strong, eventual, local)
- Return: Status (success, timeout, partition_detected, failed)
- Details: Attempt write with given consistency level data Operation data (Phase 2 will specify format) consistency Consistency level (strong, eventual, local) Status (success, timeout, partition_detected, failed)

#### `PartitionDetectionResult DetectPartition() const =0`
- Source: `include/process/process_federation_contract.h`:349
- Brief: n/a
- Parameters: none
- Return: Partition detection result with role and reachable nodes
- Details: Detect network partition Partition detection result with role and reachable nodes

#### `FederationConsensusManager(const ProcessFederationConfig &config)`
- Source: `include/process/process_federation_contract.h`:341
- Brief: Create federation consensus manager (Phase 2 implementation).
- Parameters:
  - `config` (const ProcessFederationConfig &): n/a

#### `ConsensusState GetCurrentState() const =0`
- Source: `include/process/process_federation_contract.h`:345
- Brief: n/a
- Parameters: none
- Return: Current state (follower, candidate, leader, failed)
- Details: Get current consensus state Current state (follower, candidate, leader, failed)

#### `ReplicationStatus GetReplicationStatus(const std::string &operation_id) const =0`
- Source: `include/process/process_federation_contract.h`:362
- Brief: n/a
- Parameters:
  - `operation_id` (const std::string &): Unique operation ID
- Return: Replication status (state, replicated_on, quorum_achieved)
- Details: Get replication status for operation operation_id Unique operation ID Replication status (state, replicated_on, quorum_achieved)

#### `~FederationConsensusManager()=default`
- Source: `include/process/process_federation_contract.h`:365
- Brief: n/a
- Parameters: none

### themisdb::process::ProcessAuditLogger

#### `std::uint64_t AppendEntry(AuditTrailEntry &entry)=0`
- Source: `include/process/process_telemetry_contract.h`:222
- Brief: Append entry to immutable audit trail.
- Parameters:
  - `entry` (AuditTrailEntry &): Audit trail entry (will be assigned entry_id)
- Return: Assigned entry ID (monotonically increasing)
- Details: entry Audit trail entry (will be assigned entry_id) Assigned entry ID (monotonically increasing) Single writer (leader); followers call through leader Blocks until CRC32 checksum computed and entry persisted Cannot fail once committed (fail-closed on backend failure)

#### `std::vector< AuditTrailEntry > GetEntriesInRange(std::uint64_t start_id, std::uint64_t end_id) const =0`
- Source: `include/process/process_telemetry_contract.h`:244
- Brief: Retrieve entries in range.
- Parameters:
  - `start_id` (std::uint64_t): Starting entry ID (inclusive)
  - `end_id` (std::uint64_t): Ending entry ID (inclusive)
- Return: Vector of entries in range
- Details: start_id Starting entry ID (inclusive) end_id Ending entry ID (inclusive) Vector of entries in range Multiple concurrent readers allowed

#### `std::optional< AuditTrailEntry > GetEntry(std::uint64_t entry_id) const =0`
- Source: `include/process/process_telemetry_contract.h`:233
- Brief: Retrieve entry from audit trail.
- Parameters:
  - `entry_id` (std::uint64_t): Entry ID to retrieve
- Return: Optional entry (empty if entry_id >= next_entry_id)
- Details: entry_id Entry ID to retrieve Optional entry (empty if entry_id >= next_entry_id) Multiple concurrent readers allowed Returned entry is immutable; modifications create new entries

#### `std::uint64_t GetNextEntryId() const =0`
- Source: `include/process/process_telemetry_contract.h`:275
- Brief: Get current next entry ID.
- Parameters: none
- Return: Entry ID to be assigned to next append
- Details: Entry ID to be assigned to next append

#### `TemporalQueryResult ReconstructAsOfTime(const std::string &model_id, std::chrono::system_clock::time_point timestamp) const =0`
- Source: `include/process/process_telemetry_contract.h`:257
- Brief: Reconstruct model as of given timestamp.
- Parameters:
  - `model_id` (const std::string &): Model ID to reconstruct
  - `timestamp` (std::chrono::system_clock::time_point): Target timestamp for reconstruction
- Return: Temporal query result (reconstructed model, deltas applied)
- Details: model_id Model ID to reconstruct timestamp Target timestamp for reconstruction Temporal query result (reconstructed model, deltas applied) O(log N) snapshot lookup; O(D) delta application (D = deltas since snapshot) Multiple concurrent queries allowed

#### `bool VerifyIntegrity() const =0`
- Source: `include/process/process_telemetry_contract.h`:268
- Brief: Verify audit trail integrity (all CRC32 checksums).
- Parameters: none
- Return: Integrity check result (all_valid, corrupted_entries_count)
- Details: Integrity check result (all_valid, corrupted_entries_count) Bulk verification on recovery; detects offline corruption

#### `~ProcessAuditLogger()=default`
- Source: `include/process/process_telemetry_contract.h`:277
- Brief: n/a
- Parameters: none

### themisdb::process::stress_scenarios

#### `std::vector< std::shared_ptr< FederationStressScenario > > GetAllStressScenarios()`
- Source: `include/process/process_federation_stress_scenarios.h`:495
- Brief: Vector of all 12 stress scenarios for Phase 4 test registration.
- Parameters: none

### themisdb::process::stress_scenarios::AuditTrailHighChurnScenario

#### `std::string Description()`
- Source: `include/process/process_federation_stress_scenarios.h`:225
- Brief: n/a
- Parameters: none

#### `std::string Name()`
- Source: `include/process/process_federation_stress_scenarios.h`:224
- Brief: n/a
- Parameters: none

### themisdb::process::stress_scenarios::ByzantineConsensusScenario

#### `std::string Description()`
- Source: `include/process/process_federation_stress_scenarios.h`:84
- Brief: n/a
- Parameters: none

#### `std::string Name()`
- Source: `include/process/process_federation_stress_scenarios.h`:83
- Brief: n/a
- Parameters: none

### themisdb::process::stress_scenarios::CallbackUnderHighChurnScenario

#### `std::string Description()`
- Source: `include/process/process_federation_stress_scenarios.h`:156
- Brief: n/a
- Parameters: none

#### `std::string Name()`
- Source: `include/process/process_federation_stress_scenarios.h`:155
- Brief: n/a
- Parameters: none

### themisdb::process::stress_scenarios::CascadingFailuresScenario

#### `std::string Description()`
- Source: `include/process/process_federation_stress_scenarios.h`:423
- Brief: n/a
- Parameters: none

#### `std::string Name()`
- Source: `include/process/process_federation_stress_scenarios.h`:422
- Brief: n/a
- Parameters: none

### themisdb::process::stress_scenarios::CorrelationIdPropagationScenario

#### `std::string Description()`
- Source: `include/process/process_federation_stress_scenarios.h`:293
- Brief: n/a
- Parameters: none

#### `std::string Name()`
- Source: `include/process/process_federation_stress_scenarios.h`:292
- Brief: n/a
- Parameters: none

### themisdb::process::stress_scenarios::FederationStressScenario

#### `void Cleanup()=0`
- Source: `include/process/process_federation_stress_scenarios.h`:489
- Brief: Cleanup phase (tear down resources).
- Parameters: none

#### `std::string Description() const =0`
- Source: `include/process/process_federation_stress_scenarios.h`:477
- Brief: Human-readable description.
- Parameters: none

#### `void Execute()=0`
- Source: `include/process/process_federation_stress_scenarios.h`:483
- Brief: Execute phase (run scenario, collect results).
- Parameters: none

#### `std::string Name() const =0`
- Source: `include/process/process_federation_stress_scenarios.h`:474
- Brief: Name of scenario (for test registration).
- Parameters: none

#### `void Setup()=0`
- Source: `include/process/process_federation_stress_scenarios.h`:480
- Brief: Setup phase (initialize cluster, apply initial state).
- Parameters: none

#### `void Verify()=0`
- Source: `include/process/process_federation_stress_scenarios.h`:486
- Brief: Verify phase (check invariants, assertions).
- Parameters: none

#### `~FederationStressScenario()=default`
- Source: `include/process/process_federation_stress_scenarios.h`:491
- Brief: n/a
- Parameters: none

### themisdb::process::stress_scenarios::MultiModelConflictScenario

#### `std::string Description()`
- Source: `include/process/process_federation_stress_scenarios.h`:188
- Brief: n/a
- Parameters: none

#### `std::string Name()`
- Source: `include/process/process_federation_stress_scenarios.h`:187
- Brief: n/a
- Parameters: none

### themisdb::process::stress_scenarios::NetworkPartitionHealingScenario

#### `std::string Description()`
- Source: `include/process/process_federation_stress_scenarios.h`:118
- Brief: n/a
- Parameters: none

#### `std::string Name()`
- Source: `include/process/process_federation_stress_scenarios.h`:117
- Brief: n/a
- Parameters: none

### themisdb::process::stress_scenarios::PartitionDetectionLatencyScenario

#### `std::string Description()`
- Source: `include/process/process_federation_stress_scenarios.h`:361
- Brief: n/a
- Parameters: none

#### `std::string Name()`
- Source: `include/process/process_federation_stress_scenarios.h`:360
- Brief: n/a
- Parameters: none

### themisdb::process::stress_scenarios::RejoinLogSyncScenario

#### `std::string Description()`
- Source: `include/process/process_federation_stress_scenarios.h`:457
- Brief: n/a
- Parameters: none

#### `std::string Name()`
- Source: `include/process/process_federation_stress_scenarios.h`:456
- Brief: n/a
- Parameters: none

### themisdb::process::stress_scenarios::SplitBrainPreventionScenario

#### `std::string Description()`
- Source: `include/process/process_federation_stress_scenarios.h`:392
- Brief: n/a
- Parameters: none

#### `std::string Name()`
- Source: `include/process/process_federation_stress_scenarios.h`:391
- Brief: n/a
- Parameters: none

### themisdb::process::stress_scenarios::TemporalReconstructionScenario

#### `std::string Description()`
- Source: `include/process/process_federation_stress_scenarios.h`:256
- Brief: n/a
- Parameters: none

#### `std::string Name()`
- Source: `include/process/process_federation_stress_scenarios.h`:255
- Brief: n/a
- Parameters: none

### themisdb::process::stress_scenarios::TraceCompletenessPartitionScenario

#### `std::string Description()`
- Source: `include/process/process_federation_stress_scenarios.h`:326
- Brief: n/a
- Parameters: none

#### `std::string Name()`
- Source: `include/process/process_federation_stress_scenarios.h`:325
- Brief: n/a
- Parameters: none

