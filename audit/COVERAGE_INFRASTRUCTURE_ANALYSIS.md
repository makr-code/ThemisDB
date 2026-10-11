---
Author: "Copilot Task Agent"
Created: "2026-10-11"
Last Updated: "2026-10-11"
Status: "Block C.1 - Infrastructure Analysis Complete"
---

# Coverage Infrastructure Analysis (Block C.1)

## Current State Assessment

### Existing Coverage Infrastructure

**Baseline File:**
- Location: `.github/cmake-source-coverage-baseline.txt` (27.5 KB)
- Format: Line-delimited source files (benchmarks, tests, plugins)
- Purpose: Audit artifact tracking source file coverage status
- Last Updated: 2026-03-27T09:18:02
- Notes: Represents "Kategorie A" files (neither path nor filename found in CMake)

**Doxygen Coverage:**
- File: `Doxyfile.coverage` (exists, configured)
- Validation: `gate-pr-doxygen-governance.yml` enforces Doxygen completeness
- Metrics: Symbol presence ratio (per-module documentation coverage)
- Type: API-level documentation coverage, not code execution coverage

**Existing Coverage Reports:**
- Location: `./artifacts/nightly/audit/coverage_report.{md,json}`
- Workflow: `build-benchmarks.yml` generates traffic-light coverage report
- Script: `benchmarks/nightly_coverage_report.py`
- Frequency: Nightly (post-build)
- Output: Markdown + JSON aggregate metrics

**Python Coverage Tools:**
- `tools/bench_coverage_report.py` — Coverage reporting from benchmark runs
- `tools/perf_coverage_top10_audit.py` — Performance/coverage analysis
- `scripts/integration_test_coverage.sh` — Integration test coverage wrapper

### Compiler Support Analysis

**GCC/Clang:**
- Support: Full (via `--coverage` flag or `-fprofile-arcs -ftest-coverage`)
- Flags Not Currently Set: CMake does not define coverage flags
- lcov Integration: Available (not configured)
- Status: Ready to enable on Linux platforms

**MSVC (Windows):**
- Support: Partial (`/PROFILE` for instrumentation)
- Recommendation: Use OpenCppCoverage tool wrapper for MSVC
- Status: Complex, requires post-build instrumentation
- Alternative: Skip coverage on MSVC, report Linux-only baselines

**Clang (macOS):**
- Support: Full (`-fprofile-instr-generate -fcoverage-mapping`)
- Flags Not Currently Set: CMake does not define coverage flags
- llvm-cov Integration: Available
- Status: Ready to enable on macOS

### Existing Gate Workflow Infrastructure

**Release-Critical Tests Gate:**
- Workflow: `gate-pr-core.yml`
- Filter: `--label-regex "release_critical"`
- Aggregate Target: `themis_release_critical_tests`
- Current Tests: ~30 marked tests (per FOCUSED_TEST_GATES.md)
- Status: Ready, no changes needed

**Focused Module Tests Gates:**
- Workflows: `build-*-fast.yml` lanes (clang, gcc, sanitizer, gpu)
- Filter: `--label-regex "focused"`
- Aggregate Target: `themis_focused_targets` (per-module)
- Current Tests: ~496 targets (per FOCUSED_TEST_GATES.md)
- Status: Ready, no changes needed

**Benchmark Performance Gate:**
- Workflow: `benchmark-performance-gate.yml`
- Filter: Explicit targets + label filtering
- Aggregate Target: `themis_benchmarks_all_eligible`
- Current Tests: ~19 aggregate targets
- Status: Ready, no changes needed

### Gap Analysis: What's Missing

**Code Execution Coverage (Block C Priority):**
1. ❌ No gcov/lcov compiler flags configured
2. ❌ No coverage collection in CI workflows
3. ❌ No baseline per-module metrics (.gcda/.profraw aggregation missing)
4. ❌ No HTML report generation (lcov → HTML)
5. ❌ No coverage gate enforcement (pass/fail threshold)
6. ❌ No nightly automation for baseline tracking
7. ❌ No branch coverage metrics (statement + branch)

**Test Count vs. Coverage Correlation:**
- Currently: 1,562 binaries, ~4,246 test files
- Metric: Test count ≠ execution coverage
- Required: Measure % of code lines executed by tests
- Status: Will be enabled with Phase 3 tests

