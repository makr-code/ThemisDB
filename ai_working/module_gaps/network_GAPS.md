# network Module - Developer Gap Note

> Auto-generated from ai_working/gap_scan_v3_aggregate.json.
> This file is overwritten on each regeneration.

## Scan Snapshot

- Module: network
- Generated: 2026-05-31 08:50:11
- Status: Critical Findings Present
- Total Findings: 750
- Actionable Findings (Critical + High): 499
- Affected Files: 24

## Severity Summary

| Severity | Count |
|---|---:|
| Critical | 55 |
| High | 444 |
| Medium | 251 |
| Low | 0 |

## Category Summary

| Category | Count |
|---|---:|
| reliability | 203 |
| raii | 128 |
| performance_patterns | 92 |
| memory | 73 |
| container | 71 |
| exception_safety | 38 |
| concurrency | 34 |
| platform | 28 |
| performance | 23 |
| observability | 19 |
| audit_logging | 10 |
| security | 9 |
| distributed_consistency | 7 |
| input_validation | 4 |
| legacy_duplication | 4 |
| uninitialized | 4 |
| determinism | 2 |
| type_conversion | 1 |

## File Overview

| File | Findings | Critical | High | Medium | Low |
|---|---:|---:|---:|---:|---:|
| src/network/wire_protocol_server.cpp | 216 | 10 | 147 | 59 | 0 |
| src/network/envoy_xds.cpp | 61 | 1 | 30 | 30 | 0 |
| src/network/quic_server.cpp | 54 | 2 | 31 | 21 | 0 |
| src/network/wire_protocol_connection_pool.cpp | 53 | 5 | 33 | 15 | 0 |
| src/network/kernel_bypass.cpp | 50 | 3 | 16 | 31 | 0 |
| src/network/qos_manager.cpp | 40 | 11 | 22 | 7 | 0 |
| src/network/raft_load_balancer.cpp | 37 | 1 | 24 | 12 | 0 |
| src/network/wire_protocol_server_ws.cpp | 30 | 1 | 25 | 4 | 0 |
| src/network/quic_transport.cpp | 27 | 2 | 15 | 10 | 0 |
| src/network/wire_protocol_v2.cpp | 27 | 3 | 13 | 11 | 0 |
| src/network/wire_protocol_zero_copy.cpp | 25 | 2 | 15 | 8 | 0 |
| src/network/socket_timeout_manager.cpp | 16 | 2 | 13 | 1 | 0 |
| src/network/adaptive_circuit_breaker.cpp | 13 | 0 | 13 | 0 | 0 |
| src/network/network_audit_log.cpp | 13 | 4 | 5 | 4 | 0 |
| src/network/service_mesh.cpp | 13 | 3 | 3 | 7 | 0 |
| src/network/udp_fast_path.cpp | 12 | 0 | 6 | 6 | 0 |
| src/network/udp_server.cpp | 12 | 0 | 8 | 4 | 0 |
| src/network/io_uring_batcher.cpp | 10 | 1 | 5 | 4 | 0 |
| src/network/wire_protocol_helpers.cpp | 10 | 0 | 4 | 6 | 0 |
| src/network/wire_protocol_performance.cpp | 10 | 1 | 4 | 5 | 0 |
| src/network/grpc_transport.cpp | 9 | 0 | 5 | 4 | 0 |
| src/network/connection_compression.cpp | 7 | 1 | 6 | 0 | 0 |
| src/network/geo_topology_router.cpp | 3 | 2 | 1 | 0 | 0 |
| src/network/wire_protocol_batch.cpp | 2 | 0 | 0 | 2 | 0 |

## Full Scanner Findings

### src/network/wire_protocol_server.cpp
Total findings: 216

- Line 8: severity=CRITICAL; category=new_without_delete
  Description: Raw new without RAII wrapper — potential memory leak
  Remediation: Use std::unique_ptr<T> or std::make_unique<T>()
  Context: * PR: #3145 [themis] Move wire protocol server to new location (Phase 3) (2026-03-12T06:24:06Z)
- Line 8: severity=CRITICAL; category=smart_ptr_misuse
  Description: Raw new without immediate wrapping in smart pointer
  Remediation: Use auto ptr = std::make_unique<T>(...);
  Context: * PR: #3145 [themis] Move wire protocol server to new location (Phase 3) (2026-03-12T06:24:06Z)
- Line 413: severity=CRITICAL; category=no_timeout
  Description: file_io without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: acceptor_->open(endpoint.protocol());
- Line 465: severity=CRITICAL; category=no_timeout
  Description: semaphore_wait without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: worker_pool_->wait();
