# Phase 5 MEDIUM & LOW Fixes Agent Specifications

**Phase:** 5 (Weeks 6-8, Sep 19 – Oct 3, parallel with Phase 3-4 completion)  
**Agent Type:** `task` agents (bulk operations) + `themisdb-implementer` (complex logic)  
**Total Scope:** 87 MEDIUM + LOW gaps (82 MEDIUM, 5 LOW)  
**Target Artifact:** `IMPORTERS_PHASE5_MEDIUM_LOW_COMPLETE.md`

---

## Batching Strategy

### Batch M1: Data Structure Optimization (28 items)
**Duration:** 1-2 weeks  
**Agent Type:** `task` (parallelizable)

**Categories:**
- map_vs_unordered_map (13 items)
- missing_vector_reserve (2 items)
- size_assumption (2 items)
- Other container patterns (11 items)

**Implementation Strategy:**

1. **map → unordered_map migration (13 items)**
   - Identify maps where order is not required
   - Replace std::map with std::unordered_map
   - Verify hash function exists or provide custom hash
   - Add comments explaining hash distribution assumptions

   ```cpp
   // Before:
   std::map<std::string, ImportConnector*> connectors_;  // Ordered but slower
   
   // After:
   std::unordered_map<std::string, ImportConnector*> connectors_;  // O(1) avg lookup
   ```

2. **Vector reserve optimization (2 items)**
   - Identify loops that grow vectors via push_back
   - Calculate expected size if deterministic
   - Add reserve() call before loop to avoid reallocations

   ```cpp
   // Before:
   std::vector<Row> rows;
   for (size_t i = 0; i < 1000000; ++i) {
       rows.push_back(parse_row(i));  // Causes reallocations
   }
   
   // After:
   std::vector<Row> rows;
   rows.reserve(1000000);  // Pre-allocate capacity
   for (size_t i = 0; i < 1000000; ++i) {
       rows.push_back(parse_row(i));
   }
   ```

3. **Container pattern cleanup (11 items)**
   - Remove unnecessary copies from containers
   - Use range-based for loops consistently
   - Replace auto-generated mutable references with const

**Test Coverage:** IMPI-P5M1-01..28
- Functionality regression tests for map→unordered_map
- Performance validation (ensure hash-based lookup faster)
- Iterator invariant tests

**Acceptance Criteria:**
- [ ] 28/28 (100%) data structure changes completed
- [ ] Compilation clean (zero new warnings)
- [ ] Functional tests PASS
- [ ] Performance tests show improvement or neutral (no regression)
- [ ] Git commit: `IMPORTERS-P5-BATCH-M1: Optimize 28 data structures (map→unordered_map, vector::reserve)`

---

### Batch M2: Algorithmic Refinements (22 items)
**Duration:** 1-2 weeks  
**Agent Type:** `themisdb-implementer` (needs semantic validation)

**Categories:**
- nested_loop_find → hash-based lookup (7 items)
- repeated_search → cached results (7 items)
- range_temporary cleanup (2 items)
- Other algorithmic patterns (6 items)

**Implementation Strategy:**

1. **Nested loop string search optimization (7 items)**
   - Identify string::find() calls inside loops
   - Replace with pre-computed hash set or trie
   - Measure performance improvement (target 5-10x speedup)

   ```cpp
   // Before:
   for (const auto& sql_stmt : statements) {
       if (sql_stmt.find("FOREIGN KEY") != std::string::npos ||
           sql_stmt.find("REFERENCES") != std::string::npos ||
           sql_stmt.find("CASCADE") != std::string::npos) {
           // Process statement  O(n*m) complexity
       }
   }
   
   // After:
   static const std::unordered_set<std::string> sql_keywords{
       "FOREIGN KEY", "REFERENCES", "CASCADE"
   };
   for (const auto& sql_stmt : statements) {
       if (sql_keywords.count(sql_stmt) > 0) {
           // Process statement  O(n) complexity
       }
   }
   ```

2. **Repeated search caching (7 items)**
   - Identify redundant queries/searches in hot paths
   - Add local cache or memoization
   - Document cache invalidation strategy

   ```cpp
   // Before:
   for (const auto& column : columns) {
       if (type_registry_.find(column.type_name) != type_registry_.end()) {
           // Query type_registry multiple times per loop iteration
       }
   }
   
   // After:
   std::unordered_map<std::string, TypeInfo> type_cache;
   for (const auto& column : columns) {
       if (type_cache.find(column.type_name) == type_cache.end()) {
           type_cache[column.type_name] = type_registry_.lookup(column.type_name);
       }
       if (type_cache.at(column.type_name)) {
           // Use cached lookup
       }
   }
   ```

3. **Range temporary cleanup (2 items)**
   - Remove unnecessary temporary objects in ranges
   - Use range adapters or views where available

**Test Coverage:** IMPI-P5M2-01..22
- Performance regression tests (ensure speedup or neutral)
- Correctness tests for cached/optimized paths
- Load tests to verify cache invalidation