---

## Block C Implementation Plan

### C1: Enable Coverage Flags in CMake

**What to Add:**

```cmake
# In CMakeLists.txt root level
option(THEMIS_ENABLE_COVERAGE "Enable code coverage instrumentation (GCC/Clang only)" OFF)

if(THEMIS_ENABLE_COVERAGE)
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        add_compile_options(--coverage)
        add_link_options(--coverage)
        message(STATUS "Coverage instrumentation enabled (--coverage)")
    else()
        message(WARNING "Code coverage not supported for ${CMAKE_CXX_COMPILER_ID}")
    endif()
endif()
```

**How to Enable:**
```bash
cmake -B build -DTHEMIS_ENABLE_COVERAGE=ON -DTHEMIS_BUILD_TESTS=ON
cmake --build build --parallel 8
```

**Compiler Impact:**
- GCC: Adds 10-15% build time, 20-30% test time overhead
- Clang: Similar impact, generates `.profraw` files
- Size: Instrumented binaries ~2-3x larger

---

### C2: Coverage Baseline Measurement Strategy

**Baseline Metrics File Format:**

```
# Module Coverage Baseline (2026-10-11)
# Format: module_name | lines_total | lines_executable | baseline_coverage_% | tier

server | 45230 | 38200 | 75.2 | A
sharding | 28450 | 22100 | 68.3 | A
storage | 31200 | 25000 | 70.1 | A
query | 22150 | 17800 | 72.4 | A
transaction | 18900 | 15200 | 73.1 | A
auth | 14200 | 11800 | 74.6 | A
index | 16800 | 13200 | 65.5 | B
rag | 12400 | 9800 | 64.5 | B
```

**How to Generate:**

```bash
# 1. Build with coverage enabled
cmake -B build -DTHEMIS_ENABLE_COVERAGE=ON -DTHEMIS_BUILD_TESTS=ON
cmake --build build

# 2. Run focused test set (Phase 3 Batch 1+)
ctest --test-dir build --label-regex "focused" -v

# 3. Collect coverage data
find build -name "*.gcda" -o -name "*.profraw" | xargs gcov/llvm-cov process

# 4. Generate JSON report per module
lcov --capture --directory build --output-file coverage.lcov
lcov --summary coverage.lcov

# 5. Parse to baseline format
python3 tools/generate_coverage_baseline.py coverage.lcov > .github/coverage-baseline-2026-10-11.txt
```

**Storage:**
- File: `.github/coverage-baseline-YYYY-MM-DD.txt`
- Retention: Keep last 4 baselines (monthly)
- Tracking: Git history shows delta over time

---

### C3: GitHub Gate Configuration Changes

**gate-pr-core.yml Updates (if implementing coverage gate):**

```yaml
# NEW STEP: Collect coverage after release_critical tests
- name: Generate coverage report (optional Phase 4)
  if: always() && steps.release_critical.outcome == 'success'
  run: |
    lcov --capture --directory build --output-file coverage.lcov
    lcov --summary coverage.lcov > coverage-summary.txt
    python3 scripts/validate_coverage_delta.py \
      --baseline .github/coverage-baseline.txt \
      --current coverage.lcov \
      --threshold 5.0 \
      --output coverage-gate-result.json

- name: Comment PR with coverage delta
  if: always() && github.event_name == 'pull_request'
  uses: actions/github-script@v7
  with:
    script: |
      const fs = require('fs');
      const result = JSON.parse(fs.readFileSync('coverage-gate-result.json', 'utf8'));
      github.rest.issues.createComment({
        issue_number: context.issue.number,
        owner: context.repo.owner,
        repo: context.repo.repo,
        body: `## Coverage Analysis\n\n${result.summary}\n\n${result.details}`
      });
```

**Status:** Phase 4 decision point — implement coverage gate or report-only?

---

### C4: Nightly Automation

**New Workflow: `coverage-report-nightly.yml`**

```yaml
name: Nightly Coverage Report

on:
  schedule:
    - cron: '0 2 * * *'  # 02:00 UTC nightly
  workflow_dispatch:

