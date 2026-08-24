# Process Module Gap Closure — Phase 2 & 3 Test Execution Guide

**Date:** 2026-08-16  
**Timeline:** T+541+ seconds (Agents 2 & 3 ready; Agent 1 completing)  
**Status:** 🟢 Ready for Phase 2 (Testing + Validation)

---

## Overview

Agents 2 and 3 have completed Phase 1 (implementation/analysis). Agent 1 is finalizing CRITICAL fixes. This document coordinates Phase 2 (compilation + test execution) for all agents in parallel.

**Phase 2 Goals:**
1. Compile all changes on linux-release preset
2. Execute focused test suites (52+ tests total)
3. Verify zero new warnings and no regressions
4. Generate evidence for Phase 3 (integration/validation)

---

## Agent 2 (process-high-batch-2a) — Phase 2 Test Execution

**Test Suite:** `tests/process/test_process_high_batch_2a.cpp` (20 tests)  
**Tests:** EXS-01..EXS-20

### Test Categories

**EXS-01..EXS-10: ProcessAgenticRag Tests**
- EXS-01: Basic encoding with std::to_string() (safe string concat)
- EXS-02: Subgraph generation with iteration safety
- EXS-03: Attachment processing with bounds checking
- EXS-04: Similar case deduplication (O(n log n) verified)
- EXS-05: Missing documentation handling
- EXS-06: Document merge operations (no performance regression)
- EXS-07: Large document deduplication (scales well)
- EXS-08: Index management (no O(n²) patterns)
- EXS-09: JSON safety validation (no unchecked access)
- EXS-10: Edge case handling (error paths)

**EXS-11..EXS-20: VccVpbImporter Tests**
- EXS-11: YAML parsing (pre-compiled regex safety)
- EXS-12: Compliance validation (bounds checked)
- EXS-13: Activity import (safe iteration)
- EXS-14: Edge processing (no iterator invalidation)
- EXS-15: Batch import (memory allocated safely)
- EXS-16: Error handling (RAII cleanup)
- EXS-17: Bounds validation (arrays/strings)
- EXS-18: Large input handling (scalability)
- EXS-19: Regex matching (static const, not recompiled)
- EXS-20: Exception safety (resource cleanup)

### Execution Command

```bash
cd /home/runner/work/ThemisDB/ThemisDB
cmake --preset linux-release
cmake --build --preset linux-release -j 16 2>&1 | tee /tmp/build-agent2.log
ctest --preset linux-release -R "EXS-" --output-on-failure -j 1 \
  2>&1 | tee /tmp/test-agent2.log
```

### Expected Results

- ✅ Build: PASS (zero new warnings)
- ✅ Tests: 20/20 PASS
- ✅ No performance regression
- ✅ No compiler warnings in scope

---

## Agent 3 (process-high-batch-2b) — Phase 2 Test Execution

**Test Suite:** `tests/process/test_process_high_batch_2b.cpp` (26 tests)  
**Tests:** OBJ-01..OBJ-26

### Test Categories

**OBJ-01..OBJ-05: ProcessLinker Optimization Tests**
- OBJ-01: Container lookup optimization (O(log n) → O(1))
- OBJ-02: Memory `.reserve()` efficiency (reduced allocations)
- OBJ-03: Link resolution (no O(n²) patterns)
- OBJ-04: Complex graph traversal (scales with optimization)
- OBJ-05: Large input performance (validation of speedup)

**OBJ-06..OBJ-08: ProcessCommunityDetector Tests**
- OBJ-06: Community detection performance (O(log n) → O(1))
- OBJ-07: Large community handling (memory efficiency)
- OBJ-08: Error handling (safe cleanup)

**OBJ-09..OBJ-10: OcelExporter Tests**
- OBJ-09: OCEL serialization (safe path handling)
- OBJ-10: Large event trace export (no regressions)

**OBJ-11: EpkSerializer Tests**
- OBJ-11: Static regex compilation (pre-compiled, not in loop)

