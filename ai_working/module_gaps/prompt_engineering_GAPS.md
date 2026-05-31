# prompt_engineering Module - Developer Gap Note

> Auto-generated from ai_working/gap_scan_v3_aggregate.json.
> This file is overwritten on each regeneration.

## Scan Snapshot

- Module: prompt_engineering
- Generated: 2026-05-31 08:50:11
- Status: Critical Findings Present
- Total Findings: 608
- Actionable Findings (Critical + High): 237
- Affected Files: 31

## Severity Summary

| Severity | Count |
|---|---:|
| Critical | 49 |
| High | 188 |
| Medium | 371 |
| Low | 0 |

## Category Summary

| Category | Count |
|---|---:|
| container | 196 |
| performance_patterns | 152 |
| llm_ai_safety | 65 |
| audit_logging | 33 |
| reliability | 31 |
| exception_safety | 24 |
| memory | 24 |
| determinism | 23 |
| concurrency | 21 |
| performance | 13 |
| security | 9 |
| platform | 6 |
| observability | 5 |
| raii | 4 |
| input_validation | 1 |
| legacy_duplication | 1 |

## File Overview

| File | Findings | Critical | High | Medium | Low |
|---|---:|---:|---:|---:|---:|
| src/prompt_engineering/prompt_version_control.cpp | 75 | 10 | 17 | 48 | 0 |
| src/prompt_engineering/feedback_collector.cpp | 53 | 4 | 15 | 34 | 0 |
| src/prompt_engineering/self_improvement_orchestrator.cpp | 47 | 9 | 28 | 10 | 0 |
| src/prompt_engineering/dspy_module.cpp | 44 | 8 | 28 | 8 | 0 |
| src/prompt_engineering/prompt_template_compiler.cpp | 34 | 2 | 13 | 19 | 0 |
| src/prompt_engineering/tree_of_thoughts.cpp | 32 | 6 | 0 | 26 | 0 |
| src/prompt_engineering/meta_prompt_generator.cpp | 30 | 2 | 4 | 24 | 0 |
| src/prompt_engineering/prompt_manager.cpp | 29 | 0 | 6 | 23 | 0 |
| src/prompt_engineering/prompt_evaluator.cpp | 26 | 3 | 10 | 11 | 2 |
| src/prompt_engineering/reflection_tuner.cpp | 24 | 0 | 11 | 13 | 0 |
| src/prompt_engineering/prompt_compressor.cpp | 22 | 0 | 2 | 20 | 0 |
| src/prompt_engineering/prompt_injection_detector.cpp | 19 | 0 | 3 | 16 | 0 |
| src/prompt_engineering/prompt_ab_experiment.cpp | 18 | 1 | 8 | 7 | 2 |
| src/prompt_engineering/prompt_library_io.cpp | 18 | 0 | 0 | 18 | 0 |
| src/prompt_engineering/structured_output.cpp | 18 | 0 | 3 | 15 | 0 |
| src/prompt_engineering/protegi_optimizer.cpp | 16 | 1 | 1 | 14 | 0 |
| src/prompt_engineering/prompt_performance_tracker.cpp | 15 | 2 | 6 | 7 | 0 |
| src/prompt_engineering/prompt_regression_runner.cpp | 14 | 0 | 7 | 7 | 0 |
| src/prompt_engineering/cot_tracer.cpp | 13 | 0 | 7 | 6 | 0 |
| src/prompt_engineering/context_window_manager.cpp | 12 | 0 | 3 | 9 | 0 |
| src/prompt_engineering/prompt_quality_evaluator.cpp | 9 | 0 | 0 | 9 | 0 |
| src/prompt_engineering/prompt_engineering_integration.cpp | 7 | 0 | 2 | 5 | 0 |
| src/prompt_engineering/llm_reflection_adapter.cpp | 5 | 0 | 1 | 4 | 0 |
| src/prompt_engineering/rag_context_budget_manager.cpp | 5 | 0 | 5 | 0 | 0 |
| src/prompt_engineering/prompt_engineering_metrics.cpp | 4 | 0 | 3 | 1 | 0 |
| src/prompt_engineering/prompt_optimizer.cpp | 4 | 0 | 4 | 0 | 0 |
| src/prompt_engineering/rag_prompt_builder.cpp | 4 | 0 | 0 | 4 | 0 |
| src/prompt_engineering/system_prompt_manager.cpp | 4 | 0 | 1 | 3 | 0 |
| src/prompt_engineering/prompt_template_validator.cpp | 3 | 0 | 0 | 3 | 0 |
| src/prompt_engineering/adversarial_prompt_tester.cpp | 2 | 0 | 0 | 2 | 0 |
| src/prompt_engineering/markdown_utils.cpp | 2 | 1 | 0 | 1 | 0 |

## Full Scanner Findings

### src/prompt_engineering/prompt_version_control.cpp
Total findings: 75

