# Documentation Archival Process

**Version:** 1.0  
**Last Updated:** 2026-04-06  
**Status:** Official

---

## Overview

This guide documents the process for archiving outdated, superseded, or no-longer-relevant documentation in ThemisDB while preserving historical context and maintaining git history.

## When to Archive Documentation

Archive documentation when:

- ✅ Content is outdated and superseded by newer documentation
- ✅ Feature/component no longer exists or has been replaced
- ✅ Information has been consolidated into another document
- ✅ Content is no longer accurate or relevant to current versions
- ✅ Duplicate or redundant information exists
- ✅ Planned feature was not implemented
- ✅ Implementation summaries from completed work (keep for reference but move from active docs)

## Archive Structure

### Directory Organization

ThemisDB uses a language-specific archive structure:

```
docs/
├── archive/                    # Root-level archives (language-agnostic)
│   ├── README.md              # Archive index
│   └── ARCHIVE_NOTE_TEMPLATE.md
├── de/
│   └── archive/               # German documentation archives
│       └── README.md
├── en/
│   └── archive/               # English documentation archives
│       └── README.md
├── fr/
│   └── archive/               # French documentation archives
│       └── README.md
├── es/
│   └── archive/               # Spanish documentation archives
│       └── README.md
└── ja/
    └── archive/               # Japanese documentation archives
        └── README.md
```

### File Naming

- Keep original filename when archiving
- Add date prefix if multiple versions: `YYYY-MM-DD_original-name.md`
- Use lowercase with hyphens: `feature-implementation-summary.md`

## Archival Process

### Phase 1: Assessment

1. **Verify Need for Archival**
   ```bash
   # Check document age
   git log --follow path/to/document.md
   
   # Check references to document
   grep -r "document-name" docs/
   ```

2. **Review Content**
   - Identify valuable information to preserve
   - Check for content that should be moved to current docs
   - Review git history for important context

3. **Get Approval** (if needed)
   - Discuss with team for major documentation
   - Document decision in issue or PR

### Phase 2: Preparation

4. **Extract Valuable Content**
   - Copy relevant sections to current documentation
   - Update current docs with preserved information
   - Note the source in commit message

5. **Create Archive Note**
   - Use template from `docs/archive/ARCHIVE_NOTE_TEMPLATE.md`
   - Fill in all required fields:
     - Archived Date
     - Reason for archival
     - Replacement documentation link
     - Last valid version (commit SHA)
   - Add historical context
   - Document migration path if applicable

6. **Find All References**
   ```bash
   # Search for references
   grep -r "path/to/document" docs/
   grep -r "document-name" docs/
   
   # Check mkdocs navigation
   grep -A 5 -B 5 "document-name" mkdocs.yml
   ```

### Phase 3: Archival

7. **Move Document**
   ```bash
   # Move preserves git history
   git mv path/to/document.md docs/LANG/archive/document.md
   
   # Or for root-level docs
   git mv document.md docs/archive/document.md
   ```

8. **Add Archive Note to Document**
   - Add archive note as first section of moved document
   - Keep original content below the note
   - Example:
   ```markdown
   # ARCHIVED: Original Document Title
   
   **Archived Date:** 2026-01-12
   **Reason:** Superseded by new implementation guide
   **Replaced By:** [New Implementation Guide](../guides/new-guide.md)
   **Last Valid Version:** a1b2c3d (2025-12-15)
   
   ## Context
   This document described the original implementation approach...
   
   ---
   
   [Original document content follows...]
   ```

9. **Update Archive Index**
   - Add entry to `docs/LANG/archive/README.md`
   - Include metadata table entry
   - Link to replacement documentation

### Phase 4: Update References

10. **Update Internal Links**
    - Find all references: `grep -r "old-document-name" docs/`
    - Update links to point to:
      - New replacement documentation, or
      - Archive location with note that it's archived
    - Remove from navigation menus

11. **Update mkdocs.yml**
    ```bash
    # Remove from navigation
    vim mkdocs.yml
    # Search for document and remove entry
    ```

12. **Update Index Files**
    - Remove from `docs/DOCUMENTATION_INDEX.md`
    - Remove from `docs/00_DOCUMENTATION_INDEX.md`
    - Update any category-specific indexes

### Phase 5: Verification

13. **Check for Broken Links**
    ```bash
    # Build documentation
    mkdocs build
    
    # Check for broken links (if tool available)
    # Or manually verify navigation
    ```

14. **Verify Git History**
    ```bash
    # Confirm history preserved
    git log --follow docs/archive/document.md
    ```

