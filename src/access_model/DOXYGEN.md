# ACCESS_MODEL DOXYGEN

Author: ThemisDB Contributors
Created: 2026-09-20
Last Updated: 2026-09-20
Status: active

## Source
- This file is generated from module-scoped Doxygen XML.
- XML index: `C:\Projects\ThemisDB\.tmp\module_doxygen_global_e2e\access_model\xml\index.xml`
- Warnings log: `C:\Projects\ThemisDB\.tmp\module_doxygen_global_e2e\access_model\doxygen-warnings.log`

## Structure Metrics
- C/C++ Files (scanned): 21
- Compounds: 83
- Classes/Structs: 48
- Namespaces: 6
- File Compounds: 21

## Namespaces
- benchmark
- testing
- themis
- themis::access_model
- themis::access_model::gates
- themis::access_model::test

## Types
### Classes
- themis::access_model::AccessCoordinator
- themis::access_model::AccessCoordinatorConcurrencyTest
- themis::access_model::AccessCoordinatorFocusedTest
- themis::access_model::AccessCoordinatorImpl
- themis::access_model::AccessCoordinatorTest
- themis::access_model::AccessModelE2ETest
- themis::access_model::AccessModelLogger
- themis::access_model::AccessModelMetrics
- themis::access_model::AccessTier
- themis::access_model::BenchAccessCoordinator
- themis::access_model::BenchmarkAccessTier
- themis::access_model::CacheTier
- themis::access_model::DefaultAccessModelLogger
- themis::access_model::LatencyHistogram
- themis::access_model::LoggingTest
- themis::access_model::MockAccessTier
- themis::access_model::ObservabilityIntegrationTest
- themis::access_model::PromotionDemotionTest
- themis::access_model::StorageTier
- themis::access_model::TestAccessTier
- themis::access_model::TraceContextManager
- themis::access_model::TraceContextManager::ScopedContext
- themis::access_model::TraceContextTest
- themis::access_model::test::CacheStorageIntegrationTest
- themis::access_model::test::MockAccessTier
- themis::access_model::test::MockEvictionListener
- themis::access_model::test::MockPromotionListener

### Structs
- themis::access_model::AccessCoordinatorImpl::DemotionEvent
- themis::access_model::AccessEvent
- themis::access_model::AccessMetrics
- themis::access_model::AccessOperationCounters
- themis::access_model::AccessTransitionEvent
- themis::access_model::AgeBasedPolicy
- themis::access_model::CoordinatorLifecycleLog
- themis::access_model::DemotionPlan
- themis::access_model::DemotionResult
- themis::access_model::EvictionEvent
- themis::access_model::EvictionEventLog
- themis::access_model::EvictionListener
- themis::access_model::PromotionDecisionLog
- themis::access_model::PromotionListener
- themis::access_model::PromotionResult
- themis::access_model::TierAccessOptions
- themis::access_model::TierGetResult
- themis::access_model::TierPromotionResult
- themis::access_model::TierPutResult
- themis::access_model::TierTransitionLog
- themis::access_model::TraceContext

## Notes
- This is a derived artifact. Do not manually edit semantic content.
- Regenerate via scripts/generate_module_doxygen_xml.py --write-module-markdown.

## Detailed Function Documentation (Soll-Ist)
- Functions extracted: 348

### bench_access_coordinator_gates.cpp

#### `BENCHMARK_MAIN()`
- Source: `benchmarks/access_model/bench_access_coordinator_gates.cpp`:416
- Brief: n/a
- Parameters: none

### themis::access_model

#### `state SetItemsProcessed(state.iterations())`
- Source: `benchmarks/access_model/bench_access_coordinator_gates.cpp`:244
- Brief: n/a
- Parameters:
  - `iterations` (state.): n/a

#### `TEST_F(AccessCoordinatorConcurrencyTest, C10_MetricsAtomicity_100ThreadsDecrementCounter)`
- Source: `tests/access_model/test_coordination_concurrency.cpp`:455
- Brief: n/a
- Parameters:
  - `<unnamed>` (AccessCoordinatorConcurrencyTest): n/a
  - `<unnamed>` (C10_MetricsAtomicity_100ThreadsDecrementCounter): n/a

#### `TEST_F(AccessCoordinatorConcurrencyTest, C1_ConcurrentEvictionEvents_10Threads_100Each)`
- Source: `tests/access_model/test_coordination_concurrency.cpp`:140
- Brief: n/a
- Parameters:
  - `<unnamed>` (AccessCoordinatorConcurrencyTest): n/a
  - `<unnamed>` (C1_ConcurrentEvictionEvents_10Threads_100Each): n/a

#### `TEST_F(AccessCoordinatorConcurrencyTest, C2_ConcurrentPromotionEvents_5Threads_200Each)`
- Source: `tests/access_model/test_coordination_concurrency.cpp`:174
- Brief: n/a
- Parameters:
  - `<unnamed>` (AccessCoordinatorConcurrencyTest): n/a
  - `<unnamed>` (C2_ConcurrentPromotionEvents_5Threads_200Each): n/a

#### `TEST_F(AccessCoordinatorConcurrencyTest, C3_MixedConcurrentEvents_5Threads_AlternatingEvictionPromotion)`
- Source: `tests/access_model/test_coordination_concurrency.cpp`:209
- Brief: n/a
- Parameters:
  - `<unnamed>` (AccessCoordinatorConcurrencyTest): n/a
  - `<unnamed>` (C3_MixedConcurrentEvents_5Threads_AlternatingEvictionPromotion): n/a

#### `TEST_F(AccessCoordinatorConcurrencyTest, C4_ConcurrentPromoteCallsOnSameKey_Idempotent)`
- Source: `tests/access_model/test_coordination_concurrency.cpp`:254
- Brief: n/a
- Parameters:
  - `<unnamed>` (AccessCoordinatorConcurrencyTest): n/a
  - `<unnamed>` (C4_ConcurrentPromoteCallsOnSameKey_Idempotent): n/a

#### `TEST_F(AccessCoordinatorConcurrencyTest, C5_ConcurrentDemotePromoteOnSameKey_NoDataLoss)`
- Source: `tests/access_model/test_coordination_concurrency.cpp`:283
- Brief: n/a
- Parameters:
  - `<unnamed>` (AccessCoordinatorConcurrencyTest): n/a
  - `<unnamed>` (C5_ConcurrentDemotePromoteOnSameKey_NoDataLoss): n/a

#### `TEST_F(AccessCoordinatorConcurrencyTest, C6_ConcurrentTierRegistration_AddRemoveDynamic)`
- Source: `tests/access_model/test_coordination_concurrency.cpp`:323
- Brief: n/a
- Parameters:
  - `<unnamed>` (AccessCoordinatorConcurrencyTest): n/a
  - `<unnamed>` (C6_ConcurrentTierRegistration_AddRemoveDynamic): n/a

#### `TEST_F(AccessCoordinatorConcurrencyTest, C7_WorkerThreadScaling_1To8ThreadsThroughputCheck)`
- Source: `tests/access_model/test_coordination_concurrency.cpp`:358
- Brief: n/a
- Parameters:
  - `<unnamed>` (AccessCoordinatorConcurrencyTest): n/a
  - `<unnamed>` (C7_WorkerThreadScaling_1To8ThreadsThroughputCheck): n/a

#### `TEST_F(AccessCoordinatorConcurrencyTest, C8_WorkerUnderprovisioningWith1000Events_QueueStability)`
- Source: `tests/access_model/test_coordination_concurrency.cpp`:398
- Brief: n/a
- Parameters:
  - `<unnamed>` (AccessCoordinatorConcurrencyTest): n/a
  - `<unnamed>` (C8_WorkerUnderprovisioningWith1000Events_QueueStability): n/a

#### `TEST_F(AccessCoordinatorConcurrencyTest, C9_WorkerShutdownDuringInflightEvents_GracefulDrain)`
- Source: `tests/access_model/test_coordination_concurrency.cpp`:425
- Brief: n/a
- Parameters:
  - `<unnamed>` (AccessCoordinatorConcurrencyTest): n/a
  - `<unnamed>` (C9_WorkerShutdownDuringInflightEvents_GracefulDrain): n/a

#### `TEST_F(AccessCoordinatorFocusedTest, ACM01_MultipleTiersRegistration)`
- Source: `tests/access_model/test_access_coordinator_focused.cpp`:138
- Brief: n/a
- Parameters:
  - `<unnamed>` (AccessCoordinatorFocusedTest): n/a
  - `<unnamed>` (ACM01_MultipleTiersRegistration): n/a

#### `TEST_F(AccessCoordinatorFocusedTest, ACM01_TierRegistryInitialization)`
- Source: `tests/access_model/test_access_coordinator_focused.cpp`:127
- Brief: n/a
- Parameters:
  - `<unnamed>` (AccessCoordinatorFocusedTest): n/a
  - `<unnamed>` (ACM01_TierRegistryInitialization): n/a

#### `TEST_F(AccessCoordinatorFocusedTest, ACM02_CacheEvictionEventProcessing)`
- Source: `tests/access_model/test_access_coordinator_focused.cpp`:153
- Brief: n/a
- Parameters:
  - `<unnamed>` (AccessCoordinatorFocusedTest): n/a
  - `<unnamed>` (ACM02_CacheEvictionEventProcessing): n/a

#### `TEST_F(AccessCoordinatorFocusedTest, ACM02_HotAccessEventProcessing)`
- Source: `tests/access_model/test_access_coordinator_focused.cpp`:172
- Brief: n/a
- Parameters:
  - `<unnamed>` (AccessCoordinatorFocusedTest): n/a
  - `<unnamed>` (ACM02_HotAccessEventProcessing): n/a

#### `TEST_F(AccessCoordinatorFocusedTest, ACM02_MultipleEventsProcessing)`
- Source: `tests/access_model/test_access_coordinator_focused.cpp`:197
- Brief: n/a
- Parameters:
  - `<unnamed>` (AccessCoordinatorFocusedTest): n/a
  - `<unnamed>` (ACM02_MultipleEventsProcessing): n/a

#### `TEST_F(AccessCoordinatorFocusedTest, ACM03_HotnessClassification)`
- Source: `tests/access_model/test_access_coordinator_focused.cpp`:238
- Brief: n/a
- Parameters:
  - `<unnamed>` (AccessCoordinatorFocusedTest): n/a
  - `<unnamed>` (ACM03_HotnessClassification): n/a

#### `TEST_F(AccessCoordinatorFocusedTest, ACM03_L1ToL2Promotion)`
- Source: `tests/access_model/test_access_coordinator_focused.cpp`:221
- Brief: n/a
- Parameters:
  - `<unnamed>` (AccessCoordinatorFocusedTest): n/a
  - `<unnamed>` (ACM03_L1ToL2Promotion): n/a

#### `TEST_F(AccessCoordinatorFocusedTest, ACM03_TierRecommendation)`
- Source: `tests/access_model/test_access_coordinator_focused.cpp`:258
- Brief: n/a
- Parameters:
  - `<unnamed>` (AccessCoordinatorFocusedTest): n/a
  - `<unnamed>` (ACM03_TierRecommendation): n/a

#### `TEST_F(AccessCoordinatorFocusedTest, ACM04_AsyncPromotionExecution)`
- Source: `tests/access_model/test_access_coordinator_focused.cpp`:276
- Brief: n/a
- Parameters:
  - `<unnamed>` (AccessCoordinatorFocusedTest): n/a
  - `<unnamed>` (ACM04_AsyncPromotionExecution): n/a

#### `TEST_F(AccessCoordinatorFocusedTest, ACM04_MultipleAsyncPromotions)`
- Source: `tests/access_model/test_access_coordinator_focused.cpp`:300
- Brief: n/a
- Parameters:
  - `<unnamed>` (AccessCoordinatorFocusedTest): n/a
  - `<unnamed>` (ACM04_MultipleAsyncPromotions): n/a

#### `TEST_F(AccessCoordinatorFocusedTest, ACM05_DemotionPlanCreation)`
- Source: `tests/access_model/test_access_coordinator_focused.cpp`:330
- Brief: n/a
- Parameters:
  - `<unnamed>` (AccessCoordinatorFocusedTest): n/a
  - `<unnamed>` (ACM05_DemotionPlanCreation): n/a

#### `TEST_F(AccessCoordinatorFocusedTest, ACM05_DemotionPlanExecution)`
- Source: `tests/access_model/test_access_coordinator_focused.cpp`:348
- Brief: n/a
- Parameters:
  - `<unnamed>` (AccessCoordinatorFocusedTest): n/a
  - `<unnamed>` (ACM05_DemotionPlanExecution): n/a

#### `TEST_F(AccessCoordinatorFocusedTest, ACM06_CorrelationIDGeneration)`
- Source: `tests/access_model/test_access_coordinator_focused.cpp`:377
- Brief: n/a
- Parameters:
  - `<unnamed>` (AccessCoordinatorFocusedTest): n/a
  - `<unnamed>` (ACM06_CorrelationIDGeneration): n/a

#### `TEST_F(AccessCoordinatorFocusedTest, ACM07_LatencyHistogramTracking)`
- Source: `tests/access_model/test_access_coordinator_focused.cpp`:443
- Brief: n/a
- Parameters:
  - `<unnamed>` (AccessCoordinatorFocusedTest): n/a
  - `<unnamed>` (ACM07_LatencyHistogramTracking): n/a

#### `TEST_F(AccessCoordinatorFocusedTest, ACM07_MetricsCollection)`
- Source: `tests/access_model/test_access_coordinator_focused.cpp`:421
- Brief: n/a
- Parameters:
  - `<unnamed>` (AccessCoordinatorFocusedTest): n/a
  - `<unnamed>` (ACM07_MetricsCollection): n/a

#### `TEST_F(AccessCoordinatorFocusedTest, ACM08_ConcurrentEvictionAndPromotion)`
- Source: `tests/access_model/test_access_coordinator_focused.cpp`:481
- Brief: n/a
- Parameters:
  - `<unnamed>` (AccessCoordinatorFocusedTest): n/a
  - `<unnamed>` (ACM08_ConcurrentEvictionAndPromotion): n/a

#### `TEST_F(AccessCoordinatorFocusedTest, ACM08_ConcurrentPromotionAndMetricsRead)`
- Source: `tests/access_model/test_access_coordinator_focused.cpp`:531
- Brief: n/a
- Parameters:
  - `<unnamed>` (AccessCoordinatorFocusedTest): n/a
  - `<unnamed>` (ACM08_ConcurrentPromotionAndMetricsRead): n/a

