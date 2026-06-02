# ethics_ai Module - Developer Gap Note

> Auto-generated from ai_working/gap_scan_v3_aggregate.json.
> This file is overwritten on each regeneration.

## Scan Snapshot

- Module: ethics_ai
- Generated: 2026-06-02 11:55:48
- Status: Critical Findings Present
- Total Findings: 173
- Actionable Findings (Critical + High): 56
- Affected Files: 17

## Severity Summary

| Severity | Count |
|---|---:|
| Critical | 14 |
| High | 42 |
| Medium | 117 |
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
| src/ethics_ai/ethics_selection_router.cpp | 34 | 1 | 5 | 28 | 0 |
| src/ethics_ai/philosophy_loader.cpp | 22 | 0 | 1 | 21 | 0 |
| src/ethics_ai/argument_store.cpp | 15 | 8 | 1 | 6 | 0 |
| src/ethics_ai/tournament_mode_selector.cpp | 15 | 0 | 3 | 12 | 0 |
| src/ethics_ai/prior_round_compressor.cpp | 13 | 1 | 4 | 8 | 0 |
| src/ethics_ai/rag_context_engine.cpp | 11 | 2 | 0 | 9 | 0 |
| src/ethics_ai/synthesis_matrix_builder.cpp | 10 | 1 | 6 | 3 | 0 |
| src/ethics_ai/chain_visualizer.cpp | 9 | 0 | 0 | 9 | 0 |
| src/ethics_ai/convergence_marker_engine.cpp | 8 | 0 | 7 | 1 | 0 |
| src/ethics_ai/ethics_ai_plugin.cpp | 7 | 1 | 2 | 4 | 0 |
| src/ethics_ai/ethics_profile_registry.cpp | 7 | 0 | 3 | 4 | 0 |
| src/ethics_ai/position_abstract_validator.cpp | 7 | 0 | 6 | 1 | 0 |
| src/ethics_ai/discourse_engine.cpp | 6 | 0 | 2 | 4 | 0 |
| src/ethics_ai/discourse_memory_store.cpp | 4 | 0 | 1 | 3 | 0 |
| src/ethics_ai/cross_school_tension_resolver.cpp | 3 | 0 | 0 | 3 | 0 |
| src/ethics_ai/ethics_evaluator.cpp | 1 | 0 | 0 | 1 | 0 |
| src/ethics_ai/llm_cascade_router.cpp | 1 | 0 | 1 | 0 | 0 |

## Full Scanner Findings

### src/ethics_ai/ethics_selection_router.cpp
Total findings: 34

- Line 211: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto it = taxonomy_map.find(cls);
- Line 75: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: auto it = b.find(t);
- Line 292: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: auto it = id_to_text.find(sid);
- Line 293: severity=HIGH; category=performance; pattern=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Context: auto it = id_to_text.find(sid);
  Confidence: band=very_high; score=0.9
- Line 341: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: auto it = id_to_text.find(sid);
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
- Line 175: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: taxonomy_map[cls].push_back(item.as<std::string>());
- Line 186: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: domain_class_map[dom].push_back(item.as<std::string>());
  Confidence: band=high; score=0.74
- Line 186: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: domain_class_map[dom].push_back(item.as<std::string>());
  Confidence: band=high; score=0.74
- Line 187: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: domain_class_map[dom].push_back(item.as<std::string>());
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
- Line 283: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: for (const auto& t : m.tags)              profile_text += " " + t;
- Line 284: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: for (const auto& d : m.applicable_domains) profile_text += " " + d;
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
- Line 331: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: for (const auto& t : m.tags)             profile_text += " " + t;
- Line 332: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: for (const auto& d : m.applicable_domains) profile_text += " " + d;
- Line 352: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(cand);
  Confidence: band=high; score=0.74

### src/ethics_ai/philosophy_loader.cpp
Total findings: 22

