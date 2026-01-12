# Merge Strategy Quick Reference

## TL;DR

- ✅ **Feature/Bugfix PRs** → Use **Squash and merge**
- ✅ **Release/Hotfix PRs** → Use **Merge commit**

## Merge Strategy Table

| From Branch | To Branch | Method | Button to Click |
|------------|-----------|--------|----------------|
| `feature/*` | `develop` | Squash | ⬇️ **Squash and merge** |
| `bugfix/*` | `develop` | Squash | ⬇️ **Squash and merge** |
| `release/*` | `main` | Merge commit | ⬇️ **Create a merge commit** |
| `hotfix/*` | `main` | Merge commit | ⬇️ **Create a merge commit** |
| `main` | `develop` | Merge commit | ⬇️ **Create a merge commit** |

## Why Squash for Features?

✅ One commit per feature in history  
✅ Clean, readable git log  
✅ Easy to revert  
✅ No "WIP" commits in main branch  

## Why Merge Commit for Releases?

✅ Preserves full release structure  
✅ Shows all commits that went into release  
✅ Better for audit trail  
✅ Matches Git Flow model  

## For Contributors

### When Creating a PR:

1. **Write a good PR title** (it becomes the commit message!)
   - ✅ `feat(storage): Add vector search optimization`
   - ❌ `Update files`

2. **Write a good PR description** (it becomes the commit body!)
   - Explain what changed
   - Explain why it changed
   - Reference issues: `Closes #123`

3. **Don't worry about your commit history**
   - WIP commits are fine in feature branches
   - They won't appear in develop after squash merge

### PR Title Format:

```
<type>(<scope>): <description>

Examples:
feat(query): Add support for JSON queries
fix(storage): Resolve memory leak in cache
docs(api): Update authentication examples
refactor(server): Simplify connection handling
```

## For Maintainers

### Merging Feature/Bugfix PRs:

1. Click the **dropdown arrow** next to "Merge pull request"
2. Select **"Squash and merge"**
3. Review the commit message (should be PR title + description)
4. Edit if needed
5. Click **"Confirm squash and merge"**

### Merging Release/Hotfix PRs:

1. Click the **dropdown arrow** next to "Merge pull request"
2. Select **"Create a merge commit"**
3. Use default merge commit message
4. Click **"Confirm merge"**

## Visual Guide

```
Feature Branch Merge (Squash):
─────────────────────────────────
feature/123
  ├─ commit: "WIP: Initial implementation"
  ├─ commit: "fix typo"
  ├─ commit: "address review feedback"
  └─ commit: "Update tests"
        ↓
    [Squash and merge]
        ↓
develop
  └─ commit: "feat(module): Add new feature (#123)"

Only ONE commit appears in develop!
```

```
Release Branch Merge (Merge Commit):
────────────────────────────────────
release/1.4.0
  └─ All feature commits preserved
        ↓
   [Merge commit]
        ↓
main
  └─ merge commit: "Merge release/1.4.0"
       ├─ Links to all commits in release
       └─ Tagged as v1.4.0

Full structure preserved!
```

## Common Questions

**Q: What if I already merged with wrong method?**  
A: Don't worry! It's done. Use correct method going forward.

**Q: Can I change a merged PR?**  
A: No. Once merged, it's permanent. Be careful which button you click!

**Q: What about documentation PRs?**  
A: Treat as feature PRs → Squash and merge

**Q: My PR has merge conflicts, what do I do?**  
A: Update your branch first:
```bash
git checkout feature/my-feature
git merge develop  # or: git rebase develop
git push
```

**Q: The merge button shows the wrong default!**  
A: Ask repo admin to configure GitHub settings (see GITHUB_SETTINGS_CONFIGURATION.md)

## Related Documentation

- 📖 [Full Merge Strategy Migration Guide](MERGE_STRATEGY_MIGRATION.md)
- ⚙️ [GitHub Settings Configuration](GITHUB_SETTINGS_CONFIGURATION.md)
- 📋 [Contributing Guidelines](../CONTRIBUTING.md)
- 🌿 [Branching Strategy](BRANCHING_STRATEGY_EN.md)

## Remember

> **Squash for features, merge for releases!**  
> When in doubt, use squash for anything going to `develop`.

---

**Quick Tip**: GitHub will use the default you configured, so if you set up the repository correctly, the right option will usually already be selected! Just double-check before clicking "Confirm".
