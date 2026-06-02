# ethics_ai Module - Developer Gap Note

> Auto-generated from ai_working/gap_scan_v3_aggregate.json.
> This file is overwritten on each regeneration.

## Scan Snapshot

- Module: ethics_ai
- Generated: 2026-06-02 11:09:13
- Status: Critical Findings Present
- Total Findings: 111
- Actionable Findings (Critical + High): 24
- Affected Files: 17

## Severity Summary

| Severity | Count |
|---|---:|
| Critical | 9 |
| High | 15 |
| Medium | 87 |
| Low | 0 |

## Category Summary

| Category | Count |
|---|---:|
| performance_patterns | 75 |
| container | 25 |
| determinism | 15 |
| performance | 14 |
| llm_ai_safety | 11 |
| audit_logging | 10 |
| reliability | 9 |
| platform | 4 |
| concurrency | 3 |
| memory | 2 |
| raii | 2 |
| exception_safety | 1 |
| security | 1 |
| uninitialized | 1 |

## File Overview

| File | Findings | Critical | High | Medium | Low |
|---|---:|---:|---:|---:|---:|
| src/ethics_ai/ethics_selection_router.cpp | 24 | 0 | 2 | 22 | 0 |
| src/ethics_ai/philosophy_loader.cpp | 16 | 0 | 0 | 16 | 0 |
| src/ethics_ai/tournament_mode_selector.cpp | 13 | 0 | 1 | 12 | 0 |
| src/ethics_ai/argument_store.cpp | 12 | 8 | 0 | 4 | 0 |
| src/ethics_ai/rag_context_engine.cpp | 8 | 0 | 0 | 8 | 0 |
| src/ethics_ai/convergence_marker_engine.cpp | 7 | 0 | 7 | 0 | 0 |
| src/ethics_ai/prior_round_compressor.cpp | 6 | 0 | 0 | 6 | 0 |
| src/ethics_ai/discourse_engine.cpp | 5 | 0 | 2 | 3 | 0 |
| src/ethics_ai/ethics_profile_registry.cpp | 4 | 0 | 0 | 4 | 0 |
| src/ethics_ai/chain_visualizer.cpp | 3 | 0 | 0 | 3 | 0 |
| src/ethics_ai/discourse_memory_store.cpp | 3 | 0 | 0 | 3 | 0 |
| src/ethics_ai/synthesis_matrix_builder.cpp | 3 | 1 | 1 | 1 | 0 |
| src/ethics_ai/cross_school_tension_resolver.cpp | 2 | 0 | 0 | 2 | 0 |
| src/ethics_ai/ethics_ai_plugin.cpp | 2 | 0 | 0 | 2 | 0 |
| src/ethics_ai/position_abstract_validator.cpp | 2 | 0 | 2 | 0 | 0 |
| src/ethics_ai/ethics_evaluator.cpp | 1 | 0 | 0 | 1 | 0 |
| src/ethics_ai/llm_cascade_router.cpp | 0 | 0 | 0 | 0 | 0 |

## Full Scanner Findings

### src/ethics_ai/ethics_selection_router.cpp
Total findings: 24

- Line 293: severity=HIGH; category=performance; pattern=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Context: auto it = id_to_text.find(sid);
  Confidence: band=very_high; score=0.9
- Line 342: severity=HIGH; category=performance; pattern=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Context: auto it = id_to_text.find(sid);
  Confidence: band=very_high; score=0.9
- Line 43: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: if (cur.size() >= 3) tokens.push_back(cur);
  Confidence: band=high; score=0.74
- Line 53: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, double> termFreq(
  Confidence: band=medium; score=0.66
- Line 56: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, double> freq;
  Confidence: band=medium; score=0.66
- Line 68: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: const std::unordered_map<std::string, double>& a,
  Confidence: band=medium; score=0.66
- Line 69: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: const std::unordered_map<std::string, double>& b)
  Confidence: band=medium; score=0.66
- Line 151: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: const std::unordered_set<std::string>& candidates) const;
  Confidence: band=medium; score=0.66
- Line 174: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: taxonomy_map[cls].push_back(item.as<std::string>());
  Confidence: band=high; score=0.74
- Line 174: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: taxonomy_map[cls].push_back(item.as<std::string>());
  Confidence: band=high; score=0.74
