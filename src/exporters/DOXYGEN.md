# EXPORTERS DOXYGEN

Author: ThemisDB Contributors
Created: 2026-09-20
Last Updated: 2026-09-20
Status: active

## Source
- This file is generated from module-scoped Doxygen XML.
- XML index: `C:\Projects\ThemisDB\.tmp\module_doxygen_global_e2e\exporters\xml\index.xml`
- Warnings log: `C:\Projects\ThemisDB\.tmp\module_doxygen_global_e2e\exporters\doxygen-warnings.log`

## Structure Metrics
- C/C++ Files (scanned): 56
- Compounds: 185
- Classes/Structs: 102
- Namespaces: 18
- File Compounds: 56

## Namespaces
- @050043157206023102366112134164351370243341106053
- @051213233036056163110063240027373377305006001367
- @124266240310247024241302040236063006161211122275
- @167040224276077355306247316263256007375156010022
- @257322234175060076217105035003271132111115346246
- std
- std::chrono_literals
- testing
- themis
- themis::bench
- themis::bench::errg
- themis::exporters
- themis::exporters::@072023075053163247047247351056072113366367374204
- themis::exporters::@142331357073316121373071010340330337111213300104
- themis::exporters::@203161172360341225225173376016057171042127052110
- themis::governance
- themis::query
- themis::utils

## Types
### Classes
- AqlPredicateFilterIntegrationTest
- AqlPredicateFilterTest
- ArrowIPCExporterTest
- DataAugmentationTest
- ExportEncryptionTest
- ExportFormatRegistryTest
- ExportPolicyEnforcementTest
- FormatTemplateExporterTest
- FormatTemplateTest
- HuggingFaceExporterTest
- IncrementalExporterTest
- JSONLLLMExporterTest
- JoinExporterTest
- ParquetExporterTest
- StreamingExporterTest
- StubExportQueue
- StubExportQueueBench
- StubMetricExporterBench
- StubMetricExporterStress
- StubTraceExporterBench
- StubTraceExporterStress
- TestTokenKeyProvider
- ValidateTemplateTest
- themis::exporters::AlpacaTemplate
- themis::exporters::AqlPredicateFilter
- themis::exporters::AqlPredicateFilterException
- themis::exporters::ArrowIPCExporter
- themis::exporters::ChatMLTemplate
- themis::exporters::ConfigException
- themis::exporters::DataAugmentationPipeline
- themis::exporters::ExportCursor
- themis::exporters::ExportEncryption
- themis::exporters::ExportEncryptor
- themis::exporters::ExportFormatRegistry
- themis::exporters::ExportIOException
- themis::exporters::ExporterException
- themis::exporters::ExporterMetrics
- themis::exporters::FormatException
- themis::exporters::HuggingFaceExporter
- themis::exporters::HuggingFaceHubClient
- themis::exporters::IExporter
- themis::exporters::IFormatTemplate
- themis::exporters::IncrementalExporter
- themis::exporters::JSONLLLMExporter
- themis::exporters::JoinExporter
- themis::exporters::OpenAIFineTuningTemplate
- themis::exporters::PIIDetector
- themis::exporters::ParquetExporter
- themis::exporters::PolicyDeniedException
- themis::exporters::QualityFilterException
- themis::exporters::SchemaValidationException
- themis::exporters::ShareGPTTemplate
- themis::exporters::SizeLimitException
- themis::exporters::StreamWriter
- themis::exporters::StreamingExporter
- themis::exporters::VectorExportCursor

### Structs
- HfTokenGuard
- themis::bench::errg::ExportQuota
- themis::bench::errg::IncomingRow
- themis::bench::errg::ParquetCell
- themis::bench::errg::ParquetRowGroup
- themis::bench::errg::SchemaCol
- themis::exporters::ArrowIPCExportConfig
- themis::exporters::AugmentationConfig
- themis::exporters::AugmentationStats
- themis::exporters::AugmentationStrategyConfig
- themis::exporters::ExportEncryptionConfig
- themis::exporters::ExportOptions
- themis::exporters::ExportStats
- themis::exporters::ExportTenantContext
- themis::exporters::ExporterMetrics::LatencyHistogram
- themis::exporters::ExporterMetrics::SchemaValidationStats
- themis::exporters::FormatTemplateFieldMapping
- themis::exporters::HubUploadConfig
- themis::exporters::HubUploadResult
- themis::exporters::HuggingFaceExporterConfig
- themis::exporters::HuggingFaceFeature
- themis::exporters::IncrementalExportConfig
- themis::exporters::JSONLFormat
- themis::exporters::JSONLLLMConfig
- themis::exporters::JSONLLLMConfig::AdapterMetadata
- themis::exporters::JSONLLLMConfig::AdapterMetadata::TrainingConfig
- themis::exporters::JSONLLLMConfig::AdapterMetadata::VLLMConfig
- themis::exporters::JSONLLLMConfig::FieldMapping
- themis::exporters::JSONLLLMConfig::PIIConfig
- themis::exporters::JSONLLLMConfig::QualityFilter
- themis::exporters::JSONLLLMConfig::QualityMetrics
- themis::exporters::JSONLLLMConfig::StructuredGeneration
- themis::exporters::JSONLLLMConfig::WeightConfig
- themis::exporters::JSONLLLMExporter::RuntimeMetrics
- themis::exporters::JoinExportConfig
- themis::exporters::JoinExportConfig::PIIConfig
- themis::exporters::MemoryShardSpec
- themis::exporters::PIIDetector::Config
- themis::exporters::PIIDetector::PIIMatch
- themis::exporters::PIIMetrics
- themis::exporters::ParquetColumnHint
- themis::exporters::ParquetExportConfig
- themis::exporters::ParquetExportConfig::PIIConfig
- themis::exporters::StreamWriter::Config
- themis::exporters::StreamingExportConfig
- themis::exporters::TemplateValidationResult

## Notes
- This is a derived artifact. Do not manually edit semantic content.
- Regenerate via scripts/generate_module_doxygen_xml.py --write-module-markdown.

## Detailed Function Documentation (Soll-Ist)
- Functions extracted: 856

### AqlPredicateFilterIntegrationTest

#### `void SetUp() override`
- Source: `tests/exporters/test_aql_predicate_filter.cpp`:158
- Brief: n/a
- Parameters: none

#### `void TearDown() override`
- Source: `tests/exporters/test_aql_predicate_filter.cpp`:174
- Brief: n/a
- Parameters: none

#### `std::vector< std::string > readLines(const std::string &path)`
- Source: `tests/exporters/test_aql_predicate_filter.cpp`:180
- Brief: n/a
- Parameters:
  - `path` (const std::string &): n/a

### AqlPredicateFilterTest

#### `void SetUp() override`
- Source: `tests/exporters/test_aql_predicate_filter.cpp`:15
- Brief: n/a
- Parameters: none

#### `void TearDown() override`
- Source: `tests/exporters/test_aql_predicate_filter.cpp`:48
- Brief: n/a
- Parameters: none

#### `std::vector< std::string > readLines(const std::string &path)`
- Source: `tests/exporters/test_aql_predicate_filter.cpp`:54
- Brief: n/a
- Parameters:
  - `path` (const std::string &): n/a

### ArrowIPCExporterTest

#### `void SetUp() override`
- Source: `tests/exporters/test_arrow_ipc_exporter.cpp`:81
- Brief: n/a
- Parameters: none

#### `void TearDown() override`
- Source: `tests/exporters/test_arrow_ipc_exporter.cpp`:89
- Brief: n/a
- Parameters: none

#### `void createTestEntities(int count=10)`
- Source: `tests/exporters/test_arrow_ipc_exporter.cpp`:95
- Brief: n/a
- Parameters:
  - `count` (int): n/a

### DataAugmentationTest

#### `std::vector< BaseEntity > sampleEntities() const`
- Source: `tests/exporters/test_data_augmentation.cpp`:38
- Brief: n/a
- Parameters: none

### ExportEncryptionTest

#### `void SetUp() override`
- Source: `tests/exporters/test_export_encryption.cpp`:52
- Brief: n/a
- Parameters: none

#### `void TearDown() override`
- Source: `tests/exporters/test_export_encryption.cpp`:65
- Brief: n/a
- Parameters: none

#### `ExportEncryptionConfig makeConfig(const std::string &kek_id="test_kek", std::shared_ptr< KeyProvider > kp=nullptr, const std::string &job_id="job-001") const`
- Source: `tests/exporters/test_export_encryption.cpp`:110
- Brief: n/a
- Parameters:
  - `kek_id` (const std::string &): n/a
  - `kp` (std::shared_ptr< KeyProvider >): n/a
  - `job_id` (const std::string &): n/a

#### `std::vector< BaseEntity > makeEntities(int count=5)`
- Source: `tests/exporters/test_export_encryption.cpp`:92
- Brief: n/a
- Parameters:
  - `count` (int): n/a

#### `std::string readFile(const std::string &path)`
- Source: `tests/exporters/test_export_encryption.cpp`:81
- Brief: n/a
- Parameters:
  - `path` (const std::string &): n/a

#### `std::string writePlainFile(const std::string &filename, const std::string &content)`
- Source: `tests/exporters/test_export_encryption.cpp`:72
- Brief: n/a
- Parameters:
  - `filename` (const std::string &): n/a
  - `content` (const std::string &): n/a

### ExportFormatRegistryTest

#### `void SetUp() override`
- Source: `tests/exporters/test_export_format_registry.cpp`:19
- Brief: n/a
- Parameters: none

#### `void TearDown() override`
- Source: `tests/exporters/test_export_format_registry.cpp`:23
- Brief: n/a
- Parameters: none

### ExportPolicyEnforcementTest

#### `void SetUp() override`
- Source: `tests/exporters/test_export_encryption.cpp`:991
- Brief: n/a
- Parameters: none

#### `void TearDown() override`
- Source: `tests/exporters/test_export_encryption.cpp`:1000
- Brief: n/a
- Parameters: none

#### `std::vector< BaseEntity > makeEntities(int n=3)`
- Source: `tests/exporters/test_export_encryption.cpp`:1005
- Brief: n/a
- Parameters:
  - `n` (int): n/a

### FormatTemplateExporterTest

#### `void SetUp() override`
- Source: `tests/exporters/test_format_template.cpp`:325
- Brief: n/a
- Parameters: none

#### `void TearDown() override`
- Source: `tests/exporters/test_format_template.cpp`:334
- Brief: n/a
- Parameters: none

#### `std::vector< BaseEntity > makeSampleEntities(int count=5) const`
- Source: `tests/exporters/test_format_template.cpp`:340
- Brief: n/a
- Parameters:
  - `count` (int): n/a

#### `std::vector< std::string > readLines(const std::string &path) const`
- Source: `tests/exporters/test_format_template.cpp`:357
- Brief: n/a
- Parameters:
  - `path` (const std::string &): n/a

### FormatTemplateTest

#### `FormatTemplateFieldMapping defaultMapping() const`
- Source: `tests/exporters/test_format_template.cpp`:53
- Brief: n/a
- Parameters: none

### HfTokenGuard

#### `HfTokenGuard(const char *value)`
- Source: `tests/exporters/test_huggingface_hub_client.cpp`:165
- Brief: n/a
- Parameters:
  - `value` (const char *): n/a

#### `std::string getEnvValue(const char *name, bool &found)`
- Source: `tests/exporters/test_huggingface_hub_client.cpp`:139
- Brief: n/a
- Parameters:
  - `name` (const char *): n/a
  - `found` (bool &): n/a

#### `void setEnvValue(const char *name, const char *value)`
- Source: `tests/exporters/test_huggingface_hub_client.cpp`:149
- Brief: n/a
- Parameters:
  - `name` (const char *): n/a
  - `value` (const char *): n/a

#### `void unsetEnvValue(const char *name)`
- Source: `tests/exporters/test_huggingface_hub_client.cpp`:157
- Brief: n/a
- Parameters:
  - `name` (const char *): n/a

#### `~HfTokenGuard()`
- Source: `tests/exporters/test_huggingface_hub_client.cpp`:172
- Brief: n/a
- Parameters: none

### HuggingFaceExporterTest

#### `void SetUp() override`
- Source: `tests/exporters/test_huggingface_exporter.cpp`:18
- Brief: n/a
- Parameters: none

#### `void TearDown() override`
- Source: `tests/exporters/test_huggingface_exporter.cpp`:25
- Brief: n/a
- Parameters: none

#### `void createTestEntities()`
- Source: `tests/exporters/test_huggingface_exporter.cpp`:31
- Brief: n/a
- Parameters: none

#### `std::string readFile(const std::string &path)`
- Source: `tests/exporters/test_huggingface_exporter.cpp`:47
- Brief: n/a
- Parameters:
  - `path` (const std::string &): n/a

#### `std::vector< std::string > readLines(const std::string &path)`
- Source: `tests/exporters/test_huggingface_exporter.cpp`:54
- Brief: n/a
- Parameters:
  - `path` (const std::string &): n/a

### IncrementalExporterTest

#### `void SetUp() override`
- Source: `tests/exporters/test_incremental_exporter.cpp`:40
- Brief: n/a
- Parameters: none

#### `void TearDown() override`
- Source: `tests/exporters/test_incremental_exporter.cpp`:50
- Brief: n/a
- Parameters: none

#### `void createTestEntities(int count)`
- Source: `tests/exporters/test_incremental_exporter.cpp`:57
- Brief: Create entities with _seq field values 1..count.
- Parameters:
  - `count` (int): n/a

#### `std::string outputPath() const`
- Source: `tests/exporters/test_incremental_exporter.cpp`:85
- Brief: n/a
- Parameters: none

#### `std::vector< std::string > readLines(const std::string &path)`
- Source: `tests/exporters/test_incremental_exporter.cpp`:69
- Brief: n/a
- Parameters:
  - `path` (const std::string &): n/a

#### `std::string watermarkPath() const`
- Source: `tests/exporters/test_incremental_exporter.cpp`:81
- Brief: n/a
- Parameters: none

### JSONLLLMExporterTest

#### `void SetUp() override`
- Source: `tests/exporters/test_jsonl_llm_exporter.cpp`:20
- Brief: n/a
- Parameters: none

#### `void TearDown() override`
- Source: `tests/exporters/test_jsonl_llm_exporter.cpp`:30
- Brief: n/a
- Parameters: none

#### `void createTestEntities()`
- Source: `tests/exporters/test_jsonl_llm_exporter.cpp`:37
- Brief: n/a
- Parameters: none

#### `std::vector< std::string > readLinesFromFile(const std::string &path)`
- Source: `tests/exporters/test_jsonl_llm_exporter.cpp`:56
- Brief: n/a
- Parameters:
  - `path` (const std::string &): n/a

### JoinExporterTest

#### `void SetUp() override`
- Source: `tests/exporters/test_join_exporter.cpp`:58
- Brief: n/a
- Parameters: none

#### `void TearDown() override`
- Source: `tests/exporters/test_join_exporter.cpp`:82
- Brief: n/a
- Parameters: none

#### `std::string outPath(const std::string &name) const`
- Source: `tests/exporters/test_join_exporter.cpp`:88
- Brief: n/a
- Parameters:
  - `name` (const std::string &): n/a

### ParquetExporterTest

#### `void SetUp() override`
- Source: `tests/exporters/test_parquet_exporter.cpp`:19
- Brief: n/a
- Parameters: none

#### `void TearDown() override`
- Source: `tests/exporters/test_parquet_exporter.cpp`:28
- Brief: n/a
- Parameters: none

#### `void createTestEntities()`
- Source: `tests/exporters/test_parquet_exporter.cpp`:34
- Brief: n/a
- Parameters: none

#### `bool hasValidParquetFooter(const std::string &path)`
- Source: `tests/exporters/test_parquet_exporter.cpp`:68
- Brief: Read the trailing 4 bytes (before final magic) and verify PAR1 footer.
- Parameters:
  - `path` (const std::string &): n/a

#### `bool isValidParquetMagic(const std::string &path)`
- Source: `tests/exporters/test_parquet_exporter.cpp`:55
- Brief: Read the first 4 bytes of a file and verify it starts with PAR1.
- Parameters:
  - `path` (const std::string &): n/a

### StreamingExporterTest

#### `void SetUp() override`
- Source: `tests/exporters/test_streaming_exporter.cpp`:29
- Brief: n/a
- Parameters: none

#### `void TearDown() override`
- Source: `tests/exporters/test_streaming_exporter.cpp`:39
- Brief: n/a
- Parameters: none

#### `void createTestEntities(int count=20)`
- Source: `tests/exporters/test_streaming_exporter.cpp`:45
- Brief: n/a
- Parameters:
  - `count` (int): n/a

#### `std::vector< std::string > readLines(const std::string &path)`
- Source: `tests/exporters/test_streaming_exporter.cpp`:57
- Brief: n/a
- Parameters:
  - `path` (const std::string &): n/a

### StubExportQueue

#### `StubExportQueue(std::size_t capacity)`
- Source: `tests/exporters/test_exporters_highcardinality_stress.cpp`:61
- Brief: n/a
- Parameters:
  - `capacity` (std::size_t): n/a

#### `bool dequeue(std::string &out)`
- Source: `tests/exporters/test_exporters_highcardinality_stress.cpp`:73
- Brief: n/a
- Parameters:
  - `out` (std::string &): n/a

#### `bool enqueue(const std::string &item)`
- Source: `tests/exporters/test_exporters_highcardinality_stress.cpp`:63
- Brief: n/a
- Parameters:
  - `item` (const std::string &): n/a

#### `uint64_t overflow() const noexcept`
- Source: `tests/exporters/test_exporters_highcardinality_stress.cpp`:81
- Brief: n/a
- Parameters: none

### StubExportQueueBench

#### `StubExportQueueBench(std::size_t cap)`
- Source: `benchmarks/exporters/bench_exporters_dedicated_gates.cpp`:59
- Brief: n/a
- Parameters:
  - `cap` (std::size_t): n/a

#### `bool dequeue(std::string &out)`
- Source: `benchmarks/exporters/bench_exporters_dedicated_gates.cpp`:66
- Brief: n/a
- Parameters:
  - `out` (std::string &): n/a

#### `bool enqueue(const std::string &item)`
- Source: `benchmarks/exporters/bench_exporters_dedicated_gates.cpp`:60
- Brief: n/a
- Parameters:
  - `item` (const std::string &): n/a

### StubMetricExporterBench

#### `uint64_t count() const noexcept`
- Source: `benchmarks/exporters/bench_exporters_dedicated_gates.cpp`:41
- Brief: n/a
- Parameters: none

#### `void exportMetric(const std::string &name, double value)`
- Source: `benchmarks/exporters/bench_exporters_dedicated_gates.cpp`:37
- Brief: n/a
- Parameters:
  - `name` (const std::string &): n/a
  - `value` (double): n/a

### StubMetricExporterStress

#### `void exportMetric(const std::string &name, double value)`
- Source: `tests/exporters/test_exporters_highcardinality_stress.cpp`:39
- Brief: n/a
- Parameters:
  - `name` (const std::string &): n/a
  - `value` (double): n/a

#### `uint64_t exported() const noexcept`
- Source: `tests/exporters/test_exporters_highcardinality_stress.cpp`:43
- Brief: n/a
- Parameters: none

### StubTraceExporterBench

#### `uint64_t count() const noexcept`
- Source: `benchmarks/exporters/bench_exporters_dedicated_gates.cpp`:52
- Brief: n/a
- Parameters: none

#### `void exportTrace(const std::string &id, const std::string &span)`
- Source: `benchmarks/exporters/bench_exporters_dedicated_gates.cpp`:48
- Brief: n/a
- Parameters:
  - `id` (const std::string &): n/a
  - `span` (const std::string &): n/a

### StubTraceExporterStress

#### `void exportTrace(const std::string &trace_id, const std::string &span)`
- Source: `tests/exporters/test_exporters_highcardinality_stress.cpp`:50
- Brief: n/a
- Parameters:
  - `trace_id` (const std::string &): n/a
  - `span` (const std::string &): n/a

#### `uint64_t exported() const noexcept`
- Source: `tests/exporters/test_exporters_highcardinality_stress.cpp`:54
- Brief: n/a
- Parameters: none

### TestTokenKeyProvider

#### `uint32_t createKeyFromBytes(const std::string &key_id, const std::vector< uint8_t > &key_bytes, const themis::KeyMetadata &={}) override`
- Source: `tests/exporters/test_huggingface_hub_client.cpp`:219
- Brief: n/a
- Parameters:
  - `key_id` (const std::string &): n/a
  - `key_bytes` (const std::vector< uint8_t > &): n/a
  - `<unnamed>` (const themis::KeyMetadata &): n/a

#### `void deleteKey(const std::string &, uint32_t) override`
- Source: `tests/exporters/test_huggingface_hub_client.cpp`:213
- Brief: n/a
- Parameters:
  - `<unnamed>` (const std::string &): n/a
  - `<unnamed>` (uint32_t): n/a

#### `std::vector< uint8_t > getKey(const std::string &key_id) override`
- Source: `tests/exporters/test_huggingface_hub_client.cpp`:192
- Brief: n/a
- Parameters:
  - `key_id` (const std::string &): n/a

#### `std::vector< uint8_t > getKey(const std::string &key_id, uint32_t) override`
- Source: `tests/exporters/test_huggingface_hub_client.cpp`:200
- Brief: n/a
- Parameters:
  - `key_id` (const std::string &): n/a
  - `<unnamed>` (uint32_t): n/a

#### `themis::KeyMetadata getKeyMetadata(const std::string &key_id, uint32_t=0) override`
- Source: `tests/exporters/test_huggingface_hub_client.cpp`:207
- Brief: n/a
- Parameters:
  - `key_id` (const std::string &): n/a
  - `<unnamed>` (uint32_t): n/a

#### `bool hasKey(const std::string &key_id, uint32_t=0) override`
- Source: `tests/exporters/test_huggingface_hub_client.cpp`:215
- Brief: n/a
- Parameters:
  - `key_id` (const std::string &): n/a
  - `<unnamed>` (uint32_t): n/a

#### `std::vector< themis::KeyMetadata > listKeys() override`
- Source: `tests/exporters/test_huggingface_hub_client.cpp`:205
- Brief: n/a
- Parameters: none

#### `uint32_t rotateKey(const std::string &) override`
- Source: `tests/exporters/test_huggingface_hub_client.cpp`:204
- Brief: n/a
- Parameters:
  - `<unnamed>` (const std::string &): n/a

#### `void storeToken(const std::string &kek_id, const std::string &token)`
- Source: `tests/exporters/test_huggingface_hub_client.cpp`:188
- Brief: Register a token for the given kek_id.
- Parameters:
  - `kek_id` (const std::string &): n/a
  - `token` (const std::string &): n/a

### ValidateTemplateTest

#### `FormatTemplateFieldMapping defaultMapping() const`
- Source: `tests/exporters/test_format_template.cpp`:535
- Brief: n/a
- Parameters: none

#### `BaseEntity makeAlpacaEntity(const std::string &pk, bool with_instruction=true, bool with_output=true) const`
- Source: `tests/exporters/test_format_template.cpp`:537
- Brief: n/a
- Parameters:
  - `pk` (const std::string &): n/a
  - `with_instruction` (bool): n/a
  - `with_output` (bool): n/a

#### `BaseEntity makeChatEntity(const std::string &pk, bool with_user=true, bool with_assistant=true) const`
- Source: `tests/exporters/test_format_template.cpp`:551
- Brief: n/a
- Parameters:
  - `pk` (const std::string &): n/a
  - `with_user` (bool): n/a
  - `with_assistant` (bool): n/a

### bench_csv_export.cpp

#### `BENCHMARK(BM_CsvExport_1M) -> Unit(benchmark::kMillisecond)`
- Source: `benchmarks/exporters/bench_csv_export.cpp`:78
- Brief: n/a
- Parameters:
  - `<unnamed>` (BM_CsvExport_1M): n/a

#### `BENCHMARK_MAIN()`
- Source: `benchmarks/exporters/bench_csv_export.cpp`:80
- Brief: n/a
- Parameters: none

#### `void BM_CsvExport_1M(benchmark::State &state)`
- Source: `benchmarks/exporters/bench_csv_export.cpp`:48
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

### bench_exporters.cpp

#### `Arg(100) -> Arg(1000) ->Arg(10000) ->Unit(benchmark::kMillisecond)`
- Source: `benchmarks/exporters/bench_exporters.cpp`:90
- Brief: n/a
- Parameters:
  - `<unnamed>` (100): n/a

#### `Arg(1000) -> Arg(5000) ->Unit(benchmark::kMillisecond)`
- Source: `benchmarks/exporters/bench_exporters.cpp`:199
- Brief: n/a
- Parameters:
  - `<unnamed>` (1000): n/a

#### `Arg(static_cast< int >(FormatTemplateType::ALPACA)) -> Arg(static_cast< int >(FormatTemplateType::SHAREGPT)) ->Arg(static_cast< int >(FormatTemplateType::CHATML)) ->Arg(static_cast< int >(FormatTemplateType::OPENAI_FINETUNING)) ->Unit(benchmark::kMillisecond)`
- Source: `benchmarks/exporters/bench_exporters.cpp`:134
- Brief: n/a
- Parameters:
  - `<unnamed>` (static_cast< int >): n/a

#### `BENCHMARK_MAIN()`
- Source: `benchmarks/exporters/bench_exporters.cpp`:355
- Brief: n/a
- Parameters: none

#### `void BM_Export_CSV_1M(benchmark::State &state)`
- Source: `benchmarks/exporters/bench_exporters.cpp`:324
- Brief: CSV-target 1M row export throughput (CSV proxy benchmark).
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_Export_Parquet_1M(benchmark::State &state)`
- Source: `benchmarks/exporters/bench_exporters.cpp`:291
- Brief: Parquet-target 1M row export throughput (PR proxy benchmark).
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_IncrementalExport_Delta(benchmark::State &state)`
- Source: `benchmarks/exporters/bench_exporters.cpp`:238
- Brief: Delta export: watermark already set to seq N-10; only ~10 new entities are exported.
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_IncrementalExport_Full(benchmark::State &state)`
- Source: `benchmarks/exporters/bench_exporters.cpp`:208
- Brief: Full export baseline: export all N entities.
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_JsonlExport_BatchThroughput(benchmark::State &state)`
- Source: `benchmarks/exporters/bench_exporters.cpp`:66
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a
- Details: Measures per-entity throughput for the default INSTRUCTION_TUNING style. State.range(0) = number of entities to export.

#### `void BM_JsonlExport_Compressed(benchmark::State &state)`
- Source: `benchmarks/exporters/bench_exporters.cpp`:144
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_JsonlExport_FormatTemplate(benchmark::State &state)`
- Source: `benchmarks/exporters/bench_exporters.cpp`:101
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a
- Details: Measures per-entity throughput for ChatML and Alpaca templates. State.range(0) = FormatTemplateType (1 = ALPACA, 2 = SHAREGPT/ChatML)

#### `void BM_StreamingExport_Throughput(benchmark::State &state)`
- Source: `benchmarks/exporters/bench_exporters.cpp`:174
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `Unit(benchmark::kMillisecond)`
- Source: `benchmarks/exporters/bench_exporters.cpp`:168
- Brief: n/a
- Parameters:
  - `<unnamed>` (benchmark::kMillisecond): n/a

#### `Unit(benchmark::kSecond)`
- Source: `benchmarks/exporters/bench_exporters.cpp`:321
- Brief: n/a
- Parameters:
  - `<unnamed>` (benchmark::kSecond): n/a

#### `std::vector< BaseEntity > makeEntities(int n)`
- Source: `benchmarks/exporters/bench_exporters.cpp`:36
- Brief: n/a
- Parameters:
  - `n` (int): n/a

#### `ExportOptions makeOptions(const std::string &path)`
- Source: `benchmarks/exporters/bench_exporters.cpp`:52
- Brief: n/a
- Parameters:
  - `path` (const std::string &): n/a

### bench_exporters_dedicated_gates.cpp

#### `BENCHMARK(EX_BM_01_MetricExport_Throughput) -> Repetitions(5) ->ReportAggregatesOnly(true)`
- Source: `benchmarks/exporters/bench_exporters_dedicated_gates.cpp`:91
- Brief: n/a
- Parameters:
  - `<unnamed>` (EX_BM_01_MetricExport_Throughput): n/a

#### `BENCHMARK(EX_BM_02_TraceExport_Throughput) -> Repetitions(5) ->ReportAggregatesOnly(true)`
- Source: `benchmarks/exporters/bench_exporters_dedicated_gates.cpp`:107
- Brief: n/a
- Parameters:
  - `<unnamed>` (EX_BM_02_TraceExport_Throughput): n/a

#### `BENCHMARK(EX_BM_03_ExportQueue_Enqueue_Latency) -> Repetitions(5) ->ReportAggregatesOnly(true)`
- Source: `benchmarks/exporters/bench_exporters_dedicated_gates.cpp`:126
- Brief: n/a
- Parameters:
  - `<unnamed>` (EX_BM_03_ExportQueue_Enqueue_Latency): n/a

#### `BENCHMARK(EX_BM_04_ExportQueue_Dequeue_Latency) -> Repetitions(5) ->ReportAggregatesOnly(true)`
- Source: `benchmarks/exporters/bench_exporters_dedicated_gates.cpp`:147
- Brief: n/a
- Parameters:
  - `<unnamed>` (EX_BM_04_ExportQueue_Dequeue_Latency): n/a

#### `BENCHMARK_MAIN()`
- Source: `benchmarks/exporters/bench_exporters_dedicated_gates.cpp`:149
- Brief: n/a
- Parameters: none

#### `void EX_BM_01_MetricExport_Throughput(benchmark::State &state)`
- Source: `benchmarks/exporters/bench_exporters_dedicated_gates.cpp`:80
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void EX_BM_02_TraceExport_Throughput(benchmark::State &state)`
- Source: `benchmarks/exporters/bench_exporters_dedicated_gates.cpp`:96
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void EX_BM_03_ExportQueue_Enqueue_Latency(benchmark::State &state)`
- Source: `benchmarks/exporters/bench_exporters_dedicated_gates.cpp`:112
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void EX_BM_04_ExportQueue_Dequeue_Latency(benchmark::State &state)`
- Source: `benchmarks/exporters/bench_exporters_dedicated_gates.cpp`:131
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

### bench_exporters_release_gates.cpp

#### `BENCHMARK_MAIN()`
- Source: `benchmarks/exporters/bench_exporters_release_gates.cpp`:367
- Brief: n/a
- Parameters: none

### bench_parquet_export.cpp

#### `BENCHMARK(BM_ParquetExport_1M) -> Unit(benchmark::kMillisecond)`
- Source: `benchmarks/exporters/bench_parquet_export.cpp`:86
- Brief: n/a
- Parameters:
  - `<unnamed>` (BM_ParquetExport_1M): n/a

#### `BENCHMARK_MAIN()`
- Source: `benchmarks/exporters/bench_parquet_export.cpp`:88
- Brief: n/a
- Parameters: none

#### `void BM_ParquetExport_1M(benchmark::State &state)`
- Source: `benchmarks/exporters/bench_parquet_export.cpp`:50
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

### test_aql_predicate_filter.cpp

#### `TEST_F(AqlPredicateFilterIntegrationTest, JSONLExporterFiltersEntities)`
- Source: `tests/exporters/test_aql_predicate_filter.cpp`:196
- Brief: n/a
- Parameters:
  - `<unnamed>` (AqlPredicateFilterIntegrationTest): n/a
  - `<unnamed>` (JSONLExporterFiltersEntities): n/a

#### `TEST_F(AqlPredicateFilterIntegrationTest, JSONLExporterInvalidFilterThrows)`
- Source: `tests/exporters/test_aql_predicate_filter.cpp`:234
- Brief: n/a
- Parameters:
  - `<unnamed>` (AqlPredicateFilterIntegrationTest): n/a
  - `<unnamed>` (JSONLExporterInvalidFilterThrows): n/a

#### `TEST_F(AqlPredicateFilterIntegrationTest, JSONLExporterNoFilterExportsAll)`
- Source: `tests/exporters/test_aql_predicate_filter.cpp`:218
- Brief: n/a
- Parameters:
  - `<unnamed>` (AqlPredicateFilterIntegrationTest): n/a
  - `<unnamed>` (JSONLExporterNoFilterExportsAll): n/a

#### `TEST_F(AqlPredicateFilterTest, DoubleFieldFilter)`
- Source: `tests/exporters/test_aql_predicate_filter.cpp`:114
- Brief: n/a
- Parameters:
  - `<unnamed>` (AqlPredicateFilterTest): n/a
  - `<unnamed>` (DoubleFieldFilter): n/a

#### `TEST_F(AqlPredicateFilterTest, EmptyPredicateAcceptsAll)`
- Source: `tests/exporters/test_aql_predicate_filter.cpp`:72
- Brief: n/a
- Parameters:
  - `<unnamed>` (AqlPredicateFilterTest): n/a
  - `<unnamed>` (EmptyPredicateAcceptsAll): n/a

#### `TEST_F(AqlPredicateFilterTest, EqualityStringFilter)`
- Source: `tests/exporters/test_aql_predicate_filter.cpp`:79
- Brief: n/a
- Parameters:
  - `<unnamed>` (AqlPredicateFilterTest): n/a
  - `<unnamed>` (EqualityStringFilter): n/a

#### `TEST_F(AqlPredicateFilterTest, GetPredicateReturnsOriginal)`
- Source: `tests/exporters/test_aql_predicate_filter.cpp`:148
- Brief: n/a
- Parameters:
  - `<unnamed>` (AqlPredicateFilterTest): n/a
  - `<unnamed>` (GetPredicateReturnsOriginal): n/a

#### `TEST_F(AqlPredicateFilterTest, InequalityStringFilter)`
- Source: `tests/exporters/test_aql_predicate_filter.cpp`:86
- Brief: n/a
- Parameters:
  - `<unnamed>` (AqlPredicateFilterTest): n/a
  - `<unnamed>` (InequalityStringFilter): n/a

#### `TEST_F(AqlPredicateFilterTest, InvalidPredicateThrows)`
- Source: `tests/exporters/test_aql_predicate_filter.cpp`:135
- Brief: n/a
- Parameters:
  - `<unnamed>` (AqlPredicateFilterTest): n/a
  - `<unnamed>` (InvalidPredicateThrows): n/a

