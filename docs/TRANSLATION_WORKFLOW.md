# Documentation Translation Workflow

## Overview

This document describes the workflow for translating ThemisDB documentation from German to English.

## Current State

- **Total Documents**: 686 markdown files in `/docs` directory
- **Current Language**: Primarily German (with some English documents)
- **Target**: Bilingual documentation (German + English)

## Translation Priorities

### Priority 1: Core User-Facing Documents (Immediate)
These are the most critical documents that users interact with first:

1. `/docs/README.md` - Main documentation index
2. `/docs/DOCUMENTATION_INDEX.md` - Documentation navigation
3. `/docs/INDEX.md` - Index page
4. `/docs/QUICK_REFERENCE.md` - Quick reference guide
5. `/docs/Home.md` - Home page (already in English)

### Priority 2: Getting Started & Guides (High Priority)
6. `/docs/guides/guides_quick_start.md` - Quick start guide
7. `/docs/guides/guides_installation.md` - Installation guide
8. `/docs/guides/guides_build.md` - Build guide
9. `/docs/guides/guides_build_strategy.md` - Build strategy
10. `/docs/deployment/deployment_overview.md` - Deployment overview

### Priority 3: API & Query Documentation (High Priority)
11. `/docs/aql/aql_syntax.md` - AQL syntax reference
12. `/docs/aql/aql_query_engine.md` - Query engine documentation
13. `/docs/api/*.md` - All API documentation files
14. `/docs/apis/*.md` - API specifications

### Priority 4: Architecture & Core Concepts (Medium Priority)
15. `/docs/architecture/*.md` - Architecture documentation
16. `/docs/storage/*.md` - Storage layer documentation
17. `/docs/transaction/*.md` - Transaction documentation

### Priority 5: Security & Compliance (Medium Priority)
18. `/docs/security/*.md` - Security documentation
19. `/docs/compliance/*.md` - Compliance documentation
20. `/docs/auth/*.md` - Authentication documentation

### Priority 6: Features & LLM Integration (Medium Priority)
21. `/docs/llm/*.md` - LLM integration documentation
22. `/docs/features/*.md` - Feature documentation
23. `/docs/plugins/*.md` - Plugin documentation

### Priority 7: Enterprise & Performance (Lower Priority)
24. `/docs/enterprise/*.md` - Enterprise features
25. `/docs/performance/*.md` - Performance documentation

### Priority 8: Development & Source Documentation (Lower Priority)
26. `/docs/development/*.md` - Development guides
27. `/docs/src/*.md` - Source code documentation

### Priority 9: Archive & Release Notes (Lowest Priority)
28. `/docs/archive/*.md` - Archived documents
29. `/docs/release_notes/*.md` - Release notes

## Translation Strategy

### Approach
1. **Bilingual Structure**: Keep both German and English versions
   - German files in `docs/de/` directory
   - English files in `docs/en/` directory
   - Root level docs remain for backward compatibility

2. **Incremental Translation**: Translate documents in priority order
   - Start with Priority 1 (Core User-Facing)
   - Progress through priorities based on user needs

3. **Consistency**: Maintain consistent terminology
   - Create a glossary of technical terms
   - Use consistent translation for database concepts

4. **Quality Control**: Review translations for accuracy
   - Technical accuracy is critical
   - Maintain formatting and structure
   - Preserve code examples and commands

## File Naming Convention

### Directory-based Structure (Implemented)
- German: `docs/de/filename.md`
- English: `docs/en/filename.md`

**Benefits:**
- Clear language separation
- Easier to maintain separate language versions
- Standard approach for multi-language documentation
- Simpler for automation and tooling

## Translation Progress Tracking

Track progress in `/docs/TRANSLATION_STATUS.md`:
- Total documents: 686
- Documents translated: [count]
- Translation percentage: [percentage]
- Last updated: [date]

## Technical Terms Glossary

Key terms to translate consistently:

| German | English |
|--------|---------|
| Dokumentation | Documentation |
| Übersicht | Overview |
| Anleitung | Guide |
| Schnelleinstieg | Quick Start |
| Architektur | Architecture |
| Sicherheit | Security |
| Verschlüsselung | Encryption |
| Abfrage | Query |
| Transaktion | Transaction |
| Leistung | Performance |
| Bereitstellung | Deployment |
| Entwicklung | Development |

## Next Steps

1. Create initial English translations for Priority 1 documents
2. Update mkdocs.yml to support bilingual documentation
3. Create TRANSLATION_STATUS.md to track progress
4. Begin systematic translation of remaining documents
