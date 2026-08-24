# LLM Module Gap Closure — Validation & Test Checklist

**Date**: 2026-08-17  
**Target Completion**: 2026-08-31  
**Validation Owner**: LLM Module Team

---

## Baseline Model Evaluation Matrix (Release Candidate)

**Status**: Active baseline policy (updated 2026-08-23)

### A) Hard Gates (must pass, otherwise reject)

- [ ] Safety/policy gates from `PRODUCTION_REQUIREMENTS.md` pass (prompt policy, validator, classifier/guardian, quota, monitoring).
- [ ] LLM integration tests pass (`ctest -L llm --output-on-failure`).
- [ ] No sanitizer regressions in validated profile (ASan/TSan/UBSan runbook scope).
- [ ] Model artifact is runtime-compatible GGUF for release packaging.

### B) Weighted Scoring (0-100)

Use this score only after all hard gates pass.

| Criterion | Weight | Measurement method | Pass threshold |
|---|---:|---|---|
| Quality on Themis RAG tasks | 30 | `tests/llm/test_wiki_rag_quality.cpp` + curated prompt set | Recall@5 >= 80% and no critical safety failures |
| Runtime latency (TTFT + wall time) | 20 | Local Ollama API benchmark with fixed prompt batch | p95 wall-time <= baseline + 20% |
| Throughput (tokens/s) | 15 | `eval_count / eval_duration` from Ollama generate API | >= baseline - 20% |
| Resource efficiency (RAM/VRAM/disk) | 15 | model file size + runtime memory stats | Must fit deployment budget |
| Context window adequacy | 10 | `ollama show <model>` context length | >= 32k for long-context tier |
| Tooling capability fit | 10 | `ollama show <model>` capabilities | tools support required for tool paths |

**Decision bands**:

- **>= 85**: Primary baseline candidate
- **70-84**: Secondary/specialized candidate
- **< 70**: Not suitable as default baseline

### C) Current Local Comparison Snapshot (2026-08-23)

Source: local Ollama (`http://127.0.0.1:11434`).

| Model | Size | Context | Quantization | Observed wall time (single fixed prompt) | Observed tokens/s | Capability notes |
|---|---:|---:|---|---:|---:|---|
| `gemma4:latest` | 9.6 GB | 131072 | Q4_K_M | 104878 ms | 70.49 | completion, tools, thinking, vision, audio |
| `hf.co/yuxinlu1/gemma-4-12B-coder-fable5-composer2.5-v1-GGUF:Q8_0` | 12 GB | 131072 | Q8_0 | 171128 ms | 4.54 | completion, tools |

### D) Baseline Recommendation

- [x] **Local test standard**: `gemma4:latest`
- [ ] Optional quality-tier model: 12B Q8 variant for offline or quality-priority workloads only
- [x] **Distribution standard**: `tinyllama:latest` (`tinyllama_latest.gguf` in `models/`)

### D.1) Licensing and Distribution Constraint (mandatory)

- [x] Gemma-family models are permitted for local development and validation runs.
- [x] Gemma-family models are **not** part of distributable runtime payloads.
- [x] Public/community release artifacts must include TinyLlama GGUF payload as the default shipped model.
- [x] CI/release packaging checks must fail if distributable bundles include non-approved model families.

### E) Reproducible Local Benchmark Command

```powershell
$models=@('gemma4:latest','hf.co/yuxinlu1/gemma-4-12B-coder-fable5-composer2.5-v1-GGUF:Q8_0')
$prompt='Gib in genau einem Satz den Zweck von ThemisDB als Multi-Model-Datenbank wieder.'
foreach($m in $models){
  $body=@{model=$m;prompt=$prompt;stream=$false}|ConvertTo-Json -Compress
  $sw=[System.Diagnostics.Stopwatch]::StartNew()
  $resp=curl.exe -sS -X POST http://127.0.0.1:11434/api/generate -H "Content-Type: application/json" -d $body
  $sw.Stop()
  $json=$resp|ConvertFrom-Json
  $tps=0
  if($json.eval_duration -gt 0){$tps=[math]::Round(($json.eval_count/($json.eval_duration/1e9)),2)}
  Write-Host "MODEL=$m WALL_MS=$([int]$sw.Elapsed.TotalMilliseconds) TOKENS_PER_SEC=$tps"
}
```

---

## Phase 1: Structural Fixes Validation

### 1.1 Braces Imbalance Fixes ✓

