# CORE DOXYGEN

Author: ThemisDB Contributors
Created: 2026-09-20
Last Updated: 2026-09-20
Status: active

## Source
- This file is generated from module-scoped Doxygen XML.
- XML index: `C:\Projects\ThemisDB\.tmp\module_doxygen_global_e2e\core\xml\index.xml`
- Warnings log: `C:\Projects\ThemisDB\.tmp\module_doxygen_global_e2e\core\doxygen-warnings.log`

## Structure Metrics
- C/C++ Files (scanned): 63
- Compounds: 207
- Classes/Structs: 118
- Namespaces: 15
- File Compounds: 63

## Namespaces
- benchmark
- core
- std
- themis
- themis::acceleration
- themis::core
- themis::core::concerns
- themis::core::concerns::@067143327213233025015064254304024273053357201245
- themis::core::concerns::@222206137266143114003270333157343334133163000272
- themis::core::concerns::@246275063113153321022112211141147334247275001032
- themis::core::concerns::@344174006172317251325101070012071121024356251043
- themis::core::concerns::context_keys
- themis::core::concerns::labels
- themis::test
- themis::test::wave_d

## Types
### Classes
- GraphIndexBench
- QueryEngineBench
- RejectAllValidator
- SecondaryIndexBench
- TimeseriesBench
- VectorIndexBench
- themis::IndexManagerBuilder
- themis::QueryEngineBuilder
- themis::SecurityLayerBuilder
- themis::StorageEngineBuilder
- themis::core::ConfigValidator
- themis::core::IConfigHotReloader
- themis::core::IHealthProbe
- themis::core::IHealthProbeRegistry
- themis::core::ProductionMode
- themis::core::concerns::ARCEvictionStrategy
- themis::core::concerns::AdapterRegistry
- themis::core::concerns::AdapterValidator
- themis::core::concerns::ConcernsContext
- themis::core::concerns::ContextPropagation
- themis::core::concerns::ContextScope
- themis::core::concerns::DefaultCircuitBreaker
- themis::core::concerns::EnvSecretsProvider
- themis::core::concerns::IAsyncCache
- themis::core::concerns::IAsyncLogger
- themis::core::concerns::IAuditLog
- themis::core::concerns::ICache
- themis::core::concerns::ICircuitBreaker
- themis::core::concerns::IContext
- themis::core::concerns::IEvictionStrategy
- themis::core::concerns::IFeatureFlags
- themis::core::concerns::ILogger
- themis::core::concerns::IMetrics
- themis::core::concerns::ISecrets
- themis::core::concerns::ITracer
- themis::core::concerns::ITracer::ISpan
- themis::core::concerns::InMemoryAuditLog
- themis::core::concerns::InMemoryCacheImpl
- themis::core::concerns::InMemoryFeatureFlags
- themis::core::concerns::InMemorySecrets
- themis::core::concerns::JaegerTracerAdapter
- themis::core::concerns::JaegerTracerAdapter::JaegerSpanAdapter
- themis::core::concerns::LFUEvictionStrategy
- themis::core::concerns::LRUEvictionStrategy
- themis::core::concerns::LatencyTimer
- themis::core::concerns::LockFreeMetrics
- themis::core::concerns::MetricLabels
- themis::core::concerns::NoOpAsyncCache
- themis::core::concerns::NoOpAsyncLogger
- themis::core::concerns::NoOpAuditLog
- themis::core::concerns::NoOpCache
- themis::core::concerns::NoOpCircuitBreaker
- themis::core::concerns::NoOpFeatureFlags
- themis::core::concerns::NoOpLogger
- themis::core::concerns::NoOpMetrics
- themis::core::concerns::NoOpSecrets
- themis::core::concerns::NoOpTracer
- themis::core::concerns::NoOpTracer::NoOpSpan
- themis::core::concerns::OpenTelemetryTracerAdapter
- themis::core::concerns::OpenTelemetryTracerAdapter::OtelSpanAdapter
- themis::core::concerns::PrometheusMetricsAdapter
- themis::core::concerns::RedisCache
- themis::core::concerns::ScopedSpan
- themis::core::concerns::SignedAdapterValidator
- themis::core::concerns::SimpleContext
- themis::core::concerns::SpdlogLoggerAdapter
- themis::core::concerns::StrategicCacheImpl
- themis::core::concerns::TTLEvictionStrategy
- themis::core::concerns::TwoTierEvictionStrategy
- themis::core::concerns::W3CTraceContextPropagator
- themis::core::concerns::ZeroCopyLogger
- themis::core::concerns::ZipkinTracerAdapter
- themis::core::concerns::ZipkinTracerAdapter::ZipkinSpanAdapter

### Structs
- CustomAdapterImpl
- FakeAdapterImpl
- FakeAlphaImpl
- FakeBetaImpl
- ICustomAdapter
- IFakeAdapter
- IFakeAlpha
- IFakeBeta
- TempFile
- themis::SecurityLayerBuilder::SecurityLayer
- themis::core::AggregateHealthReport
- themis::core::ConfigChange
- themis::core::ConfigValidator::ValidationResult
- themis::core::HealthCheckResult
- themis::core::HotReloadResult
- themis::core::concerns::AdapterMetadata
- themis::core::concerns::AdapterRegistry::PluginHandle
- themis::core::concerns::AdapterSignature
- themis::core::concerns::AuditEvent
- themis::core::concerns::CacheEntry
- themis::core::concerns::CacheMetrics
- themis::core::concerns::ConcernsContext::Config
- themis::core::concerns::HealthStatus
- themis::core::concerns::ICircuitBreaker::Config
- themis::core::concerns::InMemoryCacheImpl::CachedValue
- themis::core::concerns::JaegerTracerAdapter::CircuitBreakerConfig
- themis::core::concerns::JaegerTracerAdapter::UberTraceIds
- themis::core::concerns::LFUEvictionStrategy::FrequencyData
- themis::core::concerns::LockFreeMetrics::CounterEntry
- themis::core::concerns::LockFreeMetrics::GaugeEntry
- themis::core::concerns::LockFreeMetrics::HistoAggregate
- themis::core::concerns::LockFreeMetrics::HistoObservation
- themis::core::concerns::LockFreeMetrics::SPSCRing
- themis::core::concerns::LockFreeMetrics::ThreadEntry
- themis::core::concerns::OpenTelemetryTracerAdapter::CircuitBreakerConfig
- themis::core::concerns::ProbeResult
- themis::core::concerns::RedisCache::NodeConn
- themis::core::concerns::RedisCacheConfig
- themis::core::concerns::StrategicCacheImpl::CachedValue
- themis::core::concerns::TraceContext
- themis::core::concerns::ZipkinTracerAdapter::B3Ids
- themis::core::concerns::ZipkinTracerAdapter::CircuitBreakerConfig
- themis::test::wave_d::StubConfigStore
- themis::test::wave_d::StubModuleLifecycle
- themis::test::wave_d::StubSpanExporter

## Notes
- This is a derived artifact. Do not manually edit semantic content.
- Regenerate via scripts/generate_module_doxygen_xml.py --write-module-markdown.

## Detailed Function Documentation (Soll-Ist)
- Functions extracted: 903

### CustomAdapterImpl

#### `CustomAdapterImpl(int v)`
- Source: `tests/core/test_concerns_context_focused.cpp`:35
- Brief: n/a
- Parameters:
  - `v` (int): n/a

#### `int id() const override`
- Source: `tests/core/test_concerns_context_focused.cpp`:36
- Brief: n/a
- Parameters: none

### FakeAdapterImpl

#### `FakeAdapterImpl(int v)`
- Source: `tests/core/test_adapter_signing_focused.cpp`:131
- Brief: n/a
- Parameters:
  - `v` (int): n/a

#### `int value() const override`
- Source: `tests/core/test_adapter_signing_focused.cpp`:132
- Brief: n/a
- Parameters: none

### FakeAlphaImpl

#### `FakeAlphaImpl(int v)`
- Source: `tests/core/test_adapter_registry_focused.cpp`:27
- Brief: n/a
- Parameters:
  - `v` (int): n/a

#### `int value() const override`
- Source: `tests/core/test_adapter_registry_focused.cpp`:28
- Brief: n/a
- Parameters: none

### FakeBetaImpl

#### `FakeBetaImpl(std::string l)`
- Source: `tests/core/test_adapter_registry_focused.cpp`:33
- Brief: n/a
- Parameters:
  - `l` (std::string): n/a

#### `std::string label() const override`
- Source: `tests/core/test_adapter_registry_focused.cpp`:34
- Brief: n/a
- Parameters: none

### GraphIndexBench

#### `void SetUp(const ::benchmark::State &state) override`
- Source: `benchmarks/core/bench_core_performance.cpp`:378
- Brief: n/a
- Parameters:
  - `state` (const ::benchmark::State &): n/a

#### `void TearDown(const ::benchmark::State &state) override`
- Source: `benchmarks/core/bench_core_performance.cpp`:390
- Brief: n/a
- Parameters:
  - `state` (const ::benchmark::State &): n/a

### ICustomAdapter

#### `int id() const =0`
- Source: `tests/core/test_concerns_context_focused.cpp`:32
- Brief: n/a
- Parameters: none

#### `~ICustomAdapter()=default`
- Source: `tests/core/test_concerns_context_focused.cpp`:31
- Brief: n/a
- Parameters: none

### IFakeAdapter

#### `int value() const =0`
- Source: `tests/core/test_adapter_signing_focused.cpp`:127
- Brief: n/a
- Parameters: none

#### `~IFakeAdapter()=default`
- Source: `tests/core/test_adapter_signing_focused.cpp`:126
- Brief: n/a
- Parameters: none

### IFakeAlpha

#### `int value() const =0`
- Source: `tests/core/test_adapter_registry_focused.cpp`:18
- Brief: n/a
- Parameters: none

#### `~IFakeAlpha()=default`
- Source: `tests/core/test_adapter_registry_focused.cpp`:17
- Brief: n/a
- Parameters: none

### IFakeBeta

#### `std::string label() const =0`
- Source: `tests/core/test_adapter_registry_focused.cpp`:23
- Brief: n/a
- Parameters: none

#### `~IFakeBeta()=default`
- Source: `tests/core/test_adapter_registry_focused.cpp`:22
- Brief: n/a
- Parameters: none

### QueryEngineBench

#### `void SetUp(const ::benchmark::State &state) override`
- Source: `benchmarks/core/bench_core_performance.cpp`:297
- Brief: n/a
- Parameters:
  - `state` (const ::benchmark::State &): n/a

#### `void TearDown(const ::benchmark::State &state) override`
- Source: `benchmarks/core/bench_core_performance.cpp`:328
- Brief: n/a
- Parameters:
  - `state` (const ::benchmark::State &): n/a

### RejectAllValidator

#### `bool validate(const AdapterMetadata &) override`
- Source: `tests/core/test_adapter_registry_focused.cpp`:128
- Brief: Validate.
- Parameters:
  - `m` (const AdapterMetadata &): Input parameter.
- Return: True when the operation succeeds.
- Details: m Input parameter. True when the operation succeeds.

### SecondaryIndexBench

#### `void SetUp(const ::benchmark::State &state) override`
- Source: `benchmarks/core/bench_core_performance.cpp`:137
- Brief: n/a
- Parameters:
  - `state` (const ::benchmark::State &): n/a

#### `void TearDown(const ::benchmark::State &state) override`
- Source: `benchmarks/core/bench_core_performance.cpp`:178
- Brief: n/a
- Parameters:
  - `state` (const ::benchmark::State &): n/a

### TempFile

#### `TempFile(const std::string &suffix, const std::string &content="")`
- Source: `tests/core/test_plugin_loading_focused.cpp`:19
- Brief: n/a
- Parameters:
  - `suffix` (const std::string &): n/a
  - `content` (const std::string &): n/a

#### `~TempFile()`
- Source: `tests/core/test_plugin_loading_focused.cpp`:29
- Brief: n/a
- Parameters: none

### TimeseriesBench

#### `void SetUp(const ::benchmark::State &state) override`
- Source: `benchmarks/core/bench_core_performance.cpp`:422
- Brief: n/a
- Parameters:
  - `state` (const ::benchmark::State &): n/a

#### `void TearDown(const ::benchmark::State &state) override`
- Source: `benchmarks/core/bench_core_performance.cpp`:433
- Brief: n/a
- Parameters:
  - `state` (const ::benchmark::State &): n/a

### VectorIndexBench

#### `void SetUp(const ::benchmark::State &state) override`
- Source: `benchmarks/core/bench_core_performance.cpp`:25
- Brief: n/a
- Parameters:
  - `state` (const ::benchmark::State &): n/a

#### `void TearDown(const ::benchmark::State &state) override`
- Source: `benchmarks/core/bench_core_performance.cpp`:36
- Brief: n/a
- Parameters:
  - `state` (const ::benchmark::State &): n/a

#### `std::vector< float > genVec(size_t dim)`
- Source: `benchmarks/core/bench_core_performance.cpp`:45
- Brief: n/a
- Parameters:
  - `dim` (size_t): n/a

### bench_core_performance.cpp

#### `BENCHMARK(SIMDDistanceThroughput_PERFD3)`
- Source: `benchmarks/core/bench_core_performance.cpp`:129
- Brief: n/a
- Parameters:
  - `<unnamed>` (SIMDDistanceThroughput_PERFD3): n/a

#### `BENCHMARK_DEFINE_F(SecondaryIndexBench, BM_SecondaryIndex_BatchInsert)(benchmark`
- Source: `benchmarks/core/bench_core_performance.cpp`:234
- Brief: n/a
- Parameters:
  - `<unnamed>` (SecondaryIndexBench): n/a
  - `<unnamed>` (BM_SecondaryIndex_BatchInsert): n/a

#### `BENCHMARK_F(GraphIndexBench, AddEdges)(benchmark`
- Source: `benchmarks/core/bench_core_performance.cpp`:402
- Brief: n/a
- Parameters:
  - `<unnamed>` (GraphIndexBench): n/a
  - `<unnamed>` (AddEdges): n/a

#### `BENCHMARK_F(QueryEngineBench, PointLookupWithDeserialize)(benchmark`
- Source: `benchmarks/core/bench_core_performance.cpp`:356
- Brief: n/a
- Parameters:
  - `<unnamed>` (QueryEngineBench): n/a
  - `<unnamed>` (PointLookupWithDeserialize): n/a

#### `BENCHMARK_F(QueryEngineBench, SimpleEvaluation)(benchmark`
- Source: `benchmarks/core/bench_core_performance.cpp`:344
- Brief: n/a
- Parameters:
  - `<unnamed>` (QueryEngineBench): n/a
  - `<unnamed>` (SimpleEvaluation): n/a

#### `BENCHMARK_F(SecondaryIndexBench, BM_SecondaryIndex_SingleInsert)(benchmark`
- Source: `benchmarks/core/bench_core_performance.cpp`:214
- Brief: n/a
- Parameters:
  - `<unnamed>` (SecondaryIndexBench): n/a
  - `<unnamed>` (BM_SecondaryIndex_SingleInsert): n/a

#### `BENCHMARK_F(SecondaryIndexBench, IndexInsert)(benchmark`
- Source: `benchmarks/core/bench_core_performance.cpp`:190
- Brief: n/a
- Parameters:
  - `<unnamed>` (SecondaryIndexBench): n/a
  - `<unnamed>` (IndexInsert): n/a

#### `BENCHMARK_F(SecondaryIndexBench, RawWriteOnly)(benchmark`
- Source: `benchmarks/core/bench_core_performance.cpp`:268
- Brief: n/a
- Parameters:
  - `<unnamed>` (SecondaryIndexBench): n/a
  - `<unnamed>` (RawWriteOnly): n/a

#### `BENCHMARK_F(TimeseriesBench, InsertTimepoints)(benchmark`
- Source: `benchmarks/core/bench_core_performance.cpp`:443
- Brief: n/a
- Parameters:
  - `<unnamed>` (TimeseriesBench): n/a
  - `<unnamed>` (InsertTimepoints): n/a

#### `BENCHMARK_F(VectorIndexBench, InsertPlaintext)(benchmark`
- Source: `benchmarks/core/bench_core_performance.cpp`:56
- Brief: n/a
- Parameters:
  - `<unnamed>` (VectorIndexBench): n/a
  - `<unnamed>` (InsertPlaintext): n/a

#### `BENCHMARK_F(VectorIndexBench, ParallelBatchInsert_PERFD3)(benchmark`
- Source: `benchmarks/core/bench_core_performance.cpp`:73
- Brief: n/a
- Parameters:
  - `<unnamed>` (VectorIndexBench): n/a
  - `<unnamed>` (ParallelBatchInsert_PERFD3): n/a

#### `BENCHMARK_MAIN()`
- Source: `benchmarks/core/bench_core_performance.cpp`:459
- Brief: n/a
- Parameters: none

#### `BENCHMARK_REGISTER_F(SecondaryIndexBench, BM_SecondaryIndex_BatchInsert) -> Arg(64)`
- Source: `benchmarks/core/bench_core_performance.cpp`:265
- Brief: n/a
- Parameters:
  - `<unnamed>` (SecondaryIndexBench): n/a
  - `<unnamed>` (BM_SecondaryIndex_BatchInsert): n/a

#### `void SIMDDistanceThroughput_PERFD3(benchmark::State &state)`
- Source: `benchmarks/core/bench_core_performance.cpp`:105
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

### test_adapter_registry_focused.cpp

#### `TEST(AdapterRegistryTest, AR_01_EmptyRegistryResolveReturnsNull)`
- Source: `tests/core/test_adapter_registry_focused.cpp`:41
- Brief: n/a
- Parameters:
  - `<unnamed>` (AdapterRegistryTest): n/a
  - `<unnamed>` (AR_01_EmptyRegistryResolveReturnsNull): n/a

#### `TEST(AdapterRegistryTest, AR_02_RegisterAndResolve)`
- Source: `tests/core/test_adapter_registry_focused.cpp`:50
- Brief: n/a
- Parameters:
  - `<unnamed>` (AdapterRegistryTest): n/a
  - `<unnamed>` (AR_02_RegisterAndResolve): n/a

#### `TEST(AdapterRegistryTest, AR_03_WrongTypeResolveReturnsNull)`
- Source: `tests/core/test_adapter_registry_focused.cpp`:63
- Brief: n/a
- Parameters:
  - `<unnamed>` (AdapterRegistryTest): n/a
  - `<unnamed>` (AR_03_WrongTypeResolveReturnsNull): n/a

#### `TEST(AdapterRegistryTest, AR_04_ApiVersionBelowCurrentThrows)`
- Source: `tests/core/test_adapter_registry_focused.cpp`:76
- Brief: n/a
- Parameters:
  - `<unnamed>` (AdapterRegistryTest): n/a
  - `<unnamed>` (AR_04_ApiVersionBelowCurrentThrows): n/a

#### `TEST(AdapterRegistryTest, AR_05_EmptyIdThrows)`
- Source: `tests/core/test_adapter_registry_focused.cpp`:94
- Brief: n/a
- Parameters:
  - `<unnamed>` (AdapterRegistryTest): n/a
  - `<unnamed>` (AR_05_EmptyIdThrows): n/a

#### `TEST(AdapterRegistryTest, AR_06_HotSwapReplacesAdapter)`
- Source: `tests/core/test_adapter_registry_focused.cpp`:107
- Brief: n/a
- Parameters:
  - `<unnamed>` (AdapterRegistryTest): n/a
  - `<unnamed>` (AR_06_HotSwapReplacesAdapter): n/a

#### `TEST(AdapterRegistryTest, AR_07_ValidatorRejectionThrows)`
- Source: `tests/core/test_adapter_registry_focused.cpp`:131
- Brief: n/a
- Parameters:
  - `<unnamed>` (AdapterRegistryTest): n/a
  - `<unnamed>` (AR_07_ValidatorRejectionThrows): n/a

#### `TEST(AdapterRegistryTest, AR_08_LoadFromPluginNonExistentPathReturnsFalse)`
- Source: `tests/core/test_adapter_registry_focused.cpp`:145
- Brief: n/a
- Parameters:
  - `<unnamed>` (AdapterRegistryTest): n/a
  - `<unnamed>` (AR_08_LoadFromPluginNonExistentPathReturnsFalse): n/a

#### `TEST(AdapterRegistryTest, AR_09_CountAndHasAdapterCorrect)`
- Source: `tests/core/test_adapter_registry_focused.cpp`:155
- Brief: n/a
- Parameters:
  - `<unnamed>` (AdapterRegistryTest): n/a
  - `<unnamed>` (AR_09_CountAndHasAdapterCorrect): n/a

#### `TEST(AdapterRegistryTest, AR_10_ConcurrentResolveNoCrash)`
- Source: `tests/core/test_adapter_registry_focused.cpp`:177
- Brief: n/a
- Parameters:
  - `<unnamed>` (AdapterRegistryTest): n/a
  - `<unnamed>` (AR_10_ConcurrentResolveNoCrash): n/a

#### `TEST(AdapterRegistryTest, AR_11_NullptrAdapterThrows)`
- Source: `tests/core/test_adapter_registry_focused.cpp`:212
- Brief: n/a
- Parameters:
  - `<unnamed>` (AdapterRegistryTest): n/a
  - `<unnamed>` (AR_11_NullptrAdapterThrows): n/a

### test_adapter_signing_focused.cpp

#### `TEST(AdapterSigningTest, SGN_01_EmptySignatureNotPresent)`
- Source: `tests/core/test_adapter_signing_focused.cpp`:11
- Brief: n/a
- Parameters:
  - `<unnamed>` (AdapterSigningTest): n/a
  - `<unnamed>` (SGN_01_EmptySignatureNotPresent): n/a

#### `TEST(AdapterSigningTest, SGN_02_PopulatedSignatureIsPresent)`
- Source: `tests/core/test_adapter_signing_focused.cpp`:19
- Brief: n/a
- Parameters:
  - `<unnamed>` (AdapterSigningTest): n/a
  - `<unnamed>` (SGN_02_PopulatedSignatureIsPresent): n/a

#### `TEST(AdapterSigningTest, SGN_03_Sha256HexEmptyInput)`
- Source: `tests/core/test_adapter_signing_focused.cpp`:29
- Brief: n/a
- Parameters:
  - `<unnamed>` (AdapterSigningTest): n/a
  - `<unnamed>` (SGN_03_Sha256HexEmptyInput): n/a

#### `TEST(AdapterSigningTest, SGN_04_CanonicalStringFormat)`
- Source: `tests/core/test_adapter_signing_focused.cpp`:39
- Brief: n/a
- Parameters:
  - `<unnamed>` (AdapterSigningTest): n/a
  - `<unnamed>` (SGN_04_CanonicalStringFormat): n/a

#### `TEST(AdapterSigningTest, SGN_04b_CanonicalStringEmptyDescription)`
- Source: `tests/core/test_adapter_signing_focused.cpp`:49
- Brief: n/a
- Parameters:
  - `<unnamed>` (AdapterSigningTest): n/a
  - `<unnamed>` (SGN_04b_CanonicalStringEmptyDescription): n/a

#### `TEST(AdapterSigningTest, SGN_05_MatchingDigestAccepted)`
- Source: `tests/core/test_adapter_signing_focused.cpp`:61
- Brief: n/a
- Parameters:
  - `<unnamed>` (AdapterSigningTest): n/a
  - `<unnamed>` (SGN_05_MatchingDigestAccepted): n/a

#### `TEST(AdapterSigningTest, SGN_06_MismatchedDigestRejected)`
- Source: `tests/core/test_adapter_signing_focused.cpp`:80
- Brief: n/a
- Parameters:
  - `<unnamed>` (AdapterSigningTest): n/a
  - `<unnamed>` (SGN_06_MismatchedDigestRejected): n/a

#### `TEST(AdapterSigningTest, SGN_07_EmptySignatureRejected)`
- Source: `tests/core/test_adapter_signing_focused.cpp`:96
- Brief: n/a
- Parameters:
  - `<unnamed>` (AdapterSigningTest): n/a
  - `<unnamed>` (SGN_07_EmptySignatureRejected): n/a

#### `TEST(AdapterSigningTest, SGN_08_UnknownAlgorithmRejected)`
- Source: `tests/core/test_adapter_signing_focused.cpp`:110
- Brief: n/a
- Parameters:
  - `<unnamed>` (AdapterSigningTest): n/a
  - `<unnamed>` (SGN_08_UnknownAlgorithmRejected): n/a

#### `TEST(AdapterSigningTest, SGN_09_RegisterAdapterPassesWithValidSignature)`
- Source: `tests/core/test_adapter_signing_focused.cpp`:136
- Brief: n/a
- Parameters:
  - `<unnamed>` (AdapterSigningTest): n/a
  - `<unnamed>` (SGN_09_RegisterAdapterPassesWithValidSignature): n/a

#### `TEST(AdapterSigningTest, SGN_10_RegisterAdapterThrowsWithWrongSignature)`
- Source: `tests/core/test_adapter_signing_focused.cpp`:163
- Brief: n/a
- Parameters:
  - `<unnamed>` (AdapterSigningTest): n/a
  - `<unnamed>` (SGN_10_RegisterAdapterThrowsWithWrongSignature): n/a

### test_circuit_breaker_focused.cpp

#### `TEST(CircuitBreakerTest, CB_01_ClosedStateExecutesFn)`
- Source: `tests/core/test_circuit_breaker_focused.cpp`:36
- Brief: n/a
- Parameters:
  - `<unnamed>` (CircuitBreakerTest): n/a
  - `<unnamed>` (CB_01_ClosedStateExecutesFn): n/a

#### `TEST(CircuitBreakerTest, CB_02_OpenAfterThreshold_CallUsesFallback)`
- Source: `tests/core/test_circuit_breaker_focused.cpp`:55
- Brief: n/a
- Parameters:
  - `<unnamed>` (CircuitBreakerTest): n/a
  - `<unnamed>` (CB_02_OpenAfterThreshold_CallUsesFallback): n/a

#### `TEST(CircuitBreakerTest, CB_03_GetStateOpenAfterThreshold)`
- Source: `tests/core/test_circuit_breaker_focused.cpp`:74
- Brief: n/a
- Parameters:
  - `<unnamed>` (CircuitBreakerTest): n/a
  - `<unnamed>` (CB_03_GetStateOpenAfterThreshold): n/a

#### `TEST(CircuitBreakerTest, CB_04_CallReturnTypeMatchesFn)`
- Source: `tests/core/test_circuit_breaker_focused.cpp`:85
- Brief: n/a
- Parameters:
  - `<unnamed>` (CircuitBreakerTest): n/a
  - `<unnamed>` (CB_04_CallReturnTypeMatchesFn): n/a

#### `TEST(CircuitBreakerTest, CB_05_VoidFnAndFallback)`
- Source: `tests/core/test_circuit_breaker_focused.cpp`:99
- Brief: n/a
- Parameters:
  - `<unnamed>` (CircuitBreakerTest): n/a
  - `<unnamed>` (CB_05_VoidFnAndFallback): n/a

#### `TEST(CircuitBreakerTest, CB_06_ResetClosesCircuit)`
- Source: `tests/core/test_circuit_breaker_focused.cpp`:126
- Brief: n/a
- Parameters:
  - `<unnamed>` (CircuitBreakerTest): n/a
  - `<unnamed>` (CB_06_ResetClosesCircuit): n/a

#### `TEST(CircuitBreakerTest, CB_07_HalfOpenAllowsProbeCall)`
- Source: `tests/core/test_circuit_breaker_focused.cpp`:144
- Brief: n/a
- Parameters:
  - `<unnamed>` (CircuitBreakerTest): n/a
  - `<unnamed>` (CB_07_HalfOpenAllowsProbeCall): n/a

#### `TEST(CircuitBreakerTest, CB_08_StateObservableViaGetState)`
- Source: `tests/core/test_circuit_breaker_focused.cpp`:170
- Brief: n/a
- Parameters:
  - `<unnamed>` (CircuitBreakerTest): n/a
  - `<unnamed>` (CB_08_StateObservableViaGetState): n/a

#### `DefaultCircuitBreaker makeBreaker(size_t threshold=3, std::chrono::seconds timeout=std::chrono::seconds(0), size_t success_threshold=1)`
- Source: `tests/core/test_circuit_breaker_focused.cpp`:16
- Brief: n/a
- Parameters:
  - `threshold` (size_t): n/a
  - `timeout` (std::chrono::seconds): n/a
  - `success_threshold` (size_t): n/a
- Details: Build a DefaultCircuitBreaker with a low failure threshold and zero timeout so OPEN→HALF_OPEN transitions happen immediately in tests.

#### `void tripBreaker(ICircuitBreaker &cb, size_t n)`
- Source: `tests/core/test_circuit_breaker_focused.cpp`:27
- Brief: Trip the circuit by calling recordFailure() n times.
- Parameters:
  - `cb` (ICircuitBreaker &): n/a
  - `n` (size_t): n/a

### test_concerns_context_focused.cpp

#### `TEST(ConcernsContextTest, CCT_01_CreateCustomNoOpNoThrow)`
- Source: `tests/core/test_concerns_context_focused.cpp`:43
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConcernsContextTest): n/a
  - `<unnamed>` (CCT_01_CreateCustomNoOpNoThrow): n/a

#### `TEST(ConcernsContextTest, CCT_02_CreateCustomWithAdapters)`
- Source: `tests/core/test_concerns_context_focused.cpp`:53
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConcernsContextTest): n/a
  - `<unnamed>` (CCT_02_CreateCustomWithAdapters): n/a

#### `TEST(ConcernsContextTest, CCT_03_CreateFromConfig)`
- Source: `tests/core/test_concerns_context_focused.cpp`:70
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConcernsContextTest): n/a
  - `<unnamed>` (CCT_03_CreateFromConfig): n/a

#### `TEST(ConcernsContextTest, CCT_04_DefaultContextHasNonNullLoggerTracer)`
- Source: `tests/core/test_concerns_context_focused.cpp`:90
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConcernsContextTest): n/a
  - `<unnamed>` (CCT_04_DefaultContextHasNonNullLoggerTracer): n/a

