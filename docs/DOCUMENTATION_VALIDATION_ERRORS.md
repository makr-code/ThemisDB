# Documentation Validation Error Reference

This document provides a comprehensive reference of all errors and warnings that can be reported by the ThemisDB documentation validation tools.

## Error Severity Levels

### Critical Errors ❌
- **Block merge**: These must be fixed before code can be merged
- **Exit code**: Causes validation to fail (exit code 1)
- **Priority**: High - fix immediately

### Warnings ⚠️
- **Informational**: Should be reviewed but don't block merge
- **Exit code**: Don't cause validation failure by default
- **Priority**: Medium - fix when convenient

## Documentation Linter Errors

### Heading Errors

#### Missing Space After Heading Marker
**Severity**: Error  
**Tool**: `docs-lint.py`

**Problem**: Heading lacks required space after `#` markers.

**Example**:
```markdown
##Wrong Heading
```

**Solution**:
```markdown
## Correct Heading
```

#### Heading Level Skipped
**Severity**: Warning  
**Tool**: `docs-lint.py`

**Problem**: Heading hierarchy jumps levels (e.g., H1 to H3, skipping H2).

**Example**:
```markdown
# Heading 1

### Heading 3  ← Skips H2
```

**Solution**:
```markdown
# Heading 1

## Heading 2

### Heading 3
```

### Link Errors

#### Empty Link Text
**Severity**: Error  
**Tool**: `docs-lint.py`

**Problem**: Markdown link has no text between brackets.

**Example**:
```markdown
[](https://example.com)
```

**Solution**:
```markdown
[Example Website](https://example.com)
```

#### Empty Link URL
**Severity**: Error  
**Tool**: `docs-lint.py`

**Problem**: Markdown link has no URL in parentheses.

**Example**:
```markdown
[Link text]()
```

**Solution**:
```markdown
[Link text](https://example.com)
```

### File Naming Errors

#### Filename Contains Spaces
**Severity**: Error  
**Tool**: `docs-lint.py`

**Problem**: File name includes spaces which can cause issues.

**Example**:
```
my documentation file.md
```

**Solution**:
```
my-documentation-file.md
or
my_documentation_file.md
```

#### Uppercase File Extension
**Severity**: Warning  
**Tool**: `docs-lint.py`

**Problem**: File extension is not lowercase.

**Example**:
```
README.MD
document.Md
```

**Solution**:
```
README.md
document.md
```

### Content Errors

#### Trailing Whitespace
**Severity**: Warning  
**Tool**: `docs-lint.py`

**Problem**: Line ends with unnecessary spaces.

**Example**:
```markdown
This line has trailing spaces   
```

**Solution**:
```markdown
This line has no trailing spaces
```

#### File Encoding Error
**Severity**: Error  
**Tool**: `docs-lint.py`

**Problem**: File is not encoded in UTF-8.

**Solution**: Re-save file with UTF-8 encoding.

#### Missing Required Section
**Severity**: Warning  
**Tool**: `docs-lint.py`

**Problem**: Important documentation file missing expected sections (README files should have "Installation" and "Usage").

**Solution**: Add the missing sections to the documentation.

## Link Checker Errors

### Internal Link Errors

#### Broken Internal Link: Target File Not Found
**Severity**: Error  
**Tool**: `link-check.py`

**Problem**: Link points to a file that doesn't exist.

**Example**:
```markdown
[See guide](./non-existent-file.md)
```

**Solution**:
1. Check if file was moved or renamed
2. Update link to correct path
3. Or create the missing file if needed

#### Anchor Not Found in Target File
**Severity**: Warning  
**Tool**: `link-check.py`

**Problem**: Link points to a heading/anchor that doesn't exist in target file.

**Example**:
```markdown
[See section](./file.md#non-existent-heading)
```

**Solution**:
1. Check heading exists in target file
2. Verify anchor name matches heading
3. Remember: anchors are lowercase with hyphens

**GitHub Anchor Conversion Rules**:
- Lowercase: `Installation Guide` → `installation-guide`
- Spaces to hyphens: `My Heading` → `my-heading`
- Remove special chars: `What's New?` → `whats-new`

### External Link Errors

#### External Link Contains Spaces
**Severity**: Error  
**Tool**: `link-check.py`

**Problem**: URL has unencoded spaces.

**Example**:
```markdown
[Link](https://example.com/my page)
```

**Solution**:
```markdown
[Link](https://example.com/my%20page)
```

#### Invalid External Link Format
**Severity**: Error  
**Tool**: `link-check.py`

**Problem**: URL is malformed or missing protocol.

**Example**:
```markdown
[Link](www.example.com)
```

**Solution**:
```markdown
[Link](https://www.example.com)
```

#### Malformed External Link
**Severity**: Error  
**Tool**: `link-check.py`

**Problem**: URL cannot be parsed.

**Solution**: Check URL syntax, ensure proper encoding.

## TOC Validator Errors

### Navigation Errors

#### File Referenced in Nav But Not Found on Disk
**Severity**: Error  
**Tool**: `toc-check.py`

**Problem**: `mkdocs.yml` navigation references a file that doesn't exist.

**Example in mkdocs.yml**:
```yaml
nav:
  - Home: index.md
  - Guide: guide/missing-file.md  ← File doesn't exist
```

**Solution**:
1. Remove reference from `mkdocs.yml`
2. Or create the missing file

#### Important File Not in Navigation
**Severity**: Warning  
**Tool**: `toc-check.py`

**Problem**: Important files (README.md, index.md, INDEX.md, Home.md) exist but aren't in navigation.

