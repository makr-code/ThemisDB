# Phase 3-4 HIGH Fixes Agent Specifications

**Phase:** 3-4 (Weeks 4-5, Sep 5-19, runs in parallel)  
**Agent Type:** `themisdb-implementer` (coding + build/test)  
**Total Scope:** 113 HIGH gaps across 6 files  
**Target Artifact:** `IMPORTERS_PHASE3_HIGH_BATCH_A1_COMPLETE.md` + `IMPORTERS_PHASE4_HIGH_BATCH_A2_COMPLETE.md`

---

## Overview

**Phase 3 (Batch A1):** postgres_importer.cpp (31), mysql_importer.cpp (15), mongo_importer.cpp (12) = 58 HIGH gaps  
**Phase 4 (Batch A2):** flatfile_importer.cpp (10), s3_importer.cpp (12), kafka_importer.cpp (12), oracle_importer.cpp (8), sqlite_importer.cpp (9), schema_inference.cpp (4) = 55 HIGH gaps

---

## Phase 3: HIGH Fixes Batch A1 (postgres/mysql/mongo)

**Duration:** 2-3 weeks  
**Target:** 58 HIGH gaps, ≥80% closure (≥47 gaps fixed)

### Categories & Fix Strategy

**1. Null Dereference (20 items)**
- postgres: Connection pooling, result parsing, statement preparation
- mysql: Result set handling, cursor iteration
- mongo: Document parsing, cursor traversal

**Fix Pattern:**
```cpp
// Before:
if (ct != custom_type_map_.end()) return ct->second;

// After:
if (ct != custom_type_map_.end() && ct->second != nullptr) {
    return ct->second;
}
```

**2. Uninitialized Container Access (18 items)**
- postgres: Column metadata vectors
- mysql: Field definition arrays
- mongo: Document field maps

**Fix Pattern:**
```cpp
// Before:
std::vector<Column> columns;  // Used immediately without bounds check
columns[0].name = "id";

// After:
std::vector<Column> columns;
if (!columns.empty()) {
    columns[0].name = "id";
} else {
    // Handle empty state
}
```

**3. Nested Loop O(n²) Patterns (15 items)**
- postgres: Foreign key detection (lines 951, 615-627)
- mysql: Index validation
- mongo: Schema matching

**Fix Pattern:**
```cpp
// Before:
for (const auto& line : sql_lines) {
    if (line.find("FOREIGN KEY") != std::string::npos) { ... }  // O(n) per iteration
}

// After:
std::unordered_set<std::string> foreign_key_markers = {"FOREIGN KEY", "REFERENCES", ...};
for (const auto& line : sql_lines) {
    if (foreign_key_markers.count(line) > 0) { ... }  // O(1) per iteration
}
```

**4. Exception Safety (5 items)**
- Transaction rollback paths
- Connection cleanup in error handling
- Resource cleanup in catch blocks

**Fix Pattern:**
```cpp
// Before:
try {
    auto handle = new ImportHandle();
    handle->run(...);
} catch (...) {
    delete handle;  // May be missed in some catch paths
    throw;
}

// After:
try {
    auto handle = std::make_shared<ImportHandle>();
    handle->run(...);
} catch (...) {
    // Auto-cleanup via smart pointer
    throw;
}
```

### Test Coverage

**Focused Test Suite:** IMPI-P3-01..58

- IMPI-P3-01..06: Null dereference paths in postgres connection pooling
- IMPI-P3-07..14: Uninitialized container access in result parsing
- IMPI-P3-15..20: Nested loop O(n²) detection and hash-based replacement
- IMPI-P3-21..24: Exception safety in transaction paths
- IMPI-P3-25..30: MySQL specific null checks and field validation
- IMPI-P3-31..40: MongoDB cursor safety and document traversal
- IMPI-P3-41..58: Integration tests for mixed source ingestion

**Benchmark Gates:** IMRG-01..06 stable (p99 ±5% of baseline)

### Acceptance Criteria

- [ ] ≥47 (80%) of 58 HIGH gaps fixed
- [ ] All 3 files compile without new warnings
- [ ] Focused tests IMPI-P3-01..58 ≥95% PASS
- [ ] p99 latency within ±5% of baseline
- [ ] No connector availability regressions
- [ ] Git commit: `IMPORTERS-P3-HIGH-A1: Fix 58 HIGH gaps (postgres, mysql, mongo) — ≥80% closure`

