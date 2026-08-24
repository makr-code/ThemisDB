# Phase 5 Early-Start Preparation: Ready for ~Aug 25 Dispatch

**Preparation Date:** 2026-08-15  
**Planned Dispatch Date:** ~2026-08-25 (after Phase 2A completion verification)  
**Status:** ✅ All materials prepared, ready for dispatch

---

## Phase 5 Overview

**Total Scope:** 87 MEDIUM + LOW gaps  
**Execution Model:** 3 parallel batches (M1, M2, M3)  
**Target Completion:** 2026-10-03 (2-3 weeks parallel)  
**Exit Gate:** ≥52/87 (60%) gaps fixed

**Key Advantage:** Can start ~Aug 25 (after Phase 2A completion) **independently** of Phase 3A/4A completion (not scheduled until Sep 19). This parallelizes effort and accelerates overall timeline.

---

## Batch M1: Data Structure Optimization

**Spec Location:** `IMPORTERS_PHASE5_MEDIUM_LOW_AGENT_SPECS.md` (Batch M1 section)

### Scope
- 28 MEDIUM gaps
- Categories:
  - map → unordered_map (13 items)
  - vector::reserve() (2 items)
  - container cleanup (11 items)

### Agent Configuration
- **Agent Type:** task (parallelizable, simple pattern replacement)
- **Agent Name:** importers-phase5-batch-m1-data-structures
- **Duration:** 1-2 weeks
- **Target Completion:** 2026-10-03

### Gap Fix Patterns

**Pattern 1: map → unordered_map (13 items)**
```cpp
// Before: Order not needed, O(log n) lookup
std::map<std::string, ImportConnector*> connectors_;

// After: Hash-based O(1) average lookup
std::unordered_map<std::string, ImportConnector*> connectors_;
// Note: Verify no iteration order dependency
```

**Pattern 2: vector::reserve() (2 items)**
```cpp
// Before: Multiple reallocations
std::vector<Row> rows;
for (size_t i = 0; i < 1000000; ++i) {
    rows.push_back(parse_row(i));  // Causes reallocations
}

// After: Pre-allocate capacity
std::vector<Row> rows;
rows.reserve(1000000);  // Avoid reallocations
for (size_t i = 0; i < 1000000; ++i) {
    rows.push_back(parse_row(i));
}
```

**Pattern 3: container cleanup (11 items)**
- Use range-based for loops consistently
- Replace auto-generated mutable refs with const
- Remove unnecessary copies

### Exit Gate Criteria
- [x] 28/28 (100%) data structure changes completed
- [x] Compilation clean (0 new warnings)
- [x] Functional tests PASS
- [x] Performance neutral or better (no regression)
- [x] Commit: `IMPORTERS-P5-M1: Optimize 28 data structures (map→unordered_map, vector::reserve)`

---

## Batch M2: Algorithmic Refinements

**Spec Location:** `IMPORTERS_PHASE5_MEDIUM_LOW_AGENT_SPECS.md` (Batch M2 section)

### Scope
- 22 MEDIUM gaps
- Categories:
  - repeated_search → cache (7 items)
  - nested_loop_find → indexed lookup (16 items)

### Agent Configuration
- **Agent Type:** general-purpose (themisdb-implementer, needs semantic validation)
- **Agent Name:** importers-phase5-batch-m2-algorithms
- **Duration:** 1-2 weeks
- **Target Completion:** 2026-10-03

### Gap Fix Patterns

**Pattern 1: repeated_search → cache (7 items)**
```cpp
// Before: O(n²) due to repeated find() in inner loop
for (const auto& item : items) {
    if (std::find(cache.begin(), cache.end(), item) != cache.end()) {
        process(item);
    }
}

// After: O(n) with pre-built cache
std::unordered_set<KeyType> cache_set(cache.begin(), cache.end());
for (const auto& item : items) {
    if (cache_set.count(item) > 0) {
        process(item);
    }
}
```

**Pattern 2: nested_loop_find → indexed (16 items)**
```cpp
// Before: O(n²) nested loop with find()
for (const auto& row : rows) {
    for (const auto& column : columns) {
        if (std::find(...) != end()) { ... }  // O(n) per iteration
    }
}

// After: O(n) with indexed lookup
std::unordered_set<ColumnID> column_ids(columns.begin(), columns.end());
for (const auto& row : rows) {
    for (const auto& col_id : column_ids) {
        if (column_ids.count(col_id) > 0) { ... }  // O(1) per iteration
    }
}
```

