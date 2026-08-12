# 📚 Branching Strategy Documentation Index

> **📌 Navigation-Hinweis:** Der kanonische Root-Index ist [`00_DOCUMENTATION_INDEX.md`](00_DOCUMENTATION_INDEX.md).
> Dieser Index bleibt als themenspezifischer Einstieg zur Branching-Strategie erhalten; für die vollständige Dokumentationsnavigation bitte den kanonischen Index nutzen.

Welcome to the ThemisDB branching strategy documentation. This index helps you find the right document for your needs.

## 🎯 Choose Your Document

### 📖 For First-Time Readers

Start here to understand the overall strategy:

1. **[BRANCHING_IMPLEMENTATION_SUMMARY.md](implementation-history/summaries/BRANCHING_IMPLEMENTATION_SUMMARY.md)** ⭐ **START HERE**
   - Overview of the branching strategy history and rollout
   - Quick reference to supporting documents
   - **Time**: 5-10 minutes

2. **[../BRANCHING_STRATEGY.md](../BRANCHING_STRATEGY.md)** ⭐ **CANONICAL ROOT STRATEGY**
   - Canonical edition-to-branch mapping
   - Release-lane and merge rules
   - AI / governance alignment
   - **Time**: 15-25 minutes

### 📚 Complete Strategy Documentation

Choose based on your language preference:

- **[../BRANCHING_STRATEGY.md](../BRANCHING_STRATEGY.md)** 🇩🇪/Root
  - Canonical root strategy
  - Edition branches, release lanes, migration rules
  - Governance and AI-agent alignment
  - **Time**: 15-25 minutes

- **[BRANCHING_STRATEGY_EN.md](BRANCHING_STRATEGY_EN.md)** 🇬🇧
  - English strategy explanation
  - Supporting workflows and migration notes
  - **Time**: 20-30 minutes

### 🚀 Quick References

For daily use and quick lookups:

- **[BRANCHING_QUICK_REF.md](BRANCHING_QUICK_REF.md)**
  - One-page cheat sheet
  - Common commands
  - Branch types and naming
  - **Print this for your desk!** 📄
  - **Time**: 2-3 minutes

### 🔄 Migration and Setup

For transitioning to the canonical strategy:

- **[MIGRATION_GUIDE.md](MIGRATION_GUIDE.md)**
  - Step-by-step migration instructions
  - For maintainers and contributors
  - Handling existing PRs and legacy branch names
  - **Time**: 15-20 minutes

- **[BRANCH_PROTECTION_SETUP.md](BRANCH_PROTECTION_SETUP.md)**
  - GitHub branch protection configuration
  - CODEOWNERS setup
  - Automated scripts
  - **Time**: 10-15 minutes

## 🎭 Choose by Your Role

### 👨‍💻 As a Developer

**First time contributing:**
1. Read: [../BRANCHING_STRATEGY.md](../BRANCHING_STRATEGY.md)
2. Review: [BRANCHING_STRATEGY_EN.md](BRANCHING_STRATEGY_EN.md) if you want the English version
3. Keep handy: [BRANCHING_QUICK_REF.md](BRANCHING_QUICK_REF.md)

**Daily work:**
- Quick reference: [BRANCHING_QUICK_REF.md](BRANCHING_QUICK_REF.md)
- Contributing guide: [CONTRIBUTING.md](../CONTRIBUTING.md)

### 👨‍💼 As a Maintainer

**Initial setup:**
1. Read: [../BRANCHING_STRATEGY.md](../BRANCHING_STRATEGY.md)
2. Follow: [MIGRATION_GUIDE.md](MIGRATION_GUIDE.md)
3. Configure: [BRANCH_PROTECTION_SETUP.md](BRANCH_PROTECTION_SETUP.md)

### 🆕 As a New Contributor

**Getting started:**
1. Start: [../BRANCHING_STRATEGY.md](../BRANCHING_STRATEGY.md)
2. Contribute: [CONTRIBUTING.md](../CONTRIBUTING.md)
3. Print: [BRANCHING_QUICK_REF.md](BRANCHING_QUICK_REF.md)

## 🔍 Find Information By Topic

### Branch Types
- Complete info: [../BRANCHING_STRATEGY.md](../BRANCHING_STRATEGY.md)
- Quick ref: [BRANCHING_QUICK_REF.md](BRANCHING_QUICK_REF.md)
- English explainer: [BRANCHING_STRATEGY_EN.md](BRANCHING_STRATEGY_EN.md)

### Release Process
- Canonical source: [../BRANCHING_STRATEGY.md](../BRANCHING_STRATEGY.md)
- Release flow: [../RELEASE_STRATEGY.md](../RELEASE_STRATEGY.md)
- Migration help: [MIGRATION_GUIDE.md](MIGRATION_GUIDE.md)

### Pull Requests
- Guidelines: [CONTRIBUTING.md](../CONTRIBUTING.md)
- Workflow: [../BRANCHING_STRATEGY.md](../BRANCHING_STRATEGY.md)

### Migration
- Full guide: [MIGRATION_GUIDE.md](MIGRATION_GUIDE.md)
- Canonical strategy: [../BRANCHING_STRATEGY.md](../BRANCHING_STRATEGY.md)

## 🏆 Best Practices Reminder

Always remember:

1. ✅ **Feature branches** branch from `develop`
2. ✅ **Normal PRs target** `develop`
3. ✅ **Community release work targets** `community`
4. ✅ **Military release work targets** `military`
5. ✅ **Legacy names** `main` and `millitary` are migration-only

## 📝 Additional Resources

### Related Documents in Repository

- **[README.md](README.md)** - Main project README with strategy links
- **[CONTRIBUTING.md](../CONTRIBUTING.md)** - Contribution guidelines
- **[SECURITY.md](../SECURITY.md)** - Security policy
- **[CHANGELOG.md](../CHANGELOG.md)** - Release history examples
- **[../RELEASE_STRATEGY.md](../RELEASE_STRATEGY.md)** - Canonical release flow

### Configuration Files

- **[.github/CODEOWNERS](../.github/CODEOWNERS)** - Code ownership configuration
- **[.github/workflows/develop-ci.yml.example](../.github/workflows/develop-ci.yml.example)** - CI workflow example

## 💬 Getting Help

**About the branching strategy:**
- 💬 [GitHub Discussions](https://github.com/makr-code/ThemisDB/discussions)
- 🏷️ Label: `branching-strategy`

**About migrating PRs:**
- 💬 [GitHub Discussions](https://github.com/makr-code/ThemisDB/discussions)
- 🏷️ Label: `branching-migration`

---

**Last Updated**: 2026-06-15  
**Maintainer**: ThemisDB Documentation Team  
**Questions?** [Open a discussion](https://github.com/makr-code/ThemisDB/discussions)
