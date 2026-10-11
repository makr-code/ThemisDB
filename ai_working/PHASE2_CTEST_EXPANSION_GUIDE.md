---
Author: "copilot-swe-agent"
Created: "2026-10-11"
Last Updated: "2026-10-11"
Status: "ready"
---

# Phase 2 Roadmap: CTest Registration Expansion

**Scope:** Expand benchmark CTest coverage from 4% → 95%+ (338 → ~320+ active entries)  
**Timeline:** Week 2-3  
**Effort Estimate:** 4-6 hours (batch-driven automation)  
**Blockers Fixed by Phase 1:** 0 (audit infrastructure ready)

---

## Overview

Phase 2 focuses on systematic registration of benchmark CMake targets into CTest. Current state:

| Status | Count | Target |
|--------|-------|--------|
| CMake targets | 343 | - |
| CTest registered | 16 | 95%+ (320+) |
| **Coverage Gap** | **327** | **Eliminate via Phase 2** |

### Key Deliverables

1. **Fix 4 broken targets** (BROKEN_TARGET errors)
2. **Register 62 unregistered sources** in CMake
3. **Add CTest entries for ~300 targets** via `themis_register_benchmark_ctest()`
4. **Clean 18 orphaned CTest entries** (stale/incomplete registrations)
5. **Re-run audit** → verify exitcode 0

---

## Task Breakdown

### Task 1: Investigate & Fix Broken Targets (15 min)

**Affected targets:**
- `bench_arm_memory`
- `bench_arm_simd`
- `bench_async_io_multiscan`
- `bench_multithreading_comprehensive`

**Action:**
1. Locate each target's CMakeLists.txt registration
2. Check if source file exists (`base/bench_arm_*.cpp`, etc.)
3. Either:
   - Update path if source file moved
   - Remove target if source is permanently deleted
   - Create stub source if intentionally deferred

**Verification:**
```bash
python3 tools/ci/audit_benchmark_coverage.py --verbose | grep BROKEN_TARGET
# Should show 0 broken targets after fix
```

**Effort:** ~5 min per target

---

### Task 2: Register 62 Unregistered Sources (2 hours)

**Problem:** Source files exist but no CMake target registers them.

**Example files (from audit):**
- `analytics/bench_analytics_critical_paths_focused.cpp`
- `base/bench_multithreading_comprehensive.cpp` (after Task 1 fixes)
- `wave8/bench_w8a_incident_regression_shielding.cpp`
- `graph/bench_graph_release_gates.cpp`
- ... (58 more)

**Solution: Script-Assisted Batch Registration**

Create a helper script to auto-generate registration stubs:

```python
# tools/ci/generate_missing_benchmark_registrations.py
"""Generate themis_add_standard_benchmark() calls for unregistered sources."""

import json
from pathlib import Path

# Load audit findings
with open("benchmark_audit_baseline.json") as f:
    audit = json.load(f)

unregistered = [
    f for finding in audit["findings"]
    if finding["code"] == "UNREGISTERED_SOURCE"
    for f in finding["affected_items"]
]

for source_path in unregistered:
    # Parse: module/bench_<name>.cpp -> target: bench_<module>_<name>
    parts = source_path.replace(".cpp", "").split("/")
    if len(parts) == 2:
        module, filename = parts
        target_name = filename if filename.startswith("bench_") else f"bench_{module}_{filename}"
        print(f'themis_add_standard_benchmark({target_name} {source_path})')
    elif len(parts) == 1:
        # Root-level: bench_foo.cpp -> bench_foo
        target_name = parts[0]
        print(f'themis_add_standard_benchmark({target_name} {source_path})')
```

**Manual Process (if script unavailable):**

1. For each unregistered source, determine:
   - Module (first directory component)
   - Target name (e.g., `bench_graph_release_gates` from `graph/bench_graph_release_gates.cpp`)
   
2. Add to appropriate CMakeLists.txt:
   - If module-specific: `benchmarks/<module>/CMakeLists.txt`
   - If root-level: `benchmarks/CMakeLists.txt`

