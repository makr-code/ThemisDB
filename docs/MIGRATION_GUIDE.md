# Migration Guide: Transitioning to Git Flow Branching Strategy

## Overview

This guide helps ThemisDB contributors and maintainers transition from the current branching model to the new Git Flow strategy where:
- **`main`** = Production-ready release branch (protected)
- **`develop`** = Active development integration branch (protected)

## Timeline

- **Effective Date**: 2025-12-30
- **Grace Period**: 2 weeks for open PRs
- **Full Enforcement**: 2025-01-15

## For Repository Maintainers

### Phase 1: Preparation (Before Enforcement)

#### 1. Create `develop` Branch

```bash
# Clone repository
git clone https://github.com/makr-code/ThemisDB.git
cd ThemisDB

# Create develop branch from current main
git checkout main
git pull origin main
git checkout -b develop
git push origin develop

# Set develop as default branch on GitHub (optional during transition)
# GitHub Settings → Branches → Default branch → develop
```

#### 2. Configure Branch Protection

Follow the [BRANCH_PROTECTION_SETUP.md](ci-cd/branching-release-history/BRANCH_PROTECTION_SETUP.md) guide:

1. Protect `main` branch (strict rules)
2. Protect `develop` branch (moderate rules)
3. Set up CODEOWNERS
4. Configure required status checks

#### 3. Update CI/CD Workflows

Update GitHub Actions workflows to trigger on correct branches:

**Before:**
```yaml
on:
  push:
    branches: [main]
  pull_request:
    branches: [main]
```

**After:**
```yaml
on:
  push:
    branches: [develop, main]
  pull_request:
    branches: [develop]
```

**For Release Workflows:**
```yaml
on:
  push:
    tags: ['v*']
    branches: [main]  # Only main for releases
```

#### 4. Communicate Changes

- [ ] Post announcement in GitHub Discussions
- [ ] Update README.md with branching strategy link
- [ ] Send notification to all active contributors
- [ ] Update contribution guidelines
- [ ] Create pinned issue explaining the change

**Announcement Template:**

```markdown
## 📢 Important: New Branching Strategy

Starting 2025-12-30, ThemisDB is adopting a Git Flow branching strategy:

### Key Changes
- ✅ `develop` is now the default branch for PRs
- ✅ `main` is reserved for production releases
- ✅ All feature branches should target `develop`
- ✅ Hotfixes target `main` and merge back to `develop`

### What You Need to Do
1. Read [BRANCHING_STRATEGY.md](ci-cd/branching-release-history/BRANCHING_STRATEGY.md)
2. Rebase open PRs to target `develop`
3. Create new branches from `develop`

### Migration Guide
See [MIGRATION_GUIDE.md](MIGRATION_GUIDE.md) for detailed instructions.

Questions? Reply to this discussion or open an issue with label `question`.
```

### Phase 2: Migration (During Grace Period)

#### 1. Handle Existing PRs

For each open PR targeting `main`:

1. Contact PR author:
   ```markdown
   Hi @author,
   
   We're transitioning to a new branching strategy. Could you please rebase your PR to target `develop` instead of `main`?
   
   Instructions: [MIGRATION_GUIDE.md - For Contributors](#for-contributors-with-open-prs)
   
   Let me know if you need help!
   ```

2. Provide assistance if needed
3. Close stale PRs (> 6 months with no activity)

#### 2. Verify CI/CD Workflows

Test all workflows with both branches:

```bash
# Test develop branch workflow
git checkout develop
git commit --allow-empty -m "test: Trigger CI"
git push origin develop

# Verify workflows run correctly
# Check GitHub Actions tab
```

#### 3. Monitor and Adjust

- Watch for confusion or issues
- Provide quick support in Discussions
- Document common problems and solutions
- Update FAQ section in branching strategy

### Phase 3: Full Enforcement (After Grace Period)

#### 1. Enable Strict Branch Protection

Update `main` branch protection:
- ✅ Include administrators
- ✅ Require conversation resolution
- ✅ All status checks required
- ✅ No force pushes
- ✅ No deletions

#### 2. Set `develop` as Default Branch

1. Go to GitHub Settings → Branches
2. Change default branch from `main` to `develop`
3. This ensures new clones and PRs default to `develop`

#### 3. Update All Documentation

Verify these files reference the new strategy:
- [ ] README.md
- [ ] CONTRIBUTING.md
- [ ] GitHub issue templates
- [ ] PR templates
- [ ] Wiki pages (if any)