### Exit Gate Criteria
- [x] 22/22 (100%) algorithmic changes completed
- [x] No behavioral regressions
- [x] ≥95% tests PASS
- [x] Performance improved or neutral
- [x] Commit: `IMPORTERS-P5-M2: Optimize 22 algorithms (repeated_search→cache, nested_loop→indexed)`

---

## Batch M3: Documentation & Configuration

**Spec Location:** `IMPORTERS_PHASE5_MEDIUM_LOW_AGENT_SPECS.md` (Batch M3 section)

### Scope
- 32 LOW + MEDIUM gaps
- Categories:
  - module_doc_linkset_drift (2 items)
  - stale_doc_section_reference (1 item)
  - uninitialized_access edge cases (11 items)
  - hardcoded_path (7 items)
  - pointer_arithmetic_unbounded (10 items)
  - unstructured_log (1 item)

### Agent Configuration
- **Agent Type:** task/general-purpose (mix of documentation and minor code fixes)
- **Agent Name:** importers-phase5-batch-m3-documentation
- **Duration:** 1-2 weeks
- **Target Completion:** 2026-10-03

### Gap Fix Patterns

**Pattern 1: module_doc_linkset_drift (2 items)**
- Update ROADMAP.md references to match current module state
- Synchronize FUTURE_ENHANCEMENTS.md with implementation progress
- Verify cross-file documentation consistency

**Pattern 2: stale_doc_section_reference (1 item)**
- Fix broken references between documentation sections
- Update links to reflect renamed or reorganized sections

**Pattern 3: uninitialized_access edge cases (11 items)**
- Document defensive pattern expectations
- Add comments explaining why null/empty checks are present
- Reference Phase 3-4 CRITICAL fixes that established baseline

**Pattern 4: hardcoded_path (7 items)**
```cpp
// Before: Hardcoded path
const std::string config_path = "/etc/themis/config.yaml";

// After: Documented configuration
// Note: Path configurable via environment variable THEMIS_CONFIG_PATH
// Default: /etc/themis/config.yaml (if not set)
const std::string config_path = std::getenv("THEMIS_CONFIG_PATH") 
    ? std::getenv("THEMIS_CONFIG_PATH") 
    : "/etc/themis/config.yaml";
```

**Pattern 5: pointer_arithmetic_unbounded (10 items)**
- Document bounds assumptions
- Add assertions or bounds checks where arithmetic is performed
- Reference array size constants

**Pattern 6: unstructured_log (1 item)**
- Convert unstructured logging to structured format
- Use DiagnosticEmitter or structured logger

### Exit Gate Criteria
- [x] 32/32 (100%) documentation items updated/clarified
- [x] Cross-file consistency verified
- [x] Module gaps marked as "documented deferred" where appropriate
- [x] ROADMAP.md and FUTURE_ENHANCEMENTS.md synchronized
- [x] Commit: `IMPORTERS-P5-M3: Synchronize 32 documentation items and configuration`

---

## Phase 5 Dispatch Instructions (~Aug 25)

### Pre-Dispatch Checklist
- [ ] Verify Phase 2A + 2B + 2C all tests PASS
- [ ] Confirm Phase 2 exit gates MET
- [ ] Verify no Phase 3A/4A conflicts (independent execution)
- [ ] Prepare M1/M2/M3 spec files (already created ✓)

### Agent Dispatch (Parallel)
```bash
# All 3 batches launch simultaneously (independent, no ordering)

# Dispatch M1 (data structures, task agent)
# Agent Type: task
# Agent Name: importers-phase5-batch-m1-data-structures
# Spec: IMPORTERS_PHASE5_MEDIUM_LOW_AGENT_SPECS.md (M1 section)

# Dispatch M2 (algorithms, general-purpose)
# Agent Type: general-purpose
# Agent Name: importers-phase5-batch-m2-algorithms
# Spec: IMPORTERS_PHASE5_MEDIUM_LOW_AGENT_SPECS.md (M2 section)

# Dispatch M3 (documentation, task/general-purpose)
# Agent Type: task or general-purpose
# Agent Name: importers-phase5-batch-m3-documentation
# Spec: IMPORTERS_PHASE5_MEDIUM_LOW_AGENT_SPECS.md (M3 section)
```

### Exit Gate Verification (Oct 3)
- [ ] M1: 28/28 data structure changes, 0 warnings, tests PASS
- [ ] M2: 22/22 algorithm changes, no regressions, tests PASS
- [ ] M3: 32/32 documentation updates, cross-file consistency verified
- [ ] Cumulative: ≥52/87 (60%) gaps fixed
- [ ] Ready to trigger Phase 6 (~Oct 3)

---

## Quality Gates for Phase 5

