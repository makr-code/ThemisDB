# TOOLBOX DOXYGEN

Author: ThemisDB Contributors
Created: 2026-09-20
Last Updated: 2026-09-20
Status: active

## Source
- This file is generated from module-scoped Doxygen XML.
- XML index: `C:\Projects\ThemisDB\.tmp\module_doxygen_global_e2e\toolbox\xml\index.xml`
- Warnings log: `C:\Projects\ThemisDB\.tmp\module_doxygen_global_e2e\toolbox\doxygen-warnings.log`

## Structure Metrics
- C/C++ Files (scanned): 31
- Compounds: 87
- Classes/Structs: 30
- Namespaces: 17
- File Compounds: 31

## Namespaces
- @000262052006073163261222040224331143074352027311
- @155022305267366232067334347227133271366115135017
- @276257272141142262075212213167072021316013215130
- testing
- themis
- themis::aql
- themis::bench
- themis::bench::toolbox_dedicated
- themis::ingestion
- themis::rag
- themis::toolbox
- themis::toolbox::@054324037053145107206252221027274213302275020312
- themis::toolbox::@212141334176235243327256313017147227227204045247
- themis::toolbox::@246205220117321375041150055307051206251210050146
- themis::toolbox::@257066257343253224110024101355102107132227347275
- themis::toolbox::@264224036174241060153252170351243220362053321327
- themis::toolbox::@325361140014107175311031020116152210232165126217

## Types
### Classes
- DegradedPathFixture
- HighConcurrencyFixture
- IngestionToolbox
- IngestionToolbox::Impl
- LongRunStressFixture
- MixedContentFixture
- StubCompositeRouter
- StubContentBridge
- ToolboxContractTest
- ToolboxRegistryTest
- themis::toolbox::ContentFingerprinter
- themis::toolbox::ContentToolboxBridge
- themis::toolbox::ContentToolboxBridge::Impl
- themis::toolbox::DefaultLanguageDetector
- themis::toolbox::ILanguageDetector
- themis::toolbox::IngestionToolbox
- themis::toolbox::IngestionToolbox::Impl
- themis::toolbox::TextChunker
- themis::toolbox::TextNormalizer
- themis::toolbox::TextQualityScorer
- themis::toolbox::ToolboxBuilder
- themis::toolbox::ToolboxComposite
- themis::toolbox::ToolboxCompositeBuilder
- themis::toolbox::ToolboxRegistry

### Structs
- DispatchRecord
- themis::toolbox::ContentFingerprint
- themis::toolbox::ContentToolboxBridge::BridgeResult
- themis::toolbox::TextQualityScore
- themis::toolbox::ToolboxBuilder::BuiltToolbox
- themis::toolbox::ToolboxBuilder::Impl

## Notes
- This is a derived artifact. Do not manually edit semantic content.
- Regenerate via scripts/generate_module_doxygen_xml.py --write-module-markdown.

## Detailed Function Documentation (Soll-Ist)
- Functions extracted: 268

### IngestionToolbox

#### `IngestionToolbox()`
- Source: `include/toolbox/ingestion_toolbox.h`:77
- Brief: n/a
- Parameters: none

#### `IngestionToolbox(IngestionToolbox &&) noexcept`
- Source: `include/toolbox/ingestion_toolbox.h`:83
- Brief: n/a
- Parameters:
  - `<unnamed>` (IngestionToolbox &&): n/a

#### `IngestionToolbox(const IngestionToolbox &)=delete`
- Source: `include/toolbox/ingestion_toolbox.h`:81
- Brief: n/a
- Parameters:
  - `<unnamed>` (const IngestionToolbox &): n/a

#### `std::shared_ptr< IngestionToolbox > createDefault()`
- Source: `include/toolbox/ingestion_toolbox.h`:96
- Brief: Create an IngestionToolbox pre-configured with all built-in steps and a NullTextGenerationBackend.
- Parameters: none
- Return: Return value.
- Details: ── Factory ────────────────────────────────────────────────────────────────── The returned toolbox is ready for immediate use. Inject a real ITextGenerationBackend via setTextBackend() to enable LLM-backed NER and entity extraction. Return value. Calls: stepRegistry(), registerStep(), ingestion::builtin::createNerDeStep(), textBackend(), ingestion::builtin::createLlmExtractStep(), ingestion::builtin::createChunkTtDecomposeStep(), ingestion::builtin::createTensorCoreBridgeStep().

#### `std::vector< ingestion::BaseEntity > extractEntities(const std::string &text, const std::string &mime="text/plain", const std::string &filename="input.txt")`
- Source: `include/toolbox/ingestion_toolbox.h`:159
- Brief: Extract entities from a plain text string.
- Parameters:
  - `text` (const std::string &): Input parameter.
  - `mime` (const std::string &): Input parameter.
  - `filename` (const std::string &): Input parameter.
- Return: Extracted and normalised entity nodes; empty on workflow failure (failure details are logged at warn level).
- Details: Extract Entities. Constructs a minimal ExtractionContext from text, selects the best matching workflow profile (using MIME type mime and filename hint filename), runs the workflow, and returns the assembled entity nodes. This is the primary entry point for consumers that need entity extraction without managing ExtractionContext directly. text UTF-8 text to process (may be empty — returns {}). mime Detected MIME type hint (default: "text/plain"). filename Filename hint used for profile selection (default: "input.txt"). Extracted and normalised entity nodes; empty on workflow failure (failure details are logged at warn level). text Input parameter. mime Input parameter. filename Input parameter. Return value.

#### `ingestion::BaseEntitySet extractEntitySet(const std::string &text, const std::string &mime="text/plain", const std::string &filename="input.txt")`
- Source: `include/toolbox/ingestion_toolbox.h`:180
- Brief: Extract the full BaseEntitySet from a plain text string.
- Parameters:
  - `text` (const std::string &): Input parameter.
  - `mime` (const std::string &): Input parameter.
  - `filename` (const std::string &): Input parameter.
- Return: Full assembled entity set; empty BaseEntitySet on failure.
- Details: Extract Entity Set. Like extractEntities() but returns the complete BaseEntitySet including nodes, edges, and chunks (vector index entries). Consumers that need both the graph entities and the embedding chunks (e.g. ContentToolboxBridge for BridgeResult::vectors) should call this method instead of extractEntities(). text UTF-8 text to process (may be empty — returns {}). mime Detected MIME type hint (default: "text/plain"). filename Filename hint used for profile selection (default: "input.txt"). Full assembled entity set; empty BaseEntitySet on failure. text Input parameter. mime Input parameter. filename Input parameter. Return value.

#### `std::string getMetricsText() const`
- Source: `include/toolbox/ingestion_toolbox.h`:216
- Brief: Produce Prometheus text-format metrics.
- Parameters: none
- Return: Prometheus text payload, or "" if unused.
- Details: Emits the following metric families (Prometheus text v0.0.4): toolbox_extract_calls_total counter — total extractEntities() calls toolbox_extract_errors_total counter — failed calls toolbox_extract_entities_total counter — cumulative entity count toolbox_extract_latency_ms_total counter — cumulative latency Returns an empty string when no calls have been recorded. Prometheus text payload, or "" if unused.

#### `IngestionToolbox & operator=(IngestionToolbox &&) noexcept`
- Source: `include/toolbox/ingestion_toolbox.h`:84
- Brief: n/a
- Parameters:
  - `<unnamed>` (IngestionToolbox &&): n/a

#### `IngestionToolbox & operator=(const IngestionToolbox &)=delete`
- Source: `include/toolbox/ingestion_toolbox.h`:82
- Brief: n/a
- Parameters:
  - `<unnamed>` (const IngestionToolbox &): n/a

#### `void recordExtraction(std::size_t entity_count, uint64_t latency_ms, bool success) noexcept`
- Source: `include/toolbox/ingestion_toolbox.h`:199
- Brief: Record one completed extractEntities() call in the metrics counters.
- Parameters:
  - `entity_count` (std::size_t): Number of entities returned (0 on failure).
  - `latency_ms` (uint64_t): Wall-clock latency of the call in milliseconds.
  - `success` (bool): Whether the workflow completed without error.
- Details: Call this after every extractEntities() / extractEntitySet() invocation (including error paths). Thread-safe; uses std::atomic. entity_count Number of entities returned (0 on failure). latency_ms Wall-clock latency of the call in milliseconds. success Whether the workflow completed without error.

#### `void setTextBackend(std::shared_ptr< ingestion::ITextGenerationBackend > backend)`
- Source: `include/toolbox/ingestion_toolbox.h`:114
- Brief: Inject or replace the text-generation backend.
- Parameters:
  - `backend` (std::shared_ptr< ingestion::ITextGenerationBackend >): Input parameter.
- Details: Set Text Backend. When backend is nullptr, a NullTextGenerationBackend is reinstated. The new backend is propagated to all NER/LLM steps that have been registered in the StepRegistry. backend Input parameter.

#### `void setWorkflowEngine(std::shared_ptr< ingestion::WorkflowEngine > engine)`
- Source: `include/toolbox/ingestion_toolbox.h`:105
- Brief: Replace the WorkflowEngine.
- Parameters:
  - `engine` (std::shared_ptr< ingestion::WorkflowEngine >): Input parameter.
- Details: ── Dependency injection ────────────────────────────────────────────────────── engine Must not be null. engine Input parameter.

#### `ingestion::StepRegistry & stepRegistry()`
- Source: `include/toolbox/ingestion_toolbox.h`:130
- Brief: Access the StepRegistry for custom step registration.
- Parameters: none
- Return: Reference to the registry owned by workflowEngine().
- Details: Step Registry. Reference to the registry owned by workflowEngine(). Return value. Implements stepRegistry without additional internal calls.

#### `std::shared_ptr< ingestion::ITextGenerationBackend > textBackend() const`
- Source: `include/toolbox/ingestion_toolbox.h`:137
- Brief: Access the currently active text-generation backend.
- Parameters: none
- Return: Always non-null (falls back to NullTextGenerationBackend).
- Details: Always non-null (falls back to NullTextGenerationBackend).

#### `std::shared_ptr< ingestion::WorkflowEngine > workflowEngine() const`
- Source: `include/toolbox/ingestion_toolbox.h`:123
- Brief: Access the WorkflowEngine for profile loading and execution.
- Parameters: none
- Return: Always non-null.
- Details: Always non-null.

#### `~IngestionToolbox()`
- Source: `include/toolbox/ingestion_toolbox.h`:78
- Brief: n/a
- Parameters: none

### IngestionToolbox::Impl

#### `Impl()`
- Source: `src/toolbox/ingestion_toolbox.cpp`:31
- Brief: n/a
- Parameters: none

### StubCompositeRouter

#### `DispatchRecord dispatch(const std::string &tool_name, const std::string &) noexcept`
- Source: `tests/toolbox/test_toolbox_highcardinality_stress.cpp`:48
- Brief: n/a
- Parameters:
  - `tool_name` (const std::string &): n/a
  - `<unnamed>` (const std::string &): n/a

#### `uint64_t totalOps() const noexcept`
- Source: `tests/toolbox/test_toolbox_highcardinality_stress.cpp`:52
- Brief: n/a
- Parameters: none

### StubContentBridge

#### `bool bridge(uint64_t content_id, const std::string &) noexcept`
- Source: `tests/toolbox/test_toolbox_highcardinality_stress.cpp`:59
- Brief: n/a
- Parameters:
  - `content_id` (uint64_t): n/a
  - `<unnamed>` (const std::string &): n/a

#### `uint64_t totalOps() const noexcept`
- Source: `tests/toolbox/test_toolbox_highcardinality_stress.cpp`:64
- Brief: n/a
- Parameters: none

### ToolboxRegistryTest

#### `void SetUp() override`
- Source: `tests/toolbox/test_toolbox_phase5.cpp`:210
- Brief: n/a
- Parameters: none

#### `void TearDown() override`
- Source: `tests/toolbox/test_toolbox_phase5.cpp`:211
- Brief: n/a
- Parameters: none

### bench_toolbox_dedicated_gates.cpp

#### `BENCHMARK_MAIN()`
- Source: `benchmarks/toolbox/bench_toolbox_dedicated_gates.cpp`:143
- Brief: n/a
- Parameters: none

### bench_toolbox_native_workloads.cpp

#### `BENCHMARK_MAIN()`
- Source: `benchmarks/toolbox/bench_toolbox_native_workloads.cpp`:256
- Brief: n/a
- Parameters: none