#### `TEST_F(AccessCoordinatorTest, ACM_01_InitializeWithTiers)`
- Source: `tests/access_model/test_access_coordinator.cpp`:129
- Brief: n/a
- Parameters:
  - `<unnamed>` (AccessCoordinatorTest): n/a
  - `<unnamed>` (ACM_01_InitializeWithTiers): n/a

#### `TEST_F(AccessCoordinatorTest, ACM_02_InitializeEmptyTiers)`
- Source: `tests/access_model/test_access_coordinator.cpp`:138
- Brief: n/a
- Parameters:
  - `<unnamed>` (AccessCoordinatorTest): n/a
  - `<unnamed>` (ACM_02_InitializeEmptyTiers): n/a

#### `TEST_F(AccessCoordinatorTest, ACM_03_SetAgePolicy)`
- Source: `tests/access_model/test_access_coordinator.cpp`:144
- Brief: n/a
- Parameters:
  - `<unnamed>` (AccessCoordinatorTest): n/a
  - `<unnamed>` (ACM_03_SetAgePolicy): n/a

#### `TEST_F(AccessCoordinatorTest, ACM_04_OnEvictionLowAccessCount)`
- Source: `tests/access_model/test_access_coordinator.cpp`:157
- Brief: n/a
- Parameters:
  - `<unnamed>` (AccessCoordinatorTest): n/a
  - `<unnamed>` (ACM_04_OnEvictionLowAccessCount): n/a

#### `TEST_F(AccessCoordinatorTest, ACM_05_OnEvictionHighAccessCount)`
- Source: `tests/access_model/test_access_coordinator.cpp`:182
- Brief: n/a
- Parameters:
  - `<unnamed>` (AccessCoordinatorTest): n/a
  - `<unnamed>` (ACM_05_OnEvictionHighAccessCount): n/a

#### `TEST_F(AccessCoordinatorTest, ACM_06_OnHotAccessAboveThreshold)`
- Source: `tests/access_model/test_access_coordinator.cpp`:209
- Brief: n/a
- Parameters:
  - `<unnamed>` (AccessCoordinatorTest): n/a
  - `<unnamed>` (ACM_06_OnHotAccessAboveThreshold): n/a

#### `TEST_F(AccessCoordinatorTest, ACM_07_OnHotAccessColdTierPromotesToWarm)`
- Source: `tests/access_model/test_access_coordinator.cpp`:230
- Brief: n/a
- Parameters:
  - `<unnamed>` (AccessCoordinatorTest): n/a
  - `<unnamed>` (ACM_07_OnHotAccessColdTierPromotesToWarm): n/a

#### `TEST_F(AccessCoordinatorTest, ACM_08_PromoteAsyncReturnsCorrectTiers)`
- Source: `tests/access_model/test_access_coordinator.cpp`:262
- Brief: n/a
- Parameters:
  - `<unnamed>` (AccessCoordinatorTest): n/a
  - `<unnamed>` (ACM_08_PromoteAsyncReturnsCorrectTiers): n/a

#### `TEST_F(AccessCoordinatorTest, ACM_09_PromoteAsyncNotRunning)`
- Source: `tests/access_model/test_access_coordinator.cpp`:277
- Brief: n/a
- Parameters:
  - `<unnamed>` (AccessCoordinatorTest): n/a
  - `<unnamed>` (ACM_09_PromoteAsyncNotRunning): n/a

#### `TEST_F(AccessCoordinatorTest, ACM_10_PlanDemotionReturnsValidPlan)`
- Source: `tests/access_model/test_access_coordinator.cpp`:293
- Brief: n/a
- Parameters:
  - `<unnamed>` (AccessCoordinatorTest): n/a
  - `<unnamed>` (ACM_10_PlanDemotionReturnsValidPlan): n/a

#### `TEST_F(AccessCoordinatorTest, ACM_11_ExecuteDemotionValidPlan)`
- Source: `tests/access_model/test_access_coordinator.cpp`:308
- Brief: n/a
- Parameters:
  - `<unnamed>` (AccessCoordinatorTest): n/a
  - `<unnamed>` (ACM_11_ExecuteDemotionValidPlan): n/a

#### `TEST_F(AccessCoordinatorTest, ACM_12_ExecuteDemotionUnknownPlan)`
- Source: `tests/access_model/test_access_coordinator.cpp`:324
- Brief: n/a
- Parameters:
  - `<unnamed>` (AccessCoordinatorTest): n/a
  - `<unnamed>` (ACM_12_ExecuteDemotionUnknownPlan): n/a

#### `TEST_F(AccessCoordinatorTest, ACM_13_GetMetricsReturnsSaneValues)`
- Source: `tests/access_model/test_access_coordinator.cpp`:334
- Brief: n/a
- Parameters:
  - `<unnamed>` (AccessCoordinatorTest): n/a
  - `<unnamed>` (ACM_13_GetMetricsReturnsSaneValues): n/a

#### `TEST_F(AccessCoordinatorTest, ACM_14_GetRecentTransitionsAfterEviction)`
- Source: `tests/access_model/test_access_coordinator.cpp`:345
- Brief: n/a
- Parameters:
  - `<unnamed>` (AccessCoordinatorTest): n/a
  - `<unnamed>` (ACM_14_GetRecentTransitionsAfterEviction): n/a

#### `TEST_F(AccessCoordinatorTest, ACM_15_SetPromotionThresholds)`
- Source: `tests/access_model/test_access_coordinator.cpp`:368
- Brief: n/a
- Parameters:
  - `<unnamed>` (AccessCoordinatorTest): n/a
  - `<unnamed>` (ACM_15_SetPromotionThresholds): n/a

#### `TEST_F(AccessModelE2ETest, T10_HotHotspot_Single1000xAccessPriorityPromotion)`
- Source: `tests/access_model/test_access_model_e2e.cpp`:351
- Brief: n/a
- Parameters:
  - `<unnamed>` (AccessModelE2ETest): n/a
  - `<unnamed>` (T10_HotHotspot_Single1000xAccessPriorityPromotion): n/a

#### `TEST_F(AccessModelE2ETest, T11_EmptyCoordinator_NoTiersRegistered)`
- Source: `tests/access_model/test_access_model_e2e.cpp`:377
- Brief: n/a
- Parameters:
  - `<unnamed>` (AccessModelE2ETest): n/a
  - `<unnamed>` (T11_EmptyCoordinator_NoTiersRegistered): n/a

#### `TEST_F(AccessModelE2ETest, T12_SingleTierPromotion_DemotionNOP)`
- Source: `tests/access_model/test_access_model_e2e.cpp`:398
- Brief: n/a
- Parameters:
  - `<unnamed>` (AccessModelE2ETest): n/a
  - `<unnamed>` (T12_SingleTierPromotion_DemotionNOP): n/a

#### `TEST_F(AccessModelE2ETest, T13_RapidFireEvents_100EventsIn10ms_QueueBackpressure)`
- Source: `tests/access_model/test_access_model_e2e.cpp`:427
- Brief: n/a
- Parameters:
  - `<unnamed>` (AccessModelE2ETest): n/a
  - `<unnamed>` (T13_RapidFireEvents_100EventsIn10ms_QueueBackpressure): n/a

#### `TEST_F(AccessModelE2ETest, T14_WorkerThreadFailureRecovery_OneThreadDiesOthersContinue)`
- Source: `tests/access_model/test_access_model_e2e.cpp`:445
- Brief: n/a
- Parameters:
  - `<unnamed>` (AccessModelE2ETest): n/a
  - `<unnamed>` (T14_WorkerThreadFailureRecovery_OneThreadDiesOthersContinue): n/a

#### `TEST_F(AccessModelE2ETest, T15_LongRunning_1000OperationsOver10sWithoutDeadlock)`
- Source: `tests/access_model/test_access_model_e2e.cpp`:465
- Brief: n/a
- Parameters:
  - `<unnamed>` (AccessModelE2ETest): n/a
  - `<unnamed>` (T15_LongRunning_1000OperationsOver10sWithoutDeadlock): n/a

#### `TEST_F(AccessModelE2ETest, T1_SingleKeyColdToWarmOnThreeAccesses)`
- Source: `tests/access_model/test_access_model_e2e.cpp`:139
- Brief: n/a
- Parameters:
  - `<unnamed>` (AccessModelE2ETest): n/a
  - `<unnamed>` (T1_SingleKeyColdToWarmOnThreeAccesses): n/a

#### `TEST_F(AccessModelE2ETest, T2_MultipleKeysLRUOrderPreservation)`
- Source: `tests/access_model/test_access_model_e2e.cpp`:165
- Brief: n/a
- Parameters:
  - `<unnamed>` (AccessModelE2ETest): n/a
  - `<unnamed>` (T2_MultipleKeysLRUOrderPreservation): n/a

#### `TEST_F(AccessModelE2ETest, T3_ConcurrentPromotions_10KeysParallelAccess)`
- Source: `tests/access_model/test_access_model_e2e.cpp`:184
- Brief: n/a
- Parameters:
  - `<unnamed>` (AccessModelE2ETest): n/a
  - `<unnamed>` (T3_ConcurrentPromotions_10KeysParallelAccess): n/a

#### `TEST_F(AccessModelE2ETest, T4_PromotionCascades_L3ToL2ToL1BackToBackWithin1s)`
- Source: `tests/access_model/test_access_model_e2e.cpp`:216
- Brief: n/a
- Parameters:
  - `<unnamed>` (AccessModelE2ETest): n/a
  - `<unnamed>` (T4_PromotionCascades_L3ToL2ToL1BackToBackWithin1s): n/a

#### `TEST_F(AccessModelE2ETest, T5_CacheL1FullL2EvictionStorageColdFeedback)`
- Source: `tests/access_model/test_access_model_e2e.cpp`:249
- Brief: n/a
- Parameters:
  - `<unnamed>` (AccessModelE2ETest): n/a
  - `<unnamed>` (T5_CacheL1FullL2EvictionStorageColdFeedback): n/a

#### `TEST_F(AccessModelE2ETest, T6_DemotionRejection_TierAlreadyFullBackpressure)`
- Source: `tests/access_model/test_access_model_e2e.cpp`:271
- Brief: n/a
- Parameters:
  - `<unnamed>` (AccessModelE2ETest): n/a
  - `<unnamed>` (T6_DemotionRejection_TierAlreadyFullBackpressure): n/a

#### `TEST_F(AccessModelE2ETest, T7_CascadingDemotions_L1FillL2FillL3Eviction)`
- Source: `tests/access_model/test_access_model_e2e.cpp`:295
- Brief: n/a
- Parameters:
  - `<unnamed>` (AccessModelE2ETest): n/a
  - `<unnamed>` (T7_CascadingDemotions_L1FillL2FillL3Eviction): n/a

#### `TEST_F(AccessModelE2ETest, T8_AgeBasedAutomaticDemotion_AgeExceedsPolicy)`
- Source: `tests/access_model/test_access_model_e2e.cpp`:316
- Brief: n/a
- Parameters:
  - `<unnamed>` (AccessModelE2ETest): n/a
  - `<unnamed>` (T8_AgeBasedAutomaticDemotion_AgeExceedsPolicy): n/a

#### `TEST_F(AccessModelE2ETest, T9_SizeBasedBlockingFromL1_LargeObjectL2OnlyPath)`
- Source: `tests/access_model/test_access_model_e2e.cpp`:334
- Brief: n/a
- Parameters:
  - `<unnamed>` (AccessModelE2ETest): n/a
  - `<unnamed>` (T9_SizeBasedBlockingFromL1_LargeObjectL2OnlyPath): n/a

#### `TEST_F(LoggingTest, LogCoordinatorLifecycle)`
- Source: `tests/access_model/test_access_model_observability.cpp`:179
- Brief: n/a
- Parameters:
  - `<unnamed>` (LoggingTest): n/a
  - `<unnamed>` (LogCoordinatorLifecycle): n/a

#### `TEST_F(LoggingTest, LogEvictionEvent)`
- Source: `tests/access_model/test_access_model_observability.cpp`:140
- Brief: n/a
- Parameters:
  - `<unnamed>` (LoggingTest): n/a
  - `<unnamed>` (LogEvictionEvent): n/a

#### `TEST_F(LoggingTest, LogPromotionDecision)`
- Source: `tests/access_model/test_access_model_observability.cpp`:158
- Brief: n/a
- Parameters:
  - `<unnamed>` (LoggingTest): n/a
  - `<unnamed>` (LogPromotionDecision): n/a

#### `TEST_F(LoggingTest, LogTierTransition)`
- Source: `tests/access_model/test_access_model_observability.cpp`:123
- Brief: n/a
- Parameters:
  - `<unnamed>` (LoggingTest): n/a
  - `<unnamed>` (LogTierTransition): n/a

#### `TEST_F(ObservabilityIntegrationTest, CorrelationPropagation)`
- Source: `tests/access_model/test_access_model_observability.cpp`:203
- Brief: n/a
- Parameters:
  - `<unnamed>` (ObservabilityIntegrationTest): n/a
  - `<unnamed>` (CorrelationPropagation): n/a

#### `TEST_F(ObservabilityIntegrationTest, HierarchicalTracing)`
- Source: `tests/access_model/test_access_model_observability.cpp`:231
- Brief: n/a
- Parameters:
  - `<unnamed>` (ObservabilityIntegrationTest): n/a
  - `<unnamed>` (HierarchicalTracing): n/a

#### `TEST_F(PromotionDemotionTest, APD_01_CreateDemotionPlan)`
- Source: `tests/access_model/test_promotion_demotion.cpp`:27
- Brief: n/a
- Parameters:
  - `<unnamed>` (PromotionDemotionTest): n/a
  - `<unnamed>` (APD_01_CreateDemotionPlan): n/a

#### `TEST_F(PromotionDemotionTest, APD_02_GracePeriodCalculation)`
- Source: `tests/access_model/test_promotion_demotion.cpp`:48
- Brief: n/a
- Parameters:
  - `<unnamed>` (PromotionDemotionTest): n/a
  - `<unnamed>` (APD_02_GracePeriodCalculation): n/a

#### `TEST_F(PromotionDemotionTest, APD_03_SuccessfulDemotionResult)`
- Source: `tests/access_model/test_promotion_demotion.cpp`:74
- Brief: n/a
- Parameters:
  - `<unnamed>` (PromotionDemotionTest): n/a
  - `<unnamed>` (APD_03_SuccessfulDemotionResult): n/a

