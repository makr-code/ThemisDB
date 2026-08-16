# Phase 2A Implementation: Data Race Critical Fixes (21 gaps)

**Phase:** 2A (Weeks 2-3, Aug 22 – Sep 5)  
**Agent Type:** `themisdb-implementer` (coding + build/test)  
**Scope:** 21 CRITICAL data_race gaps across 4 files  
**Blocker:** Sequential gate — MUST COMPLETE before Phase 2B/2C  
**Target Artifact:** `IMPORTERS_PHASE2A_DATA_RACE_FIXES_COMPLETE.md`

---

## Critical Data Race Cluster

**Files & Shared State:**

### 1. postgres_importer.cpp (1 CRITICAL data_race gap)
**Shared State:** `custom_type_map_` (mutable, accessed by:
- Main import thread: ParseColumnType() method
- Progress callback thread: weak_ptr.lock() callback → field access

**Gap Lines:** 2104, 2106 (find() without lock protection)

**Fix Pattern:**
```cpp
// Before:
class PostgreSQLImporter {
    std::unordered_map<std::string, int> custom_type_map_;  // Shared across threads
    
    int ParseColumnType(const std::string& type_name) {
        if (custom_type_map_.find(type_name) != custom_type_map_.end()) {
            return custom_type_map_[type_name];  // DATA RACE
        }
        return DEFAULT_TYPE;
    }
};

// After:
class PostgreSQLImporter {
    std::mutex type_map_mutex_;  // NEW
    std::unordered_map<std::string, int> custom_type_map_;
    
    int ParseColumnType(const std::string& type_name) {
        std::lock_guard<std::mutex> lock(type_map_mutex_);  // LOCK
        if (custom_type_map_.find(type_name) != custom_type_map_.end()) {
            return custom_type_map_[type_name];  // SAFE
        }
        return DEFAULT_TYPE;
    }
};
```

**Tests to Add:** IMPI-2A-PG-01 (concurrent type_map access, 1,000 iterations)

---

### 2. mysql_importer.cpp (8 CRITICAL data_race gaps)
**Shared State:** Multiple fields:
- `type_mapping_cache_` — concurrent map access
- `field_metadata_snapshot_` — progress callback reads while main thread writes
- `connection_pool_stats_` — concurrent statistics collection

**Gap Pattern:** type_mapping_cache_ accessed without lock in:
- Line N1: InitializeTypeMapping() → cache_.insert()
- Line N2: ResolveFieldType() → cache_.find()
- Line N3: GetCachedType() → cache_.at()
- Plus 5 similar in field_metadata_snapshot_, connection_pool_stats_

**Fix Strategy:**
- Add `std::mutex type_cache_mutex_`, `std::mutex metadata_mutex_`, `std::mutex stats_mutex_`
- Wrap all map access with `std::lock_guard<std::mutex> lock(type_cache_mutex_)`
- Verify no nested locks (lock ordering: type_cache → metadata → stats)

**Tests to Add:** IMPI-2A-MY-01..08 (8 tests covering each shared state, concurrent stress)

---

### 3. flatfile_importer.cpp (7 CRITICAL data_race gaps)
**Shared State:**
- `column_options_map_` — field validation options shared between parser and progress callback
- `field_validator_state_` — per-field validation state updated by multiple threads
- `schema_inference_cache_` — schema type hints cached across import batches

**Gap Pattern:** column_options_map_.find/insert without synchronization

**Fix Strategy:**
- Add mutex to flatfile_importer.cpp for column_options_map_
- Ensure progress callback uses lock before reading column_options_map_
- Add const accessor method that acquires lock internally

**Tests to Add:** IMPI-2A-FF-01..07 (7 concurrent field validation tests)

---

### 4. huggingface_ingestion_plugin.cpp (5 CRITICAL data_race gaps)
**Shared State:**
- `config_state_` — plugin configuration read by progress callback and main thread
- `progress_tracking_state_` — progress counters updated by worker threads and aggregator

**Gap Pattern:** progress_tracking_state_ updated without atomic or lock

**Fix Strategy:**
- Convert progress counters to `std::atomic<size_t>` where only increment/decrement needed
- Add mutex for compound updates (read-modify-write on config_state_)
- Add `std::lock_guard<std::mutex>` for all config_state_ access

**Tests to Add:** IMPI-2A-HF-01..05 (5 concurrent config/progress update tests)

---

## Implementation Strategy

### Week 1 (Day 1-3): postgres_importer + mysql_importer

**postgres_importer.cpp (Day 1, 1 gap):**
1. Add `std::mutex type_map_mutex_` member variable to PostgreSQLImporter class
2. Wrap custom_type_map_ access in lines 2104, 2106 with `std::lock_guard<std::mutex> lock(type_map_mutex_)`
3. Add test IMPI-2A-PG-01 (concurrent access, 1000 iterations, verify no race)
4. Build: `cmake --build --preset community-release-allow-missing-rocksdb --target module_importers_postgres_importer_focused`
5. Test: `ctest -R "importers_postgres_importer_focused" --output-on-failure`

**mysql_importer.cpp (Day 2-3, 8 gaps):**
1. Add mutexes: `type_cache_mutex_`, `metadata_mutex_`, `stats_mutex_`
2. Identify all cache access sites (type_mapping_cache_, field_metadata_snapshot_, connection_pool_stats_)
3. Wrap each with appropriate `std::lock_guard<std::mutex>`
4. Verify lock ordering (no deadlock): type_cache → metadata → stats
5. Add tests IMPI-2A-MY-01..08 (concurrent stress for each shared state)
6. Build + test: `cmake --build ... --target module_importers_mysql_importer_focused && ctest -R "importers_mysql_importer_focused"`

**Intermediate Verification:**
- Run: `ctest -R "importers.*focused" --output-on-failure` (ensures no regression in other modules)
- Run: `./build/community-release-allow-missing-rocksdb/benchmarks/importers/bench_importers_release_gates` (IMRG-01..06 stable)

### Week 1-2 (Day 4-6): flatfile_importer + huggingface_ingestion_plugin

**flatfile_importer.cpp (Day 4-5, 7 gaps):**
1. Add `std::mutex column_options_mutex_`, `std::mutex validator_state_mutex_`, `std::mutex schema_cache_mutex_`
2. Wrap column_options_map_, field_validator_state_, schema_inference_cache_ access
3. Ensure progress_callback uses lock before accessing shared state
4. Add tests IMPI-2A-FF-01..07
5. Build + test

**huggingface_ingestion_plugin.cpp (Day 6-7, 5 gaps):**
1. Add `std::mutex config_state_mutex_`
2. Convert atomic counters in progress_tracking_state_ to `std::atomic<size_t>` for increment/decrement
3. Wrap config_state_ read-modify-write operations with `std::lock_guard<std::mutex>`
4. Add tests IMPI-2A-HF-01..05
5. Build + test

**Final Verification (End of Week 2):**
- Compile all 4 files: `cmake --build --preset community-release-allow-missing-rocksdb 2>&1 | grep -i "error\|warning" | wc -l` (Expected: 0)
- Run all Phase 2A tests: `ctest -R "importers.*2A.*" --output-on-failure` (Expected: 21/21 PASS)
- Run benchmarks: `./build/community-release-allow-missing-rocksdb/benchmarks/importers/bench_importers_release_gates` (IMRG-01..06 stable)

---

## Test Coverage (21 focused tests: IMPI-2A-*)

| File | Gap Count | Test Cases | Pattern |
|------|-----------|-----------|---------|
| postgres_importer | 1 | IMPI-2A-PG-01 | Concurrent type_map access |
| mysql_importer | 8 | IMPI-2A-MY-01..08 | Type cache, metadata, stats under contention |
| flatfile_importer | 7 | IMPI-2A-FF-01..07 | Column options, field validator, schema cache |
| huggingface | 5 | IMPI-2A-HF-01..05 | Config state, progress tracking atomicity |

**Test Requirements:**
- Each test: 1000+ iterations of concurrent access (main thread + 2-4 worker threads)
- Verification: No data race detected by ThreadSanitizer
- Timeout: 120s per test
- Seed: kImportersConcurrencySeed = 42 (deterministic)

---

## Acceptance Criteria (Phase 2A Exit Gate)

✅ **All 21 data_race CRITICAL gaps fixed:**
- [ ] postgres_importer.cpp: 1/1 gap fixed (custom_type_map_ mutex)
- [ ] mysql_importer.cpp: 8/8 gaps fixed (3 mutex guards + lock ordering verified)
- [ ] flatfile_importer.cpp: 7/7 gaps fixed (3 mutex guards)
- [ ] huggingface_ingestion_plugin.cpp: 5/5 gaps fixed (config + atomic progress)

✅ **Compilation & Testing:**
- [ ] All 4 files compile without new warnings
- [ ] All 21 focused tests (IMPI-2A-*) PASS
- [ ] ThreadSanitizer detects zero new data races
- [ ] Benchmark gates IMRG-01..06 stable (±5% variance)

✅ **Code Quality:**
- [ ] Lock ordering documented (no deadlock risk)
- [ ] Exception-safe (lock_guard auto-releases on exception)
- [ ] No nested locks (single mutex per critical section)
- [ ] Comments explain shared state and mutex protection

✅ **Git Commit:**
- Message: `IMPORTERS-P2A-DATA-RACE: Fix 21 CRITICAL data_race gaps (postgres, mysql, flatfile, huggingface)`
- All 4 files modified in single commit
- 21 focused tests added to test suite

---

## Blockers & Risks

| Risk | Mitigation |
|------|-----------|
| std::mutex not available | Verify in cmake (should be standard C++11+) |
| Lock ordering deadlock | Document lock acquisition order; unit tests verify |
| Exception safety | Use std::lock_guard (RAII auto-releases) |
| Performance regression | Benchmark gates measure latency; tolerance ±5% |
| Test coverage incomplete | 1000+ iterations per test catches most races |

---

## Related Documentation

- Phase 1 Triage Report: `ai_working/IMPORTERS_PHASE1_GAP_TRIAGE.md` (sections: Data Race Cluster, Blocker Analysis)
- Master Coordination: `ai_working/IMPORTERS_GAP_CLOSURE_COORDINATION.md` (Phase 2 section)
- C++ Best Practices: `.github/instructions/cpp-best-practices.instructions.md` (threading section)
- Module ARCHITECTURE: `src/importers/ARCHITECTURE.md` (concurrency model)

---

## Success Indicators

✅ **Phase 2A is successful when:**
- All 21 data_race gaps closed with mutex/lock_guard implementation
- Zero new warnings during compilation
- 100% of focused tests (IMPI-2A-01..21) PASS
- Benchmarks stable (no regression >±5%)
- Code review approved (concurrency patterns validated)
- Ready for Phase 2B dispatch (exception safety)