## For Contributors with Open PRs

If you have an open PR targeting `main`, you need to update it to target `develop`.

### Option 1: Change PR Target (No Rebase Needed)

**For simple cases where your branch is clean:**

1. Go to your PR on GitHub
2. Click **Edit** next to the base branch
3. Change from `main` to `develop`
4. Click **Update pull request**

### Option 2: Rebase onto develop (Recommended)

**For branches that need to incorporate latest changes:**

```bash
# 1. Update your local repository
git fetch origin

# 2. Switch to your feature branch
git checkout feature/your-feature

# 3. Rebase onto develop
git rebase origin/develop

# 4. Resolve any conflicts if they occur
# Edit conflicted files, then:
git add <resolved-files>
git rebase --continue

# 5. Force push (safe for your feature branch)
git push origin feature/your-feature --force-with-lease

# 6. Update PR target on GitHub (if not already done)
# Edit PR → Change base branch to 'develop'
```

### Option 3: Start Fresh (If Rebase is Too Complex)

**For very old branches with many conflicts:**

```bash
# 1. Create new branch from develop
git fetch origin
git checkout -b feature/your-feature-v2 origin/develop

# 2. Cherry-pick your commits
# Find commit SHAs from your old branch
git log feature/your-feature

# Cherry-pick each commit
git cherry-pick <commit-sha-1>
git cherry-pick <commit-sha-2>
# ... etc

# 3. Push new branch
git push origin feature/your-feature-v2

# 4. Close old PR and create new one targeting develop
```

## For New Contributors

**From now on, always:**

```bash
# 1. Clone repository
git clone https://github.com/makr-code/ThemisDB.git
cd ThemisDB

# 2. Checkout develop (will be default)
git checkout develop

# 3. Create feature branch
git checkout -b feature/my-new-feature

# 4. Make changes and commit
git add .
git commit -m "feat: Add new feature"

# 5. Push and create PR to develop
git push origin feature/my-new-feature
# On GitHub: Create PR with base = develop
```

## For Active Developers

### Update Local Development Workflow

```bash
# Add this to your ~/.gitconfig or repository .git/config
[alias]
    # Shortcut to update develop
    sync-develop = !git checkout develop && git pull origin develop
    
    # Shortcut to create feature branch
    new-feature = "!f() { git checkout develop && git pull && git checkout -b feature/$1; }; f"
    
    # Shortcut to create bugfix branch
    new-bugfix = "!f() { git checkout develop && git pull && git checkout -b bugfix/$1; }; f"

# Usage:
git new-feature my-feature-name
git new-bugfix critical-bug
```

### Update IDE/Editor Configuration

**VS Code `.vscode/settings.json`:**
```json
{
  "git.defaultBranchName": "develop",
  "git.branchPrefix": "feature/",
  "git.branchProtection": ["main", "develop"]
}
```

**JetBrains IDEs (IntelliJ, CLion, etc.):**
1. Settings → Version Control → Git
2. Protected branches: Add `main` and `develop`
3. Default branch: `develop`

## Common Scenarios

### Scenario 1: Starting New Feature

```bash
# OLD way (don't do this anymore)
git checkout main
git checkout -b feature/new-feature

# NEW way (correct)
git checkout develop
git pull origin develop
git checkout -b feature/new-feature
```

### Scenario 2: Fixing Production Bug

```bash
# Create hotfix from main
git checkout main
git pull origin main
git checkout -b hotfix/1.3.4-critical-bug

# Make fix
git commit -m "fix: Critical production bug"

# Create PR to main (with label 'hotfix')
git push origin hotfix/1.3.4-critical-bug

# After merging to main, also merge to develop:
git checkout develop
git pull origin develop
git cherry-pick <hotfix-commit-sha>
git push origin develop
```

### Scenario 3: Preparing Release

```bash
# Create release branch from develop
git checkout develop
git pull origin develop
git checkout -b release/1.4.0

# Update version
echo "1.4.0" > VERSION
./.github/workflows/04-release_create-release-archive.yml 1.4.0

# Commit and test
git commit -am "chore: Prepare release v1.4.0"
git push origin release/1.4.0

# After testing, merge to main
git checkout main
git merge --no-ff release/1.4.0
git tag -a v1.4.0 -m "Release v1.4.0"
git push origin main --tags

# Merge back to develop
git checkout develop
git merge --no-ff release/1.4.0
git push origin develop
```

## Troubleshooting

### Problem: PR Automatically Targets main