#### `TEST_F(PromotionDemotionTest, APD_04_FailedDemotionResult)`
- Source: `tests/access_model/test_promotion_demotion.cpp`:92
- Brief: n/a
- Parameters:
  - `<unnamed>` (PromotionDemotionTest): n/a
  - `<unnamed>` (APD_04_FailedDemotionResult): n/a

#### `TEST_F(PromotionDemotionTest, APD_05_ShouldDemoteL1ToL2)`
- Source: `tests/access_model/test_promotion_demotion.cpp`:112
- Brief: n/a
- Parameters:
  - `<unnamed>` (PromotionDemotionTest): n/a
  - `<unnamed>` (APD_05_ShouldDemoteL1ToL2): n/a

#### `TEST_F(PromotionDemotionTest, APD_06_ShouldDemoteHotToWarm)`
- Source: `tests/access_model/test_promotion_demotion.cpp`:124
- Brief: n/a
- Parameters:
  - `<unnamed>` (PromotionDemotionTest): n/a
  - `<unnamed>` (APD_06_ShouldDemoteHotToWarm): n/a

#### `TEST_F(PromotionDemotionTest, APD_07_ShouldPromoteStorageToCache)`
- Source: `tests/access_model/test_promotion_demotion.cpp`:140
- Brief: n/a
- Parameters:
  - `<unnamed>` (PromotionDemotionTest): n/a
  - `<unnamed>` (APD_07_ShouldPromoteStorageToCache): n/a

#### `TEST_F(PromotionDemotionTest, APD_08_PolicyDescription)`
- Source: `tests/access_model/test_promotion_demotion.cpp`:151
- Brief: n/a
- Parameters:
  - `<unnamed>` (PromotionDemotionTest): n/a
  - `<unnamed>` (APD_08_PolicyDescription): n/a

#### `TEST_F(TraceContextTest, CurrentCorrelationID)`
- Source: `tests/access_model/test_access_model_observability.cpp`:62
- Brief: n/a
- Parameters:
  - `<unnamed>` (TraceContextTest): n/a
  - `<unnamed>` (CurrentCorrelationID): n/a

#### `TEST_F(TraceContextTest, GenerateCorrelationID)`
- Source: `tests/access_model/test_access_model_observability.cpp`:36
- Brief: n/a
- Parameters:
  - `<unnamed>` (TraceContextTest): n/a
  - `<unnamed>` (GenerateCorrelationID): n/a

#### `TEST_F(TraceContextTest, ScopedContextRAII)`
- Source: `tests/access_model/test_access_model_observability.cpp`:72
- Brief: n/a
- Parameters:
  - `<unnamed>` (TraceContextTest): n/a
  - `<unnamed>` (ScopedContextRAII): n/a

#### `TEST_F(TraceContextTest, SetAndGetContext)`
- Source: `tests/access_model/test_access_model_observability.cpp`:52
- Brief: n/a
- Parameters:
  - `<unnamed>` (TraceContextTest): n/a
  - `<unnamed>` (SetAndGetContext): n/a

#### `TEST_F(TraceContextTest, ThreadLocalIsolation)`
- Source: `tests/access_model/test_access_model_observability.cpp`:92
- Brief: n/a
- Parameters:
  - `<unnamed>` (TraceContextTest): n/a
  - `<unnamed>` (ThreadLocalIsolation): n/a

#### `AccessModelLogger & accessModelLogger()`
- Source: `src/access_model/access_model_logging.cpp`:139
- Brief: Access Model Logger.
- Parameters: none
- Return: Return value.
- Details: Return value. Return value. Implements accessModelLogger without additional internal calls.

#### `TierClassification classifyTier(TierLevel level)`
- Source: `include/access_model/access_tier_interface.h`:67
- Brief: Classify Tier.
- Parameters:
  - `level` (TierLevel): Input parameter.
- Return: Return value.
- Details: level Input parameter. Return value. Calls: THEMIS_UNREACHABLE().

#### `std::shared_ptr< AccessCoordinator > createAccessCoordinator(size_t thread_pool_size)`
- Source: `src/access_model/access_coordinator.cpp`:722
- Brief: Create Access Coordinator.
- Parameters:
  - `thread_pool_size` (size_t): Input parameter.
- Return: Return value.
- Details: thread_pool_size Input parameter. Return value. Implements createAccessCoordinator without additional internal calls.

#### `std::shared_ptr< AccessCoordinator > createAccessCoordinator(std::size_t thread_pool_size=4)`
- Source: `include/access_model/access_coordinator.h`:328
- Brief: n/a
- Parameters:
  - `thread_pool_size` (std::size_t): n/a

#### `std::string describeResult(const DemotionResult &result)`
- Source: `src/access_model/promotion_demotion.cpp`:46
- Brief: Returns a human-readable description of a DemotionResult.
- Parameters:
  - `result` (const DemotionResult &): Completed demotion result
- Return: Formatted string representation
- Details: Intended for logging and diagnostic output. result Completed demotion result Formatted string representation

#### `for(auto _ :state)`
- Source: `benchmarks/access_model/bench_access_coordinator_gates.cpp`:225
- Brief: n/a
- Parameters:
  - `<unnamed>` (auto _ :state): n/a

#### `bool isPlanExecutable(const DemotionPlan &plan)`
- Source: `src/access_model/promotion_demotion.cpp`:30
- Brief: Returns true when the grace period has elapsed and the plan is ready for execution.
- Parameters:
  - `plan` (const DemotionPlan &): Demotion plan created by AccessCoordinator::planDemotion()
- Return: true if now >= plan.scheduled_execution_time
- Details: Used by AccessCoordinator::executeDemotion() to guard against early execution. plan Demotion plan created by AccessCoordinator::planDemotion() true if now >= plan.scheduled_execution_time

#### `std::uniform_int_distribution key_dist(0, 99)`
- Source: `benchmarks/access_model/bench_access_coordinator_gates.cpp`:221
- Brief: n/a
- Parameters:
  - `<unnamed>` (0): n/a
  - `<unnamed>` (99): n/a

#### `std::string_view tierLevelName(TierLevel level)`
- Source: `include/access_model/access_tier_interface.h`:88
- Brief: Tier Level Name.
- Parameters:
  - `level` (TierLevel): Input parameter.
- Return: Return value.
- Details: level Input parameter. Return value. Implements tierLevelName without additional internal calls.

### themis::access_model::AccessCoordinator

#### `std::optional< DemotionResult > executeDemotion(const std::string &plan_id)=0`
- Source: `include/access_model/access_coordinator.h`:296
- Brief: Execute Demotion.
- Parameters:
  - `plan_id` (const std::string &): Identifier of the plan.
- Return: Return value.
- Details: plan_id Identifier of the plan. Return value.

#### `AccessModelMetrics getAccessModelMetrics()=0`
- Source: `include/access_model/access_coordinator.h`:318
- Brief: Get Access Model Metrics.
- Parameters: none
- Return: Return value.
- Details: Return value.

#### `AccessMetrics getKeyMetrics(const std::string &key)=0`
- Source: `include/access_model/access_coordinator.h`:305
- Brief: Get Key Metrics.
- Parameters:
  - `key` (const std::string &): Input parameter.
- Return: Return value.
- Details: key Input parameter. Return value.

#### `std::vector< AccessTransitionEvent > getRecentTransitions(std::size_t limit=100)=0`
- Source: `include/access_model/access_coordinator.h`:320
- Brief: n/a
- Parameters:
  - `limit` (std::size_t): n/a

#### `AccessMetrics getTierMetrics(TierLevel tier_level)=0`
- Source: `include/access_model/access_coordinator.h`:312
- Brief: Get Tier Metrics.
- Parameters:
  - `tier_level` (TierLevel): Input parameter.
- Return: Return value.
- Details: tier_level Input parameter. Return value.

#### `bool initialize(const std::map< TierLevel, std::shared_ptr< AccessTier > > &all_tiers)=0`
- Source: `include/access_model/access_coordinator.h`:174
- Brief: n/a
- Parameters:
  - `all_tiers` (const std::map< TierLevel, std::shared_ptr< AccessTier > > &): n/a

#### `bool isRunning() const =0`
- Source: `include/access_model/access_coordinator.h`:191
- Brief: Is Running.
- Parameters: none
- Return: True when the operation succeeds.
- Details: True when the operation succeeds.

#### `void onCacheEvicted(std::string_view key, TierLevel from_tier, std::size_t size_bytes, uint64_t access_count, std::chrono::seconds last_access_age_secs, std::string_view eviction_reason)`
- Source: `include/access_model/access_coordinator.h`:216
- Brief: On Cache Evicted.
- Parameters:
  - `key` (std::string_view): Input parameter.
  - `from_tier` (TierLevel): Input parameter.
  - `size_bytes` (std::size_t): Input parameter.
  - `access_count` (uint64_t): Input parameter.
  - `last_access_age_secs` (std::chrono::seconds): Input parameter.
  - `eviction_reason` (std::string_view): Input parameter.
- Details: key Input parameter. from_tier Input parameter. size_bytes Input parameter. access_count Input parameter. last_access_age_secs Input parameter. eviction_reason Input parameter. Calls: std::string(), onEviction().

#### `void onEviction(const EvictionEvent &event)=0`
- Source: `include/access_model/access_coordinator.h`:198
- Brief: On Eviction.
- Parameters:
  - `event` (const EvictionEvent &): Input parameter.
- Details: event Input parameter.

#### `void onHotAccess(const AccessEvent &event)=0`
- Source: `include/access_model/access_coordinator.h`:204
- Brief: On Hot Access.
- Parameters:
  - `event` (const AccessEvent &): Input parameter.
- Details: event Input parameter.

#### `void onStorageAccess(std::string_view key, TierLevel from_tier, uint64_t access_count, std::chrono::seconds access_window)`
- Source: `include/access_model/access_coordinator.h`:238
- Brief: On Storage Access.
- Parameters:
  - `key` (std::string_view): Input parameter.
  - `from_tier` (TierLevel): Input parameter.
  - `access_count` (uint64_t): Input parameter.
  - `access_window` (std::chrono::seconds): Input parameter.
- Details: key Input parameter. from_tier Input parameter. access_count Input parameter. access_window Input parameter. Calls: std::string(), onHotAccess().

#### `std::optional< DemotionPlan > planDemotion(const std::string &key, TierLevel from_tier, TierLevel to_tier, uint64_t data_size_bytes)=0`
- Source: `include/access_model/access_coordinator.h`:286
- Brief: Plan Demotion.
- Parameters:
  - `key` (const std::string &): Input parameter.
  - `from_tier` (TierLevel): Input parameter.
  - `to_tier` (TierLevel): Input parameter.
  - `data_size_bytes` (uint64_t): Input parameter.
- Return: Return value.
- Details: key Input parameter. from_tier Input parameter. to_tier Input parameter. data_size_bytes Input parameter. Return value.

#### `std::future< PromotionResult > promoteAsync(const std::string &key, TierLevel from_tier, TierLevel to_tier, uint64_t size_bytes)=0`
- Source: `include/access_model/access_coordinator.h`:273
- Brief: Promote Async.
- Parameters:
  - `key` (const std::string &): Input parameter.
  - `from_tier` (TierLevel): Input parameter.
  - `to_tier` (TierLevel): Input parameter.
  - `size_bytes` (uint64_t): Input parameter.
- Return: Return value.
- Details: key Input parameter. from_tier Input parameter. to_tier Input parameter. size_bytes Input parameter. Return value.

#### `void setAgePolicy(const AgeBasedPolicy &policy)=0`
- Source: `include/access_model/access_coordinator.h`:254
- Brief: Set Age Policy.
- Parameters:
  - `policy` (const AgeBasedPolicy &): Input parameter.
- Details: policy Input parameter.

#### `void setPromotionThresholds(uint64_t cache_threshold, uint64_t storage_threshold)=0`
- Source: `include/access_model/access_coordinator.h`:261
- Brief: Set Promotion Thresholds.
- Parameters:
  - `cache_threshold` (uint64_t): Input parameter.
  - `storage_threshold` (uint64_t): Input parameter.
- Details: cache_threshold Input parameter. storage_threshold Input parameter.

#### `void shutdown()=0`
- Source: `include/access_model/access_coordinator.h`:185
- Brief: Shutdown.
- Parameters: none

#### `void start()=0`
- Source: `include/access_model/access_coordinator.h`:180
- Brief: Start.
- Parameters: none

#### `~AccessCoordinator()=default`
- Source: `include/access_model/access_coordinator.h`:171
- Brief: Access Coordinator.
- Parameters: none
- Return: Return value.
- Details: Return value.

### themis::access_model::AccessCoordinatorConcurrencyTest

#### `void SetUp() override`
- Source: `tests/access_model/test_coordination_concurrency.cpp`:68
- Brief: n/a
- Parameters: none

#### `void TearDown() override`
- Source: `tests/access_model/test_coordination_concurrency.cpp`:121
- Brief: n/a
- Parameters: none

### themis::access_model::AccessCoordinatorFocusedTest

#### `void SetUp() override`
- Source: `tests/access_model/test_access_coordinator_focused.cpp`:89
- Brief: n/a
- Parameters: none

#### `void TearDown() override`
- Source: `tests/access_model/test_access_coordinator_focused.cpp`:108
- Brief: n/a
- Parameters: none

### themis::access_model::AccessCoordinatorImpl

#### `AccessCoordinatorImpl(size_t thread_pool_size=4)`
- Source: `src/access_model/access_coordinator.cpp`:50
- Brief: n/a
- Parameters:
  - `thread_pool_size` (size_t): n/a

#### `std::optional< DemotionResult > executeDemotion(const std::string &plan_id) override`
- Source: `src/access_model/access_coordinator.cpp`:432
- Brief: Execute Demotion.
- Parameters:
  - `plan_id` (const std::string &): Identifier of the plan.
- Return: Return value.
- Details: plan_id Identifier of the plan. Return value.

#### `std::string generateCorrelationId(const std::string &prefix)`
- Source: `src/access_model/access_coordinator.cpp`:678
- Brief: Generate Correlation Id.
- Parameters:
  - `prefix` (const std::string &): Input parameter.
- Return: Return value.
- Details: prefix Input parameter. Return value. Calls: std::to_string().

#### `std::string generatePlanId()`
- Source: `src/access_model/access_coordinator.cpp`:688
- Brief: Generate Plan Id.
- Parameters: none
- Return: Return value.
- Details: Return value. Calls: std::to_string().

#### `AccessModelMetrics getAccessModelMetrics() override`
- Source: `src/access_model/access_coordinator.cpp`:535
- Brief: Get Access Model Metrics.
- Parameters: none
- Return: Return value.
- Details: Return value.

