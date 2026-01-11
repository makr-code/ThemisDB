# .github Directory

This directory contains GitHub-specific configuration and documentation for the ThemisDB repository.

## 📁 Directory Structure

### Configuration Files
- **`CODEOWNERS`** - Defines code ownership and auto-reviewers
- **`labels.yml`** - Label definitions for issue/PR categorization
- **`pull_request_template.md`** - Template for pull requests
- **`act-secrets.example`** - Example secrets for local GitHub Actions testing

### Documentation
- **`COPILOT_INSTRUCTIONS.md`** - Instructions for GitHub Copilot
- **`LABELS_GUIDE.md`** - Comprehensive label system guide (EN + DE)
- **`LABELS_QUICK_REF.md`** - Quick reference for GitHub labels
- **`LABELS_IMPLEMENTATION_SUMMARY.md`** - Implementation details (EN + DE)

### Directories
- **`ISSUE_TEMPLATE/`** - Issue templates for bugs, features, etc.
- **`workflows/`** - GitHub Actions CI/CD workflows
- **`scripts/`** - Utility scripts (label sync, etc.)
- **`instructions/`** - Additional GitHub-related instructions

## 🏷️ Label System

ThemisDB uses a comprehensive label system with **64 labels** in **10 categories**:

1. **Priority** (`priority:*`) - P0 (critical) to P3 (low)
2. **Type** (`type:*`) - bug, feature, enhancement, etc.
3. **Area** (`area:*`) - Component/module identification
4. **Status** (`status:*`) - Current state tracking
5. **Effort** (`effort:*`) - Work estimation
6. **Experience** - good first issue, help wanted
7. **Special** - breaking-change, regression, etc.
8. **Edition** (`edition:*`) - minimal, standard, enterprise
9. **Platform** (`platform:*`) - linux, windows, macos
10. **Language** (`lang:*`) - german, english

### Using Labels

**For Issue Reporters:**
- Don't worry about labels - maintainers will add them
- Focus on clear descriptions and reproduction steps

**For Contributors:**
- Check labels to understand priority and scope
- Look for `good first issue` if you're new
- Update status to `status:in-progress` when starting work

**For Maintainers:**
- Sync labels from `labels.yml` to GitHub:
  ```bash
  python3 .github/scripts/sync-labels.py --apply
  ```

### Documentation
- **Full Guide:** [LABELS_GUIDE.md](LABELS_GUIDE.md)
- **Quick Reference:** [LABELS_QUICK_REF.md](LABELS_QUICK_REF.md)
- **Implementation:** [LABELS_IMPLEMENTATION_SUMMARY.md](LABELS_IMPLEMENTATION_SUMMARY.md)

## 🔄 Workflows

The `workflows/` directory contains GitHub Actions workflows for:

- **CI/CD** - Continuous integration and deployment
- **Code Quality** - Linting, formatting, security checks
- **Testing** - Unit tests, integration tests, benchmarks
- **Documentation** - Building and deploying docs
- **Release** - Automated release process

See [workflows/](workflows/) for details.

## 📝 Issue Templates

The `ISSUE_TEMPLATE/` directory contains templates for:

- Bug reports
- Feature requests
- Documentation improvements
- Security vulnerabilities
- And more...

These templates help ensure consistent, high-quality issue reports.

## 🛠️ Scripts

The `scripts/` directory contains utility scripts:

- **`sync-labels.py`** - Sync labels from `labels.yml` to GitHub (Python)
- **`sync-labels.sh`** - Sync labels from `labels.yml` to GitHub (Bash)

See [scripts/README.md](scripts/README.md) for usage details.

## 🤝 Contributing

For contribution guidelines, see [../CONTRIBUTING.md](../CONTRIBUTING.md).

For branching strategy, see:
- [../BRANCHING_STRATEGY.md](../BRANCHING_STRATEGY.md) (German)
- [../BRANCHING_STRATEGY_EN.md](../BRANCHING_STRATEGY_EN.md) (English)

## 📚 Additional Resources

- **Security Policy:** [../SECURITY.md](../SECURITY.md)
- **Code of Conduct:** Embedded in [../CONTRIBUTING.md](../CONTRIBUTING.md)
- **License:** [../LICENSE](../LICENSE)

---

**Last Updated:** 2026-01-11