#### `TEST(ConcernsContextTest, CCT_05_ResolveILogger_NonNull)`
- Source: `tests/core/test_concerns_context_focused.cpp`:101
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConcernsContextTest): n/a
  - `<unnamed>` (CCT_05_ResolveILogger_NonNull): n/a

#### `TEST(ConcernsContextTest, CCT_06_ResolveITracer_NonNull)`
- Source: `tests/core/test_concerns_context_focused.cpp`:111
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConcernsContextTest): n/a
  - `<unnamed>` (CCT_06_ResolveITracer_NonNull): n/a

#### `TEST(ConcernsContextTest, CCT_07_ResolveIMetrics_NonNull)`
- Source: `tests/core/test_concerns_context_focused.cpp`:121
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConcernsContextTest): n/a
  - `<unnamed>` (CCT_07_ResolveIMetrics_NonNull): n/a

#### `TEST(ConcernsContextTest, CCT_08_ResolveICache_NonNull)`
- Source: `tests/core/test_concerns_context_focused.cpp`:131
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConcernsContextTest): n/a
  - `<unnamed>` (CCT_08_ResolveICache_NonNull): n/a

#### `TEST(ConcernsContextTest, CCT_09_ReplaceLoggerSwaps)`
- Source: `tests/core/test_concerns_context_focused.cpp`:141
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConcernsContextTest): n/a
  - `<unnamed>` (CCT_09_ReplaceLoggerSwaps): n/a

#### `TEST(ConcernsContextTest, CCT_10_ReplaceMetricsSwaps)`
- Source: `tests/core/test_concerns_context_focused.cpp`:153
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConcernsContextTest): n/a
  - `<unnamed>` (CCT_10_ReplaceMetricsSwaps): n/a

#### `TEST(ConcernsContextTest, CCT_11_NullptrReplaceLoggerThrows)`
- Source: `tests/core/test_concerns_context_focused.cpp`:164
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConcernsContextTest): n/a
  - `<unnamed>` (CCT_11_NullptrReplaceLoggerThrows): n/a

#### `TEST(ConcernsContextTest, CCT_12_NullptrReplaceMetricsThrowsAndSwapVisible)`
- Source: `tests/core/test_concerns_context_focused.cpp`:173
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConcernsContextTest): n/a
  - `<unnamed>` (CCT_12_NullptrReplaceMetricsThrowsAndSwapVisible): n/a

#### `TEST(ConcernsContextTest, CCT_13_AdapterRegistryRegisterAndResolve)`
- Source: `tests/core/test_concerns_context_focused.cpp`:187
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConcernsContextTest): n/a
  - `<unnamed>` (CCT_13_AdapterRegistryRegisterAndResolve): n/a

#### `TEST(ConcernsContextTest, CCT_14_AdapterRegistryHotSwap)`
- Source: `tests/core/test_concerns_context_focused.cpp`:200
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConcernsContextTest): n/a
  - `<unnamed>` (CCT_14_AdapterRegistryHotSwap): n/a

#### `TEST(ConcernsContextTest, CCT_15_AdapterRegistryCountCorrect)`
- Source: `tests/core/test_concerns_context_focused.cpp`:217
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConcernsContextTest): n/a
  - `<unnamed>` (CCT_15_AdapterRegistryCountCorrect): n/a

#### `TEST(ConcernsContextTest, CCT_16_AdapterRegistryHasAdapter)`
- Source: `tests/core/test_concerns_context_focused.cpp`:234
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConcernsContextTest): n/a
  - `<unnamed>` (CCT_16_AdapterRegistryHasAdapter): n/a

#### `TEST(ConcernsContextTest, CCT_17_ConcurrentResolvesNoCrash)`
- Source: `tests/core/test_concerns_context_focused.cpp`:246
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConcernsContextTest): n/a
  - `<unnamed>` (CCT_17_ConcurrentResolvesNoCrash): n/a

#### `TEST(ConcernsContextTest, CCT_18_ConcurrentReplaceAndReadNoCrash)`
- Source: `tests/core/test_concerns_context_focused.cpp`:277
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConcernsContextTest): n/a
  - `<unnamed>` (CCT_18_ConcurrentReplaceAndReadNoCrash): n/a

#### `std::shared_ptr< ConcernsContext > makeNoOp()`
- Source: `tests/core/test_concerns_context_focused.cpp`:21
- Brief: Create a fully no-op ConcernsContext without triggering ProductionMode check.
- Parameters: none

### test_core_smoke.cpp

#### `TEST(CoreSmokeTest, NoOpLoggerConstructsWithoutThrow)`
- Source: `tests/core/test_core_smoke.cpp`:10
- Brief: n/a
- Parameters:
  - `<unnamed>` (CoreSmokeTest): n/a
  - `<unnamed>` (NoOpLoggerConstructsWithoutThrow): n/a

#### `TEST(CoreSmokeTest, TraceContextDefaultIsEmpty)`
- Source: `tests/core/test_core_smoke.cpp`:20
- Brief: n/a
- Parameters:
  - `<unnamed>` (CoreSmokeTest): n/a
  - `<unnamed>` (TraceContextDefaultIsEmpty): n/a

### test_plugin_loading_focused.cpp

#### `TEST(PluginLoadingTest, PL_01_EmptyPathReturnsFalse)`
- Source: `tests/core/test_plugin_loading_focused.cpp`:38
- Brief: n/a
- Parameters:
  - `<unnamed>` (PluginLoadingTest): n/a
  - `<unnamed>` (PL_01_EmptyPathReturnsFalse): n/a

#### `TEST(PluginLoadingTest, PL_02_NonExistentPathReturnsFalse)`
- Source: `tests/core/test_plugin_loading_focused.cpp`:46
- Brief: n/a
- Parameters:
  - `<unnamed>` (PluginLoadingTest): n/a
  - `<unnamed>` (PL_02_NonExistentPathReturnsFalse): n/a

#### `TEST(PluginLoadingTest, PL_03_NotALibraryReturnsFalse)`
- Source: `tests/core/test_plugin_loading_focused.cpp`:57
- Brief: n/a
- Parameters:
  - `<unnamed>` (PluginLoadingTest): n/a
  - `<unnamed>` (PL_03_NotALibraryReturnsFalse): n/a

#### `TEST(PluginLoadingTest, PL_04_RequireSignatureMissingSigFileReturnsFalse)`
- Source: `tests/core/test_plugin_loading_focused.cpp`:66
- Brief: n/a
- Parameters:
  - `<unnamed>` (PluginLoadingTest): n/a
  - `<unnamed>` (PL_04_RequireSignatureMissingSigFileReturnsFalse): n/a

#### `TEST(PluginLoadingTest, PL_05_RequireSignatureDigestMismatchReturnsFalse)`
- Source: `tests/core/test_plugin_loading_focused.cpp`:77
- Brief: n/a
- Parameters:
  - `<unnamed>` (PluginLoadingTest): n/a
  - `<unnamed>` (PL_05_RequireSignatureDigestMismatchReturnsFalse): n/a

#### `TEST(PluginLoadingTest, PL_06_TrustAllSkipsSignatureCheck)`
- Source: `tests/core/test_plugin_loading_focused.cpp`:96
- Brief: n/a
- Parameters:
  - `<unnamed>` (PluginLoadingTest): n/a
  - `<unnamed>` (PL_06_TrustAllSkipsSignatureCheck): n/a

#### `TEST(PluginLoadingTest, PL_07_SetTrustPolicyDoesNotCorruptRegistry)`
- Source: `tests/core/test_plugin_loading_focused.cpp`:113
- Brief: n/a
- Parameters:
  - `<unnamed>` (PluginLoadingTest): n/a
  - `<unnamed>` (PL_07_SetTrustPolicyDoesNotCorruptRegistry): n/a

#### `TEST(PluginLoadingTest, PL_08_ConcurrentLoadFromPluginBadPathNoCrash)`
- Source: `tests/core/test_plugin_loading_focused.cpp`:136
- Brief: n/a
- Parameters:
  - `<unnamed>` (PluginLoadingTest): n/a
  - `<unnamed>` (PL_08_ConcurrentLoadFromPluginBadPathNoCrash): n/a

### themis::IndexManagerBuilder

#### `IndexManagerBuilder()=default`
- Source: `include/core/index_initialization.h`:24
- Brief: n/a
- Parameters: none

#### `std::shared_ptr< IndexManager > build()`
- Source: `include/core/index_initialization.h`:64
- Brief: Build.
- Parameters: none
- Return: Return value.
- Details: Return value. Calls: setRocksDB().

#### `IndexManagerBuilder standard()`
- Source: `include/core/index_initialization.h`:81
- Brief: Standard.
- Parameters: none
- Return: Return value.
- Details: Return value. Implements standard without additional internal calls.

#### `IndexManagerBuilder & withEvaluator(IExpressionEvaluatorPtr evaluator)`
- Source: `include/core/index_initialization.h`:32
- Brief: With Evaluator.
- Parameters:
  - `evaluator` (IExpressionEvaluatorPtr): Input parameter.
- Return: Return value.
- Details: evaluator Input parameter. Return value. Implements withEvaluator without additional internal calls.

#### `IndexManagerBuilder & withRocksDB(std::shared_ptr< RocksDBWrapper > db)`
- Source: `include/core/index_initialization.h`:54
- Brief: With Rocks DB.
- Parameters:
  - `db` (std::shared_ptr< RocksDBWrapper >): Input parameter.
- Return: Return value.
- Details: db Input parameter. Return value. Implements withRocksDB without additional internal calls.

#### `IndexManagerBuilder & withStorage(IStorageEnginePtr storage)`
- Source: `include/core/index_initialization.h`:43
- Brief: With Storage.
- Parameters:
  - `storage` (IStorageEnginePtr): Input parameter.
- Return: Return value.
- Details: storage Input parameter. Return value. Implements withStorage without additional internal calls.

### themis::QueryEngineBuilder

#### `QueryEngineBuilder()=default`
- Source: `include/core/query_engine_builder.h`:47
- Brief: n/a
- Parameters: none

#### `std::shared_ptr< query::QueryEngine > build()`
- Source: `include/core/query_engine_builder.h`:77
- Brief: Build.
- Parameters: none
- Return: Return value.
- Throws:
  - std::runtime_error: if an error occurs.
- Details: Return value. std::runtime_error if an error occurs. Implements build without additional internal calls.

#### `QueryEngineBuilder standard()`
- Source: `include/core/query_engine_builder.h`:91
- Brief: Standard.
- Parameters: none
- Return: Return value.
- Details: Return value. Implements standard without additional internal calls.

#### `QueryEngineBuilder & withIndexManager(IIndexManagerPtr index_manager)`
- Source: `include/core/query_engine_builder.h`:66
- Brief: With Index Manager.
- Parameters:
  - `index_manager` (IIndexManagerPtr): Input parameter.
- Return: Return value.
- Details: index_manager Input parameter. Return value. Implements withIndexManager without additional internal calls.

#### `QueryEngineBuilder & withStorage(IStorageEnginePtr storage)`
- Source: `include/core/query_engine_builder.h`:55
- Brief: With Storage.
- Parameters:
  - `storage` (IStorageEnginePtr): Input parameter.
- Return: Return value.
- Details: storage Input parameter. Return value. Implements withStorage without additional internal calls.

### themis::SecurityLayerBuilder

#### `SecurityLayerBuilder()`
- Source: `include/core/security_initialization.h`:41
- Brief: n/a
- Parameters: none

#### `SecurityLayer build()`
- Source: `include/core/security_initialization.h`:85
- Brief: Build.
- Parameters: none
- Return: Return value.
- Throws:
  - std::runtime_error: if an error occurs.
- Details: Return value. Return value. std::runtime_error if an error occurs. Calls: core::ProductionMode::isEnabled(), has_value(), createKeyProvider(), value(), empty(), setEncryptionConfig(), loadConfig(), core::ConfigValidator::validateJWTConfig().

#### `IKeyProviderPtr createKeyProvider(KeyProviderType type, const std::string &config_json)`
- Source: `include/core/security_initialization.h`:115
- Brief: Create Key Provider.
- Parameters:
  - `type` (KeyProviderType): Input parameter.
  - `config_json` (const std::string &): Input parameter.
- Return: Return value.
- Details: type Input parameter. config_json Input parameter. Return value.

#### `std::string loadFile(const std::string &path)`
- Source: `include/core/security_initialization.h`:107
- Brief: Load File.
- Parameters:
  - `path` (const std::string &): Input parameter.
- Return: Return value.
- Throws:
  - std::runtime_error: if an error occurs.
- Details: path Input parameter. Return value. path Input parameter. Return value. std::runtime_error if an error occurs. Calls: file(), is_open(), rdbuf(), str().

#### `SecurityLayerBuilder standard()`
- Source: `include/core/security_initialization.h`:91
- Brief: Standard.
- Parameters: none
- Return: Return value.
- Details: Return value. Return value. Calls: SecurityLayerBuilder(), withKeyProvider().

#### `SecurityLayerBuilder & withFieldEncryption(const EncryptionConfig &config)`
- Source: `include/core/security_initialization.h`:52
- Brief: With Field Encryption.
- Parameters:
  - `config` (const EncryptionConfig &): Input parameter.
- Return: Return value.
- Details: config Input parameter. Return value.

#### `SecurityLayerBuilder & withJWT(const auth::JWTValidatorConfig &config)`
- Source: `include/core/security_initialization.h`:78
- Brief: With JWT.
- Parameters:
  - `config` (const auth::JWTValidatorConfig &): Input parameter.
- Return: Return value.
- Details: config Input parameter. Return value.

#### `SecurityLayerBuilder & withJWT(const std::string &cert_file, const std::vector< std::string > &allowed_issuers)`
- Source: `include/core/security_initialization.h`:69
- Brief: With JWT.
- Parameters:
  - `cert_file` (const std::string &): Input parameter.
  - `allowed_issuers` (const std::vector< std::string > &): Input parameter.
- Return: Return value.
- Details: cert_file Input parameter. allowed_issuers Input parameter. Return value.

#### `SecurityLayerBuilder & withKeyProvider(KeyProviderType type, const std::string &config_json="{}")`
- Source: `include/core/security_initialization.h`:43
- Brief: With Key Provider.
- Parameters:
  - `type` (KeyProviderType): Input parameter.
  - `config_json` (const std::string &): Input parameter.
- Return: Return value.
- Details: type Input parameter. config_json Input parameter. Return value.

#### `SecurityLayerBuilder & withRBACPolicy(const std::string &policy_file)`
- Source: `include/core/security_initialization.h`:60
- Brief: With RBACPolicy.
- Parameters:
  - `policy_file` (const std::string &): Input parameter.
- Return: Return value.
- Details: policy_file Input parameter. Return value.

### themis::StorageEngineBuilder

#### `StorageEngineBuilder()=default`
- Source: `include/core/storage_initialization.h`:25
- Brief: n/a
- Parameters: none

#### `std::shared_ptr< StorageEngine > build()`
- Source: `include/core/storage_initialization.h`:77
- Brief: Build.
- Parameters: none
- Return: Return value.
- Throws:
  - std::runtime_error: if an error occurs.
- Details: Return value. std::runtime_error if an error occurs. Implements build without additional internal calls.

#### `StorageEngineBuilder standard()`
- Source: `include/core/storage_initialization.h`:100
- Brief: Standard.
- Parameters: none
- Return: Return value.
- Details: Return value. Calls: withEvaluator(), StorageEngine::createDefaultEvaluator(), withEncryption(), StorageEngine::createDefaultEncryption(), withKeyProvider(), StorageEngine::createDefaultKeyProvider(), withIndexManager(), StorageEngine::createDefaultIndexManager().

#### `StorageEngineBuilder & withEncryption(IFieldEncryptionPtr enc)`
- Source: `include/core/storage_initialization.h`:44
- Brief: With Encryption.
- Parameters:
  - `enc` (IFieldEncryptionPtr): Input parameter.
- Return: Return value.
- Details: enc Input parameter. Return value. Implements withEncryption without additional internal calls.

#### `StorageEngineBuilder & withEvaluator(IExpressionEvaluatorPtr eval)`
- Source: `include/core/storage_initialization.h`:33
- Brief: With Evaluator.
- Parameters:
  - `eval` (IExpressionEvaluatorPtr): Input parameter.
- Return: Return value.
- Details: eval Input parameter. Return value. Implements withEvaluator without additional internal calls.

#### `StorageEngineBuilder & withIndexManager(IIndexManagerPtr index)`
- Source: `include/core/storage_initialization.h`:66
- Brief: With Index Manager.
- Parameters:
  - `index` (IIndexManagerPtr): Input parameter.
- Return: Return value.
- Details: index Input parameter. Return value. Implements withIndexManager without additional internal calls.

#### `StorageEngineBuilder & withKeyProvider(IKeyProviderPtr provider)`
- Source: `include/core/storage_initialization.h`:55
- Brief: With Key Provider.
- Parameters:
  - `provider` (IKeyProviderPtr): Input parameter.
- Return: Return value.
- Details: provider Input parameter. Return value. Implements withKeyProvider without additional internal calls.

### themis::core::AggregateHealthReport

#### `bool isHealthy() const`
- Source: `include/core/health_probe.h`:55
- Brief: n/a
- Parameters: none

#### `bool isReady() const`
- Source: `include/core/health_probe.h`:57
- Brief: n/a
- Parameters: none

### themis::core::ConfigValidator

#### `ValidationResult validateAdapterConfig(const std::string &logger_adapter, const std::string &tracer_adapter, const std::string &metrics_adapter, const std::string &cache_adapter, const std::string &circuit_breaker_adapter="default", const std::string &feature_flags_adapter="inmemory", const std::string &audit_adapter="noop", const std::string &secrets_adapter="noop", const std::string &cache_redis_url="")`
- Source: `include/core/config_validator.h`:203
- Brief: n/a
- Parameters:
  - `logger_adapter` (const std::string &): n/a
  - `tracer_adapter` (const std::string &): n/a
  - `metrics_adapter` (const std::string &): n/a
  - `cache_adapter` (const std::string &): n/a
  - `circuit_breaker_adapter` (const std::string &): n/a
  - `feature_flags_adapter` (const std::string &): n/a
  - `audit_adapter` (const std::string &): n/a
  - `secrets_adapter` (const std::string &): n/a
  - `cache_redis_url` (const std::string &): n/a

#### `ValidationResult validateCacheConfig(size_t max_size, uint64_t default_ttl)`
- Source: `include/core/config_validator.h`:269
- Brief: Validate Cache Config.
- Parameters:
  - `max_size` (size_t): Input parameter.
  - `default_ttl` (uint64_t): Input parameter.
- Return: Return value.
- Details: max_size Input parameter. default_ttl Input parameter. Return value. Calls: addWarning().

#### `ValidationResult validateJWTConfig(const auth::JWTValidatorConfig &config, bool production_mode)`
- Source: `include/core/config_validator.h`:104
- Brief: Validate JWTConfig.
- Parameters:
  - `config` (const auth::JWTValidatorConfig &): Input parameter.
  - `production_mode` (bool): Input parameter.
- Return: Return value.
- Details: config Input parameter. production_mode Input parameter. Return value. Calls: empty(), addError(), has_value(), addWarning(), count().

#### `ValidationResult validateLogConfig(const std::string &log_level, const std::string &log_pattern)`
- Source: `include/core/config_validator.h`:154
- Brief: Validate Log Config.
- Parameters:
  - `log_level` (const std::string &): Input parameter.
  - `log_pattern` (const std::string &): Input parameter.
- Return: Return value.
- Details: log_level Input parameter. log_pattern Input parameter. Return value. Calls: addError(), empty(), addWarning().

#### `ValidationResult validateTracingConfig(bool enabled, const std::string &endpoint, const std::string &service_name)`
- Source: `include/core/config_validator.h`:187
- Brief: Validate Tracing Config.
- Parameters:
  - `enabled` (bool): Input parameter.
  - `endpoint` (const std::string &): Input parameter.
  - `service_name` (const std::string &): Name of the service.
- Return: Return value.
- Details: enabled Input parameter. endpoint Input parameter. service_name Name of the service. Return value. Calls: empty(), addError(), addWarning().

#### `ValidationResult validateVaultConfig(const nlohmann::json &config)`
- Source: `include/core/config_validator.h`:67
- Brief: Validate Vault Config.
- Parameters:
  - `config` (const nlohmann::json &): Input parameter.
- Return: Return value.
- Details: config Input parameter. Return value. Calls: contains(), empty(), addError(), find().

### themis::core::ConfigValidator::ValidationResult

#### `void addError(const std::string &error)`
- Source: `include/core/config_validator.h`:35
- Brief: Add Error.
- Parameters:
  - `error` (const std::string &): Input parameter.
- Details: error Input parameter. Calls: push_back().

#### `void addWarning(const std::string &warning)`
- Source: `include/core/config_validator.h`:45
- Brief: Add Warning.
- Parameters:
  - `warning` (const std::string &): Input parameter.
- Details: warning Input parameter. Calls: push_back().

#### `std::string formatErrors() const`
- Source: `include/core/config_validator.h`:49
- Brief: n/a
- Parameters: none

### themis::core::IConfigHotReloader

#### `HotReloadResult lastReloadResult() const =0`
- Source: `include/core/config_hot_reloader.h`:108
- Brief: Last Reload Result.
- Parameters: none
- Return: Return value.
- Details: Return value.

#### `bool onConfigChange(const std::string &key_prefix, ConfigChangeCallback callback)=0`
- Source: `include/core/config_hot_reloader.h`:99
- Brief: On Config Change.
- Parameters:
  - `key_prefix` (const std::string &): Input parameter.
  - `callback` (ConfigChangeCallback): Input parameter.
- Return: True when the operation succeeds.
- Details: key_prefix Input parameter. callback Input parameter. True when the operation succeeds.

#### `HotReloadResult reload()=0`
- Source: `include/core/config_hot_reloader.h`:91
- Brief: Reload.
- Parameters: none
- Return: Return value.
- Details: Return value.

#### `std::chrono::milliseconds reloadInterval() const =0`
- Source: `include/core/config_hot_reloader.h`:114
- Brief: Reload Interval.
- Parameters: none
- Return: Return value.
- Details: Return value.

#### `void setReloadInterval(std::chrono::milliseconds interval)=0`
- Source: `include/core/config_hot_reloader.h`:120
- Brief: Set Reload Interval.
- Parameters:
  - `interval` (std::chrono::milliseconds): Input parameter.
- Details: interval Input parameter.

#### `bool unwatch(const std::string &config_path_or_key)=0`
- Source: `include/core/config_hot_reloader.h`:85
- Brief: Unwatch.
- Parameters:
  - `config_path_or_key` (const std::string &): Input parameter.
- Return: True when the operation succeeds.
- Details: config_path_or_key Input parameter. True when the operation succeeds.

#### `bool watch(const std::string &config_path_or_key)=0`
- Source: `include/core/config_hot_reloader.h`:78
- Brief: Watch.
- Parameters:
  - `config_path_or_key` (const std::string &): Input parameter.
- Return: True when the operation succeeds.
- Details: config_path_or_key Input parameter. True when the operation succeeds.

#### `~IConfigHotReloader()=default`
- Source: `include/core/config_hot_reloader.h`:71
- Brief: IConfig Hot Reloader.
- Parameters: none
- Return: Return value.
- Details: Return value.

### themis::core::IHealthProbe

#### `HealthCheckResult checkLiveness()=0`
- Source: `include/core/health_probe.h`:76
- Brief: Check Liveness.
- Parameters: none
- Return: Return value.
- Details: Return value.

#### `HealthCheckResult checkReadiness()=0`
- Source: `include/core/health_probe.h`:82
- Brief: Check Readiness.
- Parameters: none
- Return: Return value.
- Details: Return value.

#### `HealthCheckResult checkStartup()=0`
- Source: `include/core/health_probe.h`:88
- Brief: Check Startup.
- Parameters: none
- Return: Return value.
- Details: Return value.

#### `std::string componentName() const =0`
- Source: `include/core/health_probe.h`:94
- Brief: Component Name.
- Parameters: none
- Return: Return value.
- Details: Return value.

#### `~IHealthProbe()=default`
- Source: `include/core/health_probe.h`:70
- Brief: IHealth Probe.
- Parameters: none
- Return: Return value.
- Details: Return value.

### themis::core::IHealthProbeRegistry

#### `AggregateHealthReport checkAll()=0`
- Source: `include/core/health_probe.h`:127
- Brief: Check All.
- Parameters: none
- Return: Return value.
- Details: Return value.

#### `HealthCheckResult checkComponent(const std::string &component_name)=0`
- Source: `include/core/health_probe.h`:134
- Brief: Check Component.
- Parameters:
  - `component_name` (const std::string &): Name of the component.
- Return: Return value.
- Details: component_name Name of the component. Return value.

#### `bool registerProbe(std::shared_ptr< IHealthProbe > probe)=0`
- Source: `include/core/health_probe.h`:114
- Brief: Register Probe.
- Parameters:
  - `probe` (std::shared_ptr< IHealthProbe >): Input parameter.
- Return: True when the operation succeeds.
- Details: probe Input parameter. True when the operation succeeds.

#### `bool unregisterProbe(const std::string &component_name)=0`
- Source: `include/core/health_probe.h`:121
- Brief: Unregister Probe.
- Parameters:
  - `component_name` (const std::string &): Name of the component.
- Return: True when the operation succeeds.
- Details: component_name Name of the component. True when the operation succeeds.

#### `~IHealthProbeRegistry()=default`
- Source: `include/core/health_probe.h`:107
- Brief: IHealth Probe Registry.
- Parameters: none
- Return: Return value.
- Details: Return value.

### themis::core::ProductionMode

#### `void enforce(bool condition, const std::string &error_message)`
- Source: `include/core/production_mode.h`:61
- Brief: Enforce.
- Parameters:
  - `condition` (bool): Input parameter.
  - `error_message` (const std::string &): Input parameter.
- Throws:
  - std::runtime_error: if an error occurs.
- Details: condition Input parameter. error_message Input parameter. std::runtime_error if an error occurs. Calls: isEnabled().

#### `bool isEnabled()`
- Source: `include/core/production_mode.h`:28
- Brief: Is Enabled.
- Parameters: none
- Return: True when the operation succeeds.
- Details: True when the operation succeeds. Calls: std::getenv(), mode_str(), env_str().

#### `std::string modeName()`
- Source: `include/core/production_mode.h`:72
- Brief: Mode Name.
- Parameters: none
- Return: Return value.
- Details: Return value. Calls: isEnabled().

### themis::core::concerns::ARCEvictionStrategy

#### `ARCEvictionStrategy(size_t capacity=128)`
- Source: `include/core/concerns/eviction_strategies.h`:298
- Brief: n/a
- Parameters:
  - `capacity` (size_t): n/a

#### `void clear() override`
- Source: `include/core/concerns/eviction_strategies.h`:412
- Brief: Clear.
- Parameters: none

#### `std::string_view getName() const override`
- Source: `include/core/concerns/eviction_strategies.h`:423
- Brief: Get Name.
- Parameters: none
- Return: Return value.
- Details: Return value.

#### `void onAccess(std::string_view key) override`
- Source: `include/core/concerns/eviction_strategies.h`:301
- Brief: On Access.
- Parameters:
  - `key` (std::string_view): Input parameter.
- Details: key Input parameter.

#### `void onInsert(std::string_view key, uint64_t) override`
- Source: `include/core/concerns/eviction_strategies.h`:324
- Brief: On Insert.
- Parameters:
  - `key` (std::string_view): Input parameter.
  - `timestamp_ms` (uint64_t): Input parameter.
- Details: key Input parameter. timestamp_ms Input parameter.

#### `void onRemove(std::string_view key) override`
- Source: `include/core/concerns/eviction_strategies.h`:364
- Brief: On Remove.
- Parameters:
  - `key` (std::string_view): Input parameter.
- Details: key Input parameter.

#### `std::optional< std::string > selectVictim() override`
- Source: `include/core/concerns/eviction_strategies.h`:398
- Brief: Select Victim.
- Parameters: none
- Return: Return value.
- Details: Return value.

#### `size_t size() const override`
- Source: `include/core/concerns/eviction_strategies.h`:419
- Brief: Size.
- Parameters: none
- Return: Return value.
- Details: Return value.

### themis::core::concerns::AdapterRegistry

#### `AdapterRegistry()=default`
- Source: `include/core/concerns/adapter_registry.h`:46
- Brief: n/a
- Parameters: none

#### `size_t count() const`
- Source: `include/core/concerns/adapter_registry.h`:177
- Brief: Count.
- Parameters: none
- Return: Return value.
- Details: Return value.

#### `bool hasAdapter(std::type_index type) const`
- Source: `include/core/concerns/adapter_registry.h`:184
- Brief: Has Adapter.
- Parameters:
  - `type` (std::type_index): Input parameter.
- Return: True when the operation succeeds.
- Details: type Input parameter. True when the operation succeeds.

#### `bool hotSwap(std::shared_ptr< T > new_adapter, AdapterMetadata meta={})`
- Source: `include/core/concerns/adapter_registry.h`:128
- Brief: n/a
- Parameters:
  - `new_adapter` (std::shared_ptr< T >): n/a
  - `meta` (AdapterMetadata): n/a

#### `bool loadFromPlugin(const std::string &path, const std::string &adapter_id)`
- Source: `include/core/concerns/adapter_registry.h`:196
- Brief: Load From Plugin.
- Parameters:
  - `path` (const std::string &): Input parameter.
  - `adapter_id` (const std::string &): Identifier of the adapter.
- Return: True when the operation succeeds.
- Details: path Input parameter. adapter_id Identifier of the adapter. True when the operation succeeds. Calls: empty(), std::filesystem::exists(), lock(), sig_file(), is_open(), std::getline(), back(), pop_back().

#### `void registerAdapter(std::string id, std::shared_ptr< T > adapter, AdapterValidator *validator=nullptr, AdapterMetadata meta={})`
- Source: `include/core/concerns/adapter_registry.h`:60
- Brief: n/a
- Parameters:
  - `id` (std::string): n/a
  - `adapter` (std::shared_ptr< T >): n/a
  - `validator` (AdapterValidator *): n/a
  - `meta` (AdapterMetadata): n/a