#### `TEST_F(AqlPredicateFilterTest, LogicalAndFilter)`
- Source: `tests/exporters/test_aql_predicate_filter.cpp`:121
- Brief: n/a
- Parameters:
  - `<unnamed>` (AqlPredicateFilterTest): n/a
  - `<unnamed>` (LogicalAndFilter): n/a

#### `TEST_F(AqlPredicateFilterTest, LogicalOrFilter)`
- Source: `tests/exporters/test_aql_predicate_filter.cpp`:128
- Brief: n/a
- Parameters:
  - `<unnamed>` (AqlPredicateFilterTest): n/a
  - `<unnamed>` (LogicalOrFilter): n/a

#### `TEST_F(AqlPredicateFilterTest, MissingFieldReturnsFalse)`
- Source: `tests/exporters/test_aql_predicate_filter.cpp`:142
- Brief: n/a
- Parameters:
  - `<unnamed>` (AqlPredicateFilterTest): n/a
  - `<unnamed>` (MissingFieldReturnsFalse): n/a

#### `TEST_F(AqlPredicateFilterTest, NumericGreaterEqualFilter)`
- Source: `tests/exporters/test_aql_predicate_filter.cpp`:107
- Brief: n/a
- Parameters:
  - `<unnamed>` (AqlPredicateFilterTest): n/a
  - `<unnamed>` (NumericGreaterEqualFilter): n/a

#### `TEST_F(AqlPredicateFilterTest, NumericGreaterThanFilter)`
- Source: `tests/exporters/test_aql_predicate_filter.cpp`:93
- Brief: n/a
- Parameters:
  - `<unnamed>` (AqlPredicateFilterTest): n/a
  - `<unnamed>` (NumericGreaterThanFilter): n/a

#### `TEST_F(AqlPredicateFilterTest, NumericLessThanFilter)`
- Source: `tests/exporters/test_aql_predicate_filter.cpp`:100
- Brief: n/a
- Parameters:
  - `<unnamed>` (AqlPredicateFilterTest): n/a
  - `<unnamed>` (NumericLessThanFilter): n/a

### test_arrow_ipc_exporter.cpp

#### `TEST_F(ArrowIPCExporterTest, BasicFileExport)`
- Source: `tests/exporters/test_arrow_ipc_exporter.cpp`:116
- Brief: n/a
- Parameters:
  - `<unnamed>` (ArrowIPCExporterTest): n/a
  - `<unnamed>` (BasicFileExport): n/a

#### `TEST_F(ArrowIPCExporterTest, EmptyEntitiesFileFormat)`
- Source: `tests/exporters/test_arrow_ipc_exporter.cpp`:216
- Brief: n/a
- Parameters:
  - `<unnamed>` (ArrowIPCExporterTest): n/a
  - `<unnamed>` (EmptyEntitiesFileFormat): n/a

#### `TEST_F(ArrowIPCExporterTest, EmptyEntitiesStreamFormat)`
- Source: `tests/exporters/test_arrow_ipc_exporter.cpp`:231
- Brief: n/a
- Parameters:
  - `<unnamed>` (ArrowIPCExporterTest): n/a
  - `<unnamed>` (EmptyEntitiesStreamFormat): n/a

#### `TEST_F(ArrowIPCExporterTest, EmptyOutputPathThrows)`
- Source: `tests/exporters/test_arrow_ipc_exporter.cpp`:346
- Brief: n/a
- Parameters:
  - `<unnamed>` (ArrowIPCExporterTest): n/a
  - `<unnamed>` (EmptyOutputPathThrows): n/a

#### `TEST_F(ArrowIPCExporterTest, ExcludeColumns)`
- Source: `tests/exporters/test_arrow_ipc_exporter.cpp`:307
- Brief: n/a
- Parameters:
  - `<unnamed>` (ArrowIPCExporterTest): n/a
  - `<unnamed>` (ExcludeColumns): n/a

#### `TEST_F(ArrowIPCExporterTest, ExcludeColumnsViaOptions)`
- Source: `tests/exporters/test_arrow_ipc_exporter.cpp`:331
- Brief: n/a
- Parameters:
  - `<unnamed>` (ArrowIPCExporterTest): n/a
  - `<unnamed>` (ExcludeColumnsViaOptions): n/a

#### `TEST_F(ArrowIPCExporterTest, FileFormatHasLeadingMagic)`
- Source: `tests/exporters/test_arrow_ipc_exporter.cpp`:130
- Brief: n/a
- Parameters:
  - `<unnamed>` (ArrowIPCExporterTest): n/a
  - `<unnamed>` (FileFormatHasLeadingMagic): n/a

#### `TEST_F(ArrowIPCExporterTest, FileFormatHasSchemaMarker)`
- Source: `tests/exporters/test_arrow_ipc_exporter.cpp`:148
- Brief: n/a
- Parameters:
  - `<unnamed>` (ArrowIPCExporterTest): n/a
  - `<unnamed>` (FileFormatHasSchemaMarker): n/a

#### `TEST_F(ArrowIPCExporterTest, FileFormatHasTrailingMagic)`
- Source: `tests/exporters/test_arrow_ipc_exporter.cpp`:139
- Brief: n/a
- Parameters:
  - `<unnamed>` (ArrowIPCExporterTest): n/a
  - `<unnamed>` (FileFormatHasTrailingMagic): n/a

#### `TEST_F(ArrowIPCExporterTest, FileFormatMinimumSize)`
- Source: `tests/exporters/test_arrow_ipc_exporter.cpp`:157
- Brief: n/a
- Parameters:
  - `<unnamed>` (ArrowIPCExporterTest): n/a
  - `<unnamed>` (FileFormatMinimumSize): n/a

#### `TEST_F(ArrowIPCExporterTest, GetName)`
- Source: `tests/exporters/test_arrow_ipc_exporter.cpp`:386
- Brief: n/a
- Parameters:
  - `<unnamed>` (ArrowIPCExporterTest): n/a
  - `<unnamed>` (GetName): n/a

#### `TEST_F(ArrowIPCExporterTest, GetVersion)`
- Source: `tests/exporters/test_arrow_ipc_exporter.cpp`:391
- Brief: n/a
- Parameters:
  - `<unnamed>` (ArrowIPCExporterTest): n/a
  - `<unnamed>` (GetVersion): n/a

#### `TEST_F(ArrowIPCExporterTest, IncludeColumns)`
- Source: `tests/exporters/test_arrow_ipc_exporter.cpp`:294
- Brief: n/a
- Parameters:
  - `<unnamed>` (ArrowIPCExporterTest): n/a
  - `<unnamed>` (IncludeColumns): n/a

#### `TEST_F(ArrowIPCExporterTest, IncludeColumnsViaOptions)`
- Source: `tests/exporters/test_arrow_ipc_exporter.cpp`:320
- Brief: n/a
- Parameters:
  - `<unnamed>` (ArrowIPCExporterTest): n/a
  - `<unnamed>` (IncludeColumnsViaOptions): n/a

#### `TEST_F(ArrowIPCExporterTest, InvalidDirectoryThrows)`
- Source: `tests/exporters/test_arrow_ipc_exporter.cpp`:354
- Brief: n/a
- Parameters:
  - `<unnamed>` (ArrowIPCExporterTest): n/a
  - `<unnamed>` (InvalidDirectoryThrows): n/a

#### `TEST_F(ArrowIPCExporterTest, IsArrowAvailableReturnsBool)`
- Source: `tests/exporters/test_arrow_ipc_exporter.cpp`:407
- Brief: n/a
- Parameters:
  - `<unnamed>` (ArrowIPCExporterTest): n/a
  - `<unnamed>` (IsArrowAvailableReturnsBool): n/a

#### `TEST_F(ArrowIPCExporterTest, LargeEntitySet)`
- Source: `tests/exporters/test_arrow_ipc_exporter.cpp`:439
- Brief: n/a
- Parameters:
  - `<unnamed>` (ArrowIPCExporterTest): n/a
  - `<unnamed>` (LargeEntitySet): n/a

#### `TEST_F(ArrowIPCExporterTest, MetricsResetWorks)`
- Source: `tests/exporters/test_arrow_ipc_exporter.cpp`:422
- Brief: n/a
- Parameters:
  - `<unnamed>` (ArrowIPCExporterTest): n/a
  - `<unnamed>` (MetricsResetWorks): n/a

#### `TEST_F(ArrowIPCExporterTest, OverwriteExistingFile)`
- Source: `tests/exporters/test_arrow_ipc_exporter.cpp`:464
- Brief: n/a
- Parameters:
  - `<unnamed>` (ArrowIPCExporterTest): n/a
  - `<unnamed>` (OverwriteExistingFile): n/a

#### `TEST_F(ArrowIPCExporterTest, ProgressCallbackInvoked)`
- Source: `tests/exporters/test_arrow_ipc_exporter.cpp`:366
- Brief: n/a
- Parameters:
  - `<unnamed>` (ArrowIPCExporterTest): n/a
  - `<unnamed>` (ProgressCallbackInvoked): n/a

#### `TEST_F(ArrowIPCExporterTest, SetAndGetConfig)`
- Source: `tests/exporters/test_arrow_ipc_exporter.cpp`:414
- Brief: n/a
- Parameters:
  - `<unnamed>` (ArrowIPCExporterTest): n/a
  - `<unnamed>` (SetAndGetConfig): n/a

#### `TEST_F(ArrowIPCExporterTest, StatsBytesWrittenMatchesFileSize)`
- Source: `tests/exporters/test_arrow_ipc_exporter.cpp`:261
- Brief: n/a
- Parameters:
  - `<unnamed>` (ArrowIPCExporterTest): n/a
  - `<unnamed>` (StatsBytesWrittenMatchesFileSize): n/a

#### `TEST_F(ArrowIPCExporterTest, StatsDurationNonNegative)`
- Source: `tests/exporters/test_arrow_ipc_exporter.cpp`:281
- Brief: n/a
- Parameters:
  - `<unnamed>` (ArrowIPCExporterTest): n/a
  - `<unnamed>` (StatsDurationNonNegative): n/a

#### `TEST_F(ArrowIPCExporterTest, StatsEntityCountMatchesInput)`
- Source: `tests/exporters/test_arrow_ipc_exporter.cpp`:251
- Brief: n/a
- Parameters:
  - `<unnamed>` (ArrowIPCExporterTest): n/a
  - `<unnamed>` (StatsEntityCountMatchesInput): n/a

#### `TEST_F(ArrowIPCExporterTest, StatsMetricsAttached)`
- Source: `tests/exporters/test_arrow_ipc_exporter.cpp`:272
- Brief: n/a
- Parameters:
  - `<unnamed>` (ArrowIPCExporterTest): n/a
  - `<unnamed>` (StatsMetricsAttached): n/a

#### `TEST_F(ArrowIPCExporterTest, StreamFormatExport)`
- Source: `tests/exporters/test_arrow_ipc_exporter.cpp`:172
- Brief: n/a
- Parameters:
  - `<unnamed>` (ArrowIPCExporterTest): n/a
  - `<unnamed>` (StreamFormatExport): n/a

#### `TEST_F(ArrowIPCExporterTest, StreamFormatHasSchemaMarker)`
- Source: `tests/exporters/test_arrow_ipc_exporter.cpp`:186
- Brief: n/a
- Parameters:
  - `<unnamed>` (ArrowIPCExporterTest): n/a
  - `<unnamed>` (StreamFormatHasSchemaMarker): n/a

#### `TEST_F(ArrowIPCExporterTest, StreamFormatNoLeadingMagic)`
- Source: `tests/exporters/test_arrow_ipc_exporter.cpp`:199
- Brief: n/a
- Parameters:
  - `<unnamed>` (ArrowIPCExporterTest): n/a
  - `<unnamed>` (StreamFormatNoLeadingMagic): n/a

#### `TEST_F(ArrowIPCExporterTest, SupportedFormatsIncludesArrow)`
- Source: `tests/exporters/test_arrow_ipc_exporter.cpp`:396
- Brief: n/a
- Parameters:
  - `<unnamed>` (ArrowIPCExporterTest): n/a
  - `<unnamed>` (SupportedFormatsIncludesArrow): n/a

#### `bool hasArrowMagic(const std::string &path, std::streamoff offset)`
- Source: `tests/exporters/test_arrow_ipc_exporter.cpp`:20
- Brief: Verify the 8-byte Arrow magic bytes ("ARROW1\0\0") at position offset.
- Parameters:
  - `path` (const std::string &): n/a
  - `offset` (std::streamoff): n/a

#### `bool hasSchemaMarker(const std::string &path, bool is_file_format)`
- Source: `tests/exporters/test_arrow_ipc_exporter.cpp`:57
- Brief: n/a
- Parameters:
  - `path` (const std::string &): n/a
  - `is_file_format` (bool): n/a
- Details: Read the first 4 bytes after the leading magic and verify they are the continuation marker (0xFFFFFFFF = -1 as LE int32).

#### `bool hasTrailingArrowMagic(const std::string &path)`
- Source: `tests/exporters/test_arrow_ipc_exporter.cpp`:39
- Brief: Read the trailing 8 bytes and verify they are the Arrow magic.
- Parameters:
  - `path` (const std::string &): n/a

### test_data_augmentation.cpp

#### `TEST_F(DataAugmentationTest, ApplyStrategyReturnsRequestedCount)`
- Source: `tests/exporters/test_data_augmentation.cpp`:470
- Brief: n/a
- Parameters:
  - `<unnamed>` (DataAugmentationTest): n/a
  - `<unnamed>` (ApplyStrategyReturnsRequestedCount): n/a

#### `TEST_F(DataAugmentationTest, AugmentFieldsRestrictsScope)`
- Source: `tests/exporters/test_data_augmentation.cpp`:391
- Brief: n/a
- Parameters:
  - `<unnamed>` (DataAugmentationTest): n/a
  - `<unnamed>` (AugmentFieldsRestrictsScope): n/a

#### `TEST_F(DataAugmentationTest, AugmentedEntitiesExportToJSONL)`
- Source: `tests/exporters/test_data_augmentation.cpp`:527
- Brief: n/a
- Parameters:
  - `<unnamed>` (DataAugmentationTest): n/a
  - `<unnamed>` (AugmentedEntitiesExportToJSONL): n/a

#### `TEST_F(DataAugmentationTest, AugmentedKeyPrefixIsApplied)`
- Source: `tests/exporters/test_data_augmentation.cpp`:118
- Brief: n/a
- Parameters:
  - `<unnamed>` (DataAugmentationTest): n/a
  - `<unnamed>` (AugmentedKeyPrefixIsApplied): n/a

#### `TEST_F(DataAugmentationTest, CustomSynonymIsUsed)`
- Source: `tests/exporters/test_data_augmentation.cpp`:291
- Brief: n/a
- Parameters:
  - `<unnamed>` (DataAugmentationTest): n/a
  - `<unnamed>` (CustomSynonymIsUsed): n/a

#### `TEST_F(DataAugmentationTest, EmptyInputReturnsEmpty)`
- Source: `tests/exporters/test_data_augmentation.cpp`:487
- Brief: n/a
- Parameters:
  - `<unnamed>` (DataAugmentationTest): n/a
  - `<unnamed>` (EmptyInputReturnsEmpty): n/a

#### `TEST_F(DataAugmentationTest, EmptyStrategiesReturnsOriginalsOnly)`
- Source: `tests/exporters/test_data_augmentation.cpp`:51
- Brief: n/a
- Parameters:
  - `<unnamed>` (DataAugmentationTest): n/a
  - `<unnamed>` (EmptyStrategiesReturnsOriginalsOnly): n/a

#### `TEST_F(DataAugmentationTest, ExcludeOriginalsReturnsOnlySynthetic)`
- Source: `tests/exporters/test_data_augmentation.cpp`:67
- Brief: n/a
- Parameters:
  - `<unnamed>` (DataAugmentationTest): n/a
  - `<unnamed>` (ExcludeOriginalsReturnsOnlySynthetic): n/a

#### `TEST_F(DataAugmentationTest, GetSetConfigRoundTrip)`
- Source: `tests/exporters/test_data_augmentation.cpp`:504
- Brief: n/a
- Parameters:
  - `<unnamed>` (DataAugmentationTest): n/a
  - `<unnamed>` (GetSetConfigRoundTrip): n/a

#### `TEST_F(DataAugmentationTest, IncludeOriginalsAddsToResult)`
- Source: `tests/exporters/test_data_augmentation.cpp`:81
- Brief: n/a
- Parameters:
  - `<unnamed>` (DataAugmentationTest): n/a
  - `<unnamed>` (IncludeOriginalsAddsToResult): n/a

#### `TEST_F(DataAugmentationTest, LowercaseStrategyConvertsAllCaps)`
- Source: `tests/exporters/test_data_augmentation.cpp`:137
- Brief: n/a
- Parameters:
  - `<unnamed>` (DataAugmentationTest): n/a
  - `<unnamed>` (LowercaseStrategyConvertsAllCaps): n/a

#### `TEST_F(DataAugmentationTest, MultipleStrategiesProduceExpectedCount)`
- Source: `tests/exporters/test_data_augmentation.cpp`:422
- Brief: n/a
- Parameters:
  - `<unnamed>` (DataAugmentationTest): n/a
  - `<unnamed>` (MultipleStrategiesProduceExpectedCount): n/a

#### `TEST_F(DataAugmentationTest, QuestionReformulationChangesPrefix)`
- Source: `tests/exporters/test_data_augmentation.cpp`:317
- Brief: n/a
- Parameters:
  - `<unnamed>` (DataAugmentationTest): n/a
  - `<unnamed>` (QuestionReformulationChangesPrefix): n/a

#### `TEST_F(DataAugmentationTest, QuestionReformulationCustomInstructionField)`
- Source: `tests/exporters/test_data_augmentation.cpp`:366
- Brief: n/a
- Parameters:
  - `<unnamed>` (DataAugmentationTest): n/a
  - `<unnamed>` (QuestionReformulationCustomInstructionField): n/a

#### `TEST_F(DataAugmentationTest, QuestionReformulationMultipleVariantsDiffer)`
- Source: `tests/exporters/test_data_augmentation.cpp`:339
- Brief: n/a
- Parameters:
  - `<unnamed>` (DataAugmentationTest): n/a
  - `<unnamed>` (QuestionReformulationMultipleVariantsDiffer): n/a

#### `TEST_F(DataAugmentationTest, SentenceCasingUppercasesFirstChar)`
- Source: `tests/exporters/test_data_augmentation.cpp`:165
- Brief: n/a
- Parameters:
  - `<unnamed>` (DataAugmentationTest): n/a
  - `<unnamed>` (SentenceCasingUppercasesFirstChar): n/a

#### `TEST_F(DataAugmentationTest, SynonymReplacementChangesKnownWord)`
- Source: `tests/exporters/test_data_augmentation.cpp`:222
- Brief: n/a
- Parameters:
  - `<unnamed>` (DataAugmentationTest): n/a
  - `<unnamed>` (SynonymReplacementChangesKnownWord): n/a

#### `TEST_F(DataAugmentationTest, SynonymReplacementMultipleVariants)`
- Source: `tests/exporters/test_data_augmentation.cpp`:273
- Brief: n/a
- Parameters:
  - `<unnamed>` (DataAugmentationTest): n/a
  - `<unnamed>` (SynonymReplacementMultipleVariants): n/a

#### `TEST_F(DataAugmentationTest, SynonymReplacementPreservesCapitalization)`
- Source: `tests/exporters/test_data_augmentation.cpp`:250
- Brief: n/a
- Parameters:
  - `<unnamed>` (DataAugmentationTest): n/a
  - `<unnamed>` (SynonymReplacementPreservesCapitalization): n/a

#### `TEST_F(DataAugmentationTest, SyntheticEntitiesHaveUniqueKeys)`
- Source: `tests/exporters/test_data_augmentation.cpp`:99
- Brief: n/a
- Parameters:
  - `<unnamed>` (DataAugmentationTest): n/a
  - `<unnamed>` (SyntheticEntitiesHaveUniqueKeys): n/a

#### `TEST_F(DataAugmentationTest, WhitespaceNormalizationCollapseSpaces)`
- Source: `tests/exporters/test_data_augmentation.cpp`:194
- Brief: n/a
- Parameters:
  - `<unnamed>` (DataAugmentationTest): n/a
  - `<unnamed>` (WhitespaceNormalizationCollapseSpaces): n/a

#### `TEST_F(DataAugmentationTest, ZeroCountStrategyIsNoop)`
- Source: `tests/exporters/test_data_augmentation.cpp`:450
- Brief: n/a
- Parameters:
  - `<unnamed>` (DataAugmentationTest): n/a
  - `<unnamed>` (ZeroCountStrategyIsNoop): n/a

#### `BaseEntity makeEntity(const std::string &pk, const std::string &question, const std::string &answer, const std::string &context="")`
- Source: `tests/exporters/test_data_augmentation.cpp`:16
- Brief: n/a
- Parameters:
  - `pk` (const std::string &): n/a
  - `question` (const std::string &): n/a
  - `answer` (const std::string &): n/a
  - `context` (const std::string &): n/a

### test_export_encryption.cpp

#### `TEST_F(ExportEncryptionTest, ConfigEmptyByDefault)`
- Source: `tests/exporters/test_export_encryption.cpp`:477
- Brief: n/a
- Parameters:
  - `<unnamed>` (ExportEncryptionTest): n/a
  - `<unnamed>` (ConfigEmptyByDefault): n/a

#### `TEST_F(ExportEncryptionTest, ConfigEmptyWithoutKeyProvider)`
- Source: `tests/exporters/test_export_encryption.cpp`:493
- Brief: n/a
- Parameters:
  - `<unnamed>` (ExportEncryptionTest): n/a
  - `<unnamed>` (ConfigEmptyWithoutKeyProvider): n/a

#### `TEST_F(ExportEncryptionTest, ConfigNotEmptyWhenSet)`
- Source: `tests/exporters/test_export_encryption.cpp`:484
- Brief: n/a
- Parameters:
  - `<unnamed>` (ExportEncryptionTest): n/a
  - `<unnamed>` (ConfigNotEmptyWhenSet): n/a

#### `TEST_F(ExportEncryptionTest, DISABLED_WrongJobIdFailsAuthentication)`
- Source: `tests/exporters/test_export_encryption.cpp`:614
- Brief: n/a
- Parameters:
  - `<unnamed>` (ExportEncryptionTest): n/a
  - `<unnamed>` (DISABLED_WrongJobIdFailsAuthentication): n/a

#### `TEST_F(ExportEncryptionTest, DifferentJobIdProducesDifferentCiphertext)`
- Source: `tests/exporters/test_export_encryption.cpp`:174
- Brief: n/a
- Parameters:
  - `<unnamed>` (ExportEncryptionTest): n/a
  - `<unnamed>` (DifferentJobIdProducesDifferentCiphertext): n/a

#### `TEST_F(ExportEncryptionTest, DisabledEncryptFileCopiesFile)`
- Source: `tests/exporters/test_export_encryption.cpp`:328
- Brief: n/a
- Parameters:
  - `<unnamed>` (ExportEncryptionTest): n/a
  - `<unnamed>` (DisabledEncryptFileCopiesFile): n/a

#### `TEST_F(ExportEncryptionTest, DisabledEncryptionPassesThrough)`
- Source: `tests/exporters/test_export_encryption.cpp`:270
- Brief: n/a
- Parameters:
  - `<unnamed>` (ExportEncryptionTest): n/a
  - `<unnamed>` (DisabledEncryptionPassesThrough): n/a

#### `TEST_F(ExportEncryptionTest, EmptyConfigThrows)`
- Source: `tests/exporters/test_export_encryption.cpp`:652
- Brief: n/a
- Parameters:
  - `<unnamed>` (ExportEncryptionTest): n/a
  - `<unnamed>` (EmptyConfigThrows): n/a

#### `TEST_F(ExportEncryptionTest, EmptyJobIdThrows)`
- Source: `tests/exporters/test_export_encryption.cpp`:366
- Brief: n/a
- Parameters:
  - `<unnamed>` (ExportEncryptionTest): n/a
  - `<unnamed>` (EmptyJobIdThrows): n/a

#### `TEST_F(ExportEncryptionTest, EncryptDecryptEmptyPayload)`
- Source: `tests/exporters/test_export_encryption.cpp`:143
- Brief: n/a
- Parameters:
  - `<unnamed>` (ExportEncryptionTest): n/a
  - `<unnamed>` (EncryptDecryptEmptyPayload): n/a

#### `TEST_F(ExportEncryptionTest, EncryptDecryptFile)`
- Source: `tests/exporters/test_export_encryption.cpp`:289
- Brief: n/a
- Parameters:
  - `<unnamed>` (ExportEncryptionTest): n/a
  - `<unnamed>` (EncryptDecryptFile): n/a

#### `TEST_F(ExportEncryptionTest, EncryptDecryptLargePayload)`
- Source: `tests/exporters/test_export_encryption.cpp`:154
- Brief: n/a
- Parameters:
  - `<unnamed>` (ExportEncryptionTest): n/a
  - `<unnamed>` (EncryptDecryptLargePayload): n/a

#### `TEST_F(ExportEncryptionTest, EncryptDecryptRoundTrip)`
- Source: `tests/exporters/test_export_encryption.cpp`:126
- Brief: n/a
- Parameters:
  - `<unnamed>` (ExportEncryptionTest): n/a
  - `<unnamed>` (EncryptDecryptRoundTrip): n/a

#### `TEST_F(ExportEncryptionTest, EncryptedFileDoesNotContainKekId_InPlaintext)`
- Source: `tests/exporters/test_export_encryption.cpp`:698
- Brief: n/a
- Parameters:
  - `<unnamed>` (ExportEncryptionTest): n/a
  - `<unnamed>` (EncryptedFileDoesNotContainKekId_InPlaintext): n/a

#### `TEST_F(ExportEncryptionTest, ExportOptionsDefaultHasNoEncryption)`
- Source: `tests/exporters/test_export_encryption.cpp`:878
- Brief: n/a
- Parameters:
  - `<unnamed>` (ExportEncryptionTest): n/a
  - `<unnamed>` (ExportOptionsDefaultHasNoEncryption): n/a

#### `TEST_F(ExportEncryptionTest, ExportWithNoEncryptionProducesPlaintextJsonl)`
- Source: `tests/exporters/test_export_encryption.cpp`:884
- Brief: n/a
- Parameters:
  - `<unnamed>` (ExportEncryptionTest): n/a
  - `<unnamed>` (ExportWithNoEncryptionProducesPlaintextJsonl): n/a

#### `TEST_F(ExportEncryptionTest, IncrementalExporterEncryptsOutput)`
- Source: `tests/exporters/test_export_encryption.cpp`:812
- Brief: n/a
- Parameters:
  - `<unnamed>` (ExportEncryptionTest): n/a
  - `<unnamed>` (IncrementalExporterEncryptsOutput): n/a

#### `TEST_F(ExportEncryptionTest, InvalidMagicThrowsOnDecrypt)`
- Source: `tests/exporters/test_export_encryption.cpp`:682
- Brief: n/a
- Parameters:
  - `<unnamed>` (ExportEncryptionTest): n/a
  - `<unnamed>` (InvalidMagicThrowsOnDecrypt): n/a

#### `TEST_F(ExportEncryptionTest, JSONLLLMExporterEncryptsOutput)`
- Source: `tests/exporters/test_export_encryption.cpp`:722
- Brief: n/a
- Parameters:
  - `<unnamed>` (ExportEncryptionTest): n/a
  - `<unnamed>` (JSONLLLMExporterEncryptsOutput): n/a

#### `TEST_F(ExportEncryptionTest, JobACannotDecryptJobBOutput)`
- Source: `tests/exporters/test_export_encryption.cpp`:188
- Brief: n/a
- Parameters:
  - `<unnamed>` (ExportEncryptionTest): n/a
  - `<unnamed>` (JobACannotDecryptJobBOutput): n/a

#### `TEST_F(ExportEncryptionTest, MetricsEncryptionCounters)`
- Source: `tests/exporters/test_export_encryption.cpp`:390
- Brief: n/a
- Parameters:
  - `<unnamed>` (ExportEncryptionTest): n/a
  - `<unnamed>` (MetricsEncryptionCounters): n/a

#### `TEST_F(ExportEncryptionTest, MetricsEncryptionInToJson)`
- Source: `tests/exporters/test_export_encryption.cpp`:408
- Brief: n/a
- Parameters:
  - `<unnamed>` (ExportEncryptionTest): n/a
  - `<unnamed>` (MetricsEncryptionInToJson): n/a

#### `TEST_F(ExportEncryptionTest, MetricsTrackEncryptedBytes)`
- Source: `tests/exporters/test_export_encryption.cpp`:835
- Brief: n/a
- Parameters:
  - `<unnamed>` (ExportEncryptionTest): n/a
  - `<unnamed>` (MetricsTrackEncryptedBytes): n/a

#### `TEST_F(ExportEncryptionTest, MetricsZeroWithoutEncryption)`
- Source: `tests/exporters/test_export_encryption.cpp`:856
- Brief: n/a
- Parameters:
  - `<unnamed>` (ExportEncryptionTest): n/a
  - `<unnamed>` (MetricsZeroWithoutEncryption): n/a

#### `TEST_F(ExportEncryptionTest, MissingInputFileThrows)`
- Source: `tests/exporters/test_export_encryption.cpp`:641
- Brief: n/a
- Parameters:
  - `<unnamed>` (ExportEncryptionTest): n/a
  - `<unnamed>` (MissingInputFileThrows): n/a

#### `TEST_F(ExportEncryptionTest, MissingKeyProviderThrows)`
- Source: `tests/exporters/test_export_encryption.cpp`:354
- Brief: n/a
- Parameters:
  - `<unnamed>` (ExportEncryptionTest): n/a
  - `<unnamed>` (MissingKeyProviderThrows): n/a

#### `TEST_F(ExportEncryptionTest, MissingSourceFileThrows)`
- Source: `tests/exporters/test_export_encryption.cpp`:378
- Brief: n/a
- Parameters:
  - `<unnamed>` (ExportEncryptionTest): n/a
  - `<unnamed>` (MissingSourceFileThrows): n/a

#### `TEST_F(ExportEncryptionTest, NoKeyProviderForDecryptThrows)`
- Source: `tests/exporters/test_export_encryption.cpp`:662
- Brief: n/a
- Parameters:
  - `<unnamed>` (ExportEncryptionTest): n/a
  - `<unnamed>` (NoKeyProviderForDecryptThrows): n/a

#### `TEST_F(ExportEncryptionTest, OversizedHeaderStringIsRejected)`
- Source: `tests/exporters/test_export_encryption.cpp`:247
- Brief: n/a
- Parameters:
  - `<unnamed>` (ExportEncryptionTest): n/a
  - `<unnamed>` (OversizedHeaderStringIsRejected): n/a

#### `TEST_F(ExportEncryptionTest, RoundTripEmptyFile)`
- Source: `tests/exporters/test_export_encryption.cpp`:533
- Brief: n/a
- Parameters:
  - `<unnamed>` (ExportEncryptionTest): n/a
  - `<unnamed>` (RoundTripEmptyFile): n/a

#### `TEST_F(ExportEncryptionTest, RoundTripLargerThanOneChunk)`
- Source: `tests/exporters/test_export_encryption.cpp`:548
- Brief: n/a
- Parameters:
  - `<unnamed>` (ExportEncryptionTest): n/a
  - `<unnamed>` (RoundTripLargerThanOneChunk): n/a

#### `TEST_F(ExportEncryptionTest, RoundTripSmallFile)`
- Source: `tests/exporters/test_export_encryption.cpp`:504
- Brief: n/a
- Parameters:
  - `<unnamed>` (ExportEncryptionTest): n/a
  - `<unnamed>` (RoundTripSmallFile): n/a

#### `TEST_F(ExportEncryptionTest, StreamingExporterEncryptsOutput)`
- Source: `tests/exporters/test_export_encryption.cpp`:769
- Brief: n/a
- Parameters:
  - `<unnamed>` (ExportEncryptionTest): n/a
  - `<unnamed>` (StreamingExporterEncryptsOutput): n/a

#### `TEST_F(ExportEncryptionTest, StreamingExporterWithEncryption)`
- Source: `tests/exporters/test_export_encryption.cpp`:421
- Brief: n/a
- Parameters:
  - `<unnamed>` (ExportEncryptionTest): n/a
  - `<unnamed>` (StreamingExporterWithEncryption): n/a

#### `TEST_F(ExportEncryptionTest, TamperedCiphertextFailsAuthentication)`
- Source: `tests/exporters/test_export_encryption.cpp`:575
- Brief: n/a
- Parameters:
  - `<unnamed>` (ExportEncryptionTest): n/a
  - `<unnamed>` (TamperedCiphertextFailsAuthentication): n/a

#### `TEST_F(ExportEncryptionTest, TamperedCiphertextIsRejected)`
- Source: `tests/exporters/test_export_encryption.cpp`:211
- Brief: n/a
- Parameters:
  - `<unnamed>` (ExportEncryptionTest): n/a
  - `<unnamed>` (TamperedCiphertextIsRejected): n/a

#### `TEST_F(ExportEncryptionTest, TruncatedContainerIsRejected)`
- Source: `tests/exporters/test_export_encryption.cpp`:225
- Brief: n/a
- Parameters:
  - `<unnamed>` (ExportEncryptionTest): n/a
  - `<unnamed>` (TruncatedContainerIsRejected): n/a

#### `TEST_F(ExportEncryptionTest, WrongMagicIsRejected)`
- Source: `tests/exporters/test_export_encryption.cpp`:236
- Brief: n/a
- Parameters:
  - `<unnamed>` (ExportEncryptionTest): n/a
  - `<unnamed>` (WrongMagicIsRejected): n/a

#### `TEST_F(ExportPolicyEnforcementTest, IncrementalExporter_PolicyEngineDenies_ThrowsExporterException)`
- Source: `tests/exporters/test_export_encryption.cpp`:1205
- Brief: n/a
- Parameters:
  - `<unnamed>` (ExportPolicyEnforcementTest): n/a
  - `<unnamed>` (IncrementalExporter_PolicyEngineDenies_ThrowsExporterException): n/a

