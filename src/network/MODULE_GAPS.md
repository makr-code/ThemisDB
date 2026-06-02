# network Module - Developer Gap Note

> Auto-generated from ai_working/gap_scan_v3_aggregate.json.
> This file is overwritten on each regeneration.

## Scan Snapshot

- Module: network
- Generated: 2026-06-02 12:40:50
- Status: Critical Findings Present
- Total Findings: 528
- Actionable Findings (Critical + High): 357
- Affected Files: 24

## Severity Summary

| Severity | Count |
|---|---:|
| Critical | 30 |
| High | 327 |
| Medium | 171 |
| Low | 0 |

## Category Summary

| Category | Count |
|---|---:|
| reliability | 168 |
| raii | 83 |
| performance_patterns | 59 |
| exception_safety | 38 |
| container | 35 |
| platform | 28 |
| memory | 26 |
| performance | 23 |
| observability | 19 |
| concurrency | 15 |
| security | 12 |
| audit_logging | 11 |
| distributed_consistency | 7 |
| input_validation | 4 |
| uninitialized | 4 |
| legacy_duplication | 3 |
| determinism | 2 |
| type_conversion | 1 |

## File Overview

| File | Findings | Critical | High | Medium | Low |
|---|---:|---:|---:|---:|---:|
| src/network/wire_protocol_server.cpp | 167 | 6 | 121 | 40 | 0 |
| src/network/envoy_xds.cpp | 47 | 0 | 23 | 24 | 0 |
| src/network/quic_server.cpp | 37 | 2 | 21 | 14 | 0 |
| src/network/wire_protocol_connection_pool.cpp | 30 | 1 | 21 | 8 | 0 |
| src/network/raft_load_balancer.cpp | 29 | 0 | 19 | 10 | 0 |
| src/network/kernel_bypass.cpp | 28 | 3 | 10 | 15 | 0 |
| src/network/quic_transport.cpp | 27 | 2 | 18 | 7 | 0 |
| src/network/wire_protocol_server_ws.cpp | 20 | 0 | 18 | 2 | 0 |
| src/network/socket_timeout_manager.cpp | 16 | 2 | 13 | 1 | 0 |
| src/network/wire_protocol_v2.cpp | 16 | 1 | 5 | 10 | 0 |
| src/network/qos_manager.cpp | 13 | 0 | 10 | 3 | 0 |
| src/network/service_mesh.cpp | 13 | 3 | 3 | 7 | 0 |
| src/network/wire_protocol_zero_copy.cpp | 12 | 2 | 5 | 5 | 0 |
| src/network/adaptive_circuit_breaker.cpp | 10 | 0 | 10 | 0 | 0 |
| src/network/network_audit_log.cpp | 10 | 4 | 3 | 3 | 0 |
| src/network/udp_server.cpp | 9 | 0 | 5 | 4 | 0 |
| src/network/udp_fast_path.cpp | 8 | 0 | 5 | 3 | 0 |
| src/network/wire_protocol_helpers.cpp | 8 | 0 | 2 | 6 | 0 |
| src/network/connection_compression.cpp | 7 | 1 | 6 | 0 | 0 |
| src/network/io_uring_batcher.cpp | 7 | 1 | 5 | 1 | 0 |
| src/network/grpc_transport.cpp | 6 | 0 | 3 | 3 | 0 |
| src/network/wire_protocol_performance.cpp | 6 | 1 | 1 | 4 | 0 |
| src/network/geo_topology_router.cpp | 1 | 1 | 0 | 0 | 0 |
| src/network/wire_protocol_batch.cpp | 1 | 0 | 0 | 1 | 0 |

## Full Scanner Findings

### src/network/wire_protocol_server.cpp
Total findings: 167

- Line 344: severity=CRITICAL; category=audit_logging; pattern=sensitive_data_logging
  Description: Potential PII/credential logging: auth
  Context: std::cerr << "[WireProtocol] Invalid configuration: auth_token length exceeds AUTH "
  Confidence: band=very_high; score=0.92
- Line 424: severity=CRITICAL; category=no_timeout
  Description: file_io without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: acceptor_->open(endpoint.protocol());
- Line 476: severity=CRITICAL; category=no_timeout
  Description: semaphore_wait without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: worker_pool_->wait();
