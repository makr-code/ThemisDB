# Phase 4 Test Expansion — Detailed Test Listing

## Summary Statistics

| Test File | Total Tests | Phase 4 New | Coverage Area |
|-----------|------------|-----------|----------------|
| query_planner_test.cc | 54 | 8 | Category C enforcement, FallbackReason taxonomy, tensor freshness |
| retrieval_metrics_test.cc | 58 | 11 | MetricErrorKind paths, numeric validation, NaN/±Inf detection |
| approximation_rules_test.cc | 48 | 6 | ApproximationZone progression, Category C denial, policy versioning |
| artifact_lifecycle_test.cc | 54 | 10 | FAILED state machine, overlapping staleness thresholds, InvalidationReason |
| **TOTAL** | **214** | **35** | **Phase 3 error handling & fail-closed semantics** |

---

## query_planner_test.cc — 8 New Tests

### 1. CategoryC_BlocksGpuDispatchEvenWhenAllGatesPass
**Lines:** ~18
**Preconditions:**
- `query_kernel_category = KernelCategory::C` (ACL/provenance/transaction)
- All eligibility gates enabled (CUDA available, parity validated, etc.)
- Fresh tensor artifact available

**Test Logic:**
1. Create planner with Category C kernel classification
2. Call `selectPath()` with all gates passing
3. Assert `uses_gpu = false` (GPU dispatch blocked)
4. Assert `fallback_reason = CategoryCSubpathDetected`

**Validates:** Category C operations never route to GPU (fail-closed enforcement)

---

### 2. TensorArtifactStale_ExactFallbackReason
**Lines:** ~14
**Preconditions:**
- Stale tensor artifact: `artifact_age_ms = 10'000 > max_staleness_ms = 5'000`
- Full eligibility enabled

**Test Logic:**
1. Create stale tensor artifact
2. Call `selectPath()`
3. Assert `fallback_reason = FallbackReason::TensorArtifactStale`

**Validates:** Tensor age threshold enforcement with correct FallbackReason

---

### 3. TensorDeltaLagExceeds_TriggersRebuild
**Lines:** ~16
**Preconditions:**
- Fresh artifact (age < threshold)
- High delta lag: `delta_lag = 2000 > threshold = 1000`

**Test Logic:**
1. Create fresh artifact with high delta lag
2. Call `selectPath()`
3. Assert `path != AnnTensorSummary && path != AnnTensorExactGraph`

**Validates:** Delta lag threshold blocks tensor-based paths

---

### 4. TensorResidualLow_BlocksTensorPath
**Lines:** ~16
**Preconditions:**
- Fresh artifact (age < threshold)
- Low residual: `residual_threshold = 0.90 < minimum = 0.95`

**Test Logic:**
1. Create artifact with low residual quality metric
2. Call `selectPath()`
3. Assert tensor paths (2, 3) are not selected

**Validates:** Quality gate blocks low-quality tensor artifacts

---

### 5. TensorRankCapExceeded_FallsBackToExact
**Lines:** ~16
**Preconditions:**
- Fresh artifact with rank cap: `rank_cap = 1500 > limit = 1000`

**Test Logic:**
1. Create artifact exceeding rank cap threshold
2. Call `selectPath()`
3. Assert `fallback_reason = FallbackReason::TensorRankCapExceeded`

**Validates:** Rank cap policy enforcement with correct FallbackReason

---

### 6. TensorRebuildInProgress_BlocksTensorPath
**Lines:** ~16
**Preconditions:**
- Fresh artifact with `rebuild_in_progress = true`

**Test Logic:**
1. Create artifact with active rebuild
2. Call `selectPath()`
3. Assert `fallback_reason = FallbackReason::TensorRebuildInProgress`

**Validates:** Active rebuilds block tensor-based paths

---

### 7. FallbackChain_GpuErrorFallsBackToCpu
**Lines:** ~14
**Preconditions:**
- `gpu_parity_validated = false` (GPU parity check failure)
- Fresh artifact available

**Test Logic:**
1. Create eligibility with GPU parity failure
2. Call `selectPath()`
3. Assert `uses_gpu = false`

**Validates:** GPU failures trigger CPU fallback (fallback chain semantics)

---

### 8. NoSilentFallback_AllFallbacksHaveReason
**Lines:** ~26
**Preconditions:**
- Multiple fallback scenarios: stale artifact, missing artifact

**Test Logic:**
1. Create multiple stale/invalid scenarios
2. For each, call `selectPath()`
3. Assert that if `path != AnnOnly`, then `fallback_reason != None`

