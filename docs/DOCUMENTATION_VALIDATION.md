# Documentation Validation Tools

## Overview

ThemisDB uses automated validation tools to ensure documentation quality and consistency. These tools check for structural issues, broken links, and TOC consistency before any code is merged.

## Validation Tools

### 1. Documentation Linter (`docs-lint.py`)

The documentation linter checks for:

- **Heading hierarchy**: Ensures headings follow proper nesting (no skipping levels)
- **Markdown syntax**: Validates proper markdown formatting
- **Link format**: Checks for empty link text or URLs
- **File naming**: Validates naming conventions (no spaces, lowercase extensions)
- **Required sections**: Ensures important files contain necessary sections
- **Trailing whitespace**: Detects unnecessary trailing spaces

#### Usage

```bash
# Lint all documentation
python3 scripts/docs-lint.py

# Lint specific paths
python3 scripts/docs-lint.py docs/de compendium

# Generate JSON report
python3 scripts/docs-lint.py --format json --output lint-report.json

# Fail on warnings
python3 scripts/docs-lint.py --fail-on-warnings
```

#### Common Errors and Solutions

| Error | Solution |
|-------|----------|
| Missing space after heading marker | Add space: `##Heading` → `## Heading` |
| Heading level skipped | Don't skip levels: `# H1` → `### H3` should be `# H1` → `## H2` |
| Empty link text | Provide descriptive text: `[]()` → `[Link Text](url)` |
| Filename contains spaces | Use hyphens or underscores: `my file.md` → `my-file.md` |
| Trailing whitespace | Remove spaces at end of lines |

### 2. Link Checker (`link-check.py`)

The link checker validates:

- **Internal links**: Verifies files and anchors exist
- **Anchor links**: Checks that heading anchors are valid
- **External links**: Basic format validation (full HTTP checks via GitHub Actions)
- **Relative paths**: Ensures correct path resolution

#### Usage

```bash
# Check all links
python3 scripts/link-check.py

# Check specific paths
python3 scripts/link-check.py docs compendium

# Check only internal links
python3 scripts/link-check.py --internal-only

# Generate JSON report
python3 scripts/link-check.py --format json --output link-report.json
```

#### Common Errors and Solutions

| Error | Solution |
|-------|----------|
| Broken internal link: target file not found | Update link to correct file path |
| Anchor not found in target file | Check heading exists or fix anchor name |
| External link contains spaces | Encode spaces: `my url` → `my%20url` |
| Invalid external link format | Ensure proper URL: `www.example.com` → `https://www.example.com` |

### 3. TOC Validator (`toc-check.py`)

The TOC validator checks:

- **Navigation structure**: Validates mkdocs.yml navigation entries
- **File references**: Ensures all nav entries point to existing files
- **Orphaned files**: Detects important files not in navigation
- **Duplicate entries**: Warns about files appearing multiple times
- **Cross-references**: Validates consistency between docs and compendium

#### Usage

```bash
# Validate all mkdocs configs
python3 scripts/toc-check.py

# Validate specific config
python3 scripts/toc-check.py --configs mkdocs.yml

# Generate JSON report
python3 scripts/toc-check.py --format json --output toc-report.json
```

#### Common Errors and Solutions

| Error | Solution |
|-------|----------|
| File referenced in nav but not found | Remove entry or create missing file |
| Important file not in navigation | Add file to mkdocs.yml nav section |
| File appears multiple times in navigation | Remove duplicate references |
| Missing files | Create files or update navigation |

## GitHub Actions Integration

### Workflow: `documentation-validation.yml`

The validation workflow (`.github/workflows/documentation-validation.yml`) runs automatically on:

- **Pull requests** affecting documentation (`docs/**`, `compendium/**`, `scripts/docs-lint.py`, etc.)
- **Pushes** to `main`, `develop`, or `release/**` branches
- **Manual trigger** via `workflow_dispatch`

### Validation Jobs

