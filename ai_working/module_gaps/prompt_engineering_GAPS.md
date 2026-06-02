# prompt_engineering Module - Developer Gap Note

> Auto-generated from ai_working/gap_scan_v3_aggregate.json.
> This file is overwritten on each regeneration.

## Scan Snapshot

- Module: prompt_engineering
- Generated: 2026-06-02 11:55:48
- Status: Critical Findings Present
- Total Findings: 491
- Actionable Findings (Critical + High): 208
- Affected Files: 31

## Severity Summary

| Severity | Count |
|---|---:|
| Critical | 36 |
| High | 172 |
| Medium | 279 |
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
| src/prompt_engineering/prompt_template_compiler.cpp | 64 | 0 | 49 | 15 | 0 |
| src/prompt_engineering/prompt_version_control.cpp | 58 | 5 | 15 | 38 | 0 |
| src/prompt_engineering/dspy_module.cpp | 41 | 8 | 27 | 6 | 0 |
| src/prompt_engineering/self_improvement_orchestrator.cpp | 35 | 9 | 21 | 5 | 0 |
| src/prompt_engineering/tree_of_thoughts.cpp | 29 | 6 | 0 | 23 | 0 |
| src/prompt_engineering/meta_prompt_generator.cpp | 28 | 2 | 2 | 24 | 0 |
| src/prompt_engineering/feedback_collector.cpp | 27 | 0 | 5 | 22 | 0 |
| src/prompt_engineering/prompt_manager.cpp | 22 | 0 | 4 | 18 | 0 |
| src/prompt_engineering/prompt_evaluator.cpp | 21 | 3 | 10 | 6 | 2 |
| src/prompt_engineering/prompt_compressor.cpp | 17 | 0 | 2 | 15 | 0 |
| src/prompt_engineering/structured_output.cpp | 16 | 0 | 3 | 13 | 0 |
| src/prompt_engineering/prompt_injection_detector.cpp | 15 | 0 | 3 | 12 | 0 |
| src/prompt_engineering/reflection_tuner.cpp | 15 | 0 | 7 | 8 | 0 |
| src/prompt_engineering/prompt_library_io.cpp | 14 | 0 | 0 | 14 | 0 |
| src/prompt_engineering/prompt_regression_runner.cpp | 12 | 0 | 7 | 5 | 0 |
| src/prompt_engineering/context_window_manager.cpp | 9 | 0 | 3 | 6 | 0 |
| src/prompt_engineering/protegi_optimizer.cpp | 9 | 1 | 1 | 7 | 0 |
| src/prompt_engineering/prompt_ab_experiment.cpp | 8 | 0 | 1 | 5 | 2 |
| src/prompt_engineering/prompt_performance_tracker.cpp | 7 | 1 | 1 | 5 | 0 |
| src/prompt_engineering/prompt_quality_evaluator.cpp | 7 | 0 | 0 | 7 | 0 |
| src/prompt_engineering/prompt_engineering_integration.cpp | 6 | 0 | 1 | 5 | 0 |
| src/prompt_engineering/cot_tracer.cpp | 5 | 0 | 0 | 5 | 0 |
| src/prompt_engineering/llm_reflection_adapter.cpp | 4 | 0 | 0 | 4 | 0 |
| src/prompt_engineering/prompt_engineering_metrics.cpp | 4 | 0 | 3 | 1 | 0 |
| src/prompt_engineering/rag_context_budget_manager.cpp | 4 | 0 | 4 | 0 | 0 |
| src/prompt_engineering/prompt_optimizer.cpp | 3 | 0 | 3 | 0 | 0 |
| src/prompt_engineering/prompt_template_validator.cpp | 3 | 0 | 0 | 3 | 0 |
| src/prompt_engineering/adversarial_prompt_tester.cpp | 2 | 0 | 0 | 2 | 0 |
| src/prompt_engineering/markdown_utils.cpp | 2 | 1 | 0 | 1 | 0 |
| src/prompt_engineering/rag_prompt_builder.cpp | 2 | 0 | 0 | 2 | 0 |
| src/prompt_engineering/system_prompt_manager.cpp | 2 | 0 | 0 | 2 | 0 |