#### `TEST_F(ExportPolicyEnforcementTest, MultiCollection_AnyRestrictedDeniesExport)`
- Source: `tests/exporters/test_export_encryption.cpp`:1227
- Brief: n/a
- Parameters:
  - `<unnamed>` (ExportPolicyEnforcementTest): n/a
  - `<unnamed>` (MultiCollection_AnyRestrictedDeniesExport): n/a

#### `TEST_F(ExportPolicyEnforcementTest, NoPolicyEngine_ExportProceeds)`
- Source: `tests/exporters/test_export_encryption.cpp`:1021
- Brief: n/a
- Parameters:
  - `<unnamed>` (ExportPolicyEnforcementTest): n/a
  - `<unnamed>` (NoPolicyEngine_ExportProceeds): n/a

#### `TEST_F(ExportPolicyEnforcementTest, PolicyEngineDenies_AuditEventHasMediumSeverity)`
- Source: `tests/exporters/test_export_encryption.cpp`:1275
- Brief: n/a
- Parameters:
  - `<unnamed>` (ExportPolicyEnforcementTest): n/a
  - `<unnamed>` (PolicyEngineDenies_AuditEventHasMediumSeverity): n/a

#### `TEST_F(ExportPolicyEnforcementTest, PolicyEngineDenies_AuditLogReceivesExportDeniedEvent)`
- Source: `tests/exporters/test_export_encryption.cpp`:1099
- Brief: n/a
- Parameters:
  - `<unnamed>` (ExportPolicyEnforcementTest): n/a
  - `<unnamed>` (PolicyEngineDenies_AuditLogReceivesExportDeniedEvent): n/a

#### `TEST_F(ExportPolicyEnforcementTest, PolicyEngineDenies_NoAuditLogger_NoCrash)`
- Source: `tests/exporters/test_export_encryption.cpp`:1252
- Brief: n/a
- Parameters:
  - `<unnamed>` (ExportPolicyEnforcementTest): n/a
  - `<unnamed>` (PolicyEngineDenies_NoAuditLogger_NoCrash): n/a

#### `TEST_F(ExportPolicyEnforcementTest, PolicyEngineDenies_ThrowsExporterException)`
- Source: `tests/exporters/test_export_encryption.cpp`:1064
- Brief: n/a
- Parameters:
  - `<unnamed>` (ExportPolicyEnforcementTest): n/a
  - `<unnamed>` (PolicyEngineDenies_ThrowsExporterException): n/a

#### `TEST_F(ExportPolicyEnforcementTest, PolicyEnginePermits_AuditLogReceivesBulkExportEvent)`
- Source: `tests/exporters/test_export_encryption.cpp`:1144
- Brief: n/a
- Parameters:
  - `<unnamed>` (ExportPolicyEnforcementTest): n/a
  - `<unnamed>` (PolicyEnginePermits_AuditLogReceivesBulkExportEvent): n/a

#### `TEST_F(ExportPolicyEnforcementTest, PolicyEnginePermits_ExportProceeds)`
- Source: `tests/exporters/test_export_encryption.cpp`:1040
- Brief: n/a
- Parameters:
  - `<unnamed>` (ExportPolicyEnforcementTest): n/a
  - `<unnamed>` (PolicyEnginePermits_ExportProceeds): n/a

#### `TEST_F(ExportPolicyEnforcementTest, StreamingExporter_PolicyEngineDenies_ThrowsExporterException)`
- Source: `tests/exporters/test_export_encryption.cpp`:1184
- Brief: n/a
- Parameters:
  - `<unnamed>` (ExportPolicyEnforcementTest): n/a
  - `<unnamed>` (StreamingExporter_PolicyEngineDenies_ThrowsExporterException): n/a

#### `std::string decodeBase64ForAuditTest(const std::string &input)`
- Source: `tests/exporters/test_export_encryption.cpp`:928
- Brief: n/a
- Parameters:
  - `input` (const std::string &): n/a

#### `std::shared_ptr< themis::utils::AuditLogger > makeAuditLogger(const std::string &log_path)`
- Source: `tests/exporters/test_export_encryption.cpp`:911
- Brief: n/a
- Parameters:
  - `log_path` (const std::string &): n/a

#### `std::shared_ptr< MockKeyProvider > makeProvider(const std::string &kek_id)`
- Source: `tests/exporters/test_export_encryption.cpp`:40
- Brief: n/a
- Parameters:
  - `kek_id` (const std::string &): n/a

#### `std::vector< json > readDecodedAuditPayloads(const std::string &path)`
- Source: `tests/exporters/test_export_encryption.cpp`:957
- Brief: n/a
- Parameters:
  - `path` (const std::string &): n/a

### test_export_format_registry.cpp

#### `TEST_F(ExportFormatRegistryTest, BuiltinTemplateAlpacaCreatesCorrectExporter)`
- Source: `tests/exporters/test_export_format_registry.cpp`:154
- Brief: n/a
- Parameters:
  - `<unnamed>` (ExportFormatRegistryTest): n/a
  - `<unnamed>` (BuiltinTemplateAlpacaCreatesCorrectExporter): n/a

#### `TEST_F(ExportFormatRegistryTest, BuiltinTemplateChatMLCreatesCorrectExporter)`
- Source: `tests/exporters/test_export_format_registry.cpp`:173
- Brief: n/a
- Parameters:
  - `<unnamed>` (ExportFormatRegistryTest): n/a
  - `<unnamed>` (BuiltinTemplateChatMLCreatesCorrectExporter): n/a

#### `TEST_F(ExportFormatRegistryTest, BuiltinTemplateOpenAIFTCreatesCorrectExporter)`
- Source: `tests/exporters/test_export_format_registry.cpp`:182
- Brief: n/a
- Parameters:
  - `<unnamed>` (ExportFormatRegistryTest): n/a
  - `<unnamed>` (BuiltinTemplateOpenAIFTCreatesCorrectExporter): n/a

#### `TEST_F(ExportFormatRegistryTest, BuiltinTemplateShareGPTCreatesCorrectExporter)`
- Source: `tests/exporters/test_export_format_registry.cpp`:164
- Brief: n/a
- Parameters:
  - `<unnamed>` (ExportFormatRegistryTest): n/a
  - `<unnamed>` (BuiltinTemplateShareGPTCreatesCorrectExporter): n/a

#### `TEST_F(ExportFormatRegistryTest, ClearRemovesAllFormats)`
- Source: `tests/exporters/test_export_format_registry.cpp`:145
- Brief: n/a
- Parameters:
  - `<unnamed>` (ExportFormatRegistryTest): n/a
  - `<unnamed>` (ClearRemovesAllFormats): n/a

#### `TEST_F(ExportFormatRegistryTest, CreateExporterArrowFileReturnsNonNull)`
- Source: `tests/exporters/test_export_format_registry.cpp`:68
- Brief: n/a
- Parameters:
  - `<unnamed>` (ExportFormatRegistryTest): n/a
  - `<unnamed>` (CreateExporterArrowFileReturnsNonNull): n/a

#### `TEST_F(ExportFormatRegistryTest, CreateExporterArrowStreamReturnsNonNull)`
- Source: `tests/exporters/test_export_format_registry.cpp`:74
- Brief: n/a
- Parameters:
  - `<unnamed>` (ExportFormatRegistryTest): n/a
  - `<unnamed>` (CreateExporterArrowStreamReturnsNonNull): n/a

#### `TEST_F(ExportFormatRegistryTest, CreateExporterHuggingFaceReturnsNonNull)`
- Source: `tests/exporters/test_export_format_registry.cpp`:80
- Brief: n/a
- Parameters:
  - `<unnamed>` (ExportFormatRegistryTest): n/a
  - `<unnamed>` (CreateExporterHuggingFaceReturnsNonNull): n/a

#### `TEST_F(ExportFormatRegistryTest, CreateExporterIncrementalReturnsNonNull)`
- Source: `tests/exporters/test_export_format_registry.cpp`:93
- Brief: n/a
- Parameters:
  - `<unnamed>` (ExportFormatRegistryTest): n/a
  - `<unnamed>` (CreateExporterIncrementalReturnsNonNull): n/a

#### `TEST_F(ExportFormatRegistryTest, CreateExporterJsonlReturnsNonNull)`
- Source: `tests/exporters/test_export_format_registry.cpp`:54
- Brief: n/a
- Parameters:
  - `<unnamed>` (ExportFormatRegistryTest): n/a
  - `<unnamed>` (CreateExporterJsonlReturnsNonNull): n/a

#### `TEST_F(ExportFormatRegistryTest, CreateExporterParquetReturnsNonNull)`
- Source: `tests/exporters/test_export_format_registry.cpp`:61
- Brief: n/a
- Parameters:
  - `<unnamed>` (ExportFormatRegistryTest): n/a
  - `<unnamed>` (CreateExporterParquetReturnsNonNull): n/a

#### `TEST_F(ExportFormatRegistryTest, CreateExporterStreamingReturnsNonNull)`
- Source: `tests/exporters/test_export_format_registry.cpp`:87
- Brief: n/a
- Parameters:
  - `<unnamed>` (ExportFormatRegistryTest): n/a
  - `<unnamed>` (CreateExporterStreamingReturnsNonNull): n/a

#### `TEST_F(ExportFormatRegistryTest, CreateExporterUnknownFormatThrows)`
- Source: `tests/exporters/test_export_format_registry.cpp`:99
- Brief: n/a
- Parameters:
  - `<unnamed>` (ExportFormatRegistryTest): n/a
  - `<unnamed>` (CreateExporterUnknownFormatThrows): n/a

#### `TEST_F(ExportFormatRegistryTest, HasFormatReturnsFalseBeforeRegister)`
- Source: `tests/exporters/test_export_format_registry.cpp`:109
- Brief: n/a
- Parameters:
  - `<unnamed>` (ExportFormatRegistryTest): n/a
  - `<unnamed>` (HasFormatReturnsFalseBeforeRegister): n/a

#### `TEST_F(ExportFormatRegistryTest, HasFormatReturnsTrueAfterRegister)`
- Source: `tests/exporters/test_export_format_registry.cpp`:113
- Brief: n/a
- Parameters:
  - `<unnamed>` (ExportFormatRegistryTest): n/a
  - `<unnamed>` (HasFormatReturnsTrueAfterRegister): n/a

#### `TEST_F(ExportFormatRegistryTest, LoadTemplatesFromConfigRegistersFormats)`
- Source: `tests/exporters/test_export_format_registry.cpp`:281
- Brief: n/a
- Parameters:
  - `<unnamed>` (ExportFormatRegistryTest): n/a
  - `<unnamed>` (LoadTemplatesFromConfigRegistersFormats): n/a

#### `TEST_F(ExportFormatRegistryTest, LoadTemplatesFromConfigThrowsOnMissingFile)`
- Source: `tests/exporters/test_export_format_registry.cpp`:294
- Brief: n/a
- Parameters:
  - `<unnamed>` (ExportFormatRegistryTest): n/a
  - `<unnamed>` (LoadTemplatesFromConfigThrowsOnMissingFile): n/a

#### `TEST_F(ExportFormatRegistryTest, LoadTemplatesFromJsonAppliesFieldMapping)`
- Source: `tests/exporters/test_export_format_registry.cpp`:209
- Brief: n/a
- Parameters:
  - `<unnamed>` (ExportFormatRegistryTest): n/a
  - `<unnamed>` (LoadTemplatesFromJsonAppliesFieldMapping): n/a

#### `TEST_F(ExportFormatRegistryTest, LoadTemplatesFromJsonCreatesExporterWithCorrectType)`
- Source: `tests/exporters/test_export_format_registry.cpp`:199
- Brief: n/a
- Parameters:
  - `<unnamed>` (ExportFormatRegistryTest): n/a
  - `<unnamed>` (LoadTemplatesFromJsonCreatesExporterWithCorrectType): n/a

#### `TEST_F(ExportFormatRegistryTest, LoadTemplatesFromJsonIsAtomicOnPartialFailure)`
- Source: `tests/exporters/test_export_format_registry.cpp`:267
- Brief: n/a
- Parameters:
  - `<unnamed>` (ExportFormatRegistryTest): n/a
  - `<unnamed>` (LoadTemplatesFromJsonIsAtomicOnPartialFailure): n/a

#### `TEST_F(ExportFormatRegistryTest, LoadTemplatesFromJsonMultipleEntries)`
- Source: `tests/exporters/test_export_format_registry.cpp`:220
- Brief: n/a
- Parameters:
  - `<unnamed>` (ExportFormatRegistryTest): n/a
  - `<unnamed>` (LoadTemplatesFromJsonMultipleEntries): n/a

#### `TEST_F(ExportFormatRegistryTest, LoadTemplatesFromJsonRegistersNewFormat)`
- Source: `tests/exporters/test_export_format_registry.cpp`:193
- Brief: n/a
- Parameters:
  - `<unnamed>` (ExportFormatRegistryTest): n/a
  - `<unnamed>` (LoadTemplatesFromJsonRegistersNewFormat): n/a

#### `TEST_F(ExportFormatRegistryTest, LoadTemplatesFromJsonThrowsOnInvalidJson)`
- Source: `tests/exporters/test_export_format_registry.cpp`:260
- Brief: n/a
- Parameters:
  - `<unnamed>` (ExportFormatRegistryTest): n/a
  - `<unnamed>` (LoadTemplatesFromJsonThrowsOnInvalidJson): n/a

#### `TEST_F(ExportFormatRegistryTest, LoadTemplatesFromJsonThrowsOnMissingFormatKey)`
- Source: `tests/exporters/test_export_format_registry.cpp`:236
- Brief: n/a
- Parameters:
  - `<unnamed>` (ExportFormatRegistryTest): n/a
  - `<unnamed>` (LoadTemplatesFromJsonThrowsOnMissingFormatKey): n/a

#### `TEST_F(ExportFormatRegistryTest, LoadTemplatesFromJsonThrowsOnMissingTemplateType)`
- Source: `tests/exporters/test_export_format_registry.cpp`:244
- Brief: n/a
- Parameters:
  - `<unnamed>` (ExportFormatRegistryTest): n/a
  - `<unnamed>` (LoadTemplatesFromJsonThrowsOnMissingTemplateType): n/a

#### `TEST_F(ExportFormatRegistryTest, LoadTemplatesFromJsonThrowsOnMissingTemplatesArray)`
- Source: `tests/exporters/test_export_format_registry.cpp`:228
- Brief: n/a
- Parameters:
  - `<unnamed>` (ExportFormatRegistryTest): n/a
  - `<unnamed>` (LoadTemplatesFromJsonThrowsOnMissingTemplatesArray): n/a

#### `TEST_F(ExportFormatRegistryTest, LoadTemplatesFromJsonThrowsOnUnknownTemplateType)`
- Source: `tests/exporters/test_export_format_registry.cpp`:252
- Brief: n/a
- Parameters:
  - `<unnamed>` (ExportFormatRegistryTest): n/a
  - `<unnamed>` (LoadTemplatesFromJsonThrowsOnUnknownTemplateType): n/a

#### `TEST_F(ExportFormatRegistryTest, RegisterBuiltinsIsIdempotent)`
- Source: `tests/exporters/test_export_format_registry.cpp`:44
- Brief: n/a
- Parameters:
  - `<unnamed>` (ExportFormatRegistryTest): n/a
  - `<unnamed>` (RegisterBuiltinsIsIdempotent): n/a

#### `TEST_F(ExportFormatRegistryTest, RegisterBuiltinsPopulatesExpectedFormats)`
- Source: `tests/exporters/test_export_format_registry.cpp`:30
- Brief: n/a
- Parameters:
  - `<unnamed>` (ExportFormatRegistryTest): n/a
  - `<unnamed>` (RegisterBuiltinsPopulatesExpectedFormats): n/a

#### `TEST_F(ExportFormatRegistryTest, RegisterFormatOverridesExisting)`
- Source: `tests/exporters/test_export_format_registry.cpp`:123
- Brief: n/a
- Parameters:
  - `<unnamed>` (ExportFormatRegistryTest): n/a
  - `<unnamed>` (RegisterFormatOverridesExisting): n/a

#### `TEST_F(ExportFormatRegistryTest, RegisteredFormatsIsSorted)`
- Source: `tests/exporters/test_export_format_registry.cpp`:137
- Brief: n/a
- Parameters:
  - `<unnamed>` (ExportFormatRegistryTest): n/a
  - `<unnamed>` (RegisteredFormatsIsSorted): n/a

### test_exporters_contract_hardening_focused.cpp

#### `TEST(ExportersContractHardeningEXCH01, NullCellRenderedEmpty)`
- Source: `tests/exporters/test_exporters_contract_hardening_focused.cpp`:188
- Brief: n/a
- Parameters:
  - `<unnamed>` (ExportersContractHardeningEXCH01): n/a
  - `<unnamed>` (NullCellRenderedEmpty): n/a

#### `TEST(ExportersContractHardeningEXCH02, DelimiterInFieldQuoted)`
- Source: `tests/exporters/test_exporters_contract_hardening_focused.cpp`:200
- Brief: n/a
- Parameters:
  - `<unnamed>` (ExportersContractHardeningEXCH02): n/a
  - `<unnamed>` (DelimiterInFieldQuoted): n/a

#### `TEST(ExportersContractHardeningEXCH03, HeaderAlwaysFirst)`
- Source: `tests/exporters/test_exporters_contract_hardening_focused.cpp`:212
- Brief: n/a
- Parameters:
  - `<unnamed>` (ExportersContractHardeningEXCH03): n/a
  - `<unnamed>` (HeaderAlwaysFirst): n/a

#### `TEST(ExportersContractHardeningEXCH04, LineSeparatorContract)`
- Source: `tests/exporters/test_exporters_contract_hardening_focused.cpp`:235
- Brief: n/a
- Parameters:
  - `<unnamed>` (ExportersContractHardeningEXCH04): n/a
  - `<unnamed>` (LineSeparatorContract): n/a

#### `TEST(ExportersContractHardeningEXCH05, ParquetSchemaPreserved)`
- Source: `tests/exporters/test_exporters_contract_hardening_focused.cpp`:253
- Brief: n/a
- Parameters:
  - `<unnamed>` (ExportersContractHardeningEXCH05): n/a
  - `<unnamed>` (ParquetSchemaPreserved): n/a

#### `TEST(ExportersContractHardeningEXCH06, ParquetNullBitmapCorrect)`
- Source: `tests/exporters/test_exporters_contract_hardening_focused.cpp`:267
- Brief: n/a
- Parameters:
  - `<unnamed>` (ExportersContractHardeningEXCH06): n/a
  - `<unnamed>` (ParquetNullBitmapCorrect): n/a

#### `TEST(ExportersContractHardeningEXCH07, RepeatedExportDeterministic)`
- Source: `tests/exporters/test_exporters_contract_hardening_focused.cpp`:282
- Brief: n/a
- Parameters:
  - `<unnamed>` (ExportersContractHardeningEXCH07): n/a
  - `<unnamed>` (RepeatedExportDeterministic): n/a

#### `TEST(ExportersContractHardeningEXCH08, OutputColumnCountMatchesSchema)`
- Source: `tests/exporters/test_exporters_contract_hardening_focused.cpp`:297
- Brief: n/a
- Parameters:
  - `<unnamed>` (ExportersContractHardeningEXCH08): n/a
  - `<unnamed>` (OutputColumnCountMatchesSchema): n/a

#### `TEST(ExportersContractHardeningEXCH09, ChunksDeliveredInOrder)`
- Source: `tests/exporters/test_exporters_contract_hardening_focused.cpp`:314
- Brief: n/a
- Parameters:
  - `<unnamed>` (ExportersContractHardeningEXCH09): n/a
  - `<unnamed>` (ChunksDeliveredInOrder): n/a

#### `TEST(ExportersContractHardeningEXCH10, PartialExportSurfacesStreamInterrupted)`
- Source: `tests/exporters/test_exporters_contract_hardening_focused.cpp`:328
- Brief: n/a
- Parameters:
  - `<unnamed>` (ExportersContractHardeningEXCH10): n/a
  - `<unnamed>` (PartialExportSurfacesStreamInterrupted): n/a

#### `TEST(ExportersContractHardeningEXCH11, ExportResumesFromLastAckedChunk)`
- Source: `tests/exporters/test_exporters_contract_hardening_focused.cpp`:347
- Brief: n/a
- Parameters:
  - `<unnamed>` (ExportersContractHardeningEXCH11): n/a
  - `<unnamed>` (ExportResumesFromLastAckedChunk): n/a

#### `TEST(ExportersContractHardeningEXCH12, ChunkSequenceStartsAtZero)`
- Source: `tests/exporters/test_exporters_contract_hardening_focused.cpp`:367
- Brief: n/a
- Parameters:
  - `<unnamed>` (ExportersContractHardeningEXCH12): n/a
  - `<unnamed>` (ChunkSequenceStartsAtZero): n/a

#### `TEST(ExportersContractHardeningEXCH13, UnsupportedFormatErrorBeforeOutput)`
- Source: `tests/exporters/test_exporters_contract_hardening_focused.cpp`:380
- Brief: n/a
- Parameters:
  - `<unnamed>` (ExportersContractHardeningEXCH13): n/a
  - `<unnamed>` (UnsupportedFormatErrorBeforeOutput): n/a

#### `TEST(ExportersContractHardeningEXCH14, WriteFailureSurfacesError)`
- Source: `tests/exporters/test_exporters_contract_hardening_focused.cpp`:400
- Brief: n/a
- Parameters:
  - `<unnamed>` (ExportersContractHardeningEXCH14): n/a
  - `<unnamed>` (WriteFailureSurfacesError): n/a

#### `TEST(ExportersContractHardeningEXCH15, QuotaExceededSurfaced)`
- Source: `tests/exporters/test_exporters_contract_hardening_focused.cpp`:414
- Brief: n/a
- Parameters:
  - `<unnamed>` (ExportersContractHardeningEXCH15): n/a
  - `<unnamed>` (QuotaExceededSurfaced): n/a

#### `TEST(ExportersContractHardeningEXCH16, ResumabilityContract)`
- Source: `tests/exporters/test_exporters_contract_hardening_focused.cpp`:431
- Brief: n/a
- Parameters:
  - `<unnamed>` (ExportersContractHardeningEXCH16): n/a
  - `<unnamed>` (ResumabilityContract): n/a

### test_exporters_highcardinality_stress.cpp

#### `TEST(ConcurrentTraceExportStress, MultiThreadedExport)`
- Source: `tests/exporters/test_exporters_highcardinality_stress.cpp`:134
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConcurrentTraceExportStress): n/a
  - `<unnamed>` (MultiThreadedExport): n/a
- Details: TestConcurrentTraceExportStress Runs 8 concurrent trace export threads each exporting 10 000 traces and verifies total count and no errors.

#### `TEST(ExporterBackpressureStress, MultiThreadedEnqueueDequeue)`
- Source: `tests/exporters/test_exporters_highcardinality_stress.cpp`:168
- Brief: n/a
- Parameters:
  - `<unnamed>` (ExporterBackpressureStress): n/a
  - `<unnamed>` (MultiThreadedEnqueueDequeue): n/a
- Details: TestExporterBackpressureStress Runs 8 concurrent producers against a bounded export queue and verifies no uncaught exceptions occur (overflow is tolerated by design).

#### `TEST(HighCardinalityMetricExport, ConcurrentExport)`
- Source: `tests/exporters/test_exporters_highcardinality_stress.cpp`:99
- Brief: n/a
- Parameters:
  - `<unnamed>` (HighCardinalityMetricExport): n/a
  - `<unnamed>` (ConcurrentExport): n/a
- Details: TestHighCardinalityMetricExport Exports 1 000 000 metrics across 8 concurrent threads and verifies that all 1 000 000 exports complete without exceptions.

### test_format_template.cpp

#### `TEST(ValidateTemplateExporterTest, ExporterDelegatesNoneType)`
- Source: `tests/exporters/test_format_template.cpp`:703
- Brief: n/a
- Parameters:
  - `<unnamed>` (ValidateTemplateExporterTest): n/a
  - `<unnamed>` (ExporterDelegatesNoneType): n/a

#### `TEST(ValidateTemplateExporterTest, ExporterReturnsCorrectMissingFields)`
- Source: `tests/exporters/test_format_template.cpp`:715
- Brief: n/a
- Parameters:
  - `<unnamed>` (ValidateTemplateExporterTest): n/a
  - `<unnamed>` (ExporterReturnsCorrectMissingFields): n/a

#### `TEST(ValidateTemplateExporterTest, ExporterUsesConfigFieldMapping)`
- Source: `tests/exporters/test_format_template.cpp`:739
- Brief: n/a
- Parameters:
  - `<unnamed>` (ValidateTemplateExporterTest): n/a
  - `<unnamed>` (ExporterUsesConfigFieldMapping): n/a

#### `TEST_F(FormatTemplateExporterTest, AlpacaViaExporter)`
- Source: `tests/exporters/test_format_template.cpp`:372
- Brief: n/a
- Parameters:
  - `<unnamed>` (FormatTemplateExporterTest): n/a
  - `<unnamed>` (AlpacaViaExporter): n/a

#### `TEST_F(FormatTemplateExporterTest, ChatMLViaExporter)`
- Source: `tests/exporters/test_format_template.cpp`:430
- Brief: n/a
- Parameters:
  - `<unnamed>` (FormatTemplateExporterTest): n/a
  - `<unnamed>` (ChatMLViaExporter): n/a

#### `TEST_F(FormatTemplateExporterTest, MissingRequiredFieldsSkipsEntity)`
- Source: `tests/exporters/test_format_template.cpp`:508
- Brief: n/a
- Parameters:
  - `<unnamed>` (FormatTemplateExporterTest): n/a
  - `<unnamed>` (MissingRequiredFieldsSkipsEntity): n/a

#### `TEST_F(FormatTemplateExporterTest, OpenAIViaExporter)`
- Source: `tests/exporters/test_format_template.cpp`:451
- Brief: n/a
- Parameters:
  - `<unnamed>` (FormatTemplateExporterTest): n/a
  - `<unnamed>` (OpenAIViaExporter): n/a

#### `TEST_F(FormatTemplateExporterTest, SetConfigRecreatesTemplate)`
- Source: `tests/exporters/test_format_template.cpp`:477
- Brief: n/a
- Parameters:
  - `<unnamed>` (FormatTemplateExporterTest): n/a
  - `<unnamed>` (SetConfigRecreatesTemplate): n/a

#### `TEST_F(FormatTemplateExporterTest, ShareGPTViaExporter)`
- Source: `tests/exporters/test_format_template.cpp`:408
- Brief: n/a
- Parameters:
  - `<unnamed>` (FormatTemplateExporterTest): n/a
  - `<unnamed>` (ShareGPTViaExporter): n/a

#### `TEST_F(FormatTemplateTest, AlpacaRenderEmptyInputStillIncludesKey)`
- Source: `tests/exporters/test_format_template.cpp`:87
- Brief: n/a
- Parameters:
  - `<unnamed>` (FormatTemplateTest): n/a
  - `<unnamed>` (AlpacaRenderEmptyInputStillIncludesKey): n/a

#### `TEST_F(FormatTemplateTest, AlpacaRenderMissingRequiredReturnsEmpty)`
- Source: `tests/exporters/test_format_template.cpp`:135
- Brief: n/a
- Parameters:
  - `<unnamed>` (FormatTemplateTest): n/a
  - `<unnamed>` (AlpacaRenderMissingRequiredReturnsEmpty): n/a

#### `TEST_F(FormatTemplateTest, AlpacaRenderWithInput)`
- Source: `tests/exporters/test_format_template.cpp`:68
- Brief: n/a
- Parameters:
  - `<unnamed>` (FormatTemplateTest): n/a
  - `<unnamed>` (AlpacaRenderWithInput): n/a

#### `TEST_F(FormatTemplateTest, AlpacaTemplateType)`
- Source: `tests/exporters/test_format_template.cpp`:62
- Brief: n/a
- Parameters:
  - `<unnamed>` (FormatTemplateTest): n/a
  - `<unnamed>` (AlpacaTemplateType): n/a

#### `TEST_F(FormatTemplateTest, AlpacaValidateFieldsMissingInstruction)`
- Source: `tests/exporters/test_format_template.cpp`:108
- Brief: n/a
- Parameters:
  - `<unnamed>` (FormatTemplateTest): n/a
  - `<unnamed>` (AlpacaValidateFieldsMissingInstruction): n/a

#### `TEST_F(FormatTemplateTest, AlpacaValidateFieldsOk)`
- Source: `tests/exporters/test_format_template.cpp`:123
- Brief: n/a
- Parameters:
  - `<unnamed>` (FormatTemplateTest): n/a
  - `<unnamed>` (AlpacaValidateFieldsOk): n/a

#### `TEST_F(FormatTemplateTest, ChatMLRenderNoSystem)`
- Source: `tests/exporters/test_format_template.cpp`:223
- Brief: n/a
- Parameters:
  - `<unnamed>` (FormatTemplateTest): n/a
  - `<unnamed>` (ChatMLRenderNoSystem): n/a

#### `TEST_F(FormatTemplateTest, ChatMLRenderWithSystem)`
- Source: `tests/exporters/test_format_template.cpp`:245
- Brief: n/a
- Parameters:
  - `<unnamed>` (FormatTemplateTest): n/a
  - `<unnamed>` (ChatMLRenderWithSystem): n/a

#### `TEST_F(FormatTemplateTest, ChatMLTemplateType)`
- Source: `tests/exporters/test_format_template.cpp`:217
- Brief: n/a
- Parameters:
  - `<unnamed>` (FormatTemplateTest): n/a
  - `<unnamed>` (ChatMLTemplateType): n/a

#### `TEST_F(FormatTemplateTest, ChatMLValidateFieldsOk)`
- Source: `tests/exporters/test_format_template.cpp`:267
- Brief: n/a
- Parameters:
  - `<unnamed>` (FormatTemplateTest): n/a
  - `<unnamed>` (ChatMLValidateFieldsOk): n/a

#### `TEST_F(FormatTemplateTest, FactoryNoneReturnsNull)`
- Source: `tests/exporters/test_format_template.cpp`:315
- Brief: n/a
- Parameters:
  - `<unnamed>` (FormatTemplateTest): n/a
  - `<unnamed>` (FactoryNoneReturnsNull): n/a

#### `TEST_F(FormatTemplateTest, OpenAIFineTuningRenderWithSystem)`
- Source: `tests/exporters/test_format_template.cpp`:289
- Brief: n/a
- Parameters:
  - `<unnamed>` (FormatTemplateTest): n/a
  - `<unnamed>` (OpenAIFineTuningRenderWithSystem): n/a

#### `TEST_F(FormatTemplateTest, OpenAIFineTuningType)`
- Source: `tests/exporters/test_format_template.cpp`:283
- Brief: n/a
- Parameters:
  - `<unnamed>` (FormatTemplateTest): n/a
  - `<unnamed>` (OpenAIFineTuningType): n/a

#### `TEST_F(FormatTemplateTest, ShareGPTRenderNoSystem)`
- Source: `tests/exporters/test_format_template.cpp`:156
- Brief: n/a
- Parameters:
  - `<unnamed>` (FormatTemplateTest): n/a
  - `<unnamed>` (ShareGPTRenderNoSystem): n/a

#### `TEST_F(FormatTemplateTest, ShareGPTRenderWithSystem)`
- Source: `tests/exporters/test_format_template.cpp`:178
- Brief: n/a
- Parameters:
  - `<unnamed>` (FormatTemplateTest): n/a
  - `<unnamed>` (ShareGPTRenderWithSystem): n/a

#### `TEST_F(FormatTemplateTest, ShareGPTTemplateType)`
- Source: `tests/exporters/test_format_template.cpp`:150
- Brief: n/a
- Parameters:
  - `<unnamed>` (FormatTemplateTest): n/a
  - `<unnamed>` (ShareGPTTemplateType): n/a

#### `TEST_F(FormatTemplateTest, ShareGPTValidateFieldsMissingUser)`
- Source: `tests/exporters/test_format_template.cpp`:200
- Brief: n/a
- Parameters:
  - `<unnamed>` (FormatTemplateTest): n/a
  - `<unnamed>` (ShareGPTValidateFieldsMissingUser): n/a

#### `TEST_F(ValidateTemplateTest, AlpacaAllEntitiesValid)`
- Source: `tests/exporters/test_format_template.cpp`:586
- Brief: n/a
- Parameters:
  - `<unnamed>` (ValidateTemplateTest): n/a
  - `<unnamed>` (AlpacaAllEntitiesValid): n/a

#### `TEST_F(ValidateTemplateTest, AlpacaBothRequiredFieldsMissing)`
- Source: `tests/exporters/test_format_template.cpp`:614
- Brief: n/a
- Parameters:
  - `<unnamed>` (ValidateTemplateTest): n/a
  - `<unnamed>` (AlpacaBothRequiredFieldsMissing): n/a

#### `TEST_F(ValidateTemplateTest, AlpacaMissingInstructionField)`
- Source: `tests/exporters/test_format_template.cpp`:600
- Brief: n/a
- Parameters:
  - `<unnamed>` (ValidateTemplateTest): n/a
  - `<unnamed>` (AlpacaMissingInstructionField): n/a

#### `TEST_F(ValidateTemplateTest, ChatMLAllEntitiesValid)`
- Source: `tests/exporters/test_format_template.cpp`:656
- Brief: n/a
- Parameters:
  - `<unnamed>` (ValidateTemplateTest): n/a
  - `<unnamed>` (ChatMLAllEntitiesValid): n/a

#### `TEST_F(ValidateTemplateTest, CustomFieldMappingRespected)`
- Source: `tests/exporters/test_format_template.cpp`:679
- Brief: n/a
- Parameters:
  - `<unnamed>` (ValidateTemplateTest): n/a
  - `<unnamed>` (CustomFieldMappingRespected): n/a

#### `TEST_F(ValidateTemplateTest, EmptySampleIsValid)`
- Source: `tests/exporters/test_format_template.cpp`:577
- Brief: n/a
- Parameters:
  - `<unnamed>` (ValidateTemplateTest): n/a
  - `<unnamed>` (EmptySampleIsValid): n/a