**Acceptance Criteria:**
- [ ] 22/22 (100%) algorithmic improvements completed
- [ ] Compilation clean
- [ ] Functional tests PASS (correctness validation)
- [ ] Benchmark improvement ≥5% or neutral (no regression)
- [ ] Git commit: `IMPORTERS-P5-BATCH-M2: Refactor 22 algorithms (nested loops→hash, repeated search→cache)`

---

### Batch M3: Documentation & Cross-File Consistency (32 items)
**Duration:** 1-2 weeks  
**Agent Type:** `task` + `doc-orchestrator`

**Categories:**
- module_doc_linkset_drift (2 items)
- stale_doc_section_reference (1 item)
- unstructured_log (1 item)
- hardcoded_path (7 items)
- hardcoded_output (2 items)
- copy_overhead (2 items)
- Other patterns (16 items)

**Implementation Strategy:**

1. **Documentation Synchronization (3 items)**
   - Verify all public APIs documented (Doxygen comments)
   - Update module README/ARCHITECTURE to match current design
   - Link source truth references in docs

   **Acceptance Criteria for Docs:**
   - [ ] README.md reflects current module architecture
   - [ ] ARCHITECTURE.md covers all connector types
   - [ ] API documentation complete for public interfaces
   - [ ] Cross-references between docs verified (no stale links)

2. **Remove Hardcoded Paths (7 items)**
   - Identify hardcoded file paths in code
   - Replace with configuration or environment variables
   - Add comments documenting parameterization

   ```cpp
   // Before:
   const std::string LOG_PATH = "/var/log/themis/importers.log";
   
   // After:
   const std::string LOG_PATH = getenv("THEMIS_LOG_PATH") 
       ? getenv("THEMIS_LOG_PATH") 
       : "/var/log/themis/importers.log";
   ```

3. **Copy Overhead Reduction (2 items)**
   - Identify unnecessary copies in function parameters
   - Replace with const references where applicable
   - Use std::move for return values

4. **Code Comment Cleanup (16 items)**
   - Standardize inline comments (explain "why", not "what")
   - Update outdated comments
   - Ensure structured log messages (avoid ad-hoc formatting)

**Test Coverage:** IMPI-P5M3-01..32
- Documentation completeness check
- Comment correctness review (manual or lint-based)
- Hardcoded path validation tests

**Acceptance Criteria:**
- [ ] 32/32 (100%) documentation and style changes completed
- [ ] All hardcoded paths removed or parameterized
- [ ] Documentation linkset verified (no stale references)
- [ ] Log messages structured and queryable
- [ ] Git commit: `IMPORTERS-P5-BATCH-M3: Docs & consistency — 32 items (hardcoded paths, doc sync, code style)`

---

## Parallelization

**Phase 5 Full Parallelization:**
- Batch M1 (data structures): Deploy 2-3 task agents in parallel
- Batch M2 (algorithms): Deploy 1-2 themisdb-implementer agents (needs semantic validation)
- Batch M3 (documentation): Deploy 1 doc-orchestrator agent + 1-2 task agents for cleanup

**Expected Throughput:** All 87 MEDIUM/LOW items closed in 3-4 weeks

---

## Build & Test Verification

**For each batch:**
```bash
# Compile check
cmake --build --preset community-release-allow-missing-rocksdb 2>&1 | grep -i "warning" | wc -l
# Expected: 0

# Focused test execution
ctest -R "importers.*P5.*focused" --output-on-failure
# Expected: ≥95% PASS

# Benchmark verification
./build/community-release-allow-missing-rocksdb/benchmarks/importers/bench_importers_release_gates
# Expected: No regression >±5%
```

---

## Success Metrics

✅ **Phase 5 Exit Gate (MEDIUM/LOW ≥60% closure):**
- [ ] ≥52 (60%) of 87 MEDIUM/LOW gaps fixed
- [ ] All files compile clean (zero new warnings)
- [ ] Focused tests ≥90% PASS
- [ ] Benchmarks stable (IMRG-01..06 no regression)
- [ ] Documentation updated and cross-linked
- [ ] Ready for Phase 6 review

---

## Performance Impact Targets

| Batch | Operation | Expected Impact | Gate |
|-------|-----------|-----------------|------|
| M1 | Container lookup | 10-50% speedup (map→unordered_map) | p99 ±5% acceptable |
| M2 | Nested loops | 5-10x speedup (string search→hash) | p99 ±5% acceptable |
| M3 | Documentation | No performance impact | Documentation quality |

---

## Notes for Dispatcher

- Phase 5 can start as soon as Phase 2 CRITICAL completes (no dependency on Phase 3-4)
- Batch M1 and M3 are highly parallelizable; deploy multiple agents
- Batch M2 requires themisdb-implementer (semantic validation of correctness)
- Expected to complete before Phase 3-4 finish (M1+M3 are fast track)
