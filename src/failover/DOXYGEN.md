# FAILOVER DOXYGEN

Author: ThemisDB Contributors
Created: 2026-09-20
Last Updated: 2026-09-20
Status: active

## Source
- This file is generated from module-scoped Doxygen XML.
- XML index: `C:\Projects\ThemisDB\.tmp\module_doxygen_global_e2e\failover\xml\index.xml`
- Warnings log: `C:\Projects\ThemisDB\.tmp\module_doxygen_global_e2e\failover\doxygen-warnings.log`

## Structure Metrics
- C/C++ Files (scanned): 24
- Compounds: 72
- Classes/Structs: 23
- Namespaces: 17
- File Compounds: 24

## Namespaces
- @031130112135124234056063224170366113076136362330
- @036055323304043122231013321005101011236142116046
- @160031333047117344323160027010055365120031125145
- @167325001277070264116334136010014262164073244342
- @244016062232013226323154076345262144331213132000
- @253217345363203142105100346352067202343051214106
- @265111263155144223316226217255341272154126134054
- @313212177131176064250247221310101000124064212160
- std::chrono_literals
- themis
- themis::bench
- themis::bench::fp23
- themis::bench::frg
- themis::failover
- themis::failover::@027245370360115255137310365220315202231202057014
- themis::failover::test
- themis::sharding

## Types
### Classes
- themis::failover::AutoFailoverManager
- themis::failover::DisasterRecoveryManager
- themis::failover::QuorumLog

### Structs
- themis::bench::fp23::MockFencingManager
- themis::bench::fp23::MockQueueStats
- themis::bench::fp23::MockRecoveryStats
- themis::bench::frg::HeartbeatMsg
- themis::bench::frg::InFlightBuffer
- themis::bench::frg::NodeHealthState
- themis::bench::frg::StateSyncMsg
- themis::failover::AutoFailoverConfig
- themis::failover::AutoFailoverManager::FailoverTask
- themis::failover::AutoFailoverManager::Statistics
- themis::failover::DisasterRecoveryConfig
- themis::failover::DisasterRecoveryManager::EnumHash
- themis::failover::DisasterRecoveryManager::Statistics
- themis::failover::DisasterRecoveryPlan
- themis::failover::DisasterRecoveryResult
- themis::failover::DisasterRecoveryStepResult
- themis::failover::FailoverResult
- themis::failover::QuorumEntry
- themis::failover::QuorumState
- themis::failover::TopologySnapshot

## Notes
- This is a derived artifact. Do not manually edit semantic content.
- Regenerate via scripts/generate_module_doxygen_xml.py --write-module-markdown.

## Detailed Function Documentation (Soll-Ist)
- Functions extracted: 200

### bench_failover_phase2_phase3_gates.cpp

#### `BENCHMARK_MAIN()`
- Source: `benchmarks/failover/bench_failover_phase2_phase3_gates.cpp`:504
- Brief: n/a
- Parameters: none

### bench_failover_release_gates.cpp

#### `BENCHMARK_MAIN()`
- Source: `benchmarks/failover/bench_failover_release_gates.cpp`:312
- Brief: n/a
- Parameters: none

### bench_failover_wave_b_gates.cpp

#### `BENCHMARK(BM_FWB01_AdaptiveIntervalUpdate) -> Repetitions(5) ->ReportAggregatesOnly(true)`
- Source: `benchmarks/failover/bench_failover_wave_b_gates.cpp`:149
- Brief: n/a
- Parameters:
  - `<unnamed>` (BM_FWB01_AdaptiveIntervalUpdate): n/a

#### `BENCHMARK(BM_FWB02_QuorumLogAppendInMemory) -> Repetitions(5) ->ReportAggregatesOnly(true)`
- Source: `benchmarks/failover/bench_failover_wave_b_gates.cpp`:213
- Brief: n/a
- Parameters:
  - `<unnamed>` (BM_FWB02_QuorumLogAppendInMemory): n/a

#### `BENCHMARK(BM_FWB03_TopologySnapshotCapture) -> Repetitions(5) ->ReportAggregatesOnly(true)`
- Source: `benchmarks/failover/bench_failover_wave_b_gates.cpp`:234
- Brief: n/a
- Parameters:
  - `<unnamed>` (BM_FWB03_TopologySnapshotCapture): n/a

#### `BENCHMARK(BM_FWB04_TopologySnapshotDiff) -> Repetitions(5) ->ReportAggregatesOnly(true)`
- Source: `benchmarks/failover/bench_failover_wave_b_gates.cpp`:259
- Brief: n/a
- Parameters:
  - `<unnamed>` (BM_FWB04_TopologySnapshotDiff): n/a

#### `BENCHMARK(BM_FWB05_GcGraceBurstDetection) -> Repetitions(5) ->ReportAggregatesOnly(true)`
- Source: `benchmarks/failover/bench_failover_wave_b_gates.cpp`:293
- Brief: n/a
- Parameters:
  - `<unnamed>` (BM_FWB05_GcGraceBurstDetection): n/a

#### `BENCHMARK(BM_FWB06_QuorumResolutionFastPath) -> Repetitions(5) ->ReportAggregatesOnly(true)`
- Source: `benchmarks/failover/bench_failover_wave_b_gates.cpp`:316
- Brief: n/a
- Parameters:
  - `<unnamed>` (BM_FWB06_QuorumResolutionFastPath): n/a

#### `BENCHMARK(BM_FWB07_RecoveryBatchStatFlush) -> Repetitions(5) ->ReportAggregatesOnly(true)`
- Source: `benchmarks/failover/bench_failover_wave_b_gates.cpp`:349
- Brief: n/a
- Parameters:
  - `<unnamed>` (BM_FWB07_RecoveryBatchStatFlush): n/a

#### `void BM_FWB01_AdaptiveIntervalUpdate(benchmark::State &state)`
- Source: `benchmarks/failover/bench_failover_wave_b_gates.cpp`:130
- Brief: FWB-01: overhead of computing p95 latency from a 20-sample rolling window and updating the adaptive interval.
- Parameters:
  - `state` (benchmark::State &): n/a
- Details: Uses a standalone p95-sort simulation that mirrors updateAdaptiveInterval() logic without requiring a full AutoFailoverManager instance.

#### `void BM_FWB02_QuorumLogAppendInMemory(benchmark::State &state)`
- Source: `benchmarks/failover/bench_failover_wave_b_gates.cpp`:162
- Brief: FWB-02: overhead of quorum entry CRC32 computation and string serialization.
- Parameters:
  - `state` (benchmark::State &): n/a
- Details: Simulates the work performed by QuorumLog::append() excluding the actual file write (which would be I/O-bound and not representative of the hot path overhead).

#### `void BM_FWB03_TopologySnapshotCapture(benchmark::State &state)`
- Source: `benchmarks/failover/bench_failover_wave_b_gates.cpp`:225
- Brief: FWB-03: overhead of TopologySnapshot::capture() with 16 nodes.
- Parameters:
  - `state` (benchmark::State &): n/a
- Details: Capture includes copying the failure map and sorting node IDs for deterministic ordering.

