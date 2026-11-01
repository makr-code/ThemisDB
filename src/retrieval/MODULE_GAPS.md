---
Author: "ThemisDB Contributors"
Created: "2026-06-04"
Last Updated: "2026-10-31"
Status: "archived"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "ThemisDB documentation orchestration assistant"
Human_Review: false
Human_Author: "ThemisDB Contributors"
Source_Level: "level1"
SOT_Domain: "module-behavior"
---

Exact model backend not verifiable from final document text alone.

# Historical generated gap-scan snapshot — 2026-06-04

> This snapshot is retained for history only. It is not a current scan: the
> listed finding that `PRODUCTION_REQUIREMENTS.md` is missing is stale, and
> the counts/status below have not been regenerated or verified.

> Auto-generated from ai_working\gap_scan_results.json.
> This file is overwritten on each regeneration.

## Scan Snapshot

- Module: retrieval
- Generated: 2026-06-04 08:50:22
- Status: Findings Present
- Total Findings: 1
- Actionable Findings (Critical + High): 0
- Affected Files: 1

## Severity Summary

| Severity | Count |
|---|---:|
| Critical | 0 |
| High | 0 |
| Medium | 1 |
| Low | 0 |

## Category Summary

| Category | Count |
|---|---:|
| missing_module_doc | 1 |

## File Overview

| File | Findings | Critical | High | Medium | Low |
|---|---:|---:|---:|---:|---:|
| retrieval | 1 | 0 | 0 | 1 | 0 |

## Full Scanner Findings

### retrieval
Total findings: 1

- Line 1: severity=MEDIUM; category=missing_module_doc
  Description: Module 'retrieval' missing required governance doc 'PRODUCTION_REQUIREMENTS.md'
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::themis_module_governance_rules
  Context: Expected file: src/retrieval/PRODUCTION_REQUIREMENTS.md

## Update Workflow

- Refresh scanner artifacts with: python tools/gs3_orchestrator.py ./src --output ai_working/gap_scan_results.json
- Regenerate docs with: python tools/module_doc_generator.py . ai_working ai_working/module_gaps
- Add --no-mirror when you only want archive docs in ai_working/module_gaps.

Format: THEMIS_MODULE_GAPS_V4