**Validates:** ADR E2-005 enforcement — no silent fallback

---

## retrieval_metrics_test.cc — 11 New Tests

### 1. Phase4_ThrowsOnNaNInScores
**Lines:** ~11
**Preconditions:**
- Score contains `std::nan("")`

**Test Logic:**
1. Create ranked results with NaN score
2. Call `computeRetrievalQuality()`
3. Assert `MetricError` is thrown

**Validates:** MetricErrorKind::NonFiniteInput detection

---

### 2. Phase4_ThrowsOnInfinityInScores
**Lines:** ~11
**Preconditions:**
- Score contains `INFINITY`

**Test Logic:**
1. Create ranked results with ±Inf score
2. Call `computeRetrievalQuality()`
3. Assert `MetricError` is thrown

**Validates:** ±Inf detection in numeric validation

---

### 3. Phase4_ThrowsOnZeroK
**Lines:** ~10
**Preconditions:**
- `k = 0` (invalid parameter)

**Test Logic:**
1. Call `computeRetrievalQuality(..., k=0)`
2. Assert `MetricError` with `kind = InvalidK`

**Validates:** Invalid k rejection (k must be > 0)

---

### 4. Phase4_ThrowsOnNegativeCounts
**Lines:** ~10
**Preconditions:**
- `groundtruth_count = -1`

**Test Logic:**
1. Call `computeLlmAnswerQuality(-1, 10)`
2. Assert `MetricError` with `kind = InvalidRange`

**Validates:** Negative count rejection

---

### 5. Phase4_ThrowsOnNegativeSupportTokens
**Lines:** ~10
**Preconditions:**
- `support_tokens = -1`

**Test Logic:**
1. Call `computeLlmAnswerQuality(5, 10, -1, 100)`
2. Assert `MetricError` with `kind = InvalidRange`

**Validates:** Negative token count validation

---

### 6. Phase4_ThrowsOnZeroCompressedSize
**Lines:** ~10
**Preconditions:**
- `compressed_size = 0`

**Test Logic:**
1. Call `computeCompressionMetrics(1000, 0, {0.01})`
2. Assert `MetricError` thrown

**Validates:** Division-by-zero prevention

---

### 7. Phase4_ThrowsOnNegativeResidual
**Lines:** ~12
**Preconditions:**
- `residual_error = -0.01` (invalid)

**Test Logic:**
1. Create snapshot with negative residual
2. Call `computeTensorGraphRuntimeMetrics({s})`
3. Assert `MetricError` thrown

**Validates:** Residual must be ≥ 0

---

### 8. Phase4_ThrowsOnEmptySnapshotInSummarize
**Lines:** ~10
**Preconditions:**
- No snapshots recorded in collector

**Test Logic:**
1. Create empty MetricCollector
2. Call `summarizeTensorGraph()`
3. Assert `MetricError` with `kind = EmptyGroundTruth`

**Validates:** Empty metric collection prevention

---

### 9. Phase4_ThrowsOnDoubleCountedInMultipleStrata
**Lines:** ~12
**Preconditions:**
- Ground truth contains duplicate entry: `{"a", "b", "a"}`

**Test Logic:**
1. Create ranked results
2. Call `computeRetrievalQuality()` with duplicated GT
3. Assert `MetricError` with `kind = DuplicateEntries`

**Validates:** Double-counted item detection

---

### 10. Phase4_ThrowsOnEmptyRequiredEvidence
**Lines:** ~10
**Preconditions:**
- Empty required evidence set

**Test Logic:**
1. Call `computeEvidenceQuality({"e1"}, {})`
2. Assert `MetricError` thrown

**Validates:** Empty required evidence rejection

---

### 11. Phase4_ResidualThresholdBoundaryConditions
**Lines:** ~16
**Preconditions:**
- `residual_error = 0.10`

**Test Logic:**
1. Call with threshold = 0.10 (exactly at threshold) → Assert throws
2. Call with threshold = 0.101 (just below) → Assert succeeds
3. Verify mean residual matches expected value

**Validates:** Boundary conditions for residual threshold enforcement

---

## approximation_rules_test.cc — 6 New Tests

### 1. Phase4_ApproximationZoneProgression
**Lines:** ~15
**Preconditions:**
- Engine initialized with default policy

**Test Logic:**
1. Query canonical boundary for ANN → Assert zone = Approximate
2. Query canonical boundary for TensorSummary → Assert zone = Bounded
3. Query canonical boundary for ExactGraph → Assert zone = Exact

