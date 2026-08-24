# Phase 4-5 Regression Testing & Performance Baseline Execution Plan

**Status:** In Progress (Week 1: Block R4.1 - Error Taxonomy Regression)  
**Date Started:** 2026-08-02  
**Target Completion:** 2026-09-06 (5 weeks)

## Executive Summary

This document tracks the execution of a 5-week comprehensive regression testing and performance baseline initiative for the AQL module. The work spans two major phases:

- **Phase 4 (Weeks 1-2)**: Regression Testing for Error Handling & Edge Cases
- **Phase 5 (Weeks 3-5)**: Performance Baseline for Concurrency, Degradation, Policies, and Release Gates

## Test Inventory

### Phase 4: Regression Testing (Total: 29 tests)

| Block | Test File | Test Count | Target |
|-------|-----------|------------|--------|
| R4.1 | test_aql_validation_error_handling.cpp | 8 | Error taxonomy and recovery |
| R4.1 | test_aql_translation_recovery.cpp | 8 | Translation error paths |
| R4.1 | test_aql_bridge_degradation.cpp | 7 | Bridge error handling |
| R4.2 | test_aql_schema_edge_cases.cpp | 6 | Schema context edge cases |
| **Phase 4 Total** | | **29** | |

### Phase 5: Performance Baseline (Total: 28 tests + benchmarks)

| Block | Test File | Test Count | Target |
|-------|-----------|------------|--------|
| P5.1 | test_aql_conversation_concurrency.cpp | 8 | Concurrency and thread safety |
| P5.2 | test_aql_provider_degradation.cpp | 8 | Provider degradation performance |
| P5.3 | test_aql_token_policy.cpp | 6 | Token budget policy performance |
| P5.3 | test_aql_circuit_breaker_policy.cpp | 6 | Circuit breaker performance |
| **Phase 5 Total** | | **28** | |

### Benchmarks (Phase 6 supporting)

- benchmarks/aql/bench_aql_translation.cpp (simple/complex translation + validation batch)
- benchmarks/aql/bench_aql_helper_paths.cpp (scorer/few-shot/highlighter/tokens)

## Build Instructions

### Prerequisites
- CMake 3.25+
- C++20 compiler (clang 15+ or g++ 11+)
- System packages: libfmt-dev, libspdlog-dev, nlohmann-json3-dev, libboost-all-dev

### Configuration (Community Release)
```bash
cd /home/runner/work/ThemisDB/ThemisDB
cmake -DTHEMIS_AUTO_BOOTSTRAP_DEPS=ON \
       -DTHEMIS_ALLOW_MISSING_ROCKSDB=ON \
       -DBUILD_TESTS=ON \
       -DCMAKE_BUILD_TYPE=Release \
       -DTHEMIS_EDITION=COMMUNITY \
       -DTHEMIS_ENABLE_MIMALLOC=OFF \
       -DTHEMIS_ENABLE_COMPILER_CACHE=OFF \
       .
```

### Build Phase 4 Tests
```bash
# Build all AQL tests
cmake --build . --target module_aql_test_aql_validation_error_handling_focused --parallel 4
cmake --build . --target module_aql_test_aql_translation_recovery_focused --parallel 4
cmake --build . --target module_aql_test_aql_bridge_degradation_focused --parallel 4
cmake --build . --target module_aql_test_aql_schema_edge_cases_focused --parallel 4
```

### Run Phase 4 Tests (CTest)
```bash
# Run specific tests with verbose output
ctest --verbose -R "AQLValidationErrorHandling" 
ctest --verbose -R "AQLTranslationRecovery"
ctest --verbose -R "AQLBridgeDegradation"
ctest --verbose -R "AQLSchemaEdgeCases"

# Or run all AQL tests at once
ctest --verbose -R "aql"
```

## Week-by-Week Execution Schedule

### Week 1 (Aug 2-8)
- [ ] Block R4.1.1: Error Taxonomy Definition Verification
  - Execute test_aql_validation_error_handling.cpp (8 cases)
  - Verify all error paths detected and logged correctly
  - Document error coverage matrix
