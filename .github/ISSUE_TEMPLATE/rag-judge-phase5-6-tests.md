---
name: RAG Judge - Phase 5 & 6 Test Suite
about: Umfassende Test-Suite für Bias-Detection, Calibration, Cache und Batch-Processing (2-3 Tage)
title: '[RAG-JUDGE-P5P6-TEST] Phase 5 & 6 Comprehensive Test Suite Implementation'
labels: 'priority:P2, type:test, area:rag, area:testing, effort:medium, phase:5, phase:6'
assignees: ''
---

## 📋 Übersicht

Implementierung der umfassenden Test-Suite für Phase 5 (Bias-Mitigation & Calibration) und Phase 6 (Performance & Caching) des RAG Judge Frameworks.

**Test Files:**
- `tests/test_rag_judge_phase5.cpp` (🚧 zu erstellen)
- `tests/test_rag_judge_phase6.cpp` (🚧 zu erstellen)
- Update: `tests/CMakeLists.txt` (Test-Targets hinzufügen)

**Total Tests:** 32 neue Tests (14 Phase 5 + 18 Phase 6)

## 🎯 Ziele

- 🚧 Phase 5 Tests: BiasDetector (8) + CalibrationManager (6)
- 🚧 Phase 6 Tests: EvaluationCache (10) + BatchEvaluator (8)
- 🚧 Integration Tests: Phase 5+6 zusammen (4)
- 🚧 Performance Benchmarks: Latency & Throughput
- 🚧 CMake-Integration: Test-Targets konfigurieren

## 📦 Arbeitspakete

### 1. Phase 5 Tests - BiasDetector (1 Tag)

**Test-File:** `tests/test_rag_judge_phase5.cpp`

**Zu implementieren (8 Tests):**

#### Bias Detection Tests
1. [ ] `TEST(BiasDetector, PositionBiasBalanced)`
   - 50/50 split A vs B wins
   - Verify no significant bias (p > 0.05)
2. [ ] `TEST(BiasDetector, PositionBiasImbalanced)`
   - 70/30 split (Position 1 preferred)
   - Verify significant bias detected (p < 0.05)
3. [ ] `TEST(BiasDetector, LengthBiasPositiveCorrelation)`
   - Scores increase with length
   - Verify positive correlation (r > 0.5)
4. [ ] `TEST(BiasDetector, LengthBiasNoCorrelation)`
   - Random scores vs length
   - Verify no significant bias
5. [ ] `TEST(BiasDetector, InsufficientSamples)`
   - < 10 samples
   - Verify "insufficient samples" result
6. [ ] `TEST(BiasDetector, BiasMitigation)`
   - Apply mitigation to high-bias result
   - Verify confidence reduced
7. [ ] `TEST(BiasDetector, StatisticalSignificance)`
   - Borderline significance (p ≈ 0.05)
   - Verify correct classification
8. [ ] `TEST(BiasDetector, ConfigurationUpdate)`
   - Change significance_threshold
   - Verify behavior changes

**Test-Daten:**
- Mock comparison results with known bias patterns
- Synthetic score-length pairs with controlled correlation

**Acceptance Criteria:**
- Alle 8 Tests bestehen
- Edge-Cases abgedeckt (0 samples, all ties, etc.)
- Statistical tests numerically stable

---

### 2. Phase 5 Tests - CalibrationManager (1 Tag)

**Zu implementieren (6 Tests):**

1. [ ] `TEST(CalibrationManager, LoadGroundTruthJSON)`
   - Load sample ground truth file
   - Verify all annotations parsed correctly
2. [ ] `TEST(CalibrationManager, TemperatureScalingTraining)`
   - Train on synthetic data
   - Verify ECE improves (pre > post)
3. [ ] `TEST(CalibrationManager, CalculateECE)`
   - Perfect calibration: ECE = 0
   - Poor calibration: ECE > 0.2
4. [ ] `TEST(CalibrationManager, BrierScore)`
   - Perfect predictions: Brier = 0
   - Random predictions: Brier ≈ 0.25
5. [ ] `TEST(CalibrationManager, InterAnnotatorAgreement)`
   - 2 annotators: Cohen's Kappa
   - 3+ annotators: Fleiss' Kappa
   - Perfect agreement: Kappa = 1.0
6. [ ] `TEST(CalibrationManager, ModelPersistence)`
   - Train model, save to file
   - Load model, verify parameters identical
   - Apply calibration, verify results match

**Test-Daten:**
- `tests/data/ground_truth_sample.json` (20-30 annotations)
- Synthetic predictions with known calibration error