#### `void BM_FWB04_TopologySnapshotDiff(benchmark::State &state)`
- Source: `benchmarks/failover/bench_failover_wave_b_gates.cpp`:244
- Brief: FWB-04: overhead of has_topology_change() + added_nodes() between two 16-node snapshots that differ by one node.
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_FWB05_GcGraceBurstDetection(benchmark::State &state)`
- Source: `benchmarks/failover/bench_failover_wave_b_gates.cpp`:271
- Brief: FWB-05: overhead of scanning and pruning a 20-element failure-timestamp vector to determine whether GC grace period applies.
- Parameters:
  - `state` (benchmark::State &): n/a
- Details: Simulates the inner loop of checkAndApplyGcGrace().

#### `void BM_FWB06_QuorumResolutionFastPath(benchmark::State &state)`
- Source: `benchmarks/failover/bench_failover_wave_b_gates.cpp`:304
- Brief: FWB-06: overhead of quorum deadline check when quorum is instantly satisfied.
- Parameters:
  - `state` (benchmark::State &): n/a
- Details: Simulates the tight loop in checkAndWaitForQuorum() with a no-op hasQuorum mock.

#### `void BM_FWB07_RecoveryBatchStatFlush(benchmark::State &state)`
- Source: `benchmarks/failover/bench_failover_wave_b_gates.cpp`:328
- Brief: FWB-07: overhead of batch stats flush in attemptRecovery() success path.
- Parameters:
  - `state` (benchmark::State &): n/a
- Details: Simulates the single stats_mutex_ acquisition + three counter increments that replace the per-iteration locking pattern.

#### `void BM_FWB08_ConcurrentQueueEnqueue(benchmark::State &state)`
- Source: `benchmarks/failover/bench_failover_wave_b_gates.cpp`:361
- Brief: FWB-08: per-enqueue throughput of triggerManualFailover() queue path under 4 concurrent producers.
- Parameters:
  - `state` (benchmark::State &): n/a
- Details: Simulates the mutex-protected queue push without the full manager overhead.

#### `Repetitions(5) -> ReportAggregatesOnly(true) ->Threads(4)`
- Source: `benchmarks/failover/bench_failover_wave_b_gates.cpp`:387
- Brief: n/a
- Parameters:
  - `<unnamed>` (5): n/a

### test_failover_contract_hardening_focused.cpp

#### `TEST(FailoverContractHardeningFCH01, SingleLeaderPerEpoch)`
- Source: `tests/failover/test_failover_contract_hardening_focused.cpp`:147
- Brief: n/a
- Parameters:
  - `<unnamed>` (FailoverContractHardeningFCH01): n/a
  - `<unnamed>` (SingleLeaderPerEpoch): n/a

#### `TEST(FailoverContractHardeningFCH02, EpochMonotonicallyIncreases)`
- Source: `tests/failover/test_failover_contract_hardening_focused.cpp`:162
- Brief: n/a
- Parameters:
  - `<unnamed>` (FailoverContractHardeningFCH02): n/a
  - `<unnamed>` (EpochMonotonicallyIncreases): n/a

#### `TEST(FailoverContractHardeningFCH03, OldLeaderDeposedOnHigherEpoch)`
- Source: `tests/failover/test_failover_contract_hardening_focused.cpp`:181
- Brief: n/a
- Parameters:
  - `<unnamed>` (FailoverContractHardeningFCH03): n/a
  - `<unnamed>` (OldLeaderDeposedOnHigherEpoch): n/a

#### `TEST(FailoverContractHardeningFCH04, NoSplitBrainSameEpoch)`
- Source: `tests/failover/test_failover_contract_hardening_focused.cpp`:207
- Brief: n/a
- Parameters:
  - `<unnamed>` (FailoverContractHardeningFCH04): n/a
  - `<unnamed>` (NoSplitBrainSameEpoch): n/a

#### `TEST(FailoverContractHardeningFCH05, InFlightCompletedBeforeYield)`
- Source: `tests/failover/test_failover_contract_hardening_focused.cpp`:224
- Brief: n/a
- Parameters:
  - `<unnamed>` (FailoverContractHardeningFCH05): n/a
  - `<unnamed>` (InFlightCompletedBeforeYield): n/a

#### `TEST(FailoverContractHardeningFCH06, InFlightRetriedByNewLeader)`
- Source: `tests/failover/test_failover_contract_hardening_focused.cpp`:242
- Brief: n/a
- Parameters:
  - `<unnamed>` (FailoverContractHardeningFCH06): n/a
  - `<unnamed>` (InFlightRetriedByNewLeader): n/a

#### `TEST(FailoverContractHardeningFCH07, NoSilentDropOnHandover)`
- Source: `tests/failover/test_failover_contract_hardening_focused.cpp`:262
- Brief: n/a
- Parameters:
  - `<unnamed>` (FailoverContractHardeningFCH07): n/a
  - `<unnamed>` (NoSilentDropOnHandover): n/a

#### `TEST(FailoverContractHardeningFCH08, HandoverDrainBudgetRespected)`
- Source: `tests/failover/test_failover_contract_hardening_focused.cpp`:286
- Brief: n/a
- Parameters:
  - `<unnamed>` (FailoverContractHardeningFCH08): n/a
  - `<unnamed>` (HandoverDrainBudgetRespected): n/a

#### `TEST(FailoverContractHardeningFCH09, FailedNodeRejoinsAsFollower)`
- Source: `tests/failover/test_failover_contract_hardening_focused.cpp`:310
- Brief: n/a
- Parameters:
  - `<unnamed>` (FailoverContractHardeningFCH09): n/a
  - `<unnamed>` (FailedNodeRejoinsAsFollower): n/a

#### `TEST(FailoverContractHardeningFCH10, RejoiningNodeCannotClaimLeaderDirectly)`
- Source: `tests/failover/test_failover_contract_hardening_focused.cpp`:327
- Brief: n/a
- Parameters:
  - `<unnamed>` (FailoverContractHardeningFCH10): n/a
  - `<unnamed>` (RejoiningNodeCannotClaimLeaderDirectly): n/a

#### `TEST(FailoverContractHardeningFCH11, StateSyncBeforeActiveFollower)`
- Source: `tests/failover/test_failover_contract_hardening_focused.cpp`:346
- Brief: n/a
- Parameters:
  - `<unnamed>` (FailoverContractHardeningFCH11): n/a
  - `<unnamed>` (StateSyncBeforeActiveFollower): n/a

#### `TEST(FailoverContractHardeningFCH12, StateSyncTimeoutSurfaces)`
- Source: `tests/failover/test_failover_contract_hardening_focused.cpp`:369
- Brief: n/a
- Parameters:
  - `<unnamed>` (FailoverContractHardeningFCH12): n/a
  - `<unnamed>` (StateSyncTimeoutSurfaces): n/a

#### `TEST(FailoverContractHardeningFCH13, ElectionTimeoutAfterMissedHeartbeats)`
- Source: `tests/failover/test_failover_contract_hardening_focused.cpp`:389
- Brief: n/a
- Parameters:
  - `<unnamed>` (FailoverContractHardeningFCH13): n/a
  - `<unnamed>` (ElectionTimeoutAfterMissedHeartbeats): n/a

#### `TEST(FailoverContractHardeningFCH14, SplitBrainDetectedSurfaced)`
- Source: `tests/failover/test_failover_contract_hardening_focused.cpp`:407
- Brief: n/a
- Parameters:
  - `<unnamed>` (FailoverContractHardeningFCH14): n/a
  - `<unnamed>` (SplitBrainDetectedSurfaced): n/a

#### `TEST(FailoverContractHardeningFCH15, HandoverIncompleteWhenDrainExceeded)`
- Source: `tests/failover/test_failover_contract_hardening_focused.cpp`:430
- Brief: n/a
- Parameters:
  - `<unnamed>` (FailoverContractHardeningFCH15): n/a
  - `<unnamed>` (HandoverIncompleteWhenDrainExceeded): n/a

#### `TEST(FailoverContractHardeningFCH16, HeartbeatMissedCounterIncrement)`
- Source: `tests/failover/test_failover_contract_hardening_focused.cpp`:454
- Brief: n/a
- Parameters:
  - `<unnamed>` (FailoverContractHardeningFCH16): n/a
  - `<unnamed>` (HeartbeatMissedCounterIncrement): n/a

#### `TEST(FailoverContractHardeningRetryContract, RetryEscalationAndTimeoutSourceMapping)`
- Source: `tests/failover/test_failover_contract_hardening_focused.cpp`:474
- Brief: n/a
- Parameters:
  - `<unnamed>` (FailoverContractHardeningRetryContract): n/a
  - `<unnamed>` (RetryEscalationAndTimeoutSourceMapping): n/a

### test_failover_dr_edge_scenarios.cpp

#### `TEST(DisasterRecoveryEdgeDRE01, EmptyPlanIdRejected)`
- Source: `tests/failover/test_failover_dr_edge_scenarios.cpp`:78
- Brief: n/a
- Parameters:
  - `<unnamed>` (DisasterRecoveryEdgeDRE01): n/a
  - `<unnamed>` (EmptyPlanIdRejected): n/a

#### `TEST(DisasterRecoveryEdgeDRE02, EmptyPrimarySiteRejected)`
- Source: `tests/failover/test_failover_dr_edge_scenarios.cpp`:93
- Brief: n/a
- Parameters:
  - `<unnamed>` (DisasterRecoveryEdgeDRE02): n/a
  - `<unnamed>` (EmptyPrimarySiteRejected): n/a

#### `TEST(DisasterRecoveryEdgeDRE03, EmptyRecoverySiteRejected)`
- Source: `tests/failover/test_failover_dr_edge_scenarios.cpp`:108
- Brief: n/a
- Parameters:
  - `<unnamed>` (DisasterRecoveryEdgeDRE03): n/a
  - `<unnamed>` (EmptyRecoverySiteRejected): n/a

#### `TEST(DisasterRecoveryEdgeDRE04, MissingSnapshotIdRejectedForNonDryRun)`
- Source: `tests/failover/test_failover_dr_edge_scenarios.cpp`:123
- Brief: n/a
- Parameters:
  - `<unnamed>` (DisasterRecoveryEdgeDRE04): n/a
  - `<unnamed>` (MissingSnapshotIdRejectedForNonDryRun): n/a

#### `TEST(DisasterRecoveryEdgeDRE05, DryRunAcceptedWithoutSnapshotId)`
- Source: `tests/failover/test_failover_dr_edge_scenarios.cpp`:139
- Brief: n/a
- Parameters:
  - `<unnamed>` (DisasterRecoveryEdgeDRE05): n/a
  - `<unnamed>` (DryRunAcceptedWithoutSnapshotId): n/a

#### `TEST(DisasterRecoveryEdgeDRE06, InvalidPlanSetsFailedStateWithError)`
- Source: `tests/failover/test_failover_dr_edge_scenarios.cpp`:155
- Brief: n/a
- Parameters:
  - `<unnamed>` (DisasterRecoveryEdgeDRE06): n/a
  - `<unnamed>` (InvalidPlanSetsFailedStateWithError): n/a

#### `TEST(DisasterRecoveryEdgeDRE07, DryRunPlanCompletesSuccessfully)`
- Source: `tests/failover/test_failover_dr_edge_scenarios.cpp`:172
- Brief: n/a
- Parameters:
  - `<unnamed>` (DisasterRecoveryEdgeDRE07): n/a
  - `<unnamed>` (DryRunPlanCompletesSuccessfully): n/a

#### `TEST(DisasterRecoveryEdgeDRE08, StepHookFailureIsolatedAndStatisticsUpdated)`
- Source: `tests/failover/test_failover_dr_edge_scenarios.cpp`:206
- Brief: n/a
- Parameters:
  - `<unnamed>` (DisasterRecoveryEdgeDRE08): n/a
  - `<unnamed>` (StepHookFailureIsolatedAndStatisticsUpdated): n/a

### test_failover_phase2_phase3_focused.cpp

#### `TEST(FailoverPhase2Phase3, P23_01_CanTransitionFalseForImpossibleTransitions)`
- Source: `tests/failover/test_failover_phase2_phase3_focused.cpp`:107
- Brief: n/a
- Parameters:
  - `<unnamed>` (FailoverPhase2Phase3): n/a
  - `<unnamed>` (P23_01_CanTransitionFalseForImpossibleTransitions): n/a

#### `TEST(FailoverPhase2Phase3, P23_02_CanTransitionTrueForValidForwardTransitions)`
- Source: `tests/failover/test_failover_phase2_phase3_focused.cpp`:132
- Brief: n/a
- Parameters:
  - `<unnamed>` (FailoverPhase2Phase3): n/a
  - `<unnamed>` (P23_02_CanTransitionTrueForValidForwardTransitions): n/a

#### `TEST(FailoverPhase2Phase3, P23_03_PreventSplitBrainFailsClosedWithNoFencingManager)`
- Source: `tests/failover/test_failover_phase2_phase3_focused.cpp`:174
- Brief: n/a
- Parameters:
  - `<unnamed>` (FailoverPhase2Phase3): n/a
  - `<unnamed>` (P23_03_PreventSplitBrainFailsClosedWithNoFencingManager): n/a

#### `TEST(FailoverPhase2Phase3, P23_04_AttemptRecoveryStatsAfterMaxAttemptsExhausted)`
- Source: `tests/failover/test_failover_phase2_phase3_focused.cpp`:189
- Brief: n/a
- Parameters:
  - `<unnamed>` (FailoverPhase2Phase3): n/a
  - `<unnamed>` (P23_04_AttemptRecoveryStatsAfterMaxAttemptsExhausted): n/a

#### `TEST(FailoverPhase2Phase3, P23_05_TriggerManualFailoverDropsWhenQueueFull)`
- Source: `tests/failover/test_failover_phase2_phase3_focused.cpp`:210
- Brief: n/a
- Parameters:
  - `<unnamed>` (FailoverPhase2Phase3): n/a
  - `<unnamed>` (P23_05_TriggerManualFailoverDropsWhenQueueFull): n/a

#### `TEST(FailoverPhase2Phase3, P23_06_ExecutePlanConcurrentCallRejected)`
- Source: `tests/failover/test_failover_phase2_phase3_focused.cpp`:235
- Brief: n/a
- Parameters:
  - `<unnamed>` (FailoverPhase2Phase3): n/a
  - `<unnamed>` (P23_06_ExecutePlanConcurrentCallRejected): n/a

#### `TEST(FailoverPhase2Phase3, P23_07_EmitDiagnosticFiresCallbackForQuorumUnavailable)`
- Source: `tests/failover/test_failover_phase2_phase3_focused.cpp`:281
- Brief: n/a
- Parameters:
  - `<unnamed>` (FailoverPhase2Phase3): n/a
  - `<unnamed>` (P23_07_EmitDiagnosticFiresCallbackForQuorumUnavailable): n/a

#### `TEST(FailoverPhase2Phase3, P23_08_AttemptRecoveryStatsBatchUpdated)`
- Source: `tests/failover/test_failover_phase2_phase3_focused.cpp`:305
- Brief: n/a
- Parameters:
  - `<unnamed>` (FailoverPhase2Phase3): n/a
  - `<unnamed>` (P23_08_AttemptRecoveryStatsBatchUpdated): n/a

### test_failover_wave_a_quorum_persistence.cpp

#### `TEST_F(QuorumLogTest, AppendRecover)`
- Source: `tests/failover/test_failover_wave_a_quorum_persistence.cpp`:140
- Brief: n/a
- Parameters:
  - `<unnamed>` (QuorumLogTest): n/a
  - `<unnamed>` (AppendRecover): n/a

#### `TEST_F(QuorumLogTest, CorruptSkip)`
- Source: `tests/failover/test_failover_wave_a_quorum_persistence.cpp`:158
- Brief: n/a
- Parameters:
  - `<unnamed>` (QuorumLogTest): n/a
  - `<unnamed>` (CorruptSkip): n/a

#### `TEST_F(QuorumLogTest, EmptyLog)`
- Source: `tests/failover/test_failover_wave_a_quorum_persistence.cpp`:185
- Brief: n/a
- Parameters:
  - `<unnamed>` (QuorumLogTest): n/a
  - `<unnamed>` (EmptyLog): n/a

#### `TEST_F(QuorumLogTest, Integration)`
- Source: `tests/failover/test_failover_wave_a_quorum_persistence.cpp`:223
- Brief: n/a
- Parameters:
  - `<unnamed>` (QuorumLogTest): n/a
  - `<unnamed>` (Integration): n/a

#### `TEST_F(QuorumLogTest, WriteFail)`
- Source: `tests/failover/test_failover_wave_a_quorum_persistence.cpp`:200
- Brief: n/a
- Parameters:
  - `<unnamed>` (QuorumLogTest): n/a
  - `<unnamed>` (WriteFail): n/a

### test_failover_wave_c_fencing_security.cpp

#### `TEST(FailoverWaveCConcurrent01, SamePlanIdBothGetCachedResult)`
- Source: `tests/failover/test_failover_wave_c_fencing_security.cpp`:371
- Brief: n/a
- Parameters:
  - `<unnamed>` (FailoverWaveCConcurrent01): n/a
  - `<unnamed>` (SamePlanIdBothGetCachedResult): n/a

#### `TEST(FailoverWaveCConcurrent02, DifferentPlanIdsOneRejected)`
- Source: `tests/failover/test_failover_wave_c_fencing_security.cpp`:408
- Brief: n/a
- Parameters:
  - `<unnamed>` (FailoverWaveCConcurrent02): n/a
  - `<unnamed>` (DifferentPlanIdsOneRejected): n/a

#### `TEST(FailoverWaveCFence01, NoFencingManagerFails)`
- Source: `tests/failover/test_failover_wave_c_fencing_security.cpp`:239
- Brief: n/a
- Parameters:
  - `<unnamed>` (FailoverWaveCFence01): n/a
  - `<unnamed>` (NoFencingManagerFails): n/a

#### `TEST(FailoverWaveCFence02, InvalidEpochZeroFails)`
- Source: `tests/failover/test_failover_wave_c_fencing_security.cpp`:275
- Brief: n/a
- Parameters:
  - `<unnamed>` (FailoverWaveCFence02): n/a
  - `<unnamed>` (InvalidEpochZeroFails): n/a

#### `TEST(FailoverWaveCFence03, ValidEpochPopulatesResult)`
- Source: `tests/failover/test_failover_wave_c_fencing_security.cpp`:307
- Brief: n/a
- Parameters:
  - `<unnamed>` (FailoverWaveCFence03): n/a
  - `<unnamed>` (ValidEpochPopulatesResult): n/a

#### `TEST(FailoverWaveCFence04, FencingDisabledSkipsStep)`
- Source: `tests/failover/test_failover_wave_c_fencing_security.cpp`:338
- Brief: n/a
- Parameters:
  - `<unnamed>` (FailoverWaveCFence04): n/a
  - `<unnamed>` (FencingDisabledSkipsStep): n/a

#### `TEST(FailoverWaveCIdem01, SamePlanIdCachedResult)`
- Source: `tests/failover/test_failover_wave_c_fencing_security.cpp`:138
- Brief: n/a
- Parameters:
  - `<unnamed>` (FailoverWaveCIdem01): n/a
  - `<unnamed>` (SamePlanIdCachedResult): n/a

#### `TEST(FailoverWaveCIdem02, FailedPlanNotCachedForEmptyId)`
- Source: `tests/failover/test_failover_wave_c_fencing_security.cpp`:165
- Brief: n/a
- Parameters:
  - `<unnamed>` (FailoverWaveCIdem02): n/a
  - `<unnamed>` (FailedPlanNotCachedForEmptyId): n/a

#### `TEST(FailoverWaveCIdem03, TwoDifferentPlanIdsIndependent)`
- Source: `tests/failover/test_failover_wave_c_fencing_security.cpp`:211
- Brief: n/a
- Parameters:
  - `<unnamed>` (FailoverWaveCIdem03): n/a
  - `<unnamed>` (TwoDifferentPlanIdsIndependent): n/a

### themis::bench::fp23

#### `void BM_FP23_01_CanTransitionLookup(benchmark::State &state)`
- Source: `benchmarks/failover/bench_failover_phase2_phase3_gates.cpp`:319
- Brief: FP23-01: State-machine canTransition() switch dispatch over a mixed set of valid and invalid pairs.
- Parameters:
  - `state` (benchmark::State &): n/a
- Details: GATE-FP23-01: p99 ≤ 100 µs.

#### `void BM_FP23_02_PreventSplitBrainFailClosed(benchmark::State &state)`
- Source: `benchmarks/failover/bench_failover_phase2_phase3_gates.cpp`:375
- Brief: FP23-02: preventSplitBrain() null-fencing-manager fast reject.
- Parameters:
  - `state` (benchmark::State &): n/a
- Details: Measures the null pointer check + fail-closed decision. GATE-FP23-02: p99 ≤ 200 µs.

#### `void BM_FP23_03_ExecutePlanConcurrencyGuard(benchmark::State &state)`
- Source: `benchmarks/failover/bench_failover_phase2_phase3_gates.cpp`:404
- Brief: FP23-03: executePlan() try_to_lock concurrency guard, uncontested path.
- Parameters:
  - `state` (benchmark::State &): n/a
- Details: Measures unique_lock construction with try_to_lock + owns_lock() check on an uncontested mutex. This is the fast-accept path; the reject path (contested mutex) would be similarly fast. GATE-FP23-03: p99 ≤ 100 µs.

#### `void BM_FP23_04_AttemptRecoveryBatchStatsFlush(benchmark::State &state)`
- Source: `benchmarks/failover/bench_failover_phase2_phase3_gates.cpp`:430
- Brief: FP23-04: attemptRecovery() batch stats update — single lock + 3 increments.
- Parameters:
  - `state` (benchmark::State &): n/a
- Details: Measures the batched stats-flush pattern (one lock_guard per call) that replaced the per-iteration lock pattern in Phase 2 hardening. GATE-FP23-04: p99 ≤ 200 µs.

#### `void BM_FP23_05_EmitDiagnosticDispatch(benchmark::State &state)`
- Source: `benchmarks/failover/bench_failover_phase2_phase3_gates.cpp`:457
- Brief: FP23-05: emitDiagnostic() code-to-event-type switch + empty callback dispatch.
- Parameters:
  - `state` (benchmark::State &): n/a
- Details: Represents the most common runtime case: QUORUM_UNAVAILABLE diagnostic with no subscribers registered. The spdlog::error call present in production is omitted to isolate dispatch overhead. GATE-FP23-05: p99 ≤ 100 µs.

#### `void BM_FP23_06_TriggerFailoverQueueFullDrop(benchmark::State &state)`
- Source: `benchmarks/failover/bench_failover_phase2_phase3_gates.cpp`:484
- Brief: FP23-06: triggerManualFailover() fast-drop path — full-queue detection + stats.
- Parameters:
  - `state` (benchmark::State &): n/a
- Details: Simulates the queue-saturation rejection introduced in Phase 2 hardening: size check against kMockMaxConcurrentFailovers, then lock_guard + counter increment. GATE-FP23-06: p99 ≤ 200 µs.

#### `Repetitions(kRepetitions) -> ReportAggregatesOnly(true)`
- Source: `benchmarks/failover/bench_failover_phase2_phase3_gates.cpp`:361
- Brief: n/a
- Parameters:
  - `<unnamed>` (kRepetitions): n/a

#### `void mockBatchStatsFlush(uint64_t local_total, uint64_t local_failed, uint64_t local_success) noexcept`
- Source: `benchmarks/failover/bench_failover_phase2_phase3_gates.cpp`:229
- Brief: Inline replica of the batch stats flush at end of attemptRecovery().
- Parameters:
  - `local_total` (uint64_t): Accumulated total attempt count (not yet flushed).
  - `local_failed` (uint64_t): Accumulated failed attempt count.
  - `local_success` (uint64_t): Accumulated successful attempt count.
- Details: local_total Accumulated total attempt count (not yet flushed). local_failed Accumulated failed attempt count. local_success Accumulated successful attempt count.

#### `bool mockCanTransition(FailoverOrchestratorState from, FailoverOrchestratorState to) noexcept`
- Source: `benchmarks/failover/bench_failover_phase2_phase3_gates.cpp`:106
- Brief: Inline replica of AutoFailoverManager::canTransition() for benchmarking.
- Parameters:
  - `from` (FailoverOrchestratorState): Source state.
  - `to` (FailoverOrchestratorState): Target state.
- Return: true if the transition is in the allowed table.
- Details: Models the state-machine switch (9 arms + 2 universal rules) that was delivered as part of Phase 2/3 hardening. The logic is intentionally kept identical to the production implementation. from Source state. to Target state. true if the transition is in the allowed table.

#### `void mockEmitDiagnostic(FailoverErrorCode code, const std::vector< FailoverEventCallback > &callbacks) noexcept`
- Source: `benchmarks/failover/bench_failover_phase2_phase3_gates.cpp`:256
- Brief: Inline replica of emitDiagnostic() code-to-event-type mapping + dispatch.
- Parameters:
  - `code` (FailoverErrorCode): Canonical error code.
  - `callbacks` (const std::vector< FailoverEventCallback > &): Registered event callbacks (empty in the typical case).
- Details: code Canonical error code. callbacks Registered event callbacks (empty in the typical case).

#### `bool mockExecutionGuard() noexcept`
- Source: `benchmarks/failover/bench_failover_phase2_phase3_gates.cpp`:200
- Brief: Inline replica of the executePlan() concurrency guard.
- Parameters: none
- Return: true if the lock was acquired (no concurrent execution); false otherwise.
- Details: true if the lock was acquired (no concurrent execution); false otherwise.

#### `bool mockPreventSplitBrainFailClosed(const MockFencingManager *fencing_mgr) noexcept`
- Source: `benchmarks/failover/bench_failover_phase2_phase3_gates.cpp`:174
- Brief: Inline replica of the preventSplitBrain() null-manager fast-reject path.
- Parameters:
  - `fencing_mgr` (const MockFencingManager *): Pointer to the fencing manager (nullptr → fail closed).
- Return: false when no fencing manager is configured.
- Details: fencing_mgr Pointer to the fencing manager (nullptr → fail closed). false when no fencing manager is configured.

#### `bool mockTriggerFailoverQueueFullDrop(std::size_t queue_size) noexcept`
- Source: `benchmarks/failover/bench_failover_phase2_phase3_gates.cpp`:297
- Brief: Inline replica of the triggerManualFailover() drop path.
- Parameters:
  - `queue_size` (std::size_t): Current number of items already in the failover queue.
- Return: false when the queue is at capacity (drop path).
- Details: queue_size Current number of items already in the failover queue. false when the queue is at capacity (drop path).

### themis::bench::frg

#### `Arg(3) -> Arg(5) ->Arg(9) ->Repetitions(kRepetitions) ->ReportAggregatesOnly(true)`
- Source: `benchmarks/failover/bench_failover_release_gates.cpp`:210
- Brief: n/a
- Parameters:
  - `<unnamed>` (3): n/a

#### `void BM_FRG01_HeartbeatSendOverhead(benchmark::State &state)`
- Source: `benchmarks/failover/bench_failover_release_gates.cpp`:174
- Brief: FRG-01: Heartbeat message construction + in-memory "send" overhead. GATE-FRG-01: p99 ≤ 500 µs.
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_FRG02_ElectionDecision(benchmark::State &state)`
- Source: `benchmarks/failover/bench_failover_release_gates.cpp`:198
- Brief: FRG-02: In-memory leader election decision for 5-node cluster. GATE-FRG-02: p99 ≤ 5 ms.
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_FRG03_StateSyncSerialize(benchmark::State &state)`
- Source: `benchmarks/failover/bench_failover_release_gates.cpp`:222
- Brief: FRG-03: StateSyncMsg construction (mock serialize). GATE-FRG-03: p99 ≤ 200 µs.
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_FRG04_HealthCheckEvaluation(benchmark::State &state)`
- Source: `benchmarks/failover/bench_failover_release_gates.cpp`:244
- Brief: FRG-04: NodeHealthState evaluation (single node). GATE-FRG-04: p99 ≤ 100 µs.
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_FRG05_InFlightBufferCheck(benchmark::State &state)`
- Source: `benchmarks/failover/bench_failover_release_gates.cpp`:271
- Brief: FRG-05: InFlightBuffer::hasRoom() atomic load. GATE-FRG-05: p99 ≤ 50 µs.
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_FRG06_EpochIncrementPersist(benchmark::State &state)`
- Source: `benchmarks/failover/bench_failover_release_gates.cpp`:295
- Brief: FRG-06: Atomic epoch increment (models "increment + mock persist" cost). GATE-FRG-06: p99 ≤ 1 ms.
- Parameters:
  - `state` (benchmark::State &): n/a

