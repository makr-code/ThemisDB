# Git Branching Quick Reference - ThemisDB

## Branch Overview

```
main (protected)          ← Production releases only
  ↑
  ├─ release/1.4.0        ← Release preparation
  │   ↑
  │   └─ develop (protected)  ← Integration branch
  │       ↑
  │       ├─ feature/search-opt
  │       ├─ feature/llm-stream
  │       └─ bugfix/query-fix
  │
  └─ hotfix/1.3.4         ← Critical production fixes
```

## Common Commands

### Start New Feature
```bash
git checkout develop
git pull origin develop
git checkout -b feature/my-feature
```

### Update Feature Branch
```bash
git checkout feature/my-feature
git pull origin develop
```

### Complete Feature
```bash
git push origin feature/my-feature
# Create PR: feature/my-feature → develop
```

### Create Hotfix
```bash
git checkout main
git pull origin main
git checkout -b hotfix/1.3.4-critical
# Make fix
git push origin hotfix/1.3.4-critical
# Create PR: hotfix/1.3.4-critical → main
# Then merge back to develop
```

### Start Release
```bash
git checkout develop
git pull origin develop
git checkout -b release/1.4.0
echo "1.4.0" > VERSION
git commit -am "chore: Prepare v1.4.0"
```

### Finish Release
```bash
# Merge to main
git checkout main
git merge --no-ff release/1.4.0
git tag -a v1.4.0 -m "Release v1.4.0"
git push origin main --tags

# Merge back to develop
git checkout develop
git merge --no-ff release/1.4.0
git push origin develop
```

## Branch Types

| Type | Base | Target | Naming | Example |
|------|------|--------|--------|---------|
| **Feature** | develop | develop | `feature/<desc>` | `feature/123-vector-search` |
| **Bugfix** | develop | develop | `bugfix/<desc>` | `bugfix/456-memory-leak` |
| **Hotfix** | main | main + develop | `hotfix/<ver>-<desc>` | `hotfix/1.3.4-security` |
| **Release** | develop | main + develop | `release/<ver>` | `release/1.4.0` |

## PR Targets

✅ **Correct:**
- `feature/*` → `develop`
- `bugfix/*` → `develop`
- `hotfix/*` → `main` (then to `develop`)
- `release/*` → `main` (then to `develop`)

❌ **Incorrect:**
- `feature/*` → `main`
- `bugfix/*` → `main`
- Any direct push to `main` or `develop`

## Git Aliases (Optional)

Add to `~/.gitconfig`:

```ini
[alias]
    # Quick branch switching
    dev = checkout develop
    main = checkout main
    
    # Create feature branch
    feat = "!f() { git checkout develop && git pull && git checkout -b feature/$1; }; f"
    
    # Create bugfix branch
    bug = "!f() { git checkout develop && git pull && git checkout -b bugfix/$1; }; f"
    
    # Create hotfix branch
    hot = "!f() { git checkout main && git pull && git checkout -b hotfix/$1; }; f"
    
    # Update current branch from develop
    sync = !git pull origin develop
    
    # Show branch structure
    tree = log --graph --oneline --all --decorate
```

**Usage:**
```bash
git feat my-feature        # Creates feature/my-feature from develop
git bug connection-leak    # Creates bugfix/connection-leak from develop
git hot 1.3.4-critical     # Creates hotfix/1.3.4-critical from main
git sync                   # Updates from develop
git tree                   # Visualize branch structure
```

## Commit Message Format

```
<type>(<scope>): <subject>

<body>

<footer>
```

**Types:**
- `feat`: New feature
- `fix`: Bug fix
- `docs`: Documentation
- `style`: Formatting
- `refactor`: Code restructuring
- `test`: Tests
- `chore`: Maintenance

**Examples:**
```bash
git commit -m "feat(storage): Add incremental backup"
git commit -m "fix(query): Resolve pagination edge case"
git commit -m "docs(api): Update REST API examples"
```

## Emergency Procedures

### Revert Last Commit (Local Only)
```bash
git reset --soft HEAD~1
```

### Fix Wrong Branch
```bash
# If committed to wrong branch (not pushed yet)
git checkout correct-branch
git cherry-pick wrong-branch
git checkout wrong-branch
git reset --hard HEAD~1
```

### Resolve Merge Conflicts
```bash
# During rebase
git rebase origin/develop
# Fix conflicts in files
git add <resolved-files>
git rebase --continue

# Or abort and ask for help
git rebase --abort
```

## Status Checks

### Before Creating PR
- [ ] Branch is up to date with target
- [ ] All tests pass locally
- [ ] Code follows style guidelines
- [ ] Commit messages are clear
- [ ] No secrets or sensitive data

### During Review
- [ ] Address review comments
- [ ] Keep PR updated with target branch
- [ ] Resolve all conversations
- [ ] CI checks are green

## Quick Links

- 📖 [Full Strategy](BRANCHING_STRATEGY.md)
- 🔄 [Migration Guide](../../MIGRATION_GUIDE.md)
- 🛡️ [Branch Protection](BRANCH_PROTECTION_SETUP.md)
- 🤝 [Contributing](../../../CONTRIBUTING.md)

## Help

**Questions?**
- GitHub Discussions: https://github.com/makr-code/ThemisDB/discussions
- Create issue with `question` label

**Found a bug in workflow?**
- Create issue with `branching-strategy` label

---

**Print this page for your desk! 📄**

Save as PDF: `Ctrl+P` → `Save as PDF`
