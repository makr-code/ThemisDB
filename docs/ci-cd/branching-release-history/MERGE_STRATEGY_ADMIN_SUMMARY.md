# Merge Strategy Update - Summary for Repository Administrators

## What Changed?

ThemisDB now has an **official merge strategy** that prefers **squash merges** for feature and bugfix branches, while maintaining merge commits for releases and hotfixes.

## Why This Change?

**Previous situation**: The repository was using regular merge commits for all PRs, leading to:
- ❌ Cluttered git history with WIP commits
- ❌ Hard to read `git log`
- ❌ Difficult to identify what each feature actually changed
- ❌ Many "fix typo", "addressing review comments" commits in main branch

**New approach**: Use squash merge for features/bugfixes, resulting in:
- ✅ Clean, readable history in `develop` branch
- ✅ One logical commit per feature
- ✅ Easy to revert changes
- ✅ Better automatic changelog generation
- ✅ Focus on what changed, not how it was developed

## Quick Reference

### Merge Strategy Table

| Branch Type | Merge Method | When |
|------------|--------------|------|
| `feature/*` → `develop` | **Squash and merge** ✅ | Every feature PR |
| `bugfix/*` → `develop` | **Squash and merge** ✅ | Every bugfix PR |
| `release/*` → `main` | **Merge commit** | Release PRs only |
| `hotfix/*` → `main` | **Merge commit** | Hotfix PRs only |

**Simple rule**: If it's going to `develop`, squash it. If it's going to `main`, merge commit.

## Action Required: Configure GitHub Settings

⚠️ **Repository administrators must manually configure GitHub settings**

### Step-by-Step:

1. Go to: `https://github.com/makr-code/ThemisDB/settings`
2. Scroll to **Pull Requests** section
3. Configure:
   - ✅ Enable "Allow squash merging"
   - ✅ Keep "Allow merge commits" enabled
   - ❌ Disable "Allow rebase merging" (optional)
4. Set **Default to: squash merge**
5. Configure commit message: "Pull request title and description"

**Detailed guide**: [docs/GITHUB_SETTINGS_CONFIGURATION.md](../../GITHUB_SETTINGS_CONFIGURATION.md)

## Documentation Updated

All relevant documentation has been updated:

### Core Files:
1. ✅ **CONTRIBUTING.md** - Added merge strategy guidelines section
2. ✅ **.github/pull_request_template.md** - Updated merge checklist
3. ✅ **docs/BRANCHING_STRATEGY.md** (German) - Added merge strategy section
4. ✅ **docs/BRANCHING_STRATEGY_EN.md** (English) - Added merge strategy section
5. ✅ **docs/BRANCHING_VISUAL_GUIDE.md** - Updated visual diagrams

### New Documentation:
6. ✅ **docs/MERGE_STRATEGY_MIGRATION.md** - Complete migration guide
7. ✅ **docs/GITHUB_SETTINGS_CONFIGURATION.md** - GitHub setup instructions
8. ✅ **docs/MERGE_STRATEGY_QUICK_REF.md** - One-page quick reference
9. ✅ **docs/BRANCHING_DOCS_INDEX.md** - Updated index with new docs

## For Team Members

### Contributors:
- Your workflow doesn't change much
- Write good PR titles (they become commit messages!)
- Write good PR descriptions (they become commit bodies!)
- Don't worry about WIP commits in feature branches

**Quick guide**: [docs/MERGE_STRATEGY_QUICK_REF.md](MERGE_STRATEGY_QUICK_REF.md)

### Maintainers:
- Use "Squash and merge" for feature/bugfix PRs (will be default)
- Use "Merge commit" for release/hotfix PRs (select from dropdown)
- Review PR title/description before squashing (becomes the commit message)

**Quick guide**: [docs/MERGE_STRATEGY_QUICK_REF.md](MERGE_STRATEGY_QUICK_REF.md)

## FAQ

### Can we change past history?

**No.** Rewriting history would require force-pushing to protected branches and would disrupt all contributors. Past commits stay as they are. This only affects future PRs.

### What about open PRs?

They will automatically use the new default merge method once GitHub settings are configured. No action needed from contributors.

### What if I use the wrong merge method?

