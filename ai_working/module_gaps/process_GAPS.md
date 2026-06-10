# process Module - Developer Gap Note

> Auto-generated from ai_working\gap_scan_results.json.
> This file is overwritten on each regeneration.

## Scan Snapshot

- Module: process
- Generated: 2026-06-04 08:50:22
- Status: Critical Findings Present
- Total Findings: 177
- Actionable Findings (Critical + High): 53
- Affected Files: 19

## Severity Summary

| Severity | Count |
|---|---:|
| Critical | 5 |
| High | 48 |
| Medium | 111 |
| Low | 13 |

## Category Summary

| Category | Count |
|---|---:|
| string_concat_loop | 37 |
| unordered_container_iter | 37 |
| pointer_arithmetic_unbounded | 18 |
| hardcoded_path | 12 |
| hardcoded_output | 11 |
| o_n_squared | 8 |
| resource_leaked_in_exception | 7 |
| generic_catch | 6 |
| nested_loop_find | 6 |
| uncaught_exception | 6 |
| copy_overhead | 5 |
| regex_in_loop | 5 |
| missing_resource_limits | 4 |
| iterator_invalidation | 2 |
| missing_latency_metric | 2 |
| module_doc_linkset_drift | 2 |
| allocation_loop | 1 |
| data_race | 1 |
| delete_without_nullptr | 1 |
| explicit_delete | 1 |
| multiplication_overflow | 1 |
| new_without_raii | 1 |
| timestamp_sorting_unstable | 1 |
| uninitialized_member_field | 1 |
| user_controlled_size | 1 |

## File Overview

| File | Findings | Critical | High | Medium | Low |
|---|---:|---:|---:|---:|---:|
| process/process_graph_rag.cpp | 28 | 2 | 12 | 14 | 0 |
| process/bpmn_serializer.cpp | 22 | 0 | 3 | 19 | 0 |
| process/cmmn_serializer.cpp | 17 | 0 | 1 | 16 | 0 |
| process/process_agentic_rag.cpp | 14 | 0 | 12 | 2 | 0 |
| process/vcc_vpb_importer.cpp | 14 | 1 | 9 | 4 | 0 |
| process/dmn_evaluator.cpp | 13 | 2 | 0 | 0 | 11 |
| process/epk_aris_xml_importer.cpp | 10 | 0 | 0 | 10 | 0 |
| process/process_community_detector.cpp | 10 | 0 | 3 | 7 | 0 |
| process/ocel_exporter.cpp | 8 | 0 | 2 | 6 | 0 |
| process/object_centric_tracer.cpp | 7 | 0 | 0 | 7 | 0 |
| process/fim_importer.cpp | 6 | 0 | 0 | 6 | 0 |
| process/process_linker.cpp | 6 | 0 | 5 | 1 | 0 |
| process/epk_serializer.cpp | 5 | 0 | 1 | 4 | 0 |
| process/llm_process_descriptor.cpp | 5 | 0 | 0 | 5 | 0 |
| process/process_light_retriever.cpp | 5 | 0 | 0 | 5 | 0 |
| process/process_model_generator.cpp | 4 | 0 | 0 | 4 | 0 |
| process/FUTURE_ENHANCEMENTS.md | 1 | 0 | 0 | 0 | 1 |
| process/PRODUCTION_REQUIREMENTS.md | 1 | 0 | 0 | 0 | 1 |
| process/process_model_manager.cpp | 1 | 0 | 0 | 1 | 0 |

## Full Scanner Findings

### process/process_graph_rag.cpp
Total findings: 28

- Line 367: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator fi may be invalidated by container modification
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: auto fi = node_index.find(from);
- Line 368: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator ti may be invalidated by container modification
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: auto ti = node_index.find(to);
- Line 244: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: att_node.type           = rag::kg::EntityType::PRODUCT;

            att_node.properties["collection"] = att.object_collection;

            att_node.properties["link_type"]  = std::string(toString(att.link_type));

            if (att.metadata.contains("doc_type") && att.metadata["doc_type"].is_string()) {

                att_node.properties["doc_type"] = att.metadata["doc_type"].get<std::string>();

            }

            kg.nodes.push_back(std::move(att_node));
