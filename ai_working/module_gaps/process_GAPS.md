# process Module - Developer Gap Note

> Auto-generated from ai_working/gap_scan_v3_aggregate.json.
> This file is overwritten on each regeneration.

## Scan Snapshot

- Module: process
- Generated: 2026-05-31 08:50:11
- Status: Critical Findings Present
- Total Findings: 480
- Actionable Findings (Critical + High): 93
- Affected Files: 17

## Severity Summary

| Severity | Count |
|---|---:|
| Critical | 7 |
| High | 86 |
| Medium | 387 |
| Low | 0 |

## Category Summary

| Category | Count |
|---|---:|
| container | 160 |
| performance_patterns | 136 |
| determinism | 38 |
| performance | 31 |
| platform | 27 |
| memory | 23 |
| reliability | 21 |
| security | 16 |
| audit_logging | 11 |
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
| src/process/process_graph_rag.cpp | 106 | 3 | 18 | 85 | 0 |
| src/process/bpmn_serializer.cpp | 39 | 0 | 4 | 35 | 0 |
| src/process/vcc_vpb_importer.cpp | 37 | 0 | 9 | 28 | 0 |
| src/process/ocel_exporter.cpp | 33 | 0 | 4 | 29 | 0 |
| src/process/dmn_evaluator.cpp | 31 | 2 | 11 | 18 | 0 |
| src/process/cmmn_serializer.cpp | 29 | 1 | 2 | 26 | 0 |
| src/process/process_agentic_rag.cpp | 28 | 0 | 12 | 16 | 0 |
| src/process/fim_importer.cpp | 27 | 0 | 2 | 25 | 0 |
| src/process/process_model_manager.cpp | 23 | 0 | 3 | 20 | 0 |
| src/process/object_centric_tracer.cpp | 21 | 0 | 4 | 17 | 0 |
| src/process/process_model_generator.cpp | 20 | 0 | 0 | 20 | 0 |
| src/process/epk_aris_xml_importer.cpp | 18 | 0 | 3 | 15 | 0 |
| src/process/epk_serializer.cpp | 18 | 0 | 1 | 17 | 0 |
| src/process/process_community_detector.cpp | 18 | 1 | 4 | 13 | 0 |
| src/process/process_linker.cpp | 14 | 0 | 7 | 7 | 0 |
| src/process/llm_process_descriptor.cpp | 11 | 0 | 2 | 9 | 0 |
| src/process/process_light_retriever.cpp | 7 | 0 | 0 | 7 | 0 |

## Full Scanner Findings

### src/process/process_graph_rag.cpp
Total findings: 106

- Line 369: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator fi may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto fi = node_index.find(from);
- Line 370: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator ti may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto ti = node_index.find(to);
- Line 1278: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto it = sla_rules_.find(inst_id);
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 368: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: auto fi = node_index.find(from);
- Line 368: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: auto fi = node_index.find(from);
- Line 368: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: auto fi = node_index.find(from);
- Line 369: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: auto ti = node_index.find(to);
- Line 369: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: auto ti = node_index.find(to);
- Line 369: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: auto ti = node_index.find(to);
- Line 369: severity=HIGH; category=performance; pattern=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Context: auto fi = node_index.find(from);
  Confidence: band=very_high; score=0.9
- Line 370: severity=HIGH; category=performance; pattern=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Context: auto ti = node_index.find(to);
  Confidence: band=very_high; score=0.9
- Line 379: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: auto it = node_index.find(seed);
- Line 379: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: auto it = node_index.find(seed);
- Line 379: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: auto it = node_index.find(seed);
- Line 380: severity=HIGH; category=performance; pattern=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Context: auto it = node_index.find(seed);
  Confidence: band=very_high; score=0.9