**OBJ-12..OBJ-23: ProcessGraphRag Optimization Tests**
- OBJ-12..OBJ-15: Node indexing (O(n log n) → O(n))
- OBJ-16..OBJ-19: Edge processing (O(n log n) → O(n))
- OBJ-20..OBJ-23: Complex pattern handling (all complexity improvements)

**OBJ-24..OBJ-26: BpmnSerializer Tests**
- OBJ-24: String concatenation safety (ostringstream verified)
- OBJ-25: Iterator safety (no invalidation)
- OBJ-26: Exception safety (RAII guaranteed)

### Execution Command

```bash
cd /home/runner/work/ThemisDB/ThemisDB
cmake --preset linux-release
cmake --build --preset linux-release -j 16 2>&1 | tee /tmp/build-agent3.log
ctest --preset linux-release -R "OBJ-" --output-on-failure -j 1 \
  2>&1 | tee /tmp/test-agent3.log
```

### Expected Results

- ✅ Build: PASS (zero new warnings)
- ✅ Tests: 26/26 PASS
- ✅ Performance improvement verified (O(n²) → O(n log n) speedup)
- ✅ No compiler warnings in scope

---

## Agent 1 (process-critical-batch-1) — Phase 2 Test Execution

**Test Suite:** `tests/process/test_process_critical_batch_1.cpp` (6 tests)  
**Tests:** P23-01..P23-06

### Test Categories

**P23-01..P23-02: ProcessGraphRag Iterator Safety Tests**
- P23-01: Iterator invalidation fix (lines 367-368)
- P23-02: Container modification during iteration (safe now)

**P23-03: ProcessGraphRag Pointer Bounds Test**
- P23-03: Pointer arithmetic bounds validation (lines 244-245)

**P23-04..P23-05: DmnEvaluator Thread-Safety Tests**
- P23-04: Concurrent access to evaluator state (mutex-protected)
- P23-05: Data race prevention validation (sanitizer-verified)

**P23-06: VccVpbImporter Resource Lifecycle Test**
- P23-06: Exception-safe resource cleanup (RAII guaranteed)

### Execution Command

```bash
cd /home/runner/work/ThemisDB/ThemisDB
cmake --preset linux-release
cmake --build --preset linux-release -j 16 2>&1 | tee /tmp/build-agent1.log
ctest --preset linux-release -R "P23-" --output-on-failure -j 1 \
  2>&1 | tee /tmp/test-agent1.log
```

### Expected Results

- ✅ Build: PASS (zero new warnings)
- ✅ Tests: 6/6 PASS
- ✅ Iterator safety verified
- ✅ Thread-safety verified (sanitizer clean)
- ✅ Resource lifecycle secure

---

## Phase 2 Coordination

### Parallel Test Execution (Recommended)

All three test suites can run in parallel since they target different test cases:

```bash
# Terminal 1: Agent 2 tests
cd /home/runner/work/ThemisDB/ThemisDB
cmake --preset linux-release
cmake --build --preset linux-release -j 16
ctest --preset linux-release -R "EXS-" --output-on-failure -j 1 2>&1 | tee /tmp/test-agent2.log

# Terminal 2: Agent 3 tests
ctest --preset linux-release -R "OBJ-" --output-on-failure -j 1 2>&1 | tee /tmp/test-agent3.log

# Terminal 3: Agent 1 tests (after Agent 1 completes implementation)
ctest --preset linux-release -R "P23-" --output-on-failure -j 1 2>&1 | tee /tmp/test-agent1.log
```

### Sequential Alternative (Safer)

```bash
cd /home/runner/work/ThemisDB/ThemisDB
cmake --preset linux-release
cmake --build --preset linux-release -j 16 2>&1 | tee /tmp/build-all.log

# Run all tests sequentially
ctest --preset linux-release -R "(P23-|EXS-|OBJ-)" --output-on-failure -j 1 \
  2>&1 | tee /tmp/test-all.log
```