**Affected Files** (37 total):
- [ ] active_vram_allocator.cpp
- [ ] adapter_registry.cpp
- [ ] async_inference_engine.cpp
- [ ] block_table.cpp
- [ ] ethics_aware_confidence_detector.cpp
- [ ] gguf_loader.cpp
- [ ] grafana_metrics.cpp
- [ ] inference_engine_enhanced.cpp
- [ ] llama_wrapper.cpp
- [ ] llm_model_storage.cpp
- [ ] llm_prefix_cache.cpp
- [ ] meta_prompt_generator.cpp
- [ ] model_downloader.cpp
- [ ] model_loader.cpp
- [ ] multi_lora_manager.cpp
- [ ] multi_perspective_generator.cpp
- [ ] prompt_evaluator.cpp
- [ ] prompt_optimizer.cpp
- [ ] streaming_handler.cpp
- [ ] token_quota_manager.cpp
- [ ] (17 additional files)

**Validation Steps**:
1. [ ] All files compile with `clang++ -std=c++17 -Wall -Wextra`
2. [ ] No new compiler warnings introduced
3. [ ] clang-format exits 0 on all files: `clang-format -i file.cpp && echo OK`
4. [ ] Syntax check passes: `clang -fsyntax-only file.cpp`
5. [ ] All existing tests still pass: `ctest --label "llm" --output-on-failure`

**Success Criteria**:
- ✅ 0 brace-imbalance related errors in compilation
- ✅ 0 new compiler warnings
- ✅ All 37 files parse correctly
- ✅ No functional code changes (only brace fixes)

---

### 1.2 Thread-Safety & Data-Race Fixes ✓

**Affected Categories**:
- [ ] Data race (11 instances)
- [ ] Circular lock ordering (108 instances)
- [ ] Lock contention (15 instances)
- [ ] Deadlock risk (11 instances)
- [ ] Primitive no volatile (22 instances)
- [ ] Missing sync threads (2 instances)

**Validation Steps**:

#### Build & Compilation
```bash
[ ] cmake --preset windows-release -DCMAKE_CXX_FLAGS="-fsanitize=thread -g"
[ ] cmake --build --preset windows-release --parallel 16
```

#### ThreadSanitizer Testing
```bash
[ ] clang -fsanitize=thread -g tests/llm/*.cpp -o test_llm_tsan
[ ] ./test_llm_tsan --gtest_filter="*ThreadSafety*"
[ ] No TSan errors in output (0 race detections)
```

#### Lock Order Verification
```bash
[ ] Review lock hierarchy documentation
[ ] Verify all mutex acquisitions follow documented order
[ ] No new deadlock warnings from static analysis
```

**Success Criteria**:
- ✅ ThreadSanitizer clean: 0 data races detected
- ✅ No new deadlock warnings
- ✅ All existing concurrency tests pass
- ✅ Thread-safety contracts documented for all guarded state

---

### 1.3 RAII & Resource Management Fixes ✓

**Affected Categories**:
- [ ] Resource leaked in exception (108 instances)
- [ ] DB connection leak (192 instances)
- [ ] GPU memory leak (10 instances)
- [ ] Manual cleanup (44 instances)
- [ ] Delete without nullptr (12 instances)
- [ ] Null dereference (59 instances)
- [ ] Memory order (7 instances)

**Validation Steps**:

#### AddressSanitizer Testing
```bash
[ ] cmake --preset windows-release -DCMAKE_CXX_FLAGS="-fsanitize=address -g"
[ ] cmake --build --preset windows-release --parallel 16
[ ] ctest --preset windows-release -L llm -V
```

**ASan Output Validation**:
```
[ ] SUMMARY: AddressSanitizer: 0 bytes leaked in X allocations
[ ] No "use-after-free" errors
[ ] No "heap-buffer-overflow" errors
[ ] No "global-buffer-overflow" errors
[ ] No "double-free" errors
```

#### Exception-Safety Testing
```cpp
// Verify RAII cleanup on exception
[ ] Model load with exception throws → no leak
[ ] Adapter load with exception throws → no leak
[ ] GPU allocation with exception throws → no GPU leak
[ ] DB connection acquisition with exception throws → connection closed
```

**Success Criteria**:
- ✅ AddressSanitizer: 0 bytes leaked on all tests
- ✅ No manual delete statements in critical paths
- ✅ All resources guarded by unique_ptr/shared_ptr/RAII wrapper
- ✅ Exception-safety tests pass (strong/basic/nothrow guarantees)

