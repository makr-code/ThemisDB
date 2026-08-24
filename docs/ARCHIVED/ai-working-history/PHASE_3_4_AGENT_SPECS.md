# Index Module Gap Closure — Phase 3-4 Agent Specifications

**Document Type:** Agent task queue (for manual dispatch after Phase 2 completion)  
**Status:** Ready for dispatch (est. 2026-08-25 after Phase 2 completes)  
**Target Phases:** Phase 3 (HIGH), Phase 4 (MEDIUM)

---

## Phase 3: HIGH-Severity Remediation (themisdb-implementer — TBD Agent ID)

**Agent Type:** themisdb-implementer  
**Duration:** 3-4 weeks  
**Dependency:** Phase 2 completion (2026-08-25) + Phase 1 verification output  
**Key Deliverable:** ~1,800/3,057 HIGH gaps fixed + ThreadSanitizer validation

### Prompt Template

```
## Task: Index Module HIGH-Severity Gap Remediation Phase 3 (Batches A-5 to A-8+)

**Repository:** makr-code/ThemisDB  
**Target Module:** src/index  
**Input:** 
- Phase 1 output: ai_working/gap_index_phase1_verification.json
- Phase 2 completion (29 CRITICAL gaps fixed)
- Top HIGH-priority patterns: circular_lock_ordering (11), db_connection_leak (34), deadlock_risk (2+)

### Phase 3 Batches Overview

#### Batch A-5: circular_lock_ordering (11 gaps)
**Severity:** HIGH (deadlock risk)  
**Methodology:**
1. Identify lock acquisition order in conflicting code paths
2. Establish canonical lock order (e.g., tier 1: global, tier 2: shard, tier 3: local)
3. Annotate with clang thread-safety annotations if applicable
4. Add test case for lock order enforcement

**Success Criteria:**
- [x] 11/11 circular_lock_ordering gaps fixed
- [x] Canonical lock hierarchy documented (in comments or config)
- [x] ThreadSanitizer PASS (≥2 consecutive runs)

#### Batch A-6: db_connection_leak (34 gaps)
**Severity:** HIGH (resource leak)  
**Methodology:**
1. Audit all database connection allocation/deallocation pairs
2. Ensure connections released in RAII destructors or error paths
3. Add connection pooling or lifecycle management if missing
4. Add test case for connection cleanup under error conditions

**Success Criteria:**
- [x] 34/34 db_connection_leak gaps fixed
- [x] All connection allocation/deallocation pairs balanced
- [x] ASan/Valgrind PASS (connection lifecycle)

#### Batch A-7: deadlock_risk (2+ gaps) + generic HIGH patterns
**Severity:** HIGH  
**Methodology:**
1. For deadlock_risk: fix locking order, add timeout guards
2. For generic patterns (unchecked_array_index, pointer_arithmetic_unbounded, etc.):
   - Identify pattern occurrence (e.g., unchecked array access before bounds check)
   - Add defensive checks or RAII guards
   - Validate with ASan/UBSan

**Success Criteria:**
- [x] 2+/2+ deadlock_risk gaps fixed
- [x] 200-400 generic HIGH pattern gaps fixed (batched)
- [x] ASan/UBSan PASS

#### Batch A-8+: High-Pattern Bulk Fixes (target 1,600+ more gaps)
**Severity:** HIGH  
**Patterns (sorted by count):**
- generic_catch (26) → use typed exception handlers
- unchecked_result (17) → add null/error checks
- uninitialized_access (9) → initialize before use
- unchecked_cuda_call (26) → add CUDA error handling
- pointer_arithmetic_unbounded (7) → add bounds checks
- delete_without_nullptr (9) → set to nullptr after delete

**Methodology:**
- Group by pattern; implement fix template
- Apply across all occurrences in index module
- Batch-validate with focused tests

**Success Criteria:**
- [x] ≥60% of 3,057 HIGH gaps fixed (target 1,800–2,000)
- [x] All focused tests PASS
- [x] ThreadSanitizer PASS for concurrency-related fixes

### Parallel Validation (gap-verifier)
- After each batch, run focused test suite
- ThreadSanitizer validation: ≥2 consecutive PASS for A-5, A-6, A-7
- Build + ctest: index-specific lane

### Build & Test Commands
- Configure: cmake --preset develop-strict
- Build: cmake --build <build_dir> --target <index_tests>
- Test (TSan): ctest -L index --output-on-failure -j 1 -E "perf"
- Memory check: ctest -L index --output-on-failure -E "perf" --with-asan

### Output
- Commit message: "fix(index): Phase 3 Batch A-5..8+ HIGH gaps — locks, connections, deadlock, generic patterns"
- Track progress: ai_working/BATCH_A5_A6_A7_A8_COMPLETION_SUMMARY.md (similar to Phase 2)
- Update ROADMAP.md Phase 3 checklist

### Timeline
- A-5 (locks): 2-3 days
- A-6 (connections): 2-3 days
- A-7 (deadlock + generic): 3-4 days
- A-8+ (bulk HIGH fixes): 5-7 days
- **Total Phase 3:** 12-17 days (parallel batches: 3-4 weeks)

**Dependency:** Phase 2 complete (2026-08-25)  
**Estimated Start:** 2026-08-26  
**Estimated Completion:** 2026-09-15
```

---

## Phase 4: MEDIUM-Severity & Tooling Artifacts (task agents — TBD Agent IDs)

**Agent Type:** task agents (parallel with Phase 3)  
**Duration:** 2-3 weeks  
**Dependency:** Phase 1 verification output + Phase 2 completion (optional parallel)  
**Key Deliverable:** ~1,400/4,623 MEDIUM gaps fixed + validation