#### `Repetitions(kRepetitions) -> ReportAggregatesOnly(true)`
- Source: `benchmarks/failover/bench_failover_release_gates.cpp`:186
- Brief: n/a
- Parameters:
  - `<unnamed>` (kRepetitions): n/a

#### `bool evaluateHealth(const NodeHealthState &s, std::uint64_t now_ms) noexcept`
- Source: `benchmarks/failover/bench_failover_release_gates.cpp`:135
- Brief: n/a
- Parameters:
  - `s` (const NodeHealthState &): n/a
  - `now_ms` (std::uint64_t): n/a

#### `std::uint64_t incrementAndPersistEpoch() noexcept`
- Source: `benchmarks/failover/bench_failover_release_gates.cpp`:161
- Brief: n/a
- Parameters: none

#### `HeartbeatMsg makeHeartbeat(std::uint64_t epoch, std::uint64_t id) noexcept`
- Source: `benchmarks/failover/bench_failover_release_gates.cpp`:86
- Brief: n/a
- Parameters:
  - `epoch` (std::uint64_t): n/a
  - `id` (std::uint64_t): n/a

#### `StateSyncMsg makeStateSync(std::uint64_t epoch, std::uint64_t offset) noexcept`
- Source: `benchmarks/failover/bench_failover_release_gates.cpp`:117
- Brief: n/a
- Parameters:
  - `epoch` (std::uint64_t): n/a
  - `offset` (std::uint64_t): n/a

