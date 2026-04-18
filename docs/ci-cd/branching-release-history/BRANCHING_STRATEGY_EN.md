# Git Branching Strategy for ThemisDB

## Overview

ThemisDB uses a modified **Git Flow** strategy where the `main` branch is always production-ready and serves as the release branch. All development work happens through the `develop` branch.

## Branch Structure

### 🎯 Main Branches

#### `main` Branch
- **Purpose**: Production-ready Release Branch
- **Protection**: ✅ Fully protected
- **Merges**: Only from `release/*` and `hotfix/*` branches
- **Status**: Every commit represents a production-ready version
- **Tags**: All release tags (e.g., `v1.4.0`) are created here
- **CI/CD**: Automatic deployment to production

**Rules:**
- ❌ Direct commits forbidden
- ❌ No feature branches merged directly
- ✅ Only via Pull Requests with code review
- ✅ All CI/CD checks must pass
- ✅ At least 1 maintainer approval required

#### `develop` Branch
- **Purpose**: Integration branch for ongoing development
- **Protection**: ✅ Protected
- **Merges**: From `feature/*`, `bugfix/*`, and `release/*` (after release)
- **Status**: Contains the latest completed features
- **CI/CD**: Automatic tests and validation

**Rules:**
- ❌ Avoid direct commits (Exception: Documentation)
- ✅ Feature branches are merged here
- ✅ Pull Requests with code review
- ✅ CI/CD checks must pass

### 🚀 Supporting Branches

#### `feature/*` Branches
- **Purpose**: Development of new features
- **Base**: `develop`
- **Merge Target**: `develop`
- **Lifetime**: Until feature is complete
- **Naming**: `feature/<issue-nr>-<description>` or `feature/<description>`

**Examples:**
```bash
feature/123-vector-search-optimization
feature/llm-streaming-support
feature/postgres-wire-protocol
```

**Workflow:**
```bash
# Create feature branch
git checkout develop
git pull origin develop
git checkout -b feature/123-vector-search

# Develop feature
git add .
git commit -m "feat(search): Implement vector search optimization"

# Complete feature
git push origin feature/123-vector-search
# Create Pull Request to develop
```

#### `bugfix/*` Branches
- **Purpose**: Bug fixes for develop branch
- **Base**: `develop`
- **Merge Target**: `develop`
- **Lifetime**: Until bug is fixed
- **Naming**: `bugfix/<issue-nr>-<description>` or `bugfix/<description>`

**Examples:**
```bash
bugfix/456-connection-pool-leak
bugfix/query-timeout-handling
```

#### `hotfix/*` Branches
- **Purpose**: Critical bugfixes for production
- **Base**: `main`
- **Merge Target**: `main` AND `develop`
- **Lifetime**: Immediately after fix
- **Naming**: `hotfix/<version>-<description>`

**Examples:**
```bash
hotfix/1.3.4-security-vulnerability
hotfix/1.3.4-critical-crash
```

**Workflow:**
```bash
# Create hotfix branch
git checkout main
git pull origin main
git checkout -b hotfix/1.3.4-security-fix

# Implement fix
git add .
git commit -m "fix(security): Patch critical vulnerability"

# Bump version
echo "1.3.4" > VERSION
git add VERSION
git commit -m "chore: Bump version to 1.3.4"

# Merge to main
git checkout main
git merge --no-ff hotfix/1.3.4-security-fix
git tag -a v1.3.4 -m "Release v1.3.4 - Security Fix"
git push origin main --tags

# Merge back to develop
git checkout develop
git merge --no-ff hotfix/1.3.4-security-fix
git push origin develop

# Delete hotfix branch
git branch -d hotfix/1.3.4-security-fix
git push origin --delete hotfix/1.3.4-security-fix
```

#### `release/*` Branches
- **Purpose**: Release preparation and stabilization
- **Base**: `develop`
- **Merge Target**: `main` AND `develop` (after release)
- **Lifetime**: Until release is complete
- **Naming**: `release/<version>`

**Examples:**
```bash
release/1.4.0
release/2.0.0-beta.1
```