#### `AccessMetrics getKeyMetrics(const std::string &key) override`
- Source: `src/access_model/access_coordinator.cpp`:511
- Brief: Get Key Metrics.
- Parameters:
  - `key` (const std::string &): Input parameter.
- Return: Return value.
- Details: key Input parameter. Return value.

#### `std::vector< AccessTransitionEvent > getRecentTransitions(size_t limit=100) override`
- Source: `src/access_model/access_coordinator.cpp`:545
- Brief: n/a
- Parameters:
  - `limit` (size_t): n/a

#### `AccessMetrics getTierMetrics(TierLevel tier) override`
- Source: `src/access_model/access_coordinator.cpp`:523
- Brief: Get Tier Metrics.
- Parameters:
  - `tier_level` (TierLevel): Input parameter.
- Return: Return value.
- Details: tier_level Input parameter. Return value.

#### `bool initialize(const std::map< TierLevel, std::shared_ptr< AccessTier > > &tiers) override`
- Source: `src/access_model/access_coordinator.cpp`:63
- Brief: n/a
- Parameters:
  - `tiers` (const std::map< TierLevel, std::shared_ptr< AccessTier > > &): n/a

#### `bool isRunning() const override`
- Source: `src/access_model/access_coordinator.cpp`:146
- Brief: Is Running.
- Parameters: none
- Return: True when the operation succeeds.
- Details: True when the operation succeeds.

#### `void onEviction(const EvictionEvent &event) override`
- Source: `src/access_model/access_coordinator.cpp`:149
- Brief: On Eviction.
- Parameters:
  - `event` (const EvictionEvent &): Input parameter.
- Details: event Input parameter.

#### `void onHotAccess(const AccessEvent &event) override`
- Source: `src/access_model/access_coordinator.cpp`:246
- Brief: On Hot Access.
- Parameters:
  - `event` (const AccessEvent &): Input parameter.
- Details: event Input parameter.

#### `std::optional< DemotionPlan > planDemotion(const std::string &key, TierLevel from_tier, TierLevel to_tier, uint64_t data_size_bytes) override`
- Source: `src/access_model/access_coordinator.cpp`:397
- Brief: Plan Demotion.
- Parameters:
  - `key` (const std::string &): Input parameter.
  - `from_tier` (TierLevel): Input parameter.
  - `to_tier` (TierLevel): Input parameter.
  - `data_size_bytes` (uint64_t): Input parameter.
- Return: Return value.
- Details: key Input parameter. from_tier Input parameter. to_tier Input parameter. data_size_bytes Input parameter. Return value.

#### `void processPromotionTask(const DemotionEvent &task)`
- Source: `src/access_model/access_coordinator.cpp`:615
- Brief: Process Promotion Task.
- Parameters:
  - `task` (const DemotionEvent &): Input parameter.
- Details: task Input parameter. Calls: std::chrono::system_clock::now(), generateCorrelationId(), lock(), set_value(), emplace_back(), THEMIS_DEBUG(), tierLevelName(), count().

#### `std::future< PromotionResult > promoteAsync(const std::string &key, TierLevel from_tier, TierLevel to_tier, uint64_t size_bytes) override`
- Source: `src/access_model/access_coordinator.cpp`:345
- Brief: Promote Async.
- Parameters:
  - `key` (const std::string &): Input parameter.
  - `from_tier` (TierLevel): Input parameter.
  - `to_tier` (TierLevel): Input parameter.
  - `size_bytes` (uint64_t): Input parameter.
- Return: Return value.
- Details: key Input parameter. from_tier Input parameter. to_tier Input parameter. size_bytes Input parameter. Return value.

#### `void setAgePolicy(const AgeBasedPolicy &policy) override`
- Source: `src/access_model/access_coordinator.cpp`:486
- Brief: Set Age Policy.
- Parameters:
  - `policy` (const AgeBasedPolicy &): Input parameter.
- Details: policy Input parameter.

#### `void setPromotionThresholds(uint64_t cache_threshold, uint64_t storage_threshold) override`
- Source: `src/access_model/access_coordinator.cpp`:499
- Brief: Set Promotion Thresholds.
- Parameters:
  - `cache_threshold` (uint64_t): Input parameter.
  - `storage_threshold` (uint64_t): Input parameter.
- Details: cache_threshold Input parameter. storage_threshold Input parameter.

#### `void shutdown() override`
- Source: `src/access_model/access_coordinator.cpp`:108
- Brief: Shutdown.
- Parameters: none

#### `void start() override`
- Source: `src/access_model/access_coordinator.cpp`:76
- Brief: Start.
- Parameters: none

#### `void workerMain()`
- Source: `src/access_model/access_coordinator.cpp`:570
- Brief: Worker Main.
- Parameters: none
- Details: Calls: generateCorrelationId(), trace_guard(), THEMIS_DEBUG(), lock(), wait(), empty(), front(), pop().

#### `~AccessCoordinatorImpl()`
- Source: `src/access_model/access_coordinator.cpp`:56
- Brief: n/a
- Parameters: none

### themis::access_model::AccessCoordinatorTest

#### `void SetUp() override`
- Source: `tests/access_model/test_access_coordinator.cpp`:87
- Brief: n/a
- Parameters: none

#### `void TearDown() override`
- Source: `tests/access_model/test_access_coordinator.cpp`:98
- Brief: n/a
- Parameters: none

#### `std::map< TierLevel, std::shared_ptr< AccessTier > > allTiers()`
- Source: `tests/access_model/test_access_coordinator.cpp`:104
- Brief: n/a
- Parameters: none

### themis::access_model::AccessMetrics

#### `double cacheHitRate() const`
- Source: `include/access_model/access_metrics.h`:103
- Brief: n/a
- Parameters: none

#### `std::string describe() const`
- Source: `include/access_model/access_metrics.h`:105
- Brief: n/a
- Parameters: none

#### `void recordAccess(uint64_t latency_us)`
- Source: `include/access_model/access_metrics.h`:86
- Brief: Record Access.
- Parameters:
  - `latency_us` (uint64_t): Input parameter.
- Details: ============================================================================ § 2 Access Metrics (Per-Key / Per-Tier) ============================================================================ latency_us Input parameter. latency_us Input parameter. Calls: std::chrono::system_clock::now(), record().

#### `void recordCacheHit()`
- Source: `include/access_model/access_metrics.h`:91
- Brief: Record Cache Hit.
- Parameters: none
- Details: Implements recordCacheHit without additional internal calls.

#### `void recordCacheMiss()`
- Source: `include/access_model/access_metrics.h`:96
- Brief: Record Cache Miss.
- Parameters: none
- Details: Implements recordCacheMiss without additional internal calls.

#### `void recordEviction()`
- Source: `include/access_model/access_metrics.h`:101
- Brief: Record Eviction.
- Parameters: none
- Details: Implements recordEviction without additional internal calls.

### themis::access_model::AccessModelE2ETest

#### `void SetUp() override`
- Source: `tests/access_model/test_access_model_e2e.cpp`:69
- Brief: n/a
- Parameters: none

#### `void TearDown() override`
- Source: `tests/access_model/test_access_model_e2e.cpp`:120
- Brief: n/a
- Parameters: none

### themis::access_model::AccessModelLogger

#### `void logCoordinatorLifecycle(const CoordinatorLifecycleLog &log)=0`
- Source: `include/access_model/access_model_logging.h`:157
- Brief: Log Coordinator Lifecycle.
- Parameters:
  - `log` (const CoordinatorLifecycleLog &): Input parameter.
- Details: log Input parameter.

#### `void logEvictionEvent(const EvictionEventLog &log)=0`
- Source: `include/access_model/access_model_logging.h`:145
- Brief: Log Eviction Event.
- Parameters:
  - `log` (const EvictionEventLog &): Input parameter.
- Details: log Input parameter.

#### `void logPromotionDecision(const PromotionDecisionLog &log)=0`
- Source: `include/access_model/access_model_logging.h`:151
- Brief: Log Promotion Decision.
- Parameters:
  - `log` (const PromotionDecisionLog &): Input parameter.
- Details: log Input parameter.

#### `void logTierTransition(const TierTransitionLog &log)=0`
- Source: `include/access_model/access_model_logging.h`:139
- Brief: Log Tier Transition.
- Parameters:
  - `log` (const TierTransitionLog &): Input parameter.
- Details: log Input parameter.

#### `~AccessModelLogger()=default`
- Source: `include/access_model/access_model_logging.h`:133
- Brief: Access Model Logger.
- Parameters: none
- Return: Return value.
- Details: Return value.

### themis::access_model::AccessModelMetrics

#### `AccessModelMetrics()`
- Source: `include/access_model/access_metrics.h`:125
- Brief: n/a
- Parameters: none

#### `double coordinationOverheadPercent() const`
- Source: `include/access_model/access_metrics.h`:148
- Brief: n/a
- Parameters: none

#### `std::string describe() const`
- Source: `include/access_model/access_metrics.h`:150
- Brief: n/a
- Parameters: none

#### `std::string detailedReport() const`
- Source: `include/access_model/access_metrics.h`:152
- Brief: n/a
- Parameters: none

#### `void recordEventProcessingLatency(uint64_t latency_us)`
- Source: `include/access_model/access_metrics.h`:132
- Brief: Record Event Processing Latency.
- Parameters:
  - `latency_us` (uint64_t): Input parameter.
- Details: latency_us Input parameter. latency_us Input parameter. Calls: record().

#### `void recordPolicyDecisionLatency(uint64_t latency_us)`
- Source: `include/access_model/access_metrics.h`:144
- Brief: Record Policy Decision Latency.
- Parameters:
  - `latency_us` (uint64_t): Input parameter.
- Details: latency_us Input parameter. latency_us Input parameter. Calls: record().

#### `void recordTierPromotionLatency(uint64_t latency_us)`
- Source: `include/access_model/access_metrics.h`:138
- Brief: Record Tier Promotion Latency.
- Parameters:
  - `latency_us` (uint64_t): Input parameter.
- Details: latency_us Input parameter. latency_us Input parameter. Calls: record().

### themis::access_model::AccessTier

#### `std::chrono::milliseconds estimatePromotionLatency(TierLevel from_tier, std::size_t data_size_bytes) const`
- Source: `include/access_model/access_tier_interface.h`:288
- Brief: n/a
- Parameters:
  - `from_tier` (TierLevel): n/a
  - `data_size_bytes` (std::size_t): n/a

#### `TierGetResult get(std::string_view key, const TierAccessOptions &options)=0`
- Source: `include/access_model/access_tier_interface.h`:193
- Brief: Get.
- Parameters:
  - `key` (std::string_view): Input parameter.
  - `options` (const TierAccessOptions &): Input parameter.
- Return: Return value.
- Details: key Input parameter. options Input parameter. Return value.

#### `uint64_t getAccessCount(std::string_view key) const =0`
- Source: `include/access_model/access_tier_interface.h`:274
- Brief: Get Access Count.
- Parameters:
  - `key` (std::string_view): Input parameter.
- Return: Return value.
- Details: key Input parameter. Return value.

#### `std::chrono::microseconds getAverageGetLatency() const =0`
- Source: `include/access_model/access_tier_interface.h`:261
- Brief: Get Average Get Latency.
- Parameters: none
- Return: Return value.
- Details: Return value.

#### `std::chrono::microseconds getAveragePutLatency() const =0`
- Source: `include/access_model/access_tier_interface.h`:267
- Brief: Get Average Put Latency.
- Parameters: none
- Return: Return value.
- Details: Return value.

#### `std::size_t getCurrentSizeBytes() const =0`
- Source: `include/access_model/access_tier_interface.h`:236
- Brief: Get Current Size Bytes.
- Parameters: none
- Return: Return value.
- Details: Return value.

#### `std::size_t getEntryCount() const =0`
- Source: `include/access_model/access_tier_interface.h`:248
- Brief: Get Entry Count.
- Parameters: none
- Return: Return value.
- Details: Return value.

#### `double getHitRate() const =0`
- Source: `include/access_model/access_tier_interface.h`:255
- Brief: Get Hit Rate.
- Parameters: none
- Return: Return value.
- Details: Return value.

#### `std::chrono::seconds getKeyAge(std::string_view key) const =0`
- Source: `include/access_model/access_tier_interface.h`:281
- Brief: Get Key Age.
- Parameters:
  - `key` (std::string_view): Input parameter.
- Return: Return value.
- Details: key Input parameter. Return value.

#### `std::size_t getMaxCapacityBytes() const =0`
- Source: `include/access_model/access_tier_interface.h`:242
- Brief: Get Max Capacity Bytes.
- Parameters: none
- Return: Return value.
- Details: Return value.

#### `TierLevel getTierLevel() const =0`
- Source: `include/access_model/access_tier_interface.h`:217
- Brief: Get Tier Level.
- Parameters: none
- Return: Return value.
- Details: Return value.

#### `std::string getTierName() const =0`
- Source: `include/access_model/access_tier_interface.h`:223
- Brief: Get Tier Name.
- Parameters: none
- Return: Return value.
- Details: Return value.

#### `bool hasKey(std::string_view key) const =0`
- Source: `include/access_model/access_tier_interface.h`:230
- Brief: Has Key.
- Parameters:
  - `key` (std::string_view): Input parameter.
- Return: True when the operation succeeds.
- Details: key Input parameter. True when the operation succeeds.

#### `bool initialize()=0`
- Source: `include/access_model/access_tier_interface.h`:300
- Brief: Initialize.
- Parameters: none
- Return: True when the operation succeeds.
- Details: True when the operation succeeds.

#### `bool invalidate(std::string_view key)=0`
- Source: `include/access_model/access_tier_interface.h`:210
- Brief: Invalidate.
- Parameters:
  - `key` (std::string_view): Input parameter.
- Return: True when the operation succeeds.
- Details: key Input parameter. True when the operation succeeds.

#### `bool isHealthy() const =0`
- Source: `include/access_model/access_tier_interface.h`:311
- Brief: Is Healthy.
- Parameters: none
- Return: True when the operation succeeds.
- Details: True when the operation succeeds.

#### `TierPutResult put(std::string_view key, std::string_view value, const TierAccessOptions &options)=0`
- Source: `include/access_model/access_tier_interface.h`:202
- Brief: Put.
- Parameters:
  - `key` (std::string_view): Input parameter.
  - `value` (std::string_view): Input parameter.
  - `options` (const TierAccessOptions &): Input parameter.
- Return: Return value.
- Details: key Input parameter. value Input parameter. options Input parameter. Return value.

