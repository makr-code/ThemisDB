# ThemisDB Documentation - Language Structure

**Last Updated:** December 22, 2025  
**Version:** 1.3.0

---

## Directory Structure

ThemisDB documentation is now organized by language using directory-based structure:

```
docs/
├── en/                     # English documentation
│   ├── README.md
│   ├── DOCUMENTATION_INDEX.md
│   ├── INDEX.md
│   └── Home.md
│
├── de/                     # German documentation (Deutsche Dokumentation)
│   ├── README.md
│   ├── DOCUMENTATION_INDEX.md
│   ├── INDEX.md
│   └── Home.md
│
└── [root level files]      # Language-agnostic files and legacy structure
```

## Key Principles

### 1. Same File Names
Files have identical names across languages:
- ✅ `en/README.md` and `de/README.md`
- ✅ `en/DOCUMENTATION_INDEX.md` and `de/DOCUMENTATION_INDEX.md`
- ❌ ~~`README_en.md` and `README_de.md`~~ (old approach)

### 2. Language Separation by Directory
- **English**: All files in `docs/en/` directory
- **German**: All files in `docs/de/` directory
- **Language-agnostic**: Translation infrastructure files remain at root

### 3. Parallel Structure
Both language directories maintain identical file structure:
```
en/
└── subdirectory/
    └── document.md

de/
└── subdirectory/
    └── document.md
```

## Benefits

1. **Clear Organization**: Language is determined by directory, not filename
2. **Easy Maintenance**: Same file names make it easy to keep languages in sync
3. **Tooling-Friendly**: Standard structure for automation and translation tools
4. **Scalability**: Easy to add more languages (e.g., `docs/fr/`, `docs/es/`)
5. **Clean URLs**: Better URLs for documentation sites (e.g., `/en/readme`, `/de/readme`)

## Usage

### For Readers

**English Documentation:**
- Navigate to `docs/en/` directory
- Start with `en/README.md` or `en/INDEX.md`

**German Documentation:**
- Navigate to `docs/de/` directory  
- Start with `de/README.md` or `de/INDEX.md`

### For Contributors

When creating new documentation:

1. **New Document**: Create in both `en/` and `de/` with same filename
2. **Update Existing**: Update both language versions
3. **Translation**: Copy from `de/` to `en/` and translate

Example:
```bash
# Create new document
touch docs/en/new-feature.md
touch docs/de/new-feature.md

# Edit both versions
vim docs/en/new-feature.md  # Write in English
vim docs/de/new-feature.md  # Write in German
```

## Migration Status

### Completed
- ✅ Directory structure created (`docs/en/`, `docs/de/`)
- ✅ Core files migrated:
  - README.md
  - DOCUMENTATION_INDEX.md
  - INDEX.md
  - Home.md

### In Progress
- 🔄 Migrating remaining documentation files
- 🔄 Updating mkdocs.yml for multi-language support
- 🔄 Setting up language switcher

### Planned
- [ ] Migrate all existing German docs to `de/`
- [ ] Translate and place all English docs in `en/`
- [ ] Update all internal links
- [ ] Configure documentation site for language switching
- [ ] Add language selector to navigation

## Backward Compatibility

Root-level documentation files are maintained for backward compatibility:
- Existing links to `docs/README.md` continue to work
- New structure coexists with legacy structure
- Gradual migration prevents breaking changes

## Technical Implementation

### mkdocs.yml Configuration

The `mkdocs.yml` will be updated to support multi-language navigation:

```yaml
plugins:
  - i18n:
      default_language: en
      languages:
        en:
          name: English
          build: true
        de:
          name: Deutsch
          build: true
```

### Link Format

Internal links should use relative paths:
```markdown
<!-- In en/README.md -->
[See Documentation Index](DOCUMENTATION_INDEX.md)
[See Architecture](../architecture/overview.md)

<!-- In de/README.md -->
[Siehe Dokumentations-Index](DOCUMENTATION_INDEX.md)
[Siehe Architektur](../architecture/overview.md)
```

## Translation Workflow

1. **Write in German**: Create document in `docs/de/`
2. **Translate to English**: Create matching file in `docs/en/`
3. **Keep in Sync**: Update both versions when content changes
4. **Review**: Both versions reviewed for quality

See [TRANSLATION_WORKFLOW.md](TRANSLATION_WORKFLOW.md) for detailed process.

## Statistics

- **English Files**: 4 (README, DOCUMENTATION_INDEX, INDEX, Home)
- **German Files**: 4 (README, DOCUMENTATION_INDEX, INDEX, Home)
- **Total Pending**: ~682 files to be organized into language directories

## Future Enhancements

1. **Automated Sync Checking**: Tool to verify both languages have same files
2. **Translation Memory**: Reuse translations across similar content
3. **Community Translation**: Enable community contributions
4. **Additional Languages**: French, Spanish, etc.

## Questions?

- **Translation Process**: See [TRANSLATION_WORKFLOW.md](TRANSLATION_WORKFLOW.md)
- **Translation Status**: See [TRANSLATION_STATUS.md](TRANSLATION_STATUS.md)
- **Project Analysis**: See [TRANSLATION_SCALE_ANALYSIS.md](TRANSLATION_SCALE_ANALYSIS.md)

---

**Structure Version:** 2.0 (Directory-based)  
**Previous Version:** 1.0 (Suffix-based)  
**Migration Date:** December 22, 2025
