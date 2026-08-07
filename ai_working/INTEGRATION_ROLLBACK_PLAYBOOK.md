# Integration Rollback Playbook: Tensor Phase 2-3 Hardening

**Status**: PREPARATION (Aug 21-26, Phase 3)  
**Purpose**: Step-by-step rollback procedure if Phase 2-3 integration discovers critical issues  
**Owner**: Release Management Team

---

## Rollback Trigger Conditions

### CRITICAL Triggers (Immediate Rollback)
- **RC-1**: Production data corruption detected (fingerprint hash mismatches)
- **RC-2**: Loss of service (>95% query failure rate post-merge)
- **RC-3**: Silent data loss (fingerprints not persisted)
- **RC-4**: Uncontrollable resource exhaustion (memory, CPU spike >50%)
- **RC-5**: Security breach (unauthorized access to multi-tenant data)

### HIGH Triggers (Pause & Investigate, Conditional Rollback)
- **RH-1**: Intermittent failures (flaky tests, <80% pass rate)
- **RH-2**: Performance regression (p99 latency +30% vs baseline)
- **RH-3**: New CRITICAL findings in post-merge validation
- **RH-4**: ThreadSanitizer races detected in production-like workload
- **RH-5**: Code review concerns (unaddressed architectural risk)

---

## Rollback Levels

### Level 1: Code Rollback Only (Fastest)
**Time**: ~5 minutes  
**Loss**: None (develop branch unchanged)  
**Use Case**: Integration test failures discovered before merge

#### Steps:
```bash
# 1. Abort current PR merge
git reset --hard develop

# 2. Verify reset
git log --oneline -n 3
# Should show current develop tip

# 3. Notify team
# Post: "Rollback Level 1 initiated due to [RC/RH trigger]"
# Create incident issue (rollback reason, failure evidence)

# 4. Root cause analysis
# - Review integration test logs
# - Identify which finding fix caused failure
# - Create fix issue for next iteration
```

**Recovery Path**:
- Phase 2 agent investigates failing integration test
- Isolate the problematic finding fix (e.g., CRITICAL-1 or CRITICAL-2)
- Apply targeted fix without full Phase 2 re-do
- Merge fixed version once integration tests PASS

---

### Level 2: Partial Rollback (Post-Merge, Single Fix Rollout)
**Time**: ~15 minutes  
**Loss**: One fix/feature partially reverted  
**Use Case**: One of the 11 A2 findings causes issues post-merge

#### Steps:
```bash
# 1. Identify problematic fix
# Example: CRITICAL-1 diagnostic emission causes performance cliff
git log --oneline --grep="CRITICAL-1" | head -1
# Returns: abc1234 fix(tensor): CRITICAL-1 silent failure diagnostic emission

# 2. Create rollback branch
git checkout -b rollback/partial-critical1-revert
git revert abc1234 --no-edit

# 3. Verify revert removes exactly CRITICAL-1 fix
git diff develop..HEAD
# Should show revert of:
#   - emitDiagnostic() calls at lines 267, 282, 293, 311, 320, 358
#   - P2-A2-01 test removal
#   - Error code TENSOR-9501 if unused elsewhere

# 4. Run focused tests
cmake --preset linux-release
ctest -L "tensor" -V --timeout 300

# 5. If tests PASS, promote to main
git checkout develop
git merge rollback/partial-critical1-revert

# 6. Notify team
# Post: "Partial rollback: CRITICAL-1 reverted due to [performance/crash]. 
#        CRITICAL-2 + HIGH-3/4 + MEDIUM/LOW fixes remain active."
```

**Recovery Path**:
- Phase 2 agent re-investigates CRITICAL-1 implementation
- Alternative approach: less aggressive diagnostic emission, sampling, or async logging
- Re-test, then cherry-pick into develop

---

### Level 3: Full Phase 2 Rollback (Pre-Merge Safety Net)
**Time**: ~30 minutes  
**Loss**: All A2 fixes reverted, A1/A3 retained  
**Use Case**: Multiple integration failures or architectural incompatibility

