# stable_diffusion Module - Developer Gap Note

> Auto-generated from ai_working/gap_scan_v3_aggregate.json.
> This file is overwritten on each regeneration.

## Scan Snapshot

- Module: stable_diffusion
- Generated: 2026-06-02 11:09:13
- Status: Findings Present
- Total Findings: 5
- Actionable Findings (Critical + High): 0
- Affected Files: 4

## Severity Summary

| Severity | Count |
|---|---:|
| Critical | 0 |
| High | 0 |
| Medium | 5 |
| Low | 0 |

## Category Summary

| Category | Count |
|---|---:|
| container | 7 |
| memory | 5 |
| performance_patterns | 5 |
| exception_safety | 3 |
| raii | 2 |
| reliability | 2 |
| type_conversion | 2 |
| uninitialized | 1 |

## File Overview

| File | Findings | Critical | High | Medium | Low |
|---|---:|---:|---:|---:|---:|
| src/stable_diffusion/sd_plugin.cpp | 4 | 0 | 0 | 4 | 0 |
| src/stable_diffusion/sd_prompt_sanitizer.cpp | 1 | 0 | 0 | 1 | 0 |
| src/stable_diffusion/sd_plugin_registrar.cpp | 0 | 0 | 0 | 0 | 0 |
| src/stable_diffusion/tests/test_sd_plugin.cpp | 0 | 0 | 0 | 0 | 0 |

## Full Scanner Findings

### src/stable_diffusion/sd_plugin.cpp
Total findings: 4

- Line 110: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: v.push_back(static_cast<uint8_t>(x >> 24));
  Confidence: band=high; score=0.74
- Line 110: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: v.push_back(static_cast<uint8_t>(x >> 24));
  Confidence: band=high; score=0.74
- Line 154: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: idat_payload.push_back(0x78u);
  Confidence: band=high; score=0.74
- Line 288: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results.push_back(generateLocked(p, cfg));
  Confidence: band=high; score=0.74

### src/stable_diffusion/sd_prompt_sanitizer.cpp
Total findings: 1

- Line 32: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: if (!lower.empty()) blocked_keywords_.push_back(lower);
  Confidence: band=high; score=0.74

### src/stable_diffusion/sd_plugin_registrar.cpp
Total findings: 0


### src/stable_diffusion/tests/test_sd_plugin.cpp
Total findings: 0

## Update Workflow

- Refresh scan artifacts with: python tools/gap_scanner_v3.py
- Regenerate all module notes with: python tools/module_doc_generator.py . ai_working ai_working/module_gaps
- The generator mirrors each archive document directly into src/<module>/MODULE_GAPS.md.

Format: THEMIS_MODULE_GAPS_V3
