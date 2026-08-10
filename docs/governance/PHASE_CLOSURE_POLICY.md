# Phase Closure Enforcement Policy

**Document:** Module Phase Lock-In and Regression Prevention  
**Status:** Phase 3 Enforcement (2026-Q4)  
**Version:** 1.0  
**Owner:** Platform Release / @makr-code  
**Last Updated:** 2026-08-10

---

## Purpose

This document defines the rules and enforcement mechanisms for locking in phase completions, preventing phase regressions, and managing maturity freeze after release.

**Key Principle:** *Once a phase is marked complete (`[x]`), it is locked. Regressions are blocked. Phase reopening requires human justification and multi-party approval.*

---

## Phase Definition & Lifecycle

### Phase States

```
[_] Pending  ──(design/implementation begins)──→  
[~] In Progress  ──(implementation ongoing)──→  
[x] Complete  ──(locked: no regressions allowed)──→  
    ├─ In Release Lane (read-only, hotfix-only)
    └─ Post-Release (historic; locked forever)
```

---

## Phase Closure Requirements

### Acceptance Criteria Verification

**For Phase X to close (mark `[x]`):**

1. **Minimum Acceptance Criteria Met (Auto-Checked)**
   - All "acceptance criteria" items in `src/<module>/ROADMAP.md` §Phase X must have clear evidence
   - Acceptance criteria are checkboxes: `- [x] <criterion description>`
   - Evidence must be one of:
     - Test file with specific test count (e.g., "P1-01..08 tests in test_phase1_focused.cpp")
     - Benchmark file with specific benchmark count (e.g., "FP23-01..06 benchmarks in bench_failover_phase2_phase3_gates.cpp")
     - Merged PR or commit hash (e.g., "#12345" or "commit abc1234")
     - Documentation file (e.g., "include/auth/auth_principal_contract.h v1.x frozen")
   - Automated checker: `.github/scripts/module_phase_verifier.py --module <name> --phase <N> --verify-acceptance`

2. **Test Count ≥ 80% of Acceptance Criteria**
   - Count test files matching `tests/<module>/test_*_phase<N>*_focused.cpp` pattern
   - Compare against acceptance criteria item count
   - If test count < 80% of criteria, phase cannot close
   - Exception: Design-only phases (e.g., Phase 1) may have lower threshold if design doc is comprehensive
   - Automated checker: `.github/scripts/module_phase_verifier.py --module <name> --phase <N> --check-test-ratio`

3. **Zero Blockers in Implementation**
   - No GitHub issues with label `[blocker] Phase-<N>` exist for this module
   - Blocker = critical defect or missing feature that breaks phase contract
   - Automated check: query GitHub API for blocker issues
   - Resolution: all blockers must be closed before phase closure

4. **Human Sign-Off from Domain Owner**
   - Domain owner (module author/maintainer) must explicitly approve phase closure
   - Sign-off stored in phase header comment in ROADMAP.md:
     ```markdown
     ## Phase X: <Phase Name>
     <!-- PHASE_CLOSURE_SIGN_OFF:
          Module: <name>
          Phase: <N>
          Approved By: <github-handle>
          Approval Date: <YYYY-MM-DD>
          Acceptance Criteria: <count> items, <test_count> tests (ratio: <X>%)
          Merge Commit: <hash>
     -->
     ```
   - Sign-off cannot be automated; must be explicit PR review approval

5. **Timestamp Capture: PR Merge Date = Phase Closure Date**
   - When PR updating ROADMAP with `[x]` is merged, record merge timestamp
   - Timestamp used as "phase closure date" for freeze and regression detection
   - Automated: GitHub Actions workflow captures timestamp and appends to `ai_working/PHASE_CLOSURE_LOG.json`

### Phase Closure Workflow (Automated PR Gate)

**Trigger:** PR proposes changing `[ ]` or `[~]` → `[x]` in `src/*/ROADMAP.md`

**Workflow:** `.github/workflows/11-governance_module-phase-gate.yml`

**Actions:**