#### `std::uint64_t runMockElection(std::uint64_t current_epoch, std::uint64_t n_nodes) noexcept`
- Source: `benchmarks/failover/bench_failover_release_gates.cpp`:95
- Brief: n/a
- Parameters:
  - `current_epoch` (std::uint64_t): n/a
  - `n_nodes` (std::uint64_t): n/a

### themis::bench::frg::InFlightBuffer

#### `bool hasRoom(std::size_t n=1) const noexcept`
- Source: `benchmarks/failover/bench_failover_release_gates.cpp`:150
- Brief: n/a
- Parameters:
  - `n` (std::size_t): n/a

### themis::failover

#### `bool isFailSafeCode(FailoverErrorCode code) noexcept`
- Source: `include/failover/failover_api_contract.h`:188
- Brief: Returns true for error codes that mandate a fail-safe (no-leader) outcome.
- Parameters:
  - `code` (FailoverErrorCode): n/a

#### `bool isNewerEpoch(std::uint64_t newer, std::uint64_t current) noexcept`
- Source: `include/failover/failover_api_contract.h`:107
- Brief: Returns true when newer is strictly higher than current, i.e. the caller should step down and accept newer as authoritative.
- Parameters:
  - `newer` (std::uint64_t): n/a
  - `current` (std::uint64_t): n/a

#### `bool isRetryEscalationCode(FailoverErrorCode code) noexcept`
- Source: `include/failover/failover_api_contract.h`:197
- Brief: Returns true for failover errors that indicate retry escalation paths.
- Parameters:
  - `code` (FailoverErrorCode): n/a

#### `bool isValidEpoch(std::uint64_t epoch) noexcept`
- Source: `include/failover/failover_api_contract.h`:101
- Brief: Returns true when the given epoch value is valid (non-zero).
- Parameters:
  - `epoch` (std::uint64_t): n/a

#### `bool isVoteEligible(NodeRole role) noexcept`
- Source: `include/failover/failover_api_contract.h`:138
- Brief: Returns true when the role allows a node to cast votes in elections.
- Parameters:
  - `role` (NodeRole): n/a

#### `bool isWriteEligible(NodeRole role) noexcept`
- Source: `include/failover/failover_api_contract.h`:133
- Brief: Returns true when the role allows a node to accept write operations.
- Parameters:
  - `role` (NodeRole): n/a

#### `std::size_t quorumSize(std::size_t cluster_size) noexcept`
- Source: `include/failover/failover_api_contract.h`:241
- Brief: Compute the quorum size required for a cluster of cluster_size nodes.
- Parameters:
  - `cluster_size` (std::size_t): n/a

