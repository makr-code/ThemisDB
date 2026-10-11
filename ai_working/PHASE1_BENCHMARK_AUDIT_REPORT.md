---
Author: "copilot-swe-agent"
Created: "2026-10-11"
Last Updated: "2026-10-11"
Status: "in-progress"
---

# Phase 1 Deliverable: Benchmark Coverage & Gates Audit Infrastructure

**Date:** 2026-10-11  
**Scope:** Enhancement of benchmark audit tooling (3-state validation: source → CMake → CTest)  
**Status:** Complete — Baseline audit executed, findings documented, remediation plan ready

---

## Executive Summary

**Benchmark coverage baseline audit completed.** Current state reveals significant coverage gaps:

| Metric | Value | Status |
|--------|-------|--------|
| Total benchmark sources discovered | 388 | ✓ |
| Intentionally excluded | 1 | ✓ |
| CMake targets | 343 | ✓ |
| CMake targets with source mapping | 329 | ⚠️ |
| **CTest registered** | **16** | ❌ **4% coverage** |
| Unregistered sources | 62 | ❌ Error |
| Orphaned CTest entries | 18 | ❌ Error |
| Broken targets (missing sources) | 4 | ❌ Error |

**Exit Code:** `1` (AUDIT_FAILED due to errors)

---

## 1. Enhanced Audit Tool

### 1.1 New Tool: `tools/ci/audit_benchmark_coverage.py`

**Purpose:** Three-state validation (source → CMake → CTest) with actionable JSON reporting.

**Capabilities:**

- **Discovery Phase:**
  - Scan `benchmarks/` for all benchmark source files (pattern: `bench*.cpp`, `benchmark_*.cpp`, `*_bench.cpp`)
  - Extract all CMake targets from `benchmarks/CMakeLists.txt` + module subdirectories
  - Parse CTest registrations from all CMakeLists.txt files

- **Validation Phase:**
  - **State 1 → 2:** Verify each benchmark source is registered as a CMake target
  - **State 2 → 3:** Verify each CMake target is registered in CTest
  - Detect orphaned/broken references at each transition

- **Output Modes:**
  - Console: Human-readable summary + detailed findings (with `--verbose`)
  - JSON: Structured audit report + findings array
  - Target Mapping: `benchmark_target_mapping.json` (like `test_target_mapping.json`)

**Usage:**
```bash
python3 tools/ci/audit_benchmark_coverage.py \
  --output /tmp/benchmark_audit.json \
  --target-mapping /tmp/benchmark_target_mapping.json \
  --verbose
```

**Exit Codes:**
- `0` = AUDIT_PASSED (no errors, warnings optional)
- `1` = AUDIT_FAILED (errors detected)
- `2` = AUDIT_FAILED_WITH_WARNINGS (only if `--fail-on-warning`)

### 1.2 Findings Categories

The audit detects four error/warning classes:

| Code | Severity | Meaning | Count |
|------|----------|---------|-------|
| `UNREGISTERED_SOURCE` | Error | Source file exists but no CMake target | **62** |
| `ORPHANED_CTEST` | Error | CTest entry references missing/incomplete target | **18** |
| `MISSING_CTEST_REGISTRATION` | Warning | CMake target exists but not exposed to CTest | **327** |
| `BROKEN_TARGET` | Error | CMake target references missing source file | **4** |

---

## 2. Baseline Audit Results (2026-10-11 10:13 UTC)

### 2.1 Summary Statistics

```
Sources discovered:       388
Intentionally excluded:   1
CMake targets:            343
  - with source mapping:  329
CTest registered:         16
  - coverage:             4%

Findings:
  - Errors:   3
  - Warnings: 1
```

### 2.2 Error Details

#### **ERROR 1: UNREGISTERED_SOURCE (62 sources)**

Benchmark sources that exist on disk but have no corresponding CMake target:

**Examples:**
- `analytics/bench_analytics_critical_paths_focused.cpp`
- `base/bench_multithreading_comprehensive.cpp`
- `base/bench_arm_memory.cpp`
- `base/bench_async_io_multiscan.cpp`
- `base/bench_arm_simd.cpp`
- `wave8/bench_w8a_incident_regression_shielding.cpp`
- `wave8/bench_w8b_threshold_hardening_drift_detection.cpp`
- `wave8/bench_w8d_operability_runbooks_ownership.cpp`
- `wave8/bench_w8c_deterministic_ci_harness.cpp`
- `graph/bench_graph_release_gates.cpp`
- `tensor/bench_tensor_release_gates.cpp`
- ... (52 more)