#### `void BM_BridgeProxy_EntitySetLatency(benchmark::State &state)`
- Source: `benchmarks/toolbox/bench_toolbox_native_workloads.cpp`:199
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_ContentFingerprinting_Throughput(benchmark::State &state)`
- Source: `benchmarks/toolbox/bench_toolbox_native_workloads.cpp`:173
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_EmptyExtraction_Throughput(benchmark::State &state)`
- Source: `benchmarks/toolbox/bench_toolbox_native_workloads.cpp`:218
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_ExtractEntities_Throughput(benchmark::State &state)`
- Source: `benchmarks/toolbox/bench_toolbox_native_workloads.cpp`:82
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_ExtractEntitySet_Latency(benchmark::State &state)`
- Source: `benchmarks/toolbox/bench_toolbox_native_workloads.cpp`:105
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_LanguageDetection_Latency(benchmark::State &state)`
- Source: `benchmarks/toolbox/bench_toolbox_native_workloads.cpp`:151
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_MetricsGeneration(benchmark::State &state)`
- Source: `benchmarks/toolbox/bench_toolbox_native_workloads.cpp`:236
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_TextNormalization_Latency(benchmark::State &state)`
- Source: `benchmarks/toolbox/bench_toolbox_native_workloads.cpp`:129
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `Iterations(100) -> Repetitions(5) ->ReportAggregatesOnly(true) ->Unit(benchmark::kMillisecond)`
- Source: `benchmarks/toolbox/bench_toolbox_native_workloads.cpp`:96
- Brief: n/a
- Parameters:
  - `<unnamed>` (100): n/a

#### `Iterations(1000) -> Repetitions(5) ->ReportAggregatesOnly(true) ->Unit(benchmark::kMicrosecond)`
- Source: `benchmarks/toolbox/bench_toolbox_native_workloads.cpp`:142
- Brief: n/a
- Parameters:
  - `<unnamed>` (1000): n/a

#### `Iterations(10000) -> Repetitions(5) ->ReportAggregatesOnly(true) ->Unit(benchmark::kMicrosecond)`
- Source: `benchmarks/toolbox/bench_toolbox_native_workloads.cpp`:227
- Brief: n/a
- Parameters:
  - `<unnamed>` (10000): n/a

#### `Iterations(50) -> Repetitions(5) ->ReportAggregatesOnly(true) ->Unit(benchmark::kMillisecond)`
- Source: `benchmarks/toolbox/bench_toolbox_native_workloads.cpp`:120
- Brief: n/a
- Parameters:
  - `<unnamed>` (50): n/a

### bench_toolbox_release_gates.cpp

#### `BENCHMARK(BM_Fingerprint_DefaultInit) -> Repetitions(5) ->ReportAggregatesOnly(true)`
- Source: `benchmarks/toolbox/bench_toolbox_release_gates.cpp`:17
- Brief: n/a
- Parameters:
  - `<unnamed>` (BM_Fingerprint_DefaultInit): n/a

#### `BENCHMARK(BM_Fingerprint_Memset) -> Repetitions(5) ->ReportAggregatesOnly(true)`
- Source: `benchmarks/toolbox/bench_toolbox_release_gates.cpp`:41
- Brief: n/a
- Parameters:
  - `<unnamed>` (BM_Fingerprint_Memset): n/a

#### `BENCHMARK(BM_ToolboxError_Cast) -> Repetitions(5) ->ReportAggregatesOnly(true)`
- Source: `benchmarks/toolbox/bench_toolbox_release_gates.cpp`:32
- Brief: n/a
- Parameters:
  - `<unnamed>` (BM_ToolboxError_Cast): n/a

#### `BENCHMARK(BM_ToolboxError_SwitchDispatch) -> Repetitions(5) ->ReportAggregatesOnly(true)`
- Source: `benchmarks/toolbox/bench_toolbox_release_gates.cpp`:63
- Brief: n/a
- Parameters:
  - `<unnamed>` (BM_ToolboxError_SwitchDispatch): n/a

#### `BENCHMARK_MAIN()`
- Source: `benchmarks/toolbox/bench_toolbox_release_gates.cpp`:65
- Brief: n/a
- Parameters: none

#### `void BM_Fingerprint_DefaultInit(benchmark::State &state)`
- Source: `benchmarks/toolbox/bench_toolbox_release_gates.cpp`:11
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_Fingerprint_Memset(benchmark::State &state)`
- Source: `benchmarks/toolbox/bench_toolbox_release_gates.cpp`:34
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_ToolboxError_Cast(benchmark::State &state)`
- Source: `benchmarks/toolbox/bench_toolbox_release_gates.cpp`:19
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_ToolboxError_SwitchDispatch(benchmark::State &state)`
- Source: `benchmarks/toolbox/bench_toolbox_release_gates.cpp`:43
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

### test_toolbox_contract_hardening_focused.cpp

#### `TEST_F(ToolboxContractTest, IT13_BuilderMixedContentTextOnly)`
- Source: `tests/toolbox/test_toolbox_contract_hardening_focused.cpp`:107
- Brief: n/a
- Parameters:
  - `<unnamed>` (ToolboxContractTest): n/a
  - `<unnamed>` (IT13_BuilderMixedContentTextOnly): n/a
- Details: TestIT-13: Builder mixed-content scenario — text-only validation Verifies that ToolboxBuilder handles mixed-content scenarios correctly when only text content is provided. Pass condition: build() succeeds no null pointers in returned toolbox text-only extraction does not crash

#### `TEST_F(ToolboxContractTest, IT14_BuilderReusePrevention)`
- Source: `tests/toolbox/test_toolbox_contract_hardening_focused.cpp`:129
- Brief: n/a
- Parameters:
  - `<unnamed>` (ToolboxContractTest): n/a
  - `<unnamed>` (IT14_BuilderReusePrevention): n/a
- Details: TestIT-14: Builder reuse error handling Verifies that calling build() a second time on the same builder throws std::logic_error (single-use pattern enforcement). Pass condition: First build() succeeds Second build() throws std::logic_error with meaningful message

#### `TEST_F(ToolboxContractTest, IT15_RegistryDoubleInitialization)`
- Source: `tests/toolbox/test_toolbox_contract_hardening_focused.cpp`:153
- Brief: n/a
- Parameters:
  - `<unnamed>` (ToolboxContractTest): n/a
  - `<unnamed>` (IT15_RegistryDoubleInitialization): n/a
- Details: TestIT-15: Registry double-initialization behavior Verifies that calling initialize() twice replaces the previous instance (last-write-wins semantics for live reconfiguration). Pass condition: First initialize() sets instance Second initialize() replaces instance without throwing Third retrieve returns the second instance

#### `TEST_F(ToolboxContractTest, IT16_BridgeNullOptionalWriters)`
- Source: `tests/toolbox/test_toolbox_contract_hardening_focused.cpp`:188
- Brief: n/a
- Parameters:
  - `<unnamed>` (ToolboxContractTest): n/a
  - `<unnamed>` (IT16_BridgeNullOptionalWriters): n/a
- Details: TestIT-16: Bridge with null optional writers Verifies that ContentToolboxBridge accepts null graph_writer and vector_writer (optional soft-fail behavior). Pass condition: Constructor succeeds with null writers Accessors return null pointers for unset writers No exceptions thrown during construction

#### `TEST_F(ToolboxContractTest, IT17_BridgeEmptyExtractionHandling)`
- Source: `tests/toolbox/test_toolbox_contract_hardening_focused.cpp`:216
- Brief: n/a
- Parameters:
  - `<unnamed>` (ToolboxContractTest): n/a
  - `<unnamed>` (IT17_BridgeEmptyExtractionHandling): n/a
- Details: TestIT-17: Bridge empty content extraction handling Verifies that the bridge correctly handles empty extracted text (binary-only content scenarios). Pass condition: No exception thrown for empty content Result returns ok=true with empty entities/vectors No crashes in soft-fail paths

#### `TEST_F(ToolboxContractTest, IT18_StreamingBoundaryConditions)`
- Source: `tests/toolbox/test_toolbox_contract_hardening_focused.cpp`:232
- Brief: n/a
- Parameters:
  - `<unnamed>` (ToolboxContractTest): n/a
  - `<unnamed>` (IT18_StreamingBoundaryConditions): n/a
- Details: TestIT-18: Streaming boundary conditions Verifies that streaming and batching operations respect edge cases (empty batches, single items, boundary sizes). Pass condition: Operations complete without error for all boundary cases Metrics are recorded correctly No buffer overflows or underflows

#### `TEST_F(ToolboxContractTest, IT19_CompositeRoutingFallback)`
- Source: `tests/toolbox/test_toolbox_contract_hardening_focused.cpp`:254
- Brief: n/a
- Parameters:
  - `<unnamed>` (ToolboxContractTest): n/a
  - `<unnamed>` (IT19_CompositeRoutingFallback): n/a
- Details: TestIT-19: Composite routing fallback behavior Verifies that composite routing falls back gracefully when a step is unavailable (e.g., null backends). Pass condition: No exception on missing steps Degraded behavior (empty result or partial enrichment) is acceptable Pipeline continues for available steps

#### `TEST_F(ToolboxContractTest, IT20_EmptyInputEdgeCases)`
- Source: `tests/toolbox/test_toolbox_contract_hardening_focused.cpp`:275
- Brief: n/a
- Parameters:
  - `<unnamed>` (ToolboxContractTest): n/a
  - `<unnamed>` (IT20_EmptyInputEdgeCases): n/a
- Details: TestIT-20: Empty input edge cases Verifies that all toolbox entry points handle empty input gracefully (empty strings, empty spans, empty collections). Pass condition: Empty input does not cause crashes Appropriate errors are returned (not silently ignored) Metrics track empty-input cases No undefined behavior or buffer access

#### `TEST_F(ToolboxContractTest, TBX01_ErrorCodesUnique)`
- Source: `tests/toolbox/test_toolbox_contract_hardening_focused.cpp`:26
- Brief: n/a
- Parameters:
  - `<unnamed>` (ToolboxContractTest): n/a
  - `<unnamed>` (TBX01_ErrorCodesUnique): n/a

#### `TEST_F(ToolboxContractTest, TBX02_ErrorCodesInRange)`
- Source: `tests/toolbox/test_toolbox_contract_hardening_focused.cpp`:37
- Brief: n/a
- Parameters:
  - `<unnamed>` (ToolboxContractTest): n/a
  - `<unnamed>` (TBX02_ErrorCodesInRange): n/a

#### `TEST_F(ToolboxContractTest, TBX03_FingerprintSizeIs32)`
- Source: `tests/toolbox/test_toolbox_contract_hardening_focused.cpp`:48
- Brief: n/a
- Parameters:
  - `<unnamed>` (ToolboxContractTest): n/a
  - `<unnamed>` (TBX03_FingerprintSizeIs32): n/a

#### `TEST_F(ToolboxContractTest, TBX04_FingerprintDefaultAllZero)`
- Source: `tests/toolbox/test_toolbox_contract_hardening_focused.cpp`:53
- Brief: n/a
- Parameters:
  - `<unnamed>` (ToolboxContractTest): n/a
  - `<unnamed>` (TBX04_FingerprintDefaultAllZero): n/a

#### `TEST_F(ToolboxContractTest, TBX05_FingerprintCanBeSet)`
- Source: `tests/toolbox/test_toolbox_contract_hardening_focused.cpp`:60
- Brief: n/a
- Parameters:
  - `<unnamed>` (ToolboxContractTest): n/a
  - `<unnamed>` (TBX05_FingerprintCanBeSet): n/a

#### `TEST_F(ToolboxContractTest, TBX06_EmptyInputDistinctFromNoProcessor)`
- Source: `tests/toolbox/test_toolbox_contract_hardening_focused.cpp`:67
- Brief: n/a
- Parameters:
  - `<unnamed>` (ToolboxContractTest): n/a
  - `<unnamed>` (TBX06_EmptyInputDistinctFromNoProcessor): n/a

#### `TEST_F(ToolboxContractTest, TBX07_ErrorSwitchDispatch)`
- Source: `tests/toolbox/test_toolbox_contract_hardening_focused.cpp`:72
- Brief: n/a
- Parameters:
  - `<unnamed>` (ToolboxContractTest): n/a
  - `<unnamed>` (TBX07_ErrorSwitchDispatch): n/a

#### `TEST_F(ToolboxContractTest, TBX08_RandomisedFingerprintCoverage)`
- Source: `tests/toolbox/test_toolbox_contract_hardening_focused.cpp`:84
- Brief: n/a
- Parameters:
  - `<unnamed>` (ToolboxContractTest): n/a
  - `<unnamed>` (TBX08_RandomisedFingerprintCoverage): n/a

### test_toolbox_highcardinality_stress.cpp

#### `TEST(ToolboxHighCardinalityStress, TBSTR01_HighCardinalityToolDispatch)`
- Source: `tests/toolbox/test_toolbox_highcardinality_stress.cpp`:72
- Brief: n/a
- Parameters:
  - `<unnamed>` (ToolboxHighCardinalityStress): n/a
  - `<unnamed>` (TBSTR01_HighCardinalityToolDispatch): n/a

#### `TEST(ToolboxHighCardinalityStress, TBSTR02_ConcurrentStreamBridgeStress)`
- Source: `tests/toolbox/test_toolbox_highcardinality_stress.cpp`:99
- Brief: n/a
- Parameters:
  - `<unnamed>` (ToolboxHighCardinalityStress): n/a
  - `<unnamed>` (TBSTR02_ConcurrentStreamBridgeStress): n/a

#### `TEST(ToolboxHighCardinalityStress, TBSTR03_CompositeRoutingStress)`
- Source: `tests/toolbox/test_toolbox_highcardinality_stress.cpp`:133
- Brief: n/a
- Parameters:
  - `<unnamed>` (ToolboxHighCardinalityStress): n/a
  - `<unnamed>` (TBSTR03_CompositeRoutingStress): n/a

### test_toolbox_ingestion.cpp

#### `TEST(AQLIngestionBridge, AB01_ConstructWithValidToolbox)`
- Source: `tests/toolbox/test_toolbox_ingestion.cpp`:161
- Brief: n/a
- Parameters:
  - `<unnamed>` (AQLIngestionBridge): n/a
  - `<unnamed>` (AB01_ConstructWithValidToolbox): n/a

