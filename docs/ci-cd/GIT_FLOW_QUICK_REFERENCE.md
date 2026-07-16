# Git Flow CI/CD Quick Reference

## Workflow Trigger Matrix

| Branch Type | Target Branch | Event | Workflow(s) Triggered | Purpose |
|-------------|---------------|-------|----------------------|---------|
| `feature/*` | `develop` | PR opened/updated | `feature-ci.yml` | Validate feature development |
| `bugfix/*` | `develop` | PR opened/updated | `feature-ci.yml` | Validate bug fixes |
| Any branch | `develop` | PR opened/updated | `develop-ci.yml` | Integration validation |
| - | `develop` | Push | `develop-ci.yml` | Post-merge validation |
| `release/*` | - | Push | `release-ci.yml` | Release branch validation |
| `release/*` | `main` | PR opened/updated | `release-ci.yml` | Pre-release validation |
| `hotfix/*` | - | Push | `hotfix-ci.yml` | Hotfix branch validation |
| `hotfix/*` | `main` | PR opened/updated | `hotfix-ci.yml` | Pre-hotfix validation |
| - | `main` | Push | `main-ci.yml` | Production deployment |
| - | - | Tag `v*` | `main-ci.yml` | Release creation |
| Any | `main` or `develop` | PR | `ci.yml` | General CI check |

## Branch Strategy Rules

### ✅ Valid PR Targets

- `feature/*` → `develop`
- `bugfix/*` → `develop`
- `release/*` → `main` (for release)
- `release/*` → `develop` (for back-merge after release)
- `hotfix/*` → `main` (for immediate fix)

### ❌ Invalid PR Targets

- `feature/*` → `main` ❌ (Use `develop` instead)
- `bugfix/*` → `main` ❌ (Use `develop` instead)
- `hotfix/*` → `develop` ❌ (Target `main` first, then auto-sync)

## Common Workflows by Scenario

### Scenario 1: New Feature Development

```bash
git checkout develop
git checkout -b feature/awesome-feature
# ... develop ...
git push origin feature/awesome-feature
```

**Create PR**: `feature/awesome-feature` → `develop`

**Workflows Triggered**:
1. `feature-ci.yml` - Validates feature
2. `develop-ci.yml` - Validates integration

**On Merge to develop**:
- `develop-ci.yml` runs on push

---

### Scenario 2: Release Preparation

```bash
git checkout develop
git checkout -b release/1.4.0
echo "1.4.0" > VERSION
git commit -am "chore: Prepare release v1.4.0"
git push origin release/1.4.0
```

**On Push**:
- `release-ci.yml` runs to validate release branch

**Create PR**: `release/1.4.0` → `main`

**Workflows Triggered**:
- `release-ci.yml` - Full validation

**On Merge to main**:
- `main-ci.yml` runs
- Create tag `v1.4.0`
- `main-ci.yml` runs again for release creation

**After Release**:
Create PR: `release/1.4.0` → `develop` (back-merge)
- `develop-ci.yml` validates the back-merge

---

### Scenario 3: Hotfix

```bash
git checkout main
git checkout -b hotfix/1.3.5-critical
# ... fix ...
echo "1.3.5" > VERSION
git commit -am "fix: Critical fix"
git push origin hotfix/1.3.5-critical
```

**On Push**:
- `hotfix-ci.yml` runs

**Create PR**: `hotfix/1.3.5-critical` → `main`

**Workflows Triggered**:
- `hotfix-ci.yml` - Fast validation

**On Merge to main**:
- `main-ci.yml` runs for deployment
- `hotfix-ci.yml` auto-creates PR to `develop`

**Auto-Created PR**: `hotfix/1.3.5-critical` → `develop`
- Review and merge manually
- `develop-ci.yml` validates the sync

---

### Scenario 4: Direct Commit to Develop (Docs)

```bash
git checkout develop
# ... edit docs ...
git commit -am "docs: Update README"
git push origin develop
```

**Workflows Triggered**:
- `develop-ci.yml` - Validates changes

---

## Validation Points

### Branch Validation

All workflows validate branch strategy:

```bash
# feature/* or bugfix/* → develop ✅
# release/* → main ✅
# hotfix/* → main ✅
```

### Version Validation

Release and hotfix workflows validate VERSION file:

```bash
# Must exist
# Must follow semver: X.Y.Z or X.Y.Z-prerelease
```

### Required Files

- `VERSION` - Always required
- `CHANGELOG.md` - Recommended
- `RELEASE_NOTES_vX.Y.Z.md` - Recommended for releases

## Workflow Status Checks

### For PRs to `main`:
- Release CI / validate-release (for `release/*`)
- Release CI / build-and-test
- OR Hotfix CI / validate-hotfix (for `hotfix/*`)
- OR Hotfix CI / build-and-critical-tests

### For PRs to `develop`:
- Feature/Bugfix CI / validate-branch
- Feature/Bugfix CI / build-and-test
- Develop CI / validate
- Develop CI / build-linux

## Artifact Retention

| Workflow | Artifact Type | Retention |
|----------|--------------|-----------|
| Feature CI | Code quality reports | 7 days |
| Develop CI | Build artifacts | 7 days |
| Release CI | Release packages | 30 days |
| Hotfix CI | Hotfix packages | 30 days |
| Main CI | Production artifacts | 90 days |

## Troubleshooting

### "Branch validation failed"
**Problem**: Feature branch targeting wrong base

**Solution**: Change PR target from `main` to `develop`

### "Version validation failed"
**Problem**: VERSION file missing or invalid format

**Solution**: 
```bash
echo "1.4.0" > VERSION
git add VERSION
git commit -m "fix: Update version"
```

### "Tests failed"
**Problem**: Build or test failures

**Solution**: Check workflow logs, fix issues, push updates

### "Hotfix sync PR not created"
**Problem**: Auto-sync didn't trigger

**Solution**: 
```bash
# Manually create PR
gh pr create --base develop --head hotfix/1.3.5-critical
```

## Manual Workflow Triggers

Some workflows support manual triggering:

```bash
# Trigger via GitHub UI:
# Actions → Select workflow → Run workflow

# Or via GitHub CLI:
gh workflow run release-ci.yml
gh workflow run main-ci.yml
```

## Further Reading

- [CI_CD_WORKFLOWS.md](CI_CD_WORKFLOWS.md) - Complete workflow documentation
- [BRANCHING_STRATEGY.md](ci-cd/branching-release-history/BRANCHING_STRATEGY.md) - Git Flow strategy
- [BRANCH_PROTECTION_SETUP.md](ci-cd/branching-release-history/BRANCH_PROTECTION_SETUP.md) - Protection setup
- [.github/workflows/README.md](.github/workflows/README.md) - Workflow directory guide

---

**Last Updated**: 2026-04-06  
**Version**: 1.0  
**Maintained by**: ThemisDB Core Team