**Remediation:**
For each unregistered source, add to module-level or root CMakeLists.txt:
```cmake
themis_add_standard_benchmark(bench_<module>_<name> <module>/bench_<module>_<name>.cpp)
```

**Effort Estimate:** ~1-2 hours (script-assisted batch registration possible)

#### **ERROR 2: ORPHANED_CTEST (18 entries)**

CTest entries that reference non-existent or incomplete CMake targets:

**Examples:**
- `"bench_llm_inference_performance_smoke"`
- `"bench_sharding_release_gates_smoke"`
- `"bench_vector_search_dedicated_gates_smoke"`
- `"${_target}"` (variable expansion failed)
- `"${_target}_wave_a_smoke"` (variable expansion failed)
- `"bench_server_http3_gates_smoke"`
- `"bench_server_dedicated_gates_wave_d"`

**Root Cause:** Likely from incomplete template expansion or stale registrations.

**Remediation:**
1. Audit each orphaned entry's source CMakeLists.txt
2. Either:
   - Fix the template variable (if `${_target}` is used)
   - Register the corresponding CMake target
   - Remove the stale CTest entry

**Effort Estimate:** ~30 minutes (manual investigation)

#### **ERROR 3: BROKEN_TARGET (4 targets)**

CMake targets that reference source files that don't exist on disk:

**Targets:**
- `bench_arm_memory`
- `bench_arm_simd`
- `bench_async_io_multiscan`
- `bench_multithreading_comprehensive`

**Remediation:**
1. Verify if source should exist (check git history/comments)
2. Either:
   - Restore missing source file
   - Update CMakeLists.txt with correct path
   - Remove dead target

**Effort Estimate:** ~15 minutes per target (depends on whether sources are recoverable)

### 2.3 Warning Details

#### **WARNING: MISSING_CTEST_REGISTRATION (327 targets)**

CMake benchmark targets that exist but are not registered in CTest:

**Current CTest Coverage:** 16 / 343 = **4%**

This means 327 benchmark executables are built but **not exposed to CTest**, so:
- Cannot run via `ctest -L tier:benchmark`
- Cannot be validated in PR gates
- Cannot track performance regressions automatically

**Remediation:** Systematic CTest registration in Phase 2 (see roadmap below).

**Effort Estimate:** ~4-6 hours (Phase 2, batch-driven)

---

## 3. Target Mapping JSON

**File:** `benchmark_target_mapping.json` (generated)

**Structure:** Mirrors `test_target_mapping.json` for consistency.

**Example Entry:**
```json
{
  "bench_ycsb": {
    "name": "bench_ycsb",
    "source": "ycsb/bench_ycsb.cpp",
    "module": "ycsb",
    "ctest_entries": [],
    "is_registered_ctest": false,
    "timeout_seconds": 120,
    "labels": ["module:ycsb", "tier:benchmark", "kind:smoke"]
  },
  "bench_phase3_optimization": {
    "name": "bench_phase3_optimization",
    "source": "performance_optimizations/bench_phase3_optimization.cpp",
    "module": "performance_optimizations",
    "ctest_entries": ["bench_phase3_optimization_smoke"],
    "is_registered_ctest": true,
    "timeout_seconds": 120,
    "labels": ["module:performance_optimizations", "tier:benchmark", "kind:smoke"]
  }
}
```

---

## 4. Artifacts Generated

**Baseline Audit Report (JSON):**
- Location: `benchmark_audit_baseline.json`
- Content: Full audit stats + findings array
- Use: CI gate reference, issue tracking, trend analysis

**Target Mapping:**
- Location: `benchmark_target_mapping.json`
- Content: All 343 CMake targets with source/CTest metadata
- Use: PR gate validation, test report correlation

---

## 5. Remediation Roadmap (Phases 2-4)

### Phase 2: CTest Registration Expansion (Week 2-3)

**Goal:** Increase CTest coverage from 4% → 95%+ (300+ new CTest entries)

**Tasks:**
1. Fix 4 broken targets (BROKEN_TARGET findings)
2. Register 62 unregistered sources in CMake
3. Add CTest registrations for ~300 targets via `themis_register_benchmark_ctest()`
4. Clean up 18 orphaned CTest entries
5. Re-run audit → verify exitcode 0

**Deliverable:** All 343 targets registered, CTest coverage ≥95%

