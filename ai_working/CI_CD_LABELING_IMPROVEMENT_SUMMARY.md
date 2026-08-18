# CI/CD PR Labeling and Milestone Assignment — Implementation Summary

**Date:** 2026-08-18  
**Issue:** Improve CI/CD PR labeling quality and add milestone assignment  
**New Requirement:** Add valid milestone assignment (previously missing)

## Problem Analysis

### Before Improvements

1. **Minimal Labeling** (.github/labeler.yml)
   - Only 7 basic labels (area:acceleration, area:vector, area:graph, area:storage, type:documentation, type:test, ai-generated)
   - Missing coverage for 40+ modules (llm, access_model, search, security, audit, process, updates, sharding, etc.)
   - No ROADMAP Wave awareness
   - No semantic/AI analysis

2. **No Milestone Assignment**
   - `automation-community.yml` had no milestone job
   - New requirement: milestone assignment based on ROADMAP Waves and quarters

3. **No Semantic Labeling**
   - Only path-based labeling, no semantic analysis
   - Unable to detect breaking changes, severity, impact from PR content

## Implementation

### 1. Enhanced labeler.yml

**Added Wave Labels:**
- `wave:A` — Access Model, Search, Sharding, Replication, Utils, Updates, Process, Audit (Q3-Q4 2026)
- `wave:B` — LLM, Retrieval, Tensor, GPU, Acceleration (Q3-Q4 2026)
- `wave:C` — Security, Auth, Governance (Q4 2026)
- `wave:D` — Observability, Maintenance, Docs/Operability (Q1 2027)

**Added Module Area Labels (15 total):**
- area:acceleration, area:vector, area:graph, area:storage, area:llm
- area:search, area:security, area:access-model, area:sharding
- area:replication, area:updates, area:process, area:audit
- area:observability, area:plugins, area:api

**Added Change Type Labels:**
- type:documentation, type:test, type:ci, type:build, type:refactor
- type:feature, type:bugfix, type:performance, type:security (AI-detected)

**Added Status Labels:**
- release_critical (Wave A modules)
- ai-generated (ai_working/ files)
- breaking-change, severity:*, impact:*, migration-guide-needed (AI-detected)

### 2. Milestone Assignment (New)

**Created `assign-milestone` job in automation-community.yml:**

- Automatically detects which ROADMAP Wave(s) are affected by PR changes
- Maps to appropriate milestone:
  - Wave A/B/C modules → `Q4-2026`
  - Wave D modules → `Q1-2027`
  - Documentation-only → `Documentation`
  - Other changes → `Backlog`

**Features:**
- Checks if milestone exists before assigning (graceful degradation)
- Skips draft PRs (only assigns to open PRs)
- Handles 100+ changed files per PR
- Logs milestone assignment for debugging

### 3. AI Semantic Labeling (New)

**Created `semantic-labeling` job in automation-community.yml:**

- Analyzes PR title and body using GitHub AI Inference
- Detects:
  - Breaking changes
  - Severity (critical, high, medium, low)
  - Impact scope (single_module, multiple_modules, core, api, infrastructure)
  - Change type (feature, bugfix, refactor, performance, security, documentation)
  - Migration guide requirement

**Features:**
- Gracefully degrades if AI service unavailable or returns invalid JSON
- Merges AI labels with existing path-based labels (no duplicates)
- Only runs for non-draft PRs
- Permissive error handling (can be safely disabled)

### 4. Documentation

**Created `.github/PR_LABELING_GOVERNANCE.md`:**
- Comprehensive label taxonomy (53 labels documented)
- Milestone assignment logic and flow
- Valid milestones list
- AI semantic labeling classification guide
- Troubleshooting section
- Maintenance guidelines

## Configuration Changes

### Files Modified

1. **.github/labeler.yml** (167 lines)
   - Expanded from 31 to 167 lines
   - ~60 path patterns for 20+ labels
   - Added ROADMAP Wave awareness

2. **.github/workflows/automation-community.yml** (365+ lines)
   - Added `assign-milestone` job (60+ lines)
   - Added `semantic-labeling` job (100+ lines)
   - Updated global permissions (added issues:write, pull-requests:write, models:read)

3. **.github/PR_LABELING_GOVERNANCE.md** (NEW)
   - 353 lines of governance documentation
   - Label taxonomy, milestone logic, troubleshooting

### Expected Behavior Changes

#### For Wave A PR (e.g., modifies src/search/retrieval.cpp)

Before:
- Labels: area:storage (wrong), type:test
- Milestone: (none)

After:
- Labels: area:search, wave:A, release_critical, (optionally: breaking-change, severity:*, impact:api)
- Milestone: Q4-2026

#### For Documentation PR (e.g., modifies docs/operability/runbook.md)

Before:
- Labels: type:documentation
- Milestone: (none)

After:
- Labels: type:documentation, wave:D (if in docs/operability/)
- Milestone: Documentation

#### For AI-Generated PR (e.g., modifies ai_working/compact/*)

Before:
- Labels: ai-generated
- Milestone: (none)

After:
- Labels: ai-generated
- Milestone: Backlog (or Q4-2026 if also modifies Wave A code)

