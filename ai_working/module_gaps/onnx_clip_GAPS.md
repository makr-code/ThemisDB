# onnx_clip Module - Developer Gap Note

> Auto-generated from ai_working/gap_scan_v3_aggregate.json.
> This file is overwritten on each regeneration.

## Scan Snapshot

- Module: onnx_clip
- Generated: 2026-06-02 11:09:13
- Status: Findings Present
- Total Findings: 4
- Actionable Findings (Critical + High): 0
- Affected Files: 1

## Severity Summary

| Severity | Count |
|---|---:|
| Critical | 0 |
| High | 0 |
| Medium | 4 |
| Low | 0 |

## Category Summary

| Category | Count |
|---|---:|
| performance_patterns | 4 |
| concurrency | 3 |
| raii | 3 |

## File Overview

| File | Findings | Critical | High | Medium | Low |
|---|---:|---:|---:|---:|---:|
| src/onnx_clip/onnx_clip_plugin.cpp | 4 | 0 | 0 | 4 | 0 |

## Full Scanner Findings

### src/onnx_clip/onnx_clip_plugin.cpp
Total findings: 4

- Line 150: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: tokens.push_back(token);
  Confidence: band=high; score=0.74
- Line 424: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results.push_back(std::move(result));
  Confidence: band=high; score=0.74
- Line 435: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results.push_back(impl_->computeEmbedding(images[i], nullptr,
  Confidence: band=high; score=0.74
- Line 435: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results.push_back(impl_->computeEmbedding(images[i], nullptr,
  Confidence: band=high; score=0.74

## Update Workflow

- Refresh scan artifacts with: python tools/gap_scanner_v3.py
- Regenerate all module notes with: python tools/module_doc_generator.py . ai_working ai_working/module_gaps
- The generator mirrors each archive document directly into src/<module>/MODULE_GAPS.md.

Format: THEMIS_MODULE_GAPS_V3
