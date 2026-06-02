# prompt_engineering Module - Developer Gap Note

> Auto-generated from ai_working/gap_scan_v3_aggregate.json.
> This file is overwritten on each regeneration.

## Scan Snapshot

- Module: prompt_engineering
- Generated: 2026-06-02 11:09:13
- Status: Critical Findings Present
- Total Findings: 251
- Actionable Findings (Critical + High): 82
- Affected Files: 31

## Severity Summary

| Severity | Count |
|---|---:|
| Critical | 23 |
| High | 59 |
| Medium | 165 |
| Low | 0 |

## Category Summary

| Category | Count |
|---|---:|
| performance_patterns | 127 |
| container | 102 |
| llm_ai_safety | 65 |
| security | 36 |
| audit_logging | 33 |
| reliability | 28 |
| exception_safety | 24 |
| determinism | 23 |
| concurrency | 13 |
| memory | 13 |
| performance | 13 |
| platform | 6 |
| observability | 5 |
| raii | 4 |
| input_validation | 1 |
| legacy_duplication | 1 |

## File Overview

| File | Findings | Critical | High | Medium | Low |
|---|---:|---:|---:|---:|---:|
| src/prompt_engineering/dspy_module.cpp | 37 | 8 | 23 | 6 | 0 |
| src/prompt_engineering/prompt_version_control.cpp | 30 | 2 | 2 | 26 | 0 |
| src/prompt_engineering/self_improvement_orchestrator.cpp | 28 | 9 | 14 | 5 | 0 |
| src/prompt_engineering/feedback_collector.cpp | 17 | 0 | 0 | 17 | 0 |
| src/prompt_engineering/tree_of_thoughts.cpp | 15 | 0 | 0 | 15 | 0 |
| src/prompt_engineering/prompt_evaluator.cpp | 13 | 0 | 7 | 4 | 2 |
| src/prompt_engineering/prompt_manager.cpp | 10 | 0 | 0 | 10 | 0 |
| src/prompt_engineering/prompt_regression_runner.cpp | 10 | 0 | 7 | 3 | 0 |
| src/prompt_engineering/prompt_compressor.cpp | 9 | 0 | 0 | 9 | 0 |
| src/prompt_engineering/prompt_injection_detector.cpp | 9 | 0 | 0 | 9 | 0 |
| src/prompt_engineering/protegi_optimizer.cpp | 7 | 1 | 1 | 5 | 0 |
| src/prompt_engineering/prompt_library_io.cpp | 6 | 0 | 0 | 6 | 0 |
| src/prompt_engineering/reflection_tuner.cpp | 6 | 0 | 1 | 5 | 0 |
| src/prompt_engineering/meta_prompt_generator.cpp | 5 | 2 | 2 | 1 | 0 |
| src/prompt_engineering/prompt_ab_experiment.cpp | 5 | 0 | 0 | 3 | 2 |
| src/prompt_engineering/prompt_performance_tracker.cpp | 5 | 0 | 0 | 5 | 0 |
| src/prompt_engineering/prompt_quality_evaluator.cpp | 5 | 0 | 0 | 5 | 0 |
| src/prompt_engineering/structured_output.cpp | 5 | 0 | 0 | 5 | 0 |
| src/prompt_engineering/context_window_manager.cpp | 4 | 0 | 0 | 4 | 0 |
| src/prompt_engineering/llm_reflection_adapter.cpp | 4 | 0 | 0 | 4 | 0 |
| src/prompt_engineering/prompt_template_compiler.cpp | 4 | 0 | 0 | 4 | 0 |
| src/prompt_engineering/cot_tracer.cpp | 3 | 0 | 0 | 3 | 0 |
| src/prompt_engineering/prompt_engineering_integration.cpp | 3 | 0 | 0 | 3 | 0 |
| src/prompt_engineering/adversarial_prompt_tester.cpp | 2 | 0 | 0 | 2 | 0 |
| src/prompt_engineering/rag_prompt_builder.cpp | 2 | 0 | 0 | 2 | 0 |
| src/prompt_engineering/system_prompt_manager.cpp | 2 | 0 | 0 | 2 | 0 |
| src/prompt_engineering/markdown_utils.cpp | 1 | 1 | 0 | 0 | 0 |
| src/prompt_engineering/prompt_engineering_metrics.cpp | 1 | 0 | 0 | 1 | 0 |
| src/prompt_engineering/prompt_optimizer.cpp | 1 | 0 | 1 | 0 | 0 |
| src/prompt_engineering/prompt_template_validator.cpp | 1 | 0 | 0 | 1 | 0 |
| src/prompt_engineering/rag_context_budget_manager.cpp | 1 | 0 | 1 | 0 | 0 |

## Full Scanner Findings

### src/prompt_engineering/dspy_module.cpp
Total findings: 37