1. Extract phase details from PR diff
2. Run acceptance criteria checker:
   ```bash
   python .github/scripts/module_phase_verifier.py \
     --module <name> --phase <N> --verify-acceptance
   ```
3. Check test ratio:
   ```bash
   python .github/scripts/module_phase_verifier.py \
     --module <name> --phase <N> --check-test-ratio
   ```
4. Query GitHub for blocker issues
5. Post validation summary comment on PR:
   ```
   ✅ Phase X Closure Validation
   
   - Acceptance Criteria: 10/10 items have evidence ✅
   - Test Ratio: 9/10 tests present (90%) ✅
   - Blockers: 0 open `[blocker] Phase-X` issues ✅
   - Domain Owner Sign-Off: Pending
   
   ⚠️ Waiting for domain owner approval before merge
   ```
6. Block merge until:
   - All checks PASS
   - Domain owner has approved PR (review from repo collaborator list)

---

## Phase Regression Detection & Prevention

### Regression Conditions

**A phase is considered to have regressed if:**

1. **Test Count Drops > 10%**
   - Compare current test count in `tests/<module>/test_*_phase<N>*_focused.cpp` against:
     - Baseline (recorded at phase closure)
     - Previous week's count
   - If current < baseline × 0.9: regression detected
   - Exclusion: flaky test removal is not regression (must be documented)

2. **Implementation LOC Reduced > 15% (Non-Stub)**
   - Count lines of production code in `src/<module>/*.cpp` (excluding test files, benchmarks)
   - Exclude stub methods: `// STUB:` comments don't count
   - If current LOC < baseline × 0.85: regression detected
   - Exclusion: cleanup of duplicate/dead code is not regression

3. **New CRITICAL Findings Introduced**
   - Run latest security scan (sanitizer, clang-tidy, CodeQL)
   - If any new CRITICAL findings in code touched by recent commits: regression detected
   - Exclusion: pre-existing findings don't count

### Regression Detection Workflow

**Trigger:** Daily automated check on `develop` branch

**Workflow:** `.github/workflows/phase_regression_detector.yml` (new)

**Actions:**

1. For each module at Phase ≥ 1:
   - Fetch baseline (recorded at phase closure)
   - Check current test count, LOC, security findings
   - If regression detected: auto-create GitHub issue
2. Issue template:
   ```
   [phase-regression] <Module> Phase <N> — <regression reason>
   
   ## Summary
   Phase <N> for <Module> has regressed:
   - Test count: <baseline> → <current> (-<percent>%)
   - OR Implementation LOC: <baseline> → <current> (-<percent>%)
   - OR New CRITICAL findings: <list>
   
   ## Impact
   Phase <N> cannot advance until regression is resolved.
   
   ## Required Action
   1. Investigate regression root cause
   2. Restore tests / code / fix findings
   3. Re-validate regression detector
   4. Close this issue when regression resolved
   
   Blocks: Phase <N+1> advancement
   ```
3. Label: `[phase-regression]`, auto-assign to module owner
4. Block: Cannot advance phase if active regression issue exists

---

## Module Maturity Freeze (Release Lane)

### Freeze Trigger

**When:** Module at Phase 6 is included in a release tag (e.g., `v2.4.0` on `community` branch)

**Actions:**

1. Module is marked as "frozen" in release lane
2. Recorded in `docs/governance/RELEASE_LANE_FROZEN_MODULES.md`:
   ```markdown
   | Module | Phase | Released | Freeze Date | Exceptions |
   |--------|-------|----------|------------|-----------|
   | process | 6 | v2.4.0 | 2026-08-15 | hotfix only |
   | auth | 6 | v2.4.0 | 2026-08-15 | hotfix only |
   ```

### Freeze Constraints (Released Modules)

Once frozen:

1. **Read-Only in Release Lane**
   - No new features or improvements can land
   - Only security/compliance patches allowed (via hotfix protocol in `BRANCHING_STRATEGY.md`)

