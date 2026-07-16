# chimera Module - Developer Gap Note

> Auto-generated from ai_working\gap_scan_results.json.
> This file is overwritten on each regeneration.

## Scan Snapshot

- Module: chimera
- Generated: 2026-06-04 08:50:22
- Status: Critical Findings Present
- Total Findings: 51
- Actionable Findings (Critical + High): 31
- Affected Files: 4

## Severity Summary

| Severity | Count |
|---|---:|
| Critical | 10 |
| High | 21 |
| Medium | 17 |
| Low | 3 |

## Category Summary

| Category | Count |
|---|---:|
| map_vs_unordered_map | 8 |
| iterator_invalidation | 6 |
| o_n_squared | 5 |
| db_connection_leak | 4 |
| missing_audit_log | 4 |
| nested_loop_find | 4 |
| string_concat_loop | 3 |
| module_doc_linkset_drift | 2 |
| null_dereference | 2 |
| repeated_search | 2 |
| cast_to_smaller_type | 1 |
| chimera_retry_duplication | 1 |
| manual_cleanup | 1 |
| missing_adr_reference | 1 |
| missing_latency_metric | 1 |
| missing_move_constructor_defaulted | 1 |
| missing_trace_point | 1 |
| no_retry_logic | 1 |
| pointer_arithmetic_unbounded | 1 |
| uninitialized_access | 1 |
| unordered_container_iter | 1 |

## File Overview

| File | Findings | Critical | High | Medium | Low |
|---|---:|---:|---:|---:|---:|
| chimera/themisdb_adapter.cpp | 48 | 10 | 21 | 17 | 0 |
| chimera/ARCHITECTURE.md | 1 | 0 | 0 | 0 | 1 |
| chimera/FUTURE_ENHANCEMENTS.md | 1 | 0 | 0 | 0 | 1 |
| chimera/PRODUCTION_REQUIREMENTS.md | 1 | 0 | 0 | 0 | 1 |

## Full Scanner Findings

### chimera/themisdb_adapter.cpp
Total findings: 48

