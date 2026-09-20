# TRAINING DOXYGEN

Author: ThemisDB Contributors
Created: 2026-09-20
Last Updated: 2026-09-20
Status: active

## Source
- This file is generated from module-scoped Doxygen XML.
- XML index: `C:\Projects\ThemisDB\.tmp\module_doxygen_global_e2e\training\xml\index.xml`
- Warnings log: `C:\Projects\ThemisDB\.tmp\module_doxygen_global_e2e\training\doxygen-warnings.log`

## Structure Metrics
- C/C++ Files (scanned): 60
- Compounds: 286
- Classes/Structs: 185
- Namespaces: 32
- File Compounds: 60

## Namespaces
- @035103364310234067031314020266162237253004034111
- @064131232112155210374202205231321173365217150034
- @105177243144272223067224055341123341042172042377
- @134177327164312220257356066113105321014024030314
- @251260161047165327134102240347331236160141333350
- @266326253255133230023037015033025271046044266326
- @317130012017053126216144232111275347027002005112
- @351377241121160244255100072041312253377172310242
- @353167352166063324020137236027245145204275330264
- gates
- std
- std::chrono_literals
- testing
- themis
- themis::@311102272032135057126032037243066123303010061340
- themis::analytics
- themis::llm::lora
- themis::query
- themis::sharding
- themis::training
- themis::training::@023116274235024336132332000203124121211051336062
- themis::training::@033367135005133133371145144171046233124257074202
- themis::training::@153131046371305372244072120266273120076340345251
- themis::training::@172260015270123206021262306325063237151340361043
- themis::training::@360142137027144076337206146172017153051030066105
- themis::training::@372204352027114372206272253163142153235077166245
- themis::training::aql_templates
- themis::training::checkpoint
- themis::training::detail
- themis::training::graph_aql
- themis::training::provenance_aql
- themis::training::yaml_detail

## Types
### Classes
- CheckpointRecoveryTest
- CheckpointResumeGpuStressTest
- CheckpointResumeStressTest
- CitationExtractorTest
- DataSelectionPipeline
- DataSelectionPipeline::Impl
- DatabaseDomainAutoLabeler
- DeployVersionExTest
- MergeConflictTest
- MockLLMRouter
- ModalityDetectorParseTest
- ModalityDetectorTest
- MultiAdapterConcurrentTest
- MultiTaskLoRAAcceptanceGatesTest
- RollbackVersionExTest
- RouterIntegrationTest
- SimpleCheckpointManager
- TableExtractorTest
- TextClauseExtractorTest
- TrainingLifecycleStressTest
- TrainingPipelineE2ETest
- TrainingServiceRegistryTest
- UnavailableLLMRouter
- themis::DataSelectionPipeline
- themis::DataSelectionPipeline::Impl
- themis::training::AdaLoRAAdapter
- themis::training::AdaLoRAAdapter::Impl
- themis::training::AdaLoraTTBridge
- themis::training::AdapterMergeFailsafe
- themis::training::CheckpointException
- themis::training::CheckpointFaultHandler
- themis::training::CitationExtractor
- themis::training::ConfidenceCalibrator
- themis::training::DataSelectionPipeline
- themis::training::DataSelectionPipeline::Impl
- themis::training::DatabaseDomainAutoLabeler
- themis::training::DatasetException
- themis::training::EnrichmentCache
- themis::training::EnrichmentException
- themis::training::EnrichmentGapHandler
- themis::training::EnrichmentLRUCache
- themis::training::IConfidenceCalibrator
- themis::training::IKGEnrichmentInterface
- themis::training::ILLMRouter
- themis::training::ILineageQueryAPI
- themis::training::ILoRACheckpointManager
- themis::training::ISampleProvenanceTracker
- themis::training::ITrainingPipeline
- themis::training::IncrementalLoRATrainer
- themis::training::IncrementalLoRATrainer::Impl
- themis::training::KnowledgeGraphEnricher
- themis::training::KnowledgeGraphEnricher::Impl
- themis::training::LabelingException
- themis::training::LegalAutoLabeler
- themis::training::LegalAutoLabeler::Impl
- themis::training::LoRAAdapter
- themis::training::LoRAAdapter::Impl
- themis::training::LoRAAdapterMerger
- themis::training::LoRACheckpointManager
- themis::training::LoRACheckpointManager::Impl
- themis::training::MergeException
- themis::training::ModalityDetector
- themis::training::ModalityDetector::Impl
- themis::training::MultiTaskLoRATrainer
- themis::training::MultiTaskLoRATrainer::Impl
- themis::training::OCRExtractor
- themis::training::ProvenanceException
- themis::training::ProvenanceTracker
- themis::training::ProvenanceTracker::Impl
- themis::training::SelfImprovementModule
- themis::training::SelfImprovementModule::Impl
- themis::training::ServingException
- themis::training::TableExtractor
- themis::training::TextClauseExtractor
- themis::training::TrainingDiagnostics
- themis::training::TrainingErrorLogger
- themis::training::TrainingException
- themis::training::TrainingFailureException
- themis::training::TrainingIncidentEmitter
- themis::training::TrainingIncidentListener
- themis::training::TrainingPipeline
- themis::training::TrainingPipeline::Impl
- themis::training::TrainingSession

### Structs
- DataSample
- FeedbackEntry
- LabeledDbSample
- LoRADataSelectionConfig
- SimpleCheckpointManager::CheckpointData
- themis::DataSample
- themis::LoRADataSelectionConfig
- themis::PipelineConfig
- themis::training::AcceptanceGateMetrics
- themis::training::AdaLoRAAdapter::Impl::Layer
- themis::training::AdaLoRALayerStats
- themis::training::AdaLoraTTBridge::BridgeStats
- themis::training::AdaLoraTTBridge::Impl
- themis::training::AdaLoraTTBridge::SimilarAdapter
- themis::training::AdaLoraTTBridgeConfig
- themis::training::AdaLoraTTExport
- themis::training::AdaLoraTTLayerExport
- themis::training::AdapterDescriptor
- themis::training::AdapterMergeFaultContext
- themis::training::AdapterMergeFaultResult
- themis::training::AdapterMergeSnapshot
- themis::training::AdaptiveRule
- themis::training::AsyncTrainingResult
- themis::training::AutoLabelConfig
- themis::training::CacheEntry
- themis::training::CacheStats
- themis::training::CalibratedThreshold
- themis::training::CalibrationDataset
- themis::training::CalibrationResult
- themis::training::CalibrationSample
- themis::training::CalibratorOutput
- themis::training::CheckpointDescriptor
- themis::training::CheckpointFaultContext
- themis::training::CheckpointFaultResult
- themis::training::CheckpointManagerConfig
- themis::training::CheckpointManifestEntry
- themis::training::ConfidenceCalibrator::Sample
- themis::training::DataQualityReport
- themis::training::DataSample
- themis::training::DataSelectionMetrics
- themis::training::DataSelectionResult
- themis::training::DeployResult
- themis::training::DomainGatingResult
- themis::training::DriftReport
- themis::training::EnrichmentCache::Stats
- themis::training::EnrichmentCacheConfig
- themis::training::EnrichmentCacheStats
- themis::training::EnrichmentConfig
- themis::training::EnrichmentGapContext
- themis::training::EnrichmentGapSummary
- themis::training::EnrichmentResult
- themis::training::EnrichmentStats
- themis::training::EntityRef
- themis::training::EpochMetrics
- themis::training::FeedbackEntry
- themis::training::GraphContext
- themis::training::HyperparamResult
- themis::training::HyperparamSearchConfig
- themis::training::HyperparamTrialResult
- themis::training::IncrementalTrainingConfig
- themis::training::KGRelation
- themis::training::LabeledDbSample
- themis::training::LabelingStats
- themis::training::LineageGraph
- themis::training::LineageNode
- themis::training::LoRADataSelectionConfig
- themis::training::LoRAWeightEntry
- themis::training::LoRAWeights
- themis::training::MTLSample
- themis::training::MTLTrainResult
- themis::training::MergeLayerResult
- themis::training::MergeResult
- themis::training::ModalityParseResult
- themis::training::ModalityParseStats
- themis::training::ModalityParserConfig
- themis::training::MultiTaskLoRAConfig
- themis::training::OriginRecord
- themis::training::PipelineConfig
- themis::training::PipelineMetrics
- themis::training::PipelineStats
- themis::training::PreprocessingStep
- themis::training::ProvenanceRecord
- themis::training::ProvenanceTrackerConfig
- themis::training::ProvenanceWriteStats
- themis::training::QuantizationConfig
- themis::training::ReallocResult
- themis::training::SampleProvenance
- themis::training::SampleRef
- themis::training::SelectionAuditEntry
- themis::training::SelfImprovementConfig
- themis::training::TaskConfig
- themis::training::TaskMetrics
- themis::training::TrainingIncident
- themis::training::TrainingJob
- themis::training::TrainingMetrics
- themis::training::TrainingResult
- themis::training::TrainingSample
- themis::training::TrainingSession::CleanupStats
- themis::training::VersionRecord
- themis::training::WeightUpdateBatch
- themis::training::WeightUpdateResult
- themis::training::detail::TableBlock

## Notes
- This is a derived artifact. Do not manually edit semantic content.
- Regenerate via scripts/generate_module_doxygen_xml.py --write-module-markdown.

## Detailed Function Documentation (Soll-Ist)
- Functions extracted: 1128

### CheckpointRecoveryTest

#### `void SetUp() override`
- Source: `tests/training/test_checkpoint_recovery.cpp`:38
- Brief: n/a
- Parameters: none

#### `void TearDown() override`
- Source: `tests/training/test_checkpoint_recovery.cpp`:47
- Brief: n/a
- Parameters: none

#### `std::vector< uint8_t > createDummyCheckpoint(size_t size)`
- Source: `tests/training/test_checkpoint_recovery.cpp`:65
- Brief: n/a
- Parameters:
  - `size` (size_t): n/a

#### `CheckpointManagerConfig makeConfig()`
- Source: `tests/training/test_checkpoint_recovery.cpp`:53
- Brief: n/a
- Parameters: none

#### `std::string writeCheckpointFile(const std::vector< uint8_t > &data, const std::string &filename)`
- Source: `tests/training/test_checkpoint_recovery.cpp`:75
- Brief: n/a
- Parameters:
  - `data` (const std::vector< uint8_t > &): n/a
  - `filename` (const std::string &): n/a

### CheckpointResumeGpuStressTest

#### `void SetUp() override`
- Source: `tests/training/test_checkpoint_resume_gpu_stress.cpp`:41
- Brief: n/a
- Parameters: none

#### `void TearDown() override`
- Source: `tests/training/test_checkpoint_resume_gpu_stress.cpp`:50
- Brief: n/a
- Parameters: none

#### `CheckpointManagerConfig makeConfig() const`
- Source: `tests/training/test_checkpoint_resume_gpu_stress.cpp`:56
- Brief: n/a
- Parameters: none

#### `fs::path writeSourceCheckpoint(size_t index, size_t bytes) const`
- Source: `tests/training/test_checkpoint_resume_gpu_stress.cpp`:65
- Brief: n/a
- Parameters:
  - `index` (size_t): n/a
  - `bytes` (size_t): n/a

### CheckpointResumeStressTest

#### `std::vector< float > generateRandomDelta(float scale=0.01f)`
- Source: `tests/training/test_checkpoint_resume_stress.cpp`:123
- Brief: n/a
- Parameters:
  - `scale` (float): n/a

#### `void trainAdapter(LoRAAdapter &adapter, int steps, const std::string &layer_name)`
- Source: `tests/training/test_checkpoint_resume_stress.cpp`:132
- Brief: n/a
- Parameters:
  - `adapter` (LoRAAdapter &): n/a
  - `steps` (int): n/a
  - `layer_name` (const std::string &): n/a

### CitationExtractorTest

#### `void SetUp() override`
- Source: `tests/training/test_training_convergence.cpp`:332
- Brief: n/a
- Parameters: none

### DataSample

#### `DataSample()=default`
- Source: `include/training/lora_data_selection.h`:137
- Brief: n/a
- Parameters: none

#### `DataSample(std::string id_, std::string text_)`
- Source: `include/training/lora_data_selection.h`:138
- Brief: n/a
- Parameters:
  - `id_` (std::string): n/a
  - `text_` (std::string): n/a

### DataSelectionPipeline

#### `DataSelectionPipeline(const DataSelectionPipeline &)=delete`
- Source: `include/training/lora_data_selection.h`:288
- Brief: n/a
- Parameters:
  - `<unnamed>` (const DataSelectionPipeline &): n/a

#### `DataSelectionPipeline(const LoRADataSelectionConfig &config)`
- Source: `include/training/lora_data_selection.h`:285
- Brief: Construct pipeline with the given configuration.
- Parameters:
  - `config` (const LoRADataSelectionConfig &): n/a

#### `std::vector< DataSample > clusterAndSample(const std::vector< DataSample > &samples, size_t k=0) const`
- Source: `include/training/lora_data_selection.h`:326
- Brief: Run only Stage 3: cluster-based diversity sampling.
- Parameters:
  - `samples` (const std::vector< DataSample > &): Samples to cluster.
  - `k` (size_t): Number of clusters (computed from config if 0).
- Return: Centroid-nearest samples covering diverse clusters.
- Details: samples Samples to cluster. k Number of clusters (computed from config if 0). Centroid-nearest samples covering diverse clusters.

#### `DataSelectionMetrics computeMetrics(const DataSelectionResult &result)`
- Source: `include/training/lora_data_selection.h`:368
- Brief: Derive a DataSelectionMetrics snapshot from a completed pipeline result.
- Parameters:
  - `result` (const DataSelectionResult &): Input parameter.
- Return: Populated metrics snapshot (all fields 0.0 for empty results).
- Details: Compute Metrics. Computes per-stage rejection rates and average quality/difficulty/ diversity scores from the returned samples. Useful for feeding the result directly into SelfImprovementModule::applyAdaptiveRules() and SelfImprovementModule::needsRollback(). result The result returned by a previous run() call. Populated metrics snapshot (all fields 0.0 for empty results). result Input parameter. Return value. Calls: DataSelectionMetrics(), empty(), ss(), reserve(), size(), std::isalpha(), std::tolower(), std::isdigit().

#### `std::vector< DataSample > deduplicate(const std::vector< DataSample > &samples) const`
- Source: `include/training/lora_data_selection.h`:317
- Brief: Run only Stage 2: MinHash deduplication.
- Parameters:
  - `samples` (const std::vector< DataSample > &): n/a
- Return: Samples with duplicates removed.
- Details: Samples with duplicates removed.

#### `std::vector< DataSample > filterByQuality(const std::vector< DataSample > &samples) const`
- Source: `include/training/lora_data_selection.h`:310
- Brief: Run only Stage 1: quality filtering.
- Parameters:
  - `samples` (const std::vector< DataSample > &): n/a
- Return: Samples that pass all quality filters.
- Details: Applies the shared prompt-safety policy before token/language/toxicity checks. Samples matching blocked prompt-injection patterns are rejected (fail-closed). Allowed samples continue with sanitized control-token redaction applied to their text. Samples that pass all quality filters.

#### `const LoRADataSelectionConfig & getConfig() const`
- Source: `include/training/lora_data_selection.h`:354
- Brief: Get the current pipeline configuration.
- Parameters: none

#### `DataSelectionPipeline & operator=(const DataSelectionPipeline &)=delete`
- Source: `include/training/lora_data_selection.h`:289
- Brief: n/a
- Parameters:
  - `<unnamed>` (const DataSelectionPipeline &): n/a

#### `DataSelectionResult run(const std::vector< DataSample > &input_samples, SelectionProgressCallback callback=nullptr)`
- Source: `include/training/lora_data_selection.h`:297
- Brief: Execute all five pipeline stages on input_samples.
- Parameters:
  - `input_samples` (const std::vector< DataSample > &): Input parameter.
  - `callback` (SelectionProgressCallback): Input parameter.
- Return: Selection result including selected samples and audit entry.
- Details: Run. input_samples Raw samples loaded from the training collection. callback Optional per-stage progress callback. Selection result including selected samples and audit entry. input_samples Input parameter. callback Input parameter. Return value. Calls: std::move(), getConfig(), empty(), detail::appendAuditJSONL(), toJSONL().

#### `void scoreQualityAndDifficulty(std::vector< DataSample > &samples) const`
- Source: `include/training/lora_data_selection.h`:334
- Brief: Run only Stage 4: quality/difficulty scoring. Modifies quality_score and difficulty_score in-place.
- Parameters:
  - `samples` (std::vector< DataSample > &): n/a

#### `void setConfig(const LoRADataSelectionConfig &config)`
- Source: `include/training/lora_data_selection.h`:349
- Brief: Update the pipeline configuration (live reload support).
- Parameters:
  - `config` (const LoRADataSelectionConfig &): Input parameter.
- Details: Set Config. config Input parameter. Implements setConfig without additional internal calls.

#### `std::vector< DataSample > stratifiedSample(const std::vector< DataSample > &scored_samples, size_t target=0) const`
- Source: `include/training/lora_data_selection.h`:342
- Brief: Run only Stage 5: curriculum stratified sampling.
- Parameters:
  - `scored_samples` (const std::vector< DataSample > &): Samples with difficulty_score populated.
  - `target` (size_t): Total samples to return (0 = use config).
- Return: Stratified subset (easy + medium + hard).
- Details: scored_samples Samples with difficulty_score populated. target Total samples to return (0 = use config). Stratified subset (easy + medium + hard).

#### `~DataSelectionPipeline()`
- Source: `include/training/lora_data_selection.h`:286
- Brief: n/a
- Parameters: none

### DataSelectionPipeline::Impl

#### `Impl(const LoRADataSelectionConfig &config)`
- Source: `src/training/lora_data_selection.cpp`:404
- Brief: n/a
- Parameters:
  - `config` (const LoRADataSelectionConfig &): n/a

#### `std::vector< DataSample > clusterAndSample(const std::vector< DataSample > &samples, size_t k) const`
- Source: `src/training/lora_data_selection.cpp`:506
- Brief: n/a
- Parameters:
  - `samples` (const std::vector< DataSample > &): n/a
  - `k` (size_t): n/a

#### `std::vector< DataSample > deduplicate(const std::vector< DataSample > &samples) const`
- Source: `src/training/lora_data_selection.cpp`:466
- Brief: n/a
- Parameters:
  - `samples` (const std::vector< DataSample > &): n/a

#### `std::vector< DataSample > filterByQuality(const std::vector< DataSample > &samples) const`
- Source: `src/training/lora_data_selection.cpp`:407
- Brief: n/a
- Parameters:
  - `samples` (const std::vector< DataSample > &): n/a

#### `const LoRADataSelectionConfig & getConfig() const`
- Source: `src/training/lora_data_selection.cpp`:798
- Brief: n/a
- Parameters: none

#### `DataSelectionResult run(const std::vector< DataSample > &input, SelectionProgressCallback cb)`
- Source: `src/training/lora_data_selection.cpp`:722
- Brief: - Full pipeline run ----------------------------------------------
- Parameters:
  - `input` (const std::vector< DataSample > &): Input parameter.
  - `cb` (SelectionProgressCallback): Input parameter.
- Return: Return value.
- Details: input Input parameter. cb Input parameter. Return value. Calls: DataSelectionResult(), std::chrono::steady_clock::now(), filterByQuality(), cb(), size(), deduplicate(), clusterAndSample(), scoreQualityAndDifficulty().

#### `void scoreQualityAndDifficulty(std::vector< DataSample > &samples) const`
- Source: `src/training/lora_data_selection.cpp`:628
- Brief: n/a
- Parameters:
  - `samples` (std::vector< DataSample > &): n/a

#### `void setConfig(const LoRADataSelectionConfig &config)`
- Source: `src/training/lora_data_selection.cpp`:797
- Brief: Set Config.
- Parameters:
  - `config` (const LoRADataSelectionConfig &): Input parameter.
- Details: config Input parameter. Implements setConfig without additional internal calls.

#### `std::vector< DataSample > stratifiedSample(const std::vector< DataSample > &scored, size_t target) const`
- Source: `src/training/lora_data_selection.cpp`:655
- Brief: n/a
- Parameters:
  - `scored` (const std::vector< DataSample > &): n/a
  - `target` (size_t): n/a

### DatabaseDomainAutoLabeler

#### `DatabaseDomainAutoLabeler(double sensitivity_ms=10.0)`
- Source: `include/training/database_domain_auto_labeler.h`:68
- Brief: Construct with optional sensitivity parameter.
- Parameters:
  - `sensitivity_ms` (double): Latency delta (ms) that maps to sigmoid inflection point (confidence = 0.73). Default: 10.0 ms.
- Details: sensitivity_ms Latency delta (ms) that maps to sigmoid inflection point (confidence = 0.73). Default: 10.0 ms.

#### `LabeledDbSample buildSample(const std::string &query, const std::string &plan_json, double delta_p99_ms, const std::string &source) const`
- Source: `include/training/database_domain_auto_labeler.h`:152
- Brief: Internal: build a LabeledDbSample from raw components.
- Parameters:
  - `query` (const std::string &): n/a
  - `plan_json` (const std::string &): n/a
  - `delta_p99_ms` (double): n/a
  - `source` (const std::string &): n/a

#### `double computeConfidence(double delta_p99_ms) const`
- Source: `include/training/database_domain_auto_labeler.h`:123
- Brief: Map a latency delta to a confidence score in [0.0, 1.0].
- Parameters:
  - `delta_p99_ms` (double): Measured latency improvement (sign is ignored).
- Return: Confidence in [0.5, 1.0). Returns exactly 0.5 when delta == 0.
- Details: confidence = sigmoid(\|delta_p99_ms\| / sensitivity_ms) delta_p99_ms Measured latency improvement (sign is ignored). Confidence in [0.5, 1.0). Returns exactly 0.5 when delta == 0.

#### `std::string exportToJsonl(const std::vector< LabeledDbSample > &samples)`
- Source: `include/training/database_domain_auto_labeler.h`:145
- Brief: Serialize a batch of labeled samples to a JSONL string.
- Parameters:
  - `samples` (const std::vector< LabeledDbSample > &): Input parameter.
- Return: JSONL string; empty string when samples is empty.
- Details: static Each sample is emitted as one compact JSON object on its own line: {"query":"SELECT…","explain_plan":"…","latency_delta_ms":-42.5} The output is suitable for appending to a .jsonl log file or piping to a downstream ingestion tool. samples Samples to serialize (may be empty). JSONL string; empty string when samples is empty. samples Input parameter. Return value.

#### `LabeledDbSample labelFromBaoDecision(const std::string &query, const std::string &bao_plan_json, double delta_p99_ms) const`
- Source: `include/training/database_domain_auto_labeler.h`:80
- Brief: Create a labeled sample from a BaoOptimizer decision log entry.
- Parameters:
  - `query` (const std::string &): SQL query string.
  - `bao_plan_json` (const std::string &): JSON-serialized BaoOptimizer plan.
  - `delta_p99_ms` (double): Latency improvement: negative = query got faster.
- Return: LabeledDbSample with source = "bao_log".
- Details: query SQL query string. bao_plan_json JSON-serialized BaoOptimizer plan. delta_p99_ms Latency improvement: negative = query got faster. LabeledDbSample with source = "bao_log".

#### `LabeledDbSample labelFromDBAFeedback(const FeedbackEntry &entry) const`
- Source: `include/training/database_domain_auto_labeler.h`:94
- Brief: Create a labeled sample from a DBA feedback entry.
- Parameters:
  - `entry` (const FeedbackEntry &): FeedbackEntry from the DBA or system operator.
- Return: LabeledDbSample with source = "dba_feedback".
- Details: Negative (is_positive=false) feedback receives confidence ≥ 0.9, reflecting high signal strength. entry FeedbackEntry from the DBA or system operator. LabeledDbSample with source = "dba_feedback".

#### `std::vector< LabeledDbSample > labelFromLogFile(const std::string &log_path, size_t max_samples=50000, double min_confidence=0.0) const`
- Source: `include/training/database_domain_auto_labeler.h`:108
- Brief: Batch-label samples from a query log file.
- Parameters:
  - `log_path` (const std::string &): Path to the query log file.
  - `max_samples` (size_t): Maximum number of samples to return (0 = unlimited).
  - `min_confidence` (double): Filter: samples below this threshold are discarded.
- Return: Vector of labeled samples (may be empty if file is missing / empty).
- Details: Each line in the file must be a JSON object with at least the fields "query", "plan", and "delta_p99_ms". Lines that cannot be parsed are silently skipped and counted in the returned stats. log_path Path to the query log file. max_samples Maximum number of samples to return (0 = unlimited). min_confidence Filter: samples below this threshold are discarded. Vector of labeled samples (may be empty if file is missing / empty).

#### `double sensitivityMs() const`
- Source: `include/training/database_domain_auto_labeler.h`:127
- Brief: n/a
- Parameters: none

### LoRADataSelectionConfig

#### `LoRADataSelectionConfig()=default`
- Source: `include/training/lora_data_selection.h`:84
- Brief: n/a
- Parameters: none

#### `LoRADataSelectionConfig fromYAMLString(const std::string &yaml_text, const std::string &section="lora_data_selection")`
- Source: `include/training/lora_data_selection.h`:111
- Brief: Parse configuration from an in-memory YAML string.
- Parameters:
  - `yaml_text` (const std::string &): Input parameter.
  - `section` (const std::string &): Input parameter.
- Return: Return value.
- Details: From YAMLString. Useful for unit testing or when the YAML content is already loaded. yaml_text YAML text containing the data-selection section. section Top-level key to read (default: lora_data_selection). yaml_text Input parameter. section Input parameter. Return value. Calls: yaml_detail::parseYAMLText().

#### `LoRADataSelectionConfig loadFromYAML(const std::string &path, const std::string &section="lora_data_selection")`
- Source: `include/training/lora_data_selection.h`:99
- Brief: Load configuration from a YAML file.
- Parameters:
  - `path` (const std::string &): Input parameter.
  - `section` (const std::string &): Input parameter.
- Return: Return value.
- Throws:
  - std::runtime_error: if the file cannot be opened.
  - std::runtime_error: if an error occurs.
- Details: Load From YAML. Reads the section block (default: lora_data_selection) from the YAML file at path and fills a new config object. Uses a built-in line-by-line parser – no external yaml-cpp dependency. Supports live-reload: call again at any time to obtain an updated config. path Path to the YAML configuration file. section Top-level YAML key containing the data-selection block. std::runtime_error if the file cannot be opened. path Input parameter. section Input parameter. Return value. std::runtime_error if an error occurs. Calls: f(), is_open(), rdbuf(), yaml_detail::parseYAMLText(), str().

### MergeConflictTest

#### `void SetUp() override`
- Source: `tests/training/test_merge_conflicts.cpp`:37
- Brief: n/a
- Parameters: none

#### `void initializeAdapterWeights(LoRAAdapter *adapter, float scale)`
- Source: `tests/training/test_merge_conflicts.cpp`:54
- Brief: n/a
- Parameters:
  - `adapter` (LoRAAdapter *): n/a
  - `scale` (float): n/a

### MockLLMRouter

#### `std::string activeVersion() const override`
- Source: `tests/training/test_training_phase2.cpp`:47
- Brief: Return the version identifier that currently receives 100% of traffic, or an empty string if no version is fully active.
- Parameters: none

#### `bool isAvailable() const override`
- Source: `tests/training/test_training_phase2.cpp`:46
- Brief: Whether the router is reachable and ready to accept weight updates.
- Parameters: none

#### `bool setAdapterWeight(const std::string &version, float weight) override`
- Source: `tests/training/test_training_phase2.cpp`:38
- Brief: Update the traffic weight for the named adapter version.
- Parameters:
  - `version` (const std::string &): Adapter version identifier (e.g., "legal_v1.1").
  - `weight` (float): Desired traffic fraction in [0.0, 1.0].
- Return: true on success; false if the version is unknown to the router (the caller may retry after loading the adapter).
- Details: Sets the fraction of incoming inference requests that should be routed to version. Implementations must normalise weights across all registered versions so that the total sums to 1.0. version Adapter version identifier (e.g., "legal_v1.1"). weight Desired traffic fraction in [0.0, 1.0]. true on success; false if the version is unknown to the router (the caller may retry after loading the adapter).

### ModalityDetectorParseTest

#### `void SetUp() override`
- Source: `tests/training/test_training_convergence.cpp`:390
- Brief: n/a
- Parameters: none

### ModalityDetectorTest

#### `void SetUp() override`
- Source: `tests/training/test_training_convergence.cpp`:165
- Brief: n/a
- Parameters: none

### MultiAdapterConcurrentTest

#### `std::vector< std::unique_ptr< LoRAAdapter > > createAdapters(int count)`
- Source: `tests/training/test_multi_adapter_concurrent.cpp`:51
- Brief: n/a
- Parameters:
  - `count` (int): n/a

#### `std::vector< float > generateRandomDelta(float scale=0.01f)`
- Source: `tests/training/test_multi_adapter_concurrent.cpp`:63
- Brief: n/a
- Parameters:
  - `scale` (float): n/a

### MultiTaskLoRAAcceptanceGatesTest

#### `void SetUp() override`
- Source: `tests/training/test_multitask_lora_acceptance_gates.cpp`:40
- Brief: n/a
- Parameters: none

#### `std::vector< MTLSample > createMultiTaskSamples(const std::vector< std::string > &task_ids, size_t samples_per_task=50, size_t input_dim=32)`
- Source: `tests/training/test_multitask_lora_acceptance_gates.cpp`:76
- Brief: n/a
- Parameters:
  - `task_ids` (const std::vector< std::string > &): n/a
  - `samples_per_task` (size_t): n/a
  - `input_dim` (size_t): n/a

#### `std::vector< MTLSample > createSimpleSamples(const std::string &task_id, size_t num_samples, size_t input_dim=32, size_t seed=42)`
- Source: `tests/training/test_multitask_lora_acceptance_gates.cpp`:50
- Brief: n/a
- Parameters:
  - `task_id` (const std::string &): n/a
  - `num_samples` (size_t): n/a
  - `input_dim` (size_t): n/a
  - `seed` (size_t): n/a

### SimpleCheckpointManager

#### `void clear()`
- Source: `tests/training/test_checkpoint_resume_stress.cpp`:97
- Brief: n/a
- Parameters: none

#### `size_t getCheckpointCount() const`
- Source: `tests/training/test_checkpoint_resume_stress.cpp`:93
- Brief: n/a
- Parameters: none

#### `size_t getLastEpoch() const`
- Source: `tests/training/test_checkpoint_resume_stress.cpp`:68
- Brief: n/a
- Parameters: none

#### `double getLastLoss() const`
- Source: `tests/training/test_checkpoint_resume_stress.cpp`:82
- Brief: n/a
- Parameters: none

#### `size_t getLastStep() const`
- Source: `tests/training/test_checkpoint_resume_stress.cpp`:75
- Brief: n/a
- Parameters: none

#### `bool hasCheckpoint() const`
- Source: `tests/training/test_checkpoint_resume_stress.cpp`:89
- Brief: n/a
- Parameters: none

#### `bool resumeCheckpoint(LoRAAdapter &adapter)`
- Source: `tests/training/test_checkpoint_resume_stress.cpp`:58
- Brief: n/a
- Parameters:
  - `adapter` (LoRAAdapter &): n/a

#### `void saveCheckpoint(const LoRAAdapter &adapter, size_t epoch, size_t step, double loss)`
- Source: `tests/training/test_checkpoint_resume_stress.cpp`:47
- Brief: n/a
- Parameters:
  - `adapter` (const LoRAAdapter &): n/a
  - `epoch` (size_t): n/a
  - `step` (size_t): n/a
  - `loss` (double): n/a

### TableExtractorTest

#### `void SetUp() override`
- Source: `tests/training/test_training_convergence.cpp`:290
- Brief: n/a
- Parameters: none

### TextClauseExtractorTest

#### `void SetUp() override`
- Source: `tests/training/test_training_convergence.cpp`:215
- Brief: n/a
- Parameters: none

### TrainingLifecycleStressTest

#### `std::vector< float > generateRandomDeltas(size_t size, float scale=0.01f)`
- Source: `tests/training/test_stress_training_lifecycle.cpp`:60
- Brief: n/a
- Parameters:
  - `size` (size_t): n/a
  - `scale` (float): n/a

#### `std::vector< float > generateRandomWeights(size_t size, float scale=0.1f)`
- Source: `tests/training/test_stress_training_lifecycle.cpp`:51
- Brief: n/a
- Parameters:
  - `size` (size_t): n/a
  - `scale` (float): n/a

### TrainingPipelineE2ETest

#### `void SetUp() override`
- Source: `tests/training/test_training_pipeline_e2e.cpp`:33
- Brief: n/a
- Parameters: none

### TrainingServiceRegistryTest

#### `void SetUp() override`
- Source: `tests/training/test_training_service_registry.cpp`:37
- Brief: n/a
- Parameters: none

#### `void TearDown() override`
- Source: `tests/training/test_training_service_registry.cpp`:42
- Brief: n/a
- Parameters: none

### UnavailableLLMRouter

#### `std::string activeVersion() const override`
- Source: `tests/training/test_training_phase2.cpp`:54
- Brief: Return the version identifier that currently receives 100% of traffic, or an empty string if no version is fully active.
- Parameters: none

#### `bool isAvailable() const override`
- Source: `tests/training/test_training_phase2.cpp`:53
- Brief: Whether the router is reachable and ready to accept weight updates.
- Parameters: none

#### `bool setAdapterWeight(const std::string &, float) override`
- Source: `tests/training/test_training_phase2.cpp`:52
- Brief: Update the traffic weight for the named adapter version.
- Parameters:
  - `version` (const std::string &): Adapter version identifier (e.g., "legal_v1.1").
  - `weight` (float): Desired traffic fraction in [0.0, 1.0].
- Return: true on success; false if the version is unknown to the router (the caller may retry after loading the adapter).
- Details: Sets the fraction of incoming inference requests that should be routed to version. Implementations must normalise weights across all registered versions so that the total sums to 1.0. version Adapter version identifier (e.g., "legal_v1.1"). weight Desired traffic fraction in [0.0, 1.0]. true on success; false if the version is unknown to the router (the caller may retry after loading the adapter).

### bench_training_dedicated_gates.cpp

#### `BENCHMARK_MAIN()`
- Source: `benchmarks/training/bench_training_dedicated_gates.cpp`:166
- Brief: n/a
- Parameters: none

#### `Repetitions(3) -> Unit(benchmark::kMillisecond)`
- Source: `benchmarks/training/bench_training_dedicated_gates.cpp`:163
- Brief: n/a
- Parameters:
  - `<unnamed>` (3): n/a

#### `Repetitions(5) -> Unit(benchmark::kNanosecond)`
- Source: `benchmarks/training/bench_training_dedicated_gates.cpp`:96
- Brief: n/a
- Parameters:
  - `<unnamed>` (5): n/a

#### `void TRBM01_BatchDispatchEnqueueLatency(benchmark::State &state)`
- Source: `benchmarks/training/bench_training_dedicated_gates.cpp`:84
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void TRBM02_CheckpointRoundTrip(benchmark::State &state)`
- Source: `benchmarks/training/bench_training_dedicated_gates.cpp`:102
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void TRBM03_GradientSyncThroughput(benchmark::State &state)`
- Source: `benchmarks/training/bench_training_dedicated_gates.cpp`:120
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void TRBM04_ConcurrentBatchDispatch(benchmark::State &state)`
- Source: `benchmarks/training/bench_training_dedicated_gates.cpp`:137
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

### bench_training_lifecycle_gates.cpp

#### `BENCHMARK(BM_TLG01_AdapterConstruct) -> Arg(64) ->Arg(256) ->Arg(768)`
- Source: `benchmarks/training/bench_training_lifecycle_gates.cpp`:108
- Brief: n/a
- Parameters:
  - `<unnamed>` (BM_TLG01_AdapterConstruct): n/a

#### `BENCHMARK(BM_TLG02_ApplyUpdate) -> Arg(64) ->Arg(256) ->Arg(768)`
- Source: `benchmarks/training/bench_training_lifecycle_gates.cpp`:137
- Brief: n/a
- Parameters:
  - `<unnamed>` (BM_TLG02_ApplyUpdate): n/a

#### `BENCHMARK(BM_TLG03_BatchUpdate)`
- Source: `benchmarks/training/bench_training_lifecycle_gates.cpp`:172
- Brief: n/a
- Parameters:
  - `<unnamed>` (BM_TLG03_BatchUpdate): n/a

#### `BENCHMARK(BM_TLG04_LinearMerge) -> Arg(64) ->Arg(256)`
- Source: `benchmarks/training/bench_training_lifecycle_gates.cpp`:209
- Brief: n/a
- Parameters:
  - `<unnamed>` (BM_TLG04_LinearMerge): n/a

#### `BENCHMARK(BM_TLG05_TIESMerge) -> Arg(64) ->Arg(256)`
- Source: `benchmarks/training/bench_training_lifecycle_gates.cpp`:245
- Brief: n/a
- Parameters:
  - `<unnamed>` (BM_TLG05_TIESMerge): n/a

#### `BENCHMARK(BM_TLG06_ExportWeights)`
- Source: `benchmarks/training/bench_training_lifecycle_gates.cpp`:275
- Brief: n/a
- Parameters:
  - `<unnamed>` (BM_TLG06_ExportWeights): n/a

#### `BENCHMARK(BM_TLG07_ImportWeights)`
- Source: `benchmarks/training/bench_training_lifecycle_gates.cpp`:307
- Brief: n/a
- Parameters:
  - `<unnamed>` (BM_TLG07_ImportWeights): n/a

#### `BENCHMARK(BM_TLG08_ThousandStepLifecycle)`
- Source: `benchmarks/training/bench_training_lifecycle_gates.cpp`:339
- Brief: n/a
- Parameters:
  - `<unnamed>` (BM_TLG08_ThousandStepLifecycle): n/a

#### `BENCHMARK_MAIN()`
- Source: `benchmarks/training/bench_training_lifecycle_gates.cpp`:341
- Brief: n/a
- Parameters: none

#### `void BM_TLG01_AdapterConstruct(benchmark::State &state)`
- Source: `benchmarks/training/bench_training_lifecycle_gates.cpp`:89
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_TLG02_ApplyUpdate(benchmark::State &state)`
- Source: `benchmarks/training/bench_training_lifecycle_gates.cpp`:113
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_TLG03_BatchUpdate(benchmark::State &state)`
- Source: `benchmarks/training/bench_training_lifecycle_gates.cpp`:142
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_TLG04_LinearMerge(benchmark::State &state)`
- Source: `benchmarks/training/bench_training_lifecycle_gates.cpp`:177
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_TLG05_TIESMerge(benchmark::State &state)`
- Source: `benchmarks/training/bench_training_lifecycle_gates.cpp`:214
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_TLG06_ExportWeights(benchmark::State &state)`
- Source: `benchmarks/training/bench_training_lifecycle_gates.cpp`:250
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_TLG07_ImportWeights(benchmark::State &state)`
- Source: `benchmarks/training/bench_training_lifecycle_gates.cpp`:280
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_TLG08_ThousandStepLifecycle(benchmark::State &state)`
- Source: `benchmarks/training/bench_training_lifecycle_gates.cpp`:312
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

### database_optimizer_labeler.cpp

#### `int main()`
- Source: `src/training/examples/database_optimizer_labeler.cpp`:100
- Brief: Main.
- Parameters: none
- Return: Return value.
- Details: Return value. Calls: simulateOptimizerLog(), computeOptimizerConfidence(), substr(), push_back(), std::move(), size(), assert(), PLANNED().

### gates

#### `void reportViolation(const char *gate_name, double measured, double target, const char *unit)`
- Source: `benchmarks/training/bench_training_lifecycle_gates.cpp`:55
- Brief: n/a
- Parameters:
  - `gate_name` (const char *): n/a
  - `measured` (double): n/a
  - `target` (double): n/a
  - `unit` (const char *): n/a

### test_adalora_edge_regressions.cpp

#### `TEST(TrainingAdaLoRAEdgeRegressions, AddLayerDeterministicInitializationByName)`
- Source: `tests/training/test_adalora_edge_regressions.cpp`:12
- Brief: n/a
- Parameters:
  - `<unnamed>` (TrainingAdaLoRAEdgeRegressions): n/a
  - `<unnamed>` (AddLayerDeterministicInitializationByName): n/a

#### `TEST(TrainingAdaLoRAEdgeRegressions, ReallocateRanksBudgetBelowLayerCountThrows)`
- Source: `tests/training/test_adalora_edge_regressions.cpp`:32
- Brief: n/a
- Parameters:
  - `<unnamed>` (TrainingAdaLoRAEdgeRegressions): n/a
  - `<unnamed>` (ReallocateRanksBudgetBelowLayerCountThrows): n/a

#### `TEST(TrainingAdaLoRAEdgeRegressions, ReallocateRanksZeroImportanceDeterministicDistribution)`
- Source: `tests/training/test_adalora_edge_regressions.cpp`:41
- Brief: n/a
- Parameters:
  - `<unnamed>` (TrainingAdaLoRAEdgeRegressions): n/a
  - `<unnamed>` (ReallocateRanksZeroImportanceDeterministicDistribution): n/a

### test_checkpoint_recovery.cpp

#### `TEST_F(CheckpointRecoveryTest, AllCheckpointsCorrupted_ReturnsEmpty)`
- Source: `tests/training/test_checkpoint_recovery.cpp`:175
- Brief: n/a
- Parameters:
  - `<unnamed>` (CheckpointRecoveryTest): n/a
  - `<unnamed>` (AllCheckpointsCorrupted_ReturnsEmpty): n/a

#### `TEST_F(CheckpointRecoveryTest, AtomicWrite_NoIncompleteFiles)`
- Source: `tests/training/test_checkpoint_recovery.cpp`:240
- Brief: n/a
- Parameters:
  - `<unnamed>` (CheckpointRecoveryTest): n/a
  - `<unnamed>` (AtomicWrite_NoIncompleteFiles): n/a

#### `TEST_F(CheckpointRecoveryTest, CorruptedCheckpoint_DetectedOnLoad)`
- Source: `tests/training/test_checkpoint_recovery.cpp`:133
- Brief: n/a
- Parameters:
  - `<unnamed>` (CheckpointRecoveryTest): n/a
  - `<unnamed>` (CorruptedCheckpoint_DetectedOnLoad): n/a

#### `TEST_F(CheckpointRecoveryTest, CorruptedManifest_RecoveryAttempted)`
- Source: `tests/training/test_checkpoint_recovery.cpp`:267
- Brief: n/a
- Parameters:
  - `<unnamed>` (CheckpointRecoveryTest): n/a
  - `<unnamed>` (CorruptedManifest_RecoveryAttempted): n/a

#### `TEST_F(CheckpointRecoveryTest, MaxCheckpoints_OldestPruned)`
- Source: `tests/training/test_checkpoint_recovery.cpp`:208
- Brief: n/a
- Parameters:
  - `<unnamed>` (CheckpointRecoveryTest): n/a
  - `<unnamed>` (MaxCheckpoints_OldestPruned): n/a

#### `TEST_F(CheckpointRecoveryTest, RecoveryStats_Tracked)`
- Source: `tests/training/test_checkpoint_recovery.cpp`:329
- Brief: n/a
- Parameters:
  - `<unnamed>` (CheckpointRecoveryTest): n/a
  - `<unnamed>` (RecoveryStats_Tracked): n/a

#### `TEST_F(CheckpointRecoveryTest, ResumeEmpty_ReturnsNone)`
- Source: `tests/training/test_checkpoint_recovery.cpp`:121
- Brief: n/a
- Parameters:
  - `<unnamed>` (CheckpointRecoveryTest): n/a
  - `<unnamed>` (ResumeEmpty_ReturnsNone): n/a

#### `TEST_F(CheckpointRecoveryTest, ResumeSpecific_ByEpoch)`
- Source: `tests/training/test_checkpoint_recovery.cpp`:382
- Brief: n/a
- Parameters:
  - `<unnamed>` (CheckpointRecoveryTest): n/a
  - `<unnamed>` (ResumeSpecific_ByEpoch): n/a

#### `TEST_F(CheckpointRecoveryTest, SaveAndResume_Succeeds)`
- Source: `tests/training/test_checkpoint_recovery.cpp`:95
- Brief: n/a
- Parameters:
  - `<unnamed>` (CheckpointRecoveryTest): n/a
  - `<unnamed>` (SaveAndResume_Succeeds): n/a

#### `TEST_F(CheckpointRecoveryTest, SequentialSaves_AllValid)`
- Source: `tests/training/test_checkpoint_recovery.cpp`:353
- Brief: n/a
- Parameters:
  - `<unnamed>` (CheckpointRecoveryTest): n/a
  - `<unnamed>` (SequentialSaves_AllValid): n/a

#### `TEST_F(CheckpointRecoveryTest, UnderMinimumSize_Rejected)`
- Source: `tests/training/test_checkpoint_recovery.cpp`:304
- Brief: n/a
- Parameters:
  - `<unnamed>` (CheckpointRecoveryTest): n/a
  - `<unnamed>` (UnderMinimumSize_Rejected): n/a

### test_checkpoint_resume_gpu_stress.cpp

#### `TEST_F(CheckpointResumeGpuStressTest, CorruptedLatestRollsBackUnderStress)`
- Source: `tests/training/test_checkpoint_resume_gpu_stress.cpp`:103
- Brief: n/a
- Parameters:
  - `<unnamed>` (CheckpointResumeGpuStressTest): n/a
  - `<unnamed>` (CorruptedLatestRollsBackUnderStress): n/a

#### `TEST_F(CheckpointResumeGpuStressTest, ResumeDeterministicAcrossRepeatedCalls)`
- Source: `tests/training/test_checkpoint_resume_gpu_stress.cpp`:76
- Brief: n/a
- Parameters:
  - `<unnamed>` (CheckpointResumeGpuStressTest): n/a
  - `<unnamed>` (ResumeDeterministicAcrossRepeatedCalls): n/a

### test_checkpoint_resume_stress.cpp

#### `TEST_F(CheckpointResumeStressTest, BasicCheckpointResume_Cycle)`
- Source: `tests/training/test_checkpoint_resume_stress.cpp`:145
- Brief: n/a
- Parameters:
  - `<unnamed>` (CheckpointResumeStressTest): n/a
  - `<unnamed>` (BasicCheckpointResume_Cycle): n/a

#### `TEST_F(CheckpointResumeStressTest, ExtendedSessionSimulation)`
- Source: `tests/training/test_checkpoint_resume_stress.cpp`:343
- Brief: n/a
- Parameters:
  - `<unnamed>` (CheckpointResumeStressTest): n/a
  - `<unnamed>` (ExtendedSessionSimulation): n/a

#### `TEST_F(CheckpointResumeStressTest, LargeScaleCycleStress)`
- Source: `tests/training/test_checkpoint_resume_stress.cpp`:495
- Brief: n/a
- Parameters:
  - `<unnamed>` (CheckpointResumeStressTest): n/a
  - `<unnamed>` (LargeScaleCycleStress): n/a

#### `TEST_F(CheckpointResumeStressTest, MetadataTracking_AcrossCycles)`
- Source: `tests/training/test_checkpoint_resume_stress.cpp`:426
- Brief: n/a
- Parameters:
  - `<unnamed>` (CheckpointResumeStressTest): n/a
  - `<unnamed>` (MetadataTracking_AcrossCycles): n/a

#### `TEST_F(CheckpointResumeStressTest, MultiLayerCheckpointCycles)`
- Source: `tests/training/test_checkpoint_resume_stress.cpp`:393
- Brief: n/a
- Parameters:
  - `<unnamed>` (CheckpointResumeStressTest): n/a
  - `<unnamed>` (MultiLayerCheckpointCycles): n/a

#### `TEST_F(CheckpointResumeStressTest, MultipleResumptions_Consistent)`
- Source: `tests/training/test_checkpoint_resume_stress.cpp`:312
- Brief: n/a
- Parameters:
  - `<unnamed>` (CheckpointResumeStressTest): n/a
  - `<unnamed>` (MultipleResumptions_Consistent): n/a

#### `TEST_F(CheckpointResumeStressTest, PerformanceUnderRepeatedCycles)`
- Source: `tests/training/test_checkpoint_resume_stress.cpp`:286
- Brief: n/a
- Parameters:
  - `<unnamed>` (CheckpointResumeStressTest): n/a
  - `<unnamed>` (PerformanceUnderRepeatedCycles): n/a

#### `TEST_F(CheckpointResumeStressTest, RepeatedCycles_StatePreserved)`
- Source: `tests/training/test_checkpoint_resume_stress.cpp`:179
- Brief: n/a
- Parameters:
  - `<unnamed>` (CheckpointResumeStressTest): n/a
  - `<unnamed>` (RepeatedCycles_StatePreserved): n/a

#### `TEST_F(CheckpointResumeStressTest, ResumeThenTrain_Deterministic)`
- Source: `tests/training/test_checkpoint_resume_stress.cpp`:208
- Brief: n/a
- Parameters:
  - `<unnamed>` (CheckpointResumeStressTest): n/a
  - `<unnamed>` (ResumeThenTrain_Deterministic): n/a

#### `TEST_F(CheckpointResumeStressTest, RollbackSimulation)`
- Source: `tests/training/test_checkpoint_resume_stress.cpp`:460
- Brief: n/a
- Parameters:
  - `<unnamed>` (CheckpointResumeStressTest): n/a
  - `<unnamed>` (RollbackSimulation): n/a

#### `TEST_F(CheckpointResumeStressTest, WeightConsistency_AcrossResume)`
- Source: `tests/training/test_checkpoint_resume_stress.cpp`:248
- Brief: n/a
- Parameters:
  - `<unnamed>` (CheckpointResumeStressTest): n/a
  - `<unnamed>` (WeightConsistency_AcrossResume): n/a

### test_deterministic_training_lifecycle.cpp

#### `TEST(DeterministicTrainingLifecycle, DTL_01_adapter_init_deterministic)`
- Source: `tests/training/test_deterministic_training_lifecycle.cpp`:76
- Brief: n/a
- Parameters:
  - `<unnamed>` (DeterministicTrainingLifecycle): n/a
  - `<unnamed>` (DTL_01_adapter_init_deterministic): n/a

#### `TEST(DeterministicTrainingLifecycle, DTL_02_addlayer_identical_shape)`
- Source: `tests/training/test_deterministic_training_lifecycle.cpp`:97
- Brief: n/a
- Parameters:
  - `<unnamed>` (DeterministicTrainingLifecycle): n/a
  - `<unnamed>` (DTL_02_addlayer_identical_shape): n/a

#### `TEST(DeterministicTrainingLifecycle, DTL_03_update_accumulation_deterministic)`
- Source: `tests/training/test_deterministic_training_lifecycle.cpp`:123
- Brief: n/a
- Parameters:
  - `<unnamed>` (DeterministicTrainingLifecycle): n/a
  - `<unnamed>` (DTL_03_update_accumulation_deterministic): n/a

#### `TEST(DeterministicTrainingLifecycle, DTL_04_batch_update_deterministic)`
- Source: `tests/training/test_deterministic_training_lifecycle.cpp`:153
- Brief: n/a
- Parameters:
  - `<unnamed>` (DeterministicTrainingLifecycle): n/a
  - `<unnamed>` (DTL_04_batch_update_deterministic): n/a

#### `TEST(DeterministicTrainingLifecycle, DTL_05_export_import_roundtrip)`
- Source: `tests/training/test_deterministic_training_lifecycle.cpp`:186
- Brief: n/a
- Parameters:
  - `<unnamed>` (DeterministicTrainingLifecycle): n/a
  - `<unnamed>` (DTL_05_export_import_roundtrip): n/a

#### `TEST(DeterministicTrainingLifecycle, DTL_06_multilayer_count_stable)`
- Source: `tests/training/test_deterministic_training_lifecycle.cpp`:217
- Brief: n/a
- Parameters:
  - `<unnamed>` (DeterministicTrainingLifecycle): n/a
  - `<unnamed>` (DTL_06_multilayer_count_stable): n/a

#### `TEST(DeterministicTrainingLifecycle, DTL_07_ties_merge_reproducible)`
- Source: `tests/training/test_deterministic_training_lifecycle.cpp`:237
- Brief: n/a
- Parameters:
  - `<unnamed>` (DeterministicTrainingLifecycle): n/a
  - `<unnamed>` (DTL_07_ties_merge_reproducible): n/a

#### `TEST(DeterministicTrainingLifecycle, DTL_08_linear_merge_reproducible)`
- Source: `tests/training/test_deterministic_training_lifecycle.cpp`:273
- Brief: n/a
- Parameters:
  - `<unnamed>` (DeterministicTrainingLifecycle): n/a
  - `<unnamed>` (DTL_08_linear_merge_reproducible): n/a

#### `TEST(DeterministicTrainingLifecycle, DTL_09_synthetic_loss_monotone)`
- Source: `tests/training/test_deterministic_training_lifecycle.cpp`:310
- Brief: n/a
- Parameters:
  - `<unnamed>` (DeterministicTrainingLifecycle): n/a
  - `<unnamed>` (DTL_09_synthetic_loss_monotone): n/a

#### `TEST(DeterministicTrainingLifecycle, DTL_10_thousand_steps_no_norm_growth)`
- Source: `tests/training/test_deterministic_training_lifecycle.cpp`:359
- Brief: n/a
- Parameters:
  - `<unnamed>` (DeterministicTrainingLifecycle): n/a
  - `<unnamed>` (DTL_10_thousand_steps_no_norm_growth): n/a

#### `TEST(DeterministicTrainingLifecycle, DTL_11_n_identical_updates_equals_n_scaled)`
- Source: `tests/training/test_deterministic_training_lifecycle.cpp`:387
- Brief: n/a
- Parameters:
  - `<unnamed>` (DeterministicTrainingLifecycle): n/a
  - `<unnamed>` (DTL_11_n_identical_updates_equals_n_scaled): n/a

#### `TEST(DeterministicTrainingLifecycle, DTL_12_incident_emitter_all_classes_ordered)`
- Source: `tests/training/test_deterministic_training_lifecycle.cpp`:433
- Brief: n/a
- Parameters:
  - `<unnamed>` (DeterministicTrainingLifecycle): n/a
  - `<unnamed>` (DTL_12_incident_emitter_all_classes_ordered): n/a

### test_enrichment_cache.cpp

#### `TEST(EnrichmentCacheTest, ClearEmpty_Safe)`
- Source: `tests/training/test_enrichment_cache.cpp`:501
- Brief: n/a
- Parameters:
  - `<unnamed>` (EnrichmentCacheTest): n/a
  - `<unnamed>` (ClearEmpty_Safe): n/a

#### `TEST(EnrichmentCacheTest, ConcurrentGets_Safe)`
- Source: `tests/training/test_enrichment_cache.cpp`:408
- Brief: n/a
- Parameters:
  - `<unnamed>` (EnrichmentCacheTest): n/a
  - `<unnamed>` (ConcurrentGets_Safe): n/a

#### `TEST(EnrichmentCacheTest, ConcurrentMixed_Safe)`
- Source: `tests/training/test_enrichment_cache.cpp`:436
- Brief: n/a
- Parameters:
  - `<unnamed>` (EnrichmentCacheTest): n/a
  - `<unnamed>` (ConcurrentMixed_Safe): n/a

#### `TEST(EnrichmentCacheTest, ConcurrentPuts_Safe)`
- Source: `tests/training/test_enrichment_cache.cpp`:385
- Brief: n/a
- Parameters:
  - `<unnamed>` (EnrichmentCacheTest): n/a
  - `<unnamed>` (ConcurrentPuts_Safe): n/a

#### `TEST(EnrichmentCacheTest, EmptyCache_Size)`
- Source: `tests/training/test_enrichment_cache.cpp`:496
- Brief: n/a
- Parameters:
  - `<unnamed>` (EnrichmentCacheTest): n/a
  - `<unnamed>` (EmptyCache_Size): n/a

#### `TEST(EnrichmentCacheTest, ExpirationStats_Tracked)`
- Source: `tests/training/test_enrichment_cache.cpp`:273
- Brief: n/a
- Parameters:
  - `<unnamed>` (EnrichmentCacheTest): n/a
  - `<unnamed>` (ExpirationStats_Tracked): n/a

#### `TEST(EnrichmentCacheTest, ExpiredEntry_NotReturned)`
- Source: `tests/training/test_enrichment_cache.cpp`:253
- Brief: n/a
- Parameters:
  - `<unnamed>` (EnrichmentCacheTest): n/a
  - `<unnamed>` (ExpiredEntry_NotReturned): n/a

#### `TEST(EnrichmentCacheTest, GetMissing_ReturnsFalse)`
- Source: `tests/training/test_enrichment_cache.cpp`:208
- Brief: n/a
- Parameters:
  - `<unnamed>` (EnrichmentCacheTest): n/a
  - `<unnamed>` (GetMissing_ReturnsFalse): n/a

#### `TEST(EnrichmentCacheTest, HitRate_AllMisses)`
- Source: `tests/training/test_enrichment_cache.cpp`:561
- Brief: n/a
- Parameters:
  - `<unnamed>` (EnrichmentCacheTest): n/a
  - `<unnamed>` (HitRate_AllMisses): n/a

#### `TEST(EnrichmentCacheTest, HitRate_Calculated)`
- Source: `tests/training/test_enrichment_cache.cpp`:543
- Brief: n/a
- Parameters:
  - `<unnamed>` (EnrichmentCacheTest): n/a
  - `<unnamed>` (HitRate_Calculated): n/a

#### `TEST(EnrichmentCacheTest, InvalidateAll_ClearsCache)`
- Source: `tests/training/test_enrichment_cache.cpp`:307
- Brief: n/a
- Parameters:
  - `<unnamed>` (EnrichmentCacheTest): n/a
  - `<unnamed>` (InvalidateAll_ClearsCache): n/a

#### `TEST(EnrichmentCacheTest, InvalidateEntry_Succeeds)`
- Source: `tests/training/test_enrichment_cache.cpp`:289
- Brief: n/a
- Parameters:
  - `<unnamed>` (EnrichmentCacheTest): n/a
  - `<unnamed>` (InvalidateEntry_Succeeds): n/a

#### `TEST(EnrichmentCacheTest, InvalidateMissing_ReturnsFalse)`
- Source: `tests/training/test_enrichment_cache.cpp`:301
- Brief: n/a
- Parameters:
  - `<unnamed>` (EnrichmentCacheTest): n/a
  - `<unnamed>` (InvalidateMissing_ReturnsFalse): n/a

#### `TEST(EnrichmentCacheTest, InvalidationStats_Tracked)`
- Source: `tests/training/test_enrichment_cache.cpp`:321
- Brief: n/a
- Parameters:
  - `<unnamed>` (EnrichmentCacheTest): n/a
  - `<unnamed>` (InvalidationStats_Tracked): n/a

#### `TEST(EnrichmentCacheTest, LRU_EvictsLeastRecentlyUsed)`
- Source: `tests/training/test_enrichment_cache.cpp`:471
- Brief: n/a
- Parameters:
  - `<unnamed>` (EnrichmentCacheTest): n/a
  - `<unnamed>` (LRU_EvictsLeastRecentlyUsed): n/a

#### `TEST(EnrichmentCacheTest, MarkStale_Tracked)`
- Source: `tests/training/test_enrichment_cache.cpp`:370
- Brief: n/a
- Parameters:
  - `<unnamed>` (EnrichmentCacheTest): n/a
  - `<unnamed>` (MarkStale_Tracked): n/a

#### `TEST(EnrichmentCacheTest, PutAndGet_Succeeds)`
- Source: `tests/training/test_enrichment_cache.cpp`:193
- Brief: n/a
- Parameters:
  - `<unnamed>` (EnrichmentCacheTest): n/a
  - `<unnamed>` (PutAndGet_Succeeds): n/a

#### `TEST(EnrichmentCacheTest, ShortTTL_QuickExpiration)`
- Source: `tests/training/test_enrichment_cache.cpp`:520
- Brief: n/a
- Parameters:
  - `<unnamed>` (EnrichmentCacheTest): n/a
  - `<unnamed>` (ShortTTL_QuickExpiration): n/a

#### `TEST(EnrichmentCacheTest, SizeBounded_EvictsOldest)`
- Source: `tests/training/test_enrichment_cache.cpp`:336
- Brief: n/a
- Parameters:
  - `<unnamed>` (EnrichmentCacheTest): n/a
  - `<unnamed>` (SizeBounded_EvictsOldest): n/a

#### `TEST(EnrichmentCacheTest, SizeBounded_MaxEnforced)`
- Source: `tests/training/test_enrichment_cache.cpp`:355
- Brief: n/a
- Parameters:
  - `<unnamed>` (EnrichmentCacheTest): n/a
  - `<unnamed>` (SizeBounded_MaxEnforced): n/a

#### `TEST(EnrichmentCacheTest, SmallMaxEntries_Enforced)`
- Source: `tests/training/test_enrichment_cache.cpp`:510
- Brief: n/a
- Parameters:
  - `<unnamed>` (EnrichmentCacheTest): n/a
  - `<unnamed>` (SmallMaxEntries_Enforced): n/a

#### `TEST(EnrichmentCacheTest, Stats_HitsAndMisses)`
- Source: `tests/training/test_enrichment_cache.cpp`:221
- Brief: n/a
- Parameters:
  - `<unnamed>` (EnrichmentCacheTest): n/a
  - `<unnamed>` (Stats_HitsAndMisses): n/a

#### `TEST(EnrichmentCacheTest, Stats_Puts)`
- Source: `tests/training/test_enrichment_cache.cpp`:237
- Brief: n/a
- Parameters:
  - `<unnamed>` (EnrichmentCacheTest): n/a
  - `<unnamed>` (Stats_Puts): n/a

### test_merge_conflicts.cpp

#### `TEST_F(MergeConflictTest, DeterministicMerge_SameWeights)`
- Source: `tests/training/test_merge_conflicts.cpp`:360
- Brief: n/a
- Parameters:
  - `<unnamed>` (MergeConflictTest): n/a
  - `<unnamed>` (DeterministicMerge_SameWeights): n/a

#### `TEST_F(MergeConflictTest, DifferentAlpha_Accepted)`
- Source: `tests/training/test_merge_conflicts.cpp`:386
- Brief: n/a
- Parameters:
  - `<unnamed>` (MergeConflictTest): n/a
  - `<unnamed>` (DifferentAlpha_Accepted): n/a

#### `TEST_F(MergeConflictTest, DimensionMismatch_InputDim_Fails)`
- Source: `tests/training/test_merge_conflicts.cpp`:119
- Brief: n/a
- Parameters:
  - `<unnamed>` (MergeConflictTest): n/a
  - `<unnamed>` (DimensionMismatch_InputDim_Fails): n/a

#### `TEST_F(MergeConflictTest, DimensionMismatch_OutputDim_Fails)`
- Source: `tests/training/test_merge_conflicts.cpp`:135
- Brief: n/a
- Parameters:
  - `<unnamed>` (MergeConflictTest): n/a
  - `<unnamed>` (DimensionMismatch_OutputDim_Fails): n/a

#### `TEST_F(MergeConflictTest, EmptyAdapterList_Fails)`
- Source: `tests/training/test_merge_conflicts.cpp`:278
- Brief: n/a
- Parameters:
  - `<unnamed>` (MergeConflictTest): n/a
  - `<unnamed>` (EmptyAdapterList_Fails): n/a

#### `TEST_F(MergeConflictTest, FailureMessagesPopulated)`
- Source: `tests/training/test_merge_conflicts.cpp`:305
- Brief: n/a
- Parameters:
  - `<unnamed>` (MergeConflictTest): n/a
  - `<unnamed>` (FailureMessagesPopulated): n/a

#### `TEST_F(MergeConflictTest, LinearMerge_OutputHasCorrectShape)`
- Source: `tests/training/test_merge_conflicts.cpp`:101
- Brief: n/a
- Parameters:
  - `<unnamed>` (MergeConflictTest): n/a
  - `<unnamed>` (LinearMerge_OutputHasCorrectShape): n/a

#### `TEST_F(MergeConflictTest, LinearMerge_ThreeAdapters_Succeeds)`
- Source: `tests/training/test_merge_conflicts.cpp`:86
- Brief: n/a
- Parameters:
  - `<unnamed>` (MergeConflictTest): n/a
  - `<unnamed>` (LinearMerge_ThreeAdapters_Succeeds): n/a

#### `TEST_F(MergeConflictTest, LinearMerge_TwoAdapters_Succeeds)`
- Source: `tests/training/test_merge_conflicts.cpp`:70
- Brief: n/a
- Parameters:
  - `<unnamed>` (MergeConflictTest): n/a
  - `<unnamed>` (LinearMerge_TwoAdapters_Succeeds): n/a

#### `TEST_F(MergeConflictTest, MergeResult_HasValidMetadata)`
- Source: `tests/training/test_merge_conflicts.cpp`:340
- Brief: n/a
- Parameters:
  - `<unnamed>` (MergeConflictTest): n/a
  - `<unnamed>` (MergeResult_HasValidMetadata): n/a

#### `TEST_F(MergeConflictTest, MultipleOutputLayers_Tracked)`
- Source: `tests/training/test_merge_conflicts.cpp`:256
- Brief: n/a
- Parameters:
  - `<unnamed>` (MergeConflictTest): n/a
  - `<unnamed>` (MultipleOutputLayers_Tracked): n/a

#### `TEST_F(MergeConflictTest, NullAdapterPointer_Handled)`
- Source: `tests/training/test_merge_conflicts.cpp`:323
- Brief: n/a
- Parameters:
  - `<unnamed>` (MergeConflictTest): n/a
  - `<unnamed>` (NullAdapterPointer_Handled): n/a

#### `TEST_F(MergeConflictTest, RankTooLarge_Fails)`
- Source: `tests/training/test_merge_conflicts.cpp`:155
- Brief: n/a
- Parameters:
  - `<unnamed>` (MergeConflictTest): n/a
  - `<unnamed>` (RankTooLarge_Fails): n/a

#### `TEST_F(MergeConflictTest, RankZero_Fails)`
- Source: `tests/training/test_merge_conflicts.cpp`:171
- Brief: n/a
- Parameters:
  - `<unnamed>` (MergeConflictTest): n/a
  - `<unnamed>` (RankZero_Fails): n/a

#### `TEST_F(MergeConflictTest, SingleAdapter_Succeeds)`
- Source: `tests/training/test_merge_conflicts.cpp`:288
- Brief: n/a
- Parameters:
  - `<unnamed>` (MergeConflictTest): n/a
  - `<unnamed>` (SingleAdapter_Succeeds): n/a

#### `TEST_F(MergeConflictTest, TIESMerge_BasicSucceeds)`
- Source: `tests/training/test_merge_conflicts.cpp`:205
- Brief: n/a
- Parameters:
  - `<unnamed>` (MergeConflictTest): n/a
  - `<unnamed>` (TIESMerge_BasicSucceeds): n/a

#### `TEST_F(MergeConflictTest, TIESMerge_OutputShape)`
- Source: `tests/training/test_merge_conflicts.cpp`:219
- Brief: n/a
- Parameters:
  - `<unnamed>` (MergeConflictTest): n/a
  - `<unnamed>` (TIESMerge_OutputShape): n/a

#### `TEST_F(MergeConflictTest, UnknownLayer_Skipped)`
- Source: `tests/training/test_merge_conflicts.cpp`:187
- Brief: n/a
- Parameters:
  - `<unnamed>` (MergeConflictTest): n/a
  - `<unnamed>` (UnknownLayer_Skipped): n/a

#### `TEST_F(MergeConflictTest, UnnormalizedWeights_Normalized)`
- Source: `tests/training/test_merge_conflicts.cpp`:237
- Brief: n/a
- Parameters:
  - `<unnamed>` (MergeConflictTest): n/a
  - `<unnamed>` (UnnormalizedWeights_Normalized): n/a

### test_multi_adapter_concurrent.cpp

#### `TEST_F(MultiAdapterConcurrentTest, AdapterState_Independent)`
- Source: `tests/training/test_multi_adapter_concurrent.cpp`:108
- Brief: n/a
- Parameters:
  - `<unnamed>` (MultiAdapterConcurrentTest): n/a
  - `<unnamed>` (AdapterState_Independent): n/a

#### `TEST_F(MultiAdapterConcurrentTest, AdapterState_NotShared)`
- Source: `tests/training/test_multi_adapter_concurrent.cpp`:513
- Brief: n/a
- Parameters:
  - `<unnamed>` (MultiAdapterConcurrentTest): n/a
  - `<unnamed>` (AdapterState_NotShared): n/a

#### `TEST_F(MultiAdapterConcurrentTest, ConcurrentBatchUpdates)`
- Source: `tests/training/test_multi_adapter_concurrent.cpp`:224
- Brief: n/a
- Parameters:
  - `<unnamed>` (MultiAdapterConcurrentTest): n/a
  - `<unnamed>` (ConcurrentBatchUpdates): n/a

#### `TEST_F(MultiAdapterConcurrentTest, ConcurrentForwardPasses_Safe)`
- Source: `tests/training/test_multi_adapter_concurrent.cpp`:154
- Brief: n/a
- Parameters:
  - `<unnamed>` (MultiAdapterConcurrentTest): n/a
  - `<unnamed>` (ConcurrentForwardPasses_Safe): n/a

#### `TEST_F(MultiAdapterConcurrentTest, ConcurrentLayerQueries)`
- Source: `tests/training/test_multi_adapter_concurrent.cpp`:348
- Brief: n/a
- Parameters:
  - `<unnamed>` (MultiAdapterConcurrentTest): n/a
  - `<unnamed>` (ConcurrentLayerQueries): n/a

#### `TEST_F(MultiAdapterConcurrentTest, ExportWhileConcurrentTraining)`
- Source: `tests/training/test_multi_adapter_concurrent.cpp`:265
- Brief: n/a
- Parameters:
  - `<unnamed>` (MultiAdapterConcurrentTest): n/a
  - `<unnamed>` (ExportWhileConcurrentTraining): n/a

#### `TEST_F(MultiAdapterConcurrentTest, HighConcurrency_Stable)`
- Source: `tests/training/test_multi_adapter_concurrent.cpp`:390
- Brief: n/a
- Parameters:
  - `<unnamed>` (MultiAdapterConcurrentTest): n/a
  - `<unnamed>` (HighConcurrency_Stable): n/a

#### `TEST_F(MultiAdapterConcurrentTest, InterleavedUpdatesAndForwards)`
- Source: `tests/training/test_multi_adapter_concurrent.cpp`:188
- Brief: n/a
- Parameters:
  - `<unnamed>` (MultiAdapterConcurrentTest): n/a
  - `<unnamed>` (InterleavedUpdatesAndForwards): n/a

#### `TEST_F(MultiAdapterConcurrentTest, LoadDistribution_Balanced)`
- Source: `tests/training/test_multi_adapter_concurrent.cpp`:309
- Brief: n/a
- Parameters:
  - `<unnamed>` (MultiAdapterConcurrentTest): n/a
  - `<unnamed>` (LoadDistribution_Balanced): n/a

#### `TEST_F(MultiAdapterConcurrentTest, MultiAdapter_ConcurrentTraining)`
- Source: `tests/training/test_multi_adapter_concurrent.cpp`:77
- Brief: n/a
- Parameters:
  - `<unnamed>` (MultiAdapterConcurrentTest): n/a
  - `<unnamed>` (MultiAdapter_ConcurrentTraining): n/a

#### `TEST_F(MultiAdapterConcurrentTest, NoDataRaces_StressTest)`
- Source: `tests/training/test_multi_adapter_concurrent.cpp`:471
- Brief: n/a
- Parameters:
  - `<unnamed>` (MultiAdapterConcurrentTest): n/a
  - `<unnamed>` (NoDataRaces_StressTest): n/a

#### `TEST_F(MultiAdapterConcurrentTest, NoDeadlock_TimedCompletion)`
- Source: `tests/training/test_multi_adapter_concurrent.cpp`:437
- Brief: n/a
- Parameters:
  - `<unnamed>` (MultiAdapterConcurrentTest): n/a
  - `<unnamed>` (NoDeadlock_TimedCompletion): n/a

#### `TEST_F(MultiAdapterConcurrentTest, ParameterCountConsistency)`
- Source: `tests/training/test_multi_adapter_concurrent.cpp`:375
- Brief: n/a
- Parameters:
  - `<unnamed>` (MultiAdapterConcurrentTest): n/a
  - `<unnamed>` (ParameterCountConsistency): n/a

### test_multitask_lora_acceptance_gates.cpp

#### `TEST_F(MultiTaskLoRAAcceptanceGatesTest, ErrorOnEmptySamples)`
- Source: `tests/training/test_multitask_lora_acceptance_gates.cpp`:461
- Brief: n/a
- Parameters:
  - `<unnamed>` (MultiTaskLoRAAcceptanceGatesTest): n/a
  - `<unnamed>` (ErrorOnEmptySamples): n/a

#### `TEST_F(MultiTaskLoRAAcceptanceGatesTest, ErrorOnForwardBeforeTrain)`
- Source: `tests/training/test_multitask_lora_acceptance_gates.cpp`:444
- Brief: n/a
- Parameters:
  - `<unnamed>` (MultiTaskLoRAAcceptanceGatesTest): n/a
  - `<unnamed>` (ErrorOnForwardBeforeTrain): n/a

#### `TEST_F(MultiTaskLoRAAcceptanceGatesTest, ErrorOnGatingBeforeTrain)`
- Source: `tests/training/test_multitask_lora_acceptance_gates.cpp`:450
- Brief: n/a
- Parameters:
  - `<unnamed>` (MultiTaskLoRAAcceptanceGatesTest): n/a
  - `<unnamed>` (ErrorOnGatingBeforeTrain): n/a

#### `TEST_F(MultiTaskLoRAAcceptanceGatesTest, ErrorOnUnknownTaskID)`
- Source: `tests/training/test_multitask_lora_acceptance_gates.cpp`:470
- Brief: n/a
- Parameters:
  - `<unnamed>` (MultiTaskLoRAAcceptanceGatesTest): n/a
  - `<unnamed>` (ErrorOnUnknownTaskID): n/a

#### `TEST_F(MultiTaskLoRAAcceptanceGatesTest, ErrorOnValidateBeforeTrain)`
- Source: `tests/training/test_multitask_lora_acceptance_gates.cpp`:456
- Brief: n/a
- Parameters:
  - `<unnamed>` (MultiTaskLoRAAcceptanceGatesTest): n/a
  - `<unnamed>` (ErrorOnValidateBeforeTrain): n/a

#### `TEST_F(MultiTaskLoRAAcceptanceGatesTest, MTL_01_SingleTaskForwardPass)`
- Source: `tests/training/test_multitask_lora_acceptance_gates.cpp`:96
- Brief: n/a
- Parameters:
  - `<unnamed>` (MultiTaskLoRAAcceptanceGatesTest): n/a
  - `<unnamed>` (MTL_01_SingleTaskForwardPass): n/a

#### `TEST_F(MultiTaskLoRAAcceptanceGatesTest, MTL_02_TwoTaskForwardPass)`
- Source: `tests/training/test_multitask_lora_acceptance_gates.cpp`:115
- Brief: n/a
- Parameters:
  - `<unnamed>` (MultiTaskLoRAAcceptanceGatesTest): n/a
  - `<unnamed>` (MTL_02_TwoTaskForwardPass): n/a

#### `TEST_F(MultiTaskLoRAAcceptanceGatesTest, MTL_03_ThreeTaskForwardPass)`
- Source: `tests/training/test_multitask_lora_acceptance_gates.cpp`:136
- Brief: n/a
- Parameters:
  - `<unnamed>` (MultiTaskLoRAAcceptanceGatesTest): n/a
  - `<unnamed>` (MTL_03_ThreeTaskForwardPass): n/a

#### `TEST_F(MultiTaskLoRAAcceptanceGatesTest, MTL_04_DomainGatingRoutingCorrectness)`
- Source: `tests/training/test_multitask_lora_acceptance_gates.cpp`:153
- Brief: n/a
- Parameters:
  - `<unnamed>` (MultiTaskLoRAAcceptanceGatesTest): n/a
  - `<unnamed>` (MTL_04_DomainGatingRoutingCorrectness): n/a

#### `TEST_F(MultiTaskLoRAAcceptanceGatesTest, MTL_05_GatingLatencyShouldBeFast)`
- Source: `tests/training/test_multitask_lora_acceptance_gates.cpp`:176
- Brief: n/a
- Parameters:
  - `<unnamed>` (MultiTaskLoRAAcceptanceGatesTest): n/a
  - `<unnamed>` (MTL_05_GatingLatencyShouldBeFast): n/a

#### `TEST_F(MultiTaskLoRAAcceptanceGatesTest, MTL_06_GatingConfidenceScoreRange)`
- Source: `tests/training/test_multitask_lora_acceptance_gates.cpp`:202
- Brief: n/a
- Parameters:
  - `<unnamed>` (MultiTaskLoRAAcceptanceGatesTest): n/a
  - `<unnamed>` (MTL_06_GatingConfidenceScoreRange): n/a

#### `TEST_F(MultiTaskLoRAAcceptanceGatesTest, MTL_07_ForwardPassConsistency)`
- Source: `tests/training/test_multitask_lora_acceptance_gates.cpp`:226
- Brief: n/a
- Parameters:
  - `<unnamed>` (MultiTaskLoRAAcceptanceGatesTest): n/a
  - `<unnamed>` (MTL_07_ForwardPassConsistency): n/a

#### `TEST_F(MultiTaskLoRAAcceptanceGatesTest, MTL_08_ExportWeightsValidity)`
- Source: `tests/training/test_multitask_lora_acceptance_gates.cpp`:241
- Brief: n/a
- Parameters:
  - `<unnamed>` (MultiTaskLoRAAcceptanceGatesTest): n/a
  - `<unnamed>` (MTL_08_ExportWeightsValidity): n/a

#### `TEST_F(MultiTaskLoRAAcceptanceGatesTest, MTL_09_JointLossDecreasesOverEpochs)`
- Source: `tests/training/test_multitask_lora_acceptance_gates.cpp`:264
- Brief: n/a
- Parameters:
  - `<unnamed>` (MultiTaskLoRAAcceptanceGatesTest): n/a
  - `<unnamed>` (MTL_09_JointLossDecreasesOverEpochs): n/a

#### `TEST_F(MultiTaskLoRAAcceptanceGatesTest, MTL_10_MultiTaskConvergenceWithUnbalancedWeights)`
- Source: `tests/training/test_multitask_lora_acceptance_gates.cpp`:285
- Brief: n/a
- Parameters:
  - `<unnamed>` (MultiTaskLoRAAcceptanceGatesTest): n/a
  - `<unnamed>` (MTL_10_MultiTaskConvergenceWithUnbalancedWeights): n/a

#### `TEST_F(MultiTaskLoRAAcceptanceGatesTest, MTL_11_ConvergenceAcrossTaskWeights)`
- Source: `tests/training/test_multitask_lora_acceptance_gates.cpp`:302
- Brief: n/a
- Parameters:
  - `<unnamed>` (MultiTaskLoRAAcceptanceGatesTest): n/a
  - `<unnamed>` (MTL_11_ConvergenceAcrossTaskWeights): n/a

#### `TEST_F(MultiTaskLoRAAcceptanceGatesTest, MTL_12_PerTaskMetricsAccuracy)`
- Source: `tests/training/test_multitask_lora_acceptance_gates.cpp`:324
- Brief: n/a
- Parameters:
  - `<unnamed>` (MultiTaskLoRAAcceptanceGatesTest): n/a
  - `<unnamed>` (MTL_12_PerTaskMetricsAccuracy): n/a

#### `TEST_F(MultiTaskLoRAAcceptanceGatesTest, MTL_13_AblationStudySharedVsSingleTaskBaseline)`
- Source: `tests/training/test_multitask_lora_acceptance_gates.cpp`:350
- Brief: n/a
- Parameters:
  - `<unnamed>` (MultiTaskLoRAAcceptanceGatesTest): n/a
  - `<unnamed>` (MTL_13_AblationStudySharedVsSingleTaskBaseline): n/a

#### `TEST_F(MultiTaskLoRAAcceptanceGatesTest, WaveB_AcceptanceGatesMetricsPopulated)`
- Source: `tests/training/test_multitask_lora_acceptance_gates.cpp`:382
- Brief: n/a
- Parameters:
  - `<unnamed>` (MultiTaskLoRAAcceptanceGatesTest): n/a
  - `<unnamed>` (WaveB_AcceptanceGatesMetricsPopulated): n/a

#### `TEST_F(MultiTaskLoRAAcceptanceGatesTest, WaveB_ThreeTaskBenchmarkGates)`
- Source: `tests/training/test_multitask_lora_acceptance_gates.cpp`:424
- Brief: n/a
- Parameters:
  - `<unnamed>` (MultiTaskLoRAAcceptanceGatesTest): n/a
  - `<unnamed>` (WaveB_ThreeTaskBenchmarkGates): n/a

#### `TEST_F(MultiTaskLoRAAcceptanceGatesTest, WaveB_ValidateAcceptanceGatesMethod)`
- Source: `tests/training/test_multitask_lora_acceptance_gates.cpp`:403
- Brief: n/a
- Parameters:
  - `<unnamed>` (MultiTaskLoRAAcceptanceGatesTest): n/a
  - `<unnamed>` (WaveB_ValidateAcceptanceGatesMethod): n/a

### test_stress_training_lifecycle.cpp

#### `TEST_F(TrainingLifecycleStressTest, ConcurrentLayerNames_Consistent)`
- Source: `tests/training/test_stress_training_lifecycle.cpp`:371
- Brief: n/a
- Parameters:
  - `<unnamed>` (TrainingLifecycleStressTest): n/a
  - `<unnamed>` (ConcurrentLayerNames_Consistent): n/a

#### `TEST_F(TrainingLifecycleStressTest, ExportImport_RoundTrip)`
- Source: `tests/training/test_stress_training_lifecycle.cpp`:280
- Brief: n/a
- Parameters:
  - `<unnamed>` (TrainingLifecycleStressTest): n/a
  - `<unnamed>` (ExportImport_RoundTrip): n/a

#### `TEST_F(TrainingLifecycleStressTest, MemoryStability_LongSession)`
- Source: `tests/training/test_stress_training_lifecycle.cpp`:324
- Brief: n/a
- Parameters:
  - `<unnamed>` (TrainingLifecycleStressTest): n/a
  - `<unnamed>` (MemoryStability_LongSession): n/a

#### `TEST_F(TrainingLifecycleStressTest, MultiLayer_ConcurrentUpdates)`
- Source: `tests/training/test_stress_training_lifecycle.cpp`:146
- Brief: n/a
- Parameters:
  - `<unnamed>` (TrainingLifecycleStressTest): n/a
  - `<unnamed>` (MultiLayer_ConcurrentUpdates): n/a

#### `TEST_F(TrainingLifecycleStressTest, MultiLayer_LayerRemovalStress)`
- Source: `tests/training/test_stress_training_lifecycle.cpp`:173
- Brief: n/a
- Parameters:
  - `<unnamed>` (TrainingLifecycleStressTest): n/a
  - `<unnamed>` (MultiLayer_LayerRemovalStress): n/a

#### `TEST_F(TrainingLifecycleStressTest, MultiLayer_SequentialAddition)`
- Source: `tests/training/test_stress_training_lifecycle.cpp`:134
- Brief: n/a
- Parameters:
  - `<unnamed>` (TrainingLifecycleStressTest): n/a
  - `<unnamed>` (MultiLayer_SequentialAddition): n/a

#### `TEST_F(TrainingLifecycleStressTest, NoLeakOnLayerAddRemove)`
- Source: `tests/training/test_stress_training_lifecycle.cpp`:343
- Brief: n/a
- Parameters:
  - `<unnamed>` (TrainingLifecycleStressTest): n/a
  - `<unnamed>` (NoLeakOnLayerAddRemove): n/a

#### `TEST_F(TrainingLifecycleStressTest, ParameterCount_Accurate)`
- Source: `tests/training/test_stress_training_lifecycle.cpp`:264
- Brief: n/a
- Parameters:
  - `<unnamed>` (TrainingLifecycleStressTest): n/a
  - `<unnamed>` (ParameterCount_Accurate): n/a

#### `TEST_F(TrainingLifecycleStressTest, RecoveryFromErrors_ContinuesTraining)`
- Source: `tests/training/test_stress_training_lifecycle.cpp`:432
- Brief: n/a
- Parameters:
  - `<unnamed>` (TrainingLifecycleStressTest): n/a
  - `<unnamed>` (RecoveryFromErrors_ContinuesTraining): n/a

#### `TEST_F(TrainingLifecycleStressTest, SingleAdapter_BatchUpdatesStress)`
- Source: `tests/training/test_stress_training_lifecycle.cpp`:96
- Brief: n/a
- Parameters:
  - `<unnamed>` (TrainingLifecycleStressTest): n/a
  - `<unnamed>` (SingleAdapter_BatchUpdatesStress): n/a

#### `TEST_F(TrainingLifecycleStressTest, SingleAdapter_ExtendedTraining)`
- Source: `tests/training/test_stress_training_lifecycle.cpp`:74
- Brief: n/a
- Parameters:
  - `<unnamed>` (TrainingLifecycleStressTest): n/a
  - `<unnamed>` (SingleAdapter_ExtendedTraining): n/a

#### `TEST_F(TrainingLifecycleStressTest, SingleAdapter_ForwardPassStress)`
- Source: `tests/training/test_stress_training_lifecycle.cpp`:111
- Brief: n/a
- Parameters:
  - `<unnamed>` (TrainingLifecycleStressTest): n/a
  - `<unnamed>` (SingleAdapter_ForwardPassStress): n/a

#### `TEST_F(TrainingLifecycleStressTest, TrainingPipeline_Realistic)`
- Source: `tests/training/test_stress_training_lifecycle.cpp`:393
- Brief: n/a
- Parameters:
  - `<unnamed>` (TrainingLifecycleStressTest): n/a
  - `<unnamed>` (TrainingPipeline_Realistic): n/a

#### `TEST_F(TrainingLifecycleStressTest, WeightAccumulation_Linear)`
- Source: `tests/training/test_stress_training_lifecycle.cpp`:233
- Brief: n/a
- Parameters:
  - `<unnamed>` (TrainingLifecycleStressTest): n/a
  - `<unnamed>` (WeightAccumulation_Linear): n/a

#### `TEST_F(TrainingLifecycleStressTest, WeightConvergence_Deterministic)`
- Source: `tests/training/test_stress_training_lifecycle.cpp`:203
- Brief: n/a
- Parameters:
  - `<unnamed>` (TrainingLifecycleStressTest): n/a
  - `<unnamed>` (WeightConvergence_Deterministic): n/a

### test_training_cancellation.cpp

#### `TEST(TrainingCancellationTest, CancelAfterCheckpoint_PreservesCheckpoint)`
- Source: `tests/training/test_training_cancellation.cpp`:267
- Brief: n/a
- Parameters:
  - `<unnamed>` (TrainingCancellationTest): n/a
  - `<unnamed>` (CancelAfterCheckpoint_PreservesCheckpoint): n/a

#### `TEST(TrainingCancellationTest, CancelBeforeCheckpoint_NoCheckpoint)`
- Source: `tests/training/test_training_cancellation.cpp`:254
- Brief: n/a
- Parameters:
  - `<unnamed>` (TrainingCancellationTest): n/a
  - `<unnamed>` (CancelBeforeCheckpoint_NoCheckpoint): n/a

#### `TEST(TrainingCancellationTest, CancelBeforeCompletion_StopsTraining)`
- Source: `tests/training/test_training_cancellation.cpp`:425
- Brief: n/a
- Parameters:
  - `<unnamed>` (TrainingCancellationTest): n/a
  - `<unnamed>` (CancelBeforeCompletion_StopsTraining): n/a

#### `TEST(TrainingCancellationTest, CancelBeforeStart_Safe)`
- Source: `tests/training/test_training_cancellation.cpp`:361
- Brief: n/a
- Parameters:
  - `<unnamed>` (TrainingCancellationTest): n/a
  - `<unnamed>` (CancelBeforeStart_Safe): n/a

#### `TEST(TrainingCancellationTest, CancelDuringTraining_Safe)`
- Source: `tests/training/test_training_cancellation.cpp`:324
- Brief: n/a
- Parameters:
  - `<unnamed>` (TrainingCancellationTest): n/a
  - `<unnamed>` (CancelDuringTraining_Safe): n/a

#### `TEST(TrainingCancellationTest, CancelMultipleTimes_NoThrow)`
- Source: `tests/training/test_training_cancellation.cpp`:440
- Brief: n/a
- Parameters:
  - `<unnamed>` (TrainingCancellationTest): n/a
  - `<unnamed>` (CancelMultipleTimes_NoThrow): n/a

#### `TEST(TrainingCancellationTest, CancelledSessionStopsTraining)`
- Source: `tests/training/test_training_cancellation.cpp`:182
- Brief: n/a
- Parameters:
  - `<unnamed>` (TrainingCancellationTest): n/a
  - `<unnamed>` (CancelledSessionStopsTraining): n/a

#### `TEST(TrainingCancellationTest, CleanupTiming_Reasonable)`
- Source: `tests/training/test_training_cancellation.cpp`:376
- Brief: n/a
- Parameters:
  - `<unnamed>` (TrainingCancellationTest): n/a
  - `<unnamed>` (CleanupTiming_Reasonable): n/a

#### `TEST(TrainingCancellationTest, Cleanup_ResourcesFreed)`
- Source: `tests/training/test_training_cancellation.cpp`:205
- Brief: n/a
- Parameters:
  - `<unnamed>` (TrainingCancellationTest): n/a
  - `<unnamed>` (Cleanup_ResourcesFreed): n/a

#### `TEST(TrainingCancellationTest, Cleanup_StatsReported)`
- Source: `tests/training/test_training_cancellation.cpp`:216
- Brief: n/a
- Parameters:
  - `<unnamed>` (TrainingCancellationTest): n/a
  - `<unnamed>` (Cleanup_StatsReported): n/a

#### `TEST(TrainingCancellationTest, CompletionBeforeCancel_PreservesState)`
- Source: `tests/training/test_training_cancellation.cpp`:413
- Brief: n/a
- Parameters:
  - `<unnamed>` (TrainingCancellationTest): n/a
  - `<unnamed>` (CompletionBeforeCancel_PreservesState): n/a

#### `TEST(TrainingCancellationTest, ConcurrentCancels_Safe)`
- Source: `tests/training/test_training_cancellation.cpp`:304
- Brief: n/a
- Parameters:
  - `<unnamed>` (TrainingCancellationTest): n/a
  - `<unnamed>` (ConcurrentCancels_Safe): n/a

#### `TEST(TrainingCancellationTest, GracefulShutdown_AllResourcesReleased)`
- Source: `tests/training/test_training_cancellation.cpp`:286
- Brief: n/a
- Parameters:
  - `<unnamed>` (TrainingCancellationTest): n/a
  - `<unnamed>` (GracefulShutdown_AllResourcesReleased): n/a

#### `TEST(TrainingCancellationTest, MultipleCancels_Idempotent)`
- Source: `tests/training/test_training_cancellation.cpp`:233
- Brief: n/a
- Parameters:
  - `<unnamed>` (TrainingCancellationTest): n/a
  - `<unnamed>` (MultipleCancels_Idempotent): n/a

#### `TEST(TrainingCancellationTest, MultipleSessions_IndependentCancellation)`
- Source: `tests/training/test_training_cancellation.cpp`:392
- Brief: n/a
- Parameters:
  - `<unnamed>` (TrainingCancellationTest): n/a
  - `<unnamed>` (MultipleSessions_IndependentCancellation): n/a

#### `TEST(TrainingCancellationTest, SessionLifecycle_CompleteCancellationFlow)`
- Source: `tests/training/test_training_cancellation.cpp`:456
- Brief: n/a
- Parameters:
  - `<unnamed>` (TrainingCancellationTest): n/a
  - `<unnamed>` (SessionLifecycle_CompleteCancellationFlow): n/a

#### `TEST(TrainingCancellationTest, StartAndCancel_Succeeds)`
- Source: `tests/training/test_training_cancellation.cpp`:172
- Brief: n/a
- Parameters:
  - `<unnamed>` (TrainingCancellationTest): n/a
  - `<unnamed>` (StartAndCancel_Succeeds): n/a

#### `TEST(TrainingCancellationTest, StateTransitions_Valid)`
- Source: `tests/training/test_training_cancellation.cpp`:342
- Brief: n/a
- Parameters:
  - `<unnamed>` (TrainingCancellationTest): n/a
  - `<unnamed>` (StateTransitions_Valid): n/a

### test_training_convergence.cpp

#### `TEST(ConfidenceCalibratorConvergence, ElapsedSecondsIsNonNegative)`
- Source: `tests/training/test_training_convergence.cpp`:132
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConfidenceCalibratorConvergence): n/a
  - `<unnamed>` (ElapsedSecondsIsNonNegative): n/a

#### `TEST(ConfidenceCalibratorConvergence, EmptySamplesReturnsSuccess)`
- Source: `tests/training/test_training_convergence.cpp`:35
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConfidenceCalibratorConvergence): n/a
  - `<unnamed>` (EmptySamplesReturnsSuccess): n/a

#### `TEST(ConfidenceCalibratorConvergence, F1ImprovementOverStaticBaseline)`
- Source: `tests/training/test_training_convergence.cpp`:72
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConfidenceCalibratorConvergence): n/a
  - `<unnamed>` (F1ImprovementOverStaticBaseline): n/a

#### `TEST(ConfidenceCalibratorConvergence, MultiCategoryProducesOneThresholdPerCategory)`
- Source: `tests/training/test_training_convergence.cpp`:90
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConfidenceCalibratorConvergence): n/a
  - `<unnamed>` (MultiCategoryProducesOneThresholdPerCategory): n/a

#### `TEST(ConfidenceCalibratorConvergence, PerfectlySeparableData_SelectsHighThreshold)`
- Source: `tests/training/test_training_convergence.cpp`:54
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConfidenceCalibratorConvergence): n/a
  - `<unnamed>` (PerfectlySeparableData_SelectsHighThreshold): n/a

#### `TEST(ConfidenceCalibratorConvergence, ResetClearsSamples)`
- Source: `tests/training/test_training_convergence.cpp`:113
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConfidenceCalibratorConvergence): n/a
  - `<unnamed>` (ResetClearsSamples): n/a

#### `TEST(ConfidenceCalibratorConvergence, SampleCountTracksAdditions)`
- Source: `tests/training/test_training_convergence.cpp`:124
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConfidenceCalibratorConvergence): n/a
  - `<unnamed>` (SampleCountTracksAdditions): n/a

#### `TEST(ConfidenceCalibratorConvergence, SingleSampleReturnsThreshold)`
- Source: `tests/training/test_training_convergence.cpp`:42
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConfidenceCalibratorConvergence): n/a
  - `<unnamed>` (SingleSampleReturnsThreshold): n/a

#### `TEST(ModalityDetectorBatch, BatchAggregatesAllSamples)`
- Source: `tests/training/test_training_convergence.cpp`:460
- Brief: n/a
- Parameters:
  - `<unnamed>` (ModalityDetectorBatch): n/a
  - `<unnamed>` (BatchAggregatesAllSamples): n/a

#### `TEST(ModalityDetectorBatch, EmptyBatch_ReturnsZeroStats)`
- Source: `tests/training/test_training_convergence.cpp`:479
- Brief: n/a
- Parameters:
  - `<unnamed>` (ModalityDetectorBatch): n/a
  - `<unnamed>` (EmptyBatch_ReturnsZeroStats): n/a

#### `TEST(OCRExtractorTest, DisabledByDefault_NotAvailable)`
- Source: `tests/training/test_training_convergence.cpp`:493
- Brief: n/a
- Parameters:
  - `<unnamed>` (OCRExtractorTest): n/a
  - `<unnamed>` (DisabledByDefault_NotAvailable): n/a

#### `TEST(OCRExtractorTest, DisabledExtract_ReturnsEmpty)`
- Source: `tests/training/test_training_convergence.cpp`:500
- Brief: n/a
- Parameters:
  - `<unnamed>` (OCRExtractorTest): n/a
  - `<unnamed>` (DisabledExtract_ReturnsEmpty): n/a

#### `TEST(TrainingPipelineCallbackSanitizer, BlocksInjectionPatternFailClosed)`
- Source: `tests/training/test_training_convergence.cpp`:144
- Brief: n/a
- Parameters:
  - `<unnamed>` (TrainingPipelineCallbackSanitizer): n/a
  - `<unnamed>` (BlocksInjectionPatternFailClosed): n/a

#### `TEST(TrainingPipelineCallbackSanitizer, RedactsControlTokensButAllowsMessage)`
- Source: `tests/training/test_training_convergence.cpp`:151
- Brief: n/a
- Parameters:
  - `<unnamed>` (TrainingPipelineCallbackSanitizer): n/a
  - `<unnamed>` (RedactsControlTokensButAllowsMessage): n/a

#### `TEST_F(CitationExtractorTest, ConfidenceInRange)`
- Source: `tests/training/test_training_convergence.cpp`:373
- Brief: n/a
- Parameters:
  - `<unnamed>` (CitationExtractorTest): n/a
  - `<unnamed>` (ConfidenceInRange): n/a

#### `TEST_F(CitationExtractorTest, CourtDecision_Detected)`
- Source: `tests/training/test_training_convergence.cpp`:355
- Brief: n/a
- Parameters:
  - `<unnamed>` (CitationExtractorTest): n/a
  - `<unnamed>` (CourtDecision_Detected): n/a

#### `TEST_F(CitationExtractorTest, EmptyText_ReturnsEmpty)`
- Source: `tests/training/test_training_convergence.cpp`:368
- Brief: n/a
- Parameters:
  - `<unnamed>` (CitationExtractorTest): n/a
  - `<unnamed>` (EmptyText_ReturnsEmpty): n/a

#### `TEST_F(CitationExtractorTest, GermanStatutory_Detected)`
- Source: `tests/training/test_training_convergence.cpp`:338
- Brief: n/a
- Parameters:
  - `<unnamed>` (CitationExtractorTest): n/a
  - `<unnamed>` (GermanStatutory_Detected): n/a

#### `TEST_F(ModalityDetectorParseTest, EmptyDocument_SucceedsWithNoSamples)`
- Source: `tests/training/test_training_convergence.cpp`:434
- Brief: n/a
- Parameters:
  - `<unnamed>` (ModalityDetectorParseTest): n/a
  - `<unnamed>` (EmptyDocument_SucceedsWithNoSamples): n/a

#### `TEST_F(ModalityDetectorParseTest, MixedDocument_ExtractsAllModalities)`
- Source: `tests/training/test_training_convergence.cpp`:396
- Brief: n/a
- Parameters:
  - `<unnamed>` (ModalityDetectorParseTest): n/a
  - `<unnamed>` (MixedDocument_ExtractsAllModalities): n/a

#### `TEST_F(ModalityDetectorParseTest, StatsAreConsistentWithSamples)`
- Source: `tests/training/test_training_convergence.cpp`:441
- Brief: n/a
- Parameters:
  - `<unnamed>` (ModalityDetectorParseTest): n/a
  - `<unnamed>` (StatsAreConsistentWithSamples): n/a

#### `TEST_F(ModalityDetectorTest, EmptyContent_ReturnsUnknown)`
- Source: `tests/training/test_training_convergence.cpp`:202
- Brief: n/a
- Parameters:
  - `<unnamed>` (ModalityDetectorTest): n/a
  - `<unnamed>` (EmptyContent_ReturnsUnknown): n/a

#### `TEST_F(ModalityDetectorTest, ImageMimeHint_ReturnsOCRImage)`
- Source: `tests/training/test_training_convergence.cpp`:174
- Brief: n/a
- Parameters:
  - `<unnamed>` (ModalityDetectorTest): n/a
  - `<unnamed>` (ImageMimeHint_ReturnsOCRImage): n/a

#### `TEST_F(ModalityDetectorTest, PipeTable_ReturnsTable)`
- Source: `tests/training/test_training_convergence.cpp`:180
- Brief: n/a
- Parameters:
  - `<unnamed>` (ModalityDetectorTest): n/a
  - `<unnamed>` (PipeTable_ReturnsTable): n/a

#### `TEST_F(ModalityDetectorTest, PlainText_ReturnsTextClause)`
- Source: `tests/training/test_training_convergence.cpp`:192
- Brief: n/a
- Parameters:
  - `<unnamed>` (ModalityDetectorTest): n/a
  - `<unnamed>` (PlainText_ReturnsTextClause): n/a

#### `TEST_F(TableExtractorTest, EmptyText_ReturnsEmpty)`
- Source: `tests/training/test_training_convergence.cpp`:312
- Brief: n/a
- Parameters:
  - `<unnamed>` (TableExtractorTest): n/a
  - `<unnamed>` (EmptyText_ReturnsEmpty): n/a

#### `TEST_F(TableExtractorTest, PipeTable_Detected)`
- Source: `tests/training/test_training_convergence.cpp`:296
- Brief: n/a
- Parameters:
  - `<unnamed>` (TableExtractorTest): n/a
  - `<unnamed>` (PipeTable_Detected): n/a

#### `TEST_F(TableExtractorTest, SingleLineTable_NotExtracted)`
- Source: `tests/training/test_training_convergence.cpp`:317
- Brief: n/a
- Parameters:
  - `<unnamed>` (TableExtractorTest): n/a
  - `<unnamed>` (SingleLineTable_NotExtracted): n/a

#### `TEST_F(TextClauseExtractorTest, ControlTokensAreRedactedAndSampleRemains)`
- Source: `tests/training/test_training_convergence.cpp`:249
- Brief: n/a
- Parameters:
  - `<unnamed>` (TextClauseExtractorTest): n/a
  - `<unnamed>` (ControlTokensAreRedactedAndSampleRemains): n/a

#### `TEST_F(TextClauseExtractorTest, EmptyText_ReturnsEmpty)`
- Source: `tests/training/test_training_convergence.cpp`:259
- Brief: n/a
- Parameters:
  - `<unnamed>` (TextClauseExtractorTest): n/a
  - `<unnamed>` (EmptyText_ReturnsEmpty): n/a

#### `TEST_F(TextClauseExtractorTest, ExtractsClausesFromLegalText)`
- Source: `tests/training/test_training_convergence.cpp`:221
- Brief: n/a
- Parameters:
  - `<unnamed>` (TextClauseExtractorTest): n/a
  - `<unnamed>` (ExtractsClausesFromLegalText): n/a

#### `TEST_F(TextClauseExtractorTest, PromptInjectionLikeClauseIsRejected)`
- Source: `tests/training/test_training_convergence.cpp`:238
- Brief: n/a
- Parameters:
  - `<unnamed>` (TextClauseExtractorTest): n/a
  - `<unnamed>` (PromptInjectionLikeClauseIsRejected): n/a

#### `TEST_F(TextClauseExtractorTest, ShortClauses_AreFiltered)`
- Source: `tests/training/test_training_convergence.cpp`:265
- Brief: n/a
- Parameters:
  - `<unnamed>` (TextClauseExtractorTest): n/a
  - `<unnamed>` (ShortClauses_AreFiltered): n/a

#### `TEST_F(TextClauseExtractorTest, SourceIdPropagated)`
- Source: `tests/training/test_training_convergence.cpp`:272
- Brief: n/a
- Parameters:
  - `<unnamed>` (TextClauseExtractorTest): n/a
  - `<unnamed>` (SourceIdPropagated): n/a

### test_training_database_optimizer.cpp

#### `TEST(DatabaseOptimizerLabeler, DBO01_BaoDecisionLabelIsDatabaseOptimizer)`
- Source: `tests/training/test_training_database_optimizer.cpp`:58
- Brief: n/a
- Parameters:
  - `<unnamed>` (DatabaseOptimizerLabeler): n/a
  - `<unnamed>` (DBO01_BaoDecisionLabelIsDatabaseOptimizer): n/a

#### `TEST(DatabaseOptimizerLabeler, DBO02_ConfidenceAtZeroDeltaIsHalf)`
- Source: `tests/training/test_training_database_optimizer.cpp`:75
- Brief: n/a
- Parameters:
  - `<unnamed>` (DatabaseOptimizerLabeler): n/a
  - `<unnamed>` (DBO02_ConfidenceAtZeroDeltaIsHalf): n/a

#### `TEST(DatabaseOptimizerLabeler, DBO03_ConfidenceAtFiftyMsIsAboveThreshold)`
- Source: `tests/training/test_training_database_optimizer.cpp`:85
- Brief: n/a
- Parameters:
  - `<unnamed>` (DatabaseOptimizerLabeler): n/a
  - `<unnamed>` (DBO03_ConfidenceAtFiftyMsIsAboveThreshold): n/a

#### `TEST(DatabaseOptimizerLabeler, DBO04_DomainKeywordsPreservedInPlanJson)`
- Source: `tests/training/test_training_database_optimizer.cpp`:95
- Brief: n/a
- Parameters:
  - `<unnamed>` (DatabaseOptimizerLabeler): n/a
  - `<unnamed>` (DBO04_DomainKeywordsPreservedInPlanJson): n/a

#### `TEST(DatabaseOptimizerLabeler, DBO05_ExportToJsonlProducesValidLines)`
- Source: `tests/training/test_training_database_optimizer.cpp`:115
- Brief: n/a
- Parameters:
  - `<unnamed>` (DatabaseOptimizerLabeler): n/a
  - `<unnamed>` (DBO05_ExportToJsonlProducesValidLines): n/a

#### `TEST(DatabaseOptimizerLabeler, DBO05b_ExportEmptySamplesReturnsEmptyString)`
- Source: `tests/training/test_training_database_optimizer.cpp`:146
- Brief: n/a
- Parameters:
  - `<unnamed>` (DatabaseOptimizerLabeler): n/a
  - `<unnamed>` (DBO05b_ExportEmptySamplesReturnsEmptyString): n/a

#### `TEST(DatabaseOptimizerLabeler, DBO06_DuplicateQueryFilteredByPipeline)`
- Source: `tests/training/test_training_database_optimizer.cpp`:155
- Brief: n/a
- Parameters:
  - `<unnamed>` (DatabaseOptimizerLabeler): n/a
  - `<unnamed>` (DBO06_DuplicateQueryFilteredByPipeline): n/a

#### `TEST(DatabaseOptimizerLabeler, DBO07_LegalDomainDistinctFromDatabaseOptimizer)`
- Source: `tests/training/test_training_database_optimizer.cpp`:188
- Brief: n/a
- Parameters:
  - `<unnamed>` (DatabaseOptimizerLabeler): n/a
  - `<unnamed>` (DBO07_LegalDomainDistinctFromDatabaseOptimizer): n/a

#### `TEST(DatabaseOptimizerLabeler, DBO08_OneThousandSampleGoldenDatasetHighConfidence)`
- Source: `tests/training/test_training_database_optimizer.cpp`:207
- Brief: n/a
- Parameters:
  - `<unnamed>` (DatabaseOptimizerLabeler): n/a
  - `<unnamed>` (DBO08_OneThousandSampleGoldenDatasetHighConfidence): n/a

### test_training_diagnostics_consistency.cpp

#### `TEST(TrainingDiagnosticsConsistency, TDC_01_labeling_stats_default_zero)`
- Source: `tests/training/test_training_diagnostics_consistency.cpp`:36
- Brief: n/a
- Parameters:
  - `<unnamed>` (TrainingDiagnosticsConsistency): n/a
  - `<unnamed>` (TDC_01_labeling_stats_default_zero): n/a

#### `TEST(TrainingDiagnosticsConsistency, TDC_02_deploy_fail_has_error_and_false_success)`
- Source: `tests/training/test_training_diagnostics_consistency.cpp`:48
- Brief: n/a
- Parameters:
  - `<unnamed>` (TrainingDiagnosticsConsistency): n/a
  - `<unnamed>` (TDC_02_deploy_fail_has_error_and_false_success): n/a

#### `TEST(TrainingDiagnosticsConsistency, TDC_03_deploy_ok_has_no_error)`
- Source: `tests/training/test_training_diagnostics_consistency.cpp`:58
- Brief: n/a
- Parameters:
  - `<unnamed>` (TrainingDiagnosticsConsistency): n/a
  - `<unnamed>` (TDC_03_deploy_ok_has_no_error): n/a

#### `TEST(TrainingDiagnosticsConsistency, TDC_04_deploy_fail_codes_are_distinct)`
- Source: `tests/training/test_training_diagnostics_consistency.cpp`:69
- Brief: n/a
- Parameters:
  - `<unnamed>` (TrainingDiagnosticsConsistency): n/a
  - `<unnamed>` (TDC_04_deploy_fail_codes_are_distinct): n/a

#### `TEST(TrainingDiagnosticsConsistency, TDC_05_provenance_stats_default_zero)`
- Source: `tests/training/test_training_diagnostics_consistency.cpp`:92
- Brief: n/a
- Parameters:
  - `<unnamed>` (TrainingDiagnosticsConsistency): n/a
  - `<unnamed>` (TDC_05_provenance_stats_default_zero): n/a

#### `TEST(TrainingDiagnosticsConsistency, TDC_06_provenance_write_empty_input_zero_stats)`
- Source: `tests/training/test_training_diagnostics_consistency.cpp`:102
- Brief: n/a
- Parameters:
  - `<unnamed>` (TrainingDiagnosticsConsistency): n/a
  - `<unnamed>` (TDC_06_provenance_write_empty_input_zero_stats): n/a

#### `TEST(TrainingDiagnosticsConsistency, TDC_07_checkpoint_entry_default_zero)`
- Source: `tests/training/test_training_diagnostics_consistency.cpp`:117
- Brief: n/a
- Parameters:
  - `<unnamed>` (TrainingDiagnosticsConsistency): n/a
  - `<unnamed>` (TDC_07_checkpoint_entry_default_zero): n/a

#### `TEST(TrainingDiagnosticsConsistency, TDC_08_deploy_fail_codes_non_empty)`
- Source: `tests/training/test_training_diagnostics_consistency.cpp`:133
- Brief: n/a
- Parameters:
  - `<unnamed>` (TrainingDiagnosticsConsistency): n/a
  - `<unnamed>` (TDC_08_deploy_fail_codes_non_empty): n/a

#### `TEST(TrainingDiagnosticsConsistency, TDC_09_deploy_ok_split_roundtrip)`
- Source: `tests/training/test_training_diagnostics_consistency.cpp`:146
- Brief: n/a
- Parameters:
  - `<unnamed>` (TrainingDiagnosticsConsistency): n/a
  - `<unnamed>` (TDC_09_deploy_ok_split_roundtrip): n/a

#### `TEST(TrainingDiagnosticsConsistency, TDC_10_labeling_stats_elapsed_non_negative)`
- Source: `tests/training/test_training_diagnostics_consistency.cpp`:156
- Brief: n/a
- Parameters:
  - `<unnamed>` (TrainingDiagnosticsConsistency): n/a
  - `<unnamed>` (TDC_10_labeling_stats_elapsed_non_negative): n/a

### test_training_highcardinality_stress.cpp

#### `TEST(WaveD_TrainingStress, ConcurrentCheckpointStress)`
- Source: `tests/training/test_training_highcardinality_stress.cpp`:160
- Brief: n/a
- Parameters:
  - `<unnamed>` (WaveD_TrainingStress): n/a
  - `<unnamed>` (ConcurrentCheckpointStress): n/a

#### `TEST(WaveD_TrainingStress, GradientAccumulationStress)`
- Source: `tests/training/test_training_highcardinality_stress.cpp`:187
- Brief: n/a
- Parameters:
  - `<unnamed>` (WaveD_TrainingStress): n/a
  - `<unnamed>` (GradientAccumulationStress): n/a

#### `TEST(WaveD_TrainingStress, HighCardinalityBatchDispatch)`
- Source: `tests/training/test_training_highcardinality_stress.cpp`:130
- Brief: n/a
- Parameters:
  - `<unnamed>` (WaveD_TrainingStress): n/a
  - `<unnamed>` (HighCardinalityBatchDispatch): n/a

### test_training_lora_adapter.cpp

#### `TEST(TrainingLoRAAdapterTest, AMatrix_InitialisedAllZero)`
- Source: `tests/training/test_training_lora_adapter.cpp`:142
- Brief: n/a
- Parameters:
  - `<unnamed>` (TrainingLoRAAdapterTest): n/a
  - `<unnamed>` (AMatrix_InitialisedAllZero): n/a

#### `TEST(TrainingLoRAAdapterTest, AddLayer_DuplicateNameThrows)`
- Source: `tests/training/test_training_lora_adapter.cpp`:83
- Brief: n/a
- Parameters:
  - `<unnamed>` (TrainingLoRAAdapterTest): n/a
  - `<unnamed>` (AddLayer_DuplicateNameThrows): n/a

#### `TEST(TrainingLoRAAdapterTest, AddLayer_MultipleDistinctNames)`
- Source: `tests/training/test_training_lora_adapter.cpp`:68
- Brief: n/a
- Parameters:
  - `<unnamed>` (TrainingLoRAAdapterTest): n/a
  - `<unnamed>` (AddLayer_MultipleDistinctNames): n/a

#### `TEST(TrainingLoRAAdapterTest, AddLayer_OverrideRankAndAlpha)`
- Source: `tests/training/test_training_lora_adapter.cpp`:101
- Brief: n/a
- Parameters:
  - `<unnamed>` (TrainingLoRAAdapterTest): n/a
  - `<unnamed>` (AddLayer_OverrideRankAndAlpha): n/a

#### `TEST(TrainingLoRAAdapterTest, AddLayer_RankExceedsDimThrows)`
- Source: `tests/training/test_training_lora_adapter.cpp`:95
- Brief: n/a
- Parameters:
  - `<unnamed>` (TrainingLoRAAdapterTest): n/a
  - `<unnamed>` (AddLayer_RankExceedsDimThrows): n/a

#### `TEST(TrainingLoRAAdapterTest, AddLayer_Success)`
- Source: `tests/training/test_training_lora_adapter.cpp`:61
- Brief: n/a
- Parameters:
  - `<unnamed>` (TrainingLoRAAdapterTest): n/a
  - `<unnamed>` (AddLayer_Success): n/a

#### `TEST(TrainingLoRAAdapterTest, AddLayer_ZeroDimThrows)`
- Source: `tests/training/test_training_lora_adapter.cpp`:89
- Brief: n/a
- Parameters:
  - `<unnamed>` (TrainingLoRAAdapterTest): n/a
  - `<unnamed>` (AddLayer_ZeroDimThrows): n/a

#### `TEST(TrainingLoRAAdapterTest, ApplyBatchUpdate_AllLayersUpdated)`
- Source: `tests/training/test_training_lora_adapter.cpp`:278
- Brief: n/a
- Parameters:
  - `<unnamed>` (TrainingLoRAAdapterTest): n/a
  - `<unnamed>` (ApplyBatchUpdate_AllLayersUpdated): n/a

#### `TEST(TrainingLoRAAdapterTest, ApplyBatchUpdate_SizeMismatchVectorsThrows)`
- Source: `tests/training/test_training_lora_adapter.cpp`:327
- Brief: n/a
- Parameters:
  - `<unnamed>` (TrainingLoRAAdapterTest): n/a
  - `<unnamed>` (ApplyBatchUpdate_SizeMismatchVectorsThrows): n/a

#### `TEST(TrainingLoRAAdapterTest, ApplyBatchUpdate_SkipsUnknownLayers)`
- Source: `tests/training/test_training_lora_adapter.cpp`:310
- Brief: n/a
- Parameters:
  - `<unnamed>` (TrainingLoRAAdapterTest): n/a
  - `<unnamed>` (ApplyBatchUpdate_SkipsUnknownLayers): n/a

#### `TEST(TrainingLoRAAdapterTest, ApplyUpdate_DeltaAsizeMismatchThrows)`
- Source: `tests/training/test_training_lora_adapter.cpp`:265
- Brief: n/a
- Parameters:
  - `<unnamed>` (TrainingLoRAAdapterTest): n/a
  - `<unnamed>` (ApplyUpdate_DeltaAsizeMismatchThrows): n/a

#### `TEST(TrainingLoRAAdapterTest, ApplyUpdate_DeltaBsizeMismatchThrows)`
- Source: `tests/training/test_training_lora_adapter.cpp`:256
- Brief: n/a
- Parameters:
  - `<unnamed>` (TrainingLoRAAdapterTest): n/a
  - `<unnamed>` (ApplyUpdate_DeltaBsizeMismatchThrows): n/a

#### `TEST(TrainingLoRAAdapterTest, ApplyUpdate_MutatesWeightsAdditively)`
- Source: `tests/training/test_training_lora_adapter.cpp`:228
- Brief: n/a
- Parameters:
  - `<unnamed>` (TrainingLoRAAdapterTest): n/a
  - `<unnamed>` (ApplyUpdate_MutatesWeightsAdditively): n/a

#### `TEST(TrainingLoRAAdapterTest, ApplyUpdate_UnknownLayerThrows)`
- Source: `tests/training/test_training_lora_adapter.cpp`:251
- Brief: n/a
- Parameters:
  - `<unnamed>` (TrainingLoRAAdapterTest): n/a
  - `<unnamed>` (ApplyUpdate_UnknownLayerThrows): n/a

#### `TEST(TrainingLoRAAdapterTest, BMatrix_InitialisedWithNonZeroKaiming)`
- Source: `tests/training/test_training_lora_adapter.cpp`:131
- Brief: n/a
- Parameters:
  - `<unnamed>` (TrainingLoRAAdapterTest): n/a
  - `<unnamed>` (BMatrix_InitialisedWithNonZeroKaiming): n/a

#### `TEST(TrainingLoRAAdapterTest, ConstructsWithCustomRankAlpha)`
- Source: `tests/training/test_training_lora_adapter.cpp`:42
- Brief: n/a
- Parameters:
  - `<unnamed>` (TrainingLoRAAdapterTest): n/a
  - `<unnamed>` (ConstructsWithCustomRankAlpha): n/a

#### `TEST(TrainingLoRAAdapterTest, ConstructsWithDefaults)`
- Source: `tests/training/test_training_lora_adapter.cpp`:35
- Brief: n/a
- Parameters:
  - `<unnamed>` (TrainingLoRAAdapterTest): n/a
  - `<unnamed>` (ConstructsWithDefaults): n/a

#### `TEST(TrainingLoRAAdapterTest, DifferentLayers_DifferentBMatrices)`
- Source: `tests/training/test_training_lora_adapter.cpp`:151
- Brief: n/a
- Parameters:
  - `<unnamed>` (TrainingLoRAAdapterTest): n/a
  - `<unnamed>` (DifferentLayers_DifferentBMatrices): n/a

#### `TEST(TrainingLoRAAdapterTest, ExportImport_RoundTrip)`
- Source: `tests/training/test_training_lora_adapter.cpp`:424
- Brief: n/a
- Parameters:
  - `<unnamed>` (TrainingLoRAAdapterTest): n/a
  - `<unnamed>` (ExportImport_RoundTrip): n/a

#### `TEST(TrainingLoRAAdapterTest, Forward_InputSizeMismatchThrows)`
- Source: `tests/training/test_training_lora_adapter.cpp`:412
- Brief: n/a
- Parameters:
  - `<unnamed>` (TrainingLoRAAdapterTest): n/a
  - `<unnamed>` (Forward_InputSizeMismatchThrows): n/a

#### `TEST(TrainingLoRAAdapterTest, Forward_NonZeroAfterAUpdate)`
- Source: `tests/training/test_training_lora_adapter.cpp`:364
- Brief: n/a
- Parameters:
  - `<unnamed>` (TrainingLoRAAdapterTest): n/a
  - `<unnamed>` (Forward_NonZeroAfterAUpdate): n/a

#### `TEST(TrainingLoRAAdapterTest, Forward_OutputShape)`
- Source: `tests/training/test_training_lora_adapter.cpp`:354
- Brief: n/a
- Parameters:
  - `<unnamed>` (TrainingLoRAAdapterTest): n/a
  - `<unnamed>` (Forward_OutputShape): n/a

#### `TEST(TrainingLoRAAdapterTest, Forward_ScalingApplied)`
- Source: `tests/training/test_training_lora_adapter.cpp`:382
- Brief: n/a
- Parameters:
  - `<unnamed>` (TrainingLoRAAdapterTest): n/a
  - `<unnamed>` (Forward_ScalingApplied): n/a

#### `TEST(TrainingLoRAAdapterTest, Forward_UnknownLayerThrows)`
- Source: `tests/training/test_training_lora_adapter.cpp`:407
- Brief: n/a
- Parameters:
  - `<unnamed>` (TrainingLoRAAdapterTest): n/a
  - `<unnamed>` (Forward_UnknownLayerThrows): n/a

#### `TEST(TrainingLoRAAdapterTest, Forward_ZeroAtInit)`
- Source: `tests/training/test_training_lora_adapter.cpp`:341
- Brief: n/a
- Parameters:
  - `<unnamed>` (TrainingLoRAAdapterTest): n/a
  - `<unnamed>` (Forward_ZeroAtInit): n/a

#### `TEST(TrainingLoRAAdapterTest, HasLayer_FalseForUnknown)`
- Source: `tests/training/test_training_lora_adapter.cpp`:122
- Brief: n/a
- Parameters:
  - `<unnamed>` (TrainingLoRAAdapterTest): n/a
  - `<unnamed>` (HasLayer_FalseForUnknown): n/a

#### `TEST(TrainingLoRAAdapterTest, ImportWeights_AddsNewLayer)`
- Source: `tests/training/test_training_lora_adapter.cpp`:473
- Brief: n/a
- Parameters:
  - `<unnamed>` (TrainingLoRAAdapterTest): n/a
  - `<unnamed>` (ImportWeights_AddsNewLayer): n/a

#### `TEST(TrainingLoRAAdapterTest, ImportWeights_OverwritesExistingLayer)`
- Source: `tests/training/test_training_lora_adapter.cpp`:453
- Brief: n/a
- Parameters:
  - `<unnamed>` (TrainingLoRAAdapterTest): n/a
  - `<unnamed>` (ImportWeights_OverwritesExistingLayer): n/a

#### `TEST(TrainingLoRAAdapterTest, ImportWeights_SizeMismatchThrows)`
- Source: `tests/training/test_training_lora_adapter.cpp`:490
- Brief: n/a
- Parameters:
  - `<unnamed>` (TrainingLoRAAdapterTest): n/a
  - `<unnamed>` (ImportWeights_SizeMismatchThrows): n/a

#### `TEST(TrainingLoRAAdapterTest, InvalidAlphaThrows)`
- Source: `tests/training/test_training_lora_adapter.cpp`:52
- Brief: n/a
- Parameters:
  - `<unnamed>` (TrainingLoRAAdapterTest): n/a
  - `<unnamed>` (InvalidAlphaThrows): n/a

#### `TEST(TrainingLoRAAdapterTest, InvalidRankThrows)`
- Source: `tests/training/test_training_lora_adapter.cpp`:48
- Brief: n/a
- Parameters:
  - `<unnamed>` (TrainingLoRAAdapterTest): n/a
  - `<unnamed>` (InvalidRankThrows): n/a

#### `TEST(TrainingLoRAAdapterTest, RemoveLayer_ExistingSucceeds)`
- Source: `tests/training/test_training_lora_adapter.cpp`:109
- Brief: n/a
- Parameters:
  - `<unnamed>` (TrainingLoRAAdapterTest): n/a
  - `<unnamed>` (RemoveLayer_ExistingSucceeds): n/a

#### `TEST(TrainingLoRAAdapterTest, RemoveLayer_NonExistentReturnsFalse)`
- Source: `tests/training/test_training_lora_adapter.cpp`:117
- Brief: n/a
- Parameters:
  - `<unnamed>` (TrainingLoRAAdapterTest): n/a
  - `<unnamed>` (RemoveLayer_NonExistentReturnsFalse): n/a

#### `TEST(TrainingLoRAAdapterTest, RepeatedBatchUpdates_WeightsAccumulate)`
- Source: `tests/training/test_training_lora_adapter.cpp`:509
- Brief: n/a
- Parameters:
  - `<unnamed>` (TrainingLoRAAdapterTest): n/a
  - `<unnamed>` (RepeatedBatchUpdates_WeightsAccumulate): n/a

#### `TEST(TrainingLoRAAdapterTest, SetWeights_AsizeMismatchThrows)`
- Source: `tests/training/test_training_lora_adapter.cpp`:215
- Brief: n/a
- Parameters:
  - `<unnamed>` (TrainingLoRAAdapterTest): n/a
  - `<unnamed>` (SetWeights_AsizeMismatchThrows): n/a

#### `TEST(TrainingLoRAAdapterTest, SetWeights_BsizeMismatchThrows)`
- Source: `tests/training/test_training_lora_adapter.cpp`:206
- Brief: n/a
- Parameters:
  - `<unnamed>` (TrainingLoRAAdapterTest): n/a
  - `<unnamed>` (SetWeights_BsizeMismatchThrows): n/a

#### `TEST(TrainingLoRAAdapterTest, SetWeights_RoundTrip)`
- Source: `tests/training/test_training_lora_adapter.cpp`:184
- Brief: n/a
- Parameters:
  - `<unnamed>` (TrainingLoRAAdapterTest): n/a
  - `<unnamed>` (SetWeights_RoundTrip): n/a

#### `TEST(TrainingLoRAAdapterTest, SetWeights_UnknownLayerThrows)`
- Source: `tests/training/test_training_lora_adapter.cpp`:201
- Brief: n/a
- Parameters:
  - `<unnamed>` (TrainingLoRAAdapterTest): n/a
  - `<unnamed>` (SetWeights_UnknownLayerThrows): n/a

#### `TEST(TrainingLoRAAdapterTest, TotalParameterCount_MatchesExpected)`
- Source: `tests/training/test_training_lora_adapter.cpp`:172
- Brief: n/a
- Parameters:
  - `<unnamed>` (TrainingLoRAAdapterTest): n/a
  - `<unnamed>` (TotalParameterCount_MatchesExpected): n/a

### test_training_phase2.cpp

#### `TEST(DeployResult, DefaultIsNotSuccess)`
- Source: `tests/training/test_training_phase2.cpp`:75
- Brief: n/a
- Parameters:
  - `<unnamed>` (DeployResult): n/a
  - `<unnamed>` (DefaultIsNotSuccess): n/a

#### `TEST(DeployResult, FailFactory)`
- Source: `tests/training/test_training_phase2.cpp`:90
- Brief: n/a
- Parameters:
  - `<unnamed>` (DeployResult): n/a
  - `<unnamed>` (FailFactory): n/a

#### `TEST(DeployResult, OkFactory)`
- Source: `tests/training/test_training_phase2.cpp`:82
- Brief: n/a
- Parameters:
  - `<unnamed>` (DeployResult): n/a
  - `<unnamed>` (OkFactory): n/a

#### `TEST(DomainType, CanSetFinancial)`
- Source: `tests/training/test_training_phase2.cpp`:277
- Brief: n/a
- Parameters:
  - `<unnamed>` (DomainType): n/a
  - `<unnamed>` (CanSetFinancial): n/a

#### `TEST(DomainType, CanSetMedical)`
- Source: `tests/training/test_training_phase2.cpp`:271
- Brief: n/a
- Parameters:
  - `<unnamed>` (DomainType): n/a
  - `<unnamed>` (CanSetMedical): n/a

#### `TEST(DomainType, DefaultIsLegal)`
- Source: `tests/training/test_training_phase2.cpp`:266
- Brief: n/a
- Parameters:
  - `<unnamed>` (DomainType): n/a
  - `<unnamed>` (DefaultIsLegal): n/a

#### `TEST(IntegrityVerification, NoCheckpointDirBypassesCheck)`
- Source: `tests/training/test_training_phase2.cpp`:426
- Brief: n/a
- Parameters:
  - `<unnamed>` (IntegrityVerification): n/a
  - `<unnamed>` (NoCheckpointDirBypassesCheck): n/a

#### `TEST(LegacyDeployApi, DeployVersionReturnsBool)`
- Source: `tests/training/test_training_phase2.cpp`:235
- Brief: n/a
- Parameters:
  - `<unnamed>` (LegacyDeployApi): n/a
  - `<unnamed>` (DeployVersionReturnsBool): n/a

#### `TEST(LegacyDeployApi, EmptyVersionReturnsFalse)`
- Source: `tests/training/test_training_phase2.cpp`:241
- Brief: n/a
- Parameters:
  - `<unnamed>` (LegacyDeployApi): n/a
  - `<unnamed>` (EmptyVersionReturnsFalse): n/a

#### `TEST(MultiDomainLabeler, EmptyTextProducesNoSamples)`
- Source: `tests/training/test_training_phase2.cpp`:377
- Brief: n/a
- Parameters:
  - `<unnamed>` (MultiDomainLabeler): n/a
  - `<unnamed>` (EmptyTextProducesNoSamples): n/a

#### `TEST(MultiDomainLabeler, FinancialDomainExtractsGermanMuss)`
- Source: `tests/training/test_training_phase2.cpp`:360
- Brief: n/a
- Parameters:
  - `<unnamed>` (MultiDomainLabeler): n/a
  - `<unnamed>` (FinancialDomainExtractsGermanMuss): n/a

#### `TEST(MultiDomainLabeler, FinancialDomainExtractsMust)`
- Source: `tests/training/test_training_phase2.cpp`:340
- Brief: n/a
- Parameters:
  - `<unnamed>` (MultiDomainLabeler): n/a
  - `<unnamed>` (FinancialDomainExtractsMust): n/a

#### `TEST(MultiDomainLabeler, FinancialDomainExtractsProhibited)`
- Source: `tests/training/test_training_phase2.cpp`:350
- Brief: n/a
- Parameters:
  - `<unnamed>` (MultiDomainLabeler): n/a
  - `<unnamed>` (FinancialDomainExtractsProhibited): n/a

#### `TEST(MultiDomainLabeler, FinancialProhibitionHighConfidence)`
- Source: `tests/training/test_training_phase2.cpp`:410
- Brief: n/a
- Parameters:
  - `<unnamed>` (MultiDomainLabeler): n/a
  - `<unnamed>` (FinancialProhibitionHighConfidence): n/a

#### `TEST(MultiDomainLabeler, LegalDomainDoesNotProduceContraindicated)`
- Source: `tests/training/test_training_phase2.cpp`:370
- Brief: n/a
- Parameters:
  - `<unnamed>` (MultiDomainLabeler): n/a
  - `<unnamed>` (LegalDomainDoesNotProduceContraindicated): n/a

#### `TEST(MultiDomainLabeler, LegalDomainExtractsMuss)`
- Source: `tests/training/test_training_phase2.cpp`:300
- Brief: n/a
- Parameters:
  - `<unnamed>` (MultiDomainLabeler): n/a
  - `<unnamed>` (LegalDomainExtractsMuss): n/a

#### `TEST(MultiDomainLabeler, MedicalDomainExtractsContraindicated)`
- Source: `tests/training/test_training_phase2.cpp`:320
- Brief: n/a
- Parameters:
  - `<unnamed>` (MultiDomainLabeler): n/a
  - `<unnamed>` (MedicalDomainExtractsContraindicated): n/a

#### `TEST(MultiDomainLabeler, MedicalDomainExtractsMust)`
- Source: `tests/training/test_training_phase2.cpp`:310
- Brief: n/a
- Parameters:
  - `<unnamed>` (MultiDomainLabeler): n/a
  - `<unnamed>` (MedicalDomainExtractsMust): n/a

#### `TEST(MultiDomainLabeler, MedicalDomainExtractsRecommendation)`
- Source: `tests/training/test_training_phase2.cpp`:330
- Brief: n/a
- Parameters:
  - `<unnamed>` (MultiDomainLabeler): n/a
  - `<unnamed>` (MedicalDomainExtractsRecommendation): n/a

#### `TEST(MultiDomainLabeler, MedicalGermanKann)`
- Source: `tests/training/test_training_phase2.cpp`:385
- Brief: n/a
- Parameters:
  - `<unnamed>` (MultiDomainLabeler): n/a
  - `<unnamed>` (MedicalGermanKann): n/a

#### `TEST(MultiDomainLabeler, MedicalObligationHighConfidence)`
- Source: `tests/training/test_training_phase2.cpp`:399
- Brief: n/a
- Parameters:
  - `<unnamed>` (MultiDomainLabeler): n/a
  - `<unnamed>` (MedicalObligationHighConfidence): n/a

#### `TEST(TrainingSafety, TrainRejectsPromptInjectionLikeCollectionName)`
- Source: `tests/training/test_training_phase2.cpp`:247
- Brief: n/a
- Parameters:
  - `<unnamed>` (TrainingSafety): n/a
  - `<unnamed>` (TrainRejectsPromptInjectionLikeCollectionName): n/a

#### `TEST_F(DeployVersionExTest, EmptyVersionFails)`
- Source: `tests/training/test_training_phase2.cpp`:106
- Brief: n/a
- Parameters:
  - `<unnamed>` (DeployVersionExTest): n/a
  - `<unnamed>` (EmptyVersionFails): n/a

#### `TEST_F(DeployVersionExTest, InvalidSplitAboveOneFails)`
- Source: `tests/training/test_training_phase2.cpp`:112
- Brief: n/a
- Parameters:
  - `<unnamed>` (DeployVersionExTest): n/a
  - `<unnamed>` (InvalidSplitAboveOneFails): n/a

#### `TEST_F(DeployVersionExTest, InvalidSplitBelowZeroFails)`
- Source: `tests/training/test_training_phase2.cpp`:118
- Brief: n/a
- Parameters:
  - `<unnamed>` (DeployVersionExTest): n/a
  - `<unnamed>` (InvalidSplitBelowZeroFails): n/a

#### `TEST_F(DeployVersionExTest, PartialTrafficSplit)`
- Source: `tests/training/test_training_phase2.cpp`:131
- Brief: n/a
- Parameters:
  - `<unnamed>` (DeployVersionExTest): n/a
  - `<unnamed>` (PartialTrafficSplit): n/a

#### `TEST_F(DeployVersionExTest, ValidVersionWithNoCheckpointDirSucceeds)`
- Source: `tests/training/test_training_phase2.cpp`:124
- Brief: n/a
- Parameters:
  - `<unnamed>` (DeployVersionExTest): n/a
  - `<unnamed>` (ValidVersionWithNoCheckpointDirSucceeds): n/a

#### `TEST_F(DeployVersionExTest, ZeroSplitSucceeds)`
- Source: `tests/training/test_training_phase2.cpp`:138
- Brief: n/a
- Parameters:
  - `<unnamed>` (DeployVersionExTest): n/a
  - `<unnamed>` (ZeroSplitSucceeds): n/a

#### `TEST_F(RollbackVersionExTest, EmptyVersionFails)`
- Source: `tests/training/test_training_phase2.cpp`:153
- Brief: n/a
- Parameters:
  - `<unnamed>` (RollbackVersionExTest): n/a
  - `<unnamed>` (EmptyVersionFails): n/a

#### `TEST_F(RollbackVersionExTest, RollbackDeactivatesOtherVersions)`
- Source: `tests/training/test_training_phase2.cpp`:166
- Brief: n/a
- Parameters:
  - `<unnamed>` (RollbackVersionExTest): n/a
  - `<unnamed>` (RollbackDeactivatesOtherVersions): n/a

#### `TEST_F(RollbackVersionExTest, ValidVersionSucceeds)`
- Source: `tests/training/test_training_phase2.cpp`:159
- Brief: n/a
- Parameters:
  - `<unnamed>` (RollbackVersionExTest): n/a
  - `<unnamed>` (ValidVersionSucceeds): n/a

#### `TEST_F(RouterIntegrationTest, DeployCallsRouter)`
- Source: `tests/training/test_training_phase2.cpp`:184
- Brief: n/a
- Parameters:
  - `<unnamed>` (RouterIntegrationTest): n/a
  - `<unnamed>` (DeployCallsRouter): n/a

#### `TEST_F(RouterIntegrationTest, DeployWithPartialSplitPassesSplitToRouter)`
- Source: `tests/training/test_training_phase2.cpp`:201
- Brief: n/a
- Parameters:
  - `<unnamed>` (RouterIntegrationTest): n/a
  - `<unnamed>` (DeployWithPartialSplitPassesSplitToRouter): n/a

#### `TEST_F(RouterIntegrationTest, DeployWithoutRouterStillSucceeds)`
- Source: `tests/training/test_training_phase2.cpp`:224
- Brief: n/a
- Parameters:
  - `<unnamed>` (RouterIntegrationTest): n/a
  - `<unnamed>` (DeployWithoutRouterStillSucceeds): n/a

#### `TEST_F(RouterIntegrationTest, DetachRouterStopsRouterCalls)`
- Source: `tests/training/test_training_phase2.cpp`:216
- Brief: n/a
- Parameters:
  - `<unnamed>` (RouterIntegrationTest): n/a
  - `<unnamed>` (DetachRouterStopsRouterCalls): n/a

#### `TEST_F(RouterIntegrationTest, RollbackCallsRouter)`
- Source: `tests/training/test_training_phase2.cpp`:193
- Brief: n/a
- Parameters:
  - `<unnamed>` (RouterIntegrationTest): n/a
  - `<unnamed>` (RollbackCallsRouter): n/a

#### `TEST_F(RouterIntegrationTest, UnavailableRouterReturnsError)`
- Source: `tests/training/test_training_phase2.cpp`:208
- Brief: n/a
- Parameters:
  - `<unnamed>` (RouterIntegrationTest): n/a
  - `<unnamed>` (UnavailableRouterReturnsError): n/a

#### `std::vector< TrainingSample > labelText(const std::string &text, DomainType domain)`
- Source: `tests/training/test_training_phase2.cpp`:288
- Brief: n/a
- Parameters:
  - `text` (const std::string &): n/a
  - `domain` (DomainType): n/a

#### `IncrementalLoRATrainer makeTrainer(const std::string &checkpoint_dir="")`
- Source: `tests/training/test_training_phase2.cpp`:60
- Brief: n/a
- Parameters:
  - `checkpoint_dir` (const std::string &): n/a

### test_training_pipeline_e2e.cpp

#### `TEST_F(TrainingPipelineE2ETest, AddCalibrationSample_ThenCalibrate_ProducesThreshold)`
- Source: `tests/training/test_training_pipeline_e2e.cpp`:432
- Brief: n/a
- Parameters:
  - `<unnamed>` (TrainingPipelineE2ETest): n/a
  - `<unnamed>` (AddCalibrationSample_ThenCalibrate_ProducesThreshold): n/a

#### `TEST_F(TrainingPipelineE2ETest, CheckDataQuality_EmptyCollection_Passes)`
- Source: `tests/training/test_training_pipeline_e2e.cpp`:198
- Brief: n/a
- Parameters:
  - `<unnamed>` (TrainingPipelineE2ETest): n/a
  - `<unnamed>` (CheckDataQuality_EmptyCollection_Passes): n/a

#### `TEST_F(TrainingPipelineE2ETest, CheckDataQuality_ReturnsReport)`
- Source: `tests/training/test_training_pipeline_e2e.cpp`:192
- Brief: n/a
- Parameters:
  - `<unnamed>` (TrainingPipelineE2ETest): n/a
  - `<unnamed>` (CheckDataQuality_ReturnsReport): n/a

#### `TEST_F(TrainingPipelineE2ETest, CheckDataQuality_SummaryNotEmpty)`
- Source: `tests/training/test_training_pipeline_e2e.cpp`:206
- Brief: n/a
- Parameters:
  - `<unnamed>` (TrainingPipelineE2ETest): n/a
  - `<unnamed>` (CheckDataQuality_SummaryNotEmpty): n/a

#### `TEST_F(TrainingPipelineE2ETest, CheckpointManager_SaveLoadCalibrationJson)`
- Source: `tests/training/test_training_pipeline_e2e.cpp`:606
- Brief: n/a
- Parameters:
  - `<unnamed>` (TrainingPipelineE2ETest): n/a
  - `<unnamed>` (CheckpointManager_SaveLoadCalibrationJson): n/a

#### `TEST_F(TrainingPipelineE2ETest, Construction_EmptyDbConnection_Succeeds)`
- Source: `tests/training/test_training_pipeline_e2e.cpp`:74
- Brief: n/a
- Parameters:
  - `<unnamed>` (TrainingPipelineE2ETest): n/a
  - `<unnamed>` (Construction_EmptyDbConnection_Succeeds): n/a

#### `TEST_F(TrainingPipelineE2ETest, Construction_Succeeds)`
- Source: `tests/training/test_training_pipeline_e2e.cpp`:70
- Brief: n/a
- Parameters:
  - `<unnamed>` (TrainingPipelineE2ETest): n/a
  - `<unnamed>` (Construction_Succeeds): n/a

#### `TEST_F(TrainingPipelineE2ETest, DetectLabelDrift_NoDrift_WhenNoDb)`
- Source: `tests/training/test_training_pipeline_e2e.cpp`:222
- Brief: n/a
- Parameters:
  - `<unnamed>` (TrainingPipelineE2ETest): n/a
  - `<unnamed>` (DetectLabelDrift_NoDrift_WhenNoDb): n/a

#### `TEST_F(TrainingPipelineE2ETest, DetectLabelDrift_NoReference_ReturnsReport)`
- Source: `tests/training/test_training_pipeline_e2e.cpp`:216
- Brief: n/a
- Parameters:
  - `<unnamed>` (TrainingPipelineE2ETest): n/a
  - `<unnamed>` (DetectLabelDrift_NoReference_ReturnsReport): n/a

#### `TEST_F(TrainingPipelineE2ETest, DetectLabelDrift_SummaryNotEmpty)`
- Source: `tests/training/test_training_pipeline_e2e.cpp`:235
- Brief: n/a
- Parameters:
  - `<unnamed>` (TrainingPipelineE2ETest): n/a
  - `<unnamed>` (DetectLabelDrift_SummaryNotEmpty): n/a

#### `TEST_F(TrainingPipelineE2ETest, DetectLabelDrift_WithReference_DoesNotThrow)`
- Source: `tests/training/test_training_pipeline_e2e.cpp`:229
- Brief: n/a
- Parameters:
  - `<unnamed>` (TrainingPipelineE2ETest): n/a
  - `<unnamed>` (DetectLabelDrift_WithReference_DoesNotThrow): n/a

#### `TEST_F(TrainingPipelineE2ETest, GetLastStats_AfterRun_ReturnsStats)`
- Source: `tests/training/test_training_pipeline_e2e.cpp`:149
- Brief: n/a
- Parameters:
  - `<unnamed>` (TrainingPipelineE2ETest): n/a
  - `<unnamed>` (GetLastStats_AfterRun_ReturnsStats): n/a

#### `TEST_F(TrainingPipelineE2ETest, HyperparamSearch_BestRankAndLrPopulated)`
- Source: `tests/training/test_training_pipeline_e2e.cpp`:589
- Brief: n/a
- Parameters:
  - `<unnamed>` (TrainingPipelineE2ETest): n/a
  - `<unnamed>` (HyperparamSearch_BestRankAndLrPopulated): n/a

#### `TEST_F(TrainingPipelineE2ETest, HyperparamSearch_BudgetZero_RunsAllTrials)`
- Source: `tests/training/test_training_pipeline_e2e.cpp`:567
- Brief: n/a
- Parameters:
  - `<unnamed>` (TrainingPipelineE2ETest): n/a
  - `<unnamed>` (HyperparamSearch_BudgetZero_RunsAllTrials): n/a

#### `TEST_F(TrainingPipelineE2ETest, HyperparamSearch_CallbackFiredForEachTrial)`
- Source: `tests/training/test_training_pipeline_e2e.cpp`:548
- Brief: n/a
- Parameters:
  - `<unnamed>` (TrainingPipelineE2ETest): n/a
  - `<unnamed>` (HyperparamSearch_CallbackFiredForEachTrial): n/a

#### `TEST_F(TrainingPipelineE2ETest, HyperparamSearch_DeterministicOrdering_SameSeedSameTrials)`
- Source: `tests/training/test_training_pipeline_e2e.cpp`:530
- Brief: n/a
- Parameters:
  - `<unnamed>` (TrainingPipelineE2ETest): n/a
  - `<unnamed>` (HyperparamSearch_DeterministicOrdering_SameSeedSameTrials): n/a

#### `TEST_F(TrainingPipelineE2ETest, HyperparamSearch_ElapsedSecondsPositive)`
- Source: `tests/training/test_training_pipeline_e2e.cpp`:579
- Brief: n/a
- Parameters:
  - `<unnamed>` (TrainingPipelineE2ETest): n/a
  - `<unnamed>` (HyperparamSearch_ElapsedSecondsPositive): n/a

#### `TEST_F(TrainingPipelineE2ETest, HyperparamSearch_EmptyCandidates_ReturnsFailure)`
- Source: `tests/training/test_training_pipeline_e2e.cpp`:476
- Brief: n/a
- Parameters:
  - `<unnamed>` (TrainingPipelineE2ETest): n/a
  - `<unnamed>` (HyperparamSearch_EmptyCandidates_ReturnsFailure): n/a

#### `TEST_F(TrainingPipelineE2ETest, HyperparamSearch_EmptyLrCandidates_ReturnsFailure)`
- Source: `tests/training/test_training_pipeline_e2e.cpp`:495
- Brief: n/a
- Parameters:
  - `<unnamed>` (TrainingPipelineE2ETest): n/a
  - `<unnamed>` (HyperparamSearch_EmptyLrCandidates_ReturnsFailure): n/a

#### `TEST_F(TrainingPipelineE2ETest, HyperparamSearch_EmptyRankCandidates_ReturnsFailure)`
- Source: `tests/training/test_training_pipeline_e2e.cpp`:485
- Brief: n/a
- Parameters:
  - `<unnamed>` (TrainingPipelineE2ETest): n/a
  - `<unnamed>` (HyperparamSearch_EmptyRankCandidates_ReturnsFailure): n/a

#### `TEST_F(TrainingPipelineE2ETest, HyperparamSearch_MaxTrialsLimitsCandidates)`
- Source: `tests/training/test_training_pipeline_e2e.cpp`:519
- Brief: n/a
- Parameters:
  - `<unnamed>` (TrainingPipelineE2ETest): n/a
  - `<unnamed>` (HyperparamSearch_MaxTrialsLimitsCandidates): n/a

#### `TEST_F(TrainingPipelineE2ETest, HyperparamSearch_SingleTrial_RunsAndReturns)`
- Source: `tests/training/test_training_pipeline_e2e.cpp`:505
- Brief: n/a
- Parameters:
  - `<unnamed>` (TrainingPipelineE2ETest): n/a
  - `<unnamed>` (HyperparamSearch_SingleTrial_RunsAndReturns): n/a

#### `TEST_F(TrainingPipelineE2ETest, PipelineConfig_HasDataSelectionFields)`
- Source: `tests/training/test_training_pipeline_e2e.cpp`:367
- Brief: n/a
- Parameters:
  - `<unnamed>` (TrainingPipelineE2ETest): n/a
  - `<unnamed>` (PipelineConfig_HasDataSelectionFields): n/a

#### `TEST_F(TrainingPipelineE2ETest, PipelineConfig_ProvenanceFields_Defaults)`
- Source: `tests/training/test_training_pipeline_e2e.cpp`:387
- Brief: n/a
- Parameters:
  - `<unnamed>` (TrainingPipelineE2ETest): n/a
  - `<unnamed>` (PipelineConfig_ProvenanceFields_Defaults): n/a

#### `TEST_F(TrainingPipelineE2ETest, PipelineStats_DefaultValues)`
- Source: `tests/training/test_training_pipeline_e2e.cpp`:292
- Brief: n/a
- Parameters:
  - `<unnamed>` (TrainingPipelineE2ETest): n/a
  - `<unnamed>` (PipelineStats_DefaultValues): n/a

#### `TEST_F(TrainingPipelineE2ETest, RunCalibration_EmptySamples_Succeeds)`
- Source: `tests/training/test_training_pipeline_e2e.cpp`:425
- Brief: n/a
- Parameters:
  - `<unnamed>` (TrainingPipelineE2ETest): n/a
  - `<unnamed>` (RunCalibration_EmptySamples_Succeeds): n/a

#### `TEST_F(TrainingPipelineE2ETest, RunCalibration_WithCheckpointManager_PersistsManifest)`
- Source: `tests/training/test_training_pipeline_e2e.cpp`:446
- Brief: n/a
- Parameters:
  - `<unnamed>` (TrainingPipelineE2ETest): n/a
  - `<unnamed>` (RunCalibration_WithCheckpointManager_PersistsManifest): n/a

#### `TEST_F(TrainingPipelineE2ETest, RunDataSelection_AuditEntryPresent)`
- Source: `tests/training/test_training_pipeline_e2e.cpp`:325
- Brief: n/a
- Parameters:
  - `<unnamed>` (TrainingPipelineE2ETest): n/a
  - `<unnamed>` (RunDataSelection_AuditEntryPresent): n/a

#### `TEST_F(TrainingPipelineE2ETest, RunDataSelection_Succeeds)`
- Source: `tests/training/test_training_pipeline_e2e.cpp`:317
- Brief: n/a
- Parameters:
  - `<unnamed>` (TrainingPipelineE2ETest): n/a
  - `<unnamed>` (RunDataSelection_Succeeds): n/a

#### `TEST_F(TrainingPipelineE2ETest, RunDataSelection_WithCallback_DoesNotThrow)`
- Source: `tests/training/test_training_pipeline_e2e.cpp`:332
- Brief: n/a
- Parameters:
  - `<unnamed>` (TrainingPipelineE2ETest): n/a
  - `<unnamed>` (RunDataSelection_WithCallback_DoesNotThrow): n/a

#### `TEST_F(TrainingPipelineE2ETest, RunEnrichment_Succeeds)`
- Source: `tests/training/test_training_pipeline_e2e.cpp`:167
- Brief: n/a
- Parameters:
  - `<unnamed>` (TrainingPipelineE2ETest): n/a
  - `<unnamed>` (RunEnrichment_Succeeds): n/a

#### `TEST_F(TrainingPipelineE2ETest, RunLabeling_Succeeds)`
- Source: `tests/training/test_training_pipeline_e2e.cpp`:160
- Brief: n/a
- Parameters:
  - `<unnamed>` (TrainingPipelineE2ETest): n/a
  - `<unnamed>` (RunLabeling_Succeeds): n/a

#### `TEST_F(TrainingPipelineE2ETest, RunTraining_Succeeds)`
- Source: `tests/training/test_training_pipeline_e2e.cpp`:174
- Brief: n/a
- Parameters:
  - `<unnamed>` (TrainingPipelineE2ETest): n/a
  - `<unnamed>` (RunTraining_Succeeds): n/a

#### `TEST_F(TrainingPipelineE2ETest, RunTraining_WithCallback_DoesNotThrow)`
- Source: `tests/training/test_training_pipeline_e2e.cpp`:181
- Brief: n/a
- Parameters:
  - `<unnamed>` (TrainingPipelineE2ETest): n/a
  - `<unnamed>` (RunTraining_WithCallback_DoesNotThrow): n/a

#### `TEST_F(TrainingPipelineE2ETest, Run_AccuracyInValidRange)`
- Source: `tests/training/test_training_pipeline_e2e.cpp`:109
- Brief: n/a
- Parameters:
  - `<unnamed>` (TrainingPipelineE2ETest): n/a
  - `<unnamed>` (Run_AccuracyInValidRange): n/a

#### `TEST_F(TrainingPipelineE2ETest, Run_Completes_WithoutException)`
- Source: `tests/training/test_training_pipeline_e2e.cpp`:82
- Brief: n/a
- Parameters:
  - `<unnamed>` (TrainingPipelineE2ETest): n/a
  - `<unnamed>` (Run_Completes_WithoutException): n/a

#### `TEST_F(TrainingPipelineE2ETest, Run_DataSelectionStageReported_InCallback)`
- Source: `tests/training/test_training_pipeline_e2e.cpp`:344
- Brief: n/a
- Parameters:
  - `<unnamed>` (TrainingPipelineE2ETest): n/a
  - `<unnamed>` (Run_DataSelectionStageReported_InCallback): n/a

#### `TEST_F(TrainingPipelineE2ETest, Run_ElapsedTimeRecorded)`
- Source: `tests/training/test_training_pipeline_e2e.cpp`:87
- Brief: n/a
- Parameters:
  - `<unnamed>` (TrainingPipelineE2ETest): n/a
  - `<unnamed>` (Run_ElapsedTimeRecorded): n/a

#### `TEST_F(TrainingPipelineE2ETest, Run_LabelingOnly_SkipsOtherStages)`
- Source: `tests/training/test_training_pipeline_e2e.cpp`:261
- Brief: n/a
- Parameters:
  - `<unnamed>` (TrainingPipelineE2ETest): n/a
  - `<unnamed>` (Run_LabelingOnly_SkipsOtherStages): n/a

#### `TEST_F(TrainingPipelineE2ETest, Run_LabelingStageReported_InCallback)`
- Source: `tests/training/test_training_pipeline_e2e.cpp`:127
- Brief: n/a
- Parameters:
  - `<unnamed>` (TrainingPipelineE2ETest): n/a
  - `<unnamed>` (Run_LabelingStageReported_InCallback): n/a

#### `TEST_F(TrainingPipelineE2ETest, Run_ProvenanceDisabled_NoRecordsWritten)`
- Source: `tests/training/test_training_pipeline_e2e.cpp`:394
- Brief: n/a
- Parameters:
  - `<unnamed>` (TrainingPipelineE2ETest): n/a
  - `<unnamed>` (Run_ProvenanceDisabled_NoRecordsWritten): n/a

#### `TEST_F(TrainingPipelineE2ETest, Run_ProvenanceEnabled_DoesNotThrow)`
- Source: `tests/training/test_training_pipeline_e2e.cpp`:401
- Brief: n/a
- Parameters:
  - `<unnamed>` (TrainingPipelineE2ETest): n/a
  - `<unnamed>` (Run_ProvenanceEnabled_DoesNotThrow): n/a

#### `TEST_F(TrainingPipelineE2ETest, Run_ProvenanceStats_NonNegative)`
- Source: `tests/training/test_training_pipeline_e2e.cpp`:411
- Brief: n/a
- Parameters:
  - `<unnamed>` (TrainingPipelineE2ETest): n/a
  - `<unnamed>` (Run_ProvenanceStats_NonNegative): n/a

#### `TEST_F(TrainingPipelineE2ETest, Run_ProvenanceStats_SumEqualsTotal)`
- Source: `tests/training/test_training_pipeline_e2e.cpp`:650
- Brief: n/a
- Parameters:
  - `<unnamed>` (TrainingPipelineE2ETest): n/a
  - `<unnamed>` (Run_ProvenanceStats_SumEqualsTotal): n/a

#### `TEST_F(TrainingPipelineE2ETest, Run_ProvenanceTimeout_PipelineCompletes)`
- Source: `tests/training/test_training_pipeline_e2e.cpp`:638
- Brief: n/a
- Parameters:
  - `<unnamed>` (TrainingPipelineE2ETest): n/a
  - `<unnamed>` (Run_ProvenanceTimeout_PipelineCompletes): n/a

#### `TEST_F(TrainingPipelineE2ETest, Run_SelectionDisabled_NotReported)`
- Source: `tests/training/test_training_pipeline_e2e.cpp`:355
- Brief: n/a
- Parameters:
  - `<unnamed>` (TrainingPipelineE2ETest): n/a
  - `<unnamed>` (Run_SelectionDisabled_NotReported): n/a

#### `TEST_F(TrainingPipelineE2ETest, Run_SelectionStatsPopulated)`
- Source: `tests/training/test_training_pipeline_e2e.cpp`:374
- Brief: n/a
- Parameters:
  - `<unnamed>` (TrainingPipelineE2ETest): n/a
  - `<unnamed>` (Run_SelectionStatsPopulated): n/a

#### `TEST_F(TrainingPipelineE2ETest, Run_TrainingOnly_SkipsLabelingEnrichment)`
- Source: `tests/training/test_training_pipeline_e2e.cpp`:275
- Brief: n/a
- Parameters:
  - `<unnamed>` (TrainingPipelineE2ETest): n/a
  - `<unnamed>` (Run_TrainingOnly_SkipsLabelingEnrichment): n/a

#### `TEST_F(TrainingPipelineE2ETest, Run_TrainingStageReported_InCallback)`
- Source: `tests/training/test_training_pipeline_e2e.cpp`:138
- Brief: n/a
- Parameters:
  - `<unnamed>` (TrainingPipelineE2ETest): n/a
  - `<unnamed>` (Run_TrainingStageReported_InCallback): n/a

#### `TEST_F(TrainingPipelineE2ETest, Run_TrainingStage_LossIsNonNegative)`
- Source: `tests/training/test_training_pipeline_e2e.cpp`:103
- Brief: n/a
- Parameters:
  - `<unnamed>` (TrainingPipelineE2ETest): n/a
  - `<unnamed>` (Run_TrainingStage_LossIsNonNegative): n/a

#### `TEST_F(TrainingPipelineE2ETest, Run_TrainingStage_ProducesVersion)`
- Source: `tests/training/test_training_pipeline_e2e.cpp`:93
- Brief: n/a
- Parameters:
  - `<unnamed>` (TrainingPipelineE2ETest): n/a
  - `<unnamed>` (Run_TrainingStage_ProducesVersion): n/a

#### `TEST_F(TrainingPipelineE2ETest, Run_WithCallback_DoesNotThrow)`
- Source: `tests/training/test_training_pipeline_e2e.cpp`:116
- Brief: n/a
- Parameters:
  - `<unnamed>` (TrainingPipelineE2ETest): n/a
  - `<unnamed>` (Run_WithCallback_DoesNotThrow): n/a

#### `TEST_F(TrainingPipelineE2ETest, ScheduleRetraining_DoesNotThrow)`
- Source: `tests/training/test_training_pipeline_e2e.cpp`:245
- Brief: n/a
- Parameters:
  - `<unnamed>` (TrainingPipelineE2ETest): n/a
  - `<unnamed>` (ScheduleRetraining_DoesNotThrow): n/a

#### `TEST_F(TrainingPipelineE2ETest, ScheduleRetraining_WithCallback_DoesNotThrow)`
- Source: `tests/training/test_training_pipeline_e2e.cpp`:250
- Brief: n/a
- Parameters:
  - `<unnamed>` (TrainingPipelineE2ETest): n/a
  - `<unnamed>` (ScheduleRetraining_WithCallback_DoesNotThrow): n/a

### test_training_service_registry.cpp

#### `TEST_F(TrainingServiceRegistryTest, Clear_RemovesAllRegistrations)`
- Source: `tests/training/test_training_service_registry.cpp`:136
- Brief: n/a
- Parameters:
  - `<unnamed>` (TrainingServiceRegistryTest): n/a
  - `<unnamed>` (Clear_RemovesAllRegistrations): n/a

#### `TEST_F(TrainingServiceRegistryTest, ConcurrentAccess_ThreadSafe)`
- Source: `tests/training/test_training_service_registry.cpp`:200
- Brief: n/a
- Parameters:
  - `<unnamed>` (TrainingServiceRegistryTest): n/a
  - `<unnamed>` (ConcurrentAccess_ThreadSafe): n/a

#### `TEST_F(TrainingServiceRegistryTest, ConcurrentRegistration_ThreadSafe)`
- Source: `tests/training/test_training_service_registry.cpp`:172
- Brief: n/a
- Parameters:
  - `<unnamed>` (TrainingServiceRegistryTest): n/a
  - `<unnamed>` (ConcurrentRegistration_ThreadSafe): n/a

#### `TEST_F(TrainingServiceRegistryTest, InitialState_NoInfrastructure)`
- Source: `tests/training/test_training_service_registry.cpp`:60
- Brief: n/a
- Parameters:
  - `<unnamed>` (TrainingServiceRegistryTest): n/a
  - `<unnamed>` (InitialState_NoInfrastructure): n/a

#### `TEST_F(TrainingServiceRegistryTest, RegisterBoth_InfrastructureAvailable)`
- Source: `tests/training/test_training_service_registry.cpp`:98
- Brief: n/a
- Parameters:
  - `<unnamed>` (TrainingServiceRegistryTest): n/a
  - `<unnamed>` (RegisterBoth_InfrastructureAvailable): n/a

#### `TEST_F(TrainingServiceRegistryTest, RegisterNull_HandledCorrectly)`
- Source: `tests/training/test_training_service_registry.cpp`:156
- Brief: n/a
- Parameters:
  - `<unnamed>` (TrainingServiceRegistryTest): n/a
  - `<unnamed>` (RegisterNull_HandledCorrectly): n/a

#### `TEST_F(TrainingServiceRegistryTest, RegisterOnlyRouter_InfrastructureNotAvailable)`
- Source: `tests/training/test_training_service_registry.cpp`:112
- Brief: n/a
- Parameters:
  - `<unnamed>` (TrainingServiceRegistryTest): n/a
  - `<unnamed>` (RegisterOnlyRouter_InfrastructureNotAvailable): n/a

#### `TEST_F(TrainingServiceRegistryTest, RegisterOnlyTopology_InfrastructureNotAvailable)`
- Source: `tests/training/test_training_service_registry.cpp`:125
- Brief: n/a
- Parameters:
  - `<unnamed>` (TrainingServiceRegistryTest): n/a
  - `<unnamed>` (RegisterOnlyTopology_InfrastructureNotAvailable): n/a

#### `TEST_F(TrainingServiceRegistryTest, RegisterShardRouter_Success)`
- Source: `tests/training/test_training_service_registry.cpp`:68
- Brief: n/a
- Parameters:
  - `<unnamed>` (TrainingServiceRegistryTest): n/a
  - `<unnamed>` (RegisterShardRouter_Success): n/a

#### `TEST_F(TrainingServiceRegistryTest, RegisterShardTopology_Success)`
- Source: `tests/training/test_training_service_registry.cpp`:85
- Brief: n/a
- Parameters:
  - `<unnamed>` (TrainingServiceRegistryTest): n/a
  - `<unnamed>` (RegisterShardTopology_Success): n/a

#### `TEST_F(TrainingServiceRegistryTest, RegistryLifecycle_CompleteFlow)`
- Source: `tests/training/test_training_service_registry.cpp`:244
- Brief: n/a
- Parameters:
  - `<unnamed>` (TrainingServiceRegistryTest): n/a
  - `<unnamed>` (RegistryLifecycle_CompleteFlow): n/a

#### `TEST_F(TrainingServiceRegistryTest, ReplaceRegistration_NewInstanceReplacesPrevious)`
- Source: `tests/training/test_training_service_registry.cpp`:276
- Brief: n/a
- Parameters:
  - `<unnamed>` (TrainingServiceRegistryTest): n/a
  - `<unnamed>` (ReplaceRegistration_NewInstanceReplacesPrevious): n/a

#### `TEST_F(TrainingServiceRegistryTest, Singleton_ReturnsSameInstance)`
- Source: `tests/training/test_training_service_registry.cpp`:52
- Brief: n/a
- Parameters:
  - `<unnamed>` (TrainingServiceRegistryTest): n/a
  - `<unnamed>` (Singleton_ReturnsSameInstance): n/a

### themis::DataSample

#### `DataSample()=default`
- Source: `include/training/lora_data_selection.h`:137
- Brief: n/a
- Parameters: none

#### `DataSample(std::string id_, std::string text_)`
- Source: `include/training/lora_data_selection.h`:138
- Brief: n/a
- Parameters:
  - `id_` (std::string): n/a
  - `text_` (std::string): n/a

### themis::DataSelectionPipeline

#### `DataSelectionPipeline(const DataSelectionPipeline &)=delete`
- Source: `include/training/lora_data_selection.h`:288
- Brief: n/a
- Parameters:
  - `<unnamed>` (const DataSelectionPipeline &): n/a

#### `DataSelectionPipeline(const LoRADataSelectionConfig &config)`
- Source: `include/training/lora_data_selection.h`:285
- Brief: Construct pipeline with the given configuration.
- Parameters:
  - `config` (const LoRADataSelectionConfig &): n/a

#### `std::vector< DataSample > clusterAndSample(const std::vector< DataSample > &samples, size_t k=0) const`
- Source: `include/training/lora_data_selection.h`:326
- Brief: Run only Stage 3: cluster-based diversity sampling.
- Parameters:
  - `samples` (const std::vector< DataSample > &): Samples to cluster.
  - `k` (size_t): Number of clusters (computed from config if 0).
- Return: Centroid-nearest samples covering diverse clusters.
- Details: samples Samples to cluster. k Number of clusters (computed from config if 0). Centroid-nearest samples covering diverse clusters.

#### `DataSelectionMetrics computeMetrics(const DataSelectionResult &result)`
- Source: `include/training/lora_data_selection.h`:368
- Brief: Derive a DataSelectionMetrics snapshot from a completed pipeline result.
- Parameters:
  - `result` (const DataSelectionResult &): Input parameter.
- Return: Populated metrics snapshot (all fields 0.0 for empty results).
- Details: Compute Metrics. Computes per-stage rejection rates and average quality/difficulty/ diversity scores from the returned samples. Useful for feeding the result directly into SelfImprovementModule::applyAdaptiveRules() and SelfImprovementModule::needsRollback(). result The result returned by a previous run() call. Populated metrics snapshot (all fields 0.0 for empty results). result Input parameter. Return value. Calls: DataSelectionMetrics(), empty(), ss(), reserve(), size(), std::isalpha(), std::tolower(), std::isdigit().

#### `std::vector< DataSample > deduplicate(const std::vector< DataSample > &samples) const`
- Source: `include/training/lora_data_selection.h`:317
- Brief: Run only Stage 2: MinHash deduplication.
- Parameters:
  - `samples` (const std::vector< DataSample > &): n/a
- Return: Samples with duplicates removed.
- Details: Samples with duplicates removed.

#### `std::vector< DataSample > filterByQuality(const std::vector< DataSample > &samples) const`
- Source: `include/training/lora_data_selection.h`:310
- Brief: Run only Stage 1: quality filtering.
- Parameters:
  - `samples` (const std::vector< DataSample > &): n/a
- Return: Samples that pass all quality filters.
- Details: Applies the shared prompt-safety policy before token/language/toxicity checks. Samples matching blocked prompt-injection patterns are rejected (fail-closed). Allowed samples continue with sanitized control-token redaction applied to their text. Samples that pass all quality filters.

#### `const LoRADataSelectionConfig & getConfig() const`
- Source: `include/training/lora_data_selection.h`:354
- Brief: Get the current pipeline configuration.
- Parameters: none

#### `DataSelectionPipeline & operator=(const DataSelectionPipeline &)=delete`
- Source: `include/training/lora_data_selection.h`:289
- Brief: n/a
- Parameters:
  - `<unnamed>` (const DataSelectionPipeline &): n/a

#### `DataSelectionResult run(const std::vector< DataSample > &input_samples, SelectionProgressCallback callback=nullptr)`
- Source: `include/training/lora_data_selection.h`:297
- Brief: Execute all five pipeline stages on input_samples.
- Parameters:
  - `input_samples` (const std::vector< DataSample > &): Input parameter.
  - `callback` (SelectionProgressCallback): Input parameter.
- Return: Selection result including selected samples and audit entry.
- Details: Run. input_samples Raw samples loaded from the training collection. callback Optional per-stage progress callback. Selection result including selected samples and audit entry. input_samples Input parameter. callback Input parameter. Return value. Calls: std::move(), getConfig(), empty(), detail::appendAuditJSONL(), toJSONL().

#### `void scoreQualityAndDifficulty(std::vector< DataSample > &samples) const`
- Source: `include/training/lora_data_selection.h`:334
- Brief: Run only Stage 4: quality/difficulty scoring. Modifies quality_score and difficulty_score in-place.
- Parameters:
  - `samples` (std::vector< DataSample > &): n/a

#### `void setConfig(const LoRADataSelectionConfig &config)`
- Source: `include/training/lora_data_selection.h`:349
- Brief: Update the pipeline configuration (live reload support).
- Parameters:
  - `config` (const LoRADataSelectionConfig &): Input parameter.
- Details: Set Config. config Input parameter. Implements setConfig without additional internal calls.

#### `std::vector< DataSample > stratifiedSample(const std::vector< DataSample > &scored_samples, size_t target=0) const`
- Source: `include/training/lora_data_selection.h`:342
- Brief: Run only Stage 5: curriculum stratified sampling.
- Parameters:
  - `scored_samples` (const std::vector< DataSample > &): Samples with difficulty_score populated.
  - `target` (size_t): Total samples to return (0 = use config).
- Return: Stratified subset (easy + medium + hard).
- Details: scored_samples Samples with difficulty_score populated. target Total samples to return (0 = use config). Stratified subset (easy + medium + hard).

#### `~DataSelectionPipeline()`
- Source: `include/training/lora_data_selection.h`:286
- Brief: n/a
- Parameters: none

### themis::DataSelectionPipeline::Impl

#### `Impl(const LoRADataSelectionConfig &config)`
- Source: `src/training/lora_data_selection.cpp`:404
- Brief: n/a
- Parameters:
  - `config` (const LoRADataSelectionConfig &): n/a

#### `std::vector< DataSample > clusterAndSample(const std::vector< DataSample > &samples, size_t k) const`
- Source: `src/training/lora_data_selection.cpp`:506
- Brief: n/a
- Parameters:
  - `samples` (const std::vector< DataSample > &): n/a
  - `k` (size_t): n/a

#### `std::vector< DataSample > deduplicate(const std::vector< DataSample > &samples) const`
- Source: `src/training/lora_data_selection.cpp`:466
- Brief: n/a
- Parameters:
  - `samples` (const std::vector< DataSample > &): n/a

#### `std::vector< DataSample > filterByQuality(const std::vector< DataSample > &samples) const`
- Source: `src/training/lora_data_selection.cpp`:407
- Brief: n/a
- Parameters:
  - `samples` (const std::vector< DataSample > &): n/a

#### `const LoRADataSelectionConfig & getConfig() const`
- Source: `src/training/lora_data_selection.cpp`:798
- Brief: n/a
- Parameters: none

#### `DataSelectionResult run(const std::vector< DataSample > &input, SelectionProgressCallback cb)`
- Source: `src/training/lora_data_selection.cpp`:722
- Brief: - Full pipeline run ----------------------------------------------
- Parameters:
  - `input` (const std::vector< DataSample > &): Input parameter.
  - `cb` (SelectionProgressCallback): Input parameter.
- Return: Return value.
- Details: input Input parameter. cb Input parameter. Return value. Calls: DataSelectionResult(), std::chrono::steady_clock::now(), filterByQuality(), cb(), size(), deduplicate(), clusterAndSample(), scoreQualityAndDifficulty().

#### `void scoreQualityAndDifficulty(std::vector< DataSample > &samples) const`
- Source: `src/training/lora_data_selection.cpp`:628
- Brief: n/a
- Parameters:
  - `samples` (std::vector< DataSample > &): n/a

#### `void setConfig(const LoRADataSelectionConfig &config)`
- Source: `src/training/lora_data_selection.cpp`:797
- Brief: Set Config.
- Parameters:
  - `config` (const LoRADataSelectionConfig &): Input parameter.
- Details: config Input parameter. Implements setConfig without additional internal calls.

#### `std::vector< DataSample > stratifiedSample(const std::vector< DataSample > &scored, size_t target) const`
- Source: `src/training/lora_data_selection.cpp`:655
- Brief: n/a
- Parameters:
  - `scored` (const std::vector< DataSample > &): n/a
  - `target` (size_t): n/a

### themis::LoRADataSelectionConfig

#### `LoRADataSelectionConfig()=default`
- Source: `include/training/lora_data_selection.h`:84
- Brief: n/a
- Parameters: none

#### `LoRADataSelectionConfig fromYAMLString(const std::string &yaml_text, const std::string &section="lora_data_selection")`
- Source: `include/training/lora_data_selection.h`:111
- Brief: Parse configuration from an in-memory YAML string.
- Parameters:
  - `yaml_text` (const std::string &): Input parameter.
  - `section` (const std::string &): Input parameter.
- Return: Return value.
- Details: From YAMLString. Useful for unit testing or when the YAML content is already loaded. yaml_text YAML text containing the data-selection section. section Top-level key to read (default: lora_data_selection). yaml_text Input parameter. section Input parameter. Return value. Calls: yaml_detail::parseYAMLText().

#### `LoRADataSelectionConfig loadFromYAML(const std::string &path, const std::string &section="lora_data_selection")`
- Source: `include/training/lora_data_selection.h`:99
- Brief: Load configuration from a YAML file.
- Parameters:
  - `path` (const std::string &): Input parameter.
  - `section` (const std::string &): Input parameter.
- Return: Return value.
- Throws:
  - std::runtime_error: if the file cannot be opened.
  - std::runtime_error: if an error occurs.
- Details: Load From YAML. Reads the section block (default: lora_data_selection) from the YAML file at path and fills a new config object. Uses a built-in line-by-line parser – no external yaml-cpp dependency. Supports live-reload: call again at any time to obtain an updated config. path Path to the YAML configuration file. section Top-level YAML key containing the data-selection block. std::runtime_error if the file cannot be opened. path Input parameter. section Input parameter. Return value. std::runtime_error if an error occurs. Calls: f(), is_open(), rdbuf(), yaml_detail::parseYAMLText(), str().

### themis::PipelineConfig

#### `PipelineConfig()=default`
- Source: `include/training/training_pipeline.h`:297
- Brief: n/a
- Parameters: none

### themis::training

#### `std::string computeDeploymentFingerprint(const std::string &adapter_version, const std::string &checkpoint_sha256)`
- Source: `src/training/adapter_serving.cpp`:112
- Brief: Generate deterministic deployment fingerprint for an adapter.
- Parameters:
  - `adapter_version` (const std::string &): Input parameter.
  - `checkpoint_sha256` (const std::string &): Input parameter.
- Return: Deterministic fingerprint string (hex-encoded)
- Details: Compute Deployment Fingerprint. Creates a stable identifier based on the adapter version and checkpoint hash. Used to ensure repeatable deployment decisions and audit trail consistency. adapter_version Version identifier checkpoint_sha256 SHA-256 of the checkpoint file Deterministic fingerprint string (hex-encoded) adapter_version Input parameter. checkpoint_sha256 Input parameter. Return value. Calls: str().

#### `std::string jsonEscape(const std::string &s)`
- Source: `src/training/lora_data_selection.cpp`:1190
- Brief: Json Escape.
- Parameters:
  - `s` (const std::string &): Input parameter.
- Return: Return value.
- Details: s Input parameter. Return value. Calls: reserve(), size(), std::string().

#### `std::string trainingErrorCodeToString(TrainingErrorCode code)`
- Source: `include/training/training_error_codes.h`:469
- Brief: Convert TrainingErrorCode to human-readable string.
- Parameters:
  - `code` (TrainingErrorCode): Error code to convert.
- Return: Descriptive string for the error code.
- Details: code Error code to convert. Descriptive string for the error code.

#### `const char * trainingIncidentClassName(TrainingIncidentClass cls) noexcept`
- Source: `include/training/training_incident_emitter.h`:74
- Brief: Human-readable label for a TrainingIncidentClass value.
- Parameters:
  - `cls` (TrainingIncidentClass): Incident class discriminator.
- Return: Null-terminated string literal ("dataset", "training", or "adapter").
- Details: cls Incident class discriminator. Null-terminated string literal ("dataset", "training", or "adapter").

#### `std::string validateDeploymentReadiness(const std::string &adapter_version, const std::string &checkpoint_path, const std::string &expected_sha256="")`
- Source: `src/training/adapter_serving.cpp`:43
- Brief: Validate an adapter before deployment to serving infrastructure.
- Parameters:
  - `adapter_version` (const std::string &): Input parameter.
  - `checkpoint_path` (const std::string &): Path to the checkpoint.
  - `expected_sha256` (const std::string &): Input parameter.
- Return: Empty string if valid; otherwise error message describing the issue
- Details: ============================================================================ Phase 2: Deployment Validation Implementation ============================================================================ Phase 2 hardening: performs comprehensive deployment readiness checks including version string validation, state consistency, and pre-deployment health assessment. adapter_version Version identifier (must be non-empty, valid format) checkpoint_path Path to the checkpoint file (must exist and be readable) expected_sha256 Expected SHA-256 of checkpoint (if empty, skipped) Empty string if valid; otherwise error message describing the issue adapter_version Input parameter. checkpoint_path Path to the checkpoint. expected_sha256 Input parameter. Return value. Calls: empty(), length(), std::isalnum(), std::string(), f(), is_open(), seekg(), tellg().

### themis::training::AdaLoRAAdapter

#### `AdaLoRAAdapter(AdaLoRAAdapter &&) noexcept`
- Source: `include/training/ada_lora_adapter.h`:78
- Brief: n/a
- Parameters:
  - `<unnamed>` (AdaLoRAAdapter &&): n/a

#### `AdaLoRAAdapter(const AdaLoRAAdapter &)=delete`
- Source: `include/training/ada_lora_adapter.h`:76
- Brief: n/a
- Parameters:
  - `<unnamed>` (const AdaLoRAAdapter &): n/a

#### `AdaLoRAAdapter(size_t default_rank=4, float default_alpha=8.0f, size_t rank_budget=64)`
- Source: `include/training/ada_lora_adapter.h`:70
- Brief: Construct AdaLoRA adapter.
- Parameters:
  - `default_rank` (size_t): Default maximum rank per layer (> 0)
  - `default_alpha` (float): Default LoRA alpha (> 0)
  - `rank_budget` (size_t): Global rank budget used by reallocateRanks() (> 0)
- Details: default_rank Default maximum rank per layer (> 0) default_alpha Default LoRA alpha (> 0) rank_budget Global rank budget used by reallocateRanks() (> 0)

#### `void addLayer(const std::string &layer_name, size_t in_dim, size_t out_dim, size_t max_rank=0, float alpha=0.0f)`
- Source: `include/training/ada_lora_adapter.h`:98
- Brief: Register a new adapter layer.
- Parameters:
  - `layer_name` (const std::string &): Name of the layer.
  - `in_dim` (size_t): Input parameter.
  - `out_dim` (size_t): Input parameter.
  - `max_rank` (size_t): Input parameter.
  - `alpha` (float): Input parameter.
- Throws:
  - std::invalid_argument: on bad dimensions or duplicate name
- Details: Add Layer. B is Kaiming-uniform initialised; A is zero-initialised. The effective rank starts at max_rank (or the adapter default if 0). layer_name Unique identifier (must not already exist) in_dim Input feature dimension (> 0) out_dim Output feature dimension (> 0) max_rank Maximum rank for this layer (0 = use adapter default) alpha LoRA alpha override (0.0 = use adapter default) std::invalid_argument on bad dimensions or duplicate name layer_name Name of the layer. in_dim Input parameter. out_dim Input parameter. max_rank Input parameter. alpha Input parameter. Implements addLayer without additional internal calls.

#### `std::vector< float > forward(const std::string &layer_name, const std::vector< float > &input, size_t batch_size) const`
- Source: `include/training/ada_lora_adapter.h`:235
- Brief: Compute the LoRA contribution using only the active rank components.
- Parameters:
  - `layer_name` (const std::string &): Layer whose weights are applied
  - `input` (const std::vector< float > &): Flat row-major input (batch_size × in_dim)
  - `batch_size` (size_t): Number of rows in input
- Return: Flat row-major output (batch_size × out_dim)
- Throws:
  - std::out_of_range: if the layer does not exist
  - std::invalid_argument: if input size != batch_size × in_dim
- Details: Performs (using only the first active_rank columns of B / rows of A): hidden = input @ B[:, :active_rank] (batch_size × active_rank) output = hidden @ A[:active_rank, :] (batch_size × out_dim) return output × scaling where scaling = alpha / max_rank layer_name Layer whose weights are applied input Flat row-major input (batch_size × in_dim) batch_size Number of rows in input Flat row-major output (batch_size × out_dim) std::out_of_range if the layer does not exist std::invalid_argument if input size != batch_size × in_dim

#### `size_t getActiveRank(const std::string &layer_name) const`
- Source: `include/training/ada_lora_adapter.h`:166
- Brief: Current active rank for a layer.
- Parameters:
  - `layer_name` (const std::string &): n/a
- Throws:
  - std::out_of_range: if the layer does not exist
- Details: std::out_of_range if the layer does not exist

#### `float getImportance(const std::string &layer_name) const`
- Source: `include/training/ada_lora_adapter.h`:178
- Brief: Current importance score for a layer.
- Parameters:
  - `layer_name` (const std::string &): n/a
- Throws:
  - std::out_of_range: if the layer does not exist
- Details: std::out_of_range if the layer does not exist

#### `std::vector< AdaLoRALayerStats > getLayerStats() const`
- Source: `include/training/ada_lora_adapter.h`:184
- Brief: Snapshot of all layer statistics.
- Parameters: none
- Return: Vector of AdaLoRALayerStats (one per registered layer)
- Details: Vector of AdaLoRALayerStats (one per registered layer)

#### `size_t getMaxRank(const std::string &layer_name) const`
- Source: `include/training/ada_lora_adapter.h`:172
- Brief: Maximum rank for a layer.
- Parameters:
  - `layer_name` (const std::string &): n/a
- Throws:
  - std::out_of_range: if the layer does not exist
- Details: std::out_of_range if the layer does not exist

#### `std::pair< std::vector< float >, std::vector< float > > getWeights(const std::string &layer_name) const`
- Source: `include/training/ada_lora_adapter.h`:214
- Brief: Read B and A weight matrices for a layer.
- Parameters:
  - `layer_name` (const std::string &): n/a
- Throws:
  - std::out_of_range: if the layer does not exist
- Details: std::out_of_range if the layer does not exist

#### `bool hasLayer(const std::string &layer_name) const`
- Source: `include/training/ada_lora_adapter.h`:111
- Brief: Whether a layer with the given name is registered.
- Parameters:
  - `layer_name` (const std::string &): n/a

#### `bool isCacheValid(const std::string &checkpoint_path, const std::string &current_fingerprint)`
- Source: `include/training/ada_lora_adapter.h`:304
- Brief: Check whether a saved checkpoint is still valid for the given model.
- Parameters:
  - `checkpoint_path` (const std::string &): Path to the checkpoint.
  - `current_fingerprint` (const std::string &): Input parameter.
- Return: true — checkpoint exists and fingerprint matches (no rebuild needed).
- Details: ─── isCacheValid ───────────────────────────────────────────────────────────── Reads only the header of checkpoint_path and compares the stored fingerprint against current_fingerprint. checkpoint_path Path to a checkpoint written by saveToFile(). current_fingerprint SHA-256 fingerprint of the current base model. true — checkpoint exists and fingerprint matches (no rebuild needed). false — checkpoint absent, unreadable, or fingerprint differs (rebuild required). checkpoint_path Path to the checkpoint. current_fingerprint Input parameter. True when the operation succeeds. Calls: ifs(), read(), std::memcmp(), data(), stored_fp(), empty().

#### `size_t layerCount() const`
- Source: `include/training/ada_lora_adapter.h`:117
- Brief: Number of registered layers.
- Parameters: none

#### `std::vector< std::string > layerNames() const`
- Source: `include/training/ada_lora_adapter.h`:114
- Brief: Names of all registered layers (order unspecified).
- Parameters: none

#### `std::string loadFromFile(const std::string &path)`
- Source: `include/training/ada_lora_adapter.h`:290
- Brief: Restore adapter state from a binary checkpoint file.
- Parameters:
  - `path` (const std::string &): Input parameter.
- Return: The model fingerprint string stored in the checkpoint header (64 hex chars, may be all zeros if none was stored).
- Throws:
  - std::runtime_error: on I/O failure or format mismatch.
  - std::runtime_error: if an error occurs.
- Details: ─── loadFromFile ───────────────────────────────────────────────────────────── Replaces all current layers with the layers stored in the file. The adapter's rank_budget is updated to match the stored budget (sum of stored max_ranks). path Source file path. The model fingerprint string stored in the checkpoint header (64 hex chars, may be all zeros if none was stored). std::runtime_error on I/O failure or format mismatch. path Input parameter. Return value. std::runtime_error if an error occurs. Calls: ifs(), read(), std::memcmp(), data(), std::to_string(), fingerprint(), layerNames(), removeLayer().

#### `AdaLoRAAdapter & operator=(AdaLoRAAdapter &&) noexcept`
- Source: `include/training/ada_lora_adapter.h`:79
- Brief: n/a
- Parameters:
  - `<unnamed>` (AdaLoRAAdapter &&): n/a

#### `AdaLoRAAdapter & operator=(const AdaLoRAAdapter &)=delete`
- Source: `include/training/ada_lora_adapter.h`:77
- Brief: n/a
- Parameters:
  - `<unnamed>` (const AdaLoRAAdapter &): n/a

#### `size_t rankBudget() const`
- Source: `include/training/ada_lora_adapter.h`:244
- Brief: Global rank budget used by reallocateRanks().
- Parameters: none

#### `ReallocResult reallocateRanks()`
- Source: `include/training/ada_lora_adapter.h`:156
- Brief: Reallocate using the adapter's configured rank_budget.
- Parameters: none
- Return: Return value.
- Details: Reallocate Ranks. Return value. Calls: rankBudget().

#### `ReallocResult reallocateRanks(size_t total_budget)`
- Source: `include/training/ada_lora_adapter.h`:151
- Brief: Reallocate rank budget across layers based on current importance scores.
- Parameters:
  - `total_budget` (size_t): Input parameter.
- Return: ReallocResult describing the changes made
- Throws:
  - std::invalid_argument: if total_budget is zero
- Details: Reallocate Ranks. Distributes total_budget rank slots proportionally to each layer's normalised importance score, subject to [1, max_rank] per-layer bounds. Layers with zero importance receive rank 1 (minimum). total_budget Global rank budget to distribute (> 0) ReallocResult describing the changes made std::invalid_argument if total_budget is zero total_budget Input parameter. Return value. Implements reallocateRanks without additional internal calls.

#### `bool removeLayer(const std::string &layer_name)`
- Source: `include/training/ada_lora_adapter.h`:108
- Brief: Remove a registered layer.
- Parameters:
  - `layer_name` (const std::string &): Name of the layer.
- Return: true if the layer existed and was removed; false otherwise.
- Details: Remove Layer. true if the layer existed and was removed; false otherwise. layer_name Name of the layer. True when the operation succeeds. Implements removeLayer without additional internal calls.

#### `void saveToFile(const std::string &path, const std::string &model_fingerprint="") const`
- Source: `include/training/ada_lora_adapter.h`:275
- Brief: Persist the complete adapter state to a binary checkpoint file.
- Parameters:
  - `path` (const std::string &): Destination file path (parent directory must exist).
  - `model_fingerprint` (const std::string &): Optional SHA-256 fingerprint of the base model (64 hex chars); stored in the header for cache invalidation. Empty string stores all-zero bytes.
- Throws:
  - std::runtime_error: on I/O failure.
- Details: The file format is self-describing: 8-byte magic ("ADALORA\0") 4-byte uint32 version (currently 1) 64-byte model fingerprint (SHA-256 hex, NUL-padded) 4-byte uint32 layer count For each layer (in insertion order): 4-byte uint32 name length name bytes (UTF-8, no NUL) 8-byte uint64 in_dim, out_dim, max_rank, active_rank 4-byte float alpha, importance B matrix: (in_dim × max_rank) float32 values A matrix: (max_rank × out_dim) float32 values path Destination file path (parent directory must exist). model_fingerprint Optional SHA-256 fingerprint of the base model (64 hex chars); stored in the header for cache invalidation. Empty string stores all-zero bytes. std::runtime_error on I/O failure.

#### `void setRankBudget(size_t budget)`
- Source: `include/training/ada_lora_adapter.h`:247
- Brief: Update the global rank budget.
- Parameters:
  - `budget` (size_t): Input parameter.
- Details: Set Rank Budget. budget Input parameter. Implements setRankBudget without additional internal calls.

#### `void setWeights(const std::string &layer_name, const std::vector< float > &B, const std::vector< float > &A)`
- Source: `include/training/ada_lora_adapter.h`:205
- Brief: Overwrite B and A matrices for a layer.
- Parameters:
  - `layer_name` (const std::string &): Name of the layer.
  - `B` (const std::vector< float > &): Input parameter.
  - `A` (const std::vector< float > &): Input parameter.
- Throws:
  - std::out_of_range: if the layer does not exist
  - std::invalid_argument: if the vector sizes do not match
- Details: Set Weights. layer_name Target layer B Flat row-major B data (in_dim × max_rank floats) A Flat row-major A data (max_rank × out_dim floats) std::out_of_range if the layer does not exist std::invalid_argument if the vector sizes do not match layer_name Name of the layer. B Input parameter. A Input parameter. Implements setWeights without additional internal calls.

#### `size_t totalActiveParameterCount() const`
- Source: `include/training/ada_lora_adapter.h`:190
- Brief: Total active parameter count (sum over layers of 2 * active_rank * (in_dim + out_dim) / 2).
- Parameters: none

#### `void updateAllImportances()`
- Source: `include/training/ada_lora_adapter.h`:138
- Brief: Update importance for all registered layers.
- Parameters: none
- Details: Update All Importances. Implements updateAllImportances without additional internal calls.

#### `void updateImportance(const std::string &layer_name)`
- Source: `include/training/ada_lora_adapter.h`:133
- Brief: Recompute the importance score for a layer.
- Parameters:
  - `layer_name` (const std::string &): Name of the layer.
- Throws:
  - std::out_of_range: if the layer does not exist
- Details: Update Importance. Importance is estimated as the mean squared Frobenius norm of the rank- component contributions: sum_i( \|\|B[:,i]\|\|^2 * \|\|A[i,:]\|\|^2 ) / active_rank. This approximates the singular-value importance without a full SVD. layer_name Target layer std::out_of_range if the layer does not exist layer_name Name of the layer. Implements updateImportance without additional internal calls.

#### `~AdaLoRAAdapter()`
- Source: `include/training/ada_lora_adapter.h`:74
- Brief: n/a
- Parameters: none

### themis::training::AdaLoRAAdapter::Impl

#### `Impl(size_t default_rank, float default_alpha, size_t rank_budget)`
- Source: `src/training/ada_lora_adapter.cpp`:91
- Brief: Impl.
- Parameters:
  - `default_rank` (size_t): Input parameter.
  - `default_alpha` (float): Input parameter.
  - `rank_budget` (size_t): Input parameter.
- Return: Return value.
- Details: default_rank Input parameter. default_alpha Input parameter. rank_budget Input parameter. Return value.

#### `void addLayer(const std::string &name, size_t in_dim, size_t out_dim, size_t max_rank, float alpha)`
- Source: `src/training/ada_lora_adapter.cpp`:111
- Brief: Add Layer.
- Parameters:
  - `name` (const std::string &): Input parameter.
  - `in_dim` (size_t): Input parameter.
  - `out_dim` (size_t): Input parameter.
  - `max_rank` (size_t): Input parameter.
  - `alpha` (float): Input parameter.
- Throws:
  - std::invalid_argument: if an error occurs.
- Details: name Input parameter. in_dim Input parameter. out_dim Input parameter. max_rank Input parameter. alpha Input parameter. std::invalid_argument if an error occurs. Calls: empty(), count(), kaimingUniform(), zeros(), std::move(), push_back().

#### `std::vector< float > forward(const std::string &name, const std::vector< float > &input, size_t batch_size) const`
- Source: `src/training/ada_lora_adapter.cpp`:377
- Brief: n/a
- Parameters:
  - `name` (const std::string &): n/a
  - `input` (const std::vector< float > &): n/a
  - `batch_size` (size_t): n/a

#### `size_t getActiveRank(const std::string &name) const`
- Source: `src/training/ada_lora_adapter.cpp`:312
- Brief: n/a
- Parameters:
  - `name` (const std::string &): n/a

#### `float getImportance(const std::string &name) const`
- Source: `src/training/ada_lora_adapter.cpp`:320
- Brief: n/a
- Parameters:
  - `name` (const std::string &): n/a

#### `Layer & getLayer(const std::string &name)`
- Source: `src/training/ada_lora_adapter.cpp`:439
- Brief: Get Layer.
- Parameters:
  - `name` (const std::string &): Input parameter.
- Return: Return value.
- Throws:
  - std::out_of_range: if an error occurs.
- Details: name Input parameter. Return value. std::out_of_range if an error occurs. Calls: find(), end().

#### `const Layer & getLayerConst(const std::string &name) const`
- Source: `src/training/ada_lora_adapter.cpp`:446
- Brief: n/a
- Parameters:
  - `name` (const std::string &): n/a

#### `std::vector< AdaLoRALayerStats > getLayerStats() const`
- Source: `src/training/ada_lora_adapter.cpp`:324
- Brief: n/a
- Parameters: none

#### `size_t getMaxRank(const std::string &name) const`
- Source: `src/training/ada_lora_adapter.cpp`:316
- Brief: n/a
- Parameters:
  - `name` (const std::string &): n/a

#### `std::pair< std::vector< float >, std::vector< float > > getWeights(const std::string &name) const`
- Source: `src/training/ada_lora_adapter.cpp`:368
- Brief: n/a
- Parameters:
  - `name` (const std::string &): n/a

#### `bool hasLayer(const std::string &name) const`
- Source: `src/training/ada_lora_adapter.cpp`:158
- Brief: n/a
- Parameters:
  - `name` (const std::string &): n/a

#### `size_t layerCount() const`
- Source: `src/training/ada_lora_adapter.cpp`:166
- Brief: n/a
- Parameters: none

#### `std::vector< std::string > layerNames() const`
- Source: `src/training/ada_lora_adapter.cpp`:162
- Brief: n/a
- Parameters: none

#### `size_t rankBudget() const`
- Source: `src/training/ada_lora_adapter.cpp`:416
- Brief: n/a
- Parameters: none

#### `ReallocResult reallocateRanks(size_t total_budget)`
- Source: `src/training/ada_lora_adapter.cpp`:227
- Brief: Reallocate Ranks.
- Parameters:
  - `total_budget` (size_t): Input parameter.
- Return: Return value.
- Throws:
  - std::invalid_argument: if an error occurs.
- Details: total_budget Input parameter. Return value. std::invalid_argument if an error occurs. Calls: empty(), size(), std::min(), allocs(), at(), std::round().

#### `bool removeLayer(const std::string &name)`
- Source: `src/training/ada_lora_adapter.cpp`:146
- Brief: Remove Layer.
- Parameters:
  - `name` (const std::string &): Input parameter.
- Return: True when the operation succeeds.
- Details: name Input parameter. True when the operation succeeds. Calls: find(), end(), erase(), std::remove(), begin().

#### `void setRankBudget(size_t b)`
- Source: `src/training/ada_lora_adapter.cpp`:422
- Brief: Set Rank Budget.
- Parameters:
  - `b` (size_t): Input parameter.
- Details: b Input parameter. Implements setRankBudget without additional internal calls.

#### `void setWeights(const std::string &name, const std::vector< float > &B, const std::vector< float > &A)`
- Source: `src/training/ada_lora_adapter.cpp`:355
- Brief: Set Weights.
- Parameters:
  - `name` (const std::string &): Input parameter.
  - `B` (const std::vector< float > &): Input parameter.
  - `A` (const std::vector< float > &): Input parameter.
- Throws:
  - std::invalid_argument: if an error occurs.
- Details: name Input parameter. B Input parameter. A Input parameter. std::invalid_argument if an error occurs. Calls: getLayer(), size().

#### `size_t totalActiveParameterCount() const`
- Source: `src/training/ada_lora_adapter.cpp`:340
- Brief: n/a
- Parameters: none

#### `void updateAllImportances()`
- Source: `src/training/ada_lora_adapter.cpp`:210
- Brief: Update All Importances.
- Parameters: none
- Details: Calls: updateImportance().

#### `void updateImportance(const std::string &name)`
- Source: `src/training/ada_lora_adapter.cpp`:177
- Brief: Importance ≈ mean_i( ||B[:,i]||_F^2 * ||A[i,:]||_F^2 ) for i in active_rank.
- Parameters:
  - `name` (const std::string &): Input parameter.
- Details: name Input parameter. This is a rank-component outer-product approximation to the nuclear-norm importance used in the original AdaLoRA paper. Calls: getLayer().

### themis::training::AdaLoraTTBridge

#### `AdaLoraTTBridge(const AdaLoraTTBridge &)=delete`
- Source: `include/training/adalora_tt_bridge.h`:176
- Brief: n/a
- Parameters:
  - `<unnamed>` (const AdaLoraTTBridge &): n/a

#### `AdaLoraTTBridge(std::shared_ptr< storage::TensorNetworkStorageEngine > engine, AdaLoraTTBridgeConfig cfg={})`
- Source: `include/training/adalora_tt_bridge.h`:169
- Brief: n/a
- Parameters:
  - `engine` (std::shared_ptr< storage::TensorNetworkStorageEngine >): n/a
  - `cfg` (AdaLoraTTBridgeConfig): n/a

#### `void clearMapAdapterFn()`
- Source: `include/training/adalora_tt_bridge.h`:350
- Brief: Clear Map Adapter Fn.
- Parameters: none
- Details: Calls: lk().

#### `void clearTrainingStepFn()`
- Source: `include/training/adalora_tt_bridge.h`:385
- Brief: Clear the Phase 4 training-loop backend.
- Parameters: none
- Details: Clear Training Step Fn. Calls: lk(), trainingStepFnMutex(), trainingStepFnStorage().

#### `const AdaLoraTTBridgeConfig & config() const noexcept`
- Source: `include/training/adalora_tt_bridge.h`:326
- Brief: n/a
- Parameters: none

#### `AdaLoraTTLayerExport exportLayer(const AdaLoRAAdapter &adapter, const std::string &layer_name) const`
- Source: `include/training/adalora_tt_bridge.h`:208
- Brief: Export a single named layer.
- Parameters:
  - `adapter` (const AdaLoRAAdapter &): Source AdaLoRAAdapter.
  - `layer_name` (const std::string &): Name of the layer to export.
- Return: TT-export for that layer.
- Details: adapter Source AdaLoRAAdapter. layer_name Name of the layer to export. TT-export for that layer.

#### `AdaLoraTTExport exportToTT(const AdaLoRAAdapter &adapter, const std::string &adapter_name, const std::string &tenant="") const`
- Source: `include/training/adalora_tt_bridge.h`:197
- Brief: Export all layers of an AdaLoRAAdapter to TT-format.
- Parameters:
  - `adapter` (const AdaLoRAAdapter &): Source AdaLoRAAdapter (must have at least one layer).
  - `adapter_name` (const std::string &): Logical name for storage (used as field key).
  - `tenant` (const std::string &): ThemisDB tenant ID.
- Return: Complete AdaLoraTTExport with one entry per layer.
- Throws:
  - std::invalid_argument: if any layer has active_rank == 0 or if active_rank > config.max_tt_rank (use native storage instead).
- Details: For each registered layer, converts B/A matrices to TT-cores G₀, G₁. Applies QR sign-normalisation and optional TT-rounding per the config. adapter Source AdaLoRAAdapter (must have at least one layer). adapter_name Logical name for storage (used as field key). tenant ThemisDB tenant ID. Complete AdaLoraTTExport with one entry per layer. std::invalid_argument if any layer has active_rank == 0 or if active_rank > config.max_tt_rank (use native storage instead).

#### `std::vector< SimilarAdapter > findSimilarAdapters(const AdaLoraTTExport &query_exp, std::size_t top_k=5, const std::string &tenant="") const`
- Source: `include/training/adalora_tt_bridge.h`:306
- Brief: Find adapters in storage similar to a query adapter.
- Parameters:
  - `query_exp` (const AdaLoraTTExport &): TT-export to compare against stored adapters.
  - `top_k` (std::size_t): Maximum number of results.
  - `tenant` (const std::string &): Tenant scope (empty = all tenants).
- Details: Uses TensorFingerprintGraph::findSimilar() on the stored TT-cores. Primary use-case: FLARE live adapter selection (≤ 15 ms per call). query_exp TT-export to compare against stored adapters. top_k Maximum number of results. tenant Tenant scope (empty = all tenants).

#### `AdaLoRAAdapter importFromTT(const AdaLoraTTExport &exp) const`
- Source: `include/training/adalora_tt_bridge.h`:224
- Brief: Reconstruct an AdaLoRAAdapter from TT-format.
- Parameters:
  - `exp` (const AdaLoraTTExport &): TT-export to import from.
- Return: Reconstructed AdaLoRAAdapter with weights set to G₀/G₁ columns.
- Details: Inverse of exportToTT(). Validates orthogonality of reconstructed P/Q matrices (‖P^T·P - I‖_F < eps_orth) and logs a warning if violated. exp TT-export to import from. Reconstructed AdaLoRAAdapter with weights set to G₀/G₁ columns.

#### `std::optional< AdaLoRAAdapter > loadAdapter(const std::string &tenant, const std::string &adapter_name) const`
- Source: `include/training/adalora_tt_bridge.h`:257
- Brief: Load an adapter from storage and reconstruct as AdaLoRAAdapter.
- Parameters:
  - `tenant` (const std::string &): ThemisDB tenant.
  - `adapter_name` (const std::string &): Adapter logical name.
- Return: Reconstructed AdaLoRAAdapter, or nullopt if not found.
- Details: tenant ThemisDB tenant. adapter_name Adapter logical name. Reconstructed AdaLoRAAdapter, or nullopt if not found.

#### `bool mapAdapter(const AdaLoraTTExport &exp) const`
- Source: `include/training/adalora_tt_bridge.h`:361
- Brief: Map a stored adapter into the GGML context (Phase 3 bridge).
- Parameters:
  - `exp` (const AdaLoraTTExport &): AdaLoraTTExport to inject.
- Return: true on success; false when the bridge is not yet wired.
- Details: Delegates to the injected MapAdapterFn when set. Returns false when no fn is injected (Phase 3 not yet wired — STUB #271). exp AdaLoraTTExport to inject. true on success; false when the bridge is not yet wired.

#### `AdaLoraTTBridge & operator=(const AdaLoraTTBridge &)=delete`
- Source: `include/training/adalora_tt_bridge.h`:177
- Brief: n/a
- Parameters:
  - `<unnamed>` (const AdaLoraTTBridge &): n/a

#### `std::size_t roundAndReallocate(AdaLoraTTExport &exp, double eps) const`
- Source: `include/training/adalora_tt_bridge.h`:284
- Brief: Apply TT-rounding (globally optimal rank cut) to an exported adapter.
- Parameters:
  - `exp` (AdaLoraTTExport &): TT-export to round (modified in-place).
  - `eps` (double): Target Frobenius relative error (e.g. 0.01 = 1%).
- Return: Total active rank after rounding.
- Details: TT-rounding minimises ‖ΔW - ΔW_approx‖_F for a given ε budget, making it globally optimal vs. AdaLoRA's greedy singular-value pruning. After rounding, the new active_rank per layer is set to the number of kept singular values and can be fed back to AdaLoRAAdapter::reallocateRanks(). exp TT-export to round (modified in-place). eps Target Frobenius relative error (e.g. 0.01 = 1%). Total active rank after rounding. STUB/SIMULATION NOTE: Purpose: Calls TensorTrainDecomposer::round() per layer. Activation: Used only post-training (not during backprop). Production Delta: Full integration with AdaLoRA training loop requires differentiable TT-layer (ggml-autograd or libtorch custom op). Removal Plan: Not removed; training-loop integration added in Phase 4.

#### `void setMapAdapterFn(MapAdapterFn fn)`
- Source: `include/training/adalora_tt_bridge.h`:349
- Brief: Set the Phase 3 GgmlTensorBridge::mapAdapter() bridge.
- Parameters:
  - `fn` (MapAdapterFn): Input parameter.
- Details: Set Map Adapter Fn. fn Function called by mapAdapter() to inject TT-cores into llama.cpp. fn Input parameter. Calls: lk(), std::move().

#### `void setTrainingStepFn(TrainingStepFn fn)`
- Source: `include/training/adalora_tt_bridge.h`:383
- Brief: Inject the Phase 4 training-loop backend. Thread-safe.
- Parameters:
  - `fn` (TrainingStepFn): Input parameter.
- Details: Set Training Step Fn. fn Input parameter. Calls: lk(), trainingStepFnMutex(), trainingStepFnStorage(), std::move().

#### `BridgeStats stats() const noexcept`
- Source: `include/training/adalora_tt_bridge.h`:324
- Brief: n/a
- Parameters: none

#### `bool store(const AdaLoraTTExport &exp)`
- Source: `include/training/adalora_tt_bridge.h`:240
- Brief: Store an AdaLoraTTExport in TensorNetworkStorageEngine.
- Parameters:
  - `exp` (const AdaLoraTTExport &): Input parameter.
- Return: true on success.
- Details: Store. Each layer is stored under key: __lora_adapters__:<tenant>:<adapter_name>:<layer_name>:G<0\|1> If config.auto_deduplicate is true, inserts into TensorFingerprintGraph. true on success. exp Input parameter. True when the operation succeeds. Calls: empty(), lk(), cacheKey(), fg_lk(), insert().

#### `bool storeAdapter(const AdaLoraTTExport &exp)`
- Source: `include/training/adalora_tt_bridge.h`:248
- Brief: Backward-compatible alias for store().
- Parameters:
  - `exp` (const AdaLoraTTExport &): n/a
- Details: Forwards to store(exp) to preserve existing call sites and tests that still use the historical method name.

#### `~AdaLoraTTBridge()`
- Source: `include/training/adalora_tt_bridge.h`:173
- Brief: n/a
- Parameters: none

### themis::training::AdaLoraTTExport

#### `std::size_t totalActiveRank() const noexcept`
- Source: `include/training/adalora_tt_bridge.h`:59
- Brief: Total active TT-rank (sum across layers).
- Parameters: none

#### `std::size_t totalParameters() const noexcept`
- Source: `include/training/adalora_tt_bridge.h`:68
- Brief: Total number of float32 parameters stored (TT-cores only, not base weights).
- Parameters: none

### themis::training::AdapterMergeFailsafe

#### `AdapterMergeFaultResult handle(const AdapterMergeFaultContext &ctx, TrainingIncidentEmitter *emitter) const noexcept`
- Source: `include/training/training_failsafe.h`:239
- Brief: Evaluate a merge fault and return the recovery decision.
- Parameters:
  - `ctx` (const AdapterMergeFaultContext &): Merge fault context.
  - `emitter` (TrainingIncidentEmitter *): Optional incident emitter.
- Return: Resolved AdapterMergeFaultResult.
- Details: ctx Merge fault context. emitter Optional incident emitter. Resolved AdapterMergeFaultResult.

### themis::training::AdaptiveRule

#### `AdaptiveRule()=default`
- Source: `include/training/lora_data_selection.h`:392
- Brief: n/a
- Parameters: none

### themis::training::AsyncTrainingResult

#### `AsyncTrainingResult()=default`
- Source: `include/training/training_interfaces.h`:174
- Brief: n/a
- Parameters: none

### themis::training::AutoLabelConfig

#### `AutoLabelConfig()=default`
- Source: `include/training/auto_labeler.h`:118
- Brief: n/a
- Parameters: none

### themis::training::CacheStats

#### `CacheStats()=default`
- Source: `include/training/training_interfaces.h`:91
- Brief: n/a
- Parameters: none

### themis::training::CalibratedThreshold

#### `CalibratedThreshold()=default`
- Source: `include/training/training_pipeline.h`:111
- Brief: n/a
- Parameters: none

### themis::training::CalibrationDataset

#### `CalibrationDataset()=default`
- Source: `include/training/training_interfaces.h`:140
- Brief: n/a
- Parameters: none

### themis::training::CalibrationResult

#### `CalibrationResult()=default`
- Source: `include/training/training_pipeline.h`:123
- Brief: n/a
- Parameters: none

### themis::training::CalibrationSample

#### `CalibrationSample()=default`
- Source: `include/training/training_interfaces.h`:133
- Brief: n/a
- Parameters: none

### themis::training::CalibratorOutput

#### `CalibratorOutput()=default`
- Source: `include/training/training_interfaces.h`:151
- Brief: n/a
- Parameters: none

### themis::training::CheckpointDescriptor

#### `CheckpointDescriptor()=default`
- Source: `include/training/training_interfaces.h`:53
- Brief: n/a
- Parameters: none

### themis::training::CheckpointException

#### `CheckpointException(const std::string &message, TrainingErrorCode code=TrainingErrorCode::CHECKPOINT_DIR_INVALID, bool recoverable=false, const std::string &context="")`
- Source: `include/training/training_exceptions.h`:121
- Brief: n/a
- Parameters:
  - `message` (const std::string &): n/a
  - `code` (TrainingErrorCode): n/a
  - `recoverable` (bool): n/a
  - `context` (const std::string &): n/a

### themis::training::CheckpointFaultHandler

#### `CheckpointFaultResult handle(const CheckpointFaultContext &ctx, TrainingIncidentEmitter *emitter) const noexcept`
- Source: `include/training/training_failsafe.h`:133
- Brief: Evaluate a checkpoint fault and return the recovery decision.
- Parameters:
  - `ctx` (const CheckpointFaultContext &): Fault context (path, code, fallback availability).
  - `emitter` (TrainingIncidentEmitter *): Optional incident emitter; pass nullptr to suppress output.
- Return: Resolved CheckpointFaultResult.
- Details: ctx Fault context (path, code, fallback availability). emitter Optional incident emitter; pass nullptr to suppress output. Resolved CheckpointFaultResult.

#### `bool isCorruptionFault(TrainingErrorCode code) noexcept`
- Source: `include/training/training_failsafe.h`:144
- Brief: Classify the fault code into broad fault families.
- Parameters:
  - `code` (TrainingErrorCode): Error code to classify.
- Return: true if the fault indicates data corruption or integrity failure.
- Details: code Error code to classify. true if the fault indicates data corruption or integrity failure.

#### `bool isDiskExhaustionFault(TrainingErrorCode code) noexcept`
- Source: `include/training/training_failsafe.h`:152
- Brief: Return true if the fault is a disk/resource exhaustion fault.
- Parameters:
  - `code` (TrainingErrorCode): Error code to classify.
- Return: true for disk space or I/O resource exhaustion.
- Details: code Error code to classify. true for disk space or I/O resource exhaustion.

#### `bool isTransientIoFault(TrainingErrorCode code) noexcept`
- Source: `include/training/training_failsafe.h`:160
- Brief: Return true if the fault is a transient I/O or timeout fault.
- Parameters:
  - `code` (TrainingErrorCode): Error code to classify.
- Return: true for I/O read/write errors and timeouts.
- Details: code Error code to classify. true for I/O read/write errors and timeouts.

### themis::training::CheckpointManagerConfig

#### `CheckpointManagerConfig()=default`
- Source: `include/training/lora_checkpoint_manager.h`:64
- Brief: n/a
- Parameters: none

### themis::training::CheckpointManifestEntry

#### `CheckpointManifestEntry()=default`
- Source: `include/training/lora_checkpoint_manager.h`:43
- Brief: n/a
- Parameters: none

### themis::training::CitationExtractor

#### `CitationExtractor(const ModalityParserConfig &config)`
- Source: `include/training/modality_parser.h`:173
- Brief: n/a
- Parameters:
  - `config` (const ModalityParserConfig &): n/a

#### `std::vector< TrainingSample > extract(const std::string &text, const std::string &document_id) const`
- Source: `include/training/modality_parser.h`:185
- Brief: Extract legal citations from text.
- Parameters:
  - `text` (const std::string &): Raw document text.
  - `document_id` (const std::string &): Identifier propagated to sample.source_id.
- Return: Vector of CITATION-typed TrainingSample records.
- Details: text Raw document text. document_id Identifier propagated to sample.source_id. Vector of CITATION-typed TrainingSample records. Applies shared prompt-safety policy per citation text. Blocked payloads are rejected (fail-closed); allowed payloads are emitted with control-token redaction applied.

### themis::training::ConfidenceCalibrator

#### `ConfidenceCalibrator()=default`
- Source: `include/training/training_pipeline.h`:149
- Brief: n/a
- Parameters: none

#### `void addSample(const std::string &category, float confidence, bool model_correct)`
- Source: `include/training/training_pipeline.h`:157
- Brief: Record a validation sample for calibration.
- Parameters:
  - `category` (const std::string &): Input parameter.
  - `confidence` (float): Input parameter.
  - `model_correct` (bool): Input parameter.
- Details: ============================================================================ ConfidenceCalibrator implementation (Phase 3 – isotonic regression / PAV) ============================================================================ category Legal category of the sample. confidence Model confidence score in [0, 1]. model_correct Whether the model produced the correct label. category Input parameter. confidence Input parameter. model_correct Input parameter. Calls: push_back().

#### `CalibrationResult calibrate() const`
- Source: `include/training/training_pipeline.h`:167
- Brief: Compute calibrated thresholds using isotonic regression (PAV).
- Parameters: none
- Return: Calibration result with per-category thresholds.
- Details: For each category, fits a monotone non-decreasing step function to the (confidence → correct) pairs and selects the threshold that maximises F1. Calibration result with per-category thresholds.

#### `void reset()`
- Source: `include/training/training_pipeline.h`:172
- Brief: Reset all accumulated samples.
- Parameters: none
- Details: Reset the modification detection flag. Calls: clear().

#### `size_t sampleCount() const`
- Source: `include/training/training_pipeline.h`:177
- Brief: Number of samples accumulated so far.
- Parameters: none

### themis::training::DataQualityReport

#### `DataQualityReport()=default`
- Source: `include/training/training_pipeline.h`:87
- Brief: n/a
- Parameters: none

### themis::training::DataSample

#### `DataSample()=default`
- Source: `include/training/lora_data_selection.h`:137
- Brief: n/a
- Parameters: none

#### `DataSample(std::string id_, std::string text_)`
- Source: `include/training/lora_data_selection.h`:138
- Brief: n/a
- Parameters:
  - `id_` (std::string): n/a
  - `text_` (std::string): n/a

### themis::training::DataSelectionMetrics

#### `DataSelectionMetrics()=default`
- Source: `include/training/lora_data_selection.h`:233
- Brief: n/a
- Parameters: none

### themis::training::DataSelectionPipeline

#### `DataSelectionPipeline(const DataSelectionPipeline &)=delete`
- Source: `include/training/lora_data_selection.h`:288
- Brief: n/a
- Parameters:
  - `<unnamed>` (const DataSelectionPipeline &): n/a

#### `DataSelectionPipeline(const LoRADataSelectionConfig &config)`
- Source: `include/training/lora_data_selection.h`:285
- Brief: Construct pipeline with the given configuration.
- Parameters:
  - `config` (const LoRADataSelectionConfig &): n/a

#### `std::vector< DataSample > clusterAndSample(const std::vector< DataSample > &samples, size_t k=0) const`
- Source: `include/training/lora_data_selection.h`:326
- Brief: Run only Stage 3: cluster-based diversity sampling.
- Parameters:
  - `samples` (const std::vector< DataSample > &): Samples to cluster.
  - `k` (size_t): Number of clusters (computed from config if 0).
- Return: Centroid-nearest samples covering diverse clusters.
- Details: samples Samples to cluster. k Number of clusters (computed from config if 0). Centroid-nearest samples covering diverse clusters.

#### `DataSelectionMetrics computeMetrics(const DataSelectionResult &result)`
- Source: `include/training/lora_data_selection.h`:368
- Brief: Derive a DataSelectionMetrics snapshot from a completed pipeline result.
- Parameters:
  - `result` (const DataSelectionResult &): Input parameter.
- Return: Populated metrics snapshot (all fields 0.0 for empty results).
- Details: Compute Metrics. Computes per-stage rejection rates and average quality/difficulty/ diversity scores from the returned samples. Useful for feeding the result directly into SelfImprovementModule::applyAdaptiveRules() and SelfImprovementModule::needsRollback(). result The result returned by a previous run() call. Populated metrics snapshot (all fields 0.0 for empty results). result Input parameter. Return value. Calls: DataSelectionMetrics(), empty(), ss(), reserve(), size(), std::isalpha(), std::tolower(), std::isdigit().

#### `std::vector< DataSample > deduplicate(const std::vector< DataSample > &samples) const`
- Source: `include/training/lora_data_selection.h`:317
- Brief: Run only Stage 2: MinHash deduplication.
- Parameters:
  - `samples` (const std::vector< DataSample > &): n/a
- Return: Samples with duplicates removed.
- Details: Samples with duplicates removed.

#### `std::vector< DataSample > filterByQuality(const std::vector< DataSample > &samples) const`
- Source: `include/training/lora_data_selection.h`:310
- Brief: Run only Stage 1: quality filtering.
- Parameters:
  - `samples` (const std::vector< DataSample > &): n/a
- Return: Samples that pass all quality filters.
- Details: Applies the shared prompt-safety policy before token/language/toxicity checks. Samples matching blocked prompt-injection patterns are rejected (fail-closed). Allowed samples continue with sanitized control-token redaction applied to their text. Samples that pass all quality filters.

#### `const LoRADataSelectionConfig & getConfig() const`
- Source: `include/training/lora_data_selection.h`:354
- Brief: Get the current pipeline configuration.
- Parameters: none

#### `DataSelectionPipeline & operator=(const DataSelectionPipeline &)=delete`
- Source: `include/training/lora_data_selection.h`:289
- Brief: n/a
- Parameters:
  - `<unnamed>` (const DataSelectionPipeline &): n/a

#### `DataSelectionResult run(const std::vector< DataSample > &input_samples, SelectionProgressCallback callback=nullptr)`
- Source: `include/training/lora_data_selection.h`:297
- Brief: Execute all five pipeline stages on input_samples.
- Parameters:
  - `input_samples` (const std::vector< DataSample > &): Input parameter.
  - `callback` (SelectionProgressCallback): Input parameter.
- Return: Selection result including selected samples and audit entry.
- Details: Run. input_samples Raw samples loaded from the training collection. callback Optional per-stage progress callback. Selection result including selected samples and audit entry. input_samples Input parameter. callback Input parameter. Return value. Calls: std::move(), getConfig(), empty(), detail::appendAuditJSONL(), toJSONL().

#### `void scoreQualityAndDifficulty(std::vector< DataSample > &samples) const`
- Source: `include/training/lora_data_selection.h`:334
- Brief: Run only Stage 4: quality/difficulty scoring. Modifies quality_score and difficulty_score in-place.
- Parameters:
  - `samples` (std::vector< DataSample > &): n/a

#### `void setConfig(const LoRADataSelectionConfig &config)`
- Source: `include/training/lora_data_selection.h`:349
- Brief: Update the pipeline configuration (live reload support).
- Parameters:
  - `config` (const LoRADataSelectionConfig &): Input parameter.
- Details: Set Config. config Input parameter. Implements setConfig without additional internal calls.

#### `std::vector< DataSample > stratifiedSample(const std::vector< DataSample > &scored_samples, size_t target=0) const`
- Source: `include/training/lora_data_selection.h`:342
- Brief: Run only Stage 5: curriculum stratified sampling.
- Parameters:
  - `scored_samples` (const std::vector< DataSample > &): Samples with difficulty_score populated.
  - `target` (size_t): Total samples to return (0 = use config).
- Return: Stratified subset (easy + medium + hard).
- Details: scored_samples Samples with difficulty_score populated. target Total samples to return (0 = use config). Stratified subset (easy + medium + hard).

#### `~DataSelectionPipeline()`
- Source: `include/training/lora_data_selection.h`:286
- Brief: n/a
- Parameters: none

### themis::training::DataSelectionPipeline::Impl

#### `Impl(const LoRADataSelectionConfig &config)`
- Source: `src/training/lora_data_selection.cpp`:404
- Brief: n/a
- Parameters:
  - `config` (const LoRADataSelectionConfig &): n/a

#### `std::vector< DataSample > clusterAndSample(const std::vector< DataSample > &samples, size_t k) const`
- Source: `src/training/lora_data_selection.cpp`:506
- Brief: n/a
- Parameters:
  - `samples` (const std::vector< DataSample > &): n/a
  - `k` (size_t): n/a

#### `std::vector< DataSample > deduplicate(const std::vector< DataSample > &samples) const`
- Source: `src/training/lora_data_selection.cpp`:466
- Brief: n/a
- Parameters:
  - `samples` (const std::vector< DataSample > &): n/a

#### `std::vector< DataSample > filterByQuality(const std::vector< DataSample > &samples) const`
- Source: `src/training/lora_data_selection.cpp`:407
- Brief: n/a
- Parameters:
  - `samples` (const std::vector< DataSample > &): n/a

#### `const LoRADataSelectionConfig & getConfig() const`
- Source: `src/training/lora_data_selection.cpp`:798
- Brief: n/a
- Parameters: none

#### `DataSelectionResult run(const std::vector< DataSample > &input, SelectionProgressCallback cb)`
- Source: `src/training/lora_data_selection.cpp`:722
- Brief: - Full pipeline run ----------------------------------------------
- Parameters:
  - `input` (const std::vector< DataSample > &): Input parameter.
  - `cb` (SelectionProgressCallback): Input parameter.
- Return: Return value.
- Details: input Input parameter. cb Input parameter. Return value. Calls: DataSelectionResult(), std::chrono::steady_clock::now(), filterByQuality(), cb(), size(), deduplicate(), clusterAndSample(), scoreQualityAndDifficulty().

#### `void scoreQualityAndDifficulty(std::vector< DataSample > &samples) const`
- Source: `src/training/lora_data_selection.cpp`:628
- Brief: n/a
- Parameters:
  - `samples` (std::vector< DataSample > &): n/a

#### `void setConfig(const LoRADataSelectionConfig &config)`
- Source: `src/training/lora_data_selection.cpp`:797
- Brief: Set Config.
- Parameters:
  - `config` (const LoRADataSelectionConfig &): Input parameter.
- Details: config Input parameter. Implements setConfig without additional internal calls.

#### `std::vector< DataSample > stratifiedSample(const std::vector< DataSample > &scored, size_t target) const`
- Source: `src/training/lora_data_selection.cpp`:655
- Brief: n/a
- Parameters:
  - `scored` (const std::vector< DataSample > &): n/a
  - `target` (size_t): n/a

### themis::training::DataSelectionResult

#### `DataSelectionResult()=default`
- Source: `include/training/lora_data_selection.h`:192
- Brief: n/a
- Parameters: none

### themis::training::DatabaseDomainAutoLabeler

#### `DatabaseDomainAutoLabeler(double sensitivity_ms=10.0)`
- Source: `include/training/database_domain_auto_labeler.h`:68
- Brief: Construct with optional sensitivity parameter.
- Parameters:
  - `sensitivity_ms` (double): Latency delta (ms) that maps to sigmoid inflection point (confidence = 0.73). Default: 10.0 ms.
- Details: sensitivity_ms Latency delta (ms) that maps to sigmoid inflection point (confidence = 0.73). Default: 10.0 ms.

#### `LabeledDbSample buildSample(const std::string &query, const std::string &plan_json, double delta_p99_ms, const std::string &source) const`
- Source: `include/training/database_domain_auto_labeler.h`:152
- Brief: Internal: build a LabeledDbSample from raw components.
- Parameters:
  - `query` (const std::string &): n/a
  - `plan_json` (const std::string &): n/a
  - `delta_p99_ms` (double): n/a
  - `source` (const std::string &): n/a

#### `double computeConfidence(double delta_p99_ms) const`
- Source: `include/training/database_domain_auto_labeler.h`:123
- Brief: Map a latency delta to a confidence score in [0.0, 1.0].
- Parameters:
  - `delta_p99_ms` (double): Measured latency improvement (sign is ignored).
- Return: Confidence in [0.5, 1.0). Returns exactly 0.5 when delta == 0.
- Details: confidence = sigmoid(\|delta_p99_ms\| / sensitivity_ms) delta_p99_ms Measured latency improvement (sign is ignored). Confidence in [0.5, 1.0). Returns exactly 0.5 when delta == 0.

#### `std::string exportToJsonl(const std::vector< LabeledDbSample > &samples)`
- Source: `include/training/database_domain_auto_labeler.h`:145
- Brief: Serialize a batch of labeled samples to a JSONL string.
- Parameters:
  - `samples` (const std::vector< LabeledDbSample > &): Input parameter.
- Return: JSONL string; empty string when samples is empty.
- Details: static Each sample is emitted as one compact JSON object on its own line: {"query":"SELECT…","explain_plan":"…","latency_delta_ms":-42.5} The output is suitable for appending to a .jsonl log file or piping to a downstream ingestion tool. samples Samples to serialize (may be empty). JSONL string; empty string when samples is empty. samples Input parameter. Return value.

#### `LabeledDbSample labelFromBaoDecision(const std::string &query, const std::string &bao_plan_json, double delta_p99_ms) const`
- Source: `include/training/database_domain_auto_labeler.h`:80
- Brief: Create a labeled sample from a BaoOptimizer decision log entry.
- Parameters:
  - `query` (const std::string &): SQL query string.
  - `bao_plan_json` (const std::string &): JSON-serialized BaoOptimizer plan.
  - `delta_p99_ms` (double): Latency improvement: negative = query got faster.
- Return: LabeledDbSample with source = "bao_log".
- Details: query SQL query string. bao_plan_json JSON-serialized BaoOptimizer plan. delta_p99_ms Latency improvement: negative = query got faster. LabeledDbSample with source = "bao_log".

#### `LabeledDbSample labelFromDBAFeedback(const FeedbackEntry &entry) const`
- Source: `include/training/database_domain_auto_labeler.h`:94
- Brief: Create a labeled sample from a DBA feedback entry.
- Parameters:
  - `entry` (const FeedbackEntry &): FeedbackEntry from the DBA or system operator.
- Return: LabeledDbSample with source = "dba_feedback".
- Details: Negative (is_positive=false) feedback receives confidence ≥ 0.9, reflecting high signal strength. entry FeedbackEntry from the DBA or system operator. LabeledDbSample with source = "dba_feedback".

#### `std::vector< LabeledDbSample > labelFromLogFile(const std::string &log_path, size_t max_samples=50000, double min_confidence=0.0) const`
- Source: `include/training/database_domain_auto_labeler.h`:108
- Brief: Batch-label samples from a query log file.
- Parameters:
  - `log_path` (const std::string &): Path to the query log file.
  - `max_samples` (size_t): Maximum number of samples to return (0 = unlimited).
  - `min_confidence` (double): Filter: samples below this threshold are discarded.
- Return: Vector of labeled samples (may be empty if file is missing / empty).
- Details: Each line in the file must be a JSON object with at least the fields "query", "plan", and "delta_p99_ms". Lines that cannot be parsed are silently skipped and counted in the returned stats. log_path Path to the query log file. max_samples Maximum number of samples to return (0 = unlimited). min_confidence Filter: samples below this threshold are discarded. Vector of labeled samples (may be empty if file is missing / empty).

#### `double sensitivityMs() const`
- Source: `include/training/database_domain_auto_labeler.h`:127
- Brief: n/a
- Parameters: none

### themis::training::DatasetException

#### `DatasetException(const std::string &message, TrainingErrorCode code=TrainingErrorCode::DATASET_PATH_INVALID, bool recoverable=false, const std::string &context="")`
- Source: `include/training/training_exceptions.h`:236
- Brief: n/a
- Parameters:
  - `message` (const std::string &): n/a
  - `code` (TrainingErrorCode): n/a
  - `recoverable` (bool): n/a
  - `context` (const std::string &): n/a

### themis::training::DeployResult

#### `DeployResult()=default`
- Source: `include/training/adapter_serving.h`:83
- Brief: n/a
- Parameters: none

#### `DeployResult fail(const std::string &reason)`
- Source: `include/training/adapter_serving.h`:95
- Brief: Convenience: create a failed result.
- Parameters:
  - `reason` (const std::string &): n/a

#### `DeployResult ok(const std::string &version, float split)`
- Source: `include/training/adapter_serving.h`:86
- Brief: Convenience: create a successful result.
- Parameters:
  - `version` (const std::string &): n/a
  - `split` (float): n/a

### themis::training::DriftReport

#### `DriftReport()=default`
- Source: `include/training/training_pipeline.h`:99
- Brief: n/a
- Parameters: none

### themis::training::EnrichmentCache

#### `EnrichmentCache(size_t max_entries=100, int ttl_seconds=3600)`
- Source: `tests/training/test_enrichment_cache.cpp`:45
- Brief: n/a
- Parameters:
  - `max_entries` (size_t): n/a
  - `ttl_seconds` (int): n/a

#### `void clear()`
- Source: `tests/training/test_enrichment_cache.cpp`:137
- Brief: n/a
- Parameters: none

#### `bool contains(const std::string &key) const`
- Source: `tests/training/test_enrichment_cache.cpp`:132
- Brief: n/a
- Parameters:
  - `key` (const std::string &): n/a

#### `void evictOne()`
- Source: `tests/training/test_enrichment_cache.cpp`:161
- Brief: n/a
- Parameters: none

#### `std::vector< float > get(const std::string &key, bool *hit=nullptr)`
- Source: `tests/training/test_enrichment_cache.cpp`:67
- Brief: n/a
- Parameters:
  - `key` (const std::string &): n/a
  - `hit` (bool *): n/a

#### `Stats getStats() const`
- Source: `tests/training/test_enrichment_cache.cpp`:155
- Brief: n/a
- Parameters: none

#### `bool invalidate(const std::string &key)`
- Source: `tests/training/test_enrichment_cache.cpp`:102
- Brief: n/a
- Parameters:
  - `key` (const std::string &): n/a

#### `void invalidateAll()`
- Source: `tests/training/test_enrichment_cache.cpp`:113
- Brief: n/a
- Parameters: none

#### `void markStale(const std::string &key)`
- Source: `tests/training/test_enrichment_cache.cpp`:119
- Brief: n/a
- Parameters:
  - `key` (const std::string &): n/a

#### `void put(const std::string &key, const std::vector< float > &data)`
- Source: `tests/training/test_enrichment_cache.cpp`:48
- Brief: n/a
- Parameters:
  - `key` (const std::string &): n/a
  - `data` (const std::vector< float > &): n/a

#### `size_t size() const`
- Source: `tests/training/test_enrichment_cache.cpp`:127
- Brief: n/a
- Parameters: none

### themis::training::EnrichmentCache::Stats

#### `double hitRate() const`
- Source: `tests/training/test_enrichment_cache.cpp`:149
- Brief: n/a
- Parameters: none

### themis::training::EnrichmentCacheConfig

#### `EnrichmentCacheConfig()=default`
- Source: `include/training/knowledge_graph_enricher.h`:71
- Brief: n/a
- Parameters: none

### themis::training::EnrichmentCacheStats

#### `EnrichmentCacheStats()=default`
- Source: `include/training/knowledge_graph_enricher.h`:83
- Brief: n/a
- Parameters: none

### themis::training::EnrichmentConfig

#### `EnrichmentConfig()=default`
- Source: `include/training/knowledge_graph_enricher.h`:102
- Brief: n/a
- Parameters: none

### themis::training::EnrichmentException

#### `EnrichmentException(const std::string &message, TrainingErrorCode code=TrainingErrorCode::ENRICHMENT_GRAPH_NOT_INITIALIZED, bool recoverable=true, const std::string &context="")`
- Source: `include/training/training_exceptions.h`:171
- Brief: n/a
- Parameters:
  - `message` (const std::string &): n/a
  - `code` (TrainingErrorCode): n/a
  - `recoverable` (bool): n/a
  - `context` (const std::string &): n/a

### themis::training::EnrichmentGapHandler

#### `EnrichmentGapSummary evaluate(const EnrichmentGapContext &ctx, TrainingIncidentEmitter *emitter) const noexcept`
- Source: `include/training/training_failsafe.h`:299
- Brief: Evaluate enrichment gap coverage and emit diagnostics.
- Parameters:
  - `ctx` (const EnrichmentGapContext &): Enrichment gap context (counts + threshold).
  - `emitter` (TrainingIncidentEmitter *): Optional incident emitter.
- Return: EnrichmentGapSummary with coverage ratio and threshold flag.
- Details: ctx Enrichment gap context (counts + threshold). emitter Optional incident emitter. EnrichmentGapSummary with coverage ratio and threshold flag.

### themis::training::EnrichmentLRUCache

#### `EnrichmentLRUCache(size_t capacity)`
- Source: `src/training/knowledge_graph_enricher.cpp`:90
- Brief: n/a
- Parameters:
  - `capacity` (size_t): n/a

#### `void evictAll()`
- Source: `src/training/knowledge_graph_enricher.cpp`:143
- Brief: Evict all entries (e.
- Parameters: none
- Details: g., on graph-version change). Calls: lock(), size(), clear().

#### `bool get(const Key &key, Value &out)`
- Source: `src/training/knowledge_graph_enricher.cpp`:99
- Brief: Attempt to retrieve a cached result.
- Parameters:
  - `key` (const Key &): Input parameter.
  - `out` (Value &): Input/output parameter.
- Return: True when the operation succeeds.
- Details: key Input parameter. out Input/output parameter. True when the operation succeeds. Returns true on hit. Calls: lock(), find(), end(), splice(), begin().

#### `void put(const Key &key, Value value)`
- Source: `src/training/knowledge_graph_enricher.cpp`:119
- Brief: Insert or update a cache entry.
- Parameters:
  - `key` (const Key &): Input parameter.
  - `value` (Value): Input parameter.
- Details: key Input parameter. value Input parameter. Calls: lock(), find(), end(), std::move(), splice(), begin(), size(), erase().

#### `void resetStats()`
- Source: `src/training/knowledge_graph_enricher.cpp`:166
- Brief: Reset Stats.
- Parameters: none
- Details: Calls: lock().

#### `EnrichmentCacheStats stats() const`
- Source: `src/training/knowledge_graph_enricher.cpp`:150
- Brief: n/a
- Parameters: none

### themis::training::EnrichmentResult

#### `EnrichmentResult()=default`
- Source: `include/training/training_interfaces.h`:121
- Brief: n/a
- Parameters: none

### themis::training::EnrichmentStats

#### `EnrichmentStats()=default`
- Source: `include/training/knowledge_graph_enricher.h`:53
- Brief: n/a
- Parameters: none

### themis::training::EntityRef

#### `EntityRef()=default`
- Source: `include/training/training_interfaces.h`:99
- Brief: n/a
- Parameters: none

#### `EntityRef(const std::string &key, const std::string &schema_version="")`
- Source: `include/training/training_interfaces.h`:100
- Brief: n/a
- Parameters:
  - `key` (const std::string &): n/a
  - `schema_version` (const std::string &): n/a

### themis::training::EpochMetrics

#### `EpochMetrics()=default`
- Source: `include/training/incremental_lora_trainer.h`:88
- Brief: n/a
- Parameters: none

### themis::training::GraphContext

#### `GraphContext()=default`
- Source: `include/training/knowledge_graph_enricher.h`:40
- Brief: n/a
- Parameters: none

### themis::training::HyperparamResult

#### `HyperparamResult()=default`
- Source: `include/training/training_pipeline.h`:257
- Brief: n/a
- Parameters: none

### themis::training::HyperparamSearchConfig

#### `HyperparamSearchConfig()=default`
- Source: `include/training/training_pipeline.h`:224
- Brief: n/a
- Parameters: none

### themis::training::HyperparamTrialResult

#### `HyperparamTrialResult()=default`
- Source: `include/training/training_pipeline.h`:236
- Brief: n/a
- Parameters: none

### themis::training::IConfidenceCalibrator

#### `void applyThresholds(const ThresholdMap &thresholds)=0`
- Source: `include/training/training_interfaces.h`:385
- Brief: Replace the active threshold map.
- Parameters:
  - `thresholds` (const ThresholdMap &): New per-category thresholds to apply.
- Details: thresholds New per-category thresholds to apply.

#### `std::future< CalibratorOutput > calibrate(const CalibrationDataset &dataset)=0`
- Source: `include/training/training_interfaces.h`:373
- Brief: Asynchronously compute calibrated thresholds.
- Parameters:
  - `dataset` (const CalibrationDataset &): Labelled (confidence, correct) pairs.
- Return: Future resolving to the calibration output.
- Details: dataset Labelled (confidence, correct) pairs. Future resolving to the calibration output.

#### `const ThresholdMap & currentThresholds() const =0`
- Source: `include/training/training_interfaces.h`:379
- Brief: Return the currently active threshold map.
- Parameters: none

#### `void resetToDefaults()=0`
- Source: `include/training/training_interfaces.h`:390
- Brief: Reset all thresholds to their factory defaults (0.5 per category).
- Parameters: none

#### `~IConfidenceCalibrator()=default`
- Source: `include/training/training_interfaces.h`:366
- Brief: n/a
- Parameters: none

### themis::training::IKGEnrichmentInterface

#### `CacheStats cacheStats() const =0`
- Source: `include/training/training_interfaces.h`:346
- Brief: Return hit/miss/eviction counters.
- Parameters: none

#### `void clearCache()=0`
- Source: `include/training/training_interfaces.h`:341
- Brief: Evict all cached enrichment results.
- Parameters: none

#### `EnrichmentResult enrich(const EntityRef &entity)=0`
- Source: `include/training/training_interfaces.h`:330
- Brief: Enrich an entity, serving from cache when available.
- Parameters:
  - `entity` (const EntityRef &): Entity reference identifying the enrichment query.
- Return: Enrichment result; result.cache_hit is true on cache hit.
- Details: entity Entity reference identifying the enrichment query. Enrichment result; result.cache_hit is true on cache hit.

#### `void invalidateCache(const EntityRef &entity)=0`
- Source: `include/training/training_interfaces.h`:336
- Brief: Invalidate the cached result for a specific entity.
- Parameters:
  - `entity` (const EntityRef &): Entity whose cache entry should be evicted.
- Details: entity Entity whose cache entry should be evicted.

#### `~IKGEnrichmentInterface()=default`
- Source: `include/training/training_interfaces.h`:323
- Brief: n/a
- Parameters: none

### themis::training::ILLMRouter

#### `std::string activeVersion() const =0`
- Source: `include/training/adapter_serving.h`:64
- Brief: Return the version identifier that currently receives 100% of traffic, or an empty string if no version is fully active.
- Parameters: none

#### `bool isAvailable() const =0`
- Source: `include/training/adapter_serving.h`:58
- Brief: Whether the router is reachable and ready to accept weight updates.
- Parameters: none

#### `bool setAdapterWeight(const std::string &version, float weight)=0`
- Source: `include/training/adapter_serving.h`:53
- Brief: Update the traffic weight for the named adapter version.
- Parameters:
  - `version` (const std::string &): Adapter version identifier (e.g., "legal_v1.1").
  - `weight` (float): Desired traffic fraction in [0.0, 1.0].
- Return: true on success; false if the version is unknown to the router (the caller may retry after loading the adapter).
- Details: Sets the fraction of incoming inference requests that should be routed to version. Implementations must normalise weights across all registered versions so that the total sums to 1.0. version Adapter version identifier (e.g., "legal_v1.1"). weight Desired traffic fraction in [0.0, 1.0]. true on success; false if the version is unknown to the router (the caller may retry after loading the adapter).

#### `~ILLMRouter()`
- Source: `include/training/adapter_serving.h`:39
- Brief: n/a
- Parameters: none

### themis::training::ILineageQueryAPI

#### `std::vector< SampleRef > getProvenance(const std::string &model_id) const =0`
- Source: `include/training/training_interfaces.h`:454
- Brief: Return all training samples that contributed to a deployed model.
- Parameters:
  - `model_id` (const std::string &): Adapter version or model identifier.
- Return: Sample references sorted by contribution weight (descending).
- Details: model_id Adapter version or model identifier. Sample references sorted by contribution weight (descending).

#### `OriginRecord getSampleOrigin(const std::string &sample_id) const =0`
- Source: `include/training/training_interfaces.h`:462
- Brief: Return the origin record for a specific training sample.
- Parameters:
  - `sample_id` (const std::string &): Sample key.
- Return: Origin record; result.found is false when the sample is unknown.
- Details: sample_id Sample key. Origin record; result.found is false when the sample is unknown.

#### `~ILineageQueryAPI()=default`
- Source: `include/training/training_interfaces.h`:447
- Brief: n/a
- Parameters: none

### themis::training::ILoRACheckpointManager

#### `std::vector< CheckpointDescriptor > listCheckpoints(const std::string &model_id) const =0`
- Source: `include/training/training_interfaces.h`:259
- Brief: List all checkpoints associated with a base model.
- Parameters:
  - `model_id` (const std::string &): Base model identifier.
- Return: Descriptors sorted newest-first.
- Details: model_id Base model identifier. Descriptors sorted newest-first.

#### `std::future< LoRAWeights > load(const CheckpointId &checkpoint_id)=0`
- Source: `include/training/training_interfaces.h`:245
- Brief: Asynchronously load adapter weights by their content ID.
- Parameters:
  - `checkpoint_id` (const CheckpointId &): Identifier returned by save().
- Return: Future resolving to the loaded weights.
- Details: checkpoint_id Identifier returned by save(). Future resolving to the loaded weights.

#### `CheckpointId save(const LoRAWeights &weights)=0`
- Source: `include/training/training_interfaces.h`:238
- Brief: Persist LoRA weights and return their content-addressed ID.
- Parameters:
  - `weights` (const LoRAWeights &): Adapter weights to store.
- Return: SHA-256-derived checkpoint identifier.
- Throws:
  - std::runtime_error: on I/O failure.
- Details: weights Adapter weights to store. SHA-256-derived checkpoint identifier. std::runtime_error on I/O failure.

#### `bool verify(const CheckpointId &checkpoint_id) const =0`
- Source: `include/training/training_interfaces.h`:252
- Brief: Re-derive the content hash and confirm integrity.
- Parameters:
  - `checkpoint_id` (const CheckpointId &): Identifier to validate.
- Return: true if the stored file matches its SHA-256 digest.
- Details: checkpoint_id Identifier to validate. true if the stored file matches its SHA-256 digest.

#### `~ILoRACheckpointManager()=default`
- Source: `include/training/training_interfaces.h`:230
- Brief: n/a
- Parameters: none

### themis::training::ISampleProvenanceTracker

#### `LineageGraph queryLineage(const std::string &sample_id) const =0`
- Source: `include/training/training_interfaces.h`:294
- Brief: Retrieve the transformation DAG for a sample.
- Parameters:
  - `sample_id` (const std::string &): Sample to trace.
- Return: Lineage graph rooted at the training-ready sample.
- Details: sample_id Sample to trace. Lineage graph rooted at the training-ready sample.

#### `void record(const SampleProvenance &provenance)=0`
- Source: `include/training/training_interfaces.h`:287
- Brief: Append a provenance record. Cannot overwrite an existing record.
- Parameters:
  - `provenance` (const SampleProvenance &): Provenance data to store.
- Throws:
  - std::invalid_argument: if sample_id is already recorded.
- Details: provenance Provenance data to store. std::invalid_argument if sample_id is already recorded.

#### `size_t storageEstimateBytes() const =0`
- Source: `include/training/training_interfaces.h`:304
- Brief: Estimated storage footprint in bytes.
- Parameters: none

#### `size_t totalRecords() const =0`
- Source: `include/training/training_interfaces.h`:299
- Brief: Total number of provenance records stored.
- Parameters: none

#### `~ISampleProvenanceTracker()=default`
- Source: `include/training/training_interfaces.h`:280
- Brief: n/a
- Parameters: none

### themis::training::ITrainingPipeline

#### `CancelResult cancel(const JobId &job_id)=0`
- Source: `include/training/training_interfaces.h`:423
- Brief: Request cancellation of a running or queued job.
- Parameters:
  - `job_id` (const JobId &): Identifier returned by submit (or job.job_id if set).
- Return: Cancelled, AlreadyCompleted, or NotFound.
- Details: job_id Identifier returned by submit (or job.job_id if set). Cancelled, AlreadyCompleted, or NotFound.

#### `JobStatus status(const JobId &job_id) const =0`
- Source: `include/training/training_interfaces.h`:430
- Brief: Query the current lifecycle state of a job.
- Parameters:
  - `job_id` (const JobId &): Job identifier.
- Return: Current JobStatus.
- Details: job_id Job identifier. Current JobStatus.

#### `std::future< AsyncTrainingResult > submit(const TrainingJob &job)=0`
- Source: `include/training/training_interfaces.h`:416
- Brief: Submit a training job for asynchronous execution.
- Parameters:
  - `job` (const TrainingJob &): Job specification (dataset, model config, optional LoRA config).
- Return: Future resolving to the training result.
- Details: job Job specification (dataset, model config, optional LoRA config). Future resolving to the training result.

#### `~ITrainingPipeline()=default`
- Source: `include/training/training_interfaces.h`:409
- Brief: n/a
- Parameters: none

### themis::training::IncrementalLoRATrainer

#### `IncrementalLoRATrainer(const IncrementalLoRATrainer &)=delete`
- Source: `include/training/incremental_lora_trainer.h`:241
- Brief: n/a
- Parameters:
  - `<unnamed>` (const IncrementalLoRATrainer &): n/a

#### `IncrementalLoRATrainer(const IncrementalTrainingConfig &config, const std::string &db_connection)`
- Source: `include/training/incremental_lora_trainer.h`:235
- Brief: Construct incremental trainer.
- Parameters:
  - `config` (const IncrementalTrainingConfig &): Training configuration
  - `db_connection` (const std::string &): Database connection string
- Details: config Training configuration db_connection Database connection string

#### `void applyGlobalDelta(const themis::distributed_knowledge::GlobalAdapterDelta &delta)`
- Source: `include/training/incremental_lora_trainer.h`:422
- Brief: Incorporate an aggregated global delta into the local adapter weights.
- Parameters:
  - `delta` (const themis::distributed_knowledge::GlobalAdapterDelta &): Input parameter.
- Details: Apply Global Delta. Applies: local_weight[layer] += federated_lr * delta.delta[layer] for every layer name present in delta.delta. Unknown layer names in the delta are silently ignored (forward-compatible with larger global models). delta Aggregated weight delta produced by LoRAFederationCoordinator. delta Input parameter. Implements applyGlobalDelta without additional internal calls.

#### `bool deployVersion(const std::string &adapter_version, float traffic_split=1.0f)`
- Source: `include/training/incremental_lora_trainer.h`:275
- Brief: Deploy adapter version to production.
- Parameters:
  - `adapter_version` (const std::string &): Input parameter.
  - `traffic_split` (float): Input parameter.
- Return: true if deployment successful (including integrity verification)
- Details: Deploy Version. adapter_version Version to deploy traffic_split Traffic split for A/B testing (0.0-1.0) true if deployment successful (including integrity verification) adapter_version Input parameter. traffic_split Input parameter. True when the operation succeeds. Implements deployVersion without additional internal calls.

#### `DeployResult deployVersionEx(const std::string &adapter_version, float traffic_split=1.0f)`
- Source: `include/training/incremental_lora_trainer.h`:297
- Brief: Deploy adapter version with full result details.
- Parameters:
  - `adapter_version` (const std::string &): Input parameter.
  - `traffic_split` (float): Input parameter.
- Return: DeployResult with success flag, active_version, split_applied, error.
- Details: Deploy Version Ex. Equivalent to deployVersion() but returns a DeployResult that includes the active version, applied split fraction, and a human-readable error message on failure. When an ILLMRouter has been injected via setLLMRouter(), the router's setAdapterWeight() is called atomically after the local version registry is updated. Error codes in DeployResult::error: "version_not_found" – version is unknown and not in checkpoint dir "integrity_failure" – checkpoint checksum validation failed "router_unavailable" – router is not reachable "router_update_failed" – router rejected the weight update and local state was reverted "invalid_split" – traffic_split outside [0, 1] adapter_version Version identifier to deploy. traffic_split Fraction of traffic routed to this version [0,1]. DeployResult with success flag, active_version, split_applied, error. adapter_version Input parameter. traffic_split Input parameter. Return value. Implements deployVersionEx without additional internal calls.

#### `void enableIntermediateCheckpointing(bool enabled, size_t save_interval=100)`
- Source: `include/training/incremental_lora_trainer.h`:486
- Brief: Phase 2: Enable intermediate checkpoint saving during training.
- Parameters:
  - `enabled` (bool): Input parameter.
  - `save_interval` (size_t): Input parameter.
- Details: Enable Intermediate Checkpointing. When enabled, saves adapter state to checkpoint every N steps. This allows resuming from the most recent checkpoint on interruption. enabled Whether to enable intermediate checkpoints save_interval Steps between intermediate saves (default: 100) enabled Input parameter. save_interval Input parameter. Calls: setCheckpointing().

#### `TrainingResult evaluate(const std::string &adapter_version)`
- Source: `include/training/incremental_lora_trainer.h`:267
- Brief: Evaluate adapter on validation set.
- Parameters:
  - `adapter_version` (const std::string &): Input parameter.
- Return: Evaluation metrics
- Details: Evaluate. adapter_version Adapter version to evaluate Evaluation metrics adapter_version Input parameter. Return value. Implements evaluate without additional internal calls.

#### `themis::distributed_knowledge::EncryptedGradient exportGradient(uint64_t federation_round)`
- Source: `include/training/incremental_lora_trainer.h`:411
- Brief: Export the accumulated gradient delta as a federated contribution.
- Parameters:
  - `federation_round` (uint64_t): Current federated round number (embedded in the result).
- Return: EncryptedGradient with non-empty data map.
- Throws:
  - std::runtime_error: When no training has occurred since the last export.
- Details: Reads the gradient accumulator that has been filled during train() calls since the last exportGradient() invocation (or since construction). The gradient is normalised: data[layer] = Σ(deltas) / update_count. After a successful export the accumulator is reset to zero so that the next export only reflects new training steps. federation_round Current federated round number (embedded in the result). EncryptedGradient with non-empty data map. std::runtime_error When no training has occurred since the last export.

#### `double getLocalWeight(const std::string &layer_name) const`
- Source: `include/training/incremental_lora_trainer.h`:434
- Brief: Return the current local weight for a named layer.
- Parameters:
  - `layer_name` (const std::string &): Layer identifier (e.g. "lora_A_layer_0").
- Return: Current weight value, or 0.0 if the layer is not yet tracked.
- Details: Used by tests and observability tooling to verify that applyGlobalDelta() has modified the local weight map. layer_name Layer identifier (e.g. "lora_A_layer_0"). Current weight value, or 0.0 if the layer is not yet tracked.

#### `TrainingMetrics getMetrics() const`
- Source: `include/training/incremental_lora_trainer.h`:370
- Brief: Get training metrics accumulated during the last train() call.
- Parameters: none
- Details: Returns per-epoch and per-step losses, best loss values, and timing. The metrics are reset at the start of each train() call.

#### `std::string getRecoveryStatus() const`
- Source: `include/training/incremental_lora_trainer.h`:475
- Brief: Phase 2: Get training interruption status and recovery options.
- Parameters: none
- Return: Recovery status string (empty if no interruption).
- Details: Returns information about any previous training interruption: Whether a checkpoint can be resumed from Recommended recovery action Time since interruption Recovery status string (empty if no interruption).

#### `std::string getTrainingDiagnostics() const`
- Source: `include/training/incremental_lora_trainer.h`:463
- Brief: Phase 2: Get detailed training diagnostics and recovery info.
- Parameters: none
- Return: Formatted diagnostics string.
- Details: Returns human-readable diagnostics including: Current training state (IDLE/TRAINING/SAVING/RECOVERING) Last error encountered (if any) Checkpoint recovery status Performance metrics summary Formatted diagnostics string.

#### `std::vector< std::string > listVersions() const`
- Source: `include/training/incremental_lora_trainer.h`:322
- Brief: Get list of available adapter versions.
- Parameters: none
- Return: Vector of version identifiers
- Details: Vector of version identifiers

#### `IncrementalLoRATrainer & operator=(const IncrementalLoRATrainer &)=delete`
- Source: `include/training/incremental_lora_trainer.h`:242
- Brief: n/a
- Parameters:
  - `<unnamed>` (const IncrementalLoRATrainer &): n/a

#### `TrainingResult resumeFromCheckpoint(const std::string &checkpoint_path, TrainingCallback callback=nullptr)`
- Source: `include/training/incremental_lora_trainer.h`:259
- Brief: Resume training from checkpoint.
- Parameters:
  - `checkpoint_path` (const std::string &): Path to the checkpoint.
  - `callback` (TrainingCallback): Input parameter.
- Return: Training result
- Details: Resume From Checkpoint. checkpoint_path Path to checkpoint file callback Optional progress callback Training result checkpoint_path Path to the checkpoint. callback Input parameter. Return value. Implements resumeFromCheckpoint without additional internal calls.

#### `bool rollbackVersion(const std::string &target_version)`
- Source: `include/training/incremental_lora_trainer.h`:305
- Brief: Rollback to previous adapter version.
- Parameters:
  - `target_version` (const std::string &): Input parameter.
- Return: true if rollback successful
- Details: Rollback Version. target_version Version to roll back to true if rollback successful target_version Input parameter. True when the operation succeeds. Implements rollbackVersion without additional internal calls.

#### `DeployResult rollbackVersionEx(const std::string &target_version)`
- Source: `include/training/incremental_lora_trainer.h`:316
- Brief: Roll back to a target version with full result details.
- Parameters:
  - `target_version` (const std::string &): Input parameter.
- Return: DeployResult describing the outcome.
- Details: Rollback Version Ex. Equivalent to rollbackVersion() but returns a DeployResult. When an ILLMRouter has been injected, the router weight is updated atomically. target_version Version to roll back to. DeployResult describing the outcome. target_version Input parameter. Return value. Implements rollbackVersionEx without additional internal calls.

#### `std::string selectAdapterForRequest() const`
- Source: `include/training/incremental_lora_trainer.h`:333
- Brief: Select an adapter version for the next request using weighted-random traffic routing.
- Parameters: none
- Return: Selected adapter version name, or empty string if no version is active.
- Details: Returns the adapter version name chosen according to the traffic_split weights set via deployVersion(). When multiple versions are active, each call independently samples from the configured distribution, enabling A/B-test-style canary rollouts. Selected adapter version name, or empty string if no version is active.

#### `void setCheckpointing(bool enabled, size_t checkpoint_steps=100)`
- Source: `include/training/incremental_lora_trainer.h`:348
- Brief: Enable/disable checkpointing.
- Parameters:
  - `enabled` (bool): Input parameter.
  - `checkpoint_steps` (size_t): Input parameter.
- Details: Set Checkpointing. enabled Whether to save checkpoints checkpoint_steps Steps between checkpoints enabled Input parameter. checkpoint_steps Input parameter. Implements setCheckpointing without additional internal calls.

#### `void setFederatedLearningRate(double lr)`
- Source: `include/training/incremental_lora_trainer.h`:394
- Brief: Set the learning rate applied when incorporating a global delta.
- Parameters:
  - `lr` (double): Input parameter.
- Details: Set Federated Learning Rate. Controls the weight update in applyGlobalDelta(): local_weight[layer] += federated_lr * global_delta[layer] Defaults to 0.01. Must be positive. lr Federated learning rate (> 0). lr Input parameter. Implements setFederatedLearningRate without additional internal calls.

#### `void setHyperparameters(int rank, float alpha, float learning_rate)`
- Source: `include/training/incremental_lora_trainer.h`:341
- Brief: Set training hyperparameters.
- Parameters:
  - `rank` (int): Input parameter.
  - `alpha` (float): Input parameter.
  - `learning_rate` (float): Input parameter.
- Details: Set Hyperparameters. rank LoRA rank alpha LoRA alpha learning_rate Learning rate rank Input parameter. alpha Input parameter. learning_rate Input parameter. Implements setHyperparameters without additional internal calls.

#### `void setLLMRouter(ILLMRouter *router)`
- Source: `include/training/incremental_lora_trainer.h`:362
- Brief: Inject an LLM router for adapter serving integration.
- Parameters:
  - `router` (ILLMRouter *): Input/output parameter.
- Details: Set LLMRouter. When a non-null router is set, deployVersionEx() and rollbackVersionEx() call router->setAdapterWeight() after updating the local version registry to propagate the traffic split to the live inference layer. The trainer does NOT take ownership; the router must remain valid for the lifetime of this trainer. router Pointer to ILLMRouter implementation, or nullptr to detach. router Input/output parameter. Implements setLLMRouter without additional internal calls.

#### `void setShardId(const std::string &shard_id)`
- Source: `include/training/incremental_lora_trainer.h`:382
- Brief: Set the shard identifier used in exported gradients.
- Parameters:
  - `shard_id` (const std::string &): Identifier of the shard.
- Details: ── IMPL-A3: Federation bridges ──────────────────────────────────────────── The shard_id is embedded in every EncryptedGradient produced by exportGradient(). Defaults to "default_shard" when not set. shard_id Cluster-unique shard identifier. shard_id Identifier of the shard. Implements setShardId without additional internal calls.

#### `TrainingResult train(TrainingMode mode=TrainingMode::INITIAL, TrainingCallback callback=nullptr)`
- Source: `include/training/incremental_lora_trainer.h`:250
- Brief: Train LoRA adapter.
- Parameters:
  - `mode` (TrainingMode): Input parameter.
  - `callback` (TrainingCallback): Input parameter.
- Return: Training result with version info
- Details: Train. mode Training mode (INITIAL/INCREMENTAL/FINETUNE) callback Optional progress callback Training result with version info mode Input parameter. callback Input parameter. Return value. Implements train without additional internal calls.

#### `std::string validateTrainingState() const`
- Source: `include/training/incremental_lora_trainer.h`:450
- Brief: Phase 2: Validate training state machine for correctness.
- Parameters: none
- Return: Empty string if state is valid; error message otherwise.
- Details: Returns an error message if the trainer is in an invalid state: Training cannot start if already in progress Checkpointing cannot save intermediate states incorrectly Deployment cannot occur during active training Empty string if state is valid; error message otherwise.

#### `~IncrementalLoRATrainer()`
- Source: `include/training/incremental_lora_trainer.h`:238
- Brief: n/a
- Parameters: none

### themis::training::IncrementalLoRATrainer::Impl

#### `Impl(const IncrementalTrainingConfig &config, const std::string &db_connection)`
- Source: `src/training/incremental_lora_trainer.cpp`:272
- Brief: Impl.
- Parameters:
  - `config` (const IncrementalTrainingConfig &): Input parameter.
  - `db_connection` (const std::string &): Input parameter.
- Return: Return value.
- Details: config Input parameter. db_connection Input parameter. Return value.

#### `int activeGpuCount() const`
- Source: `src/training/incremental_lora_trainer.cpp`:1785
- Brief: n/a
- Parameters: none

#### `void applyGlobalDelta(const themis::distributed_knowledge::GlobalAdapterDelta &delta)`
- Source: `src/training/incremental_lora_trainer.cpp`:846
- Brief: Apply Global Delta.
- Parameters:
  - `delta` (const themis::distributed_knowledge::GlobalAdapterDelta &): Input parameter.
- Details: delta Input parameter. Calls: items(), count(), is_number().

#### `double computeAccuracy(double loss)`
- Source: `src/training/incremental_lora_trainer.cpp`:1815
- Brief: Estimate accuracy from loss (Phase 3).
- Parameters:
  - `loss` (double): Input parameter.
- Return: Return value.
- Details: loss Input parameter. Return value. Calls: std::exp().

#### `double computeSimulatedLoss(size_t step, float learning_rate)`
- Source: `src/training/incremental_lora_trainer.cpp`:1802
- Brief: Simulate a decreasing loss curve (Phase 3).
- Parameters:
  - `step` (size_t): Input parameter.
  - `learning_rate` (float): Input parameter.
- Return: Return value.
- Details: step Input parameter. learning_rate Input parameter. Return value. Calls: std::exp().

#### `bool deployVersion(const std::string &adapter_version, float traffic_split)`
- Source: `src/training/incremental_lora_trainer.cpp`:583
- Brief: ---------------------------------------------------------------------- Phase 4: Deployment and version management ----------------------------------------------------------------------
- Parameters:
  - `adapter_version` (const std::string &): Input parameter.
  - `traffic_split` (float): Input parameter.
- Return: True when the operation succeeds.
- Details: adapter_version Input parameter. traffic_split Input parameter. True when the operation succeeds. Calls: empty(), version_lock(), find(), end(), VersionRecord(), std::chrono::system_clock::now().

#### `DeployResult deployVersionEx(const std::string &adapter_version, float traffic_split)`
- Source: `src/training/incremental_lora_trainer.cpp`:971
- Brief: Deploy Version Ex.
- Parameters:
  - `adapter_version` (const std::string &): Input parameter.
  - `traffic_split` (float): Input parameter.
- Return: Return value.
- Details: adapter_version Input parameter. traffic_split Input parameter. Return value. Calls: empty(), DeployResult::fail(), verifyAdapterIntegrity(), version_lock(), deployVersion(), lk(), isAvailable(), setAdapterWeight().

#### `std::vector< float > encodeSample(const std::string &text, size_t feature_dim) const`
- Source: `src/training/incremental_lora_trainer.cpp`:1319
- Brief: n/a
- Parameters:
  - `text` (const std::string &): n/a
  - `feature_dim` (size_t): n/a

#### `TrainingResult evaluate(const std::string &adapter_version)`
- Source: `src/training/incremental_lora_trainer.cpp`:531
- Brief: ---------------------------------------------------------------------- Phase 4: Evaluation ----------------------------------------------------------------------
- Parameters:
  - `adapter_version` (const std::string &): Input parameter.
- Return: Return value.
- Details: adapter_version Input parameter. Return value. Calls: TrainingResult(), std::chrono::steady_clock::now(), empty(), size(), computeAccuracy(), std::string(), what(), count().

#### `themis::distributed_knowledge::EncryptedGradient exportGradient(uint64_t federation_round)`
- Source: `src/training/incremental_lora_trainer.cpp`:817
- Brief: n/a
- Parameters:
  - `federation_round` (uint64_t): n/a

#### `std::string generateVersionId(const std::string &base_version) const`
- Source: `src/training/incremental_lora_trainer.cpp`:1962
- Brief: n/a
- Parameters:
  - `base_version` (const std::string &): n/a

#### `double getLocalWeight(const std::string &layer_name) const`
- Source: `src/training/incremental_lora_trainer.cpp`:860
- Brief: n/a
- Parameters:
  - `layer_name` (const std::string &): n/a

#### `TrainingMetrics getMetrics() const`
- Source: `src/training/incremental_lora_trainer.cpp`:790
- Brief: n/a
- Parameters: none

#### `void initLoRAComponents()`
- Source: `src/training/incremental_lora_trainer.cpp`:1157
- Brief: Init Lo RAComponents.
- Parameters: none
- Throws:
  - std::runtime_error: if an error occurs.
- Details: std::runtime_error if an error occurs. Calls: std::max(), defined(), num_gpus(), std::to_string(), empty(), create_layer(), spdlog::info(), reset().

#### `std::vector< std::string > listVersions() const`
- Source: `src/training/incremental_lora_trainer.cpp`:667
- Brief: n/a
- Parameters: none

#### `bool loadCheckpoint(const std::string &path, std::string &version, size_t &epoch, size_t &step, double &loss, double &accuracy, std::string *error_reason=nullptr) const`
- Source: `src/training/incremental_lora_trainer.cpp`:1894
- Brief: n/a
- Parameters:
  - `path` (const std::string &): n/a
  - `version` (std::string &): n/a
  - `epoch` (size_t &): n/a
  - `step` (size_t &): n/a
  - `loss` (double &): n/a
  - `accuracy` (double &): n/a
  - `error_reason` (std::string *): n/a

#### `void registerVersion(const TrainingResult &result)`
- Source: `src/training/incremental_lora_trainer.cpp`:1948
- Brief: ---------------------------------------------------------------------- Phase 4: Version registry helpers ----------------------------------------------------------------------
- Parameters:
  - `result` (const TrainingResult &): Input parameter.
- Details: result Input parameter. Calls: std::chrono::system_clock::now().

#### `TrainingResult resumeFromCheckpoint(const std::string &checkpoint_path, TrainingCallback callback)`
- Source: `src/training/incremental_lora_trainer.cpp`:449
- Brief: ---------------------------------------------------------------------- Phase 5: Resume from checkpoint ----------------------------------------------------------------------
- Parameters:
  - `checkpoint_path` (const std::string &): Path to the checkpoint.
  - `callback` (TrainingCallback): Input parameter.
- Return: Return value.
- Details: checkpoint_path Path to the checkpoint. callback Input parameter. Return value. Calls: TrainingResult(), std::chrono::steady_clock::now(), empty(), loadCheckpoint(), verifyCheckpointPayloadIntegrity(), initLoRAComponents(), loadCheckpointWeights(), train().

#### `bool rollbackVersion(const std::string &target_version)`
- Source: `src/training/incremental_lora_trainer.cpp`:635
- Brief: Rollback Version.
- Parameters:
  - `target_version` (const std::string &): Input parameter.
- Return: True when the operation succeeds.
- Details: target_version Input parameter. True when the operation succeeds. Calls: empty(), version_lock(), find(), end(), VersionRecord(), std::chrono::system_clock::now().

#### `DeployResult rollbackVersionEx(const std::string &target_version)`
- Source: `src/training/incremental_lora_trainer.cpp`:1031
- Brief: Rollback Version Ex.
- Parameters:
  - `target_version` (const std::string &): Input parameter.
- Return: Return value.
- Details: target_version Input parameter. Return value. Calls: empty(), DeployResult::fail(), verifyAdapterIntegrity(), version_lock(), rollbackVersion(), lk(), isAvailable(), setAdapterWeight().

#### `double runTrainingStep(const std::vector< std::pair< std::string, std::string > > &training_data, size_t batch_offset, size_t step_idx)`
- Source: `src/training/incremental_lora_trainer.cpp`:1584
- Brief: n/a
- Parameters:
  - `training_data` (const std::vector< std::pair< std::string, std::string > > &): n/a
  - `batch_offset` (size_t): n/a
  - `step_idx` (size_t): n/a

#### `void saveCheckpoint(const std::string &version, size_t epoch, size_t step, double loss, double accuracy) const`
- Source: `src/training/incremental_lora_trainer.cpp`:1823
- Brief: n/a
- Parameters:
  - `version` (const std::string &): n/a
  - `epoch` (size_t): n/a
  - `step` (size_t): n/a
  - `loss` (double): n/a
  - `accuracy` (double): n/a

#### `std::string selectAdapterForRequest() const`
- Source: `src/training/incremental_lora_trainer.cpp`:685
- Brief: n/a
- Parameters: none

#### `void setCheckpointing(bool enabled, size_t checkpoint_steps)`
- Source: `src/training/incremental_lora_trainer.cpp`:775
- Brief: Set Checkpointing.
- Parameters:
  - `enabled` (bool): Input parameter.
  - `checkpoint_steps` (size_t): Input parameter.
- Details: enabled Input parameter. checkpoint_steps Input parameter. Implements setCheckpointing without additional internal calls.

#### `void setFederatedLearningRate(double lr)`
- Source: `src/training/incremental_lora_trainer.cpp`:810
- Brief: Set Federated Learning Rate.
- Parameters:
  - `lr` (double): Input parameter.
- Throws:
  - std::invalid_argument: if an error occurs.
- Details: lr Input parameter. std::invalid_argument if an error occurs. Implements setFederatedLearningRate without additional internal calls.

#### `void setHyperparameters(int rank, float alpha, float learning_rate)`
- Source: `src/training/incremental_lora_trainer.cpp`:731
- Brief: ---------------------------------------------------------------------- Phase 3: Hyperparameter API ----------------------------------------------------------------------
- Parameters:
  - `rank` (int): Input parameter.
  - `alpha` (float): Input parameter.
  - `learning_rate` (float): Input parameter.
- Throws:
  - std::invalid_argument: if an error occurs.
- Details: rank Input parameter. alpha Input parameter. learning_rate Input parameter. std::invalid_argument if an error occurs. Calls: reset(), defined().

#### `void setLLMRouter(ILLMRouter *router)`
- Source: `src/training/incremental_lora_trainer.cpp`:785
- Brief: Set LLMRouter.
- Parameters:
  - `router` (ILLMRouter *): Input/output parameter.
- Details: router Input/output parameter. Calls: lk().

#### `void setShardId(const std::string &shard_id)`
- Source: `src/training/incremental_lora_trainer.cpp`:800
- Brief: ── IMPL-A3: Federation bridges ──────────────────────────────────────────
- Parameters:
  - `shard_id` (const std::string &): Identifier of the shard.
- Details: shard_id Identifier of the shard. Calls: empty().

#### `TrainingResult train(TrainingMode mode, TrainingCallback callback)`
- Source: `src/training/incremental_lora_trainer.cpp`:288
- Brief: ---------------------------------------------------------------------- Phase 3: Training implementation ----------------------------------------------------------------------
- Parameters:
  - `mode` (TrainingMode): Input parameter.
  - `callback` (TrainingCallback): Input parameter.
- Return: Return value.
- Details: mode Input parameter. callback Input parameter. Return value. Calls: exchange(), TrainingResult(), THEMIS_ERROR(), std::chrono::steady_clock::now(), sanitizeTrainingPromptLikeText(), store(), reset(), generateVersionId().

#### `void validateHyperparameters() const`
- Source: `src/training/incremental_lora_trainer.cpp`:1767
- Brief: n/a
- Parameters: none

#### `bool verifyAdapterIntegrity(const std::string &adapter_version) const`
- Source: `src/training/incremental_lora_trainer.cpp`:873
- Brief: n/a
- Parameters:
  - `adapter_version` (const std::string &): n/a

#### `bool verifyCheckpointPayloadIntegrity(const std::string &checkpoint_prefix, const std::string &adapter_version, size_t epoch, size_t step, std::string *error_reason) const`
- Source: `src/training/incremental_lora_trainer.cpp`:899
- Brief: n/a
- Parameters:
  - `checkpoint_prefix` (const std::string &): n/a
  - `adapter_version` (const std::string &): n/a
  - `epoch` (size_t): n/a
  - `step` (size_t): n/a
  - `error_reason` (std::string *): n/a

#### `~Impl()=default`
- Source: `src/training/incremental_lora_trainer.cpp`:279
- Brief: n/a
- Parameters: none

### themis::training::IncrementalTrainingConfig

#### `IncrementalTrainingConfig()=default`
- Source: `include/training/incremental_lora_trainer.h`:191
- Brief: n/a
- Parameters: none

### themis::training::KGRelation

#### `KGRelation()=default`
- Source: `include/training/training_interfaces.h`:111
- Brief: n/a
- Parameters: none

### themis::training::KnowledgeGraphEnricher

#### `KnowledgeGraphEnricher(const EnrichmentConfig &config, const std::string &db_connection)`
- Source: `include/training/knowledge_graph_enricher.h`:137
- Brief: Construct graph enricher.
- Parameters:
  - `config` (const EnrichmentConfig &): Enrichment configuration
  - `db_connection` (const std::string &): Database connection string
- Details: config Enrichment configuration db_connection Database connection string

#### `KnowledgeGraphEnricher(const KnowledgeGraphEnricher &)=delete`
- Source: `include/training/knowledge_graph_enricher.h`:143
- Brief: n/a
- Parameters:
  - `<unnamed>` (const KnowledgeGraphEnricher &): n/a

#### `void disableCache()`
- Source: `include/training/knowledge_graph_enricher.h`:271
- Brief: Disable the enrichment LRU cache and evict all entries.
- Parameters: none
- Details: Disable Cache. Implements disableCache without additional internal calls.

#### `void enableCache(const EnrichmentCacheConfig &config={})`
- Source: `include/training/knowledge_graph_enricher.h`:266
- Brief: Enable the enrichment LRU cache (Phase 9).
- Parameters:
  - `config` (const EnrichmentCacheConfig &): Input parameter.
- Details: Enable Cache. config Cache configuration (capacity, refresh interval). config Input parameter. Implements enableCache without additional internal calls.

#### `EnrichmentStats enrichAll(EnrichmentCallback callback=nullptr)`
- Source: `include/training/knowledge_graph_enricher.h`:151
- Brief: Enrich all samples in target collection.
- Parameters:
  - `callback` (EnrichmentCallback): Input parameter.
- Return: Enrichment statistics
- Details: Enrich All. callback Optional progress callback Enrichment statistics callback Input parameter. Return value. Implements enrichAll without additional internal calls.

#### `EnrichmentStats enrichQuery(const std::string &aql_query, EnrichmentCallback callback=nullptr)`
- Source: `include/training/knowledge_graph_enricher.h`:166
- Brief: Enrich samples matching a query.
- Parameters:
  - `aql_query` (const std::string &): Input parameter.
  - `callback` (EnrichmentCallback): Input parameter.
- Return: Enrichment statistics
- Details: Enrich Query. aql_query AQL query to select samples callback Optional progress callback Enrichment statistics aql_query Input parameter. callback Input parameter. Return value. Implements enrichQuery without additional internal calls.

#### `GraphContext enrichSample(const std::string &sample_id)`
- Source: `include/training/knowledge_graph_enricher.h`:158
- Brief: Enrich a specific sample.
- Parameters:
  - `sample_id` (const std::string &): Identifier of the sample.
- Return: Graph context for this sample
- Details: Enrich Sample. sample_id Sample ID Graph context for this sample sample_id Identifier of the sample. Return value. Implements enrichSample without additional internal calls.

#### `std::vector< std::string > findRelatedCaseLaw(const std::string &document_id, size_t max_results=5)`
- Source: `include/training/knowledge_graph_enricher.h`:184
- Brief: Find related case law for a document.
- Parameters:
  - `document_id` (const std::string &): Identifier of the document.
  - `max_results` (size_t): Input parameter.
- Return: Vector of case law document IDs
- Details: Find Related Case Law. document_id Document ID max_results Maximum number of results Vector of case law document IDs document_id Identifier of the document. max_results Input parameter. Return value. Implements findRelatedCaseLaw without additional internal calls.

#### `std::vector< std::string > findRelatedGuidance(const std::string &document_id, size_t max_results=5)`
- Source: `include/training/knowledge_graph_enricher.h`:193
- Brief: Find internal administrative guidance documents for a document.
- Parameters:
  - `document_id` (const std::string &): Identifier of the document.
  - `max_results` (size_t): Input parameter.
- Return: Vector of guidance document IDs
- Details: Find Related Guidance. document_id Document ID max_results Maximum number of results Vector of guidance document IDs document_id Identifier of the document. max_results Input parameter. Return value. Implements findRelatedGuidance without additional internal calls.

#### `std::vector< std::string > findRelatedProvisions(const std::string &document_id, size_t max_results=5)`
- Source: `include/training/knowledge_graph_enricher.h`:175
- Brief: Find related legal provisions for a document.
- Parameters:
  - `document_id` (const std::string &): Identifier of the document.
  - `max_results` (size_t): Input parameter.
- Return: Vector of related provision IDs
- Details: Find Related Provisions. document_id Document ID max_results Maximum number of results Vector of related provision IDs document_id Identifier of the document. max_results Input parameter. Return value. Implements findRelatedProvisions without additional internal calls.

#### `std::vector< std::pair< std::string, float > > findSimilarDocuments(const std::string &document_id, size_t max_results=5)`
- Source: `include/training/knowledge_graph_enricher.h`:202
- Brief: Find similar documents using semantic search.
- Parameters:
  - `document_id` (const std::string &): Document ID
  - `max_results` (size_t): Maximum number of results
- Return: Vector of similar document IDs with similarity scores
- Details: document_id Document ID max_results Maximum number of results Vector of similar document IDs with similarity scores

#### `EnrichmentCacheStats getCacheStats() const`
- Source: `include/training/knowledge_graph_enricher.h`:276
- Brief: Return current cache hit/miss/eviction statistics.
- Parameters: none

#### `std::string getQueryTemplate(const std::string &query_name) const`
- Source: `include/training/knowledge_graph_enricher.h`:260
- Brief: Get AQL query template by name (Phase 6).
- Parameters:
  - `query_name` (const std::string &): Built-in name ("find_provisions", "find_case_law", "find_guidance", "find_similar", "update_context", "fetch_all") or custom name
- Return: AQL query template string, or empty string if not found
- Details: query_name Built-in name ("find_provisions", "find_case_law", "find_guidance", "find_similar", "update_context", "fetch_all") or custom name AQL query template string, or empty string if not found

#### `KnowledgeGraphEnricher & operator=(const KnowledgeGraphEnricher &)=delete`
- Source: `include/training/knowledge_graph_enricher.h`:144
- Brief: n/a
- Parameters:
  - `<unnamed>` (const KnowledgeGraphEnricher &): n/a

#### `void registerSourceDocument(const std::string &sample_id, const std::string &document_id)`
- Source: `include/training/knowledge_graph_enricher.h`:243
- Brief: Register a sample → source-document mapping for offline use.
- Parameters:
  - `sample_id` (const std::string &): Identifier of the sample.
  - `document_id` (const std::string &): Identifier of the document.
- Details: Register Source Document. When no AQL query engine is wired, enrichSample() resolves the source document ID of a sample by looking up this in-process registry. Entries must be registered before calling enrichSample(). sample_id Training sample key. document_id Corresponding source document ID / URN. sample_id Identifier of the sample. document_id Identifier of the document. Implements registerSourceDocument without additional internal calls.

#### `void setCustomQuery(const std::string &query_name, const std::string &aql_query)`
- Source: `include/training/knowledge_graph_enricher.h`:251
- Brief: Set custom graph traversal query.
- Parameters:
  - `query_name` (const std::string &): Name of the query.
  - `aql_query` (const std::string &): Input parameter.
- Details: Set Custom Query. query_name Query name (e.g., "find_provisions") aql_query AQL query template with placeholders query_name Name of the query. aql_query Input parameter. Implements setCustomQuery without additional internal calls.

#### `void setGraphVersion(const std::string &version)`
- Source: `include/training/knowledge_graph_enricher.h`:231
- Brief: Set the graph schema version used for cache-key generation.
- Parameters:
  - `version` (const std::string &): Input parameter.
- Details: Set Graph Version. The version string is appended to every cache key so that schema changes can be reflected immediately without clearing the entire cache. Defaults to "v0" (deterministic for offline/test builds). In production, call this with the current schema version after connecting to the graph DB. version Non-empty version string (e.g. "v3", "2026-05-05"). Ignored if empty. version Input parameter. Implements setGraphVersion without additional internal calls.

#### `void setVectorIndex(VectorIndexManager *vim)`
- Source: `include/training/knowledge_graph_enricher.h`:218
- Brief: Wire a vector index for semantic similarity search.
- Parameters:
  - `vim` (VectorIndexManager *): Input/output parameter.
- Details: Set Vector Index. When set, findSimilarDocuments() uses this index for cosine-similarity queries instead of returning an empty stub result. The index must already be initialised (i.e. init() called) and contain document embeddings stored under the key equal to the document ID. Ownership is NOT transferred; the caller must ensure the index outlives the enricher. vim Pointer to an initialised VectorIndexManager, or nullptr to disable vector search and revert to the offline stub. vim Input/output parameter. Implements setVectorIndex without additional internal calls.

#### `~KnowledgeGraphEnricher()`
- Source: `include/training/knowledge_graph_enricher.h`:140
- Brief: n/a
- Parameters: none

### themis::training::KnowledgeGraphEnricher::Impl

#### `Impl(const EnrichmentConfig &config, const std::string &db_connection)`
- Source: `src/training/knowledge_graph_enricher.cpp`:196
- Brief: Impl.
- Parameters:
  - `config` (const EnrichmentConfig &): Input parameter.
  - `db_connection` (const std::string &): Input parameter.
- Return: Return value.
- Details: config Input parameter. db_connection Input parameter. Return value.

#### `std::string buildContextSummary(const GraphContext &context) const`
- Source: `src/training/knowledge_graph_enricher.cpp`:688
- Brief: n/a
- Parameters:
  - `context` (const GraphContext &): n/a

#### `std::string cacheKey(const std::string &entity_key) const`
- Source: `src/training/knowledge_graph_enricher.cpp`:607
- Brief: n/a
- Parameters:
  - `entity_key` (const std::string &): n/a

#### `double computeContextQuality(const GraphContext &context)`
- Source: `src/training/knowledge_graph_enricher.cpp`:670
- Brief: Phase 6: Compute context quality score [0.
- Parameters:
  - `context` (const GraphContext &): Input parameter.
- Return: Return value.
- Details: context Input parameter. Return value. .1] Calls: empty(), std::min().

#### `void disableCache()`
- Source: `src/training/knowledge_graph_enricher.cpp`:573
- Brief: Disable Cache.
- Parameters: none
- Details: Calls: evictAll(), reset().

#### `float distanceToSimilarityScore(float distance)`
- Source: `src/training/knowledge_graph_enricher.cpp`:602
- Brief: Convert a VectorIndexManager distance to a cosine similarity score [0, 1].
- Parameters:
  - `distance` (float): Input parameter.
- Return: Return value.
- Details: distance Input parameter. Return value. For the COSINE metric VectorIndexManager stores distance = 1 - cosine, so similarity = 1 - distance. Clamped to [0, 1] to guard against floating- point rounding artefacts near the boundaries. Calls: std::max(), std::min().

#### `void enableCache(const EnrichmentCacheConfig &cfg)`
- Source: `src/training/knowledge_graph_enricher.cpp`:564
- Brief: ---------------------------------------------------------------------- Phase 9: LRU cache management ----------------------------------------------------------------------
- Parameters:
  - `cfg` (const EnrichmentCacheConfig &): Input parameter.
- Details: cfg Input parameter. Implements enableCache without additional internal calls.

#### `EnrichmentStats enrichAll(EnrichmentCallback callback)`
- Source: `src/training/knowledge_graph_enricher.cpp`:214
- Brief: ---------------------------------------------------------------------- Phase 6: Enrich all samples in collection ----------------------------------------------------------------------
- Parameters:
  - `callback` (EnrichmentCallback): Input parameter.
- Return: Return value.
- Details: callback Input parameter. Return value. Calls: EnrichmentStats(), std::chrono::steady_clock::now(), enrichSample(), size(), empty(), persistContext(), callback(), count().

#### `EnrichmentStats enrichQuery(const std::string &aql_query, EnrichmentCallback callback)`
- Source: `src/training/knowledge_graph_enricher.cpp`:332
- Brief: ---------------------------------------------------------------------- Phase 6: Query-based enrichment ----------------------------------------------------------------------
- Parameters:
  - `aql_query` (const std::string &): Input parameter.
  - `callback` (EnrichmentCallback): Input parameter.
- Return: Return value.
- Details: aql_query Input parameter. callback Input parameter. Return value. Calls: empty(), std::chrono::steady_clock::now(), enrichSample(), size(), persistContext(), callback(), count().

#### `GraphContext enrichSample(const std::string &sample_id)`
- Source: `src/training/knowledge_graph_enricher.cpp`:265
- Brief: ---------------------------------------------------------------------- Phase 6: Enrich a single sample ----------------------------------------------------------------------
- Parameters:
  - `sample_id` (const std::string &): Identifier of the sample.
- Return: Return value.
- Details: sample_id Identifier of the sample. Return value. Calls: empty(), resolveSourceDocumentId(), get(), cacheKey(), findRelatedProvisions(), findRelatedCaseLaw(), findSimilarDocuments(), push_back().

#### `std::vector< std::string > findRelatedCaseLaw(const std::string &document_id, size_t max_results)`
- Source: `src/training/knowledge_graph_enricher.cpp`:416
- Brief: Find Related Case Law.
- Parameters:
  - `document_id` (const std::string &): Identifier of the document.
  - `max_results` (size_t): Input parameter.
- Return: Return value.
- Details: document_id Identifier of the document. max_results Input parameter. Return value. Calls: empty(), find().

#### `std::vector< std::string > findRelatedGuidance(const std::string &document_id, size_t max_results)`
- Source: `src/training/knowledge_graph_enricher.cpp`:439
- Brief: Find Related Guidance.
- Parameters:
  - `document_id` (const std::string &): Identifier of the document.
  - `max_results` (size_t): Input parameter.
- Return: Return value.
- Details: document_id Identifier of the document. max_results Input parameter. Return value. Calls: empty(), find().

#### `std::vector< std::string > findRelatedProvisions(const std::string &document_id, size_t max_results)`
- Source: `src/training/knowledge_graph_enricher.cpp`:385
- Brief: ---------------------------------------------------------------------- Phase 6: Graph traversal helpers ----------------------------------------------------------------------
- Parameters:
  - `document_id` (const std::string &): Identifier of the document.
  - `max_results` (size_t): Input parameter.
- Return: Return value.
- Details: document_id Identifier of the document. max_results Input parameter. Return value. Calls: empty(), find().

#### `std::vector< std::pair< std::string, float > > findSimilarDocuments(const std::string &document_id, size_t max_results)`
- Source: `src/training/knowledge_graph_enricher.cpp`:455
- Brief: n/a
- Parameters:
  - `document_id` (const std::string &): n/a
  - `max_results` (size_t): n/a

#### `EnrichmentCacheStats getCacheStats() const`
- Source: `src/training/knowledge_graph_enricher.cpp`:580
- Brief: n/a
- Parameters: none

#### `std::string getQueryTemplate(const std::string &query_name) const`
- Source: `src/training/knowledge_graph_enricher.cpp`:532
- Brief: n/a
- Parameters:
  - `query_name` (const std::string &): n/a

#### `void persistContext(const std::string &sample_id, const GraphContext &context) const`
- Source: `src/training/knowledge_graph_enricher.cpp`:652
- Brief: n/a
- Parameters:
  - `sample_id` (const std::string &): n/a
  - `context` (const GraphContext &): n/a

#### `void registerSourceDocument(const std::string &sample_id, const std::string &document_id)`
- Source: `src/training/knowledge_graph_enricher.cpp`:632
- Brief: Register a sample → source-document mapping for offline/in-process use.
- Parameters:
  - `sample_id` (const std::string &): Identifier of the sample.
  - `document_id` (const std::string &): Identifier of the document.
- Details: sample_id Identifier of the sample. document_id Identifier of the document. Implements registerSourceDocument without additional internal calls.

#### `std::string resolveSourceDocumentId(const std::string &sample_id) const`
- Source: `src/training/knowledge_graph_enricher.cpp`:638
- Brief: n/a
- Parameters:
  - `sample_id` (const std::string &): n/a

#### `void setCustomQuery(const std::string &query_name, const std::string &aql_query)`
- Source: `src/training/knowledge_graph_enricher.cpp`:518
- Brief: Set Custom Query.
- Parameters:
  - `query_name` (const std::string &): Name of the query.
  - `aql_query` (const std::string &): Input parameter.
- Details: query_name Name of the query. aql_query Input parameter. Implements setCustomQuery without additional internal calls.

#### `void setGraphVersion(const std::string &version)`
- Source: `src/training/knowledge_graph_enricher.cpp`:620
- Brief: Set the graph schema version used in cache-key generation.
- Parameters:
  - `version` (const std::string &): Input parameter.
- Details: version Input parameter. Calls: empty().

#### `void setVectorIndex(VectorIndexManager *vim)`
- Source: `src/training/knowledge_graph_enricher.cpp`:527
- Brief: Set Vector Index.
- Parameters:
  - `vim` (VectorIndexManager *): Input/output parameter.
- Details: vim Input/output parameter. Implements setVectorIndex without additional internal calls.

#### `~Impl()=default`
- Source: `src/training/knowledge_graph_enricher.cpp`:206
- Brief: n/a
- Parameters: none

### themis::training::LabelingException

#### `LabelingException(const std::string &message, TrainingErrorCode code=TrainingErrorCode::LABELING_NO_CONTENT, bool recoverable=false, const std::string &context="")`
- Source: `include/training/training_exceptions.h`:188
- Brief: n/a
- Parameters:
  - `message` (const std::string &): n/a
  - `code` (TrainingErrorCode): n/a
  - `recoverable` (bool): n/a
  - `context` (const std::string &): n/a

### themis::training::LabelingStats

#### `LabelingStats()=default`
- Source: `include/training/auto_labeler.h`:74
- Brief: n/a
- Parameters: none

### themis::training::LegalAutoLabeler

#### `LegalAutoLabeler(const AutoLabelConfig &config, const std::string &db_connection, query::QueryEngine *engine=nullptr)`
- Source: `include/training/auto_labeler.h`:157
- Brief: Construct auto-labeler without a database connection.
- Parameters:
  - `config` (const AutoLabelConfig &): Labeling configuration
  - `db_connection` (const std::string &): Database connection string (informational)
  - `engine` (query::QueryEngine *): Optional AQL query engine; when non-null, labelAll() and labelQuery() fetch document IDs from the database via AQL. Pass nullptr (the default) to operate in test/offline mode, where no documents are fetched from the database.
- Details: config Labeling configuration db_connection Database connection string (informational) engine Optional AQL query engine; when non-null, labelAll() and labelQuery() fetch document IDs from the database via AQL. Pass nullptr (the default) to operate in test/offline mode, where no documents are fetched from the database.

#### `LegalAutoLabeler(const LegalAutoLabeler &)=delete`
- Source: `include/training/auto_labeler.h`:164
- Brief: n/a
- Parameters:
  - `<unnamed>` (const LegalAutoLabeler &): n/a

#### `std::vector< TrainingSample > getLowConfidenceSamples(float min_confidence=0.5f)`
- Source: `include/training/auto_labeler.h`:195
- Brief: Get low-confidence samples for human review.
- Parameters:
  - `min_confidence` (float): Input parameter.
- Return: Vector of low-confidence samples
- Details: Get Low Confidence Samples. min_confidence Minimum confidence threshold Vector of low-confidence samples min_confidence Input parameter. Return value. Implements getLowConfidenceSamples without additional internal calls.

#### `LabelingStats labelAll(LabelingCallback callback=nullptr)`
- Source: `include/training/auto_labeler.h`:172
- Brief: Label all documents in source collection.
- Parameters:
  - `callback` (LabelingCallback): Input parameter.
- Return: Labeling statistics
- Details: Label All. callback Optional progress callback Labeling statistics callback Input parameter. Return value. Implements labelAll without additional internal calls.

#### `std::vector< TrainingSample > labelDocument(const std::string &document_id)`
- Source: `include/training/auto_labeler.h`:179
- Brief: Label a specific document.
- Parameters:
  - `document_id` (const std::string &): Identifier of the document.
- Return: Vector of generated training samples
- Details: Label Document. document_id Document ID Vector of generated training samples document_id Identifier of the document. Return value. Implements labelDocument without additional internal calls.

#### `LabelingStats labelQuery(const std::string &aql_query, LabelingCallback callback=nullptr)`
- Source: `include/training/auto_labeler.h`:187
- Brief: Label documents matching a query.
- Parameters:
  - `aql_query` (const std::string &): Input parameter.
  - `callback` (LabelingCallback): Input parameter.
- Return: Labeling statistics
- Details: Label Query. aql_query AQL query to select documents callback Optional progress callback Labeling statistics aql_query Input parameter. callback Input parameter. Return value. Implements labelQuery without additional internal calls.

#### `LegalAutoLabeler & operator=(const LegalAutoLabeler &)=delete`
- Source: `include/training/auto_labeler.h`:165
- Brief: n/a
- Parameters:
  - `<unnamed>` (const LegalAutoLabeler &): n/a

#### `void registerDocument(const std::string &document_id, const std::string &text)`
- Source: `include/training/auto_labeler.h`:223
- Brief: Register a document text for offline/test-mode labeling.
- Parameters:
  - `document_id` (const std::string &): Identifier of the document.
  - `text` (const std::string &): Input parameter.
- Details: Register Document. When no QueryEngine is wired in, labelDocument() and fetchDocumentText() look up documents registered here instead of falling back to a fixed hardcoded placeholder paragraph. This allows unit and integration tests to exercise the full NLP pipeline with controlled, per-document texts without requiring a live database. Documents registered via this method take precedence over the built-in hardcoded fallback text. They do not affect the AQL-backed code path: when a QueryEngine is present, the DB always wins. document_id Primary key used to identify the document. text Document body text. document_id Identifier of the document. text Input parameter. Implements registerDocument without additional internal calls.

#### `void updateSampleConfidence(const std::string &sample_id, float new_confidence, const std::string &reviewed_by)`
- Source: `include/training/auto_labeler.h`:203
- Brief: Update sample confidence after human review.
- Parameters:
  - `sample_id` (const std::string &): Identifier of the sample.
  - `new_confidence` (float): Input parameter.
  - `reviewed_by` (const std::string &): Input parameter.
- Details: Update Sample Confidence. sample_id Sample ID new_confidence Updated confidence score reviewed_by User who reviewed the sample sample_id Identifier of the sample. new_confidence Input parameter. reviewed_by Input parameter. Implements updateSampleConfidence without additional internal calls.

#### `~LegalAutoLabeler()`
- Source: `include/training/auto_labeler.h`:161
- Brief: n/a
- Parameters: none

### themis::training::LegalAutoLabeler::Impl

#### `Impl(const AutoLabelConfig &config, const std::string &db_connection, ::themis::query::QueryEngine *engine)`
- Source: `src/training/auto_labeler.cpp`:87
- Brief: n/a
- Parameters:
  - `config` (const AutoLabelConfig &): n/a
  - `db_connection` (const std::string &): n/a
  - `engine` (::themis::query::QueryEngine *): n/a

#### `std::string buildQuery(const std::string &tmpl, const std::vector< std::pair< std::string, std::string > > &bindings) const`
- Source: `src/training/auto_labeler.cpp`:742
- Brief: n/a
- Parameters:
  - `tmpl` (const std::string &): n/a
  - `bindings` (const std::vector< std::pair< std::string, std::string > > &): n/a

#### `TrainingSample createSampleFromModality(const std::string &document_id, const std::string &text, const analytics::LegalModality &modality) const`
- Source: `src/training/auto_labeler.cpp`:811
- Brief: n/a
- Parameters:
  - `document_id` (const std::string &): n/a
  - `text` (const std::string &): n/a
  - `modality` (const analytics::LegalModality &): n/a

#### `std::vector< std::string > executeAqlQuery(const std::string &aql) const`
- Source: `src/training/auto_labeler.cpp`:532
- Brief: n/a
- Parameters:
  - `aql` (const std::string &): n/a

#### `std::optional< nlohmann::json > executeAqlWithRetry(const std::string &aql) const`
- Source: `src/training/auto_labeler.cpp`:790
- Brief: n/a
- Parameters:
  - `aql` (const std::string &): n/a

#### `std::vector< analytics::LegalModality > extractFallbackModalities(const std::string &text) const`
- Source: `src/training/auto_labeler.cpp`:645
- Brief: n/a
- Parameters:
  - `text` (const std::string &): n/a

#### `std::vector< std::string > fetchAllDocumentIdsDirect() const`
- Source: `src/training/auto_labeler.cpp`:479
- Brief: n/a
- Parameters: none

#### `std::vector< BaseEntity > fetchAllDocumentsDirect() const`
- Source: `src/training/auto_labeler.cpp`:466
- Brief: n/a
- Parameters: none

#### `std::string fetchDocumentText(const std::string &document_id) const`
- Source: `src/training/auto_labeler.cpp`:570
- Brief: n/a
- Parameters:
  - `document_id` (const std::string &): n/a

#### `std::string getBatchInsertQuery() const`
- Source: `src/training/auto_labeler.cpp`:439
- Brief: n/a
- Parameters: none

#### `std::string getFetchAllQuery() const`
- Source: `src/training/auto_labeler.cpp`:434
- Brief: n/a
- Parameters: none

#### `std::vector< TrainingSample > getLowConfidenceSamples(float min_confidence)`
- Source: `src/training/auto_labeler.cpp`:394
- Brief: Get Low Confidence Samples.
- Parameters:
  - `min_confidence` (float): Input parameter.
- Return: Return value.
- Details: min_confidence Input parameter. Return value. Implements getLowConfidenceSamples without additional internal calls.

#### `size_t getTotalErrors() const`
- Source: `src/training/auto_labeler.cpp`:431
- Brief: n/a
- Parameters: none

#### `size_t getTotalProcessed() const`
- Source: `src/training/auto_labeler.cpp`:430
- Brief: n/a
- Parameters: none

#### `bool isReadOnlyAqlQuery(const std::string &aql)`
- Source: `src/training/auto_labeler.cpp`:762
- Brief: Is Read Only Aql Query.
- Parameters:
  - `aql` (const std::string &): Input parameter.
- Return: True when the operation succeeds.
- Details: aql Input parameter. True when the operation succeeds. Calls: empty(), reserve(), size(), push_back(), std::toupper(), find().

#### `LabelingStats labelAll(LabelingCallback callback)`
- Source: `src/training/auto_labeler.cpp`:117
- Brief: Label All.
- Parameters:
  - `callback` (LabelingCallback): Input parameter.
- Return: Return value.
- Details: callback Input parameter. Return value. Calls: LabelingStats(), std::chrono::steady_clock::now(), buildQuery(), executeAqlQuery(), empty(), fetchAllDocumentIdsDirect(), reserve(), labelDocument().

#### `std::vector< TrainingSample > labelDocument(const std::string &document_id)`
- Source: `src/training/auto_labeler.cpp`:186
- Brief: Label Document.
- Parameters:
  - `document_id` (const std::string &): Identifier of the document.
- Return: Return value.
- Details: document_id Identifier of the document. Return value. Calls: empty(), fetchDocumentText(), lock(), parseDocument(), THEMIS_INFO(), mean_conf(), push_back(), std::move().

#### `LabelingStats labelQuery(const std::string &aql_query, LabelingCallback callback)`
- Source: `src/training/auto_labeler.cpp`:329
- Brief: Label Query.
- Parameters:
  - `aql_query` (const std::string &): Input parameter.
  - `callback` (LabelingCallback): Input parameter.
- Return: Return value.
- Details: aql_query Input parameter. callback Input parameter. Return value. Calls: LabelingStats(), std::chrono::steady_clock::now(), empty(), isReadOnlyAqlQuery(), executeAqlQuery(), reserve(), labelDocument(), updateStats().

#### `void persistSampleBatch(const std::vector< TrainingSample > &batch) const`
- Source: `src/training/auto_labeler.cpp`:723
- Brief: n/a
- Parameters:
  - `batch` (const std::vector< TrainingSample > &): n/a

#### `void registerDocument(const std::string &document_id, const std::string &text)`
- Source: `src/training/auto_labeler.cpp`:461
- Brief: Register Document.
- Parameters:
  - `document_id` (const std::string &): Identifier of the document.
  - `text` (const std::string &): Input parameter.
- Details: document_id Identifier of the document. text Input parameter. Calls: lock().

#### `std::vector< std::string > tryDirectKeyQueryFallback(const std::string &aql) const`
- Source: `src/training/auto_labeler.cpp`:491
- Brief: n/a
- Parameters:
  - `aql` (const std::string &): n/a

#### `void updateSampleConfidence(const std::string &sample_id, float new_confidence, const std::string &reviewed_by)`
- Source: `src/training/auto_labeler.cpp`:415
- Brief: Update Sample Confidence.
- Parameters:
  - `sample_id` (const std::string &): Identifier of the sample.
  - `new_confidence` (float): Input parameter.
  - `reviewed_by` (const std::string &): Input parameter.
- Details: sample_id Identifier of the sample. new_confidence Input parameter. reviewed_by Input parameter. Calls: empty(), std::max(), std::min().

#### `void updateStats(LabelingStats &stats, const TrainingSample &sample) const`
- Source: `src/training/auto_labeler.cpp`:732
- Brief: n/a
- Parameters:
  - `stats` (LabelingStats &): n/a
  - `sample` (const TrainingSample &): n/a

#### `~Impl()=default`
- Source: `src/training/auto_labeler.cpp`:109
- Brief: n/a
- Parameters: none

### themis::training::LineageGraph

#### `LineageGraph()=default`
- Source: `include/training/training_interfaces.h`:81
- Brief: n/a
- Parameters: none

### themis::training::LineageNode

#### `LineageNode()=default`
- Source: `include/training/provenance_tracker.h`:63
- Brief: n/a
- Parameters: none

### themis::training::LoRAAdapter

#### `LoRAAdapter(LoRAAdapter &&) noexcept`
- Source: `include/training/lora_adapter.h`:98
- Brief: n/a
- Parameters:
  - `<unnamed>` (LoRAAdapter &&): n/a

#### `LoRAAdapter(const LoRAAdapter &)=delete`
- Source: `include/training/lora_adapter.h`:96
- Brief: n/a
- Parameters:
  - `<unnamed>` (const LoRAAdapter &): n/a

#### `LoRAAdapter(size_t default_rank=4, float default_alpha=8.0f)`
- Source: `include/training/lora_adapter.h`:92
- Brief: Construct a LoRA adapter.
- Parameters:
  - `default_rank` (size_t): Default LoRA rank for addLayer() calls (> 0)
  - `default_alpha` (float): Default LoRA alpha for addLayer() calls (> 0)
- Details: default_rank Default LoRA rank for addLayer() calls (> 0) default_alpha Default LoRA alpha for addLayer() calls (> 0)

#### `void addLayer(const std::string &layer_name, size_t in_dim, size_t out_dim, size_t rank=0, float alpha=0.0f)`
- Source: `include/training/lora_adapter.h`:119
- Brief: Register a new LoRA adapter layer.
- Parameters:
  - `layer_name` (const std::string &): Name of the layer.
  - `in_dim` (size_t): Input parameter.
  - `out_dim` (size_t): Input parameter.
  - `rank` (size_t): Input parameter.
  - `alpha` (float): Input parameter.
- Throws:
  - std::invalid_argument: if any dimension or rank is zero, or if a layer with the same name already exists
- Details: Add Layer. B is Kaiming-uniform initialised; A is zero-initialised so that the initial adapter contribution to the base model is zero. layer_name Unique identifier (must not already exist) in_dim Input feature dimension (> 0) out_dim Output feature dimension (> 0) rank Override rank (0 = use adapter default) alpha Override alpha (0.0 = use adapter default) std::invalid_argument if any dimension or rank is zero, or if a layer with the same name already exists layer_name Name of the layer. in_dim Input parameter. out_dim Input parameter. rank Input parameter. alpha Input parameter. Implements addLayer without additional internal calls.

#### `WeightUpdateResult applyBatchUpdate(const WeightUpdateBatch &batch)`
- Source: `include/training/lora_adapter.h`:195
- Brief: Apply a batch of additive weight deltas atomically.
- Parameters:
  - `batch` (const WeightUpdateBatch &): Input parameter.
- Return: WeightUpdateResult summarising how many layers were updated
- Throws:
  - std::invalid_argument: if the batch vectors have mismatched sizes
- Details: Apply Batch Update. Each (layer_names[i], delta_B[i], delta_A[i]) triple is applied in sequence. Unknown layer names are silently skipped and counted in WeightUpdateResult::layers_skipped. batch Batch of deltas; all three parallel vectors must have the same length WeightUpdateResult summarising how many layers were updated std::invalid_argument if the batch vectors have mismatched sizes batch Input parameter. Return value. Implements applyBatchUpdate without additional internal calls.

#### `WeightUpdateResult applyUpdate(const std::string &layer_name, const std::vector< float > &delta_B, const std::vector< float > &delta_A)`
- Source: `include/training/lora_adapter.h`:179
- Brief: Apply an additive delta to a single layer's B and A matrices.
- Parameters:
  - `layer_name` (const std::string &): Name of the layer.
  - `delta_B` (const std::vector< float > &): Input parameter.
  - `delta_A` (const std::vector< float > &): Input parameter.
- Return: WeightUpdateResult (layers_updated == 1 on success)
- Throws:
  - std::out_of_range: if the layer does not exist
  - std::invalid_argument: if delta sizes are mismatched
- Details: Apply Update. B_new = B + delta_B A_new = A + delta_A layer_name Target layer delta_B Additive delta for B (same size as B) delta_A Additive delta for A (same size as A) WeightUpdateResult (layers_updated == 1 on success) std::out_of_range if the layer does not exist std::invalid_argument if delta sizes are mismatched layer_name Name of the layer. delta_B Input parameter. delta_A Input parameter. Return value. Implements applyUpdate without additional internal calls.

#### `std::vector< LoRAWeightEntry > exportWeights() const`
- Source: `include/training/lora_adapter.h`:228
- Brief: Export copies of all weight entries.
- Parameters: none
- Return: Vector of LoRAWeightEntry (one per registered layer)
- Details: Vector of LoRAWeightEntry (one per registered layer)

#### `std::vector< float > forward(const std::string &layer_name, const std::vector< float > &input, size_t batch_size) const`
- Source: `include/training/lora_adapter.h`:216
- Brief: Compute the LoRA contribution for a given layer and input batch.
- Parameters:
  - `layer_name` (const std::string &): Layer whose weights are applied
  - `input` (const std::vector< float > &): Flat row-major input (batch_size × in_dim)
  - `batch_size` (size_t): Number of rows in input
- Return: Flat row-major output (batch_size × out_dim)
- Throws:
  - std::out_of_range: if the layer does not exist
  - std::invalid_argument: if input size != batch_size × in_dim
- Details: Performs the following (no simulation): hidden = input @ B (batch_size × rank) output = hidden @ A (batch_size × out_dim) return output × scaling where scaling = alpha / rank layer_name Layer whose weights are applied input Flat row-major input (batch_size × in_dim) batch_size Number of rows in input Flat row-major output (batch_size × out_dim) std::out_of_range if the layer does not exist std::invalid_argument if input size != batch_size × in_dim

#### `const LoRAWeightEntry & getWeights(const std::string &layer_name) const`
- Source: `include/training/lora_adapter.h`:151
- Brief: Read-only snapshot of the weight entry for a layer.
- Parameters:
  - `layer_name` (const std::string &): n/a
- Throws:
  - std::out_of_range: if the layer does not exist
- Details: std::out_of_range if the layer does not exist

#### `bool hasLayer(const std::string &layer_name) const`
- Source: `include/training/lora_adapter.h`:132
- Brief: Whether a layer with the given name is registered.
- Parameters:
  - `layer_name` (const std::string &): n/a

#### `void importWeights(const std::vector< LoRAWeightEntry > &entries)`
- Source: `include/training/lora_adapter.h`:239
- Brief: Import a set of weight entries, overwriting matching names.
- Parameters:
  - `entries` (const std::vector< LoRAWeightEntry > &): Input parameter.
- Throws:
  - std::invalid_argument: if any entry has inconsistent sizes
- Details: Import Weights. Entries whose layer_name does not yet exist are added automatically; entries whose layer_name already exists replace the current weights. entries Vector of weight entries to import std::invalid_argument if any entry has inconsistent sizes entries Input parameter. Implements importWeights without additional internal calls.

#### `size_t layerCount() const`
- Source: `include/training/lora_adapter.h`:138
- Brief: Number of registered layers.
- Parameters: none

#### `std::vector< std::string > layerNames() const`
- Source: `include/training/lora_adapter.h`:135
- Brief: Names of all registered layers (order unspecified).
- Parameters: none

#### `LoRAAdapter & operator=(LoRAAdapter &&) noexcept`
- Source: `include/training/lora_adapter.h`:99
- Brief: n/a
- Parameters:
  - `<unnamed>` (LoRAAdapter &&): n/a

#### `LoRAAdapter & operator=(const LoRAAdapter &)=delete`
- Source: `include/training/lora_adapter.h`:97
- Brief: n/a
- Parameters:
  - `<unnamed>` (const LoRAAdapter &): n/a

#### `bool removeLayer(const std::string &layer_name)`
- Source: `include/training/lora_adapter.h`:129
- Brief: Remove a layer entry.
- Parameters:
  - `layer_name` (const std::string &): Name of the layer.
- Return: true if the layer existed and was removed; false otherwise
- Details: Remove Layer. true if the layer existed and was removed; false otherwise layer_name Name of the layer. True when the operation succeeds. Implements removeLayer without additional internal calls.

#### `void setWeights(const std::string &layer_name, const std::vector< float > &B, const std::vector< float > &A)`
- Source: `include/training/lora_adapter.h`:162
- Brief: Overwrite the B and A matrices of an existing layer.
- Parameters:
  - `layer_name` (const std::string &): Name of the layer.
  - `B` (const std::vector< float > &): Input parameter.
  - `A` (const std::vector< float > &): Input parameter.
- Throws:
  - std::out_of_range: if the layer does not exist
  - std::invalid_argument: if the vector sizes do not match
- Details: Set Weights. layer_name Target layer B New flat row-major B data (must be in_dim × rank floats) A New flat row-major A data (must be rank × out_dim floats) std::out_of_range if the layer does not exist std::invalid_argument if the vector sizes do not match layer_name Name of the layer. B Input parameter. A Input parameter. Implements setWeights without additional internal calls.

#### `size_t totalParameterCount() const`
- Source: `include/training/lora_adapter.h`:141
- Brief: Total trainable parameter count across all layers.
- Parameters: none

#### `~LoRAAdapter()`
- Source: `include/training/lora_adapter.h`:94
- Brief: n/a
- Parameters: none

### themis::training::LoRAAdapter::Impl

#### `Impl(size_t default_rank, float default_alpha)`
- Source: `src/training/lora_adapter.cpp`:110
- Brief: Impl.
- Parameters:
  - `default_rank` (size_t): Input parameter.
  - `default_alpha` (float): Input parameter.
- Return: Return value.
- Details: default_rank Input parameter. default_alpha Input parameter. Return value.

#### `void addLayer(const std::string &layer_name, size_t in_dim, size_t out_dim, size_t rank, float alpha)`
- Source: `src/training/lora_adapter.cpp`:133
- Brief: Add Layer.
- Parameters:
  - `layer_name` (const std::string &): Name of the layer.
  - `in_dim` (size_t): Input parameter.
  - `out_dim` (size_t): Input parameter.
  - `rank` (size_t): Input parameter.
  - `alpha` (float): Input parameter.
- Throws:
  - std::invalid_argument: if an error occurs.
- Details: layer_name Name of the layer. in_dim Input parameter. out_dim Input parameter. rank Input parameter. alpha Input parameter. std::invalid_argument if an error occurs. Calls: empty(), count(), std::to_string(), std::min(), detail::kaimingUniform(), detail::seedFromName(), assign(), emplace().

#### `WeightUpdateResult applyBatchUpdate(const WeightUpdateBatch &batch)`
- Source: `src/training/lora_adapter.cpp`:310
- Brief: Apply Batch Update.
- Parameters:
  - `batch` (const WeightUpdateBatch &): Input parameter.
- Return: Return value.
- Throws:
  - std::invalid_argument: if an error occurs.
- Details: batch Input parameter. Return value. std::invalid_argument if an error occurs. Calls: size(), find(), end(), empty().

#### `WeightUpdateResult applyUpdate(const std::string &layer_name, const std::vector< float > &delta_B, const std::vector< float > &delta_A)`
- Source: `src/training/lora_adapter.cpp`:267
- Brief: Apply Update.
- Parameters:
  - `layer_name` (const std::string &): Name of the layer.
  - `delta_B` (const std::vector< float > &): Input parameter.
  - `delta_A` (const std::vector< float > &): Input parameter.
- Return: Return value.
- Throws:
  - std::out_of_range: if an error occurs.
  - std::invalid_argument: if an error occurs.
- Details: layer_name Name of the layer. delta_B Input parameter. delta_A Input parameter. Return value. std::out_of_range if an error occurs. std::invalid_argument if an error occurs. Calls: find(), end(), size(), str().

#### `std::vector< LoRAWeightEntry > exportWeights() const`
- Source: `src/training/lora_adapter.cpp`:399
- Brief: n/a
- Parameters: none

#### `std::vector< float > forward(const std::string &layer_name, const std::vector< float > &input, size_t batch_size) const`
- Source: `src/training/lora_adapter.cpp`:358
- Brief: n/a
- Parameters:
  - `layer_name` (const std::string &): n/a
  - `input` (const std::vector< float > &): n/a
  - `batch_size` (size_t): n/a

#### `const LoRAWeightEntry & getWeights(const std::string &layer_name) const`
- Source: `src/training/lora_adapter.cpp`:209
- Brief: n/a
- Parameters:
  - `layer_name` (const std::string &): n/a

#### `bool hasLayer(const std::string &layer_name) const`
- Source: `src/training/lora_adapter.cpp`:180
- Brief: n/a
- Parameters:
  - `layer_name` (const std::string &): n/a

#### `void importWeights(const std::vector< LoRAWeightEntry > &entries)`
- Source: `src/training/lora_adapter.cpp`:415
- Brief: Import Weights.
- Parameters:
  - `entries` (const std::vector< LoRAWeightEntry > &): Input parameter.
- Throws:
  - std::invalid_argument: if an error occurs.
- Details: entries Input parameter. std::invalid_argument if an error occurs. Calls: size(), str().

#### `size_t layerCount() const`
- Source: `src/training/lora_adapter.cpp`:194
- Brief: n/a
- Parameters: none

#### `std::vector< std::string > layerNames() const`
- Source: `src/training/lora_adapter.cpp`:184
- Brief: n/a
- Parameters: none

#### `bool removeLayer(const std::string &layer_name)`
- Source: `src/training/lora_adapter.cpp`:176
- Brief: Remove Layer.
- Parameters:
  - `layer_name` (const std::string &): Name of the layer.
- Return: True when the operation succeeds.
- Details: layer_name Name of the layer. True when the operation succeeds. Calls: erase().

#### `void setWeights(const std::string &layer_name, const std::vector< float > &B, const std::vector< float > &A)`
- Source: `src/training/lora_adapter.cpp`:225
- Brief: Set Weights.
- Parameters:
  - `layer_name` (const std::string &): Name of the layer.
  - `B` (const std::vector< float > &): Input parameter.
  - `A` (const std::vector< float > &): Input parameter.
- Throws:
  - std::out_of_range: if an error occurs.
  - std::invalid_argument: if an error occurs.
- Details: layer_name Name of the layer. B Input parameter. A Input parameter. std::out_of_range if an error occurs. std::invalid_argument if an error occurs. Calls: find(), end(), size(), str().

#### `size_t totalParameterCount() const`
- Source: `src/training/lora_adapter.cpp`:196
- Brief: n/a
- Parameters: none

### themis::training::LoRAAdapterMerger

#### `LoRAAdapterMerger()=default`
- Source: `include/training/lora_adapter_merger.h`:83
- Brief: n/a
- Parameters: none

#### `MergeLayerResult mergeLinear(const std::vector< AdapterDescriptor > &adapters, const std::string &out_layer, size_t in_dim, size_t out_dim, size_t rank, float alpha=8.0f) const`
- Source: `include/training/lora_adapter_merger.h`:104
- Brief: Linearly combine multiple LoRA adapters for a single output layer.
- Parameters:
  - `adapters` (const std::vector< AdapterDescriptor > &): Adapters to merge (all must have the given layer name)
  - `out_layer` (const std::string &): Name for the output layer in the returned result
  - `in_dim` (size_t): Input dimension of the layer
  - `out_dim` (size_t): Output dimension of the layer
  - `rank` (size_t): Rank for the output B' and A' matrices
  - `alpha` (float): LoRA alpha for the output layer
- Return: MergeLayerResult with merged B and A (in_dim×rank, rank×out_dim)
- Details: Computes the merged weight delta as a weighted sum of the individual ΔW = (B @ A) × scaling contributions, then factorises back to (B', A'). adapters Adapters to merge (all must have the given layer name) out_layer Name for the output layer in the returned result in_dim Input dimension of the layer out_dim Output dimension of the layer rank Rank for the output B' and A' matrices alpha LoRA alpha for the output layer MergeLayerResult with merged B and A (in_dim×rank, rank×out_dim)

#### `MergeResult mergeLinearAll(const std::vector< const LoRAAdapter * > &adapters, const std::vector< float > &weights, size_t output_rank) const`
- Source: `include/training/lora_adapter_merger.h`:122
- Brief: Linearly merge all layers shared by all given adapters.
- Parameters:
  - `adapters` (const std::vector< const LoRAAdapter * > &): Collection of adapters to merge (all non-null)
  - `weights` (const std::vector< float > &): Per-adapter blend weights (must match adapters.size())
  - `output_rank` (size_t): Rank for all output layers
- Return: MergeResult describing the merged layers
- Details: Iterates over the layer names present in the first adapter and merges each layer that exists in all adapters. adapters Collection of adapters to merge (all non-null) weights Per-adapter blend weights (must match adapters.size()) output_rank Rank for all output layers MergeResult describing the merged layers

#### `MergeLayerResult mergeTIES(const std::vector< AdapterDescriptor > &adapters, const std::string &out_layer, size_t in_dim, size_t out_dim, size_t rank, float alpha=8.0f, float trim_threshold=0.2f) const`
- Source: `include/training/lora_adapter_merger.h`:147
- Brief: TIES-merge multiple LoRA adapters for a single output layer.
- Parameters:
  - `adapters` (const std::vector< AdapterDescriptor > &): Adapters to merge
  - `out_layer` (const std::string &): Name for the output layer
  - `in_dim` (size_t): Input dimension
  - `out_dim` (size_t): Output dimension
  - `rank` (size_t): Rank for the output matrices
  - `alpha` (float): LoRA alpha
  - `trim_threshold` (float): Fraction of each adapter's max-abs-value used as the trimming threshold (default: 0.2 → 20%)
- Return: MergeLayerResult with TIES-merged B and A
- Details: Implements the Trim–Resolve–Merge algorithm on the flattened ΔW matrices. The output B' and A' represent the merged delta refactored to rank 1 (or the requested rank via SVD approximation). adapters Adapters to merge out_layer Name for the output layer in_dim Input dimension out_dim Output dimension rank Rank for the output matrices alpha LoRA alpha trim_threshold Fraction of each adapter's max-abs-value used as the trimming threshold (default: 0.2 → 20%) MergeLayerResult with TIES-merged B and A

#### `MergeResult mergeTIESAll(const std::vector< const LoRAAdapter * > &adapters, size_t output_rank, float trim_threshold=0.2f) const`
- Source: `include/training/lora_adapter_merger.h`:163
- Brief: TIES-merge all layers shared by all given adapters.
- Parameters:
  - `adapters` (const std::vector< const LoRAAdapter * > &): Collection of adapters
  - `output_rank` (size_t): Rank for all output layers
  - `trim_threshold` (float): Trimming threshold fraction (default 0.2)
- Return: MergeResult describing the merged layers
- Details: adapters Collection of adapters output_rank Rank for all output layers trim_threshold Trimming threshold fraction (default 0.2) MergeResult describing the merged layers

#### `std::string validateMergeInputs(const std::vector< AdapterDescriptor > &adapters) const`
- Source: `include/training/lora_adapter_merger.h`:184
- Brief: Validate merge inputs before performing a merge operation.
- Parameters:
  - `adapters` (const std::vector< AdapterDescriptor > &): Collection of adapter descriptors to validate
- Return: Empty string if valid; otherwise an error message describing the problem
- Details: Checks that: All adapters are non-null All adapter descriptors reference valid layers No dimension mismatches exist Adapter is not empty (has at least one layer) Weights are normalized and positive adapters Collection of adapter descriptors to validate Empty string if valid; otherwise an error message describing the problem

#### `bool validateMergeResult(const MergeResult &result) const`
- Source: `include/training/lora_adapter_merger.h`:197
- Brief: Phase 2: Validate that a merge result is acceptable.
- Parameters:
  - `result` (const MergeResult &): The merge result to validate
- Return: true if the result is valid and can be deployed
- Details: Checks that: Result matrices have correct dimensions Result matrices contain finite values (no NaN/Inf) Result is marked successful if all layers passed result The merge result to validate true if the result is valid and can be deployed

#### `~LoRAAdapterMerger()=default`
- Source: `include/training/lora_adapter_merger.h`:84
- Brief: n/a
- Parameters: none

### themis::training::LoRACheckpointManager

#### `LoRACheckpointManager(LoRACheckpointManager &&) noexcept=default`
- Source: `include/training/lora_checkpoint_manager.h`:150
- Brief: n/a
- Parameters:
  - `<unnamed>` (LoRACheckpointManager &&): n/a

#### `LoRACheckpointManager(const CheckpointManagerConfig &config)`
- Source: `include/training/lora_checkpoint_manager.h`:143
- Brief: Construct the checkpoint manager.
- Parameters:
  - `config` (const CheckpointManagerConfig &): Configuration for directory, window size, and validation.
- Throws:
  - CheckpointException: if checkpoint_dir is empty or path is unsafe (error code: CHECKPOINT_DIR_INVALID or CHECKPOINT_PATH_UNSAFE)
- Details: config Configuration for directory, window size, and validation. CheckpointException if checkpoint_dir is empty or path is unsafe (error code: CHECKPOINT_DIR_INVALID or CHECKPOINT_PATH_UNSAFE)

#### `LoRACheckpointManager(const LoRACheckpointManager &)=delete`
- Source: `include/training/lora_checkpoint_manager.h`:148
- Brief: n/a
- Parameters:
  - `<unnamed>` (const LoRACheckpointManager &): n/a

#### `size_t auditCheckpoints(std::string *diagnostics=nullptr)`
- Source: `include/training/lora_checkpoint_manager.h`:256
- Brief: Phase 2: Verify all checkpoints in the manifest and report status.
- Parameters:
  - `diagnostics` (std::string *): Input/output parameter.
- Return: Number of valid checkpoints found
- Details: Audit Checkpoints. Performs a full audit of the checkpoint directory: validates each entry, reports which are valid/corrupt, and optionally removes corrupt entries. diagnostics Optional string to receive audit results Number of valid checkpoints found diagnostics Input/output parameter. Return value. Implements auditCheckpoints without additional internal calls.

#### `size_t cleanupPartialCheckpoints()`
- Source: `include/training/lora_checkpoint_manager.h`:245
- Brief: Phase 2: Clean up partial/corrupted checkpoints in the directory.
- Parameters: none
- Return: Number of files cleaned up
- Details: Cleanup Partial Checkpoints. Removes checkpoint files that are not in the manifest, or checkpoint files that fail validation and are marked for cleanup in the config. This helps recover disk space and maintain a clean checkpoint directory. Number of files cleaned up Return value. Implements cleanupPartialCheckpoints without additional internal calls.

#### `void clearAll()`
- Source: `include/training/lora_checkpoint_manager.h`:210
- Brief: Delete all checkpoints and clear the manifest.
- Parameters: none
- Details: Clear All. Implements clearAll without additional internal calls.

#### `std::vector< CheckpointManifestEntry > listCheckpoints() const`
- Source: `include/training/lora_checkpoint_manager.h`:198
- Brief: Return all manifest entries, newest first.
- Parameters: none

#### `std::string loadCalibrationJson() const`
- Source: `include/training/lora_checkpoint_manager.h`:234
- Brief: Load the calibration manifest from the checkpoint directory.
- Parameters: none
- Return: Contents of calibration_manifest.json, or empty string if not present.
- Details: Contents of calibration_manifest.json, or empty string if not present.

#### `std::string manifestPath() const`
- Source: `include/training/lora_checkpoint_manager.h`:215
- Brief: Return the path to the manifest JSON file.
- Parameters: none

#### `LoRACheckpointManager & operator=(LoRACheckpointManager &&) noexcept=default`
- Source: `include/training/lora_checkpoint_manager.h`:151
- Brief: n/a
- Parameters:
  - `<unnamed>` (LoRACheckpointManager &&): n/a

#### `LoRACheckpointManager & operator=(const LoRACheckpointManager &)=delete`
- Source: `include/training/lora_checkpoint_manager.h`:149
- Brief: n/a
- Parameters:
  - `<unnamed>` (const LoRACheckpointManager &): n/a

#### `std::optional< CheckpointManifestEntry > resume() const`
- Source: `include/training/lora_checkpoint_manager.h`:179
- Brief: Validate and return the latest valid checkpoint entry.
- Parameters: none
- Return: Latest valid manifest entry, or std::nullopt.
- Details: If validate_on_load is true the SHA-256 of the latest checkpoint is verified. On mismatch, if auto_rollback is true the previous entry is tried and a WARN log is emitted. Returns std::nullopt if no valid checkpoint exists. Latest valid manifest entry, or std::nullopt.

#### `std::optional< CheckpointManifestEntry > resumeWithDiagnostics(std::string *diagnostics=nullptr) const`
- Source: `include/training/lora_checkpoint_manager.h`:192
- Brief: Phase 2: Recover a checkpoint with detailed diagnostics.
- Parameters:
  - `diagnostics` (std::string *): Optional string to receive detailed recovery info
- Return: Latest valid manifest entry, or std::nullopt if recovery fails
- Details: Attempts to recover a checkpoint from the manifest, returning full diagnostic information about the recovery process (which checkpoints were attempted, why they failed, etc.). This is useful for understanding checkpoint corruption or rollback behavior. diagnostics Optional string to receive detailed recovery info Latest valid manifest entry, or std::nullopt if recovery fails

#### `CheckpointManifestEntry save(const std::string &source_path, CheckpointManifestEntry meta)`
- Source: `include/training/lora_checkpoint_manager.h`:166
- Brief: Atomically save a checkpoint with integrity metadata.
- Parameters:
  - `source_path` (const std::string &): Path to the source.
  - `meta` (CheckpointManifestEntry): Input parameter.
- Return: Manifest entry with the final path and computed SHA-256.
- Throws:
  - std::runtime_error: on I/O failure.
- Details: Save. Reads source_path, copies it to checkpoint_dir/<filename>.tmp, computes its SHA-256, renames to the final path, then updates the manifest. The oldest entry is pruned once the rolling window overflows. source_path Path to the adapter weights file to checkpoint. meta Metadata to store in the manifest (sha256 is computed automatically and must not be pre-filled). Manifest entry with the final path and computed SHA-256. std::runtime_error on I/O failure. source_path Path to the source. meta Input parameter. Return value. Calls: std::move().

#### `void saveCalibrationJson(const std::string &json_content)`
- Source: `include/training/lora_checkpoint_manager.h`:228
- Brief: Persist a calibration result as calibration_manifest.json in the checkpoint directory.
- Parameters:
  - `json_content` (const std::string &): Input parameter.
- Throws:
  - std::runtime_error: on I/O failure.
- Details: Save Calibration Json. The calibration manifest is written alongside adapter weights so that ConfidenceCalibrator thresholds are always co-located with the checkpoint they were derived from. json_content Serialised calibration result (key=value or JSON string). std::runtime_error on I/O failure. json_content Input parameter. Implements saveCalibrationJson without additional internal calls.

#### `bool validate(const CheckpointManifestEntry &entry) const`
- Source: `include/training/lora_checkpoint_manager.h`:205
- Brief: Validate the integrity of a specific checkpoint file.
- Parameters:
  - `entry` (const CheckpointManifestEntry &): Manifest entry to validate.
- Return: true if the file exists and its SHA-256 matches the stored digest.
- Details: entry Manifest entry to validate. true if the file exists and its SHA-256 matches the stored digest.

#### `~LoRACheckpointManager()`
- Source: `include/training/lora_checkpoint_manager.h`:145
- Brief: n/a
- Parameters: none

### themis::training::LoRACheckpointManager::Impl

#### `Impl(const CheckpointManagerConfig &config)`
- Source: `src/training/lora_checkpoint_manager.cpp`:172
- Brief: Impl.
- Parameters:
  - `config` (const CheckpointManagerConfig &): Input parameter.
- Return: Return value.
- Details: config Input parameter. Return value.

#### `size_t auditCheckpoints(std::string *diagnostics)`
- Source: `src/training/lora_checkpoint_manager.cpp`:393
- Brief: Phase 2: Audit all checkpoints.
- Parameters:
  - `diagnostics` (std::string *): Input/output parameter.
- Return: Return value.
- Details: diagnostics Input/output parameter. Return value. Calls: size(), validate(), str().

#### `std::string calibrationManifestPath() const`
- Source: `src/training/lora_checkpoint_manager.cpp`:317
- Brief: n/a
- Parameters: none

#### `size_t cleanupPartialCheckpoints()`
- Source: `src/training/lora_checkpoint_manager.cpp`:358
- Brief: Phase 2: Clean up partial/corrupted checkpoints.
- Parameters: none
- Return: Return value.
- Details: Return value. Calls: insert(), validate(), std::remove(), c_str().

#### `void clearAll()`
- Source: `src/training/lora_checkpoint_manager.cpp`:302
- Brief: Clear All.
- Parameters: none
- Details: Calls: std::remove(), c_str(), clear(), manifestPath(), calibrationManifestPath().

#### `std::vector< CheckpointManifestEntry > listCheckpoints() const`
- Source: `src/training/lora_checkpoint_manager.cpp`:284
- Brief: n/a
- Parameters: none

#### `std::string loadCalibrationJson() const`
- Source: `src/training/lora_checkpoint_manager.cpp`:343
- Brief: n/a
- Parameters: none

#### `void loadManifest()`
- Source: `src/training/lora_checkpoint_manager.cpp`:427
- Brief: Load Manifest.
- Parameters: none
- Details: Calls: f(), manifestPath(), is_open(), rdbuf(), parseManifest(), str().

#### `std::string manifestPath() const`
- Source: `src/training/lora_checkpoint_manager.cpp`:312
- Brief: n/a
- Parameters: none

#### `void persistManifest() const`
- Source: `src/training/lora_checkpoint_manager.cpp`:436
- Brief: n/a
- Parameters: none

#### `std::optional< CheckpointManifestEntry > resume() const`
- Source: `src/training/lora_checkpoint_manager.cpp`:258
- Brief: n/a
- Parameters: none

#### `CheckpointManifestEntry save(const std::string &source_path, CheckpointManifestEntry meta)`
- Source: `src/training/lora_checkpoint_manager.cpp`:193
- Brief: Save.
- Parameters:
  - `source_path` (const std::string &): Path to the source.
  - `meta` (CheckpointManifestEntry): Input parameter.
- Return: Return value.
- Throws:
  - std::invalid_argument: if an error occurs.
  - std::runtime_error: if an error occurs.
- Details: source_path Path to the source. meta Input parameter. Return value. std::invalid_argument if an error occurs. std::runtime_error if an error occurs. Calls: empty(), std::to_string(), copyFile(), utils::calculateSHA256(), std::remove(), c_str(), std::rename(), std::time().

#### `void saveCalibrationJson(const std::string &json_content)`
- Source: `src/training/lora_checkpoint_manager.cpp`:328
- Brief: Save Calibration Json.
- Parameters:
  - `json_content` (const std::string &): Input parameter.
- Throws:
  - std::runtime_error: if an error occurs.
- Details: json_content Input parameter. std::runtime_error if an error occurs. Calls: f(), calibrationManifestPath(), is_open(), good().

#### `bool validate(const CheckpointManifestEntry &entry) const`
- Source: `src/training/lora_checkpoint_manager.cpp`:289
- Brief: n/a
- Parameters:
  - `entry` (const CheckpointManifestEntry &): n/a

### themis::training::LoRADataSelectionConfig

#### `LoRADataSelectionConfig()=default`
- Source: `include/training/lora_data_selection.h`:84
- Brief: n/a
- Parameters: none

#### `LoRADataSelectionConfig fromYAMLString(const std::string &yaml_text, const std::string &section="lora_data_selection")`
- Source: `include/training/lora_data_selection.h`:111
- Brief: Parse configuration from an in-memory YAML string.
- Parameters:
  - `yaml_text` (const std::string &): Input parameter.
  - `section` (const std::string &): Input parameter.
- Return: Return value.
- Details: From YAMLString. Useful for unit testing or when the YAML content is already loaded. yaml_text YAML text containing the data-selection section. section Top-level key to read (default: lora_data_selection). yaml_text Input parameter. section Input parameter. Return value. Calls: yaml_detail::parseYAMLText().

#### `LoRADataSelectionConfig loadFromYAML(const std::string &path, const std::string &section="lora_data_selection")`
- Source: `include/training/lora_data_selection.h`:99
- Brief: Load configuration from a YAML file.
- Parameters:
  - `path` (const std::string &): Input parameter.
  - `section` (const std::string &): Input parameter.
- Return: Return value.
- Throws:
  - std::runtime_error: if the file cannot be opened.
  - std::runtime_error: if an error occurs.
- Details: Load From YAML. Reads the section block (default: lora_data_selection) from the YAML file at path and fills a new config object. Uses a built-in line-by-line parser – no external yaml-cpp dependency. Supports live-reload: call again at any time to obtain an updated config. path Path to the YAML configuration file. section Top-level YAML key containing the data-selection block. std::runtime_error if the file cannot be opened. path Input parameter. section Input parameter. Return value. std::runtime_error if an error occurs. Calls: f(), is_open(), rdbuf(), yaml_detail::parseYAMLText(), str().

### themis::training::LoRAWeights

#### `LoRAWeights()=default`
- Source: `include/training/training_interfaces.h`:42
- Brief: n/a
- Parameters: none

### themis::training::MergeException

#### `MergeException(const std::string &message, TrainingErrorCode code=TrainingErrorCode::MERGE_NO_ADAPTERS, bool recoverable=false, const std::string &context="")`
- Source: `include/training/training_exceptions.h`:155
- Brief: n/a
- Parameters:
  - `message` (const std::string &): n/a
  - `code` (TrainingErrorCode): n/a
  - `recoverable` (bool): n/a
  - `context` (const std::string &): n/a

### themis::training::ModalityDetector

#### `ModalityDetector(ModalityDetector &&) noexcept=default`
- Source: `include/training/modality_parser.h`:260
- Brief: n/a
- Parameters:
  - `<unnamed>` (ModalityDetector &&): n/a

#### `ModalityDetector(const ModalityDetector &)=delete`
- Source: `include/training/modality_parser.h`:258
- Brief: n/a
- Parameters:
  - `<unnamed>` (const ModalityDetector &): n/a

#### `ModalityDetector(const ModalityParserConfig &config)`
- Source: `include/training/modality_parser.h`:253
- Brief: Construct the detector with the given configuration.
- Parameters:
  - `config` (const ModalityParserConfig &): Parser configuration (language, thresholds, OCR flag).
- Details: config Parser configuration (language, thresholds, OCR flag).

#### `ContentModality detectModality(const std::string &content, const std::string &mime_hint="") const`
- Source: `include/training/modality_parser.h`:276
- Brief: Heuristically detect the dominant modality of content.
- Parameters:
  - `content` (const std::string &): Raw document content (text or binary).
  - `mime_hint` (const std::string &): Optional MIME-type string to guide detection.
- Return: Dominant ContentModality of the content.
- Details: Checks, in order: mime_hint ("image/..." → OCR_IMAGE if enabled) Table density (pipe characters, aligned columns) Citation density (§, court-decision patterns) Falls back to TEXT_CLAUSE content Raw document content (text or binary). mime_hint Optional MIME-type string to guide detection. Dominant ContentModality of the content.

#### `ModalityDetector & operator=(ModalityDetector &&) noexcept=default`
- Source: `include/training/modality_parser.h`:261
- Brief: n/a
- Parameters:
  - `<unnamed>` (ModalityDetector &&): n/a

#### `ModalityDetector & operator=(const ModalityDetector &)=delete`
- Source: `include/training/modality_parser.h`:259
- Brief: n/a
- Parameters:
  - `<unnamed>` (const ModalityDetector &): n/a

#### `ModalityParseStats parseBatch(const std::vector< std::pair< std::string, std::string > > &documents, std::vector< TrainingSample > &out_samples) const`
- Source: `include/training/modality_parser.h`:307
- Brief: Extract training samples from a batch of documents.
- Parameters:
  - `documents` (const std::vector< std::pair< std::string, std::string > > &): Pairs of (content, document_id).
  - `out_samples` (std::vector< TrainingSample > &): Output vector; samples are appended (not replaced).
- Return: Aggregated statistics across the entire batch.
- Details: Calls parseDocument() for each entry in documents and appends all produced samples to out_samples. documents Pairs of (content, document_id). out_samples Output vector; samples are appended (not replaced). Aggregated statistics across the entire batch.

#### `ModalityParseResult parseDocument(const std::string &content, const std::string &document_id, const std::string &mime_hint="") const`
- Source: `include/training/modality_parser.h`:293
- Brief: Extract modality-typed training samples from a single document.
- Parameters:
  - `content` (const std::string &): Raw document text (or image path when mime_hint is "image/tiff" etc. for OCR-only documents.
  - `document_id` (const std::string &): Source document identifier for provenance tracing.
  - `mime_hint` (const std::string &): Optional MIME-type hint; pass "image/tiff" etc. for OCR-only documents.
- Return: Parse result with extracted samples and statistics.
- Details: Runs all applicable extractors (text, table, citation, OCR) and aggregates results into a single ModalityParseResult. Per-modality extraction statistics are embedded in the result. content Raw document text (or image path when mime_hint is "image/tiff" etc. for OCR-only documents. document_id Source document identifier for provenance tracing. mime_hint Optional MIME-type hint; pass "image/tiff" etc. for OCR-only documents. Parse result with extracted samples and statistics.

#### `~ModalityDetector()`
- Source: `include/training/modality_parser.h`:255
- Brief: n/a
- Parameters: none

### themis::training::ModalityDetector::Impl

#### `Impl(const ModalityParserConfig &config)`
- Source: `src/training/modality_parser.cpp`:573
- Brief: Impl.
- Parameters:
  - `config` (const ModalityParserConfig &): Input parameter.
- Return: Return value.
- Details: config Input parameter. Return value.

#### `ContentModality detectModality(const std::string &content, const std::string &mime_hint) const`
- Source: `src/training/modality_parser.cpp`:582
- Brief: n/a
- Parameters:
  - `content` (const std::string &): n/a
  - `mime_hint` (const std::string &): n/a

#### `ModalityParseStats parseBatch(const std::vector< std::pair< std::string, std::string > > &documents, std::vector< TrainingSample > &out_samples) const`
- Source: `src/training/modality_parser.cpp`:690
- Brief: n/a
- Parameters:
  - `documents` (const std::vector< std::pair< std::string, std::string > > &): n/a
  - `out_samples` (std::vector< TrainingSample > &): n/a

#### `ModalityParseResult parseDocument(const std::string &content, const std::string &document_id, const std::string &mime_hint) const`
- Source: `src/training/modality_parser.cpp`:630
- Brief: n/a
- Parameters:
  - `content` (const std::string &): n/a
  - `document_id` (const std::string &): n/a
  - `mime_hint` (const std::string &): n/a

### themis::training::ModalityParseResult

#### `ModalityParseResult()=default`
- Source: `include/training/modality_parser.h`:89
- Brief: n/a
- Parameters: none

### themis::training::ModalityParseStats

#### `ModalityParseStats()=default`
- Source: `include/training/modality_parser.h`:76
- Brief: n/a
- Parameters: none

### themis::training::ModalityParserConfig

#### `ModalityParserConfig()=default`
- Source: `include/training/modality_parser.h`:57
- Brief: n/a
- Parameters: none

### themis::training::MultiTaskLoRATrainer

#### `MultiTaskLoRATrainer(MultiTaskLoRAConfig cfg={})`
- Source: `include/training/multi_task_lora.h`:150
- Brief: n/a
- Parameters:
  - `cfg` (MultiTaskLoRAConfig): n/a

#### `void addTask(const TaskConfig &task)`
- Source: `include/training/multi_task_lora.h`:172
- Brief: Register a task.
- Parameters:
  - `task` (const TaskConfig &): Input parameter.
- Throws:
  - std::invalid_argument: if any configuration guard fails (QW-41): if task.id is empty if task.task_rank is 0 (must be >= 1) if task.learning_rate <= 0 (must be > 0) if task.loss_weight < 0 (must be >= 0)
- Details: Add Task. Must be called before train(). Duplicate task ids are silently ignored (first registration wins). task Task configuration. std::invalid_argument if any configuration guard fails (QW-41): if task.id is empty if task.task_rank is 0 (must be >= 1) if task.learning_rate <= 0 (must be > 0) if task.loss_weight < 0 (must be >= 0) Fail-closed: On guard failure, exception is thrown and task is not added. Calling code must handle the exception; task registration fails atomically. task Input parameter. Implements addTask without additional internal calls.

#### `MTLTrainResult benchmarkThreeTaskTransfer(size_t num_samples=100)`
- Source: `include/training/multi_task_lora.h`:264
- Brief: Run a three-task transfer evaluation benchmark for Wave B.
- Parameters:
  - `num_samples` (size_t): Input parameter.
- Return: MTLTrainResult with comprehensive metrics and gates.
- Details: Benchmark Three Task Transfer. Implements the Wave B benchmark harness: Create three synthetic tasks (semantic similarity task, sentiment, QA) Measure baseline single-task performance Measure multi-task performance with shared base Calculate task interference and gating effectiveness num_samples Number of samples per task for evaluation. MTLTrainResult with comprehensive metrics and gates. num_samples Input parameter. Return value. Implements benchmarkThreeTaskTransfer without additional internal calls.

#### `const MultiTaskLoRAConfig & config() const noexcept`
- Source: `include/training/multi_task_lora.h`:283
- Brief: n/a
- Parameters: none

#### `std::vector< float > exportSharedWeights() const`
- Source: `include/training/multi_task_lora.h`:223
- Brief: Export the shared LoRA base weight matrix (B × A product).
- Parameters: none
- Return: Flat row-major weight matrix of size (input_dim × shared_rank). Returns empty vector if not trained.
- Details: Flat row-major weight matrix of size (input_dim × shared_rank). Returns empty vector if not trained.

#### `std::vector< float > exportTaskWeights(const std::string &task_id) const`
- Source: `include/training/multi_task_lora.h`:232
- Brief: Export the task-specific head weights for a given task.
- Parameters:
  - `task_id` (const std::string &): Task identifier.
- Return: Flat row-major weight matrix of size (shared_rank × output_dim). Returns empty vector if not trained or unknown task.
- Details: task_id Task identifier. Flat row-major weight matrix of size (shared_rank × output_dim). Returns empty vector if not trained or unknown task.

#### `std::vector< float > forward(const std::vector< float > &input) const`
- Source: `include/training/multi_task_lora.h`:211
- Brief: Run the shared base + predicted task head on an input.
- Parameters:
  - `input` (const std::vector< float > &): Input feature vector.
- Return: Output from the gated task head.
- Details: input Input feature vector. Output from the gated task head.

#### `DomainGatingResult inferTask(const std::vector< float > &input) const`
- Source: `include/training/multi_task_lora.h`:203
- Brief: Run domain-gating to predict which task an input belongs to.
- Parameters:
  - `input` (const std::vector< float > &): Input feature vector.
- Return: Gating result with predicted task and confidence.
- Throws:
  - std::runtime_error: if model has not been trained.
- Details: Uses a learned (or heuristic) gate trained during train(). input Input feature vector. Gating result with predicted task and confidence. std::runtime_error if model has not been trained.

#### `std::pair< MTLTrainResult, MTLTrainResult > runAblationStudy(const std::vector< MTLSample > &samples)`
- Source: `include/training/multi_task_lora.h`:276
- Brief: Run ablation study comparing shared multi-task training vs per-task single-task baselines.
- Parameters:
  - `samples` (const std::vector< MTLSample > &): Training samples for evaluation.
- Return: Pair of (shared_result, baseline_result) for comparison.
- Details: Compares two training configurations: Shared base with per-task heads (current implementation) One independently trained single-task model per task, aggregated as the baseline samples Training samples for evaluation. Pair of (shared_result, baseline_result) for comparison.

#### `size_t taskCount() const`
- Source: `include/training/multi_task_lora.h`:175
- Brief: Return the number of registered tasks.
- Parameters: none

#### `MTLTrainResult train(const std::vector< MTLSample > &samples)`
- Source: `include/training/multi_task_lora.h`:188
- Brief: Run multi-task LoRA training on the supplied samples.
- Parameters:
  - `samples` (const std::vector< MTLSample > &): Input parameter.
- Return: Training result with per-task metrics.
- Throws:
  - std::runtime_error: if no tasks registered or no samples supplied.
- Details: Train. samples Training samples (from one or more tasks). Training result with per-task metrics. std::runtime_error if no tasks registered or no samples supplied. samples Input parameter. Return value. Implements train without additional internal calls.

#### `AcceptanceGateMetrics validateAcceptanceGates() const`
- Source: `include/training/multi_task_lora.h`:250
- Brief: Validate acceptance gates for Wave B deployment.
- Parameters: none
- Return: AcceptanceGateMetrics with measured values and validation status.
- Throws:
  - std::runtime_error: if model has not been trained.
- Details: Runs acceptance gate validation: Average task performance gain ≥ +8% vs single-task baseline Training-time increase ≤ 15% across benchmarked task sets Task routing latency ≤ 10ms Convergence stability across configured task-weight schedules AcceptanceGateMetrics with measured values and validation status. std::runtime_error if model has not been trained.

#### `~MultiTaskLoRATrainer()`
- Source: `include/training/multi_task_lora.h`:151
- Brief: n/a
- Parameters: none

### themis::training::MultiTaskLoRATrainer::Impl

#### `Impl(MultiTaskLoRAConfig cfg)`
- Source: `src/training/multi_task_lora.cpp`:31
- Brief: n/a
- Parameters:
  - `cfg` (MultiTaskLoRAConfig): n/a

#### `void addTask(const TaskConfig &task)`
- Source: `src/training/multi_task_lora.cpp`:43
- Brief: Add Task.
- Parameters:
  - `task` (const TaskConfig &): Input parameter.
- Throws:
  - std::invalid_argument: if an error occurs.
- Details: task Input parameter. std::invalid_argument if an error occurs. Calls: empty(), count(), size(), push_back().

#### `MTLTrainResult benchmarkThreeTaskTransfer(size_t num_samples_per_task=100)`
- Source: `src/training/multi_task_lora.cpp`:416
- Brief: n/a
- Parameters:
  - `num_samples_per_task` (size_t): n/a

#### `std::vector< float > exportSharedWeights() const`
- Source: `src/training/multi_task_lora.cpp`:372
- Brief: n/a
- Parameters: none

#### `std::vector< float > exportTaskWeights(const std::string &task_id) const`
- Source: `src/training/multi_task_lora.cpp`:376
- Brief: n/a
- Parameters:
  - `task_id` (const std::string &): n/a

#### `std::vector< float > forward(const std::vector< float > &input) const`
- Source: `src/training/multi_task_lora.cpp`:346
- Brief: n/a
- Parameters:
  - `input` (const std::vector< float > &): n/a

#### `DomainGatingResult inferTask(const std::vector< float > &input) const`
- Source: `src/training/multi_task_lora.cpp`:296
- Brief: n/a
- Parameters:
  - `input` (const std::vector< float > &): n/a

#### `std::pair< MTLTrainResult, MTLTrainResult > runAblationStudy(const std::vector< MTLSample > &samples)`
- Source: `src/training/multi_task_lora.cpp`:487
- Brief: n/a
- Parameters:
  - `samples` (const std::vector< MTLSample > &): n/a

#### `size_t taskCount() const`
- Source: `src/training/multi_task_lora.cpp`:57
- Brief: n/a
- Parameters: none

#### `MTLTrainResult train(const std::vector< MTLSample > &samples)`
- Source: `src/training/multi_task_lora.cpp`:71
- Brief: Train.
- Parameters:
  - `samples` (const std::vector< MTLSample > &): Input parameter.
- Return: Return value.
- Throws:
  - std::runtime_error: if an error occurs.
  - std::invalid_argument: if an error occurs.
- Details: samples Input parameter. Return value. std::runtime_error if an error occurs. std::invalid_argument if an error occurs. Calls: empty(), size(), rng(), init(), assign(), resize(), std::min(), count().

#### `AcceptanceGateMetrics validateAcceptanceGates() const`
- Source: `src/training/multi_task_lora.cpp`:387
- Brief: n/a
- Parameters: none

### themis::training::OCRExtractor

#### `OCRExtractor(const ModalityParserConfig &config)`
- Source: `include/training/modality_parser.h`:209
- Brief: n/a
- Parameters:
  - `config` (const ModalityParserConfig &): n/a

#### `std::vector< TrainingSample > extract(const std::string &image_path, const std::string &document_id) const`
- Source: `include/training/modality_parser.h`:224
- Brief: Run OCR on an image file and return extracted samples.
- Parameters:
  - `image_path` (const std::string &): Path to TIFF / PNG / JPEG image.
  - `document_id` (const std::string &): Source document identifier for provenance.
- Return: Vector of OCR_IMAGE-typed TrainingSample records, or empty if OCR is unavailable.
- Details: image_path Path to TIFF / PNG / JPEG image. document_id Source document identifier for provenance. Vector of OCR_IMAGE-typed TrainingSample records, or empty if OCR is unavailable.

#### `bool isAvailable() const noexcept`
- Source: `include/training/modality_parser.h`:214
- Brief: Returns true when OCR support is compiled in and initialised.
- Parameters: none

### themis::training::OriginRecord

#### `OriginRecord()=default`
- Source: `include/training/training_interfaces.h`:210
- Brief: n/a
- Parameters: none

### themis::training::PipelineConfig

#### `PipelineConfig()=default`
- Source: `include/training/training_pipeline.h`:297
- Brief: n/a
- Parameters: none

### themis::training::PipelineMetrics

#### `void beginStage(const std::string &name)`
- Source: `src/training/training_pipeline.cpp`:68
- Brief: Begin Stage.
- Parameters:
  - `name` (const std::string &): Input parameter.
- Details: name Input parameter. Calls: std::chrono::steady_clock::now().

#### `void endStage(const std::string &name)`
- Source: `src/training/training_pipeline.cpp`:77
- Brief: End Stage.
- Parameters:
  - `name` (const std::string &): Input parameter.
- Details: name Input parameter. Calls: std::chrono::steady_clock::now(), count().

#### `double totalElapsed() const`
- Source: `src/training/training_pipeline.cpp`:83
- Brief: n/a
- Parameters: none

### themis::training::PipelineStats

#### `PipelineStats()=default`
- Source: `include/training/training_pipeline.h`:72
- Brief: n/a
- Parameters: none

### themis::training::PreprocessingStep

#### `PreprocessingStep()=default`
- Source: `include/training/training_interfaces.h`:62
- Brief: n/a
- Parameters: none

### themis::training::ProvenanceException

#### `ProvenanceException(const std::string &message, TrainingErrorCode code=TrainingErrorCode::PROVENANCE_RECORD_INVALID, bool recoverable=false, const std::string &context="")`
- Source: `include/training/training_exceptions.h`:220
- Brief: n/a
- Parameters:
  - `message` (const std::string &): n/a
  - `code` (TrainingErrorCode): n/a
  - `recoverable` (bool): n/a
  - `context` (const std::string &): n/a

### themis::training::ProvenanceRecord

#### `ProvenanceRecord()=default`
- Source: `include/training/provenance_tracker.h`:51
- Brief: n/a
- Parameters: none

### themis::training::ProvenanceTracker

#### `ProvenanceTracker(const ProvenanceTracker &)=delete`
- Source: `include/training/provenance_tracker.h`:135
- Brief: n/a
- Parameters:
  - `<unnamed>` (const ProvenanceTracker &): n/a

#### `ProvenanceTracker(const ProvenanceTrackerConfig &config, const std::string &db_connection, query::QueryEngine *engine=nullptr)`
- Source: `include/training/provenance_tracker.h`:128
- Brief: Construct the tracker.
- Parameters:
  - `config` (const ProvenanceTrackerConfig &): Tracker configuration.
  - `db_connection` (const std::string &): Database connection string (ArangoDB endpoint).
  - `engine` (query::QueryEngine *): Optional AQL query engine. When non-null, write() persists vertices and edges via AQL INSERT statements and queryLineage() traverses the live graph via AQL. Pass nullptr (the default) to operate in offline / test mode, where the in-process store is used.
- Details: config Tracker configuration. db_connection Database connection string (ArangoDB endpoint). engine Optional AQL query engine. When non-null, write() persists vertices and edges via AQL INSERT statements and queryLineage() traverses the live graph via AQL. Pass nullptr (the default) to operate in offline / test mode, where the in-process store is used.

#### `ProvenanceRecord getRecord(const std::string &sample_id) const`
- Source: `include/training/provenance_tracker.h`:184
- Brief: Return the provenance record for a specific sample (if persisted).
- Parameters:
  - `sample_id` (const std::string &): Sample key.
- Return: Provenance record, or empty record if not found.
- Details: sample_id Sample key. Provenance record, or empty record if not found.

#### `ProvenanceTracker & operator=(const ProvenanceTracker &)=delete`
- Source: `include/training/provenance_tracker.h`:136
- Brief: n/a
- Parameters:
  - `<unnamed>` (const ProvenanceTracker &): n/a

#### `LineageNode queryLineage(const std::string &model_id, size_t max_hops=10) const`
- Source: `include/training/provenance_tracker.h`:176
- Brief: Query the lineage graph, tracing a model back to source documents.
- Parameters:
  - `model_id` (const std::string &): Model or adapter version to start the traversal from.
  - `max_hops` (size_t): Maximum number of edge hops (default 10).
- Return: Root lineage node for the model with populated parent chain.
- Details: Traverses DerivedFrom edges up to max_hops hops starting from the given model node, returning a recursive lineage tree. model_id Model or adapter version to start the traversal from. max_hops Maximum number of edge hops (default 10). Root lineage node for the model with populated parent chain.

#### `void recordFilteredSample(const std::string &sample_id, const std::string &category, float confidence, float threshold_used)`
- Source: `include/training/provenance_tracker.h`:161
- Brief: Record an audit event for a sample filtered out by the confidence threshold.
- Parameters:
  - `sample_id` (const std::string &): Identifier of the sample.
  - `category` (const std::string &): Input parameter.
  - `confidence` (float): Input parameter.
  - `threshold_used` (float): Input parameter.
- Details: Record Filtered Sample. Emits a structured event to utils/audit_logger containing the sample ID, category, confidence score, and applied threshold. sample_id ID of the rejected sample. category Legal category of the sample. confidence Confidence score that caused rejection. threshold_used Category threshold applied at the time of rejection. sample_id Identifier of the sample. category Input parameter. confidence Input parameter. threshold_used Input parameter. Implements recordFilteredSample without additional internal calls.

#### `void setQueryEngine(query::QueryEngine *engine)`
- Source: `include/training/provenance_tracker.h`:202
- Brief: Inject or replace the AQL query engine after construction.
- Parameters:
  - `engine` (query::QueryEngine *): Input/output parameter.
- Details: Set Query Engine. Allows server bootstrap code to wire a live QueryEngine into an already-constructed ProvenanceTracker without recreating it. When engine is non-null, subsequent write() calls persist vertices and edges via AQL INSERT, and queryLineage() traverses the live graph via AQL. Pass nullptr to revert to offline/test mode (in-process store only). Thread safety: not thread-safe with respect to concurrent write() or queryLineage() calls; call this method before first use or while no other threads are accessing the tracker. engine Non-owning pointer to the AQL query engine; may be null. engine Input/output parameter. Implements setQueryEngine without additional internal calls.

#### `ProvenanceWriteStats write(const std::vector< ProvenanceRecord > &records)`
- Source: `include/training/provenance_tracker.h`:148
- Brief: Persist provenance records to the AQL graph.
- Parameters:
  - `records` (const std::vector< ProvenanceRecord > &): Input parameter.
- Return: Write statistics.
- Details: Write. Writes a TrainingSample vertex and a DerivedFrom edge for each record. Records without source_doc_urn are rejected when reject_without_urn is true. records Provenance records to persist. Write statistics. records Input parameter. Return value. Implements write without additional internal calls.

#### `~ProvenanceTracker()`
- Source: `include/training/provenance_tracker.h`:132
- Brief: n/a
- Parameters: none

### themis::training::ProvenanceTracker::Impl

#### `Impl(const ProvenanceTrackerConfig &config, const std::string &db_connection, query::QueryEngine *engine)`
- Source: `src/training/provenance_tracker.cpp`:78
- Brief: Impl.
- Parameters:
  - `config` (const ProvenanceTrackerConfig &): Input parameter.
  - `db_connection` (const std::string &): Input parameter.
  - `engine` (query::QueryEngine *): Input/output parameter.
- Return: Return value.
- Details: config Input parameter. db_connection Input parameter. engine Input/output parameter. Return value.

#### `const std::vector< std::string > & auditLog() const`
- Source: `src/training/provenance_tracker.cpp`:386
- Brief: n/a
- Parameters: none

#### `std::string buildQuery(const std::string &tmpl, const std::vector< std::pair< std::string, std::string > > &bindings)`
- Source: `src/training/provenance_tracker.cpp`:399
- Brief: n/a
- Parameters:
  - `tmpl` (const std::string &): n/a
  - `bindings` (const std::vector< std::pair< std::string, std::string > > &): n/a

#### `std::string escapedStr(const std::string &raw)`
- Source: `src/training/provenance_tracker.cpp`:421
- Brief: Escape characters that would break an AQL inline string literal.
- Parameters:
  - `raw` (const std::string &): Input parameter.
- Return: Return value.
- Details: raw Input parameter. Return value. Calls: reserve(), size().

#### `ProvenanceRecord getRecord(const std::string &sample_id) const`
- Source: `src/training/provenance_tracker.cpp`:331
- Brief: n/a
- Parameters:
  - `sample_id` (const std::string &): n/a

#### `LineageNode queryLineage(const std::string &model_id, size_t max_hops) const`
- Source: `src/training/provenance_tracker.cpp`:237
- Brief: n/a
- Parameters:
  - `model_id` (const std::string &): n/a
  - `max_hops` (size_t): n/a

#### `void recordFilteredSample(const std::string &sample_id, const std::string &category, float confidence, float threshold_used)`
- Source: `src/training/provenance_tracker.cpp`:214
- Brief: Record Filtered Sample.
- Parameters:
  - `sample_id` (const std::string &): Identifier of the sample.
  - `category` (const std::string &): Input parameter.
  - `confidence` (float): Input parameter.
  - `threshold_used` (float): Input parameter.
- Details: sample_id Identifier of the sample. category Input parameter. confidence Input parameter. threshold_used Input parameter. Calls: push_back(), str().

#### `void setQueryEngine(query::QueryEngine *engine)`
- Source: `src/training/provenance_tracker.cpp`:381
- Brief: Set Query Engine.
- Parameters:
  - `engine` (query::QueryEngine *): Input/output parameter.
- Details: engine Input/output parameter. Implements setQueryEngine without additional internal calls.

#### `ProvenanceWriteStats write(const std::vector< ProvenanceRecord > &records)`
- Source: `src/training/provenance_tracker.cpp`:93
- Brief: Write.
- Parameters:
  - `records` (const std::vector< ProvenanceRecord > &): Input parameter.
- Return: Return value.
- Details: records Input parameter. Return value. Calls: ProvenanceWriteStats(), std::chrono::steady_clock::now(), std::chrono::milliseconds(), size(), std::min(), empty(), fingerprints_arr(), dump().

### themis::training::ProvenanceTrackerConfig

#### `ProvenanceTrackerConfig()=default`
- Source: `include/training/provenance_tracker.h`:91
- Brief: n/a
- Parameters: none

### themis::training::ProvenanceWriteStats

#### `ProvenanceWriteStats()=default`
- Source: `include/training/provenance_tracker.h`:74
- Brief: n/a
- Parameters: none

### themis::training::QuantizationConfig

#### `QuantizationConfig()=default`
- Source: `include/training/incremental_lora_trainer.h`:66
- Brief: n/a
- Parameters: none

#### `QuantizationConfig(TrainingQuantizationType t, int bs=64)`
- Source: `include/training/incremental_lora_trainer.h`:67
- Brief: n/a
- Parameters:
  - `t` (TrainingQuantizationType): n/a
  - `bs` (int): n/a

### themis::training::SampleProvenance

#### `SampleProvenance()=default`
- Source: `include/training/training_interfaces.h`:73
- Brief: n/a
- Parameters: none

### themis::training::SampleRef

#### `SampleRef()=default`
- Source: `include/training/training_interfaces.h`:199
- Brief: n/a
- Parameters: none

### themis::training::SelectionAuditEntry

#### `SelectionAuditEntry()`
- Source: `include/training/lora_data_selection.h`:166
- Brief: n/a
- Parameters: none

#### `std::string toJSONL() const`
- Source: `include/training/lora_data_selection.h`:175
- Brief: Serialize to a single JSON Lines (JSONL) string.
- Parameters: none
- Details: Produces one compact JSON object per call, suitable for appending to a .jsonl file. Uses a self-contained serializer – no external JSON library dependency.

### themis::training::SelfImprovementConfig

#### `SelfImprovementConfig()=default`
- Source: `include/training/lora_data_selection.h`:419
- Brief: n/a
- Parameters: none

#### `SelfImprovementConfig fromYAMLString(const std::string &yaml_text, const std::string &section="self_improvement")`
- Source: `include/training/lora_data_selection.h`:437
- Brief: Parse configuration from an in-memory YAML string.
- Parameters:
  - `yaml_text` (const std::string &): Input parameter.
  - `section` (const std::string &): Input parameter.
- Return: Return value.
- Details: From YAMLString. yaml_text Input parameter. section Input parameter. Return value. Calls: yaml_detail::parseSelfImprovementYAML().

#### `SelfImprovementConfig loadFromYAML(const std::string &path, const std::string &section="self_improvement")`
- Source: `include/training/lora_data_selection.h`:430
- Brief: Load configuration from a YAML file.
- Parameters:
  - `path` (const std::string &): Input parameter.
  - `section` (const std::string &): Input parameter.
- Return: Return value.
- Throws:
  - std::runtime_error: if the file cannot be opened.
  - std::runtime_error: if an error occurs.
- Details: Load From YAML. Reads the self_improvement: section (or a custom section) from the file at path. Uses the same built-in line parser as LoRADataSelectionConfig::loadFromYAML(). std::runtime_error if the file cannot be opened. path Input parameter. section Input parameter. Return value. std::runtime_error if an error occurs. Calls: f(), is_open(), rdbuf(), yaml_detail::parseSelfImprovementYAML(), str().

### themis::training::SelfImprovementModule

#### `SelfImprovementModule(const SelfImprovementConfig &config)`
- Source: `include/training/lora_data_selection.h`:464
- Brief: n/a
- Parameters:
  - `config` (const SelfImprovementConfig &): n/a

#### `SelfImprovementModule(const SelfImprovementModule &)=delete`
- Source: `include/training/lora_data_selection.h`:467
- Brief: n/a
- Parameters:
  - `<unnamed>` (const SelfImprovementModule &): n/a

#### `LoRADataSelectionConfig applyAdaptiveRules(const LoRADataSelectionConfig &current_config, const DataSelectionMetrics &metrics) const`
- Source: `include/training/lora_data_selection.h`:482
- Brief: Evaluate all adaptive rules against metrics.
- Parameters:
  - `current_config` (const LoRADataSelectionConfig &): Config snapshot to start from.
  - `metrics` (const DataSelectionMetrics &): Current monitoring metrics.
- Return: Updated config (unchanged if no rules triggered, or if threshold_auto_adjust is false).
- Details: For each rule whose condition is satisfied, the corresponding field in a copy of current_config is adjusted by the rule's delta. The modified copy is returned; the original is never mutated. current_config Config snapshot to start from. metrics Current monitoring metrics. Updated config (unchanged if no rules triggered, or if threshold_auto_adjust is false).

#### `const SelfImprovementConfig & getConfig() const`
- Source: `include/training/lora_data_selection.h`:528
- Brief: Get the current self-improvement configuration.
- Parameters: none

#### `size_t lastTriggeredRuleCount() const`
- Source: `include/training/lora_data_selection.h`:490
- Brief: Return how many rules were triggered on the last call to applyAdaptiveRules().
- Parameters: none

#### `bool needsReselection(std::chrono::system_clock::time_point last_selection_time) const`
- Source: `include/training/lora_data_selection.h`:517
- Brief: Decide whether a new data selection run is due.
- Parameters:
  - `last_selection_time` (std::chrono::system_clock::time_point): Time-point of the most recent pipeline run.
- Details: Returns true when enabled is true and at least period_seconds have elapsed since last_selection_time. last_selection_time Time-point of the most recent pipeline run.

#### `bool needsRollback(const DataSelectionMetrics &metrics) const`
- Source: `include/training/lora_data_selection.h`:507
- Brief: Decide whether the current metrics warrant a rollback.
- Parameters:
  - `metrics` (const DataSelectionMetrics &): Current monitoring snapshot.
- Details: Returns true when any of the following holds: training_accuracy has dropped by more than accuracy_rollback_threshold relative to a baseline of 1.0 – i.e. (1.0 - metrics.training_accuracy) > accuracy_rollback_threshold avg_quality_score < min_avg_quality_score diversity_monitoring is true and diversity_score < min_diversity_score Always returns false when enabled is false. metrics Current monitoring snapshot.

#### `SelfImprovementModule & operator=(const SelfImprovementModule &)=delete`
- Source: `include/training/lora_data_selection.h`:468
- Brief: n/a
- Parameters:
  - `<unnamed>` (const SelfImprovementModule &): n/a

#### `void setConfig(const SelfImprovementConfig &config)`
- Source: `include/training/lora_data_selection.h`:523
- Brief: Update the self-improvement configuration (live reload).
- Parameters:
  - `config` (const SelfImprovementConfig &): Input parameter.
- Details: Set Config. config Input parameter. Implements setConfig without additional internal calls.

#### `~SelfImprovementModule()`
- Source: `include/training/lora_data_selection.h`:465
- Brief: n/a
- Parameters: none

### themis::training::SelfImprovementModule::Impl

#### `Impl(const SelfImprovementConfig &cfg)`
- Source: `src/training/lora_data_selection.cpp`:1455
- Brief: n/a
- Parameters:
  - `cfg` (const SelfImprovementConfig &): n/a

#### `void applyAction(LoRADataSelectionConfig &cfg, const std::string &action, double delta)`
- Source: `src/training/lora_data_selection.cpp`:1551
- Brief: Apply a triggered rule action to the config copy.
- Parameters:
  - `cfg` (LoRADataSelectionConfig &): Input/output parameter.
  - `action` (const std::string &): Input parameter.
  - `delta` (double): Input parameter.
- Details: cfg Input/output parameter. action Input parameter. delta Input parameter. Calls: find(), contains(), std::clamp().

#### `LoRADataSelectionConfig applyAdaptiveRules(const LoRADataSelectionConfig &cfg, const DataSelectionMetrics &metrics)`
- Source: `src/training/lora_data_selection.cpp`:1583
- Brief: Apply Adaptive Rules.
- Parameters:
  - `cfg` (const LoRADataSelectionConfig &): Input parameter.
  - `metrics` (const DataSelectionMetrics &): Input parameter.
- Return: Return value.
- Details: cfg Input parameter. metrics Input parameter. Return value. Calls: getMetric(), evaluateCondition(), applyAction().

#### `bool evaluateCondition(const std::string &condition, double metric_value)`
- Source: `src/training/lora_data_selection.cpp`:1464
- Brief: Evaluate a single rule condition against observed metric value.
- Parameters:
  - `condition` (const std::string &): Input parameter.
  - `metric_value` (double): Input parameter.
- Return: True when the operation succeeds.
- Details: condition Input parameter. metric_value Input parameter. True when the operation succeeds. Supported conditions: "< N", "> N", "<= N", ">= N", "== N" Calls: size(), std::stod(), substr(), std::abs().

#### `const SelfImprovementConfig & getConfig() const`
- Source: `src/training/lora_data_selection.cpp`:1642
- Brief: n/a
- Parameters: none

#### `double getMetric(const DataSelectionMetrics &m, const std::string &name)`
- Source: `src/training/lora_data_selection.cpp`:1515
- Brief: Retrieve the monitored metric value by name.
- Parameters:
  - `m` (const DataSelectionMetrics &): Input parameter.
  - `name` (const std::string &): Input parameter.
- Return: Return value.
- Details: m Input parameter. name Input parameter. Return value. Implements getMetric without additional internal calls.

#### `size_t lastTriggeredRuleCount() const`
- Source: `src/training/lora_data_selection.cpp`:1635
- Brief: n/a
- Parameters: none

#### `bool needsReselection(std::chrono::system_clock::time_point last_selection_time) const`
- Source: `src/training/lora_data_selection.cpp`:1626
- Brief: n/a
- Parameters:
  - `last_selection_time` (std::chrono::system_clock::time_point): n/a

#### `bool needsRollback(const DataSelectionMetrics &metrics) const`
- Source: `src/training/lora_data_selection.cpp`:1601
- Brief: n/a
- Parameters:
  - `metrics` (const DataSelectionMetrics &): n/a

#### `void setConfig(const SelfImprovementConfig &cfg)`
- Source: `src/training/lora_data_selection.cpp`:1641
- Brief: Set Config.
- Parameters:
  - `cfg` (const SelfImprovementConfig &): Input parameter.
- Details: cfg Input parameter. Implements setConfig without additional internal calls.

### themis::training::ServingException

#### `ServingException(const std::string &message, TrainingErrorCode code=TrainingErrorCode::SERVING_ADAPTER_NOT_FOUND, bool recoverable=false, const std::string &context="")`
- Source: `include/training/training_exceptions.h`:204
- Brief: n/a
- Parameters:
  - `message` (const std::string &): n/a
  - `code` (TrainingErrorCode): n/a
  - `recoverable` (bool): n/a
  - `context` (const std::string &): n/a

### themis::training::TableExtractor

#### `TableExtractor(const ModalityParserConfig &config)`
- Source: `include/training/modality_parser.h`:137
- Brief: n/a
- Parameters:
  - `config` (const ModalityParserConfig &): n/a

#### `std::vector< TrainingSample > extract(const std::string &text, const std::string &document_id) const`
- Source: `include/training/modality_parser.h`:149
- Brief: Extract table blocks from text.
- Parameters:
  - `text` (const std::string &): Raw document text.
  - `document_id` (const std::string &): Identifier propagated to sample.source_id.
- Return: Vector of TABLE-typed TrainingSample records.
- Details: text Raw document text. document_id Identifier propagated to sample.source_id. Vector of TABLE-typed TrainingSample records. Applies shared prompt-safety policy per extracted table block. Blocks that trigger a prompt-injection block rule are dropped; allowed blocks are emitted with control-token redaction applied.

### themis::training::TextClauseExtractor

#### `TextClauseExtractor(const ModalityParserConfig &config)`
- Source: `include/training/modality_parser.h`:106
- Brief: n/a
- Parameters:
  - `config` (const ModalityParserConfig &): n/a

#### `std::vector< TrainingSample > extract(const std::string &text, const std::string &document_id) const`
- Source: `include/training/modality_parser.h`:118
- Brief: Extract text clauses from text.
- Parameters:
  - `text` (const std::string &): Raw document text.
  - `document_id` (const std::string &): Identifier propagated to sample.source_id.
- Return: Vector of TEXT_CLAUSE-typed TrainingSample records.
- Details: text Raw document text. document_id Identifier propagated to sample.source_id. Vector of TEXT_CLAUSE-typed TrainingSample records. Applies shared prompt-safety policy per extracted clause. Clauses matching blocked prompt-injection patterns are dropped (fail-closed). Allowed clauses are emitted with control-token redaction applied.

### themis::training::TrainingDiagnostics

#### `TrainingDiagnostics()`
- Source: `include/training/training_error_diagnostics.h`:55
- Brief: n/a
- Parameters: none

#### `TrainingDiagnostics & add_note(const std::string &note)`
- Source: `include/training/training_error_diagnostics.h`:106
- Brief: Add a diagnostic note or observation.
- Parameters:
  - `note` (const std::string &): Human-readable note describing the failure or observed state.
- Details: note Human-readable note describing the failure or observed state.

#### `TrainingDiagnostics & error_code(TrainingErrorCode code)`
- Source: `include/training/training_error_diagnostics.h`:114
- Brief: Set error code.
- Parameters:
  - `code` (TrainingErrorCode): n/a

#### `TrainingDiagnostics & input(const std::string &key, bool value)`
- Source: `include/training/training_error_diagnostics.h`:97
- Brief: Add a boolean input parameter.
- Parameters:
  - `key` (const std::string &): n/a
  - `value` (bool): n/a

#### `TrainingDiagnostics & input(const std::string &key, const std::string &value)`
- Source: `include/training/training_error_diagnostics.h`:73
- Brief: Add an input parameter or state variable.
- Parameters:
  - `key` (const std::string &): Parameter name.
  - `value` (const std::string &): Parameter value (as string).
- Details: key Parameter name. value Parameter value (as string).

#### `TrainingDiagnostics & input(const std::string &key, double value)`
- Source: `include/training/training_error_diagnostics.h`:88
- Brief: Add a numeric input parameter.
- Parameters:
  - `key` (const std::string &): n/a
  - `value` (double): n/a

#### `TrainingDiagnostics & input(const std::string &key, int64_t value)`
- Source: `include/training/training_error_diagnostics.h`:81
- Brief: Add a numeric input parameter.
- Parameters:
  - `key` (const std::string &): n/a
  - `value` (int64_t): n/a

#### `TrainingDiagnostics & operation(const std::string &op)`
- Source: `include/training/training_error_diagnostics.h`:62
- Brief: Set the operation name (e.g., "checkpoint_save", "training_step").
- Parameters:
  - `op` (const std::string &): n/a

#### `TrainingDiagnostics & recoverable(bool is_recoverable)`
- Source: `include/training/training_error_diagnostics.h`:122
- Brief: Set whether error is recoverable.
- Parameters:
  - `is_recoverable` (bool): n/a

#### `std::string timestamp_string() const`
- Source: `include/training/training_error_diagnostics.h`:130
- Brief: Get the timestamp as ISO8601 string.
- Parameters: none

#### `std::string to_string() const`
- Source: `include/training/training_error_diagnostics.h`:147
- Brief: Convert diagnostics to formatted string.
- Parameters: none
- Details: Format: [TIMESTAMP]operation="{operation}"error_code="{code}" Inputs:{key1}={value1},{key2}={value2},... Notes:{note1};{note2};...

### themis::training::TrainingErrorLogger

#### `TrainingErrorLogger(TrainingErrorCode code, const std::string &message, const std::string &context="")`
- Source: `include/training/training_error_diagnostics.h`:212
- Brief: n/a
- Parameters:
  - `code` (TrainingErrorCode): n/a
  - `message` (const std::string &): n/a
  - `context` (const std::string &): n/a

#### `std::string to_log_string() const`
- Source: `include/training/training_error_diagnostics.h`:221
- Brief: Convert to loggable string.
- Parameters: none

### themis::training::TrainingException

#### `TrainingException(const std::string &message, TrainingErrorCode code=TrainingErrorCode::SUCCESS, bool recoverable=false, const std::string &context="")`
- Source: `include/training/training_exceptions.h`:50
- Brief: Construct a training exception with full diagnostic information.
- Parameters:
  - `message` (const std::string &): Human-readable error message (actionable for operators).
  - `code` (TrainingErrorCode): Error code identifying the specific failure mode.
  - `recoverable` (bool): Whether the operation can be retried or recovered.
  - `context` (const std::string &): Optional diagnostic context (operation name, input state, etc).
- Details: message Human-readable error message (actionable for operators). code Error code identifying the specific failure mode. recoverable Whether the operation can be retried or recovered. context Optional diagnostic context (operation name, input state, etc).

#### `const std::string & context() const noexcept`
- Source: `include/training/training_exceptions.h`:90
- Brief: Get diagnostic context.
- Parameters: none
- Details: Context string provides operation name, input state, or other information useful for debugging and production troubleshooting.

#### `std::string diagnostic_message() const`
- Source: `include/training/training_exceptions.h`:98
- Brief: Get formatted diagnostic message.
- Parameters: none
- Details: Format: "[{ErrorCodeName}] {message} ({context})" Useful for structured logging and operator alerts.

#### `TrainingErrorCode error_code() const noexcept`
- Source: `include/training/training_exceptions.h`:65
- Brief: Get the structured error code.
- Parameters: none

#### `uint32_t error_code_value() const noexcept`
- Source: `include/training/training_exceptions.h`:70
- Brief: Get numeric error code value.
- Parameters: none

#### `bool is_recoverable() const noexcept`
- Source: `include/training/training_exceptions.h`:82
- Brief: Check if the error is transient/recoverable.
- Parameters: none
- Details: Recoverable errors typically indicate resource exhaustion, timeouts, or temporary unavailability that may succeed on retry. Non-recoverable errors indicate invalid input or state that requires operator intervention.

#### `~TrainingException()=default`
- Source: `include/training/training_exceptions.h`:60
- Brief: n/a
- Parameters: none

### themis::training::TrainingFailureException

#### `TrainingFailureException(const std::string &message, TrainingErrorCode code=TrainingErrorCode::TRAINING_CONFIG_INVALID, bool recoverable=false, const std::string &context="")`
- Source: `include/training/training_exceptions.h`:138
- Brief: n/a
- Parameters:
  - `message` (const std::string &): n/a
  - `code` (TrainingErrorCode): n/a
  - `recoverable` (bool): n/a
  - `context` (const std::string &): n/a

### themis::training::TrainingIncidentEmitter

#### `TrainingIncidentEmitter()=default`
- Source: `include/training/training_incident_emitter.h`:171
- Brief: n/a
- Parameters: none

#### `TrainingIncidentEmitter(const TrainingIncidentEmitter &)=delete`
- Source: `include/training/training_incident_emitter.h`:174
- Brief: n/a
- Parameters:
  - `<unnamed>` (const TrainingIncidentEmitter &): n/a

#### `void addListener(std::shared_ptr< TrainingIncidentListener > listener)`
- Source: `include/training/training_incident_emitter.h`:187
- Brief: Register a listener to receive all subsequent incidents.
- Parameters:
  - `listener` (std::shared_ptr< TrainingIncidentListener >): Non-null shared_ptr to the listener. Duplicate registrations result in duplicate notifications.
- Details: listener Non-null shared_ptr to the listener. Duplicate registrations result in duplicate notifications.

#### `void broadcast(const TrainingIncident &incident)`
- Source: `include/training/training_incident_emitter.h`:309
- Brief: n/a
- Parameters:
  - `incident` (const TrainingIncident &): n/a

#### `void emitAdapterIncident(TrainingErrorCode error_code, const std::string &component, const std::string &operation, const std::string &message, bool recoverable=false, const std::string &context={})`
- Source: `include/training/training_incident_emitter.h`:274
- Brief: Emit an adapter-class incident (merge, serving, handoff).
- Parameters:
  - `error_code` (TrainingErrorCode): Structured error code from the training taxonomy.
  - `component` (const std::string &): Emitting component (e.g., "adapter_serving").
  - `operation` (const std::string &): Operation name (e.g., "deploy_version").
  - `message` (const std::string &): Human-readable incident summary.
  - `recoverable` (bool): Whether the fault can recover without operator action.
  - `context` (const std::string &): Additional key=value diagnostic context string.
- Details: error_code Structured error code from the training taxonomy. component Emitting component (e.g., "adapter_serving"). operation Operation name (e.g., "deploy_version"). message Human-readable incident summary. recoverable Whether the fault can recover without operator action. context Additional key=value diagnostic context string.

#### `void emitDatasetIncident(TrainingErrorCode error_code, const std::string &component, const std::string &operation, const std::string &message, bool recoverable=false, const std::string &context={})`
- Source: `include/training/training_incident_emitter.h`:224
- Brief: Emit a dataset-class incident (labeling, enrichment, validation).
- Parameters:
  - `error_code` (TrainingErrorCode): Structured error code from the training taxonomy.
  - `component` (const std::string &): Emitting component (e.g., "auto_labeler").
  - `operation` (const std::string &): Operation name (e.g., "label_batch").
  - `message` (const std::string &): Human-readable incident summary.
  - `recoverable` (bool): Whether the fault can recover without operator action.
  - `context` (const std::string &): Additional key=value diagnostic context string.
- Details: error_code Structured error code from the training taxonomy. component Emitting component (e.g., "auto_labeler"). operation Operation name (e.g., "label_batch"). message Human-readable incident summary. recoverable Whether the fault can recover without operator action. context Additional key=value diagnostic context string.

#### `void emitTrainingIncident(TrainingErrorCode error_code, const std::string &component, const std::string &operation, const std::string &message, bool recoverable=false, const std::string &context={})`
- Source: `include/training/training_incident_emitter.h`:249
- Brief: Emit a training-class incident (step, convergence, checkpoint).
- Parameters:
  - `error_code` (TrainingErrorCode): Structured error code from the training taxonomy.
  - `component` (const std::string &): Emitting component (e.g., "incremental_lora_trainer").
  - `operation` (const std::string &): Operation name (e.g., "train_step").
  - `message` (const std::string &): Human-readable incident summary.
  - `recoverable` (bool): Whether the fault can recover without operator action.
  - `context` (const std::string &): Additional key=value diagnostic context string.
- Details: error_code Structured error code from the training taxonomy. component Emitting component (e.g., "incremental_lora_trainer"). operation Operation name (e.g., "train_step"). message Human-readable incident summary. recoverable Whether the fault can recover without operator action. context Additional key=value diagnostic context string.

#### `size_t listenerCount() const`
- Source: `include/training/training_incident_emitter.h`:205
- Brief: Number of currently registered listeners.
- Parameters: none

#### `TrainingIncident makeIncident(TrainingIncidentClass cls, TrainingErrorCode error_code, const std::string &component, const std::string &operation, const std::string &message, bool recoverable, const std::string &context)`
- Source: `include/training/training_incident_emitter.h`:290
- Brief: n/a
- Parameters:
  - `cls` (TrainingIncidentClass): n/a
  - `error_code` (TrainingErrorCode): n/a
  - `component` (const std::string &): n/a
  - `operation` (const std::string &): n/a
  - `message` (const std::string &): n/a
  - `recoverable` (bool): n/a
  - `context` (const std::string &): n/a

#### `TrainingIncidentEmitter & operator=(const TrainingIncidentEmitter &)=delete`
- Source: `include/training/training_incident_emitter.h`:175
- Brief: n/a
- Parameters:
  - `<unnamed>` (const TrainingIncidentEmitter &): n/a

#### `void removeListeners()`
- Source: `include/training/training_incident_emitter.h`:197
- Brief: Remove all registered listeners.
- Parameters: none
- Details: After this call no incidents will be forwarded until new listeners are added.

### themis::training::TrainingIncidentListener

#### `void onIncident(const TrainingIncident &incident)=0`
- Source: `include/training/training_incident_emitter.h`:145
- Brief: Receive a training incident event.
- Parameters:
  - `incident` (const TrainingIncident &): The incident. All fields are populated before the call.
- Details: incident The incident. All fields are populated before the call.

#### `~TrainingIncidentListener()=default`
- Source: `include/training/training_incident_emitter.h`:138
- Brief: n/a
- Parameters: none

### themis::training::TrainingJob

#### `TrainingJob()=default`
- Source: `include/training/training_interfaces.h`:161
- Brief: n/a
- Parameters: none

### themis::training::TrainingMetrics

#### `TrainingMetrics()=default`
- Source: `include/training/incremental_lora_trainer.h`:105
- Brief: n/a
- Parameters: none

#### `void reset()`
- Source: `include/training/incremental_lora_trainer.h`:108
- Brief: Reset all accumulated metrics.
- Parameters: none

### themis::training::TrainingPipeline

#### `TrainingPipeline(const PipelineConfig &config, const std::string &db_connection)`
- Source: `include/training/training_pipeline.h`:341
- Brief: Construct pipeline.
- Parameters:
  - `config` (const PipelineConfig &): Pipeline configuration
  - `db_connection` (const std::string &): Database connection string
- Details: config Pipeline configuration db_connection Database connection string

#### `TrainingPipeline(const TrainingPipeline &)=delete`
- Source: `include/training/training_pipeline.h`:347
- Brief: n/a
- Parameters:
  - `<unnamed>` (const TrainingPipeline &): n/a

#### `void addCalibrationSample(const std::string &category, float confidence, bool model_correct)`
- Source: `include/training/training_pipeline.h`:424
- Brief: Feed a per-sample validation pair to the internal calibrator.
- Parameters:
  - `category` (const std::string &): Input parameter.
  - `confidence` (float): Input parameter.
  - `model_correct` (bool): Input parameter.
- Details: Add Calibration Sample. Should be called once per sample after the validation loop in IncrementalLoRATrainer to accumulate data for runCalibration(). category Legal category of the sample. confidence Model confidence score in [0, 1]. model_correct Whether the model produced the correct label. category Input parameter. confidence Input parameter. model_correct Input parameter. Implements addCalibrationSample without additional internal calls.

#### `DataQualityReport checkDataQuality(float min_confidence=0.5f)`
- Source: `include/training/training_pipeline.h`:433
- Brief: Perform data-quality checks on the training collection.
- Parameters:
  - `min_confidence` (float): Input parameter.
- Return: Data quality report
- Details: Check Data Quality. min_confidence Minimum acceptable sample confidence Data quality report min_confidence Input parameter. Return value. Implements checkDataQuality without additional internal calls.

#### `DriftReport detectLabelDrift(const std::vector< std::string > &reference_samples={})`
- Source: `include/training/training_pipeline.h`:440
- Brief: Detect label drift between training samples and new data.
- Parameters:
  - `reference_samples` (const std::vector< std::string > &): Input parameter.
- Return: Drift detection report
- Details: Detect Label Drift. reference_samples Baseline sample IDs for comparison Drift detection report reference_samples Input parameter. Return value. Implements detectLabelDrift without additional internal calls.

#### `PipelineStats getLastStats() const`
- Source: `include/training/training_pipeline.h`:453
- Brief: Get the last pipeline execution statistics.
- Parameters: none

#### `TrainingPipeline & operator=(const TrainingPipeline &)=delete`
- Source: `include/training/training_pipeline.h`:348
- Brief: n/a
- Parameters:
  - `<unnamed>` (const TrainingPipeline &): n/a

#### `PipelineStats run(PipelineCallback callback=nullptr)`
- Source: `include/training/training_pipeline.h`:361
- Brief: Execute the full training pipeline.
- Parameters:
  - `callback` (PipelineCallback): Input parameter.
- Return: Aggregated pipeline statistics
- Details: Run. callback Optional per-stage progress callback Aggregated pipeline statistics Fail-closed guards (QW-40): Validates critical inputs before execution: target_collection non-empty (prompt injection prevention) drift_threshold in valid range [0, 1] (prevent invalid LLM queries) data_selection collection_name non-empty (prevent injection) If any guard fails, returns error_message in stats and avoids LLM calls. callback Input parameter. Return value. Implements run without additional internal calls.

#### `CalibrationResult runCalibration()`
- Source: `include/training/training_pipeline.h`:412
- Brief: Run the confidence calibration stage and persist the result.
- Parameters: none
- Return: Calibration result with per-category thresholds.
- Details: Run Calibration. Executes ConfidenceCalibrator::calibrate() on any accumulated per-sample (confidence, correct) pairs and, when enable_checkpoint_manager is true, writes a calibration_manifest.json to the checkpoint directory via LoRACheckpointManager. Calibration result with per-category thresholds. Return value. Implements runCalibration without additional internal calls.

#### `DataSelectionResult runDataSelection(SelectionProgressCallback callback=nullptr)`
- Source: `include/training/training_pipeline.h`:391
- Brief: Run only the automated data selection stage.
- Parameters:
  - `callback` (SelectionProgressCallback): Input parameter.
- Return: Selection result including selected samples and provenance.
- Details: Run Data Selection. Executes all five selection sub-stages (quality filter, deduplication, clustering, scoring, curriculum sampling) on the current training collection and returns the selection result with audit entry. callback Optional per-stage progress callback. Emitted messages are sanitized via the shared prompt-safety policy before callback invocation. Selection result including selected samples and provenance. callback Input parameter. Return value. Calls: std::move().

#### `EnrichmentStats runEnrichment(EnrichmentCallback callback=nullptr)`
- Source: `include/training/training_pipeline.h`:377
- Brief: Run only the enrichment stage.
- Parameters:
  - `callback` (EnrichmentCallback): Input parameter.
- Return: Return value.
- Details: Run Enrichment. Callback messages are sanitized via the shared prompt-safety policy before emission. callback Input parameter. Return value. Implements runEnrichment without additional internal calls.

#### `HyperparamResult runHyperparamSearch(const HyperparamSearchConfig &config, HyperparamSearchCallback callback=nullptr)`
- Source: `include/training/training_pipeline.h`:475
- Brief: Run an automated hyperparameter search over rank × lr combinations.
- Parameters:
  - `config` (const HyperparamSearchConfig &): Input parameter.
  - `callback` (HyperparamSearchCallback): Input parameter.
- Return: Search result with best rank, lr, val_loss, and trial log.
- Details: Run Hyperparam Search. Generates the Cartesian product of config.rank_candidates and config.lr_candidates, shuffles the trial list with config.seed for deterministic ordering, then executes up to config.max_trials trials. Each trial clones the pipeline's trainer config with the trial's rank and learning rate and runs a single-pass training simulation. When config.budget_seconds > 0 the search stops early once the wall-clock budget is exceeded and returns the best result seen so far. On success, the best (rank, lr) pair is automatically applied to the pipeline's internal trainer configuration so subsequent calls to run() or runTraining() use the optimised hyperparameters. config Search configuration (candidates, budget, seed). callback Optional per-trial callback. Search result with best rank, lr, val_loss, and trial log. config Input parameter. callback Input parameter. Return value. Calls: std::move().

#### `LabelingStats runLabeling(LabelingCallback callback=nullptr)`
- Source: `include/training/training_pipeline.h`:369
- Brief: Run only the auto-labeling stage.
- Parameters:
  - `callback` (LabelingCallback): Input parameter.
- Return: Return value.
- Details: Run Labeling. Callback messages are sanitized via the shared prompt-safety policy before emission. callback Input parameter. Return value. Implements runLabeling without additional internal calls.

#### `TrainingResult runTraining(TrainingCallback callback=nullptr)`
- Source: `include/training/training_pipeline.h`:400
- Brief: Run only the training stage.
- Parameters:
  - `callback` (TrainingCallback): Input parameter.
- Return: Return value.
- Details: Run Training. Callback messages are sanitized via the shared prompt-safety policy before emission. callback Input parameter. Return value. Implements runTraining without additional internal calls.

#### `std::string sanitizeCallbackMessage(const std::string &message)`
- Source: `include/training/training_pipeline.h`:334
- Brief: Sanitize a pipeline callback/progress message with the shared prompt-safety policy.
- Parameters:
  - `message` (const std::string &): Input parameter.
- Return: Sanitized callback message suitable for downstream emission.
- Details: Sanitize Callback Message. The returned string is safe to emit through telemetry, logs, or external callback sinks: blocked prompt-injection patterns are fail-closed to a constant marker allowed payloads are returned with control-token redaction applied message Raw callback/progress message. Sanitized callback message suitable for downstream emission. message Input parameter. Return value. Calls: sanitizeTrainingPipelineMessage().

#### `void scheduleRetraining(size_t interval_hours, PipelineCallback callback=nullptr)`
- Source: `include/training/training_pipeline.h`:447
- Brief: Schedule automated retraining at a fixed interval.
- Parameters:
  - `interval_hours` (size_t): Input parameter.
  - `callback` (PipelineCallback): Input parameter.
- Details: Schedule Retraining. interval_hours How often to retrain (in hours) callback Optional per-run callback interval_hours Input parameter. callback Input parameter. Implements scheduleRetraining without additional internal calls.

#### `~TrainingPipeline()`
- Source: `include/training/training_pipeline.h`:344
- Brief: n/a
- Parameters: none

### themis::training::TrainingPipeline::Impl

#### `Impl(const PipelineConfig &config, const std::string &db_connection)`
- Source: `src/training/training_pipeline.cpp`:103
- Brief: Impl.
- Parameters:
  - `config` (const PipelineConfig &): Input parameter.
  - `db_connection` (const std::string &): Input parameter.
- Return: Return value.
- Details: config Input parameter. db_connection Input parameter. Return value.

#### `void addCalibrationSample(const std::string &category, float confidence, bool correct)`
- Source: `src/training/training_pipeline.cpp`:502
- Brief: Add Calibration Sample.
- Parameters:
  - `category` (const std::string &): Input parameter.
  - `confidence` (float): Input parameter.
  - `correct` (bool): Input parameter.
- Details: category Input parameter. confidence Input parameter. correct Input parameter. Calls: addSample().

#### `DataQualityReport checkDataQuality(float min_confidence)`
- Source: `src/training/training_pipeline.cpp`:410
- Brief: ---------------------------------------------------------------------- Phase 7: Data-quality checks ----------------------------------------------------------------------
- Parameters:
  - `min_confidence` (float): Input parameter.
- Return: Return value.
- Details: min_confidence Input parameter. Return value. Calls: DataQualityReport().

#### `DriftReport detectLabelDrift(const std::vector< std::string > &reference_samples)`
- Source: `src/training/training_pipeline.cpp`:441
- Brief: ---------------------------------------------------------------------- Phase 7: Label-drift detection ----------------------------------------------------------------------
- Parameters:
  - `reference_samples` (const std::vector< std::string > &): Input parameter.
- Return: Return value.
- Details: reference_samples Input parameter. Return value. Calls: DriftReport().

#### `PipelineStats getLastStats() const`
- Source: `src/training/training_pipeline.cpp`:460
- Brief: n/a
- Parameters: none

#### `PipelineStats run(PipelineCallback callback)`
- Source: `src/training/training_pipeline.cpp`:131
- Brief: ---------------------------------------------------------------------- Phase 7: Full pipeline execution ----------------------------------------------------------------------
- Parameters:
  - `callback` (PipelineCallback): Input parameter.
- Return: Return value.
- Details: callback Input parameter. Return value. Calls: PipelineStats(), PipelineMetrics(), std::chrono::steady_clock::now(), empty(), callback(), sanitizeTrainingPipelineMessage(), beginStage(), emitCallback().

#### `CalibrationResult runCalibration()`
- Source: `src/training/training_pipeline.cpp`:481
- Brief: ---------------------------------------------------------------------- Phase 3: Calibration wrappers ----------------------------------------------------------------------
- Parameters: none
- Return: Return value.
- Details: Return value. Calls: calibrate(), empty(), serializeCalibrationResult(), saveCalibrationJson().

#### `DataSelectionResult runDataSelection(SelectionProgressCallback callback)`
- Source: `src/training/training_pipeline.cpp`:381
- Brief: ---------------------------------------------------------------------- Data selection stage (Quality & Diversity Layer) ----------------------------------------------------------------------
- Parameters:
  - `callback` (SelectionProgressCallback): Input parameter.
- Return: Return value.
- Details: callback Input parameter. Return value. Calls: setConfig(), run(), std::move(), callback(), sanitizeTrainingPipelineMessage().

#### `EnrichmentStats runEnrichment(EnrichmentCallback callback)`
- Source: `src/training/training_pipeline.cpp`:348
- Brief: Run Enrichment.
- Parameters:
  - `callback` (EnrichmentCallback): Input parameter.
- Return: Return value.
- Details: callback Input parameter. Return value. Calls: enrichAll(), callback(), sanitizeTrainingPipelineMessage().

#### `HyperparamResult runHyperparamSearch(const HyperparamSearchConfig &cfg, HyperparamSearchCallback callback)`
- Source: `src/training/training_pipeline.cpp`:513
- Brief: ---------------------------------------------------------------------- Phase 2: Automated hyperparameter search (rank × lr grid sweep) ----------------------------------------------------------------------
- Parameters:
  - `cfg` (const HyperparamSearchConfig &): Input parameter.
  - `callback` (HyperparamSearchCallback): Input parameter.
- Return: Return value.
- Details: cfg Input parameter. callback Input parameter. Return value. Calls: empty(), std::chrono::steady_clock::now(), reserve(), size(), push_back(), std::swap(), shuffle_lcg(), resize().

#### `LabelingStats runLabeling(LabelingCallback callback)`
- Source: `src/training/training_pipeline.cpp`:332
- Brief: ---------------------------------------------------------------------- Phase 7: Stage-specific entry points ----------------------------------------------------------------------
- Parameters:
  - `callback` (LabelingCallback): Input parameter.
- Return: Return value.
- Details: callback Input parameter. Return value. Calls: labelAll(), callback(), sanitizeTrainingPipelineMessage().

#### `TrainingResult runTraining(TrainingCallback callback)`
- Source: `src/training/training_pipeline.cpp`:364
- Brief: Run Training.
- Parameters:
  - `callback` (TrainingCallback): Input parameter.
- Return: Return value.
- Details: callback Input parameter. Return value. Calls: train(), callback(), sanitizeTrainingPipelineMessage().

#### `void scheduleRetraining(size_t interval_hours, PipelineCallback callback)`
- Source: `src/training/training_pipeline.cpp`:470
- Brief: Phase 7: Schedule retraining (stored for reference; actual scheduling requires a thread/timer service in production).
- Parameters:
  - `interval_hours` (size_t): Input parameter.
  - `callback` (PipelineCallback): Input parameter.
- Details: interval_hours Input parameter. callback Input parameter. Implements scheduleRetraining without additional internal calls.

#### `std::string serializeCalibrationResult(const CalibrationResult &r)`
- Source: `src/training/training_pipeline.cpp`:655
- Brief: Serialise a CalibrationResult to a key=value text format (no JSON dep).
- Parameters:
  - `r` (const CalibrationResult &): Input parameter.
- Return: Return value.
- Details: r Input parameter. Return value. Calls: size(), str().

#### `~Impl()=default`
- Source: `src/training/training_pipeline.cpp`:123
- Brief: n/a
- Parameters: none

### themis::training::TrainingResult

#### `TrainingResult()=default`
- Source: `include/training/incremental_lora_trainer.h`:134
- Brief: n/a
- Parameters: none

### themis::training::TrainingSample

#### `TrainingSample()`
- Source: `include/training/auto_labeler.h`:61
- Brief: n/a
- Parameters: none

### themis::training::TrainingSession

#### `TrainingSession(const std::string &id="session_1")`
- Source: `tests/training/test_training_cancellation.cpp`:36
- Brief: n/a
- Parameters:
  - `id` (const std::string &): n/a

#### `void cancel()`
- Source: `tests/training/test_training_cancellation.cpp`:70
- Brief: n/a
- Parameters: none

#### `void cleanupResources()`
- Source: `tests/training/test_training_cancellation.cpp`:127
- Brief: n/a
- Parameters: none

#### `CleanupStats getCleanupStats() const`
- Source: `tests/training/test_training_cancellation.cpp`:109
- Brief: n/a
- Parameters: none

#### `int getCurrentEpoch() const`
- Source: `tests/training/test_training_cancellation.cpp`:94
- Brief: n/a
- Parameters: none

#### `std::string getSessionId() const`
- Source: `tests/training/test_training_cancellation.cpp`:98
- Brief: n/a
- Parameters: none

#### `bool hasCheckpointSaved() const`
- Source: `tests/training/test_training_cancellation.cpp`:89
- Brief: n/a
- Parameters: none

#### `bool isCancelled() const`
- Source: `tests/training/test_training_cancellation.cpp`:80
- Brief: n/a
- Parameters: none

#### `bool isCompleted() const`
- Source: `tests/training/test_training_cancellation.cpp`:84
- Brief: n/a
- Parameters: none

#### `void saveCheckpoint()`
- Source: `tests/training/test_training_cancellation.cpp`:148
- Brief: n/a
- Parameters: none

#### `void startTraining(int duration_ms=5000)`
- Source: `tests/training/test_training_cancellation.cpp`:39
- Brief: n/a
- Parameters:
  - `duration_ms` (int): n/a

#### `void waitForCompletion(int timeout_ms=10000)`
- Source: `tests/training/test_training_cancellation.cpp`:114
- Brief: n/a
- Parameters:
  - `timeout_ms` (int): n/a

### themis::training::checkpoint

#### `bool parseDoubleStrict(const std::string &value, double &out)`
- Source: `src/training/incremental_lora_trainer.cpp`:144
- Brief: Parse Double Strict.
- Parameters:
  - `value` (const std::string &): Input parameter.
  - `out` (double &): Input/output parameter.
- Return: True when the operation succeeds.
- Details: value Input parameter. out Input/output parameter. True when the operation succeeds. Calls: empty(), std::strtod(), c_str(), std::isfinite().

#### `bool parseMetadata(const std::string &data, std::string &version, size_t &epoch, size_t &step, double &loss, double &accuracy, std::string *error_reason=nullptr)`
- Source: `src/training/incremental_lora_trainer.cpp`:185
- Brief: n/a
- Parameters:
  - `data` (const std::string &): n/a
  - `version` (std::string &): n/a
  - `epoch` (size_t &): n/a
  - `step` (size_t &): n/a
  - `loss` (double &): n/a
  - `accuracy` (double &): n/a
  - `error_reason` (std::string *): n/a

#### `bool parseSizeTStrict(const std::string &value, size_t &out)`
- Source: `src/training/incremental_lora_trainer.cpp`:116
- Brief: Parse Size TStrict.
- Parameters:
  - `value` (const std::string &): Input parameter.
  - `out` (size_t &): Input/output parameter.
- Return: True when the operation succeeds.
- Details: value Input parameter. out Input/output parameter. True when the operation succeeds. Calls: empty(), front(), std::strtoull(), c_str(), max().

#### `std::string serializeMetadata(const std::string &version, size_t epoch, size_t step, double loss, double accuracy)`
- Source: `src/training/incremental_lora_trainer.cpp`:169
- Brief: Serialize checkpoint metadata to a simple key=value string.
- Parameters:
  - `version` (const std::string &): Input parameter.
  - `epoch` (size_t): Input parameter.
  - `step` (size_t): Input parameter.
  - `loss` (double): Input parameter.
  - `accuracy` (double): Input parameter.
- Return: Return value.
- Details: version Input parameter. epoch Input parameter. step Input parameter. loss Input parameter. accuracy Input parameter. Return value. Calls: str().

### themis::training::detail

#### `const std::regex RE_COURT_DECISION(R"((?:BGH\|BVerwG\|BAG\|BSG\|BFH\|BVerfG\|OLG\|LG\|AG\|VG\|OVG\|VGH\|LAG\|FG\|FGH\|LSG\|SGG?)\b[,\s]*(?:Urt\.\|Beschl\.\|Bes\.\|Beschluss\|Urteil)?[,\s]*(?:v\.\|vom)?\s*\d{1,2}\.\d{1,2}\.\d{2,4}[,\s–-]+[A-Z0-9 /]+)", std::regex_constants::optimize\|std::regex_constants::ECMAScript)`
- Source: `src/training/modality_parser.cpp`:165
- Brief: n/a
- Parameters:
  - `<unnamed>` (R"((?:BGH\|BVerwG\|BAG\|BSG\|BFH\|BVerfG\|OLG\|LG\|AG\|VG\|OVG\|VGH\|LAG\|FG\|FGH\|LSG\|SGG?)\b[,\s]*(?:Urt\.\|Beschl\.\|Bes\.\|Beschluss\|Urteil)?[,\s]*(?:v\.\|vom)?\s*\d{1,2}\.\d{1,2}\.\d{2,4}[,\s–-]+[A-Z0-9 /]+)"): n/a
  - `<unnamed>` (std::regex_constants::optimize\|std::regex_constants::ECMAScript): n/a

#### `const std::regex RE_EU_CITATION(R"((?:EuGH\|EuG\|EGMR\|ECtHR)\b[,\s]*(?:Rs\.\s*)?[CT]-?\d+/\d{2,4})", std::regex_constants::optimize\|std::regex_constants::ECMAScript)`
- Source: `src/training/modality_parser.cpp`:171
- Brief: n/a
- Parameters:
  - `<unnamed>` (R"((?:EuGH\|EuG\|EGMR\|ECtHR)\b[,\s]*(?:Rs\.\s*)?[CT]-?\d+/\d{2,4})"): n/a
  - `<unnamed>` (std::regex_constants::optimize\|std::regex_constants::ECMAScript): n/a

#### `const std::regex RE_STATUTORY(R"((?:(?:§§?\|\xC2\xA7(?:\xC2\xA7)?\|\xA7\xA7?)\s*\d+(?:\s*Abs\.\s*\d+)?(?:\s+(?:BGB\|HGB\|StGB\|ZPO\|GG\|VwGO\|AO\|UStG\|InsO\|GmbHG\|AktG\|WpHG\|KWG\|SGB\|UrhG\|BRAO\|BBodSchG\|TKG\|TMG\|GDPR\|DSGVO\|MarkenG\|PatG\|[A-Z]{2,10}))?)\|(?:Art\.\s*\d+(?:\s*Abs\.\s*\d+)?\s+[A-Z]{2,10}))", std::regex_constants::optimize\|std::regex_constants::ECMAScript)`
- Source: `src/training/modality_parser.cpp`:156
- Brief: n/a
- Parameters:
  - `<unnamed>` (R"((?:(?:§§?\|\xC2\xA7(?:\xC2\xA7)?\|\xA7\xA7?)\s*\d+(?:\s*Abs\.\s*\d+)?(?:\s+(?:BGB\|HGB\|StGB\|ZPO\|GG\|VwGO\|AO\|UStG\|InsO\|GmbHG\|AktG\|WpHG\|KWG\|SGB\|UrhG\|BRAO\|BBodSchG\|TKG\|TMG\|GDPR\|DSGVO\|MarkenG\|PatG\|[A-Z]{2,10}))?)\|(?:Art\.\s*\d+(?:\s*Abs\.\s*\d+)?\s+[A-Z]{2,10}))"): n/a
  - `<unnamed>` (std::regex_constants::optimize\|std::regex_constants::ECMAScript): n/a

#### `void appendAuditJSONL(const std::string &path, const std::string &jsonl_line)`
- Source: `src/training/lora_data_selection.cpp`:369
- Brief: Append Audit JSONL.
- Parameters:
  - `path` (const std::string &): Input parameter.
  - `jsonl_line` (const std::string &): Input parameter.
- Details: path Input parameter. jsonl_line Input parameter. Calls: empty(), rfind(), std::filesystem::create_directories(), std::filesystem::path(), parent_path(), lk(), ofs(), is_open().

#### `size_t approximateTokenCount(const std::string &text)`
- Source: `src/training/lora_data_selection.cpp`:69
- Brief: Approximate token count: split on whitespace.
- Parameters:
  - `text` (const std::string &): Input parameter.
- Return: Return value.
- Details: text Input parameter. Return value. Calls: empty().

#### `std::vector< uint32_t > buildMinHash(const std::string &text, size_t num_perm)`
- Source: `src/training/lora_data_selection.cpp`:179
- Brief: Build a MinHash signature (one value per permutation) using word 3-shingles.
- Parameters:
  - `text` (const std::string &): Input parameter.
  - `num_perm` (size_t): Input parameter.
- Return: Return value.
- Details: text Input parameter. num_perm Input parameter. Return value. Calls: iss(), push_back(), size(), insert(), empty(), signature(), fnv1a().

#### `double computeDomainRelevance(const std::string &text, const DomainKeywords &domain_keywords, const std::string &domain_hint="")`
- Source: `src/training/lora_data_selection.cpp`:267
- Brief: n/a
- Parameters:
  - `text` (const std::string &): n/a
  - `domain_keywords` (const DomainKeywords &): n/a
  - `domain_hint` (const std::string &): n/a

#### `double computePerplexityScore(const std::string &text)`
- Source: `src/training/lora_data_selection.cpp`:323
- Brief: Pseudo-perplexity estimate from token count and character entropy.
- Parameters:
  - `text` (const std::string &): Input parameter.
- Return: Return value.
- Details: text Input parameter. Return value. Calls: empty(), size(), std::log2(), std::min().

#### `double computeTTR(const std::string &text)`
- Source: `src/training/lora_data_selection.cpp`:247
- Brief: Compute type-token ratio (TTR) as diversity score.
- Parameters:
  - `text` (const std::string &): Input parameter.
- Return: Return value.
- Details: text Input parameter. Return value. Calls: iss(), std::transform(), begin(), end(), insert(), size().

#### `double computeToxicity(const std::string &text)`
- Source: `src/training/lora_data_selection.cpp`:115
- Brief: Heuristic toxicity score: counts hostile/offensive term occurrences and maps to [0.
- Parameters:
  - `text` (const std::string &): Input parameter.
- Return: Return value.
- Details: text Input parameter. Return value. .1]. Returns 0 for benign text. Calls: std::transform(), begin(), end(), find(), size(), std::min().

#### `bool containsPII(const std::string &text)`
- Source: `src/training/lora_data_selection.cpp`:142
- Brief: Heuristic PII check: returns true if text appears to contain PII.
- Parameters:
  - `text` (const std::string &): Input parameter.
- Return: True when the operation succeeds.
- Details: text Input parameter. True when the operation succeeds. Calls: std::transform(), begin(), end(), find().

#### `size_t countChar(const std::string &s, char c) noexcept`
- Source: `src/training/modality_parser.cpp`:57
- Brief: n/a
- Parameters:
  - `s` (const std::string &): n/a
  - `c` (char): n/a

#### `std::string detectLanguage(const std::string &text)`
- Source: `src/training/lora_data_selection.cpp`:89
- Brief: Very lightweight language detection based on common German stop words.
- Parameters:
  - `text` (const std::string &): Input parameter.
- Return: Return value.
- Details: text Input parameter. Return value. Returns "de" if text contains enough German indicators, else "other". Calls: std::transform(), begin(), end(), find().

#### `std::vector< TableBlock > detectTableBlocks(const std::vector< std::string > &lines)`
- Source: `src/training/modality_parser.cpp`:254
- Brief: Detect Table Blocks.
- Parameters:
  - `lines` (const std::vector< std::string > &): Input parameter.
- Return: Return value.
- Details: lines Input parameter. Return value.

#### `uint32_t fnv1a(const std::string &s)`
- Source: `src/training/lora_data_selection.cpp`:163
- Brief: Compute a lightweight FNV-1a hash of a string (for MinHash shingle hashing).
- Parameters:
  - `s` (const std::string &): Input parameter.
- Return: Return value.
- Details: s Input parameter. Return value. Implements fnv1a without additional internal calls.

#### `std::string hashConfig(const LoRADataSelectionConfig &cfg)`
- Source: `src/training/lora_data_selection.cpp`:352
- Brief: Build a compact FNV hash string for the config (provenance fingerprint).
- Parameters:
  - `cfg` (const LoRADataSelectionConfig &): Input parameter.
- Return: Return value.
- Details: cfg Input parameter. Return value. Calls: fnv1a(), str().

#### `bool isAlignedTableRow(const std::string &line)`
- Source: `src/training/modality_parser.cpp`:125
- Brief: Check whether a line looks like a whitespace-aligned table line: multiple consecutive-space runs of ≥3 characters separating words.
- Parameters:
  - `line` (const std::string &): Input parameter.
- Return: True when the operation succeeds.
- Details: line Input parameter. True when the operation succeeds. Calls: size().

#### `bool isPipeTableRow(const std::string &line)`
- Source: `src/training/modality_parser.cpp`:92
- Brief: Return true if the line looks like part of a pipe-delimited table row.
- Parameters:
  - `line` (const std::string &): Input parameter.
- Return: True when the operation succeeds.
- Details: line Input parameter. True when the operation succeeds. Calls: themis::utils::trim(), empty(), front(), back(), countChar().

#### `bool isTableSeparator(const std::string &line)`
- Source: `src/training/modality_parser.cpp`:106
- Brief: Return true if the line consists primarily of dashes (table separator).
- Parameters:
  - `line` (const std::string &): Input parameter.
- Return: True when the operation succeeds.
- Details: line Input parameter. True when the operation succeeds. Calls: themis::utils::trim(), empty(), countChar().

#### `double jaccardEstimate(const std::vector< uint32_t > &a, const std::vector< uint32_t > &b)`
- Source: `src/training/lora_data_selection.cpp`:227
- Brief: Estimate Jaccard similarity from two MinHash signatures.
- Parameters:
  - `a` (const std::vector< uint32_t > &): Input parameter.
  - `b` (const std::vector< uint32_t > &): Input parameter.
- Return: Return value.
- Details: a Input parameter. b Input parameter. Return value. Calls: size(), empty().

#### `std::vector< float > kaimingUniform(size_t size, size_t fan_in, uint32_t seed)`
- Source: `src/training/lora_adapter.cpp`:40
- Brief: Kaiming Uniform.
- Parameters:
  - `size` (size_t): Input parameter.
  - `fan_in` (size_t): Input parameter.
  - `seed` (uint32_t): Input parameter.
- Return: Return value.
- Details: size Input parameter. fan_in Input parameter. seed Input parameter. Return value. Calls: w(), gen(), std::sqrt(), dist().

#### `std::vector< float > matmul(const std::vector< float > &A, size_t M, size_t K, const std::vector< float > &B, size_t N)`
- Source: `src/training/lora_adapter.cpp`:79
- Brief: Matmul.
- Parameters:
  - `A` (const std::vector< float > &): Input parameter.
  - `M` (size_t): Input parameter.
  - `K` (size_t): Input parameter.
  - `B` (const std::vector< float > &): Input parameter.
  - `N` (size_t): Input parameter.
- Return: Return value.
- Details: A Input parameter. M Input parameter. K Input parameter. B Input parameter. N Input parameter. Return value. Calls: assert(), size(), C().

#### `bool sanitizeTrainingPromptSurface(const std::string &input, std::string &sanitized, std::string *blocked_rule, std::string *blocked_reason)`
- Source: `src/training/modality_parser.cpp`:43
- Brief: Sanitize Training Prompt Surface.
- Parameters:
  - `input` (const std::string &): Input parameter.
  - `sanitized` (std::string &): Input/output parameter.
  - `blocked_rule` (std::string *): Input/output parameter.
  - `blocked_reason` (std::string *): Input/output parameter.
- Return: True when the operation succeeds.
- Details: input Input parameter. sanitized Input/output parameter. blocked_rule Input/output parameter. blocked_reason Input/output parameter. True when the operation succeeds.

#### `bool sanitizeTrainingText(const std::string &input, std::string &sanitized, std::string *blocked_rule, std::string *blocked_reason)`
- Source: `src/training/lora_data_selection.cpp`:50
- Brief: Sanitize Training Text.
- Parameters:
  - `input` (const std::string &): Input parameter.
  - `sanitized` (std::string &): Input/output parameter.
  - `blocked_rule` (std::string *): Input/output parameter.
  - `blocked_reason` (std::string *): Input/output parameter.
- Return: True when the operation succeeds.
- Details: input Input parameter. sanitized Input/output parameter. blocked_rule Input/output parameter. blocked_reason Input/output parameter. True when the operation succeeds.

#### `uint32_t seedFromName(const std::string &name)`
- Source: `src/training/lora_adapter.cpp`:60
- Brief: Seed From Name.
- Parameters:
  - `name` (const std::string &): Input parameter.
- Return: Return value.
- Details: name Input parameter. Return value. Implements seedFromName without additional internal calls.

#### `std::vector< std::string > splitLines(const std::string &text)`
- Source: `src/training/modality_parser.cpp`:73
- Brief: Split a string by a delimiter character.
- Parameters:
  - `text` (const std::string &): Input parameter.
- Return: Return value.
- Details: text Input parameter. Return value. Calls: stream(), std::getline(), push_back(), std::move().

#### `std::vector< std::string > splitSentences(const std::string &text)`
- Source: `src/training/modality_parser.cpp`:185
- Brief: Split text into sentences using common German legal sentence boundaries.
- Parameters:
  - `text` (const std::string &): Input parameter.
- Return: Return value.
- Details: text Input parameter. Return value. Avoids splitting on abbreviations ("Abs.", "Nr.", "Art.", numbers). Calls: empty(), RE_ABBREV(), reserve(), size(), std::isupper(), themis::utils::trim(), std::regex_search(), push_back().

### themis::training::yaml_detail

#### `void applyScalar(LoRADataSelectionConfig &cfg, const std::string &key, const std::string &raw_val)`
- Source: `src/training/lora_data_selection.cpp`:1010
- Brief: Assign a scalar YAML value to the matching field of cfg.
- Parameters:
  - `cfg` (LoRADataSelectionConfig &): Input/output parameter.
  - `key` (const std::string &): Input parameter.
  - `raw_val` (const std::string &): Input parameter.
- Throws:
  - std::runtime_error: if an error occurs.
- Details: cfg Input/output parameter. key Input parameter. raw_val Input parameter. std::runtime_error if an error occurs. Calls: stripQuotes(), trimLeft(), empty(), std::stoull(), std::stod(), what().

#### `SelfImprovementConfig parseSelfImprovementYAML(const std::string &text, const std::string &section)`
- Source: `src/training/lora_data_selection.cpp`:1274
- Brief: Parse Self Improvement YAML.
- Parameters:
  - `text` (const std::string &): Input parameter.
  - `section` (const std::string &): Input parameter.
- Return: Return value.
- Throws:
  - std::runtime_error: if an error occurs.
- Details: text Input parameter. section Input parameter. Return value. std::runtime_error if an error occurs. Calls: SelfImprovementConfig(), iss(), AdaptiveRule(), empty(), push_back(), std::getline(), trimRight(), removeComment().

#### `LoRADataSelectionConfig parseYAMLText(const std::string &text, const std::string &section)`
- Source: `src/training/lora_data_selection.cpp`:1057
- Brief: Parse YAMLText.
- Parameters:
  - `text` (const std::string &): Input parameter.
  - `section` (const std::string &): Input parameter.
- Return: Return value.
- Details: text Input parameter. section Input parameter. Return value. Calls: LoRADataSelectionConfig(), iss(), std::getline(), trimRight(), removeComment(), empty(), size(), substr().

#### `std::string removeComment(const std::string &s)`
- Source: `src/training/lora_data_selection.cpp`:988
- Brief: Remove Comment.
- Parameters:
  - `s` (const std::string &): Input parameter.
- Return: Return value.
- Details: s Input parameter. Return value. Calls: size(), substr().

#### `std::string stripQuotes(const std::string &s)`
- Source: `src/training/lora_data_selection.cpp`:974
- Brief: Strip Quotes.
- Parameters:
  - `s` (const std::string &): Input parameter.
- Return: Return value.
- Details: s Input parameter. Return value. Calls: size(), front(), back(), substr().

#### `std::string trimLeft(const std::string &s)`
- Source: `src/training/lora_data_selection.cpp`:960
- Brief: Trim Left.
- Parameters:
  - `s` (const std::string &): Input parameter.
- Return: Return value.
- Details: s Input parameter. Return value. Calls: size(), substr().

#### `std::string trimRight(std::string s)`
- Source: `src/training/lora_data_selection.cpp`:948
- Brief: Trim Right.
- Parameters:
  - `s` (std::string): Input parameter.
- Return: Return value.
- Details: s Input parameter. Return value. Calls: empty(), back(), pop_back().