- [ ] Block R4.1.2: Translation Recovery Tests
  - Execute test_aql_translation_recovery.cpp (8 cases)
  - Verify retry logic and error enrichment
  - Document translation error paths

### Week 2 (Aug 9-15)
- [ ] Block R4.2: Bridge Degradation Tests
  - Execute test_aql_bridge_degradation.cpp (7 cases)
  - Verify context overflow handling
  - Document memory leak verification (AddressSanitizer)
- [ ] Block R4.3: Schema Edge Cases
  - Execute test_aql_schema_edge_cases.cpp (6 cases)
  - Verify edge case handling
  - Complete Phase 4 exit gate verification

### Week 3 (Aug 16-22)
- [ ] Block P5.1: Concurrency Performance Baseline
  - Execute test_aql_conversation_concurrency.cpp (8 cases)
  - Profile p50/p95/p99 latencies
  - Verify memory safety with AddressSanitizer
  - Document concurrency baseline

### Week 4 (Aug 23-29)
- [ ] Block P5.2: Degraded-Mode Performance
  - Execute test_aql_provider_degradation.cpp (8 cases)
  - Measure degradation latency impact
  - Document baseline metrics
- [ ] Block P5.3: Policy Performance
  - Execute test_aql_token_policy.cpp (6 cases)
  - Execute test_aql_circuit_breaker_policy.cpp (6 cases)
  - Establish policy enforcement latency baselines

### Week 5 (Aug 30-Sep 6)
- [ ] Block P5.4: Release Gate Benchmark Stabilization
  - Run benchmarks 10x for variance analysis
  - Lock release gates AG-4/AG-5/AG-6
  - Update PERFORMANCE_EXPECTATIONS.md
  - Finalize and commit all evidence

## Evidence Collection Template

For each block, collect:
1. **Test Results**
   - Total tests run
   - Total tests passed
   - Total tests failed
   - Flaky tests (if any)
   - Test timeouts

2. **Error Coverage**
   - Error categories tested
   - Error path coverage %
   - Diagnostic message quality

3. **Resource Metrics**
   - Peak memory usage
   - Resource leaks (AddressSanitizer)
   - CPU utilization

4. **Performance Baseline** (Phase 5 only)
   - p50/p95/p99 latencies
   - Throughput metrics
   - Variance analysis (target: < 5%)

## Success Criteria

### Phase 4 Hard Gates
- ✅ All 29 regression tests PASS (zero flakes)
- ✅ 100% error path coverage verified  
- ✅ Zero resource leaks (AddressSanitizer PASS)
- ✅ Diagnostic messages reviewed and production-ready
- ✅ Phase 4 Exit Gate Report completed

### Phase 5 Hard Gates
- ✅ All 28 performance tests PASS
- ✅ Baseline metrics established (p50/p95/p99)
- ✅ Benchmark variance < 5% across 10 runs
- ✅ Release gates AG-4/AG-5/AG-6 locked
- ✅ PERFORMANCE_EXPECTATIONS.md updated

## Documentation Deliverables

1. `ai_working/PHASE4_EXIT_GATE_REPORT.md`
2. `ai_working/phase4_evidence/error_taxonomy_regression_report.md`
3. `ai_working/phase4_evidence/translation_pipeline_regression_report.md`
4. `ai_working/phase4_evidence/bridge_consistency_report.md`
5. `ai_working/phase4_evidence/edge_case_regression_report.md`
6. `ai_working/phase5_evidence/concurrency_baseline.md`
7. `ai_working/phase5_evidence/degraded_mode_baseline.md`
8. `ai_working/phase5_evidence/policy_edge_baseline.md`
9. `ai_working/phase5_evidence/benchmark_stabilization_report.md`
10. Updated `src/aql/PERFORMANCE_EXPECTATIONS.md`
11. Updated `src/aql/ROADMAP.md`

## Known Issues / Blockers

- [ ] Full system build has dependency resolution issues (httplib, additional boost)
- [ ] Consider focusing on individual test targets if full build is problematic
- [ ] Alternative: Use prebuilt binaries if available for CI/CD pipeline