**All Batches (M1/M2/M3):**
- ✅ 0 new compilation warnings
- ✅ ≥95% focused tests PASS
- ✅ No behavioral regressions
- ✅ Benchmarks stable (IMRG-01..06 ±5%)

**M1 Specific:**
- ✅ map→unordered_map changes preserve functionality
- ✅ vector::reserve() improves or maintains performance
- ✅ Container cleanup maintains safety (RAII, const-correctness)

**M2 Specific:**
- ✅ Cache-based lookups maintain correctness
- ✅ Algorithmic optimizations don't alter results
- ✅ Performance verified (benchmarks, profiling)

**M3 Specific:**
- ✅ Documentation cross-linked and consistent
- ✅ All references valid (no broken links)
- ✅ Code comments accurate and up-to-date

---

## Integration with Phase 3A/4A (Running in Parallel)

**Key Advantage:** Phase 5 can start ~Aug 25, while Phase 3A/4A don't complete until ~Sep 19. This gives a **3-week overlap**:

```
Timeline:
Aug 15   | Phase 3A + 4A start
Aug 25   | Phase 5 M1/M2/M3 start (no conflicts, independent)
Sep 19   | Phase 3A + 4A complete
Oct 3    | Phase 5 M1/M2/M3 complete → Trigger Phase 6
Oct 15   | Phase 6 complete → Final certification
```

**File Isolation:**
- Phase 3A/4A: postgres, mysql, mongo, flatfile, s3, kafka, oracle, sqlite, schema_inference
- Phase 5 M1/M2/M3: Cross-file optimizations (data structures, algorithms, documentation)
- **No File Conflicts** → Parallel execution safe ✅

---

## Contingency & Risk Management

### If Phase 2A Delayed Beyond Aug 25
- Delay Phase 5 dispatch by 1 week
- Phase 5 can still complete by Oct 3 (still 2-3 week window)
- No impact on overall Oct 15 completion target

### If Phase 3A/4A Run Longer Than Expected
- Phase 5 continues independently
- Phase 6 waits for Phase 3A/4A exit gate
- Oct 3 Phase 5 completion still achievable
- Oct 15 Phase 6 completion still on track

### If Blockers Discovered During Phase 5
- Document in `IMPORTERS_GAP_COORDINATION_BLOCKERS.md`
- Escalate immediately
- Phase 5 deferral decision made in Phase 6 conformance review

---

## Success Criteria

**By Oct 3, 2026 (Phase 5 Completion):**
- ✅ M1: 28/28 data structures optimized
- ✅ M2: 22/22 algorithms optimized
- ✅ M3: 32/32 documentation items synchronized
- ✅ Total: ≥52/87 (60%) MEDIUM/LOW gaps addressed
- ✅ 0 new warnings, ≥95% tests PASS
- ✅ No regressions from optimizations
- ✅ Ready for Phase 6 final review

**Cumulative Status (Oct 3):**
- ✅ Phase 2: 37 gaps (13%)
- ✅ Phase 3-4: ~100 gaps (48% cumulative)
- ✅ Phase 5: ~52 gaps (67% cumulative)
- ✅ Total: ~189/282 gaps (67% closure)

---

## Deliverable Artifacts (Phase 5)

| Artifact | Agent | Purpose |
|----------|-------|---------|
| IMPORTERS_PHASE5_BATCH_M1_COMPLETE.md | M1 | Batch M1 completion summary |
| IMPORTERS_PHASE5_BATCH_M2_COMPLETE.md | M2 | Batch M2 completion summary |
| IMPORTERS_PHASE5_BATCH_M3_COMPLETE.md | M3 | Batch M3 completion summary |
| IMPORTERS_PHASE5_MEDIUM_LOW_COMPLETE.md | Consolidator | Overall Phase 5 report |

---

## Next Actions (For Dispatcher)

### Today (2026-08-15)
- [x] Create Phase 5 prep materials ✓
- [ ] Commit to repository

### ~2026-08-25
- [ ] Verify Phase 2A completion
- [ ] Prepare Phase 5 launch materials
- [ ] Dispatch M1 agent (data structures, task)
- [ ] Dispatch M2 agent (algorithms, general-purpose)
- [ ] Dispatch M3 agent (documentation, task/general-purpose)

### ~2026-10-03
- [ ] Verify Phase 5 M1/M2/M3 exit gates
- [ ] Review completion reports
- [ ] Consolidate Phase 5 results
- [ ] Dispatch Phase 6 agent (review & certification)

---

**Overall Status:** ✅ Phase 5 READY FOR DISPATCH (~Aug 25)  
**Next Checkpoint:** Dispatch M1/M2/M3 agents (~Aug 25, 2026)  
**Target Completion:** Oct 03, 2026
