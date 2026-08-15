# Phase 2 CRITICAL Fixes Agent Specification

**Phase:** 2 (Weeks 2-3, Aug 22 – Sep 5)  
**Agent Type:** `themisdb-implementer` (coding + build/test)  
**Scope:** 44 CRITICAL gaps across 11 files  
**Target Artifact:** `IMPORTERS_PHASE2_CRITICAL_FIXES_COMPLETE.md`

---

## Objective

Resolve **all 44 CRITICAL gaps** that pose immediate runtime safety risks:
- Null pointer dereference
- Data race conditions
- Blocking operations without timeout
- Smart pointer misuse (raw new/delete)
- Resource leak in exception paths

---

## Batching Strategy

### Batch A: PostgreSQL & MySQL CRITICAL (11 gaps)
**Duration:** 3-4 days  
**Files:** postgres_importer.cpp (5), mysql_importer.cpp (6)

**Gap Categories:**
- Null dereference: Handle lifecycle in postgres (356, 359, 360, 362, 363, 367, 400, 401, 402) — 9 items
- Data race: custom_type_map_ access (postgres 2104, 2106) — 2 items
- Blocking without timeout: postgres line 373 (ProgressCallback) — 1 item
- No timeout: postgres line 373 (condition_variable) — 1 item
- Smart ptr misuse: postgres line 2452, mysql (6 items) — 7 items

**Implementation Strategy:**
1. Add null checks for handle lifecycle: wrap in `if (handle && handle->...)`
2. Add scoped mutex protection: std::lock_guard for custom_type_map_ access
3. Replace raw new/delete with std::make_unique/std::make_shared
4. Add timeout to mutex_lock and condition_variable waits
5. Test: Create focused test suite IMPI-A-01..11 for these gaps

**Acceptance Criteria:**
- [ ] All 11 gaps fixed or documented as deferred
- [ ] postgres_importer.cpp compiles without new warnings
- [ ] mysql_importer.cpp compiles without new warnings
- [ ] Focused tests IMPI-A-01..11 PASS
- [ ] Benchmark gates IMRG-01..06 stable (±5% of baseline)
- [ ] Git commit with message: `IMPORTERS-P2-BATCH-A: Fix 11 CRITICAL gaps (postgres, mysql)`

**Test Scope:** IMPI-A-01..11 (11 focused tests covering handle null checks, data race protection, timeout enforcement, smart ptr wrapping)

---

### Batch B: Flatfile, HuggingFace, Deterministic, GUI (19 gaps)
**Duration:** 4-5 days  
**Files:** flatfile_importer.cpp (7), huggingface_ingestion_plugin.cpp (7), deterministic_matcher.cpp (3), gui_import_wizard.cpp (2)

**Gap Categories:**
- Null dereference: Flatfile (multiple), HuggingFace (multiple)
- Data race: HuggingFace progress tracking
- Blocking without timeout: Flatfile file I/O operations
- Smart ptr misuse: All 4 files

**Implementation Strategy:**
1. Standardize null-check patterns for file handles and result objects
2. Wrap all async operations in timeout guards
3. Replace raw allocations with smart pointers
4. Add exception-safe cleanup paths

**Acceptance Criteria:**
- [ ] All 19 gaps fixed or documented as deferred
- [ ] All 4 files compile without new warnings
- [ ] Focused tests IMPI-B-01..19 PASS
- [ ] Benchmark stable
- [ ] Git commit: `IMPORTERS-P2-BATCH-B: Fix 19 CRITICAL gaps (flatfile, huggingface, deterministic, gui)`

**Test Scope:** IMPI-B-01..19 (covering flatfile I/O safety, HF progress tracking, deterministic matching timeout, GUI handle lifecycle)

---

### Batch C: S3, Kafka, Oracle, SQLite, MongoDB, DataQuality (13 gaps)
**Duration:** 3-4 days  
**Files:** s3_importer.cpp (3), kafka_importer.cpp (2), oracle_importer.cpp (2), sqlite_importer.cpp (2), mongo_importer.cpp (3), data_quality.cpp (1)

