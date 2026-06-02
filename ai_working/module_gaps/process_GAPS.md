# process Module - Developer Gap Note

> Auto-generated from ai_working/gap_scan_v3_aggregate.json.
> This file is overwritten on each regeneration.

## Scan Snapshot

- Module: process
- Generated: 2026-06-02 11:09:13
- Status: High-Priority Findings Present
- Total Findings: 191
- Actionable Findings (Critical + High): 22
- Affected Files: 17

## Severity Summary

| Severity | Count |
|---|---:|
| Critical | 0 |
| High | 22 |
| Medium | 169 |
| Low | 0 |

## Category Summary

| Category | Count |
|---|---:|
| performance_patterns | 136 |
| container | 56 |
| determinism | 38 |
| performance | 31 |
| platform | 27 |
| reliability | 21 |
| audit_logging | 11 |
| memory | 10 |
| exception_safety | 7 |
| llm_ai_safety | 4 |
| observability | 2 |
| concurrency | 1 |
| input_validation | 1 |
| type_conversion | 1 |
| uninitialized | 1 |

## File Overview

| File | Findings | Critical | High | Medium | Low |
|---|---:|---:|---:|---:|---:|
| src/process/process_graph_rag.cpp | 47 | 0 | 3 | 44 | 0 |
| src/process/dmn_evaluator.cpp | 17 | 0 | 11 | 6 | 0 |
| src/process/ocel_exporter.cpp | 17 | 0 | 1 | 16 | 0 |
| src/process/vcc_vpb_importer.cpp | 16 | 0 | 5 | 11 | 0 |
| src/process/object_centric_tracer.cpp | 13 | 0 | 0 | 13 | 0 |
| src/process/process_community_detector.cpp | 11 | 0 | 2 | 9 | 0 |
| src/process/epk_serializer.cpp | 10 | 0 | 0 | 10 | 0 |
| src/process/process_model_generator.cpp | 10 | 0 | 0 | 10 | 0 |
| src/process/process_model_manager.cpp | 9 | 0 | 0 | 9 | 0 |
| src/process/process_agentic_rag.cpp | 8 | 0 | 0 | 8 | 0 |
| src/process/fim_importer.cpp | 7 | 0 | 0 | 7 | 0 |
| src/process/llm_process_descriptor.cpp | 6 | 0 | 0 | 6 | 0 |
| src/process/bpmn_serializer.cpp | 5 | 0 | 0 | 5 | 0 |
| src/process/epk_aris_xml_importer.cpp | 4 | 0 | 0 | 4 | 0 |
| src/process/process_light_retriever.cpp | 4 | 0 | 0 | 4 | 0 |
| src/process/process_linker.cpp | 4 | 0 | 0 | 4 | 0 |
| src/process/cmmn_serializer.cpp | 3 | 0 | 0 | 3 | 0 |

## Full Scanner Findings

### src/process/process_graph_rag.cpp
Total findings: 47

- Line 367: severity=HIGH; category=performance; pattern=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Context: auto fi = node_index.find(from);
  Confidence: band=very_high; score=0.9
- Line 368: severity=HIGH; category=performance; pattern=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Context: auto ti = node_index.find(to);
  Confidence: band=very_high; score=0.9
- Line 378: severity=HIGH; category=performance; pattern=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Context: auto it = node_index.find(seed);
  Confidence: band=very_high; score=0.9
