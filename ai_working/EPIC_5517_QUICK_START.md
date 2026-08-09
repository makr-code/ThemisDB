# EPIC #5517 Quick Start Guide

**For**: Team members contributing to research-review consolidation  
**Duration**: 5-10 minutes to understand the structure  
**EPIC**: #5517 - Research Review Backlog 2026 (Consolidation & Prioritization)

---

## What Is This EPIC About?

We're consolidating all open research-review issues into a single, organized structure with:

1. **Unified hierarchy**: All research issues linked to parent EPIC #5517
2. **Clear phases**: Draft → Finalize → Transfer-to-Engineering
3. **Proper governance**: Branch model, PR flow, and status tracking

---

## Your Role

Depending on your involvement:

### If You're a Research Lead or Contributor

1. **Find your research issue** (or create one using template)
2. **Label it correctly**: Select workstream (`research-draft`, `research-finalize`, etc.) and priority (`P0`, `P1`, `P2`)
3. **Link to parent**: Add EPIC #5517 as parent issue
4. **Track progress**: Update status in comments as research evolves
5. **Transition when ready**: Move between workstreams by updating labels

### If You're a Module Owner or Tech Lead

1. **Review research issues** in your area
2. **Assess engineering applicability**: Will this research help your module?
3. **Provide feedback**: Comment on feasibility and next steps
4. **Approve transfers**: Sign off when ready for engineering handoff

### If You're Contributing Code/Docs to This EPIC

1. **Create feature branch**: `feature/5517-<kurzname>` or `fix/5517-<kurzname>`
2. **Make changes**: Update research documents, structure, status tracking
3. **Create PR to epic branch**: Target `epic/research-review-5517` (NOT develop)
4. **Reference EPIC #5517**: Include in PR description
5. **Use template**: Follow `.github/PULL_REQUEST_TEMPLATE/epic-branch-flow.md`

---

## Key Concepts

### Workstreams

| Phase | Label | Meaning | Next Step |
|---|---|---|---|
| **Draft** | `workstream/research-draft` | Actively exploring | Move to Finalize when findings are conclusive |
| **Finalize** | `workstream/research-finalize` | Findings validated, ready for review | Move to Transfer-to-Engineering if approved |
| **Transfer** | `workstream/transfer-to-engineering` | Approved for engineering implementation | Engineering team creates implementation EPIC |
| **Archived** | `workstream/archived` | Shelved or out-of-scope for 2026 | Can be revisited later if priorities change |

### Priority

- **P0**: Critical for 2026 roadmap
- **P1**: Important, should do in 2026
- **P2**: Nice-to-have, longer-term planning

### Issue Linking

**Parent**: #5517 (this EPIC)  
**Children**: Your research issues

Add parent relationship in GitHub issue sidebar → "Link issue" → Select "is child of" → Search #5517

---

## Important Files

| File | Purpose | Audience |
|---|---|---|
| `ai_working/EPIC_5517_RESEARCH_REVIEW_CONSOLIDATION.md` | Full governance and strategy | Project leads |
| `ai_working/RESEARCH_BACKLOG_TRACKER.md` | Issue inventory and progress tracking | Status reporters |
| `ai_working/EPIC_5517_BRANCH_SETUP_GUIDE.md` | Branch creation and verification | DevOps, maintainers |
| `.github/ISSUE_TEMPLATE/research-review-sub-issue.md` | Template for new issues | Anyone creating research issues |
| This file | Quick orientation | Everyone |

---

## Common Tasks

### Task 1: Link My Research Issue to EPIC #5517

1. Open your research issue
2. In right sidebar, find "Linked issues"
3. Click "Add linked issue"
4. Search for "5517"
5. Select "is child of" relationship
6. Save

**Result**: Your issue now shows as child of #5517 ✓

### Task 2: Add the Correct Workstream Label

1. Open your research issue
2. Click "Labels" in right sidebar
3. Select ONE of:
   - `workstream/research-draft` (actively researching)
   - `workstream/research-finalize` (findings ready)
   - `workstream/transfer-to-engineering` (approved for engineering)
   - `workstream/archived` (not prioritized)
4. Also add one priority: `priority/p0`, `priority/p1`, or `priority/p2`
5. Also add: `research-review`

**Result**: Your issue is now properly classified ✓

### Task 3: Create a PR for EPIC Work

```bash
# 1. Create feature branch
git checkout epic/research-review-5517
git pull origin epic/research-review-5517
git checkout -b feature/5517-my-research-docs

# 2. Make changes to research documents or governance files

# 3. Commit with clear message
git commit -m "feature(#5517): Add new research document on X

- Added research/NEW_TOPIC.md with findings
- Updated RESEARCH_BACKLOG_TRACKER.md to track new issue
- References #5517 parent EPIC"

# 4. Push to remote
git push -u origin feature/5517-my-research-docs

# 5. Create PR on GitHub
# - Source: feature/5517-my-research-docs
# - Target: epic/research-review-5517
# - Use template: epic-branch-flow.md
# - Reference: #5517 and any sub-issues
```