#### `void shutdown()=0`
- Source: `include/access_model/access_tier_interface.h`:305
- Brief: Shutdown.
- Parameters: none

#### `bool supportsDemotion() const`
- Source: `include/access_model/access_tier_interface.h`:286
- Brief: n/a
- Parameters: none

#### `bool supportsPromotion() const`
- Source: `include/access_model/access_tier_interface.h`:284
- Brief: n/a
- Parameters: none

#### `~AccessTier()=default`
- Source: `include/access_model/access_tier_interface.h`:184
- Brief: Access Tier.
- Parameters: none
- Return: Return value.
- Details: Return value.

### themis::access_model::AgeBasedPolicy

#### `DataHotnessLevel classifyHotness(uint64_t access_count, const std::chrono::seconds &time_since_last_access) const`
- Source: `include/access_model/age_based_policy.h`:186
- Brief: Classify Hotness.
- Parameters:
  - `access_count` (uint64_t): Input parameter.
  - `time_since_last_access` (const std::chrono::seconds &): Input parameter.
- Return: Return value.
- Details: access_count Input parameter. time_since_last_access Input parameter. Return value.

#### `std::string describe() const`
- Source: `include/access_model/age_based_policy.h`:216
- Brief: Describe.
- Parameters: none
- Return: Return value.
- Details: Return value.

#### `bool isValid() const`
- Source: `include/access_model/age_based_policy.h`:204
- Brief: Is Valid.
- Parameters: none
- Return: True when the operation succeeds.
- Details: True when the operation succeeds.

#### `TierLevel recommendTierForData(uint64_t access_count, const std::chrono::seconds &time_since_last_access) const`
- Source: `include/access_model/age_based_policy.h`:196
- Brief: Recommend Tier For Data.
- Parameters:
  - `access_count` (uint64_t): Input parameter.
  - `time_since_last_access` (const std::chrono::seconds &): Input parameter.
- Return: Return value.
- Details: access_count Input parameter. time_since_last_access Input parameter. Return value.

#### `bool shouldDemoteHotToWarm(uint32_t seconds_since_access, uint32_t seconds_since_write) const noexcept`
- Source: `include/access_model/age_based_policy.h`:89
- Brief: n/a
- Parameters:
  - `seconds_since_access` (uint32_t): n/a
  - `seconds_since_write` (uint32_t): n/a

#### `bool shouldDemoteL1ToL2(uint32_t seconds_since_access) const noexcept`
- Source: `include/access_model/age_based_policy.h`:75
- Brief: n/a
- Parameters:
  - `seconds_since_access` (uint32_t): n/a

#### `bool shouldDemoteL2ToL3(uint32_t seconds_since_access) const noexcept`
- Source: `include/access_model/age_based_policy.h`:82
- Brief: n/a
- Parameters:
  - `seconds_since_access` (uint32_t): n/a

#### `bool shouldDemoteStorageCold(uint64_t access_count, const std::chrono::seconds &time_since_last_access) const`
- Source: `include/access_model/age_based_policy.h`:162
- Brief: Should Demote Storage Cold.
- Parameters:
  - `access_count` (uint64_t): Input parameter.
  - `time_since_last_access` (const std::chrono::seconds &): Input parameter.
- Return: True when the operation succeeds.
- Details: access_count Input parameter. time_since_last_access Input parameter. True when the operation succeeds.

#### `bool shouldDemoteWarmToCold(uint32_t seconds_since_access, uint32_t seconds_since_write) const noexcept`
- Source: `include/access_model/age_based_policy.h`:101
- Brief: n/a
- Parameters:
  - `seconds_since_access` (uint32_t): n/a
  - `seconds_since_write` (uint32_t): n/a

#### `bool shouldPromoteL1ToL2(uint64_t access_count, const std::chrono::seconds &time_since_last_access) const`
- Source: `include/access_model/age_based_policy.h`:125
- Brief: Should Promote L1 To L2.
- Parameters:
  - `access_count` (uint64_t): Input parameter.
  - `time_since_last_access` (const std::chrono::seconds &): Input parameter.
- Return: True when the operation succeeds.
- Details: access_count Input parameter. time_since_last_access Input parameter. True when the operation succeeds.

#### `bool shouldPromoteL2ToL3(uint64_t access_count, const std::chrono::seconds &time_since_last_access) const`
- Source: `include/access_model/age_based_policy.h`:134
- Brief: Should Promote L2 To L3.
- Parameters:
  - `access_count` (uint64_t): Input parameter.
  - `time_since_last_access` (const std::chrono::seconds &): Input parameter.
- Return: True when the operation succeeds.
- Details: access_count Input parameter. time_since_last_access Input parameter. True when the operation succeeds.

#### `bool shouldPromoteL3ToStorage(uint64_t access_count, const std::chrono::seconds &time_since_last_access) const`
- Source: `include/access_model/age_based_policy.h`:143
- Brief: Should Promote L3 To Storage.
- Parameters:
  - `access_count` (uint64_t): Input parameter.
  - `time_since_last_access` (const std::chrono::seconds &): Input parameter.
- Return: True when the operation succeeds.
- Details: access_count Input parameter. time_since_last_access Input parameter. True when the operation succeeds.

#### `bool shouldPromoteStorageColdToWarm(uint64_t access_count) const`
- Source: `include/access_model/age_based_policy.h`:171
- Brief: Should Promote Storage Cold To Warm.
- Parameters:
  - `access_count` (uint64_t): Input parameter.
- Return: True when the operation succeeds.
- Details: access_count Input parameter. True when the operation succeeds.

#### `bool shouldPromoteStorageToCache(uint64_t access_count) const noexcept`
- Source: `include/access_model/age_based_policy.h`:114
- Brief: n/a
- Parameters:
  - `access_count` (uint64_t): n/a

#### `bool shouldPromoteStorageWarmToCold(uint64_t access_count, const std::chrono::seconds &time_since_last_access) const`
- Source: `include/access_model/age_based_policy.h`:152
- Brief: Should Promote Storage Warm To Cold.
- Parameters:
  - `access_count` (uint64_t): Input parameter.
  - `time_since_last_access` (const std::chrono::seconds &): Input parameter.
- Return: True when the operation succeeds.
- Details: access_count Input parameter. time_since_last_access Input parameter. True when the operation succeeds.

#### `bool shouldPromoteStorageWarmToL3Cache(uint64_t access_count) const`
- Source: `include/access_model/age_based_policy.h`:178
- Brief: Should Promote Storage Warm To L3 Cache.
- Parameters:
  - `access_count` (uint64_t): Input parameter.
- Return: True when the operation succeeds.
- Details: access_count Input parameter. True when the operation succeeds.

#### `std::string toJson() const`
- Source: `include/access_model/age_based_policy.h`:210
- Brief: To Json.
- Parameters: none
- Return: Return value.
- Details: Return value.

### themis::access_model::BenchAccessCoordinator

#### `void SetUp(const benchmark::State &state) override`
- Source: `benchmarks/access_model/bench_access_coordinator_gates.cpp`:168
- Brief: n/a
- Parameters:
  - `state` (const benchmark::State &): n/a

#### `void TearDown(const benchmark::State &state) override`
- Source: `benchmarks/access_model/bench_access_coordinator_gates.cpp`:195
- Brief: n/a
- Parameters:
  - `state` (const benchmark::State &): n/a

### themis::access_model::BenchmarkAccessTier

#### `BenchmarkAccessTier(TierLevel level, std::size_t capacity)`
- Source: `benchmarks/access_model/bench_access_coordinator_gates.cpp`:151
- Brief: n/a
- Parameters:
  - `level` (TierLevel): n/a
  - `capacity` (std::size_t): n/a

#### `TierGetResult get(std::string_view, const TierAccessOptions &) override`
- Source: `benchmarks/access_model/bench_access_coordinator_gates.cpp`:89
- Brief: Get.
- Parameters:
  - `key` (std::string_view): Input parameter.
  - `options` (const TierAccessOptions &): Input parameter.
- Return: Return value.
- Details: key Input parameter. options Input parameter. Return value.

#### `uint64_t getAccessCount(std::string_view) const override`
- Source: `benchmarks/access_model/bench_access_coordinator_gates.cpp`:139
- Brief: Get Access Count.
- Parameters:
  - `key` (std::string_view): Input parameter.
- Return: Return value.
- Details: key Input parameter. Return value.

#### `std::chrono::microseconds getAverageGetLatency() const override`
- Source: `benchmarks/access_model/bench_access_coordinator_gates.cpp`:131
- Brief: Get Average Get Latency.
- Parameters: none
- Return: Return value.
- Details: Return value.

#### `std::chrono::microseconds getAveragePutLatency() const override`
- Source: `benchmarks/access_model/bench_access_coordinator_gates.cpp`:135
- Brief: Get Average Put Latency.
- Parameters: none
- Return: Return value.
- Details: Return value.

#### `std::size_t getCurrentSizeBytes() const override`
- Source: `benchmarks/access_model/bench_access_coordinator_gates.cpp`:121
- Brief: Get Current Size Bytes.
- Parameters: none
- Return: Return value.
- Details: Return value.

#### `std::size_t getEntryCount() const override`
- Source: `benchmarks/access_model/bench_access_coordinator_gates.cpp`:127
- Brief: Get Entry Count.
- Parameters: none
- Return: Return value.
- Details: Return value.

#### `double getHitRate() const override`
- Source: `benchmarks/access_model/bench_access_coordinator_gates.cpp`:129
- Brief: Get Hit Rate.
- Parameters: none
- Return: Return value.
- Details: Return value.

#### `std::chrono::seconds getKeyAge(std::string_view) const override`
- Source: `benchmarks/access_model/bench_access_coordinator_gates.cpp`:141
- Brief: Get Key Age.
- Parameters:
  - `key` (std::string_view): Input parameter.
- Return: Return value.
- Details: key Input parameter. Return value.

#### `std::size_t getMaxCapacityBytes() const override`
- Source: `benchmarks/access_model/bench_access_coordinator_gates.cpp`:125
- Brief: Get Max Capacity Bytes.
- Parameters: none
- Return: Return value.
- Details: Return value.

#### `TierLevel getTierLevel() const override`
- Source: `benchmarks/access_model/bench_access_coordinator_gates.cpp`:100
- Brief: Get Tier Level.
- Parameters: none
- Return: Return value.
- Details: Return value.

#### `std::string getTierName() const override`
- Source: `benchmarks/access_model/bench_access_coordinator_gates.cpp`:102
- Brief: Get Tier Name.
- Parameters: none
- Return: Return value.
- Details: Return value.

#### `bool hasKey(std::string_view) const override`
- Source: `benchmarks/access_model/bench_access_coordinator_gates.cpp`:119
- Brief: Has Key.
- Parameters:
  - `key` (std::string_view): Input parameter.
- Return: True when the operation succeeds.
- Details: key Input parameter. True when the operation succeeds.

#### `bool initialize() override`
- Source: `benchmarks/access_model/bench_access_coordinator_gates.cpp`:145
- Brief: Initialize.
- Parameters: none
- Return: True when the operation succeeds.
- Details: True when the operation succeeds.

#### `bool invalidate(std::string_view) override`
- Source: `benchmarks/access_model/bench_access_coordinator_gates.cpp`:98
- Brief: Invalidate.
- Parameters:
  - `key` (std::string_view): Input parameter.
- Return: True when the operation succeeds.
- Details: key Input parameter. True when the operation succeeds.

#### `bool isHealthy() const override`
- Source: `benchmarks/access_model/bench_access_coordinator_gates.cpp`:149
- Brief: Is Healthy.
- Parameters: none
- Return: True when the operation succeeds.
- Details: True when the operation succeeds.

#### `TierPutResult put(std::string_view, std::string_view, const TierAccessOptions &) override`
- Source: `benchmarks/access_model/bench_access_coordinator_gates.cpp`:93
- Brief: Put.
- Parameters:
  - `key` (std::string_view): Input parameter.
  - `value` (std::string_view): Input parameter.
  - `options` (const TierAccessOptions &): Input parameter.
- Return: Return value.
- Details: key Input parameter. value Input parameter. options Input parameter. Return value.

#### `void setCurrentSize(std::size_t size)`
- Source: `benchmarks/access_model/bench_access_coordinator_gates.cpp`:154
- Brief: n/a
- Parameters:
  - `size` (std::size_t): n/a

#### `void shutdown() override`
- Source: `benchmarks/access_model/bench_access_coordinator_gates.cpp`:147
- Brief: Shutdown.
- Parameters: none

### themis::access_model::CacheTier

#### `void notifyEviction(std::string_view key, std::size_t size_bytes, uint64_t access_count)=0`
- Source: `include/access_model/access_tier_interface.h`:326
- Brief: Notify Eviction.
- Parameters:
  - `key` (std::string_view): Input parameter.
  - `size_bytes` (std::size_t): Input parameter.
  - `access_count` (uint64_t): Input parameter.
- Details: key Input parameter. size_bytes Input parameter. access_count Input parameter.

### themis::access_model::DefaultAccessModelLogger

#### `void logCoordinatorLifecycle(const CoordinatorLifecycleLog &log) override`
- Source: `include/access_model/access_model_logging.h`:165
- Brief: Log Coordinator Lifecycle.
- Parameters:
  - `log` (const CoordinatorLifecycleLog &): Input parameter.
- Details: log Input parameter. Calls: THEMIS_INFO().

#### `void logEvictionEvent(const EvictionEventLog &log) override`
- Source: `include/access_model/access_model_logging.h`:163
- Brief: Log Eviction Event.
- Parameters:
  - `log` (const EvictionEventLog &): Input parameter.
- Details: log Input parameter. Calls: THEMIS_DEBUG(), tierLevelName(), count().

#### `void logPromotionDecision(const PromotionDecisionLog &log) override`
- Source: `include/access_model/access_model_logging.h`:164
- Brief: Log Promotion Decision.
- Parameters:
  - `log` (const PromotionDecisionLog &): Input parameter.
- Details: log Input parameter. Calls: THEMIS_DEBUG(), tierLevelName(), has_value(), value(), count().

#### `void logTierTransition(const TierTransitionLog &log) override`
- Source: `include/access_model/access_model_logging.h`:162
- Brief: Log Tier Transition.
- Parameters:
  - `log` (const TierTransitionLog &): Input parameter.
- Details: log Input parameter. Calls: THEMIS_INFO(), tierLevelName().

### themis::access_model::EvictionListener

