# EPIC #5517: Branch Setup & Verification Guide

**Objective**: Ensure `epic/research-review-5517` branch is properly configured and ready for integration work.

**Owner**: @makr-code  
**Created**: 2026-08-08  
**Status**: Ready for Execution

---

## 1. Pre-Flight Checklist

Before starting branch work, verify:

- [ ] You have push access to https://github.com/makr-code/ThemisDB
- [ ] You understand BRANCHING_STRATEGY.md (canonical branch model)
- [ ] You have read the epic-branch-flow.md PR template
- [ ] You have local git configured with correct SSH/HTTPS credentials

---

## 2. Branch Creation & Verification

### Option A: Branch Already Exists (Verify)

If `epic/research-review-5517` already exists, verify it:

```bash
cd /home/runner/work/ThemisDB/ThemisDB

# Fetch latest from remote
git fetch origin

# Check if branch exists locally
git branch -a | grep research-review-5517

# Expected output:
# remotes/origin/epic/research-review-5517
#   epic/research-review-5517
```

**Verification Steps**:

```bash
# Verify branch exists and is accessible
git checkout epic/research-review-5517
git log --oneline -n 5

# Should show recent commits from this branch
# Example output:
# abc1234 Feature or initial commit
# def5678 Previous work
```

### Option B: Create New Branch (if needed)

If branch doesn't exist, create it from develop:

```bash
# Ensure develop is up-to-date
git checkout develop
git pull origin develop

# Create epic/research-review-5517 from develop
git checkout -b epic/research-review-5517
git push -u origin epic/research-review-5517

# Verify creation
git branch -a | grep research-review-5517
```

### Verification Output

Expected result after creation or verification:

```
$ git branch -a | grep research-review-5517
  epic/research-review-5517
  remotes/origin/epic/research-review-5517
```

---

## 3. Branch Protection Rules Configuration

### Purpose

Branch protection ensures:
- No accidental direct commits to EPIC branch
- All work goes through feature branches + PRs
- Proper CI/CD gates before integration
- Clear audit trail via PR reviews

### GitHub Web UI Configuration

1. Navigate to: https://github.com/makr-code/ThemisDB/settings/branches
2. Click "Add rule"
3. Fill in protection rules:

```
Branch name pattern: epic/research-review-5517

Protection Settings:
☑ Require pull request reviews before merging
  ├─ Require approvals: 1
  └─ Dismiss stale pull request approvals when new commits are pushed: Yes

☑ Require status checks to pass before merging
  ├─ Require branches to be up to date before merging: Yes
  └─ (Select relevant CI status checks for this repo)

☑ Require code reviews before merging

☑ Restrict who can push to matching branches
  └─ (If needed; typically leave unset to allow all with write access)

☑ Allow force pushes
  └─ (Select: Do not allow force pushes - prevents history rewrites)

☐ Allow deletions (leave unchecked - preserve branch)
```

### CLI Verification (GitHub CLI)

```bash
# If you have GitHub CLI installed
gh repo rules list --branch epic/research-review-5517

# Expected: Shows rules created above
```

---

## 4. GitHub Labels Configuration

Create labels for research-review issue classification:

### Label Creation

Navigate to: https://github.com/makr-code/ThemisDB/labels

Create the following labels:

#### Workstream Labels

| Label | Color | Description |
|---|---|---|
| `workstream/research-draft` | #1f883d | Active research exploration or conceptual phase |
| `workstream/research-finalize` | #0366d6 | Research conclusions documented, findings validated |
| `workstream/transfer-to-engineering` | #a371f7 | Research approved for engineering implementation |
| `workstream/archived` | #6a737d | Research shelved, not prioritized for 2026 |

#### Priority Labels (if not exists)

| Label | Color | Description |
|---|---|---|
| `priority/p0` | #d1242f | Highest priority, critical path |
| `priority/p1` | #fbca04 | Medium priority, important |
| `priority/p2` | #cccccc | Lower priority, longer term |

#### Status Labels (if not exists)

| Label | Color | Description |
|---|---|---|
| `status/blocked` | #f29513 | Issue is blocked on external input |
| `status/in-progress` | #0366d6 | Currently being worked on |
| `status/review` | #fbca04 | Pending review or approval |

#### Tracking Label (root)

| Label | Color | Description |
|---|---|---|
| `research-review` | #c5def5 | Part of research-review backlog |

### CLI Creation (Example)

```bash
gh label create "workstream/research-draft" \
  --description "Active research exploration" \
  --color 1f883d

gh label create "priority/p0" \
  --description "Highest priority" \
  --color d1242f
```

---

## 5. Initial Commit to EPIC Branch

After branch creation, add governance documents:

### Files to Commit

1. `ai_working/EPIC_5517_RESEARCH_REVIEW_CONSOLIDATION.md` (governance)
2. `ai_working/RESEARCH_BACKLOG_TRACKER.md` (tracking)
3. `.github/ISSUE_TEMPLATE/research-review-sub-issue.md` (new - see §6)

### Commit Process

```bash
cd /home/runner/work/ThemisDB/ThemisDB

# Ensure on EPIC branch
git checkout epic/research-review-5517

# Add governance files (already created)
git add ai_working/EPIC_5517_RESEARCH_REVIEW_CONSOLIDATION.md
git add ai_working/RESEARCH_BACKLOG_TRACKER.md

# Create initial commit
git commit -m "chore(#5517): Add EPIC governance and tracking documentation

- EPIC governance document with structure, phases, and acceptance criteria
- Research backlog consolidation tracker for phase monitoring
- Workstream classification guidance (draft/finalize/transfer)
- Branch and PR governance for EPIC #5517 implementation"

# Push to remote
git push origin epic/research-review-5517
```

---

## 6. Issue Template Creation