#### `TEST_F(ValidateTemplateTest, MissingFieldsAreDeduplicated)`
- Source: `tests/exporters/test_format_template.cpp`:630
- Brief: n/a
- Parameters:
  - `<unnamed>` (ValidateTemplateTest): n/a
  - `<unnamed>` (MissingFieldsAreDeduplicated): n/a

#### `TEST_F(ValidateTemplateTest, NoneTypeAlwaysValid)`
- Source: `tests/exporters/test_format_template.cpp`:567
- Brief: n/a
- Parameters:
  - `<unnamed>` (ValidateTemplateTest): n/a
  - `<unnamed>` (NoneTypeAlwaysValid): n/a

#### `TEST_F(ValidateTemplateTest, OpenAIFineTuningMissingAssistantField)`
- Source: `tests/exporters/test_format_template.cpp`:668
- Brief: n/a
- Parameters:
  - `<unnamed>` (ValidateTemplateTest): n/a
  - `<unnamed>` (OpenAIFineTuningMissingAssistantField): n/a

#### `TEST_F(ValidateTemplateTest, ShareGPTMissingUserField)`
- Source: `tests/exporters/test_format_template.cpp`:645
- Brief: n/a
- Parameters:
  - `<unnamed>` (ValidateTemplateTest): n/a
  - `<unnamed>` (ShareGPTMissingUserField): n/a

#### `BaseEntity makeFullEntity(const std::string &pk, const std::string &instruction, const std::string &input, const std::string &output, const std::string &user_msg, const std::string &assistant_resp, const std::string &system="")`
- Source: `tests/exporters/test_format_template.cpp`:25
- Brief: n/a
- Parameters:
  - `pk` (const std::string &): n/a
  - `instruction` (const std::string &): n/a
  - `input` (const std::string &): n/a
  - `output` (const std::string &): n/a
  - `user_msg` (const std::string &): n/a
  - `assistant_resp` (const std::string &): n/a
  - `system` (const std::string &): n/a

### test_huggingface_exporter.cpp

#### `TEST_F(HuggingFaceExporterTest, CreatesDatasetDirectory)`
- Source: `tests/exporters/test_huggingface_exporter.cpp`:74
- Brief: n/a
- Parameters:
  - `<unnamed>` (HuggingFaceExporterTest): n/a
  - `<unnamed>` (CreatesDatasetDirectory): n/a

#### `TEST_F(HuggingFaceExporterTest, CreatesDatasetInfoJson)`
- Source: `tests/exporters/test_huggingface_exporter.cpp`:95
- Brief: n/a
- Parameters:
  - `<unnamed>` (HuggingFaceExporterTest): n/a
  - `<unnamed>` (CreatesDatasetInfoJson): n/a

#### `TEST_F(HuggingFaceExporterTest, CustomSplitName)`
- Source: `tests/exporters/test_huggingface_exporter.cpp`:138
- Brief: n/a
- Parameters:
  - `<unnamed>` (HuggingFaceExporterTest): n/a
  - `<unnamed>` (CustomSplitName): n/a

#### `TEST_F(HuggingFaceExporterTest, DataFileContainsValidJsonl)`
- Source: `tests/exporters/test_huggingface_exporter.cpp`:179
- Brief: n/a
- Parameters:
  - `<unnamed>` (HuggingFaceExporterTest): n/a
  - `<unnamed>` (DataFileContainsValidJsonl): n/a

#### `TEST_F(HuggingFaceExporterTest, DatasetCardContainsSplitSection)`
- Source: `tests/exporters/test_huggingface_exporter.cpp`:306
- Brief: n/a
- Parameters:
  - `<unnamed>` (HuggingFaceExporterTest): n/a
  - `<unnamed>` (DatasetCardContainsSplitSection): n/a

#### `TEST_F(HuggingFaceExporterTest, DatasetCardSkippedWhenDisabled)`
- Source: `tests/exporters/test_huggingface_exporter.cpp`:277
- Brief: n/a
- Parameters:
  - `<unnamed>` (HuggingFaceExporterTest): n/a
  - `<unnamed>` (DatasetCardSkippedWhenDisabled): n/a

#### `TEST_F(HuggingFaceExporterTest, DatasetCardUsesCustomTemplate)`
- Source: `tests/exporters/test_huggingface_exporter.cpp`:290
- Brief: n/a
- Parameters:
  - `<unnamed>` (HuggingFaceExporterTest): n/a
  - `<unnamed>` (DatasetCardUsesCustomTemplate): n/a

#### `TEST_F(HuggingFaceExporterTest, DatasetCardYamlEscapesNewlineInTag)`
- Source: `tests/exporters/test_huggingface_exporter.cpp`:476
- Brief: n/a
- Parameters:
  - `<unnamed>` (HuggingFaceExporterTest): n/a
  - `<unnamed>` (DatasetCardYamlEscapesNewlineInTag): n/a

#### `TEST_F(HuggingFaceExporterTest, DatasetCardYamlEscapesSpecialCharsInLicense)`
- Source: `tests/exporters/test_huggingface_exporter.cpp`:451
- Brief: n/a
- Parameters:
  - `<unnamed>` (HuggingFaceExporterTest): n/a
  - `<unnamed>` (DatasetCardYamlEscapesSpecialCharsInLicense): n/a

#### `TEST_F(HuggingFaceExporterTest, DatasetInfoContainsMetadata)`
- Source: `tests/exporters/test_huggingface_exporter.cpp`:326
- Brief: n/a
- Parameters:
  - `<unnamed>` (HuggingFaceExporterTest): n/a
  - `<unnamed>` (DatasetInfoContainsMetadata): n/a

#### `TEST_F(HuggingFaceExporterTest, DatasetInfoContainsSplitInfo)`
- Source: `tests/exporters/test_huggingface_exporter.cpp`:121
- Brief: n/a
- Parameters:
  - `<unnamed>` (HuggingFaceExporterTest): n/a
  - `<unnamed>` (DatasetInfoContainsSplitInfo): n/a

#### `TEST_F(HuggingFaceExporterTest, DefaultSplitIsTrainWhenEmpty)`
- Source: `tests/exporters/test_huggingface_exporter.cpp`:157
- Brief: n/a
- Parameters:
  - `<unnamed>` (HuggingFaceExporterTest): n/a
  - `<unnamed>` (DefaultSplitIsTrainWhenEmpty): n/a

#### `TEST_F(HuggingFaceExporterTest, ExportStatsHaveDurationOnEmptyPath)`
- Source: `tests/exporters/test_huggingface_exporter.cpp`:526
- Brief: n/a
- Parameters:
  - `<unnamed>` (HuggingFaceExporterTest): n/a
  - `<unnamed>` (ExportStatsHaveDurationOnEmptyPath): n/a

#### `TEST_F(HuggingFaceExporterTest, ExportStatsToJson)`
- Source: `tests/exporters/test_huggingface_exporter.cpp`:403
- Brief: n/a
- Parameters:
  - `<unnamed>` (HuggingFaceExporterTest): n/a
  - `<unnamed>` (ExportStatsToJson): n/a

#### `TEST_F(HuggingFaceExporterTest, ExporterNameAndVersion)`
- Source: `tests/exporters/test_huggingface_exporter.cpp`:372
- Brief: n/a
- Parameters:
  - `<unnamed>` (HuggingFaceExporterTest): n/a
  - `<unnamed>` (ExporterNameAndVersion): n/a

#### `TEST_F(HuggingFaceExporterTest, GenerateDatasetInfoJsonStandalone)`
- Source: `tests/exporters/test_huggingface_exporter.cpp`:423
- Brief: n/a
- Parameters:
  - `<unnamed>` (HuggingFaceExporterTest): n/a
  - `<unnamed>` (GenerateDatasetInfoJsonStandalone): n/a

#### `TEST_F(HuggingFaceExporterTest, GeneratesDatasetCard)`
- Source: `tests/exporters/test_huggingface_exporter.cpp`:252
- Brief: n/a
- Parameters:
  - `<unnamed>` (HuggingFaceExporterTest): n/a
  - `<unnamed>` (GeneratesDatasetCard): n/a

#### `TEST_F(HuggingFaceExporterTest, GetAndSetConfig)`
- Source: `tests/exporters/test_huggingface_exporter.cpp`:378
- Brief: n/a
- Parameters:
  - `<unnamed>` (HuggingFaceExporterTest): n/a
  - `<unnamed>` (GetAndSetConfig): n/a

#### `TEST_F(HuggingFaceExporterTest, InfersFeatureTypesFromEntities)`
- Source: `tests/exporters/test_huggingface_exporter.cpp`:206
- Brief: n/a
- Parameters:
  - `<unnamed>` (HuggingFaceExporterTest): n/a
  - `<unnamed>` (InfersFeatureTypesFromEntities): n/a

#### `TEST_F(HuggingFaceExporterTest, MetricsAttachedToStats)`
- Source: `tests/exporters/test_huggingface_exporter.cpp`:391
- Brief: n/a
- Parameters:
  - `<unnamed>` (HuggingFaceExporterTest): n/a
  - `<unnamed>` (MetricsAttachedToStats): n/a

#### `TEST_F(HuggingFaceExporterTest, ReturnsErrorForEmptyOutputPath)`
- Source: `tests/exporters/test_huggingface_exporter.cpp`:349
- Brief: n/a
- Parameters:
  - `<unnamed>` (HuggingFaceExporterTest): n/a
  - `<unnamed>` (ReturnsErrorForEmptyOutputPath): n/a

#### `TEST_F(HuggingFaceExporterTest, SetConfigClearsInferredFeatures)`
- Source: `tests/exporters/test_huggingface_exporter.cpp`:500
- Brief: n/a
- Parameters:
  - `<unnamed>` (HuggingFaceExporterTest): n/a
  - `<unnamed>` (SetConfigClearsInferredFeatures): n/a

#### `TEST_F(HuggingFaceExporterTest, SupportedFormats)`
- Source: `tests/exporters/test_huggingface_exporter.cpp`:365
- Brief: n/a
- Parameters:
  - `<unnamed>` (HuggingFaceExporterTest): n/a
  - `<unnamed>` (SupportedFormats): n/a

#### `TEST_F(HuggingFaceExporterTest, UsesExplicitFeaturesWhenProvided)`
- Source: `tests/exporters/test_huggingface_exporter.cpp`:227
- Brief: n/a
- Parameters:
  - `<unnamed>` (HuggingFaceExporterTest): n/a
  - `<unnamed>` (UsesExplicitFeaturesWhenProvided): n/a

### test_huggingface_hub_client.cpp

#### `TEST(HuggingFaceHubClientTest, AuditLogNullptrIsBackwardCompatible)`
- Source: `tests/exporters/test_huggingface_hub_client.cpp`:468
- Brief: n/a
- Parameters:
  - `<unnamed>` (HuggingFaceHubClientTest): n/a
  - `<unnamed>` (AuditLogNullptrIsBackwardCompatible): n/a

#### `TEST(HuggingFaceHubClientTest, AuditLogWrittenOnNoTokenError)`
- Source: `tests/exporters/test_huggingface_hub_client.cpp`:444
- Brief: n/a
- Parameters:
  - `<unnamed>` (HuggingFaceHubClientTest): n/a
  - `<unnamed>` (AuditLogWrittenOnNoTokenError): n/a

#### `TEST(HuggingFaceHubClientTest, AuditLogWrittenOnPolicyDenial)`
- Source: `tests/exporters/test_huggingface_hub_client.cpp`:411
- Brief: n/a
- Parameters:
  - `<unnamed>` (HuggingFaceHubClientTest): n/a
  - `<unnamed>` (AuditLogWrittenOnPolicyDenial): n/a

#### `TEST(HuggingFaceHubClientTest, DefaultCommitMessage)`
- Source: `tests/exporters/test_huggingface_hub_client.cpp`:288
- Brief: n/a
- Parameters:
  - `<unnamed>` (HuggingFaceHubClientTest): n/a
  - `<unnamed>` (DefaultCommitMessage): n/a

#### `TEST(HuggingFaceHubClientTest, DefaultConfigHasNoKekFields)`
- Source: `tests/exporters/test_huggingface_hub_client.cpp`:663
- Brief: n/a
- Parameters:
  - `<unnamed>` (HuggingFaceHubClientTest): n/a
  - `<unnamed>` (DefaultConfigHasNoKekFields): n/a

#### `TEST(HuggingFaceHubClientTest, DefaultConfigValues)`
- Source: `tests/exporters/test_huggingface_hub_client.cpp`:274
- Brief: n/a
- Parameters:
  - `<unnamed>` (HuggingFaceHubClientTest): n/a
  - `<unnamed>` (DefaultConfigValues): n/a

#### `TEST(HuggingFaceHubClientTest, EmptyRepoidReturnsError)`
- Source: `tests/exporters/test_huggingface_hub_client.cpp`:249
- Brief: n/a
- Parameters:
  - `<unnamed>` (HuggingFaceHubClientTest): n/a
  - `<unnamed>` (EmptyRepoidReturnsError): n/a

#### `TEST(HuggingFaceHubClientTest, ExporterMetrics_RecordRateLimitHit_IncrementsCounter)`
- Source: `tests/exporters/test_huggingface_hub_client.cpp`:840
- Brief: n/a
- Parameters:
  - `<unnamed>` (HuggingFaceHubClientTest): n/a
  - `<unnamed>` (ExporterMetrics_RecordRateLimitHit_IncrementsCounter): n/a

#### `TEST(HuggingFaceHubClientTest, ExporterMetrics_Reset_ClearsRateLimitHits)`
- Source: `tests/exporters/test_huggingface_hub_client.cpp`:851
- Brief: n/a
- Parameters:
  - `<unnamed>` (HuggingFaceHubClientTest): n/a
  - `<unnamed>` (ExporterMetrics_Reset_ClearsRateLimitHits): n/a

#### `TEST(HuggingFaceHubClientTest, ExporterMetrics_ToJson_ContainsRateLimitHitKey)`
- Source: `tests/exporters/test_huggingface_hub_client.cpp`:861
- Brief: n/a
- Parameters:
  - `<unnamed>` (HuggingFaceHubClientTest): n/a
  - `<unnamed>` (ExporterMetrics_ToJson_ContainsRateLimitHitKey): n/a

#### `TEST(HuggingFaceHubClientTest, HubUploadConfig_MetricsFieldRoundtrip)`
- Source: `tests/exporters/test_huggingface_hub_client.cpp`:871
- Brief: n/a
- Parameters:
  - `<unnamed>` (HuggingFaceHubClientTest): n/a
  - `<unnamed>` (HubUploadConfig_MetricsFieldRoundtrip): n/a

#### `TEST(HuggingFaceHubClientTest, KekTokenResolution_AuditLogWrittenOnKekError)`
- Source: `tests/exporters/test_huggingface_hub_client.cpp`:798
- Brief: n/a
- Parameters:
  - `<unnamed>` (HuggingFaceHubClientTest): n/a
  - `<unnamed>` (KekTokenResolution_AuditLogWrittenOnKekError): n/a

#### `TEST(HuggingFaceHubClientTest, KekTokenResolution_HappyPath)`
- Source: `tests/exporters/test_huggingface_hub_client.cpp`:669
- Brief: n/a
- Parameters:
  - `<unnamed>` (HuggingFaceHubClientTest): n/a
  - `<unnamed>` (KekTokenResolution_HappyPath): n/a

#### `TEST(HuggingFaceHubClientTest, KekTokenResolution_HfTokenTakesPriorityOverKek)`
- Source: `tests/exporters/test_huggingface_hub_client.cpp`:729
- Brief: n/a
- Parameters:
  - `<unnamed>` (HuggingFaceHubClientTest): n/a
  - `<unnamed>` (KekTokenResolution_HfTokenTakesPriorityOverKek): n/a

#### `TEST(HuggingFaceHubClientTest, KekTokenResolution_KeyNotFoundReturnsError)`
- Source: `tests/exporters/test_huggingface_hub_client.cpp`:775
- Brief: n/a
- Parameters:
  - `<unnamed>` (HuggingFaceHubClientTest): n/a
  - `<unnamed>` (KekTokenResolution_KeyNotFoundReturnsError): n/a

#### `TEST(HuggingFaceHubClientTest, KekTokenResolution_NullProviderReturnsError)`
- Source: `tests/exporters/test_huggingface_hub_client.cpp`:757
- Brief: n/a
- Parameters:
  - `<unnamed>` (HuggingFaceHubClientTest): n/a
  - `<unnamed>` (KekTokenResolution_NullProviderReturnsError): n/a

#### `TEST(HuggingFaceHubClientTest, KekTokenResolution_PriorityOverEnv)`
- Source: `tests/exporters/test_huggingface_hub_client.cpp`:701
- Brief: n/a
- Parameters:
  - `<unnamed>` (HuggingFaceHubClientTest): n/a
  - `<unnamed>` (KekTokenResolution_PriorityOverEnv): n/a

#### `TEST(HuggingFaceHubClientTest, MemoryShardSpecHoldsDataCorrectly)`
- Source: `tests/exporters/test_huggingface_hub_client.cpp`:825
- Brief: n/a
- Parameters:
  - `<unnamed>` (HuggingFaceHubClientTest): n/a
  - `<unnamed>` (MemoryShardSpecHoldsDataCorrectly): n/a

#### `TEST(HuggingFaceHubClientTest, NoTokenReturnsError)`
- Source: `tests/exporters/test_huggingface_hub_client.cpp`:232
- Brief: n/a
- Parameters:
  - `<unnamed>` (HuggingFaceHubClientTest): n/a
  - `<unnamed>` (NoTokenReturnsError): n/a

#### `TEST(HuggingFaceHubClientTest, NonexistentDatasetDirBehavesGracefully)`
- Source: `tests/exporters/test_huggingface_hub_client.cpp`:260
- Brief: n/a
- Parameters:
  - `<unnamed>` (HuggingFaceHubClientTest): n/a
  - `<unnamed>` (NonexistentDatasetDirBehavesGracefully): n/a

#### `TEST(HuggingFaceHubClientTest, PolicyEngineDeniedUploadReturnsFailure)`
- Source: `tests/exporters/test_huggingface_hub_client.cpp`:338
- Brief: n/a
- Parameters:
  - `<unnamed>` (HuggingFaceHubClientTest): n/a
  - `<unnamed>` (PolicyEngineDeniedUploadReturnsFailure): n/a

#### `TEST(HuggingFaceHubClientTest, PolicyEngineNullptrIsBackwardCompatible)`
- Source: `tests/exporters/test_huggingface_hub_client.cpp`:394
- Brief: n/a
- Parameters:
  - `<unnamed>` (HuggingFaceHubClientTest): n/a
  - `<unnamed>` (PolicyEngineNullptrIsBackwardCompatible): n/a

#### `TEST(HuggingFaceHubClientTest, PolicyEnginePermittedUploadProceedsToNetwork)`
- Source: `tests/exporters/test_huggingface_hub_client.cpp`:365
- Brief: n/a
- Parameters:
  - `<unnamed>` (HuggingFaceHubClientTest): n/a
  - `<unnamed>` (PolicyEnginePermittedUploadProceedsToNetwork): n/a

#### `TEST(HuggingFaceHubClientTest, ProgressCallbackNotCalledWhenNoToken)`
- Source: `tests/exporters/test_huggingface_hub_client.cpp`:322
- Brief: n/a
- Parameters:
  - `<unnamed>` (HuggingFaceHubClientTest): n/a
  - `<unnamed>` (ProgressCallbackNotCalledWhenNoToken): n/a

#### `TEST(HuggingFaceHubClientTest, RateLimit_MetricsNotIncrementedOnAuthFailure)`
- Source: `tests/exporters/test_huggingface_hub_client.cpp`:886
- Brief: n/a
- Parameters:
  - `<unnamed>` (HuggingFaceHubClientTest): n/a
  - `<unnamed>` (RateLimit_MetricsNotIncrementedOnAuthFailure): n/a

#### `TEST(HuggingFaceHubClientTest, TokenFromEnvPreferredOverEmpty)`
- Source: `tests/exporters/test_huggingface_hub_client.cpp`:296
- Brief: n/a
- Parameters:
  - `<unnamed>` (HuggingFaceHubClientTest): n/a
  - `<unnamed>` (TokenFromEnvPreferredOverEmpty): n/a

#### `TEST(HuggingFaceHubClientTest, UploadShardsAuditLogWrittenOnNoToken)`
- Source: `tests/exporters/test_huggingface_hub_client.cpp`:635
- Brief: n/a
- Parameters:
  - `<unnamed>` (HuggingFaceHubClientTest): n/a
  - `<unnamed>` (UploadShardsAuditLogWrittenOnNoToken): n/a

#### `TEST(HuggingFaceHubClientTest, UploadShardsAuditLogWrittenOnPolicyDenial)`
- Source: `tests/exporters/test_huggingface_hub_client.cpp`:603
- Brief: n/a
- Parameters:
  - `<unnamed>` (HuggingFaceHubClientTest): n/a
  - `<unnamed>` (UploadShardsAuditLogWrittenOnPolicyDenial): n/a

#### `TEST(HuggingFaceHubClientTest, UploadShardsEmptyListReturnsError)`
- Source: `tests/exporters/test_huggingface_hub_client.cpp`:514
- Brief: n/a
- Parameters:
  - `<unnamed>` (HuggingFaceHubClientTest): n/a
  - `<unnamed>` (UploadShardsEmptyListReturnsError): n/a

#### `TEST(HuggingFaceHubClientTest, UploadShardsEmptyRepoIdReturnsError)`
- Source: `tests/exporters/test_huggingface_hub_client.cpp`:499
- Brief: n/a
- Parameters:
  - `<unnamed>` (HuggingFaceHubClientTest): n/a
  - `<unnamed>` (UploadShardsEmptyRepoIdReturnsError): n/a

#### `TEST(HuggingFaceHubClientTest, UploadShardsNoTokenReturnsError)`
- Source: `tests/exporters/test_huggingface_hub_client.cpp`:482
- Brief: n/a
- Parameters:
  - `<unnamed>` (HuggingFaceHubClientTest): n/a
  - `<unnamed>` (UploadShardsNoTokenReturnsError): n/a

#### `TEST(HuggingFaceHubClientTest, UploadShardsPolicyDeniedReturnsFailure)`
- Source: `tests/exporters/test_huggingface_hub_client.cpp`:545
- Brief: n/a
- Parameters:
  - `<unnamed>` (HuggingFaceHubClientTest): n/a
  - `<unnamed>` (UploadShardsPolicyDeniedReturnsFailure): n/a

#### `TEST(HuggingFaceHubClientTest, UploadShardsProgressCallbackNotCalledWhenNoToken)`
- Source: `tests/exporters/test_huggingface_hub_client.cpp`:525
- Brief: n/a
- Parameters:
  - `<unnamed>` (HuggingFaceHubClientTest): n/a
  - `<unnamed>` (UploadShardsProgressCallbackNotCalledWhenNoToken): n/a

#### `TEST(HuggingFaceHubClientTest, UploadShardsTokenFromEnvProceeedsToNetwork)`
- Source: `tests/exporters/test_huggingface_hub_client.cpp`:570
- Brief: n/a
- Parameters:
  - `<unnamed>` (HuggingFaceHubClientTest): n/a
  - `<unnamed>` (UploadShardsTokenFromEnvProceeedsToNetwork): n/a

#### `std::string decodeBase64ForTest(const std::string &input)`
- Source: `tests/exporters/test_huggingface_hub_client.cpp`:62
- Brief: n/a
- Parameters:
  - `input` (const std::string &): n/a

#### `std::string makeDatasetDir()`
- Source: `tests/exporters/test_huggingface_hub_client.cpp`:25
- Brief: n/a
- Parameters: none

#### `std::pair< std::shared_ptr< themis::utils::AuditLogger >, std::string > makeTestAuditLogger()`
- Source: `tests/exporters/test_huggingface_hub_client.cpp`:51
- Brief: Create an AuditLogger that writes to a temporary file.
- Parameters: none

#### `std::string readDecodedAuditPayloadText(const std::string &log_path)`
- Source: `tests/exporters/test_huggingface_hub_client.cpp`:91
- Brief: n/a
- Parameters:
  - `log_path` (const std::string &): n/a

### test_incremental_exporter.cpp

#### `TEST_F(IncrementalExporterTest, CorruptWatermarkFallsBackToFullExport)`
- Source: `tests/exporters/test_incremental_exporter.cpp`:607
- Brief: n/a
- Parameters:
  - `<unnamed>` (IncrementalExporterTest): n/a
  - `<unnamed>` (CorruptWatermarkFallsBackToFullExport): n/a

#### `TEST_F(IncrementalExporterTest, DeltaExportSkipsEntitiesBelowWatermark)`
- Source: `tests/exporters/test_incremental_exporter.cpp`:146
- Brief: n/a
- Parameters:
  - `<unnamed>` (IncrementalExporterTest): n/a
  - `<unnamed>` (DeltaExportSkipsEntitiesBelowWatermark): n/a

#### `TEST_F(IncrementalExporterTest, EntitiesWithoutSequenceFieldAreExportedByDefault)`
- Source: `tests/exporters/test_incremental_exporter.cpp`:285
- Brief: n/a
- Parameters:
  - `<unnamed>` (IncrementalExporterTest): n/a
  - `<unnamed>` (EntitiesWithoutSequenceFieldAreExportedByDefault): n/a

#### `TEST_F(IncrementalExporterTest, EntitiesWithoutSequenceFieldSkippedWhenFailClosed)`
- Source: `tests/exporters/test_incremental_exporter.cpp`:321
- Brief: n/a
- Parameters:
  - `<unnamed>` (IncrementalExporterTest): n/a
  - `<unnamed>` (EntitiesWithoutSequenceFieldSkippedWhenFailClosed): n/a

#### `TEST_F(IncrementalExporterTest, ExportStatsToJsonContainsSkippedEntities)`
- Source: `tests/exporters/test_incremental_exporter.cpp`:632
- Brief: n/a
- Parameters:
  - `<unnamed>` (IncrementalExporterTest): n/a
  - `<unnamed>` (ExportStatsToJsonContainsSkippedEntities): n/a

#### `TEST_F(IncrementalExporterTest, FieldFilteringApplied)`
- Source: `tests/exporters/test_incremental_exporter.cpp`:439
- Brief: n/a
- Parameters:
  - `<unnamed>` (IncrementalExporterTest): n/a
  - `<unnamed>` (FieldFilteringApplied): n/a

#### `TEST_F(IncrementalExporterTest, FilterExpressionAppliedBeforeExportWrite)`
- Source: `tests/exporters/test_incremental_exporter.cpp`:458
- Brief: n/a
- Parameters:
  - `<unnamed>` (IncrementalExporterTest): n/a
  - `<unnamed>` (FilterExpressionAppliedBeforeExportWrite): n/a

#### `TEST_F(IncrementalExporterTest, FloatingPointSequenceFieldRespected)`
- Source: `tests/exporters/test_incremental_exporter.cpp`:399
- Brief: n/a
- Parameters:
  - `<unnamed>` (IncrementalExporterTest): n/a
  - `<unnamed>` (FloatingPointSequenceFieldRespected): n/a

#### `TEST_F(IncrementalExporterTest, FullExportWithoutWatermarkFile)`
- Source: `tests/exporters/test_incremental_exporter.cpp`:97
- Brief: n/a
- Parameters:
  - `<unnamed>` (IncrementalExporterTest): n/a
  - `<unnamed>` (FullExportWithoutWatermarkFile): n/a

#### `TEST_F(IncrementalExporterTest, FullExportWritesWatermark)`
- Source: `tests/exporters/test_incremental_exporter.cpp`:119
- Brief: n/a
- Parameters:
  - `<unnamed>` (IncrementalExporterTest): n/a
  - `<unnamed>` (FullExportWritesWatermark): n/a

#### `TEST_F(IncrementalExporterTest, GetNameAndVersion)`
- Source: `tests/exporters/test_incremental_exporter.cpp`:482
- Brief: n/a
- Parameters:
  - `<unnamed>` (IncrementalExporterTest): n/a
  - `<unnamed>` (GetNameAndVersion): n/a

#### `TEST_F(IncrementalExporterTest, MetricsJsonContainsDeltaField)`
- Source: `tests/exporters/test_incremental_exporter.cpp`:383
- Brief: n/a
- Parameters:
  - `<unnamed>` (IncrementalExporterTest): n/a
  - `<unnamed>` (MetricsJsonContainsDeltaField): n/a

#### `TEST_F(IncrementalExporterTest, MetricsTrackSkippedEntities)`
- Source: `tests/exporters/test_incremental_exporter.cpp`:359
- Brief: n/a
- Parameters:
  - `<unnamed>` (IncrementalExporterTest): n/a
  - `<unnamed>` (MetricsTrackSkippedEntities): n/a

#### `TEST_F(IncrementalExporterTest, NewEntitiesExportedAfterWatermarkSet)`
- Source: `tests/exporters/test_incremental_exporter.cpp`:230
- Brief: n/a
- Parameters:
  - `<unnamed>` (IncrementalExporterTest): n/a
  - `<unnamed>` (NewEntitiesExportedAfterWatermarkSet): n/a

#### `TEST_F(IncrementalExporterTest, ProgressCallbackInvoked)`
- Source: `tests/exporters/test_incremental_exporter.cpp`:503
- Brief: n/a
- Parameters:
  - `<unnamed>` (IncrementalExporterTest): n/a
  - `<unnamed>` (ProgressCallbackInvoked): n/a

#### `TEST_F(IncrementalExporterTest, ReadWatermarkReturnsMINWhenNoFile)`
- Source: `tests/exporters/test_incremental_exporter.cpp`:265
- Brief: n/a
- Parameters:
  - `<unnamed>` (IncrementalExporterTest): n/a
  - `<unnamed>` (ReadWatermarkReturnsMINWhenNoFile): n/a

#### `TEST_F(IncrementalExporterTest, ReadWatermarkReturnsMINWhenPathEmpty)`
- Source: `tests/exporters/test_incremental_exporter.cpp`:273
- Brief: n/a
- Parameters:
  - `<unnamed>` (IncrementalExporterTest): n/a
  - `<unnamed>` (ReadWatermarkReturnsMINWhenPathEmpty): n/a

#### `TEST_F(IncrementalExporterTest, SecondRunExportsNothingWhenNothingChanged)`
- Source: `tests/exporters/test_incremental_exporter.cpp`:209
- Brief: n/a
- Parameters:
  - `<unnamed>` (IncrementalExporterTest): n/a
  - `<unnamed>` (SecondRunExportsNothingWhenNothingChanged): n/a

#### `TEST_F(IncrementalExporterTest, SupportedFormats)`
- Source: `tests/exporters/test_incremental_exporter.cpp`:488
- Brief: n/a
- Parameters:
  - `<unnamed>` (IncrementalExporterTest): n/a
  - `<unnamed>` (SupportedFormats): n/a

#### `TEST_F(IncrementalExporterTest, WatermarkNotAdvancedOnPartialSizeLimitedScan)`
- Source: `tests/exporters/test_incremental_exporter.cpp`:550
- Brief: n/a
- Parameters:
  - `<unnamed>` (IncrementalExporterTest): n/a
  - `<unnamed>` (WatermarkNotAdvancedOnPartialSizeLimitedScan): n/a

#### `TEST_F(IncrementalExporterTest, WatermarkUnchangedWhenNothingExported)`
- Source: `tests/exporters/test_incremental_exporter.cpp`:523
- Brief: n/a
- Parameters:
  - `<unnamed>` (IncrementalExporterTest): n/a
  - `<unnamed>` (WatermarkUnchangedWhenNothingExported): n/a

#### `TEST_F(IncrementalExporterTest, WatermarkUpdatedAfterDeltaExport)`
- Source: `tests/exporters/test_incremental_exporter.cpp`:182
- Brief: n/a
- Parameters:
  - `<unnamed>` (IncrementalExporterTest): n/a
  - `<unnamed>` (WatermarkUpdatedAfterDeltaExport): n/a

### test_join_exporter.cpp

#### `TEST(JoinExportConfigTest, DefaultValues)`
- Source: `tests/exporters/test_join_exporter.cpp`:99
- Brief: n/a
- Parameters:
  - `<unnamed>` (JoinExportConfigTest): n/a
  - `<unnamed>` (DefaultValues): n/a

#### `TEST(JoinExportConfigTest, FieldsCanBeSet)`
- Source: `tests/exporters/test_join_exporter.cpp`:109
- Brief: n/a
- Parameters:
  - `<unnamed>` (JoinExportConfigTest): n/a
  - `<unnamed>` (FieldsCanBeSet): n/a

#### `TEST(JoinExporterErrorTest, AmbiguousFieldInOutputFieldsWithoutAliasThrows)`
- Source: `tests/exporters/test_join_exporter.cpp`:460
- Brief: n/a
- Parameters:
  - `<unnamed>` (JoinExporterErrorTest): n/a
  - `<unnamed>` (AmbiguousFieldInOutputFieldsWithoutAliasThrows): n/a

#### `TEST(JoinExporterErrorTest, AmbiguousFieldWithoutAliasThrows)`
- Source: `tests/exporters/test_join_exporter.cpp`:398
- Brief: n/a
- Parameters:
  - `<unnamed>` (JoinExporterErrorTest): n/a
  - `<unnamed>` (AmbiguousFieldWithoutAliasThrows): n/a

#### `TEST(JoinExporterErrorTest, EmptyLeftCollectionThrows)`
- Source: `tests/exporters/test_join_exporter.cpp`:305
- Brief: n/a
- Parameters:
  - `<unnamed>` (JoinExporterErrorTest): n/a
  - `<unnamed>` (EmptyLeftCollectionThrows): n/a

#### `TEST(JoinExporterErrorTest, EmptyRightCollectionSetRightThrows)`
- Source: `tests/exporters/test_join_exporter.cpp`:327
- Brief: n/a
- Parameters:
  - `<unnamed>` (JoinExporterErrorTest): n/a
  - `<unnamed>` (EmptyRightCollectionSetRightThrows): n/a

#### `TEST(JoinExporterErrorTest, ExportWithoutSetRightCollectionFailsClosed)`
- Source: `tests/exporters/test_join_exporter.cpp`:345
- Brief: n/a
- Parameters:
  - `<unnamed>` (JoinExporterErrorTest): n/a
  - `<unnamed>` (ExportWithoutSetRightCollectionFailsClosed): n/a