**Acceptance Criteria:**
- Alle 6 Tests bestehen
- Calibration verbessert ECE messbar
- Persistence round-trip funktioniert

---

### 3. Phase 6 Tests - EvaluationCache (1 Tag)

**Test-File:** `tests/test_rag_judge_phase6.cpp`

**Zu implementieren (10 Tests):**

1. [ ] `TEST(EvaluationCache, BasicGetPut)`
   - Put entry, verify get returns same
   - Verify cache_hits increments
2. [ ] `TEST(EvaluationCache, LRUEviction)`
   - Fill cache to capacity + 1
   - Verify oldest entry evicted
3. [ ] `TEST(EvaluationCache, TTLExpiration)`
   - Insert with short TTL (1s)
   - Sleep 1.5s, verify entry expired
4. [ ] `TEST(EvaluationCache, CacheWarming)`
   - Warm with 10 queries
   - Verify all 10 in cache
   - Verify hit rate high on re-query
5. [ ] `TEST(EvaluationCache, ThreadSafetyConcurrentGetPut)`
   - 4 threads doing concurrent get/put
   - Verify no crashes, data consistent
6. [ ] `TEST(EvaluationCache, InvalidationModelUpdate)`
   - Fill cache, invalidate MODEL_UPDATE
   - Verify cache cleared
7. [ ] `TEST(EvaluationCache, Statistics)`
   - 10 hits, 5 misses
   - Verify hit_rate = 10/15 = 0.67
8. [ ] `TEST(EvaluationCache, ConfigUpdateMaxEntries)`
   - Reduce max_entries from 100 to 50
   - Verify 50 entries evicted
9. [ ] `TEST(EvaluationCache, CacheClear)`
   - Fill cache, clear()
   - Verify size = 0
10. [ ] `TEST(EvaluationCache, HighThroughputPerformance)`
    - 10,000 sequential gets
    - Verify < 1ms average lookup time

**Acceptance Criteria:**
- Alle 10 Tests bestehen
- ThreadSanitizer clean (no data races)
- Performance-Target erfüllt

---

### 4. Phase 6 Tests - BatchEvaluator (1 Tag)

**Zu implementieren (8 Tests):**

1. [ ] `TEST(BatchEvaluator, BasicBatchEvaluation)`
   - 10 inputs, 2 workers
   - Verify all 10 evaluated
2. [ ] `TEST(BatchEvaluator, ParallelSpeedup)`
   - 100 inputs, measure time with 1 vs 4 workers
   - Verify 4-worker ≈ 3x faster (accounting for overhead)
3. [ ] `TEST(BatchEvaluator, AsyncEvaluationSingle)`
   - evaluateAsync(input)
   - Verify handle.isDone() eventually true
   - Verify handle.get() returns result
4. [ ] `TEST(BatchEvaluator, AsyncEvaluationMultiple)`
   - evaluateAsync(10 inputs)
   - Wait for all handles
   - Verify all results correct
5. [ ] `TEST(BatchEvaluator, ProgressTrackingCallback)`
   - Set progress_callback
   - Verify called with progress updates
   - Verify final progress = 100%
6. [ ] `TEST(BatchEvaluator, TimeoutHandling)`
   - Mock slow evaluation (> timeout)
   - Verify timeout exception thrown
7. [ ] `TEST(BatchEvaluator, CancellationSupport)`
   - evaluateAsync(), immediately cancel()
   - Verify not processed or exception on get()
8. [ ] `TEST(BatchEvaluator, StopResumeControl)`
   - Start batch, stop() mid-way
   - Verify queue cleared
   - Resume(), verify new items processed

**Acceptance Criteria:**
- Alle 8 Tests bestehen
- Parallel speedup nachweisbar
- Timeout/Cancellation funktioniert

---

### 5. Integration Tests (0.5 Tage)

**Zu implementieren (4 Tests):**

1. [ ] `TEST(Phase5P6Integration, BiasDetectionWithCache)`
   - Run bias detection, cache results
   - Verify subsequent bias checks use cache
2. [ ] `TEST(Phase5P6Integration, CalibrationWithBatchEval)`
   - Train calibration on batch of ground truth
   - Verify batch processing efficient
3. [ ] `TEST(Phase5P6Integration, FullPipelineWithAllFeatures)`
   - BiasDetector + CalibrationManager + Cache + BatchEval
   - End-to-end test
4. [ ] `TEST(Phase5P6Integration, PerformanceRegression)`
   - Benchmark full pipeline
   - Verify < 50ms overhead per evaluation

**Acceptance Criteria:**
- Components work together
- No performance regressions

---