#### Steps:
```bash
# 1. Check current branch
git branch
# Should be: feature/tensor-q3-hardening (not merged yet)

# 2. Reset to before Phase 2 work
# Phase 2 = commits with all 11 fixes
# Identify first Phase 2 commit
git log --oneline | grep -i "phase 2\|A2.*remediation" | tail -1
# Returns: def5678 feat(tensor): Phase 2 A2 remediation batch

# 3. Revert all Phase 2 commits
for commit in $(git log --oneline develop..HEAD | grep -i "A2\|finding" | awk '{print $1}'); do
  git revert $commit --no-edit
done

# 4. Keep A1 concurrent hardening + A3 benchmarks
# Verify only A2 changes are reverted
git diff develop..HEAD | grep "P2-A2\|CRITICAL\|emitDiagnostic" || echo "A2 cleanly reverted"

# 5. Run full test suite
cmake --preset linux-release
ctest -L "tensor" -V

# 6. If tests PASS, push modified branch
git push origin rollback/full-phase2-revert

# 7. Create new PR
gh pr create \
  --title "rollback: Phase 2 A2 remediation (full)" \
  --body "Full Phase 2 rollback due to [RH trigger]. A1/A3 retained. See incident #XXXX."
```

**Recovery Path**:
- Pause Phase 2-3-4, extend timeline
- Root cause analysis: which 11 findings are truly problematic?
- Redesign Phase 2 with subset of findings + additional mitigations
- Re-implement in next iteration (Sept+ planning)

---

### Level 4: Full Stream A Rollback (Panic Button)
**Time**: ~1 hour  
**Loss**: A1/A2/A3 all reverted, back to Aug 6 baseline  
**Use Case**: Architectural incompatibility, major security issue, data corruption

#### Steps:
```bash
# 1. Hard reset to Aug 6 baseline
git log --oneline --since="2026-08-06" --until="2026-08-07" | tail -1
# Returns: xyz9999 commit(Aug 6 EOD)

git reset --hard xyz9999

# 2. Verify we're at safe baseline
git diff --stat develop | wc -l
# Should show 0 lines (identical to develop)

# 3. Create incident branch
git checkout -b incident/stream-a-revert-aug6
git push origin incident/stream-a-revert-aug6

# 4. Create major incident issue
gh issue create \
  --title "INCIDENT: Full Stream A rollback triggered (Aug 7)" \
  --body "All tensor Phase 1-3 work reverted due to [CRITICAL trigger]. See rollback playbook."
  --labels "severity/critical,rollback,incident"

# 5. War room + post-mortem
# - Assess why all 3 blocks failed integration
# - Re-architect tensor approach
# - Plan Stream A v2 (Sept)
```

**Recovery Path**:
- Multi-day post-mortem and redesign
- Escalate to leadership if architectural
- Plan Stream A Revision (Sept 1+)
- Consider splitting tensor work across Q4

---

## Decision Tree: Which Level to Activate?

```
Failure Detected
  │
  ├─ Pre-Merge (PR still open)?
  │  │
  │  ├─ One specific test failing?
  │  │  └─→ LEVEL 1: Code Rollback Only
  │  │
  │  ├─ Multiple tests failing, isolated module?
  │  │  └─→ LEVEL 1 OR 2: Revert single finding
  │  │
  │  └─ Systematic failures across many tests?
  │     └─→ LEVEL 3: Full Phase 2 Rollback
  │
  └─ Post-Merge (on develop)?
     │
     ├─ Single fix causing issue?
     │  └─→ LEVEL 2: Partial Rollback + Fix
     │
     ├─ Multiple fixes causing cascading failures?
     │  └─→ LEVEL 3: Full Phase 2 Rollback
     │
     └─ Data corruption OR security breach?
        └─→ LEVEL 4: Full Stream A Rollback (+ incident)
```

---

## Rollback Validation Checklist

After activating any rollback level:

### Immediate (5-15 min)
- [ ] Rollback branch/commit created and pushed
- [ ] Build succeeds (all presets)
- [ ] Unit tests PASS (72+ tensor tests)
- [ ] Integration tests stable (or marked flaky if expected)
- [ ] Notify #incident-response channel

### Short-term (1-2 hours)
- [ ] All gates re-verified (GATE-P2-01..10 if post-merge)
- [ ] Performance metrics compared vs baseline
- [ ] ThreadSanitizer clean
- [ ] Wave 6 retention tests PASS
- [ ] No new regressions vs Aug 6 baseline

### Root Cause (2-4 hours)
- [ ] Identify which finding(s) caused failure
- [ ] Reproduce failure in isolated test
- [ ] Design alternative implementation
- [ ] Update ROADMAP.md with lesson learned
- [ ] Create follow-up issue for next iteration

### Communication (ongoing)
- [ ] Incident issue created (if CRITICAL trigger)
- [ ] Stakeholder notification (leadership, community)
- [ ] Post-mortem scheduled (if Level 3+)
- [ ] Public update in #announcements (if Level 4)