#### `themis::utils::RetryTimeoutSource toRetryTimeoutSource(FailoverErrorCode code) noexcept`
- Source: `include/failover/failover_api_contract.h`:206
- Brief: Map failover errors to canonical timeout-source taxonomy.
- Parameters:
  - `code` (FailoverErrorCode): n/a

### themis::failover::AutoFailoverManager

#### `AutoFailoverManager(const AutoFailoverConfig &config, std::shared_ptr< themisdb::replication::ReplicationManager > replication_mgr, std::shared_ptr< sharding::HealthMonitor > health_monitor, std::shared_ptr< sharding::HotSpareManager > spare_manager, std::shared_ptr< sharding::EpochFencingManager > fencing_manager)`
- Source: `include/failover/auto_failover_manager.h`:138
- Brief: Auto Failover Manager.
- Parameters:
  - `config` (const AutoFailoverConfig &): Input parameter.
  - `replication_mgr` (std::shared_ptr< themisdb::replication::ReplicationManager >): Input parameter.
  - `health_monitor` (std::shared_ptr< sharding::HealthMonitor >): Input parameter.
  - `spare_manager` (std::shared_ptr< sharding::HotSpareManager >): Input parameter.
  - `fencing_manager` (std::shared_ptr< sharding::EpochFencingManager >): Input parameter.
- Return: Return value.
- Details: config Input parameter. replication_mgr Input parameter. health_monitor Input parameter. spare_manager Input parameter. fencing_manager Input parameter. Return value.

#### `bool activateSpareIfNeeded(const std::string &failed_node_id)`
- Source: `include/failover/auto_failover_manager.h`:505
- Brief: Activate Spare If Needed.
- Parameters:
  - `failed_node_id` (const std::string &): Identifier of the failed node.
- Return: True when the operation succeeds.
- Details: failed_node_id Identifier of the failed node. True when the operation succeeds. failed_node_id Identifier of the failed node. True when the operation succeeds. Calls: spdlog::info(), emitEvent().

#### `bool attemptRecovery(const std::string &failed_node_id)`
- Source: `include/failover/auto_failover_manager.h`:553
- Brief: Attempt Recovery.
- Parameters:
  - `failed_node_id` (const std::string &): Identifier of the failed node.
- Return: True when the operation succeeds.
- Details: failed_node_id Identifier of the failed node. True when the operation succeeds. failed_node_id Identifier of the failed node. True when the operation succeeds. Calls: spdlog::info(), recovery_override_(), stats_lock(), emitDiagnostic(), waitForNodeRecovery(), updateFailureTracking(), emitEvent(), std::this_thread::sleep_for().

#### `bool canTransition(FailoverOrchestratorState from, FailoverOrchestratorState to) const`
- Source: `include/failover/auto_failover_manager.h`:260
- Brief: Can Transition.
- Parameters:
  - `from` (FailoverOrchestratorState): Input parameter.
  - `to` (FailoverOrchestratorState): Input parameter.
- Return: True when the operation succeeds.
- Details: from Input parameter. to Input parameter. True when the operation succeeds.

#### `TopologySnapshot captureTopologySnapshot() const`
- Source: `include/failover/auto_failover_manager.h`:406
- Brief: Capture Topology Snapshot.
- Parameters: none
- Return: Return value.
- Details: Return value.

#### `bool checkAndApplyGcGrace(const std::string &node_id)`
- Source: `include/failover/auto_failover_manager.h`:470
- Brief: Check And Apply Gc Grace.
- Parameters:
  - `node_id` (const std::string &): Identifier of the node.
- Return: True when the operation succeeds.
- Details: node_id Identifier of the node. True when the operation succeeds. node_id Identifier of the node. True when the operation succeeds. Calls: std::chrono::steady_clock::now(), lock(), spdlog::debug(), push_back(), erase(), std::remove_if(), begin(), end().

#### `bool checkAndWaitForQuorum()`
- Source: `include/failover/auto_failover_manager.h`:486
- Brief: Check And Wait For Quorum.
- Parameters: none
- Return: True when the operation succeeds.
- Details: True when the operation succeeds. True when the operation succeeds. Calls: spdlog::warn(), getConfig(), std::chrono::steady_clock::now(), hasQuorum(), append(), spdlog::error(), emitDiagnostic(), std::this_thread::sleep_for().

#### `void checkForNetworkPartitions()`
- Source: `include/failover/auto_failover_manager.h`:439
- Brief: Check For Network Partitions.
- Parameters: none
- Details: Calls: isNetworkPartitionedFromQuorum(), spdlog::warn(), emitEvent(), lock(), handleNetworkPartition().

#### `void detectNodeFailures()`
- Source: `include/failover/auto_failover_manager.h`:443
- Brief: Detect Node Failures.
- Parameters: none
- Details: Calls: lock(), captureTopologySnapshot(), getFailingNodes(), has_topology_change(), spdlog::warn(), triggerManualFailover().

#### `void emitDiagnostic(FailoverErrorCode code, const std::string &node_id, const std::string &detail) noexcept`
- Source: `include/failover/auto_failover_manager.h`:576
- Brief: Emit Diagnostic.
- Parameters:
  - `code` (FailoverErrorCode): Input parameter.
  - `node_id` (const std::string &): Identifier of the node.
  - `detail` (const std::string &): Input parameter.
- Details: code Input parameter. node_id Identifier of the node. detail Input parameter. Exception safety: noexcept.

#### `void emitEvent(FailoverEventType type, const std::string &node_id, const std::string &detail) noexcept`
- Source: `include/failover/auto_failover_manager.h`:587
- Brief: Emit Event.
- Parameters:
  - `type` (FailoverEventType): Input parameter.
  - `node_id` (const std::string &): Identifier of the node.
  - `detail` (const std::string &): Input parameter.
- Details: type Input parameter. node_id Identifier of the node. detail Input parameter. Exception safety: noexcept.

#### `void failoverLoop()`
- Source: `include/failover/auto_failover_manager.h`:475
- Brief: Failover Loop.
- Parameters: none
- Details: Calls: load(), lock(), wait_for(), std::chrono::seconds(), empty(), front(), pop(), stats_lock().

#### `AutoFailoverConfig getConfig() const`
- Source: `include/failover/auto_failover_manager.h`:203
- Brief: Get Config.
- Parameters: none
- Return: Return value.
- Details: Return value.

#### `std::vector< std::string > getFailingNodes() const`
- Source: `include/failover/auto_failover_manager.h`:186
- Brief: Get Failing Nodes.
- Parameters: none
- Return: Return value.
- Details: Return value.

#### `std::optional< FailoverResult > getLastFailoverResult() const`
- Source: `include/failover/auto_failover_manager.h`:191
- Brief: Get Last Failover Result.
- Parameters: none
- Return: Return value.
- Details: Return value.

#### `FailoverOrchestratorState getState() const`
- Source: `include/failover/auto_failover_manager.h`:176
- Brief: Get State.
- Parameters: none
- Return: Return value.
- Details: Return value.

#### `Statistics getStatistics() const`
- Source: `include/failover/auto_failover_manager.h`:235
- Brief: Return access control statistics.
- Parameters: none
- Return: Access control statistics.
- Details: Access control statistics.

#### `bool handleNetworkPartition()`
- Source: `include/failover/auto_failover_manager.h`:540
- Brief: Handle Network Partition.
- Parameters: none
- Return: True when the operation succeeds.
- Details: True when the operation succeeds. True when the operation succeeds. Calls: spdlog::warn(), isNetworkPartitionedFromQuorum(), spdlog::error().

#### `bool isFailoverInProgress() const`
- Source: `include/failover/auto_failover_manager.h`:181
- Brief: Is Failover In Progress.
- Parameters: none
- Return: True when the operation succeeds.
- Details: True when the operation succeeds.

#### `bool isNetworkPartitionedFromQuorum() const`
- Source: `include/failover/auto_failover_manager.h`:545
- Brief: Is Network Partitioned From Quorum.
- Parameters: none
- Return: True when the operation succeeds.
- Details: True when the operation succeeds.

#### `bool isRunning() const`
- Source: `include/failover/auto_failover_manager.h`:163
- Brief: Is Running.
- Parameters: none
- Return: True when the operation succeeds.
- Details: True when the operation succeeds.

#### `void monitoringLoop()`
- Source: `include/failover/auto_failover_manager.h`:431
- Brief: Monitoring Loop.
- Parameters: none
- Details: Calls: load(), performHealthChecks(), checkForNetworkPartitions(), detectNodeFailures(), std::this_thread::sleep_for(), spdlog::error(), what().

#### `bool performBoundedHealthCheck(const std::string &node_id) noexcept`
- Source: `include/failover/auto_failover_manager.h`:457
- Brief: Perform Bounded Health Check.
- Parameters:
  - `node_id` (const std::string &): Identifier of the node.
- Return: True when the operation succeeds.
- Details: node_id Identifier of the node. True when the operation succeeds. Exception safety: noexcept.

#### `void performHealthChecks()`
- Source: `include/failover/auto_failover_manager.h`:435
- Brief: Perform Health Checks.
- Parameters: none
- Details: Calls: valid(), wait_for(), std::chrono::seconds(), get(), spdlog::debug(), getConfig(), std::chrono::steady_clock::now(), std::async().

#### `bool preventSplitBrain(const std::string &failed_node_id)`
- Source: `include/failover/auto_failover_manager.h`:526
- Brief: Prevent Split Brain.
- Parameters:
  - `failed_node_id` (const std::string &): Identifier of the failed node.
- Return: True when the operation succeeds.
- Details: failed_node_id Identifier of the failed node. True when the operation succeeds. failed_node_id Identifier of the failed node. True when the operation succeeds. Calls: emitDiagnostic(), spdlog::error(), spdlog::info(), bumpEpoch(), lock().

#### `FailoverResult processFailover(const FailoverTask &task)`
- Source: `include/failover/auto_failover_manager.h`:481
- Brief: Process Failover.
- Parameters:
  - `task` (const FailoverTask &): Input parameter.