- Line 245: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: att_node.properties["collection"] = att.object_collection;

            att_node.properties["link_type"]  = std::string(toString(att.link_type));

            if (att.metadata.contains("doc_type") && att.metadata["doc_type"].is_string()) {

                att_node.properties["doc_type"] = att.metadata["doc_type"].get<std::string>();

            }

            kg.nodes.push_back(std::move(att_node));
- Line 366: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: auto fi = node_index.find(from);
- Line 367: severity=HIGH; category=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: auto fi = node_index.find(from);
- Line 367: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: auto ti = node_index.find(to);
- Line 368: severity=HIGH; category=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: auto ti = node_index.find(to);
- Line 377: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: auto it = node_index.find(seed);
- Line 378: severity=HIGH; category=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: auto it = node_index.find(seed);
- Line 397: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 409: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 412: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 415: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 59: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: float jaccardSimilarity(const std::unordered_set<std::string>& a,
- Line 60: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: const std::unordered_set<std::string>& b) {
- Line 279: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_map<std::string, std::vector<std::pair<std::string, json>>> adj;
- Line 290: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_set<std::string> visited_nodes;
- Line 291: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_set<std::string> visited_edges;
- Line 357: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_map<std::string, int> node_index;
- Line 516: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_set<std::string> top_set;
- Line 667: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_set<std::string> all_visited;
- Line 881: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_set<std::string> ref_var_keys;
- Line 907: severity=MEDIUM; category=generic_catch
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: for (const auto& v : emb_json) other_emb.push_back(v.get<float>());

                    sim = cosineSimilarity(ref_embedding, other_emb);

                }

            } catch (...) {}

        }



        if (sim >= min_similarity) {
- Line 907: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: } catch (...) {}
- Line 948: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_set<std::string> other_var_keys;
- Line 1327: severity=MEDIUM; category=generic_catch
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: std::string val;

        if (db_.get(key, val) && !val.empty()) {

            try { agg = json::parse(val); }

            catch (...) { agg = json::object(); }

        }

    }
- Line 1327: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: catch (...) { agg = json::object(); }

### process/bpmn_serializer.cpp
Total findings: 22

- Line 20: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: * Implementation note: we use a minimal hand-written XML parser for import

 * (no external XML library dependency) and produce standards-compliant XML on

 * export.  BPMNDI (diagram interchange) BPMNShape bounds (x/y/width/height)

 * are parsed on import and stored in ProcessNodeInfo::metadata["layout"].

 * BPMNDI data is not emitted on export because ThemisDB stores processes as

 * graph data, not as graphical diagrams.

 */
- Line 611: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: layout["y"]      = b.y;

            layout["width"]  = b.width;

            layout["height"] = b.height;

            node.metadata["layout"] = std::move(layout);

        }

    };
- Line 637: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: if (it->second.retention_days.has_value()) {

            ann_json["retention_days"] = *it->second.retention_days;

        }

        node.metadata["dsgvo_annotation"] = std::move(ann_json);

    }



    if (result.process_id.empty()) {
- Line 59: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: if      (ent == "&amp;")  out += '&';
- Line 60: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: if      (ent == "&amp;")  out += '&';
- Line 61: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: else if (ent == "<")   out += '<';
- Line 62: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: else if (ent == ">")   out += '>';
- Line 63: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: else if (ent == "&quot;") out += '"';
- Line 64: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: else if (ent == "&apos;") out += '\'';
- Line 259: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: case '&':  out += "&amp;";  break;
- Line 260: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: case '&':  out += "&amp;";  break;
- Line 261: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: case '<':  out += "<";   break;
- Line 262: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: case '>':  out += ">";   break;
- Line 263: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: case '"':  out += "&quot;"; break;
- Line 264: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: case '\'': out += "&apos;"; break;
- Line 733: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: xml << "/>\n";
- Line 734: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: xml << "      </extensionElements>\n";
- Line 735: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: xml << "    </" << tag << ">\n";
- Line 737: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: xml << "/>\n";
- Line 759: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: xml << "/>\n";
- Line 762: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: xml << "  </process>\n";
- Line 763: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: xml << "</definitions>\n";

### process/cmmn_serializer.cpp
Total findings: 17

- Line 341: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: }



            // Store case_id in metadata for traceability.

            node.metadata["cmmn_case_id"] = result.case_id;



            result.nodes.push_back(std::move(node));
