# prompt_engineering Module - Developer Gap Note

> Auto-generated from ai_working\gap_scan_results.json.
> This file is overwritten on each regeneration.

## Scan Snapshot

- Module: prompt_engineering
- Generated: 2026-06-04 08:50:22
- Status: Critical Findings Present
- Total Findings: 298
- Actionable Findings (Critical + High): 140
- Affected Files: 28

## Severity Summary

| Severity | Count |
|---|---:|
| Critical | 17 |
| High | 123 |
| Medium | 120 |
| Low | 38 |

## Category Summary

| Category | Count |
|---|---:|
| copy_overhead | 43 |
| null_dereference | 36 |
| hardcoded_output | 32 |
| pointer_arithmetic_unbounded | 24 |
| resource_leaked_in_exception | 24 |
| missing_resource_limits | 20 |
| unordered_container_iter | 20 |
| string_concat_loop | 18 |
| data_race | 9 |
| o_n_squared | 7 |
| range_temporary | 7 |
| hardcoded_path | 5 |
| unnecessary_copy | 5 |
| memory_order | 4 |
| repeated_search | 4 |
| unstructured_log | 4 |
| delete_without_nullptr | 3 |
| explicit_delete | 3 |
| iterator_invalidation | 3 |
| uncaught_exception | 3 |
| unvalidated_llm_output | 3 |
| db_connection_leak | 2 |
| fp_exact_comparison | 2 |
| generic_catch | 2 |
| missing_vector_reserve | 2 |
| module_doc_linkset_drift | 2 |
| prompt_injection | 2 |
| duplicate_qualified_signature | 1 |
| manual_cleanup | 1 |
| missing_trace_point | 1 |
| nested_loop_find | 1 |
| new_without_raii | 1 |
| sensitive_data_logging | 1 |
| smart_ptr_misuse | 1 |
| timestamp_sorting_unstable | 1 |
| unchecked_array_index | 1 |

## File Overview

| File | Findings | Critical | High | Medium | Low |
|---|---:|---:|---:|---:|---:|
| prompt_engineering/prompt_template_compiler.cpp | 48 | 0 | 39 | 9 | 0 |
| prompt_engineering/prompt_version_control.cpp | 30 | 3 | 12 | 15 | 0 |
| prompt_engineering/meta_prompt_generator.cpp | 29 | 0 | 5 | 24 | 0 |
| prompt_engineering/dspy_module.cpp | 26 | 1 | 5 | 5 | 15 |
| prompt_engineering/prompt_evaluator.cpp | 24 | 3 | 7 | 7 | 7 |
| prompt_engineering/self_improvement_orchestrator.cpp | 19 | 3 | 11 | 0 | 5 |
| prompt_engineering/feedback_collector.cpp | 18 | 0 | 10 | 8 | 0 |
| prompt_engineering/tree_of_thoughts.cpp | 16 | 6 | 0 | 10 | 0 |
| prompt_engineering/prompt_compressor.cpp | 14 | 0 | 2 | 12 | 0 |
| prompt_engineering/prompt_manager.cpp | 10 | 0 | 1 | 9 | 0 |
| prompt_engineering/reflection_tuner.cpp | 9 | 0 | 7 | 2 | 0 |
| prompt_engineering/prompt_regression_runner.cpp | 7 | 0 | 0 | 0 | 7 |
| prompt_engineering/prompt_ab_experiment.cpp | 6 | 0 | 4 | 0 | 2 |
| prompt_engineering/structured_output.cpp | 5 | 0 | 3 | 2 | 0 |
| prompt_engineering/llm_reflection_adapter.cpp | 4 | 0 | 0 | 4 | 0 |
| prompt_engineering/prompt_engineering_integration.cpp | 4 | 0 | 1 | 3 | 0 |
| prompt_engineering/prompt_engineering_metrics.cpp | 4 | 0 | 3 | 1 | 0 |
| prompt_engineering/prompt_injection_detector.cpp | 4 | 0 | 2 | 2 | 0 |
| prompt_engineering/prompt_optimizer.cpp | 4 | 0 | 4 | 0 | 0 |
| prompt_engineering/prompt_performance_tracker.cpp | 4 | 1 | 1 | 2 | 0 |
| prompt_engineering/rag_context_budget_manager.cpp | 4 | 0 | 4 | 0 | 0 |
| prompt_engineering/context_window_manager.cpp | 2 | 0 | 2 | 0 | 0 |
| prompt_engineering/prompt_library_io.cpp | 2 | 0 | 0 | 2 | 0 |
| prompt_engineering/FUTURE_ENHANCEMENTS.md | 1 | 0 | 0 | 0 | 1 |
| prompt_engineering/PRODUCTION_REQUIREMENTS.md | 1 | 0 | 0 | 0 | 1 |
| prompt_engineering/markdown_utils.cpp | 1 | 0 | 0 | 1 | 0 |
| prompt_engineering/prompt_quality_evaluator.cpp | 1 | 0 | 0 | 1 | 0 |
| prompt_engineering/system_prompt_manager.cpp | 1 | 0 | 0 | 1 | 0 |

## Full Scanner Findings

### prompt_engineering/prompt_template_compiler.cpp
Total findings: 48

