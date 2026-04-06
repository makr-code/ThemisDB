# ThemisDB Documentation - Language Structure

**Last Updated:** April 2026  
**Version:** 1.3.0

---

## Documentation Languages

ThemisDB documentation is available in multiple languages:

### Available Languages

- **🇩🇪 German (Deutsch)** - `docs/de/` - **Primary/Authoritative Documentation**
- **🇬🇧 English** - `docs/en/` - Complete translation
- **🇫🇷 French (Français)** - `docs/fr/` - In development
- **🇪🇸 Spanish (Español)** - `docs/es/` - In development
- **🇯🇵 Japanese (日本語)** - `docs/ja/` - In development

> **Important:** The German documentation (`docs/de/`) is the **authoritative and most up-to-date** version. All translations may lag behind the German documentation.

## Directory Structure

```
docs/
├── de/                     # German (PRIMARY - Authoritative)
│   ├── README.md
│   ├── DOCUMENTATION_INDEX.md
│   ├── INDEX.md
│   └── Home.md
│
├── en/                     # English (Complete translation)
│   ├── README.md
│   ├── DOCUMENTATION_INDEX.md
│   ├── INDEX.md
│   └── Home.md
│
├── fr/                     # French (In development)
│   └── README.md
│
├── es/                     # Spanish (In development)
│   └── README.md
│
├── ja/                     # Japanese (In development)
│   └── README.md
│
└── [root level files]      # Language-agnostic files and infrastructure

## Root File Language Mapping

| Root File | Language | Current State | Target Location |
|-----------|:--------:|---------------|-----------------|
| `README.md` | Neutral stub | Links to de/en | `docs/de/README.md` (primary), `docs/en/README.md` |
| `DOCUMENTATION_INDEX.md` | Neutral stub | Links to de/en | `docs/de/DOCUMENTATION_INDEX.md`, `docs/en/DOCUMENTATION_INDEX.md` |
| `INDEX.md` | Neutral stub | Links to de/en | `docs/de/INDEX.md`, `docs/en/INDEX.md` |
| `Home.md` | Neutral stub | Links to de/en | `docs/de/Home.md`, `docs/en/Home.md` |
| `glossary.md` | German | Moved to de | `docs/de/glossary.md` (authoritative), EN pending |
| `README-DOCUMENTATION.md` | English | Moved to en | `docs/en/README-DOCUMENTATION.md`, DE pending |
| `QUICK_REFERENCE.md` | English (archived) | Moved to en | `docs/en/QUICK_REFERENCE.md`, DE pending |

```

## Key Principles

### 1. German is Primary
**The German documentation is the authoritative source:**
- All new content is written in German first
- German documentation is always the most up-to-date
- Translations follow the German version
- In case of conflicts or discrepancies, German documentation prevails

### 2. Same File Names
Files have identical names across languages:
- ✅ `en/README.md` and `de/README.md`
- ✅ `en/DOCUMENTATION_INDEX.md` and `de/DOCUMENTATION_INDEX.md`
- ❌ ~~`README_en.md` and `README_de.md`~~ (old approach)

### 2. Language Separation by Directory
- **German (Primary)**: All files in `docs/de/` directory
- **English**: All files in `docs/en/` directory
- **French**: All files in `docs/fr/` directory
- **Spanish**: All files in `docs/es/` directory
- **Japanese**: All files in `docs/ja/` directory
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
2. **Authoritative Source**: German documentation is clearly marked as primary
3. **Easy Maintenance**: Same file names make it easy to keep languages in sync
4. **Tooling-Friendly**: Standard structure for automation and translation tools
5. **Scalability**: Easy to add more languages
6. **Clean URLs**: Better URLs for documentation sites (e.g., `/de/readme`, `/en/readme`, `/fr/readme`)
7. **Community Contributions**: Clear structure for community translators

## Usage

### For Readers

**German Documentation (Authoritative):**
- Navigate to `docs/de/` directory
- Start with `de/README.md` or `de/INDEX.md`
- **This is the official, most up-to-date documentation**

**English Documentation:**
- Navigate to `docs/en/` directory
- Start with `en/README.md` or `en/INDEX.md`

**Other Languages:**
- French: `docs/fr/README.md` (In development)
- Spanish: `docs/es/README.md` (In development)
- Japanese: `docs/ja/README.md` (In development)

### For Contributors

When creating new documentation:

1. **New Document**: Create in German (`de/`) first, then translate to other languages
2. **Update Existing**: Update German version first, then update translations
3. **Translation**: Copy from `de/` to target language and translate

Example:
```bash
# Create new document in German (primary)
vim docs/de/new-feature.md

# Translate to English
cp docs/de/new-feature.md docs/en/new-feature.md
vim docs/en/new-feature.md  # Translate content

# Translate to other languages
cp docs/de/new-feature.md docs/fr/new-feature.md
vim docs/fr/new-feature.md  # Translate content
```

## Migration Status

### Completed
- ✅ Directory structure created (`docs/de/`, `docs/en/`, `docs/fr/`, `docs/es/`, `docs/ja/`)
- ✅ Core files migrated to German and English:
  - README.md
  - DOCUMENTATION_INDEX.md
  - INDEX.md
  - Home.md
- ✅ Basic structure for French, Spanish, and Japanese
- ✅ Language notices added to all translations

### Translation Status by Language

| Language | Status | Files Translated | Primary Contact |
|----------|--------|------------------|-----------------|
| 🇩🇪 German | ✅ Complete (Primary) | All files | Main documentation |
| 🇬🇧 English | 🟢 Active | 4 core files | In progress |
| 🇫🇷 French | 🔴 Planned | 1 file (README) | Community needed |
| 🇪🇸 Spanish | 🔴 Planned | 1 file (README) | Community needed |
| 🇯🇵 Japanese | 🔴 Planned | 1 file (README) | Community needed |

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

1. **Automated Sync Checking**: Tool to verify all languages have same file structure
2. **Translation Memory**: Reuse translations across similar content
3. **Community Translation Platform**: Enable community contributions with review process
4. **Additional Languages**: Based on community demand (Chinese, Korean, Portuguese, etc.)
5. **Automated Translation Updates**: Notify translators when German docs are updated

## Questions?

- **Translation Process**: See [TRANSLATION_WORKFLOW.md](TRANSLATION_WORKFLOW.md)
- **Translation Status**: See [TRANSLATION_STATUS.md](TRANSLATION_STATUS.md)
- **Project Analysis**: See [TRANSLATION_SCALE_ANALYSIS.md](TRANSLATION_SCALE_ANALYSIS.md)

---

**Structure Version:** 2.0 (Directory-based)  
**Previous Version:** 1.0 (Suffix-based)  
**Migration Date:** December 22, 2025