- Line 186: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: domain_class_map[dom].push_back(item.as<std::string>());
  Confidence: band=high; score=0.74
- Line 186: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: domain_class_map[dom].push_back(item.as<std::string>());
  Confidence: band=high; score=0.74
- Line 203: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> EthicsSelectionRouter::Impl::stage1(
  Confidence: band=medium; score=0.66
- Line 208: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> candidates;
  Confidence: band=medium; score=0.66
- Line 245: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> valid;
  Confidence: band=medium; score=0.66
- Line 271: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: const std::unordered_set<std::string>& candidates) const
  Confidence: band=medium; score=0.66
- Line 278: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, std::string> id_to_text;
  Confidence: band=medium; score=0.66
- Line 282: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: for (const auto& t : m.tags)              profile_text += " " + t;
  Confidence: band=high; score=0.74
- Line 283: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: for (const auto& d : m.applicable_domains) profile_text += " " + d;
  Confidence: band=high; score=0.74
- Line 307: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(cand);
  Confidence: band=high; score=0.74
- Line 326: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, std::string> id_to_text;
  Confidence: band=medium; score=0.66
- Line 330: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: for (const auto& t : m.tags)             profile_text += " " + t;
  Confidence: band=high; score=0.74
- Line 331: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: for (const auto& d : m.applicable_domains) profile_text += " " + d;
  Confidence: band=high; score=0.74
- Line 352: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(cand);
  Confidence: band=high; score=0.74

### src/ethics_ai/philosophy_loader.cpp
Total findings: 16

- Line 90: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: acc += "; ";
  Confidence: band=high; score=0.74
- Line 90: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: acc += "; ";
  Confidence: band=high; score=0.74
- Line 136: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: acc += "; ";
  Confidence: band=high; score=0.74
- Line 136: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: acc += "; ";
  Confidence: band=high; score=0.74
- Line 155: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: acc += "; ";
  Confidence: band=high; score=0.74
- Line 155: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: acc += "; ";
  Confidence: band=high; score=0.74
- Line 170: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: profile.main_theses.push_back(text);
  Confidence: band=high; score=0.74
- Line 189: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: pt.activation_rounds.push_back(r.as<int>());
  Confidence: band=high; score=0.74
- Line 198: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: profile.typed_theses.push_back(std::move(pt));
  Confidence: band=high; score=0.74
- Line 209: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: profile.secondary_theses.push_back(text);
  Confidence: band=high; score=0.74
- Line 225: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: pt.activation_rounds.push_back(r.as<int>());
  Confidence: band=high; score=0.74
- Line 234: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: profile.typed_theses.push_back(std::move(pt));
  Confidence: band=high; score=0.74
- Line 259: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: profile.strengths.push_back(text);
  Confidence: band=high; score=0.74
- Line 259: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: profile.strengths.push_back(text);
  Confidence: band=high; score=0.74
- Line 269: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: profile.weaknesses.push_back(text);
  Confidence: band=high; score=0.74
- Line 321: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: ids.push_back(kv.first);
  Confidence: band=high; score=0.74

### src/ethics_ai/tournament_mode_selector.cpp
Total findings: 13