3. Registration pattern:
```cmake
themis_add_standard_benchmark(bench_<module>_<name> <module>/bench_<module>_<name>.cpp)
```

**Example additions (by module):**

**benchmarks/analytics/CMakeLists.txt:**
```cmake
themis_add_standard_benchmark(
    bench_analytics_critical_paths_focused
    analytics/bench_analytics_critical_paths_focused.cpp
)
```

**benchmarks/wave8/CMakeLists.txt:**
```cmake
themis_add_standard_benchmark(bench_w8a_incident_regression_shielding wave8/bench_w8a_incident_regression_shielding.cpp)
themis_add_standard_benchmark(bench_w8b_threshold_hardening_drift_detection wave8/bench_w8b_threshold_hardening_drift_detection.cpp)
themis_add_standard_benchmark(bench_w8c_deterministic_ci_harness wave8/bench_w8c_deterministic_ci_harness.cpp)
themis_add_standard_benchmark(bench_w8d_operability_runbooks_ownership wave8/bench_w8d_operability_runbooks_ownership.cpp)
```

**Effort:** ~2-3 hours (script-assisted batch, or ~5 min per source manually)

**Verification:**
```bash
python3 tools/ci/audit_benchmark_coverage.py --verbose | grep UNREGISTERED_SOURCE
# Should show 0 unregistered sources after registration
```

---

### Task 3: Add CTest Registrations for ~300 Targets (3-4 hours)

**Problem:** CMake targets exist but aren't exposed to CTest (327 missing).

**Solution:** Batch-add `themis_register_benchmark_ctest()` calls.

**Pattern (from benchmarks/cmake/BenchmarkPolicy.cmake):**

```cmake
themis_register_benchmark_ctest(
    NAME         <unique_ctest_name>
    TARGET       <cmake_target_name>
    MODULE       "<module_name>"
    TIMEOUT      120                           # or vary per benchmark
    EXTRA_LABELS release_critical              # only for critical benchmarks
)
```

**Implementation Strategy:**

**Step 1: Identify registration locations**

Group targets by module to organize registrations:

```cmake
# benchmarks/CMakeLists.txt (after all add_subdirectory calls)

# After main builds, add CTest registrations per module:

if(THEMIS_BUILD_BENCHMARKS)
    # Core benchmarks
    themis_register_benchmark_ctest(NAME bench_ycsb TARGET bench_ycsb MODULE core)
    themis_register_benchmark_ctest(NAME bench_phase3 TARGET bench_phase3_optimization MODULE performance)
    
    # Wave 8 benchmarks
    themis_register_benchmark_ctest(NAME bench_w8a TARGET bench_w8a_incident_regression_shielding MODULE wave8 TIMEOUT 180)
    themis_register_benchmark_ctest(NAME bench_w8b TARGET bench_w8b_threshold_hardening_drift_detection MODULE wave8 TIMEOUT 180)
    # ... continue for all targets
endif()
```

**Step 2: Generate batch CMake code**

Create `tools/ci/generate_ctest_registrations.py`:

```python
"""Generate themis_register_benchmark_ctest() calls for all CMake targets."""

import json
from pathlib import Path

# Load target mapping
with open("benchmark_target_mapping.json") as f:
    targets = json.load(f)

# Group by module
by_module = {}
for target_name, info in targets.items():
    module = info.get("module", "unknown")
    if module not in by_module:
        by_module[module] = []
    by_module[module].append((target_name, info))

# Generate CMake code per module
for module in sorted(by_module.keys()):
    print(f"\n# === Module: {module} ===")
    for target_name, info in by_module[module]:
        if info["is_registered_ctest"]:
            print(f"# ✓ {target_name} (already registered)")
            continue
        
        timeout = info["timeout_seconds"]
        labels = " ".join(info["labels"])
        
        print(f'themis_register_benchmark_ctest(NAME {target_name} TARGET {target_name} MODULE "{module}" TIMEOUT {timeout})')
```

**Step 3: Manual batch insertion**

