# ai Module - Developer Gap Note

> Auto-generated from ai_working\gap_scan_results.json.
> This file is overwritten on each regeneration.

## Scan Snapshot

- Module: ai
- Generated: 2026-06-04 08:50:21
- Status: High-Priority Findings Present
- Total Findings: 19
- Actionable Findings (Critical + High): 12
- Affected Files: 4

## Severity Summary

| Severity | Count |
|---|---:|
| Critical | 0 |
| High | 12 |
| Medium | 3 |
| Low | 4 |

## Category Summary

| Category | Count |
|---|---:|
| pointer_arithmetic_unbounded | 8 |
| hardcoded_output | 2 |
| module_doc_linkset_drift | 2 |
| unordered_container_iter | 2 |
| unvalidated_llm_output | 2 |
| no_retry_logic | 1 |
| primitive_no_volatile | 1 |
| range_temporary | 1 |

## File Overview

| File | Findings | Critical | High | Medium | Low |
|---|---:|---:|---:|---:|---:|
| ai/cai_ethics_integration.cpp | 9 | 0 | 8 | 0 | 1 |
| ai/ai_plugin_generator.cpp | 8 | 0 | 4 | 3 | 1 |
| ai/FUTURE_ENHANCEMENTS.md | 1 | 0 | 0 | 0 | 1 |
| ai/PRODUCTION_REQUIREMENTS.md | 1 | 0 | 0 | 0 | 1 |

## Full Scanner Findings

### ai/cai_ethics_integration.cpp
Total findings: 9

- Line 280: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: decision.argument_chain_ids = argument_chain_ids;

    decision.confidence         = static_cast<double>(cai_result.revised_score);

    decision.consensus_level    = cai_result.violated_principles.empty() ? 1.0 : 0.0;

    decision.metadata["query"]  = query;

    decision.metadata["evaluation_framework"] = "constitutional_ai";

    decision.metadata["formalized_principles"] = joinValues(formalized_principles);

    decision.metadata["formalized_domains"] = joinValues(formalized_domains);
- Line 281: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: decision.confidence         = static_cast<double>(cai_result.revised_score);

    decision.consensus_level    = cai_result.violated_principles.empty() ? 1.0 : 0.0;

    decision.metadata["query"]  = query;

    decision.metadata["evaluation_framework"] = "constitutional_ai";

    decision.metadata["formalized_principles"] = joinValues(formalized_principles);

    decision.metadata["formalized_domains"] = joinValues(formalized_domains);

    return decision;
- Line 282: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: decision.consensus_level    = cai_result.violated_principles.empty() ? 1.0 : 0.0;

    decision.metadata["query"]  = query;

    decision.metadata["evaluation_framework"] = "constitutional_ai";

    decision.metadata["formalized_principles"] = joinValues(formalized_principles);

    decision.metadata["formalized_domains"] = joinValues(formalized_domains);

    return decision;

}
- Line 283: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: decision.metadata["query"]  = query;

    decision.metadata["evaluation_framework"] = "constitutional_ai";

    decision.metadata["formalized_principles"] = joinValues(formalized_principles);

    decision.metadata["formalized_domains"] = joinValues(formalized_domains);

    return decision;

}
- Line 302: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: arg.content          = "Principle applied: " + pid;

        arg.strength         = plugins::ethics::ArgumentStrength::STRONG;

        arg.principle_basis  = {pid};

        arg.metadata["constitutional_principle_id"] = pid;

        arg.metadata["ethics_domain"] = arg.philosophy_school;

        args.push_back(std::move(arg));

    }
- Line 303: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: arg.strength         = plugins::ethics::ArgumentStrength::STRONG;

        arg.principle_basis  = {pid};

        arg.metadata["constitutional_principle_id"] = pid;

        arg.metadata["ethics_domain"] = arg.philosophy_school;

        args.push_back(std::move(arg));

    }
- Line 315: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: arg.content          = "Principle violated: " + pid;

        arg.strength         = plugins::ethics::ArgumentStrength::MODERATE;

        arg.principle_basis  = {pid};

        arg.metadata["constitutional_principle_id"] = pid;

        arg.metadata["ethics_domain"] = arg.philosophy_school;

        args.push_back(std::move(arg));

    }
- Line 316: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: arg.strength         = plugins::ethics::ArgumentStrength::MODERATE;

        arg.principle_basis  = {pid};

        arg.metadata["constitutional_principle_id"] = pid;

        arg.metadata["ethics_domain"] = arg.philosophy_school;

        args.push_back(std::move(arg));

    }
- Line 180: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: // --- 2. Build EthicsEvaluator inputs from CAI result ---

### ai/ai_plugin_generator.cpp
Total findings: 8

- Line 266: severity=HIGH; category=no_retry_logic
  Description: RPC/network call without retry logic — transient failures will propagate
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: request["security_level"] = static_cast<int>(prompt.security_level);

    request["generate_tests"] = prompt.generate_tests;

    request["generate_docs"] = prompt.generate_docs;

    const std::string request_body = request.dump();

    if (request_body.size() > config_.max_request_body_bytes) {

        ++stat_validation_errors_;

        return tl::unexpected(
- Line 299: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: std::this_thread::sleep_for(std::chrono::milliseconds(100 * (1 << attempt)));
- Line 411: severity=HIGH; category=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: auto safety_result = config_.c1_cai_eval_fn(generated.implementation_code, safe_description);
- Line 450: severity=HIGH; category=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: auto sandbox_result = config_.sandbox_verify_fn(generated);
- Line 196: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_set<std::string> unique_capabilities;
- Line 211: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_set<std::string> unique_dependencies;
- Line 289: severity=MEDIUM; category=primitive_no_volatile
  Description: Primitive shared across threads without volatile
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: for (int attempt = 0; attempt < kMaxRetries; ++attempt) {
- Line 232: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: // 1. Validate inputs first.

### ai/FUTURE_ENHANCEMENTS.md
Total findings: 1

- Line 1: severity=LOW; category=module_doc_linkset_drift
  Description: Module doc 'FUTURE_ENHANCEMENTS.md' is missing expected cross-links: ARCHITECTURE.md
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::themis_doc_freshness_rules
  Context: Refresh the top-level <!-- Links: ... --> metadata to match the current module doc set

### ai/PRODUCTION_REQUIREMENTS.md
Total findings: 1

- Line 1: severity=LOW; category=module_doc_linkset_drift
  Description: Module doc 'PRODUCTION_REQUIREMENTS.md' is missing expected cross-links: FUTURE_ENHANCEMENTS.md, README.md, ROADMAP.md
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::themis_doc_freshness_rules
  Context: Refresh the top-level <!-- Links: ... --> metadata to match the current module doc set

## Update Workflow

- Refresh scanner artifacts with: python tools/gs3_orchestrator.py ./src --output ai_working/gap_scan_results.json
- Regenerate docs with: python tools/module_doc_generator.py . ai_working ai_working/module_gaps
- Add --no-mirror when you only want archive docs in ai_working/module_gaps.

Format: THEMIS_MODULE_GAPS_V4
