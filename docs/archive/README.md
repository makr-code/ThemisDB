# Archive - Root Documentation

**Last Updated:** 2026-01-12  
**Version:** 1.0

---

## 📋 Overview

This directory contains archived language-agnostic documentation that is no longer current but preserved for historical reference and context.

## 📂 Archive Structure

```
docs/archive/
├── README.md                    # This file
├── ARCHIVE_NOTE_TEMPLATE.md    # Template for archive notes
└── [archived-documents.md]      # Archived files
```

For language-specific archives, see:
- German: [docs/de/archive/](../de/archive/)
- English: [docs/en/archive/](../en/archive/) (to be created)
- French: [docs/fr/archive/](../fr/archive/) (to be created)
- Spanish: [docs/es/archive/](../es/archive/) (to be created)
- Japanese: [docs/ja/archive/](../ja/archive/) (to be created)

## 📚 Archived Documents

| Document | Archived Date | Reason | Replaced By |
|----------|---------------|--------|-------------|
| PITR_IMPLEMENTATION_COMPLETE.md | 2026-01-12 | Implementation completed | [PITR Feature Guide](../en/features/features_pitr.md) |
| NLP_INTEGRATION_PHASE1_COMPLETE.md | 2026-01-12 | Phase 1 completed | [NLP Features](../en/features/) |
| NLP_INTEGRATION_PHASE2_COMPLETE.md | 2026-01-12 | Phase 2 completed | [NLP Features](../en/features/) |
| NLP_INTEGRATION_FINAL_SUMMARY.md | 2026-01-12 | Implementation completed | [NLP Features](../en/features/) |
| DURABILITY_TESTING_COMPLETE.md | 2026-01-12 | Testing completed | [Testing Docs](../testing/) |
| LORA_IMPLEMENTATION_SUMMARY.md | 2026-01-12 | Implementation completed | [LoRA Docs](../../LORA_DOCUMENTATION_SUMMARY.md) |
| LORA_API_IMPLEMENTATION_SUMMARY.md | 2026-01-12 | API implemented | [API Reference](../../API_REFERENCE.md) |
| LORA_AQL_IMPLEMENTATION_SUMMARY.md | 2026-01-12 | AQL functions implemented | [LoRA AQL Reference](../../LORA_AQL_REFERENCE.md) |
| LORA_HELP_INTEGRATION_SUMMARY.md | 2026-01-12 | Help integration completed | [LoRA Docs](../../LORA_DOCUMENTATION_SUMMARY.md) |
| THEMIS_HELP_LORA_IMPLEMENTATION_SUMMARY.md | 2026-01-12 | Feature completed | [LoRA Docs](../../LORA_DOCUMENTATION_SUMMARY.md) |
| KERBEROS_IMPLEMENTATION_SUMMARY.md | 2026-01-12 | Authentication implemented | [Security Docs](../../SECURITY.md) |

## ℹ️ About Archives

### Purpose

Archives preserve outdated or superseded documentation for:
- Historical reference
- Understanding past decisions
- Tracking evolution of the project
- Maintaining git history

### When Documents Are Archived

Documentation is archived when:
- Content is outdated and superseded by newer documentation
- Feature/component no longer exists or has been replaced
- Information has been consolidated into another document
- Content is no longer accurate or relevant
- Implementation summaries from completed work

### Using Archived Documentation

⚠️ **Warning:** Archived documents may contain outdated information. Always refer to current documentation for development and deployment.

Each archived document includes an archive note with:
- Date of archival
- Reason for archival
- Link to replacement documentation (if applicable)
- Historical context

## 📖 Current Documentation

For up-to-date documentation, see:
- [Documentation Index](../DOCUMENTATION_INDEX.md)
- [00_DOCUMENTATION_INDEX.md](../00_DOCUMENTATION_INDEX.md)
- [README.md](../README.md)

## 🔄 Archival Process

To archive a document, follow the [Documentation Archival Process](../DOCUMENTATION_ARCHIVAL_PROCESS.md).

Key steps:
1. Assess document for archival
2. Extract valuable content to preserve
3. Create archive note using template
4. Move document with `git mv` (preserves history)
5. Update all references
6. Update this README

## 📝 Archive Policy

See [CONTRIBUTING.md § Documentation Archival](../../CONTRIBUTING.md#documentation-archival) for the complete policy.

---

**Note:** This archive follows ThemisDB's documentation archival policy established in 2026-01-12.