For each module subdirectory with CMake targets:
1. Run the generator to get CMake code
2. Add registrations to end of `benchmarks/<module>/CMakeLists.txt`
3. Verify syntax: `cmake -P benchmarks/<module>/CMakeLists.txt`

**Example for wave8:**

```cmake
# benchmarks/wave8/CMakeLists.txt (end of file)

if(THEMIS_BUILD_BENCHMARKS AND TARGET bench_w8a_incident_regression_shielding)
    themis_register_benchmark_ctest(
        NAME bench_w8a_incident_regression_shielding
        TARGET bench_w8a_incident_regression_shielding
        MODULE "wave8"
        TIMEOUT 180
        EXTRA_LABELS "release_critical"
    )
endif()

if(THEMIS_BUILD_BENCHMARKS AND TARGET bench_w8b_threshold_hardening_drift_detection)
    themis_register_benchmark_ctest(
        NAME bench_w8b_threshold_hardening_drift_detection
        TARGET bench_w8b_threshold_hardening_drift_detection
        MODULE "wave8"
        TIMEOUT 180
        EXTRA_LABELS "release_critical"
    )
endif()
```

**Critical Benchmarks (release_critical):**

Prioritize these with `release_critical` label (from audit findings or ROADMAP):
- LLM benchmarks (llm, llama_cpp, llm_streaming)
- Sharding benchmarks (sharding, replication)
- Query/storage transaction (transaction, query, storage)
- GPU acceleration (gpu, cuda)

**Effort:**
- Generator script: 30 min
- Manual CMake additions: 2-3 hours (batch-driven per module)
- Syntax verification: 30 min

**Verification:**
```bash
# Test CMake parsing
cmake -P benchmarks/CMakeLists.txt

# Count registered tests
ctest --show-only | grep "bench_" | wc -l
# Should show ~320+ tests

# Run audit
python3 tools/ci/audit_benchmark_coverage.py --verbose | grep MISSING_CTEST
# Should show 0 missing registrations
```

---

### Task 4: Clean Up Orphaned CTest Entries (30 min)

**Problem:** 18 CTest entries reference incomplete/non-existent targets.