## Setup Instructions

### Required Actions

1. **Create GitHub Milestones** (if not already present)
   - Navigate to: Repository Settings > Milestones
   - Create the following milestones:
     - **Q4-2026** — Description: "Q4 2026 delivery cycle (Wave A, B, C exit criteria)"
     - **Q1-2027** — Description: "Q1 2027 delivery cycle (Wave D operability hardening)"
     - **Documentation** — Description: "Documentation and governance updates"
     - **Backlog** — Description: "Unscoped work or future enhancements"

2. **No Branch/PR Changes Required**
   - New labeling applies automatically on PR open/synchronize
   - Existing PRs are not retroactively labeled (GitHub limitation)
   - Workflow is backward compatible

### Optional: Disable Semantic Labeling

If AI semantic labeling produces false positives or is not desired:

1. In `.github/workflows/automation-community.yml`, remove or comment out:
   ```yaml
   # semantic-labeling:
   #   name: Semantic labeling with AI
   #   ...
   ```

2. Remove `models:read` from global permissions

## Verification

### Test Case 1: Wave A Module PR

```bash
# Simulate a PR modifying Wave A module
git checkout -b test/wave-a-search
echo "test" >> src/search/retrieval.cpp
git add -A && git commit -m "test: improve retrieval performance"
# Expected labels: area:search, wave:A, release_critical, type:test
# Expected milestone: Q4-2026
```

### Test Case 2: Documentation PR

```bash
git checkout -b test/docs-wave-d
echo "# Runbook" >> docs/operability/runbook.md
git add -A && git commit -m "docs: add failover runbook"
# Expected labels: type:documentation, wave:D
# Expected milestone: Documentation
```

### Test Case 3: AI-Generated PR (Skip CI)

```bash
git checkout -b test/ai-compact
echo "log" >> ai_working/compact/INDEX.md
git add -A && git commit -m "chore(ai-working): update index [skip ci]"
# Expected labels: ai-generated
# Expected milestone: Backlog (ai_working files don't match Wave patterns)
```

## Known Limitations

1. **Milestones must exist** — If required milestone doesn't exist, assignment is skipped with warning
2. **100 changed files max** — Workflow fetches max 100 files per PR (API limitation)
3. **AI inference optional** — Graceful degradation if GitHub AI service unavailable
4. **No retroactive labeling** — Existing PRs are not relabeled; only new PRs or resynchronizations
5. **Draft PRs excluded** — Milestones only assigned to non-draft PRs

## Maintenance Checklist

**When to Update labeler.yml:**
- [ ] New modules added to Wave A/B/C/D (add to wave:* labels)
- [ ] Build/CI structure changes (update type:ci patterns)
- [ ] New high-level areas emerge (create new area:* label)

**When to Update milestone assignment:**
- [ ] ROADMAP Wave structure changes (update regex patterns)
- [ ] New milestone quarters added (add cases to milestone logic)
- [ ] Major module reorganization (update file path patterns)

**When to Create New Milestones:**
- [ ] New quarterly delivery cycle (Q2-2027, etc.)
- [ ] New program phase or wave
- [ ] New governance phase introduced

## Related Documentation

- **ROADMAP.md** — Wave definitions and target dates
- **RELEASE_STRATEGY.md** — Release gate and milestone criteria
- **BRANCHING_STRATEGY.md** — Branch targeting and edition gating
- **DOCUMENTATION_GOVERNANCE.md** — Label governance integration
- **PR_LABELING_GOVERNANCE.md** — Comprehensive label and milestone reference

## Future Enhancements

1. **Commit Message Parsing** — Extract milestone from PR title/body format (e.g., "targets: Q4-2026")
2. **Label Conflict Detection** — Warn if conflicting labels detected (e.g., both type:feature and type:bugfix)
3. **CODEOWNERS Integration** — Auto-request review from wave-specific code owners
4. **Milestone Filtering** — Filter release notes by milestone/wave
5. **Analytics Dashboard** — Track PR->milestone->release flow metrics

## Rollback Instructions

If issues are discovered and immediate rollback is needed:

1. **Revert workflow changes:**
   ```bash
   git revert <commit-hash-of-workflow-changes>
   git push origin <branch>
   ```

2. **Revert labeler changes:**
   ```bash
   git revert <commit-hash-of-labeler-changes>
   git push origin <branch>
   ```

3. **Disable semantic labeling immediately** (if causing issues):
   - Remove or comment out semantic-labeling job
   - Push immediately to stop new AI label applications

## Conclusion

The improved PR labeling and milestone assignment system provides:

✅ **Comprehensive module coverage** — 20+ areas now labeled automatically  
✅ **ROADMAP alignment** — Wave-based labeling reflects project structure  
✅ **Milestone automation** — No manual milestone assignment needed  
✅ **AI enhancement** — Semantic analysis detects breaking changes, severity, impact  
✅ **Graceful degradation** — All new jobs handle failures without blocking workflow  
✅ **Full documentation** — Governance and troubleshooting guide included  

This enables better PR triage, release planning, and impact analysis going forward.