### Phase 3: PR Gate Creation (Week 3-4)

**Goal:** Automated gate to prevent future coverage regressions

**Gate Logic:**
- Run `audit_benchmark_coverage.py` on PR changes
- Fail if: new unregistered sources detected
- Fail if: orphaned/broken targets introduced
- Label: `quality/benchmark-registration-pass` or `-fail`

**Deliverable:** `.github/workflows/gate-pr-benchmark-registration.yml`

### Phase 4: Release Gate Integration (Week 4)

**Goal:** Block release if benchmark infrastructure broken

**Integration Points:**
- `gate-wave-closure.yml`: Require audit to pass before release
- Performance gate: Track regressions per module
- Issue upsert: Auto-open regression ticket on failure

**Deliverable:** Release-blocking benchmark gate + runbook

---

## 6. Success Criteria (Phase 1 Complete)

✅ **Completed:**
- Enhanced audit tool (`tools/ci/audit_benchmark_coverage.py`)
  - 3-state validation (source → CMake → CTest)
  - JSON output + target mapping generation
  - Exit codes for CI integration
- Baseline audit executed
  - 388 sources discovered
  - 62 errors (unregistered), 327 warnings (not in CTest), 18 errors (orphaned), 4 errors (broken)
- Findings documented in structured JSON
- Remediation plan for Phases 2-4 defined

✅ **Ready for:**
- Phase 2 batch registration work
- PR gate design (Phase 3)
- Release gate integration (Phase 4)

---

## 7. How to Use Phase 1 Artifacts

### Run Baseline Audit Again:
```bash
cd /home/runner/work/ThemisDB/ThemisDB
python3 tools/ci/audit_benchmark_coverage.py \
  --output /tmp/audit_$(date +%s).json \
  --target-mapping /tmp/benchmark_target_mapping.json \
  --verbose
```

### Integrate into CI (example):
```yaml
# .github/workflows/build-benchmarks.yml (or similar)
- name: Audit benchmark registration
  run: |
    python3 tools/ci/audit_benchmark_coverage.py \
      --output benchmark_audit_report.json \
      --target-mapping benchmark_target_mapping.json \
      --fail-on-warning
  continue-on-error: false

- name: Upload audit report
  uses: actions/upload-artifact@v4
  with:
    name: benchmark-audit-report
    path: benchmark_audit_report.json
```

### Query Results Programmatically:
```python
import json

with open("benchmark_audit_report.json") as f:
    report = json.load(f)

stats = report["stats"]
print(f"CTest Coverage: {stats['ctest_coverage_percent']}%")
print(f"Errors: {stats['errors']}")

for finding in report["findings"]:
    if finding["severity"] == "error":
        print(f"[ERROR] {finding['code']}: {finding['message']}")
```

---

## 8. Next Steps

**Immediate (before Phase 2):**
1. ✅ Copy `tools/ci/audit_benchmark_coverage.py` to repo
2. ✅ Run baseline audit and review findings
3. ✅ Document known issues/exclusions
4. Schedule Phase 2 registration work (Week 2-3)

**Phase 2 Kickoff:**
1. Prioritize the 4 broken targets (quick wins)
2. Register 62 unregistered sources (batch-driven, ~2 hours)
3. Add ~300 CTest entries (script-assisted, ~4-6 hours)
4. Clean up 18 orphaned CTest entries (manual, ~30 min)
5. Re-run audit for 95%+ CTest coverage

---

## Files Modified/Created

| File | Status | Purpose |
|------|--------|---------|
| `tools/ci/audit_benchmark_coverage.py` | ✅ Created | Main audit tool |
| `benchmark_audit_baseline.json` | ✅ Generated | Baseline findings |
| `benchmark_target_mapping.json` | ✅ Generated | Target metadata |
| `ai_working/BENCHMARK_COVERAGE_AND_GATES_PLAN.md` | ✅ Created | Execution roadmap |

---

## References

- **Benchmark Standards:** `benchmarks/BENCHMARK_STANDARDS.md`
- **CTest Registration Helper:** `benchmarks/cmake/BenchmarkPolicy.cmake`
- **Prior Audit:** `benchmarks/AUDIT.md` (outdated, re-executed 2026-10-11)
- **Target Mapping Template:** `test_target_mapping.json` (test side equivalent)

---

**Status:** Phase 1 infrastructure complete ✅  
**Next Action:** Proceed to Phase 2 (CTest registration expansion)