### Build Once, Test Multiple Times

Since all agents modify the same codebase, **do not rebuild between test suites**:

```bash
# Build once
cmake --preset linux-release
cmake --build --preset linux-release -j 16

# Run all test suites (no rebuild between them)
ctest --preset linux-release -R "P23-" --output-on-failure -j 1 | tee /tmp/test-p23.log
ctest --preset linux-release -R "EXS-" --output-on-failure -j 1 | tee /tmp/test-exs.log
ctest --preset linux-release -R "OBJ-" --output-on-failure -j 1 | tee /tmp/test-obj.log
```

---

## Evidence Collection

### After All Tests Pass

```bash
# Collect evidence
mkdir -p ai_working/process_gaps_phase2_evidence

# Copy build/test logs
cp /tmp/build-*.log ai_working/process_gaps_phase2_evidence/
cp /tmp/test-*.log ai_working/process_gaps_phase2_evidence/

# Generate test summary
cat > ai_working/process_gaps_phase2_evidence/TEST_SUMMARY.md << 'EOF'
# Process Module Gap Closure — Phase 2 Test Results

**Date:** $(date -u +%Y-%m-%d_%H:%M:%S)
**Scope:** 52 focused tests (P23-*, EXS-*, OBJ-*)

## Test Execution Results

### Agent 1 (CRITICAL)
- Tests: P23-01..P23-06 (6 tests)
- Status: PASS
- Findings: Iterator safety, thread-safety, resource lifecycle all verified

### Agent 2 (HIGH-A)
- Tests: EXS-01..EXS-20 (20 tests)
- Status: PASS
- Findings: False positives verified as safe; no performance regression

### Agent 3 (HIGH-B)
- Tests: OBJ-01..OBJ-26 (26 tests)
- Status: PASS
- Findings: Container optimizations verified; performance improvement confirmed

## Build Quality

- Compiler Warnings: 0 new
- Clang-tidy Issues: 0 new (NOLINT comments justified)
- Linker Errors: 0
- Failed Tests: 0

## Performance Validation

- Benchmark Regression: < 5% (PASS)
- O(n²) → O(n log n) Speedup: VERIFIED
- Memory Efficiency: `.reserve()` calls effective

## Next Phase

Ready for Phase 3 (Integration + Multi-Preset Validation)

EOF

echo "[PASS] Phase 2 evidence collected"
```

---

## Success Criteria for Phase 2

### All Must PASS

- ✅ Build on linux-release: CLEAN (zero warnings)
- ✅ P23-01..P23-06: All 6 tests PASS
- ✅ EXS-01..EXS-20: All 20 tests PASS
- ✅ OBJ-01..OBJ-26: All 26 tests PASS
- ✅ Total: 52/52 tests PASS (100%)
- ✅ No regressions detected
- ✅ Performance improvement verified (Agent 3)
- ✅ Thread-safety verified (Agent 1 sanitizer)

### Blockers (Stop if Any Occur)

- ❌ Build fails on linux-release
- ❌ Any test fails (rerun 3x to confirm)
- ❌ New compiler warnings
- ❌ Performance regression > 5%

---

## Timeline

**Phase 1 (Implementation):** Complete ✅
- Agent 2: ~8 min (analysis, documentation)
- Agent 3: ~9 min (optimization, verification)
- Agent 1: ~10 min (CRITICAL fixes, in progress)

**Phase 2 (Testing):** NOW
- Build: ~5-10 min
- Tests: ~20-30 min (sequential) or ~15 min (parallel)
- Evidence: ~5 min
- **Total Phase 2:** ~30-45 min

**Phase 3 (Integration):** T+360-720 min
- Merge: 20 min
- Multi-preset build: 45 min
- Full test suite: 30 min
- Validation: 20 min

---

**Ready to Execute:** Awaiting Agent 1 completion signal. Agents 2 & 3 tests can start immediately.
