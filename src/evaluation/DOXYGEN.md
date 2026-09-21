# EVALUATION DOXYGEN

Author: ThemisDB Contributors
Created: 2026-09-20
Last Updated: 2026-09-20
Status: active

## Source
- This file is generated from module-scoped Doxygen XML.
- XML index: `C:\Projects\ThemisDB\.tmp\module_doxygen_global_e2e\evaluation\xml\index.xml`
- Warnings log: `C:\Projects\ThemisDB\.tmp\module_doxygen_global_e2e\evaluation\doxygen-warnings.log`

## Structure Metrics
- C/C++ Files (scanned): 21
- Compounds: 88
- Classes/Structs: 47
- Namespaces: 10
- File Compounds: 21

## Namespaces
- @002276072133101103132351126060376144072146012147
- @150123117004047174025066317366336231340131333017
- std
- themis
- themis::evaluation
- themis::evaluation::@125056133326231214200176341270221143324233276016
- themis::evaluation::@127275355372217344014074020015040113153124173156
- themis::evaluation::@220257120155045327240072173322026322232316155171
- themis::evaluation::@234357071373162255261116132103335120262231232261
- themis::evaluation::@256263074225077107263060060241310206177227143355

## Types
### Classes
- themis::evaluation::AblationError
- themis::evaluation::AblationRunner
- themis::evaluation::ApproximationRuleEngine
- themis::evaluation::ArtifactLifecycleManager
- themis::evaluation::BenchmarkMatrix
- themis::evaluation::DefaultApproximationRuleEngine
- themis::evaluation::DefaultQueryPlanner
- themis::evaluation::HardwareProfileRegistry
- themis::evaluation::MetricCollector
- themis::evaluation::MetricError
- themis::evaluation::PlannerObserver
- themis::evaluation::QueryPlanner
- themis::evaluation::StalenessPolicy

### Structs
- themis::evaluation::AblationConfig
- themis::evaluation::AblationQuery
- themis::evaluation::AblationReport
- themis::evaluation::AblationResult
- themis::evaluation::AblationRunner::Experiment
- themis::evaluation::ApproximationBoundary
- themis::evaluation::ApproximationPolicy
- themis::evaluation::BenchmarkEntry
- themis::evaluation::BenchmarkMatrix::Key
- themis::evaluation::BenchmarkMatrix::KeyHash
- themis::evaluation::BenchmarkResult
- themis::evaluation::BoundaryCheckResult
- themis::evaluation::CompressionMetrics
- themis::evaluation::DistributedEfficiencyMetrics
- themis::evaluation::EvidenceQualityMetrics
- themis::evaluation::ExecutionEligibility
- themis::evaluation::HardwareProfile
- themis::evaluation::HardwareProfileValidationResult
- themis::evaluation::LayerSizingRule
- themis::evaluation::LifecycleMetadata
- themis::evaluation::LlmAnswerQualityMetrics
- themis::evaluation::PlannerConfig
- themis::evaluation::PlannerDecision
- themis::evaluation::ProvenanceAssertion
- themis::evaluation::ProvenanceQualityMetrics
- themis::evaluation::RankedResult
- themis::evaluation::ResourceSizingBand
- themis::evaluation::RetrievalQualityMetrics
- themis::evaluation::TensorArtifactFreshness
- themis::evaluation::TensorGraphRuntimeMetrics
- themis::evaluation::TensorGraphSnapshot
- themis::evaluation::TierTransitionRequest
- themis::evaluation::TierTransitionResult
- themis::evaluation::TieringPolicy

## Notes
- This is a derived artifact. Do not manually edit semantic content.
- Regenerate via scripts/generate_module_doxygen_xml.py --write-module-markdown.

## Detailed Function Documentation (Soll-Ist)
- Functions extracted: 224

### bench_evaluation_dedicated_gates.cpp

#### `BENCHMARK_MAIN()`
- Source: `benchmarks/evaluation/bench_evaluation_dedicated_gates.cpp`:147
- Brief: n/a
- Parameters: none