2. **Phase Changes Blocked**
   - Cannot reopen phase (regression)
   - Cannot skip to new phase (must follow sequence)
   - Any phase change requires joint approval: domain owner + release lead
   - Approval must reference business justification and full regression testing plan

3. **Hotfix Exception**
   - Cherry-pick specific commits from `develop` to release lane
   - Hotfix must include:
     - Security fix (CVE or internal assessment)
     - Compliance fix (regulatory requirement)
     - Critical data corruption fix
   - All other changes must wait for next release

4. **Regression Blocks Hotfix**
   - If module has active regression issue, hotfix cannot land until regression resolved
   - Prevents shipping code with known regressions

### Freeze Removal (Next Release)

When planning next release:

1. Module can be moved to new release branch with updated phase
2. Must repeat Phase Closure Verification (acceptance criteria, test ratio, sign-off)
3. All regressions must be resolved before unfreezing

---

## Phase Reopening (Regression Recovery)

### Authorization

**To reopen a Phase marked `[x]`:**

1. Domain owner submits PR with:
   - Clear justification: "Why is this phase being reopened?"
   - Impact statement: "What changed to necessitate reopening?"
   - Remediation plan: "What will be fixed before re-closing?"
2. PR must be approved by:
   - Domain owner (submitter)
   - Release lead (for Tier 0 approval)
3. Approval post-timestamps stored in phase header comment

### Process

1. Change phase marker from `[x]` to `[~]` in ROADMAP.md
2. Post explanation comment in PR:
   ```markdown
   ## Phase Reopening Justification
   
   Module: <name>
   Phase: <N>
   Reason: <specific regression or critical finding>
   
   Expected remediation timeline: <date>
   
   Approved by:
   - Domain Owner: @<handle> (approval timestamp)
   - Release Lead: @<handle> (approval timestamp)
   ```
3. Once merged:
   - Regression becomes tracked issue (if not already)
   - Phase becomes `[~] In Progress` again
   - Phase closure gate waits for re-closure with full verification

---

## Compliance & Audit Trail

### Closure Tracking Log

**File:** `ai_working/PHASE_CLOSURE_LOG.json`

**Format:**
```json
{
  "module": "auth",
  "phase": 6,
  "action": "CLOSED",
  "closure_date": "2026-08-07T14:30:00Z",
  "approval_by": "domain-owner",
  "merge_commit": "abc1234",
  "test_count": 42,
  "test_ratio": 0.95,
  "acceptance_criteria": 44,
  "blocker_count": 0,
  "released_in": ["v2.4.0"],
  "release_date": "2026-08-15",
  "freeze_status": "ACTIVE"
}
```

### Regression Tracking Log

**File:** `ai_working/PHASE_REGRESSION_LOG.json`

**Format:**
```json
{
  "module": "process",
  "phase": 5,
  "detection_date": "2026-08-16T03:00:00Z",
  "regression_type": "test_count_drop",
  "baseline_tests": 72,
  "current_tests": 65,
  "drop_percent": 9.7,
  "github_issue": "#12456",
  "resolution_date": "2026-08-18T10:00:00Z",
  "resolved_by": "pr-#12789"
}
```

---

## Enforcement Summary

| Event | Enforcement | Automation |
|-------|-------------|-----------|
| Phase closure attempt | Verify acceptance + test ratio + sign-off; block if incomplete | Workflow gate |
| Phase regression detected | Auto-create issue; block phase advancement | Daily check + issue creation |
| Released module phase change | Require domain owner + release lead approval | Manual approval |
| Hotfix to released module | Only security/compliance; must re-verify regression | Hotfix workflow gate |
| Phase reopening | Require full justification + multi-party approval | Manual PR review |

---

## References

- `BRANCHING_STRATEGY.md` — Hotfix protocol
- `RELEASE_PROMOTION_GATE_POLICY.md` — Module phase dependency gates
- `PHASE_DEPENDENCY_GRAPH.md` — Cross-module prerequisites
- `.github/workflows/11-governance_module-phase-gate.yml` — Phase closure gate implementation