- Line 469: severity=CRITICAL; category=no_timeout
  Description: semaphore_wait without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: void WireProtocolServer::wait() {
- Line 531: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: if (it != connections_per_ip_.end() && it->second >= config_.max_connections_per_ip) {
- Line 1220: severity=CRITICAL; category=no_timeout
  Description: semaphore_wait without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: timeout_timer_->async_wait([this, self](const boost::system::error_code& ec) {
- Line 1295: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: response["auth_required"] = server_->config_.require_auth;
- Line 1360: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: const auto& stored = server_->config_.auth_token;
- Line 3076: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto agg_result = server_->ts_store_->aggregate(query_opts);
- Line 8: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: * PR: #3145 [themis] Move wire protocol server to new location (Phase 3) (2026-03-12T06:24:06Z)
- Line 41: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: #include <cstdio>  // For snprintf
  Confidence: band=very_high; score=0.9
- Line 173: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (size_t i = 0; i < len; ++i)
  Confidence: band=very_high; score=0.9
- Line 174: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: crc = kCrc32Table[(crc ^ data[i]) & 0xFF] ^ (crc >> 8);
- Line 467: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (auto& t : io_threads_) {
  Confidence: band=very_high; score=0.9
- Line 477: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (auto& t : io_threads_) {
  Confidence: band=very_high; score=0.9
- Line 526: severity=HIGH; category=memory_order
  Description: memory_order_relaxed used — potential visibility issue
  Remediation: Use memory_order_acquire/release unless truly lock-free
  Context: active_connection_count_.load(std::memory_order_relaxed) >= config_.max_connections) {
- Line 613: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: if (!running_.load(std::memory_order_acquire)) return;
- Line 617: severity=HIGH; category=no_retry_logic
  Description: socket_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: tcp::socket(*io_context_),
- Line 620: severity=HIGH; category=no_retry_logic
  Description: socket_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: acceptor_->async_accept(
- Line 774: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: std::memcpy(&magic_recv, &header_buffer_[0], sizeof(uint32_t));
- Line 785: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: std::memcpy(&flags, &header_buffer_[6], sizeof(uint16_t));
- Line 785: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Use <cstdint> types (uint32_t, uint64_t) and sizeof() checks, not constants
  Context: std::memcpy(&flags, &header_buffer_[6], sizeof(uint16_t));
- Line 792: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: std::memcpy(&payload_size, &header_buffer_[8], sizeof(uint32_t));
- Line 792: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Use <cstdint> types (uint32_t, uint64_t) and sizeof() checks, not constants
  Context: std::memcpy(&payload_size, &header_buffer_[8], sizeof(uint32_t));
- Line 836: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: header_buffer_[0] == 'G' &&
- Line 837: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: header_buffer_[1] == 'E' &&
- Line 838: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: header_buffer_[2] == 'T' &&
- Line 839: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: header_buffer_[3] == ' ';
- Line 868: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: std::memcpy(&magic_recv, &header_buffer_[0], sizeof(uint32_t));
- Line 879: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: std::memcpy(&flags, &header_buffer_[6], sizeof(uint16_t));
- Line 879: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Use <cstdint> types (uint32_t, uint64_t) and sizeof() checks, not constants
  Context: std::memcpy(&flags, &header_buffer_[6], sizeof(uint16_t));
- Line 884: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: std::memcpy(&payload_size, &header_buffer_[8], sizeof(uint32_t));
- Line 884: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Use <cstdint> types (uint32_t, uint64_t) and sizeof() checks, not constants
  Context: std::memcpy(&payload_size, &header_buffer_[8], sizeof(uint32_t));
- Line 917: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: auto mutable_buf = buf->prepare(4);
- Line 1097: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: uint8_t opcode = header_buffer_[5];
- Line 1181: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: std::snprintf(hex_opcode, sizeof(hex_opcode), "0x%02X", opcode);
  Confidence: band=very_high; score=0.9
- Line 1238: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: net::dispatch(socket_.get_executor(), [this, self, data_copy = std::move(data_copy)]() {
- Line 1330: severity=HIGH; category=no_retry_logic
  Description: http_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: if (!request.is_object()) {
- Line 1334: severity=HIGH; category=no_retry_logic
  Description: http_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: if (request.contains("token") && !request["token"].is_string()) {
- Line 1338: severity=HIGH; category=no_retry_logic
  Description: http_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: if (request.contains("username") && !request["username"].is_string()) {
- Line 1343: severity=HIGH; category=no_retry_logic
  Description: http_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: token = request.value("token", "");
- Line 1344: severity=HIGH; category=no_retry_logic
  Description: http_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: username_req = request.value("username", "");
- Line 1422: severity=HIGH; category=no_retry_logic
  Description: http_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: if (!request.is_object()) {
- Line 1426: severity=HIGH; category=no_retry_logic
  Description: http_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: if ((request.contains("collection") && !request["collection"].is_string()) ||
- Line 1427: severity=HIGH; category=no_retry_logic
  Description: http_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: (request.contains("key") && !request["key"].is_string())) {
- Line 1431: severity=HIGH; category=no_retry_logic
  Description: http_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: std::string collection = request.value("collection", "");
- Line 1432: severity=HIGH; category=no_retry_logic
  Description: http_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: std::string key = request.value("key", "");
- Line 1495: severity=HIGH; category=no_retry_logic
  Description: http_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: if (!request.is_object()) {
- Line 1499: severity=HIGH; category=no_retry_logic
  Description: http_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: if ((request.contains("collection") && !request["collection"].is_string()) ||
- Line 1500: severity=HIGH; category=no_retry_logic
  Description: http_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: (request.contains("key") && !request["key"].is_string())) {
- Line 1504: severity=HIGH; category=no_retry_logic
  Description: http_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: std::string collection = request.value("collection", "");
- Line 1505: severity=HIGH; category=no_retry_logic
  Description: http_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: std::string key = request.value("key", "");
- Line 1565: severity=HIGH; category=no_retry_logic
  Description: http_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: if (!request.is_object()) {
- Line 1569: severity=HIGH; category=no_retry_logic
  Description: http_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: if ((request.contains("collection") && !request["collection"].is_string()) ||
- Line 1570: severity=HIGH; category=no_retry_logic
  Description: http_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: (request.contains("key") && !request["key"].is_string())) {
- Line 1574: severity=HIGH; category=no_retry_logic
  Description: http_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: std::string collection = request.value("collection", "");
- Line 1575: severity=HIGH; category=no_retry_logic
  Description: http_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: std::string key = request.value("key", "");
- Line 1623: severity=HIGH; category=no_retry_logic
  Description: http_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: if (!request.is_object()) {
- Line 1627: severity=HIGH; category=no_retry_logic
  Description: http_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: if (request.contains("collection") && !request["collection"].is_string()) {
- Line 1631: severity=HIGH; category=no_retry_logic
  Description: http_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: std::string collection = request.value("collection", "");
- Line 1641: severity=HIGH; category=no_retry_logic
  Description: http_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: if (!request.contains("keys") || !request["keys"].is_array()) {
- Line 1740: severity=HIGH; category=no_retry_logic
  Description: http_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: if (!request.is_object()) {
- Line 1744: severity=HIGH; category=no_retry_logic
  Description: http_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: if (request.contains("collection") && !request["collection"].is_string()) {
- Line 1748: severity=HIGH; category=no_retry_logic
  Description: http_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: std::string collection = request.value("collection", "");
- Line 1758: severity=HIGH; category=no_retry_logic
  Description: http_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: if (!request.contains("items") || !request["items"].is_array()) {
- Line 1899: severity=HIGH; category=no_retry_logic
  Description: http_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: if (!request.is_object()) {
- Line 1903: severity=HIGH; category=no_retry_logic
  Description: http_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: if (request.contains("isolation_level") && !request["isolation_level"].is_string()) {
- Line 1907: severity=HIGH; category=no_retry_logic
  Description: http_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: if (request.contains("timeout_ms") && !request["timeout_ms"].is_number_integer()) {
- Line 1911: severity=HIGH; category=no_retry_logic
  Description: http_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: if (request.contains("timeout_ms")) {
- Line 1920: severity=HIGH; category=no_retry_logic
  Description: http_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: std::string isolation_str = request.value("isolation_level", "read_committed");
- Line 1970: severity=HIGH; category=no_retry_logic
  Description: http_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: if (!request.is_object()) {
- Line 1974: severity=HIGH; category=no_retry_logic
  Description: http_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: if (request.contains("transaction_id") && !request["transaction_id"].is_string()) {
- Line 1978: severity=HIGH; category=no_retry_logic
  Description: http_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: std::string tx_id_str = request.value("transaction_id", "");
- Line 2037: severity=HIGH; category=no_retry_logic
  Description: http_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: if (!request.is_object()) {
- Line 2041: severity=HIGH; category=no_retry_logic
  Description: http_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: if (request.contains("transaction_id") && !request["transaction_id"].is_string()) {
- Line 2045: severity=HIGH; category=no_retry_logic
  Description: http_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: std::string tx_id_str = request.value("transaction_id", "");
- Line 2100: severity=HIGH; category=no_retry_logic
  Description: http_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: if (!request.is_object()) {
- Line 2104: severity=HIGH; category=no_retry_logic
  Description: http_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: if ((request.contains("collection") && !request["collection"].is_string()) ||
- Line 2105: severity=HIGH; category=no_retry_logic
  Description: http_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: (request.contains("start_vertex") && !request["start_vertex"].is_string())) {
- Line 2109: severity=HIGH; category=no_retry_logic
  Description: http_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: if ((request.contains("direction") && !request["direction"].is_string()) ||
- Line 2110: severity=HIGH; category=no_retry_logic
  Description: http_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: (request.contains("edge_type") && !request["edge_type"].is_string())) {
- Line 2114: severity=HIGH; category=no_retry_logic
  Description: http_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: if ((request.contains("depth_min") && !request["depth_min"].is_number_integer()) ||
- Line 2115: severity=HIGH; category=no_retry_logic
  Description: http_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: (request.contains("depth_max") && !request["depth_max"].is_number_integer()) ||
- Line 2116: severity=HIGH; category=no_retry_logic
  Description: http_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: (request.contains("limit") && !request["limit"].is_number_integer())) {
- Line 2120: severity=HIGH; category=no_retry_logic
  Description: http_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: std::string collection = request.value("collection", "");
- Line 2121: severity=HIGH; category=no_retry_logic
  Description: http_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: std::string start_vertex = request.value("start_vertex", "");
- Line 2151: severity=HIGH; category=no_retry_logic
  Description: http_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: std::string direction_str = request.value("direction", "outbound");
- Line 2152: severity=HIGH; category=no_retry_logic
  Description: http_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: int depth_min = request.value("depth_min", 1);
- Line 2153: severity=HIGH; category=no_retry_logic
  Description: http_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: int depth_max = request.value("depth_max", 3);
- Line 2154: severity=HIGH; category=no_retry_logic
  Description: http_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: int limit     = request.value("limit", 100);
- Line 2155: severity=HIGH; category=no_retry_logic
  Description: http_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: std::string edge_type = request.value("edge_type", "");
- Line 2199: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: v["data"]      = tr.vertex_data;
- Line 2237: severity=HIGH; category=no_retry_logic
  Description: http_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: if (!request.is_object()) {
- Line 2241: severity=HIGH; category=no_retry_logic
  Description: http_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: if (request.contains("query") && !request["query"].is_string()) {
- Line 2245: severity=HIGH; category=no_retry_logic
  Description: http_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: if (request.contains("batch_size") && !request["batch_size"].is_number_integer()) {
- Line 2249: severity=HIGH; category=no_retry_logic
  Description: http_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: std::string query_str = request.value("query", "");
- Line 2284: severity=HIGH; category=no_retry_logic
  Description: http_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: int batch_size_i = request.value("batch_size", 100);
- Line 2306: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (size_t i = 0; i < batch_size; ++i) {
  Confidence: band=very_high; score=0.9
- Line 2315: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: std::snprintf(buf, sizeof(buf), "cursor-%llu-%llu",
  Confidence: band=very_high; score=0.9
- Line 2363: severity=HIGH; category=no_retry_logic
  Description: http_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: if (!request.is_object()) {
- Line 2367: severity=HIGH; category=no_retry_logic
  Description: http_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: if (request.contains("cursor_id") && !request["cursor_id"].is_string()) {
- Line 2371: severity=HIGH; category=no_retry_logic
  Description: http_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: if (request.contains("batch_size") && !request["batch_size"].is_number_integer()) {
- Line 2375: severity=HIGH; category=no_retry_logic
  Description: http_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: std::string cursor_id = request.value("cursor_id", "");
- Line 2386: severity=HIGH; category=no_retry_logic
  Description: http_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: int batch_size_i = request.value("batch_size", 100);
- Line 2457: severity=HIGH; category=no_retry_logic
  Description: http_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: if (!request.is_object()) {
- Line 2461: severity=HIGH; category=no_retry_logic
  Description: http_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: if (request.contains("cursor_id") && !request["cursor_id"].is_string()) {
- Line 2465: severity=HIGH; category=no_retry_logic
  Description: http_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: std::string cursor_id = request.value("cursor_id", "");
- Line 2511: severity=HIGH; category=no_retry_logic
  Description: http_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: if (request.contains("k") && !request["k"].is_number_integer()) {
- Line 2516: severity=HIGH; category=no_retry_logic
  Description: http_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: if (request.contains("collection") && !request["collection"].is_string()) {
- Line 2521: severity=HIGH; category=no_retry_logic
  Description: http_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: const std::string collection = request.value("collection", "");
- Line 2527: severity=HIGH; category=no_retry_logic
  Description: http_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: if (!request.contains("vector") || !request["vector"].is_array()) {
- Line 2559: severity=HIGH; category=no_retry_logic
  Description: http_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: int64_t k_i = request.value("k", static_cast<int64_t>(10));
- Line 2616: severity=HIGH; category=no_retry_logic
  Description: http_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: if (request.contains("collection") && !request["collection"].is_string()) {
- Line 2620: severity=HIGH; category=no_retry_logic
  Description: http_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: if (request.contains("type") && !request["type"].is_string()) {
- Line 2624: severity=HIGH; category=no_retry_logic
  Description: http_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: if (request.contains("limit") && !request["limit"].is_number_integer()) {
- Line 2629: severity=HIGH; category=no_retry_logic
  Description: http_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: std::string collection = request.value("collection", "");
- Line 2639: severity=HIGH; category=no_retry_logic
  Description: http_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: std::string query_type = request.value("type", "");
- Line 2656: severity=HIGH; category=no_retry_logic
  Description: http_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: if (!request.contains("center") || !request["center"].is_object()) {
- Line 2662: severity=HIGH; category=no_retry_logic
  Description: http_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: if (!request.contains("radius")) {
- Line 2695: severity=HIGH; category=no_retry_logic
  Description: http_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: int64_t limit_i = request.value("limit", static_cast<int64_t>(100));
- Line 2746: severity=HIGH; category=no_retry_logic
  Description: http_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: int64_t limit_i = request.value("limit", static_cast<int64_t>(100));
- Line 2756: severity=HIGH; category=no_retry_logic
  Description: http_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: if (!request.contains("bbox") || !request["bbox"].is_object()) {
- Line 2790: severity=HIGH; category=no_retry_logic
  Description: http_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: if (!request.contains("center") || !request["center"].is_object()) {
- Line 2796: severity=HIGH; category=no_retry_logic
  Description: http_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: if (!request.contains("radius")) {
- Line 2902: severity=HIGH; category=no_retry_logic
  Description: http_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: if (request.collection.empty()) {
- Line 2984: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: auto result = server_->ts_store_->query(query_opts);
- Line 3021: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: bucket_data[bucket_start_ms].push_back(point.value);
- Line 3025: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: for (const auto& [bucket_start_ms, values] : bucket_data) {
- Line 3127: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: auto result = server_->ts_store_->query(query_opts);
- Line 3228: severity=HIGH; category=no_retry_logic
  Description: http_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: if (!request.is_object()) {
- Line 3233: severity=HIGH; category=no_retry_logic
  Description: http_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: if (request.contains("process_definition_key") && !request["process_definition_key"].is_string()) {
- Line 3237: severity=HIGH; category=no_retry_logic
  Description: http_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: if (request.contains("variables") && !request["variables"].is_object()) {
- Line 3241: severity=HIGH; category=no_retry_logic
  Description: http_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: if (request.contains("business_key") && !request["business_key"].is_string()) {
- Line 3246: severity=HIGH; category=no_retry_logic
  Description: http_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: std::string process_key = request.value("process_definition_key", "");
- Line 3247: severity=HIGH; category=no_retry_logic
  Description: http_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: json variables = request.value("variables", json::object());
- Line 3248: severity=HIGH; category=no_retry_logic
  Description: http_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: std::string business_key = request.value("business_key", "");
- Line 3324: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: response["active_task_ids"] = json::array();
- Line 3363: severity=HIGH; category=no_retry_logic
  Description: http_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: if (!request.is_object()) {
- Line 3368: severity=HIGH; category=no_retry_logic
  Description: http_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: if (request.contains("task_id") && !request["task_id"].is_string()) {
- Line 3372: severity=HIGH; category=no_retry_logic
  Description: http_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: if (request.contains("variables") && !request["variables"].is_object()) {
- Line 3376: severity=HIGH; category=no_retry_logic
  Description: http_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: if (request.contains("assignee") && !request["assignee"].is_string()) {
- Line 3381: severity=HIGH; category=no_retry_logic
  Description: http_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: std::string task_id = request.value("task_id", "");
- Line 3382: severity=HIGH; category=no_retry_logic
  Description: http_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: json variables = request.value("variables", json::object());
- Line 3383: severity=HIGH; category=no_retry_logic
  Description: http_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: std::string assignee = request.value("assignee", username_);
- Line 3502: severity=HIGH; category=no_retry_logic
  Description: http_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: if (!request.is_object()) {
- Line 3507: severity=HIGH; category=no_retry_logic
  Description: http_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: if (request.contains("process_instance_id") && !request["process_instance_id"].is_string()) {
- Line 3511: severity=HIGH; category=no_retry_logic
  Description: http_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: if (request.contains("include_variables") && !request["include_variables"].is_boolean()) {
- Line 3515: severity=HIGH; category=no_retry_logic
  Description: http_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: if (request.contains("include_history") && !request["include_history"].is_boolean()) {
- Line 3519: severity=HIGH; category=no_retry_logic
  Description: http_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: if (request.contains("max_history_events") && !request["max_history_events"].is_number_unsigned()) {
- Line 3524: severity=HIGH; category=no_retry_logic
  Description: http_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: std::string instance_id = request.value("process_instance_id", "");
- Line 3525: severity=HIGH; category=no_retry_logic
  Description: http_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: bool include_variables = request.value("include_variables", true);
- Line 3526: severity=HIGH; category=no_retry_logic
  Description: http_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: bool include_history = request.value("include_history", false);
- Line 3527: severity=HIGH; category=no_retry_logic
  Description: http_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: std::size_t max_history_events = request.value("max_history_events", kDefaultBpmnHistoryEvents);
- Line 3619: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: event["data"] = json::object();
- Line 3620: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: event["data"]["node_id"] = node;
- Line 3630: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: response["history"] = json::array();
- Line 88: severity=MEDIUM; category=legacy_duplication; pattern=duplicate_qualified_signature
  Description: Potential duplicate implementation signature across files: std::all_of(value.begin(), value.end(), [](unsigned char ch)
  Context: return std::all_of(value.begin(), value.end(), [](unsigned char ch) {
  Confidence: band=medium; score=0.56
- Line 242: severity=MEDIUM; category=observability; pattern=missing_health_check
  Description: Service initialization without nearby health/status handling
  Context: void WireProtocolServer::start() {
  Confidence: band=medium; score=0.66
- Line 441: severity=MEDIUM; category=expensive_inner_op
  Description: I/O operation in inner loop — very expensive
  Remediation: Buffer output or move I/O outside loop
  Context: std::cerr << "[WireProtocol] IO thread error: " << e.what() << std::endl;
- Line 443: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: io_threads_.emplace_back([this]() {
  Confidence: band=high; score=0.74
- Line 453: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: acceptor_->close();
- Line 642: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: session->socket_.close(close_ec);
- Line 664: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: session->socket_.close(close_ec);
- Line 703: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: close();
- Line 705: severity=MEDIUM; category=observability; pattern=missing_health_check
  Description: Service initialization without nearby health/status handling
  Context: , client_ip_("unknown")  // Will be set after accept in start()
  Confidence: band=medium; score=0.66
- Line 713: severity=MEDIUM; category=observability; pattern=missing_health_check
  Description: Service initialization without nearby health/status handling
  Context: void WireProtocolServer::Session::start() {
  Confidence: band=medium; score=0.66
- Line 729: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: void WireProtocolServer::Session::close() {
- Line 732: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: socket_.close();
- Line 779: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: socket_.close();
- Line 873: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: socket_.close();
- Line 943: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: close();
- Line 1207: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: close();
- Line 1214: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: close();
- Line 1222: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: close();
- Line 1676: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: client_keys.push_back(key);
- Line 1677: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: storage_keys.push_back(collection + ":" + key);
- Line 1682: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: client_keys.push_back(key);
  Confidence: band=high; score=0.74
- Line 1702: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: results.push_back(std::move(item));
- Line 1708: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results.push_back(std::move(item));
  Confidence: band=high; score=0.74
- Line 1792: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: results.push_back(std::move(r));
- Line 1798: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results.push_back(std::move(r));
  Confidence: band=high; score=0.74
- Line 1801: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: results.push_back(std::move(r));
- Line 1861: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: results.push_back(std::move(item_result));
- Line 1867: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results.push_back(std::move(item_result));
  Confidence: band=high; score=0.74
- Line 1867: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results.push_back(std::move(item_result));
  Confidence: band=high; score=0.74
- Line 1919: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: const auto timeout_ms = request["timeout_ms"].get<int64_t>();
  Confidence: band=high; score=0.74
- Line 2200: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: vertices.push_back(std::move(v));
- Line 2206: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: vertices.push_back(std::move(v));
  Confidence: band=high; score=0.74
- Line 2227: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: void WireProtocolServer::Session::handleQuery() {
  Confidence: band=high; score=0.74
- Line 2300: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: first_batch.push_back(result_json[i]);
- Line 2306: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: first_batch.push_back(result_json[i]);
  Confidence: band=high; score=0.74
- Line 2418: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: batch.push_back(entry.results[i]);
- Line 2424: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: batch.push_back(entry.results[i]);
  Confidence: band=high; score=0.74
- Line 2551: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: query_vector.push_back(value);
- Line 2557: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: query_vector.push_back(value);
  Confidence: band=high; score=0.74
- Line 2586: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: hits.push_back(std::move(hit));
- Line 2592: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: hits.push_back(std::move(hit));
  Confidence: band=high; score=0.74
- Line 2608: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: void WireProtocolServer::Session::handleGeoQuery() {
  Confidence: band=high; score=0.74
- Line 2863: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: results_arr.push_back(std::move(entry));
- Line 2869: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results_arr.push_back(std::move(entry));
  Confidence: band=high; score=0.74
- Line 2889: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: void WireProtocolServer::Session::handleTimeseriesQuery() {
  Confidence: band=high; score=0.74
- Line 3021: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: bucket_data[bucket_start_ms].push_back(point.value);
- Line 3027: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: bucket_data[bucket_start_ms].push_back(point.value);
  Confidence: band=high; score=0.74
- Line 3152: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: response.buckets.push_back(bucket);
- Line 3158: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: response.buckets.push_back(bucket);
  Confidence: band=high; score=0.74
- Line 3172: severity=MEDIUM; category=endianness_assumption
  Description: Endianness conversion may have undefined behavior
  Remediation: Document endianness assumptions or use portable serialization
  Context: uint32_t magic = htonl(0x544D4442);  // "TMDB" in network byte order
- Line 3181: severity=MEDIUM; category=endianness_assumption
  Description: Endianness conversion may have undefined behavior
  Remediation: Document endianness assumptions or use portable serialization
  Context: uint32_t payload_size_net = htonl(payload_size);  // Convert to network byte order
- Line 3214: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: void WireProtocolServer::Session::handleBpmnStartProcess() {
  Confidence: band=high; score=0.74
- Line 3319: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: active_tasks.push_back(instance_id + ":" + token.current_node);
- Line 3325: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: active_tasks.push_back(instance_id + ":" + token.current_node);
  Confidence: band=high; score=0.74
- Line 3587: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: active_tasks.push_back(task);
- Line 3593: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: active_tasks.push_back(task);
  Confidence: band=high; score=0.74
- Line 3621: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: history.push_back(event);
- Line 3627: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: history.push_back(event);
  Confidence: band=high; score=0.74
- Line 3627: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: history.push_back(event);
  Confidence: band=high; score=0.74

### src/network/envoy_xds.cpp
Total findings: 61

- Line 639: severity=CRITICAL; category=no_timeout
  Description: file_io without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: http::read(stream, buf, res);
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
- Line 7: severity=HIGH; category=legacy_duplication; pattern=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Context: * PR: #3395 Add service mesh sidecar proxy mode with Envoy xDS v3 compatibility (2026-03-12T07:09:06Z)
  Confidence: band=high; score=0.8
- Line 57: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: std::snprintf(buf, sizeof(buf), "\\u%04x", c);
  Confidence: band=very_high; score=0.9
- Line 57: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Use <cstdint> types (uint32_t, uint64_t) and sizeof() checks, not constants
  Context: std::snprintf(buf, sizeof(buf), "\\u%04x", c);
- Line 256: severity=HIGH; category=deadlock_risk
  Description: Multiple locks acquired in nested scope — potential deadlock
  Remediation: Use std::scoped_lock(m1, m2) or enforce consistent lock ordering
  Context: std::lock_guard<std::mutex> lk(callbacks_mutex_);
- Line 261: severity=HIGH; category=deadlock_risk
  Description: Multiple locks acquired in nested scope — potential deadlock
  Remediation: Use std::scoped_lock(m1, m2) or enforce consistent lock ordering
  Context: std::lock_guard<std::mutex> lk(callbacks_mutex_);
- Line 280: severity=HIGH; category=deadlock_risk
  Description: Multiple locks acquired in nested scope — potential deadlock
  Remediation: Use std::scoped_lock(m1, m2) or enforce consistent lock ordering
  Context: std::lock_guard<std::mutex> lk(stats_mutex_);
- Line 285: severity=HIGH; category=deadlock_risk
  Description: Multiple locks acquired in nested scope — potential deadlock
  Remediation: Use std::scoped_lock(m1, m2) or enforce consistent lock ordering
  Context: std::lock_guard<std::mutex> lk(versions_mutex_);
- Line 290: severity=HIGH; category=deadlock_risk
  Description: Multiple locks acquired in nested scope — potential deadlock
  Remediation: Use std::scoped_lock(m1, m2) or enforce consistent lock ordering
  Context: std::lock_guard<std::mutex> lk(versions_mutex_);
- Line 382: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: for (const auto& item : splitJsonArray(body)) {
- Line 417: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: for (const auto& item : splitJsonArray(body)) {
- Line 430: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: for (const auto& locality_ep : splitJsonArray(eps_body)) {
- Line 434: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: for (const auto& lbep : splitJsonArray(lb_body)) {
- Line 474: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: for (const auto& item : splitJsonArray(body)) {
- Line 481: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: for (const auto& locality_ep : splitJsonArray(eps_body)) {
- Line 485: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: for (const auto& lbep : splitJsonArray(lb_body)) {
- Line 524: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: for (const auto& rc : splitJsonArray(body)) {
- Line 530: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: for (const auto& vh : splitJsonArray(vhosts_body)) {
- Line 540: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: const auto q1 = dom_body.find('"', pos);
- Line 541: severity=HIGH; category=performance; pattern=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Context: const auto q1 = dom_body.find('"', pos);
  Confidence: band=very_high; score=0.9
- Line 561: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: for (const auto& rt : splitJsonArray(routes_body)) {
- Line 625: severity=HIGH; category=no_retry_logic
  Description: socket_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: stream.connect(results);
- Line 717: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: while (running_.load(std::memory_order_acquire)) {
- Line 830: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: wake_cv_.wait_for(lk, std::chrono::milliseconds(wait_ms),
- Line 831: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: [this] { return !running_.load(std::memory_order_acquire); });
- Line 48: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: case '"':  out += "\\\""; break;
  Confidence: band=high; score=0.74
- Line 49: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: case '"':  out += "\\\""; break;
- Line 50: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: case '\\': out += "\\\\"; break;
- Line 51: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: case '\n': out += "\\n";  break;
- Line 52: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: case '\r': out += "\\r";  break;
- Line 53: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: case '\t': out += "\\t";  break;
- Line 57: severity=MEDIUM; category=expensive_inner_op
  Description: I/O operation in inner loop — very expensive
  Remediation: Buffer output or move I/O outside loop
  Context: std::snprintf(buf, sizeof(buf), "\\u%04x", c);
- Line 81: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: case '"': value += '"'; break;
  Confidence: band=high; score=0.74
- Line 82: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: case '"': value += '"'; break;
- Line 83: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: case '\\': value += '\\'; break;
- Line 84: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: case 'n': value += '\n'; break;
- Line 85: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: case 'r': value += '\r'; break;
- Line 86: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: case 't': value += '\t'; break;
- Line 169: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: items.push_back(array_body.substr(item_start, i - item_start + 1));
  Confidence: band=high; score=0.74
- Line 170: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: items.push_back(array_body.substr(item_start, i - item_start + 1));
- Line 184: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {}
- Line 194: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {}
- Line 398: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(std::move(info));
  Confidence: band=high; score=0.74
- Line 399: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.push_back(std::move(info));
- Line 447: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: info.endpoints.push_back(std::move(ep));
  Confidence: band=high; score=0.74
- Line 447: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: info.endpoints.push_back(std::move(ep));
  Confidence: band=high; score=0.74
- Line 448: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: info.endpoints.push_back(std::move(ep));
- Line 498: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: info.endpoints.push_back(std::move(ep));
  Confidence: band=high; score=0.74
- Line 498: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: info.endpoints.push_back(std::move(ep));
  Confidence: band=high; score=0.74
- Line 499: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: info.endpoints.push_back(std::move(ep));
- Line 551: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: if (!domain.empty()) info.domains.push_back(domain);
  Confidence: band=high; score=0.74
- Line 552: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: if (!domain.empty()) info.domains.push_back(domain);
- Line 589: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {}
- Line 594: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: info.routes.push_back(std::move(route));
- Line 600: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.push_back(std::move(info));

### src/network/quic_server.cpp
Total findings: 54

- Line 95: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: if (RAND_bytes(cid->data, static_cast<int>(cid->datalen)) != 1) {
- Line 566: severity=CRITICAL; category=smart_ptr_misuse
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
- Line 94: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: cid->datalen = NGTCP2_MIN_CIDLEN;
- Line 176: severity=HIGH; category=legacy_duplication; pattern=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Context: // Advertise HTTP/3 ALPN ("h3") for compatibility with standard HTTP/3
  Confidence: band=high; score=0.8
- Line 239: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: if (running_.load(std::memory_order_acquire)) {
- Line 284: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (std::size_t i = 0; i < n; ++i) {
  Confidence: band=very_high; score=0.9
- Line 358: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: if (running_.load(std::memory_order_acquire)) {
- Line 389: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: void* tls = ngtcp2_conn_get_tls_native_handle(it->second);
- Line 454: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: void* /*user_data*/) -> int {
- Line 459: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: callbacks.recv_crypto_data = [](ngtcp2_conn*            conn,
- Line 464: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: void*                   /*user_data*/) -> int {
- Line 479: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: callbacks.client_initial = [](ngtcp2_conn* /*conn*/, void* /*user_data*/) -> int {
- Line 484: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: void*         user_data) -> int {
- Line 501: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: void*          /*stream_user_data*/) -> int {
- Line 517: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: void*        user_data) -> int {
- Line 604: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: if (open_.load(std::memory_order_acquire)) {
- Line 610: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: if (!open_.load(std::memory_order_acquire)) {
- Line 654: severity=HIGH; category=no_retry_logic
  Description: socket_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: disconnect();
- Line 701: severity=HIGH; category=no_retry_logic
  Description: socket_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: void QUICClient::connect() {
- Line 709: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error(
- Line 720: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error("[QUICClient] SSL_CTX_new failed");
- Line 751: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: void* /*user_data*/) -> int {
- Line 752: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: cid->datalen = NGTCP2_MIN_CIDLEN;
- Line 753: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: RAND_bytes(cid->data, static_cast<int>(cid->datalen));
- Line 757: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: callbacks.recv_crypto_data = [](ngtcp2_conn*            conn,
- Line 762: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: void*                   /*user_data*/) -> int {
- Line 797: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error(
- Line 846: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: if (!connected_.load(std::memory_order_acquire)) {
- Line 847: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error("[QUICClient] openStream called when not connected");
- Line 139: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: SSL_CTX_free(ssl_ctx_);
- Line 164: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: SSL_CTX_free(ctx);
- Line 172: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: SSL_CTX_free(ctx);
- Line 179: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: "\x02h3"          // "h3"   (length 2)
- Line 180: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: "\x04tmdb";       // "tmdb" (length 4)
- Line 238: severity=MEDIUM; category=observability; pattern=missing_health_check
  Description: Service initialization without nearby health/status handling
  Context: void QUICServer::start() {
  Confidence: band=medium; score=0.66
- Line 284: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: threads_.emplace_back([this] { io_ctx_->run(); });
  Confidence: band=high; score=0.74
- Line 301: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: SSL_free(static_cast<SSL*>(tls));
- Line 311: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: socket_->close(ec);
- Line 324: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: SSL_CTX_free(ssl_ctx_);
- Line 391: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: SSL_free(static_cast<SSL*>(tls));
- Line 542: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: SSL_free(ssl);
- Line 556: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: SSL_free(ssl);
- Line 605: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: close();
- Line 632: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: void QUICClient::Stream::close() {
- Line 657: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: SSL_CTX_free(ssl_ctx_);
- Line 804: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: ssl_ctx_ = ssl_ctx_guard.release();
- Line 805: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: (void)ssl_guard.release();
- Line 822: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: stream->close();
- Line 831: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: SSL_free(static_cast<SSL*>(tls));
- Line 838: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: SSL_CTX_free(ssl_ctx_);

### src/network/wire_protocol_connection_pool.cpp
Total findings: 53

- Line 254: severity=CRITICAL; category=no_timeout
  Description: semaphore_wait without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: timer.async_wait([&](const boost::system::error_code& error) {
- Line 309: severity=CRITICAL; category=no_timeout
  Description: semaphore_wait without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: timer.async_wait([&](const boost::system::error_code& error) {
- Line 412: severity=CRITICAL; category=no_timeout
  Description: mutex_lock without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: lock.lock();
- Line 540: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: pool->available = std::move(fresh_queue);
- Line 588: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: size_t ideal = config_.adaptive_strategy->getIdealConnectionCount(
- Line 136: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: while (!shutdown_.load(std::memory_order_acquire)) {
- Line 138: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Acquire lock before loop or redesign to minimize lock time
  Context: std::unique_lock<std::mutex> lock(shutdown_mutex_);
- Line 139: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: if (shutdown_cv_.wait_for(lock, std::chrono::seconds(10),
- Line 172: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument("Invalid target format. Expected 'host:port'");
- Line 216: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error("mTLS requires CA certificate path (ssl_ca_cert_path)");
- Line 222: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error(
- Line 262: severity=HIGH; category=no_retry_logic
  Description: socket_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: net::async_connect(*plain_socket, endpoints,
- Line 274: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error("Connection timed out after " +
- Line 277: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error("Connection failed: " + ec.message());
- Line 289: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error("SSL context not initialized");
- Line 297: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: if (!SSL_set_tlsext_host_name(ssl_stream->native_handle(), host.c_str())) {
- Line 298: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error("Failed to set SNI hostname");
- Line 329: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error("SSL handshake timed out after " +
- Line 332: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error("SSL handshake failed: " + ec.message());
- Line 366: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: WireProtocolConnectionPool::acquireConnection(const std::string& target) {
- Line 370: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: auto deadline = std::chrono::steady_clock::now() + config_.acquire_timeout;
- Line 400: severity=HIGH; category=allocation_loop
  Description: Dynamic allocation in loop — high overhead
  Remediation: Pre-allocate outside loop or use object pool pattern
  Context: // Try to create new connection if under limit
- Line 422: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: std::this_thread::sleep_for(std::chrono::milliseconds(100));
- Line 486: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (size_t i = 0; i < config_.min_connections_per_target; ++i) {
  Confidence: band=very_high; score=0.9
- Line 498: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Acquire lock before loop or redesign to minimize lock time
  Context: std::lock_guard<std::mutex> lock(pool->mutex);
- Line 515: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (auto& [target, pool] : target_pools_) {
  Confidence: band=very_high; score=0.9
- Line 516: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Acquire lock before loop or redesign to minimize lock time
  Context: std::lock_guard<std::mutex> lock(pool->mutex);
- Line 553: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto& [t, _] : target_pools_) {
  Confidence: band=very_high; score=0.9
- Line 558: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto& target : targets) {
  Confidence: band=very_high; score=0.9
- Line 649: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (auto& [target, pool] : target_pools_) {
  Confidence: band=very_high; score=0.9
- Line 650: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Acquire lock before loop or redesign to minimize lock time
  Context: std::lock_guard<std::mutex> pool_lock(pool->mutex);
- Line 653: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: for (auto& [socket_ptr, conn] : pool->all_connections) {
- Line 653: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: for (auto& [socket_ptr, conn] : pool->all_connections) {
- Line 678: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: stats.acquire_timeouts = acquire_timeouts_.load(std::memory_order_relaxed);
- Line 687: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto& [target, pool] : target_pools_) {
  Confidence: band=very_high; score=0.9
- Line 688: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Acquire lock before loop or redesign to minimize lock time
  Context: std::lock_guard<std::mutex> pool_lock(pool->mutex);
- Line 722: severity=HIGH; category=no_retry_logic
  Description: socket_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: socket.plain_socket()->receive(net::buffer(&dummy, 1), tcp::socket::message_peek, ec);
- Line 722: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: socket.plain_socket()->receive(net::buffer(&dummy, 1), tcp::socket::message_peek, ec);
- Line 103: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: void SocketWrapper::close(boost::system::error_code& ec) {
- Line 105: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: plain_socket_->close(ec);
- Line 110: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: ssl_socket_->lowest_layer().close(ec);
- Line 257: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: plain_socket->close(ec);
- Line 313: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: ssl_stream->lowest_layer().close(shutdown_ec);
- Line 391: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: conn->socket->close(ec);
- Line 463: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: conn->socket->close(ec);
- Line 506: severity=MEDIUM; category=expensive_inner_op
  Description: I/O operation in inner loop — very expensive
  Remediation: Buffer output or move I/O outside loop
  Context: std::cerr << "[WireProtocolConnectionPool] Warmup failed for " << target
- Line 531: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: conn->socket->close(ec);
- Line 553: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: targets.push_back(t);
  Confidence: band=high; score=0.74
- Line 554: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: targets.push_back(t);
- Line 636: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: conn->socket->close(ec);
- Line 656: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: conn->socket->close(ec);
- Line 748: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: WireProtocolConnectionPool::ConnectionHandle::~ConnectionHandle() {
  Confidence: band=high; score=0.74
- Line 754: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: WireProtocolConnectionPool::ConnectionHandle::ConnectionHandle(ConnectionHandle&& other) noexcept
  Confidence: band=high; score=0.74

### src/network/kernel_bypass.cpp
Total findings: 50

- Line 0: severity=CRITICAL; category=uncategorized
  Confidence: band=very_high; score=0.85
- Line 0: severity=CRITICAL; category=uncategorized
  Context: Pointer declared but not initialized
  Confidence: band=very_high; score=0.93
- Line 706: severity=CRITICAL; category=missing_dtor
  Description: Class io_uring_params allocates resources but has no destructor
  Remediation: Add explicit destructor: ~io_uring_params() { /* cleanup */ }
  Context: class/struct io_uring_params
- Line 31: severity=HIGH; category=posix_only_api
  Description: POSIX-only API pthread_ without platform guard
  Remediation: Wrap in #ifndef _WIN32 ... #endif or provide Windows alternative
  Context: * - CpuPinner uses sched_setaffinity / pthread_setaffinity_np (Linux only).
- Line 127: severity=HIGH; category=posix_only_api
  Description: POSIX-only API pthread_ without platform guard
  Remediation: Wrap in #ifndef _WIN32 ... #endif or provide Windows alternative
  Context: return ::pthread_setaffinity_np(thread.native_handle(),
- Line 142: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: std::snprintf(path, sizeof(path),
  Confidence: band=very_high; score=0.9
- Line 194: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: void* NumaAllocator::allocate(size_t size, int node) {
- Line 194: severity=HIGH; category=observability; pattern=missing_trace_point
  Description: Critical function allocate without trace point
  Context: void* NumaAllocator::allocate(size_t size, int node) {
  Confidence: band=very_high; score=0.9
- Line 196: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: if (size == 0) throw std::bad_alloc{};
- Line 205: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: if (!p) throw std::bad_alloc{};
- Line 218: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: if (!p) throw std::bad_alloc{};
- Line 223: severity=HIGH; category=observability; pattern=missing_trace_point
  Description: Critical function deallocate without trace point
  Context: void NumaAllocator::deallocate(void* ptr, size_t size) noexcept {
  Confidence: band=very_high; score=0.9
- Line 275: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Use <cstdint> types (uint32_t, uint64_t) and sizeof() checks, not constants
  Context: &nodemask, sizeof(nodemask) * 8 + 1, /* MPOL_MF_MOVE */ 2);
- Line 344: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: NumaAllocator::deallocate(data_, size_);
- Line 429: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: std::snprintf(mask_str, sizeof(mask_str), "0x%llX",
  Confidence: band=very_high; score=0.9
- Line 633: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (uint16_t i = 0; i < nb_rx; ++i) {
  Confidence: band=very_high; score=0.9
- Line 738: severity=HIGH; category=no_retry_logic
  Description: socket_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: int fd = ::socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
- Line 740: severity=HIGH; category=no_retry_logic
  Description: socket_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: last_error_ = "socket() failed: " + std::string(std::strerror(errno));
- Line 967: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (auto& t : workers_) {
  Confidence: band=very_high; score=0.9
- Line 127: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: return ::pthread_setaffinity_np(thread.native_handle(),
  Confidence: band=high; score=0.74
- Line 183: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(i);
  Confidence: band=high; score=0.74
- Line 184: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.push_back(i);
- Line 227: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: ::numa_free(ptr, size);
- Line 230: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: _aligned_free(ptr);
- Line 233: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: ::free(ptr);
- Line 383: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: cores.push_back(i);
  Confidence: band=high; score=0.74
- Line 384: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: cores.push_back(i);
- Line 419: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: eal_arg_strs.push_back("themisdb");
- Line 423: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: eal_arg_strs.push_back("-m");
- Line 424: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: eal_arg_strs.push_back(std::to_string(config_.huge_pages_mb));
- Line 443: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: eal_argv.push_back(const_cast<char*>(s.c_str()));
  Confidence: band=high; score=0.74
- Line 444: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: eal_argv.push_back(const_cast<char*>(s.c_str()));
- Line 514: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: rte_mempool_free(mbuf_pool);
- Line 529: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: rte_mempool_free(mbuf_pool);
- Line 545: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: rte_mempool_free(mbuf_pool);
- Line 556: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: rte_mempool_free(mbuf_pool);
- Line 572: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: workers_.emplace_back([this, core, qid]() {
  Confidence: band=high; score=0.74
- Line 646: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: rte_pktmbuf_free(m);
- Line 690: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: ::close(fd);
- Line 710: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: ::close(fd);
- Line 759: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: ::close(fd);
- Line 765: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: ::close(fd);
- Line 815: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: iovecs.push_back({buf->data(), buf->size()});
  Confidence: band=high; score=0.74
- Line 816: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: iovecs.push_back({buf->data(), buf->size()});
- Line 817: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: fixed_bufs_.push_back(std::move(buf));
- Line 862: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: ::close(listen_fd_);
- Line 871: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: workers_.emplace_back([this, i]() { workerLoop(static_cast<int>(i)); });
  Confidence: band=high; score=0.74
- Line 941: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: ::close(listen_fd_);
- Line 945: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: ::close(ring_fd_);
- Line 962: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: ::close(ring_fd_);

### src/network/qos_manager.cpp
Total findings: 40

- Line 351: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto it = tenant_assignments_.find(connection_id);
- Line 397: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: state->token_bucket = std::make_shared<TokenBucket>(rate_bps, burst);
- Line 455: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: state->leaky_bucket = std::make_shared<LeakyBucket>(drain_rate_bps, capacity_bytes);
- Line 578: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: state->congestion_ctrl = std::make_shared<CongestionController>();
- Line 594: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: state->congestion_ctrl = std::make_shared<CongestionController>();
- Line 806: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: uint64_t in_flight = state->queue_depth.load(std::memory_order_relaxed);
- Line 829: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: uint64_t current = state->queue_depth.load(std::memory_order_relaxed);
- Line 901: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: cs.priority             = static_cast<Priority>(state->priority.load(std::memory_order_relaxed));
- Line 905: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: cs.queue_depth          = state->queue_depth.load(std::memory_order_relaxed);
- Line 988: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: ts->token_bucket = std::make_shared<TokenBucket>(rate_bps, burst > 0 ? burst : 1);
- Line 994: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: ts->token_bucket = std::make_shared<TokenBucket>(rate_bps, burst > 0 ? burst : 1);
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 98: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: std::this_thread::sleep_for(std::chrono::milliseconds(wait_ms));
  Confidence: band=very_high; score=0.9
- Line 98: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: std::this_thread::sleep_for(std::chrono::milliseconds(wait_ms));
- Line 100: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: std::this_thread::sleep_for(std::chrono::milliseconds(1));
  Confidence: band=very_high; score=0.9
- Line 100: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: std::this_thread::sleep_for(std::chrono::milliseconds(1));
- Line 206: severity=HIGH; category=deadlock_risk
  Description: Multiple locks acquired in nested scope — potential deadlock
  Remediation: Use std::scoped_lock(m1, m2) or enforce consistent lock ordering
  Context: std::lock_guard<std::mutex> lock(mutex_);
- Line 269: severity=HIGH; category=deadlock_risk
  Description: Multiple locks acquired in nested scope — potential deadlock
  Remediation: Use std::scoped_lock(m1, m2) or enforce consistent lock ordering
  Context: std::lock_guard<std::mutex> lock(mutex_);
- Line 274: severity=HIGH; category=deadlock_risk
  Description: Multiple locks acquired in nested scope — potential deadlock
  Remediation: Use std::scoped_lock(m1, m2) or enforce consistent lock ordering
  Context: std::lock_guard<std::mutex> lock(mutex_);
- Line 535: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (std::deque<PendingSend>* q : queues) {
  Confidence: band=very_high; score=0.9
- Line 627: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: //     prevent shell metacharacter injection via snprintf.
  Confidence: band=very_high; score=0.9
- Line 659: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: std::snprintf(cmd, sizeof(cmd),
  Confidence: band=very_high; score=0.9
- Line 668: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: std::snprintf(cmd, sizeof(cmd),
  Confidence: band=very_high; score=0.9
- Line 680: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: std::snprintf(cmd, sizeof(cmd),
  Confidence: band=very_high; score=0.9
- Line 893: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: QoSManager::getConnectionStats(uint64_t connection_id) const {
- Line 912: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: cs.has_token_bucket = (state->token_bucket != nullptr);
- Line 912: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: cs.has_token_bucket = (state->token_bucket != nullptr);
- Line 936: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto& [id, _] : connections_) {
  Confidence: band=very_high; score=0.9
- Line 943: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (uint64_t id : ids) {
  Confidence: band=very_high; score=0.9
- Line 944: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: result.push_back(getConnectionStats(id));
- Line 43: severity=MEDIUM; category=legacy_duplication; pattern=duplicate_qualified_signature
  Description: Potential duplicate implementation signature across files: TokenBucket::refill()
  Context: void TokenBucket::refill() {
  Confidence: band=medium; score=0.56
- Line 937: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: ids.push_back(id);
- Line 943: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(getConnectionStats(id));
  Confidence: band=high; score=0.74
- Line 944: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.push_back(getConnectionStats(id));
- Line 1070: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: ids.push_back(id);
- Line 1076: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(getTenantStats(id));
  Confidence: band=high; score=0.74
- Line 1077: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.push_back(getTenantStats(id));

### src/network/raft_load_balancer.cpp
Total findings: 37

- Line 291: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: if (b->healthy && b->datacenter == config_.datacenter) {
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 99: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: health_check_thread_ = std::thread([this]() { healthCheckLoop(); });
  Confidence: band=very_high; score=0.9
- Line 100: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: raft_thread_         = std::thread([this]() { raftLoop(); });
  Confidence: band=very_high; score=0.9
- Line 126: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto& b : backends_) {
  Confidence: band=very_high; score=0.9
- Line 133: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: backend->datacenter   = datacenter;
- Line 159: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto& b : backends_) {
  Confidence: band=very_high; score=0.9
- Line 160: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: result.push_back(b.get());
  Confidence: band=very_high; score=0.9
- Line 174: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: strategy_.load(std::memory_order_acquire);
- Line 234: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: return role_.load(std::memory_order_acquire) == RaftRole::LEADER;
- Line 238: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: return role_.load(std::memory_order_acquire);
- Line 242: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: return current_term_.load(std::memory_order_acquire);
- Line 258: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto& b : backends_) {
  Confidence: band=very_high; score=0.9
- Line 281: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: if (b->address == address) return b.get();
  Confidence: band=very_high; score=0.9
- Line 292: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: result.push_back(b.get());
  Confidence: band=very_high; score=0.9
- Line 299: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: if (b->healthy) result.push_back(b.get());
  Confidence: band=very_high; score=0.9
- Line 409: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: for (const auto& b : backends_) backends_snapshot.push_back(b.get());
  Confidence: band=very_high; score=0.9
- Line 409: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto& b : backends_) backends_snapshot.push_back(b.get());
  Confidence: band=very_high; score=0.9
- Line 412: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (Backend* b : backends_snapshot) {
  Confidence: band=very_high; score=0.9
- Line 448: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: while (!shutdown_.load(std::memory_order_acquire)) {
- Line 449: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Acquire lock before loop or redesign to minimize lock time
  Context: std::unique_lock<std::mutex> lk(shutdown_mutex_);
- Line 450: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: shutdown_cv_.wait_for(lk, interval,
  Confidence: band=very_high; score=0.9
- Line 451: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: [this] { return shutdown_.load(std::memory_order_acquire); });
- Line 454: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: if (shutdown_.load(std::memory_order_acquire)) break;
- Line 568: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: [this] { return shutdown_.load(std::memory_order_acquire); });
- Line 0: severity=MEDIUM; category=uncategorized
  Confidence: band=medium; score=0.57
- Line 0: severity=MEDIUM; category=uncategorized
  Confidence: band=medium; score=0.57
- Line 133: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: backends_.push_back(std::move(backend));
  Confidence: band=high; score=0.74
- Line 134: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: backends_.push_back(std::move(backend));
- Line 159: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(b.get());
  Confidence: band=high; score=0.74
- Line 160: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.push_back(b.get());
- Line 291: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(b.get());
  Confidence: band=high; score=0.74
- Line 291: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(b.get());
  Confidence: band=high; score=0.74
- Line 292: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.push_back(b.get());
- Line 298: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: if (b->healthy) result.push_back(b.get());
  Confidence: band=high; score=0.74
- Line 299: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: if (b->healthy) result.push_back(b.get());
- Line 409: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: for (const auto& b : backends_) backends_snapshot.push_back(b.get());

### src/network/wire_protocol_server_ws.cpp
Total findings: 30

- Line 692: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: << (prev - 1) << " (limit=" << server_->config_.max_connections
- Line 0: severity=HIGH; category=uncategorized
  Context: ['        const size_t crc_offset = kWireHeaderSize + payload_size;', '        const uint32_t expected_crc =', '            (static_cast<uint32_t>(data[crc_offset]) << 24) |', '            (static_cast<uint32_t>(data[crc_offset + 1]) << 16) |', '            (static_cast<uint32_t>(data[crc_offset + 2]) << 8) |']
  Confidence: band=high; score=0.78
- Line 95: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: crc = kWsCrc32Table[(crc ^ data[i]) & 0xFF] ^ (crc >> 8);
- Line 149: severity=HIGH; category=no_retry_logic
  Description: socket_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: ws_.async_accept(
- Line 264: severity=HIGH; category=no_retry_logic
  Description: socket_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: send(resp.dump());
- Line 270: severity=HIGH; category=no_retry_logic
  Description: socket_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: send(makeError(req_id, "get_response", "Missing 'key' in payload"));
- Line 274: severity=HIGH; category=no_retry_logic
  Description: socket_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: send(makeError(req_id, "get_response", "Storage not available"));
- Line 290: severity=HIGH; category=no_retry_logic
  Description: socket_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: send(resp.dump());
- Line 297: severity=HIGH; category=no_retry_logic
  Description: socket_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: send(makeError(req_id, "put_response", "Missing 'key' in payload"));
- Line 301: severity=HIGH; category=no_retry_logic
  Description: socket_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: send(makeError(req_id, "put_response", "Storage not available"));
- Line 310: severity=HIGH; category=no_retry_logic
  Description: socket_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: send(resp.dump());
- Line 316: severity=HIGH; category=no_retry_logic
  Description: socket_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: send(makeError(req_id, "delete_response", "Missing 'key' in payload"));
- Line 320: severity=HIGH; category=no_retry_logic
  Description: socket_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: send(makeError(req_id, "delete_response", "Storage not available"));
- Line 329: severity=HIGH; category=no_retry_logic
  Description: socket_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: send(resp.dump());
- Line 336: severity=HIGH; category=no_retry_logic
  Description: socket_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: send(makeError(req_id, "query_response",
- Line 342: severity=HIGH; category=no_retry_logic
  Description: socket_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: send(makeError(req_id, "error", "Unknown message type: " + type));
- Line 347: severity=HIGH; category=no_retry_logic
  Description: socket_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: send(makeError(req_id, "error", std::string("Invalid JSON: ") + e.what()));
- Line 528: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: const uint8_t opcode = data[5];
- Line 529: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: const uint16_t flags = (static_cast<uint16_t>(data[6]) << 8) | data[7];
- Line 531: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: (static_cast<uint32_t>(data[8]) << 24) |
- Line 532: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: (static_cast<uint32_t>(data[9]) << 16) |
- Line 533: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: (static_cast<uint32_t>(data[10]) << 8) |
- Line 534: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: static_cast<uint32_t>(data[11]);
- Line 553: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: (static_cast<uint32_t>(data[crc_offset + 2]) << 8) |
- Line 554: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: static_cast<uint32_t>(data[crc_offset + 3]);
- Line 595: severity=HIGH; category=no_retry_logic
  Description: socket_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: void WireProtocolWebSocketSession::send(const std::string& message) {
- Line 122: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 574: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: close();
- Line 655: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: void WireProtocolWebSocketSession::close() {
- Line 661: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: ws_.close(websocket::close_code::normal, ec);

### src/network/quic_transport.cpp
Total findings: 27

- Line 48: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: if (RAND_bytes(cid->data, static_cast<int>(cid->datalen)) != 1) {
- Line 432: severity=CRITICAL; category=smart_ptr_misuse
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
- Line 47: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: cid->datalen = NGTCP2_MIN_CIDLEN;
- Line 181: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (std::size_t i = 0; i < n; ++i) {
  Confidence: band=very_high; score=0.9
- Line 258: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: if (running_.load(std::memory_order_acquire)) {
- Line 289: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: void* tls = ngtcp2_conn_get_tls_native_handle(it->second);
- Line 342: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: void* /*user_data*/) -> int {
- Line 347: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: callbacks.recv_crypto_data = [](ngtcp2_conn*            conn,
- Line 352: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: void*                   /*user_data*/) -> int {
- Line 357: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: void*         user_data) -> int {
- Line 365: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: callbacks.recv_datagram = [](ngtcp2_conn* /*conn*/, uint32_t /*flags*/,
- Line 367: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: void* user_data) -> int {
- Line 370: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: ++transport->stats_.datagrams_received;
- Line 72: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: SSL_CTX_free(ssl_ctx_);
- Line 97: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: SSL_CTX_free(ctx);
- Line 104: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: SSL_CTX_free(ctx);
- Line 181: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: threads_.emplace_back([this] { io_ctx_->run(); });
  Confidence: band=high; score=0.74
- Line 200: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: SSL_free(static_cast<SSL*>(tls_handle));
- Line 210: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: socket_->close(ec);
- Line 223: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: SSL_CTX_free(ssl_ctx_);
- Line 291: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: SSL_free(static_cast<SSL*>(tls));
- Line 408: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: SSL_free(ssl);
- Line 422: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: SSL_free(ssl);

### src/network/wire_protocol_v2.cpp
Total findings: 27

- Line 172: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: hdr.payload_length = static_cast<uint32_t>(payload->size());
- Line 468: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: it->second.send_window += static_cast<int32_t>(inc);
- Line 584: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator pos may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto pos = line.find(':');
- Line 7: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: * PR: #5081 [Docs][themis] Refresh module docs in src/include with runtime beha... (2026-05-13T11:00
- Line 124: severity=HIGH; category=memory_order
  Description: memory_order_relaxed used — potential visibility issue
  Remediation: Use memory_order_acquire/release unless truly lock-free
  Context: oss << "v2conn-" << counter.fetch_add(1, std::memory_order_relaxed);
- Line 271: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto& [id, s] : streams_)
  Confidence: band=very_high; score=0.9
- Line 667: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (auto& t : io_threads_)
  Confidence: band=very_high; score=0.9
- Line 702: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto& [id, s] : sessions_)
  Confidence: band=very_high; score=0.9
- Line 710: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto& [id, s] : sessions_)
  Confidence: band=very_high; score=0.9
- Line 717: severity=HIGH; category=no_retry_logic
  Description: socket_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: acceptor_.async_accept(
- Line 724: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: session->set_disconnect_handler(
- Line 774: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: { impl_->set_data_handler(std::move(h)); }
- Line 774: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: { impl_->set_data_handler(std::move(h)); }
- Line 776: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: { impl_->set_headers_handler(std::move(h)); }
- Line 778: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: { impl_->set_rst_stream_handler(std::move(h)); }
- Line 784: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: return impl_->push_to_client(conn_id, associated_sid, headers, data);
- Line 72: severity=MEDIUM; category=endianness_assumption
  Description: Endianness conversion may have undefined behavior
  Remediation: Document endianness assumptions or use portable serialization
  Context: uint32_t len_be     = htonl32(h.payload_length);
- Line 192: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: const std::unordered_map<std::string, std::string>& headers) override {
  Confidence: band=medium; score=0.66
- Line 207: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: encoded += k + ": " + v + "\n";
  Confidence: band=high; score=0.74
- Line 229: severity=MEDIUM; category=endianness_assumption
  Description: Endianness conversion may have undefined behavior
  Remediation: Document endianness assumptions or use portable serialization
  Context: uint32_t ec_be = htonl32(error_code);
- Line 253: severity=MEDIUM; category=endianness_assumption
  Description: Endianness conversion may have undefined behavior
  Remediation: Document endianness assumptions or use portable serialization
  Context: uint32_t ec_be   = htonl32(error_code);
- Line 264: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: socket_.close();
- Line 654: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: io_threads_.emplace_back([this]() {
  Confidence: band=high; score=0.74
- Line 658: severity=MEDIUM; category=expensive_inner_op
  Description: I/O operation in inner loop — very expensive
  Remediation: Buffer output or move I/O outside loop
  Context: std::cerr << "[WireV2] I/O thread error: " << e.what() << '\n';
- Line 658: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: std::cerr << "[WireV2] I/O thread error: " << e.what() << '\n';
- Line 658: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: std::cerr << "[WireV2] I/O thread error: " << e.what() << '\n';
- Line 670: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: acceptor_.close();

### src/network/wire_protocol_zero_copy.cpp
Total findings: 25

- Line 103: severity=CRITICAL; category=no_timeout
  Description: file_io without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: return ::write(fd, header_.data(), HEADER_SIZE);
- Line 151: severity=CRITICAL; category=no_timeout
  Description: file_io without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: fd_ = ::open(path.c_str(), O_RDONLY | O_CLOEXEC);
- Line 0: severity=HIGH; category=uncategorized
  Context: Array declared without initialization
  Confidence: band=high; score=0.81
- Line 0: severity=HIGH; category=uncategorized
  Context: Array declared without initialization
  Confidence: band=high; score=0.81
- Line 108: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: iov[0].iov_base = const_cast<uint8_t*>(header_.data());
- Line 125: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error("MemoryMappedPayload: open failed: " + path);
- Line 129: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error("MemoryMappedPayload: file is empty: " + path);
- Line 134: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error("MemoryMappedPayload: file too large (> 256 MiB): " + path);
- Line 139: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error("MemoryMappedPayload: allocation failed");
- Line 148: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error("MemoryMappedPayload: read failed: " + path);
- Line 153: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::system_error(errno, std::system_category(),
- Line 161: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error(
- Line 169: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error(
- Line 178: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::system_error(err, std::system_category(),
- Line 189: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument(
- Line 196: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error("MemoryMappedPayload: allocation failed");
- Line 204: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::system_error(errno, std::system_category(),
- Line 145: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: std::free(addr_);
- Line 159: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: ::close(fd_);
- Line 167: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: ::close(fd_);
- Line 176: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: ::close(fd_);
- Line 214: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: std::free(addr_);
- Line 223: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: ::close(fd_);
- Line 243: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: if (addr_ != MAP_FAILED && addr_ != nullptr) std::free(addr_);
- Line 246: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: if (fd_ >= 0) ::close(fd_);

### src/network/socket_timeout_manager.cpp
Total findings: 16

- Line 62: severity=CRITICAL; category=missing_dtor
  Description: Class timeval allocates resources but has no destructor
  Remediation: Add explicit destructor: ~timeval() { /* cleanup */ }
  Context: class/struct timeval
- Line 193: severity=CRITICAL; category=smart_ptr_misuse
  Description: Raw new without immediate wrapping in smart pointer
  Remediation: Use auto ptr = std::make_unique<T>(...);
  Context: spdlog::warn("Circuit breaker is open, refusing new connections");
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 110: severity=HIGH; category=no_retry_logic
  Description: socket_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: if (ioctlsocket(socket, FIONBIO, &mode) != 0) {
- Line 247: severity=HIGH; category=no_retry_logic
  Description: socket_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: socket_t client_socket = accept(server_socket, nullptr, nullptr);
- Line 249: severity=HIGH; category=no_retry_logic
  Description: socket_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: spdlog::error("accept() failed: {} ({})", strerror(errno), errno);
- Line 274: severity=HIGH; category=no_retry_logic
  Description: socket_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: int bytes = recv(socket, static_cast<char*>(buffer), static_cast<int>(size), 0);
- Line 282: severity=HIGH; category=no_retry_logic
  Description: socket_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: spdlog::error("recv() failed: {}", error);
- Line 288: severity=HIGH; category=no_retry_logic
  Description: socket_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: ssize_t bytes = recv(socket, buffer, size, 0);
- Line 295: severity=HIGH; category=no_retry_logic
  Description: socket_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: spdlog::error("recv() failed: {} ({})", strerror(errno), errno);
- Line 319: severity=HIGH; category=no_retry_logic
  Description: socket_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: int bytes = send(socket, static_cast<const char*>(buffer), static_cast<int>(size), 0);
- Line 327: severity=HIGH; category=no_retry_logic
  Description: socket_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: spdlog::error("send() failed: {}", error);
- Line 333: severity=HIGH; category=no_retry_logic
  Description: socket_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: ssize_t bytes = send(socket, buffer, size, MSG_NOSIGNAL);  // MSG_NOSIGNAL prevents SIGPIPE
- Line 340: severity=HIGH; category=no_retry_logic
  Description: socket_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: spdlog::error("send() failed: {} ({})", strerror(errno), errno);
- Line 362: severity=HIGH; category=no_retry_logic
  Description: socket_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: closesocket(socket);
- Line 364: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: close(socket);

### src/network/adaptive_circuit_breaker.cpp
Total findings: 13

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
- Line 34: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument(
- Line 38: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument(
- Line 42: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument(
- Line 55: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: if (state_.load(std::memory_order_acquire) == CircuitState::CLOSED) {
- Line 94: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: const CircuitState s = state_.load(std::memory_order_acquire);
- Line 133: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: const CircuitState s = state_.load(std::memory_order_acquire);
- Line 166: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: return state_.load(std::memory_order_acquire);

### src/network/network_audit_log.cpp
Total findings: 13

- Line 87: severity=CRITICAL; category=array_bounds
  Description: Array bounds violation: loop 64 > array 0
  Remediation: Fix loop condition or increase array size
  Context: uint32_t a=h[0], b=h[1], c=h[2], d=h[3],
- Line 88: severity=CRITICAL; category=array_bounds
  Description: Array bounds violation: loop 64 > array 4
  Remediation: Fix loop condition or increase array size
  Context: e=h[4], f=h[5], g=h[6], hh=h[7];
- Line 101: severity=CRITICAL; category=array_bounds
  Description: Array bounds violation: loop 8 > array 0
  Remediation: Fix loop condition or increase array size
  Context: h[0]+=a; h[1]+=b; h[2]+=c; h[3]+=d;
- Line 102: severity=CRITICAL; category=array_bounds
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
- Line 128: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument(
- Line 248: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto& ev : buffer_) {
  Confidence: band=very_high; score=0.9
- Line 0: severity=MEDIUM; category=uncategorized
  Confidence: band=medium; score=0.57
- Line 0: severity=MEDIUM; category=uncategorized
  Confidence: band=medium; score=0.57
- Line 248: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: if (ev.type == type) result.push_back(ev);
  Confidence: band=high; score=0.74
- Line 249: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: if (ev.type == type) result.push_back(ev);

### src/network/service_mesh.cpp
Total findings: 13

- Line 137: severity=CRITICAL; category=no_timeout
  Description: file_io without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: boost::asio::write(socket, net::buffer(resp), ec);
- Line 148: severity=CRITICAL; category=no_timeout
  Description: file_io without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: boost::asio::write(socket, net::buffer(resp), ec);
- Line 192: severity=CRITICAL; category=no_timeout
  Description: file_io without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: acceptor_->open(ep.protocol());
- Line 160: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: while (running_.load(std::memory_order_acquire)) {
- Line 162: severity=HIGH; category=no_retry_logic
  Description: socket_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: tcp::socket socket(*io_ctx_);
- Line 163: severity=HIGH; category=no_retry_logic
  Description: socket_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: acceptor_->accept(socket, ec);
- Line 114: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: req.find("GET /healthz\r") != std::string::npos);
- Line 117: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: req.find("GET /readyz\r")  != std::string::npos);
- Line 133: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: "HTTP/1.1 404 Not Found\r\n"
- Line 142: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: "HTTP/1.1 200 OK\r\n"
- Line 143: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: "Content-Type: text/plain\r\n"
- Line 176: severity=MEDIUM; category=observability; pattern=missing_health_check
  Description: Service initialization without nearby health/status handling
  Context: bool ServiceMeshIntegration::start() {
  Confidence: band=medium; score=0.66
- Line 238: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: acceptor_->close(ec);

### src/network/udp_fast_path.cpp
Total findings: 12

- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 126: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: if (running_.load(std::memory_order_acquire)) {
- Line 159: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: socket_->send_to(net::buffer(resp), sender, 0, send_ec);
- Line 174: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: const uint8_t opcode = data[3];
- Line 202: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: socket_->send_to(net::buffer(resp), sender, 0, send_ec);
- Line 229: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: const std::size_t sent = socket_->send_to(net::buffer(response), sender,
- Line 71: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: threads_.emplace_back([this] { io_ctx_->run(); });
  Confidence: band=high; score=0.74
- Line 83: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: socket_->close(ec);
- Line 178: severity=MEDIUM; category=endianness_assumption
  Description: Endianness conversion may have undefined behavior
  Remediation: Document endianness assumptions or use portable serialization
  Context: const uint32_t request_id = ntohl(req_id_be);
- Line 392: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: out.push_back(kUdpFastPathRespMagic0);
- Line 393: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: out.push_back(kUdpFastPathRespMagic1);
- Line 399: severity=MEDIUM; category=endianness_assumption
  Description: Endianness conversion may have undefined behavior
  Remediation: Document endianness assumptions or use portable serialization
  Context: const uint32_t req_id_be = htonl(request_id);

### src/network/udp_server.cpp
Total findings: 12

- Line 71: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (std::size_t i = 0; i < n; ++i) {
  Confidence: band=very_high; score=0.9
- Line 138: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: if (running_.load(std::memory_order_acquire)) {
- Line 182: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: const uint8_t opcode = data[3];
- Line 188: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: const uint8_t flags = data[8];
- Line 304: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: socket_->send_to(net::buffer(ack), dest, 0, ec);
- Line 361: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Acquire lock before loop or redesign to minimize lock time
  Context: std::unique_lock<std::mutex> lk(batch_mutex_);
- Line 362: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: batch_cv_.wait_for(lk, interval, [this] { return batch_stop_; });
  Confidence: band=very_high; score=0.9
- Line 396: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto& pkt : local) {
  Confidence: band=very_high; score=0.9
- Line 49: severity=MEDIUM; category=observability; pattern=missing_health_check
  Description: Service initialization without nearby health/status handling
  Context: void UDPServer::start() {
  Confidence: band=medium; score=0.66
- Line 71: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: io_threads_.emplace_back([this] { io_ctx_->run(); });
  Confidence: band=high; score=0.74
- Line 96: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: socket_->close(ec);
- Line 471: severity=MEDIUM; category=endianness_assumption
  Description: Endianness conversion may have undefined behavior
  Remediation: Document endianness assumptions or use portable serialization
  Context: const uint32_t seq_be = htonl(seq_num);

### src/network/io_uring_batcher.cpp
Total findings: 10

- Line 0: severity=CRITICAL; category=uncategorized
  Context: ['    std::memcpy(sqe + 16, &addr,     sizeof(addr));   // addr (iovec ptr)', '    uint32_t len = static_cast<uint32_t>(iov_cnt);', '    std::memcpy(sqe + 24, &len,      sizeof(len));    // len  (iov_cnt)', '    std::memcpy(sqe + 32, &user_data,sizeof(user_data)); // user_data', '']
  Confidence: band=very_high; score=0.9
- Line 209: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: [[maybe_unused]] size_t iov_cnt, [[maybe_unused]] uint64_t user_data) {
- Line 238: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Use <cstdint> types (uint32_t, uint64_t) and sizeof() checks, not constants
  Context: std::memcpy(sqe + 4,  &fd_i,     sizeof(fd_i));  // fd
- Line 240: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Use <cstdint> types (uint32_t, uint64_t) and sizeof() checks, not constants
  Context: std::memcpy(sqe + 16, &addr,     sizeof(addr));   // addr (iovec ptr)
- Line 242: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Use <cstdint> types (uint32_t, uint64_t) and sizeof() checks, not constants
  Context: std::memcpy(sqe + 24, &len,      sizeof(len));    // len  (iov_cnt)
- Line 245: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: sq_.array[tail & *sq_.ring_mask] = tail & *sq_.ring_mask;
- Line 279: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: ::close(ring_fd_); ring_fd_ = -1;
- Line 293: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: ::close(ring_fd_); ring_fd_ = -1;
- Line 309: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: ::close(ring_fd_); ring_fd_ = -1;
- Line 346: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: ::close(ring_fd_);

### src/network/wire_protocol_helpers.cpp
Total findings: 10

- Line 52: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: value |= static_cast<uint64_t>(data_[pos_ + i]) << (i * 8);
- Line 66: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: value |= static_cast<uint32_t>(data_[pos_ + i]) << (i * 8);
- Line 236: severity=HIGH; category=no_retry_logic
  Description: http_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: if (request.collection.empty()) {
- Line 293: severity=HIGH; category=determinism; pattern=fp_exact_comparison
  Description: Floating-point exact comparison (use tolerance/epsilon)
  Context: if (data_density != 0.0) {
  Confidence: band=very_high; score=0.9
- Line 137: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: data_.push_back(static_cast<uint8_t>((value & 0x7F) | 0x80));
- Line 140: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: data_.push_back(static_cast<uint8_t>(value & 0x7F));
- Line 145: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: data_.push_back(static_cast<uint8_t>(value & 0xFF));
  Confidence: band=high; score=0.74
- Line 146: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: data_.push_back(static_cast<uint8_t>(value & 0xFF));
- Line 153: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: data_.push_back(static_cast<uint8_t>(value & 0xFF));
  Confidence: band=high; score=0.74
- Line 154: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: data_.push_back(static_cast<uint8_t>(value & 0xFF));

### src/network/wire_protocol_performance.cpp
Total findings: 10

- Line 223: severity=CRITICAL; category=no_timeout
  Description: semaphore_wait without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: PayloadBufferPool::Handle PayloadBufferPool::acquire() {
- Line 127: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (uint64_t b : buckets_ms) hist[b] = 0;
  Confidence: band=very_high; score=0.9
- Line 129: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (double s : sorted_samples) {
  Confidence: band=very_high; score=0.9
- Line 132: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (uint64_t b : buckets_ms) {
  Confidence: band=very_high; score=0.9
- Line 223: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: PayloadBufferPool::Handle PayloadBufferPool::acquire() {
- Line 185: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: PayloadBufferPool::Handle::Handle(Handle&& o) noexcept
  Confidence: band=high; score=0.74
- Line 192: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: release();
- Line 200: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: void PayloadBufferPool::Handle::release() noexcept {
- Line 208: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: PayloadBufferPool::Handle::~Handle() {
  Confidence: band=high; score=0.74
- Line 209: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: release();

### src/network/grpc_transport.cpp
Total findings: 9

- Line 80: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error("GrpcTransport: cannot open file: " + path);
- Line 123: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error(
- Line 162: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Acquire lock before loop or redesign to minimize lock time
  Context: std::lock_guard<std::mutex> lk(stats_mutex_);
- Line 176: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: if (running_.load(std::memory_order_acquire)) {
- Line 252: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (auto& t : threads_) {
  Confidence: band=very_high; score=0.9
- Line 20: severity=MEDIUM; category=observability; pattern=missing_correlation_id
  Description: Distributed call without correlation ID
  Context: #include <grpcpp/generic/async_generic_service.h>
  Confidence: band=high; score=0.74
- Line 175: severity=MEDIUM; category=observability; pattern=missing_health_check
  Description: Service initialization without nearby health/status handling
  Context: bool GrpcTransport::start() {
  Confidence: band=medium; score=0.66
- Line 205: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: cqs_.push_back(builder.AddCompletionQueue());
- Line 222: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: threads_.emplace_back([this, cq] { drainCompletionQueue(cq); });
  Confidence: band=high; score=0.74

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

### src/network/geo_topology_router.cpp
Total findings: 3

- Line 106: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: (it != config_.region_latency_hints.end()) ? it->second : kUnhintedRegionLatencyMs;
- Line 166: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: selected->region == config_.local_region);
- Line 223: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (std::size_t i = 1; i < shards.size(); ++i) {
  Confidence: band=very_high; score=0.9

### src/network/wire_protocol_batch.cpp
Total findings: 2

- Line 176: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: posix_iov.push_back(item);
  Confidence: band=high; score=0.74
- Line 177: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: posix_iov.push_back(item);

## Update Workflow

- Refresh scan artifacts with: python tools/gap_scanner_v3.py
- Regenerate all module notes with: python tools/module_doc_generator.py . ai_working ai_working/module_gaps
- The generator mirrors each archive document directly into src/<module>/MODULE_GAPS.md.

Format: THEMIS_MODULE_GAPS_V3