### Prompt Template

```
## Task: Index Module MEDIUM-Severity & Tooling Artifact Remediation Phase 4

**Repository:** makr-code/ThemisDB  
**Target Module:** src/index  
**Input:**
- Phase 1 output: ai_working/gap_index_phase1_verification.json
- MEDIUM gap summary: 4,623 total gaps (3 LOW, 4,620 MEDIUM)

### Phase 4 Focus Areas

#### 1. scope_mismatch Cluster (4,578 total — likely 60-70% false positives)
**Methodology:**
1. Use Phase 1 gap-verifier output to estimate false-positive rate
2. Sample-based validation: pick stratified sample (≥50 random cases)
3. For each sample, verify actual scope violation vs. tooling artifact
4. Separate TRUE_POSITIVE (real violation) from DEFERRED (style/low-impact)
5. Apply fixes to TRUE_POSITIVE subset (estimated 30-40% of 4,578)

**Expected Outcomes:**
- 1,200-1,800 scope_mismatch gaps fixed (assuming 30-40% TRUE_POSITIVE rate)
- 2,700-3,300 marked DEFERRED (style-only, low-impact)
- Sample validation report (≥50 cases analyzed)

**Success Criteria:**
- [x] ≥200 scope_mismatch gaps fixed (minimum batch)
- [x] Sample validation report PASS (≥95% classification accuracy)
- [x] Build + focused tests PASS

#### 2. Copy Overhead & String Concat Patterns (20 + 15 total)
**Patterns:**
- copy_overhead (20): use move semantics or std::string_view
- string_concat_loop (15): use std::stringstream or std::format

**Methodology:**
1. Identify hot paths with repeated copy or string concatenation
2. Apply pattern-based fix (e.g., move semantics, stringstream)
3. Validate performance (optional: benchmark gate)

**Success Criteria:**
- [x] 20/20 copy_overhead gaps fixed
- [x] 15/15 string_concat_loop gaps fixed
- [x] No performance regressions (benchmarks PASS if applicable)

#### 3. Braces_imbalance_midfile & Other Tooling Artifacts
**Patterns:**
- braces_imbalance_midfile (2,662): likely false positive (macro/template scope)
- other tooling artifacts (allocation_loop, etc.)

**Methodology:**
1. Validate sample (≥100 cases) for actual brace imbalance vs. artifact
2. If >90% false positive, mark DEFERRED with confidence note
3. For TRUE_POSITIVE, fix structural issues

**Success Criteria:**
- [x] braces_imbalance_midfile sample validation (≥100 cases)
- [x] Classification confidence documented
- [x] TRUE_POSITIVE subset (<10% expected) fixed

#### 4. Generic/Unchecked Patterns (miscellaneous MEDIUM)
**Patterns:**
- size_assumption (13): validate array size before use
- legacy_or_compat_path (25): mark for removal or consolidate
- range_temporary (10): fix temporary lifetime
- Other miscellaneous (50+): pattern-based batch fixes

**Methodology:**
- Pattern-based fix template
- Batch application across module
- Focused test validation

**Success Criteria:**
- [x] ≥400 MEDIUM pattern gaps fixed (50-100 per pattern)
- [x] Build + focused tests PASS
- [x] Benchmarks PASS (if applicable)

### Parallel Build/Test/Benchmark Validation
- Configure: cmake --preset develop-strict
- Build: cmake --build <build_dir> --target <index_tests>
- Test: ctest -L index --output-on-failure -j 1
- Benchmark: ctest -L "index.*bench" (if applicable)

### Output
- Commit message: "fix(index): Phase 4 MEDIUM gaps — scope_mismatch, copy overhead, string concat, artifacts"
- Track progress: ai_working/BATCH_MEDIUM_COMPLETION_SUMMARY.md
- Sample validation report: ai_working/MEDIUM_SAMPLE_VALIDATION.json
- Update ROADMAP.md Phase 4 checklist

### Timeline
- scope_mismatch sample validation + fixes: 3-5 days
- Copy overhead & string concat: 1-2 days
- Artifacts validation + fixes: 2-3 days
- Generic patterns: 3-4 days
- **Total Phase 4:** 9-14 days (parallel with Phase 3: 2-3 weeks)

**Dependency:** Phase 1 verification output (parallel start: 2026-08-20)  
**Estimated Start:** 2026-08-26 (after Phase 1 complete)  
**Estimated Completion:** 2026-09-15
```

---

## Dispatch Instructions

### When to Launch Phase 3
1. **Trigger Condition:** Phase 2 completion (2026-08-25 expected)
2. **Manual Dispatch:** Use task tool with agent_type: themisdb-implementer
3. **Copy prompt template** above, fill in actual Phase 1 output path
4. **Launch Mode:** background (runs in parallel with Phase 4 + Phase 5)

### When to Launch Phase 4
1. **Trigger Condition:** Phase 1 completion (2026-08-20 expected)
2. **Manual Dispatch:** Use task tool with agent_type: task (or multiple task agents for parallelism)
3. **Copy prompt template** above, fill in actual Phase 1 output path
4. **Launch Mode:** background (runs in parallel with Phase 3)

### Coordination Tips
- Phase 3 and Phase 4 can run in parallel (Phase 1 output is shared dependency)
- Monitor progress via ai_working/ batch completion summaries
- If Phase 1 reveals major classification changes, update prompt templates
- Use write_agent to send follow-up messages to running agents

---

**Last Updated:** 2026-08-15 10:53 UTC  
**Status:** Ready for dispatch (awaiting Phase 2 completion)