- Line 59: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: float jaccardSimilarity(const std::unordered_set<std::string>& a,
  Confidence: band=medium; score=0.66
- Line 60: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: const std::unordered_set<std::string>& b) {
  Confidence: band=medium; score=0.66
- Line 134: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: kgn.aliases.push_back(n["name_en"].get<std::string>());
  Confidence: band=high; score=0.74
- Line 170: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: kg.edges.push_back(std::move(kge));
  Confidence: band=high; score=0.74
- Line 216: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: kg.nodes.push_back(std::move(tok_node));
  Confidence: band=high; score=0.74
- Line 246: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: kg.nodes.push_back(std::move(att_node));
  Confidence: band=high; score=0.74
- Line 279: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, std::vector<std::pair<std::string, json>>> adj;
  Confidence: band=medium; score=0.66
- Line 283: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: adj[from].push_back({to, e});
  Confidence: band=high; score=0.74
- Line 290: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> visited_nodes;
  Confidence: band=medium; score=0.66
- Line 291: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> visited_edges;
  Confidence: band=medium; score=0.66
- Line 318: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result_nodes.push_back(n);
  Confidence: band=high; score=0.74
- Line 318: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result_nodes.push_back(n);
  Confidence: band=high; score=0.74
- Line 325: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result_edges.push_back(e);
  Confidence: band=high; score=0.74
- Line 357: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, int> node_index;
  Confidence: band=medium; score=0.66
- Line 369: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: out_neighbors[fi->second].push_back(ti->second);
  Confidence: band=high; score=0.74
- Line 369: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: out_neighbors[fi->second].push_back(ti->second);
  Confidence: band=high; score=0.74
- Line 422: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: scored.emplace_back(node_ids[i], r[i]);
  Confidence: band=high; score=0.74
- Line 484: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: ctx.active_nodes.push_back(tok.current_node);
  Confidence: band=high; score=0.74
- Line 487: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: ctx.visited_nodes.push_back(vn);
  Confidence: band=high; score=0.74
- Line 516: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> top_set;
  Confidence: band=medium; score=0.66
- Line 524: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: if (top_set.count(n.value("id", ""))) ppr_nodes.push_back(n);
  Confidence: band=high; score=0.74
- Line 524: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: if (top_set.count(n.value("id", ""))) ppr_nodes.push_back(n);
  Confidence: band=high; score=0.74
- Line 529: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: ppr_edges.push_back(e);
  Confidence: band=high; score=0.74
- Line 563: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: ctx.attachments.push_back(att.toDocument());
  Confidence: band=high; score=0.74
- Line 563: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: ctx.attachments.push_back(att.toDocument());
  Confidence: band=high; score=0.74
- Line 563: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: ctx.attachments.push_back(att.toDocument());
  Confidence: band=high; score=0.74
- Line 573: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: ctx.missing_documents.push_back(std::move(m));
  Confidence: band=high; score=0.74
- Line 573: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: ctx.missing_documents.push_back(std::move(m));
  Confidence: band=high; score=0.74
- Line 594: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: ctx.similar_cases.push_back(std::move(sc_doc));
  Confidence: band=high; score=0.74
- Line 627: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: ctx.attachments.push_back(att.toDocument());
  Confidence: band=high; score=0.74
- Line 661: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: current_tasks.push_back(tok.current_node);
  Confidence: band=high; score=0.74
- Line 667: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> all_visited;
  Confidence: band=medium; score=0.66
- Line 691: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: for (auto& m : ms) missing.push_back(m);
  Confidence: band=high; score=0.74
- Line 701: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: for (const auto& v : comp.violations) viol.push_back(v);
  Confidence: band=high; score=0.74
- Line 746: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.violations.push_back("Instance not found: " + std::string(instance_id));
  Confidence: band=high; score=0.74
- Line 762: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.violations.push_back(
  Confidence: band=high; score=0.74
- Line 762: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.violations.push_back(
  Confidence: band=high; score=0.74
- Line 821: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.violations.push_back(
  Confidence: band=high; score=0.74
- Line 871: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: ref_embedding.push_back(v.get<float>());
  Confidence: band=high; score=0.74
- Line 881: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> ref_var_keys;
  Confidence: band=medium; score=0.66
- Line 926: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: candidates.push_back(std::move(sc));
  Confidence: band=high; score=0.74
- Line 948: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> other_var_keys;
  Confidence: band=medium; score=0.66
- Line 969: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: candidates.push_back(std::move(sc));
  Confidence: band=high; score=0.74
- Line 969: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: candidates.push_back(std::move(sc));
  Confidence: band=high; score=0.74

### src/process/dmn_evaluator.cpp
Total findings: 17

- Line 208: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: // Support both "inputs" (array) and "input_expressions" (array)
  Confidence: band=very_high; score=0.9
- Line 209: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: const auto& inputs_key =
  Confidence: band=very_high; score=0.9
- Line 210: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: r.contains("inputs") ? "inputs" : "input_expressions";
  Confidence: band=very_high; score=0.9
- Line 211: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: if (r.contains(inputs_key) && r[inputs_key].is_array()) {
  Confidence: band=very_high; score=0.9
- Line 212: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: for (const auto& e : r[inputs_key]) {
  Confidence: band=very_high; score=0.9
- Line 217: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: // Support both "outputs" (object) and "output_values" (object)
  Confidence: band=very_high; score=0.9
- Line 218: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: const auto& outputs_key =
  Confidence: band=very_high; score=0.9
- Line 219: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: r.contains("outputs") ? "outputs" : "output_values";
  Confidence: band=very_high; score=0.9
- Line 220: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: if (r.contains(outputs_key) && r[outputs_key].is_object()) {
  Confidence: band=very_high; score=0.9
- Line 221: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: rule.output_values = r[outputs_key];
  Confidence: band=very_high; score=0.9
- Line 458: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: // UNIQUE and FIRST: return first matching rule's outputs
  Confidence: band=very_high; score=0.9
- Line 198: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: for (const auto& c : dmn_json["output_columns"]) dt.output_columns.push_back(c);
  Confidence: band=high; score=0.74
- Line 212: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: rule.input_expressions.push_back(e.is_string() ? e.get<std::string>() : e.dump());
  Confidence: band=high; score=0.74
- Line 212: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: rule.input_expressions.push_back(e.is_string() ? e.get<std::string>() : e.dump());
  Confidence: band=high; score=0.74
- Line 212: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: rule.input_expressions.push_back(e.is_string() ? e.get<std::string>() : e.dump());
  Confidence: band=high; score=0.74
- Line 305: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: if (attrs.count("label")) dt.input_columns.push_back(attrs["label"]);
  Confidence: band=high; score=0.74
- Line 451: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results.push_back(rule.output_values);
  Confidence: band=high; score=0.74

### src/process/ocel_exporter.cpp
Total findings: 17

- Line 118: severity=HIGH; category=performance; pattern=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Context: auto vt_it = tok.visit_timestamps.find(nid);
  Confidence: band=very_high; score=0.9
- Line 80: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: attrs.push_back({{"name", "attached_by"}, {"time", toIso8601_(0)},
  Confidence: band=high; score=0.74
- Line 85: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: attrs.push_back({{"name", k},
  Confidence: band=high; score=0.74
- Line 123: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: entries.push_back(EventEntry{.node_id = nid, .timestamp_ms = ts});
  Confidence: band=high; score=0.74
- Line 123: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: entries.push_back(EventEntry{.node_id = nid, .timestamp_ms = ts});
  Confidence: band=high; score=0.74
- Line 136: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: entries.push_back(EventEntry{.node_id = tok.current_node, .timestamp_ms = ts});
  Confidence: band=high; score=0.74
- Line 142: severity=MEDIUM; category=determinism; pattern=timestamp_sorting_unstable
  Description: Timestamp-based sorting without stable_sort (non-deterministic ties)
  Context: // Sort by timestamp
  Confidence: band=high; score=0.74
- Line 149: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, std::string> node_names;
  Confidence: band=medium; score=0.66
- Line 171: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: attrs.push_back({{"name", "node_id"}, {"value", entry.node_id}});
  Confidence: band=high; score=0.74
- Line 171: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: attrs.push_back({{"name", "node_id"}, {"value", entry.node_id}});
  Confidence: band=high; score=0.74
- Line 181: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: rels.push_back(rel);
  Confidence: band=high; score=0.74
- Line 196: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> seen;
  Confidence: band=medium; score=0.66
- Line 200: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: types.push_back({{"name", t}, {"attributes", json::array()}});
  Confidence: band=high; score=0.74
- Line 212: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> seen;
  Confidence: band=medium; score=0.66
- Line 216: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: types.push_back({{"name", t}, {"attributes", json::array()}});
  Confidence: band=high; score=0.74
- Line 293: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: all_objects.push_back(o);
  Confidence: band=high; score=0.74
- Line 296: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: all_events.push_back(e);
  Confidence: band=high; score=0.74

### src/process/vcc_vpb_importer.cpp
Total findings: 16

- Line 220: severity=HIGH; category=performance; pattern=regex_in_loop
  Description: std::regex compiled in loop (compile once, reuse)
  Context: for (const auto& l : lines) {
  Confidence: band=very_high; score=0.9
- Line 227: severity=HIGH; category=performance; pattern=regex_in_loop
  Description: std::regex compiled in loop (compile once, reuse)
  Context: for (; it != end; ++it) {
  Confidence: band=very_high; score=0.9
- Line 239: severity=HIGH; category=performance; pattern=regex_in_loop
  Description: std::regex compiled in loop (compile once, reuse)
  Context: for (const auto& l : lines) {
  Confidence: band=very_high; score=0.9
- Line 265: severity=HIGH; category=performance; pattern=regex_in_loop
  Description: std::regex compiled in loop (compile once, reuse)
  Context: for (size_t i = 0; i < lines.size(); ++i) {
  Confidence: band=very_high; score=0.9
- Line 334: severity=HIGH; category=performance; pattern=regex_in_loop
  Description: std::regex compiled in loop (compile once, reuse)
  Context: for (const auto& l : lines) {
  Confidence: band=very_high; score=0.9
- Line 228: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: if (!val.empty()) comp_list.push_back(val);
  Confidence: band=high; score=0.74
- Line 228: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: if (!val.empty()) comp_list.push_back(val);
  Confidence: band=high; score=0.74
- Line 248: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: comp_list.push_back(trimStr(m[1].str()));
  Confidence: band=high; score=0.74
- Line 279: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: activities.push_back(current_activity);
  Confidence: band=high; score=0.74
- Line 346: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: edges.push_back(current_edge);
  Confidence: band=high; score=0.74
- Line 435: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: if (tag.is_string()) rec.compliance_tags.push_back(tag.get<std::string>());
  Confidence: band=high; score=0.74
- Line 503: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: edges.push_back(std::move(edge));
  Confidence: band=high; score=0.74
- Line 559: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: jedges.push_back(std::move(je));
  Confidence: band=high; score=0.74
- Line 660: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results.push_back(importYaml(chunk, meta_defaults));
  Confidence: band=high; score=0.74
- Line 685: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results.push_back({false, "Cannot open: " + entry.path().string(), {}});
  Confidence: band=high; score=0.74
- Line 697: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results.push_back(importYaml(content, meta_defaults));
  Confidence: band=high; score=0.74

### src/process/object_centric_tracer.cpp
Total findings: 13

- Line 50: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> object_types_set;
  Confidence: band=medium; score=0.66
- Line 60: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: for (const auto& ot : object_types_set) obj_types_arr.push_back(ot);
  Confidence: band=high; score=0.74
- Line 80: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: events_arr.push_back(std::move(ev));
  Confidence: band=high; score=0.74
- Line 80: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: events_arr.push_back(std::move(ev));
  Confidence: band=high; score=0.74
- Line 124: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> node_set;
  Confidence: band=medium; score=0.66
- Line 134: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, std::unordered_map<std::string, int>> freq;
  Confidence: band=medium; score=0.66
- Line 174: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: arcs_arr.push_back({{"from", from}, {"to", to}, {"frequency", cnt}});
  Confidence: band=high; score=0.74
- Line 174: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: arcs_arr.push_back({{"from", from}, {"to", to}, {"frequency", cnt}});
  Confidence: band=high; score=0.74
- Line 174: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: arcs_arr.push_back({{"from", from}, {"to", to}, {"frequency", cnt}});
  Confidence: band=high; score=0.74
- Line 204: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, std::unordered_map<std::string, int>> in_deg;
  Confidence: band=medium; score=0.66
- Line 205: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, std::unordered_map<std::string, int>> out_deg;
  Confidence: band=medium; score=0.66
- Line 224: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> conv_set;
  Confidence: band=medium; score=0.66
- Line 225: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> div_set;
  Confidence: band=medium; score=0.66

### src/process/process_community_detector.cpp
Total findings: 11

- Line 281: severity=HIGH; category=performance; pattern=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Context: auto it = g.adj[u].find(v);
  Confidence: band=very_high; score=0.9
- Line 295: severity=HIGH; category=performance; pattern=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Context: auto nit = node_names.find(pc.node_ids[i]);
  Confidence: band=very_high; score=0.9
- Line 44: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, int> node_index;  // node_id → index
  Confidence: band=medium; score=0.66
- Line 45: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::vector<std::unordered_map<int, float>> adj;  // adjacency list (weighted)
  Confidence: band=medium; score=0.66
- Line 60: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: g.node_ids.push_back(id);
  Confidence: band=high; score=0.74
- Line 130: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<int, std::unordered_set<int>> comm_nodes;
  Confidence: band=medium; score=0.66
- Line 139: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<int> visited_comms;
  Confidence: band=medium; score=0.66
- Line 191: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, std::string> node_names;
  Confidence: band=medium; score=0.66
- Line 212: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<int, int> label_remap;
  Confidence: band=medium; score=0.66
- Line 255: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<int, std::vector<int>> comm_map;
  Confidence: band=medium; score=0.66
- Line 302: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: communities.push_back(std::move(pc));
  Confidence: band=high; score=0.74

### src/process/epk_serializer.cpp
Total findings: 10

- Line 141: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.nodes.push_back(node);
  Confidence: band=high; score=0.74
- Line 196: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.nodes.push_back(std::move(n));
  Confidence: band=high; score=0.74
- Line 207: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.edges.push_back(std::move(e));
  Confidence: band=high; score=0.74
- Line 230: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, std::vector<std::string>> adj;
  Confidence: band=medium; score=0.66
- Line 231: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: adj[e.from_node].push_back(e.to_node);
  Confidence: band=high; score=0.74
- Line 236: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, const ProcessNodeInfo*> node_map;
  Confidence: band=medium; score=0.66
- Line 242: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> has_incoming;
  Confidence: band=medium; score=0.66
- Line 248: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> visited;
  Confidence: band=medium; score=0.66
- Line 320: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: jnodes.push_back(std::move(jn));
  Confidence: band=high; score=0.74
- Line 331: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: jedges.push_back(std::move(je));
  Confidence: band=high; score=0.74

### src/process/process_model_generator.cpp
Total findings: 10

- Line 219: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> all_node_ids;
  Confidence: band=medium; score=0.66
- Line 220: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, std::string> node_types; // id → type
  Confidence: band=medium; score=0.66
- Line 221: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, int>         out_degree;
  Confidence: band=medium; score=0.66
- Line 222: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, int>         in_degree;
  Confidence: band=medium; score=0.66
- Line 246: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.errors.push_back("No startEvent node found");
  Confidence: band=high; score=0.74
- Line 256: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.errors.push_back("Isolated node: " + nid);
  Confidence: band=high; score=0.74
- Line 303: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: norm_nodes.push_back(node);
  Confidence: band=high; score=0.74
- Line 315: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: norm_nodes.push_back(node);
  Confidence: band=high; score=0.74
- Line 326: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: norm_nodes.push_back(node);
  Confidence: band=high; score=0.74
- Line 338: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: norm_edges.push_back(edge);
  Confidence: band=high; score=0.74

### src/process/process_model_manager.cpp
Total findings: 9

- Line 194: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: r.compliance_tags.push_back(tag.get<std::string>());
  Confidence: band=high; score=0.74
- Line 206: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: r.embedding.push_back(v.get<float>());
  Confidence: band=high; score=0.74
- Line 354: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: jedges.push_back(std::move(je));
  Confidence: band=high; score=0.74
- Line 601: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results.push_back(std::move(*rec));
  Confidence: band=high; score=0.74
- Line 658: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: candidates.emplace_back(std::move(*rec), sim);
  Confidence: band=high; score=0.74
- Line 686: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: candidates.emplace_back(std::move(r), sim);
  Confidence: band=high; score=0.74
- Line 726: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: nodes.push_back(n);
  Confidence: band=high; score=0.74
- Line 736: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: edges.push_back(e);
  Confidence: band=high; score=0.74
- Line 749: severity=MEDIUM; category=llm_ai_safety; pattern=missing_resource_limits
  Description: LLM inference without token limit or timeout (DOS risk)
  Context: return LlmProcessDescriptor::generate(*record);
  Confidence: band=high; score=0.74

### src/process/process_agentic_rag.cpp
Total findings: 8

- Line 82: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: docs.push_back(std::move(d));
  Confidence: band=high; score=0.74
- Line 94: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: docs.push_back(std::move(d));
  Confidence: band=high; score=0.74
- Line 109: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: docs.push_back(std::move(d));
  Confidence: band=high; score=0.74
- Line 139: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: ctx.attachments.push_back(nlohmann::json::parse(doc.content));
  Confidence: band=high; score=0.74
- Line 139: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: ctx.attachments.push_back(nlohmann::json::parse(doc.content));
  Confidence: band=high; score=0.74
- Line 199: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: if (!already_seen) fresh.push_back(std::move(d));
  Confidence: band=high; score=0.74
- Line 199: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: if (!already_seen) fresh.push_back(std::move(d));
  Confidence: band=high; score=0.74
- Line 231: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.iteration_history.push_back(std::move(s));
  Confidence: band=high; score=0.74

### src/process/fim_importer.cpp
Total findings: 7

- Line 58: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: if      (ent == "&amp;")  out += '&';
  Confidence: band=high; score=0.74
- Line 342: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results.push_back(std::move(r));
  Confidence: band=high; score=0.74
- Line 373: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: nodes_arr.push_back(std::move(jn));
  Confidence: band=high; score=0.74
- Line 381: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: edges_arr.push_back(std::move(je));
  Confidence: band=high; score=0.74
- Line 426: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: nodes_arr.push_back(std::move(jn));
  Confidence: band=high; score=0.74
- Line 434: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: edges_arr.push_back(std::move(je));
  Confidence: band=high; score=0.74
- Line 506: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results.push_back(importSingleModel(bpmn_xml, domain));
  Confidence: band=high; score=0.74

### src/process/llm_process_descriptor.cpp
Total findings: 6

- Line 71: severity=MEDIUM; category=llm_ai_safety; pattern=missing_resource_limits
  Description: LLM inference without token limit or timeout (DOS risk)
  Context: json LlmProcessDescriptor::generate(const ProcessModelRecord& record)
  Confidence: band=high; score=0.74
- Line 73: severity=MEDIUM; category=llm_ai_safety; pattern=missing_resource_limits
  Description: LLM inference without token limit or timeout (DOS risk)
  Context: return generate(record, Config{});
  Confidence: band=high; score=0.74
- Line 76: severity=MEDIUM; category=llm_ai_safety; pattern=missing_resource_limits
  Description: LLM inference without token limit or timeout (DOS risk)
  Context: json LlmProcessDescriptor::generate(
  Confidence: band=high; score=0.74
- Line 104: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: nodes_array.push_back(nodeToJson_(jn, cfg));
  Confidence: band=high; score=0.74
- Line 106: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: total_sla_ms += static_cast<size_t>(jn["timeout_ms"].get<double>());
  Confidence: band=high; score=0.74
- Line 116: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: edges_array.push_back(edgeToJson_(je));
  Confidence: band=high; score=0.74

### src/process/bpmn_serializer.cpp
Total findings: 5

- Line 59: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: if      (ent == "&amp;")  out += '&';
  Confidence: band=high; score=0.74
- Line 259: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: case '&':  out += "&amp;";  break;
  Confidence: band=high; score=0.74
- Line 584: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.nodes.push_back(std::move(node));
  Confidence: band=high; score=0.74
- Line 789: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: nodes.push_back(std::move(n));
  Confidence: band=high; score=0.74
- Line 802: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: edges.push_back(std::move(e));
  Confidence: band=high; score=0.74

### src/process/epk_aris_xml_importer.cpp
Total findings: 4

- Line 47: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: if      (ent == "&amp;")  out += '&';
  Confidence: band=high; score=0.74
- Line 451: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: const std::unordered_map<std::string, ObjDefInfo>& obj_defs)
  Confidence: band=medium; score=0.66
- Line 499: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: res.edges.push_back(std::move(edge));
  Confidence: band=high; score=0.74
- Line 593: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results.push_back(buildImportResult(m, parsed.obj_defs));
  Confidence: band=high; score=0.74

### src/process/process_light_retriever.cpp
Total findings: 4

- Line 47: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(static_cast<char>(std::tolower(c)));
  Confidence: band=high; score=0.74
- Line 68: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: RetrievalMode ProcessLightRetriever::classifyQuery(std::string_view query) const {
  Confidence: band=high; score=0.74
- Line 86: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: ? classifyQuery(query)
  Confidence: band=high; score=0.74
- Line 119: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: used_ids.push_back(communities[i].community_id);
  Confidence: band=high; score=0.74

### src/process/process_linker.cpp
Total findings: 4

- Line 296: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(std::move(att));
  Confidence: band=high; score=0.74
- Line 472: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> present_types;
  Confidence: band=medium; score=0.66
- Line 485: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: missing.push_back(dtype);
  Confidence: band=high; score=0.74
- Line 485: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: missing.push_back(dtype);
  Confidence: band=high; score=0.74

### src/process/cmmn_serializer.cpp
Total findings: 3

- Line 53: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: if      (ent == "&amp;")  out += '&';
  Confidence: band=high; score=0.74
- Line 223: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: case '&':  out += "&amp;";  break;
  Confidence: band=high; score=0.74
- Line 404: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.edges.push_back(std::move(edge));
  Confidence: band=high; score=0.74

## Update Workflow

- Refresh scan artifacts with: python tools/gap_scanner_v3.py
- Regenerate all module notes with: python tools/module_doc_generator.py . ai_working ai_working/module_gaps
- The generator mirrors each archive document directly into src/<module>/MODULE_GAPS.md.

Format: THEMIS_MODULE_GAPS_V3