#### `void BM_EV_BM_01_MetricComputationThroughput(benchmark::State &state)`
- Source: `benchmarks/evaluation/bench_evaluation_dedicated_gates.cpp`:80
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_EV_BM_02_AggregationLatency(benchmark::State &state)`
- Source: `benchmarks/evaluation/bench_evaluation_dedicated_gates.cpp`:99
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_EV_BM_03_HarnessInvocation(benchmark::State &state)`
- Source: `benchmarks/evaluation/bench_evaluation_dedicated_gates.cpp`:116
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_EV_BM_04_JudgeRequestLatency(benchmark::State &state)`
- Source: `benchmarks/evaluation/bench_evaluation_dedicated_gates.cpp`:134
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `MinTime(1.0) -> UseRealTime()`
- Source: `benchmarks/evaluation/bench_evaluation_dedicated_gates.cpp`:109
- Brief: n/a
- Parameters:
  - `0` (1.): n/a

#### `Threads(8) -> MinTime(1.0) ->UseRealTime()`
- Source: `benchmarks/evaluation/bench_evaluation_dedicated_gates.cpp`:91
- Brief: n/a
- Parameters:
  - `<unnamed>` (8): n/a

### test_evaluation_ablation_framework_focused.cpp

#### `TEST(EvaluationAblationFrameworkFocusedTests, AF10_Reset_ClearsExperiments)`
- Source: `tests/evaluation/test_evaluation_ablation_framework_focused.cpp`:138
- Brief: n/a
- Parameters:
  - `<unnamed>` (EvaluationAblationFrameworkFocusedTests): n/a
  - `<unnamed>` (AF10_Reset_ClearsExperiments): n/a

#### `TEST(EvaluationAblationFrameworkFocusedTests, AF11_ResultName_MatchesRegisteredName)`
- Source: `tests/evaluation/test_evaluation_ablation_framework_focused.cpp`:146
- Brief: n/a
- Parameters:
  - `<unnamed>` (EvaluationAblationFrameworkFocusedTests): n/a
  - `<unnamed>` (AF11_ResultName_MatchesRegisteredName): n/a

#### `TEST(EvaluationAblationFrameworkFocusedTests, AF1_InitialExperimentCount_IsZero)`
- Source: `tests/evaluation/test_evaluation_ablation_framework_focused.cpp`:52
- Brief: n/a
- Parameters:
  - `<unnamed>` (EvaluationAblationFrameworkFocusedTests): n/a
  - `<unnamed>` (AF1_InitialExperimentCount_IsZero): n/a

#### `TEST(EvaluationAblationFrameworkFocusedTests, AF2_AddExperiment_IncrementsCount)`
- Source: `tests/evaluation/test_evaluation_ablation_framework_focused.cpp`:58
- Brief: n/a
- Parameters:
  - `<unnamed>` (EvaluationAblationFrameworkFocusedTests): n/a
  - `<unnamed>` (AF2_AddExperiment_IncrementsCount): n/a

#### `TEST(EvaluationAblationFrameworkFocusedTests, AF3_EmptyExperimentName_ThrowsAblationError)`
- Source: `tests/evaluation/test_evaluation_ablation_framework_focused.cpp`:65
- Brief: n/a
- Parameters:
  - `<unnamed>` (EvaluationAblationFrameworkFocusedTests): n/a
  - `<unnamed>` (AF3_EmptyExperimentName_ThrowsAblationError): n/a

#### `TEST(EvaluationAblationFrameworkFocusedTests, AF4_DuplicateExperimentName_ThrowsAblationError)`
- Source: `tests/evaluation/test_evaluation_ablation_framework_focused.cpp`:74
- Brief: n/a
- Parameters:
  - `<unnamed>` (EvaluationAblationFrameworkFocusedTests): n/a
  - `<unnamed>` (AF4_DuplicateExperimentName_ThrowsAblationError): n/a

#### `TEST(EvaluationAblationFrameworkFocusedTests, AF5_RunEmptyQueryBatch_ThrowsAblationError)`
- Source: `tests/evaluation/test_evaluation_ablation_framework_focused.cpp`:84
- Brief: n/a
- Parameters:
  - `<unnamed>` (EvaluationAblationFrameworkFocusedTests): n/a
  - `<unnamed>` (AF5_RunEmptyQueryBatch_ThrowsAblationError): n/a

#### `TEST(EvaluationAblationFrameworkFocusedTests, AF6_RunNoExperiments_ThrowsAblationError)`
- Source: `tests/evaluation/test_evaluation_ablation_framework_focused.cpp`:91
- Brief: n/a
- Parameters:
  - `<unnamed>` (EvaluationAblationFrameworkFocusedTests): n/a
  - `<unnamed>` (AF6_RunNoExperiments_ThrowsAblationError): n/a

#### `TEST(EvaluationAblationFrameworkFocusedTests, AF7_Run_ReturnsOneResultPerExperiment)`
- Source: `tests/evaluation/test_evaluation_ablation_framework_focused.cpp`:98
- Brief: n/a
- Parameters:
  - `<unnamed>` (EvaluationAblationFrameworkFocusedTests): n/a
  - `<unnamed>` (AF7_Run_ReturnsOneResultPerExperiment): n/a

#### `TEST(EvaluationAblationFrameworkFocusedTests, AF8_PerfectRecall_MeanRecallIsOne)`
- Source: `tests/evaluation/test_evaluation_ablation_framework_focused.cpp`:110
- Brief: n/a
- Parameters:
  - `<unnamed>` (EvaluationAblationFrameworkFocusedTests): n/a
  - `<unnamed>` (AF8_PerfectRecall_MeanRecallIsOne): n/a

#### `TEST(EvaluationAblationFrameworkFocusedTests, AF9_QueryCount_MatchesInput)`
- Source: `tests/evaluation/test_evaluation_ablation_framework_focused.cpp`:122
- Brief: n/a
- Parameters:
  - `<unnamed>` (EvaluationAblationFrameworkFocusedTests): n/a
  - `<unnamed>` (AF9_QueryCount_MatchesInput): n/a

#### `AblationConfig make_config(PathVariant path)`
- Source: `tests/evaluation/test_evaluation_ablation_framework_focused.cpp`:40
- Brief: n/a
- Parameters:
  - `path` (PathVariant): n/a

#### `AblationQuery make_simple_query(const std::string &qid, bool relevant_first)`
- Source: `tests/evaluation/test_evaluation_ablation_framework_focused.cpp`:19
- Brief: n/a
- Parameters:
  - `qid` (const std::string &): n/a
  - `relevant_first` (bool): n/a

### test_evaluation_artifact_lifecycle_focused.cpp

#### `TEST(EvaluationArtifactLifecycleFocusedTests, AL10_PristineArtifact_StaysPristine)`
- Source: `tests/evaluation/test_evaluation_artifact_lifecycle_focused.cpp`:132
- Brief: n/a
- Parameters:
  - `<unnamed>` (EvaluationArtifactLifecycleFocusedTests): n/a
  - `<unnamed>` (AL10_PristineArtifact_StaysPristine): n/a

#### `TEST(EvaluationArtifactLifecycleFocusedTests, AL1_ReadyArtifact_BelowThresholds_StaysReady)`
- Source: `tests/evaluation/test_evaluation_artifact_lifecycle_focused.cpp`:34
- Brief: n/a
- Parameters:
  - `<unnamed>` (EvaluationArtifactLifecycleFocusedTests): n/a
  - `<unnamed>` (AL1_ReadyArtifact_BelowThresholds_StaysReady): n/a

#### `TEST(EvaluationArtifactLifecycleFocusedTests, AL2_ReadyArtifact_AgeExceeded_BecomesStale)`
- Source: `tests/evaluation/test_evaluation_artifact_lifecycle_focused.cpp`:47
- Brief: n/a
- Parameters:
  - `<unnamed>` (EvaluationArtifactLifecycleFocusedTests): n/a
  - `<unnamed>` (AL2_ReadyArtifact_AgeExceeded_BecomesStale): n/a

#### `TEST(EvaluationArtifactLifecycleFocusedTests, AL3_ReadyArtifact_DeltaLagExceeded_BecomesStale)`
- Source: `tests/evaluation/test_evaluation_artifact_lifecycle_focused.cpp`:59
- Brief: n/a
- Parameters:
  - `<unnamed>` (EvaluationArtifactLifecycleFocusedTests): n/a
  - `<unnamed>` (AL3_ReadyArtifact_DeltaLagExceeded_BecomesStale): n/a

#### `TEST(EvaluationArtifactLifecycleFocusedTests, AL4_InvalidatedArtifact_StaysInvalidated)`
- Source: `tests/evaluation/test_evaluation_artifact_lifecycle_focused.cpp`:71
- Brief: n/a
- Parameters:
  - `<unnamed>` (EvaluationArtifactLifecycleFocusedTests): n/a
  - `<unnamed>` (AL4_InvalidatedArtifact_StaysInvalidated): n/a

#### `TEST(EvaluationArtifactLifecycleFocusedTests, AL5_IsUsableForPlanning_ReadyAndStale)`
- Source: `tests/evaluation/test_evaluation_artifact_lifecycle_focused.cpp`:81
- Brief: n/a
- Parameters:
  - `<unnamed>` (EvaluationArtifactLifecycleFocusedTests): n/a
  - `<unnamed>` (AL5_IsUsableForPlanning_ReadyAndStale): n/a

#### `TEST(EvaluationArtifactLifecycleFocusedTests, AL6_RequiresImmediateRebuild_InvalidatedAndFailed)`
- Source: `tests/evaluation/test_evaluation_artifact_lifecycle_focused.cpp`:90
- Brief: n/a
- Parameters:
  - `<unnamed>` (EvaluationArtifactLifecycleFocusedTests): n/a
  - `<unnamed>` (AL6_RequiresImmediateRebuild_InvalidatedAndFailed): n/a

#### `TEST(EvaluationArtifactLifecycleFocusedTests, AL7_Invalidate_SetsStateAndReason)`
- Source: `tests/evaluation/test_evaluation_artifact_lifecycle_focused.cpp`:99
- Brief: n/a
- Parameters:
  - `<unnamed>` (EvaluationArtifactLifecycleFocusedTests): n/a
  - `<unnamed>` (AL7_Invalidate_SetsStateAndReason): n/a

#### `TEST(EvaluationArtifactLifecycleFocusedTests, AL8_BeginRebuild_SetsRebuildingAndIncrementsCount)`
- Source: `tests/evaluation/test_evaluation_artifact_lifecycle_focused.cpp`:110
- Brief: n/a
- Parameters:
  - `<unnamed>` (EvaluationArtifactLifecycleFocusedTests): n/a
  - `<unnamed>` (AL8_BeginRebuild_SetsRebuildingAndIncrementsCount): n/a

#### `TEST(EvaluationArtifactLifecycleFocusedTests, AL9_ResidualThreshold_TriggersStaleness)`
- Source: `tests/evaluation/test_evaluation_artifact_lifecycle_focused.cpp`:120
- Brief: n/a
- Parameters:
  - `<unnamed>` (EvaluationArtifactLifecycleFocusedTests): n/a
  - `<unnamed>` (AL9_ResidualThreshold_TriggersStaleness): n/a

#### `LifecycleMetadata make_ready_metadata(const std::string &id="art1")`
- Source: `tests/evaluation/test_evaluation_artifact_lifecycle_focused.cpp`:20
- Brief: n/a
- Parameters:
  - `id` (const std::string &): n/a

### test_evaluation_benchmark_matrix_focused.cpp

#### `TEST(EvaluationBenchmarkMatrixFocusedTests, BM1_RecordAndLookup_Roundtrip)`
- Source: `tests/evaluation/test_evaluation_benchmark_matrix_focused.cpp`:25
- Brief: n/a
- Parameters:
  - `<unnamed>` (EvaluationBenchmarkMatrixFocusedTests): n/a
  - `<unnamed>` (BM1_RecordAndLookup_Roundtrip): n/a

#### `TEST(EvaluationBenchmarkMatrixFocusedTests, BM2_Lookup_Missing_ReturnsNullopt)`
- Source: `tests/evaluation/test_evaluation_benchmark_matrix_focused.cpp`:36
- Brief: n/a
- Parameters:
  - `<unnamed>` (EvaluationBenchmarkMatrixFocusedTests): n/a
  - `<unnamed>` (BM2_Lookup_Missing_ReturnsNullopt): n/a

#### `TEST(EvaluationBenchmarkMatrixFocusedTests, BM3_RecordZeroSamples_ThrowsInvalidArgument)`
- Source: `tests/evaluation/test_evaluation_benchmark_matrix_focused.cpp`:43
- Brief: n/a
- Parameters:
  - `<unnamed>` (EvaluationBenchmarkMatrixFocusedTests): n/a
  - `<unnamed>` (BM3_RecordZeroSamples_ThrowsInvalidArgument): n/a

#### `TEST(EvaluationBenchmarkMatrixFocusedTests, BM4_Overwrite_ReplacesOldResult)`
- Source: `tests/evaluation/test_evaluation_benchmark_matrix_focused.cpp`:55
- Brief: n/a
- Parameters:
  - `<unnamed>` (EvaluationBenchmarkMatrixFocusedTests): n/a
  - `<unnamed>` (BM4_Overwrite_ReplacesOldResult): n/a

#### `TEST(EvaluationBenchmarkMatrixFocusedTests, BM5_InvalidateScenario_RemovesEntries)`
- Source: `tests/evaluation/test_evaluation_benchmark_matrix_focused.cpp`:67
- Brief: n/a
- Parameters:
  - `<unnamed>` (EvaluationBenchmarkMatrixFocusedTests): n/a
  - `<unnamed>` (BM5_InvalidateScenario_RemovesEntries): n/a

#### `TEST(EvaluationBenchmarkMatrixFocusedTests, BM6_InvalidateDimension_RemovesEntries)`
- Source: `tests/evaluation/test_evaluation_benchmark_matrix_focused.cpp`:86
- Brief: n/a
- Parameters:
  - `<unnamed>` (EvaluationBenchmarkMatrixFocusedTests): n/a
  - `<unnamed>` (BM6_InvalidateDimension_RemovesEntries): n/a

#### `TEST(EvaluationBenchmarkMatrixFocusedTests, BM7_MultipleEntries_Coexist)`
- Source: `tests/evaluation/test_evaluation_benchmark_matrix_focused.cpp`:106
- Brief: n/a
- Parameters:
  - `<unnamed>` (EvaluationBenchmarkMatrixFocusedTests): n/a
  - `<unnamed>` (BM7_MultipleEntries_Coexist): n/a

#### `TEST(EvaluationBenchmarkMatrixFocusedTests, BM8_Size_ReflectsRecordedEntries)`
- Source: `tests/evaluation/test_evaluation_benchmark_matrix_focused.cpp`:121
- Brief: n/a
- Parameters:
  - `<unnamed>` (EvaluationBenchmarkMatrixFocusedTests): n/a
  - `<unnamed>` (BM8_Size_ReflectsRecordedEntries): n/a

#### `TEST(EvaluationBenchmarkMatrixFocusedTests, BM9_Clear_EmptiesMatrix)`
- Source: `tests/evaluation/test_evaluation_benchmark_matrix_focused.cpp`:130
- Brief: n/a
- Parameters:
  - `<unnamed>` (EvaluationBenchmarkMatrixFocusedTests): n/a
  - `<unnamed>` (BM9_Clear_EmptiesMatrix): n/a

#### `BenchmarkResult make_result(double value, uint32_t samples=1)`
- Source: `tests/evaluation/test_evaluation_benchmark_matrix_focused.cpp`:14
- Brief: n/a
- Parameters:
  - `value` (double): n/a
  - `samples` (uint32_t): n/a

### test_evaluation_highcardinality_stress.cpp

#### `TEST(EvaluationHighCardinalityStress, ConcurrentBenchmarkHarnessStress)`
- Source: `tests/evaluation/test_evaluation_highcardinality_stress.cpp`:107
- Brief: ConcurrentBenchmarkHarnessStress.
- Parameters:
  - `<unnamed>` (EvaluationHighCardinalityStress): n/a
  - `<unnamed>` (ConcurrentBenchmarkHarnessStress): n/a
- Details: Runs 8 threads each executing 50 000 harness calls. Asserts no crashes and all invocations complete successfully.

#### `TEST(EvaluationHighCardinalityStress, HighCardinalityMetricEval)`
- Source: `tests/evaluation/test_evaluation_highcardinality_stress.cpp`:77
- Brief: HighCardinalityMetricEval.
- Parameters:
  - `<unnamed>` (EvaluationHighCardinalityStress): n/a
  - `<unnamed>` (HighCardinalityMetricEval): n/a
- Details: Evaluates 10 000 samples per thread across 8 threads (80 000 total). Asserts all samples are processed with no silent drops.

#### `TEST(EvaluationHighCardinalityStress, ResultAggregationStress)`
- Source: `tests/evaluation/test_evaluation_highcardinality_stress.cpp`:137
- Brief: ResultAggregationStress.
- Parameters:
  - `<unnamed>` (EvaluationHighCardinalityStress): n/a
  - `<unnamed>` (ResultAggregationStress): n/a
- Details: Aggregates batches of 1 000 results, 500 times per thread, from 4 threads. Asserts zero aggregation overflows.

### test_evaluation_retrieval_metrics_focused.cpp

#### `TEST(EvaluationRetrievalMetricsFocusedTests, RM10_CandidateReduction_ZeroWhenTotalIsZero)`
- Source: `tests/evaluation/test_evaluation_retrieval_metrics_focused.cpp`:122
- Brief: n/a
- Parameters:
  - `<unnamed>` (EvaluationRetrievalMetricsFocusedTests): n/a
  - `<unnamed>` (RM10_CandidateReduction_ZeroWhenTotalIsZero): n/a

#### `TEST(EvaluationRetrievalMetricsFocusedTests, RM11_MetricCollector_AccumulatesSnapshots)`
- Source: `tests/evaluation/test_evaluation_retrieval_metrics_focused.cpp`:130
- Brief: n/a
- Parameters:
  - `<unnamed>` (EvaluationRetrievalMetricsFocusedTests): n/a
  - `<unnamed>` (RM11_MetricCollector_AccumulatesSnapshots): n/a

#### `TEST(EvaluationRetrievalMetricsFocusedTests, RM12_MetricCollector_ResetClearsState)`
- Source: `tests/evaluation/test_evaluation_retrieval_metrics_focused.cpp`:149
- Brief: n/a
- Parameters:
  - `<unnamed>` (EvaluationRetrievalMetricsFocusedTests): n/a
  - `<unnamed>` (RM12_MetricCollector_ResetClearsState): n/a

#### `TEST(EvaluationRetrievalMetricsFocusedTests, RM13_MetricCollector_NoSnapshots_Throws)`
- Source: `tests/evaluation/test_evaluation_retrieval_metrics_focused.cpp`:162
- Brief: n/a
- Parameters:
  - `<unnamed>` (EvaluationRetrievalMetricsFocusedTests): n/a
  - `<unnamed>` (RM13_MetricCollector_NoSnapshots_Throws): n/a

#### `TEST(EvaluationRetrievalMetricsFocusedTests, RM14_MetricError_KindIsPreserved)`
- Source: `tests/evaluation/test_evaluation_retrieval_metrics_focused.cpp`:168
- Brief: n/a
- Parameters:
  - `<unnamed>` (EvaluationRetrievalMetricsFocusedTests): n/a
  - `<unnamed>` (RM14_MetricError_KindIsPreserved): n/a

#### `TEST(EvaluationRetrievalMetricsFocusedTests, RM1_PerfectRecall)`
- Source: `tests/evaluation/test_evaluation_retrieval_metrics_focused.cpp`:43
- Brief: n/a
- Parameters:
  - `<unnamed>` (EvaluationRetrievalMetricsFocusedTests): n/a
  - `<unnamed>` (RM1_PerfectRecall): n/a

#### `TEST(EvaluationRetrievalMetricsFocusedTests, RM2_ZeroRecall)`
- Source: `tests/evaluation/test_evaluation_retrieval_metrics_focused.cpp`:51
- Brief: n/a
- Parameters:
  - `<unnamed>` (EvaluationRetrievalMetricsFocusedTests): n/a
  - `<unnamed>` (RM2_ZeroRecall): n/a

#### `TEST(EvaluationRetrievalMetricsFocusedTests, RM3_PartialRecall)`
- Source: `tests/evaluation/test_evaluation_retrieval_metrics_focused.cpp`:59
- Brief: n/a
- Parameters:
  - `<unnamed>` (EvaluationRetrievalMetricsFocusedTests): n/a
  - `<unnamed>` (RM3_PartialRecall): n/a

#### `TEST(EvaluationRetrievalMetricsFocusedTests, RM4_MRR_FirstRelevant)`
- Source: `tests/evaluation/test_evaluation_retrieval_metrics_focused.cpp`:67
- Brief: n/a
- Parameters:
  - `<unnamed>` (EvaluationRetrievalMetricsFocusedTests): n/a
  - `<unnamed>` (RM4_MRR_FirstRelevant): n/a

#### `TEST(EvaluationRetrievalMetricsFocusedTests, RM5_MRR_SecondRelevant)`
- Source: `tests/evaluation/test_evaluation_retrieval_metrics_focused.cpp`:75
- Brief: n/a
- Parameters:
  - `<unnamed>` (EvaluationRetrievalMetricsFocusedTests): n/a
  - `<unnamed>` (RM5_MRR_SecondRelevant): n/a

#### `TEST(EvaluationRetrievalMetricsFocusedTests, RM6_EmptyGroundTruth_ThrowsMetricError)`
- Source: `tests/evaluation/test_evaluation_retrieval_metrics_focused.cpp`:83
- Brief: n/a
- Parameters:
  - `<unnamed>` (EvaluationRetrievalMetricsFocusedTests): n/a
  - `<unnamed>` (RM6_EmptyGroundTruth_ThrowsMetricError): n/a

#### `TEST(EvaluationRetrievalMetricsFocusedTests, RM7_KZero_ThrowsMetricError)`
- Source: `tests/evaluation/test_evaluation_retrieval_metrics_focused.cpp`:92
- Brief: n/a
- Parameters:
  - `<unnamed>` (EvaluationRetrievalMetricsFocusedTests): n/a
  - `<unnamed>` (RM7_KZero_ThrowsMetricError): n/a

#### `TEST(EvaluationRetrievalMetricsFocusedTests, RM8_KExceedsRankedSize_ThrowsMetricError)`
- Source: `tests/evaluation/test_evaluation_retrieval_metrics_focused.cpp`:102
- Brief: n/a
- Parameters:
  - `<unnamed>` (EvaluationRetrievalMetricsFocusedTests): n/a
  - `<unnamed>` (RM8_KExceedsRankedSize_ThrowsMetricError): n/a

#### `TEST(EvaluationRetrievalMetricsFocusedTests, RM9_DuplicateIds_ThrowsMetricError)`
- Source: `tests/evaluation/test_evaluation_retrieval_metrics_focused.cpp`:112
- Brief: n/a
- Parameters:
  - `<unnamed>` (EvaluationRetrievalMetricsFocusedTests): n/a
  - `<unnamed>` (RM9_DuplicateIds_ThrowsMetricError): n/a

#### `std::vector< std::string > make_gt(std::initializer_list< const char * > ids)`
- Source: `tests/evaluation/test_evaluation_retrieval_metrics_focused.cpp`:30
- Brief: n/a
- Parameters:
  - `ids` (std::initializer_list< const char * >): n/a

#### `std::vector< RankedResult > make_ranked(std::initializer_list< const char * > ids)`
- Source: `tests/evaluation/test_evaluation_retrieval_metrics_focused.cpp`:19
- Brief: n/a
- Parameters:
  - `ids` (std::initializer_list< const char * >): n/a

### themis::evaluation

#### `CompressionMetrics computeCompressionMetrics(std::size_t original_size_bytes, std::size_t compressed_size_bytes, const std::vector< double > &approximation_errors, const std::vector< int > &rank_samples)`
- Source: `src/evaluation/src/retrieval_metrics.cc`:388
- Brief: Compute Compression Metrics.
- Parameters:
  - `original_size_bytes` (std::size_t): Input parameter.
  - `compressed_size_bytes` (std::size_t): Input parameter.
  - `approximation_errors` (const std::vector< double > &): Input parameter.
  - `rank_samples` (const std::vector< int > &): Input parameter.
- Return: Return value.
- Details: original_size_bytes Input parameter. compressed_size_bytes Input parameter. approximation_errors Input parameter. rank_samples Input parameter. Return value.

#### `DistributedEfficiencyMetrics computeDistributedEfficiency(const std::vector< uint32_t > &per_query_shard_counts, const std::vector< double > &per_query_bytes, const std::vector< uint32_t > &summary_skipped_shards, uint32_t total_shards)`
- Source: `src/evaluation/src/retrieval_metrics.cc`:517
- Brief: Compute Distributed Efficiency.
- Parameters:
  - `per_query_shard_counts` (const std::vector< uint32_t > &): Input parameter.
  - `per_query_bytes` (const std::vector< double > &): Input parameter.
  - `summary_skipped_shards` (const std::vector< uint32_t > &): Input parameter.
  - `total_shards` (uint32_t): Input parameter.
- Return: Return value.
- Details: per_query_shard_counts Input parameter. per_query_bytes Input parameter. summary_skipped_shards Input parameter. total_shards Input parameter. Return value.

#### `EvidenceQualityMetrics computeEvidenceQuality(const std::vector< std::string > &returned_evidence_ids, const std::vector< std::string > &required_evidence_ids, const std::vector< int > &hop_chain_lengths)`
- Source: `src/evaluation/src/retrieval_metrics.cc`:222
- Brief: Compute Evidence Quality.
- Parameters:
  - `returned_evidence_ids` (const std::vector< std::string > &): Input parameter.
  - `required_evidence_ids` (const std::vector< std::string > &): Input parameter.
  - `hop_chain_lengths` (const std::vector< int > &): Input parameter.
- Return: Return value.
- Details: returned_evidence_ids Input parameter. required_evidence_ids Input parameter. hop_chain_lengths Input parameter. Return value.

#### `LlmAnswerQualityMetrics computeLlmAnswerQuality(uint32_t supported_claims, uint32_t total_claims, uint32_t evidence_tokens, uint32_t prompt_token_count)`
- Source: `src/evaluation/src/retrieval_metrics.cc`:468
- Brief: Compute Llm Answer Quality.
- Parameters:
  - `supported_claims` (uint32_t): Input parameter.
  - `total_claims` (uint32_t): Input parameter.
  - `evidence_tokens` (uint32_t): Input parameter.
  - `prompt_token_count` (uint32_t): Input parameter.
- Return: Return value.
- Details: supported_claims Input parameter. total_claims Input parameter. evidence_tokens Input parameter. prompt_token_count Input parameter. Return value.

#### `ProvenanceQualityMetrics computeProvenanceQuality(const std::vector< ProvenanceAssertion > &returned, const std::vector< ProvenanceAssertion > &ground_truth)`
- Source: `src/evaluation/src/retrieval_metrics.cc`:291
- Brief: Compute Provenance Quality.
- Parameters:
  - `returned` (const std::vector< ProvenanceAssertion > &): Input parameter.
  - `ground_truth` (const std::vector< ProvenanceAssertion > &): Input parameter.
- Return: Return value.
- Details: returned Input parameter. ground_truth Input parameter. Return value.

#### `RetrievalQualityMetrics computeRetrievalQuality(const std::vector< RankedResult > &ranked, const std::vector< std::string > &ground_truth, std::size_t k, std::size_t total_candidates)`
- Source: `src/evaluation/src/retrieval_metrics.cc`:123
- Brief: Compute Retrieval Quality.
- Parameters:
  - `ranked` (const std::vector< RankedResult > &): Input parameter.
  - `ground_truth` (const std::vector< std::string > &): Input parameter.
  - `k` (std::size_t): Input parameter.
  - `total_candidates` (std::size_t): Input parameter.
- Return: Return value.
- Details: ranked Input parameter. ground_truth Input parameter. k Input parameter. total_candidates Input parameter. Return value.

#### `TensorGraphRuntimeMetrics computeTensorGraphRuntimeMetrics(const std::vector< TensorGraphSnapshot > &snapshots, double max_residual_error)`
- Source: `src/evaluation/src/retrieval_metrics.cc`:585
- Brief: ============================================================================ § 7 Tensor-graph runtime metrics ============================================================================
- Parameters:
  - `snapshots` (const std::vector< TensorGraphSnapshot > &): Input parameter.
  - `max_residual_error` (double): Input parameter.
- Return: Return value.
- Details: snapshots Input parameter. max_residual_error Input parameter. Return value.

#### `std::vector< HardwareProfile > defaultHardwareProfiles()`
- Source: `src/evaluation/src/hardware_profile.cc`:202
- Brief: Default Hardware Profiles.
- Parameters: none
- Return: Return value.
- Details: Return value. Calls: developmentLayerRules(), productionLayerRules(), federatedLayerRules().

#### `std::string_view dimensionName(BenchmarkDimension d) noexcept`
- Source: `src/evaluation/include/benchmark_matrix.h`:146
- Brief: n/a
- Parameters:
  - `d` (BenchmarkDimension): n/a

#### `const HardwareProfile * findHardwareProfile(std::span< const HardwareProfile > profiles, DeploymentProfileId profile_id)`
- Source: `src/evaluation/src/hardware_profile.cc`:375
- Brief: Find Hardware Profile.
- Parameters:
  - `profiles` (std::span< const HardwareProfile >): Input parameter.
  - `profile_id` (DeploymentProfileId): Identifier of the profile.
- Return: Pointer to the result.
- Details: profiles Input parameter. profile_id Identifier of the profile. Pointer to the result. Calls: std::find_if(), begin(), end().

#### `const HardwareProfile * findHardwareProfile(std::span< const HardwareProfile > profiles, std::string_view profile_name)`
- Source: `src/evaluation/src/hardware_profile.cc`:392
- Brief: Find Hardware Profile.
- Parameters:
  - `profiles` (std::span< const HardwareProfile >): Input parameter.
  - `profile_name` (std::string_view): Name of the profile.
- Return: Pointer to the result.
- Details: profiles Input parameter. profile_name Name of the profile. Pointer to the result. Calls: parseDeploymentProfileId().

#### `const LayerSizingRule * findLayerSizingRule(const HardwareProfile &profile, LayerId layer)`
- Source: `src/evaluation/src/hardware_profile.cc`:407
- Brief: Find Layer Sizing Rule.
- Parameters:
  - `profile` (const HardwareProfile &): Input parameter.
  - `layer` (LayerId): Input parameter.
- Return: Pointer to the result.
- Details: profile Input parameter. layer Input parameter. Pointer to the result. Calls: std::find_if(), begin(), end().

#### `bool hasEdgeCase(BenchmarkEdgeCase flags, BenchmarkEdgeCase flag) noexcept`
- Source: `src/evaluation/include/benchmark_matrix.h`:193
- Brief: n/a
- Parameters:
  - `flags` (BenchmarkEdgeCase): n/a
  - `flag` (BenchmarkEdgeCase): n/a

#### `std::string invalidationReasonToString(InvalidationReason reason) noexcept`
- Source: `src/evaluation/src/artifact_lifecycle.cc`:298
- Brief: n/a
- Parameters:
  - `reason` (InvalidationReason): n/a

#### `std::string lifecycleStateToString(LifecycleState state) noexcept`
- Source: `src/evaluation/src/artifact_lifecycle.cc`:260
- Brief: n/a
- Parameters:
  - `state` (LifecycleState): n/a

#### `std::unique_ptr< ApproximationRuleEngine > makeDefaultApproximationRuleEngine()`
- Source: `src/evaluation/src/approximation_rules.cc`:521
- Brief: Create the default production approximation rule engine.
- Parameters: none
- Return: std::unique_ptr<ApproximationRuleEngine> owning the default engine.
- Details: std::unique_ptr<ApproximationRuleEngine> owning the default engine.

#### `std::unique_ptr< QueryPlanner > makeDefaultQueryPlanner()`
- Source: `src/evaluation/src/query_planner.cc`:434
- Brief: Create a DefaultQueryPlanner instance without an observer.
- Parameters: none
- Return: Owning pointer to the default planner implementation.
- Details: Returns a std::unique_ptr<QueryPlanner> owning a DefaultQueryPlanner. Use this factory in production code to avoid coupling to the concrete type. Owning pointer to the default planner implementation.

#### `std::unique_ptr< QueryPlanner > makeDefaultQueryPlanner(PlannerObserver *observer)`
- Source: `src/evaluation/src/query_planner.cc`:448
- Brief: Create a DefaultQueryPlanner instance with an optional observer.
- Parameters:
  - `observer` (PlannerObserver *): Non-owning observer pointer, or nullptr to disable hooks.
- Return: Owning pointer to the default planner implementation.
- Details: When observer is non-null, PlannerObserver::onDecision() is invoked after every selectPath() call with the decision and wall-clock latency. The caller owns observer; its lifetime must exceed the planner's. observer Non-owning observer pointer, or nullptr to disable hooks. Owning pointer to the default planner implementation.

#### `BenchmarkEdgeCase operator&(BenchmarkEdgeCase a, BenchmarkEdgeCase b) noexcept`
- Source: `src/evaluation/include/benchmark_matrix.h`:188
- Brief: n/a
- Parameters:
  - `a` (BenchmarkEdgeCase): n/a
  - `b` (BenchmarkEdgeCase): n/a

#### `BenchmarkEdgeCase operator\|(BenchmarkEdgeCase a, BenchmarkEdgeCase b) noexcept`
- Source: `src/evaluation/include/benchmark_matrix.h`:183
- Brief: n/a
- Parameters:
  - `a` (BenchmarkEdgeCase): n/a
  - `b` (BenchmarkEdgeCase): n/a

#### `std::optional< DeploymentProfileId > parseDeploymentProfileId(std::string_view value)`
- Source: `src/evaluation/src/hardware_profile.cc`:162
- Brief: Parse Deployment Profile Id.
- Parameters:
  - `value` (std::string_view): Input parameter.
- Return: Return value.
- Details: value Input parameter. Return value. Calls: normalizeToken().

#### `std::optional< StorageTier > parseStorageTier(std::string_view value)`
- Source: `src/evaluation/src/hardware_profile.cc`:183
- Brief: Parse Storage Tier.
- Parameters:
  - `value` (std::string_view): Input parameter.
- Return: Return value.
- Details: value Input parameter. Return value. Calls: normalizeToken().

#### `std::string_view scenarioName(BenchmarkScenario s) noexcept`
- Source: `src/evaluation/include/benchmark_matrix.h`:82
- Brief: n/a
- Parameters:
  - `s` (BenchmarkScenario): n/a

#### `std::optional< InvalidationReason > stringToInvalidationReason(const std::string &reason_str) noexcept`
- Source: `src/evaluation/src/artifact_lifecycle.cc`:323
- Brief: n/a
- Parameters:
  - `reason_str` (const std::string &): n/a

#### `std::optional< LifecycleState > stringToLifecycleState(const std::string &state_str) noexcept`
- Source: `src/evaluation/src/artifact_lifecycle.cc`:279
- Brief: n/a
- Parameters:
  - `state_str` (const std::string &): n/a

#### `std::string toString(AcceleratorClass accelerator_class)`
- Source: `src/evaluation/src/hardware_profile.cc`:124
- Brief: To String.
- Parameters:
  - `accelerator_class` (AcceleratorClass): Input parameter.
- Return: Return value.
- Details: accelerator_class Input parameter. Return value. Implements toString without additional internal calls.

#### `std::string toString(DeploymentProfileId profile_id)`
- Source: `src/evaluation/src/hardware_profile.cc`:70
- Brief: To String.
- Parameters:
  - `profile_id` (DeploymentProfileId): Identifier of the profile.
- Return: Return value.
- Details: profile_id Identifier of the profile. Return value. Implements toString without additional internal calls.

#### `std::string toString(LayerId layer)`
- Source: `src/evaluation/src/hardware_profile.cc`:142
- Brief: To String.
- Parameters:
  - `layer` (LayerId): Input parameter.
- Return: Return value.
- Details: layer Input parameter. Return value. Implements toString without additional internal calls.

#### `std::string toString(NetworkFabric fabric)`
- Source: `src/evaluation/src/hardware_profile.cc`:106
- Brief: To String.
- Parameters:
  - `fabric` (NetworkFabric): Input parameter.
- Return: Return value.
- Details: fabric Input parameter. Return value. Implements toString without additional internal calls.

#### `std::string toString(StorageTier tier)`
- Source: `src/evaluation/src/hardware_profile.cc`:88
- Brief: To String.
- Parameters:
  - `tier` (StorageTier): Input parameter.
- Return: Return value.
- Details: tier Input parameter. Return value. Implements toString without additional internal calls.

#### `HardwareProfileValidationResult validateHardwareProfile(const HardwareProfile &profile)`
- Source: `src/evaluation/src/hardware_profile.cc`:264
- Brief: Validate Hardware Profile.
- Parameters:
  - `profile` (const HardwareProfile &): Input parameter.
- Return: Return value.
- Details: profile Input parameter. Return value. Calls: push_back(), std::string(), empty(), validate_band(), unique_tiers(), begin(), end(), size().

#### `TierTransitionResult validateTierTransition(const HardwareProfile &current, const HardwareProfile &target, const TierTransitionRequest &request)`
- Source: `src/evaluation/src/hardware_profile.cc`:422
- Brief: Validate Tier Transition.
- Parameters:
  - `current` (const HardwareProfile &): Input parameter.
  - `target` (const HardwareProfile &): Input parameter.
  - `request` (const TierTransitionRequest &): Input parameter.
- Return: Return value.
- Details: current Input parameter. target Input parameter. request Input parameter. Return value. Calls: validateHardwareProfile(), insert(), end(), begin(), containsTier(), push_back(), toString(), findLayerSizingRule().

### themis::evaluation::AblationError

#### `AblationError(std::string_view what)`
- Source: `src/evaluation/include/ablation_framework.h`:180
- Brief: Ablation Error.
- Parameters:
  - `what` (std::string_view): Input parameter.
- Return: Return value.
- Details: what Input parameter. Return value.

### themis::evaluation::AblationReport

#### `std::string bestByFallbackEfficiency() const noexcept`
- Source: `src/evaluation/include/ablation_framework.h`:160
- Brief: n/a
- Parameters: none

#### `std::string bestByNdcg() const noexcept`
- Source: `src/evaluation/include/ablation_framework.h`:158
- Brief: n/a
- Parameters: none

#### `std::string bestByRecall() const noexcept`
- Source: `src/evaluation/include/ablation_framework.h`:156
- Brief: n/a
- Parameters: none

#### `std::optional< double > ndcgGain(std::string_view a, std::string_view b) const noexcept`
- Source: `src/evaluation/include/ablation_framework.h`:165
- Brief: n/a
- Parameters:
  - `a` (std::string_view): n/a
  - `b` (std::string_view): n/a

#### `std::optional< double > recallGain(std::string_view a, std::string_view b) const noexcept`
- Source: `src/evaluation/include/ablation_framework.h`:162
- Brief: n/a
- Parameters:
  - `a` (std::string_view): n/a
  - `b` (std::string_view): n/a

### themis::evaluation::AblationRunner

#### `AblationRunner()=default`
- Source: `src/evaluation/include/ablation_framework.h`:190
- Brief: n/a
- Parameters: none

#### `AblationRunner(AblationRunner &&)=default`
- Source: `src/evaluation/include/ablation_framework.h`:195
- Brief: n/a
- Parameters:
  - `<unnamed>` (AblationRunner &&): n/a

#### `AblationRunner(const AblationRunner &)=delete`
- Source: `src/evaluation/include/ablation_framework.h`:193
- Brief: n/a
- Parameters:
  - `<unnamed>` (const AblationRunner &): n/a

#### `void addExperiment(std::string name, AblationConfig config)`
- Source: `src/evaluation/include/ablation_framework.h`:203
- Brief: Add Experiment.
- Parameters:
  - `name` (std::string): Input parameter.
  - `config` (AblationConfig): Input parameter.
- Throws:
  - AblationError: if an error occurs.
- Details: name Input parameter. config Input parameter. name Input parameter. config Input parameter. AblationError if an error occurs. Calls: empty(), push_back(), std::move().

#### `std::size_t experimentCount() const noexcept`
- Source: `src/evaluation/include/ablation_framework.h`:205
- Brief: n/a
- Parameters: none

#### `AblationRunner & operator=(AblationRunner &&)=default`
- Source: `src/evaluation/include/ablation_framework.h`:196
- Brief: n/a
- Parameters:
  - `<unnamed>` (AblationRunner &&): n/a

#### `AblationRunner & operator=(const AblationRunner &)=delete`
- Source: `src/evaluation/include/ablation_framework.h`:194
- Brief: n/a
- Parameters:
  - `<unnamed>` (const AblationRunner &): n/a

#### `void reset() noexcept`
- Source: `src/evaluation/include/ablation_framework.h`:215
- Brief: Reset the modification detection flag.
- Parameters: none
- Details: Exception safety: noexcept.

#### `AblationReport run(const std::vector< AblationQuery > &queries, double max_residual=0.10) const`
- Source: `src/evaluation/include/ablation_framework.h`:207
- Brief: n/a
- Parameters:
  - `queries` (const std::vector< AblationQuery > &): n/a
  - `max_residual` (double): n/a

#### `AblationResult runExperiment(const Experiment &exp, const std::vector< AblationQuery > &queries, double max_residual) const`
- Source: `src/evaluation/include/ablation_framework.h`:225
- Brief: n/a
- Parameters:
  - `exp` (const Experiment &): n/a
  - `queries` (const std::vector< AblationQuery > &): n/a
  - `max_residual` (double): n/a

### themis::evaluation::ApproximationRuleEngine

#### `ApproximationRuleEngine()=default`
- Source: `src/evaluation/include/approximation_rules.h`:183
- Brief: n/a
- Parameters: none

#### `ApproximationRuleEngine(ApproximationRuleEngine &&)=delete`
- Source: `src/evaluation/include/approximation_rules.h`:192
- Brief: n/a
- Parameters:
  - `<unnamed>` (ApproximationRuleEngine &&): n/a

#### `ApproximationRuleEngine(const ApproximationRuleEngine &)=delete`
- Source: `src/evaluation/include/approximation_rules.h`:190
- Brief: n/a
- Parameters:
  - `<unnamed>` (const ApproximationRuleEngine &): n/a

#### `ApproximationBoundary canonicalBoundary(RetrievalLayer layer) const noexcept=0`
- Source: `src/evaluation/include/approximation_rules.h`:207
- Brief: n/a
- Parameters:
  - `layer` (RetrievalLayer): n/a

#### `BoundaryCheckResult checkBoundary(RetrievalLayer layer, ApproximationZone zone, KernelCategory category, const ApproximationPolicy &policy, double confidence, bool uses_gpu=false) const noexcept=0`
- Source: `src/evaluation/include/approximation_rules.h`:195
- Brief: n/a
- Parameters:
  - `layer` (RetrievalLayer): n/a
  - `zone` (ApproximationZone): n/a
  - `category` (KernelCategory): n/a
  - `policy` (const ApproximationPolicy &): n/a
  - `confidence` (double): n/a
  - `uses_gpu` (bool): n/a

#### `ApproximationRuleEngine & operator=(ApproximationRuleEngine &&)=delete`
- Source: `src/evaluation/include/approximation_rules.h`:193
- Brief: n/a
- Parameters:
  - `<unnamed>` (ApproximationRuleEngine &&): n/a

#### `ApproximationRuleEngine & operator=(const ApproximationRuleEngine &)=delete`
- Source: `src/evaluation/include/approximation_rules.h`:191
- Brief: n/a
- Parameters:
  - `<unnamed>` (const ApproximationRuleEngine &): n/a

#### `BoundaryCheckResult validatePlannedPath(const PlannerDecision &decision, const ApproximationPolicy &policy) const noexcept=0`
- Source: `src/evaluation/include/approximation_rules.h`:203
- Brief: n/a
- Parameters:
  - `decision` (const PlannerDecision &): n/a
  - `policy` (const ApproximationPolicy &): n/a

#### `~ApproximationRuleEngine()=default`
- Source: `src/evaluation/include/approximation_rules.h`:188
- Brief: Approximation Rule Engine.
- Parameters: none
- Return: Return value.
- Details: Return value.

### themis::evaluation::ArtifactLifecycleManager

#### `ArtifactLifecycleManager()=default`
- Source: `src/evaluation/include/artifact_lifecycle.h`:255
- Brief: n/a
- Parameters: none

#### `ArtifactLifecycleManager(ArtifactLifecycleManager &&)=default`
- Source: `src/evaluation/include/artifact_lifecycle.h`:260
- Brief: n/a
- Parameters:
  - `<unnamed>` (ArtifactLifecycleManager &&): n/a

#### `ArtifactLifecycleManager(const ArtifactLifecycleManager &)=delete`
- Source: `src/evaluation/include/artifact_lifecycle.h`:257
- Brief: n/a
- Parameters:
  - `<unnamed>` (const ArtifactLifecycleManager &): n/a

#### `LifecycleMetadata beginRebuild(LifecycleMetadata metadata) noexcept`
- Source: `src/evaluation/include/artifact_lifecycle.h`:285
- Brief: n/a
- Parameters:
  - `metadata` (LifecycleMetadata): n/a

#### `LifecycleMetadata completeRebuildFailure(LifecycleMetadata metadata) noexcept`
- Source: `src/evaluation/include/artifact_lifecycle.h`:296
- Brief: n/a
- Parameters:
  - `metadata` (LifecycleMetadata): n/a

#### `LifecycleMetadata completeRebuildSuccess(LifecycleMetadata metadata, std::uint32_t new_age_ms, std::uint64_t new_delta_lag, double new_residual) noexcept`
- Source: `src/evaluation/include/artifact_lifecycle.h`:289
- Brief: n/a
- Parameters:
  - `metadata` (LifecycleMetadata): n/a
  - `new_age_ms` (std::uint32_t): n/a
  - `new_delta_lag` (std::uint64_t): n/a
  - `new_residual` (double): n/a

#### `LifecycleState computeState(const LifecycleMetadata &metadata, const StalenessPolicy &policy) const noexcept`
- Source: `src/evaluation/include/artifact_lifecycle.h`:267
- Brief: n/a
- Parameters:
  - `metadata` (const LifecycleMetadata &): n/a
  - `policy` (const StalenessPolicy &): n/a

#### `std::vector< LifecycleState > computeStatesBatch(const std::vector< LifecycleMetadata > &metadata_batch, const StalenessPolicy &policy) const noexcept`
- Source: `src/evaluation/include/artifact_lifecycle.h`:320
- Brief: n/a
- Parameters:
  - `metadata_batch` (const std::vector< LifecycleMetadata > &): n/a
  - `policy` (const StalenessPolicy &): n/a

#### `std::optional< std::string > diagnoseStalenessCause(const LifecycleMetadata &metadata, const StalenessPolicy &policy) const noexcept`
- Source: `src/evaluation/include/artifact_lifecycle.h`:311
- Brief: n/a
- Parameters:
  - `metadata` (const LifecycleMetadata &): n/a
  - `policy` (const StalenessPolicy &): n/a

#### `std::vector< LifecycleMetadata > filterUsableArtifacts(const std::vector< LifecycleMetadata > &metadata_batch, const StalenessPolicy &policy) const noexcept`
- Source: `src/evaluation/include/artifact_lifecycle.h`:325
- Brief: n/a
- Parameters:
  - `metadata_batch` (const std::vector< LifecycleMetadata > &): n/a
  - `policy` (const StalenessPolicy &): n/a

#### `std::vector< LifecycleMetadata > identifyRebuildCandidates(const std::vector< LifecycleMetadata > &metadata_batch) const noexcept`
- Source: `src/evaluation/include/artifact_lifecycle.h`:330
- Brief: n/a
- Parameters:
  - `metadata_batch` (const std::vector< LifecycleMetadata > &): n/a

#### `LifecycleMetadata invalidate(LifecycleMetadata metadata, InvalidationReason reason) noexcept`
- Source: `src/evaluation/include/artifact_lifecycle.h`:280
- Brief: n/a
- Parameters:
  - `metadata` (LifecycleMetadata): n/a
  - `reason` (InvalidationReason): n/a

#### `bool isUsableForPlanning(LifecycleState state) noexcept`
- Source: `src/evaluation/include/artifact_lifecycle.h`:272
- Brief: n/a
- Parameters:
  - `state` (LifecycleState): n/a

#### `LifecycleMetadata markReady(LifecycleMetadata metadata, std::uint32_t age_ms, std::uint64_t delta_lag, double residual) noexcept`
- Source: `src/evaluation/include/artifact_lifecycle.h`:300
- Brief: n/a
- Parameters:
  - `metadata` (LifecycleMetadata): n/a
  - `age_ms` (std::uint32_t): n/a
  - `delta_lag` (std::uint64_t): n/a
  - `residual` (double): n/a

#### `ArtifactLifecycleManager & operator=(ArtifactLifecycleManager &&)=default`
- Source: `src/evaluation/include/artifact_lifecycle.h`:261
- Brief: n/a
- Parameters:
  - `<unnamed>` (ArtifactLifecycleManager &&): n/a

#### `ArtifactLifecycleManager & operator=(const ArtifactLifecycleManager &)=delete`
- Source: `src/evaluation/include/artifact_lifecycle.h`:258
- Brief: n/a
- Parameters:
  - `<unnamed>` (const ArtifactLifecycleManager &): n/a

#### `bool requiresImmediateRebuild(LifecycleState state) noexcept`
- Source: `src/evaluation/include/artifact_lifecycle.h`:274
- Brief: n/a
- Parameters:
  - `state` (LifecycleState): n/a

### themis::evaluation::BenchmarkMatrix

#### `BenchmarkMatrix()=default`
- Source: `src/evaluation/include/benchmark_matrix.h`:236
- Brief: n/a
- Parameters: none

#### `BenchmarkMatrix(BenchmarkMatrix &&) noexcept=default`
- Source: `src/evaluation/include/benchmark_matrix.h`:242
- Brief: n/a
- Parameters:
  - `<unnamed>` (BenchmarkMatrix &&): n/a

#### `BenchmarkMatrix(const BenchmarkMatrix &)=delete`
- Source: `src/evaluation/include/benchmark_matrix.h`:240
- Brief: n/a
- Parameters:
  - `<unnamed>` (const BenchmarkMatrix &): n/a

#### `std::optional< BenchmarkScenario > bestScenario(BenchmarkDimension dimension, bool higher_is_better=true) const noexcept`
- Source: `src/evaluation/include/benchmark_matrix.h`:314
- Brief: n/a
- Parameters:
  - `dimension` (BenchmarkDimension): n/a
  - `higher_is_better` (bool): n/a

#### `void clear() noexcept`
- Source: `src/evaluation/include/benchmark_matrix.h`:275
- Brief: Clear.
- Parameters: none
- Details: Exception safety: noexcept.

#### `std::optional< double > compareScenarios(BenchmarkScenario a, BenchmarkScenario b, BenchmarkDimension dimension) const noexcept`
- Source: `src/evaluation/include/benchmark_matrix.h`:309
- Brief: n/a
- Parameters:
  - `a` (BenchmarkScenario): n/a
  - `b` (BenchmarkScenario): n/a
  - `dimension` (BenchmarkDimension): n/a

#### `bool contains(BenchmarkScenario scenario, BenchmarkDimension dimension) const noexcept`
- Source: `src/evaluation/include/benchmark_matrix.h`:301
- Brief: n/a
- Parameters:
  - `scenario` (BenchmarkScenario): n/a
  - `dimension` (BenchmarkDimension): n/a

#### `std::vector< std::pair< BenchmarkScenario, BenchmarkResult > > dimensionSlice(BenchmarkDimension dimension) const`
- Source: `src/evaluation/include/benchmark_matrix.h`:293
- Brief: n/a
- Parameters:
  - `dimension` (BenchmarkDimension): n/a

#### `std::vector< BenchmarkEntry > entries(std::string_view dataset_tag="", std::string_view hardware_tag="", std::string_view runner_version="") const`
- Source: `src/evaluation/include/benchmark_matrix.h`:284
- Brief: n/a
- Parameters:
  - `dataset_tag` (std::string_view): n/a
  - `hardware_tag` (std::string_view): n/a
  - `runner_version` (std::string_view): n/a

#### `void invalidateDimension(BenchmarkDimension dimension)`
- Source: `src/evaluation/include/benchmark_matrix.h`:269
- Brief: Invalidate Dimension.
- Parameters:
  - `dimension` (BenchmarkDimension): Input parameter.
- Details: ============================================================================ BenchmarkMatrix::invalidateDimension ============================================================================ dimension Input parameter. dimension Input parameter. Calls: begin(), end(), erase().

#### `void invalidateScenario(BenchmarkScenario scenario)`
- Source: `src/evaluation/include/benchmark_matrix.h`:263
- Brief: Invalidate Scenario.
- Parameters:
  - `scenario` (BenchmarkScenario): Input parameter.
- Details: ============================================================================ BenchmarkMatrix::invalidateScenario ============================================================================ scenario Input parameter. scenario Input parameter. Calls: begin(), end(), erase().

#### `std::optional< BenchmarkResult > lookup(BenchmarkScenario scenario, BenchmarkDimension dimension) const noexcept`
- Source: `src/evaluation/include/benchmark_matrix.h`:282
- Brief: n/a
- Parameters:
  - `scenario` (BenchmarkScenario): n/a
  - `dimension` (BenchmarkDimension): n/a

#### `BenchmarkMatrix & operator=(BenchmarkMatrix &&) noexcept=default`
- Source: `src/evaluation/include/benchmark_matrix.h`:243
- Brief: n/a
- Parameters:
  - `<unnamed>` (BenchmarkMatrix &&): n/a

#### `BenchmarkMatrix & operator=(const BenchmarkMatrix &)=delete`
- Source: `src/evaluation/include/benchmark_matrix.h`:241
- Brief: n/a
- Parameters:
  - `<unnamed>` (const BenchmarkMatrix &): n/a

#### `void record(BenchmarkScenario scenario, BenchmarkDimension dimension, const BenchmarkResult &result)`
- Source: `src/evaluation/include/benchmark_matrix.h`:255
- Brief: Record.
- Parameters:
  - `scenario` (BenchmarkScenario): Input parameter.
  - `dimension` (BenchmarkDimension): Input parameter.
  - `result` (const BenchmarkResult &): Input parameter.
- Throws:
  - std::invalid_argument: if an error occurs.
- Details: ============================================================================ BenchmarkMatrix::record ============================================================================ scenario Input parameter. dimension Input parameter. result Input parameter. scenario Input parameter. dimension Input parameter. result Input parameter. std::invalid_argument if an error occurs. Calls: std::string(), scenarioName(), dimensionName().

#### `std::vector< std::pair< BenchmarkDimension, BenchmarkResult > > scenarioSlice(BenchmarkScenario scenario) const`
- Source: `src/evaluation/include/benchmark_matrix.h`:290
- Brief: n/a
- Parameters:
  - `scenario` (BenchmarkScenario): n/a

#### `std::vector< BenchmarkScenario > scenariosWithFullCoverage(const std::vector< BenchmarkDimension > &required_dimensions) const`
- Source: `src/evaluation/include/benchmark_matrix.h`:296
- Brief: n/a
- Parameters:
  - `required_dimensions` (const std::vector< BenchmarkDimension > &): n/a

#### `std::size_t size() const noexcept`
- Source: `src/evaluation/include/benchmark_matrix.h`:299
- Brief: n/a
- Parameters: none

#### `~BenchmarkMatrix()=default`
- Source: `src/evaluation/include/benchmark_matrix.h`:237
- Brief: n/a
- Parameters: none

### themis::evaluation::BenchmarkMatrix::Key

#### `bool operator==(const Key &o) const noexcept`
- Source: `src/evaluation/include/benchmark_matrix.h`:322
- Brief: n/a
- Parameters:
  - `o` (const Key &): n/a

### themis::evaluation::BenchmarkMatrix::KeyHash

#### `std::size_t operator()(const Key &k) const noexcept`
- Source: `src/evaluation/include/benchmark_matrix.h`:328
- Brief: n/a
- Parameters:
  - `k` (const Key &): n/a

### themis::evaluation::BenchmarkResult

#### `bool hasSufficientData(uint32_t min_samples=3) const noexcept`
- Source: `src/evaluation/include/benchmark_matrix.h`:211
- Brief: n/a
- Parameters:
  - `min_samples` (uint32_t): n/a

#### `bool isClean() const noexcept`
- Source: `src/evaluation/include/benchmark_matrix.h`:207
- Brief: n/a
- Parameters: none

### themis::evaluation::BoundaryCheckResult

#### `bool isAllowed() const noexcept`
- Source: `src/evaluation/include/approximation_rules.h`:171
- Brief: n/a
- Parameters: none

### themis::evaluation::DefaultApproximationRuleEngine

#### `DefaultApproximationRuleEngine()=default`
- Source: `src/evaluation/src/approximation_rules.cc`:177
- Brief: n/a
- Parameters: none

#### `ApproximationBoundary canonicalBoundary(RetrievalLayer layer) const noexcept override`
- Source: `src/evaluation/src/approximation_rules.cc`:492
- Brief: Return the canonical boundary descriptor for a retrieval layer.
- Parameters:
  - `layer` (RetrievalLayer): The retrieval layer.
- Return: Canonical ApproximationBoundary. Returns a safe default (ExactGraph / Exact / fail-closed) for unrecognized layer values.
- Details: layer The retrieval layer. Canonical ApproximationBoundary. Returns a safe default (ExactGraph / Exact / fail-closed) for unrecognized layer values.

#### `BoundaryCheckResult checkBoundary(RetrievalLayer layer, ApproximationZone zone, KernelCategory category, const ApproximationPolicy &policy, double confidence, bool uses_gpu) const noexcept override`
- Source: `src/evaluation/src/approximation_rules.cc`:198
- Brief: Evaluate approximation boundary and policy rules for one request.
- Parameters:
  - `layer` (RetrievalLayer): Retrieval layer being evaluated.
  - `zone` (ApproximationZone): Zone being requested by the caller.
  - `category` (KernelCategory): Kernel category active at this layer.
  - `policy` (const ApproximationPolicy &): Active governance policy.
  - `confidence` (double): Query-time confidence in [0.0, 1.0].
  - `uses_gpu` (bool): True when this layer is being asked to dispatch on GPU.
- Return: BoundaryCheckResult with decision and optional violation.
- Details: Applies rules in priority order (see file-level overview). Returns the first violation found, or Allow when all rules pass. layer Retrieval layer being evaluated. zone Zone being requested by the caller. category Kernel category active at this layer. policy Active governance policy. confidence Query-time confidence in [0.0, 1.0]. uses_gpu True when this layer is being asked to dispatch on GPU. BoundaryCheckResult with decision and optional violation.

#### `BoundaryCheckResult validatePlannedPath(const PlannerDecision &decision, const ApproximationPolicy &policy) const noexcept override`
- Source: `src/evaluation/src/approximation_rules.cc`:407
- Brief: Validate a planner decision against approximation boundaries.
- Parameters:
  - `decision` (const PlannerDecision &): The planner decision to validate.
  - `policy` (const ApproximationPolicy &): Active governance policy.
- Return: BoundaryCheckResult representing the overall compliance.
- Details: Maps each ExecutionPath to its implied layer(s) and applies checkBoundary() for each. Returns the most restrictive result. Path → layer(s) mapping: AnnOnly → {Ann} AnnTensorSummary → {Ann, TensorSummary} AnnTensorExactGraph → {Ann, TensorSummary, ExactGraph} DirectExactGraph → {ExactGraph} DistributedSummaryFirstExactOnDemand → {DistributedShard, ExactGraph} decision The planner decision to validate. policy Active governance policy. BoundaryCheckResult representing the overall compliance.

#### `~DefaultApproximationRuleEngine() override=default`
- Source: `src/evaluation/src/approximation_rules.cc`:178
- Brief: n/a
- Parameters: none

### themis::evaluation::DefaultQueryPlanner

#### `DefaultQueryPlanner() noexcept`
- Source: `src/evaluation/src/query_planner.cc`:255
- Brief: Construct without an observer (no observability callbacks).
- Parameters: none

#### `DefaultQueryPlanner(PlannerObserver *observer) noexcept`
- Source: `src/evaluation/src/query_planner.cc`:264
- Brief: Construct with an optional observer for observability hooks.
- Parameters:
  - `observer` (PlannerObserver *): Non-owning pointer to a PlannerObserver, or nullptr to disable callbacks. Lifetime of observer must exceed the lifetime of this planner instance.
- Details: observer Non-owning pointer to a PlannerObserver, or nullptr to disable callbacks. Lifetime of observer must exceed the lifetime of this planner instance.

#### `PlannerDecision selectPath(const ExecutionEligibility &eligibility, const TensorArtifactFreshness &freshness, const PlannerConfig &config) const noexcept override`
- Source: `src/evaluation/src/query_planner.cc`:291
- Brief: Select the optimal execution path for a query.
- Parameters:
  - `eligibility` (const ExecutionEligibility &): Runtime hardware and module-readiness signals.
  - `freshness` (const TensorArtifactFreshness &): Tensor artifact freshness state.
  - `config` (const PlannerConfig &): Versioned policy thresholds.
- Return: A PlannerDecision with the selected path and any fallback reason.
- Details: The planner evaluates paths 1–5 in order from cheapest to most expensive. Any failed gate causes a fall-through to the next path with the appropriate FallbackReason. The safe default is Path 4 (DirectExactGraph). After the decision is made, PlannerObserver::onDecision() is called (if an observer was registered) with the decision and the wall-clock latency of the planner logic in microseconds. Distributed Path 5 is only eligible when distributed mode signals are set; it is not evaluated in the normal 1–4 fallback chain. Callers may request Path 5 explicitly by setting eligibility signals that disable paths 1–3 while marking distributed_multi_shard. eligibility Runtime hardware and module-readiness signals. freshness Tensor artifact freshness state. config Versioned policy thresholds. A PlannerDecision with the selected path and any fallback reason.

#### `~DefaultQueryPlanner() override=default`
- Source: `src/evaluation/src/query_planner.cc`:267
- Brief: n/a
- Parameters: none

### themis::evaluation::ExecutionEligibility

#### `bool isGpuEligible(KernelCategory category) const noexcept`
- Source: `src/evaluation/include/query_planner.h`:193
- Brief: n/a
- Parameters:
  - `category` (KernelCategory): n/a

### themis::evaluation::HardwareProfileRegistry

#### `HardwareProfileRegistry()`
- Source: `src/evaluation/include/hardware_profile.h`:145
- Brief: n/a
- Parameters: none

#### `HardwareProfileRegistry(std::vector< HardwareProfile > profiles)`
- Source: `src/evaluation/include/hardware_profile.h`:151
- Brief: Hardware Profile Registry.
- Parameters:
  - `profiles` (std::vector< HardwareProfile >): Input parameter.
- Return: Return value.
- Details: profiles Input parameter. Return value.

#### `bool activate(DeploymentProfileId profile_id, std::string *error=nullptr)`
- Source: `src/evaluation/include/hardware_profile.h`:163
- Brief: Activate.
- Parameters:
  - `profile_id` (DeploymentProfileId): Identifier of the profile.
  - `error` (std::string *): Input/output parameter.
- Return: True when the operation succeeds.
- Details: profile_id Identifier of the profile. error Input/output parameter. True when the operation succeeds. Calls: find(), toString(), validateHardwareProfile(), ok(), front().

#### `bool activate(std::string_view profile_name, std::string *error=nullptr)`
- Source: `src/evaluation/include/hardware_profile.h`:165
- Brief: Activate.
- Parameters:
  - `profile_name` (std::string_view): Name of the profile.
  - `error` (std::string *): Input/output parameter.
- Return: True when the operation succeeds.
- Details: profile_name Name of the profile. error Input/output parameter. True when the operation succeeds. Calls: parseDeploymentProfileId(), std::string().

#### `const HardwareProfile * activeProfile() const`
- Source: `src/evaluation/include/hardware_profile.h`:157
- Brief: n/a
- Parameters: none

#### `const HardwareProfile * find(DeploymentProfileId profile_id) const`
- Source: `src/evaluation/include/hardware_profile.h`:159
- Brief: n/a
- Parameters:
  - `profile_id` (DeploymentProfileId): n/a

#### `const HardwareProfile * find(std::string_view profile_name) const`
- Source: `src/evaluation/include/hardware_profile.h`:161
- Brief: n/a
- Parameters:
  - `profile_name` (std::string_view): n/a

#### `std::span< const HardwareProfile > profiles() const`
- Source: `src/evaluation/include/hardware_profile.h`:155
- Brief: n/a
- Parameters: none

#### `TierTransitionResult transitionTo(DeploymentProfileId target_profile, const TierTransitionRequest &request) const`
- Source: `src/evaluation/include/hardware_profile.h`:169
- Brief: n/a
- Parameters:
  - `target_profile` (DeploymentProfileId): n/a
  - `request` (const TierTransitionRequest &): n/a

#### `HardwareProfileValidationResult validate() const`
- Source: `src/evaluation/include/hardware_profile.h`:167
- Brief: n/a
- Parameters: none

#### `HardwareProfileRegistry withBuiltIns()`
- Source: `src/evaluation/include/hardware_profile.h`:153
- Brief: With Built Ins.
- Parameters: none
- Return: Return value.
- Details: Return value. Calls: HardwareProfileRegistry(), defaultHardwareProfiles().

### themis::evaluation::HardwareProfileValidationResult

#### `bool ok() const`
- Source: `src/evaluation/include/hardware_profile.h`:92
- Brief: n/a
- Parameters: none

### themis::evaluation::MetricCollector

#### `MetricCollector()=default`
- Source: `src/evaluation/include/retrieval_metrics.h`:254
- Brief: n/a
- Parameters: none

#### `MetricCollector(MetricCollector &&)=default`
- Source: `src/evaluation/include/retrieval_metrics.h`:259
- Brief: n/a
- Parameters:
  - `<unnamed>` (MetricCollector &&): n/a

#### `MetricCollector(const MetricCollector &)=delete`
- Source: `src/evaluation/include/retrieval_metrics.h`:257
- Brief: n/a
- Parameters:
  - `<unnamed>` (const MetricCollector &): n/a

#### `MetricCollector & operator=(MetricCollector &&)=default`
- Source: `src/evaluation/include/retrieval_metrics.h`:260
- Brief: n/a
- Parameters:
  - `<unnamed>` (MetricCollector &&): n/a

#### `MetricCollector & operator=(const MetricCollector &)=delete`
- Source: `src/evaluation/include/retrieval_metrics.h`:258
- Brief: n/a
- Parameters:
  - `<unnamed>` (const MetricCollector &): n/a

#### `void recordShardQuery(uint32_t shard_count, double bytes, uint32_t skipped)`
- Source: `src/evaluation/include/retrieval_metrics.h`:274
- Brief: Record Shard Query.
- Parameters:
  - `shard_count` (uint32_t): Input parameter.
  - `bytes` (double): Input parameter.
  - `skipped` (uint32_t): Input parameter.
- Details: shard_count Input parameter. bytes Input parameter. skipped Input parameter.

#### `void recordSnapshot(TensorGraphSnapshot snapshot)`
- Source: `src/evaluation/include/retrieval_metrics.h`:266
- Brief: Record Snapshot.
- Parameters:
  - `snapshot` (TensorGraphSnapshot): Input parameter.
- Details: snapshot Input parameter. snapshot Input parameter. Calls: push_back(), std::move().

#### `void reset() noexcept`
- Source: `src/evaluation/include/retrieval_metrics.h`:288
- Brief: Reset the modification detection flag.
- Parameters: none
- Details: Exception safety: noexcept.

#### `std::size_t snapshotCount() const noexcept`
- Source: `src/evaluation/include/retrieval_metrics.h`:276
- Brief: n/a
- Parameters: none

#### `DistributedEfficiencyMetrics summarizeDistributed(uint32_t total_shards) const`
- Source: `src/evaluation/include/retrieval_metrics.h`:281
- Brief: n/a
- Parameters:
  - `total_shards` (uint32_t): n/a

#### `TensorGraphRuntimeMetrics summarizeTensorGraph(double max_residual_error=0.10) const`
- Source: `src/evaluation/include/retrieval_metrics.h`:278
- Brief: n/a
- Parameters:
  - `max_residual_error` (double): n/a

### themis::evaluation::MetricError

#### `MetricError(MetricErrorKind kind, std::string_view what)`
- Source: `src/evaluation/include/retrieval_metrics.h`:80
- Brief: Metric Error.
- Parameters:
  - `kind` (MetricErrorKind): Input parameter.
  - `what` (std::string_view): Input parameter.
- Return: Return value.
- Details: kind Input parameter. what Input parameter. Return value.

#### `MetricErrorKind kind() const noexcept`
- Source: `src/evaluation/include/retrieval_metrics.h`:83
- Brief: n/a
- Parameters: none

### themis::evaluation::PlannerObserver

#### `PlannerObserver()=default`
- Source: `src/evaluation/include/query_planner.h`:288
- Brief: n/a
- Parameters: none

#### `PlannerObserver(PlannerObserver &&)=delete`
- Source: `src/evaluation/include/query_planner.h`:297
- Brief: n/a
- Parameters:
  - `<unnamed>` (PlannerObserver &&): n/a

#### `PlannerObserver(const PlannerObserver &)=delete`
- Source: `src/evaluation/include/query_planner.h`:295
- Brief: n/a
- Parameters:
  - `<unnamed>` (const PlannerObserver &): n/a

#### `void onDecision(const PlannerDecision &decision, uint64_t latency_us) noexcept=0`
- Source: `src/evaluation/include/query_planner.h`:306
- Brief: On Decision.
- Parameters:
  - `decision` (const PlannerDecision &): Input parameter.
  - `latency_us` (uint64_t): Input parameter.
- Details: decision Input parameter. latency_us Input parameter. Exception safety: noexcept.

#### `PlannerObserver & operator=(PlannerObserver &&)=delete`
- Source: `src/evaluation/include/query_planner.h`:298
- Brief: n/a
- Parameters:
  - `<unnamed>` (PlannerObserver &&): n/a

#### `PlannerObserver & operator=(const PlannerObserver &)=delete`
- Source: `src/evaluation/include/query_planner.h`:296
- Brief: n/a
- Parameters:
  - `<unnamed>` (const PlannerObserver &): n/a

#### `~PlannerObserver()=default`
- Source: `src/evaluation/include/query_planner.h`:293
- Brief: Planner Observer.
- Parameters: none
- Return: Return value.
- Details: Return value.

### themis::evaluation::QueryPlanner

#### `QueryPlanner()=default`
- Source: `src/evaluation/include/query_planner.h`:257
- Brief: n/a
- Parameters: none

#### `QueryPlanner(QueryPlanner &&)=delete`
- Source: `src/evaluation/include/query_planner.h`:266
- Brief: n/a
- Parameters:
  - `<unnamed>` (QueryPlanner &&): n/a

#### `QueryPlanner(const QueryPlanner &)=delete`
- Source: `src/evaluation/include/query_planner.h`:264
- Brief: n/a
- Parameters:
  - `<unnamed>` (const QueryPlanner &): n/a

#### `bool isKernelEligibleForGpu(KernelCategory category, const ExecutionEligibility &eligibility) noexcept`
- Source: `src/evaluation/include/query_planner.h`:274
- Brief: n/a
- Parameters:
  - `category` (KernelCategory): n/a
  - `eligibility` (const ExecutionEligibility &): n/a

#### `QueryPlanner & operator=(QueryPlanner &&)=delete`
- Source: `src/evaluation/include/query_planner.h`:267
- Brief: n/a
- Parameters:
  - `<unnamed>` (QueryPlanner &&): n/a

#### `QueryPlanner & operator=(const QueryPlanner &)=delete`
- Source: `src/evaluation/include/query_planner.h`:265
- Brief: n/a
- Parameters:
  - `<unnamed>` (const QueryPlanner &): n/a

#### `PlannerDecision selectPath(const ExecutionEligibility &eligibility, const TensorArtifactFreshness &freshness, const PlannerConfig &config) const noexcept=0`
- Source: `src/evaluation/include/query_planner.h`:269
- Brief: n/a
- Parameters:
  - `eligibility` (const ExecutionEligibility &): n/a
  - `freshness` (const TensorArtifactFreshness &): n/a
  - `config` (const PlannerConfig &): n/a

#### `~QueryPlanner()=default`
- Source: `src/evaluation/include/query_planner.h`:262
- Brief: Query Planner.
- Parameters: none
- Return: Return value.
- Details: Return value.

### themis::evaluation::StalenessPolicy

#### `StalenessPolicy()=default`
- Source: `src/evaluation/include/artifact_lifecycle.h`:130
- Brief: n/a
- Parameters: none

#### `std::optional< std::uint32_t > ageThresholdMs() const`
- Source: `src/evaluation/include/artifact_lifecycle.h`:187
- Brief: n/a
- Parameters: none

#### `std::optional< std::uint64_t > deltaLagThreshold() const`
- Source: `src/evaluation/include/artifact_lifecycle.h`:191
- Brief: n/a
- Parameters: none

#### `std::optional< std::uint32_t > rankCapThreshold() const`
- Source: `src/evaluation/include/artifact_lifecycle.h`:199
- Brief: n/a
- Parameters: none

#### `std::optional< double > residualThreshold() const`
- Source: `src/evaluation/include/artifact_lifecycle.h`:195
- Brief: n/a
- Parameters: none

#### `std::optional< double > residualVarianceThreshold() const`
- Source: `src/evaluation/include/artifact_lifecycle.h`:203
- Brief: n/a
- Parameters: none

#### `StalenessPolicy & withAgeThresholdMs(std::uint32_t threshold_ms)`
- Source: `src/evaluation/include/artifact_lifecycle.h`:138
- Brief: With Age Threshold Ms.
- Parameters:
  - `threshold_ms` (std::uint32_t): Input parameter.
- Return: Return value.
- Details: threshold_ms Input parameter. Return value. Implements withAgeThresholdMs without additional internal calls.

#### `StalenessPolicy & withDeltaLagThreshold(std::uint64_t threshold)`
- Source: `src/evaluation/include/artifact_lifecycle.h`:149
- Brief: With Delta Lag Threshold.
- Parameters:
  - `threshold` (std::uint64_t): Input parameter.
- Return: Return value.
- Details: threshold Input parameter. Return value. Implements withDeltaLagThreshold without additional internal calls.

#### `StalenessPolicy & withRankCapThreshold(std::uint32_t threshold)`
- Source: `src/evaluation/include/artifact_lifecycle.h`:171
- Brief: With Rank Cap Threshold.
- Parameters:
  - `threshold` (std::uint32_t): Input parameter.
- Return: Return value.
- Details: threshold Input parameter. Return value. Implements withRankCapThreshold without additional internal calls.

#### `StalenessPolicy & withResidualThreshold(double threshold)`
- Source: `src/evaluation/include/artifact_lifecycle.h`:160
- Brief: With Residual Threshold.
- Parameters:
  - `threshold` (double): Input parameter.
- Return: Return value.
- Details: threshold Input parameter. Return value. Implements withResidualThreshold without additional internal calls.

#### `StalenessPolicy & withResidualVarianceThreshold(double threshold)`
- Source: `src/evaluation/include/artifact_lifecycle.h`:182
- Brief: With Residual Variance Threshold.
- Parameters:
  - `threshold` (double): Input parameter.
- Return: Return value.
- Details: threshold Input parameter. Return value. Implements withResidualVarianceThreshold without additional internal calls.

### themis::evaluation::TensorArtifactFreshness

#### `bool isFresh(uint64_t max_age_ms, double min_residual=0.95) const noexcept`
- Source: `src/evaluation/include/query_planner.h`:130
- Brief: n/a
- Parameters:
  - `max_age_ms` (uint64_t): n/a
  - `min_residual` (double): n/a

#### `FallbackReason staleness_reason(uint64_t max_age_ms, double min_residual=0.95) const noexcept`
- Source: `src/evaluation/include/query_planner.h`:146
- Brief: n/a
- Parameters:
  - `max_age_ms` (uint64_t): n/a
  - `min_residual` (double): n/a

### themis::evaluation::TensorGraphSnapshot

#### `bool isResidualUnsafe(double max_residual_error) const noexcept`
- Source: `src/evaluation/include/retrieval_metrics.h`:239
- Brief: n/a
- Parameters:
  - `max_residual_error` (double): n/a

#### `bool isUnrecoveredFalseNegative() const noexcept`
- Source: `src/evaluation/include/retrieval_metrics.h`:235
- Brief: n/a
- Parameters: none

### themis::evaluation::TierTransitionResult

#### `bool ok() const`
- Source: `src/evaluation/include/hardware_profile.h`:108
- Brief: n/a
- Parameters: none