1. **docs-lint** (`Documentation Linting`): Runs `scripts/docs-lint.py`
2. **link-check** (`Link Validation`): Validates internal links via `scripts/link-check.py --internal-only`
3. **external-link-check** (`External Link Check`): Checks external links using `markdown-link-check` (runs on `main`/`develop` branches only)
4. **toc-validation** (`TOC Validation`): Validates TOC structure via `scripts/toc-check.py`
5. **validation-summary** (`Validation Summary`): Aggregates results from all required jobs

### Required Status Checks

The following checks must pass before merging:

- ✅ Documentation Linting
- ✅ Link Validation
- ✅ TOC Validation

### Viewing Results

Results are available in multiple places:

1. **Job Summary**: View in GitHub Actions job summary for each job
2. **Artifacts**: Download JSON reports (`lint-report`, `link-report`, `toc-report`) for detailed analysis
3. **PR Comments**: Automated comments on pull requests (if enabled via branch protection)

## Local Development

### Pre-commit Checks

Run validation locally before committing:

```bash
# Run all checks
./scripts/validate-docs.sh

# Or run individually
python3 scripts/docs-lint.py
python3 scripts/link-check.py
python3 scripts/toc-check.py
```

### Installing Dependencies

```bash
# Python dependencies
pip install pyyaml

# For external link checking (optional)
npm install -g markdown-link-check
```

## Configuration

### Linter Configuration

The linter uses built-in rules. To customize:

1. Edit `scripts/docs-lint.py`
2. Modify checking functions
3. Add custom validation rules

### Link Checker Configuration

External link checking uses `.markdown-link-check.json`:

```json
{
  "ignorePatterns": [
    {"pattern": "^https://github.com/.*/issues/[0-9]+$"},
    {"pattern": "^https://github.com/.*/pull/[0-9]+$"}
  ],
  "timeout": "5s",
  "retryOn429": true,
  "aliveStatusCodes": [200, 206, 301, 302, 307, 308]
}
```

### Exclusions

By default, these paths are excluded:

- `ARCHIVED/`
- `archive/`
- `node_modules/`
- `.git/`
- `site/`
- `build/`

To add exclusions:

```bash
python3 scripts/docs-lint.py --exclude ARCHIVED archive temp
```

## Troubleshooting

### Validation Failed in CI

1. **Check job logs**: Review GitHub Actions logs
2. **Download artifacts**: Get detailed JSON reports
3. **Run locally**: Reproduce issue with same command
4. **Fix issues**: Address errors and warnings
5. **Commit fixes**: Push changes to trigger re-validation

### False Positives

If you encounter false positives:

1. **Review the rule**: Check if it's appropriate
2. **Add exclusion**: Exclude specific patterns if needed
3. **Report issue**: Open GitHub issue for incorrect validation

### Performance Issues

For large documentation sets:

1. **Limit scope**: Validate only changed files
2. **Use exclusions**: Skip unnecessary directories
3. **Parallel execution**: Consider splitting checks

## Error Reference

### Critical Errors (Block Merge)

- Broken internal links
- Missing files in navigation
- Invalid markdown syntax
- File encoding errors

### Warnings (Informational)

- Heading hierarchy issues
- Trailing whitespace
- Orphaned files
- Anchor issues

## Best Practices

1. **Run validation locally** before pushing
2. **Fix errors immediately** - don't accumulate issues
3. **Review warnings** - they often indicate real problems
4. **Keep TOC updated** when adding/removing files
5. **Use descriptive link text** for accessibility
6. **Follow naming conventions** for consistency
7. **Test anchors** when linking to specific sections

## Support

For questions or issues:

1. Check this documentation
2. Review error messages carefully
3. Open a GitHub issue with details
4. Contact the documentation team

## Version History

- **v1.0** (2026-01): Initial implementation
  - Documentation linting
  - Link validation
  - TOC checking
  - GitHub Actions integration
