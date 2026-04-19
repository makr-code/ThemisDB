# Documentation Validation Quick Reference

**Quick reference for ThemisDB documentation validation tools**

## Quick Start

```bash
# Run all validation checks
./scripts/validate-docs.sh

# Run before committing
git add .
./scripts/validate-docs.sh && git commit -m "Your message"
```

## Individual Tools

```bash
# Lint documentation
python3 scripts/docs-lint.py

# Check links
python3 scripts/link-check.py

# Validate TOC
python3 scripts/toc-check.py
```

## Common Commands

### Check Specific Path
```bash
python3 scripts/docs-lint.py docs/de
python3 scripts/link-check.py compendium
```

### Generate JSON Report
```bash
python3 scripts/docs-lint.py --format json --output report.json
```

### Exclude Patterns
```bash
python3 scripts/docs-lint.py --exclude ARCHIVED temp
```

## Common Errors Quick Fix

| Error | Fix |
|-------|-----|
| Missing space after # | `##Title` → `## Title` |
| Broken internal link | Update path or create missing file |
| Empty link text | `[]()` → `[Text](url)` |
| Filename spaces | `my file.md` → `my-file.md` |
| Missing in nav | Add to mkdocs.yml |

## Heading Hierarchy Rules

```markdown
✅ Correct:
# H1
## H2
### H3

❌ Wrong:
# H1
### H3  ← Skipped H2
```

## Link Format

```markdown
✅ Correct:
[Link Text](./path/to/file.md)
[Section](./file.md#heading-name)
[External](https://example.com)

❌ Wrong:
[](./file.md)           ← Empty text
[Link]()                ← Empty URL
[Link](./missing.md)    ← File doesn't exist
```

## File Naming

```bash
✅ Correct:
my-document.md
my_document.md
README.md

❌ Wrong:
my document.md    # Spaces
document.MD       # Uppercase extension
```

## GitHub Actions

Workflow: `.github/workflows/documentation-validation.yml`

**Runs on:**
- Pull requests (docs changes)
- Push to main/develop
- Manual trigger

**View results:**
1. GitHub Actions tab
2. Click workflow run
3. Check job summaries
4. Download artifacts

## Error Severity

### ❌ Errors (Block Merge)
- Broken internal links
- Missing files in nav
- Invalid markdown syntax
- File encoding errors

### ⚠️ Warnings (Review)
- Heading hierarchy
- Trailing whitespace
- Orphaned files
- Missing anchors

## Help

```bash
# Get help for any script
python3 scripts/docs-lint.py --help
python3 scripts/link-check.py --help
python3 scripts/toc-check.py --help
```

## Documentation

- [Full Guide](DOCUMENTATION_VALIDATION.md)
- [Error Reference](DOCUMENTATION_VALIDATION_ERRORS.md)
- [German Guide](de/DOKUMENTATIONS_VALIDIERUNG.md)

## Dependencies

```bash
# Install if needed
pip install pyyaml
```

## CI/CD Integration

All checks run automatically in CI. To require checks before merge:

1. Settings → Branches
2. Add protection rule
3. Enable "Require status checks"
4. Select: Documentation Linting, Link Validation, TOC Validation

---

**Need Help?** Check the full documentation or open a GitHub issue.
