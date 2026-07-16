# Migration Guide: Transitioning to Canonical Edition Branching

## Overview

This guide helps ThemisDB contributors and maintainers transition from the historical branch model to the canonical edition-based branch strategy where:

- **`develop`** = Active integration branch (protected)
- **`community`** = Community release branch (protected)
- **`minimal`** = Minimal release branch (protected)
- **`enterprise`** = Enterprise release branch (protected)
- **`hyperscaler`** = Hyperscaler release branch (protected)
- **`military`** = Military release branch (protected)

Legacy names:

- **`main`** = historical Community release branch; replaced by `community`
- **`millitary`** = historical misspelling; replaced by `military`

## Timeline

- **Effective Date**: 2026-06-15
- **Grace Period**: 2 weeks for open PRs and documentation/workflow cleanup
- **Full Enforcement**: after repository settings, protections, and references are aligned

## For Repository Maintainers

### Phase 1: Preparation (Before Enforcement)

#### 1. Create or confirm canonical release branches

```bash
# Clone repository
git clone https://github.com/makr-code/ThemisDB.git
cd ThemisDB

# Confirm develop exists and is current
git checkout develop
git pull origin develop

# Create community from historical main if needed
git checkout main
git pull origin main
git checkout -b community
git push origin community

# Create military from historical millitary if needed
# Only if the canonical military branch does not already exist
git checkout millitary
git pull origin millitary
git checkout -b military
git push origin military
```

#### 2. Configure Branch Protection

Follow the canonical model from `BRANCHING_STRATEGY.md`:

1. Protect `community` branch (strict release rules)
2. Protect `develop` branch (integration rules)
3. Protect `minimal`, `enterprise`, `hyperscaler`, and `military` release branches
4. Set up CODEOWNERS
5. Configure required status checks

#### 3. Update CI/CD Workflows

Update GitHub Actions workflows to trigger on canonical branches.

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
    branches: [develop, community]
  pull_request:
    branches: [develop]
```

**For Community Release Workflows:**
```yaml
on:
  push:
    tags: ['v*']
    branches: [community]
```

#### 4. Communicate Changes

- [ ] Post announcement in GitHub Discussions
- [ ] Update README.md with branching strategy link
- [ ] Send notification to all active contributors
- [ ] Update contribution guidelines
- [ ] Create pinned issue explaining the change

**Announcement Template:**

```markdown
## 📢 Important: Canonical Edition Branching Strategy

Starting 2026-06-15, ThemisDB is adopting a canonical edition-based branch strategy.

### Key Changes
- ✅ `develop` remains the default branch for PRs
- ✅ `community` replaces `main` as the Community release branch
- ✅ `military` replaces historical `millitary`
- ✅ All feature branches should target `develop`
- ✅ Edition release work targets the matching edition branch

### What You Need to Do
1. Read [BRANCHING_STRATEGY.md](../BRANCHING_STRATEGY.md)
2. Rebase open feature PRs to target `develop`
3. Retarget Community release PRs from `main` to `community`
4. Stop using `millitary` for new work

### Migration Guide
See [MIGRATION_GUIDE.md](MIGRATION_GUIDE.md) for detailed instructions.

Questions? Reply to this discussion or open an issue with label `question`.
```

### Phase 2: Migration (During Grace Period)

#### 1. Handle Existing PRs

For each open PR targeting a legacy branch:

- `main` → decide whether it should target `develop` or `community`
- `millitary` → retarget to `military`

Example message:

```markdown
Hi @author,

We're transitioning to the canonical edition-based branching strategy.
Could you please update your PR target away from the legacy branch?

- Feature / bugfix work should target `develop`
- Community release work should target `community`
- Military release work should target `military`