- Return: Return value.
- Details: task Input parameter. Return value. task Input parameter. Return value. Calls: std::chrono::steady_clock::now(), transitionState(), spdlog::info(), emitEvent(), checkAndWaitForQuorum(), spdlog::error(), preventSplitBrain(), selectAndPromoteReplica().

#### `void registerEventCallback(FailoverEventCallback callback)`
- Source: `include/failover/auto_failover_manager.h`:252
- Brief: Register Event Callback.
- Parameters:
  - `callback` (FailoverEventCallback): Input parameter.
- Details: callback Input parameter. callback Input parameter. Calls: lock(), push_back(), std::move().

#### `void resetStatistics()`
- Source: `include/failover/auto_failover_manager.h`:239
- Brief: Reset Statistics.
- Parameters: none
- Details: Calls: lock(), clear().

#### `std::string resolveSplitVote(const std::vector< std::string > &candidates) const`
- Source: `include/failover/auto_failover_manager.h`:533
- Brief: Resolve Split Vote.
- Parameters:
  - `candidates` (const std::vector< std::string > &): Input parameter.
- Return: Return value.
- Details: candidates Input parameter. Return value.

#### `bool selectAndPromoteReplica(const std::string &failed_node_id, std::string &promoted_id)`
- Source: `include/failover/auto_failover_manager.h`:499
- Brief: Select And Promote Replica.
- Parameters:
  - `failed_node_id` (const std::string &): Identifier of the failed node.
  - `promoted_id` (std::string &): Identifier of the promoted.
- Return: True when the operation succeeds.
- Details: failed_node_id Identifier of the failed node. promoted_id Identifier of the promoted. True when the operation succeeds. failed_node_id Identifier of the failed node. promoted_id Identifier of the promoted. True when the operation succeeds. Calls: getReplicas(), getReplicaHealthStatus(), push_back(), empty(), spdlog::error(), size(), getConfig(), front().

#### `bool start()`
- Source: `include/failover/auto_failover_manager.h`:153
- Brief: Start.
- Parameters: none
- Return: True when the operation succeeds.
- Details: True when the operation succeeds. True when the operation succeeds. Calls: exchange(), spdlog::warn(), transitionState(), std::thread(), spdlog::info(), spdlog::error(), what().

#### `bool startLeaderElection(const std::string &failed_node_id)`
- Source: `include/failover/auto_failover_manager.h`:492
- Brief: Start Leader Election.
- Parameters:
  - `failed_node_id` (const std::string &): Identifier of the failed node.
- Return: True when the operation succeeds.
- Details: failed_node_id Identifier of the failed node. True when the operation succeeds. failed_node_id Identifier of the failed node. True when the operation succeeds. Calls: transitionState(), emitEvent(), triggerFailover().

#### `bool stop()`
- Source: `include/failover/auto_failover_manager.h`:158
- Brief: Stop.
- Parameters: none
- Return: True when the operation succeeds.
- Details: True when the operation succeeds. True when the operation succeeds. Calls: exchange(), notify_all(), std::chrono::steady_clock::now(), std::chrono::seconds(), joinable(), count(), spdlog::warn(), join().

#### `void transitionState(FailoverOrchestratorState new_state)`
- Source: `include/failover/auto_failover_manager.h`:567
- Brief: Transition State.
- Parameters:
  - `new_state` (FailoverOrchestratorState): Input parameter.
- Details: new_state Input parameter. new_state Input parameter. Calls: exchange(), canTransition(), spdlog::warn(), spdlog::debug().

#### `bool triggerManualFailover(const std::string &failed_node_id, const std::string &target_promote_id="")`
- Source: `include/failover/auto_failover_manager.h`:166
- Brief: Trigger Manual Failover.
- Parameters:
  - `failed_node_id` (const std::string &): Identifier of the failed node.
  - `target_promote_id` (const std::string &): Identifier of the target promote.
- Return: True when the operation succeeds.
- Details: failed_node_id Identifier of the failed node. target_promote_id Identifier of the target promote. True when the operation succeeds. Calls: load(), spdlog::error(), lock(), config_lock(), size(), stats_lock(), std::chrono::steady_clock::now(), push().

#### `void updateAdaptiveInterval(std::chrono::milliseconds last_latency)`
- Source: `include/failover/auto_failover_manager.h`:463
- Brief: Update Adaptive Interval.
- Parameters:
  - `last_latency` (std::chrono::milliseconds): Input parameter.
- Details: last_latency Input parameter. last_latency Input parameter. Calls: lock(), push_back(), size(), erase(), begin(), std::sort(), end(), std::min().

#### `void updateConfig(const AutoFailoverConfig &config)`
- Source: `include/failover/auto_failover_manager.h`:198
- Brief: Update the access control configuration.
- Parameters:
  - `config` (const AutoFailoverConfig &): New access control configuration.
- Details: config New access control configuration. config New access control configuration. Calls: lock(), spdlog::info().

#### `void updateFailureTracking(const std::string &node_id, bool is_healthy)`
- Source: `include/failover/auto_failover_manager.h`:449
- Brief: Update Failure Tracking.
- Parameters:
  - `node_id` (const std::string &): Identifier of the node.
  - `is_healthy` (bool): Input parameter.
- Details: node_id Identifier of the node. is_healthy Input parameter. node_id Identifier of the node. is_healthy Input parameter. Calls: lock(), find(), end(), fetch_add(), checkAndApplyGcGrace(), emitEvent(), std::to_string().

#### `bool updateMetadata(const std::string &old_leader_id, const std::string &new_leader_id)`
- Source: `include/failover/auto_failover_manager.h`:512
- Brief: Update Metadata.
- Parameters:
  - `old_leader_id` (const std::string &): Identifier of the old leader.
  - `new_leader_id` (const std::string &): Identifier of the new leader.
- Return: True when the operation succeeds.
- Details: old_leader_id Identifier of the old leader. new_leader_id Identifier of the new leader. True when the operation succeeds. old_leader_id Identifier of the old leader. new_leader_id Identifier of the new leader. True when the operation succeeds. Calls: spdlog::info().

#### `void updateStatistics(const FailoverResult &result)`
- Source: `include/failover/auto_failover_manager.h`:592
- Brief: Update Statistics.
- Parameters:
  - `result` (const FailoverResult &): Input parameter.
- Details: result Input parameter. result Input parameter. Calls: push_back(), size(), erase(), begin(), empty(), std::accumulate(), end(), std::chrono::milliseconds().

#### `bool verifyFailoverCompletion(const FailoverTask &task)`
- Source: `include/failover/auto_failover_manager.h`:518
- Brief: Verify Failover Completion.
- Parameters:
  - `task` (const FailoverTask &): Input parameter.
- Return: True when the operation succeeds.
- Details: task Input parameter. True when the operation succeeds. task Input parameter. True when the operation succeeds. Calls: spdlog::info(), hasQuorum().

#### `bool waitForNodeRecovery(const std::string &node_id, uint32_t max_attempts)`
- Source: `include/failover/auto_failover_manager.h`:560
- Brief: Wait For Node Recovery.
- Parameters:
  - `node_id` (const std::string &): Identifier of the node.
  - `max_attempts` (uint32_t): Input parameter.
- Return: True when the operation succeeds.
- Details: node_id Identifier of the node. max_attempts Input parameter. True when the operation succeeds. node_id Identifier of the node. max_attempts Input parameter. True when the operation succeeds. Calls: getHealthStatus(), isHealthy(), std::this_thread::sleep_for(), std::chrono::milliseconds().

#### `~AutoFailoverManager()`
- Source: `include/failover/auto_failover_manager.h`:146
- Brief: n/a
- Parameters: none

### themis::failover::DisasterRecoveryManager

#### `DisasterRecoveryManager(DisasterRecoveryConfig config, std::shared_ptr< themisdb::replication::ReplicationManager > replication_mgr, std::shared_ptr< sharding::EpochFencingManager > fencing_mgr)`
- Source: `include/failover/disaster_recovery_manager.h`:106
- Brief: Disaster Recovery Manager.
- Parameters:
  - `config` (DisasterRecoveryConfig): Input parameter.
  - `replication_mgr` (std::shared_ptr< themisdb::replication::ReplicationManager >): Input parameter.
  - `fencing_mgr` (std::shared_ptr< sharding::EpochFencingManager >): Input parameter.
- Return: Return value.
- Details: config Input parameter. replication_mgr Input parameter. fencing_mgr Input parameter. Return value.

#### `bool applyEpochFencing(const DisasterRecoveryPlan &plan, std::string &detail, uint64_t &fenced_epoch)`
- Source: `include/failover/disaster_recovery_manager.h`:215
- Brief: Apply Epoch Fencing.
- Parameters:
  - `plan` (const DisasterRecoveryPlan &): Input parameter.
  - `detail` (std::string &): Input/output parameter.
  - `fenced_epoch` (uint64_t &): Input/output parameter.
- Return: True when the operation succeeds.
- Details: plan Input parameter. detail Input/output parameter. fenced_epoch Input/output parameter. True when the operation succeeds. plan Input parameter. detail Input/output parameter. fenced_epoch Input/output parameter. True when the operation succeeds. Calls: bumpEpoch(), std::to_string().

#### `void clearStepHooks()`
- Source: `include/failover/disaster_recovery_manager.h`:135
- Brief: Clear Step Hooks.
- Parameters: none
- Details: Calls: clear().

#### `DisasterRecoveryResult executePlan(const DisasterRecoveryPlan &plan)`
- Source: `include/failover/disaster_recovery_manager.h`:116
- Brief: Execute Plan.
- Parameters:
  - `plan` (const DisasterRecoveryPlan &): Input parameter.
- Return: Return value.
- Details: plan Input parameter. Return value. plan Input parameter. Return value. Calls: idem_lock(), find(), end(), spdlog::info(), exec_lock(), owns_lock(), spdlog::error(), std::chrono::steady_clock::now().

#### `DisasterRecoveryState getState() const noexcept`
- Source: `include/failover/disaster_recovery_manager.h`:142
- Brief: Get State.
- Parameters: none
- Return: Return value.
- Details: Return value. Exception safety: noexcept.