#### `TEST(AQLIngestionBridge, AB02_ConstructWithNullToolboxThrows)`
- Source: `tests/toolbox/test_toolbox_ingestion.cpp`:166
- Brief: n/a
- Parameters:
  - `<unnamed>` (AQLIngestionBridge): n/a
  - `<unnamed>` (AB02_ConstructWithNullToolboxThrows): n/a

#### `TEST(AQLIngestionBridge, AB03_ToolboxAccessor)`
- Source: `tests/toolbox/test_toolbox_ingestion.cpp`:170
- Brief: n/a
- Parameters:
  - `<unnamed>` (AQLIngestionBridge): n/a
  - `<unnamed>` (AB03_ToolboxAccessor): n/a

#### `TEST(AQLIngestionBridge, AB04_GraphWriterNullByDefault)`
- Source: `tests/toolbox/test_toolbox_ingestion.cpp`:176
- Brief: n/a
- Parameters:
  - `<unnamed>` (AQLIngestionBridge): n/a
  - `<unnamed>` (AB04_GraphWriterNullByDefault): n/a

#### `TEST(AQLIngestionBridge, AB05_EnrichNonObjectIsNoop)`
- Source: `tests/toolbox/test_toolbox_ingestion.cpp`:182
- Brief: n/a
- Parameters:
  - `<unnamed>` (AQLIngestionBridge): n/a
  - `<unnamed>` (AB05_EnrichNonObjectIsNoop): n/a

#### `TEST(AQLIngestionBridge, AB06_EnrichObjectWithoutTextField)`
- Source: `tests/toolbox/test_toolbox_ingestion.cpp`:192
- Brief: n/a
- Parameters:
  - `<unnamed>` (AQLIngestionBridge): n/a
  - `<unnamed>` (AB06_EnrichObjectWithoutTextField): n/a

#### `TEST(AQLIngestionBridge, AB07_EnrichObjectWithTextFieldNoCrash)`
- Source: `tests/toolbox/test_toolbox_ingestion.cpp`:202
- Brief: n/a
- Parameters:
  - `<unnamed>` (AQLIngestionBridge): n/a
  - `<unnamed>` (AB07_EnrichObjectWithTextFieldNoCrash): n/a

#### `TEST(AQLIngestionBridge, AB08_ExtractEntitiesForContextEmpty)`
- Source: `tests/toolbox/test_toolbox_ingestion.cpp`:212
- Brief: n/a
- Parameters:
  - `<unnamed>` (AQLIngestionBridge): n/a
  - `<unnamed>` (AB08_ExtractEntitiesForContextEmpty): n/a

#### `TEST(AQLIngestionBridge, AB09_BuildEntityContextEmpty)`
- Source: `tests/toolbox/test_toolbox_ingestion.cpp`:219
- Brief: n/a
- Parameters:
  - `<unnamed>` (AQLIngestionBridge): n/a
  - `<unnamed>` (AB09_BuildEntityContextEmpty): n/a

#### `TEST(AQLIngestionBridge, AB10_BuildEntityContextNonEmpty)`
- Source: `tests/toolbox/test_toolbox_ingestion.cpp`:224
- Brief: n/a
- Parameters:
  - `<unnamed>` (AQLIngestionBridge): n/a
  - `<unnamed>` (AB10_BuildEntityContextNonEmpty): n/a

#### `TEST(AQLQueryBuilderEnrichment, QB01_DefaultFalse)`
- Source: `tests/toolbox/test_toolbox_ingestion.cpp`:241
- Brief: n/a
- Parameters:
  - `<unnamed>` (AQLQueryBuilderEnrichment): n/a
  - `<unnamed>` (QB01_DefaultFalse): n/a

#### `TEST(AQLQueryBuilderEnrichment, QB02_SetTrue)`
- Source: `tests/toolbox/test_toolbox_ingestion.cpp`:246
- Brief: n/a
- Parameters:
  - `<unnamed>` (AQLQueryBuilderEnrichment): n/a
  - `<unnamed>` (QB02_SetTrue): n/a

#### `TEST(AQLQueryBuilderEnrichment, QB03_SetFalse)`
- Source: `tests/toolbox/test_toolbox_ingestion.cpp`:252
- Brief: n/a
- Parameters:
  - `<unnamed>` (AQLQueryBuilderEnrichment): n/a
  - `<unnamed>` (QB03_SetFalse): n/a

#### `TEST(AQLQueryBuilderEnrichment, QB04_Fluent)`
- Source: `tests/toolbox/test_toolbox_ingestion.cpp`:259
- Brief: n/a
- Parameters:
  - `<unnamed>` (AQLQueryBuilderEnrichment): n/a
  - `<unnamed>` (QB04_Fluent): n/a

#### `TEST(IngestionToolbox, IT01_DefaultConstruct)`
- Source: `tests/toolbox/test_toolbox_ingestion.cpp`:55
- Brief: n/a
- Parameters:
  - `<unnamed>` (IngestionToolbox): n/a
  - `<unnamed>` (IT01_DefaultConstruct): n/a

#### `TEST(IngestionToolbox, IT02_CreateDefault)`
- Source: `tests/toolbox/test_toolbox_ingestion.cpp`:60
- Brief: n/a
- Parameters:
  - `<unnamed>` (IngestionToolbox): n/a
  - `<unnamed>` (IT02_CreateDefault): n/a

#### `TEST(IngestionToolbox, IT03_WorkflowEngineNeverNull)`
- Source: `tests/toolbox/test_toolbox_ingestion.cpp`:66
- Brief: n/a
- Parameters:
  - `<unnamed>` (IngestionToolbox): n/a
  - `<unnamed>` (IT03_WorkflowEngineNeverNull): n/a

#### `TEST(IngestionToolbox, IT04_DefaultTextBackendIsNull)`
- Source: `tests/toolbox/test_toolbox_ingestion.cpp`:71
- Brief: n/a
- Parameters:
  - `<unnamed>` (IngestionToolbox): n/a
  - `<unnamed>` (IT04_DefaultTextBackendIsNull): n/a

#### `TEST(IngestionToolbox, IT05_SetTextBackendNullReinstatesNull)`
- Source: `tests/toolbox/test_toolbox_ingestion.cpp`:79
- Brief: n/a
- Parameters:
  - `<unnamed>` (IngestionToolbox): n/a
  - `<unnamed>` (IT05_SetTextBackendNullReinstatesNull): n/a

#### `TEST(IngestionToolbox, IT06_SetWorkflowEngineNullThrows)`
- Source: `tests/toolbox/test_toolbox_ingestion.cpp`:88
- Brief: n/a
- Parameters:
  - `<unnamed>` (IngestionToolbox): n/a
  - `<unnamed>` (IT06_SetWorkflowEngineNullThrows): n/a

#### `TEST(IngestionToolbox, IT07_SetWorkflowEngineReplaces)`
- Source: `tests/toolbox/test_toolbox_ingestion.cpp`:93
- Brief: n/a
- Parameters:
  - `<unnamed>` (IngestionToolbox): n/a
  - `<unnamed>` (IT07_SetWorkflowEngineReplaces): n/a

#### `TEST(IngestionToolbox, IT08_StepRegistryAccessible)`
- Source: `tests/toolbox/test_toolbox_ingestion.cpp`:102
- Brief: n/a
- Parameters:
  - `<unnamed>` (IngestionToolbox): n/a
  - `<unnamed>` (IT08_StepRegistryAccessible): n/a

#### `TEST(IngestionToolbox, IT09_ExtractEntitiesEmptyTextReturnsEmpty)`
- Source: `tests/toolbox/test_toolbox_ingestion.cpp`:110
- Brief: n/a
- Parameters:
  - `<unnamed>` (IngestionToolbox): n/a
  - `<unnamed>` (IT09_ExtractEntitiesEmptyTextReturnsEmpty): n/a

#### `TEST(IngestionToolbox, IT10_ExtractEntitiesTextNoCrash)`
- Source: `tests/toolbox/test_toolbox_ingestion.cpp`:116
- Brief: n/a
- Parameters:
  - `<unnamed>` (IngestionToolbox): n/a
  - `<unnamed>` (IT10_ExtractEntitiesTextNoCrash): n/a

#### `TEST(IngestionToolbox, IT11_MetricsTrackEmptyExtractions)`
- Source: `tests/toolbox/test_toolbox_ingestion.cpp`:125
- Brief: n/a
- Parameters:
  - `<unnamed>` (IngestionToolbox): n/a
  - `<unnamed>` (IT11_MetricsTrackEmptyExtractions): n/a

#### `TEST(IngestionToolbox, IT12_ExtractEntitySetWithEmptyText)`
- Source: `tests/toolbox/test_toolbox_ingestion.cpp`:138
- Brief: n/a
- Parameters:
  - `<unnamed>` (IngestionToolbox): n/a
  - `<unnamed>` (IT12_ExtractEntitySetWithEmptyText): n/a

#### `TEST(LLMAQLHandlerBridge, LH01_SetNullBridgeAccepted)`
- Source: `tests/toolbox/test_toolbox_ingestion.cpp`:270
- Brief: n/a
- Parameters:
  - `<unnamed>` (LLMAQLHandlerBridge): n/a
  - `<unnamed>` (LH01_SetNullBridgeAccepted): n/a

#### `TEST(LLMAQLHandlerBridge, LH02_BridgeNullByDefault)`
- Source: `tests/toolbox/test_toolbox_ingestion.cpp`:275
- Brief: n/a
- Parameters:
  - `<unnamed>` (LLMAQLHandlerBridge): n/a
  - `<unnamed>` (LH02_BridgeNullByDefault): n/a

#### `TEST(LLMAQLHandlerBridge, LH03_SetBridgeStoredAndRetrievable)`
- Source: `tests/toolbox/test_toolbox_ingestion.cpp`:280
- Brief: n/a
- Parameters:
  - `<unnamed>` (LLMAQLHandlerBridge): n/a
  - `<unnamed>` (LH03_SetBridgeStoredAndRetrievable): n/a

### test_toolbox_phase5.cpp

#### `TEST(IngestionToolboxMetrics, ITM01_EmptyTextBeforeAnyCall)`
- Source: `tests/toolbox/test_toolbox_phase5.cpp`:56
- Brief: n/a
- Parameters:
  - `<unnamed>` (IngestionToolboxMetrics): n/a
  - `<unnamed>` (ITM01_EmptyTextBeforeAnyCall): n/a

#### `TEST(IngestionToolboxMetrics, ITM02_RecordExtractionIncrementsCounters)`
- Source: `tests/toolbox/test_toolbox_phase5.cpp`:62
- Brief: n/a
- Parameters:
  - `<unnamed>` (IngestionToolboxMetrics): n/a
  - `<unnamed>` (ITM02_RecordExtractionIncrementsCounters): n/a

#### `TEST(IngestionToolboxMetrics, ITM03_MetricsTextHasFourFamilies)`
- Source: `tests/toolbox/test_toolbox_phase5.cpp`:88
- Brief: n/a
- Parameters:
  - `<unnamed>` (IngestionToolboxMetrics): n/a
  - `<unnamed>` (ITM03_MetricsTextHasFourFamilies): n/a

#### `TEST(IngestionToolboxMetrics, ITM04_ErrorCounterOnlyOnFailure)`
- Source: `tests/toolbox/test_toolbox_phase5.cpp`:100
- Brief: n/a
- Parameters:
  - `<unnamed>` (IngestionToolboxMetrics): n/a
  - `<unnamed>` (ITM04_ErrorCounterOnlyOnFailure): n/a

#### `TEST(IngestionToolboxMetrics, ITM05_ExtractEntitiesAutoRecords)`
- Source: `tests/toolbox/test_toolbox_phase5.cpp`:112
- Brief: n/a
- Parameters:
  - `<unnamed>` (IngestionToolboxMetrics): n/a
  - `<unnamed>` (ITM05_ExtractEntitiesAutoRecords): n/a

#### `TEST(IngestionToolboxMetrics, ITM06_ExtractEntitySetAutoRecords)`
- Source: `tests/toolbox/test_toolbox_phase5.cpp`:131
- Brief: n/a
- Parameters:
  - `<unnamed>` (IngestionToolboxMetrics): n/a
  - `<unnamed>` (ITM06_ExtractEntitySetAutoRecords): n/a

#### `TEST(IngestionToolboxVectors, VEC01_ExtractEntitySetReturnsBaseEntitySet)`
- Source: `tests/toolbox/test_toolbox_phase5.cpp`:150
- Brief: n/a
- Parameters:
  - `<unnamed>` (IngestionToolboxVectors): n/a
  - `<unnamed>` (VEC01_ExtractEntitySetReturnsBaseEntitySet): n/a

#### `TEST(IngestionToolboxVectors, VEC02_BridgeResultVectorsPopulatedFromEntitySetChunks)`
- Source: `tests/toolbox/test_toolbox_phase5.cpp`:167
- Brief: n/a
- Parameters:
  - `<unnamed>` (IngestionToolboxVectors): n/a
  - `<unnamed>` (VEC02_BridgeResultVectorsPopulatedFromEntitySetChunks): n/a

#### `TEST(IngestionToolboxVectors, VEC03_VectorWriterCalledWhenBridgeResultHasVectors)`
- Source: `tests/toolbox/test_toolbox_phase5.cpp`:179
- Brief: n/a
- Parameters:
  - `<unnamed>` (IngestionToolboxVectors): n/a
  - `<unnamed>` (VEC03_VectorWriterCalledWhenBridgeResultHasVectors): n/a

