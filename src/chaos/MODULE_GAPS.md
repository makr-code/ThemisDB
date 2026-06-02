# chaos Module - Developer Gap Note

> Auto-generated from ai_working/gap_scan_v3_aggregate.json.
> This file is overwritten on each regeneration.

## Scan Snapshot

- Module: chaos
- Generated: 2026-06-02 12:40:50
- Status: Critical Findings Present
- Total Findings: 4
- Actionable Findings (Critical + High): 2
- Affected Files: 1

## Severity Summary

| Severity | Count |
|---|---:|
| Critical | 1 |
| High | 1 |
| Medium | 2 |
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
| src/chaos/chaos_framework.cpp | 4 | 1 | 1 | 2 | 0 |

## Full Scanner Findings

### src/chaos/chaos_framework.cpp
Total findings: 4

- Line 0: severity=CRITICAL; category=uncategorized
  Confidence: band=very_high; score=0.85
- Line 247: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Acquire lock before loop or redesign to minimize lock time
  Context: std::lock_guard<std::mutex> lock(sched_mutex_);
- Line 154: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(af);
  Confidence: band=high; score=0.74
- Line 250: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: to_fire.push_back(e.fault);

## Update Workflow

- Refresh scan artifacts with: python tools/gap_scanner_v3.py
- Regenerate all module notes with: python tools/module_doc_generator.py . ai_working ai_working/module_gaps
- The generator mirrors each archive document directly into src/<module>/MODULE_GAPS.md.

Format: THEMIS_MODULE_GAPS_V3