#### `std::shared_ptr< T > resolve() const`
- Source: `include/core/concerns/adapter_registry.h`:109
- Brief: n/a
- Parameters: none

#### `void setTrustPolicy(AdapterTrustPolicy policy)`
- Source: `include/core/concerns/adapter_registry.h`:194
- Brief: Set Trust Policy.
- Parameters:
  - `policy` (AdapterTrustPolicy): Input parameter.
- Details: policy Input parameter. policy Input parameter. Calls: lock().

#### `~AdapterRegistry()`
- Source: `include/core/concerns/adapter_registry.h`:48
- Brief: n/a
- Parameters: none

### themis::core::concerns::AdapterRegistry::PluginHandle

#### `PluginHandle()=default`
- Source: `include/core/concerns/adapter_registry.h`:208
- Brief: n/a
- Parameters: none

#### `PluginHandle(PluginHandle &&o) noexcept`
- Source: `include/core/concerns/adapter_registry.h`:221
- Brief: n/a
- Parameters:
  - `o` (PluginHandle &&): n/a

#### `PluginHandle(const PluginHandle &)=delete`
- Source: `include/core/concerns/adapter_registry.h`:218
- Brief: n/a
- Parameters:
  - `<unnamed>` (const PluginHandle &): n/a

#### `PluginHandle(void *h, std::string p)`
- Source: `include/core/concerns/adapter_registry.h`:215
- Brief: Plugin Handle.
- Parameters:
  - `h` (void *): Input/output parameter.
  - `p` (std::string): Input parameter.
- Return: Return value.
- Details: h Input/output parameter. p Input parameter. Return value.

#### `PluginHandle & operator=(PluginHandle &&o) noexcept`
- Source: `include/core/concerns/adapter_registry.h`:225
- Brief: n/a
- Parameters:
  - `o` (PluginHandle &&): n/a

#### `PluginHandle & operator=(const PluginHandle &)=delete`
- Source: `include/core/concerns/adapter_registry.h`:219
- Brief: n/a
- Parameters:
  - `<unnamed>` (const PluginHandle &): n/a

#### `~PluginHandle()`
- Source: `include/core/concerns/adapter_registry.h`:234
- Brief: n/a
- Parameters: none

### themis::core::concerns::AdapterSignature

#### `bool present() const noexcept`
- Source: `include/core/concerns/adapter_metadata.h`:46
- Brief: n/a
- Parameters: none

### themis::core::concerns::AdapterValidator

#### `bool validate(const AdapterMetadata &m)=0`
- Source: `include/core/concerns/adapter_metadata.h`:68
- Brief: Validate.
- Parameters:
  - `m` (const AdapterMetadata &): Input parameter.
- Return: True when the operation succeeds.
- Details: m Input parameter. True when the operation succeeds.

#### `~AdapterValidator()=default`
- Source: `include/core/concerns/adapter_metadata.h`:61
- Brief: Adapter Validator.
- Parameters: none
- Return: Return value.
- Details: Return value.

### themis::core::concerns::AuditEvent

#### `AuditEvent denied(std::string_view event_type, std::string_view actor, std::string_view resource, std::string_view action, std::map< std::string, std::string > details={})`
- Source: `include/core/concerns/i_audit_log.h`:73
- Brief: n/a
- Parameters:
  - `event_type` (std::string_view): n/a
  - `actor` (std::string_view): n/a
  - `resource` (std::string_view): n/a
  - `action` (std::string_view): n/a
  - `details` (std::map< std::string, std::string >): n/a

#### `AuditEvent error(std::string_view event_type, std::string_view actor, std::string_view resource, std::string_view action, std::map< std::string, std::string > details={})`
- Source: `include/core/concerns/i_audit_log.h`:82
- Brief: n/a
- Parameters:
  - `event_type` (std::string_view): n/a
  - `actor` (std::string_view): n/a
  - `resource` (std::string_view): n/a
  - `action` (std::string_view): n/a
  - `details` (std::map< std::string, std::string >): n/a

#### `AuditEvent make(std::string_view event_type, std::string_view actor, std::string_view resource, std::string_view action, std::string_view outcome, std::map< std::string, std::string > details={})`
- Source: `include/core/concerns/i_audit_log.h`:44
- Brief: n/a
- Parameters:
  - `event_type` (std::string_view): n/a
  - `actor` (std::string_view): n/a
  - `resource` (std::string_view): n/a
  - `action` (std::string_view): n/a
  - `outcome` (std::string_view): n/a
  - `details` (std::map< std::string, std::string >): n/a

#### `AuditEvent success(std::string_view event_type, std::string_view actor, std::string_view resource, std::string_view action, std::map< std::string, std::string > details={})`
- Source: `include/core/concerns/i_audit_log.h`:64
- Brief: n/a
- Parameters:
  - `event_type` (std::string_view): n/a
  - `actor` (std::string_view): n/a
  - `resource` (std::string_view): n/a
  - `action` (std::string_view): n/a
  - `details` (std::map< std::string, std::string >): n/a

### themis::core::concerns::CacheEntry

#### `CacheEntry()=default`
- Source: `include/core/concerns/i_cache.h`:33
- Brief: n/a
- Parameters: none

#### `CacheEntry(std::string data, uint64_t ver=0, uint64_t ts=0)`
- Source: `include/core/concerns/i_cache.h`:34
- Brief: n/a
- Parameters:
  - `data` (std::string): n/a
  - `ver` (uint64_t): n/a
  - `ts` (uint64_t): n/a

### themis::core::concerns::CacheMetrics

#### `double avgLatencyNs() const`
- Source: `include/core/concerns/cache_strategies.h`:88
- Brief: n/a
- Parameters: none

#### `double hitRate() const`
- Source: `include/core/concerns/cache_strategies.h`:83
- Brief: n/a
- Parameters: none

### themis::core::concerns::ConcernsContext

#### `ConcernsContext(std::unique_ptr< ILogger > logger, std::unique_ptr< ITracer > tracer, std::unique_ptr< IMetrics > metrics, std::unique_ptr< ICache > cache, std::unique_ptr< ICircuitBreaker > circuit_breaker, std::unique_ptr< ISecrets > secrets, std::unique_ptr< IFeatureFlags > featureFlags, std::unique_ptr< IAuditLog > auditLog)`
- Source: `include/core/concerns/concerns_context.h`:477
- Brief: n/a
- Parameters:
  - `logger` (std::unique_ptr< ILogger >): n/a
  - `tracer` (std::unique_ptr< ITracer >): n/a
  - `metrics` (std::unique_ptr< IMetrics >): n/a
  - `cache` (std::unique_ptr< ICache >): n/a
  - `circuit_breaker` (std::unique_ptr< ICircuitBreaker >): n/a
  - `secrets` (std::unique_ptr< ISecrets >): n/a
  - `featureFlags` (std::unique_ptr< IFeatureFlags >): n/a
  - `auditLog` (std::unique_ptr< IAuditLog >): n/a

#### `IAuditLog & auditLog()`
- Source: `include/core/concerns/concerns_context.h`:205
- Brief: Audit Log.
- Parameters: none
- Return: Return value.
- Details: Return value. Implements auditLog without additional internal calls.

#### `const IAuditLog & auditLog() const`
- Source: `include/core/concerns/concerns_context.h`:214
- Brief: n/a
- Parameters: none

#### `ICache & cache()`
- Source: `include/core/concerns/concerns_context.h`:181
- Brief: Cache.
- Parameters: none
- Return: Return value.
- Details: Return value. Implements cache without additional internal calls.

#### `const ICache & cache() const`
- Source: `include/core/concerns/concerns_context.h`:210
- Brief: n/a
- Parameters: none

#### `ICircuitBreaker & circuitBreaker()`
- Source: `include/core/concerns/concerns_context.h`:193
- Brief: Circuit Breaker.
- Parameters: none
- Return: Return value.
- Details: Return value. Implements circuitBreaker without additional internal calls.

#### `const ICircuitBreaker & circuitBreaker() const`
- Source: `include/core/concerns/concerns_context.h`:212
- Brief: n/a
- Parameters: none

#### `std::shared_ptr< ConcernsContext > create()`
- Source: `include/core/concerns/concerns_context.h`:87
- Brief: Create.
- Parameters: none
- Return: Return value.
- Details: Return value. Return value. Implements create without additional internal calls.

#### `std::shared_ptr< ConcernsContext > create(const Config &config)`
- Source: `include/core/concerns/concerns_context.h`:94
- Brief: Create.
- Parameters:
  - `config` (const Config &): Input parameter.
- Return: Return value.
- Throws:
  - std::runtime_error: if an error occurs.
- Details: config Input parameter. Return value. config Input parameter. Return value. std::runtime_error if an error occurs. Calls: core::ProductionMode::isEnabled(), core::ConfigValidator::validateLogConfig(), formatErrors(), core::ConfigValidator::validateTracingConfig(), core::ConfigValidator::validateCacheConfig(), core::ConfigValidator::validateAdapterConfig(), ILogger::levelFromString(), utils::Logger::init().

#### `std::shared_ptr< ConcernsContext > createCustom(std::unique_ptr< ILogger > logger, std::unique_ptr< ITracer > tracer, std::unique_ptr< IMetrics > metrics, std::unique_ptr< ICache > cache, std::unique_ptr< ICircuitBreaker > circuit_breaker=nullptr)`
- Source: `include/core/concerns/concerns_context.h`:96
- Brief: Create Custom.
- Parameters:
  - `logger` (std::unique_ptr< ILogger >): Input parameter.
  - `tracer` (std::unique_ptr< ITracer >): Input parameter.
  - `metrics` (std::unique_ptr< IMetrics >): Input parameter.
  - `cache` (std::unique_ptr< ICache >): Input parameter.
  - `circuit_breaker` (std::unique_ptr< ICircuitBreaker >): Input parameter.
- Return: Return value.
- Details: logger Input parameter. tracer Input parameter. metrics Input parameter. cache Input parameter. circuit_breaker Input parameter. Return value. Calls: ConcernsContext(), std::move().

#### `std::shared_ptr< ConcernsContext > createCustom(std::unique_ptr< ILogger > logger, std::unique_ptr< ITracer > tracer, std::unique_ptr< IMetrics > metrics, std::unique_ptr< ICache > cache, std::unique_ptr< IFeatureFlags > featureFlags)`
- Source: `include/core/concerns/concerns_context.h`:122
- Brief: Create Custom.
- Parameters:
  - `logger` (std::unique_ptr< ILogger >): Input parameter.
  - `tracer` (std::unique_ptr< ITracer >): Input parameter.
  - `metrics` (std::unique_ptr< IMetrics >): Input parameter.
  - `cache` (std::unique_ptr< ICache >): Input parameter.
  - `featureFlags` (std::unique_ptr< IFeatureFlags >): Input parameter.
- Return: Return value.
- Details: logger Input parameter. tracer Input parameter. metrics Input parameter. cache Input parameter. featureFlags Input parameter. Return value. logger Input parameter. tracer Input parameter. metrics Input parameter. cache Input parameter. featureFlags Input parameter. Return value. Calls: ConcernsContext(), std::move().

#### `std::shared_ptr< ConcernsContext > createCustom(std::unique_ptr< ILogger > logger, std::unique_ptr< ITracer > tracer, std::unique_ptr< IMetrics > metrics, std::unique_ptr< ICache > cache, std::unique_ptr< ISecrets > secrets, std::unique_ptr< IFeatureFlags > featureFlags, std::unique_ptr< IAuditLog > auditLog)`
- Source: `include/core/concerns/concerns_context.h`:141
- Brief: Create Custom.
- Parameters:
  - `logger` (std::unique_ptr< ILogger >): Input parameter.
  - `tracer` (std::unique_ptr< ITracer >): Input parameter.
  - `metrics` (std::unique_ptr< IMetrics >): Input parameter.
  - `cache` (std::unique_ptr< ICache >): Input parameter.
  - `secrets` (std::unique_ptr< ISecrets >): Input parameter.
  - `featureFlags` (std::unique_ptr< IFeatureFlags >): Input parameter.
  - `auditLog` (std::unique_ptr< IAuditLog >): Input parameter.
- Return: Return value.
- Details: logger Input parameter. tracer Input parameter. metrics Input parameter. cache Input parameter. secrets Input parameter. featureFlags Input parameter. auditLog Input parameter. Return value. logger Input parameter. tracer Input parameter. metrics Input parameter. cache Input parameter. secrets Input parameter. featureFlags Input parameter. auditLog Input parameter. Return value. Calls: ConcernsContext(), std::move().

#### `std::shared_ptr< ConcernsContext > createCustom(std::unique_ptr< ILogger > logger, std::unique_ptr< ITracer > tracer, std::unique_ptr< IMetrics > metrics, std::unique_ptr< ICache > cache, std::unique_ptr< ISecrets > secrets, std::unique_ptr< IFeatureFlags > featureFlags=nullptr)`
- Source: `include/core/concerns/concerns_context.h`:104
- Brief: Create Custom.
- Parameters:
  - `logger` (std::unique_ptr< ILogger >): Input parameter.
  - `tracer` (std::unique_ptr< ITracer >): Input parameter.
  - `metrics` (std::unique_ptr< IMetrics >): Input parameter.
  - `cache` (std::unique_ptr< ICache >): Input parameter.
  - `secrets` (std::unique_ptr< ISecrets >): Input parameter.
  - `featureFlags` (std::unique_ptr< IFeatureFlags >): Input parameter.
- Return: Return value.
- Details: logger Input parameter. tracer Input parameter. metrics Input parameter. cache Input parameter. secrets Input parameter. featureFlags Input parameter. Return value. Calls: ConcernsContext(), std::move().

#### `std::shared_ptr< ConcernsContext > createNoOp()`
- Source: `include/core/concerns/concerns_context.h`:155
- Brief: Create No Op.
- Parameters: none
- Return: Return value.
- Throws:
  - std::runtime_error: if an error occurs.
- Details: Return value. Return value. std::runtime_error if an error occurs. Calls: core::ProductionMode::isEnabled(), ConcernsContext().

#### `IFeatureFlags & featureFlags()`
- Source: `include/core/concerns/concerns_context.h`:199
- Brief: Feature Flags.
- Parameters: none
- Return: Return value.
- Details: Return value. Implements featureFlags without additional internal calls.

#### `const IFeatureFlags & featureFlags() const`
- Source: `include/core/concerns/concerns_context.h`:213
- Brief: n/a
- Parameters: none

#### `void flush()`
- Source: `include/core/concerns/concerns_context.h`:420
- Brief: Flush.
- Parameters: none
- Details: Implements flush without additional internal calls.

#### `ILogger::Level getLogLevel() const`
- Source: `include/core/concerns/concerns_context.h`:332
- Brief: n/a
- Parameters: none

#### `HealthStatus healthCheck() const`
- Source: `include/core/concerns/concerns_context.h`:456
- Brief: n/a
- Parameters: none

#### `void injectContext(std::map< std::string, std::string > &headers)`
- Source: `include/core/concerns/concerns_context.h`:394
- Brief: n/a
- Parameters:
  - `headers` (std::map< std::string, std::string > &): n/a

#### `void logDebug(const std::string &message)`
- Source: `include/core/concerns/concerns_context.h`:323
- Brief: Log Debug.
- Parameters:
  - `message` (const std::string &): Input parameter.
- Details: message Input parameter. Calls: debug().

#### `void logError(const std::string &message)`
- Source: `include/core/concerns/concerns_context.h`:311
- Brief: Log Error.
- Parameters:
  - `message` (const std::string &): Input parameter.
- Details: message Input parameter. Calls: error().

#### `void logInfo(const std::string &message)`
- Source: `include/core/concerns/concerns_context.h`:305
- Brief: Convenience methods for common operations.
- Parameters:
  - `message` (const std::string &): Input parameter.
- Details: message Input parameter. Calls: info().

#### `void logWarn(const std::string &message)`
- Source: `include/core/concerns/concerns_context.h`:317
- Brief: Log Warn.
- Parameters:
  - `message` (const std::string &): Input parameter.
- Details: message Input parameter. Calls: warn().

#### `void logWithTrace(ILogger::Level level, const std::string &message, const ILogger::Fields &fields={})`
- Source: `include/core/concerns/concerns_context.h`:408
- Brief: Log With Trace.
- Parameters:
  - `level` (ILogger::Level): Input parameter.
  - `message` (const std::string &): Input parameter.
  - `fields` (const ILogger::Fields &): Input parameter.
- Details: level Input parameter. message Input parameter. fields Input parameter. Calls: themis::Tracer::getCurrentTraceId(), themis::Tracer::getCurrentSpanId(), logWithContext().

#### `ILogger & logger()`
- Source: `include/core/concerns/concerns_context.h`:163
- Brief: Logger.
- Parameters: none
- Return: Return value.
- Details: Return value. Implements logger without additional internal calls.

#### `const ILogger & logger() const`
- Source: `include/core/concerns/concerns_context.h`:207
- Brief: n/a
- Parameters: none

#### `IMetrics & metrics()`
- Source: `include/core/concerns/concerns_context.h`:175
- Brief: Metrics.
- Parameters: none
- Return: Return value.
- Details: Return value. Implements metrics without additional internal calls.

#### `const IMetrics & metrics() const`
- Source: `include/core/concerns/concerns_context.h`:209
- Brief: n/a
- Parameters: none

#### `HealthStatus readinessCheck() const`
- Source: `include/core/concerns/concerns_context.h`:469
- Brief: n/a
- Parameters: none

#### `void recordMetric(const std::string &name, double value)`
- Source: `include/core/concerns/concerns_context.h`:404
- Brief: Record Metric.
- Parameters:
  - `name` (const std::string &): Input parameter.
  - `value` (double): Input parameter.
- Details: name Input parameter. value Input parameter. Calls: observeHistogram().

#### `AdapterRegistry & registry()`
- Source: `include/core/concerns/concerns_context.h`:296
- Brief: Registry.
- Parameters: none
- Return: Return value.
- Details: Return value. Implements registry without additional internal calls.

#### `const AdapterRegistry & registry() const`
- Source: `include/core/concerns/concerns_context.h`:298
- Brief: n/a
- Parameters: none

#### `void replaceAuditLog(std::unique_ptr< IAuditLog > new_audit)`
- Source: `include/core/concerns/concerns_context.h`:376
- Brief: Replace Audit Log.
- Parameters:
  - `new_audit` (std::unique_ptr< IAuditLog >): Input parameter.
- Throws:
  - std::invalid_argument: if an error occurs.
- Details: new_audit Input parameter. new_audit Input parameter. std::invalid_argument if an error occurs. Calls: std::move(), lk(), drainAdapter().

#### `void replaceCache(std::unique_ptr< ICache > new_cache)`
- Source: `include/core/concerns/concerns_context.h`:358
- Brief: Replace Cache.
- Parameters:
  - `new_cache` (std::unique_ptr< ICache >): Input parameter.
- Throws:
  - std::invalid_argument: if an error occurs.
- Details: new_cache Input parameter. new_cache Input parameter. std::invalid_argument if an error occurs. Calls: std::move(), lk(), drainAdapter().

#### `void replaceFeatureFlags(std::unique_ptr< IFeatureFlags > new_ff)`
- Source: `include/core/concerns/concerns_context.h`:370
- Brief: Replace Feature Flags.
- Parameters:
  - `new_ff` (std::unique_ptr< IFeatureFlags >): Input parameter.
- Throws:
  - std::invalid_argument: if an error occurs.
- Details: new_ff Input parameter. new_ff Input parameter. std::invalid_argument if an error occurs. Calls: std::move(), lk(), drainAdapter().

#### `void replaceLogger(std::unique_ptr< ILogger > new_logger)`
- Source: `include/core/concerns/concerns_context.h`:340
- Brief: ---------------------------------------------------------------------- Dynamic Adapter Reconfiguration (Issue #1412 / core/FUTURE_ENHANCEMENTS.
- Parameters:
  - `new_logger` (std::unique_ptr< ILogger >): Input parameter.
- Throws:
  - std::invalid_argument: if an error occurs.
- Details: Replace Logger. new_logger Input parameter. md) These methods replace an active concern adapter at runtime without restarting the database process. The old adapter is flushed before the swap so that no buffered data is lost. All replace* calls are thread-safe for callers that resolve adapters through resolve<T>() shared_ptr snapshots: a brief exclusive lock is taken only to swap the pointer; in-flight calls on the old adapter complete while those snapshots remain alive. Passing nullptr is rejected (throws std::invalid_argument). ---------------------------------------------------------------------- new_logger Input parameter. std::invalid_argument if an error occurs. Calls: std::move(), lk(), drainAdapter().

#### `void replaceMetrics(std::unique_ptr< IMetrics > new_metrics)`
- Source: `include/core/concerns/concerns_context.h`:352
- Brief: Replace Metrics.
- Parameters:
  - `new_metrics` (std::unique_ptr< IMetrics >): Input parameter.
- Throws:
  - std::invalid_argument: if an error occurs.
- Details: new_metrics Input parameter. new_metrics Input parameter. std::invalid_argument if an error occurs. Calls: std::move(), lk(), drainAdapter().

#### `void replaceSecrets(std::unique_ptr< ISecrets > new_secrets)`
- Source: `include/core/concerns/concerns_context.h`:364
- Brief: Replace Secrets.
- Parameters:
  - `new_secrets` (std::unique_ptr< ISecrets >): Input parameter.
- Throws:
  - std::invalid_argument: if an error occurs.
- Details: new_secrets Input parameter. new_secrets Input parameter. std::invalid_argument if an error occurs. Calls: std::move(), lk(), drainAdapter().

#### `void replaceTracer(std::unique_ptr< ITracer > new_tracer)`
- Source: `include/core/concerns/concerns_context.h`:346
- Brief: Replace Tracer.
- Parameters:
  - `new_tracer` (std::unique_ptr< ITracer >): Input parameter.
- Throws:
  - std::invalid_argument: if an error occurs.
- Details: new_tracer Input parameter. new_tracer Input parameter. std::invalid_argument if an error occurs. Calls: std::move(), lk(), drainAdapter().

#### `std::shared_ptr< T > resolve() const`
- Source: `include/core/concerns/concerns_context.h`:221
- Brief: n/a
- Parameters: none

#### `ISecrets & secrets()`
- Source: `include/core/concerns/concerns_context.h`:187
- Brief: Secrets.
- Parameters: none
- Return: Return value.
- Details: Return value. Implements secrets without additional internal calls.

#### `const ISecrets & secrets() const`
- Source: `include/core/concerns/concerns_context.h`:211
- Brief: n/a
- Parameters: none

#### `void setLogLevel(ILogger::Level level)`
- Source: `include/core/concerns/concerns_context.h`:330
- Brief: Set Log Level.
- Parameters:
  - `level` (ILogger::Level): Input parameter.
- Details: level Input parameter. Calls: setLevel().

#### `void shutdown()`
- Source: `include/core/concerns/concerns_context.h`:435
- Brief: Shutdown.
- Parameters: none
- Details: Calls: flush().

#### `std::unique_ptr< ITracer::ISpan > startSpan(const std::string &name)`
- Source: `include/core/concerns/concerns_context.h`:384
- Brief: Start Span.
- Parameters:
  - `name` (const std::string &): Input parameter.
- Return: Return value.
- Details: name Input parameter. Return value. Implements startSpan without additional internal calls.

#### `std::unique_ptr< ITracer::ISpan > startSpanFromHeaders(const std::string &name, const std::map< std::string, std::string > &headers)`
- Source: `include/core/concerns/concerns_context.h`:388
- Brief: n/a
- Parameters:
  - `name` (const std::string &): n/a
  - `headers` (const std::map< std::string, std::string > &): n/a

#### `ITracer & tracer()`
- Source: `include/core/concerns/concerns_context.h`:169
- Brief: Tracer.
- Parameters: none
- Return: Return value.
- Details: Return value. Implements tracer without additional internal calls.

#### `const ITracer & tracer() const`
- Source: `include/core/concerns/concerns_context.h`:208
- Brief: n/a
- Parameters: none

### themis::core::concerns::ContextPropagation

#### `ContextPropagation()=delete`
- Source: `include/core/concerns/context_propagation.h`:40
- Brief: n/a
- Parameters: none

#### `IContextPtr current() noexcept`
- Source: `include/core/concerns/context_propagation.h`:27
- Brief: n/a
- Parameters: none

#### `std::future< std::invoke_result_t< Fn > > propagate(Fn &&fn)`
- Source: `include/core/concerns/context_propagation.h`:37
- Brief: Propagate.
- Parameters:
  - `fn` (Fn &&): Input parameter.
- Return: Return value.
- Details: fn Input parameter. Return value.

### themis::core::concerns::ContextScope

#### `ContextScope(ContextScope &&)=delete`
- Source: `include/core/concerns/context_propagation.h`:68
- Brief: n/a
- Parameters:
  - `<unnamed>` (ContextScope &&): n/a

#### `ContextScope(IContextPtr ctx) noexcept`
- Source: `include/core/concerns/context_propagation.h`:55
- Brief: n/a
- Parameters:
  - `ctx` (IContextPtr): n/a

#### `ContextScope(const ContextScope &)=delete`
- Source: `include/core/concerns/context_propagation.h`:66
- Brief: n/a
- Parameters:
  - `<unnamed>` (const ContextScope &): n/a

#### `ContextScope & operator=(ContextScope &&)=delete`
- Source: `include/core/concerns/context_propagation.h`:69
- Brief: n/a
- Parameters:
  - `<unnamed>` (ContextScope &&): n/a

#### `ContextScope & operator=(const ContextScope &)=delete`
- Source: `include/core/concerns/context_propagation.h`:67
- Brief: n/a
- Parameters:
  - `<unnamed>` (const ContextScope &): n/a

#### `~ContextScope() noexcept`
- Source: `include/core/concerns/context_propagation.h`:61
- Brief: n/a
- Parameters: none

### themis::core::concerns::DefaultCircuitBreaker

#### `DefaultCircuitBreaker(const Config &config=Config{})`
- Source: `include/core/concerns/i_circuit_breaker.h`:138
- Brief: n/a
- Parameters:
  - `config` (const Config &): n/a

#### `bool allowRequest() override`
- Source: `include/core/concerns/i_circuit_breaker.h`:147
- Brief: n/a
- Parameters: none

#### `void forceOpen() override`
- Source: `include/core/concerns/i_circuit_breaker.h`:153
- Brief: Force Open.
- Parameters: none

#### `size_t getFailureCount() const override`
- Source: `include/core/concerns/i_circuit_breaker.h`:150
- Brief: n/a
- Parameters: none

#### `State getState() const override`
- Source: `include/core/concerns/i_circuit_breaker.h`:155
- Brief: n/a
- Parameters: none

#### `size_t getSuccessCount() const override`
- Source: `include/core/concerns/i_circuit_breaker.h`:151
- Brief: n/a
- Parameters: none

#### `void recordFailure() override`
- Source: `include/core/concerns/i_circuit_breaker.h`:149
- Brief: Record Failure.
- Parameters: none

#### `void recordSuccess() override`
- Source: `include/core/concerns/i_circuit_breaker.h`:148
- Brief: Record Success.
- Parameters: none

#### `void reset() override`
- Source: `include/core/concerns/i_circuit_breaker.h`:152
- Brief: Reset the modification detection flag.
- Parameters: none

### themis::core::concerns::EnvSecretsProvider

#### `EnvSecretsProvider(std::string prefix="THEMIS_SECRET_")`
- Source: `include/core/concerns/inmemory_secrets.h`:126
- Brief: n/a
- Parameters:
  - `prefix` (std::string): n/a

#### `std::string envKeyFor(std::string_view name) const`
- Source: `include/core/concerns/inmemory_secrets.h`:181
- Brief: n/a
- Parameters:
  - `name` (std::string_view): n/a

#### `void flush() noexcept override`
- Source: `include/core/concerns/inmemory_secrets.h`:166
- Brief: n/a
- Parameters: none

#### `std::optional< std::string > getSecret(std::string_view name) const override`
- Source: `include/core/concerns/inmemory_secrets.h`:133
- Brief: n/a
- Parameters:
  - `name` (std::string_view): n/a

#### `bool hasSecret(std::string_view name) const override`
- Source: `include/core/concerns/inmemory_secrets.h`:142
- Brief: n/a
- Parameters:
  - `name` (std::string_view): n/a

#### `ProbeResult isHealthy() const override`
- Source: `include/core/concerns/inmemory_secrets.h`:168
- Brief: n/a
- Parameters: none

#### `std::vector< std::string > listSecretNames() const override`
- Source: `include/core/concerns/inmemory_secrets.h`:147
- Brief: n/a
- Parameters: none

#### `void registerName(std::string_view name)`
- Source: `include/core/concerns/inmemory_secrets.h`:176
- Brief: -------------------------------------------------------------------- Registration (not part of ISecrets) --------------------------------------------------------------------
- Parameters:
  - `name` (std::string_view): Input parameter.
- Details: name Input parameter. Calls: lock(), push_back(), std::string().

#### `void shutdown() noexcept override`
- Source: `include/core/concerns/inmemory_secrets.h`:167
- Brief: n/a
- Parameters: none

#### `std::string toEnvKey(std::string_view name) const`
- Source: `include/core/concerns/inmemory_secrets.h`:186
- Brief: n/a
- Parameters:
  - `name` (std::string_view): n/a

### themis::core::concerns::HealthStatus

#### `bool isHealthy() const noexcept`
- Source: `include/core/concerns/lifecycle.h`:48
- Brief: n/a
- Parameters: none

### themis::core::concerns::IAsyncCache

#### `std::future< std::optional< CacheEntry > > getAsync(std::string_view key)`
- Source: `include/core/concerns/i_async_cache.h`:34
- Brief: Get Async.
- Parameters:
  - `key` (std::string_view): Input parameter.
- Return: Return value.
- Details: key Input parameter. Return value. Calls: std::async(), std::string(), get().

