# PR Version Targeting Backfill Guide

> **Purpose:** Guidance for backfilling existing open PRs with version targeting metadata  
> **Audience:** Release managers, maintainers  
> **Status:** Active (2026-08-18)

---

## Overview

This guide covers the process of retrofitting existing open PRs with the new "Target Version" field requirement, ensuring all PRs have clear version targeting before the first release using this policy.

---

## 1. Scope & Impact

### 1.1 What Needs to Be Done

All **open PRs** on the following branches need Target Version assignments:
- `develop` (main development branch)
- `release/*` (any open release branches)
- `epic/*` (feature integration branches)

**PRs that are already merged or closed** do not need updates (for historical purposes, the merge date can serve as a proxy for version targeting).

### 1.2 Why This Matters

- **Release Planning**: Release managers need to know which version each open PR targets
- **Milestone Scoping**: GitHub milestones aggregate work for each release
- **Changelog Accuracy**: PRs with version targeting enable automated changelog generation
- **Team Communication**: Clear version assignments prevent scope confusion

---

## 2. Step-by-Step Backfill Process

### 2.1 Phase 1: Audit Open PRs (1-2 hours)

**Who:** Release manager or senior maintainer  
**When:** Immediately after version targeting policy is activated  

**Steps:**

1. **List all open PRs:**
   ```bash
   # Via GitHub CLI (if available):
   gh pr list --state open --limit 999 --json number,title,branch,createdAt
   
   # Or via GitHub UI:
   # Go to Pull Requests → Filter by state:open
   ```

2. **Categorize each PR:**
   - **Type A:** Feature PRs (new features, enhancements)
   - **Type B:** Bug fix PRs (addressing issues)
   - **Type C:** Documentation PRs
   - **Type D:** Infrastructure / Refactoring PRs

3. **Record audit results in:** `ai_working/PR_BACKFILL_AUDIT.md`
   - Example:
     ```markdown
     # PR Backfill Audit (2026-08-18)
     
     ## Summary
     - Total Open PRs: 42
     - Type A (Features): 15
     - Type B (Bug Fixes): 18
     - Type C (Docs): 5
     - Type D (Infrastructure): 4
     
     ## Assignments (TBD)
     | PR # | Title | Type | Target Version | Assigned To |
     |------|-------|------|---|---|
     | #123 | Add query optimization API | A | v2.5.0-alpha1 | @reviewer1 |
     | #124 | Fix sharding timeout | B | v2.4.1 | @reviewer2 |
     ```

### 2.2 Phase 2: Version Assignments (2-4 hours)

**Who:** Release manager + module maintainers  
**When:** Same day or next day after audit

**Process:**

For **each PR**, use this decision tree:

```
1. Is it a feature or enhancement?
   → YES: Assign to next planned MINOR (e.g., v2.5.0-alpha1)
   → NO: Go to step 2

2. Is it a bug fix for the current RC/stable line?
   → YES: Assign to current version (e.g., v2.4.0-rc1 or v2.4.1)
   → NO: Go to step 3

3. Is it a critical/high-priority fix?
   → YES: Assign to next planned MINOR (e.g., v2.5.0-alpha1)
   → NO: Assign to [Unreleased] for triage

4. Check alignment with ROADMAP:
   → If PR scope matches planned features: Keep assignment
   → If PR scope is unplanned: Discuss with maintainers
   → If uncertain: Use [Unreleased] and let release manager triage quarterly
```

**Recommended Assignments (Example):**

| PR Type | Target Version | Rationale |
|---------|---|---|
| Feature / Enhancement | `v2.5.0-alpha1` | Next planned minor release |
| Bug fix (current RC) | `v2.4.0-rc1` | Stabilization for upcoming GA |
| Bug fix (stable) | `v2.4.1` | Patch release for current stable |
| Security fix | `v2.4.0` or `v2.4.1` (stable first) | Current release, then backports |
| Documentation | Same version as documented feature | Example: `v2.5.0-alpha1` if docs feature in alpha |
| Infrastructure / Refactoring | `v2.5.0-alpha1` or `[Unreleased]` | Next minor or unassigned backlog |

**Assignment Tips:**

- **Ask the PR author** when in doubt: comment on the PR asking for version targeting guidance
- **Link to ROADMAP** to justify assignments: "Assigning to v2.5.0-alpha1 because this feature is in the ROADMAP Phase 1"
- **Batch similar PRs** together to make decisions faster
- **Document disagreements**: if the author disagrees with the assignment, comment and discuss

