# chaos Module - Developer Gap Note

> Auto-generated from ai_working/gap_scan_v3_aggregate.json.
> This file is overwritten on each regeneration.

## Scan Snapshot

- Module: chaos
- Generated: 2026-06-02 11:09:12
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
| container | 1 |
| exception_safety | 1 |
| performance | 1 |
| performance_patterns | 1 |

## File Overview

| File | Findings | Critical | High | Medium | Low |
|---|---:|---:|---:|---:|---:|
| src/chaos/chaos_framework.cpp | 1 | 0 | 0 | 1 | 0 |

## Full Scanner Findings

### src/chaos/chaos_framework.cpp
Total findings: 1

- Line 154: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(af);
  Confidence: band=high; score=0.74

## Update Workflow

- Refresh scan artifacts with: python tools/gap_scanner_v3.py
- Regenerate all module notes with: python tools/module_doc_generator.py . ai_working ai_working/module_gaps
- The generator mirrors each archive document directly into src/<module>/MODULE_GAPS.md.

Format: THEMIS_MODULE_GAPS_V3