### 6. Performance Benchmarks (0.5 Tage)

**Zu implementieren:**
- [ ] `BENCHMARK(BiasDetection100Samples)` - < 50ms
- [ ] `BENCHMARK(CalibrationApplication)` - < 1ms per result
- [ ] `BENCHMARK(CacheLookup1000Queries)` - < 1ms average
- [ ] `BENCHMARK(BatchEval100Items4Workers)` - Throughput > 100/sec

**Framework:** Google Benchmark oder custom timing

**Acceptance Criteria:**
- All targets met
- Results documented in progress doc

---

### 7. CMake Integration & CI (0.5 Tage)

**Zu implementieren:**

#### CMakeLists.txt Updates
```cmake
# tests/CMakeLists.txt

# Phase 5 Tests
add_executable(test_rag_judge_phase5
    test_rag_judge_phase5.cpp
)
target_link_libraries(test_rag_judge_phase5 PRIVATE
    ${TEST_LIBS}
    themis_core
    spdlog::spdlog
    nlohmann_json::nlohmann_json
    GTest::gtest
    GTest::gtest_main
)
add_test(NAME RAGJudgePhase5Tests COMMAND test_rag_judge_phase5)
set_tests_properties(RAGJudgePhase5Tests PROPERTIES
    LABELS "rag;llm;judge;bias;calibration;phase5"
    TIMEOUT 120
)

# Phase 6 Tests
add_executable(test_rag_judge_phase6
    test_rag_judge_phase6.cpp
)
target_link_libraries(test_rag_judge_phase6 PRIVATE
    ${TEST_LIBS}
    themis_core
    spdlog::spdlog
    GTest::gtest
    GTest::gtest_main
)
add_test(NAME RAGJudgePhase6Tests COMMAND test_rag_judge_phase6)
set_tests_properties(RAGJudgePhase6Tests PROPERTIES
    LABELS "rag;llm;judge;cache;batch;phase6"
    TIMEOUT 180
)
```

#### Test-Daten-Files
- [ ] `tests/data/ground_truth_sample.json` - Calibration test data
- [ ] `tests/data/bias_comparison_samples.json` - Bias detection test data

**Acceptance Criteria:**
- Tests bauen erfolgreich
- ctest integration funktioniert
- Test-Labels korrekt für selective runs

---

## 🔗 Abhängigkeiten

**Test-Framework:**
- Google Test (gtest) - Unit testing
- Google Benchmark (optional) - Performance benchmarks

**Test-Daten:**
- Ground truth annotations (JSON)
- Mock evaluation results
- Synthetic bias patterns

**Voraussetzungen:**
- Phase 5 & 6 Implementations complete
- Test infrastructure from Phase 1-4

---

## 📊 Erfolgskriterien

- [ ] 32 neue Tests implementiert (14 P5 + 18 P6)
- [ ] Alle Tests bestehen
- [ ] Code-Coverage > 85% für Phase 5 & 6
- [ ] Performance-Benchmarks erfüllt
- [ ] CI-Integration funktioniert
- [ ] Test-Dokumentation aktualisiert
- [ ] ThreadSanitizer/Valgrind clean

---

## 📝 Implementation Notes

**Test-Best-Practices:**
- Use mocks für Judge (avoid real LLM calls)
- Deterministic test data (no randomness without seed)
- Clear test names (describe expected behavior)
- Arrange-Act-Assert pattern
- Clean up resources (RAII, fixtures)

**Coverage-Ziele:**
- BiasDetector: > 90% (core statistical logic)
- CalibrationManager: > 85% (training + metrics)
- EvaluationCache: > 90% (cache operations)
- BatchEvaluator: > 85% (threading code challenging)

**Performance-Test-Setup:**
```cpp
auto start = std::chrono::high_resolution_clock::now();
// Run operation
auto end = std::chrono::high_resolution_clock::now();
auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
EXPECT_LT(duration.count(), 50); // < 50ms
```

---

## 📚 Referenzen

- Phase 1-4 Test Suites: Existing patterns to follow
- Google Test Documentation: https://google.github.io/googletest/
- Phase 5 & 6 Progress: `docs/implementation-history/IMPLEMENTATION_PROGRESS_RAG_JUDGE_P5_P6.md`

---

**Labels:** `priority:P2`, `type:test`, `area:rag`, `area:testing`, `effort:medium`, `phase:5`, `phase:6`  
**Estimated Effort:** 2-3 Tage (1 Developer)  
**Dependencies:** CalibrationManager, EvaluationCache, BatchEvaluator implementations complete  
**Blocks:** Production deployment, Performance validation
