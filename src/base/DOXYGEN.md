# BASE DOXYGEN

Author: ThemisDB Contributors
Created: 2026-09-20
Last Updated: 2026-09-20
Status: active

## Source
- This file is generated from module-scoped Doxygen XML.
- XML index: `C:\Projects\ThemisDB\.tmp\module_doxygen_global_e2e\base\xml\index.xml`
- Warnings log: `C:\Projects\ThemisDB\.tmp\module_doxygen_global_e2e\base\doxygen-warnings.log`

## Structure Metrics
- C/C++ Files (scanned): 36
- Compounds: 165
- Classes/Structs: 95
- Namespaces: 26
- File Compounds: 36

## Namespaces
- @023145272000230206346062306235166237225134174306
- @036141366375014224014126232054145310177356061344
- @061161203363020277333331362253315347362024122346
- @206061106320240224146063213166357050243027132361
- @236131045375206042320330335220314252072205327061
- @331003171355345150150012114146145072366205355070
- @363234074343254173026244301222070371262020154123
- @371050306223151315204276103022070017164015121002
- BaseErrorTaxonomy
- benchmark
- std::chrono
- testing
- themis
- themis::acceleration
- themis::modules
- themis::modules::@015203323205265137167037122135107305321372053320
- themis::modules::@072365216323054016146007371015025106361210153116
- themis::modules::@137315133046366067351076120226315346127351072327
- themis::modules::@204374062331216224203347216056172245170027156204
- themis::modules::@210135313215327224033376237157366162357255041171
- themis::modules::@333001141364354212242001001344162244364277067250
- themis::modules::BaseErrorTaxonomy
- themis::modules::ModuleSandbox
- themis::modules::ModuleSecurityVerifier
- themis::resource
- themis::storage

## Types
### Classes
- AQLJoinBench
- AQLQueryBench
- AbiCheckerFixture
- BaseEntityTest
- BaseHotPathsLoaderFixture
- BaseHotPathsReloadFixture
- BatchOperationsBench
- BestPracticeBench
- BinaryOperationsBench
- ComplexVectorBench
- DeterministicRNG
- GapAnalysisBench
- GraphExportFixture
- GraphOperationsBench
- HotReloadLoadFixture
- LLMInferencingBench
- LatencyBenchFixture
- LatencyTracker
- MockExpressionEvaluator
- MockFieldEncryption
- MockGraphIndex
- MockKeyProvider
- MockQueryEngine
- MockSecondaryIndex
- MockStorageEngine
- MockVectorIndex
- ModuleLoaderFixture
- MultiThreadBenchEnv
- ParallelityBench
- ParallelityBenchBatchWrite
- ParallelityBenchOptimized
- ParallelityBenchOptimizedConfig
- ParallelityBenchPhase1Final
- ParallelityBenchPhase2Final
- ParallelityBenchPhase2G
- ParallelityBenchPhase2G_2H
- ParallelityBenchPhase2G_NonTxn
- ParallelityBenchPhase2G_Txn10
- ParallelityBenchPhase2G_Txn10_DualQueue
- ParallelityBenchPhase2G_Txn5
- ParallelityBenchPhase2G_Unprepared_NonTxn
- ParallelityBenchSharded
- ReadWriteRatioBench
- RocksDBRaw_NonTxn
- RocksDBRaw_Txn10
- SandboxStatsFixture
- ScalabilityBenchFixture
- SecondaryIndexBench
- SelfProtectionBench
- SimpleVectorBench
- StressTestBench
- ThemisNoPipe_NonTxn
- ThemisNoPipe_Txn10
- ThemisWithPipe_NonTxn
- ThemisWithPipe_Txn10
- WasmLoadBytesFixture
- themis::modules::AbiCheckerSymbolTest
- themis::modules::AbiCheckerVersionTest
- themis::modules::AdvancedDependencyTest
- themis::modules::BaseErrorTaxonomyTest
- themis::modules::DependencyResolverEdgeTest
- themis::modules::FailClosedTest
- themis::modules::ModuleSecurityVerifier::Impl
- themis::modules::MultiModuleTest
- themis::modules::OperatorDiagnosticsTest
- themis::modules::PluginDependencyGraphEdgeTest
- themis::modules::PluginGraphExtendedTest
- themis::modules::RegistryConfigValidationTest
- themis::modules::ReloadPhaseOrderTest
- themis::modules::ReloadRollbackTest
- themis::modules::ReloadStatsTest
- themis::modules::SandboxDegradedStateTest
- themis::modules::SandboxWasmIsolationTest
- themis::modules::StateSaveRestoreTest
- themis::modules::TracingIntegrationTest
- themis::modules::WasmFuelBudgetTest
- themis::modules::WasmHostFunctionTest
- themis::modules::WasmModuleInfoTest
- themis::modules::WasmSandboxValidationTest
- themis::resource::AdaptiveConnectionPool
- themis::resource::BufferHandle
- themis::resource::BufferPool
- themis::resource::ResourcePoolManager

### Structs
- LatencyTracker::LatencyStats
- themis::modules::ModuleSandbox::PlatformHandle
- themis::modules::ModuleSandbox::PlatformHandle::saved_cpu_limit
- themis::modules::ModuleSandbox::PlatformHandle::saved_mem_limit
- themis::modules::SpanCollector
- themis::resource::AdaptiveConnectionPool::Config
- themis::resource::AdaptiveConnectionPool::Statistics
- themis::resource::BufferPool::Config
- themis::resource::BufferPool::Slab
- themis::resource::BufferPool::Statistics
- themis::resource::ResourcePoolManager::Config
- themis::resource::ResourcePoolManager::GlobalStatistics

## Notes
- This is a derived artifact. Do not manually edit semantic content.
- Regenerate via scripts/generate_module_doxygen_xml.py --write-module-markdown.

## Detailed Function Documentation (Soll-Ist)
- Functions extracted: 837

### AQLJoinBench

#### `void SetUp(const ::benchmark::State &state) override`
- Source: `benchmarks/base/bench_comprehensive.cpp`:371
- Brief: n/a
- Parameters:
  - `state` (const ::benchmark::State &): n/a

#### `void TearDown(const ::benchmark::State &state) override`
- Source: `benchmarks/base/bench_comprehensive.cpp`:386
- Brief: n/a
- Parameters:
  - `state` (const ::benchmark::State &): n/a

### AQLQueryBench

#### `void SetUp(const ::benchmark::State &state) override`
- Source: `benchmarks/base/bench_comprehensive.cpp`:298
- Brief: n/a
- Parameters:
  - `state` (const ::benchmark::State &): n/a

#### `void TearDown(const ::benchmark::State &state) override`
- Source: `benchmarks/base/bench_comprehensive.cpp`:314
- Brief: n/a
- Parameters:
  - `state` (const ::benchmark::State &): n/a

### AbiCheckerFixture

#### `void SetUp(const benchmark::State &) override`
- Source: `benchmarks/base/bench_base_wasm_sandbox.cpp`:136
- Brief: n/a
- Parameters:
  - `<unnamed>` (const benchmark::State &): n/a

#### `void TearDown(const benchmark::State &) override`
- Source: `benchmarks/base/bench_base_wasm_sandbox.cpp`:148
- Brief: n/a
- Parameters:
  - `<unnamed>` (const benchmark::State &): n/a

### BaseEntityTest

#### `void SetUp() override`
- Source: `tests/base/test_base_entity.cpp`:10
- Brief: n/a
- Parameters: none

#### `void TearDown() override`
- Source: `tests/base/test_base_entity.cpp`:14
- Brief: n/a
- Parameters: none

### BaseHotPathsLoaderFixture

#### `void SetUp(const benchmark::State &) override`
- Source: `benchmarks/base/bench_base_hot_paths.cpp`:48
- Brief: n/a
- Parameters:
  - `<unnamed>` (const benchmark::State &): n/a

#### `void TearDown(const benchmark::State &) override`
- Source: `benchmarks/base/bench_base_hot_paths.cpp`:53
- Brief: n/a
- Parameters:
  - `<unnamed>` (const benchmark::State &): n/a

### BaseHotPathsReloadFixture

#### `void SetUp(const benchmark::State &) override`
- Source: `benchmarks/base/bench_base_hot_paths.cpp`:124
- Brief: n/a
- Parameters:
  - `<unnamed>` (const benchmark::State &): n/a

#### `void TearDown(const benchmark::State &) override`
- Source: `benchmarks/base/bench_base_hot_paths.cpp`:130
- Brief: n/a
- Parameters:
  - `<unnamed>` (const benchmark::State &): n/a

### BatchOperationsBench

#### `void SetUp(const ::benchmark::State &state) override`
- Source: `benchmarks/base/bench_comprehensive.cpp`:724
- Brief: n/a
- Parameters:
  - `state` (const ::benchmark::State &): n/a

#### `void TearDown(const ::benchmark::State &state) override`
- Source: `benchmarks/base/bench_comprehensive.cpp`:737
- Brief: n/a
- Parameters:
  - `state` (const ::benchmark::State &): n/a

### BestPracticeBench

#### `void SetUp(const benchmark::State &) override`
- Source: `benchmarks/base/bench_advanced_patterns.cpp`:2916
- Brief: n/a
- Parameters:
  - `<unnamed>` (const benchmark::State &): n/a

#### `void TearDown(const benchmark::State &) override`
- Source: `benchmarks/base/bench_advanced_patterns.cpp`:2920
- Brief: n/a
- Parameters:
  - `<unnamed>` (const benchmark::State &): n/a

### BinaryOperationsBench

#### `void SetUp(const ::benchmark::State &state) override`
- Source: `benchmarks/base/bench_comprehensive.cpp`:437
- Brief: n/a
- Parameters:
  - `state` (const ::benchmark::State &): n/a

#### `void TearDown(const ::benchmark::State &state) override`
- Source: `benchmarks/base/bench_comprehensive.cpp`:456
- Brief: n/a
- Parameters:
  - `state` (const ::benchmark::State &): n/a

### ComplexVectorBench

#### `void SetUp(const ::benchmark::State &state) override`
- Source: `benchmarks/base/bench_comprehensive.cpp`:133
- Brief: n/a
- Parameters:
  - `state` (const ::benchmark::State &): n/a

#### `void TearDown(const ::benchmark::State &state) override`
- Source: `benchmarks/base/bench_comprehensive.cpp`:145
- Brief: n/a
- Parameters:
  - `state` (const ::benchmark::State &): n/a

### DeterministicRNG

#### `DeterministicRNG(uint64_t seed=42)`
- Source: `benchmarks/base/bench_edge_cases_comprehensive.cpp`:44
- Brief: n/a
- Parameters:
  - `seed` (uint64_t): n/a

#### `DeterministicRNG(uint64_t seed=42)`
- Source: `benchmarks/base/bench_latency_comprehensive.cpp`:111
- Brief: n/a
- Parameters:
  - `seed` (uint64_t): n/a

#### `DeterministicRNG(uint64_t seed=42)`
- Source: `benchmarks/base/bench_multithreading_comprehensive.cpp`:35
- Brief: n/a
- Parameters:
  - `seed` (uint64_t): n/a

#### `DeterministicRNG(uint64_t seed=42)`
- Source: `benchmarks/base/bench_scalability_comprehensive.cpp`:37
- Brief: n/a
- Parameters:
  - `seed` (uint64_t): n/a

#### `int64_t generateInt(int64_t min, int64_t max)`
- Source: `benchmarks/base/bench_edge_cases_comprehensive.cpp`:58
- Brief: n/a
- Parameters:
  - `min` (int64_t): n/a
  - `max` (int64_t): n/a

#### `int64_t generateInt(int64_t min, int64_t max)`
- Source: `benchmarks/base/bench_latency_comprehensive.cpp`:127
- Brief: n/a
- Parameters:
  - `min` (int64_t): n/a
  - `max` (int64_t): n/a

#### `int64_t generateInt(int64_t min, int64_t max)`
- Source: `benchmarks/base/bench_multithreading_comprehensive.cpp`:52
- Brief: n/a
- Parameters:
  - `min` (int64_t): n/a
  - `max` (int64_t): n/a

#### `int64_t generateInt(int64_t min, int64_t max)`
- Source: `benchmarks/base/bench_scalability_comprehensive.cpp`:51
- Brief: n/a
- Parameters:
  - `min` (int64_t): n/a
  - `max` (int64_t): n/a

#### `std::string generateString(size_t length)`
- Source: `benchmarks/base/bench_edge_cases_comprehensive.cpp`:48
- Brief: n/a
- Parameters:
  - `length` (size_t): n/a

#### `std::string generateString(size_t length)`
- Source: `benchmarks/base/bench_latency_comprehensive.cpp`:117
- Brief: n/a
- Parameters:
  - `length` (size_t): n/a

#### `std::string generateString(size_t length)`
- Source: `benchmarks/base/bench_multithreading_comprehensive.cpp`:42
- Brief: n/a
- Parameters:
  - `length` (size_t): n/a

#### `std::string generateString(size_t length)`
- Source: `benchmarks/base/bench_scalability_comprehensive.cpp`:41
- Brief: n/a
- Parameters:
  - `length` (size_t): n/a

#### `uint64_t next()`
- Source: `benchmarks/base/bench_edge_cases_comprehensive.cpp`:46
- Brief: n/a
- Parameters: none

#### `uint64_t next()`
- Source: `benchmarks/base/bench_latency_comprehensive.cpp`:113
- Brief: n/a
- Parameters: none

#### `uint64_t next()`
- Source: `benchmarks/base/bench_multithreading_comprehensive.cpp`:37
- Brief: n/a
- Parameters: none

#### `uint64_t next()`
- Source: `benchmarks/base/bench_scalability_comprehensive.cpp`:39
- Brief: n/a
- Parameters: none

### GapAnalysisBench

#### `void SetUp(const benchmark::State &) override`
- Source: `benchmarks/base/bench_advanced_patterns.cpp`:3000
- Brief: n/a
- Parameters:
  - `<unnamed>` (const benchmark::State &): n/a

#### `void TearDown(const benchmark::State &) override`
- Source: `benchmarks/base/bench_advanced_patterns.cpp`:3004
- Brief: n/a
- Parameters:
  - `<unnamed>` (const benchmark::State &): n/a

### GraphExportFixture

#### `void SetUp(const benchmark::State &) override`
- Source: `benchmarks/base/bench_base_wasm_sandbox.cpp`:391
- Brief: n/a
- Parameters:
  - `<unnamed>` (const benchmark::State &): n/a

#### `void TearDown(const benchmark::State &) override`
- Source: `benchmarks/base/bench_base_wasm_sandbox.cpp`:403
- Brief: n/a
- Parameters:
  - `<unnamed>` (const benchmark::State &): n/a

### GraphOperationsBench

#### `void SetUp(const ::benchmark::State &state) override`
- Source: `benchmarks/base/bench_comprehensive.cpp`:528
- Brief: n/a
- Parameters:
  - `state` (const ::benchmark::State &): n/a

#### `void TearDown(const ::benchmark::State &state) override`
- Source: `benchmarks/base/bench_comprehensive.cpp`:541
- Brief: n/a
- Parameters:
  - `state` (const ::benchmark::State &): n/a

### HotReloadLoadFixture

#### `void SetUp(const benchmark::State &) override`
- Source: `benchmarks/base/bench_module_load_hot_reload.cpp`:138
- Brief: n/a
- Parameters:
  - `<unnamed>` (const benchmark::State &): n/a

#### `void TearDown(const benchmark::State &) override`
- Source: `benchmarks/base/bench_module_load_hot_reload.cpp`:148
- Brief: n/a
- Parameters:
  - `<unnamed>` (const benchmark::State &): n/a

### LLMInferencingBench

#### `void SetUp(const ::benchmark::State &state) override`
- Source: `benchmarks/base/bench_comprehensive.cpp`:200
- Brief: n/a
- Parameters:
  - `state` (const ::benchmark::State &): n/a

#### `void TearDown(const ::benchmark::State &state) override`
- Source: `benchmarks/base/bench_comprehensive.cpp`:212
- Brief: n/a
- Parameters:
  - `state` (const ::benchmark::State &): n/a

### LatencyBenchFixture

#### `void SetUp(const ::benchmark::State &state) override`
- Source: `benchmarks/base/bench_latency_comprehensive.cpp`:138
- Brief: n/a
- Parameters:
  - `state` (const ::benchmark::State &): n/a

#### `void TearDown(const ::benchmark::State &state) override`
- Source: `benchmarks/base/bench_latency_comprehensive.cpp`:159
- Brief: n/a
- Parameters:
  - `state` (const ::benchmark::State &): n/a

