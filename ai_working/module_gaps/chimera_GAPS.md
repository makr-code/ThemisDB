# chimera Module - Developer Gap Note

> Auto-generated from ai_working/gap_scan_v3_aggregate.json.
> This file is overwritten on each regeneration.

## Scan Snapshot

- Module: chimera
- Generated: 2026-06-02 11:55:47
- Status: Critical Findings Present
- Total Findings: 110
- Actionable Findings (Critical + High): 61
- Affected Files: 3

## Severity Summary

| Severity | Count |
|---|---:|
| Critical | 11 |
| High | 50 |
| Medium | 49 |
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
| src/chimera/themisdb_adapter.cpp | 89 | 11 | 33 | 45 | 0 |
| include/chimera/themisdb_adapter.hpp | 15 | 0 | 14 | 1 | 0 |
| include/chimera/database_adapter.hpp | 6 | 0 | 3 | 3 | 0 |

## Full Scanner Findings

### src/chimera/themisdb_adapter.cpp
Total findings: 89

- Line 120: severity=CRITICAL; category=audit_logging; pattern=missing_audit_log
  Description: Security function "execute_query" without audit log
  Context: Result<RelationalTable> ThemisDBAdapter::execute_query(
  Confidence: band=very_high; score=0.99
- Line 531: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: if (auto it = graph_nodes_.find(source_id);
- Line 581: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator eit may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto eit = via_edge.find(cur);
- Line 590: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator nit may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto nit = graph_nodes_.find(nid);
- Line 599: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator eit may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto eit = graph_edges_.find(eid);
- Line 636: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto it = graph_nodes_.find(nid);
- Line 697: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator nit may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto nit = graph_nodes_.find(cur_id);
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
- Line 1701: severity=CRITICAL; category=hardcoded_secret
  Description: hardcoded_secret: Hardcoded secret — use environment variable
  Remediation: Hardcoded secret — use environment variable
  Context: const std::string token = "@" + kv.first;
- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: * PR History (last 5): #4701 docs(chimera): migrate modu... (2026-04-16) | #4096 feat(chimera): Prod
- Line 85: severity=HIGH; category=no_retry_logic
  Description: socket_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: Result<bool> ThemisDBAdapter::connect(
- Line 109: severity=HIGH; category=no_retry_logic
  Description: socket_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: Result<bool> ThemisDBAdapter::disconnect() {
- Line 120: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: Result<RelationalTable> ThemisDBAdapter::execute_query(
- Line 149: severity=HIGH; category=performance; pattern=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Context: if (std::find(table.column_names.begin(),
  Confidence: band=very_high; score=0.9
- Line 149: severity=HIGH; category=repeated_search
  Description: find/search in loop — O(n²) or worse
  Remediation: Move search outside loop or build index/map before loop
  Context: if (std::find(table.column_names.begin(),
- Line 149: severity=HIGH; category=repeated_search
  Description: find/search in loop — O(n²) or worse
  Remediation: Move search outside loop or build index/map before loop
  Context: if (std::find(table.column_names.begin(),
- Line 201: severity=HIGH; category=performance; pattern=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Context: if (std::find(table.column_names.begin(),
  Confidence: band=very_high; score=0.9
- Line 201: severity=HIGH; category=repeated_search
  Description: find/search in loop — O(n²) or worse
  Remediation: Move search outside loop or build index/map before loop
  Context: if (std::find(table.column_names.begin(),
- Line 201: severity=HIGH; category=repeated_search
  Description: find/search in loop — O(n²) or worse
  Remediation: Move search outside loop or build index/map before loop
  Context: if (std::find(table.column_names.begin(),
- Line 542: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: std::map<std::string, std::string> parent;   // node -> predecessor node
- Line 543: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: std::map<std::string, std::string> via_edge; // node -> edge_id used to reach it
- Line 580: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: auto eit = via_edge.find(cur);
- Line 589: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: auto nit = graph_nodes_.find(nid);
- Line 598: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: auto eit = graph_edges_.find(eid);
- Line 635: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: auto it = graph_nodes_.find(nid);
- Line 730: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: Result<std::vector<GraphPath>> ThemisDBAdapter::execute_graph_query(
- Line 814: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: auto field_it = doc.fields.find(key);
- Line 815: severity=HIGH; category=performance; pattern=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Context: auto field_it = doc.fields.find(key);
  Confidence: band=very_high; score=0.9
- Line 851: severity=HIGH; category=performance; pattern=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Context: auto fld = doc.fields.find(key);
  Confidence: band=very_high; score=0.9
- Line 1194: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: return static_cast<bool>(connection_pool_acquire_fn_);
- Line 1218: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: if (connection_pool_acquire_fn_) {
- Line 1228: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: void ThemisDBAdapter::setConnectionPool(std::function<void*()> acquire_fn) {
- Line 1229: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: connection_pool_acquire_fn_ = std::move(acquire_fn);
- Line 1353: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: return execute_query(query, params);
- Line 1479: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: auto table_result = execute_query(query, params);
- Line 1506: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: Result<std::unique_ptr<IPreparedStatement>> ThemisDBAdapter::prepare(
- Line 1525: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: Result<bool> ThemisDBAdapter::unprepare(const std::string& statement_id) {
- Line 1614: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: std::string ThemisDBPreparedStatement::get_query() const { return query_; }
- Line 1645: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: Result<RelationalTable> ThemisDBPreparedStatement::execute() {
- Line 1645: severity=HIGH; category=observability; pattern=missing_trace_point
  Description: Critical function execute without trace point
  Context: Result<RelationalTable> ThemisDBPreparedStatement::execute() {
  Confidence: band=very_high; score=0.9
- Line 1654: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: auto result = adapter_->execute_query(effective_query, pos_params);
- Line 1668: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: return execute();
- Line 0: severity=MEDIUM; category=uncategorized
  Context: ['        //   • single label -> BFS filtered by that edge type', '        //   • multi-label  -> run one BFS per label and merge with deduplication', '        const int depth = static_cast<int>(max_depth);', '        std::unordered_set<std::string> seen_ids;', '        std::vector<GraphNode> nodes;']
  Confidence: band=medium; score=0.62
- Line 0: severity=MEDIUM; category=uncategorized
  Confidence: band=medium; score=0.57
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
- Line 481: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: GraphIndexManager::Status status;
- Line 494: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: ErrorCode::INTERNAL_ERROR, status.message);
- Line 504: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: path.nodes.push_back(std::move(node));
  Confidence: band=high; score=0.74
- Line 533: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: path.nodes.push_back(it->second);
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
- Line 601: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: path.edges.push_back(eit->second);
- Line 630: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> seen_ids;
  Confidence: band=medium; score=0.66
- Line 637: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: nodes.push_back(it->second);
  Confidence: band=high; score=0.74
- Line 638: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: nodes.push_back(it->second);
- Line 651: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: ErrorCode::INTERNAL_ERROR, status.message);
- Line 660: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: ErrorCode::INTERNAL_ERROR, status.message);
- Line 684: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: std::map<std::string, bool> visited;
  Confidence: band=high; score=0.74
- Line 699: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: visited_nodes.push_back(nit->second);
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
- Line 1594: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: Result<bool> ThemisDBResultStream::close() {
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
- Line 1720: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: if (c == '\\') escaped += "\\\\";
- Line 1720: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: if (c == '\\') escaped += "\\\\";
- Line 1721: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: else if (c == '\'') escaped += "\\'";
- Line 1721: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: else if (c == '\'') escaped += "\\'";

### include/chimera/themisdb_adapter.hpp
Total findings: 15

- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: * PR History (last 5): #4701 docs(chimera): migrate modu... (2026-04-16) | #4122 feat(chimera): asyn
- Line 121: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: std::string get_query() const override;
- Line 129: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: Result<RelationalTable> execute() override;
- Line 204: severity=HIGH; category=no_retry_logic
  Description: socket_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: Result<bool> connect(
- Line 209: severity=HIGH; category=no_retry_logic
  Description: socket_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: Result<bool> disconnect() override;
- Line 213: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: Result<RelationalTable> execute_query(
- Line 270: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: Result<std::vector<GraphPath>> execute_graph_query(
- Line 363: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: Result<std::unique_ptr<IPreparedStatement>> prepare(
- Line 367: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: Result<bool> unprepare(const std::string& statement_id) override;
- Line 374: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: * When a non-null `acquire_fn` is set, `has_capability(CONNECTION_POOLING)`
- Line 377: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: * The `acquire_fn` is a zero-argument callable returning a connection
- Line 381: severity=HIGH; category=no_retry_logic
  Description: socket_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: * Thread safety: call before the first `connect()` invocation.
- Line 383: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: void setConnectionPool(std::function<void*()> acquire_fn);
- Line 399: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: std::function<void*()> connection_pool_acquire_fn_;
- Line 85: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: Result<bool> close() override;

### include/chimera/database_adapter.hpp
Total findings: 6

- Line 94: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: virtual std::string get_query() const = 0;
- Line 129: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: virtual Result<std::unique_ptr<IPreparedStatement>> prepare(
- Line 134: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: virtual Result<bool> unprepare(const std::string& statement_id) = 0;
- Line 10: severity=MEDIUM; category=unportable_pragma
  Description: Non-standard pragma may not be supported on all platforms
  Remediation: Use standard C++ mechanisms: static_assert, or move to portable config
  Context: #pragma once
- Line 91: severity=MEDIUM; category=expensive_copy
  Description: Unnecessary expensive copy
  Remediation: Use const reference (const T&) or std::move if transfer is needed
  Context: virtual std::string get_id() const = 0;
- Line 94: severity=MEDIUM; category=expensive_copy
  Description: Unnecessary expensive copy
  Remediation: Use const reference (const T&) or std::move if transfer is needed
  Context: virtual std::string get_query() const = 0;

## Update Workflow

- Refresh scan artifacts with: python tools/gap_scanner_v3.py
- Regenerate all module notes with: python tools/module_doc_generator.py . ai_working ai_working/module_gaps
- The generator mirrors each archive document directly into src/<module>/MODULE_GAPS.md.

Format: THEMIS_MODULE_GAPS_V3