#### `Statistics getStatistics() const`
- Source: `include/failover/disaster_recovery_manager.h`:156
- Brief: Return access control statistics.
- Parameters: none
- Return: Access control statistics.
- Details: Access control statistics.

#### `bool runPrechecks(const DisasterRecoveryPlan &plan, std::string &detail)`
- Source: `include/failover/disaster_recovery_manager.h`:200
- Brief: Run Prechecks.
- Parameters:
  - `plan` (const DisasterRecoveryPlan &): Input parameter.
  - `detail` (std::string &): Input/output parameter.
- Return: True when the operation succeeds.
- Details: plan Input parameter. detail Input/output parameter. True when the operation succeeds. plan Input parameter. detail Input/output parameter. True when the operation succeeds. Calls: hasQuorum(), getClusterHealth(), std::any_of(), begin(), end().

#### `bool runRestore(const DisasterRecoveryPlan &plan, std::string &detail)`
- Source: `include/failover/disaster_recovery_manager.h`:222
- Brief: Run Restore.
- Parameters:
  - `plan` (const DisasterRecoveryPlan &): Input parameter.
  - `detail` (std::string &): Input/output parameter.
- Return: True when the operation succeeds.
- Details: plan Input parameter. detail Input/output parameter. True when the operation succeeds. plan Input parameter. detail Input/output parameter. True when the operation succeeds. Implements runRestore without additional internal calls.

#### `bool runStep(DisasterRecoveryStep step, DisasterRecoveryState state, const DisasterRecoveryPlan &plan, DisasterRecoveryResult &result, std::string &error, uint64_t &fenced_epoch)`
- Source: `include/failover/disaster_recovery_manager.h`:187
- Brief: Run Step.
- Parameters:
  - `step` (DisasterRecoveryStep): Input parameter.
  - `state` (DisasterRecoveryState): Input parameter.
  - `plan` (const DisasterRecoveryPlan &): Input parameter.
  - `result` (DisasterRecoveryResult &): Input/output parameter.
  - `error` (std::string &): Input/output parameter.
  - `fenced_epoch` (uint64_t &): Input/output parameter.
- Return: True when the operation succeeds.
- Details: step Input parameter. state Input parameter. plan Input parameter. result Input/output parameter. error Input/output parameter. fenced_epoch Input/output parameter. True when the operation succeeds. step Input parameter. state Input parameter. plan Input parameter. result Input/output parameter. error Input/output parameter. fenced_epoch Input/output parameter. True when the operation succeeds. Calls: transitionState(), find(), end(), second(), runPrechecks(), validateSnapshot(), applyEpochFencing(), runRestore().

#### `void setStepHook(DisasterRecoveryStep step, StepHook hook)`
- Source: `include/failover/disaster_recovery_manager.h`:131
- Brief: Set Step Hook.
- Parameters:
  - `step` (DisasterRecoveryStep): Input parameter.
  - `hook` (StepHook): Input parameter.
- Details: step Input parameter. hook Input parameter. step Input parameter. hook Input parameter. Calls: std::move().

#### `bool shiftTraffic(const DisasterRecoveryPlan &plan, std::string &detail)`
- Source: `include/failover/disaster_recovery_manager.h`:236
- Brief: Shift Traffic.
- Parameters:
  - `plan` (const DisasterRecoveryPlan &): Input parameter.
  - `detail` (std::string &): Input/output parameter.
- Return: True when the operation succeeds.
- Details: plan Input parameter. detail Input/output parameter. True when the operation succeeds. plan Input parameter. detail Input/output parameter. True when the operation succeeds. Implements shiftTraffic without additional internal calls.

#### `void transitionState(DisasterRecoveryState next) noexcept`
- Source: `include/failover/disaster_recovery_manager.h`:250
- Brief: Transition State.
- Parameters:
  - `next` (DisasterRecoveryState): Input parameter.
- Details: next Input parameter. Exception safety: noexcept.

#### `void updateStatistics(const DisasterRecoveryResult &result)`
- Source: `include/failover/disaster_recovery_manager.h`:255
- Brief: Update Statistics.
- Parameters:
  - `result` (const DisasterRecoveryResult &): Input parameter.
- Details: result Input parameter. result Input parameter. Calls: lock(), push_back(), std::accumulate(), begin(), end(), std::chrono::milliseconds(), size().

#### `bool validatePlan(const DisasterRecoveryPlan &plan, std::string &error) const`
- Source: `include/failover/disaster_recovery_manager.h`:124
- Brief: Validate Plan.
- Parameters:
  - `plan` (const DisasterRecoveryPlan &): Input parameter.
  - `error` (std::string &): Input/output parameter.
- Return: True when the operation succeeds.
- Details: plan Input parameter. error Input/output parameter. True when the operation succeeds.

#### `bool validateSnapshot(const DisasterRecoveryPlan &plan, std::string &detail)`
- Source: `include/failover/disaster_recovery_manager.h`:207
- Brief: Validate Snapshot.
- Parameters:
  - `plan` (const DisasterRecoveryPlan &): Input parameter.
  - `detail` (std::string &): Input/output parameter.
- Return: True when the operation succeeds.
- Details: plan Input parameter. detail Input/output parameter. True when the operation succeeds. plan Input parameter. detail Input/output parameter. True when the operation succeeds. Calls: empty().

#### `bool verifyRecoveredState(const DisasterRecoveryPlan &plan, std::string &detail)`
- Source: `include/failover/disaster_recovery_manager.h`:243
- Brief: Verify Recovered State.
- Parameters:
  - `plan` (const DisasterRecoveryPlan &): Input parameter.
  - `detail` (std::string &): Input/output parameter.
- Return: True when the operation succeeds.
- Details: plan Input parameter. detail Input/output parameter. True when the operation succeeds. plan Input parameter. detail Input/output parameter. True when the operation succeeds. Calls: getClusterHealth(), std::any_of(), begin(), end(), hasQuorum(), std::this_thread::sleep_for().

#### `bool waitForCatchup(const DisasterRecoveryPlan &plan, std::string &detail)`
- Source: `include/failover/disaster_recovery_manager.h`:229
- Brief: Wait For Catchup.
- Parameters:
  - `plan` (const DisasterRecoveryPlan &): Input parameter.
  - `detail` (std::string &): Input/output parameter.
- Return: True when the operation succeeds.
- Details: plan Input parameter. detail Input/output parameter. True when the operation succeeds. plan Input parameter. detail Input/output parameter. True when the operation succeeds. Calls: std::chrono::steady_clock::now(), hasQuorum(), std::this_thread::sleep_for(), std::chrono::milliseconds().

### themis::failover::DisasterRecoveryManager::EnumHash

#### `size_t operator()(T t) const noexcept`
- Source: `include/failover/disaster_recovery_manager.h`:172
- Brief: n/a
- Parameters:
  - `t` (T): n/a

### themis::failover::QuorumLog

#### `QuorumLog(const QuorumLog &)=delete`
- Source: `include/failover/quorum_log.h`:38
- Brief: n/a
- Parameters:
  - `<unnamed>` (const QuorumLog &): n/a

#### `QuorumLog(std::filesystem::path log_path)`
- Source: `include/failover/quorum_log.h`:35
- Brief: Quorum Log.
- Parameters:
  - `log_path` (std::filesystem::path): Path to the log.
- Return: Return value.
- Details: log_path Path to the log. Return value.

#### `bool append(uint64_t epoch, const std::string &node_id, const std::string &decision)`
- Source: `include/failover/quorum_log.h`:48
- Brief: Append.
- Parameters:
  - `epoch` (uint64_t): Input parameter.
  - `node_id` (const std::string &): Identifier of the node.
  - `decision` (const std::string &): Input parameter.
- Return: True when the operation succeeds.
- Details: epoch Input parameter. node_id Identifier of the node. decision Input parameter. True when the operation succeeds. epoch Input parameter. node_id Identifier of the node. decision Input parameter. True when the operation succeeds. Calls: std::chrono::system_clock::now(), time_since_epoch(), count(), computeCrc32(), find(), spdlog::error(), ofs(), string().

#### `uint32_t computeCrc32(uint64_t epoch, const std::string &node_id, const std::string &decision, int64_t ts_ms) noexcept`
- Source: `include/failover/quorum_log.h`:68
- Brief: Compute Crc32.
- Parameters:
  - `epoch` (uint64_t): Input parameter.
  - `node_id` (const std::string &): Identifier of the node.
  - `decision` (const std::string &): Input parameter.
  - `ts_ms` (int64_t): Input parameter.
- Return: Return value.
- Details: epoch Input parameter. node_id Identifier of the node. decision Input parameter. ts_ms Input parameter. Return value. Exception safety: noexcept.

#### `QuorumLog & operator=(const QuorumLog &)=delete`
- Source: `include/failover/quorum_log.h`:39
- Brief: n/a
- Parameters:
  - `<unnamed>` (const QuorumLog &): n/a

#### `const std::filesystem::path & path() const noexcept`
- Source: `include/failover/quorum_log.h`:56
- Brief: n/a
- Parameters: none

#### `QuorumState recover() const`
- Source: `include/failover/quorum_log.h`:54
- Brief: Recover.
- Parameters: none
- Return: Return value.
- Details: Return value.

#### `~QuorumLog()=default`
- Source: `include/failover/quorum_log.h`:36
- Brief: n/a
- Parameters: none

### themis::failover::TopologySnapshot

#### `std::vector< std::string > added_nodes(const TopologySnapshot &other) const`
- Source: `include/failover/topology_snapshot.h`:20
- Brief: Added nodes.
- Parameters:
  - `other` (const TopologySnapshot &): Input parameter.
- Return: Return value.
- Details: other Input parameter. Return value.

#### `TopologySnapshot capture(uint64_t version, const std::unordered_map< std::string, int > &failures)`
- Source: `include/failover/topology_snapshot.h`:36
- Brief: n/a
- Parameters:
  - `version` (uint64_t): n/a
  - `failures` (const std::unordered_map< std::string, int > &): n/a

#### `bool has_topology_change(const TopologySnapshot &other) const`
- Source: `include/failover/topology_snapshot.h`:34
- Brief: Has topology change.
- Parameters:
  - `other` (const TopologySnapshot &): Input parameter.