**Examples from audit:**
- `"${_target}"` (variable didn't expand)
- `"${_target}_wave_a_smoke"` (incomplete variable)
- `"bench_llm_inference_performance_smoke"` (target doesn't exist)

**Solution:**

1. Locate each orphaned entry in CMakeLists.txt (use grep):
```bash
cd /home/runner/work/ThemisDB/ThemisDB/benchmarks
grep -r "bench_llm_inference_performance_smoke" . --include="CMakeLists.txt"
grep -r '\${_target}' . --include="CMakeLists.txt"
```

2. For each found:
   - If variable expansion failed: fix the variable name
   - If target doesn't exist: either register the target or remove the entry
   - If stale/obsolete: remove the entry

3. Re-run audit to verify cleanup

**Effort:** ~10 min per entry × 18 = ~30 min total

---

## Execution Checklist

```
## Phase 2: CTest Registration Expansion

- [ ] Task 1: Fix broken targets (15 min)
  - [ ] Investigate bench_arm_memory
  - [ ] Investigate bench_arm_simd
  - [ ] Investigate bench_async_io_multiscan
  - [ ] Investigate bench_multithreading_comprehensive
  - [ ] Verify audit shows 0 BROKEN_TARGET errors

- [ ] Task 2: Register unregistered sources (2-3 hours)
  - [ ] Generate registration stubs for 62 sources
  - [ ] Add to module CMakeLists.txt files
  - [ ] Verify CMake parsing (cmake -P)
  - [ ] Verify audit shows 0 UNREGISTERED_SOURCE errors

- [ ] Task 3: Add CTest registrations for ~300 targets (3-4 hours)
  - [ ] Create generate_ctest_registrations.py script
  - [ ] Generate CMake code per module
  - [ ] Add registrations to benchmarks/<module>/CMakeLists.txt
  - [ ] Verify CMake syntax
  - [ ] Test ctest discovery: ctest --show-only | grep bench_
  - [ ] Verify audit shows 0 MISSING_CTEST_REGISTRATION errors
  - [ ] Verify CTest coverage ≥95%

- [ ] Task 4: Clean orphaned CTest entries (30 min)
  - [ ] Search for all orphaned entries
  - [ ] Fix or remove each entry
  - [ ] Verify audit shows 0 ORPHANED_CTEST errors

- [ ] Final Verification:
  - [ ] Run complete audit: python3 tools/ci/audit_benchmark_coverage.py
  - [ ] Verify exitcode 0 (AUDIT_PASSED)
  - [ ] Verify CTest coverage ≥95%
  - [ ] Verify all stats acceptable

- [ ] Commit & Push:
  - [ ] Stage all CMakeLists.txt changes
  - [ ] Commit: "Phase 2: Expand benchmark CTest coverage to 95%"
  - [ ] Push to branch
  - [ ] Update benchmark_target_mapping.json artifact
```

---

## Parallel Work During Phase 2

While Phase 2 CMakeLists.txt changes are being made, can start Phase 3 planning:

- **PR Gate Design:** `.github/workflows/gate-pr-benchmark-registration.yml`
- **Performance Tracking:** Enhance `benchmark-performance-gate.yml`
- **Expectation Files:** Draft `src/<module>/BENCHMARK_EXPECTATIONS.md` templates

---

## Success Criteria

| Criterion | Target | Verification |
|-----------|--------|---------------|
| CTest Coverage | ≥95% (320+ entries) | `python3 tools/ci/audit_benchmark_coverage.py` → stats shows ≥95% |
| BROKEN_TARGET | 0 | Audit finding count = 0 |
| UNREGISTERED_SOURCE | 0 | Audit finding count = 0 |
| ORPHANED_CTEST | 0 | Audit finding count = 0 |
| MISSING_CTEST_REGISTRATION | ≤10 (acceptable exceptions) | Audit shows <10 warnings |
| Audit Exitcode | 0 (PASSED) | `echo $?` = 0 |
| CMake Syntax | Valid | `cmake -P benchmarks/CMakeLists.txt` → no errors |

---

## Tools & Scripts Needed for Phase 2

**Already available:**
- ✅ `tools/ci/audit_benchmark_coverage.py` (Phase 1 deliverable)
- ✅ `benchmarks/cmake/BenchmarkPolicy.cmake` (existing)

**To create (optional automation):**
- `tools/ci/generate_missing_benchmark_registrations.py` (batch CMake stub generator)
- `tools/ci/generate_ctest_registrations.py` (batch CTest registration generator)

**Manual tools:**
- Text editor (for CMakeLists.txt edits)
- `grep` (for finding orphaned entries)
- `cmake` (for syntax validation)

---

## Timeline & Milestones

| Milestone | Timeline | Owner | Status |
|-----------|----------|-------|--------|
| Broken targets fixed | Day 1 (15 min) | Phase 2 lead | 📋 Pending |
| Unregistered sources registered | Days 1-2 (2-3 hours) | Phase 2 lead | 📋 Pending |
| CTest registrations added | Days 2-3 (3-4 hours) | Phase 2 lead | 📋 Pending |
| Orphaned entries cleaned | Day 3 (30 min) | Phase 2 lead | 📋 Pending |
| Final audit & verification | Day 3 (30 min) | Phase 2 lead | 📋 Pending |
| Commit & push to branch | Day 3 | Phase 2 lead | 📋 Pending |
| **Phase 2 Complete** | **End of Week 2** | - | 🔄 In Progress |

---

## Next Phase (Phase 3)

After Phase 2 verification:

**Phase 3: PR Gate Creation** (Week 3-4)
- Create `.github/workflows/gate-pr-benchmark-registration.yml`
- Automated prevention of future coverage regressions
- Labels + CI feedback

See: `BENCHMARK_COVERAGE_AND_GATES_PLAN.md` (Section 3)

---

**Phase 2 Status:** Ready to execute 🚀  
**Estimated Duration:** 4-6 hours (batch-driven)  
**Next: Await Phase 2 execution or proceed to Phase 3 planning**