---

## Rollback Scenarios & Responses

### Scenario 1: CRITICAL-1 Silent Failure Fix Breaks Query
**Trigger**: RH-2 (Performance regression p99 +50%)  
**Root Cause**: emitDiagnostic() overhead accumulates under concurrent load  
**Response**:
1. Level 2: Revert CRITICAL-1 only
2. Investigation: Profile emitDiagnostic() call overhead
3. Fix: Batch diagnostics, sample 10% vs 100%
4. Re-test: p99 latency < 20% regression
5. Re-merge: CRITICAL-1-v2 with optimization

### Scenario 2: CRITICAL-2 Error Code Collisions
**Trigger**: RC-3 (Different module also uses TENSOR-9500 range)  
**Root Cause**: Error code range not reserved globally  
**Response**:
1. Level 3: Full Phase 2 revert
2. Investigation: Audit error_registry.h for conflicts
3. Fix: Claim TENSOR-9600..9699 range instead
4. Investigation: Expand all 11 findings to use new range
5. Re-merge: Phase 2-v2 with revised error codes

### Scenario 3: Sharding Integration Cascades
**Trigger**: RH-1 (TINT-SH tests flaky, 60% pass rate)  
**Root Cause**: Distributed fingerprint graph replication not atomic  
**Response**:
1. Level 1: Pause merge (stay on feature branch)
2. Investigation: Review TINT-SH-01/02 failure logs
3. Fix: Add replication transaction boundary
4. Re-test: TINT-SH-01..04 stable (100% pass, 10x runs)
5. Re-merge: Phase 2+3 together (skip to Level 3 of Phase 3)

### Scenario 4: Data Corruption Detected
**Trigger**: RC-3 (Fingerprints not persisted, silent loss)  
**Root Cause**: core_bridge::write() doesn't flush fingerprints to RocksDB  
**Response**:
1. Level 4: Full Stream A Rollback (emergency)
2. Data Recovery: Restore from backup
3. Investigation: 4-hour post-mortem
4. Fix: Rewrite persistence layer with strict ordering
5. Re-plan: Stream A v2 (multi-week redesign)

---

## Post-Rollback Communication

### Rollback Notification Template

```markdown
**ROLLBACK ACTIVATED: [Level X] - [Trigger]**

**Initiated**: [Time]  
**Branch**: feature/tensor-q3-hardening  
**Status**: [Reset to develop / Reverted commit XYZ]

**Reason**: [RC/RH trigger description]
**Impact**: All 11 A2 findings reverted OR [single fix reverted]
**Data Loss**: None

**Next Steps**:
1. Investigation underway (eta [2-4 hours])
2. Root cause analysis + fix design
3. Re-test plan documented
4. New PR or extended timeline TBD

**Contact**: [Release Manager]  
**Incident Issue**: #[number]

See rollback playbook: ai_working/INTEGRATION_ROLLBACK_PLAYBOOK.md
```

### Stakeholder Update (if Level 2+)

```markdown
**STREAM A UPDATE: Phase 2-3 Delay**

**New Timeline**:
- Phase 2 Root Cause Fix: Aug [X-Y]
- Phase 3 Re-test: Aug [Y-Z]
- Phase 4 PR: Aug [Z-W]
- **Merge Target**: Aug 31 (UNCHANGED) OR Sep 7 (if Level 3+)
- **Stream B Launch**: Sep 1 (may shift to Sep 8)

**Impact**: [1-3 day delay, no impact to other modules]
**Mitigation**: Additional review, extended test cycles
```

---

## Prevention: Pre-Rollback Validation

To avoid needing rollback, validate before Phase 3 even starts:

- [ ] All P2-A2-01..11 tests PASS in isolation
- [ ] Concurrent hardening tests (A1) still PASS
- [ ] Benchmark baselines stable (A3 locked)
- [ ] No breaking API changes in A1/A2/A3
- [ ] Code review sign-off on A2 fixes before Phase 3 integration
- [ ] Manual smoke test: basic query → storage → LLM pipeline works
- [ ] ThreadSanitizer clean on tensor module + immediate dependencies

---

## Rollback Drill Schedule

**Aug 15-20**: Dry-run Level 1 rollback (simulate, don't execute)  
**Aug 21-23**: Dry-run Level 2 rollback (partial revert of single finding)  
**Aug 24-25**: Full rollback playbook review with team  
**Aug 26**: Finalize on-call escalation (who activates each level?)