#### `TEST_F(DegradedPathFixture, STRESS05_NullBackendSoftFail)`
- Source: `tests/toolbox/test_toolbox_phase5.cpp`:369
- Brief: n/a
- Parameters:
  - `<unnamed>` (DegradedPathFixture): n/a
  - `<unnamed>` (STRESS05_NullBackendSoftFail): n/a

#### `TEST_F(DegradedPathFixture, STRESS06_EmptyTextHandling)`
- Source: `tests/toolbox/test_toolbox_phase5.cpp`:377
- Brief: n/a
- Parameters:
  - `<unnamed>` (DegradedPathFixture): n/a
  - `<unnamed>` (STRESS06_EmptyTextHandling): n/a

#### `TEST_F(DegradedPathFixture, STRESS07_WhitespaceOnlyHandling)`
- Source: `tests/toolbox/test_toolbox_phase5.cpp`:387
- Brief: n/a
- Parameters:
  - `<unnamed>` (DegradedPathFixture): n/a
  - `<unnamed>` (STRESS07_WhitespaceOnlyHandling): n/a

#### `TEST_F(HighConcurrencyFixture, STRESS01_ConcurrentExtractionWithMetricsConsistency)`
- Source: `tests/toolbox/test_toolbox_phase5.cpp`:299
- Brief: n/a
- Parameters:
  - `<unnamed>` (HighConcurrencyFixture): n/a
  - `<unnamed>` (STRESS01_ConcurrentExtractionWithMetricsConsistency): n/a

#### `TEST_F(LongRunStressFixture, STRESS08_LongRunStabilityWithDeterministicSeeding)`
- Source: `tests/toolbox/test_toolbox_phase5.cpp`:402
- Brief: n/a
- Parameters:
  - `<unnamed>` (LongRunStressFixture): n/a
  - `<unnamed>` (STRESS08_LongRunStabilityWithDeterministicSeeding): n/a

#### `TEST_F(LongRunStressFixture, STRESS09_MetricsConsistencyUnderLongRun)`
- Source: `tests/toolbox/test_toolbox_phase5.cpp`:433
- Brief: n/a
- Parameters:
  - `<unnamed>` (LongRunStressFixture): n/a
  - `<unnamed>` (STRESS09_MetricsConsistencyUnderLongRun): n/a

#### `TEST_F(MixedContentFixture, STRESS02_TextOnlyScenario)`
- Source: `tests/toolbox/test_toolbox_phase5.cpp`:342
- Brief: n/a
- Parameters:
  - `<unnamed>` (MixedContentFixture): n/a
  - `<unnamed>` (STRESS02_TextOnlyScenario): n/a

#### `TEST_F(MixedContentFixture, STRESS03_BinaryWithMetadataScenario)`
- Source: `tests/toolbox/test_toolbox_phase5.cpp`:349
- Brief: n/a
- Parameters:
  - `<unnamed>` (MixedContentFixture): n/a
  - `<unnamed>` (STRESS03_BinaryWithMetadataScenario): n/a

#### `TEST_F(MixedContentFixture, STRESS04_MalformedContentScenario)`
- Source: `tests/toolbox/test_toolbox_phase5.cpp`:356
- Brief: n/a
- Parameters:
  - `<unnamed>` (MixedContentFixture): n/a
  - `<unnamed>` (STRESS04_MalformedContentScenario): n/a

#### `TEST_F(ToolboxRegistryTest, REG01_InstanceThrowsBeforeInitialize)`
- Source: `tests/toolbox/test_toolbox_phase5.cpp`:214
- Brief: n/a
- Parameters:
  - `<unnamed>` (ToolboxRegistryTest): n/a
  - `<unnamed>` (REG01_InstanceThrowsBeforeInitialize): n/a

#### `TEST_F(ToolboxRegistryTest, REG02_InitializeAndInstanceRoundTrip)`
- Source: `tests/toolbox/test_toolbox_phase5.cpp`:219
- Brief: n/a
- Parameters:
  - `<unnamed>` (ToolboxRegistryTest): n/a
  - `<unnamed>` (REG02_InitializeAndInstanceRoundTrip): n/a

#### `TEST_F(ToolboxRegistryTest, REG03_IsInitializedReflectsState)`
- Source: `tests/toolbox/test_toolbox_phase5.cpp`:229
- Brief: n/a
- Parameters:
  - `<unnamed>` (ToolboxRegistryTest): n/a
  - `<unnamed>` (REG03_IsInitializedReflectsState): n/a

#### `TEST_F(ToolboxRegistryTest, REG04_GlobalToolboxFreeFunction)`
- Source: `tests/toolbox/test_toolbox_phase5.cpp`:236
- Brief: n/a
- Parameters:
  - `<unnamed>` (ToolboxRegistryTest): n/a
  - `<unnamed>` (REG04_GlobalToolboxFreeFunction): n/a

#### `TEST_F(ToolboxRegistryTest, REG05_ExtractEntitiesFreeFunctionDelegatesToRegistry)`
- Source: `tests/toolbox/test_toolbox_phase5.cpp`:244
- Brief: n/a
- Parameters:
  - `<unnamed>` (ToolboxRegistryTest): n/a
  - `<unnamed>` (REG05_ExtractEntitiesFreeFunctionDelegatesToRegistry): n/a

#### `TEST_F(ToolboxRegistryTest, REG06_GetMetricsTextFreeFunctionDelegatesToRegistry)`
- Source: `tests/toolbox/test_toolbox_phase5.cpp`:257
- Brief: n/a
- Parameters:
  - `<unnamed>` (ToolboxRegistryTest): n/a
  - `<unnamed>` (REG06_GetMetricsTextFreeFunctionDelegatesToRegistry): n/a

#### `TEST_F(ToolboxRegistryTest, REG07_InitializeWithNullThrows)`
- Source: `tests/toolbox/test_toolbox_phase5.cpp`:268
- Brief: n/a
- Parameters:
  - `<unnamed>` (ToolboxRegistryTest): n/a
  - `<unnamed>` (REG07_InitializeWithNullThrows): n/a

#### `TEST_F(ToolboxRegistryTest, REG08_ResetClearsRegistry)`
- Source: `tests/toolbox/test_toolbox_phase5.cpp`:274
- Brief: n/a
- Parameters:
  - `<unnamed>` (ToolboxRegistryTest): n/a
  - `<unnamed>` (REG08_ResetClearsRegistry): n/a

### themis::bench::toolbox_dedicated

