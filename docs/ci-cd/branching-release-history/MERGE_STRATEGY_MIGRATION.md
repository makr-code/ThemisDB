# Migration to Squash Merge Strategy

## Overview

This document explains the change from using merge commits to squash merges for feature and bugfix branches, and provides guidance on how to configure and enforce this going forward.

## Problem Statement

In the past, ThemisDB has been using regular merge commits for all types of pull requests, including feature and bugfix branches merging to `develop`. This has led to a cluttered git history with many intermediate commits (WIP commits, "fix typo", etc.) that don't add value to the main branch history.

## Solution

Going forward, ThemisDB will use **squash merges** for feature and bugfix branches, while maintaining merge commits for release and hotfix branches.

## Merge Strategy Table

| Branch Type | Merge Method | Reason |
|------------|--------------|---------|
| `feature/*` → `develop` | **Squash and merge** ✅ | Keeps develop history clean, one commit per feature |
| `bugfix/*` → `develop` | **Squash and merge** ✅ | Keeps develop history clean, one commit per fix |
| `release/*` → `main` | **Merge commit** | Preserves full release history and commit metadata |
| `hotfix/*` → `main` | **Merge commit** | Preserves full hotfix history for audit purposes |
| `main` → `develop` (after release) | **Merge commit** | Preserves release merge structure |
| `develop` → `release/*` | **Branch creation** | N/A - this is a branch point, not a merge |

## Benefits of Squash Merging

### ✅ Advantages

1. **Cleaner History**: Each feature appears as a single, coherent commit in the main branch
2. **Better Readability**: `git log` on develop shows logical changes, not work-in-progress commits
3. **Easier Reverts**: Reverting a feature is a single operation
4. **Better Changelogs**: Automatic changelog generation produces cleaner output
5. **Reduced Noise**: No more "fix typo", "WIP", "addressing review comments" commits in main history

### ❌ What You Lose

1. **Detailed Development History**: Individual development commits are not preserved in develop
   - *Mitigation*: Full history is still available in the feature branch and PR
2. **Individual Commit Signatures**: Only the merge commit is signed
   - *Mitigation*: PR metadata preserves author information

## Can We Change History Retroactively?

### Short Answer: **No, not recommended**

Rewriting git history to change past merge commits to squash merges is:
- ❌ **Dangerous**: Would require force-pushing to main branches
- ❌ **Disruptive**: All contributors would need to re-clone
- ❌ **Not Worth It**: Past history is already there; focus on future commits

### What We CAN Do

✅ **Going Forward**: Configure GitHub to use squash merge as default
✅ **Documentation**: Update all docs to reflect new policy
✅ **Team Training**: Educate maintainers on when to use which merge method
✅ **Automation**: Consider adding PR checks that suggest squash merge for feature branches

## Implementation Steps

### Step 1: Configure GitHub Repository Settings

**For Repository Administrators:**

1. Navigate to: `https://github.com/makr-code/ThemisDB/settings`
2. Go to section: **Pull Requests**
3. Configure merge methods:
   - ✅ **Allow squash merging** - Enable
   - ✅ **Allow merge commits** - Keep enabled (needed for releases)
   - ❌ **Allow rebase merging** - Disable (optional, to avoid confusion)
4. Set **default to squash merge** for the repository
5. Configure squash merge commit message format:
   - Title: Use pull request title
   - Description: Use pull request description

### Step 2: Update Branch Protection Rules

Ensure that branch protection rules don't conflict with the new merge strategy:

**For `develop` branch:**
```yaml
develop:
  required_pull_request_reviews:
    required_approving_review_count: 1
  required_status_checks:
    strict: true
    contexts:
      - "CI / Build & Test (ubuntu-latest)"
  enforce_admins: false
  allow_force_pushes: false
```

**For `main` branch:**
```yaml
main:
  required_pull_request_reviews:
    required_approving_review_count: 1
    require_code_owner_reviews: true
  required_status_checks:
    strict: true
    contexts:
      - "CI / Build & Test (ubuntu-latest)"
      - "CI / Build & Test (windows-latest)"
      - "CI / Build & Test (macos-latest)"
  enforce_admins: true
  allow_force_pushes: false
```