Create an issue template for consistent sub-issue structure:

### File Location

Create: `.github/ISSUE_TEMPLATE/research-review-sub-issue.md`

### Template Content

```markdown
---
name: Research Review Sub-Issue
about: Used for research-review issues linked to EPIC #5517
labels: 'research-review'
---

## Research Overview

**Parent EPIC**: #5517 - Research Review Backlog 2026 (Consolidation & Prioritization)

**Research Topic**: [brief title of research area]

**Owner**: @[username]

## Status

**Workstream**:
- [ ] research-draft (active exploration)
- [ ] research-finalize (conclusions ready)
- [ ] transfer-to-engineering (approved for implementation)
- [ ] archived (shelved for now)

**Priority**: 
- [ ] P0 (highest)
- [ ] P1 (medium)
- [ ] P2 (lower)

## Research Objective

[Describe the research question or exploration goal]

## Current Status

[Current findings, progress, or blockers]

## Key Artifacts

[Links to research documents, findings, notes]
- Document 1: [link]
- Document 2: [link]

## Transition Criteria

[What conditions must be met to move to next workstream phase]

## Engineering Applicability

[If applicable: how could this research inform engineering work]

## Related Issues

[Links to related research or engineering work]

## Blockers

[Any external dependencies or blockers]
```

---

## 7. Verification Checklist

After setup, verify all components:

### Branch Verification

```bash
# 1. Verify branch exists and is from develop
git checkout epic/research-review-5517
git log --oneline | head -1

# 2. Verify no local uncommitted changes
git status

# Expected: working tree clean
```

### GitHub Verification

```bash
# 3. Visit in browser and verify:
# - Branch exists: https://github.com/makr-code/ThemisDB/tree/epic/research-review-5517
# - Branch protection: https://github.com/makr-code/ThemisDB/settings/branches
# - Labels created: https://github.com/makr-code/ThemisDB/labels

# 4. Verify initial commit pushed
# Expected: "chore(#5517): Add EPIC governance and tracking documentation"
```

### Label Verification

```bash
# 5. Check labels exist (GitHub web UI)
# Visit: https://github.com/makr-code/ThemisDB/labels
# Verify these exist:
#  - workstream/research-draft
#  - workstream/research-finalize
#  - workstream/transfer-to-engineering
#  - priority/p0
#  - priority/p1
#  - priority/p2
#  - research-review
```

### Template Verification

```bash
# 6. Check issue template created
ls -la .github/ISSUE_TEMPLATE/research-review-sub-issue.md

# 7. Create test issue using template
# Visit: https://github.com/makr-code/ThemisDB/issues/new
# Select template: "Research Review Sub-Issue"
# Verify structure loads correctly
# (Do NOT submit - just verify)
```

---

## 8. Signoff Checklist

Mark each item complete before proceeding to Phase 2:

- [ ] epic/research-review-5517 branch created from develop
- [ ] Branch protection rules configured
- [ ] GitHub labels created (workstream/*, priority/*, research-review)
- [ ] Governance documents committed to epic branch
- [ ] Initial commit pushed to epic/research-review-5517
- [ ] Branch protection rules verified in GitHub settings
- [ ] Labels verified in GitHub labels page
- [ ] Issue template created and accessible
- [ ] All verification steps passed
- [ ] This guide reviewed and walkthrough completed

**Signoff**:

```
Date: [2026-08-08]
Completed By: Copilot Coding Agent
Approved By: @makr-code (pending)
Status: ✅ Ready for Phase 2
```

---

## 9. Troubleshooting

### Branch Not Appearing in GitHub

**Problem**: Branch created locally but not showing on GitHub web UI

**Solution**:
```bash
# Verify push was successful
git push -u origin epic/research-review-5517

# Check if push was successful
git branch -r | grep research-review-5517
# Should show: origin/epic/research-review-5517

# If still not appearing, wait 30 seconds and refresh GitHub page
```

### Branch Protection Rules Not Applying

**Problem**: PR allows merge without status checks

**Solution**:
1. Go to https://github.com/makr-code/ThemisDB/settings/branches
2. Verify rule exists for `epic/research-review-5517`
3. Check "Require status checks to pass before merging"
4. Add relevant CI workflows (e.g., `build`, `test`)
5. Wait 1 minute and try again

### Labels Not Showing in New Issues

**Problem**: Labels created but don't appear in new issue form

**Solution**:
```bash
# Labels cache might need refresh
# Clear browser cache or use incognito/private mode
# Then navigate to:
# https://github.com/makr-code/ThemisDB/issues/new
```

---

## 10. Next Steps

After completing this branch setup verification:

1. **Phase 2**: Begin research issue audit and classification
   - Refer to ai_working/RESEARCH_BACKLOG_TRACKER.md
   - Start linking existing research-review issues to #5517

2. **Create Feature Branches**: As needed for research-review work
   - Follow naming: feature/5517-*, fix/5517-*, chore/5517-*
   - Create PRs → epic/research-review-5517
   - Never PR directly to develop

3. **Monitor**: Use this guide as reference throughout EPIC lifecycle

---

## Quick Reference Commands

```bash
# Checkout EPIC branch
git checkout epic/research-review-5517

# Update from remote
git pull origin epic/research-review-5517

# Create feature branch
git checkout -b feature/5517-<kurzname>

# Push feature branch
git push -u origin feature/5517-<kurzname>

# Create PR to EPIC (via GitHub web UI)
# Source: feature/5517-<kurzname>
# Target: epic/research-review-5517

# After PR merged, update local
git checkout epic/research-review-5517
git pull origin epic/research-review-5517
```

---

**Created**: 2026-08-08  
**Owner**: Copilot Coding Agent  
**Review Status**: Ready for @makr-code approval
