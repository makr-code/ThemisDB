# Dynamic Glossary System Guide

**Version:** 2.0  
**Generated:** 2026-09-02  
**Status:** Production-Ready

## Overview

The ThemisDB dynamic glossary system automatically extracts, enriches, and links terminology across repository documentation. Unlike static glossaries that require manual maintenance, this system:

- **Scans** `docs/`, `src/`, and `include/` for glossary term candidates using heuristics
- **Enriches** terms with descriptions, multilingual translations, and categories
- **Generates** language-specific glossaries (English, German, French)
- **Links** glossary terms automatically in wiki pages using `[[Term]]` notation

## Architecture

### Pipeline Phases

```
Phase B.2           Phase B.3                 Phase B.4              Phase A
Scanning   →    Enrichment    →    Generation    →    Publishing
   ↓                ↓                   ↓                   ↓
candidates.json  glossary_index.json  {de,en,fr}/  wiki with term links
                                    glossary.md
```

#### Phase B.2: Glossary Scanning (`scan_glossary_terms.py`)

**Inputs:**
- `docs/`, `src/`, `include/` directories
- Existing glossaries: `docs/{de,fr}/glossary.md`, `docs/compendium/docs/appendix_h_glossary.md`

**Outputs:**
- `docs/glossary_candidates.json` (unvalidated term list)

**Heuristics Used:**
1. **Doxygen `@term` tags** in C++ headers/source (priority: high)
2. **Markdown `## Header`** patterns in documentation (priority: medium)
3. **CAPS-word patterns** (e.g., `MVCC`, `AQL`, `HNSW`) in documentation (priority: low)
4. **Existing glossary consolidation** from manual glossary files (priority: high)

**Features:**
- Deduplication across sources
- Occurrence counting (reference tracking)
- Source attribution with line numbers
- Filtering by priority level (`--min-priority high|medium|low`)

**Usage:**
```bash
python scripts/scan_glossary_terms.py \
  --repo-root . \
  --output docs/glossary_candidates.json \
  --min-priority low
```

#### Phase B.3: Glossary Enrichment (`build_glossary_with_copilot.py`)

**Inputs:**
- `docs/glossary_candidates.json` from Phase B.2

**Outputs:**
- `docs/glossary_index.json` (enriched, categorized glossary)

**Enrichment Steps:**
1. **Category Assignment** via heuristics (4 categories):
   - `themisdb-core`: Core database concepts (Entity, Edge, Collection, Transaction, MVCC, WAL, TSStore)
   - `data-model`: Data model abstractions (Relational, Document, Graph, Vector, Time-Series)
   - `query-performance`: Query and performance concepts (Index, Query Plan, Bind Variables, Selectivity)
   - `general-knowledge`: Wikipedia-linked concepts (ACID, HNSW, BTree, etc.)

2. **Description Generation** (fallback heuristic):
   - Short descriptions (~100 chars): single-sentence definitions
   - Long descriptions (~500 chars): detailed explanations with context

3. **Multilingual Support**:
   - English (primary)
   - German (de_DE)
   - French (fr_FR)
   - Via heuristic translation database (extensible)

4. **Wikipedia Mapping**:
   - Links for general-knowledge terms
   - Example: `ACID` → https://en.wikipedia.org/wiki/ACID

5. **Cross-References**:
   - Auto-generated "see also" suggestions based on term relationships

**Features:**
- Optional Copilot/LLM enrichment (pass `--enable-copilot` to use GitHub Models API)
- Validation flags for human review
- Schema versioning (v2.0)
- Timestamp and source tracking

**Usage:**
```bash
# Heuristic-only (fast, no API calls)
python scripts/build_glossary_with_copilot.py \
  --input docs/glossary_candidates.json \
  --output docs/glossary_index.json \
  --repo-root .

# With Copilot enrichment (requires GITHUB_TOKEN)
python scripts/build_glossary_with_copilot.py \
  --input docs/glossary_candidates.json \
  --output docs/glossary_index.json \
  --repo-root . \
  --enable-copilot \
  --model gpt-4o-mini
```

#### Phase B.4: Glossary Generation (`generate_glossaries.py`)

**Inputs:**
- `docs/glossary_index.json` from Phase B.3