### 2.3 Phase 3: Bulk PR Description Updates (2-3 hours)

**Who:** Automated script + human review OR manual edits  
**When:** After all version assignments are finalized

**Option A: Automated Script (preferred)**

Create a script in `scripts/backfill-pr-versions.sh` to update PR descriptions:

```bash
#!/bin/bash
# Backfill PR Target Version field

# Usage: scripts/backfill-pr-versions.sh <PR_NUMBER> <TARGET_VERSION>
# Example: scripts/backfill-pr-versions.sh 123 v2.5.0-alpha1

PR_NUMBER=$1
TARGET_VERSION=$2

if [[ -z "$PR_NUMBER" ]] || [[ -z "$TARGET_VERSION" ]]; then
  echo "Usage: $0 <PR_NUMBER> <TARGET_VERSION>"
  echo "Example: $0 123 v2.5.0-alpha1"
  exit 1
fi

# Get current PR description
# (This requires GitHub CLI or API)
CURRENT_BODY=$(gh pr view $PR_NUMBER --json body -q '.body')

# Inject Target Version field if not present
if ! echo "$CURRENT_BODY" | grep -q "Target Version"; then
  # Prepend Target Version section after PR header
  NEW_BODY="## Target Version (Required)

**Target Version:** $TARGET_VERSION

---

$CURRENT_BODY"
  
  # Update PR description
  gh pr edit $PR_NUMBER --body "$NEW_BODY"
  echo "✓ Updated PR #$PR_NUMBER with Target Version: $TARGET_VERSION"
else
  echo "⚠ PR #$PR_NUMBER already has Target Version field"
fi
```

**Manual Process (if script is unavailable):**

1. Open each PR on GitHub
2. Edit the description
3. Add the Target Version field at the top (after the "## Description" section)
4. Copy the template from `.github/pull_request_template.md`:
   ```markdown
   ## Target Version (Required)
   
   **Target Version:** v2.5.0-alpha1
   
   <!-- Justify the version choice -->
   This PR adds query optimization APIs planned for v2.5.0-alpha1 per ROADMAP.
   ```
5. Save the PR description

**Option B: GitHub API Script**

Use the GitHub API to bulk update descriptions (requires authentication):

```bash
#!/bin/bash
# Bulk update PR descriptions with Target Version

# Read from CSV: pr_number,target_version
while IFS=, read -r PR_NUM VERSION; do
  curl -X PATCH \
    -H "Authorization: token $GH_TOKEN" \
    -H "Accept: application/vnd.github.v3+json" \
    "https://api.github.com/repos/makr-code/ThemisDB/pulls/$PR_NUM" \
    -d "{\"body\": \"## Target Version (Required)\n\n**Target Version:** $VERSION\n\n---\n\n$CURRENT_BODY\"}"
  echo "Updated PR #$PR_NUM"
done < pr_versions.csv
```

### 2.4 Phase 4: Milestone Assignment (1 hour)

**Who:** Release manager or automation  
**When:** After PR descriptions are updated

**Steps:**

1. **Verify Milestones Exist:**
   - Go to GitHub Issues → Milestones
   - Ensure these milestones are created:
     - `v2.4.0`, `v2.4.1`, `v2.5.0-alpha1`, `v2.5.0-beta1`, `[Unreleased]`
   - Create any missing milestones (see GITHUB_MILESTONES_SETUP.md)

2. **Assign Milestones to PRs:**
   - **Option A (Automated):**
     ```bash
     # GitHub CLI
     gh pr list --state open | while read PR_NUM _; do
       VERSION=$(gh pr view $PR_NUM --json body -q '.body' | grep -oP '(?<=\*\*Target Version:\*\*\s)\S+')
       gh pr edit $PR_NUM --milestone "$VERSION"
     done
     ```
   
   - **Option B (Manual):**
     - For each open PR, click "Milestone" on the right panel
     - Select the version matching the Target Version field
     - Save

3. **Verify Assignments:**
   - Go to Issues → Milestones
   - For each milestone, verify the PR count matches expected count from audit
   - Spot-check a few PRs to ensure correct assignment

### 2.5 Phase 5: Validation & Cleanup (1 hour)

**Who:** Release manager  
**When:** After all assignments are complete

**Checklist:**

- [ ] All open PRs have "Target Version" field in description
- [ ] All PRs are assigned to corresponding milestone on GitHub
- [ ] No PRs are missing milestone assignments (should show "no milestone")
- [ ] Milestone counts match expected counts from audit
- [ ] No PRs have conflicting or invalid target versions
- [ ] Audit document is finalized: `ai_working/PR_BACKFILL_AUDIT.md`

