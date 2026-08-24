# Replication Module Gap Closure — Batch 4 Master Plan

**Scope**: Close 1519 identified gaps (16 CRITICAL + 194 HIGH + 1307 MEDIUM + 2 LOW) in replication module.  
**Strategy**: 3-agent parallel batching model (proven from Process module: 5x speedup).  
**Target**: Production-ready closure by 2026-08-16.

---

## Executive Summary

- **Total Gaps**: 1519 (16 CRITICAL + 194 HIGH + 1307 MEDIUM + 2 LOW)
- **Unimplemented Findings (Critical)**: 22 (logical_replication, replication_manager)
- **Largest Categories**:
  - scope_mismatch: 1262
  - circular_lock_ordering: 96
  - range_temporary: 21
  - todo_as_productionlogic: 20
  - missing_volatile: 14

- **Affected Files** (top 4):
  1. replication_manager.cpp (517 findings)
  2. observability.cpp (highest CRITICAL count)
  3. logical_replication.cpp (multiple unimplemented)
  4. replication_slot.cpp (circular_lock_ordering)

---

## Batching Strategy (3 Agents)

Parallel execution with file-level isolation (no merge conflicts):

### Agent 1 — CRITICAL Batch
**Scope**: Fix all 16 CRITICAL findings + 22 unimplemented patterns  
**Files**: 
- logical_replication.cpp (lines 499, 713, 725 + others)
- replication_manager.cpp (lines 2610, 2612, 2735, 3953, 4008 + others)
- observability.cpp (scope_mismatch, braces_imbalance)
- policy.cpp (braces_imbalance)

**Pattern Focus**:
- Unimplemented return statements (empty {} → proper logic)
- Scope_mismatch violations (variable lifetime issues)
- Braces imbalance fixes
- Overflow safety (multiplication_overflow at line 549)
- No-timeout fixes (critical async operations)
- Iterator invalidation guards (lines 2769, 4052)

**Expected Output**:
- All CRITICAL findings resolved
- 16 CRITICAL + core HIGH-severity unimplemented patterns fixed
- No breaking changes to public API
- Comprehensive test coverage for each fix

**Acceptance Criteria**:
- [ ] All 16 CRITICAL findings closed
- [ ] All 22 unimplemented patterns implemented
- [ ] Build passes (no compiler errors)
- [ ] Existing tests pass
- [ ] Security review for overflow/null safety

---

### Agent 2 — HIGH-A Batch
**Scope**: Fix HIGH-severity findings set A (file isolation)  
**Files**:
- replication_slot.cpp (circular_lock_ordering: 96 findings)
- raft_v2.cpp (distributed consistency patterns)
- event_stream.cpp (range_temporary, string_concat_loop)

**Pattern Focus**:
- Circular lock ordering (prevent deadlock)
- Missing noexcept on move operations
- Iterator validation
- Memory/resource safety (manual_cleanup → RAII)
- Range temporary lifetime issues

**Expected Output**:
- ~80-100 HIGH findings from replication_slot resolved
- Lock ordering verified
- RAII-safe resource management

**Acceptance Criteria**:
- [ ] replication_slot circular_lock_ordering resolved
- [ ] raft_v2 HIGH findings addressed
- [ ] event_stream performance patterns fixed
- [ ] Build + existing tests pass

---

### Agent 3 — HIGH-B + MEDIUM Batch
**Scope**: Remaining HIGH + bulk MEDIUM patterns  
**Files**:
- async_wal_shipper.cpp (HIGH + MEDIUM patterns)
- multi_tier_replication.cpp (scope_mismatch + distributed patterns)
- conflict_resolution.cpp (lock_contention, range_temporary)

**Pattern Focus**:
- scope_mismatch bulk closure (1100+ findings)
- lock_contention & no_timeout hardening
- Copy overhead & performance patterns
- Todo-as-productionlogic conversion
- Null dereference guards