**Workflow:**
```bash
# Create release branch
git checkout develop
git pull origin develop
git checkout -b release/1.4.0

# Prepare version and documentation
echo "1.4.0" > VERSION
./.github/workflows/04-release_create-release-archive.yml 1.4.0
git add VERSION CHANGELOG.md
git commit -m "chore: Prepare release v1.4.0"

# Bug fixes during release phase
git commit -m "fix(docs): Update release notes"

# Finalize release
git checkout main
git merge --no-ff release/1.4.0
git tag -a v1.4.0 -m "Release v1.4.0"
git push origin main --tags

# Merge back to develop
git checkout develop
git merge --no-ff release/1.4.0
git push origin develop

# Delete release branch
git branch -d release/1.4.0
git push origin --delete release/1.4.0
```

## Merge Strategy

> [!IMPORTANT]
> **ThemisDB uses different merge methods depending on the branch type:**

| Branch Type | Merge Method | Reason |
|------------|--------------|---------|
| **feature/** → develop | **Squash and merge** ✅ | Keeps develop history clean, one commit per feature |
| **bugfix/** → develop | **Squash and merge** ✅ | Keeps develop history clean, one commit per fix |
| **release/** → main | **Merge commit** | Preserves full release history and commit metadata |
| **hotfix/** → main | **Merge commit** | Preserves full hotfix history for audit purposes |

**Why squash merge for features/bugfixes?**
- ✅ Cleaner, more readable git history
- ✅ One logical commit per feature/fix
- ✅ Easier to revert if needed
- ✅ Better changelog generation
- ❌ Development commits (WIP, fix typo, etc.) stay in feature branch

**Configuring GitHub Repository Settings:**

Maintainers should configure the repository settings on GitHub to enforce this:
1. Go to Settings → General → Pull Requests
2. Enable "Allow squash merging" ✅
3. Enable "Allow merge commits" ✅ (needed for releases)
4. Disable "Allow rebase merging" ❌ (optional)
5. Set "Squash merging" as the default for the repository

## Branch Protection Rules

### `main` Branch Protection

**Required Settings (via GitHub):**

```yaml
# Example .github/branch-protection.yml
main:
  required_pull_request_reviews:
    required_approving_review_count: 1
    require_code_owner_reviews: true
    dismiss_stale_reviews: true
  required_status_checks:
    strict: true
    contexts:
      - "CI / Build & Test (ubuntu-latest)"
      - "CI / Build & Test (windows-latest)"
      - "Code Quality / clang-tidy"
      - "Code Quality / cppcheck"
      - "Security / Gitleaks"
  enforce_admins: true
  required_linear_history: false  # Allows merge commits
  allow_force_pushes: false
  allow_deletions: false
  restrictions:
    users: []
    teams: ["maintainers"]
```

**Manual Configuration:**
1. GitHub Repo Settings → Branches → Add rule
2. Branch name pattern: `main`
3. Enable:
   - ✅ Require a pull request before merging
   - ✅ Require approvals (1)
   - ✅ Require status checks to pass before merging
   - ✅ Require branches to be up to date before merging
   - ✅ Require conversation resolution before merging
   - ✅ Include administrators
   - ✅ Do not allow bypassing the above settings

### `develop` Branch Protection

```yaml
develop:
  required_pull_request_reviews:
    required_approving_review_count: 1
    dismiss_stale_reviews: false
  required_status_checks:
    strict: true
    contexts:
      - "CI / Build & Test (ubuntu-latest)"
      - "Code Quality / clang-tidy"
  enforce_admins: false
  allow_force_pushes: false
  allow_deletions: false
```

## Release Process

### 1. Feature Freeze (Start Release Cycle)

```bash
# Complete development
git checkout develop
git pull origin develop

# Create release branch
git checkout -b release/1.4.0

# Bump version
echo "1.4.0" > VERSION
./.github/workflows/04-release_create-release-archive.yml 1.4.0

git add VERSION CHANGELOG.md RELEASE_NOTES_v1.4.0.md
git commit -m "chore: Prepare release v1.4.0"
git push origin release/1.4.0
```

### 2. Release Testing & Stabilization

```bash
# Work on release branch
git checkout release/1.4.0

# Bug fixes (critical only!)
git commit -m "fix(docs): Correct installation instructions"
git commit -m "fix(build): Resolve build warning on Windows"

# Push updates
git push origin release/1.4.0
```

### 3. Release Finalization

```bash
# Final checks
cd build
ctest --output-on-failure
cd ..
./scripts/check-quality.sh

# Merge to main
git checkout main
git pull origin main
git merge --no-ff release/1.4.0 -m "Release v1.4.0"

# Create tag
git tag -a v1.4.0 -m "Release v1.4.0

Highlights:
- Feature A: Description
- Feature B: Description
- Performance improvements

See RELEASE_NOTES_v1.4.0.md for details."

# Push main and tags
git push origin main
git push origin v1.4.0

# Merge back to develop
git checkout develop
git pull origin develop
git merge --no-ff release/1.4.0 -m "Merge release v1.4.0 back to develop"
git push origin develop

# Delete release branch
git branch -d release/1.4.0
git push origin --delete release/1.4.0
```

### 4. Automatic Deployment

GitHub Actions will automatically trigger via tag `v1.4.0`:
- Build and push Docker images
- Generate release notes
- Create GitHub Release
- Deploy documentation

## Pull Request Workflows

### Feature Development → develop

```bash
# Develop feature
git checkout -b feature/new-awesome-feature develop
# ... commits ...
git push origin feature/new-awesome-feature
```

**Create PR:**
- **Base**: `develop`
- **Compare**: `feature/new-awesome-feature`
- **Title**: `feat(module): Add awesome feature`
- **Labels**: `enhancement`, `feature`
- **Reviewers**: Select team member

**PR Checklist:**
- [ ] Code follows style guidelines
- [ ] Tests added and passing
- [ ] Documentation updated
- [ ] No new warnings
- [ ] CI checks passing

### Hotfix → main + develop

```bash
# Create hotfix
git checkout -b hotfix/1.3.4-critical-bug main
# ... fix ...
git push origin hotfix/1.3.4-critical-bug
```

**Create PR to main:**
- **Base**: `main`
- **Compare**: `hotfix/1.3.4-critical-bug`
- **Title**: `hotfix: Fix critical production bug`
- **Labels**: `hotfix`, `critical`
- **Priority**: HIGH

**After merge to main:**
- Cherry-pick to develop or create new PR to develop

## Merge Strategies

### Feature → develop
- **Strategy**: Squash and Merge (preferred) or Merge Commit
- **Reason**: Clean history, feature as one unit

### Release → main
- **Strategy**: Merge Commit (--no-ff)
- **Reason**: Preserve release history

### Release → develop (back-merge)
- **Strategy**: Merge Commit (--no-ff)
- **Reason**: Adopt changes from release phase

### Hotfix → main
- **Strategy**: Merge Commit (--no-ff)
- **Reason**: Preserve hotfix history

## Best Practices

### ✅ DOs

1. **Always branch from develop** (except hotfixes)
   ```bash
   git checkout develop
   git pull origin develop
   git checkout -b feature/my-feature
   ```

2. **Regularly pull develop**
   ```bash
   git checkout feature/my-feature
   git pull origin develop
   # Resolve merge conflicts if necessary
   ```

3. **Meaningful Commit Messages**
   ```bash
   feat(storage): Add incremental backup support
   fix(query): Resolve off-by-one error in pagination
   docs(readme): Update installation instructions
   ```

4. **Delete branch after merge**
   ```bash
   git push origin --delete feature/my-feature
   ```

5. **Stay small and focused**
   - One feature = One branch
   - Commit regularly
   - Open Pull Request early (Draft PR for feedback)

### ❌ DON'Ts

1. ❌ **Don't push directly to main/develop**
2. ❌ **No long-lived feature branches** (> 2 weeks)
3. ❌ **No cherry-picking without reason**
4. ❌ **Don't merge main into feature branches**
5. ❌ **No force-pushes on shared branches**

## Migration Guide for Existing Contributors

### For Developers with Open PRs to main

1. **Change PR base:**
   ```bash
   # Update local branch
   git checkout your-feature-branch
   git fetch origin
   
   # Rebase onto develop
   git rebase origin/develop
   
   # Force push (only for feature branches!)
   git push origin your-feature-branch --force-with-lease
   ```

2. **Change PR target on GitHub:**
   - Open PR
   - Click "Edit" next to Base Branch
   - Change from `main` to `develop`

### For New Features

From now on:
```bash
# NEW: Branch from develop
git checkout develop
git pull origin develop
git checkout -b feature/my-feature

# OLD (no longer use):
# git checkout main
# git checkout -b feature/my-feature
```

## Versioning Schema

ThemisDB follows **Semantic Versioning 2.0.0**:

```
MAJOR.MINOR.PATCH[-PRERELEASE][+BUILD]

Examples:
1.4.0         - Standard Release
1.4.1         - Patch Release (Bugfix)
2.0.0         - Major Release (Breaking Changes)
2.0.0-beta.1  - Pre-Release
2.0.0+20231215 - Build Metadata
```

**Version Bump Rules:**
- **MAJOR**: Breaking Changes (API changes)
- **MINOR**: New Features (backward-compatible)
- **PATCH**: Bug Fixes (backward-compatible)

## CI/CD Integration

ThemisDB uses a comprehensive Git Flow CI/CD pipeline with dedicated workflows for each branch type.

### Automated Workflows

**Feature/Bugfix Development:**
- Workflow: `feature-ci.yml`
- Triggers: PRs to `develop` from `feature/*` or `bugfix/*`
- Validates: Branch strategy, build, tests, code quality, security

**Develop Branch:**
- Workflow: `develop-ci.yml`
- Triggers: Push to `develop`, PRs to `develop`
- Runs: Full CI suite, integration tests, creates artifacts

**Release Preparation:**
- Workflow: `release-ci.yml`
- Triggers: Push to `release/*`, PRs to `main` from `release/*`
- Validates: Version number, changelog, runs full test suite

**Hotfix:**
- Workflow: `hotfix-ci.yml`
- Triggers: PRs to `main` from `hotfix/*`
- Runs: Accelerated tests, auto-creates PR to sync to `develop`

**Production Deployment:**
- Workflow: `main-ci.yml`
- Triggers: Push to `main`, tags `v*`
- Deploys: Creates releases, publishes Docker images, deploys docs

**For complete workflow documentation, see [CI_CD_WORKFLOWS.md](../../CI_CD_WORKFLOWS.md)**

### Branch Protection

All workflows integrate with GitHub branch protection rules:
- `main`: Requires approval + CI checks (release-ci.yml or hotfix-ci.yml)
- `develop`: Requires approval + CI checks (develop-ci.yml, feature-ci.yml)

See [BRANCH_PROTECTION_SETUP.md](BRANCH_PROTECTION_SETUP.md) for configuration.

## Troubleshooting

### Problem: Feature branch is outdated

**Symptom**: Merge conflicts with develop

**Solution:**
```bash
git checkout feature/my-feature
git fetch origin
git rebase origin/develop

# Resolve conflicts
git add .
git rebase --continue

# Force push (safe for feature branch)
git push origin feature/my-feature --force-with-lease
```

### Problem: Accidental commit to develop

**Solution:**
```bash
# Undo last commit (locally)
git checkout develop
git reset --soft HEAD~1

# Create feature branch
git checkout -b feature/my-feature
git commit -m "feat: My feature"
git push origin feature/my-feature
```

### Problem: Hotfix needs to go to develop too

**Solution 1: Cherry-Pick**
```bash
git checkout develop
git cherry-pick <hotfix-commit-sha>
git push origin develop
```

**Solution 2: Merge**
```bash
git checkout develop
git merge --no-ff hotfix/1.3.4-fix
git push origin develop
```

## Additional Resources

- **Git Flow**: https://nvie.com/posts/a-successful-git-branching-model/
- **Semantic Versioning**: https://semver.org/
- **Conventional Commits**: https://www.conventionalcommits.org/
- **GitHub Flow**: https://guides.github.com/introduction/flow/

## FAQ

### When do I create a release/* branch?

When develop is stable and ready for production:
- All planned features for the version are merged
- Tests are green
- Documentation is up to date

### Can I develop multiple features simultaneously?

Yes! Create separate feature branches:
```bash
git checkout -b feature/feature-a develop
git checkout -b feature/feature-b develop
```

### What if my feature depends on another feature?

**Option 1**: Wait until the first feature is merged to develop

**Option 2**: Branch temporarily from the other feature branch:
```bash
git checkout feature/feature-a
git checkout -b feature/feature-b-depends-on-a

# Later: Rebase onto develop when feature-a is merged
git rebase origin/develop
```

### How long should a feature branch live?

**Recommendation**: Max. 1-2 weeks

**Why**: The longer the branch lives, the higher the probability of merge conflicts

**Tip**: Break large features into smaller tasks

## Contact

For questions about the branching strategy:
- GitHub Discussions: https://github.com/makr-code/ThemisDB/discussions
- GitHub Issues: Create an issue with label `question`

---

**Last Updated**: 2026-04-06  
**Version**: 1.0  
**Maintainer**: ThemisDB Core Team