---

## Phase 2: Documentation Validation

### 2.1 Module-Level Documentation ✓

**Files to Validate**:
- [ ] ARCHITECTURE.md — System design, concurrency, resource mgmt
- [ ] PRODUCTION_REQUIREMENTS.md — SLOs, constraints, acceptance
- [ ] SECURITY.md — Threat model, audit, compliance
- [ ] OPERATIONS.md — Runbooks, debugging, recovery procedures

**Validation Checklist**:
- [ ] ARCHITECTURE.md covers:
  - [ ] Thread-safety model (which fields guarded by which locks)
  - [ ] Resource ownership (who owns what, cleanup triggers)
  - [ ] Error handling & fail-closed behavior
  - [ ] Integration boundaries with other modules
- [ ] PRODUCTION_REQUIREMENTS.md covers:
  - [ ] SLA/SLO targets (latency p50/p95/p99, throughput)
  - [ ] Resource constraints (VRAM, CPU, memory)
  - [ ] Operational expectations (log volume, metrics, alerts)
- [ ] SECURITY.md covers:
  - [ ] Threat model and risk assessment
  - [ ] Input validation and sanitization rules
  - [ ] Audit and compliance requirements
- [ ] OPERATIONS.md covers:
  - [ ] Model loading/unloading runbooks
  - [ ] Common errors and recovery steps
  - [ ] Performance tuning procedures
  - [ ] Debugging checklist

**Success Criteria**:
- ✅ All documentation files present and linked
- ✅ No stale references or broken links
- ✅ Doxygen builds cleanly: `doxygen Doxyfile.audit`
- ✅ All narrative content is current (date: 2026-08-XX)

### 2.2 Inline Code Documentation ✓

**Validation Coverage**:
- [ ] All 90+ .cpp files have @file headers
- [ ] All public functions documented with Doxygen tags
- [ ] All public classes documented
- [ ] Thread-safety documented for all mutable state
- [ ] Exception-safety levels documented

**Template Validation**:
Each public function must have:
```cpp
/// @brief <one-line purpose>
/// @param <name> <description>
/// @return <description>
/// @throws <exception> <when>
/// @thread_safety <THREAD_SAFE|CALLER_MUST_SYNCHRONIZE>
/// @exception_safety <STRONG|BASIC|NOTHROW>
```

**Validation Steps**:
```bash
[ ] clang-doc -p . -output=../docs/generated/ src/llm/
[ ] Check that all public symbols are documented
[ ] doxygen produces no warnings about undocumented public members
[ ] Doxygen HTML renders correctly
```

**Success Criteria**:
- ✅ All public APIs have Doxygen documentation
- ✅ Doxygen builds with 0 warnings for undocumented symbols
- ✅ Thread-safety documented for every class with mutable state
- ✅ Exception-safety level documented for every throwing function

---

## Phase 3: Code Quality & Performance

### 3.1 Performance Regression Tests ✓

**Benchmark Gates** (GATE-LLM-01..08):
- [ ] Inference latency (p50, p95, p99) within baseline ±10%
- [ ] Model load time within baseline ±15%
- [ ] Adapter switch overhead < 100ms
- [ ] Memory pressure under sustained load acceptable
- [ ] Token throughput stable

**Validation**:
```bash
[ ] cmake --preset windows-release
[ ] cmake --build --preset windows-release -j16
[ ] ctest --preset windows-release -L benchmark -V
[ ] Compare results to baseline (GATE-LLM-01..08)
[ ] All gates pass (no regression > threshold)
```

### 3.2 Code Quality Checks ✓

**Static Analysis**:
```bash
[ ] clang-tidy -p . src/llm/*.cpp -- -checks="-*,readability-*,performance-*"
[ ] 0 critical findings in clang-tidy output
```

**Dynamic Analysis**:
```bash
[ ] Build with -fsanitize=undefined -fno-sanitize-recover=all
[ ] Run all tests; no undefined behavior detected
[ ] AddressSanitizer clean (see Phase 1.3)
[ ] ThreadSanitizer clean (see Phase 1.2)
```

**Code Coverage**:
```bash
[ ] Run tests with coverage: -fprofile-instr-generate -fcoverage-mapping
[ ] lcov --capture --directory . --output-file coverage.info
[ ] Verify critical paths have >= 80% line coverage
```

---

## Phase 4: Testing & Integration