**Outputs:**
- `docs/glossary.md` (English primary)
- `docs/en/glossary.md` (English variant)
- `docs/de/glossary.md` (German)
- `docs/fr/glossary.md` (French)

**Generation Features:**
1. **Category Organization**: Terms grouped by category with localized headers
2. **Table of Contents**: Auto-generated with category links
3. **Cross-References**: "See also" links between related terms
4. **Wikipedia Links**: Inline links for general-knowledge terms
5. **Source Attribution**: References to original documentation locations
6. **Metadata Headers**: Version, generation date, language indicator

**Output Format (per language):**
```markdown
# Glossary

[Metadata section]

## Table of Contents
- [ThemisDB Core Concepts](#themisdb-core-concepts)
- ...

## ThemisDB Core Concepts

### Entity
Short description...

*Long description with more detail...*

**Aliases:** Entity, Document, Node
**See also:** Collection, Edge
🔗 [Learn more on Wikipedia](...)

*Source: docs/de/glossary.md:18; src/storage/entity.hpp:42*
```

**Usage:**
```bash
python scripts/generate_glossaries.py \
  --input docs/glossary_index.json \
  --output docs/ \
  --languages en,de,fr
```

#### Phase A: Automatic Term-Linking (`build_wiki.py` enhancement)

**Feature:** Automatically inject `[[Term]]` wiki links into generated wiki pages based on glossary terms.

**Inputs:**
- `docs/glossary_index.json` from Phase B.4
- Wiki content pages during `build_wiki.py` processing

**Linking Strategy:**
1. **CAPS-word Detection**: Identifies term candidates (pattern: `[A-Z][A-Z0-9]{2,}`)
2. **Glossary Matching**: Matches detected words against glossary terms (case-insensitive)
3. **Priority Filtering**: Respects `min_priority` to avoid over-linking
4. **Context Awareness**:
   - Preserves code blocks (``` ``` and `` `` syntax)
   - Skips already-linked terms
   - Avoids duplicate links

**CLI Flags:**
- `--enable-term-linking`: Enable automatic term-linking (requires glossary_index.json)
- `--term-link-priority {high|medium|low}`: Minimum term priority to link (default: high)

**Example:**
```bash
python scripts/build_wiki.py \
  --repo-root . \
  --output /tmp/wiki-staging \
  --enable-breadcrumbs \
  --enable-term-linking \
  --term-link-priority high
```

**Before:**
```markdown
MVCC is a concurrency control mechanism. See Transaction for details.
```

**After:**
```markdown
[[MVCC]] is a concurrency control mechanism. See [[Transaction]] for details.
```

## Workflow Integration

### publish-wiki.yml Integration

The `publish-wiki.yml` GitHub Actions workflow automatically runs the glossary pipeline:

```yaml
# Phase 2a0: Generate dynamic glossary
- name: Scan glossary term candidates
  run: python scripts/scan_glossary_terms.py ...

- name: Build enriched glossary index
  run: python scripts/build_glossary_with_copilot.py ...

- name: Generate language-specific glossaries
  run: python scripts/generate_glossaries.py ...

# Phase 2a: Build wiki with term-linking
- name: Build wiki staging
  run: python scripts/build_wiki.py --enable-term-linking ...
```

**Trigger:**
- **Schedule:** Nightly at 03:00 UTC (default)
- **Manual:** Via `workflow_dispatch` with optional inputs

**Outputs:**
- Generated glossaries committed to wiki.git
- Glossary index available for term-linking

### Local Testing

**Full pipeline (dev):**
```bash
# 1. Scan for term candidates
python scripts/scan_glossary_terms.py \
  --repo-root . \
  --output docs/glossary_candidates.json

# 2. Build enriched index
python scripts/build_glossary_with_copilot.py \
  --input docs/glossary_candidates.json \
  --output docs/glossary_index.json

# 3. Generate language-specific glossaries
python scripts/generate_glossaries.py \
  --input docs/glossary_index.json \
  --output docs/

# 4. Build wiki with term-linking
python scripts/build_wiki.py \
  --repo-root . \
  --output /tmp/wiki-staging \
  --enable-term-linking \
  --term-link-priority high
```

