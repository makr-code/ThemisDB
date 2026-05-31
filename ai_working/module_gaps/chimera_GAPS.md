# chimera Module - Developer Gap Note

> Auto-generated from ai_working/gap_scan_v3_aggregate.json.
> This file is overwritten on each regeneration.

## Scan Snapshot

- Module: chimera
- Generated: 2026-05-31 08:50:11
- Status: Critical Findings Present
- Total Findings: 126
- Actionable Findings (Critical + High): 64
- Affected Files: 3

## Severity Summary

| Severity | Count |
|---|---:|
| Critical | 16 |
| High | 48 |
| Medium | 62 |
| Low | 0 |

## Category Summary

| Category | Count |
|---|---:|
| performance_patterns | 34 |
| container | 32 |
| reliability | 28 |
| raii | 11 |
| performance | 10 |
| audit_logging | 4 |
| observability | 2 |
| determinism | 1 |
| exception_safety | 1 |
| platform | 1 |
| security | 1 |
| type_conversion | 1 |

## File Overview

| File | Findings | Critical | High | Medium | Low |
|---|---:|---:|---:|---:|---:|
| src/chimera/themisdb_adapter.cpp | 105 | 16 | 32 | 57 | 0 |
| include/chimera/themisdb_adapter.hpp | 14 | 0 | 13 | 1 | 0 |
| include/chimera/database_adapter.hpp | 7 | 0 | 3 | 4 | 0 |

## Full Scanner Findings

### src/chimera/themisdb_adapter.cpp
Total findings: 105