- Line 53: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: if      (ent == "&amp;")  out += '&';
- Line 54: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: if      (ent == "&amp;")  out += '&';
- Line 55: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: else if (ent == "<")   out += '<';
- Line 56: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: else if (ent == ">")   out += '>';
- Line 57: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: else if (ent == "&quot;") out += '"';
- Line 58: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: else if (ent == "&apos;") out += '\'';
- Line 223: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: case '&':  out += "&amp;";  break;
- Line 224: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: case '&':  out += "&amp;";  break;
- Line 225: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: case '<':  out += "<";   break;
- Line 226: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: case '>':  out += ">";   break;
- Line 227: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: case '"':  out += "&quot;"; break;
- Line 228: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: case '\'': out += "&apos;"; break;
- Line 505: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: << " name=\"" << escapeXml_(n.name) << "\"/>\n";
- Line 508: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: xml << "    </casePlanModel>\n";
- Line 509: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: xml << "  </case>\n";
- Line 510: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: xml << "</definitions>\n";

### process/process_agentic_rag.cpp
Total findings: 14

- Line 56: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: d.id               = "proc_prompt:" + ctx.instance_id;

        d.content          = ctx.llm_prompt;

        d.similarity_score = 1.0;

        d.metadata["type"] = "llm_prompt";

        d.metadata["instance_id"] = ctx.instance_id;

        docs.push_back(std::move(d));

    }
- Line 57: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: d.content          = ctx.llm_prompt;

        d.similarity_score = 1.0;

        d.metadata["type"] = "llm_prompt";

        d.metadata["instance_id"] = ctx.instance_id;

        docs.push_back(std::move(d));

    }
- Line 67: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: d.id               = "proc_subgraph:" + ctx.instance_id;

        d.content          = ctx.subgraph.dump(2);

        d.similarity_score = 0.9;

        d.metadata["type"] = "subgraph";

        d.metadata["instance_id"] = ctx.instance_id;

        d.metadata["process_name"] = ctx.process_name;

        docs.push_back(std::move(d));
- Line 68: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: d.content          = ctx.subgraph.dump(2);

        d.similarity_score = 0.9;

        d.metadata["type"] = "subgraph";

        d.metadata["instance_id"] = ctx.instance_id;

        d.metadata["process_name"] = ctx.process_name;

        docs.push_back(std::move(d));

    }
- Line 69: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: d.similarity_score = 0.9;

        d.metadata["type"] = "subgraph";

        d.metadata["instance_id"] = ctx.instance_id;

        d.metadata["process_name"] = ctx.process_name;

        docs.push_back(std::move(d));

    }
- Line 81: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: std::to_string(att_idx++);

        d.content          = att.dump(2);

        d.similarity_score = 0.8;

        d.metadata["type"] = "attachment";

        d.metadata["instance_id"] = ctx.instance_id;

        docs.push_back(std::move(d));

    }
- Line 82: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: d.content          = att.dump(2);

        d.similarity_score = 0.8;

        d.metadata["type"] = "attachment";

        d.metadata["instance_id"] = ctx.instance_id;

        docs.push_back(std::move(d));

    }
- Line 94: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: std::to_string(sc_idx++);

        d.content          = sc.dump(2);

        d.similarity_score = 0.7;

        d.metadata["type"] = "similar_case";

        docs.push_back(std::move(d));

    }
- Line 108: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: d.id               = "proc_missing:" + ctx.instance_id;

        d.content          = oss.str();

        d.similarity_score = 0.85;

        d.metadata["type"] = "missing_documents";

        d.metadata["instance_id"] = ctx.instance_id;

        docs.push_back(std::move(d));

    }
- Line 109: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: d.content          = oss.str();

        d.similarity_score = 0.85;

        d.metadata["type"] = "missing_documents";

        d.metadata["instance_id"] = ctx.instance_id;

        docs.push_back(std::move(d));

    }