- Return: True when the operation succeeds.
- Details: other Input parameter. True when the operation succeeds.

#### `std::vector< std::string > removed_nodes(const TopologySnapshot &other) const`
- Source: `include/failover/topology_snapshot.h`:27
- Brief: Removed nodes.
- Parameters:
  - `other` (const TopologySnapshot &): Input parameter.
- Return: Return value.
- Details: other Input parameter. Return value.

### themis::failover::test

#### `TEST(FailoverChaosScenarios, ConcurrentTriggersSafe)`
- Source: `tests/failover/test_failover_chaos_scenarios.cpp`:355
- Brief: n/a
- Parameters:
  - `<unnamed>` (FailoverChaosScenarios): n/a
  - `<unnamed>` (ConcurrentTriggersSafe): n/a

#### `TEST(FailoverChaosScenarios, ConfigHotUpdateWhileRunning)`
- Source: `tests/failover/test_failover_chaos_scenarios.cpp`:245
- Brief: n/a
- Parameters:
  - `<unnamed>` (FailoverChaosScenarios): n/a
  - `<unnamed>` (ConfigHotUpdateWhileRunning): n/a

#### `TEST(FailoverChaosScenarios, DropCountDoesNotAffectTotalFailovers)`
- Source: `tests/failover/test_failover_chaos_scenarios.cpp`:195
- Brief: n/a
- Parameters:
  - `<unnamed>` (FailoverChaosScenarios): n/a
  - `<unnamed>` (DropCountDoesNotAffectTotalFailovers): n/a

#### `TEST(FailoverChaosScenarios, EventCallbacksReceiveNodeFailureEvents)`
- Source: `tests/failover/test_failover_chaos_scenarios.cpp`:265
- Brief: n/a
- Parameters:
  - `<unnamed>` (FailoverChaosScenarios): n/a
  - `<unnamed>` (EventCallbacksReceiveNodeFailureEvents): n/a

#### `TEST(FailoverChaosScenarios, GetFailingNodesInitiallyEmpty)`
- Source: `tests/failover/test_failover_chaos_scenarios.cpp`:312
- Brief: n/a
- Parameters:
  - `<unnamed>` (FailoverChaosScenarios): n/a
  - `<unnamed>` (GetFailingNodesInitiallyEmpty): n/a

#### `TEST(FailoverChaosScenarios, IdempotentDoubleStart)`
- Source: `tests/failover/test_failover_chaos_scenarios.cpp`:58
- Brief: n/a
- Parameters:
  - `<unnamed>` (FailoverChaosScenarios): n/a
  - `<unnamed>` (IdempotentDoubleStart): n/a

#### `TEST(FailoverChaosScenarios, IdempotentDoubleStop)`
- Source: `tests/failover/test_failover_chaos_scenarios.cpp`:69
- Brief: n/a
- Parameters:
  - `<unnamed>` (FailoverChaosScenarios): n/a
  - `<unnamed>` (IdempotentDoubleStop): n/a

#### `TEST(FailoverChaosScenarios, LastFailoverResultNullBeforeAnyFailover)`
- Source: `tests/failover/test_failover_chaos_scenarios.cpp`:320
- Brief: n/a
- Parameters:
  - `<unnamed>` (FailoverChaosScenarios): n/a
  - `<unnamed>` (LastFailoverResultNullBeforeAnyFailover): n/a

#### `TEST(FailoverChaosScenarios, LastFailoverResultPopulatedAfterFailover)`
- Source: `tests/failover/test_failover_chaos_scenarios.cpp`:326
- Brief: n/a
- Parameters:
  - `<unnamed>` (FailoverChaosScenarios): n/a
  - `<unnamed>` (LastFailoverResultPopulatedAfterFailover): n/a

#### `TEST(FailoverChaosScenarios, MultipleCallbacksAreAllInvoked)`
- Source: `tests/failover/test_failover_chaos_scenarios.cpp`:287
- Brief: n/a
- Parameters:
  - `<unnamed>` (FailoverChaosScenarios): n/a
  - `<unnamed>` (MultipleCallbacksAreAllInvoked): n/a

#### `TEST(FailoverChaosScenarios, QueueDepthTracked)`
- Source: `tests/failover/test_failover_chaos_scenarios.cpp`:118
- Brief: n/a
- Parameters:
  - `<unnamed>` (FailoverChaosScenarios): n/a
  - `<unnamed>` (QueueDepthTracked): n/a

#### `TEST(FailoverChaosScenarios, QueuePressureEventEmittedAtThreshold)`
- Source: `tests/failover/test_failover_chaos_scenarios.cpp`:139
- Brief: n/a
- Parameters:
  - `<unnamed>` (FailoverChaosScenarios): n/a
  - `<unnamed>` (QueuePressureEventEmittedAtThreshold): n/a

#### `TEST(FailoverChaosScenarios, QueueSaturationDropsTasks)`
- Source: `tests/failover/test_failover_chaos_scenarios.cpp`:92
- Brief: n/a
- Parameters:
  - `<unnamed>` (FailoverChaosScenarios): n/a
  - `<unnamed>` (QueueSaturationDropsTasks): n/a

#### `TEST(FailoverChaosScenarios, RapidEnqueueDrainCycle)`
- Source: `tests/failover/test_failover_chaos_scenarios.cpp`:221
- Brief: n/a
- Parameters:
  - `<unnamed>` (FailoverChaosScenarios): n/a
  - `<unnamed>` (RapidEnqueueDrainCycle): n/a

#### `TEST(FailoverChaosScenarios, StartStopMultipleCycles)`
- Source: `tests/failover/test_failover_chaos_scenarios.cpp`:78
- Brief: n/a
- Parameters:
  - `<unnamed>` (FailoverChaosScenarios): n/a
  - `<unnamed>` (StartStopMultipleCycles): n/a

#### `TEST(FailoverChaosScenarios, StateIsIdleAfterStop)`
- Source: `tests/failover/test_failover_chaos_scenarios.cpp`:343
- Brief: n/a
- Parameters:
  - `<unnamed>` (FailoverChaosScenarios): n/a
  - `<unnamed>` (StateIsIdleAfterStop): n/a

#### `TEST(FailoverChaosScenarios, StatisticsAfterMultipleFailoverCycles)`
- Source: `tests/failover/test_failover_chaos_scenarios.cpp`:172
- Brief: n/a
- Parameters:
  - `<unnamed>` (FailoverChaosScenarios): n/a
  - `<unnamed>` (StatisticsAfterMultipleFailoverCycles): n/a

#### `TEST(TopologyVersioning, CaptureEmptyMapGivesEmptySnapshot)`
- Source: `tests/failover/test_failover_wave_a_topology_versioning.cpp`:247
- Brief: n/a
- Parameters:
  - `<unnamed>` (TopologyVersioning): n/a
  - `<unnamed>` (CaptureEmptyMapGivesEmptySnapshot): n/a

#### `TEST(TopologyVersioning, CapturePreservesFailureCounts)`
- Source: `tests/failover/test_failover_wave_a_topology_versioning.cpp`:255
- Brief: n/a
- Parameters:
  - `<unnamed>` (TopologyVersioning): n/a
  - `<unnamed>` (CapturePreservesFailureCounts): n/a

#### `TEST(TopologyVersioning, CaptureProducesSortedDeterministicNodeList)`
- Source: `tests/failover/test_failover_wave_a_topology_versioning.cpp`:224
- Brief: n/a
- Parameters:
  - `<unnamed>` (TopologyVersioning): n/a
  - `<unnamed>` (CaptureProducesSortedDeterministicNodeList): n/a

#### `TEST(TopologyVersioning, RaceDetectTriggersRetry)`
- Source: `tests/failover/test_failover_wave_a_topology_versioning.cpp`:194
- Brief: n/a
- Parameters:
  - `<unnamed>` (TopologyVersioning): n/a
  - `<unnamed>` (RaceDetectTriggersRetry): n/a

#### `TEST(TopologyVersioning, SnapshotDiffDetectsAddedNodes)`
- Source: `tests/failover/test_failover_wave_a_topology_versioning.cpp`:134
- Brief: n/a
- Parameters:
  - `<unnamed>` (TopologyVersioning): n/a
  - `<unnamed>` (SnapshotDiffDetectsAddedNodes): n/a

#### `TEST(TopologyVersioning, SnapshotDiffDetectsRemovedNodes)`
- Source: `tests/failover/test_failover_wave_a_topology_versioning.cpp`:151
- Brief: n/a
- Parameters:
  - `<unnamed>` (TopologyVersioning): n/a
  - `<unnamed>` (SnapshotDiffDetectsRemovedNodes): n/a

#### `TEST(TopologyVersioning, SnapshotSameDifferentVersionStillNoSetChange)`
- Source: `tests/failover/test_failover_wave_a_topology_versioning.cpp`:182
- Brief: n/a
- Parameters:
  - `<unnamed>` (TopologyVersioning): n/a
  - `<unnamed>` (SnapshotSameDifferentVersionStillNoSetChange): n/a

#### `TEST(TopologyVersioning, SnapshotSameNoChange)`
- Source: `tests/failover/test_failover_wave_a_topology_versioning.cpp`:171
- Brief: n/a
- Parameters:
  - `<unnamed>` (TopologyVersioning): n/a
  - `<unnamed>` (SnapshotSameNoChange): n/a

#### `TEST(TopologyVersioning, VersionIncrementOnNewNode)`
- Source: `tests/failover/test_failover_wave_a_topology_versioning.cpp`:59
- Brief: n/a
- Parameters:
  - `<unnamed>` (TopologyVersioning): n/a
  - `<unnamed>` (VersionIncrementOnNewNode): n/a

#### `TEST(TopologyVersioning, VersionIncrementSequential)`
- Source: `tests/failover/test_failover_wave_a_topology_versioning.cpp`:103
- Brief: n/a
- Parameters:
  - `<unnamed>` (TopologyVersioning): n/a
  - `<unnamed>` (VersionIncrementSequential): n/a