#### `void onCacheEvicted(std::string_view key, TierLevel from_tier, std::size_t size_bytes, uint64_t access_count, std::chrono::seconds last_access_age_secs, std::string_view eviction_reason)=0`
- Source: `include/access_model/access_coordinator.h`:66
- Brief: On Cache Evicted.
- Parameters:
  - `key` (std::string_view): Input parameter.
  - `from_tier` (TierLevel): Input parameter.
  - `size_bytes` (std::size_t): Input parameter.
  - `access_count` (uint64_t): Input parameter.
  - `last_access_age_secs` (std::chrono::seconds): Input parameter.
  - `eviction_reason` (std::string_view): Input parameter.
- Details: key Input parameter. from_tier Input parameter. size_bytes Input parameter. access_count Input parameter. last_access_age_secs Input parameter. eviction_reason Input parameter.

#### `~EvictionListener()=default`
- Source: `include/access_model/access_coordinator.h`:55
- Brief: Eviction Listener.
- Parameters: none
- Return: Return value.
- Details: Return value.

### themis::access_model::LatencyHistogram

#### `LatencyHistogram(std::size_t num_buckets=1000, uint64_t max_latency_us=100000)`
- Source: `include/access_model/access_metrics.h`:32
- Brief: n/a
- Parameters:
  - `num_buckets` (std::size_t): n/a
  - `max_latency_us` (uint64_t): n/a

#### `uint64_t count() const noexcept`
- Source: `include/access_model/access_metrics.h`:47
- Brief: n/a
- Parameters: none

#### `std::string describe() const`
- Source: `include/access_model/access_metrics.h`:49
- Brief: n/a
- Parameters: none

#### `double mean() const`
- Source: `include/access_model/access_metrics.h`:43
- Brief: n/a
- Parameters: none

#### `uint64_t percentile(double p) const`
- Source: `include/access_model/access_metrics.h`:41
- Brief: n/a
- Parameters:
  - `p` (double): n/a

#### `void record(uint64_t latency_us)`
- Source: `include/access_model/access_metrics.h`:39
- Brief: Record.
- Parameters:
  - `latency_us` (uint64_t): Input parameter.
- Details: latency_us Input parameter. latency_us Input parameter. Calls: std::min(), std::max().

#### `double stdDev() const`
- Source: `include/access_model/access_metrics.h`:45
- Brief: n/a
- Parameters: none

### themis::access_model::LoggingTest

#### `void SetUp() override`
- Source: `tests/access_model/test_access_model_observability.cpp`:116
- Brief: n/a
- Parameters: none

### themis::access_model::MockAccessTier

#### `MOCK_METHOD(TierGetResult, get,(std::string_view, const TierAccessOptions &),(override))`
- Source: `tests/access_model/test_access_model_e2e.cpp`:42
- Brief: n/a
- Parameters:
  - `<unnamed>` (TierGetResult): n/a
  - `<unnamed>` (get): n/a
  - `<unnamed>` ((std::string_view, const TierAccessOptions &)): n/a
  - `<unnamed>` ((override)): n/a

#### `MOCK_METHOD(TierGetResult, get,(std::string_view, const TierAccessOptions &),(override))`
- Source: `tests/access_model/test_coordination_concurrency.cpp`:41
- Brief: n/a
- Parameters:
  - `<unnamed>` (TierGetResult): n/a
  - `<unnamed>` (get): n/a
  - `<unnamed>` ((std::string_view, const TierAccessOptions &)): n/a
  - `<unnamed>` ((override)): n/a

#### `MOCK_METHOD(TierLevel, getTierLevel,(),(const, override))`
- Source: `tests/access_model/test_access_model_e2e.cpp`:48
- Brief: n/a
- Parameters:
  - `<unnamed>` (TierLevel): n/a
  - `<unnamed>` (getTierLevel): n/a
  - `<unnamed>` (()): n/a
  - `<unnamed>` ((const, override)): n/a

#### `MOCK_METHOD(TierLevel, getTierLevel,(),(const, override))`
- Source: `tests/access_model/test_coordination_concurrency.cpp`:47
- Brief: n/a
- Parameters:
  - `<unnamed>` (TierLevel): n/a
  - `<unnamed>` (getTierLevel): n/a
  - `<unnamed>` (()): n/a
  - `<unnamed>` ((const, override)): n/a

#### `MOCK_METHOD(TierPutResult, put,(std::string_view, std::string_view, const TierAccessOptions &),(override))`
- Source: `tests/access_model/test_access_model_e2e.cpp`:44
- Brief: n/a
- Parameters:
  - `<unnamed>` (TierPutResult): n/a
  - `<unnamed>` (put): n/a
  - `<unnamed>` ((std::string_view, std::string_view, const TierAccessOptions &)): n/a
  - `<unnamed>` ((override)): n/a

#### `MOCK_METHOD(TierPutResult, put,(std::string_view, std::string_view, const TierAccessOptions &),(override))`
- Source: `tests/access_model/test_coordination_concurrency.cpp`:43
- Brief: n/a
- Parameters:
  - `<unnamed>` (TierPutResult): n/a
  - `<unnamed>` (put): n/a
  - `<unnamed>` ((std::string_view, std::string_view, const TierAccessOptions &)): n/a
  - `<unnamed>` ((override)): n/a

#### `MOCK_METHOD(bool, hasKey,(std::string_view),(const, override))`
- Source: `tests/access_model/test_access_model_e2e.cpp`:50
- Brief: n/a
- Parameters:
  - `<unnamed>` (bool): n/a
  - `<unnamed>` (hasKey): n/a
  - `<unnamed>` ((std::string_view)): n/a
  - `<unnamed>` ((const, override)): n/a

#### `MOCK_METHOD(bool, hasKey,(std::string_view),(const, override))`
- Source: `tests/access_model/test_coordination_concurrency.cpp`:49
- Brief: n/a
- Parameters:
  - `<unnamed>` (bool): n/a
  - `<unnamed>` (hasKey): n/a
  - `<unnamed>` ((std::string_view)): n/a
  - `<unnamed>` ((const, override)): n/a

#### `MOCK_METHOD(bool, initialize,(),(override))`
- Source: `tests/access_model/test_access_model_e2e.cpp`:62
- Brief: n/a
- Parameters:
  - `<unnamed>` (bool): n/a
  - `<unnamed>` (initialize): n/a
  - `<unnamed>` (()): n/a
  - `<unnamed>` ((override)): n/a

#### `MOCK_METHOD(bool, initialize,(),(override))`
- Source: `tests/access_model/test_coordination_concurrency.cpp`:61
- Brief: n/a
- Parameters:
  - `<unnamed>` (bool): n/a
  - `<unnamed>` (initialize): n/a
  - `<unnamed>` (()): n/a
  - `<unnamed>` ((override)): n/a

#### `MOCK_METHOD(bool, invalidate,(std::string_view),(override))`
- Source: `tests/access_model/test_access_model_e2e.cpp`:47
- Brief: n/a
- Parameters:
  - `<unnamed>` (bool): n/a
  - `<unnamed>` (invalidate): n/a
  - `<unnamed>` ((std::string_view)): n/a
  - `<unnamed>` ((override)): n/a

#### `MOCK_METHOD(bool, invalidate,(std::string_view),(override))`
- Source: `tests/access_model/test_coordination_concurrency.cpp`:46
- Brief: n/a
- Parameters:
  - `<unnamed>` (bool): n/a
  - `<unnamed>` (invalidate): n/a
  - `<unnamed>` ((std::string_view)): n/a
  - `<unnamed>` ((override)): n/a

#### `MOCK_METHOD(bool, isHealthy,(),(const, override))`
- Source: `tests/access_model/test_access_model_e2e.cpp`:64
- Brief: n/a
- Parameters:
  - `<unnamed>` (bool): n/a
  - `<unnamed>` (isHealthy): n/a
  - `<unnamed>` (()): n/a
  - `<unnamed>` ((const, override)): n/a

#### `MOCK_METHOD(bool, isHealthy,(),(const, override))`
- Source: `tests/access_model/test_coordination_concurrency.cpp`:63
- Brief: n/a
- Parameters:
  - `<unnamed>` (bool): n/a
  - `<unnamed>` (isHealthy): n/a
  - `<unnamed>` (()): n/a
  - `<unnamed>` ((const, override)): n/a

#### `MOCK_METHOD(double, getHitRate,(),(const, override))`
- Source: `tests/access_model/test_access_model_e2e.cpp`:54
- Brief: n/a
- Parameters:
  - `<unnamed>` (double): n/a
  - `<unnamed>` (getHitRate): n/a
  - `<unnamed>` (()): n/a
  - `<unnamed>` ((const, override)): n/a

#### `MOCK_METHOD(double, getHitRate,(),(const, override))`
- Source: `tests/access_model/test_coordination_concurrency.cpp`:53
- Brief: n/a
- Parameters:
  - `<unnamed>` (double): n/a
  - `<unnamed>` (getHitRate): n/a
  - `<unnamed>` (()): n/a
  - `<unnamed>` ((const, override)): n/a

#### `MOCK_METHOD(std::chrono::microseconds, getAverageGetLatency,(),(const, override))`
- Source: `tests/access_model/test_access_model_e2e.cpp`:55
- Brief: n/a
- Parameters:
  - `<unnamed>` (std::chrono::microseconds): n/a
  - `<unnamed>` (getAverageGetLatency): n/a
  - `<unnamed>` (()): n/a
  - `<unnamed>` ((const, override)): n/a

#### `MOCK_METHOD(std::chrono::microseconds, getAverageGetLatency,(),(const, override))`
- Source: `tests/access_model/test_coordination_concurrency.cpp`:54
- Brief: n/a
- Parameters:
  - `<unnamed>` (std::chrono::microseconds): n/a
  - `<unnamed>` (getAverageGetLatency): n/a
  - `<unnamed>` (()): n/a
  - `<unnamed>` ((const, override)): n/a

#### `MOCK_METHOD(std::chrono::microseconds, getAveragePutLatency,(),(const, override))`
- Source: `tests/access_model/test_access_model_e2e.cpp`:57
- Brief: n/a
- Parameters:
  - `<unnamed>` (std::chrono::microseconds): n/a
  - `<unnamed>` (getAveragePutLatency): n/a
  - `<unnamed>` (()): n/a
  - `<unnamed>` ((const, override)): n/a

#### `MOCK_METHOD(std::chrono::microseconds, getAveragePutLatency,(),(const, override))`
- Source: `tests/access_model/test_coordination_concurrency.cpp`:56
- Brief: n/a
- Parameters:
  - `<unnamed>` (std::chrono::microseconds): n/a
  - `<unnamed>` (getAveragePutLatency): n/a
  - `<unnamed>` (()): n/a
  - `<unnamed>` ((const, override)): n/a

#### `MOCK_METHOD(std::chrono::seconds, getKeyAge,(std::string_view),(const, override))`
- Source: `tests/access_model/test_access_model_e2e.cpp`:60
- Brief: n/a
- Parameters:
  - `<unnamed>` (std::chrono::seconds): n/a
  - `<unnamed>` (getKeyAge): n/a
  - `<unnamed>` ((std::string_view)): n/a
  - `<unnamed>` ((const, override)): n/a

#### `MOCK_METHOD(std::chrono::seconds, getKeyAge,(std::string_view),(const, override))`
- Source: `tests/access_model/test_coordination_concurrency.cpp`:59
- Brief: n/a
- Parameters:
  - `<unnamed>` (std::chrono::seconds): n/a
  - `<unnamed>` (getKeyAge): n/a
  - `<unnamed>` ((std::string_view)): n/a
  - `<unnamed>` ((const, override)): n/a

#### `MOCK_METHOD(std::size_t, getCurrentSizeBytes,(),(const, override))`
- Source: `tests/access_model/test_access_model_e2e.cpp`:51
- Brief: n/a
- Parameters:
  - `<unnamed>` (std::size_t): n/a
  - `<unnamed>` (getCurrentSizeBytes): n/a
  - `<unnamed>` (()): n/a
  - `<unnamed>` ((const, override)): n/a

#### `MOCK_METHOD(std::size_t, getCurrentSizeBytes,(),(const, override))`
- Source: `tests/access_model/test_coordination_concurrency.cpp`:50
- Brief: n/a
- Parameters:
  - `<unnamed>` (std::size_t): n/a
  - `<unnamed>` (getCurrentSizeBytes): n/a
  - `<unnamed>` (()): n/a
  - `<unnamed>` ((const, override)): n/a

#### `MOCK_METHOD(std::size_t, getEntryCount,(),(const, override))`
- Source: `tests/access_model/test_access_model_e2e.cpp`:53
- Brief: n/a
- Parameters:
  - `<unnamed>` (std::size_t): n/a
  - `<unnamed>` (getEntryCount): n/a
  - `<unnamed>` (()): n/a
  - `<unnamed>` ((const, override)): n/a

#### `MOCK_METHOD(std::size_t, getEntryCount,(),(const, override))`
- Source: `tests/access_model/test_coordination_concurrency.cpp`:52
- Brief: n/a
- Parameters:
  - `<unnamed>` (std::size_t): n/a
  - `<unnamed>` (getEntryCount): n/a
  - `<unnamed>` (()): n/a
  - `<unnamed>` ((const, override)): n/a

#### `MOCK_METHOD(std::size_t, getMaxCapacityBytes,(),(const, override))`
- Source: `tests/access_model/test_access_model_e2e.cpp`:52
- Brief: n/a
- Parameters:
  - `<unnamed>` (std::size_t): n/a
  - `<unnamed>` (getMaxCapacityBytes): n/a
  - `<unnamed>` (()): n/a
  - `<unnamed>` ((const, override)): n/a

#### `MOCK_METHOD(std::size_t, getMaxCapacityBytes,(),(const, override))`
- Source: `tests/access_model/test_coordination_concurrency.cpp`:51
- Brief: n/a
- Parameters:
  - `<unnamed>` (std::size_t): n/a
  - `<unnamed>` (getMaxCapacityBytes): n/a
  - `<unnamed>` (()): n/a
  - `<unnamed>` ((const, override)): n/a

#### `MOCK_METHOD(std::string, getTierName,(),(const, override))`
- Source: `tests/access_model/test_access_model_e2e.cpp`:49
- Brief: n/a
- Parameters:
  - `<unnamed>` (std::string): n/a
  - `<unnamed>` (getTierName): n/a
  - `<unnamed>` (()): n/a
  - `<unnamed>` ((const, override)): n/a

#### `MOCK_METHOD(std::string, getTierName,(),(const, override))`
- Source: `tests/access_model/test_coordination_concurrency.cpp`:48
- Brief: n/a
- Parameters:
  - `<unnamed>` (std::string): n/a
  - `<unnamed>` (getTierName): n/a
  - `<unnamed>` (()): n/a
  - `<unnamed>` ((const, override)): n/a