#### `TEST(JoinExporterErrorTest, InvalidJoinPredicateThrows)`
- Source: `tests/exporters/test_join_exporter.cpp`:373
- Brief: n/a
- Parameters:
  - `<unnamed>` (JoinExporterErrorTest): n/a
  - `<unnamed>` (InvalidJoinPredicateThrows): n/a

#### `TEST(JoinExporterErrorTest, MemoryLimitExceededThrows)`
- Source: `tests/exporters/test_join_exporter.cpp`:434
- Brief: n/a
- Parameters:
  - `<unnamed>` (JoinExporterErrorTest): n/a
  - `<unnamed>` (MemoryLimitExceededThrows): n/a

#### `TEST(JoinExporterInterfaceTest, MetricsAccessible)`
- Source: `tests/exporters/test_join_exporter.cpp`:655
- Brief: n/a
- Parameters:
  - `<unnamed>` (JoinExporterInterfaceTest): n/a
  - `<unnamed>` (MetricsAccessible): n/a

#### `TEST(JoinExporterInterfaceTest, NameAndVersion)`
- Source: `tests/exporters/test_join_exporter.cpp`:642
- Brief: n/a
- Parameters:
  - `<unnamed>` (JoinExporterInterfaceTest): n/a
  - `<unnamed>` (NameAndVersion): n/a

#### `TEST(JoinExporterInterfaceTest, SupportedFormats)`
- Source: `tests/exporters/test_join_exporter.cpp`:648
- Brief: n/a
- Parameters:
  - `<unnamed>` (JoinExporterInterfaceTest): n/a
  - `<unnamed>` (SupportedFormats): n/a

#### `TEST(JoinExporterMemoryTest, CustomMemoryBudget)`
- Source: `tests/exporters/test_join_exporter.cpp`:634
- Brief: n/a
- Parameters:
  - `<unnamed>` (JoinExporterMemoryTest): n/a
  - `<unnamed>` (CustomMemoryBudget): n/a

#### `TEST(JoinExporterMemoryTest, MemoryBudgetDefault1GiB)`
- Source: `tests/exporters/test_join_exporter.cpp`:628
- Brief: n/a
- Parameters:
  - `<unnamed>` (JoinExporterMemoryTest): n/a
  - `<unnamed>` (MemoryBudgetDefault1GiB): n/a

#### `TEST_F(JoinExporterTest, BasicInnerJoin)`
- Source: `tests/exporters/test_join_exporter.cpp`:128
- Brief: n/a
- Parameters:
  - `<unnamed>` (JoinExporterTest): n/a
  - `<unnamed>` (BasicInnerJoin): n/a

#### `TEST_F(JoinExporterTest, EmptyLeftProducesNoOutput)`
- Source: `tests/exporters/test_join_exporter.cpp`:689
- Brief: n/a
- Parameters:
  - `<unnamed>` (JoinExporterTest): n/a
  - `<unnamed>` (EmptyLeftProducesNoOutput): n/a

#### `TEST_F(JoinExporterTest, EmptyRightProducesNoOutput)`
- Source: `tests/exporters/test_join_exporter.cpp`:707
- Brief: n/a
- Parameters:
  - `<unnamed>` (JoinExporterTest): n/a
  - `<unnamed>` (EmptyRightProducesNoOutput): n/a

#### `TEST_F(JoinExporterTest, ExportOptionsFilterExpressionFiltersMergedRows)`
- Source: `tests/exporters/test_join_exporter.cpp`:279
- Brief: n/a
- Parameters:
  - `<unnamed>` (JoinExporterTest): n/a
  - `<unnamed>` (ExportOptionsFilterExpressionFiltersMergedRows): n/a

#### `TEST_F(JoinExporterTest, JoinPredicateFiltersRows)`
- Source: `tests/exporters/test_join_exporter.cpp`:255
- Brief: n/a
- Parameters:
  - `<unnamed>` (JoinExporterTest): n/a
  - `<unnamed>` (JoinPredicateFiltersRows): n/a

#### `TEST_F(JoinExporterTest, MergedDocContainsBothSideFields)`
- Source: `tests/exporters/test_join_exporter.cpp`:162
- Brief: n/a
- Parameters:
  - `<unnamed>` (JoinExporterTest): n/a
  - `<unnamed>` (MergedDocContainsBothSideFields): n/a

#### `TEST_F(JoinExporterTest, OutputFieldsQualifiedLeftRight)`
- Source: `tests/exporters/test_join_exporter.cpp`:218
- Brief: n/a
- Parameters:
  - `<unnamed>` (JoinExporterTest): n/a
  - `<unnamed>` (OutputFieldsQualifiedLeftRight): n/a

#### `TEST_F(JoinExporterTest, OutputFieldsSelectAndRename)`
- Source: `tests/exporters/test_join_exporter.cpp`:191
- Brief: n/a
- Parameters:
  - `<unnamed>` (JoinExporterTest): n/a
  - `<unnamed>` (OutputFieldsSelectAndRename): n/a

#### `TEST_F(JoinExporterTest, PIIDetectionFailOnPIIThrows)`
- Source: `tests/exporters/test_join_exporter.cpp`:533
- Brief: n/a
- Parameters:
  - `<unnamed>` (JoinExporterTest): n/a
  - `<unnamed>` (PIIDetectionFailOnPIIThrows): n/a

#### `TEST_F(JoinExporterTest, PIIDetectionOnMergedRecord)`
- Source: `tests/exporters/test_join_exporter.cpp`:498
- Brief: n/a
- Parameters:
  - `<unnamed>` (JoinExporterTest): n/a
  - `<unnamed>` (PIIDetectionOnMergedRecord): n/a

#### `TEST_F(JoinExporterTest, SetRightCollectionReplacesTable)`
- Source: `tests/exporters/test_join_exporter.cpp`:662
- Brief: n/a
- Parameters:
  - `<unnamed>` (JoinExporterTest): n/a
  - `<unnamed>` (SetRightCollectionReplacesTable): n/a

#### `TEST_F(JoinExporterTest, Throughput_50kDocsPerSecond)`
- Source: `tests/exporters/test_join_exporter.cpp`:570
- Brief: n/a
- Parameters:
  - `<unnamed>` (JoinExporterTest): n/a
  - `<unnamed>` (Throughput_50kDocsPerSecond): n/a

#### `BaseEntity makeAnnotation(const std::string &key, const std::string &doc_id, const std::string &label, double score=1.0)`
- Source: `tests/exporters/test_join_exporter.cpp`:30
- Brief: n/a
- Parameters:
  - `key` (const std::string &): n/a
  - `doc_id` (const std::string &): n/a
  - `label` (const std::string &): n/a
  - `score` (double): n/a

#### `BaseEntity makeDoc(const std::string &key, const std::string &content, const std::string &join_key)`
- Source: `tests/exporters/test_join_exporter.cpp`:20
- Brief: n/a
- Parameters:
  - `key` (const std::string &): n/a
  - `content` (const std::string &): n/a
  - `join_key` (const std::string &): n/a

#### `std::vector< std::string > readLines(const std::string &path)`
- Source: `tests/exporters/test_join_exporter.cpp`:42
- Brief: n/a
- Parameters:
  - `path` (const std::string &): n/a

### test_jsonl_llm_exporter.cpp

#### `TEST_F(JSONLLLMExporterTest, BasicExportInstructionTuning)`
- Source: `tests/exporters/test_jsonl_llm_exporter.cpp`:74
- Brief: n/a
- Parameters:
  - `<unnamed>` (JSONLLLMExporterTest): n/a
  - `<unnamed>` (BasicExportInstructionTuning): n/a

#### `TEST_F(JSONLLLMExporterTest, BufferSizeConfiguration)`
- Source: `tests/exporters/test_jsonl_llm_exporter.cpp`:939
- Brief: n/a
- Parameters:
  - `<unnamed>` (JSONLLLMExporterTest): n/a
  - `<unnamed>` (BufferSizeConfiguration): n/a

#### `TEST_F(JSONLLLMExporterTest, CompressionGzip)`
- Source: `tests/exporters/test_jsonl_llm_exporter.cpp`:848
- Brief: n/a
- Parameters:
  - `<unnamed>` (JSONLLLMExporterTest): n/a
  - `<unnamed>` (CompressionGzip): n/a

#### `TEST_F(JSONLLLMExporterTest, CompressionZstd)`
- Source: `tests/exporters/test_jsonl_llm_exporter.cpp`:876
- Brief: n/a
- Parameters:
  - `<unnamed>` (JSONLLLMExporterTest): n/a
  - `<unnamed>` (CompressionZstd): n/a

#### `TEST_F(JSONLLLMExporterTest, ContinueOnError)`
- Source: `tests/exporters/test_jsonl_llm_exporter.cpp`:173
- Brief: n/a
- Parameters:
  - `<unnamed>` (JSONLLLMExporterTest): n/a
  - `<unnamed>` (ContinueOnError): n/a

#### `TEST_F(JSONLLLMExporterTest, DuplicateDetection)`
- Source: `tests/exporters/test_jsonl_llm_exporter.cpp`:329
- Brief: n/a
- Parameters:
  - `<unnamed>` (JSONLLLMExporterTest): n/a
  - `<unnamed>` (DuplicateDetection): n/a

#### `TEST_F(JSONLLLMExporterTest, ExcludeFieldsDoesNotAffectRequiredFormatFields)`
- Source: `tests/exporters/test_jsonl_llm_exporter.cpp`:581
- Brief: n/a
- Parameters:
  - `<unnamed>` (JSONLLLMExporterTest): n/a
  - `<unnamed>` (ExcludeFieldsDoesNotAffectRequiredFormatFields): n/a

#### `TEST_F(JSONLLLMExporterTest, ExcludeFieldsFromMetadata)`
- Source: `tests/exporters/test_jsonl_llm_exporter.cpp`:513
- Brief: n/a
- Parameters:
  - `<unnamed>` (JSONLLLMExporterTest): n/a
  - `<unnamed>` (ExcludeFieldsFromMetadata): n/a

#### `TEST_F(JSONLLLMExporterTest, ExcludeRequiredCoreFieldSkipsEntity)`
- Source: `tests/exporters/test_jsonl_llm_exporter.cpp`:608
- Brief: n/a
- Parameters:
  - `<unnamed>` (JSONLLLMExporterTest): n/a
  - `<unnamed>` (ExcludeRequiredCoreFieldSkipsEntity): n/a

#### `TEST_F(JSONLLLMExporterTest, ExportChatCompletionStyle)`
- Source: `tests/exporters/test_jsonl_llm_exporter.cpp`:108
- Brief: n/a
- Parameters:
  - `<unnamed>` (JSONLLLMExporterTest): n/a
  - `<unnamed>` (ExportChatCompletionStyle): n/a

#### `TEST_F(JSONLLLMExporterTest, ExportStatsToJson)`
- Source: `tests/exporters/test_jsonl_llm_exporter.cpp`:459
- Brief: n/a
- Parameters:
  - `<unnamed>` (JSONLLLMExporterTest): n/a
  - `<unnamed>` (ExportStatsToJson): n/a

#### `TEST_F(JSONLLLMExporterTest, ExportTextCompletionStyle)`
- Source: `tests/exporters/test_jsonl_llm_exporter.cpp`:132
- Brief: n/a
- Parameters:
  - `<unnamed>` (JSONLLLMExporterTest): n/a
  - `<unnamed>` (ExportTextCompletionStyle): n/a

#### `TEST_F(JSONLLLMExporterTest, FileSizeLimit)`
- Source: `tests/exporters/test_jsonl_llm_exporter.cpp`:925
- Brief: n/a
- Parameters:
  - `<unnamed>` (JSONLLLMExporterTest): n/a
  - `<unnamed>` (FileSizeLimit): n/a

#### `TEST_F(JSONLLLMExporterTest, IOErrorHandling)`
- Source: `tests/exporters/test_jsonl_llm_exporter.cpp`:156
- Brief: n/a
- Parameters:
  - `<unnamed>` (JSONLLLMExporterTest): n/a
  - `<unnamed>` (IOErrorHandling): n/a

#### `TEST_F(JSONLLLMExporterTest, IncludeFieldsLimitsMetadata)`
- Source: `tests/exporters/test_jsonl_llm_exporter.cpp`:545
- Brief: n/a
- Parameters:
  - `<unnamed>` (JSONLLLMExporterTest): n/a
  - `<unnamed>` (IncludeFieldsLimitsMetadata): n/a

#### `TEST_F(JSONLLLMExporterTest, MaxErrorsLimit)`
- Source: `tests/exporters/test_jsonl_llm_exporter.cpp`:189
- Brief: n/a
- Parameters:
  - `<unnamed>` (JSONLLLMExporterTest): n/a
  - `<unnamed>` (MaxErrorsLimit): n/a

#### `TEST_F(JSONLLLMExporterTest, MetadataInclusion)`
- Source: `tests/exporters/test_jsonl_llm_exporter.cpp`:483
- Brief: n/a
- Parameters:
  - `<unnamed>` (JSONLLLMExporterTest): n/a
  - `<unnamed>` (MetadataInclusion): n/a

#### `TEST_F(JSONLLLMExporterTest, MetricsCollection)`
- Source: `tests/exporters/test_jsonl_llm_exporter.cpp`:208
- Brief: n/a
- Parameters:
  - `<unnamed>` (JSONLLLMExporterTest): n/a
  - `<unnamed>` (MetricsCollection): n/a

#### `TEST_F(JSONLLLMExporterTest, MetricsErrorTracking)`
- Source: `tests/exporters/test_jsonl_llm_exporter.cpp`:268
- Brief: n/a
- Parameters:
  - `<unnamed>` (JSONLLLMExporterTest): n/a
  - `<unnamed>` (MetricsErrorTracking): n/a

#### `TEST_F(JSONLLLMExporterTest, MetricsLatencyPercentiles)`
- Source: `tests/exporters/test_jsonl_llm_exporter.cpp`:227
- Brief: n/a
- Parameters:
  - `<unnamed>` (JSONLLLMExporterTest): n/a
  - `<unnamed>` (MetricsLatencyPercentiles): n/a

#### `TEST_F(JSONLLLMExporterTest, MetricsToJson)`
- Source: `tests/exporters/test_jsonl_llm_exporter.cpp`:248
- Brief: n/a
- Parameters:
  - `<unnamed>` (JSONLLLMExporterTest): n/a
  - `<unnamed>` (MetricsToJson): n/a

#### `TEST_F(JSONLLLMExporterTest, NoCompressionMetrics)`
- Source: `tests/exporters/test_jsonl_llm_exporter.cpp`:906
- Brief: n/a
- Parameters:
  - `<unnamed>` (JSONLLLMExporterTest): n/a
  - `<unnamed>` (NoCompressionMetrics): n/a

#### `TEST_F(JSONLLLMExporterTest, NoDuplicateDetection)`
- Source: `tests/exporters/test_jsonl_llm_exporter.cpp`:348
- Brief: n/a
- Parameters:
  - `<unnamed>` (JSONLLLMExporterTest): n/a
  - `<unnamed>` (NoDuplicateDetection): n/a

#### `TEST_F(JSONLLLMExporterTest, PIIDetection)`
- Source: `tests/exporters/test_jsonl_llm_exporter.cpp`:706
- Brief: n/a
- Parameters:
  - `<unnamed>` (JSONLLLMExporterTest): n/a
  - `<unnamed>` (PIIDetection): n/a

#### `TEST_F(JSONLLLMExporterTest, PIIFailOnDetection)`
- Source: `tests/exporters/test_jsonl_llm_exporter.cpp`:812
- Brief: n/a
- Parameters:
  - `<unnamed>` (JSONLLLMExporterTest): n/a
  - `<unnamed>` (PIIFailOnDetection): n/a

#### `TEST_F(JSONLLLMExporterTest, PIIRedactionHash)`
- Source: `tests/exporters/test_jsonl_llm_exporter.cpp`:774
- Brief: n/a
- Parameters:
  - `<unnamed>` (JSONLLLMExporterTest): n/a
  - `<unnamed>` (PIIRedactionHash): n/a

#### `TEST_F(JSONLLLMExporterTest, PIIRedactionMask)`
- Source: `tests/exporters/test_jsonl_llm_exporter.cpp`:736
- Brief: n/a
- Parameters:
  - `<unnamed>` (JSONLLLMExporterTest): n/a
  - `<unnamed>` (PIIRedactionMask): n/a

#### `TEST_F(JSONLLLMExporterTest, ProgressCallback)`
- Source: `tests/exporters/test_jsonl_llm_exporter.cpp`:432
- Brief: n/a
- Parameters:
  - `<unnamed>` (JSONLLLMExporterTest): n/a
  - `<unnamed>` (ProgressCallback): n/a

#### `TEST_F(JSONLLLMExporterTest, QualityFilterMaxLength)`
- Source: `tests/exporters/test_jsonl_llm_exporter.cpp`:312
- Brief: n/a
- Parameters:
  - `<unnamed>` (JSONLLLMExporterTest): n/a
  - `<unnamed>` (QualityFilterMaxLength): n/a

#### `TEST_F(JSONLLLMExporterTest, QualityFilterMinLength)`
- Source: `tests/exporters/test_jsonl_llm_exporter.cpp`:292
- Brief: n/a
- Parameters:
  - `<unnamed>` (JSONLLLMExporterTest): n/a
  - `<unnamed>` (QualityFilterMinLength): n/a

#### `TEST_F(JSONLLLMExporterTest, SchemaValidationDisabled)`
- Source: `tests/exporters/test_jsonl_llm_exporter.cpp`:365
- Brief: n/a
- Parameters:
  - `<unnamed>` (JSONLLLMExporterTest): n/a
  - `<unnamed>` (SchemaValidationDisabled): n/a

#### `TEST_F(JSONLLLMExporterTest, TenantInsufficientScopes)`
- Source: `tests/exporters/test_jsonl_llm_exporter.cpp`:683
- Brief: n/a
- Parameters:
  - `<unnamed>` (JSONLLLMExporterTest): n/a
  - `<unnamed>` (TenantInsufficientScopes): n/a

#### `TEST_F(JSONLLLMExporterTest, TenantIsolationBlocksCrossTenant)`
- Source: `tests/exporters/test_jsonl_llm_exporter.cpp`:653
- Brief: n/a
- Parameters:
  - `<unnamed>` (JSONLLLMExporterTest): n/a
  - `<unnamed>` (TenantIsolationBlocksCrossTenant): n/a

#### `TEST_F(JSONLLLMExporterTest, TenantIsolationWithContext)`
- Source: `tests/exporters/test_jsonl_llm_exporter.cpp`:626
- Brief: n/a
- Parameters:
  - `<unnamed>` (JSONLLLMExporterTest): n/a
  - `<unnamed>` (TenantIsolationWithContext): n/a

#### `TEST_F(JSONLLLMExporterTest, ToxicityFilterDisabledByDefault)`
- Source: `tests/exporters/test_jsonl_llm_exporter.cpp`:954
- Brief: n/a
- Parameters:
  - `<unnamed>` (JSONLLLMExporterTest): n/a
  - `<unnamed>` (ToxicityFilterDisabledByDefault): n/a

#### `TEST_F(JSONLLLMExporterTest, ToxicityFilterMetricsRecorded)`
- Source: `tests/exporters/test_jsonl_llm_exporter.cpp`:1027
- Brief: n/a
- Parameters:
  - `<unnamed>` (JSONLLLMExporterTest): n/a
  - `<unnamed>` (ToxicityFilterMetricsRecorded): n/a

#### `TEST_F(JSONLLLMExporterTest, ToxicityFilterPassesBenignSamples)`
- Source: `tests/exporters/test_jsonl_llm_exporter.cpp`:1012
- Brief: n/a
- Parameters:
  - `<unnamed>` (JSONLLLMExporterTest): n/a
  - `<unnamed>` (ToxicityFilterPassesBenignSamples): n/a

#### `TEST_F(JSONLLLMExporterTest, ToxicityFilterRejectsToxicSamples)`
- Source: `tests/exporters/test_jsonl_llm_exporter.cpp`:978
- Brief: n/a
- Parameters:
  - `<unnamed>` (JSONLLLMExporterTest): n/a
  - `<unnamed>` (ToxicityFilterRejectsToxicSamples): n/a

#### `TEST_F(JSONLLLMExporterTest, WeightingDisabled)`
- Source: `tests/exporters/test_jsonl_llm_exporter.cpp`:409
- Brief: n/a
- Parameters:
  - `<unnamed>` (JSONLLLMExporterTest): n/a
  - `<unnamed>` (WeightingDisabled): n/a

#### `TEST_F(JSONLLLMExporterTest, WeightingEnabled)`
- Source: `tests/exporters/test_jsonl_llm_exporter.cpp`:387
- Brief: n/a
- Parameters:
  - `<unnamed>` (JSONLLLMExporterTest): n/a
  - `<unnamed>` (WeightingEnabled): n/a

### test_parquet_exporter.cpp

#### `TEST_F(ParquetExporterTest, BasicExport)`
- Source: `tests/exporters/test_parquet_exporter.cpp`:92
- Brief: n/a
- Parameters:
  - `<unnamed>` (ParquetExporterTest): n/a
  - `<unnamed>` (BasicExport): n/a

#### `TEST_F(ParquetExporterTest, ColumnHintsConfiguration)`
- Source: `tests/exporters/test_parquet_exporter.cpp`:252
- Brief: n/a
- Parameters:
  - `<unnamed>` (ParquetExporterTest): n/a
  - `<unnamed>` (ColumnHintsConfiguration): n/a

#### `TEST_F(ParquetExporterTest, CompressionConfigNone)`
- Source: `tests/exporters/test_parquet_exporter.cpp`:227
- Brief: n/a
- Parameters:
  - `<unnamed>` (ParquetExporterTest): n/a
  - `<unnamed>` (CompressionConfigNone): n/a

#### `TEST_F(ParquetExporterTest, DeduplicatesByPrimaryKey)`
- Source: `tests/exporters/test_parquet_exporter.cpp`:286
- Brief: n/a
- Parameters:
  - `<unnamed>` (ParquetExporterTest): n/a
  - `<unnamed>` (DeduplicatesByPrimaryKey): n/a

#### `TEST_F(ParquetExporterTest, EmptyEntitiesProducesValidParquet)`
- Source: `tests/exporters/test_parquet_exporter.cpp`:124
- Brief: n/a
- Parameters:
  - `<unnamed>` (ParquetExporterTest): n/a
  - `<unnamed>` (EmptyEntitiesProducesValidParquet): n/a

#### `TEST_F(ParquetExporterTest, EmptyOutputPathThrows)`
- Source: `tests/exporters/test_parquet_exporter.cpp`:485
- Brief: n/a
- Parameters:
  - `<unnamed>` (ParquetExporterTest): n/a
  - `<unnamed>` (EmptyOutputPathThrows): n/a

#### `TEST_F(ParquetExporterTest, ExportOptionsIncludeFieldsRespected)`
- Source: `tests/exporters/test_parquet_exporter.cpp`:196
- Brief: n/a
- Parameters:
  - `<unnamed>` (ParquetExporterTest): n/a
  - `<unnamed>` (ExportOptionsIncludeFieldsRespected): n/a

#### `TEST_F(ParquetExporterTest, ExportProducesNonEmptyFile)`
- Source: `tests/exporters/test_parquet_exporter.cpp`:113
- Brief: n/a
- Parameters:
  - `<unnamed>` (ParquetExporterTest): n/a
  - `<unnamed>` (ExportProducesNonEmptyFile): n/a

#### `TEST_F(ParquetExporterTest, ExportWithExcludeColumns)`
- Source: `tests/exporters/test_parquet_exporter.cpp`:183
- Brief: n/a
- Parameters:
  - `<unnamed>` (ParquetExporterTest): n/a
  - `<unnamed>` (ExportWithExcludeColumns): n/a

#### `TEST_F(ParquetExporterTest, ExportWithIncludeColumns)`
- Source: `tests/exporters/test_parquet_exporter.cpp`:169
- Brief: n/a
- Parameters:
  - `<unnamed>` (ParquetExporterTest): n/a
  - `<unnamed>` (ExportWithIncludeColumns): n/a

#### `TEST_F(ParquetExporterTest, FileMetadataWritten)`
- Source: `tests/exporters/test_parquet_exporter.cpp`:268
- Brief: n/a
- Parameters:
  - `<unnamed>` (ParquetExporterTest): n/a
  - `<unnamed>` (FileMetadataWritten): n/a

#### `TEST_F(ParquetExporterTest, GetName)`
- Source: `tests/exporters/test_parquet_exporter.cpp`:571
- Brief: n/a
- Parameters:
  - `<unnamed>` (ParquetExporterTest): n/a
  - `<unnamed>` (GetName): n/a

#### `TEST_F(ParquetExporterTest, GetSupportedFormats)`
- Source: `tests/exporters/test_parquet_exporter.cpp`:581
- Brief: n/a
- Parameters:
  - `<unnamed>` (ParquetExporterTest): n/a
  - `<unnamed>` (GetSupportedFormats): n/a

#### `TEST_F(ParquetExporterTest, GetVersion)`
- Source: `tests/exporters/test_parquet_exporter.cpp`:576
- Brief: n/a
- Parameters:
  - `<unnamed>` (ParquetExporterTest): n/a
  - `<unnamed>` (GetVersion): n/a

#### `TEST_F(ParquetExporterTest, InvalidOutputPathErrors)`
- Source: `tests/exporters/test_parquet_exporter.cpp`:472
- Brief: n/a
- Parameters:
  - `<unnamed>` (ParquetExporterTest): n/a
  - `<unnamed>` (InvalidOutputPathErrors): n/a

#### `TEST_F(ParquetExporterTest, IsArrowAvailableReturnsBool)`
- Source: `tests/exporters/test_parquet_exporter.cpp`:588
- Brief: n/a
- Parameters:
  - `<unnamed>` (ParquetExporterTest): n/a
  - `<unnamed>` (IsArrowAvailableReturnsBool): n/a

#### `TEST_F(ParquetExporterTest, LargeBatchExport)`
- Source: `tests/exporters/test_parquet_exporter.cpp`:599
- Brief: n/a
- Parameters:
  - `<unnamed>` (ParquetExporterTest): n/a
  - `<unnamed>` (LargeBatchExport): n/a

#### `TEST_F(ParquetExporterTest, MetricsRecordedAfterExport)`
- Source: `tests/exporters/test_parquet_exporter.cpp`:501
- Brief: n/a
- Parameters:
  - `<unnamed>` (ParquetExporterTest): n/a
  - `<unnamed>` (MetricsRecordedAfterExport): n/a

#### `TEST_F(ParquetExporterTest, MetricsResetWorks)`
- Source: `tests/exporters/test_parquet_exporter.cpp`:514
- Brief: n/a
- Parameters:
  - `<unnamed>` (ParquetExporterTest): n/a
  - `<unnamed>` (MetricsResetWorks): n/a

#### `TEST_F(ParquetExporterTest, PIIDetectionTracksHits)`
- Source: `tests/exporters/test_parquet_exporter.cpp`:394
- Brief: n/a
- Parameters:
  - `<unnamed>` (ParquetExporterTest): n/a
  - `<unnamed>` (PIIDetectionTracksHits): n/a

#### `TEST_F(ParquetExporterTest, PIIFailOnDetectionThrows)`
- Source: `tests/exporters/test_parquet_exporter.cpp`:444
- Brief: n/a
- Parameters:
  - `<unnamed>` (ParquetExporterTest): n/a
  - `<unnamed>` (PIIFailOnDetectionThrows): n/a

#### `TEST_F(ParquetExporterTest, PIIRedactionMask)`
- Source: `tests/exporters/test_parquet_exporter.cpp`:419
- Brief: n/a
- Parameters:
  - `<unnamed>` (ParquetExporterTest): n/a
  - `<unnamed>` (PIIRedactionMask): n/a

#### `TEST_F(ParquetExporterTest, ParquetBytesCounterAccumulatesAcrossExports)`
- Source: `tests/exporters/test_parquet_exporter.cpp`:553
- Brief: n/a
- Parameters:
  - `<unnamed>` (ParquetExporterTest): n/a
  - `<unnamed>` (ParquetBytesCounterAccumulatesAcrossExports): n/a

#### `TEST_F(ParquetExporterTest, ParquetBytesCounterMatchesBytesWritten)`
- Source: `tests/exporters/test_parquet_exporter.cpp`:540
- Brief: n/a
- Parameters:
  - `<unnamed>` (ParquetExporterTest): n/a
  - `<unnamed>` (ParquetBytesCounterMatchesBytesWritten): n/a

#### `TEST_F(ParquetExporterTest, ParquetBytesWrittenCounterTracked)`
- Source: `tests/exporters/test_parquet_exporter.cpp`:528
- Brief: n/a
- Parameters:
  - `<unnamed>` (ParquetExporterTest): n/a
  - `<unnamed>` (ParquetBytesWrittenCounterTracked): n/a

#### `TEST_F(ParquetExporterTest, ProgressCallbackInvoked)`
- Source: `tests/exporters/test_parquet_exporter.cpp`:304
- Brief: n/a
- Parameters:
  - `<unnamed>` (ParquetExporterTest): n/a
  - `<unnamed>` (ProgressCallbackInvoked): n/a

#### `TEST_F(ParquetExporterTest, RowGroupSizeConfiguration)`
- Source: `tests/exporters/test_parquet_exporter.cpp`:213
- Brief: n/a
- Parameters:
  - `<unnamed>` (ParquetExporterTest): n/a
  - `<unnamed>` (RowGroupSizeConfiguration): n/a

#### `TEST_F(ParquetExporterTest, SetConfigAndGetConfig)`
- Source: `tests/exporters/test_parquet_exporter.cpp`:240
- Brief: n/a
- Parameters:
  - `<unnamed>` (ParquetExporterTest): n/a
  - `<unnamed>` (SetConfigAndGetConfig): n/a

#### `TEST_F(ParquetExporterTest, StatsMetricsAttached)`
- Source: `tests/exporters/test_parquet_exporter.cpp`:154
- Brief: n/a
- Parameters:
  - `<unnamed>` (ParquetExporterTest): n/a
  - `<unnamed>` (StatsMetricsAttached): n/a

#### `TEST_F(ParquetExporterTest, StatsTotalEntitiesMatchesInput)`
- Source: `tests/exporters/test_parquet_exporter.cpp`:143
- Brief: n/a
- Parameters:
  - `<unnamed>` (ParquetExporterTest): n/a
  - `<unnamed>` (StatsTotalEntitiesMatchesInput): n/a

#### `TEST_F(ParquetExporterTest, TenantInsufficientScopesThrows)`
- Source: `tests/exporters/test_parquet_exporter.cpp`:371
- Brief: n/a
- Parameters:
  - `<unnamed>` (ParquetExporterTest): n/a
  - `<unnamed>` (TenantInsufficientScopesThrows): n/a

#### `TEST_F(ParquetExporterTest, TenantIsolationBlocksCrossTenantRows)`
- Source: `tests/exporters/test_parquet_exporter.cpp`:348
- Brief: n/a
- Parameters:
  - `<unnamed>` (ParquetExporterTest): n/a
  - `<unnamed>` (TenantIsolationBlocksCrossTenantRows): n/a

#### `TEST_F(ParquetExporterTest, TenantIsolationWithMatchingTenant)`
- Source: `tests/exporters/test_parquet_exporter.cpp`:324
- Brief: n/a
- Parameters:
  - `<unnamed>` (ParquetExporterTest): n/a
  - `<unnamed>` (TenantIsolationWithMatchingTenant): n/a

### test_streaming_exporter.cpp

#### `TEST_F(StreamingExporterTest, CheckpointMetricsRecorded)`
- Source: `tests/exporters/test_streaming_exporter.cpp`:334
- Brief: n/a
- Parameters:
  - `<unnamed>` (StreamingExporterTest): n/a
  - `<unnamed>` (CheckpointMetricsRecorded): n/a

#### `TEST_F(StreamingExporterTest, CheckpointWrittenAfterEachPage)`
- Source: `tests/exporters/test_streaming_exporter.cpp`:312
- Brief: n/a
- Parameters:
  - `<unnamed>` (StreamingExporterTest): n/a
  - `<unnamed>` (CheckpointWrittenAfterEachPage): n/a

#### `TEST_F(StreamingExporterTest, CursorBasic)`
- Source: `tests/exporters/test_streaming_exporter.cpp`:77
- Brief: n/a
- Parameters:
  - `<unnamed>` (StreamingExporterTest): n/a
  - `<unnamed>` (CursorBasic): n/a

#### `TEST_F(StreamingExporterTest, CursorEmptyCollection)`
- Source: `tests/exporters/test_streaming_exporter.cpp`:110
- Brief: n/a
- Parameters:
  - `<unnamed>` (StreamingExporterTest): n/a
  - `<unnamed>` (CursorEmptyCollection): n/a

#### `TEST_F(StreamingExporterTest, CursorSeekBeyondEnd)`
- Source: `tests/exporters/test_streaming_exporter.cpp`:105
- Brief: n/a
- Parameters:
  - `<unnamed>` (StreamingExporterTest): n/a
  - `<unnamed>` (CursorSeekBeyondEnd): n/a

#### `TEST_F(StreamingExporterTest, CursorSeekTo)`
- Source: `tests/exporters/test_streaming_exporter.cpp`:93
- Brief: n/a
- Parameters:
  - `<unnamed>` (StreamingExporterTest): n/a
  - `<unnamed>` (CursorSeekTo): n/a

#### `TEST_F(StreamingExporterTest, ETAPopulatedDuringProgress)`
- Source: `tests/exporters/test_streaming_exporter.cpp`:234
- Brief: n/a
- Parameters:
  - `<unnamed>` (StreamingExporterTest): n/a
  - `<unnamed>` (ETAPopulatedDuringProgress): n/a

#### `TEST_F(StreamingExporterTest, ExcludeFieldsFilter)`
- Source: `tests/exporters/test_streaming_exporter.cpp`:291
- Brief: n/a
- Parameters:
  - `<unnamed>` (StreamingExporterTest): n/a
  - `<unnamed>` (ExcludeFieldsFilter): n/a

#### `TEST_F(StreamingExporterTest, ExportEntitiesBasic)`
- Source: `tests/exporters/test_streaming_exporter.cpp`:121
- Brief: n/a
- Parameters:
  - `<unnamed>` (StreamingExporterTest): n/a
  - `<unnamed>` (ExportEntitiesBasic): n/a