#### `void setLatencyCounters(benchmark::State &state)`
- Source: `benchmarks/base/bench_latency_comprehensive.cpp`:180
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void warmup()`
- Source: `benchmarks/base/bench_latency_comprehensive.cpp`:169
- Brief: n/a
- Parameters: none

### LatencyTracker

#### `LatencyStats calculateStats() const`
- Source: `benchmarks/base/bench_latency_comprehensive.cpp`:54
- Brief: n/a
- Parameters: none

#### `void record(double latency_us)`
- Source: `benchmarks/base/bench_latency_comprehensive.cpp`:38
- Brief: n/a
- Parameters:
  - `latency_us` (double): n/a

#### `void reset()`
- Source: `benchmarks/base/bench_latency_comprehensive.cpp`:96
- Brief: n/a
- Parameters: none

### MockExpressionEvaluator

#### `bool evaluate(const std::string &, const void *) const override`
- Source: `tests/base/test_base_interfaces.cpp`:107
- Brief: n/a
- Parameters:
  - `<unnamed>` (const std::string &): n/a
  - `<unnamed>` (const void *): n/a

#### `std::string get_expression_type() const override`
- Source: `tests/base/test_base_interfaces.cpp`:110
- Brief: n/a
- Parameters: none

### MockFieldEncryption

#### `std::vector< uint8_t > decrypt_field(const std::string &field_name, const std::vector< uint8_t > &ciphertext) override`
- Source: `tests/base/test_base_interfaces.cpp`:151
- Brief: n/a
- Parameters:
  - `field_name` (const std::string &): n/a
  - `ciphertext` (const std::vector< uint8_t > &): n/a

#### `std::vector< uint8_t > encrypt_field(const std::string &, const std::vector< uint8_t > &plaintext) override`
- Source: `tests/base/test_base_interfaces.cpp`:140
- Brief: n/a
- Parameters:
  - `<unnamed>` (const std::string &): n/a
  - `plaintext` (const std::vector< uint8_t > &): n/a

#### `bool should_encrypt(const std::string &field_name) const override`
- Source: `tests/base/test_base_interfaces.cpp`:157
- Brief: n/a
- Parameters:
  - `field_name` (const std::string &): n/a

### MockGraphIndex

#### `std::vector< std::string > findShortestPath(std::string_view from, std::string_view to, std::string_view="", uint32_t=0) const override`
- Source: `tests/base/test_base_interfaces.cpp`:322
- Brief: n/a
- Parameters:
  - `from` (std::string_view): n/a
  - `to` (std::string_view): n/a
  - `<unnamed>` (std::string_view): n/a
  - `<unnamed>` (uint32_t): n/a

#### `std::vector< GraphEdge > getIncomingEdges(std::string_view node_id, std::string_view edge_type="") const override`
- Source: `tests/base/test_base_interfaces.cpp`:309
- Brief: n/a
- Parameters:
  - `node_id` (std::string_view): n/a
  - `edge_type` (std::string_view): n/a

#### `std::string getName() const override`
- Source: `tests/base/test_base_interfaces.cpp`:336
- Brief: n/a
- Parameters: none

#### `std::vector< GraphEdge > getOutgoingEdges(std::string_view node_id, std::string_view edge_type="") const override`
- Source: `tests/base/test_base_interfaces.cpp`:296
- Brief: n/a
- Parameters:
  - `node_id` (std::string_view): n/a
  - `edge_type` (std::string_view): n/a

#### `std::string getStatistics() const override`
- Source: `tests/base/test_base_interfaces.cpp`:337
- Brief: n/a
- Parameters: none

#### `bool insertEdge(const GraphEdge &edge) override`
- Source: `tests/base/test_base_interfaces.cpp`:278
- Brief: n/a
- Parameters:
  - `edge` (const GraphEdge &): n/a

#### `bool removeEdge(std::string_view from, std::string_view to, std::string_view edge_type="") override`
- Source: `tests/base/test_base_interfaces.cpp`:283
- Brief: n/a
- Parameters:
  - `from` (std::string_view): n/a
  - `to` (std::string_view): n/a
  - `edge_type` (std::string_view): n/a

### MockKeyProvider

#### `std::vector< uint8_t > get_key(const std::string &) override`
- Source: `tests/base/test_base_interfaces.cpp`:166
- Brief: n/a
- Parameters:
  - `<unnamed>` (const std::string &): n/a

#### `std::vector< uint8_t > rotate_key(const std::string &) override`
- Source: `tests/base/test_base_interfaces.cpp`:170
- Brief: n/a
- Parameters:
  - `<unnamed>` (const std::string &): n/a

### MockQueryEngine

#### `Result< std::unique_ptr< IExpressionEvaluator > > createExpressionEvaluator() const override`
- Source: `tests/base/test_base_interfaces.cpp`:126
- Brief: n/a
- Parameters: none

#### `Result< std::string > execute(const std::string &) override`
- Source: `tests/base/test_base_interfaces.cpp`:117
- Brief: n/a
- Parameters:
  - `<unnamed>` (const std::string &): n/a

#### `Result< std::string > explainQuery(const std::string &) const override`
- Source: `tests/base/test_base_interfaces.cpp`:131
- Brief: n/a
- Parameters:
  - `<unnamed>` (const std::string &): n/a

#### `Result< void > validate(const std::string &) const override`
- Source: `tests/base/test_base_interfaces.cpp`:121
- Brief: n/a
- Parameters:
  - `<unnamed>` (const std::string &): n/a

### MockSecondaryIndex

#### `std::string getFieldName() const override`
- Source: `tests/base/test_base_interfaces.cpp`:220
- Brief: n/a
- Parameters: none

#### `std::string getName() const override`
- Source: `tests/base/test_base_interfaces.cpp`:219
- Brief: n/a
- Parameters: none

#### `std::string getStatistics() const override`
- Source: `tests/base/test_base_interfaces.cpp`:221
- Brief: n/a
- Parameters: none

#### `bool insert(std::string_view indexed_value, std::string_view primary_key) override`
- Source: `tests/base/test_base_interfaces.cpp`:179
- Brief: n/a
- Parameters:
  - `indexed_value` (std::string_view): n/a
  - `primary_key` (std::string_view): n/a

#### `std::vector< std::string > lookup(std::string_view value) const override`
- Source: `tests/base/test_base_interfaces.cpp`:200
- Brief: n/a
- Parameters:
  - `value` (std::string_view): n/a

#### `std::vector< std::string > rangeScan(std::string_view start, std::string_view end, ScanOrder=ScanOrder::ASCENDING) const override`
- Source: `tests/base/test_base_interfaces.cpp`:206
- Brief: n/a
- Parameters:
  - `start` (std::string_view): n/a
  - `end` (std::string_view): n/a
  - `<unnamed>` (ScanOrder): n/a

#### `bool remove(std::string_view indexed_value, std::string_view primary_key) override`
- Source: `tests/base/test_base_interfaces.cpp`:185
- Brief: n/a
- Parameters:
  - `indexed_value` (std::string_view): n/a
  - `primary_key` (std::string_view): n/a

### MockStorageEngine

#### `void close() override`
- Source: `tests/base/test_base_interfaces.cpp`:75
- Brief: n/a
- Parameters: none

#### `Result< void > del(const std::string &key) override`
- Source: `tests/base/test_base_interfaces.cpp`:90
- Brief: n/a
- Parameters:
  - `key` (const std::string &): n/a

#### `Result< std::string > get(const std::string &key) override`
- Source: `tests/base/test_base_interfaces.cpp`:82
- Brief: n/a
- Parameters:
  - `key` (const std::string &): n/a

#### `Result< void > open(const std::string &) override`
- Source: `tests/base/test_base_interfaces.cpp`:70
- Brief: n/a
- Parameters:
  - `<unnamed>` (const std::string &): n/a

#### `Result< void > put(const std::string &key, const std::string &value) override`
- Source: `tests/base/test_base_interfaces.cpp`:77
- Brief: n/a
- Parameters:
  - `key` (const std::string &): n/a
  - `value` (const std::string &): n/a

### MockVectorIndex

#### `uint32_t getDimension() const override`
- Source: `tests/base/test_base_interfaces.cpp`:267
- Brief: n/a
- Parameters: none

#### `std::string getName() const override`
- Source: `tests/base/test_base_interfaces.cpp`:266
- Brief: n/a
- Parameters: none

#### `std::string getStatistics() const override`
- Source: `tests/base/test_base_interfaces.cpp`:268
- Brief: n/a
- Parameters: none

#### `bool insert(std::string_view primary_key, const std::vector< float > &vector) override`
- Source: `tests/base/test_base_interfaces.cpp`:231
- Brief: n/a
- Parameters:
  - `primary_key` (std::string_view): n/a
  - `vector` (const std::vector< float > &): n/a

#### `std::vector< VectorSearchResult > rangeSearch(const std::vector< float > &, float, const IExpressionEvaluator *=nullptr) const override`
- Source: `tests/base/test_base_interfaces.cpp`:255
- Brief: n/a
- Parameters:
  - `<unnamed>` (const std::vector< float > &): n/a
  - `<unnamed>` (float): n/a
  - `<unnamed>` (const IExpressionEvaluator *): n/a

#### `bool remove(std::string_view primary_key) override`
- Source: `tests/base/test_base_interfaces.cpp`:237
- Brief: n/a
- Parameters:
  - `primary_key` (std::string_view): n/a

#### `std::vector< VectorSearchResult > search(const std::vector< float > &, uint32_t k, const IExpressionEvaluator *=nullptr) const override`
- Source: `tests/base/test_base_interfaces.cpp`:241
- Brief: n/a
- Parameters:
  - `<unnamed>` (const std::vector< float > &): n/a
  - `k` (uint32_t): n/a
  - `<unnamed>` (const IExpressionEvaluator *): n/a

### ModuleLoaderFixture

#### `void SetUp(const benchmark::State &) override`
- Source: `benchmarks/base/bench_module_load_hot_reload.cpp`:37
- Brief: n/a
- Parameters:
  - `<unnamed>` (const benchmark::State &): n/a

#### `void TearDown(const benchmark::State &) override`
- Source: `benchmarks/base/bench_module_load_hot_reload.cpp`:42
- Brief: n/a
- Parameters:
  - `<unnamed>` (const benchmark::State &): n/a

### MultiThreadBenchEnv

#### `std::shared_ptr< RocksDBWrapper > getDB()`
- Source: `benchmarks/base/bench_multithreading_comprehensive.cpp`:116
- Brief: n/a
- Parameters: none

#### `DeterministicRNG & getRNG()`
- Source: `benchmarks/base/bench_multithreading_comprehensive.cpp`:123
- Brief: n/a
- Parameters: none

#### `void initialize()`
- Source: `benchmarks/base/bench_multithreading_comprehensive.cpp`:75
- Brief: n/a
- Parameters: none

#### `MultiThreadBenchEnv & instance()`
- Source: `benchmarks/base/bench_multithreading_comprehensive.cpp`:70
- Brief: n/a
- Parameters: none

#### `void warmup()`
- Source: `benchmarks/base/bench_multithreading_comprehensive.cpp`:105
- Brief: n/a
- Parameters: none

#### `~MultiThreadBenchEnv()`
- Source: `benchmarks/base/bench_multithreading_comprehensive.cpp`:125
- Brief: n/a
- Parameters: none

### ParallelityBench

#### `void SetUp(const benchmark::State &) override`
- Source: `benchmarks/base/bench_advanced_patterns.cpp`:2132
- Brief: n/a
- Parameters:
  - `<unnamed>` (const benchmark::State &): n/a

#### `void TearDown(const benchmark::State &) override`
- Source: `benchmarks/base/bench_advanced_patterns.cpp`:2147
- Brief: n/a
- Parameters:
  - `<unnamed>` (const benchmark::State &): n/a

### ParallelityBenchBatchWrite

#### `void SetUp(const benchmark::State &) override`
- Source: `benchmarks/base/bench_advanced_patterns.cpp`:2662
- Brief: n/a
- Parameters:
  - `<unnamed>` (const benchmark::State &): n/a

#### `void TearDown(const benchmark::State &) override`
- Source: `benchmarks/base/bench_advanced_patterns.cpp`:2668
- Brief: n/a
- Parameters:
  - `<unnamed>` (const benchmark::State &): n/a

### ParallelityBenchOptimized

#### `void SetUp(const benchmark::State &) override`
- Source: `benchmarks/base/bench_advanced_patterns.cpp`:2242
- Brief: n/a
- Parameters:
  - `<unnamed>` (const benchmark::State &): n/a

#### `void TearDown(const benchmark::State &) override`
- Source: `benchmarks/base/bench_advanced_patterns.cpp`:2250
- Brief: n/a
- Parameters:
  - `<unnamed>` (const benchmark::State &): n/a

### ParallelityBenchOptimizedConfig

#### `void SetUp(const benchmark::State &) override`
- Source: `benchmarks/base/bench_advanced_patterns.cpp`:2538
- Brief: n/a
- Parameters:
  - `<unnamed>` (const benchmark::State &): n/a

#### `void TearDown(const benchmark::State &) override`
- Source: `benchmarks/base/bench_advanced_patterns.cpp`:2544
- Brief: n/a
- Parameters:
  - `<unnamed>` (const benchmark::State &): n/a

### ParallelityBenchPhase1Final

#### `void SetUp(const benchmark::State &) override`
- Source: `benchmarks/base/bench_advanced_patterns.cpp`:200
- Brief: n/a
- Parameters:
  - `<unnamed>` (const benchmark::State &): n/a

#### `void TearDown(const benchmark::State &) override`
- Source: `benchmarks/base/bench_advanced_patterns.cpp`:206
- Brief: n/a
- Parameters:
  - `<unnamed>` (const benchmark::State &): n/a

### ParallelityBenchPhase2Final

#### `void SetUp(const benchmark::State &) override`
- Source: `benchmarks/base/bench_advanced_patterns.cpp`:315
- Brief: n/a
- Parameters:
  - `<unnamed>` (const benchmark::State &): n/a

#### `void TearDown(const benchmark::State &) override`
- Source: `benchmarks/base/bench_advanced_patterns.cpp`:340
- Brief: n/a
- Parameters:
  - `<unnamed>` (const benchmark::State &): n/a

### ParallelityBenchPhase2G

#### `void SetUp(const benchmark::State &) override`
- Source: `benchmarks/base/bench_advanced_patterns.cpp`:446
- Brief: n/a
- Parameters:
  - `<unnamed>` (const benchmark::State &): n/a

#### `void TearDown(const benchmark::State &) override`
- Source: `benchmarks/base/bench_advanced_patterns.cpp`:467
- Brief: n/a
- Parameters:
  - `<unnamed>` (const benchmark::State &): n/a

#### `void doTxnWrites(int thread_id, int records)`
- Source: `benchmarks/base/bench_advanced_patterns.cpp`:473
- Brief: n/a
- Parameters:
  - `thread_id` (int): n/a
  - `records` (int): n/a

### ParallelityBenchPhase2G_2H

#### `void SetUp(const benchmark::State &) override`
- Source: `benchmarks/base/bench_advanced_patterns.cpp`:501
- Brief: n/a
- Parameters:
  - `<unnamed>` (const benchmark::State &): n/a

#### `void TearDown(const benchmark::State &) override`
- Source: `benchmarks/base/bench_advanced_patterns.cpp`:534
- Brief: n/a
- Parameters:
  - `<unnamed>` (const benchmark::State &): n/a

#### `void doTxnWrites(int thread_id, int records)`
- Source: `benchmarks/base/bench_advanced_patterns.cpp`:540
- Brief: n/a
- Parameters:
  - `thread_id` (int): n/a
  - `records` (int): n/a

### ParallelityBenchPhase2G_NonTxn

#### `void SetUp(const benchmark::State &) override`
- Source: `benchmarks/base/bench_advanced_patterns.cpp`:677
- Brief: n/a
- Parameters:
  - `<unnamed>` (const benchmark::State &): n/a

#### `void TearDown(const benchmark::State &) override`
- Source: `benchmarks/base/bench_advanced_patterns.cpp`:698
- Brief: n/a
- Parameters:
  - `<unnamed>` (const benchmark::State &): n/a

### ParallelityBenchPhase2G_Txn10

#### `void SetUp(const benchmark::State &) override`
- Source: `benchmarks/base/bench_advanced_patterns.cpp`:933
- Brief: n/a
- Parameters:
  - `<unnamed>` (const benchmark::State &): n/a

#### `void TearDown(const benchmark::State &) override`
- Source: `benchmarks/base/bench_advanced_patterns.cpp`:953
- Brief: n/a
- Parameters:
  - `<unnamed>` (const benchmark::State &): n/a

#### `void doTxnChunked(int thread_id, int total_records, int chunk)`
- Source: `benchmarks/base/bench_advanced_patterns.cpp`:959
- Brief: n/a
- Parameters:
  - `thread_id` (int): n/a
  - `total_records` (int): n/a
  - `chunk` (int): n/a

### ParallelityBenchPhase2G_Txn10_DualQueue

#### `void SetUp(const benchmark::State &) override`
- Source: `benchmarks/base/bench_advanced_patterns.cpp`:1161
- Brief: n/a
- Parameters:
  - `<unnamed>` (const benchmark::State &): n/a

#### `void TearDown(const benchmark::State &) override`
- Source: `benchmarks/base/bench_advanced_patterns.cpp`:1181
- Brief: n/a
- Parameters:
  - `<unnamed>` (const benchmark::State &): n/a

#### `void doTxnChunked(int thread_id, int total_records, int chunk)`
- Source: `benchmarks/base/bench_advanced_patterns.cpp`:1187
- Brief: n/a
- Parameters:
  - `thread_id` (int): n/a
  - `total_records` (int): n/a
  - `chunk` (int): n/a

### ParallelityBenchPhase2G_Txn5

#### `void SetUp(const benchmark::State &) override`
- Source: `benchmarks/base/bench_advanced_patterns.cpp`:1047
- Brief: n/a
- Parameters:
  - `<unnamed>` (const benchmark::State &): n/a

#### `void TearDown(const benchmark::State &) override`
- Source: `benchmarks/base/bench_advanced_patterns.cpp`:1067
- Brief: n/a
- Parameters:
  - `<unnamed>` (const benchmark::State &): n/a

#### `void doTxnChunked(int thread_id, int total_records, int chunk)`
- Source: `benchmarks/base/bench_advanced_patterns.cpp`:1073
- Brief: n/a
- Parameters:
  - `thread_id` (int): n/a
  - `total_records` (int): n/a
  - `chunk` (int): n/a

### ParallelityBenchPhase2G_Unprepared_NonTxn

#### `void SetUp(const benchmark::State &) override`
- Source: `benchmarks/base/bench_advanced_patterns.cpp`:805
- Brief: n/a
- Parameters:
  - `<unnamed>` (const benchmark::State &): n/a

#### `void TearDown(const benchmark::State &) override`
- Source: `benchmarks/base/bench_advanced_patterns.cpp`:826
- Brief: n/a
- Parameters:
  - `<unnamed>` (const benchmark::State &): n/a

### ParallelityBenchSharded

#### `void SetUp(const benchmark::State &) override`
- Source: `benchmarks/base/bench_advanced_patterns.cpp`:2393
- Brief: n/a
- Parameters:
  - `<unnamed>` (const benchmark::State &): n/a

#### `void TearDown(const benchmark::State &) override`
- Source: `benchmarks/base/bench_advanced_patterns.cpp`:2405
- Brief: n/a
- Parameters:
  - `<unnamed>` (const benchmark::State &): n/a

### ReadWriteRatioBench

#### `void SetUp(const benchmark::State &) override`
- Source: `benchmarks/base/bench_advanced_patterns.cpp`:1997
- Brief: n/a
- Parameters:
  - `<unnamed>` (const benchmark::State &): n/a

#### `void TearDown(const benchmark::State &) override`
- Source: `benchmarks/base/bench_advanced_patterns.cpp`:2006
- Brief: n/a
- Parameters:
  - `<unnamed>` (const benchmark::State &): n/a

#### `void performRead(int entity_id)`
- Source: `benchmarks/base/bench_advanced_patterns.cpp`:2021
- Brief: n/a
- Parameters:
  - `entity_id` (int): n/a

#### `void performWrite(int entity_id)`
- Source: `benchmarks/base/bench_advanced_patterns.cpp`:2027
- Brief: n/a
- Parameters:
  - `entity_id` (int): n/a

#### `void populateDataset(int count)`
- Source: `benchmarks/base/bench_advanced_patterns.cpp`:2011
- Brief: n/a
- Parameters:
  - `count` (int): n/a

### RocksDBRaw_NonTxn

#### `void SetUp(const benchmark::State &) override`
- Source: `benchmarks/base/bench_advanced_patterns.cpp`:1278
- Brief: n/a
- Parameters:
  - `<unnamed>` (const benchmark::State &): n/a

#### `void TearDown(const benchmark::State &) override`
- Source: `benchmarks/base/bench_advanced_patterns.cpp`:1301
- Brief: n/a
- Parameters:
  - `<unnamed>` (const benchmark::State &): n/a

### RocksDBRaw_Txn10

#### `void SetUp(const benchmark::State &) override`
- Source: `benchmarks/base/bench_advanced_patterns.cpp`:1436
- Brief: n/a
- Parameters:
  - `<unnamed>` (const benchmark::State &): n/a

#### `void TearDown(const benchmark::State &) override`
- Source: `benchmarks/base/bench_advanced_patterns.cpp`:1459
- Brief: n/a
- Parameters:
  - `<unnamed>` (const benchmark::State &): n/a

#### `void doTxnChunked(int thread_id, int total_records, int chunk)`
- Source: `benchmarks/base/bench_advanced_patterns.cpp`:1464
- Brief: n/a
- Parameters:
  - `thread_id` (int): n/a
  - `total_records` (int): n/a
  - `chunk` (int): n/a

### SandboxStatsFixture

#### `void SetUp(const benchmark::State &) override`
- Source: `benchmarks/base/bench_base_wasm_sandbox.cpp`:269
- Brief: n/a
- Parameters:
  - `<unnamed>` (const benchmark::State &): n/a

#### `void TearDown(const benchmark::State &) override`
- Source: `benchmarks/base/bench_base_wasm_sandbox.cpp`:277
- Brief: n/a
- Parameters:
  - `<unnamed>` (const benchmark::State &): n/a

### ScalabilityBenchFixture

#### `void SetUp(const ::benchmark::State &state) override`
- Source: `benchmarks/base/bench_scalability_comprehensive.cpp`:62
- Brief: n/a
- Parameters:
  - `state` (const ::benchmark::State &): n/a

#### `void TearDown(const ::benchmark::State &state) override`
- Source: `benchmarks/base/bench_scalability_comprehensive.cpp`:83
- Brief: n/a
- Parameters:
  - `state` (const ::benchmark::State &): n/a

#### `void populateDataset()`
- Source: `benchmarks/base/bench_scalability_comprehensive.cpp`:93
- Brief: n/a
- Parameters: none

### SecondaryIndexBench

#### `void SetUp(const ::benchmark::State &state) override`
- Source: `benchmarks/base/bench_comprehensive.cpp`:618
- Brief: n/a
- Parameters:
  - `state` (const ::benchmark::State &): n/a

#### `void TearDown(const ::benchmark::State &state) override`
- Source: `benchmarks/base/bench_comprehensive.cpp`:631
- Brief: n/a
- Parameters:
  - `state` (const ::benchmark::State &): n/a

### SelfProtectionBench

#### `void SetUp(const benchmark::State &) override`
- Source: `benchmarks/base/bench_advanced_patterns.cpp`:2798
- Brief: n/a
- Parameters:
  - `<unnamed>` (const benchmark::State &): n/a

#### `void TearDown(const benchmark::State &) override`
- Source: `benchmarks/base/bench_advanced_patterns.cpp`:2813
- Brief: n/a
- Parameters:
  - `<unnamed>` (const benchmark::State &): n/a

### SimpleVectorBench

#### `void SetUp(const ::benchmark::State &state) override`
- Source: `benchmarks/base/bench_comprehensive.cpp`:60
- Brief: n/a
- Parameters:
  - `state` (const ::benchmark::State &): n/a

#### `void TearDown(const ::benchmark::State &state) override`
- Source: `benchmarks/base/bench_comprehensive.cpp`:72
- Brief: n/a
- Parameters:
  - `state` (const ::benchmark::State &): n/a

### StressTestBench

#### `void SetUp(const ::benchmark::State &state) override`
- Source: `benchmarks/base/bench_comprehensive.cpp`:798
- Brief: n/a
- Parameters:
  - `state` (const ::benchmark::State &): n/a

#### `void TearDown(const ::benchmark::State &state) override`
- Source: `benchmarks/base/bench_comprehensive.cpp`:813
- Brief: n/a
- Parameters:
  - `state` (const ::benchmark::State &): n/a

### ThemisNoPipe_NonTxn

#### `void SetUp(const benchmark::State &) override`
- Source: `benchmarks/base/bench_advanced_patterns.cpp`:1592
- Brief: n/a
- Parameters:
  - `<unnamed>` (const benchmark::State &): n/a

#### `void TearDown(const benchmark::State &) override`
- Source: `benchmarks/base/bench_advanced_patterns.cpp`:1612
- Brief: n/a
- Parameters:
  - `<unnamed>` (const benchmark::State &): n/a

### ThemisNoPipe_Txn10

#### `void SetUp(const benchmark::State &) override`
- Source: `benchmarks/base/bench_advanced_patterns.cpp`:1700
- Brief: n/a
- Parameters:
  - `<unnamed>` (const benchmark::State &): n/a

#### `void TearDown(const benchmark::State &) override`
- Source: `benchmarks/base/bench_advanced_patterns.cpp`:1720
- Brief: n/a
- Parameters:
  - `<unnamed>` (const benchmark::State &): n/a

#### `void doTxnChunked(int thread_id, int total_records, int chunk)`
- Source: `benchmarks/base/bench_advanced_patterns.cpp`:1726
- Brief: n/a
- Parameters:
  - `thread_id` (int): n/a
  - `total_records` (int): n/a
  - `chunk` (int): n/a

### ThemisWithPipe_NonTxn

#### `void SetUp(const benchmark::State &) override`
- Source: `benchmarks/base/bench_advanced_patterns.cpp`:1796
- Brief: n/a
- Parameters:
  - `<unnamed>` (const benchmark::State &): n/a

#### `void TearDown(const benchmark::State &) override`
- Source: `benchmarks/base/bench_advanced_patterns.cpp`:1816
- Brief: n/a
- Parameters:
  - `<unnamed>` (const benchmark::State &): n/a

### ThemisWithPipe_Txn10

#### `void SetUp(const benchmark::State &) override`
- Source: `benchmarks/base/bench_advanced_patterns.cpp`:1904
- Brief: n/a
- Parameters:
  - `<unnamed>` (const benchmark::State &): n/a

#### `void TearDown(const benchmark::State &) override`
- Source: `benchmarks/base/bench_advanced_patterns.cpp`:1924
- Brief: n/a
- Parameters:
  - `<unnamed>` (const benchmark::State &): n/a

#### `void doTxnChunked(int thread_id, int total_records, int chunk)`
- Source: `benchmarks/base/bench_advanced_patterns.cpp`:1930
- Brief: n/a
- Parameters:
  - `thread_id` (int): n/a
  - `total_records` (int): n/a
  - `chunk` (int): n/a

### WasmLoadBytesFixture

#### `void SetUp(const benchmark::State &) override`
- Source: `benchmarks/base/bench_base_wasm_sandbox.cpp`:69
- Brief: n/a
- Parameters:
  - `<unnamed>` (const benchmark::State &): n/a

#### `void TearDown(const benchmark::State &) override`
- Source: `benchmarks/base/bench_base_wasm_sandbox.cpp`:73
- Brief: n/a
- Parameters:
  - `<unnamed>` (const benchmark::State &): n/a

### bench_advanced_patterns.cpp

#### `Arg(1) -> Arg(4) ->Arg(8) ->Arg(16) ->Arg(32) ->Unit(benchmark::kMillisecond) ->UseRealTime()`
- Source: `benchmarks/base/bench_advanced_patterns.cpp`:3327
- Brief: n/a
- Parameters:
  - `<unnamed>` (1): n/a

#### `Args({1, 1}) -> Args({4, 1}) ->Args({8, 1}) ->Args({16, 1}) ->Args({1, 2}) ->Args({4, 2}) ->Args({8, 2}) ->Args({16, 2}) ->Args({1, 4}) ->Args({4, 4}) ->Args({8, 4}) ->Args({16, 4}) ->Args({1, 8}) ->Args({4, 8}) ->Args({8, 8}) ->Args({16, 8}) ->Unit(benchmark::kMillisecond) ->UseRealTime()`
- Source: `benchmarks/base/bench_advanced_patterns.cpp`:3230
- Brief: n/a
- Parameters:
  - `<unnamed>` ({1, 1}): n/a

#### `BENCHMARK_F(BestPracticeBench, AntiPattern_NewIndex_PerOperation)(benchmark`
- Source: `benchmarks/base/bench_advanced_patterns.cpp`:2929
- Brief: n/a
- Parameters:
  - `<unnamed>` (BestPracticeBench): n/a
  - `<unnamed>` (AntiPattern_NewIndex_PerOperation): n/a

#### `BENCHMARK_F(BestPracticeBench, BestPractice_Batch_1000Items)(benchmark`
- Source: `benchmarks/base/bench_advanced_patterns.cpp`:2966
- Brief: n/a
- Parameters:
  - `<unnamed>` (BestPracticeBench): n/a
  - `<unnamed>` (BestPractice_Batch_1000Items): n/a

#### `BENCHMARK_F(BestPracticeBench, BestPractice_ReuseIndex_Manager)(benchmark`
- Source: `benchmarks/base/bench_advanced_patterns.cpp`:2947
- Brief: n/a
- Parameters:
  - `<unnamed>` (BestPracticeBench): n/a
  - `<unnamed>` (BestPractice_ReuseIndex_Manager): n/a

#### `BENCHMARK_F(GapAnalysisBench, Gap_ConcurrencyScaling_8Threads)(benchmark`
- Source: `benchmarks/base/bench_advanced_patterns.cpp`:3077
- Brief: n/a
- Parameters:
  - `<unnamed>` (GapAnalysisBench): n/a
  - `<unnamed>` (Gap_ConcurrencyScaling_8Threads): n/a
- Details: Concurrent operations efficiency Expected: Near-linear scaling up to CPU cores (then sublinear) Actual: Measure to identify bottlenecks

#### `BENCHMARK_F(ParallelityBench, ParallelInserts_16Threads)(benchmark`
- Source: `benchmarks/base/bench_advanced_patterns.cpp`:2203
- Brief: n/a
- Parameters:
  - `<unnamed>` (ParallelityBench): n/a
  - `<unnamed>` (ParallelInserts_16Threads): n/a

#### `BENCHMARK_F(ParallelityBench, ParallelInserts_1Thread)(benchmark`
- Source: `benchmarks/base/bench_advanced_patterns.cpp`:2158
- Brief: n/a
- Parameters:
  - `<unnamed>` (ParallelityBench): n/a
  - `<unnamed>` (ParallelInserts_1Thread): n/a

#### `BENCHMARK_F(ParallelityBench, ParallelInserts_32Threads)(benchmark`
- Source: `benchmarks/base/bench_advanced_patterns.cpp`:2218
- Brief: n/a
- Parameters:
  - `<unnamed>` (ParallelityBench): n/a
  - `<unnamed>` (ParallelInserts_32Threads): n/a

#### `BENCHMARK_F(ParallelityBench, ParallelInserts_4Threads)(benchmark`
- Source: `benchmarks/base/bench_advanced_patterns.cpp`:2173
- Brief: n/a
- Parameters:
  - `<unnamed>` (ParallelityBench): n/a
  - `<unnamed>` (ParallelInserts_4Threads): n/a

#### `BENCHMARK_F(ParallelityBench, ParallelInserts_8Threads)(benchmark`
- Source: `benchmarks/base/bench_advanced_patterns.cpp`:2188
- Brief: n/a
- Parameters:
  - `<unnamed>` (ParallelityBench): n/a
  - `<unnamed>` (ParallelInserts_8Threads): n/a

#### `BENCHMARK_F(ParallelityBenchBatchWrite, BatchWrite_16Threads)(benchmark`
- Source: `benchmarks/base/bench_advanced_patterns.cpp`:2746
- Brief: n/a
- Parameters:
  - `<unnamed>` (ParallelityBenchBatchWrite): n/a
  - `<unnamed>` (BatchWrite_16Threads): n/a

#### `BENCHMARK_F(ParallelityBenchBatchWrite, BatchWrite_1Thread)(benchmark`
- Source: `benchmarks/base/bench_advanced_patterns.cpp`:2679
- Brief: n/a
- Parameters:
  - `<unnamed>` (ParallelityBenchBatchWrite): n/a
  - `<unnamed>` (BatchWrite_1Thread): n/a

#### `BENCHMARK_F(ParallelityBenchBatchWrite, BatchWrite_32Threads)(benchmark`
- Source: `benchmarks/base/bench_advanced_patterns.cpp`:2768
- Brief: n/a
- Parameters:
  - `<unnamed>` (ParallelityBenchBatchWrite): n/a
  - `<unnamed>` (BatchWrite_32Threads): n/a

#### `BENCHMARK_F(ParallelityBenchBatchWrite, BatchWrite_4Threads)(benchmark`
- Source: `benchmarks/base/bench_advanced_patterns.cpp`:2702
- Brief: n/a
- Parameters:
  - `<unnamed>` (ParallelityBenchBatchWrite): n/a
  - `<unnamed>` (BatchWrite_4Threads): n/a

#### `BENCHMARK_F(ParallelityBenchBatchWrite, BatchWrite_8Threads)(benchmark`
- Source: `benchmarks/base/bench_advanced_patterns.cpp`:2724
- Brief: n/a
- Parameters:
  - `<unnamed>` (ParallelityBenchBatchWrite): n/a
  - `<unnamed>` (BatchWrite_8Threads): n/a

#### `BENCHMARK_F(ParallelityBenchOptimized, OptimizedParallel_16Threads)(benchmark`
- Source: `benchmarks/base/bench_advanced_patterns.cpp`:2334
- Brief: n/a
- Parameters:
  - `<unnamed>` (ParallelityBenchOptimized): n/a
  - `<unnamed>` (OptimizedParallel_16Threads): n/a

#### `BENCHMARK_F(ParallelityBenchOptimized, OptimizedParallel_1Thread)(benchmark`
- Source: `benchmarks/base/bench_advanced_patterns.cpp`:2259
- Brief: n/a
- Parameters:
  - `<unnamed>` (ParallelityBenchOptimized): n/a
  - `<unnamed>` (OptimizedParallel_1Thread): n/a

#### `BENCHMARK_F(ParallelityBenchOptimized, OptimizedParallel_32Threads)(benchmark`
- Source: `benchmarks/base/bench_advanced_patterns.cpp`:2359
- Brief: n/a
- Parameters:
  - `<unnamed>` (ParallelityBenchOptimized): n/a
  - `<unnamed>` (OptimizedParallel_32Threads): n/a

#### `BENCHMARK_F(ParallelityBenchOptimized, OptimizedParallel_4Threads)(benchmark`
- Source: `benchmarks/base/bench_advanced_patterns.cpp`:2284
- Brief: n/a
- Parameters:
  - `<unnamed>` (ParallelityBenchOptimized): n/a
  - `<unnamed>` (OptimizedParallel_4Threads): n/a

#### `BENCHMARK_F(ParallelityBenchOptimized, OptimizedParallel_8Threads)(benchmark`
- Source: `benchmarks/base/bench_advanced_patterns.cpp`:2309
- Brief: n/a
- Parameters:
  - `<unnamed>` (ParallelityBenchOptimized): n/a
  - `<unnamed>` (OptimizedParallel_8Threads): n/a

#### `BENCHMARK_F(ParallelityBenchOptimizedConfig, ConfigOpt_16Threads)(benchmark`
- Source: `benchmarks/base/bench_advanced_patterns.cpp`:2609
- Brief: n/a
- Parameters:
  - `<unnamed>` (ParallelityBenchOptimizedConfig): n/a
  - `<unnamed>` (ConfigOpt_16Threads): n/a

#### `BENCHMARK_F(ParallelityBenchOptimizedConfig, ConfigOpt_1Thread)(benchmark`
- Source: `benchmarks/base/bench_advanced_patterns.cpp`:2555
- Brief: n/a
- Parameters:
  - `<unnamed>` (ParallelityBenchOptimizedConfig): n/a
  - `<unnamed>` (ConfigOpt_1Thread): n/a

#### `BENCHMARK_F(ParallelityBenchOptimizedConfig, ConfigOpt_32Threads)(benchmark`
- Source: `benchmarks/base/bench_advanced_patterns.cpp`:2627
- Brief: n/a
- Parameters:
  - `<unnamed>` (ParallelityBenchOptimizedConfig): n/a
  - `<unnamed>` (ConfigOpt_32Threads): n/a

#### `BENCHMARK_F(ParallelityBenchOptimizedConfig, ConfigOpt_4Threads)(benchmark`
- Source: `benchmarks/base/bench_advanced_patterns.cpp`:2573
- Brief: n/a
- Parameters:
  - `<unnamed>` (ParallelityBenchOptimizedConfig): n/a
  - `<unnamed>` (ConfigOpt_4Threads): n/a

#### `BENCHMARK_F(ParallelityBenchOptimizedConfig, ConfigOpt_8Threads)(benchmark`
- Source: `benchmarks/base/bench_advanced_patterns.cpp`:2591
- Brief: n/a
- Parameters:
  - `<unnamed>` (ParallelityBenchOptimizedConfig): n/a
  - `<unnamed>` (ConfigOpt_8Threads): n/a

#### `BENCHMARK_F(ParallelityBenchPhase1Final, Phase1Final_16Threads)(benchmark`
- Source: `benchmarks/base/bench_advanced_patterns.cpp`:271
- Brief: n/a
- Parameters:
  - `<unnamed>` (ParallelityBenchPhase1Final): n/a
  - `<unnamed>` (Phase1Final_16Threads): n/a

#### `BENCHMARK_F(ParallelityBenchPhase1Final, Phase1Final_1Thread)(benchmark`
- Source: `benchmarks/base/bench_advanced_patterns.cpp`:217
- Brief: n/a
- Parameters:
  - `<unnamed>` (ParallelityBenchPhase1Final): n/a
  - `<unnamed>` (Phase1Final_1Thread): n/a

#### `BENCHMARK_F(ParallelityBenchPhase1Final, Phase1Final_32Threads)(benchmark`
- Source: `benchmarks/base/bench_advanced_patterns.cpp`:289
- Brief: n/a
- Parameters:
  - `<unnamed>` (ParallelityBenchPhase1Final): n/a
  - `<unnamed>` (Phase1Final_32Threads): n/a

#### `BENCHMARK_F(ParallelityBenchPhase1Final, Phase1Final_4Threads)(benchmark`
- Source: `benchmarks/base/bench_advanced_patterns.cpp`:235
- Brief: n/a
- Parameters:
  - `<unnamed>` (ParallelityBenchPhase1Final): n/a
  - `<unnamed>` (Phase1Final_4Threads): n/a

#### `BENCHMARK_F(ParallelityBenchPhase1Final, Phase1Final_8Threads)(benchmark`
- Source: `benchmarks/base/bench_advanced_patterns.cpp`:253
- Brief: n/a
- Parameters:
  - `<unnamed>` (ParallelityBenchPhase1Final): n/a
  - `<unnamed>` (Phase1Final_8Threads): n/a

#### `BENCHMARK_F(ParallelityBenchPhase2Final, Phase2Final_16Threads)(benchmark`
- Source: `benchmarks/base/bench_advanced_patterns.cpp`:401
- Brief: n/a
- Parameters:
  - `<unnamed>` (ParallelityBenchPhase2Final): n/a
  - `<unnamed>` (Phase2Final_16Threads): n/a

#### `BENCHMARK_F(ParallelityBenchPhase2Final, Phase2Final_1Thread)(benchmark`
- Source: `benchmarks/base/bench_advanced_patterns.cpp`:347
- Brief: n/a
- Parameters:
  - `<unnamed>` (ParallelityBenchPhase2Final): n/a
  - `<unnamed>` (Phase2Final_1Thread): n/a

#### `BENCHMARK_F(ParallelityBenchPhase2Final, Phase2Final_32Threads)(benchmark`
- Source: `benchmarks/base/bench_advanced_patterns.cpp`:419
- Brief: n/a
- Parameters:
  - `<unnamed>` (ParallelityBenchPhase2Final): n/a
  - `<unnamed>` (Phase2Final_32Threads): n/a

#### `BENCHMARK_F(ParallelityBenchPhase2Final, Phase2Final_4Threads)(benchmark`
- Source: `benchmarks/base/bench_advanced_patterns.cpp`:365
- Brief: n/a
- Parameters:
  - `<unnamed>` (ParallelityBenchPhase2Final): n/a
  - `<unnamed>` (Phase2Final_4Threads): n/a

#### `BENCHMARK_F(ParallelityBenchPhase2Final, Phase2Final_8Threads)(benchmark`
- Source: `benchmarks/base/bench_advanced_patterns.cpp`:383
- Brief: n/a
- Parameters:
  - `<unnamed>` (ParallelityBenchPhase2Final): n/a
  - `<unnamed>` (Phase2Final_8Threads): n/a

#### `BENCHMARK_F(ParallelityBenchPhase2G, Phase2G_16Threads)(benchmark`
- Source: `benchmarks/base/bench_advanced_patterns.cpp`:646
- Brief: n/a
- Parameters:
  - `<unnamed>` (ParallelityBenchPhase2G): n/a
  - `<unnamed>` (Phase2G_16Threads): n/a

#### `BENCHMARK_F(ParallelityBenchPhase2G, Phase2G_1Thread)(benchmark`
- Source: `benchmarks/base/bench_advanced_patterns.cpp`:613
- Brief: n/a
- Parameters:
  - `<unnamed>` (ParallelityBenchPhase2G): n/a
  - `<unnamed>` (Phase2G_1Thread): n/a

#### `BENCHMARK_F(ParallelityBenchPhase2G, Phase2G_32Threads)(benchmark`
- Source: `benchmarks/base/bench_advanced_patterns.cpp`:657
- Brief: n/a
- Parameters:
  - `<unnamed>` (ParallelityBenchPhase2G): n/a
  - `<unnamed>` (Phase2G_32Threads): n/a

#### `BENCHMARK_F(ParallelityBenchPhase2G, Phase2G_4Threads)(benchmark`
- Source: `benchmarks/base/bench_advanced_patterns.cpp`:624
- Brief: n/a
- Parameters:
  - `<unnamed>` (ParallelityBenchPhase2G): n/a
  - `<unnamed>` (Phase2G_4Threads): n/a

#### `BENCHMARK_F(ParallelityBenchPhase2G, Phase2G_8Threads)(benchmark`
- Source: `benchmarks/base/bench_advanced_patterns.cpp`:635
- Brief: n/a
- Parameters:
  - `<unnamed>` (ParallelityBenchPhase2G): n/a
  - `<unnamed>` (Phase2G_8Threads): n/a

#### `BENCHMARK_F(ParallelityBenchPhase2G_2H, Phase2G2H_16Threads)(benchmark`
- Source: `benchmarks/base/bench_advanced_patterns.cpp`:591
- Brief: n/a
- Parameters:
  - `<unnamed>` (ParallelityBenchPhase2G_2H): n/a
  - `<unnamed>` (Phase2G2H_16Threads): n/a

#### `BENCHMARK_F(ParallelityBenchPhase2G_2H, Phase2G2H_1Thread)(benchmark`
- Source: `benchmarks/base/bench_advanced_patterns.cpp`:558
- Brief: n/a
- Parameters:
  - `<unnamed>` (ParallelityBenchPhase2G_2H): n/a
  - `<unnamed>` (Phase2G2H_1Thread): n/a

#### `BENCHMARK_F(ParallelityBenchPhase2G_2H, Phase2G2H_32Threads)(benchmark`
- Source: `benchmarks/base/bench_advanced_patterns.cpp`:602
- Brief: n/a
- Parameters:
  - `<unnamed>` (ParallelityBenchPhase2G_2H): n/a
  - `<unnamed>` (Phase2G2H_32Threads): n/a

#### `BENCHMARK_F(ParallelityBenchPhase2G_2H, Phase2G2H_4Threads)(benchmark`
- Source: `benchmarks/base/bench_advanced_patterns.cpp`:569
- Brief: n/a
- Parameters:
  - `<unnamed>` (ParallelityBenchPhase2G_2H): n/a
  - `<unnamed>` (Phase2G2H_4Threads): n/a

#### `BENCHMARK_F(ParallelityBenchPhase2G_2H, Phase2G2H_8Threads)(benchmark`
- Source: `benchmarks/base/bench_advanced_patterns.cpp`:580
- Brief: n/a
- Parameters:
  - `<unnamed>` (ParallelityBenchPhase2G_2H): n/a
  - `<unnamed>` (Phase2G2H_8Threads): n/a

#### `BENCHMARK_F(ParallelityBenchPhase2G_NonTxn, Phase2GNTX_16Threads)(benchmark`
- Source: `benchmarks/base/bench_advanced_patterns.cpp`:760
- Brief: n/a
- Parameters:
  - `<unnamed>` (ParallelityBenchPhase2G_NonTxn): n/a
  - `<unnamed>` (Phase2GNTX_16Threads): n/a

#### `BENCHMARK_F(ParallelityBenchPhase2G_NonTxn, Phase2GNTX_1Thread)(benchmark`
- Source: `benchmarks/base/bench_advanced_patterns.cpp`:706
- Brief: n/a
- Parameters:
  - `<unnamed>` (ParallelityBenchPhase2G_NonTxn): n/a
  - `<unnamed>` (Phase2GNTX_1Thread): n/a

#### `BENCHMARK_F(ParallelityBenchPhase2G_NonTxn, Phase2GNTX_32Threads)(benchmark`
- Source: `benchmarks/base/bench_advanced_patterns.cpp`:778
- Brief: n/a
- Parameters:
  - `<unnamed>` (ParallelityBenchPhase2G_NonTxn): n/a
  - `<unnamed>` (Phase2GNTX_32Threads): n/a

#### `BENCHMARK_F(ParallelityBenchPhase2G_NonTxn, Phase2GNTX_4Threads)(benchmark`
- Source: `benchmarks/base/bench_advanced_patterns.cpp`:724
- Brief: n/a
- Parameters:
  - `<unnamed>` (ParallelityBenchPhase2G_NonTxn): n/a
  - `<unnamed>` (Phase2GNTX_4Threads): n/a

#### `BENCHMARK_F(ParallelityBenchPhase2G_NonTxn, Phase2GNTX_8Threads)(benchmark`
- Source: `benchmarks/base/bench_advanced_patterns.cpp`:742
- Brief: n/a
- Parameters:
  - `<unnamed>` (ParallelityBenchPhase2G_NonTxn): n/a
  - `<unnamed>` (Phase2GNTX_8Threads): n/a

#### `BENCHMARK_F(ParallelityBenchPhase2G_Txn10, Phase2G_Txn10_16Threads)(benchmark`
- Source: `benchmarks/base/bench_advanced_patterns.cpp`:1016
- Brief: n/a
- Parameters:
  - `<unnamed>` (ParallelityBenchPhase2G_Txn10): n/a
  - `<unnamed>` (Phase2G_Txn10_16Threads): n/a

#### `BENCHMARK_F(ParallelityBenchPhase2G_Txn10, Phase2G_Txn10_1Thread)(benchmark`
- Source: `benchmarks/base/bench_advanced_patterns.cpp`:983
- Brief: n/a
- Parameters:
  - `<unnamed>` (ParallelityBenchPhase2G_Txn10): n/a
  - `<unnamed>` (Phase2G_Txn10_1Thread): n/a

#### `BENCHMARK_F(ParallelityBenchPhase2G_Txn10, Phase2G_Txn10_32Threads)(benchmark`
- Source: `benchmarks/base/bench_advanced_patterns.cpp`:1027
- Brief: n/a
- Parameters:
  - `<unnamed>` (ParallelityBenchPhase2G_Txn10): n/a
  - `<unnamed>` (Phase2G_Txn10_32Threads): n/a

#### `BENCHMARK_F(ParallelityBenchPhase2G_Txn10, Phase2G_Txn10_4Threads)(benchmark`
- Source: `benchmarks/base/bench_advanced_patterns.cpp`:994
- Brief: n/a
- Parameters:
  - `<unnamed>` (ParallelityBenchPhase2G_Txn10): n/a
  - `<unnamed>` (Phase2G_Txn10_4Threads): n/a

#### `BENCHMARK_F(ParallelityBenchPhase2G_Txn10, Phase2G_Txn10_8Threads)(benchmark`
- Source: `benchmarks/base/bench_advanced_patterns.cpp`:1005
- Brief: n/a
- Parameters:
  - `<unnamed>` (ParallelityBenchPhase2G_Txn10): n/a
  - `<unnamed>` (Phase2G_Txn10_8Threads): n/a

#### `BENCHMARK_F(ParallelityBenchPhase2G_Txn10_DualQueue, Phase2G_Txn10DQ_16Threads)(benchmark`
- Source: `benchmarks/base/bench_advanced_patterns.cpp`:1244
- Brief: n/a
- Parameters:
  - `<unnamed>` (ParallelityBenchPhase2G_Txn10_DualQueue): n/a
  - `<unnamed>` (Phase2G_Txn10DQ_16Threads): n/a

#### `BENCHMARK_F(ParallelityBenchPhase2G_Txn10_DualQueue, Phase2G_Txn10DQ_1Thread)(benchmark`
- Source: `benchmarks/base/bench_advanced_patterns.cpp`:1211
- Brief: n/a
- Parameters:
  - `<unnamed>` (ParallelityBenchPhase2G_Txn10_DualQueue): n/a
  - `<unnamed>` (Phase2G_Txn10DQ_1Thread): n/a

#### `BENCHMARK_F(ParallelityBenchPhase2G_Txn10_DualQueue, Phase2G_Txn10DQ_32Threads)(benchmark`
- Source: `benchmarks/base/bench_advanced_patterns.cpp`:1255
- Brief: n/a
- Parameters:
  - `<unnamed>` (ParallelityBenchPhase2G_Txn10_DualQueue): n/a
  - `<unnamed>` (Phase2G_Txn10DQ_32Threads): n/a

#### `BENCHMARK_F(ParallelityBenchPhase2G_Txn10_DualQueue, Phase2G_Txn10DQ_4Threads)(benchmark`
- Source: `benchmarks/base/bench_advanced_patterns.cpp`:1222
- Brief: n/a
- Parameters:
  - `<unnamed>` (ParallelityBenchPhase2G_Txn10_DualQueue): n/a
  - `<unnamed>` (Phase2G_Txn10DQ_4Threads): n/a

#### `BENCHMARK_F(ParallelityBenchPhase2G_Txn10_DualQueue, Phase2G_Txn10DQ_8Threads)(benchmark`
- Source: `benchmarks/base/bench_advanced_patterns.cpp`:1233
- Brief: n/a
- Parameters:
  - `<unnamed>` (ParallelityBenchPhase2G_Txn10_DualQueue): n/a
  - `<unnamed>` (Phase2G_Txn10DQ_8Threads): n/a

#### `BENCHMARK_F(ParallelityBenchPhase2G_Txn5, Phase2G_Txn5_16Threads)(benchmark`
- Source: `benchmarks/base/bench_advanced_patterns.cpp`:1130
- Brief: n/a
- Parameters:
  - `<unnamed>` (ParallelityBenchPhase2G_Txn5): n/a
  - `<unnamed>` (Phase2G_Txn5_16Threads): n/a

#### `BENCHMARK_F(ParallelityBenchPhase2G_Txn5, Phase2G_Txn5_1Thread)(benchmark`
- Source: `benchmarks/base/bench_advanced_patterns.cpp`:1097
- Brief: n/a
- Parameters:
  - `<unnamed>` (ParallelityBenchPhase2G_Txn5): n/a
  - `<unnamed>` (Phase2G_Txn5_1Thread): n/a

#### `BENCHMARK_F(ParallelityBenchPhase2G_Txn5, Phase2G_Txn5_32Threads)(benchmark`
- Source: `benchmarks/base/bench_advanced_patterns.cpp`:1141
- Brief: n/a
- Parameters:
  - `<unnamed>` (ParallelityBenchPhase2G_Txn5): n/a
  - `<unnamed>` (Phase2G_Txn5_32Threads): n/a

#### `BENCHMARK_F(ParallelityBenchPhase2G_Txn5, Phase2G_Txn5_4Threads)(benchmark`
- Source: `benchmarks/base/bench_advanced_patterns.cpp`:1108
- Brief: n/a
- Parameters:
  - `<unnamed>` (ParallelityBenchPhase2G_Txn5): n/a
  - `<unnamed>` (Phase2G_Txn5_4Threads): n/a

#### `BENCHMARK_F(ParallelityBenchPhase2G_Txn5, Phase2G_Txn5_8Threads)(benchmark`
- Source: `benchmarks/base/bench_advanced_patterns.cpp`:1119
- Brief: n/a
- Parameters:
  - `<unnamed>` (ParallelityBenchPhase2G_Txn5): n/a
  - `<unnamed>` (Phase2G_Txn5_8Threads): n/a

#### `BENCHMARK_F(ParallelityBenchPhase2G_Unprepared_NonTxn, Phase2GUNTX_16Threads)(benchmark`
- Source: `benchmarks/base/bench_advanced_patterns.cpp`:888
- Brief: n/a
- Parameters:
  - `<unnamed>` (ParallelityBenchPhase2G_Unprepared_NonTxn): n/a
  - `<unnamed>` (Phase2GUNTX_16Threads): n/a

#### `BENCHMARK_F(ParallelityBenchPhase2G_Unprepared_NonTxn, Phase2GUNTX_1Thread)(benchmark`
- Source: `benchmarks/base/bench_advanced_patterns.cpp`:834
- Brief: n/a
- Parameters:
  - `<unnamed>` (ParallelityBenchPhase2G_Unprepared_NonTxn): n/a
  - `<unnamed>` (Phase2GUNTX_1Thread): n/a

#### `BENCHMARK_F(ParallelityBenchPhase2G_Unprepared_NonTxn, Phase2GUNTX_32Threads)(benchmark`
- Source: `benchmarks/base/bench_advanced_patterns.cpp`:906
- Brief: n/a
- Parameters:
  - `<unnamed>` (ParallelityBenchPhase2G_Unprepared_NonTxn): n/a
  - `<unnamed>` (Phase2GUNTX_32Threads): n/a

#### `BENCHMARK_F(ParallelityBenchPhase2G_Unprepared_NonTxn, Phase2GUNTX_4Threads)(benchmark`
- Source: `benchmarks/base/bench_advanced_patterns.cpp`:852
- Brief: n/a
- Parameters:
  - `<unnamed>` (ParallelityBenchPhase2G_Unprepared_NonTxn): n/a
  - `<unnamed>` (Phase2GUNTX_4Threads): n/a

#### `BENCHMARK_F(ParallelityBenchPhase2G_Unprepared_NonTxn, Phase2GUNTX_8Threads)(benchmark`
- Source: `benchmarks/base/bench_advanced_patterns.cpp`:870
- Brief: n/a
- Parameters:
  - `<unnamed>` (ParallelityBenchPhase2G_Unprepared_NonTxn): n/a
  - `<unnamed>` (Phase2GUNTX_8Threads): n/a

#### `BENCHMARK_F(ParallelityBenchSharded, ShardedParallel_16Threads)(benchmark`
- Source: `benchmarks/base/bench_advanced_patterns.cpp`:2480
- Brief: n/a
- Parameters:
  - `<unnamed>` (ParallelityBenchSharded): n/a
  - `<unnamed>` (ShardedParallel_16Threads): n/a

#### `BENCHMARK_F(ParallelityBenchSharded, ShardedParallel_1Thread)(benchmark`
- Source: `benchmarks/base/bench_advanced_patterns.cpp`:2417
- Brief: n/a
- Parameters:
  - `<unnamed>` (ParallelityBenchSharded): n/a
  - `<unnamed>` (ShardedParallel_1Thread): n/a

#### `BENCHMARK_F(ParallelityBenchSharded, ShardedParallel_32Threads)(benchmark`
- Source: `benchmarks/base/bench_advanced_patterns.cpp`:2501
- Brief: n/a
- Parameters:
  - `<unnamed>` (ParallelityBenchSharded): n/a
  - `<unnamed>` (ShardedParallel_32Threads): n/a

#### `BENCHMARK_F(ParallelityBenchSharded, ShardedParallel_4Threads)(benchmark`
- Source: `benchmarks/base/bench_advanced_patterns.cpp`:2438
- Brief: n/a
- Parameters:
  - `<unnamed>` (ParallelityBenchSharded): n/a
  - `<unnamed>` (ShardedParallel_4Threads): n/a

#### `BENCHMARK_F(ParallelityBenchSharded, ShardedParallel_8Threads)(benchmark`
- Source: `benchmarks/base/bench_advanced_patterns.cpp`:2459
- Brief: n/a
- Parameters:
  - `<unnamed>` (ParallelityBenchSharded): n/a
  - `<unnamed>` (ShardedParallel_8Threads): n/a

#### `BENCHMARK_F(ReadWriteRatioBench, Balanced_50W_50R)(benchmark`
- Source: `benchmarks/base/bench_advanced_patterns.cpp`:2061
- Brief: n/a
- Parameters:
  - `<unnamed>` (ReadWriteRatioBench): n/a
  - `<unnamed>` (Balanced_50W_50R): n/a

#### `BENCHMARK_F(ReadWriteRatioBench, ReadHeavy_20W_80R)(benchmark`
- Source: `benchmarks/base/bench_advanced_patterns.cpp`:2080
- Brief: n/a
- Parameters:
  - `<unnamed>` (ReadWriteRatioBench): n/a
  - `<unnamed>` (ReadHeavy_20W_80R): n/a

#### `BENCHMARK_F(ReadWriteRatioBench, ReadOnly_0W_100R)(benchmark`
- Source: `benchmarks/base/bench_advanced_patterns.cpp`:2099
- Brief: n/a
- Parameters:
  - `<unnamed>` (ReadWriteRatioBench): n/a
  - `<unnamed>` (ReadOnly_0W_100R): n/a

#### `BENCHMARK_F(ReadWriteRatioBench, WriteHeavy_80W_20R)(benchmark`
- Source: `benchmarks/base/bench_advanced_patterns.cpp`:2042
- Brief: n/a
- Parameters:
  - `<unnamed>` (ReadWriteRatioBench): n/a
  - `<unnamed>` (WriteHeavy_80W_20R): n/a

#### `BENCHMARK_F(ReadWriteRatioBench, WriteOnly_100W_0R)(benchmark`
- Source: `benchmarks/base/bench_advanced_patterns.cpp`:2112
- Brief: n/a
- Parameters:
  - `<unnamed>` (ReadWriteRatioBench): n/a
  - `<unnamed>` (WriteOnly_100W_0R): n/a

#### `BENCHMARK_F(RocksDBRaw_NonTxn, RocksRawNTX_16Threads)(benchmark`
- Source: `benchmarks/base/bench_advanced_patterns.cpp`:1380
- Brief: n/a
- Parameters:
  - `<unnamed>` (RocksDBRaw_NonTxn): n/a
  - `<unnamed>` (RocksRawNTX_16Threads): n/a

#### `BENCHMARK_F(RocksDBRaw_NonTxn, RocksRawNTX_1Thread)(benchmark`
- Source: `benchmarks/base/bench_advanced_patterns.cpp`:1308
- Brief: n/a
- Parameters:
  - `<unnamed>` (RocksDBRaw_NonTxn): n/a
  - `<unnamed>` (RocksRawNTX_1Thread): n/a

#### `BENCHMARK_F(RocksDBRaw_NonTxn, RocksRawNTX_32Threads)(benchmark`
- Source: `benchmarks/base/bench_advanced_patterns.cpp`:1404
- Brief: n/a
- Parameters:
  - `<unnamed>` (RocksDBRaw_NonTxn): n/a
  - `<unnamed>` (RocksRawNTX_32Threads): n/a

#### `BENCHMARK_F(RocksDBRaw_NonTxn, RocksRawNTX_4Threads)(benchmark`
- Source: `benchmarks/base/bench_advanced_patterns.cpp`:1332
- Brief: n/a
- Parameters:
  - `<unnamed>` (RocksDBRaw_NonTxn): n/a
  - `<unnamed>` (RocksRawNTX_4Threads): n/a

#### `BENCHMARK_F(RocksDBRaw_NonTxn, RocksRawNTX_8Threads)(benchmark`
- Source: `benchmarks/base/bench_advanced_patterns.cpp`:1356
- Brief: n/a
- Parameters:
  - `<unnamed>` (RocksDBRaw_NonTxn): n/a
  - `<unnamed>` (RocksRawNTX_8Threads): n/a

#### `BENCHMARK_F(RocksDBRaw_Txn10, RocksRawTxn10_16Threads)(benchmark`
- Source: `benchmarks/base/bench_advanced_patterns.cpp`:1554
- Brief: n/a
- Parameters:
  - `<unnamed>` (RocksDBRaw_Txn10): n/a
  - `<unnamed>` (RocksRawTxn10_16Threads): n/a

#### `BENCHMARK_F(RocksDBRaw_Txn10, RocksRawTxn10_1Thread)(benchmark`
- Source: `benchmarks/base/bench_advanced_patterns.cpp`:1512
- Brief: n/a
- Parameters:
  - `<unnamed>` (RocksDBRaw_Txn10): n/a
  - `<unnamed>` (RocksRawTxn10_1Thread): n/a

#### `BENCHMARK_F(RocksDBRaw_Txn10, RocksRawTxn10_32Threads)(benchmark`
- Source: `benchmarks/base/bench_advanced_patterns.cpp`:1568
- Brief: n/a
- Parameters:
  - `<unnamed>` (RocksDBRaw_Txn10): n/a
  - `<unnamed>` (RocksRawTxn10_32Threads): n/a

#### `BENCHMARK_F(RocksDBRaw_Txn10, RocksRawTxn10_4Threads)(benchmark`
- Source: `benchmarks/base/bench_advanced_patterns.cpp`:1526
- Brief: n/a
- Parameters:
  - `<unnamed>` (RocksDBRaw_Txn10): n/a
  - `<unnamed>` (RocksRawTxn10_4Threads): n/a

#### `BENCHMARK_F(RocksDBRaw_Txn10, RocksRawTxn10_8Threads)(benchmark`
- Source: `benchmarks/base/bench_advanced_patterns.cpp`:1540
- Brief: n/a
- Parameters:
  - `<unnamed>` (RocksDBRaw_Txn10): n/a
  - `<unnamed>` (RocksRawTxn10_8Threads): n/a

#### `BENCHMARK_F(SelfProtectionBench, BurstLoad_NormalThen10xSpike)(benchmark`
- Source: `benchmarks/base/bench_advanced_patterns.cpp`:2849
- Brief: n/a
- Parameters:
  - `<unnamed>` (SelfProtectionBench): n/a
  - `<unnamed>` (BurstLoad_NormalThen10xSpike): n/a

#### `BENCHMARK_F(SelfProtectionBench, ConcurrentConnections_32Threads)(benchmark`
- Source: `benchmarks/base/bench_advanced_patterns.cpp`:2875
- Brief: n/a
- Parameters:
  - `<unnamed>` (SelfProtectionBench): n/a
  - `<unnamed>` (ConcurrentConnections_32Threads): n/a

#### `BENCHMARK_F(SelfProtectionBench, MemoryPressure_100KB_Documents)(benchmark`
- Source: `benchmarks/base/bench_advanced_patterns.cpp`:2892
- Brief: n/a
- Parameters:
  - `<unnamed>` (SelfProtectionBench): n/a
  - `<unnamed>` (MemoryPressure_100KB_Documents): n/a

#### `BENCHMARK_F(SelfProtectionBench, SustainedLoad_70W_30R)(benchmark`
- Source: `benchmarks/base/bench_advanced_patterns.cpp`:2824
- Brief: n/a
- Parameters:
  - `<unnamed>` (SelfProtectionBench): n/a
  - `<unnamed>` (SustainedLoad_70W_30R): n/a

#### `BENCHMARK_F(ThemisNoPipe_NonTxn, ThemisNoPipe_NTX_16Threads)(benchmark`
- Source: `benchmarks/base/bench_advanced_patterns.cpp`:1664
- Brief: n/a
- Parameters:
  - `<unnamed>` (ThemisNoPipe_NonTxn): n/a
  - `<unnamed>` (ThemisNoPipe_NTX_16Threads): n/a

#### `BENCHMARK_F(ThemisNoPipe_NonTxn, ThemisNoPipe_NTX_1Thread)(benchmark`
- Source: `benchmarks/base/bench_advanced_patterns.cpp`:1619
- Brief: n/a
- Parameters:
  - `<unnamed>` (ThemisNoPipe_NonTxn): n/a
  - `<unnamed>` (ThemisNoPipe_NTX_1Thread): n/a

#### `BENCHMARK_F(ThemisNoPipe_NonTxn, ThemisNoPipe_NTX_32Threads)(benchmark`
- Source: `benchmarks/base/bench_advanced_patterns.cpp`:1679
- Brief: n/a
- Parameters:
  - `<unnamed>` (ThemisNoPipe_NonTxn): n/a
  - `<unnamed>` (ThemisNoPipe_NTX_32Threads): n/a

#### `BENCHMARK_F(ThemisNoPipe_NonTxn, ThemisNoPipe_NTX_4Threads)(benchmark`
- Source: `benchmarks/base/bench_advanced_patterns.cpp`:1634
- Brief: n/a
- Parameters:
  - `<unnamed>` (ThemisNoPipe_NonTxn): n/a
  - `<unnamed>` (ThemisNoPipe_NTX_4Threads): n/a

#### `BENCHMARK_F(ThemisNoPipe_NonTxn, ThemisNoPipe_NTX_8Threads)(benchmark`
- Source: `benchmarks/base/bench_advanced_patterns.cpp`:1649
- Brief: n/a
- Parameters:
  - `<unnamed>` (ThemisNoPipe_NonTxn): n/a
  - `<unnamed>` (ThemisNoPipe_NTX_8Threads): n/a

#### `BENCHMARK_F(ThemisNoPipe_Txn10, ThemisNoPipe_Txn10_16Threads)(benchmark`
- Source: `benchmarks/base/bench_advanced_patterns.cpp`:1769
- Brief: n/a
- Parameters:
  - `<unnamed>` (ThemisNoPipe_Txn10): n/a
  - `<unnamed>` (ThemisNoPipe_Txn10_16Threads): n/a

#### `BENCHMARK_F(ThemisNoPipe_Txn10, ThemisNoPipe_Txn10_1Thread)(benchmark`
- Source: `benchmarks/base/bench_advanced_patterns.cpp`:1745
- Brief: n/a
- Parameters:
  - `<unnamed>` (ThemisNoPipe_Txn10): n/a
  - `<unnamed>` (ThemisNoPipe_Txn10_1Thread): n/a

#### `BENCHMARK_F(ThemisNoPipe_Txn10, ThemisNoPipe_Txn10_32Threads)(benchmark`
- Source: `benchmarks/base/bench_advanced_patterns.cpp`:1777
- Brief: n/a
- Parameters:
  - `<unnamed>` (ThemisNoPipe_Txn10): n/a
  - `<unnamed>` (ThemisNoPipe_Txn10_32Threads): n/a

#### `BENCHMARK_F(ThemisNoPipe_Txn10, ThemisNoPipe_Txn10_4Threads)(benchmark`
- Source: `benchmarks/base/bench_advanced_patterns.cpp`:1753
- Brief: n/a
- Parameters:
  - `<unnamed>` (ThemisNoPipe_Txn10): n/a
  - `<unnamed>` (ThemisNoPipe_Txn10_4Threads): n/a

#### `BENCHMARK_F(ThemisNoPipe_Txn10, ThemisNoPipe_Txn10_8Threads)(benchmark`
- Source: `benchmarks/base/bench_advanced_patterns.cpp`:1761
- Brief: n/a
- Parameters:
  - `<unnamed>` (ThemisNoPipe_Txn10): n/a
  - `<unnamed>` (ThemisNoPipe_Txn10_8Threads): n/a

#### `BENCHMARK_F(ThemisWithPipe_NonTxn, ThemisWithPipe_NTX_16Threads)(benchmark`
- Source: `benchmarks/base/bench_advanced_patterns.cpp`:1868
- Brief: n/a
- Parameters:
  - `<unnamed>` (ThemisWithPipe_NonTxn): n/a
  - `<unnamed>` (ThemisWithPipe_NTX_16Threads): n/a

#### `BENCHMARK_F(ThemisWithPipe_NonTxn, ThemisWithPipe_NTX_1Thread)(benchmark`
- Source: `benchmarks/base/bench_advanced_patterns.cpp`:1823
- Brief: n/a
- Parameters:
  - `<unnamed>` (ThemisWithPipe_NonTxn): n/a
  - `<unnamed>` (ThemisWithPipe_NTX_1Thread): n/a

#### `BENCHMARK_F(ThemisWithPipe_NonTxn, ThemisWithPipe_NTX_32Threads)(benchmark`
- Source: `benchmarks/base/bench_advanced_patterns.cpp`:1883
- Brief: n/a
- Parameters:
  - `<unnamed>` (ThemisWithPipe_NonTxn): n/a
  - `<unnamed>` (ThemisWithPipe_NTX_32Threads): n/a

#### `BENCHMARK_F(ThemisWithPipe_NonTxn, ThemisWithPipe_NTX_4Threads)(benchmark`
- Source: `benchmarks/base/bench_advanced_patterns.cpp`:1838
- Brief: n/a
- Parameters:
  - `<unnamed>` (ThemisWithPipe_NonTxn): n/a
  - `<unnamed>` (ThemisWithPipe_NTX_4Threads): n/a

#### `BENCHMARK_F(ThemisWithPipe_NonTxn, ThemisWithPipe_NTX_8Threads)(benchmark`
- Source: `benchmarks/base/bench_advanced_patterns.cpp`:1853
- Brief: n/a
- Parameters:
  - `<unnamed>` (ThemisWithPipe_NonTxn): n/a
  - `<unnamed>` (ThemisWithPipe_NTX_8Threads): n/a

#### `BENCHMARK_F(ThemisWithPipe_Txn10, ThemisWithPipe_Txn10_16Threads)(benchmark`
- Source: `benchmarks/base/bench_advanced_patterns.cpp`:1973
- Brief: n/a
- Parameters:
  - `<unnamed>` (ThemisWithPipe_Txn10): n/a
  - `<unnamed>` (ThemisWithPipe_Txn10_16Threads): n/a

#### `BENCHMARK_F(ThemisWithPipe_Txn10, ThemisWithPipe_Txn10_1Thread)(benchmark`
- Source: `benchmarks/base/bench_advanced_patterns.cpp`:1949
- Brief: n/a
- Parameters:
  - `<unnamed>` (ThemisWithPipe_Txn10): n/a
  - `<unnamed>` (ThemisWithPipe_Txn10_1Thread): n/a

#### `BENCHMARK_F(ThemisWithPipe_Txn10, ThemisWithPipe_Txn10_32Threads)(benchmark`
- Source: `benchmarks/base/bench_advanced_patterns.cpp`:1981
- Brief: n/a
- Parameters:
  - `<unnamed>` (ThemisWithPipe_Txn10): n/a
  - `<unnamed>` (ThemisWithPipe_Txn10_32Threads): n/a

#### `BENCHMARK_F(ThemisWithPipe_Txn10, ThemisWithPipe_Txn10_4Threads)(benchmark`
- Source: `benchmarks/base/bench_advanced_patterns.cpp`:1957
- Brief: n/a
- Parameters:
  - `<unnamed>` (ThemisWithPipe_Txn10): n/a
  - `<unnamed>` (ThemisWithPipe_Txn10_4Threads): n/a

#### `BENCHMARK_F(ThemisWithPipe_Txn10, ThemisWithPipe_Txn10_8Threads)(benchmark`
- Source: `benchmarks/base/bench_advanced_patterns.cpp`:1965
- Brief: n/a
- Parameters:
  - `<unnamed>` (ThemisWithPipe_Txn10): n/a
  - `<unnamed>` (ThemisWithPipe_Txn10_8Threads): n/a

#### `BENCHMARK_MAIN()`
- Source: `benchmarks/base/bench_advanced_patterns.cpp`:3333
- Brief: n/a
- Parameters: none

#### `void BM_Phase2H_BgThreads(benchmark::State &state)`
- Source: `benchmarks/base/bench_advanced_patterns.cpp`:3155
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_Phase2H_FullOptimized(benchmark::State &state)`
- Source: `benchmarks/base/bench_advanced_patterns.cpp`:3241
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `state SetItemsProcessed(state.iterations() *100 *5)`
- Source: `benchmarks/base/bench_advanced_patterns.cpp`:3129
- Brief: n/a
- Parameters:
  - `5` (state.iterations() *100 *): n/a

#### `state SetItemsProcessed(state.iterations() *1000)`
- Source: `benchmarks/base/bench_advanced_patterns.cpp`:3034
- Brief: n/a
- Parameters:
  - `1000` (state.iterations() *): n/a

#### `state SetItemsProcessed(state.iterations())`
- Source: `benchmarks/base/bench_advanced_patterns.cpp`:3147
- Brief: n/a
- Parameters:
  - `iterations` (state.): n/a

#### `sim createIndex("random_access", "id")`
- Source: `benchmarks/base/bench_advanced_patterns.cpp`:3045
- Brief: n/a
- Parameters:
  - `<unnamed>` ("random_access"): n/a
  - `<unnamed>` ("id"): n/a

#### `sim createIndex("rocksdb_baseline", "id")`
- Source: `benchmarks/base/bench_advanced_patterns.cpp`:3020
- Brief: n/a
- Parameters:
  - `<unnamed>` ("rocksdb_baseline"): n/a
  - `<unnamed>` ("id"): n/a

#### `sim createIndex("transaction_test", "id")`
- Source: `benchmarks/base/bench_advanced_patterns.cpp`:3113
- Brief: n/a
- Parameters:
  - `<unnamed>` ("transaction_test"): n/a
  - `<unnamed>` ("id"): n/a

#### `std::uniform_int_distribution< int > dis(0, 9999)`
- Source: `benchmarks/base/bench_advanced_patterns.cpp`:3056
- Brief: n/a
- Parameters:
  - `<unnamed>` (0): n/a
  - `<unnamed>` (9999): n/a

#### `for(auto _ :state)`
- Source: `benchmarks/base/bench_advanced_patterns.cpp`:3022
- Brief: n/a
- Parameters:
  - `<unnamed>` (auto _ :state): n/a

#### `for(int i=0;i< 10000;++i)`
- Source: `benchmarks/base/bench_advanced_patterns.cpp`:3048
- Brief: n/a
- Parameters: none

#### `sim reset()`
- Source: `benchmarks/base/bench_advanced_patterns.cpp`:3032
- Brief: n/a
- Parameters: none

#### `std::mt19937 rng(std::random_device{}())`
- Source: `benchmarks/base/bench_advanced_patterns.cpp`:3057
- Brief: n/a
- Parameters:
  - `<unnamed>` (std::random_device{}): n/a

### bench_arm_memory.cpp

#### `Arg(4 *1024) -> Arg(32 *1024) ->Arg(256 *1024) ->Arg(1024 *1024) ->Unit(benchmark::kMicrosecond)`
- Source: `benchmarks/base/bench_arm_memory.cpp`:222
- Brief: n/a
- Parameters:
  - `1024` (4 *): n/a

#### `Args({32 *1024, 1}) -> Args({32 *1024, 2}) ->Args({32 *1024, 4}) ->Args({32 *1024, 8}) ->Args({32 *1024, 16}) ->Unit(benchmark::kMicrosecond)`
- Source: `benchmarks/base/bench_arm_memory.cpp`:251
- Brief: n/a
- Parameters:
  - `<unnamed>` ({32 *1024, 1}): n/a

#### `BENCHMARK(BM_ARM_CacheLine_Aligned) -> Unit(benchmark::kMicrosecond)`
- Source: `benchmarks/base/bench_arm_memory.cpp`:273
- Brief: n/a
- Parameters:
  - `<unnamed>` (BM_ARM_CacheLine_Aligned): n/a

#### `BENCHMARK(BM_ARM_CacheLine_Unaligned) -> Unit(benchmark::kMicrosecond)`
- Source: `benchmarks/base/bench_arm_memory.cpp`:274
- Brief: n/a
- Parameters:
  - `<unnamed>` (BM_ARM_CacheLine_Unaligned): n/a

#### `BENCHMARK_MAIN()`
- Source: `benchmarks/base/bench_arm_memory.cpp`:276
- Brief: n/a
- Parameters: none

#### `void BM_ARM_CacheLine_Aligned(benchmark::State &state)`
- Source: `benchmarks/base/bench_arm_memory.cpp`:180
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_ARM_CacheLine_Unaligned(benchmark::State &state)`
- Source: `benchmarks/base/bench_arm_memory.cpp`:196
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_ARM_MemCopy_Builtin(benchmark::State &state)`
- Source: `benchmarks/base/bench_arm_memory.cpp`:136
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_ARM_MemCopy_Loop(benchmark::State &state)`
- Source: `benchmarks/base/bench_arm_memory.cpp`:155
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_ARM_Random_Read(benchmark::State &state)`
- Source: `benchmarks/base/bench_arm_memory.cpp`:75
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_ARM_Random_Write(benchmark::State &state)`
- Source: `benchmarks/base/bench_arm_memory.cpp`:93
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_ARM_Sequential_Read(benchmark::State &state)`
- Source: `benchmarks/base/bench_arm_memory.cpp`:30
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_ARM_Sequential_Write(benchmark::State &state)`
- Source: `benchmarks/base/bench_arm_memory.cpp`:51
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_ARM_Strided_Read(benchmark::State &state)`
- Source: `benchmarks/base/bench_arm_memory.cpp`:114
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

### bench_arm_simd.cpp

#### `BENCHMARK(BM_ARM_Batch_L2_SIMD) -> DenseRange(128, 768, 128) ->Unit(benchmark::kMillisecond)`
- Source: `benchmarks/base/bench_arm_simd.cpp`:301
- Brief: n/a
- Parameters:
  - `<unnamed>` (BM_ARM_Batch_L2_SIMD): n/a

#### `BENCHMARK(BM_ARM_Batch_L2_Scalar) -> DenseRange(128, 768, 128) ->Unit(benchmark::kMillisecond)`
- Source: `benchmarks/base/bench_arm_simd.cpp`:302
- Brief: n/a
- Parameters:
  - `<unnamed>` (BM_ARM_Batch_L2_Scalar): n/a

#### `BENCHMARK(BM_ARM_DotProduct_Scalar) -> DenseRange(64, 1536, 128) ->Unit(benchmark::kMicrosecond)`
- Source: `benchmarks/base/bench_arm_simd.cpp`:298
- Brief: n/a
- Parameters:
  - `<unnamed>` (BM_ARM_DotProduct_Scalar): n/a

#### `BENCHMARK(BM_ARM_L2_Distance_SIMD) -> DenseRange(64, 1536, 128) ->Unit(benchmark::kMicrosecond)`
- Source: `benchmarks/base/bench_arm_simd.cpp`:287
- Brief: n/a
- Parameters:
  - `<unnamed>` (BM_ARM_L2_Distance_SIMD): n/a

#### `BENCHMARK(BM_ARM_L2_Distance_Scalar) -> DenseRange(64, 1536, 128) ->Unit(benchmark::kMicrosecond)`
- Source: `benchmarks/base/bench_arm_simd.cpp`:288
- Brief: n/a
- Parameters:
  - `<unnamed>` (BM_ARM_L2_Distance_Scalar): n/a

#### `BENCHMARK(BM_ARM_L2_Distance_Squared_SIMD) -> DenseRange(64, 1536, 128) ->Unit(benchmark::kMicrosecond)`
- Source: `benchmarks/base/bench_arm_simd.cpp`:291
- Brief: n/a
- Parameters:
  - `<unnamed>` (BM_ARM_L2_Distance_Squared_SIMD): n/a

#### `BENCHMARK(BM_ARM_L2_Distance_Squared_Scalar) -> DenseRange(64, 1536, 128) ->Unit(benchmark::kMicrosecond)`
- Source: `benchmarks/base/bench_arm_simd.cpp`:292
- Brief: n/a
- Parameters:
  - `<unnamed>` (BM_ARM_L2_Distance_Squared_Scalar): n/a

#### `BENCHMARK_MAIN()`
- Source: `benchmarks/base/bench_arm_simd.cpp`:304
- Brief: n/a
- Parameters: none

#### `void BM_ARM_Batch_L2_SIMD(benchmark::State &state)`
- Source: `benchmarks/base/bench_arm_simd.cpp`:221
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_ARM_Batch_L2_Scalar(benchmark::State &state)`
- Source: `benchmarks/base/bench_arm_simd.cpp`:252
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_ARM_DotProduct_Scalar(benchmark::State &state)`
- Source: `benchmarks/base/bench_arm_simd.cpp`:203
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_ARM_L2_Distance_SIMD(benchmark::State &state)`
- Source: `benchmarks/base/bench_arm_simd.cpp`:106
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_ARM_L2_Distance_Scalar(benchmark::State &state)`
- Source: `benchmarks/base/bench_arm_simd.cpp`:129
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_ARM_L2_Distance_Squared_SIMD(benchmark::State &state)`
- Source: `benchmarks/base/bench_arm_simd.cpp`:151
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_ARM_L2_Distance_Squared_Scalar(benchmark::State &state)`
- Source: `benchmarks/base/bench_arm_simd.cpp`:169
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

### bench_async_io_multiscan.cpp

#### `BENCHMARK(BM_AsyncIO_Multiscan_Disabled)`
- Source: `benchmarks/base/bench_async_io_multiscan.cpp`:11
- Brief: n/a
- Parameters:
  - `<unnamed>` (BM_AsyncIO_Multiscan_Disabled): n/a

#### `BENCHMARK_MAIN()`
- Source: `benchmarks/base/bench_async_io_multiscan.cpp`:12
- Brief: n/a
- Parameters: none

#### `void BM_AsyncIO_Multiscan_Disabled(benchmark::State &state)`
- Source: `benchmarks/base/bench_async_io_multiscan.cpp`:4
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

### bench_auto_buffers.cpp

#### `void BM_MapOperations_Insert(benchmark::State &state)`
- Source: `benchmarks/base/bench_auto_buffers.cpp`:82
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_MapOperations_Lookup(benchmark::State &state)`
- Source: `benchmarks/base/bench_auto_buffers.cpp`:95
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_StringOperations_Concatenation(benchmark::State &state)`
- Source: `benchmarks/base/bench_auto_buffers.cpp`:68
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_VectorOperations_RandomAccess(benchmark::State &state)`
- Source: `benchmarks/base/bench_auto_buffers.cpp`:14
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_VectorOperations_SequentialRead(benchmark::State &state)`
- Source: `benchmarks/base/bench_auto_buffers.cpp`:34
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_VectorOperations_Write(benchmark::State &state)`
- Source: `benchmarks/base/bench_auto_buffers.cpp`:52
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `Range(1, 10000)`
- Source: `benchmarks/base/bench_auto_buffers.cpp`:80
- Brief: n/a
- Parameters:
  - `<unnamed>` (1): n/a
  - `<unnamed>` (10000): n/a

#### `Range(100, 10000)`
- Source: `benchmarks/base/bench_auto_buffers.cpp`:93
- Brief: n/a
- Parameters:
  - `<unnamed>` (100): n/a
  - `<unnamed>` (10000): n/a

#### `RangeMultiplier(2) -> Range(1024, 1024 *1024) ->Complexity()`
- Source: `benchmarks/base/bench_auto_buffers.cpp`:30
- Brief: n/a
- Parameters:
  - `<unnamed>` (2): n/a

#### `int main(int argc, char **argv)`
- Source: `benchmarks/base/bench_auto_buffers.cpp`:118
- Brief: n/a
- Parameters:
  - `argc` (int): n/a
  - `argv` (char **): n/a

### bench_base_hot_paths.cpp

#### `Arg(10) -> Arg(50) ->Arg(100) ->Arg(200)`
- Source: `benchmarks/base/bench_base_hot_paths.cpp`:258
- Brief: n/a
- Parameters:
  - `<unnamed>` (10): n/a

#### `BENCHMARK(BM_GateBase05_BuildFromResolver_100Node)`
- Source: `benchmarks/base/bench_base_hot_paths.cpp`:234
- Brief: n/a
- Parameters:
  - `<unnamed>` (BM_GateBase05_BuildFromResolver_100Node): n/a

#### `BENCHMARK(BM_GateBase05_TopologicalOrder_100Node)`
- Source: `benchmarks/base/bench_base_hot_paths.cpp`:282
- Brief: n/a
- Parameters:
  - `<unnamed>` (BM_GateBase05_TopologicalOrder_100Node): n/a

#### `BENCHMARK_F(BaseHotPathsLoaderFixture, GateBase01_IsModuleLoaded)(benchmark`
- Source: `benchmarks/base/bench_base_hot_paths.cpp`:66
- Brief: n/a
- Parameters:
  - `<unnamed>` (BaseHotPathsLoaderFixture): n/a
  - `<unnamed>` (GateBase01_IsModuleLoaded): n/a
- Details: GATE-BASE-01: isModuleLoaded() throughput ≥ 500k ops/s

#### `BENCHMARK_F(BaseHotPathsLoaderFixture, GateBase01_IsModuleLoaded_WithLoadedModules)(benchmark`
- Source: `benchmarks/base/bench_base_hot_paths.cpp`:75
- Brief: n/a
- Parameters:
  - `<unnamed>` (BaseHotPathsLoaderFixture): n/a
  - `<unnamed>` (GateBase01_IsModuleLoaded_WithLoadedModules): n/a
- Details: GATE-BASE-01 (variant): isModuleLoaded with pre-populated loader

#### `BENCHMARK_F(BaseHotPathsLoaderFixture, GateBase02_GetMetrics)(benchmark`
- Source: `benchmarks/base/bench_base_hot_paths.cpp`:94
- Brief: n/a
- Parameters:
  - `<unnamed>` (BaseHotPathsLoaderFixture): n/a
  - `<unnamed>` (GateBase02_GetMetrics): n/a
- Details: GATE-BASE-02: getMetrics() throughput ≥ 100k ops/s

#### `BENCHMARK_F(BaseHotPathsLoaderFixture, GateBase02_GetMetrics_FreshLoader)(benchmark`
- Source: `benchmarks/base/bench_base_hot_paths.cpp`:109
- Brief: n/a
- Parameters:
  - `<unnamed>` (BaseHotPathsLoaderFixture): n/a
  - `<unnamed>` (GateBase02_GetMetrics_FreshLoader): n/a
- Details: GATE-BASE-02 (zero-state variant): getMetrics() on a fresh loader

#### `BENCHMARK_F(BaseHotPathsReloadFixture, GateBase03_RegisteredModules_10)(benchmark`
- Source: `benchmarks/base/bench_base_hot_paths.cpp`:144
- Brief: n/a
- Parameters:
  - `<unnamed>` (BaseHotPathsReloadFixture): n/a
  - `<unnamed>` (GateBase03_RegisteredModules_10): n/a
- Details: GATE-BASE-03: registeredModules() p99 ≤ 5µs with 10 modules registered

#### `BENCHMARK_F(BaseHotPathsReloadFixture, GateBase03_RegisteredModules_Scaling)(benchmark`
- Source: `benchmarks/base/bench_base_hot_paths.cpp`:161
- Brief: n/a
- Parameters:
  - `<unnamed>` (BaseHotPathsReloadFixture): n/a
  - `<unnamed>` (GateBase03_RegisteredModules_Scaling): n/a
- Details: GATE-BASE-03 (scaling variant): registeredModules() with N modules

#### `BENCHMARK_F(BaseHotPathsReloadFixture, GateBase04_IsRollbackAvailable_NoBackup)(benchmark`
- Source: `benchmarks/base/bench_base_hot_paths.cpp`:185
- Brief: n/a
- Parameters:
  - `<unnamed>` (BaseHotPathsReloadFixture): n/a
  - `<unnamed>` (GateBase04_IsRollbackAvailable_NoBackup): n/a
- Details: GATE-BASE-04: isRollbackAvailable() p99 ≤ 1µs — registered, no backup

#### `BENCHMARK_F(BaseHotPathsReloadFixture, GateBase04_IsRollbackAvailable_NotRegistered)(benchmark`
- Source: `benchmarks/base/bench_base_hot_paths.cpp`:198
- Brief: n/a
- Parameters:
  - `<unnamed>` (BaseHotPathsReloadFixture): n/a
  - `<unnamed>` (GateBase04_IsRollbackAvailable_NotRegistered): n/a
- Details: GATE-BASE-04 (not-registered path): isRollbackAvailable() on unknown module

#### `BENCHMARK_F(BaseHotPathsReloadFixture, GateBase06_ReloadModule_FastFail)(benchmark`
- Source: `benchmarks/base/bench_base_hot_paths.cpp`:289
- Brief: n/a
- Parameters:
  - `<unnamed>` (BaseHotPathsReloadFixture): n/a
  - `<unnamed>` (GateBase06_ReloadModule_FastFail): n/a
- Details: GATE-BASE-06: reloadModule() fast-fail (non-existent binary) ≤ 50µs

#### `BENCHMARK_F(BaseHotPathsReloadFixture, GateBase06_ReloadModule_FastFail_WithCallbacks)(benchmark`
- Source: `benchmarks/base/bench_base_hot_paths.cpp`:313
- Brief: n/a
- Parameters:
  - `<unnamed>` (BaseHotPathsReloadFixture): n/a
  - `<unnamed>` (GateBase06_ReloadModule_FastFail_WithCallbacks): n/a
- Details: GATE-BASE-06 (with callbacks): reloadModule() fast-fail with 5 no-op callbacks

#### `BENCHMARK_F(BaseHotPathsReloadFixture, GateBase06_ReloadModule_Unregistered)(benchmark`
- Source: `benchmarks/base/bench_base_hot_paths.cpp`:302
- Brief: n/a
- Parameters:
  - `<unnamed>` (BaseHotPathsReloadFixture): n/a
  - `<unnamed>` (GateBase06_ReloadModule_Unregistered): n/a
- Details: GATE-BASE-06 (unregistered path): reloadModule() on unregistered module

#### `BENCHMARK_F(BaseHotPathsReloadFixture, HotPath_GetCurrentVersion_Unloaded)(benchmark`
- Source: `benchmarks/base/bench_base_hot_paths.cpp`:334
- Brief: getCurrentVersion on a registered-but-unloaded module.
- Parameters:
  - `<unnamed>` (BaseHotPathsReloadFixture): n/a
  - `<unnamed>` (HotPath_GetCurrentVersion_Unloaded): n/a

#### `BENCHMARK_F(BaseHotPathsReloadFixture, HotPath_GetStats)(benchmark`
- Source: `benchmarks/base/bench_base_hot_paths.cpp`:347
- Brief: getStats() read overhead.
- Parameters:
  - `<unnamed>` (BaseHotPathsReloadFixture): n/a
  - `<unnamed>` (HotPath_GetStats): n/a

#### `BENCHMARK_F(BaseHotPathsReloadFixture, HotPath_Rollback_NoBackup)(benchmark`
- Source: `benchmarks/base/bench_base_hot_paths.cpp`:363
- Brief: rollback() fast-fail (no backup available).
- Parameters:
  - `<unnamed>` (BaseHotPathsReloadFixture): n/a
  - `<unnamed>` (HotPath_Rollback_NoBackup): n/a

#### `BENCHMARK_MAIN()`
- Source: `benchmarks/base/bench_base_hot_paths.cpp`:375
- Brief: n/a
- Parameters: none

#### `BENCHMARK_REGISTER_F(BaseHotPathsReloadFixture, GateBase03_RegisteredModules_Scaling) -> Arg(1) ->Arg(10) ->Arg(50) ->Arg(100)`
- Source: `benchmarks/base/bench_base_hot_paths.cpp`:176
- Brief: n/a
- Parameters:
  - `<unnamed>` (BaseHotPathsReloadFixture): n/a
  - `<unnamed>` (GateBase03_RegisteredModules_Scaling): n/a

#### `void BM_GateBase05_BuildFromResolver_100Node(benchmark::State &state)`
- Source: `benchmarks/base/bench_base_hot_paths.cpp`:213
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a
- Details: GATE-BASE-05: buildFromResolver() for 100-node chain ≤ 1ms

#### `void BM_GateBase05_BuildFromResolver_Scaling(benchmark::State &state)`
- Source: `benchmarks/base/bench_base_hot_paths.cpp`:237
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a
- Details: GATE-BASE-05 (scaling): buildFromResolver() with variable chain length

#### `void BM_GateBase05_TopologicalOrder_100Node(benchmark::State &state)`
- Source: `benchmarks/base/bench_base_hot_paths.cpp`:261
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a
- Details: GATE-BASE-05 (topological order): topologicalOrder() on 100-node chain

### bench_base_wasm_sandbox.cpp

#### `BENCHMARK(BM_GateBase07_LoadFromBytes_ValidWasm_FirstLoad)`
- Source: `benchmarks/base/bench_base_wasm_sandbox.cpp`:103
- Brief: n/a
- Parameters:
  - `<unnamed>` (BM_GateBase07_LoadFromBytes_ValidWasm_FirstLoad): n/a

#### `BENCHMARK(BM_GateBase10_Format_AbiMismatch)`
- Source: `benchmarks/base/bench_base_wasm_sandbox.cpp`:236
- Brief: n/a
- Parameters:
  - `<unnamed>` (BM_GateBase10_Format_AbiMismatch): n/a

#### `BENCHMARK(BM_GateBase10_Format_LoaderPath)`
- Source: `benchmarks/base/bench_base_wasm_sandbox.cpp`:223
- Brief: n/a
- Parameters:
  - `<unnamed>` (BM_GateBase10_Format_LoaderPath): n/a

#### `BENCHMARK(BM_GateBase10_ResolveDescription_Known)`
- Source: `benchmarks/base/bench_base_wasm_sandbox.cpp`:201
- Brief: n/a
- Parameters:
  - `<unnamed>` (BM_GateBase10_ResolveDescription_Known): n/a

#### `BENCHMARK(BM_GateBase10_ResolveDescription_Unknown)`
- Source: `benchmarks/base/bench_base_wasm_sandbox.cpp`:211
- Brief: n/a
- Parameters:
  - `<unnamed>` (BM_GateBase10_ResolveDescription_Unknown): n/a

#### `BENCHMARK(BM_GateBase11_IsKnownCode_Known)`
- Source: `benchmarks/base/bench_base_wasm_sandbox.cpp`:251
- Brief: n/a
- Parameters:
  - `<unnamed>` (BM_GateBase11_IsKnownCode_Known): n/a

#### `BENCHMARK(BM_GateBase11_IsKnownCode_Unknown)`
- Source: `benchmarks/base/bench_base_wasm_sandbox.cpp`:261
- Brief: n/a
- Parameters:
  - `<unnamed>` (BM_GateBase11_IsKnownCode_Unknown): n/a

#### `BENCHMARK(BM_ModuleSandbox_ConstructDestruct_Config)`
- Source: `benchmarks/base/bench_base_wasm_sandbox.cpp`:473
- Brief: n/a
- Parameters:
  - `<unnamed>` (BM_ModuleSandbox_ConstructDestruct_Config): n/a

#### `BENCHMARK(BM_ModuleSandbox_ConstructDestruct_Default)`
- Source: `benchmarks/base/bench_base_wasm_sandbox.cpp`:458
- Brief: n/a
- Parameters:
  - `<unnamed>` (BM_ModuleSandbox_ConstructDestruct_Default): n/a

#### `BENCHMARK(BM_ModuleSandbox_LastError_Fresh)`
- Source: `benchmarks/base/bench_base_wasm_sandbox.cpp`:484
- Brief: n/a
- Parameters:
  - `<unnamed>` (BM_ModuleSandbox_LastError_Fresh): n/a

#### `BENCHMARK(BM_WasmSandbox_AddHostFunction)`
- Source: `benchmarks/base/bench_base_wasm_sandbox.cpp`:349
- Brief: n/a
- Parameters:
  - `<unnamed>` (BM_WasmSandbox_AddHostFunction): n/a

#### `BENCHMARK(BM_WasmSandbox_CallExport_NoRuntime)`
- Source: `benchmarks/base/bench_base_wasm_sandbox.cpp`:383
- Brief: n/a
- Parameters:
  - `<unnamed>` (BM_WasmSandbox_CallExport_NoRuntime): n/a

#### `BENCHMARK(BM_WasmSandbox_ConstructDestruct)`
- Source: `benchmarks/base/bench_base_wasm_sandbox.cpp`:326
- Brief: n/a
- Parameters:
  - `<unnamed>` (BM_WasmSandbox_ConstructDestruct): n/a

#### `BENCHMARK(BM_WasmSandbox_HostFunctionCount)`
- Source: `benchmarks/base/bench_base_wasm_sandbox.cpp`:370
- Brief: n/a
- Parameters:
  - `<unnamed>` (BM_WasmSandbox_HostFunctionCount): n/a

#### `BENCHMARK_F(AbiCheckerFixture, GateBase09_CheckRequiredSymbols_EmptyList)(benchmark`
- Source: `benchmarks/base/bench_base_wasm_sandbox.cpp`:178
- Brief: n/a
- Parameters:
  - `<unnamed>` (AbiCheckerFixture): n/a
  - `<unnamed>` (GateBase09_CheckRequiredSymbols_EmptyList): n/a
- Details: GATE-BASE-09: checkRequiredSymbols() — empty required-list fast path

#### `BENCHMARK_F(AbiCheckerFixture, GateBase09_CheckVersions_Compatible)(benchmark`
- Source: `benchmarks/base/bench_base_wasm_sandbox.cpp`:158
- Brief: n/a
- Parameters:
  - `<unnamed>` (AbiCheckerFixture): n/a
  - `<unnamed>` (GateBase09_CheckVersions_Compatible): n/a
- Details: GATE-BASE-09: checkVersions() — compatible path throughput

#### `BENCHMARK_F(AbiCheckerFixture, GateBase09_CheckVersions_Incompatible)(benchmark`
- Source: `benchmarks/base/bench_base_wasm_sandbox.cpp`:168
- Brief: n/a
- Parameters:
  - `<unnamed>` (AbiCheckerFixture): n/a
  - `<unnamed>` (GateBase09_CheckVersions_Incompatible): n/a
- Details: GATE-BASE-09 (incompatible): checkVersions() — major mismatch path

#### `BENCHMARK_F(GraphExportFixture, GraphDetectCycles_Acyclic)(benchmark`
- Source: `benchmarks/base/bench_base_wasm_sandbox.cpp`:438
- Brief: detectCycles() throughput on a known-acyclic graph.
- Parameters:
  - `<unnamed>` (GraphExportFixture): n/a
  - `<unnamed>` (GraphDetectCycles_Acyclic): n/a

#### `BENCHMARK_F(GraphExportFixture, GraphExport_ASCII_20Node)(benchmark`
- Source: `benchmarks/base/bench_base_wasm_sandbox.cpp`:429
- Brief: ASCII export throughput on a 20-node chain.
- Parameters:
  - `<unnamed>` (GraphExportFixture): n/a
  - `<unnamed>` (GraphExport_ASCII_20Node): n/a

#### `BENCHMARK_F(GraphExportFixture, GraphExport_DOT_20Node)(benchmark`
- Source: `benchmarks/base/bench_base_wasm_sandbox.cpp`:411
- Brief: DOT export throughput on a 20-node chain.
- Parameters:
  - `<unnamed>` (GraphExportFixture): n/a
  - `<unnamed>` (GraphExport_DOT_20Node): n/a

#### `BENCHMARK_F(GraphExportFixture, GraphExport_JSON_20Node)(benchmark`
- Source: `benchmarks/base/bench_base_wasm_sandbox.cpp`:420
- Brief: JSON export throughput on a 20-node chain.
- Parameters:
  - `<unnamed>` (GraphExportFixture): n/a
  - `<unnamed>` (GraphExport_JSON_20Node): n/a

#### `BENCHMARK_F(SandboxStatsFixture, GateBase12_IsActive_InactiveSandbox)(benchmark`
- Source: `benchmarks/base/bench_base_wasm_sandbox.cpp`:295
- Brief: n/a
- Parameters:
  - `<unnamed>` (SandboxStatsFixture): n/a
  - `<unnamed>` (GateBase12_IsActive_InactiveSandbox): n/a
- Details: GATE-BASE-12: isActive() on inactive sandbox

#### `BENCHMARK_F(SandboxStatsFixture, GateBase12_IsWasmIsolationActive)(benchmark`
- Source: `benchmarks/base/bench_base_wasm_sandbox.cpp`:305
- Brief: n/a
- Parameters:
  - `<unnamed>` (SandboxStatsFixture): n/a
  - `<unnamed>` (GateBase12_IsWasmIsolationActive): n/a
- Details: GATE-BASE-12: isWasmIsolationActive() predicate throughput

#### `BENCHMARK_F(SandboxStatsFixture, GateBase12_Stats_InactiveSandbox)(benchmark`
- Source: `benchmarks/base/bench_base_wasm_sandbox.cpp`:285
- Brief: n/a
- Parameters:
  - `<unnamed>` (SandboxStatsFixture): n/a
  - `<unnamed>` (GateBase12_Stats_InactiveSandbox): n/a
- Details: GATE-BASE-12: stats() on inactive sandbox — fast path throughput

#### `BENCHMARK_F(WasmLoadBytesFixture, GateBase07_LoadFromBytes_ValidWasm)(benchmark`
- Source: `benchmarks/base/bench_base_wasm_sandbox.cpp`:81
- Brief: n/a
- Parameters:
  - `<unnamed>` (WasmLoadBytesFixture): n/a
  - `<unnamed>` (GateBase07_LoadFromBytes_ValidWasm): n/a
- Details: GATE-BASE-07: loadFromBytes() valid WASM, including unload per iteration

#### `BENCHMARK_F(WasmLoadBytesFixture, GateBase08_LoadFromBytes_EmptyFastFail)(benchmark`
- Source: `benchmarks/base/bench_base_wasm_sandbox.cpp`:121
- Brief: n/a
- Parameters:
  - `<unnamed>` (WasmLoadBytesFixture): n/a
  - `<unnamed>` (GateBase08_LoadFromBytes_EmptyFastFail): n/a
- Details: GATE-BASE-08 (empty): loadFromBytes() empty buffer fast-fail

#### `BENCHMARK_F(WasmLoadBytesFixture, GateBase08_LoadFromBytes_InvalidFastFail)(benchmark`
- Source: `benchmarks/base/bench_base_wasm_sandbox.cpp`:110
- Brief: n/a
- Parameters:
  - `<unnamed>` (WasmLoadBytesFixture): n/a
  - `<unnamed>` (GateBase08_LoadFromBytes_InvalidFastFail): n/a
- Details: GATE-BASE-08: loadFromBytes() invalid bytes — fast-fail throughput

#### `BENCHMARK_MAIN()`
- Source: `benchmarks/base/bench_base_wasm_sandbox.cpp`:486
- Brief: n/a
- Parameters: none

#### `void BM_GateBase07_LoadFromBytes_ValidWasm_FirstLoad(benchmark::State &state)`
- Source: `benchmarks/base/bench_base_wasm_sandbox.cpp`:92
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a
- Details: GATE-BASE-07 (load-only): loadFromBytes without unload (measures first load)

#### `void BM_GateBase10_Format_AbiMismatch(benchmark::State &state)`
- Source: `benchmarks/base/bench_base_wasm_sandbox.cpp`:226
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a
- Details: GATE-BASE-10: format() diagnostic builder throughput (ABI mismatch)

#### `void BM_GateBase10_Format_LoaderPath(benchmark::State &state)`
- Source: `benchmarks/base/bench_base_wasm_sandbox.cpp`:214
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a
- Details: GATE-BASE-10: format() diagnostic builder throughput (loader path)

#### `void BM_GateBase10_ResolveDescription_Known(benchmark::State &state)`
- Source: `benchmarks/base/bench_base_wasm_sandbox.cpp`:193
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a
- Details: GATE-BASE-10: resolveDescription() for a known code throughput

#### `void BM_GateBase10_ResolveDescription_Unknown(benchmark::State &state)`
- Source: `benchmarks/base/bench_base_wasm_sandbox.cpp`:204
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a
- Details: GATE-BASE-10 (unknown): resolveDescription() for an unknown code (returns empty)

#### `void BM_GateBase11_IsKnownCode_Known(benchmark::State &state)`
- Source: `benchmarks/base/bench_base_wasm_sandbox.cpp`:243
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a
- Details: GATE-BASE-11: isKnownCode() — known code throughput

#### `void BM_GateBase11_IsKnownCode_Unknown(benchmark::State &state)`
- Source: `benchmarks/base/bench_base_wasm_sandbox.cpp`:254
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a
- Details: GATE-BASE-11 (unknown): isKnownCode() — out-of-range fast-fail throughput

#### `void BM_ModuleSandbox_ConstructDestruct_Config(benchmark::State &state)`
- Source: `benchmarks/base/bench_base_wasm_sandbox.cpp`:461
- Brief: ModuleSandbox construction with non-default config throughput.
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_ModuleSandbox_ConstructDestruct_Default(benchmark::State &state)`
- Source: `benchmarks/base/bench_base_wasm_sandbox.cpp`:451
- Brief: ModuleSandbox default construction/destruction throughput.
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_ModuleSandbox_LastError_Fresh(benchmark::State &state)`
- Source: `benchmarks/base/bench_base_wasm_sandbox.cpp`:476
- Brief: lastError() read on a fresh (never-launched) sandbox.
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_WasmSandbox_AddHostFunction(benchmark::State &state)`
- Source: `benchmarks/base/bench_base_wasm_sandbox.cpp`:329
- Brief: addHostFunction() throughput (single registration per call).
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_WasmSandbox_CallExport_NoRuntime(benchmark::State &state)`
- Source: `benchmarks/base/bench_base_wasm_sandbox.cpp`:373
- Brief: callExport() without runtime — fast-fail path throughput.
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_WasmSandbox_ConstructDestruct(benchmark::State &state)`
- Source: `benchmarks/base/bench_base_wasm_sandbox.cpp`:319
- Brief: WasmPluginSandbox construction/destruction throughput.
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_WasmSandbox_HostFunctionCount(benchmark::State &state)`
- Source: `benchmarks/base/bench_base_wasm_sandbox.cpp`:352
- Brief: hostFunctionCount() read throughput.
- Parameters:
  - `state` (benchmark::State &): n/a

### bench_comprehensive.cpp

#### `BENCHMARK_F(AQLJoinBench, JoinUsers_Posts)(benchmark`
- Source: `benchmarks/base/bench_comprehensive.cpp`:400
- Brief: n/a
- Parameters:
  - `<unnamed>` (AQLJoinBench): n/a
  - `<unnamed>` (JoinUsers_Posts): n/a

#### `BENCHMARK_F(AQLQueryBench, ComplexSelect_MultipleConditions)(benchmark`
- Source: `benchmarks/base/bench_comprehensive.cpp`:347
- Brief: n/a
- Parameters:
  - `<unnamed>` (AQLQueryBench): n/a
  - `<unnamed>` (ComplexSelect_MultipleConditions): n/a

#### `BENCHMARK_F(AQLQueryBench, SimpleSelect_WhereClause)(benchmark`
- Source: `benchmarks/base/bench_comprehensive.cpp`:327
- Brief: n/a
- Parameters:
  - `<unnamed>` (AQLQueryBench): n/a
  - `<unnamed>` (SimpleSelect_WhereClause): n/a

#### `BENCHMARK_F(BatchOperationsBench, BatchInsert_10K_WithMetadata)(benchmark`
- Source: `benchmarks/base/bench_comprehensive.cpp`:750
- Brief: n/a
- Parameters:
  - `<unnamed>` (BatchOperationsBench): n/a
  - `<unnamed>` (BatchInsert_10K_WithMetadata): n/a

#### `BENCHMARK_F(BatchOperationsBench, BatchUpdate_MultiField_5K)(benchmark`
- Source: `benchmarks/base/bench_comprehensive.cpp`:767
- Brief: n/a
- Parameters:
  - `<unnamed>` (BatchOperationsBench): n/a
  - `<unnamed>` (BatchUpdate_MultiField_5K): n/a

#### `BENCHMARK_F(BinaryOperationsBench, RetrieveBlobsBatch_100x100KB)(benchmark`
- Source: `benchmarks/base/bench_comprehensive.cpp`:502
- Brief: n/a
- Parameters:
  - `<unnamed>` (BinaryOperationsBench): n/a
  - `<unnamed>` (RetrieveBlobsBatch_100x100KB): n/a

#### `BENCHMARK_F(BinaryOperationsBench, StoreLargeBlobs_1MB)(benchmark`
- Source: `benchmarks/base/bench_comprehensive.cpp`:488
- Brief: n/a
- Parameters:
  - `<unnamed>` (BinaryOperationsBench): n/a
  - `<unnamed>` (StoreLargeBlobs_1MB): n/a

#### `BENCHMARK_F(BinaryOperationsBench, StoreThumbnails_10KB)(benchmark`
- Source: `benchmarks/base/bench_comprehensive.cpp`:467
- Brief: n/a
- Parameters:
  - `<unnamed>` (BinaryOperationsBench): n/a
  - `<unnamed>` (StoreThumbnails_10KB): n/a

#### `BENCHMARK_F(ComplexVectorBench, BatchInsert_1536D_LLMVectors)(benchmark`
- Source: `benchmarks/base/bench_comprehensive.cpp`:156
- Brief: n/a
- Parameters:
  - `<unnamed>` (ComplexVectorBench): n/a
  - `<unnamed>` (BatchInsert_1536D_LLMVectors): n/a

#### `BENCHMARK_F(ComplexVectorBench, Search_4096D_TopK_Batch)(benchmark`
- Source: `benchmarks/base/bench_comprehensive.cpp`:172
- Brief: n/a
- Parameters:
  - `<unnamed>` (ComplexVectorBench): n/a
  - `<unnamed>` (Search_4096D_TopK_Batch): n/a

#### `BENCHMARK_F(GraphOperationsBench, AddEdges_SparseGraph)(benchmark`
- Source: `benchmarks/base/bench_comprehensive.cpp`:554
- Brief: n/a
- Parameters:
  - `<unnamed>` (GraphOperationsBench): n/a
  - `<unnamed>` (AddEdges_SparseGraph): n/a

#### `BENCHMARK_F(GraphOperationsBench, GraphTraversal_BFS_Depth3)(benchmark`
- Source: `benchmarks/base/bench_comprehensive.cpp`:590
- Brief: n/a
- Parameters:
  - `<unnamed>` (GraphOperationsBench): n/a
  - `<unnamed>` (GraphTraversal_BFS_Depth3): n/a

#### `BENCHMARK_F(GraphOperationsBench, QueryNeighbors_DenseGraph)(benchmark`
- Source: `benchmarks/base/bench_comprehensive.cpp`:569
- Brief: n/a
- Parameters:
  - `<unnamed>` (GraphOperationsBench): n/a
  - `<unnamed>` (QueryNeighbors_DenseGraph): n/a

#### `BENCHMARK_F(LLMInferencingBench, EmbeddingGeneration_Store)(benchmark`
- Source: `benchmarks/base/bench_comprehensive.cpp`:223
- Brief: n/a
- Parameters:
  - `<unnamed>` (LLMInferencingBench): n/a
  - `<unnamed>` (EmbeddingGeneration_Store): n/a

#### `BENCHMARK_F(LLMInferencingBench, MultiQueryExpansion_5Queries)(benchmark`
- Source: `benchmarks/base/bench_comprehensive.cpp`:270
- Brief: n/a
- Parameters:
  - `<unnamed>` (LLMInferencingBench): n/a
  - `<unnamed>` (MultiQueryExpansion_5Queries): n/a

#### `BENCHMARK_F(LLMInferencingBench, RAG_Search_Retrieve_Top50)(benchmark`
- Source: `benchmarks/base/bench_comprehensive.cpp`:244
- Brief: n/a
- Parameters:
  - `<unnamed>` (LLMInferencingBench): n/a
  - `<unnamed>` (RAG_Search_Retrieve_Top50): n/a

#### `BENCHMARK_F(SecondaryIndexBench, CompositeIndexLookup)(benchmark`
- Source: `benchmarks/base/bench_comprehensive.cpp`:697
- Brief: n/a
- Parameters:
  - `<unnamed>` (SecondaryIndexBench): n/a
  - `<unnamed>` (CompositeIndexLookup): n/a

#### `BENCHMARK_F(SecondaryIndexBench, LargeIndexLookup_1M)(benchmark`
- Source: `benchmarks/base/bench_comprehensive.cpp`:676
- Brief: n/a
- Parameters:
  - `<unnamed>` (SecondaryIndexBench): n/a
  - `<unnamed>` (LargeIndexLookup_1M): n/a

#### `BENCHMARK_F(SecondaryIndexBench, MediumIndexInsert_100K)(benchmark`
- Source: `benchmarks/base/bench_comprehensive.cpp`:659
- Brief: n/a
- Parameters:
  - `<unnamed>` (SecondaryIndexBench): n/a
  - `<unnamed>` (MediumIndexInsert_100K): n/a

#### `BENCHMARK_F(SecondaryIndexBench, SmallIndexInsert_1K)(benchmark`
- Source: `benchmarks/base/bench_comprehensive.cpp`:644
- Brief: n/a
- Parameters:
  - `<unnamed>` (SecondaryIndexBench): n/a
  - `<unnamed>` (SmallIndexInsert_1K): n/a

#### `BENCHMARK_F(SimpleVectorBench, Insert_384D_Embeddings)(benchmark`
- Source: `benchmarks/base/bench_comprehensive.cpp`:114
- Brief: n/a
- Parameters:
  - `<unnamed>` (SimpleVectorBench): n/a
  - `<unnamed>` (Insert_384D_Embeddings): n/a

#### `BENCHMARK_F(SimpleVectorBench, Insert_RGB_Vectors)(benchmark`
- Source: `benchmarks/base/bench_comprehensive.cpp`:83
- Brief: n/a
- Parameters:
  - `<unnamed>` (SimpleVectorBench): n/a
  - `<unnamed>` (Insert_RGB_Vectors): n/a

#### `BENCHMARK_F(SimpleVectorBench, Search_RGB_KNN_Top10)(benchmark`
- Source: `benchmarks/base/bench_comprehensive.cpp`:96
- Brief: n/a
- Parameters:
  - `<unnamed>` (SimpleVectorBench): n/a
  - `<unnamed>` (Search_RGB_KNN_Top10): n/a

#### `BENCHMARK_F(StressTestBench, HotspotAccess_99PercentContention)(benchmark`
- Source: `benchmarks/base/bench_comprehensive.cpp`:857
- Brief: n/a
- Parameters:
  - `<unnamed>` (StressTestBench): n/a
  - `<unnamed>` (HotspotAccess_99PercentContention): n/a

#### `BENCHMARK_F(StressTestBench, MixedReadWrite_80Reads_20Writes)(benchmark`
- Source: `benchmarks/base/bench_comprehensive.cpp`:828
- Brief: n/a
- Parameters:
  - `<unnamed>` (StressTestBench): n/a
  - `<unnamed>` (MixedReadWrite_80Reads_20Writes): n/a

#### `BENCHMARK_MAIN()`
- Source: `benchmarks/base/bench_comprehensive.cpp`:885
- Brief: n/a
- Parameters: none

#### `std::vector< float > genVec(size_t dim, int seed=0)`
- Source: `benchmarks/base/bench_comprehensive.cpp`:20
- Brief: n/a
- Parameters:
  - `dim` (size_t): n/a
  - `seed` (int): n/a

#### `std::string randStr(size_t len)`
- Source: `benchmarks/base/bench_comprehensive.cpp`:42
- Brief: n/a
- Parameters:
  - `len` (size_t): n/a

### bench_compression.cpp

#### `BENCHMARK_MAIN()`
- Source: `benchmarks/base/bench_compression.cpp`:159
- Brief: n/a
- Parameters: none

### bench_edge_cases_comprehensive.cpp

#### `void BM_AlternatingWriteRead(benchmark::State &state)`
- Source: `benchmarks/base/bench_edge_cases_comprehensive.cpp`:499
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_DuplicateKeyWrites(benchmark::State &state)`
- Source: `benchmarks/base/bench_edge_cases_comprehensive.cpp`:339
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_EmptyStringValues(benchmark::State &state)`
- Source: `benchmarks/base/bench_edge_cases_comprehensive.cpp`:203
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_FirstInsertToEmptyDatabase(benchmark::State &state)`
- Source: `benchmarks/base/bench_edge_cases_comprehensive.cpp`:96
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_MaximumValueSize(benchmark::State &state)`
- Source: `benchmarks/base/bench_edge_cases_comprehensive.cpp`:299
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_MinMaxIntegerValues(benchmark::State &state)`
- Source: `benchmarks/base/bench_edge_cases_comprehensive.cpp`:171
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_NegativeValues(benchmark::State &state)`
- Source: `benchmarks/base/bench_edge_cases_comprehensive.cpp`:573
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_RapidSuccessiveReads(benchmark::State &state)`
- Source: `benchmarks/base/bench_edge_cases_comprehensive.cpp`:457
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_RapidSuccessiveWrites(benchmark::State &state)`
- Source: `benchmarks/base/bench_edge_cases_comprehensive.cpp`:423
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_ReadFromEmptyDatabase(benchmark::State &state)`
- Source: `benchmarks/base/bench_edge_cases_comprehensive.cpp`:67
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_SingleElementDatabase(benchmark::State &state)`
- Source: `benchmarks/base/bench_edge_cases_comprehensive.cpp`:135
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_SpecialCharacterKeys(benchmark::State &state)`
- Source: `benchmarks/base/bench_edge_cases_comprehensive.cpp`:376
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_VeryLongKeys(benchmark::State &state)`
- Source: `benchmarks/base/bench_edge_cases_comprehensive.cpp`:268
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_VeryShortKeys(benchmark::State &state)`
- Source: `benchmarks/base/bench_edge_cases_comprehensive.cpp`:238
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_ZeroValues(benchmark::State &state)`
- Source: `benchmarks/base/bench_edge_cases_comprehensive.cpp`:542
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `Unit(benchmark::kMicrosecond)`
- Source: `benchmarks/base/bench_edge_cases_comprehensive.cpp`:94
- Brief: n/a
- Parameters:
  - `<unnamed>` (benchmark::kMicrosecond): n/a

#### `Unit(benchmark::kMillisecond) -> Iterations(10)`
- Source: `benchmarks/base/bench_edge_cases_comprehensive.cpp`:332
- Brief: n/a
- Parameters:
  - `<unnamed>` (benchmark::kMillisecond): n/a

#### `std::string getUniqueTempPath(const std::string &base_name)`
- Source: `benchmarks/base/bench_edge_cases_comprehensive.cpp`:30
- Brief: n/a
- Parameters:
  - `base_name` (const std::string &): n/a

### bench_gorilla_codec.cpp

#### `Arg(100) -> Arg(1000) ->Arg(10000) ->Arg(100000) ->Unit(benchmark::kMillisecond)`
- Source: `benchmarks/base/bench_gorilla_codec.cpp`:205
- Brief: n/a
- Parameters:
  - `<unnamed>` (100): n/a

#### `BENCHMARK(BM_GorillaDecode_1MB) -> Unit(benchmark::kMicrosecond)`
- Source: `benchmarks/base/bench_gorilla_codec.cpp`:326
- Brief: n/a
- Parameters:
  - `<unnamed>` (BM_GorillaDecode_1MB): n/a

#### `BENCHMARK(BM_GorillaDecode_Constant) -> Arg(100) ->Arg(1000) ->Arg(10000) ->Unit(benchmark::kMicrosecond)`
- Source: `benchmarks/base/bench_gorilla_codec.cpp`:133
- Brief: n/a
- Parameters:
  - `<unnamed>` (BM_GorillaDecode_Constant): n/a

#### `BENCHMARK(BM_GorillaDecode_Sine) -> Arg(100) ->Arg(1000) ->Arg(10000) ->Unit(benchmark::kMicrosecond)`
- Source: `benchmarks/base/bench_gorilla_codec.cpp`:150
- Brief: n/a
- Parameters:
  - `<unnamed>` (BM_GorillaDecode_Sine): n/a

#### `BENCHMARK(BM_GorillaDecode_Truncated) -> Unit(benchmark::kMicrosecond)`
- Source: `benchmarks/base/bench_gorilla_codec.cpp`:171
- Brief: n/a
- Parameters:
  - `<unnamed>` (BM_GorillaDecode_Truncated): n/a

#### `BENCHMARK(BM_GorillaEncode_Constant) -> Arg(100) ->Arg(1000) ->Arg(10000) ->Unit(benchmark::kMicrosecond)`
- Source: `benchmarks/base/bench_gorilla_codec.cpp`:74
- Brief: n/a
- Parameters:
  - `<unnamed>` (BM_GorillaEncode_Constant): n/a

#### `BENCHMARK(BM_GorillaEncode_Random) -> Arg(100) ->Arg(1000) ->Arg(10000) ->Unit(benchmark::kMicrosecond)`
- Source: `benchmarks/base/bench_gorilla_codec.cpp`:114
- Brief: n/a
- Parameters:
  - `<unnamed>` (BM_GorillaEncode_Random): n/a

#### `BENCHMARK(BM_GorillaEncode_Sine) -> Arg(100) ->Arg(1000) ->Arg(10000) ->Unit(benchmark::kMicrosecond)`
- Source: `benchmarks/base/bench_gorilla_codec.cpp`:94
- Brief: n/a
- Parameters:
  - `<unnamed>` (BM_GorillaEncode_Sine): n/a

#### `BENCHMARK(BM_GorillaSIMDDecode_Constant) -> Arg(100) ->Arg(1000) ->Arg(10000) ->Unit(benchmark::kMicrosecond)`
- Source: `benchmarks/base/bench_gorilla_codec.cpp`:228
- Brief: n/a
- Parameters:
  - `<unnamed>` (BM_GorillaSIMDDecode_Constant): n/a

#### `BENCHMARK(BM_GorillaSIMDDecode_Random) -> Arg(100) ->Arg(1000) ->Arg(10000) ->Unit(benchmark::kMicrosecond)`
- Source: `benchmarks/base/bench_gorilla_codec.cpp`:264
- Brief: n/a
- Parameters:
  - `<unnamed>` (BM_GorillaSIMDDecode_Random): n/a

#### `BENCHMARK(BM_GorillaSIMDDecode_Sine) -> Arg(100) ->Arg(1000) ->Arg(10000) ->Unit(benchmark::kMicrosecond)`
- Source: `benchmarks/base/bench_gorilla_codec.cpp`:246
- Brief: n/a
- Parameters:
  - `<unnamed>` (BM_GorillaSIMDDecode_Sine): n/a

#### `BENCHMARK(BM_GorillaSIMDDecode_Throughput) -> Arg(10000) ->Arg(100000) ->Unit(benchmark::kMicrosecond)`
- Source: `benchmarks/base/bench_gorilla_codec.cpp`:302
- Brief: n/a
- Parameters:
  - `<unnamed>` (BM_GorillaSIMDDecode_Throughput): n/a

#### `BENCHMARK(BM_GorillaScalarDecode_Throughput) -> Arg(10000) ->Arg(100000) ->Unit(benchmark::kMicrosecond)`
- Source: `benchmarks/base/bench_gorilla_codec.cpp`:283
- Brief: n/a
- Parameters:
  - `<unnamed>` (BM_GorillaScalarDecode_Throughput): n/a

#### `BENCHMARK_MAIN()`
- Source: `benchmarks/base/bench_gorilla_codec.cpp`:328
- Brief: n/a
- Parameters: none

#### `void BM_GorillaCompressionRatio(benchmark::State &state)`
- Source: `benchmarks/base/bench_gorilla_codec.cpp`:175
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_GorillaDecode_1MB(benchmark::State &state)`
- Source: `benchmarks/base/bench_gorilla_codec.cpp`:304
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_GorillaDecode_Constant(benchmark::State &state)`
- Source: `benchmarks/base/bench_gorilla_codec.cpp`:118
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_GorillaDecode_Sine(benchmark::State &state)`
- Source: `benchmarks/base/bench_gorilla_codec.cpp`:135
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_GorillaDecode_Truncated(benchmark::State &state)`
- Source: `benchmarks/base/bench_gorilla_codec.cpp`:154
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_GorillaEncode_Constant(benchmark::State &state)`
- Source: `benchmarks/base/bench_gorilla_codec.cpp`:56
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_GorillaEncode_Random(benchmark::State &state)`
- Source: `benchmarks/base/bench_gorilla_codec.cpp`:96
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_GorillaEncode_Sine(benchmark::State &state)`
- Source: `benchmarks/base/bench_gorilla_codec.cpp`:76
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_GorillaSIMDDecode_Constant(benchmark::State &state)`
- Source: `benchmarks/base/bench_gorilla_codec.cpp`:212
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_GorillaSIMDDecode_Random(benchmark::State &state)`
- Source: `benchmarks/base/bench_gorilla_codec.cpp`:248
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_GorillaSIMDDecode_Sine(benchmark::State &state)`
- Source: `benchmarks/base/bench_gorilla_codec.cpp`:230
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_GorillaSIMDDecode_Throughput(benchmark::State &state)`
- Source: `benchmarks/base/bench_gorilla_codec.cpp`:285
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_GorillaScalarDecode_Throughput(benchmark::State &state)`
- Source: `benchmarks/base/bench_gorilla_codec.cpp`:269
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `std::vector< uint8_t > encode(const std::vector< std::pair< int64_t, double > > &s)`
- Source: `benchmarks/base/bench_gorilla_codec.cpp`:46
- Brief: n/a
- Parameters:
  - `s` (const std::vector< std::pair< int64_t, double > > &): n/a

#### `std::vector< std::pair< int64_t, double > > makeConstantSeries(int n)`
- Source: `benchmarks/base/bench_gorilla_codec.cpp`:17
- Brief: n/a
- Parameters:
  - `n` (int): n/a

#### `std::vector< std::pair< int64_t, double > > makeRandomSeries(int n, uint64_t seed=42)`
- Source: `benchmarks/base/bench_gorilla_codec.cpp`:35
- Brief: n/a
- Parameters:
  - `n` (int): n/a
  - `seed` (uint64_t): n/a

#### `std::vector< std::pair< int64_t, double > > makeSineSeries(int n)`
- Source: `benchmarks/base/bench_gorilla_codec.cpp`:26
- Brief: n/a
- Parameters:
  - `n` (int): n/a

### bench_hotspots_micro.cpp

#### `BENCHMARK(BM_MixedRW) -> Arg(1) ->Arg(4) ->Arg(8) ->Arg(16) ->UseRealTime()`
- Source: `benchmarks/base/bench_hotspots_micro.cpp`:378
- Brief: n/a
- Parameters:
  - `<unnamed>` (BM_MixedRW): n/a

#### `BENCHMARK(BM_MixedRW_Hybrid) -> Arg(1) ->Arg(4) ->Arg(8) ->Arg(16) ->UseRealTime()`
- Source: `benchmarks/base/bench_hotspots_micro.cpp`:379
- Brief: n/a
- Parameters:
  - `<unnamed>` (BM_MixedRW_Hybrid): n/a

#### `BENCHMARK(BM_PointReadP99) -> Iterations(20000) ->UseRealTime()`
- Source: `benchmarks/base/bench_hotspots_micro.cpp`:395
- Brief: n/a
- Parameters:
  - `<unnamed>` (BM_PointReadP99): n/a

#### `BENCHMARK(BM_RawWrite_WAL_Off) -> Arg(1) ->Arg(4) ->Arg(8) ->Arg(16) ->UseRealTime()`
- Source: `benchmarks/base/bench_hotspots_micro.cpp`:376
- Brief: n/a
- Parameters:
  - `<unnamed>` (BM_RawWrite_WAL_Off): n/a

#### `BENCHMARK(BM_RawWrite_WAL_On) -> Arg(1) ->Arg(4) ->Arg(8) ->Arg(16) ->UseRealTime()`
- Source: `benchmarks/base/bench_hotspots_micro.cpp`:375
- Brief: n/a
- Parameters:
  - `<unnamed>` (BM_RawWrite_WAL_On): n/a

#### `BENCHMARK(BM_RawWrite_WAL_On_Hybrid) -> Arg(1) ->Arg(4) ->Arg(8) ->Arg(16) ->UseRealTime()`
- Source: `benchmarks/base/bench_hotspots_micro.cpp`:377
- Brief: n/a
- Parameters:
  - `<unnamed>` (BM_RawWrite_WAL_On_Hybrid): n/a

#### `BENCHMARK(BM_SecondaryIndex_Write) -> Arg(1) ->Arg(4) ->Arg(8) ->Arg(16) ->UseRealTime()`
- Source: `benchmarks/base/bench_hotspots_micro.cpp`:380
- Brief: n/a
- Parameters:
  - `<unnamed>` (BM_SecondaryIndex_Write): n/a

#### `BENCHMARK(BM_SecondaryIndex_Write_Hybrid) -> Arg(1) ->Arg(4) ->Arg(8) ->Arg(16) ->UseRealTime()`
- Source: `benchmarks/base/bench_hotspots_micro.cpp`:381
- Brief: n/a
- Parameters:
  - `<unnamed>` (BM_SecondaryIndex_Write_Hybrid): n/a

#### `BENCHMARK(BM_StorageInsert) -> UseRealTime()`
- Source: `benchmarks/base/bench_hotspots_micro.cpp`:390
- Brief: n/a
- Parameters:
  - `<unnamed>` (BM_StorageInsert): n/a

#### `BENCHMARK(BM_StorageRead) -> UseRealTime()`
- Source: `benchmarks/base/bench_hotspots_micro.cpp`:391
- Brief: n/a
- Parameters:
  - `<unnamed>` (BM_StorageRead): n/a

#### `BENCHMARK(BM_StorageUpdate) -> UseRealTime()`
- Source: `benchmarks/base/bench_hotspots_micro.cpp`:392
- Brief: n/a
- Parameters:
  - `<unnamed>` (BM_StorageUpdate): n/a

#### `BENCHMARK(BM_SustainedWrite) -> Arg(1) ->Arg(4) ->Arg(8) ->Arg(16) ->UseRealTime()`
- Source: `benchmarks/base/bench_hotspots_micro.cpp`:393
- Brief: n/a
- Parameters:
  - `<unnamed>` (BM_SustainedWrite): n/a

#### `BENCHMARK(BM_SustainedWrite_Hybrid) -> Arg(1) ->Arg(4) ->Arg(8) ->Arg(16) ->UseRealTime()`
- Source: `benchmarks/base/bench_hotspots_micro.cpp`:394
- Brief: n/a
- Parameters:
  - `<unnamed>` (BM_SustainedWrite_Hybrid): n/a

#### `BENCHMARK_MAIN()`
- Source: `benchmarks/base/bench_hotspots_micro.cpp`:397
- Brief: n/a
- Parameters: none

#### `void BM_MixedRW(benchmark::State &state)`
- Source: `benchmarks/base/bench_hotspots_micro.cpp`:370
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_MixedRW_Hybrid(benchmark::State &state)`
- Source: `benchmarks/base/bench_hotspots_micro.cpp`:371
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_PointReadP99(benchmark::State &state)`
- Source: `benchmarks/base/bench_hotspots_micro.cpp`:388
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_RawWrite_WAL_Off(benchmark::State &state)`
- Source: `benchmarks/base/bench_hotspots_micro.cpp`:368
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_RawWrite_WAL_On(benchmark::State &state)`
- Source: `benchmarks/base/bench_hotspots_micro.cpp`:367
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_RawWrite_WAL_On_Hybrid(benchmark::State &state)`
- Source: `benchmarks/base/bench_hotspots_micro.cpp`:369
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_SecondaryIndex_Write(benchmark::State &state)`
- Source: `benchmarks/base/bench_hotspots_micro.cpp`:372
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_SecondaryIndex_Write_Hybrid(benchmark::State &state)`
- Source: `benchmarks/base/bench_hotspots_micro.cpp`:373
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_StorageInsert(benchmark::State &state)`
- Source: `benchmarks/base/bench_hotspots_micro.cpp`:383
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_StorageRead(benchmark::State &state)`
- Source: `benchmarks/base/bench_hotspots_micro.cpp`:384
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_StorageUpdate(benchmark::State &state)`
- Source: `benchmarks/base/bench_hotspots_micro.cpp`:385
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_SustainedWrite(benchmark::State &state)`
- Source: `benchmarks/base/bench_hotspots_micro.cpp`:386
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_SustainedWrite_Hybrid(benchmark::State &state)`
- Source: `benchmarks/base/bench_hotspots_micro.cpp`:387
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

### bench_latency_comprehensive.cpp

#### `Arg(10) -> Arg(100) ->Arg(1000)`
- Source: `benchmarks/base/bench_latency_comprehensive.cpp`:406
- Brief: n/a
- Parameters:
  - `<unnamed>` (10): n/a

#### `BENCHMARK_F(LatencyBenchFixture, BatchWriteLatency)(benchmark`
- Source: `benchmarks/base/bench_latency_comprehensive.cpp`:382
- Brief: n/a
- Parameters:
  - `<unnamed>` (LatencyBenchFixture): n/a
  - `<unnamed>` (BatchWriteLatency): n/a

#### `BENCHMARK_F(LatencyBenchFixture, CacheHitLatency)(benchmark`
- Source: `benchmarks/base/bench_latency_comprehensive.cpp`:412
- Brief: n/a
- Parameters:
  - `<unnamed>` (LatencyBenchFixture): n/a
  - `<unnamed>` (CacheHitLatency): n/a

#### `BENCHMARK_F(LatencyBenchFixture, CacheMissLatency)(benchmark`
- Source: `benchmarks/base/bench_latency_comprehensive.cpp`:437
- Brief: n/a
- Parameters:
  - `<unnamed>` (LatencyBenchFixture): n/a
  - `<unnamed>` (CacheMissLatency): n/a

#### `BENCHMARK_F(LatencyBenchFixture, HugeValueLatency)(benchmark`
- Source: `benchmarks/base/bench_latency_comprehensive.cpp`:495
- Brief: n/a
- Parameters:
  - `<unnamed>` (LatencyBenchFixture): n/a
  - `<unnamed>` (HugeValueLatency): n/a

#### `BENCHMARK_F(LatencyBenchFixture, MixedLatency_70Read30Write)(benchmark`
- Source: `benchmarks/base/bench_latency_comprehensive.cpp`:300
- Brief: n/a
- Parameters:
  - `<unnamed>` (LatencyBenchFixture): n/a
  - `<unnamed>` (MixedLatency_70Read30Write): n/a

#### `BENCHMARK_F(LatencyBenchFixture, RandomReadLatency)(benchmark`
- Source: `benchmarks/base/bench_latency_comprehensive.cpp`:357
- Brief: n/a
- Parameters:
  - `<unnamed>` (LatencyBenchFixture): n/a
  - `<unnamed>` (RandomReadLatency): n/a

#### `BENCHMARK_F(LatencyBenchFixture, ReadLatency_LargeValues)(benchmark`
- Source: `benchmarks/base/bench_latency_comprehensive.cpp`:220
- Brief: n/a
- Parameters:
  - `<unnamed>` (LatencyBenchFixture): n/a
  - `<unnamed>` (ReadLatency_LargeValues): n/a

#### `BENCHMARK_F(LatencyBenchFixture, ReadLatency_SmallValues)(benchmark`
- Source: `benchmarks/base/bench_latency_comprehensive.cpp`:199
- Brief: n/a
- Parameters:
  - `<unnamed>` (LatencyBenchFixture): n/a
  - `<unnamed>` (ReadLatency_SmallValues): n/a

#### `BENCHMARK_F(LatencyBenchFixture, SequentialReadLatency)(benchmark`
- Source: `benchmarks/base/bench_latency_comprehensive.cpp`:336
- Brief: n/a
- Parameters:
  - `<unnamed>` (LatencyBenchFixture): n/a
  - `<unnamed>` (SequentialReadLatency): n/a

#### `BENCHMARK_F(LatencyBenchFixture, TinyValueLatency)(benchmark`
- Source: `benchmarks/base/bench_latency_comprehensive.cpp`:473
- Brief: n/a
- Parameters:
  - `<unnamed>` (LatencyBenchFixture): n/a
  - `<unnamed>` (TinyValueLatency): n/a

#### `BENCHMARK_F(LatencyBenchFixture, WriteLatency_LargeValues)(benchmark`
- Source: `benchmarks/base/bench_latency_comprehensive.cpp`:274
- Brief: n/a
- Parameters:
  - `<unnamed>` (LatencyBenchFixture): n/a
  - `<unnamed>` (WriteLatency_LargeValues): n/a

#### `BENCHMARK_F(LatencyBenchFixture, WriteLatency_SmallValues)(benchmark`
- Source: `benchmarks/base/bench_latency_comprehensive.cpp`:253
- Brief: n/a
- Parameters:
  - `<unnamed>` (LatencyBenchFixture): n/a
  - `<unnamed>` (WriteLatency_SmallValues): n/a

### bench_module_load_hot_reload.cpp

#### `Arg(1) -> Arg(2) ->Arg(4) ->Arg(8) ->Arg(16) ->UseRealTime()`
- Source: `benchmarks/base/bench_module_load_hot_reload.cpp`:218
- Brief: n/a
- Parameters:
  - `<unnamed>` (1): n/a

#### `BENCHMARK(BM_GetAllLoadedModules_Growth) -> Arg(0) ->Arg(10) ->Arg(100) ->Arg(1000)`
- Source: `benchmarks/base/bench_module_load_hot_reload.cpp`:132
- Brief: n/a
- Parameters:
  - `<unnamed>` (BM_GetAllLoadedModules_Growth): n/a

#### `BENCHMARK_F(HotReloadLoadFixture, ConcurrentReload_Scalability)(benchmark`
- Source: `benchmarks/base/bench_module_load_hot_reload.cpp`:185
- Brief: n/a
- Parameters:
  - `<unnamed>` (HotReloadLoadFixture): n/a
  - `<unnamed>` (ConcurrentReload_Scalability): n/a

#### `BENCHMARK_F(HotReloadLoadFixture, RegisterReloadUnregister_Cycle)(benchmark`
- Source: `benchmarks/base/bench_module_load_hot_reload.cpp`:171
- Brief: n/a
- Parameters:
  - `<unnamed>` (HotReloadLoadFixture): n/a
  - `<unnamed>` (RegisterReloadUnregister_Cycle): n/a

#### `BENCHMARK_F(HotReloadLoadFixture, Registration_Throughput)(benchmark`
- Source: `benchmarks/base/bench_module_load_hot_reload.cpp`:159
- Brief: n/a
- Parameters:
  - `<unnamed>` (HotReloadLoadFixture): n/a
  - `<unnamed>` (Registration_Throughput): n/a

#### `BENCHMARK_F(HotReloadLoadFixture, StatsLatency)(benchmark`
- Source: `benchmarks/base/bench_module_load_hot_reload.cpp`:223
- Brief: n/a
- Parameters:
  - `<unnamed>` (HotReloadLoadFixture): n/a
  - `<unnamed>` (StatsLatency): n/a

#### `BENCHMARK_F(ModuleLoaderFixture, GetMetrics)(benchmark`
- Source: `benchmarks/base/bench_module_load_hot_reload.cpp`:99
- Brief: n/a
- Parameters:
  - `<unnamed>` (ModuleLoaderFixture): n/a
  - `<unnamed>` (GetMetrics): n/a

#### `BENCHMARK_F(ModuleLoaderFixture, IsModuleLoaded_NotLoaded)(benchmark`
- Source: `benchmarks/base/bench_module_load_hot_reload.cpp`:74
- Brief: n/a
- Parameters:
  - `<unnamed>` (ModuleLoaderFixture): n/a
  - `<unnamed>` (IsModuleLoaded_NotLoaded): n/a

#### `BENCHMARK_F(ModuleLoaderFixture, LoadAllModules_EmptyDir)(benchmark`
- Source: `benchmarks/base/bench_module_load_hot_reload.cpp`:84
- Brief: n/a
- Parameters:
  - `<unnamed>` (ModuleLoaderFixture): n/a
  - `<unnamed>` (LoadAllModules_EmptyDir): n/a

#### `BENCHMARK_F(ModuleLoaderFixture, LoadModule_MissingFile)(benchmark`
- Source: `benchmarks/base/bench_module_load_hot_reload.cpp`:52
- Brief: n/a
- Parameters:
  - `<unnamed>` (ModuleLoaderFixture): n/a
  - `<unnamed>` (LoadModule_MissingFile): n/a

#### `BENCHMARK_F(ModuleLoaderFixture, UnloadModule_Unknown)(benchmark`
- Source: `benchmarks/base/bench_module_load_hot_reload.cpp`:65
- Brief: n/a
- Parameters:
  - `<unnamed>` (ModuleLoaderFixture): n/a
  - `<unnamed>` (UnloadModule_Unknown): n/a

#### `void BM_GetAllLoadedModules_Growth(benchmark::State &state)`
- Source: `benchmarks/base/bench_module_load_hot_reload.cpp`:114
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

### bench_multithreading_comprehensive.cpp

#### `void BM_BatchInsert_MultiThread(benchmark::State &state)`
- Source: `benchmarks/base/bench_multithreading_comprehensive.cpp`:365
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_BatchInsert_SingleThread(benchmark::State &state)`
- Source: `benchmarks/base/bench_multithreading_comprehensive.cpp`:334
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_ConcurrentReads_ThreadScaling(benchmark::State &state)`
- Source: `benchmarks/base/bench_multithreading_comprehensive.cpp`:177
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_ConcurrentWrites_ThreadScaling(benchmark::State &state)`
- Source: `benchmarks/base/bench_multithreading_comprehensive.cpp`:137
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_HighContentionWrites(benchmark::State &state)`
- Source: `benchmarks/base/bench_multithreading_comprehensive.cpp`:259
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_LowContentionWrites(benchmark::State &state)`
- Source: `benchmarks/base/bench_multithreading_comprehensive.cpp`:296
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_MaxThreadStress(benchmark::State &state)`
- Source: `benchmarks/base/bench_multithreading_comprehensive.cpp`:410
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_MixedReadWrite_ThreadScaling(benchmark::State &state)`
- Source: `benchmarks/base/bench_multithreading_comprehensive.cpp`:210
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `Unit(benchmark::kMillisecond) -> Arg(1) ->Arg(2) ->Arg(4) ->Arg(8) ->Arg(16) ->Arg(32) ->UseRealTime()`
- Source: `benchmarks/base/bench_multithreading_comprehensive.cpp`:173
- Brief: n/a
- Parameters:
  - `<unnamed>` (benchmark::kMillisecond): n/a

### bench_scalability_comprehensive.cpp

#### `BENCHMARK_F(ScalabilityBenchFixture, AppendWrite_ScalingDataSize)(benchmark`
- Source: `benchmarks/base/bench_scalability_comprehensive.cpp`:175
- Brief: n/a
- Parameters:
  - `<unnamed>` (ScalabilityBenchFixture): n/a
  - `<unnamed>` (AppendWrite_ScalingDataSize): n/a

#### `BENCHMARK_F(ScalabilityBenchFixture, RandomRead_ScalingDataSize)(benchmark`
- Source: `benchmarks/base/bench_scalability_comprehensive.cpp`:110
- Brief: n/a
- Parameters:
  - `<unnamed>` (ScalabilityBenchFixture): n/a
  - `<unnamed>` (RandomRead_ScalingDataSize): n/a

#### `BENCHMARK_F(ScalabilityBenchFixture, RangeScan_ScalingDataSize)(benchmark`
- Source: `benchmarks/base/bench_scalability_comprehensive.cpp`:144
- Brief: n/a
- Parameters:
  - `<unnamed>` (ScalabilityBenchFixture): n/a
  - `<unnamed>` (RangeScan_ScalingDataSize): n/a

#### `BENCHMARK_F(ScalabilityBenchFixture, SequentialRead_ScalingDataSize)(benchmark`
- Source: `benchmarks/base/bench_scalability_comprehensive.cpp`:127
- Brief: n/a
- Parameters:
  - `<unnamed>` (ScalabilityBenchFixture): n/a
  - `<unnamed>` (SequentialRead_ScalingDataSize): n/a

#### `BENCHMARK_F(ScalabilityBenchFixture, UpdateWrite_ScalingDataSize)(benchmark`
- Source: `benchmarks/base/bench_scalability_comprehensive.cpp`:195
- Brief: n/a
- Parameters:
  - `<unnamed>` (ScalabilityBenchFixture): n/a
  - `<unnamed>` (UpdateWrite_ScalingDataSize): n/a

#### `void BM_BatchSizeScaling(benchmark::State &state)`
- Source: `benchmarks/base/bench_scalability_comprehensive.cpp`:267
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_ContinuousGrowth(benchmark::State &state)`
- Source: `benchmarks/base/bench_scalability_comprehensive.cpp`:418
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_FullScanScaling(benchmark::State &state)`
- Source: `benchmarks/base/bench_scalability_comprehensive.cpp`:309
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_LargeRecordInsertion(benchmark::State &state)`
- Source: `benchmarks/base/bench_scalability_comprehensive.cpp`:220
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_MemoryPressure(benchmark::State &state)`
- Source: `benchmarks/base/bench_scalability_comprehensive.cpp`:359
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `Unit(benchmark::kMicrosecond) -> Arg(1000) ->Arg(10000) ->Arg(100000)`
- Source: `benchmarks/base/bench_scalability_comprehensive.cpp`:124
- Brief: n/a
- Parameters:
  - `<unnamed>` (benchmark::kMicrosecond): n/a

#### `Unit(benchmark::kMillisecond) -> Arg(1024) ->Arg(10240) ->Arg(102400) ->Arg(1024000)`
- Source: `benchmarks/base/bench_scalability_comprehensive.cpp`:257
- Brief: n/a
- Parameters:
  - `<unnamed>` (benchmark::kMillisecond): n/a

### bench_simd_distance.cpp

#### `BENCHMARK(BM_SIMD_CosineDistance) -> Args({64}) ->Args({128}) ->Args({256}) ->Args({512}) ->Args({1536}) ->Unit(benchmark::kMicrosecond)`
- Source: `benchmarks/base/bench_simd_distance.cpp`:125
- Brief: n/a
- Parameters:
  - `<unnamed>` (BM_SIMD_CosineDistance): n/a

#### `BENCHMARK(BM_SIMD_InnerProduct) -> Args({64}) ->Args({128}) ->Args({256}) ->Args({512}) ->Args({1536}) ->Unit(benchmark::kMicrosecond)`
- Source: `benchmarks/base/bench_simd_distance.cpp`:89
- Brief: n/a
- Parameters:
  - `<unnamed>` (BM_SIMD_InnerProduct): n/a

#### `BENCHMARK(BM_SIMD_L2) -> Args({64}) ->Args({128}) ->Args({256}) ->Args({512}) ->Unit(benchmark::kMicrosecond)`
- Source: `benchmarks/base/bench_simd_distance.cpp`:57
- Brief: n/a
- Parameters:
  - `<unnamed>` (BM_SIMD_L2): n/a

#### `BENCHMARK(BM_Scalar_CosineDistance) -> Args({64}) ->Args({128}) ->Args({256}) ->Args({512}) ->Args({1536}) ->Unit(benchmark::kMicrosecond)`
- Source: `benchmarks/base/bench_simd_distance.cpp`:126
- Brief: n/a
- Parameters:
  - `<unnamed>` (BM_Scalar_CosineDistance): n/a

#### `BENCHMARK(BM_Scalar_InnerProduct) -> Args({64}) ->Args({128}) ->Args({256}) ->Args({512}) ->Args({1536}) ->Unit(benchmark::kMicrosecond)`
- Source: `benchmarks/base/bench_simd_distance.cpp`:90
- Brief: n/a
- Parameters:
  - `<unnamed>` (BM_Scalar_InnerProduct): n/a

#### `BENCHMARK(BM_Scalar_L2) -> Args({64}) ->Args({128}) ->Args({256}) ->Args({512}) ->Unit(benchmark::kMicrosecond)`
- Source: `benchmarks/base/bench_simd_distance.cpp`:58
- Brief: n/a
- Parameters:
  - `<unnamed>` (BM_Scalar_L2): n/a

#### `void BM_SIMD_CosineDistance(benchmark::State &state)`
- Source: `benchmarks/base/bench_simd_distance.cpp`:94
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_SIMD_InnerProduct(benchmark::State &state)`
- Source: `benchmarks/base/bench_simd_distance.cpp`:62
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_SIMD_L2(benchmark::State &state)`
- Source: `benchmarks/base/bench_simd_distance.cpp`:33
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_Scalar_CosineDistance(benchmark::State &state)`
- Source: `benchmarks/base/bench_simd_distance.cpp`:106
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_Scalar_InnerProduct(benchmark::State &state)`
- Source: `benchmarks/base/bench_simd_distance.cpp`:74
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_Scalar_L2(benchmark::State &state)`
- Source: `benchmarks/base/bench_simd_distance.cpp`:45
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

### test_base_entity.cpp

#### `TEST_F(BaseEntityTest, BlobOperations)`
- Source: `tests/base/test_base_entity.cpp`:255
- Brief: n/a
- Parameters:
  - `<unnamed>` (BaseEntityTest): n/a
  - `<unnamed>` (BlobOperations): n/a

#### `TEST_F(BaseEntityTest, CacheInvalidationOnSetBlob)`
- Source: `tests/base/test_base_entity.cpp`:282
- Brief: n/a
- Parameters:
  - `<unnamed>` (BaseEntityTest): n/a
  - `<unnamed>` (CacheInvalidationOnSetBlob): n/a

#### `TEST_F(BaseEntityTest, Clear)`
- Source: `tests/base/test_base_entity.cpp`:244
- Brief: n/a
- Parameters:
  - `<unnamed>` (BaseEntityTest): n/a
  - `<unnamed>` (Clear): n/a

#### `TEST_F(BaseEntityTest, ConstructorWithFields)`
- Source: `tests/base/test_base_entity.cpp`:33
- Brief: n/a
- Parameters:
  - `<unnamed>` (BaseEntityTest): n/a
  - `<unnamed>` (ConstructorWithFields): n/a

#### `TEST_F(BaseEntityTest, ConstructorWithPK)`
- Source: `tests/base/test_base_entity.cpp`:21
- Brief: n/a
- Parameters:
  - `<unnamed>` (BaseEntityTest): n/a
  - `<unnamed>` (ConstructorWithPK): n/a

#### `TEST_F(BaseEntityTest, ExtractAllFields)`
- Source: `tests/base/test_base_entity.cpp`:213
- Brief: n/a
- Parameters:
  - `<unnamed>` (BaseEntityTest): n/a
  - `<unnamed>` (ExtractAllFields): n/a

#### `TEST_F(BaseEntityTest, ExtractField)`
- Source: `tests/base/test_base_entity.cpp`:197
- Brief: n/a
- Parameters:
  - `<unnamed>` (BaseEntityTest): n/a
  - `<unnamed>` (ExtractField): n/a

#### `TEST_F(BaseEntityTest, ExtractFieldsWithPrefix)`
- Source: `tests/base/test_base_entity.cpp`:228
- Brief: n/a
- Parameters:
  - `<unnamed>` (BaseEntityTest): n/a
  - `<unnamed>` (ExtractFieldsWithPrefix): n/a

#### `TEST_F(BaseEntityTest, FromJsonSimple)`
- Source: `tests/base/test_base_entity.cpp`:119
- Brief: n/a
- Parameters:
  - `<unnamed>` (BaseEntityTest): n/a
  - `<unnamed>` (FromJsonSimple): n/a

#### `TEST_F(BaseEntityTest, FromJsonWithVector)`
- Source: `tests/base/test_base_entity.cpp`:129
- Brief: n/a
- Parameters:
  - `<unnamed>` (BaseEntityTest): n/a
  - `<unnamed>` (FromJsonWithVector): n/a

#### `TEST_F(BaseEntityTest, GetAllFields)`
- Source: `tests/base/test_base_entity.cpp`:102
- Brief: n/a
- Parameters:
  - `<unnamed>` (BaseEntityTest): n/a
  - `<unnamed>` (GetAllFields): n/a

#### `TEST_F(BaseEntityTest, HasField)`
- Source: `tests/base/test_base_entity.cpp`:94
- Brief: n/a
- Parameters:
  - `<unnamed>` (BaseEntityTest): n/a
  - `<unnamed>` (HasField): n/a

#### `TEST_F(BaseEntityTest, NonNegativeRotationPosition)`
- Source: `tests/base/test_base_entity.cpp`:350
- Brief: n/a
- Parameters:
  - `<unnamed>` (BaseEntityTest): n/a
  - `<unnamed>` (NonNegativeRotationPosition): n/a

#### `TEST_F(BaseEntityTest, ParseInvalidJsonDoesNotCrash)`
- Source: `tests/base/test_base_entity.cpp`:269
- Brief: n/a
- Parameters:
  - `<unnamed>` (BaseEntityTest): n/a
  - `<unnamed>` (ParseInvalidJsonDoesNotCrash): n/a

#### `TEST_F(BaseEntityTest, SafeUInt64Conversion)`
- Source: `tests/base/test_base_entity.cpp`:297
- Brief: n/a
- Parameters:
  - `<unnamed>` (BaseEntityTest): n/a
  - `<unnamed>` (SafeUInt64Conversion): n/a

#### `TEST_F(BaseEntityTest, SerializeDeserializeRoundtrip)`
- Source: `tests/base/test_base_entity.cpp`:160
- Brief: n/a
- Parameters:
  - `<unnamed>` (BaseEntityTest): n/a
  - `<unnamed>` (SerializeDeserializeRoundtrip): n/a

#### `TEST_F(BaseEntityTest, SerializeWithVector)`
- Source: `tests/base/test_base_entity.cpp`:180
- Brief: n/a
- Parameters:
  - `<unnamed>` (BaseEntityTest): n/a
  - `<unnamed>` (SerializeWithVector): n/a

#### `TEST_F(BaseEntityTest, SetAndGetBoolField)`
- Source: `tests/base/test_base_entity.cpp`:73
- Brief: n/a
- Parameters:
  - `<unnamed>` (BaseEntityTest): n/a
  - `<unnamed>` (SetAndGetBoolField): n/a

#### `TEST_F(BaseEntityTest, SetAndGetDoubleField)`
- Source: `tests/base/test_base_entity.cpp`:64
- Brief: n/a
- Parameters:
  - `<unnamed>` (BaseEntityTest): n/a
  - `<unnamed>` (SetAndGetDoubleField): n/a

#### `TEST_F(BaseEntityTest, SetAndGetIntField)`
- Source: `tests/base/test_base_entity.cpp`:55
- Brief: n/a
- Parameters:
  - `<unnamed>` (BaseEntityTest): n/a
  - `<unnamed>` (SetAndGetIntField): n/a

#### `TEST_F(BaseEntityTest, SetAndGetPrimaryKey)`
- Source: `tests/base/test_base_entity.cpp`:27
- Brief: n/a
- Parameters:
  - `<unnamed>` (BaseEntityTest): n/a
  - `<unnamed>` (SetAndGetPrimaryKey): n/a

#### `TEST_F(BaseEntityTest, SetAndGetStringField)`
- Source: `tests/base/test_base_entity.cpp`:46
- Brief: n/a
- Parameters:
  - `<unnamed>` (BaseEntityTest): n/a
  - `<unnamed>` (SetAndGetStringField): n/a

#### `TEST_F(BaseEntityTest, SetAndGetVectorField)`
- Source: `tests/base/test_base_entity.cpp`:82
- Brief: n/a
- Parameters:
  - `<unnamed>` (BaseEntityTest): n/a
  - `<unnamed>` (SetAndGetVectorField): n/a

#### `TEST_F(BaseEntityTest, ToJson)`
- Source: `tests/base/test_base_entity.cpp`:140
- Brief: n/a
- Parameters:
  - `<unnamed>` (BaseEntityTest): n/a
  - `<unnamed>` (ToJson): n/a

#### `TEST_F(BaseEntityTest, UINT32SafeConversion)`
- Source: `tests/base/test_base_entity.cpp`:333
- Brief: n/a
- Parameters:
  - `<unnamed>` (BaseEntityTest): n/a
  - `<unnamed>` (UINT32SafeConversion): n/a

#### `TEST_F(BaseEntityTest, UINT64OverflowClamping)`
- Source: `tests/base/test_base_entity.cpp`:316
- Brief: n/a
- Parameters:
  - `<unnamed>` (BaseEntityTest): n/a
  - `<unnamed>` (UINT64OverflowClamping): n/a

### test_base_interfaces.cpp

#### `TEST(BaseInterfaces, HasVirtualDestructors)`
- Source: `tests/base/test_base_interfaces.cpp`:55
- Brief: n/a
- Parameters:
  - `<unnamed>` (BaseInterfaces): n/a
  - `<unnamed>` (HasVirtualDestructors): n/a

#### `TEST(BaseInterfaces, IndexInterfaceIsAbstract)`
- Source: `tests/base/test_base_interfaces.cpp`:41
- Brief: n/a
- Parameters:
  - `<unnamed>` (BaseInterfaces): n/a
  - `<unnamed>` (IndexInterfaceIsAbstract): n/a

#### `TEST(BaseInterfaces, QueryInterfaceIsAbstract)`
- Source: `tests/base/test_base_interfaces.cpp`:35
- Brief: n/a
- Parameters:
  - `<unnamed>` (BaseInterfaces): n/a
  - `<unnamed>` (QueryInterfaceIsAbstract): n/a

#### `TEST(BaseInterfaces, SecurityInterfaceIsAbstract)`
- Source: `tests/base/test_base_interfaces.cpp`:48
- Brief: n/a
- Parameters:
  - `<unnamed>` (BaseInterfaces): n/a
  - `<unnamed>` (SecurityInterfaceIsAbstract): n/a

#### `TEST(BaseInterfaces, StorageInterfaceIsAbstract)`
- Source: `tests/base/test_base_interfaces.cpp`:30
- Brief: n/a
- Parameters:
  - `<unnamed>` (BaseInterfaces): n/a
  - `<unnamed>` (StorageInterfaceIsAbstract): n/a

#### `TEST(GraphEdge, Construction)`
- Source: `tests/base/test_base_interfaces.cpp`:656
- Brief: n/a
- Parameters:
  - `<unnamed>` (GraphEdge): n/a
  - `<unnamed>` (Construction): n/a

#### `TEST(GraphEdge, DefaultWeight)`
- Source: `tests/base/test_base_interfaces.cpp`:664
- Brief: n/a
- Parameters:
  - `<unnamed>` (GraphEdge): n/a
  - `<unnamed>` (DefaultWeight): n/a

#### `TEST(MockFieldEncryption, EncryptDecryptRoundTrip)`
- Source: `tests/base/test_base_interfaces.cpp`:466
- Brief: n/a
- Parameters:
  - `<unnamed>` (MockFieldEncryption): n/a
  - `<unnamed>` (EncryptDecryptRoundTrip): n/a

#### `TEST(MockFieldEncryption, ShouldEncryptSelectedFields)`
- Source: `tests/base/test_base_interfaces.cpp`:477
- Brief: n/a
- Parameters:
  - `<unnamed>` (MockFieldEncryption): n/a
  - `<unnamed>` (ShouldEncryptSelectedFields): n/a

#### `TEST(MockGraphIndex, FindShortestPathDirect)`
- Source: `tests/base/test_base_interfaces.cpp`:635
- Brief: n/a
- Parameters:
  - `<unnamed>` (MockGraphIndex): n/a
  - `<unnamed>` (FindShortestPathDirect): n/a

#### `TEST(MockGraphIndex, FindShortestPathNoPath)`
- Source: `tests/base/test_base_interfaces.cpp`:644
- Brief: n/a
- Parameters:
  - `<unnamed>` (MockGraphIndex): n/a
  - `<unnamed>` (FindShortestPathNoPath): n/a

#### `TEST(MockGraphIndex, GetIncomingEdges)`
- Source: `tests/base/test_base_interfaces.cpp`:619
- Brief: n/a
- Parameters:
  - `<unnamed>` (MockGraphIndex): n/a
  - `<unnamed>` (GetIncomingEdges): n/a

#### `TEST(MockGraphIndex, GetOutgoingEdges)`
- Source: `tests/base/test_base_interfaces.cpp`:600
- Brief: n/a
- Parameters:
  - `<unnamed>` (MockGraphIndex): n/a
  - `<unnamed>` (GetOutgoingEdges): n/a

#### `TEST(MockGraphIndex, GetOutgoingEdgesFilteredByType)`
- Source: `tests/base/test_base_interfaces.cpp`:609
- Brief: n/a
- Parameters:
  - `<unnamed>` (MockGraphIndex): n/a
  - `<unnamed>` (GetOutgoingEdgesFilteredByType): n/a

#### `TEST(MockGraphIndex, InsertEdge)`
- Source: `tests/base/test_base_interfaces.cpp`:595
- Brief: n/a
- Parameters:
  - `<unnamed>` (MockGraphIndex): n/a
  - `<unnamed>` (InsertEdge): n/a

#### `TEST(MockGraphIndex, Metadata)`
- Source: `tests/base/test_base_interfaces.cpp`:650
- Brief: n/a
- Parameters:
  - `<unnamed>` (MockGraphIndex): n/a
  - `<unnamed>` (Metadata): n/a

#### `TEST(MockGraphIndex, RemoveEdge)`
- Source: `tests/base/test_base_interfaces.cpp`:628
- Brief: n/a
- Parameters:
  - `<unnamed>` (MockGraphIndex): n/a
  - `<unnamed>` (RemoveEdge): n/a

#### `TEST(MockKeyProvider, GetKeyReturnsBytes)`
- Source: `tests/base/test_base_interfaces.cpp`:487
- Brief: n/a
- Parameters:
  - `<unnamed>` (MockKeyProvider): n/a
  - `<unnamed>` (GetKeyReturnsBytes): n/a

#### `TEST(MockKeyProvider, RotateKeyReturnsDifferentBytes)`
- Source: `tests/base/test_base_interfaces.cpp`:494
- Brief: n/a
- Parameters:
  - `<unnamed>` (MockKeyProvider): n/a
  - `<unnamed>` (RotateKeyReturnsDifferentBytes): n/a

#### `TEST(MockQueryEngine, CreateExpressionEvaluator)`
- Source: `tests/base/test_base_interfaces.cpp`:434
- Brief: n/a
- Parameters:
  - `<unnamed>` (MockQueryEngine): n/a
  - `<unnamed>` (CreateExpressionEvaluator): n/a

#### `TEST(MockQueryEngine, Execute)`
- Source: `tests/base/test_base_interfaces.cpp`:421
- Brief: n/a
- Parameters:
  - `<unnamed>` (MockQueryEngine): n/a
  - `<unnamed>` (Execute): n/a

#### `TEST(MockQueryEngine, ExplainQuery)`
- Source: `tests/base/test_base_interfaces.cpp`:443
- Brief: n/a
- Parameters:
  - `<unnamed>` (MockQueryEngine): n/a
  - `<unnamed>` (ExplainQuery): n/a

#### `TEST(MockQueryEngine, Validate)`
- Source: `tests/base/test_base_interfaces.cpp`:428
- Brief: n/a
- Parameters:
  - `<unnamed>` (MockQueryEngine): n/a
  - `<unnamed>` (Validate): n/a

#### `TEST(MockSecondaryIndex, InsertAndLookup)`
- Source: `tests/base/test_base_interfaces.cpp`:506
- Brief: n/a
- Parameters:
  - `<unnamed>` (MockSecondaryIndex): n/a
  - `<unnamed>` (InsertAndLookup): n/a

#### `TEST(MockSecondaryIndex, LookupMissingValue)`
- Source: `tests/base/test_base_interfaces.cpp`:515
- Brief: n/a
- Parameters:
  - `<unnamed>` (MockSecondaryIndex): n/a
  - `<unnamed>` (LookupMissingValue): n/a

#### `TEST(MockSecondaryIndex, Metadata)`
- Source: `tests/base/test_base_interfaces.cpp`:543
- Brief: n/a
- Parameters:
  - `<unnamed>` (MockSecondaryIndex): n/a
  - `<unnamed>` (Metadata): n/a

#### `TEST(MockSecondaryIndex, RangeScan)`
- Source: `tests/base/test_base_interfaces.cpp`:533
- Brief: n/a
- Parameters:
  - `<unnamed>` (MockSecondaryIndex): n/a
  - `<unnamed>` (RangeScan): n/a

#### `TEST(MockSecondaryIndex, RemoveEntry)`
- Source: `tests/base/test_base_interfaces.cpp`:521
- Brief: n/a
- Parameters:
  - `<unnamed>` (MockSecondaryIndex): n/a
  - `<unnamed>` (RemoveEntry): n/a

#### `TEST(MockSecondaryIndex, RemoveNonexistentReturnsFalse)`
- Source: `tests/base/test_base_interfaces.cpp`:528
- Brief: n/a
- Parameters:
  - `<unnamed>` (MockSecondaryIndex): n/a
  - `<unnamed>` (RemoveNonexistentReturnsFalse): n/a

#### `TEST(MockStorageEngine, DeleteExistingKey)`
- Source: `tests/base/test_base_interfaces.cpp`:372
- Brief: n/a
- Parameters:
  - `<unnamed>` (MockStorageEngine): n/a
  - `<unnamed>` (DeleteExistingKey): n/a

#### `TEST(MockStorageEngine, DeleteNonexistentReturnsError)`
- Source: `tests/base/test_base_interfaces.cpp`:382
- Brief: n/a
- Parameters:
  - `<unnamed>` (MockStorageEngine): n/a
  - `<unnamed>` (DeleteNonexistentReturnsError): n/a

#### `TEST(MockStorageEngine, GetNonexistentReturnsError)`
- Source: `tests/base/test_base_interfaces.cpp`:366
- Brief: n/a
- Parameters:
  - `<unnamed>` (MockStorageEngine): n/a
  - `<unnamed>` (GetNonexistentReturnsError): n/a

#### `TEST(MockStorageEngine, OpenAndClose)`
- Source: `tests/base/test_base_interfaces.cpp`:347
- Brief: n/a
- Parameters:
  - `<unnamed>` (MockStorageEngine): n/a
  - `<unnamed>` (OpenAndClose): n/a

#### `TEST(MockStorageEngine, PolymorphicUsage)`
- Source: `tests/base/test_base_interfaces.cpp`:407
- Brief: n/a
- Parameters:
  - `<unnamed>` (MockStorageEngine): n/a
  - `<unnamed>` (PolymorphicUsage): n/a

#### `TEST(MockStorageEngine, PutAndGet)`
- Source: `tests/base/test_base_interfaces.cpp`:356
- Brief: n/a
- Parameters:
  - `<unnamed>` (MockStorageEngine): n/a
  - `<unnamed>` (PutAndGet): n/a

#### `TEST(MockStorageEngine, ScanPrefixDefaultImplementation)`
- Source: `tests/base/test_base_interfaces.cpp`:398
- Brief: n/a
- Parameters:
  - `<unnamed>` (MockStorageEngine): n/a
  - `<unnamed>` (ScanPrefixDefaultImplementation): n/a

#### `TEST(MockStorageEngine, ScanRangeDefaultImplementation)`
- Source: `tests/base/test_base_interfaces.cpp`:388
- Brief: n/a
- Parameters:
  - `<unnamed>` (MockStorageEngine): n/a
  - `<unnamed>` (ScanRangeDefaultImplementation): n/a

#### `TEST(MockVectorIndex, InsertAndSearch)`
- Source: `tests/base/test_base_interfaces.cpp`:554
- Brief: n/a
- Parameters:
  - `<unnamed>` (MockVectorIndex): n/a
  - `<unnamed>` (InsertAndSearch): n/a

#### `TEST(MockVectorIndex, Metadata)`
- Source: `tests/base/test_base_interfaces.cpp`:578
- Brief: n/a
- Parameters:
  - `<unnamed>` (MockVectorIndex): n/a
  - `<unnamed>` (Metadata): n/a

#### `TEST(MockVectorIndex, RangeSearch)`
- Source: `tests/base/test_base_interfaces.cpp`:563
- Brief: n/a
- Parameters:
  - `<unnamed>` (MockVectorIndex): n/a
  - `<unnamed>` (RangeSearch): n/a

#### `TEST(MockVectorIndex, RemoveVector)`
- Source: `tests/base/test_base_interfaces.cpp`:571
- Brief: n/a
- Parameters:
  - `<unnamed>` (MockVectorIndex): n/a
  - `<unnamed>` (RemoveVector): n/a

#### `TEST(QueryResult, DefaultState)`
- Source: `tests/base/test_base_interfaces.cpp`:450
- Brief: n/a
- Parameters:
  - `<unnamed>` (QueryResult): n/a
  - `<unnamed>` (DefaultState): n/a

#### `TEST(QueryResult, ErrorState)`
- Source: `tests/base/test_base_interfaces.cpp`:456
- Brief: n/a
- Parameters:
  - `<unnamed>` (QueryResult): n/a
  - `<unnamed>` (ErrorState): n/a

#### `TEST(VectorSearchResult, Construction)`
- Source: `tests/base/test_base_interfaces.cpp`:585
- Brief: n/a
- Parameters:
  - `<unnamed>` (VectorSearchResult): n/a
  - `<unnamed>` (Construction): n/a

### themis::modules

#### `TEST(FailClosedTaxonomyTest, AllLoaderSandboxCodesKnown)`
- Source: `tests/base/test_base_wave_d_tracing.cpp`:316
- Brief: All loader and sandbox codes are known to resolveDescription().
- Parameters:
  - `<unnamed>` (FailClosedTaxonomyTest): n/a
  - `<unnamed>` (AllLoaderSandboxCodesKnown): n/a

#### `TEST(FailClosedTaxonomyTest, LoaderPathNotFoundCodeAndDescription)`
- Source: `tests/base/test_base_wave_d_tracing.cpp`:297
- Brief: BASE_LOADER_PATH_NOT_FOUND: format and description are non-empty; code is 1100.
- Parameters:
  - `<unnamed>` (FailClosedTaxonomyTest): n/a
  - `<unnamed>` (LoaderPathNotFoundCodeAndDescription): n/a

#### `TEST(FailClosedTaxonomyTest, SandboxDegradedCodeAndRemediationHint)`
- Source: `tests/base/test_base_wave_d_tracing.cpp`:309
- Brief: BASE_SANDBOX_DEGRADED: code is 1153 with non-empty remediation hint.
- Parameters:
  - `<unnamed>` (FailClosedTaxonomyTest): n/a
  - `<unnamed>` (SandboxDegradedCodeAndRemediationHint): n/a

#### `TEST(HighCardinalityStressTest, ConcurrentRegistration)`
- Source: `tests/base/test_base_wave_d_tracing.cpp`:197
- Brief: 500 concurrent registerModule() calls must not corrupt internal state.
- Parameters:
  - `<unnamed>` (HighCardinalityStressTest): n/a
  - `<unnamed>` (ConcurrentRegistration): n/a

#### `TEST(HighCardinalityStressTest, ConcurrentUnregisteredReloads)`
- Source: `tests/base/test_base_wave_d_tracing.cpp`:170
- Brief: n/a
- Parameters:
  - `<unnamed>` (HighCardinalityStressTest): n/a
  - `<unnamed>` (ConcurrentUnregisteredReloads): n/a
- Details: 200 concurrent reloadModule() calls on unregistered modules must all fail cleanly, not crash, and each produce exactly one matching span pair.

#### `TEST(HighCardinalityStressTest, MixedConcurrentOperations)`
- Source: `tests/base/test_base_wave_d_tracing.cpp`:218
- Brief: n/a
- Parameters:
  - `<unnamed>` (HighCardinalityStressTest): n/a
  - `<unnamed>` (MixedConcurrentOperations): n/a
- Details: Mixed concurrent load of registerModule + reloadModule + rollback does not crash and leaves stats consistent (total ≥ successful + failed).

#### `TEST(NoOpSpanEmitterTest, DoesNotThrow)`
- Source: `tests/base/test_base_wave_d_tracing.cpp`:403
- Brief: noOpSpanEmitter does not throw and discards events silently.
- Parameters:
  - `<unnamed>` (NoOpSpanEmitterTest): n/a
  - `<unnamed>` (DoesNotThrow): n/a

#### `TEST(ScopedSpanTest, EmitsStartAndEndEvents)`
- Source: `tests/base/test_base_wave_d_tracing.cpp`:369
- Brief: ScopedSpan emits a start event on construction and end event on destruction.
- Parameters:
  - `<unnamed>` (ScopedSpanTest): n/a
  - `<unnamed>` (EmitsStartAndEndEvents): n/a

#### `TEST(ScopedSpanTest, SetErrorPropagates)`
- Source: `tests/base/test_base_wave_d_tracing.cpp`:386
- Brief: ScopedSpan setError() propagates to the end event.
- Parameters:
  - `<unnamed>` (ScopedSpanTest): n/a
  - `<unnamed>` (SetErrorPropagates): n/a

#### `TEST(SoakTest, ConcurrentSoakLoops)`
- Source: `tests/base/test_base_soak.cpp`:136
- Brief: Soak: concurrent soak with N threads each doing M iterations.
- Parameters:
  - `<unnamed>` (SoakTest): n/a
  - `<unnamed>` (ConcurrentSoakLoops): n/a
- Details: Validates that concurrent reload attempts against N independent modules do not corrupt the global stats counter.

#### `TEST(SoakTest, ReloadRollbackReregisterCycle)`
- Source: `tests/base/test_base_soak.cpp`:65
- Brief: Long-duration soak: simulated reload/rollback/re-register loop.
- Parameters:
  - `<unnamed>` (SoakTest): n/a
  - `<unnamed>` (ReloadRollbackReregisterCycle): n/a
- Details: Each iteration: Attempt reloadModule() on a registered module with a nonexistent path (→ fails cleanly; failedReloads increments). Attempt rollback() with no backup (→ fails cleanly; rollbacks stays 0). Unregister and re-register the module. Invariants validated at the end: totalReloads == failedReloads (all fail because path is nonexistent) totalReloads equals the configured iteration count rollbacks == 0 (no rollback ever had a backup) successfulReloads == 0

#### `TEST(SoakTest, StatsMonotonicity)`
- Source: `tests/base/test_base_soak.cpp`:106
- Brief: Soak: stats monotonicity — totalReloads never decreases.
- Parameters:
  - `<unnamed>` (SoakTest): n/a
  - `<unnamed>` (StatsMonotonicity): n/a
- Details: Runs a smaller number of iterations and samples stats at each step to confirm that all counters are non-decreasing.

#### `TEST(TraceContextTest, ChildSharesTraceId)`
- Source: `tests/base/test_base_wave_d_tracing.cpp`:351
- Brief: child() shares the trace ID and sets parent_span_id.
- Parameters:
  - `<unnamed>` (TraceContextTest): n/a
  - `<unnamed>` (ChildSharesTraceId): n/a

#### `TEST(TraceContextTest, GenerateProducesValidContext)`
- Source: `tests/base/test_base_wave_d_tracing.cpp`:341
- Brief: generate() produces valid contexts with non-zero IDs.
- Parameters:
  - `<unnamed>` (TraceContextTest): n/a
  - `<unnamed>` (GenerateProducesValidContext): n/a

#### `TEST(TraceContextTest, UniqueIds)`
- Source: `tests/base/test_base_wave_d_tracing.cpp`:361
- Brief: Successive generate() calls produce unique IDs.
- Parameters:
  - `<unnamed>` (TraceContextTest): n/a
  - `<unnamed>` (UniqueIds): n/a

#### `TEST(TracingDefaultEmitterTest, NoOpEmitterIsDefault)`
- Source: `tests/base/test_base_wave_d_tracing.cpp`:127
- Brief: The default no-op emitter produces no observable side-effects.
- Parameters:
  - `<unnamed>` (TracingDefaultEmitterTest): n/a
  - `<unnamed>` (NoOpEmitterIsDefault): n/a

#### `TEST(TracingEmitterAccessTest, SpanEmitterGetterNonNull)`
- Source: `tests/base/test_base_wave_d_tracing.cpp`:152
- Brief: spanEmitter() returns the currently installed emitter (non-null).
- Parameters:
  - `<unnamed>` (TracingEmitterAccessTest): n/a
  - `<unnamed>` (SpanEmitterGetterNonNull): n/a

#### `TEST(TracingEmitterReplacement, ReplacementTakesEffect)`
- Source: `tests/base/test_base_wave_d_tracing.cpp`:137
- Brief: setSpanEmitter() replaces the emitter; subsequent operations use the new one.
- Parameters:
  - `<unnamed>` (TracingEmitterReplacement): n/a
  - `<unnamed>` (ReplacementTakesEffect): n/a

#### `TEST_F(AbiCheckerSymbolTest, DeprecatedSymbolCheckNullHandleNoThrow)`
- Source: `tests/base/test_base_future_enhancements.cpp`:128
- Brief: A deprecated-symbol check on a null handle must not crash.
- Parameters:
  - `<unnamed>` (AbiCheckerSymbolTest): n/a
  - `<unnamed>` (DeprecatedSymbolCheckNullHandleNoThrow): n/a

#### `TEST_F(AbiCheckerSymbolTest, EmptyRequiredSymbolListPasses)`
- Source: `tests/base/test_base_future_enhancements.cpp`:115
- Brief: An empty required-symbol list always passes the symbol check.
- Parameters:
  - `<unnamed>` (AbiCheckerSymbolTest): n/a
  - `<unnamed>` (EmptyRequiredSymbolListPasses): n/a

#### `TEST_F(AbiCheckerSymbolTest, UseDefaultListsIsCallable)`
- Source: `tests/base/test_base_future_enhancements.cpp`:138
- Brief: useDefaultLists() populates required symbols (check is callable after).
- Parameters:
  - `<unnamed>` (AbiCheckerSymbolTest): n/a
  - `<unnamed>` (UseDefaultListsIsCallable): n/a

#### `TEST_F(AbiCheckerVersionTest, MajorMismatchIncompatible)`
- Source: `tests/base/test_base_future_enhancements.cpp`:58
- Brief: Major version mismatch renders the module incompatible.
- Parameters:
  - `<unnamed>` (AbiCheckerVersionTest): n/a
  - `<unnamed>` (MajorMismatchIncompatible): n/a

#### `TEST_F(AbiCheckerVersionTest, ModuleMinorAheadOfHostIncompatible)`
- Source: `tests/base/test_base_future_enhancements.cpp`:71
- Brief: Module minor > host minor is a forward-compatibility violation.
- Parameters:
  - `<unnamed>` (AbiCheckerVersionTest): n/a
  - `<unnamed>` (ModuleMinorAheadOfHostIncompatible): n/a

#### `TEST_F(AbiCheckerVersionTest, ModuleMinorBehindHostCompatible)`
- Source: `tests/base/test_base_future_enhancements.cpp`:84
- Brief: Module minor ≤ host minor is backward-compatible.
- Parameters:
  - `<unnamed>` (AbiCheckerVersionTest): n/a
  - `<unnamed>` (ModuleMinorBehindHostCompatible): n/a

#### `TEST_F(AbiCheckerVersionTest, PatchDifferenceDoesNotBreakCompat)`
- Source: `tests/base/test_base_future_enhancements.cpp`:97
- Brief: Patch difference with matching major.minor is compatible.
- Parameters:
  - `<unnamed>` (AbiCheckerVersionTest): n/a
  - `<unnamed>` (PatchDifferenceDoesNotBreakCompat): n/a

#### `TEST_F(AbiCheckerVersionTest, SameMajorMinorIsCompatible)`
- Source: `tests/base/test_base_future_enhancements.cpp`:46
- Brief: Same major and minor versions are compatible.
- Parameters:
  - `<unnamed>` (AbiCheckerVersionTest): n/a
  - `<unnamed>` (SameMajorMinorIsCompatible): n/a

#### `TEST_F(AdvancedDependencyTest, DiamondDependencyResolvesCorrectly)`
- Source: `tests/base/test_base_future_enhancements.cpp`:410
- Brief: n/a
- Parameters:
  - `<unnamed>` (AdvancedDependencyTest): n/a
  - `<unnamed>` (DiamondDependencyResolvesCorrectly): n/a
- Details: Diamond dependency: A depends on B and C, both B and C depend on D. D must appear once in the load order and before B, C, and A.

#### `TEST_F(AdvancedDependencyTest, FiveLevelChainCorrectOrder)`
- Source: `tests/base/test_base_future_enhancements.cpp`:477
- Brief: 5-level deep chain resolves in the correct activation order.
- Parameters:
  - `<unnamed>` (AdvancedDependencyTest): n/a
  - `<unnamed>` (FiveLevelChainCorrectOrder): n/a

#### `TEST_F(AdvancedDependencyTest, IndirectCycleDetected)`
- Source: `tests/base/test_base_future_enhancements.cpp`:504
- Brief: Indirect cycle (A→B→C→A) is detected.
- Parameters:
  - `<unnamed>` (AdvancedDependencyTest): n/a
  - `<unnamed>` (IndirectCycleDetected): n/a

#### `TEST_F(AdvancedDependencyTest, MultiRootIndependentModulesResolve)`
- Source: `tests/base/test_base_future_enhancements.cpp`:452
- Brief: Multi-root: two independent modules with no shared dependencies resolve together.
- Parameters:
  - `<unnamed>` (AdvancedDependencyTest): n/a
  - `<unnamed>` (MultiRootIndependentModulesResolve): n/a

#### `TEST_F(AdvancedDependencyTest, ReRegisterModuleDoesNotDuplicate)`
- Source: `tests/base/test_base_future_enhancements.cpp`:463
- Brief: Re-registering a module with the same name does not accumulate duplicates.
- Parameters:
  - `<unnamed>` (AdvancedDependencyTest): n/a
  - `<unnamed>` (ReRegisterModuleDoesNotDuplicate): n/a

#### `TEST_F(BaseErrorTaxonomyTest, AllCodesAreUnique)`
- Source: `tests/base/test_base_reload_regression.cpp`:925
- Brief: All error codes are unique (no two taxonomy entries share a code).
- Parameters:
  - `<unnamed>` (BaseErrorTaxonomyTest): n/a
  - `<unnamed>` (AllCodesAreUnique): n/a

#### `TEST_F(BaseErrorTaxonomyTest, AllDescriptionsNonEmpty)`
- Source: `tests/base/test_base_reload_regression.cpp`:985
- Brief: All descriptions are non-empty.
- Parameters:
  - `<unnamed>` (BaseErrorTaxonomyTest): n/a
  - `<unnamed>` (AllDescriptionsNonEmpty): n/a

#### `TEST_F(BaseErrorTaxonomyTest, CodesInCorrectRanges)`
- Source: `tests/base/test_base_reload_regression.cpp`:960
- Brief: All codes fall within their declared range.
- Parameters:
  - `<unnamed>` (BaseErrorTaxonomyTest): n/a
  - `<unnamed>` (CodesInCorrectRanges): n/a

#### `TEST_F(BaseErrorTaxonomyTest, FormatOutputContainsCode)`
- Source: `tests/base/test_base_reload_regression.cpp`:1013
- Brief: format() output contains the error code as a substring.
- Parameters:
  - `<unnamed>` (BaseErrorTaxonomyTest): n/a
  - `<unnamed>` (FormatOutputContainsCode): n/a

#### `TEST_F(BaseErrorTaxonomyTest, FormatOutputContainsModuleName)`
- Source: `tests/base/test_base_reload_regression.cpp`:1021
- Brief: format() output contains the module name argument.
- Parameters:
  - `<unnamed>` (BaseErrorTaxonomyTest): n/a
  - `<unnamed>` (FormatOutputContainsModuleName): n/a

#### `TEST_F(BaseErrorTaxonomyTest, IsKnownCodeCorrect)`
- Source: `tests/base/test_base_reload_regression.cpp`:1072
- Brief: isKnownCode is true for every defined code and false for unknowns.
- Parameters:
  - `<unnamed>` (BaseErrorTaxonomyTest): n/a
  - `<unnamed>` (IsKnownCodeCorrect): n/a

#### `TEST_F(BaseErrorTaxonomyTest, ResolveDescriptionCoversAllKnownCodes)`
- Source: `tests/base/test_base_reload_regression.cpp`:1027
- Brief: resolveDescription returns a non-empty string for all known codes.
- Parameters:
  - `<unnamed>` (BaseErrorTaxonomyTest): n/a
  - `<unnamed>` (ResolveDescriptionCoversAllKnownCodes): n/a

#### `TEST_F(BaseErrorTaxonomyTest, ResolveDescriptionUnknownCodeReturnsUnknown)`
- Source: `tests/base/test_base_reload_regression.cpp`:1065
- Brief: resolveDescription returns "unknown error code" for out-of-range values.
- Parameters:
  - `<unnamed>` (BaseErrorTaxonomyTest): n/a
  - `<unnamed>` (ResolveDescriptionUnknownCodeReturnsUnknown): n/a

#### `TEST_F(DependencyResolverEdgeTest, ClearResetsResolver)`
- Source: `tests/base/test_base_reload_regression.cpp`:641
- Brief: clear() removes all registrations.
- Parameters:
  - `<unnamed>` (DependencyResolverEdgeTest): n/a
  - `<unnamed>` (ClearResetsResolver): n/a

#### `TEST_F(DependencyResolverEdgeTest, DirectCyclicDependencyDetected)`
- Source: `tests/base/test_base_reload_regression.cpp`:570
- Brief: Direct circular dependency is detected.
- Parameters:
  - `<unnamed>` (DependencyResolverEdgeTest): n/a
  - `<unnamed>` (DirectCyclicDependencyDetected): n/a

#### `TEST_F(DependencyResolverEdgeTest, LinearChainResolvesInOrder)`
- Source: `tests/base/test_base_reload_regression.cpp`:544
- Brief: Linear chain resolves successfully with correct order.
- Parameters:
  - `<unnamed>` (DependencyResolverEdgeTest): n/a
  - `<unnamed>` (LinearChainResolvesInOrder): n/a

#### `TEST_F(DependencyResolverEdgeTest, MissingOptionalDependencyDoesNotFail)`
- Source: `tests/base/test_base_reload_regression.cpp`:626
- Brief: Optional dependency that is absent does not cause resolution failure.
- Parameters:
  - `<unnamed>` (DependencyResolverEdgeTest): n/a
  - `<unnamed>` (MissingOptionalDependencyDoesNotFail): n/a

#### `TEST_F(DependencyResolverEdgeTest, MissingRequiredDependencyReportsError)`
- Source: `tests/base/test_base_reload_regression.cpp`:581
- Brief: A required dependency that is missing produces an error.
- Parameters:
  - `<unnamed>` (DependencyResolverEdgeTest): n/a
  - `<unnamed>` (MissingRequiredDependencyReportsError): n/a

#### `TEST_F(DependencyResolverEdgeTest, VersionConstraintSatisfiedSucceeds)`
- Source: `tests/base/test_base_reload_regression.cpp`:592
- Brief: Version-constrained dependency: compatible version satisfies constraint.
- Parameters:
  - `<unnamed>` (DependencyResolverEdgeTest): n/a
  - `<unnamed>` (VersionConstraintSatisfiedSucceeds): n/a

#### `TEST_F(DependencyResolverEdgeTest, VersionConstraintViolationReportsError)`
- Source: `tests/base/test_base_reload_regression.cpp`:610
- Brief: Version-constrained dependency: incompatible version produces mismatch.
- Parameters:
  - `<unnamed>` (DependencyResolverEdgeTest): n/a
  - `<unnamed>` (VersionConstraintViolationReportsError): n/a

#### `TEST_F(FailClosedTest, FailedReloadDoesNotMutateSlot)`
- Source: `tests/base/test_base_wave_d_tracing.cpp`:268
- Brief: n/a
- Parameters:
  - `<unnamed>` (FailClosedTest): n/a
  - `<unnamed>` (FailedReloadDoesNotMutateSlot): n/a
- Details: After a failed reloadModule(), the module slot retains the original loader (no partial-state mutation of the slot).

#### `TEST_F(FailClosedTest, RollbackUnregisteredFailsClosed)`
- Source: `tests/base/test_base_wave_d_tracing.cpp`:289
- Brief: rollback() on an unregistered module fails closed with no side-effects.
- Parameters:
  - `<unnamed>` (FailClosedTest): n/a
  - `<unnamed>` (RollbackUnregisteredFailsClosed): n/a

#### `TEST_F(MultiModuleTest, ReloadFailureOnOneModuleDoesNotAffectAnother)`
- Source: `tests/base/test_base_reload_regression.cpp`:527
- Brief: Reload failure on one module does not affect rollback availability on another.
- Parameters:
  - `<unnamed>` (MultiModuleTest): n/a
  - `<unnamed>` (ReloadFailureOnOneModuleDoesNotAffectAnother): n/a

#### `TEST_F(MultiModuleTest, TenModulesAllRegistered)`
- Source: `tests/base/test_base_reload_regression.cpp`:487
- Brief: Registering 10 modules: registeredModules() returns all names.
- Parameters:
  - `<unnamed>` (MultiModuleTest): n/a
  - `<unnamed>` (TenModulesAllRegistered): n/a

#### `TEST_F(MultiModuleTest, UnregisterOneDoesNotAffectOthers)`
- Source: `tests/base/test_base_reload_regression.cpp`:504
- Brief: Unregistering one module does not affect the others.
- Parameters:
  - `<unnamed>` (MultiModuleTest): n/a
  - `<unnamed>` (UnregisterOneDoesNotAffectOthers): n/a

#### `TEST_F(OperatorDiagnosticsTest, DependencyConflictFormatEmbedsBothModules)`
- Source: `tests/base/test_base_future_enhancements.cpp`:681
- Brief: format() for dependency conflict embeds the dependency name and both conflicting modules.
- Parameters:
  - `<unnamed>` (OperatorDiagnosticsTest): n/a
  - `<unnamed>` (DependencyConflictFormatEmbedsBothModules): n/a

#### `TEST_F(OperatorDiagnosticsTest, FormatOutputHasBasePrefix)`
- Source: `tests/base/test_base_future_enhancements.cpp`:588
- Brief: Every format() output must begin with "[BASE_" prefix.
- Parameters:
  - `<unnamed>` (OperatorDiagnosticsTest): n/a
  - `<unnamed>` (FormatOutputHasBasePrefix): n/a

#### `TEST_F(OperatorDiagnosticsTest, IsKnownCodeFalseForOutOfRange)`
- Source: `tests/base/test_base_future_enhancements.cpp`:603
- Brief: isKnownCode returns false for out-of-range codes.
- Parameters:
  - `<unnamed>` (OperatorDiagnosticsTest): n/a
  - `<unnamed>` (IsKnownCodeFalseForOutOfRange): n/a

#### `TEST_F(OperatorDiagnosticsTest, IsKnownCodeTrueForAllTaxonomyCodes)`
- Source: `tests/base/test_base_future_enhancements.cpp`:612
- Brief: isKnownCode returns true for all 24 taxonomy codes.
- Parameters:
  - `<unnamed>` (OperatorDiagnosticsTest): n/a
  - `<unnamed>` (IsKnownCodeTrueForAllTaxonomyCodes): n/a

#### `TEST_F(OperatorDiagnosticsTest, RegistryErrorFormatEmbedsBothArgs)`
- Source: `tests/base/test_base_future_enhancements.cpp`:672
- Brief: format() output for registry error contains the module name.
- Parameters:
  - `<unnamed>` (OperatorDiagnosticsTest): n/a
  - `<unnamed>` (RegistryErrorFormatEmbedsBothArgs): n/a

#### `TEST_F(OperatorDiagnosticsTest, ResolveDescriptionConsistentWithStaticDescription)`
- Source: `tests/base/test_base_future_enhancements.cpp`:653
- Brief: resolveDescription is consistent with struct description() for all known codes.
- Parameters:
  - `<unnamed>` (OperatorDiagnosticsTest): n/a
  - `<unnamed>` (ResolveDescriptionConsistentWithStaticDescription): n/a

#### `TEST_F(OperatorDiagnosticsTest, ResolveDescriptionEmptyForUnknownCode)`
- Source: `tests/base/test_base_future_enhancements.cpp`:646
- Brief: resolveDescription returns a stable fallback text for unknown codes.
- Parameters:
  - `<unnamed>` (OperatorDiagnosticsTest): n/a
  - `<unnamed>` (ResolveDescriptionEmptyForUnknownCode): n/a

#### `TEST_F(PluginDependencyGraphEdgeTest, BuildFromResolverPopulatesGraph)`
- Source: `tests/base/test_base_reload_regression.cpp`:659
- Brief: buildFromResolver populates nodes and edges correctly.
- Parameters:
  - `<unnamed>` (PluginDependencyGraphEdgeTest): n/a
  - `<unnamed>` (BuildFromResolverPopulatesGraph): n/a

#### `TEST_F(PluginDependencyGraphEdgeTest, ClearResetsGraph)`
- Source: `tests/base/test_base_reload_regression.cpp`:711
- Brief: clear() resets the graph to empty.
- Parameters:
  - `<unnamed>` (PluginDependencyGraphEdgeTest): n/a
  - `<unnamed>` (ClearResetsGraph): n/a

#### `TEST_F(PluginDependencyGraphEdgeTest, CycleDetectionFindsDirectCycle)`
- Source: `tests/base/test_base_reload_regression.cpp`:673
- Brief: Cycle detection in PluginDependencyGraph finds the cycle.
- Parameters:
  - `<unnamed>` (PluginDependencyGraphEdgeTest): n/a
  - `<unnamed>` (CycleDetectionFindsDirectCycle): n/a

#### `TEST_F(PluginDependencyGraphEdgeTest, DotExportProducesNonEmptyString)`
- Source: `tests/base/test_base_reload_regression.cpp`:744
- Brief: DOT export from a non-empty graph produces non-empty string.
- Parameters:
  - `<unnamed>` (PluginDependencyGraphEdgeTest): n/a
  - `<unnamed>` (DotExportProducesNonEmptyString): n/a

#### `TEST_F(PluginDependencyGraphEdgeTest, HundredNodeChainBuildFromResolver)`
- Source: `tests/base/test_base_reload_regression.cpp`:724
- Brief: 100-node linear chain: buildFromResolver produces correct node count.
- Parameters:
  - `<unnamed>` (PluginDependencyGraphEdgeTest): n/a
  - `<unnamed>` (HundredNodeChainBuildFromResolver): n/a

#### `TEST_F(PluginDependencyGraphEdgeTest, JsonExportContainsExpectedKeys)`
- Source: `tests/base/test_base_reload_regression.cpp`:757
- Brief: JSON export contains expected keys.
- Parameters:
  - `<unnamed>` (PluginDependencyGraphEdgeTest): n/a
  - `<unnamed>` (JsonExportContainsExpectedKeys): n/a

#### `TEST_F(PluginDependencyGraphEdgeTest, TopologicalOrderOnAcyclicGraph)`
- Source: `tests/base/test_base_reload_regression.cpp`:685
- Brief: Topological order on an acyclic graph is non-empty.
- Parameters:
  - `<unnamed>` (PluginDependencyGraphEdgeTest): n/a
  - `<unnamed>` (TopologicalOrderOnAcyclicGraph): n/a

#### `TEST_F(PluginGraphExtendedTest, AcyclicGraphHasNoCycles)`
- Source: `tests/base/test_base_future_enhancements.cpp`:549
- Brief: detectCycles() on an acyclic graph returns an empty vector.
- Parameters:
  - `<unnamed>` (PluginGraphExtendedTest): n/a
  - `<unnamed>` (AcyclicGraphHasNoCycles): n/a

#### `TEST_F(PluginGraphExtendedTest, AsciiExportNonEmpty)`
- Source: `tests/base/test_base_future_enhancements.cpp`:538
- Brief: ASCII export of a non-empty graph produces non-empty output.
- Parameters:
  - `<unnamed>` (PluginGraphExtendedTest): n/a
  - `<unnamed>` (AsciiExportNonEmpty): n/a

#### `TEST_F(PluginGraphExtendedTest, DuplicateAddModuleNoExtraNode)`
- Source: `tests/base/test_base_future_enhancements.cpp`:530
- Brief: Adding the same module twice does not create a duplicate node.
- Parameters:
  - `<unnamed>` (PluginGraphExtendedTest): n/a
  - `<unnamed>` (DuplicateAddModuleNoExtraNode): n/a

#### `TEST_F(PluginGraphExtendedTest, FreshGraphIsEmpty)`
- Source: `tests/base/test_base_future_enhancements.cpp`:523
- Brief: nodeCount() and edgeCount() are both 0 on a fresh graph.
- Parameters:
  - `<unnamed>` (PluginGraphExtendedTest): n/a
  - `<unnamed>` (FreshGraphIsEmpty): n/a

#### `TEST_F(PluginGraphExtendedTest, TopologicalOrderOnCyclicGraphNoThrow)`
- Source: `tests/base/test_base_future_enhancements.cpp`:568
- Brief: topologicalOrder() on a cyclic graph does not crash.
- Parameters:
  - `<unnamed>` (PluginGraphExtendedTest): n/a
  - `<unnamed>` (TopologicalOrderOnCyclicGraphNoThrow): n/a

#### `TEST_F(PluginGraphExtendedTest, TopologicalOrderOnEmptyGraphSafe)`
- Source: `tests/base/test_base_future_enhancements.cpp`:560
- Brief: topologicalOrder() on an empty graph returns empty vector without crash.
- Parameters:
  - `<unnamed>` (PluginGraphExtendedTest): n/a
  - `<unnamed>` (TopologicalOrderOnEmptyGraphSafe): n/a

#### `TEST_F(RegistryConfigValidationTest, DefaultConfigHasSaneDefaults)`
- Source: `tests/base/test_base_reload_regression.cpp`:853
- Brief: Default-constructed RegistryConfig has sane defaults.
- Parameters:
  - `<unnamed>` (RegistryConfigValidationTest): n/a
  - `<unnamed>` (DefaultConfigHasSaneDefaults): n/a

#### `TEST_F(RegistryConfigValidationTest, RegistryConfigCopyEquivalent)`
- Source: `tests/base/test_base_reload_regression.cpp`:903
- Brief: RegistryConfig copy is value-equivalent.
- Parameters:
  - `<unnamed>` (RegistryConfigValidationTest): n/a
  - `<unnamed>` (RegistryConfigCopyEquivalent): n/a

#### `TEST_F(RegistryConfigValidationTest, RegistryConfigFieldAssignment)`
- Source: `tests/base/test_base_reload_regression.cpp`:868
- Brief: RegistryConfig fields can be assigned without throwing.
- Parameters:
  - `<unnamed>` (RegistryConfigValidationTest): n/a
  - `<unnamed>` (RegistryConfigFieldAssignment): n/a

#### `TEST_F(RegistryConfigValidationTest, ZeroMaxRetriesIsAssignable)`
- Source: `tests/base/test_base_reload_regression.cpp`:896
- Brief: RegistryConfig: zero max_retries is representable (edge value — no retry).
- Parameters:
  - `<unnamed>` (RegistryConfigValidationTest): n/a
  - `<unnamed>` (ZeroMaxRetriesIsAssignable): n/a

#### `TEST_F(RegistryConfigValidationTest, ZeroTimeoutIsAssignable)`
- Source: `tests/base/test_base_reload_regression.cpp`:889
- Brief: RegistryConfig: zero timeout is representable (edge value).
- Parameters:
  - `<unnamed>` (RegistryConfigValidationTest): n/a
  - `<unnamed>` (ZeroTimeoutIsAssignable): n/a

#### `TEST_F(ReloadPhaseOrderTest, CallbackReregisterSameLoaderAllowsFollowupReloadAttempt)`
- Source: `tests/base/test_base_reload_regression.cpp`:264
- Brief: n/a
- Parameters:
  - `<unnamed>` (ReloadPhaseOrderTest): n/a
  - `<unnamed>` (CallbackReregisterSameLoaderAllowsFollowupReloadAttempt): n/a
- Details: Re-register race with the same loader must not poison manager state: after the re-registering callback runs once, a follow-up reload attempt still executes the normal registered-module failure path.

#### `TEST_F(ReloadPhaseOrderTest, CallbackReregisterSameLoaderDuringReloadKeepsRegistration)`
- Source: `tests/base/test_base_reload_regression.cpp`:239
- Brief: n/a
- Parameters:
  - `<unnamed>` (ReloadPhaseOrderTest): n/a
  - `<unnamed>` (CallbackReregisterSameLoaderDuringReloadKeepsRegistration): n/a
- Details: If a callback unregisters and re-registers the same module with the same loader object during BEFORE_UNLOAD, reload must still fail cleanly and leave exactly one fresh registration behind.

#### `TEST_F(ReloadPhaseOrderTest, CallbackUnregisterDuringReloadFailsCleanly)`
- Source: `tests/base/test_base_reload_regression.cpp`:219
- Brief: n/a
- Parameters:
  - `<unnamed>` (ReloadPhaseOrderTest): n/a
  - `<unnamed>` (CallbackUnregisterDuringReloadFailsCleanly): n/a
- Details: If a callback unregisters the module during BEFORE_UNLOAD, reload must fail cleanly without dereferencing stale slot storage.

#### `TEST_F(ReloadPhaseOrderTest, ClearCallbacksPreventsDispatch)`
- Source: `tests/base/test_base_reload_regression.cpp`:201
- Brief: Callbacks can be cleared; after clearReloadCallbacks no callbacks fire.
- Parameters:
  - `<unnamed>` (ReloadPhaseOrderTest): n/a
  - `<unnamed>` (ClearCallbacksPreventsDispatch): n/a

#### `TEST_F(ReloadPhaseOrderTest, FailedReloadEmitsOnlyBeforeUnload)`
- Source: `tests/base/test_base_reload_regression.cpp`:146
- Brief: On a failed reload (non-existent binary) only BEFORE_UNLOAD is emitted.
- Parameters:
  - `<unnamed>` (ReloadPhaseOrderTest): n/a
  - `<unnamed>` (FailedReloadEmitsOnlyBeforeUnload): n/a

#### `TEST_F(ReloadPhaseOrderTest, MultipleCallbacksAllInvoked)`
- Source: `tests/base/test_base_reload_regression.cpp`:170
- Brief: Multiple callbacks are all invoked for a reload attempt.
- Parameters:
  - `<unnamed>` (ReloadPhaseOrderTest): n/a
  - `<unnamed>` (MultipleCallbacksAllInvoked): n/a

#### `TEST_F(ReloadPhaseOrderTest, RollbackPhaseEmittedOnRollbackAttempt)`
- Source: `tests/base/test_base_reload_regression.cpp`:291
- Brief: n/a
- Parameters:
  - `<unnamed>` (ReloadPhaseOrderTest): n/a
  - `<unnamed>` (RollbackPhaseEmittedOnRollbackAttempt): n/a
- Details: Rollback phase callback: ROLLBACK must be emitted when rollback is called and the module has no backup (the rollback itself fails, but the phase must fire).

#### `TEST_F(ReloadRollbackTest, IsRollbackAvailableOnUnknownReturnsFalse)`
- Source: `tests/base/test_base_reload_regression.cpp`:105
- Brief: isRollbackAvailable returns false for a module that was never registered.
- Parameters:
  - `<unnamed>` (ReloadRollbackTest): n/a
  - `<unnamed>` (IsRollbackAvailableOnUnknownReturnsFalse): n/a

#### `TEST_F(ReloadRollbackTest, ReloadFailurePreservesRegistrationState)`
- Source: `tests/base/test_base_reload_regression.cpp`:89
- Brief: n/a
- Parameters:
  - `<unnamed>` (ReloadRollbackTest): n/a
  - `<unnamed>` (ReloadFailurePreservesRegistrationState): n/a
- Details: Reload failure (non-existent path) keeps the module registered and returns an error result; subsequent rollback is still unavailable (no prior success).

#### `TEST_F(ReloadRollbackTest, ReloadUnregisteredModuleFails)`
- Source: `tests/base/test_base_reload_regression.cpp`:71
- Brief: Reload on an unregistered module name must fail gracefully.
- Parameters:
  - `<unnamed>` (ReloadRollbackTest): n/a
  - `<unnamed>` (ReloadUnregisteredModuleFails): n/a

#### `TEST_F(ReloadRollbackTest, RollbackDisabledConfigNoBackupAvailable)`
- Source: `tests/base/test_base_reload_regression.cpp`:110
- Brief: Reload disabled (enableRollback=false): rollback call must still not crash.
- Parameters:
  - `<unnamed>` (ReloadRollbackTest): n/a
  - `<unnamed>` (RollbackDisabledConfigNoBackupAvailable): n/a

#### `TEST_F(ReloadRollbackTest, RollbackUnavailableWithNoBackup)`
- Source: `tests/base/test_base_reload_regression.cpp`:79
- Brief: Rollback is unavailable when no successful reload has been performed.
- Parameters:
  - `<unnamed>` (ReloadRollbackTest): n/a
  - `<unnamed>` (RollbackUnavailableWithNoBackup): n/a

#### `TEST_F(ReloadRollbackTest, RollbackUnregisteredModuleFails)`
- Source: `tests/base/test_base_reload_regression.cpp`:64
- Brief: Rollback on an unregistered module name must fail gracefully.
- Parameters:
  - `<unnamed>` (ReloadRollbackTest): n/a
  - `<unnamed>` (RollbackUnregisteredModuleFails): n/a

#### `TEST_F(ReloadStatsTest, FailedReloadIncrementsCounts)`
- Source: `tests/base/test_base_reload_regression.cpp`:334
- Brief: Each failed reload increments failedReloads and totalReloads.
- Parameters:
  - `<unnamed>` (ReloadStatsTest): n/a
  - `<unnamed>` (FailedReloadIncrementsCounts): n/a

#### `TEST_F(ReloadStatsTest, ResetStatsClearsAllCounters)`
- Source: `tests/base/test_base_reload_regression.cpp`:349
- Brief: resetStats() zeroes all counters.
- Parameters:
  - `<unnamed>` (ReloadStatsTest): n/a
  - `<unnamed>` (ResetStatsClearsAllCounters): n/a

#### `TEST_F(ReloadStatsTest, StatsAdditiveAcrossMultipleModules)`
- Source: `tests/base/test_base_reload_regression.cpp`:368
- Brief: Stats are additive across multiple registered modules.
- Parameters:
  - `<unnamed>` (ReloadStatsTest): n/a
  - `<unnamed>` (StatsAdditiveAcrossMultipleModules): n/a

#### `TEST_F(SandboxDegradedStateTest, LastErrorEmptyOnFreshSandbox)`
- Source: `tests/base/test_base_reload_regression.cpp`:791
- Brief: lastError() is available immediately (empty on a fresh sandbox).
- Parameters:
  - `<unnamed>` (SandboxDegradedStateTest): n/a
  - `<unnamed>` (LastErrorEmptyOnFreshSandbox): n/a

#### `TEST_F(SandboxDegradedStateTest, LaunchEmptyModuleNameDoesNotCrash)`
- Source: `tests/base/test_base_reload_regression.cpp`:803
- Brief: launch() with an empty module name does not crash and returns a bool result.
- Parameters:
  - `<unnamed>` (SandboxDegradedStateTest): n/a
  - `<unnamed>` (LaunchEmptyModuleNameDoesNotCrash): n/a

#### `TEST_F(SandboxDegradedStateTest, LaunchWarningsEmptyBeforeLaunch)`
- Source: `tests/base/test_base_reload_regression.cpp`:797
- Brief: launchWarnings() is empty before launch() is called.
- Parameters:
  - `<unnamed>` (SandboxDegradedStateTest): n/a
  - `<unnamed>` (LaunchWarningsEmptyBeforeLaunch): n/a

#### `TEST_F(SandboxDegradedStateTest, SandboxStatsNulloptForUnknownModule)`
- Source: `tests/base/test_base_reload_regression.cpp`:834
- Brief: getSandboxStats() returns nullopt for an unregistered module.
- Parameters:
  - `<unnamed>` (SandboxDegradedStateTest): n/a
  - `<unnamed>` (SandboxStatsNulloptForUnknownModule): n/a

#### `TEST_F(SandboxDegradedStateTest, SandboxStatsNulloptWhenNotConfigured)`
- Source: `tests/base/test_base_reload_regression.cpp`:816
- Brief: n/a
- Parameters:
  - `<unnamed>` (SandboxDegradedStateTest): n/a
  - `<unnamed>` (SandboxStatsNulloptWhenNotConfigured): n/a
- Details: HotReloadManager::getSandboxStats() returns nullopt when sandboxing is not configured (the default config).

#### `TEST_F(SandboxDegradedStateTest, StatsOnInactiveSandboxReturnsZeros)`
- Source: `tests/base/test_base_reload_regression.cpp`:776
- Brief: stats() on an inactive sandbox returns zero-value struct without crashing.
- Parameters:
  - `<unnamed>` (SandboxDegradedStateTest): n/a
  - `<unnamed>` (StatsOnInactiveSandboxReturnsZeros): n/a

#### `TEST_F(SandboxWasmIsolationTest, WasmIsolationConfigConstructable)`
- Source: `tests/base/test_base_future_enhancements.cpp`:389
- Brief: Config with enable_wasm_isolation=true can be constructed without crash.
- Parameters:
  - `<unnamed>` (SandboxWasmIsolationTest): n/a
  - `<unnamed>` (WasmIsolationConfigConstructable): n/a

#### `TEST_F(SandboxWasmIsolationTest, WasmIsolationInactiveByDefault)`
- Source: `tests/base/test_base_future_enhancements.cpp`:377
- Brief: isWasmIsolationActive() is false by default (no wasm config).
- Parameters:
  - `<unnamed>` (SandboxWasmIsolationTest): n/a
  - `<unnamed>` (WasmIsolationInactiveByDefault): n/a

#### `TEST_F(SandboxWasmIsolationTest, WasmSandboxNullptrWhenNotActive)`
- Source: `tests/base/test_base_future_enhancements.cpp`:383
- Brief: wasmSandbox() returns nullptr when wasm isolation is not active.
- Parameters:
  - `<unnamed>` (SandboxWasmIsolationTest): n/a
  - `<unnamed>` (WasmSandboxNullptrWhenNotActive): n/a

#### `TEST_F(StateSaveRestoreTest, FalseReturningStateRestoreCallbackIsIsolated)`
- Source: `tests/base/test_base_reload_regression.cpp`:425
- Brief: A state-restore callback that returns false does not crash the system.
- Parameters:
  - `<unnamed>` (StateSaveRestoreTest): n/a
  - `<unnamed>` (FalseReturningStateRestoreCallbackIsIsolated): n/a

#### `TEST_F(StateSaveRestoreTest, StateSaveCallbackInvokedDuringReload)`
- Source: `tests/base/test_base_reload_regression.cpp`:450
- Brief: n/a
- Parameters:
  - `<unnamed>` (StateSaveRestoreTest): n/a
  - `<unnamed>` (StateSaveCallbackInvokedDuringReload): n/a
- Details: State-save callback producing non-empty state increments statesSaved when a reload succeeds (fast-fail case: save fires even if reload then fails).

#### `TEST_F(StateSaveRestoreTest, ThrowingStateSaveCallbackIsIsolated)`
- Source: `tests/base/test_base_reload_regression.cpp`:408
- Brief: n/a
- Parameters:
  - `<unnamed>` (StateSaveRestoreTest): n/a
  - `<unnamed>` (ThrowingStateSaveCallbackIsIsolated): n/a
- Details: A state-save callback that throws must not propagate the exception out of reloadModule(); the reload should still return a usable result.

#### `TEST_F(TracingIntegrationTest, RollbackNoBackupEmitsErrorSpan)`
- Source: `tests/base/test_base_wave_d_tracing.cpp`:115
- Brief: rollback() on a module with no backup emits a start and error-end span.
- Parameters:
  - `<unnamed>` (TracingIntegrationTest): n/a
  - `<unnamed>` (RollbackNoBackupEmitsErrorSpan): n/a

#### `TEST_F(TracingIntegrationTest, UnregisteredModuleEmitsErrorSpan)`
- Source: `tests/base/test_base_wave_d_tracing.cpp`:102
- Brief: reloadModule() on an unregistered module emits a start and error-end span.
- Parameters:
  - `<unnamed>` (TracingIntegrationTest): n/a
  - `<unnamed>` (UnregisteredModuleEmitsErrorSpan): n/a

#### `TEST_F(WasmFuelBudgetTest, NonZeroBudgetDoesNotInterfereWithNoRuntimeError)`
- Source: `tests/base/test_base_future_enhancements.cpp`:316
- Brief: n/a
- Parameters:
  - `<unnamed>` (WasmFuelBudgetTest): n/a
  - `<unnamed>` (NonZeroBudgetDoesNotInterfereWithNoRuntimeError): n/a
- Details: A non-zero budget is set; callExport without runtime still fails for missing runtime, not for budget reasons (budget gate only applies when runtime is present).

#### `TEST_F(WasmFuelBudgetTest, ZeroInstructionBudgetIsUnlimited)`
- Source: `tests/base/test_base_future_enhancements.cpp`:299
- Brief: n/a
- Parameters:
  - `<unnamed>` (WasmFuelBudgetTest): n/a
  - `<unnamed>` (ZeroInstructionBudgetIsUnlimited): n/a
- Details: Zero max_instructions is treated as unlimited — callExport may still fail for no-runtime reasons but must not fail with a fuel-exhaustion message.

#### `TEST_F(WasmHostFunctionTest, AddHostFunctionIncrementsCount)`
- Source: `tests/base/test_base_future_enhancements.cpp`:243
- Brief: Adding host functions increments the count.
- Parameters:
  - `<unnamed>` (WasmHostFunctionTest): n/a
  - `<unnamed>` (AddHostFunctionIncrementsCount): n/a

#### `TEST_F(WasmHostFunctionTest, ClearHostFunctionsResetsCount)`
- Source: `tests/base/test_base_future_enhancements.cpp`:268
- Brief: clearHostFunctions() resets count to zero.
- Parameters:
  - `<unnamed>` (WasmHostFunctionTest): n/a
  - `<unnamed>` (ClearHostFunctionsResetsCount): n/a

#### `TEST_F(WasmHostFunctionTest, InitialHostFunctionCountIsZero)`
- Source: `tests/base/test_base_future_enhancements.cpp`:237
- Brief: hostFunctionCount() starts at zero.
- Parameters:
  - `<unnamed>` (WasmHostFunctionTest): n/a
  - `<unnamed>` (InitialHostFunctionCountIsZero): n/a

#### `TEST_F(WasmModuleInfoTest, DefaultIsInvalid)`
- Source: `tests/base/test_base_future_enhancements.cpp`:338
- Brief: Default-constructed WasmModuleInfo is invalid.
- Parameters:
  - `<unnamed>` (WasmModuleInfoTest): n/a
  - `<unnamed>` (DefaultIsInvalid): n/a

#### `TEST_F(WasmModuleInfoTest, ModuleInfoValidAfterLoad)`
- Source: `tests/base/test_base_future_enhancements.cpp`:358
- Brief: After loading a valid WASM binary, moduleInfo() reflects valid=true.
- Parameters:
  - `<unnamed>` (WasmModuleInfoTest): n/a
  - `<unnamed>` (ModuleInfoValidAfterLoad): n/a

#### `TEST_F(WasmModuleInfoTest, SummaryCallableOnInvalidInfo)`
- Source: `tests/base/test_base_future_enhancements.cpp`:349
- Brief: summary() is callable on a default-constructed (invalid) info without crashing.
- Parameters:
  - `<unnamed>` (WasmModuleInfoTest): n/a
  - `<unnamed>` (SummaryCallableOnInvalidInfo): n/a

#### `TEST_F(WasmSandboxValidationTest, CallExportWithoutRuntimeReturnsError)`
- Source: `tests/base/test_base_future_enhancements.cpp`:201
- Brief: callExport without a runtime returns an error result (not a crash).
- Parameters:
  - `<unnamed>` (WasmSandboxValidationTest): n/a
  - `<unnamed>` (CallExportWithoutRuntimeReturnsError): n/a

#### `TEST_F(WasmSandboxValidationTest, EmptyBytesFailGracefully)`
- Source: `tests/base/test_base_future_enhancements.cpp`:182
- Brief: Loading empty bytes fails gracefully.
- Parameters:
  - `<unnamed>` (WasmSandboxValidationTest): n/a
  - `<unnamed>` (EmptyBytesFailGracefully): n/a

#### `TEST_F(WasmSandboxValidationTest, FreshSandboxDefaults)`
- Source: `tests/base/test_base_future_enhancements.cpp`:163
- Brief: Fresh sandbox has no runtime, no module loaded.
- Parameters:
  - `<unnamed>` (WasmSandboxValidationTest): n/a
  - `<unnamed>` (FreshSandboxDefaults): n/a

#### `TEST_F(WasmSandboxValidationTest, InvalidBytesFailGracefully)`
- Source: `tests/base/test_base_future_enhancements.cpp`:191
- Brief: Loading invalid (non-WASM) bytes fails gracefully.
- Parameters:
  - `<unnamed>` (WasmSandboxValidationTest): n/a
  - `<unnamed>` (InvalidBytesFailGracefully): n/a

#### `TEST_F(WasmSandboxValidationTest, RepeatedLoadUnloadCyclesSafe)`
- Source: `tests/base/test_base_future_enhancements.cpp`:221
- Brief: Repeated load/unload cycles do not crash.
- Parameters:
  - `<unnamed>` (WasmSandboxValidationTest): n/a
  - `<unnamed>` (RepeatedLoadUnloadCyclesSafe): n/a

#### `TEST_F(WasmSandboxValidationTest, UnloadResetsLoadedState)`
- Source: `tests/base/test_base_future_enhancements.cpp`:211
- Brief: unload() after a successful load resets isLoaded to false.
- Parameters:
  - `<unnamed>` (WasmSandboxValidationTest): n/a
  - `<unnamed>` (UnloadResetsLoadedState): n/a

#### `TEST_F(WasmSandboxValidationTest, ValidWasmBytesLoadSucceeds)`
- Source: `tests/base/test_base_future_enhancements.cpp`:173
- Brief: Loading minimal valid WASM bytes succeeds in validation-only mode.
- Parameters:
  - `<unnamed>` (WasmSandboxValidationTest): n/a
  - `<unnamed>` (ValidWasmBytesLoadSucceeds): n/a

#### `size_t readUleb128(const uint8_t *data, const uint8_t *end, uint64_t &out) noexcept`
- Source: `src/base/wasm_plugin_sandbox.cpp`:56
- Brief: n/a
- Parameters:
  - `data` (const uint8_t *): n/a
  - `end` (const uint8_t *): n/a
  - `out` (uint64_t &): n/a

#### `size_t readWasmName(const uint8_t *data, const uint8_t *end, std::string &out) noexcept`
- Source: `src/base/wasm_plugin_sandbox.cpp`:75
- Brief: n/a
- Parameters:
  - `data` (const uint8_t *): n/a
  - `end` (const uint8_t *): n/a
  - `out` (std::string &): n/a

### themis::modules::FailClosedTest

#### `void SetUp() override`
- Source: `tests/base/test_base_wave_d_tracing.cpp`:261
- Brief: n/a
- Parameters: none

### themis::modules::ModuleSecurityVerifier::Impl

#### `Impl()`
- Source: `src/base/module_loader.cpp`:76
- Brief: n/a
- Parameters: none

#### `void addBlacklistedHash(const std::string &hash)`
- Source: `src/base/module_loader.cpp`:164
- Brief: Add Blacklisted Hash.
- Parameters:
  - `hash` (const std::string &): Input parameter.
- Details: hash Input parameter. Calls: push_back(), updatePolicy().

#### `void addWhitelistedHash(const std::string &hash)`
- Source: `src/base/module_loader.cpp`:154
- Brief: Add Whitelisted Hash.
- Parameters:
  - `hash` (const std::string &): Input parameter.
- Details: hash Input parameter. Calls: push_back(), updatePolicy().

#### `std::string calculateFileHash(const std::string &modulePath)`
- Source: `src/base/module_loader.cpp`:125
- Brief: Calculate File Hash.
- Parameters:
  - `modulePath` (const std::string &): Input parameter.
- Return: Return value.
- Details: modulePath Input parameter. Return value. Implements calculateFileHash without additional internal calls.

#### `void setAllowUnsigned(bool allow)`
- Source: `src/base/module_loader.cpp`:144
- Brief: Set Allow Unsigned.
- Parameters:
  - `allow` (bool): Input parameter.
- Details: allow Input parameter. Calls: updatePolicy().

#### `void setRequireSignature(bool require)`
- Source: `src/base/module_loader.cpp`:134
- Brief: Set Require Signature.
- Parameters:
  - `require` (bool): Input parameter.
- Details: require Input parameter. Calls: updatePolicy().

#### `bool verifyModule(const std::string &modulePath, std::string &errorMessage)`
- Source: `src/base/module_loader.cpp`:104
- Brief: Verify Module.
- Parameters:
  - `modulePath` (const std::string &): Input parameter.
  - `errorMessage` (std::string &): Input/output parameter.
- Return: True when the operation succeeds.
- Details: modulePath Input parameter. errorMessage Input/output parameter. True when the operation succeeds. Calls: spdlog::debug(), verifyPlugin(), spdlog::info(), spdlog::error().

### themis::modules::MultiModuleTest

#### `void SetUp() override`
- Source: `tests/base/test_base_reload_regression.cpp`:472
- Brief: n/a
- Parameters: none

### themis::modules::ReloadPhaseOrderTest

#### `void SetUp() override`
- Source: `tests/base/test_base_reload_regression.cpp`:131
- Brief: n/a
- Parameters: none

### themis::modules::ReloadRollbackTest

#### `void SetUp() override`
- Source: `tests/base/test_base_reload_regression.cpp`:47
- Brief: n/a
- Parameters: none

### themis::modules::ReloadStatsTest

#### `void SetUp() override`
- Source: `tests/base/test_base_reload_regression.cpp`:319
- Brief: n/a
- Parameters: none

### themis::modules::SpanCollector

#### `void clear()`
- Source: `tests/base/test_base_wave_d_tracing.cpp`:75
- Brief: n/a
- Parameters: none

#### `std::size_t countEndEvents() const`
- Source: `tests/base/test_base_wave_d_tracing.cpp`:59
- Brief: n/a
- Parameters: none

#### `std::size_t countErrorEndEvents() const`
- Source: `tests/base/test_base_wave_d_tracing.cpp`:66
- Brief: n/a
- Parameters: none

#### `std::size_t countStartEvents() const`
- Source: `tests/base/test_base_wave_d_tracing.cpp`:52
- Brief: n/a
- Parameters: none

#### `SpanEmitter emitter()`
- Source: `tests/base/test_base_wave_d_tracing.cpp`:45
- Brief: n/a
- Parameters: none

#### `SpanEvent lastEvent() const`
- Source: `tests/base/test_base_wave_d_tracing.cpp`:81
- Brief: Return a copy of the most recent event under the lock.
- Parameters: none

### themis::modules::StateSaveRestoreTest

#### `void SetUp() override`
- Source: `tests/base/test_base_reload_regression.cpp`:392
- Brief: n/a
- Parameters: none

### themis::modules::TracingIntegrationTest

#### `void SetUp() override`
- Source: `tests/base/test_base_wave_d_tracing.cpp`:96
- Brief: n/a
- Parameters: none

### themis::modules::WasmFuelBudgetTest

#### `std::vector< uint8_t > minimalWasm()`
- Source: `tests/base/test_base_future_enhancements.cpp`:292
- Brief: n/a
- Parameters: none

### themis::modules::WasmSandboxValidationTest

#### `std::vector< uint8_t > minimalWasm()`
- Source: `tests/base/test_base_future_enhancements.cpp`:155
- Brief: Minimal valid WASM binary (magic + version, no sections).
- Parameters: none

### themis::resource::AdaptiveConnectionPool

#### `AdaptiveConnectionPool()`
- Source: `include/base/resource_pool_manager.h`:66
- Brief: n/a
- Parameters: none

#### `AdaptiveConnectionPool(const AdaptiveConnectionPool &)=delete`
- Source: `include/base/resource_pool_manager.h`:78
- Brief: n/a
- Parameters:
  - `<unnamed>` (const AdaptiveConnectionPool &): n/a

#### `AdaptiveConnectionPool(const Config &cfg)`
- Source: `include/base/resource_pool_manager.h`:73
- Brief: Adaptive Connection Pool.
- Parameters:
  - `cfg` (const Config &): Input parameter.
- Return: Return value.
- Details: cfg Input parameter. Return value.

#### `bool acquire(std::chrono::milliseconds timeout, int &slot_id)`
- Source: `include/base/resource_pool_manager.h`:87
- Brief: Acquire.
- Parameters:
  - `timeout` (std::chrono::milliseconds): Input parameter.
  - `slot_id` (int &): Identifier of the slot.
- Return: True when the operation succeeds.
- Throws:
  - std::runtime_error: if an error occurs.
- Details: timeout Input parameter. slot_id Identifier of the slot. True when the operation succeeds. timeout Input parameter. slot_id Identifier of the slot. True when the operation succeeds. std::runtime_error if an error occurs. Calls: load(), std::chrono::steady_clock::now(), lk(), wait_until(), empty(), count(), back(), pop_back().

#### `std::size_t available() const noexcept`
- Source: `include/base/resource_pool_manager.h`:96
- Brief: n/a
- Parameters: none

#### `void forceScaleDown()`
- Source: `include/base/resource_pool_manager.h`:119
- Brief: Force Scale Down.
- Parameters: none
- Details: Calls: lk(), shrinkLocked().

#### `void forceScaleUp()`
- Source: `include/base/resource_pool_manager.h`:114
- Brief: Force Scale Up.
- Parameters: none
- Details: Calls: lk(), growLocked().

#### `void growLocked(std::size_t count)`
- Source: `include/base/resource_pool_manager.h`:126
- Brief: Grow Locked.
- Parameters:
  - `count` (std::size_t): Input parameter.
- Details: ------------------------------------------------------------------------ grow / shrink (caller must hold mutex_) ------------------------------------------------------------------------ count Input parameter. Grow pool (caller holds lock). count Input parameter. Calls: std::min(), push_back().

#### `std::size_t in_use() const noexcept`
- Source: `include/base/resource_pool_manager.h`:97
- Brief: n/a
- Parameters: none

#### `bool is_shutdown() const noexcept`
- Source: `include/base/resource_pool_manager.h`:101
- Brief: n/a
- Parameters: none

#### `AdaptiveConnectionPool & operator=(const AdaptiveConnectionPool &)=delete`
- Source: `include/base/resource_pool_manager.h`:79
- Brief: n/a
- Parameters:
  - `<unnamed>` (const AdaptiveConnectionPool &): n/a

#### `void release(int slot_id)`
- Source: `include/base/resource_pool_manager.h`:93
- Brief: Release.
- Parameters:
  - `slot_id` (int): Identifier of the slot.
- Details: slot_id Identifier of the slot. slot_id Identifier of the slot. Calls: lk(), push_back(), size(), shrinkLocked(), notify_one().

#### `void shrinkLocked(std::size_t count)`
- Source: `include/base/resource_pool_manager.h`:131
- Brief: Shrink Locked.
- Parameters:
  - `count` (std::size_t): Input parameter.
- Details: count Input parameter. Shrink pool (caller holds lock). count Input parameter. Calls: std::max(), std::min(), size(), pop_back().

#### `void shutdown() noexcept`
- Source: `include/base/resource_pool_manager.h`:109
- Brief: Shutdown.
- Parameters: none
- Details: Exception safety: noexcept.

#### `std::size_t size() const noexcept`
- Source: `include/base/resource_pool_manager.h`:95
- Brief: n/a
- Parameters: none

#### `Statistics statistics() const noexcept`
- Source: `include/base/resource_pool_manager.h`:99
- Brief: n/a
- Parameters: none

#### `~AdaptiveConnectionPool()`
- Source: `include/base/resource_pool_manager.h`:75
- Brief: n/a
- Parameters: none

### themis::resource::BufferHandle

#### `BufferHandle()=default`
- Source: `include/base/buffer_pool.h`:48
- Brief: n/a
- Parameters: none

#### `BufferHandle(BufferHandle &&o) noexcept`
- Source: `include/base/buffer_pool.h`:58
- Brief: n/a
- Parameters:
  - `o` (BufferHandle &&): n/a

#### `BufferHandle(const BufferHandle &)=delete`
- Source: `include/base/buffer_pool.h`:55
- Brief: n/a
- Parameters:
  - `<unnamed>` (const BufferHandle &): n/a

#### `BufferHandle(void *data, std::size_t size, SlabClass slab, class BufferPool *pool, bool auto_release=true) noexcept`
- Source: `include/base/buffer_pool.h`:50
- Brief: n/a
- Parameters:
  - `data` (void *): n/a
  - `size` (std::size_t): n/a
  - `slab` (SlabClass): n/a
  - `pool` (class BufferPool *): n/a
  - `auto_release` (bool): n/a

#### `uint8_t * bytes() noexcept`
- Source: `include/base/buffer_pool.h`:86
- Brief: n/a
- Parameters: none

#### `const void * data() const noexcept`
- Source: `include/base/buffer_pool.h`:82
- Brief: n/a
- Parameters: none

#### `void * data() noexcept`
- Source: `include/base/buffer_pool.h`:81
- Brief: n/a
- Parameters: none

#### `BufferHandle & operator=(BufferHandle &&o) noexcept`
- Source: `include/base/buffer_pool.h`:65
- Brief: n/a
- Parameters:
  - `o` (BufferHandle &&): n/a

#### `BufferHandle & operator=(const BufferHandle &)=delete`
- Source: `include/base/buffer_pool.h`:56
- Brief: n/a
- Parameters:
  - `<unnamed>` (const BufferHandle &): n/a

#### `void release() noexcept`
- Source: `include/base/buffer_pool.h`:94
- Brief: Release.
- Parameters: none
- Details: Exception safety: noexcept.

#### `std::size_t size() const noexcept`
- Source: `include/base/buffer_pool.h`:83
- Brief: n/a
- Parameters: none

#### `bool valid() const noexcept`
- Source: `include/base/buffer_pool.h`:84
- Brief: n/a
- Parameters: none

#### `~BufferHandle()`
- Source: `include/base/buffer_pool.h`:79
- Brief: n/a
- Parameters: none

### themis::resource::BufferPool

#### `BufferPool()`
- Source: `include/base/buffer_pool.h`:122
- Brief: n/a
- Parameters: none

#### `BufferPool(const BufferPool &)=delete`
- Source: `include/base/buffer_pool.h`:131
- Brief: n/a
- Parameters:
  - `<unnamed>` (const BufferPool &): n/a

#### `BufferPool(const Config &config)`
- Source: `include/base/buffer_pool.h`:129
- Brief: Buffer Pool.
- Parameters:
  - `config` (const Config &): Input parameter.
- Return: Return value.
- Details: config Input parameter. Return value.

#### `BufferHandle acquire(std::size_t bytes) noexcept`
- Source: `include/base/buffer_pool.h`:136
- Brief: n/a
- Parameters:
  - `bytes` (std::size_t): n/a

#### `SlabClass indexToClass(std::size_t idx) noexcept`
- Source: `include/base/buffer_pool.h`:181
- Brief: n/a
- Parameters:
  - `idx` (std::size_t): n/a

#### `bool is_shutdown() const noexcept`
- Source: `include/base/buffer_pool.h`:154
- Brief: n/a
- Parameters: none

#### `BufferPool & operator=(const BufferPool &)=delete`
- Source: `include/base/buffer_pool.h`:132
- Brief: n/a
- Parameters:
  - `<unnamed>` (const BufferPool &): n/a

#### `void preallocateSlab(Slab &s, std::size_t count)`
- Source: `include/base/buffer_pool.h`:188
- Brief: Preallocate Slab.
- Parameters:
  - `s` (Slab &): Input/output parameter.
  - `count` (std::size_t): Input parameter.
- Details: s Input/output parameter. count Input parameter. s Input/output parameter. count Input parameter. Calls: reserve(), size(), std::malloc(), push_back().

#### `void release(void *data, SlabClass slab) noexcept`
- Source: `include/base/buffer_pool.h`:144
- Brief: Release.
- Parameters:
  - `data` (void *): Input/output parameter.
  - `slab` (SlabClass): Input parameter.
- Details: data Input/output parameter. slab Input parameter. Exception safety: noexcept.

#### `void shutdown() noexcept`
- Source: `include/base/buffer_pool.h`:152
- Brief: Shutdown.
- Parameters: none
- Details: Exception safety: noexcept.

#### `std::size_t slabIndex(std::size_t bytes) noexcept`
- Source: `include/base/buffer_pool.h`:180
- Brief: n/a
- Parameters:
  - `bytes` (std::size_t): n/a

#### `Statistics statistics() const noexcept`
- Source: `include/base/buffer_pool.h`:146
- Brief: n/a
- Parameters: none

#### `~BufferPool()`
- Source: `include/base/buffer_pool.h`:134
- Brief: n/a
- Parameters: none

### themis::resource::ResourcePoolManager

#### `ResourcePoolManager()`
- Source: `include/base/resource_pool_manager.h`:178
- Brief: n/a
- Parameters: none

#### `ResourcePoolManager(const Config &cfg)`
- Source: `include/base/resource_pool_manager.h`:185
- Brief: Resource Pool Manager.
- Parameters:
  - `cfg` (const Config &): Input parameter.
- Return: Return value.
- Details: cfg Input parameter. Return value.

#### `ResourcePoolManager(const ResourcePoolManager &)=delete`
- Source: `include/base/resource_pool_manager.h`:190
- Brief: n/a
- Parameters:
  - `<unnamed>` (const ResourcePoolManager &): n/a

#### `BufferPool & bufferPool() noexcept`
- Source: `include/base/resource_pool_manager.h`:197
- Brief: n/a
- Parameters: none

#### `AdaptiveConnectionPool & connectionPool() noexcept`
- Source: `include/base/resource_pool_manager.h`:193
- Brief: n/a
- Parameters: none

#### `bool is_shutdown() const noexcept`
- Source: `include/base/resource_pool_manager.h`:209
- Brief: n/a
- Parameters: none

#### `ResourcePoolManager & operator=(const ResourcePoolManager &)=delete`
- Source: `include/base/resource_pool_manager.h`:191
- Brief: n/a
- Parameters:
  - `<unnamed>` (const ResourcePoolManager &): n/a

#### `void shutdown() noexcept`
- Source: `include/base/resource_pool_manager.h`:207
- Brief: Shutdown.
- Parameters: none
- Details: Exception safety: noexcept.

#### `GlobalStatistics statistics() const noexcept`
- Source: `include/base/resource_pool_manager.h`:201
- Brief: n/a
- Parameters: none

#### `~ResourcePoolManager()`
- Source: `include/base/resource_pool_manager.h`:187
- Brief: n/a
- Parameters: none

### themis::storage

#### `TEST(BaseEntitySetFieldTest, FailClosedGuardsAreIndependent)`
- Source: `tests/base/test_base_entity_focused.cpp`:57
- Brief: n/a
- Parameters:
  - `<unnamed>` (BaseEntitySetFieldTest): n/a
  - `<unnamed>` (FailClosedGuardsAreIndependent): n/a

#### `TEST(BaseEntitySetFieldTest, FieldMapCorruptionPrevented)`
- Source: `tests/base/test_base_entity_focused.cpp`:81
- Brief: n/a
- Parameters:
  - `<unnamed>` (BaseEntitySetFieldTest): n/a
  - `<unnamed>` (FieldMapCorruptionPrevented): n/a

#### `TEST(BaseEntitySetFieldTest, MultipleFieldsCanBeSetCorrectly)`
- Source: `tests/base/test_base_entity_focused.cpp`:38
- Brief: n/a
- Parameters:
  - `<unnamed>` (BaseEntitySetFieldTest): n/a
  - `<unnamed>` (MultipleFieldsCanBeSetCorrectly): n/a

#### `TEST(BaseEntitySetFieldTest, SetFieldAcceptsValidFieldName)`
- Source: `tests/base/test_base_entity_focused.cpp`:25
- Brief: n/a
- Parameters:
  - `<unnamed>` (BaseEntitySetFieldTest): n/a
  - `<unnamed>` (SetFieldAcceptsValidFieldName): n/a

#### `TEST(BaseEntitySetFieldTest, SetFieldFailsClosedForEmptyFieldName)`
- Source: `tests/base/test_base_entity_focused.cpp`:8
- Brief: n/a
- Parameters:
  - `<unnamed>` (BaseEntitySetFieldTest): n/a
  - `<unnamed>` (SetFieldFailsClosedForEmptyFieldName): n/a

