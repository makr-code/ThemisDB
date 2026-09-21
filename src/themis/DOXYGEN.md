# THEMIS DOXYGEN

Author: ThemisDB Contributors
Created: 2026-09-20
Last Updated: 2026-09-20
Status: active

## Source
- This file is generated from module-scoped Doxygen XML.
- XML index: `C:\Projects\ThemisDB\.tmp\module_doxygen_global_e2e\themis\xml\index.xml`
- Warnings log: `C:\Projects\ThemisDB\.tmp\module_doxygen_global_e2e\themis\doxygen-warnings.log`

## Structure Metrics
- C/C++ Files (scanned): 97
- Compounds: 430
- Classes/Structs: 276
- Namespaces: 35
- File Compounds: 97

## Namespaces
- @071353033122056104231211101161367062131065054063
- @073147234323313306141011235055121214001347146347
- std
- testing
- themis
- themis::acceleration
- themis::bench
- themis::bench::themis_dedicated
- themis::build_info
- themis::build_info::@003323161050252135113032336174166223172010253114
- themis::edition
- themis::engine
- themis::gpu
- themis::index
- themis::license
- themis::llm
- themis::llm::applications
- themis::llm::lora
- themis::modules
- themis::modules::@146121322216365347364367054366210144217016310206
- themis::modules::@316024362102257131166316227253013131231026341216
- themis::modules::BaseErrorTaxonomy
- themis::network
- themis::observability
- themis::query
- themis::rag
- themis::rag::kg
- themis::ratelimit
- themis::search
- themis::sharding
- themis::storage
- themis::tenant
- themis::wire
- themis::wire::@113332333006020050353262221354163056242117301202
- themis::wire::v1

## Types
### Classes
- StubLoader
- StubTrustVerifier
- StubWireServer
- ThemisIntegrationTest
- themis::IExpressionEvaluator
- themis::IFieldEncryption
- themis::IFieldEncryptionFactory
- themis::IGraphIndex
- themis::IIndexManager
- themis::IKeyProvider
- themis::IKeyProviderFactory
- themis::ILlmReranker
- themis::IQueryEngine
- themis::IQueryEngineFactory
- themis::ISecondaryIndex
- themis::IStorageEngine
- themis::IStorageEngineFactory
- themis::IVectorIndex
- themis::edition::EditionManager
- themis::gpu::DeviceDiscovery
- themis::gpu::GPUAdminAPI
- themis::gpu::GPUAlerts
- themis::gpu::GPUAuditLog
- themis::gpu::GPUClusterCoordinator
- themis::gpu::GPUClusterTopology
- themis::gpu::GPUErrorHandler
- themis::gpu::GPUFeatureFlags
- themis::gpu::GPUGraphCache
- themis::gpu::GPUKernelValidator
- themis::gpu::GPULauncher
- themis::gpu::GPULoadBalancer
- themis::gpu::GPUMemoryManager
- themis::gpu::GPUMemoryPool
- themis::gpu::GPUMetrics
- themis::gpu::GPUModule
- themis::gpu::GPUP2PTransferManager
- themis::gpu::GPUPolicy
- themis::gpu::GPUProfiler
- themis::gpu::GPUQueryAccelerator
- themis::gpu::GPUSafeFail
- themis::gpu::GPUStreamManager
- themis::gpu::GPUTensorBuffer
- themis::gpu::GPUTimeSliceScheduler
- themis::gpu::GPUTrainingLoop
- themis::gpu::GPUUnifiedMemoryAllocator
- themis::gpu::IVRAMPolicy
- themis::gpu::KernelSLAGuard
- themis::gpu::MIGManager
- themis::gpu::ROCmBackend
- themis::gpu::ScopedGPURange
- themis::gpu::VulkanComputeBackend
- themis::gpu::WASMKernelSandbox
- themis::gpu::shared_gpu_ptr
- themis::gpu::unique_gpu_ptr
- themis::license::LicenseClient
- themis::license::LicenseClient::Impl
- themis::license::LicenseInfo
- themis::license::RuntimeLicenseGate
- themis::llm::IDocsAssistant
- themis::llm::IEmbeddedLLM
- themis::llm::ILLMModelAuditLogger
- themis::llm::ILLMPluginManager
- themis::llm::ILLMResourcePolicy
- themis::llm::ILlamaWrapper
- themis::llm::IThemisHelpLoRA
- themis::llm::lora::ILoRAOrchestrator
- themis::modules::ABTestManager
- themis::modules::AbiChecker
- themis::modules::HotReloadManager
- themis::modules::IWasmRuntime
- themis::modules::ModuleDependencyResolver
- themis::modules::ModuleHashVerifier
- themis::modules::ModuleLoader
- themis::modules::ModuleRegistry
- themis::modules::ModuleSandbox
- themis::modules::ModuleSecurityVerifier
- themis::modules::ModuleSecurityVerifier::Impl
- themis::modules::ModuleSignatureVerifier
- themis::modules::PluginBundleLoader
- themis::modules::PluginDependencyGraph
- themis::modules::RemoteRegistryClient
- themis::modules::ScopedSpan
- themis::modules::WasmModuleValidator
- themis::modules::WasmPluginSandbox
- themis::modules::WasmRuntime
- themis::modules::WasmRuntimeInjector
- themis::network::IConnectionPolicy
- themis::query::IQueryLimitPolicy
- themis::rag::kg::IKnowledgeGraph
- themis::ratelimit::IRateLimitPolicy
- themis::sharding::IShardLimitPolicy
- themis::storage::IStorageOpsPolicy
- themis::tenant::ITenantQuotaPolicy
- themis::wire::MessageDispatcher
- themis::wire::V2Server
- themis::wire::V2Session
- themis::wire::WireProtocolServer
- themis::wire::WireProtocolSession

### Structs
- LoadResult
- StubTrustVerifier::TrustResult
- themis::GraphEdge
- themis::ILlmReranker::Config
- themis::IStorageEngine::ScanRange
- themis::LlmRerankCandidate
- themis::LlmRerankResult
- themis::QueryResult
- themis::VectorSearchResult
- themis::build_info::BuildConfiguration
- themis::build_info::ModuleInfo
- themis::build_info::ReproducibilityInfo
- themis::edition::EditionInfo
- themis::gpu::ClusterConfig
- themis::gpu::ClusterConfig::NodeEntry
- themis::gpu::ClusterNode
- themis::gpu::DeviceInfo
- themis::gpu::GPUAlerts::AlertStatus
- themis::gpu::GPUAlerts::Config
- themis::gpu::GPUAuditLog::Event
- themis::gpu::GPUClusterCoordinator::ClusterHealth
- themis::gpu::GPUClusterCoordinator::NodeInfo
- themis::gpu::GPUClusterCoordinator::Placement
- themis::gpu::GPUConfig
- themis::gpu::GPUConfig::ValidationResult
- themis::gpu::GPUFeatureFlags::FeatureStatus
- themis::gpu::GPUGraphCache::Stats
- themis::gpu::GPUKernelValidator::Stats
- themis::gpu::GPUKernelValidator::ValidationResult
- themis::gpu::GPULauncher::Stats
- themis::gpu::GPULauncher::WorkItem
- themis::gpu::GPULauncher::WorkResult
- themis::gpu::GPULoadBalancer::DeviceEntry
- themis::gpu::GPULoadBalancer::DeviceLoad
- themis::gpu::GPUMemoryManager::AllocationRecord
- themis::gpu::GPUMemoryManager::HintHandle
- themis::gpu::GPUMemoryManager::HintRecord
- themis::gpu::GPUMemoryManager::Stats
- themis::gpu::GPUMemoryManager::TenantState
- themis::gpu::GPUMemoryManager::TenantStats
- themis::gpu::GPUMemoryPool::DefragResult
- themis::gpu::GPUMemoryPool::Slab
- themis::gpu::GPUMemoryPool::Stats
- themis::gpu::GPUMetrics::KernelRecord
- themis::gpu::GPUMetrics::Sample
- themis::gpu::GPUModule::InitResult
- themis::gpu::GPUModule::SubmitResult
- themis::gpu::GPUP2PTransferManager::PairInfo
- themis::gpu::GPUP2PTransferManager::Stats
- themis::gpu::GPUP2PTransferManager::TransferRequest
- themis::gpu::GPUP2PTransferManager::TransferResult
- themis::gpu::GPUPolicy::PolicyDecision
- themis::gpu::GPUProfiler::ActiveRange
- themis::gpu::GPUProfiler::Range
- themis::gpu::GPUQueryAccelerator::AggResult
- themis::gpu::GPUQueryAccelerator::AnnNeighbor
- themis::gpu::GPUQueryAccelerator::AnnResult
- themis::gpu::GPUQueryAccelerator::Config
- themis::gpu::GPUQueryAccelerator::DotProductResult
- themis::gpu::GPUQueryAccelerator::JoinResult
- themis::gpu::GPUQueryAccelerator::Row
- themis::gpu::GPUQueryAccelerator::ScanResult
- themis::gpu::GPUQueryAccelerator::SortResult
- themis::gpu::GPUQueryAccelerator::Stats
- themis::gpu::GPUQueryAccelerator::TopKResult
- themis::gpu::GPUSafeFail::Config
- themis::gpu::GPUSafeFail::HealthStatus
- themis::gpu::GPUStreamManager::Stream
- themis::gpu::GPUStreamManager::StreamConfig
- themis::gpu::GPUStreamManager::StreamStats
- themis::gpu::GPUTensorBuffer::Shape
- themis::gpu::GPUTensorBuffer::Stats
- themis::gpu::GPUTensorBuffer::View
- themis::gpu::GPUTimeSliceScheduler::Stats
- themis::gpu::GPUTimeSliceScheduler::TenantConfig
- themis::gpu::GPUTimeSliceScheduler::TenantState
- themis::gpu::GPUTimeSliceScheduler::TenantStats
- themis::gpu::GPUTrainingLoop::Config
- themis::gpu::GPUTrainingLoop::EpochStats
- themis::gpu::GPUTrainingLoop::StepRecord
- themis::gpu::GPUUnifiedMemoryAllocator::AllocationRecord
- themis::gpu::GPUUnifiedMemoryAllocator::Stats
- themis::gpu::GraphEntry
- themis::gpu::MIGManager::MIGInstance
- themis::gpu::MIGManager::Stats
- themis::gpu::QueryShape
- themis::gpu::QueryShapeHash
- themis::gpu::ROCmBackend::AllocationRecord
- themis::gpu::ROCmBackend::Result
- themis::gpu::ROCmBackend::Stats
- themis::gpu::ROCmBackend::StreamHandle
- themis::gpu::TopologyLink
- themis::gpu::VulkanComputeBackend::Result
- themis::gpu::VulkanComputeBackend::Stats
- themis::gpu::VulkanComputeBackend::StreamHandle
- themis::gpu::WASMKernelSandbox::ExecutionResult
- themis::gpu::WASMKernelSandbox::SandboxConfig
- themis::gpu::WASMKernelSandbox::Stats
- themis::gpu::shared_gpu_ptr::ControlBlock
- themis::license::GateResult
- themis::license::LicenseActivationResult
- themis::license::LicenseClientConfig
- themis::license::LicenseData
- themis::llm::lora::LoRAJobInfo
- themis::modules::ABModuleTestConfig
- themis::modules::ABModuleTestResult
- themis::modules::ABTestManager::TestEntry
- themis::modules::ABTestManager::VariantData
- themis::modules::ABTestMetricRow
- themis::modules::ABVariantMetrics
- themis::modules::AbiCheckResult
- themis::modules::BaseErrorTaxonomy::BASE_DEPENDENCY_CONFLICT
- themis::modules::BaseErrorTaxonomy::BASE_DEPENDENCY_CYCLE
- themis::modules::BaseErrorTaxonomy::BASE_DEPENDENCY_MISSING_REQUIRED
- themis::modules::BaseErrorTaxonomy::BASE_DEPENDENCY_VERSION_RANGE_MISMATCH
- themis::modules::BaseErrorTaxonomy::BASE_LOADER_ABI_MISMATCH
- themis::modules::BaseErrorTaxonomy::BASE_LOADER_HEALTH_CHECK_FAILED
- themis::modules::BaseErrorTaxonomy::BASE_LOADER_INIT_FAILED
- themis::modules::BaseErrorTaxonomy::BASE_LOADER_LOAD_FAILED
- themis::modules::BaseErrorTaxonomy::BASE_LOADER_PATH_NOT_FOUND
- themis::modules::BaseErrorTaxonomy::BASE_LOADER_SIGNATURE_REJECTED
- themis::modules::BaseErrorTaxonomy::BASE_REGISTRY_AUTH_FAILURE
- themis::modules::BaseErrorTaxonomy::BASE_REGISTRY_CHECKSUM_MISMATCH
- themis::modules::BaseErrorTaxonomy::BASE_REGISTRY_DOWNLOAD_FAILED
- themis::modules::BaseErrorTaxonomy::BASE_REGISTRY_NETWORK_ERROR
- themis::modules::BaseErrorTaxonomy::BASE_RELOAD_CANDIDATE_LOAD_FAILED
- themis::modules::BaseErrorTaxonomy::BASE_RELOAD_NOT_REGISTERED
- themis::modules::BaseErrorTaxonomy::BASE_RELOAD_NO_BACKUP
- themis::modules::BaseErrorTaxonomy::BASE_RELOAD_ROLLBACK_FAILED
- themis::modules::BaseErrorTaxonomy::BASE_RELOAD_STATE_RESTORE_FAILED
- themis::modules::BaseErrorTaxonomy::BASE_SANDBOX_DEGRADED
- themis::modules::BaseErrorTaxonomy::BASE_SANDBOX_INACTIVE_STATS
- themis::modules::BaseErrorTaxonomy::BASE_SANDBOX_LAUNCH_FAILED
- themis::modules::BaseErrorTaxonomy::BASE_SANDBOX_RESOURCE_LIMIT
- themis::modules::BaseErrorTaxonomy::BASE_SANDBOX_TIMEOUT
- themis::modules::DependencyResolutionResult
- themis::modules::HealthCheckResult
- themis::modules::HotReloadManager::Config
- themis::modules::HotReloadManager::ModuleSlot
- themis::modules::HotReloadManager::Stats
- themis::modules::HotReloadResult
- themis::modules::LoadedModule
- themis::modules::ModuleDependency
- themis::modules::ModuleDependencyResolver::ModuleEntry
- themis::modules::ModuleDependencyResolver::RegisteredModuleInfo
- themis::modules::ModuleFailureHistory
- themis::modules::ModuleHashVerificationResult
- themis::modules::ModuleMetadata
- themis::modules::ModuleMetrics
- themis::modules::ModuleSandbox::Config
- themis::modules::ModuleSignatureVerificationResult
- themis::modules::ModuleVerificationResult
- themis::modules::ModuleVersion
- themis::modules::PluginBundleLoadResult
- themis::modules::PluginBundleManifest
- themis::modules::PluginDependencyGraph::Edge
- themis::modules::PluginDependencyGraph::Node
- themis::modules::PluginDownloadResult
- themis::modules::RegistryConfig
- themis::modules::RegistryPluginEntry
- themis::modules::RequestStats
- themis::modules::SandboxStats
- themis::modules::SpanEvent
- themis::modules::TraceContext
- themis::modules::WasmCallResult
- themis::modules::WasmHostFunction
- themis::modules::WasmModuleInfo
- themis::modules::WasmPluginSandbox::Config
- themis::modules::WasmPluginSandbox::Stats
- themis::modules::WasmRuntimeDescriptor
- themis::modules::WatchdogConfig
- themis::modules::WatchdogModuleStats
- themis::wire::V2ConnectionConfig
- themis::wire::V2FrameHeader
- themis::wire::V2Setting
- themis::wire::V2Stream
- themis::wire::WireEngineConfig
- themis::wire::WireFrameHeader

## Notes
- This is a derived artifact. Do not manually edit semantic content.
- Regenerate via scripts/generate_module_doxygen_xml.py --write-module-markdown.

## Detailed Function Documentation (Soll-Ist)
- Functions extracted: 1576

### StubLoader

#### `LoadResult load(const std::string &module_name) noexcept`
- Source: `tests/themis/test_themis_highcardinality_stress.cpp`:47
- Brief: n/a
- Parameters:
  - `module_name` (const std::string &): n/a

#### `uint64_t totalOps() const noexcept`
- Source: `tests/themis/test_themis_highcardinality_stress.cpp`:51
- Brief: n/a
- Parameters: none

### StubTrustVerifier

#### `uint64_t totalOps() const noexcept`
- Source: `tests/themis/test_themis_highcardinality_stress.cpp`:82
- Brief: n/a
- Parameters: none

#### `TrustResult verify(const std::string &module_name, uint8_t claimed_level) noexcept`
- Source: `tests/themis/test_themis_highcardinality_stress.cpp`:75
- Brief: n/a
- Parameters:
  - `module_name` (const std::string &): n/a
  - `claimed_level` (uint8_t): n/a

### StubWireServer

#### `bool handleRequest(uint64_t request_id, const std::string &) noexcept`
- Source: `tests/themis/test_themis_highcardinality_stress.cpp`:58
- Brief: n/a
- Parameters:
  - `request_id` (uint64_t): n/a
  - `<unnamed>` (const std::string &): n/a

#### `uint64_t totalOps() const noexcept`
- Source: `tests/themis/test_themis_highcardinality_stress.cpp`:63
- Brief: n/a
- Parameters: none

### ThemisIntegrationTest

#### `void SetUp() override`
- Source: `tests/themis/test_themis_integration.cpp`:47
- Brief: n/a
- Parameters: none

#### `void TearDown() override`
- Source: `tests/themis/test_themis_integration.cpp`:55
- Brief: n/a
- Parameters: none

#### `LicenseActivationResult makeActivation(bool success, std::string status, int grace_days=0, std::string error_msg="")`
- Source: `tests/themis/test_themis_integration.cpp`:64
- Brief: n/a
- Parameters:
  - `success` (bool): n/a
  - `status` (std::string): n/a
  - `grace_days` (int): n/a
  - `error_msg` (std::string): n/a

#### `LicenseData makeLicense(const std::string &edition_str="ENTERPRISE", const std::string &contact_email="support@example.com")`
- Source: `tests/themis/test_themis_integration.cpp`:77
- Brief: n/a
- Parameters:
  - `edition_str` (const std::string &): n/a
  - `contact_email` (const std::string &): n/a

#### `std::string tmpPath(const std::string &suffix)`
- Source: `tests/themis/test_themis_integration.cpp`:93
- Brief: Returns a unique temporary file path for this test.
- Parameters:
  - `suffix` (const std::string &): n/a

#### `std::string writeTempFile(const std::string &content, const std::string &name)`
- Source: `tests/themis/test_themis_integration.cpp`:98
- Brief: Write content to a temporary file and return the path.
- Parameters:
  - `content` (const std::string &): n/a
  - `name` (const std::string &): n/a

### bench_themis_core.cpp

#### `Iterations(200)`
- Source: `benchmarks/themis/bench_themis_core.cpp`:338
- Brief: n/a
- Parameters:
  - `<unnamed>` (200): n/a

#### `Iterations(2000)`
- Source: `benchmarks/themis/bench_themis_core.cpp`:326
- Brief: n/a
- Parameters:
  - `<unnamed>` (2000): n/a

#### `Iterations(20000)`
- Source: `benchmarks/themis/bench_themis_core.cpp`:330
- Brief: n/a
- Parameters:
  - `<unnamed>` (20000): n/a

#### `Iterations(2000000)`
- Source: `benchmarks/themis/bench_themis_core.cpp`:342
- Brief: n/a
- Parameters:
  - `<unnamed>` (2000000): n/a

#### `Iterations(300)`
- Source: `benchmarks/themis/bench_themis_core.cpp`:322
- Brief: n/a
- Parameters:
  - `<unnamed>` (300): n/a

### bench_themis_dedicated_gates.cpp

#### `BENCHMARK_MAIN()`
- Source: `benchmarks/themis/bench_themis_dedicated_gates.cpp`:139
- Brief: n/a
- Parameters: none

### bench_themis_release_gates.cpp

#### `BENCHMARK(BM_Edition_BatchCast) -> Arg(1000) ->Repetitions(5) ->ReportAggregatesOnly(true)`
- Source: `benchmarks/themis/bench_themis_release_gates.cpp`:72
- Brief: n/a
- Parameters:
  - `<unnamed>` (BM_Edition_BatchCast): n/a

#### `BENCHMARK(BM_Edition_SwitchDispatch) -> Repetitions(5) ->ReportAggregatesOnly(true)`
- Source: `benchmarks/themis/bench_themis_release_gates.cpp`:42
- Brief: n/a
- Parameters:
  - `<unnamed>` (BM_Edition_SwitchDispatch): n/a

#### `BENCHMARK(BM_Edition_ValueLookup) -> Repetitions(5) ->ReportAggregatesOnly(true)`
- Source: `benchmarks/themis/bench_themis_release_gates.cpp`:55
- Brief: n/a
- Parameters:
  - `<unnamed>` (BM_Edition_ValueLookup): n/a

#### `BENCHMARK(BM_ThemisError_Cast) -> Repetitions(5) ->ReportAggregatesOnly(true)`
- Source: `benchmarks/themis/bench_themis_release_gates.cpp`:21
- Brief: n/a
- Parameters:
  - `<unnamed>` (BM_ThemisError_Cast): n/a

#### `BENCHMARK_MAIN()`
- Source: `benchmarks/themis/bench_themis_release_gates.cpp`:74
- Brief: n/a
- Parameters: none

#### `void BM_Edition_BatchCast(benchmark::State &state)`
- Source: `benchmarks/themis/bench_themis_release_gates.cpp`:57
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_Edition_SwitchDispatch(benchmark::State &state)`
- Source: `benchmarks/themis/bench_themis_release_gates.cpp`:23
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_Edition_ValueLookup(benchmark::State &state)`
- Source: `benchmarks/themis/bench_themis_release_gates.cpp`:44
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_ThemisError_Cast(benchmark::State &state)`
- Source: `benchmarks/themis/bench_themis_release_gates.cpp`:10
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

### test_themis_contract_hardening_focused.cpp

#### `TEST(ThemisContractTest, THE01_ErrorCodesUnique)`
- Source: `tests/themis/test_themis_contract_hardening_focused.cpp`:16
- Brief: n/a
- Parameters:
  - `<unnamed>` (ThemisContractTest): n/a
  - `<unnamed>` (THE01_ErrorCodesUnique): n/a

#### `TEST(ThemisContractTest, THE02_ErrorCodesInRange)`
- Source: `tests/themis/test_themis_contract_hardening_focused.cpp`:24
- Brief: n/a
- Parameters:
  - `<unnamed>` (ThemisContractTest): n/a
  - `<unnamed>` (THE02_ErrorCodesInRange): n/a

#### `TEST(ThemisContractTest, THE03_EditionValuesAreDistinct)`
- Source: `tests/themis/test_themis_contract_hardening_focused.cpp`:29
- Brief: n/a
- Parameters:
  - `<unnamed>` (ThemisContractTest): n/a
  - `<unnamed>` (THE03_EditionValuesAreDistinct): n/a

#### `TEST(ThemisContractTest, THE04_MinimalEditionLowest)`
- Source: `tests/themis/test_themis_contract_hardening_focused.cpp`:41
- Brief: n/a
- Parameters:
  - `<unnamed>` (ThemisContractTest): n/a
  - `<unnamed>` (THE04_MinimalEditionLowest): n/a

#### `TEST(ThemisContractTest, THE05_MilitaryEditionHighest)`
- Source: `tests/themis/test_themis_contract_hardening_focused.cpp`:45
- Brief: n/a
- Parameters:
  - `<unnamed>` (ThemisContractTest): n/a
  - `<unnamed>` (THE05_MilitaryEditionHighest): n/a

#### `TEST(ThemisContractTest, THE06_EditionSwitchDispatch)`
- Source: `tests/themis/test_themis_contract_hardening_focused.cpp`:50
- Brief: n/a
- Parameters:
  - `<unnamed>` (ThemisContractTest): n/a
  - `<unnamed>` (THE06_EditionSwitchDispatch): n/a

#### `TEST(ThemisContractTest, THE07_EditionMismatchIsFirstCode)`
- Source: `tests/themis/test_themis_contract_hardening_focused.cpp`:63
- Brief: n/a
- Parameters:
  - `<unnamed>` (ThemisContractTest): n/a
  - `<unnamed>` (THE07_EditionMismatchIsFirstCode): n/a

#### `TEST(ThemisContractTest, THE08_FeatureUnknownIsSecondCode)`
- Source: `tests/themis/test_themis_contract_hardening_focused.cpp`:67
- Brief: n/a
- Parameters:
  - `<unnamed>` (ThemisContractTest): n/a
  - `<unnamed>` (THE08_FeatureUnknownIsSecondCode): n/a

### test_themis_core_grpc_service_bridge.cpp

#### `TEST(ThemisCoreServiceImplBridgeTest, AccessorExceptionThrows)`
- Source: `tests/themis/test_themis_core_grpc_service_bridge.cpp`:47
- Brief: n/a
- Parameters:
  - `<unnamed>` (ThemisCoreServiceImplBridgeTest): n/a
  - `<unnamed>` (AccessorExceptionThrows): n/a

#### `TEST(ThemisCoreServiceImplBridgeTest, InjectedPointerIsReturned)`
- Source: `tests/themis/test_themis_core_grpc_service_bridge.cpp`:35
- Brief: n/a
- Parameters:
  - `<unnamed>` (ThemisCoreServiceImplBridgeTest): n/a
  - `<unnamed>` (InjectedPointerIsReturned): n/a

#### `TEST(ThemisCoreServiceImplBridgeTest, NoAccessorThrows)`
- Source: `tests/themis/test_themis_core_grpc_service_bridge.cpp`:27
- Brief: n/a
- Parameters:
  - `<unnamed>` (ThemisCoreServiceImplBridgeTest): n/a
  - `<unnamed>` (NoAccessorThrows): n/a

#### `TEST(ThemisCoreServiceImplBridgeTest, NullAccessorResultThrows)`
- Source: `tests/themis/test_themis_core_grpc_service_bridge.cpp`:56
- Brief: n/a
- Parameters:
  - `<unnamed>` (ThemisCoreServiceImplBridgeTest): n/a
  - `<unnamed>` (NullAccessorResultThrows): n/a

#### `ThemisCoreServiceImpl makeService()`
- Source: `tests/themis/test_themis_core_grpc_service_bridge.cpp`:21
- Brief: n/a
- Parameters: none

### test_themis_highcardinality_stress.cpp

#### `TEST(ThemisHighCardinalityStress, THSTR01_HighCardinalityModuleLoad)`
- Source: `tests/themis/test_themis_highcardinality_stress.cpp`:90
- Brief: n/a
- Parameters:
  - `<unnamed>` (ThemisHighCardinalityStress): n/a
  - `<unnamed>` (THSTR01_HighCardinalityModuleLoad): n/a

#### `TEST(ThemisHighCardinalityStress, THSTR02_ConcurrentWireSessionStress)`
- Source: `tests/themis/test_themis_highcardinality_stress.cpp`:115
- Brief: n/a
- Parameters:
  - `<unnamed>` (ThemisHighCardinalityStress): n/a
  - `<unnamed>` (THSTR02_ConcurrentWireSessionStress): n/a

#### `TEST(ThemisHighCardinalityStress, THSTR03_TrustEdgeCaseStress)`
- Source: `tests/themis/test_themis_highcardinality_stress.cpp`:149
- Brief: n/a
- Parameters:
  - `<unnamed>` (ThemisHighCardinalityStress): n/a
  - `<unnamed>` (THSTR03_TrustEdgeCaseStress): n/a

### test_themis_integration.cpp

#### `TEST_F(ThemisIntegrationTest, ActiveLicense_GatePasses_EditionReflectsState)`
- Source: `tests/themis/test_themis_integration.cpp`:111
- Brief: n/a
- Parameters:
  - `<unnamed>` (ThemisIntegrationTest): n/a
  - `<unnamed>` (ActiveLicense_GatePasses_EditionReflectsState): n/a

#### `TEST_F(ThemisIntegrationTest, BuildConfig_EditionMatchesEditionHeader)`
- Source: `tests/themis/test_themis_integration.cpp`:399
- Brief: n/a
- Parameters:
  - `<unnamed>` (ThemisIntegrationTest): n/a
  - `<unnamed>` (BuildConfig_EditionMatchesEditionHeader): n/a

#### `TEST_F(ThemisIntegrationTest, BuildConfig_ModuleList_MatchesIsModuleCompiledIn)`
- Source: `tests/themis/test_themis_integration.cpp`:371
- Brief: n/a
- Parameters:
  - `<unnamed>` (ThemisIntegrationTest): n/a
  - `<unnamed>` (BuildConfig_ModuleList_MatchesIsModuleCompiledIn): n/a

#### `TEST_F(ThemisIntegrationTest, BuildManifest_RoundTrip_WithHashVerifier)`
- Source: `tests/themis/test_themis_integration.cpp`:462
- Brief: n/a
- Parameters:
  - `<unnamed>` (ThemisIntegrationTest): n/a
  - `<unnamed>` (BuildManifest_RoundTrip_WithHashVerifier): n/a

#### `TEST_F(ThemisIntegrationTest, ClearAllOverrides_ResetsAllAtOnce)`
- Source: `tests/themis/test_themis_integration.cpp`:338
- Brief: n/a
- Parameters:
  - `<unnamed>` (ThemisIntegrationTest): n/a
  - `<unnamed>` (ClearAllOverrides_ResetsAllAtOnce): n/a

#### `TEST_F(ThemisIntegrationTest, CompileTimeGate_BlocksRegardlessOfLicense)`
- Source: `tests/themis/test_themis_integration.cpp`:168
- Brief: n/a
- Parameters:
  - `<unnamed>` (ThemisIntegrationTest): n/a
  - `<unnamed>` (CompileTimeGate_BlocksRegardlessOfLicense): n/a

#### `TEST_F(ThemisIntegrationTest, ExpiredLicense_GateBlocks_EditionReflects)`
- Source: `tests/themis/test_themis_integration.cpp`:124
- Brief: n/a
- Parameters:
  - `<unnamed>` (ThemisIntegrationTest): n/a
  - `<unnamed>` (ExpiredLicense_GateBlocks_EditionReflects): n/a

#### `TEST_F(ThemisIntegrationTest, FullGateStack_AdminOverrideFalse_WinsOverActiveLicense)`
- Source: `tests/themis/test_themis_integration.cpp`:485
- Brief: n/a
- Parameters:
  - `<unnamed>` (ThemisIntegrationTest): n/a
  - `<unnamed>` (FullGateStack_AdminOverrideFalse_WinsOverActiveLicense): n/a

#### `TEST_F(ThemisIntegrationTest, FullGateStack_ExpiredLicense_ThenAdminOverrideTrue)`
- Source: `tests/themis/test_themis_integration.cpp`:503
- Brief: n/a
- Parameters:
  - `<unnamed>` (ThemisIntegrationTest): n/a
  - `<unnamed>` (FullGateStack_ExpiredLicense_ThenAdminOverrideTrue): n/a

#### `TEST_F(ThemisIntegrationTest, GateAndEditionManager_AreConsistent)`
- Source: `tests/themis/test_themis_integration.cpp`:208
- Brief: n/a
- Parameters:
  - `<unnamed>` (ThemisIntegrationTest): n/a
  - `<unnamed>` (GateAndEditionManager_AreConsistent): n/a

#### `TEST_F(ThemisIntegrationTest, GracePeriodZero_StillGrace_NotExpired)`
- Source: `tests/themis/test_themis_integration.cpp`:246
- Brief: n/a
- Parameters:
  - `<unnamed>` (ThemisIntegrationTest): n/a
  - `<unnamed>` (GracePeriodZero_StillGrace_NotExpired): n/a

#### `TEST_F(ThemisIntegrationTest, GracePeriod_AllowsFeatures_WithWarning)`
- Source: `tests/themis/test_themis_integration.cpp`:229
- Brief: n/a
- Parameters:
  - `<unnamed>` (ThemisIntegrationTest): n/a
  - `<unnamed>` (GracePeriod_AllowsFeatures_WithWarning): n/a

#### `TEST_F(ThemisIntegrationTest, HashVerifier_TamperedFile_DetectedByManifest)`
- Source: `tests/themis/test_themis_integration.cpp`:436
- Brief: n/a
- Parameters:
  - `<unnamed>` (ThemisIntegrationTest): n/a
  - `<unnamed>` (HashVerifier_TamperedFile_DetectedByManifest): n/a

#### `TEST_F(ThemisIntegrationTest, HashVerifier_TempFile_RoundTrip)`
- Source: `tests/themis/test_themis_integration.cpp`:411
- Brief: n/a
- Parameters:
  - `<unnamed>` (ThemisIntegrationTest): n/a
  - `<unnamed>` (HashVerifier_TempFile_RoundTrip): n/a

#### `TEST_F(ThemisIntegrationTest, LicenseWithContactEmail_ErrorContainsEmail)`
- Source: `tests/themis/test_themis_integration.cpp`:277
- Brief: n/a
- Parameters:
  - `<unnamed>` (ThemisIntegrationTest): n/a
  - `<unnamed>` (LicenseWithContactEmail_ErrorContainsEmail): n/a

#### `TEST_F(ThemisIntegrationTest, MultipleInvalidStatuses_AllBlock)`
- Source: `tests/themis/test_themis_integration.cpp`:257
- Brief: n/a
- Parameters:
  - `<unnamed>` (ThemisIntegrationTest): n/a
  - `<unnamed>` (MultipleInvalidStatuses_AllBlock): n/a

#### `TEST_F(ThemisIntegrationTest, OverrideDoesNotPersistAcrossLicenseUpdate)`
- Source: `tests/themis/test_themis_integration.cpp`:321
- Brief: n/a
- Parameters:
  - `<unnamed>` (ThemisIntegrationTest): n/a
  - `<unnamed>` (OverrideDoesNotPersistAcrossLicenseUpdate): n/a

#### `TEST_F(ThemisIntegrationTest, OverrideFalse_BlocksEvenOnActiveLicense)`
- Source: `tests/themis/test_themis_integration.cpp`:352
- Brief: n/a
- Parameters:
  - `<unnamed>` (ThemisIntegrationTest): n/a
  - `<unnamed>` (OverrideFalse_BlocksEvenOnActiveLicense): n/a

#### `TEST_F(ThemisIntegrationTest, OverrideLifecycle_Set_Query_Clear_Requery)`
- Source: `tests/themis/test_themis_integration.cpp`:297
- Brief: n/a
- Parameters:
  - `<unnamed>` (ThemisIntegrationTest): n/a
  - `<unnamed>` (OverrideLifecycle_Set_Query_Clear_Requery): n/a

#### `TEST_F(ThemisIntegrationTest, RuntimeGate_BlocksCompileTimeEnabledFeature_OnExpiry)`
- Source: `tests/themis/test_themis_integration.cpp`:190
- Brief: n/a
- Parameters:
  - `<unnamed>` (ThemisIntegrationTest): n/a
  - `<unnamed>` (RuntimeGate_BlocksCompileTimeEnabledFeature_OnExpiry): n/a

#### `TEST_F(ThemisIntegrationTest, StatusTransition_Active_To_Expired_To_Active)`
- Source: `tests/themis/test_themis_integration.cpp`:141
- Brief: n/a
- Parameters:
  - `<unnamed>` (ThemisIntegrationTest): n/a
  - `<unnamed>` (StatusTransition_Active_To_Expired_To_Active): n/a

#### `TEST_F(ThemisIntegrationTest, TotalFeatureCount_Invariant_AcrossGateChanges)`
- Source: `tests/themis/test_themis_integration.cpp`:534
- Brief: n/a
- Parameters:
  - `<unnamed>` (ThemisIntegrationTest): n/a
  - `<unnamed>` (TotalFeatureCount_Invariant_AcrossGateChanges): n/a

#### `TEST_F(ThemisIntegrationTest, VersionSummary_ContainsEditionFromBuildConfig)`
- Source: `tests/themis/test_themis_integration.cpp`:390
- Brief: n/a
- Parameters:
  - `<unnamed>` (ThemisIntegrationTest): n/a
  - `<unnamed>` (VersionSummary_ContainsEditionFromBuildConfig): n/a

### test_themis_wire_protocol_server.cpp

#### `TEST(DeprecatedWireSessionBridgeTest, NullptrClearIsIdempotent)`
- Source: `tests/themis/test_themis_wire_protocol_server.cpp`:1225
- Brief: n/a
- Parameters:
  - `<unnamed>` (DeprecatedWireSessionBridgeTest): n/a
  - `<unnamed>` (NullptrClearIsIdempotent): n/a

#### `TEST(DeprecatedWireSessionBridgeTest, SetAndClearAqlBridge)`
- Source: `tests/themis/test_themis_wire_protocol_server.cpp`:1172
- Brief: n/a
- Parameters:
  - `<unnamed>` (DeprecatedWireSessionBridgeTest): n/a
  - `<unnamed>` (SetAndClearAqlBridge): n/a

#### `TEST(DeprecatedWireSessionBridgeTest, SetAndClearCursorCloseBridge)`
- Source: `tests/themis/test_themis_wire_protocol_server.cpp`:1191
- Brief: n/a
- Parameters:
  - `<unnamed>` (DeprecatedWireSessionBridgeTest): n/a
  - `<unnamed>` (SetAndClearCursorCloseBridge): n/a

#### `TEST(DeprecatedWireSessionBridgeTest, SetAndClearCursorNextBridge)`
- Source: `tests/themis/test_themis_wire_protocol_server.cpp`:1183
- Brief: n/a
- Parameters:
  - `<unnamed>` (DeprecatedWireSessionBridgeTest): n/a
  - `<unnamed>` (SetAndClearCursorNextBridge): n/a

#### `TEST(DeprecatedWireSessionBridgeTest, SetAndClearGeoQueryBridge)`
- Source: `tests/themis/test_themis_wire_protocol_server.cpp`:1197
- Brief: n/a
- Parameters:
  - `<unnamed>` (DeprecatedWireSessionBridgeTest): n/a
  - `<unnamed>` (SetAndClearGeoQueryBridge): n/a

#### `TEST(DeprecatedWireSessionBridgeTest, SetAndClearGraphTraversalBridge)`
- Source: `tests/themis/test_themis_wire_protocol_server.cpp`:1215
- Brief: n/a
- Parameters:
  - `<unnamed>` (DeprecatedWireSessionBridgeTest): n/a
  - `<unnamed>` (SetAndClearGraphTraversalBridge): n/a

#### `TEST(DeprecatedWireSessionBridgeTest, SetAndClearTSQueryBridge)`
- Source: `tests/themis/test_themis_wire_protocol_server.cpp`:1206
- Brief: n/a
- Parameters:
  - `<unnamed>` (DeprecatedWireSessionBridgeTest): n/a
  - `<unnamed>` (SetAndClearTSQueryBridge): n/a

#### `TEST(MessageDispatcher, HandlerReplacement)`
- Source: `tests/themis/test_themis_wire_protocol_server.cpp`:327
- Brief: n/a
- Parameters:
  - `<unnamed>` (MessageDispatcher): n/a
  - `<unnamed>` (HandlerReplacement): n/a

#### `TEST(MessageDispatcher, MultipleOpcodes)`
- Source: `tests/themis/test_themis_wire_protocol_server.cpp`:353
- Brief: n/a
- Parameters:
  - `<unnamed>` (MessageDispatcher): n/a
  - `<unnamed>` (MultipleOpcodes): n/a

#### `TEST(MessageDispatcher, RegisterAndDispatch)`
- Source: `tests/themis/test_themis_wire_protocol_server.cpp`:297
- Brief: n/a
- Parameters:
  - `<unnamed>` (MessageDispatcher): n/a
  - `<unnamed>` (RegisterAndDispatch): n/a

#### `TEST(MessageDispatcher, UnknownOpcodeDoesNotThrow)`
- Source: `tests/themis/test_themis_wire_protocol_server.cpp`:317
- Brief: n/a
- Parameters:
  - `<unnamed>` (MessageDispatcher): n/a
  - `<unnamed>` (UnknownOpcodeDoesNotThrow): n/a

#### `TEST(MessageFlags, Combinable)`
- Source: `tests/themis/test_themis_wire_protocol_server.cpp`:162
- Brief: n/a
- Parameters:
  - `<unnamed>` (MessageFlags): n/a
  - `<unnamed>` (Combinable): n/a

#### `TEST(MessageFlags, Values)`
- Source: `tests/themis/test_themis_wire_protocol_server.cpp`:155
- Brief: n/a
- Parameters:
  - `<unnamed>` (MessageFlags): n/a
  - `<unnamed>` (Values): n/a

#### `TEST(OpCode, CRUDValues)`
- Source: `tests/themis/test_themis_wire_protocol_server.cpp`:122
- Brief: n/a
- Parameters:
  - `<unnamed>` (OpCode): n/a
  - `<unnamed>` (CRUDValues): n/a

#### `TEST(OpCode, HandshakeValues)`
- Source: `tests/themis/test_themis_wire_protocol_server.cpp`:113
- Brief: n/a
- Parameters:
  - `<unnamed>` (OpCode): n/a
  - `<unnamed>` (HandshakeValues): n/a

#### `TEST(OpCode, QueryValues)`
- Source: `tests/themis/test_themis_wire_protocol_server.cpp`:130
- Brief: n/a
- Parameters:
  - `<unnamed>` (OpCode): n/a
  - `<unnamed>` (QueryValues): n/a

#### `TEST(OpCode, SpecialValues)`
- Source: `tests/themis/test_themis_wire_protocol_server.cpp`:144
- Brief: n/a
- Parameters:
  - `<unnamed>` (OpCode): n/a
  - `<unnamed>` (SpecialValues): n/a

#### `TEST(OpCode, TransactionValues)`
- Source: `tests/themis/test_themis_wire_protocol_server.cpp`:138
- Brief: n/a
- Parameters:
  - `<unnamed>` (OpCode): n/a
  - `<unnamed>` (TransactionValues): n/a

#### `TEST(WireFrameHeader, GetOpcode)`
- Source: `tests/themis/test_themis_wire_protocol_server.cpp`:76
- Brief: n/a
- Parameters:
  - `<unnamed>` (WireFrameHeader): n/a
  - `<unnamed>` (GetOpcode): n/a

#### `TEST(WireFrameHeader, HasFlagCompressed)`
- Source: `tests/themis/test_themis_wire_protocol_server.cpp`:97
- Brief: n/a
- Parameters:
  - `<unnamed>` (WireFrameHeader): n/a
  - `<unnamed>` (HasFlagCompressed): n/a

#### `TEST(WireFrameHeader, HasFlagNone)`
- Source: `tests/themis/test_themis_wire_protocol_server.cpp`:82
- Brief: n/a
- Parameters:
  - `<unnamed>` (WireFrameHeader): n/a
  - `<unnamed>` (HasFlagNone): n/a

#### `TEST(WireFrameHeader, HasFlagSkipChecksum)`
- Source: `tests/themis/test_themis_wire_protocol_server.cpp`:90
- Brief: n/a
- Parameters:
  - `<unnamed>` (WireFrameHeader): n/a
  - `<unnamed>` (HasFlagSkipChecksum): n/a

#### `TEST(WireFrameHeader, InvalidMagic)`
- Source: `tests/themis/test_themis_wire_protocol_server.cpp`:62
- Brief: n/a
- Parameters:
  - `<unnamed>` (WireFrameHeader): n/a
  - `<unnamed>` (InvalidMagic): n/a

#### `TEST(WireFrameHeader, InvalidVersion)`
- Source: `tests/themis/test_themis_wire_protocol_server.cpp`:69
- Brief: n/a
- Parameters:
  - `<unnamed>` (WireFrameHeader): n/a
  - `<unnamed>` (InvalidVersion): n/a

#### `TEST(WireFrameHeader, PackedLayout)`
- Source: `tests/themis/test_themis_wire_protocol_server.cpp`:104
- Brief: n/a
- Parameters:
  - `<unnamed>` (WireFrameHeader): n/a
  - `<unnamed>` (PackedLayout): n/a

#### `TEST(WireFrameHeader, SizeAndAlignment)`
- Source: `tests/themis/test_themis_wire_protocol_server.cpp`:391
- Brief: n/a
- Parameters:
  - `<unnamed>` (WireFrameHeader): n/a
  - `<unnamed>` (SizeAndAlignment): n/a

#### `TEST(WireFrameHeader, ValidHeader)`
- Source: `tests/themis/test_themis_wire_protocol_server.cpp`:55
- Brief: n/a
- Parameters:
  - `<unnamed>` (WireFrameHeader): n/a
  - `<unnamed>` (ValidHeader): n/a

#### `TEST(WireProtocolConstants, ChecksumSizeIsCrc32Width)`
- Source: `tests/themis/test_themis_wire_protocol_server.cpp`:618
- Brief: n/a
- Parameters:
  - `<unnamed>` (WireProtocolConstants): n/a
  - `<unnamed>` (ChecksumSizeIsCrc32Width): n/a

#### `TEST(WireProtocolServer, ConstructDestruct)`
- Source: `tests/themis/test_themis_wire_protocol_server.cpp`:445
- Brief: n/a
- Parameters:
  - `<unnamed>` (WireProtocolServer): n/a
  - `<unnamed>` (ConstructDestruct): n/a

#### `TEST(WireProtocolServer, DoubleStart)`
- Source: `tests/themis/test_themis_wire_protocol_server.cpp`:481
- Brief: n/a
- Parameters:
  - `<unnamed>` (WireProtocolServer): n/a
  - `<unnamed>` (DoubleStart): n/a

#### `TEST(WireProtocolServer, GenericBridgesDoNotSatisfyProtobufBootstrap)`
- Source: `tests/themis/test_themis_wire_protocol_server.cpp`:493
- Brief: n/a
- Parameters:
  - `<unnamed>` (WireProtocolServer): n/a
  - `<unnamed>` (GenericBridgesDoNotSatisfyProtobufBootstrap): n/a

#### `TEST(WireProtocolServer, InitialStatistics)`
- Source: `tests/themis/test_themis_wire_protocol_server.cpp`:453
- Brief: n/a
- Parameters:
  - `<unnamed>` (WireProtocolServer): n/a
  - `<unnamed>` (InitialStatistics): n/a

#### `TEST(WireProtocolServer, Sessions_Pruned_After_Disconnect)`
- Source: `tests/themis/test_themis_wire_protocol_server.cpp`:528
- Brief: n/a
- Parameters:
  - `<unnamed>` (WireProtocolServer): n/a
  - `<unnamed>` (Sessions_Pruned_After_Disconnect): n/a

#### `TEST(WireProtocolServer, SingleThreadedIoContextPrunesSessionsAfterDisconnect)`
- Source: `tests/themis/test_themis_wire_protocol_server.cpp`:566
- Brief: n/a
- Parameters:
  - `<unnamed>` (WireProtocolServer): n/a
  - `<unnamed>` (SingleThreadedIoContextPrunesSessionsAfterDisconnect): n/a

#### `TEST(WireProtocolServer, StartStop)`
- Source: `tests/themis/test_themis_wire_protocol_server.cpp`:462
- Brief: n/a
- Parameters:
  - `<unnamed>` (WireProtocolServer): n/a
  - `<unnamed>` (StartStop): n/a

#### `TEST(WireProtocolServer, StopBeforeStart)`
- Source: `tests/themis/test_themis_wire_protocol_server.cpp`:474
- Brief: n/a
- Parameters:
  - `<unnamed>` (WireProtocolServer): n/a
  - `<unnamed>` (StopBeforeStart): n/a

#### `TEST(WireProtocolSession, CloseIsIdempotent)`
- Source: `tests/themis/test_themis_wire_protocol_server.cpp`:428
- Brief: n/a
- Parameters:
  - `<unnamed>` (WireProtocolSession): n/a
  - `<unnamed>` (CloseIsIdempotent): n/a

#### `TEST(WireProtocolSession, InitialState)`
- Source: `tests/themis/test_themis_wire_protocol_server.cpp`:415
- Brief: n/a
- Parameters:
  - `<unnamed>` (WireProtocolSession): n/a
  - `<unnamed>` (InitialState): n/a

#### `TEST(WireProtocolV1ThemisAuth, AuthFailsWithEmptyUsername)`
- Source: `tests/themis/test_themis_wire_protocol_server.cpp`:646
- Brief: n/a
- Parameters:
  - `<unnamed>` (WireProtocolV1ThemisAuth): n/a
  - `<unnamed>` (AuthFailsWithEmptyUsername): n/a

#### `TEST(WireProtocolV1ThemisAuth, AuthSuccessRequiresNonEmptyUsername)`
- Source: `tests/themis/test_themis_wire_protocol_server.cpp`:639
- Brief: n/a
- Parameters:
  - `<unnamed>` (WireProtocolV1ThemisAuth): n/a
  - `<unnamed>` (AuthSuccessRequiresNonEmptyUsername): n/a

#### `TEST(WireProtocolV1ThemisAuth, NewSessionIsNotAuthenticated)`
- Source: `tests/themis/test_themis_wire_protocol_server.cpp`:631
- Brief: n/a
- Parameters:
  - `<unnamed>` (WireProtocolV1ThemisAuth): n/a
  - `<unnamed>` (NewSessionIsNotAuthenticated): n/a

#### `TEST(WireProtocolV1ThemisAuthCode, UnauthenticatedUsesCode0x0401)`
- Source: `tests/themis/test_themis_wire_protocol_server.cpp`:974
- Brief: n/a
- Parameters:
  - `<unnamed>` (WireProtocolV1ThemisAuthCode): n/a
  - `<unnamed>` (UnauthenticatedUsesCode0x0401): n/a

#### `TEST(WireProtocolV1ThemisBpmn, BlankProcessInstanceIdWouldBeRejected)`
- Source: `tests/themis/test_themis_wire_protocol_server.cpp`:946
- Brief: n/a
- Parameters:
  - `<unnamed>` (WireProtocolV1ThemisBpmn): n/a
  - `<unnamed>` (BlankProcessInstanceIdWouldBeRejected): n/a

#### `TEST(WireProtocolV1ThemisBpmn, BlankProcessKeyWouldBeRejected)`
- Source: `tests/themis/test_themis_wire_protocol_server.cpp`:924
- Brief: n/a
- Parameters:
  - `<unnamed>` (WireProtocolV1ThemisBpmn): n/a
  - `<unnamed>` (BlankProcessKeyWouldBeRejected): n/a

#### `TEST(WireProtocolV1ThemisBpmn, BlankTaskIdWouldBeRejected)`
- Source: `tests/themis/test_themis_wire_protocol_server.cpp`:932
- Brief: n/a
- Parameters:
  - `<unnamed>` (WireProtocolV1ThemisBpmn): n/a
  - `<unnamed>` (BlankTaskIdWouldBeRejected): n/a

#### `TEST(WireProtocolV1ThemisBpmn, EmptyProcessKeyWouldBeRejected)`
- Source: `tests/themis/test_themis_wire_protocol_server.cpp`:912
- Brief: n/a
- Parameters:
  - `<unnamed>` (WireProtocolV1ThemisBpmn): n/a
  - `<unnamed>` (EmptyProcessKeyWouldBeRejected): n/a

#### `TEST(WireProtocolV1ThemisBpmn, NonEmptyProcessKeyPassesValidation)`
- Source: `tests/themis/test_themis_wire_protocol_server.cpp`:918
- Brief: n/a
- Parameters:
  - `<unnamed>` (WireProtocolV1ThemisBpmn): n/a
  - `<unnamed>` (NonEmptyProcessKeyPassesValidation): n/a

#### `TEST(WireProtocolV1ThemisBpmn, QueryInstanceAcceptsBoundedMaxHistoryEvents)`
- Source: `tests/themis/test_themis_wire_protocol_server.cpp`:966
- Brief: n/a
- Parameters:
  - `<unnamed>` (WireProtocolV1ThemisBpmn): n/a
  - `<unnamed>` (QueryInstanceAcceptsBoundedMaxHistoryEvents): n/a

#### `TEST(WireProtocolV1ThemisBpmn, QueryInstanceRejectsNonObjectRequest)`
- Source: `tests/themis/test_themis_wire_protocol_server.cpp`:940
- Brief: n/a
- Parameters:
  - `<unnamed>` (WireProtocolV1ThemisBpmn): n/a
  - `<unnamed>` (QueryInstanceRejectsNonObjectRequest): n/a

#### `TEST(WireProtocolV1ThemisBpmn, QueryInstanceRejectsTooLargeMaxHistoryEvents)`
- Source: `tests/themis/test_themis_wire_protocol_server.cpp`:960
- Brief: n/a
- Parameters:
  - `<unnamed>` (WireProtocolV1ThemisBpmn): n/a
  - `<unnamed>` (QueryInstanceRejectsTooLargeMaxHistoryEvents): n/a

#### `TEST(WireProtocolV1ThemisBpmn, QueryInstanceRejectsZeroMaxHistoryEvents)`
- Source: `tests/themis/test_themis_wire_protocol_server.cpp`:954
- Brief: n/a
- Parameters:
  - `<unnamed>` (WireProtocolV1ThemisBpmn): n/a
  - `<unnamed>` (QueryInstanceRejectsZeroMaxHistoryEvents): n/a

#### `TEST(WireProtocolV1ThemisCursor, BatchSizeOutOfRangeWouldBeRejected)`
- Source: `tests/themis/test_themis_wire_protocol_server.cpp`:882
- Brief: n/a
- Parameters:
  - `<unnamed>` (WireProtocolV1ThemisCursor): n/a
  - `<unnamed>` (BatchSizeOutOfRangeWouldBeRejected): n/a

#### `TEST(WireProtocolV1ThemisCursor, BlankCursorIdWouldBeRejected)`
- Source: `tests/themis/test_themis_wire_protocol_server.cpp`:874
- Brief: n/a
- Parameters:
  - `<unnamed>` (WireProtocolV1ThemisCursor): n/a
  - `<unnamed>` (BlankCursorIdWouldBeRejected): n/a

#### `TEST(WireProtocolV1ThemisCursor, NonObjectRequestWouldBeRejected)`
- Source: `tests/themis/test_themis_wire_protocol_server.cpp`:868
- Brief: n/a
- Parameters:
  - `<unnamed>` (WireProtocolV1ThemisCursor): n/a
  - `<unnamed>` (NonObjectRequestWouldBeRejected): n/a

#### `TEST(WireProtocolV1ThemisDelete, BlankUuidWouldBeRejected)`
- Source: `tests/themis/test_themis_wire_protocol_server.cpp`:737
- Brief: n/a
- Parameters:
  - `<unnamed>` (WireProtocolV1ThemisDelete): n/a
  - `<unnamed>` (BlankUuidWouldBeRejected): n/a

#### `TEST(WireProtocolV1ThemisDelete, EmptyCollectionWouldBeRejected)`
- Source: `tests/themis/test_themis_wire_protocol_server.cpp`:723
- Brief: n/a
- Parameters:
  - `<unnamed>` (WireProtocolV1ThemisDelete): n/a
  - `<unnamed>` (EmptyCollectionWouldBeRejected): n/a

#### `TEST(WireProtocolV1ThemisDelete, EmptyUuidWouldBeRejected)`
- Source: `tests/themis/test_themis_wire_protocol_server.cpp`:730
- Brief: n/a
- Parameters:
  - `<unnamed>` (WireProtocolV1ThemisDelete): n/a
  - `<unnamed>` (EmptyUuidWouldBeRejected): n/a

#### `TEST(WireProtocolV1ThemisErrorCodes, GeoQueryUses501)`
- Source: `tests/themis/test_themis_wire_protocol_server.cpp`:1099
- Brief: n/a
- Parameters:
  - `<unnamed>` (WireProtocolV1ThemisErrorCodes): n/a
  - `<unnamed>` (GeoQueryUses501): n/a

#### `TEST(WireProtocolV1ThemisErrorCodes, QueryAqlUses501)`
- Source: `tests/themis/test_themis_wire_protocol_server.cpp`:1093
- Brief: n/a
- Parameters:
  - `<unnamed>` (WireProtocolV1ThemisErrorCodes): n/a
  - `<unnamed>` (QueryAqlUses501): n/a

#### `TEST(WireProtocolV1ThemisErrorCodes, StorageUnavailableUses503)`
- Source: `tests/themis/test_themis_wire_protocol_server.cpp`:1105
- Brief: n/a
- Parameters:
  - `<unnamed>` (WireProtocolV1ThemisErrorCodes): n/a
  - `<unnamed>` (StorageUnavailableUses503): n/a

#### `TEST(WireProtocolV1ThemisGeoQuery, AuthenticatedValidRequestSends501)`
- Source: `tests/themis/test_themis_wire_protocol_server.cpp`:1060
- Brief: n/a
- Parameters:
  - `<unnamed>` (WireProtocolV1ThemisGeoQuery): n/a
  - `<unnamed>` (AuthenticatedValidRequestSends501): n/a

#### `TEST(WireProtocolV1ThemisGeoQuery, EmptyCollectionWouldBeRejected)`
- Source: `tests/themis/test_themis_wire_protocol_server.cpp`:802
- Brief: n/a
- Parameters:
  - `<unnamed>` (WireProtocolV1ThemisGeoQuery): n/a
  - `<unnamed>` (EmptyCollectionWouldBeRejected): n/a

#### `TEST(WireProtocolV1ThemisGeoQuery, NonEmptyCollectionPassesValidation)`
- Source: `tests/themis/test_themis_wire_protocol_server.cpp`:808
- Brief: n/a
- Parameters:
  - `<unnamed>` (WireProtocolV1ThemisGeoQuery): n/a
  - `<unnamed>` (NonEmptyCollectionPassesValidation): n/a

#### `TEST(WireProtocolV1ThemisGet, AuthenticatedMissingFieldSends400)`
- Source: `tests/themis/test_themis_wire_protocol_server.cpp`:1013
- Brief: n/a
- Parameters:
  - `<unnamed>` (WireProtocolV1ThemisGet): n/a
  - `<unnamed>` (AuthenticatedMissingFieldSends400): n/a

#### `TEST(WireProtocolV1ThemisGet, AuthenticatedValidRequestSends503)`
- Source: `tests/themis/test_themis_wire_protocol_server.cpp`:998
- Brief: n/a
- Parameters:
  - `<unnamed>` (WireProtocolV1ThemisGet): n/a
  - `<unnamed>` (AuthenticatedValidRequestSends503): n/a

#### `TEST(WireProtocolV1ThemisGet, BlankCollectionWouldBeRejected)`
- Source: `tests/themis/test_themis_wire_protocol_server.cpp`:662
- Brief: n/a
- Parameters:
  - `<unnamed>` (WireProtocolV1ThemisGet): n/a
  - `<unnamed>` (BlankCollectionWouldBeRejected): n/a

#### `TEST(WireProtocolV1ThemisGet, EmptyCollectionWouldBeRejected)`
- Source: `tests/themis/test_themis_wire_protocol_server.cpp`:654
- Brief: n/a
- Parameters:
  - `<unnamed>` (WireProtocolV1ThemisGet): n/a
  - `<unnamed>` (EmptyCollectionWouldBeRejected): n/a

#### `TEST(WireProtocolV1ThemisGet, EmptyUuidWouldBeRejected)`
- Source: `tests/themis/test_themis_wire_protocol_server.cpp`:671
- Brief: n/a
- Parameters:
  - `<unnamed>` (WireProtocolV1ThemisGet): n/a
  - `<unnamed>` (EmptyUuidWouldBeRejected): n/a

#### `TEST(WireProtocolV1ThemisGet, UnauthenticatedRequestSends0x0401)`
- Source: `tests/themis/test_themis_wire_protocol_server.cpp`:983
- Brief: n/a
- Parameters:
  - `<unnamed>` (WireProtocolV1ThemisGet): n/a
  - `<unnamed>` (UnauthenticatedRequestSends0x0401): n/a

#### `TEST(WireProtocolV1ThemisGet, ValidInputWouldPassValidation)`
- Source: `tests/themis/test_themis_wire_protocol_server.cpp`:678
- Brief: n/a
- Parameters:
  - `<unnamed>` (WireProtocolV1ThemisGet): n/a
  - `<unnamed>` (ValidInputWouldPassValidation): n/a

#### `TEST(WireProtocolV1ThemisGraph, BlankStartVertexWouldBeRejected)`
- Source: `tests/themis/test_themis_wire_protocol_server.cpp`:857
- Brief: n/a
- Parameters:
  - `<unnamed>` (WireProtocolV1ThemisGraph): n/a
  - `<unnamed>` (BlankStartVertexWouldBeRejected): n/a

#### `TEST(WireProtocolV1ThemisGraph, NonObjectRequestWouldBeRejected)`
- Source: `tests/themis/test_themis_wire_protocol_server.cpp`:851
- Brief: n/a
- Parameters:
  - `<unnamed>` (WireProtocolV1ThemisGraph): n/a
  - `<unnamed>` (NonObjectRequestWouldBeRejected): n/a

#### `TEST(WireProtocolV1ThemisPut, BlankUuidWouldBeRejected)`
- Source: `tests/themis/test_themis_wire_protocol_server.cpp`:703
- Brief: n/a
- Parameters:
  - `<unnamed>` (WireProtocolV1ThemisPut): n/a
  - `<unnamed>` (BlankUuidWouldBeRejected): n/a

#### `TEST(WireProtocolV1ThemisPut, EmptyCollectionWouldBeRejected)`
- Source: `tests/themis/test_themis_wire_protocol_server.cpp`:687
- Brief: n/a
- Parameters:
  - `<unnamed>` (WireProtocolV1ThemisPut): n/a
  - `<unnamed>` (EmptyCollectionWouldBeRejected): n/a

#### `TEST(WireProtocolV1ThemisPut, EmptyEntityWouldBeRejected)`
- Source: `tests/themis/test_themis_wire_protocol_server.cpp`:695
- Brief: n/a
- Parameters:
  - `<unnamed>` (WireProtocolV1ThemisPut): n/a
  - `<unnamed>` (EmptyEntityWouldBeRejected): n/a

#### `TEST(WireProtocolV1ThemisPut, ValidInputWouldPassValidation)`
- Source: `tests/themis/test_themis_wire_protocol_server.cpp`:713
- Brief: n/a
- Parameters:
  - `<unnamed>` (WireProtocolV1ThemisPut): n/a
  - `<unnamed>` (ValidInputWouldPassValidation): n/a

#### `TEST(WireProtocolV1ThemisQuery, AuthenticatedValidRequestSends501)`
- Source: `tests/themis/test_themis_wire_protocol_server.cpp`:1029
- Brief: n/a
- Parameters:
  - `<unnamed>` (WireProtocolV1ThemisQuery): n/a
  - `<unnamed>` (AuthenticatedValidRequestSends501): n/a

#### `TEST(WireProtocolV1ThemisQuery, BlankAqlWouldBeRejected)`
- Source: `tests/themis/test_themis_wire_protocol_server.cpp`:754
- Brief: n/a
- Parameters:
  - `<unnamed>` (WireProtocolV1ThemisQuery): n/a
  - `<unnamed>` (BlankAqlWouldBeRejected): n/a

#### `TEST(WireProtocolV1ThemisQuery, EmptyAqlWouldBeRejected)`
- Source: `tests/themis/test_themis_wire_protocol_server.cpp`:748
- Brief: n/a
- Parameters:
  - `<unnamed>` (WireProtocolV1ThemisQuery): n/a
  - `<unnamed>` (EmptyAqlWouldBeRejected): n/a

#### `TEST(WireProtocolV1ThemisQuery, NonEmptyAqlPassesValidation)`
- Source: `tests/themis/test_themis_wire_protocol_server.cpp`:762
- Brief: n/a
- Parameters:
  - `<unnamed>` (WireProtocolV1ThemisQuery): n/a
  - `<unnamed>` (NonEmptyAqlPassesValidation): n/a

#### `TEST(WireProtocolV1ThemisSanitize, AllControlCharsReplaced)`
- Source: `tests/themis/test_themis_wire_protocol_server.cpp`:1159
- Brief: n/a
- Parameters:
  - `<unnamed>` (WireProtocolV1ThemisSanitize): n/a
  - `<unnamed>` (AllControlCharsReplaced): n/a

#### `TEST(WireProtocolV1ThemisSanitize, DelCharReplacedWithQuestionMark)`
- Source: `tests/themis/test_themis_wire_protocol_server.cpp`:1148
- Brief: n/a
- Parameters:
  - `<unnamed>` (WireProtocolV1ThemisSanitize): n/a
  - `<unnamed>` (DelCharReplacedWithQuestionMark): n/a

#### `TEST(WireProtocolV1ThemisSanitize, EmptyStringUnchanged)`
- Source: `tests/themis/test_themis_wire_protocol_server.cpp`:1155
- Brief: n/a
- Parameters:
  - `<unnamed>` (WireProtocolV1ThemisSanitize): n/a
  - `<unnamed>` (EmptyStringUnchanged): n/a

#### `TEST(WireProtocolV1ThemisSanitize, NewlineReplacedWithQuestionMark)`
- Source: `tests/themis/test_themis_wire_protocol_server.cpp`:1138
- Brief: n/a
- Parameters:
  - `<unnamed>` (WireProtocolV1ThemisSanitize): n/a
  - `<unnamed>` (NewlineReplacedWithQuestionMark): n/a

#### `TEST(WireProtocolV1ThemisSanitize, PrintableStringUnchanged)`
- Source: `tests/themis/test_themis_wire_protocol_server.cpp`:1132
- Brief: n/a
- Parameters:
  - `<unnamed>` (WireProtocolV1ThemisSanitize): n/a
  - `<unnamed>` (PrintableStringUnchanged): n/a

#### `TEST(WireProtocolV1ThemisSanitize, TabReplacedWithQuestionMark)`
- Source: `tests/themis/test_themis_wire_protocol_server.cpp`:1144
- Brief: n/a
- Parameters:
  - `<unnamed>` (WireProtocolV1ThemisSanitize): n/a
  - `<unnamed>` (TabReplacedWithQuestionMark): n/a

#### `TEST(WireProtocolV1ThemisTimeseries, AuthenticatedValidRequestSends503)`
- Source: `tests/themis/test_themis_wire_protocol_server.cpp`:1075
- Brief: n/a
- Parameters:
  - `<unnamed>` (WireProtocolV1ThemisTimeseries): n/a
  - `<unnamed>` (AuthenticatedValidRequestSends503): n/a

#### `TEST(WireProtocolV1ThemisTimeseries, EmptyCollectionWouldBeRejected)`
- Source: `tests/themis/test_themis_wire_protocol_server.cpp`:816
- Brief: n/a
- Parameters:
  - `<unnamed>` (WireProtocolV1ThemisTimeseries): n/a
  - `<unnamed>` (EmptyCollectionWouldBeRejected): n/a

#### `TEST(WireProtocolV1ThemisTimeseries, EqualTimestampsWouldBeRejected)`
- Source: `tests/themis/test_themis_wire_protocol_server.cpp`:833
- Brief: n/a
- Parameters:
  - `<unnamed>` (WireProtocolV1ThemisTimeseries): n/a
  - `<unnamed>` (EqualTimestampsWouldBeRejected): n/a

#### `TEST(WireProtocolV1ThemisTimeseries, InvalidTimeRangeWouldBeRejected)`
- Source: `tests/themis/test_themis_wire_protocol_server.cpp`:824
- Brief: n/a
- Parameters:
  - `<unnamed>` (WireProtocolV1ThemisTimeseries): n/a
  - `<unnamed>` (InvalidTimeRangeWouldBeRejected): n/a

#### `TEST(WireProtocolV1ThemisTimeseries, ValidRequestPassesValidation)`
- Source: `tests/themis/test_themis_wire_protocol_server.cpp`:841
- Brief: n/a
- Parameters:
  - `<unnamed>` (WireProtocolV1ThemisTimeseries): n/a
  - `<unnamed>` (ValidRequestPassesValidation): n/a

#### `TEST(WireProtocolV1ThemisTransaction, BlankTransactionIdWouldBeRejected)`
- Source: `tests/themis/test_themis_wire_protocol_server.cpp`:896
- Brief: n/a
- Parameters:
  - `<unnamed>` (WireProtocolV1ThemisTransaction): n/a
  - `<unnamed>` (BlankTransactionIdWouldBeRejected): n/a

#### `TEST(WireProtocolV1ThemisTransaction, NonObjectBeginRequestWouldBeRejected)`
- Source: `tests/themis/test_themis_wire_protocol_server.cpp`:890
- Brief: n/a
- Parameters:
  - `<unnamed>` (WireProtocolV1ThemisTransaction): n/a
  - `<unnamed>` (NonObjectBeginRequestWouldBeRejected): n/a

#### `TEST(WireProtocolV1ThemisTransaction, TimeoutOutOfRangeWouldBeRejected)`
- Source: `tests/themis/test_themis_wire_protocol_server.cpp`:904
- Brief: n/a
- Parameters:
  - `<unnamed>` (WireProtocolV1ThemisTransaction): n/a
  - `<unnamed>` (TimeoutOutOfRangeWouldBeRejected): n/a

#### `TEST(WireProtocolV1ThemisVectorSearch, AuthenticatedValidRequestSends503)`
- Source: `tests/themis/test_themis_wire_protocol_server.cpp`:1044
- Brief: n/a
- Parameters:
  - `<unnamed>` (WireProtocolV1ThemisVectorSearch): n/a
  - `<unnamed>` (AuthenticatedValidRequestSends503): n/a

#### `TEST(WireProtocolV1ThemisVectorSearch, BlankCollectionWouldBeRejected)`
- Source: `tests/themis/test_themis_wire_protocol_server.cpp`:777
- Brief: n/a
- Parameters:
  - `<unnamed>` (WireProtocolV1ThemisVectorSearch): n/a
  - `<unnamed>` (BlankCollectionWouldBeRejected): n/a

#### `TEST(WireProtocolV1ThemisVectorSearch, EmptyCollectionWouldBeRejected)`
- Source: `tests/themis/test_themis_wire_protocol_server.cpp`:770
- Brief: n/a
- Parameters:
  - `<unnamed>` (WireProtocolV1ThemisVectorSearch): n/a
  - `<unnamed>` (EmptyCollectionWouldBeRejected): n/a

#### `TEST(WireProtocolV1ThemisVectorSearch, EmptyVectorWouldBeRejected)`
- Source: `tests/themis/test_themis_wire_protocol_server.cpp`:786
- Brief: n/a
- Parameters:
  - `<unnamed>` (WireProtocolV1ThemisVectorSearch): n/a
  - `<unnamed>` (EmptyVectorWouldBeRejected): n/a

#### `TEST(WireProtocolV1ThemisVectorSearch, ValidRequestPassesValidation)`
- Source: `tests/themis/test_themis_wire_protocol_server.cpp`:793
- Brief: n/a
- Parameters:
  - `<unnamed>` (WireProtocolV1ThemisVectorSearch): n/a
  - `<unnamed>` (ValidRequestPassesValidation): n/a

#### `TEST(WireV1Constants, ChecksumSize)`
- Source: `tests/themis/test_themis_wire_protocol_server.cpp`:43
- Brief: n/a
- Parameters:
  - `<unnamed>` (WireV1Constants): n/a
  - `<unnamed>` (ChecksumSize): n/a

#### `TEST(WireV1Constants, HeaderSize)`
- Source: `tests/themis/test_themis_wire_protocol_server.cpp`:38
- Brief: n/a
- Parameters:
  - `<unnamed>` (WireV1Constants): n/a
  - `<unnamed>` (HeaderSize): n/a

#### `TEST(WireV1Constants, MagicValue)`
- Source: `tests/themis/test_themis_wire_protocol_server.cpp`:29
- Brief: n/a
- Parameters:
  - `<unnamed>` (WireV1Constants): n/a
  - `<unnamed>` (MagicValue): n/a

#### `TEST(WireV1Constants, MaxPayloadSize)`
- Source: `tests/themis/test_themis_wire_protocol_server.cpp`:47
- Brief: n/a
- Parameters:
  - `<unnamed>` (WireV1Constants): n/a
  - `<unnamed>` (MaxPayloadSize): n/a

#### `TEST(WireV1Constants, Version)`
- Source: `tests/themis/test_themis_wire_protocol_server.cpp`:34
- Brief: n/a
- Parameters:
  - `<unnamed>` (WireV1Constants): n/a
  - `<unnamed>` (Version): n/a

### themis::GraphEdge

#### `GraphEdge(std::string from, std::string to, std::string type="", double w=1.0)`
- Source: `include/themis/base/interfaces/index_interface.h`:181
- Brief: n/a
- Parameters:
  - `from` (std::string): n/a
  - `to` (std::string): n/a
  - `type` (std::string): n/a
  - `w` (double): n/a

### themis::IExpressionEvaluator

#### `bool evaluate(const std::string &expression, const void *context) const =0`
- Source: `include/themis/base/interfaces/query_interface.h`:53
- Brief: Evaluate an expression against provided context.
- Parameters:
  - `expression` (const std::string &): The filter expression to evaluate
  - `context` (const void *): Opaque context pointer (e.g., document, row data)
- Return: true if expression evaluates to true, false otherwise
- Details: expression The filter expression to evaluate context Opaque context pointer (e.g., document, row data) true if expression evaluates to true, false otherwise

#### `std::string get_expression_type() const =0`
- Source: `include/themis/base/interfaces/query_interface.h`:60
- Brief: Get the type of expression language supported.
- Parameters: none
- Return: String identifying the expression language (e.g., "AQL", "SQL")
- Details: String identifying the expression language (e.g., "AQL", "SQL")

#### `~IExpressionEvaluator()=default`
- Source: `include/themis/base/interfaces/query_interface.h`:44
- Brief: n/a
- Parameters: none

### themis::IFieldEncryption

#### `std::vector< uint8_t > decrypt_field(const std::string &field_name, const std::vector< uint8_t > &ciphertext)=0`
- Source: `include/themis/base/interfaces/security_interface.h`:49
- Brief: Decrypt a field value.
- Parameters:
  - `field_name` (const std::string &): Name of the field being decrypted
  - `ciphertext` (const std::vector< uint8_t > &): The encrypted data to decrypt
- Return: Decrypted plaintext as byte vector
- Details: field_name Name of the field being decrypted ciphertext The encrypted data to decrypt Decrypted plaintext as byte vector

#### `std::vector< uint8_t > encrypt_field(const std::string &field_name, const std::vector< uint8_t > &plaintext)=0`
- Source: `include/themis/base/interfaces/security_interface.h`:38
- Brief: Encrypt a field value.
- Parameters:
  - `field_name` (const std::string &): Name of the field being encrypted
  - `plaintext` (const std::vector< uint8_t > &): The plaintext data to encrypt
- Return: Encrypted data as byte vector
- Details: field_name Name of the field being encrypted plaintext The plaintext data to encrypt Encrypted data as byte vector

#### `bool should_encrypt(const std::string &field_name) const =0`
- Source: `include/themis/base/interfaces/security_interface.h`:59
- Brief: Check if a field should be encrypted.
- Parameters:
  - `field_name` (const std::string &): Name of the field to check
- Return: true if field should be encrypted, false otherwise
- Details: field_name Name of the field to check true if field should be encrypted, false otherwise

#### `~IFieldEncryption()=default`
- Source: `include/themis/base/interfaces/security_interface.h`:29
- Brief: n/a
- Parameters: none

### themis::IFieldEncryptionFactory

#### `IFieldEncryptionPtr create()=0`
- Source: `include/themis/base/interfaces/security_interface.h`:107
- Brief: Create a field encryption instance.
- Parameters: none
- Return: Shared pointer to field encryption
- Details: Shared pointer to field encryption

#### `~IFieldEncryptionFactory()=default`
- Source: `include/themis/base/interfaces/security_interface.h`:100
- Brief: n/a
- Parameters: none

### themis::IGraphIndex

#### `std::vector< std::string > findShortestPath(std::string_view from_node, std::string_view to_node, std::string_view edge_type="", uint32_t max_depth=0) const =0`
- Source: `include/themis/base/interfaces/index_interface.h`:233
- Brief: Find shortest path between two nodes.
- Parameters:
  - `from_node` (std::string_view): Source node
  - `to_node` (std::string_view): Target node
  - `edge_type` (std::string_view): Optional edge type filter
  - `max_depth` (uint32_t): Maximum search depth (0 = unlimited)
- Return: Path as vector of node IDs, empty if no path exists
- Details: from_node Source node to_node Target node edge_type Optional edge type filter max_depth Maximum search depth (0 = unlimited) Path as vector of node IDs, empty if no path exists

#### `std::vector< GraphEdge > getIncomingEdges(std::string_view node_id, std::string_view edge_type="") const =0`
- Source: `include/themis/base/interfaces/index_interface.h`:223
- Brief: Get incoming edges to a node.
- Parameters:
  - `node_id` (std::string_view): Node ID
  - `edge_type` (std::string_view): Optional edge type filter
- Return: Vector of incoming edges
- Details: node_id Node ID edge_type Optional edge type filter Vector of incoming edges

#### `std::string getName() const =0`
- Source: `include/themis/base/interfaces/index_interface.h`:241
- Brief: Get index name.
- Parameters: none
- Return: Index name/identifier
- Details: Index name/identifier

#### `std::vector< GraphEdge > getOutgoingEdges(std::string_view node_id, std::string_view edge_type="") const =0`
- Source: `include/themis/base/interfaces/index_interface.h`:215
- Brief: Get outgoing edges from a node.
- Parameters:
  - `node_id` (std::string_view): Node ID
  - `edge_type` (std::string_view): Optional edge type filter
- Return: Vector of outgoing edges
- Details: node_id Node ID edge_type Optional edge type filter Vector of outgoing edges

#### `std::string getStatistics() const =0`
- Source: `include/themis/base/interfaces/index_interface.h`:245
- Brief: Get index statistics.
- Parameters: none
- Return: JSON string with statistics (nodes, edges, etc.)
- Details: JSON string with statistics (nodes, edges, etc.)

#### `bool insertEdge(const GraphEdge &edge)=0`
- Source: `include/themis/base/interfaces/index_interface.h`:200
- Brief: Insert or update an edge.
- Parameters:
  - `edge` (const GraphEdge &): The edge to insert
- Return: true on success, false on failure
- Details: edge The edge to insert true on success, false on failure

#### `bool removeEdge(std::string_view from_node, std::string_view to_node, std::string_view edge_type="")=0`
- Source: `include/themis/base/interfaces/index_interface.h`:207
- Brief: Remove an edge.
- Parameters:
  - `from_node` (std::string_view): Source node ID
  - `to_node` (std::string_view): Target node ID
  - `edge_type` (std::string_view): Edge type (empty = all types)
- Return: true if edge existed and was removed, false otherwise
- Details: from_node Source node ID to_node Target node ID edge_type Edge type (empty = all types) true if edge existed and was removed, false otherwise

#### `~IGraphIndex()=default`
- Source: `include/themis/base/interfaces/index_interface.h`:195
- Brief: n/a
- Parameters: none

### themis::IIndexManager

#### `Result< IGraphIndex * > createGraphIndex(std::string_view name, const std::string &config="")=0`
- Source: `include/themis/base/interfaces/index_interface.h`:289
- Brief: Create a new graph index.
- Parameters:
  - `name` (std::string_view): Index name
  - `config` (const std::string &): Implementation-specific configuration
- Return: Result containing pointer to created index, or Error on failure Possible errors: ERR_INDEX_NOT_INITIALIZED, ERR_INDEX_CREATION_FAILED
- Details: name Index name config Implementation-specific configuration Result containing pointer to created index, or Error on failure Possible errors: ERR_INDEX_NOT_INITIALIZED, ERR_INDEX_CREATION_FAILED

#### `Result< ISecondaryIndex * > createSecondaryIndex(std::string_view name, std::string_view field_name, const std::string &config="")=0`
- Source: `include/themis/base/interfaces/index_interface.h`:268
- Brief: Create a new secondary index.
- Parameters:
  - `name` (std::string_view): Index name
  - `field_name` (std::string_view): Field to index
  - `config` (const std::string &): Implementation-specific configuration
- Return: Result containing pointer to created index, or Error on failure Possible errors: ERR_INDEX_NOT_INITIALIZED, ERR_INDEX_CREATION_FAILED, ERR_API_INVALID_REQUEST
- Details: name Index name field_name Field to index config Implementation-specific configuration Result containing pointer to created index, or Error on failure Possible errors: ERR_INDEX_NOT_INITIALIZED, ERR_INDEX_CREATION_FAILED, ERR_API_INVALID_REQUEST

#### `Result< IVectorIndex * > createVectorIndex(std::string_view name, uint32_t dimension, const std::string &config="")=0`
- Source: `include/themis/base/interfaces/index_interface.h`:279
- Brief: Create a new vector index.
- Parameters:
  - `name` (std::string_view): Index name
  - `dimension` (uint32_t): Vector dimension
  - `config` (const std::string &): Implementation-specific configuration
- Return: Result containing pointer to created index, or Error on failure Possible errors: ERR_INDEX_NOT_INITIALIZED, ERR_INDEX_CREATION_FAILED, ERR_API_INVALID_REQUEST
- Details: name Index name dimension Vector dimension config Implementation-specific configuration Result containing pointer to created index, or Error on failure Possible errors: ERR_INDEX_NOT_INITIALIZED, ERR_INDEX_CREATION_FAILED, ERR_API_INVALID_REQUEST

#### `Result< void > dropIndex(std::string_view name)=0`
- Source: `include/themis/base/interfaces/index_interface.h`:315
- Brief: Drop an index by name.
- Parameters:
  - `name` (std::string_view): Index name
- Return: Result<void> indicating success or error Possible errors: ERR_INDEX_NOT_FOUND
- Details: name Index name Result<void> indicating success or error Possible errors: ERR_INDEX_NOT_FOUND

#### `Result< IGraphIndex * > getGraphIndex(std::string_view name) const =0`
- Source: `include/themis/base/interfaces/index_interface.h`:309
- Brief: Get an existing graph index by name.
- Parameters:
  - `name` (std::string_view): Index name
- Return: Result containing pointer to index, or Error if not found Possible errors: ERR_INDEX_NOT_FOUND, ERR_INDEX_INVALID_TYPE
- Details: name Index name Result containing pointer to index, or Error if not found Possible errors: ERR_INDEX_NOT_FOUND, ERR_INDEX_INVALID_TYPE

#### `Result< IndexType > getIndexType(std::string_view name) const =0`
- Source: `include/themis/base/interfaces/index_interface.h`:324
- Brief: Get type of an index.
- Parameters:
  - `name` (std::string_view): Index name
- Return: Result<IndexType> with index type, or ERR_INDEX_NOT_FOUND if index doesn't exist
- Details: name Index name Result<IndexType> with index type, or ERR_INDEX_NOT_FOUND if index doesn't exist

#### `Result< ISecondaryIndex * > getSecondaryIndex(std::string_view name) const =0`
- Source: `include/themis/base/interfaces/index_interface.h`:297
- Brief: Get an existing secondary index by name.
- Parameters:
  - `name` (std::string_view): Index name
- Return: Result containing pointer to index, or Error if not found Possible errors: ERR_INDEX_NOT_FOUND, ERR_INDEX_INVALID_TYPE
- Details: name Index name Result containing pointer to index, or Error if not found Possible errors: ERR_INDEX_NOT_FOUND, ERR_INDEX_INVALID_TYPE

#### `Result< IVectorIndex * > getVectorIndex(std::string_view name) const =0`
- Source: `include/themis/base/interfaces/index_interface.h`:303
- Brief: Get an existing vector index by name.
- Parameters:
  - `name` (std::string_view): Index name
- Return: Result containing pointer to index, or Error if not found Possible errors: ERR_INDEX_NOT_FOUND, ERR_INDEX_INVALID_TYPE
- Details: name Index name Result containing pointer to index, or Error if not found Possible errors: ERR_INDEX_NOT_FOUND, ERR_INDEX_INVALID_TYPE

#### `std::vector< std::string > listIndexes() const =0`
- Source: `include/themis/base/interfaces/index_interface.h`:319
- Brief: List all indexes.
- Parameters: none
- Return: Vector of index names
- Details: Vector of index names

#### `~IIndexManager()=default`
- Source: `include/themis/base/interfaces/index_interface.h`:260
- Brief: n/a
- Parameters: none

### themis::IKeyProvider

#### `std::vector< uint8_t > get_key(const std::string &key_id)=0`
- Source: `include/themis/base/interfaces/security_interface.h`:81
- Brief: Get an encryption key by identifier.
- Parameters:
  - `key_id` (const std::string &): Logical key identifier (e.g., "user_pii", "payment_info")
- Return: The encryption key as byte vector
- Details: key_id Logical key identifier (e.g., "user_pii", "payment_info") The encryption key as byte vector

#### `std::vector< uint8_t > rotate_key(const std::string &key_id)=0`
- Source: `include/themis/base/interfaces/security_interface.h`:89
- Brief: Rotate a key to a new version.
- Parameters:
  - `key_id` (const std::string &): Logical key identifier
- Return: The new encryption key as byte vector
- Details: key_id Logical key identifier The new encryption key as byte vector

#### `~IKeyProvider()=default`
- Source: `include/themis/base/interfaces/security_interface.h`:73
- Brief: n/a
- Parameters: none

### themis::IKeyProviderFactory

#### `IKeyProviderPtr create()=0`
- Source: `include/themis/base/interfaces/security_interface.h`:122
- Brief: Create a key provider instance.
- Parameters: none
- Return: Shared pointer to key provider
- Details: Shared pointer to key provider

#### `~IKeyProviderFactory()=default`
- Source: `include/themis/base/interfaces/security_interface.h`:115
- Brief: n/a
- Parameters: none

### themis::ILlmReranker

#### `const Config & getConfig() const =0`
- Source: `include/themis/search/llm_reranker_interface.h`:30
- Brief: n/a
- Parameters: none

#### `std::vector< LlmRerankResult > rerank(const std::string &query, const std::vector< LlmRerankCandidate > &candidates) const =0`
- Source: `include/themis/search/llm_reranker_interface.h`:28
- Brief: n/a
- Parameters:
  - `query` (const std::string &): n/a
  - `candidates` (const std::vector< LlmRerankCandidate > &): n/a

#### `void setBackend(LlmBackend backend)=0`
- Source: `include/themis/search/llm_reranker_interface.h`:27
- Brief: n/a
- Parameters:
  - `backend` (LlmBackend): n/a

#### `~ILlmReranker()=default`
- Source: `include/themis/search/llm_reranker_interface.h`:26
- Brief: n/a
- Parameters: none

### themis::ILlmReranker::Config

#### `Config defaults()`
- Source: `include/themis/search/llm_reranker_interface.h`:23
- Brief: n/a
- Parameters: none

### themis::IQueryEngine

#### `Result< std::unique_ptr< IExpressionEvaluator > > createExpressionEvaluator() const =0`
- Source: `include/themis/base/interfaces/query_interface.h`:126
- Brief: Create an expression evaluator.
- Parameters: none
- Return: Result<std::unique_ptr<IExpressionEvaluator>> with evaluator or error
- Details: Result<std::unique_ptr<IExpressionEvaluator>> with evaluator or error

#### `Result< std::string > execute(const std::string &query)=0`
- Source: `include/themis/base/interfaces/query_interface.h`:85
- Brief: Execute a query.
- Parameters:
  - `query` (const std::string &): Query string
- Return: Result<std::string> with query output or error details
- Details: query Query string Result<std::string> with query output or error details Legacy signature returning QueryResult is deprecated. Implementations should migrate to Result<std::string>.

#### `Result< std::string > execute(const std::string &query, const std::string &bind_vars_json)`
- Source: `include/themis/base/interfaces/query_interface.h`:102
- Brief: Execute a query with named bind variables.
- Parameters:
  - `query` (const std::string &): AQL query string.
  - `bind_vars_json` (const std::string &): Named bind variables as a JSON object string (e.g. {"@col":"users","limit":10}). Pass "{}" or an empty string when there are no bind variables.
- Return: JSON-encoded result string, or error.
- Details: query AQL query string. bind_vars_json Named bind variables as a JSON object string (e.g. {"@col":"users","limit":10}). Pass "{}" or an empty string when there are no bind variables. JSON-encoded result string, or error. The default implementation calls the single-argument execute() overload and emits a warning when non-empty bind variables are supplied, because they cannot be forwarded. Engine implementations that natively support bind variables should override this method.

#### `Result< std::string > explainQuery(const std::string &query) const =0`
- Source: `include/themis/base/interfaces/query_interface.h`:134
- Brief: Generate execution plan explanation.
- Parameters:
  - `query` (const std::string &): Query string
- Return: Result<std::string> with execution plan or error
- Details: query Query string Result<std::string> with execution plan or error

#### `Result< void > validate(const std::string &query) const =0`
- Source: `include/themis/base/interfaces/query_interface.h`:119
- Brief: Validate a query without executing it.
- Parameters:
  - `query` (const std::string &): Query string
- Return: Result<void> - success if valid, error with details otherwise
- Details: query Query string Result<void> - success if valid, error with details otherwise

#### `~IQueryEngine()=default`
- Source: `include/themis/base/interfaces/query_interface.h`:74
- Brief: n/a
- Parameters: none

### themis::IQueryEngineFactory

#### `IQueryEnginePtr create()=0`
- Source: `include/themis/base/interfaces/query_interface.h`:152
- Brief: Create a query engine instance.
- Parameters: none
- Return: Shared pointer to query engine
- Details: Shared pointer to query engine

#### `~IQueryEngineFactory()=default`
- Source: `include/themis/base/interfaces/query_interface.h`:145
- Brief: n/a
- Parameters: none

### themis::ISecondaryIndex

#### `std::string getFieldName() const =0`
- Source: `include/themis/base/interfaces/index_interface.h`:105
- Brief: Get indexed field name.
- Parameters: none
- Return: Field name that this index covers
- Details: Field name that this index covers

#### `std::string getName() const =0`
- Source: `include/themis/base/interfaces/index_interface.h`:101
- Brief: Get index name.
- Parameters: none
- Return: Index name/identifier
- Details: Index name/identifier

#### `std::string getStatistics() const =0`
- Source: `include/themis/base/interfaces/index_interface.h`:109
- Brief: Get index statistics.
- Parameters: none
- Return: JSON string with statistics (size, cardinality, etc.)
- Details: JSON string with statistics (size, cardinality, etc.)

#### `bool insert(std::string_view indexed_value, std::string_view primary_key)=0`
- Source: `include/themis/base/interfaces/index_interface.h`:74
- Brief: Insert or update an index entry.
- Parameters:
  - `indexed_value` (std::string_view): The value being indexed
  - `primary_key` (std::string_view): The primary key of the document/row
- Return: true on success, false on failure
- Details: indexed_value The value being indexed primary_key The primary key of the document/row true on success, false on failure

#### `std::vector< std::string > lookup(std::string_view value) const =0`
- Source: `include/themis/base/interfaces/index_interface.h`:87
- Brief: Lookup by exact value.
- Parameters:
  - `value` (std::string_view): The value to look up
- Return: Vector of matching primary keys
- Details: value The value to look up Vector of matching primary keys

#### `std::vector< std::string > rangeScan(std::string_view start_value, std::string_view end_value, ScanOrder order=ScanOrder::ASCENDING) const =0`
- Source: `include/themis/base/interfaces/index_interface.h`:94
- Brief: Range scan [start_value, end_value).
- Parameters:
  - `start_value` (std::string_view): Start of range (inclusive)
  - `end_value` (std::string_view): End of range (exclusive)
  - `order` (ScanOrder): Scan order
- Return: Vector of matching primary keys in requested order
- Details: start_value Start of range (inclusive) end_value End of range (exclusive) order Scan order Vector of matching primary keys in requested order

#### `bool remove(std::string_view indexed_value, std::string_view primary_key)=0`
- Source: `include/themis/base/interfaces/index_interface.h`:81
- Brief: Remove an index entry.
- Parameters:
  - `indexed_value` (std::string_view): The indexed value
  - `primary_key` (std::string_view): The primary key
- Return: true if entry existed and was removed, false otherwise
- Details: indexed_value The indexed value primary_key The primary key true if entry existed and was removed, false otherwise

#### `~ISecondaryIndex()=default`
- Source: `include/themis/base/interfaces/index_interface.h`:68
- Brief: n/a
- Parameters: none

### themis::IStorageEngine

#### `void close()=0`
- Source: `include/themis/base/interfaces/storage_interface.h`:51
- Brief: Close the storage engine.
- Parameters: none

#### `Result< void > del(const std::string &key)=0`
- Source: `include/themis/base/interfaces/storage_interface.h`:76
- Brief: Delete a key-value pair.
- Parameters:
  - `key` (const std::string &): The key to delete
- Return: Result<void> - success or error with details
- Details: key The key to delete Result<void> - success or error with details

#### `Result< std::string > get(const std::string &key)=0`
- Source: `include/themis/base/interfaces/storage_interface.h`:68
- Brief: Get a value by key.
- Parameters:
  - `key` (const std::string &): The key
- Return: Result<std::string> - The value if found, or error with details
- Details: key The key Result<std::string> - The value if found, or error with details

#### `Result< void > open(const std::string &db_path)=0`
- Source: `include/themis/base/interfaces/storage_interface.h`:46
- Brief: Open/initialize the storage engine.
- Parameters:
  - `db_path` (const std::string &): Path to database directory
- Return: Result<void> - success or error with details
- Details: db_path Path to database directory Result<void> - success or error with details

#### `Result< void > put(const std::string &key, const std::string &value)=0`
- Source: `include/themis/base/interfaces/storage_interface.h`:60
- Brief: Put a key-value pair.
- Parameters:
  - `key` (const std::string &): The key
  - `value` (const std::string &): The value
- Return: Result<void> - success or error with details
- Details: key The key value The value Result<void> - success or error with details

#### `Result< void > scanMultiRange(const std::vector< ScanRange > &ranges, std::function< bool(std::string_view key, std::string_view value)> callback)`
- Source: `include/themis/base/interfaces/storage_interface.h`:145
- Brief: Scan multiple key ranges in a single call.
- Parameters:
  - `ranges` (const std::vector< ScanRange > &): List of {start_key, end_key} pairs to scan.
  - `callback` (std::function< bool(std::string_view key, std::string_view value)>): Called for each key-value pair; return false to stop.
- Return: Result<void> – ok on success, error on first failure.
- Details: Iterates all provided ranges in order, calling callback for every key-value pair encountered. Ranges are processed sequentially; overlapping ranges may deliver duplicate entries. Returns false from callback to stop iteration over the current range (and all subsequent ranges). The default implementation delegates each range to scanRange(). ranges List of {start_key, end_key} pairs to scan. callback Called for each key-value pair; return false to stop. Result<void> – ok on success, error on first failure.

#### `Result< void > scanPrefix(std::string_view prefix, std::function< bool(std::string_view key, std::string_view value)> callback)`
- Source: `include/themis/base/interfaces/storage_interface.h`:113
- Brief: Scan all keys with a given prefix.
- Parameters:
  - `prefix` (std::string_view): Key prefix to match.
  - `callback` (std::function< bool(std::string_view key, std::string_view value)>): Called for each key-value pair; return false to stop.
- Return: Result<void> – ok on success, error on failure.
- Details: Iterates all keys whose byte representation starts with prefix and calls callback for each. Iteration stops when callback returns false or no more matching keys exist. prefix Key prefix to match. callback Called for each key-value pair; return false to stop. Result<void> – ok on success, error on failure.

#### `Result< void > scanRange(std::string_view start_key, std::string_view end_key, std::function< bool(std::string_view key, std::string_view value)> callback)`
- Source: `include/themis/base/interfaces/storage_interface.h`:93
- Brief: Scan a key range [start_key, end_key).
- Parameters:
  - `start_key` (std::string_view): Inclusive lower bound (empty = beginning of keyspace).
  - `end_key` (std::string_view): Exclusive upper bound (empty = end of keyspace).
  - `callback` (std::function< bool(std::string_view key, std::string_view value)>): Called for each key-value pair; return false to stop.
- Return: Result<void> – ok on success, error on failure.
- Details: Iterates all keys in sorted order within the range and calls callback for each key-value pair. Iteration stops when callback returns false or the end of the range is reached. The default implementation returns ERR_STORAGE_NOT_IMPLEMENTED. Concrete engines that support range scans should override this. start_key Inclusive lower bound (empty = beginning of keyspace). end_key Exclusive upper bound (empty = end of keyspace). callback Called for each key-value pair; return false to stop. Result<void> – ok on success, error on failure.

#### `~IStorageEngine()=default`
- Source: `include/themis/base/interfaces/storage_interface.h`:38
- Brief: n/a
- Parameters: none

### themis::IStorageEngineFactory

#### `IStorageEnginePtr create(const std::string &db_path)=0`
- Source: `include/themis/base/interfaces/storage_interface.h`:183
- Brief: Create a storage engine instance.
- Parameters:
  - `db_path` (const std::string &): Path to database directory
- Return: Shared pointer to storage engine
- Details: db_path Path to database directory Shared pointer to storage engine

#### `~IStorageEngineFactory()=default`
- Source: `include/themis/base/interfaces/storage_interface.h`:175
- Brief: n/a
- Parameters: none

### themis::IVectorIndex

#### `uint32_t getDimension() const =0`
- Source: `include/themis/base/interfaces/index_interface.h`:167
- Brief: Get vector dimension.
- Parameters: none
- Return: Dimension of indexed vectors
- Details: Dimension of indexed vectors

#### `std::string getName() const =0`
- Source: `include/themis/base/interfaces/index_interface.h`:163
- Brief: Get index name.
- Parameters: none
- Return: Index name/identifier
- Details: Index name/identifier

#### `std::string getStatistics() const =0`
- Source: `include/themis/base/interfaces/index_interface.h`:171
- Brief: Get index statistics.
- Parameters: none
- Return: JSON string with statistics (count, memory usage, etc.)
- Details: JSON string with statistics (count, memory usage, etc.)

#### `bool insert(std::string_view primary_key, const std::vector< float > &vector)=0`
- Source: `include/themis/base/interfaces/index_interface.h`:133
- Brief: Insert or update a vector entry.
- Parameters:
  - `primary_key` (std::string_view): The primary key of the document
  - `vector` (const std::vector< float > &): The embedding vector
- Return: true on success, false on failure
- Details: primary_key The primary key of the document vector The embedding vector true on success, false on failure

#### `std::vector< VectorSearchResult > rangeSearch(const std::vector< float > &query_vector, float max_distance, const IExpressionEvaluator *filter=nullptr) const =0`
- Source: `include/themis/base/interfaces/index_interface.h`:156
- Brief: Range search (all vectors within distance threshold).
- Parameters:
  - `query_vector` (const std::vector< float > &): The query vector
  - `max_distance` (float): Maximum distance threshold
  - `filter` (const IExpressionEvaluator *): Optional filter expression
- Return: All results within threshold
- Details: query_vector The query vector max_distance Maximum distance threshold filter Optional filter expression All results within threshold

#### `bool remove(std::string_view primary_key)=0`
- Source: `include/themis/base/interfaces/index_interface.h`:139
- Brief: Remove a vector entry.
- Parameters:
  - `primary_key` (std::string_view): The primary key
- Return: true if entry existed and was removed, false otherwise
- Details: primary_key The primary key true if entry existed and was removed, false otherwise

#### `std::vector< VectorSearchResult > search(const std::vector< float > &query_vector, uint32_t k, const IExpressionEvaluator *filter=nullptr) const =0`
- Source: `include/themis/base/interfaces/index_interface.h`:146
- Brief: Search for k nearest neighbors.
- Parameters:
  - `query_vector` (const std::vector< float > &): The query vector
  - `k` (uint32_t): Number of results to return
  - `filter` (const IExpressionEvaluator *): Optional filter expression (injected evaluator)
- Return: Top k results sorted by distance (closest first)
- Details: query_vector The query vector k Number of results to return filter Optional filter expression (injected evaluator) Top k results sorted by distance (closest first)

#### `~IVectorIndex()=default`
- Source: `include/themis/base/interfaces/index_interface.h`:127
- Brief: n/a
- Parameters: none

### themis::QueryResult

#### `bool hasError() const`
- Source: `include/themis/base/interfaces/query_interface.h`:33
- Brief: n/a
- Parameters: none

### themis::VectorSearchResult

#### `VectorSearchResult(std::string pk, float dist)`
- Source: `include/themis/base/interfaces/index_interface.h`:117
- Brief: n/a
- Parameters:
  - `pk` (std::string): n/a
  - `dist` (float): n/a

### themis::bench::themis_dedicated

#### `void BM_Themis_BatchModuleVerify(benchmark::State &state)`
- Source: `benchmarks/themis/bench_themis_dedicated_gates.cpp`:118
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_Themis_DependencyResolve(benchmark::State &state)`
- Source: `benchmarks/themis/bench_themis_dedicated_gates.cpp`:102
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_Themis_ModuleVerify(benchmark::State &state)`
- Source: `benchmarks/themis/bench_themis_dedicated_gates.cpp`:70
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_Themis_WireSessionRequest(benchmark::State &state)`
- Source: `benchmarks/themis/bench_themis_dedicated_gates.cpp`:86
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `Name("TH-BM-01/ModuleVerify") -> Repetitions(kRepetitions) ->ReportAggregatesOnly(true)`
- Source: `benchmarks/themis/bench_themis_dedicated_gates.cpp`:79
- Brief: n/a
- Parameters:
  - `<unnamed>` ("TH-BM-01/ModuleVerify"): n/a

#### `Name("TH-BM-02/WireSessionRequest") -> Repetitions(kRepetitions) ->ReportAggregatesOnly(true)`
- Source: `benchmarks/themis/bench_themis_dedicated_gates.cpp`:95
- Brief: n/a
- Parameters:
  - `<unnamed>` ("TH-BM-02/WireSessionRequest"): n/a

#### `Name("TH-BM-03/DependencyResolve") -> Repetitions(kRepetitions) ->ReportAggregatesOnly(true)`
- Source: `benchmarks/themis/bench_themis_dedicated_gates.cpp`:111
- Brief: n/a
- Parameters:
  - `<unnamed>` ("TH-BM-03/DependencyResolve"): n/a

#### `Name("TH-BM-04/BatchModuleVerify1000") -> Repetitions(kRepetitions) ->ReportAggregatesOnly(true)`
- Source: `benchmarks/themis/bench_themis_dedicated_gates.cpp`:131
- Brief: n/a
- Parameters:
  - `<unnamed>` ("TH-BM-04/BatchModuleVerify1000"): n/a

#### `bool stubDependencyResolve(const std::string &dep) noexcept`
- Source: `benchmarks/themis/bench_themis_dedicated_gates.cpp`:62
- Brief: n/a
- Parameters:
  - `dep` (const std::string &): n/a

#### `uint8_t stubVerifyModule(const std::string &name) noexcept`
- Source: `benchmarks/themis/bench_themis_dedicated_gates.cpp`:53
- Brief: n/a
- Parameters:
  - `name` (const std::string &): n/a

#### `bool stubWireRequest(uint64_t session_id) noexcept`
- Source: `benchmarks/themis/bench_themis_dedicated_gates.cpp`:57
- Brief: n/a
- Parameters:
  - `session_id` (uint64_t): n/a

### themis::build_info

#### `THEMIS_BASE_API void clearHsmModuleStatusFn()`
- Source: `src/themis/build_info.cpp`:1139
- Brief: Clear Hsm Module Status Fn.
- Parameters: none
- Details: Remove any previously registered HSM module status bridge. Thread-safe. Calls: lk(), hsmStatusFnMutex(), hsmStatusFnStorage().

#### `std::string computeExecutableHash()`
- Source: `src/themis/build_info.cpp`:980
- Brief: ── Helper: SHA-256 hash of the running executable ─────────────────────────
- Parameters: none
- Return: Return value.
- Details: Return value. Calls: defined(), readlink(), assign(), GetModuleFileNameA(), empty(), f(), EVP_MD_CTX_new(), EVP_DigestInit_ex().

#### `THEMIS_BASE_API bool exportBuildManifest(const std::string &output_path)`
- Source: `src/themis/build_info.cpp`:1064
- Brief: Write a JSON build-manifest to output_path.
- Parameters:
  - `output_path` (const std::string &): Path to the output.
- Return: true on success, false on I/O error.
- Details: Export Build Manifest. The manifest contains all fields of ReproducibilityInfo plus the full BuildConfiguration. It is suitable for archiving alongside a release binary and for automated CI reproducibility checks. output_path Destination file path (will be created or overwritten). true on success, false on I/O error. output_path Path to the output. True when the operation succeeds. Calls: getReproducibilityInfo(), getBuildConfiguration(), out(), good().

#### `THEMIS_BASE_API std::string formatBuildInfo(const BuildConfiguration &config)`
- Source: `src/themis/build_info.cpp`:806
- Brief: Format Build Info.
- Parameters:
  - `config` (const BuildConfiguration &): Input parameter.
- Return: Return value.
- Details: Format build configuration as a human-readable string suitable for logging at server startup config Input parameter. Return value. Calls: size(), std::setw(), str().

#### `THEMIS_BASE_API BuildConfiguration getBuildConfiguration()`
- Source: `src/themis/build_info.cpp`:89
- Brief: Get Build Configuration.
- Parameters: none
- Return: Return value.
- Details: Get complete build configuration including edition and modules Return value. Calls: edition::EditionInfo::Get(), std::string(), defined(), std::to_string(), push_back(), lk(), hsmStatusFnMutex(), hsmStatusFnStorage().

#### `THEMIS_BASE_API std::vector< std::string > getCompiledModules()`
- Source: `src/themis/build_info.cpp`:917
- Brief: Get Compiled Modules.
- Parameters: none
- Return: Return value.
- Details: Get list of all modules compiled into this binary Return value. Calls: getBuildConfiguration(), push_back().

#### `THEMIS_BASE_API std::vector< std::string > getDisabledModules()`
- Source: `src/themis/build_info.cpp`:934
- Brief: Get Disabled Modules.
- Parameters: none
- Return: Return value.
- Details: Get list of all supported but not compiled modules Return value. Calls: getBuildConfiguration(), push_back().

#### `THEMIS_BASE_API ReproducibilityInfo getReproducibilityInfo()`
- Source: `src/themis/build_info.cpp`:1028
- Brief: Collect reproducibility metadata embedded at compile time.
- Parameters: none
- Return: Return value.
- Details: Get Reproducibility Info. cmake/CMakeLists.txt captures git HEAD, branch, dirty flag, and build-host via execute_process() and injects them as compile definitions (THEMIS_GIT_COMMIT, THEMIS_GIT_BRANCH, THEMIS_GIT_DIRTY, THEMIS_BUILD_HOST, THEMIS_BUILD_USER). Those definitions are read here. Return value. Calls: getBuildConfiguration(), computeExecutableHash().

#### `THEMIS_BASE_API std::string getVersionSummary()`
- Source: `src/themis/build_info.cpp`:883
- Brief: Get Version Summary.
- Parameters: none
- Return: Return value.
- Details: Get a compact summary of key build information for version endpoint Return value. Calls: getBuildConfiguration(), str().

#### `THEMIS_BASE_API bool isModuleCompiledIn(const std::string &module_name)`
- Source: `src/themis/build_info.cpp`:902
- Brief: Is Module Compiled In.
- Parameters:
  - `module_name` (const std::string &): Name of the module.
- Return: True when the operation succeeds.
- Details: Check if a specific module was compiled into the binary module_name Name of the module. True when the operation succeeds. Calls: getBuildConfiguration().

#### `THEMIS_BASE_API void setHsmModuleStatusFn(HsmModuleStatusFn fn)`
- Source: `src/themis/build_info.cpp`:1130
- Brief: Set Hsm Module Status Fn.
- Parameters:
  - `fn` (HsmModuleStatusFn): Input parameter.
- Details: Register a bridge that reports runtime HSM module status. Thread-safe. Pass an empty function to restore static defaults. fn Input parameter. Calls: lk(), hsmStatusFnMutex(), hsmStatusFnStorage(), std::move().

#### `THEMIS_BASE_API bool verifyBuildManifest(const std::string &manifest_path)`
- Source: `src/themis/build_info.cpp`:1104
- Brief: Verify that the build manifest at manifest_path matches the current binary's embedded build metadata.
- Parameters:
  - `manifest_path` (const std::string &): Path to the manifest.
- Return: true if all compared fields match, false otherwise.
- Details: Verify Build Manifest. Compares git_commit, toolchain, and key compile flags. Useful in CI to assert that a binary was built from the expected commit. manifest_path Path to a previously generated manifest JSON file. true if all compared fields match, false otherwise. manifest_path Path to the manifest. True when the operation succeeds. Calls: in(), content(), getReproducibilityInfo(), find(), containsField().

### themis::edition

#### `EditionType GetEditionType()`
- Source: `include/themis/edition.h`:65
- Brief: n/a
- Parameters: none

#### `bool IsEdition()`
- Source: `include/themis/edition.h`:345
- Brief: n/a
- Parameters: none

#### `bool IsFeatureEnabled(std::string_view feature_name)`
- Source: `include/themis/edition.h`:350
- Brief: n/a
- Parameters:
  - `feature_name` (std::string_view): n/a

### themis::edition::EditionInfo

#### `EditionInfo Get()`
- Source: `include/themis/edition.h`:381
- Brief: n/a
- Parameters: none

### themis::edition::EditionManager

#### `EditionManager()=default`
- Source: `include/themis/edition_manager.h`:480
- Brief: n/a
- Parameters: none

#### `EditionManager(const EditionManager &)=delete`
- Source: `include/themis/edition_manager.h`:82
- Brief: n/a
- Parameters:
  - `<unnamed>` (const EditionManager &): n/a

#### `bool checkNodeLimit(int requested_nodes, std::string &error_out) const`
- Source: `include/themis/edition_manager.h`:126
- Brief: Returns true if requested_nodes is within the edition limit.
- Parameters:
  - `requested_nodes` (int): Number of shard nodes the caller wants to use.
  - `error_out` (std::string &): Populated with an error string on failure.
- Details: COMMUNITY: up to SHARDING_MAX_NODES ENTERPRISE: up to SHARDING_MAX_NODES HYPERSCALER: unlimited (always returns true) requested_nodes Number of shard nodes the caller wants to use. error_out Populated with an error string on failure.

#### `bool checkVRAMLimit(int requested_vram_gb, std::string &error_out) const`
- Source: `include/themis/edition_manager.h`:138
- Brief: Returns true if requested_vram_gb is within the edition limit.
- Parameters:
  - `requested_vram_gb` (int): VRAM to allocate in gigabytes.
  - `error_out` (std::string &): Populated with an error string on failure.
- Details: COMMUNITY: up to GPU_MAX_VRAM_GB ENTERPRISE: up to GPU_MAX_VRAM_GB HYPERSCALER: unlimited (always returns true) requested_vram_gb VRAM to allocate in gigabytes. error_out Populated with an error string on failure.

#### `void clearAllFeatureOverrides()`
- Source: `include/themis/edition_manager.h`:457
- Brief: Remove all runtime overrides.
- Parameters: none
- Details: Clear All Feature Overrides. After this call every feature reverts to the standard edition + license decision. Thread-safe. Calls: lock(), clear().

#### `void clearConnectionPolicy()`
- Source: `include/themis/edition_manager.h`:358
- Brief: Remove any previously installed connection policy. Thread-safe.
- Parameters: none
- Details: Clear Connection Policy. Calls: lock(), reset(), THEMIS_INFO().

#### `void clearFeatureOverride(std::string_view feature_name)`
- Source: `include/themis/edition_manager.h`:447
- Brief: Remove the runtime override for a named feature flag.
- Parameters:
  - `feature_name` (std::string_view): Name of the feature.
- Details: Clear Feature Override. After calling this, isFeatureAvailable(feature_name) reverts to the standard edition + license decision. A no-op if no override exists for feature_name. Thread-safe. feature_name Name of the feature. Calls: lock(), erase(), std::string().

#### `void clearLLMResourcePolicy()`
- Source: `include/themis/edition_manager.h`:266
- Brief: Remove any previously installed LLM resource policy. Thread-safe.
- Parameters: none
- Details: Clear LLMResource Policy. Calls: lock(), reset(), THEMIS_INFO().

#### `void clearQueryLimitPolicy()`
- Source: `include/themis/edition_manager.h`:328
- Brief: Remove any previously installed query-limit policy. Thread-safe.
- Parameters: none
- Details: Clear Query Limit Policy. Calls: lock(), reset(), THEMIS_INFO().

#### `void clearRateLimitPolicy()`
- Source: `include/themis/edition_manager.h`:410
- Brief: Remove any previously installed global rate-limit policy. Thread-safe.
- Parameters: none
- Details: Clear Rate Limit Policy. Calls: lock(), reset(), THEMIS_INFO().

#### `void clearShardPolicy()`
- Source: `include/themis/edition_manager.h`:234
- Brief: Remove any previously installed shard-limit policy (reverts to compile-time default). Thread-safe.
- Parameters: none
- Details: Clear Shard Policy. Calls: lock(), reset(), THEMIS_INFO().

#### `void clearStorageOpsPolicy()`
- Source: `include/themis/edition_manager.h`:386
- Brief: Remove any previously installed storage-operations policy. Thread-safe.
- Parameters: none
- Details: Clear Storage Ops Policy. Calls: lock(), reset(), THEMIS_INFO().

#### `void clearTenantQuotaPolicy()`
- Source: `include/themis/edition_manager.h`:298
- Brief: Remove any previously installed tenant-quota policy. Thread-safe.
- Parameters: none
- Details: Clear Tenant Quota Policy. Calls: lock(), reset(), THEMIS_INFO().

#### `void clearVRAMPolicy()`
- Source: `include/themis/edition_manager.h`:228
- Brief: Remove any previously installed VRAM policy (reverts to compile-time default). Thread-safe.
- Parameters: none
- Details: Clear VRAMPolicy. Calls: lock(), reset(), THEMIS_INFO().

#### `std::vector< std::string > getAvailableFeatures() const`
- Source: `include/themis/edition_manager.h`:164
- Brief: Returns the names of all known gated features that are currently available (both compile-time ON and runtime license valid).
- Parameters: none

#### `std::string_view getEditionName() const noexcept`
- Source: `include/themis/edition_manager.h`:148
- Brief: Returns the compile-time edition name string (e.g. "COMMUNITY").
- Parameters: none

#### `EditionType getEditionType() const noexcept`
- Source: `include/themis/edition_manager.h`:145
- Brief: Returns the compile-time edition type.
- Parameters: none

#### `std::optional< bool > getFeatureOverride(std::string_view feature_name) const`
- Source: `include/themis/edition_manager.h`:477
- Brief: Returns the current runtime override value, if any.
- Parameters:
  - `feature_name` (std::string_view): n/a
- Return: std::nullopt when no override is set; true / false when an override has been set via setFeatureOverride().
- Details: std::nullopt when no override is set; true / false when an override has been set via setFeatureOverride(). Thread-safe.

#### `int getMaxNodes() const noexcept`
- Source: `include/themis/edition_manager.h`:151
- Brief: Maximum shard nodes for this edition (-1 = unlimited).
- Parameters: none

#### `int getMaxVRAMGB() const noexcept`
- Source: `include/themis/edition_manager.h`:154
- Brief: Maximum GPU VRAM in GB for this edition (-1 = unlimited).
- Parameters: none

#### `std::vector< std::string > getUnavailableFeatures() const`
- Source: `include/themis/edition_manager.h`:170
- Brief: Returns the names of all known gated features that are currently unavailable (compile-time OFF or runtime license invalid).
- Parameters: none

#### `std::string getUpgradeMessage(std::string_view feature_name) const`
- Source: `include/themis/edition_manager.h`:182
- Brief: Returns a human-readable upgrade message for the given feature.
- Parameters:
  - `feature_name` (std::string_view): n/a
- Details: The message explains which edition supports the feature and how to obtain it. Returns an empty string if the feature is already available.

#### `bool hasFeatureOverride(std::string_view feature_name) const`
- Source: `include/themis/edition_manager.h`:467
- Brief: Returns true if a runtime override is set for the feature.
- Parameters:
  - `feature_name` (std::string_view): n/a
- Details: Does not indicate whether the feature is available — use isFeatureAvailable() for that. Thread-safe.

#### `bool installConnectionPolicy(std::shared_ptr< network::IConnectionPolicy > policy, uint32_t claimed_max_http2_streams, uint32_t claimed_max_sse_connections, uint32_t claimed_max_total_connections, uint32_t claimed_max_sse_events_per_sec)`
- Source: `include/themis/edition_manager.h`:348
- Brief: Install a signed connection policy supplied by an edition-upgrade plugin.
- Parameters:
  - `policy` (std::shared_ptr< network::IConnectionPolicy >): Input parameter.
  - `claimed_max_http2_streams` (uint32_t): Input parameter.
  - `claimed_max_sse_connections` (uint32_t): Input parameter.
  - `claimed_max_total_connections` (uint32_t): Input parameter.
  - `claimed_max_sse_events_per_sec` (uint32_t): Input parameter.
- Return: true on acceptance, false when a ceiling is exceeded.
- Details: ============================================================================ Group 2+3: Connection policy ============================================================================ Defense in Depth: each claimed value is validated against the corresponding compile-time ceiling in edition.h (0 = unlimited ceilings always accept). A rejected install is logged and returns false. Thread-safe. policy Connection policy implementation. claimed_max_http2_streams Max HTTP/2 streams per connection; 0 = unlimited. claimed_max_sse_connections Max total SSE connections; 0 = unlimited. claimed_max_total_connections Max total server connections; 0 = unlimited. claimed_max_sse_events_per_sec Max SSE events/sec; 0 = unlimited. true on acceptance, false when a ceiling is exceeded. policy Input parameter. claimed_max_http2_streams Input parameter. claimed_max_sse_connections Input parameter. claimed_max_total_connections Input parameter. claimed_max_sse_events_per_sec Input parameter. True when the operation succeeds.

#### `bool installLLMResourcePolicy(std::shared_ptr< llm::ILLMResourcePolicy > policy, int64_t claimed_max_context_tokens, int32_t claimed_max_model_instances, int64_t claimed_max_vram_per_model_mb)`
- Source: `include/themis/edition_manager.h`:257
- Brief: Install a signed LLM resource policy supplied by an edition-upgrade plugin.
- Parameters:
  - `policy` (std::shared_ptr< llm::ILLMResourcePolicy >): Input parameter.
  - `claimed_max_context_tokens` (int64_t): Input parameter.
  - `claimed_max_model_instances` (int32_t): Input parameter.
  - `claimed_max_vram_per_model_mb` (int64_t): Input parameter.
- Return: true on acceptance, false when a ceiling is exceeded.
- Details: ============================================================================ Group 4: LLM resource policy ============================================================================ Defense in Depth: claimed_max_context_tokens must satisfy (LLM_MAX_CONTEXT_TOKENS == 0 \|\| claimed ≤ LLM_MAX_CONTEXT_TOKENS); claimed_max_model_instances must satisfy (LLM_MAX_MODEL_INSTANCES == -1 \|\| claimed ≤ LLM_MAX_MODEL_INSTANCES); claimed_max_vram_per_model_mb must satisfy (LLM_MAX_VRAM_PER_MODEL_MB == 0 \|\| claimed ≤ LLM_MAX_VRAM_PER_MODEL_MB). A rejected install is logged and returns false. Thread-safe. policy LLM resource policy implementation. claimed_max_context_tokens Max context tokens per inference; 0 = unlimited. claimed_max_model_instances Max concurrently loaded models; -1 = unlimited. claimed_max_vram_per_model_mb Max VRAM per model in MiB; 0 = unlimited. true on acceptance, false when a ceiling is exceeded. policy Input parameter. claimed_max_context_tokens Input parameter. claimed_max_model_instances Input parameter. claimed_max_vram_per_model_mb Input parameter. True when the operation succeeds.

#### `bool installQueryLimitPolicy(std::shared_ptr< query::IQueryLimitPolicy > policy, uint32_t claimed_max_depth, uint32_t claimed_max_complexity, uint64_t claimed_max_payload_bytes, uint64_t claimed_max_result_rows)`
- Source: `include/themis/edition_manager.h`:318
- Brief: Install a signed query-limit policy supplied by an edition-upgrade plugin.
- Parameters:
  - `policy` (std::shared_ptr< query::IQueryLimitPolicy >): Input parameter.
  - `claimed_max_depth` (uint32_t): Input parameter.
  - `claimed_max_complexity` (uint32_t): Input parameter.
  - `claimed_max_payload_bytes` (uint64_t): Input parameter.
  - `claimed_max_result_rows` (uint64_t): Input parameter.
- Return: true on acceptance, false when a ceiling is exceeded.
- Details: ============================================================================ Group 2: Query limit policy ============================================================================ Defense in Depth: each claimed value is validated against the corresponding compile-time ceiling in edition.h (0 = unlimited ceilings always accept). A rejected install is logged and returns false. Thread-safe. policy Query limit policy implementation. claimed_max_depth Max GraphQL/AQL depth; 0 = unlimited. claimed_max_complexity Max query complexity score; 0 = unlimited. claimed_max_payload_bytes Max request payload bytes; 0 = unlimited. claimed_max_result_rows Max result rows per query; 0 = unlimited. true on acceptance, false when a ceiling is exceeded. policy Input parameter. claimed_max_depth Input parameter. claimed_max_complexity Input parameter. claimed_max_payload_bytes Input parameter. claimed_max_result_rows Input parameter. True when the operation succeeds.

#### `bool installRateLimitPolicy(std::shared_ptr< ratelimit::IRateLimitPolicy > policy, uint64_t claimed_max_global_rps)`
- Source: `include/themis/edition_manager.h`:403
- Brief: Install a signed global rate-limit policy supplied by an edition-upgrade plugin.
- Parameters:
  - `policy` (std::shared_ptr< ratelimit::IRateLimitPolicy >): Input parameter.
  - `claimed_max_global_rps` (uint64_t): Input parameter.
- Return: true on acceptance, false when the ceiling is exceeded.
- Details: ============================================================================ Group 3: Global rate-limit policy ============================================================================ Defense in Depth: claimed_max_global_rps must satisfy (RATE_LIMIT_MAX_GLOBAL_RPS == 0 \|\| claimed ≤ RATE_LIMIT_MAX_GLOBAL_RPS). A rejected install is logged and returns false. Thread-safe. policy Rate-limit policy implementation. claimed_max_global_rps Max global requests per second; 0 = unlimited. true on acceptance, false when the ceiling is exceeded. policy Input parameter. claimed_max_global_rps Input parameter. True when the operation succeeds.

#### `bool installShardPolicy(std::shared_ptr< sharding::IShardLimitPolicy > policy, int claimed_max_nodes)`
- Source: `include/themis/edition_manager.h`:221
- Brief: Install a signed shard-limit policy supplied by an edition-upgrade plugin.
- Parameters:
  - `policy` (std::shared_ptr< sharding::IShardLimitPolicy >): Input parameter.
  - `claimed_max_nodes` (int): Input parameter.
- Return: true Policy accepted and installed.
- Details: Install Shard Policy. Enforces the compile-time ceiling SHARDING_MAX_NODES (Defense in Depth). A rejected install is logged; the binary retains its default node limit. Thread-safe. policy The IShardLimitPolicy implementation provided by the plugin. claimed_max_nodes Maximum shard-node count the policy declares it allows. Must be <= SHARDING_MAX_NODES compile-time ceiling, or -1. true Policy accepted and installed. false Policy rejected — ceiling exceeded. policy Input parameter. claimed_max_nodes Input parameter. True when the operation succeeds.

#### `bool installStorageOpsPolicy(std::shared_ptr< storage::IStorageOpsPolicy > policy, int32_t claimed_max_background_jobs, uint64_t claimed_max_compaction_bytes_per_sec, int32_t claimed_max_concurrent_snapshots)`
- Source: `include/themis/edition_manager.h`:377
- Brief: Install a signed storage-operations policy supplied by an edition-upgrade plugin.
- Parameters:
  - `policy` (std::shared_ptr< storage::IStorageOpsPolicy >): Input parameter.
  - `claimed_max_background_jobs` (int32_t): Input parameter.
  - `claimed_max_compaction_bytes_per_sec` (uint64_t): Input parameter.
  - `claimed_max_concurrent_snapshots` (int32_t): Input parameter.
- Return: true on acceptance, false when a ceiling is exceeded.
- Details: ============================================================================ Group 5: Storage operations policy ============================================================================ Defense in Depth: each claimed value is validated against the corresponding compile-time ceiling in edition.h (-1 = unlimited for counts, 0 = unlimited for rates). A rejected install is logged and returns false. Thread-safe. policy Storage ops policy implementation. claimed_max_background_jobs Max concurrent background jobs; -1 = unlimited. claimed_max_compaction_bytes_per_sec Max compaction I/O rate in bytes/s; 0 = unlimited. claimed_max_concurrent_snapshots Max concurrent snapshots; -1 = unlimited. true on acceptance, false when a ceiling is exceeded. policy Input parameter. claimed_max_background_jobs Input parameter. claimed_max_compaction_bytes_per_sec Input parameter. claimed_max_concurrent_snapshots Input parameter. True when the operation succeeds.

#### `bool installTenantQuotaPolicy(std::shared_ptr< tenant::ITenantQuotaPolicy > policy, uint64_t claimed_max_storage_bytes, uint64_t claimed_max_documents, uint32_t claimed_max_collections, uint32_t claimed_max_concurrent_queries, uint32_t claimed_max_rps)`
- Source: `include/themis/edition_manager.h`:287
- Brief: Install a signed tenant-quota policy supplied by an edition-upgrade plugin.
- Parameters:
  - `policy` (std::shared_ptr< tenant::ITenantQuotaPolicy >): Input parameter.
  - `claimed_max_storage_bytes` (uint64_t): Input parameter.
  - `claimed_max_documents` (uint64_t): Input parameter.
  - `claimed_max_collections` (uint32_t): Input parameter.
  - `claimed_max_concurrent_queries` (uint32_t): Input parameter.
  - `claimed_max_rps` (uint32_t): Input parameter.
- Return: true on acceptance, false when a ceiling is exceeded.
- Details: ============================================================================ Group 1+3: Tenant quota policy ============================================================================ Defense in Depth: each claimed value is validated against the corresponding compile-time ceiling in edition.h (0 = unlimited ceilings always accept). A rejected install is logged and returns false. Thread-safe. policy Tenant quota policy implementation. claimed_max_storage_bytes Max storage per tenant in bytes; 0 = unlimited. claimed_max_documents Max documents per tenant; 0 = unlimited. claimed_max_collections Max collections per tenant; 0 = unlimited. claimed_max_concurrent_queries Max concurrent queries per tenant; 0 = unlimited. claimed_max_rps Max requests per second per tenant; 0 = unlimited. true on acceptance, false when a ceiling is exceeded. policy Input parameter. claimed_max_storage_bytes Input parameter. claimed_max_documents Input parameter. claimed_max_collections Input parameter. claimed_max_concurrent_queries Input parameter. claimed_max_rps Input parameter. True when the operation succeeds.

#### `bool installVRAMPolicy(std::shared_ptr< gpu::IVRAMPolicy > policy, int claimed_max_vram_gb)`
- Source: `include/themis/edition_manager.h`:204
- Brief: Install a signed VRAM policy supplied by an edition-upgrade plugin.
- Parameters:
  - `policy` (std::shared_ptr< gpu::IVRAMPolicy >): Input parameter.
  - `claimed_max_vram_gb` (int): Input parameter.
- Return: true Policy accepted and installed.
- Details: Install VRAMPolicy. The policy is accepted only when claimed_max_vram_gb does not exceed the compile-time ceiling (GPU_MAX_VRAM_GB), or the ceiling is -1 (Hyperscaler — unlimited). A rejected install is logged and returns false; the binary retains its compile-time default. Thread-safe. policy The IVRAMPolicy implementation provided by the plugin. claimed_max_vram_gb Maximum VRAM (GiB) the policy declares it allows. Must be <= GPU_MAX_VRAM_GB compile-time ceiling, or -1. true Policy accepted and installed. false Policy rejected — ceiling exceeded. policy Input parameter. claimed_max_vram_gb Input parameter. True when the operation succeeds.

#### `EditionManager & instance()`
- Source: `include/themis/edition_manager.h`:79
- Brief: Retrieve the process-wide singleton instance.
- Parameters: none
- Return: Return value.
- Details: Instance. Return value. Implements instance without additional internal calls.

#### `bool isFeatureAvailable(std::string_view feature_name) const`
- Source: `include/themis/edition_manager.h`:103
- Brief: Returns true if the named feature is available at runtime.
- Parameters:
  - `feature_name` (std::string_view): One of: "enterprise_plugins", "multi_master", "field_encryption", "rbac", "hsm", or any unknown feature name (unknown → always allowed).
- Details: The check is two-stage: Compile-time: edition::IsFeatureEnabled(feature_name) must be true. Runtime: license::RuntimeLicenseGate::instance() must allow it. Features that are not in the Enterprise gate list (i.e. Community features) are always allowed. feature_name One of: "enterprise_plugins", "multi_master", "field_encryption", "rbac", "hsm", or any unknown feature name (unknown → always allowed).

#### `bool isFeatureAvailable(std::string_view feature_name, std::string &error_out) const`
- Source: `include/themis/edition_manager.h`:109
- Brief: Like isFeatureAvailable() but also populates error_out with a human-readable explanation when returning false.
- Parameters:
  - `feature_name` (std::string_view): n/a
  - `error_out` (std::string &): n/a

#### `EditionManager & operator=(const EditionManager &)=delete`
- Source: `include/themis/edition_manager.h`:83
- Brief: n/a
- Parameters:
  - `<unnamed>` (const EditionManager &): n/a

#### `void setFeatureOverride(std::string_view feature_name, bool enabled)`
- Source: `include/themis/edition_manager.h`:436
- Brief: Set a runtime override for a named feature flag.
- Parameters:
  - `feature_name` (std::string_view): Name of the feature.
  - `enabled` (bool): Input parameter.
- Details: Set Feature Override. Overrides are layered on top of (and cannot bypass) the compile-time edition gate. An override of true for a Community-edition feature that is compile-time disabled is silently stored but will never allow the feature — the compile-time gate still applies. Typical use-cases: Staged rollout: set to false while validating, flip to true when ready to expose the feature to this node. Maintenance mode: set to false to block a feature without redeployment. Thread-safe. feature_name Feature to override (see edition::kGatedFeatureNames). enabled true = allow (if edition/license also allow); false = always block. feature_name Name of the feature. enabled Input parameter. Calls: lock(), std::string().

#### `~EditionManager()=default`
- Source: `include/themis/edition_manager.h`:481
- Brief: n/a
- Parameters: none

### themis::gpu

#### `bool CanUseGPUForVectorSearch() noexcept`
- Source: `include/themis/gpu/memory_manager.h`:360
- Brief: Returns true when the current edition supports GPU acceleration (i.e. VRAM limit > 0).
- Parameters: none

#### `std::string GetGPUFallbackStrategy()`
- Source: `include/themis/gpu/memory_manager.h`:367
- Brief: Human-readable CPU-fallback message for when VRAM is exhausted.
- Parameters: none

#### `const char * capabilityName(GPUPolicy::Capability cap)`
- Source: `include/themis/gpu/policy.h`:146
- Brief: n/a
- Parameters:
  - `cap` (GPUPolicy::Capability): n/a

#### `const char * interconnectTypeName(InterconnectType t) noexcept`
- Source: `include/themis/gpu/cluster_topology.h`:39
- Brief: Returns a human-readable name for an InterconnectType.
- Parameters:
  - `t` (InterconnectType): n/a

#### `shared_gpu_ptr< T > make_shared_gpu(size_t count)`
- Source: `include/themis/gpu/gpu_memory.h`:534
- Brief: Factory function: create shared_gpu_ptr with allocation.
- Parameters:
  - `count` (size_t): Number of elements
- Return: shared_gpu_ptr<T> with refcount 1
- Throws:
  - std::runtime_error: on allocation failure
- Details: Allocates count*sizeof(T) bytes on GPU and returns shared_gpu_ptr. T Element type count Number of elements shared_gpu_ptr<T> with refcount 1 std::runtime_error on allocation failure Example: autoshared_buf=make_shared_gpu<float>(1000); autoview=shared_buf;//copies;incrementsrefcount

#### `unique_gpu_ptr< T > make_unique_gpu(size_t count)`
- Source: `include/themis/gpu/gpu_memory.h`:324
- Brief: Factory function: create unique_gpu_ptr with allocation.
- Parameters:
  - `count` (size_t): Number of elements to allocate
- Return: unique_gpu_ptr<T> wrapping allocation
- Throws:
  - std::runtime_error: on allocation failure (cudaErrorMemoryAllocation)
  - std::bad_alloc: if wrapped by error handler
- Details: Allocates count*sizeof(T) bytes on GPU device and returns unique_gpu_ptr. T Element type count Number of elements to allocate unique_gpu_ptr<T> wrapping allocation std::runtime_error on allocation failure (cudaErrorMemoryAllocation) std::bad_alloc if wrapped by error handler Exception safety: If allocation fails, exception thrown; no partial allocation Caller must catch and handle out-of-memory Example: try{ autod_data=make_unique_gpu<float>(num_elements); //...used_data... }catch(conststd::runtime_error&e){ //handleOOM:degradetoCPU } unique_gpu_ptr

#### `const char * migStatusName(MIGManager::Status s) noexcept`
- Source: `include/themis/gpu/mig_manager.h`:309
- Brief: Human-readable name for a MIGManager::Status value.
- Parameters:
  - `s` (MIGManager::Status): n/a

#### `const char * p2pStatusName(GPUP2PTransferManager::Status s) noexcept`
- Source: `include/themis/gpu/p2p_transfer.h`:249
- Brief: Human-readable name for a GPUP2PTransferManager::Status value.
- Parameters:
  - `s` (GPUP2PTransferManager::Status): n/a

#### `const char * sandboxStatusName(WASMKernelSandbox::Status s) noexcept`
- Source: `include/themis/gpu/wasm_kernel_sandbox.h`:244
- Brief: Human-readable name for a WASMKernelSandbox::Status value.
- Parameters:
  - `s` (WASMKernelSandbox::Status): n/a

### themis::gpu::ClusterConfig

#### `bool has_infiniband() const noexcept`
- Source: `include/themis/gpu/cluster_config.h`:151
- Brief: True when InfiniBand is configured and enabled.
- Parameters: none

#### `bool is_multi_node() const noexcept`
- Source: `include/themis/gpu/cluster_config.h`:148
- Brief: True when this config represents a real multi-node deployment.
- Parameters: none

### themis::gpu::DeviceDiscovery

#### `std::vector< DeviceInfo > Enumerate()`
- Source: `include/themis/gpu/device_discovery.h`:74
- Brief: Enumerate all available GPU devices.
- Parameters: none
- Return: List of discovered devices. Empty only when the edition has disabled GPU and no CPU-fallback sentinel is appropriate. On CI / no-GPU machines returns a single CPU_FALLBACK entry.
- Details: List of discovered devices. Empty only when the edition has disabled GPU and no CPU-fallback sentinel is appropriate. On CI / no-GPU machines returns a single CPU_FALLBACK entry.

#### `DeviceInfo GetBestDevice()`
- Source: `include/themis/gpu/device_discovery.h`:89
- Brief: Convenience overload: enumerate then pick best.
- Parameters: none

#### `DeviceInfo GetBestDevice(const std::vector< DeviceInfo > &devices)`
- Source: `include/themis/gpu/device_discovery.h`:84
- Brief: Return the device best suited for a new allocation.
- Parameters:
  - `devices` (const std::vector< DeviceInfo > &): List produced by Enumerate() (avoids re-enumeration).
- Details: Currently returns the device with the most free VRAM. Returns the CPU-fallback sentinel when no GPU is available. devices List produced by Enumerate() (avoids re-enumeration).

#### `std::vector< DeviceInfo > GetHealthyDevices(const std::vector< DeviceInfo > &devices)`
- Source: `include/themis/gpu/device_discovery.h`:94
- Brief: Return every device that reported is_healthy == true.
- Parameters:
  - `devices` (const std::vector< DeviceInfo > &): n/a

#### `bool HasGPU()`
- Source: `include/themis/gpu/device_discovery.h`:106
- Brief: Convenience overload: enumerate then check.
- Parameters: none

#### `bool HasGPU(const std::vector< DeviceInfo > &devices)`
- Source: `include/themis/gpu/device_discovery.h`:101
- Brief: True when at least one real (non-CPU-fallback) device is present and healthy.
- Parameters:
  - `devices` (const std::vector< DeviceInfo > &): n/a

### themis::gpu::GPUAdminAPI

#### `GPUAdminAPI(const GPUConfig &config, GPULoadBalancer *balancer=nullptr)`
- Source: `include/themis/gpu/admin_api.h`:51
- Brief: Default constructor: uses the global GPUMemoryManager singleton and GPUMetrics singleton. Pass a non-null balancer for device stats; if nullptr the devices endpoint returns an empty list.
- Parameters:
  - `config` (const GPUConfig &): n/a
  - `balancer` (GPULoadBalancer *): n/a

#### `std::string getDevicesJson() const`
- Source: `include/themis/gpu/admin_api.h`:85
- Brief: Serialise per-device load to JSON.
- Parameters: none
- Details: Returns a JSON array where each element has: index, name, backend, free_vram_bytes, tracked_alloc_bytes, is_healthy, failure_reason Returns an empty JSON array if no load balancer was provided.

#### `std::string getGeoBackendStatsJson() const`
- Source: `include/themis/gpu/admin_api.h`:109
- Brief: Serialise GPU spatial backend (geo) stats to JSON.
- Parameters: none
- Details: Returns a JSON object with keys: backend_name, gpu_present, circuit_open, device_name, batch_calls, batch_fallbacks, batch_pairs_processed, exact_calls, exact_errors, batch_avg_latency_us, batch_max_latency_us Suitable for the endpoint: GET /admin/gpu/geo

#### `std::string getMIGInstancesJson() const`
- Source: `include/themis/gpu/admin_api.h`:124
- Brief: Serialise active MIG partition list to JSON.
- Parameters: none
- Details: Returns a JSON array where each element has: instance_id, device_index, gi_id, profile, memory_bytes, is_active, tenant_id Returns an empty JSON array when no MIG instances exist. Because MIGManager::createPartition() is gated on the MIG_MANAGER feature flag, the array will naturally be empty when the flag is disabled. Suitable for the endpoint: GET /admin/gpu/mig

#### `std::string getStatsJson() const`
- Source: `include/themis/gpu/admin_api.h`:66
- Brief: Serialise global GPU stats to JSON.
- Parameters: none
- Details: Returns a JSON object with keys: edition_vram_limit_bytes, allocated_bytes, peak_bytes, allocation_count, deallocation_count, usage_percent, gpu_acceleration_enabled, edition_info

#### `std::string getTenantsJson() const`
- Source: `include/themis/gpu/admin_api.h`:74
- Brief: Serialise per-tenant VRAM breakdown to JSON.
- Parameters: none
- Details: Returns a JSON array where each element has: tenant_id, quota_bytes, allocated_bytes, peak_bytes, headroom_bytes

#### `std::string jsonEscape(const std::string &s)`
- Source: `include/themis/gpu/admin_api.h`:130
- Brief: n/a
- Parameters:
  - `s` (const std::string &): n/a

#### `std::string simulateJson(uint64_t bytes) const`
- Source: `include/themis/gpu/admin_api.h`:96
- Brief: Dry-run simulation: would bytes be accepted right now?
- Parameters:
  - `bytes` (uint64_t): n/a
- Details: Uses GPUConfig::simulateAllocation() against the current live allocation counter. Input JSON (optional, for logging): { "bytes": <uint64>, "tag": "..." } Returns JSON: { "accepted": true/false, "reason": "..." }

### themis::gpu::GPUAlerts

#### `GPUAlerts()=default`
- Source: `include/themis/gpu/alerts.h`:87
- Brief: n/a
- Parameters: none

#### `GPUAlerts(const Config &cfg)`
- Source: `include/themis/gpu/alerts.h`:88
- Brief: n/a
- Parameters:
  - `cfg` (const Config &): n/a

#### `std::vector< AlertStatus > currentStatuses() const`
- Source: `include/themis/gpu/alerts.h`:125
- Brief: n/a
- Parameters: none

#### `size_t evaluate()`
- Source: `include/themis/gpu/alerts.h`:120
- Brief: Evaluate all rules against current metric values.
- Parameters: none
- Return: Number of currently firing alerts.
- Details: For each rule that transitions state (INACTIVE→FIRING or FIRING→INACTIVE) registered callbacks are called. Number of currently firing alerts.

#### `void fireCallback(const AlertStatus &s)`
- Source: `include/themis/gpu/alerts.h`:145
- Brief: n/a
- Parameters:
  - `s` (const AlertStatus &): n/a

#### `size_t firingCount() const`
- Source: `include/themis/gpu/alerts.h`:126
- Brief: n/a
- Parameters: none

#### `bool isFiring(const std::string &alert_name) const`
- Source: `include/themis/gpu/alerts.h`:127
- Brief: n/a
- Parameters:
  - `alert_name` (const std::string &): n/a

#### `void onAlert(AlertCallback callback)`
- Source: `include/themis/gpu/alerts.h`:106
- Brief: Register a callback invoked whenever an alert fires or resolves.
- Parameters:
  - `callback` (AlertCallback): n/a

#### `void setCircuitOpen(bool is_open)`
- Source: `include/themis/gpu/alerts.h`:96
- Brief: n/a
- Parameters:
  - `is_open` (bool): n/a

#### `void setDeviceAvailable(bool available)`
- Source: `include/themis/gpu/alerts.h`:97
- Brief: n/a
- Parameters:
  - `available` (bool): n/a

#### `void setErrorRate(float rate)`
- Source: `include/themis/gpu/alerts.h`:94
- Brief: 0.0–1.0
- Parameters:
  - `rate` (float): n/a

#### `void setFallbackRate(float rate)`
- Source: `include/themis/gpu/alerts.h`:95
- Brief: 0.0–1.0
- Parameters:
  - `rate` (float): n/a

#### `void setVRAMUsage(float used_fraction)`
- Source: `include/themis/gpu/alerts.h`:93
- Brief: 0.0–1.0
- Parameters:
  - `used_fraction` (float): n/a

#### `void updateAlert(const std::string &name, bool condition, float value, float threshold, const std::string &msg)`
- Source: `include/themis/gpu/alerts.h`:148
- Brief: n/a
- Parameters:
  - `name` (const std::string &): n/a
  - `condition` (bool): n/a
  - `value` (float): n/a
  - `threshold` (float): n/a
  - `msg` (const std::string &): n/a

### themis::gpu::GPUAuditLog

#### `GPUAuditLog(const GPUAuditLog &)=delete`
- Source: `include/themis/gpu/audit_log.h`:70
- Brief: n/a
- Parameters:
  - `<unnamed>` (const GPUAuditLog &): n/a

#### `GPUAuditLog(size_t capacity=1024)`
- Source: `include/themis/gpu/audit_log.h`:67
- Brief: n/a
- Parameters:
  - `capacity` (size_t): Maximum events kept. When the buffer is full the oldest event is overwritten (ring buffer semantics).
- Details: capacity Maximum events kept. When the buffer is full the oldest event is overwritten (ring buffer semantics).

#### `size_t capacity() const`
- Source: `include/themis/gpu/audit_log.h`:107
- Brief: Capacity of the ring buffer.
- Parameters: none

#### `void clear()`
- Source: `include/themis/gpu/audit_log.h`:113
- Brief: Clear all events.
- Parameters: none

#### `GPUAuditLog & operator=(const GPUAuditLog &)=delete`
- Source: `include/themis/gpu/audit_log.h`:71
- Brief: n/a
- Parameters:
  - `<unnamed>` (const GPUAuditLog &): n/a

#### `void record(EventType type, uint64_t size_bytes, const std::string &tag, const std::string &tenant_id="", const std::string &message="")`
- Source: `include/themis/gpu/audit_log.h`:76
- Brief: n/a
- Parameters:
  - `type` (EventType): n/a
  - `size_bytes` (uint64_t): n/a
  - `tag` (const std::string &): n/a
  - `tenant_id` (const std::string &): n/a
  - `message` (const std::string &): n/a

#### `void recordAllocFailGlobalLimit(uint64_t bytes, const std::string &tag, const std::string &tenant_id="")`
- Source: `include/themis/gpu/audit_log.h`:85
- Brief: n/a
- Parameters:
  - `bytes` (uint64_t): n/a
  - `tag` (const std::string &): n/a
  - `tenant_id` (const std::string &): n/a

#### `void recordAllocFailTenantQuota(uint64_t bytes, const std::string &tag, const std::string &tenant_id)`
- Source: `include/themis/gpu/audit_log.h`:87
- Brief: n/a
- Parameters:
  - `bytes` (uint64_t): n/a
  - `tag` (const std::string &): n/a
  - `tenant_id` (const std::string &): n/a

#### `void recordAllocSuccess(uint64_t bytes, const std::string &tag, const std::string &tenant_id="")`
- Source: `include/themis/gpu/audit_log.h`:83
- Brief: n/a
- Parameters:
  - `bytes` (uint64_t): n/a
  - `tag` (const std::string &): n/a
  - `tenant_id` (const std::string &): n/a

#### `void recordCircuitOpened(const std::string &detail)`
- Source: `include/themis/gpu/audit_log.h`:94
- Brief: n/a
- Parameters:
  - `detail` (const std::string &): n/a

#### `void recordCircuitReset()`
- Source: `include/themis/gpu/audit_log.h`:95
- Brief: n/a
- Parameters: none

#### `void recordDealloc(uint64_t bytes, const std::string &tag, const std::string &tenant_id="")`
- Source: `include/themis/gpu/audit_log.h`:89
- Brief: n/a
- Parameters:
  - `bytes` (uint64_t): n/a
  - `tag` (const std::string &): n/a
  - `tenant_id` (const std::string &): n/a

#### `void recordDeviceUnavailable(const std::string &detail)`
- Source: `include/themis/gpu/audit_log.h`:93
- Brief: n/a
- Parameters:
  - `detail` (const std::string &): n/a

#### `void recordFallbackToCPU(const std::string &reason, const std::string &tenant_id="")`
- Source: `include/themis/gpu/audit_log.h`:91
- Brief: n/a
- Parameters:
  - `reason` (const std::string &): n/a
  - `tenant_id` (const std::string &): n/a

#### `size_t size() const`
- Source: `include/themis/gpu/audit_log.h`:104
- Brief: Number of events currently stored (≤ capacity).
- Parameters: none

#### `std::vector< Event > snapshot() const`
- Source: `include/themis/gpu/audit_log.h`:101
- Brief: Snapshot of all events currently in the ring buffer (oldest-first).
- Parameters: none

#### `uint64_t totalRecorded() const`
- Source: `include/themis/gpu/audit_log.h`:110
- Brief: Total events recorded since construction (may exceed capacity).
- Parameters: none

### themis::gpu::GPUClusterCoordinator

#### `GPUClusterCoordinator()=default`
- Source: `include/themis/gpu/cluster_coordinator.h`:267
- Brief: n/a
- Parameters: none

#### `GPUClusterCoordinator & GetInstance()`
- Source: `include/themis/gpu/cluster_coordinator.h`:108
- Brief: n/a
- Parameters: none

#### `const ClusterConfig & clusterConfig() const noexcept`
- Source: `include/themis/gpu/cluster_coordinator.h`:251
- Brief: Return the active cluster configuration.
- Parameters: none

#### `ClusterHealth clusterHealth() const`
- Source: `include/themis/gpu/cluster_coordinator.h`:254
- Brief: Return a cluster-wide health summary.
- Parameters: none

#### `const ClusterConfig & config() const noexcept`
- Source: `include/themis/gpu/cluster_coordinator.h`:265
- Brief: n/a
- Parameters: none

#### `bool deregisterNode(const std::string &node_id)`
- Source: `include/themis/gpu/cluster_coordinator.h`:177
- Brief: Remove a node from the cluster registry (registry path).
- Parameters:
  - `node_id` (const std::string &): n/a
- Return: false if no node with that ID exists.
- Details: false if no node with that ID exists.

#### `void expireStaleNodes()`
- Source: `include/themis/gpu/cluster_coordinator.h`:198
- Brief: Scan the node list and mark nodes whose last heartbeat exceeds the configured timeout as OFFLINE.
- Parameters: none
- Details: Should be called periodically by a background health-check thread.

#### `NodeInfo * findNode(const std::string &id)`
- Source: `include/themis/gpu/cluster_coordinator.h`:279
- Brief: n/a
- Parameters:
  - `id` (const std::string &): n/a

#### `std::vector< NodeInfo > getClusterNodes() const`
- Source: `include/themis/gpu/cluster_coordinator.h`:256
- Brief: n/a
- Parameters: none

#### `std::vector< NodeInfo > getOnlineNodes() const`
- Source: `include/themis/gpu/cluster_coordinator.h`:257
- Brief: n/a
- Parameters: none

#### `bool initialize(const ClusterConfig &config)`
- Source: `include/themis/gpu/cluster_coordinator.h`:162
- Brief: Initialise the coordinator from a cluster config alone.
- Parameters:
  - `config` (const ClusterConfig &): n/a
- Return: true on success.
- Details: Seeds the node list from ClusterConfig::nodes (COORDINATOR mode) or registers the local node (STANDALONE / WORKER mode). true on success.

#### `void initialize(const ClusterConfig &config, const std::vector< DeviceInfo > &devices)`
- Source: `include/themis/gpu/cluster_coordinator.h`:126
- Brief: Initialise with a cluster config and the local device list.
- Parameters:
  - `config` (const ClusterConfig &): Cluster configuration (empty = single-node mode).
  - `devices` (const std::vector< DeviceInfo > &): Local GPU device list from DeviceDiscovery::Enumerate().
- Details: Detects intra-node NVLink topology and registers the local node. Safe to call multiple times; re-initialises in place. config Cluster configuration (empty = single-node mode). devices Local GPU device list from DeviceDiscovery::Enumerate().

#### `bool isCoordinator() const noexcept`
- Source: `include/themis/gpu/cluster_coordinator.h`:262
- Brief: n/a
- Parameters: none

#### `bool isInitialized() const noexcept`
- Source: `include/themis/gpu/cluster_coordinator.h`:263
- Brief: n/a
- Parameters: none

#### `void markNodeOffline(const std::string &node_id)`
- Source: `include/themis/gpu/cluster_coordinator.h`:190
- Brief: Immediately mark node_id as OFFLINE.
- Parameters:
  - `node_id` (const std::string &): n/a

#### `size_t onlineNodeCount() const`
- Source: `include/themis/gpu/cluster_coordinator.h`:260
- Brief: n/a
- Parameters: none

#### `void registerNode(const ClusterNode &node, float ib_bw_gbps=0.0f)`
- Source: `include/themis/gpu/cluster_coordinator.h`:138
- Brief: Register a remote peer node (topology-aware path).
- Parameters:
  - `node` (const ClusterNode &): Remote node descriptor.
  - `ib_bw_gbps` (float): Estimated InfiniBand bandwidth (0 = use default).
- Details: Adds the node to the topology and, when InfiniBand is configured, records an estimated inter-node link. node Remote node descriptor. ib_bw_gbps Estimated InfiniBand bandwidth (0 = use default).

#### `void registerNode(const NodeInfo &node)`
- Source: `include/themis/gpu/cluster_coordinator.h`:170
- Brief: Register or update a cluster node (registry path).
- Parameters:
  - `node` (const NodeInfo &): n/a
- Details: If a node with node.id already exists its information is updated; otherwise a new entry is created.

#### `void removeNode(const std::string &node_id)`
- Source: `include/themis/gpu/cluster_coordinator.h`:143
- Brief: Remove a peer node (topology-aware path).
- Parameters:
  - `node_id` (const std::string &): n/a

#### `Placement selectDevice(uint64_t required_vram_bytes=0) const`
- Source: `include/themis/gpu/cluster_coordinator.h`:214
- Brief: Select the best local GPU for a new work item.
- Parameters:
  - `required_vram_bytes` (uint64_t): Minimum free VRAM needed (0 = any).
- Return: Placement with device_index set; node_id is the local node.
- Details: Uses NVLink bandwidth when multiple GPUs are present and config.enable_nvlink is true; otherwise falls back to the first healthy device. required_vram_bytes Minimum free VRAM needed (0 = any). Placement with device_index set; node_id is the local node.

#### `const NodeInfo * selectNode(uint64_t required_vram_bytes=0)`
- Source: `include/themis/gpu/cluster_coordinator.h`:241
- Brief: Select the best online node for the next GPU work item.
- Parameters:
  - `required_vram_bytes` (uint64_t): Minimum free VRAM required (0 = any).
- Return: Pointer to the selected NodeInfo (stable reference into the internal list), or nullptr if no suitable node is found.
- Details: Uses LEAST_LOADED strategy: picks the ONLINE node with the highest free VRAM that satisfies required_vram_bytes. required_vram_bytes Minimum free VRAM required (0 = any). Pointer to the selected NodeInfo (stable reference into the internal list), or nullptr if no suitable node is found.

#### `Placement selectNodeForTransfer(const std::string &src_node_id="") const`
- Source: `include/themis/gpu/cluster_coordinator.h`:225
- Brief: Select the best destination node for a data transfer.
- Parameters:
  - `src_node_id` (const std::string &): Source node (empty = local node).
- Return: Placement with node_id and route filled in.
- Details: Returns the node reachable via the highest-bandwidth InfiniBand link. Falls back to the local node when no inter-node links are registered. src_node_id Source node (empty = local node). Placement with node_id and route filled in.

#### `const GPUClusterTopology & topology() const noexcept`
- Source: `include/themis/gpu/cluster_coordinator.h`:248
- Brief: Return a read-only reference to the current topology snapshot.
- Parameters: none

#### `size_t totalNodes() const`
- Source: `include/themis/gpu/cluster_coordinator.h`:259
- Brief: n/a
- Parameters: none

#### `void updateHeartbeat(const std::string &node_id, uint64_t free_vram_bytes)`
- Source: `include/themis/gpu/cluster_coordinator.h`:184
- Brief: Record a heartbeat from node_id and update its free VRAM.
- Parameters:
  - `node_id` (const std::string &): n/a
  - `free_vram_bytes` (uint64_t): n/a
- Details: Sets the node's status to ONLINE and records the current timestamp.

#### `void updateTopology()`
- Source: `include/themis/gpu/cluster_coordinator.h`:148
- Brief: Rebuild the topology snapshot after node additions/removals.
- Parameters: none

### themis::gpu::GPUClusterTopology

#### `GPUClusterTopology()=default`
- Source: `include/themis/gpu/cluster_topology.h`:123
- Brief: Default-construct an empty topology (zero devices, zero links).
- Parameters: none

#### `void addLink(const TopologyLink &link)`
- Source: `include/themis/gpu/cluster_topology.h`:145
- Brief: Add a directed inter-node InfiniBand link.
- Parameters:
  - `link` (const TopologyLink &): n/a
- Details: Both src and dst node_ids must have been added via addNode() first; if either is unknown the call is silently ignored.

#### `void addNode(const ClusterNode &node)`
- Source: `include/themis/gpu/cluster_topology.h`:132
- Brief: Register a cluster node. Duplicate node_ids are ignored.
- Parameters:
  - `node` (const ClusterNode &): n/a

#### `float bandwidthBetween(int device_a, int device_b) const`
- Source: `include/themis/gpu/cluster_topology.h`:195
- Brief: Return the estimated bandwidth in GB/s between two local GPU device indices. Returns 0 when either index is out of range.
- Parameters:
  - `device_a` (int): n/a
  - `device_b` (int): n/a

#### `std::pair< std::string, std::string > bestInfiniBandPair() const`
- Source: `include/themis/gpu/cluster_topology.h`:189
- Brief: Return the pair of nodes with the highest InfiniBand bandwidth.
- Parameters: none
- Details: Returns {"", ""} when no inter-node InfiniBand links are registered.

#### `std::pair< int, int > bestNVLinkPair() const`
- Source: `include/themis/gpu/cluster_topology.h`:182
- Brief: Return the pair of device indices with the highest NVLink bandwidth. Returns {-1, -1} when no NVLink is present.
- Parameters: none

#### `GPUClusterTopology detect(const std::vector< DeviceInfo > &devices)`
- Source: `include/themis/gpu/cluster_topology.h`:118
- Brief: Detect intra-node GPU topology from an already-enumerated device list (single-node or local-node view of a multi-node cluster).
- Parameters:
  - `devices` (const std::vector< DeviceInfo > &): Output of DeviceDiscovery::Enumerate().
- Return: Topology object with intra-node links filled in.
- Details: Populates the NVLink bandwidth matrix and has_nvlink flag. InfiniBand inter-node links are populated via addNode() / addLink() by the cluster coordinator once remote node information is available. devices Output of DeviceDiscovery::Enumerate(). Topology object with intra-node links filled in.

#### `ClusterNode getNode(const std::string &node_id) const`
- Source: `include/themis/gpu/cluster_topology.h`:212
- Brief: Return the node descriptor for a given id (empty node_id if not found).
- Parameters:
  - `node_id` (const std::string &): n/a

#### `std::vector< std::string > nodeIds() const`
- Source: `include/themis/gpu/cluster_topology.h`:206
- Brief: Return all registered node ids.
- Parameters: none

#### `InterconnectType preferredInterconnect(int device_a, int device_b) const`
- Source: `include/themis/gpu/cluster_topology.h`:201
- Brief: Return the interconnect type used on the highest-bandwidth path between two local device indices.
- Parameters:
  - `device_a` (int): n/a
  - `device_b` (int): n/a

#### `void removeNode(const std::string &node_id)`
- Source: `include/themis/gpu/cluster_topology.h`:137
- Brief: Remove a cluster node by id (also removes its links).
- Parameters:
  - `node_id` (const std::string &): n/a

### themis::gpu::GPUConfig

#### `std::pair< bool, std::string > simulateAllocation(uint64_t bytes, uint64_t current_allocated_bytes) const`
- Source: `include/themis/gpu/config.h`:116
- Brief: Simulate whether an allocation of bytes would be accepted under this config given current_allocated_bytes.
- Parameters:
  - `bytes` (uint64_t): n/a
  - `current_allocated_bytes` (uint64_t): n/a
- Details: Does not touch any real GPU state. Returns a pair of (accepted, reason).

#### `ValidationResult validate() const`
- Source: `include/themis/gpu/config.h`:108
- Brief: Validate all fields for consistency.
- Parameters: none
- Details: Rules enforced pool_slab_size must be > 0 when enable_pool == true oom_warning_threshold must be in (0, 1] circuit_failure_threshold must be >= 1 circuit_success_threshold must be >= 1 circuit_reset_timeout_secs must be > 0 If max_vram_bytes is non-zero, min_free_vram_bytes must be < max_vram_bytes If pool is enabled and pool_num_slabs is 0, max_vram_bytes must be set so it can be derived (warns if not)

### themis::gpu::GPUErrorHandler

#### `std::shared_ptr< GPUErrorHandler > Create()`
- Source: `include/themis/gpu/gpu_error.h`:329
- Brief: Factory: create singleton instance.
- Parameters: none
- Return: Shared pointer to GPUErrorHandler instance
- Details: Shared pointer to GPUErrorHandler instance Behavior: Returns same instance on repeated calls (singleton). Caller should cache result for performance.

#### `GPUErrorHandler()=default`
- Source: `include/themis/gpu/gpu_error.h`:341
- Brief: n/a
- Parameters: none

#### `GPUErrorHandler(const GPUErrorHandler &)=delete`
- Source: `include/themis/gpu/gpu_error.h`:190
- Brief: Non-copyable, non-movable (singleton pattern).
- Parameters:
  - `<unnamed>` (const GPUErrorHandler &): n/a

#### `std::shared_ptr< spdlog::logger > GetLogger() noexcept`
- Source: `include/themis/gpu/gpu_error.h`:338
- Brief: Get logger for GPU error diagnostics.
- Parameters: none
- Return: spdlog logger instance
- Details: spdlog logger instance May be configured at runtime via spdlog factory.

#### `std::string cudaErrorName(cudaError_t cuda_err) const noexcept=0`
- Source: `include/themis/gpu/gpu_error.h`:309
- Brief: Get human-readable string for CUDA error code.
- Parameters:
  - `cuda_err` (cudaError_t): CUDA error code
- Return: Description (e.g., "cudaErrorMemoryAllocation")
- Details: cuda_err CUDA error code Description (e.g., "cudaErrorMemoryAllocation") Wraps cudaGetErrorString() or equivalent.

#### `ErrorRecoveryPolicy defaultPolicy(GPUErrorClass error_class) const noexcept=0`
- Source: `include/themis/gpu/gpu_error.h`:291
- Brief: Log a CUDA error to diagnostics without recovery action.
- Parameters:
  - `error_class` (GPUErrorClass): Error class from taxonomy
- Return: Classified error; kUnknown if unmapped
- Details: cuda_err CUDA error code context Human-readable context (e.g., "cudaMalloc") Behavior: Converts cuda_err to GPUErrorClass, formats message, logs via spdlog. Does not apply recovery policy; caller is responsible. Log a HIP error to diagnostics without recovery action. hip_err HIP error code context Human-readable context (e.g., "hipMalloc") Behavior: Converts hip_err to GPUErrorClass, formats message, logs via spdlog. Does not apply recovery policy. Handle a CUDA error with recovery policy application. cuda_err CUDA error code context Human-readable context policy Override default recovery policy for this error class (optional) Behavior: Classifies error, logs diagnostic, applies recovery policy. May throw on critical errors depending on policy and configuration. Thread-safe. Handle a HIP error with recovery policy application. hip_err HIP error code context Human-readable context policy Override default recovery policy for this error class (optional) Behavior: Classifies error, logs diagnostic, applies recovery policy. May throw on critical errors depending on policy and configuration. Thread-safe. Convert CUDA error code to GPUErrorClass taxonomy. cuda_err CUDA error code Classified error; kUnknown if unmapped Behavior: Pure lookup; no side effects. May be called frequently. Convert HIP error code to GPUErrorClass taxonomy. hip_err HIP error code Classified error; kUnknown if unmapped Behavior: Pure lookup; no side effects. Get default recovery policy for error class. error_class Error class from taxonomy Recovery policy; kFallbackCPU if unmapped Default mappings: kQuotaExceeded → kFallbackCPU kKernelTimeout → kFallbackCPU kBackendUnavailable → kMarkUnavailable + kFallbackCPU kMemoryCommunication → kRetryOnce kNumerical → kEmitWarning kUnsupportedOperation → kFallbackCPU

#### `std::string errorClassName(GPUErrorClass error_class) const noexcept=0`
- Source: `include/themis/gpu/gpu_error.h`:299
- Brief: Get human-readable string for error class.
- Parameters:
  - `error_class` (GPUErrorClass): Error class
- Return: Description (e.g., "kQuotaExceeded")
- Details: error_class Error class Description (e.g., "kQuotaExceeded")

#### `std::string hipErrorName(hipError_t hip_err) const noexcept=0`
- Source: `include/themis/gpu/gpu_error.h`:319
- Brief: Get human-readable string for HIP error code.
- Parameters:
  - `hip_err` (hipError_t): HIP error code
- Return: Description (e.g., "hipErrorOutOfMemory")
- Details: hip_err HIP error code Description (e.g., "hipErrorOutOfMemory") Wraps hipGetErrorName() or equivalent.

#### `GPUErrorHandler & operator=(const GPUErrorHandler &)=delete`
- Source: `include/themis/gpu/gpu_error.h`:191
- Brief: n/a
- Parameters:
  - `<unnamed>` (const GPUErrorHandler &): n/a

#### `~GPUErrorHandler()=default`
- Source: `include/themis/gpu/gpu_error.h`:187
- Brief: n/a
- Parameters: none

### themis::gpu::GPUFeatureFlags

#### `GPUFeatureFlags()`
- Source: `include/themis/gpu/feature_flags.h`:134
- Brief: n/a
- Parameters: none

#### `GPUFeatureFlags & GetInstance()`
- Source: `include/themis/gpu/feature_flags.h`:77
- Brief: n/a
- Parameters: none

#### `void disable(Feature feature)`
- Source: `include/themis/gpu/feature_flags.h`:105
- Brief: Explicitly disable a feature, overriding the edition default.
- Parameters:
  - `feature` (Feature): n/a

#### `bool editionDefaultFor(Feature f)`
- Source: `include/themis/gpu/feature_flags.h`:148
- Brief: n/a
- Parameters:
  - `f` (Feature): n/a

#### `std::string editionName()`
- Source: `include/themis/gpu/feature_flags.h`:131
- Brief: Return the human-readable edition name.
- Parameters: none

#### `void enable(Feature feature)`
- Source: `include/themis/gpu/feature_flags.h`:100
- Brief: Explicitly enable a feature, overriding the edition default.
- Parameters:
  - `feature` (Feature): n/a

#### `const char * featureName(Feature feature) noexcept`
- Source: `include/themis/gpu/feature_flags.h`:91
- Brief: Return the feature name as a string (for logging).
- Parameters:
  - `feature` (Feature): n/a

#### `std::vector< FeatureStatus > getAll() const`
- Source: `include/themis/gpu/feature_flags.h`:126
- Brief: n/a
- Parameters: none

#### `void initDefaults()`
- Source: `include/themis/gpu/feature_flags.h`:147
- Brief: n/a
- Parameters: none

#### `bool isEnabled(Feature feature) const`
- Source: `include/themis/gpu/feature_flags.h`:86
- Brief: Return true if feature is currently enabled.
- Parameters:
  - `feature` (Feature): n/a

#### `int key(Feature f) noexcept`
- Source: `include/themis/gpu/feature_flags.h`:145
- Brief: n/a
- Parameters:
  - `f` (Feature): n/a

#### `void resetToDefaults()`
- Source: `include/themis/gpu/feature_flags.h`:110
- Brief: Reset all overrides; features revert to their edition defaults.
- Parameters: none

### themis::gpu::GPUGraphCache

#### `void capture(const QueryShape &shape)`
- Source: `include/themis/gpu/graph_cache.h`:135
- Brief: Capture (record) a new graph entry for shape.
- Parameters:
  - `shape` (const QueryShape &): n/a
- Details: If an entry for shape already exists its capture_count is incremented (idempotent). Otherwise a new entry is inserted, evicting the LRU entry first if the cache is full.

#### `void clear()`
- Source: `include/themis/gpu/graph_cache.h`:148
- Brief: Remove all entries from the cache.
- Parameters: none

#### `void evictLRU()`
- Source: `include/themis/gpu/graph_cache.h`:154
- Brief: Remove the least-recently-used entry (O(n), n ≤ 32).
- Parameters: none

#### `Stats getStats() const`
- Source: `include/themis/gpu/graph_cache.h`:151
- Brief: n/a
- Parameters: none

#### `void invalidate(const QueryShape &shape)`
- Source: `include/themis/gpu/graph_cache.h`:143
- Brief: Remove the entry for shape, if present.
- Parameters:
  - `shape` (const QueryShape &): n/a
- Details: In a production CUDA build this would also call cudaGraphExecDestroy / cudaGraphDestroy to release device resources.

#### `const GraphEntry * lookup(const QueryShape &shape)`
- Source: `include/themis/gpu/graph_cache.h`:126
- Brief: Look up a shape in the cache.
- Parameters:
  - `shape` (const QueryShape &): n/a
- Details: On a hit, increments the entry's replay counter and last_access stamp, then returns a pointer to the entry (valid until the next mutating call on this cache object). Returns nullptr on a miss.

#### `size_t size() const`
- Source: `include/themis/gpu/graph_cache.h`:150
- Brief: n/a
- Parameters: none

### themis::gpu::GPUKernelValidator

#### `GPUKernelValidator()=default`
- Source: `include/themis/gpu/kernel_validator.h`:165
- Brief: n/a
- Parameters: none

#### `GPUKernelValidator & GetInstance()`
- Source: `include/themis/gpu/kernel_validator.h`:71
- Brief: n/a
- Parameters: none

#### `uint64_t computeChecksum(const std::vector< uint8_t > &data)`
- Source: `include/themis/gpu/kernel_validator.h`:138
- Brief: Compute FNV-1a 64-bit checksum of data.
- Parameters:
  - `data` (const std::vector< uint8_t > &): n/a

#### `uint64_t computeChecksum(const uint8_t *data, size_t length)`
- Source: `include/themis/gpu/kernel_validator.h`:143
- Brief: Compute FNV-1a 64-bit checksum of a raw byte range.
- Parameters:
  - `data` (const uint8_t *): n/a
  - `length` (size_t): n/a

#### `Stats getStats() const`
- Source: `include/themis/gpu/kernel_validator.h`:157
- Brief: n/a
- Parameters: none

#### `bool isRegistered(const std::string &kernel_id) const`
- Source: `include/themis/gpu/kernel_validator.h`:105
- Brief: Return true when kernel_id is whitelisted.
- Parameters:
  - `kernel_id` (const std::string &): n/a

#### `bool isValid(const std::string &kernel_id, const std::vector< uint8_t > &blob) const`
- Source: `include/themis/gpu/kernel_validator.h`:128
- Brief: Convenience method — returns true iff validation succeeds.
- Parameters:
  - `kernel_id` (const std::string &): n/a
  - `blob` (const std::vector< uint8_t > &): n/a

#### `void registerKernel(const std::string &kernel_id, const std::vector< uint8_t > &canonical_blob)`
- Source: `include/themis/gpu/kernel_validator.h`:94
- Brief: Register a kernel by computing its checksum from the blob.
- Parameters:
  - `kernel_id` (const std::string &): Unique identifier (e.g. "vector_dot_fp32").
  - `canonical_blob` (const std::vector< uint8_t > &): Canonical byte sequence for checksum computation.
- Details: kernel_id Unique identifier (e.g. "vector_dot_fp32"). canonical_blob Canonical byte sequence for checksum computation.

#### `void registerKernel(const std::string &kernel_id, uint64_t expected_checksum)`
- Source: `include/themis/gpu/kernel_validator.h`:86
- Brief: Register a kernel with its expected checksum.
- Parameters:
  - `kernel_id` (const std::string &): Unique identifier (e.g. "vector_dot_fp32").
  - `expected_checksum` (uint64_t): FNV-1a 64-bit hash of the canonical blob.
- Details: kernel_id Unique identifier (e.g. "vector_dot_fp32"). expected_checksum FNV-1a 64-bit hash of the canonical blob.

#### `std::vector< std::string > registeredKernels() const`
- Source: `include/themis/gpu/kernel_validator.h`:110
- Brief: Return all registered kernel IDs.
- Parameters: none

#### `void reset()`
- Source: `include/themis/gpu/kernel_validator.h`:162
- Brief: Clear all registered kernels and reset stats (for testing).
- Parameters: none

#### `void unregisterKernel(const std::string &kernel_id)`
- Source: `include/themis/gpu/kernel_validator.h`:100
- Brief: Remove a kernel from the whitelist.
- Parameters:
  - `kernel_id` (const std::string &): n/a

#### `ValidationResult validate(const std::string &kernel_id, const std::vector< uint8_t > &blob) const`
- Source: `include/themis/gpu/kernel_validator.h`:122
- Brief: Validate blob against the registered checksum for kernel_id.
- Parameters:
  - `kernel_id` (const std::string &): n/a
  - `blob` (const std::vector< uint8_t > &): n/a
- Return: ValidationResult describing the outcome.
- Details: ValidationResult describing the outcome.

### themis::gpu::GPULauncher

#### `GPULauncher(BackendFn backend)`
- Source: `include/themis/gpu/launcher.h`:82
- Brief: n/a
- Parameters:
  - `backend` (BackendFn): n/a

#### `WorkResult executeOne(WorkItem item)`
- Source: `include/themis/gpu/launcher.h`:124
- Brief: n/a
- Parameters:
  - `item` (WorkItem): n/a

#### `Stats getStats() const`
- Source: `include/themis/gpu/launcher.h`:117
- Brief: n/a
- Parameters: none

#### `std::future< WorkResult > submit(WorkItem item)`
- Source: `include/themis/gpu/launcher.h`:93
- Brief: Submit a single work item for asynchronous execution.
- Parameters:
  - `item` (WorkItem): n/a
- Return: Future resolving to the WorkResult when execution completes.
- Details: Future resolving to the WorkResult when execution completes.

#### `std::future< std::vector< WorkResult > > submitBatch(std::vector< WorkItem > items)`
- Source: `include/themis/gpu/launcher.h`:103
- Brief: Submit a batch of work items.
- Parameters:
  - `items` (std::vector< WorkItem >): n/a
- Return: Future resolving to one WorkResult per item.
- Details: Items are executed in submission order via std::async. The returned future resolves when all items have completed. Future resolving to one WorkResult per item.

### themis::gpu::GPULoadBalancer

#### `GPULoadBalancer(Strategy strategy, const std::vector< DeviceInfo > &devices)`
- Source: `include/themis/gpu/load_balancer.h`:62
- Brief: Construct and initialise device list immediately.
- Parameters:
  - `strategy` (Strategy): n/a
  - `devices` (const std::vector< DeviceInfo > &): n/a

#### `GPULoadBalancer(Strategy strategy=Strategy::LEAST_LOADED)`
- Source: `include/themis/gpu/load_balancer.h`:57
- Brief: n/a
- Parameters:
  - `strategy` (Strategy): n/a

#### `std::vector< DeviceLoad > getDeviceLoads() const`
- Source: `include/themis/gpu/load_balancer.h`:147
- Brief: n/a
- Parameters: none

#### `size_t healthyDevices() const`
- Source: `include/themis/gpu/load_balancer.h`:131
- Brief: n/a
- Parameters: none

#### `bool isEligible(const DeviceEntry &e, uint64_t required_vram) const`
- Source: `include/themis/gpu/load_balancer.h`:168
- Brief: n/a
- Parameters:
  - `e` (const DeviceEntry &): n/a
  - `required_vram` (uint64_t): n/a

#### `void markDeviceFailed(int device_index, const std::string &reason="")`
- Source: `include/themis/gpu/load_balancer.h`:79
- Brief: Mark a device as failed/unavailable by device index.
- Parameters:
  - `device_index` (int): n/a
  - `reason` (const std::string &): n/a
- Details: The device is removed from selection until resetDevice() is called.

#### `void recordAllocation(int device_index, uint64_t bytes)`
- Source: `include/themis/gpu/load_balancer.h`:119
- Brief: Notify the balancer that bytes were allocated on device_index so it can update internal load tracking.
- Parameters:
  - `device_index` (int): n/a
  - `bytes` (uint64_t): n/a

#### `void recordDeallocation(int device_index, uint64_t bytes)`
- Source: `include/themis/gpu/load_balancer.h`:125
- Brief: Notify the balancer that bytes were freed from device_index.
- Parameters:
  - `device_index` (int): n/a
  - `bytes` (uint64_t): n/a

#### `void resetDevice(int device_index)`
- Source: `include/themis/gpu/load_balancer.h`:84
- Brief: Restore a previously failed device to the eligible pool.
- Parameters:
  - `device_index` (int): n/a

#### `const DeviceInfo * selectDevice(uint64_t required_vram_bytes=0)`
- Source: `include/themis/gpu/load_balancer.h`:113
- Brief: Select the best device for the next work item.
- Parameters:
  - `required_vram_bytes` (uint64_t): Minimum free VRAM needed (0 = any).
- Return: Pointer to selected DeviceInfo (stable reference into the internal list), or nullptr if no eligible device found.
- Details: required_vram_bytes Minimum free VRAM needed (0 = any). Pointer to selected DeviceInfo (stable reference into the internal list), or nullptr if no eligible device found.

#### `DeviceEntry * selectFirstHealthy(uint64_t required_vram)`
- Source: `include/themis/gpu/load_balancer.h`:166
- Brief: n/a
- Parameters:
  - `required_vram` (uint64_t): n/a

#### `DeviceEntry * selectLeastLoaded(uint64_t required_vram)`
- Source: `include/themis/gpu/load_balancer.h`:165
- Brief: n/a
- Parameters:
  - `required_vram` (uint64_t): n/a

#### `DeviceEntry * selectRoundRobin(uint64_t required_vram)`
- Source: `include/themis/gpu/load_balancer.h`:164
- Brief: n/a
- Parameters:
  - `required_vram` (uint64_t): n/a

#### `DeviceEntry * selectTopologyAware(uint64_t required_vram)`
- Source: `include/themis/gpu/load_balancer.h`:167
- Brief: n/a
- Parameters:
  - `required_vram` (uint64_t): n/a

#### `void setTopology(const GPUClusterTopology &topology)`
- Source: `include/themis/gpu/load_balancer.h`:100
- Brief: Supply a topology snapshot for TOPOLOGY_AWARE scheduling.
- Parameters:
  - `topology` (const GPUClusterTopology &): Topology snapshot produced by GPUClusterTopology::detect() or assembled manually for testing.
- Details: When the TOPOLOGY_AWARE strategy is active the balancer picks the device with the highest sum of outgoing NVLink bandwidths according to the provided topology's bandwidth_matrix. If the topology has no NVLink links (or no topology has been set) the strategy falls back to LEAST_LOADED behaviour. Thread safety: acquires the internal mutex. topology Topology snapshot produced by GPUClusterTopology::detect() or assembled manually for testing.

#### `Strategy strategy() const`
- Source: `include/themis/gpu/load_balancer.h`:132
- Brief: n/a
- Parameters: none

#### `size_t totalDevices() const`
- Source: `include/themis/gpu/load_balancer.h`:130
- Brief: n/a
- Parameters: none

#### `void updateDevices(const std::vector< DeviceInfo > &devices)`
- Source: `include/themis/gpu/load_balancer.h`:72
- Brief: Replace the device list (e.g. after re-enumeration).
- Parameters:
  - `devices` (const std::vector< DeviceInfo > &): n/a

### themis::gpu::GPUMemoryManager

#### `void CancelHint(uint64_t hint_id)`
- Source: `include/themis/gpu/memory_manager.h`:203
- Brief: Release a previously reserved hint without allocating.
- Parameters:
  - `hint_id` (uint64_t): n/a
- Details: Safe to call with an invalid (id == 0) handle.

#### `bool ConsumeHint(uint64_t hint_id)`
- Source: `include/themis/gpu/memory_manager.h`:214
- Brief: Convert a hint into a real allocation (atomic swap).
- Parameters:
  - `hint_id` (uint64_t): n/a
- Return: true if the hint was found and converted; false if the hint_id was not found (already consumed or cancelled).
- Details: The reserved bytes remain occupied in the VRAM budget; only their classification changes from "hint" to "active allocation". true if the hint was found and converted; false if the hint_id was not found (already consumed or cancelled).

#### `void DeallocateGPU(uint64_t size_bytes)`
- Source: `include/themis/gpu/memory_manager.h`:154
- Brief: Release size_bytes of previously allocated VRAM.
- Parameters:
  - `size_bytes` (uint64_t): n/a
- Details: Silently clamps to zero if size_bytes exceeds the tracked total to guard against double-free or mis-matched sizes.

#### `void DeallocateGPU(uint64_t size_bytes, const std::string &tenant_id)`
- Source: `include/themis/gpu/memory_manager.h`:161
- Brief: Tenant-aware deallocation.
- Parameters:
  - `size_bytes` (uint64_t): n/a
  - `tenant_id` (const std::string &): n/a
- Details: Decrements both the global counter and the per-tenant counter.

#### `GPUMemoryManager()=default`
- Source: `include/themis/gpu/memory_manager.h`:304
- Brief: n/a
- Parameters: none

#### `GPUMemoryManager(const GPUMemoryManager &)=delete`
- Source: `include/themis/gpu/memory_manager.h`:307
- Brief: n/a
- Parameters:
  - `<unnamed>` (const GPUMemoryManager &): n/a

#### `std::vector< AllocationRecord > GetActiveAllocations() const`
- Source: `include/themis/gpu/memory_manager.h`:279
- Brief: Return a snapshot of all currently live allocation records.
- Parameters: none
- Details: Useful for debugging and leak detection: callers can inspect which tags still hold VRAM after their workload completes.

#### `std::vector< TenantStats > GetAllTenantStats() const`
- Source: `include/themis/gpu/memory_manager.h`:293
- Brief: Return a snapshot of stats for all tenants that have a quota or at least one live allocation.
- Parameters: none

#### `std::string GetEditionInfo() const`
- Source: `include/themis/gpu/memory_manager.h`:271
- Brief: n/a
- Parameters: none

#### `float GetGPUMemoryUsagePercent() const`
- Source: `include/themis/gpu/memory_manager.h`:267
- Brief: n/a
- Parameters: none

#### `uint64_t GetGPUMemoryUsed() const`
- Source: `include/themis/gpu/memory_manager.h`:266
- Brief: n/a
- Parameters: none

#### `uint64_t GetHintReservedBytes() const`
- Source: `include/themis/gpu/memory_manager.h`:217
- Brief: Total bytes currently held by outstanding hints.
- Parameters: none

#### `GPUMemoryManager & GetInstance()`
- Source: `include/themis/gpu/memory_manager.h`:82
- Brief: n/a
- Parameters: none

#### `uint64_t GetMaxGPUVRAMBytes() noexcept`
- Source: `include/themis/gpu/memory_manager.h`:94
- Brief: n/a
- Parameters: none

#### `int GetMaxGPUVRAMGB() noexcept`
- Source: `include/themis/gpu/memory_manager.h`:90
- Brief: n/a
- Parameters: none

#### `Stats GetStats() const`
- Source: `include/themis/gpu/memory_manager.h`:269
- Brief: n/a
- Parameters: none

#### `uint64_t GetTenantHeadroom(const std::string &tenant_id) const`
- Source: `include/themis/gpu/memory_manager.h`:301
- Brief: Return how many bytes the tenant may still allocate.
- Parameters:
  - `tenant_id` (const std::string &): n/a
- Details: Returns the lesser of (global_remaining) and (tenant_quota - tenant_used). If the tenant has no quota registered, only the global limit is considered.

#### `TenantStats GetTenantStats(const std::string &tenant_id) const`
- Source: `include/themis/gpu/memory_manager.h`:287
- Brief: Return stats for a specific tenant.
- Parameters:
  - `tenant_id` (const std::string &): n/a
- Details: Returns a zero-filled TenantStats if the tenant has never allocated or had a quota set.

#### `bool IsGPUAccelerationEnabled() const noexcept`
- Source: `include/themis/gpu/memory_manager.h`:268
- Brief: n/a
- Parameters: none

#### `void RemoveTenantQuota(const std::string &tenant_id)`
- Source: `include/themis/gpu/memory_manager.h`:117
- Brief: Remove the per-tenant VRAM cap (sets quota to 0 / unlimited).
- Parameters:
  - `tenant_id` (const std::string &): n/a
- Details: The tenant entry is kept in the internal map so that usage tracking continues until all its allocations have been freed.

#### `HintHandle ReserveHint(uint64_t bytes, const std::string &tag="hint", const std::string &tenant_id="")`
- Source: `include/themis/gpu/memory_manager.h`:194
- Brief: Reserve bytes of VRAM headroom for a future allocation.
- Parameters:
  - `bytes` (uint64_t): n/a
  - `tag` (const std::string &): n/a
  - `tenant_id` (const std::string &): n/a
- Return: HintHandle with id > 0 on success, id == 0 if the reservation cannot be honoured (limit already exceeded).
- Details: The reserved bytes count against the edition limit and against the tenant quota (if set) so that other callers cannot consume the capacity before the reservation is consumed or cancelled. HintHandle with id > 0 on success, id == 0 if the reservation cannot be honoured (limit already exceeded).

#### `void RollbackAllocationUnderLock(const std::string &tenant_id, uint64_t size_bytes)`
- Source: `include/themis/gpu/memory_manager.h`:349
- Brief: n/a
- Parameters:
  - `tenant_id` (const std::string &): n/a
  - `size_bytes` (uint64_t): n/a

#### `void SetTenantQuota(const std::string &tenant_id, uint64_t quota_bytes)`
- Source: `include/themis/gpu/memory_manager.h`:109
- Brief: Register or update a per-tenant VRAM quota.
- Parameters:
  - `tenant_id` (const std::string &): Opaque tenant identifier.
  - `quota_bytes` (uint64_t): Maximum VRAM this tenant may use concurrently. Pass 0 to remove the per-tenant cap (global limit applies).
- Details: tenant_id Opaque tenant identifier. quota_bytes Maximum VRAM this tenant may use concurrently. Pass 0 to remove the per-tenant cap (global limit applies).

#### `bool TryAllocateGPU(uint64_t size_bytes, const std::string &tag, const std::string &tenant_id)`
- Source: `include/themis/gpu/memory_manager.h`:144
- Brief: Tenant-aware variant of TryAllocateGPU().
- Parameters:
  - `size_bytes` (uint64_t): Bytes to reserve.
  - `tag` (const std::string &): Owner / reason label.
  - `tenant_id` (const std::string &): Tenant identifier; empty string = no tenant check.
- Return: true if allocation was granted, false if either limit was exceeded (without modifying state).
- Details: Checks both the global edition VRAM limit and the per-tenant quota (if one has been set with SetTenantQuota()). size_bytes Bytes to reserve. tag Owner / reason label. tenant_id Tenant identifier; empty string = no tenant check. true if allocation was granted, false if either limit was exceeded (without modifying state).

#### `bool TryAllocateGPU(uint64_t size_bytes, const std::string &tag="Unknown")`
- Source: `include/themis/gpu/memory_manager.h`:130
- Brief: Request VRAM of size_bytes tagged with tag.
- Parameters:
  - `size_bytes` (uint64_t): n/a
  - `tag` (const std::string &): n/a
- Details: Returns true and records the allocation if the request fits within the edition limit. Returns false (without throwing) if it would exceed the limit.

#### `bool TryAllocateUnderLock(uint64_t size_bytes, const std::string &tag, const std::string &tenant_id)`
- Source: `include/themis/gpu/memory_manager.h`:342
- Brief: n/a
- Parameters:
  - `size_bytes` (uint64_t): n/a
  - `tag` (const std::string &): n/a
  - `tenant_id` (const std::string &): n/a

#### `void ValidateAllocation(uint64_t size_bytes)`
- Source: `include/themis/gpu/memory_manager.h`:167
- Brief: Validate a proposed allocation; throws std::runtime_error on rejection instead of returning false.
- Parameters:
  - `size_bytes` (uint64_t): n/a

#### `bool canAllocate(uint64_t size_bytes, const std::string &tenant_id="") const override`
- Source: `include/themis/gpu/memory_manager.h`:230
- Brief: Check if size_bytes can be allocated (IVRAMPolicy contract).
- Parameters:
  - `size_bytes` (uint64_t): n/a
  - `tenant_id` (const std::string &): n/a
- Details: Equivalent to a non-committing TryAllocateGPU() probe: checks the global edition VRAM limit and the per-tenant quota (when tenant_id is non-empty) without modifying any state.

#### `bool isGPUEnabled() const noexcept override`
- Source: `include/themis/gpu/memory_manager.h`:261
- Brief: Return true when the current edition allows GPU acceleration (IVRAMPolicy contract).
- Parameters: none

#### `void onAllocate(uint64_t size_bytes, const std::string &tag, const std::string &tenant_id="") override`
- Source: `include/themis/gpu/memory_manager.h`:240
- Brief: Record a successful allocation (IVRAMPolicy contract).
- Parameters:
  - `size_bytes` (uint64_t): n/a
  - `tag` (const std::string &): n/a
  - `tenant_id` (const std::string &): n/a
- Details: Updates the global and per-tenant accounting as if TryAllocateGPU() had been called. Use this when physical memory is allocated outside the manager but its VRAM footprint must be tracked here.

#### `void onDeallocate(uint64_t size_bytes, const std::string &tenant_id="") override`
- Source: `include/themis/gpu/memory_manager.h`:249
- Brief: Record a deallocation (IVRAMPolicy contract).
- Parameters:
  - `size_bytes` (uint64_t): n/a
  - `tenant_id` (const std::string &): n/a
- Details: Decrements global and per-tenant accounting.

#### `GPUMemoryManager & operator=(const GPUMemoryManager &)=delete`
- Source: `include/themis/gpu/memory_manager.h`:308
- Brief: n/a
- Parameters:
  - `<unnamed>` (const GPUMemoryManager &): n/a

#### `uint64_t usedBytes() const override`
- Source: `include/themis/gpu/memory_manager.h`:255
- Brief: Return currently tracked VRAM usage (IVRAMPolicy contract).
- Parameters: none

#### `~GPUMemoryManager()=default`
- Source: `include/themis/gpu/memory_manager.h`:305
- Brief: n/a
- Parameters: none

### themis::gpu::GPUMemoryPool

#### `GPUMemoryPool(const GPUMemoryPool &)=delete`
- Source: `include/themis/gpu/memory_pool.h`:105
- Brief: n/a
- Parameters:
  - `<unnamed>` (const GPUMemoryPool &): n/a

#### `GPUMemoryPool(uint64_t total_bytes, uint64_t slab_size, size_t num_slabs=0)`
- Source: `include/themis/gpu/memory_pool.h`:101
- Brief: n/a
- Parameters:
  - `total_bytes` (uint64_t): Total VRAM budget managed by this pool.
  - `slab_size` (uint64_t): Size of each slab. All allocations occupy exactly one slab (first-fit). Requests larger than slab_size result in a pool miss.
  - `num_slabs` (size_t): Number of pre-allocated slabs. If 0, computed as total_bytes / slab_size.
- Details: total_bytes Total VRAM budget managed by this pool. slab_size Size of each slab. All allocations occupy exactly one slab (first-fit). Requests larger than slab_size result in a pool miss. num_slabs Number of pre-allocated slabs. If 0, computed as total_bytes / slab_size.

#### `DefragResult defragment(float threshold=0.05f)`
- Source: `include/themis/gpu/memory_pool.h`:189
- Brief: Defragment the pool by compacting occupied slabs toward offset 0.
- Parameters:
  - `threshold` (float): Only run if the current fragmentation ratio exceeds this value (0.0–1.0). Pass 0.0 to always compact.
- Return: DefragResult with before/after metrics, a moved-slab count, and an offset_map (old_offset → new_offset) for each relocated slab so callers can update any raw device pointers they hold.
- Details: Occupied slabs are reassigned contiguous offsets starting at 0 and the internal fragmentation counters are recalculated from the stored per-slab request sizes. Free slabs are assigned the remaining offsets. When a non-zero device base pointer has been set via setDeviceBasePtr(), physical VRAM data is moved via cudaMemcpy (THEMIS_ENABLE_CUDA) or hipMemcpy (THEMIS_ENABLE_HIP) to keep the device memory consistent with the updated logical offsets. In the CPU bookkeeping simulation (no device pointer set) it performs logical compaction only. threshold Only run if the current fragmentation ratio exceeds this value (0.0–1.0). Pass 0.0 to always compact. DefragResult with before/after metrics, a moved-slab count, and an offset_map (old_offset → new_offset) for each relocated slab so callers can update any raw device pointers they hold.

#### `float fragmentation() const`
- Source: `include/themis/gpu/memory_pool.h`:163
- Brief: Fragmentation ratio: (allocated - useful) / total.
- Parameters: none
- Details: Because each request occupies exactly one slab, fragmentation equals the fraction of allocated slab space that exceeds the actual request size. This is always 0 when slab_size == request_size.

#### `size_t freeSlabs() const`
- Source: `include/themis/gpu/memory_pool.h`:154
- Brief: n/a
- Parameters: none

#### `uintptr_t getDeviceBasePtr() const noexcept`
- Source: `include/themis/gpu/memory_pool.h`:203
- Brief: n/a
- Parameters: none

#### `Stats getStats() const`
- Source: `include/themis/gpu/memory_pool.h`:151
- Brief: n/a
- Parameters: none

#### `bool getZeroOnFree() const noexcept`
- Source: `include/themis/gpu/memory_pool.h`:121
- Brief: n/a
- Parameters: none

#### `size_t numSlabs() const`
- Source: `include/themis/gpu/memory_pool.h`:153
- Brief: n/a
- Parameters: none

#### `GPUMemoryPool & operator=(const GPUMemoryPool &)=delete`
- Source: `include/themis/gpu/memory_pool.h`:106
- Brief: n/a
- Parameters:
  - `<unnamed>` (const GPUMemoryPool &): n/a

#### `bool release(uint64_t offset)`
- Source: `include/themis/gpu/memory_pool.h`:146
- Brief: Return a slab to the pool by offset.
- Parameters:
  - `offset` (uint64_t): The offset previously returned by tryAcquire().
- Return: true if the slab was found and freed.
- Details: offset The offset previously returned by tryAcquire(). true if the slab was found and freed.

#### `void setDeviceBasePtr(uintptr_t ptr) noexcept`
- Source: `include/themis/gpu/memory_pool.h`:202
- Brief: Bind a real device base pointer to the pool.
- Parameters:
  - `ptr` (uintptr_t): n/a
- Details: When set (non-zero), defragment() will use cudaMemcpy / hipMemcpy to physically move slab data in device memory. The pool does not own this pointer; the caller is responsible for its lifetime.

#### `void setZeroOnFree(bool z) noexcept`
- Source: `include/themis/gpu/memory_pool.h`:120
- Brief: When enabled, each slab is conceptually zeroed on release.
- Parameters:
  - `z` (bool): n/a
- Details: In a real CUDA implementation this calls cudaMemset(ptr, 0, slab_size) before returning the slab to the free list, preventing one tenant from reading another tenant's data. In the bookkeeping-only simulation the flag increments a counter that is visible in Stats.

#### `size_t slabSize() const`
- Source: `include/themis/gpu/memory_pool.h`:152
- Brief: n/a
- Parameters: none

#### `std::vector< Slab > slabSnapshot() const`
- Source: `include/themis/gpu/memory_pool.h`:168
- Brief: Return a read-only snapshot of the slab table (for diagnostics).
- Parameters: none

#### `bool tryAcquire(uint64_t size_bytes, const std::string &tag, uint64_t &offset)`
- Source: `include/themis/gpu/memory_pool.h`:137
- Brief: Attempt to allocate size_bytes from the pool.
- Parameters:
  - `size_bytes` (uint64_t): Must be ≤ slab_size. Larger requests are pool misses.
  - `tag` (const std::string &): Caller-supplied owner label for diagnostics.
  - `offset` (uint64_t &): Set to the byte offset within the pool when the allocation succeeds.
- Return: true if a free slab was found and marked occupied.
- Details: size_bytes Must be ≤ slab_size. Larger requests are pool misses. tag Caller-supplied owner label for diagnostics. offset Set to the byte offset within the pool when the allocation succeeds. true if a free slab was found and marked occupied.

### themis::gpu::GPUMetrics

#### `GPUMetrics()=default`
- Source: `include/themis/gpu/metrics.h`:182
- Brief: n/a
- Parameters: none

#### `GPUMetrics & GetInstance()`
- Source: `include/themis/gpu/metrics.h`:77
- Brief: n/a
- Parameters: none

#### `std::string buildKey(const std::string &name, const std::unordered_map< std::string, std::string > &labels)`
- Source: `include/themis/gpu/metrics.h`:201
- Brief: n/a
- Parameters:
  - `name` (const std::string &): n/a
  - `labels` (const std::unordered_map< std::string, std::string > &): n/a

#### `void incrCounter(const std::string &name, const std::unordered_map< std::string, std::string > &labels, double delta=1.0)`
- Source: `include/themis/gpu/metrics.h`:194
- Brief: n/a
- Parameters:
  - `name` (const std::string &): n/a
  - `labels` (const std::unordered_map< std::string, std::string > &): n/a
  - `delta` (double): n/a

#### `std::string nsight_export() const`
- Source: `include/themis/gpu/metrics.h`:160
- Brief: Export kernel records in CUDA Nsight Compute-compatible JSON.
- Parameters: none
- Return: UTF-8 JSON string. When no kernels have been recorded the "Kernels" array is present but empty ("Kernels": []).
- Details: Produces a JSON document that mirrors the top-level schema used by Nsight Compute's --export json option so that tooling built around that format (e.g. ncu-ui, custom analysis scripts) can consume the output without modification. UTF-8 JSON string. When no kernels have been recorded the "Kernels" array is present but empty ("Kernels": []).

#### `void recordAllocFailGlobal(uint64_t bytes, const std::string &tenant_id="")`
- Source: `include/themis/gpu/metrics.h`:87
- Brief: n/a
- Parameters:
  - `bytes` (uint64_t): n/a
  - `tenant_id` (const std::string &): n/a

#### `void recordAllocFailTenant(uint64_t bytes, const std::string &tenant_id)`
- Source: `include/themis/gpu/metrics.h`:88
- Brief: n/a
- Parameters:
  - `bytes` (uint64_t): n/a
  - `tenant_id` (const std::string &): n/a

#### `void recordAllocSuccess(uint64_t bytes, const std::string &tenant_id="")`
- Source: `include/themis/gpu/metrics.h`:86
- Brief: n/a
- Parameters:
  - `bytes` (uint64_t): n/a
  - `tenant_id` (const std::string &): n/a

#### `void recordCircuitOpen()`
- Source: `include/themis/gpu/metrics.h`:91
- Brief: n/a
- Parameters: none

#### `void recordDealloc(uint64_t bytes, const std::string &tenant_id="")`
- Source: `include/themis/gpu/metrics.h`:89
- Brief: n/a
- Parameters:
  - `bytes` (uint64_t): n/a
  - `tenant_id` (const std::string &): n/a

#### `void recordFallback(const std::string &reason="oom")`
- Source: `include/themis/gpu/metrics.h`:90
- Brief: n/a
- Parameters:
  - `reason` (const std::string &): n/a

#### `void recordKernelDuration(const KernelRecord &record)`
- Source: `include/themis/gpu/metrics.h`:136
- Brief: Record a GPU kernel execution for Nsight-compatible export.
- Parameters:
  - `record` (const KernelRecord &): Populated KernelRecord (name and duration_ns required).
- Details: Appends one KernelRecord to the internal kernel list and also increments the themis_gpu_kernel_duration_ns gauge so the data is visible in Prometheus snapshots too. record Populated KernelRecord (name and duration_ns required).

#### `void reset()`
- Source: `include/themis/gpu/metrics.h`:179
- Brief: Reset all counters, gauges, and kernel records (for testing).
- Parameters: none

#### `std::string rocm_profiler_export() const`
- Source: `include/themis/gpu/metrics.h`:174
- Brief: Export kernel records in ROCm profiler Chrome trace JSON format.
- Parameters: none
- Return: UTF-8 JSON string. When no kernels have been recorded the "traceEvents" array is present but empty.
- Details: Produces a Chrome trace JSON document compatible with AMD ROCm profiler's --sys-trace output and with Perfetto / chrome://tracing. Each kernel is emitted as a complete event ("ph": "X") so that tooling built around the ROCm profiler JSON schema can consume the output without modification. UTF-8 JSON string. When no kernels have been recorded the "traceEvents" array is present but empty.

#### `void setGauge(const std::string &name, const std::unordered_map< std::string, std::string > &labels, double value)`
- Source: `include/themis/gpu/metrics.h`:197
- Brief: n/a
- Parameters:
  - `name` (const std::string &): n/a
  - `labels` (const std::unordered_map< std::string, std::string > &): n/a
  - `value` (double): n/a

#### `void setPowerDraw(int device_id, double watts)`
- Source: `include/themis/gpu/metrics.h`:117
- Brief: Update the live power-draw gauge for a specific GPU device.
- Parameters:
  - `device_id` (int): CUDA/ROCm device ordinal (0-based).
  - `watts` (double): Current board power draw in watts.
- Details: device_id CUDA/ROCm device ordinal (0-based). watts Current board power draw in watts.

#### `void setPowerLimit(int device_id, double watts)`
- Source: `include/themis/gpu/metrics.h`:125
- Brief: Update the enforced power-limit gauge for a specific GPU device.
- Parameters:
  - `device_id` (int): CUDA/ROCm device ordinal (0-based).
  - `watts` (double): Configured power limit in watts.
- Details: device_id CUDA/ROCm device ordinal (0-based). watts Configured power limit in watts.

#### `void setTemperature(int device_id, double celsius)`
- Source: `include/themis/gpu/metrics.h`:109
- Brief: Update the live temperature gauge for a specific GPU device.
- Parameters:
  - `device_id` (int): CUDA/ROCm device ordinal (0-based).
  - `celsius` (double): Current junction temperature in degrees Celsius.
- Details: device_id CUDA/ROCm device ordinal (0-based). celsius Current junction temperature in degrees Celsius.

#### `void setVRAMAllocated(uint64_t bytes, const std::string &tenant_id="")`
- Source: `include/themis/gpu/metrics.h`:96
- Brief: Update the live VRAM gauge (must be called after each alloc/free).
- Parameters:
  - `bytes` (uint64_t): n/a
  - `tenant_id` (const std::string &): n/a

#### `void setVRAMPeak(uint64_t bytes)`
- Source: `include/themis/gpu/metrics.h`:97
- Brief: n/a
- Parameters:
  - `bytes` (uint64_t): n/a

#### `std::vector< Sample > snapshot() const`
- Source: `include/themis/gpu/metrics.h`:147
- Brief: Return all current metric samples.
- Parameters: none
- Details: Suitable for serialisation into Prometheus text format or OTel OTLP.

### themis::gpu::GPUModule

#### `GPUModule()=default`
- Source: `include/themis/gpu/gpu_module.h`:57
- Brief: n/a
- Parameters: none

#### `GPUModule & GetInstance()`
- Source: `include/themis/gpu/gpu_module.h`:79
- Brief: n/a
- Parameters: none

#### `bool allocate(const std::string &caller_id, const std::string &tenant_id, uint64_t bytes, const std::string &tag="gpu_module")`
- Source: `include/themis/gpu/gpu_module.h`:134
- Brief: Reserve bytes of VRAM for caller_id / tenant_id.
- Parameters:
  - `caller_id` (const std::string &): n/a
  - `tenant_id` (const std::string &): n/a
  - `bytes` (uint64_t): n/a
  - `tag` (const std::string &): n/a
- Return: true if the allocation was granted.
- Details: Checks policy (GPU_ALLOCATE), tenant quota, and edition limit. Records metrics and an audit event on success or failure. true if the allocation was granted.

#### `void deallocate(const std::string &tenant_id, uint64_t bytes)`
- Source: `include/themis/gpu/gpu_module.h`:144
- Brief: Release bytes of VRAM previously granted to tenant_id.
- Parameters:
  - `tenant_id` (const std::string &): n/a
  - `bytes` (uint64_t): n/a
- Details: Records a dealloc metric and audit event.

#### `std::vector< GPUAuditLog::Event > getAuditLog(size_t last_n=64) const`
- Source: `include/themis/gpu/gpu_module.h`:180
- Brief: n/a
- Parameters:
  - `last_n` (size_t): n/a

#### `GPUMemoryManager::Stats getMemoryStats() const`
- Source: `include/themis/gpu/gpu_module.h`:178
- Brief: n/a
- Parameters: none

#### `GPUSafeFail::HealthStatus getSafeFailStatus() const`
- Source: `include/themis/gpu/gpu_module.h`:179
- Brief: n/a
- Parameters: none

#### `void grantCaller(const std::string &caller_id, GPUPolicy::Capability cap=GPUPolicy::Capability::GPU_ANY)`
- Source: `include/themis/gpu/gpu_module.h`:149
- Brief: n/a
- Parameters:
  - `caller_id` (const std::string &): n/a
  - `cap` (GPUPolicy::Capability): n/a

#### `InitResult initialize(const GPUConfig &config, GPULauncher::BackendFn backend=nullptr)`
- Source: `include/themis/gpu/gpu_module.h`:94
- Brief: Validate config and wire up internal components.
- Parameters:
  - `config` (const GPUConfig &): n/a
  - `backend` (GPULauncher::BackendFn): n/a
- Details: Safe to call multiple times; re-initialises the module in place. Returns an error result if the config is invalid.

#### `bool isInitialized() const noexcept`
- Source: `include/themis/gpu/gpu_module.h`:100
- Brief: Return true if the module has been successfully initialised.
- Parameters: none

#### `const MIGManager & mig() const noexcept`
- Source: `include/themis/gpu/gpu_module.h`:173
- Brief: n/a
- Parameters: none

#### `MIGManager & mig() noexcept`
- Source: `include/themis/gpu/gpu_module.h`:172
- Brief: Access the process-wide MIG partition manager.
- Parameters: none
- Details: Returns the MIGManager singleton so callers can create/destroy MIG partitions, assign them to tenants, and query their status without importing mig_manager.h directly. The MIG_MANAGER feature flag is enforced inside MIGManager: mutating operations (createPartition, destroyPartition) return Status::MIG_FEATURE_DISABLED when the flag is off; read-only queries (getInstances, getInstance, etc.) are always permitted. Callers can check GPUFeatureFlags::GetInstance().isEnabled( GPUFeatureFlags::Feature::MIG_MANAGER) before calling mutating operations if they want to short-circuit early.

#### `void revokeCaller(const std::string &caller_id)`
- Source: `include/themis/gpu/gpu_module.h`:151
- Brief: n/a
- Parameters:
  - `caller_id` (const std::string &): n/a

#### `SubmitResult submitWork(const std::string &caller_id, const std::string &tenant_id, const GPULauncher::WorkItem &item)`
- Source: `include/themis/gpu/gpu_module.h`:118
- Brief: Submit a GPU work item.
- Parameters:
  - `caller_id` (const std::string &): Identity of the submitting component (policy check).
  - `tenant_id` (const std::string &): Tenant for VRAM quota enforcement.
  - `item` (const GPULauncher::WorkItem &): Work item (kernel_id, args, timeout).
- Return: SubmitResult indicating success/failure and whether GPU was used.
- Details: Checks policy, circuit breaker, and VRAM availability before dispatching to the launcher backend. Records metrics and audit event regardless of the outcome. caller_id Identity of the submitting component (policy check). tenant_id Tenant for VRAM quota enforcement. item Work item (kernel_id, args, timeout). SubmitResult indicating success/failure and whether GPU was used.

### themis::gpu::GPUP2PTransferManager

#### `GPUP2PTransferManager()=default`
- Source: `include/themis/gpu/p2p_transfer.h`:124
- Brief: n/a
- Parameters: none

#### `GPUP2PTransferManager(const GPUP2PTransferManager &)=delete`
- Source: `include/themis/gpu/p2p_transfer.h`:121
- Brief: n/a
- Parameters:
  - `<unnamed>` (const GPUP2PTransferManager &): n/a

#### `GPUP2PTransferManager & GetInstance()`
- Source: `include/themis/gpu/p2p_transfer.h`:115
- Brief: n/a
- Parameters: none

#### `bool canAccessPeer(int src_device, int dst_device, const std::vector< DeviceInfo > &devices={}) const`
- Source: `include/themis/gpu/p2p_transfer.h`:146
- Brief: Check whether direct P2P access is possible between two devices.
- Parameters:
  - `src_device` (int): Source GPU device index.
  - `dst_device` (int): Destination GPU device index.
  - `devices` (const std::vector< DeviceInfo > &): Device list from DeviceDiscovery::Enumerate(). Pass an empty vector to use the live driver query.
- Return: true when direct P2P access is supported.
- Details: Does NOT require the PEER_TO_PEER feature flag; always callable. On CUDA: calls cudaDeviceCanAccessPeer. On HIP: calls hipDeviceCanAccessPeer. CPU fallback: returns false (no hardware P2P available). src_device Source GPU device index. dst_device Destination GPU device index. devices Device list from DeviceDiscovery::Enumerate(). Pass an empty vector to use the live driver query. true when direct P2P access is supported.

#### `Status disablePeerAccess(int src_device, int dst_device)`
- Source: `include/themis/gpu/p2p_transfer.h`:175
- Brief: Disable direct peer access from src_device to dst_device.
- Parameters:
  - `src_device` (int): Source GPU device index.
  - `dst_device` (int): Destination GPU device index.
- Return: Status::OK or an error code.
- Details: Requires PEER_TO_PEER feature flag. Peer access must currently be enabled for this pair. src_device Source GPU device index. dst_device Destination GPU device index. Status::OK or an error code.

#### `Status enablePeerAccess(int src_device, int dst_device, const std::vector< DeviceInfo > &devices={})`
- Source: `include/themis/gpu/p2p_transfer.h`:162
- Brief: Enable direct peer access from src_device to dst_device.
- Parameters:
  - `src_device` (int): Source GPU device index.
  - `dst_device` (int): Destination GPU device index.
  - `devices` (const std::vector< DeviceInfo > &): Device list for capability checks.
- Return: Status::OK or an error code.
- Details: Requires PEER_TO_PEER feature flag. Peer access must be supported (see canAccessPeer) and must not have been enabled already for this pair. On the CPU simulation path this always returns PEER_ACCESS_NOT_SUPPORTED. src_device Source GPU device index. dst_device Destination GPU device index. devices Device list for capability checks. Status::OK or an error code.

#### `Stats getStats() const`
- Source: `include/themis/gpu/p2p_transfer.h`:217
- Brief: Return current transfer statistics.
- Parameters: none

#### `bool isPeerAccessEnabled(int src_device, int dst_device) const`
- Source: `include/themis/gpu/p2p_transfer.h`:181
- Brief: Return true when peer access has been enabled for the pair (src_device → dst_device).
- Parameters:
  - `src_device` (int): n/a
  - `dst_device` (int): n/a

#### `GPUP2PTransferManager & operator=(const GPUP2PTransferManager &)=delete`
- Source: `include/themis/gpu/p2p_transfer.h`:122
- Brief: n/a
- Parameters:
  - `<unnamed>` (const GPUP2PTransferManager &): n/a

#### `uint32_t pairKey(int src, int dst) noexcept`
- Source: `include/themis/gpu/p2p_transfer.h`:240
- Brief: n/a
- Parameters:
  - `src` (int): n/a
  - `dst` (int): n/a

#### `void reset()`
- Source: `include/themis/gpu/p2p_transfer.h`:222
- Brief: Reset statistics and remove all enabled peer-access records.
- Parameters: none

#### `TransferResult transfer(const TransferRequest &request, const std::vector< DeviceInfo > &devices={})`
- Source: `include/themis/gpu/p2p_transfer.h`:207
- Brief: Perform a direct GPU-to-GPU memory transfer.
- Parameters:
  - `request` (const TransferRequest &): Describes source, destination, pointers, and size.
  - `devices` (const std::vector< DeviceInfo > &): Device list for topology decisions. Pass an empty vector to use DeviceDiscovery::Enumerate().
- Return: TransferResult with ok==true on success.
- Details: Requires PEER_TO_PEER feature flag. Transfer routing (best available path, in priority order): NVLink — when a NVLink link is detected between the two devices. PCIe P2P — when peer access is enabled and NVLink is not present. CPU fallback — when no hardware P2P is available; the operation succeeds via a host-side memcpy simulation so that tests can exercise the full transfer pipeline without GPU hardware. On CUDA the actual transfer is cudaMemcpyPeer(dst, dst_dev, src, src_dev, size). On HIP it is hipMemcpyPeer. request Describes source, destination, pointers, and size. devices Device list for topology decisions. Pass an empty vector to use DeviceDiscovery::Enumerate(). TransferResult with ok==true on success.

#### `~GPUP2PTransferManager()=default`
- Source: `include/themis/gpu/p2p_transfer.h`:125
- Brief: n/a
- Parameters: none

### themis::gpu::GPUPolicy

#### `GPUPolicy()=default`
- Source: `include/themis/gpu/policy.h`:63
- Brief: n/a
- Parameters: none

#### `GPUPolicy(const std::vector< std::string > &pre_granted_callers)`
- Source: `include/themis/gpu/policy.h`:68
- Brief: Construct with a set of pre-granted callers (for tests / bootstrap).
- Parameters:
  - `pre_granted_callers` (const std::vector< std::string > &): n/a

#### `int cap_to_int(Capability c)`
- Source: `include/themis/gpu/policy.h`:138
- Brief: n/a
- Parameters:
  - `c` (Capability): n/a

#### `std::vector< Capability > capabilitiesOf(const std::string &caller_id) const`
- Source: `include/themis/gpu/policy.h`:124
- Brief: List capabilities held by caller_id.
- Parameters:
  - `caller_id` (const std::string &): n/a

#### `PolicyDecision check(const std::string &caller_id, Capability cap=Capability::GPU_ALLOCATE) const`
- Source: `include/themis/gpu/policy.h`:103
- Brief: Check whether caller_id holds cap.
- Parameters:
  - `caller_id` (const std::string &): n/a
  - `cap` (Capability): n/a
- Details: Default-deny: returns denied decision if the caller is unknown or if the specific capability has not been granted.

#### `void grant(const std::string &caller_id, Capability cap=Capability::GPU_ANY)`
- Source: `include/themis/gpu/policy.h`:79
- Brief: Grant a specific capability to caller_id.
- Parameters:
  - `caller_id` (const std::string &): n/a
  - `cap` (Capability): n/a
- Details: If cap is GPU_ANY, all capabilities are granted.

#### `std::vector< std::string > grantedCallers() const`
- Source: `include/themis/gpu/policy.h`:119
- Brief: List all caller IDs that currently hold at least one capability.
- Parameters: none

#### `size_t grantedCount() const`
- Source: `include/themis/gpu/policy.h`:129
- Brief: Return the number of callers with at least one capability.
- Parameters: none

#### `bool hasCapability(const std::string &id, Capability cap) const`
- Source: `include/themis/gpu/policy.h`:140
- Brief: n/a
- Parameters:
  - `id` (const std::string &): n/a
  - `cap` (Capability): n/a

#### `bool isAllowed(const std::string &caller_id, Capability cap=Capability::GPU_ALLOCATE) const`
- Source: `include/themis/gpu/policy.h`:109
- Brief: Returns true if caller_id is allowed cap (convenience wrapper).
- Parameters:
  - `caller_id` (const std::string &): n/a
  - `cap` (Capability): n/a

#### `void revoke(const std::string &caller_id, Capability cap=Capability::GPU_ANY)`
- Source: `include/themis/gpu/policy.h`:86
- Brief: Revoke a specific capability from caller_id.
- Parameters:
  - `caller_id` (const std::string &): n/a
  - `cap` (Capability): n/a
- Details: If cap is GPU_ANY, all capabilities are revoked.

#### `void revokeAll(const std::string &caller_id)`
- Source: `include/themis/gpu/policy.h`:91
- Brief: Revoke all capabilities from caller_id and remove the entry.
- Parameters:
  - `caller_id` (const std::string &): n/a

### themis::gpu::GPUProfiler

#### `GPUProfiler()=default`
- Source: `include/themis/gpu/profiler.h`:153
- Brief: n/a
- Parameters: none

#### `GPUProfiler & GetInstance()`
- Source: `include/themis/gpu/profiler.h`:83
- Brief: n/a
- Parameters: none

#### `void beginRange(const std::string &name, uint32_t argb_color=0xFF00FF00)`
- Source: `include/themis/gpu/profiler.h`:103
- Brief: Push a named profiling range.
- Parameters:
  - `name` (const std::string &): Human-readable range label (shown in profiler UI).
  - `argb_color` (uint32_t): ARGB colour for the range in the Nsight/rocTX UI. Default: opaque green (0xFF00FF00).
- Details: On CUDA builds emits an NVTX nvtxRangePushEx with the given ARGB color. On HIP builds emits a roctxRangePushA. On CPU-only builds records the start timestamp internally. name Human-readable range label (shown in profiler UI). argb_color ARGB colour for the range in the Nsight/rocTX UI. Default: opaque green (0xFF00FF00).

#### `void endRange()`
- Source: `include/themis/gpu/profiler.h`:112
- Brief: Pop the most recently pushed range.
- Parameters: none
- Details: Emits nvtxRangePop / roctxRangePop on hardware builds and records the completed range internally. A call without a matching beginRange is silently ignored.

#### `std::vector< Range > getRanges() const`
- Source: `include/themis/gpu/profiler.h`:143
- Brief: Return a copy of all completed ranges.
- Parameters: none

#### `void markEvent(const std::string &name)`
- Source: `include/themis/gpu/profiler.h`:122
- Brief: Record a point event (instant marker).
- Parameters:
  - `name` (const std::string &): Event label.
- Details: Emits nvtxMarkA / roctxMarkA on hardware builds. Stored internally as a zero-duration range (start_ns == end_ns). name Event label.

#### `uint64_t nowNs()`
- Source: `include/themis/gpu/profiler.h`:166
- Brief: n/a
- Parameters: none

#### `void reset()`
- Source: `include/themis/gpu/profiler.h`:150
- Brief: Reset all recorded ranges (for testing).
- Parameters: none
- Details: Does not affect any in-flight hardware profiling sessions.

#### `std::string rocm_profiler_export() const`
- Source: `include/themis/gpu/profiler.h`:138
- Brief: Export completed ranges in Chrome trace JSON format.
- Parameters: none
- Return: UTF-8 JSON string. The "traceEvents" array is present but empty when no ranges have been recorded.
- Details: The output is compatible with: AMD ROCm profiler's --sys-trace JSON output format Perfetto / chrome://tracing for offline analysis UTF-8 JSON string. The "traceEvents" array is present but empty when no ranges have been recorded.

### themis::gpu::GPUQueryAccelerator

#### `GPUQueryAccelerator()`
- Source: `include/themis/gpu/query_accelerator.h`:209
- Brief: n/a
- Parameters: none

#### `GPUQueryAccelerator(const Config &config)`
- Source: `include/themis/gpu/query_accelerator.h`:210
- Brief: n/a
- Parameters:
  - `config` (const Config &): n/a

#### `AggResult aggregate(const std::vector< Row > &rows, AggFunc func, KeyFn value_fn)`
- Source: `include/themis/gpu/query_accelerator.h`:240
- Brief: Compute an aggregate over rows using value_fn.
- Parameters:
  - `rows` (const std::vector< Row > &): n/a
  - `func` (AggFunc): n/a
  - `value_fn` (KeyFn): n/a
- Details: GPU path would use a reduction kernel; CPU path is a sequential pass.

#### `AnnResult annSearch(const std::vector< float > &queries, size_t numQueries, size_t dim, const std::vector< float > &database, size_t numVectors, size_t k, bool useL2=true)`
- Source: `include/themis/gpu/query_accelerator.h`:310
- Brief: Approximate k-nearest-neighbor vector similarity search.
- Parameters:
  - `queries` (const std::vector< float > &): Flat float array of numQueries × dim elements.
  - `numQueries` (size_t): Number of query vectors.
  - `dim` (size_t): Vector dimensionality; must match for queries and database.
  - `database` (const std::vector< float > &): Flat float array of numVectors × dim elements.
  - `numVectors` (size_t): Number of database vectors.
  - `k` (size_t): Number of nearest neighbors to return per query.
  - `useL2` (bool): If true use L2 distance; if false use inner-product distance.
- Return: AnnResult where results[i] holds the k nearest neighbors for query i, sorted ascending by distance. Returns empty results if inputs are invalid (dim=0, k=0, etc.).
- Details: Searches database (a flat array of numVectors vectors each of length dim) for the k nearest neighbors of each query in queries (a flat array of numQueries vectors each of length dim). Distance metricWhen useL2 is true (default), squared Euclidean (L2) distance is used. When false, negative inner product is used as a distance metric (lower value = higher similarity). For unit-normalized vectors this is equivalent to cosine distance; for unnormalized vectors callers performing maximum inner product search (MIPS) should normalize their vectors beforehand. GPU path (when THEMIS_ENABLE_CUDA is defined and vector count ≥ Config::gpu_threshold_rows) — stub for production cuVS/RAFT wiring: Allocate device memory and copy database + queries. Build an IVF-Flat index: cuvs::neighbors::ivf_flat::build() Search: cuvs::neighbors::ivf_flat::search() Copy results back to host and populate AnnResult. CPU fallback — brute-force exact k-NN using a max-heap per query. queries Flat float array of numQueries × dim elements. numQueries Number of query vectors. dim Vector dimensionality; must match for queries and database. database Flat float array of numVectors × dim elements. numVectors Number of database vectors. k Number of nearest neighbors to return per query. useL2 If true use L2 distance; if false use inner-product distance. AnnResult where results[i] holds the k nearest neighbors for query i, sorted ascending by distance. Returns empty results if inputs are invalid (dim=0, k=0, etc.).

#### `void disableGraphCache()`
- Source: `include/themis/gpu/query_accelerator.h`:367
- Brief: Disable CUDA graph capture. The existing cache is preserved but will not be consulted until re-enabled.
- Parameters: none

#### `DotProductResult dotProduct(const std::vector< float > &a, const std::vector< float > &b)`
- Source: `include/themis/gpu/query_accelerator.h`:272
- Brief: Compute the dot product of two float vectors using the configured precision mode (FP32, FP16, or BF16).
- Parameters:
  - `a` (const std::vector< float > &): First operand; must be the same length as b.
  - `b` (const std::vector< float > &): Second operand.
- Return: DotProductResult with value and the precision actually used. Returns 0.0 on empty or size-mismatch input.
- Details: In FP16/BF16 modes inputs are first quantised to the target precision then de-quantised back to float before accumulation, simulating the precision loss of Tensor Core operations on hardware that does not accumulate FP32 internally. On real hardware this call would be replaced by a cuBLAS cublasSgemv (FP32), cublasHgemm (FP16), or cublasGemmEx with CUBLAS_COMPUTE_16F / CUDA_R_16BF (BF16). a First operand; must be the same length as b. b Second operand. DotProductResult with value and the precision actually used. Returns 0.0 on empty or size-mismatch input.

#### `void enableGraphCache()`
- Source: `include/themis/gpu/query_accelerator.h`:361
- Brief: Enable CUDA graph capture for recurring query patterns.
- Parameters: none
- Details: When enabled, each operation checks the graph cache before executing. On a cache miss the shape is captured; on a hit the cached graph is replayed and the graph_cache_hits stat is incremented. In a production CUDA build, replaying a cached graph eliminates per-launch kernel-setup overhead via cudaGraphLaunch.

#### `GPUGraphCache::Stats getGraphCacheStats() const`
- Source: `include/themis/gpu/query_accelerator.h`:372
- Brief: Return statistics from the underlying GPUGraphCache.
- Parameters: none

#### `Stats getStats() const`
- Source: `include/themis/gpu/query_accelerator.h`:344
- Brief: n/a
- Parameters: none

#### `JoinResult hashJoin(const std::vector< Row > &left, const std::vector< Row > &right, JoinKeyFn left_key, JoinKeyFn right_key)`
- Source: `include/themis/gpu/query_accelerator.h`:251
- Brief: Hash join left and right on matching join keys.
- Parameters:
  - `left` (const std::vector< Row > &): n/a
  - `right` (const std::vector< Row > &): n/a
  - `left_key` (JoinKeyFn): n/a
  - `right_key` (JoinKeyFn): n/a
- Details: Builds a hash table on the smaller side then probes with the larger side. GPU path would use a parallel hash join; CPU path uses std::unordered_multimap.

#### `QueryShape makeShape(QueryShape::OpType op, size_t row_count, uint64_t param_hash=0) noexcept`
- Source: `include/themis/gpu/query_accelerator.h`:385
- Brief: Build a QueryShape for a single-sided operation.
- Parameters:
  - `op` (QueryShape::OpType): n/a
  - `row_count` (size_t): n/a
  - `param_hash` (uint64_t): n/a

#### `void recordOp(size_t rows, uint64_t bytes, bool gpu_used)`
- Source: `include/themis/gpu/query_accelerator.h`:382
- Brief: n/a
- Parameters:
  - `rows` (size_t): n/a
  - `bytes` (uint64_t): n/a
  - `gpu_used` (bool): n/a

#### `void resetStats()`
- Source: `include/themis/gpu/query_accelerator.h`:345
- Brief: n/a
- Parameters: none

#### `ScanResult scan(const std::vector< Row > &rows, FilterFn filter=nullptr)`
- Source: `include/themis/gpu/query_accelerator.h`:222
- Brief: Parallel row scan with optional filter predicate.
- Parameters:
  - `rows` (const std::vector< Row > &): n/a
  - `filter` (FilterFn): n/a
- Details: When filter is nullptr every row passes. GPU path would use a Thrust/cub parallel select; CPU path is a sequential scan.

#### `bool shouldUseGPU(size_t num_rows) const noexcept`
- Source: `include/themis/gpu/query_accelerator.h`:381
- Brief: n/a
- Parameters:
  - `num_rows` (size_t): n/a

#### `SortResult sort(std::vector< Row > rows, KeyFn key_fn, SortOrder order=SortOrder::ASC)`
- Source: `include/themis/gpu/query_accelerator.h`:231
- Brief: Sort rows by key_fn in order.
- Parameters:
  - `rows` (std::vector< Row >): n/a
  - `key_fn` (KeyFn): n/a
  - `order` (SortOrder): n/a
- Details: GPU path would use Thrust sort; CPU path uses std::stable_sort. Rows with equal keys retain their original relative order.

#### `TopKResult topK(std::vector< Row > rows, KeyFn key_fn, size_t k, SortOrder order=SortOrder::ASC)`
- Source: `include/themis/gpu/query_accelerator.h`:336
- Brief: GPU-accelerated partial sort — return the top k rows by key_fn in order without fully sorting the input.
- Parameters:
  - `rows` (std::vector< Row >): Input row set.
  - `key_fn` (KeyFn): Numeric key extractor (host callable).
  - `k` (size_t): Maximum number of rows to return.
  - `order` (SortOrder): ASC returns the k rows with the smallest keys; DESC returns the k rows with the largest keys.
- Return: TopKResult with at most k rows sorted in order.
- Details: Uses thrust::partial_sort_copy on GPU (CUDA/HIP) and std::partial_sort on the CPU fallback path. When k >= rows.size() the result is equivalent to a full sort. Fail-closed: any GPU allocation failure or SLA timeout causes an immediate CPU fallback; used_gpu reflects the path actually taken. rows Input row set. key_fn Numeric key extractor (host callable). k Maximum number of rows to return. order ASC returns the k rows with the smallest keys; DESC returns the k rows with the largest keys. TopKResult with at most k rows sorted in order.

### themis::gpu::GPUSafeFail

#### `GPUSafeFail()=default`
- Source: `include/themis/gpu/safe_fail.h`:97
- Brief: n/a
- Parameters: none

#### `GPUSafeFail(const Config &cfg)`
- Source: `include/themis/gpu/safe_fail.h`:98
- Brief: n/a
- Parameters:
  - `cfg` (const Config &): n/a

#### `void applyFailure()`
- Source: `include/themis/gpu/safe_fail.h`:189
- Brief: n/a
- Parameters: none

#### `void applySuccess()`
- Source: `include/themis/gpu/safe_fail.h`:190
- Brief: n/a
- Parameters: none

#### `bool canResetCircuit() const`
- Source: `include/themis/gpu/safe_fail.h`:147
- Brief: n/a
- Parameters: none

#### `bool checkMemoryAvailable(uint64_t required_bytes, uint64_t available_bytes, uint64_t total_bytes=0) const`
- Source: `include/themis/gpu/safe_fail.h`:166
- Brief: Check whether required_bytes can be safely allocated given available_bytes.
- Parameters:
  - `required_bytes` (uint64_t): n/a
  - `available_bytes` (uint64_t): n/a
  - `total_bytes` (uint64_t): n/a
- Details: Returns false when available < required, or when available falls below the configured OOM threshold of total.

#### `bool executeWithFallback(std::function< bool()> gpu_op, std::function< bool()> cpu_fallback, const std::string &op_name="")`
- Source: `include/themis/gpu/safe_fail.h`:127
- Brief: Execute a GPU operation with automatic CPU fallback.
- Parameters:
  - `gpu_op` (std::function< bool()>): GPU operation; returns true on success.
  - `cpu_fallback` (std::function< bool()>): CPU fallback; may be nullptr (operation fails if no fallback and GPU is unavailable).
  - `op_name` (const std::string &): Reserved for structured logging / audit trail; currently stored in status for future use.
- Return: true if the overall operation succeeded (via GPU or CPU).
- Details: If the circuit is open (or GPU is in FAILED state) cpu_fallback is called directly without attempting gpu_op. Otherwise gpu_op is called; on failure (returns false or throws) the circuit state is updated and cpu_fallback is invoked. gpu_op GPU operation; returns true on success. cpu_fallback CPU fallback; may be nullptr (operation fails if no fallback and GPU is unavailable). op_name Reserved for structured logging / audit trail; currently stored in status for future use. true if the overall operation succeeded (via GPU or CPU).

#### `void forceFailed(const std::string &reason="")`
- Source: `include/themis/gpu/safe_fail.h`:150
- Brief: n/a
- Parameters:
  - `reason` (const std::string &): n/a

#### `void forceHealthy()`
- Source: `include/themis/gpu/safe_fail.h`:149
- Brief: n/a
- Parameters: none

#### `float getErrorRate() const`
- Source: `include/themis/gpu/safe_fail.h`:157
- Brief: n/a
- Parameters: none

#### `HealthStatus getStatus() const`
- Source: `include/themis/gpu/safe_fail.h`:156
- Brief: n/a
- Parameters: none

#### `bool isHealthy() const`
- Source: `include/themis/gpu/safe_fail.h`:155
- Brief: n/a
- Parameters: none

#### `void recordFailure(FailureType type, const std::string &msg="")`
- Source: `include/themis/gpu/safe_fail.h`:136
- Brief: Record a manual GPU failure.
- Parameters:
  - `type` (FailureType): n/a
  - `msg` (const std::string &): n/a
- Details: Useful when the caller detects a failure outside of executeWithFallback.

#### `void recordSuccess()`
- Source: `include/themis/gpu/safe_fail.h`:141
- Brief: Record a manual GPU success.
- Parameters: none

#### `void reset(const Config &cfg)`
- Source: `include/themis/gpu/safe_fail.h`:106
- Brief: Reconfigure and reset internal state.
- Parameters:
  - `cfg` (const Config &): n/a
- Details: Useful for module re-initialization when the type is non-assignable.

#### `bool shouldAttemptGPU() const`
- Source: `include/themis/gpu/safe_fail.h`:146
- Brief: n/a
- Parameters: none

#### `void tryResetCircuit()`
- Source: `include/themis/gpu/safe_fail.h`:148
- Brief: n/a
- Parameters: none

#### `~GPUSafeFail()=default`
- Source: `include/themis/gpu/safe_fail.h`:99
- Brief: n/a
- Parameters: none

### themis::gpu::GPUStreamManager

#### `GPUStreamManager()=default`
- Source: `include/themis/gpu/stream_manager.h`:105
- Brief: Default constructor.
- Parameters: none
- Details: Allows constructing local instances for testing and non-singleton use. Use GetInstance() for the process-wide singleton.

#### `GPUStreamManager & GetInstance()`
- Source: `include/themis/gpu/stream_manager.h`:111
- Brief: n/a
- Parameters: none

#### `bool createCudaStream(const StreamConfig &cfg, int device_index=0)`
- Source: `include/themis/gpu/stream_manager.h`:149
- Brief: Create a new named CUDA stream on device_index.
- Parameters:
  - `cfg` (const StreamConfig &): Stream configuration (name must be non-empty).
  - `device_index` (int): CUDA device ordinal (0-based).
- Return: false if a stream with that name already exists or cfg.name is empty.
- Details: Wires a CUDA-backed execution path into the stream when THEMIS_ENABLE_CUDA is defined; falls back to the ROCm/CPU backend when CUDA is unavailable so the call is always safe to use. This overload resolves the "Stubs: 1" noted in the stream_manager header by providing a first-class CUDA stream creation path alongside the existing ROCm path. cfg Stream configuration (name must be non-empty). device_index CUDA device ordinal (0-based). false if a stream with that name already exists or cfg.name is empty.

#### `bool createStream(const StreamConfig &cfg, GPULauncher::BackendFn backend=nullptr)`
- Source: `include/themis/gpu/stream_manager.h`:131
- Brief: Create a new named stream.
- Parameters:
  - `cfg` (const StreamConfig &): Stream configuration.
  - `backend` (GPULauncher::BackendFn): GPU execution backend. Pass nullptr to use the ROCm backend (which creates a named HIP stream and transparently falls back to CPU execution when THEMIS_ENABLE_HIP is absent). When THEMIS_ENABLE_CUDA is also active a cudaStream_t is created alongside and stored for future kernel dispatch.
- Return: false if a stream with that name already exists.
- Details: cfg Stream configuration. backend GPU execution backend. Pass nullptr to use the ROCm backend (which creates a named HIP stream and transparently falls back to CPU execution when THEMIS_ENABLE_HIP is absent). When THEMIS_ENABLE_CUDA is also active a cudaStream_t is created alongside and stored for future kernel dispatch. false if a stream with that name already exists.

#### `bool destroyStream(const std::string &name)`
- Source: `include/themis/gpu/stream_manager.h`:156
- Brief: Destroy a named stream.
- Parameters:
  - `name` (const std::string &): n/a
- Return: false if no stream with that name exists.
- Details: false if no stream with that name exists.

#### `std::vector< StreamStats > getAllStreamStats() const`
- Source: `include/themis/gpu/stream_manager.h`:190
- Brief: Return stats for all registered streams.
- Parameters: none

#### `StreamStats getStreamStats(const std::string &name) const`
- Source: `include/themis/gpu/stream_manager.h`:185
- Brief: Return stats for a single stream.
- Parameters:
  - `name` (const std::string &): n/a
- Details: Returns a zero-filled StreamStats (with the given name) if the stream does not exist.

#### `bool hasStream(const std::string &name) const`
- Source: `include/themis/gpu/stream_manager.h`:158
- Brief: n/a
- Parameters:
  - `name` (const std::string &): n/a

#### `void setCudaStreamBackendFn(CudaStreamBackendFn fn)`
- Source: `include/themis/gpu/stream_manager.h`:221
- Brief: n/a
- Parameters:
  - `fn` (CudaStreamBackendFn): n/a
- Details: Register a CUDA backend factory used by createCudaStream() when THEMIS_ENABLE_CUDA is not defined. Pass an empty std::function to clear and revert to the ROCm/CPU fallback. Thread-safe (guarded by a static mutex).

#### `size_t streamCount() const`
- Source: `include/themis/gpu/stream_manager.h`:160
- Brief: n/a
- Parameters: none

#### `std::vector< std::string > streamNames() const`
- Source: `include/themis/gpu/stream_manager.h`:159
- Brief: n/a
- Parameters: none

#### `std::future< GPULauncher::WorkResult > submit(const std::string &stream_name, GPULauncher::WorkItem item)`
- Source: `include/themis/gpu/stream_manager.h`:172
- Brief: Submit a work item to the named stream.
- Parameters:
  - `stream_name` (const std::string &): n/a
  - `item` (GPULauncher::WorkItem): n/a
- Return: A future that resolves to the WorkResult. The future holds an error result if the stream does not exist.
- Details: A future that resolves to the WorkResult. The future holds an error result if the stream does not exist.

#### `~GPUStreamManager()`
- Source: `include/themis/gpu/stream_manager.h`:106
- Brief: n/a
- Parameters: none

### themis::gpu::GPUTensorBuffer

#### `GPUTensorBuffer(GPUTensorBuffer &&) noexcept`
- Source: `include/themis/gpu/tensor_buffer.h`:106
- Brief: n/a
- Parameters:
  - `<unnamed>` (GPUTensorBuffer &&): n/a

#### `GPUTensorBuffer(const GPUTensorBuffer &)=delete`
- Source: `include/themis/gpu/tensor_buffer.h`:111
- Brief: n/a
- Parameters:
  - `<unnamed>` (const GPUTensorBuffer &): n/a

#### `GPUTensorBuffer(std::string name, const Shape &shape, DType dtype)`
- Source: `include/themis/gpu/tensor_buffer.h`:105
- Brief: n/a
- Parameters:
  - `name` (std::string): Human-readable label (used in diagnostics and serialise).
  - `shape` (const Shape &): Tensor shape.
  - `dtype` (DType): Element data type.
- Details: name Human-readable label (used in diagnostics and serialise). shape Tensor shape. dtype Element data type.

#### `void copyFromHost(const void *src, size_t bytes)`
- Source: `include/themis/gpu/tensor_buffer.h`:130
- Brief: Copy bytes bytes from src into the buffer.
- Parameters:
  - `src` (const void *): n/a
  - `bytes` (size_t): n/a
- Details: bytes <= totalBytes()

#### `void copyToHost(void *dst, size_t bytes) const`
- Source: `include/themis/gpu/tensor_buffer.h`:137
- Brief: Copy bytes bytes from the buffer into dst.
- Parameters:
  - `dst` (void *): n/a
  - `bytes` (size_t): n/a
- Details: bytes <= totalBytes()

#### `View createView(const std::string &view_name, size_t offset_elements, const Shape &view_shape) const`
- Source: `include/themis/gpu/tensor_buffer.h`:151
- Brief: Create a named logical view over a slice of this buffer.
- Parameters:
  - `view_name` (const std::string &): Label for the view.
  - `offset_elements` (size_t): Element offset from the start of this buffer.
  - `view_shape` (const Shape &): Shape of the sub-tensor.
- Return: View descriptor.
- Details: view_name Label for the view. offset_elements Element offset from the start of this buffer. view_shape Shape of the sub-tensor. View descriptor.

#### `GPUTensorBuffer deserialize(const std::vector< uint8_t > &bytes)`
- Source: `include/themis/gpu/tensor_buffer.h`:182
- Brief: Reconstruct a GPUTensorBuffer from serialised bytes.
- Parameters:
  - `bytes` (const std::vector< uint8_t > &): n/a
- Throws:
  - std::runtime_error: on corrupt data.
- Details: std::runtime_error on corrupt data.

#### `DType dtype() const noexcept`
- Source: `include/themis/gpu/tensor_buffer.h`:160
- Brief: n/a
- Parameters: none

#### `void fill(double value)`
- Source: `include/themis/gpu/tensor_buffer.h`:123
- Brief: Fill the entire buffer with a constant scalar.
- Parameters:
  - `value` (double): n/a
- Details: The double value is cast to the buffer's DType.

#### `Stats getGlobalStats()`
- Source: `include/themis/gpu/tensor_buffer.h`:187
- Brief: n/a
- Parameters: none

#### `bool isValid() const noexcept`
- Source: `include/themis/gpu/tensor_buffer.h`:162
- Brief: n/a
- Parameters: none

#### `const std::string & name() const noexcept`
- Source: `include/themis/gpu/tensor_buffer.h`:158
- Brief: n/a
- Parameters: none

#### `GPUTensorBuffer & operator=(GPUTensorBuffer &&) noexcept`
- Source: `include/themis/gpu/tensor_buffer.h`:107
- Brief: n/a
- Parameters:
  - `<unnamed>` (GPUTensorBuffer &&): n/a

#### `GPUTensorBuffer & operator=(const GPUTensorBuffer &)=delete`
- Source: `include/themis/gpu/tensor_buffer.h`:112
- Brief: n/a
- Parameters:
  - `<unnamed>` (const GPUTensorBuffer &): n/a

#### `size_t rawBytes() const noexcept`
- Source: `include/themis/gpu/tensor_buffer.h`:166
- Brief: n/a
- Parameters: none

#### `const uint8_t * rawData() const noexcept`
- Source: `include/themis/gpu/tensor_buffer.h`:165
- Brief: n/a
- Parameters: none

#### `void resetGlobalStats()`
- Source: `include/themis/gpu/tensor_buffer.h`:188
- Brief: n/a
- Parameters: none

#### `std::vector< uint8_t > serialize() const`
- Source: `include/themis/gpu/tensor_buffer.h`:175
- Brief: Serialise the buffer (header + raw bytes) for checkpointing.
- Parameters: none

#### `const Shape & shape() const noexcept`
- Source: `include/themis/gpu/tensor_buffer.h`:159
- Brief: n/a
- Parameters: none

#### `size_t totalBytes() const noexcept`
- Source: `include/themis/gpu/tensor_buffer.h`:161
- Brief: n/a
- Parameters: none

#### `~GPUTensorBuffer()`
- Source: `include/themis/gpu/tensor_buffer.h`:108
- Brief: n/a
- Parameters: none

### themis::gpu::GPUTensorBuffer::Shape

#### `size_t elementBytes(DType dtype) noexcept`
- Source: `include/themis/gpu/tensor_buffer.h`:66
- Brief: Size in bytes of a single element of dtype.
- Parameters:
  - `dtype` (DType): n/a

#### `size_t numElements() const noexcept`
- Source: `include/themis/gpu/tensor_buffer.h`:63
- Brief: Total number of elements (product of all dims).
- Parameters: none

#### `bool operator!=(const Shape &other) const noexcept`
- Source: `include/themis/gpu/tensor_buffer.h`:72
- Brief: n/a
- Parameters:
  - `other` (const Shape &): n/a

#### `bool operator==(const Shape &other) const noexcept`
- Source: `include/themis/gpu/tensor_buffer.h`:71
- Brief: n/a
- Parameters:
  - `other` (const Shape &): n/a

#### `size_t totalBytes(DType dtype) const noexcept`
- Source: `include/themis/gpu/tensor_buffer.h`:69
- Brief: Total bytes required for this shape and dtype.
- Parameters:
  - `dtype` (DType): n/a

### themis::gpu::GPUTimeSliceScheduler

#### `GPUTimeSliceScheduler()=default`
- Source: `include/themis/gpu/time_slice_scheduler.h`:98
- Brief: n/a
- Parameters: none

#### `GPUTimeSliceScheduler(const GPUTimeSliceScheduler &)=delete`
- Source: `include/themis/gpu/time_slice_scheduler.h`:102
- Brief: n/a
- Parameters:
  - `<unnamed>` (const GPUTimeSliceScheduler &): n/a

#### `GPUTimeSliceScheduler & GetInstance()`
- Source: `include/themis/gpu/time_slice_scheduler.h`:105
- Brief: n/a
- Parameters: none

#### `bool allQueuesEmpty() const`
- Source: `include/themis/gpu/time_slice_scheduler.h`:202
- Brief: Return true when all tenant queues are empty.
- Parameters: none

#### `void dispatch(GPULauncher::BackendFn backend=nullptr)`
- Source: `include/themis/gpu/time_slice_scheduler.h`:187
- Brief: Execute one scheduling round.
- Parameters:
  - `backend` (GPULauncher::BackendFn): GPU execution backend. Receives each WorkItem and returns true on success. When nullptr is passed a CPU no-op backend (always succeeds) is used.
- Details: Visits each registered tenant in round-robin order (registration order). For each tenant, executes queued items via backend until either: the tenant's time quantum (slice_ms) has elapsed, or the tenant's queue is empty. Items not reached within this round remain in the queue for the next dispatch() call. The preempted counter is incremented for a tenant when the slice expires while items remain in the queue. backend GPU execution backend. Receives each WorkItem and returns true on success. When nullptr is passed a CPU no-op backend (always succeeds) is used.

#### `void drainAll(GPULauncher::BackendFn backend=nullptr)`
- Source: `include/themis/gpu/time_slice_scheduler.h`:197
- Brief: Drain all tenant queues by calling dispatch() until empty.
- Parameters:
  - `backend` (GPULauncher::BackendFn): GPU execution backend; nullptr = CPU no-op.
- Details: Terminates when every tenant's queue is empty. Intended for tests and batch workflows. Avoid calling from a latency-sensitive path. backend GPU execution backend; nullptr = CPU no-op.

#### `std::vector< TenantStats > getAllTenantStats() const`
- Source: `include/themis/gpu/time_slice_scheduler.h`:218
- Brief: Return stats for all registered tenants.
- Parameters: none

#### `Stats getStats() const`
- Source: `include/themis/gpu/time_slice_scheduler.h`:223
- Brief: Return aggregate scheduler statistics.
- Parameters: none

#### `TenantStats getTenantStats(const std::string &tenant_id) const`
- Source: `include/themis/gpu/time_slice_scheduler.h`:213
- Brief: Return stats for a specific tenant.
- Parameters:
  - `tenant_id` (const std::string &): n/a
- Details: Returns a zero-filled TenantStats if the tenant is not registered.

#### `bool hasTenant(const std::string &tenant_id) const`
- Source: `include/themis/gpu/time_slice_scheduler.h`:134
- Brief: Return true if the tenant is currently registered.
- Parameters:
  - `tenant_id` (const std::string &): n/a

#### `GPUTimeSliceScheduler & operator=(const GPUTimeSliceScheduler &)=delete`
- Source: `include/themis/gpu/time_slice_scheduler.h`:103
- Brief: n/a
- Parameters:
  - `<unnamed>` (const GPUTimeSliceScheduler &): n/a

#### `size_t queueDepth(const std::string &tenant_id) const`
- Source: `include/themis/gpu/time_slice_scheduler.h`:165
- Brief: Return the number of items currently queued for tenant_id.
- Parameters:
  - `tenant_id` (const std::string &): n/a
- Details: Returns 0 if the tenant is not registered.

#### `bool registerTenant(const TenantConfig &config)`
- Source: `include/themis/gpu/time_slice_scheduler.h`:122
- Brief: Register a tenant with its time-slice configuration.
- Parameters:
  - `config` (const TenantConfig &): Tenant configuration. tenant_id must be non-empty and slice_ms must be > 0.
- Return: true on success; false if the tenant is already registered or the configuration is invalid.
- Details: config Tenant configuration. tenant_id must be non-empty and slice_ms must be > 0. true on success; false if the tenant is already registered or the configuration is invalid.

#### `void resetStats()`
- Source: `include/themis/gpu/time_slice_scheduler.h`:230
- Brief: Reset all statistics and clear all queues (keeps tenant registrations).
- Parameters: none
- Details: Intended for unit tests.

#### `bool submit(const std::string &tenant_id, GPULauncher::WorkItem item)`
- Source: `include/themis/gpu/time_slice_scheduler.h`:158
- Brief: Enqueue a work item for tenant_id.
- Parameters:
  - `tenant_id` (const std::string &): n/a
  - `item` (GPULauncher::WorkItem): n/a
- Return: true on success; false if the tenant is not registered.
- Details: The item is placed at the back of the tenant's FIFO queue and will be dispatched in a future dispatch() call. true on success; false if the tenant is not registered.

#### `size_t tenantCount() const`
- Source: `include/themis/gpu/time_slice_scheduler.h`:139
- Brief: Return the number of registered tenants.
- Parameters: none

#### `std::vector< std::string > tenantIds() const`
- Source: `include/themis/gpu/time_slice_scheduler.h`:144
- Brief: Return all registered tenant identifiers.
- Parameters: none

#### `bool unregisterTenant(const std::string &tenant_id)`
- Source: `include/themis/gpu/time_slice_scheduler.h`:129
- Brief: Unregister a tenant and discard any pending queue entries.
- Parameters:
  - `tenant_id` (const std::string &): n/a
- Return: true if the tenant was found and removed; false otherwise.
- Details: true if the tenant was found and removed; false otherwise.

#### `~GPUTimeSliceScheduler()=default`
- Source: `include/themis/gpu/time_slice_scheduler.h`:99
- Brief: n/a
- Parameters: none

### themis::gpu::GPUTrainingLoop

#### `GPUTrainingLoop()`
- Source: `include/themis/gpu/training_loop.h`:88
- Brief: n/a
- Parameters: none

#### `GPUTrainingLoop(const Config &config)`
- Source: `include/themis/gpu/training_loop.h`:89
- Brief: n/a
- Parameters:
  - `config` (const Config &): n/a

#### `size_t currentStep() const`
- Source: `include/themis/gpu/training_loop.h`:113
- Brief: n/a
- Parameters: none

#### `const std::vector< StepRecord > & history() const`
- Source: `include/themis/gpu/training_loop.h`:115
- Brief: n/a
- Parameters: none

#### `bool isStopped() const`
- Source: `include/themis/gpu/training_loop.h`:117
- Brief: n/a
- Parameters: none

#### `EpochStats lastEpochStats() const`
- Source: `include/themis/gpu/training_loop.h`:116
- Brief: n/a
- Parameters: none

#### `double lastLoss() const`
- Source: `include/themis/gpu/training_loop.h`:114
- Brief: n/a
- Parameters: none

#### `void reset()`
- Source: `include/themis/gpu/training_loop.h`:120
- Brief: Reset step counter, loss history, and stopped flag.
- Parameters: none

#### `EpochStats run(const std::vector< Batch > &batches, LossFn loss_fn, CheckpointFn checkpoint=nullptr)`
- Source: `include/themis/gpu/training_loop.h`:106
- Brief: Run one epoch over batches using loss_fn.
- Parameters:
  - `batches` (const std::vector< Batch > &): n/a
  - `loss_fn` (LossFn): n/a
  - `checkpoint` (CheckpointFn): n/a
- Return: EpochStats summarising the completed epoch.
- Details: Iterates batches in order (or shuffled when Config::shuffle_batches is true), calls loss_fn for each batch, tracks loss history, fires checkpoint at every checkpoint_interval steps, and stops early when Config::early_stop_loss is configured and the loss crosses the threshold. EpochStats summarising the completed epoch.

### themis::gpu::GPUUnifiedMemoryAllocator

#### `GPUUnifiedMemoryAllocator()=default`
- Source: `include/themis/gpu/unified_memory.h`:111
- Brief: n/a
- Parameters: none

#### `GPUUnifiedMemoryAllocator(const GPUUnifiedMemoryAllocator &)=delete`
- Source: `include/themis/gpu/unified_memory.h`:125
- Brief: n/a
- Parameters:
  - `<unnamed>` (const GPUUnifiedMemoryAllocator &): n/a

#### `GPUUnifiedMemoryAllocator & GetInstance()`
- Source: `include/themis/gpu/unified_memory.h`:106
- Brief: n/a
- Parameters: none

#### `bool advise(const void *ptr, size_t bytes, MemAdvice advice, int device_id=0)`
- Source: `include/themis/gpu/unified_memory.h`:201
- Brief: Set a memory-access hint for the range [ptr, ptr+bytes).
- Parameters:
  - `ptr` (const void *): n/a
  - `bytes` (size_t): n/a
  - `advice` (MemAdvice): n/a
  - `device_id` (int): The device (or -1 for CPU) the hint applies to.
- Return: true on success (or when unified memory is not hardware-backed).
- Details: On CUDA: calls cudaMemAdvise. On HIP: calls hipMemAdvise. CPU fallback: no-op, returns true. device_id The device (or -1 for CPU) the hint applies to. true on success (or when unified memory is not hardware-backed).

#### `void * allocate(size_t bytes, const std::string &tag="unknown", const std::string &tenant_id="")`
- Source: `include/themis/gpu/unified_memory.h`:158
- Brief: Allocate bytes of unified (CPU+GPU shared) memory.
- Parameters:
  - `bytes` (size_t): Number of bytes to allocate. Must be > 0.
  - `tag` (const std::string &): Human-readable owner label for diagnostics.
  - `tenant_id` (const std::string &): Optional tenant identifier for per-tenant tracking.
- Return: Pointer to the allocated region, or nullptr on failure.
- Details: Uses cudaMallocManaged when CUDA is available, hipMallocManaged when HIP is available, and malloc otherwise. bytes Number of bytes to allocate. Must be > 0. tag Human-readable owner label for diagnostics. tenant_id Optional tenant identifier for per-tenant tracking. Pointer to the allocated region, or nullptr on failure.

#### `bool free(void *ptr)`
- Source: `include/themis/gpu/unified_memory.h`:172
- Brief: Free memory previously returned by allocate().
- Parameters:
  - `ptr` (void *): Pointer returned by allocate(); passing nullptr is a no-op.
- Return: true if the pointer was found in the live-allocation table and successfully freed, false otherwise.
- Details: Uses cudaFree / hipFree when unified memory is hardware-backed, free() on the CPU fallback path. ptr Pointer returned by allocate(); passing nullptr is a no-op. true if the pointer was found in the live-allocation table and successfully freed, false otherwise.

#### `std::vector< AllocationRecord > getActiveAllocations() const`
- Source: `include/themis/gpu/unified_memory.h`:212
- Brief: Return a snapshot of all currently live allocations.
- Parameters: none

#### `Stats getStats() const`
- Source: `include/themis/gpu/unified_memory.h`:209
- Brief: Return a copy of the aggregate statistics.
- Parameters: none

#### `uint64_t getTenantBytes(const std::string &tenant_id) const`
- Source: `include/themis/gpu/unified_memory.h`:219
- Brief: Return the total bytes currently live for tenant_id.
- Parameters:
  - `tenant_id` (const std::string &): n/a
- Details: Returns 0 if the tenant has no live allocations.

#### `bool isSupported() noexcept`
- Source: `include/themis/gpu/unified_memory.h`:141
- Brief: Returns true when the runtime supports hardware unified memory.
- Parameters: none
- Details: On CUDA builds: queries cudaDeviceProp::unifiedAddressing for device 0. On HIP builds: queries hipDeviceProp_t::unifiedAddressing for device 0. CPU-only builds: always returns false. The result is cached after the first call.

#### `GPUUnifiedMemoryAllocator & operator=(const GPUUnifiedMemoryAllocator &)=delete`
- Source: `include/themis/gpu/unified_memory.h`:126
- Brief: n/a
- Parameters:
  - `<unnamed>` (const GPUUnifiedMemoryAllocator &): n/a

#### `bool prefetch(const void *ptr, size_t bytes, int device_id=0)`
- Source: `include/themis/gpu/unified_memory.h`:189
- Brief: Prefetch bytes starting at ptr to device_id.
- Parameters:
  - `ptr` (const void *): n/a
  - `bytes` (size_t): n/a
  - `device_id` (int): Target device ordinal; pass -1 / cudaCpuDeviceId to migrate pages back to CPU.
- Return: true on success (or when unified memory is not hardware-backed).
- Details: On CUDA: calls cudaMemPrefetchAsync(ptr, bytes, device_id, nullptr). On HIP: calls hipMemPrefetchAsync(ptr, bytes, device_id, nullptr). CPU fallback: no-op, returns true. device_id Target device ordinal; pass -1 / cudaCpuDeviceId to migrate pages back to CPU. true on success (or when unified memory is not hardware-backed).

#### `void reset()`
- Source: `include/themis/gpu/unified_memory.h`:227
- Brief: Reset all internal state (free all tracked allocations).
- Parameters: none
- Details: Intended for unit tests; in production code call free() on each individual pointer instead.

#### `~GPUUnifiedMemoryAllocator()`
- Source: `include/themis/gpu/unified_memory.h`:122
- Brief: Destructor — frees any allocations that were not explicitly freed by the caller.
- Parameters: none
- Details: For the singleton instance this runs at process exit. On CUDA/HIP builds the runtime is typically still active at that point, but callers that construct local instances must ensure the runtime is still active when the destructor runs (or call reset() explicitly before destruction).

### themis::gpu::IVRAMPolicy

#### `IVRAMPolicy()=default`
- Source: `include/themis/gpu/ivram_policy.h`:96
- Brief: n/a
- Parameters: none

#### `IVRAMPolicy(IVRAMPolicy &&) noexcept=default`
- Source: `include/themis/gpu/ivram_policy.h`:97
- Brief: n/a
- Parameters:
  - `<unnamed>` (IVRAMPolicy &&): n/a

#### `IVRAMPolicy(const IVRAMPolicy &)=delete`
- Source: `include/themis/gpu/ivram_policy.h`:39
- Brief: n/a
- Parameters:
  - `<unnamed>` (const IVRAMPolicy &): n/a

#### `bool canAllocate(uint64_t size_bytes, const std::string &tenant_id="") const =0`
- Source: `include/themis/gpu/ivram_policy.h`:52
- Brief: Return true iff size_bytes can be allocated under current policy.
- Parameters:
  - `size_bytes` (uint64_t): Bytes that the caller intends to allocate.
  - `tenant_id` (const std::string &): Tenant identifier; empty = no per-tenant check.
- Return: true when the allocation would be granted.
- Details: Checks both the global edition VRAM limit and the per-tenant quota when tenant_id is non-empty. Does not modify any state. size_bytes Bytes that the caller intends to allocate. tenant_id Tenant identifier; empty = no per-tenant check. true when the allocation would be granted.

#### `bool isGPUEnabled() const noexcept=0`
- Source: `include/themis/gpu/ivram_policy.h`:93
- Brief: Return true when this policy allows any GPU acceleration.
- Parameters: none
- Details: Implementations should return false when the edition limit is zero or no GPU hardware is available.

#### `void onAllocate(uint64_t size_bytes, const std::string &tag, const std::string &tenant_id="")=0`
- Source: `include/themis/gpu/ivram_policy.h`:64
- Brief: Notify the policy that size_bytes have been successfully allocated.
- Parameters:
  - `size_bytes` (uint64_t): Bytes that were allocated.
  - `tag` (const std::string &): Owner / reason label.
  - `tenant_id` (const std::string &): Tenant identifier; empty = no per-tenant tracking.
- Details: Updates accounting state (used bytes, tenant counters, peak). size_bytes Bytes that were allocated. tag Owner / reason label. tenant_id Tenant identifier; empty = no per-tenant tracking.

#### `void onDeallocate(uint64_t size_bytes, const std::string &tenant_id="")=0`
- Source: `include/themis/gpu/ivram_policy.h`:77
- Brief: Notify the policy that size_bytes have been freed.
- Parameters:
  - `size_bytes` (uint64_t): Bytes that were freed.
  - `tenant_id` (const std::string &): Tenant identifier; empty = no per-tenant update.
- Details: Updates accounting state. Implementations must clamp to zero on mis-matched sizes to prevent underflow. size_bytes Bytes that were freed. tenant_id Tenant identifier; empty = no per-tenant update.

#### `IVRAMPolicy & operator=(IVRAMPolicy &&) noexcept=default`
- Source: `include/themis/gpu/ivram_policy.h`:98
- Brief: n/a
- Parameters:
  - `<unnamed>` (IVRAMPolicy &&): n/a

#### `IVRAMPolicy & operator=(const IVRAMPolicy &)=delete`
- Source: `include/themis/gpu/ivram_policy.h`:40
- Brief: n/a
- Parameters:
  - `<unnamed>` (const IVRAMPolicy &): n/a

#### `uint64_t usedBytes() const =0`
- Source: `include/themis/gpu/ivram_policy.h`:85
- Brief: Return the number of VRAM bytes currently tracked as allocated.
- Parameters: none
- Return: Currently accounted VRAM in bytes.
- Details: Currently accounted VRAM in bytes.

#### `~IVRAMPolicy()=default`
- Source: `include/themis/gpu/ivram_policy.h`:36
- Brief: n/a
- Parameters: none

### themis::gpu::KernelSLAGuard

#### `KernelSLAGuard(KernelSLAGuard &&other) noexcept`
- Source: `include/themis/gpu/gpu_timeout.h`:30
- Brief: n/a
- Parameters:
  - `other` (KernelSLAGuard &&): n/a

#### `KernelSLAGuard(const KernelSLAGuard &)=delete`
- Source: `include/themis/gpu/gpu_timeout.h`:27
- Brief: n/a
- Parameters:
  - `<unnamed>` (const KernelSLAGuard &): n/a

#### `KernelSLAGuard(std::chrono::steady_clock::duration timeout_duration=DEFAULT_SLA_DURATION) noexcept`
- Source: `include/themis/gpu/gpu_timeout.h`:21
- Brief: n/a
- Parameters:
  - `timeout_duration` (std::chrono::steady_clock::duration): n/a

#### `bool checkTimeoutDeadline() const noexcept`
- Source: `include/themis/gpu/gpu_timeout.h`:46
- Brief: n/a
- Parameters: none

#### `std::chrono::steady_clock::time_point getDeadline() const noexcept`
- Source: `include/themis/gpu/gpu_timeout.h`:63
- Brief: n/a
- Parameters: none

#### `std::chrono::steady_clock::duration getElapsedTime() const noexcept`
- Source: `include/themis/gpu/gpu_timeout.h`:51
- Brief: n/a
- Parameters: none

#### `std::chrono::steady_clock::duration getRemainingTime() const noexcept`
- Source: `include/themis/gpu/gpu_timeout.h`:55
- Brief: n/a
- Parameters: none

#### `std::chrono::steady_clock::duration getSLADuration() const noexcept`
- Source: `include/themis/gpu/gpu_timeout.h`:59
- Brief: n/a
- Parameters: none

#### `std::chrono::steady_clock::time_point getStartTime() const noexcept`
- Source: `include/themis/gpu/gpu_timeout.h`:64
- Brief: n/a
- Parameters: none

#### `KernelSLAGuard & operator=(KernelSLAGuard &&other) noexcept`
- Source: `include/themis/gpu/gpu_timeout.h`:35
- Brief: n/a
- Parameters:
  - `other` (KernelSLAGuard &&): n/a

#### `KernelSLAGuard & operator=(const KernelSLAGuard &)=delete`
- Source: `include/themis/gpu/gpu_timeout.h`:28
- Brief: n/a
- Parameters:
  - `<unnamed>` (const KernelSLAGuard &): n/a

#### `~KernelSLAGuard() noexcept=default`
- Source: `include/themis/gpu/gpu_timeout.h`:44
- Brief: n/a
- Parameters: none

### themis::gpu::MIGManager

#### `MIGManager & GetInstance()`
- Source: `include/themis/gpu/mig_manager.h`:128
- Brief: n/a
- Parameters: none

#### `MIGManager()=default`
- Source: `include/themis/gpu/mig_manager.h`:140
- Brief: n/a
- Parameters: none

#### `MIGManager(const MIGManager &)=delete`
- Source: `include/themis/gpu/mig_manager.h`:134
- Brief: n/a
- Parameters:
  - `<unnamed>` (const MIGManager &): n/a

#### `Status assignToTenant(const std::string &instance_id, const std::string &tenant_id)`
- Source: `include/themis/gpu/mig_manager.h`:204
- Brief: Assign a MIG instance to a tenant.
- Parameters:
  - `instance_id` (const std::string &): MIG instance to assign.
  - `tenant_id` (const std::string &): Non-empty tenant identifier.
- Return: Status::OK, INSTANCE_NOT_FOUND, or ALREADY_ASSIGNED.
- Details: Tenants are identified by an arbitrary non-empty string. An instance can be assigned to at most one tenant at a time. instance_id MIG instance to assign. tenant_id Non-empty tenant identifier. Status::OK, INSTANCE_NOT_FOUND, or ALREADY_ASSIGNED.

#### `Status createPartition(int device_index, const std::string &profile, std::string &out_instance_id)`
- Source: `include/themis/gpu/mig_manager.h`:165
- Brief: Create a MIG partition on the specified device.
- Parameters:
  - `device_index` (int): Physical GPU device index.
  - `profile` (const std::string &): MIG profile string (e.g. "1g.5gb").
  - `out_instance_id` (std::string &): Receives the new instance ID on success.
- Return: Status::OK or an error code.
- Details: Requirements: MIG_MANAGER feature flag must be enabled. device_index must refer to a healthy GPU with compute major >= 8. profile must be one of the known profile strings. The device must have capacity for at least one more instance (max 7 per device). On success out_instance_id is populated with a unique identifier that can be used in subsequent calls. device_index Physical GPU device index. profile MIG profile string (e.g. "1g.5gb"). out_instance_id Receives the new instance ID on success. Status::OK or an error code.

#### `Status createPartition(int device_index, const std::string &profile, std::string &out_instance_id, const std::vector< DeviceInfo > &devices)`
- Source: `include/themis/gpu/mig_manager.h`:174
- Brief: Overload that accepts an explicit device list instead of calling DeviceDiscovery::Enumerate(). Useful for unit testing without GPU hardware.
- Parameters:
  - `device_index` (int): n/a
  - `profile` (const std::string &): n/a
  - `out_instance_id` (std::string &): n/a
  - `devices` (const std::vector< DeviceInfo > &): n/a

#### `Status destroyPartition(const std::string &instance_id)`
- Source: `include/themis/gpu/mig_manager.h`:188
- Brief: Destroy a MIG partition and release its resources.
- Parameters:
  - `instance_id` (const std::string &): ID returned by a prior createPartition() call.
- Return: Status::OK or an error code.
- Details: The partition must not be assigned to a tenant at the time of destruction (call unassignFromTenant first). instance_id ID returned by a prior createPartition() call. Status::OK or an error code.

#### `bool deviceSupportsMIG(const DeviceInfo &device) noexcept`
- Source: `include/themis/gpu/mig_manager.h`:253
- Brief: Return true when device supports MIG (compute major >= 8).
- Parameters:
  - `device` (const DeviceInfo &): n/a
- Details: Does NOT check whether MIG mode is currently enabled in the driver.

#### `bool getInstance(const std::string &instance_id, MIGInstance &out) const`
- Source: `include/themis/gpu/mig_manager.h`:241
- Brief: Return the MIG instance descriptor for a given instance ID.
- Parameters:
  - `instance_id` (const std::string &): n/a
  - `out` (MIGInstance &): n/a
- Details: Returns false (and leaves out unchanged) when the instance does not exist.

#### `std::vector< MIGInstance > getInstances() const`
- Source: `include/themis/gpu/mig_manager.h`:222
- Brief: Return all active MIG instances across all devices.
- Parameters: none

#### `std::vector< MIGInstance > getInstancesForDevice(int device_index) const`
- Source: `include/themis/gpu/mig_manager.h`:227
- Brief: Return all active MIG instances on a specific device.
- Parameters:
  - `device_index` (int): n/a

#### `std::vector< MIGInstance > getInstancesForTenant(const std::string &tenant_id) const`
- Source: `include/themis/gpu/mig_manager.h`:232
- Brief: Return all MIG instances currently assigned to a tenant.
- Parameters:
  - `tenant_id` (const std::string &): n/a

#### `Stats getStats() const`
- Source: `include/themis/gpu/mig_manager.h`:270
- Brief: n/a
- Parameters: none

#### `bool isKnownProfile(const std::string &profile) noexcept`
- Source: `include/themis/gpu/mig_manager.h`:258
- Brief: Return true when profile is a recognised MIG profile string.
- Parameters:
  - `profile` (const std::string &): n/a

#### `std::string makeInstanceId(int device_index, int gi_id)`
- Source: `include/themis/gpu/mig_manager.h`:283
- Brief: Build a deterministic instance ID from device index and gi_id.
- Parameters:
  - `device_index` (int): n/a
  - `gi_id` (int): n/a
- Details: Public so that tests can construct expected IDs without duplicating the naming logic.

#### `MIGManager & operator=(const MIGManager &)=delete`
- Source: `include/themis/gpu/mig_manager.h`:135
- Brief: n/a
- Parameters:
  - `<unnamed>` (const MIGManager &): n/a

#### `uint64_t profileMemoryBytes(const std::string &profile) noexcept`
- Source: `include/themis/gpu/mig_manager.h`:265
- Brief: Return the VRAM size in bytes associated with profile.
- Parameters:
  - `profile` (const std::string &): n/a
- Details: Returns 0 for unknown profiles.

#### `void reset()`
- Source: `include/themis/gpu/mig_manager.h`:275
- Brief: Reset statistics and remove all instances (for testing).
- Parameters: none

#### `Status unassignFromTenant(const std::string &instance_id)`
- Source: `include/themis/gpu/mig_manager.h`:213
- Brief: Remove the tenant assignment from a MIG instance.
- Parameters:
  - `instance_id` (const std::string &): MIG instance to unassign.
- Return: Status::OK, INSTANCE_NOT_FOUND, or NOT_ASSIGNED.
- Details: instance_id MIG instance to unassign. Status::OK, INSTANCE_NOT_FOUND, or NOT_ASSIGNED.

#### `~MIGManager()=default`
- Source: `include/themis/gpu/mig_manager.h`:141
- Brief: n/a
- Parameters: none

### themis::gpu::QueryShape

#### `bool operator==(const QueryShape &o) const noexcept`
- Source: `include/themis/gpu/graph_cache.h`:43
- Brief: n/a
- Parameters:
  - `o` (const QueryShape &): n/a

### themis::gpu::QueryShapeHash

#### `size_t operator()(const QueryShape &s) const noexcept`
- Source: `include/themis/gpu/graph_cache.h`:50
- Brief: n/a
- Parameters:
  - `s` (const QueryShape &): n/a

### themis::gpu::ROCmBackend

#### `ROCmBackend & GetInstance()`
- Source: `include/themis/gpu/rocm_backend.h`:95
- Brief: n/a
- Parameters: none

#### `ROCmBackend()=default`
- Source: `include/themis/gpu/rocm_backend.h`:236
- Brief: n/a
- Parameters: none

#### `ROCmBackend(const ROCmBackend &)=delete`
- Source: `include/themis/gpu/rocm_backend.h`:238
- Brief: n/a
- Parameters:
  - `<unnamed>` (const ROCmBackend &): n/a

#### `AllocationRecord allocate(size_t size_bytes, const std::string &tag="")`
- Source: `include/themis/gpu/rocm_backend.h`:194
- Brief: Allocate size_bytes of device memory via hipMalloc.
- Parameters:
  - `size_bytes` (size_t): Bytes to allocate.
  - `tag` (const std::string &): Owner / reason label for diagnostics.
- Return: AllocationRecord; check is_valid() before use.
- Details: On systems without HIP returns an invalid record (AllocationRecord::device_ptr == 0). size_bytes Bytes to allocate. tag Owner / reason label for diagnostics. AllocationRecord; check is_valid() before use.

#### `GPULauncher::BackendFn createBackendFn(int device_index=0)`
- Source: `include/themis/gpu/rocm_backend.h`:136
- Brief: Create a GPULauncher::BackendFn backed by HIP.
- Parameters:
  - `device_index` (int): Target HIP device index (0-based).
- Details: The returned function: On systems with HIP: selects device_index, then executes the work item. Kernel blob dispatch via .hsaco is a hardware-only feature; on a device without loaded kernels the call succeeds (no-op kernel). On systems without HIP: returns true immediately (CPU path). The function is safe to move into GPULauncher or GPUStreamManager::createStream(). device_index Target HIP device index (0-based).

#### `Result createStream(const std::string &name, int device_index=0)`
- Source: `include/themis/gpu/rocm_backend.h`:152
- Brief: Create a named HIP stream on device_index.
- Parameters:
  - `name` (const std::string &): n/a
  - `device_index` (int): n/a
- Return: ok == false only when a stream with name already exists. If HIP stream creation fails at runtime, a virtual stream entry is registered so fallback execution remains available.
- Details: On systems without HIP records a virtual stream entry so that hasStream() / streamNames() still work correctly. ok == false only when a stream with name already exists. If HIP stream creation fails at runtime, a virtual stream entry is registered so fallback execution remains available.

#### `Result deallocate(AllocationRecord &rec)`
- Source: `include/themis/gpu/rocm_backend.h`:202
- Brief: Release device memory previously returned by allocate().
- Parameters:
  - `rec` (AllocationRecord &): n/a
- Details: Calls hipFree on the stored pointer; safe to call with an invalid record. Clears rec on success.

#### `Result destroyStream(const std::string &name)`
- Source: `include/themis/gpu/rocm_backend.h`:160
- Brief: Destroy a named HIP stream.
- Parameters:
  - `name` (const std::string &): n/a
- Return: ok == false when no stream with name exists.
- Details: Calls hipStreamDestroy when THEMIS_ENABLE_HIP is active. ok == false when no stream with name exists.

#### `int deviceCount() const`
- Source: `include/themis/gpu/rocm_backend.h`:110
- Brief: Number of HIP-capable devices visible to the process.
- Parameters: none
- Details: Returns 0 when THEMIS_ENABLE_HIP is not defined or when no device is detected at runtime.

#### `Stats getStats() const`
- Source: `include/themis/gpu/rocm_backend.h`:226
- Brief: n/a
- Parameters: none

#### `StreamHandle getStream(const std::string &name) const`
- Source: `include/themis/gpu/rocm_backend.h`:172
- Brief: Return the handle for a named stream (invalid if not found).
- Parameters:
  - `name` (const std::string &): n/a

#### `bool hasStream(const std::string &name) const`
- Source: `include/themis/gpu/rocm_backend.h`:175
- Brief: True when a stream with name has been created.
- Parameters:
  - `name` (const std::string &): n/a

#### `bool isAvailable() const`
- Source: `include/themis/gpu/rocm_backend.h`:115
- Brief: True when at least one HIP device is available and responsive.
- Parameters: none

#### `ROCmBackend & operator=(const ROCmBackend &)=delete`
- Source: `include/themis/gpu/rocm_backend.h`:239
- Brief: n/a
- Parameters:
  - `<unnamed>` (const ROCmBackend &): n/a

#### `void resetStats()`
- Source: `include/themis/gpu/rocm_backend.h`:233
- Brief: Reset all statistics counters (for testing).
- Parameters: none
- Details: Does not affect existing streams or allocations.

#### `std::vector< std::string > streamNames() const`
- Source: `include/themis/gpu/rocm_backend.h`:178
- Brief: Return all registered stream names.
- Parameters: none

#### `Result synchronizeStream(const std::string &name)`
- Source: `include/themis/gpu/rocm_backend.h`:169
- Brief: Block until all work enqueued on the named stream completes.
- Parameters:
  - `name` (const std::string &): n/a
- Return: ok == false when the stream does not exist or synchronization fails.
- Details: Calls hipStreamSynchronize when THEMIS_ENABLE_HIP is active. ok == false when the stream does not exist or synchronization fails.

#### `Result zeroMemory(uintptr_t device_ptr, size_t size_bytes)`
- Source: `include/themis/gpu/rocm_backend.h`:213
- Brief: Fill device memory with zeros via hipMemset.
- Parameters:
  - `device_ptr` (uintptr_t): Device pointer (hipMalloc result cast to uintptr_t).
  - `size_bytes` (size_t): Bytes to zero.
- Details: Falls back to std::memset on a CPU-side buffer when HIP is absent. Used by GPUMemoryPool::release() when zero_on_free is set. device_ptr Device pointer (hipMalloc result cast to uintptr_t). size_bytes Bytes to zero.

### themis::gpu::ROCmBackend::AllocationRecord

#### `bool is_valid() const noexcept`
- Source: `include/themis/gpu/rocm_backend.h`:89
- Brief: True when the allocation holds a real device pointer.
- Parameters: none

### themis::gpu::ROCmBackend::StreamHandle

#### `bool is_valid() const noexcept`
- Source: `include/themis/gpu/rocm_backend.h`:76
- Brief: True when the native handle has been created.
- Parameters: none

### themis::gpu::ScopedGPURange

#### `ScopedGPURange(const ScopedGPURange &)=delete`
- Source: `include/themis/gpu/profiler.h`:187
- Brief: n/a
- Parameters:
  - `<unnamed>` (const ScopedGPURange &): n/a

#### `ScopedGPURange(const std::string &name, uint32_t argb_color=0xFF00FF00)`
- Source: `include/themis/gpu/profiler.h`:183
- Brief: n/a
- Parameters:
  - `name` (const std::string &): n/a
  - `argb_color` (uint32_t): n/a

#### `ScopedGPURange & operator=(const ScopedGPURange &)=delete`
- Source: `include/themis/gpu/profiler.h`:188
- Brief: n/a
- Parameters:
  - `<unnamed>` (const ScopedGPURange &): n/a

#### `~ScopedGPURange()`
- Source: `include/themis/gpu/profiler.h`:185
- Brief: n/a
- Parameters: none

### themis::gpu::TopologyLink

#### `bool is_inter_node() const noexcept`
- Source: `include/themis/gpu/cluster_topology.h`:61
- Brief: n/a
- Parameters: none

### themis::gpu::VulkanComputeBackend

#### `VulkanComputeBackend & GetInstance()`
- Source: `include/themis/gpu/vulkan_backend.h`:86
- Brief: n/a
- Parameters: none

#### `VulkanComputeBackend()=default`
- Source: `include/themis/gpu/vulkan_backend.h`:197
- Brief: n/a
- Parameters: none

#### `VulkanComputeBackend(const VulkanComputeBackend &)=delete`
- Source: `include/themis/gpu/vulkan_backend.h`:199
- Brief: n/a
- Parameters:
  - `<unnamed>` (const VulkanComputeBackend &): n/a

#### `GPULauncher::BackendFn createBackendFn(int device_index=0)`
- Source: `include/themis/gpu/vulkan_backend.h`:135
- Brief: Create a GPULauncher::BackendFn backed by Vulkan compute.
- Parameters:
  - `device_index` (int): Vulkan physical device ordinal (0-based, ignored when Vulkan is unavailable).
- Details: The returned function: On systems with Vulkan: selects a compute-capable device and dispatches the work item through the Vulkan acceleration backend. Falls back to CPU when the Vulkan backend is not initialized. On systems without Vulkan: returns true immediately (CPU path). The function is safe to move into GPULauncher or GPUStreamManager::createStream(). device_index Vulkan physical device ordinal (0-based, ignored when Vulkan is unavailable).

#### `Result createStream(const std::string &name, int device_index=0)`
- Source: `include/themis/gpu/vulkan_backend.h`:150
- Brief: Register a named logical compute stream on device_index.
- Parameters:
  - `name` (const std::string &): n/a
  - `device_index` (int): n/a
- Return: ok == false when a stream with name already exists or name is empty.
- Details: On systems without Vulkan records a virtual stream entry so that hasStream() / streamNames() still work correctly. ok == false when a stream with name already exists or name is empty.

#### `Result destroyStream(const std::string &name)`
- Source: `include/themis/gpu/vulkan_backend.h`:157
- Brief: Unregister and release a named logical compute stream.
- Parameters:
  - `name` (const std::string &): n/a
- Return: ok == false when no stream with name exists.
- Details: ok == false when no stream with name exists.

#### `int deviceCount() const`
- Source: `include/themis/gpu/vulkan_backend.h`:101
- Brief: Number of Vulkan-capable physical devices visible to the process.
- Parameters: none
- Details: Returns 0 when THEMIS_ENABLE_VULKAN is not defined or when no Vulkan device is detected at runtime.

#### `Stats getStats() const`
- Source: `include/themis/gpu/vulkan_backend.h`:187
- Brief: n/a
- Parameters: none

#### `StreamHandle getStream(const std::string &name) const`
- Source: `include/themis/gpu/vulkan_backend.h`:168
- Brief: Return the handle for a named stream (invalid if not found).
- Parameters:
  - `name` (const std::string &): n/a

#### `bool hasStream(const std::string &name) const`
- Source: `include/themis/gpu/vulkan_backend.h`:171
- Brief: True when a stream with name has been created.
- Parameters:
  - `name` (const std::string &): n/a

#### `bool isAvailable() const`
- Source: `include/themis/gpu/vulkan_backend.h`:106
- Brief: True when at least one Vulkan compute device is available.
- Parameters: none

#### `VulkanComputeBackend & operator=(const VulkanComputeBackend &)=delete`
- Source: `include/themis/gpu/vulkan_backend.h`:200
- Brief: n/a
- Parameters:
  - `<unnamed>` (const VulkanComputeBackend &): n/a

#### `void probeDevices() const`
- Source: `include/themis/gpu/vulkan_backend.h`:213
- Brief: n/a
- Parameters: none

#### `void resetStats()`
- Source: `include/themis/gpu/vulkan_backend.h`:194
- Brief: Reset all statistics counters (for testing).
- Parameters: none
- Details: Does not affect existing streams.

#### `std::vector< std::string > streamNames() const`
- Source: `include/themis/gpu/vulkan_backend.h`:174
- Brief: Return all registered stream names.
- Parameters: none

#### `Result synchronizeStream(const std::string &name)`
- Source: `include/themis/gpu/vulkan_backend.h`:165
- Brief: Block until all pending work on the named stream completes.
- Parameters:
  - `name` (const std::string &): n/a
- Return: ok == false when the stream does not exist.
- Details: On the CPU fallback path this is a no-op that returns ok == true. ok == false when the stream does not exist.

#### `std::string vendorName() const`
- Source: `include/themis/gpu/vulkan_backend.h`:114
- Brief: Human-readable name of the vendor for the selected device.
- Parameters: none
- Details: Returns "Unknown" when Vulkan is not available or the vendor cannot be determined from the PCI vendor ID.

### themis::gpu::VulkanComputeBackend::StreamHandle

#### `bool is_valid() const noexcept`
- Source: `include/themis/gpu/vulkan_backend.h`:80
- Brief: True when the native handle has been associated.
- Parameters: none

### themis::gpu::WASMKernelSandbox

#### `WASMKernelSandbox & GetInstance()`
- Source: `include/themis/gpu/wasm_kernel_sandbox.h`:125
- Brief: n/a
- Parameters: none

#### `WASMKernelSandbox()=default`
- Source: `include/themis/gpu/wasm_kernel_sandbox.h`:137
- Brief: Construct with default sandbox configuration.
- Parameters: none

#### `WASMKernelSandbox(SandboxConfig config)`
- Source: `include/themis/gpu/wasm_kernel_sandbox.h`:142
- Brief: Construct with an explicit sandbox configuration.
- Parameters:
  - `config` (SandboxConfig): n/a

#### `WASMKernelSandbox(const WASMKernelSandbox &)=delete`
- Source: `include/themis/gpu/wasm_kernel_sandbox.h`:145
- Brief: n/a
- Parameters:
  - `<unnamed>` (const WASMKernelSandbox &): n/a

#### `ExecutionResult execute(const std::string &kernel_id, const std::vector< uint8_t > &blob, GPULauncher::BackendFn backend=nullptr)`
- Source: `include/themis/gpu/wasm_kernel_sandbox.h`:201
- Brief: Execute blob as a sandboxed GPU kernel.
- Parameters:
  - `kernel_id` (const std::string &): Kernel identifier (must be registered with GPUKernelValidator).
  - `blob` (const std::vector< uint8_t > &): Kernel bytecode / binary to execute.
  - `backend` (GPULauncher::BackendFn): Optional execution backend. When nullptr a CPU no-op that always succeeds is used (for testing / CI without GPU hardware).
- Return: ExecutionResult describing the outcome.
- Details: Execution pipeline: Check WASM_SANDBOX feature flag → reject if disabled. Reject empty blobs. Reject blobs that exceed SandboxConfig::memory_limit_bytes. Validate kernel_id + blob via GPUKernelValidator. Execute in the WASM sandbox (or CPU simulation fallback). Enforce max_execution_ms timeout; reject with REJECTED_TIMEOUT if the deadline is exceeded. kernel_id Kernel identifier (must be registered with GPUKernelValidator). blob Kernel bytecode / binary to execute. backend Optional execution backend. When nullptr a CPU no-op that always succeeds is used (for testing / CI without GPU hardware). ExecutionResult describing the outcome.

#### `SandboxConfig getConfig() const`
- Source: `include/themis/gpu/wasm_kernel_sandbox.h`:162
- Brief: Return the current sandbox configuration.
- Parameters: none

#### `Stats getStats() const`
- Source: `include/themis/gpu/wasm_kernel_sandbox.h`:209
- Brief: n/a
- Parameters: none

#### `bool isWASMSupported() const noexcept`
- Source: `include/themis/gpu/wasm_kernel_sandbox.h`:175
- Brief: Return true when a WASM runtime is available.
- Parameters: none
- Details: Always returns false in the current build (CPU simulation path). Returns true when THEMIS_ENABLE_WASM is defined and the runtime initialises successfully.

#### `WASMKernelSandbox & operator=(const WASMKernelSandbox &)=delete`
- Source: `include/themis/gpu/wasm_kernel_sandbox.h`:146
- Brief: n/a
- Parameters:
  - `<unnamed>` (const WASMKernelSandbox &): n/a

#### `void recordResult(const ExecutionResult &r)`
- Source: `include/themis/gpu/wasm_kernel_sandbox.h`:238
- Brief: n/a
- Parameters:
  - `r` (const ExecutionResult &): n/a

#### `void resetStats()`
- Source: `include/themis/gpu/wasm_kernel_sandbox.h`:214
- Brief: Reset all statistics (for testing).
- Parameters: none

#### `ExecutionResult runInSandbox(const std::string &kernel_id, const std::vector< uint8_t > &blob, GPULauncher::BackendFn backend)`
- Source: `include/themis/gpu/wasm_kernel_sandbox.h`:234
- Brief: n/a
- Parameters:
  - `kernel_id` (const std::string &): n/a
  - `blob` (const std::vector< uint8_t > &): n/a
  - `backend` (GPULauncher::BackendFn): n/a

#### `void setConfig(SandboxConfig config)`
- Source: `include/themis/gpu/wasm_kernel_sandbox.h`:157
- Brief: Replace the current sandbox configuration.
- Parameters:
  - `config` (SandboxConfig): n/a
- Details: Thread-safe; effective for all subsequent execute() calls.

### themis::gpu::WASMKernelSandbox::ExecutionResult

#### `bool ok() const noexcept`
- Source: `include/themis/gpu/wasm_kernel_sandbox.h`:103
- Brief: n/a
- Parameters: none

### themis::gpu::shared_gpu_ptr

#### `const T * get() const noexcept`
- Source: `include/themis/gpu/gpu_memory.h`:469
- Brief: Get raw pointer (const).
- Parameters: none

#### `T * get() noexcept`
- Source: `include/themis/gpu/gpu_memory.h`:464
- Brief: Get raw pointer.
- Parameters: none

#### `operator bool() const noexcept`
- Source: `include/themis/gpu/gpu_memory.h`:498
- Brief: Boolean conversion.
- Parameters: none

#### `const T & operator*() const noexcept`
- Source: `include/themis/gpu/gpu_memory.h`:480
- Brief: Dereference (const).
- Parameters: none

#### `T & operator*() noexcept`
- Source: `include/themis/gpu/gpu_memory.h`:474
- Brief: Dereference.
- Parameters: none

#### `const T * operator->() const noexcept`
- Source: `include/themis/gpu/gpu_memory.h`:492
- Brief: Member access (const).
- Parameters: none

#### `T * operator->() noexcept`
- Source: `include/themis/gpu/gpu_memory.h`:486
- Brief: Member access.
- Parameters: none

#### `shared_gpu_ptr & operator=(const shared_gpu_ptr &other) noexcept`
- Source: `include/themis/gpu/gpu_memory.h`:447
- Brief: Copy assignment.
- Parameters:
  - `other` (const shared_gpu_ptr &): n/a

#### `shared_gpu_ptr & operator=(shared_gpu_ptr &&other) noexcept`
- Source: `include/themis/gpu/gpu_memory.h`:458
- Brief: Move assignment.
- Parameters:
  - `other` (shared_gpu_ptr &&): n/a

#### `constexpr shared_gpu_ptr() noexcept`
- Source: `include/themis/gpu/gpu_memory.h`:422
- Brief: Default constructor: null pointer.
- Parameters: none

#### `shared_gpu_ptr(T *p) noexcept`
- Source: `include/themis/gpu/gpu_memory.h`:425
- Brief: Construct from raw pointer.
- Parameters:
  - `p` (T *): n/a

#### `shared_gpu_ptr(const shared_gpu_ptr &other) noexcept`
- Source: `include/themis/gpu/gpu_memory.h`:440
- Brief: Copy constructor: increments refcount.
- Parameters:
  - `other` (const shared_gpu_ptr &): n/a

#### `shared_gpu_ptr(shared_gpu_ptr &&other) noexcept`
- Source: `include/themis/gpu/gpu_memory.h`:455
- Brief: Move constructor.
- Parameters:
  - `other` (shared_gpu_ptr &&): n/a

#### `shared_gpu_ptr(std::nullptr_t) noexcept`
- Source: `include/themis/gpu/gpu_memory.h`:429
- Brief: Construct from nullptr.
- Parameters:
  - `<unnamed>` (std::nullptr_t): n/a

#### `void swap(shared_gpu_ptr &other) noexcept`
- Source: `include/themis/gpu/gpu_memory.h`:508
- Brief: Swap.
- Parameters:
  - `other` (shared_gpu_ptr &): n/a

#### `int use_count() const noexcept`
- Source: `include/themis/gpu/gpu_memory.h`:503
- Brief: Get reference count.
- Parameters: none

#### `~shared_gpu_ptr() noexcept`
- Source: `include/themis/gpu/gpu_memory.h`:432
- Brief: Destructor: decrements refcount; frees if zero.
- Parameters: none

### themis::gpu::shared_gpu_ptr::ControlBlock

#### `ControlBlock(T *p)`
- Source: `include/themis/gpu/gpu_memory.h`:401
- Brief: n/a
- Parameters:
  - `p` (T *): n/a

#### `~ControlBlock()`
- Source: `include/themis/gpu/gpu_memory.h`:403
- Brief: n/a
- Parameters: none

### themis::gpu::unique_gpu_ptr

#### `const_pointer get() const noexcept`
- Source: `include/themis/gpu/gpu_memory.h`:192
- Brief: Get raw pointer (const).
- Parameters: none

#### `pointer get() noexcept`
- Source: `include/themis/gpu/gpu_memory.h`:189
- Brief: n/a
- Parameters: none
- Return: Raw pointer; may be nullptr
- Details: Get raw pointer (non-owning). Raw pointer; may be nullptr

#### `operator bool() const noexcept`
- Source: `include/themis/gpu/gpu_memory.h`:223
- Brief: Boolean conversion: true if owns non-null pointer.
- Parameters: none

#### `bool operator!=(const unique_gpu_ptr &other) const noexcept`
- Source: `include/themis/gpu/gpu_memory.h`:231
- Brief: Comparison: inequality.
- Parameters:
  - `other` (const unique_gpu_ptr &): n/a

#### `const_reference operator*() const noexcept`
- Source: `include/themis/gpu/gpu_memory.h`:203
- Brief: Dereference (const).
- Parameters: none

#### `reference operator*() noexcept`
- Source: `include/themis/gpu/gpu_memory.h`:197
- Brief: n/a
- Parameters: none
- Return: Reference to element
- Details: Dereference: access pointed-to element. Reference to element ptr_ != nullptr

#### `const_pointer operator->() const noexcept`
- Source: `include/themis/gpu/gpu_memory.h`:217
- Brief: Member access (const).
- Parameters: none

#### `pointer operator->() noexcept`
- Source: `include/themis/gpu/gpu_memory.h`:211
- Brief: n/a
- Parameters: none
- Return: Raw pointer
- Details: Member access: pointer-to-member. Raw pointer ptr_ != nullptr

#### `bool operator<(const unique_gpu_ptr &other) const noexcept`
- Source: `include/themis/gpu/gpu_memory.h`:236
- Brief: Comparison: less-than (for use in containers).
- Parameters:
  - `other` (const unique_gpu_ptr &): n/a

#### `unique_gpu_ptr & operator=(const unique_gpu_ptr &)=delete`
- Source: `include/themis/gpu/gpu_memory.h`:181
- Brief: Deleted copy assignment: prevent accidental duplication.
- Parameters:
  - `<unnamed>` (const unique_gpu_ptr &): n/a

#### `unique_gpu_ptr & operator=(std::nullptr_t) noexcept`
- Source: `include/themis/gpu/gpu_memory.h`:172
- Brief: Move from nullptr.
- Parameters:
  - `<unnamed>` (std::nullptr_t): n/a

#### `unique_gpu_ptr & operator=(unique_gpu_ptr &&other) noexcept`
- Source: `include/themis/gpu/gpu_memory.h`:166
- Brief: Move assignment: transfers ownership.
- Parameters:
  - `other` (unique_gpu_ptr &&): n/a

#### `bool operator==(const unique_gpu_ptr &other) const noexcept`
- Source: `include/themis/gpu/gpu_memory.h`:226
- Brief: Comparison: equality.
- Parameters:
  - `other` (const unique_gpu_ptr &): n/a

#### `pointer release() noexcept`
- Source: `include/themis/gpu/gpu_memory.h`:247
- Brief: n/a
- Parameters: none
- Return: Owned pointer; caller now responsible for deallocation
- Details: Release ownership and return raw pointer. Owned pointer; caller now responsible for deallocation ptr_ == nullptr

#### `void reset(pointer p=nullptr) noexcept`
- Source: `include/themis/gpu/gpu_memory.h`:255
- Brief: n/a
- Parameters:
  - `p` (pointer): New pointer to manage (may be nullptr)
- Details: Reset to new pointer and free old one. p New pointer to manage (may be nullptr)

#### `void swap(unique_gpu_ptr &other) noexcept`
- Source: `include/themis/gpu/gpu_memory.h`:287
- Brief: Swap ownership with another unique_gpu_ptr.
- Parameters:
  - `other` (unique_gpu_ptr &): n/a

#### `constexpr unique_gpu_ptr() noexcept`
- Source: `include/themis/gpu/gpu_memory.h`:149
- Brief: Default constructor: null pointer.
- Parameters: none

#### `unique_gpu_ptr(const unique_gpu_ptr &)=delete`
- Source: `include/themis/gpu/gpu_memory.h`:178
- Brief: Deleted copy constructor: prevent accidental duplication.
- Parameters:
  - `<unnamed>` (const unique_gpu_ptr &): n/a

#### `unique_gpu_ptr(pointer p) noexcept`
- Source: `include/themis/gpu/gpu_memory.h`:152
- Brief: Construct from raw pointer (takes ownership).
- Parameters:
  - `p` (pointer): n/a

#### `unique_gpu_ptr(std::nullptr_t) noexcept`
- Source: `include/themis/gpu/gpu_memory.h`:155
- Brief: Construct from nullptr.
- Parameters:
  - `<unnamed>` (std::nullptr_t): n/a

#### `unique_gpu_ptr(unique_gpu_ptr &&other) noexcept`
- Source: `include/themis/gpu/gpu_memory.h`:163
- Brief: Move constructor: transfers ownership.
- Parameters:
  - `other` (unique_gpu_ptr &&): n/a

#### `~unique_gpu_ptr() noexcept`
- Source: `include/themis/gpu/gpu_memory.h`:158
- Brief: Destructor: calls deleter on owned pointer.
- Parameters: none

### themis::license

#### `std::vector< uint8_t > base64Decode(const std::string &encoded)`
- Source: `src/themis/license_info.cpp`:351
- Brief: Helper: Base64 decode.
- Parameters:
  - `encoded` (const std::string &): Input parameter.
- Return: Return value.
- Details: encoded Input parameter. Return value. Calls: BIO_new_mem_buf(), data(), size(), BIO_new(), BIO_f_base64(), BIO_free(), BIO_set_flags(), themis::utils::BIOPtr().

#### `std::string computeFingerprintHash(const std::string &raw)`
- Source: `src/themis/license_info.cpp`:472
- Brief: Compute a hex-encoded SHA-256 of the primary MAC address (or a fallback).
- Parameters:
  - `raw` (const std::string &): Input parameter.
- Return: Return value.
- Details: raw Input parameter. Return value. Calls: EVP_MD_CTX_new(), EVP_DigestInit_ex(), EVP_sha256(), EVP_DigestUpdate(), data(), size(), EVP_DigestFinal_ex(), EVP_MD_CTX_free().

#### `THEMIS_BASE_API std::string formatLicenseInfo(const LicenseData &license)`
- Source: `src/themis/license_info.cpp`:202
- Brief: Format License Info.
- Parameters:
  - `license` (const LicenseData &): Input parameter.
- Return: Return value.
- Details: Get a human-readable summary of the license information suitable for logging at server startup license Input parameter. Return value. Calls: empty(), getDaysUntilExpiry(), verifyLicenseSignature(), substr(), str().

#### `THEMIS_BASE_API int getDaysUntilExpiry(const LicenseData &license)`
- Source: `src/themis/license_info.cpp`:301
- Brief: Get Days Until Expiry.
- Parameters:
  - `license` (const LicenseData &): Input parameter.
- Return: Return value.
- Details: Get number of days until license expires Returns negative value if already expired license Input parameter. Return value. Calls: empty(), ss(), std::get_time(), fail(), std::chrono::system_clock::now(), std::chrono::system_clock::to_time_t(), gmtime_s(), gmtime_r().

#### `THEMIS_BASE_API std::optional< LicenseData > getEmbeddedLicense()`
- Source: `src/themis/license_info.cpp`:161
- Brief: Get Embedded License.
- Parameters: none
- Return: Return value.
- Details: Get the embedded license data from this build Returns empty optional if no license data was embedded Return value. Calls: empty().

#### `std::string getPrimaryMacAddress()`
- Source: `src/themis/license_info.cpp`:494
- Brief: Get Primary Mac Address.
- Parameters: none
- Return: Return value.
- Details: Return value. Calls: defined(), getifaddrs(), std::string(), socket(), std::strncpy(), ioctl(), std::snprintf(), close().

#### `THEMIS_BASE_API bool hasEmbeddedLicense()`
- Source: `src/themis/license_info.cpp`:191
- Brief: Has Embedded License.
- Parameters: none
- Return: True when the operation succeeds.
- Details: Check if this build has embedded license data True when the operation succeeds. Calls: empty().

#### `THEMIS_BASE_API bool isLicenseValid(const LicenseData &license)`
- Source: `src/themis/license_info.cpp`:291
- Brief: Is License Valid.
- Parameters:
  - `license` (const LicenseData &): Input parameter.
- Return: True when the operation succeeds.
- Details: Verify license validity (expiry date check) Returns true if license is currently valid license Input parameter. True when the operation succeeds. Calls: getDaysUntilExpiry().

#### `THEMIS_BASE_API bool verifyLicenseSignature(const LicenseData &license)`
- Source: `src/themis/license_info.cpp`:375
- Brief: Verify License Signature.
- Parameters:
  - `license` (const LicenseData &): Input parameter.
- Return: True when the operation succeeds.
- Details: Verify license signature (if present) Returns true if signature is valid or no signature present license Input parameter. True when the operation succeeds. Calls: std::toupper(), empty(), str(), themis::utils::make_bio_mem_buf(), themis::utils::EVPKeyPtr(), PEM_read_bio_PUBKEY(), get(), base64Decode().

### themis::license::GateResult

#### `std::string message() const`
- Source: `include/themis/runtime_license_gate.h`:98
- Brief: Returns a locale-independent English description of the denial reason suitable for logging.
- Parameters: none
- Details: Returns "Feature is allowed." when allowed is true.

#### `operator bool() const noexcept`
- Source: `include/themis/runtime_license_gate.h`:101
- Brief: Convenience implicit conversion to bool.
- Parameters: none

### themis::license::LicenseClient

#### `LicenseClient(const LicenseClient &)=delete`
- Source: `include/themis/license_info.h`:150
- Brief: n/a
- Parameters:
  - `<unnamed>` (const LicenseClient &): n/a

#### `LicenseClient(const LicenseClientConfig &config)`
- Source: `include/themis/license_info.h`:146
- Brief: n/a
- Parameters:
  - `config` (const LicenseClientConfig &): n/a

#### `LicenseActivationResult activate()`
- Source: `include/themis/license_info.h`:162
- Brief: Activate the license with the server.
- Parameters: none
- Return: Activation result with status and optional refreshed license.
- Details: Activate. Sends the embedded license key + machine fingerprint to the license server. On success the server returns an authoritative LicenseData (possibly with an updated expiry date). Activation result with status and optional refreshed license. Return value. Implements activate without additional internal calls.

#### `std::optional< LicenseData > getCachedLicense() const`
- Source: `include/themis/license_info.h`:178
- Brief: Return the currently cached license, if any.
- Parameters: none

#### `std::string getMachineFingerprint()`
- Source: `include/themis/license_info.h`:191
- Brief: Machine fingerprint used to bind licenses.
- Parameters: none
- Return: Return value.
- Details: Get Machine Fingerprint. Computed from stable hardware identifiers (MAC address, CPU ID, etc.). This value is sent to the license server during activation. Return value. Implements getMachineFingerprint without additional internal calls.

#### `LicenseClient & operator=(const LicenseClient &)=delete`
- Source: `include/themis/license_info.h`:151
- Brief: n/a
- Parameters:
  - `<unnamed>` (const LicenseClient &): n/a

#### `LicenseActivationResult refresh()`
- Source: `include/themis/license_info.h`:183
- Brief: Force-refresh the cached license from the server.
- Parameters: none
- Return: Return value.
- Details: Refresh. Return value. Implements refresh without additional internal calls.

#### `LicenseActivationResult validate()`
- Source: `include/themis/license_info.h`:173
- Brief: Validate the currently active license (online or offline).
- Parameters: none
- Return: Validation result.
- Details: Validate. If an online validation was previously successful and the cached result is still fresh, this returns immediately. Otherwise it re-contacts the server. Falls back to grace/offline mode as configured. Validation result. Return value. Implements validate without additional internal calls.

#### `~LicenseClient()`
- Source: `include/themis/license_info.h`:147
- Brief: n/a
- Parameters: none

### themis::license::LicenseClient::Impl

#### `Impl(const LicenseClientConfig &cfg)`
- Source: `src/themis/license_info.cpp`:556
- Brief: n/a
- Parameters:
  - `cfg` (const LicenseClientConfig &): n/a

#### `LicenseActivationResult activate()`
- Source: `src/themis/license_info.cpp`:563
- Brief: Activate.
- Parameters: none
- Return: Return value.
- Details: Return value. Calls: getEmbeddedLicense(), empty(), performOnlineRequest(), isLicenseValid(), lock(), std::chrono::steady_clock::now().

#### `std::optional< LicenseData > getCachedLicense() const`
- Source: `src/themis/license_info.cpp`:632
- Brief: n/a
- Parameters: none

#### `std::string getMachineFingerprint()`
- Source: `src/themis/license_info.cpp`:660
- Brief: Get Machine Fingerprint.
- Parameters: none
- Return: Return value.
- Details: Return value. Calls: getPrimaryMacAddress(), computeFingerprintHash().

#### `LicenseActivationResult handleOfflineFallback(const LicenseData &license, LicenseActivationResult &base)`
- Source: `src/themis/license_info.cpp`:833
- Brief: Handle Offline Fallback.
- Parameters:
  - `license` (const LicenseData &): Input parameter.
  - `base` (LicenseActivationResult &): Input/output parameter.
- Return: Return value.
- Details: license Input parameter. base Input/output parameter. Return value. Calls: lock(), std::chrono::steady_clock::now(), count(), std::to_string(), isLicenseValid().

#### `LicenseActivationResult refresh()`
- Source: `src/themis/license_info.cpp`:647
- Brief: Refresh.
- Parameters: none
- Return: Return value.
- Details: Return value. Calls: lock(), activate().

#### `LicenseActivationResult validate()`
- Source: `src/themis/license_info.cpp`:610
- Brief: Validate.
- Parameters: none
- Return: Return value.
- Details: Return value. Calls: lock(), std::chrono::steady_clock::now(), std::chrono::hours(), isLicenseValid(), activate().

### themis::license::LicenseInfo

#### `LicenseInfo(const LicenseData &data, int grace_period_days=kDefaultGracePeriodDays)`
- Source: `include/themis/license_info.h`:229
- Brief: Construct a LicenseInfo view over data.
- Parameters:
  - `data` (const LicenseData &): The license data to inspect.
  - `grace_period_days` (int): The configured grace period (days). Defaults to kDefaultGracePeriodDays.
- Details: data The license data to inspect. grace_period_days The configured grace period (days). Defaults to kDefaultGracePeriodDays.

#### `const LicenseData & data() const noexcept`
- Source: `include/themis/license_info.h`:249
- Brief: Accessor for the underlying data.
- Parameters: none

#### `int remaining_grace_days() const`
- Source: `include/themis/license_info.h`:246
- Brief: Returns the number of days remaining in the grace window.
- Parameters: none
- Return: Non-negative integer days remaining in grace period.
- Details: Interpretation: If the license has not yet expired, returns grace_period_days (the full grace window is available after expiry). If the license has expired and the expiry occurred within the grace window, returns the remaining days in that window. If the license has fully expired (beyond the grace window), returns 0. If expiry_date is empty or unparseable, returns 0. Non-negative integer days remaining in grace period.

### themis::license::RuntimeLicenseGate

#### `RuntimeLicenseGate()=default`
- Source: `include/themis/runtime_license_gate.h`:227
- Brief: n/a
- Parameters: none

#### `RuntimeLicenseGate(const RuntimeLicenseGate &)=delete`
- Source: `include/themis/runtime_license_gate.h`:135
- Brief: n/a
- Parameters:
  - `<unnamed>` (const RuntimeLicenseGate &): n/a

#### `std::string buildDenialMessage(std::string_view feature_name) const`
- Source: `include/themis/runtime_license_gate.h`:231
- Brief: Build a user-facing denial message for a blocked feature.
- Parameters:
  - `feature_name` (std::string_view): n/a

#### `GateResult checkFeature(std::string_view feature_name) const`
- Source: `include/themis/runtime_license_gate.h`:207
- Brief: Check whether a named feature is allowed, returning a GateResult that includes the structured denial reason for programmatic handling.
- Parameters:
  - `feature_name` (std::string_view): Feature name (see edition::IsFeatureEnabled()).
- Return: GateResult with allowed flag and denial_reason set.
- Details: Prefer this over isFeatureAllowed() when the caller needs to distinguish between different denial reasons (e.g. show an upgrade prompt for TIER_TOO_LOW vs. a "renew your license" prompt for LICENSE_EXPIRED). feature_name Feature name (see edition::IsFeatureEnabled()). GateResult with allowed flag and denial_reason set.

#### `std::optional< LicenseData > currentLicense() const`
- Source: `include/themis/runtime_license_gate.h`:224
- Brief: Returns the active license data (if available).
- Parameters: none

#### `int graceDaysRemaining() const`
- Source: `include/themis/runtime_license_gate.h`:221
- Brief: Returns the number of grace days remaining (0 if not in grace period).
- Parameters: none

#### `void initialize(const LicenseActivationResult &activation, const std::optional< LicenseData > &license=std::nullopt)`
- Source: `include/themis/runtime_license_gate.h`:153
- Brief: Initialise the gate with the result of a LicenseClient::activate() or validate() call.
- Parameters:
  - `activation` (const LicenseActivationResult &): Result from LicenseClient::activate() / validate().
  - `license` (const std::optional< LicenseData > &): Optional authoritative license data (may come from activation.refreshed_license or getEmbeddedLicense()).
- Details: Must be called once at server startup (after license validation) before any feature checks are performed. Thread-safe. activation Result from LicenseClient::activate() / validate(). license Optional authoritative license data (may come from activation.refreshed_license or getEmbeddedLicense()).

#### `RuntimeLicenseGate & instance()`
- Source: `include/themis/runtime_license_gate.h`:132
- Brief: Retrieve the process-wide singleton.
- Parameters: none

#### `bool isFeatureAllowed(std::string_view feature_name) const`
- Source: `include/themis/runtime_license_gate.h`:183
- Brief: Returns true if the named feature is allowed at runtime.
- Parameters:
  - `feature_name` (std::string_view): Feature name (see edition::IsFeatureEnabled()).
- Return: true if the feature may be used; false if blocked.
- Details: The following features are gated: "enterprise_plugins" "multi_master" "field_encryption" "rbac" "hsm" Features not on the list (e.g. Community-edition features) are always allowed — the gate only restricts Enterprise/Hyperscaler capabilities. feature_name Feature name (see edition::IsFeatureEnabled()). true if the feature may be used; false if blocked.

#### `bool isFeatureAllowed(std::string_view feature_name, std::string &error_out) const`
- Source: `include/themis/runtime_license_gate.h`:193
- Brief: Like isFeatureAllowed() but also sets error_out on failure with a human-readable message (license key, expiry info, contact email).
- Parameters:
  - `feature_name` (std::string_view): Feature to check.
  - `error_out` (std::string &): Populated with an error string when returning false.
- Return: true if allowed; false with error_out filled in if blocked.
- Details: feature_name Feature to check. error_out Populated with an error string when returning false. true if allowed; false with error_out filled in if blocked.

#### `bool isInitialized() const`
- Source: `include/themis/runtime_license_gate.h`:214
- Brief: Returns true if the gate has been initialized.
- Parameters: none

#### `std::string licenseStatus() const`
- Source: `include/themis/runtime_license_gate.h`:218
- Brief: n/a
- Parameters: none
- Details: Returns the current license status string ("active", "grace", "expired", "invalid", "offline", …).

#### `RuntimeLicenseGate & operator=(const RuntimeLicenseGate &)=delete`
- Source: `include/themis/runtime_license_gate.h`:136
- Brief: n/a
- Parameters:
  - `<unnamed>` (const RuntimeLicenseGate &): n/a

#### `void update(const LicenseActivationResult &activation, const std::optional< LicenseData > &license=std::nullopt)`
- Source: `include/themis/runtime_license_gate.h`:160
- Brief: Re-initialise the gate (used after a periodic license refresh). Equivalent to initialize().
- Parameters:
  - `activation` (const LicenseActivationResult &): n/a
  - `license` (const std::optional< LicenseData > &): n/a

#### `~RuntimeLicenseGate()=default`
- Source: `include/themis/runtime_license_gate.h`:228
- Brief: n/a
- Parameters: none

### themis::llm

#### `std::shared_ptr< IDocsAssistant > createDocsAssistant()`
- Source: `include/themis/llm/llm_factory.h`:37
- Brief: n/a
- Parameters: none

#### `std::shared_ptr< IEmbeddedLLM > createEmbeddedLLM()`
- Source: `include/themis/llm/llm_factory.h`:38
- Brief: n/a
- Parameters: none

#### `std::shared_ptr< ILLMModelAuditLogger > createLLMModelAuditLogger()`
- Source: `include/themis/llm/llm_factory.h`:41
- Brief: n/a
- Parameters: none

#### `std::shared_ptr< themis::llm::ILLMPluginManager > createLLMPluginManager()`
- Source: `include/themis/llm/llm_factory.h`:42
- Brief: n/a
- Parameters: none

#### `std::shared_ptr< ILlamaWrapper > createLlamaWrapper()`
- Source: `include/themis/llm/llm_factory.h`:40
- Brief: n/a
- Parameters: none

#### `std::shared_ptr< themis::llm::lora::ILoRAOrchestrator > createLoRAOrchestrator()`
- Source: `include/themis/llm/llm_factory.h`:43
- Brief: n/a
- Parameters: none

#### `std::shared_ptr< IThemisHelpLoRA > createThemisHelpLoRA()`
- Source: `include/themis/llm/llm_factory.h`:39
- Brief: n/a
- Parameters: none

#### `void registerDocsAssistantFactory(DocsAssistantFactory f)`
- Source: `include/themis/llm/llm_factory.h`:27
- Brief: n/a
- Parameters:
  - `f` (DocsAssistantFactory): n/a

#### `void registerEmbeddedLLMFactory(EmbeddedLLMFactory f)`
- Source: `include/themis/llm/llm_factory.h`:28
- Brief: n/a
- Parameters:
  - `f` (EmbeddedLLMFactory): n/a

#### `void registerLLMModelAuditLoggerFactory(LLMModelAuditLoggerFactory f)`
- Source: `include/themis/llm/llm_factory.h`:31
- Brief: n/a
- Parameters:
  - `f` (LLMModelAuditLoggerFactory): n/a

#### `void registerLLMPluginManagerFactory(LLMPluginManagerFactory f)`
- Source: `include/themis/llm/llm_factory.h`:32
- Brief: n/a
- Parameters:
  - `f` (LLMPluginManagerFactory): n/a

#### `void registerLlamaWrapperFactory(LlamaWrapperFactory f)`
- Source: `include/themis/llm/llm_factory.h`:30
- Brief: n/a
- Parameters:
  - `f` (LlamaWrapperFactory): n/a

#### `void registerLoRAOrchestratorFactory(LoRAOrchestratorFactory f)`
- Source: `include/themis/llm/llm_factory.h`:33
- Brief: n/a
- Parameters:
  - `f` (LoRAOrchestratorFactory): n/a

#### `void registerThemisHelpLoRAFactory(ThemisHelpLoRAFactory f)`
- Source: `include/themis/llm/llm_factory.h`:29
- Brief: n/a
- Parameters:
  - `f` (ThemisHelpLoRAFactory): n/a

### themis::llm::IDocsAssistant

#### `void clearCache()=0`
- Source: `include/themis/llm/llm_interfaces.h`:32
- Brief: n/a
- Parameters: none

#### `themis::llm::DocsQueryResult getConfigHelp(const std::string &topic)=0`
- Source: `include/themis/llm/llm_interfaces.h`:33
- Brief: n/a
- Parameters:
  - `topic` (const std::string &): n/a

#### `nlohmann::json getStats() const =0`
- Source: `include/themis/llm/llm_interfaces.h`:35
- Brief: n/a
- Parameters: none

#### `themis::llm::DocsQueryResult getTroubleshootingHelp(const std::string &topic)=0`
- Source: `include/themis/llm/llm_interfaces.h`:34
- Brief: n/a
- Parameters:
  - `topic` (const std::string &): n/a

#### `bool isReady() const =0`
- Source: `include/themis/llm/llm_interfaces.h`:30
- Brief: n/a
- Parameters: none

#### `bool loadDatabase(const std::string &path)=0`
- Source: `include/themis/llm/llm_interfaces.h`:29
- Brief: n/a
- Parameters:
  - `path` (const std::string &): n/a

#### `std::string query(const std::string &q)=0`
- Source: `include/themis/llm/llm_interfaces.h`:31
- Brief: n/a
- Parameters:
  - `q` (const std::string &): n/a

#### `~IDocsAssistant()=default`
- Source: `include/themis/llm/llm_interfaces.h`:28
- Brief: n/a
- Parameters: none

### themis::llm::IEmbeddedLLM

#### `std::string chat(const std::vector< ChatMessage > &messages, ChatFormat format)=0`
- Source: `include/themis/llm/llm_interfaces.h`:47
- Brief: n/a
- Parameters:
  - `messages` (const std::vector< ChatMessage > &): n/a
  - `format` (ChatFormat): n/a

#### `void clearCache()=0`
- Source: `include/themis/llm/llm_interfaces.h`:54
- Brief: n/a
- Parameters: none

#### `std::vector< float > embed(const std::string &text)=0`
- Source: `include/themis/llm/llm_interfaces.h`:44
- Brief: n/a
- Parameters:
  - `text` (const std::string &): n/a

#### `std::vector< std::vector< float > > embedBatch(const std::vector< std::string > &texts)=0`
- Source: `include/themis/llm/llm_interfaces.h`:45
- Brief: n/a
- Parameters:
  - `texts` (const std::vector< std::string > &): n/a

#### `std::string generate(const std::string &prompt)=0`
- Source: `include/themis/llm/llm_interfaces.h`:42
- Brief: n/a
- Parameters:
  - `prompt` (const std::string &): n/a

#### `nlohmann::json generateAsJsonMarkdown(const std::string &prompt, int max_tokens)=0`
- Source: `include/themis/llm/llm_interfaces.h`:49
- Brief: n/a
- Parameters:
  - `prompt` (const std::string &): n/a
  - `max_tokens` (int): n/a

#### `nlohmann::json generateAsMCP(const std::string &prompt, int max_tokens)=0`
- Source: `include/themis/llm/llm_interfaces.h`:48
- Brief: n/a
- Parameters:
  - `prompt` (const std::string &): n/a
  - `max_tokens` (int): n/a

#### `InferenceResponse generateFull(const InferenceRequest &request)=0`
- Source: `include/themis/llm/llm_interfaces.h`:50
- Brief: n/a
- Parameters:
  - `request` (const InferenceRequest &): n/a

#### `void generateStreaming(const std::string &prompt, const std::function< void(const std::string &)> &token_cb)=0`
- Source: `include/themis/llm/llm_interfaces.h`:43
- Brief: n/a
- Parameters:
  - `prompt` (const std::string &): n/a
  - `token_cb` (const std::function< void(const std::string &)> &): n/a

#### `std::string generateWithParams(const std::string &prompt, float temperature, float top_p, int max_tokens)=0`
- Source: `include/themis/llm/llm_interfaces.h`:46
- Brief: n/a
- Parameters:
  - `prompt` (const std::string &): n/a
  - `temperature` (float): n/a
  - `top_p` (float): n/a
  - `max_tokens` (int): n/a

#### `EthicalGuidelinesManager * getEthicalGuidelines()=0`
- Source: `include/themis/llm/llm_interfaces.h`:52
- Brief: n/a
- Parameters: none

#### `std::string getModelInfo() const =0`
- Source: `include/themis/llm/llm_interfaces.h`:55
- Brief: n/a
- Parameters: none

#### `nlohmann::json getStats() const =0`
- Source: `include/themis/llm/llm_interfaces.h`:53
- Brief: n/a
- Parameters: none

#### `bool hasEthicalGuidelines() const =0`
- Source: `include/themis/llm/llm_interfaces.h`:51
- Brief: n/a
- Parameters: none

#### `bool isReady() const =0`
- Source: `include/themis/llm/llm_interfaces.h`:41
- Brief: n/a
- Parameters: none

#### `~IEmbeddedLLM()=default`
- Source: `include/themis/llm/llm_interfaces.h`:40
- Brief: n/a
- Parameters: none

### themis::llm::ILLMModelAuditLogger

#### `void logRequest(const std::string &req)=0`
- Source: `include/themis/llm/llm_interfaces.h`:79
- Brief: n/a
- Parameters:
  - `req` (const std::string &): n/a

#### `~ILLMModelAuditLogger()=default`
- Source: `include/themis/llm/llm_interfaces.h`:78
- Brief: n/a
- Parameters: none

### themis::llm::ILLMPluginManager

#### `InferenceResponse generate(const InferenceRequest &req)=0`
- Source: `include/themis/llm/llm_plugin_manager.h`:15
- Brief: n/a
- Parameters:
  - `req` (const InferenceRequest &): n/a

#### `std::string pluginVersion() const =0`
- Source: `include/themis/llm/llm_plugin_manager.h`:16
- Brief: n/a
- Parameters: none

#### `~ILLMPluginManager()=default`
- Source: `include/themis/llm/llm_plugin_manager.h`:14
- Brief: n/a
- Parameters: none

### themis::llm::ILLMResourcePolicy

#### `ILLMResourcePolicy()=default`
- Source: `include/themis/llm/illm_resource_policy.h`:155
- Brief: n/a
- Parameters: none

#### `ILLMResourcePolicy(ILLMResourcePolicy &&) noexcept=default`
- Source: `include/themis/llm/illm_resource_policy.h`:156
- Brief: n/a
- Parameters:
  - `<unnamed>` (ILLMResourcePolicy &&): n/a

#### `ILLMResourcePolicy(const ILLMResourcePolicy &)=delete`
- Source: `include/themis/llm/illm_resource_policy.h`:50
- Brief: n/a
- Parameters:
  - `<unnamed>` (const ILLMResourcePolicy &): n/a

#### `int32_t activeModelInstances() const =0`
- Source: `include/themis/llm/illm_resource_policy.h`:121
- Brief: Return the number of model instances currently tracked as active.
- Parameters: none

#### `bool canAllocateContext(int64_t token_count) const =0`
- Source: `include/themis/llm/illm_resource_policy.h`:68
- Brief: Return true iff an inference request for token_count tokens is permitted under the current policy.
- Parameters:
  - `token_count` (int64_t): Total token budget (prompt + completion) requested.
- Return: true when the request fits within the policy.
- Details: Does not modify any state. A value of 0 for the policy's maxContextTokens() means unlimited — this method must return true for any positive token_count in that case. token_count Total token budget (prompt + completion) requested. true when the request fits within the policy.

#### `bool canAllocateModelVRAM(int64_t vram_mb) const =0`
- Source: `include/themis/llm/illm_resource_policy.h`:133
- Brief: Return true iff allocating vram_mb MiB for a single model is permitted.
- Parameters:
  - `vram_mb` (int64_t): VRAM to allocate in mebibytes.
- Return: true when the allocation fits within the per-model ceiling.
- Details: vram_mb VRAM to allocate in mebibytes. true when the allocation fits within the per-model ceiling.

#### `bool canLoadModel() const =0`
- Source: `include/themis/llm/illm_resource_policy.h`:90
- Brief: Return true iff loading one more model instance is permitted.
- Parameters: none
- Return: true when another model instance may be loaded.
- Details: Should consult activeModelInstances() to make the decision. Does not modify accounting state — call onModelLoaded() only after the load succeeds. true when another model instance may be loaded.

#### `bool isLLMEnabled() const noexcept=0`
- Source: `include/themis/llm/illm_resource_policy.h`:152
- Brief: Return true when any LLM acceleration is permitted by this policy.
- Parameters: none
- Details: Implementations should return false when all model-instance or context-token limits are exhausted and no further work can be dispatched.

#### `int64_t maxContextTokens() const noexcept=0`
- Source: `include/themis/llm/illm_resource_policy.h`:75
- Brief: Declared maximum context-token budget this policy allows per inference.
- Parameters: none
- Return: Maximum token count; 0 signals unlimited.
- Details: Maximum token count; 0 signals unlimited.

#### `int32_t maxModelInstances() const noexcept=0`
- Source: `include/themis/llm/illm_resource_policy.h`:97
- Brief: Declared maximum number of concurrently loaded model instances.
- Parameters: none
- Return: Maximum instance count; -1 signals unlimited.
- Details: Maximum instance count; -1 signals unlimited.

#### `int64_t maxVRAMPerModelMB() const noexcept=0`
- Source: `include/themis/llm/illm_resource_policy.h`:140
- Brief: Declared maximum VRAM per model instance in mebibytes.
- Parameters: none
- Return: VRAM ceiling in MiB; 0 signals unlimited.
- Details: VRAM ceiling in MiB; 0 signals unlimited.

#### `void onModelLoaded(const std::string &model_id)=0`
- Source: `include/themis/llm/illm_resource_policy.h`:106
- Brief: Notify the policy that a model with model_id has been loaded.
- Parameters:
  - `model_id` (const std::string &): Unique model identifier (non-empty string).
- Details: Updates internal accounting. Thread-safe. model_id Unique model identifier (non-empty string).

#### `void onModelUnloaded(const std::string &model_id)=0`
- Source: `include/themis/llm/illm_resource_policy.h`:116
- Brief: Notify the policy that the model model_id has been unloaded.
- Parameters:
  - `model_id` (const std::string &): Model identifier previously passed to onModelLoaded().
- Details: Updates internal accounting. Implementations must clamp to zero on mismatched calls to prevent underflow. Thread-safe. model_id Model identifier previously passed to onModelLoaded().

#### `ILLMResourcePolicy & operator=(ILLMResourcePolicy &&) noexcept=default`
- Source: `include/themis/llm/illm_resource_policy.h`:157
- Brief: n/a
- Parameters:
  - `<unnamed>` (ILLMResourcePolicy &&): n/a

#### `ILLMResourcePolicy & operator=(const ILLMResourcePolicy &)=delete`
- Source: `include/themis/llm/illm_resource_policy.h`:51
- Brief: n/a
- Parameters:
  - `<unnamed>` (const ILLMResourcePolicy &): n/a

#### `~ILLMResourcePolicy()=default`
- Source: `include/themis/llm/illm_resource_policy.h`:47
- Brief: n/a
- Parameters: none

### themis::llm::ILlamaWrapper

#### `std::string formatAsMCPResponse(const std::string &raw)=0`
- Source: `include/themis/llm/llm_interfaces.h`:73
- Brief: n/a
- Parameters:
  - `raw` (const std::string &): n/a

#### `bool loadModel(const std::string &path)=0`
- Source: `include/themis/llm/llm_interfaces.h`:71
- Brief: n/a
- Parameters:
  - `path` (const std::string &): n/a

#### `bool unloadModel()=0`
- Source: `include/themis/llm/llm_interfaces.h`:72
- Brief: n/a
- Parameters: none

#### `~ILlamaWrapper()=default`
- Source: `include/themis/llm/llm_interfaces.h`:70
- Brief: n/a
- Parameters: none

### themis::llm::IThemisHelpLoRA

#### `applications::FeedbackStats getFeedbackStats() const =0`
- Source: `include/themis/llm/llm_interfaces.h`:64
- Brief: n/a
- Parameters: none

#### `applications::PerformanceMetrics getMetrics() const =0`
- Source: `include/themis/llm/llm_interfaces.h`:63
- Brief: n/a
- Parameters: none

#### `std::string getVersion() const =0`
- Source: `include/themis/llm/llm_interfaces.h`:65
- Brief: n/a
- Parameters: none

#### `bool isTrained() const =0`
- Source: `include/themis/llm/llm_interfaces.h`:61
- Brief: n/a
- Parameters: none

#### `std::string query(const std::string &prompt)=0`
- Source: `include/themis/llm/llm_interfaces.h`:62
- Brief: n/a
- Parameters:
  - `prompt` (const std::string &): n/a

#### `~IThemisHelpLoRA()=default`
- Source: `include/themis/llm/llm_interfaces.h`:60
- Brief: n/a
- Parameters: none

### themis::llm::lora::ILoRAOrchestrator

#### `std::string createAdapter(const std::string &adapter_id, const TrainingData &data, const LoRAHyperparameters &params, bool async)=0`
- Source: `include/themis/llm/lora_orchestrator_interface.h`:23
- Brief: n/a
- Parameters:
  - `adapter_id` (const std::string &): n/a
  - `data` (const TrainingData &): n/a
  - `params` (const LoRAHyperparameters &): n/a
  - `async` (bool): n/a

#### `std::optional< nlohmann::json > getAdapter(const std::string &adapter_id) const =0`
- Source: `include/themis/llm/lora_orchestrator_interface.h`:33
- Brief: n/a
- Parameters:
  - `adapter_id` (const std::string &): n/a

#### `std::vector< nlohmann::json > getInferenceAuditLog(const std::string &adapter_id) const =0`
- Source: `include/themis/llm/lora_orchestrator_interface.h`:37
- Brief: n/a
- Parameters:
  - `adapter_id` (const std::string &): n/a

#### `std::optional< LoRAJobInfo > getJob(const std::string &job_id)=0`
- Source: `include/themis/llm/lora_orchestrator_interface.h`:27
- Brief: n/a
- Parameters:
  - `job_id` (const std::string &): n/a

#### `std::optional< nlohmann::json > getProvenanceRecord(const std::string &adapter_id) const =0`
- Source: `include/themis/llm/lora_orchestrator_interface.h`:36
- Brief: n/a
- Parameters:
  - `adapter_id` (const std::string &): n/a

#### `std::vector< std::string > getVersions(const std::string &adapter_id) const =0`
- Source: `include/themis/llm/lora_orchestrator_interface.h`:35
- Brief: n/a
- Parameters:
  - `adapter_id` (const std::string &): n/a

#### `bool isLoaded(const std::string &adapter_id) const =0`
- Source: `include/themis/llm/lora_orchestrator_interface.h`:28
- Brief: n/a
- Parameters:
  - `adapter_id` (const std::string &): n/a

#### `std::vector< nlohmann::json > listAdapterSnapshots(const std::string &adapter_id) const =0`
- Source: `include/themis/llm/lora_orchestrator_interface.h`:38
- Brief: n/a
- Parameters:
  - `adapter_id` (const std::string &): n/a

#### `void loadAdapter(const std::string &adapter_id, bool async)=0`
- Source: `include/themis/llm/lora_orchestrator_interface.h`:29
- Brief: n/a
- Parameters:
  - `adapter_id` (const std::string &): n/a
  - `async` (bool): n/a

#### `std::vector< nlohmann::json > searchAdapters(const nlohmann::json &criteria) const =0`
- Source: `include/themis/llm/lora_orchestrator_interface.h`:34
- Brief: n/a
- Parameters:
  - `criteria` (const nlohmann::json &): n/a

#### `bool verifyAuditChain(const std::string &adapter_id) const =0`
- Source: `include/themis/llm/lora_orchestrator_interface.h`:30
- Brief: n/a
- Parameters:
  - `adapter_id` (const std::string &): n/a

#### `~ILoRAOrchestrator()=default`
- Source: `include/themis/llm/lora_orchestrator_interface.h`:22
- Brief: n/a
- Parameters: none

### themis::modules

#### `SpanEmitter noOpSpanEmitter() noexcept`
- Source: `include/themis/base/trace_context.h`:173
- Brief: Return a no-op SpanEmitter that discards all events.
- Parameters: none
- Details: Used as the default emitter when tracing is not configured, ensuring zero overhead from tracing instrumentation in production paths.

### themis::modules::ABTestManager

#### `ABTestManager()`
- Source: `include/themis/base/ab_test_manager.h`:174
- Brief: n/a
- Parameters: none

#### `ABTestManager(HotReloadManager &reload_manager)`
- Source: `include/themis/base/ab_test_manager.h`:183
- Brief: Construct with an optional HotReloadManager for promotion support.
- Parameters:
  - `reload_manager` (HotReloadManager &): n/a
- Details: When a HotReloadManager is provided, promoteTest() will call HotReloadManager::reloadModule() to atomically swap the control binary with the treatment binary without a database restart.

#### `ABTestManager(const ABTestManager &)=delete`
- Source: `include/themis/base/ab_test_manager.h`:187
- Brief: n/a
- Parameters:
  - `<unnamed>` (const ABTestManager &): n/a

#### `double calculatePValue(double z_statistic)`
- Source: `include/themis/base/ab_test_manager.h`:419
- Brief: Approximate two-tailed p-value from z-statistic (A&S 26.2.17).
- Parameters:
  - `z_statistic` (double): n/a

#### `double calculateZStatistic(const ABVariantMetrics &ctrl, const ABVariantMetrics &trt)`
- Source: `include/themis/base/ab_test_manager.h`:415
- Brief: Two-proportion z-test statistic.
- Parameters:
  - `ctrl` (const ABVariantMetrics &): n/a
  - `trt` (const ABVariantMetrics &): n/a

#### `void cancelTest(const std::string &test_id)`
- Source: `include/themis/base/ab_test_manager.h`:284
- Brief: Cancel a test without making a promote/rollback decision.
- Parameters:
  - `test_id` (const std::string &): Test identifier.
- Details: Unloads the treatment module if it was loaded and marks the test CANCELLED. No-op if the test is not currently ACTIVE. test_id Test identifier.

#### `ABModuleTestResult evaluateTest(const std::string &test_id) const`
- Source: `include/themis/base/ab_test_manager.h`:343
- Brief: Compute a statistical evaluation snapshot for the test.
- Parameters:
  - `test_id` (const std::string &): Test identifier.
- Return: Statistical result including p-value and significance flag.
- Details: Performs a two-proportion z-test on the success rates once both groups have reached min_samples. Returns an empty result if the test is not found. test_id Test identifier. Statistical result including p-value and significance flag.

#### `std::vector< ABTestMetricRow > exportMetricsSnapshot() const`
- Source: `include/themis/base/ab_test_manager.h`:363
- Brief: Export a flat metric snapshot for all known tests.
- Parameters: none
- Return: One ABTestMetricRow per variant per test.
- Details: Returns two rows per test (control + treatment) with request/conversion counts, success rates, mean latency and estimated p99. Intended for the admin API. One ABTestMetricRow per variant per test.

#### `std::vector< std::string > getActiveTests() const`
- Source: `include/themis/base/ab_test_manager.h`:350
- Brief: n/a
- Parameters: none

#### `ABVariantMetrics getControlMetrics(const std::string &test_id) const`
- Source: `include/themis/base/ab_test_manager.h`:351
- Brief: n/a
- Parameters:
  - `test_id` (const std::string &): n/a

#### `ABTestStatus getTestStatus(const std::string &test_id) const`
- Source: `include/themis/base/ab_test_manager.h`:349
- Brief: n/a
- Parameters:
  - `test_id` (const std::string &): n/a

#### `ABVariantMetrics getTreatmentMetrics(const std::string &test_id) const`
- Source: `include/themis/base/ab_test_manager.h`:352
- Brief: n/a
- Parameters:
  - `test_id` (const std::string &): n/a

#### `size_t hashRequestKey(const std::string &key)`
- Source: `include/themis/base/ab_test_manager.h`:422
- Brief: Stable hash for deterministic request routing.
- Parameters:
  - `key` (const std::string &): n/a

#### `bool isTreatmentLoaded(const std::string &test_id) const`
- Source: `include/themis/base/ab_test_manager.h`:310
- Brief: Check whether the treatment binary was successfully loaded.
- Parameters:
  - `test_id` (const std::string &): n/a
- Return: true if the treatment is loaded, false otherwise.
- Details: true if the treatment is loaded, false otherwise.

#### `ABTestManager & operator=(const ABTestManager &)=delete`
- Source: `include/themis/base/ab_test_manager.h`:188
- Brief: n/a
- Parameters:
  - `<unnamed>` (const ABTestManager &): n/a

#### `void persistTestEntry(const std::string &test_id, const TestEntry &entry) const`
- Source: `include/themis/base/ab_test_manager.h`:433
- Brief: n/a
- Parameters:
  - `test_id` (const std::string &): n/a
  - `entry` (const TestEntry &): n/a
- Details: Serialize and write entry to storage under key "ab_test::<test_id>". No-op when storage_engine_ is null. Must NOT be called while holding mutex_.

#### `bool promoteTest(const std::string &test_id)`
- Source: `include/themis/base/ab_test_manager.h`:263
- Brief: Promote the treatment variant to production via hot-reload.
- Parameters:
  - `test_id` (const std::string &): Test identifier.
- Return: true on success, false if the test was not found or not ACTIVE.
- Details: If a HotReloadManager was provided at construction, calls HotReloadManager::reloadModule(module_name, treatment_path) to atomically replace the running control binary with the treatment binary. The treatment slot is then unloaded and the test status becomes PROMOTED. If no HotReloadManager was configured, the status is still set to PROMOTED (useful for tests that manage module lifetimes externally). test_id Test identifier. true on success, false if the test was not found or not ACTIVE.

#### `void recordOutcome(const std::string &test_id, bool is_treatment, bool success, double latency_ms=0.0)`
- Source: `include/themis/base/ab_test_manager.h`:324
- Brief: Record the outcome of a single dispatched request.
- Parameters:
  - `test_id` (const std::string &): Test identifier.
  - `is_treatment` (bool): true = outcome belongs to treatment group.
  - `success` (bool): Whether the request completed successfully.
  - `latency_ms` (double): Optional observed latency in milliseconds.
- Details: test_id Test identifier. is_treatment true = outcome belongs to treatment group. success Whether the request completed successfully. latency_ms Optional observed latency in milliseconds.

#### `bool rollbackTest(const std::string &test_id)`
- Source: `include/themis/base/ab_test_manager.h`:274
- Brief: Roll back: unload the treatment module and keep the control.
- Parameters:
  - `test_id` (const std::string &): Test identifier.
- Return: true on success, false if the test was not found or not ACTIVE.
- Details: Unloads the treatment binary and transitions the test to ROLLED_BACK. The control module remains running undisturbed. test_id Test identifier. true on success, false if the test was not found or not ACTIVE.

#### `void setMetricsCollector(observability::MetricsCollector *metrics)`
- Source: `include/themis/base/ab_test_manager.h`:214
- Brief: Attach a MetricsCollector for observability export (optional).
- Parameters:
  - `metrics` (observability::MetricsCollector *): Non-owning pointer to a MetricsCollector instance. Pass nullptr to disable metrics emission.
- Details: When set, recordOutcome() emits per-variant counters and gauges to the collector outside the internal mutex so the hot-path is not blocked. metrics Non-owning pointer to a MetricsCollector instance. Pass nullptr to disable metrics emission.

#### `void setStorageEngine(IStorageEngine *engine)`
- Source: `include/themis/base/ab_test_manager.h`:203
- Brief: Attach a storage engine for RocksDB persistence (optional).
- Parameters:
  - `engine` (IStorageEngine *): Non-owning pointer to an IStorageEngine implementation. Pass nullptr to disable persistence.
- Details: When set, test configs and metrics are persisted under the key prefix "ab_test::" so they survive server restarts. Call before start(). engine Non-owning pointer to an IStorageEngine implementation. Pass nullptr to disable persistence.

#### `bool shouldUseTreatment(const std::string &test_id, const std::string &request_key) const`
- Source: `include/themis/base/ab_test_manager.h`:302
- Brief: Determine which variant a request should be routed to.
- Parameters:
  - `test_id` (const std::string &): Test identifier.
  - `request_key` (const std::string &): Any string (user ID, session ID, query hash, …).
- Return: true → route to treatment, false → route to control (also returned when the test is not ACTIVE or not found).
- Details: Uses a stable hash of request_key so a given key is always assigned to the same variant (deterministic routing). test_id Test identifier. request_key Any string (user ID, session ID, query hash, …). true → route to treatment, false → route to control (also returned when the test is not ACTIVE or not found).

#### `void start()`
- Source: `include/themis/base/ab_test_manager.h`:227
- Brief: Load previously persisted test entries from storage.
- Parameters: none
- Details: Scans the "ab_test::" key prefix in the attached storage engine and restores config and accumulated metrics for each entry. Active tests are restored with treatment_loaded = false; callers should re-call startTest() with the same test_id to re-attach a module loader and reload the treatment binary while preserving the accumulated metrics. No-op when no storage engine has been set.

#### `bool startTest(const ABModuleTestConfig &config, ModuleLoader &loader)`
- Source: `include/themis/base/ab_test_manager.h`:247
- Brief: Register and start an A/B test.
- Parameters:
  - `config` (const ABModuleTestConfig &): Test configuration.
  - `loader` (ModuleLoader &): ModuleLoader to use for loading the treatment binary.
- Return: true if the test was registered; false if a test with the same test_id already exists.
- Details: Loads the treatment binary under treatmentKey(config.module_name) so both variants are simultaneously resident. If the treatment binary cannot be loaded (e.g. file not found), the test is still registered and routing will always fall back to control until the binary becomes available. config Test configuration. loader ModuleLoader to use for loading the treatment binary. true if the test was registered; false if a test with the same test_id already exists.

#### `double thompsonProbTreatmentWins(size_t ctrl_success, size_t ctrl_failure, size_t trt_success, size_t trt_failure)`
- Source: `include/themis/base/ab_test_manager.h`:450
- Brief: Estimate P(treatment_success_rate > control_success_rate) via the normal approximation of the Beta-Binomial posterior.
- Parameters:
  - `ctrl_success` (size_t): Control successes.
  - `ctrl_failure` (size_t): Control failures.
  - `trt_success` (size_t): Treatment successes.
  - `trt_failure` (size_t): Treatment failures.
- Details: Uses uniform Beta(1,1) priors. Returns a value in [0, 1]. ctrl_success Control successes. ctrl_failure Control failures. trt_success Treatment successes. trt_failure Treatment failures.

#### `std::string treatmentKey(const std::string &module_name)`
- Source: `include/themis/base/ab_test_manager.h`:378
- Brief: Internal module key used to register the treatment binary.
- Parameters:
  - `module_name` (const std::string &): Logical module name (e.g. "themis_storage").
- Return: Derived key (e.g. "themis_storage__ab_treatment__").
- Details: The treatment binary is loaded under this derived name so it can coexist with the control binary in the same ModuleLoader. module_name Logical module name (e.g. "themis_storage"). Derived key (e.g. "themis_storage__ab_treatment__").

#### `void unloadTreatment(TestEntry &entry)`
- Source: `include/themis/base/ab_test_manager.h`:425
- Brief: Unload the treatment binary for a test entry (caller must hold mutex_).
- Parameters:
  - `entry` (TestEntry &): n/a

#### `~ABTestManager()`
- Source: `include/themis/base/ab_test_manager.h`:185
- Brief: n/a
- Parameters: none

### themis::modules::AbiCheckResult

#### `bool hasIssues() const noexcept`
- Source: `include/themis/base/module_sandbox.h`:61
- Brief: n/a
- Parameters: none

### themis::modules::AbiChecker

#### `AbiChecker()`
- Source: `include/themis/base/module_sandbox.h`:98
- Brief: n/a
- Parameters: none

#### `void addDeprecatedSymbol(const std::string &symbol)`
- Source: `include/themis/base/module_sandbox.h`:116
- Brief: Add a symbol that was removed from the host ABI; warn if the new module still exports it (may indicate stale build).
- Parameters:
  - `symbol` (const std::string &): Symbol name to flag as deprecated in the host ABI.
- Details: symbol Symbol name to flag as deprecated in the host ABI.

#### `void addRequiredSymbol(const std::string &symbol)`
- Source: `include/themis/base/module_sandbox.h`:108
- Brief: Add a symbol that every valid module must export.
- Parameters:
  - `symbol` (const std::string &): Exported symbol name (null-terminated, mangled if C++).
- Details: symbol Exported symbol name (null-terminated, mangled if C++).

#### `AbiCheckResult check(void *module_handle, const ModuleMetadata &module_meta, uint32_t host_major, uint32_t host_minor) const`
- Source: `include/themis/base/module_sandbox.h`:139
- Brief: Run all ABI checks.
- Parameters:
  - `module_handle` (void *): OS handle to the newly loaded (but not yet active) module.
  - `module_meta` (const ModuleMetadata &): Metadata extracted from module_handle.
  - `host_major` (uint32_t): ThemisDB host ABI major version.
  - `host_minor` (uint32_t): ThemisDB host ABI minor version.
- Return: AbiCheckResult with pass/fail status and diagnostic message.
- Details: module_handle OS handle to the newly loaded (but not yet active) module. module_meta Metadata extracted from module_handle. host_major ThemisDB host ABI major version. host_minor ThemisDB host ABI minor version. AbiCheckResult with pass/fail status and diagnostic message.

#### `AbiCheckResult checkDeprecatedSymbols(void *handle) const`
- Source: `include/themis/base/module_sandbox.h`:172
- Brief: Warn if the module exports any symbols that are deprecated in the host ABI.
- Parameters:
  - `handle` (void *): OS handle to the loaded module.
- Return: AbiCheckResult listing any deprecated symbols still exported.
- Details: handle OS handle to the loaded module. AbiCheckResult listing any deprecated symbols still exported.

#### `AbiCheckResult checkRequiredSymbols(void *handle) const`
- Source: `include/themis/base/module_sandbox.h`:164
- Brief: Verify that all required symbols are present in the module.
- Parameters:
  - `handle` (void *): OS handle to the loaded module.
- Return: AbiCheckResult listing any missing required symbols.
- Details: handle OS handle to the loaded module. AbiCheckResult listing any missing required symbols.

#### `AbiCheckResult checkVersions(const ModuleMetadata &meta, uint32_t host_major, uint32_t host_minor) const`
- Source: `include/themis/base/module_sandbox.h`:154
- Brief: Verify that the module's ABI version is compatible with the host.
- Parameters:
  - `meta` (const ModuleMetadata &): Metadata extracted from the loaded module.
  - `host_major` (uint32_t): ThemisDB host ABI major version.
  - `host_minor` (uint32_t): ThemisDB host ABI minor version.
- Return: AbiCheckResult indicating version compatibility.
- Details: meta Metadata extracted from the loaded module. host_major ThemisDB host ABI major version. host_minor ThemisDB host ABI minor version. AbiCheckResult indicating version compatibility.

#### `void * resolveSymbol(void *handle, const std::string &name) noexcept`
- Source: `include/themis/base/module_sandbox.h`:179
- Brief: n/a
- Parameters:
  - `handle` (void *): n/a
  - `name` (const std::string &): n/a

#### `void useDefaultLists()`
- Source: `include/themis/base/module_sandbox.h`:126
- Brief: Convenience: load the default ThemisDB required/deprecated lists.
- Parameters: none
- Details: Required: themis_module_init, themis_module_shutdown, themis_module_version, themis_api_version_major, themis_api_version_minor Deprecated: (none in v1.x)

#### `~AbiChecker()`
- Source: `include/themis/base/module_sandbox.h`:99
- Brief: n/a
- Parameters: none

### themis::modules::BaseErrorTaxonomy

#### `bool isKnownCode(int error_code) noexcept`
- Source: `include/themis/base/base_error_taxonomy.h`:943
- Brief: Return true when code falls within any known taxonomy range.
- Parameters:
  - `error_code` (int): Integer error code to test.
- Details: error_code Integer error code to test.

#### `std::string_view resolveDescription(int error_code) noexcept`
- Source: `include/themis/base/base_error_taxonomy.h`:860
- Brief: Resolve a numeric error code to its taxonomy description at runtime.
- Parameters:
  - `error_code` (int): Integer error code (e.g. BASE_LOADER_ABI_MISMATCH::code).
- Return: Human-readable description string view or "unknown error code".
- Details: Checks every known code in the taxonomy and returns its description string. Returns "unknown error code" for values outside all known ranges. error_code Integer error code (e.g. BASE_LOADER_ABI_MISMATCH::code). Human-readable description string view or "unknown error code".

#### `std::string_view resolveRemediationHint(int error_code) noexcept`
- Source: `include/themis/base/base_error_taxonomy.h`:904
- Brief: Resolve a numeric error code to its operator remediation hint.
- Parameters:
  - `error_code` (int): Integer error code (e.g. BASE_LOADER_ABI_MISMATCH::code).
- Return: Actionable hint string view.
- Details: Returns a short, actionable string for each known taxonomy code. Returns "no remediation hint available" for unknown codes. error_code Integer error code (e.g. BASE_LOADER_ABI_MISMATCH::code). Actionable hint string view.

### themis::modules::BaseErrorTaxonomy::BASE_DEPENDENCY_CONFLICT

#### `std::string_view description() noexcept`
- Source: `include/themis/base/base_error_taxonomy.h`:591
- Brief: n/a
- Parameters: none

#### `std::string format(const std::string &dep_name, const std::string &mod_a, const std::string &mod_b)`
- Source: `include/themis/base/base_error_taxonomy.h`:608
- Brief: n/a
- Parameters:
  - `dep_name` (const std::string &): Name of the shared dependency.
  - `mod_a` (const std::string &): First requiring module and its constraint.
  - `mod_b` (const std::string &): Second requiring module and its constraint.
- Details: dep_name Name of the shared dependency. mod_a First requiring module and its constraint. mod_b Second requiring module and its constraint.

#### `std::string_view remediationHint() noexcept`
- Source: `include/themis/base/base_error_taxonomy.h`:596
- Brief: Actionable operator remediation hint.
- Parameters: none

### themis::modules::BaseErrorTaxonomy::BASE_DEPENDENCY_CYCLE

#### `std::string_view description() noexcept`
- Source: `include/themis/base/base_error_taxonomy.h`:624
- Brief: n/a
- Parameters: none

#### `std::string format(const std::string &cycle_str)`
- Source: `include/themis/base/base_error_taxonomy.h`:639
- Brief: n/a
- Parameters:
  - `cycle_str` (const std::string &): Human-readable cycle description, e.g. "A→B→C→A".
- Details: cycle_str Human-readable cycle description, e.g. "A→B→C→A".

#### `std::string_view remediationHint() noexcept`
- Source: `include/themis/base/base_error_taxonomy.h`:629
- Brief: Actionable operator remediation hint.
- Parameters: none

### themis::modules::BaseErrorTaxonomy::BASE_DEPENDENCY_MISSING_REQUIRED

#### `std::string_view description() noexcept`
- Source: `include/themis/base/base_error_taxonomy.h`:652
- Brief: n/a
- Parameters: none

#### `std::string format(const std::string &module_name, const std::string &dep_name)`
- Source: `include/themis/base/base_error_taxonomy.h`:667
- Brief: n/a
- Parameters:
  - `module_name` (const std::string &): Module that has the unsatisfied dependency.
  - `dep_name` (const std::string &): Name of the missing required dependency.
- Details: module_name Module that has the unsatisfied dependency. dep_name Name of the missing required dependency.

#### `std::string_view remediationHint() noexcept`
- Source: `include/themis/base/base_error_taxonomy.h`:657
- Brief: Actionable operator remediation hint.
- Parameters: none

### themis::modules::BaseErrorTaxonomy::BASE_DEPENDENCY_VERSION_RANGE_MISMATCH

#### `std::string_view description() noexcept`
- Source: `include/themis/base/base_error_taxonomy.h`:682
- Brief: n/a
- Parameters: none

#### `std::string format(const std::string &module_name, const std::string &dep_name, const std::string &have, const std::string &need_min, const std::string &need_max)`
- Source: `include/themis/base/base_error_taxonomy.h`:700
- Brief: n/a
- Parameters:
  - `module_name` (const std::string &): Requiring module.
  - `dep_name` (const std::string &): Dependency module name.
  - `have` (const std::string &): Registered version.
  - `need_min` (const std::string &): Required minimum version.
  - `need_max` (const std::string &): Required maximum version (empty = unconstrained).
- Details: module_name Requiring module. dep_name Dependency module name. have Registered version. need_min Required minimum version. need_max Required maximum version (empty = unconstrained).

#### `std::string_view remediationHint() noexcept`
- Source: `include/themis/base/base_error_taxonomy.h`:687
- Brief: Actionable operator remediation hint.
- Parameters: none

### themis::modules::BaseErrorTaxonomy::BASE_LOADER_ABI_MISMATCH

#### `std::string_view description() noexcept`
- Source: `include/themis/base/base_error_taxonomy.h`:140
- Brief: n/a
- Parameters: none

#### `std::string format(const std::string &module_name, const std::string &module_abi, const std::string &host_abi)`
- Source: `include/themis/base/base_error_taxonomy.h`:157
- Brief: Build a formatted diagnostic message.
- Parameters:
  - `module_name` (const std::string &): Logical module name.
  - `module_abi` (const std::string &): ABI version string reported by the module.
  - `host_abi` (const std::string &): ABI version string required by the host.
- Details: module_name Logical module name. module_abi ABI version string reported by the module. host_abi ABI version string required by the host.

#### `std::string_view remediationHint() noexcept`
- Source: `include/themis/base/base_error_taxonomy.h`:145
- Brief: Actionable operator remediation hint.
- Parameters: none

### themis::modules::BaseErrorTaxonomy::BASE_LOADER_HEALTH_CHECK_FAILED

#### `std::string_view description() noexcept`
- Source: `include/themis/base/base_error_taxonomy.h`:239
- Brief: n/a
- Parameters: none

#### `std::string format(const std::string &module_name, const std::string &check_name, const std::string &detail)`
- Source: `include/themis/base/base_error_taxonomy.h`:256
- Brief: n/a
- Parameters:
  - `module_name` (const std::string &): Logical module name.
  - `check_name` (const std::string &): Name of the health check that failed.
  - `detail` (const std::string &): Diagnostic detail from the health check.
- Details: module_name Logical module name. check_name Name of the health check that failed. detail Diagnostic detail from the health check.

#### `std::string_view remediationHint() noexcept`
- Source: `include/themis/base/base_error_taxonomy.h`:244
- Brief: Actionable operator remediation hint.
- Parameters: none

### themis::modules::BaseErrorTaxonomy::BASE_LOADER_INIT_FAILED

#### `std::string_view description() noexcept`
- Source: `include/themis/base/base_error_taxonomy.h`:206
- Brief: n/a
- Parameters: none

#### `std::string format(const std::string &module_name, const std::string &init_symbol, int return_code)`
- Source: `include/themis/base/base_error_taxonomy.h`:223
- Brief: n/a
- Parameters:
  - `module_name` (const std::string &): Logical module name.
  - `init_symbol` (const std::string &): Name of the init symbol that was called.
  - `return_code` (int): Return value from the init function.
- Details: module_name Logical module name. init_symbol Name of the init symbol that was called. return_code Return value from the init function.

#### `std::string_view remediationHint() noexcept`
- Source: `include/themis/base/base_error_taxonomy.h`:211
- Brief: Actionable operator remediation hint.
- Parameters: none

### themis::modules::BaseErrorTaxonomy::BASE_LOADER_LOAD_FAILED

#### `std::string_view description() noexcept`
- Source: `include/themis/base/base_error_taxonomy.h`:173
- Brief: n/a
- Parameters: none

#### `std::string format(const std::string &module_name, const std::string &path, const std::string &os_error)`
- Source: `include/themis/base/base_error_taxonomy.h`:190
- Brief: n/a
- Parameters:
  - `module_name` (const std::string &): Logical module name.
  - `path` (const std::string &): Path attempted.
  - `os_error` (const std::string &): dlerror() / GetLastError() message.
- Details: module_name Logical module name. path Path attempted. os_error dlerror() / GetLastError() message.

#### `std::string_view remediationHint() noexcept`
- Source: `include/themis/base/base_error_taxonomy.h`:178
- Brief: Actionable operator remediation hint.
- Parameters: none

### themis::modules::BaseErrorTaxonomy::BASE_LOADER_PATH_NOT_FOUND

#### `std::string_view description() noexcept`
- Source: `include/themis/base/base_error_taxonomy.h`:79
- Brief: One-line description.
- Parameters: none

#### `std::string format(const std::string &module_name, const std::string &path)`
- Source: `include/themis/base/base_error_taxonomy.h`:96
- Brief: Build a formatted diagnostic message.
- Parameters:
  - `module_name` (const std::string &): Logical module name.
  - `path` (const std::string &): Filesystem path that was not found.
- Return: Formatted diagnostic string.
- Details: module_name Logical module name. path Filesystem path that was not found. Formatted diagnostic string.

#### `std::string_view remediationHint() noexcept`
- Source: `include/themis/base/base_error_taxonomy.h`:84
- Brief: Actionable operator remediation hint.
- Parameters: none

### themis::modules::BaseErrorTaxonomy::BASE_LOADER_SIGNATURE_REJECTED

#### `std::string_view description() noexcept`
- Source: `include/themis/base/base_error_taxonomy.h`:110
- Brief: n/a
- Parameters: none

#### `std::string format(const std::string &module_name, const std::string &reason)`
- Source: `include/themis/base/base_error_taxonomy.h`:126
- Brief: Build a formatted diagnostic message.
- Parameters:
  - `module_name` (const std::string &): Logical module name.
  - `reason` (const std::string &): Human-readable rejection reason.
- Details: module_name Logical module name. reason Human-readable rejection reason.

#### `std::string_view remediationHint() noexcept`
- Source: `include/themis/base/base_error_taxonomy.h`:115
- Brief: Actionable operator remediation hint.
- Parameters: none

### themis::modules::BaseErrorTaxonomy::BASE_REGISTRY_AUTH_FAILURE

#### `std::string_view description() noexcept`
- Source: `include/themis/base/base_error_taxonomy.h`:756
- Brief: n/a
- Parameters: none

#### `std::string format(const std::string &registry_url, int http_status)`
- Source: `include/themis/base/base_error_taxonomy.h`:772
- Brief: n/a
- Parameters:
  - `registry_url` (const std::string &): URL of the registry.
  - `http_status` (int): HTTP status code (typically 401 or 403).
- Details: registry_url URL of the registry. http_status HTTP status code (typically 401 or 403).

#### `std::string_view remediationHint() noexcept`
- Source: `include/themis/base/base_error_taxonomy.h`:761
- Brief: Actionable operator remediation hint.
- Parameters: none

### themis::modules::BaseErrorTaxonomy::BASE_REGISTRY_CHECKSUM_MISMATCH

#### `std::string_view description() noexcept`
- Source: `include/themis/base/base_error_taxonomy.h`:787
- Brief: n/a
- Parameters: none

#### `std::string format(const std::string &plugin_name, const std::string &expected_sha256, const std::string &actual_sha256)`
- Source: `include/themis/base/base_error_taxonomy.h`:804
- Brief: n/a
- Parameters:
  - `plugin_name` (const std::string &): Plugin name.
  - `expected_sha256` (const std::string &): Expected SHA-256 from the registry manifest.
  - `actual_sha256` (const std::string &): Computed SHA-256 of the downloaded file.
- Details: plugin_name Plugin name. expected_sha256 Expected SHA-256 from the registry manifest. actual_sha256 Computed SHA-256 of the downloaded file.

#### `std::string_view remediationHint() noexcept`
- Source: `include/themis/base/base_error_taxonomy.h`:792
- Brief: Actionable operator remediation hint.
- Parameters: none

### themis::modules::BaseErrorTaxonomy::BASE_REGISTRY_DOWNLOAD_FAILED

#### `std::string_view description() noexcept`
- Source: `include/themis/base/base_error_taxonomy.h`:821
- Brief: n/a
- Parameters: none

#### `std::string format(const std::string &plugin_name, const std::string &download_url, const std::string &reason)`
- Source: `include/themis/base/base_error_taxonomy.h`:838
- Brief: n/a
- Parameters:
  - `plugin_name` (const std::string &): Plugin name.
  - `download_url` (const std::string &): Full download URL.
  - `reason` (const std::string &): I/O or file-system error description.
- Details: plugin_name Plugin name. download_url Full download URL. reason I/O or file-system error description.

#### `std::string_view remediationHint() noexcept`
- Source: `include/themis/base/base_error_taxonomy.h`:826
- Brief: Actionable operator remediation hint.
- Parameters: none

### themis::modules::BaseErrorTaxonomy::BASE_REGISTRY_NETWORK_ERROR

#### `std::string_view description() noexcept`
- Source: `include/themis/base/base_error_taxonomy.h`:723
- Brief: n/a
- Parameters: none

#### `std::string format(const std::string &registry_url, int http_status, const std::string &curl_error)`
- Source: `include/themis/base/base_error_taxonomy.h`:740
- Brief: n/a
- Parameters:
  - `registry_url` (const std::string &): URL of the registry.
  - `http_status` (int): HTTP status code (0 if no response received).
  - `curl_error` (const std::string &): cURL error string or OS-level error description.
- Details: registry_url URL of the registry. http_status HTTP status code (0 if no response received). curl_error cURL error string or OS-level error description.

#### `std::string_view remediationHint() noexcept`
- Source: `include/themis/base/base_error_taxonomy.h`:728
- Brief: Actionable operator remediation hint.
- Parameters: none

### themis::modules::BaseErrorTaxonomy::BASE_RELOAD_CANDIDATE_LOAD_FAILED

#### `std::string_view description() noexcept`
- Source: `include/themis/base/base_error_taxonomy.h`:497
- Brief: n/a
- Parameters: none

#### `std::string format(const std::string &module_name, const std::string &new_path, const std::string &reason)`
- Source: `include/themis/base/base_error_taxonomy.h`:514
- Brief: n/a
- Parameters:
  - `module_name` (const std::string &): Module name.
  - `new_path` (const std::string &): Path to the candidate binary.
  - `reason` (const std::string &): Load failure reason.
- Details: module_name Module name. new_path Path to the candidate binary. reason Load failure reason.

#### `std::string_view remediationHint() noexcept`
- Source: `include/themis/base/base_error_taxonomy.h`:502
- Brief: Actionable operator remediation hint.
- Parameters: none

### themis::modules::BaseErrorTaxonomy::BASE_RELOAD_NOT_REGISTERED

#### `std::string_view description() noexcept`
- Source: `include/themis/base/base_error_taxonomy.h`:559
- Brief: n/a
- Parameters: none

#### `std::string format(const std::string &module_name)`
- Source: `include/themis/base/base_error_taxonomy.h`:573
- Brief: n/a
- Parameters:
  - `module_name` (const std::string &): Module name that was not found.
- Details: module_name Module name that was not found.

#### `std::string_view remediationHint() noexcept`
- Source: `include/themis/base/base_error_taxonomy.h`:564
- Brief: Actionable operator remediation hint.
- Parameters: none

### themis::modules::BaseErrorTaxonomy::BASE_RELOAD_NO_BACKUP

#### `std::string_view description() noexcept`
- Source: `include/themis/base/base_error_taxonomy.h`:436
- Brief: n/a
- Parameters: none

#### `std::string format(const std::string &module_name)`
- Source: `include/themis/base/base_error_taxonomy.h`:451
- Brief: n/a
- Parameters:
  - `module_name` (const std::string &): Module name.
- Details: module_name Module name.

#### `std::string_view remediationHint() noexcept`
- Source: `include/themis/base/base_error_taxonomy.h`:441
- Brief: Actionable operator remediation hint.
- Parameters: none

### themis::modules::BaseErrorTaxonomy::BASE_RELOAD_ROLLBACK_FAILED

#### `std::string_view description() noexcept`
- Source: `include/themis/base/base_error_taxonomy.h`:465
- Brief: n/a
- Parameters: none

#### `std::string format(const std::string &module_name, const std::string &backup_path, const std::string &reason)`
- Source: `include/themis/base/base_error_taxonomy.h`:481
- Brief: n/a
- Parameters:
  - `module_name` (const std::string &): Module name.
  - `backup_path` (const std::string &): Path to the backup binary.
  - `reason` (const std::string &): Failure reason.
- Details: module_name Module name. backup_path Path to the backup binary. reason Failure reason.

#### `std::string_view remediationHint() noexcept`
- Source: `include/themis/base/base_error_taxonomy.h`:470
- Brief: Actionable operator remediation hint.
- Parameters: none

### themis::modules::BaseErrorTaxonomy::BASE_RELOAD_STATE_RESTORE_FAILED

#### `std::string_view description() noexcept`
- Source: `include/themis/base/base_error_taxonomy.h`:530
- Brief: n/a
- Parameters: none

#### `std::string format(const std::string &module_name)`
- Source: `include/themis/base/base_error_taxonomy.h`:545
- Brief: n/a
- Parameters:
  - `module_name` (const std::string &): Module name.
- Details: module_name Module name.

#### `std::string_view remediationHint() noexcept`
- Source: `include/themis/base/base_error_taxonomy.h`:535
- Brief: Actionable operator remediation hint.
- Parameters: none

### themis::modules::BaseErrorTaxonomy::BASE_SANDBOX_DEGRADED

#### `std::string_view description() noexcept`
- Source: `include/themis/base/base_error_taxonomy.h`:373
- Brief: n/a
- Parameters: none

#### `std::string format(const std::string &module_name, const std::string &warnings)`
- Source: `include/themis/base/base_error_taxonomy.h`:389
- Brief: n/a
- Parameters:
  - `module_name` (const std::string &): Module name.
  - `warnings` (const std::string &): Comma-separated list of launchWarnings().
- Details: module_name Module name. warnings Comma-separated list of launchWarnings().

#### `std::string_view remediationHint() noexcept`
- Source: `include/themis/base/base_error_taxonomy.h`:378
- Brief: Actionable operator remediation hint.
- Parameters: none

### themis::modules::BaseErrorTaxonomy::BASE_SANDBOX_INACTIVE_STATS

#### `std::string_view description() noexcept`
- Source: `include/themis/base/base_error_taxonomy.h`:404
- Brief: n/a
- Parameters: none

#### `std::string format(const std::string &module_name)`
- Source: `include/themis/base/base_error_taxonomy.h`:418
- Brief: n/a
- Parameters:
  - `module_name` (const std::string &): Module name.
- Details: module_name Module name.

#### `std::string_view remediationHint() noexcept`
- Source: `include/themis/base/base_error_taxonomy.h`:409
- Brief: Actionable operator remediation hint.
- Parameters: none

### themis::modules::BaseErrorTaxonomy::BASE_SANDBOX_LAUNCH_FAILED

#### `std::string_view description() noexcept`
- Source: `include/themis/base/base_error_taxonomy.h`:276
- Brief: n/a
- Parameters: none

#### `std::string format(const std::string &module_name, const std::string &last_error)`
- Source: `include/themis/base/base_error_taxonomy.h`:292
- Brief: n/a
- Parameters:
  - `module_name` (const std::string &): Name of the module being sandboxed.
  - `last_error` (const std::string &): ModuleSandbox::lastError() string.
- Details: module_name Name of the module being sandboxed. last_error ModuleSandbox::lastError() string.

#### `std::string_view remediationHint() noexcept`
- Source: `include/themis/base/base_error_taxonomy.h`:281
- Brief: Actionable operator remediation hint.
- Parameters: none

### themis::modules::BaseErrorTaxonomy::BASE_SANDBOX_RESOURCE_LIMIT

#### `std::string_view description() noexcept`
- Source: `include/themis/base/base_error_taxonomy.h`:307
- Brief: n/a
- Parameters: none

#### `std::string format(const std::string &module_name, const std::string &resource, const std::string &limit_value, const std::string &measured)`
- Source: `include/themis/base/base_error_taxonomy.h`:325
- Brief: n/a
- Parameters:
  - `module_name` (const std::string &): Module name.
  - `resource` (const std::string &): "memory" or "cpu".
  - `limit_value` (const std::string &): Configured limit (e.g. "256 MB").
  - `measured` (const std::string &): Observed value.
- Details: module_name Module name. resource "memory" or "cpu". limit_value Configured limit (e.g. "256 MB"). measured Observed value.

#### `std::string_view remediationHint() noexcept`
- Source: `include/themis/base/base_error_taxonomy.h`:312
- Brief: Actionable operator remediation hint.
- Parameters: none

### themis::modules::BaseErrorTaxonomy::BASE_SANDBOX_TIMEOUT

#### `std::string_view description() noexcept`
- Source: `include/themis/base/base_error_taxonomy.h`:343
- Brief: n/a
- Parameters: none

#### `std::string format(const std::string &module_name, unsigned int timeout_seconds)`
- Source: `include/themis/base/base_error_taxonomy.h`:358
- Brief: n/a
- Parameters:
  - `module_name` (const std::string &): Module name.
  - `timeout_seconds` (unsigned int): Configured timeout value.
- Details: module_name Module name. timeout_seconds Configured timeout value.

#### `std::string_view remediationHint() noexcept`
- Source: `include/themis/base/base_error_taxonomy.h`:348
- Brief: Actionable operator remediation hint.
- Parameters: none

### themis::modules::HealthCheckResult

#### `HealthCheckResult failure(const std::string &name, const std::string &msg)`
- Source: `include/themis/base/module_loader.h`:76
- Brief: n/a
- Parameters:
  - `name` (const std::string &): n/a
  - `msg` (const std::string &): n/a

#### `HealthCheckResult success(const std::string &name, const std::string &msg="")`
- Source: `include/themis/base/module_loader.h`:68
- Brief: n/a
- Parameters:
  - `name` (const std::string &): n/a
  - `msg` (const std::string &): n/a

### themis::modules::HotReloadManager

#### `HotReloadManager()`
- Source: `include/themis/base/hot_reload_manager.h`:187
- Brief: n/a
- Parameters: none

#### `HotReloadManager(const Config &config)`
- Source: `include/themis/base/hot_reload_manager.h`:188
- Brief: n/a
- Parameters:
  - `config` (const Config &): n/a

#### `HotReloadManager(const HotReloadManager &)=delete`
- Source: `include/themis/base/hot_reload_manager.h`:191
- Brief: n/a
- Parameters:
  - `<unnamed>` (const HotReloadManager &): n/a

#### `void addReloadCallback(ReloadCallback cb)`
- Source: `include/themis/base/hot_reload_manager.h`:284
- Brief: n/a
- Parameters:
  - `cb` (ReloadCallback): n/a

#### `void clearReloadCallbacks()`
- Source: `include/themis/base/hot_reload_manager.h`:285
- Brief: n/a
- Parameters: none

#### `std::optional< ModuleVersion > getCurrentVersion(const std::string &module_name) const`
- Source: `include/themis/base/hot_reload_manager.h`:254
- Brief: Get the current version of a registered module.
- Parameters:
  - `module_name` (const std::string &): n/a
- Return: Version info, or std::nullopt if the module is not registered or not currently loaded.
- Details: Version info, or std::nullopt if the module is not registered or not currently loaded.

#### `std::optional< SandboxStats > getSandboxStats(const std::string &module_name) const`
- Source: `include/themis/base/hot_reload_manager.h`:274
- Brief: Get sandbox resource-usage statistics for a loaded module.
- Parameters:
  - `module_name` (const std::string &): n/a
- Return: SandboxStats sampled from the active sandbox, or std::nullopt if sandboxing is not configured for this manager, or the module is not registered / not currently sandboxed.
- Details: SandboxStats sampled from the active sandbox, or std::nullopt if sandboxing is not configured for this manager, or the module is not registered / not currently sandboxed.

#### `Stats getStats() const`
- Source: `include/themis/base/hot_reload_manager.h`:331
- Brief: n/a
- Parameters: none

#### `bool isRollbackAvailable(const std::string &module_name) const`
- Source: `include/themis/base/hot_reload_manager.h`:260
- Brief: Check whether a rollback is available for a module.
- Parameters:
  - `module_name` (const std::string &): n/a

#### `void notify(const std::string &name, ReloadPhase phase)`
- Source: `include/themis/base/hot_reload_manager.h`:376
- Brief: n/a
- Parameters:
  - `name` (const std::string &): n/a
  - `phase` (ReloadPhase): n/a

#### `HotReloadManager & operator=(const HotReloadManager &)=delete`
- Source: `include/themis/base/hot_reload_manager.h`:192
- Brief: n/a
- Parameters:
  - `<unnamed>` (const HotReloadManager &): n/a

#### `void registerModule(const std::string &module_name, ModuleLoader &loader)`
- Source: `include/themis/base/hot_reload_manager.h`:204
- Brief: Register a module so it can be hot-reloaded later.
- Parameters:
  - `module_name` (const std::string &): Logical name (e.g. "themis_storage").
  - `loader` (ModuleLoader &): The ModuleLoader that currently owns the module.
- Details: module_name Logical name (e.g. "themis_storage"). loader The ModuleLoader that currently owns the module.

#### `std::vector< std::string > registeredModules() const`
- Source: `include/themis/base/hot_reload_manager.h`:265
- Brief: Return the list of all registered module names.
- Parameters: none

#### `HotReloadResult reloadModule(const std::string &module_name, const std::string &new_path)`
- Source: `include/themis/base/hot_reload_manager.h`:231
- Brief: Hot-reload a module from a new path without stopping the database.
- Parameters:
  - `module_name` (const std::string &): Logical module name (must be registered).
  - `new_path` (const std::string &): Filesystem path to the updated binary.
- Return: HotReloadResult describing outcome, duration, and versions.
- Details: Steps: Optionally save state (if StateSaveCallback is set). Emit BEFORE_UNLOAD. Load new binary via the registered ModuleLoader. On success: unload old binary, emit AFTER_UNLOAD, emit AFTER_LOAD, optionally restore state. On failure: keep old binary running, return error result. module_name Logical module name (must be registered). new_path Filesystem path to the updated binary. HotReloadResult describing outcome, duration, and versions.

#### `void resetStats()`
- Source: `include/themis/base/hot_reload_manager.h`:332
- Brief: n/a
- Parameters: none

#### `bool restoreState(const std::string &name, const std::string &state)`
- Source: `include/themis/base/hot_reload_manager.h`:378
- Brief: n/a
- Parameters:
  - `name` (const std::string &): n/a
  - `state` (const std::string &): n/a

#### `HotReloadResult rollback(const std::string &module_name)`
- Source: `include/themis/base/hot_reload_manager.h`:243
- Brief: Roll back to the previous version of a module.
- Parameters:
  - `module_name` (const std::string &): Logical module name.
- Return: HotReloadResult describing rollback outcome.
- Details: Only available when enableRollback=true and a successful reload has already been performed (making the prior version the backup). module_name Logical module name. HotReloadResult describing rollback outcome.

#### `std::string saveState(const std::string &name)`
- Source: `include/themis/base/hot_reload_manager.h`:377
- Brief: n/a
- Parameters:
  - `name` (const std::string &): n/a

#### `void setSpanEmitter(SpanEmitter emitter)`
- Source: `include/themis/base/hot_reload_manager.h`:300
- Brief: Replace the span emitter used by reloadModule() and rollback().
- Parameters:
  - `emitter` (SpanEmitter): Thread-safe callable that receives SpanEvent values. Must remain valid for the lifetime of this manager.
- Details: The default emitter is a no-op; replace it with a collecting emitter in tests or with an OpenTelemetry adapter in production to capture spans. emitter Thread-safe callable that receives SpanEvent values. Must remain valid for the lifetime of this manager.

#### `void setStateRestoreCallback(StateRestoreCallback cb)`
- Source: `include/themis/base/hot_reload_manager.h`:282
- Brief: n/a
- Parameters:
  - `cb` (StateRestoreCallback): n/a

#### `void setStateSaveCallback(StateSaveCallback cb)`
- Source: `include/themis/base/hot_reload_manager.h`:281
- Brief: n/a
- Parameters:
  - `cb` (StateSaveCallback): n/a

#### `SpanEmitter spanEmitter() const`
- Source: `include/themis/base/hot_reload_manager.h`:313
- Brief: Return the active span emitter by value (thread-safe snapshot).
- Parameters: none
- Details: The returned SpanEmitter is a copy taken under a shared lock, so callers receive a stable, independently-owned value. Store the returned emitter by value and pass it by reference into ScopedSpan; do not bind it to a const& or auto& (that would dangle immediately). Returns the current span emitter by value (thread-safe read). Callers must not invoke setSpanEmitter() concurrently with active reload/rollback operations.

#### `void unregisterModule(const std::string &module_name)`
- Source: `include/themis/base/hot_reload_manager.h`:210
- Brief: Unregister a module and free any backup slots.
- Parameters:
  - `module_name` (const std::string &): Logical name previously passed to registerModule().
- Details: module_name Logical name previously passed to registerModule().

#### `ModuleVersion versionFromLoader(ModuleLoader &loader, const std::string &module_name)`
- Source: `include/themis/base/hot_reload_manager.h`:380
- Brief: n/a
- Parameters:
  - `loader` (ModuleLoader &): n/a
  - `module_name` (const std::string &): n/a

#### `~HotReloadManager()`
- Source: `include/themis/base/hot_reload_manager.h`:189
- Brief: n/a
- Parameters: none

### themis::modules::IWasmRuntime

#### `bool call(const std::string &fn_name, const std::vector< uint8_t > &args, std::vector< uint8_t > &out)=0`
- Source: `include/themis/base/wasm_runtime_injector.h`:62
- Brief: Call a named export function.
- Parameters:
  - `fn_name` (const std::string &): Exported function name.
  - `args` (const std::vector< uint8_t > &): Serialised argument blob (plugin-defined format).
  - `out` (std::vector< uint8_t > &): Output blob written by the function.
- Return: true on success; false on trap or if the function is not exported.
- Details: fn_name Exported function name. args Serialised argument blob (plugin-defined format). out Output blob written by the function. true on success; false on trap or if the function is not exported.

#### `bool instantiate(const std::vector< uint8_t > &wasm_bytes, const std::vector< WasmHostFunction > &host_fns, size_t memory_limit_bytes)=0`
- Source: `include/themis/base/wasm_runtime_injector.h`:50
- Brief: Compile and instantiate a .wasm binary.
- Parameters:
  - `wasm_bytes` (const std::vector< uint8_t > &): Raw .wasm bytes.
  - `host_fns` (const std::vector< WasmHostFunction > &): Host functions the module may import.
  - `memory_limit_bytes` (size_t): Maximum linear-memory bytes (0 = runtime default).
- Return: true on success; false if compilation or instantiation fails.
- Details: wasm_bytes Raw .wasm bytes. host_fns Host functions the module may import. memory_limit_bytes Maximum linear-memory bytes (0 = runtime default). true on success; false if compilation or instantiation fails.

#### `bool isInstantiated() const =0`
- Source: `include/themis/base/wasm_runtime_injector.h`:86
- Brief: True if the runtime compiled and instantiated successfully.
- Parameters: none

#### `uint8_t * linearMemory(size_t &out_size)=0`
- Source: `include/themis/base/wasm_runtime_injector.h`:71
- Brief: Return a pointer to the instance's linear memory and its size.
- Parameters:
  - `out_size` (size_t &): n/a
- Details: Callers must not hold this pointer across calls to call().

#### `std::string name() const =0`
- Source: `include/themis/base/wasm_runtime_injector.h`:76
- Brief: Human-readable name of this runtime (e.g. "wasmtime").
- Parameters: none

#### `std::string version() const =0`
- Source: `include/themis/base/wasm_runtime_injector.h`:81
- Brief: Runtime version string.
- Parameters: none

#### `~IWasmRuntime()=default`
- Source: `include/themis/base/wasm_runtime_injector.h`:40
- Brief: n/a
- Parameters: none

### themis::modules::ModuleDependencyResolver

#### `void clear()`
- Source: `include/themis/base/module_loader.h`:932
- Brief: Remove all registered module entries.
- Parameters: none
- Details: Clear.

#### `std::vector< RegisteredModuleInfo > getRegisteredModules() const`
- Source: `include/themis/base/module_loader.h`:954
- Brief: Return all registered modules and their dependency declarations.
- Parameters: none
- Return: Sorted vector of registered module information.
- Details: Intended for read-only inspection (e.g., dependency graph visualisation). The returned snapshot is sorted by module name. Sorted vector of registered module information.

#### `bool isVersionCompatible(const std::string &version, const std::string &minVersion, const std::string &maxVersion)`
- Source: `include/themis/base/module_loader.h`:968
- Brief: Check if a version string satisfies the given constraints.
- Parameters:
  - `version` (const std::string &): Version to test.
  - `minVersion` (const std::string &): Lower bound (inclusive). "" = no lower bound.
  - `maxVersion` (const std::string &): Upper bound (inclusive). "" = no upper bound.
- Return: true if version satisfies both constraints.
- Details: Supports semantic version strings of the form "major.minor.patch". An empty constraint string is treated as unconstrained (always passes). An empty version satisfies only fully unconstrained dependencies. version Version to test. minVersion Lower bound (inclusive). "" = no lower bound. maxVersion Upper bound (inclusive). "" = no upper bound. true if version satisfies both constraints.

#### `void registerModule(const std::string &name, const std::string &version, const std::vector< ModuleDependency > &deps)`
- Source: `include/themis/base/module_loader.h`:893
- Brief: Register a module together with its dependency declarations.
- Parameters:
  - `name` (const std::string &): Input parameter.
  - `version` (const std::string &): Input parameter.
  - `deps` (const std::vector< ModuleDependency > &): Input parameter.
- Details: Register Module. Registering a module a second time replaces the previous registration. name Unique module name (e.g., "themis_base"). version Semantic version of this module (e.g., "1.2.3"). May be "". deps Dependency list; may be empty. name Input parameter. version Input parameter. deps Input parameter.

#### `void registerModule(const std::string &name, const std::vector< ModuleDependency > &deps)`
- Source: `include/themis/base/module_loader.h`:907
- Brief: Convenience overload — registers a module with no version string.
- Parameters:
  - `name` (const std::string &): Input parameter.
  - `deps` (const std::vector< ModuleDependency > &): Input parameter.
- Details: Register Module. Version constraints declared by other modules against this module will only be satisfied if they are unconstrained (both minVersion and maxVersion are empty). name Unique module name. deps Dependency list; may be empty. name Input parameter. deps Input parameter.

#### `DependencyResolutionResult resolve() const`
- Source: `include/themis/base/module_loader.h`:915
- Brief: Resolve load order for all registered modules.
- Parameters: none
- Return: Resolution result with load order or error information.
- Details: Resolution result with load order or error information.

#### `DependencyResolutionResult resolveFor(const std::vector< std::string > &moduleNames) const`
- Source: `include/themis/base/module_loader.h`:926
- Brief: Resolve load order for a specific subset of modules.
- Parameters:
  - `moduleNames` (const std::vector< std::string > &): Modules to resolve (by name).
- Return: Resolution result.
- Details: Transitive dependencies that are registered but not in moduleNames are included in the resolution automatically. moduleNames Modules to resolve (by name). Resolution result.

#### `DependencyResolutionResult topologicalSort(const std::vector< std::string > &nodes) const`
- Source: `include/themis/base/module_loader.h`:980
- Brief: Topological sort (Kahn's algorithm) over the supplied node set.
- Parameters:
  - `nodes` (const std::vector< std::string > &): n/a

### themis::modules::ModuleFailureHistory

#### `bool canRetry(uint64_t currentTime) const`
- Source: `include/themis/base/module_loader.h`:208
- Brief: n/a
- Parameters:
  - `currentTime` (uint64_t): n/a

#### `bool isQuarantined() const`
- Source: `include/themis/base/module_loader.h`:204
- Brief: n/a
- Parameters: none

### themis::modules::ModuleHashVerifier

#### `void addExpectedHash(const std::string &moduleName, const std::string &expectedHash)`
- Source: `include/themis/module_hash_verifier.h`:110
- Brief: Add or update an expected hash in the in-memory manifest.
- Parameters:
  - `moduleName` (const std::string &): Input parameter.
  - `expectedHash` (const std::string &): Input parameter.
- Details: Add Expected Hash. moduleName Logical module name (e.g., "themis_storage"). expectedHash Lowercase hex-encoded SHA-256 hash. moduleName Input parameter. expectedHash Input parameter. Calls: spdlog::debug().

#### `void clearManifest()`
- Source: `include/themis/module_hash_verifier.h`:148
- Brief: Remove all entries from the in-memory manifest.
- Parameters: none
- Details: Clear Manifest. Calls: clear().

#### `std::string computeSHA256(const std::string &filePath)`
- Source: `include/themis/module_hash_verifier.h`:71
- Brief: Compute the SHA-256 hash of any file.
- Parameters:
  - `filePath` (const std::string &): Input parameter.
- Return: Lowercase hex-encoded 64-character SHA-256 digest, or an empty string on I/O or OpenSSL error.
- Details: Compute SHA256. filePath Absolute path to the file. Lowercase hex-encoded 64-character SHA-256 digest, or an empty string on I/O or OpenSSL error. filePath Input parameter. Return value. Calls: file(), spdlog::warn(), EVP_MD_CTX_new(), spdlog::error(), EVP_DigestInit_ex(), EVP_sha256(), EVP_MD_CTX_free(), read().

#### `std::optional< std::string > getExpectedHash(const std::string &moduleName) const`
- Source: `include/themis/module_hash_verifier.h`:138
- Brief: Look up the expected hash for moduleName in the manifest.
- Parameters:
  - `moduleName` (const std::string &): Module name (key in the manifest).
- Return: The expected hex-encoded SHA-256 string, or an empty optional if moduleName is not in the manifest.
- Details: Does NOT open any file; queries only the in-memory manifest loaded via loadManifest() or populated via addExpectedHash(). Intended for use by callers (e.g. ModuleLoader) that have already computed the file hash and want to avoid hashing the file a second time. moduleName Module name (key in the manifest). The expected hex-encoded SHA-256 string, or an empty optional if moduleName is not in the manifest.

#### `bool loadManifest(const std::string &manifestPath)`
- Source: `include/themis/module_hash_verifier.h`:94
- Brief: Load (or replace) the in-memory hash manifest from a JSON file.
- Parameters:
  - `manifestPath` (const std::string &): Input parameter.
- Return: true on success, false on I/O or parse error.
- Details: Load Manifest. The file must contain a JSON object whose keys are module names and whose values are hex-encoded SHA-256 hash strings. manifestPath Path to the JSON manifest file. true on success, false on I/O or parse error. manifestPath Input parameter. True when the operation succeeds. Calls: file(), spdlog::error(), is_object(), clear(), items(), is_string(), spdlog::warn(), spdlog::info().

#### `size_t manifestSize() const`
- Source: `include/themis/module_hash_verifier.h`:143
- Brief: Number of entries currently in the in-memory manifest.
- Parameters: none

#### `bool saveManifest(const std::string &outputPath) const`
- Source: `include/themis/module_hash_verifier.h`:102
- Brief: Save the current in-memory manifest to a JSON file.
- Parameters:
  - `outputPath` (const std::string &): Destination file (created or overwritten).
- Return: true on success, false on I/O error.
- Details: outputPath Destination file (created or overwritten). true on success, false on I/O error.

#### `bool verifyHash(const std::string &modulePath, const std::string &expectedHash)`
- Source: `include/themis/module_hash_verifier.h`:80
- Brief: Verify a module file against a known-good hash.
- Parameters:
  - `modulePath` (const std::string &): Input parameter.
  - `expectedHash` (const std::string &): Input parameter.
- Return: true if the computed hash matches expectedHash exactly.
- Details: Verify Hash. modulePath Absolute path to the module file. expectedHash Lowercase hex-encoded expected SHA-256 digest. true if the computed hash matches expectedHash exactly. modulePath Input parameter. expectedHash Input parameter. True when the operation succeeds. Calls: computeSHA256(), empty(), spdlog::error().

#### `ModuleHashVerificationResult verifyModule(const std::string &moduleName, const std::string &modulePath) const`
- Source: `include/themis/module_hash_verifier.h`:123
- Brief: Verify a module file against the entry in the loaded manifest.
- Parameters:
  - `moduleName` (const std::string &): Module name (key in the manifest).
  - `modulePath` (const std::string &): Absolute path to the module file to check.
- Return: Verification result with success flag, hashes, and any error.
- Details: Fails with an error message if moduleName is absent from the manifest or if the hash cannot be computed. moduleName Module name (key in the manifest). modulePath Absolute path to the module file to check. Verification result with success flag, hashes, and any error.

### themis::modules::ModuleLoader

#### `ModuleLoader()`
- Source: `include/themis/base/module_loader.h`:352
- Brief: n/a
- Parameters: none

#### `ModuleLoader(const ModuleLoader &)=delete`
- Source: `include/themis/base/module_loader.h`:356
- Brief: n/a
- Parameters:
  - `<unnamed>` (const ModuleLoader &): n/a

#### `void addBlacklistedHash(const std::string &hash)`
- Source: `include/themis/base/module_loader.h`:428
- Brief: Add a hash to the module blacklist.
- Parameters:
  - `hash` (const std::string &): Input parameter.
- Details: Add Blacklisted Hash. hash SHA-256 hash to blacklist hash Input parameter. Implements addBlacklistedHash without additional internal calls.

#### `void addWhitelistedHash(const std::string &hash)`
- Source: `include/themis/base/module_loader.h`:422
- Brief: Add a hash to the module whitelist.
- Parameters:
  - `hash` (const std::string &): Input parameter.
- Details: Add Whitelisted Hash. hash SHA-256 hash to whitelist hash Input parameter. Implements addWhitelistedHash without additional internal calls.

#### `uint64_t calculateBackoffTime(uint32_t consecutiveFailures) const`
- Source: `include/themis/base/module_loader.h`:740
- Brief: n/a
- Parameters:
  - `consecutiveFailures` (uint32_t): n/a

#### `std::string calculateModuleHash(const std::string &modulePath)`
- Source: `include/themis/base/module_loader.h`:727
- Brief: n/a
- Parameters:
  - `modulePath` (const std::string &): n/a

#### `ErrorCategory categorizeError(ModuleErrorCode code) const`
- Source: `include/themis/base/module_loader.h`:734
- Brief: n/a
- Parameters:
  - `code` (ModuleErrorCode): n/a

#### `bool checkQuarantine(const std::string &modulePath, ModuleVerificationResult &result)`
- Source: `include/themis/base/module_loader.h`:741
- Brief: Check Quarantine.
- Parameters:
  - `modulePath` (const std::string &): Input parameter.
  - `result` (ModuleVerificationResult &): Input/output parameter.
- Return: True when the operation succeeds.
- Details: modulePath Input parameter. result Input/output parameter. True when the operation succeeds. Calls: find(), end(), isQuarantined(), getErrorMessage(), std::to_string(), spdlog::error(), std::time(), canRetry().

#### `void clearFailureHistory(const std::string &modulePath)`
- Source: `include/themis/base/module_loader.h`:454
- Brief: Clear failure history for a module.
- Parameters:
  - `modulePath` (const std::string &): Input parameter.
- Details: Clear Failure History. modulePath Path to module modulePath Input parameter. Calls: find(), end(), isQuarantined(), erase(), spdlog::info().

#### `void clearHealthChecks()`
- Source: `include/themis/base/module_loader.h`:496
- Brief: Clear all registered health checks.
- Parameters: none
- Details: Clear Health Checks. Calls: clear(), spdlog::info().

#### `void configureWatchdog(const WatchdogConfig &config)`
- Source: `include/themis/base/module_loader.h`:567
- Brief: Configure the watchdog before starting it.
- Parameters:
  - `config` (const WatchdogConfig &): Input parameter.
- Details: Configure Watchdog. May be called before or after startWatchdog(); if called while the watchdog is running the new settings take effect on the next sweep. config Watchdog configuration parameters. config Input parameter. Calls: lk(), spdlog::info().

#### `bool exportAuditLog(const std::string &outputPath) const`
- Source: `include/themis/base/module_loader.h`:540
- Brief: Export security audit log.
- Parameters:
  - `outputPath` (const std::string &): Path to export JSON audit log
- Return: true if successful, false otherwise
- Details: outputPath Path to export JSON audit log true if successful, false otherwise

#### `ModuleMetadata extractMetadataFromHandle(void *handle)`
- Source: `include/themis/base/module_loader.h`:749
- Brief: Extract Metadata From Handle.
- Parameters:
  - `handle` (void *): Input/output parameter.
- Return: Return value.
- Details: handle Input/output parameter. Return value. Calls: uint32_t(), getSymbol(), getVersionStr(), getAbiVersion(), getBuildId(), getMajor(), getMinor(), getPatch().

#### `ModuleMetadata extractModuleMetadata(const std::string &modulePath)`
- Source: `include/themis/base/module_loader.h`:732
- Brief: n/a
- Parameters:
  - `modulePath` (const std::string &): n/a

#### `std::vector< LoadedModule > getAllLoadedModules() const`
- Source: `include/themis/base/module_loader.h`:404
- Brief: Get all loaded modules.
- Parameters: none
- Return: Vector of all loaded modules
- Details: Vector of all loaded modules

#### `std::map< std::string, WatchdogModuleStats > getAllWatchdogStats() const`
- Source: `include/themis/base/module_loader.h`:606
- Brief: Get watchdog statistics for all tracked modules.
- Parameters: none
- Return: Map from module name to watchdog stats.
- Details: Map from module name to watchdog stats.

#### `ModuleMetadata getCachedMetadata(const std::string &modulePath)`
- Source: `include/themis/base/module_loader.h`:750
- Brief: Get Cached Metadata.
- Parameters:
  - `modulePath` (const std::string &): Input parameter.
- Return: Return value.
- Details: modulePath Input parameter. Return value. Calls: find(), end(), spdlog::debug(), extractModuleMetadata(), isValid().

#### `std::string getErrorMessage(ModuleErrorCode code) const`
- Source: `include/themis/base/module_loader.h`:733
- Brief: n/a
- Parameters:
  - `code` (ModuleErrorCode): n/a

#### `std::optional< ModuleFailureHistory > getFailureHistory(const std::string &modulePath) const`
- Source: `include/themis/base/module_loader.h`:435
- Brief: Get failure history for a module.
- Parameters:
  - `modulePath` (const std::string &): Path to module
- Return: Optional failure history (nullopt if no failures)
- Details: modulePath Path to module Optional failure history (nullopt if no failures)

#### `std::vector< HealthCheckResult > getHealthCheckResults(const std::string &moduleName) const`
- Source: `include/themis/base/module_loader.h`:510
- Brief: Get health check results for a module.
- Parameters:
  - `moduleName` (const std::string &): Name of the module
- Return: Vector of health check results
- Details: moduleName Name of the module Vector of health check results

#### `ModuleMetrics getMetrics() const`
- Source: `include/themis/base/module_loader.h`:460
- Brief: Get current module metrics.
- Parameters: none
- Return: Current metrics snapshot
- Details: Current metrics snapshot

#### `std::optional< LoadedModule > getModuleInfo(const std::string &moduleName) const`
- Source: `include/themis/base/module_loader.h`:398
- Brief: Get information about a loaded module.
- Parameters:
  - `moduleName` (const std::string &): Name of the module
- Return: Optional module info (nullopt if not loaded)
- Details: moduleName Name of the module Optional module info (nullopt if not loaded)

#### `std::string getModuleNameFromPath(const std::string &path)`
- Source: `include/themis/base/module_loader.h`:728
- Brief: Get Module Name From Path.
- Parameters:
  - `path` (const std::string &): Input parameter.
- Return: Return value.
- Details: path Input parameter. Return value. Calls: p(), stem(), string(), rfind(), substr().

#### `std::vector< themis::acceleration::PluginSecurityEvent > getPluginAuditTrail(const std::string &modulePath) const`
- Source: `include/themis/base/module_loader.h`:553
- Brief: Get the per-plugin audit trail (load, unload, errors).
- Parameters:
  - `modulePath` (const std::string &): Full path to the module (as supplied to loadModule)
- Return: Vector of audit events for that specific plugin
- Details: Returns all recorded security and lifecycle events for the given module path in chronological order. Events include PLUGIN_LOADED, PLUGIN_UNLOADED, PLUGIN_LOAD_FAILED, and related security events. modulePath Full path to the module (as supplied to loadModule) Vector of audit events for that specific plugin

#### `std::vector< std::string > getQuarantinedModules() const`
- Source: `include/themis/base/module_loader.h`:441
- Brief: Get all quarantined modules.
- Parameters: none
- Return: Vector of quarantined module paths
- Details: Vector of quarantined module paths

#### `void * getSymbol(void *handle, const std::string &symbolName)`
- Source: `include/themis/base/module_loader.h`:723
- Brief: Get Symbol.
- Parameters:
  - `handle` (void *): Input/output parameter.
  - `symbolName` (const std::string &): Input parameter.
- Return: Pointer to the result.
- Details: handle Input/output parameter. symbolName Input parameter. Pointer to the result. Calls: GetProcAddress(), c_str(), dlsym().

#### `std::optional< WatchdogModuleStats > getWatchdogStats(const std::string &moduleName) const`
- Source: `include/themis/base/module_loader.h`:599
- Brief: Get watchdog statistics for a specific module.
- Parameters:
  - `moduleName` (const std::string &): Name of the module.
- Return: Optional stats (nullopt if module not tracked).
- Details: moduleName Name of the module. Optional stats (nullopt if module not tracked).

#### `bool isABICompatible(const ModuleMetadata &metadata) const`
- Source: `include/themis/base/module_loader.h`:472
- Brief: Check if ABI is compatible with ThemisDB.
- Parameters:
  - `metadata` (const ModuleMetadata &): Module metadata to check
- Return: true if compatible, false otherwise
- Details: metadata Module metadata to check true if compatible, false otherwise

#### `bool isModuleLoaded(const std::string &moduleName) const`
- Source: `include/themis/base/module_loader.h`:391
- Brief: Check if a module is loaded.
- Parameters:
  - `moduleName` (const std::string &): Name of the module
- Return: true if loaded, false otherwise
- Details: moduleName Name of the module true if loaded, false otherwise

#### `bool isThemisModule(const std::string &filename)`
- Source: `include/themis/base/module_loader.h`:729
- Brief: Is Themis Module.
- Parameters:
  - `filename` (const std::string &): Input parameter.
- Return: True when the operation succeeds.
- Details: filename Input parameter. True when the operation succeeds. Calls: rfind().

#### `bool isWatchdogRunning() const`
- Source: `include/themis/base/module_loader.h`:591
- Brief: Return true if the watchdog background thread is running.
- Parameters: none

#### `size_t loadAllModules(const std::string &moduleDirectory)`
- Source: `include/themis/base/module_loader.h`:373
- Brief: Load all required modules from a directory.
- Parameters:
  - `moduleDirectory` (const std::string &): Input parameter.
- Return: Number of successfully loaded modules
- Details: Load All Modules. moduleDirectory Directory containing themis_* modules Number of successfully loaded modules moduleDirectory Input parameter. Return value. Calls: spdlog::info(), std::filesystem::exists(), spdlog::error(), std::filesystem::directory_iterator(), is_regular_file(), path(), filename(), string().

#### `void * loadLibrary(const std::string &path)`
- Source: `include/themis/base/module_loader.h`:721
- Brief: ============================================================================ Platform-independent OS loader primitives ============================================================================
- Parameters:
  - `path` (const std::string &): Input parameter.
- Return: Pointer to the result.
- Details: path Input parameter. Pointer to the result. Calls: LoadLibraryA(), c_str(), dlopen().

#### `ModuleVerificationResult loadModule(const std::string &modulePath, const std::string &moduleName)`
- Source: `include/themis/base/module_loader.h`:365
- Brief: Load a ThemisDB module with security verification.
- Parameters:
  - `modulePath` (const std::string &): Input parameter.
  - `moduleName` (const std::string &): Input parameter.
- Return: Verification result with success status
- Details: Load Module. modulePath Full path to the module DLL/SO moduleName Expected module name (e.g., "themis_storage") Verification result with success status modulePath Input parameter. moduleName Input parameter. Return value. Calls: std::chrono::steady_clock::now(), std::time(), spdlog::info(), checkQuarantine(), updateMetrics(), isModuleLoaded(), categorizeError(), getErrorMessage().

#### `uint64_t nowMs()`
- Source: `include/themis/base/module_loader.h`:758
- Brief: Now Ms.
- Parameters: none
- Return: Return value.
- Details: Return value. Calls: std::chrono::steady_clock::now(), time_since_epoch(), count().

#### `ModuleLoader & operator=(const ModuleLoader &)=delete`
- Source: `include/themis/base/module_loader.h`:357
- Brief: n/a
- Parameters:
  - `<unnamed>` (const ModuleLoader &): n/a

#### `void quarantineModule(const std::string &modulePath)`
- Source: `include/themis/base/module_loader.h`:739
- Brief: Quarantine Module.
- Parameters:
  - `modulePath` (const std::string &): Input parameter.
- Details: modulePath Input parameter. Calls: find(), end(), std::time(), spdlog::critical().

#### `std::optional< LoadStage > queryModuleStage(const std::string &moduleName) const`
- Source: `include/themis/base/module_loader.h`:503
- Brief: Query current load stage of a module.
- Parameters:
  - `moduleName` (const std::string &): Name of the module
- Return: Optional load stage (nullopt if not found)
- Details: moduleName Name of the module Optional load stage (nullopt if not found)

#### `void recordFailure(const std::string &modulePath, ModuleErrorCode errorCode, const std::string &errorMessage)`
- Source: `include/themis/base/module_loader.h`:737
- Brief: Record Failure.
- Parameters:
  - `modulePath` (const std::string &): Input parameter.
  - `errorCode` (ModuleErrorCode): Input parameter.
  - `errorMessage` (const std::string &): Input parameter.
- Details: modulePath Input parameter. errorCode Input parameter. errorMessage Input parameter. Calls: std::time(), push_back(), calculateBackoffTime(), spdlog::warn(), shouldQuarantine(), quarantineModule().

#### `void registerHealthCheck(const std::string &checkName, HealthCheckFunction checkFunc)`
- Source: `include/themis/base/module_loader.h`:491
- Brief: Register a health check function.
- Parameters:
  - `checkName` (const std::string &): Input parameter.
  - `checkFunc` (HealthCheckFunction): Input parameter.
- Details: Register Health Check. checkName Unique name for this health check checkFunc Health check function checkName Input parameter. checkFunc Input parameter. Calls: spdlog::info().

#### `bool releaseFromQuarantine(const std::string &modulePath)`
- Source: `include/themis/base/module_loader.h`:448
- Brief: Release a module from quarantine.
- Parameters:
  - `modulePath` (const std::string &): Input parameter.
- Return: true if released, false if not quarantined
- Details: Release From Quarantine. modulePath Path to module to release true if released, false if not quarantined modulePath Input parameter. True when the operation succeeds. Calls: find(), end(), isQuarantined(), spdlog::info().

#### `void resetMetrics()`
- Source: `include/themis/base/module_loader.h`:465
- Brief: Reset all metrics to zero.
- Parameters: none
- Details: Reset Metrics. Calls: ModuleMetrics(), spdlog::info().

#### `void resetWatchdogStats()`
- Source: `include/themis/base/module_loader.h`:614
- Brief: Reset watchdog statistics for all modules.
- Parameters: none
- Details: Reset Watchdog Stats. Clears restart counts, failure counters, and permanently_failed flags. Does not stop the watchdog if it is running. Calls: lk(), clear(), spdlog::info().

#### `bool runHealthChecks(LoadedModule &module, ModuleVerificationResult &result)`
- Source: `include/themis/base/module_loader.h`:748
- Brief: Run Health Checks.
- Parameters:
  - `module` (LoadedModule &): Input/output parameter.
  - `result` (ModuleVerificationResult &): Input/output parameter.
- Return: True when the operation succeeds.
- Details: module Input/output parameter. result Input/output parameter. True when the operation succeeds. Calls: empty(), spdlog::debug(), spdlog::info(), size(), std::chrono::steady_clock::now(), checkFunc(), count(), push_back().

#### `void setAllowUnsigned(bool allow)`
- Source: `include/themis/base/module_loader.h`:416
- Brief: Set whether to allow unsigned modules.
- Parameters:
  - `allow` (bool): Input parameter.
- Details: Set Allow Unsigned. allow If true, allow unsigned modules (development mode) allow Input parameter. Implements setAllowUnsigned without additional internal calls.

#### `bool setHashManifest(const std::string &manifestPath)`
- Source: `include/themis/base/module_loader.h`:533
- Brief: Load a SHA-256 hash manifest for module integrity verification.
- Parameters:
  - `manifestPath` (const std::string &): Input parameter.
- Return: true if manifest was loaded successfully, false on I/O or parse error.
- Details: Set Hash Manifest. When a manifest is set, every call to loadModule() verifies the module's SHA-256 hash against the manifest entry for that module name. If the module name is present in the manifest and the hashes do not match, the load is rejected with HASH_MISMATCH. Modules not listed in the manifest are still loaded — the manifest acts as an integrity verification list for known modules, not a global block/allowlist. manifestPath Path to JSON manifest file (format: { "moduleName": "hex-sha256", … }). true if manifest was loaded successfully, false on I/O or parse error. manifestPath Input parameter. True when the operation succeeds. Calls: loadManifest(), spdlog::info(), manifestSize(), spdlog::error().

#### `void setMaxBackoffSeconds(uint32_t maxSeconds)`
- Source: `include/themis/base/module_loader.h`:484
- Brief: Set maximum backoff time in seconds.
- Parameters:
  - `maxSeconds` (uint32_t): Input parameter.
- Details: Set Max Backoff Seconds. maxSeconds Maximum backoff time (default: 300 = 5 minutes) maxSeconds Input parameter. Calls: spdlog::info().

#### `void setQuarantineThreshold(uint32_t threshold)`
- Source: `include/themis/base/module_loader.h`:478
- Brief: Set quarantine threshold (consecutive failures before quarantine).
- Parameters:
  - `threshold` (uint32_t): Input parameter.
- Details: Set Quarantine Threshold. threshold Number of failures (default: 3) threshold Input parameter. Calls: spdlog::info().

#### `void setRequireSignature(bool require)`
- Source: `include/themis/base/module_loader.h`:410
- Brief: Set whether to require signature verification.
- Parameters:
  - `require` (bool): Input parameter.
- Details: Set Require Signature. require If true, reject unsigned modules (production mode) require Input parameter. Implements setRequireSignature without additional internal calls.

#### `void setStagedLoadingEnabled(bool enable)`
- Source: `include/themis/base/module_loader.h`:516
- Brief: Enable or disable staged loading.
- Parameters:
  - `enable` (bool): Input parameter.
- Details: Set Staged Loading Enabled. enable If true, use staged loading; if false, load directly enable Input parameter. Calls: spdlog::info().

#### `bool shouldQuarantine(const std::string &modulePath) const`
- Source: `include/themis/base/module_loader.h`:738
- Brief: n/a
- Parameters:
  - `modulePath` (const std::string &): n/a

#### `void startWatchdog()`
- Source: `include/themis/base/module_loader.h`:578
- Brief: Start the watchdog background thread.
- Parameters: none
- Details: Start Watchdog. The thread performs periodic health checks on all loaded modules and automatically restarts any module that fails its checks, applying exponential backoff between successive restart attempts. Calling startWatchdog() while it is already running is a no-op. Calls: exchange(), std::thread(), watchdogLoop(), spdlog::info().

#### `void stopWatchdog()`
- Source: `include/themis/base/module_loader.h`:586
- Brief: Stop the watchdog background thread.
- Parameters: none
- Details: Stop Watchdog. Blocks until the background thread has exited. Calling stopWatchdog() when the watchdog is not running is a no-op. Calls: exchange(), notify_all(), joinable(), join(), spdlog::info().

#### `void unloadAllModules()`
- Source: `include/themis/base/module_loader.h`:384
- Brief: Unload all modules.
- Parameters: none
- Details: Unload All Modules. Calls: lk(), spdlog::info(), size(), PluginSecurityAuditor::instance(), std::time(), logEvent(), unloadLibrary(), ModuleRegistry::instance().

#### `void unloadLibrary(void *handle)`
- Source: `include/themis/base/module_loader.h`:722
- Brief: Unload Library.
- Parameters:
  - `handle` (void *): Input/output parameter.
- Details: handle Input/output parameter. Calls: FreeLibrary(), dlclose().

#### `void unloadModule(const std::string &moduleName)`
- Source: `include/themis/base/module_loader.h`:379
- Brief: Unload a specific module.
- Parameters:
  - `moduleName` (const std::string &): Input parameter.
- Details: Unload Module. moduleName Name of the module to unload moduleName Input parameter. Calls: lk(), find(), end(), spdlog::info(), std::time(), PluginSecurityAuditor::instance(), logEvent(), unloadLibrary().

#### `void updateMetrics(bool success, uint64_t durationMs, ModuleErrorCode errorCode)`
- Source: `include/themis/base/module_loader.h`:744
- Brief: Update Metrics.
- Parameters:
  - `success` (bool): Input parameter.
  - `durationMs` (uint64_t): Input parameter.
  - `errorCode` (ModuleErrorCode): Input parameter.
- Details: success Input parameter. durationMs Input parameter. errorCode Input parameter. Calls: std::min(), std::max().

#### `bool updateModuleStage(const std::string &moduleName, LoadStage newStage)`
- Source: `include/themis/base/module_loader.h`:747
- Brief: Update Module Stage.
- Parameters:
  - `moduleName` (const std::string &): Input parameter.
  - `newStage` (LoadStage): Input parameter.
- Return: True when the operation succeeds.
- Details: moduleName Input parameter. newStage Input parameter. True when the operation succeeds. Calls: lk(), find(), end(), spdlog::debug().

#### `bool verifyModuleSignature(const std::string &modulePath, std::string &errorMessage)`
- Source: `include/themis/base/module_loader.h`:726
- Brief: n/a
- Parameters:
  - `modulePath` (const std::string &): n/a
  - `errorMessage` (std::string &): n/a

#### `uint64_t watchdogCalculateBackoff(uint32_t consecutiveFailures) const`
- Source: `include/themis/base/module_loader.h`:757
- Brief: n/a
- Parameters:
  - `consecutiveFailures` (uint32_t): n/a

#### `void watchdogCheckAllModules()`
- Source: `include/themis/base/module_loader.h`:754
- Brief: Watchdog Check All Modules.
- Parameters: none
- Details: Calls: lk(), emplace_back(), load(), find(), end(), empty(), watchdogRunHealthChecks(), nowMs().

#### `void watchdogLoop()`
- Source: `include/themis/base/module_loader.h`:753
- Brief: Watchdog Loop.
- Parameters: none
- Details: Calls: spdlog::debug(), load(), lk(), wait_for(), std::chrono::milliseconds(), watchdogCheckAllModules().

#### `bool watchdogRestartModule(WatchdogModuleStats &stats, const std::string &modulePath)`
- Source: `include/themis/base/module_loader.h`:756
- Brief: Watchdog Restart Module.
- Parameters:
  - `stats` (WatchdogModuleStats &): Input/output parameter.
  - `modulePath` (const std::string &): Input parameter.
- Return: True when the operation succeeds.
- Details: stats Input/output parameter. modulePath Input parameter. True when the operation succeeds. Calls: spdlog::info(), unloadModule(), loadModule(), nowMs(), watchdogCalculateBackoff(), spdlog::error().

#### `bool watchdogRunHealthChecks(LoadedModule &module, std::string &errorMessage)`
- Source: `include/themis/base/module_loader.h`:755
- Brief: Watchdog Run Health Checks.
- Parameters:
  - `module` (LoadedModule &): Input/output parameter.
  - `errorMessage` (std::string &): Input/output parameter.
- Return: True when the operation succeeds.
- Details: module Input/output parameter. errorMessage Input/output parameter. True when the operation succeeds. Calls: empty(), checkFunc(), what().

#### `~ModuleLoader()`
- Source: `include/themis/base/module_loader.h`:353
- Brief: n/a
- Parameters: none

### themis::modules::ModuleMetadata

#### `bool isValid() const`
- Source: `include/themis/base/module_loader.h`:186
- Brief: n/a
- Parameters: none

### themis::modules::ModuleMetrics

#### `double getAverageLoadDurationMs() const`
- Source: `include/themis/base/module_loader.h`:246
- Brief: n/a
- Parameters: none

#### `double getSuccessRate() const`
- Source: `include/themis/base/module_loader.h`:240
- Brief: n/a
- Parameters: none

### themis::modules::ModuleRegistry

#### `ModuleRegistry()=default`
- Source: `include/themis/base/module_loader.h`:836
- Brief: n/a
- Parameters: none

#### `ModuleRegistry(const ModuleRegistry &)=delete`
- Source: `include/themis/base/module_loader.h`:838
- Brief: n/a
- Parameters:
  - `<unnamed>` (const ModuleRegistry &): n/a

#### `void clear()`
- Source: `include/themis/base/module_loader.h`:833
- Brief: Clear all registrations (for testing).
- Parameters: none
- Details: Clear. Implements clear without additional internal calls.

#### `std::vector< LoadedModule > getAllModules() const`
- Source: `include/themis/base/module_loader.h`:828
- Brief: Get all registered modules.
- Parameters: none

#### `ModuleRegistry & instance()`
- Source: `include/themis/base/module_loader.h`:808
- Brief: Instance.
- Parameters: none
- Return: Return value.
- Details: Return value. Implements instance without additional internal calls.

#### `bool isRegistered(const std::string &moduleName) const`
- Source: `include/themis/base/module_loader.h`:823
- Brief: Check if module is registered.
- Parameters:
  - `moduleName` (const std::string &): n/a

#### `ModuleRegistry & operator=(const ModuleRegistry &)=delete`
- Source: `include/themis/base/module_loader.h`:839
- Brief: n/a
- Parameters:
  - `<unnamed>` (const ModuleRegistry &): n/a

#### `void registerModule(const LoadedModule &module)`
- Source: `include/themis/base/module_loader.h`:813
- Brief: Register a loaded module.
- Parameters:
  - `module` (const LoadedModule &): Input parameter.
- Details: Register Module. module Input parameter. Calls: push_back(), spdlog::debug().

#### `void unregisterModule(const std::string &moduleName)`
- Source: `include/themis/base/module_loader.h`:818
- Brief: Unregister a module.
- Parameters:
  - `moduleName` (const std::string &): Input parameter.
- Details: Unregister Module. moduleName Input parameter. Calls: std::find_if(), begin(), end(), erase(), spdlog::debug().

#### `~ModuleRegistry()=default`
- Source: `include/themis/base/module_loader.h`:837
- Brief: n/a
- Parameters: none

### themis::modules::ModuleSandbox

#### `ModuleSandbox(const Config &config=Config::defaults())`
- Source: `include/themis/base/module_sandbox.h`:287
- Brief: n/a
- Parameters:
  - `config` (const Config &): n/a

#### `ModuleSandbox(const ModuleSandbox &)=delete`
- Source: `include/themis/base/module_sandbox.h`:291
- Brief: n/a
- Parameters:
  - `<unnamed>` (const ModuleSandbox &): n/a

#### `bool applyCpuLimit()`
- Source: `include/themis/base/module_sandbox.h`:394
- Brief: n/a
- Parameters: none

#### `bool applyFilesystemRestrictions()`
- Source: `include/themis/base/module_sandbox.h`:396
- Brief: n/a
- Parameters: none

#### `bool applyMemoryLimit()`
- Source: `include/themis/base/module_sandbox.h`:393
- Brief: n/a
- Parameters: none

#### `bool applyNetworkIsolation()`
- Source: `include/themis/base/module_sandbox.h`:395
- Brief: n/a
- Parameters: none

#### `bool applySyscallFilter()`
- Source: `include/themis/base/module_sandbox.h`:397
- Brief: n/a
- Parameters: none

#### `bool isActive() const noexcept`
- Source: `include/themis/base/module_sandbox.h`:315
- Brief: n/a
- Parameters: none

#### `bool isWasmIsolationActive() const noexcept`
- Source: `include/themis/base/module_sandbox.h`:355
- Brief: True if WASM isolation is active.
- Parameters: none
- Details: WASM isolation is active when Config::enable_wasm_isolation was true and a WasmRuntime was successfully injected during launch().

#### `const std::string & lastError() const noexcept`
- Source: `include/themis/base/module_sandbox.h`:333
- Brief: Last error message produced during launch() or shutdown().
- Parameters: none
- Return: Human-readable error string; empty if no error has occurred.
- Details: Human-readable error string; empty if no error has occurred.

#### `bool launch(const std::string &module_name)`
- Source: `include/themis/base/module_sandbox.h`:306
- Brief: Apply sandbox constraints to the current process/thread context and associate the sandbox with module_name.
- Parameters:
  - `module_name` (const std::string &): Human-readable name for logging/stats.
- Return: true on success (constraints applied); false on fatal error.
- Details: On Linux this configures cgroups and sets up seccomp-bpf. On Windows this creates and assigns a Job Object. module_name Human-readable name for logging/stats. true on success (constraints applied); false on fatal error.

#### `const std::vector< std::string > & launchWarnings() const noexcept`
- Source: `include/themis/base/module_sandbox.h`:324
- Brief: Warnings produced during launch() for unsupported mechanisms.
- Parameters: none
- Return: Reference to the list of warning strings accumulated by launch().
- Details: Reference to the list of warning strings accumulated by launch().

#### `ModuleSandbox & operator=(const ModuleSandbox &)=delete`
- Source: `include/themis/base/module_sandbox.h`:292
- Brief: n/a
- Parameters:
  - `<unnamed>` (const ModuleSandbox &): n/a

#### `bool setupCgroupV2()`
- Source: `include/themis/base/module_sandbox.h`:407
- Brief: Set up a cgroup v2 sub-hierarchy for this sandbox instance.
- Parameters: none
- Details: Creates /sys/fs/cgroup/themis/<sandbox_id>/, enables the memory and cpu controllers in cgroup.subtree_control, writes memory.max, then moves the current process into the new cgroup. If CPU limiting is enabled, cpu.max is written later by applyCpuLimit() when active. Returns true on success; on failure emits spdlog::warn and the caller falls back to RLIMIT_* enforcement.

#### `void shutdown()`
- Source: `include/themis/base/module_sandbox.h`:313
- Brief: Remove all sandbox constraints and release OS resources.
- Parameters: none
- Details: Must be called before the module is unloaded.

#### `SandboxStats stats() const`
- Source: `include/themis/base/module_sandbox.h`:345
- Brief: Sample current resource usage.
- Parameters: none
- Return: SandboxStats snapshot of current memory and CPU utilisation.
- Details: On Linux reads /sys/fs/cgroup/…/memory.current and cpuacct.usage. On Windows queries the Job Object. SandboxStats snapshot of current memory and CPU utilisation.

#### `void teardownCgroupV2()`
- Source: `include/themis/base/module_sandbox.h`:413
- Brief: Remove the cgroup v2 directory created by setupCgroupV2().
- Parameters: none
- Details: Migrates the current process back to the root cgroup before issuing rmdir(2) on the sandbox-specific sub-directory.

#### `const WasmPluginSandbox * wasmSandbox() const noexcept`
- Source: `include/themis/base/module_sandbox.h`:376
- Brief: Returns a read-only pointer to the inner WasmPluginSandbox (const overload).
- Parameters: none
- Return: Const pointer to the inner WasmPluginSandbox, or nullptr if WASM isolation is not active.
- Details: Non-null only when isWasmIsolationActive() is true. Use this to inspect .wasm plugin binaries and their exports without modification. Const pointer to the inner WasmPluginSandbox, or nullptr if WASM isolation is not active.

#### `WasmPluginSandbox * wasmSandbox() noexcept`
- Source: `include/themis/base/module_sandbox.h`:366
- Brief: Return the inner WasmPluginSandbox.
- Parameters: none
- Return: Pointer to the inner WasmPluginSandbox, or nullptr if WASM isolation is not active.
- Details: Non-null only when isWasmIsolationActive() is true. Use this to load .wasm plugin binaries and call their exports. Pointer to the inner WasmPluginSandbox, or nullptr if WASM isolation is not active.

#### `~ModuleSandbox()`
- Source: `include/themis/base/module_sandbox.h`:288
- Brief: n/a
- Parameters: none

### themis::modules::ModuleSandbox::Config

#### `Config defaults()`
- Source: `include/themis/base/module_sandbox.h`:284
- Brief: Construct a Config with safe default values.
- Parameters: none
- Return: Config with 256 MiB memory limit, 50 % CPU share, read-only filesystem access, and no network.
- Details: Config with 256 MiB memory limit, 50 % CPU share, read-only filesystem access, and no network.

### themis::modules::ModuleSecurityVerifier

#### `ModuleSecurityVerifier()`
- Source: `include/themis/base/module_loader.h`:769
- Brief: n/a
- Parameters: none

#### `void addBlacklistedHash(const std::string &hash)`
- Source: `include/themis/base/module_loader.h`:793
- Brief: Add Blacklisted Hash.
- Parameters:
  - `hash` (const std::string &): Input parameter.
- Details: hash Input parameter. Implements addBlacklistedHash without additional internal calls.

#### `void addWhitelistedHash(const std::string &hash)`
- Source: `include/themis/base/module_loader.h`:792
- Brief: Add Whitelisted Hash.
- Parameters:
  - `hash` (const std::string &): Input parameter.
- Details: hash Input parameter. Implements addWhitelistedHash without additional internal calls.

#### `std::string calculateFileHash(const std::string &modulePath)`
- Source: `include/themis/base/module_loader.h`:785
- Brief: Calculate SHA-256 hash of module file.
- Parameters:
  - `modulePath` (const std::string &): Input parameter.
- Return: Hex-encoded SHA-256 hash (empty string on error)
- Details: Calculate File Hash. modulePath Path to module file Hex-encoded SHA-256 hash (empty string on error) modulePath Input parameter. Return value. Implements calculateFileHash without additional internal calls.

#### `void setAllowUnsigned(bool allow)`
- Source: `include/themis/base/module_loader.h`:791
- Brief: Set Allow Unsigned.
- Parameters:
  - `allow` (bool): Input parameter.
- Details: allow Input parameter. Implements setAllowUnsigned without additional internal calls.

#### `void setRequireSignature(bool require)`
- Source: `include/themis/base/module_loader.h`:790
- Brief: Set security policy.
- Parameters:
  - `require` (bool): Input parameter.
- Details: Set Require Signature. require Input parameter. Implements setRequireSignature without additional internal calls.

#### `bool verifyModule(const std::string &modulePath, std::string &errorMessage)`
- Source: `include/themis/base/module_loader.h`:778
- Brief: Verify a module before loading.
- Parameters:
  - `modulePath` (const std::string &): Input parameter.
  - `errorMessage` (std::string &): Input/output parameter.
- Return: true if module is safe to load, false otherwise
- Details: Verify Module. modulePath Path to module DLL/SO errorMessage Output parameter for error details true if module is safe to load, false otherwise modulePath Input parameter. errorMessage Input/output parameter. True when the operation succeeds. Implements verifyModule without additional internal calls.

#### `~ModuleSecurityVerifier()`
- Source: `include/themis/base/module_loader.h`:770
- Brief: n/a
- Parameters: none

### themis::modules::ModuleSecurityVerifier::Impl

#### `Impl()`
- Source: `src/themis/module_security.cpp`:34
- Brief: n/a
- Parameters: none

#### `void addBlacklistedHash(const std::string &hash)`
- Source: `src/themis/module_security.cpp`:121
- Brief: Add Blacklisted Hash.
- Parameters:
  - `hash` (const std::string &): Input parameter.
- Details: hash Input parameter. Calls: push_back(), updatePolicy().

#### `void addWhitelistedHash(const std::string &hash)`
- Source: `src/themis/module_security.cpp`:111
- Brief: Add Whitelisted Hash.
- Parameters:
  - `hash` (const std::string &): Input parameter.
- Details: hash Input parameter. Calls: push_back(), updatePolicy().

#### `std::string calculateFileHash(const std::string &modulePath)`
- Source: `src/themis/module_security.cpp`:82
- Brief: Calculate File Hash.
- Parameters:
  - `modulePath` (const std::string &): Input parameter.
- Return: Return value.
- Details: modulePath Input parameter. Return value. Implements calculateFileHash without additional internal calls.

#### `void setAllowUnsigned(bool allow)`
- Source: `src/themis/module_security.cpp`:101
- Brief: Set Allow Unsigned.
- Parameters:
  - `allow` (bool): Input parameter.
- Details: allow Input parameter. Calls: updatePolicy().

#### `void setRequireSignature(bool require)`
- Source: `src/themis/module_security.cpp`:91
- Brief: Set Require Signature.
- Parameters:
  - `require` (bool): Input parameter.
- Details: require Input parameter. Calls: updatePolicy().

#### `bool verifyModule(const std::string &modulePath, std::string &errorMessage)`
- Source: `src/themis/module_security.cpp`:62
- Brief: Verify Module.
- Parameters:
  - `modulePath` (const std::string &): Input parameter.
  - `errorMessage` (std::string &): Input/output parameter.
- Return: True when the operation succeeds.
- Details: modulePath Input parameter. errorMessage Input/output parameter. True when the operation succeeds. Calls: spdlog::debug(), verifyPlugin(), spdlog::info(), spdlog::error().

### themis::modules::ModuleSignatureVerifier

#### `ModuleSignatureVerificationResult verifySignature(const std::string &modulePath, const std::string &signaturePath="")`
- Source: `include/themis/module_signature_verifier.h`:65
- Brief: Verify the digital signature of a module file.
- Parameters:
  - `modulePath` (const std::string &): Input parameter.
  - `signaturePath` (const std::string &): Input parameter.
- Return: Verification result with success flag, signer info, and any error.
- Details: Verify Signature. Dispatches to verifyAuthenticodeSignature() on Windows or verifyGPGSignature() on Linux. modulePath Absolute path to the module file (.dll / .so). signaturePath Path to detached signature (Linux only; ignored on Windows). Pass an empty string to auto-detect. Verification result with success flag, signer info, and any error. modulePath Input parameter. signaturePath Input parameter. Return value.

### themis::modules::ModuleVersion

#### `ModuleVersion fromMetadata(const ModuleMetadata &m)`
- Source: `include/themis/base/hot_reload_manager.h`:69
- Brief: Construct from ModuleMetadata.
- Parameters:
  - `m` (const ModuleMetadata &): n/a

#### `bool operator!=(const ModuleVersion &o) const noexcept`
- Source: `include/themis/base/hot_reload_manager.h`:57
- Brief: n/a
- Parameters:
  - `o` (const ModuleVersion &): n/a

#### `bool operator==(const ModuleVersion &o) const noexcept`
- Source: `include/themis/base/hot_reload_manager.h`:53
- Brief: n/a
- Parameters:
  - `o` (const ModuleVersion &): n/a

#### `std::string toString() const`
- Source: `include/themis/base/hot_reload_manager.h`:60
- Brief: n/a
- Parameters: none
- Return: "major.minor.patch" or version string, whichever is available.
- Details: "major.minor.patch" or version string, whichever is available.

### themis::modules::PluginBundleLoader

#### `PluginBundleLoader()`
- Source: `include/themis/base/module_loader.h`:1077
- Brief: n/a
- Parameters: none

#### `PluginBundleLoader(const PluginBundleLoader &)=delete`
- Source: `include/themis/base/module_loader.h`:1080
- Brief: n/a
- Parameters:
  - `<unnamed>` (const PluginBundleLoader &): n/a

#### `std::string currentPlatform()`
- Source: `include/themis/base/module_loader.h`:1114
- Brief: Return the canonical platform token for the current build.
- Parameters: none
- Return: Return value.
- Details: Current Platform. Examples: "linux-x86_64", "linux-arm64", "windows-x86_64", "macos-arm64", "macos-x86_64". Return value. Calls: defined().

#### `std::string extractToTempDir(const std::string &bundlePath, std::string &error)`
- Source: `include/themis/base/module_loader.h`:1164
- Brief: Extract To Temp Dir.
- Parameters:
  - `bundlePath` (const std::string &): Input parameter.
  - `error` (std::string &): Input/output parameter.
- Return: Return value.
- Details: Unpack the ZIP at bundlePath into a new temporary subdirectory. Returns the path to the temp dir on success or an empty string + error. bundlePath Input parameter. error Input/output parameter. Return value. Calls: zip_open(), c_str(), zip_error_init_with_code(), std::string(), zip_error_strerror(), zip_error_fini(), makeTempDirPath(), fs::create_directories().

#### `PluginBundleLoadResult loadBundle(const std::string &bundlePath, ModuleLoader &loader)`
- Source: `include/themis/base/module_loader.h`:1124
- Brief: Load a PluginBundle archive and delegate to a ModuleLoader.
- Parameters:
  - `bundlePath` (const std::string &): Input parameter.
  - `loader` (ModuleLoader &): Input/output parameter.
- Return: PluginBundleLoadResult indicating success/failure and details.
- Details: Load Bundle. bundlePath Path to the .tdb (ZIP) bundle file. loader ModuleLoader instance used for the final native- library load step. PluginBundleLoadResult indicating success/failure and details. bundlePath Input parameter. loader Input/output parameter. Return value. Calls: extractToTempDir(), empty(), spdlog::error(), spdlog::debug(), tempGuard(), fs::path(), manifestFile(), is_open().

#### `PluginBundleLoader & operator=(const PluginBundleLoader &)=delete`
- Source: `include/themis/base/module_loader.h`:1081
- Brief: n/a
- Parameters:
  - `<unnamed>` (const PluginBundleLoader &): n/a

#### `bool parseManifest(const std::string &jsonText, PluginBundleManifest &manifest, std::string &error)`
- Source: `include/themis/base/module_loader.h`:1138
- Brief: Parse a PluginBundleManifest from raw JSON text.
- Parameters:
  - `jsonText` (const std::string &): Input parameter.
  - `manifest` (PluginBundleManifest &): Input/output parameter.
  - `error` (std::string &): Input/output parameter.
- Return: true if parsing succeeded and the manifest is valid.
- Details: Parse Manifest. Exposed as a public static helper to facilitate unit testing without requiring a real ZIP archive. jsonText Contents of manifest.json. manifest Output: populated on success. error Output: human-readable error message on failure. true if parsing succeeded and the manifest is valid. jsonText Input parameter. manifest Input/output parameter. error Input/output parameter. True when the operation succeeds. Calls: nlohmann::json::parse(), contains(), is_string(), empty(), is_object(), items(), std::string(), what().

#### `void setAllowUnsignedBundles(bool allow)`
- Source: `include/themis/base/module_loader.h`:1106
- Brief: Explicitly allow loading bundles without a signature.
- Parameters:
  - `allow` (bool): Input parameter.
- Details: Set Allow Unsigned Bundles. By default (allow = false) loadBundle() fails when no public key has been configured, so that unsigned bundles are never silently accepted. Pass allow = true only in development / testing environments where you intentionally want to skip signature verification. allow If true, unsigned bundles are accepted when no public key is set. If false (default), missing public key is an error. allow Input parameter. Implements setAllowUnsignedBundles without additional internal calls.

#### `void setPublicKey(const std::string &publicKeyPem)`
- Source: `include/themis/base/module_loader.h`:1093
- Brief: Set the PEM-encoded Ed25519 public key used to verify bundle signatures.
- Parameters:
  - `publicKeyPem` (const std::string &): Input parameter.
- Details: Set Public Key. When no key is set (the default) signature verification is skipped, which is only suitable for development / unit-testing. In production, always set a public key before calling loadBundle(). publicKeyPem PEM-encoded Ed25519 public key. publicKeyPem Input parameter. Implements setPublicKey without additional internal calls.

#### `bool verifyEd25519Signature(const uint8_t *message, size_t messageLen, const std::vector< uint8_t > &signatureBytes, const std::string &publicKeyPem, std::string &error)`
- Source: `include/themis/base/module_loader.h`:1152
- Brief: Verify an Ed25519 signature over a message.
- Parameters:
  - `message` (const uint8_t *): Input parameter.
  - `messageLen` (size_t): Input parameter.
  - `signatureBytes` (const std::vector< uint8_t > &): Input parameter.
  - `publicKeyPem` (const std::string &): Input parameter.
  - `error` (std::string &): Input/output parameter.
- Return: true if the signature is valid; false otherwise.
- Details: Verify Ed25519 Signature. message Pointer to the data that was signed. messageLen Length of the data in bytes. signatureBytes Raw 64-byte Ed25519 signature. publicKeyPem PEM-encoded Ed25519 public key. error Output: error description on failure. true if the signature is valid; false otherwise. message Input parameter. messageLen Input parameter. signatureBytes Input parameter. publicKeyPem Input parameter. error Input/output parameter. True when the operation succeeds. Calls: size(), std::to_string(), BIO_new_mem_buf(), data(), PEM_read_bio_PUBKEY(), BIO_free(), opensslLastError(), EVP_MD_CTX_new().

#### `~PluginBundleLoader()`
- Source: `include/themis/base/module_loader.h`:1078
- Brief: n/a
- Parameters: none

### themis::modules::PluginBundleManifest

#### `bool isValid() const`
- Source: `include/themis/base/module_loader.h`:1030
- Brief: Returns true when the mandatory fields (name, version) are non-empty.
- Parameters: none

### themis::modules::PluginDependencyGraph

#### `void addDependency(const std::string &from, const std::string &to, bool required=true, const std::string &minVersion="", const std::string &maxVersion="")`
- Source: `include/themis/base/plugin_dependency_graph.h`:125
- Brief: Add a directed dependency edge.
- Parameters:
  - `from` (const std::string &): Dependent module name.
  - `to` (const std::string &): Dependency module name.
  - `required` (bool): True if the dependency is required, false if optional.
  - `minVersion` (const std::string &): Minimum version of to required by from (may be "").
  - `maxVersion` (const std::string &): Maximum version of to required by from (may be "").
- Details: Both from and to are added as nodes if not already present. from Dependent module name. to Dependency module name. required True if the dependency is required, false if optional. minVersion Minimum version of to required by from (may be ""). maxVersion Maximum version of to required by from (may be "").

#### `void addModule(const std::string &name, const std::string &version="")`
- Source: `include/themis/base/plugin_dependency_graph.h`:112
- Brief: Add a module node with an optional version string.
- Parameters:
  - `name` (const std::string &): Module name.
  - `version` (const std::string &): Semantic version string (may be "").
- Details: Registering the same name twice replaces the previous node. name Module name. version Semantic version string (may be "").

#### `std::map< std::string, std::set< std::string > > buildAdjacency() const`
- Source: `include/themis/base/plugin_dependency_graph.h`:232
- Brief: Build an adjacency list (name → set of direct dependency names).
- Parameters: none

#### `void buildFromRegistry()`
- Source: `include/themis/base/plugin_dependency_graph.h`:147
- Brief: Populate the graph from the ModuleRegistry singleton.
- Parameters: none
- Details: Uses the currently registered (loaded) modules and the dependency metadata stored in each module's LoadedModule::metadata field. Replaces any previously added nodes and edges.

#### `void buildFromResolver(const ModuleDependencyResolver &resolver)`
- Source: `include/themis/base/plugin_dependency_graph.h`:138
- Brief: Populate the graph from a ModuleDependencyResolver.
- Parameters:
  - `resolver` (const ModuleDependencyResolver &): Source resolver (must already have modules registered).
- Details: Replaces any previously added nodes and edges. resolver Source resolver (must already have modules registered).

#### `void clear()`
- Source: `include/themis/base/plugin_dependency_graph.h`:152
- Brief: Remove all nodes and edges.
- Parameters: none

#### `std::vector< std::vector< std::string > > detectCycles() const`
- Source: `include/themis/base/plugin_dependency_graph.h`:176
- Brief: Detect cycles in the dependency graph.
- Parameters: none
- Return: Each inner vector is one cycle (list of module names). Empty if the graph is acyclic.
- Details: Each inner vector is one cycle (list of module names). Empty if the graph is acyclic.

#### `void dfsVisit(const std::string &node, const std::map< std::string, std::set< std::string > > &adj, std::map< std::string, int > &color, std::vector< std::string > &path, std::vector< std::vector< std::string > > &cycles) const`
- Source: `include/themis/base/plugin_dependency_graph.h`:235
- Brief: DFS helper used by detectCycles().
- Parameters:
  - `node` (const std::string &): n/a
  - `adj` (const std::map< std::string, std::set< std::string > > &): n/a
  - `color` (std::map< std::string, int > &): n/a
  - `path` (std::vector< std::string > &): n/a
  - `cycles` (std::vector< std::vector< std::string > > &): n/a

#### `std::size_t edgeCount() const`
- Source: `include/themis/base/plugin_dependency_graph.h`:162
- Brief: Number of edges in the graph.
- Parameters: none

#### `const std::vector< Edge > & edges() const`
- Source: `include/themis/base/plugin_dependency_graph.h`:168
- Brief: All edges.
- Parameters: none

#### `std::string escapeDotId(const std::string &s)`
- Source: `include/themis/base/plugin_dependency_graph.h`:228
- Brief: Escape a string for use as a DOT identifier / JSON string value.
- Parameters:
  - `s` (const std::string &): n/a

#### `std::string escapeJson(const std::string &s)`
- Source: `include/themis/base/plugin_dependency_graph.h`:229
- Brief: n/a
- Parameters:
  - `s` (const std::string &): n/a

#### `void exportTo(std::ostream &out, GraphExportFormat format) const`
- Source: `include/themis/base/plugin_dependency_graph.h`:195
- Brief: Export the graph in the requested format to an output stream.
- Parameters:
  - `out` (std::ostream &): Destination stream (file, std::cout, std::ostringstream…).
  - `format` (GraphExportFormat): One of DOT, JSON, or ASCII.
- Details: out Destination stream (file, std::cout, std::ostringstream…). format One of DOT, JSON, or ASCII.

#### `std::size_t nodeCount() const`
- Source: `include/themis/base/plugin_dependency_graph.h`:159
- Brief: Number of nodes in the graph.
- Parameters: none

#### `std::vector< Node > nodes() const`
- Source: `include/themis/base/plugin_dependency_graph.h`:165
- Brief: All nodes, sorted by name.
- Parameters: none

#### `void renderAscii(std::ostream &out) const`
- Source: `include/themis/base/plugin_dependency_graph.h`:221
- Brief: n/a
- Parameters:
  - `out` (std::ostream &): n/a

#### `void renderDot(std::ostream &out) const`
- Source: `include/themis/base/plugin_dependency_graph.h`:219
- Brief: n/a
- Parameters:
  - `out` (std::ostream &): n/a

#### `void renderJson(std::ostream &out) const`
- Source: `include/themis/base/plugin_dependency_graph.h`:220
- Brief: n/a
- Parameters:
  - `out` (std::ostream &): n/a

#### `std::string toString(GraphExportFormat format) const`
- Source: `include/themis/base/plugin_dependency_graph.h`:205
- Brief: Export the graph and return it as a string.
- Parameters:
  - `format` (GraphExportFormat): One of DOT, JSON, or ASCII.
- Return: Serialised graph.
- Details: Convenience wrapper around exportTo(). format One of DOT, JSON, or ASCII. Serialised graph.

#### `std::vector< std::string > topologicalOrder() const`
- Source: `include/themis/base/plugin_dependency_graph.h`:183
- Brief: Compute a topological load order (leaves first).
- Parameters: none
- Return: Ordered module names, or empty if cycles prevent ordering.
- Details: Ordered module names, or empty if cycles prevent ordering.

### themis::modules::RemoteRegistryClient

#### `RemoteRegistryClient(const RegistryConfig &config)`
- Source: `include/themis/base/remote_registry_client.h`:250
- Brief: n/a
- Parameters:
  - `config` (const RegistryConfig &): n/a

#### `RemoteRegistryClient(const RemoteRegistryClient &)=delete`
- Source: `include/themis/base/remote_registry_client.h`:253
- Brief: n/a
- Parameters:
  - `<unnamed>` (const RemoteRegistryClient &): n/a

#### `void asyncBackoffSleep(int ms)`
- Source: `include/themis/base/remote_registry_client.h`:293
- Brief: n/a
- Parameters:
  - `ms` (int): n/a

#### `std::string buildAuthorizationHeader() const`
- Source: `include/themis/base/remote_registry_client.h`:292
- Brief: n/a
- Parameters: none

#### `const RegistryConfig & config() const`
- Source: `include/themis/base/remote_registry_client.h`:268
- Brief: n/a
- Parameters: none

#### `ModuleVerificationResult downloadAndLoad(const RegistryPluginEntry &entry, ModuleLoader &loader)`
- Source: `include/themis/base/remote_registry_client.h`:265
- Brief: n/a
- Parameters:
  - `entry` (const RegistryPluginEntry &): n/a
  - `loader` (ModuleLoader &): n/a

#### `PluginDownloadResult downloadPlugin(const RegistryPluginEntry &entry)`
- Source: `include/themis/base/remote_registry_client.h`:262
- Brief: n/a
- Parameters:
  - `entry` (const RegistryPluginEntry &): n/a

#### `std::future< PluginDownloadResult > downloadPluginAsync(const RegistryPluginEntry &entry)`
- Source: `include/themis/base/remote_registry_client.h`:263
- Brief: n/a
- Parameters:
  - `entry` (const RegistryPluginEntry &): n/a

#### `std::optional< RegistryPluginEntry > fetchPlugin(const std::string &name)`
- Source: `include/themis/base/remote_registry_client.h`:259
- Brief: n/a
- Parameters:
  - `name` (const std::string &): n/a

#### `std::future< std::optional< RegistryPluginEntry > > fetchPluginAsync(const std::string &name)`
- Source: `include/themis/base/remote_registry_client.h`:260
- Brief: n/a
- Parameters:
  - `name` (const std::string &): n/a

#### `std::string httpGet(const std::string &url)`
- Source: `include/themis/base/remote_registry_client.h`:287
- Brief: n/a
- Parameters:
  - `url` (const std::string &): n/a

#### `std::future< std::string > httpGetAsync(const std::string &url)`
- Source: `include/themis/base/remote_registry_client.h`:270
- Brief: n/a
- Parameters:
  - `url` (const std::string &): n/a

#### `bool httpGetBinary(const std::string &url, const std::string &out_path)`
- Source: `include/themis/base/remote_registry_client.h`:288
- Brief: n/a
- Parameters:
  - `url` (const std::string &): n/a
  - `out_path` (const std::string &): n/a

#### `std::future< bool > httpGetBinaryAsync(const std::string &url, const std::string &out_path)`
- Source: `include/themis/base/remote_registry_client.h`:271
- Brief: n/a
- Parameters:
  - `url` (const std::string &): n/a
  - `out_path` (const std::string &): n/a

#### `RequestStats lastRequestStats() const`
- Source: `include/themis/base/remote_registry_client.h`:277
- Brief: n/a
- Parameters: none

#### `std::vector< RegistryPluginEntry > listPlugins()`
- Source: `include/themis/base/remote_registry_client.h`:256
- Brief: n/a
- Parameters: none

#### `std::future< std::vector< RegistryPluginEntry > > listPluginsAsync()`
- Source: `include/themis/base/remote_registry_client.h`:257
- Brief: n/a
- Parameters: none

#### `RemoteRegistryClient & operator=(const RemoteRegistryClient &)=delete`
- Source: `include/themis/base/remote_registry_client.h`:254
- Brief: n/a
- Parameters:
  - `<unnamed>` (const RemoteRegistryClient &): n/a

#### `bool parseEntry(const nlohmann::json &obj, RegistryPluginEntry &out)`
- Source: `include/themis/base/remote_registry_client.h`:294
- Brief: n/a
- Parameters:
  - `obj` (const nlohmann::json &): n/a
  - `out` (RegistryPluginEntry &): n/a

#### `void setBackoffDispatcher(std::function< std::future< void >(std::chrono::milliseconds)> dispatcher)`
- Source: `include/themis/base/remote_registry_client.h`:274
- Brief: n/a
- Parameters:
  - `dispatcher` (std::function< std::future< void >(std::chrono::milliseconds)>): n/a

#### `void setObservabilityHook(ObservabilityHook hook)`
- Source: `include/themis/base/remote_registry_client.h`:248
- Brief: Register a callback to receive retry-exhaust and timeout events.
- Parameters:
  - `hook` (ObservabilityHook): Callback; must be thread-safe and non-blocking.
- Details: Replaces any previously registered hook. Pass a default-constructed ObservabilityHook (empty function) to disable the hook. hook Callback; must be thread-safe and non-blocking.

#### `bool verifyIntegrity(const std::string &file_path, const std::string &expected_sha256)`
- Source: `include/themis/base/remote_registry_client.h`:290
- Brief: n/a
- Parameters:
  - `file_path` (const std::string &): n/a
  - `expected_sha256` (const std::string &): n/a

#### `~RemoteRegistryClient()`
- Source: `include/themis/base/remote_registry_client.h`:251
- Brief: n/a
- Parameters: none

### themis::modules::ScopedSpan

#### `ScopedSpan(ScopedSpan &&)=delete`
- Source: `include/themis/base/trace_context.h`:234
- Brief: n/a
- Parameters:
  - `<unnamed>` (ScopedSpan &&): n/a

#### `ScopedSpan(const ScopedSpan &)=delete`
- Source: `include/themis/base/trace_context.h`:232
- Brief: n/a
- Parameters:
  - `<unnamed>` (const ScopedSpan &): n/a

#### `ScopedSpan(const TraceContext &ctx, SpanEmitter &emitter)`
- Source: `include/themis/base/trace_context.h`:204
- Brief: Construct and emit a span-start event.
- Parameters:
  - `ctx` (const TraceContext &): Trace context for this span.
  - `emitter` (SpanEmitter &): Emitter to receive span events (must outlive ScopedSpan).
- Details: ctx Trace context for this span. emitter Emitter to receive span events (must outlive ScopedSpan).

#### `ScopedSpan & operator=(ScopedSpan &&)=delete`
- Source: `include/themis/base/trace_context.h`:235
- Brief: n/a
- Parameters:
  - `<unnamed>` (ScopedSpan &&): n/a

#### `ScopedSpan & operator=(const ScopedSpan &)=delete`
- Source: `include/themis/base/trace_context.h`:233
- Brief: n/a
- Parameters:
  - `<unnamed>` (const ScopedSpan &): n/a

#### `void setError(int code, std::string detail) noexcept`
- Source: `include/themis/base/trace_context.h`:226
- Brief: Mark this span as having ended with an error.
- Parameters:
  - `code` (int): Base module taxonomy error code.
  - `detail` (std::string): Human-readable detail string.
- Details: code Base module taxonomy error code. detail Human-readable detail string.

#### `~ScopedSpan()`
- Source: `include/themis/base/trace_context.h`:213
- Brief: n/a
- Parameters: none

### themis::modules::TraceContext

#### `TraceContext child(std::string_view operation) const`
- Source: `include/themis/base/trace_context.h`:107
- Brief: Derive a child TraceContext that shares this span's trace ID.
- Parameters:
  - `operation` (std::string_view): Operation name for the child span.
- Return: Child TraceContext.
- Details: The child receives a fresh span ID; its parent_span_id is set to this span's span_id. operation Operation name for the child span. Child TraceContext.

#### `TraceContext generate(std::string_view operation)`
- Source: `include/themis/base/trace_context.h`:89
- Brief: Generate a new root TraceContext with a unique trace ID and span ID.
- Parameters:
  - `operation` (std::string_view): Operation name for this span.
- Return: New root TraceContext.
- Details: operation Operation name for this span. New root TraceContext.

#### `bool isValid() const noexcept`
- Source: `include/themis/base/trace_context.h`:117
- Brief: Return true when this context carries a valid (non-zero) span ID.
- Parameters: none

#### `uint64_t nextId() noexcept`
- Source: `include/themis/base/trace_context.h`:121
- Brief: Thread-safe monotonic ID counter.
- Parameters: none

### themis::modules::WasmModuleInfo

#### `std::string summary() const`
- Source: `include/themis/base/wasm_plugin_sandbox.h`:69
- Brief: Human-readable one-liner summary.
- Parameters: none

### themis::modules::WasmModuleValidator

#### `const uint8_t * magicBytes() noexcept`
- Source: `include/themis/base/wasm_plugin_sandbox.h`:451
- Brief: Return the 4 WASM magic bytes { 0x00, 0x61, 0x73, 0x6d }.
- Parameters: none

#### `WasmModuleInfo validate(const std::vector< uint8_t > &bytes)`
- Source: `include/themis/base/wasm_plugin_sandbox.h`:443
- Brief: Check that bytes starts with the WASM magic bytes and a supported version.
- Parameters:
  - `bytes` (const std::vector< uint8_t > &): n/a
- Return: A WasmModuleInfo with valid==true on success.
- Details: A WasmModuleInfo with valid==true on success.

#### `WasmModuleInfo validateFile(const std::string &path)`
- Source: `include/themis/base/wasm_plugin_sandbox.h`:448
- Brief: Convenience overload: read the file at path and validate.
- Parameters:
  - `path` (const std::string &): n/a

### themis::modules::WasmPluginSandbox

#### `WasmPluginSandbox(const Config &config=Config::defaults())`
- Source: `include/themis/base/wasm_plugin_sandbox.h`:266
- Brief: n/a
- Parameters:
  - `config` (const Config &): n/a

#### `WasmPluginSandbox(const WasmPluginSandbox &)=delete`
- Source: `include/themis/base/wasm_plugin_sandbox.h`:269
- Brief: n/a
- Parameters:
  - `<unnamed>` (const WasmPluginSandbox &): n/a

#### `void addHostFunction(WasmHostFunction fn)`
- Source: `include/themis/base/wasm_plugin_sandbox.h`:298
- Brief: Register a host function that the WASM plugin is allowed to call.
- Parameters:
  - `fn` (WasmHostFunction): n/a
- Details: Must be called before loadFromFile() / loadFromBytes().

#### `bool allocateLinearMemory()`
- Source: `include/themis/base/wasm_plugin_sandbox.h`:422
- Brief: n/a
- Parameters: none

#### `WasmCallResult callExport(const std::string &export_name, const std::vector< uint8_t > &args={})`
- Source: `include/themis/base/wasm_plugin_sandbox.h`:351
- Brief: Call a named export in the loaded WASM plugin.
- Parameters:
  - `export_name` (const std::string &): Name of the WASM export function.
  - `args` (const std::vector< uint8_t > &): Serialised argument blob (format is plugin-defined).
- Return: WasmCallResult describing success, output, duration.
- Details: Requires a runtime to be injected; otherwise returns an error result. export_name Name of the WASM export function. args Serialised argument blob (format is plugin-defined). WasmCallResult describing success, output, duration.

#### `bool checkImportAllowlist()`
- Source: `include/themis/base/wasm_plugin_sandbox.h`:421
- Brief: n/a
- Parameters: none

#### `void clearHostFunctions()`
- Source: `include/themis/base/wasm_plugin_sandbox.h`:303
- Brief: Remove all registered host functions.
- Parameters: none

#### `std::string engineName() const`
- Source: `include/themis/base/wasm_plugin_sandbox.h`:289
- Brief: Return the engine name (empty string if no runtime).
- Parameters: none

#### `bool hasRuntime() const noexcept`
- Source: `include/themis/base/wasm_plugin_sandbox.h`:286
- Brief: Return true if a runtime has been injected.
- Parameters: none

#### `size_t hostFunctionCount() const noexcept`
- Source: `include/themis/base/wasm_plugin_sandbox.h`:306
- Brief: Return the number of registered host functions.
- Parameters: none

#### `bool isLoaded() const noexcept`
- Source: `include/themis/base/wasm_plugin_sandbox.h`:356
- Brief: n/a
- Parameters: none

#### `const std::string & lastError() const noexcept`
- Source: `include/themis/base/wasm_plugin_sandbox.h`:357
- Brief: n/a
- Parameters: none

#### `bool launchOsSandbox(const std::string &module_name)`
- Source: `include/themis/base/wasm_plugin_sandbox.h`:423
- Brief: n/a
- Parameters:
  - `module_name` (const std::string &): n/a

#### `const uint8_t * linearMemory() const noexcept`
- Source: `include/themis/base/wasm_plugin_sandbox.h`:373
- Brief: Return a pointer to the plugin's linear memory.
- Parameters: none
- Return: nullptr if not loaded.
- Details: nullptr if not loaded.

#### `uint8_t * linearMemory() noexcept`
- Source: `include/themis/base/wasm_plugin_sandbox.h`:374
- Brief: n/a
- Parameters: none

#### `size_t linearMemorySize() const noexcept`
- Source: `include/themis/base/wasm_plugin_sandbox.h`:377
- Brief: Byte size of the linear memory arena.
- Parameters: none

#### `bool loadFromBytes(const std::vector< uint8_t > &bytes, const std::string &module_name="anonymous")`
- Source: `include/themis/base/wasm_plugin_sandbox.h`:332
- Brief: Validate and load a .wasm plugin from an in-memory byte buffer.
- Parameters:
  - `bytes` (const std::vector< uint8_t > &): n/a
  - `module_name` (const std::string &): n/a
- Return: true on success.
- Details: Same steps as loadFromFile() but reads from bytes directly. true on success.

#### `bool loadFromFile(const std::string &path)`
- Source: `include/themis/base/wasm_plugin_sandbox.h`:323
- Brief: Validate and load a .wasm plugin from a file path.
- Parameters:
  - `path` (const std::string &): n/a
- Return: true on success.
- Details: Steps: Read the file into memory. Validate the WASM magic bytes and version. Parse import/export sections to check against the allowlist. Allocate the linear memory arena. Instantiate via the injected WasmRuntime (if any). Launch the inner ModuleSandbox resource limits. true on success.

#### `const std::vector< std::string > & loadWarnings() const noexcept`
- Source: `include/themis/base/wasm_plugin_sandbox.h`:363
- Brief: Warnings produced during load (e.g. sandbox limitations).
- Parameters: none

#### `const WasmModuleInfo & moduleInfo() const noexcept`
- Source: `include/themis/base/wasm_plugin_sandbox.h`:360
- Brief: Metadata parsed from the loaded .wasm binary.
- Parameters: none

#### `WasmPluginSandbox & operator=(const WasmPluginSandbox &)=delete`
- Source: `include/themis/base/wasm_plugin_sandbox.h`:270
- Brief: n/a
- Parameters:
  - `<unnamed>` (const WasmPluginSandbox &): n/a

#### `bool parseImportsExports(const std::vector< uint8_t > &bytes)`
- Source: `include/themis/base/wasm_plugin_sandbox.h`:420
- Brief: n/a
- Parameters:
  - `bytes` (const std::vector< uint8_t > &): n/a

#### `uint64_t remainingFuel() const noexcept`
- Source: `include/themis/base/wasm_plugin_sandbox.h`:401
- Brief: Return the number of fuel units remaining in the sandbox budget.
- Parameters: none
- Details: When Config::max_instructions is 0 (unlimited), this always returns UINT64_MAX. After a fuel-exhaustion error the value is 0. Fuel is reset to Config::max_instructions when a new module is loaded via loadFromBytes() / loadFromFile().

#### `void setRuntime(std::unique_ptr< WasmRuntime > runtime)`
- Source: `include/themis/base/wasm_plugin_sandbox.h`:283
- Brief: Inject a concrete WASM execution engine.
- Parameters:
  - `runtime` (std::unique_ptr< WasmRuntime >): Owning pointer to the runtime.
- Details: If no runtime is set the sandbox operates in validation-only mode: it validates the binary and the host-function allowlist but callExport() will return an error. runtime Owning pointer to the runtime.

#### `Stats stats() const noexcept`
- Source: `include/themis/base/wasm_plugin_sandbox.h`:388
- Brief: n/a
- Parameters: none

#### `void unload()`
- Source: `include/themis/base/wasm_plugin_sandbox.h`:338
- Brief: Unload the plugin and release all resources.
- Parameters: none

#### `bool validateWasmHeader(const std::vector< uint8_t > &bytes)`
- Source: `include/themis/base/wasm_plugin_sandbox.h`:419
- Brief: n/a
- Parameters:
  - `bytes` (const std::vector< uint8_t > &): n/a

#### `~WasmPluginSandbox()`
- Source: `include/themis/base/wasm_plugin_sandbox.h`:267
- Brief: n/a
- Parameters: none

### themis::modules::WasmPluginSandbox::Config

#### `Config defaults()`
- Source: `include/themis/base/wasm_plugin_sandbox.h`:261
- Brief: n/a
- Parameters: none

### themis::modules::WasmRuntime

#### `bool call(const std::string &export_name, const std::vector< uint8_t > &args, std::vector< uint8_t > &out)=0`
- Source: `include/themis/base/wasm_plugin_sandbox.h`:158
- Brief: Call a named exported function.
- Parameters:
  - `export_name` (const std::string &): Name of the WASM export to invoke.
  - `args` (const std::vector< uint8_t > &): Serialised argument blob.
  - `out` (std::vector< uint8_t > &): Output blob filled by the runtime.
- Return: true on success; false on trap or missing export.
- Details: export_name Name of the WASM export to invoke. args Serialised argument blob. out Output blob filled by the runtime. true on success; false on trap or missing export.

#### `void destroy()=0`
- Source: `include/themis/base/wasm_plugin_sandbox.h`:163
- Brief: Release all resources held by this runtime instance.
- Parameters: none

#### `std::string engineName() const =0`
- Source: `include/themis/base/wasm_plugin_sandbox.h`:166
- Brief: Human-readable name of the engine (e.g. "wasmtime-0.35").
- Parameters: none

#### `bool instantiate(const std::vector< uint8_t > &wasm_bytes, const std::vector< WasmHostFunction > &host_fns, uint8_t *linear_memory, size_t memory_size)=0`
- Source: `include/themis/base/wasm_plugin_sandbox.h`:144
- Brief: Instantiate a WASM module from its binary bytes.
- Parameters:
  - `wasm_bytes` (const std::vector< uint8_t > &): Raw .wasm binary.
  - `host_fns` (const std::vector< WasmHostFunction > &): Host functions to bind as imports.
  - `linear_memory` (uint8_t *): Pre-allocated linear memory for the module.
  - `memory_size` (size_t): Size of linear_memory in bytes.
- Return: true on success.
- Details: wasm_bytes Raw .wasm binary. host_fns Host functions to bind as imports. linear_memory Pre-allocated linear memory for the module. memory_size Size of linear_memory in bytes. true on success.

#### `~WasmRuntime()=default`
- Source: `include/themis/base/wasm_plugin_sandbox.h`:133
- Brief: n/a
- Parameters: none

### themis::modules::WasmRuntimeInjector

#### `bool available() noexcept`
- Source: `include/themis/base/wasm_runtime_injector.h`:150
- Brief: True if at least one backend is registered.
- Parameters: none

#### `void clearAll()`
- Source: `include/themis/base/wasm_runtime_injector.h`:160
- Brief: Unregister all backends (useful for testing).
- Parameters: none

#### `std::unique_ptr< IWasmRuntime > create(const std::string &runtime_name={})`
- Source: `include/themis/base/wasm_runtime_injector.h`:145
- Brief: Create a runtime instance by name.
- Parameters:
  - `runtime_name` (const std::string &): Empty string = select highest-priority registered backend.
- Return: Owning pointer to a new (un-instantiated) runtime, or nullptr if not found.
- Details: runtime_name Empty string = select highest-priority registered backend. Owning pointer to a new (un-instantiated) runtime, or nullptr if not found.

#### `void registerRuntime(WasmRuntimeDescriptor desc)`
- Source: `include/themis/base/wasm_runtime_injector.h`:137
- Brief: Register a WASM runtime backend.
- Parameters:
  - `desc` (WasmRuntimeDescriptor): n/a
- Details: Must be called before any sandbox is created. Duplicate names replace the previous registration.

#### `std::vector< std::string > registeredNames()`
- Source: `include/themis/base/wasm_runtime_injector.h`:155
- Brief: List registered runtime names, sorted by descending priority.
- Parameters: none

### themis::network::IConnectionPolicy

#### `IConnectionPolicy()=default`
- Source: `include/themis/network/iconnection_policy.h`:182
- Brief: n/a
- Parameters: none

#### `IConnectionPolicy(IConnectionPolicy &&) noexcept=default`
- Source: `include/themis/network/iconnection_policy.h`:183
- Brief: n/a
- Parameters:
  - `<unnamed>` (IConnectionPolicy &&): n/a

#### `IConnectionPolicy(const IConnectionPolicy &)=delete`
- Source: `include/themis/network/iconnection_policy.h`:53
- Brief: n/a
- Parameters:
  - `<unnamed>` (const IConnectionPolicy &): n/a

#### `uint32_t activeConnections() const =0`
- Source: `include/themis/network/iconnection_policy.h`:143
- Brief: Return the number of connections currently tracked as open.
- Parameters: none

#### `bool allowSSEEvent()=0`
- Source: `include/themis/network/iconnection_policy.h`:162
- Brief: Return true iff emitting one more SSE event right now is permitted.
- Parameters: none
- Return: true when the event emission is within the allowed rate.
- Details: Uses token-bucket semantics. May consume a token — callers must not emit the event if this method returns false. true when the event emission is within the allowed rate.

#### `bool canAcceptConnection() const =0`
- Source: `include/themis/network/iconnection_policy.h`:123
- Brief: Return true iff accepting one more server connection is permitted.
- Parameters: none
- Return: true when another connection may be accepted.
- Details: Does not modify accounting state — call onConnectionAccepted() only after the connection has been admitted. true when another connection may be accepted.

#### `bool canOpenSSEConnection() const =0`
- Source: `include/themis/network/iconnection_policy.h`:89
- Brief: Return true iff accepting one more SSE subscription is permitted.
- Parameters: none
- Return: true when another SSE connection may be opened.
- Details: Does not modify accounting state — call onSSEConnectionOpened() only after the subscription has been accepted. true when another SSE connection may be opened.

#### `bool canOpenStream(uint32_t current_streams) const =0`
- Source: `include/themis/network/iconnection_policy.h`:70
- Brief: Return true iff opening one more HTTP/2 stream on the current connection is permitted.
- Parameters:
  - `current_streams` (uint32_t): Number of streams currently open on this connection.
- Return: true when another stream may be opened.
- Details: Does not modify accounting state — call onStreamOpened() only after the stream has been accepted. current_streams Number of streams currently open on this connection. true when another stream may be opened.

#### `bool isEnforced() const noexcept=0`
- Source: `include/themis/network/iconnection_policy.h`:179
- Brief: Return true when any connection-limit enforcement is active.
- Parameters: none
- Details: Implementations should return false when all limits are 0 (unlimited) so that callers may bypass the admission check on hot paths.

#### `uint32_t maxHttp2StreamsPerConnection() const noexcept=0`
- Source: `include/themis/network/iconnection_policy.h`:75
- Brief: Maximum concurrent HTTP/2 streams per connection; 0 = unlimited.
- Parameters: none

#### `uint32_t maxSSEConnections() const noexcept=0`
- Source: `include/themis/network/iconnection_policy.h`:109
- Brief: Maximum total SSE connections; 0 = unlimited.
- Parameters: none

#### `uint32_t maxSSEEventsPerSecond() const noexcept=0`
- Source: `include/themis/network/iconnection_policy.h`:167
- Brief: Maximum SSE events emitted per second; 0 = unlimited.
- Parameters: none

#### `uint32_t maxTotalConnections() const noexcept=0`
- Source: `include/themis/network/iconnection_policy.h`:148
- Brief: Maximum total simultaneous server connections; 0 = unlimited.
- Parameters: none

#### `void onConnectionAccepted()=0`
- Source: `include/themis/network/iconnection_policy.h`:130
- Brief: Notify the policy that a new connection has been accepted.
- Parameters: none
- Details: Updates total-connection accounting. Thread-safe.

#### `void onConnectionClosed()=0`
- Source: `include/themis/network/iconnection_policy.h`:138
- Brief: Notify the policy that an existing connection has been closed.
- Parameters: none
- Details: Releases the connection slot. Implementations must clamp to zero on mismatched calls. Thread-safe.

#### `void onSSEConnectionClosed()=0`
- Source: `include/themis/network/iconnection_policy.h`:104
- Brief: Notify the policy that an SSE connection has been closed.
- Parameters: none
- Details: Releases the SSE slot. Implementations must clamp to zero on mismatched calls. Thread-safe.

#### `void onSSEConnectionOpened()=0`
- Source: `include/themis/network/iconnection_policy.h`:96
- Brief: Notify the policy that an SSE connection has been opened.
- Parameters: none
- Details: Updates accounting state. Thread-safe.

#### `IConnectionPolicy & operator=(IConnectionPolicy &&) noexcept=default`
- Source: `include/themis/network/iconnection_policy.h`:184
- Brief: n/a
- Parameters:
  - `<unnamed>` (IConnectionPolicy &&): n/a

#### `IConnectionPolicy & operator=(const IConnectionPolicy &)=delete`
- Source: `include/themis/network/iconnection_policy.h`:54
- Brief: n/a
- Parameters:
  - `<unnamed>` (const IConnectionPolicy &): n/a

#### `~IConnectionPolicy()=default`
- Source: `include/themis/network/iconnection_policy.h`:50
- Brief: n/a
- Parameters: none

### themis::query::IQueryLimitPolicy

#### `IQueryLimitPolicy()=default`
- Source: `include/themis/query/iquery_limit_policy.h`:139
- Brief: n/a
- Parameters: none

#### `IQueryLimitPolicy(IQueryLimitPolicy &&) noexcept=default`
- Source: `include/themis/query/iquery_limit_policy.h`:140
- Brief: n/a
- Parameters:
  - `<unnamed>` (IQueryLimitPolicy &&): n/a

#### `IQueryLimitPolicy(const IQueryLimitPolicy &)=delete`
- Source: `include/themis/query/iquery_limit_policy.h`:52
- Brief: n/a
- Parameters:
  - `<unnamed>` (const IQueryLimitPolicy &): n/a

#### `bool isComplexityAllowed(uint32_t complexity) const =0`
- Source: `include/themis/query/iquery_limit_policy.h`:82
- Brief: Return true iff a query with complexity score is permitted.
- Parameters:
  - `complexity` (uint32_t): Computed query complexity (resolver-weighted node count).
- Return: true when complexity ≤ maxGraphQLComplexity() or limit is 0.
- Details: complexity Computed query complexity (resolver-weighted node count). true when complexity ≤ maxGraphQLComplexity() or limit is 0.

#### `bool isDepthAllowed(uint32_t depth) const =0`
- Source: `include/themis/query/iquery_limit_policy.h`:65
- Brief: Return true iff a query with depth nesting levels is permitted.
- Parameters:
  - `depth` (uint32_t): Observed query AST depth (number of nested selection levels).
- Return: true when depth ≤ maxGraphQLDepth() or maxGraphQLDepth() == 0.
- Details: depth Observed query AST depth (number of nested selection levels). true when depth ≤ maxGraphQLDepth() or maxGraphQLDepth() == 0.

#### `bool isEnforced() const noexcept=0`
- Source: `include/themis/query/iquery_limit_policy.h`:136
- Brief: Return true when any query-limit enforcement is active.
- Parameters: none
- Details: Implementations should return false when all limits are 0 (unlimited) so that callers may bypass the check on hot paths.

#### `bool isPayloadAllowed(uint64_t payload_bytes) const =0`
- Source: `include/themis/query/iquery_limit_policy.h`:99
- Brief: Return true iff a request body of payload_bytes is permitted.
- Parameters:
  - `payload_bytes` (uint64_t): Size of the incoming request body in bytes.
- Return: true when payload_bytes ≤ maxPayloadBytes() or limit is 0.
- Details: payload_bytes Size of the incoming request body in bytes. true when payload_bytes ≤ maxPayloadBytes() or limit is 0.

#### `bool isResultSizeAllowed(uint64_t row_count) const =0`
- Source: `include/themis/query/iquery_limit_policy.h`:119
- Brief: Return true iff returning row_count rows is permitted.
- Parameters:
  - `row_count` (uint64_t): Number of rows the executor intends to return.
- Return: true when row_count ≤ maxResultRows() or limit is 0.
- Details: Should be checked at the result-serialisation exit to enforce server-side row caps independent of client-supplied LIMIT clauses. row_count Number of rows the executor intends to return. true when row_count ≤ maxResultRows() or limit is 0.

#### `uint32_t maxGraphQLComplexity() const noexcept=0`
- Source: `include/themis/query/iquery_limit_policy.h`:87
- Brief: Maximum allowed query complexity score; 0 = unlimited.
- Parameters: none

#### `uint32_t maxGraphQLDepth() const noexcept=0`
- Source: `include/themis/query/iquery_limit_policy.h`:70
- Brief: Maximum allowed query nesting depth; 0 = unlimited.
- Parameters: none

#### `uint64_t maxPayloadBytes() const noexcept=0`
- Source: `include/themis/query/iquery_limit_policy.h`:104
- Brief: Maximum allowed request payload in bytes; 0 = unlimited.
- Parameters: none

#### `uint64_t maxResultRows() const noexcept=0`
- Source: `include/themis/query/iquery_limit_policy.h`:124
- Brief: Maximum rows returned per query execution; 0 = unlimited.
- Parameters: none

#### `IQueryLimitPolicy & operator=(IQueryLimitPolicy &&) noexcept=default`
- Source: `include/themis/query/iquery_limit_policy.h`:141
- Brief: n/a
- Parameters:
  - `<unnamed>` (IQueryLimitPolicy &&): n/a

#### `IQueryLimitPolicy & operator=(const IQueryLimitPolicy &)=delete`
- Source: `include/themis/query/iquery_limit_policy.h`:53
- Brief: n/a
- Parameters:
  - `<unnamed>` (const IQueryLimitPolicy &): n/a

#### `~IQueryLimitPolicy()=default`
- Source: `include/themis/query/iquery_limit_policy.h`:49
- Brief: n/a
- Parameters: none

### themis::rag::kg

#### `std::shared_ptr< IKnowledgeGraph > makeIKnowledgeGraph(const KnowledgeGraph &kg)`
- Source: `include/themis/rag/kg/knowledge_graph_interface.h`:30
- Brief: n/a
- Parameters:
  - `kg` (const KnowledgeGraph &): n/a

### themis::rag::kg::IKnowledgeGraph

#### `std::optional< KGNode > findNode(const std::string &node_id) const =0`
- Source: `include/themis/rag/kg/knowledge_graph_interface.h`:17
- Brief: n/a
- Parameters:
  - `node_id` (const std::string &): n/a

#### `std::optional< KGNode > findNodeByName(const std::string &text) const =0`
- Source: `include/themis/rag/kg/knowledge_graph_interface.h`:18
- Brief: n/a
- Parameters:
  - `text` (const std::string &): n/a

#### `std::unordered_set< std::string > neighbours(const std::string &start_id, size_t max_depth=1, double min_edge_weight=0.0, size_t max_nodes=4096) const =0`
- Source: `include/themis/rag/kg/knowledge_graph_interface.h`:20
- Brief: n/a
- Parameters:
  - `start_id` (const std::string &): n/a
  - `max_depth` (size_t): n/a
  - `min_edge_weight` (double): n/a
  - `max_nodes` (size_t): n/a

#### `size_t nodeCount() const =0`
- Source: `include/themis/rag/kg/knowledge_graph_interface.h`:19
- Brief: n/a
- Parameters: none

#### `std::vector< KGEdge > outEdges(const std::string &node_id) const =0`
- Source: `include/themis/rag/kg/knowledge_graph_interface.h`:25
- Brief: n/a
- Parameters:
  - `node_id` (const std::string &): n/a

#### `~IKnowledgeGraph()=default`
- Source: `include/themis/rag/kg/knowledge_graph_interface.h`:15
- Brief: n/a
- Parameters: none

### themis::ratelimit::IRateLimitPolicy

#### `IRateLimitPolicy()=default`
- Source: `include/themis/ratelimit/irate_limit_policy.h`:140
- Brief: n/a
- Parameters: none

#### `IRateLimitPolicy(IRateLimitPolicy &&) noexcept=default`
- Source: `include/themis/ratelimit/irate_limit_policy.h`:141
- Brief: n/a
- Parameters:
  - `<unnamed>` (IRateLimitPolicy &&): n/a

#### `IRateLimitPolicy(const IRateLimitPolicy &)=delete`
- Source: `include/themis/ratelimit/irate_limit_policy.h`:56
- Brief: n/a
- Parameters:
  - `<unnamed>` (const IRateLimitPolicy &): n/a

#### `bool allowRequest(const std::string &client_id="")=0`
- Source: `include/themis/ratelimit/irate_limit_policy.h`:74
- Brief: Return true iff the next incoming request is admitted under the global rate limit.
- Parameters:
  - `client_id` (const std::string &): Optional client/tenant identifier for per-client accounting; empty string = no per-client tracking.
- Return: true when the request is within the global budget.
- Details: Uses token-bucket semantics for the global budget. Callers must not process the request if this returns false. client_id Optional client/tenant identifier for per-client accounting; empty string = no per-client tracking. true when the request is within the global budget.

#### `uint64_t effectiveRPS(const std::string &client_id="") const =0`
- Source: `include/themis/ratelimit/irate_limit_policy.h`:119
- Brief: Return the current effective rate limit in requests per second.
- Parameters:
  - `client_id` (const std::string &): Client/tenant identifier; empty = global effective rate.
- Return: Effective RPS; 0 signals unlimited.
- Details: For non-adaptive policies this is identical to maxGlobalRequestsPerSecond(). For adaptive policies the value may be lower when backends are degraded. client_id Client/tenant identifier; empty = global effective rate. Effective RPS; 0 signals unlimited.

#### `bool isEnforced() const noexcept=0`
- Source: `include/themis/ratelimit/irate_limit_policy.h`:132
- Brief: Return true when any rate-limit enforcement is active.
- Parameters: none
- Details: Implementations should return false when the global limit is 0 (unlimited) and no adaptive throttling is triggered, so callers can bypass the check on hot paths.

#### `bool isThrottled(const std::string &client_id="") const =0`
- Source: `include/themis/ratelimit/irate_limit_policy.h`:108
- Brief: Return true when adaptive throttling is active for client_id.
- Parameters:
  - `client_id` (const std::string &): Client/tenant identifier; empty = global check.
- Details: Adaptive throttling is considered active when the effective rate has been reduced below the configured maximum due to observed health signals. client_id Client/tenant identifier; empty = global check.

#### `uint64_t maxGlobalRequestsPerSecond() const noexcept=0`
- Source: `include/themis/ratelimit/irate_limit_policy.h`:79
- Brief: Maximum global requests per second across all clients; 0 = unlimited.
- Parameters: none

#### `IRateLimitPolicy & operator=(IRateLimitPolicy &&) noexcept=default`
- Source: `include/themis/ratelimit/irate_limit_policy.h`:142
- Brief: n/a
- Parameters:
  - `<unnamed>` (IRateLimitPolicy &&): n/a

#### `IRateLimitPolicy & operator=(const IRateLimitPolicy &)=delete`
- Source: `include/themis/ratelimit/irate_limit_policy.h`:57
- Brief: n/a
- Parameters:
  - `<unnamed>` (const IRateLimitPolicy &): n/a

#### `void recordSample(double latency_ms, bool success, const std::string &client_id="")=0`
- Source: `include/themis/ratelimit/irate_limit_policy.h`:96
- Brief: Record a completed backend request to feed the adaptive algorithm.
- Parameters:
  - `latency_ms` (double): Round-trip latency of the backend call in milliseconds.
  - `success` (bool): true when the backend call succeeded; false on error.
  - `client_id` (const std::string &): Optional client identifier for per-client health tracking.
- Details: The implementation uses the observed latency and success/failure outcome to adjust the effective rate limit dynamically. Must be called after every dispatched backend call regardless of outcome. latency_ms Round-trip latency of the backend call in milliseconds. success true when the backend call succeeded; false on error. client_id Optional client identifier for per-client health tracking.

#### `uint64_t totalRejectedRequests() const noexcept=0`
- Source: `include/themis/ratelimit/irate_limit_policy.h`:137
- Brief: Return the total number of requests rejected due to rate limiting.
- Parameters: none

#### `~IRateLimitPolicy()=default`
- Source: `include/themis/ratelimit/irate_limit_policy.h`:53
- Brief: n/a
- Parameters: none

### themis::search

#### `std::unique_ptr< ILlmReranker > createLlmReranker(const ILlmReranker::Config &cfg=ILlmReranker::Config::defaults())`
- Source: `include/themis/search/llm_reranker_factory.h`:12
- Brief: n/a
- Parameters:
  - `cfg` (const ILlmReranker::Config &): n/a

#### `void registerLlmRerankerFactory(LlmRerankerFactory f)`
- Source: `include/themis/search/llm_reranker_factory.h`:11
- Brief: n/a
- Parameters:
  - `f` (LlmRerankerFactory): n/a

### themis::sharding::IShardLimitPolicy

#### `IShardLimitPolicy()=default`
- Source: `include/themis/sharding/ishard_limit_policy.h`:108
- Brief: n/a
- Parameters: none

#### `IShardLimitPolicy(IShardLimitPolicy &&) noexcept=default`
- Source: `include/themis/sharding/ishard_limit_policy.h`:109
- Brief: n/a
- Parameters:
  - `<unnamed>` (IShardLimitPolicy &&): n/a

#### `IShardLimitPolicy(const IShardLimitPolicy &)=delete`
- Source: `include/themis/sharding/ishard_limit_policy.h`:46
- Brief: n/a
- Parameters:
  - `<unnamed>` (const IShardLimitPolicy &): n/a

#### `int activeNodeCount() const =0`
- Source: `include/themis/sharding/ishard_limit_policy.h`:97
- Brief: Return the number of nodes currently tracked as active.
- Parameters: none
- Return: Non-negative active node count.
- Details: Non-negative active node count.

#### `bool canExpand(int requested_total) const =0`
- Source: `include/themis/sharding/ishard_limit_policy.h`:70
- Brief: Return true iff the cluster is permitted to reach requested_total nodes.
- Parameters:
  - `requested_total` (int): Intended total cluster size after the pending join.
- Return: true when the expansion is permitted under the current policy.
- Details: The policy may consult activeNodeCount() to make the admission decision. Does not modify accounting state — call onNodeAdded() only after the node has actually joined. requested_total Intended total cluster size after the pending join. true when the expansion is permitted under the current policy.

#### `bool isShardingEnabled() const noexcept=0`
- Source: `include/themis/sharding/ishard_limit_policy.h`:105
- Brief: Return true when this policy permits multi-node sharding.
- Parameters: none
- Details: Implementations should return false when maxNodes() == 1 (single-node Community edition) or when sharding has been administratively disabled.

#### `int maxNodes() const noexcept=0`
- Source: `include/themis/sharding/ishard_limit_policy.h`:58
- Brief: Declared maximum number of shard nodes this policy allows.
- Parameters: none
- Return: Maximum node count; -1 signals unlimited (only valid for Hyperscaler edition binaries where SHARDING_MAX_NODES == -1).
- Details: This value is validated against the compile-time ceiling by EditionManager::installShardPolicy before the policy is accepted. Maximum node count; -1 signals unlimited (only valid for Hyperscaler edition binaries where SHARDING_MAX_NODES == -1).

#### `void onNodeAdded(const std::string &shard_id)=0`
- Source: `include/themis/sharding/ishard_limit_policy.h`:80
- Brief: Notify the policy that a shard node with shard_id has joined.
- Parameters:
  - `shard_id` (const std::string &): Opaque, unique shard identifier (non-empty string).
- Details: Updates internal accounting. Implementations must be safe to call concurrently from multiple threads. shard_id Opaque, unique shard identifier (non-empty string).

#### `void onNodeRemoved(const std::string &shard_id)=0`
- Source: `include/themis/sharding/ishard_limit_policy.h`:90
- Brief: Notify the policy that the shard node shard_id has left.
- Parameters:
  - `shard_id` (const std::string &): Opaque shard identifier previously passed to onNodeAdded().
- Details: Updates internal accounting. Implementations must clamp to zero on mismatched calls to prevent underflow. shard_id Opaque shard identifier previously passed to onNodeAdded().

#### `IShardLimitPolicy & operator=(IShardLimitPolicy &&) noexcept=default`
- Source: `include/themis/sharding/ishard_limit_policy.h`:110
- Brief: n/a
- Parameters:
  - `<unnamed>` (IShardLimitPolicy &&): n/a

#### `IShardLimitPolicy & operator=(const IShardLimitPolicy &)=delete`
- Source: `include/themis/sharding/ishard_limit_policy.h`:47
- Brief: n/a
- Parameters:
  - `<unnamed>` (const IShardLimitPolicy &): n/a

#### `~IShardLimitPolicy()=default`
- Source: `include/themis/sharding/ishard_limit_policy.h`:43
- Brief: n/a
- Parameters: none

### themis::storage::IStorageOpsPolicy

#### `IStorageOpsPolicy()=default`
- Source: `include/themis/storage/istorage_ops_policy.h`:170
- Brief: n/a
- Parameters: none

#### `IStorageOpsPolicy(IStorageOpsPolicy &&) noexcept=default`
- Source: `include/themis/storage/istorage_ops_policy.h`:171
- Brief: n/a
- Parameters:
  - `<unnamed>` (IStorageOpsPolicy &&): n/a

#### `IStorageOpsPolicy(const IStorageOpsPolicy &)=delete`
- Source: `include/themis/storage/istorage_ops_policy.h`:52
- Brief: n/a
- Parameters:
  - `<unnamed>` (const IStorageOpsPolicy &): n/a

#### `int32_t activeJobCount() const =0`
- Source: `include/themis/storage/istorage_ops_policy.h`:91
- Brief: Return the number of background jobs currently tracked as running.
- Parameters: none

#### `bool allowCompactionBytes(uint64_t bytes)=0`
- Source: `include/themis/storage/istorage_ops_policy.h`:112
- Brief: Return true iff writing bytes to the compaction engine right now is within the allowed throughput budget.
- Parameters:
  - `bytes` (uint64_t): Bytes the compaction engine intends to write in this step.
- Return: true when the write is within the allowed rate.
- Details: Uses token-bucket semantics. May consume tokens — callers must throttle when this returns false. bytes Bytes the compaction engine intends to write in this step. true when the write is within the allowed rate.

#### `bool canScheduleJob() const =0`
- Source: `include/themis/storage/istorage_ops_policy.h`:67
- Brief: Return true iff scheduling one more background job is permitted.
- Parameters: none
- Return: true when another background job slot is available.
- Details: Does not modify accounting state — call onJobStarted() only after the job has been submitted to the job scheduler. true when another background job slot is available.

#### `bool canStartSnapshot() const =0`
- Source: `include/themis/storage/istorage_ops_policy.h`:131
- Brief: Return true iff starting one more snapshot operation is permitted.
- Parameters: none
- Return: true when another snapshot slot is available.
- Details: Does not modify accounting state — call onSnapshotStarted() only after the snapshot has been initiated. true when another snapshot slot is available.

#### `bool isEnforced() const noexcept=0`
- Source: `include/themis/storage/istorage_ops_policy.h`:167
- Brief: Return true when any storage-ops limit enforcement is active.
- Parameters: none
- Details: Implementations should return false when all limits are unlimited (-1 or 0) so that callers may bypass the check on hot paths.

#### `int32_t maxBackgroundJobs() const noexcept=0`
- Source: `include/themis/storage/istorage_ops_policy.h`:96
- Brief: Maximum concurrent background jobs; -1 = unlimited.
- Parameters: none

#### `uint64_t maxCompactionBytesPerSec() const noexcept=0`
- Source: `include/themis/storage/istorage_ops_policy.h`:117
- Brief: Maximum compaction I/O throughput in bytes per second; 0 = unlimited.
- Parameters: none

#### `int32_t maxConcurrentSnapshots() const noexcept=0`
- Source: `include/themis/storage/istorage_ops_policy.h`:155
- Brief: Maximum concurrent snapshot operations; -1 = unlimited.
- Parameters: none

#### `void onJobFinished(const std::string &job_id)=0`
- Source: `include/themis/storage/istorage_ops_policy.h`:86
- Brief: Notify the policy that a background job has finished.
- Parameters:
  - `job_id` (const std::string &): Job identifier previously passed to onJobStarted().
- Details: Releases the job slot. Implementations must clamp to zero on mismatched calls. Thread-safe. job_id Job identifier previously passed to onJobStarted().

#### `void onJobStarted(const std::string &job_id)=0`
- Source: `include/themis/storage/istorage_ops_policy.h`:76
- Brief: Notify the policy that a background job has started.
- Parameters:
  - `job_id` (const std::string &): Opaque identifier for the job (non-empty string).
- Details: Updates concurrent-job accounting. Thread-safe. job_id Opaque identifier for the job (non-empty string).

#### `void onSnapshotFinished(const std::string &snapshot_id)=0`
- Source: `include/themis/storage/istorage_ops_policy.h`:150
- Brief: Notify the policy that a snapshot operation has completed or failed.
- Parameters:
  - `snapshot_id` (const std::string &): Snapshot identifier previously passed to onSnapshotStarted().
- Details: Releases the snapshot slot. Implementations must clamp to zero on mismatched calls. Thread-safe. snapshot_id Snapshot identifier previously passed to onSnapshotStarted().

#### `void onSnapshotStarted(const std::string &snapshot_id)=0`
- Source: `include/themis/storage/istorage_ops_policy.h`:140
- Brief: Notify the policy that a snapshot operation has started.
- Parameters:
  - `snapshot_id` (const std::string &): Opaque snapshot identifier (non-empty string).
- Details: Updates snapshot accounting. Thread-safe. snapshot_id Opaque snapshot identifier (non-empty string).

#### `IStorageOpsPolicy & operator=(IStorageOpsPolicy &&) noexcept=default`
- Source: `include/themis/storage/istorage_ops_policy.h`:172
- Brief: n/a
- Parameters:
  - `<unnamed>` (IStorageOpsPolicy &&): n/a

#### `IStorageOpsPolicy & operator=(const IStorageOpsPolicy &)=delete`
- Source: `include/themis/storage/istorage_ops_policy.h`:53
- Brief: n/a
- Parameters:
  - `<unnamed>` (const IStorageOpsPolicy &): n/a

#### `~IStorageOpsPolicy()=default`
- Source: `include/themis/storage/istorage_ops_policy.h`:49
- Brief: n/a
- Parameters: none

### themis::tenant::ITenantQuotaPolicy

#### `ITenantQuotaPolicy()=default`
- Source: `include/themis/tenant/itenant_quota_policy.h`:178
- Brief: n/a
- Parameters: none

#### `ITenantQuotaPolicy(ITenantQuotaPolicy &&) noexcept=default`
- Source: `include/themis/tenant/itenant_quota_policy.h`:179
- Brief: n/a
- Parameters:
  - `<unnamed>` (ITenantQuotaPolicy &&): n/a

#### `ITenantQuotaPolicy(const ITenantQuotaPolicy &)=delete`
- Source: `include/themis/tenant/itenant_quota_policy.h`:52
- Brief: n/a
- Parameters:
  - `<unnamed>` (const ITenantQuotaPolicy &): n/a

#### `bool allowRequest(const std::string &tenant_id)=0`
- Source: `include/themis/tenant/itenant_quota_policy.h`:158
- Brief: Return true iff tenant_id is allowed to send one more request now.
- Parameters:
  - `tenant_id` (const std::string &): Tenant whose rate limit is checked.
- Details: Uses token-bucket semantics internally. May consume a token — callers must not issue the request if this method returns false. tenant_id Tenant whose rate limit is checked.

#### `bool canAddCollection(const std::string &tenant_id) const =0`
- Source: `include/themis/tenant/itenant_quota_policy.h`:101
- Brief: Return true iff tenant_id may create one more collection.
- Parameters:
  - `tenant_id` (const std::string &): Tenant whose quota is checked.
- Details: tenant_id Tenant whose quota is checked.

#### `bool canAddDocuments(const std::string &tenant_id, uint64_t count) const =0`
- Source: `include/themis/tenant/itenant_quota_policy.h`:88
- Brief: Return true iff tenant_id may create count more documents.
- Parameters:
  - `tenant_id` (const std::string &): Tenant whose quota is checked.
  - `count` (uint64_t): Number of documents the caller intends to insert.
- Details: tenant_id Tenant whose quota is checked. count Number of documents the caller intends to insert.

#### `bool canAllocateStorage(const std::string &tenant_id, uint64_t additional_bytes) const =0`
- Source: `include/themis/tenant/itenant_quota_policy.h`:68
- Brief: Return true iff tenant_id may store additional_bytes more bytes.
- Parameters:
  - `tenant_id` (const std::string &): Tenant whose quota is checked.
  - `additional_bytes` (uint64_t): Bytes the operation intends to add.
- Details: Does not modify accounting state. Returns true unconditionally when maxStorageBytes() == 0 (unlimited). tenant_id Tenant whose quota is checked. additional_bytes Bytes the operation intends to add.

#### `bool canStartQuery(const std::string &tenant_id) const =0`
- Source: `include/themis/tenant/itenant_quota_policy.h`:120
- Brief: Return true iff tenant_id may dispatch one more concurrent query.
- Parameters:
  - `tenant_id` (const std::string &): Tenant whose slot availability is checked.
- Details: Does not modify accounting state — call onQueryStarted() only after the query has actually been admitted. tenant_id Tenant whose slot availability is checked.

#### `bool isEnforced() const noexcept=0`
- Source: `include/themis/tenant/itenant_quota_policy.h`:175
- Brief: Return true when per-tenant quota enforcement is active.
- Parameters: none
- Details: Implementations should return false when every limit is 0 (unlimited) and enforcement is a no-op, so callers can skip the check on hot paths.

#### `uint32_t maxCollections() const noexcept=0`
- Source: `include/themis/tenant/itenant_quota_policy.h`:106
- Brief: Maximum collection count per tenant; 0 = unlimited.
- Parameters: none

#### `uint32_t maxConcurrentQueries() const noexcept=0`
- Source: `include/themis/tenant/itenant_quota_policy.h`:144
- Brief: Maximum concurrent queries per tenant; 0 = unlimited.
- Parameters: none

#### `uint64_t maxDocuments() const noexcept=0`
- Source: `include/themis/tenant/itenant_quota_policy.h`:94
- Brief: Maximum document count per tenant; 0 = unlimited.
- Parameters: none

#### `uint32_t maxRequestsPerSecond() const noexcept=0`
- Source: `include/themis/tenant/itenant_quota_policy.h`:163
- Brief: Maximum requests per second per tenant; 0 = unlimited.
- Parameters: none

#### `uint64_t maxStorageBytes() const noexcept=0`
- Source: `include/themis/tenant/itenant_quota_policy.h`:76
- Brief: Maximum storage bytes allowed per tenant by this policy.
- Parameters: none
- Return: Byte limit; 0 signals unlimited.
- Details: Byte limit; 0 signals unlimited.

#### `void onQueryFinished(const std::string &tenant_id)=0`
- Source: `include/themis/tenant/itenant_quota_policy.h`:139
- Brief: Notify the policy that tenant_id's query has finished.
- Parameters:
  - `tenant_id` (const std::string &): Tenant identifier.
- Details: Releases the concurrent-query slot. Implementations must clamp to zero on mismatched calls. Thread-safe. tenant_id Tenant identifier.

#### `void onQueryStarted(const std::string &tenant_id)=0`
- Source: `include/themis/tenant/itenant_quota_policy.h`:129
- Brief: Notify the policy that tenant_id started a query.
- Parameters:
  - `tenant_id` (const std::string &): Tenant identifier.
- Details: Updates concurrent-query accounting. Thread-safe. tenant_id Tenant identifier.

#### `ITenantQuotaPolicy & operator=(ITenantQuotaPolicy &&) noexcept=default`
- Source: `include/themis/tenant/itenant_quota_policy.h`:180
- Brief: n/a
- Parameters:
  - `<unnamed>` (ITenantQuotaPolicy &&): n/a

#### `ITenantQuotaPolicy & operator=(const ITenantQuotaPolicy &)=delete`
- Source: `include/themis/tenant/itenant_quota_policy.h`:53
- Brief: n/a
- Parameters:
  - `<unnamed>` (const ITenantQuotaPolicy &): n/a

#### `~ITenantQuotaPolicy()=default`
- Source: `include/themis/tenant/itenant_quota_policy.h`:49
- Brief: n/a
- Parameters: none

### themis::wire

#### `void setWireAqlExecFn(WireAqlExecFn fn)`
- Source: `src/themis/wire_protocol_server.cpp`:88
- Brief: Register the AQL execution bridge for the Protobuf wire protocol.
- Parameters:
  - `fn` (WireAqlExecFn): Input parameter.
- Details: Set Wire Aql Exec Fn. Thread-safe. Pass nullptr to clear. Registered once at server startup. DeprecatedUse WireProtocolServer::setAqlQueryFn() for per-server wiring or WireProtocolSession::setQueryAqlFn() for the process-global Protobuf fallback hook. fn Input parameter. Calls: lock(), std::move().

#### `void setWireCursorCloseFn(WireCursorCloseFn fn)`
- Source: `src/themis/wire_protocol_server.cpp`:106
- Brief: Register the cursor close bridge for the Protobuf wire protocol. Thread-safe. Pass nullptr to clear.
- Parameters:
  - `fn` (WireCursorCloseFn): Input parameter.
- Details: Set Wire Cursor Close Fn. DeprecatedUse WireProtocolServer::setCursorCloseFn() instead. fn Input parameter. Calls: lock(), std::move().

#### `void setWireCursorNextFn(WireCursorNextFn fn)`
- Source: `src/themis/wire_protocol_server.cpp`:97
- Brief: Register the cursor next-page bridge for the Protobuf wire protocol. Thread-safe. Pass nullptr to clear.
- Parameters:
  - `fn` (WireCursorNextFn): Input parameter.
- Details: Set Wire Cursor Next Fn. DeprecatedUse WireProtocolServer::setCursorNextFn() instead. fn Input parameter. Calls: lock(), std::move().

#### `void setWireGeoQueryFn(WireGeoQueryFn fn)`
- Source: `src/themis/wire_protocol_server.cpp`:115
- Brief: Register the geospatial query bridge for the Protobuf wire protocol. Thread-safe. Pass nullptr to clear.
- Parameters:
  - `fn` (WireGeoQueryFn): Input parameter.
- Details: Set Wire Geo Query Fn. DeprecatedUse WireProtocolServer::setGeoQueryFn() for per-server wiring or WireProtocolSession::setGeoQueryFn() for the process-global Protobuf fallback hook. fn Input parameter. Calls: lock(), std::move().

#### `void setWireGraphTraversalFn(WireGraphTraversalFn fn)`
- Source: `src/themis/wire_protocol_server.cpp`:133
- Brief: Register the graph traversal bridge for the Protobuf wire protocol. Thread-safe. Pass nullptr to clear.
- Parameters:
  - `fn` (WireGraphTraversalFn): Input parameter.
- Details: Set Wire Graph Traversal Fn. DeprecatedUse WireProtocolServer::setGraphTraverseFn() for per-server wiring or WireProtocolSession::setGraphTraverseFn() for the process-global Protobuf fallback hook. fn Input parameter. Calls: lock(), std::move().

#### `void setWireTSQueryFn(WireTSQueryFn fn)`
- Source: `src/themis/wire_protocol_server.cpp`:124
- Brief: Register the time-series query bridge for the Protobuf wire protocol. Thread-safe. Pass nullptr to clear.
- Parameters:
  - `fn` (WireTSQueryFn): Input parameter.
- Details: Set Wire TSQuery Fn. DeprecatedUse WireProtocolServer::setTimeseriesQueryFn() for per-server wiring or WireProtocolSession::setTimeseriesQueryFn() for the process-global Protobuf fallback hook. fn Input parameter. Calls: lock(), std::move().

### themis::wire::MessageDispatcher

#### `void dispatch(WireProtocolSession &session, OpCode opcode, const std::vector< uint8_t > &payload)`
- Source: `include/themis/network/wire_protocol_server.hpp`:646
- Brief: Dispatch.
- Parameters:
  - `session` (WireProtocolSession &): Input/output parameter.
  - `opcode` (OpCode): Input parameter.
  - `payload` (const std::vector< uint8_t > &): Input parameter.
- Details: session Input/output parameter. opcode Input parameter. payload Input parameter. Calls: find(), end(), second().

#### `void register_handler(OpCode opcode, handler_fn handler)`
- Source: `include/themis/network/wire_protocol_server.hpp`:645
- Brief: Register handler.
- Parameters:
  - `opcode` (OpCode): Input parameter.
  - `handler` (handler_fn): Input parameter.
- Details: opcode Input parameter. handler Input parameter. Calls: std::move().

### themis::wire::V2FrameHeader

#### `V2FrameType get_type() const noexcept`
- Source: `include/themis/network/wire_protocol_v2.hpp`:100
- Brief: n/a
- Parameters: none

#### `bool has_flag(V2FrameFlags f) const noexcept`
- Source: `include/themis/network/wire_protocol_v2.hpp`:104
- Brief: n/a
- Parameters:
  - `f` (V2FrameFlags): n/a

#### `bool is_valid() const noexcept`
- Source: `include/themis/network/wire_protocol_v2.hpp`:96
- Brief: n/a
- Parameters: none

### themis::wire::V2Server

#### `V2Server(const V2ConnectionConfig &config)`
- Source: `include/themis/network/wire_protocol_v2.hpp`:298
- Brief: n/a
- Parameters:
  - `config` (const V2ConnectionConfig &): n/a

#### `V2Server(const V2Server &)=delete`
- Source: `include/themis/network/wire_protocol_v2.hpp`:302
- Brief: n/a
- Parameters:
  - `<unnamed>` (const V2Server &): n/a

#### `size_t active_connections() const`
- Source: `include/themis/network/wire_protocol_v2.hpp`:339
- Brief: n/a
- Parameters: none

#### `bool is_running() const`
- Source: `include/themis/network/wire_protocol_v2.hpp`:313
- Brief: n/a
- Parameters: none

#### `V2Server & operator=(const V2Server &)=delete`
- Source: `include/themis/network/wire_protocol_v2.hpp`:303
- Brief: n/a
- Parameters:
  - `<unnamed>` (const V2Server &): n/a

#### `bool push_to_client(const std::string &connection_id, uint32_t associated_sid, const std::unordered_map< std::string, std::string > &headers, const std::vector< uint8_t > &data)`
- Source: `include/themis/network/wire_protocol_v2.hpp`:332
- Brief: Push data to a connected client on a new server-initiated stream.
- Parameters:
  - `connection_id` (const std::string &): Identifies the target V2Session.
  - `associated_sid` (uint32_t): Client stream that triggered the push.
  - `headers` (const std::unordered_map< std::string, std::string > &): Headers for the pushed resource.
  - `data` (const std::vector< uint8_t > &): Payload to push.
- Return: true if the push was enqueued successfully.
- Details: connection_id Identifies the target V2Session. associated_sid Client stream that triggered the push. headers Headers for the pushed resource. data Payload to push. true if the push was enqueued successfully.

#### `void set_data_handler(V2DataHandler handler)`
- Source: `include/themis/network/wire_protocol_v2.hpp`:317
- Brief: n/a
- Parameters:
  - `handler` (V2DataHandler): n/a

#### `void set_headers_handler(V2HeadersHandler handler)`
- Source: `include/themis/network/wire_protocol_v2.hpp`:318
- Brief: n/a
- Parameters:
  - `handler` (V2HeadersHandler): n/a

#### `void set_rst_stream_handler(V2RstStreamHandler handler)`
- Source: `include/themis/network/wire_protocol_v2.hpp`:319
- Brief: n/a
- Parameters:
  - `handler` (V2RstStreamHandler): n/a

#### `void start()`
- Source: `include/themis/network/wire_protocol_v2.hpp`:308
- Brief: Start accepting connections. Non-blocking; spawns I/O threads.
- Parameters: none

#### `void stop()`
- Source: `include/themis/network/wire_protocol_v2.hpp`:311
- Brief: Stop accepting connections and wait for in-flight work to finish.
- Parameters: none

#### `uint64_t total_frames_received() const`
- Source: `include/themis/network/wire_protocol_v2.hpp`:342
- Brief: n/a
- Parameters: none

#### `uint64_t total_frames_sent() const`
- Source: `include/themis/network/wire_protocol_v2.hpp`:341
- Brief: n/a
- Parameters: none

#### `uint64_t total_streams_opened() const`
- Source: `include/themis/network/wire_protocol_v2.hpp`:340
- Brief: n/a
- Parameters: none

#### `~V2Server()`
- Source: `include/themis/network/wire_protocol_v2.hpp`:299
- Brief: n/a
- Parameters: none

### themis::wire::V2Session

#### `uint64_t bytes_received() const =0`
- Source: `include/themis/network/wire_protocol_v2.hpp`:270
- Brief: n/a
- Parameters: none

#### `uint64_t bytes_sent() const =0`
- Source: `include/themis/network/wire_protocol_v2.hpp`:271
- Brief: n/a
- Parameters: none

#### `const std::string & connection_id() const =0`
- Source: `include/themis/network/wire_protocol_v2.hpp`:224
- Brief: Unique connection identifier (UUID or sequential number).
- Parameters: none

#### `uint64_t frames_received() const =0`
- Source: `include/themis/network/wire_protocol_v2.hpp`:268
- Brief: n/a
- Parameters: none

#### `uint64_t frames_sent() const =0`
- Source: `include/themis/network/wire_protocol_v2.hpp`:269
- Brief: n/a
- Parameters: none

#### `void go_away(uint32_t last_stream_id, uint32_t error_code=0)=0`
- Source: `include/themis/network/wire_protocol_v2.hpp`:240
- Brief: Send a GOAWAY and close the connection after in-flight frames.
- Parameters:
  - `last_stream_id` (uint32_t): n/a
  - `error_code` (uint32_t): n/a

#### `size_t open_stream_count() const =0`
- Source: `include/themis/network/wire_protocol_v2.hpp`:243
- Brief: Number of currently open streams on this connection.
- Parameters: none

#### `uint32_t push_promise(uint32_t associated_stream_id, const std::unordered_map< std::string, std::string > &headers)=0`
- Source: `include/themis/network/wire_protocol_v2.hpp`:232
- Brief: Initiate a server-push stream (server → client unsolicited).
- Parameters:
  - `associated_stream_id` (uint32_t): n/a
  - `headers` (const std::unordered_map< std::string, std::string > &): n/a

#### `void reset_stream(uint32_t stream_id, uint32_t error_code=0)=0`
- Source: `include/themis/network/wire_protocol_v2.hpp`:237
- Brief: Gracefully close a stream with RST_STREAM.
- Parameters:
  - `stream_id` (uint32_t): n/a
  - `error_code` (uint32_t): n/a

#### `void send_data(uint32_t stream_id, const std::vector< uint8_t > &data, bool end_stream=true)=0`
- Source: `include/themis/network/wire_protocol_v2.hpp`:227
- Brief: Send a DATA frame on stream_id.
- Parameters:
  - `stream_id` (uint32_t): n/a
  - `data` (const std::vector< uint8_t > &): n/a
  - `end_stream` (bool): n/a

#### `int32_t send_window(uint32_t stream_id) const =0`
- Source: `include/themis/network/wire_protocol_v2.hpp`:246
- Brief: Current send-window size for stream_id (bytes).
- Parameters:
  - `stream_id` (uint32_t): n/a

#### `void set_stream_priority(uint32_t stream_id, uint32_t dependency, uint8_t weight, bool exclusive=false)=0`
- Source: `include/themis/network/wire_protocol_v2.hpp`:262
- Brief: Send a PRIORITY frame to inform the remote side of stream ordering.
- Parameters:
  - `stream_id` (uint32_t): Stream to reprioritise.
  - `dependency` (uint32_t): Parent stream ID this stream depends on (0 = root).
  - `weight` (uint8_t): Priority weight 0–255.
  - `exclusive` (bool): When true the stream exclusively depends on dependency.
- Details: The PRIORITY frame payload follows RFC 7540 §6.3: E (1 bit) – exclusive dependency flag Stream Dependency (31 bits) – ID of the parent stream (0 = root) Weight (8 bits) – priority weight 0–255 (maps to HTTP/2 weight 1–256) stream_id Stream to reprioritise. dependency Parent stream ID this stream depends on (0 = root). weight Priority weight 0–255. exclusive When true the stream exclusively depends on dependency.

#### `void update_connection_window(uint32_t increment)=0`
- Source: `include/themis/network/wire_protocol_v2.hpp`:249
- Brief: Update the connection-level send-window (WINDOW_UPDATE).
- Parameters:
  - `increment` (uint32_t): n/a

#### `~V2Session()=default`
- Source: `include/themis/network/wire_protocol_v2.hpp`:221
- Brief: n/a
- Parameters: none

### themis::wire::V2Stream

#### `bool is_open() const noexcept`
- Source: `include/themis/network/wire_protocol_v2.hpp`:154
- Brief: n/a
- Parameters: none

### themis::wire::WireFrameHeader

#### `OpCode get_opcode() const`
- Source: `include/themis/network/wire_protocol_server.hpp`:182
- Brief: n/a
- Parameters: none

#### `bool has_flag(MessageFlags flag) const`
- Source: `include/themis/network/wire_protocol_server.hpp`:186
- Brief: n/a
- Parameters:
  - `flag` (MessageFlags): n/a

#### `bool is_valid() const`
- Source: `include/themis/network/wire_protocol_server.hpp`:178
- Brief: n/a
- Parameters: none

### themis::wire::WireProtocolServer

#### `WireProtocolServer(boost::asio::io_context &io_context, uint16_t port)`
- Source: `include/themis/network/wire_protocol_server.hpp`:570
- Brief: Minimal constructor (no engine injection; all advanced handlers return redirect errors).
- Parameters:
  - `io_context` (boost::asio::io_context &): n/a
  - `port` (uint16_t): n/a

#### `WireProtocolServer(boost::asio::io_context &io_context, uint16_t port, WireEngineConfig engines)`
- Source: `include/themis/network/wire_protocol_server.hpp`:579
- Brief: Engine-injected constructor.
- Parameters:
  - `io_context` (boost::asio::io_context &): Boost.Asio I/O context for async network operations.
  - `port` (uint16_t): TCP port to listen on.
  - `engines` (WireEngineConfig): Engine references to wire into session handlers.
- Details: io_context Boost.Asio I/O context for async network operations. port TCP port to listen on. engines Engine references to wire into session handlers.

#### `size_t active_sessions() const`
- Source: `include/themis/network/wire_protocol_server.hpp`:588
- Brief: n/a
- Parameters: none

#### `void async_accept()`
- Source: `include/themis/network/wire_protocol_server.hpp`:612
- Brief: Async accept.
- Parameters: none
- Details: Calls: std::move(), lock(), bindSessionCallbacksLocked(), handle_accept().

#### `void bindSessionCallbacksLocked(WireProtocolSession &session) const`
- Source: `include/themis/network/wire_protocol_server.hpp`:614
- Brief: n/a
- Parameters:
  - `session` (WireProtocolSession &): n/a

#### `void handle_accept(std::shared_ptr< WireProtocolSession > session, const boost::system::error_code &error)`
- Source: `include/themis/network/wire_protocol_server.hpp`:613
- Brief: Handle accept.
- Parameters:
  - `session` (std::shared_ptr< WireProtocolSession >): Input parameter.
  - `error` (const boost::system::error_code &): Input parameter.
- Details: session Input parameter. error Input parameter. Calls: set_disconnect_callback(), lock(), erase(), session_id(), start(), close(), async_accept().

#### `void setAqlQueryFn(WireProtocolSession::AqlQueryFn fn)`
- Source: `include/themis/network/wire_protocol_server.hpp`:599
- Brief: Inject AQL query executor into all new sessions.
- Parameters:
  - `fn` (WireProtocolSession::AqlQueryFn): Input parameter.
- Details: Set Aql Query Fn. fn Input parameter. Calls: lock(), std::move().

#### `void setCursorCloseFn(WireProtocolSession::CursorCloseFn fn)`
- Source: `include/themis/network/wire_protocol_server.hpp`:603
- Brief: Inject cursor-close executor into all new sessions.
- Parameters:
  - `fn` (WireProtocolSession::CursorCloseFn): Input parameter.
- Details: Set Cursor Close Fn. fn Input parameter. Calls: lock(), std::move().

#### `void setCursorNextFn(WireProtocolSession::CursorNextFn fn)`
- Source: `include/themis/network/wire_protocol_server.hpp`:601
- Brief: Inject cursor-next executor into all new sessions.
- Parameters:
  - `fn` (WireProtocolSession::CursorNextFn): Input parameter.
- Details: Set Cursor Next Fn. fn Input parameter. Calls: lock(), std::move().

#### `void setGeoQueryFn(WireProtocolSession::GeoQueryFn fn)`
- Source: `include/themis/network/wire_protocol_server.hpp`:605
- Brief: Inject geospatial query executor into all new sessions.
- Parameters:
  - `fn` (WireProtocolSession::GeoQueryFn): Input parameter.
- Details: Set Geo Query Fn. fn Input parameter. Calls: lock(), std::move().

#### `void setGraphTraverseFn(WireProtocolSession::GraphTraverseFn fn)`
- Source: `include/themis/network/wire_protocol_server.hpp`:609
- Brief: Inject graph traversal executor into all new sessions.
- Parameters:
  - `fn` (WireProtocolSession::GraphTraverseFn): Input parameter.
- Details: Set Graph Traverse Fn. fn Input parameter. Calls: lock(), std::move().

#### `void setTimeseriesQueryFn(WireProtocolSession::TimeseriesQueryFn fn)`
- Source: `include/themis/network/wire_protocol_server.hpp`:607
- Brief: Inject time-series query executor into all new sessions.
- Parameters:
  - `fn` (WireProtocolSession::TimeseriesQueryFn): Input parameter.
- Details: Set Timeseries Query Fn. fn Input parameter. Calls: lock(), std::move().

#### `void start()`
- Source: `include/themis/network/wire_protocol_server.hpp`:584
- Brief: Start.
- Parameters: none
- Details: Calls: lock(), collectProtobufBootstrapState(), network::wire_bootstrap::validateRequiredBackends(), async_accept().

#### `void stop()`
- Source: `include/themis/network/wire_protocol_server.hpp`:585
- Brief: Stop.
- Parameters: none
- Details: Calls: lock(), push_back(), clear(), close().

#### `uint64_t total_connections() const`
- Source: `include/themis/network/wire_protocol_server.hpp`:589
- Brief: n/a
- Parameters: none

#### `uint64_t total_messages() const`
- Source: `include/themis/network/wire_protocol_server.hpp`:590
- Brief: n/a
- Parameters: none

#### `~WireProtocolServer()`
- Source: `include/themis/network/wire_protocol_server.hpp`:582
- Brief: n/a
- Parameters: none

### themis::wire::WireProtocolSession

#### `WireProtocolSession(socket_t socket)`
- Source: `include/themis/network/wire_protocol_server.hpp`:390
- Brief: n/a
- Parameters:
  - `socket` (socket_t): n/a

#### `void async_read_header()`
- Source: `include/themis/network/wire_protocol_server.hpp`:483
- Brief: - async read pipeline -
- Parameters: none
- Details: Calls: shared_from_this(), resize(), net::async_read(), net::buffer(), message(), close(), deserializeHeader(), data().

#### `void async_read_payload(const WireFrameHeader &header)`
- Source: `include/themis/network/wire_protocol_server.hpp`:484
- Brief: Async read payload.
- Parameters:
  - `header` (const WireFrameHeader &): Input parameter.
- Details: header Input parameter. Calls: shared_from_this(), has_flag(), get_opcode(), defined(), send_error(), handle_ping(), handle_close(), async_read_header().

#### `void async_write_response(OpCode opcode, const google::protobuf::Message &message)`
- Source: `include/themis/network/wire_protocol_server.hpp`:485
- Brief: Async write response.
- Parameters:
  - `opcode` (OpCode): Input parameter.
  - `message` (const google::protobuf::Message &): Input parameter.
- Details: opcode Input parameter. message Input parameter. Calls: defined(), SerializeToString(), send_error(), size(), serializeHeader(), crc32Compute(), data(), htonl().

#### `void close(const std::string &reason="")`
- Source: `include/themis/network/wire_protocol_server.hpp`:394
- Brief: Close.
- Parameters:
  - `reason` (const std::string &): n/a
- Details: param Input parameter. Calls: void(), lock(), is_open(), shutdown(), disconnect_callback().

#### `std::vector< uint8_t > compress_lz4(const std::vector< uint8_t > &data)`
- Source: `include/themis/network/wire_protocol_server.hpp`:518
- Brief: Compress lz4.
- Parameters:
  - `data` (const std::vector< uint8_t > &): Input parameter.
- Return: Return value.
- Details: data Input parameter. Return value. Implements compress_lz4 without additional internal calls.

#### `uint32_t compute_checksum(const WireFrameHeader &header, const std::vector< uint8_t > &payload)`
- Source: `include/themis/network/wire_protocol_server.hpp`:514
- Brief: Compute checksum.
- Parameters:
  - `header` (const WireFrameHeader &): Input parameter.
  - `payload` (const std::vector< uint8_t > &): Input parameter.
- Return: Return value.
- Details: header Input parameter. payload Input parameter. Return value. Calls: serializeHeader().

#### `std::vector< uint8_t > decompress_lz4(const std::vector< uint8_t > &compressed)`
- Source: `include/themis/network/wire_protocol_server.hpp`:517
- Brief: Decompress lz4.
- Parameters:
  - `compressed` (const std::vector< uint8_t > &): Input parameter.
- Return: Return value.
- Details: compressed Input parameter. Return value. Implements decompress_lz4 without additional internal calls.

#### `void handle_auth_response(const v1::AuthResponse &req)`
- Source: `include/themis/network/wire_protocol_server.hpp`:489
- Brief: n/a
- Parameters:
  - `req` (const v1::AuthResponse &): n/a

#### `void handle_batch_get(const v1::BatchGetRequest &req)`
- Source: `include/themis/network/wire_protocol_server.hpp`:493
- Brief: n/a
- Parameters:
  - `req` (const v1::BatchGetRequest &): n/a

#### `void handle_batch_put(const v1::BatchPutRequest &req)`
- Source: `include/themis/network/wire_protocol_server.hpp`:494
- Brief: n/a
- Parameters:
  - `req` (const v1::BatchPutRequest &): n/a

#### `void handle_bpmn_query_instance(const v1::BpmnQueryInstanceRequest &req)`
- Source: `include/themis/network/wire_protocol_server.hpp`:507
- Brief: n/a
- Parameters:
  - `req` (const v1::BpmnQueryInstanceRequest &): n/a

#### `void handle_bpmn_start(const v1::BpmnStartProcessRequest &req)`
- Source: `include/themis/network/wire_protocol_server.hpp`:505
- Brief: n/a
- Parameters:
  - `req` (const v1::BpmnStartProcessRequest &): n/a

#### `void handle_bpmn_task_complete(const v1::BpmnTaskCompleteRequest &req)`
- Source: `include/themis/network/wire_protocol_server.hpp`:506
- Brief: n/a
- Parameters:
  - `req` (const v1::BpmnTaskCompleteRequest &): n/a

#### `void handle_close(const v1::CloseRequest &req)`
- Source: `include/themis/network/wire_protocol_server.hpp`:509
- Brief: n/a
- Parameters:
  - `req` (const v1::CloseRequest &): n/a

#### `void handle_cursor_close(const v1::CursorCloseRequest &req)`
- Source: `include/themis/network/wire_protocol_server.hpp`:497
- Brief: n/a
- Parameters:
  - `req` (const v1::CursorCloseRequest &): n/a

#### `void handle_cursor_next(const v1::CursorNextRequest &req)`
- Source: `include/themis/network/wire_protocol_server.hpp`:496
- Brief: n/a
- Parameters:
  - `req` (const v1::CursorNextRequest &): n/a

#### `void handle_delete(const v1::DeleteRequest &req)`
- Source: `include/themis/network/wire_protocol_server.hpp`:492
- Brief: n/a
- Parameters:
  - `req` (const v1::DeleteRequest &): n/a

#### `void handle_geo_query(const v1::GeoQueryRequest &req)`
- Source: `include/themis/network/wire_protocol_server.hpp`:503
- Brief: n/a
- Parameters:
  - `req` (const v1::GeoQueryRequest &): n/a

#### `void handle_get(const v1::GetRequest &req)`
- Source: `include/themis/network/wire_protocol_server.hpp`:490
- Brief: n/a
- Parameters:
  - `req` (const v1::GetRequest &): n/a

#### `void handle_graph_traverse(std::string_view raw_payload)`
- Source: `include/themis/network/wire_protocol_server.hpp`:502
- Brief: n/a
- Parameters:
  - `raw_payload` (std::string_view): n/a

#### `void handle_hello(const v1::HelloRequest &req)`
- Source: `include/themis/network/wire_protocol_server.hpp`:488
- Brief: n/a
- Parameters:
  - `req` (const v1::HelloRequest &): n/a

#### `void handle_ping(const v1::PingRequest &req)`
- Source: `include/themis/network/wire_protocol_server.hpp`:508
- Brief: n/a
- Parameters:
  - `req` (const v1::PingRequest &): n/a

#### `void handle_put(const v1::PutRequest &req)`
- Source: `include/themis/network/wire_protocol_server.hpp`:491
- Brief: n/a
- Parameters:
  - `req` (const v1::PutRequest &): n/a

#### `void handle_query_aql(const v1::QueryRequest &req)`
- Source: `include/themis/network/wire_protocol_server.hpp`:495
- Brief: n/a
- Parameters:
  - `req` (const v1::QueryRequest &): n/a

#### `void handle_timeseries_query(const v1::TimeSeriesQueryRequest &req)`
- Source: `include/themis/network/wire_protocol_server.hpp`:504
- Brief: n/a
- Parameters:
  - `req` (const v1::TimeSeriesQueryRequest &): n/a

#### `void handle_transaction_abort(const v1::TransactionAbortRequest &req)`
- Source: `include/themis/network/wire_protocol_server.hpp`:500
- Brief: n/a
- Parameters:
  - `req` (const v1::TransactionAbortRequest &): n/a

#### `void handle_transaction_begin(const v1::TransactionBeginRequest &req)`
- Source: `include/themis/network/wire_protocol_server.hpp`:498
- Brief: n/a
- Parameters:
  - `req` (const v1::TransactionBeginRequest &): n/a

#### `void handle_transaction_commit(const v1::TransactionCommitRequest &req)`
- Source: `include/themis/network/wire_protocol_server.hpp`:499
- Brief: n/a
- Parameters:
  - `req` (const v1::TransactionCommitRequest &): n/a

#### `void handle_vector_search(const v1::VectorSearchRequest &req)`
- Source: `include/themis/network/wire_protocol_server.hpp`:501
- Brief: n/a
- Parameters:
  - `req` (const v1::VectorSearchRequest &): n/a

#### `bool is_authenticated() const`
- Source: `include/themis/network/wire_protocol_server.hpp`:409
- Brief: n/a
- Parameters: none

#### `void send_error(uint32_t error_code, const std::string &message)`
- Source: `include/themis/network/wire_protocol_server.hpp`:512
- Brief: - utility methods -
- Parameters:
  - `error_code` (uint32_t): n/a
  - `message` (const std::string &): Input parameter.
- Details: err_code Input parameter. message Input parameter. Calls: payload(), size(), htonl(), std::memcpy(), data(), serializeHeader(), clear(), insert().

#### `void send_ok(const std::string &message="")`
- Source: `include/themis/network/wire_protocol_server.hpp`:513
- Brief: Send ok.
- Parameters:
  - `message` (const std::string &): Input parameter.
- Details: message Input parameter. Calls: size(), serializeHeader(), clear(), insert(), end(), begin(), shared_from_this(), net::async_write().

#### `const std::string & session_id() const`
- Source: `include/themis/network/wire_protocol_server.hpp`:408
- Brief: n/a
- Parameters: none

#### `void set_disconnect_callback(std::function< void(const std::string &)> callback)`
- Source: `include/themis/network/wire_protocol_server.hpp`:395
- Brief: n/a
- Parameters:
  - `callback` (std::function< void(const std::string &)>): n/a

#### `void set_engines(const WireEngineConfig *engines) noexcept`
- Source: `include/themis/network/wire_protocol_server.hpp`:404
- Brief: Inject engine references for stub #281.
- Parameters:
  - `engines` (const WireEngineConfig *): n/a
- Details: Called by WireProtocolServer immediately after session construction. The pointer is non-owning (the WireProtocolServer's engines_ field owns the WireEngineConfig).

#### `void start()`
- Source: `include/themis/network/wire_protocol_server.hpp`:393
- Brief: Start.
- Parameters: none
- Details: Calls: async_read_header().

#### `const std::string & username() const`
- Source: `include/themis/network/wire_protocol_server.hpp`:410
- Brief: n/a
- Parameters: none

#### `bool verify_checksum(const WireFrameHeader &header, const std::vector< uint8_t > &payload, uint32_t checksum)`
- Source: `include/themis/network/wire_protocol_server.hpp`:515
- Brief: Verify checksum.
- Parameters:
  - `header` (const WireFrameHeader &): Input parameter.
  - `payload` (const std::vector< uint8_t > &): Input parameter.
  - `checksum` (uint32_t): Input parameter.
- Return: True when the operation succeeds.
- Details: header Input parameter. payload Input parameter. checksum Input parameter. True when the operation succeeds. Calls: compute_checksum().

#### `~WireProtocolSession()`
- Source: `include/themis/network/wire_protocol_server.hpp`:391
- Brief: n/a
- Parameters: none