#### `std::future< void > invalidateAsync(std::string_view key)`
- Source: `include/core/concerns/i_async_cache.h`:57
- Brief: Invalidate Async.
- Parameters:
  - `key` (std::string_view): Input parameter.
- Return: Return value.
- Details: key Input parameter. Return value. Calls: std::async(), std::string(), invalidate().

#### `std::future< bool > putAsync(std::string_view key, const CacheEntry &entry, uint64_t ttl_ms=0)`
- Source: `include/core/concerns/i_async_cache.h`:41
- Brief: n/a
- Parameters:
  - `key` (std::string_view): n/a
  - `entry` (const CacheEntry &): n/a
  - `ttl_ms` (uint64_t): n/a

### themis::core::concerns::IAsyncLogger

#### `std::future< void > criticalAsync(std::string_view message)`
- Source: `include/core/concerns/i_async_logger.h`:98
- Brief: Critical Async.
- Parameters:
  - `message` (std::string_view): Input parameter.
- Return: Return value.
- Details: message Input parameter. Return value. Calls: logAsync().

#### `std::future< void > debugAsync(std::string_view message)`
- Source: `include/core/concerns/i_async_logger.h`:58
- Brief: Debug Async.
- Parameters:
  - `message` (std::string_view): Input parameter.
- Return: Return value.
- Details: message Input parameter. Return value. Calls: logAsync().

#### `std::future< void > errorAsync(std::string_view message)`
- Source: `include/core/concerns/i_async_logger.h`:88
- Brief: Error Async.
- Parameters:
  - `message` (std::string_view): Input parameter.
- Return: Return value.
- Details: message Input parameter. Return value. Calls: logAsync().

#### `std::future< void > infoAsync(std::string_view message)`
- Source: `include/core/concerns/i_async_logger.h`:68
- Brief: Info Async.
- Parameters:
  - `message` (std::string_view): Input parameter.
- Return: Return value.
- Details: message Input parameter. Return value. Calls: logAsync().

#### `std::future< void > logAsync(Level level, std::string_view message)`
- Source: `include/core/concerns/i_async_logger.h`:35
- Brief: Log Async.
- Parameters:
  - `level` (Level): Input parameter.
  - `message` (std::string_view): Input parameter.
- Return: Return value.
- Details: level Input parameter. message Input parameter. Return value. Calls: std::async(), std::string(), log().

#### `std::future< void > logStructuredAsync(Level level, std::string_view message, const Fields &fields={})`
- Source: `include/core/concerns/i_async_logger.h`:102
- Brief: n/a
- Parameters:
  - `level` (Level): n/a
  - `message` (std::string_view): n/a
  - `fields` (const Fields &): n/a

#### `std::future< void > traceAsync(std::string_view message)`
- Source: `include/core/concerns/i_async_logger.h`:48
- Brief: Trace Async.
- Parameters:
  - `message` (std::string_view): Input parameter.
- Return: Return value.
- Details: message Input parameter. Return value. Calls: logAsync().

#### `std::future< void > warnAsync(std::string_view message)`
- Source: `include/core/concerns/i_async_logger.h`:78
- Brief: Warn Async.
- Parameters:
  - `message` (std::string_view): Input parameter.
- Return: Return value.
- Details: message Input parameter. Return value. Calls: logAsync().

### themis::core::concerns::IAuditLog

#### `void flush() noexcept`
- Source: `include/core/concerns/i_audit_log.h`:111
- Brief: n/a
- Parameters: none

#### `ProbeResult isHealthy() const`
- Source: `include/core/concerns/i_audit_log.h`:115
- Brief: n/a
- Parameters: none

#### `void record(const AuditEvent &event) noexcept=0`
- Source: `include/core/concerns/i_audit_log.h`:105
- Brief: Record.
- Parameters:
  - `event` (const AuditEvent &): Input parameter.
- Details: event Input parameter. Exception safety: noexcept.

#### `void shutdown() noexcept`
- Source: `include/core/concerns/i_audit_log.h`:113
- Brief: n/a
- Parameters: none

#### `~IAuditLog()=default`
- Source: `include/core/concerns/i_audit_log.h`:98
- Brief: IAudit Log.
- Parameters: none
- Return: Return value.
- Details: Return value.

### themis::core::concerns::ICache

#### `void clear()=0`
- Source: `include/core/concerns/i_cache.h`:63
- Brief: Clear.
- Parameters: none

#### `void flush() noexcept`
- Source: `include/core/concerns/i_cache.h`:119
- Brief: n/a
- Parameters: none

#### `std::optional< CacheEntry > get(std::string_view key) const =0`
- Source: `include/core/concerns/i_cache.h`:50
- Brief: n/a
- Parameters:
  - `key` (std::string_view): n/a

#### `IEvictionStrategy * getEvictionStrategy()`
- Source: `include/core/concerns/i_cache.h`:112
- Brief: Get Eviction Strategy.
- Parameters: none
- Return: Pointer to the result.
- Details: Pointer to the result. Implements getEvictionStrategy without additional internal calls.

#### `const IEvictionStrategy * getEvictionStrategy() const`
- Source: `include/core/concerns/i_cache.h`:114
- Brief: n/a
- Parameters: none

#### `const CacheMetrics * getMetrics() const`
- Source: `include/core/concerns/i_cache.h`:116
- Brief: n/a
- Parameters: none

#### `uint64_t hitCount() const =0`
- Source: `include/core/concerns/i_cache.h`:81
- Brief: n/a
- Parameters: none

#### `double hitRate() const =0`
- Source: `include/core/concerns/i_cache.h`:85
- Brief: n/a
- Parameters: none

#### `void invalidate(std::string_view key)=0`
- Source: `include/core/concerns/i_cache.h`:58
- Brief: Invalidate.
- Parameters:
  - `key` (std::string_view): Input parameter.
- Details: key Input parameter.

#### `void invalidatePattern(std::string_view pattern)=0`
- Source: `include/core/concerns/i_cache.h`:73
- Brief: Invalidate Pattern.
- Parameters:
  - `pattern` (std::string_view): Input parameter.
- Details: pattern Input parameter.

#### `ProbeResult isHealthy() const`
- Source: `include/core/concerns/i_cache.h`:123
- Brief: n/a
- Parameters: none

#### `uint64_t missCount() const =0`
- Source: `include/core/concerns/i_cache.h`:83
- Brief: n/a
- Parameters: none

#### `bool put(std::string_view key, const CacheEntry &entry, uint64_t ttl_ms=0)=0`
- Source: `include/core/concerns/i_cache.h`:52
- Brief: n/a
- Parameters:
  - `key` (std::string_view): n/a
  - `entry` (const CacheEntry &): n/a
  - `ttl_ms` (uint64_t): n/a

#### `void setDefaultTTL(uint64_t ttl_ms)=0`
- Source: `include/core/concerns/i_cache.h`:101
- Brief: Set Default TTL.
- Parameters:
  - `ttl_ms` (uint64_t): Input parameter.
- Details: ttl_ms Input parameter.

#### `void setMaxSize(size_t maxSize)=0`
- Source: `include/core/concerns/i_cache.h`:95
- Brief: Set Max Size.
- Parameters:
  - `maxSize` (size_t): Input parameter.
- Details: maxSize Input parameter.

#### `void shutdown() noexcept`
- Source: `include/core/concerns/i_cache.h`:121
- Brief: n/a
- Parameters: none

#### `size_t size() const =0`
- Source: `include/core/concerns/i_cache.h`:79
- Brief: n/a
- Parameters: none

#### `~ICache()=default`
- Source: `include/core/concerns/i_cache.h`:44
- Brief: ICache.
- Parameters: none
- Return: Return value.
- Details: Return value.

### themis::core::concerns::ICircuitBreaker

#### `bool allowRequest()=0`
- Source: `include/core/concerns/i_circuit_breaker.h`:49
- Brief: n/a
- Parameters: none

#### `decltype(fn()) call(Fn &&fn, Fallback &&fallback)`
- Source: `include/core/concerns/i_circuit_breaker.h`:82
- Brief: n/a
- Parameters:
  - `fn` (Fn &&): n/a
  - `fallback` (Fallback &&): n/a

#### `void flush() noexcept`
- Source: `include/core/concerns/i_circuit_breaker.h`:125
- Brief: n/a
- Parameters: none

#### `void forceOpen()=0`
- Source: `include/core/concerns/i_circuit_breaker.h`:75
- Brief: Force Open.
- Parameters: none

#### `size_t getFailureCount() const =0`
- Source: `include/core/concerns/i_circuit_breaker.h`:63
- Brief: n/a
- Parameters: none

#### `State getState() const =0`
- Source: `include/core/concerns/i_circuit_breaker.h`:61
- Brief: n/a
- Parameters: none

#### `size_t getSuccessCount() const =0`
- Source: `include/core/concerns/i_circuit_breaker.h`:65
- Brief: n/a
- Parameters: none

#### `ProbeResult isHealthy() const`
- Source: `include/core/concerns/i_circuit_breaker.h`:129
- Brief: n/a
- Parameters: none

#### `void recordFailure()=0`
- Source: `include/core/concerns/i_circuit_breaker.h`:59
- Brief: Record Failure.
- Parameters: none

#### `void recordSuccess()=0`
- Source: `include/core/concerns/i_circuit_breaker.h`:54
- Brief: Record Success.
- Parameters: none

#### `void reset()=0`
- Source: `include/core/concerns/i_circuit_breaker.h`:70
- Brief: Reset the modification detection flag.
- Parameters: none

#### `void shutdown() noexcept`
- Source: `include/core/concerns/i_circuit_breaker.h`:127
- Brief: n/a
- Parameters: none

#### `std::string stateToString(State state)`
- Source: `include/core/concerns/i_circuit_breaker.h`:112
- Brief: State To String.
- Parameters:
  - `state` (State): Input parameter.
- Return: Return value.
- Details: state Input parameter. Return value. Implements stateToString without additional internal calls.

#### `~ICircuitBreaker()=default`
- Source: `include/core/concerns/i_circuit_breaker.h`:43
- Brief: ICircuit Breaker.
- Parameters: none
- Return: Return value.
- Details: Return value.

### themis::core::concerns::IContext

#### `IContextPtr createChild() const =0`
- Source: `include/core/concerns/i_context.h`:55
- Brief: n/a
- Parameters: none

#### `std::optional< std::string > get(std::string_view key) const =0`
- Source: `include/core/concerns/i_context.h`:47
- Brief: n/a
- Parameters:
  - `key` (std::string_view): n/a

#### `bool has(std::string_view key) const =0`
- Source: `include/core/concerns/i_context.h`:49
- Brief: n/a
- Parameters:
  - `key` (std::string_view): n/a

#### `void set(std::string_view key, std::string_view value)=0`
- Source: `include/core/concerns/i_context.h`:45
- Brief: Set.
- Parameters:
  - `key` (std::string_view): Input parameter.
  - `value` (std::string_view): Input parameter.
- Details: key Input parameter. value Input parameter.

#### `TraceContext toTraceContext() const =0`
- Source: `include/core/concerns/i_context.h`:61
- Brief: n/a
- Parameters: none

#### `~IContext()=default`
- Source: `include/core/concerns/i_context.h`:34
- Brief: IContext.
- Parameters: none
- Return: Return value.
- Details: Return value.

### themis::core::concerns::IEvictionStrategy

#### `void clear()=0`
- Source: `include/core/concerns/cache_strategies.h`:59
- Brief: Clear.
- Parameters: none

#### `std::string_view getName() const =0`
- Source: `include/core/concerns/cache_strategies.h`:71
- Brief: Get Name.
- Parameters: none
- Return: Return value.
- Details: Return value.

#### `void onAccess(std::string_view key)=0`
- Source: `include/core/concerns/cache_strategies.h`:35
- Brief: On Access.
- Parameters:
  - `key` (std::string_view): Input parameter.
- Details: key Input parameter.

#### `void onInsert(std::string_view key, uint64_t timestamp_ms)=0`
- Source: `include/core/concerns/cache_strategies.h`:42
- Brief: On Insert.
- Parameters:
  - `key` (std::string_view): Input parameter.
  - `timestamp_ms` (uint64_t): Input parameter.
- Details: key Input parameter. timestamp_ms Input parameter.

#### `void onRemove(std::string_view key)=0`
- Source: `include/core/concerns/cache_strategies.h`:48
- Brief: On Remove.
- Parameters:
  - `key` (std::string_view): Input parameter.
- Details: key Input parameter.

#### `std::optional< std::string > selectVictim()=0`
- Source: `include/core/concerns/cache_strategies.h`:54
- Brief: Select Victim.
- Parameters: none
- Return: Return value.
- Details: Return value.

#### `size_t size() const =0`
- Source: `include/core/concerns/cache_strategies.h`:65
- Brief: Size.
- Parameters: none
- Return: Return value.
- Details: Return value.

#### `~IEvictionStrategy()=default`
- Source: `include/core/concerns/cache_strategies.h`:29
- Brief: IEviction Strategy.
- Parameters: none
- Return: Return value.
- Details: Return value.

### themis::core::concerns::IFeatureFlags

#### `void flush() noexcept`
- Source: `include/core/concerns/i_feature_flags.h`:60
- Brief: n/a
- Parameters: none

#### `std::unordered_map< std::string, bool > getAllFlags() const =0`
- Source: `include/core/concerns/i_feature_flags.h`:54
- Brief: n/a
- Parameters: none

#### `bool isEnabled(std::string_view name) const =0`
- Source: `include/core/concerns/i_feature_flags.h`:37
- Brief: n/a
- Parameters:
  - `name` (std::string_view): n/a

#### `ProbeResult isHealthy() const`
- Source: `include/core/concerns/i_feature_flags.h`:64
- Brief: n/a
- Parameters: none

#### `void setValue(std::string_view name, bool value)=0`
- Source: `include/core/concerns/i_feature_flags.h`:48
- Brief: Set Value.
- Parameters:
  - `name` (std::string_view): Input parameter.
  - `value` (bool): Input parameter.
- Details: name Input parameter. value Input parameter.

#### `void shutdown() noexcept`
- Source: `include/core/concerns/i_feature_flags.h`:62
- Brief: n/a
- Parameters: none

#### `~IFeatureFlags()=default`
- Source: `include/core/concerns/i_feature_flags.h`:31
- Brief: IFeature Flags.
- Parameters: none
- Return: Return value.
- Details: Return value.

### themis::core::concerns::ILogger

#### `void critical(const std::string &message)=0`
- Source: `include/core/concerns/i_logger.h`:101
- Brief: Critical.
- Parameters:
  - `message` (const std::string &): Input parameter.
- Details: message Input parameter.

#### `void debug(const std::string &message)=0`
- Source: `include/core/concerns/i_logger.h`:77
- Brief: Debug.
- Parameters:
  - `message` (const std::string &): Input parameter.
- Details: message Input parameter.

#### `void error(const std::string &message)=0`
- Source: `include/core/concerns/i_logger.h`:95
- Brief: Error.
- Parameters:
  - `message` (const std::string &): Input parameter.
- Details: message Input parameter.

#### `void flush() noexcept`
- Source: `include/core/concerns/i_logger.h`:153
- Brief: n/a
- Parameters: none

#### `Level getLevel() const =0`
- Source: `include/core/concerns/i_logger.h`:144
- Brief: n/a
- Parameters: none

#### `void info(const std::string &message)=0`
- Source: `include/core/concerns/i_logger.h`:83
- Brief: Info.
- Parameters:
  - `message` (const std::string &): Input parameter.
- Details: message Input parameter.

#### `ProbeResult isHealthy() const`
- Source: `include/core/concerns/i_logger.h`:157
- Brief: n/a
- Parameters: none

#### `Level levelFromString(const std::string &level)`
- Source: `include/core/concerns/i_logger.h`:168
- Brief: Level From String.
- Parameters:
  - `level` (const std::string &): Input parameter.
- Return: Return value.
- Details: level Input parameter. Return value. level Input parameter. Return value. Calls: std::transform(), begin(), end(), std::tolower().

#### `const char * levelToString(Level level)`
- Source: `include/core/concerns/i_logger.h`:175
- Brief: Level To String.
- Parameters:
  - `level` (Level): Input parameter.
- Return: Pointer to the result.
- Details: level Input parameter. Pointer to the result.

#### `void log(Level level, const std::string &message)=0`
- Source: `include/core/concerns/i_logger.h`:65
- Brief: Log.
- Parameters:
  - `level` (Level): Input parameter.
  - `message` (const std::string &): Input parameter.
- Details: level Input parameter. message Input parameter.

#### `void logStructured(Level level, const std::string &message, const Fields &fields={})`
- Source: `include/core/concerns/i_logger.h`:103
- Brief: n/a
- Parameters:
  - `level` (Level): n/a
  - `message` (const std::string &): n/a
  - `fields` (const Fields &): n/a

#### `void logWithContext(Level level, const std::string &message, const TraceContext &ctx, const Fields &fields={})`
- Source: `include/core/concerns/i_logger.h`:117
- Brief: n/a
- Parameters:
  - `level` (Level): n/a
  - `message` (const std::string &): n/a
  - `ctx` (const TraceContext &): n/a
  - `fields` (const Fields &): n/a

#### `void setLevel(Level level)=0`
- Source: `include/core/concerns/i_logger.h`:142
- Brief: Set Level.
- Parameters:
  - `level` (Level): Input parameter.
- Details: level Input parameter.

#### `void setPattern(const std::string &pattern)=0`
- Source: `include/core/concerns/i_logger.h`:150
- Brief: Set Pattern.
- Parameters:
  - `pattern` (const std::string &): Input parameter.
- Details: pattern Input parameter.

#### `void shutdown() noexcept`
- Source: `include/core/concerns/i_logger.h`:155
- Brief: n/a
- Parameters: none

#### `void trace(const std::string &message)=0`
- Source: `include/core/concerns/i_logger.h`:71
- Brief: Trace.
- Parameters:
  - `message` (const std::string &): Input parameter.
- Details: message Input parameter.

#### `void warn(const std::string &message)=0`
- Source: `include/core/concerns/i_logger.h`:89
- Brief: Warn.
- Parameters:
  - `message` (const std::string &): Input parameter.
- Details: message Input parameter.

#### `~ILogger()=default`
- Source: `include/core/concerns/i_logger.h`:54
- Brief: ILogger.
- Parameters: none
- Return: Return value.
- Details: Return value.

### themis::core::concerns::IMetrics

#### `void decrementGauge(const std::string &name, double delta, const Labels &labels={})=0`
- Source: `include/core/concerns/i_metrics.h`:47
- Brief: n/a
- Parameters:
  - `name` (const std::string &): n/a
  - `delta` (double): n/a
  - `labels` (const Labels &): n/a

#### `std::string exportMetrics() const =0`
- Source: `include/core/concerns/i_metrics.h`:69
- Brief: n/a
- Parameters: none

#### `void flush() noexcept`
- Source: `include/core/concerns/i_metrics.h`:77
- Brief: n/a
- Parameters: none

#### `void incrementCounter(const std::string &name, int64_t value=1, const Labels &labels={})=0`
- Source: `include/core/concerns/i_metrics.h`:37
- Brief: n/a
- Parameters:
  - `name` (const std::string &): n/a
  - `value` (int64_t): n/a
  - `labels` (const Labels &): n/a

#### `void incrementGauge(const std::string &name, double delta, const Labels &labels={})=0`
- Source: `include/core/concerns/i_metrics.h`:45
- Brief: n/a
- Parameters:
  - `name` (const std::string &): n/a
  - `delta` (double): n/a
  - `labels` (const Labels &): n/a

#### `ProbeResult isHealthy() const`
- Source: `include/core/concerns/i_metrics.h`:81
- Brief: n/a
- Parameters: none

#### `void observeHistogram(const std::string &name, double value, const Labels &labels={})=0`
- Source: `include/core/concerns/i_metrics.h`:53
- Brief: n/a
- Parameters:
  - `name` (const std::string &): n/a
  - `value` (double): n/a
  - `labels` (const Labels &): n/a

#### `void recordError(const std::string &operation, const Labels &labels={})=0`
- Source: `include/core/concerns/i_metrics.h`:61
- Brief: n/a
- Parameters:
  - `operation` (const std::string &): n/a
  - `labels` (const Labels &): n/a

#### `void recordLatency(const std::string &operation, double latencyMs, const Labels &labels={})=0`
- Source: `include/core/concerns/i_metrics.h`:59
- Brief: n/a
- Parameters:
  - `operation` (const std::string &): n/a
  - `latencyMs` (double): n/a
  - `labels` (const Labels &): n/a

#### `void recordSuccess(const std::string &operation, const Labels &labels={})=0`
- Source: `include/core/concerns/i_metrics.h`:63
- Brief: n/a
- Parameters:
  - `operation` (const std::string &): n/a
  - `labels` (const Labels &): n/a

#### `void reset()=0`
- Source: `include/core/concerns/i_metrics.h`:74
- Brief: Reset the modification detection flag.
- Parameters: none

#### `void setGauge(const std::string &name, double value, const Labels &labels={})=0`
- Source: `include/core/concerns/i_metrics.h`:43
- Brief: n/a
- Parameters:
  - `name` (const std::string &): n/a
  - `value` (double): n/a
  - `labels` (const Labels &): n/a

#### `void shutdown() noexcept`
- Source: `include/core/concerns/i_metrics.h`:79
- Brief: n/a
- Parameters: none

#### `~IMetrics()=default`
- Source: `include/core/concerns/i_metrics.h`:31
- Brief: IMetrics.
- Parameters: none
- Return: Return value.
- Details: Return value.

### themis::core::concerns::ISecrets

#### `void flush() noexcept`
- Source: `include/core/concerns/i_secrets.h`:45
- Brief: n/a
- Parameters: none

#### `std::optional< std::string > getSecret(std::string_view name) const =0`
- Source: `include/core/concerns/i_secrets.h`:35
- Brief: n/a
- Parameters:
  - `name` (std::string_view): n/a

#### `bool hasSecret(std::string_view name) const =0`
- Source: `include/core/concerns/i_secrets.h`:37
- Brief: n/a
- Parameters:
  - `name` (std::string_view): n/a

#### `ProbeResult isHealthy() const`
- Source: `include/core/concerns/i_secrets.h`:49
- Brief: n/a
- Parameters: none

#### `std::vector< std::string > listSecretNames() const =0`
- Source: `include/core/concerns/i_secrets.h`:39
- Brief: n/a
- Parameters: none

#### `void shutdown() noexcept`
- Source: `include/core/concerns/i_secrets.h`:47
- Brief: n/a
- Parameters: none

#### `~ISecrets()=default`
- Source: `include/core/concerns/i_secrets.h`:29
- Brief: ISecrets.
- Parameters: none
- Return: Return value.
- Details: Return value.

### themis::core::concerns::ITracer

#### `void flush() noexcept`
- Source: `include/core/concerns/i_tracer.h`:128
- Brief: n/a
- Parameters: none

#### `bool initialize(const std::string &serviceName, const std::string &endpoint)=0`
- Source: `include/core/concerns/i_tracer.h`:115
- Brief: n/a
- Parameters:
  - `serviceName` (const std::string &): n/a
  - `endpoint` (const std::string &): n/a

#### `void injectContext(std::map< std::string, std::string > &headers)`
- Source: `include/core/concerns/i_tracer.h`:107
- Brief: n/a
- Parameters:
  - `headers` (std::map< std::string, std::string > &): n/a

#### `ProbeResult isHealthy() const`
- Source: `include/core/concerns/i_tracer.h`:130
- Brief: n/a
- Parameters: none

#### `bool isInitialized() const =0`
- Source: `include/core/concerns/i_tracer.h`:122
- Brief: n/a
- Parameters: none

#### `void shutdown()=0`
- Source: `include/core/concerns/i_tracer.h`:120
- Brief: Shutdown.
- Parameters: none

#### `std::unique_ptr< ISpan > startChildSpan(const std::string &name, const ISpan &parent)=0`
- Source: `include/core/concerns/i_tracer.h`:98
- Brief: n/a
- Parameters:
  - `name` (const std::string &): n/a
  - `parent` (const ISpan &): n/a

#### `std::unique_ptr< ISpan > startSpan(const std::string &name)=0`
- Source: `include/core/concerns/i_tracer.h`:96
- Brief: n/a
- Parameters:
  - `name` (const std::string &): n/a

#### `std::unique_ptr< ISpan > startSpanFromHeaders(const std::string &name, const std::map< std::string, std::string > &headers)`
- Source: `include/core/concerns/i_tracer.h`:100
- Brief: n/a
- Parameters:
  - `name` (const std::string &): n/a
  - `headers` (const std::map< std::string, std::string > &): n/a

#### `~ITracer()=default`
- Source: `include/core/concerns/i_tracer.h`:90
- Brief: ITracer.
- Parameters: none
- Return: Return value.
- Details: Return value.

### themis::core::concerns::ITracer::ISpan

#### `void end()=0`
- Source: `include/core/concerns/i_tracer.h`:81
- Brief: End.
- Parameters: none

#### `bool isValid() const =0`
- Source: `include/core/concerns/i_tracer.h`:83
- Brief: n/a
- Parameters: none

#### `void recordError(const std::string &errorMessage)=0`
- Source: `include/core/concerns/i_tracer.h`:74
- Brief: Record Error.
- Parameters:
  - `errorMessage` (const std::string &): Input parameter.
- Details: errorMessage Input parameter.

#### `void setAttribute(const std::string &key, bool value)=0`
- Source: `include/core/concerns/i_tracer.h`:68
- Brief: Set Attribute.
- Parameters:
  - `key` (const std::string &): Input parameter.
  - `value` (bool): Input parameter.
- Details: key Input parameter. value Input parameter.

#### `void setAttribute(const std::string &key, const char *value)`
- Source: `include/core/concerns/i_tracer.h`:45
- Brief: Set Attribute.
- Parameters:
  - `key` (const std::string &): Input parameter.
  - `value` (const char *): Input parameter.
- Details: key Input parameter. value Input parameter. Calls: std::string().

#### `void setAttribute(const std::string &key, const std::string &value)=0`
- Source: `include/core/concerns/i_tracer.h`:37
- Brief: Set Attribute.
- Parameters:
  - `key` (const std::string &): Input parameter.
  - `value` (const std::string &): Input parameter.
- Details: key Input parameter. value Input parameter.

#### `void setAttribute(const std::string &key, double value)=0`
- Source: `include/core/concerns/i_tracer.h`:61
- Brief: Set Attribute.
- Parameters:
  - `key` (const std::string &): Input parameter.
  - `value` (double): Input parameter.
- Details: key Input parameter. value Input parameter.

#### `void setAttribute(const std::string &key, int64_t value)=0`
- Source: `include/core/concerns/i_tracer.h`:54
- Brief: Set Attribute.
- Parameters:
  - `key` (const std::string &): Input parameter.
  - `value` (int64_t): Input parameter.
- Details: key Input parameter. value Input parameter.

#### `void setStatus(bool ok, const std::string &description="")=0`
- Source: `include/core/concerns/i_tracer.h`:76
- Brief: n/a
- Parameters:
  - `ok` (bool): n/a
  - `description` (const std::string &): n/a

#### `~ISpan()=default`
- Source: `include/core/concerns/i_tracer.h`:30
- Brief: ISpan.
- Parameters: none
- Return: Return value.
- Details: Return value.

### themis::core::concerns::InMemoryAuditLog

#### `void clear()`
- Source: `include/core/concerns/i_audit_log.h`:158
- Brief: Clear.
- Parameters: none
- Details: Calls: lock().

#### `void flush() noexcept override`
- Source: `include/core/concerns/i_audit_log.h`:163
- Brief: n/a
- Parameters: none

#### `std::vector< AuditEvent > getEvents() const`
- Source: `include/core/concerns/i_audit_log.h`:134
- Brief: n/a
- Parameters: none

#### `ProbeResult isHealthy() const override`
- Source: `include/core/concerns/i_audit_log.h`:165
- Brief: n/a
- Parameters: none

#### `void record(const AuditEvent &event) noexcept override`
- Source: `include/core/concerns/i_audit_log.h`:124
- Brief: Record.
- Parameters:
  - `event` (const AuditEvent &): Input parameter.
- Details: event Input parameter. Exception safety: noexcept.

#### `void shutdown() noexcept override`
- Source: `include/core/concerns/i_audit_log.h`:164
- Brief: n/a
- Parameters: none

#### `size_t size() const`
- Source: `include/core/concerns/i_audit_log.h`:144
- Brief: n/a
- Parameters: none

### themis::core::concerns::InMemoryCacheImpl

#### `InMemoryCacheImpl(size_t maxSize=1000, uint64_t defaultTTL=0)`
- Source: `include/core/concerns/inmemory_cache_impl.h`:25
- Brief: n/a
- Parameters:
  - `maxSize` (size_t): n/a
  - `defaultTTL` (uint64_t): n/a

#### `void clear() override`
- Source: `include/core/concerns/inmemory_cache_impl.h`:89
- Brief: Clear.
- Parameters: none

#### `void flush() noexcept override`
- Source: `include/core/concerns/inmemory_cache_impl.h`:167
- Brief: n/a
- Parameters: none

#### `std::optional< CacheEntry > get(std::string_view key) const override`
- Source: `include/core/concerns/inmemory_cache_impl.h`:28
- Brief: n/a
- Parameters:
  - `key` (std::string_view): n/a

#### `uint64_t hitCount() const override`
- Source: `include/core/concerns/inmemory_cache_impl.h`:139
- Brief: n/a
- Parameters: none

#### `double hitRate() const override`
- Source: `include/core/concerns/inmemory_cache_impl.h`:147
- Brief: n/a
- Parameters: none

#### `void invalidate(std::string_view key) override`
- Source: `include/core/concerns/inmemory_cache_impl.h`:79
- Brief: Invalidate.
- Parameters:
  - `key` (std::string_view): Input parameter.
- Details: key Input parameter.

#### `void invalidatePattern(std::string_view pattern) override`
- Source: `include/core/concerns/inmemory_cache_impl.h`:101
- Brief: Invalidate Pattern.
- Parameters:
  - `pattern` (std::string_view): Input parameter.
- Details: pattern Input parameter.