Instructions: [MIGRATION_GUIDE.md](#for-contributors-with-open-prs)

Let me know if you need help!
```

#### 2. Verify CI/CD Workflows

Test workflows on canonical branches:

```bash
# Test develop branch workflow
git checkout develop
git commit --allow-empty -m "test: Trigger CI"
git push origin develop

# Optionally test community workflow
git checkout community
git commit --allow-empty -m "test: Trigger community CI"
git push origin community
```

#### 3. Monitor and Adjust

- Watch for confusion or issues
- Provide quick support in Discussions
- Document common problems and solutions
- Update FAQ sections in branching documents

### Phase 3: Full Enforcement

#### 1. Enable strict branch protection on canonical release lanes

Update `community`, `minimal`, `enterprise`, `hyperscaler`, and `military` branch protections:
- ✅ Include administrators
- ✅ Require conversation resolution
- ✅ All required status checks
- ✅ No force pushes
- ✅ No deletions

#### 2. Set `develop` as Default Branch

1. Go to GitHub Settings → Branches
2. Change default branch from legacy `main` to `develop` if not already set
3. This ensures new clones and PRs default to `develop`

#### 3. Freeze legacy branches

After migration:
- mark `main` as legacy-only
- mark `millitary` as legacy-only
- disable new PRs against them where possible
- retain only as temporary migration references until fully retired

## For Contributors with Open PRs

If you have an open PR targeting a legacy branch, update it as follows:

- Feature/bugfix work: retarget to `develop`
- Community release work: retarget to `community`
- Military release work: retarget to `military`

### Option 1: Change PR Target

1. Go to your PR on GitHub
2. Click **Edit** next to the base branch
3. Change from the legacy target to the canonical target
4. Click **Update pull request**

### Option 2: Rebase onto develop (Recommended for feature work)

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
```

## For New Contributors

**From now on, always:**

```bash
# 1. Clone repository
git clone https://github.com/makr-code/ThemisDB.git
cd ThemisDB

# 2. Checkout develop
git checkout develop

# 3. Create feature branch
git checkout -b feature/my-new-feature

# 4. Make changes and commit
git add .
git commit -m "feat: Add new feature"

# 5. Push and create PR to develop
git push origin feature/my-new-feature
```

## Common Scenarios

### Scenario 1: Starting New Feature

```bash
# Correct
git checkout develop
git pull origin develop
git checkout -b feature/new-feature
```

### Scenario 2: Fixing Community Production Bug

```bash
# Create hotfix from community
git checkout community
git pull origin community
git checkout -b hotfix/community/1.3.4-critical-bug

# Make fix
git commit -m "fix: Critical production bug"

# Create PR to community
git push origin hotfix/community/1.3.4-critical-bug

# After merging to community, also merge or cherry-pick to develop
```

### Scenario 3: Preparing Community Release

```bash
# Create release branch from develop
git checkout develop
git pull origin develop
git checkout -b release/community/v1.4.0

# Update version and notes
echo "1.4.0" > VERSION

# Commit and test
git commit -am "chore: Prepare community release v1.4.0"
git push origin release/community/v1.4.0

# After testing, merge to community
git checkout community
git merge --no-ff release/community/v1.4.0
git tag -a v1.4.0 -m "Release v1.4.0"
git push origin community --tags
```

## Troubleshooting

### Problem: PR Automatically Targets main

**Solution:**
GitHub may still default to a legacy branch until settings are updated.
Change PR target manually:
1. Click "Edit" next to base branch in PR
2. Select `develop` for feature work or `community` for Community release work
3. Resolve conflicts if needed after changing base

### Problem: Need to use military but old docs mention millitary

**Solution:**
Use `military` for all new work.
Treat `millitary` as historical-only during migration cleanup.

## FAQ

### Q: Why are we changing the branching strategy?

**A:** The new canonical edition strategy provides:
- ✅ Clearer mapping between product editions and release lanes
- ✅ Less ambiguity than using `main` for Community edition
- ✅ Safer release promotion and hotfix handling
- ✅ Better automation and AI-agent alignment
- ✅ Cleaner long-term governance for multiple editions

### Q: What happens to existing main commits?

**A:** Nothing changes in history. `main` remains a historical branch reference during migration, but new Community release work should move to `community`.

### Q: Can I still target main?

**A:** No for new work. Use:
- `develop` for normal implementation
- `community` for Community release work
- `military` for Military release work
- matching edition lanes for edition-specific release work

## Support

### Getting Help

- **GitHub Discussions**: https://github.com/makr-code/ThemisDB/discussions
- **Migration Issues**: Use label `branching-migration`
- **Urgent Help**: Mention `@makr-code` in discussions

### Resources

- [../BRANCHING_STRATEGY.md](../BRANCHING_STRATEGY.md) - Canonical root strategy
- [BRANCHING_STRATEGY_EN.md](BRANCHING_STRATEGY_EN.md) - Historical detailed English guide (needs ongoing alignment)
- [../RELEASE_STRATEGY.md](../RELEASE_STRATEGY.md) - Release flow
- [../CONTRIBUTING.md](../CONTRIBUTING.md) - Contribution guidelines

## Checklist for Maintainers

- [ ] Confirm canonical release branches exist
- [ ] Configure branch protection for `community`
- [ ] Configure branch protection for `develop`
- [ ] Configure branch protection for `minimal`, `enterprise`, `hyperscaler`, `military`
- [ ] Set up CODEOWNERS file
- [ ] Update CI/CD workflows
- [ ] Post migration announcement
- [ ] Update documentation
- [ ] Contact open PR authors
- [ ] Test workflows on canonical branches
- [ ] Set `develop` as default branch
- [ ] Freeze legacy `main` / `millitary` usage

## Checklist for Contributors

- [ ] Read branching strategy documentation
- [ ] Update local development workflow
- [ ] Rebase open feature PRs to target `develop`
- [ ] Retarget Community release PRs to `community` if applicable
- [ ] Stop using `millitary` for new work
- [ ] Ask questions if anything is unclear

---

**Last Updated**: 2026-06-15  
**Version**: 2.0  
**Maintainer**: ThemisDB Core Team

**Questions?** Open an issue with label `branching-migration` or ask in Discussions.
