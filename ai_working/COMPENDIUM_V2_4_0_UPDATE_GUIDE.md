# Compendium v2.4.0 Update & Release Guide

**Date:** 2026-08-13  
**Status:** Ready for v2.4.0 Release Build  
**Version Target:** ThemisDB-Kompendium-v2.4.0  
**Language:** German (Deutsch)

---

## Overview

The ThemisDB Kompendium is a comprehensive German-language handbook (43+ chapters, 7 appendices) that documents the complete platform at a user-friendly level. This guide covers the transition from v1.4.0-alpha to v2.4.0 GA release.

---

## Current State (Pre-v2.4.0)

| Component | Version | Location | Status |
|-----------|---------|----------|--------|
| **HTML Output** | v1.4.0-alpha | `docs/compendium/output/ThemisDB-Kompendium-v1.4.0.html` | ✅ Exists |
| **PDF Output** | v1.4.0-alpha | `docs/compendium/output/ThemisDB-Kompendium-v1.4.0.pdf` | ✅ Exists |
| **mkdocs Config** | v1.4.0-alpha | `docs/compendium/mkdocs-compendium.yml` | ⚠️ Needs update |
| **Source Docs** | Current | `docs/de/` | ✅ v2.4.0-aligned |
| **Chapter Mapping** | Current | `docs/compendium/PHASE3_MAPPING_TABLE.md` | ✅ Valid |

---

## v2.4.0 Updates Required

### 1. mkdocs Configuration Update

**File:** `docs/compendium/mkdocs-compendium.yml`

**Changes needed:**

```diff
- site_description: Umfassendes Kompendium für ThemisDB v1.4.0-alpha
+ site_description: Umfassendes Kompendium für ThemisDB v2.4.0 GA

- output_path: pdf/ThemisDB-Kompendium-v1.4.0-alpha.pdf
+ output_path: pdf/ThemisDB-Kompendium-v2.4.0.pdf

- cover_subtitle: "Das vollständige Handbuch - Version 1.4.0-alpha"
+ cover_subtitle: "Das vollständige Handbuch - Version 2.4.0 GA"
```

### 2. Chapter Content Updates

**High-Priority Chapters (Q3 2026 enriched):**

| Chapter | Updated Sources | Action |
|---------|-----------------|--------|
| Kapitel 17 — LLM Integration | `docs/de/llm/`, `docs/de/lora/`, `docs/de/rag/` | ✅ Ready (sources current) |
| Kapitel 29 — Analytics & Process Mining | `docs/de/analytics/`, `docs/de/process/` | ✅ Ready (sources current) |
| Kapitel 31 — API Protocols | `docs/de/apis/`, `docs/de/rpc_grpc/` | ✅ Ready (sources current) |
| Kapitel 40 — Data Governance & Compliance | `docs/de/compliance/`, `docs/de/governance/` | ✅ Ready (sources current) |

**Action:** Rebuild from current `docs/de/` sources (which are v2.4.0-aligned).

### 3. Feature & Module Highlights for v2.4.0

**New in v2.4.0:**
- **Access Model Coordination Layer** — new `src/access_model/` module with unified tiering and promotion/demotion
- **Phase 6 Documentation** — all modules completed and documented
- **Multi-Model AI/LLM Integration** — hardened and expanded
- **Security Hardening** — GA-ready with sanitizer/penetration-test evidence

**Chapters to emphasize:**
- Kapitel 17: Expand LLM section with new coordination layer details
- Kapitel 20 (wenn vorhanden): Storage tiering with access_model integration
- Kapitel 40: Update compliance section with GA sign-off references

### 4. Cross-Reference Validation

**Before rebuild, verify:**
- [ ] All chapter internal links are valid (no 404s)
- [ ] Cross-references to `docs/de/` sources are current
- [ ] Code examples reference v2.4.0 APIs (not v1.x)
- [ ] Feature claims cite `src/*/ROADMAP.md` v2.4.0 entries

---

## Build & Release Process

### Prerequisites

1. **mkdocs-material** installed (`pip install mkdocs-material`)
2. **mkdocs-with-pdf** installed (`pip install mkdocs-with-pdf`)
3. **Pandoc** installed (for LaTeX → PDF conversion)

### Build Steps

```bash
cd docs/compendium

# 1. Update mkdocs-compendium.yml (see section 1 above)
# 2. Build HTML output
mkdocs build -f mkdocs-compendium.yml -d output

# 3. The -f flag applies the with-pdf plugin, generating PDF automatically
# Output: output/ThemisDB-Kompendium-v2.4.0.pdf
```

### Output Location

After build:
- **HTML:** `docs/compendium/output/ThemisDB-Kompendium-v2.4.0.html`
- **PDF:** `docs/compendium/output/ThemisDB-Kompendium-v2.4.0.pdf`

---

## Quality Checklist (Before Release)

- [ ] mkdocs config updated (version strings)
- [ ] Chapter sources verified from v2.4.0 docs/de/
- [ ] Cross-references validated (no broken links)
- [ ] PDF generation successful (no encoding errors)
- [ ] Version string in cover/title page correct (v2.4.0 GA)
- [ ] Table of contents generated correctly
- [ ] Code snippets reviewed for v2.4.0 API accuracy
- [ ] German language quality review (optional)

---

## Deployment

### To GitHub Pages (if automated):

1. Commit updated `output/` folder to repository
2. GitHub Pages will serve from `docs/compendium/output/`

### For Release Artifacts:

1. Archive both HTML and PDF as release attachments
2. Link in RELEASE_NOTES_v2.4.0.md
3. Update compendium access URL in README.md

---

## Optional Post-Release

- Archive v1.4.0 outputs to `docs/compendium/archive/v1.4.0/`
- Create v2.4.0 → v2.5.0 upgrade guide (future)
- Set up automated compendium rebuilds in CI/CD

---

## Related Documentation

- **Source Governance:** `DOCUMENTATION_GOVERNANCE.md` §2.0a
- **Mapping Table:** `docs/compendium/PHASE3_MAPPING_TABLE.md`
- **Development Guide:** `docs/compendium/CHAPTER_GENERATION_GUIDE.md`
- **Current Roadmap:** `docs/compendium/PHASE3_ROADMAP.md`

---

**Last Updated:** 2026-08-13  
**Prepared By:** Documentation Consolidation Agent  
**Status:** Ready for v2.4.0 Release Build