- Line 894: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: db_.scanPrefix(emb_prefix, [&](std::string_view key, std::string_view value) -> bool {
- Line 937: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: db_.scanPrefix(inst_prefix, [&](std::string_view key, std::string_view value) -> bool {
- Line 61: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: float jaccardSimilarity(const std::unordered_set<std::string>& a,
  Confidence: band=medium; score=0.66
- Line 62: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: const std::unordered_set<std::string>& b) {
  Confidence: band=medium; score=0.66
- Line 136: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: kgn.aliases.push_back(n["name_en"].get<std::string>());
  Confidence: band=high; score=0.74
- Line 137: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: kgn.aliases.push_back(n["name_en"].get<std::string>());
- Line 140: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: kgn.aliases.push_back(n["subtype"].get<std::string>());
- Line 149: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: kg.nodes.push_back(std::move(kgn));
- Line 172: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: kg.edges.push_back(std::move(kge));
  Confidence: band=high; score=0.74
- Line 173: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: kg.edges.push_back(std::move(kge));
- Line 218: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: kg.nodes.push_back(std::move(tok_node));
  Confidence: band=high; score=0.74
- Line 219: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: kg.nodes.push_back(std::move(tok_node));
- Line 226: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: kg.edges.push_back(std::move(inst_edge));
- Line 248: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: kg.nodes.push_back(std::move(att_node));
  Confidence: band=high; score=0.74
- Line 249: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: kg.nodes.push_back(std::move(att_node));
- Line 256: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: kg.edges.push_back(std::move(att_edge));
- Line 281: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, std::vector<std::pair<std::string, json>>> adj;
  Confidence: band=medium; score=0.66
- Line 285: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: adj[from].push_back({to, e});
  Confidence: band=high; score=0.74
- Line 286: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: adj[from].push_back({to, e});
- Line 287: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: adj[to].push_back({from, e}); // undirected BFS for context
- Line 292: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> visited_nodes;
  Confidence: band=medium; score=0.66
- Line 293: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> visited_edges;
  Confidence: band=medium; score=0.66
- Line 320: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result_nodes.push_back(n);
  Confidence: band=high; score=0.74
- Line 320: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result_nodes.push_back(n);
  Confidence: band=high; score=0.74
- Line 321: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result_nodes.push_back(n);
- Line 327: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result_edges.push_back(e);
  Confidence: band=high; score=0.74
- Line 328: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result_edges.push_back(e);
- Line 354: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: if (!nid.empty()) node_ids.push_back(nid);
- Line 359: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, int> node_index;
  Confidence: band=medium; score=0.66
- Line 371: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: out_neighbors[fi->second].push_back(ti->second);
  Confidence: band=high; score=0.74
- Line 371: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: out_neighbors[fi->second].push_back(ti->second);
  Confidence: band=high; score=0.74
- Line 372: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: out_neighbors[fi->second].push_back(ti->second);
- Line 374: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: out_neighbors[ti->second].push_back(fi->second);
- Line 424: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: scored.emplace_back(node_ids[i], r[i]);
  Confidence: band=high; score=0.74
- Line 486: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: ctx.active_nodes.push_back(tok.current_node);
  Confidence: band=high; score=0.74
- Line 487: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: ctx.active_nodes.push_back(tok.current_node);
- Line 489: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: ctx.visited_nodes.push_back(vn);
  Confidence: band=high; score=0.74
- Line 490: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: ctx.visited_nodes.push_back(vn);
- Line 518: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> top_set;
  Confidence: band=medium; score=0.66
- Line 526: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: if (top_set.count(n.value("id", ""))) ppr_nodes.push_back(n);
  Confidence: band=high; score=0.74
- Line 526: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: if (top_set.count(n.value("id", ""))) ppr_nodes.push_back(n);
  Confidence: band=high; score=0.74
- Line 527: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: if (top_set.count(n.value("id", ""))) ppr_nodes.push_back(n);
- Line 531: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: ppr_edges.push_back(e);
  Confidence: band=high; score=0.74
- Line 532: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: ppr_edges.push_back(e);
- Line 565: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: ctx.attachments.push_back(att.toDocument());
  Confidence: band=high; score=0.74
- Line 565: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: ctx.attachments.push_back(att.toDocument());
  Confidence: band=high; score=0.74
- Line 565: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: ctx.attachments.push_back(att.toDocument());
  Confidence: band=high; score=0.74
- Line 566: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: ctx.attachments.push_back(att.toDocument());
- Line 575: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: ctx.missing_documents.push_back(std::move(m));
  Confidence: band=high; score=0.74
- Line 575: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: ctx.missing_documents.push_back(std::move(m));
  Confidence: band=high; score=0.74
- Line 576: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: ctx.missing_documents.push_back(std::move(m));
- Line 596: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: ctx.similar_cases.push_back(std::move(sc_doc));
  Confidence: band=high; score=0.74
- Line 597: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: ctx.similar_cases.push_back(std::move(sc_doc));
- Line 629: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: ctx.attachments.push_back(att.toDocument());
  Confidence: band=high; score=0.74
- Line 630: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: ctx.attachments.push_back(att.toDocument());
- Line 663: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: current_tasks.push_back(tok.current_node);
  Confidence: band=high; score=0.74
- Line 664: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: current_tasks.push_back(tok.current_node);
- Line 669: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> all_visited;
  Confidence: band=medium; score=0.66
- Line 693: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: for (auto& m : ms) missing.push_back(m);
  Confidence: band=high; score=0.74
- Line 694: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: for (auto& m : ms) missing.push_back(m);
- Line 703: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: for (const auto& v : comp.violations) viol.push_back(v);
  Confidence: band=high; score=0.74
- Line 704: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: for (const auto& v : comp.violations) viol.push_back(v);
- Line 729: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: for (const auto& t : compliance_tags) ctags.push_back(t);
- Line 748: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.violations.push_back("Instance not found: " + std::string(instance_id));
  Confidence: band=high; score=0.74
- Line 764: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.violations.push_back(
  Confidence: band=high; score=0.74
- Line 764: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.violations.push_back(
  Confidence: band=high; score=0.74
- Line 765: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.violations.push_back(
- Line 823: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.violations.push_back(
  Confidence: band=high; score=0.74
- Line 824: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.violations.push_back(
- Line 873: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: ref_embedding.push_back(v.get<float>());
  Confidence: band=high; score=0.74
- Line 874: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: ref_embedding.push_back(v.get<float>());
- Line 877: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 883: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> ref_var_keys;
  Confidence: band=medium; score=0.66
- Line 906: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: for (const auto& v : emb_json) other_emb.push_back(v.get<float>());
- Line 909: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {}
- Line 928: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: candidates.push_back(std::move(sc));
  Confidence: band=high; score=0.74
- Line 929: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: candidates.push_back(std::move(sc));
- Line 950: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> other_var_keys;
  Confidence: band=medium; score=0.66
- Line 971: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: candidates.push_back(std::move(sc));
  Confidence: band=high; score=0.74
- Line 971: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: candidates.push_back(std::move(sc));
  Confidence: band=high; score=0.74
- Line 972: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: candidates.push_back(std::move(sc));
- Line 974: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {}
- Line 1225: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: rc.actions.push_back(std::move(ac));
- Line 1251: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: rc.actions.push_back(std::move(ac));
- Line 1298: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: SlaAlert alert{instance_id, process_name, sla_ms, elapsed_ms, status};
- Line 1329: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: catch (...) { agg = json::object(); }
- Line 1370: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: catch (...) { return true; }

### src/process/bpmn_serializer.cpp
Total findings: 39

- Line 22: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: * are parsed on import and stored in ProcessNodeInfo::metadata["layout"].
- Line 437: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: shape_elem_ref = it->second;
- Line 613: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: node.metadata["layout"] = std::move(layout);
- Line 729: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: << " dataCategory=\"" << escapeXml_(n.dsgvo_annotation->data_category) << "\""
- Line 61: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: if      (ent == "&amp;")  out += '&';
  Confidence: band=high; score=0.74
- Line 62: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: if      (ent == "&amp;")  out += '&';
- Line 63: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: else if (ent == "<")   out += '<';
- Line 64: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: else if (ent == ">")   out += '>';
- Line 65: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: else if (ent == "&quot;") out += '"';
- Line 66: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: else if (ent == "&apos;") out += '\'';
- Line 261: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: case '&':  out += "&amp;";  break;
  Confidence: band=high; score=0.74
- Line 262: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: case '&':  out += "&amp;";  break;
- Line 263: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: case '<':  out += "<";   break;
- Line 264: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: case '>':  out += ">";   break;
- Line 265: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: case '"':  out += "&quot;"; break;
- Line 266: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: case '\'': out += "&apos;"; break;
- Line 449: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: try { return std::stof(it->second); } catch (...) { return 0.f; }
- Line 485: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: try { ann.retention_days = std::stoi(rd); } catch (...) {}
- Line 586: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.nodes.push_back(std::move(node));
  Confidence: band=high; score=0.74
- Line 587: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.nodes.push_back(std::move(node));
- Line 688: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: xml << R"(<definitions xmlns="http://www.omg.org/spec/BPMN/20100524/MODEL")" << "\n";
- Line 688: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: xml << R"(<definitions xmlns="http://www.omg.org/spec/BPMN/20100524/MODEL")" << "\n";
- Line 689: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: xml << R"(             xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance")" << "\n";
- Line 689: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: xml << R"(             xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance")" << "\n";
- Line 690: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: xml << R"(             targetNamespace="http://themis.db/process")" << "\n";
- Line 690: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: xml << R"(             targetNamespace="http://themis.db/process")" << "\n";
- Line 728: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: << " xmlns:bpmns=\"http://bpmn-s.org/schema\""
- Line 735: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: xml << "/>\n";
- Line 736: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: xml << "      </extensionElements>\n";
- Line 737: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: xml << "    </" << tag << ">\n";
- Line 737: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: xml << "    </" << tag << ">\n";
- Line 739: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: xml << "/>\n";
- Line 761: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: xml << "/>\n";
- Line 764: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: xml << "  </process>\n";
- Line 765: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: xml << "</definitions>\n";
- Line 791: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: nodes.push_back(std::move(n));
  Confidence: band=high; score=0.74
- Line 792: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: nodes.push_back(std::move(n));
- Line 804: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: edges.push_back(std::move(e));
  Confidence: band=high; score=0.74
- Line 805: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: edges.push_back(std::move(e));

### src/process/vcc_vpb_importer.cpp
Total findings: 37

- Line 0: severity=HIGH; category=uncategorized
  Context: ['    while (std::getline(ss, line)) {', '        // Check for start of new model (line starts with 2 spaces + "- ")', "        bool is_new_model = (line.size() >= 4 && line[0] == ' ' && line[1] == ' '", "                             && line[2] == '-' && line[3] == ' ');", '        // Also detect "  -\\n" (just the dash, id on next line)']
  Confidence: band=high; score=0.78
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 222: severity=HIGH; category=performance; pattern=regex_in_loop
  Description: std::regex compiled in loop (compile once, reuse)
  Context: for (const auto& l : lines) {
  Confidence: band=very_high; score=0.9
- Line 229: severity=HIGH; category=performance; pattern=regex_in_loop
  Description: std::regex compiled in loop (compile once, reuse)
  Context: for (; it != end; ++it) {
  Confidence: band=very_high; score=0.9
- Line 241: severity=HIGH; category=performance; pattern=regex_in_loop
  Description: std::regex compiled in loop (compile once, reuse)
  Context: for (const auto& l : lines) {
  Confidence: band=very_high; score=0.9
- Line 267: severity=HIGH; category=performance; pattern=regex_in_loop
  Description: std::regex compiled in loop (compile once, reuse)
  Context: for (size_t i = 0; i < lines.size(); ++i) {
  Confidence: band=very_high; score=0.9
- Line 336: severity=HIGH; category=performance; pattern=regex_in_loop
  Description: std::regex compiled in loop (compile once, reuse)
  Context: for (const auto& l : lines) {
  Confidence: band=very_high; score=0.9
- Line 625: severity=HIGH; category=allocation_loop
  Description: Dynamic allocation in loop — high overhead
  Remediation: Pre-allocate outside loop or use object pool pattern
  Context: // Check for start of new model (line starts with 2 spaces + "- ")
- Line 0: severity=MEDIUM; category=uncategorized
  Context: Struct with uninitialized fields
  Confidence: band=medium; score=0.65
- Line 157: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: lines.push_back(line);
- Line 173: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: stack.push_back({&result, -1, false, ""});
- Line 230: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: if (!val.empty()) comp_list.push_back(val);
  Confidence: band=high; score=0.74
- Line 230: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: if (!val.empty()) comp_list.push_back(val);
  Confidence: band=high; score=0.74
- Line 231: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: if (!val.empty()) comp_list.push_back(val);
- Line 250: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: comp_list.push_back(trimStr(m[1].str()));
  Confidence: band=high; score=0.74
- Line 251: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: comp_list.push_back(trimStr(m[1].str()));
- Line 281: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: activities.push_back(current_activity);
  Confidence: band=high; score=0.74
- Line 282: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: activities.push_back(current_activity);
- Line 348: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: edges.push_back(current_edge);
  Confidence: band=high; score=0.74
- Line 349: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: edges.push_back(current_edge);
- Line 437: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: if (tag.is_string()) rec.compliance_tags.push_back(tag.get<std::string>());
  Confidence: band=high; score=0.74
- Line 438: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: if (tag.is_string()) rec.compliance_tags.push_back(tag.get<std::string>());
- Line 473: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: catch (...) {}
- Line 505: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: edges.push_back(std::move(edge));
  Confidence: band=high; score=0.74
- Line 506: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: edges.push_back(std::move(edge));
- Line 561: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: jedges.push_back(std::move(je));
  Confidence: band=high; score=0.74
- Line 562: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: jedges.push_back(std::move(je));
- Line 633: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: model_chunks.push_back(current_chunk);
- Line 662: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results.push_back(importYaml(chunk, meta_defaults));
  Confidence: band=high; score=0.74
- Line 663: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: results.push_back(importYaml(chunk, meta_defaults));
- Line 687: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results.push_back({false, "Cannot open: " + entry.path().string(), {}});
  Confidence: band=high; score=0.74
- Line 688: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: results.push_back({false, "Cannot open: " + entry.path().string(), {}});
- Line 698: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: for (auto& r : list_results) results.push_back(std::move(r));
- Line 699: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results.push_back(importYaml(content, meta_defaults));
  Confidence: band=high; score=0.74
- Line 700: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: results.push_back(importYaml(content, meta_defaults));
- Line 704: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: results.push_back({false, std::string("Directory scan error: ") + ex.what(), {}});

### src/process/ocel_exporter.cpp
Total findings: 33

- Line 119: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: auto vt_it = tok.visit_timestamps.find(nid);
- Line 119: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: auto vt_it = tok.visit_timestamps.find(nid);
- Line 120: severity=HIGH; category=performance; pattern=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Context: auto vt_it = tok.visit_timestamps.find(nid);
  Confidence: band=very_high; score=0.9
- Line 273: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: db_.scanPrefix(instance_prefix, [&](std::string_view key, std::string_view) -> bool {
- Line 82: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: attrs.push_back({{"name", "attached_by"}, {"time", toIso8601_(0)},
  Confidence: band=high; score=0.74
- Line 83: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: attrs.push_back({{"name", "attached_by"}, {"time", toIso8601_(0)},
- Line 87: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: attrs.push_back({{"name", k},
  Confidence: band=high; score=0.74
- Line 88: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: attrs.push_back({{"name", k},
- Line 94: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: objects.push_back(obj);
- Line 125: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: entries.push_back(EventEntry{.node_id = nid, .timestamp_ms = ts});
  Confidence: band=high; score=0.74
- Line 125: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: entries.push_back(EventEntry{.node_id = nid, .timestamp_ms = ts});
  Confidence: band=high; score=0.74
- Line 126: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: entries.push_back(EventEntry{.node_id = nid, .timestamp_ms = ts});
- Line 138: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: entries.push_back(EventEntry{.node_id = tok.current_node, .timestamp_ms = ts});
  Confidence: band=high; score=0.74
- Line 139: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: entries.push_back(EventEntry{.node_id = tok.current_node, .timestamp_ms = ts});
- Line 144: severity=MEDIUM; category=determinism; pattern=timestamp_sorting_unstable
  Description: Timestamp-based sorting without stable_sort (non-deterministic ties)
  Context: // Sort by timestamp
  Confidence: band=high; score=0.74
- Line 151: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, std::string> node_names;
  Confidence: band=medium; score=0.66
- Line 173: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: attrs.push_back({{"name", "node_id"}, {"value", entry.node_id}});
  Confidence: band=high; score=0.74
- Line 173: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: attrs.push_back({{"name", "node_id"}, {"value", entry.node_id}});
  Confidence: band=high; score=0.74
- Line 174: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: attrs.push_back({{"name", "node_id"}, {"value", entry.node_id}});
- Line 175: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: attrs.push_back({{"name", "process_instance"}, {"value", inst.instance_id}});
- Line 183: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: rels.push_back(rel);
  Confidence: band=high; score=0.74
- Line 184: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: rels.push_back(rel);
- Line 187: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: events.push_back(evt);
- Line 198: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> seen;
  Confidence: band=medium; score=0.66
- Line 202: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: types.push_back({{"name", t}, {"attributes", json::array()}});
  Confidence: band=high; score=0.74
- Line 203: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: types.push_back({{"name", t}, {"attributes", json::array()}});
- Line 214: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> seen;
  Confidence: band=medium; score=0.66
- Line 218: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: types.push_back({{"name", t}, {"attributes", json::array()}});
  Confidence: band=high; score=0.74
- Line 219: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: types.push_back({{"name", t}, {"attributes", json::array()}});
- Line 295: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: all_objects.push_back(o);
  Confidence: band=high; score=0.74
- Line 296: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: all_objects.push_back(o);
- Line 298: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: all_events.push_back(e);
  Confidence: band=high; score=0.74
- Line 299: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: all_events.push_back(e);

### src/process/dmn_evaluator.cpp
Total findings: 31

- Line 0: severity=CRITICAL; category=uncategorized
  Context: ['', '    // Security guard: 10 MiB', '    if (dmn_xml.size() > 10 * 1024 * 1024) {', '        SPDLOG_ERROR("[DmnEvaluator] DMN XML exceeds 10 MiB size limit");', '        return false;']
  Confidence: band=very_high; score=0.93
- Line 262: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto stripNs = [](std::string_view tag) -> std::string_view {
- Line 210: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: // Support both "inputs" (array) and "input_expressions" (array)
  Confidence: band=very_high; score=0.9
- Line 211: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: const auto& inputs_key =
  Confidence: band=very_high; score=0.9
- Line 212: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: r.contains("inputs") ? "inputs" : "input_expressions";
  Confidence: band=very_high; score=0.9
- Line 213: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: if (r.contains(inputs_key) && r[inputs_key].is_array()) {
  Confidence: band=very_high; score=0.9
- Line 214: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: for (const auto& e : r[inputs_key]) {
  Confidence: band=very_high; score=0.9
- Line 219: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: // Support both "outputs" (object) and "output_values" (object)
  Confidence: band=very_high; score=0.9
- Line 220: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: const auto& outputs_key =
  Confidence: band=very_high; score=0.9
- Line 221: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: r.contains("outputs") ? "outputs" : "output_values";
  Confidence: band=very_high; score=0.9
- Line 222: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: if (r.contains(outputs_key) && r[outputs_key].is_object()) {
  Confidence: band=very_high; score=0.9
- Line 223: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: rule.output_values = r[outputs_key];
  Confidence: band=very_high; score=0.9
- Line 460: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: // UNIQUE and FIRST: return first matching rule's outputs
  Confidence: band=very_high; score=0.9
- Line 55: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 198: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: for (const auto& c : dmn_json["input_columns"]) dt.input_columns.push_back(c);
- Line 200: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: for (const auto& c : dmn_json["output_columns"]) dt.output_columns.push_back(c);
  Confidence: band=high; score=0.74
- Line 201: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: for (const auto& c : dmn_json["output_columns"]) dt.output_columns.push_back(c);
- Line 214: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: rule.input_expressions.push_back(e.is_string() ? e.get<std::string>() : e.dump());
  Confidence: band=high; score=0.74
- Line 214: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: rule.input_expressions.push_back(e.is_string() ? e.get<std::string>() : e.dump());
  Confidence: band=high; score=0.74
- Line 214: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: rule.input_expressions.push_back(e.is_string() ? e.get<std::string>() : e.dump());
  Confidence: band=high; score=0.74
- Line 215: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: rule.input_expressions.push_back(e.is_string() ? e.get<std::string>() : e.dump());
- Line 225: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: dt.rules.push_back(std::move(rule));
- Line 307: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: if (attrs.count("label")) dt.input_columns.push_back(attrs["label"]);
  Confidence: band=high; score=0.74
- Line 308: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: if (attrs.count("label")) dt.input_columns.push_back(attrs["label"]);
- Line 309: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: else if (attrs.count("id")) dt.input_columns.push_back(attrs["id"]);
- Line 314: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: if (!output_col_name.empty()) dt.output_columns.push_back(output_col_name);
- Line 335: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: current_rule.input_expressions.push_back(expr);
- Line 347: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: dt.rules.push_back(std::move(current_rule));
- Line 453: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results.push_back(rule.output_values);
  Confidence: band=high; score=0.74
- Line 454: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: results.push_back(rule.output_values);
- Line 476: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: for (const auto& [id, _] : tables_) ids.push_back(id);

### src/process/cmmn_serializer.cpp
Total findings: 29

- Line 397: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto it = sentry_targets.find(sentry_id);
- Line 343: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: node.metadata["cmmn_case_id"] = result.case_id;
- Line 399: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: const std::string& target_ref = it->second;
- Line 55: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: if      (ent == "&amp;")  out += '&';
  Confidence: band=high; score=0.74
- Line 56: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: if      (ent == "&amp;")  out += '&';
- Line 57: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: else if (ent == "<")   out += '<';
- Line 58: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: else if (ent == ">")   out += '>';
- Line 59: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: else if (ent == "&quot;") out += '"';
- Line 60: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: else if (ent == "&apos;") out += '\'';
- Line 225: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: case '&':  out += "&amp;";  break;
  Confidence: band=high; score=0.74
- Line 226: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: case '&':  out += "&amp;";  break;
- Line 227: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: case '<':  out += "<";   break;
- Line 228: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: case '>':  out += ">";   break;
- Line 229: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: case '"':  out += "&quot;"; break;
- Line 230: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: case '\'': out += "&apos;"; break;
- Line 345: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.nodes.push_back(std::move(node));
- Line 406: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.edges.push_back(std::move(edge));
  Confidence: band=high; score=0.74
- Line 407: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.edges.push_back(std::move(edge));
- Line 457: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: xml << R"(<definitions xmlns="http://www.omg.org/spec/CMMN/20151109/MODEL")" << "\n";
- Line 457: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: xml << R"(<definitions xmlns="http://www.omg.org/spec/CMMN/20151109/MODEL")" << "\n";
- Line 458: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: xml << R"(             xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance")" << "\n";
- Line 458: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: xml << R"(             xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance")" << "\n";
- Line 459: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: xml << R"(             targetNamespace="http://themis.db/cmmn")" << "\n";
- Line 459: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: xml << R"(             targetNamespace="http://themis.db/cmmn")" << "\n";
- Line 507: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: << " name=\"" << escapeXml_(n.name) << "\"/>\n";
- Line 507: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: << " name=\"" << escapeXml_(n.name) << "\"/>\n";
- Line 510: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: xml << "    </casePlanModel>\n";
- Line 511: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: xml << "  </case>\n";
- Line 512: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: xml << "</definitions>\n";

### src/process/process_agentic_rag.cpp
Total findings: 28

- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 58: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: d.metadata["type"] = "llm_prompt";
- Line 59: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: d.metadata["instance_id"] = ctx.instance_id;
- Line 69: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: d.metadata["type"] = "subgraph";
- Line 70: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: d.metadata["instance_id"] = ctx.instance_id;
- Line 71: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: d.metadata["process_name"] = ctx.process_name;
- Line 83: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: d.metadata["type"] = "attachment";
- Line 84: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: d.metadata["instance_id"] = ctx.instance_id;
- Line 96: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: d.metadata["type"] = "similar_case";
- Line 110: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: d.metadata["type"] = "missing_documents";
- Line 111: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: d.metadata["instance_id"] = ctx.instance_id;
- Line 126: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: auto type_it = doc.metadata.find("type");
- Line 84: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: docs.push_back(std::move(d));
  Confidence: band=high; score=0.74
- Line 85: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: docs.push_back(std::move(d));
- Line 96: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: docs.push_back(std::move(d));
  Confidence: band=high; score=0.74
- Line 97: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: docs.push_back(std::move(d));
- Line 111: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: docs.push_back(std::move(d));
  Confidence: band=high; score=0.74
- Line 112: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: docs.push_back(std::move(d));
- Line 141: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: ctx.attachments.push_back(nlohmann::json::parse(doc.content));
  Confidence: band=high; score=0.74
- Line 141: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: ctx.attachments.push_back(nlohmann::json::parse(doc.content));
  Confidence: band=high; score=0.74
- Line 142: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: ctx.attachments.push_back(nlohmann::json::parse(doc.content));
- Line 143: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 144: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: ctx.attachments.push_back(nlohmann::json{{"_id", doc.id},
- Line 201: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: if (!already_seen) fresh.push_back(std::move(d));
  Confidence: band=high; score=0.74
- Line 201: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: if (!already_seen) fresh.push_back(std::move(d));
  Confidence: band=high; score=0.74
- Line 202: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: if (!already_seen) fresh.push_back(std::move(d));
- Line 233: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.iteration_history.push_back(std::move(s));
  Confidence: band=high; score=0.74
- Line 234: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.iteration_history.push_back(std::move(s));

### src/process/fim_importer.cpp
Total findings: 27

- Line 375: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: jn["metadata"] = n.metadata;
- Line 428: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: jn["metadata"] = n.metadata;
- Line 60: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: if      (ent == "&amp;")  out += '&';
  Confidence: band=high; score=0.74
- Line 61: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: if      (ent == "&amp;")  out += '&';
- Line 62: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: else if (ent == "<")   out += '<';
- Line 63: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: else if (ent == ">")   out += '>';
- Line 64: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: else if (ent == "&quot;") out += '"';
- Line 65: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: else if (ent == "&apos;") out += '\'';
- Line 293: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: entries.push_back(current);
- Line 324: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: results.push_back(std::move(single));
- Line 344: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results.push_back(std::move(r));
  Confidence: band=high; score=0.74
- Line 345: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: results.push_back(std::move(r));
- Line 354: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: results.push_back(std::move(r));
- Line 365: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: rec.compliance_tags.push_back("fim:leika:" + entry.leika_key);
- Line 367: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: rec.compliance_tags.push_back("fim:import");
- Line 375: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: nodes_arr.push_back(std::move(jn));
  Confidence: band=high; score=0.74
- Line 376: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: nodes_arr.push_back(std::move(jn));
- Line 383: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: edges_arr.push_back(std::move(je));
  Confidence: band=high; score=0.74
- Line 384: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: edges_arr.push_back(std::move(je));
- Line 392: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: results.push_back(std::move(r));
- Line 428: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: nodes_arr.push_back(std::move(jn));
  Confidence: band=high; score=0.74
- Line 429: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: nodes_arr.push_back(std::move(jn));
- Line 436: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: edges_arr.push_back(std::move(je));
  Confidence: band=high; score=0.74
- Line 437: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: edges_arr.push_back(std::move(je));
- Line 508: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results.push_back(importSingleModel(bpmn_xml, domain));
  Confidence: band=high; score=0.74
- Line 509: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: results.push_back(importSingleModel(bpmn_xml, domain));
- Line 515: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: results.push_back(std::move(r));

### src/process/process_model_manager.cpp
Total findings: 23

- Line 565: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: db_.scanPrefix("proc:def:", [&](std::string_view /*key*/, std::string_view value) -> bool {
- Line 617: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: db_.scanPrefix("proc:def:", [&](std::string_view /*key*/, std::string_view value) -> bool {
- Line 670: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: db_.scanPrefix("proc:def:", [&](std::string_view /*key*/, std::string_view value) -> bool {
- Line 196: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: r.compliance_tags.push_back(tag.get<std::string>());
  Confidence: band=high; score=0.74
- Line 197: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: r.compliance_tags.push_back(tag.get<std::string>());
- Line 208: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: r.embedding.push_back(v.get<float>());
  Confidence: band=high; score=0.74
- Line 209: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: r.embedding.push_back(v.get<float>());
- Line 356: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: jedges.push_back(std::move(je));
  Confidence: band=high; score=0.74
- Line 357: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: jedges.push_back(std::move(je));
- Line 580: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 603: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results.push_back(std::move(*rec));
  Confidence: band=high; score=0.74
- Line 604: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: results.push_back(std::move(*rec));
- Line 631: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: results.push_back(std::move(r));
- Line 634: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {}
- Line 660: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: candidates.emplace_back(std::move(*rec), sim);
  Confidence: band=high; score=0.74
- Line 688: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: candidates.emplace_back(std::move(r), sim);
  Confidence: band=high; score=0.74
- Line 691: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {}
- Line 728: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: nodes.push_back(n);
  Confidence: band=high; score=0.74
- Line 729: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: nodes.push_back(n);
- Line 738: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: edges.push_back(e);
  Confidence: band=high; score=0.74
- Line 739: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: edges.push_back(e);
- Line 751: severity=MEDIUM; category=llm_ai_safety; pattern=missing_resource_limits
  Description: LLM inference without token limit or timeout (DOS risk)
  Context: return LlmProcessDescriptor::generate(*record);
  Confidence: band=high; score=0.74
- Line 770: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: "Failed to register process '" + record->id + "': " + status.message);

### src/process/object_centric_tracer.cpp
Total findings: 21

- Line 61: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: global_log["ocel:attribute-names"] = json::array();
- Line 77: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: omap[att.object_collection] = json::array({att.object_id});
- Line 111: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: result["nodes"]       = json::array();
- Line 112: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: result["arcs"]        = json::array();
- Line 52: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> object_types_set;
  Confidence: band=medium; score=0.66
- Line 62: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: for (const auto& ot : object_types_set) obj_types_arr.push_back(ot);
  Confidence: band=high; score=0.74
- Line 63: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: for (const auto& ot : object_types_set) obj_types_arr.push_back(ot);
- Line 82: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: events_arr.push_back(std::move(ev));
  Confidence: band=high; score=0.74
- Line 82: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: events_arr.push_back(std::move(ev));
  Confidence: band=high; score=0.74
- Line 83: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: events_arr.push_back(std::move(ev));
- Line 126: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> node_set;
  Confidence: band=medium; score=0.66
- Line 136: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, std::unordered_map<std::string, int>> freq;
  Confidence: band=medium; score=0.66
- Line 170: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: for (const auto& nid : node_set) nodes_arr.push_back(nid);
- Line 176: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: arcs_arr.push_back({{"from", from}, {"to", to}, {"frequency", cnt}});
  Confidence: band=high; score=0.74
- Line 176: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: arcs_arr.push_back({{"from", from}, {"to", to}, {"frequency", cnt}});
  Confidence: band=high; score=0.74
- Line 176: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: arcs_arr.push_back({{"from", from}, {"to", to}, {"frequency", cnt}});
  Confidence: band=high; score=0.74
- Line 177: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: arcs_arr.push_back({{"from", from}, {"to", to}, {"frequency", cnt}});
- Line 206: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, std::unordered_map<std::string, int>> in_deg;
  Confidence: band=medium; score=0.66
- Line 207: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, std::unordered_map<std::string, int>> out_deg;
  Confidence: band=medium; score=0.66
- Line 226: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> conv_set;
  Confidence: band=medium; score=0.66
- Line 227: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> div_set;
  Confidence: band=medium; score=0.66

### src/process/process_model_generator.cpp
Total findings: 20

- Line 189: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {}
- Line 199: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 221: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> all_node_ids;
  Confidence: band=medium; score=0.66
- Line 222: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, std::string> node_types; // id → type
  Confidence: band=medium; score=0.66
- Line 223: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, int>         out_degree;
  Confidence: band=medium; score=0.66
- Line 224: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, int>         in_degree;
  Confidence: band=medium; score=0.66
- Line 248: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.errors.push_back("No startEvent node found");
  Confidence: band=high; score=0.74
- Line 249: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.errors.push_back("No startEvent node found");
- Line 253: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.errors.push_back("No endEvent node found");
- Line 258: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.errors.push_back("Isolated node: " + nid);
  Confidence: band=high; score=0.74
- Line 259: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.errors.push_back("Isolated node: " + nid);
- Line 267: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.errors.push_back("Gateway '" + nid + "' has no outgoing edges");
- Line 305: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: norm_nodes.push_back(node);
  Confidence: band=high; score=0.74
- Line 306: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: norm_nodes.push_back(node);
- Line 317: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: norm_nodes.push_back(node);
  Confidence: band=high; score=0.74
- Line 318: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: norm_nodes.push_back(node);
- Line 328: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: norm_nodes.push_back(node);
  Confidence: band=high; score=0.74
- Line 329: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: norm_nodes.push_back(node);
- Line 340: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: norm_edges.push_back(edge);
  Confidence: band=high; score=0.74
- Line 341: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: norm_edges.push_back(edge);

### src/process/epk_aris_xml_importer.cpp
Total findings: 18

- Line 366: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: current_model.occ_to_def[it_id->second] = it_ref->second;
- Line 397: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: current_obj_id = (it_id != tag.attrs.end()) ? it_id->second : "";
- Line 399: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: try { current_obj.type_num = std::stoi(it_num->second); }
- Line 49: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: if      (ent == "&amp;")  out += '&';
  Confidence: band=high; score=0.74
- Line 50: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: if      (ent == "&amp;")  out += '&';
- Line 51: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: else if (ent == "<")   out += '<';
- Line 52: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: else if (ent == ">")   out += '>';
- Line 53: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: else if (ent == "&quot;") out += '"';
- Line 54: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: else if (ent == "&apos;") out += '\'';
- Line 74: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 371: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {}
- Line 400: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: catch (...) {}
- Line 453: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: const std::unordered_map<std::string, ObjDefInfo>& obj_defs)
  Confidence: band=medium; score=0.66
- Line 489: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: res.nodes.push_back(std::move(node));
- Line 501: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: res.edges.push_back(std::move(edge));
  Confidence: band=high; score=0.74
- Line 502: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: res.edges.push_back(std::move(edge));
- Line 595: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results.push_back(buildImportResult(m, parsed.obj_defs));
  Confidence: band=high; score=0.74
- Line 596: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: results.push_back(buildImportResult(m, parsed.obj_defs));

### src/process/epk_serializer.cpp
Total findings: 18

- Line 252: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: if (has_incoming.find(n.node_id) == has_incoming.end()) {
- Line 143: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.nodes.push_back(node);
  Confidence: band=high; score=0.74
- Line 144: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.nodes.push_back(node);
- Line 153: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.edges.push_back(std::move(edge));
- Line 198: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.nodes.push_back(std::move(n));
  Confidence: band=high; score=0.74
- Line 199: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.nodes.push_back(std::move(n));
- Line 209: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.edges.push_back(std::move(e));
  Confidence: band=high; score=0.74
- Line 210: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.edges.push_back(std::move(e));
- Line 232: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, std::vector<std::string>> adj;
  Confidence: band=medium; score=0.66
- Line 233: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: adj[e.from_node].push_back(e.to_node);
  Confidence: band=high; score=0.74
- Line 234: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: adj[e.from_node].push_back(e.to_node);
- Line 238: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, const ProcessNodeInfo*> node_map;
  Confidence: band=medium; score=0.66
- Line 244: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> has_incoming;
  Confidence: band=medium; score=0.66
- Line 250: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> visited;
  Confidence: band=medium; score=0.66
- Line 322: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: jnodes.push_back(std::move(jn));
  Confidence: band=high; score=0.74
- Line 323: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: jnodes.push_back(std::move(jn));
- Line 333: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: jedges.push_back(std::move(je));
  Confidence: band=high; score=0.74
- Line 334: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: jedges.push_back(std::move(je));

### src/process/process_community_detector.cpp
Total findings: 18

- Line 297: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator nit may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto nit = node_names.find(pc.node_ids[i]);
- Line 282: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: auto it = g.adj[u].find(v);
- Line 283: severity=HIGH; category=performance; pattern=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Context: auto it = g.adj[u].find(v);
  Confidence: band=very_high; score=0.9
- Line 297: severity=HIGH; category=performance; pattern=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Context: auto nit = node_names.find(pc.node_ids[i]);
  Confidence: band=very_high; score=0.9
- Line 416: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: db_.scanPrefix(prefix, [&](std::string_view /*key*/, std::string_view value) -> bool {
- Line 46: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, int> node_index;  // node_id → index
  Confidence: band=medium; score=0.66
- Line 47: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::vector<std::unordered_map<int, float>> adj;  // adjacency list (weighted)
  Confidence: band=medium; score=0.66
- Line 62: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: g.node_ids.push_back(id);
  Confidence: band=high; score=0.74
- Line 63: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: g.node_ids.push_back(id);
- Line 132: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<int, std::unordered_set<int>> comm_nodes;
  Confidence: band=medium; score=0.66
- Line 141: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<int> visited_comms;
  Confidence: band=medium; score=0.66
- Line 193: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, std::string> node_names;
  Confidence: band=medium; score=0.66
- Line 214: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<int, int> label_remap;
  Confidence: band=medium; score=0.66
- Line 257: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<int, std::vector<int>> comm_map;
  Confidence: band=medium; score=0.66
- Line 259: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: comm_map[assignment[u]].push_back(u);
- Line 274: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: for (int u : members) pc.node_ids.push_back(g.node_ids[u]);
- Line 304: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: communities.push_back(std::move(pc));
  Confidence: band=high; score=0.74
- Line 305: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: communities.push_back(std::move(pc));

### src/process/process_linker.cpp
Total findings: 14

- Line 78: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: doc["metadata"]          = metadata;
- Line 270: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: db_.scanPrefix(prefix, [&](std::string_view /*key*/, std::string_view value) -> bool {
- Line 322: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: db_.scanPrefix(prefix, [&](std::string_view key, std::string_view /*value*/) -> bool {
- Line 385: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: db_.scanPrefix(prefix, [&](std::string_view /*key*/, std::string_view value) -> bool {
- Line 448: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: db_.scanPrefix(prefix, [&](std::string_view /*key*/, std::string_view value) -> bool {
- Line 486: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: if (!dtype.empty() && present_types.find(dtype) == present_types.end()) {
- Line 486: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: if (!dtype.empty() && present_types.find(dtype) == present_types.end()) {
- Line 298: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(std::move(att));
  Confidence: band=high; score=0.74
- Line 299: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.push_back(std::move(att));
- Line 450: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: results.push_back(json::parse(value));
- Line 474: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> present_types;
  Confidence: band=medium; score=0.66
- Line 487: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: missing.push_back(dtype);
  Confidence: band=high; score=0.74
- Line 487: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: missing.push_back(dtype);
  Confidence: band=high; score=0.74
- Line 488: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: missing.push_back(dtype);

### src/process/llm_process_descriptor.cpp
Total findings: 11

- Line 113: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: desc["nodes"] = std::move(nodes_array);
- Line 122: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: desc["edges"] = std::move(edges_array);
- Line 73: severity=MEDIUM; category=llm_ai_safety; pattern=missing_resource_limits
  Description: LLM inference without token limit or timeout (DOS risk)
  Context: json LlmProcessDescriptor::generate(const ProcessModelRecord& record)
  Confidence: band=high; score=0.74
- Line 75: severity=MEDIUM; category=llm_ai_safety; pattern=missing_resource_limits
  Description: LLM inference without token limit or timeout (DOS risk)
  Context: return generate(record, Config{});
  Confidence: band=high; score=0.74
- Line 78: severity=MEDIUM; category=llm_ai_safety; pattern=missing_resource_limits
  Description: LLM inference without token limit or timeout (DOS risk)
  Context: json LlmProcessDescriptor::generate(
  Confidence: band=high; score=0.74
- Line 106: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: nodes_array.push_back(nodeToJson_(jn, cfg));
  Confidence: band=high; score=0.74
- Line 107: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: nodes_array.push_back(nodeToJson_(jn, cfg));
- Line 108: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: total_sla_ms += static_cast<size_t>(jn["timeout_ms"].get<double>());
  Confidence: band=high; score=0.74
- Line 118: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: edges_array.push_back(edgeToJson_(je));
  Confidence: band=high; score=0.74
- Line 119: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: edges_array.push_back(edgeToJson_(je));
- Line 279: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: prompt << "3. List all deviations from expected model with severity (LOW/MEDIUM/HIGH)\n";

### src/process/process_light_retriever.cpp
Total findings: 7

- Line 49: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(static_cast<char>(std::tolower(c)));
  Confidence: band=high; score=0.74
- Line 50: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.push_back(static_cast<char>(std::tolower(c)));
- Line 70: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: RetrievalMode ProcessLightRetriever::classifyQuery(std::string_view query) const {
  Confidence: band=high; score=0.74
- Line 88: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: ? classifyQuery(query)
  Confidence: band=high; score=0.74
- Line 102: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {}
- Line 121: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: used_ids.push_back(communities[i].community_id);
  Confidence: band=high; score=0.74
- Line 122: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: used_ids.push_back(communities[i].community_id);

## Update Workflow

- Refresh scan artifacts with: python tools/gap_scanner_v3.py
- Regenerate all module notes with: python tools/module_doc_generator.py . ai_working ai_working/module_gaps
- The generator mirrors each archive document directly into src/<module>/MODULE_GAPS.md.

Format: THEMIS_MODULE_GAPS_V3