---

## Phase 4: HIGH Fixes Batch A2 (flatfile/s3/kafka/oracle/sqlite/schema_inference)

**Duration:** 2-3 weeks  
**Target:** 55 HIGH gaps, ≥80% closure (≥44 gaps fixed)

### Categories & Fix Strategy

**1. Schema Inference Hardening (4 items)**
- Bounds checking on schema metadata
- Cycle detection in type inference
- Null handling in type resolution

**2. Cloud Connector Validation (24 items)**
- S3 stream validation and error mapping
- Kafka offset management and retry logic
- Timeout enforcement on cloud API calls

**Fix Pattern:**
```cpp
// Before:
auto result = s3_client_.get_object(bucket, key);  // No timeout

// After:
auto future = s3_client_.get_object_async(bucket, key);
if (!future.wait_for(std::chrono::seconds(30))) {
    throw ImportException(IMPORT_TIMEOUT, "S3 get_object timeout");
}
auto result = future.get();
```

**3. SQLite/Oracle Connector Robustness (17 items)**
- Graceful degradation on missing capabilities
- Error code mapping for database-specific failures
- Connection pooling robustness

**4. Flatfile Parser Hardening (10 items)**
- Field validation and encoding error handling
- Boundary conditions (empty file, oversized records)
- Type inference robustness

### Test Coverage

**Focused Test Suite:** IMPI-P4-01..55

- IMPI-P4-01..04: Schema inference edge cases
- IMPI-P4-05..16: S3 stream validation and timeout
- IMPI-P4-17..24: Kafka offset and retry semantics
- IMPI-P4-25..32: SQLite connector degradation
- IMPI-P4-33..40: Oracle connection pool safety
- IMPI-P4-41..50: Flatfile parser boundary conditions
- IMPI-P4-51..55: Cross-connector integration tests

**Benchmark Gates:** IMRG-01..06 stable (p99 ±5% of baseline)

### Acceptance Criteria

- [ ] ≥44 (80%) of 55 HIGH gaps fixed
- [ ] All 6 files compile without new warnings
- [ ] Focused tests IMPI-P4-01..55 ≥95% PASS
- [ ] No regression in connector fallback paths
- [ ] Release gates stable
- [ ] Git commit: `IMPORTERS-P4-HIGH-A2: Fix 55 HIGH gaps (flatfile/s3/kafka/oracle/sqlite/schema) — ≥80% closure`

---

## Parallelization Notes

- **Phase 3 and Phase 4 can run in parallel** (different files, minimal shared state)
- **Dependency:** Both require Phase 2 CRITICAL to complete (no concurrent access conflicts)
- **Coordination:** Weekly sync on shared patterns (timeout enforcement, null checks, exception safety)
- **Test isolation:** Phase 3 tests use postgres/mysql/mongo mocks; Phase 4 tests use cloud/file mocks

---

## Build Verification

**Phase 3:**
```bash
cmake --build --preset community-release-allow-missing-rocksdb --target module_importers_postgres_importer_focused module_importers_mysql_importer_focused module_importers_mongo_importer_focused
ctest -R "importers.*postgres.*focused|importers.*mysql.*focused|importers.*mongo.*focused" --output-on-failure
```

**Phase 4:**
```bash
cmake --build --preset community-release-allow-missing-rocksdb --target module_importers_flatfile_importer_focused module_importers_s3_importer_focused module_importers_kafka_importer_focused module_importers_oracle_importer_focused module_importers_sqlite_importer_focused
ctest -R "importers.*flatfile.*focused|importers.*s3.*focused|importers.*kafka.*focused|importers.*oracle.*focused|importers.*sqlite.*focused|importers.*schema.*focused" --output-on-failure
```

**Benchmark:**
```bash
./build/community-release-allow-missing-rocksdb/benchmarks/importers/bench_importers_release_gates --benchmark_min_time=5s
```

---

## Success Metrics

✅ **Phase 3-4 Exit Gate (HIGH ≥80% closure):**
- [ ] ≥135 HIGH gaps fixed (80% of 169 Phase 3-4 total)
- [ ] All files compile clean (zero new warnings)
- [ ] ≥95% of focused tests PASS
- [ ] Benchmarks stable (IMRG-01..06 no regression >±5%)
- [ ] Connector availability verified
- [ ] Ready for Phase 5
