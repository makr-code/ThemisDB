# EPIC #5517 PR Validation Checklist

**For**: PR reviewers and maintainers validating EPIC #5517 contributions  
**Status**: Ready for use  
**Owner**: @makr-code

---

## Overview

Use this checklist when reviewing PRs that target `epic/research-review-5517`.

Ensures compliance with:
- Branch governance (BRANCHING_STRATEGY.md)
- EPIC structure (EPIC_5517_RESEARCH_REVIEW_CONSOLIDATION.md)
- PR process (epic-branch-flow.md template)

---

## Pre-Review Checklist

Before diving into code/content review:

### Basic PR Structure

- [ ] **Branch name follows convention**
  - ✅ Correct: `feature/5517-`, `fix/5517-`, `chore/5517-`
  - ❌ Wrong: `feature/research-5517`, `main`, `develop`
  - **Action if failed**: Request branch rename or close and re-open with correct name

- [ ] **Target branch is correct**
  - ✅ Correct: `epic/research-review-5517`
  - ❌ Wrong: `develop`, `main`, `community`, any other
  - **Action if failed**: Reject PR with comment: "PRs must target epic/research-review-5517, not develop. See branch governance in BRANCHING_STRATEGY.md"

- [ ] **Source branch is not develop or main**
  - ✅ Correct: Feature branch created from epic/research-review-5517
  - ❌ Wrong: Feature branch created from develop
  - **Action if failed**: Ask contributor to rebase onto epic/research-review-5517

- [ ] **PR title follows convention**
  - ✅ Correct: `[EPIC #5517] Research issue consolidation` or `chore(#5517): Update tracker`
  - ❌ Wrong: `Fix research`, `Update docs`, `[#5517]`, no reference to EPIC
  - **Action if failed**: Request title edit (use GitHub PR edit feature)

---

## Content Review Checklist

For all PRs to epic/research-review-5517:

### EPIC Context Requirements

- [ ] **PR references parent EPIC #5517**
  - Location: PR description mentions #5517
  - Format: "Closes #5517" (if completing) or "Refs #5517" (if partial)
  - **Action if missing**: Request addition of reference

- [ ] **Sub-issues referenced (if applicable)**
  - If PR affects specific research issues, they're mentioned
  - Examples: "Affects #5400, #5401, #5402"
  - **Action if missing**: OK if infrastructure-only work; request if research-related

- [ ] **Workstream classification is clear**
  - PR describes which phase it addresses: draft/finalize/transfer/archived
  - **Action if missing**: Request clarification in PR description

---

### Governance Compliance

- [ ] **No direct mentions of bypassing epic branch**
  - Watch for comments like "this could be squashed into develop"
  - ❌ Never allow direct develop merges
  - **Action if found**: Reject with governance reminder

- [ ] **Branch protection rules acknowledged**
  - PR doesn't request disabling status checks
  - PR doesn't request force-push capability
  - **Action if found**: Reject request

- [ ] **No unilateral removal of parent-child links**
  - If PR removes issue links, reason is documented
  - **Action if suspicious**: Request explanation

---

### Documentation Quality

For PRs adding/modifying documentation:

- [ ] **Governance documents updated if needed**
  - Changes to branch model? → Update EPIC_5517_RESEARCH_REVIEW_CONSOLIDATION.md
  - Changes to issue structure? → Update RESEARCH_BACKLOG_TRACKER.md
  - Changes to branch setup? → Update EPIC_5517_BRANCH_SETUP_GUIDE.md

- [ ] **Tracker updated with new issues (if applicable)**
  - New research issues mentioned? → Check RESEARCH_BACKLOG_TRACKER.md updated
  - New phase reached? → Tracker reflects current counts

- [ ] **Issue template remains consistent**
  - Changes to issue tracking fields? → .github/ISSUE_TEMPLATE/research-review-sub-issue.md updated
  - New labels introduced? → Documented in governance doc

- [ ] **Cross-references accurate**
  - Links point to correct issues/docs
  - File paths valid
  - Branch names up-to-date

---

### Process Compliance

- [ ] **PR uses correct template**
  - Contains sections from epic-branch-flow.md:
    - EPIC Context (EPIC issue, sub-issues, flow type)
    - Branch Routing (source, target, naming check)
    - Change Summary
    - Linked Issues
    - Validation Evidence
    - EPIC Acceptance Impact