- Line 480: severity=CRITICAL; category=no_timeout
  Description: semaphore_wait without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: void WireProtocolServer::wait() {
- Line 1371: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: const auto& stored = server_->config_.auth_token;
- Line 3087: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto agg_result = server_->ts_store_->aggregate(query_opts);
- Line 537: severity=HIGH; category=memory_order
  Description: memory_order_relaxed used — potential visibility issue
  Remediation: Use memory_order_acquire/release unless truly lock-free
  Context: active_connection_count_.load(std::memory_order_relaxed) >= config_.max_connections) {
- Line 624: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: if (!running_.load(std::memory_order_acquire)) return;
- Line 628: severity=HIGH; category=no_retry_logic
  Description: socket_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: tcp::socket(*io_context_),
- Line 631: severity=HIGH; category=no_retry_logic
  Description: socket_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: acceptor_->async_accept(
- Line 796: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Use <cstdint> types (uint32_t, uint64_t) and sizeof() checks, not constants
  Context: std::memcpy(&flags, &header_buffer_[6], sizeof(uint16_t));
- Line 803: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Use <cstdint> types (uint32_t, uint64_t) and sizeof() checks, not constants
  Context: std::memcpy(&payload_size, &header_buffer_[8], sizeof(uint32_t));
- Line 890: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Use <cstdint> types (uint32_t, uint64_t) and sizeof() checks, not constants
  Context: std::memcpy(&flags, &header_buffer_[6], sizeof(uint16_t));
- Line 895: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Use <cstdint> types (uint32_t, uint64_t) and sizeof() checks, not constants
  Context: std::memcpy(&payload_size, &header_buffer_[8], sizeof(uint32_t));
- Line 928: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: auto mutable_buf = buf->prepare(4);
- Line 1341: severity=HIGH; category=no_retry_logic
  Description: http_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: if (!request.is_object()) {
- Line 1345: severity=HIGH; category=no_retry_logic
  Description: http_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: if (request.contains("token") && !request["token"].is_string()) {
- Line 1349: severity=HIGH; category=no_retry_logic
  Description: http_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: if (request.contains("username") && !request["username"].is_string()) {
- Line 1354: severity=HIGH; category=no_retry_logic
  Description: http_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: token = request.value("token", "");
- Line 1355: severity=HIGH; category=no_retry_logic
  Description: http_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: username_req = request.value("username", "");
- Line 1433: severity=HIGH; category=no_retry_logic
  Description: http_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: if (!request.is_object()) {
- Line 1437: severity=HIGH; category=no_retry_logic
  Description: http_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: if ((request.contains("collection") && !request["collection"].is_string()) ||
- Line 1438: severity=HIGH; category=no_retry_logic
  Description: http_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: (request.contains("key") && !request["key"].is_string())) {
- Line 1442: severity=HIGH; category=no_retry_logic
  Description: http_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: std::string collection = request.value("collection", "");
- Line 1443: severity=HIGH; category=no_retry_logic
  Description: http_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: std::string key = request.value("key", "");
- Line 1506: severity=HIGH; category=no_retry_logic
  Description: http_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: if (!request.is_object()) {
- Line 1510: severity=HIGH; category=no_retry_logic
  Description: http_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: if ((request.contains("collection") && !request["collection"].is_string()) ||
- Line 1511: severity=HIGH; category=no_retry_logic
  Description: http_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: (request.contains("key") && !request["key"].is_string())) {
- Line 1515: severity=HIGH; category=no_retry_logic
  Description: http_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: std::string collection = request.value("collection", "");
- Line 1516: severity=HIGH; category=no_retry_logic
  Description: http_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: std::string key = request.value("key", "");
- Line 1576: severity=HIGH; category=no_retry_logic
  Description: http_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: if (!request.is_object()) {
- Line 1580: severity=HIGH; category=no_retry_logic
  Description: http_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: if ((request.contains("collection") && !request["collection"].is_string()) ||
- Line 1581: severity=HIGH; category=no_retry_logic
  Description: http_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: (request.contains("key") && !request["key"].is_string())) {
- Line 1585: severity=HIGH; category=no_retry_logic
  Description: http_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: std::string collection = request.value("collection", "");
- Line 1586: severity=HIGH; category=no_retry_logic
  Description: http_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: std::string key = request.value("key", "");
- Line 1634: severity=HIGH; category=no_retry_logic
  Description: http_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: if (!request.is_object()) {
- Line 1638: severity=HIGH; category=no_retry_logic
  Description: http_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: if (request.contains("collection") && !request["collection"].is_string()) {
- Line 1642: severity=HIGH; category=no_retry_logic
  Description: http_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: std::string collection = request.value("collection", "");
- Line 1652: severity=HIGH; category=no_retry_logic
  Description: http_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: if (!request.contains("keys") || !request["keys"].is_array()) {
- Line 1751: severity=HIGH; category=no_retry_logic
  Description: http_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: if (!request.is_object()) {
- Line 1755: severity=HIGH; category=no_retry_logic
  Description: http_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: if (request.contains("collection") && !request["collection"].is_string()) {
- Line 1759: severity=HIGH; category=no_retry_logic
  Description: http_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: std::string collection = request.value("collection", "");
- Line 1769: severity=HIGH; category=no_retry_logic
  Description: http_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: if (!request.contains("items") || !request["items"].is_array()) {
- Line 1910: severity=HIGH; category=no_retry_logic
  Description: http_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: if (!request.is_object()) {
- Line 1914: severity=HIGH; category=no_retry_logic
  Description: http_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: if (request.contains("isolation_level") && !request["isolation_level"].is_string()) {
- Line 1918: severity=HIGH; category=no_retry_logic
  Description: http_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: if (request.contains("timeout_ms") && !request["timeout_ms"].is_number_integer()) {
- Line 1922: severity=HIGH; category=no_retry_logic
  Description: http_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: if (request.contains("timeout_ms")) {
- Line 1931: severity=HIGH; category=no_retry_logic
  Description: http_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: std::string isolation_str = request.value("isolation_level", "read_committed");
- Line 1981: severity=HIGH; category=no_retry_logic
  Description: http_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: if (!request.is_object()) {
- Line 1985: severity=HIGH; category=no_retry_logic
  Description: http_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: if (request.contains("transaction_id") && !request["transaction_id"].is_string()) {
- Line 1989: severity=HIGH; category=no_retry_logic
  Description: http_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: std::string tx_id_str = request.value("transaction_id", "");
- Line 2048: severity=HIGH; category=no_retry_logic
  Description: http_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: if (!request.is_object()) {
- Line 2052: severity=HIGH; category=no_retry_logic
  Description: http_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: if (request.contains("transaction_id") && !request["transaction_id"].is_string()) {
- Line 2056: severity=HIGH; category=no_retry_logic
  Description: http_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: std::string tx_id_str = request.value("transaction_id", "");
- Line 2111: severity=HIGH; category=no_retry_logic
  Description: http_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: if (!request.is_object()) {
- Line 2115: severity=HIGH; category=no_retry_logic
  Description: http_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: if ((request.contains("collection") && !request["collection"].is_string()) ||
- Line 2116: severity=HIGH; category=no_retry_logic
  Description: http_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: (request.contains("start_vertex") && !request["start_vertex"].is_string())) {
- Line 2120: severity=HIGH; category=no_retry_logic
  Description: http_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: if ((request.contains("direction") && !request["direction"].is_string()) ||
- Line 2121: severity=HIGH; category=no_retry_logic
  Description: http_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: (request.contains("edge_type") && !request["edge_type"].is_string())) {
- Line 2125: severity=HIGH; category=no_retry_logic
  Description: http_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: if ((request.contains("depth_min") && !request["depth_min"].is_number_integer()) ||
- Line 2126: severity=HIGH; category=no_retry_logic
  Description: http_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: (request.contains("depth_max") && !request["depth_max"].is_number_integer()) ||
- Line 2127: severity=HIGH; category=no_retry_logic
  Description: http_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: (request.contains("limit") && !request["limit"].is_number_integer())) {
- Line 2131: severity=HIGH; category=no_retry_logic
  Description: http_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: std::string collection = request.value("collection", "");
- Line 2132: severity=HIGH; category=no_retry_logic
  Description: http_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: std::string start_vertex = request.value("start_vertex", "");
- Line 2162: severity=HIGH; category=no_retry_logic
  Description: http_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: std::string direction_str = request.value("direction", "outbound");
- Line 2163: severity=HIGH; category=no_retry_logic
  Description: http_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: int depth_min = request.value("depth_min", 1);
- Line 2164: severity=HIGH; category=no_retry_logic
  Description: http_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: int depth_max = request.value("depth_max", 3);
- Line 2165: severity=HIGH; category=no_retry_logic
  Description: http_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: int limit     = request.value("limit", 100);
- Line 2166: severity=HIGH; category=no_retry_logic
  Description: http_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: std::string edge_type = request.value("edge_type", "");
- Line 2248: severity=HIGH; category=no_retry_logic
  Description: http_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: if (!request.is_object()) {
- Line 2252: severity=HIGH; category=no_retry_logic
  Description: http_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: if (request.contains("query") && !request["query"].is_string()) {
- Line 2256: severity=HIGH; category=no_retry_logic
  Description: http_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: if (request.contains("batch_size") && !request["batch_size"].is_number_integer()) {
- Line 2260: severity=HIGH; category=no_retry_logic
  Description: http_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: std::string query_str = request.value("query", "");
- Line 2295: severity=HIGH; category=no_retry_logic
  Description: http_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: int batch_size_i = request.value("batch_size", 100);
- Line 2374: severity=HIGH; category=no_retry_logic
  Description: http_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: if (!request.is_object()) {
- Line 2378: severity=HIGH; category=no_retry_logic
  Description: http_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: if (request.contains("cursor_id") && !request["cursor_id"].is_string()) {
- Line 2382: severity=HIGH; category=no_retry_logic
  Description: http_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: if (request.contains("batch_size") && !request["batch_size"].is_number_integer()) {
- Line 2386: severity=HIGH; category=no_retry_logic
  Description: http_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: std::string cursor_id = request.value("cursor_id", "");
- Line 2397: severity=HIGH; category=no_retry_logic
  Description: http_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: int batch_size_i = request.value("batch_size", 100);
- Line 2468: severity=HIGH; category=no_retry_logic
  Description: http_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: if (!request.is_object()) {
- Line 2472: severity=HIGH; category=no_retry_logic
  Description: http_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: if (request.contains("cursor_id") && !request["cursor_id"].is_string()) {
- Line 2476: severity=HIGH; category=no_retry_logic
  Description: http_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: std::string cursor_id = request.value("cursor_id", "");
- Line 2522: severity=HIGH; category=no_retry_logic
  Description: http_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: if (request.contains("k") && !request["k"].is_number_integer()) {
- Line 2527: severity=HIGH; category=no_retry_logic
  Description: http_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: if (request.contains("collection") && !request["collection"].is_string()) {
- Line 2532: severity=HIGH; category=no_retry_logic
  Description: http_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: const std::string collection = request.value("collection", "");
- Line 2538: severity=HIGH; category=no_retry_logic
  Description: http_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: if (!request.contains("vector") || !request["vector"].is_array()) {
- Line 2570: severity=HIGH; category=no_retry_logic
  Description: http_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: int64_t k_i = request.value("k", static_cast<int64_t>(10));
- Line 2627: severity=HIGH; category=no_retry_logic
  Description: http_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: if (request.contains("collection") && !request["collection"].is_string()) {
- Line 2631: severity=HIGH; category=no_retry_logic
  Description: http_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: if (request.contains("type") && !request["type"].is_string()) {
- Line 2635: severity=HIGH; category=no_retry_logic
  Description: http_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: if (request.contains("limit") && !request["limit"].is_number_integer()) {
- Line 2640: severity=HIGH; category=no_retry_logic
  Description: http_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: std::string collection = request.value("collection", "");
- Line 2650: severity=HIGH; category=no_retry_logic
  Description: http_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: std::string query_type = request.value("type", "");
- Line 2667: severity=HIGH; category=no_retry_logic
  Description: http_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: if (!request.contains("center") || !request["center"].is_object()) {
- Line 2673: severity=HIGH; category=no_retry_logic
  Description: http_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: if (!request.contains("radius")) {
- Line 2706: severity=HIGH; category=no_retry_logic
  Description: http_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: int64_t limit_i = request.value("limit", static_cast<int64_t>(100));
- Line 2757: severity=HIGH; category=no_retry_logic
  Description: http_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: int64_t limit_i = request.value("limit", static_cast<int64_t>(100));
- Line 2767: severity=HIGH; category=no_retry_logic
  Description: http_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: if (!request.contains("bbox") || !request["bbox"].is_object()) {
- Line 2801: severity=HIGH; category=no_retry_logic
  Description: http_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: if (!request.contains("center") || !request["center"].is_object()) {
- Line 2807: severity=HIGH; category=no_retry_logic
  Description: http_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: if (!request.contains("radius")) {
- Line 2913: severity=HIGH; category=no_retry_logic
  Description: http_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: if (request.collection.empty()) {
- Line 2995: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: auto result = server_->ts_store_->query(query_opts);
- Line 3032: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: bucket_data[bucket_start_ms].push_back(point.value);
- Line 3036: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: for (const auto& [bucket_start_ms, values] : bucket_data) {
- Line 3138: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: auto result = server_->ts_store_->query(query_opts);
- Line 3239: severity=HIGH; category=no_retry_logic
  Description: http_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: if (!request.is_object()) {
- Line 3244: severity=HIGH; category=no_retry_logic
  Description: http_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: if (request.contains("process_definition_key") && !request["process_definition_key"].is_string()) {
- Line 3248: severity=HIGH; category=no_retry_logic
  Description: http_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: if (request.contains("variables") && !request["variables"].is_object()) {
- Line 3252: severity=HIGH; category=no_retry_logic
  Description: http_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: if (request.contains("business_key") && !request["business_key"].is_string()) {
- Line 3257: severity=HIGH; category=no_retry_logic
  Description: http_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: std::string process_key = request.value("process_definition_key", "");
- Line 3258: severity=HIGH; category=no_retry_logic
  Description: http_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: json variables = request.value("variables", json::object());
- Line 3259: severity=HIGH; category=no_retry_logic
  Description: http_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: std::string business_key = request.value("business_key", "");
- Line 3374: severity=HIGH; category=no_retry_logic
  Description: http_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: if (!request.is_object()) {
- Line 3379: severity=HIGH; category=no_retry_logic
  Description: http_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: if (request.contains("task_id") && !request["task_id"].is_string()) {
- Line 3383: severity=HIGH; category=no_retry_logic
  Description: http_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: if (request.contains("variables") && !request["variables"].is_object()) {
- Line 3387: severity=HIGH; category=no_retry_logic
  Description: http_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: if (request.contains("assignee") && !request["assignee"].is_string()) {
- Line 3392: severity=HIGH; category=no_retry_logic
  Description: http_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: std::string task_id = request.value("task_id", "");
- Line 3393: severity=HIGH; category=no_retry_logic
  Description: http_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: json variables = request.value("variables", json::object());
- Line 3394: severity=HIGH; category=no_retry_logic
  Description: http_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: std::string assignee = request.value("assignee", username_);
- Line 3513: severity=HIGH; category=no_retry_logic
  Description: http_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: if (!request.is_object()) {
- Line 3518: severity=HIGH; category=no_retry_logic
  Description: http_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: if (request.contains("process_instance_id") && !request["process_instance_id"].is_string()) {
- Line 3522: severity=HIGH; category=no_retry_logic
  Description: http_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: if (request.contains("include_variables") && !request["include_variables"].is_boolean()) {
- Line 3526: severity=HIGH; category=no_retry_logic
  Description: http_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: if (request.contains("include_history") && !request["include_history"].is_boolean()) {
- Line 3530: severity=HIGH; category=no_retry_logic
  Description: http_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: if (request.contains("max_history_events") && !request["max_history_events"].is_number_unsigned()) {
- Line 3535: severity=HIGH; category=no_retry_logic
  Description: http_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: std::string instance_id = request.value("process_instance_id", "");
- Line 3536: severity=HIGH; category=no_retry_logic
  Description: http_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: bool include_variables = request.value("include_variables", true);
- Line 3537: severity=HIGH; category=no_retry_logic
  Description: http_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: bool include_history = request.value("include_history", false);
- Line 3538: severity=HIGH; category=no_retry_logic
  Description: http_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: std::size_t max_history_events = request.value("max_history_events", kDefaultBpmnHistoryEvents);
- Line 85: severity=MEDIUM; category=legacy_duplication; pattern=duplicate_qualified_signature
  Description: Potential duplicate implementation signature across files: std::all_of(value.begin(), value.end(), [](unsigned char ch)
  Context: return std::all_of(value.begin(), value.end(), [](unsigned char ch) {
  Confidence: band=medium; score=0.56
- Line 239: severity=MEDIUM; category=observability; pattern=missing_health_check
  Description: Service initialization without nearby health/status handling
  Context: void WireProtocolServer::start() {
  Confidence: band=medium; score=0.66
- Line 447: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: io_threads_.emplace_back([this]() {
  Confidence: band=high; score=0.74
- Line 452: severity=MEDIUM; category=expensive_inner_op
  Description: I/O operation in inner loop — very expensive
  Remediation: Buffer output or move I/O outside loop
  Context: std::cerr << "[WireProtocol] IO thread error: " << e.what() << std::endl;
- Line 464: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: acceptor_->close();
- Line 709: severity=MEDIUM; category=observability; pattern=missing_health_check
  Description: Service initialization without nearby health/status handling
  Context: , client_ip_("unknown")  // Will be set after accept in start()
  Confidence: band=medium; score=0.66
- Line 717: severity=MEDIUM; category=observability; pattern=missing_health_check
  Description: Service initialization without nearby health/status handling
  Context: void WireProtocolServer::Session::start() {
  Confidence: band=medium; score=0.66
- Line 740: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: void WireProtocolServer::Session::close() {
- Line 790: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: socket_.close();
- Line 884: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: socket_.close();
- Line 1218: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: close();
- Line 1225: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: close();
- Line 1686: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: client_keys.push_back(key);
  Confidence: band=high; score=0.74
- Line 1712: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results.push_back(std::move(item));
  Confidence: band=high; score=0.74
- Line 1802: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results.push_back(std::move(r));
  Confidence: band=high; score=0.74
- Line 1871: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results.push_back(std::move(item_result));
  Confidence: band=high; score=0.74
- Line 1871: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results.push_back(std::move(item_result));
  Confidence: band=high; score=0.74
- Line 1923: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: const auto timeout_ms = request["timeout_ms"].get<int64_t>();
  Confidence: band=high; score=0.74
- Line 2210: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: vertices.push_back(std::move(v));
  Confidence: band=high; score=0.74
- Line 2231: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: void WireProtocolServer::Session::handleQuery() {
  Confidence: band=high; score=0.74
- Line 2310: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: first_batch.push_back(result_json[i]);
  Confidence: band=high; score=0.74
- Line 2311: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: first_batch.push_back(result_json[i]);
- Line 2428: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: batch.push_back(entry.results[i]);
  Confidence: band=high; score=0.74
- Line 2429: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: batch.push_back(entry.results[i]);
- Line 2561: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: query_vector.push_back(value);
  Confidence: band=high; score=0.74
- Line 2596: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: hits.push_back(std::move(hit));
  Confidence: band=high; score=0.74
- Line 2612: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: void WireProtocolServer::Session::handleGeoQuery() {
  Confidence: band=high; score=0.74
- Line 2873: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results_arr.push_back(std::move(entry));
  Confidence: band=high; score=0.74
- Line 2893: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: void WireProtocolServer::Session::handleTimeseriesQuery() {
  Confidence: band=high; score=0.74
- Line 3031: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: bucket_data[bucket_start_ms].push_back(point.value);
  Confidence: band=high; score=0.74
- Line 3032: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: bucket_data[bucket_start_ms].push_back(point.value);
- Line 3162: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: response.buckets.push_back(bucket);
  Confidence: band=high; score=0.74
- Line 3183: severity=MEDIUM; category=endianness_assumption
  Description: Endianness conversion may have undefined behavior
  Remediation: Document endianness assumptions or use portable serialization
  Context: uint32_t magic = htonl(0x544D4442);  // "TMDB" in network byte order
- Line 3192: severity=MEDIUM; category=endianness_assumption
  Description: Endianness conversion may have undefined behavior
  Remediation: Document endianness assumptions or use portable serialization
  Context: uint32_t payload_size_net = htonl(payload_size);  // Convert to network byte order
- Line 3218: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: void WireProtocolServer::Session::handleBpmnStartProcess() {
  Confidence: band=high; score=0.74
- Line 3329: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: active_tasks.push_back(instance_id + ":" + token.current_node);
  Confidence: band=high; score=0.74
- Line 3330: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: active_tasks.push_back(instance_id + ":" + token.current_node);
- Line 3597: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: active_tasks.push_back(task);
  Confidence: band=high; score=0.74
- Line 3631: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: history.push_back(event);
  Confidence: band=high; score=0.74
- Line 3631: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: history.push_back(event);
  Confidence: band=high; score=0.74

### src/network/envoy_xds.cpp
Total findings: 47

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
- Line 55: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Use <cstdint> types (uint32_t, uint64_t) and sizeof() checks, not constants
  Context: std::snprintf(buf, sizeof(buf), "\\u%04x", c);
- Line 380: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: for (const auto& item : splitJsonArray(body)) {
- Line 415: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: for (const auto& item : splitJsonArray(body)) {
- Line 428: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: for (const auto& locality_ep : splitJsonArray(eps_body)) {
- Line 432: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: for (const auto& lbep : splitJsonArray(lb_body)) {
- Line 472: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: for (const auto& item : splitJsonArray(body)) {
- Line 479: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: for (const auto& locality_ep : splitJsonArray(eps_body)) {
- Line 483: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: for (const auto& lbep : splitJsonArray(lb_body)) {
- Line 522: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: for (const auto& rc : splitJsonArray(body)) {
- Line 528: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: for (const auto& vh : splitJsonArray(vhosts_body)) {
- Line 538: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: const auto q1 = dom_body.find('"', pos);
- Line 539: severity=HIGH; category=performance; pattern=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Context: const auto q1 = dom_body.find('"', pos);
  Confidence: band=very_high; score=0.9
- Line 559: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: for (const auto& rt : splitJsonArray(routes_body)) {
- Line 623: severity=HIGH; category=no_retry_logic
  Description: socket_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: stream.connect(results);
- Line 715: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: while (running_.load(std::memory_order_acquire)) {
- Line 828: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: wake_cv_.wait_for(lk, std::chrono::milliseconds(wait_ms),
- Line 829: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: [this] { return !running_.load(std::memory_order_acquire); });
- Line 46: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: case '"':  out += "\\\""; break;
  Confidence: band=high; score=0.74
- Line 47: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: case '"':  out += "\\\""; break;
- Line 48: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: case '\\': out += "\\\\"; break;
- Line 49: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: case '\n': out += "\\n";  break;
- Line 50: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: case '\r': out += "\\r";  break;
- Line 51: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: case '\t': out += "\\t";  break;
- Line 55: severity=MEDIUM; category=expensive_inner_op
  Description: I/O operation in inner loop — very expensive
  Remediation: Buffer output or move I/O outside loop
  Context: std::snprintf(buf, sizeof(buf), "\\u%04x", c);
- Line 79: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: case '"': value += '"'; break;
  Confidence: band=high; score=0.74
- Line 80: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: case '"': value += '"'; break;
- Line 81: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: case '\\': value += '\\'; break;
- Line 82: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: case 'n': value += '\n'; break;
- Line 83: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: case 'r': value += '\r'; break;
- Line 84: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: case 't': value += '\t'; break;
- Line 167: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: items.push_back(array_body.substr(item_start, i - item_start + 1));
  Confidence: band=high; score=0.74
- Line 168: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: items.push_back(array_body.substr(item_start, i - item_start + 1));
- Line 182: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {}
- Line 192: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {}
- Line 396: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(std::move(info));
  Confidence: band=high; score=0.74
- Line 445: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: info.endpoints.push_back(std::move(ep));
  Confidence: band=high; score=0.74
- Line 445: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: info.endpoints.push_back(std::move(ep));
  Confidence: band=high; score=0.74
- Line 496: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: info.endpoints.push_back(std::move(ep));
  Confidence: band=high; score=0.74
- Line 496: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: info.endpoints.push_back(std::move(ep));
  Confidence: band=high; score=0.74
- Line 549: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: if (!domain.empty()) info.domains.push_back(domain);
  Confidence: band=high; score=0.74
- Line 587: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {}

### src/network/quic_server.cpp
Total findings: 37

- Line 93: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: if (RAND_bytes(cid->data, static_cast<int>(cid->datalen)) != 1) {
- Line 564: severity=CRITICAL; category=smart_ptr_misuse
  Description: Raw new without immediate wrapping in smart pointer
  Remediation: Use auto ptr = std::make_unique<T>(...);
  Context: THEMIS_INFO("[QUICServer] new QUIC connection from {}", key);
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 174: severity=HIGH; category=legacy_duplication; pattern=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Context: // Advertise HTTP/3 ALPN ("h3") for compatibility with standard HTTP/3
  Confidence: band=high; score=0.8
- Line 237: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: if (running_.load(std::memory_order_acquire)) {
- Line 283: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: threads_.emplace_back([this] { io_ctx_->run(); });
- Line 311: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: io_ctx_->stop();
- Line 319: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: io_ctx_->restart();
- Line 356: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: if (running_.load(std::memory_order_acquire)) {
- Line 372: severity=HIGH; category=deadlock_risk
  Description: Multiple locks acquired in nested scope — potential deadlock
  Remediation: Use std::scoped_lock(m1, m2) or enforce consistent lock ordering
  Context: std::lock_guard<std::mutex> lk(sessions_mutex_);
- Line 452: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: void* /*user_data*/) -> int {
- Line 457: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: callbacks.recv_crypto_data = [](ngtcp2_conn*            conn,
- Line 462: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: void*                   /*user_data*/) -> int {
- Line 477: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: callbacks.client_initial = [](ngtcp2_conn* /*conn*/, void* /*user_data*/) -> int {
- Line 482: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: void*         user_data) -> int {
- Line 602: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: if (open_.load(std::memory_order_acquire)) {
- Line 608: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: if (!open_.load(std::memory_order_acquire)) {
- Line 652: severity=HIGH; category=no_retry_logic
  Description: socket_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: disconnect();
- Line 699: severity=HIGH; category=no_retry_logic
  Description: socket_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: void QUICClient::connect() {
- Line 844: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: if (!connected_.load(std::memory_order_acquire)) {
- Line 177: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: "\x02h3"          // "h3"   (length 2)
- Line 178: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: "\x04tmdb";       // "tmdb" (length 4)
- Line 236: severity=MEDIUM; category=observability; pattern=missing_health_check
  Description: Service initialization without nearby health/status handling
  Context: void QUICServer::start() {
  Confidence: band=medium; score=0.66
- Line 282: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: threads_.emplace_back([this] { io_ctx_->run(); });
  Confidence: band=high; score=0.74
- Line 299: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: SSL_free(static_cast<SSL*>(tls));
- Line 309: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: socket_->close(ec);
- Line 322: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: SSL_CTX_free(ssl_ctx_);
- Line 389: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: SSL_free(static_cast<SSL*>(tls));
- Line 540: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: SSL_free(ssl);
- Line 554: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: SSL_free(ssl);
- Line 802: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: ssl_ctx_ = ssl_ctx_guard.release();
- Line 803: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: (void)ssl_guard.release();
- Line 829: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: SSL_free(static_cast<SSL*>(tls));
- Line 836: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: SSL_CTX_free(ssl_ctx_);

### src/network/wire_protocol_connection_pool.cpp
Total findings: 30

- Line 410: severity=CRITICAL; category=no_timeout
  Description: mutex_lock without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: lock.lock();
- Line 134: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: while (!shutdown_.load(std::memory_order_acquire)) {
- Line 136: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Acquire lock before loop or redesign to minimize lock time
  Context: std::unique_lock<std::mutex> lock(shutdown_mutex_);
- Line 137: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: if (shutdown_cv_.wait_for(lock, std::chrono::seconds(10),
- Line 260: severity=HIGH; category=no_retry_logic
  Description: socket_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: net::async_connect(*plain_socket, endpoints,
- Line 364: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: WireProtocolConnectionPool::acquireConnection(const std::string& target) {
- Line 368: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: auto deadline = std::chrono::steady_clock::now() + config_.acquire_timeout;
- Line 398: severity=HIGH; category=allocation_loop
  Description: Dynamic allocation in loop — high overhead
  Remediation: Pre-allocate outside loop or use object pool pattern
  Context: // Try to create new connection if under limit
- Line 420: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: std::this_thread::sleep_for(std::chrono::milliseconds(100));
- Line 496: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Acquire lock before loop or redesign to minimize lock time
  Context: std::lock_guard<std::mutex> lock(pool->mutex);
- Line 511: severity=HIGH; category=deadlock_risk
  Description: Multiple locks acquired in nested scope — potential deadlock
  Remediation: Use std::scoped_lock(m1, m2) or enforce consistent lock ordering
  Context: std::lock_guard<std::mutex> pools_lock(pools_mutex_);
- Line 513: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (auto& [target, pool] : target_pools_) {
  Confidence: band=very_high; score=0.9
- Line 514: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Acquire lock before loop or redesign to minimize lock time
  Context: std::lock_guard<std::mutex> lock(pool->mutex);
- Line 645: severity=HIGH; category=deadlock_risk
  Description: Multiple locks acquired in nested scope — potential deadlock
  Remediation: Use std::scoped_lock(m1, m2) or enforce consistent lock ordering
  Context: std::lock_guard<std::mutex> lock(pools_mutex_);
- Line 647: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (auto& [target, pool] : target_pools_) {
  Confidence: band=very_high; score=0.9
- Line 648: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Acquire lock before loop or redesign to minimize lock time
  Context: std::lock_guard<std::mutex> pool_lock(pool->mutex);
- Line 651: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: for (auto& [socket_ptr, conn] : pool->all_connections) {
- Line 676: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: stats.acquire_timeouts = acquire_timeouts_.load(std::memory_order_relaxed);
- Line 680: severity=HIGH; category=deadlock_risk
  Description: Multiple locks acquired in nested scope — potential deadlock
  Remediation: Use std::scoped_lock(m1, m2) or enforce consistent lock ordering
  Context: std::lock_guard<std::mutex> lock(pools_mutex_);
- Line 685: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto& [target, pool] : target_pools_) {
  Confidence: band=very_high; score=0.9
- Line 686: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Acquire lock before loop or redesign to minimize lock time
  Context: std::lock_guard<std::mutex> pool_lock(pool->mutex);
- Line 720: severity=HIGH; category=no_retry_logic
  Description: socket_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: socket.plain_socket()->receive(net::buffer(&dummy, 1), tcp::socket::message_peek, ec);
- Line 389: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: conn->socket->close(ec);
- Line 461: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: conn->socket->close(ec);
- Line 504: severity=MEDIUM; category=expensive_inner_op
  Description: I/O operation in inner loop — very expensive
  Remediation: Buffer output or move I/O outside loop
  Context: std::cerr << "[WireProtocolConnectionPool] Warmup failed for " << target
- Line 529: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: conn->socket->close(ec);
- Line 551: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: targets.push_back(t);
  Confidence: band=high; score=0.74
- Line 654: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: conn->socket->close(ec);
- Line 746: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: WireProtocolConnectionPool::ConnectionHandle::~ConnectionHandle() {
  Confidence: band=high; score=0.74
- Line 752: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: WireProtocolConnectionPool::ConnectionHandle::ConnectionHandle(ConnectionHandle&& other) noexcept
  Confidence: band=high; score=0.74

### src/network/raft_load_balancer.cpp
Total findings: 29

- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 97: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: health_check_thread_ = std::thread([this]() { healthCheckLoop(); });
  Confidence: band=very_high; score=0.9
- Line 98: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: raft_thread_         = std::thread([this]() { raftLoop(); });
  Confidence: band=very_high; score=0.9
- Line 158: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: result.push_back(b.get());
  Confidence: band=very_high; score=0.9
- Line 172: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: strategy_.load(std::memory_order_acquire);
- Line 232: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: return role_.load(std::memory_order_acquire) == RaftRole::LEADER;
- Line 236: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: return role_.load(std::memory_order_acquire);
- Line 240: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: return current_term_.load(std::memory_order_acquire);
- Line 279: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: if (b->address == address) return b.get();
  Confidence: band=very_high; score=0.9
- Line 290: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: result.push_back(b.get());
  Confidence: band=very_high; score=0.9
- Line 297: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: if (b->healthy) result.push_back(b.get());
  Confidence: band=very_high; score=0.9
- Line 407: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: for (const auto& b : backends_) backends_snapshot.push_back(b.get());
  Confidence: band=very_high; score=0.9
- Line 410: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (Backend* b : backends_snapshot) {
  Confidence: band=very_high; score=0.9
- Line 446: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: while (!shutdown_.load(std::memory_order_acquire)) {
- Line 447: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Acquire lock before loop or redesign to minimize lock time
  Context: std::unique_lock<std::mutex> lk(shutdown_mutex_);
- Line 449: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: [this] { return shutdown_.load(std::memory_order_acquire); });
- Line 452: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: if (shutdown_.load(std::memory_order_acquire)) break;
- Line 530: severity=HIGH; category=deadlock_risk
  Description: Multiple locks acquired in nested scope — potential deadlock
  Remediation: Use std::scoped_lock(m1, m2) or enforce consistent lock ordering
  Context: std::unique_lock<std::mutex> lk(shutdown_mutex_);
- Line 566: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: [this] { return shutdown_.load(std::memory_order_acquire); });
- Line 0: severity=MEDIUM; category=uncategorized
  Confidence: band=medium; score=0.57
- Line 0: severity=MEDIUM; category=uncategorized
  Confidence: band=medium; score=0.57
- Line 131: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: backends_.push_back(std::move(backend));
  Confidence: band=high; score=0.74
- Line 157: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(b.get());
  Confidence: band=high; score=0.74
- Line 289: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(b.get());
  Confidence: band=high; score=0.74
- Line 289: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(b.get());
  Confidence: band=high; score=0.74
- Line 290: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.push_back(b.get());
- Line 296: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: if (b->healthy) result.push_back(b.get());
  Confidence: band=high; score=0.74
- Line 297: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: if (b->healthy) result.push_back(b.get());
- Line 407: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: for (const auto& b : backends_) backends_snapshot.push_back(b.get());

### src/network/kernel_bypass.cpp
Total findings: 28

- Line 0: severity=CRITICAL; category=uncategorized
  Confidence: band=very_high; score=0.85
- Line 0: severity=CRITICAL; category=uncategorized
  Context: Pointer declared but not initialized
  Confidence: band=very_high; score=0.93
- Line 704: severity=CRITICAL; category=missing_dtor
  Description: Class io_uring_params allocates resources but has no destructor
  Remediation: Add explicit destructor: ~io_uring_params() { /* cleanup */ }
  Context: class/struct io_uring_params
- Line 29: severity=HIGH; category=posix_only_api
  Description: POSIX-only API pthread_ without platform guard
  Remediation: Wrap in #ifndef _WIN32 ... #endif or provide Windows alternative
  Context: * - CpuPinner uses sched_setaffinity / pthread_setaffinity_np (Linux only).
- Line 125: severity=HIGH; category=posix_only_api
  Description: POSIX-only API pthread_ without platform guard
  Remediation: Wrap in #ifndef _WIN32 ... #endif or provide Windows alternative
  Context: return ::pthread_setaffinity_np(thread.native_handle(),
- Line 192: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: void* NumaAllocator::allocate(size_t size, int node) {
- Line 192: severity=HIGH; category=observability; pattern=missing_trace_point
  Description: Critical function allocate without trace point
  Context: void* NumaAllocator::allocate(size_t size, int node) {
  Confidence: band=very_high; score=0.9
- Line 221: severity=HIGH; category=observability; pattern=missing_trace_point
  Description: Critical function deallocate without trace point
  Context: void NumaAllocator::deallocate(void* ptr, size_t size) noexcept {
  Confidence: band=very_high; score=0.9
- Line 273: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Use <cstdint> types (uint32_t, uint64_t) and sizeof() checks, not constants
  Context: &nodemask, sizeof(nodemask) * 8 + 1, /* MPOL_MF_MOVE */ 2);
- Line 342: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: NumaAllocator::deallocate(data_, size_);
- Line 631: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (uint16_t i = 0; i < nb_rx; ++i) {
  Confidence: band=very_high; score=0.9
- Line 736: severity=HIGH; category=no_retry_logic
  Description: socket_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: int fd = ::socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
- Line 738: severity=HIGH; category=no_retry_logic
  Description: socket_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: last_error_ = "socket() failed: " + std::string(std::strerror(errno));
- Line 125: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: return ::pthread_setaffinity_np(thread.native_handle(),
  Confidence: band=high; score=0.74
- Line 181: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(i);
  Confidence: band=high; score=0.74
- Line 381: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: cores.push_back(i);
  Confidence: band=high; score=0.74
- Line 417: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: eal_arg_strs.push_back("themisdb");
- Line 421: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: eal_arg_strs.push_back("-m");
- Line 422: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: eal_arg_strs.push_back(std::to_string(config_.huge_pages_mb));
- Line 441: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: eal_argv.push_back(const_cast<char*>(s.c_str()));
  Confidence: band=high; score=0.74
- Line 570: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: workers_.emplace_back([this, core, qid]() {
  Confidence: band=high; score=0.74
- Line 644: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: rte_pktmbuf_free(m);
- Line 688: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: ::close(fd);
- Line 708: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: ::close(fd);
- Line 813: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: iovecs.push_back({buf->data(), buf->size()});
  Confidence: band=high; score=0.74
- Line 869: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: workers_.emplace_back([this, i]() { workerLoop(static_cast<int>(i)); });
  Confidence: band=high; score=0.74
- Line 939: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: ::close(listen_fd_);
- Line 943: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: ::close(ring_fd_);

### src/network/quic_transport.cpp
Total findings: 27

- Line 46: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: if (RAND_bytes(cid->data, static_cast<int>(cid->datalen)) != 1) {
- Line 430: severity=CRITICAL; category=smart_ptr_misuse
  Description: Raw new without immediate wrapping in smart pointer
  Remediation: Use auto ptr = std::make_unique<T>(...);
  Context: THEMIS_INFO("[QuicTransport] new QUIC connection from {}", key);
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: * PR History (last 5): #4632 feat(network): QUIC Protoco... (2026-04-13) | #3291 [network] QUIC/HTTP
- Line 45: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: cid->datalen = NGTCP2_MIN_CIDLEN;
- Line 180: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: threads_.emplace_back([this] { io_ctx_->run(); });
- Line 210: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: io_ctx_->stop();
- Line 218: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: io_ctx_->restart();
- Line 256: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: if (running_.load(std::memory_order_acquire)) {
- Line 272: severity=HIGH; category=deadlock_risk
  Description: Multiple locks acquired in nested scope — potential deadlock
  Remediation: Use std::scoped_lock(m1, m2) or enforce consistent lock ordering
  Context: std::lock_guard<std::mutex> lk(sessions_mutex_);
- Line 340: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: void* /*user_data*/) -> int {
- Line 345: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: callbacks.recv_crypto_data = [](ngtcp2_conn*            conn,
- Line 350: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: void*                   /*user_data*/) -> int {
- Line 355: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: void*         user_data) -> int {
- Line 363: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: callbacks.recv_datagram = [](ngtcp2_conn* /*conn*/, uint32_t /*flags*/,
- Line 365: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: void* user_data) -> int {
- Line 368: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: ++transport->stats_.datagrams_received;
- Line 179: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: threads_.emplace_back([this] { io_ctx_->run(); });
  Confidence: band=high; score=0.74
- Line 198: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: SSL_free(static_cast<SSL*>(tls_handle));
- Line 208: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: socket_->close(ec);
- Line 221: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: SSL_CTX_free(ssl_ctx_);
- Line 289: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: SSL_free(static_cast<SSL*>(tls));
- Line 406: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: SSL_free(ssl);
- Line 420: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: SSL_free(ssl);

### src/network/wire_protocol_server_ws.cpp
Total findings: 20

- Line 0: severity=HIGH; category=uncategorized
  Context: ['        const size_t crc_offset = kWireHeaderSize + payload_size;', '        const uint32_t expected_crc =', '            (static_cast<uint32_t>(data[crc_offset]) << 24) |', '            (static_cast<uint32_t>(data[crc_offset + 1]) << 16) |', '            (static_cast<uint32_t>(data[crc_offset + 2]) << 8) |']
  Confidence: band=high; score=0.78
- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: * PR History (last 5): #3632 fix(build): register 40+ mi... (2026-03-12) | #3388 feat(network): impl
- Line 93: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: crc = kWsCrc32Table[(crc ^ data[i]) & 0xFF] ^ (crc >> 8);
- Line 147: severity=HIGH; category=no_retry_logic
  Description: socket_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: ws_.async_accept(
- Line 262: severity=HIGH; category=no_retry_logic
  Description: socket_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: send(resp.dump());
- Line 268: severity=HIGH; category=no_retry_logic
  Description: socket_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: send(makeError(req_id, "get_response", "Missing 'key' in payload"));
- Line 272: severity=HIGH; category=no_retry_logic
  Description: socket_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: send(makeError(req_id, "get_response", "Storage not available"));
- Line 288: severity=HIGH; category=no_retry_logic
  Description: socket_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: send(resp.dump());
- Line 295: severity=HIGH; category=no_retry_logic
  Description: socket_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: send(makeError(req_id, "put_response", "Missing 'key' in payload"));
- Line 299: severity=HIGH; category=no_retry_logic
  Description: socket_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: send(makeError(req_id, "put_response", "Storage not available"));
- Line 308: severity=HIGH; category=no_retry_logic
  Description: socket_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: send(resp.dump());
- Line 314: severity=HIGH; category=no_retry_logic
  Description: socket_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: send(makeError(req_id, "delete_response", "Missing 'key' in payload"));
- Line 318: severity=HIGH; category=no_retry_logic
  Description: socket_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: send(makeError(req_id, "delete_response", "Storage not available"));
- Line 327: severity=HIGH; category=no_retry_logic
  Description: socket_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: send(resp.dump());
- Line 334: severity=HIGH; category=no_retry_logic
  Description: socket_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: send(makeError(req_id, "query_response",
- Line 340: severity=HIGH; category=no_retry_logic
  Description: socket_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: send(makeError(req_id, "error", "Unknown message type: " + type));
- Line 345: severity=HIGH; category=no_retry_logic
  Description: socket_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: send(makeError(req_id, "error", std::string("Invalid JSON: ") + e.what()));
- Line 593: severity=HIGH; category=no_retry_logic
  Description: socket_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: void WireProtocolWebSocketSession::send(const std::string& message) {
- Line 120: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 572: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: close();

### src/network/socket_timeout_manager.cpp
Total findings: 16

- Line 60: severity=CRITICAL; category=missing_dtor
  Description: Class timeval allocates resources but has no destructor
  Remediation: Add explicit destructor: ~timeval() { /* cleanup */ }
  Context: class/struct timeval
- Line 191: severity=CRITICAL; category=smart_ptr_misuse
  Description: Raw new without immediate wrapping in smart pointer
  Remediation: Use auto ptr = std::make_unique<T>(...);
  Context: spdlog::warn("Circuit breaker is open, refusing new connections");
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 108: severity=HIGH; category=no_retry_logic
  Description: socket_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: if (ioctlsocket(socket, FIONBIO, &mode) != 0) {
- Line 245: severity=HIGH; category=no_retry_logic
  Description: socket_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: socket_t client_socket = accept(server_socket, nullptr, nullptr);
- Line 247: severity=HIGH; category=no_retry_logic
  Description: socket_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: spdlog::error("accept() failed: {} ({})", strerror(errno), errno);
- Line 272: severity=HIGH; category=no_retry_logic
  Description: socket_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: int bytes = recv(socket, static_cast<char*>(buffer), static_cast<int>(size), 0);
- Line 280: severity=HIGH; category=no_retry_logic
  Description: socket_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: spdlog::error("recv() failed: {}", error);
- Line 286: severity=HIGH; category=no_retry_logic
  Description: socket_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: ssize_t bytes = recv(socket, buffer, size, 0);
- Line 293: severity=HIGH; category=no_retry_logic
  Description: socket_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: spdlog::error("recv() failed: {} ({})", strerror(errno), errno);
- Line 317: severity=HIGH; category=no_retry_logic
  Description: socket_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: int bytes = send(socket, static_cast<const char*>(buffer), static_cast<int>(size), 0);
- Line 325: severity=HIGH; category=no_retry_logic
  Description: socket_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: spdlog::error("send() failed: {}", error);
- Line 331: severity=HIGH; category=no_retry_logic
  Description: socket_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: ssize_t bytes = send(socket, buffer, size, MSG_NOSIGNAL);  // MSG_NOSIGNAL prevents SIGPIPE
- Line 338: severity=HIGH; category=no_retry_logic
  Description: socket_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: spdlog::error("send() failed: {} ({})", strerror(errno), errno);
- Line 360: severity=HIGH; category=no_retry_logic
  Description: socket_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: closesocket(socket);
- Line 362: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: close(socket);

### src/network/wire_protocol_v2.cpp
Total findings: 16

- Line 170: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: hdr.payload_length = static_cast<uint32_t>(payload->size());
- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: * PR History (last 5): #5081 [Docs][themis] Refresh modu... (2026-05-13) | #4267 feat(themis): Wire
- Line 122: severity=HIGH; category=memory_order
  Description: memory_order_relaxed used — potential visibility issue
  Remediation: Use memory_order_acquire/release unless truly lock-free
  Context: oss << "v2conn-" << counter.fetch_add(1, std::memory_order_relaxed);
- Line 715: severity=HIGH; category=no_retry_logic
  Description: socket_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: acceptor_.async_accept(
- Line 772: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: { impl_->set_data_handler(std::move(h)); }
- Line 782: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: return impl_->push_to_client(conn_id, associated_sid, headers, data);
- Line 70: severity=MEDIUM; category=endianness_assumption
  Description: Endianness conversion may have undefined behavior
  Remediation: Document endianness assumptions or use portable serialization
  Context: uint32_t len_be     = htonl32(h.payload_length);
- Line 190: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: const std::unordered_map<std::string, std::string>& headers) override {
  Confidence: band=medium; score=0.66
- Line 205: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: encoded += k + ": " + v + "\n";
  Confidence: band=high; score=0.74
- Line 227: severity=MEDIUM; category=endianness_assumption
  Description: Endianness conversion may have undefined behavior
  Remediation: Document endianness assumptions or use portable serialization
  Context: uint32_t ec_be = htonl32(error_code);
- Line 251: severity=MEDIUM; category=endianness_assumption
  Description: Endianness conversion may have undefined behavior
  Remediation: Document endianness assumptions or use portable serialization
  Context: uint32_t ec_be   = htonl32(error_code);
- Line 262: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: socket_.close();
- Line 652: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: io_threads_.emplace_back([this]() {
  Confidence: band=high; score=0.74
- Line 656: severity=MEDIUM; category=expensive_inner_op
  Description: I/O operation in inner loop — very expensive
  Remediation: Buffer output or move I/O outside loop
  Context: std::cerr << "[WireV2] I/O thread error: " << e.what() << '\n';
- Line 656: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: std::cerr << "[WireV2] I/O thread error: " << e.what() << '\n';
- Line 656: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: std::cerr << "[WireV2] I/O thread error: " << e.what() << '\n';

### src/network/qos_manager.cpp
Total findings: 13

- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 96: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: std::this_thread::sleep_for(std::chrono::milliseconds(wait_ms));
- Line 98: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: std::this_thread::sleep_for(std::chrono::milliseconds(1));
- Line 891: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: QoSManager::getConnectionStats(uint64_t connection_id) const {
- Line 910: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: cs.has_token_bucket = (state->token_bucket != nullptr);
- Line 942: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: result.push_back(getConnectionStats(id));
- Line 977: severity=HIGH; category=deadlock_risk
  Description: Multiple locks acquired in nested scope — potential deadlock
  Remediation: Use std::scoped_lock(m1, m2) or enforce consistent lock ordering
  Context: std::lock_guard<std::mutex> lock(tenants_mutex_);
- Line 41: severity=MEDIUM; category=legacy_duplication; pattern=duplicate_qualified_signature
  Description: Potential duplicate implementation signature across files: TokenBucket::refill()
  Context: void TokenBucket::refill() {
  Confidence: band=medium; score=0.56
- Line 941: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(getConnectionStats(id));
  Confidence: band=high; score=0.74
- Line 1074: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(getTenantStats(id));
  Confidence: band=high; score=0.74

### src/network/service_mesh.cpp
Total findings: 13

- Line 135: severity=CRITICAL; category=no_timeout
  Description: file_io without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: boost::asio::write(socket, net::buffer(resp), ec);
- Line 146: severity=CRITICAL; category=no_timeout
  Description: file_io without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: boost::asio::write(socket, net::buffer(resp), ec);
- Line 190: severity=CRITICAL; category=no_timeout
  Description: file_io without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: acceptor_->open(ep.protocol());
- Line 158: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: while (running_.load(std::memory_order_acquire)) {
- Line 160: severity=HIGH; category=no_retry_logic
  Description: socket_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: tcp::socket socket(*io_ctx_);
- Line 161: severity=HIGH; category=no_retry_logic
  Description: socket_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: acceptor_->accept(socket, ec);
- Line 112: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: req.find("GET /healthz\r") != std::string::npos);
- Line 115: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: req.find("GET /readyz\r")  != std::string::npos);
- Line 131: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: "HTTP/1.1 404 Not Found\r\n"
- Line 140: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: "HTTP/1.1 200 OK\r\n"
- Line 141: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: "Content-Type: text/plain\r\n"
- Line 174: severity=MEDIUM; category=observability; pattern=missing_health_check
  Description: Service initialization without nearby health/status handling
  Context: bool ServiceMeshIntegration::start() {
  Confidence: band=medium; score=0.66
- Line 236: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: acceptor_->close(ec);

### src/network/wire_protocol_zero_copy.cpp
Total findings: 12

- Line 101: severity=CRITICAL; category=no_timeout
  Description: file_io without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: return ::write(fd, header_.data(), HEADER_SIZE);
- Line 149: severity=CRITICAL; category=no_timeout
  Description: file_io without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: fd_ = ::open(path.c_str(), O_RDONLY | O_CLOEXEC);
- Line 0: severity=HIGH; category=uncategorized
  Context: Array declared without initialization
  Confidence: band=high; score=0.81
- Line 0: severity=HIGH; category=uncategorized
  Context: Array declared without initialization
  Confidence: band=high; score=0.81
- Line 151: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::system_error(errno, std::system_category(),
- Line 176: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::system_error(err, std::system_category(),
- Line 202: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::system_error(errno, std::system_category(),
- Line 157: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: ::close(fd_);
- Line 165: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: ::close(fd_);
- Line 174: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: ::close(fd_);
- Line 241: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: if (addr_ != MAP_FAILED && addr_ != nullptr) std::free(addr_);
- Line 244: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: if (fd_ >= 0) ::close(fd_);

### src/network/adaptive_circuit_breaker.cpp
Total findings: 10

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
- Line 53: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: if (state_.load(std::memory_order_acquire) == CircuitState::CLOSED) {
- Line 92: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: const CircuitState s = state_.load(std::memory_order_acquire);
- Line 131: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: const CircuitState s = state_.load(std::memory_order_acquire);
- Line 164: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: return state_.load(std::memory_order_acquire);

### src/network/network_audit_log.cpp
Total findings: 10

- Line 85: severity=CRITICAL; category=array_bounds
  Description: Array bounds violation: loop 64 > array 0
  Remediation: Fix loop condition or increase array size
  Context: uint32_t a=h[0], b=h[1], c=h[2], d=h[3],
- Line 86: severity=CRITICAL; category=array_bounds
  Description: Array bounds violation: loop 64 > array 4
  Remediation: Fix loop condition or increase array size
  Context: e=h[4], f=h[5], g=h[6], hh=h[7];
- Line 99: severity=CRITICAL; category=array_bounds
  Description: Array bounds violation: loop 8 > array 0
  Remediation: Fix loop condition or increase array size
  Context: h[0]+=a; h[1]+=b; h[2]+=c; h[3]+=d;
- Line 100: severity=CRITICAL; category=array_bounds
  Description: Array bounds violation: loop 8 > array 4
  Remediation: Fix loop condition or increase array size
  Context: h[4]+=e; h[5]+=f; h[6]+=g; h[7]+=hh;
- Line 0: severity=HIGH; category=uncategorized
  Context: ['    // Pre-processing: padding', '    const uint64_t bit_len = static_cast<uint64_t>(len) * 8u;', '    size_t padded_len = len + 1;', '    while (padded_len % 64 != 56) ++padded_len;', '    padded_len += 8;']
  Confidence: band=high; score=0.81
- Line 0: severity=HIGH; category=uncategorized
  Context: ['    std::vector<uint8_t> msg(padded_len, 0);', '    std::copy(data, data + len, msg.begin());', '    msg[len] = 0x80u;', '    // Big-endian bit length at the end', '    for (int i = 0; i < 8; ++i) {']
  Confidence: band=high; score=0.81
- Line 0: severity=HIGH; category=uncategorized
  Context: Array declared without initialization
  Confidence: band=high; score=0.81
- Line 0: severity=MEDIUM; category=uncategorized
  Confidence: band=medium; score=0.57
- Line 0: severity=MEDIUM; category=uncategorized
  Confidence: band=medium; score=0.57
- Line 246: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: if (ev.type == type) result.push_back(ev);
  Confidence: band=high; score=0.74

### src/network/udp_server.cpp
Total findings: 9

- Line 70: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: io_threads_.emplace_back([this] { io_ctx_->run(); });
- Line 96: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: io_ctx_->stop();
- Line 103: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: io_ctx_->restart();
- Line 136: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: if (running_.load(std::memory_order_acquire)) {
- Line 359: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Acquire lock before loop or redesign to minimize lock time
  Context: std::unique_lock<std::mutex> lk(batch_mutex_);
- Line 47: severity=MEDIUM; category=observability; pattern=missing_health_check
  Description: Service initialization without nearby health/status handling
  Context: void UDPServer::start() {
  Confidence: band=medium; score=0.66
- Line 69: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: io_threads_.emplace_back([this] { io_ctx_->run(); });
  Confidence: band=high; score=0.74
- Line 94: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: socket_->close(ec);
- Line 469: severity=MEDIUM; category=endianness_assumption
  Description: Endianness conversion may have undefined behavior
  Remediation: Document endianness assumptions or use portable serialization
  Context: const uint32_t seq_be = htonl(seq_num);

### src/network/udp_fast_path.cpp
Total findings: 8

- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 70: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: threads_.emplace_back([this] { io_ctx_->run(); });
- Line 83: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: io_ctx_->stop();
- Line 90: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: io_ctx_->restart();
- Line 124: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: if (running_.load(std::memory_order_acquire)) {
- Line 69: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: threads_.emplace_back([this] { io_ctx_->run(); });
  Confidence: band=high; score=0.74
- Line 176: severity=MEDIUM; category=endianness_assumption
  Description: Endianness conversion may have undefined behavior
  Remediation: Document endianness assumptions or use portable serialization
  Context: const uint32_t request_id = ntohl(req_id_be);
- Line 397: severity=MEDIUM; category=endianness_assumption
  Description: Endianness conversion may have undefined behavior
  Remediation: Document endianness assumptions or use portable serialization
  Context: const uint32_t req_id_be = htonl(request_id);

### src/network/wire_protocol_helpers.cpp
Total findings: 8

- Line 234: severity=HIGH; category=no_retry_logic
  Description: http_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: if (request.collection.empty()) {
- Line 291: severity=HIGH; category=determinism; pattern=fp_exact_comparison
  Description: Floating-point exact comparison (use tolerance/epsilon)
  Context: if (data_density != 0.0) {
  Confidence: band=very_high; score=0.9
- Line 135: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: data_.push_back(static_cast<uint8_t>((value & 0x7F) | 0x80));
- Line 138: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: data_.push_back(static_cast<uint8_t>(value & 0x7F));
- Line 143: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: data_.push_back(static_cast<uint8_t>(value & 0xFF));
  Confidence: band=high; score=0.74
- Line 144: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: data_.push_back(static_cast<uint8_t>(value & 0xFF));
- Line 151: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: data_.push_back(static_cast<uint8_t>(value & 0xFF));
  Confidence: band=high; score=0.74
- Line 152: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: data_.push_back(static_cast<uint8_t>(value & 0xFF));

### src/network/connection_compression.cpp
Total findings: 7

- Line 0: severity=CRITICAL; category=uncategorized
  Context: ['    size_t offset = 0;', '    for (size_t i = 0; i < samples.size(); ++i) {', '        std::memcpy(concat.data() + offset, samples[i].data(), samples[i].size());', '        sample_sizes[i] = samples[i].size();', '        offset += samples[i].size();']
  Confidence: band=very_high; score=0.9
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

### src/network/io_uring_batcher.cpp
Total findings: 7

- Line 0: severity=CRITICAL; category=uncategorized
  Context: ['    std::memcpy(sqe + 16, &addr,     sizeof(addr));   // addr (iovec ptr)', '    uint32_t len = static_cast<uint32_t>(iov_cnt);', '    std::memcpy(sqe + 24, &len,      sizeof(len));    // len  (iov_cnt)', '    std::memcpy(sqe + 32, &user_data,sizeof(user_data)); // user_data', '']
  Confidence: band=very_high; score=0.9
- Line 207: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: [[maybe_unused]] size_t iov_cnt, [[maybe_unused]] uint64_t user_data) {
- Line 236: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Use <cstdint> types (uint32_t, uint64_t) and sizeof() checks, not constants
  Context: std::memcpy(sqe + 4,  &fd_i,     sizeof(fd_i));  // fd
- Line 238: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Use <cstdint> types (uint32_t, uint64_t) and sizeof() checks, not constants
  Context: std::memcpy(sqe + 16, &addr,     sizeof(addr));   // addr (iovec ptr)
- Line 240: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Use <cstdint> types (uint32_t, uint64_t) and sizeof() checks, not constants
  Context: std::memcpy(sqe + 24, &len,      sizeof(len));    // len  (iov_cnt)
- Line 243: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: sq_.array[tail & *sq_.ring_mask] = tail & *sq_.ring_mask;
- Line 344: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: ::close(ring_fd_);

### src/network/grpc_transport.cpp
Total findings: 6

- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: * PR History (last 5): #4330 feat(cache): network-backed... (2026-03-19) | #3577 [MODULE] network +
- Line 160: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Acquire lock before loop or redesign to minimize lock time
  Context: std::lock_guard<std::mutex> lk(stats_mutex_);
- Line 174: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: if (running_.load(std::memory_order_acquire)) {
- Line 18: severity=MEDIUM; category=observability; pattern=missing_correlation_id
  Description: Distributed call without correlation ID
  Context: #include <grpcpp/generic/async_generic_service.h>
  Confidence: band=high; score=0.74
- Line 173: severity=MEDIUM; category=observability; pattern=missing_health_check
  Description: Service initialization without nearby health/status handling
  Context: bool GrpcTransport::start() {
  Confidence: band=medium; score=0.66
- Line 220: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: threads_.emplace_back([this, cq] { drainCompletionQueue(cq); });
  Confidence: band=high; score=0.74

### src/network/wire_protocol_performance.cpp
Total findings: 6

- Line 221: severity=CRITICAL; category=no_timeout
  Description: semaphore_wait without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: PayloadBufferPool::Handle PayloadBufferPool::acquire() {
- Line 221: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: PayloadBufferPool::Handle PayloadBufferPool::acquire() {
- Line 183: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: PayloadBufferPool::Handle::Handle(Handle&& o) noexcept
  Confidence: band=high; score=0.74
- Line 190: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: release();
- Line 198: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: void PayloadBufferPool::Handle::release() noexcept {
- Line 206: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: PayloadBufferPool::Handle::~Handle() {
  Confidence: band=high; score=0.74

### src/network/geo_topology_router.cpp
Total findings: 1

- Line 104: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: (it != config_.region_latency_hints.end()) ? it->second : kUnhintedRegionLatencyMs;

### src/network/wire_protocol_batch.cpp
Total findings: 1

- Line 174: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: posix_iov.push_back(item);
  Confidence: band=high; score=0.74

## Update Workflow

- Refresh scan artifacts with: python tools/gap_scanner_v3.py
- Regenerate all module notes with: python tools/module_doc_generator.py . ai_working ai_working/module_gaps
- The generator mirrors each archive document directly into src/<module>/MODULE_GAPS.md.

Format: THEMIS_MODULE_GAPS_V3