jobs:
  coverage-collection:
    runs-on: ubuntu-latest-heavy
    steps:
      - uses: actions/checkout@v5
      
      - name: Setup build environment
        run: |
          sudo apt-get update
          sudo apt-get install -y lcov python3 jq
      
      - name: Build with coverage enabled
        run: |
          cmake -B build \
            -DTHEMIS_ENABLE_COVERAGE=ON \
            -DTHEMIS_BUILD_TESTS=ON \
            -DCMAKE_BUILD_TYPE=Debug
          cmake --build build --parallel 8
      
      - name: Run all focused tests
        run: |
          ctest --test-dir build --label-regex "focused" -v --output-on-failure
      
      - name: Generate coverage report
        run: |
          lcov --capture --directory build --output-file coverage.lcov
          lcov --summary coverage.lcov > coverage-summary.txt
          python3 scripts/generate_coverage_report.py coverage.lcov
      
      - name: Upload coverage artifact
        uses: actions/upload-artifact@v4
        with:
          name: coverage-report-${{ github.run_id }}
          path: |
            coverage-report/
            coverage-summary.txt
          retention-days: 30
      
      - name: Update baseline if needed
        if: github.ref == 'refs/heads/develop'
        run: |
          python3 scripts/update_coverage_baseline.py coverage.lcov
          git config user.name "Copilot"
          git config user.email "copilot@themisdb.dev"
          git add .github/coverage-baseline*.txt
          git commit -m "coverage: Update baseline metrics (nightly run)" || true
          git push
```

**Schedule:**
- Nightly: 02:00 UTC (after all build-* workflows complete)
- Artifacts: Stored for 30 days
- Baseline: Updated on develop branch automatically
- Duration: ~45 minutes (build + coverage collection + reporting)

---

## Implementation Roadmap

| Step | Task | Duration | Depends On |
|---|---|---|---|
| **C1** | Add THEMIS_ENABLE_COVERAGE CMake option | 30 min | None |
| **C2a** | Create generate_coverage_baseline.py script | 2 hours | C1 |
| **C2b** | Generate initial baseline for Phase 3 | 1 hour | C2a |
| **C3a** | (Optional) Update gate-pr-core.yml for coverage | 1 hour | C2a |
| **C3b** | Create coverage-report-nightly.yml | 2 hours | C2a |
| **C4** | Lint + test nightly workflow locally | 30 min | C3b |

**Total: ~7 hours (including Phase 3 baseline generation)**

---

## Success Criteria for Block C

✅ CMake coverage flags available via `-DTHEMIS_ENABLE_COVERAGE=ON`
✅ Baseline metrics generated for all 71 modules
✅ Nightly workflow collects + reports coverage per module
✅ Coverage delta calculation working (phase 4 gate ready)
✅ No workflow linting errors
✅ Coverage reports published to artifacts
✅ README includes coverage badge (optional Phase 4)

---

## Known Issues & Assumptions

### Issue 1: .gcda/.profraw Aggregation
- Problem: Parallel CI jobs generate coverage data in separate job contexts
- Solution: Collect coverage per job, merge with lcov before reporting
- Impact: Nightly baseline collection on single machine (not parallel CI)

### Issue 2: Platform-Specific Coverage
- GCC/Clang: Full support (.gcda files)
- MSVC: Requires OpenCppCoverage post-instrumentation (separate tool)
- Decision: Phase 1 implementation Linux-only, MSVC coverage Phase 4

### Issue 3: Coverage Overhead on Build Performance
- Problem: --coverage flag adds 15-20% overhead
- Solution: Only enable in nightly builds, not PR gates (initially)
- Threshold Gate: Phase 4 decision point

### Assumption 1: lcov Tool Availability
- Assumed: lcov installed on Linux CI runners
- Fallback: Use apt-get install lcov in workflow setup

### Assumption 2: Baseline Stability
- Assumed: Baseline won't fluctuate >5% between runs (deterministic)
- Risk: Large refactors could require baseline reset

---

## Recommended Sequence for Implementation

1. **Day 1:** Add CMake coverage option (C1) + test locally
2. **Day 2:** Create coverage collection script (C2a) + generate baseline (C2b)
3. **Day 3:** Implement nightly workflow (C3b) + lint
4. **Day 4:** Integration testing (Phase 3 Batch 1 tests + coverage collection)
5. **Phase 4 (Future):** Add coverage gate to gate-pr-core.yml + badge

**Parallel Work:** Block D (Governance) can start after Day 1 (CMake option available)