#### `ProbeResult isHealthy() const override`
- Source: `include/core/concerns/inmemory_cache_impl.h`:176
- Brief: n/a
- Parameters: none

#### `uint64_t missCount() const override`
- Source: `include/core/concerns/inmemory_cache_impl.h`:143
- Brief: n/a
- Parameters: none

#### `bool put(std::string_view key, const CacheEntry &entry, uint64_t ttl_ms=0) override`
- Source: `include/core/concerns/inmemory_cache_impl.h`:59
- Brief: n/a
- Parameters:
  - `key` (std::string_view): n/a
  - `entry` (const CacheEntry &): n/a
  - `ttl_ms` (uint64_t): n/a

#### `void setDefaultTTL(uint64_t ttl_ms) override`
- Source: `include/core/concerns/inmemory_cache_impl.h`:162
- Brief: Set Default TTL.
- Parameters:
  - `ttl_ms` (uint64_t): Input parameter.
- Details: ttl_ms Input parameter.

#### `void setMaxSize(size_t maxSize) override`
- Source: `include/core/concerns/inmemory_cache_impl.h`:152
- Brief: Set Max Size.
- Parameters:
  - `maxSize` (size_t): Input parameter.
- Details: maxSize Input parameter.

#### `void shutdown() noexcept override`
- Source: `include/core/concerns/inmemory_cache_impl.h`:171
- Brief: n/a
- Parameters: none

#### `size_t size() const override`
- Source: `include/core/concerns/inmemory_cache_impl.h`:129
- Brief: n/a
- Parameters: none

### themis::core::concerns::InMemoryFeatureFlags

#### `InMemoryFeatureFlags()=default`
- Source: `include/core/concerns/i_feature_flags.h`:73
- Brief: n/a
- Parameters: none

#### `InMemoryFeatureFlags(std::unordered_map< std::string, bool > initial)`
- Source: `include/core/concerns/i_feature_flags.h`:75
- Brief: n/a
- Parameters:
  - `initial` (std::unordered_map< std::string, bool >): n/a

#### `void flush() noexcept override`
- Source: `include/core/concerns/i_feature_flags.h`:109
- Brief: n/a
- Parameters: none

#### `std::unordered_map< std::string, bool > getAllFlags() const override`
- Source: `include/core/concerns/i_feature_flags.h`:99
- Brief: n/a
- Parameters: none

#### `bool isEnabled(std::string_view name) const override`
- Source: `include/core/concerns/i_feature_flags.h`:78
- Brief: n/a
- Parameters:
  - `name` (std::string_view): n/a

#### `ProbeResult isHealthy() const override`
- Source: `include/core/concerns/i_feature_flags.h`:111
- Brief: n/a
- Parameters: none

#### `void setValue(std::string_view name, bool value) override`
- Source: `include/core/concerns/i_feature_flags.h`:89
- Brief: Set Value.
- Parameters:
  - `name` (std::string_view): Input parameter.
  - `value` (bool): Input parameter.
- Details: name Input parameter. value Input parameter.

#### `void shutdown() noexcept override`
- Source: `include/core/concerns/i_feature_flags.h`:110
- Brief: n/a
- Parameters: none

### themis::core::concerns::InMemorySecrets

#### `InMemorySecrets()=default`
- Source: `include/core/concerns/inmemory_secrets.h`:30
- Brief: n/a
- Parameters: none

#### `InMemorySecrets(std::map< std::string, std::string > initial)`
- Source: `include/core/concerns/inmemory_secrets.h`:32
- Brief: n/a
- Parameters:
  - `initial` (std::map< std::string, std::string >): n/a

#### `void flush() noexcept override`
- Source: `include/core/concerns/inmemory_secrets.h`:80
- Brief: n/a
- Parameters: none

#### `std::optional< std::string > getSecret(std::string_view name) const override`
- Source: `include/core/concerns/inmemory_secrets.h`:39
- Brief: n/a
- Parameters:
  - `name` (std::string_view): n/a

#### `bool hasSecret(std::string_view name) const override`
- Source: `include/core/concerns/inmemory_secrets.h`:53
- Brief: n/a
- Parameters:
  - `name` (std::string_view): n/a

#### `ProbeResult isHealthy() const override`
- Source: `include/core/concerns/inmemory_secrets.h`:82
- Brief: n/a
- Parameters: none

#### `std::vector< std::string > listSecretNames() const override`
- Source: `include/core/concerns/inmemory_secrets.h`:63
- Brief: n/a
- Parameters: none

#### `bool removeSecret(std::string_view name)`
- Source: `include/core/concerns/inmemory_secrets.h`:102
- Brief: Remove Secret.
- Parameters:
  - `name` (std::string_view): Input parameter.
- Return: True when the operation succeeds.
- Details: name Input parameter. True when the operation succeeds. Calls: lock(), erase(), std::string().

#### `void setSecret(std::string_view name, std::string_view value)`
- Source: `include/core/concerns/inmemory_secrets.h`:91
- Brief: -------------------------------------------------------------------- Mutable operations (not part of ISecrets) --------------------------------------------------------------------
- Parameters:
  - `name` (std::string_view): Input parameter.
  - `value` (std::string_view): Input parameter.
- Details: name Input parameter. value Input parameter. Calls: lock(), std::string().

#### `void shutdown() noexcept override`
- Source: `include/core/concerns/inmemory_secrets.h`:81
- Brief: n/a
- Parameters: none

#### `size_t size() const`
- Source: `include/core/concerns/inmemory_secrets.h`:107
- Brief: n/a
- Parameters: none

### themis::core::concerns::JaegerTracerAdapter

#### `JaegerTracerAdapter()`
- Source: `include/core/concerns/jaeger_tracer_adapter.h`:52
- Brief: n/a
- Parameters: none

#### `JaegerTracerAdapter(const CircuitBreakerConfig &cb_config)`
- Source: `include/core/concerns/jaeger_tracer_adapter.h`:44
- Brief: Jaeger Tracer Adapter.
- Parameters:
  - `cb_config` (const CircuitBreakerConfig &): Input parameter.
- Return: Return value.
- Details: cb_config Input parameter. Return value. Implements JaegerTracerAdapter without additional internal calls.

#### `sharding::CircuitBreaker::State circuitBreakerState() const`
- Source: `include/core/concerns/jaeger_tracer_adapter.h`:235
- Brief: n/a
- Parameters: none

#### `void flush() noexcept override`
- Source: `include/core/concerns/jaeger_tracer_adapter.h`:221
- Brief: n/a
- Parameters: none

#### `std::string headerValueCI(const std::map< std::string, std::string > &headers, const std::string &name)`
- Source: `include/core/concerns/jaeger_tracer_adapter.h`:247
- Brief: n/a
- Parameters:
  - `headers` (const std::map< std::string, std::string > &): n/a
  - `name` (const std::string &): n/a

#### `bool initialize(const std::string &serviceName, const std::string &endpoint) override`
- Source: `include/core/concerns/jaeger_tracer_adapter.h`:204
- Brief: n/a
- Parameters:
  - `serviceName` (const std::string &): n/a
  - `endpoint` (const std::string &): n/a

#### `void injectContext(std::map< std::string, std::string > &carrier_headers) override`
- Source: `include/core/concerns/jaeger_tracer_adapter.h`:184
- Brief: n/a
- Parameters:
  - `carrier_headers` (std::map< std::string, std::string > &): n/a

#### `ProbeResult isHealthy() const override`
- Source: `include/core/concerns/jaeger_tracer_adapter.h`:225
- Brief: n/a
- Parameters: none

#### `bool isInitialized() const override`
- Source: `include/core/concerns/jaeger_tracer_adapter.h`:219
- Brief: n/a
- Parameters: none

#### `bool parseUberTraceId(const std::string &value, UberTraceIds &out)`
- Source: `include/core/concerns/jaeger_tracer_adapter.h`:279
- Brief: Parse Uber Trace Id.
- Parameters:
  - `value` (const std::string &): Input parameter.
  - `out` (UberTraceIds &): Input/output parameter.
- Return: True when the operation succeeds.
- Details: value Input parameter. out Input/output parameter. True when the operation succeeds. Calls: reserve(), find(), push_back(), substr(), size(), empty(), std::all_of(), begin().

#### `void shutdown() override`
- Source: `include/core/concerns/jaeger_tracer_adapter.h`:214
- Brief: Shutdown.
- Parameters: none

#### `std::unique_ptr< ISpan > startChildSpan(const std::string &name, const ISpan &parent) override`
- Source: `include/core/concerns/jaeger_tracer_adapter.h`:121
- Brief: n/a
- Parameters:
  - `name` (const std::string &): n/a
  - `parent` (const ISpan &): n/a

#### `std::unique_ptr< ISpan > startSpan(const std::string &name) override`
- Source: `include/core/concerns/jaeger_tracer_adapter.h`:111
- Brief: n/a
- Parameters:
  - `name` (const std::string &): n/a

#### `std::unique_ptr< ISpan > startSpanFromHeaders(const std::string &name, const std::map< std::string, std::string > &carrier_headers) override`
- Source: `include/core/concerns/jaeger_tracer_adapter.h`:140
- Brief: n/a
- Parameters:
  - `name` (const std::string &): n/a
  - `carrier_headers` (const std::map< std::string, std::string > &): n/a

### themis::core::concerns::JaegerTracerAdapter::JaegerSpanAdapter

#### `JaegerSpanAdapter(themis::Tracer::Span span)`
- Source: `include/core/concerns/jaeger_tracer_adapter.h`:65
- Brief: Jaeger Span Adapter.
- Parameters:
  - `span` (themis::Tracer::Span): Input parameter.
- Return: Return value.
- Details: span Input parameter. Return value.

#### `void end() override`
- Source: `include/core/concerns/jaeger_tracer_adapter.h`:86
- Brief: End.
- Parameters: none

#### `themis::Tracer::Span & getSpan()`
- Source: `include/core/concerns/jaeger_tracer_adapter.h`:100
- Brief: Get Span.
- Parameters: none
- Return: Return value.
- Details: Return value. Implements getSpan without additional internal calls.

#### `const themis::Tracer::Span & getSpan() const`
- Source: `include/core/concerns/jaeger_tracer_adapter.h`:101
- Brief: n/a
- Parameters: none

#### `bool isValid() const override`
- Source: `include/core/concerns/jaeger_tracer_adapter.h`:89
- Brief: n/a
- Parameters: none

#### `void recordError(const std::string &errorMessage) override`
- Source: `include/core/concerns/jaeger_tracer_adapter.h`:80
- Brief: Record Error.
- Parameters:
  - `errorMessage` (const std::string &): Input parameter.
- Details: errorMessage Input parameter.

#### `void setAttribute(const std::string &key, bool value) override`
- Source: `include/core/concerns/jaeger_tracer_adapter.h`:77
- Brief: Set Attribute.
- Parameters:
  - `key` (const std::string &): Input parameter.
  - `value` (bool): Input parameter.
- Details: key Input parameter. value Input parameter.

#### `void setAttribute(const std::string &key, const std::string &value) override`
- Source: `include/core/concerns/jaeger_tracer_adapter.h`:68
- Brief: Set Attribute.
- Parameters:
  - `key` (const std::string &): Input parameter.
  - `value` (const std::string &): Input parameter.
- Details: key Input parameter. value Input parameter.

#### `void setAttribute(const std::string &key, double value) override`
- Source: `include/core/concerns/jaeger_tracer_adapter.h`:74
- Brief: Set Attribute.
- Parameters:
  - `key` (const std::string &): Input parameter.
  - `value` (double): Input parameter.
- Details: key Input parameter. value Input parameter.

#### `void setAttribute(const std::string &key, int64_t value) override`
- Source: `include/core/concerns/jaeger_tracer_adapter.h`:71
- Brief: Set Attribute.
- Parameters:
  - `key` (const std::string &): Input parameter.
  - `value` (int64_t): Input parameter.
- Details: key Input parameter. value Input parameter.

#### `void setStatus(bool ok, const std::string &description="") override`
- Source: `include/core/concerns/jaeger_tracer_adapter.h`:83
- Brief: n/a
- Parameters:
  - `ok` (bool): n/a
  - `description` (const std::string &): n/a

#### `~JaegerSpanAdapter() override`
- Source: `include/core/concerns/jaeger_tracer_adapter.h`:93
- Brief: n/a
- Parameters: none

### themis::core::concerns::LFUEvictionStrategy

#### `void clear() override`
- Source: `include/core/concerns/eviction_strategies.h`:136
- Brief: Clear.
- Parameters: none

#### `uint64_t getCurrentTimeMs() const`
- Source: `include/core/concerns/eviction_strategies.h`:156
- Brief: n/a
- Parameters: none

#### `std::string_view getName() const override`
- Source: `include/core/concerns/eviction_strategies.h`:144
- Brief: Get Name.
- Parameters: none
- Return: Return value.
- Details: Return value.

#### `void onAccess(std::string_view key) override`
- Source: `include/core/concerns/eviction_strategies.h`:89
- Brief: On Access.
- Parameters:
  - `key` (std::string_view): Input parameter.
- Details: key Input parameter.

#### `void onInsert(std::string_view key, uint64_t timestamp_ms) override`
- Source: `include/core/concerns/eviction_strategies.h`:97
- Brief: On Insert.
- Parameters:
  - `key` (std::string_view): Input parameter.
  - `timestamp_ms` (uint64_t): Input parameter.
- Details: key Input parameter. timestamp_ms Input parameter.

#### `void onRemove(std::string_view key) override`
- Source: `include/core/concerns/eviction_strategies.h`:114
- Brief: On Remove.
- Parameters:
  - `key` (std::string_view): Input parameter.
- Details: key Input parameter.

#### `std::optional< std::string > selectVictim() override`
- Source: `include/core/concerns/eviction_strategies.h`:118
- Brief: Select Victim.
- Parameters: none
- Return: Return value.
- Details: Return value.

#### `size_t size() const override`
- Source: `include/core/concerns/eviction_strategies.h`:140
- Brief: Size.
- Parameters: none
- Return: Return value.
- Details: Return value.

### themis::core::concerns::LRUEvictionStrategy

#### `void clear() override`
- Source: `include/core/concerns/eviction_strategies.h`:69
- Brief: Clear.
- Parameters: none

#### `std::string_view getName() const override`
- Source: `include/core/concerns/eviction_strategies.h`:78
- Brief: Get Name.
- Parameters: none
- Return: Return value.
- Details: Return value.

#### `void onAccess(std::string_view key) override`
- Source: `include/core/concerns/eviction_strategies.h`:27
- Brief: On Access.
- Parameters:
  - `key` (std::string_view): Input parameter.
- Details: key Input parameter.

#### `void onInsert(std::string_view key, uint64_t timestamp_ms) override`
- Source: `include/core/concerns/eviction_strategies.h`:35
- Brief: On Insert.
- Parameters:
  - `key` (std::string_view): Input parameter.
  - `timestamp_ms` (uint64_t): Input parameter.
- Details: key Input parameter. timestamp_ms Input parameter.

#### `void onRemove(std::string_view key) override`
- Source: `include/core/concerns/eviction_strategies.h`:54
- Brief: On Remove.
- Parameters:
  - `key` (std::string_view): Input parameter.
- Details: key Input parameter.

#### `std::optional< std::string > selectVictim() override`
- Source: `include/core/concerns/eviction_strategies.h`:62
- Brief: Select Victim.
- Parameters: none
- Return: Return value.
- Details: Return value.

#### `size_t size() const override`
- Source: `include/core/concerns/eviction_strategies.h`:74
- Brief: Size.
- Parameters: none
- Return: Return value.
- Details: Return value.

### themis::core::concerns::LatencyTimer

#### `LatencyTimer(IMetrics &metrics, const std::string &operation, const IMetrics::Labels &labels={})`
- Source: `include/core/concerns/i_metrics.h`:86
- Brief: n/a
- Parameters:
  - `metrics` (IMetrics &): n/a
  - `operation` (const std::string &): n/a
  - `labels` (const IMetrics::Labels &): n/a

#### `double elapsedMs() const noexcept`
- Source: `include/core/concerns/i_metrics.h`:96
- Brief: n/a
- Parameters: none

#### `~LatencyTimer()`
- Source: `include/core/concerns/i_metrics.h`:90
- Brief: n/a
- Parameters: none

### themis::core::concerns::LockFreeMetrics

#### `LockFreeMetrics(LockFreeMetrics &&)=delete`
- Source: `include/core/concerns/lockfree_metrics.h`:48
- Brief: n/a
- Parameters:
  - `<unnamed>` (LockFreeMetrics &&): n/a

#### `LockFreeMetrics(const LockFreeMetrics &)=delete`
- Source: `include/core/concerns/lockfree_metrics.h`:46
- Brief: n/a
- Parameters:
  - `<unnamed>` (const LockFreeMetrics &): n/a

#### `LockFreeMetrics(std::chrono::milliseconds flush_interval=DEFAULT_FLUSH_INTERVAL)`
- Source: `include/core/concerns/lockfree_metrics.h`:40
- Brief: n/a
- Parameters:
  - `flush_interval` (std::chrono::milliseconds): n/a

#### `void applyObservation(const HistoObservation &obs) noexcept`
- Source: `include/core/concerns/lockfree_metrics.h`:300
- Brief: Apply Observation.
- Parameters:
  - `obs` (const HistoObservation &): Input parameter.
- Details: obs Input parameter. Exception safety: noexcept.

#### `void decrementGauge(const std::string &name, double delta, const Labels &labels={}) override`
- Source: `include/core/concerns/lockfree_metrics.h`:68
- Brief: Decrement Gauge.
- Parameters:
  - `name` (const std::string &): Input parameter.
  - `delta` (double): Input parameter.
  - `labels` (const Labels &): Input parameter.
- Details: name Input parameter. delta Input parameter. labels Input parameter. Calls: makeKey(), getOrCreateGauge(), fetch_sub().

#### `void drainAllRings() noexcept`
- Source: `include/core/concerns/lockfree_metrics.h`:293
- Brief: Drain All Rings.
- Parameters: none
- Details: Exception safety: noexcept.

#### `uint64_t droppedObservations() const noexcept`
- Source: `include/core/concerns/lockfree_metrics.h`:113
- Brief: n/a
- Parameters: none

#### `std::string exportMetrics() const override`
- Source: `include/core/concerns/lockfree_metrics.h`:95
- Brief: n/a
- Parameters: none

#### `void flush() noexcept override`
- Source: `include/core/concerns/lockfree_metrics.h`:103
- Brief: n/a
- Parameters: none

#### `void flushLoop() noexcept`
- Source: `include/core/concerns/lockfree_metrics.h`:325
- Brief: Flush Loop.
- Parameters: none
- Details: Exception safety: noexcept.

#### `CounterEntry * getOrCreateCounter(const std::string &key, const std::string &name, const Labels &labels)`
- Source: `include/core/concerns/lockfree_metrics.h`:153
- Brief: Get Or Create Counter.
- Parameters:
  - `key` (const std::string &): Input parameter.
  - `name` (const std::string &): Input parameter.
  - `labels` (const Labels &): Input parameter.
- Return: Pointer to the result.
- Details: key Input parameter. name Input parameter. labels Input parameter. Pointer to the result.

#### `GaugeEntry * getOrCreateGauge(const std::string &key, const std::string &name, const Labels &labels)`
- Source: `include/core/concerns/lockfree_metrics.h`:180
- Brief: Get Or Create Gauge.
- Parameters:
  - `key` (const std::string &): Input parameter.
  - `name` (const std::string &): Input parameter.
  - `labels` (const Labels &): Input parameter.
- Return: Pointer to the result.
- Details: key Input parameter. name Input parameter. labels Input parameter. Pointer to the result.

#### `HistoAggregate * getOrCreateHistoAggregate(const std::string &key, const std::string &name, const Labels &labels)`
- Source: `include/core/concerns/lockfree_metrics.h`:285
- Brief: Get Or Create Histo Aggregate.
- Parameters:
  - `key` (const std::string &): Input parameter.
  - `name` (const std::string &): Input parameter.
  - `labels` (const Labels &): Input parameter.
- Return: Pointer to the result.
- Details: key Input parameter. name Input parameter. labels Input parameter. Pointer to the result.

#### `HistoRing & getOrRegisterThreadRing()`
- Source: `include/core/concerns/lockfree_metrics.h`:260
- Brief: Get Or Register Thread Ring.
- Parameters: none
- Return: Return value.
- Details: Return value.

#### `void incrementCounter(const std::string &name, int64_t value=1, const Labels &labels={}) override`
- Source: `include/core/concerns/lockfree_metrics.h`:55
- Brief: Increment Counter.
- Parameters:
  - `name` (const std::string &): Input parameter.
  - `value` (int64_t): Input parameter.
  - `labels` (const Labels &): Input parameter.
- Details: name Input parameter. value Input parameter. labels Input parameter. Calls: makeKey(), getOrCreateCounter(), fetch_add().

#### `void incrementGauge(const std::string &name, double delta, const Labels &labels={}) override`
- Source: `include/core/concerns/lockfree_metrics.h`:65
- Brief: Increment Gauge.
- Parameters:
  - `name` (const std::string &): Input parameter.
  - `delta` (double): Input parameter.
  - `labels` (const Labels &): Input parameter.
- Details: name Input parameter. delta Input parameter. labels Input parameter. Calls: makeKey(), getOrCreateGauge(), fetch_add().

#### `ProbeResult isHealthy() const override`
- Source: `include/core/concerns/lockfree_metrics.h`:107
- Brief: n/a
- Parameters: none

#### `std::string makeKey(const std::string &name, const Labels &labels)`
- Source: `include/core/concerns/lockfree_metrics.h`:128
- Brief: Make Key.
- Parameters:
  - `name` (const std::string &): Input parameter.
  - `labels` (const Labels &): Input parameter.
- Return: Return value.
- Details: name Input parameter. labels Input parameter. Return value. name Input parameter. labels Input parameter. Return value. Calls: empty(), reserve(), size().

#### `void observeHistogram(const std::string &name, double value, const Labels &labels={}) override`
- Source: `include/core/concerns/lockfree_metrics.h`:75
- Brief: Observe Histogram.
- Parameters:
  - `name` (const std::string &): Input parameter.
  - `value` (double): Input parameter.
  - `labels` (const Labels &): Input parameter.
- Details: name Input parameter. value Input parameter. labels Input parameter. Calls: getOrRegisterThreadRing(), makeKey(), tryPush(), std::move(), fetch_add().

#### `LockFreeMetrics & operator=(LockFreeMetrics &&)=delete`
- Source: `include/core/concerns/lockfree_metrics.h`:49
- Brief: n/a
- Parameters:
  - `<unnamed>` (LockFreeMetrics &&): n/a

#### `LockFreeMetrics & operator=(const LockFreeMetrics &)=delete`
- Source: `include/core/concerns/lockfree_metrics.h`:47
- Brief: n/a
- Parameters:
  - `<unnamed>` (const LockFreeMetrics &): n/a

#### `void recordError(const std::string &operation, const Labels &labels={}) override`
- Source: `include/core/concerns/lockfree_metrics.h`:85
- Brief: Record Error.
- Parameters:
  - `operation` (const std::string &): Input parameter.
  - `labels` (const Labels &): Input parameter.
- Details: operation Input parameter. labels Input parameter. Calls: incrementCounter().

#### `void recordLatency(const std::string &operation, double latencyMs, const Labels &labels={}) override`
- Source: `include/core/concerns/lockfree_metrics.h`:82
- Brief: Record Latency.
- Parameters:
  - `operation` (const std::string &): Input parameter.
  - `latencyMs` (double): Input parameter.
  - `labels` (const Labels &): Input parameter.
- Details: operation Input parameter. latencyMs Input parameter. labels Input parameter. Calls: observeHistogram().

#### `void recordSuccess(const std::string &operation, const Labels &labels={}) override`
- Source: `include/core/concerns/lockfree_metrics.h`:88
- Brief: Record Success.
- Parameters:
  - `operation` (const std::string &): Input parameter.
  - `labels` (const Labels &): Input parameter.
- Details: operation Input parameter. labels Input parameter. Calls: incrementCounter().

#### `void reset() override`
- Source: `include/core/concerns/lockfree_metrics.h`:97
- Brief: Reset the modification detection flag.
- Parameters: none
- Details: Calls: lock(), clear(), store().

#### `void setGauge(const std::string &name, double value, const Labels &labels={}) override`
- Source: `include/core/concerns/lockfree_metrics.h`:62
- Brief: Set Gauge.
- Parameters:
  - `name` (const std::string &): Input parameter.
  - `value` (double): Input parameter.
  - `labels` (const Labels &): Input parameter.
- Details: name Input parameter. value Input parameter. labels Input parameter. Calls: makeKey(), getOrCreateGauge(), store().

#### `void shutdown() noexcept override`
- Source: `include/core/concerns/lockfree_metrics.h`:105
- Brief: n/a
- Parameters: none

#### `void startFlushThread()`
- Source: `include/core/concerns/lockfree_metrics.h`:315
- Brief: Start Flush Thread.
- Parameters: none
- Details: Calls: count(), store(), std::thread().

#### `void stopFlushThread() noexcept`
- Source: `include/core/concerns/lockfree_metrics.h`:320
- Brief: Stop Flush Thread.
- Parameters: none
- Details: Exception safety: noexcept.

#### `~LockFreeMetrics() override`
- Source: `include/core/concerns/lockfree_metrics.h`:43
- Brief: n/a
- Parameters: none

### themis::core::concerns::LockFreeMetrics::CounterEntry

#### `CounterEntry(std::string n, Labels l)`
- Source: `include/core/concerns/lockfree_metrics.h`:139
- Brief: n/a
- Parameters:
  - `n` (std::string): n/a
  - `l` (Labels): n/a

### themis::core::concerns::LockFreeMetrics::GaugeEntry

#### `GaugeEntry(std::string n, Labels l)`
- Source: `include/core/concerns/lockfree_metrics.h`:166
- Brief: n/a
- Parameters:
  - `n` (std::string): n/a
  - `l` (Labels): n/a

### themis::core::concerns::LockFreeMetrics::SPSCRing

#### `bool empty() const noexcept`
- Source: `include/core/concerns/lockfree_metrics.h`:231
- Brief: n/a
- Parameters: none

#### `bool tryPop(T &out) noexcept`
- Source: `include/core/concerns/lockfree_metrics.h`:221
- Brief: n/a
- Parameters:
  - `out` (T &): n/a

#### `bool tryPush(T item) noexcept`
- Source: `include/core/concerns/lockfree_metrics.h`:209
- Brief: n/a
- Parameters:
  - `item` (T): n/a

### themis::core::concerns::MetricLabels

#### `MetricLabels()=default`
- Source: `include/core/concerns/metric_labels.h`:23
- Brief: n/a
- Parameters: none

#### `MetricLabels & add(std::string_view key, std::string_view value)`
- Source: `include/core/concerns/metric_labels.h`:32
- Brief: Add.
- Parameters:
  - `key` (std::string_view): Input parameter.
  - `value` (std::string_view): Input parameter.
- Return: Return value.
- Details: key Input parameter. value Input parameter. Return value. Calls: std::string().

#### `bool empty() const noexcept`
- Source: `include/core/concerns/metric_labels.h`:41
- Brief: n/a
- Parameters: none

#### `operator IMetrics::Labels() const`
- Source: `include/core/concerns/metric_labels.h`:39
- Brief: n/a
- Parameters: none

#### `std::size_t size() const noexcept`
- Source: `include/core/concerns/metric_labels.h`:43
- Brief: n/a
- Parameters: none

#### `IMetrics::Labels toLabels() const`
- Source: `include/core/concerns/metric_labels.h`:37
- Brief: n/a
- Parameters: none

### themis::core::concerns::NoOpAsyncCache

#### `void clear() override`
- Source: `include/core/concerns/i_async_cache.h`:79
- Brief: Clear.
- Parameters: none

#### `void flush() noexcept override`
- Source: `include/core/concerns/i_async_cache.h`:87
- Brief: n/a
- Parameters: none

#### `std::optional< CacheEntry > get(std::string_view) const override`
- Source: `include/core/concerns/i_async_cache.h`:72
- Brief: n/a
- Parameters:
  - `<unnamed>` (std::string_view): n/a

#### `std::future< std::optional< CacheEntry > > getAsync(std::string_view) override`
- Source: `include/core/concerns/i_async_cache.h`:92
- Brief: Get Async.
- Parameters:
  - `key` (std::string_view): Input parameter.
- Return: Return value.
- Details: key Input parameter. Return value. Calls: std::async(), std::string(), get().

#### `uint64_t hitCount() const override`
- Source: `include/core/concerns/i_async_cache.h`:82
- Brief: n/a
- Parameters: none

#### `double hitRate() const override`
- Source: `include/core/concerns/i_async_cache.h`:84
- Brief: n/a
- Parameters: none

#### `void invalidate(std::string_view) override`
- Source: `include/core/concerns/i_async_cache.h`:78
- Brief: Invalidate.
- Parameters:
  - `key` (std::string_view): Input parameter.
- Details: key Input parameter.

#### `std::future< void > invalidateAsync(std::string_view) override`
- Source: `include/core/concerns/i_async_cache.h`:99
- Brief: Invalidate Async.
- Parameters:
  - `key` (std::string_view): Input parameter.
- Return: Return value.
- Details: key Input parameter. Return value. Calls: std::async(), std::string(), invalidate().

#### `void invalidatePattern(std::string_view) override`
- Source: `include/core/concerns/i_async_cache.h`:80
- Brief: Invalidate Pattern.
- Parameters:
  - `pattern` (std::string_view): Input parameter.
- Details: pattern Input parameter.

#### `ProbeResult isHealthy() const override`
- Source: `include/core/concerns/i_async_cache.h`:89
- Brief: n/a
- Parameters: none

#### `uint64_t missCount() const override`
- Source: `include/core/concerns/i_async_cache.h`:83
- Brief: n/a
- Parameters: none