- Line 235: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: node->kind = detail::ASTNode::Kind::TEXT;
- Line 236: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: node->text = tok.value;
- Line 244: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: node->kind = detail::ASTNode::Kind::SLOT;
- Line 245: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: node->text = tok.value; // name stored in .text
- Line 249: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: node->required      = it->second.required;
- Line 250: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: node->default_value = it->second.default_value;
- Line 252: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: node->required = false;
- Line 261: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: node->kind = detail::ASTNode::Kind::IF;
- Line 262: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: node->name = tok.value; // condition variable
- Line 265: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: node->children = parse(tokens, idx, slot_index,
- Line 270: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: node->else_children = parse(tokens, idx, slot_index,
- Line 299: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: node->kind    = detail::ASTNode::Kind::FOREACH;
- Line 300: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: node->name    = tok.value;  // item variable
- Line 301: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: node->list_var = tok.value2; // list slot name
- Line 303: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: node->children = parse(tokens, idx, slot_index,
- Line 339: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: switch (node->kind) {
- Line 341: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: out << node->text;
- Line 345: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: const std::string& name = node->text;
- Line 353: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: if (node->required) {
- Line 356: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: out << node->default_value;
- Line 364: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: const std::string& cond_var = node->name;
- Line 376: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: renderNodes(node->children, ctx, item_var, item_val, out);
- Line 377: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: } else if (!node->else_children.empty()) {
- Line 378: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: renderNodes(node->else_children, ctx, item_var, item_val, out);
- Line 384: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: const std::string& list_name = node->list_var;
- Line 385: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: const std::string& item_name = node->name;
- Line 394: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: renderNodes(node->children, ctx, item_name, elem, out);
- Line 398: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: renderNodes(node->children, ctx, item_name,
- Line 403: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: renderNodes(node->children, ctx, item_name,
- Line 424: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: switch (node->kind) {
- Line 429: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: const std::string& name = node->text;
- Line 430: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: if (node->required && ctx.find(name) == ctx.end()) {
- Line 431: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: if (node->required && ctx.find(name) == ctx.end()) {
- Line 438: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: validateNodes(node->children, ctx, item_var, errors);
- Line 439: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: if (!node->else_children.empty()) {
- Line 440: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: validateNodes(node->else_children, ctx, item_var, errors);
- Line 447: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: validateNodes(node->children, ctx, node->name, errors);
- Line 529: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: if (slot_index.find(n->text) == slot_index.end()) {
- Line 547: severity=HIGH; category=repeated_search
  Description: find/search in loop — O(n²) or worse
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: if (std::find_if(final_slots.begin(), final_slots.end(),
- Line 129: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: tokens.push_back({TokenKind::TEXT, text_buf, {}});
- Line 142: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: tokens.push_back({TokenKind::SLOT, inner, {}});
- Line 164: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: tokens.push_back({TokenKind::IF, var, {}});
- Line 166: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: tokens.push_back({TokenKind::ELSE, {}, {}});
- Line 168: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: tokens.push_back({TokenKind::ENDIF, {}, {}});
- Line 179: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: tokens.push_back({TokenKind::FOR, item_var, list_var});
- Line 181: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: tokens.push_back({TokenKind::ENDFOR, {}, {}});
- Line 199: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: tokens.push_back({TokenKind::SLOT, inner, {}});
- Line 505: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_map<std::string, SlotDefinition> slot_index;

### prompt_engineering/prompt_version_control.cpp
Total findings: 30

- Line 462: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator prompt_it may be invalidated by container modification
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: auto prompt_it = branches_.find(prompt_id);
- Line 476: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator source_it may be invalidated by container modification
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: auto source_it = versions_.find(source_id);
- Line 477: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator target_it may be invalidated by container modification
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: auto target_it = versions_.find(target_id);
- Line 282: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 283: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 284: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 285: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 286: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 287: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 288: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 289: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 290: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 294: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 880: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: for (size_t k = hunk_end; k < std::min(hunk_end + size_t(CONTEXT) * 2, N); ++k) {
- Line 887: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 64: severity=MEDIUM; category=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: auto time_val = j["timestamp"].get<std::time_t>();
- Line 220: severity=MEDIUM; category=timestamp_sorting_unstable
  Description: Timestamp-based sorting without stable_sort (non-deterministic ties)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: // Sort by timestamp (descending)
- Line 513: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_set<std::string> tgt_set(tgt_ancestors.begin(), tgt_ancestors.end());
- Line 590: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_map<std::string, std::string> PromptVersionControl::getGenealogy(
- Line 595: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_map<std::string, std::string> genealogy;
- Line 835: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: edits.push_back({Op::KEEP, lines_a[i - 1]});
- Line 839: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: edits.push_back({Op::ADD, lines_b[j - 1]});
- Line 842: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: edits.push_back({Op::REMOVE, lines_a[i - 1]});
- Line 1023: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: changes[base_idx].insertions_before.push_back(e.second);
- Line 1026: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: eof_slot.insertions_before.push_back(e.second);
- Line 1045: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_set<std::string> sc_ins_set(
- Line 1046: severity=MEDIUM; category=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: for (const auto& ins : sc.insertions_before) merged.push_back(ins);
- Line 1062: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: merged.push_back(base_lines[i]);
- Line 1068: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_set<std::string> src_eof_set(
- Line 1082: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: for (const auto& l : lines) { out += l; out += '\n'; }

### prompt_engineering/meta_prompt_generator.cpp
Total findings: 29

- Line 86: severity=HIGH; category=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: std::string llm_response = llm_provider_->complete(result.meta_prompt);
- Line 89: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: std::string llm_response = llm_provider_->complete(result.meta_prompt);

            if (!llm_response.empty()) {

                result.improvement_suggestion = llm_response;

                result.metadata["llm_provider"] = llm_provider_->name();

                result.metadata["llm_generated"] = true;

                THEMIS_DEBUG("LLM provider returned {} chars", llm_response.size());

                // key_insights remain from the template-based path below
- Line 90: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: if (!llm_response.empty()) {

                result.improvement_suggestion = llm_response;

                result.metadata["llm_provider"] = llm_provider_->name();

                result.metadata["llm_generated"] = true;

                THEMIS_DEBUG("LLM provider returned {} chars", llm_response.size());

                // key_insights remain from the template-based path below

            } else {
- Line 134: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: }

    

    result.metadata["score"] = score;

    result.metadata["strategy"] = config_.improvement_strategy;

    result.metadata["original_length"] = original_prompt.length();

    

    THEMIS_DEBUG("Generated meta-prompt of length {}", result.meta_prompt.length());
- Line 135: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: result.metadata["score"] = score;

    result.metadata["strategy"] = config_.improvement_strategy;

    result.metadata["original_length"] = original_prompt.length();

    

    THEMIS_DEBUG("Generated meta-prompt of length {}", result.meta_prompt.length());
- Line 55: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: meta_prompt << "Current Score: " << score << " / 1.0\n";
- Line 86: severity=MEDIUM; category=missing_resource_limits
  Description: LLM inference without token limit or timeout (DOS risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: std::string llm_response = llm_provider_->complete(result.meta_prompt);
- Line 181: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: suggestions.push_back("Break down instructions into numbered steps");
- Line 182: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: suggestions.push_back("Use clear, simple language");
- Line 183: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: suggestions.push_back("Define any technical terms");
- Line 188: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: suggestions.push_back("Add 2-3 concrete examples");
- Line 189: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: suggestions.push_back("Show both simple and complex cases");
- Line 190: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: suggestions.push_back("Highlight key patterns in examples");
- Line 195: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: suggestions.push_back("Specify exact output format");
- Line 196: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: suggestions.push_back("Provide output template or schema");
- Line 197: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: suggestions.push_back("Include formatting requirements");
- Line 202: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: suggestions.push_back("List explicit constraints");
- Line 203: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: suggestions.push_back("Define boundary conditions");
- Line 204: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: suggestions.push_back("Specify error handling requirements");
- Line 209: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: suggestions.push_back("Review and simplify language");
- Line 210: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: suggestions.push_back("Add structure with headers");
- Line 211: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: suggestions.push_back("Include validation criteria");
- Line 260: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: patterns.push_back("High-performing prompts include concrete examples");
- Line 264: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: patterns.push_back("Successful prompts use step-by-step instructions");
- Line 268: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: patterns.push_back("Clear output format specifications improve performance");
- Line 272: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: patterns.push_back("Explicit constraints help guide the model");
- Line 276: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: patterns.push_back("Structure prompts with clear sections (task, examples, output)");
- Line 277: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: patterns.push_back("Use precise, unambiguous language");
- Line 278: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: patterns.push_back("Include both positive and negative examples when relevant");

### prompt_engineering/dspy_module.cpp
Total findings: 26

- Line 207: severity=CRITICAL; category=sensitive_data_logging
  Description: Potential PII/credential logging: token
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: response << token << ": [echo]\n";
- Line 145: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: for (const auto& suffix : {std::string(":"), std::string(" :")}) {
- Line 147: severity=HIGH; category=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: auto other_pos = response.find(other_marker, value_start);
- Line 246: severity=HIGH; category=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: std::string response = llm_provider_->complete(prompt);
- Line 270: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 280: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 68: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: const std::unordered_map<std::string, std::string>& context) const
- Line 105: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_map<std::string, std::string> DspySignature::parseResponse(
- Line 108: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_map<std::string, std::string> parsed;
- Line 171: severity=MEDIUM; category=missing_resource_limits
  Description: LLM inference without token limit or timeout (DOS risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: std::string EchoDspyLLMProvider::complete(const std::string& prompt)
- Line 246: severity=MEDIUM; category=missing_resource_limits
  Description: LLM inference without token limit or timeout (DOS risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: std::string response = llm_provider_->complete(prompt);
- Line 52: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: inputs_.push_back(std::move(field));
- Line 58: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: outputs_.push_back(std::move(field));
- Line 64: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: const std::vector<DspyField>& DspySignature::inputs()  const { return inputs_;  }
- Line 65: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: const std::vector<DspyField>& DspySignature::outputs() const { return outputs_; }
- Line 78: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: for (const auto& field : inputs_) {
- Line 94: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: for (const auto& field : outputs_) {
- Line 110: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: for (const auto& field : outputs_) {
- Line 142: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: for (const auto& other : outputs_) {
- Line 269: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: // Rebuild outputs with Reasoning prepended
- Line 270: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: std::vector<DspyField> new_outputs;
- Line 271: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: new_outputs.push_back(std::move(reasoning_field));
- Line 272: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: for (const auto& f : signature.outputs()) {
- Line 273: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: new_outputs.push_back(f);
- Line 277: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: for (const auto& f : signature.inputs()) {
- Line 280: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: for (auto& f : new_outputs) {

### prompt_engineering/prompt_evaluator.cpp
Total findings: 24

- Line 49: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: metrics.details["embedding_provider"] = embedding_provider_->name();
- Line 486: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: auto v1 = embedding_provider_->embed(s1);
- Line 487: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: auto v2 = embedding_provider_->embed(s2);
- Line 240: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 253: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 270: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 305: severity=HIGH; category=fp_exact_comparison
  Description: Floating-point exact comparison (use tolerance/epsilon)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: if (x == 0.0) return 0.0;
- Line 306: severity=HIGH; category=fp_exact_comparison
  Description: Floating-point exact comparison (use tolerance/epsilon)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: if (x == 1.0) return 1.0;
- Line 339: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: for (int m = 1; m <= MAX_ITER; ++m) {

            double m2 = 2.0 * m;

            // Even step

            double dm = m * (bb - m) * xx / ((qam + m2) * (aa + m2));

            d = 1.0 + dm * d;

            if (std::abs(d) < 1e-30) d = 1e-30;

            c = 1.0 + dm / c;
- Line 347: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: d = 1.0 / d;

            h *= d * c;

            // Odd step

            dm = -(aa + m) * (qab + m) * xx / ((aa + m2) * (qap + m2));

            d = 1.0 + dm * d;

            if (std::abs(d) < 1e-30) d = 1e-30;

            c = 1.0 + dm / c;
- Line 90: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: similarities.push_back(metrics.semantic_similarity);
- Line 160: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_set<std::string> set1(tokens1.begin(), tokens1.end());
- Line 161: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_set<std::string> set2(tokens2.begin(), tokens2.end());
- Line 223: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_set<std::string> output_set(tokens_output.begin(), tokens_output.end());
- Line 382: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: result += ' ';
- Line 486: severity=MEDIUM; category=missing_resource_limits
  Description: LLM inference without token limit or timeout (DOS risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: auto v1 = embedding_provider_->embed(s1);
- Line 487: severity=MEDIUM; category=missing_resource_limits
  Description: LLM inference without token limit or timeout (DOS risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: auto v2 = embedding_provider_->embed(s2);
- Line 68: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: const std::vector<std::string>& outputs,
- Line 73: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: if (outputs.size() != expected.size()) {
- Line 78: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: if (outputs.empty()) {
- Line 86: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: for (size_t i = 0; i < outputs.size(); ++i) {
- Line 87: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: auto metrics = evaluateSingle(outputs[i], expected[i]);
- Line 319: severity=LOW; category=unstructured_log
  Description: Unstructured logging (use structured format)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: return std::log(M_PI / std::sin(M_PI * z)) - std::lgamma(1.0 - z);
- Line 324: severity=LOW; category=unstructured_log
  Description: Unstructured logging (use structured format)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: double front = std::exp(aa * std::log(xx) + bb * std::log(1.0 - xx) - log_beta) / aa;

### prompt_engineering/self_improvement_orchestrator.cpp
Total findings: 19

- Line 205: severity=CRITICAL; category=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: // between (prompt + input) and expected_output, which serves as a proxy for
- Line 223: severity=CRITICAL; category=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: prompt_with_inputs.push_back(prompt + "\n" + tc.input);
- Line 611: severity=CRITICAL; category=new_without_raii
  Description: Raw new() without RAII wrapper — potential memory leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: updated_template.metadata["last_deployed"] = 

        std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());

    

    // Create new version (in production, this would use PromptVersionControl)

    manager_->createTemplate(updated_template);

    

    THEMIS_INFO("Deployed optimized version for prompt: {}", prompt_id);
- Line 222: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 268: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: config_.ab_test_sample_size

                );

                

                result.metadata["ab_test_id"] = test_id;

                

                THEMIS_INFO("Optimization completed, starting A/B test: {}", test_id);

            } else {
- Line 288: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: } catch (const std::exception& e) {

        THEMIS_ERROR("Optimization failed: {}", e.what());

        result.status = OptimizationStatus::FAILED;

        result.metadata["error"] = e.what();

    }

    

    result.completed_at = std::chrono::system_clock::now();
- Line 348: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 349: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 353: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 354: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 398: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: // Update optimization history

        for (auto& results : optimization_history_[test.prompt_id]) {

            if (results.metadata.contains("ab_test_id") && 

                results.metadata["ab_test_id"] == test_id) {

                results.status = OptimizationStatus::DEPLOYED;

            }

        }
- Line 409: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: // Update optimization history

        for (auto& results : optimization_history_[test.prompt_id]) {

            if (results.metadata.contains("ab_test_id") && 

                results.metadata["ab_test_id"] == test_id) {

                results.status = OptimizationStatus::COMPLETED;

            }

        }
- Line 608: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: updated_template.content = version;

    

    // Update metadata to track deployment

    updated_template.metadata["last_deployed"] = 

        std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());

    

    // Create new version (in production, this would use PromptVersionControl)
- Line 611: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 213: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: // Guard: evaluateBatch() returns 0.0 for empty inputs, which would
- Line 216: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: std::vector<std::string> prompt_with_inputs;
- Line 218: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: prompt_with_inputs.reserve(cases.size());
- Line 223: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: prompt_with_inputs.push_back(prompt + "\n" + tc.input);
- Line 226: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: return evaluator_->evaluateBatch(prompt_with_inputs, expected).overall_score;

### prompt_engineering/feedback_collector.cpp
Total findings: 18

- Line 448: severity=HIGH; category=delete_without_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: entries.erase(it, entries.end());

    }

    

    // Also delete from DB if available: delete both primary records and index entries

    if (db_ && deleted > 0) {

        // Collect entries to delete by scanning; we need the full entry to

        // also remove the time-index key.
- Line 448: severity=HIGH; category=explicit_delete
  Description: Explicit delete statement (prefer smart pointers)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_raii
  Context: // Also delete from DB if available: delete both primary records and index entries
- Line 450: severity=HIGH; category=delete_without_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: // Also delete from DB if available: delete both primary records and index entries

    if (db_ && deleted > 0) {

        // Collect entries to delete by scanning; we need the full entry to

        // also remove the time-index key.

        std::vector<FeedbackEntry> to_delete;

        std::string prefix = KEY_PREFIX;
- Line 450: severity=HIGH; category=explicit_delete
  Description: Explicit delete statement (prefer smart pointers)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_raii
  Context: // Collect entries to delete by scanning; we need the full entry to
- Line 492: severity=HIGH; category=delete_without_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: size_t count = it->second.size();

    feedback_.erase(it);

    

    // Delete from DB if available: delete both primary records and index entries

    if (db_) {

        std::string prompt_prefix = std::string(KEY_PREFIX) + prompt_id + ":";

        // Collect full entries so we can also remove the secondary index keys
- Line 492: severity=HIGH; category=explicit_delete
  Description: Explicit delete statement (prefer smart pointers)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_raii
  Context: // Delete from DB if available: delete both primary records and index entries
- Line 779: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: for (size_t i = 0; i < std::min(size_t(3), sorted_types.size()); ++i) {
- Line 810: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: if (STOP_WORDS.find(cur) == STOP_WORDS.end() && cur.size() >= 2) {
- Line 846: severity=HIGH; category=unchecked_array_index
  Description: Array index not validated before access
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::input_validation
  Context: ['    // becomes the representative for the pattern.', '    auto best_keyword = [&](size_t entry_idx) -> std::string {', '        const auto& tokens = per_entry_tokens[entry_idx];', '        if (tokens.empty()) return "[empty]";', '']
- Line 906: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 112: severity=MEDIUM; category=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: auto time_val = j["timestamp"].get<std::time_t>();
- Line 225: severity=MEDIUM; category=missing_resource_limits
  Description: LLM inference without token limit or timeout (DOS risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: summary.reason_embedding    = embedding_model_->embed(query);
- Line 408: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: result.push_back(FeedbackEntry::fromJson(j));
- Line 463: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: to_delete.push_back(FeedbackEntry::fromJson(j));
- Line 500: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: to_delete.push_back(FeedbackEntry::fromJson(j));
- Line 828: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_map<std::string, size_t> document_frequency;
- Line 836: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_set<std::string> seen(tokens.begin(), tokens.end());
- Line 850: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_map<std::string, size_t> tf;

### prompt_engineering/tree_of_thoughts.cpp
Total findings: 16

- Line 159: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: auto root_thoughts = generator_->generate(problem, {}, config_.branching_factor);
- Line 205: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: auto children = generator_->generate(problem, child_path, config_.branching_factor);
- Line 233: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: auto root_thoughts = generator_->generate(problem, {}, config_.branching_factor);
- Line 277: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: auto children = generator_->generate(problem, child_path, config_.branching_factor);
- Line 305: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: auto root_thoughts = generator_->generate(problem, {}, config_.branching_factor);
- Line 343: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: auto children = generator_->generate(problem, child_path, config_.branching_factor);
- Line 30: severity=MEDIUM; category=missing_resource_limits
  Description: LLM inference without token limit or timeout (DOS risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: std::vector<std::string> HeuristicThoughtGenerator::generate(
- Line 159: severity=MEDIUM; category=missing_resource_limits
  Description: LLM inference without token limit or timeout (DOS risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: auto root_thoughts = generator_->generate(problem, {}, config_.branching_factor);
- Line 204: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: child_path.push_back(node.thought);
- Line 205: severity=MEDIUM; category=missing_resource_limits
  Description: LLM inference without token limit or timeout (DOS risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: auto children = generator_->generate(problem, child_path, config_.branching_factor);
- Line 233: severity=MEDIUM; category=missing_resource_limits
  Description: LLM inference without token limit or timeout (DOS risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: auto root_thoughts = generator_->generate(problem, {}, config_.branching_factor);
- Line 276: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: child_path.push_back(node.thought);
- Line 277: severity=MEDIUM; category=missing_resource_limits
  Description: LLM inference without token limit or timeout (DOS risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: auto children = generator_->generate(problem, child_path, config_.branching_factor);
- Line 305: severity=MEDIUM; category=missing_resource_limits
  Description: LLM inference without token limit or timeout (DOS risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: auto root_thoughts = generator_->generate(problem, {}, config_.branching_factor);
- Line 343: severity=MEDIUM; category=missing_resource_limits
  Description: LLM inference without token limit or timeout (DOS risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: auto children = generator_->generate(problem, child_path, config_.branching_factor);
- Line 457: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: out << "Then assign a verdict: sure / maybe / impossible.\n";

### prompt_engineering/prompt_compressor.cpp
Total findings: 14

- Line 220: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: for (size_t i = std::max(sys_end, tail_start);
- Line 275: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: for (size_t i = std::max(sys_end, tail_start);
- Line 44: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: if (!current.empty()) current += '\n';
- Line 66: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: if (i > 0) result += ' ';
- Line 67: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: if (i > 0) result += ' ';
- Line 218: severity=MEDIUM; category=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: kept_indices.push_back(i);
- Line 226: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: if (!result.empty()) result += "\n\n";
- Line 227: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: if (!result.empty()) result += "\n\n";
- Line 258: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: if (!middle.empty()) middle += "\n\n";
- Line 259: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: if (!middle.empty()) middle += "\n\n";
- Line 265: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: if (!result.empty()) result += "\n\n";
- Line 266: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: if (!result.empty()) result += "\n\n";
- Line 276: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: if (!result.empty()) result += "\n\n";
- Line 277: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: if (!result.empty()) result += "\n\n";

### prompt_engineering/prompt_manager.cpp
Total findings: 10

- Line 219: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: return false;

    }

    

    acc->second.metadata["experiment_id"] = experiment_id;



    if (db_) {

        std::string key = std::string(KEY_PREFIX) + id;
- Line 92: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: acc.release(); // Release lock
- Line 285: severity=MEDIUM; category=generic_catch
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: YAML::Emitter emitter;

                    emitter << prompt_node["metadata"];

                    pt.metadata = nlohmann::json::parse(emitter.c_str());

                } catch (...) {

                    pt.metadata = nlohmann::json::object();

                }

            }
- Line 285: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: } catch (...) {
- Line 332: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: const std::unordered_map<std::string, std::string>& context) const {
- Line 431: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: const std::unordered_map<std::string, std::string>& context) {
- Line 445: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: result += "\n\n[Images]\n";
- Line 449: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: result += std::to_string(i + 1) + ". [" + mime + "] " + img.alt_text + "\n";
- Line 452: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: result += "   Description: " + img.description + "\n";
- Line 455: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: result += "   URL: " + img.url + "\n";

### prompt_engineering/reflection_tuner.cpp
Total findings: 9

- Line 348: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: if (lower_resp.find(pattern) != std::string::npos) {
- Line 491: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: score            = provider_->score(prompt, revised_response);



        // Store the critique prompt in metadata for observability.

        step.metadata["critique_prompt"] =

            prompt_builder_.buildCritiquePrompt(prompt, current_response, ctx);

    } else {

        // Fallback: template-based critique + heuristic score.
- Line 526: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: revised_response = current_response;  // cannot revise without an LLM

        score            = computeHeuristicScore(prompt, revised_response);



        step.metadata["critique_prompt"]  = critique_prompt;

        step.metadata["revision_prompt"]  =

            prompt_builder_.buildRevisionPrompt(prompt, current_response, critique, ctx);

    }
- Line 527: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: score            = computeHeuristicScore(prompt, revised_response);



        step.metadata["critique_prompt"]  = critique_prompt;

        step.metadata["revision_prompt"]  =

            prompt_builder_.buildRevisionPrompt(prompt, current_response, critique, ctx);

    }
- Line 561: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: ReflectionResult ReflectionTuner::tune(const std::string& prompt,

                                        const std::string& initial_response) {

    ReflectionResult result;

    result.metadata["strategy"]       = static_cast<int>(config_.strategy);

    result.metadata["max_iterations"] = config_.max_iterations;

    result.metadata["provider"]       =

        provider_ ? provider_->name() : std::string("fallback");
- Line 562: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: const std::string& initial_response) {

    ReflectionResult result;

    result.metadata["strategy"]       = static_cast<int>(config_.strategy);

    result.metadata["max_iterations"] = config_.max_iterations;

    result.metadata["provider"]       =

        provider_ ? provider_->name() : std::string("fallback");
- Line 563: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: ReflectionResult result;

    result.metadata["strategy"]       = static_cast<int>(config_.strategy);

    result.metadata["max_iterations"] = config_.max_iterations;

    result.metadata["provider"]       =

        provider_ ? provider_->name() : std::string("fallback");



    std::string current_response = initial_response;
- Line 244: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: out << "Claim / Response:\n" << response << "\n\n";
- Line 554: severity=MEDIUM; category=missing_resource_limits
  Description: LLM inference without token limit or timeout (DOS risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: provider_ ? provider_->generate(prompt) : prompt;

### prompt_engineering/prompt_regression_runner.cpp
Total findings: 7

- Line 123: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: const std::vector<std::string>& baseline_outputs,
- Line 124: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: const std::vector<std::string>& candidate_outputs) const {
- Line 132: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: baseline_outputs.size(),
- Line 133: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: candidate_outputs.size()});
- Line 137: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: baseline_outputs.size() != candidate_outputs.size() ||
- Line 155: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: baseline_outputs[i], fixture.expected_output);
- Line 157: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: candidate_outputs[i], fixture.expected_output);

### prompt_engineering/prompt_ab_experiment.cpp
Total findings: 6

- Line 207: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: const double dm  = static_cast<double>(m);

        // Even step.

        double num = dm * (b - dm) * x /

                     ((a + 2.0 * dm - 1.0) * (a + 2.0 * dm));

        D = 1.0 + num * D;

        if (std::abs(D) < kEps) { D = kEps; }

        C = 1.0 + num / C;
- Line 215: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: D = 1.0 / D;

        f *= C * D;

        // Odd step.

        num = -(a + dm) * (a + b + dm) * x /

              ((a + 2.0 * dm) * (a + 2.0 * dm + 1.0));

        D = 1.0 + num * D;

        if (std::abs(D) < kEps) { D = kEps; }
- Line 216: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: f *= C * D;

        // Odd step.

        num = -(a + dm) * (a + b + dm) * x /

              ((a + 2.0 * dm) * (a + 2.0 * dm + 1.0));

        D = 1.0 + num * D;

        if (std::abs(D) < kEps) { D = kEps; }

        C = 1.0 + num / C;
- Line 279: severity=HIGH; category=memory_order
  Description: memory_order_relaxed used — potential visibility issue
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: ss << "exp-" << counter.fetch_add(1, std::memory_order_relaxed);
- Line 193: severity=LOW; category=unstructured_log
  Description: Unstructured logging (use structured format)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: const double front  = std::exp(std::log(x) * a +
- Line 194: severity=LOW; category=unstructured_log
  Description: Unstructured logging (use structured format)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: std::log(1.0 - x) * b - lnBeta) / a;

### prompt_engineering/structured_output.cpp
Total findings: 5

- Line 157: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: for (auto it = begin; it != std::sregex_iterator(); ++it) {
- Line 303: severity=HIGH; category=repeated_search
  Description: find/search in loop — O(n²) or worse
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: const bool found = std::find(actual_keys.begin(), actual_keys.end(), req)
- Line 315: severity=HIGH; category=repeated_search
  Description: find/search in loop — O(n²) or worse
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: const bool known = std::find(allowed.begin(), allowed.end(), k)
- Line 42: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: static const std::regex line_comment(R"(//[^\n]*)", std::regex::ECMAScript);
- Line 158: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: result.push_back((*it)[1].str());

### prompt_engineering/llm_reflection_adapter.cpp
Total findings: 4

- Line 29: severity=MEDIUM; category=missing_resource_limits
  Description: LLM inference without token limit or timeout (DOS risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: std::string ILLMProviderReflectionAdapter::generate(
- Line 32: severity=MEDIUM; category=missing_resource_limits
  Description: LLM inference without token limit or timeout (DOS risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: return llm_->complete(prompt);
- Line 42: severity=MEDIUM; category=missing_resource_limits
  Description: LLM inference without token limit or timeout (DOS risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: return llm_->complete(critique_prompt);
- Line 53: severity=MEDIUM; category=missing_resource_limits
  Description: LLM inference without token limit or timeout (DOS risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: return llm_->complete(revision_prompt);

### prompt_engineering/prompt_engineering_integration.cpp
Total findings: 4

- Line 220: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: std::this_thread::sleep_for(std::chrono::seconds(1));
- Line 92: severity=MEDIUM; category=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: auto timestamp = j["start_time"].get<std::time_t>();
- Line 462: severity=MEDIUM; category=missing_resource_limits
  Description: LLM inference without token limit or timeout (DOS risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: snap_metrics->recordReflectionCycleComplete(
- Line 606: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_map<std::string, std::string> ctx_map;

### prompt_engineering/prompt_engineering_metrics.cpp
Total findings: 4

- Line 727: severity=HIGH; category=memory_order
  Description: memory_order_relaxed used — potential visibility issue
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: if (snapshot.contains(key)) target.store(snapshot[key].get<int64_t>(), std::memory_order_relaxed);
- Line 730: severity=HIGH; category=memory_order
  Description: memory_order_relaxed used — potential visibility issue
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: if (snapshot.contains(key)) target.store(snapshot[key].get<int>(), std::memory_order_relaxed);
- Line 733: severity=HIGH; category=memory_order
  Description: memory_order_relaxed used — potential visibility issue
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: if (snapshot.contains(key)) target.store(snapshot[key].get<double>(), std::memory_order_relaxed);
- Line 795: severity=MEDIUM; category=missing_resource_limits
  Description: LLM inference without token limit or timeout (DOS risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: void PromptEngineeringMetrics::recordReflectionCycleComplete(

### prompt_engineering/prompt_injection_detector.cpp
Total findings: 4

- Line 110: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: if (lower.find(kw) != std::string::npos) {
- Line 124: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: text.find("[/INST]") != std::string::npos) {
- Line 44: severity=MEDIUM; category=duplicate_qualified_signature
  Description: Potential duplicate implementation signature across files: PromptInjectionDetector::initializePatterns()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::legacy_duplication
  Context: void PromptInjectionDetector::initializePatterns() {
- Line 260: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: pos += 10; // len("[REDACTED]")

### prompt_engineering/prompt_optimizer.cpp
Total findings: 4

- Line 112: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: result.final_score = current_score;

    result.converged = result.converged || (current_score >= config_.target_score);

    

    result.metadata["initial_score"] = result.score_history[0];

    result.metadata["improvement"] = current_score - result.score_history[0];

    result.metadata["relative_improvement"] = 

        (result.score_history[0] > 0) ?
- Line 113: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: result.converged = result.converged || (current_score >= config_.target_score);

    

    result.metadata["initial_score"] = result.score_history[0];

    result.metadata["improvement"] = current_score - result.score_history[0];

    result.metadata["relative_improvement"] = 

        (result.score_history[0] > 0) ? 

        (current_score - result.score_history[0]) / result.score_history[0] : 0.0;
- Line 114: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: result.metadata["initial_score"] = result.score_history[0];

    result.metadata["improvement"] = current_score - result.score_history[0];

    result.metadata["relative_improvement"] = 

        (result.score_history[0] > 0) ? 

        (current_score - result.score_history[0]) / result.score_history[0] : 0.0;
- Line 169: severity=HIGH; category=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: auto meta_result = meta_gen.generateImprovementPrompt(

### prompt_engineering/prompt_performance_tracker.cpp
Total findings: 4

- Line 103: severity=CRITICAL; category=smart_ptr_misuse
  Description: Raw new without immediate wrapping in smart pointer
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: THEMIS_DEBUG("Created new metrics for prompt: {}", prompt_id);
- Line 94: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 59: severity=MEDIUM; category=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: auto time_val = j["last_updated"].get<std::time_t>();
- Line 64: severity=MEDIUM; category=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: auto time_val = j["created_at"].get<std::time_t>();

### prompt_engineering/rag_context_budget_manager.cpp
Total findings: 4

- Line 30: severity=HIGH; category=missing_trace_point
  Description: Critical function allocate without trace point
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: BudgetHandle RagContextBudgetManager::allocate(size_t tokens) {
- Line 38: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: ? (total_budget_ - current_allocated)

                             : 0;

    if (tokens > avail) {

        throw BudgetExhaustedError(tokens, avail, total_budget_);

    }



    allocated_.fetch_add(tokens, std::memory_order_relaxed);
- Line 72: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: allocated_.store(0, std::memory_order_relaxed);
- Line 77: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: const size_t alloc = allocated_.load(std::memory_order_relaxed);

### prompt_engineering/context_window_manager.cpp
Total findings: 2

- Line 187: severity=HIGH; category=repeated_search
  Description: find/search in loop — O(n²) or worse
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: (std::find(t.activation_rounds.begin(),
- Line 198: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: auto it = t->round_role_weights.find(round_role);

### prompt_engineering/prompt_library_io.cpp
Total findings: 2

- Line 66: severity=MEDIUM; category=generic_catch
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: try {

            t.metadata = nlohmann::json::parse(

                node["metadata"].as<std::string>());

        } catch (...) {

            t.metadata = nlohmann::json::object();

        }

    }
- Line 66: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: } catch (...) {

### prompt_engineering/FUTURE_ENHANCEMENTS.md
Total findings: 1

- Line 1: severity=LOW; category=module_doc_linkset_drift
  Description: Module doc 'FUTURE_ENHANCEMENTS.md' is missing expected cross-links: ARCHITECTURE.md
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::themis_doc_freshness_rules
  Context: Refresh the top-level <!-- Links: ... --> metadata to match the current module doc set

### prompt_engineering/PRODUCTION_REQUIREMENTS.md
Total findings: 1

- Line 1: severity=LOW; category=module_doc_linkset_drift
  Description: Module doc 'PRODUCTION_REQUIREMENTS.md' is missing expected cross-links: FUTURE_ENHANCEMENTS.md, README.md, ROADMAP.md
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::themis_doc_freshness_rules
  Context: Refresh the top-level <!-- Links: ... --> metadata to match the current module doc set

### prompt_engineering/markdown_utils.cpp
Total findings: 1

- Line 59: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: static const std::regex line_comment(R"(//[^\r\n]*)",

### prompt_engineering/prompt_quality_evaluator.cpp
Total findings: 1

- Line 123: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_map<std::string, size_t> bigram_counts;

### prompt_engineering/system_prompt_manager.cpp
Total findings: 1

- Line 76: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: const std::unordered_map<std::string, std::string>& context) {

## Update Workflow

- Refresh scanner artifacts with: python tools/gs3_orchestrator.py ./src --output ai_working/gap_scan_results.json
- Regenerate docs with: python tools/module_doc_generator.py . ai_working ai_working/module_gaps
- Add --no-mirror when you only want archive docs in ai_working/module_gaps.

Format: THEMIS_MODULE_GAPS_V4