**Validates:** ApproximationZone progression (Approximate→Bounded→Exact)

---

### 2. Phase4_CategoryC_DenyOnAllLayers
**Lines:** ~22
**Preconditions:**
- Category C kernel classification

**Test Logic:**
1. For each layer (Ann, TensorSummary, ExactGraph):
   - Call `checkBoundary()` with KernelCategory::C
   - Assert `decision = Deny`
   - Assert `violation = CategoryCSubpathDetected`

**Validates:** Category C denied on all layers

---

### 3. Phase4_CategoryC_NoTruthBearingDegradation
**Lines:** ~16
**Preconditions:**
- Category C kernel, ExactGraph layer, Exact zone

**Test Logic:**
1. Call `checkBoundary(ExactGraph, Exact, C, ...)`
2. Assert `decision = Deny` (even in Exact zone)

**Validates:** Category C never bypasses governance on truth-bearing layer

---

### 4. Phase4_PolicyVersionTracking
**Lines:** ~30
**Preconditions:**
- Two distinct policies with different versions

**Test Logic:**
1. Create policy v1 (conservative) and v2 (permissive with bypass)
2. Call `checkBoundary()` with each policy
3. Assert policies are distinct in version
4. Verify decisions may differ based on bypass setting

**Validates:** Policy version tracking for audit trails

---

### 5. Phase4_PathProgressionFromAnnToExact
**Lines:** ~18
**Preconditions:**
- Path 3 (AnnTensorExactGraph) with proper routing

**Test Logic:**
1. Create decision for Path 3
2. Call `validatePlannedPath()`
3. Assert decision is allowed or escalates (respects progression)

**Validates:** Path progression semantics (ANN→Tensor→Exact)

---

### 6. Phase4_ConfidenceThresholdUniformity
**Lines:** ~30
**Preconditions:**
- Low confidence policy (threshold = 0.30)
- High confidence policy (threshold = 0.90)
- Test confidence = 0.50

**Test Logic:**
1. With low threshold:
   - Call `checkBoundary(..., confidence=0.50)`
   - Assert `decision = Allow`
2. With high threshold:
   - Call `checkBoundary(..., confidence=0.50)`
   - Assert `decision = EscalateToExact`

**Validates:** Confidence thresholds enforced uniformly

---

## artifact_lifecycle_test.cc — 10 New Tests

### 1. Phase4_FailedState_IsUsableForPlanningFalse
**Lines:** ~5
**Preconditions:**
- State = FAILED

**Test Logic:**
1. Assert `isUsableForPlanning(FAILED) = false`

**Validates:** FAILED state is never usable for planning

---

### 2. Phase4_RebuildAttemptCountIncrementsMonotonically
**Lines:** ~12
**Preconditions:**
- Artifact with `rebuild_attempt_count = 3`

**Test Logic:**
1. Call `beginRebuild()` on FAILED artifact
2. Assert `state = REBUILDING`
3. Assert `rebuild_attempt_count = 4` (incremented)

**Validates:** Monotonic rebuild attempt tracking

---

### 3. Phase4_FailedToRebuildingTransition
**Lines:** ~12
**Preconditions:**
- State = FAILED

**Test Logic:**
1. Call `beginRebuild()`
2. Assert `state = REBUILDING`
3. Assert timestamp updated

**Validates:** FAILED → REBUILDING transition

---

### 4. Phase4_OverlappingStalenessThresholds_AgeAndDeltaLag
**Lines:** ~16
**Preconditions:**
- Age = 3000 > threshold 2000 (STALE)
- Delta lag = 2000 > threshold 1000 (STALE)

**Test Logic:**
1. Create policy with both thresholds
2. Call `computeState()`
3. Assert `state = STALE`
4. Assert `diagnoseStalenessCause()` returns non-empty

**Validates:** Multiple overlapping staleness thresholds trigger STALE

---

### 5. Phase4_OverlappingStalenessThresholds_ResidualAndRankCap
**Lines:** ~16
**Preconditions:**
- Residual = 0.05 > threshold 0.02 (STALE)
- Rank cap = 80 < threshold 100 (STALE)

**Test Logic:**
1. Create policy with both thresholds
2. Call `computeState()`
3. Assert `state = STALE`

**Validates:** Residual and rank cap thresholds both trigger staleness

---

### 6. Phase4_InvalidationReasonPropagation
**Lines:** ~20
**Preconditions:**
- Multiple InvalidationReason values