**Expected Output**:
- ~1100+ MEDIUM scope_mismatch findings addressed
- HIGH-B findings closed
- Performance + safety hardened

**Acceptance Criteria**:
- [ ] scope_mismatch bulk patterns addressed
- [ ] MEDIUM todo_as_productionlogic converted
- [ ] Build + existing tests pass
- [ ] Benchmarks stable

---

## Sequential Merge Strategy

After parallel execution:
1. **Merge A1 → A2**: CRITICAL + HIGH-A consolidation
   - Verify no file overlap conflicts
   - Update replication module's ROADMAP.md with closure evidence
   - Check A1 + A2 build together

2. **Merge (A1+A2) → A3**: Add HIGH-B + MEDIUM
   - Final integration test
   - Run replication test suite
   - CodeQL/security verification

3. **Final PR**: Single comprehensive closure PR
   - Commit message: "Replication module gap closure Batch 4: close 1519 gaps (CRITICAL+HIGH+MEDIUM)"
   - Evidence: Agent execution logs + test results
   - Target: `develop` branch

---

## Implementation Phases (Per Agent)

Each agent follows:
1. **Analysis** (5 min)
   - Identify exact gap locations
   - Understand fix context (API contracts, callers)
   - Classify fix complexity

2. **Implementation** (30-45 min)
   - Fix production logic (not stubs)
   - Add proper error handling
   - Ensure const-correctness, RAII

3. **Testing** (10 min)
   - Verify existing tests still pass
   - Add focused regression tests if needed
   - Validate fix semantics

4. **Documentation** (5 min)
   - Update code comments if semantics changed
   - Log fix in working notes

---

## Build & Test Verification

After each agent completes:
```bash
# Agent 1 (CRITICAL)
cmake --preset windows-release
cmake --build --preset windows-release --parallel 16
ctest --preset windows-release -k "replication" --output-on-failure

# After Agent 2 merge
ctest --preset windows-release -k "replication" --output-on-failure

# After Agent 3 merge
ctest --preset windows-release --output-on-failure  # full test
```

---

## Known Constraints

1. **No breaking changes** to replication_api_contract.h or public surfaces
2. **Maintain deterministic behavior** in failover/promotion paths
3. **Thread-safe** changes only (no removal of existing synchronization)
4. **Performance**: No intentional regression (verify via existing benchmarks)
5. **Exception safety**: RAII-only resource management

---

## Evidence Tracking

All findings and fixes logged in:
- `ai_working/REPLICATION_GAPS_BATCH4_A1_COMPLETION_REPORT.md` (Agent 1)
- `ai_working/REPLICATION_GAPS_BATCH4_A2_COMPLETION_REPORT.md` (Agent 2)
- `ai_working/REPLICATION_GAPS_BATCH4_A3_COMPLETION_REPORT.md` (Agent 3)
- Final merged into: `/home/runner/work/ThemisDB/ThemisDB/src/replication/MODULE_GAPS_BATCH4_CLOSURE_EVIDENCE.md`

---

## Success Criteria

✓ All 1519 gaps addressed (resolved or false-positive documented)  
✓ All 16 CRITICAL findings fixed  
✓ All 194 HIGH findings fixed or documented as design-safe  
✓ Build + tests pass on windows-release  
✓ No breaking changes to public API  
✓ Merge conflict-free sequential consolidation  
✓ Single comprehensive closure PR on develop  

---

## Timeline Estimate

- Agent 1 (CRITICAL): ~25 min
- Agent 2 (HIGH-A): ~30 min
- Agent 3 (HIGH-B + MEDIUM): ~35 min
- Sequential merge + verification: ~10 min
- **Total**: ~100 min (~1.7 hours)

(Based on Process module parallel model: 3-5 agents × 25-40 min each = 75-200 min sequential)

---

Generated: 2026-08-16 08:48 UTC  
Status: Ready for parallel agent dispatch