**Result**: PR ready for review into EPIC branch ✓

### Task 4: Move Issue Between Workstreams

1. Open your research issue
2. Find "Labels" in right sidebar
3. Remove old workstream label
4. Add new workstream label
5. Add comment explaining transition:

   ```
   Moving from [OLD] to [NEW] because:
   - [Reason 1]
   - [Reason 2]
   
   Transition completed on [date]. Ready for [next step].
   ```

**Result**: Issue now reflects new phase ✓

### Task 5: Report Status to Team

Update `ai_working/RESEARCH_BACKLOG_TRACKER.md`:

1. Count issues in each workstream
2. Update phase progress section
3. Add any new blockers or decisions
4. Create a branch and PR to epic/research-review-5517 with updates
5. Note that this is a `chore/5517-*` branch

**Result**: Team has current status ✓

---

## Branch Rules

### ✅ DO

```
feature/5517-my-research → PR to epic/research-review-5517
fix/5517-typo → PR to epic/research-review-5517
chore/5517-update-tracker → PR to epic/research-review-5517
```

### ❌ DON'T

```
feature/my-research → PR to develop (WRONG! Use epic branch)
feature/5517-xyz → PR to develop (WRONG! Use epic branch)
feature/5517-xyz → Direct commit to epic/research-review-5517 (WRONG! Use PR)
Direct commit to epic/research-review-5517 (WRONG! Use feature branch + PR)
```

---

## PR Checklist Template

When you create a PR to epic/research-review-5517:

```markdown
## EPIC Context
- EPIC issue: #5517
- Sub-issue(s): [e.g. #5400, #5401 or N/A]
- Flow type: [X] Feature branch → EPIC branch

## Branch Routing
- Source: feature/5517-<name>
- Target: epic/research-review-5517
- [X] Branch naming follows EPIC convention
- [X] No direct merge to develop

## What Changed
[Describe your changes]

## Research Status
- [ ] Related to which research issue?
- [X] No new issues created (just updates)

## Validation
- [X] Build passed (if applicable)
- [X] No unresolved findings
- [X] Documentation updated
```

---

## Frequently Asked Questions

**Q: My research issue isn't linked to #5517 yet. What do I do?**

A: Open your issue and use "Link issue" in sidebar to add parent #5517.

**Q: Which workstream label should I use?**

A: Pick the one where your research **currently is**:
- Just started? → `research-draft`
- Have findings? → `research-finalize`
- Ready for engineering? → `transfer-to-engineering`
- Out-of-scope? → `archived`

**Q: Can I PR directly to develop?**

A: ❌ **NO**. All EPIC work goes:
1. Feature branch → epic/research-review-5517 (via PR)
2. epic/research-review-5517 → develop (via consolidated PR)

**Q: Who approves PRs to epic/research-review-5517?**

A: EPIC owner (@makr-code) or designated reviewers. Not as strict as develop, but still require review.

**Q: What if I find a blocker?**

A: Add label `status/blocked` and comment with details. Tag @makr-code if urgent.

**Q: When do we merge epic/research-review-5517 to develop?**

A: After all research issues are processed and EPIC acceptance criteria met (see governance doc).

---

## Getting Help

| Issue | Resource |
|---|---|
| Understanding EPIC structure | Read: ai_working/EPIC_5517_RESEARCH_REVIEW_CONSOLIDATION.md |
| Setting up branch | Read: ai_working/EPIC_5517_BRANCH_SETUP_GUIDE.md |
| Tracking progress | Check: ai_working/RESEARCH_BACKLOG_TRACKER.md |
| Creating new issue | Use template: .github/ISSUE_TEMPLATE/research-review-sub-issue.md |
| EPIC GitHub issue | Visit: https://github.com/makr-code/ThemisDB/issues/5517 |
| Questions/Blockers | Comment on #5517 or tag @makr-code |

---

## Success Looks Like

✅ All research-review issues linked to #5517  
✅ Each issue has correct workstream + priority labels  
✅ Clear status tracking in comments and labels  
✅ Transitions documented as issues move between phases  
✅ Engineering EPICs created for transfer-ready research  
✅ All PRs use correct branch naming + routing  
✅ Final integration PR merged to develop  

---

## Next Steps

1. **Read this file** (you are here!) ✓
2. **Read governance doc**: ai_working/EPIC_5517_RESEARCH_REVIEW_CONSOLIDATION.md (5 min)
3. **Find/create your research issue** (5 min)
4. **Add labels + link to #5517** (2 min)
5. **Start contributing!** (ongoing)

---

**Questions?** Comment on EPIC issue #5517 or reach out to @makr-code

**Good luck!** 🚀