### Step 3: Update Documentation

All relevant documentation has been updated:

- ✅ `CONTRIBUTING.md` - Added merge strategy guidelines
- ✅ `.github/pull_request_template.md` - Updated merge strategy checklist
- ✅ `docs/BRANCHING_STRATEGY.md` - Added merge strategy section (German)
- ✅ `docs/BRANCHING_STRATEGY_EN.md` - Added merge strategy section (English)
- ✅ `docs/BRANCHING_VISUAL_GUIDE.md` - Updated visual guide
- ✅ `docs/MERGE_STRATEGY_MIGRATION.md` - This document

### Step 4: Educate Team Members

**For Contributors:**
- When creating a PR for feature/bugfix, expect it to be squash merged
- Write a clear PR title and description (they become the commit message)
- Focus on making your final PR state clean and reviewable
- Don't worry about commit history in feature branches - it won't appear in develop

**For Maintainers:**
- Use "Squash and merge" for feature and bugfix PRs
- Use "Merge commit" for release and hotfix PRs
- Edit squash commit message to be clear and descriptive
- Include PR number in commit message for traceability

## Best Practices Going Forward

### For Feature/Bugfix PRs

1. **Write a Good PR Title**: It will become the commit message
   - ✅ `feat(storage): Add vector search optimization`
   - ❌ `Update files`

2. **Write a Good PR Description**: It will become the commit body
   - Explain what changed
   - Explain why it changed
   - Reference related issues

3. **Keep PRs Focused**: One feature/fix per PR makes squash merges cleaner

4. **Don't Worry About Commit History in Branch**: Your WIP commits won't be merged

### For Release PRs

1. **Use Merge Commit**: Preserve the release branch structure
2. **Tag After Merge**: Tag the merge commit on main
3. **Merge Back to Develop**: Use merge commit to keep branches in sync

## Verification

To verify the merge strategy is working:

```bash
# Check recent commits on develop - should see squash commits for features
git log develop --oneline --graph -20

# Check recent commits on main - should see merge commits for releases
git log main --oneline --graph -20

# Verify a specific PR was squash merged (look for single commit)
git log develop --all --grep="PR #123"
```

## FAQ

### Q: What happens to my existing feature branch?
**A:** Nothing changes. When you merge it, it will be squash merged going forward.

### Q: Can I still see the full development history?
**A:** Yes! The full history is preserved in:
- The original feature branch (until deleted)
- The GitHub PR page
- Any local clones that have the feature branch

### Q: What if I'm in the middle of a long-running feature?
**A:** No problem. Continue working as normal. When you merge, it will be squash merged.

### Q: Should I change my workflow?
**A:** Not really. The only change is being more careful about PR titles/descriptions since they become the commit message.

### Q: What about merge conflicts?
**A:** Handle them the same way. Merge or rebase develop into your feature branch before creating/updating the PR.

### Q: Can I request a merge commit instead of squash merge?
**A:** Only for release and hotfix branches. For features/bugfixes, squash merge is the standard.

## References

- [CONTRIBUTING.md](../../../CONTRIBUTING.md) - Contribution guidelines
- [BRANCHING_STRATEGY.md](BRANCHING_STRATEGY.md) - Git Flow branching strategy (German)
- [BRANCHING_STRATEGY_EN.md](BRANCHING_STRATEGY_EN.md) - Git Flow branching strategy (English)
- [GitHub Docs: About merge methods](https://docs.github.com/en/repositories/configuring-branches-and-merges-in-your-repository/configuring-pull-request-merges/about-merge-methods-on-github)

## Conclusion

The migration to squash merge for feature and bugfix branches will improve the readability and maintainability of the ThemisDB git history going forward. While we cannot change past history, all new PRs will benefit from this cleaner approach.

---

**Document Version**: 1.0  
**Last Updated**: 2026-04-06  
**Status**: Active