- Line 50: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: DspySignature& DspySignature::addInput(DspyField field)
  Confidence: band=very_high; score=0.99
- Line 52: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: inputs_.push_back(std::move(field));
  Confidence: band=very_high; score=0.99
- Line 64: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: const std::vector<DspyField>& DspySignature::inputs()  const { return inputs_;  }
  Confidence: band=very_high; score=0.99
- Line 77: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: // Render input fields
  Confidence: band=very_high; score=0.99
- Line 78: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: for (const auto& field : inputs_) {
  Confidence: band=very_high; score=0.99
- Line 207: severity=CRITICAL; category=audit_logging; pattern=sensitive_data_logging
  Description: Potential PII/credential logging: token
  Context: response << token << ": [echo]\n";
  Confidence: band=very_high; score=0.92
- Line 277: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: for (const auto& f : signature.inputs()) {
  Confidence: band=very_high; score=0.99
- Line 278: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: result.addInput(f);
  Confidence: band=very_high; score=0.99
- Line 50: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: DspySignature& DspySignature::addInput(DspyField field)
  Confidence: band=very_high; score=0.9
- Line 52: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: inputs_.push_back(std::move(field));
  Confidence: band=very_high; score=0.9
- Line 52: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: inputs_.push_back(std::move(field));
  Confidence: band=very_high; score=0.9
- Line 58: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: outputs_.push_back(std::move(field));
  Confidence: band=very_high; score=0.9
- Line 64: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: const std::vector<DspyField>& DspySignature::inputs()  const { return inputs_;  }
  Confidence: band=very_high; score=0.9
- Line 64: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: const std::vector<DspyField>& DspySignature::inputs()  const { return inputs_;  }
  Confidence: band=very_high; score=0.9
- Line 65: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: const std::vector<DspyField>& DspySignature::outputs() const { return outputs_; }
  Confidence: band=very_high; score=0.9
- Line 77: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: // Render input fields
  Confidence: band=very_high; score=0.9
- Line 78: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: for (const auto& field : inputs_) {
  Confidence: band=very_high; score=0.9
- Line 78: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: for (const auto& field : inputs_) {
  Confidence: band=very_high; score=0.9
- Line 94: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: for (const auto& field : outputs_) {
  Confidence: band=very_high; score=0.9
- Line 110: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: for (const auto& field : outputs_) {
  Confidence: band=very_high; score=0.9
- Line 142: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: for (const auto& other : outputs_) {
  Confidence: band=very_high; score=0.9
- Line 147: severity=HIGH; category=performance; pattern=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Context: auto other_pos = response.find(other_marker, value_start);
  Confidence: band=very_high; score=0.9
- Line 269: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: // Rebuild outputs with Reasoning prepended
  Confidence: band=very_high; score=0.9
- Line 270: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: std::vector<DspyField> new_outputs;
  Confidence: band=very_high; score=0.9
- Line 271: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: new_outputs.push_back(std::move(reasoning_field));
  Confidence: band=very_high; score=0.9
- Line 272: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: for (const auto& f : signature.outputs()) {
  Confidence: band=very_high; score=0.9
- Line 273: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: new_outputs.push_back(f);
  Confidence: band=very_high; score=0.9
- Line 277: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: for (const auto& f : signature.inputs()) {
  Confidence: band=very_high; score=0.9
- Line 277: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: for (const auto& f : signature.inputs()) {
  Confidence: band=very_high; score=0.9
- Line 278: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: result.addInput(f);
  Confidence: band=very_high; score=0.9
- Line 280: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: for (auto& f : new_outputs) {
  Confidence: band=very_high; score=0.9
- Line 68: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: const std::unordered_map<std::string, std::string>& context) const
  Confidence: band=medium; score=0.66
- Line 105: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, std::string> DspySignature::parseResponse(
  Confidence: band=medium; score=0.66
- Line 108: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, std::string> parsed;
  Confidence: band=medium; score=0.66
- Line 171: severity=MEDIUM; category=llm_ai_safety; pattern=missing_resource_limits
  Description: LLM inference without token limit or timeout (DOS risk)
  Context: std::string EchoDspyLLMProvider::complete(const std::string& prompt)
  Confidence: band=high; score=0.74
- Line 246: severity=MEDIUM; category=llm_ai_safety; pattern=missing_resource_limits
  Description: LLM inference without token limit or timeout (DOS risk)
  Context: std::string response = llm_provider_->complete(prompt);
  Confidence: band=high; score=0.74
- Line 272: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: new_outputs.push_back(f);
  Confidence: band=high; score=0.74

### src/prompt_engineering/prompt_version_control.cpp
Total findings: 30

- Line 719: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: std::string input = prompt_id + content + parent +
  Confidence: band=very_high; score=0.99
- Line 723: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: SHA256(reinterpret_cast<const unsigned char*>(input.c_str()), input.length(), hash);
  Confidence: band=very_high; score=0.99
- Line 719: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: std::string input = prompt_id + content + parent +
  Confidence: band=very_high; score=0.9
- Line 723: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: SHA256(reinterpret_cast<const unsigned char*>(input.c_str()), input.length(), hash);
  Confidence: band=very_high; score=0.9
- Line 64: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto time_val = j["timestamp"].get<std::time_t>();
  Confidence: band=high; score=0.74
- Line 215: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: history.push_back(version);
  Confidence: band=high; score=0.74
- Line 220: severity=MEDIUM; category=determinism; pattern=timestamp_sorting_unstable
  Description: Timestamp-based sorting without stable_sort (non-deterministic ties)
  Context: // Sort by timestamp (descending)
  Confidence: band=high; score=0.74
- Line 420: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(info);
  Confidence: band=high; score=0.74
- Line 513: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> tgt_set(tgt_ancestors.begin(), tgt_ancestors.end());
  Confidence: band=medium; score=0.66
- Line 590: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, std::string> PromptVersionControl::getGenealogy(
  Confidence: band=medium; score=0.66
- Line 595: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, std::string> genealogy;
  Confidence: band=medium; score=0.66
- Line 691: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: stats["branches"].push_back(name);
  Confidence: band=high; score=0.74
- Line 691: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: stats["branches"].push_back(name);
  Confidence: band=high; score=0.74
- Line 834: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: edits.push_back({Op::KEEP, lines_a[i - 1]});
  Confidence: band=high; score=0.74
- Line 834: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: edits.push_back({Op::KEEP, lines_a[i - 1]});
  Confidence: band=high; score=0.74
- Line 851: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: diff.removed_lines.push_back(e.line);
  Confidence: band=high; score=0.74
- Line 976: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: edits.push_back({' ', from[i-1]});
  Confidence: band=high; score=0.74
- Line 976: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: edits.push_back({' ', from[i-1]});
  Confidence: band=high; score=0.74
- Line 1022: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: changes[base_idx].insertions_before.push_back(e.second);
  Confidence: band=high; score=0.74
- Line 1045: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> sc_ins_set(
  Confidence: band=medium; score=0.66
- Line 1046: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: for (const auto& ins : sc.insertions_before) merged.push_back(ins);
  Confidence: band=high; score=0.74
- Line 1049: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: merged.push_back(ins);
  Confidence: band=high; score=0.74
- Line 1049: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: merged.push_back(ins);
  Confidence: band=high; score=0.74
- Line 1068: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> src_eof_set(
  Confidence: band=medium; score=0.66
- Line 1072: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: merged.push_back(ins);
  Confidence: band=high; score=0.74
- Line 1072: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: merged.push_back(ins);
  Confidence: band=high; score=0.74
- Line 1082: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: for (const auto& l : lines) { out += l; out += '\n'; }
  Confidence: band=high; score=0.74
- Line 1082: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: for (const auto& l : lines) { out += l; out += '\n'; }
  Confidence: band=high; score=0.74
- Line 1100: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.conflicts.push_back(
  Confidence: band=high; score=0.74
- Line 1100: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.conflicts.push_back(
  Confidence: band=high; score=0.74

### src/prompt_engineering/self_improvement_orchestrator.cpp
Total findings: 28

- Line 205: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: // between (prompt + input) and expected_output, which serves as a proxy for
  Confidence: band=very_high; score=0.99
- Line 208: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: // The "\n" separator between prompt and input is a neutral token boundary
  Confidence: band=very_high; score=0.99
- Line 213: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: // Guard: evaluateBatch() returns 0.0 for empty inputs, which would
  Confidence: band=very_high; score=0.99
- Line 216: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: std::vector<std::string> prompt_with_inputs;
  Confidence: band=very_high; score=0.99
- Line 218: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: prompt_with_inputs.reserve(cases.size());
  Confidence: band=very_high; score=0.99
- Line 221: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: // Combine prompt template with the test input as a single
  Confidence: band=very_high; score=0.99
- Line 223: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: prompt_with_inputs.push_back(prompt + "\n" + tc.input);
  Confidence: band=very_high; score=0.99
- Line 226: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: return evaluator_->evaluateBatch(prompt_with_inputs, expected).overall_score;
  Confidence: band=very_high; score=0.99
- Line 641: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: tc.input           = entry.query;
  Confidence: band=very_high; score=0.99
- Line 205: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: // between (prompt + input) and expected_output, which serves as a proxy for
  Confidence: band=very_high; score=0.9
- Line 208: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: // The "\n" separator between prompt and input is a neutral token boundary
  Confidence: band=very_high; score=0.9
- Line 213: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: // Guard: evaluateBatch() returns 0.0 for empty inputs, which would
  Confidence: band=very_high; score=0.9
- Line 213: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: // Guard: evaluateBatch() returns 0.0 for empty inputs, which would
  Confidence: band=very_high; score=0.9
- Line 216: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: std::vector<std::string> prompt_with_inputs;
  Confidence: band=very_high; score=0.9
- Line 216: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: std::vector<std::string> prompt_with_inputs;
  Confidence: band=very_high; score=0.9
- Line 218: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: prompt_with_inputs.reserve(cases.size());
  Confidence: band=very_high; score=0.9
- Line 218: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: prompt_with_inputs.reserve(cases.size());
  Confidence: band=very_high; score=0.9
- Line 221: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: // Combine prompt template with the test input as a single
  Confidence: band=very_high; score=0.9
- Line 223: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: prompt_with_inputs.push_back(prompt + "\n" + tc.input);
  Confidence: band=very_high; score=0.9
- Line 223: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: prompt_with_inputs.push_back(prompt + "\n" + tc.input);
  Confidence: band=very_high; score=0.9
- Line 226: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: return evaluator_->evaluateBatch(prompt_with_inputs, expected).overall_score;
  Confidence: band=very_high; score=0.9
- Line 226: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: return evaluator_->evaluateBatch(prompt_with_inputs, expected).overall_score;
  Confidence: band=very_high; score=0.9
- Line 641: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: tc.input           = entry.query;
  Confidence: band=very_high; score=0.9
- Line 137: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: candidates.emplace_back(metrics.prompt_id, std::move(test_cases));
  Confidence: band=high; score=0.74
- Line 145: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results.push_back(optimizePrompt(prompt_id, test_cases));
  Confidence: band=high; score=0.74
- Line 222: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: prompt_with_inputs.push_back(prompt + "\n" + tc.input);
  Confidence: band=high; score=0.74
- Line 475: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: tests.push_back(test);
  Confidence: band=high; score=0.74
- Line 642: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: test_cases.push_back(std::move(tc));
  Confidence: band=high; score=0.74

### src/prompt_engineering/feedback_collector.cpp
Total findings: 17

- Line 112: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto time_val = j["timestamp"].get<std::time_t>();
  Confidence: band=high; score=0.74
- Line 273: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(entry);
  Confidence: band=high; score=0.74
- Line 312: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(prompt_id);
  Confidence: band=high; score=0.74
- Line 350: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.emplace_back(entry.query, entry.response, entry.type);
  Confidence: band=high; score=0.74
- Line 376: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: failures.push_back(entry);
  Confidence: band=high; score=0.74
- Line 424: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(entry);
  Confidence: band=high; score=0.74
- Line 538: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(entry);
  Confidence: band=high; score=0.74
- Line 586: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: outliers.push_back(e);
  Confidence: band=high; score=0.74
- Line 586: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: outliers.push_back(e);
  Confidence: band=high; score=0.74
- Line 771: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: sorted_types.emplace_back(type, count);
  Confidence: band=high; score=0.74
- Line 779: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: stats.common_issues.push_back(feedbackTypeToString(sorted_types[i].first));
  Confidence: band=high; score=0.74
- Line 811: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: tokens.push_back(cur);
  Confidence: band=high; score=0.74
- Line 828: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, size_t> document_frequency;
  Confidence: band=medium; score=0.66
- Line 836: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> seen(tokens.begin(), tokens.end());
  Confidence: band=medium; score=0.66
- Line 850: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, size_t> tf;
  Confidence: band=medium; score=0.66
- Line 877: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: p.examples.push_back(entry.query);
  Confidence: band=high; score=0.74
- Line 895: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(pat);
  Confidence: band=high; score=0.74

### src/prompt_engineering/tree_of_thoughts.cpp
Total findings: 15

- Line 30: severity=MEDIUM; category=llm_ai_safety; pattern=missing_resource_limits
  Description: LLM inference without token limit or timeout (DOS risk)
  Context: std::vector<std::string> HeuristicThoughtGenerator::generate(
  Confidence: band=high; score=0.74
- Line 46: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: thoughts.push_back(oss.str());
  Confidence: band=high; score=0.74
- Line 159: severity=MEDIUM; category=llm_ai_safety; pattern=missing_resource_limits
  Description: LLM inference without token limit or timeout (DOS risk)
  Context: auto root_thoughts = generator_->generate(problem, {}, config_.branching_factor);
  Confidence: band=high; score=0.74
- Line 205: severity=MEDIUM; category=llm_ai_safety; pattern=missing_resource_limits
  Description: LLM inference without token limit or timeout (DOS risk)
  Context: auto children = generator_->generate(problem, child_path, config_.branching_factor);
  Confidence: band=high; score=0.74
- Line 214: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.best_path.push_back(best_node.thought);
  Confidence: band=high; score=0.74
- Line 233: severity=MEDIUM; category=llm_ai_safety; pattern=missing_resource_limits
  Description: LLM inference without token limit or timeout (DOS risk)
  Context: auto root_thoughts = generator_->generate(problem, {}, config_.branching_factor);
  Confidence: band=high; score=0.74
- Line 253: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.log.push_back(msg.str());
  Confidence: band=high; score=0.74
- Line 277: severity=MEDIUM; category=llm_ai_safety; pattern=missing_resource_limits
  Description: LLM inference without token limit or timeout (DOS risk)
  Context: auto children = generator_->generate(problem, child_path, config_.branching_factor);
  Confidence: band=high; score=0.74
- Line 285: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.best_path.push_back(best_node.thought);
  Confidence: band=high; score=0.74
- Line 305: severity=MEDIUM; category=llm_ai_safety; pattern=missing_resource_limits
  Description: LLM inference without token limit or timeout (DOS risk)
  Context: auto root_thoughts = generator_->generate(problem, {}, config_.branching_factor);
  Confidence: band=high; score=0.74
- Line 312: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: beam.push_back(std::move(n));
  Confidence: band=high; score=0.74
- Line 333: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.best_path.push_back(node.thought);
  Confidence: band=high; score=0.74
- Line 333: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.best_path.push_back(node.thought);
  Confidence: band=high; score=0.74
- Line 343: severity=MEDIUM; category=llm_ai_safety; pattern=missing_resource_limits
  Description: LLM inference without token limit or timeout (DOS risk)
  Context: auto children = generator_->generate(problem, child_path, config_.branching_factor);
  Confidence: band=high; score=0.74
- Line 353: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.log.push_back(msg.str());
  Confidence: band=high; score=0.74

### src/prompt_engineering/prompt_evaluator.cpp
Total findings: 13

- Line 68: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: const std::vector<std::string>& outputs,
  Confidence: band=very_high; score=0.9
- Line 73: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: if (outputs.size() != expected.size()) {
  Confidence: band=very_high; score=0.9
- Line 78: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: if (outputs.empty()) {
  Confidence: band=very_high; score=0.9
- Line 86: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: for (size_t i = 0; i < outputs.size(); ++i) {
  Confidence: band=very_high; score=0.9
- Line 87: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: auto metrics = evaluateSingle(outputs[i], expected[i]);
  Confidence: band=very_high; score=0.9
- Line 305: severity=HIGH; category=determinism; pattern=fp_exact_comparison
  Description: Floating-point exact comparison (use tolerance/epsilon)
  Context: if (x == 0.0) return 0.0;
  Confidence: band=very_high; score=0.9
- Line 306: severity=HIGH; category=determinism; pattern=fp_exact_comparison
  Description: Floating-point exact comparison (use tolerance/epsilon)
  Context: if (x == 1.0) return 1.0;
  Confidence: band=very_high; score=0.9
- Line 89: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: similarities.push_back(metrics.semantic_similarity);
  Confidence: band=high; score=0.74
- Line 160: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> set1(tokens1.begin(), tokens1.end());
  Confidence: band=medium; score=0.66
- Line 161: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> set2(tokens2.begin(), tokens2.end());
  Confidence: band=medium; score=0.66
- Line 223: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> output_set(tokens_output.begin(), tokens_output.end());
  Confidence: band=medium; score=0.66
- Line 319: severity=LOW; category=observability; pattern=unstructured_log
  Description: Unstructured logging (use structured format)
  Context: return std::log(M_PI / std::sin(M_PI * z)) - std::lgamma(1.0 - z);
  Confidence: band=medium; score=0.6
- Line 324: severity=LOW; category=observability; pattern=unstructured_log
  Description: Unstructured logging (use structured format)
  Context: double front = std::exp(aa * std::log(xx) + bb * std::log(1.0 - xx) - log_beta) / aa;
  Confidence: band=medium; score=0.6

### src/prompt_engineering/prompt_manager.cpp
Total findings: 10

- Line 57: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.errors.push_back(
  Confidence: band=high; score=0.74
- Line 128: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: t.images.push_back(ImageDescription::fromJson(img_j));
  Confidence: band=high; score=0.74
- Line 182: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: out.push_back(kv.second);
  Confidence: band=high; score=0.74
- Line 297: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: pt.images.push_back(std::move(img));
  Confidence: band=high; score=0.74
- Line 332: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: const std::unordered_map<std::string, std::string>& context) const {
  Confidence: band=medium; score=0.66
- Line 394: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: caps_array.push_back(cap);
  Confidence: band=high; score=0.74
- Line 406: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: tables_array.push_back(table_info);
  Confidence: band=high; score=0.74
- Line 431: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: const std::unordered_map<std::string, std::string>& context) {
  Confidence: band=medium; score=0.66
- Line 445: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: result += "\n\n[Images]\n";
  Confidence: band=high; score=0.74
- Line 449: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: result += std::to_string(i + 1) + ". [" + mime + "] " + img.alt_text + "\n";
  Confidence: band=high; score=0.74

### src/prompt_engineering/prompt_regression_runner.cpp
Total findings: 10

- Line 123: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: const std::vector<std::string>& baseline_outputs,
  Confidence: band=very_high; score=0.9
- Line 124: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: const std::vector<std::string>& candidate_outputs) const {
  Confidence: band=very_high; score=0.9
- Line 132: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: baseline_outputs.size(),
  Confidence: band=very_high; score=0.9
- Line 133: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: candidate_outputs.size()});
  Confidence: band=very_high; score=0.9
- Line 137: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: baseline_outputs.size() != candidate_outputs.size() ||
  Confidence: band=very_high; score=0.9
- Line 155: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: baseline_outputs[i], fixture.expected_output);
  Confidence: band=very_high; score=0.9
- Line 157: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: candidate_outputs[i], fixture.expected_output);
  Confidence: band=very_high; score=0.9
- Line 51: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: deltas.push_back({
  Confidence: band=high; score=0.74
- Line 105: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: fixtures_.push_back(std::move(f));
  Confidence: band=high; score=0.74
- Line 169: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: baseline_scores.push_back(bs);
  Confidence: band=high; score=0.74

### src/prompt_engineering/prompt_compressor.cpp
Total findings: 9

- Line 66: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: if (i > 0) result += ' ';
  Confidence: band=high; score=0.74
- Line 218: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: kept_indices.push_back(i);
  Confidence: band=high; score=0.74
- Line 221: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: kept_indices.push_back(i);
  Confidence: band=high; score=0.74
- Line 226: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: if (!result.empty()) result += "\n\n";
  Confidence: band=high; score=0.74
- Line 226: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: if (!result.empty()) result += "\n\n";
  Confidence: band=high; score=0.74
- Line 226: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: if (!result.empty()) result += "\n\n";
  Confidence: band=high; score=0.74
- Line 258: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: if (!middle.empty()) middle += "\n\n";
  Confidence: band=high; score=0.74
- Line 265: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: if (!result.empty()) result += "\n\n";
  Confidence: band=high; score=0.74
- Line 276: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: if (!result.empty()) result += "\n\n";
  Confidence: band=high; score=0.74

### src/prompt_engineering/prompt_injection_detector.cpp
Total findings: 9

- Line 44: severity=MEDIUM; category=legacy_duplication; pattern=duplicate_qualified_signature
  Description: Potential duplicate implementation signature across files: PromptInjectionDetector::initializePatterns()
  Context: void PromptInjectionDetector::initializePatterns() {
  Confidence: band=medium; score=0.56
- Line 63: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: patterns_.emplace_back(e.pattern_str, std::regex::icase);
  Confidence: band=high; score=0.74
- Line 78: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: patterns_.emplace_back(pat, std::regex::icase);
  Confidence: band=high; score=0.74
- Line 96: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: matched_out.push_back(pattern_labels_[i]);
  Confidence: band=high; score=0.74
- Line 112: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: matched_out.push_back("keyword:" + kw);
  Confidence: band=high; score=0.74
- Line 138: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: matched_out.push_back("syntax:high_special_char_density");
  Confidence: band=high; score=0.74
- Line 217: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: extra_matched.push_back(label);
  Confidence: band=high; score=0.74
- Line 260: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: pos += 10; // len("[REDACTED]")
  Confidence: band=high; score=0.74
- Line 260: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: pos += 10; // len("[REDACTED]")
  Confidence: band=high; score=0.74

### src/prompt_engineering/protegi_optimizer.cpp
Total findings: 7

- Line 96: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: << "- Read the input carefully.\n"
  Confidence: band=very_high; score=0.99
- Line 96: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: << "- Read the input carefully.\n"
  Confidence: band=very_high; score=0.9
- Line 114: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: candidates.push_back(c.str());
  Confidence: band=high; score=0.74
- Line 209: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: next_candidates.push_back(std::move(c));
  Confidence: band=high; score=0.74
- Line 223: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: scored.emplace_back(s, cand);
  Confidence: band=high; score=0.74
- Line 257: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: beam.push_back(p);
  Confidence: band=high; score=0.74
- Line 366: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: errors.push_back("Expected output longer than prompt; prompt may lack detail.");
  Confidence: band=high; score=0.74

### src/prompt_engineering/prompt_library_io.cpp
Total findings: 6

- Line 77: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: t.images.push_back(img);
  Confidence: band=high; score=0.74
- Line 100: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: tpls.push_back(t.toJson());
  Confidence: band=high; score=0.74
- Line 134: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: t.images.push_back(img);
  Confidence: band=high; score=0.74
- Line 134: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: t.images.push_back(img);
  Confidence: band=high; score=0.74
- Line 153: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: parts.push_back(t.toJson().dump());
  Confidence: band=high; score=0.74
- Line 303: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: b.templates.push_back(templateFromYaml(node));
  Confidence: band=high; score=0.74

### src/prompt_engineering/reflection_tuner.cpp
Total findings: 6

- Line 34: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: // Linguistic markers used to infer self-reported confidence.
  Confidence: band=very_high; score=0.9
- Line 85: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: ctx.uncertainty_markers.push_back(marker);
  Confidence: band=high; score=0.74
- Line 144: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: steps_arr.push_back(s);
  Confidence: band=high; score=0.74
- Line 387: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: trajectory.push_back(step.quality_score);
  Confidence: band=high; score=0.74
- Line 554: severity=MEDIUM; category=llm_ai_safety; pattern=missing_resource_limits
  Description: LLM inference without token limit or timeout (DOS risk)
  Context: provider_ ? provider_->generate(prompt) : prompt;
  Confidence: band=high; score=0.74
- Line 586: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.steps.push_back(step);
  Confidence: band=high; score=0.74

### src/prompt_engineering/meta_prompt_generator.cpp
Total findings: 5

- Line 156: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: analysis << "**Input**: " << examples[i].first << "\n";
  Confidence: band=very_high; score=0.99
- Line 162: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: analysis << "2. Assess whether examples cover the input space adequately\n";
  Confidence: band=very_high; score=0.99
- Line 156: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: analysis << "**Input**: " << examples[i].first << "\n";
  Confidence: band=very_high; score=0.9
- Line 162: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: analysis << "2. Assess whether examples cover the input space adequately\n";
  Confidence: band=very_high; score=0.9
- Line 86: severity=MEDIUM; category=llm_ai_safety; pattern=missing_resource_limits
  Description: LLM inference without token limit or timeout (DOS risk)
  Context: std::string llm_response = llm_provider_->complete(result.meta_prompt);
  Confidence: band=high; score=0.74

### src/prompt_engineering/prompt_ab_experiment.cpp
Total findings: 5

- Line 326: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: out.push_back(exp);
  Confidence: band=high; score=0.74
- Line 588: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: experiments_.push_back(std::move(descriptor));
  Confidence: band=high; score=0.74
- Line 588: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: experiments_.push_back(std::move(descriptor));
  Confidence: band=high; score=0.74
- Line 193: severity=LOW; category=observability; pattern=unstructured_log
  Description: Unstructured logging (use structured format)
  Context: const double front  = std::exp(std::log(x) * a +
  Confidence: band=medium; score=0.6
- Line 194: severity=LOW; category=observability; pattern=unstructured_log
  Description: Unstructured logging (use structured format)
  Context: std::log(1.0 - x) * b - lnBeta) / a;
  Confidence: band=medium; score=0.6

### src/prompt_engineering/prompt_performance_tracker.cpp
Total findings: 5

- Line 59: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto time_val = j["last_updated"].get<std::time_t>();
  Confidence: band=high; score=0.74
- Line 64: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto time_val = j["created_at"].get<std::time_t>();
  Confidence: band=high; score=0.74
- Line 135: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(metrics);
  Confidence: band=high; score=0.74
- Line 152: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: low_performers.push_back(id);
  Confidence: band=high; score=0.74
- Line 172: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: candidates.emplace_back(id, metrics.success_rate);
  Confidence: band=high; score=0.74

### src/prompt_engineering/prompt_quality_evaluator.cpp
Total findings: 5

- Line 36: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: tokens.push_back(std::move(word));
  Confidence: band=high; score=0.74
- Line 72: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: failed.push_back(QualityCheck{
  Confidence: band=high; score=0.74
- Line 123: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, size_t> bigram_counts;
  Confidence: band=medium; score=0.66
- Line 138: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: failed.push_back(QualityCheck{
  Confidence: band=high; score=0.74
- Line 138: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: failed.push_back(QualityCheck{
  Confidence: band=high; score=0.74

### src/prompt_engineering/structured_output.cpp
Total findings: 5

- Line 157: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back((*it)[1].str());
  Confidence: band=high; score=0.74
- Line 217: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: names.push_back(cur_key);
  Confidence: band=high; score=0.74
- Line 267: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: keys.push_back(cur_key);
  Confidence: band=high; score=0.74
- Line 305: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: errors.push_back("Missing required field: \"" + req + "\"");
  Confidence: band=high; score=0.74
- Line 317: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: errors.push_back("Unknown field not in schema properties: \"" +
  Confidence: band=high; score=0.74

### src/prompt_engineering/context_window_manager.cpp
Total findings: 4

- Line 90: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: selected.push_back(chunk);
  Confidence: band=high; score=0.74
- Line 189: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: if (is_active) active.push_back(&t);
  Confidence: band=high; score=0.74
- Line 228: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(std::move(inj));
  Confidence: band=high; score=0.74
- Line 249: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(std::move(inj));
  Confidence: band=high; score=0.74

### src/prompt_engineering/llm_reflection_adapter.cpp
Total findings: 4

- Line 29: severity=MEDIUM; category=llm_ai_safety; pattern=missing_resource_limits
  Description: LLM inference without token limit or timeout (DOS risk)
  Context: std::string ILLMProviderReflectionAdapter::generate(
  Confidence: band=high; score=0.74
- Line 32: severity=MEDIUM; category=llm_ai_safety; pattern=missing_resource_limits
  Description: LLM inference without token limit or timeout (DOS risk)
  Context: return llm_->complete(prompt);
  Confidence: band=high; score=0.74
- Line 42: severity=MEDIUM; category=llm_ai_safety; pattern=missing_resource_limits
  Description: LLM inference without token limit or timeout (DOS risk)
  Context: return llm_->complete(critique_prompt);
  Confidence: band=high; score=0.74
- Line 53: severity=MEDIUM; category=llm_ai_safety; pattern=missing_resource_limits
  Description: LLM inference without token limit or timeout (DOS risk)
  Context: return llm_->complete(revision_prompt);
  Confidence: band=high; score=0.74

### src/prompt_engineering/prompt_template_compiler.cpp
Total findings: 4

- Line 431: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: errors.push_back("Missing required slot: " + name);
  Confidence: band=high; score=0.74
- Line 489: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: slot_arr.push_back(s.toJson());
  Confidence: band=high; score=0.74
- Line 505: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, SlotDefinition> slot_index;
  Confidence: band=medium; score=0.66
- Line 549: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: final_slots.push_back(sd);
  Confidence: band=high; score=0.74

### src/prompt_engineering/cot_tracer.cpp
Total findings: 3

- Line 95: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: arr.push_back(rec.toJson());
  Confidence: band=high; score=0.74
- Line 165: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: children_.push_back(std::move(tracer));
  Confidence: band=high; score=0.74
- Line 203: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: arr.push_back(rec.toJson());
  Confidence: band=high; score=0.74

### src/prompt_engineering/prompt_engineering_integration.cpp
Total findings: 3

- Line 92: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto timestamp = j["start_time"].get<std::time_t>();
  Confidence: band=high; score=0.74
- Line 462: severity=MEDIUM; category=llm_ai_safety; pattern=missing_resource_limits
  Description: LLM inference without token limit or timeout (DOS risk)
  Context: snap_metrics->recordReflectionCycleComplete(
  Confidence: band=high; score=0.74
- Line 606: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, std::string> ctx_map;
  Confidence: band=medium; score=0.66

### src/prompt_engineering/adversarial_prompt_tester.cpp
Total findings: 2

- Line 200: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: cases_.push_back({tc.id, tc.category, tc.payload, tc.expected_blocked});
  Confidence: band=high; score=0.74
- Line 253: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: report.results.push_back(std::move(r));
  Confidence: band=high; score=0.74

### src/prompt_engineering/rag_prompt_builder.cpp
Total findings: 2

- Line 59: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: ordered.push_back(&c);
  Confidence: band=high; score=0.74
- Line 81: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: selected.push_back(chunk);
  Confidence: band=high; score=0.74

### src/prompt_engineering/system_prompt_manager.cpp
Total findings: 2

- Line 76: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: const std::unordered_map<std::string, std::string>& context) {
  Confidence: band=medium; score=0.66
- Line 191: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(sp);
  Confidence: band=high; score=0.74

### src/prompt_engineering/markdown_utils.cpp
Total findings: 1

- Line 23: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: // Handle empty or too-short input
  Confidence: band=very_high; score=0.99

### src/prompt_engineering/prompt_engineering_metrics.cpp
Total findings: 1

- Line 795: severity=MEDIUM; category=llm_ai_safety; pattern=missing_resource_limits
  Description: LLM inference without token limit or timeout (DOS risk)
  Context: void PromptEngineeringMetrics::recordReflectionCycleComplete(
  Confidence: band=high; score=0.74

### src/prompt_engineering/prompt_optimizer.cpp
Total findings: 1

- Line 169: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: auto meta_result = meta_gen.generateImprovementPrompt(
  Confidence: band=very_high; score=0.9

### src/prompt_engineering/prompt_template_validator.cpp
Total findings: 1

- Line 93: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.errors.push_back(
  Confidence: band=high; score=0.74

### src/prompt_engineering/rag_context_budget_manager.cpp
Total findings: 1

- Line 30: severity=HIGH; category=observability; pattern=missing_trace_point
  Description: Critical function allocate without trace point
  Context: BudgetHandle RagContextBudgetManager::allocate(size_t tokens) {
  Confidence: band=very_high; score=0.9

## Update Workflow

- Refresh scan artifacts with: python tools/gap_scanner_v3.py
- Regenerate all module notes with: python tools/module_doc_generator.py . ai_working ai_working/module_gaps
- The generator mirrors each archive document directly into src/<module>/MODULE_GAPS.md.

Format: THEMIS_MODULE_GAPS_V3
