# GitHub Repository Settings Configuration Guide

## Overview

This document provides step-by-step instructions for repository administrators to configure GitHub settings to enforce the new squash merge strategy for ThemisDB.

## Required Manual Configuration

⚠️ **Important**: These settings CANNOT be configured through repository files. They must be set manually through the GitHub web interface by a repository administrator.

## Step-by-Step Instructions

### Step 1: Navigate to Repository Settings

1. Go to: `https://github.com/makr-code/ThemisDB/settings`
2. Make sure you have admin access to the repository
3. Scroll down to the **Pull Requests** section

### Step 2: Configure Merge Methods

In the **Merge button** section, configure the following:

#### Enable These Options:

- ✅ **Allow squash merging** ← **PRIMARY METHOD**
  - Check this box
  - This allows maintainers to squash all commits into one when merging

- ✅ **Allow merge commits**
  - Keep this checked
  - This is needed for release branches merging to main
  - Also needed for hotfix branches

#### Disable These Options:

- ❌ **Allow rebase merging** (OPTIONAL)
  - Uncheck this box
  - This prevents confusion and enforces the two-method approach
  - If you want to keep it enabled for flexibility, that's also acceptable

### Step 3: Set Default Merge Method

Below the merge method checkboxes:

1. Find the **Default to** dropdown (might say "Default to merge commit" currently)
2. Change it to: **Default to squash merging**
3. This makes "Squash and merge" the pre-selected option for all PRs

### Step 4: Configure Squash Merge Commit Messages

In the same **Pull Requests** section, find **Default commit message for squash merge**:

#### Recommended Settings:

**Default commit message:**
- Select: **Pull request title and description**
- OR: **Pull request title and commit details**

**Why?** This ensures the commit message includes:
- The PR title (which should follow conventional commit format)
- The PR description (which explains the changes in detail)
- Reference to the PR number (for traceability)

### Step 5: Verify Branch Protection Rules

Navigate to **Branches** in the left sidebar:

#### For `develop` branch:

If a protection rule exists, verify:
- ✅ Require pull request reviews before merging
- ✅ Require status checks to pass
- Branch is not set to "Require linear history" (this would prevent merge commits for releases)

#### For `main` branch:

If a protection rule exists, verify:
- ✅ Require pull request reviews before merging
- ✅ Require status checks to pass
- ✅ Include administrators (prevents accidental direct commits)
- Branch is not set to "Require linear history" (this would prevent merge commits)

### Step 6: Verify Settings

To confirm everything is configured correctly:

1. Create a test branch and PR
2. Check that "Squash and merge" is the default selected option
3. Verify that "Merge commit" option is also available
4. Check that the commit message preview shows the PR title and description

## Visual Guide

### Before Configuration:
```
Pull Requests Settings:
[ ] Allow merge commits          ← Likely checked
[ ] Allow squash merging         ← Needs to be checked
[ ] Allow rebase merging         ← Should be unchecked

Default to: [merge commit ▼]     ← Needs to be changed
```

### After Configuration:
```
Pull Requests Settings:
[✓] Allow merge commits          ← Keep for releases
[✓] Allow squash merging         ← Primary method
[ ] Allow rebase merging         ← Disabled

Default to: [squash merge ▼]     ← Changed!
```

## What Happens After Configuration

### For Contributors:

When creating a PR to `develop`:
1. GitHub will default to "Squash and merge" button
2. The squash commit message will be based on PR title/description
3. All individual commits in the PR will be combined into one

### For Maintainers:

When merging:
1. For feature/bugfix PRs → Use "Squash and merge" (default)
2. For release PRs → Switch to "Merge commit"
3. For hotfix PRs → Switch to "Merge commit"

## Verification

After completing the configuration:

### Test 1: Check Settings Page
```bash
# Visit and verify:
https://github.com/makr-code/ThemisDB/settings
# Under "Pull Requests" section:
# - Allow squash merging: ✅
# - Allow merge commits: ✅
# - Allow rebase merging: ❌ (or ✅ if you kept it)
# - Default to: squash merge
```

### Test 2: Check on a PR
1. Open any existing PR or create a test PR
2. Look at the merge button dropdown
3. Verify "Squash and merge" is selected by default
4. Verify "Merge commit" is also available in the dropdown

### Test 3: Check Commit Message Preview
1. On a PR page, click the "Squash and merge" button
2. Check the commit message preview
3. Should show: PR title + PR description
4. Should include: PR number reference

## Troubleshooting

### Issue: "Squash and merge" option not showing
**Solution**: Make sure "Allow squash merging" is checked in settings

### Issue: Cannot change default merge method
**Solution**: You need admin access to the repository

### Issue: Getting merge conflicts
**Solution**: This is unrelated to the merge method. Resolve conflicts in the feature branch first.

### Issue: Commit message is just the PR title
**Solution**: Change the "Default commit message" setting to include PR description

## Additional Resources

- [GitHub Docs: About merge methods](https://docs.github.com/en/repositories/configuring-branches-and-merges-in-your-repository/configuring-pull-request-merges/about-merge-methods-on-github)
- [GitHub Docs: Configuring commit squashing](https://docs.github.com/en/repositories/configuring-branches-and-merges-in-your-repository/configuring-pull-request-merges/configuring-commit-squashing-for-pull-requests)
- [ThemisDB Merge Strategy Migration Guide](MERGE_STRATEGY_MIGRATION.md)
- [ThemisDB Contributing Guide](../CONTRIBUTING.md)

## Summary Checklist

Use this checklist to ensure all settings are configured:

- [ ] Navigate to repository settings
- [ ] Enable "Allow squash merging"
- [ ] Keep "Allow merge commits" enabled
- [ ] (Optional) Disable "Allow rebase merging"
- [ ] Set default to "squash merging"
- [ ] Configure commit message format to include PR title and description
- [ ] Verify branch protection rules don't conflict
- [ ] Test on a PR to confirm settings work
- [ ] Communicate changes to team members

## Notes for Repository Administrators

- This configuration affects all future PRs immediately
- Existing open PRs will also see the new default
- Past merged PRs and commits are not affected
- You can still manually choose different merge methods per PR if needed
- The default just makes the recommended method the easy choice

---

**Document Version**: 1.0  
**Last Updated**: 2026-04-06  
**Requires**: GitHub Repository Admin Access