#### `void BM_Toolbox_BatchComposite(benchmark::State &state)`
- Source: `benchmarks/toolbox/bench_toolbox_dedicated_gates.cpp`:119
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_Toolbox_ContentBridge(benchmark::State &state)`
- Source: `benchmarks/toolbox/bench_toolbox_dedicated_gates.cpp`:87
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_Toolbox_ExtractionPipeline(benchmark::State &state)`
- Source: `benchmarks/toolbox/bench_toolbox_dedicated_gates.cpp`:103
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_Toolbox_ToolDispatch(benchmark::State &state)`
- Source: `benchmarks/toolbox/bench_toolbox_dedicated_gates.cpp`:70
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `Name("TB-BM-01/ToolDispatch") -> Repetitions(kRepetitions) ->ReportAggregatesOnly(true)`
- Source: `benchmarks/toolbox/bench_toolbox_dedicated_gates.cpp`:80
- Brief: n/a
- Parameters:
  - `<unnamed>` ("TB-BM-01/ToolDispatch"): n/a

#### `Name("TB-BM-02/ContentBridge") -> Repetitions(kRepetitions) ->ReportAggregatesOnly(true)`
- Source: `benchmarks/toolbox/bench_toolbox_dedicated_gates.cpp`:96
- Brief: n/a
- Parameters:
  - `<unnamed>` ("TB-BM-02/ContentBridge"): n/a

#### `Name("TB-BM-03/ExtractionPipeline") -> Repetitions(kRepetitions) ->ReportAggregatesOnly(true)`
- Source: `benchmarks/toolbox/bench_toolbox_dedicated_gates.cpp`:112
- Brief: n/a
- Parameters:
  - `<unnamed>` ("TB-BM-03/ExtractionPipeline"): n/a

#### `Name("TB-BM-04/BatchComposite1000") -> Repetitions(kRepetitions) ->ReportAggregatesOnly(true)`
- Source: `benchmarks/toolbox/bench_toolbox_dedicated_gates.cpp`:135
- Brief: n/a
- Parameters:
  - `<unnamed>` ("TB-BM-04/BatchComposite1000"): n/a

#### `bool stubBridge(uint64_t content_id) noexcept`
- Source: `benchmarks/toolbox/bench_toolbox_dedicated_gates.cpp`:58
- Brief: n/a
- Parameters:
  - `content_id` (uint64_t): n/a

#### `bool stubDispatch(const std::string &tool, const std::string &content) noexcept`
- Source: `benchmarks/toolbox/bench_toolbox_dedicated_gates.cpp`:53
- Brief: n/a
- Parameters:
  - `tool` (const std::string &): n/a
  - `content` (const std::string &): n/a

#### `std::size_t stubExtract(const std::string &doc_id) noexcept`
- Source: `benchmarks/toolbox/bench_toolbox_dedicated_gates.cpp`:63
- Brief: n/a
- Parameters:
  - `doc_id` (const std::string &): n/a

### themis::toolbox

#### `std::vector< std::string > chunkText(const std::string &text, std::size_t chunk_size=512, std::size_t overlap=64)`
- Source: `src/toolbox/text_chunker.cpp`:92
- Brief: Split text into overlapping chunks using sentence-boundary strategy.
- Parameters:
  - `text` (const std::string &): Input parameter.
  - `chunk_size` (std::size_t): Input parameter.
  - `overlap` (std::size_t): Input parameter.
- Return: Ordered vector of chunk text strings.
- Details: Chunk Text. Convenience free function suitable for one-off chunking without managing a TextChunker instance. text UTF-8 text to split. chunk_size Target chunk size in estimated tokens (default: 512). overlap Token overlap between consecutive chunks (default: 64). Ordered vector of chunk text strings. text Input parameter. chunk_size Input parameter. overlap Input parameter. Return value.

#### `std::string detectLanguage(std::string_view text)`
- Source: `src/toolbox/language_detector.cpp`:98
- Brief: Detect the primary language of text using the default heuristic.
- Parameters:
  - `text` (std::string_view): Input parameter.
- Return: ISO 639-1 language code ("en", "de") or "und" when undetermined.
- Details: Detect Language. Convenience free function. Equivalent to DefaultLanguageDetector{}.detect(text). text UTF-8 text sample. ISO 639-1 language code ("en", "de") or "und" when undetermined. text Input parameter. Return value. Calls: detect().

#### `std::vector< ingestion::BaseEntity > extractEntities(const std::string &text, const std::string &mime="text/plain", const std::string &filename="input.txt")`
- Source: `src/toolbox/toolbox_registry.cpp`:128
- Brief: Extract entities from text using the global IngestionToolbox.
- Parameters:
  - `text` (const std::string &): Input parameter.
  - `mime` (const std::string &): Input parameter.
  - `filename` (const std::string &): Input parameter.
- Return: Extracted entity nodes; empty on failure.
- Throws:
  - std::logic_error: when the registry is not initialised.
- Details: Extract Entities. Delegates to ToolboxRegistry::instance()->extractEntities(text, mime, filename). text UTF-8 text to process. mime MIME type hint (default: "text/plain"). filename Filename hint (default: "input.txt"). Extracted entity nodes; empty on failure. std::logic_error when the registry is not initialised. text Input parameter. mime Input parameter. filename Input parameter. Return value.

#### `void extractEntitiesStream(IngestionToolbox &toolbox, const std::string &text, const EntityCallback &callback)`
- Source: `include/toolbox/toolbox_streaming.h`:58
- Brief: Overload with default MIME / filename.
- Parameters:
  - `toolbox` (IngestionToolbox &): n/a
  - `text` (const std::string &): n/a
  - `callback` (const EntityCallback &): n/a

#### `void extractEntitiesStream(IngestionToolbox &toolbox, const std::string &text, const std::string &mime, const std::string &filename, const EntityCallback &callback)`
- Source: `src/toolbox/toolbox_streaming.cpp`:30
- Brief: Chunk text and stream extracted entities to callback.
- Parameters:
  - `toolbox` (IngestionToolbox &): Input/output parameter.
  - `text` (const std::string &): Input parameter.
  - `mime` (const std::string &): Input parameter.
  - `filename` (const std::string &): Input parameter.
  - `callback` (const EntityCallback &): Input parameter.
- Details: Extract Entities Stream. Algorithm: Split text with TextChunker (default config: chunk_size=512, overlap=64, strategy=Sentence). For each chunk, call toolbox.extractEntities(chunk_text, mime, filename). Invoke callback once per extracted BaseEntity, in chunk order. toolbox Injected IngestionToolbox to use for extraction. text UTF-8 text to process. mime MIME type hint forwarded to the toolbox (default: "text/plain"). filename Filename hint forwarded to the toolbox (default: "input.txt"). callback Called once per entity, never called on empty results. The callback must not throw; exceptions propagate to the caller but leave the streaming loop in an unspecified state. toolbox Input/output parameter. text Input parameter. mime Input parameter. filename Input parameter. callback Input parameter.

#### `void extractEntitiesStream(const std::string &text, const EntityCallback &callback)`
- Source: `include/toolbox/toolbox_streaming.h`:92
- Brief: Overload with default MIME / filename.
- Parameters:
  - `text` (const std::string &): n/a
  - `callback` (const EntityCallback &): n/a

#### `void extractEntitiesStream(const std::string &text, const std::string &mime, const std::string &filename, const EntityCallback &callback)`
- Source: `src/toolbox/toolbox_streaming.cpp`:66
- Brief: Stream entities from text using the global ToolboxRegistry.
- Parameters:
  - `text` (const std::string &): Input parameter.
  - `mime` (const std::string &): Input parameter.
  - `filename` (const std::string &): Input parameter.
  - `callback` (const EntityCallback &): Input parameter.
- Throws:
  - std::logic_error: when the registry is not initialised.
- Details: Extract Entities Stream. Delegates to ToolboxRegistry::instance()->extractEntities() per chunk. text UTF-8 text to process. mime MIME type hint (default: "text/plain"). filename Filename hint (default: "input.txt"). callback Invoked once per extracted entity. std::logic_error when the registry is not initialised. text Input parameter. mime Input parameter. filename Input parameter. callback Input parameter.

#### `ingestion::BaseEntitySet extractEntitySet(const std::string &text, const std::string &mime="text/plain", const std::string &filename="input.txt")`
- Source: `src/toolbox/toolbox_registry.cpp`:143
- Brief: Extract the full BaseEntitySet from text using the global toolbox.
- Parameters:
  - `text` (const std::string &): Input parameter.
  - `mime` (const std::string &): Input parameter.
  - `filename` (const std::string &): Input parameter.
- Return: Full entity set including nodes, edges, and vector chunks.
- Throws:
  - std::logic_error: when the registry is not initialised.
- Details: Extract Entity Set. Delegates to ToolboxRegistry::instance()->extractEntitySet(text, mime, filename). text UTF-8 text to process. mime MIME type hint (default: "text/plain"). filename Filename hint (default: "input.txt"). Full entity set including nodes, edges, and vector chunks. std::logic_error when the registry is not initialised. text Input parameter. mime Input parameter. filename Input parameter. Return value.

#### `ContentFingerprint fingerprint(std::string_view text)`
- Source: `src/toolbox/content_fingerprinter.cpp`:106
- Brief: Compute a content fingerprint for text.
- Parameters:
  - `text` (std::string_view): Input parameter.
- Return: Populated ContentFingerprint.
- Details: Fingerprint. Convenience free function. Equivalent to ContentFingerprinter{}.compute(text). text UTF-8 text or raw bytes. Populated ContentFingerprint. text Input parameter. Return value. Calls: compute().

#### `std::string getMetricsText()`
- Source: `src/toolbox/toolbox_registry.cpp`:156
- Brief: Produce Prometheus text-format metrics for the global toolbox.
- Parameters: none
- Return: Prometheus text payload, or "" if no calls have been recorded.
- Throws:
  - std::logic_error: when the registry is not initialised.
- Details: Get Metrics Text. Delegates to ToolboxRegistry::instance()->getMetricsText(). Prometheus text payload, or "" if no calls have been recorded. std::logic_error when the registry is not initialised. Return value. Calls: ToolboxRegistry::instance(), load(), str().

#### `std::string getTextChunkerMetrics()`
- Source: `src/toolbox/text_chunker.cpp`:116
- Brief: ───────────────────────────────────────────────────────────────────────────── Phase 3: Metrics export for helper diagnostics ─────────────────────────────────────────────────────────────────────────────
- Parameters: none
- Return: Return value.
- Details: Return value. Calls: load(), str().

#### `std::string getTextNormalizerMetrics()`
- Source: `src/toolbox/text_normalizer.cpp`:57
- Brief: ───────────────────────────────────────────────────────────────────────────── Phase 3: Metrics export for helper diagnostics ─────────────────────────────────────────────────────────────────────────────
- Parameters: none
- Return: Return value.
- Details: Return value. Calls: load(), str().

#### `std::string getTextQualityScorerMetrics()`
- Source: `src/toolbox/text_quality_scorer.cpp`:140
- Brief: ───────────────────────────────────────────────────────────────────────────── Phase 3: Metrics export for helper diagnostics ─────────────────────────────────────────────────────────────────────────────
- Parameters: none
- Return: Return value.
- Details: Return value. Calls: load(), str().

#### `std::shared_ptr< IngestionToolbox > globalToolbox()`
- Source: `src/toolbox/toolbox_registry.cpp`:117
- Brief: Return the process-global toolbox. Convenience alias for ToolboxRegistry::instance().
- Parameters: none
- Return: Return value.
- Details: Global Toolbox. Return value. Calls: ToolboxRegistry::instance().

#### `void initializeToolbox(std::shared_ptr< IngestionToolbox > toolbox)`
- Source: `src/toolbox/toolbox_registry.cpp`:108
- Brief: Register the process-global toolbox. Convenience alias for ToolboxRegistry::initialize(toolbox).
- Parameters:
  - `toolbox` (std::shared_ptr< IngestionToolbox >): Input parameter.
- Details: Initialize Toolbox. toolbox Input parameter. Calls: ToolboxRegistry::initialize(), std::move().

#### `std::string normalizeText(std::string_view text)`
- Source: `src/toolbox/text_normalizer.cpp`:47
- Brief: Normalise German umlauts and ß in text.
- Parameters:
  - `text` (std::string_view): Input parameter.
- Return: Normalised UTF-8 string (umlaut-folded to ASCII equivalents).
- Details: Normalize Text. Convenience free function that delegates to utils::Normalizer::normalizeUmlauts. text UTF-8 input text. Normalised UTF-8 string (umlaut-folded to ASCII equivalents). text Input parameter. Return value. Calls: utils::Normalizer::normalizeUmlauts().

#### `ingestion::BaseEntitySet runWorkflow(const std::shared_ptr< ingestion::WorkflowEngine > &engine, const std::string &text, const std::string &mime, const std::string &filename)`
- Source: `src/toolbox/ingestion_toolbox.cpp`:172
- Brief: Shared helper: build an ExtractionContext and run the workflow.
- Parameters:
  - `engine` (const std::shared_ptr< ingestion::WorkflowEngine > &): Input parameter.
  - `text` (const std::string &): Input parameter.
  - `mime` (const std::string &): Input parameter.
  - `filename` (const std::string &): Input parameter.
- Return: Return value.
- Details: engine Input parameter. text Input parameter. mime Input parameter. filename Input parameter. Return value. Returns the BaseEntitySet on success, empty set on failure.

#### `TextQualityScore scoreText(std::string_view text)`
- Source: `src/toolbox/text_quality_scorer.cpp`:130
- Brief: Compute a quality score for text.
- Parameters:
  - `text` (std::string_view): Input parameter.
- Return: Populated TextQualityScore.
- Details: Score Text. Convenience free function. Equivalent to TextQualityScorer{}.score(text). text UTF-8 text to evaluate. Populated TextQualityScore. text Input parameter. Return value. Calls: score().

### themis::toolbox::ContentFingerprint

#### `bool operator!=(const ContentFingerprint &o) const noexcept`
- Source: `include/toolbox/content_fingerprinter.h`:51
- Brief: n/a
- Parameters:
  - `o` (const ContentFingerprint &): n/a

#### `bool operator==(const ContentFingerprint &o) const noexcept`
- Source: `include/toolbox/content_fingerprinter.h`:48
- Brief: Equality is defined by the SHA-256 digest only.
- Parameters:
  - `o` (const ContentFingerprint &): n/a

#### `bool valid() const noexcept`
- Source: `include/toolbox/content_fingerprinter.h`:45
- Brief: n/a
- Parameters: none
- Return: true when sha256_hex is non-empty (i.e. input was non-empty and fingerprinting succeeded).
- Details: true when sha256_hex is non-empty (i.e. input was non-empty and fingerprinting succeeded).

### themis::toolbox::ContentFingerprinter

#### `ContentFingerprinter()=default`
- Source: `include/toolbox/content_fingerprinter.h`:72
- Brief: n/a
- Parameters: none

#### `ContentFingerprinter(const ContentFingerprinter &)=default`
- Source: `include/toolbox/content_fingerprinter.h`:75
- Brief: n/a
- Parameters:
  - `<unnamed>` (const ContentFingerprinter &): n/a

#### `ContentFingerprint compute(const unsigned char *data, std::size_t len) const`
- Source: `include/toolbox/content_fingerprinter.h`:94
- Brief: Compute a ContentFingerprint for the given raw bytes.
- Parameters:
  - `data` (const unsigned char *): Pointer to the first byte.
  - `len` (std::size_t): Number of bytes.
- Return: Populated fingerprint.
- Details: data Pointer to the first byte. len Number of bytes. Populated fingerprint.

#### `ContentFingerprint compute(std::string_view text) const`
- Source: `include/toolbox/content_fingerprinter.h`:85
- Brief: Compute a ContentFingerprint for the given UTF-8 text.
- Parameters:
  - `text` (std::string_view): UTF-8 text or raw byte content.
- Return: Populated fingerprint. sha256_hex is empty only when text is empty.
- Details: text UTF-8 text or raw byte content. Populated fingerprint. sha256_hex is empty only when text is empty.

#### `ContentFingerprinter & operator=(const ContentFingerprinter &)=default`
- Source: `include/toolbox/content_fingerprinter.h`:76
- Brief: n/a
- Parameters:
  - `<unnamed>` (const ContentFingerprinter &): n/a

#### `~ContentFingerprinter()=default`
- Source: `include/toolbox/content_fingerprinter.h`:73
- Brief: n/a
- Parameters: none

### themis::toolbox::ContentToolboxBridge

#### `ContentToolboxBridge(ContentToolboxBridge &&) noexcept`
- Source: `include/toolbox/content_toolbox_bridge.h`:110
- Brief: n/a
- Parameters:
  - `<unnamed>` (ContentToolboxBridge &&): n/a

#### `ContentToolboxBridge(const ContentToolboxBridge &)=delete`
- Source: `include/toolbox/content_toolbox_bridge.h`:108
- Brief: n/a
- Parameters:
  - `<unnamed>` (const ContentToolboxBridge &): n/a

#### `ContentToolboxBridge(std::shared_ptr< IngestionToolbox > toolbox, std::shared_ptr< content::ContentManager > content_manager, std::shared_ptr< ingestion::IGraphWriter > graph_writer=nullptr, std::shared_ptr< ingestion::IVectorWriter > vector_writer=nullptr)`
- Source: `include/toolbox/content_toolbox_bridge.h`:98
- Brief: Construct the bridge.
- Parameters:
  - `toolbox` (std::shared_ptr< IngestionToolbox >): Configured IngestionToolbox for enrichment. Must not be null.
  - `content_manager` (std::shared_ptr< content::ContentManager >): Initialised ContentManager for storage. Must not be null.
  - `graph_writer` (std::shared_ptr< ingestion::IGraphWriter >): Optional sink for NER graph entities.
  - `vector_writer` (std::shared_ptr< ingestion::IVectorWriter >): Optional sink for embedding vectors.
- Details: toolbox Configured IngestionToolbox for enrichment. Must not be null. content_manager Initialised ContentManager for storage. Must not be null. graph_writer Optional sink for NER graph entities. vector_writer Optional sink for embedding vectors.

#### `std::shared_ptr< content::ContentManager > contentManager() const`
- Source: `include/toolbox/content_toolbox_bridge.h`:196
- Brief: n/a
- Parameters: none

#### `BridgeResult enrichExisting(const std::string &content_id, const std::string &collection="default")`
- Source: `include/toolbox/content_toolbox_bridge.h`:186
- Brief: Run the enrichment pipeline for content already stored in ContentManager (re-enrichment path).
- Parameters:
  - `content_id` (const std::string &): Identifier of the content.
  - `collection` (const std::string &): Input parameter.
- Return: BridgeResult with entities, vectors, and ok/error. content_id in the result is set to the input content_id.
- Details: Enrich Existing. Retrieves the extracted text for content_id from ContentManager, runs the Toolbox enrichment pipeline, and writes to the sinks. Does NOT re-ingest the raw blob (no security re-scan, no dedup check). content_id ID previously returned by ingest() or an earlier ContentManager::ingestRawBlob() call. collection Target collection for the document store sink. BridgeResult with entities, vectors, and ok/error. content_id in the result is set to the input content_id. content_id Identifier of the content. collection Input parameter. Return value.

#### `uint64_t failuresTotal() const noexcept`
- Source: `include/toolbox/content_toolbox_bridge.h`:212
- Brief: Return cumulative count of bridge ingest/enrichment failures.
- Parameters: none
- Details: Used for Prometheus metric toolbox_bridge_failures_total. Incremented on ContentManager failures, null checks, or other bridge-level errors. Does not count individual sink write failures (those are tracked separately).

#### `std::string getMetricsText() const`
- Source: `include/toolbox/content_toolbox_bridge.h`:238
- Brief: Export all bridge metrics in Prometheus text format.
- Parameters: none
- Details: Returns empty string if no operations have been recorded yet. Includes failure counters and latency histogram buckets.

#### `uint64_t graphWriteFailuresTotal() const noexcept`
- Source: `include/toolbox/content_toolbox_bridge.h`:221
- Brief: Return cumulative count of graph writer failures.
- Parameters: none
- Details: Used for Prometheus metric toolbox_bridge_graph_write_failures_total. Incremented when an entity write to the graph sink fails, but the bridge continues processing (soft-fail behavior).

#### `std::shared_ptr< ingestion::IGraphWriter > graphWriter() const`
- Source: `include/toolbox/content_toolbox_bridge.h`:197
- Brief: n/a
- Parameters: none

#### `BridgeResult ingest(std::span< const std::byte > data, const std::string &filename, const std::string &mime_type="", const std::string &collection="default", const std::string &user_context="")`
- Source: `include/toolbox/content_toolbox_bridge.h`:164
- Brief: Ingest raw bytes through both ContentManager and IngestionToolbox.
- Parameters:
  - `data` (std::span< const std::byte >): Input parameter.
  - `filename` (const std::string &): Input parameter.
  - `mime_type` (const std::string &): Input parameter.
  - `collection` (const std::string &): Input parameter.
  - `user_context` (const std::string &): Input parameter.
- Return: BridgeResult with content_id, entities, vectors, and ok/error.
- Details: Ingest. Steps performed in order: ContentManager::ingestRawBlob() — security scan, dedup, store blob, generate chunks + embeddings. Retrieve extracted text from ContentManager result. IngestionToolbox::extractEntities() — NER/deontic/graph assembly. Write entities to graph_writer if set. Write vector records to vector_writer if set. Return combined BridgeResult. data Raw binary content to ingest (any format). filename Original filename hint (used for MIME detection and archive member naming), e.g. "report.pdf". mime_type Optional MIME type override. When empty, ContentManager auto-detects via magic bytes + extension. collection Target collection name for the document store sink. Default: "default". user_context User context string forwarded to ContentManager for per-user encryption and access control. BridgeResult with content_id, entities, vectors, and ok/error. data Input parameter. filename Input parameter. mime_type Input parameter. collection Input parameter. user_context Input parameter. Return value.

#### `ContentToolboxBridge & operator=(ContentToolboxBridge &&) noexcept`
- Source: `include/toolbox/content_toolbox_bridge.h`:111
- Brief: n/a
- Parameters:
  - `<unnamed>` (ContentToolboxBridge &&): n/a

#### `ContentToolboxBridge & operator=(const ContentToolboxBridge &)=delete`
- Source: `include/toolbox/content_toolbox_bridge.h`:109
- Brief: n/a
- Parameters:
  - `<unnamed>` (const ContentToolboxBridge &): n/a

#### `void recordLatency(uint64_t latency_ms) noexcept`
- Source: `include/toolbox/content_toolbox_bridge.h`:250
- Brief: Record operation latency for metrics histogram.
- Parameters:
  - `latency_ms` (uint64_t): Operation latency in milliseconds
- Details: Used internally by ingest() and enrichExisting() to populate latency buckets. latency_ms Operation latency in milliseconds

#### `std::shared_ptr< IngestionToolbox > toolbox() const`
- Source: `include/toolbox/content_toolbox_bridge.h`:195
- Brief: n/a
- Parameters: none

#### `uint64_t vectorWriteFailuresTotal() const noexcept`
- Source: `include/toolbox/content_toolbox_bridge.h`:230
- Brief: Return cumulative count of vector writer failures.
- Parameters: none
- Details: Used for Prometheus metric toolbox_bridge_vector_write_failures_total. Incremented when a vector record write to the vector sink fails, but the bridge continues processing (soft-fail behavior).

#### `std::shared_ptr< ingestion::IVectorWriter > vectorWriter() const`
- Source: `include/toolbox/content_toolbox_bridge.h`:198
- Brief: n/a
- Parameters: none

#### `~ContentToolboxBridge()`
- Source: `include/toolbox/content_toolbox_bridge.h`:105
- Brief: n/a
- Parameters: none

### themis::toolbox::ContentToolboxBridge::Impl

#### `Impl(std::shared_ptr< IngestionToolbox > toolbox, std::shared_ptr< content::ContentManager > content_manager, std::shared_ptr< ingestion::IGraphWriter > graph_writer, std::shared_ptr< ingestion::IVectorWriter > vector_writer)`
- Source: `src/toolbox/content_toolbox_bridge.cpp`:30
- Brief: n/a
- Parameters:
  - `toolbox` (std::shared_ptr< IngestionToolbox >): n/a
  - `content_manager` (std::shared_ptr< content::ContentManager >): n/a
  - `graph_writer` (std::shared_ptr< ingestion::IGraphWriter >): n/a
  - `vector_writer` (std::shared_ptr< ingestion::IVectorWriter >): n/a

### themis::toolbox::DefaultLanguageDetector

#### `DefaultLanguageDetector()`
- Source: `include/toolbox/language_detector.h`:82
- Brief: Construct with default confidence threshold (0.05 = 5 % of words must be stopwords).
- Parameters: none

#### `DefaultLanguageDetector(const DefaultLanguageDetector &)=default`
- Source: `include/toolbox/language_detector.h`:94
- Brief: n/a
- Parameters:
  - `<unnamed>` (const DefaultLanguageDetector &): n/a

#### `DefaultLanguageDetector(double min_ratio)`
- Source: `include/toolbox/language_detector.h`:90
- Brief: Construct with a custom confidence threshold.
- Parameters:
  - `min_ratio` (double): Minimum ratio of stopword matches to total word count required to assert a language. Range [0.0, 1.0].
- Details: min_ratio Minimum ratio of stopword matches to total word count required to assert a language. Range [0.0, 1.0].

#### `std::string detect(std::string_view text) const override`
- Source: `include/toolbox/language_detector.h`:98
- Brief: Detect the primary language of text.
- Parameters:
  - `text` (std::string_view): UTF-8 text sample. For reliable results, supply at least 100 characters.
- Return: ISO 639-1 language code (e.g. "en", "de") or "und" when the language cannot be determined.
- Details: text UTF-8 text sample. For reliable results, supply at least 100 characters. ISO 639-1 language code (e.g. "en", "de") or "und" when the language cannot be determined.

#### `DefaultLanguageDetector & operator=(const DefaultLanguageDetector &)=default`
- Source: `include/toolbox/language_detector.h`:95
- Brief: n/a
- Parameters:
  - `<unnamed>` (const DefaultLanguageDetector &): n/a

#### `~DefaultLanguageDetector() override=default`
- Source: `include/toolbox/language_detector.h`:92
- Brief: n/a
- Parameters: none

### themis::toolbox::ILanguageDetector

#### `std::string detect(std::string_view text) const =0`
- Source: `include/toolbox/language_detector.h`:40
- Brief: Detect the primary language of text.
- Parameters:
  - `text` (std::string_view): UTF-8 text sample. For reliable results, supply at least 100 characters.
- Return: ISO 639-1 language code (e.g. "en", "de") or "und" when the language cannot be determined.
- Details: text UTF-8 text sample. For reliable results, supply at least 100 characters. ISO 639-1 language code (e.g. "en", "de") or "und" when the language cannot be determined.

#### `~ILanguageDetector()=default`
- Source: `include/toolbox/language_detector.h`:30
- Brief: n/a
- Parameters: none

### themis::toolbox::IngestionToolbox

#### `IngestionToolbox()`
- Source: `include/toolbox/ingestion_toolbox.h`:77
- Brief: n/a
- Parameters: none

#### `IngestionToolbox(IngestionToolbox &&) noexcept`
- Source: `include/toolbox/ingestion_toolbox.h`:83
- Brief: n/a
- Parameters:
  - `<unnamed>` (IngestionToolbox &&): n/a

#### `IngestionToolbox(const IngestionToolbox &)=delete`
- Source: `include/toolbox/ingestion_toolbox.h`:81
- Brief: n/a
- Parameters:
  - `<unnamed>` (const IngestionToolbox &): n/a

#### `std::shared_ptr< IngestionToolbox > createDefault()`
- Source: `include/toolbox/ingestion_toolbox.h`:96
- Brief: Create an IngestionToolbox pre-configured with all built-in steps and a NullTextGenerationBackend.
- Parameters: none
- Return: Return value.
- Details: ── Factory ────────────────────────────────────────────────────────────────── The returned toolbox is ready for immediate use. Inject a real ITextGenerationBackend via setTextBackend() to enable LLM-backed NER and entity extraction. Return value. Calls: stepRegistry(), registerStep(), ingestion::builtin::createNerDeStep(), textBackend(), ingestion::builtin::createLlmExtractStep(), ingestion::builtin::createChunkTtDecomposeStep(), ingestion::builtin::createTensorCoreBridgeStep().

#### `std::vector< ingestion::BaseEntity > extractEntities(const std::string &text, const std::string &mime="text/plain", const std::string &filename="input.txt")`
- Source: `include/toolbox/ingestion_toolbox.h`:159
- Brief: Extract entities from a plain text string.
- Parameters:
  - `text` (const std::string &): Input parameter.
  - `mime` (const std::string &): Input parameter.
  - `filename` (const std::string &): Input parameter.
- Return: Extracted and normalised entity nodes; empty on workflow failure (failure details are logged at warn level).
- Details: Extract Entities. Constructs a minimal ExtractionContext from text, selects the best matching workflow profile (using MIME type mime and filename hint filename), runs the workflow, and returns the assembled entity nodes. This is the primary entry point for consumers that need entity extraction without managing ExtractionContext directly. text UTF-8 text to process (may be empty — returns {}). mime Detected MIME type hint (default: "text/plain"). filename Filename hint used for profile selection (default: "input.txt"). Extracted and normalised entity nodes; empty on workflow failure (failure details are logged at warn level). text Input parameter. mime Input parameter. filename Input parameter. Return value.

#### `ingestion::BaseEntitySet extractEntitySet(const std::string &text, const std::string &mime="text/plain", const std::string &filename="input.txt")`
- Source: `include/toolbox/ingestion_toolbox.h`:180
- Brief: Extract the full BaseEntitySet from a plain text string.
- Parameters:
  - `text` (const std::string &): Input parameter.
  - `mime` (const std::string &): Input parameter.
  - `filename` (const std::string &): Input parameter.
- Return: Full assembled entity set; empty BaseEntitySet on failure.
- Details: Extract Entity Set. Like extractEntities() but returns the complete BaseEntitySet including nodes, edges, and chunks (vector index entries). Consumers that need both the graph entities and the embedding chunks (e.g. ContentToolboxBridge for BridgeResult::vectors) should call this method instead of extractEntities(). text UTF-8 text to process (may be empty — returns {}). mime Detected MIME type hint (default: "text/plain"). filename Filename hint used for profile selection (default: "input.txt"). Full assembled entity set; empty BaseEntitySet on failure. text Input parameter. mime Input parameter. filename Input parameter. Return value.

#### `std::string getMetricsText() const`
- Source: `include/toolbox/ingestion_toolbox.h`:216
- Brief: Produce Prometheus text-format metrics.
- Parameters: none
- Return: Prometheus text payload, or "" if unused.
- Details: Emits the following metric families (Prometheus text v0.0.4): toolbox_extract_calls_total counter — total extractEntities() calls toolbox_extract_errors_total counter — failed calls toolbox_extract_entities_total counter — cumulative entity count toolbox_extract_latency_ms_total counter — cumulative latency Returns an empty string when no calls have been recorded. Prometheus text payload, or "" if unused.

#### `IngestionToolbox & operator=(IngestionToolbox &&) noexcept`
- Source: `include/toolbox/ingestion_toolbox.h`:84
- Brief: n/a
- Parameters:
  - `<unnamed>` (IngestionToolbox &&): n/a

#### `IngestionToolbox & operator=(const IngestionToolbox &)=delete`
- Source: `include/toolbox/ingestion_toolbox.h`:82
- Brief: n/a
- Parameters:
  - `<unnamed>` (const IngestionToolbox &): n/a

#### `void recordExtraction(std::size_t entity_count, uint64_t latency_ms, bool success) noexcept`
- Source: `include/toolbox/ingestion_toolbox.h`:199
- Brief: Record one completed extractEntities() call in the metrics counters.
- Parameters:
  - `entity_count` (std::size_t): Number of entities returned (0 on failure).
  - `latency_ms` (uint64_t): Wall-clock latency of the call in milliseconds.
  - `success` (bool): Whether the workflow completed without error.
- Details: Call this after every extractEntities() / extractEntitySet() invocation (including error paths). Thread-safe; uses std::atomic. entity_count Number of entities returned (0 on failure). latency_ms Wall-clock latency of the call in milliseconds. success Whether the workflow completed without error.

#### `void setTextBackend(std::shared_ptr< ingestion::ITextGenerationBackend > backend)`
- Source: `include/toolbox/ingestion_toolbox.h`:114
- Brief: Inject or replace the text-generation backend.
- Parameters:
  - `backend` (std::shared_ptr< ingestion::ITextGenerationBackend >): Input parameter.
- Details: Set Text Backend. When backend is nullptr, a NullTextGenerationBackend is reinstated. The new backend is propagated to all NER/LLM steps that have been registered in the StepRegistry. backend Input parameter.

#### `void setWorkflowEngine(std::shared_ptr< ingestion::WorkflowEngine > engine)`
- Source: `include/toolbox/ingestion_toolbox.h`:105
- Brief: Replace the WorkflowEngine.
- Parameters:
  - `engine` (std::shared_ptr< ingestion::WorkflowEngine >): Input parameter.
- Details: ── Dependency injection ────────────────────────────────────────────────────── engine Must not be null. engine Input parameter.

#### `ingestion::StepRegistry & stepRegistry()`
- Source: `include/toolbox/ingestion_toolbox.h`:130
- Brief: Access the StepRegistry for custom step registration.
- Parameters: none
- Return: Reference to the registry owned by workflowEngine().
- Details: Step Registry. Reference to the registry owned by workflowEngine(). Return value. Implements stepRegistry without additional internal calls.

#### `std::shared_ptr< ingestion::ITextGenerationBackend > textBackend() const`
- Source: `include/toolbox/ingestion_toolbox.h`:137
- Brief: Access the currently active text-generation backend.
- Parameters: none
- Return: Always non-null (falls back to NullTextGenerationBackend).
- Details: Always non-null (falls back to NullTextGenerationBackend).

#### `std::shared_ptr< ingestion::WorkflowEngine > workflowEngine() const`
- Source: `include/toolbox/ingestion_toolbox.h`:123
- Brief: Access the WorkflowEngine for profile loading and execution.
- Parameters: none
- Return: Always non-null.
- Details: Always non-null.

#### `~IngestionToolbox()`
- Source: `include/toolbox/ingestion_toolbox.h`:78
- Brief: n/a
- Parameters: none

### themis::toolbox::IngestionToolbox::Impl

#### `Impl()`
- Source: `src/toolbox/ingestion_toolbox.cpp`:31
- Brief: n/a
- Parameters: none

### themis::toolbox::TextChunker

#### `TextChunker()`
- Source: `include/toolbox/text_chunker.h`:37
- Brief: n/a
- Parameters: none
- Details: Construct with default DocumentSplitterConfig (chunk_size=512, overlap=64, strategy=Sentence).

#### `TextChunker(TextChunker &&) noexcept=default`
- Source: `include/toolbox/text_chunker.h`:46
- Brief: n/a
- Parameters:
  - `<unnamed>` (TextChunker &&): n/a

#### `TextChunker(const TextChunker &)=delete`
- Source: `include/toolbox/text_chunker.h`:44
- Brief: n/a
- Parameters:
  - `<unnamed>` (const TextChunker &): n/a

#### `TextChunker(const rag::DocumentSplitterConfig &config)`
- Source: `include/toolbox/text_chunker.h`:40
- Brief: Construct with a custom configuration.
- Parameters:
  - `config` (const rag::DocumentSplitterConfig &): n/a

#### `std::vector< rag::DocumentChunk > chunk(const std::string &text, const std::string &document_id="") const`
- Source: `include/toolbox/text_chunker.h`:69
- Brief: Split text into overlapping chunks.
- Parameters:
  - `text` (const std::string &): UTF-8 source text.
  - `document_id` (const std::string &): Optional document identifier embedded in each chunk.
- Return: Ordered vector of DocumentChunk objects (empty when text is empty or only whitespace).
- Details: text UTF-8 source text. document_id Optional document identifier embedded in each chunk. Ordered vector of DocumentChunk objects (empty when text is empty or only whitespace).

#### `std::vector< std::string > chunkTexts(const std::string &text, const std::string &document_id="") const`
- Source: `include/toolbox/text_chunker.h`:83
- Brief: Split text and return only the raw text strings.
- Parameters:
  - `text` (const std::string &): UTF-8 source text.
  - `document_id` (const std::string &): Optional document identifier.
- Return: Ordered vector of chunk text strings.
- Details: Convenience variant for callers that only need the text and not the full DocumentChunk metadata. text UTF-8 source text. document_id Optional document identifier. Ordered vector of chunk text strings.

#### `std::size_t estimateTokens(const std::string &text) const`
- Source: `include/toolbox/text_chunker.h`:95
- Brief: Estimate the token count for text using the current config.
- Parameters:
  - `text` (const std::string &): Input text.
- Return: Estimated token count.
- Details: Uses the same chars_per_token factor as the underlying splitter. text Input text. Estimated token count.

#### `const rag::DocumentSplitterConfig & getConfig() const`
- Source: `include/toolbox/text_chunker.h`:52
- Brief: Return the active splitting configuration.
- Parameters: none

#### `TextChunker & operator=(TextChunker &&) noexcept=default`
- Source: `include/toolbox/text_chunker.h`:47
- Brief: n/a
- Parameters:
  - `<unnamed>` (TextChunker &&): n/a

#### `TextChunker & operator=(const TextChunker &)=delete`
- Source: `include/toolbox/text_chunker.h`:45
- Brief: n/a
- Parameters:
  - `<unnamed>` (const TextChunker &): n/a

#### `void setConfig(const rag::DocumentSplitterConfig &config)`
- Source: `include/toolbox/text_chunker.h`:57
- Brief: Set Config.
- Parameters:
  - `config` (const rag::DocumentSplitterConfig &): Input parameter.
- Throws:
  - std::invalid_argument: on invalid parameters (overlap >= chunk_size or chunk_size == 0).
- Details: Replace the active configuration. std::invalid_argument on invalid parameters (overlap >= chunk_size or chunk_size == 0). config Input parameter. Implements setConfig without additional internal calls.

#### `~TextChunker()`
- Source: `include/toolbox/text_chunker.h`:42
- Brief: n/a
- Parameters: none

### themis::toolbox::TextNormalizer

#### `TextNormalizer()=default`
- Source: `include/toolbox/text_normalizer.h`:35
- Brief: n/a
- Parameters: none

#### `TextNormalizer(const TextNormalizer &)=default`
- Source: `include/toolbox/text_normalizer.h`:38
- Brief: n/a
- Parameters:
  - `<unnamed>` (const TextNormalizer &): n/a

#### `std::string normalize(std::string_view text) const`
- Source: `include/toolbox/text_normalizer.h`:47
- Brief: Normalise German umlauts and ß in text.
- Parameters:
  - `text` (std::string_view): UTF-8 input text.
- Return: Normalised UTF-8 string.
- Details: text UTF-8 input text. Normalised UTF-8 string.

#### `TextNormalizer & operator=(const TextNormalizer &)=default`
- Source: `include/toolbox/text_normalizer.h`:39
- Brief: n/a
- Parameters:
  - `<unnamed>` (const TextNormalizer &): n/a

#### `~TextNormalizer()=default`
- Source: `include/toolbox/text_normalizer.h`:36
- Brief: n/a
- Parameters: none

### themis::toolbox::TextQualityScorer

#### `TextQualityScorer()=default`
- Source: `include/toolbox/text_quality_scorer.h`:70
- Brief: n/a
- Parameters: none

#### `TextQualityScorer(const TextQualityScorer &)=default`
- Source: `include/toolbox/text_quality_scorer.h`:73
- Brief: n/a
- Parameters:
  - `<unnamed>` (const TextQualityScorer &): n/a

#### `TextQualityScorer & operator=(const TextQualityScorer &)=default`
- Source: `include/toolbox/text_quality_scorer.h`:74
- Brief: n/a
- Parameters:
  - `<unnamed>` (const TextQualityScorer &): n/a

#### `TextQualityScore score(std::string_view text) const`
- Source: `include/toolbox/text_quality_scorer.h`:82
- Brief: Compute a quality score for text.
- Parameters:
  - `text` (std::string_view): UTF-8 text to evaluate.
- Return: Populated TextQualityScore.
- Details: text UTF-8 text to evaluate. Populated TextQualityScore.

#### `~TextQualityScorer()=default`
- Source: `include/toolbox/text_quality_scorer.h`:71
- Brief: n/a
- Parameters: none

### themis::toolbox::ToolboxBuilder

#### `ToolboxBuilder()`
- Source: `include/toolbox/toolbox_builder.h`:111
- Brief: n/a
- Parameters: none

#### `ToolboxBuilder(ToolboxBuilder &&) noexcept`
- Source: `include/toolbox/toolbox_builder.h`:117
- Brief: n/a
- Parameters:
  - `<unnamed>` (ToolboxBuilder &&): n/a

#### `ToolboxBuilder(const ToolboxBuilder &)=delete`
- Source: `include/toolbox/toolbox_builder.h`:115
- Brief: n/a
- Parameters:
  - `<unnamed>` (const ToolboxBuilder &): n/a

#### `std::shared_ptr< IngestionToolbox > build()`
- Source: `include/toolbox/toolbox_builder.h`:247
- Brief: Construct and return a fully configured IngestionToolbox.
- Parameters: none
- Return: Fully configured shared_ptr<IngestionToolbox>.
- Throws:
  - std::logic_error: if build() has already been called on this builder instance.
  - std::logic_error: if an error occurs.
  - std::invalid_argument: if an error occurs.
- Details: Build. Steps performed in order: Create or reuse a WorkflowEngine (from withWorkflowEngine() or via IngestionToolbox::createDefault()). Load each workflow profile added via withWorkflowProfile(). Inject the text-generation backend if one was provided. Return the configured toolbox instance. Profile load failures are logged at warn level; all loadable profiles are applied and the toolbox is returned even when some profiles fail. Fully configured shared_ptr<IngestionToolbox>. std::logic_error if build() has already been called on this builder instance. Return value. std::logic_error if an error occurs. std::invalid_argument if an error occurs. Calls: empty(), IngestionToolbox::createDefault(), setWorkflowEngine(), stepRegistry(), supportedMimeTypes(), front(), ingestion::builtin::createParsePdfStep(), registerStep().

#### `BuiltToolbox buildWithBridges()`
- Source: `include/toolbox/toolbox_builder.h`:264
- Brief: Construct the toolbox and auto-wire AQLIngestionBridge and RAGIngestionBridge from the registered sinks.
- Parameters: none
- Return: BuiltToolbox with all applicable fields populated.
- Throws:
  - std::logic_error: if build() or buildWithBridges() has already been called on this builder instance.
  - std::logic_error: if an error occurs.
- Details: ── buildWithBridges() ──────────────────────────────────────────────────────── Performs the same steps as build(), then creates: AQLIngestionBridge(toolbox, graph_writer) when a graph-writer has been set via withGraphWriter(). RAGIngestionBridge(toolbox, vector_writer, graph_writer) when a vector-writer or graph-writer has been set. BuiltToolbox with all applicable fields populated. std::logic_error if build() or buildWithBridges() has already been called on this builder instance. Return value. std::logic_error if an error occurs. Calls: build(), THEMIS_WARN(), what().

#### `std::shared_ptr< ingestion::IGraphWriter > graphWriter() const`
- Source: `include/toolbox/toolbox_builder.h`:272
- Brief: Return the graph-writer that was set via withGraphWriter(). May return null if no writer was configured.
- Parameters: none

#### `ToolboxBuilder & operator=(ToolboxBuilder &&) noexcept`
- Source: `include/toolbox/toolbox_builder.h`:118
- Brief: n/a
- Parameters:
  - `<unnamed>` (ToolboxBuilder &&): n/a

#### `ToolboxBuilder & operator=(const ToolboxBuilder &)=delete`
- Source: `include/toolbox/toolbox_builder.h`:116
- Brief: n/a
- Parameters:
  - `<unnamed>` (const ToolboxBuilder &): n/a

#### `std::size_t profileCount() const noexcept`
- Source: `include/toolbox/toolbox_builder.h`:283
- Brief: Return the number of profile paths registered.
- Parameters: none

#### `std::shared_ptr< ingestion::IVectorWriter > vectorWriter() const`
- Source: `include/toolbox/toolbox_builder.h`:278
- Brief: Return the vector-writer that was set via withVectorWriter(). May return null if no writer was configured.
- Parameters: none

#### `ToolboxBuilder & withFormatExtractor(std::shared_ptr< ingestion::IFormatExtractor > extractor)`
- Source: `include/toolbox/toolbox_builder.h`:211
- Brief: Register a format extractor and wire it into the corresponding builtin parse step.
- Parameters:
  - `extractor` (std::shared_ptr< ingestion::IFormatExtractor >): Input parameter.
- Return: *this for chaining.
- Throws:
  - std::invalid_argument: if extractor is null.
- Details: With Format Extractor. Calling this multiple times with different extractors registers all of them. For each extractor, the matching builtin step is created and registered in the StepRegistry during build(): IFormatExtractor supporting application/pdf → builtin.parse_pdf Supporting application/vnd.openxmlformats-* → builtin.parse_office Supporting image/\* → builtin.parse_image Supporting application/zip / application/x-tar → builtin.parse_archive Supporting audio/\* → builtin.parse_audio When a FormatExtractorFactory (from content/adapters/) is available, use withFormatExtractorFactory() to register all extractors at once. extractor Format extractor to register. Must not be null. *this for chaining. std::invalid_argument if extractor is null. extractor Input parameter. Return value.

#### `ToolboxBuilder & withFormatExtractorFactory(std::shared_ptr< ingestion::IFormatExtractorFactory > factory)`
- Source: `include/toolbox/toolbox_builder.h`:225
- Brief: Register all format extractors from a factory at once.
- Parameters:
  - `factory` (std::shared_ptr< ingestion::IFormatExtractorFactory >): Input parameter.
- Return: *this for chaining.
- Details: With Format Extractor Factory. Iterates over factory->registeredMimeTypes() and calls withFormatExtractor() for each distinct extractor. This is the preferred way to wire a content::adapters::FormatExtractorFactory into the toolbox. factory Pre-populated factory. Must not be null. *this for chaining. factory Input parameter. Return value.

#### `ToolboxBuilder & withGraphWriter(std::shared_ptr< ingestion::IGraphWriter > writer)`
- Source: `include/toolbox/toolbox_builder.h`:146
- Brief: Attach a graph-writer sink.
- Parameters:
  - `writer` (std::shared_ptr< ingestion::IGraphWriter >): Input parameter.
- Return: *this for chaining.
- Details: With Graph Writer. When set, the AQLIngestionBridge and RAGIngestionBridge that wrap this toolbox will write extracted entity nodes and relation edges to the provided sink. writer Graph-store sink. May be null to clear a previously set writer. *this for chaining. writer Input parameter. Return value.

#### `ToolboxBuilder & withTensorCoreSink(std::shared_ptr< ingestion::ITensorCoreBridge > sink)`
- Source: `include/toolbox/toolbox_builder.h`:308
- Brief: Inject a real ITensorCoreBridge for durable TT-core storage.
- Parameters:
  - `sink` (std::shared_ptr< ingestion::ITensorCoreBridge >): Input parameter.
- Return: *this for chaining.
- Details: With Tensor Core Sink. Re-registers the builtin.tensor_core_bridge step with the supplied sink, replacing the default no-op fallback. Pass nullptr to restore the no-op fallback. sink A TensorCoreStorageBridge (or any custom implementation). *this for chaining. sink Input parameter. Return value.

#### `ToolboxBuilder & withTensorDecompositionBackend(std::shared_ptr< ingestion::ITensorDecompositionBackend > backend)`
- Source: `include/toolbox/toolbox_builder.h`:295
- Brief: Inject a real ITensorDecompositionBackend for TT-core production.
- Parameters:
  - `backend` (std::shared_ptr< ingestion::ITensorDecompositionBackend >): Input parameter.
- Return: *this for chaining.
- Details: With Tensor Decomposition Backend. Re-registers the builtin.chunk_tt_decompose step with the supplied backend, replacing the default NullTensorDecompositionBackend. Pass nullptr to restore the no-op fallback. backend A TensorIngestionBridge (or any custom implementation). *this for chaining. backend Input parameter. Return value.

#### `ToolboxBuilder & withTextBackend(std::shared_ptr< ingestion::ITextGenerationBackend > backend)`
- Source: `include/toolbox/toolbox_builder.h`:173
- Brief: Inject a text-generation backend.
- Parameters:
  - `backend` (std::shared_ptr< ingestion::ITextGenerationBackend >): Input parameter.
- Return: *this for chaining.
- Details: With Text Backend. Replaces the default NullTextGenerationBackend. The backend is forwarded to IngestionToolbox::setTextBackend() which propagates it to all registered NER / LLM steps. backend LLM text-generation implementation. Passing nullptr reinstates the null backend. *this for chaining. backend Input parameter. Return value.

#### `ToolboxBuilder & withVectorWriter(std::shared_ptr< ingestion::IVectorWriter > writer)`
- Source: `include/toolbox/toolbox_builder.h`:159
- Brief: Attach a vector-writer sink.
- Parameters:
  - `writer` (std::shared_ptr< ingestion::IVectorWriter >): Input parameter.
- Return: *this for chaining.
- Details: With Vector Writer. When set and buildWithBridges() is used, the resulting RAGIngestionBridge will write embedding chunks to this sink. writer Vector-store sink. May be null to clear a previously set writer. *this for chaining. writer Input parameter. Return value.

#### `ToolboxBuilder & withWorkflowEngine(std::shared_ptr< ingestion::WorkflowEngine > engine)`
- Source: `include/toolbox/toolbox_builder.h`:187
- Brief: Use a pre-constructed WorkflowEngine instead of the default.
- Parameters:
  - `engine` (std::shared_ptr< ingestion::WorkflowEngine >): Input parameter.
- Return: *this for chaining.
- Throws:
  - std::invalid_argument: if engine is null.
- Details: With Workflow Engine. Useful for test isolation or for engines pre-loaded with custom step implementations. Profile paths added via withWorkflowProfile() are still loaded on top of the injected engine. engine Custom engine. Must not be null. *this for chaining. std::invalid_argument if engine is null. engine Input parameter. Return value.

#### `ToolboxBuilder & withWorkflowProfile(std::string profile_path)`
- Source: `include/toolbox/toolbox_builder.h`:133
- Brief: Add a workflow profile file to load during build().
- Parameters:
  - `profile_path` (std::string): Path to the profile.
- Return: *this for chaining.
- Throws:
  - std::invalid_argument: if profile_path is empty.
  - std::invalid_argument: if an error occurs.
- Details: With Workflow Profile. Multiple profiles can be added; they are loaded in registration order. Each profile is loaded via WorkflowEngine::loadProfile(path). profile_path Absolute or relative path to a YAML workflow profile file. *this for chaining. std::invalid_argument if profile_path is empty. profile_path Path to the profile. Return value. std::invalid_argument if an error occurs. Calls: empty(), push_back(), std::move().

#### `~ToolboxBuilder()`
- Source: `include/toolbox/toolbox_builder.h`:112
- Brief: n/a
- Parameters: none

### themis::toolbox::ToolboxBuilder::BuiltToolbox

#### `BuiltToolbox()`
- Source: `include/toolbox/toolbox_builder.h`:101
- Brief: n/a
- Parameters: none

#### `BuiltToolbox(BuiltToolbox &&) noexcept`
- Source: `include/toolbox/toolbox_builder.h`:103
- Brief: n/a
- Parameters:
  - `<unnamed>` (BuiltToolbox &&): n/a

#### `BuiltToolbox(const BuiltToolbox &)=default`
- Source: `include/toolbox/toolbox_builder.h`:105
- Brief: n/a
- Parameters:
  - `<unnamed>` (const BuiltToolbox &): n/a

#### `BuiltToolbox & operator=(BuiltToolbox &&) noexcept`
- Source: `include/toolbox/toolbox_builder.h`:104
- Brief: n/a
- Parameters:
  - `<unnamed>` (BuiltToolbox &&): n/a

#### `BuiltToolbox & operator=(const BuiltToolbox &)=default`
- Source: `include/toolbox/toolbox_builder.h`:106
- Brief: n/a
- Parameters:
  - `<unnamed>` (const BuiltToolbox &): n/a

#### `~BuiltToolbox()`
- Source: `include/toolbox/toolbox_builder.h`:102
- Brief: n/a
- Parameters: none

### themis::toolbox::ToolboxComposite

#### `ToolboxComposite(ToolboxComposite &&) noexcept=default`
- Source: `include/toolbox/toolbox_composite.h`:50
- Brief: n/a
- Parameters:
  - `<unnamed>` (ToolboxComposite &&): n/a

#### `ToolboxComposite(const ToolboxComposite &)=delete`
- Source: `include/toolbox/toolbox_composite.h`:48
- Brief: n/a
- Parameters:
  - `<unnamed>` (const ToolboxComposite &): n/a

#### `ToolboxComposite(std::vector< Route > routes, std::shared_ptr< IngestionToolbox > fallback=nullptr)`
- Source: `include/toolbox/toolbox_composite.h`:42
- Brief: Construct with a routing table and optional fallback.
- Parameters:
  - `routes` (std::vector< Route >): Ordered list of (mime_prefix, toolbox) pairs.
  - `fallback` (std::shared_ptr< IngestionToolbox >): Toolbox to use when no route matches. May be null.
- Details: routes Ordered list of (mime_prefix, toolbox) pairs. fallback Toolbox to use when no route matches. May be null.

#### `std::vector< ingestion::BaseEntity > extractEntities(const std::string &text, const std::string &mime="text/plain", const std::string &filename="input.txt")`
- Source: `include/toolbox/toolbox_composite.h`:67
- Brief: Extract entities, dispatching to the matching toolbox.
- Parameters:
  - `text` (const std::string &): Input parameter.
  - `mime` (const std::string &): Input parameter.
  - `filename` (const std::string &): Input parameter.
- Return: Extracted entities, or an empty vector when no toolbox is available for the given MIME type.
- Details: Extract Entities. Finds the first route whose prefix matches the start of mime and delegates to its IngestionToolbox::extractEntities(). Falls back to the fallback toolbox if no route matches. text UTF-8 text to process. mime MIME type used for routing (e.g. "text/plain", "application/pdf"). filename Filename hint forwarded to the selected toolbox. Extracted entities, or an empty vector when no toolbox is available for the given MIME type. text Input parameter. mime Input parameter. filename Input parameter. Return value.

#### `std::shared_ptr< IngestionToolbox > fallback() const noexcept`
- Source: `include/toolbox/toolbox_composite.h`:80
- Brief: Return the fallback toolbox (may be null).
- Parameters: none

#### `ToolboxComposite & operator=(ToolboxComposite &&) noexcept=default`
- Source: `include/toolbox/toolbox_composite.h`:51
- Brief: n/a
- Parameters:
  - `<unnamed>` (ToolboxComposite &&): n/a

#### `ToolboxComposite & operator=(const ToolboxComposite &)=delete`
- Source: `include/toolbox/toolbox_composite.h`:49
- Brief: n/a
- Parameters:
  - `<unnamed>` (const ToolboxComposite &): n/a

#### `std::shared_ptr< IngestionToolbox > resolve(const std::string &mime) const`
- Source: `include/toolbox/toolbox_composite.h`:87
- Brief: Look up the toolbox that would be used for mime.
- Parameters:
  - `mime` (const std::string &): n/a
- Details: Returns nullptr when no route and no fallback match.

#### `const std::vector< Route > & routes() const noexcept`
- Source: `include/toolbox/toolbox_composite.h`:75
- Brief: Return the registered routes (for inspection / testing).
- Parameters: none

#### `~ToolboxComposite()`
- Source: `include/toolbox/toolbox_composite.h`:46
- Brief: n/a
- Parameters: none

### themis::toolbox::ToolboxCompositeBuilder

#### `ToolboxCompositeBuilder()=default`
- Source: `include/toolbox/toolbox_composite.h`:114
- Brief: n/a
- Parameters: none

#### `ToolboxCompositeBuilder & addRoute(std::string mime_prefix, std::shared_ptr< IngestionToolbox > toolbox)`
- Source: `include/toolbox/toolbox_composite.h`:125
- Brief: Register a MIME-prefix → toolbox route.
- Parameters:
  - `mime_prefix` (std::string): Input parameter.
  - `toolbox` (std::shared_ptr< IngestionToolbox >): Input parameter.
- Return: Return value.
- Details: Add Route. mime_prefix Prefix to match against the mime argument of extractEntities(). E.g. "text/" matches any "text/plain", "text/html", … toolbox Toolbox to use when the prefix matches. Must not be null. mime_prefix Input parameter. toolbox Input parameter. Return value.

#### `std::unique_ptr< ToolboxComposite > build()`
- Source: `include/toolbox/toolbox_composite.h`:144
- Brief: Build the ToolboxComposite.
- Parameters: none
- Return: Return value.
- Throws:
  - std::logic_error: when no routes and no fallback have been added.
  - std::logic_error: if an error occurs.
- Details: Build. std::logic_error when no routes and no fallback have been added. Return value. std::logic_error if an error occurs. Calls: empty(), std::move().

#### `ToolboxCompositeBuilder & setFallback(std::shared_ptr< IngestionToolbox > toolbox)`
- Source: `include/toolbox/toolbox_composite.h`:134
- Brief: Set the fallback toolbox used when no route matches.
- Parameters:
  - `toolbox` (std::shared_ptr< IngestionToolbox >): Input parameter.
- Return: Return value.
- Details: Set Fallback. toolbox May be null (disables the fallback). toolbox Input parameter. Return value.

### themis::toolbox::ToolboxRegistry

#### `ToolboxRegistry()=delete`
- Source: `include/toolbox/toolbox_registry.h`:98
- Brief: n/a
- Parameters: none

#### `void initialize(std::shared_ptr< IngestionToolbox > toolbox)`
- Source: `include/toolbox/toolbox_registry.h`:111
- Brief: Register a process-global IngestionToolbox instance.
- Parameters:
  - `toolbox` (std::shared_ptr< IngestionToolbox >): Input parameter.
- Throws:
  - std::invalid_argument: when toolbox is null.
  - std::invalid_argument: if an error occurs.
- Details: Initialize. Must be called exactly once before any instance() call. Calling initialize() a second time replaces the existing instance (last-write wins); this is intentional to support live reconfiguration of the default toolbox (e.g. hot-reload of the LLM backend). toolbox Pre-configured toolbox. Must not be null. std::invalid_argument when toolbox is null. toolbox Input parameter. std::invalid_argument if an error occurs. Calls: lk(), fetch_add(), THEMIS_WARN(), std::move().

#### `std::shared_ptr< IngestionToolbox > instance()`
- Source: `include/toolbox/toolbox_registry.h`:119
- Brief: Access the registered global IngestionToolbox.
- Parameters: none
- Return: Shared pointer to the active toolbox.
- Throws:
  - std::logic_error: when initialize() has not been called.
  - std::logic_error: if an error occurs.
- Details: Instance. Shared pointer to the active toolbox. std::logic_error when initialize() has not been called. Return value. std::logic_error if an error occurs. Calls: lk(), fetch_add().

#### `bool isInitialized() noexcept`
- Source: `include/toolbox/toolbox_registry.h`:127
- Brief: Return true when a toolbox has been registered.
- Parameters: none
- Details: Use this guard in server health-check paths that must verify that the toolbox has been fully configured before serving requests.

#### `void reset() noexcept`
- Source: `include/toolbox/toolbox_registry.h`:140
- Brief: Clear the registered instance.
- Parameters: none
- Details: Intended for test isolation only. After reset() any call to instance() will throw std::logic_error until initialize() is called again. Not safe to call while any thread is executing a toolbox operation. Always call from a single-threaded context (e.g. TearDownTestSuite()).