- Line 124: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: auto type_it = doc.metadata.find("type");
- Line 178: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 141: severity=MEDIUM; category=generic_catch
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: if (!found) {

                try {

                    ctx.attachments.push_back(nlohmann::json::parse(doc.content));

                } catch (...) {

                    ctx.attachments.push_back(nlohmann::json{{"_id", doc.id},

                                                              {"content", doc.content}});

                }
- Line 141: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: } catch (...) {

### process/vcc_vpb_importer.cpp
Total findings: 14

- Line 623: severity=CRITICAL; category=new_without_raii
  Description: Raw new() without RAII wrapper — potential memory leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: std::string current_chunk;



    while (std::getline(ss, line)) {

        // Check for start of new model (line starts with 2 spaces + "- ")

        bool is_new_model = (line.size() >= 4 && line[0] == ' ' && line[1] == ' '

                             && line[2] == '-' && line[3] == ' ');

        // Also detect "  -\n" (just the dash, id on next line)
- Line 220: severity=HIGH; category=regex_in_loop
  Description: std::regex compiled in loop (compile once, reuse)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: for (const auto& l : lines) {
- Line 227: severity=HIGH; category=regex_in_loop
  Description: std::regex compiled in loop (compile once, reuse)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: for (; it != end; ++it) {
- Line 239: severity=HIGH; category=regex_in_loop
  Description: std::regex compiled in loop (compile once, reuse)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: for (const auto& l : lines) {
- Line 265: severity=HIGH; category=regex_in_loop
  Description: std::regex compiled in loop (compile once, reuse)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: for (size_t i = 0; i < lines.size(); ++i) {
- Line 334: severity=HIGH; category=regex_in_loop
  Description: std::regex compiled in loop (compile once, reuse)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: for (const auto& l : lines) {
- Line 623: severity=HIGH; category=allocation_loop
  Description: Dynamic allocation in loop — high overhead
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: // Check for start of new model (line starts with 2 spaces + "- ")
- Line 624: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 624: severity=HIGH; category=user_controlled_size
  Description: Allocation size not validated (potential DoS or overflow)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::input_validation
  Context: ['    while (std::getline(ss, line)) {', '        // Check for start of new model (line starts with 2 spaces + "- ")', "        bool is_new_model = (line.size() >= 4 && line[0] == ' ' && line[1] == ' '", "                             && line[2] == '-' && line[3] == ' ');", '        // Also detect "  -\\n" (just the dash, id on next line)']
- Line 630: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 163: severity=MEDIUM; category=uninitialized_member_field
  Description: Default constructor does not initialize POD members
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::uninitialized
  Context: Struct with uninitialized fields
- Line 471: severity=MEDIUM; category=generic_catch
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: sla_h = act["sla_hours"].get<double>();

                } else if (act["sla_hours"].is_string()) {

                    try { sla_h = std::stod(act["sla_hours"].get<std::string>()); }

                    catch (...) {}

                }

                if (sla_h > 0) {

                    n.timeout = std::chrono::milliseconds(
- Line 471: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: catch (...) {}
- Line 661: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: results.push_back(importYaml(chunk, meta_defaults));

### process/dmn_evaluator.cpp
Total findings: 13

- Line 254: severity=CRITICAL; category=multiplication_overflow
  Description: Multi-factor multiplication detected (CWE-190, likely overflow risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::type_conversion
  Context: ['', '    // Security guard: 10 MiB', '    if (dmn_xml.size() > 10 * 1024 * 1024) {', '        SPDLOG_ERROR("[DmnEvaluator] DMN XML exceeds 10 MiB size limit");', '        return false;']
- Line 260: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: auto stripNs = [](std::string_view tag) -> std::string_view {
- Line 208: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: // Support both "inputs" (array) and "input_expressions" (array)
- Line 209: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: const auto& inputs_key =
- Line 210: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: r.contains("inputs") ? "inputs" : "input_expressions";
- Line 211: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: if (r.contains(inputs_key) && r[inputs_key].is_array()) {
- Line 212: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: for (const auto& e : r[inputs_key]) {
- Line 217: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: // Support both "outputs" (object) and "output_values" (object)
- Line 218: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: const auto& outputs_key =
- Line 219: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: r.contains("outputs") ? "outputs" : "output_values";
- Line 220: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: if (r.contains(outputs_key) && r[outputs_key].is_object()) {
- Line 221: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: rule.output_values = r[outputs_key];
- Line 458: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: // UNIQUE and FIRST: return first matching rule's outputs

### process/epk_aris_xml_importer.cpp
Total findings: 10

- Line 47: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: if      (ent == "&amp;")  out += '&';
- Line 48: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: if      (ent == "&amp;")  out += '&';
- Line 49: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: else if (ent == "<")   out += '<';
- Line 50: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: else if (ent == ">")   out += '>';
- Line 51: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: else if (ent == "&quot;") out += '"';
- Line 52: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: else if (ent == "&apos;") out += '\'';
- Line 72: severity=MEDIUM; category=generic_catch
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: } else {

                    out += std::string(ent); // keep as-is for supplementary planes

                }

            } catch (...) {

                out += std::string(ent);

            }

        }
- Line 72: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: } catch (...) {
- Line 451: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: const std::unordered_map<std::string, ObjDefInfo>& obj_defs)
- Line 594: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: results.push_back(buildImportResult(m, parsed.obj_defs));

### process/process_community_detector.cpp
Total findings: 10

- Line 280: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: auto it = g.adj[u].find(v);
- Line 281: severity=HIGH; category=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: auto it = g.adj[u].find(v);
- Line 295: severity=HIGH; category=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: auto nit = node_names.find(pc.node_ids[i]);
- Line 44: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_map<std::string, int> node_index;  // node_id → index
- Line 45: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::vector<std::unordered_map<int, float>> adj;  // adjacency list (weighted)
- Line 130: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_map<int, std::unordered_set<int>> comm_nodes;
- Line 139: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_set<int> visited_comms;
- Line 191: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_map<std::string, std::string> node_names;
- Line 212: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_map<int, int> label_remap;
- Line 255: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_map<int, std::vector<int>> comm_map;

### process/ocel_exporter.cpp
Total findings: 8

- Line 117: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: auto vt_it = tok.visit_timestamps.find(nid);
- Line 118: severity=HIGH; category=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: auto vt_it = tok.visit_timestamps.find(nid);
- Line 124: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: entries.push_back(EventEntry{.node_id = nid, .timestamp_ms = ts});
- Line 137: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: entries.push_back(EventEntry{.node_id = tok.current_node, .timestamp_ms = ts});
- Line 142: severity=MEDIUM; category=timestamp_sorting_unstable
  Description: Timestamp-based sorting without stable_sort (non-deterministic ties)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: // Sort by timestamp
- Line 149: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_map<std::string, std::string> node_names;
- Line 196: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_set<std::string> seen;
- Line 212: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_set<std::string> seen;

### process/object_centric_tracer.cpp
Total findings: 7

- Line 50: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_set<std::string> object_types_set;
- Line 124: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_set<std::string> node_set;
- Line 134: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_map<std::string, std::unordered_map<std::string, int>> freq;
- Line 204: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_map<std::string, std::unordered_map<std::string, int>> in_deg;
- Line 205: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_map<std::string, std::unordered_map<std::string, int>> out_deg;
- Line 224: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_set<std::string> conv_set;
- Line 225: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_set<std::string> div_set;

### process/fim_importer.cpp
Total findings: 6

- Line 58: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: if      (ent == "&amp;")  out += '&';
- Line 59: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: if      (ent == "&amp;")  out += '&';
- Line 60: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: else if (ent == "<")   out += '<';
- Line 61: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: else if (ent == ">")   out += '>';
- Line 62: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: else if (ent == "&quot;") out += '"';
- Line 63: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: else if (ent == "&apos;") out += '\'';

### process/process_linker.cpp
Total findings: 6

- Line 247: severity=HIGH; category=delete_without_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: sid, ex.what());

    }



    // Hard delete the primary attachment record.

    if (!db_.del(sid)) {

        SPDLOG_WARN("[process_linker] detachObject: del failed for '{}'", sid);

        return false;
- Line 247: severity=HIGH; category=explicit_delete
  Description: Explicit delete statement (prefer smart pointers)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_raii
  Context: // Hard delete the primary attachment record.
- Line 475: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: std::unordered_set<std::string> present_types;

    for (const auto& att : node_atts) {

        // The attached_by convention: metadata["doc_type"] carries the type

        if (att.metadata.contains("doc_type") && att.metadata["doc_type"].is_string()) {

            present_types.insert(att.metadata["doc_type"].get<std::string>());

        }

    }
- Line 476: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: for (const auto& att : node_atts) {

        // The attached_by convention: metadata["doc_type"] carries the type

        if (att.metadata.contains("doc_type") && att.metadata["doc_type"].is_string()) {

            present_types.insert(att.metadata["doc_type"].get<std::string>());

        }

    }
- Line 484: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: if (!dtype.empty() && present_types.find(dtype) == present_types.end()) {
- Line 472: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_set<std::string> present_types;

### process/epk_serializer.cpp
Total findings: 5

- Line 250: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: if (has_incoming.find(n.node_id) == has_incoming.end()) {
- Line 230: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_map<std::string, std::vector<std::string>> adj;
- Line 236: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_map<std::string, const ProcessNodeInfo*> node_map;
- Line 242: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_set<std::string> has_incoming;
- Line 248: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_set<std::string> visited;

### process/llm_process_descriptor.cpp
Total findings: 5

- Line 71: severity=MEDIUM; category=missing_resource_limits
  Description: LLM inference without token limit or timeout (DOS risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: json LlmProcessDescriptor::generate(const ProcessModelRecord& record)
- Line 73: severity=MEDIUM; category=missing_resource_limits
  Description: LLM inference without token limit or timeout (DOS risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: return generate(record, Config{});
- Line 76: severity=MEDIUM; category=missing_resource_limits
  Description: LLM inference without token limit or timeout (DOS risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: json LlmProcessDescriptor::generate(
- Line 106: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: total_sla_ms += static_cast<size_t>(jn["timeout_ms"].get<double>());
- Line 277: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: prompt << "3. List all deviations from expected model with severity (LOW/MEDIUM/HIGH)\n";

### process/process_light_retriever.cpp
Total findings: 5

- Line 68: severity=MEDIUM; category=missing_latency_metric
  Description: No latency measurement for operation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: RetrievalMode ProcessLightRetriever::classifyQuery(std::string_view query) const {
- Line 86: severity=MEDIUM; category=missing_latency_metric
  Description: No latency measurement for operation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: ? classifyQuery(query)
- Line 100: severity=MEDIUM; category=generic_catch
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: const auto inst_doc = nlohmann::json::parse(inst_val);

                    model_id = inst_doc.value("model_id",

                               inst_doc.value("process_definition_id", ""));

                } catch (...) {}

            }

        }
- Line 100: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: } catch (...) {}
- Line 120: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: used_ids.push_back(communities[i].community_id);

### process/process_model_generator.cpp
Total findings: 4

- Line 219: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_set<std::string> all_node_ids;
- Line 220: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_map<std::string, std::string> node_types; // id → type
- Line 221: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_map<std::string, int>         out_degree;
- Line 222: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_map<std::string, int>         in_degree;

### process/FUTURE_ENHANCEMENTS.md
Total findings: 1

- Line 1: severity=LOW; category=module_doc_linkset_drift
  Description: Module doc 'FUTURE_ENHANCEMENTS.md' is missing expected cross-links: ARCHITECTURE.md
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::themis_doc_freshness_rules
  Context: Refresh the top-level <!-- Links: ... --> metadata to match the current module doc set

### process/PRODUCTION_REQUIREMENTS.md
Total findings: 1

- Line 1: severity=LOW; category=module_doc_linkset_drift
  Description: Module doc 'PRODUCTION_REQUIREMENTS.md' is missing expected cross-links: FUTURE_ENHANCEMENTS.md, README.md, ROADMAP.md
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::themis_doc_freshness_rules
  Context: Refresh the top-level <!-- Links: ... --> metadata to match the current module doc set

### process/process_model_manager.cpp
Total findings: 1

- Line 749: severity=MEDIUM; category=missing_resource_limits
  Description: LLM inference without token limit or timeout (DOS risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: return LlmProcessDescriptor::generate(*record);

## Update Workflow

- Refresh scanner artifacts with: python tools/gs3_orchestrator.py ./src --output ai_working/gap_scan_results.json
- Regenerate docs with: python tools/module_doc_generator.py . ai_working ai_working/module_gaps
- Add --no-mirror when you only want archive docs in ai_working/module_gaps.

Format: THEMIS_MODULE_GAPS_V4
