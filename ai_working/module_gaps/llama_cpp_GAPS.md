# llama_cpp Module - Developer Gap Note

> Auto-generated from ai_working/gap_scan_v3_aggregate.json.
> This file is overwritten on each regeneration.

## Scan Snapshot

- Module: llama_cpp
- Generated: 2026-06-02 11:09:13
- Status: Findings Present
- Total Findings: 9
- Actionable Findings (Critical + High): 0
- Affected Files: 3

## Severity Summary

| Severity | Count |
|---|---:|
| Critical | 0 |
| High | 0 |
| Medium | 9 |
| Low | 0 |

## Category Summary

| Category | Count |
|---|---:|
| reliability | 20 |
| performance_patterns | 9 |
| concurrency | 4 |
| raii | 2 |
| container | 1 |
| memory | 1 |
| platform | 1 |

## File Overview

| File | Findings | Critical | High | Medium | Low |
|---|---:|---:|---:|---:|---:|
| src/llama_cpp/llama_cpp_plugin.cpp | 6 | 0 | 0 | 6 | 0 |
| src/llama_cpp/tests/test_llama_cpp_plugin.cpp | 3 | 0 | 0 | 3 | 0 |
| src/llama_cpp/llama_cpp_registrar.cpp | 0 | 0 | 0 | 0 | 0 |

## Full Scanner Findings

### src/llama_cpp/llama_cpp_plugin.cpp
Total findings: 6

- Line 167: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(std::move(info));
  Confidence: band=high; score=0.74
- Line 330: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: chunks.push_back(std::move(chunk));
  Confidence: band=high; score=0.74
- Line 629: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.tokens.push_back(token_id);
  Confidence: band=high; score=0.74
- Line 665: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.tokens.push_back(token_id);
  Confidence: band=high; score=0.74
- Line 678: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.tokens.push_back(token_id);
  Confidence: band=high; score=0.74
- Line 707: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results.push_back(generate(req));
  Confidence: band=high; score=0.74

### src/llama_cpp/tests/test_llama_cpp_plugin.cpp
Total findings: 3

- Line 573: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: threads.emplace_back(worker);
  Confidence: band=high; score=0.74
- Line 610: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: threads.emplace_back(worker);
  Confidence: band=high; score=0.74
- Line 649: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: threads.emplace_back(lora_writer, t);
  Confidence: band=high; score=0.74

### src/llama_cpp/llama_cpp_registrar.cpp
Total findings: 0

## Update Workflow

- Refresh scan artifacts with: python tools/gap_scanner_v3.py
- Regenerate all module notes with: python tools/module_doc_generator.py . ai_working ai_working/module_gaps
- The generator mirrors each archive document directly into src/<module>/MODULE_GAPS.md.

Format: THEMIS_MODULE_GAPS_V3