- Line 418: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator v_it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto v_it = versions_.find(head_version);
- Line 464: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator prompt_it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto prompt_it = branches_.find(prompt_id);
- Line 478: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator source_it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto source_it = versions_.find(source_id);
- Line 479: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator target_it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto target_it = versions_.find(target_id);
- Line 636: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto tag_it = prompt_it->second.find(tag_name);
- Line 637: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: if (tag_it == prompt_it->second.end()) {
- Line 689: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator branch_it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto branch_it = branches_.find(prompt_id);
- Line 703: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: stats["tag_count"] = tag_it->second.size();
- Line 721: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: std::string input = prompt_id + content + parent +
  Confidence: band=very_high; score=0.99
- Line 725: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
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
- Line 44: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: j["metadata"] = metadata;
- Line 599: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto& [version_id, version] : versions_) {
  Confidence: band=very_high; score=0.9
- Line 721: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: std::string input = prompt_id + content + parent +
  Confidence: band=very_high; score=0.9
- Line 725: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: SHA256(reinterpret_cast<const unsigned char*>(input.c_str()), input.length(), hash);
  Confidence: band=very_high; score=0.9
- Line 768: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: db_->scanPrefix(KEY_PREFIX_VERSION, [this, &loaded_versions](std::string_view /*key*/, std::string_v
- Line 882: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: for (size_t k = hunk_end; k < std::min(hunk_end + size_t(CONTEXT) * 2, N); ++k) {
- Line 66: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto time_val = j["timestamp"].get<std::time_t>();
  Confidence: band=high; score=0.74
- Line 217: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: history.push_back(version);
  Confidence: band=high; score=0.74
- Line 218: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: history.push_back(version);
- Line 222: severity=MEDIUM; category=determinism; pattern=timestamp_sorting_unstable
  Description: Timestamp-based sorting without stable_sort (non-deterministic ties)
  Context: // Sort by timestamp (descending)
  Confidence: band=high; score=0.74
- Line 422: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(info);
  Confidence: band=high; score=0.74
- Line 423: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.push_back(info);
- Line 503: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: ancestors.push_back(cur);
- Line 515: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> tgt_set(tgt_ancestors.begin(), tgt_ancestors.end());
  Confidence: band=medium; score=0.66
- Line 592: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, std::string> PromptVersionControl::getGenealogy(
  Confidence: band=medium; score=0.66
- Line 597: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, std::string> genealogy;
  Confidence: band=medium; score=0.66
- Line 693: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: stats["branches"].push_back(name);
  Confidence: band=high; score=0.74
- Line 693: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: stats["branches"].push_back(name);
  Confidence: band=high; score=0.74
- Line 694: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: stats["branches"].push_back(name);
- Line 803: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: lines.push_back(line);
- Line 836: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: edits.push_back({Op::KEEP, lines_a[i - 1]});
  Confidence: band=high; score=0.74
- Line 836: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: edits.push_back({Op::KEEP, lines_a[i - 1]});
  Confidence: band=high; score=0.74
- Line 837: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: edits.push_back({Op::KEEP, lines_a[i - 1]});
- Line 841: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: edits.push_back({Op::ADD, lines_b[j - 1]});
- Line 844: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: edits.push_back({Op::REMOVE, lines_a[i - 1]});
- Line 853: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: diff.removed_lines.push_back(e.line);
  Confidence: band=high; score=0.74
- Line 854: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: diff.removed_lines.push_back(e.line);
- Line 857: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: diff.added_lines.push_back(e.line);
- Line 953: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: while (std::getline(iss, line)) lines.push_back(line);
- Line 978: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: edits.push_back({' ', from[i-1]});
  Confidence: band=high; score=0.74
- Line 978: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: edits.push_back({' ', from[i-1]});
  Confidence: band=high; score=0.74
- Line 979: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: edits.push_back({' ', from[i-1]});
- Line 983: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: edits.push_back({'+', to[j-1]});
- Line 986: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: edits.push_back({'-', from[i-1]});
- Line 1024: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: changes[base_idx].insertions_before.push_back(e.second);
  Confidence: band=high; score=0.74
- Line 1025: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: changes[base_idx].insertions_before.push_back(e.second);
- Line 1028: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: eof_slot.insertions_before.push_back(e.second);
- Line 1047: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> sc_ins_set(
  Confidence: band=medium; score=0.66
- Line 1048: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: for (const auto& ins : sc.insertions_before) merged.push_back(ins);
  Confidence: band=high; score=0.74
- Line 1049: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: for (const auto& ins : sc.insertions_before) merged.push_back(ins);
- Line 1051: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: merged.push_back(ins);
  Confidence: band=high; score=0.74
- Line 1051: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: merged.push_back(ins);
  Confidence: band=high; score=0.74
- Line 1052: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: merged.push_back(ins);
- Line 1064: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: merged.push_back(base_lines[i]);
- Line 1070: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> src_eof_set(
  Confidence: band=medium; score=0.66
- Line 1072: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: for (const auto& ins : src_eof.insertions_before) merged.push_back(ins);
- Line 1074: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: merged.push_back(ins);
  Confidence: band=high; score=0.74
- Line 1074: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: merged.push_back(ins);
  Confidence: band=high; score=0.74
- Line 1075: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: merged.push_back(ins);
- Line 1084: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: for (const auto& l : lines) { out += l; out += '\n'; }
  Confidence: band=high; score=0.74
- Line 1084: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: for (const auto& l : lines) { out += l; out += '\n'; }
  Confidence: band=high; score=0.74
- Line 1102: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.conflicts.push_back(
  Confidence: band=high; score=0.74
- Line 1102: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.conflicts.push_back(
  Confidence: band=high; score=0.74
- Line 1103: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.conflicts.push_back(

### src/prompt_engineering/feedback_collector.cpp
Total findings: 53

- Line 227: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: summary.reason_embedding    = embedding_model_->embed(query);
- Line 370: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto it = feedback_.find(prompt_id);
- Line 419: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto it = feedback_.find(prompt_id);
- Line 486: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto it = feedback_.find(prompt_id);
- Line 0: severity=HIGH; category=uncategorized
  Context: ['    // becomes the representative for the pattern.', '    auto best_keyword = [&](size_t entry_idx) -> std::string {', '        const auto& tokens = per_entry_tokens[entry_idx];', '        if (tokens.empty()) return "[empty]";', '']
  Confidence: band=high; score=0.78
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 87: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: j["metadata"] = metadata;
- Line 270: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto& entry : it->second) {
  Confidence: band=very_high; score=0.9
- Line 377: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto& entry : it->second) {
  Confidence: band=very_high; score=0.9
- Line 425: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto& entry : it->second) {
  Confidence: band=very_high; score=0.9
- Line 456: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: db_->scanPrefix(prefix, [&](std::string_view, std::string_view value) -> bool {
- Line 472: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto& e : to_delete) {
  Confidence: band=very_high; score=0.9
- Line 499: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: db_->scanPrefix(prompt_prefix, [&](std::string_view, std::string_view value) -> bool {
- Line 506: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto& e : to_delete) {
  Confidence: band=very_high; score=0.9
- Line 586: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto& e : entries) {
  Confidence: band=very_high; score=0.9
- Line 714: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: db_->scanPrefix(prefix, [this, &loaded](std::string_view /*key*/, std::string_view value) -> bool {
- Line 781: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: for (size_t i = 0; i < std::min(size_t(3), sorted_types.size()); ++i) {
- Line 812: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: if (STOP_WORDS.find(cur) == STOP_WORDS.end() && cur.size() >= 2) {
- Line 896: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto& [key, pat] : patterns) {
  Confidence: band=very_high; score=0.9
- Line 114: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto time_val = j["timestamp"].get<std::time_t>();
  Confidence: band=high; score=0.74
- Line 227: severity=MEDIUM; category=llm_ai_safety; pattern=missing_resource_limits
  Description: LLM inference without token limit or timeout (DOS risk)
  Context: summary.reason_embedding    = embedding_model_->embed(query);
  Confidence: band=high; score=0.74
- Line 275: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(entry);
  Confidence: band=high; score=0.74
- Line 276: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.push_back(entry);
- Line 314: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(prompt_id);
  Confidence: band=high; score=0.74
- Line 315: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.push_back(prompt_id);
- Line 352: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.emplace_back(entry.query, entry.response, entry.type);
  Confidence: band=high; score=0.74
- Line 378: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: failures.push_back(entry);
  Confidence: band=high; score=0.74
- Line 379: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: failures.push_back(entry);
- Line 410: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.push_back(FeedbackEntry::fromJson(j));
- Line 411: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {}
- Line 426: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(entry);
  Confidence: band=high; score=0.74
- Line 427: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.push_back(entry);
- Line 465: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: to_delete.push_back(FeedbackEntry::fromJson(j));
- Line 503: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {}
- Line 540: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(entry);
  Confidence: band=high; score=0.74
- Line 541: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.push_back(entry);
- Line 588: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: outliers.push_back(e);
  Confidence: band=high; score=0.74
- Line 588: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: outliers.push_back(e);
  Confidence: band=high; score=0.74
- Line 589: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: outliers.push_back(e);
- Line 718: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: feedback_[entry.prompt_id].push_back(entry);
- Line 773: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: sorted_types.emplace_back(type, count);
  Confidence: band=high; score=0.74
- Line 781: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: stats.common_issues.push_back(feedbackTypeToString(sorted_types[i].first));
  Confidence: band=high; score=0.74
- Line 782: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: stats.common_issues.push_back(feedbackTypeToString(sorted_types[i].first));
- Line 813: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: tokens.push_back(cur);
  Confidence: band=high; score=0.74
- Line 814: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: tokens.push_back(cur);
- Line 820: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: tokens.push_back(cur);
- Line 830: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, size_t> document_frequency;
  Confidence: band=medium; score=0.66
- Line 838: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> seen(tokens.begin(), tokens.end());
  Confidence: band=medium; score=0.66
- Line 852: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, size_t> tf;
  Confidence: band=medium; score=0.66
- Line 879: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: p.examples.push_back(entry.query);
  Confidence: band=high; score=0.74
- Line 880: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: p.examples.push_back(entry.query);
- Line 897: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(pat);
  Confidence: band=high; score=0.74
- Line 898: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.push_back(pat);

### src/prompt_engineering/self_improvement_orchestrator.cpp
Total findings: 47

- Line 207: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: // between (prompt + input) and expected_output, which serves as a proxy for
  Confidence: band=very_high; score=0.99
- Line 210: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: // The "\n" separator between prompt and input is a neutral token boundary
  Confidence: band=very_high; score=0.99
- Line 215: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: // Guard: evaluateBatch() returns 0.0 for empty inputs, which would
  Confidence: band=very_high; score=0.99
- Line 218: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: std::vector<std::string> prompt_with_inputs;
  Confidence: band=very_high; score=0.99
- Line 220: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: prompt_with_inputs.reserve(cases.size());
  Confidence: band=very_high; score=0.99
- Line 223: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: // Combine prompt template with the test input as a single
  Confidence: band=very_high; score=0.99
- Line 225: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: prompt_with_inputs.push_back(prompt + "\n" + tc.input);
  Confidence: band=very_high; score=0.99
- Line 228: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: return evaluator_->evaluateBatch(prompt_with_inputs, expected).overall_score;
  Confidence: band=very_high; score=0.99
- Line 643: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
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
- Line 54: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: j["metadata"] = metadata;
- Line 147: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (auto& [prompt_id, test_cases] : candidates) {
  Confidence: band=very_high; score=0.9
- Line 207: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: // between (prompt + input) and expected_output, which serves as a proxy for
  Confidence: band=very_high; score=0.9
- Line 210: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: // The "\n" separator between prompt and input is a neutral token boundary
  Confidence: band=very_high; score=0.9
- Line 215: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: // Guard: evaluateBatch() returns 0.0 for empty inputs, which would
  Confidence: band=very_high; score=0.9
- Line 215: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: // Guard: evaluateBatch() returns 0.0 for empty inputs, which would
  Confidence: band=very_high; score=0.9
- Line 218: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: std::vector<std::string> prompt_with_inputs;
  Confidence: band=very_high; score=0.9
- Line 218: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: std::vector<std::string> prompt_with_inputs;
  Confidence: band=very_high; score=0.9
- Line 220: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: prompt_with_inputs.reserve(cases.size());
  Confidence: band=very_high; score=0.9
- Line 220: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: prompt_with_inputs.reserve(cases.size());
  Confidence: band=very_high; score=0.9
- Line 223: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: // Combine prompt template with the test input as a single
  Confidence: band=very_high; score=0.9
- Line 225: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: prompt_with_inputs.push_back(prompt + "\n" + tc.input);
  Confidence: band=very_high; score=0.9
- Line 225: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: prompt_with_inputs.push_back(prompt + "\n" + tc.input);
  Confidence: band=very_high; score=0.9
- Line 228: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: return evaluator_->evaluateBatch(prompt_with_inputs, expected).overall_score;
  Confidence: band=very_high; score=0.9
- Line 228: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: return evaluator_->evaluateBatch(prompt_with_inputs, expected).overall_score;
  Confidence: band=very_high; score=0.9
- Line 270: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: result.metadata["ab_test_id"] = test_id;
- Line 290: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: result.metadata["error"] = e.what();
- Line 409: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (auto& results : optimization_history_[test.prompt_id]) {
  Confidence: band=very_high; score=0.9
- Line 443: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (auto it = history.rbegin(); it != history.rend(); ++it) {
  Confidence: band=very_high; score=0.9
- Line 477: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto& [id, test] : active_ab_tests_) {
  Confidence: band=very_high; score=0.9
- Line 610: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: updated_template.metadata["last_deployed"] =
- Line 643: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: tc.input           = entry.query;
  Confidence: band=very_high; score=0.9
- Line 139: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: candidates.emplace_back(metrics.prompt_id, std::move(test_cases));
  Confidence: band=high; score=0.74
- Line 147: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results.push_back(optimizePrompt(prompt_id, test_cases));
  Confidence: band=high; score=0.74
- Line 148: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: results.push_back(optimizePrompt(prompt_id, test_cases));
- Line 224: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: prompt_with_inputs.push_back(prompt + "\n" + tc.input);
  Confidence: band=high; score=0.74
- Line 225: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: prompt_with_inputs.push_back(prompt + "\n" + tc.input);
- Line 226: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: expected.push_back(tc.expected_output);
- Line 477: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: tests.push_back(test);
  Confidence: band=high; score=0.74
- Line 478: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: tests.push_back(test);
- Line 644: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: test_cases.push_back(std::move(tc));
  Confidence: band=high; score=0.74
- Line 645: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: test_cases.push_back(std::move(tc));

### src/prompt_engineering/dspy_module.cpp
Total findings: 44

- Line 52: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: DspySignature& DspySignature::addInput(DspyField field)
  Confidence: band=very_high; score=0.99
- Line 54: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: inputs_.push_back(std::move(field));
  Confidence: band=very_high; score=0.99
- Line 66: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: const std::vector<DspyField>& DspySignature::inputs()  const { return inputs_;  }
  Confidence: band=very_high; score=0.99
- Line 79: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: // Render input fields
  Confidence: band=very_high; score=0.99
- Line 80: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: for (const auto& field : inputs_) {
  Confidence: band=very_high; score=0.99
- Line 209: severity=CRITICAL; category=audit_logging; pattern=sensitive_data_logging
  Description: Potential PII/credential logging: token
  Context: response << token << ": [echo]\n";
  Confidence: band=very_high; score=0.92
- Line 279: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: for (const auto& f : signature.inputs()) {
  Confidence: band=very_high; score=0.99
- Line 280: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: result.addInput(f);
  Confidence: band=very_high; score=0.99
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 52: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: DspySignature& DspySignature::addInput(DspyField field)
  Confidence: band=very_high; score=0.9
- Line 54: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: inputs_.push_back(std::move(field));
  Confidence: band=very_high; score=0.9
- Line 54: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: inputs_.push_back(std::move(field));
  Confidence: band=very_high; score=0.9
- Line 60: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: outputs_.push_back(std::move(field));
  Confidence: band=very_high; score=0.9
- Line 66: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: const std::vector<DspyField>& DspySignature::inputs()  const { return inputs_;  }
  Confidence: band=very_high; score=0.9
- Line 66: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: const std::vector<DspyField>& DspySignature::inputs()  const { return inputs_;  }
  Confidence: band=very_high; score=0.9
- Line 67: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: const std::vector<DspyField>& DspySignature::outputs() const { return outputs_; }
  Confidence: band=very_high; score=0.9
- Line 79: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: // Render input fields
  Confidence: band=very_high; score=0.9
- Line 80: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: for (const auto& field : inputs_) {
  Confidence: band=very_high; score=0.9
- Line 80: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: for (const auto& field : inputs_) {
  Confidence: band=very_high; score=0.9
- Line 84: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw DspyMissingFieldError(field.name);
- Line 96: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: for (const auto& field : outputs_) {
  Confidence: band=very_high; score=0.9
- Line 112: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: for (const auto& field : outputs_) {
  Confidence: band=very_high; score=0.9
- Line 144: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: for (const auto& other : outputs_) {
  Confidence: band=very_high; score=0.9
- Line 147: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: for (const auto& suffix : {std::string(":"), std::string(" :")}) {
- Line 149: severity=HIGH; category=performance; pattern=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Context: auto other_pos = response.find(other_marker, value_start);
  Confidence: band=very_high; score=0.9
- Line 243: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error(
- Line 271: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: // Rebuild outputs with Reasoning prepended
  Confidence: band=very_high; score=0.9
- Line 272: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: std::vector<DspyField> new_outputs;
  Confidence: band=very_high; score=0.9
- Line 273: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: new_outputs.push_back(std::move(reasoning_field));
  Confidence: band=very_high; score=0.9
- Line 274: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: for (const auto& f : signature.outputs()) {
  Confidence: band=very_high; score=0.9
- Line 275: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: new_outputs.push_back(f);
  Confidence: band=very_high; score=0.9
- Line 279: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: for (const auto& f : signature.inputs()) {
  Confidence: band=very_high; score=0.9
- Line 279: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: for (const auto& f : signature.inputs()) {
  Confidence: band=very_high; score=0.9
- Line 280: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: result.addInput(f);
  Confidence: band=very_high; score=0.9
- Line 282: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: for (auto& f : new_outputs) {
  Confidence: band=very_high; score=0.9
- Line 70: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: const std::unordered_map<std::string, std::string>& context) const
  Confidence: band=medium; score=0.66
- Line 107: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, std::string> DspySignature::parseResponse(
  Confidence: band=medium; score=0.66
- Line 110: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, std::string> parsed;
  Confidence: band=medium; score=0.66
- Line 173: severity=MEDIUM; category=llm_ai_safety; pattern=missing_resource_limits
  Description: LLM inference without token limit or timeout (DOS risk)
  Context: std::string EchoDspyLLMProvider::complete(const std::string& prompt)
  Confidence: band=high; score=0.74
- Line 248: severity=MEDIUM; category=llm_ai_safety; pattern=missing_resource_limits
  Description: LLM inference without token limit or timeout (DOS risk)
  Context: std::string response = llm_provider_->complete(prompt);
  Confidence: band=high; score=0.74
- Line 273: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: new_outputs.push_back(std::move(reasoning_field));
- Line 274: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: new_outputs.push_back(f);
  Confidence: band=high; score=0.74
- Line 275: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: new_outputs.push_back(f);

### src/prompt_engineering/prompt_template_compiler.cpp
Total findings: 34

- Line 174: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator in_pos may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto in_pos = rest.find(" in ");
- Line 249: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto it = slot_index.find(tok.value);
- Line 157: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw PromptTemplateCompileError(
- Line 176: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw PromptTemplateCompileError(
- Line 185: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw PromptTemplateCompileError(
- Line 277: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw PromptTemplateCompileError(
- Line 287: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw PromptTemplateCompileError(
- Line 294: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw PromptTemplateCompileError(
- Line 308: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw PromptTemplateCompileError(
- Line 319: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw PromptTemplateCompileError(
- Line 356: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw PromptTemplateMissingSlotError(name);
- Line 432: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: if (node->required && ctx.find(name) == ctx.end()) {
- Line 521: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw PromptTemplateCompileError(
- Line 531: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: if (slot_index.find(n->text) == slot_index.end()) {
- Line 549: severity=HIGH; category=repeated_search
  Description: find/search in loop — O(n²) or worse
  Remediation: Move search outside loop or build index/map before loop
  Context: if (std::find_if(final_slots.begin(), final_slots.end(),
- Line 144: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: tokens.push_back({TokenKind::SLOT, inner, {}});
- Line 166: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: tokens.push_back({TokenKind::IF, var, {}});
- Line 168: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: tokens.push_back({TokenKind::ELSE, {}, {}});
- Line 170: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: tokens.push_back({TokenKind::ENDIF, {}, {}});
- Line 181: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: tokens.push_back({TokenKind::FOR, item_var, list_var});
- Line 183: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: tokens.push_back({TokenKind::ENDFOR, {}, {}});
- Line 201: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: tokens.push_back({TokenKind::SLOT, inner, {}});
- Line 239: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: nodes.push_back(std::move(node));
- Line 281: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: nodes.push_back(std::move(node));
- Line 313: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: nodes.push_back(std::move(node));
- Line 433: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: errors.push_back("Missing required slot: " + name);
  Confidence: band=high; score=0.74
- Line 434: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: errors.push_back("Missing required slot: " + name);
- Line 453: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 455: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: errors.push_back("Internal validation error for node");
- Line 491: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: slot_arr.push_back(s.toJson());
  Confidence: band=high; score=0.74
- Line 492: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: slot_arr.push_back(s.toJson());
- Line 507: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, SlotDefinition> slot_index;
  Confidence: band=medium; score=0.66
- Line 551: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: final_slots.push_back(sd);
  Confidence: band=high; score=0.74
- Line 552: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: final_slots.push_back(sd);

### src/prompt_engineering/tree_of_thoughts.cpp
Total findings: 32

- Line 161: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto root_thoughts = generator_->generate(problem, {}, config_.branching_factor);
- Line 207: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto children = generator_->generate(problem, child_path, config_.branching_factor);
- Line 235: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto root_thoughts = generator_->generate(problem, {}, config_.branching_factor);
- Line 279: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto children = generator_->generate(problem, child_path, config_.branching_factor);
- Line 307: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto root_thoughts = generator_->generate(problem, {}, config_.branching_factor);
- Line 345: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto children = generator_->generate(problem, child_path, config_.branching_factor);
- Line 32: severity=MEDIUM; category=llm_ai_safety; pattern=missing_resource_limits
  Description: LLM inference without token limit or timeout (DOS risk)
  Context: std::vector<std::string> HeuristicThoughtGenerator::generate(
  Confidence: band=high; score=0.74
- Line 48: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: thoughts.push_back(oss.str());
  Confidence: band=high; score=0.74
- Line 49: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: thoughts.push_back(oss.str());
- Line 161: severity=MEDIUM; category=llm_ai_safety; pattern=missing_resource_limits
  Description: LLM inference without token limit or timeout (DOS risk)
  Context: auto root_thoughts = generator_->generate(problem, {}, config_.branching_factor);
  Confidence: band=high; score=0.74
- Line 184: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.log.push_back(msg.str());
- Line 207: severity=MEDIUM; category=llm_ai_safety; pattern=missing_resource_limits
  Description: LLM inference without token limit or timeout (DOS risk)
  Context: auto children = generator_->generate(problem, child_path, config_.branching_factor);
  Confidence: band=high; score=0.74
- Line 216: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.best_path.push_back(best_node.thought);
  Confidence: band=high; score=0.74
- Line 217: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.best_path.push_back(best_node.thought);
- Line 235: severity=MEDIUM; category=llm_ai_safety; pattern=missing_resource_limits
  Description: LLM inference without token limit or timeout (DOS risk)
  Context: auto root_thoughts = generator_->generate(problem, {}, config_.branching_factor);
  Confidence: band=high; score=0.74
- Line 255: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.log.push_back(msg.str());
  Confidence: band=high; score=0.74
- Line 256: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.log.push_back(msg.str());
- Line 279: severity=MEDIUM; category=llm_ai_safety; pattern=missing_resource_limits
  Description: LLM inference without token limit or timeout (DOS risk)
  Context: auto children = generator_->generate(problem, child_path, config_.branching_factor);
  Confidence: band=high; score=0.74
- Line 287: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.best_path.push_back(best_node.thought);
  Confidence: band=high; score=0.74
- Line 288: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.best_path.push_back(best_node.thought);
- Line 307: severity=MEDIUM; category=llm_ai_safety; pattern=missing_resource_limits
  Description: LLM inference without token limit or timeout (DOS risk)
  Context: auto root_thoughts = generator_->generate(problem, {}, config_.branching_factor);
  Confidence: band=high; score=0.74
- Line 314: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: beam.push_back(std::move(n));
  Confidence: band=high; score=0.74
- Line 315: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: beam.push_back(std::move(n));
- Line 335: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.best_path.push_back(node.thought);
  Confidence: band=high; score=0.74
- Line 335: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.best_path.push_back(node.thought);
  Confidence: band=high; score=0.74
- Line 336: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.best_path.push_back(node.thought);
- Line 343: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: child_path.push_back(node.thought);
- Line 345: severity=MEDIUM; category=llm_ai_safety; pattern=missing_resource_limits
  Description: LLM inference without token limit or timeout (DOS risk)
  Context: auto children = generator_->generate(problem, child_path, config_.branching_factor);
  Confidence: band=high; score=0.74
- Line 355: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.log.push_back(msg.str());
  Confidence: band=high; score=0.74
- Line 356: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.log.push_back(msg.str());
- Line 360: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: next_beam.push_back(std::move(child));
- Line 459: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: out << "Then assign a verdict: sure / maybe / impossible.\n";

### src/prompt_engineering/meta_prompt_generator.cpp
Total findings: 30

- Line 158: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: analysis << "**Input**: " << examples[i].first << "\n";
  Confidence: band=very_high; score=0.99
- Line 164: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: analysis << "2. Assess whether examples cover the input space adequately\n";
  Confidence: band=very_high; score=0.99
- Line 135: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: result.metadata["score"] = score;
- Line 136: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: result.metadata["strategy"] = config_.improvement_strategy;
- Line 158: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: analysis << "**Input**: " << examples[i].first << "\n";
  Confidence: band=very_high; score=0.9
- Line 164: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: analysis << "2. Assess whether examples cover the input space adequately\n";
  Confidence: band=very_high; score=0.9
- Line 57: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: meta_prompt << "Current Score: " << score << " / 1.0\n";
- Line 88: severity=MEDIUM; category=llm_ai_safety; pattern=missing_resource_limits
  Description: LLM inference without token limit or timeout (DOS risk)
  Context: std::string llm_response = llm_provider_->complete(result.meta_prompt);
  Confidence: band=high; score=0.74
- Line 125: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.key_insights.push_back("Prompt lacks clarity or specificity");
- Line 126: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.key_insights.push_back("Consider restructuring with clear sections");
- Line 128: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.key_insights.push_back("Prompt is functional but can be improved");
- Line 129: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.key_insights.push_back("Focus on edge cases and formatting");
- Line 131: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.key_insights.push_back("Prompt performs well");
- Line 132: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.key_insights.push_back("Minor optimizations possible");
- Line 197: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: suggestions.push_back("Specify exact output format");
- Line 198: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: suggestions.push_back("Provide output template or schema");
- Line 199: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: suggestions.push_back("Include formatting requirements");
- Line 204: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: suggestions.push_back("List explicit constraints");
- Line 205: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: suggestions.push_back("Define boundary conditions");
- Line 206: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: suggestions.push_back("Specify error handling requirements");
- Line 211: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: suggestions.push_back("Review and simplify language");
- Line 212: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: suggestions.push_back("Add structure with headers");
- Line 213: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: suggestions.push_back("Include validation criteria");
- Line 262: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: patterns.push_back("High-performing prompts include concrete examples");
- Line 266: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: patterns.push_back("Successful prompts use step-by-step instructions");
- Line 270: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: patterns.push_back("Clear output format specifications improve performance");
- Line 274: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: patterns.push_back("Explicit constraints help guide the model");
- Line 278: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: patterns.push_back("Structure prompts with clear sections (task, examples, output)");
- Line 279: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: patterns.push_back("Use precise, unambiguous language");
- Line 280: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: patterns.push_back("Include both positive and negative examples when relevant");

### src/prompt_engineering/prompt_manager.cpp
Total findings: 29

- Line 155: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: db_->scanPrefix(prefix, [&]([[maybe_unused]] std::string_view key, std::string_view value) -> bool {
- Line 390: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: auto metadata = schema_mgr->getDatabaseMetadata();
- Line 391: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: context["table_count"] = std::to_string(metadata.table_count);
- Line 392: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: context["total_rows"] = std::to_string(metadata.total_rows);
- Line 399: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: context["capabilities"] = caps_array.dump();
- Line 411: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: context["tables"] = tables_array.dump(2);
- Line 59: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.errors.push_back(
  Confidence: band=high; score=0.74
- Line 60: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.errors.push_back(
- Line 67: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.errors.push_back("Template 'metadata' must be a JSON object");
- Line 94: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: acc.release(); // Release lock
- Line 130: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: t.images.push_back(ImageDescription::fromJson(img_j));
  Confidence: band=high; score=0.74
- Line 131: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: t.images.push_back(ImageDescription::fromJson(img_j));
- Line 169: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: t.images.push_back(ImageDescription::fromJson(img_j));
- Line 172: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: out.push_back(t);
- Line 184: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: out.push_back(kv.second);
  Confidence: band=high; score=0.74
- Line 185: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: out.push_back(kv.second);
- Line 287: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 299: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: pt.images.push_back(std::move(img));
  Confidence: band=high; score=0.74
- Line 300: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: pt.images.push_back(std::move(img));
- Line 334: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: const std::unordered_map<std::string, std::string>& context) const {
  Confidence: band=medium; score=0.66
- Line 396: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: caps_array.push_back(cap);
  Confidence: band=high; score=0.74
- Line 397: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: caps_array.push_back(cap);
- Line 408: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: tables_array.push_back(table_info);
  Confidence: band=high; score=0.74
- Line 409: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: tables_array.push_back(table_info);
- Line 433: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: const std::unordered_map<std::string, std::string>& context) {
  Confidence: band=medium; score=0.66
- Line 447: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: result += "\n\n[Images]\n";
  Confidence: band=high; score=0.74
- Line 451: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: result += std::to_string(i + 1) + ". [" + mime + "] " + img.alt_text + "\n";
  Confidence: band=high; score=0.74
- Line 454: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: result += "   Description: " + img.description + "\n";
- Line 457: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: result += "   URL: " + img.url + "\n";

### src/prompt_engineering/prompt_evaluator.cpp
Total findings: 26

- Line 51: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: metrics.details["embedding_provider"] = embedding_provider_->name();
- Line 488: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto v1 = embedding_provider_->embed(s1);
- Line 489: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto v2 = embedding_provider_->embed(s2);
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 70: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: const std::vector<std::string>& outputs,
  Confidence: band=very_high; score=0.9
- Line 75: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: if (outputs.size() != expected.size()) {
  Confidence: band=very_high; score=0.9
- Line 80: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: if (outputs.empty()) {
  Confidence: band=very_high; score=0.9
- Line 88: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: for (size_t i = 0; i < outputs.size(); ++i) {
  Confidence: band=very_high; score=0.9
- Line 89: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: auto metrics = evaluateSingle(outputs[i], expected[i]);
  Confidence: band=very_high; score=0.9
- Line 307: severity=HIGH; category=determinism; pattern=fp_exact_comparison
  Description: Floating-point exact comparison (use tolerance/epsilon)
  Context: if (x == 0.0) return 0.0;
  Confidence: band=very_high; score=0.9
- Line 308: severity=HIGH; category=determinism; pattern=fp_exact_comparison
  Description: Floating-point exact comparison (use tolerance/epsilon)
  Context: if (x == 1.0) return 1.0;
  Confidence: band=very_high; score=0.9
- Line 91: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: similarities.push_back(metrics.semantic_similarity);
  Confidence: band=high; score=0.74
- Line 92: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: similarities.push_back(metrics.semantic_similarity);
- Line 93: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: weighted_scores.push_back(weighted);
- Line 103: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: per_case.push_back(case_metrics);
- Line 162: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> set1(tokens1.begin(), tokens1.end());
  Confidence: band=medium; score=0.66
- Line 163: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> set2(tokens2.begin(), tokens2.end());
  Confidence: band=medium; score=0.66
- Line 225: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> output_set(tokens_output.begin(), tokens_output.end());
  Confidence: band=medium; score=0.66
- Line 384: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: result += ' ';
- Line 406: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: tokens.push_back(token);
- Line 488: severity=MEDIUM; category=llm_ai_safety; pattern=missing_resource_limits
  Description: LLM inference without token limit or timeout (DOS risk)
  Context: auto v1 = embedding_provider_->embed(s1);
  Confidence: band=high; score=0.74
- Line 489: severity=MEDIUM; category=llm_ai_safety; pattern=missing_resource_limits
  Description: LLM inference without token limit or timeout (DOS risk)
  Context: auto v2 = embedding_provider_->embed(s2);
  Confidence: band=high; score=0.74
- Line 321: severity=LOW; category=observability; pattern=unstructured_log
  Description: Unstructured logging (use structured format)
  Context: return std::log(M_PI / std::sin(M_PI * z)) - std::lgamma(1.0 - z);
  Confidence: band=medium; score=0.6
- Line 326: severity=LOW; category=observability; pattern=unstructured_log
  Description: Unstructured logging (use structured format)
  Context: double front = std::exp(aa * std::log(xx) + bb * std::log(1.0 - xx) - log_beta) / aa;
  Confidence: band=medium; score=0.6

### src/prompt_engineering/reflection_tuner.cpp
Total findings: 24

- Line 36: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: // Linguistic markers used to infer self-reported confidence.
  Confidence: band=very_high; score=0.9
- Line 146: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: s["metadata"]                = step.metadata;
- Line 150: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: j["metadata"] = metadata;
- Line 350: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: if (lower_resp.find(pattern) != std::string::npos) {
- Line 350: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: if (lower_resp.find(pattern) != std::string::npos) {
- Line 493: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: step.metadata["critique_prompt"] =
- Line 528: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: step.metadata["critique_prompt"]  = critique_prompt;
- Line 529: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: step.metadata["revision_prompt"]  =
- Line 563: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: result.metadata["strategy"]       = static_cast<int>(config_.strategy);
- Line 564: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: result.metadata["max_iterations"] = config_.max_iterations;
- Line 565: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: result.metadata["provider"]       =
- Line 87: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: ctx.uncertainty_markers.push_back(marker);
  Confidence: band=high; score=0.74
- Line 88: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: ctx.uncertainty_markers.push_back(marker);
- Line 146: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: steps_arr.push_back(s);
  Confidence: band=high; score=0.74
- Line 147: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: steps_arr.push_back(s);
- Line 246: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: out << "Claim / Response:\n" << response << "\n\n";
- Line 246: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: out << "Claim / Response:\n" << response << "\n\n";
- Line 389: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: trajectory.push_back(step.quality_score);
  Confidence: band=high; score=0.74
- Line 390: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: trajectory.push_back(step.quality_score);
- Line 556: severity=MEDIUM; category=llm_ai_safety; pattern=missing_resource_limits
  Description: LLM inference without token limit or timeout (DOS risk)
  Context: provider_ ? provider_->generate(prompt) : prompt;
  Confidence: band=high; score=0.74
- Line 576: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.quality_trajectory.push_back(initial_score);
- Line 588: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.steps.push_back(step);
  Confidence: band=high; score=0.74
- Line 589: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.steps.push_back(step);
- Line 590: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.quality_trajectory.push_back(step.quality_score);

### src/prompt_engineering/prompt_compressor.cpp
Total findings: 22

- Line 222: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: for (size_t i = std::max(sys_end, tail_start);
- Line 277: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: for (size_t i = std::max(sys_end, tail_start);
- Line 42: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: paragraphs.push_back(current);
- Line 46: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: if (!current.empty()) current += '\n';
- Line 50: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: if (!current.empty()) paragraphs.push_back(current);
- Line 60: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: while (ss >> word) words.push_back(word);
- Line 68: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: if (i > 0) result += ' ';
  Confidence: band=high; score=0.74
- Line 69: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: if (i > 0) result += ' ';
- Line 220: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: kept_indices.push_back(i);
  Confidence: band=high; score=0.74
- Line 221: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: kept_indices.push_back(i);
- Line 223: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: kept_indices.push_back(i);
  Confidence: band=high; score=0.74
- Line 224: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: kept_indices.push_back(i);
- Line 228: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: if (!result.empty()) result += "\n\n";
  Confidence: band=high; score=0.74
- Line 228: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: if (!result.empty()) result += "\n\n";
  Confidence: band=high; score=0.74
- Line 228: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: if (!result.empty()) result += "\n\n";
  Confidence: band=high; score=0.74
- Line 229: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: if (!result.empty()) result += "\n\n";
- Line 260: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: if (!middle.empty()) middle += "\n\n";
  Confidence: band=high; score=0.74
- Line 261: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: if (!middle.empty()) middle += "\n\n";
- Line 267: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: if (!result.empty()) result += "\n\n";
  Confidence: band=high; score=0.74
- Line 268: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: if (!result.empty()) result += "\n\n";
- Line 278: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: if (!result.empty()) result += "\n\n";
  Confidence: band=high; score=0.74
- Line 279: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: if (!result.empty()) result += "\n\n";

### src/prompt_engineering/prompt_injection_detector.cpp
Total findings: 19

- Line 112: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: if (lower.find(kw) != std::string::npos) {
- Line 112: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: if (lower.find(kw) != std::string::npos) {
- Line 126: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: text.find("[/INST]") != std::string::npos) {
- Line 46: severity=MEDIUM; category=legacy_duplication; pattern=duplicate_qualified_signature
  Description: Potential duplicate implementation signature across files: PromptInjectionDetector::initializePatterns()
  Context: void PromptInjectionDetector::initializePatterns() {
  Confidence: band=medium; score=0.56
- Line 65: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: patterns_.emplace_back(e.pattern_str, std::regex::icase);
  Confidence: band=high; score=0.74
- Line 67: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: pattern_labels_.push_back(e.label);
- Line 80: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: patterns_.emplace_back(pat, std::regex::icase);
  Confidence: band=high; score=0.74
- Line 82: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: pattern_labels_.push_back("custom:" + pat);
- Line 98: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: matched_out.push_back(pattern_labels_[i]);
  Confidence: band=high; score=0.74
- Line 99: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: matched_out.push_back(pattern_labels_[i]);
- Line 114: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: matched_out.push_back("keyword:" + kw);
  Confidence: band=high; score=0.74
- Line 115: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: matched_out.push_back("keyword:" + kw);
- Line 129: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: matched_out.push_back("syntax:instruction_bracket_token");
- Line 140: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: matched_out.push_back("syntax:high_special_char_density");
  Confidence: band=high; score=0.74
- Line 141: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: matched_out.push_back("syntax:high_special_char_density");
- Line 219: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: extra_matched.push_back(label);
  Confidence: band=high; score=0.74
- Line 220: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: extra_matched.push_back(label);
- Line 262: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: pos += 10; // len("[REDACTED]")
  Confidence: band=high; score=0.74
- Line 262: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: pos += 10; // len("[REDACTED]")
  Confidence: band=high; score=0.74

### src/prompt_engineering/prompt_ab_experiment.cpp
Total findings: 18

- Line 319: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto it = experiments_.find(experiment_id);
- Line 281: severity=HIGH; category=memory_order
  Description: memory_order_relaxed used — potential visibility issue
  Remediation: Use memory_order_acquire/release unless truly lock-free
  Context: ss << "exp-" << counter.fetch_add(1, std::memory_order_relaxed);
- Line 328: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto& [id, exp] : experiments_) {
  Confidence: band=very_high; score=0.9
- Line 569: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (unsigned char c : user_id) {
  Confidence: band=very_high; score=0.9
- Line 575: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (unsigned char c : key) {
  Confidence: band=very_high; score=0.9
- Line 584: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto& e : experiments_) {
  Confidence: band=very_high; score=0.9
- Line 586: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument(
- Line 596: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (auto& e : experiments_) {
  Confidence: band=very_high; score=0.9
- Line 618: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto& v : exp.variants) {
  Confidence: band=very_high; score=0.9
- Line 328: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: out.push_back(exp);
  Confidence: band=high; score=0.74
- Line 329: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: out.push_back(exp);
- Line 434: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: : ExperimentStatus::WINNER_CONTROL;
- Line 447: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: try { cb(eid, winner, wid); } catch (...) {}
- Line 590: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: experiments_.push_back(std::move(descriptor));
  Confidence: band=high; score=0.74
- Line 590: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: experiments_.push_back(std::move(descriptor));
  Confidence: band=high; score=0.74
- Line 591: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: experiments_.push_back(std::move(descriptor));
- Line 195: severity=LOW; category=observability; pattern=unstructured_log
  Description: Unstructured logging (use structured format)
  Context: const double front  = std::exp(std::log(x) * a +
  Confidence: band=medium; score=0.6
- Line 196: severity=LOW; category=observability; pattern=unstructured_log
  Description: Unstructured logging (use structured format)
  Context: std::log(1.0 - x) * b - lnBeta) / a;
  Confidence: band=medium; score=0.6

### src/prompt_engineering/prompt_library_io.cpp
Total findings: 18

- Line 68: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 79: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: t.images.push_back(img);
  Confidence: band=high; score=0.74
- Line 80: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: t.images.push_back(img);
- Line 102: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: tpls.push_back(t.toJson());
  Confidence: band=high; score=0.74
- Line 103: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: tpls.push_back(t.toJson());
- Line 136: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: t.images.push_back(img);
  Confidence: band=high; score=0.74
- Line 136: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: t.images.push_back(img);
  Confidence: band=high; score=0.74
- Line 137: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: t.images.push_back(img);
- Line 140: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: b.templates.push_back(std::move(t));
- Line 155: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: parts.push_back(t.toJson().dump());
  Confidence: band=high; score=0.74
- Line 156: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: parts.push_back(t.toJson().dump());
- Line 234: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 266: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 282: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 305: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: b.templates.push_back(templateFromYaml(node));
  Confidence: band=high; score=0.74
- Line 306: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: b.templates.push_back(templateFromYaml(node));
- Line 310: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 351: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {

### src/prompt_engineering/structured_output.cpp
Total findings: 18

- Line 159: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: for (auto it = begin; it != std::sregex_iterator(); ++it) {
- Line 305: severity=HIGH; category=repeated_search
  Description: find/search in loop — O(n²) or worse
  Remediation: Move search outside loop or build index/map before loop
  Context: const bool found = std::find(actual_keys.begin(), actual_keys.end(), req)
- Line 317: severity=HIGH; category=repeated_search
  Description: find/search in loop — O(n²) or worse
  Remediation: Move search outside loop or build index/map before loop
  Context: const bool known = std::find(allowed.begin(), allowed.end(), k)
- Line 44: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: static const std::regex line_comment(R"(//[^\n]*)", std::regex::ECMAScript);
- Line 67: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: errors.push_back("JSON output is empty");
- Line 73: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: errors.push_back("JSON output does not start with '{' or '['");
- Line 159: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back((*it)[1].str());
  Confidence: band=high; score=0.74
- Line 160: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.push_back((*it)[1].str());
- Line 219: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: names.push_back(cur_key);
  Confidence: band=high; score=0.74
- Line 220: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: names.push_back(cur_key);
- Line 269: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: keys.push_back(cur_key);
  Confidence: band=high; score=0.74
- Line 270: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: keys.push_back(cur_key);
- Line 307: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: errors.push_back("Missing required field: \"" + req + "\"");
  Confidence: band=high; score=0.74
- Line 308: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: errors.push_back("Missing required field: \"" + req + "\"");
- Line 319: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: errors.push_back("Unknown field not in schema properties: \"" +
  Confidence: band=high; score=0.74
- Line 320: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: errors.push_back("Unknown field not in schema properties: \"" +
- Line 347: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: errors.push_back("Output does not match regex pattern: " +
- Line 352: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: errors.push_back(std::string("Invalid regex pattern: ") + ex.what());

### src/prompt_engineering/protegi_optimizer.cpp
Total findings: 16

- Line 98: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: << "- Read the input carefully.\n"
  Confidence: band=very_high; score=0.99
- Line 98: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: << "- Read the input carefully.\n"
  Confidence: band=very_high; score=0.9
- Line 101: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: candidates.push_back(c.str());
- Line 109: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: candidates.push_back(c.str());
- Line 116: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: candidates.push_back(c.str());
  Confidence: band=high; score=0.74
- Line 117: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: candidates.push_back(c.str());
- Line 211: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: next_candidates.push_back(std::move(c));
  Confidence: band=high; score=0.74
- Line 212: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: next_candidates.push_back(std::move(c));
- Line 218: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: next_candidates.push_back(p);
- Line 225: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: scored.emplace_back(s, cand);
  Confidence: band=high; score=0.74
- Line 259: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: beam.push_back(p);
  Confidence: band=high; score=0.74
- Line 260: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: beam.push_back(p);
- Line 353: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: batch.push_back(test_cases[indices[i]]);
- Line 368: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: errors.push_back("Expected output longer than prompt; prompt may lack detail.");
  Confidence: band=high; score=0.74
- Line 369: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: errors.push_back("Expected output longer than prompt; prompt may lack detail.");
- Line 371: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: errors.push_back("");  // no error

### src/prompt_engineering/prompt_performance_tracker.cpp
Total findings: 15

- Line 105: severity=CRITICAL; category=smart_ptr_misuse
  Description: Raw new without immediate wrapping in smart pointer
  Remediation: Use auto ptr = std::make_unique<T>(...);
  Context: THEMIS_DEBUG("Created new metrics for prompt: {}", prompt_id);
- Line 194: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto it = metrics_.find(prompt_id);
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 137: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto& [id, metrics] : metrics_) {
  Confidence: band=very_high; score=0.9
- Line 152: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto& [id, metrics] : metrics_) {
  Confidence: band=very_high; score=0.9
- Line 173: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto& [id, metrics] : metrics_) {
  Confidence: band=very_high; score=0.9
- Line 219: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: db_->scanPrefix(prefix, [this](std::string_view key, std::string_view) -> bool {
- Line 292: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: db_->scanPrefix(prefix, [this, &loaded](std::string_view /*key*/, std::string_view value) -> bool {
- Line 61: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto time_val = j["last_updated"].get<std::time_t>();
  Confidence: band=high; score=0.74
- Line 66: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto time_val = j["created_at"].get<std::time_t>();
  Confidence: band=high; score=0.74
- Line 137: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(metrics);
  Confidence: band=high; score=0.74
- Line 138: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.push_back(metrics);
- Line 154: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: low_performers.push_back(id);
  Confidence: band=high; score=0.74
- Line 155: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: low_performers.push_back(id);
- Line 174: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: candidates.emplace_back(id, metrics.success_rate);
  Confidence: band=high; score=0.74

### src/prompt_engineering/prompt_regression_runner.cpp
Total findings: 14

- Line 125: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: const std::vector<std::string>& baseline_outputs,
  Confidence: band=very_high; score=0.9
- Line 126: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: const std::vector<std::string>& candidate_outputs) const {
  Confidence: band=very_high; score=0.9
- Line 134: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: baseline_outputs.size(),
  Confidence: band=very_high; score=0.9
- Line 135: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: candidate_outputs.size()});
  Confidence: band=very_high; score=0.9
- Line 139: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: baseline_outputs.size() != candidate_outputs.size() ||
  Confidence: band=very_high; score=0.9
- Line 157: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: baseline_outputs[i], fixture.expected_output);
  Confidence: band=very_high; score=0.9
- Line 159: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: candidate_outputs[i], fixture.expected_output);
  Confidence: band=very_high; score=0.9
- Line 53: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: deltas.push_back({
  Confidence: band=high; score=0.74
- Line 54: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: deltas.push_back({
- Line 107: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: fixtures_.push_back(std::move(f));
  Confidence: band=high; score=0.74
- Line 108: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: fixtures_.push_back(std::move(f));
- Line 171: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: baseline_scores.push_back(bs);
  Confidence: band=high; score=0.74
- Line 172: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: baseline_scores.push_back(bs);
- Line 253: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {

### src/prompt_engineering/cot_tracer.cpp
Total findings: 13

- Line 74: severity=HIGH; category=deadlock_risk
  Description: Multiple locks acquired in nested scope — potential deadlock
  Remediation: Use std::scoped_lock(m1, m2) or enforce consistent lock ordering
  Context: std::lock_guard<std::mutex> lk(mutex_);
- Line 79: severity=HIGH; category=deadlock_risk
  Description: Multiple locks acquired in nested scope — potential deadlock
  Remediation: Use std::scoped_lock(m1, m2) or enforce consistent lock ordering
  Context: std::lock_guard<std::mutex> lk(mutex_);
- Line 97: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto& rec : spans_) {
  Confidence: band=very_high; score=0.9
- Line 125: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (auto& child : children) {
  Confidence: band=very_high; score=0.9
- Line 159: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (auto& child : children) {
  Confidence: band=very_high; score=0.9
- Line 181: severity=HIGH; category=deadlock_risk
  Description: Multiple locks acquired in nested scope — potential deadlock
  Remediation: Use std::scoped_lock(m1, m2) or enforce consistent lock ordering
  Context: std::lock_guard<std::mutex> lk(mutex_);
- Line 186: severity=HIGH; category=deadlock_risk
  Description: Multiple locks acquired in nested scope — potential deadlock
  Remediation: Use std::scoped_lock(m1, m2) or enforce consistent lock ordering
  Context: std::lock_guard<std::mutex> lk(mutex_);
- Line 97: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: arr.push_back(rec.toJson());
  Confidence: band=high; score=0.74
- Line 98: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: arr.push_back(rec.toJson());
- Line 167: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: children_.push_back(std::move(tracer));
  Confidence: band=high; score=0.74
- Line 168: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: children_.push_back(std::move(tracer));
- Line 205: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: arr.push_back(rec.toJson());
  Confidence: band=high; score=0.74
- Line 206: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: arr.push_back(rec.toJson());

### src/prompt_engineering/context_window_manager.cpp
Total findings: 12

- Line 149: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw PromptBudgetExceededError(
- Line 189: severity=HIGH; category=repeated_search
  Description: find/search in loop — O(n²) or worse
  Remediation: Move search outside loop or build index/map before loop
  Context: (std::find(t.activation_rounds.begin(),
- Line 200: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: auto it = t->round_role_weights.find(round_role);
- Line 92: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: selected.push_back(chunk);
  Confidence: band=high; score=0.74
- Line 93: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: selected.push_back(chunk);
- Line 191: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: if (is_active) active.push_back(&t);
  Confidence: band=high; score=0.74
- Line 192: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: if (is_active) active.push_back(&t);
- Line 193: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: else           inactive.push_back(&t);
- Line 230: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(std::move(inj));
  Confidence: band=high; score=0.74
- Line 231: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.push_back(std::move(inj));
- Line 251: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(std::move(inj));
  Confidence: band=high; score=0.74
- Line 252: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.push_back(std::move(inj));

### src/prompt_engineering/prompt_quality_evaluator.cpp
Total findings: 9

- Line 38: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: tokens.push_back(std::move(word));
  Confidence: band=high; score=0.74
- Line 39: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: tokens.push_back(std::move(word));
- Line 45: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: tokens.push_back(std::move(word));
- Line 74: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: failed.push_back(QualityCheck{
  Confidence: band=high; score=0.74
- Line 75: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: failed.push_back(QualityCheck{
- Line 125: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, size_t> bigram_counts;
  Confidence: band=medium; score=0.66
- Line 140: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: failed.push_back(QualityCheck{
  Confidence: band=high; score=0.74
- Line 140: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: failed.push_back(QualityCheck{
  Confidence: band=high; score=0.74
- Line 141: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: failed.push_back(QualityCheck{

### src/prompt_engineering/prompt_engineering_integration.cpp
Total findings: 7

- Line 222: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: std::this_thread::sleep_for(std::chrono::seconds(1));
- Line 460: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: auto reflection_result = snap_tuner->tune(ctx.enhanced_prompt, response);
- Line 94: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto timestamp = j["start_time"].get<std::time_t>();
  Confidence: band=high; score=0.74
- Line 185: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: WorkerStatus status;
- Line 285: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: IntegrationStatus status;
- Line 464: severity=MEDIUM; category=llm_ai_safety; pattern=missing_resource_limits
  Description: LLM inference without token limit or timeout (DOS risk)
  Context: snap_metrics->recordReflectionCycleComplete(
  Confidence: band=high; score=0.74
- Line 608: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, std::string> ctx_map;
  Confidence: band=medium; score=0.66

### src/prompt_engineering/llm_reflection_adapter.cpp
Total findings: 5

- Line 69: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: return "llm-reflection-adapter(" + llm_->name() + ")";
- Line 31: severity=MEDIUM; category=llm_ai_safety; pattern=missing_resource_limits
  Description: LLM inference without token limit or timeout (DOS risk)
  Context: std::string ILLMProviderReflectionAdapter::generate(
  Confidence: band=high; score=0.74
- Line 34: severity=MEDIUM; category=llm_ai_safety; pattern=missing_resource_limits
  Description: LLM inference without token limit or timeout (DOS risk)
  Context: return llm_->complete(prompt);
  Confidence: band=high; score=0.74
- Line 44: severity=MEDIUM; category=llm_ai_safety; pattern=missing_resource_limits
  Description: LLM inference without token limit or timeout (DOS risk)
  Context: return llm_->complete(critique_prompt);
  Confidence: band=high; score=0.74
- Line 55: severity=MEDIUM; category=llm_ai_safety; pattern=missing_resource_limits
  Description: LLM inference without token limit or timeout (DOS risk)
  Context: return llm_->complete(revision_prompt);
  Confidence: band=high; score=0.74

### src/prompt_engineering/rag_context_budget_manager.cpp
Total findings: 5

- Line 27: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument(
- Line 32: severity=HIGH; category=observability; pattern=missing_trace_point
  Description: Critical function allocate without trace point
  Context: BudgetHandle RagContextBudgetManager::allocate(size_t tokens) {
  Confidence: band=very_high; score=0.9
- Line 40: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw BudgetExhaustedError(tokens, avail, total_budget_);
- Line 74: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: allocated_.store(0, std::memory_order_relaxed);
- Line 79: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: const size_t alloc = allocated_.load(std::memory_order_relaxed);

### src/prompt_engineering/prompt_engineering_metrics.cpp
Total findings: 4

- Line 729: severity=HIGH; category=memory_order
  Description: memory_order_relaxed used — potential visibility issue
  Remediation: Use memory_order_acquire/release unless truly lock-free
  Context: if (snapshot.contains(key)) target.store(snapshot[key].get<int64_t>(), std::memory_order_relaxed);
- Line 732: severity=HIGH; category=memory_order
  Description: memory_order_relaxed used — potential visibility issue
  Remediation: Use memory_order_acquire/release unless truly lock-free
  Context: if (snapshot.contains(key)) target.store(snapshot[key].get<int>(), std::memory_order_relaxed);
- Line 735: severity=HIGH; category=memory_order
  Description: memory_order_relaxed used — potential visibility issue
  Remediation: Use memory_order_acquire/release unless truly lock-free
  Context: if (snapshot.contains(key)) target.store(snapshot[key].get<double>(), std::memory_order_relaxed);
- Line 797: severity=MEDIUM; category=llm_ai_safety; pattern=missing_resource_limits
  Description: LLM inference without token limit or timeout (DOS risk)
  Context: void PromptEngineeringMetrics::recordReflectionCycleComplete(
  Confidence: band=high; score=0.74

### src/prompt_engineering/prompt_optimizer.cpp
Total findings: 4

- Line 114: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: result.metadata["initial_score"] = result.score_history[0];
- Line 115: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: result.metadata["improvement"] = current_score - result.score_history[0];
- Line 116: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: result.metadata["relative_improvement"] =
- Line 171: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: auto meta_result = meta_gen.generateImprovementPrompt(
  Confidence: band=very_high; score=0.9

### src/prompt_engineering/rag_prompt_builder.cpp
Total findings: 4

- Line 61: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: ordered.push_back(&c);
  Confidence: band=high; score=0.74
- Line 62: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: ordered.push_back(&c);
- Line 83: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: selected.push_back(chunk);
  Confidence: band=high; score=0.74
- Line 84: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: selected.push_back(chunk);

### src/prompt_engineering/system_prompt_manager.cpp
Total findings: 4

- Line 81: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto& [key, value] : context) {
  Confidence: band=very_high; score=0.9
- Line 78: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: const std::unordered_map<std::string, std::string>& context) {
  Confidence: band=medium; score=0.66
- Line 193: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(sp);
  Confidence: band=high; score=0.74
- Line 194: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.push_back(sp);

### src/prompt_engineering/prompt_template_validator.cpp
Total findings: 3

- Line 95: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.errors.push_back(
  Confidence: band=high; score=0.74
- Line 96: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.errors.push_back(
- Line 102: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.errors.push_back(

### src/prompt_engineering/adversarial_prompt_tester.cpp
Total findings: 2

- Line 202: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: cases_.push_back({tc.id, tc.category, tc.payload, tc.expected_blocked});
  Confidence: band=high; score=0.74
- Line 255: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: report.results.push_back(std::move(r));
  Confidence: band=high; score=0.74

### src/prompt_engineering/markdown_utils.cpp
Total findings: 2

- Line 25: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: // Handle empty or too-short input
  Confidence: band=very_high; score=0.99
- Line 61: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: static const std::regex line_comment(R"(//[^\r\n]*)",

## Update Workflow

- Refresh scan artifacts with: python tools/gap_scanner_v3.py
- Regenerate all module notes with: python tools/module_doc_generator.py . ai_working ai_working/module_gaps
- The generator mirrors each archive document directly into src/<module>/MODULE_GAPS.md.

Format: THEMIS_MODULE_GAPS_V3