- Line 37: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: for (const auto &entry : fs::directory_iterator(directory)) {
- Line 90: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: acc += "; ";
  Confidence: band=high; score=0.74
- Line 90: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: acc += "; ";
  Confidence: band=high; score=0.74
- Line 91: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: acc += "; ";
- Line 136: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: acc += "; ";
  Confidence: band=high; score=0.74
- Line 136: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: acc += "; ";
  Confidence: band=high; score=0.74
- Line 137: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: acc += "; ";
- Line 155: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: acc += "; ";
  Confidence: band=high; score=0.74
- Line 155: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: acc += "; ";
  Confidence: band=high; score=0.74
- Line 156: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: acc += "; ";
- Line 170: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: profile.main_theses.push_back(text);
  Confidence: band=high; score=0.74
- Line 189: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: pt.activation_rounds.push_back(r.as<int>());
  Confidence: band=high; score=0.74
- Line 190: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: pt.activation_rounds.push_back(r.as<int>());
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
- Line 226: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: pt.activation_rounds.push_back(r.as<int>());
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

### src/ethics_ai/argument_store.cpp
Total findings: 15

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
- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: * PR History (last 5): #4833 Continue Phase-6 tensorgrap... (2026-05-07) | #946 [FEATURE] Ethics AI
- Line 155: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results.push_back(arg);
  Confidence: band=high; score=0.74
- Line 155: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results.push_back(arg);
  Confidence: band=high; score=0.74
- Line 171: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: q.predicates.push_back({"philosophy_school", philosophy_school});
- Line 188: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: out.push_back(EthicsBaseEntityAdapter::fromBaseEntity(entity));
  Confidence: band=high; score=0.74
- Line 189: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: out.push_back(EthicsBaseEntityAdapter::fromBaseEntity(entity));
- Line 425: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: rounds.push_back(round);
  Confidence: band=high; score=0.74

### src/ethics_ai/tournament_mode_selector.cpp
Total findings: 15

- Line 111: severity=HIGH; category=determinism; pattern=fp_exact_comparison
  Description: Floating-point exact comparison (use tolerance/epsilon)
  Context: if (wa != wb) {
  Confidence: band=very_high; score=0.9
- Line 180: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: auto it = tensions_per_school.find(school_id);
- Line 180: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: auto it = tensions_per_school.find(school_id);
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

### src/ethics_ai/prior_round_compressor.cpp
Total findings: 13

- Line 294: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: score += static_cast<float>(it->second);
- Line 48: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: for (auto it = begin; it != std::sregex_iterator(); ++it) {
- Line 60: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: for (auto it = begin; it != std::sregex_iterator(); ++it) {
- Line 72: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: for (auto it = begin; it != std::sregex_iterator(); ++it) {
- Line 291: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: auto it = word_freq.find(word);
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
- Line 224: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: sentences.push_back(current.substr(start, end - start + 1));
- Line 233: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: sentences.push_back(current.substr(start, end - start + 1));
- Line 326: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: scored.emplace_back(score, i);
  Confidence: band=high; score=0.74
- Line 342: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: selected_indices.push_back(idx);
  Confidence: band=high; score=0.74

### src/ethics_ai/rag_context_engine.cpp
Total findings: 11

- Line 56: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto arg_result = store_->getArgument(id);
- Line 218: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto arg_result = store_->getArgument(current_id);
- Line 40: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: arg_ids.push_back(arg.id);
  Confidence: band=high; score=0.74
- Line 40: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: arg_ids.push_back(arg.id);
  Confidence: band=high; score=0.74
- Line 41: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: arg_ids.push_back(arg.id);
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

### src/ethics_ai/synthesis_matrix_builder.cpp
Total findings: 10

- Line 86: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: oss << "[POSITIONS-MATRIX — R4 SYNTHESIS INPUT]\n";
  Confidence: band=very_high; score=0.99
- Line 41: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw SchemaValidationError("school_id must not be empty");
- Line 44: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw SchemaValidationError(
- Line 48: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw SchemaValidationError("confidence must be in [0.0, 1.0]");
- Line 48: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: throw SchemaValidationError("confidence must be in [0.0, 1.0]");
- Line 51: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw SchemaValidationError("core_thesis_ids must not be empty");
- Line 86: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: oss << "[POSITIONS-MATRIX — R4 SYNTHESIS INPUT]\n";
  Confidence: band=very_high; score=0.9
- Line 69: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: summary.core_thesis_ids.push_back(round_output.core_thesis_ids[i]);
  Confidence: band=high; score=0.74
- Line 70: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: summary.core_thesis_ids.push_back(round_output.core_thesis_ids[i]);
- Line 113: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: << " \xe2\x86\x94 "   // UTF-8 ↔

### src/ethics_ai/chain_visualizer.cpp
Total findings: 9

- Line 49: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: if (c == '"')       out += "\\\"";
  Confidence: band=high; score=0.74
- Line 50: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: if (c == '"')       out += "\\\"";
- Line 51: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: else if (c == '\\') out += "\\\\";
- Line 62: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: if (c == '"')  out += "'";
- Line 63: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: else if (c == '\n') out += "<br/>";
- Line 63: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: else if (c == '\n') out += "<br/>";
- Line 63: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: else if (c == '\n') out += "<br/>";
- Line 79: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> id_set(argument_ids.begin(), argument_ids.end());
  Confidence: band=medium; score=0.66
- Line 130: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> id_set(argument_ids.begin(), argument_ids.end());
  Confidence: band=medium; score=0.66

### src/ethics_ai/convergence_marker_engine.cpp
Total findings: 8

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
- Line 143: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: << " \xe2\x86\x94 "  // UTF-8 ↔

### src/ethics_ai/ethics_ai_plugin.cpp
Total findings: 7

- Line 491: severity=CRITICAL; category=smart_ptr_misuse
  Description: Raw new without immediate wrapping in smart pointer
  Remediation: Use auto ptr = std::make_unique<T>(...);
  Context: return new themis::plugins::ethics::EthicsAIPlugin();
- Line 117: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: auto status = argument_store_->initialize(nullptr);
- Line 495: severity=HIGH; category=delete_no_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: After delete: plugin = nullptr;
  Context: delete plugin;
- Line 0: severity=MEDIUM; category=uncategorized
  Context: Struct with uninitialized fields
  Confidence: band=medium; score=0.65
- Line 451: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: std::map<std::string, double> getStatistics() const override {
  Confidence: band=high; score=0.74
- Line 454: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: std::map<std::string, double> stats;
  Confidence: band=high; score=0.74
- Line 495: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: delete plugin;

### src/ethics_ai/ethics_profile_registry.cpp
Total findings: 7

- Line 90: severity=HIGH; category=repeated_search
  Description: find/search in loop — O(n²) or worse
  Remediation: Move search outside loop or build index/map before loop
  Context: if (std::find(meta.tags.begin(), meta.tags.end(), t) == meta.tags.end()) {
- Line 102: severity=HIGH; category=repeated_search
  Description: find/search in loop — O(n²) or worse
  Remediation: Move search outside loop or build index/map before loop
  Context: if (std::find(meta.applicable_domains.begin(),
- Line 166: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: for (const auto& entry : fs::recursive_directory_iterator(directory)) {
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

### src/ethics_ai/position_abstract_validator.cpp
Total findings: 7

- Line 77: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw PositionAbstractSchemaError(output.school_id, output.round_number,
- Line 87: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw PositionAbstractSchemaError(output.school_id, output.round_number, "position_abstract is empty
- Line 95: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw PositionAbstractSchemaError(output.school_id, output.round_number,
- Line 105: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw PositionAbstractSchemaError(output.school_id, output.round_number, "core_thesis_ids is empty")
- Line 123: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: void PositionAbstractValidator::validateBatch(std::vector<DiscourseRoundOutput> &outputs) const {
  Confidence: band=very_high; score=0.9
- Line 124: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: for (auto &out : outputs) {
  Confidence: band=very_high; score=0.9
- Line 50: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: thesis_joined += ", ";

### src/ethics_ai/discourse_engine.cpp
Total findings: 6

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
- Line 236: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: prev_arg_ids.push_back(arg.id);
- Line 275: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: all.push_back(a);
  Confidence: band=high; score=0.74

### src/ethics_ai/discourse_memory_store.cpp
Total findings: 4

- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
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

### src/ethics_ai/cross_school_tension_resolver.cpp
Total findings: 3

- Line 39: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: parts.push_back(token);
  Confidence: band=high; score=0.74
- Line 63: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 131: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: decisions.push_back(std::move(decision));
  Confidence: band=high; score=0.74

### src/ethics_ai/ethics_evaluator.cpp
Total findings: 1

- Line 258: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: confidence_sum_micro_ += static_cast<uint64_t>(confidence * 1'000'000.0);
  Confidence: band=high; score=0.74

### src/ethics_ai/llm_cascade_router.cpp
Total findings: 1

- Line 32: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: budget.context_k = ctx_it->second;

## Update Workflow

- Refresh scan artifacts with: python tools/gap_scanner_v3.py
- Regenerate all module notes with: python tools/module_doc_generator.py . ai_working ai_working/module_gaps
- The generator mirrors each archive document directly into src/<module>/MODULE_GAPS.md.

Format: THEMIS_MODULE_GAPS_V3