## Full Scanner Findings

### src/prompt_engineering/prompt_template_compiler.cpp
Total findings: 64

- Line 155: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw PromptTemplateCompileError(
- Line 174: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw PromptTemplateCompileError(
- Line 183: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw PromptTemplateCompileError(
- Line 235: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: node->kind = detail::ASTNode::Kind::TEXT;
- Line 236: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: node->text = tok.value;
- Line 244: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: node->kind = detail::ASTNode::Kind::SLOT;
- Line 245: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: node->text = tok.value; // name stored in .text
- Line 249: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: node->required      = it->second.required;
- Line 250: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: node->default_value = it->second.default_value;
- Line 252: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: node->required = false;
- Line 261: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: node->kind = detail::ASTNode::Kind::IF;
- Line 262: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: node->name = tok.value; // condition variable
- Line 265: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: node->children = parse(tokens, idx, slot_index,
- Line 270: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: node->else_children = parse(tokens, idx, slot_index,
- Line 275: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw PromptTemplateCompileError(
- Line 285: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw PromptTemplateCompileError(
- Line 292: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw PromptTemplateCompileError(
- Line 299: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: node->kind    = detail::ASTNode::Kind::FOREACH;
- Line 300: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: node->name    = tok.value;  // item variable
- Line 301: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: node->list_var = tok.value2; // list slot name
- Line 303: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: node->children = parse(tokens, idx, slot_index,
- Line 306: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw PromptTemplateCompileError(
- Line 317: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw PromptTemplateCompileError(
- Line 339: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: switch (node->kind) {
- Line 341: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: out << node->text;
- Line 345: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: const std::string& name = node->text;
- Line 353: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: if (node->required) {
- Line 354: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw PromptTemplateMissingSlotError(name);
- Line 356: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: out << node->default_value;
- Line 364: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: const std::string& cond_var = node->name;
- Line 376: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: renderNodes(node->children, ctx, item_var, item_val, out);
- Line 377: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: } else if (!node->else_children.empty()) {
- Line 378: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: renderNodes(node->else_children, ctx, item_var, item_val, out);
- Line 384: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: const std::string& list_name = node->list_var;
- Line 385: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: const std::string& item_name = node->name;
- Line 394: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: renderNodes(node->children, ctx, item_name, elem, out);
- Line 398: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: renderNodes(node->children, ctx, item_name,
- Line 403: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: renderNodes(node->children, ctx, item_name,
- Line 424: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: switch (node->kind) {
- Line 429: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: const std::string& name = node->text;
- Line 430: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: if (node->required && ctx.find(name) == ctx.end()) {
- Line 431: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: if (node->required && ctx.find(name) == ctx.end()) {
- Line 438: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: validateNodes(node->children, ctx, item_var, errors);
- Line 439: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: if (!node->else_children.empty()) {
- Line 440: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: validateNodes(node->else_children, ctx, item_var, errors);
- Line 447: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: validateNodes(node->children, ctx, node->name, errors);
- Line 519: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw PromptTemplateCompileError(
- Line 529: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: if (slot_index.find(n->text) == slot_index.end()) {
- Line 547: severity=HIGH; category=repeated_search
  Description: find/search in loop — O(n²) or worse
  Remediation: Move search outside loop or build index/map before loop
  Context: if (std::find_if(final_slots.begin(), final_slots.end(),
- Line 142: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: tokens.push_back({TokenKind::SLOT, inner, {}});
- Line 164: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: tokens.push_back({TokenKind::IF, var, {}});
- Line 166: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: tokens.push_back({TokenKind::ELSE, {}, {}});
- Line 168: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: tokens.push_back({TokenKind::ENDIF, {}, {}});
- Line 179: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: tokens.push_back({TokenKind::FOR, item_var, list_var});
- Line 181: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: tokens.push_back({TokenKind::ENDFOR, {}, {}});
- Line 199: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: tokens.push_back({TokenKind::SLOT, inner, {}});
- Line 431: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: errors.push_back("Missing required slot: " + name);
  Confidence: band=high; score=0.74
- Line 432: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: errors.push_back("Missing required slot: " + name);
- Line 451: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 453: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: errors.push_back("Internal validation error for node");
- Line 489: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: slot_arr.push_back(s.toJson());
  Confidence: band=high; score=0.74
- Line 490: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: slot_arr.push_back(s.toJson());
- Line 505: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, SlotDefinition> slot_index;
  Confidence: band=medium; score=0.66
- Line 549: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: final_slots.push_back(sd);
  Confidence: band=high; score=0.74

### src/prompt_engineering/prompt_version_control.cpp
Total findings: 58

- Line 462: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator prompt_it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto prompt_it = branches_.find(prompt_id);
- Line 476: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator source_it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto source_it = versions_.find(source_id);
- Line 477: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator target_it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto target_it = versions_.find(target_id);
- Line 719: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: std::string input = prompt_id + content + parent +
  Confidence: band=very_high; score=0.99
- Line 723: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: SHA256(reinterpret_cast<const unsigned char*>(input.c_str()), input.length(), hash);
  Confidence: band=very_high; score=0.99
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 42: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: j["metadata"] = metadata;
- Line 719: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: std::string input = prompt_id + content + parent +
  Confidence: band=very_high; score=0.9
- Line 723: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: SHA256(reinterpret_cast<const unsigned char*>(input.c_str()), input.length(), hash);
  Confidence: band=very_high; score=0.9
- Line 880: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: for (size_t k = hunk_end; k < std::min(hunk_end + size_t(CONTEXT) * 2, N); ++k) {
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
- Line 835: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: edits.push_back({Op::KEEP, lines_a[i - 1]});
- Line 839: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: edits.push_back({Op::ADD, lines_b[j - 1]});
- Line 842: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: edits.push_back({Op::REMOVE, lines_a[i - 1]});
- Line 851: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: diff.removed_lines.push_back(e.line);
  Confidence: band=high; score=0.74
- Line 852: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: diff.removed_lines.push_back(e.line);
- Line 855: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: diff.added_lines.push_back(e.line);
- Line 976: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: edits.push_back({' ', from[i-1]});
  Confidence: band=high; score=0.74
- Line 976: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: edits.push_back({' ', from[i-1]});
  Confidence: band=high; score=0.74
- Line 977: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: edits.push_back({' ', from[i-1]});
- Line 981: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: edits.push_back({'+', to[j-1]});
- Line 984: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: edits.push_back({'-', from[i-1]});
- Line 1022: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: changes[base_idx].insertions_before.push_back(e.second);
  Confidence: band=high; score=0.74
- Line 1023: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: changes[base_idx].insertions_before.push_back(e.second);
- Line 1026: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: eof_slot.insertions_before.push_back(e.second);
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
- Line 1062: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: merged.push_back(base_lines[i]);
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
- Line 1101: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.conflicts.push_back(

### src/prompt_engineering/dspy_module.cpp
Total findings: 41

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
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
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
- Line 82: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw DspyMissingFieldError(field.name);
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
- Line 145: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: for (const auto& suffix : {std::string(":"), std::string(" :")}) {
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

### src/prompt_engineering/self_improvement_orchestrator.cpp
Total findings: 35

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
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 52: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: j["metadata"] = metadata;
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

### src/prompt_engineering/tree_of_thoughts.cpp
Total findings: 29

- Line 159: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto root_thoughts = generator_->generate(problem, {}, config_.branching_factor);
- Line 205: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto children = generator_->generate(problem, child_path, config_.branching_factor);
- Line 233: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto root_thoughts = generator_->generate(problem, {}, config_.branching_factor);
- Line 277: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto children = generator_->generate(problem, child_path, config_.branching_factor);
- Line 305: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto root_thoughts = generator_->generate(problem, {}, config_.branching_factor);
- Line 343: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto children = generator_->generate(problem, child_path, config_.branching_factor);
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
- Line 182: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.log.push_back(msg.str());
- Line 205: severity=MEDIUM; category=llm_ai_safety; pattern=missing_resource_limits
  Description: LLM inference without token limit or timeout (DOS risk)
  Context: auto children = generator_->generate(problem, child_path, config_.branching_factor);
  Confidence: band=high; score=0.74
- Line 214: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.best_path.push_back(best_node.thought);
  Confidence: band=high; score=0.74
- Line 215: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.best_path.push_back(best_node.thought);
- Line 233: severity=MEDIUM; category=llm_ai_safety; pattern=missing_resource_limits
  Description: LLM inference without token limit or timeout (DOS risk)
  Context: auto root_thoughts = generator_->generate(problem, {}, config_.branching_factor);
  Confidence: band=high; score=0.74
- Line 253: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.log.push_back(msg.str());
  Confidence: band=high; score=0.74
- Line 254: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.log.push_back(msg.str());
- Line 277: severity=MEDIUM; category=llm_ai_safety; pattern=missing_resource_limits
  Description: LLM inference without token limit or timeout (DOS risk)
  Context: auto children = generator_->generate(problem, child_path, config_.branching_factor);
  Confidence: band=high; score=0.74
- Line 285: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.best_path.push_back(best_node.thought);
  Confidence: band=high; score=0.74
- Line 286: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.best_path.push_back(best_node.thought);
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
- Line 334: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.best_path.push_back(node.thought);
- Line 341: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: child_path.push_back(node.thought);
- Line 343: severity=MEDIUM; category=llm_ai_safety; pattern=missing_resource_limits
  Description: LLM inference without token limit or timeout (DOS risk)
  Context: auto children = generator_->generate(problem, child_path, config_.branching_factor);
  Confidence: band=high; score=0.74
- Line 353: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.log.push_back(msg.str());
  Confidence: band=high; score=0.74
- Line 354: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.log.push_back(msg.str());
- Line 457: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: out << "Then assign a verdict: sure / maybe / impossible.\n";

### src/prompt_engineering/meta_prompt_generator.cpp
Total findings: 28

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
- Line 55: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: meta_prompt << "Current Score: " << score << " / 1.0\n";
- Line 86: severity=MEDIUM; category=llm_ai_safety; pattern=missing_resource_limits
  Description: LLM inference without token limit or timeout (DOS risk)
  Context: std::string llm_response = llm_provider_->complete(result.meta_prompt);
  Confidence: band=high; score=0.74
- Line 123: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.key_insights.push_back("Prompt lacks clarity or specificity");
- Line 124: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.key_insights.push_back("Consider restructuring with clear sections");
- Line 126: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.key_insights.push_back("Prompt is functional but can be improved");
- Line 127: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.key_insights.push_back("Focus on edge cases and formatting");
- Line 129: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.key_insights.push_back("Prompt performs well");
- Line 130: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.key_insights.push_back("Minor optimizations possible");
- Line 195: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: suggestions.push_back("Specify exact output format");
- Line 196: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: suggestions.push_back("Provide output template or schema");
- Line 197: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: suggestions.push_back("Include formatting requirements");
- Line 202: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: suggestions.push_back("List explicit constraints");
- Line 203: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: suggestions.push_back("Define boundary conditions");
- Line 204: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: suggestions.push_back("Specify error handling requirements");
- Line 209: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: suggestions.push_back("Review and simplify language");
- Line 210: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: suggestions.push_back("Add structure with headers");
- Line 211: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: suggestions.push_back("Include validation criteria");
- Line 260: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: patterns.push_back("High-performing prompts include concrete examples");
- Line 264: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: patterns.push_back("Successful prompts use step-by-step instructions");
- Line 268: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: patterns.push_back("Clear output format specifications improve performance");
- Line 272: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: patterns.push_back("Explicit constraints help guide the model");
- Line 276: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: patterns.push_back("Structure prompts with clear sections (task, examples, output)");
- Line 277: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: patterns.push_back("Use precise, unambiguous language");
- Line 278: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: patterns.push_back("Include both positive and negative examples when relevant");

### src/prompt_engineering/feedback_collector.cpp
Total findings: 27

- Line 0: severity=HIGH; category=uncategorized
  Context: ['    // becomes the representative for the pattern.', '    auto best_keyword = [&](size_t entry_idx) -> std::string {', '        const auto& tokens = per_entry_tokens[entry_idx];', '        if (tokens.empty()) return "[empty]";', '']
  Confidence: band=high; score=0.78
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 85: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: j["metadata"] = metadata;
- Line 779: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: for (size_t i = 0; i < std::min(size_t(3), sorted_types.size()); ++i) {
- Line 810: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: if (STOP_WORDS.find(cur) == STOP_WORDS.end() && cur.size() >= 2) {
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
- Line 408: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.push_back(FeedbackEntry::fromJson(j));
- Line 409: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {}
- Line 424: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(entry);
  Confidence: band=high; score=0.74
- Line 463: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: to_delete.push_back(FeedbackEntry::fromJson(j));
- Line 501: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {}
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
- Line 878: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: p.examples.push_back(entry.query);
- Line 895: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(pat);
  Confidence: band=high; score=0.74

### src/prompt_engineering/prompt_manager.cpp
Total findings: 22

- Line 389: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: context["table_count"] = std::to_string(metadata.table_count);
- Line 390: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: context["total_rows"] = std::to_string(metadata.total_rows);
- Line 397: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: context["capabilities"] = caps_array.dump();
- Line 409: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: context["tables"] = tables_array.dump(2);
- Line 57: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.errors.push_back(
  Confidence: band=high; score=0.74
- Line 58: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.errors.push_back(
- Line 65: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.errors.push_back("Template 'metadata' must be a JSON object");
- Line 92: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: acc.release(); // Release lock
- Line 128: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: t.images.push_back(ImageDescription::fromJson(img_j));
  Confidence: band=high; score=0.74
- Line 129: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: t.images.push_back(ImageDescription::fromJson(img_j));
- Line 167: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: t.images.push_back(ImageDescription::fromJson(img_j));
- Line 182: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: out.push_back(kv.second);
  Confidence: band=high; score=0.74
- Line 285: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
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
- Line 452: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: result += "   Description: " + img.description + "\n";
- Line 455: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: result += "   URL: " + img.url + "\n";

### src/prompt_engineering/prompt_evaluator.cpp
Total findings: 21

- Line 49: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: metrics.details["embedding_provider"] = embedding_provider_->name();
- Line 486: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto v1 = embedding_provider_->embed(s1);
- Line 487: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto v2 = embedding_provider_->embed(s2);
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
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
- Line 90: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: similarities.push_back(metrics.semantic_similarity);
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
- Line 382: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: result += ' ';
- Line 319: severity=LOW; category=observability; pattern=unstructured_log
  Description: Unstructured logging (use structured format)
  Context: return std::log(M_PI / std::sin(M_PI * z)) - std::lgamma(1.0 - z);
  Confidence: band=medium; score=0.6
- Line 324: severity=LOW; category=observability; pattern=unstructured_log
  Description: Unstructured logging (use structured format)
  Context: double front = std::exp(aa * std::log(xx) + bb * std::log(1.0 - xx) - log_beta) / aa;
  Confidence: band=medium; score=0.6

### src/prompt_engineering/prompt_compressor.cpp
Total findings: 17

- Line 220: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: for (size_t i = std::max(sys_end, tail_start);
- Line 275: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: for (size_t i = std::max(sys_end, tail_start);
- Line 44: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: if (!current.empty()) current += '\n';
- Line 66: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: if (i > 0) result += ' ';
  Confidence: band=high; score=0.74
- Line 67: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: if (i > 0) result += ' ';
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
- Line 227: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: if (!result.empty()) result += "\n\n";
- Line 258: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: if (!middle.empty()) middle += "\n\n";
  Confidence: band=high; score=0.74
- Line 259: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: if (!middle.empty()) middle += "\n\n";
- Line 265: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: if (!result.empty()) result += "\n\n";
  Confidence: band=high; score=0.74
- Line 266: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: if (!result.empty()) result += "\n\n";
- Line 276: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: if (!result.empty()) result += "\n\n";
  Confidence: band=high; score=0.74
- Line 277: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: if (!result.empty()) result += "\n\n";

### src/prompt_engineering/structured_output.cpp
Total findings: 16

- Line 157: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: for (auto it = begin; it != std::sregex_iterator(); ++it) {
- Line 303: severity=HIGH; category=repeated_search
  Description: find/search in loop — O(n²) or worse
  Remediation: Move search outside loop or build index/map before loop
  Context: const bool found = std::find(actual_keys.begin(), actual_keys.end(), req)
- Line 315: severity=HIGH; category=repeated_search
  Description: find/search in loop — O(n²) or worse
  Remediation: Move search outside loop or build index/map before loop
  Context: const bool known = std::find(allowed.begin(), allowed.end(), k)
- Line 42: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: static const std::regex line_comment(R"(//[^\n]*)", std::regex::ECMAScript);
- Line 65: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: errors.push_back("JSON output is empty");
- Line 71: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: errors.push_back("JSON output does not start with '{' or '['");
- Line 157: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back((*it)[1].str());
  Confidence: band=high; score=0.74
- Line 158: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.push_back((*it)[1].str());
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
- Line 306: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: errors.push_back("Missing required field: \"" + req + "\"");
- Line 317: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: errors.push_back("Unknown field not in schema properties: \"" +
  Confidence: band=high; score=0.74
- Line 318: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: errors.push_back("Unknown field not in schema properties: \"" +
- Line 345: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: errors.push_back("Output does not match regex pattern: " +
- Line 350: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: errors.push_back(std::string("Invalid regex pattern: ") + ex.what());

### src/prompt_engineering/prompt_injection_detector.cpp
Total findings: 15

- Line 110: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: if (lower.find(kw) != std::string::npos) {
- Line 110: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: if (lower.find(kw) != std::string::npos) {
- Line 124: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: text.find("[/INST]") != std::string::npos) {
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
- Line 113: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: matched_out.push_back("keyword:" + kw);
- Line 127: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: matched_out.push_back("syntax:instruction_bracket_token");
- Line 138: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: matched_out.push_back("syntax:high_special_char_density");
  Confidence: band=high; score=0.74
- Line 139: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: matched_out.push_back("syntax:high_special_char_density");
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

### src/prompt_engineering/reflection_tuner.cpp
Total findings: 15

- Line 34: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: // Linguistic markers used to infer self-reported confidence.
  Confidence: band=very_high; score=0.9
- Line 144: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: s["metadata"]                = step.metadata;
- Line 148: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: j["metadata"] = metadata;
- Line 348: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: if (lower_resp.find(pattern) != std::string::npos) {
- Line 348: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: if (lower_resp.find(pattern) != std::string::npos) {
- Line 526: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: step.metadata["critique_prompt"]  = critique_prompt;
- Line 527: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: step.metadata["revision_prompt"]  =
- Line 85: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: ctx.uncertainty_markers.push_back(marker);
  Confidence: band=high; score=0.74
- Line 144: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: steps_arr.push_back(s);
  Confidence: band=high; score=0.74
- Line 244: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: out << "Claim / Response:\n" << response << "\n\n";
- Line 244: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: out << "Claim / Response:\n" << response << "\n\n";
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
- Line 588: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.quality_trajectory.push_back(step.quality_score);

### src/prompt_engineering/prompt_library_io.cpp
Total findings: 14

- Line 66: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 77: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: t.images.push_back(img);
  Confidence: band=high; score=0.74
- Line 100: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: tpls.push_back(t.toJson());
  Confidence: band=high; score=0.74
- Line 101: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: tpls.push_back(t.toJson());
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
- Line 232: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 264: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 280: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 303: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: b.templates.push_back(templateFromYaml(node));
  Confidence: band=high; score=0.74
- Line 304: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: b.templates.push_back(templateFromYaml(node));
- Line 308: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 349: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {

### src/prompt_engineering/prompt_regression_runner.cpp
Total findings: 12

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
- Line 52: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: deltas.push_back({
- Line 105: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: fixtures_.push_back(std::move(f));
  Confidence: band=high; score=0.74
- Line 169: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: baseline_scores.push_back(bs);
  Confidence: band=high; score=0.74
- Line 251: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {

### src/prompt_engineering/context_window_manager.cpp
Total findings: 9

- Line 147: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw PromptBudgetExceededError(
- Line 187: severity=HIGH; category=repeated_search
  Description: find/search in loop — O(n²) or worse
  Remediation: Move search outside loop or build index/map before loop
  Context: (std::find(t.activation_rounds.begin(),
- Line 198: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: auto it = t->round_role_weights.find(round_role);
- Line 90: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: selected.push_back(chunk);
  Confidence: band=high; score=0.74
- Line 189: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: if (is_active) active.push_back(&t);
  Confidence: band=high; score=0.74
- Line 190: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: if (is_active) active.push_back(&t);
- Line 191: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: else           inactive.push_back(&t);
- Line 228: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(std::move(inj));
  Confidence: band=high; score=0.74
- Line 249: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(std::move(inj));
  Confidence: band=high; score=0.74

### src/prompt_engineering/protegi_optimizer.cpp
Total findings: 9

- Line 96: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: << "- Read the input carefully.\n"
  Confidence: band=very_high; score=0.99
- Line 96: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: << "- Read the input carefully.\n"
  Confidence: band=very_high; score=0.9
- Line 107: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: candidates.push_back(c.str());
- Line 114: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: candidates.push_back(c.str());
  Confidence: band=high; score=0.74
- Line 115: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: candidates.push_back(c.str());
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

### src/prompt_engineering/prompt_ab_experiment.cpp
Total findings: 8

- Line 279: severity=HIGH; category=memory_order
  Description: memory_order_relaxed used — potential visibility issue
  Remediation: Use memory_order_acquire/release unless truly lock-free
  Context: ss << "exp-" << counter.fetch_add(1, std::memory_order_relaxed);
- Line 326: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: out.push_back(exp);
  Confidence: band=high; score=0.74
- Line 432: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: : ExperimentStatus::WINNER_CONTROL;
- Line 445: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: try { cb(eid, winner, wid); } catch (...) {}
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
Total findings: 7

- Line 103: severity=CRITICAL; category=smart_ptr_misuse
  Description: Raw new without immediate wrapping in smart pointer
  Remediation: Use auto ptr = std::make_unique<T>(...);
  Context: THEMIS_DEBUG("Created new metrics for prompt: {}", prompt_id);
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
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
Total findings: 7

- Line 36: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: tokens.push_back(std::move(word));
  Confidence: band=high; score=0.74
- Line 72: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: failed.push_back(QualityCheck{
  Confidence: band=high; score=0.74
- Line 73: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: failed.push_back(QualityCheck{
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
- Line 139: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: failed.push_back(QualityCheck{

### src/prompt_engineering/prompt_engineering_integration.cpp
Total findings: 6

- Line 220: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: std::this_thread::sleep_for(std::chrono::seconds(1));
- Line 92: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto timestamp = j["start_time"].get<std::time_t>();
  Confidence: band=high; score=0.74
- Line 183: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: WorkerStatus status;
- Line 283: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: IntegrationStatus status;
- Line 462: severity=MEDIUM; category=llm_ai_safety; pattern=missing_resource_limits
  Description: LLM inference without token limit or timeout (DOS risk)
  Context: snap_metrics->recordReflectionCycleComplete(
  Confidence: band=high; score=0.74
- Line 606: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, std::string> ctx_map;
  Confidence: band=medium; score=0.66

### src/prompt_engineering/cot_tracer.cpp
Total findings: 5

- Line 95: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: arr.push_back(rec.toJson());
  Confidence: band=high; score=0.74
- Line 96: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: arr.push_back(rec.toJson());
- Line 165: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: children_.push_back(std::move(tracer));
  Confidence: band=high; score=0.74
- Line 203: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: arr.push_back(rec.toJson());
  Confidence: band=high; score=0.74
- Line 204: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: arr.push_back(rec.toJson());

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

### src/prompt_engineering/prompt_engineering_metrics.cpp
Total findings: 4

- Line 727: severity=HIGH; category=memory_order
  Description: memory_order_relaxed used — potential visibility issue
  Remediation: Use memory_order_acquire/release unless truly lock-free
  Context: if (snapshot.contains(key)) target.store(snapshot[key].get<int64_t>(), std::memory_order_relaxed);
- Line 730: severity=HIGH; category=memory_order
  Description: memory_order_relaxed used — potential visibility issue
  Remediation: Use memory_order_acquire/release unless truly lock-free
  Context: if (snapshot.contains(key)) target.store(snapshot[key].get<int>(), std::memory_order_relaxed);
- Line 733: severity=HIGH; category=memory_order
  Description: memory_order_relaxed used — potential visibility issue
  Remediation: Use memory_order_acquire/release unless truly lock-free
  Context: if (snapshot.contains(key)) target.store(snapshot[key].get<double>(), std::memory_order_relaxed);
- Line 795: severity=MEDIUM; category=llm_ai_safety; pattern=missing_resource_limits
  Description: LLM inference without token limit or timeout (DOS risk)
  Context: void PromptEngineeringMetrics::recordReflectionCycleComplete(
  Confidence: band=high; score=0.74

### src/prompt_engineering/rag_context_budget_manager.cpp
Total findings: 4

- Line 30: severity=HIGH; category=observability; pattern=missing_trace_point
  Description: Critical function allocate without trace point
  Context: BudgetHandle RagContextBudgetManager::allocate(size_t tokens) {
  Confidence: band=very_high; score=0.9
- Line 38: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw BudgetExhaustedError(tokens, avail, total_budget_);
- Line 72: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: allocated_.store(0, std::memory_order_relaxed);
- Line 77: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: const size_t alloc = allocated_.load(std::memory_order_relaxed);

### src/prompt_engineering/prompt_optimizer.cpp
Total findings: 3

- Line 113: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: result.metadata["improvement"] = current_score - result.score_history[0];
- Line 114: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: result.metadata["relative_improvement"] =
- Line 169: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: auto meta_result = meta_gen.generateImprovementPrompt(
  Confidence: band=very_high; score=0.9

### src/prompt_engineering/prompt_template_validator.cpp
Total findings: 3

- Line 93: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.errors.push_back(
  Confidence: band=high; score=0.74
- Line 94: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.errors.push_back(
- Line 100: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.errors.push_back(

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

### src/prompt_engineering/markdown_utils.cpp
Total findings: 2

- Line 23: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: // Handle empty or too-short input
  Confidence: band=very_high; score=0.99
- Line 59: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: static const std::regex line_comment(R"(//[^\r\n]*)",

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

## Update Workflow

- Refresh scan artifacts with: python tools/gap_scanner_v3.py
- Regenerate all module notes with: python tools/module_doc_generator.py . ai_working ai_working/module_gaps
- The generator mirrors each archive document directly into src/<module>/MODULE_GAPS.md.

Format: THEMIS_MODULE_GAPS_V3
