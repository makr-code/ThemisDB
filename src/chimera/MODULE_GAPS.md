# chimera Module - Developer Gap Note

> Auto-generated from ai_working/gap_scan_v3_aggregate.json.
> This file is overwritten on each regeneration.

## Scan Snapshot

- Module: chimera
- Generated: 2026-06-02 11:09:12
- Status: Critical Findings Present
- Total Findings: 39
- Actionable Findings (Critical + High): 9
- Affected Files: 3

## Severity Summary

| Severity | Count |
|---|---:|
| Critical | 4 |
| High | 5 |
| Medium | 30 |
| Low | 0 |

## Category Summary

| Category | Count |
|---|---:|
| performance_patterns | 32 |
| reliability | 28 |
| container | 17 |
| performance | 10 |
| raii | 10 |
| audit_logging | 4 |
| security | 3 |
| observability | 2 |
| determinism | 1 |
| exception_safety | 1 |
| platform | 1 |
| type_conversion | 1 |

## File Overview

| File | Findings | Critical | High | Medium | Low |
|---|---:|---:|---:|---:|---:|
| src/chimera/themisdb_adapter.cpp | 39 | 4 | 5 | 30 | 0 |
| include/chimera/database_adapter.hpp | 0 | 0 | 0 | 0 | 0 |
| include/chimera/themisdb_adapter.hpp | 0 | 0 | 0 | 0 | 0 |

## Full Scanner Findings

### src/chimera/themisdb_adapter.cpp
Total findings: 39

- Line 120: severity=CRITICAL; category=audit_logging; pattern=missing_audit_log
  Description: Security function "execute_query" without audit log
  Context: Result<RelationalTable> ThemisDBAdapter::execute_query(
  Confidence: band=very_high; score=0.99
- Line 1353: severity=CRITICAL; category=audit_logging; pattern=missing_audit_log
  Description: Security function "execute_query" without audit log
  Context: return execute_query(query, params);
  Confidence: band=very_high; score=0.99
- Line 1479: severity=CRITICAL; category=audit_logging; pattern=missing_audit_log
  Description: Security function "execute_query" without audit log
  Context: auto table_result = execute_query(query, params);
  Confidence: band=very_high; score=0.99
- Line 1654: severity=CRITICAL; category=audit_logging; pattern=missing_audit_log
  Description: Security function "execute_query" without audit log
  Context: auto result = adapter_->execute_query(effective_query, pos_params);
  Confidence: band=very_high; score=0.99
- Line 149: severity=HIGH; category=performance; pattern=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Context: if (std::find(table.column_names.begin(),
  Confidence: band=very_high; score=0.9
- Line 201: severity=HIGH; category=performance; pattern=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Context: if (std::find(table.column_names.begin(),
  Confidence: band=very_high; score=0.9
- Line 815: severity=HIGH; category=performance; pattern=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Context: auto field_it = doc.fields.find(key);
  Confidence: band=very_high; score=0.9
- Line 851: severity=HIGH; category=performance; pattern=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Context: auto fld = doc.fields.find(key);
  Confidence: band=very_high; score=0.9
- Line 1645: severity=HIGH; category=observability; pattern=missing_trace_point
  Description: Critical function execute without trace point
  Context: Result<RelationalTable> ThemisDBPreparedStatement::execute() {
  Confidence: band=very_high; score=0.9
- Line 151: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: table.column_names.push_back(col);
  Confidence: band=high; score=0.74
- Line 151: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: table.column_names.push_back(col);
  Confidence: band=high; score=0.74
- Line 203: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: table.column_names.push_back(col);
  Confidence: band=high; score=0.74
- Line 203: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: table.column_names.push_back(col);
  Confidence: band=high; score=0.74
- Line 295: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: store.emplace_back(generate_id(), v);
  Confidence: band=high; score=0.74
- Line 306: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: const std::map<std::string, Scalar>& filters
  Confidence: band=high; score=0.74
- Line 331: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results.emplace_back(std::move(v), static_cast<double>(r.distance));
  Confidence: band=high; score=0.74
- Line 381: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: scored.emplace_back(i, cos_dist);
  Confidence: band=high; score=0.74
- Line 381: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: scored.emplace_back(i, cos_dist);
  Confidence: band=high; score=0.74
- Line 397: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results.emplace_back(store[idx].second, dist);
  Confidence: band=high; score=0.74
- Line 504: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: path.nodes.push_back(std::move(node));
  Confidence: band=high; score=0.74
- Line 542: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: std::map<std::string, std::string> parent;   // node -> predecessor node
  Confidence: band=high; score=0.74
- Line 543: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: std::map<std::string, std::string> via_edge; // node -> edge_id used to reach it
  Confidence: band=high; score=0.74
- Line 579: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: node_seq.push_back(cur);
  Confidence: band=high; score=0.74
- Line 579: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: node_seq.push_back(cur);
  Confidence: band=high; score=0.74
- Line 591: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: path.nodes.push_back(nit->second);
  Confidence: band=high; score=0.74
- Line 600: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: path.edges.push_back(eit->second);
  Confidence: band=high; score=0.74
- Line 630: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> seen_ids;
  Confidence: band=medium; score=0.66
- Line 637: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: nodes.push_back(it->second);
  Confidence: band=high; score=0.74
- Line 684: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: std::map<std::string, bool> visited;
  Confidence: band=high; score=0.74
- Line 793: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: const std::map<std::string, Scalar>& filter,
  Confidence: band=high; score=0.74
- Line 822: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: matched.push_back(doc);
  Confidence: band=high; score=0.74
- Line 822: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: matched.push_back(doc);
  Confidence: band=high; score=0.74
- Line 831: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: const std::map<std::string, Scalar>& filter,
  Confidence: band=high; score=0.74
- Line 832: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: const std::map<std::string, Scalar>& updates
  Confidence: band=high; score=0.74
- Line 1424: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: const std::map<std::string, Scalar>& filters,
  Confidence: band=high; score=0.74
- Line 1541: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: ids.push_back(kv.first);
  Confidence: band=high; score=0.74
- Line 1614: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: std::string ThemisDBPreparedStatement::get_query() const { return query_; }
  Confidence: band=high; score=0.74
- Line 1719: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: if (c == '\\') escaped += "\\\\";
  Confidence: band=high; score=0.74
- Line 1719: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: if (c == '\\') escaped += "\\\\";
  Confidence: band=high; score=0.74

### include/chimera/database_adapter.hpp
Total findings: 0


### include/chimera/themisdb_adapter.hpp
Total findings: 0

## Update Workflow

- Refresh scan artifacts with: python tools/gap_scanner_v3.py
- Regenerate all module notes with: python tools/module_doc_generator.py . ai_working ai_working/module_gaps
- The generator mirrors each archive document directly into src/<module>/MODULE_GAPS.md.

Format: THEMIS_MODULE_GAPS_V3
