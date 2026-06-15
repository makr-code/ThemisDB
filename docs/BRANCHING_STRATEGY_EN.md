# Git Branching Strategy for ThemisDB

## Overview

ThemisDB now uses a canonical edition-based branching strategy centered on a protected `develop` integration branch and explicit release branches per edition. The historical `main` branch is no longer the canonical release branch name for Community; it is replaced by `community`.

## Branch Structure

### 🎯 Main Branches

#### `community` Branch
- **Purpose**: Production-ready Community release branch
- **Protection**: ✅ Fully protected
- **Merges**: Only from `release/*`, `hotfix/*`, or explicitly approved release-maintenance work
- **Status**: Represents the Community release lane
- **Tags**: Community tags (e.g., `v1.4.0`) are created here
- **CI/CD**: Community release workflows and production packaging

**Rules:**
- ❌ Direct commits forbidden
- ❌ No feature branches merged directly
- ✅ Only via Pull Requests with code review
- ✅ All required checks must pass
- ✅ At least 1 maintainer approval required

#### `develop` Branch
- **Purpose**: Integration branch for ongoing development
- **Protection**: ✅ Protected
- **Merges**: From `feature/*`, `bugfix/*`, and back-merges where applicable
- **Status**: Contains the latest completed development work
- **CI/CD**: Automatic tests and validation

**Rules:**
- ❌ Avoid direct commits
- ✅ Feature and bugfix branches are merged here
- ✅ Pull Requests with code review
- ✅ Required checks must pass

### 🎯 Additional Canonical Release Branches

#### `minimal` Branch
- **Purpose**: Production-ready Minimal release branch
- **Protection**: ✅ Protected
- **Tags**: `minimal-vX.Y.Z`

#### `enterprise` Branch
- **Purpose**: Production-ready Enterprise release branch
- **Protection**: ✅ Protected
- **Tags**: `enterprise-vX.Y.Z`

#### `hyperscaler` Branch
- **Purpose**: Production-ready Hyperscaler release branch
- **Protection**: ✅ Protected
- **Tags**: `hyperscaler-vX.Y.Z`

#### `military` Branch
- **Purpose**: Production-ready Military release branch
- **Protection**: ✅ Protected
- **Tags**: `military-vX.Y.Z`

### ⚠️ Legacy Branch Names

#### `main`
- **Status**: Legacy only
- **Historical Role**: Old Community release branch name
- **Replacement**: `community`

#### `millitary`
- **Status**: Legacy only
- **Historical Role**: Misspelled Military branch name
- **Replacement**: `military`

New work must not target these legacy names.

### 🚀 Supporting Branches

#### `feature/*` Branches
- **Purpose**: Development of new features
- **Base**: `develop`
- **Merge Target**: `develop`
- **Lifetime**: Until feature is complete
- **Naming**: `feature/<issue-nr>-<description>` or `feature/<description>`

#### `bugfix/*` Branches
- **Purpose**: Bug fixes for `develop`
- **Base**: `develop`
- **Merge Target**: `develop`
- **Lifetime**: Until bug is fixed
- **Naming**: `bugfix/<issue-nr>-<description>` or `bugfix/<description>`

#### `hotfix/*` Branches
- **Purpose**: Critical fixes for a specific release lane
- **Base**: affected edition release branch
- **Merge Target**: affected edition release branch, then back-merge or cherry-pick to `develop`
- **Naming**: `hotfix/<edition>/<version>-<description>`

**Example:**
```bash
hotfix/community/1.3.4-security-vulnerability
hotfix/military/1.3.4-critical-crash
```

#### `release/*` Branches
- **Purpose**: Release preparation and stabilization
- **Base**: `develop`
- **Merge Target**: the matching edition release branch
- **Lifetime**: Until release is complete
- **Naming**: `release/<edition>/v<version>`

**Examples:**
```bash
release/community/v1.4.0
release/enterprise/v2.0.0-rc1
release/military/v1.8.0
```

## Merge Strategy

> [!IMPORTANT]
> ThemisDB uses different merge methods depending on branch type and target.

| Branch Type | Merge Method | Reason |
|------------|--------------|---------|
| **feature/** → develop | **Squash and merge** ✅ | Keeps develop history clean, one commit per feature |
| **bugfix/** → develop | **Squash and merge** ✅ | Keeps develop history clean, one commit per fix |
| **release/** → edition branch | **Merge commit** | Preserves full release history and commit metadata |
| **hotfix/** → edition branch | **Merge commit** | Preserves hotfix history for audit purposes |

## Release Process

### Community release example

```bash
# Create release branch from develop
git checkout develop
git pull origin develop
git checkout -b release/community/v1.4.0

# Prepare release
echo "1.4.0" > VERSION
git add VERSION CHANGELOG.md
git commit -m "chore: Prepare Community release v1.4.0"

# Finalize release
git checkout community
git pull origin community
git merge --no-ff release/community/v1.4.0
git tag -a v1.4.0 -m "Release v1.4.0"
git push origin community --tags
```

### Hotfix example

```bash
# Create hotfix from community
git checkout community
git pull origin community
git checkout -b hotfix/community/1.3.4-security-fix

# Implement fix
git add .
git commit -m "fix(security): Patch critical vulnerability"

# Merge to community
git checkout community
git merge --no-ff hotfix/community/1.3.4-security-fix
git tag -a v1.3.4 -m "Release v1.3.4 - Security Fix"
git push origin community --tags

# Merge back to develop
git checkout develop
git merge --no-ff hotfix/community/1.3.4-security-fix
git push origin develop
```

## Branch Protection Rules

### `community` Branch Protection

Protect Community release work with the strictest rules.

### `develop` Branch Protection

Protect integration work and require PR-based merges.

### Edition release branches

Apply protected-branch rules consistently to:
- `minimal`
- `enterprise`
- `hyperscaler`
- `military`

## Pull Request Workflows

### Feature Development → develop

- **Base**: `develop`
- **Compare**: `feature/*`

### Community release → community

- **Base**: `community`
- **Compare**: `release/community/*`

### Community hotfix → community + develop

- **Base**: `community`
- **Compare**: `hotfix/community/*`
- **Follow-up**: merge or cherry-pick to `develop`

## Best Practices

### ✅ DOs

1. **Always branch from develop** for normal feature and bugfix work
2. **Use canonical edition branch names** (`community`, `military`, etc.)
3. **Delete branches after merge**
4. **Keep work focused and small**
5. **Treat `main` and `millitary` as migration-only names**

### ❌ DON'Ts

1. ❌ Don't push directly to protected release branches
2. ❌ Don't target legacy `main` for new Community work
3. ❌ Don't target legacy `millitary` for new Military work
4. ❌ Don't keep long-lived feature branches without rebasing
5. ❌ Don't add new automation that assumes `main` is canonical

## Migration Guide for Existing Contributors

### For Developers with Open PRs to legacy branches

- Change Community release PRs from `main` to `community`
- Change Military release PRs from `millitary` to `military`
- Change normal feature PRs to `develop`

## CI/CD Integration

ThemisDB CI/CD must align with canonical branch names.

Examples:
- `develop` for integration validation
- `community` for Community release-lane workflows
- edition-specific branches for edition-specific release packaging

Any remaining workflow, badge, or doc references to `main` should be treated as migration debt unless explicitly marked as legacy context.

## Contact

For questions about the branching strategy:
- GitHub Discussions: https://github.com/makr-code/ThemisDB/discussions
- GitHub Issues: Create an issue with label `question`

---

**Last Updated**: 2026-06-15  
**Version**: 2.0  
**Maintainer**: ThemisDB Core Team