- [ ] **Validation evidence provided**
  - Build status mentioned (if code changes)
  - Relevant tests passed (if code changes)
  - Review/approval status (if required)
  - Example: "✅ Build passed on feature/5517-xyz"

- [ ] **Risk assessment reasonable**
  - PR correctly identifies risk level (Low/Medium/High)
  - Risk level matches scope of changes
  - Rollback plan provided if Medium+ risk

- [ ] **Linked issues properly referenced**
  - Parent EPIC: #5517
  - Sub-issues: Listed with relationship type
  - Related work: Other PRs/issues mentioned if applicable

---

## Content Validation (By Type)

### For Governance Document PRs

- [ ] **Structure preserved**
  - Sections match template from base EPIC doc
  - Hierarchy maintained
  - No orphaned subsections

- [ ] **Terminology consistent**
  - "workstream" not "phase" or "track" (except in titles)
  - "research-draft", "research-finalize", "transfer-to-engineering" exact spelling
  - "P0/P1/P2" consistent capitalization

- [ ] **Examples accurate**
  - Sample issue links still valid
  - Branch names follow convention
  - File paths point to real files

- [ ] **Acceptance criteria realistic**
  - Criteria are verifiable
  - Not vague ("improve", "better", "ensure")
  - Measurable where possible

### For Research Issue PRs

- [ ] **Issue links to parent #5517**
  - Confirmed via "Link issue" in sidebar
  - Shows in PR as "Affects #5517" or "Child of #5517"

- [ ] **Labels applied correctly**
  - ONE workstream label (research-draft/finalize/transfer/archived)
  - ONE priority label (P0/P1/P2)
  - Includes `research-review` label
  - All labels exist in GitHub labels list

- [ ] **Issue description follows template**
  - Uses .github/ISSUE_TEMPLATE/research-review-sub-issue.md structure
  - All required sections filled
  - Owner assigned
  - Transition criteria documented

### For Tracking/Status PRs

- [ ] **Counts accurate**
  - Issue counts match current GitHub search:
    ```
    is:open label:research-review parent:#5517
    ```

- [ ] **Workstream distribution realistic**
  - No issues missing from counts
  - Totals match sum of workstreams
  - No obvious math errors

- [ ] **Timeline reasonable**
  - Phase target dates in future or reasonable
  - Phase duration estimates realistic (typically 2-4 weeks)
  - No circular dependencies

- [ ] **Progress tracking clear**
  - Checkboxes used for task tracking
  - Items marked [x] are truly complete
  - Blocker items have explanations

---

## Red Flags (Automatic Reject)

❌ **Reject immediately if**:

1. **PR targets `develop` instead of `epic/research-review-5517`**
   - This violates core governance
   - Comment: "This PR must target `epic/research-review-5517`, not develop. See BRANCHING_STRATEGY.md and EPIC #5517 governance."

2. **Branch name doesn't follow convention**
   - No `feature/5517-`, `fix/5517-`, or `chore/5517-` prefix
   - Comment: "Branch must follow naming convention: feature/5517-*, fix/5517-*, chore/5517-*"

3. **PR doesn't reference EPIC #5517**
   - No mention of parent EPIC anywhere in description
   - Comment: "Please reference parent EPIC #5517 in PR description."

4. **PR removes issue links without explanation**
   - Unlinks from parent #5517 with no clear reason
   - Comment: "Why is this unlinked from #5517? Please restore link or explain."

5. **PR tries to merge epic branch to develop**
   - Consolidation PR attempted outside EPIC governance
   - Comment: "Consolidation to develop requires full EPIC completion and sign-off. See phase 6 in governance doc."

6. **Critical security vulnerabilities**
   - CodeQL alerts or secret scanning issues
   - Comment: "This PR introduces security concerns. Please resolve before review."

---

## Approval Criteria

✅ **Approve if**:

- ✓ All structural checks pass (branch, target, template)
- ✓ All governance compliance checks pass (references, links, workstreams)
- ✓ Content quality is acceptable (clear, complete, accurate)
- ✓ No red flags (see above)
- ✓ Risk assessment and rollback plan (if needed) provided
- ✓ For code changes: build + tests passing

### Partial Approval

Use "Request changes" if:
- Minor documentation fixes needed
- Template sections incomplete
- Labels need adjustment
- Cross-reference updates needed