- Line 111: severity=HIGH; category=determinism; pattern=fp_exact_comparison
  Description: Floating-point exact comparison (use tolerance/epsilon)
  Context: if (wa != wb) {
  Confidence: band=very_high; score=0.9
- Line 77: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: std::map<std::string, std::size_t> school_to_arg_index;
  Confidence: band=high; score=0.74
- Line 90: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: std::map<std::string, float> school_weight;
  Confidence: band=high; score=0.74
- Line 104: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: ordered_schools.push_back(kv.first);
  Confidence: band=high; score=0.74
- Line 121: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.primary_opponents.push_back(school);
  Confidence: band=high; score=0.74
- Line 127: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.secondary_opponents.push_back(school);
  Confidence: band=high; score=0.74
- Line 138: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.primary_opponents.push_back(school);
  Confidence: band=high; score=0.74
- Line 159: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: std::map<std::string, TournamentSelectionResult> TournamentModeSelector::buildTournamentContext(
  Confidence: band=high; score=0.74
- Line 161: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: const std::map<std::string, std::vector<SchoolTension>> &tensions_per_school,
  Confidence: band=high; score=0.74
- Line 169: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: std::map<std::string, TournamentSelectionResult> results;
  Confidence: band=high; score=0.74
- Line 174: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: opponent_args.push_back(arg);
  Confidence: band=high; score=0.74
- Line 174: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: opponent_args.push_back(arg);
  Confidence: band=high; score=0.74
- Line 174: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: opponent_args.push_back(arg);
  Confidence: band=high; score=0.74

### src/ethics_ai/argument_store.cpp
Total findings: 12

- Line 116: severity=CRITICAL; category=llm_ai_safety; pattern=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Context: // Deserialize BaseEntity
  Confidence: band=very_high; score=0.99
- Line 117: severity=CRITICAL; category=llm_ai_safety; pattern=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Context: BaseEntity entity = BaseEntity::deserialize(argument_id, *blob);
  Confidence: band=very_high; score=0.99
- Line 214: severity=CRITICAL; category=llm_ai_safety; pattern=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Context: // Deserialize BaseEntity
  Confidence: band=very_high; score=0.99
- Line 216: severity=CRITICAL; category=llm_ai_safety; pattern=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Context: BaseEntity entity = BaseEntity::deserialize(pk, blob);
  Confidence: band=very_high; score=0.99
- Line 298: severity=CRITICAL; category=llm_ai_safety; pattern=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Context: // Deserialize BaseEntity
  Confidence: band=very_high; score=0.99
- Line 299: severity=CRITICAL; category=llm_ai_safety; pattern=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Context: BaseEntity entity = BaseEntity::deserialize(decision_id, *blob);
  Confidence: band=very_high; score=0.99
- Line 355: severity=CRITICAL; category=llm_ai_safety; pattern=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Context: // Deserialize BaseEntity
  Confidence: band=very_high; score=0.99
- Line 356: severity=CRITICAL; category=llm_ai_safety; pattern=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Context: BaseEntity entity = BaseEntity::deserialize(school, *blob);
  Confidence: band=very_high; score=0.99
- Line 155: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results.push_back(arg);
  Confidence: band=high; score=0.74
- Line 155: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results.push_back(arg);
  Confidence: band=high; score=0.74
- Line 188: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: out.push_back(EthicsBaseEntityAdapter::fromBaseEntity(entity));
  Confidence: band=high; score=0.74
- Line 425: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: rounds.push_back(round);
  Confidence: band=high; score=0.74

### src/ethics_ai/rag_context_engine.cpp
Total findings: 8

- Line 40: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: arg_ids.push_back(arg.id);
  Confidence: band=high; score=0.74
- Line 40: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: arg_ids.push_back(arg.id);
  Confidence: band=high; score=0.74
- Line 94: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results.push_back(scored[i].second);
  Confidence: band=high; score=0.74
- Line 178: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: scored.emplace_back(cosine, arg.id);
  Confidence: band=high; score=0.74
- Line 178: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: scored.emplace_back(cosine, arg.id);
  Confidence: band=high; score=0.74
- Line 188: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results.emplace_back(scored[i].second, scored[i].first);
  Confidence: band=high; score=0.74
- Line 203: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> visited;
  Confidence: band=medium; score=0.66
- Line 222: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: visited_order.push_back(nid);
  Confidence: band=high; score=0.74

### src/ethics_ai/convergence_marker_engine.cpp
Total findings: 7

- Line 49: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: const std::vector<DiscourseRoundOutput>& round_outputs,
  Confidence: band=very_high; score=0.9
- Line 54: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: if (round_outputs.size() < 2) {
  Confidence: band=very_high; score=0.9
- Line 61: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: for (const auto& out : round_outputs) {
  Confidence: band=very_high; score=0.9
- Line 80: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: for (size_t i = 0; i < round_outputs.size(); ++i) {
  Confidence: band=very_high; score=0.9
- Line 81: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: for (size_t j = i + 1; j < round_outputs.size(); ++j) {
  Confidence: band=very_high; score=0.9
- Line 82: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: const auto& a = round_outputs[i];
  Confidence: band=very_high; score=0.9
- Line 83: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: const auto& b = round_outputs[j];
  Confidence: band=very_high; score=0.9

### src/ethics_ai/prior_round_compressor.cpp
Total findings: 6

- Line 50: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: citations.push_back(match);
  Confidence: band=high; score=0.74
- Line 62: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: citations.push_back(match);
  Confidence: band=high; score=0.74
- Line 74: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: citations.push_back(match);
  Confidence: band=high; score=0.74
- Line 223: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: sentences.push_back(current.substr(start, end - start + 1));
  Confidence: band=high; score=0.74
- Line 326: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: scored.emplace_back(score, i);
  Confidence: band=high; score=0.74
- Line 342: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: selected_indices.push_back(idx);
  Confidence: band=high; score=0.74

### src/ethics_ai/discourse_engine.cpp
Total findings: 5

- Line 64: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: // Validate inputs
  Confidence: band=very_high; score=0.9
- Line 64: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: // Validate inputs
  Confidence: band=very_high; score=0.9
- Line 85: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: arguments.push_back(arg);
  Confidence: band=high; score=0.74
- Line 235: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: prev_arg_ids.push_back(arg.id);
  Confidence: band=high; score=0.74
- Line 275: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: all.push_back(a);
  Confidence: band=high; score=0.74

### src/ethics_ai/ethics_profile_registry.cpp
Total findings: 4

- Line 40: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: if (!v.empty()) result.push_back(v);
  Confidence: band=high; score=0.74
- Line 111: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results.push_back(meta);
  Confidence: band=high; score=0.74
- Line 164: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: std::map<std::string, EthicsProfileMeta> new_index;
  Confidence: band=high; score=0.74
- Line 247: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto v = root["school_id"].as<std::string>("");
  Confidence: band=high; score=0.74

### src/ethics_ai/chain_visualizer.cpp
Total findings: 3

- Line 49: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: if (c == '"')       out += "\\\"";
  Confidence: band=high; score=0.74
- Line 79: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> id_set(argument_ids.begin(), argument_ids.end());
  Confidence: band=medium; score=0.66
- Line 130: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> id_set(argument_ids.begin(), argument_ids.end());
  Confidence: band=medium; score=0.66

### src/ethics_ai/discourse_memory_store.cpp
Total findings: 3

- Line 99: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(buf[static_cast<std::size_t>(i)]);
  Confidence: band=high; score=0.74
- Line 127: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: std::map<std::string, std::string>
  Confidence: band=high; score=0.74
- Line 129: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: std::map<std::string, std::string> result;
  Confidence: band=high; score=0.74

### src/ethics_ai/synthesis_matrix_builder.cpp
Total findings: 3

- Line 86: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: oss << "[POSITIONS-MATRIX — R4 SYNTHESIS INPUT]\n";
  Confidence: band=very_high; score=0.99
- Line 86: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: oss << "[POSITIONS-MATRIX — R4 SYNTHESIS INPUT]\n";
  Confidence: band=very_high; score=0.9
- Line 69: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: summary.core_thesis_ids.push_back(round_output.core_thesis_ids[i]);
  Confidence: band=high; score=0.74

### src/ethics_ai/cross_school_tension_resolver.cpp
Total findings: 2

- Line 39: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: parts.push_back(token);
  Confidence: band=high; score=0.74
- Line 131: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: decisions.push_back(std::move(decision));
  Confidence: band=high; score=0.74

### src/ethics_ai/ethics_ai_plugin.cpp
Total findings: 2

- Line 451: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: std::map<std::string, double> getStatistics() const override {
  Confidence: band=high; score=0.74
- Line 454: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: std::map<std::string, double> stats;
  Confidence: band=high; score=0.74

### src/ethics_ai/position_abstract_validator.cpp
Total findings: 2

- Line 123: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: void PositionAbstractValidator::validateBatch(std::vector<DiscourseRoundOutput> &outputs) const {
  Confidence: band=very_high; score=0.9
- Line 124: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: for (auto &out : outputs) {
  Confidence: band=very_high; score=0.9

### src/ethics_ai/ethics_evaluator.cpp
Total findings: 1

- Line 258: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: confidence_sum_micro_ += static_cast<uint64_t>(confidence * 1'000'000.0);
  Confidence: band=high; score=0.74

### src/ethics_ai/llm_cascade_router.cpp
Total findings: 0

## Update Workflow

- Refresh scan artifacts with: python tools/gap_scanner_v3.py
- Regenerate all module notes with: python tools/module_doc_generator.py . ai_working ai_working/module_gaps
- The generator mirrors each archive document directly into src/<module>/MODULE_GAPS.md.

Format: THEMIS_MODULE_GAPS_V3