**Solution:**
GitHub defaults to the repository's default branch. Change PR target:
1. Click "Edit" next to base branch in PR
2. Select `develop`
3. Conflicts may need resolution after changing base

### Problem: Can't Push to develop

**Solution:**
Direct pushes to `develop` are restricted. Always use PRs:
```bash
# Create feature branch instead
git checkout -b feature/my-change develop
git push origin feature/my-change
# Then create PR on GitHub
```

### Problem: Merge Conflicts After Rebasing

**Solution:**
```bash
# Start rebase
git rebase origin/develop

# For each conflict:
# 1. Edit files to resolve conflicts
# 2. Stage resolved files
git add <file>

# 3. Continue rebase
git rebase --continue

# If too complex, abort and ask for help
git rebase --abort
```

### Problem: Accidentally Committed to develop Locally

**Solution:**
```bash
# Don't push! Fix locally first
git checkout develop

# Move commit to feature branch
git branch feature/accidental-commit
git reset --hard origin/develop

# Switch to feature branch
git checkout feature/accidental-commit
# Now create PR normally
```

## FAQ

### Q: Why are we changing the branching strategy?

**A:** The new Git Flow strategy provides:
- ✅ Clearer separation between stable (main) and development (develop) code
- ✅ Better protection for production releases
- ✅ Safer continuous integration workflow
- ✅ Industry-standard best practices
- ✅ Easier rollback and hotfix management

### Q: What happens to existing main commits?

**A:** Nothing changes with existing commits. The `main` branch history remains intact. We're just changing how new code flows in.

### Q: Can I still commit to main?

**A:** No, except through:
- Release branches (`release/*`)
- Hotfix branches (`hotfix/*`)
- Both require PR approval

### Q: What if I need urgent fix?

**A:** For production issues:
1. Create hotfix branch from `main`
2. Make fix
3. Create PR to `main` with `hotfix` label
4. Fast-track review (notify maintainers)
5. After merge, cherry-pick to `develop`

### Q: Do I need to recreate my fork?

**A:** No! Just update your fork:
```bash
# Add upstream if not already added
git remote add upstream https://github.com/makr-code/ThemisDB.git

# Fetch develop branch
git fetch upstream develop

# Create local develop branch
git checkout -b develop upstream/develop

# Push to your fork
git push origin develop
```

### Q: What about documentation-only changes?

**A:** Same workflow - target `develop`. Documentation is part of the codebase and should go through the same quality checks.

## Rollback Plan

If the migration causes significant issues, maintainers can temporarily:

1. Disable branch protection on `main`
2. Announce temporary reversion
3. Fix underlying issues
4. Re-attempt migration

However, **we don't expect to need this** with proper communication and support.

## Support

### Getting Help

- **GitHub Discussions**: https://github.com/makr-code/ThemisDB/discussions
- **Migration Issues**: Use label `branching-migration`
- **Urgent Help**: Mention `@makr-code` in discussions

### Resources

- [BRANCHING_STRATEGY.md](ci-cd/branching-release-history/BRANCHING_STRATEGY.md) - Complete strategy documentation
- [BRANCHING_STRATEGY_EN.md](ci-cd/branching-release-history/BRANCHING_STRATEGY_EN.md) - English version
- [BRANCH_PROTECTION_SETUP.md](ci-cd/branching-release-history/BRANCH_PROTECTION_SETUP.md) - Setup guide
- [CONTRIBUTING.md](CONTRIBUTING.md) - Updated contribution guidelines

## Checklist for Maintainers

- [ ] Create `develop` branch
- [ ] Configure branch protection for `main`
- [ ] Configure branch protection for `develop`
- [ ] Set up CODEOWNERS file
- [ ] Update CI/CD workflows
- [ ] Post migration announcement
- [ ] Update documentation
- [ ] Contact open PR authors
- [ ] Test workflows on both branches
- [ ] Monitor for issues (2 weeks)
- [ ] Set `develop` as default branch
- [ ] Enable strict enforcement
- [ ] Archive this migration guide

## Checklist for Contributors

- [ ] Read branching strategy documentation
- [ ] Update local development workflow
- [ ] Rebase open PRs to target `develop`
- [ ] Update IDE/editor configuration
- [ ] Test creating PR to `develop`
- [ ] Ask questions if anything is unclear

---

**Last Updated**: 2026-04-06  
**Version**: 1.0  
**Maintainer**: ThemisDB Core Team

**Questions?** Open an issue with label `branching-migration` or ask in Discussions.