Use "Comment" for:
- Clarifying questions
- Suggestions for improvement
- Future enhancements (not blockers)

---

## Merge Workflow

### Before Merge

1. **Confirm approvals**
   - At least 1 review approval (from @makr-code or designated reviewer)
   - All checks passing (build, lint, tests if applicable)

2. **Verify branch state**
   ```bash
   git checkout epic/research-review-5517
   git pull origin epic/research-review-5517
   git log --oneline | head -1
   # Should show recent commit before this PR
   ```

3. **Check PR one final time**
   - Description complete
   - Linked issues correct
   - No last-minute changes that need review

### Merge Options

- **Squash and merge** (default): For single-issue governance/documentation PRs
- **Create a merge commit**: For multi-file or multi-issue PRs (preserves history)
- **Rebase and merge**: Rarely used; only if linear history needed

### After Merge

1. **Verify merge to epic branch**
   ```bash
   git pull origin epic/research-review-5517
   git log --oneline | head -1
   # Should show merged PR commit
   ```

2. **Update tracking document**
   - Check if RESEARCH_BACKLOG_TRACKER.md needs phase update
   - Update phase progress in ai_working/RESEARCH_BACKLOG_TRACKER.md if needed

3. **Monitor for blockers**
   - Did merge cause issues with other PRs?
   - Do any branch protection checks need updates?

---

## Review Checklist Template

Copy and paste for quick review:

```markdown
## PR Review: epic/research-review-5517

### Pre-Review
- [ ] Branch name follows convention
- [ ] Target is epic/research-review-5517 (not develop)
- [ ] Source is feature branch (not develop)
- [ ] PR title references EPIC

### Content
- [ ] References parent EPIC #5517
- [ ] Sub-issues referenced (if applicable)
- [ ] Workstream phase clearly stated
- [ ] No governance violations

### Documentation
- [ ] Governance docs updated if needed
- [ ] Tracker updated with new counts
- [ ] Cross-references accurate
- [ ] Issue template remains consistent

### Process
- [ ] Uses epic-branch-flow template
- [ ] Validation evidence provided
- [ ] Risk assessment reasonable
- [ ] Rollback plan clear (if needed)

### Final Check
- [ ] No red flags detected
- [ ] Ready for merge to epic/research-review-5517
- [ ] Approval: ✓ or ⊗

**Reviewer**: @[username]  
**Date**: [date]
```

---

## Quick Reference

| Check | Pass | Fail |
|---|---|---|
| Branch name (feature/5517-*, etc.) | ✓ Continue | ✗ Reject |
| Target branch (epic/research-review-5517) | ✓ Continue | ✗ Reject |
| References EPIC #5517 | ✓ Continue | ✗ Request changes |
| Workstream clear | ✓ Continue | ✗ Request changes |
| Uses epic-branch-flow template | ✓ Continue | ✗ Request changes |
| Documentation accurate | ✓ Approve | ✗ Request changes |
| Build + tests passing | ✓ Approve | ⚠️ Check logs |
| No security issues | ✓ Approve | ✗ Reject |

---

## Escalation

### When to escalate to @makr-code

- Unclear branch governance question
- Potential policy violation
- Timeline/priority disputes
- Cross-EPIC coordination needed
- Blocker preventing merge

**Escalation comment format**:
```
@makr-code - Escalation needed on PR #[id]
[Brief description of issue]
Please advise.
```

---

## Additional Resources

| Resource | Link | Use Case |
|---|---|---|
| EPIC Governance | ai_working/EPIC_5517_RESEARCH_REVIEW_CONSOLIDATION.md | Understanding EPIC strategy |
| Branch Setup | ai_working/EPIC_5517_BRANCH_SETUP_GUIDE.md | Validating branch configuration |
| Backlog Tracker | ai_working/RESEARCH_BACKLOG_TRACKER.md | Verifying issue counts |
| Quick Start | ai_working/EPIC_5517_QUICK_START.md | Helping contributors understand flow |
| Branch Strategy | BRANCHING_STRATEGY.md | Core git model reference |
| PR Template | .github/PULL_REQUEST_TEMPLATE/epic-branch-flow.md | Checking PR structure |

---

**Created**: 2026-08-08  
**Owner**: Copilot Coding Agent  
**Next Review**: After first 5 PRs merged