Once merged, it can't be changed. Just be careful going forward. The GitHub default will help make it easier.

### Do I need to change my development workflow?

Not really. Just be more careful about PR titles and descriptions since they'll become the commit message.

## Announcement Template

Here's a suggested announcement for the team:

---

**📢 New Merge Strategy: Squash Merge for Features**

Hi team! 👋

We've updated our merge strategy to keep the git history cleaner and more readable.

**What's changing:**
- Feature and bugfix PRs will now be **squash merged** (one commit per feature)
- Release and hotfix PRs will still use merge commits

**What you need to do:**
- ✍️ Write good PR titles (they become commit messages)
- 📝 Write good PR descriptions (they become commit bodies)
- 🎯 Keep PRs focused on one feature/fix

**Why this is good:**
- ✅ Cleaner git history
- ✅ Easier to understand what changed
- ✅ Easier to revert if needed

**Learn more:**
- Quick ref: [MERGE_STRATEGY_QUICK_REF.md](MERGE_STRATEGY_QUICK_REF.md)
- Full guide: [MERGE_STRATEGY_MIGRATION.md](../../MERGE_STRATEGY_MIGRATION.md)

Questions? Ask in [Discussions](https://github.com/makr-code/ThemisDB/discussions)!

---

## Implementation Checklist

Use this checklist to roll out the change:

### For Repository Administrators:
- [ ] Configure GitHub settings (see GITHUB_SETTINGS_CONFIGURATION.md)
- [ ] Verify settings work correctly on a test PR
- [ ] Announce change to team
- [ ] Pin announcement in Discussions
- [ ] Monitor first few PRs to help team adapt

### For Team:
- [ ] Read quick reference guide (MERGE_STRATEGY_QUICK_REF.md)
- [ ] Understand when to use squash vs merge commit
- [ ] Update PR templates if you have personal ones
- [ ] Adjust workflow for writing better PR titles/descriptions

### Verification:
- [ ] Check GitHub settings are configured
- [ ] Test on a feature PR (should default to squash)
- [ ] Test on a release PR (should allow merge commit)
- [ ] Verify commit messages include PR titles
- [ ] Check that history looks clean after first squash merge

## Timeline

**Immediate:**
- Documentation is updated ✅
- Can be implemented right away

**Recommended:**
1. Configure GitHub settings (5 minutes)
2. Announce to team (today)
3. Start using on all new PRs (immediately)
4. Monitor and help team adapt (first week)

## Support

**Questions or issues?**
- 📖 Read: [MERGE_STRATEGY_MIGRATION.md](../../MERGE_STRATEGY_MIGRATION.md)
- ⚡ Quick ref: [MERGE_STRATEGY_QUICK_REF.md](MERGE_STRATEGY_QUICK_REF.md)
- 💬 Ask: [GitHub Discussions](https://github.com/makr-code/ThemisDB/discussions)
- 🐛 Found doc errors? Create an issue with label `documentation`

## Benefits Summary

After this change, you'll see:

**In develop branch:**
```bash
git log develop --oneline

8f8c2c4 feat(storage): Add vector search optimization (#123)
7a9b3e1 fix(query): Resolve pagination bug (#122)
6c5d2f0 feat(api): Add JSON query support (#121)
```

Clean, one commit per feature! ✨

**In feature branches:**
```bash
git log feature/123-vector-search --oneline

d3e4f5a Update tests
c2b3a4d Address review feedback
b1a2c3d Fix typo
a0b1c2d Initial implementation
```

Full development history preserved! 📚

## Conclusion

This change improves the maintainability and readability of the ThemisDB repository going forward. All documentation is in place, and the only remaining step is to configure the GitHub repository settings.

---

**Next Steps:**
1. ⚙️ Configure GitHub settings using [GITHUB_SETTINGS_CONFIGURATION.md](../../GITHUB_SETTINGS_CONFIGURATION.md)
2. 📢 Announce to team
3. ✅ Start using on all new PRs

**Questions?** Open a [Discussion](https://github.com/makr-code/ThemisDB/discussions)!

---

**Document Version**: 1.0  
**Date**: 2026-01-11  
**Status**: Ready to Implement