**Solution**: Add file to `mkdocs.yml` navigation structure.

#### File Appears Multiple Times in Navigation
**Severity**: Warning  
**Tool**: `toc-check.py`

**Problem**: Same file is referenced multiple times in nav structure.

**Solution**: Remove duplicate entries from `mkdocs.yml`.

## Error Resolution Workflow

### Step 1: Identify the Error

Run validation locally:
```bash
./scripts/validate-docs.sh
```

Or run specific checker:
```bash
python3 scripts/docs-lint.py
python3 scripts/link-check.py
python3 scripts/toc-check.py
```

### Step 2: Understand the Error

1. Read error message carefully
2. Note file path and line number
3. Refer to this document for details
4. Check the specific line in the file

### Step 3: Fix the Error

1. Make the required changes
2. Follow the solution guidelines
3. Test the fix locally

### Step 4: Verify the Fix

```bash
# Re-run validation
./scripts/validate-docs.sh

# Or run specific check
python3 scripts/docs-lint.py path/to/fixed/file.md
```

### Step 5: Commit and Push

```bash
git add .
git commit -m "Fix documentation validation errors"
git push
```

## Common Scenarios

### Scenario 1: File Was Renamed

**Problem**: Links point to old filename, TOC references old name.

**Solution**:
1. Update all links to new filename
2. Update `mkdocs.yml` navigation
3. Run validation to verify

### Scenario 2: File Was Deleted

**Problem**: Links still reference deleted file, TOC still lists it.

**Solution**:
1. Remove file from `mkdocs.yml`
2. Find and remove/update all links to deleted file
3. Run validation to verify

### Scenario 3: New File Added

**Problem**: Validation warns about orphaned file.

**Solution**:
1. Add file to appropriate place in `mkdocs.yml`
2. Ensure file follows naming conventions
3. Run validation to verify

### Scenario 4: Bulk Refactoring

**Problem**: Many files moved/renamed, many links broken.

**Solution**:
1. Create a mapping of old → new paths
2. Use find/replace with caution
3. Update `mkdocs.yml` systematically
4. Run validation frequently during process

## Exclusion Patterns

Some paths are automatically excluded from validation:

- `ARCHIVED/` - Archived documentation
- `archive/` - Archive directories
- `node_modules/` - Node.js dependencies
- `.git/` - Git internals
- `site/` - Built site
- `build/` - Build artifacts
- `__pycache__/` - Python cache

To exclude additional patterns:
```bash
python3 scripts/docs-lint.py --exclude pattern1 pattern2
```

## False Positives

### When to Report

Report false positives if:
1. Error is clearly incorrect
2. Valid markdown is flagged
3. Correct structure is rejected

### How to Report

1. Open GitHub issue
2. Include:
   - Error message
   - File and line number
   - Why it's a false positive
   - Example of problematic content

### Temporary Workarounds

While waiting for fix:
1. Use `--exclude` to skip specific paths
2. Document reason in commit message
3. Track with GitHub issue

## CI/CD Integration

### GitHub Actions Workflow

Validation runs automatically via:
- `.github/workflows/documentation-validation.yml`

### Viewing Results

1. **In PR**: Check GitHub Actions tab
2. **Job Summary**: View summary in action run
3. **Artifacts**: Download JSON reports
4. **Logs**: View detailed logs

### Handling Failures

If validation fails in CI:
1. Click on failed job
2. Review error messages
3. Fix issues locally
4. Push changes
5. CI will re-run automatically

## Best Practices

### Before Committing

1. ✅ Run `./scripts/validate-docs.sh` locally
2. ✅ Fix all errors
3. ✅ Review warnings
4. ✅ Test links manually if in doubt

### When Adding Files

1. ✅ Use proper naming (lowercase, no spaces)
2. ✅ Add to `mkdocs.yml` if important
3. ✅ Check all links work
4. ✅ Follow heading hierarchy

### When Modifying Files

1. ✅ Maintain existing structure
2. ✅ Update related links
3. ✅ Preserve anchors (or update references)
4. ✅ Test after changes

### When Removing Files

1. ✅ Remove from `mkdocs.yml`
2. ✅ Find and fix all references
3. ✅ Consider creating redirect
4. ✅ Update related documentation

## Getting Help

### Resources

1. [Documentation Validation Guide](DOCUMENTATION_VALIDATION.md)
2. [Dokumentations-Validierung (Deutsch)](de/DOKUMENTATIONS_VALIDIERUNG.md)
3. GitHub Issues
4. Documentation Team

### Support Channels

- GitHub Issues: Technical problems
- Documentation Team: Questions about standards
- Pull Request Comments: Review feedback

## Appendix: Error Codes

While the current implementation doesn't use numeric error codes, here's a categorization:

### Category: Structure (S)
- S001: Heading level skipped
- S002: Missing space after heading marker
- S003: File encoding error

### Category: Links (L)
- L001: Empty link text
- L002: Empty link URL
- L003: Broken internal link
- L004: Anchor not found
- L005: External link contains spaces
- L006: Invalid external link format
- L007: Malformed external link

### Category: Files (F)
- F001: Filename contains spaces
- F002: Uppercase file extension
- F003: Trailing whitespace

### Category: Navigation (N)
- N001: File referenced in nav but not found
- N002: Important file not in navigation
- N003: File appears multiple times in navigation

### Category: Content (C)
- C001: Missing required section

---

**Last Updated**: April 2026  
**Version**: 1.0  
**Maintained By**: ThemisDB Documentation Team