15. **Test Documentation Build**
    ```bash
    mkdocs serve
    # Verify site builds without errors
    ```

### Phase 6: Communication

16. **Update CHANGELOG**
    ```markdown
    ### Documentation
    - Archived `old-document.md` (superseded by new-guide.md)
    ```

17. **Commit Changes**
    ```bash
    git add docs/
    git commit -m "docs: Archive outdated-document (superseded by new-guide)"
    ```

18. **Create PR**
    - Use descriptive title: `docs: Archive [Document Name]`
    - Reference issue if applicable
    - List documents archived
    - Note replacement documentation

19. **Notify Stakeholders** (if significant)
    - Comment on related issues
    - Update team in appropriate channel
    - Note in release notes if user-facing

## Archive Note Template

Use the template located at `docs/archive/ARCHIVE_NOTE_TEMPLATE.md`.

**Required fields:**
- Archived Date
- Reason
- Replaced By (or "N/A")
- Last Valid Version

**Recommended sections:**
- Context (why archived)
- Historical Information (important context)
- Migration Path (how to transition)
- See Also (links to current docs)

## Example Workflow

### Example: Archiving Old Implementation Summary

```bash
# 1. Review document
git log --follow LORA_IMPLEMENTATION_SUMMARY.md

# 2. Find references
grep -r "LORA_IMPLEMENTATION_SUMMARY" docs/

# 3. Move to archive
git mv LORA_IMPLEMENTATION_SUMMARY.md docs/archive/LORA_IMPLEMENTATION_SUMMARY.md

# 4. Edit document to add archive note at top
vim docs/archive/LORA_IMPLEMENTATION_SUMMARY.md

# 5. Update archive README
vim docs/archive/README.md

# 6. Update references (if any)
# Update links to point to current documentation

# 7. Update CHANGELOG
vim CHANGELOG.md

# 8. Commit
git add docs/ CHANGELOG.md
git commit -m "docs: Archive LORA_IMPLEMENTATION_SUMMARY (implementation complete)"

# 9. Create PR
gh pr create --title "docs: Archive LORA implementation summary" --body "..."
```

## Archive Maintenance

### Periodic Review

- Review archives annually
- Remove archives older than 3 years if no longer relevant
- Update archive READMEs with better organization if needed

### Archive README Updates

Each archive directory should have a README.md with:

```markdown
# Archive - [Language] Documentation

**Last Updated:** YYYY-MM-DD

## Overview

This directory contains archived documentation that is no longer current but preserved for historical reference.

## Archived Documents

| Document | Archived Date | Reason | Replaced By |
|----------|---------------|--------|-------------|
| old-doc.md | 2026-01-12 | Superseded | [new-doc.md](../new-doc.md) |

## Notes

Archived documents are preserved with full git history. Content may be outdated and should not be used for current development.

For current documentation, see [Documentation Index](../DOCUMENTATION_INDEX.md).
```

## Troubleshooting

### Issue: Git history lost after move

**Solution:** Always use `git mv` instead of manual copy/delete:
```bash
git mv source.md destination.md
```

### Issue: Many broken links after archival

**Solution:** Use grep to find all references before archiving:
```bash
grep -r "document-name" docs/
```

### Issue: Archive directory doesn't exist

**Solution:** Create it first:
```bash
mkdir -p docs/LANG/archive
```

## Best Practices

1. ✅ **Always preserve git history** - Use `git mv`, never delete and recreate
2. ✅ **Add archive notes** - Future readers need context
3. ✅ **Update all references** - Prevent broken links
4. ✅ **Keep archive READMEs current** - Maintain organized indexes
5. ✅ **Document in CHANGELOG** - Track what was archived
6. ✅ **Be conservative** - When in doubt, keep documentation active
7. ✅ **Extract valuable content first** - Preserve useful information in current docs
8. ✅ **Test documentation builds** - Ensure no errors introduced

## Related Documentation

- **Archive Policy:** [CONTRIBUTING.md § Documentation Archival](#documentation-archival)
- **Archive Template:** [docs/archive/ARCHIVE_NOTE_TEMPLATE.md](ARCHIVE_NOTE_TEMPLATE.md)
- **Investigation Report:** [docs/DOCUMENTATION_ARCHIVAL_INVESTIGATION.md](governance/documentation-history/DOCUMENTATION_ARCHIVAL_INVESTIGATION.md)
- **Documentation Standards:** [CONTRIBUTING.md § Documentation](#documentation)

---

**Questions?** Open an issue or ask in the project chat.