- Line 120: severity=CRITICAL; category=missing_audit_log
  Description: Security function "execute_query" without audit log
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: Result<RelationalTable> ThemisDBAdapter::execute_query(
- Line 531: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: if (auto it = graph_nodes_.find(source_id);
- Line 581: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator eit may be invalidated by container modification
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: auto eit = via_edge.find(cur);
- Line 590: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator nit may be invalidated by container modification
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: auto nit = graph_nodes_.find(nid);
- Line 599: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator eit may be invalidated by container modification
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: auto eit = graph_edges_.find(eid);
- Line 636: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: auto it = graph_nodes_.find(nid);
- Line 697: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator nit may be invalidated by container modification
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: auto nit = graph_nodes_.find(cur_id);
- Line 1353: severity=CRITICAL; category=missing_audit_log
  Description: Security function "execute_query" without audit log
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: return execute_query(query, params);
- Line 1479: severity=CRITICAL; category=missing_audit_log
  Description: Security function "execute_query" without audit log
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: auto table_result = execute_query(query, params);
- Line 1654: severity=CRITICAL; category=missing_audit_log
  Description: Security function "execute_query" without audit log
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: auto result = adapter_->execute_query(effective_query, pos_params);
- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: * PR History (last 5): #4701 docs(chimera): migrate modu... (2026-04-16) | #4096 feat(chimera): Prod
- Line 85: severity=HIGH; category=no_retry_logic
  Description: RPC/network call without retry logic — transient failures will propagate
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: }



// Connection Management

Result<bool> ThemisDBAdapter::connect(

    const std::string& connection_string,

    const std::map<std::string, std::string>& /*options*/

) {
- Line 149: severity=HIGH; category=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: if (std::find(table.column_names.begin(),
- Line 149: severity=HIGH; category=repeated_search
  Description: find/search in loop — O(n²) or worse
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: if (std::find(table.column_names.begin(),
- Line 201: severity=HIGH; category=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: if (std::find(table.column_names.begin(),
- Line 201: severity=HIGH; category=repeated_search
  Description: find/search in loop — O(n²) or worse
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: if (std::find(table.column_names.begin(),
- Line 331: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: for (const auto& r : knn_results) {

            Vector v;

            // Return a placeholder vector carrying the PK as metadata.

            v.metadata["pk"] = Scalar{r.pk};

            results.emplace_back(std::move(v), static_cast<double>(r.distance));

        }

        return Result<std::vector<std::pair<Vector, double>>>::ok(
- Line 542: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: std::map<std::string, std::string> parent;   // node -> predecessor node
- Line 543: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: std::map<std::string, std::string> via_edge; // node -> edge_id used to reach it
- Line 580: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: auto eit = via_edge.find(cur);
- Line 589: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: auto nit = graph_nodes_.find(nid);
- Line 598: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: auto eit = graph_edges_.find(eid);
- Line 635: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: auto it = graph_nodes_.find(nid);
- Line 814: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: auto field_it = doc.fields.find(key);
- Line 815: severity=HIGH; category=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: auto field_it = doc.fields.find(key);
- Line 851: severity=HIGH; category=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: auto fld = doc.fields.find(key);
- Line 1194: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: return static_cast<bool>(connection_pool_acquire_fn_);
- Line 1218: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: if (connection_pool_acquire_fn_) {
- Line 1228: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: void ThemisDBAdapter::setConnectionPool(std::function<void*()> acquire_fn) {
- Line 1229: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: connection_pool_acquire_fn_ = std::move(acquire_fn);
- Line 1645: severity=HIGH; category=missing_trace_point
  Description: Critical function execute without trace point
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: Result<RelationalTable> ThemisDBPreparedStatement::execute() {
- Line 306: severity=MEDIUM; category=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: const std::map<std::string, Scalar>& filters
- Line 542: severity=MEDIUM; category=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: std::map<std::string, std::string> parent;   // node -> predecessor node
- Line 543: severity=MEDIUM; category=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: std::map<std::string, std::string> via_edge; // node -> edge_id used to reach it
- Line 629: severity=MEDIUM; category=cast_to_smaller_type
  Description: Explicit cast to int detected (verify no overflow on source)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::type_conversion
  Context: ['        //   • single label -> BFS filtered by that edge type', '        //   • multi-label  -> run one BFS per label and merge with deduplication', '        const int depth = static_cast<int>(max_depth);', '        std::unordered_set<std::string> seen_ids;', '        std::vector<GraphNode> nodes;']
- Line 630: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_set<std::string> seen_ids;
- Line 684: severity=MEDIUM; category=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: std::map<std::string, bool> visited;
- Line 793: severity=MEDIUM; category=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: const std::map<std::string, Scalar>& filter,
- Line 831: severity=MEDIUM; category=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: const std::map<std::string, Scalar>& filter,
- Line 832: severity=MEDIUM; category=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: const std::map<std::string, Scalar>& updates
- Line 1103: severity=MEDIUM; category=chimera_retry_duplication
  Description: Chimera retry logic should be centralized (avoid per-adapter retry implementations)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::themis_design_error_rules
  Context: stats.retry_count      = entry.retry_count;
- Line 1294: severity=MEDIUM; category=missing_move_constructor_defaulted
  Description: Missing move ctor prevents efficient rvalue transfers
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 1424: severity=MEDIUM; category=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: const std::map<std::string, Scalar>& filters,
- Line 1594: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: Result<bool> ThemisDBResultStream::close() {
- Line 1614: severity=MEDIUM; category=missing_latency_metric
  Description: No latency measurement for operation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: std::string ThemisDBPreparedStatement::get_query() const { return query_; }
- Line 1719: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: if (c == '\\') escaped += "\\\\";
- Line 1720: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: if (c == '\\') escaped += "\\\\";
- Line 1721: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: else if (c == '\'') escaped += "\\'";

### chimera/ARCHITECTURE.md
Total findings: 1

- Line 1: severity=LOW; category=missing_adr_reference
  Description: Architecture doc missing ADR references: adr_006
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::themis_module_governance_rules
  Context: Add explicit ADR links/references for module-critical design decisions

### chimera/FUTURE_ENHANCEMENTS.md
Total findings: 1

- Line 1: severity=LOW; category=module_doc_linkset_drift
  Description: Module doc 'FUTURE_ENHANCEMENTS.md' is missing expected cross-links: ARCHITECTURE.md
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::themis_doc_freshness_rules
  Context: Refresh the top-level <!-- Links: ... --> metadata to match the current module doc set

### chimera/PRODUCTION_REQUIREMENTS.md
Total findings: 1

- Line 1: severity=LOW; category=module_doc_linkset_drift
  Description: Module doc 'PRODUCTION_REQUIREMENTS.md' is missing expected cross-links: FUTURE_ENHANCEMENTS.md, README.md, ROADMAP.md
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::themis_doc_freshness_rules
  Context: Refresh the top-level <!-- Links: ... --> metadata to match the current module doc set

## Update Workflow

- Refresh scanner artifacts with: python tools/gs3_orchestrator.py ./src --output ai_working/gap_scan_results.json
- Regenerate docs with: python tools/module_doc_generator.py . ai_working ai_working/module_gaps
- Add --no-mirror when you only want archive docs in ai_working/module_gaps.

Format: THEMIS_MODULE_GAPS_V4