**Gap Categories:**
- Blocking without timeout: Cloud connectors (S3, Kafka)
- Smart ptr misuse: All connectors
- Data race: MongoDB concurrent operations
- Resource leak: Oracle connection pooling

**Implementation Strategy:**
1. Standardize timeout semantics across cloud connectors (S3, Kafka)
2. Add mutex protection for shared connector state
3. Implement proper exception-safe connection cleanup
4. Replace allocator patterns with RAII

**Acceptance Criteria:**
- [ ] All 13 gaps fixed or documented as deferred
- [ ] All 6 files compile without new warnings
- [ ] Focused tests IMPI-C-01..13 PASS
- [ ] Benchmark stable
- [ ] Git commit: `IMPORTERS-P2-BATCH-C: Fix 13 CRITICAL gaps (s3, kafka, oracle, sqlite, mongo, data_quality)`

**Test Scope:** IMPI-C-01..13 (covering cloud connector timeouts, connection pool safety, concurrent access protection)

---

## Build & Test Verification

**CMake Preset:** `community-release-allow-missing-rocksdb` (standard for importers testing)

**Focused Test Execution:**
```bash
ctest -N importers | grep "focused" | wc -l  # Verify all focused tests registered
ctest -R "importers.*focused" --output-on-failure  # Run all focused tests
```

**Benchmark Verification:**
```bash
./build/community-release/benchmarks/importers/bench_importers_release_gates
# Expected: IMRG-01..06 all PASS with <±5% variance vs baseline
```

**Warning Check:**
```bash
cmake --build --preset community-release-allow-missing-rocksdb 2>&1 | grep -i "warning"
# Expected: Zero new warnings introduced
```

---

## Success Metrics

✅ **Phase 2 Exit Gate (CRITICAL 100%):**
- [ ] 44/44 gaps fixed (zero outstanding CRITICAL items)
- [ ] Compilation clean (zero new warnings)
- [ ] Focused tests 100% PASS
- [ ] Benchmarks stable (IMRG-01..06 no regression >±5%)
- [ ] Code review approved
- [ ] Ready for Phase 3 start

---

## Implementation Notes

1. **Null dereference pattern:** Always check weak_ptr.lock() result before use
   ```cpp
   if (auto h = weak_handle.lock()) {
       h->field = value;  // Safe
   }
   ```

2. **Data race pattern:** Use std::lock_guard for all shared state access
   ```cpp
   std::lock_guard<std::mutex> lock(custom_type_map_mutex_);
   auto it = custom_type_map_.find(key);
   ```

3. **Timeout pattern:** Use std::chrono::seconds with lock_guard or condition_variable timeout
   ```cpp
   std::unique_lock<std::mutex> lock(mutex_);
   if (!cv_.wait_for(lock, std::chrono::seconds(5), predicate)) {
       // Handle timeout
   }
   ```

4. **Smart pointer pattern:** Prefer std::make_unique and std::make_shared
   ```cpp
   auto handle = std::make_shared<ImportHandle>();  // Not: new ImportHandle()
   ```

---

## Related Documentation

- Module ROADMAP: `/home/runner/work/ThemisDB/ThemisDB/src/importers/ROADMAP.md`
- Build Status: `/home/runner/work/ThemisDB/ThemisDB/src/importers/BUILD_STATUS.md`
- Architecture: `/home/runner/work/ThemisDB/ThemisDB/src/importers/ARCHITECTURE.md`
- C++ best practices: `.github/instructions/cpp-best-practices.instructions.md`
- Documentation enforcement: `.github/instructions/documentation-enforcement.instructions.md`

---

## Dispatcher Notes

Phase 2 Batch A is the highest-priority work item. Start immediately once Phase 1 triage complete. Batches B and C can follow in sequence (same agent or separate agents for parallelism). Ensure focused test cases are comprehensive enough to catch regressions in subsequent phases.