#### `TEST_F(StreamingExporterTest, ExportEntitiesOutputContainsPrimaryKey)`
- Source: `tests/exporters/test_streaming_exporter.cpp`:145
- Brief: n/a
- Parameters:
  - `<unnamed>` (StreamingExporterTest): n/a
  - `<unnamed>` (ExportEntitiesOutputContainsPrimaryKey): n/a

#### `TEST_F(StreamingExporterTest, ExportFromCursorEmptyCollection)`
- Source: `tests/exporters/test_streaming_exporter.cpp`:193
- Brief: n/a
- Parameters:
  - `<unnamed>` (StreamingExporterTest): n/a
  - `<unnamed>` (ExportFromCursorEmptyCollection): n/a

#### `TEST_F(StreamingExporterTest, ExportFromCursorLargePageSize)`
- Source: `tests/exporters/test_streaming_exporter.cpp`:179
- Brief: n/a
- Parameters:
  - `<unnamed>` (StreamingExporterTest): n/a
  - `<unnamed>` (ExportFromCursorLargePageSize): n/a

#### `TEST_F(StreamingExporterTest, ExportFromCursorPageSizeOne)`
- Source: `tests/exporters/test_streaming_exporter.cpp`:163
- Brief: n/a
- Parameters:
  - `<unnamed>` (StreamingExporterTest): n/a
  - `<unnamed>` (ExportFromCursorPageSizeOne): n/a

#### `TEST_F(StreamingExporterTest, FinalStatsETAIsZero)`
- Source: `tests/exporters/test_streaming_exporter.cpp`:258
- Brief: n/a
- Parameters:
  - `<unnamed>` (StreamingExporterTest): n/a
  - `<unnamed>` (FinalStatsETAIsZero): n/a

#### `TEST_F(StreamingExporterTest, GetNameAndVersion)`
- Source: `tests/exporters/test_streaming_exporter.cpp`:441
- Brief: n/a
- Parameters:
  - `<unnamed>` (StreamingExporterTest): n/a
  - `<unnamed>` (GetNameAndVersion): n/a

#### `TEST_F(StreamingExporterTest, GzipCompression)`
- Source: `tests/exporters/test_streaming_exporter.cpp`:365
- Brief: n/a
- Parameters:
  - `<unnamed>` (StreamingExporterTest): n/a
  - `<unnamed>` (GzipCompression): n/a

#### `TEST_F(StreamingExporterTest, GzipTypeProducesZstdMagicNumber)`
- Source: `tests/exporters/test_streaming_exporter.cpp`:386
- Brief: n/a
- Parameters:
  - `<unnamed>` (StreamingExporterTest): n/a
  - `<unnamed>` (GzipTypeProducesZstdMagicNumber): n/a

#### `TEST_F(StreamingExporterTest, IncludeFieldsFilter)`
- Source: `tests/exporters/test_streaming_exporter.cpp`:273
- Brief: n/a
- Parameters:
  - `<unnamed>` (StreamingExporterTest): n/a
  - `<unnamed>` (IncludeFieldsFilter): n/a

#### `TEST_F(StreamingExporterTest, MetricsTracked)`
- Source: `tests/exporters/test_streaming_exporter.cpp`:458
- Brief: n/a
- Parameters:
  - `<unnamed>` (StreamingExporterTest): n/a
  - `<unnamed>` (MetricsTracked): n/a

#### `TEST_F(StreamingExporterTest, ProgressCallbackInvoked)`
- Source: `tests/exporters/test_streaming_exporter.cpp`:210
- Brief: n/a
- Parameters:
  - `<unnamed>` (StreamingExporterTest): n/a
  - `<unnamed>` (ProgressCallbackInvoked): n/a

#### `TEST_F(StreamingExporterTest, SupportedFormats)`
- Source: `tests/exporters/test_streaming_exporter.cpp`:447
- Brief: n/a
- Parameters:
  - `<unnamed>` (StreamingExporterTest): n/a
  - `<unnamed>` (SupportedFormats): n/a

#### `TEST_F(StreamingExporterTest, ZstdCompression)`
- Source: `tests/exporters/test_streaming_exporter.cpp`:416
- Brief: n/a
- Parameters:
  - `<unnamed>` (StreamingExporterTest): n/a
  - `<unnamed>` (ZstdCompression): n/a

### themis::bench::errg