#### `bool put(std::string_view, const CacheEntry &, uint64_t) override`
- Source: `include/core/concerns/i_async_cache.h`:75
- Brief: n/a
- Parameters:
  - `<unnamed>` (std::string_view): n/a
  - `<unnamed>` (const CacheEntry &): n/a
  - `<unnamed>` (uint64_t): n/a

#### `std::future< bool > putAsync(std::string_view, const CacheEntry &, uint64_t) override`
- Source: `include/core/concerns/i_async_cache.h`:96
- Brief: n/a
- Parameters:
  - `<unnamed>` (std::string_view): n/a
  - `<unnamed>` (const CacheEntry &): n/a
  - `<unnamed>` (uint64_t): n/a

#### `void setDefaultTTL(uint64_t) override`
- Source: `include/core/concerns/i_async_cache.h`:86
- Brief: Set Default TTL.
- Parameters:
  - `ttl_ms` (uint64_t): Input parameter.
- Details: ttl_ms Input parameter.

#### `void setMaxSize(size_t) override`
- Source: `include/core/concerns/i_async_cache.h`:85
- Brief: Set Max Size.
- Parameters:
  - `maxSize` (size_t): Input parameter.
- Details: maxSize Input parameter.

#### `void shutdown() noexcept override`
- Source: `include/core/concerns/i_async_cache.h`:88
- Brief: n/a
- Parameters: none

#### `size_t size() const override`
- Source: `include/core/concerns/i_async_cache.h`:81
- Brief: n/a
- Parameters: none

### themis::core::concerns::NoOpAsyncLogger

#### `void critical(const std::string &) override`
- Source: `include/core/concerns/i_async_logger.h`:126
- Brief: Critical.
- Parameters:
  - `message` (const std::string &): Input parameter.
- Details: message Input parameter.

#### `std::future< void > criticalAsync(std::string_view) override`
- Source: `include/core/concerns/i_async_logger.h`:143
- Brief: Critical Async.
- Parameters:
  - `message` (std::string_view): Input parameter.
- Return: Return value.
- Details: message Input parameter. Return value. Calls: logAsync().

#### `void debug(const std::string &) override`
- Source: `include/core/concerns/i_async_logger.h`:122
- Brief: Debug.
- Parameters:
  - `message` (const std::string &): Input parameter.
- Details: message Input parameter.

#### `std::future< void > debugAsync(std::string_view) override`
- Source: `include/core/concerns/i_async_logger.h`:139
- Brief: Debug Async.
- Parameters:
  - `message` (std::string_view): Input parameter.
- Return: Return value.
- Details: message Input parameter. Return value. Calls: logAsync().

#### `void error(const std::string &) override`
- Source: `include/core/concerns/i_async_logger.h`:125
- Brief: Error.
- Parameters:
  - `message` (const std::string &): Input parameter.
- Details: message Input parameter.

#### `std::future< void > errorAsync(std::string_view) override`
- Source: `include/core/concerns/i_async_logger.h`:142
- Brief: Error Async.
- Parameters:
  - `message` (std::string_view): Input parameter.
- Return: Return value.
- Details: message Input parameter. Return value. Calls: logAsync().

#### `void flush() noexcept override`
- Source: `include/core/concerns/i_async_logger.h`:130
- Brief: n/a
- Parameters: none

#### `Level getLevel() const override`
- Source: `include/core/concerns/i_async_logger.h`:128
- Brief: n/a
- Parameters: none

#### `void info(const std::string &) override`
- Source: `include/core/concerns/i_async_logger.h`:123
- Brief: Info.
- Parameters:
  - `message` (const std::string &): Input parameter.
- Details: message Input parameter.

#### `std::future< void > infoAsync(std::string_view) override`
- Source: `include/core/concerns/i_async_logger.h`:140
- Brief: Info Async.
- Parameters:
  - `message` (std::string_view): Input parameter.
- Return: Return value.
- Details: message Input parameter. Return value. Calls: logAsync().

#### `ProbeResult isHealthy() const override`
- Source: `include/core/concerns/i_async_logger.h`:132
- Brief: n/a
- Parameters: none

#### `void log(Level, const std::string &) override`
- Source: `include/core/concerns/i_async_logger.h`:120
- Brief: Log.
- Parameters:
  - `level` (Level): Input parameter.
  - `message` (const std::string &): Input parameter.
- Details: level Input parameter. message Input parameter.

#### `std::future< void > logAsync(Level, std::string_view) override`
- Source: `include/core/concerns/i_async_logger.h`:135
- Brief: Log Async.
- Parameters:
  - `level` (Level): Input parameter.
  - `message` (std::string_view): Input parameter.
- Return: Return value.
- Details: level Input parameter. message Input parameter. Return value. Calls: std::async(), std::string(), log().

#### `std::future< void > logStructuredAsync(Level, std::string_view, const Fields &) override`
- Source: `include/core/concerns/i_async_logger.h`:144
- Brief: n/a
- Parameters:
  - `<unnamed>` (Level): n/a
  - `<unnamed>` (std::string_view): n/a
  - `<unnamed>` (const Fields &): n/a

#### `std::future< void > noop_future()`
- Source: `include/core/concerns/i_async_logger.h`:156
- Brief: Noop future.
- Parameters: none
- Return: Return value.
- Details: Return value. Calls: std::async().

#### `void setLevel(Level level) override`
- Source: `include/core/concerns/i_async_logger.h`:127
- Brief: Set Level.
- Parameters:
  - `level` (Level): Input parameter.
- Details: level Input parameter.

#### `void setPattern(const std::string &) override`
- Source: `include/core/concerns/i_async_logger.h`:129
- Brief: Set Pattern.
- Parameters:
  - `pattern` (const std::string &): Input parameter.
- Details: pattern Input parameter.

#### `void shutdown() noexcept override`
- Source: `include/core/concerns/i_async_logger.h`:131
- Brief: n/a
- Parameters: none

#### `void trace(const std::string &) override`
- Source: `include/core/concerns/i_async_logger.h`:121
- Brief: Trace.
- Parameters:
  - `message` (const std::string &): Input parameter.
- Details: message Input parameter.

#### `std::future< void > traceAsync(std::string_view) override`
- Source: `include/core/concerns/i_async_logger.h`:138
- Brief: Trace Async.
- Parameters:
  - `message` (std::string_view): Input parameter.
- Return: Return value.
- Details: message Input parameter. Return value. Calls: logAsync().

#### `void warn(const std::string &) override`
- Source: `include/core/concerns/i_async_logger.h`:124
- Brief: Warn.
- Parameters:
  - `message` (const std::string &): Input parameter.
- Details: message Input parameter.

#### `std::future< void > warnAsync(std::string_view) override`
- Source: `include/core/concerns/i_async_logger.h`:141
- Brief: Warn Async.
- Parameters:
  - `message` (std::string_view): Input parameter.
- Return: Return value.
- Details: message Input parameter. Return value. Calls: logAsync().

### themis::core::concerns::NoOpAuditLog

#### `void flush() noexcept override`
- Source: `include/core/concerns/noop_implementations.h`:218
- Brief: n/a
- Parameters: none

#### `ProbeResult isHealthy() const override`
- Source: `include/core/concerns/noop_implementations.h`:220
- Brief: n/a
- Parameters: none

#### `void record(const AuditEvent &) noexcept override`
- Source: `include/core/concerns/noop_implementations.h`:216
- Brief: Record.
- Parameters:
  - `event` (const AuditEvent &): Input parameter.
- Details: event Input parameter. Exception safety: noexcept.

#### `void shutdown() noexcept override`
- Source: `include/core/concerns/noop_implementations.h`:219
- Brief: n/a
- Parameters: none

### themis::core::concerns::NoOpCache

#### `void clear() override`
- Source: `include/core/concerns/noop_implementations.h`:135
- Brief: Clear.
- Parameters: none

#### `void flush() noexcept override`
- Source: `include/core/concerns/noop_implementations.h`:144
- Brief: n/a
- Parameters: none

#### `std::optional< CacheEntry > get(std::string_view key) const override`
- Source: `include/core/concerns/noop_implementations.h`:132
- Brief: n/a
- Parameters:
  - `key` (std::string_view): n/a

#### `uint64_t hitCount() const override`
- Source: `include/core/concerns/noop_implementations.h`:138
- Brief: n/a
- Parameters: none

#### `double hitRate() const override`
- Source: `include/core/concerns/noop_implementations.h`:140
- Brief: n/a
- Parameters: none

#### `void invalidate(std::string_view key) override`
- Source: `include/core/concerns/noop_implementations.h`:134
- Brief: Invalidate.
- Parameters:
  - `key` (std::string_view): Input parameter.
- Details: key Input parameter.

#### `void invalidatePattern(std::string_view pattern) override`
- Source: `include/core/concerns/noop_implementations.h`:136
- Brief: Invalidate Pattern.
- Parameters:
  - `pattern` (std::string_view): Input parameter.
- Details: pattern Input parameter.

#### `ProbeResult isHealthy() const override`
- Source: `include/core/concerns/noop_implementations.h`:146
- Brief: n/a
- Parameters: none

#### `uint64_t missCount() const override`
- Source: `include/core/concerns/noop_implementations.h`:139
- Brief: n/a
- Parameters: none

#### `bool put(std::string_view key, const CacheEntry &entry, uint64_t ttl_ms=0) override`
- Source: `include/core/concerns/noop_implementations.h`:133
- Brief: n/a
- Parameters:
  - `key` (std::string_view): n/a
  - `entry` (const CacheEntry &): n/a
  - `ttl_ms` (uint64_t): n/a

#### `void setDefaultTTL(uint64_t ttl_ms) override`
- Source: `include/core/concerns/noop_implementations.h`:142
- Brief: Set Default TTL.
- Parameters:
  - `ttl_ms` (uint64_t): Input parameter.
- Details: ttl_ms Input parameter.

#### `void setMaxSize(size_t maxSize) override`
- Source: `include/core/concerns/noop_implementations.h`:141
- Brief: Set Max Size.
- Parameters:
  - `maxSize` (size_t): Input parameter.
- Details: maxSize Input parameter.

#### `void shutdown() noexcept override`
- Source: `include/core/concerns/noop_implementations.h`:145
- Brief: n/a
- Parameters: none

#### `size_t size() const override`
- Source: `include/core/concerns/noop_implementations.h`:137
- Brief: n/a
- Parameters: none

### themis::core::concerns::NoOpCircuitBreaker

#### `bool allowRequest() override`
- Source: `include/core/concerns/noop_implementations.h`:177
- Brief: n/a
- Parameters: none

#### `void flush() noexcept override`
- Source: `include/core/concerns/noop_implementations.h`:186
- Brief: n/a
- Parameters: none

#### `void forceOpen() override`
- Source: `include/core/concerns/noop_implementations.h`:184
- Brief: Force Open.
- Parameters: none

#### `size_t getFailureCount() const override`
- Source: `include/core/concerns/noop_implementations.h`:181
- Brief: n/a
- Parameters: none

#### `State getState() const override`
- Source: `include/core/concerns/noop_implementations.h`:180
- Brief: n/a
- Parameters: none

#### `size_t getSuccessCount() const override`
- Source: `include/core/concerns/noop_implementations.h`:182
- Brief: n/a
- Parameters: none

#### `ProbeResult isHealthy() const override`
- Source: `include/core/concerns/noop_implementations.h`:188
- Brief: n/a
- Parameters: none

#### `void recordFailure() override`
- Source: `include/core/concerns/noop_implementations.h`:179
- Brief: Record Failure.
- Parameters: none

#### `void recordSuccess() override`
- Source: `include/core/concerns/noop_implementations.h`:178
- Brief: Record Success.
- Parameters: none

#### `void reset() override`
- Source: `include/core/concerns/noop_implementations.h`:183
- Brief: Reset the modification detection flag.
- Parameters: none

#### `void shutdown() noexcept override`
- Source: `include/core/concerns/noop_implementations.h`:187
- Brief: n/a
- Parameters: none

### themis::core::concerns::NoOpFeatureFlags

#### `void flush() noexcept override`
- Source: `include/core/concerns/noop_implementations.h`:203
- Brief: n/a
- Parameters: none

#### `std::unordered_map< std::string, bool > getAllFlags() const override`
- Source: `include/core/concerns/noop_implementations.h`:201
- Brief: n/a
- Parameters: none

#### `bool isEnabled(std::string_view) const override`
- Source: `include/core/concerns/noop_implementations.h`:199
- Brief: n/a
- Parameters:
  - `<unnamed>` (std::string_view): n/a

#### `ProbeResult isHealthy() const override`
- Source: `include/core/concerns/noop_implementations.h`:205
- Brief: n/a
- Parameters: none

#### `void setValue(std::string_view, bool) override`
- Source: `include/core/concerns/noop_implementations.h`:200
- Brief: Set Value.
- Parameters:
  - `name` (std::string_view): Input parameter.
  - `value` (bool): Input parameter.
- Details: name Input parameter. value Input parameter.

#### `void shutdown() noexcept override`
- Source: `include/core/concerns/noop_implementations.h`:204
- Brief: n/a
- Parameters: none

### themis::core::concerns::NoOpLogger

#### `void critical(const std::string &message) override`
- Source: `include/core/concerns/noop_implementations.h`:45
- Brief: Critical.
- Parameters:
  - `message` (const std::string &): Input parameter.
- Details: message Input parameter.

#### `void debug(const std::string &message) override`
- Source: `include/core/concerns/noop_implementations.h`:41
- Brief: Debug.
- Parameters:
  - `message` (const std::string &): Input parameter.
- Details: message Input parameter.

#### `void error(const std::string &message) override`
- Source: `include/core/concerns/noop_implementations.h`:44
- Brief: Error.
- Parameters:
  - `message` (const std::string &): Input parameter.
- Details: message Input parameter.

#### `void flush() noexcept override`
- Source: `include/core/concerns/noop_implementations.h`:52
- Brief: n/a
- Parameters: none

#### `Level getLevel() const override`
- Source: `include/core/concerns/noop_implementations.h`:49
- Brief: n/a
- Parameters: none

#### `void info(const std::string &message) override`
- Source: `include/core/concerns/noop_implementations.h`:42
- Brief: Info.
- Parameters:
  - `message` (const std::string &): Input parameter.
- Details: message Input parameter.

#### `ProbeResult isHealthy() const override`
- Source: `include/core/concerns/noop_implementations.h`:54
- Brief: n/a
- Parameters: none

#### `void log(Level level, const std::string &message) override`
- Source: `include/core/concerns/noop_implementations.h`:39
- Brief: Log.
- Parameters:
  - `level` (Level): Input parameter.
  - `message` (const std::string &): Input parameter.
- Details: level Input parameter. message Input parameter.

#### `void logStructured(Level level, const std::string &message, const Fields &fields={}) override`
- Source: `include/core/concerns/noop_implementations.h`:46
- Brief: n/a
- Parameters:
  - `level` (Level): n/a
  - `message` (const std::string &): n/a
  - `fields` (const Fields &): n/a

#### `void setLevel(Level level) override`
- Source: `include/core/concerns/noop_implementations.h`:48
- Brief: Set Level.
- Parameters:
  - `level` (Level): Input parameter.
- Details: level Input parameter.

#### `void setPattern(const std::string &pattern) override`
- Source: `include/core/concerns/noop_implementations.h`:50
- Brief: Set Pattern.
- Parameters:
  - `pattern` (const std::string &): Input parameter.
- Details: pattern Input parameter.

#### `void shutdown() noexcept override`
- Source: `include/core/concerns/noop_implementations.h`:53
- Brief: n/a
- Parameters: none

#### `void trace(const std::string &message) override`
- Source: `include/core/concerns/noop_implementations.h`:40
- Brief: Trace.
- Parameters:
  - `message` (const std::string &): Input parameter.
- Details: message Input parameter.

#### `void warn(const std::string &message) override`
- Source: `include/core/concerns/noop_implementations.h`:43
- Brief: Warn.
- Parameters:
  - `message` (const std::string &): Input parameter.
- Details: message Input parameter.

### themis::core::concerns::NoOpMetrics

#### `void decrementGauge(const std::string &name, double delta, const Labels &labels={}) override`
- Source: `include/core/concerns/noop_implementations.h`:111
- Brief: n/a
- Parameters:
  - `name` (const std::string &): n/a
  - `delta` (double): n/a
  - `labels` (const Labels &): n/a

#### `std::string exportMetrics() const override`
- Source: `include/core/concerns/noop_implementations.h`:116
- Brief: n/a
- Parameters: none

#### `void flush() noexcept override`
- Source: `include/core/concerns/noop_implementations.h`:119
- Brief: n/a
- Parameters: none

#### `void incrementCounter(const std::string &name, int64_t value=1, const Labels &labels={}) override`
- Source: `include/core/concerns/noop_implementations.h`:108
- Brief: n/a
- Parameters:
  - `name` (const std::string &): n/a
  - `value` (int64_t): n/a
  - `labels` (const Labels &): n/a

#### `void incrementGauge(const std::string &name, double delta, const Labels &labels={}) override`
- Source: `include/core/concerns/noop_implementations.h`:110
- Brief: n/a
- Parameters:
  - `name` (const std::string &): n/a
  - `delta` (double): n/a
  - `labels` (const Labels &): n/a

#### `ProbeResult isHealthy() const override`
- Source: `include/core/concerns/noop_implementations.h`:121
- Brief: n/a
- Parameters: none

#### `void observeHistogram(const std::string &name, double value, const Labels &labels={}) override`
- Source: `include/core/concerns/noop_implementations.h`:112
- Brief: n/a
- Parameters:
  - `name` (const std::string &): n/a
  - `value` (double): n/a
  - `labels` (const Labels &): n/a

#### `void recordError(const std::string &operation, const Labels &labels={}) override`
- Source: `include/core/concerns/noop_implementations.h`:114
- Brief: n/a
- Parameters:
  - `operation` (const std::string &): n/a
  - `labels` (const Labels &): n/a

#### `void recordLatency(const std::string &operation, double latencyMs, const Labels &labels={}) override`
- Source: `include/core/concerns/noop_implementations.h`:113
- Brief: n/a
- Parameters:
  - `operation` (const std::string &): n/a
  - `latencyMs` (double): n/a
  - `labels` (const Labels &): n/a

#### `void recordSuccess(const std::string &operation, const Labels &labels={}) override`
- Source: `include/core/concerns/noop_implementations.h`:115
- Brief: n/a
- Parameters:
  - `operation` (const std::string &): n/a
  - `labels` (const Labels &): n/a

#### `void reset() override`
- Source: `include/core/concerns/noop_implementations.h`:117
- Brief: Reset the modification detection flag.
- Parameters: none

#### `void setGauge(const std::string &name, double value, const Labels &labels={}) override`
- Source: `include/core/concerns/noop_implementations.h`:109
- Brief: n/a
- Parameters:
  - `name` (const std::string &): n/a
  - `value` (double): n/a
  - `labels` (const Labels &): n/a

#### `void shutdown() noexcept override`
- Source: `include/core/concerns/noop_implementations.h`:120
- Brief: n/a
- Parameters: none

### themis::core::concerns::NoOpSecrets

#### `void flush() noexcept override`
- Source: `include/core/concerns/noop_implementations.h`:164
- Brief: n/a
- Parameters: none

#### `std::optional< std::string > getSecret(std::string_view name) const override`
- Source: `include/core/concerns/noop_implementations.h`:159
- Brief: n/a
- Parameters:
  - `name` (std::string_view): n/a

#### `bool hasSecret(std::string_view name) const override`
- Source: `include/core/concerns/noop_implementations.h`:162
- Brief: n/a
- Parameters:
  - `name` (std::string_view): n/a

#### `ProbeResult isHealthy() const override`
- Source: `include/core/concerns/noop_implementations.h`:166
- Brief: n/a
- Parameters: none

#### `std::vector< std::string > listSecretNames() const override`
- Source: `include/core/concerns/noop_implementations.h`:163
- Brief: n/a
- Parameters: none

#### `void shutdown() noexcept override`
- Source: `include/core/concerns/noop_implementations.h`:165
- Brief: n/a
- Parameters: none

### themis::core::concerns::NoOpTracer

#### `void flush() noexcept override`
- Source: `include/core/concerns/noop_implementations.h`:97
- Brief: n/a
- Parameters: none

#### `bool initialize(const std::string &serviceName, const std::string &endpoint) override`
- Source: `include/core/concerns/noop_implementations.h`:90
- Brief: n/a
- Parameters:
  - `serviceName` (const std::string &): n/a
  - `endpoint` (const std::string &): n/a

#### `ProbeResult isHealthy() const override`
- Source: `include/core/concerns/noop_implementations.h`:98
- Brief: n/a
- Parameters: none

#### `bool isInitialized() const override`
- Source: `include/core/concerns/noop_implementations.h`:95
- Brief: n/a
- Parameters: none

#### `void shutdown() noexcept override`
- Source: `include/core/concerns/noop_implementations.h`:94
- Brief: Shutdown.
- Parameters: none

#### `std::unique_ptr< ISpan > startChildSpan(const std::string &name, const ISpan &parent) override`
- Source: `include/core/concerns/noop_implementations.h`:86
- Brief: n/a
- Parameters:
  - `name` (const std::string &): n/a
  - `parent` (const ISpan &): n/a

#### `std::unique_ptr< ISpan > startSpan(const std::string &name) override`
- Source: `include/core/concerns/noop_implementations.h`:82
- Brief: n/a
- Parameters:
  - `name` (const std::string &): n/a

### themis::core::concerns::NoOpTracer::NoOpSpan

#### `void end() override`
- Source: `include/core/concerns/noop_implementations.h`:78
- Brief: End.
- Parameters: none

#### `bool isValid() const override`
- Source: `include/core/concerns/noop_implementations.h`:79
- Brief: n/a
- Parameters: none

#### `void recordError(const std::string &errorMessage) override`
- Source: `include/core/concerns/noop_implementations.h`:76
- Brief: Record Error.
- Parameters:
  - `errorMessage` (const std::string &): Input parameter.
- Details: errorMessage Input parameter.

#### `void setAttribute(const std::string &key, bool value) override`
- Source: `include/core/concerns/noop_implementations.h`:75
- Brief: Set Attribute.
- Parameters:
  - `key` (const std::string &): Input parameter.
  - `value` (bool): Input parameter.
- Details: key Input parameter. value Input parameter.

#### `void setAttribute(const std::string &key, const std::string &value) override`
- Source: `include/core/concerns/noop_implementations.h`:72
- Brief: Set Attribute.
- Parameters:
  - `key` (const std::string &): Input parameter.
  - `value` (const std::string &): Input parameter.
- Details: key Input parameter. value Input parameter.

#### `void setAttribute(const std::string &key, double value) override`
- Source: `include/core/concerns/noop_implementations.h`:74
- Brief: Set Attribute.
- Parameters:
  - `key` (const std::string &): Input parameter.
  - `value` (double): Input parameter.
- Details: key Input parameter. value Input parameter.

#### `void setAttribute(const std::string &key, int64_t value) override`
- Source: `include/core/concerns/noop_implementations.h`:73
- Brief: Set Attribute.
- Parameters:
  - `key` (const std::string &): Input parameter.
  - `value` (int64_t): Input parameter.
- Details: key Input parameter. value Input parameter.

#### `void setStatus(bool ok, const std::string &description="") override`
- Source: `include/core/concerns/noop_implementations.h`:77
- Brief: n/a
- Parameters:
  - `ok` (bool): n/a
  - `description` (const std::string &): n/a

### themis::core::concerns::OpenTelemetryTracerAdapter

#### `OpenTelemetryTracerAdapter()`
- Source: `include/core/concerns/otel_tracer_adapter.h`:44
- Brief: n/a
- Parameters: none

#### `OpenTelemetryTracerAdapter(const CircuitBreakerConfig &cb_config)`
- Source: `include/core/concerns/otel_tracer_adapter.h`:34
- Brief: Open Telemetry Tracer Adapter.
- Parameters:
  - `cb_config` (const CircuitBreakerConfig &): Input parameter.
- Return: Return value.
- Details: cb_config Input parameter. Return value.

#### `sharding::CircuitBreaker::State circuitBreakerState() const`
- Source: `include/core/concerns/otel_tracer_adapter.h`:206
- Brief: n/a
- Parameters: none

#### `void flush() noexcept override`
- Source: `include/core/concerns/otel_tracer_adapter.h`:194
- Brief: n/a
- Parameters: none

#### `bool initialize(const std::string &serviceName, const std::string &endpoint) override`
- Source: `include/core/concerns/otel_tracer_adapter.h`:165
- Brief: n/a
- Parameters:
  - `serviceName` (const std::string &): n/a
  - `endpoint` (const std::string &): n/a

#### `void injectContext(std::map< std::string, std::string > &carrier_headers) override`
- Source: `include/core/concerns/otel_tracer_adapter.h`:154
- Brief: n/a
- Parameters:
  - `carrier_headers` (std::map< std::string, std::string > &): n/a

#### `ProbeResult isHealthy() const override`
- Source: `include/core/concerns/otel_tracer_adapter.h`:196
- Brief: n/a
- Parameters: none

#### `bool isInitialized() const override`
- Source: `include/core/concerns/otel_tracer_adapter.h`:189
- Brief: n/a
- Parameters: none

#### `void shutdown() override`
- Source: `include/core/concerns/otel_tracer_adapter.h`:182
- Brief: Shutdown.
- Parameters: none

#### `std::unique_ptr< ISpan > startChildSpan(const std::string &name, const ISpan &parent) override`
- Source: `include/core/concerns/otel_tracer_adapter.h`:116
- Brief: n/a
- Parameters:
  - `name` (const std::string &): n/a
  - `parent` (const ISpan &): n/a

#### `std::unique_ptr< ISpan > startSpan(const std::string &name) override`
- Source: `include/core/concerns/otel_tracer_adapter.h`:102
- Brief: n/a
- Parameters:
  - `name` (const std::string &): n/a

#### `std::unique_ptr< ISpan > startSpanFromHeaders(const std::string &name, const std::map< std::string, std::string > &carrier_headers) override`
- Source: `include/core/concerns/otel_tracer_adapter.h`:138
- Brief: n/a
- Parameters:
  - `name` (const std::string &): n/a
  - `carrier_headers` (const std::map< std::string, std::string > &): n/a

### themis::core::concerns::OpenTelemetryTracerAdapter::OtelSpanAdapter

#### `OtelSpanAdapter(themis::Tracer::Span span)`
- Source: `include/core/concerns/otel_tracer_adapter.h`:53
- Brief: Otel Span Adapter.
- Parameters:
  - `span` (themis::Tracer::Span): Input parameter.
- Return: Return value.
- Details: span Input parameter. Return value.

#### `void end() override`
- Source: `include/core/concerns/otel_tracer_adapter.h`:80
- Brief: End.
- Parameters: none

#### `themis::Tracer::Span & getSpan()`
- Source: `include/core/concerns/otel_tracer_adapter.h`:95
- Brief: Get Span.
- Parameters: none
- Return: Return value.
- Details: Return value. Implements getSpan without additional internal calls.

#### `const themis::Tracer::Span & getSpan() const`
- Source: `include/core/concerns/otel_tracer_adapter.h`:96
- Brief: n/a
- Parameters: none

#### `bool isValid() const override`
- Source: `include/core/concerns/otel_tracer_adapter.h`:84
- Brief: n/a
- Parameters: none

#### `void recordError(const std::string &errorMessage) override`
- Source: `include/core/concerns/otel_tracer_adapter.h`:72
- Brief: Record Error.
- Parameters:
  - `errorMessage` (const std::string &): Input parameter.
- Details: errorMessage Input parameter.

#### `void setAttribute(const std::string &key, bool value) override`
- Source: `include/core/concerns/otel_tracer_adapter.h`:68
- Brief: Set Attribute.
- Parameters:
  - `key` (const std::string &): Input parameter.
  - `value` (bool): Input parameter.
- Details: key Input parameter. value Input parameter.

#### `void setAttribute(const std::string &key, const std::string &value) override`
- Source: `include/core/concerns/otel_tracer_adapter.h`:56
- Brief: Set Attribute.
- Parameters:
  - `key` (const std::string &): Input parameter.
  - `value` (const std::string &): Input parameter.
- Details: key Input parameter. value Input parameter.

#### `void setAttribute(const std::string &key, double value) override`
- Source: `include/core/concerns/otel_tracer_adapter.h`:64
- Brief: Set Attribute.
- Parameters:
  - `key` (const std::string &): Input parameter.
  - `value` (double): Input parameter.
- Details: key Input parameter. value Input parameter.

#### `void setAttribute(const std::string &key, int64_t value) override`
- Source: `include/core/concerns/otel_tracer_adapter.h`:60
- Brief: Set Attribute.
- Parameters:
  - `key` (const std::string &): Input parameter.
  - `value` (int64_t): Input parameter.
- Details: key Input parameter. value Input parameter.

#### `void setStatus(bool ok, const std::string &description="") override`
- Source: `include/core/concerns/otel_tracer_adapter.h`:76
- Brief: n/a
- Parameters:
  - `ok` (bool): n/a
  - `description` (const std::string &): n/a

#### `~OtelSpanAdapter() override`
- Source: `include/core/concerns/otel_tracer_adapter.h`:88
- Brief: n/a
- Parameters: none

### themis::core::concerns::ProbeResult

#### `ProbeResult healthy(const std::string &msg="ok")`
- Source: `include/core/concerns/lifecycle.h`:23
- Brief: n/a
- Parameters:
  - `msg` (const std::string &): n/a

#### `ProbeResult unhealthy(const std::string &msg)`
- Source: `include/core/concerns/lifecycle.h`:33
- Brief: Unhealthy.
- Parameters:
  - `msg` (const std::string &): Input parameter.
- Return: Return value.
- Details: msg Input parameter. Return value. Implements unhealthy without additional internal calls.

### themis::core::concerns::PrometheusMetricsAdapter

#### `PrometheusMetricsAdapter()`
- Source: `include/core/concerns/prometheus_metrics_adapter.h`:35
- Brief: n/a
- Parameters: none

#### `void decrementGauge(const std::string &name, double delta, const Labels &labels={}) override`
- Source: `include/core/concerns/prometheus_metrics_adapter.h`:61
- Brief: n/a
- Parameters:
  - `name` (const std::string &): n/a
  - `delta` (double): n/a
  - `labels` (const Labels &): n/a

