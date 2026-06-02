# ai Module - Developer Gap Note

> Auto-generated from ai_working/gap_scan_v3_aggregate.json.
> This file is overwritten on each regeneration.

## Scan Snapshot

- Module: ai
- Generated: 2026-06-02 12:40:50
- Status: Critical Findings Present
- Total Findings: 24
- Actionable Findings (Critical + High): 18
- Affected Files: 2

## Severity Summary

| Severity | Count |
|---|---:|
| Critical | 2 |
| High | 16 |
| Medium | 6 |
| Low | 0 |

## Category Summary

| Category | Count |
|---|---:|
| llm_ai_safety | 8 |
| memory | 6 |
| performance_patterns | 4 |
| audit_logging | 2 |
| determinism | 2 |
| container | 1 |
| reliability | 1 |

## File Overview

| File | Findings | Critical | High | Medium | Low |
|---|---:|---:|---:|---:|---:|
| src/ai/ai_plugin_generator.cpp | 12 | 1 | 8 | 3 | 0 |
| src/ai/cai_ethics_integration.cpp | 12 | 1 | 8 | 3 | 0 |

## Full Scanner Findings

### src/ai/ai_plugin_generator.cpp
Total findings: 12

- Line 5: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: * PR History (last 5): #5409 ai: complete Q4-2026 harden... (2026-06-01) | #5205 fix(llm): harden LoRA input... (2026-05-23) | #4827 refactor: flatten plugin/ h... (2026-05-04)
  Confidence: band=very_high; score=0.99
- Line 5: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: * PR History (last 5): #5409 ai: complete Q4-2026 harden... (2026-06-01) | #5205 fix(llm): harden LoRA input... (2026-05-23) | #4827 refactor: flatten plugin/ h... (2026-05-04)
  Confidence: band=very_high; score=0.9
- Line 232: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: // 1. Validate inputs first.
  Confidence: band=very_high; score=0.9
- Line 232: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: // 1. Validate inputs first.
  Confidence: band=very_high; score=0.9
- Line 266: severity=HIGH; category=no_retry_logic
  Description: http_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: const std::string request_body = request.dump();
- Line 299: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: std::this_thread::sleep_for(std::chrono::milliseconds(100 * (1 << attempt)));
- Line 378: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: generated.manifest.description = generated.manifest.description.substr(0, kMaxDescLen);
  Confidence: band=very_high; score=0.9
- Line 411: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: auto safety_result = config_.c1_cai_eval_fn(generated.implementation_code, safe_description);
  Confidence: band=very_high; score=0.9
- Line 450: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: auto sandbox_result = config_.sandbox_verify_fn(generated);
  Confidence: band=very_high; score=0.9
- Line 196: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> unique_capabilities;
  Confidence: band=medium; score=0.66
- Line 211: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> unique_dependencies;
  Confidence: band=medium; score=0.66
- Line 388: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: generated.build_dependencies.push_back(std::move(dep_str));
  Confidence: band=high; score=0.74

### src/ai/cai_ethics_integration.cpp
Total findings: 12

- Line 180: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: // --- 2. Build EthicsEvaluator inputs from CAI result ---
  Confidence: band=very_high; score=0.99
- Line 180: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: // --- 2. Build EthicsEvaluator inputs from CAI result ---
  Confidence: band=very_high; score=0.9
- Line 180: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: // --- 2. Build EthicsEvaluator inputs from CAI result ---
  Confidence: band=very_high; score=0.9
- Line 280: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: decision.metadata["query"]  = query;
- Line 281: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: decision.metadata["evaluation_framework"] = "constitutional_ai";
- Line 282: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: decision.metadata["formalized_principles"] = joinValues(formalized_principles);
- Line 283: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: decision.metadata["formalized_domains"] = joinValues(formalized_domains);
- Line 315: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: arg.metadata["constitutional_principle_id"] = pid;
- Line 316: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: arg.metadata["ethics_domain"] = arg.philosophy_school;
- Line 115: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: chain_ids.push_back("constitutional_chain:" + domain);
  Confidence: band=high; score=0.74
- Line 303: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: args.push_back(std::move(arg));
  Confidence: band=high; score=0.74
- Line 316: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: args.push_back(std::move(arg));
  Confidence: band=high; score=0.74

## Update Workflow

- Refresh scan artifacts with: python tools/gap_scanner_v3.py
- Regenerate all module notes with: python tools/module_doc_generator.py . ai_working ai_working/module_gaps
- The generator mirrors each archive document directly into src/<module>/MODULE_GAPS.md.

Format: THEMIS_MODULE_GAPS_V3