#### `MOCK_METHOD(uint64_t, getAccessCount,(std::string_view),(const, override))`
- Source: `tests/access_model/test_access_model_e2e.cpp`:59
- Brief: n/a
- Parameters:
  - `<unnamed>` (uint64_t): n/a
  - `<unnamed>` (getAccessCount): n/a
  - `<unnamed>` ((std::string_view)): n/a
  - `<unnamed>` ((const, override)): n/a

#### `MOCK_METHOD(uint64_t, getAccessCount,(std::string_view),(const, override))`
- Source: `tests/access_model/test_coordination_concurrency.cpp`:58
- Brief: n/a
- Parameters:
  - `<unnamed>` (uint64_t): n/a
  - `<unnamed>` (getAccessCount): n/a
  - `<unnamed>` ((std::string_view)): n/a
  - `<unnamed>` ((const, override)): n/a

#### `MOCK_METHOD(void, shutdown,(),(override))`
- Source: `tests/access_model/test_access_model_e2e.cpp`:63
- Brief: n/a
- Parameters:
  - `<unnamed>` (void): n/a
  - `<unnamed>` (shutdown): n/a
  - `<unnamed>` (()): n/a
  - `<unnamed>` ((override)): n/a

#### `MOCK_METHOD(void, shutdown,(),(override))`
- Source: `tests/access_model/test_coordination_concurrency.cpp`:62
- Brief: n/a
- Parameters:
  - `<unnamed>` (void): n/a
  - `<unnamed>` (shutdown): n/a
  - `<unnamed>` (()): n/a
  - `<unnamed>` ((override)): n/a

### themis::access_model::ObservabilityIntegrationTest

#### `void SetUp() override`
- Source: `tests/access_model/test_access_model_observability.cpp`:198
- Brief: n/a
- Parameters: none

### themis::access_model::PromotionListener

#### `void onStorageAccess(std::string_view key, TierLevel from_tier, uint64_t access_count, std::chrono::seconds access_window)=0`
- Source: `include/access_model/access_coordinator.h`:86
- Brief: On Storage Access.
- Parameters:
  - `key` (std::string_view): Input parameter.
  - `from_tier` (TierLevel): Input parameter.
  - `access_count` (uint64_t): Input parameter.
  - `access_window` (std::chrono::seconds): Input parameter.
- Details: key Input parameter. from_tier Input parameter. access_count Input parameter. access_window Input parameter.

#### `~PromotionListener()=default`
- Source: `include/access_model/access_coordinator.h`:77
- Brief: Promotion Listener.
- Parameters: none
- Return: Return value.
- Details: Return value.

### themis::access_model::StorageTier

#### `void notifyHotAccess(std::string_view key, uint64_t access_count, std::chrono::seconds access_window)=0`
- Source: `include/access_model/access_tier_interface.h`:338
- Brief: Notify Hot Access.
- Parameters:
  - `key` (std::string_view): Input parameter.
  - `access_count` (uint64_t): Input parameter.
  - `access_window` (std::chrono::seconds): Input parameter.
- Details: key Input parameter. access_count Input parameter. access_window Input parameter.

### themis::access_model::TestAccessTier

#### `TestAccessTier(TierLevel level, std::string name)`
- Source: `tests/access_model/test_access_coordinator.cpp`:33
- Brief: n/a
- Parameters:
  - `level` (TierLevel): n/a
  - `name` (std::string): n/a

#### `TestAccessTier(TierLevel level, std::string name)`
- Source: `tests/access_model/test_access_coordinator_focused.cpp`:39
- Brief: n/a
- Parameters:
  - `level` (TierLevel): n/a
  - `name` (std::string): n/a

#### `TierGetResult get(std::string_view, const TierAccessOptions &) override`
- Source: `tests/access_model/test_access_coordinator.cpp`:36
- Brief: Get.
- Parameters:
  - `key` (std::string_view): Input parameter.
  - `options` (const TierAccessOptions &): Input parameter.
- Return: Return value.
- Details: key Input parameter. options Input parameter. Return value.

#### `TierGetResult get(std::string_view, const TierAccessOptions &) override`
- Source: `tests/access_model/test_access_coordinator_focused.cpp`:42
- Brief: Get.
- Parameters:
  - `key` (std::string_view): Input parameter.
  - `options` (const TierAccessOptions &): Input parameter.
- Return: Return value.
- Details: key Input parameter. options Input parameter. Return value.

#### `uint64_t getAccessCount(std::string_view) const override`
- Source: `tests/access_model/test_access_coordinator.cpp`:68
- Brief: Get Access Count.
- Parameters:
  - `key` (std::string_view): Input parameter.
- Return: Return value.
- Details: key Input parameter. Return value.

#### `uint64_t getAccessCount(std::string_view) const override`
- Source: `tests/access_model/test_access_coordinator_focused.cpp`:74
- Brief: Get Access Count.
- Parameters:
  - `key` (std::string_view): Input parameter.
- Return: Return value.
- Details: key Input parameter. Return value.

#### `std::chrono::microseconds getAverageGetLatency() const override`
- Source: `tests/access_model/test_access_coordinator.cpp`:62
- Brief: Get Average Get Latency.
- Parameters: none
- Return: Return value.
- Details: Return value.

#### `std::chrono::microseconds getAverageGetLatency() const override`
- Source: `tests/access_model/test_access_coordinator_focused.cpp`:68
- Brief: Get Average Get Latency.
- Parameters: none
- Return: Return value.
- Details: Return value.

#### `std::chrono::microseconds getAveragePutLatency() const override`
- Source: `tests/access_model/test_access_coordinator.cpp`:65
- Brief: Get Average Put Latency.
- Parameters: none
- Return: Return value.
- Details: Return value.

#### `std::chrono::microseconds getAveragePutLatency() const override`
- Source: `tests/access_model/test_access_coordinator_focused.cpp`:71
- Brief: Get Average Put Latency.
- Parameters: none
- Return: Return value.
- Details: Return value.

#### `std::size_t getCurrentSizeBytes() const override`
- Source: `tests/access_model/test_access_coordinator.cpp`:58
- Brief: Get Current Size Bytes.
- Parameters: none
- Return: Return value.
- Details: Return value.

#### `std::size_t getCurrentSizeBytes() const override`
- Source: `tests/access_model/test_access_coordinator_focused.cpp`:64
- Brief: Get Current Size Bytes.
- Parameters: none
- Return: Return value.
- Details: Return value.

#### `std::size_t getEntryCount() const override`
- Source: `tests/access_model/test_access_coordinator.cpp`:60
- Brief: Get Entry Count.
- Parameters: none
- Return: Return value.
- Details: Return value.

#### `std::size_t getEntryCount() const override`
- Source: `tests/access_model/test_access_coordinator_focused.cpp`:66
- Brief: Get Entry Count.
- Parameters: none
- Return: Return value.
- Details: Return value.

#### `double getHitRate() const override`
- Source: `tests/access_model/test_access_coordinator.cpp`:61
- Brief: Get Hit Rate.
- Parameters: none
- Return: Return value.
- Details: Return value.

#### `double getHitRate() const override`
- Source: `tests/access_model/test_access_coordinator_focused.cpp`:67
- Brief: Get Hit Rate.
- Parameters: none
- Return: Return value.
- Details: Return value.

#### `std::chrono::seconds getKeyAge(std::string_view) const override`
- Source: `tests/access_model/test_access_coordinator.cpp`:69
- Brief: Get Key Age.
- Parameters:
  - `key` (std::string_view): Input parameter.
- Return: Return value.
- Details: key Input parameter. Return value.

#### `std::chrono::seconds getKeyAge(std::string_view) const override`
- Source: `tests/access_model/test_access_coordinator_focused.cpp`:75
- Brief: Get Key Age.
- Parameters:
  - `key` (std::string_view): Input parameter.
- Return: Return value.
- Details: key Input parameter. Return value.

#### `std::size_t getMaxCapacityBytes() const override`
- Source: `tests/access_model/test_access_coordinator.cpp`:59
- Brief: Get Max Capacity Bytes.
- Parameters: none
- Return: Return value.
- Details: Return value.

#### `std::size_t getMaxCapacityBytes() const override`
- Source: `tests/access_model/test_access_coordinator_focused.cpp`:65
- Brief: Get Max Capacity Bytes.
- Parameters: none
- Return: Return value.
- Details: Return value.

#### `TierLevel getTierLevel() const override`
- Source: `tests/access_model/test_access_coordinator.cpp`:55
- Brief: Get Tier Level.
- Parameters: none
- Return: Return value.
- Details: Return value.

#### `TierLevel getTierLevel() const override`
- Source: `tests/access_model/test_access_coordinator_focused.cpp`:61
- Brief: Get Tier Level.
- Parameters: none
- Return: Return value.
- Details: Return value.

#### `std::string getTierName() const override`
- Source: `tests/access_model/test_access_coordinator.cpp`:56
- Brief: Get Tier Name.
- Parameters: none
- Return: Return value.
- Details: Return value.

#### `std::string getTierName() const override`
- Source: `tests/access_model/test_access_coordinator_focused.cpp`:62
- Brief: Get Tier Name.
- Parameters: none
- Return: Return value.
- Details: Return value.

#### `bool hasKey(std::string_view) const override`
- Source: `tests/access_model/test_access_coordinator.cpp`:57
- Brief: Has Key.
- Parameters:
  - `key` (std::string_view): Input parameter.
- Return: True when the operation succeeds.
- Details: key Input parameter. True when the operation succeeds.

#### `bool hasKey(std::string_view) const override`
- Source: `tests/access_model/test_access_coordinator_focused.cpp`:63
- Brief: Has Key.
- Parameters:
  - `key` (std::string_view): Input parameter.
- Return: True when the operation succeeds.
- Details: key Input parameter. True when the operation succeeds.

#### `bool initialize() override`
- Source: `tests/access_model/test_access_coordinator.cpp`:72
- Brief: Initialize.
- Parameters: none
- Return: True when the operation succeeds.
- Details: True when the operation succeeds.

#### `bool initialize() override`
- Source: `tests/access_model/test_access_coordinator_focused.cpp`:78
- Brief: Initialize.
- Parameters: none
- Return: True when the operation succeeds.
- Details: True when the operation succeeds.

#### `bool invalidate(std::string_view) override`
- Source: `tests/access_model/test_access_coordinator.cpp`:54
- Brief: Invalidate.
- Parameters:
  - `key` (std::string_view): Input parameter.
- Return: True when the operation succeeds.
- Details: key Input parameter. True when the operation succeeds.

#### `bool invalidate(std::string_view) override`
- Source: `tests/access_model/test_access_coordinator_focused.cpp`:60
- Brief: Invalidate.
- Parameters:
  - `key` (std::string_view): Input parameter.
- Return: True when the operation succeeds.
- Details: key Input parameter. True when the operation succeeds.

#### `bool isHealthy() const override`
- Source: `tests/access_model/test_access_coordinator.cpp`:74
- Brief: Is Healthy.
- Parameters: none
- Return: True when the operation succeeds.
- Details: True when the operation succeeds.

#### `bool isHealthy() const override`
- Source: `tests/access_model/test_access_coordinator_focused.cpp`:80
- Brief: Is Healthy.
- Parameters: none
- Return: True when the operation succeeds.
- Details: True when the operation succeeds.

#### `TierPutResult put(std::string_view, std::string_view, const TierAccessOptions &) override`
- Source: `tests/access_model/test_access_coordinator.cpp`:45
- Brief: Put.
- Parameters:
  - `key` (std::string_view): Input parameter.
  - `value` (std::string_view): Input parameter.
  - `options` (const TierAccessOptions &): Input parameter.
- Return: Return value.
- Details: key Input parameter. value Input parameter. options Input parameter. Return value.

#### `TierPutResult put(std::string_view, std::string_view, const TierAccessOptions &) override`
- Source: `tests/access_model/test_access_coordinator_focused.cpp`:51
- Brief: Put.
- Parameters:
  - `key` (std::string_view): Input parameter.
  - `value` (std::string_view): Input parameter.
  - `options` (const TierAccessOptions &): Input parameter.
- Return: Return value.
- Details: key Input parameter. value Input parameter. options Input parameter. Return value.

#### `void shutdown() override`
- Source: `tests/access_model/test_access_coordinator.cpp`:73
- Brief: Shutdown.
- Parameters: none

#### `void shutdown() override`
- Source: `tests/access_model/test_access_coordinator_focused.cpp`:79
- Brief: Shutdown.
- Parameters: none

### themis::access_model::TraceContext

#### `TraceContext()`
- Source: `include/access_model/access_model_trace.h`:54
- Brief: n/a
- Parameters: none

#### `TraceContext(const CorrelationID &id)`
- Source: `include/access_model/access_model_trace.h`:64
- Brief: Trace Context.
- Parameters:
  - `id` (const CorrelationID &): Input parameter.
- Return: Return value.
- Details: id Input parameter. Return value.

#### `TraceContext(const CorrelationID &id, const std::string &parent_id)`
- Source: `include/access_model/access_model_trace.h`:69
- Brief: n/a
- Parameters:
  - `id` (const CorrelationID &): n/a
  - `parent_id` (const std::string &): n/a

### themis::access_model::TraceContextManager

#### `void clearContext()`
- Source: `include/access_model/access_model_trace.h`:99
- Brief: Clear Context.
- Parameters: none
- Details: Implements clearContext without additional internal calls.

#### `CorrelationID currentCorrelationID()`
- Source: `include/access_model/access_model_trace.h`:105
- Brief: Current Correlation ID.
- Parameters: none
- Return: Return value.
- Details: Return value. Return value. Calls: getContext().

#### `CorrelationID generateCorrelationID(const std::string &prefix="op")`
- Source: `include/access_model/access_model_trace.h`:81
- Brief: Generate Correlation ID.
- Parameters:
  - `prefix` (const std::string &): Input parameter.
- Return: Return value.
- Details: prefix Input parameter. Return value. Calls: gen(), rd(), std::chrono::system_clock::now(), time_since_epoch(), count(), dis(), str().

#### `TraceContext getContext()`
- Source: `include/access_model/access_model_trace.h`:94
- Brief: Get Context.
- Parameters: none
- Return: Return value.
- Details: Return value. Return value. Implements getContext without additional internal calls.

