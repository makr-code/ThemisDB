# ai Module - Developer Gap Note

> Auto-generated from ai_working/gap_scan_v3_aggregate.json.
> This file is overwritten on each regeneration.

## Scan Snapshot

- Module: ai
- Generated: 2026-05-31 08:50:11
- Status: Partially Remediated (2026-06-01)
- Total Findings: 8
- Actionable Findings (Critical + High): 6
- Affected Files: 1

## Severity Summary

| Severity | Count |
|---|---:|
| Critical | 0 |
| High | 6 |
| Medium | 2 |
| Low | 0 |

## Category Summary

| Category | Count |
|---|---:|
| llm_ai_safety | 2 |
| audit_logging | 1 |
| container | 1 |
| memory | 1 |
| performance_patterns | 1 |
| reliability | 1 |
| security | 1 |

## File Overview

| File | Findings | Critical | High | Medium | Low |
|---|---:|---:|---:|---:|---:|
| src/ai/ai_plugin_generator.cpp | 8 | 0 | 6 | 2 | 0 |

## Full Scanner Findings

### src/ai/ai_plugin_generator.cpp
Total findings: 8

- Line 27: severity=HIGH; category=null_dereference — **FIXED** (2026-06-01): Added null guard for `ptr`/`userdata` and size_t overflow protection in `curlWriteCallback`.
- Line 27: severity=HIGH; category=pointer_arithmetic — **FIXED** (2026-06-01): Overflow-safe `size * nmemb` guard added to `curlWriteCallback`.
- Line 123: severity=HIGH; category=audit_logging; pattern=hardcoded_output — **RESOLVED** (false positive): `spdlog::debug()` is used; no bare `std::cout/printf` present.
- Line 123: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input — **FIXED** (2026-06-01): `sanitizeText()` lambda strips ASCII control characters from `prompt.description` before constructing the LLM request.
- Line 142: severity=HIGH; category=no_retry_logic — **FIXED** (2026-06-01): 3-attempt retry loop with 100 ms → 400 ms exponential back-off added around `invokeEndpointWithCurl` / `endpoint_invoke_fn`.
- Line 172: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output — **FIXED** (2026-06-01): Added 1 MiB per-field size guard and 256-char name-length guard on LLM response fields before populating `GeneratedPlugin`.
- Line 179: severity=MEDIUM; category=performance; pattern=missing_vector_reserve — **FIXED** (2026-06-01): `generated.build_dependencies.reserve(deps_arr.size())` added before push_back loop.
- Line 180: severity=MEDIUM; category=copy_overhead — **FIXED** (2026-06-01): Same `reserve()` call resolves reallocation concern.

## Update Workflow

- Refresh scan artifacts with: python tools/gap_scanner_v3.py
- Regenerate all module notes with: python tools/module_doc_generator.py . ai_working ai_working/module_gaps
- The generator mirrors each archive document directly into src/<module>/MODULE_GAPS.md.

Format: THEMIS_MODULE_GAPS_V3