**Test Logic:**
1. For each reason (INTEGRITY_CHECK_FAILED, STALENESS_EXCEEDED, etc.):
   - Call `invalidate(metadata, reason)`
   - Assert `state = INVALIDATED`
   - Assert `invalidation_reason = reason`

**Validates:** All InvalidationReason codes properly propagated

---

### 7. Phase4_PreservesStateOnInvalidTransition
**Lines:** ~10
**Preconditions:**
- State = PRISTINE

**Test Logic:**
1. Call `computeState()` on PRISTINE artifact
2. Assert `state = READY` (PRISTINE → READY transition)

**Validates:** State machine transitions respect semantics

---

### 8. Phase4_RebuildTimestampsAreRecorded
**Lines:** ~18
**Preconditions:**
- State = REBUILDING

**Test Logic:**
1. Call `completeRebuildSuccess()`
2. Assert `last_successful_rebuild_ms.has_value() = true`
3. Call `completeRebuildFailure()`
4. Assert `last_failed_rebuild_ms.has_value() = true`

**Validates:** Rebuild history tracking

---

### 9. Phase4_DiagnoseStalenessCauseCapturesDetails
**Lines:** ~14
**Preconditions:**
- Age = 5000 > threshold 2000

**Test Logic:**
1. Call `diagnoseStalenessCause()`
2. Assert diagnosis contains non-empty string
3. Assert description mentions "Age" or "age"

**Validates:** Diagnostic messages capture specific causes

---

### 10. Phase4_BatchComputeStatesPreservesSemantics
**Lines:** ~24
**Preconditions:**
- Batch with READY, READY (should be stale), and INVALIDATED artifacts

**Test Logic:**
1. Create batch with age policy
2. Call `computeStatesBatch()`
3. Assert states: [READY, STALE, INVALIDATED]

**Validates:** Batch operations preserve lifecycle semantics

---

## Coverage Summary by Error/Policy Type

### FallbackReason Coverage (query_planner_test.cc)
- ✅ `CategoryCSubpathDetected` — Test 1
- ✅ `TensorArtifactStale` — Test 2
- ✅ `TensorResidualLow` — Test 4 (implied)
- ✅ `TensorRankCapExceeded` — Test 5
- ✅ `TensorRebuildInProgress` — Test 6
- ✅ Fallback chain semantics — Test 7
- ✅ No silent fallback — Test 8

### MetricErrorKind Coverage (retrieval_metrics_test.cc)
- ✅ `EmptyGroundTruth` — Test 8, 10
- ✅ `InvalidK` — Test 3
- ✅ `NonFiniteInput` — Test 1, 2
- ✅ `InvalidRange` — Test 4, 5, 6
- ✅ `DuplicateEntries` — Test 9
- ✅ `ResidualTooHighForPlanner` — Test 11

### GovernanceDecision Coverage (approximation_rules_test.cc)
- ✅ `Deny` (Category C) — Test 2, 3
- ✅ `Allow` (proper policy) — Test 6 (low conf → high conf)
- ✅ `EscalateToExact` (confidence) — Test 6 (high threshold)
- ✅ Policy versioning — Test 4
- ✅ Path progression — Test 5

### LifecycleState Coverage (artifact_lifecycle_test.cc)
- ✅ `FAILED` state usage — Test 1
- ✅ `REBUILDING` transitions — Test 2, 3
- ✅ `READY` transitions — Test 7
- ✅ `STALE` detection — Test 4, 5
- ✅ `INVALIDATED` propagation — Test 6

### InvalidationReason Coverage (artifact_lifecycle_test.cc)
- ✅ `INTEGRITY_CHECK_FAILED` — Test 6
- ✅ `STALENESS_EXCEEDED` — Test 6
- ✅ `POLICY_VIOLATION` — Test 6
- ✅ `SHARD_UNAVAILABLE` — Test 6

---

## Build & Test Commands

```bash
# Build all evaluation tests
cmake --build . --target query_planner_test
cmake --build . --target retrieval_metrics_test
cmake --build . --target approximation_rules_test
cmake --build . --target artifact_lifecycle_test

# Run via ctest
ctest -R "query_planner_test|retrieval_metrics_test|approximation_rules_test|artifact_lifecycle_test" -V

# Run individual test suites
./tests/epic2_evaluation/query_planner_test --gtest_filter="Phase4*"
./tests/epic2_evaluation/retrieval_metrics_test --gtest_filter="*Phase4*"
./tests/epic2_evaluation/approximation_rules_test --gtest_filter="*Phase4*"
./tests/epic2_evaluation/artifact_lifecycle_test --gtest_filter="*Phase4*"
```