#### `void setContext(const TraceContext &ctx)`
- Source: `include/access_model/access_model_trace.h`:88
- Brief: Set Context.
- Parameters:
  - `ctx` (const TraceContext &): Input parameter.
- Details: ctx Input parameter. ctx Input parameter. Implements setContext without additional internal calls.

### themis::access_model::TraceContextManager::ScopedContext

#### `ScopedContext(ScopedContext &&other) noexcept`
- Source: `include/access_model/access_model_trace.h`:127
- Brief: n/a
- Parameters:
  - `other` (ScopedContext &&): n/a

#### `ScopedContext(const ScopedContext &)=delete`
- Source: `include/access_model/access_model_trace.h`:123
- Brief: n/a
- Parameters:
  - `<unnamed>` (const ScopedContext &): n/a

#### `ScopedContext(const TraceContext &ctx)`
- Source: `include/access_model/access_model_trace.h`:118
- Brief: Scoped Context.
- Parameters:
  - `ctx` (const TraceContext &): Input parameter.
- Return: Return value.
- Details: ctx Input parameter. Return value.

#### `ScopedContext & operator=(ScopedContext &&other) noexcept`
- Source: `include/access_model/access_model_trace.h`:133
- Brief: n/a
- Parameters:
  - `other` (ScopedContext &&): n/a

#### `ScopedContext & operator=(const ScopedContext &)=delete`
- Source: `include/access_model/access_model_trace.h`:124
- Brief: n/a
- Parameters:
  - `<unnamed>` (const ScopedContext &): n/a

#### `~ScopedContext()`
- Source: `include/access_model/access_model_trace.h`:120
- Brief: n/a
- Parameters: none

### themis::access_model::TraceContextTest

#### `void SetUp() override`
- Source: `tests/access_model/test_access_model_observability.cpp`:31
- Brief: n/a
- Parameters: none

### themis::access_model::gates

#### `void reportViolation(const char *gate_name, double measured, double target, const char *unit, bool is_minimum)`
- Source: `benchmarks/access_model/bench_access_coordinator_gates.cpp`:57
- Brief: n/a
- Parameters:
  - `gate_name` (const char *): n/a
  - `measured` (double): n/a
  - `target` (double): n/a
  - `unit` (const char *): n/a
  - `is_minimum` (bool): n/a

### themis::access_model::test

#### `TEST_F(CacheStorageIntegrationTest, CAI01_L1EvictionNotification)`
- Source: `tests/access_model/test_cache_storage_integration.cpp`:133
- Brief: n/a
- Parameters:
  - `<unnamed>` (CacheStorageIntegrationTest): n/a
  - `<unnamed>` (CAI01_L1EvictionNotification): n/a

#### `TEST_F(CacheStorageIntegrationTest, CAI02_L2EvictionNotification)`
- Source: `tests/access_model/test_cache_storage_integration.cpp`:163
- Brief: n/a
- Parameters:
  - `<unnamed>` (CacheStorageIntegrationTest): n/a
  - `<unnamed>` (CAI02_L2EvictionNotification): n/a

#### `TEST_F(CacheStorageIntegrationTest, CAI03_HighAccessPromotion)`
- Source: `tests/access_model/test_cache_storage_integration.cpp`:183
- Brief: n/a
- Parameters:
  - `<unnamed>` (CacheStorageIntegrationTest): n/a
  - `<unnamed>` (CAI03_HighAccessPromotion): n/a

#### `TEST_F(CacheStorageIntegrationTest, CAI04_LowAccessDemotion)`
- Source: `tests/access_model/test_cache_storage_integration.cpp`:224
- Brief: n/a
- Parameters:
  - `<unnamed>` (CacheStorageIntegrationTest): n/a
  - `<unnamed>` (CAI04_LowAccessDemotion): n/a

#### `TEST_F(CacheStorageIntegrationTest, CAI05_SharedAgePolicy)`
- Source: `tests/access_model/test_cache_storage_integration.cpp`:258
- Brief: n/a
- Parameters:
  - `<unnamed>` (CacheStorageIntegrationTest): n/a
  - `<unnamed>` (CAI05_SharedAgePolicy): n/a

#### `TEST_F(CacheStorageIntegrationTest, CAI06_ConcurrentTransitions)`
- Source: `tests/access_model/test_cache_storage_integration.cpp`:277
- Brief: n/a
- Parameters:
  - `<unnamed>` (CacheStorageIntegrationTest): n/a
  - `<unnamed>` (CAI06_ConcurrentTransitions): n/a

#### `TEST_F(CacheStorageIntegrationTest, CAI07_StorageWarmTierAccessEmitsPromotion)`
- Source: `tests/access_model/test_cache_storage_integration.cpp`:315
- Brief: n/a
- Parameters:
  - `<unnamed>` (CacheStorageIntegrationTest): n/a
  - `<unnamed>` (CAI07_StorageWarmTierAccessEmitsPromotion): n/a

#### `TEST_F(CacheStorageIntegrationTest, CAI08_ColdToWarmPromotion)`
- Source: `tests/access_model/test_cache_storage_integration.cpp`:342
- Brief: n/a
- Parameters:
  - `<unnamed>` (CacheStorageIntegrationTest): n/a
  - `<unnamed>` (CAI08_ColdToWarmPromotion): n/a

#### `TEST_F(CacheStorageIntegrationTest, CAI09_ListenerStability)`
- Source: `tests/access_model/test_cache_storage_integration.cpp`:350
- Brief: n/a
- Parameters:
  - `<unnamed>` (CacheStorageIntegrationTest): n/a
  - `<unnamed>` (CAI09_ListenerStability): n/a

#### `TEST_F(CacheStorageIntegrationTest, CAI10_NullListenerSafety)`
- Source: `tests/access_model/test_cache_storage_integration.cpp`:363
- Brief: n/a
- Parameters:
  - `<unnamed>` (CacheStorageIntegrationTest): n/a
  - `<unnamed>` (CAI10_NullListenerSafety): n/a

### themis::access_model::test::CacheStorageIntegrationTest

#### `void SetUp() override`
- Source: `tests/access_model/test_cache_storage_integration.cpp`:102
- Brief: n/a
- Parameters: none

#### `void TearDown() override`
- Source: `tests/access_model/test_cache_storage_integration.cpp`:117
- Brief: n/a
- Parameters: none

### themis::access_model::test::MockAccessTier

#### `MOCK_METHOD(TierGetResult, get,(std::string_view key, const TierAccessOptions &options),(override))`
- Source: `tests/access_model/test_cache_storage_integration.cpp`:74
- Brief: n/a
- Parameters:
  - `<unnamed>` (TierGetResult): n/a
  - `<unnamed>` (get): n/a
  - `<unnamed>` ((std::string_view key, const TierAccessOptions &options)): n/a
  - `<unnamed>` ((override)): n/a

#### `MOCK_METHOD(TierLevel, getTierLevel,(),(const, override))`
- Source: `tests/access_model/test_cache_storage_integration.cpp`:80
- Brief: n/a
- Parameters:
  - `<unnamed>` (TierLevel): n/a
  - `<unnamed>` (getTierLevel): n/a
  - `<unnamed>` (()): n/a
  - `<unnamed>` ((const, override)): n/a

#### `MOCK_METHOD(TierPutResult, put,(std::string_view key, std::string_view value, const TierAccessOptions &options),(override))`
- Source: `tests/access_model/test_cache_storage_integration.cpp`:76
- Brief: n/a
- Parameters:
  - `<unnamed>` (TierPutResult): n/a
  - `<unnamed>` (put): n/a
  - `<unnamed>` ((std::string_view key, std::string_view value, const TierAccessOptions &options)): n/a
  - `<unnamed>` ((override)): n/a

#### `MOCK_METHOD(bool, hasKey,(std::string_view key),(const, override))`
- Source: `tests/access_model/test_cache_storage_integration.cpp`:82
- Brief: n/a
- Parameters:
  - `<unnamed>` (bool): n/a
  - `<unnamed>` (hasKey): n/a
  - `<unnamed>` ((std::string_view key)): n/a
  - `<unnamed>` ((const, override)): n/a

#### `MOCK_METHOD(bool, initialize,(),(override))`
- Source: `tests/access_model/test_cache_storage_integration.cpp`:91
- Brief: n/a
- Parameters:
  - `<unnamed>` (bool): n/a
  - `<unnamed>` (initialize): n/a
  - `<unnamed>` (()): n/a
  - `<unnamed>` ((override)): n/a

#### `MOCK_METHOD(bool, invalidate,(std::string_view key),(override))`
- Source: `tests/access_model/test_cache_storage_integration.cpp`:79
- Brief: n/a
- Parameters:
  - `<unnamed>` (bool): n/a
  - `<unnamed>` (invalidate): n/a
  - `<unnamed>` ((std::string_view key)): n/a
  - `<unnamed>` ((override)): n/a

#### `MOCK_METHOD(bool, isHealthy,(),(const, override))`
- Source: `tests/access_model/test_cache_storage_integration.cpp`:93
- Brief: n/a
- Parameters:
  - `<unnamed>` (bool): n/a
  - `<unnamed>` (isHealthy): n/a
  - `<unnamed>` (()): n/a
  - `<unnamed>` ((const, override)): n/a

#### `MOCK_METHOD(double, getHitRate,(),(const, override))`
- Source: `tests/access_model/test_cache_storage_integration.cpp`:86
- Brief: n/a
- Parameters:
  - `<unnamed>` (double): n/a
  - `<unnamed>` (getHitRate): n/a
  - `<unnamed>` (()): n/a
  - `<unnamed>` ((const, override)): n/a

#### `MOCK_METHOD(std::chrono::microseconds, getAverageGetLatency,(),(const, override))`
- Source: `tests/access_model/test_cache_storage_integration.cpp`:87
- Brief: n/a
- Parameters:
  - `<unnamed>` (std::chrono::microseconds): n/a
  - `<unnamed>` (getAverageGetLatency): n/a
  - `<unnamed>` (()): n/a
  - `<unnamed>` ((const, override)): n/a

#### `MOCK_METHOD(std::chrono::microseconds, getAveragePutLatency,(),(const, override))`
- Source: `tests/access_model/test_cache_storage_integration.cpp`:88
- Brief: n/a
- Parameters:
  - `<unnamed>` (std::chrono::microseconds): n/a
  - `<unnamed>` (getAveragePutLatency): n/a
  - `<unnamed>` (()): n/a
  - `<unnamed>` ((const, override)): n/a

#### `MOCK_METHOD(std::chrono::seconds, getKeyAge,(std::string_view key),(const, override))`
- Source: `tests/access_model/test_cache_storage_integration.cpp`:90
- Brief: n/a
- Parameters:
  - `<unnamed>` (std::chrono::seconds): n/a
  - `<unnamed>` (getKeyAge): n/a
  - `<unnamed>` ((std::string_view key)): n/a
  - `<unnamed>` ((const, override)): n/a

#### `MOCK_METHOD(std::size_t, getCurrentSizeBytes,(),(const, override))`
- Source: `tests/access_model/test_cache_storage_integration.cpp`:83
- Brief: n/a
- Parameters:
  - `<unnamed>` (std::size_t): n/a
  - `<unnamed>` (getCurrentSizeBytes): n/a
  - `<unnamed>` (()): n/a
  - `<unnamed>` ((const, override)): n/a

#### `MOCK_METHOD(std::size_t, getEntryCount,(),(const, override))`
- Source: `tests/access_model/test_cache_storage_integration.cpp`:85
- Brief: n/a
- Parameters:
  - `<unnamed>` (std::size_t): n/a
  - `<unnamed>` (getEntryCount): n/a
  - `<unnamed>` (()): n/a
  - `<unnamed>` ((const, override)): n/a

#### `MOCK_METHOD(std::size_t, getMaxCapacityBytes,(),(const, override))`
- Source: `tests/access_model/test_cache_storage_integration.cpp`:84
- Brief: n/a
- Parameters:
  - `<unnamed>` (std::size_t): n/a
  - `<unnamed>` (getMaxCapacityBytes): n/a
  - `<unnamed>` (()): n/a
  - `<unnamed>` ((const, override)): n/a

#### `MOCK_METHOD(std::string, getTierName,(),(const, override))`
- Source: `tests/access_model/test_cache_storage_integration.cpp`:81
- Brief: n/a
- Parameters:
  - `<unnamed>` (std::string): n/a
  - `<unnamed>` (getTierName): n/a
  - `<unnamed>` (()): n/a
  - `<unnamed>` ((const, override)): n/a

#### `MOCK_METHOD(uint64_t, getAccessCount,(std::string_view key),(const, override))`
- Source: `tests/access_model/test_cache_storage_integration.cpp`:89
- Brief: n/a
- Parameters:
  - `<unnamed>` (uint64_t): n/a
  - `<unnamed>` (getAccessCount): n/a
  - `<unnamed>` ((std::string_view key)): n/a
  - `<unnamed>` ((const, override)): n/a

#### `MOCK_METHOD(void, shutdown,(),(override))`
- Source: `tests/access_model/test_cache_storage_integration.cpp`:92
- Brief: n/a
- Parameters:
  - `<unnamed>` (void): n/a
  - `<unnamed>` (shutdown): n/a
  - `<unnamed>` (()): n/a
  - `<unnamed>` ((override)): n/a

### themis::access_model::test::MockEvictionListener

#### `MOCK_METHOD(void, onCacheEvicted,(std::string_view key, TierLevel from_tier, std::size_t size_bytes, uint64_t access_count, std::chrono::seconds last_access_age_secs, std::string_view eviction_reason),(override))`
- Source: `tests/access_model/test_cache_storage_integration.cpp`:49
- Brief: n/a
- Parameters:
  - `<unnamed>` (void): n/a
  - `<unnamed>` (onCacheEvicted): n/a
  - `<unnamed>` ((std::string_view key, TierLevel from_tier, std::size_t size_bytes, uint64_t access_count, std::chrono::seconds last_access_age_secs, std::string_view eviction_reason)): n/a
  - `<unnamed>` ((override)): n/a

### themis::access_model::test::MockPromotionListener

#### `MOCK_METHOD(void, onStorageAccess,(std::string_view key, TierLevel from_tier, uint64_t access_count, std::chrono::seconds access_window),(override))`
- Source: `tests/access_model/test_cache_storage_integration.cpp`:62
- Brief: n/a
- Parameters:
  - `<unnamed>` (void): n/a
  - `<unnamed>` (onStorageAccess): n/a
  - `<unnamed>` ((std::string_view key, TierLevel from_tier, uint64_t access_count, std::chrono::seconds access_window)): n/a
  - `<unnamed>` ((override)): n/a