- Line 119: severity=CRITICAL; category=audit_logging; pattern=missing_audit_log
  Description: Security function "execute_query" without audit log
  Context: Result<RelationalTable> ThemisDBAdapter::execute_query(
  Confidence: band=very_high; score=0.99
- Line 196: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto it = table_store_.find(query); // treat query as a table name for simple scans
- Line 500: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto it = graph_nodes_.find(node_id);
- Line 530: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: if (auto it = graph_nodes_.find(source_id);
- Line 580: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator eit may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto eit = via_edge.find(cur);
- Line 589: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator nit may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto nit = graph_nodes_.find(nid);
- Line 598: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator eit may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto eit = graph_edges_.find(eid);
- Line 635: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto it = graph_nodes_.find(nid);
- Line 696: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator nit may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto nit = graph_nodes_.find(cur_id);
- Line 906: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto it = active_transactions_.find(transaction_id);
- Line 932: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto it = active_transactions_.find(transaction_id);
- Line 1352: severity=CRITICAL; category=audit_logging; pattern=missing_audit_log
  Description: Security function "execute_query" without audit log
  Context: return execute_query(query, params);
  Confidence: band=very_high; score=0.99
- Line 1478: severity=CRITICAL; category=audit_logging; pattern=missing_audit_log
  Description: Security function "execute_query" without audit log
  Context: auto table_result = execute_query(query, params);
  Confidence: band=very_high; score=0.99
- Line 1526: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto it = prepared_queries_.find(statement_id);
- Line 1653: severity=CRITICAL; category=audit_logging; pattern=missing_audit_log
  Description: Security function "execute_query" without audit log
  Context: auto result = adapter_->execute_query(effective_query, pos_params);
  Confidence: band=very_high; score=0.99
- Line 1700: severity=CRITICAL; category=hardcoded_secret
  Description: hardcoded_secret: Hardcoded secret — use environment variable
  Remediation: Hardcoded secret — use environment variable
  Context: const std::string token = "@" + kv.first;
- Line 84: severity=HIGH; category=no_retry_logic
  Description: socket_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: Result<bool> ThemisDBAdapter::connect(
- Line 108: severity=HIGH; category=no_retry_logic
  Description: socket_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: Result<bool> ThemisDBAdapter::disconnect() {
- Line 119: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: Result<RelationalTable> ThemisDBAdapter::execute_query(
- Line 148: severity=HIGH; category=performance; pattern=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Context: if (std::find(table.column_names.begin(),
  Confidence: band=very_high; score=0.9
- Line 148: severity=HIGH; category=repeated_search
  Description: find/search in loop — O(n²) or worse
  Remediation: Move search outside loop or build index/map before loop
  Context: if (std::find(table.column_names.begin(),
- Line 148: severity=HIGH; category=repeated_search
  Description: find/search in loop — O(n²) or worse
  Remediation: Move search outside loop or build index/map before loop
  Context: if (std::find(table.column_names.begin(),
- Line 200: severity=HIGH; category=performance; pattern=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Context: if (std::find(table.column_names.begin(),
  Confidence: band=very_high; score=0.9
- Line 200: severity=HIGH; category=repeated_search
  Description: find/search in loop — O(n²) or worse
  Remediation: Move search outside loop or build index/map before loop
  Context: if (std::find(table.column_names.begin(),
- Line 200: severity=HIGH; category=repeated_search
  Description: find/search in loop — O(n²) or worse
  Remediation: Move search outside loop or build index/map before loop
  Context: if (std::find(table.column_names.begin(),
- Line 396: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto& [idx, dist] : scored) {
  Confidence: band=very_high; score=0.9
- Line 579: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: auto eit = via_edge.find(cur);
- Line 588: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: auto nit = graph_nodes_.find(nid);
- Line 597: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: auto eit = graph_edges_.find(eid);
- Line 634: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: auto it = graph_nodes_.find(nid);
- Line 729: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: Result<std::vector<GraphPath>> ThemisDBAdapter::execute_graph_query(
- Line 813: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: auto field_it = doc.fields.find(key);
- Line 814: severity=HIGH; category=performance; pattern=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Context: auto field_it = doc.fields.find(key);
  Confidence: band=very_high; score=0.9
- Line 850: severity=HIGH; category=performance; pattern=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Context: auto fld = doc.fields.find(key);
  Confidence: band=very_high; score=0.9
- Line 857: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto& [ukey, uval] : updates) {
  Confidence: band=very_high; score=0.9
- Line 1193: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: return static_cast<bool>(connection_pool_acquire_fn_);
- Line 1217: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: if (connection_pool_acquire_fn_) {
- Line 1227: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: void ThemisDBAdapter::setConnectionPool(std::function<void*()> acquire_fn) {
- Line 1228: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: connection_pool_acquire_fn_ = std::move(acquire_fn);
- Line 1352: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: return execute_query(query, params);
- Line 1478: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: auto table_result = execute_query(query, params);
- Line 1505: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: Result<std::unique_ptr<IPreparedStatement>> ThemisDBAdapter::prepare(
- Line 1524: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: Result<bool> ThemisDBAdapter::unprepare(const std::string& statement_id) {
- Line 1613: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: std::string ThemisDBPreparedStatement::get_query() const { return query_; }
- Line 1644: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: Result<RelationalTable> ThemisDBPreparedStatement::execute() {
- Line 1644: severity=HIGH; category=observability; pattern=missing_trace_point
  Description: Critical function execute without trace point
  Context: Result<RelationalTable> ThemisDBPreparedStatement::execute() {
  Confidence: band=very_high; score=0.9
- Line 1653: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: auto result = adapter_->execute_query(effective_query, pos_params);
- Line 1667: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: return execute();
- Line 0: severity=MEDIUM; category=uncategorized
  Context: ['        //   • single label -> BFS filtered by that edge type', '        //   • multi-label  -> run one BFS per label and merge with deduplication', '        const int depth = static_cast<int>(max_depth);', '        std::unordered_set<std::string> seen_ids;', '        std::vector<GraphNode> nodes;']
  Confidence: band=medium; score=0.62
- Line 0: severity=MEDIUM; category=uncategorized
  Confidence: band=medium; score=0.57
- Line 150: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: table.column_names.push_back(col);
  Confidence: band=high; score=0.74
- Line 150: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: table.column_names.push_back(col);
  Confidence: band=high; score=0.74
- Line 151: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: table.column_names.push_back(col);
- Line 165: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: table.rows.push_back(std::move(row));
- Line 202: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: table.column_names.push_back(col);
  Confidence: band=high; score=0.74
- Line 202: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: table.column_names.push_back(col);
  Confidence: band=high; score=0.74
- Line 203: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: table.column_names.push_back(col);
- Line 206: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: table.rows.push_back(row);
- Line 294: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: store.emplace_back(generate_id(), v);
  Confidence: band=high; score=0.74
- Line 305: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: const std::map<std::string, Scalar>& filters
  Confidence: band=high; score=0.74
- Line 330: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results.emplace_back(std::move(v), static_cast<double>(r.distance));
  Confidence: band=high; score=0.74
- Line 380: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: scored.emplace_back(i, cos_dist);
  Confidence: band=high; score=0.74
- Line 380: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: scored.emplace_back(i, cos_dist);
  Confidence: band=high; score=0.74
- Line 396: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results.emplace_back(store[idx].second, dist);
  Confidence: band=high; score=0.74
- Line 480: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: GraphIndexManager::Status status;
- Line 493: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: ErrorCode::INTERNAL_ERROR, status.message);
- Line 503: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: path.nodes.push_back(std::move(node));
  Confidence: band=high; score=0.74
- Line 504: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: path.nodes.push_back(std::move(node));
- Line 532: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: path.nodes.push_back(it->second);
- Line 541: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: std::map<std::string, std::string> parent;   // node -> predecessor node
  Confidence: band=high; score=0.74
- Line 542: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: std::map<std::string, std::string> via_edge; // node -> edge_id used to reach it
  Confidence: band=high; score=0.74
- Line 578: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: node_seq.push_back(cur);
  Confidence: band=high; score=0.74
- Line 578: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: node_seq.push_back(cur);
  Confidence: band=high; score=0.74
- Line 579: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: node_seq.push_back(cur);
- Line 582: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: edge_seq.push_back(eit->second);
- Line 590: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: path.nodes.push_back(nit->second);
  Confidence: band=high; score=0.74
- Line 591: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: path.nodes.push_back(nit->second);
- Line 594: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: path.nodes.push_back(n);
- Line 599: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: path.edges.push_back(eit->second);
  Confidence: band=high; score=0.74
- Line 600: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: path.edges.push_back(eit->second);
- Line 629: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> seen_ids;
  Confidence: band=medium; score=0.66
- Line 636: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: nodes.push_back(it->second);
  Confidence: band=high; score=0.74
- Line 637: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: nodes.push_back(it->second);
- Line 640: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: nodes.push_back(std::move(n));
- Line 650: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: ErrorCode::INTERNAL_ERROR, status.message);
- Line 659: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: ErrorCode::INTERNAL_ERROR, status.message);
- Line 683: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: std::map<std::string, bool> visited;
  Confidence: band=high; score=0.74
- Line 698: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: visited_nodes.push_back(nit->second);
- Line 792: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: const std::map<std::string, Scalar>& filter,
  Confidence: band=high; score=0.74
- Line 821: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: matched.push_back(doc);
  Confidence: band=high; score=0.74
- Line 821: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: matched.push_back(doc);
  Confidence: band=high; score=0.74
- Line 822: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: matched.push_back(doc);
- Line 830: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: const std::map<std::string, Scalar>& filter,
  Confidence: band=high; score=0.74
- Line 831: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: const std::map<std::string, Scalar>& updates
  Confidence: band=high; score=0.74
- Line 1423: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: const std::map<std::string, Scalar>& filters,
  Confidence: band=high; score=0.74
- Line 1540: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: ids.push_back(kv.first);
  Confidence: band=high; score=0.74
- Line 1541: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: ids.push_back(kv.first);
- Line 1593: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: Result<bool> ThemisDBResultStream::close() {
- Line 1613: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: std::string ThemisDBPreparedStatement::get_query() const { return query_; }
  Confidence: band=high; score=0.74
- Line 1718: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: if (c == '\\') escaped += "\\\\";
  Confidence: band=high; score=0.74
- Line 1718: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: if (c == '\\') escaped += "\\\\";
  Confidence: band=high; score=0.74
- Line 1719: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: if (c == '\\') escaped += "\\\\";
- Line 1719: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: if (c == '\\') escaped += "\\\\";
- Line 1720: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: else if (c == '\'') escaped += "\\'";
- Line 1720: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: else if (c == '\'') escaped += "\\'";

### include/chimera/themisdb_adapter.hpp
Total findings: 14

- Line 120: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: std::string get_query() const override;
- Line 128: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: Result<RelationalTable> execute() override;
- Line 203: severity=HIGH; category=no_retry_logic
  Description: socket_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: Result<bool> connect(
- Line 208: severity=HIGH; category=no_retry_logic
  Description: socket_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: Result<bool> disconnect() override;
- Line 212: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: Result<RelationalTable> execute_query(
- Line 269: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: Result<std::vector<GraphPath>> execute_graph_query(
- Line 362: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: Result<std::unique_ptr<IPreparedStatement>> prepare(
- Line 366: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: Result<bool> unprepare(const std::string& statement_id) override;
- Line 373: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: * When a non-null `acquire_fn` is set, `has_capability(CONNECTION_POOLING)`
- Line 376: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: * The `acquire_fn` is a zero-argument callable returning a connection
- Line 380: severity=HIGH; category=no_retry_logic
  Description: socket_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: * Thread safety: call before the first `connect()` invocation.
- Line 382: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: void setConnectionPool(std::function<void*()> acquire_fn);
- Line 398: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: std::function<void*()> connection_pool_acquire_fn_;
- Line 84: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: Result<bool> close() override;

### include/chimera/database_adapter.hpp
Total findings: 7

- Line 93: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: virtual std::string get_query() const = 0;
- Line 128: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: virtual Result<std::unique_ptr<IPreparedStatement>> prepare(
- Line 133: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: virtual Result<bool> unprepare(const std::string& statement_id) = 0;
- Line 9: severity=MEDIUM; category=unportable_pragma
  Description: Non-standard pragma may not be supported on all platforms
  Remediation: Use standard C++ mechanisms: static_assert, or move to portable config
  Context: #pragma once
- Line 60: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: virtual Result<bool> close() = 0;
- Line 90: severity=MEDIUM; category=expensive_copy
  Description: Unnecessary expensive copy
  Remediation: Use const reference (const T&) or std::move if transfer is needed
  Context: virtual std::string get_id() const = 0;
- Line 93: severity=MEDIUM; category=expensive_copy
  Description: Unnecessary expensive copy
  Remediation: Use const reference (const T&) or std::move if transfer is needed
  Context: virtual std::string get_query() const = 0;

## Update Workflow

- Refresh scan artifacts with: python tools/gap_scanner_v3.py
- Regenerate all module notes with: python tools/module_doc_generator.py . ai_working ai_working/module_gaps
- The generator mirrors each archive document directly into src/<module>/MODULE_GAPS.md.

Format: THEMIS_MODULE_GAPS_V3
