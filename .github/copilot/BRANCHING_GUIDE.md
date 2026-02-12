# ThemisDB - Git Branching Strategy

## Git Flow Overview

ThemisDB follows a **Git Flow Branching Strategy** for organized development and releases.

### Branch Structure

| Branch | Purpose | Protection | Merges from |
|--------|---------|-----------|------------|
| **`main`** | Production-ready releases (Tagged: v1.4.0, etc.) | 🔒 Fully protected | `release/*`, `hotfix/*` |
| **`develop`** | Active development and integration | 🔒 Protected | `feature/*`, `bugfix/*`, `release/*` |
| **`feature/*`** | Develop new features | - | - |
| **`bugfix/*`** | Bug fixes for develop | - | - |
| **`hotfix/*`** | Critical production fixes | - | - |
| **`release/*`** | Release preparation | - | - |

## Workflow for Common Tasks

### Creating Feature Branches

Always branch from `develop`:

```bash
git checkout develop
git pull origin develop
git checkout -b feature/xyz
```

### Creating Pull Requests

- **Target Branch**: `develop` (NOT `main`!)
- **Exception**: Hotfixes target `main`

### Release Preparation

```bash
# Branch from develop to release branch
git checkout -b release/1.4.0 develop
# After testing: Merge to main + Tag + Merge back to develop
```

## Merge Strategy

ThemisDB uses different merge methods depending on branch type:

| Branch Type | Target | Merge Method | Reason |
|------------|--------|--------------|--------|
| **`feature/*`** | `develop` | **Squash and merge** ✅ | Clean history, one commit per feature |
| **`bugfix/*`** | `develop` | **Squash and merge** ✅ | Clean history, one commit per fix |
| **`release/*`** | `main` | **Merge commit** | Preserve complete release history |
| **`hotfix/*`** | `main` | **Merge commit** | Preserve complete hotfix history for audit |

## Important for Pull Requests

When creating PRs:

1. **PR Title is Critical**: Becomes commit message with Squash Merge
   - Format: `<type>(<scope>): <description>`
   - Example: `feat(storage): Add vector search optimization`

2. **PR Description is Important**: Becomes commit body with Squash Merge
   - Explain what changed
   - Explain why the change was necessary
   - Reference issues: `Closes #123`

3. **Individual Commits in Branch**: Not important for Feature/Bugfix
   - WIP commits are OK
   - Won't appear in `develop`
   - Only PR title and description matter

## Rules

- ✅ Always create feature branches from `develop`
- ✅ Create PRs to `develop` by default
- ✅ `main` branch is ONLY for production releases
- ❌ Never commit directly to `main` or `develop`
- ❌ No feature PRs directly to `main`

## Documentation References

- Complete guide: `docs/MERGE_STRATEGY_MIGRATION.md`
- Quick reference: `docs/MERGE_STRATEGY_QUICK_REF.md`
- See also: `CONTRIBUTING.md` → "Merge Strategy Guidelines"