### 4.1 Focused Test Expansion ✓

**New Test Categories**:
- [ ] Thread-safety under concurrent load (data-race patterns)
- [ ] Exception-safety in model load/unload
- [ ] Resource cleanup on exception paths
- [ ] VRAM allocation/deallocation under pressure
- [ ] Adapter hot-swap races

**Test Requirements**:
- [ ] Use `themis_register_module_focused_test()`
- [ ] Label: `release_critical`, `llm`, `phase_N`
- [ ] Timeout: 120s per test
- [ ] Deterministic (fixed seed, no timing dependencies)

**Validation**:
```bash
[ ] ctest --preset windows-release -L "release_critical;llm" -V
[ ] All focused tests pass
[ ] No timeout violations
[ ] No flaky tests (run 5x; all pass)
```

### 4.2 Regression Test Suite ✓

**Existing Tests**:
- [ ] `test_llm_api_contract_hardening_focused.cpp` (LAC-01..20)
- [ ] `test_llm_phase5_hardening.cpp` (EXS-01..28, MEM-01..24)
- [ ] `test_llm_memory_safety_hardening.cpp` (MEM-01..16, EXS-01..25)
- [ ] Module core tests (40+ existing tests)

**Validation**:
```bash
[ ] ctest --preset windows-release -R "llm" -V --output-on-failure
[ ] All tests pass (green checkmarks)
[ ] No new test failures introduced
[ ] Test execution time stable (< 5% variance)
```

---

## Final Validation & Sign-Off

### Pre-Merge Checklist

**Code Changes**:
- [ ] All IMPL gaps addressed (1,400 issues)
- [ ] All DOC gaps addressed (11,074 issues)
- [ ] 0 CRITICAL compiler errors
- [ ] 0 new compiler warnings (treat warnings as errors: -Werror)
- [ ] No functional behavior changes in unrelated code

**Testing**:
- [ ] All unit tests pass (120+ tests)
- [ ] All integration tests pass
- [ ] Performance benchmarks within baseline
- [ ] ThreadSanitizer: 0 data races
- [ ] AddressSanitizer: 0 memory leaks
- [ ] Undefined Behavior Sanitizer: 0 UB detections

**Documentation**:
- [ ] ROADMAP.md updated with gap closure status
- [ ] CHANGELOG.md updated with changes
- [ ] All .cpp files have Doxygen headers
- [ ] README.md is current
- [ ] No stale documentation references

**Code Review**:
- [ ] Self-review completed (no obvious issues)
- [ ] Peer review scheduled
- [ ] All feedback addressed
- [ ] PR description captures improvements

### Sign-Off Criteria

**Must Pass Before Merge**:
- ✅ Build: `cmake --build --preset windows-release` succeeds
- ✅ Tests: `ctest --preset windows-release` all pass
- ✅ Lint: No new critical warnings
- ✅ Sanitizers: ASan/TSan/UBSan all clean
- ✅ Benchmarks: GATE-LLM-01..08 all pass
- ✅ Doxygen: Builds with 0 undocumented public symbols
- ✅ Documentation: README, ARCHITECTURE, OPERATIONS all current

---

## Rollback Plan (If Needed)

If validation fails at any stage:

1. **Identify Issue**: Determine which gap category failed
2. **Isolate Fix**: Revert specific sub-agent changes if needed
3. **Root Cause**: Analyze why validation failed
4. **Re-Fix**: Apply targeted remediation
5. **Re-Test**: Run validation suite again

**Example**:
```bash
# If thread-safety tests fail
git revert <commit-hash>  # Revert thread-safety fixes
# Fix root cause
git add . && git commit -m "LLM: Fix thread-safety validation failure"
# Re-run TSan tests
ctest -L thread_safety -V
```

---

## Sign-Off & Closure

**Milestone Completion**: [ ] 2026-08-31 (or actual date)

**Final Sign-Off**:
- LLM Module Lead: ___________________ Date: ________
- QA Engineer: ______________________ Date: ________
- Integration Owner: ________________ Date: ________

**Closure Statement**:
> All 12,474 gaps in the LLM module have been systematically analyzed, remediated, and validated. Implementation fixes address all 1,400 code quality issues; documentation enhancements address all 11,074 documentation gaps. Module is production-ready with full thread-safety, exception-safety, and resource-management guarantees.

---

**Document Version**: 1.0  
**Last Updated**: 2026-08-17  
**Next Review**: After sub-agent completion