## Maintenance & Extension

### Adding New Glossary Terms

**Method 1: Documentation (Recommended)**
Add Doxygen `@term` tags in C++ headers:
```cpp
/// @term MVCC
/// Multi-Version Concurrency Control mechanism.
class TransactionManager { ... };
```

**Method 2: Manual Markdown**
Add terms to `docs/glossary.md` or language-specific files:
```markdown
- MVCC: Multi-Version Concurrency Control (Nebenläufigkeitskontrolle)
```

The scanner will automatically detect both patterns.

### Customizing Categories

Edit `build_glossary_with_copilot.py`:

```python
CATEGORY_HEURISTICS: list[tuple[str, str]] = [
    # Add custom patterns here
    (r"^(MyCustomTerm|AnotherTerm)$", "custom-category"),
]
```

### Extending Multilingual Support

**Add translations in `build_glossary_with_copilot.py`:**

```python
translations: dict[str, dict[str, str]] = {
    "MyTerm": {
        "de": "Mein Begriff",
        "fr": "Mon Terme",
    },
}
```

### Wikipedia Link Mappings

Update `WIKIPEDIA_LINKS` in `build_glossary_with_copilot.py`:

```python
WIKIPEDIA_LINKS: dict[str, str] = {
    "MyCustomTerm": "https://en.wikipedia.org/wiki/My_Custom_Term",
}
```

## Quality Assurance

### Validation Checklist

- [ ] **Scanning**: Run scan and verify `glossary_candidates.json` completeness
- [ ] **Enrichment**: Review `glossary_index.json` for accuracy and categories
- [ ] **Generation**: Verify generated glossaries in `docs/{de,en,fr}/glossary.md`
- [ ] **Term-Linking**: Test auto-linking in wiki pages (check code blocks are preserved)
- [ ] **Cross-References**: Verify "see also" links are relevant
- [ ] **Wikipedia Links**: Validate all general-knowledge term links are correct

### Performance Considerations

- **Scanning**: O(n) where n = number of files; typically < 5 seconds
- **Enrichment**: O(m) where m = number of candidates; typically < 10 seconds
- **Generation**: O(m log m) due to sorting; typically < 2 seconds
- **Term-Linking**: O(t * w) where t = terms, w = wiki pages; typically < 30 seconds with <5% overhead

### Common Issues

**Problem:** Glossary index not found during wiki build
- **Solution:** Ensure `glossary_gen` step completes before `build_wiki` step in workflow

**Problem:** Over-linking (too many terms linked)
- **Solution:** Increase `--term-link-priority` from `low` to `medium` or `high`

**Problem:** Missing translations
- **Solution:** Add translations to `translations` dict in `build_glossary_with_copilot.py`

**Problem:** Code blocks being corrupted during term-linking
- **Solution:** Verify code block pattern in `_inject_term_links()` matches your formatting

## Future Enhancements

### Phase C: Copilot-Powered Enhancement
- Implement full GitHub Models API integration for term descriptions
- Auto-generate German/French translations via LLM
- Add semantic term clustering and "related concepts" suggestions

### Phase D: Analytics & Reporting
- Track term usage across wiki pages
- Generate "most-referenced terms" reports
- Identify orphaned or under-documented terms
- Suggest new terms based on documentation patterns

### Phase E: Interactive Features
- Wiki glossary browser with filtering by category/priority
- Term search across all languages
- Reverse glossary (find terms mentioning a concept)
- Term version history tracking

## See Also

- [DOCUMENTATION_GOVERNANCE.md](../DOCUMENTATION_GOVERNANCE.md) — Documentation standards
- [publish-wiki.yml](../../.github/workflows/publish-wiki.yml) — Wiki build workflow
- [build_wiki.py](../scripts/build_wiki.py) — Wiki page generator
- [ROADMAP.md](../ROADMAP.md) — Product roadmap

## Support

For issues or questions:
1. Check workflow logs in `.github/workflows/publish-wiki.yml`
2. Review glossary artifacts in `docs/glossary_*.json`
3. Run pipeline locally to debug
4. Open issue with `glossary` label on GitHub

---

**Last Updated:** 2026-09-02  
**Maintained By:** ThemisDB Documentation Team