**Validation Command:**

```bash
# List PRs with no milestone (should be empty or only [Unreleased])
gh pr list --state open --json number,title,milestone -q '.[] | select(.milestone == null) | "\(.number): \(.title)"'

# Expected output: (empty, or only show PRs intentionally in [Unreleased])
```

---

## 3. Timeline & Effort Estimate

| Phase | Duration | Effort | Owner |
|-------|----------|--------|-------|
| 1. Audit | 1-2 hours | Low | RM + 1 person |
| 2. Version Assignments | 2-4 hours | Medium | RM + module maintainers |
| 3. PR Description Updates | 2-3 hours | Medium | Automated script + review |
| 4. Milestone Assignment | 1 hour | Low | RM + automation |
| 5. Validation & Cleanup | 1 hour | Low | RM |
| **Total** | **7-11 hours** | | |

**Note:** This is a one-time effort. After the backfill, all NEW PRs will include the Target Version field automatically via the PR template.

---

## 4. Special Cases

### 4.1 Draft PRs

**Recommendation:** Include draft PRs in the backfill.

- Draft PRs in progress should still declare target versions
- Helps with release planning visibility
- Draft status doesn't exclude a PR from version targeting

### 4.2 Stale PRs (not updated in 3+ months)

**Recommendation:** Review for closure before backfilling.

**Steps:**
1. For each PR with no activity in 3+ months:
   - Comment asking PR author for status
   - Wait 1 week for response
   - If no response: close with note "Closing due to inactivity; reopen if still relevant"
2. Only backfill active/recent PRs

### 4.3 Dependent PRs (PR chains)

**Recommendation:** Assign all PRs in a chain to the same target version.

**Example:**
- PR #100 (Feature implementation) → `v2.5.0-alpha1`
- PR #101 (Docs for feature) → `v2.5.0-alpha1` (same version)
- PR #102 (Performance improvement) → `v2.5.0-beta1` (next phase)

### 4.4 PRs Awaiting Rebase/Conflict Resolution

**Recommendation:** Still backfill; version targeting is independent of merge status.

- Assign version even if PR has conflicts
- Version helps track priority for conflict resolution
- Encourage author to rebase and resolve before merge

---

## 5. Post-Backfill Monitoring

After backfill is complete:

### 5.1 Release Manager Responsibilities (Ongoing)

- [ ] Weekly: Review new PRs to ensure they have Target Version field
- [ ] Monthly: Audit milestone assignments vs. ROADMAP scope
- [ ] Before each release: Run RELEASE_VALIDATION_CHECKLIST.md to verify scope

### 5.2 CI/CD Validation

- [ ] GitHub Actions workflow `validate-pr-version-targeting.yml` is active
- [ ] Workflow blocks PRs without Target Version (or at minimum warns)
- [ ] Review workflow logs weekly to catch missing versions

### 5.3 Quarterly Backlog Triage

- [ ] Every quarter: Review `[Unreleased]` milestone
- [ ] Assign items to upcoming versions or close if no longer relevant
- [ ] Log triage decisions (see GITHUB_MILESTONES_SETUP.md §5)

---

## 6. Rollback Plan

If the version targeting policy causes issues:

1. **Revert PR template changes:** Remove the Target Version field section
2. **Disable workflow:** Set `.github/workflows/validate-pr-version-targeting.yml` to `if: false`
3. **Notify team:** Post on GitHub Discussions explaining the issue and temporary revert
4. **Fix & retry:** After fixing the issue, reapply the policy

---

## 7. Related Documents

- [docs/governance/PR_VERSION_TARGETING.md](PR_VERSION_TARGETING.md) — Full version targeting policy
- [.github/pull_request_template.md](../../.github/pull_request_template.md) — PR template with Target Version field
- [GITHUB_MILESTONES_SETUP.md](GITHUB_MILESTONES_SETUP.md) — Milestone creation and management
- [RELEASE_VALIDATION_CHECKLIST.md](RELEASE_VALIDATION_CHECKLIST.md) — Pre-release checklist
- [VERSIONING.md](../../VERSIONING.md) — Version format rules
- [RELEASE_STRATEGY.md](../../RELEASE_STRATEGY.md) — Release process

---

**Backfill Guide Created:** 2026-08-18  
**Expected Backfill Date:** [Fill in when completed]  
**Backfill Completed By:** [Name]  
**Status:** PENDING / IN PROGRESS / COMPLETE