#### `void BM_ERRG01_CsvRowSerialize(benchmark::State &state)`
- Source: `benchmarks/exporters/bench_exporters_release_gates.cpp`:219
- Brief: ERRG-01: serializeCsvRow() for a 10-column row. GATE-ERRG-01: ≥ 1M rows/s.
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_ERRG02_ParquetRowGroupWrite(benchmark::State &state)`
- Source: `benchmarks/exporters/bench_exporters_release_gates.cpp`:248
- Brief: ERRG-02: Build and serialize a 100-row ParquetRowGroup. UseRealTime() because this involves memory allocation. GATE-ERRG-02: p99 ≤ 5 ms.
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_ERRG03_SchemaValidation(benchmark::State &state)`
- Source: `benchmarks/exporters/bench_exporters_release_gates.cpp`:279
- Brief: ERRG-03: validateSchema() for a 10-column IncomingRow. GATE-ERRG-03: p99 ≤ 100 µs.
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_ERRG04_NullHandlingDecision(benchmark::State &state)`
- Source: `benchmarks/exporters/bench_exporters_release_gates.cpp`:303
- Brief: ERRG-04: handleNull() decision for CSV/Parquet/Arrow targets. GATE-ERRG-04: p99 ≤ 10 µs.
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_ERRG05_ArrowBatchSerialize(benchmark::State &state)`
- Source: `benchmarks/exporters/bench_exporters_release_gates.cpp`:327
- Brief: ERRG-05: serializeArrowBatch() for 100 rows × 10 columns. GATE-ERRG-05: p99 ≤ 1 ms.
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_ERRG06_ExportQuotaCheck(benchmark::State &state)`
- Source: `benchmarks/exporters/bench_exporters_release_gates.cpp`:348
- Brief: ERRG-06: ExportQuota::check() — atomic load + compare. GATE-ERRG-06: p99 ≤ 50 µs.
- Parameters:
  - `state` (benchmark::State &): n/a

#### `Repetitions(kRepetitions) -> ReportAggregatesOnly(true)`
- Source: `benchmarks/exporters/bench_exporters_release_gates.cpp`:236
- Brief: n/a
- Parameters:
  - `<unnamed>` (kRepetitions): n/a

#### `UseRealTime() -> Repetitions(kRepetitions) ->ReportAggregatesOnly(true)`
- Source: `benchmarks/exporters/bench_exporters_release_gates.cpp`:267
- Brief: n/a
- Parameters: none

#### `std::string handleNull(bool is_null, NullTarget target)`
- Source: `benchmarks/exporters/bench_exporters_release_gates.cpp`:168
- Brief: n/a
- Parameters:
  - `is_null` (bool): n/a
  - `target` (NullTarget): n/a

#### `CsvRow makeCsvRow(int idx)`
- Source: `benchmarks/exporters/bench_exporters_release_gates.cpp`:81
- Brief: n/a
- Parameters:
  - `idx` (int): n/a

#### `std::string serializeArrowBatch(int n_rows, int n_cols)`
- Source: `benchmarks/exporters/bench_exporters_release_gates.cpp`:184
- Brief: n/a
- Parameters:
  - `n_rows` (int): n/a
  - `n_cols` (int): n/a

#### `std::string serializeCsvRow(const CsvRow &row)`
- Source: `benchmarks/exporters/bench_exporters_release_gates.cpp`:91
- Brief: n/a
- Parameters:
  - `row` (const CsvRow &): n/a

#### `bool validateSchema(const IncomingRow &row) noexcept`
- Source: `benchmarks/exporters/bench_exporters_release_gates.cpp`:154
- Brief: n/a
- Parameters:
  - `row` (const IncomingRow &): n/a

### themis::bench::errg::ExportQuota

#### `ExporterErrorCode check(std::uint64_t rows) noexcept`
- Source: `benchmarks/exporters/bench_exporters_release_gates.cpp`:202
- Brief: n/a
- Parameters:
  - `rows` (std::uint64_t): n/a

### themis::bench::errg::ParquetRowGroup

#### `void add(int row_idx)`
- Source: `benchmarks/exporters/bench_exporters_release_gates.cpp`:120
- Brief: n/a
- Parameters:
  - `row_idx` (int): n/a

#### `std::string serialize() const`
- Source: `benchmarks/exporters/bench_exporters_release_gates.cpp`:129
- Brief: n/a
- Parameters: none

### themis::exporters

#### `double computeToxicityScore(const std::string &text)`
- Source: `src/exporters/jsonl_llm_exporter.cpp`:758
- Brief: Heuristic toxicity score: counts hostile/offensive term occurrences and maps to [0.
- Parameters:
  - `text` (const std::string &): Input parameter.
- Return: Return value.
- Details: text Input parameter. Return value. 0, 1.0]. Returns 0.0 for benign text. 5+ hits saturates to 1.0. Markers cover both English and German to support multilingual training corpora. Calls: std::transform(), begin(), end(), find(), size(), std::min().

#### `void enforceExportPolicy(const ExportOptions &options)`
- Source: `src/exporters/jsonl_llm_exporter.cpp`:64
- Brief: Enforce Export Policy.
- Parameters:
  - `options` (const ExportOptions &): Input parameter.
- Throws:
  - ExporterException: if an error occurs.
- Details: options Input parameter. options Input parameter. ExporterException if an error occurs. Calls: empty(), checkExportPermission(), logSecurityEvent(), THEMIS_WARN().

#### `bool isResumableError(ExporterErrorCode code) noexcept`
- Source: `include/exporters/exporters_api_contract.h`:152
- Brief: n/a
- Parameters:
  - `code` (ExporterErrorCode): n/a
- Details: Returns true for errors from which an export can be resumed at the last successfully acknowledged chunk.

#### `bool isTransientError(ExporterErrorCode code) noexcept`
- Source: `include/exporters/exporters_api_contract.h`:157
- Brief: Returns true for errors that should be retried with the same parameters.
- Parameters:
  - `code` (ExporterErrorCode): n/a

#### `std::unique_ptr< IFormatTemplate > makeFormatTemplate(FormatTemplateType type)`
- Source: `src/exporters/format_template.cpp`:212
- Brief: Make Format Template.
- Parameters:
  - `type` (FormatTemplateType): Input parameter.
- Return: Return value.
- Details: type Input parameter. Return value. type Input parameter. Return value. Implements makeFormatTemplate without additional internal calls.

#### `std::string readString(const uint8_t *buf, size_t buf_size, size_t &offset)`
- Source: `src/exporters/export_encryption.cpp`:128
- Brief: Read String.
- Parameters:
  - `buf` (const uint8_t *): Input parameter.
  - `buf_size` (size_t): Input parameter.
  - `offset` (size_t &): Input/output parameter.
- Return: Return value.
- Throws:
  - std::runtime_error: if an error occurs.
- Details: buf Input parameter. buf_size Input parameter. offset Input/output parameter. Return value. std::runtime_error if an error occurs. Calls: readU32(), std::to_string(), s().

#### `bool readU16LE(std::istream &in, uint16_t &v)`
- Source: `src/exporters/export_encryption.cpp`:782
- Brief: Read U16 LE.
- Parameters:
  - `in` (std::istream &): Input/output parameter.
  - `v` (uint16_t &): Input/output parameter.
- Return: True when the operation succeeds.
- Details: in Input/output parameter. v Input/output parameter. True when the operation succeeds. Calls: get().

#### `uint32_t readU32(const uint8_t *p)`
- Source: `src/exporters/export_encryption.cpp`:95
- Brief: Read U32.
- Parameters:
  - `p` (const uint8_t *): Input parameter.
- Return: Return value.
- Details: p Input parameter. Return value. Implements readU32 without additional internal calls.

#### `bool readU32LE(std::istream &in, uint32_t &v)`
- Source: `src/exporters/export_encryption.cpp`:800
- Brief: Read U32 LE.
- Parameters:
  - `in` (std::istream &): Input/output parameter.
  - `v` (uint32_t &): Input/output parameter.
- Return: True when the operation succeeds.
- Details: in Input/output parameter. v Input/output parameter. True when the operation succeeds. Calls: get().

#### `uint64_t readU64(const uint8_t *p)`
- Source: `src/exporters/export_encryption.cpp`:107
- Brief: Read U64.
- Parameters:
  - `p` (const uint8_t *): Input parameter.
- Return: Return value.
- Details: p Input parameter. Return value. Implements readU64 without additional internal calls.

#### `bool readU8(std::istream &in, uint8_t &v)`
- Source: `src/exporters/export_encryption.cpp`:766
- Brief: Read U8.
- Parameters:
  - `in` (std::istream &): Input/output parameter.
  - `v` (uint8_t &): Input/output parameter.
- Return: True when the operation succeeds.
- Details: in Input/output parameter. v Input/output parameter. True when the operation succeeds. Calls: get().

#### `TemplateValidationResult validateTemplate(FormatTemplateType type, const FormatTemplateFieldMapping &mapping, const std::vector< BaseEntity > &sample)`
- Source: `src/exporters/format_template.cpp`:239
- Brief: Validate Template.
- Parameters:
  - `type` (FormatTemplateType): Input parameter.
  - `mapping` (const FormatTemplateFieldMapping &): Input parameter.
  - `sample` (const std::vector< BaseEntity > &): Input parameter.
- Return: Return value.
- Details: type Input parameter. mapping Input parameter. sample Input parameter. Return value. type Input parameter. mapping Input parameter. sample Input parameter. Return value. Calls: makeFormatTemplate(), validateFields(), insert(), assign(), begin(), end().

#### `void writeBytes(std::vector< uint8_t > &buf, const uint8_t *data, size_t len)`
- Source: `src/exporters/export_encryption.cpp`:73
- Brief: Write Bytes.
- Parameters:
  - `buf` (std::vector< uint8_t > &): Input/output parameter.
  - `data` (const uint8_t *): Input parameter.
  - `len` (size_t): Input parameter.
- Details: buf Input/output parameter. data Input parameter. len Input parameter. Calls: insert(), end().

#### `void writeHubUploadAuditEntry(themis::utils::AuditLogger &audit_log, const HubUploadConfig &config, const std::string &dataset_dir, const HubUploadResult &result, const std::string &outcome)`
- Source: `src/exporters/huggingface_hub_client.cpp`:431
- Brief: ── Main upload ──────────────────────────────────────────────────────────────
- Parameters:
  - `audit_log` (themis::utils::AuditLogger &): Input/output parameter.
  - `config` (const HubUploadConfig &): Input parameter.
  - `dataset_dir` (const std::string &): Input parameter.
  - `result` (const HubUploadResult &): Input parameter.
  - `outcome` (const std::string &): Input parameter.
- Details: audit_log Input/output parameter. config Input parameter. dataset_dir Input parameter. result Input parameter. outcome Input parameter. Calls: std::chrono::system_clock::now(), time_since_epoch(), count(), logEvent().

#### `void writeString(std::vector< uint8_t > &buf, const std::string &s)`
- Source: `src/exporters/export_encryption.cpp`:84
- Brief: Write String.
- Parameters:
  - `buf` (std::vector< uint8_t > &): Input/output parameter.
  - `s` (const std::string &): Input parameter.
- Details: buf Input/output parameter. s Input parameter. Calls: writeU32(), size(), writeBytes(), data().

#### `void writeU16LE(std::ostream &out, uint16_t v)`
- Source: `src/exporters/export_encryption.cpp`:741
- Brief: Write U16 LE.
- Parameters:
  - `out` (std::ostream &): Input/output parameter.
  - `v` (uint16_t): Input parameter.
- Details: out Input/output parameter. v Input parameter. Calls: put().

#### `void writeU32(std::vector< uint8_t > &buf, uint32_t v)`
- Source: `src/exporters/export_encryption.cpp`:47
- Brief: ───────────────────────────────────────────────────────────────────────────── Little-endian I/O helpers ─────────────────────────────────────────────────────────────────────────────
- Parameters:
  - `buf` (std::vector< uint8_t > &): Input/output parameter.
  - `v` (uint32_t): Input parameter.
- Details: buf Input/output parameter. v Input parameter. Calls: push_back().

#### `void writeU32LE(std::ostream &out, uint32_t v)`
- Source: `src/exporters/export_encryption.cpp`:752
- Brief: Write U32 LE.
- Parameters:
  - `out` (std::ostream &): Input/output parameter.
  - `v` (uint32_t): Input parameter.
- Details: out Input/output parameter. v Input parameter. Calls: put().

#### `void writeU64(std::vector< uint8_t > &buf, uint64_t v)`
- Source: `src/exporters/export_encryption.cpp`:60
- Brief: Write U64.
- Parameters:
  - `buf` (std::vector< uint8_t > &): Input/output parameter.
  - `v` (uint64_t): Input parameter.
- Details: buf Input/output parameter. v Input parameter. Calls: push_back().

#### `void writeU8(std::ostream &out, uint8_t v)`
- Source: `src/exporters/export_encryption.cpp`:731
- Brief: Helper: little-endian binary I/O ─────────────────────────────────────────────────────────────────────────────
- Parameters:
  - `out` (std::ostream &): Input/output parameter.
  - `v` (uint8_t): Input parameter.
- Details: out Input/output parameter. v Input parameter. Calls: put().

#### `std::string yamlQuote(const std::string &s)`
- Source: `src/exporters/huggingface_exporter.cpp`:204
- Brief: ------------------------------------------------------------------------ README.
- Parameters:
  - `s` (const std::string &): Input parameter.
- Return: Return value.
- Details: s Input parameter. Return value. md (dataset card) generation ------------------------------------------------------------------------ Calls: reserve(), size().

### themis::exporters::AlpacaTemplate

#### `std::string name() const override`
- Source: `include/exporters/format_template.h`:77
- Brief: Name.
- Parameters: none
- Return: Return value.
- Details: Return value.

#### `std::string render(const BaseEntity &entity, const FormatTemplateFieldMapping &mapping) const override`
- Source: `include/exporters/format_template.h`:85
- Brief: Render.
- Parameters:
  - `entity` (const BaseEntity &): Input parameter.
  - `mapping` (const FormatTemplateFieldMapping &): Input parameter.
- Return: Return value.
- Details: entity Input parameter. mapping Input parameter. Return value.

#### `bool validateFields(const BaseEntity &entity, const FormatTemplateFieldMapping &mapping, std::vector< std::string > *missing_fields=nullptr) const override`
- Source: `include/exporters/format_template.h`:79
- Brief: n/a
- Parameters:
  - `entity` (const BaseEntity &): n/a
  - `mapping` (const FormatTemplateFieldMapping &): n/a
  - `missing_fields` (std::vector< std::string > *): n/a

### themis::exporters::AqlPredicateFilter

#### `AqlPredicateFilter(AqlPredicateFilter &&) noexcept=default`
- Source: `include/exporters/aql_predicate_filter.h`:53
- Brief: n/a
- Parameters:
  - `<unnamed>` (AqlPredicateFilter &&): n/a

#### `AqlPredicateFilter(const AqlPredicateFilter &)=delete`
- Source: `include/exporters/aql_predicate_filter.h`:51
- Brief: n/a
- Parameters:
  - `<unnamed>` (const AqlPredicateFilter &): n/a

#### `AqlPredicateFilter(const std::string &predicate)`
- Source: `include/exporters/aql_predicate_filter.h`:47
- Brief: Aql Predicate Filter.
- Parameters:
  - `predicate` (const std::string &): Input parameter.
- Return: Return value.
- Details: predicate Input parameter. Return value.

#### `bool evaluate(const BaseEntity &entity) const`
- Source: `include/exporters/aql_predicate_filter.h`:61
- Brief: Evaluate.
- Parameters:
  - `entity` (const BaseEntity &): Input parameter.
- Return: True when the operation succeeds.
- Details: entity Input parameter. True when the operation succeeds.

#### `const std::string & getPredicate() const`
- Source: `include/exporters/aql_predicate_filter.h`:63
- Brief: n/a
- Parameters: none

#### `AqlPredicateFilter & operator=(AqlPredicateFilter &&) noexcept=default`
- Source: `include/exporters/aql_predicate_filter.h`:54
- Brief: n/a
- Parameters:
  - `<unnamed>` (AqlPredicateFilter &&): n/a

#### `AqlPredicateFilter & operator=(const AqlPredicateFilter &)=delete`
- Source: `include/exporters/aql_predicate_filter.h`:52
- Brief: n/a
- Parameters:
  - `<unnamed>` (const AqlPredicateFilter &): n/a

#### `~AqlPredicateFilter()`
- Source: `include/exporters/aql_predicate_filter.h`:49
- Brief: n/a
- Parameters: none

### themis::exporters::AqlPredicateFilterException

#### `AqlPredicateFilterException(const std::string &msg)`
- Source: `include/exporters/aql_predicate_filter.h`:36
- Brief: Aql Predicate Filter Exception.
- Parameters:
  - `msg` (const std::string &): Input parameter.
- Return: Return value.
- Details: msg Input parameter. Return value.

### themis::exporters::ArrowIPCExporter

#### `ArrowIPCExporter(const ArrowIPCExportConfig &config={})`
- Source: `include/exporters/arrow_ipc_exporter.h`:46
- Brief: n/a
- Parameters:
  - `config` (const ArrowIPCExportConfig &): n/a

#### `ExportStats exportEntities(const std::vector< BaseEntity > &entities, const ExportOptions &options) override`
- Source: `include/exporters/arrow_ipc_exporter.h`:49
- Brief: Export Entities.
- Parameters:
  - `entities` (const std::vector< BaseEntity > &): Input parameter.
  - `options` (const ExportOptions &): Input parameter.
- Return: Return value.
- Throws:
  - ExporterException: if an error occurs.
  - ConfigException: if an error occurs.
  - ExportIOException: if an error occurs.
- Details: entities Input parameter. options Input parameter. Return value. ExporterException if an error occurs. ConfigException if an error occurs. ExportIOException if an error occurs. Calls: enforceExportPolicy(), std::chrono::steady_clock::now(), hasScope(), empty(), size(), resolveColumns(), exportWithArrow(), exportFallback().

#### `ExportStats exportFallback(const std::vector< BaseEntity > &entities, const ExportOptions &options, const std::vector< std::string > &columns)`
- Source: `include/exporters/arrow_ipc_exporter.h`:119
- Brief: Export Fallback.
- Parameters:
  - `entities` (const std::vector< BaseEntity > &): Input parameter.
  - `options` (const ExportOptions &): Input parameter.
  - `columns` (const std::vector< std::string > &): Input parameter.
- Return: Return value.
- Throws:
  - ExportIOException: if an error occurs.
- Details: ───────────────────────────────────────────────────────────────────────────── Fallback: minimal Arrow IPC File writer (no Arrow library required) ───────────────────────────────────────────────────────────────────────────── entities Input parameter. options Input parameter. columns Input parameter. Return value. entities Input parameter. options Input parameter. columns Input parameter. Return value. ExportIOException if an error occurs. Calls: size(), out(), is_open(), buildSchemaMessage(), empty(), buildBatchBody(), buildRecordBatchMessage(), write().

#### `const ArrowIPCExportConfig & getConfig() const`
- Source: `include/exporters/arrow_ipc_exporter.h`:66
- Brief: n/a
- Parameters: none

#### `std::shared_ptr< ExporterMetrics > getMetrics() const`
- Source: `include/exporters/arrow_ipc_exporter.h`:68
- Brief: n/a
- Parameters: none

#### `std::string getName() const override`
- Source: `include/exporters/arrow_ipc_exporter.h`:57
- Brief: n/a
- Parameters: none

#### `std::vector< std::string > getSupportedFormats() const override`
- Source: `include/exporters/arrow_ipc_exporter.h`:54
- Brief: n/a
- Parameters: none

#### `std::string getVersion() const override`
- Source: `include/exporters/arrow_ipc_exporter.h`:58
- Brief: n/a
- Parameters: none

#### `bool isArrowAvailable()`
- Source: `include/exporters/arrow_ipc_exporter.h`:80
- Brief: Is Arrow Available.
- Parameters: none
- Return: True when the operation succeeds.
- Details: True when the operation succeeds. True when the operation succeeds. Implements isArrowAvailable without additional internal calls.

#### `void resetMetrics()`
- Source: `include/exporters/arrow_ipc_exporter.h`:74
- Brief: Reset Metrics.
- Parameters: none
- Details: Calls: reset().

#### `std::vector< std::string > resolveColumns(const std::vector< BaseEntity > &entities, const ExportOptions &options) const`
- Source: `include/exporters/arrow_ipc_exporter.h`:92
- Brief: Resolve Columns.
- Parameters:
  - `entities` (const std::vector< BaseEntity > &): Input parameter.
  - `options` (const ExportOptions &): Input parameter.
- Return: Return value.
- Details: entities Input parameter. options Input parameter. Return value.

#### `void setConfig(const ArrowIPCExportConfig &config)`
- Source: `include/exporters/arrow_ipc_exporter.h`:65
- Brief: Set Config.
- Parameters:
  - `config` (const ArrowIPCExportConfig &): Input parameter.
- Details: config Input parameter. Implements setConfig without additional internal calls.

### themis::exporters::ChatMLTemplate

#### `std::string name() const override`
- Source: `include/exporters/format_template.h`:109
- Brief: Name.
- Parameters: none
- Return: Return value.
- Details: Return value.

#### `std::string render(const BaseEntity &entity, const FormatTemplateFieldMapping &mapping) const override`
- Source: `include/exporters/format_template.h`:117
- Brief: Render.
- Parameters:
  - `entity` (const BaseEntity &): Input parameter.
  - `mapping` (const FormatTemplateFieldMapping &): Input parameter.
- Return: Return value.
- Details: entity Input parameter. mapping Input parameter. Return value.

#### `bool validateFields(const BaseEntity &entity, const FormatTemplateFieldMapping &mapping, std::vector< std::string > *missing_fields=nullptr) const override`
- Source: `include/exporters/format_template.h`:111
- Brief: n/a
- Parameters:
  - `entity` (const BaseEntity &): n/a
  - `mapping` (const FormatTemplateFieldMapping &): n/a
  - `missing_fields` (std::vector< std::string > *): n/a

### themis::exporters::ConfigException

#### `ConfigException(const std::string &message, const std::string &config_key="")`
- Source: `include/exporters/exporter_errors.h`:154
- Brief: n/a
- Parameters:
  - `message` (const std::string &): n/a
  - `config_key` (const std::string &): n/a

#### `const std::string & getConfigKey() const`
- Source: `include/exporters/exporter_errors.h`:164
- Brief: n/a
- Parameters: none

### themis::exporters::DataAugmentationPipeline

#### `DataAugmentationPipeline(const AugmentationConfig &config={})`
- Source: `include/exporters/data_augmentation.h`:64
- Brief: n/a
- Parameters:
  - `config` (const AugmentationConfig &): n/a

#### `BaseEntity applyLowercase(const BaseEntity &entity) const`
- Source: `include/exporters/data_augmentation.h`:121
- Brief: Apply Lowercase.
- Parameters:
  - `entity` (const BaseEntity &): Input parameter.
- Return: Return value.
- Details: entity Input parameter. Return value.

#### `BaseEntity applyQuestionReformulation(const BaseEntity &entity, uint32_t variant) const`
- Source: `include/exporters/data_augmentation.h`:109
- Brief: Apply Question Reformulation.
- Parameters:
  - `entity` (const BaseEntity &): Input parameter.
  - `variant` (uint32_t): Input parameter.
- Return: Return value.
- Details: entity Input parameter. variant Input parameter. Return value.

#### `BaseEntity applySentenceCasing(const BaseEntity &entity) const`
- Source: `include/exporters/data_augmentation.h`:127
- Brief: Apply Sentence Casing.
- Parameters:
  - `entity` (const BaseEntity &): Input parameter.
- Return: Return value.
- Details: entity Input parameter. Return value.

#### `std::vector< BaseEntity > applyStrategy(const BaseEntity &entity, AugmentationStrategy strategy, uint32_t count) const`
- Source: `include/exporters/data_augmentation.h`:78
- Brief: Apply Strategy.
- Parameters:
  - `entity` (const BaseEntity &): Input parameter.
  - `strategy` (AugmentationStrategy): Input parameter.
  - `count` (uint32_t): Input parameter.
- Return: Return value.
- Details: entity Input parameter. strategy Input parameter. count Input parameter. Return value.

#### `BaseEntity applySynonymReplacement(const BaseEntity &entity, uint32_t variant) const`
- Source: `include/exporters/data_augmentation.h`:102
- Brief: Apply Synonym Replacement.
- Parameters:
  - `entity` (const BaseEntity &): Input parameter.
  - `variant` (uint32_t): Input parameter.
- Return: Return value.
- Details: entity Input parameter. variant Input parameter. Return value.

#### `BaseEntity applyWhitespaceNormalization(const BaseEntity &entity) const`
- Source: `include/exporters/data_augmentation.h`:115
- Brief: Apply Whitespace Normalization.
- Parameters:
  - `entity` (const BaseEntity &): Input parameter.
- Return: Return value.
- Details: entity Input parameter. Return value.

#### `std::vector< BaseEntity > augment(const std::vector< BaseEntity > &entities, AugmentationStats *stats=nullptr) const`
- Source: `include/exporters/data_augmentation.h`:66
- Brief: n/a
- Parameters:
  - `entities` (const std::vector< BaseEntity > &): n/a
  - `stats` (AugmentationStats *): n/a

#### `const std::map< std::string, std::vector< std::string > > & builtinSynonyms()`
- Source: `include/exporters/data_augmentation.h`:176
- Brief: n/a
- Parameters: none

#### `const AugmentationConfig & getConfig() const`
- Source: `include/exporters/data_augmentation.h`:84
- Brief: n/a
- Parameters: none

#### `std::string normalizeWhitespace(const std::string &text)`
- Source: `include/exporters/data_augmentation.h`:158
- Brief: Normalize Whitespace.
- Parameters:
  - `text` (const std::string &): Input parameter.
- Return: Return value.
- Details: text Input parameter. Return value.

#### `std::string reformulateQuestion(const std::string &text, uint32_t variant)`
- Source: `include/exporters/data_augmentation.h`:151
- Brief: Reformulate Question.
- Parameters:
  - `text` (const std::string &): Input parameter.
  - `variant` (uint32_t): Input parameter.
- Return: Return value.
- Details: text Input parameter. variant Input parameter. Return value.

#### `std::string replaceSynonyms(const std::string &text, uint32_t variant) const`
- Source: `include/exporters/data_augmentation.h`:143
- Brief: Replace Synonyms.
- Parameters:
  - `text` (const std::string &): Input parameter.
  - `variant` (uint32_t): Input parameter.
- Return: Return value.
- Details: text Input parameter. variant Input parameter. Return value.

#### `std::vector< std::string > selectFields(const BaseEntity &entity) const`
- Source: `include/exporters/data_augmentation.h`:135
- Brief: Select Fields.
- Parameters:
  - `entity` (const BaseEntity &): Input parameter.
- Return: Return value.
- Details: entity Input parameter. Return value.

#### `void setConfig(const AugmentationConfig &config)`
- Source: `include/exporters/data_augmentation.h`:90
- Brief: Set Config.
- Parameters:
  - `config` (const AugmentationConfig &): Input parameter.
- Details: config Input parameter. Implements setConfig without additional internal calls.

#### `std::string toLowercase(const std::string &text)`
- Source: `include/exporters/data_augmentation.h`:165
- Brief: To Lowercase.
- Parameters:
  - `text` (const std::string &): Input parameter.
- Return: Return value.
- Details: text Input parameter. Return value.

#### `std::string toSentenceCasing(const std::string &text)`
- Source: `include/exporters/data_augmentation.h`:172
- Brief: To Sentence Casing.
- Parameters:
  - `text` (const std::string &): Input parameter.
- Return: Return value.
- Details: text Input parameter. Return value.

### themis::exporters::ExportCursor

#### `size_t currentOffset() const =0`
- Source: `include/exporters/streaming_exporter.h`:37
- Brief: n/a
- Parameters: none

#### `bool hasNext() const =0`
- Source: `include/exporters/streaming_exporter.h`:31
- Brief: n/a
- Parameters: none

#### `std::vector< BaseEntity > nextPage()=0`
- Source: `include/exporters/streaming_exporter.h`:33
- Brief: n/a
- Parameters: none

#### `bool seekTo(size_t offset)`
- Source: `include/exporters/streaming_exporter.h`:39
- Brief: n/a
- Parameters:
  - `offset` (size_t): n/a

#### `size_t totalCount() const`
- Source: `include/exporters/streaming_exporter.h`:35
- Brief: n/a
- Parameters: none

#### `~ExportCursor()=default`
- Source: `include/exporters/streaming_exporter.h`:29
- Brief: Export Cursor.
- Parameters: none
- Return: Return value.
- Details: Return value.

### themis::exporters::ExportEncryption

#### `ExportEncryption(const ExportEncryptionConfig &config)`
- Source: `include/exporters/export_encryption.h`:45
- Brief: Export Encryption.
- Parameters:
  - `config` (const ExportEncryptionConfig &): Input parameter.
- Return: Return value.
- Details: config Input parameter. Return value.

#### `std::vector< uint8_t > buildAAD(const std::string &job_id, const std::string &kek_id, uint32_t key_version, const std::vector< uint8_t > &iv)`
- Source: `include/exporters/export_encryption.h`:96
- Brief: Build AAD.
- Parameters:
  - `job_id` (const std::string &): Identifier of the job.
  - `kek_id` (const std::string &): Identifier of the kek.
  - `key_version` (uint32_t): Input parameter.
  - `iv` (const std::vector< uint8_t > &): Input parameter.
- Return: Return value.
- Details: job_id Identifier of the job. kek_id Identifier of the kek. key_version Input parameter. iv Input parameter. Return value.

#### `std::vector< uint8_t > decrypt(const std::vector< uint8_t > &container) const`
- Source: `include/exporters/export_encryption.h`:74
- Brief: Decrypt.
- Parameters:
  - `container` (const std::vector< uint8_t > &): Input parameter.
- Return: Return value.
- Details: container Input parameter. Return value.

#### `void decryptFile(const std::string &src_path, const std::string &dst_path) const`
- Source: `include/exporters/export_encryption.h`:60
- Brief: Decrypt File.
- Parameters:
  - `src_path` (const std::string &): Path to the src.
  - `dst_path` (const std::string &): Path to the dst.
- Details: src_path Path to the src. dst_path Path to the dst.

#### `std::vector< uint8_t > deriveJobDEK(uint32_t key_version) const`
- Source: `include/exporters/export_encryption.h`:86
- Brief: Derive Job DEK.
- Parameters:
  - `key_version` (uint32_t): Input parameter.
- Return: Return value.
- Details: key_version Input parameter. Return value.

#### `std::vector< uint8_t > encrypt(const std::vector< uint8_t > &plaintext) const`
- Source: `include/exporters/export_encryption.h`:68
- Brief: Encrypt.
- Parameters:
  - `plaintext` (const std::vector< uint8_t > &): Input parameter.
- Return: Return value.
- Details: plaintext Input parameter. Return value.

#### `void encryptFile(const std::string &src_path, const std::string &dst_path) const`
- Source: `include/exporters/export_encryption.h`:52
- Brief: Encrypt File.
- Parameters:
  - `src_path` (const std::string &): Path to the src.
  - `dst_path` (const std::string &): Path to the dst.
- Details: src_path Path to the src. dst_path Path to the dst.

### themis::exporters::ExportEncryptionConfig

#### `bool empty() const`
- Source: `include/exporters/export_encryption.h`:35
- Brief: n/a
- Parameters: none

### themis::exporters::ExportEncryptor

#### `ExportEncryptor(const ExportEncryptionConfig &config)`
- Source: `include/exporters/export_encryption.h`:113
- Brief: Export Encryptor.
- Parameters:
  - `config` (const ExportEncryptionConfig &): Input parameter.
- Return: Return value.
- Details: config Input parameter. Return value.

#### `size_t decryptFile(const std::string &input_path, const std::string &output_path) const`
- Source: `include/exporters/export_encryption.h`:130
- Brief: Decrypt File.
- Parameters:
  - `input_path` (const std::string &): Path to the input.
  - `output_path` (const std::string &): Path to the output.
- Return: Return value.
- Details: input_path Path to the input. output_path Path to the output. Return value.

#### `std::vector< uint8_t > deriveDataKey(const std::vector< uint8_t > &kek, const std::string &job_id)`
- Source: `include/exporters/export_encryption.h`:146
- Brief: Derive Data Key.
- Parameters:
  - `kek` (const std::vector< uint8_t > &): Input parameter.
  - `job_id` (const std::string &): Identifier of the job.
- Return: Return value.
- Details: kek Input parameter. job_id Identifier of the job. Return value.

#### `size_t encryptFile(const std::string &input_path, const std::string &output_path) const`
- Source: `include/exporters/export_encryption.h`:121
- Brief: Encrypt File.
- Parameters:
  - `input_path` (const std::string &): Path to the input.
  - `output_path` (const std::string &): Path to the output.
- Return: Return value.
- Details: input_path Path to the input. output_path Path to the output. Return value.

#### `std::string generateJobId()`
- Source: `include/exporters/export_encryption.h`:153
- Brief: Generate Job Id.
- Parameters: none
- Return: Return value.
- Throws:
  - EncryptionException: if an error occurs.
- Details: Return value. Return value. EncryptionException if an error occurs. Calls: RAND_bytes(), std::setfill(), std::setw(), str().

#### `const ExportEncryptionConfig & getConfig() const`
- Source: `include/exporters/export_encryption.h`:133
- Brief: n/a
- Parameters: none

#### `bool readHeader(std::istream &in, std::string &kek_id, uint32_t &kek_version, std::string &job_id, std::vector< uint8_t > &iv)`
- Source: `include/exporters/export_encryption.h`:179
- Brief: Read Header.
- Parameters:
  - `in` (std::istream &): Input/output parameter.
  - `kek_id` (std::string &): Identifier of the kek.
  - `kek_version` (uint32_t &): Input/output parameter.
  - `job_id` (std::string &): Identifier of the job.
  - `iv` (std::vector< uint8_t > &): Input/output parameter.
- Return: True when the operation succeeds.
- Details: in Input/output parameter. kek_id Identifier of the kek. kek_version Input/output parameter. job_id Identifier of the job. iv Input/output parameter. True when the operation succeeds. in Input/output parameter. kek_id Identifier of the kek. kek_version Input/output parameter. job_id Identifier of the job. iv Input/output parameter. True when the operation succeeds. Calls: read(), gcount(), std::memcmp(), readU8(), readU16LE(), resize(), data(), readU32LE().

#### `size_t writeHeader(std::ostream &out, const std::string &kek_id, uint32_t kek_version, const std::string &job_id, const std::vector< uint8_t > &iv)`
- Source: `include/exporters/export_encryption.h`:164
- Brief: Write Header.
- Parameters:
  - `out` (std::ostream &): Input/output parameter.
  - `kek_id` (const std::string &): Identifier of the kek.
  - `kek_version` (uint32_t): Input parameter.
  - `job_id` (const std::string &): Identifier of the job.
  - `iv` (const std::vector< uint8_t > &): Input parameter.
- Return: Return value.
- Details: out Input/output parameter. kek_id Identifier of the kek. kek_version Input parameter. job_id Identifier of the job. iv Input parameter. Return value. out Input/output parameter. kek_id Identifier of the kek. kek_version Input parameter. job_id Identifier of the job. iv Input parameter. Return value. Calls: write(), writeU8(), size(), writeU16LE(), data(), writeU32LE().

### themis::exporters::ExportFormatRegistry

#### `ExportFormatRegistry()=default`
- Source: `include/exporters/export_format_registry.h`:85
- Brief: n/a
- Parameters: none

#### `ExportFormatRegistry(const ExportFormatRegistry &)=delete`
- Source: `include/exporters/export_format_registry.h`:87
- Brief: n/a
- Parameters:
  - `<unnamed>` (const ExportFormatRegistry &): n/a

#### `void clear()`
- Source: `include/exporters/export_format_registry.h`:82
- Brief: Clear.
- Parameters: none
- Details: Calls: lock().

#### `std::unique_ptr< IExporter > createExporter(const std::string &format_key) const`
- Source: `include/exporters/export_format_registry.h`:47
- Brief: Create Exporter.
- Parameters:
  - `format_key` (const std::string &): Input parameter.
- Return: Return value.
- Details: format_key Input parameter. Return value.

#### `bool hasFormat(const std::string &format_key) const`
- Source: `include/exporters/export_format_registry.h`:54
- Brief: Has Format.
- Parameters:
  - `format_key` (const std::string &): Input parameter.
- Return: True when the operation succeeds.
- Details: format_key Input parameter. True when the operation succeeds.

#### `ExportFormatRegistry & instance()`
- Source: `include/exporters/export_format_registry.h`:33
- Brief: Instance.
- Parameters: none
- Return: Return value.
- Details: Return value.

#### `void loadTemplatesFromConfig(const std::string &config_path)`
- Source: `include/exporters/export_format_registry.h`:71
- Brief: Load Templates From Config.
- Parameters:
  - `config_path` (const std::string &): Path to the retention policy configuration file.
- Throws:
  - std::runtime_error: if an error occurs.
- Details: config_path Path to the retention policy configuration file. config_path Path to the retention policy configuration file. std::runtime_error if an error occurs. Calls: f(), is_open(), content(), loadTemplatesFromJson().

#### `void loadTemplatesFromJson(const std::string &json_str)`
- Source: `include/exporters/export_format_registry.h`:77
- Brief: Load Templates From Json.
- Parameters:
  - `json_str` (const std::string &): Input parameter.
- Throws:
  - std::invalid_argument: if an error occurs.
- Details: json_str Input parameter. json_str Input parameter. std::invalid_argument if an error occurs. Calls: nlohmann::json::parse(), contains(), is_array(), reserve(), size(), is_string(), find(), end().

#### `ExportFormatRegistry & operator=(const ExportFormatRegistry &)=delete`
- Source: `include/exporters/export_format_registry.h`:88
- Brief: n/a
- Parameters:
  - `<unnamed>` (const ExportFormatRegistry &): n/a

#### `void registerBuiltins()`
- Source: `include/exporters/export_format_registry.h`:65
- Brief: Register Builtins.
- Parameters: none
- Details: Calls: registerFormat().

#### `void registerFormat(const std::string &format_key, Factory factory)`
- Source: `include/exporters/export_format_registry.h`:40
- Brief: Register Format.
- Parameters:
  - `format_key` (const std::string &): Input parameter.
  - `factory` (Factory): Input parameter.
- Details: format_key Input parameter. factory Input parameter. format_key Input parameter. factory Input parameter. Calls: lock(), std::move().

#### `std::vector< std::string > registeredFormats() const`
- Source: `include/exporters/export_format_registry.h`:60
- Brief: Registered Formats.
- Parameters: none
- Return: Return value.
- Details: Return value.

#### `~ExportFormatRegistry()=default`
- Source: `include/exporters/export_format_registry.h`:86
- Brief: n/a
- Parameters: none

### themis::exporters::ExportIOException

#### `ExportIOException(const std::string &message, const std::string &file_path="", int errno_value=0)`
- Source: `include/exporters/exporter_errors.h`:65
- Brief: n/a
- Parameters:
  - `message` (const std::string &): n/a
  - `file_path` (const std::string &): n/a
  - `errno_value` (int): n/a

#### `int getErrno() const`
- Source: `include/exporters/exporter_errors.h`:78
- Brief: n/a
- Parameters: none

#### `const std::string & getFilePath() const`
- Source: `include/exporters/exporter_errors.h`:77
- Brief: n/a
- Parameters: none

### themis::exporters::ExportStats

#### `std::string toJson() const`
- Source: `include/exporters/exporter_interface.h`:58
- Brief: To Json.
- Parameters: none
- Return: Return value.
- Details: Return value.

### themis::exporters::ExportTenantContext

#### `bool hasScope(const std::string &scope) const`
- Source: `include/exporters/exporter_interface.h`:67
- Brief: n/a
- Parameters:
  - `scope` (const std::string &): n/a

### themis::exporters::ExporterException

#### `ExporterException(errors::ErrorCode code, const std::string &message, const std::string &context="")`
- Source: `include/exporters/exporter_errors.h`:23
- Brief: n/a
- Parameters:
  - `code` (errors::ErrorCode): n/a
  - `message` (const std::string &): n/a
  - `context` (const std::string &): n/a

#### `const std::string & getContext() const`
- Source: `include/exporters/exporter_errors.h`:32
- Brief: n/a
- Parameters: none

#### `errors::ErrorCode getErrorCode() const`
- Source: `include/exporters/exporter_errors.h`:31
- Brief: n/a
- Parameters: none

### themis::exporters::ExporterMetrics

#### `ExporterMetrics()=default`
- Source: `include/exporters/exporter_metrics.h`:25
- Brief: n/a
- Parameters: none

#### `double calculatePercentile(double percentile) const`
- Source: `include/exporters/exporter_metrics.h`:342
- Brief: Calculate Percentile.
- Parameters:
  - `percentile` (double): Input parameter.
- Return: Return value.
- Details: percentile Input parameter. Return value.

#### `double getAverageLatency() const`
- Source: `include/exporters/exporter_metrics.h`:80
- Brief: Get Average Latency.
- Parameters: none
- Return: Return value.
- Details: Return value.

#### `size_t getCheckpointCount() const`
- Source: `include/exporters/exporter_metrics.h`:177
- Brief: Get Checkpoint Count.
- Parameters: none
- Return: Return value.
- Details: Return value.

#### `double getCompressionRatio() const`
- Source: `include/exporters/exporter_metrics.h`:154
- Brief: Get Compression Ratio.
- Parameters: none
- Return: Return value.
- Details: Return value.

#### `size_t getDeltaDocsSkipped() const`
- Source: `include/exporters/exporter_metrics.h`:185
- Brief: Get Delta Docs Skipped.
- Parameters: none
- Return: Return value.
- Details: Return value.

#### `size_t getEncryptedBytesWritten() const`
- Source: `include/exporters/exporter_metrics.h`:215
- Brief: Get Encrypted Bytes Written.
- Parameters: none
- Return: Return value.
- Details: Return value.

#### `size_t getEncryptedOutputBytes() const`
- Source: `include/exporters/exporter_metrics.h`:204
- Brief: Get Encrypted Output Bytes.
- Parameters: none
- Return: Return value.
- Details: Return value.

#### `size_t getEncryptedPlaintextBytes() const`
- Source: `include/exporters/exporter_metrics.h`:198
- Brief: Get Encrypted Plaintext Bytes.
- Parameters: none
- Return: Return value.
- Details: Return value.

#### `std::map< std::string, size_t > getErrorsByType() const`
- Source: `include/exporters/exporter_metrics.h`:106
- Brief: n/a
- Parameters: none

#### `double getExportRate() const`
- Source: `include/exporters/exporter_metrics.h`:68
- Brief: Get Export Rate.
- Parameters: none
- Return: Return value.
- Details: Return value.

#### `size_t getHubUploadFailures() const`
- Source: `include/exporters/exporter_metrics.h`:251
- Brief: Get Hub Upload Failures.
- Parameters: none
- Return: Return value.
- Details: Return value.

#### `double getP50Latency() const`
- Source: `include/exporters/exporter_metrics.h`:86
- Brief: Get P50 Latency.
- Parameters: none
- Return: Return value.
- Details: Return value.

#### `double getP95Latency() const`
- Source: `include/exporters/exporter_metrics.h`:92
- Brief: Get P95 Latency.
- Parameters: none
- Return: Return value.
- Details: Return value.

#### `double getP99Latency() const`
- Source: `include/exporters/exporter_metrics.h`:98
- Brief: Get P99 Latency.
- Parameters: none
- Return: Return value.
- Details: Return value.

#### `size_t getPIIDetections() const`
- Source: `include/exporters/exporter_metrics.h`:136
- Brief: Get PIIDetections.
- Parameters: none
- Return: Return value.
- Details: Return value.

#### `size_t getPIIRedactions() const`
- Source: `include/exporters/exporter_metrics.h`:141
- Brief: Get PIIRedactions.
- Parameters: none
- Return: Return value.
- Details: Return value.

#### `size_t getParquetBytesWritten() const`
- Source: `include/exporters/exporter_metrics.h`:166
- Brief: Get Parquet Bytes Written.
- Parameters: none
- Return: Return value.
- Details: Return value.

#### `size_t getPolicyDenials() const`
- Source: `include/exporters/exporter_metrics.h`:239
- Brief: Get Policy Denials.
- Parameters: none
- Return: Return value.
- Details: Return value.

#### `std::map< std::string, size_t > getQualityFilterRejections() const`
- Source: `include/exporters/exporter_metrics.h`:114
- Brief: n/a
- Parameters: none

#### `size_t getRateLimitHits() const`
- Source: `include/exporters/exporter_metrics.h`:226
- Brief: Get Rate Limit Hits.
- Parameters: none
- Return: Return value.
- Details: Return value.

#### `SchemaValidationStats getSchemaValidationStats() const`
- Source: `include/exporters/exporter_metrics.h`:126
- Brief: Get Schema Validation Stats.
- Parameters: none
- Return: Return value.
- Details: Return value.

#### `double getThroughput() const`
- Source: `include/exporters/exporter_metrics.h`:74
- Brief: Get Throughput.
- Parameters: none
- Return: Return value.
- Details: Return value.

#### `size_t getTotalDuplicates() const`
- Source: `include/exporters/exporter_metrics.h`:112
- Brief: Get Total Duplicates.
- Parameters: none
- Return: Return value.
- Details: Return value.

#### `size_t getTotalErrors() const`
- Source: `include/exporters/exporter_metrics.h`:104
- Brief: Get Total Errors.
- Parameters: none
- Return: Return value.
- Details: Return value.

#### `void recordCheckpoint()`
- Source: `include/exporters/exporter_metrics.h`:171
- Brief: Record Checkpoint.
- Parameters: none
- Details: Implements recordCheckpoint without additional internal calls.

#### `void recordCompression(size_t uncompressed_bytes, size_t compressed_bytes)`
- Source: `include/exporters/exporter_metrics.h`:148
- Brief: Record Compression.
- Parameters:
  - `uncompressed_bytes` (size_t): Input parameter.
  - `compressed_bytes` (size_t): Input parameter.
- Details: uncompressed_bytes Input parameter. compressed_bytes Input parameter. uncompressed_bytes Input parameter. compressed_bytes Input parameter. Implements recordCompression without additional internal calls.

#### `void recordDeltaDocSkipped(size_t count=1)`
- Source: `include/exporters/exporter_metrics.h`:179
- Brief: Record Delta Doc Skipped.
- Parameters:
  - `count` (size_t): Input parameter.
- Details: count Input parameter. Implements recordDeltaDocSkipped without additional internal calls.

#### `void recordDuplicate()`
- Source: `include/exporters/exporter_metrics.h`:50
- Brief: Record Duplicate.
- Parameters: none
- Details: Implements recordDuplicate without additional internal calls.

#### `void recordEncryption(size_t encrypted_bytes)`
- Source: `include/exporters/exporter_metrics.h`:209
- Brief: Record Encryption.
- Parameters:
  - `encrypted_bytes` (size_t): Input parameter.
- Details: encrypted_bytes Input parameter. encrypted_bytes Input parameter. Implements recordEncryption without additional internal calls.

#### `void recordEncryption(size_t plaintext_bytes, size_t encrypted_bytes)`
- Source: `include/exporters/exporter_metrics.h`:192
- Brief: Record Encryption.
- Parameters:
  - `plaintext_bytes` (size_t): Input parameter.
  - `encrypted_bytes` (size_t): Input parameter.
- Details: plaintext_bytes Input parameter. encrypted_bytes Input parameter. plaintext_bytes Input parameter. encrypted_bytes Input parameter. Implements recordEncryption without additional internal calls.

#### `void recordError(const std::string &error_type)`
- Source: `include/exporters/exporter_metrics.h`:45
- Brief: Record Error.
- Parameters:
  - `error_type` (const std::string &): Input parameter.
- Details: error_type Input parameter. error_type Input parameter. Calls: lock().

#### `void recordExport(size_t entity_count, size_t bytes_written, std::chrono::milliseconds duration)`
- Source: `include/exporters/exporter_metrics.h`:38
- Brief: Record Export.
- Parameters:
  - `entity_count` (size_t): Input parameter.
  - `bytes_written` (size_t): Input parameter.
  - `duration` (std::chrono::milliseconds): Input parameter.
- Details: entity_count Input parameter. bytes_written Input parameter. duration Input parameter. entity_count Input parameter. bytes_written Input parameter. duration Input parameter. Calls: lock(), count(), updateLatencyHistogram().

#### `void recordHubUploadFailure(const std::string &reason)`
- Source: `include/exporters/exporter_metrics.h`:245
- Brief: Record Hub Upload Failure.
- Parameters:
  - `reason` (const std::string &): Input parameter.
- Details: reason Input parameter. reason Input parameter. Calls: recordError().

#### `void recordPIIDetection(size_t count=1)`
- Source: `include/exporters/exporter_metrics.h`:128
- Brief: Record PIIDetection.
- Parameters:
  - `count` (size_t): Input parameter.
- Details: count Input parameter. Implements recordPIIDetection without additional internal calls.

#### `void recordPIIRedaction(size_t count=1)`
- Source: `include/exporters/exporter_metrics.h`:130
- Brief: Record PIIRedaction.
- Parameters:
  - `count` (size_t): Input parameter.
- Details: count Input parameter. Implements recordPIIRedaction without additional internal calls.

#### `void recordParquetBytesWritten(size_t bytes)`
- Source: `include/exporters/exporter_metrics.h`:160
- Brief: Record Parquet Bytes Written.
- Parameters:
  - `bytes` (size_t): Input parameter.
- Details: bytes Input parameter. bytes Input parameter. Implements recordParquetBytesWritten without additional internal calls.

#### `void recordPolicyDenial(const std::string &collection, const std::string &user)`
- Source: `include/exporters/exporter_metrics.h`:233
- Brief: Record Policy Denial.
- Parameters:
  - `collection` (const std::string &): Input parameter.
  - `user` (const std::string &): Input parameter.
- Details: collection Input parameter. user Input parameter. collection Input parameter. user Input parameter. Calls: recordError().

#### `void recordQualityFilterRejection(const std::string &reason)`
- Source: `include/exporters/exporter_metrics.h`:56
- Brief: Record Quality Filter Rejection.
- Parameters:
  - `reason` (const std::string &): Input parameter.
- Details: reason Input parameter. reason Input parameter. Calls: lock().

#### `void recordRateLimitHit()`
- Source: `include/exporters/exporter_metrics.h`:220
- Brief: Record Rate Limit Hit.
- Parameters: none
- Details: Implements recordRateLimitHit without additional internal calls.

#### `void recordSchemaValidation(bool passed)`
- Source: `include/exporters/exporter_metrics.h`:62
- Brief: Record Schema Validation.
- Parameters:
  - `passed` (bool): Input parameter.
- Details: passed Input parameter. passed Input parameter. Implements recordSchemaValidation without additional internal calls.

#### `void reset()`
- Source: `include/exporters/exporter_metrics.h`:30
- Brief: Reset the modification detection flag.
- Parameters: none
- Details: Calls: lock(), clear().

#### `nlohmann::json toJson() const`
- Source: `include/exporters/exporter_metrics.h`:257
- Brief: To Json.
- Parameters: none
- Return: Return value.
- Details: Return value.

#### `std::string toString() const`
- Source: `include/exporters/exporter_metrics.h`:263
- Brief: To String.
- Parameters: none
- Return: Return value.
- Details: Return value.

#### `void updateLatencyHistogram(std::chrono::milliseconds duration)`
- Source: `include/exporters/exporter_metrics.h`:335
- Brief: Update Latency Histogram.
- Parameters:
  - `duration` (std::chrono::milliseconds): Input parameter.
- Details: duration Input parameter. duration Input parameter. Calls: count().

### themis::exporters::FormatException

#### `FormatException(const std::string &message, const std::string &format_name="")`
- Source: `include/exporters/exporter_errors.h`:135
- Brief: n/a
- Parameters:
  - `message` (const std::string &): n/a
  - `format_name` (const std::string &): n/a

#### `const std::string & getFormatName() const`
- Source: `include/exporters/exporter_errors.h`:145
- Brief: n/a
- Parameters: none

### themis::exporters::HuggingFaceExporter

#### `HuggingFaceExporter(const HuggingFaceExporterConfig &config={})`
- Source: `include/exporters/huggingface_exporter.h`:62
- Brief: n/a
- Parameters:
  - `config` (const HuggingFaceExporterConfig &): n/a

#### `ExportStats exportEntities(const std::vector< BaseEntity > &entities, const ExportOptions &options) override`
- Source: `include/exporters/huggingface_exporter.h`:64
- Brief: Export Entities.
- Parameters:
  - `entities` (const std::vector< BaseEntity > &): Input parameter.
  - `options` (const ExportOptions &): Input parameter.
- Return: Return value.
- Throws:
  - ExportIOException: if an error occurs.
- Details: entities Input parameter. options Input parameter. Return value. ExportIOException if an error occurs. Calls: enforceExportPolicy(), std::chrono::steady_clock::now(), empty(), push_back(), root_dir(), fs::create_directories(), inferFeatures(), jsonl_exporter().

#### `std::string generateDatasetCard() const`
- Source: `include/exporters/huggingface_exporter.h`:98
- Brief: Generate Dataset Card.
- Parameters: none
- Return: Return value.
- Details: Return value.

#### `std::string generateDatasetInfoJson(const ExportStats &stats, const std::string &dataset_name={}, size_t data_file_bytes=0) const`
- Source: `include/exporters/huggingface_exporter.h`:88
- Brief: n/a
- Parameters:
  - `stats` (const ExportStats &): n/a
  - `dataset_name` (const std::string &): n/a
  - `data_file_bytes` (size_t): n/a

#### `const HuggingFaceExporterConfig & getConfig() const`
- Source: `include/exporters/huggingface_exporter.h`:86
- Brief: n/a
- Parameters: none

#### `std::shared_ptr< ExporterMetrics > getMetrics() const`
- Source: `include/exporters/huggingface_exporter.h`:100
- Brief: n/a
- Parameters: none

#### `std::string getName() const override`
- Source: `include/exporters/huggingface_exporter.h`:73
- Brief: n/a
- Parameters: none

#### `std::vector< std::string > getSupportedFormats() const override`
- Source: `include/exporters/huggingface_exporter.h`:69
- Brief: n/a
- Parameters: none

#### `std::string getVersion() const override`
- Source: `include/exporters/huggingface_exporter.h`:74
- Brief: n/a
- Parameters: none

#### `std::string inferDtype(const Value &value)`
- Source: `include/exporters/huggingface_exporter.h`:111
- Brief: Infer Dtype.
- Parameters:
  - `value` (const Value &): Input parameter.
- Return: Return value.
- Details: value Input parameter. Return value. value Input parameter. Return value. Calls: std::visit(), constexpr().

#### `void inferFeatures(const std::vector< BaseEntity > &entities)`
- Source: `include/exporters/huggingface_exporter.h`:117
- Brief: Infer Features.
- Parameters:
  - `entities` (const std::vector< BaseEntity > &): Input parameter.
- Details: entities Input parameter. entities Input parameter. Calls: clear(), empty(), getAllFields(), find(), end(), inferDtype(), push_back(), std::move().

#### `const std::vector< HuggingFaceFeature > & resolvedFeatures() const`
- Source: `include/exporters/huggingface_exporter.h`:123
- Brief: Resolved Features.
- Parameters: none
- Return: Return value.
- Details: Return value.

#### `void setConfig(const HuggingFaceExporterConfig &config)`
- Source: `include/exporters/huggingface_exporter.h`:81
- Brief: Set Config.
- Parameters:
  - `config` (const HuggingFaceExporterConfig &): Input parameter.
- Details: config Input parameter. Calls: clear().

### themis::exporters::HuggingFaceHubClient

#### `HuggingFaceHubClient(HubUploadConfig config)`
- Source: `include/exporters/huggingface_hub_client.h`:89
- Brief: Hugging Face Hub Client.
- Parameters:
  - `config` (HubUploadConfig): Input parameter.
- Return: Return value.
- Details: config Input parameter. Return value.

#### `HubUploadResult ensureRepo(const std::string &bearer_token) const`
- Source: `include/exporters/huggingface_hub_client.h`:136
- Brief: Ensure Repo.
- Parameters:
  - `bearer_token` (const std::string &): Input parameter.
- Return: Return value.
- Details: bearer_token Input parameter. Return value.

#### `std::pair< int, std::string > httpPost(const std::string &url, const std::string &json_body, const std::string &bearer_token) const`
- Source: `include/exporters/huggingface_hub_client.h`:111
- Brief: n/a
- Parameters:
  - `url` (const std::string &): n/a
  - `json_body` (const std::string &): n/a
  - `bearer_token` (const std::string &): n/a

#### `int httpPutBytes(const std::string &url, const char *data, std::size_t size, const std::string &bearer_token, std::function< void(double)> progress_cb, std::string *retry_after_out=nullptr) const`
- Source: `include/exporters/huggingface_hub_client.h`:116
- Brief: n/a
- Parameters:
  - `url` (const std::string &): n/a
  - `data` (const char *): n/a
  - `size` (std::size_t): n/a
  - `bearer_token` (const std::string &): n/a
  - `progress_cb` (std::function< void(double)>): n/a
  - `retry_after_out` (std::string *): n/a

#### `int httpPutFile(const std::string &url, const std::string &file_path, const std::string &bearer_token, std::function< void(double)> progress_cb, std::string *retry_after_out=nullptr) const`
- Source: `include/exporters/huggingface_hub_client.h`:124
- Brief: n/a
- Parameters:
  - `url` (const std::string &): n/a
  - `file_path` (const std::string &): n/a
  - `bearer_token` (const std::string &): n/a
  - `progress_cb` (std::function< void(double)>): n/a
  - `retry_after_out` (std::string *): n/a

#### `std::string resolveToken() const`
- Source: `include/exporters/huggingface_hub_client.h`:109
- Brief: Resolve Token.
- Parameters: none
- Return: Return value.
- Details: Return value.

#### `HubUploadResult uploadDataset(const std::string &dataset_dir, std::function< void(double)> progress_cb={}) const`
- Source: `include/exporters/huggingface_hub_client.h`:92
- Brief: n/a
- Parameters:
  - `dataset_dir` (const std::string &): n/a
  - `progress_cb` (std::function< void(double)>): n/a

#### `HubUploadResult uploadShards(const std::vector< MemoryShardSpec > &shards, std::function< void(double)> progress_cb={}) const`
- Source: `include/exporters/huggingface_hub_client.h`:96
- Brief: n/a
- Parameters:
  - `shards` (const std::vector< MemoryShardSpec > &): n/a
  - `progress_cb` (std::function< void(double)>): n/a

#### `~HuggingFaceHubClient()`
- Source: `include/exporters/huggingface_hub_client.h`:90
- Brief: n/a
- Parameters: none

### themis::exporters::IExporter

#### `ExportStats exportEntities(const std::vector< BaseEntity > &entities, const ExportOptions &options)=0`
- Source: `include/exporters/exporter_interface.h`:143
- Brief: n/a
- Parameters:
  - `entities` (const std::vector< BaseEntity > &): n/a
  - `options` (const ExportOptions &): n/a

#### `std::string getName() const =0`
- Source: `include/exporters/exporter_interface.h`:150
- Brief: n/a
- Parameters: none

#### `std::vector< std::string > getSupportedFormats() const =0`
- Source: `include/exporters/exporter_interface.h`:148
- Brief: n/a
- Parameters: none

#### `std::string getVersion() const =0`
- Source: `include/exporters/exporter_interface.h`:152
- Brief: n/a
- Parameters: none

#### `~IExporter()=default`
- Source: `include/exporters/exporter_interface.h`:141
- Brief: IExporter.
- Parameters: none
- Return: Return value.
- Details: Return value.

### themis::exporters::IFormatTemplate

#### `std::string name() const =0`
- Source: `include/exporters/format_template.h`:51
- Brief: Name.
- Parameters: none
- Return: Return value.
- Details: Return value.

#### `std::string render(const BaseEntity &entity, const FormatTemplateFieldMapping &mapping) const =0`
- Source: `include/exporters/format_template.h`:65
- Brief: Render.
- Parameters:
  - `entity` (const BaseEntity &): Input parameter.
  - `mapping` (const FormatTemplateFieldMapping &): Input parameter.
- Return: Return value.
- Details: entity Input parameter. mapping Input parameter. Return value.

#### `bool validateFields(const BaseEntity &entity, const FormatTemplateFieldMapping &mapping, std::vector< std::string > *missing_fields=nullptr) const =0`
- Source: `include/exporters/format_template.h`:53
- Brief: n/a
- Parameters:
  - `entity` (const BaseEntity &): n/a
  - `mapping` (const FormatTemplateFieldMapping &): n/a
  - `missing_fields` (std::vector< std::string > *): n/a

#### `~IFormatTemplate()=default`
- Source: `include/exporters/format_template.h`:45
- Brief: IFormat Template.
- Parameters: none
- Return: Return value.
- Details: Return value.

### themis::exporters::IncrementalExporter

#### `IncrementalExporter(const IncrementalExportConfig &config={})`
- Source: `include/exporters/incremental_exporter.h`:34
- Brief: n/a
- Parameters:
  - `config` (const IncrementalExportConfig &): n/a

#### `ExportStats exportEntities(const std::vector< BaseEntity > &entities, const ExportOptions &options) override`
- Source: `include/exporters/incremental_exporter.h`:36
- Brief: ───────────────────────────────────────────────────────────────────────────── IExporter::exportEntities ─────────────────────────────────────────────────────────────────────────────
- Parameters:
  - `entities` (const std::vector< BaseEntity > &): Input parameter.
  - `options` (const ExportOptions &): Input parameter.
- Return: Return value.
- Throws:
  - ExporterException: if an error occurs.
  - ExportIOException: if an error occurs.
- Details: entities Input parameter. options Input parameter. Return value. ExporterException if an error occurs. ExportIOException if an error occurs. Calls: enforceExportPolicy(), std::chrono::steady_clock::now(), readWatermark(), THEMIS_INFO(), writer(), empty(), std::string(), what().

#### `int64_t extractSequence(const BaseEntity &entity) const`
- Source: `include/exporters/incremental_exporter.h`:76
- Brief: Extract Sequence.
- Parameters:
  - `entity` (const BaseEntity &): Input parameter.
- Return: Return value.
- Details: entity Input parameter. Return value.

#### `std::string formatEntity(const BaseEntity &entity, const ExportOptions &options)`
- Source: `include/exporters/incremental_exporter.h`:84
- Brief: Format Entity.
- Parameters:
  - `entity` (const BaseEntity &): Input parameter.
  - `options` (const ExportOptions &): Input parameter.
- Return: Return value.
- Details: entity Input parameter. options Input parameter. Return value. entity Input parameter. options Input parameter. Return value. Calls: getAllFields(), empty(), getPrimaryKey(), std::visit(), constexpr(), std::setfill(), std::setw(), str().

#### `std::shared_ptr< ExporterMetrics > getMetrics() const`
- Source: `include/exporters/incremental_exporter.h`:65
- Brief: n/a
- Parameters: none

#### `std::string getName() const override`
- Source: `include/exporters/incremental_exporter.h`:45
- Brief: n/a
- Parameters: none

#### `std::vector< std::string > getSupportedFormats() const override`
- Source: `include/exporters/incremental_exporter.h`:41
- Brief: n/a
- Parameters: none

#### `std::string getVersion() const override`
- Source: `include/exporters/incremental_exporter.h`:46
- Brief: n/a
- Parameters: none

#### `int64_t readWatermark() const`
- Source: `include/exporters/incremental_exporter.h`:52
- Brief: Read Watermark.
- Parameters: none
- Return: Return value.
- Details: Return value.

#### `bool writeWatermark(int64_t sequence, size_t exported_count, const std::string &timestamp) const`
- Source: `include/exporters/incremental_exporter.h`:61
- Brief: Write Watermark.
- Parameters:
  - `sequence` (int64_t): Input parameter.
  - `exported_count` (size_t): Input parameter.
  - `timestamp` (const std::string &): Input parameter.
- Return: True when the operation succeeds.
- Details: sequence Input parameter. exported_count Input parameter. timestamp Input parameter. True when the operation succeeds.

### themis::exporters::JSONLLLMExporter

#### `JSONLLLMExporter(const JSONLLLMConfig &config={})`
- Source: `include/exporters/jsonl_llm_exporter.h`:168
- Brief: n/a
- Parameters:
  - `config` (const JSONLLLMConfig &): n/a

#### `double calculateWeight(const BaseEntity &entity)`
- Source: `include/exporters/jsonl_llm_exporter.h`:275
- Brief: Calculate Weight.
- Parameters:
  - `entity` (const BaseEntity &): Input parameter.
- Return: Return value.
- Details: entity Input parameter. Return value. entity Input parameter. Return value. Calls: hasField(), getFieldAsDouble(), std::clamp(), getFieldAsString(), size(), std::min(), std::stoll(), std::chrono::system_clock::time_point().

#### `ExportStats exportEntities(const std::vector< BaseEntity > &entities, const ExportOptions &options) override`
- Source: `include/exporters/jsonl_llm_exporter.h`:170
- Brief: Export Entities.
- Parameters:
  - `entities` (const std::vector< BaseEntity > &): Input parameter.
  - `options` (const ExportOptions &): Input parameter.
- Return: Return value.
- Throws:
  - ExporterException: if an error occurs.
  - ExportIOException: if an error occurs.
- Details: entities Input parameter. options Input parameter. Return value. ExporterException if an error occurs. ExportIOException if an error occurs. Calls: enforceExportPolicy(), std::chrono::steady_clock::now(), hasScope(), THEMIS_INFO(), writer(), empty(), size(), isLimitReached().

#### `std::string extractMetadata(const BaseEntity &entity, const ExportOptions &options)`
- Source: `include/exporters/jsonl_llm_exporter.h`:288
- Brief: Extract Metadata.
- Parameters:
  - `entity` (const BaseEntity &): Input parameter.
  - `options` (const ExportOptions &): Input parameter.
- Return: Return value.
- Details: entity Input parameter. options Input parameter. Return value. entity Input parameter. options Input parameter. Return value. Calls: isFieldAllowed(), hasField(), getFieldAsString(), empty(), dump().

#### `std::string formatChatCompletion(const BaseEntity &entity, double &weight, const ExportOptions &options)`
- Source: `include/exporters/jsonl_llm_exporter.h`:249
- Brief: Format Chat Completion.
- Parameters:
  - `entity` (const BaseEntity &): Input parameter.
  - `weight` (double &): Input/output parameter.
  - `options` (const ExportOptions &): Input parameter.
- Return: Return value.
- Details: entity Input parameter. weight Input/output parameter. options Input parameter. Return value. entity Input parameter. weight Input/output parameter. options Input parameter. Return value. Calls: json::array(), isFieldAllowed(), getFieldAsString(), empty(), push_back(), extractMetadata(), json::parse(), dump().

#### `std::string formatInstructionTuning(const BaseEntity &entity, double &weight, const ExportOptions &options)`
- Source: `include/exporters/jsonl_llm_exporter.h`:240
- Brief: Format Instruction Tuning.
- Parameters:
  - `entity` (const BaseEntity &): Input parameter.
  - `weight` (double &): Input/output parameter.
  - `options` (const ExportOptions &): Input parameter.
- Return: Return value.
- Details: entity Input parameter. weight Input/output parameter. options Input parameter. Return value. entity Input parameter. weight Input/output parameter. options Input parameter. Return value. Calls: isFieldAllowed(), getFieldAsString(), empty(), extractMetadata(), json::parse(), dump().

#### `std::string formatTextCompletion(const BaseEntity &entity, double &weight, const ExportOptions &options)`
- Source: `include/exporters/jsonl_llm_exporter.h`:258
- Brief: Format Text Completion.
- Parameters:
  - `entity` (const BaseEntity &): Input parameter.
  - `weight` (double &): Input/output parameter.
  - `options` (const ExportOptions &): Input parameter.
- Return: Return value.
- Details: entity Input parameter. weight Input/output parameter. options Input parameter. Return value. entity Input parameter. weight Input/output parameter. options Input parameter. Return value. Calls: isFieldAllowed(), getFieldAsString(), extractMetadata(), empty(), json::parse(), dump().

#### `std::string formatWithTemplate(const BaseEntity &entity, double &weight, const ExportOptions &options)`
- Source: `include/exporters/jsonl_llm_exporter.h`:267
- Brief: Format With Template.
- Parameters:
  - `entity` (const BaseEntity &): Input parameter.
  - `weight` (double &): Input/output parameter.
  - `options` (const ExportOptions &): Input parameter.
- Return: Return value.
- Details: entity Input parameter. weight Input/output parameter. options Input parameter. Return value. entity Input parameter. weight Input/output parameter. options Input parameter. Return value. Calls: render(), empty(), json::parse(), dump(), THEMIS_WARN(), what(), is_object(), begin().

#### `std::string getAdapterMetadataJson() const`
- Source: `include/exporters/jsonl_llm_exporter.h`:209
- Brief: Get Adapter Metadata Json.
- Parameters: none
- Return: Return value.
- Details: Return value.

#### `const JSONLLLMConfig & getConfig() const`
- Source: `include/exporters/jsonl_llm_exporter.h`:201
- Brief: n/a
- Parameters: none

#### `std::shared_ptr< ExporterMetrics > getMetrics() const`
- Source: `include/exporters/jsonl_llm_exporter.h`:219
- Brief: n/a
- Parameters: none

#### `std::string getName() const override`
- Source: `include/exporters/jsonl_llm_exporter.h`:179
- Brief: n/a
- Parameters: none

#### `std::string getQualityMetricsReport() const`
- Source: `include/exporters/jsonl_llm_exporter.h`:217
- Brief: Get Quality Metrics Report.
- Parameters: none
- Return: Return value.
- Details: Return value.

#### `std::vector< std::string > getSupportedFormats() const override`
- Source: `include/exporters/jsonl_llm_exporter.h`:175
- Brief: n/a
- Parameters: none

#### `std::string getVersion() const override`
- Source: `include/exporters/jsonl_llm_exporter.h`:180
- Brief: n/a
- Parameters: none

#### `bool isFieldAllowed(const std::string &field_name, const std::vector< std::string > &include_fields, const std::vector< std::string > &exclude_fields)`
- Source: `include/exporters/jsonl_llm_exporter.h`:297
- Brief: Is Field Allowed.
- Parameters:
  - `field_name` (const std::string &): Name of the field.
  - `include_fields` (const std::vector< std::string > &): Input parameter.
  - `exclude_fields` (const std::vector< std::string > &): Input parameter.
- Return: True when the operation succeeds.
- Details: field_name Name of the field. include_fields Input parameter. exclude_fields Input parameter. True when the operation succeeds. field_name Name of the field. include_fields Input parameter. exclude_fields Input parameter. True when the operation succeeds. Calls: empty().

#### `bool passesQualityFilter(const BaseEntity &entity)`
- Source: `include/exporters/jsonl_llm_exporter.h`:281
- Brief: Passes Quality Filter.
- Parameters:
  - `entity` (const BaseEntity &): Input parameter.
- Return: True when the operation succeeds.
- Details: entity Input parameter. True when the operation succeeds. entity Input parameter. True when the operation succeeds. Calls: getFieldAsString(), empty(), size(), computeToxicityScore().

#### `void resetMetrics()`
- Source: `include/exporters/jsonl_llm_exporter.h`:225
- Brief: Reset Metrics.
- Parameters: none
- Details: Calls: reset().

#### `bool setAdapterMetadataFromJson(const std::string &json_str, std::string *error=nullptr)`
- Source: `include/exporters/jsonl_llm_exporter.h`:211
- Brief: Set Adapter Metadata From Json.
- Parameters:
  - `json_str` (const std::string &): Input parameter.
  - `error` (std::string *): Input/output parameter.
- Return: True when the operation succeeds.
- Details: json_str Input parameter. error Input/output parameter. True when the operation succeeds. Calls: json::parse(), contains(), std::string(), what().

#### `void setConfig(const JSONLLLMConfig &config)`
- Source: `include/exporters/jsonl_llm_exporter.h`:187
- Brief: Set Config.
- Parameters:
  - `config` (const JSONLLLMConfig &): Input parameter.
- Details: config Input parameter. Calls: makeFormatTemplate().

#### `bool validateAgainstSchema(const std::string &json_str, std::string *error=nullptr) const`
- Source: `include/exporters/jsonl_llm_exporter.h`:203
- Brief: n/a
- Parameters:
  - `json_str` (const std::string &): n/a
  - `error` (std::string *): n/a

#### `bool validateJsonSchema(const std::string &json_str, const std::string &schema, std::string *error) const`
- Source: `include/exporters/jsonl_llm_exporter.h`:309
- Brief: Validate Json Schema.
- Parameters:
  - `json_str` (const std::string &): Input parameter.
  - `schema` (const std::string &): Input parameter.
  - `error` (std::string *): Input/output parameter.
- Return: True when the operation succeeds.
- Details: json_str Input parameter. schema Input parameter. error Input/output parameter. True when the operation succeeds.

#### `TemplateValidationResult validateTemplate(const std::vector< BaseEntity > &sample) const`
- Source: `include/exporters/jsonl_llm_exporter.h`:197
- Brief: Validate Template.
- Parameters:
  - `sample` (const std::vector< BaseEntity > &): Input parameter.
- Return: Return value.
- Details: sample Input parameter. Return value.

### themis::exporters::JoinExporter

#### `JoinExporter(const JoinExportConfig &config={})`
- Source: `include/exporters/join_exporter.h`:61
- Brief: n/a
- Parameters:
  - `config` (const JoinExportConfig &): n/a

#### `std::unique_ptr< PIIDetector > buildPIIDetector() const`
- Source: `include/exporters/join_exporter.h`:105
- Brief: Build PIIDetector.
- Parameters: none
- Return: Return value.
- Details: Return value.

#### `size_t estimateEntityBytes(const BaseEntity &entity)`
- Source: `include/exporters/join_exporter.h`:112
- Brief: Estimate Entity Bytes.
- Parameters:
  - `entity` (const BaseEntity &): Input parameter.
- Return: Return value.
- Details: ── estimateEntityBytes ─────────────────────────────────────────────────────── entity Input parameter. Return value. entity Input parameter. Return value. Calls: toJson(), size(), getPrimaryKey().

#### `ExportStats exportEntities(const std::vector< BaseEntity > &entities, const ExportOptions &options) override`
- Source: `include/exporters/join_exporter.h`:69
- Brief: ── exportEntities ────────────────────────────────────────────────────────────
- Parameters:
  - `entities` (const std::vector< BaseEntity > &): Input parameter.
  - `options` (const ExportOptions &): Input parameter.
- Return: Return value.
- Throws:
  - ExporterException: if an error occurs.
- Details: entities Input parameter. options Input parameter. Return value. ExporterException if an error occurs. Calls: enforceExportPolicy(), empty(), std::chrono::steady_clock::now(), std::string(), what(), buildPIIDetector(), writer(), size().

#### `std::shared_ptr< ExporterMetrics > getMetrics() const`
- Source: `include/exporters/join_exporter.h`:81
- Brief: n/a
- Parameters: none

#### `std::string getName() const override`
- Source: `include/exporters/join_exporter.h`:78
- Brief: n/a
- Parameters: none

#### `std::vector< std::string > getSupportedFormats() const override`
- Source: `include/exporters/join_exporter.h`:74
- Brief: n/a
- Parameters: none

#### `std::string getVersion() const override`
- Source: `include/exporters/join_exporter.h`:79
- Brief: n/a
- Parameters: none

#### `BaseEntity mergeEntities(const BaseEntity &left, const BaseEntity &right) const`
- Source: `include/exporters/join_exporter.h`:99
- Brief: Merge Entities.
- Parameters:
  - `left` (const BaseEntity &): Input parameter.
  - `right` (const BaseEntity &): Input parameter.
- Return: Return value.
- Details: left Input parameter. right Input parameter. Return value.

#### `void setRightCollection(const std::vector< BaseEntity > &right_entities)`
- Source: `include/exporters/join_exporter.h`:67
- Brief: Set Right Collection.
- Parameters:
  - `right_entities` (const std::vector< BaseEntity > &): Input parameter.
- Throws:
  - ExporterException: if an error occurs.
- Details: ── setRightCollection ──────────────────────────────────────────────────────── right_entities Input parameter. right_entities Input parameter. ExporterException if an error occurs. Calls: empty(), clear(), getFieldString(), estimateEntityBytes(), std::to_string(), emplace(), THEMIS_INFO(), size().

### themis::exporters::OpenAIFineTuningTemplate

#### `std::string name() const override`
- Source: `include/exporters/format_template.h`:125
- Brief: Name.
- Parameters: none
- Return: Return value.
- Details: Return value.

#### `std::string render(const BaseEntity &entity, const FormatTemplateFieldMapping &mapping) const override`
- Source: `include/exporters/format_template.h`:133
- Brief: Render.
- Parameters:
  - `entity` (const BaseEntity &): Input parameter.
  - `mapping` (const FormatTemplateFieldMapping &): Input parameter.
- Return: Return value.
- Details: entity Input parameter. mapping Input parameter. Return value.

#### `bool validateFields(const BaseEntity &entity, const FormatTemplateFieldMapping &mapping, std::vector< std::string > *missing_fields=nullptr) const override`
- Source: `include/exporters/format_template.h`:127
- Brief: n/a
- Parameters:
  - `entity` (const BaseEntity &): n/a
  - `mapping` (const FormatTemplateFieldMapping &): n/a
  - `missing_fields` (std::vector< std::string > *): n/a

### themis::exporters::PIIDetector

#### `PIIDetector()`
- Source: `include/exporters/pii_detector.h`:66
- Brief: PIIDetector.
- Parameters: none
- Return: Return value.
- Details: Return value.

#### `PIIDetector(const Config &config)`
- Source: `include/exporters/pii_detector.h`:72
- Brief: PIIDetector.
- Parameters:
  - `config` (const Config &): Input parameter.
- Return: Return value.
- Details: config Input parameter. Return value.

#### `std::string applyRedaction(const std::string &value, RedactionStrategy strategy) const`
- Source: `include/exporters/pii_detector.h`:130
- Brief: Apply Redaction.
- Parameters:
  - `value` (const std::string &): Input parameter.
  - `strategy` (RedactionStrategy): Input parameter.
- Return: Return value.
- Details: value Input parameter. strategy Input parameter. Return value.

#### `bool containsPII(const std::string &text) const`
- Source: `include/exporters/pii_detector.h`:101
- Brief: Contains PII.
- Parameters:
  - `text` (const std::string &): Input parameter.
- Return: True when the operation succeeds.
- Details: text Input parameter. True when the operation succeeds.

#### `std::vector< PIIMatch > detectPII(const std::string &text) const`
- Source: `include/exporters/pii_detector.h`:79
- Brief: Detect PII.
- Parameters:
  - `text` (const std::string &): Input parameter.
- Return: Return value.
- Details: text Input parameter. Return value.

#### `RedactionStrategy getStrategy(PIIType type) const`
- Source: `include/exporters/pii_detector.h`:108
- Brief: Get Strategy.
- Parameters:
  - `type` (PIIType): Input parameter.
- Return: Return value.
- Details: type Input parameter. Return value.

#### `std::string hashString(const std::string &value) const`
- Source: `include/exporters/pii_detector.h`:142
- Brief: Hash String.
- Parameters:
  - `value` (const std::string &): Input parameter.
- Return: Return value.
- Details: value Input parameter. Return value.

#### `void initPatterns()`
- Source: `include/exporters/pii_detector.h`:123
- Brief: Init Patterns.
- Parameters: none
- Details: Calls: std::regex().

#### `std::string maskString(const std::string &value) const`
- Source: `include/exporters/pii_detector.h`:136
- Brief: Mask String.
- Parameters:
  - `value` (const std::string &): Input parameter.
- Return: Return value.
- Details: value Input parameter. Return value.

#### `std::string partialRedact(const std::string &value) const`
- Source: `include/exporters/pii_detector.h`:148
- Brief: Partial Redact.
- Parameters:
  - `value` (const std::string &): Input parameter.
- Return: Return value.
- Details: value Input parameter. Return value.

#### `std::string redactPII(const std::string &text) const`
- Source: `include/exporters/pii_detector.h`:86
- Brief: Redact PII.
- Parameters:
  - `text` (const std::string &): Input parameter.
- Return: Return value.
- Details: text Input parameter. Return value.

#### `std::string redactPII(const std::string &text, RedactionStrategy strategy) const`
- Source: `include/exporters/pii_detector.h`:94
- Brief: Redact PII.
- Parameters:
  - `text` (const std::string &): Input parameter.
  - `strategy` (RedactionStrategy): Input parameter.
- Return: Return value.
- Details: text Input parameter. strategy Input parameter. Return value.

### themis::exporters::PIIMetrics

#### `void recordDetection(PIIDetector::PIIType type)`
- Source: `include/exporters/pii_detector.h`:162
- Brief: Record Detection.
- Parameters:
  - `type` (PIIDetector::PIIType): Input parameter.
- Details: type Input parameter. Implements recordDetection without additional internal calls.

#### `void recordRedaction()`
- Source: `include/exporters/pii_detector.h`:172
- Brief: Record Redaction.
- Parameters: none
- Details: Implements recordRedaction without additional internal calls.

### themis::exporters::ParquetExporter

#### `ParquetExporter(const ParquetExportConfig &config={})`
- Source: `include/exporters/parquet_exporter.h`:71
- Brief: n/a
- Parameters:
  - `config` (const ParquetExportConfig &): n/a

#### `ExportStats exportEntities(const std::vector< BaseEntity > &entities, const ExportOptions &options) override`
- Source: `include/exporters/parquet_exporter.h`:74
- Brief: Export Entities.
- Parameters:
  - `entities` (const std::vector< BaseEntity > &): Input parameter.
  - `options` (const ExportOptions &): Input parameter.
- Return: Return value.
- Throws:
  - ExporterException: if an error occurs.
  - ConfigException: if an error occurs.
- Details: entities Input parameter. options Input parameter. Return value. ExporterException if an error occurs. ConfigException if an error occurs. Calls: enforceExportPolicy(), std::chrono::steady_clock::now(), hasScope(), THEMIS_INFO(), empty(), size(), resolveColumns(), exportWithArrow().

#### `ExportStats exportFallback(const std::vector< BaseEntity > &entities, const ExportOptions &options, const std::vector< std::string > &columns)`
- Source: `include/exporters/parquet_exporter.h`:144
- Brief: Export Fallback.
- Parameters:
  - `entities` (const std::vector< BaseEntity > &): Input parameter.
  - `options` (const ExportOptions &): Input parameter.
  - `columns` (const std::vector< std::string > &): Input parameter.
- Return: Return value.
- Throws:
  - ExporterException: if an error occurs.
  - ExportIOException: if an error occurs.
- Details: ───────────────────────────────────────────────────────────────────────────── Minimal hand-written Parquet fallback (no Arrow dependency) ───────────────────────────────────────────────────────────────────────────── entities Input parameter. options Input parameter. columns Input parameter. Return value. entities Input parameter. options Input parameter. columns Input parameter. Return value. ExporterException if an error occurs. ExportIOException if an error occurs. Calls: col_data(), size(), empty(), getFieldAsString(), evaluate(), insert(), getPrimaryKey(), valueToString().

#### `const ParquetExportConfig & getConfig() const`
- Source: `include/exporters/parquet_exporter.h`:91
- Brief: n/a
- Parameters: none

#### `std::shared_ptr< ExporterMetrics > getMetrics() const`
- Source: `include/exporters/parquet_exporter.h`:93
- Brief: n/a
- Parameters: none

#### `std::string getName() const override`
- Source: `include/exporters/parquet_exporter.h`:82
- Brief: n/a
- Parameters: none

#### `std::vector< std::string > getSupportedFormats() const override`
- Source: `include/exporters/parquet_exporter.h`:79
- Brief: n/a
- Parameters: none

#### `std::string getVersion() const override`
- Source: `include/exporters/parquet_exporter.h`:83
- Brief: n/a
- Parameters: none

#### `bool isArrowAvailable()`
- Source: `include/exporters/parquet_exporter.h`:105
- Brief: Is Arrow Available.
- Parameters: none
- Return: True when the operation succeeds.
- Details: True when the operation succeeds. True when the operation succeeds. Implements isArrowAvailable without additional internal calls.

#### `void resetMetrics()`
- Source: `include/exporters/parquet_exporter.h`:99
- Brief: Reset Metrics.
- Parameters: none
- Details: Calls: reset().

#### `std::vector< std::string > resolveColumns(const std::vector< BaseEntity > &entities, const ExportOptions &options) const`
- Source: `include/exporters/parquet_exporter.h`:117
- Brief: Resolve Columns.
- Parameters:
  - `entities` (const std::vector< BaseEntity > &): Input parameter.
  - `options` (const ExportOptions &): Input parameter.
- Return: Return value.
- Details: entities Input parameter. options Input parameter. Return value.

#### `void setConfig(const ParquetExportConfig &config)`
- Source: `include/exporters/parquet_exporter.h`:90
- Brief: Set Config.
- Parameters:
  - `config` (const ParquetExportConfig &): Input parameter.
- Details: config Input parameter. Implements setConfig without additional internal calls.

### themis::exporters::PolicyDeniedException

#### `PolicyDeniedException(const std::string &denial_reason, const std::string &requesting_user="", const std::string &collection="")`
- Source: `include/exporters/exporter_errors.h`:189
- Brief: Construct a PolicyDeniedException.
- Parameters:
  - `denial_reason` (const std::string &): Human-readable denial reason from PolicyEngine::checkExportPermission().
  - `requesting_user` (const std::string &): Identity of the user/service that requested the export (may be empty).
  - `collection` (const std::string &): Name of the collection being exported (may be empty).
- Details: denial_reason Human-readable denial reason from PolicyEngine::checkExportPermission(). requesting_user Identity of the user/service that requested the export (may be empty). collection Name of the collection being exported (may be empty).

#### `const std::string & getCollection() const`
- Source: `include/exporters/exporter_errors.h`:209
- Brief: n/a
- Parameters: none
- Return: The collection name that was being exported.
- Details: The collection name that was being exported.

#### `const std::string & getDenialReason() const`
- Source: `include/exporters/exporter_errors.h`:203
- Brief: n/a
- Parameters: none
- Return: The denial reason supplied by PolicyEngine.
- Details: The denial reason supplied by PolicyEngine.

#### `const std::string & getRequestingUser() const`
- Source: `include/exporters/exporter_errors.h`:206
- Brief: n/a
- Parameters: none
- Return: The requesting user identity.
- Details: The requesting user identity.

### themis::exporters::QualityFilterException

#### `QualityFilterException(const std::string &message, const std::string &entity_id="", const std::string &reason="")`
- Source: `include/exporters/exporter_errors.h`:112
- Brief: n/a
- Parameters:
  - `message` (const std::string &): n/a
  - `entity_id` (const std::string &): n/a
  - `reason` (const std::string &): n/a

#### `const std::string & getEntityId() const`
- Source: `include/exporters/exporter_errors.h`:124
- Brief: n/a
- Parameters: none

#### `const std::string & getReason() const`
- Source: `include/exporters/exporter_errors.h`:125
- Brief: n/a
- Parameters: none

### themis::exporters::SchemaValidationException

#### `SchemaValidationException(const std::string &message, const std::string &entity_id="", const std::string &validation_error="")`
- Source: `include/exporters/exporter_errors.h`:42
- Brief: n/a
- Parameters:
  - `message` (const std::string &): n/a
  - `entity_id` (const std::string &): n/a
  - `validation_error` (const std::string &): n/a

#### `const std::string & getEntityId() const`
- Source: `include/exporters/exporter_errors.h`:54
- Brief: n/a
- Parameters: none

#### `const std::string & getValidationError() const`
- Source: `include/exporters/exporter_errors.h`:55
- Brief: n/a
- Parameters: none

### themis::exporters::ShareGPTTemplate

#### `std::string name() const override`
- Source: `include/exporters/format_template.h`:93
- Brief: Name.
- Parameters: none
- Return: Return value.
- Details: Return value.

#### `std::string render(const BaseEntity &entity, const FormatTemplateFieldMapping &mapping) const override`
- Source: `include/exporters/format_template.h`:101
- Brief: Render.
- Parameters:
  - `entity` (const BaseEntity &): Input parameter.
  - `mapping` (const FormatTemplateFieldMapping &): Input parameter.
- Return: Return value.
- Details: entity Input parameter. mapping Input parameter. Return value.

#### `bool validateFields(const BaseEntity &entity, const FormatTemplateFieldMapping &mapping, std::vector< std::string > *missing_fields=nullptr) const override`
- Source: `include/exporters/format_template.h`:95
- Brief: n/a
- Parameters:
  - `entity` (const BaseEntity &): n/a
  - `mapping` (const FormatTemplateFieldMapping &): n/a
  - `missing_fields` (std::vector< std::string > *): n/a

### themis::exporters::SizeLimitException

#### `SizeLimitException(const std::string &message, size_t current_size, size_t max_size)`
- Source: `include/exporters/exporter_errors.h`:88
- Brief: n/a
- Parameters:
  - `message` (const std::string &): n/a
  - `current_size` (size_t): n/a
  - `max_size` (size_t): n/a

#### `size_t getCurrentSize() const`
- Source: `include/exporters/exporter_errors.h`:101
- Brief: n/a
- Parameters: none

#### `size_t getMaxSize() const`
- Source: `include/exporters/exporter_errors.h`:102
- Brief: n/a
- Parameters: none

### themis::exporters::StreamWriter

#### `StreamWriter(const Config &config)`
- Source: `include/exporters/stream_writer.h`:42
- Brief: Stream Writer.
- Parameters:
  - `config` (const Config &): Input parameter.
- Return: Return value.
- Details: config Input parameter. Return value.

#### `void close()`
- Source: `include/exporters/stream_writer.h`:65
- Brief: Close.
- Parameters: none
- Details: Calls: is_open(), finalizeCompression(), writeBuffer().

#### `void compressAndWrite(const char *data, size_t size)`
- Source: `include/exporters/stream_writer.h`:94
- Brief: Compress And Write.
- Parameters:
  - `data` (const char *): Input parameter.
  - `size` (size_t): Input parameter.
- Throws:
  - ExportIOException: if an error occurs.
- Details: data Input parameter. size Input parameter. ExportIOException if an error occurs. Calls: data(), size(), ZSTD_compressStream(), ZSTD_isError(), write().

#### `void finalizeCompression()`
- Source: `include/exporters/stream_writer.h`:98
- Brief: Finalize Compression.
- Parameters: none
- Details: Calls: data(), size(), ZSTD_endStream(), write(), ZSTD_freeCStream().

#### `void flush()`
- Source: `include/exporters/stream_writer.h`:60
- Brief: Flush.
- Parameters: none
- Details: Calls: data(), size(), ZSTD_flushStream(), write(), writeBuffer().

#### `size_t getBytesWritten() const`
- Source: `include/exporters/stream_writer.h`:67
- Brief: n/a
- Parameters: none

#### `size_t getCompressedBytesWritten() const`
- Source: `include/exporters/stream_writer.h`:69
- Brief: n/a
- Parameters: none

#### `void initCompression()`
- Source: `include/exporters/stream_writer.h`:89
- Brief: Init Compression.
- Parameters: none
- Throws:
  - ExportIOException: if an error occurs.
- Details: ExportIOException if an error occurs. Calls: ZSTD_createCStream(), ZSTD_initCStream(), ZSTD_isError(), ZSTD_freeCStream().

#### `bool isLimitReached() const`
- Source: `include/exporters/stream_writer.h`:71
- Brief: n/a
- Parameters: none

#### `void write(const char *data, size_t size)`
- Source: `include/exporters/stream_writer.h`:55
- Brief: Write.
- Parameters:
  - `data` (const char *): Input parameter.
  - `size` (size_t): Input parameter.
- Throws:
  - SizeLimitException: if an error occurs.
- Details: data Input parameter. size Input parameter. data Input parameter. size Input parameter. SizeLimitException if an error occurs. Calls: compressAndWrite(), size(), writeBuffer(), std::memcpy(), data().

#### `void write(const std::string &data)`
- Source: `include/exporters/stream_writer.h`:49
- Brief: Write.
- Parameters:
  - `data` (const std::string &): Input parameter.
- Details: data Input parameter. data Input parameter. Calls: data(), size().

#### `void writeBuffer()`
- Source: `include/exporters/stream_writer.h`:93
- Brief: Write Buffer.
- Parameters: none
- Details: Calls: write(), data().

#### `~StreamWriter()`
- Source: `include/exporters/stream_writer.h`:43
- Brief: n/a
- Parameters: none

### themis::exporters::StreamingExporter

#### `StreamingExporter(const StreamingExportConfig &config={})`
- Source: `include/exporters/streaming_exporter.h`:68
- Brief: n/a
- Parameters:
  - `config` (const StreamingExportConfig &): n/a

#### `double calculateETA(size_t processed, size_t total, std::chrono::steady_clock::time_point start_time)`
- Source: `include/exporters/streaming_exporter.h`:128
- Brief: Calculate ETA.
- Parameters:
  - `processed` (size_t): Input parameter.
  - `total` (size_t): Input parameter.
  - `start_time` (std::chrono::steady_clock::time_point): Input parameter.
- Return: Return value.
- Details: processed Input parameter. total Input parameter. start_time Input parameter. Return value. processed Input parameter. total Input parameter. start_time Input parameter. Return value. Calls: std::chrono::steady_clock::now(), count().

#### `ExportStats exportEntities(const std::vector< BaseEntity > &entities, const ExportOptions &options) override`
- Source: `include/exporters/streaming_exporter.h`:70
- Brief: Export Entities.
- Parameters:
  - `entities` (const std::vector< BaseEntity > &): Input parameter.
  - `options` (const ExportOptions &): Input parameter.
- Return: Return value.
- Details: entities Input parameter. options Input parameter. Return value. Calls: enforceExportPolicy(), cursor(), exportFromCursor().

#### `ExportStats exportFromCursor(ExportCursor &cursor, const ExportOptions &options)`
- Source: `include/exporters/streaming_exporter.h`:81
- Brief: Export From Cursor.
- Parameters:
  - `cursor` (ExportCursor &): Input/output parameter.
  - `options` (const ExportOptions &): Input parameter.
- Return: Return value.
- Throws:
  - ExportIOException: if an error occurs.
- Details: cursor Input/output parameter. options Input parameter. Return value. cursor Input/output parameter. options Input parameter. Return value. ExportIOException if an error occurs. Calls: std::chrono::steady_clock::now(), empty(), readCheckpoint(), seekTo(), recordCheckpoint(), THEMIS_INFO(), THEMIS_WARN(), totalCount().

#### `std::string formatEntity(const BaseEntity &entity, const ExportOptions &options)`
- Source: `include/exporters/streaming_exporter.h`:105
- Brief: Format Entity.
- Parameters:
  - `entity` (const BaseEntity &): Input parameter.
  - `options` (const ExportOptions &): Input parameter.
- Return: Return value.
- Details: entity Input parameter. options Input parameter. Return value. entity Input parameter. options Input parameter. Return value. Calls: getAllFields(), empty(), getPrimaryKey(), std::visit(), constexpr(), std::setfill(), std::setw(), str().

#### `std::shared_ptr< ExporterMetrics > getMetrics() const`
- Source: `include/exporters/streaming_exporter.h`:93
- Brief: n/a
- Parameters: none

#### `std::string getName() const override`
- Source: `include/exporters/streaming_exporter.h`:90
- Brief: n/a
- Parameters: none

#### `std::vector< std::string > getSupportedFormats() const override`
- Source: `include/exporters/streaming_exporter.h`:86
- Brief: n/a
- Parameters: none

#### `std::string getVersion() const override`
- Source: `include/exporters/streaming_exporter.h`:91
- Brief: n/a
- Parameters: none

#### `size_t readCheckpoint(const std::string &path)`
- Source: `include/exporters/streaming_exporter.h`:119
- Brief: Read Checkpoint.
- Parameters:
  - `path` (const std::string &): Input parameter.
- Return: Return value.
- Details: path Input parameter. Return value. path Input parameter. Return value. Calls: f(), is_open().

#### `void writeCheckpoint(const std::string &path, size_t offset)`
- Source: `include/exporters/streaming_exporter.h`:112
- Brief: Write Checkpoint.
- Parameters:
  - `path` (const std::string &): Input parameter.
  - `offset` (size_t): Input parameter.
- Details: path Input parameter. offset Input parameter. path Input parameter. offset Input parameter. Calls: tmp(), is_open(), THEMIS_WARN(), std::filesystem::rename(), message().

### themis::exporters::VectorExportCursor

#### `VectorExportCursor(const std::vector< BaseEntity > &entities, size_t page_size=1000)`
- Source: `include/exporters/streaming_exporter.h`:44
- Brief: n/a
- Parameters:
  - `entities` (const std::vector< BaseEntity > &): n/a
  - `page_size` (size_t): n/a

#### `size_t currentOffset() const override`
- Source: `include/exporters/streaming_exporter.h`:49
- Brief: n/a
- Parameters: none

#### `bool hasNext() const override`
- Source: `include/exporters/streaming_exporter.h`:46
- Brief: n/a
- Parameters: none

#### `std::vector< BaseEntity > nextPage() override`
- Source: `include/exporters/streaming_exporter.h`:47
- Brief: Next Page.
- Parameters: none
- Return: Return value.
- Details: Return value. Calls: std::min(), size(), page(), begin().

#### `bool seekTo(size_t offset) override`
- Source: `include/exporters/streaming_exporter.h`:50
- Brief: Seek To.
- Parameters:
  - `offset` (size_t): Input parameter.
- Return: True when the operation succeeds.
- Details: offset Input parameter. True when the operation succeeds. Calls: size().

#### `size_t totalCount() const override`
- Source: `include/exporters/streaming_exporter.h`:48
- Brief: n/a
- Parameters: none