#### `std::string exportMetrics() const override`
- Source: `include/core/concerns/prometheus_metrics_adapter.h`:98
- Brief: n/a
- Parameters: none

#### `void flush() noexcept override`
- Source: `include/core/concerns/prometheus_metrics_adapter.h`:116
- Brief: Flush the adapter state.
- Parameters: none
- Details: MetricsCollector is pull-based (Prometheus scrapes); there is no network push to force here, so this is intentionally a no-op.

#### `void incrementCounter(const std::string &name, int64_t value=1, const Labels &labels={}) override`
- Source: `include/core/concerns/prometheus_metrics_adapter.h`:42
- Brief: n/a
- Parameters:
  - `name` (const std::string &): n/a
  - `value` (int64_t): n/a
  - `labels` (const Labels &): n/a

#### `void incrementGauge(const std::string &name, double delta, const Labels &labels={}) override`
- Source: `include/core/concerns/prometheus_metrics_adapter.h`:56
- Brief: n/a
- Parameters:
  - `name` (const std::string &): n/a
  - `delta` (double): n/a
  - `labels` (const Labels &): n/a

#### `ProbeResult isHealthy() const override`
- Source: `include/core/concerns/prometheus_metrics_adapter.h`:135
- Brief: Report whether the in-process collector can be used.
- Parameters: none
- Details: The adapter is healthy as long as the singleton exists in-process.

#### `void observeHistogram(const std::string &name, double value, const Labels &labels={}) override`
- Source: `include/core/concerns/prometheus_metrics_adapter.h`:70
- Brief: n/a
- Parameters:
  - `name` (const std::string &): n/a
  - `value` (double): n/a
  - `labels` (const Labels &): n/a

#### `void recordError(const std::string &operation, const Labels &labels={}) override`
- Source: `include/core/concerns/prometheus_metrics_adapter.h`:84
- Brief: n/a
- Parameters:
  - `operation` (const std::string &): n/a
  - `labels` (const Labels &): n/a

#### `void recordLatency(const std::string &operation, double latencyMs, const Labels &labels={}) override`
- Source: `include/core/concerns/prometheus_metrics_adapter.h`:79
- Brief: n/a
- Parameters:
  - `operation` (const std::string &): n/a
  - `latencyMs` (double): n/a
  - `labels` (const Labels &): n/a

#### `void recordSuccess(const std::string &operation, const Labels &labels={}) override`
- Source: `include/core/concerns/prometheus_metrics_adapter.h`:89
- Brief: n/a
- Parameters:
  - `operation` (const std::string &): n/a
  - `labels` (const Labels &): n/a

#### `void reset() override`
- Source: `include/core/concerns/prometheus_metrics_adapter.h`:102
- Brief: Reset the modification detection flag.
- Parameters: none

#### `void setGauge(const std::string &name, double value, const Labels &labels={}) override`
- Source: `include/core/concerns/prometheus_metrics_adapter.h`:51
- Brief: n/a
- Parameters:
  - `name` (const std::string &): n/a
  - `value` (double): n/a
  - `labels` (const Labels &): n/a

#### `void shutdown() noexcept override`
- Source: `include/core/concerns/prometheus_metrics_adapter.h`:126
- Brief: Reset the in-process collector and release adapter state.
- Parameters: none
- Details: After shutdown(), the collector is reset so subsequent scrapes start from an empty metric set unless the process recreates metrics first.

### themis::core::concerns::RedisCache

#### `RedisCache(RedisCache &&)=delete`
- Source: `include/core/concerns/redis_cache.h`:115
- Brief: n/a
- Parameters:
  - `<unnamed>` (RedisCache &&): n/a

#### `RedisCache(const RedisCache &)=delete`
- Source: `include/core/concerns/redis_cache.h`:113
- Brief: n/a
- Parameters:
  - `<unnamed>` (const RedisCache &): n/a

#### `RedisCache(const RedisCacheConfig &config)`
- Source: `include/core/concerns/redis_cache.h`:190
- Brief: Private constructor – use factory methods.
- Parameters:
  - `config` (const RedisCacheConfig &): Input parameter.
- Return: Return value.
- Details: config Input parameter. Return value.

#### `void buildHashRing()`
- Source: `include/core/concerns/redis_cache.h`:220
- Brief: Build Hash Ring.
- Parameters: none
- Details: Calls: clear(), size(), std::to_string(), fnv1a32(), data().

#### `std::string buildRespCommand(const std::vector< std::string > &args)`
- Source: `include/core/concerns/redis_cache.h`:289
- Brief: Build Resp Command.
- Parameters:
  - `args` (const std::vector< std::string > &): Input parameter.
- Return: Return value.
- Details: static args Input parameter. Return value. args Input parameter. Return value. Calls: std::to_string(), size().

#### `void clear() override`
- Source: `include/core/concerns/redis_cache.h`:129
- Brief: Clear.
- Parameters: none
- Details: Calls: sendCommand(), publishInvalidation().

#### `void closeSocket(SocketFd &fd) noexcept`
- Source: `include/core/concerns/redis_cache.h`:257
- Brief: Close Socket.
- Parameters:
  - `fd` (SocketFd &): Input/output parameter.
- Details: fd Input/output parameter. Exception safety: noexcept.

#### `std::unique_ptr< RedisCache > create(const RedisCacheConfig &config)`
- Source: `include/core/concerns/redis_cache.h`:108
- Brief: Create.
- Parameters:
  - `config` (const RedisCacheConfig &): Input parameter.
- Return: Return value.
- Details: config Input parameter. Return value. config Input parameter. Return value. Calls: RedisCache().

#### `std::unique_ptr< RedisCache > create(const std::string &url)`
- Source: `include/core/concerns/redis_cache.h`:101
- Brief: Create.
- Parameters:
  - `url` (const std::string &): Input parameter.
- Return: Return value.
- Details: url Input parameter. Return value. url Input parameter. Return value. Calls: parseRedisUrl().

#### `std::optional< CacheEntry > decodeEntry(const std::string &raw)`
- Source: `include/core/concerns/redis_cache.h`:344
- Brief: Decode Entry.
- Parameters:
  - `raw` (const std::string &): Input parameter.
- Return: Return value.
- Details: static raw Input parameter. Return value. raw Input parameter. Return value. Calls: find(), std::stoull(), substr().

#### `void dispatchInvalidation(const std::string &payload)`
- Source: `include/core/concerns/redis_cache.h`:381
- Brief: Dispatch Invalidation.
- Parameters:
  - `payload` (const std::string &): Input parameter.
- Details: payload Input parameter. payload Input parameter. Calls: lock(), inv_callback_().

#### `std::string encodeEntry(const CacheEntry &e)`
- Source: `include/core/concerns/redis_cache.h`:337
- Brief: Encode Entry.
- Parameters:
  - `e` (const CacheEntry &): Input parameter.
- Return: Return value.
- Details: static e Input parameter. Return value. e Input parameter. Return value. Calls: std::to_string().

#### `bool ensureConnected(NodeConn &nc) const noexcept`
- Source: `include/core/concerns/redis_cache.h`:297
- Brief: Ensure Connected.
- Parameters:
  - `nc` (NodeConn &): Input/output parameter.
- Return: True when the operation succeeds.
- Details: nc Input/output parameter. True when the operation succeeds. Exception safety: noexcept.

#### `void ensureSubscriberLoopStarted()`
- Source: `include/core/concerns/redis_cache.h`:355
- Brief: Ensure Subscriber Loop Started.
- Parameters: none
- Details: Calls: load(), empty(), lock(), joinable(), std::thread().

#### `void flush() noexcept override`
- Source: `include/core/concerns/redis_cache.h`:145
- Brief: n/a
- Parameters: none

#### `uint32_t fnv1a32(const char *data, size_t len) noexcept`
- Source: `include/core/concerns/redis_cache.h`:215
- Brief: Fnv1a32.
- Parameters:
  - `data` (const char *): Input parameter.
  - `len` (size_t): Input parameter.
- Return: Return value.
- Details: data Input parameter. len Input parameter. Return value. Exception safety: noexcept.

#### `std::optional< CacheEntry > get(std::string_view key) const override`
- Source: `include/core/concerns/redis_cache.h`:122
- Brief: n/a
- Parameters:
  - `key` (std::string_view): n/a

#### `size_t hashRingSize() const`
- Source: `include/core/concerns/redis_cache.h`:180
- Brief: Hash Ring Size.
- Parameters: none
- Return: Return value.
- Details: Return value.

#### `uint64_t hitCount() const override`
- Source: `include/core/concerns/redis_cache.h`:135
- Brief: n/a
- Parameters: none

#### `double hitRate() const override`
- Source: `include/core/concerns/redis_cache.h`:139
- Brief: n/a
- Parameters: none

#### `void invalidate(std::string_view key) override`
- Source: `include/core/concerns/redis_cache.h`:127
- Brief: Invalidate.
- Parameters:
  - `key` (std::string_view): Input parameter.
- Details: key Input parameter. Calls: empty(), nodeIndexForKey(), std::string(), sendCommand(), publishInvalidation().

#### `void invalidatePattern(std::string_view pattern) override`
- Source: `include/core/concerns/redis_cache.h`:131
- Brief: Invalidate Pattern.
- Parameters:
  - `pattern` (std::string_view): Input parameter.
- Details: pattern Input parameter. Calls: std::string(), lock(), ensureConnected(), buildRespCommand(), sendAll(), closeSocket(), readLine(), empty().

#### `bool isConnected() const`
- Source: `include/core/concerns/redis_cache.h`:167
- Brief: Is Connected.
- Parameters: none
- Return: True when the operation succeeds.
- Details: True when the operation succeeds.

#### `ProbeResult isHealthy() const override`
- Source: `include/core/concerns/redis_cache.h`:149
- Brief: n/a
- Parameters: none

#### `uint64_t missCount() const override`
- Source: `include/core/concerns/redis_cache.h`:137
- Brief: n/a
- Parameters: none

#### `size_t nodeCount() const`
- Source: `include/core/concerns/redis_cache.h`:182
- Brief: n/a
- Parameters: none

#### `std::string nodeForKey(std::string_view key) const`
- Source: `include/core/concerns/redis_cache.h`:174
- Brief: Node For Key.
- Parameters:
  - `key` (std::string_view): Input parameter.
- Return: Return value.
- Details: key Input parameter. Return value.

#### `size_t nodeIndexForKey(std::string_view key) const`
- Source: `include/core/concerns/redis_cache.h`:227
- Brief: Node Index For Key.
- Parameters:
  - `key` (std::string_view): Input parameter.
- Return: Return value.
- Details: key Input parameter. Return value.

#### `RedisCache & operator=(RedisCache &&)=delete`
- Source: `include/core/concerns/redis_cache.h`:116
- Brief: n/a
- Parameters:
  - `<unnamed>` (RedisCache &&): n/a

#### `RedisCache & operator=(const RedisCache &)=delete`
- Source: `include/core/concerns/redis_cache.h`:114
- Brief: n/a
- Parameters:
  - `<unnamed>` (const RedisCache &): n/a

#### `void publishInvalidation(const std::string &key_or_pattern)`
- Source: `include/core/concerns/redis_cache.h`:351
- Brief: -------------------------------------------------------------------- Pub/sub subscriber (background thread) --------------------------------------------------------------------
- Parameters:
  - `key_or_pattern` (const std::string &): Input parameter.
- Details: Publish Invalidation. key_or_pattern Input parameter. key_or_pattern Input parameter. Calls: empty(), sendCommand().

#### `bool put(std::string_view key, const CacheEntry &entry, uint64_t ttl_ms=0) override`
- Source: `include/core/concerns/redis_cache.h`:124
- Brief: Put.
- Parameters:
  - `key` (std::string_view): Input parameter.
  - `entry` (const CacheEntry &): Input parameter.
  - `ttl_ms` (uint64_t): Input parameter.
- Return: True when the operation succeeds.
- Details: key Input parameter. entry Input parameter. ttl_ms Input parameter. True when the operation succeeds. Calls: empty(), nodeIndexForKey(), std::string(), encodeEntry(), load(), sendCommand(), std::to_string(), has_value().

#### `bool readLine(SocketFd fd, std::string &out) noexcept`
- Source: `include/core/concerns/redis_cache.h`:274
- Brief: Read Line.
- Parameters:
  - `fd` (SocketFd): Input parameter.
  - `out` (std::string &): Input/output parameter.
- Return: True when the operation succeeds.
- Details: fd Input parameter. out Input/output parameter. True when the operation succeeds. Exception safety: noexcept.

#### `bool readPubSubMessage(SocketFd fd, std::string &channel_out, std::string &payload_out) noexcept`
- Source: `include/core/concerns/redis_cache.h`:374
- Brief: Read Pub Sub Message.
- Parameters:
  - `fd` (SocketFd): Input parameter.
  - `channel_out` (std::string &): Input/output parameter.
  - `payload_out` (std::string &): Input/output parameter.
- Return: True when the operation succeeds.
- Details: fd Input parameter. channel_out Input/output parameter. payload_out Input/output parameter. True when the operation succeeds. Exception safety: noexcept.

#### `bool readReply(SocketFd fd, std::string &out) noexcept`
- Source: `include/core/concerns/redis_cache.h`:326
- Brief: Read Reply.
- Parameters:
  - `fd` (SocketFd): Input parameter.
  - `out` (std::string &): Input/output parameter.
- Return: True when the operation succeeds.
- Details: fd Input parameter. out Input/output parameter. True when the operation succeeds. Exception safety: noexcept.

#### `bool redisHandshake(SocketFd fd) const noexcept`
- Source: `include/core/concerns/redis_cache.h`:282
- Brief: Redis Handshake.
- Parameters:
  - `fd` (SocketFd): Input parameter.
- Return: True when the operation succeeds.
- Details: fd Input parameter. True when the operation succeeds. Exception safety: noexcept.

#### `bool sendAll(SocketFd fd, const std::string &buf) noexcept`
- Source: `include/core/concerns/redis_cache.h`:265
- Brief: Send All.
- Parameters:
  - `fd` (SocketFd): Input parameter.
  - `buf` (const std::string &): Input parameter.
- Return: True when the operation succeeds.
- Details: fd Input parameter. buf Input parameter. True when the operation succeeds. Exception safety: noexcept.

#### `std::optional< std::string > sendCommand(NodeConn &nc, const std::vector< std::string > &args) const noexcept`
- Source: `include/core/concerns/redis_cache.h`:306
- Brief: Send Command.
- Parameters:
  - `nc` (NodeConn &): Input/output parameter.
  - `args` (const std::vector< std::string > &): Input parameter.
- Return: Return value.
- Details: nc Input/output parameter. args Input parameter. Return value. Exception safety: noexcept.

#### `std::optional< std::string > sendCommandLocked(NodeConn &nc, const std::vector< std::string > &args) const noexcept`
- Source: `include/core/concerns/redis_cache.h`:316
- Brief: Send Command Locked.
- Parameters:
  - `nc` (NodeConn &): Input/output parameter.
  - `args` (const std::vector< std::string > &): Input parameter.
- Return: Return value.
- Details: nc Input/output parameter. args Input parameter. Return value. Exception safety: noexcept.

#### `void setDefaultTTL(uint64_t ttl_ms) override`
- Source: `include/core/concerns/redis_cache.h`:143
- Brief: Set Default TTL.
- Parameters:
  - `ttl_ms` (uint64_t): Input parameter.
- Details: ttl_ms Input parameter. Calls: store().

#### `void setMaxSize(size_t maxSize) override`
- Source: `include/core/concerns/redis_cache.h`:141
- Brief: Set Max Size.
- Parameters:
  - `maxSize` (size_t): Input parameter.
- Details: maxSize Input parameter. Calls: store().

#### `void shutdown() noexcept override`
- Source: `include/core/concerns/redis_cache.h`:147
- Brief: n/a
- Parameters: none

#### `size_t size() const override`
- Source: `include/core/concerns/redis_cache.h`:133
- Brief: n/a
- Parameters: none

#### `void subscribeInvalidations(InvalidationCallback cb)`
- Source: `include/core/concerns/redis_cache.h`:161
- Brief: Subscribe Invalidations.
- Parameters:
  - `cb` (InvalidationCallback): Input parameter.
- Details: cb Input parameter. cb Input parameter. Calls: lock(), std::move(), ensureSubscriberLoopStarted().

#### `void subscriberLoop()`
- Source: `include/core/concerns/redis_cache.h`:360
- Brief: Subscriber Loop.
- Parameters: none
- Details: Calls: std::max(), lk(), wait_for(), std::chrono::milliseconds(), load(), empty(), sleepWithStop(), tcpConnect().

#### `void subscriberSession(SocketFd fd)`
- Source: `include/core/concerns/redis_cache.h`:365
- Brief: Subscriber Session.
- Parameters:
  - `fd` (SocketFd): Input parameter.
- Details: fd Input parameter. fd Input parameter. Calls: readPubSubMessage(), load(), clear(), empty(), dispatchInvalidation().

#### `SocketFd tcpConnect(const std::string &host, uint16_t port) const`
- Source: `include/core/concerns/redis_cache.h`:251
- Brief: Tcp Connect.
- Parameters:
  - `host` (const std::string &): Input parameter.
  - `port` (uint16_t): Input parameter.
- Return: Return value.
- Details: host Input parameter. port Input parameter. Return value.

#### `~RedisCache() override`
- Source: `include/core/concerns/redis_cache.h`:110
- Brief: n/a
- Parameters: none

### themis::core::concerns::ScopedSpan

#### `ScopedSpan(ITracer &tracer, const std::string &name)`
- Source: `include/core/concerns/i_tracer.h`:141
- Brief: Scoped Span.
- Parameters:
  - `tracer` (ITracer &): Input/output parameter.
  - `name` (const std::string &): Input parameter.
- Return: Return value.
- Details: tracer Input/output parameter. name Input parameter. Return value.

#### `void recordError(const std::string &errorMessage)`
- Source: `include/core/concerns/i_tracer.h`:197
- Brief: Record Error.
- Parameters:
  - `errorMessage` (const std::string &): Input parameter.
- Details: errorMessage Input parameter. Implements recordError without additional internal calls.

#### `void setAttribute(const std::string &key, bool value)`
- Source: `include/core/concerns/i_tracer.h`:186
- Brief: Set Attribute.
- Parameters:
  - `key` (const std::string &): Input parameter.
  - `value` (bool): Input parameter.
- Details: key Input parameter. value Input parameter. Implements setAttribute without additional internal calls.

#### `void setAttribute(const std::string &key, const std::string &value)`
- Source: `include/core/concerns/i_tracer.h`:150
- Brief: Set Attribute.
- Parameters:
  - `key` (const std::string &): Input parameter.
  - `value` (const std::string &): Input parameter.
- Details: key Input parameter. value Input parameter. Implements setAttribute without additional internal calls.

#### `void setAttribute(const std::string &key, double value)`
- Source: `include/core/concerns/i_tracer.h`:174
- Brief: Set Attribute.
- Parameters:
  - `key` (const std::string &): Input parameter.
  - `value` (double): Input parameter.
- Details: key Input parameter. value Input parameter. Implements setAttribute without additional internal calls.

#### `void setAttribute(const std::string &key, int64_t value)`
- Source: `include/core/concerns/i_tracer.h`:162
- Brief: Set Attribute.
- Parameters:
  - `key` (const std::string &): Input parameter.
  - `value` (int64_t): Input parameter.
- Details: key Input parameter. value Input parameter. Implements setAttribute without additional internal calls.

#### `void setStatus(bool ok, const std::string &description="")`
- Source: `include/core/concerns/i_tracer.h`:203
- Brief: n/a
- Parameters:
  - `ok` (bool): n/a
  - `description` (const std::string &): n/a

#### `ITracer::ISpan * span()`
- Source: `include/core/concerns/i_tracer.h`:214
- Brief: Span.
- Parameters: none
- Return: Pointer to the result.
- Details: Pointer to the result. Calls: get().

#### `~ScopedSpan()`
- Source: `include/core/concerns/i_tracer.h`:216
- Brief: n/a
- Parameters: none

### themis::core::concerns::SignedAdapterValidator

#### `SignedAdapterValidator(AdapterSignature expected_sig)`
- Source: `include/core/concerns/adapter_signing.h`:64
- Brief: Signed Adapter Validator.
- Parameters:
  - `expected_sig` (AdapterSignature): Input parameter.
- Return: Return value.
- Details: expected_sig Input parameter. Return value.

#### `std::string canonicalString(const AdapterMetadata &m)`
- Source: `include/core/concerns/adapter_signing.h`:72
- Brief: Canonical String.
- Parameters:
  - `m` (const AdapterMetadata &): Input parameter.
- Return: Return value.
- Details: m Input parameter. Return value. Calls: reserve(), size(), std::to_string().

#### `std::string sha256Hex(std::string_view data)`
- Source: `include/core/concerns/adapter_signing.h`:74
- Brief: Sha256 Hex.
- Parameters:
  - `data` (std::string_view): Input parameter.
- Return: Return value.
- Details: data Input parameter. Return value. Calls: EVP_MD_CTX_new(), EVP_DigestInit_ex(), EVP_sha256(), EVP_MD_CTX_free(), EVP_DigestUpdate(), data(), size(), EVP_DigestFinal_ex().

#### `bool validate(const AdapterMetadata &m) override`
- Source: `include/core/concerns/adapter_signing.h`:66
- Brief: Validate.
- Parameters:
  - `m` (const AdapterMetadata &): Input parameter.
- Return: True when the operation succeeds.
- Details: m Input parameter. True when the operation succeeds. Calls: present(), canonicalString(), sha256Hex(), empty(), size(), CRYPTO_memcmp(), data().

### themis::core::concerns::SimpleContext

#### `SimpleContext(std::shared_ptr< SimpleContext > parent)`
- Source: `include/core/concerns/i_context.h`:181
- Brief: Simple Context.
- Parameters:
  - `parent` (std::shared_ptr< SimpleContext >): Input parameter.
- Return: Return value.
- Details: parent Input parameter. Return value.

#### `std::shared_ptr< SimpleContext > create()`
- Source: `include/core/concerns/i_context.h`:95
- Brief: Create.
- Parameters: none
- Return: Return value.
- Details: Return value. Calls: SimpleContext().

#### `std::shared_ptr< SimpleContext > create(std::string_view trace_id, std::string_view request_id)`
- Source: `include/core/concerns/i_context.h`:105
- Brief: Create.
- Parameters:
  - `trace_id` (std::string_view): Identifier of the trace.
  - `request_id` (std::string_view): Identifier of the request.
- Return: Return value.
- Details: trace_id Identifier of the trace. request_id Identifier of the request. Return value.

#### `IContextPtr createChild() const override`
- Source: `include/core/concerns/i_context.h`:154
- Brief: n/a
- Parameters: none

#### `std::optional< std::string > get(std::string_view key) const override`
- Source: `include/core/concerns/i_context.h`:130
- Brief: n/a
- Parameters:
  - `key` (std::string_view): n/a

#### `bool has(std::string_view key) const override`
- Source: `include/core/concerns/i_context.h`:150
- Brief: n/a
- Parameters:
  - `key` (std::string_view): n/a

#### `void set(std::string_view key, std::string_view value) override`
- Source: `include/core/concerns/i_context.h`:120
- Brief: Set.
- Parameters:
  - `key` (std::string_view): Input parameter.
  - `value` (std::string_view): Input parameter.
- Details: key Input parameter. value Input parameter.

#### `TraceContext toTraceContext() const override`
- Source: `include/core/concerns/i_context.h`:161
- Brief: n/a
- Parameters: none

### themis::core::concerns::SpdlogLoggerAdapter

#### `SpdlogLoggerAdapter(std::shared_ptr< spdlog::logger > logger=nullptr, bool json_mode=false)`
- Source: `include/core/concerns/spdlog_logger_adapter.h`:30
- Brief: n/a
- Parameters:
  - `logger` (std::shared_ptr< spdlog::logger >): n/a
  - `json_mode` (bool): n/a

#### `std::string buildJsonLine(Level level, const std::string &message, const Fields &fields) const`
- Source: `include/core/concerns/spdlog_logger_adapter.h`:302
- Brief: n/a
- Parameters:
  - `level` (Level): n/a
  - `message` (const std::string &): n/a
  - `fields` (const Fields &): n/a

#### `void critical(const std::string &message) override`
- Source: `include/core/concerns/spdlog_logger_adapter.h`:106
- Brief: Critical.
- Parameters:
  - `message` (const std::string &): Input parameter.
- Details: message Input parameter.

#### `void debug(const std::string &message) override`
- Source: `include/core/concerns/spdlog_logger_adapter.h`:82
- Brief: Debug.
- Parameters:
  - `message` (const std::string &): Input parameter.
- Details: message Input parameter.

#### `void error(const std::string &message) override`
- Source: `include/core/concerns/spdlog_logger_adapter.h`:100
- Brief: Error.
- Parameters:
  - `message` (const std::string &): Input parameter.
- Details: message Input parameter.

#### `void flush() noexcept override`
- Source: `include/core/concerns/spdlog_logger_adapter.h`:205
- Brief: n/a
- Parameters: none

#### `Level getLevel() const override`
- Source: `include/core/concerns/spdlog_logger_adapter.h`:183
- Brief: n/a
- Parameters: none

#### `void info(const std::string &message) override`
- Source: `include/core/concerns/spdlog_logger_adapter.h`:88
- Brief: Info.
- Parameters:
  - `message` (const std::string &): Input parameter.
- Details: message Input parameter.

#### `ProbeResult isHealthy() const override`
- Source: `include/core/concerns/spdlog_logger_adapter.h`:218
- Brief: n/a
- Parameters: none

#### `std::string jsonEscape(const std::string &s)`
- Source: `include/core/concerns/spdlog_logger_adapter.h`:262
- Brief: Json Escape.
- Parameters:
  - `s` (const std::string &): Input parameter.
- Return: Return value.
- Details: s Input parameter. Return value. Calls: reserve(), size(), std::snprintf().

#### `bool jsonMode() const`
- Source: `include/core/concerns/spdlog_logger_adapter.h`:232
- Brief: n/a
- Parameters: none

#### `void log(Level level, const std::string &message) override`
- Source: `include/core/concerns/spdlog_logger_adapter.h`:35
- Brief: Log.
- Parameters:
  - `level` (Level): Input parameter.
  - `message` (const std::string &): Input parameter.
- Details: level Input parameter. message Input parameter.

#### `void logStructured(Level level, const std::string &message, const Fields &fields={}) override`
- Source: `include/core/concerns/spdlog_logger_adapter.h`:112
- Brief: n/a
- Parameters:
  - `level` (Level): n/a
  - `message` (const std::string &): n/a
  - `fields` (const Fields &): n/a

#### `void logWithContext(Level level, const std::string &message, const TraceContext &ctx, const Fields &fields={}) override`
- Source: `include/core/concerns/spdlog_logger_adapter.h`:133
- Brief: n/a
- Parameters:
  - `level` (Level): n/a
  - `message` (const std::string &): n/a
  - `ctx` (const TraceContext &): n/a
  - `fields` (const Fields &): n/a

#### `std::string redact(const std::string &key, const std::string &value)`
- Source: `include/core/concerns/spdlog_logger_adapter.h`:292
- Brief: Redact.
- Parameters:
  - `key` (const std::string &): Input parameter.
  - `value` (const std::string &): Input parameter.
- Return: Return value.
- Details: key Input parameter. value Input parameter. Return value. Calls: pii_re(), std::regex_search().

#### `void setJsonMode(bool enabled)`
- Source: `include/core/concerns/spdlog_logger_adapter.h`:230
- Brief: Set Json Mode.
- Parameters:
  - `enabled` (bool): Input parameter.
- Details: enabled Input parameter. Implements setJsonMode without additional internal calls.

#### `void setLevel(Level level) override`
- Source: `include/core/concerns/spdlog_logger_adapter.h`:176
- Brief: Set Level.
- Parameters:
  - `level` (Level): Input parameter.
- Details: level Input parameter.

#### `void setPattern(const std::string &pattern) override`
- Source: `include/core/concerns/spdlog_logger_adapter.h`:198
- Brief: Set Pattern.
- Parameters:
  - `pattern` (const std::string &): Input parameter.
- Details: pattern Input parameter.

#### `void shutdown() noexcept override`
- Source: `include/core/concerns/spdlog_logger_adapter.h`:211
- Brief: n/a
- Parameters: none

#### `spdlog::level::level_enum toSpdlogLevel(Level level)`
- Source: `include/core/concerns/spdlog_logger_adapter.h`:244
- Brief: To Spdlog Level.
- Parameters:
  - `level` (Level): Input parameter.
- Return: Return value.
- Details: level Input parameter. Return value. Implements toSpdlogLevel without additional internal calls.

#### `void trace(const std::string &message) override`
- Source: `include/core/concerns/spdlog_logger_adapter.h`:76
- Brief: Trace.
- Parameters:
  - `message` (const std::string &): Input parameter.
- Details: message Input parameter.

#### `void warn(const std::string &message) override`
- Source: `include/core/concerns/spdlog_logger_adapter.h`:94
- Brief: Warn.
- Parameters:
  - `message` (const std::string &): Input parameter.
- Details: message Input parameter.

### themis::core::concerns::StrategicCacheImpl

#### `StrategicCacheImpl(size_t maxSize=1000, std::unique_ptr< IEvictionStrategy > strategy=nullptr, uint64_t defaultTTL=0)`
- Source: `include/core/concerns/strategic_cache_impl.h`:28
- Brief: n/a
- Parameters:
  - `maxSize` (size_t): n/a
  - `strategy` (std::unique_ptr< IEvictionStrategy >): n/a
  - `defaultTTL` (uint64_t): n/a

#### `void clear() override`
- Source: `include/core/concerns/strategic_cache_impl.h`:138
- Brief: Clear.
- Parameters: none

#### `std::optional< CacheEntry > get(std::string_view key) const override`
- Source: `include/core/concerns/strategic_cache_impl.h`:39
- Brief: n/a
- Parameters:
  - `key` (std::string_view): n/a

#### `uint64_t getCurrentTimeMs() const`
- Source: `include/core/concerns/strategic_cache_impl.h`:260
- Brief: n/a
- Parameters: none

#### `const IEvictionStrategy * getEvictionStrategy() const override`
- Source: `include/core/concerns/strategic_cache_impl.h`:218
- Brief: n/a
- Parameters: none

#### `IEvictionStrategy * getEvictionStrategy() override`
- Source: `include/core/concerns/strategic_cache_impl.h`:215
- Brief: Get Eviction Strategy.
- Parameters: none
- Return: Pointer to the result.
- Details: Pointer to the result. Implements getEvictionStrategy without additional internal calls.

#### `const CacheMetrics * getMetrics() const override`
- Source: `include/core/concerns/strategic_cache_impl.h`:223
- Brief: n/a
- Parameters: none

#### `uint64_t hitCount() const override`
- Source: `include/core/concerns/strategic_cache_impl.h`:186
- Brief: n/a
- Parameters: none

#### `double hitRate() const override`
- Source: `include/core/concerns/strategic_cache_impl.h`:194
- Brief: n/a
- Parameters: none

#### `void invalidate(std::string_view key) override`
- Source: `include/core/concerns/strategic_cache_impl.h`:122
- Brief: Invalidate.
- Parameters:
  - `key` (std::string_view): Input parameter.
- Details: key Input parameter.

#### `void invalidatePattern(std::string_view pattern) override`
- Source: `include/core/concerns/strategic_cache_impl.h`:151
- Brief: Invalidate Pattern.
- Parameters:
  - `pattern` (std::string_view): Input parameter.
- Details: pattern Input parameter.

#### `uint64_t missCount() const override`
- Source: `include/core/concerns/strategic_cache_impl.h`:190
- Brief: n/a
- Parameters: none

#### `bool put(std::string_view key, const CacheEntry &entry, uint64_t ttl_ms=0) override`
- Source: `include/core/concerns/strategic_cache_impl.h`:77
- Brief: n/a
- Parameters:
  - `key` (std::string_view): n/a
  - `entry` (const CacheEntry &): n/a
  - `ttl_ms` (uint64_t): n/a

#### `void setDefaultTTL(uint64_t ttl_ms) override`
- Source: `include/core/concerns/strategic_cache_impl.h`:210
- Brief: Set Default TTL.
- Parameters:
  - `ttl_ms` (uint64_t): Input parameter.
- Details: ttl_ms Input parameter.

#### `void setEvictionStrategy(std::unique_ptr< IEvictionStrategy > strategy)`
- Source: `include/core/concerns/strategic_cache_impl.h`:232
- Brief: Set Eviction Strategy.
- Parameters:
  - `strategy` (std::unique_ptr< IEvictionStrategy >): Input parameter.
- Details: strategy Input parameter. Calls: lock(), clear(), std::move(), onInsert().

#### `void setMaxSize(size_t maxSize) override`
- Source: `include/core/concerns/strategic_cache_impl.h`:199
- Brief: Set Max Size.
- Parameters:
  - `maxSize` (size_t): Input parameter.
- Details: maxSize Input parameter.

#### `size_t size() const override`
- Source: `include/core/concerns/strategic_cache_impl.h`:176
- Brief: n/a
- Parameters: none

#### `void updateLatency(const std::chrono::steady_clock::time_point &start) const`
- Source: `include/core/concerns/strategic_cache_impl.h`:266
- Brief: n/a
- Parameters:
  - `start` (const std::chrono::steady_clock::time_point &): n/a

### themis::core::concerns::TTLEvictionStrategy

#### `TTLEvictionStrategy(uint64_t default_ttl_ms=3600000)`
- Source: `include/core/concerns/eviction_strategies.h`:165
- Brief: n/a
- Parameters:
  - `default_ttl_ms` (uint64_t): n/a

#### `void clear() override`
- Source: `include/core/concerns/eviction_strategies.h`:205
- Brief: Clear.
- Parameters: none

#### `uint64_t getCurrentTimeMs() const`
- Source: `include/core/concerns/eviction_strategies.h`:230
- Brief: n/a
- Parameters: none

#### `std::string_view getName() const override`
- Source: `include/core/concerns/eviction_strategies.h`:213
- Brief: Get Name.
- Parameters: none
- Return: Return value.
- Details: Return value.

#### `void onAccess(std::string_view key) override`
- Source: `include/core/concerns/eviction_strategies.h`:168
- Brief: On Access.
- Parameters:
  - `key` (std::string_view): Input parameter.
- Details: key Input parameter.

#### `void onInsert(std::string_view key, uint64_t timestamp_ms) override`
- Source: `include/core/concerns/eviction_strategies.h`:172
- Brief: On Insert.
- Parameters:
  - `key` (std::string_view): Input parameter.
  - `timestamp_ms` (uint64_t): Input parameter.
- Details: key Input parameter. timestamp_ms Input parameter.

#### `void onRemove(std::string_view key) override`
- Source: `include/core/concerns/eviction_strategies.h`:176
- Brief: On Remove.
- Parameters:
  - `key` (std::string_view): Input parameter.
- Details: key Input parameter.

#### `std::optional< std::string > selectVictim() override`
- Source: `include/core/concerns/eviction_strategies.h`:180
- Brief: Select Victim.
- Parameters: none
- Return: Return value.
- Details: Return value.

#### `void setDefaultTTL(uint64_t ttl_ms)`
- Source: `include/core/concerns/eviction_strategies.h`:222
- Brief: Set Default TTL.
- Parameters:
  - `ttl_ms` (uint64_t): Input parameter.
- Details: ttl_ms Input parameter. Implements setDefaultTTL without additional internal calls.

#### `size_t size() const override`
- Source: `include/core/concerns/eviction_strategies.h`:209
- Brief: Size.
- Parameters: none
- Return: Return value.
- Details: Return value.

### themis::core::concerns::TraceContext

#### `bool empty() const noexcept`
- Source: `include/core/concerns/i_logger.h`:32
- Brief: n/a
- Parameters: none

### themis::core::concerns::TwoTierEvictionStrategy

#### `TwoTierEvictionStrategy(std::unique_ptr< IEvictionStrategy > l1_strategy, std::unique_ptr< IEvictionStrategy > l2_strategy, size_t l1_capacity)`
- Source: `include/core/concerns/eviction_strategies.h`:239
- Brief: n/a
- Parameters:
  - `l1_strategy` (std::unique_ptr< IEvictionStrategy >): n/a
  - `l2_strategy` (std::unique_ptr< IEvictionStrategy >): n/a
  - `l1_capacity` (size_t): n/a

#### `void clear() override`
- Source: `include/core/concerns/eviction_strategies.h`:277
- Brief: Clear.
- Parameters: none

#### `std::string_view getName() const override`
- Source: `include/core/concerns/eviction_strategies.h`:286
- Brief: Get Name.
- Parameters: none
- Return: Return value.
- Details: Return value.

#### `void onAccess(std::string_view key) override`
- Source: `include/core/concerns/eviction_strategies.h`:247
- Brief: On Access.
- Parameters:
  - `key` (std::string_view): Input parameter.
- Details: key Input parameter.

#### `void onInsert(std::string_view key, uint64_t timestamp_ms) override`
- Source: `include/core/concerns/eviction_strategies.h`:252
- Brief: On Insert.
- Parameters:
  - `key` (std::string_view): Input parameter.
  - `timestamp_ms` (uint64_t): Input parameter.
- Details: key Input parameter. timestamp_ms Input parameter.

#### `void onRemove(std::string_view key) override`
- Source: `include/core/concerns/eviction_strategies.h`:261
- Brief: On Remove.
- Parameters:
  - `key` (std::string_view): Input parameter.
- Details: key Input parameter.

#### `std::optional< std::string > selectVictim() override`
- Source: `include/core/concerns/eviction_strategies.h`:266
- Brief: Select Victim.
- Parameters: none
- Return: Return value.
- Details: Return value.

#### `size_t size() const override`
- Source: `include/core/concerns/eviction_strategies.h`:282
- Brief: Size.
- Parameters: none
- Return: Return value.
- Details: Return value.

### themis::core::concerns::W3CTraceContextPropagator

#### `W3CTraceContextPropagator()=delete`
- Source: `include/core/concerns/w3c_trace_context_propagator.h`:177
- Brief: n/a
- Parameters: none

#### `IContextPtr extract(const std::map< std::string, std::string > &headers, IContextPtr parent=nullptr)`
- Source: `include/core/concerns/w3c_trace_context_propagator.h`:26
- Brief: n/a
- Parameters:
  - `headers` (const std::map< std::string, std::string > &): n/a
  - `parent` (IContextPtr): n/a

#### `bool fromHexDigit(char c, uint8_t &out)`
- Source: `include/core/concerns/w3c_trace_context_propagator.h`:169
- Brief: From Hex Digit.
- Parameters:
  - `c` (char): Input parameter.
  - `out` (uint8_t &): Input/output parameter.
- Return: True when the operation succeeds.
- Details: c Input parameter. out Input/output parameter. True when the operation succeeds. Implements fromHexDigit without additional internal calls.

#### `std::string headerValueCI(const std::map< std::string, std::string > &headers, std::string_view name)`
- Source: `include/core/concerns/w3c_trace_context_propagator.h`:71
- Brief: n/a
- Parameters:
  - `headers` (const std::map< std::string, std::string > &): n/a
  - `name` (std::string_view): n/a

#### `void inject(const IContext &ctx, std::map< std::string, std::string > &headers)`
- Source: `include/core/concerns/w3c_trace_context_propagator.h`:49
- Brief: n/a
- Parameters:
  - `ctx` (const IContext &): n/a
  - `headers` (std::map< std::string, std::string > &): n/a

#### `bool isValidHex(const std::string &s, std::size_t expected_len)`
- Source: `include/core/concerns/w3c_trace_context_propagator.h`:101
- Brief: Is Valid Hex.
- Parameters:
  - `s` (const std::string &): Input parameter.
  - `expected_len` (std::size_t): Input parameter.
- Return: True when the operation succeeds.
- Details: s Input parameter. expected_len Input parameter. True when the operation succeeds. Calls: size(), std::isxdigit().

#### `bool parseTraceparent(const std::string &value, std::string &trace_id, std::string &parent_id)`
- Source: `include/core/concerns/w3c_trace_context_propagator.h`:125
- Brief: Parse Traceparent.
- Parameters:
  - `value` (const std::string &): Input parameter.
  - `trace_id` (std::string &): Identifier of the trace.
  - `parent_id` (std::string &): Identifier of the parent.
- Return: True when the operation succeeds.
- Details: value Input parameter. trace_id Identifier of the trace. parent_id Identifier of the parent. True when the operation succeeds. Calls: size(), fromHexDigit(), substr(), isValidHex(), std::move().

### themis::core::concerns::ZeroCopyLogger

#### `ZeroCopyLogger(std::shared_ptr< spdlog::logger > logger=nullptr, bool json_mode=false, std::size_t buffer_capacity=kDefaultBufferCapacity)`
- Source: `include/core/concerns/zero_copy_logger.h`:30
- Brief: n/a
- Parameters:
  - `logger` (std::shared_ptr< spdlog::logger >): n/a
  - `json_mode` (bool): n/a
  - `buffer_capacity` (std::size_t): n/a

#### `std::size_t bufferCapacity() const noexcept`
- Source: `include/core/concerns/zero_copy_logger.h`:137
- Brief: n/a
- Parameters: none

#### `void buildJsonInto(std::string &buf, Level level, std::string_view message, std::initializer_list< std::pair< std::string_view, std::string_view > > fields) const`
- Source: `include/core/concerns/zero_copy_logger.h`:174
- Brief: n/a
- Parameters:
  - `buf` (std::string &): n/a
  - `level` (Level): n/a
  - `message` (std::string_view): n/a
  - `fields` (std::initializer_list< std::pair< std::string_view, std::string_view > >): n/a

#### `void buildPlainStructuredInto(std::string &buf, std::string_view message, std::initializer_list< std::pair< std::string_view, std::string_view > > fields)`
- Source: `include/core/concerns/zero_copy_logger.h`:180
- Brief: n/a
- Parameters:
  - `buf` (std::string &): n/a
  - `message` (std::string_view): n/a
  - `fields` (std::initializer_list< std::pair< std::string_view, std::string_view > >): n/a

#### `void critical(const std::string &message) override`
- Source: `include/core/concerns/zero_copy_logger.h`:108
- Brief: Critical.
- Parameters:
  - `message` (const std::string &): Input parameter.
- Details: message Input parameter.

#### `void criticalSV(std::string_view message) noexcept`
- Source: `include/core/concerns/zero_copy_logger.h`:91
- Brief: Critical SV.
- Parameters:
  - `message` (std::string_view): Input parameter.
- Details: message Input parameter. Exception safety: noexcept.

#### `void debug(const std::string &message) override`
- Source: `include/core/concerns/zero_copy_logger.h`:104
- Brief: Debug.
- Parameters:
  - `message` (const std::string &): Input parameter.
- Details: message Input parameter.

#### `void debugSV(std::string_view message) noexcept`
- Source: `include/core/concerns/zero_copy_logger.h`:63
- Brief: Debug SV.
- Parameters:
  - `message` (std::string_view): Input parameter.
- Details: message Input parameter. Exception safety: noexcept.

#### `void error(const std::string &message) override`
- Source: `include/core/concerns/zero_copy_logger.h`:107
- Brief: Error.
- Parameters:
  - `message` (const std::string &): Input parameter.
- Details: message Input parameter.

#### `void errorSV(std::string_view message) noexcept`
- Source: `include/core/concerns/zero_copy_logger.h`:84
- Brief: Error SV.
- Parameters:
  - `message` (std::string_view): Input parameter.
- Details: message Input parameter. Exception safety: noexcept.

#### `void flush() noexcept override`
- Source: `include/core/concerns/zero_copy_logger.h`:126
- Brief: n/a
- Parameters: none

#### `std::string & formatBuffer() const noexcept`
- Source: `include/core/concerns/zero_copy_logger.h`:157
- Brief: Format Buffer.
- Parameters: none
- Return: Return value.
- Details: Return value. Exception safety: noexcept.

#### `Level getLevel() const override`
- Source: `include/core/concerns/zero_copy_logger.h`:119
- Brief: n/a
- Parameters: none

#### `void info(const std::string &message) override`
- Source: `include/core/concerns/zero_copy_logger.h`:105
- Brief: Info.
- Parameters:
  - `message` (const std::string &): Input parameter.
- Details: message Input parameter.

#### `void infoSV(std::string_view message) noexcept`
- Source: `include/core/concerns/zero_copy_logger.h`:70
- Brief: Info SV.
- Parameters:
  - `message` (std::string_view): Input parameter.
- Details: message Input parameter. Exception safety: noexcept.

#### `ProbeResult isHealthy() const override`
- Source: `include/core/concerns/zero_copy_logger.h`:128
- Brief: n/a
- Parameters: none

#### `bool isPiiKey(std::string_view key) noexcept`
- Source: `include/core/concerns/zero_copy_logger.h`:172
- Brief: Is Pii Key.
- Parameters:
  - `key` (std::string_view): Input parameter.
- Return: True when the operation succeeds.
- Details: key Input parameter. True when the operation succeeds. Exception safety: noexcept.

#### `void jsonEscapeInto(std::string &out, std::string_view s)`
- Source: `include/core/concerns/zero_copy_logger.h`:164
- Brief: Json Escape Into.
- Parameters:
  - `out` (std::string &): Input/output parameter.
  - `s` (std::string_view): Input parameter.
- Details: out Input/output parameter. s Input parameter. out Input/output parameter. s Input parameter. Calls: reserve(), size(), std::snprintf().

#### `bool jsonMode() const noexcept`
- Source: `include/core/concerns/zero_copy_logger.h`:135
- Brief: n/a
- Parameters: none

#### `void log(Level level, const std::string &message) override`
- Source: `include/core/concerns/zero_copy_logger.h`:102
- Brief: Log.
- Parameters:
  - `level` (Level): Input parameter.
  - `message` (const std::string &): Input parameter.
- Details: level Input parameter. message Input parameter.

#### `void logSV(Level level, std::string_view message) noexcept`
- Source: `include/core/concerns/zero_copy_logger.h`:49
- Brief: Log SV.
- Parameters:
  - `level` (Level): Input parameter.
  - `message` (std::string_view): Input parameter.
- Details: level Input parameter. message Input parameter. Exception safety: noexcept.

#### `void logStructured(Level level, const std::string &message, const Fields &fields={}) override`
- Source: `include/core/concerns/zero_copy_logger.h`:110
- Brief: Log Structured.
- Parameters:
  - `level` (Level): Input parameter.
  - `message` (const std::string &): Input parameter.
  - `fields` (const Fields &): Input parameter.
- Details: level Input parameter. message Input parameter. fields Input parameter. Calls: should_log(), toSpdlogLevel(), formatBuffer(), clear(), load(), std::chrono::system_clock::now(), time_since_epoch(), count().

#### `void logStructuredSV(Level level, std::string_view message, std::initializer_list< std::pair< std::string_view, std::string_view > > fields={})`
- Source: `include/core/concerns/zero_copy_logger.h`:93
- Brief: n/a
- Parameters:
  - `level` (Level): n/a
  - `message` (std::string_view): n/a
  - `fields` (std::initializer_list< std::pair< std::string_view, std::string_view > >): n/a

#### `void setJsonMode(bool enabled) noexcept`
- Source: `include/core/concerns/zero_copy_logger.h`:134
- Brief: n/a
- Parameters:
  - `enabled` (bool): n/a

#### `void setLevel(Level level) override`
- Source: `include/core/concerns/zero_copy_logger.h`:118
- Brief: Set Level.
- Parameters:
  - `level` (Level): Input parameter.
- Details: level Input parameter. Calls: set_level(), toSpdlogLevel().

#### `void setPattern(const std::string &pattern) override`
- Source: `include/core/concerns/zero_copy_logger.h`:120
- Brief: Set Pattern.
- Parameters:
  - `pattern` (const std::string &): Input parameter.
- Details: pattern Input parameter. Calls: set_pattern().

#### `bool shouldLog(Level level) const noexcept`
- Source: `include/core/concerns/zero_copy_logger.h`:41
- Brief: ========================================================================= Zero-copy string_view hot-path API =========================================================================
- Parameters:
  - `level` (Level): Input parameter.
- Return: True when the operation succeeds.
- Details: level Input parameter. True when the operation succeeds. Exception safety: noexcept.

#### `void shutdown() noexcept override`
- Source: `include/core/concerns/zero_copy_logger.h`:127
- Brief: n/a
- Parameters: none

#### `spdlog::level::level_enum toSpdlogLevel(Level level) noexcept`
- Source: `include/core/concerns/zero_copy_logger.h`:150
- Brief: To Spdlog Level.
- Parameters:
  - `level` (Level): Input parameter.
- Return: Return value.
- Details: level Input parameter. Return value. Exception safety: noexcept.

#### `void trace(const std::string &message) override`
- Source: `include/core/concerns/zero_copy_logger.h`:103
- Brief: Trace.
- Parameters:
  - `message` (const std::string &): Input parameter.
- Details: message Input parameter.

#### `void traceSV(std::string_view message) noexcept`
- Source: `include/core/concerns/zero_copy_logger.h`:56
- Brief: Trace SV.
- Parameters:
  - `message` (std::string_view): Input parameter.
- Details: message Input parameter. Exception safety: noexcept.

#### `void warn(const std::string &message) override`
- Source: `include/core/concerns/zero_copy_logger.h`:106
- Brief: Warn.
- Parameters:
  - `message` (const std::string &): Input parameter.
- Details: message Input parameter.

#### `void warnSV(std::string_view message) noexcept`
- Source: `include/core/concerns/zero_copy_logger.h`:77
- Brief: Warn SV.
- Parameters:
  - `message` (std::string_view): Input parameter.
- Details: message Input parameter. Exception safety: noexcept.

### themis::core::concerns::ZipkinTracerAdapter

#### `ZipkinTracerAdapter()`
- Source: `include/core/concerns/zipkin_tracer_adapter.h`:52
- Brief: n/a
- Parameters: none

#### `ZipkinTracerAdapter(const CircuitBreakerConfig &cb_config)`
- Source: `include/core/concerns/zipkin_tracer_adapter.h`:44
- Brief: Zipkin Tracer Adapter.
- Parameters:
  - `cb_config` (const CircuitBreakerConfig &): Input parameter.
- Return: Return value.
- Details: cb_config Input parameter. Return value. Implements ZipkinTracerAdapter without additional internal calls.

#### `sharding::CircuitBreaker::State circuitBreakerState() const`
- Source: `include/core/concerns/zipkin_tracer_adapter.h`:252
- Brief: n/a
- Parameters: none

#### `void flush() noexcept override`
- Source: `include/core/concerns/zipkin_tracer_adapter.h`:238
- Brief: n/a
- Parameters: none

#### `std::string headerValueCI(const std::map< std::string, std::string > &headers, const std::string &name)`
- Source: `include/core/concerns/zipkin_tracer_adapter.h`:264
- Brief: n/a
- Parameters:
  - `headers` (const std::map< std::string, std::string > &): n/a
  - `name` (const std::string &): n/a

#### `bool initialize(const std::string &serviceName, const std::string &endpoint) override`
- Source: `include/core/concerns/zipkin_tracer_adapter.h`:221
- Brief: n/a
- Parameters:
  - `serviceName` (const std::string &): n/a
  - `endpoint` (const std::string &): n/a

#### `void injectContext(std::map< std::string, std::string > &carrier_headers) override`
- Source: `include/core/concerns/zipkin_tracer_adapter.h`:197
- Brief: n/a
- Parameters:
  - `carrier_headers` (std::map< std::string, std::string > &): n/a

#### `ProbeResult isHealthy() const override`
- Source: `include/core/concerns/zipkin_tracer_adapter.h`:242
- Brief: n/a
- Parameters: none

#### `bool isInitialized() const override`
- Source: `include/core/concerns/zipkin_tracer_adapter.h`:236
- Brief: n/a
- Parameters: none

#### `bool parseB3Single(const std::string &value, B3Ids &out)`
- Source: `include/core/concerns/zipkin_tracer_adapter.h`:296
- Brief: Parse B3 Single.
- Parameters:
  - `value` (const std::string &): Input parameter.
  - `out` (B3Ids &): Input/output parameter.
- Return: True when the operation succeeds.
- Details: value Input parameter. out Input/output parameter. True when the operation succeeds. Calls: reserve(), find(), push_back(), substr(), size(), empty(), std::all_of(), begin().

#### `void shutdown() override`
- Source: `include/core/concerns/zipkin_tracer_adapter.h`:231
- Brief: Shutdown.
- Parameters: none

#### `std::unique_ptr< ISpan > startChildSpan(const std::string &name, const ISpan &parent) override`
- Source: `include/core/concerns/zipkin_tracer_adapter.h`:121
- Brief: n/a
- Parameters:
  - `name` (const std::string &): n/a
  - `parent` (const ISpan &): n/a

#### `std::unique_ptr< ISpan > startSpan(const std::string &name) override`
- Source: `include/core/concerns/zipkin_tracer_adapter.h`:111
- Brief: n/a
- Parameters:
  - `name` (const std::string &): n/a

#### `std::unique_ptr< ISpan > startSpanFromHeaders(const std::string &name, const std::map< std::string, std::string > &carrier_headers) override`
- Source: `include/core/concerns/zipkin_tracer_adapter.h`:140
- Brief: n/a
- Parameters:
  - `name` (const std::string &): n/a
  - `carrier_headers` (const std::map< std::string, std::string > &): n/a

### themis::core::concerns::ZipkinTracerAdapter::ZipkinSpanAdapter

#### `ZipkinSpanAdapter(themis::Tracer::Span span)`
- Source: `include/core/concerns/zipkin_tracer_adapter.h`:65
- Brief: Zipkin Span Adapter.
- Parameters:
  - `span` (themis::Tracer::Span): Input parameter.
- Return: Return value.
- Details: span Input parameter. Return value.

#### `void end() override`
- Source: `include/core/concerns/zipkin_tracer_adapter.h`:86
- Brief: End.
- Parameters: none

#### `themis::Tracer::Span & getSpan()`
- Source: `include/core/concerns/zipkin_tracer_adapter.h`:100
- Brief: Get Span.
- Parameters: none
- Return: Return value.
- Details: Return value. Implements getSpan without additional internal calls.

#### `const themis::Tracer::Span & getSpan() const`
- Source: `include/core/concerns/zipkin_tracer_adapter.h`:101
- Brief: n/a
- Parameters: none

#### `bool isValid() const override`
- Source: `include/core/concerns/zipkin_tracer_adapter.h`:89
- Brief: n/a
- Parameters: none

#### `void recordError(const std::string &errorMessage) override`
- Source: `include/core/concerns/zipkin_tracer_adapter.h`:80
- Brief: Record Error.
- Parameters:
  - `errorMessage` (const std::string &): Input parameter.
- Details: errorMessage Input parameter.

#### `void setAttribute(const std::string &key, bool value) override`
- Source: `include/core/concerns/zipkin_tracer_adapter.h`:77
- Brief: Set Attribute.
- Parameters:
  - `key` (const std::string &): Input parameter.
  - `value` (bool): Input parameter.
- Details: key Input parameter. value Input parameter.

#### `void setAttribute(const std::string &key, const std::string &value) override`
- Source: `include/core/concerns/zipkin_tracer_adapter.h`:68
- Brief: Set Attribute.
- Parameters:
  - `key` (const std::string &): Input parameter.
  - `value` (const std::string &): Input parameter.
- Details: key Input parameter. value Input parameter.

#### `void setAttribute(const std::string &key, double value) override`
- Source: `include/core/concerns/zipkin_tracer_adapter.h`:74
- Brief: Set Attribute.
- Parameters:
  - `key` (const std::string &): Input parameter.
  - `value` (double): Input parameter.
- Details: key Input parameter. value Input parameter.

#### `void setAttribute(const std::string &key, int64_t value) override`
- Source: `include/core/concerns/zipkin_tracer_adapter.h`:71
- Brief: Set Attribute.
- Parameters:
  - `key` (const std::string &): Input parameter.
  - `value` (int64_t): Input parameter.
- Details: key Input parameter. value Input parameter.

#### `void setStatus(bool ok, const std::string &description="") override`
- Source: `include/core/concerns/zipkin_tracer_adapter.h`:83
- Brief: n/a
- Parameters:
  - `ok` (bool): n/a
  - `description` (const std::string &): n/a

#### `~ZipkinSpanAdapter() override`
- Source: `include/core/concerns/zipkin_tracer_adapter.h`:93
- Brief: n/a
- Parameters: none

### themis::test::wave_d

#### `TEST(WaveD_CoreStress, ConcurrentModuleInitStress)`
- Source: `tests/core/test_core_highcardinality_stress.cpp`:152
- Brief: n/a
- Parameters:
  - `<unnamed>` (WaveD_CoreStress): n/a
  - `<unnamed>` (ConcurrentModuleInitStress): n/a

#### `TEST(WaveD_CoreStress, HighCardinalityConfigUpdate)`
- Source: `tests/core/test_core_highcardinality_stress.cpp`:120
- Brief: n/a
- Parameters:
  - `<unnamed>` (WaveD_CoreStress): n/a
  - `<unnamed>` (HighCardinalityConfigUpdate): n/a

#### `TEST(WaveD_CoreStress, TracingExporterHighLoad)`
- Source: `tests/core/test_core_highcardinality_stress.cpp`:185
- Brief: n/a
- Parameters:
  - `<unnamed>` (WaveD_CoreStress): n/a
  - `<unnamed>` (TracingExporterHighLoad): n/a

### themis::test::wave_d::StubConfigStore

#### `bool get(const std::string &key, std::string *out) const`
- Source: `tests/core/test_core_highcardinality_stress.cpp`:51
- Brief: n/a
- Parameters:
  - `key` (const std::string &): n/a
  - `out` (std::string *): n/a

#### `bool set(const std::string &key, const std::string &value)`
- Source: `tests/core/test_core_highcardinality_stress.cpp`:44
- Brief: n/a
- Parameters:
  - `key` (const std::string &): n/a
  - `value` (const std::string &): n/a

#### `std::size_t size() const`
- Source: `tests/core/test_core_highcardinality_stress.cpp`:59
- Brief: n/a
- Parameters: none

#### `long writes() const`
- Source: `tests/core/test_core_highcardinality_stress.cpp`:64
- Brief: n/a
- Parameters: none

### themis::test::wave_d::StubModuleLifecycle

#### `long failures() const`
- Source: `tests/core/test_core_highcardinality_stress.cpp`:89
- Brief: n/a
- Parameters: none

#### `bool initialize(const std::string &module_id)`
- Source: `tests/core/test_core_highcardinality_stress.cpp`:73
- Brief: n/a
- Parameters:
  - `module_id` (const std::string &): n/a

#### `long inits() const`
- Source: `tests/core/test_core_highcardinality_stress.cpp`:87
- Brief: n/a
- Parameters: none

#### `bool teardown(const std::string &module_id)`
- Source: `tests/core/test_core_highcardinality_stress.cpp`:80
- Brief: n/a
- Parameters:
  - `module_id` (const std::string &): n/a

#### `long teardowns() const`
- Source: `tests/core/test_core_highcardinality_stress.cpp`:88
- Brief: n/a
- Parameters: none

### themis::test::wave_d::StubSpanExporter

#### `bool exportSpan(uint64_t span_id, const std::string &name)`
- Source: `tests/core/test_core_highcardinality_stress.cpp`:100
- Brief: n/a
- Parameters:
  - `span_id` (uint64_t): n/a
  - `name` (const std::string &): n/a

#### `long exports() const`
- Source: `tests/core/test_core_highcardinality_stress.cpp`:107
- Brief: n/a
- Parameters: none

#### `long lag() const`
- Source: `tests/core/test_core_highcardinality_stress.cpp`:108
- Brief: n/a
- Parameters: none

