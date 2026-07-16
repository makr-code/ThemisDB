# network Module - Developer Gap Note

> Auto-generated from ai_working\gap_scan_results.json.
> This file is overwritten on each regeneration.

## Scan Snapshot

- Module: network
- Generated: 2026-06-04 08:50:22
- Status: Critical Findings Present
- Total Findings: 493
- Actionable Findings (Critical + High): 356
- Affected Files: 25

## Severity Summary

| Severity | Count |
|---|---:|
| Critical | 63 |
| High | 293 |
| Medium | 128 |
| Low | 9 |

## Category Summary

| Category | Count |
|---|---:|
| no_retry_logic | 111 |
| manual_cleanup | 42 |
| db_connection_leak | 39 |
| resource_leaked_in_exception | 33 |
| uncaught_exception | 20 |
| thread_join_no_timeout | 16 |
| range_temporary | 15 |
| string_concat_loop | 13 |
| null_dereference | 12 |
| hardcoded_output | 10 |
| no_timeout | 10 |
| missing_latency_metric | 9 |
| size_assumption | 9 |
| copy_overhead | 8 |
| endianness_assumption | 8 |
| hardcoded_path | 8 |
| lock_contention | 8 |
| deadlock_risk | 7 |
| generic_catch | 7 |
| manual_cleanup_in_destructor | 7 |
| missing_health_check | 7 |
| unspecified_consistency | 7 |
| data_race | 6 |
| lock_in_loop | 5 |
| primitive_no_volatile | 5 |
| array_bounds | 4 |
| array_bounds_violation | 4 |
| blocking_no_timeout | 4 |
| expensive_inner_op | 4 |
| missing_move_constructor_defaulted | 4 |
| uninitialized_access | 4 |
| command_injection | 3 |
| smart_ptr_misuse | 3 |
| uninitialized_array | 3 |
| duplicate_qualified_signature | 2 |
| memory_order | 2 |
| missing_dtor | 2 |
| missing_trace_point | 2 |
| module_doc_linkset_drift | 2 |
| posix_only_api | 2 |
| shared_state_no_sync | 2 |
| unchecked_array_index | 2 |
| unchecked_malloc | 2 |
| unchecked_memcpy | 2 |
| allocation_loop | 1 |
| arithmetic_overflow | 1 |
| catch_all_swallow | 1 |
| double_lock | 1 |
| exception_in_destructor | 1 |
| explicit_lock_unlock | 1 |
| fp_exact_comparison | 1 |
| legacy_or_compat_path | 1 |
| missing_correlation_id | 1 |
| nested_loop_find | 1 |
| o_n_squared | 1 |
| path_traversal | 1 |
| pointer_arithmetic_unbounded | 1 |
| sensitive_data_logging | 1 |
| stale_doc_section_reference | 1 |
| uninitialized_pointer | 1 |
| unnecessary_copy | 1 |
| unordered_container_iter | 1 |

## File Overview

| File | Findings | Critical | High | Medium | Low |
|---|---:|---:|---:|---:|---:|
| network/wire_protocol_server.cpp | 154 | 10 | 120 | 24 | 0 |
| network/envoy_xds.cpp | 47 | 1 | 23 | 22 | 1 |
| network/wire_protocol_connection_pool.cpp | 34 | 4 | 22 | 8 | 0 |
| network/quic_server.cpp | 33 | 3 | 19 | 11 | 0 |
| network/kernel_bypass.cpp | 29 | 5 | 8 | 14 | 2 |
| network/raft_load_balancer.cpp | 29 | 7 | 19 | 3 | 0 |
| network/wire_protocol_zero_copy.cpp | 24 | 2 | 17 | 5 | 0 |
| network/quic_transport.cpp | 20 | 3 | 11 | 6 | 0 |
| network/qos_manager.cpp | 17 | 3 | 9 | 1 | 4 |
| network/network_audit_log.cpp | 13 | 8 | 3 | 2 | 0 |
| network/service_mesh.cpp | 12 | 4 | 1 | 7 | 0 |
| network/wire_protocol_v2.cpp | 12 | 2 | 2 | 8 | 0 |
| network/adaptive_circuit_breaker.cpp | 10 | 0 | 10 | 0 | 0 |
| network/udp_server.cpp | 10 | 2 | 5 | 3 | 0 |
| network/udp_fast_path.cpp | 8 | 1 | 5 | 2 | 0 |
| network/wire_protocol_performance.cpp | 8 | 2 | 2 | 4 | 0 |
| network/connection_compression.cpp | 7 | 1 | 6 | 0 | 0 |
| network/grpc_transport.cpp | 7 | 1 | 3 | 3 | 0 |
| network/io_uring_batcher.cpp | 5 | 1 | 3 | 1 | 0 |
| network/wire_protocol_server_ws.cpp | 5 | 0 | 2 | 3 | 0 |
| network/socket_timeout_manager.cpp | 4 | 2 | 1 | 1 | 0 |
| network/wire_protocol_helpers.cpp | 2 | 0 | 2 | 0 | 0 |
| network/PRODUCTION_REQUIREMENTS.md | 1 | 0 | 0 | 0 | 1 |
| network/ROADMAP.md | 1 | 0 | 0 | 0 | 1 |
| network/geo_topology_router.cpp | 1 | 1 | 0 | 0 | 0 |

## Full Scanner Findings

### network/wire_protocol_server.cpp
Total findings: 154

- Line 344: severity=CRITICAL; category=sensitive_data_logging
  Description: Potential PII/credential logging: auth
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: std::cerr << "[WireProtocol] Invalid configuration: auth_token length exceeds AUTH "
- Line 424: severity=CRITICAL; category=no_timeout
  Description: file_io without timeout — can block indefinitely
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: acceptor_->open(endpoint.protocol());
- Line 472: severity=CRITICAL; category=thread_join_no_timeout
  Description: Thread join/wait without timeout (blocking indefinitely)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: if (t.joinable()) t.join();
- Line 476: severity=CRITICAL; category=blocking_no_timeout
  Description: Blocking operation without timeout — can block indefinitely
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: }



    if (worker_pool_) {

        worker_pool_->wait();

    }

}
- Line 476: severity=CRITICAL; category=no_timeout
  Description: semaphore_wait without timeout — can block indefinitely
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: worker_pool_->wait();
- Line 480: severity=CRITICAL; category=blocking_no_timeout
  Description: Blocking operation without timeout — can block indefinitely
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: }

}



void WireProtocolServer::wait() {

    for (auto& t : io_threads_) {

        if (t.joinable()) t.join();

    }
- Line 480: severity=CRITICAL; category=no_timeout
  Description: semaphore_wait without timeout — can block indefinitely
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: void WireProtocolServer::wait() {
- Line 482: severity=CRITICAL; category=thread_join_no_timeout
  Description: Thread join/wait without timeout (blocking indefinitely)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: if (t.joinable()) t.join();
- Line 1371: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: const auto& stored = server_->config_.auth_token;
- Line 3087: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: auto agg_result = server_->ts_store_->aggregate(query_opts);
- Line 38: severity=HIGH; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: #include <cstdio>  // For snprintf
- Line 537: severity=HIGH; category=memory_order
  Description: memory_order_relaxed used — potential visibility issue
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: active_connection_count_.load(std::memory_order_relaxed) >= config_.max_connections) {
- Line 624: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: if (!running_.load(std::memory_order_acquire)) return;
- Line 713: severity=HIGH; category=manual_cleanup_in_destructor
  Description: Manual resource cleanup in destructor (should use RAII wrapper)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_raii
  Context: WireProtocolServer::Session::~Session() {
- Line 745: severity=HIGH; category=catch_all_swallow
  Description: catch(...) block swallows errors without rethrowing or explicit handling
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::themis_design_error_rules
  Context: catch(...) { ... }
- Line 796: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: std::memcpy(&flags, &header_buffer_[6], sizeof(uint16_t));
- Line 803: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: std::memcpy(&payload_size, &header_buffer_[8], sizeof(uint32_t));
- Line 890: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: std::memcpy(&flags, &header_buffer_[6], sizeof(uint16_t));
- Line 895: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: std::memcpy(&payload_size, &header_buffer_[8], sizeof(uint32_t));
- Line 1185: severity=HIGH; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: std::snprintf(hex_opcode, sizeof(hex_opcode), "0x%02X", opcode);
- Line 1341: severity=HIGH; category=no_retry_logic
  Description: RPC/network call without retry logic — transient failures will propagate
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: }



            json request = parsePayloadJson(payload_buffer_);

            if (!request.is_object()) {

                sendError(400, "Invalid AUTH payload: expected JSON object");

                return;

            }
- Line 1345: severity=HIGH; category=no_retry_logic
  Description: RPC/network call without retry logic — transient failures will propagate
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: sendError(400, "Invalid AUTH payload: expected JSON object");

                return;

            }

            if (request.contains("token") && !request["token"].is_string()) {

                sendError(400, "Invalid AUTH payload: token must be string");

                return;

            }
- Line 1349: severity=HIGH; category=no_retry_logic
  Description: RPC/network call without retry logic — transient failures will propagate
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: sendError(400, "Invalid AUTH payload: token must be string");

                return;

            }

            if (request.contains("username") && !request["username"].is_string()) {

                sendError(400, "Invalid AUTH payload: username must be string");

                return;

            }
- Line 1354: severity=HIGH; category=no_retry_logic
  Description: RPC/network call without retry logic — transient failures will propagate
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: return;

            }



            token = request.value("token", "");

            username_req = request.value("username", "");



            if (!username_req.empty() && !isReasonableWireIdentifier(username_req)) {
- Line 1355: severity=HIGH; category=no_retry_logic
  Description: RPC/network call without retry logic — transient failures will propagate
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: }



            token = request.value("token", "");

            username_req = request.value("username", "");



            if (!username_req.empty() && !isReasonableWireIdentifier(username_req)) {

                sendError(400, "Invalid AUTH payload: username contains control characters or is too long");
- Line 1433: severity=HIGH; category=no_retry_logic
  Description: RPC/network call without retry logic — transient failures will propagate
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: }



        json request = parsePayloadJson(payload_buffer_);

        if (!request.is_object()) {

            sendError(400, "Invalid GET payload: expected JSON object");

            return;

        }
- Line 1437: severity=HIGH; category=no_retry_logic
  Description: RPC/network call without retry logic — transient failures will propagate
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: sendError(400, "Invalid GET payload: expected JSON object");

            return;

        }

        if ((request.contains("collection") && !request["collection"].is_string()) ||

            (request.contains("key") && !request["key"].is_string())) {

            sendError(400, "Invalid 'collection' or 'key' type in GET request");

            return;
- Line 1438: severity=HIGH; category=no_retry_logic
  Description: RPC/network call without retry logic — transient failures will propagate
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: return;

        }

        if ((request.contains("collection") && !request["collection"].is_string()) ||

            (request.contains("key") && !request["key"].is_string())) {

            sendError(400, "Invalid 'collection' or 'key' type in GET request");

            return;

        }
- Line 1442: severity=HIGH; category=no_retry_logic
  Description: RPC/network call without retry logic — transient failures will propagate
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: sendError(400, "Invalid 'collection' or 'key' type in GET request");

            return;

        }

        std::string collection = request.value("collection", "");

        std::string key = request.value("key", "");



        if (collection.empty() || key.empty() || isBlankString(collection) || isBlankString(key)) {
- Line 1443: severity=HIGH; category=no_retry_logic
  Description: RPC/network call without retry logic — transient failures will propagate
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: return;

        }

        std::string collection = request.value("collection", "");

        std::string key = request.value("key", "");



        if (collection.empty() || key.empty() || isBlankString(collection) || isBlankString(key)) {

            sendError(400, "Missing 'collection' or 'key' in GET request");
- Line 1506: severity=HIGH; category=no_retry_logic
  Description: RPC/network call without retry logic — transient failures will propagate
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: }



        json request = parsePayloadJson(payload_buffer_);

        if (!request.is_object()) {

            sendError(400, "Invalid PUT payload: expected JSON object");

            return;

        }
- Line 1510: severity=HIGH; category=no_retry_logic
  Description: RPC/network call without retry logic — transient failures will propagate
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: sendError(400, "Invalid PUT payload: expected JSON object");

            return;

        }

        if ((request.contains("collection") && !request["collection"].is_string()) ||

            (request.contains("key") && !request["key"].is_string())) {

            sendError(400, "Invalid 'collection' or 'key' type in PUT request");

            return;
- Line 1511: severity=HIGH; category=no_retry_logic
  Description: RPC/network call without retry logic — transient failures will propagate
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: return;

        }

        if ((request.contains("collection") && !request["collection"].is_string()) ||

            (request.contains("key") && !request["key"].is_string())) {

            sendError(400, "Invalid 'collection' or 'key' type in PUT request");

            return;

        }
- Line 1515: severity=HIGH; category=no_retry_logic
  Description: RPC/network call without retry logic — transient failures will propagate
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: sendError(400, "Invalid 'collection' or 'key' type in PUT request");

            return;

        }

        std::string collection = request.value("collection", "");

        std::string key = request.value("key", "");



        if (collection.empty() || key.empty() || isBlankString(collection) || isBlankString(key)) {
- Line 1516: severity=HIGH; category=no_retry_logic
  Description: RPC/network call without retry logic — transient failures will propagate
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: return;

        }

        std::string collection = request.value("collection", "");

        std::string key = request.value("key", "");



        if (collection.empty() || key.empty() || isBlankString(collection) || isBlankString(key)) {

            sendError(400, "Missing 'collection' or 'key' in PUT request");
- Line 1576: severity=HIGH; category=no_retry_logic
  Description: RPC/network call without retry logic — transient failures will propagate
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: }



        json request = parsePayloadJson(payload_buffer_);

        if (!request.is_object()) {

            sendError(400, "Invalid DELETE payload: expected JSON object");

            return;

        }
- Line 1580: severity=HIGH; category=no_retry_logic
  Description: RPC/network call without retry logic — transient failures will propagate
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: sendError(400, "Invalid DELETE payload: expected JSON object");

            return;

        }

        if ((request.contains("collection") && !request["collection"].is_string()) ||

            (request.contains("key") && !request["key"].is_string())) {

            sendError(400, "Invalid 'collection' or 'key' type in DELETE request");

            return;
- Line 1581: severity=HIGH; category=no_retry_logic
  Description: RPC/network call without retry logic — transient failures will propagate
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: return;

        }

        if ((request.contains("collection") && !request["collection"].is_string()) ||

            (request.contains("key") && !request["key"].is_string())) {

            sendError(400, "Invalid 'collection' or 'key' type in DELETE request");

            return;

        }
- Line 1585: severity=HIGH; category=no_retry_logic
  Description: RPC/network call without retry logic — transient failures will propagate
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: sendError(400, "Invalid 'collection' or 'key' type in DELETE request");

            return;

        }

        std::string collection = request.value("collection", "");

        std::string key = request.value("key", "");



        if (collection.empty() || key.empty() || isBlankString(collection) || isBlankString(key)) {
- Line 1586: severity=HIGH; category=no_retry_logic
  Description: RPC/network call without retry logic — transient failures will propagate
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: return;

        }

        std::string collection = request.value("collection", "");

        std::string key = request.value("key", "");



        if (collection.empty() || key.empty() || isBlankString(collection) || isBlankString(key)) {

            sendError(400, "Missing 'collection' or 'key' in DELETE request");
- Line 1634: severity=HIGH; category=no_retry_logic
  Description: RPC/network call without retry logic — transient failures will propagate
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: }



        json request = parsePayloadJson(payload_buffer_);

        if (!request.is_object()) {

            sendError(400, "Invalid BATCH_GET payload: expected JSON object");

            return;

        }
- Line 1638: severity=HIGH; category=no_retry_logic
  Description: RPC/network call without retry logic — transient failures will propagate
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: sendError(400, "Invalid BATCH_GET payload: expected JSON object");

            return;

        }

        if (request.contains("collection") && !request["collection"].is_string()) {

            sendError(400, "Invalid 'collection' type in BATCH_GET request");

            return;

        }
- Line 1642: severity=HIGH; category=no_retry_logic
  Description: RPC/network call without retry logic — transient failures will propagate
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: sendError(400, "Invalid 'collection' type in BATCH_GET request");

            return;

        }

        std::string collection = request.value("collection", "");



        if (collection.empty() || isBlankString(collection)) {

            sendError(400, "Missing 'collection' in BATCH_GET request");
- Line 1652: severity=HIGH; category=no_retry_logic
  Description: RPC/network call without retry logic — transient failures will propagate
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: sendError(400, "Invalid 'collection' in BATCH_GET request");

            return;

        }

        if (!request.contains("keys") || !request["keys"].is_array()) {

            sendError(400, "Missing or invalid 'keys' array in BATCH_GET request");

            return;

        }
- Line 1751: severity=HIGH; category=no_retry_logic
  Description: RPC/network call without retry logic — transient failures will propagate
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: }



        json request = parsePayloadJson(payload_buffer_);

        if (!request.is_object()) {

            sendError(400, "Invalid BATCH_PUT payload: expected JSON object");

            return;

        }
- Line 1755: severity=HIGH; category=no_retry_logic
  Description: RPC/network call without retry logic — transient failures will propagate
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: sendError(400, "Invalid BATCH_PUT payload: expected JSON object");

            return;

        }

        if (request.contains("collection") && !request["collection"].is_string()) {

            sendError(400, "Invalid 'collection' type in BATCH_PUT request");

            return;

        }
- Line 1759: severity=HIGH; category=no_retry_logic
  Description: RPC/network call without retry logic — transient failures will propagate
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: sendError(400, "Invalid 'collection' type in BATCH_PUT request");

            return;

        }

        std::string collection = request.value("collection", "");



        if (collection.empty() || isBlankString(collection)) {

            sendError(400, "Missing 'collection' in BATCH_PUT request");
- Line 1769: severity=HIGH; category=no_retry_logic
  Description: RPC/network call without retry logic — transient failures will propagate
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: sendError(400, "Invalid 'collection' in BATCH_PUT request");

            return;

        }

        if (!request.contains("items") || !request["items"].is_array()) {

            sendError(400, "Missing or invalid 'items' array in BATCH_PUT request");

            return;

        }
- Line 1910: severity=HIGH; category=no_retry_logic
  Description: RPC/network call without retry logic — transient failures will propagate
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: }



        json request = parsePayloadJson(payload_buffer_);

        if (!request.is_object()) {

            sendError(400, "Invalid TRANSACTION_BEGIN payload: expected JSON object");

            return;

        }
- Line 1914: severity=HIGH; category=no_retry_logic
  Description: RPC/network call without retry logic — transient failures will propagate
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: sendError(400, "Invalid TRANSACTION_BEGIN payload: expected JSON object");

            return;

        }

        if (request.contains("isolation_level") && !request["isolation_level"].is_string()) {

            sendError(400, "Invalid 'isolation_level' type in TRANSACTION_BEGIN request");

            return;

        }
- Line 1918: severity=HIGH; category=no_retry_logic
  Description: RPC/network call without retry logic — transient failures will propagate
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: sendError(400, "Invalid 'isolation_level' type in TRANSACTION_BEGIN request");

            return;

        }

        if (request.contains("timeout_ms") && !request["timeout_ms"].is_number_integer()) {

            sendError(400, "Invalid 'timeout_ms' type in TRANSACTION_BEGIN request");

            return;

        }
- Line 1922: severity=HIGH; category=no_retry_logic
  Description: RPC/network call without retry logic — transient failures will propagate
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: sendError(400, "Invalid 'timeout_ms' type in TRANSACTION_BEGIN request");

            return;

        }

        if (request.contains("timeout_ms")) {

            const auto timeout_ms = request["timeout_ms"].get<int64_t>();

            constexpr int64_t kMinTransactionTimeoutMs = 1;

            constexpr int64_t kMaxTransactionTimeoutMs = 3'600'000;
- Line 1931: severity=HIGH; category=no_retry_logic
  Description: RPC/network call without retry logic — transient failures will propagate
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: return;

            }

        }

        std::string isolation_str = request.value("isolation_level", "read_committed");



        IsolationLevel isolation = IsolationLevel::ReadCommitted;

        if (isolation_str == "snapshot" || isolation_str == "repeatable_read") {
- Line 1981: severity=HIGH; category=no_retry_logic
  Description: RPC/network call without retry logic — transient failures will propagate
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: }



        json request = parsePayloadJson(payload_buffer_);

        if (!request.is_object()) {

            sendError(400, "Invalid TRANSACTION_COMMIT payload: expected JSON object");

            return;

        }
- Line 1985: severity=HIGH; category=no_retry_logic
  Description: RPC/network call without retry logic — transient failures will propagate
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: sendError(400, "Invalid TRANSACTION_COMMIT payload: expected JSON object");

            return;

        }

        if (request.contains("transaction_id") && !request["transaction_id"].is_string()) {

            sendError(400, "Invalid 'transaction_id' type in TRANSACTION_COMMIT request");

            return;

        }
- Line 1989: severity=HIGH; category=no_retry_logic
  Description: RPC/network call without retry logic — transient failures will propagate
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: sendError(400, "Invalid 'transaction_id' type in TRANSACTION_COMMIT request");

            return;

        }

        std::string tx_id_str = request.value("transaction_id", "");



        if (tx_id_str.empty() || isBlankString(tx_id_str)) {

            sendError(400, "Missing 'transaction_id' in TRANSACTION_COMMIT request");
- Line 2048: severity=HIGH; category=no_retry_logic
  Description: RPC/network call without retry logic — transient failures will propagate
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: }



        json request = parsePayloadJson(payload_buffer_);

        if (!request.is_object()) {

            sendError(400, "Invalid TRANSACTION_ABORT payload: expected JSON object");

            return;

        }
- Line 2052: severity=HIGH; category=no_retry_logic
  Description: RPC/network call without retry logic — transient failures will propagate
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: sendError(400, "Invalid TRANSACTION_ABORT payload: expected JSON object");

            return;

        }

        if (request.contains("transaction_id") && !request["transaction_id"].is_string()) {

            sendError(400, "Invalid 'transaction_id' type in TRANSACTION_ABORT request");

            return;

        }
- Line 2056: severity=HIGH; category=no_retry_logic
  Description: RPC/network call without retry logic — transient failures will propagate
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: sendError(400, "Invalid 'transaction_id' type in TRANSACTION_ABORT request");

            return;

        }

        std::string tx_id_str = request.value("transaction_id", "");



        if (tx_id_str.empty() || isBlankString(tx_id_str)) {

            sendError(400, "Missing 'transaction_id' in TRANSACTION_ABORT request");
- Line 2111: severity=HIGH; category=no_retry_logic
  Description: RPC/network call without retry logic — transient failures will propagate
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: }



        json request = parsePayloadJson(payload_buffer_);

        if (!request.is_object()) {

            sendError(400, "Invalid GRAPH_TRAVERSE payload: expected JSON object");

            return;

        }
- Line 2115: severity=HIGH; category=no_retry_logic
  Description: RPC/network call without retry logic — transient failures will propagate
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: sendError(400, "Invalid GRAPH_TRAVERSE payload: expected JSON object");

            return;

        }

        if ((request.contains("collection") && !request["collection"].is_string()) ||

            (request.contains("start_vertex") && !request["start_vertex"].is_string())) {

            sendError(400, "Invalid 'collection' or 'start_vertex' type in GRAPH_TRAVERSE request");

            return;
- Line 2116: severity=HIGH; category=no_retry_logic
  Description: RPC/network call without retry logic — transient failures will propagate
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: return;

        }

        if ((request.contains("collection") && !request["collection"].is_string()) ||

            (request.contains("start_vertex") && !request["start_vertex"].is_string())) {

            sendError(400, "Invalid 'collection' or 'start_vertex' type in GRAPH_TRAVERSE request");

            return;

        }
- Line 2120: severity=HIGH; category=no_retry_logic
  Description: RPC/network call without retry logic — transient failures will propagate
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: sendError(400, "Invalid 'collection' or 'start_vertex' type in GRAPH_TRAVERSE request");

            return;

        }

        if ((request.contains("direction") && !request["direction"].is_string()) ||

            (request.contains("edge_type") && !request["edge_type"].is_string())) {

            sendError(400, "Invalid 'direction' or 'edge_type' type in GRAPH_TRAVERSE request");

            return;
- Line 2121: severity=HIGH; category=no_retry_logic
  Description: RPC/network call without retry logic — transient failures will propagate
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: return;

        }

        if ((request.contains("direction") && !request["direction"].is_string()) ||

            (request.contains("edge_type") && !request["edge_type"].is_string())) {

            sendError(400, "Invalid 'direction' or 'edge_type' type in GRAPH_TRAVERSE request");

            return;

        }
- Line 2125: severity=HIGH; category=no_retry_logic
  Description: RPC/network call without retry logic — transient failures will propagate
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: sendError(400, "Invalid 'direction' or 'edge_type' type in GRAPH_TRAVERSE request");

            return;

        }

        if ((request.contains("depth_min") && !request["depth_min"].is_number_integer()) ||

            (request.contains("depth_max") && !request["depth_max"].is_number_integer()) ||

            (request.contains("limit") && !request["limit"].is_number_integer())) {

            sendError(400, "Invalid 'depth_min', 'depth_max', or 'limit' type in GRAPH_TRAVERSE request");
- Line 2126: severity=HIGH; category=no_retry_logic
  Description: RPC/network call without retry logic — transient failures will propagate
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: return;

        }

        if ((request.contains("depth_min") && !request["depth_min"].is_number_integer()) ||

            (request.contains("depth_max") && !request["depth_max"].is_number_integer()) ||

            (request.contains("limit") && !request["limit"].is_number_integer())) {

            sendError(400, "Invalid 'depth_min', 'depth_max', or 'limit' type in GRAPH_TRAVERSE request");

            return;
- Line 2127: severity=HIGH; category=no_retry_logic
  Description: RPC/network call without retry logic — transient failures will propagate
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: }

        if ((request.contains("depth_min") && !request["depth_min"].is_number_integer()) ||

            (request.contains("depth_max") && !request["depth_max"].is_number_integer()) ||

            (request.contains("limit") && !request["limit"].is_number_integer())) {

            sendError(400, "Invalid 'depth_min', 'depth_max', or 'limit' type in GRAPH_TRAVERSE request");

            return;

        }
- Line 2131: severity=HIGH; category=no_retry_logic
  Description: RPC/network call without retry logic — transient failures will propagate
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: sendError(400, "Invalid 'depth_min', 'depth_max', or 'limit' type in GRAPH_TRAVERSE request");

            return;

        }

        std::string collection = request.value("collection", "");

        std::string start_vertex = request.value("start_vertex", "");



        if (collection.empty() || isBlankString(collection)) {
- Line 2132: severity=HIGH; category=no_retry_logic
  Description: RPC/network call without retry logic — transient failures will propagate
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: return;

        }

        std::string collection = request.value("collection", "");

        std::string start_vertex = request.value("start_vertex", "");



        if (collection.empty() || isBlankString(collection)) {

            sendError(400, "Missing 'collection' field in GRAPH_TRAVERSE request");
- Line 2162: severity=HIGH; category=no_retry_logic
  Description: RPC/network call without retry logic — transient failures will propagate
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: }



        // Parse traversal parameters.

        std::string direction_str = request.value("direction", "outbound");

        int depth_min = request.value("depth_min", 1);

        int depth_max = request.value("depth_max", 3);

        int limit     = request.value("limit", 100);
- Line 2163: severity=HIGH; category=no_retry_logic
  Description: RPC/network call without retry logic — transient failures will propagate
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: // Parse traversal parameters.

        std::string direction_str = request.value("direction", "outbound");

        int depth_min = request.value("depth_min", 1);

        int depth_max = request.value("depth_max", 3);

        int limit     = request.value("limit", 100);

        std::string edge_type = request.value("edge_type", "");
- Line 2164: severity=HIGH; category=no_retry_logic
  Description: RPC/network call without retry logic — transient failures will propagate
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: // Parse traversal parameters.

        std::string direction_str = request.value("direction", "outbound");

        int depth_min = request.value("depth_min", 1);

        int depth_max = request.value("depth_max", 3);

        int limit     = request.value("limit", 100);

        std::string edge_type = request.value("edge_type", "");
- Line 2165: severity=HIGH; category=no_retry_logic
  Description: RPC/network call without retry logic — transient failures will propagate
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: std::string direction_str = request.value("direction", "outbound");

        int depth_min = request.value("depth_min", 1);

        int depth_max = request.value("depth_max", 3);

        int limit     = request.value("limit", 100);

        std::string edge_type = request.value("edge_type", "");



        if (!edge_type.empty() && (isBlankString(edge_type) || !isReasonableWireIdentifier(edge_type))) {
- Line 2166: severity=HIGH; category=no_retry_logic
  Description: RPC/network call without retry logic — transient failures will propagate
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: int depth_min = request.value("depth_min", 1);

        int depth_max = request.value("depth_max", 3);

        int limit     = request.value("limit", 100);

        std::string edge_type = request.value("edge_type", "");



        if (!edge_type.empty() && (isBlankString(edge_type) || !isReasonableWireIdentifier(edge_type))) {

            sendError(400, "Invalid 'edge_type' in GRAPH_TRAVERSE request");
- Line 2248: severity=HIGH; category=no_retry_logic
  Description: RPC/network call without retry logic — transient failures will propagate
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: }



        json request = parsePayloadJson(payload_buffer_);

        if (!request.is_object()) {

            sendError(400, "Invalid QUERY payload: expected JSON object");

            return;

        }
- Line 2252: severity=HIGH; category=no_retry_logic
  Description: RPC/network call without retry logic — transient failures will propagate
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: sendError(400, "Invalid QUERY payload: expected JSON object");

            return;

        }

        if (request.contains("query") && !request["query"].is_string()) {

            sendError(400, "Invalid 'query' type in QUERY_AQL request");

            return;

        }
- Line 2256: severity=HIGH; category=no_retry_logic
  Description: RPC/network call without retry logic — transient failures will propagate
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: sendError(400, "Invalid 'query' type in QUERY_AQL request");

            return;

        }

        if (request.contains("batch_size") && !request["batch_size"].is_number_integer()) {

            sendError(400, "Invalid 'batch_size' type in QUERY_AQL request");

            return;

        }
- Line 2260: severity=HIGH; category=no_retry_logic
  Description: RPC/network call without retry logic — transient failures will propagate
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: sendError(400, "Invalid 'batch_size' type in QUERY_AQL request");

            return;

        }

        std::string query_str = request.value("query", "");



        if (query_str.empty()) {

            sendError(400, "Missing 'query' field in QUERY_AQL request");
- Line 2295: severity=HIGH; category=no_retry_logic
  Description: RPC/network call without retry logic — transient failures will propagate
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: json response;

        if (result) {

            const auto& result_json = result.value();

            int batch_size_i = request.value("batch_size", 100);

            constexpr int kMaxQueryBatchSize = 10000;

            if (batch_size_i < 1 || batch_size_i > kMaxQueryBatchSize) {

                sendError(400, "'batch_size' must be between 1 and 10000 in QUERY request");
- Line 2319: severity=HIGH; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: std::snprintf(buf, sizeof(buf), "cursor-%llu-%llu",
- Line 2374: severity=HIGH; category=no_retry_logic
  Description: RPC/network call without retry logic — transient failures will propagate
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: }



        json request = parsePayloadJson(payload_buffer_);

        if (!request.is_object()) {

            sendError(400, "Invalid CURSOR_NEXT payload: expected JSON object");

            return;

        }
- Line 2378: severity=HIGH; category=no_retry_logic
  Description: RPC/network call without retry logic — transient failures will propagate
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: sendError(400, "Invalid CURSOR_NEXT payload: expected JSON object");

            return;

        }

        if (request.contains("cursor_id") && !request["cursor_id"].is_string()) {

            sendError(400, "Invalid 'cursor_id' type in CURSOR_NEXT request");

            return;

        }
- Line 2382: severity=HIGH; category=no_retry_logic
  Description: RPC/network call without retry logic — transient failures will propagate
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: sendError(400, "Invalid 'cursor_id' type in CURSOR_NEXT request");

            return;

        }

        if (request.contains("batch_size") && !request["batch_size"].is_number_integer()) {

            sendError(400, "Invalid 'batch_size' type in CURSOR_NEXT request");

            return;

        }
- Line 2386: severity=HIGH; category=no_retry_logic
  Description: RPC/network call without retry logic — transient failures will propagate
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: sendError(400, "Invalid 'batch_size' type in CURSOR_NEXT request");

            return;

        }

        std::string cursor_id = request.value("cursor_id", "");



        if (cursor_id.empty() || isBlankString(cursor_id)) {

            sendError(400, "Missing 'cursor_id' in CURSOR_NEXT request");
- Line 2397: severity=HIGH; category=no_retry_logic
  Description: RPC/network call without retry logic — transient failures will propagate
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: return;

        }



        int batch_size_i = request.value("batch_size", 100);

        constexpr int kMaxCursorBatchSize = 10000;

        if (batch_size_i < 1 || batch_size_i > kMaxCursorBatchSize) {

            sendError(400, "'batch_size' must be between 1 and 10000 in CURSOR_NEXT request");
- Line 2468: severity=HIGH; category=no_retry_logic
  Description: RPC/network call without retry logic — transient failures will propagate
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: }



        json request = parsePayloadJson(payload_buffer_);

        if (!request.is_object()) {

            sendError(400, "Invalid CURSOR_CLOSE payload: expected JSON object");

            return;

        }
- Line 2472: severity=HIGH; category=no_retry_logic
  Description: RPC/network call without retry logic — transient failures will propagate
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: sendError(400, "Invalid CURSOR_CLOSE payload: expected JSON object");

            return;

        }

        if (request.contains("cursor_id") && !request["cursor_id"].is_string()) {

            sendError(400, "Invalid 'cursor_id' type in CURSOR_CLOSE request");

            return;

        }
- Line 2476: severity=HIGH; category=no_retry_logic
  Description: RPC/network call without retry logic — transient failures will propagate
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: sendError(400, "Invalid 'cursor_id' type in CURSOR_CLOSE request");

            return;

        }

        std::string cursor_id = request.value("cursor_id", "");



        if (cursor_id.empty() || isBlankString(cursor_id)) {

            sendError(400, "Missing 'cursor_id' in CURSOR_CLOSE request");
- Line 2522: severity=HIGH; category=no_retry_logic
  Description: RPC/network call without retry logic — transient failures will propagate
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: try {

        json request = parsePayloadJson(payload_buffer_);



        if (request.contains("k") && !request["k"].is_number_integer()) {

            sendError(400, "Invalid 'k' type in VECTOR_SEARCH request");

            return;

        }
- Line 2527: severity=HIGH; category=no_retry_logic
  Description: RPC/network call without retry logic — transient failures will propagate
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: return;

        }



        if (request.contains("collection") && !request["collection"].is_string()) {

            sendError(400, "Invalid 'collection' type in VECTOR_SEARCH request");

            return;

        }
- Line 2532: severity=HIGH; category=no_retry_logic
  Description: RPC/network call without retry logic — transient failures will propagate
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: return;

        }



        const std::string collection = request.value("collection", "");

        if (!collection.empty() && !isReasonableWireIdentifier(collection)) {

            sendError(400, "Invalid 'collection' field in VECTOR_SEARCH request");

            return;
- Line 2538: severity=HIGH; category=no_retry_logic
  Description: RPC/network call without retry logic — transient failures will propagate
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: return;

        }



        if (!request.contains("vector") || !request["vector"].is_array()) {

            sendError(400, "Missing or invalid 'vector' field in VECTOR_SEARCH request");

            return;

        }
- Line 2570: severity=HIGH; category=no_retry_logic
  Description: RPC/network call without retry logic — transient failures will propagate
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: return;

        }



        int64_t k_i = request.value("k", static_cast<int64_t>(10));

        if (k_i <= 0) {

            k_i = 10;

        }
- Line 2627: severity=HIGH; category=no_retry_logic
  Description: RPC/network call without retry logic — transient failures will propagate
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: try {

        json request = parsePayloadJson(payload_buffer_);



        if (request.contains("collection") && !request["collection"].is_string()) {

            sendError(400, "Invalid 'collection' type in GEO_QUERY request");

            return;

        }
- Line 2631: severity=HIGH; category=no_retry_logic
  Description: RPC/network call without retry logic — transient failures will propagate
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: sendError(400, "Invalid 'collection' type in GEO_QUERY request");

            return;

        }

        if (request.contains("type") && !request["type"].is_string()) {

            sendError(400, "Invalid 'type' field in GEO_QUERY request");

            return;

        }
- Line 2635: severity=HIGH; category=no_retry_logic
  Description: RPC/network call without retry logic — transient failures will propagate
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: sendError(400, "Invalid 'type' field in GEO_QUERY request");

            return;

        }

        if (request.contains("limit") && !request["limit"].is_number_integer()) {

            sendError(400, "Invalid 'limit' type in GEO_QUERY request");

            return;

        }
- Line 2640: severity=HIGH; category=no_retry_logic
  Description: RPC/network call without retry logic — transient failures will propagate
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: return;

        }



        std::string collection = request.value("collection", "");

        if (collection.empty()) {

            sendError(400, "Missing 'collection' field in GEO_QUERY request");

            return;
- Line 2650: severity=HIGH; category=no_retry_logic
  Description: RPC/network call without retry logic — transient failures will propagate
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: return;

        }



        std::string query_type = request.value("type", "");

        if (query_type.empty()) {

            sendError(400,

                "Missing 'type' field in GEO_QUERY request "
- Line 2667: severity=HIGH; category=no_retry_logic
  Description: RPC/network call without retry logic — transient failures will propagate
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: }



        if (!spatial_idx && query_type == "near" && geo_bridge) {

            if (!request.contains("center") || !request["center"].is_object()) {

                sendError(400,

                    "Missing or invalid 'center' in GEO_QUERY near-request "

                    "(expected: {lon, lat})");
- Line 2673: severity=HIGH; category=no_retry_logic
  Description: RPC/network call without retry logic — transient failures will propagate
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: "(expected: {lon, lat})");

                return;

            }

            if (!request.contains("radius")) {

                sendError(400, "Missing 'radius' (meters) in GEO_QUERY near-request");

                return;

            }
- Line 2706: severity=HIGH; category=no_retry_logic
  Description: RPC/network call without retry logic — transient failures will propagate
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: }



            constexpr uint32_t kMaxGeoQueryLimit = 10000;

            int64_t limit_i = request.value("limit", static_cast<int64_t>(100));

            if (limit_i < 1 || limit_i > static_cast<int64_t>(kMaxGeoQueryLimit)) {

                sendError(400, "'limit' must be between 1 and 10000 in GEO_QUERY request");

                return;
- Line 2757: severity=HIGH; category=no_retry_logic
  Description: RPC/network call without retry logic — transient failures will propagate
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: }



        constexpr uint32_t kMaxGeoQueryLimit = 10000;

        int64_t limit_i = request.value("limit", static_cast<int64_t>(100));

        if (limit_i < 1 || limit_i > static_cast<int64_t>(kMaxGeoQueryLimit)) {

            sendError(400, "'limit' must be between 1 and 10000 in GEO_QUERY request");

            return;
- Line 2767: severity=HIGH; category=no_retry_logic
  Description: RPC/network call without retry logic — transient failures will propagate
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: std::vector<index::SpatialResult> search_results;



        if (query_type == "intersects" || query_type == "within") {

            if (!request.contains("bbox") || !request["bbox"].is_object()) {

                sendError(400,

                    "Missing or invalid 'bbox' in GEO_QUERY request "

                    "(expected: {minx, miny, maxx, maxy})");
- Line 2801: severity=HIGH; category=no_retry_logic
  Description: RPC/network call without retry logic — transient failures will propagate
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: search_results = spatial_idx->searchIntersects(collection, query_bbox);



        } else if (query_type == "near") {

            if (!request.contains("center") || !request["center"].is_object()) {

                sendError(400,

                    "Missing or invalid 'center' in GEO_QUERY near-request "

                    "(expected: {lon, lat})");
- Line 2807: severity=HIGH; category=no_retry_logic
  Description: RPC/network call without retry logic — transient failures will propagate
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: "(expected: {lon, lat})");

                return;

            }

            if (!request.contains("radius")) {

                sendError(400, "Missing 'radius' (meters) in GEO_QUERY near-request");

                return;

            }
- Line 2913: severity=HIGH; category=no_retry_logic
  Description: RPC/network call without retry logic — transient failures will propagate
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: }

        

        // Validate request

        if (request.collection.empty()) {

            sendError(0x000A, "Collection (metric) name is required");

            return;

        }
- Line 3032: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: // Determine which bucket this point belongs to

                    int64_t bucket_index = (point.timestamp_ms - start_ms) / bucket_size_ms;

                    int64_t bucket_start_ms = start_ms + (bucket_index * bucket_size_ms);

                    bucket_data[bucket_start_ms].push_back(point.value);

                }

                

                // Create response buckets with aggregated values
- Line 3239: severity=HIGH; category=no_retry_logic
  Description: RPC/network call without retry logic — transient failures will propagate
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: // Expected format: { "process_definition_key": "...", "variables": {...}, "business_key": "..." }

        json request = parsePayloadJson(payload_buffer_);



        if (!request.is_object()) {

            sendError(400, "Invalid request: expected JSON object");

            return;

        }
- Line 3244: severity=HIGH; category=no_retry_logic
  Description: RPC/network call without retry logic — transient failures will propagate
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: return;

        }



        if (request.contains("process_definition_key") && !request["process_definition_key"].is_string()) {

            sendError(400, "Invalid process_definition_key: expected string");

            return;

        }
- Line 3248: severity=HIGH; category=no_retry_logic
  Description: RPC/network call without retry logic — transient failures will propagate
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: sendError(400, "Invalid process_definition_key: expected string");

            return;

        }

        if (request.contains("variables") && !request["variables"].is_object()) {

            sendError(400, "Invalid variables: expected object");

            return;

        }
- Line 3252: severity=HIGH; category=no_retry_logic
  Description: RPC/network call without retry logic — transient failures will propagate
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: sendError(400, "Invalid variables: expected object");

            return;

        }

        if (request.contains("business_key") && !request["business_key"].is_string()) {

            sendError(400, "Invalid business_key: expected string");

            return;

        }
- Line 3257: severity=HIGH; category=no_retry_logic
  Description: RPC/network call without retry logic — transient failures will propagate
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: return;

        }



        std::string process_key = request.value("process_definition_key", "");

        json variables = request.value("variables", json::object());

        std::string business_key = request.value("business_key", "");
- Line 3258: severity=HIGH; category=no_retry_logic
  Description: RPC/network call without retry logic — transient failures will propagate
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: }



        std::string process_key = request.value("process_definition_key", "");

        json variables = request.value("variables", json::object());

        std::string business_key = request.value("business_key", "");



        std::string variables_error;
- Line 3259: severity=HIGH; category=no_retry_logic
  Description: RPC/network call without retry logic — transient failures will propagate
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: std::string process_key = request.value("process_definition_key", "");

        json variables = request.value("variables", json::object());

        std::string business_key = request.value("business_key", "");



        std::string variables_error;

        if (!validateBpmnVariablesObject(variables, variables_error)) {
- Line 3374: severity=HIGH; category=no_retry_logic
  Description: RPC/network call without retry logic — transient failures will propagate
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: // Expected format: { "task_id": "...", "variables": {...}, "assignee": "..." }

        json request = parsePayloadJson(payload_buffer_);



        if (!request.is_object()) {

            sendError(400, "Invalid request: expected JSON object");

            return;

        }
- Line 3379: severity=HIGH; category=no_retry_logic
  Description: RPC/network call without retry logic — transient failures will propagate
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: return;

        }



        if (request.contains("task_id") && !request["task_id"].is_string()) {

            sendError(400, "Invalid task_id: expected string");

            return;

        }
- Line 3383: severity=HIGH; category=no_retry_logic
  Description: RPC/network call without retry logic — transient failures will propagate
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: sendError(400, "Invalid task_id: expected string");

            return;

        }

        if (request.contains("variables") && !request["variables"].is_object()) {

            sendError(400, "Invalid variables: expected object");

            return;

        }
- Line 3387: severity=HIGH; category=no_retry_logic
  Description: RPC/network call without retry logic — transient failures will propagate
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: sendError(400, "Invalid variables: expected object");

            return;

        }

        if (request.contains("assignee") && !request["assignee"].is_string()) {

            sendError(400, "Invalid assignee: expected string");

            return;

        }
- Line 3392: severity=HIGH; category=no_retry_logic
  Description: RPC/network call without retry logic — transient failures will propagate
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: return;

        }



        std::string task_id = request.value("task_id", "");

        json variables = request.value("variables", json::object());

        std::string assignee = request.value("assignee", username_);
- Line 3393: severity=HIGH; category=no_retry_logic
  Description: RPC/network call without retry logic — transient failures will propagate
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: }



        std::string task_id = request.value("task_id", "");

        json variables = request.value("variables", json::object());

        std::string assignee = request.value("assignee", username_);



        std::string variables_error;
- Line 3394: severity=HIGH; category=no_retry_logic
  Description: RPC/network call without retry logic — transient failures will propagate
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: std::string task_id = request.value("task_id", "");

        json variables = request.value("variables", json::object());

        std::string assignee = request.value("assignee", username_);



        std::string variables_error;

        if (!validateBpmnVariablesObject(variables, variables_error)) {
- Line 3513: severity=HIGH; category=no_retry_logic
  Description: RPC/network call without retry logic — transient failures will propagate
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: // Expected format: { "process_instance_id": "...", "include_variables": true/false, "include_history": true/false }

        json request = parsePayloadJson(payload_buffer_);



        if (!request.is_object()) {

            sendError(400, "Invalid request: expected JSON object");

            return;

        }
- Line 3518: severity=HIGH; category=no_retry_logic
  Description: RPC/network call without retry logic — transient failures will propagate
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: return;

        }



        if (request.contains("process_instance_id") && !request["process_instance_id"].is_string()) {

            sendError(400, "Invalid process_instance_id: expected string");

            return;

        }
- Line 3522: severity=HIGH; category=no_retry_logic
  Description: RPC/network call without retry logic — transient failures will propagate
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: sendError(400, "Invalid process_instance_id: expected string");

            return;

        }

        if (request.contains("include_variables") && !request["include_variables"].is_boolean()) {

            sendError(400, "Invalid include_variables: expected boolean");

            return;

        }
- Line 3526: severity=HIGH; category=no_retry_logic
  Description: RPC/network call without retry logic — transient failures will propagate
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: sendError(400, "Invalid include_variables: expected boolean");

            return;

        }

        if (request.contains("include_history") && !request["include_history"].is_boolean()) {

            sendError(400, "Invalid include_history: expected boolean");

            return;

        }
- Line 3530: severity=HIGH; category=no_retry_logic
  Description: RPC/network call without retry logic — transient failures will propagate
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: sendError(400, "Invalid include_history: expected boolean");

            return;

        }

        if (request.contains("max_history_events") && !request["max_history_events"].is_number_unsigned()) {

            sendError(400, "Invalid max_history_events: expected unsigned integer");

            return;

        }
- Line 3535: severity=HIGH; category=no_retry_logic
  Description: RPC/network call without retry logic — transient failures will propagate
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: return;

        }



        std::string instance_id = request.value("process_instance_id", "");

        bool include_variables = request.value("include_variables", true);

        bool include_history = request.value("include_history", false);

        std::size_t max_history_events = request.value("max_history_events", kDefaultBpmnHistoryEvents);
- Line 3536: severity=HIGH; category=no_retry_logic
  Description: RPC/network call without retry logic — transient failures will propagate
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: }



        std::string instance_id = request.value("process_instance_id", "");

        bool include_variables = request.value("include_variables", true);

        bool include_history = request.value("include_history", false);

        std::size_t max_history_events = request.value("max_history_events", kDefaultBpmnHistoryEvents);
- Line 3537: severity=HIGH; category=no_retry_logic
  Description: RPC/network call without retry logic — transient failures will propagate
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: std::string instance_id = request.value("process_instance_id", "");

        bool include_variables = request.value("include_variables", true);

        bool include_history = request.value("include_history", false);

        std::size_t max_history_events = request.value("max_history_events", kDefaultBpmnHistoryEvents);



        if (instance_id.empty() || isBlankString(instance_id)) {
- Line 3538: severity=HIGH; category=no_retry_logic
  Description: RPC/network call without retry logic — transient failures will propagate
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: std::string instance_id = request.value("process_instance_id", "");

        bool include_variables = request.value("include_variables", true);

        bool include_history = request.value("include_history", false);

        std::size_t max_history_events = request.value("max_history_events", kDefaultBpmnHistoryEvents);



        if (instance_id.empty() || isBlankString(instance_id)) {

            sendError(400, "Missing process_instance_id");
- Line 85: severity=MEDIUM; category=duplicate_qualified_signature
  Description: Potential duplicate implementation signature across files: std::all_of(value.begin(), value.end(), [](unsigned char ch)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::legacy_duplication
  Context: return std::all_of(value.begin(), value.end(), [](unsigned char ch) {
- Line 239: severity=MEDIUM; category=missing_health_check
  Description: Service initialization without nearby health/status handling
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: void WireProtocolServer::start() {
- Line 452: severity=MEDIUM; category=expensive_inner_op
  Description: I/O operation in inner loop — very expensive
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: std::cerr << "[WireProtocol] IO thread error: " << e.what() << std::endl;
- Line 464: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: acceptor_->close();
- Line 709: severity=MEDIUM; category=missing_health_check
  Description: Service initialization without nearby health/status handling
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: , client_ip_("unknown")  // Will be set after accept in start()
- Line 717: severity=MEDIUM; category=missing_health_check
  Description: Service initialization without nearby health/status handling
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: void WireProtocolServer::Session::start() {
- Line 721: severity=MEDIUM; category=generic_catch
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: try {

        // Now that socket is accepted, we can get the remote endpoint

        client_ip_ = socket_.remote_endpoint().address().to_string();

    } catch (...) {

        client_ip_ = "unknown";

    }
- Line 721: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: } catch (...) {
- Line 740: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: void WireProtocolServer::Session::close() {
- Line 745: severity=MEDIUM; category=generic_catch
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: if (socket_.is_open()) {

            socket_.close();

        }

    } catch (...) {

    }



    // Deregister from per-tenant QoS manager
- Line 745: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: } catch (...) {
- Line 762: severity=MEDIUM; category=generic_catch
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: std::string WireProtocolServer::Session::getRemoteIP() const {

    try {

        return socket_.remote_endpoint().address().to_string();

    } catch (...) {

        return "unknown";

    }

}
- Line 762: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: } catch (...) {
- Line 790: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: socket_.close();
- Line 884: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: socket_.close();
- Line 1218: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: close();
- Line 1225: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: close();
- Line 1923: severity=MEDIUM; category=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: const auto timeout_ms = request["timeout_ms"].get<int64_t>();
- Line 2231: severity=MEDIUM; category=missing_latency_metric
  Description: No latency measurement for operation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: void WireProtocolServer::Session::handleQuery() {
- Line 2612: severity=MEDIUM; category=missing_latency_metric
  Description: No latency measurement for operation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: void WireProtocolServer::Session::handleGeoQuery() {
- Line 2893: severity=MEDIUM; category=missing_latency_metric
  Description: No latency measurement for operation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: void WireProtocolServer::Session::handleTimeseriesQuery() {
- Line 3183: severity=MEDIUM; category=endianness_assumption
  Description: Endianness conversion may have undefined behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: uint32_t magic = htonl(0x544D4442);  // "TMDB" in network byte order
- Line 3192: severity=MEDIUM; category=endianness_assumption
  Description: Endianness conversion may have undefined behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: uint32_t payload_size_net = htonl(payload_size);  // Convert to network byte order
- Line 3218: severity=MEDIUM; category=missing_latency_metric
  Description: No latency measurement for operation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: void WireProtocolServer::Session::handleBpmnStartProcess() {

### network/envoy_xds.cpp
Total findings: 47

- Line 243: severity=CRITICAL; category=thread_join_no_timeout
  Description: Thread join/wait without timeout (blocking indefinitely)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: poll_thread_.join();
- Line 55: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: std::snprintf(buf, sizeof(buf), "\\u%04x", c);
- Line 380: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: for (const auto& item : splitJsonArray(body)) {
- Line 415: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: for (const auto& item : splitJsonArray(body)) {
- Line 428: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: for (const auto& locality_ep : splitJsonArray(eps_body)) {
- Line 432: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: for (const auto& lbep : splitJsonArray(lb_body)) {
- Line 472: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: for (const auto& item : splitJsonArray(body)) {
- Line 479: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: for (const auto& locality_ep : splitJsonArray(eps_body)) {
- Line 483: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: for (const auto& lbep : splitJsonArray(lb_body)) {
- Line 522: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: for (const auto& rc : splitJsonArray(body)) {
- Line 528: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: for (const auto& vh : splitJsonArray(vhosts_body)) {
- Line 538: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: const auto q1 = dom_body.find('"', pos);
- Line 539: severity=HIGH; category=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: const auto q1 = dom_body.find('"', pos);
- Line 559: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: for (const auto& rt : splitJsonArray(routes_body)) {
- Line 623: severity=HIGH; category=no_retry_logic
  Description: RPC/network call without retry logic — transient failures will propagate
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: beast::tcp_stream stream(ioc);

        stream.expires_after(std::chrono::milliseconds(config_.request_timeout_ms));

        stream.connect(results);



        http::request<http::string_body> req{http::verb::post, path, 11};

        req.set(http::field::host, config_.control_plane_host);
- Line 687: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 694: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 695: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 699: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 700: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 703: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 715: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: while (running_.load(std::memory_order_acquire)) {
- Line 828: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: wake_cv_.wait_for(lk, std::chrono::milliseconds(wait_ms),
- Line 829: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: [this] { return !running_.load(std::memory_order_acquire); });
- Line 46: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: case '"':  out += "\\\""; break;
- Line 47: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: case '"':  out += "\\\""; break;
- Line 48: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: case '\\': out += "\\\\"; break;
- Line 49: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: case '\n': out += "\\n";  break;
- Line 50: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: case '\r': out += "\\r";  break;
- Line 51: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: case '\t': out += "\\t";  break;
- Line 55: severity=MEDIUM; category=expensive_inner_op
  Description: I/O operation in inner loop — very expensive
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: std::snprintf(buf, sizeof(buf), "\\u%04x", c);
- Line 79: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: case '"': value += '"'; break;
- Line 80: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: case '"': value += '"'; break;
- Line 81: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: case '\\': value += '\\'; break;
- Line 82: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: case 'n': value += '\n'; break;
- Line 83: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: case 'r': value += '\r'; break;
- Line 84: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: case 't': value += '\t'; break;
- Line 168: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: items.push_back(array_body.substr(item_start, i - item_start + 1));
- Line 182: severity=MEDIUM; category=generic_catch
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: try {

        const long v = std::stol(s);

        if (v > 0 && v <= 65535) return static_cast<uint16_t>(v);

    } catch (...) {}

    return 0;

}
- Line 182: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: } catch (...) {}
- Line 192: severity=MEDIUM; category=generic_catch
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: try {

        const unsigned long v = std::stoul(s);

        if (v <= UINT32_MAX) return static_cast<uint32_t>(v);

    } catch (...) {}

    return default_val;

}
- Line 192: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: } catch (...) {}
- Line 587: severity=MEDIUM; category=generic_catch
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: }

                                route.timeout_ms = static_cast<uint32_t>(

                                    std::stof(numeric) * 1000.0f);

                            } catch (...) {}

                        }

                    }
- Line 587: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: } catch (...) {}
- Line 716: severity=MEDIUM; category=primitive_no_volatile
  Description: Primitive shared across threads without volatile
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: bool had_any_update = false;
- Line 721: severity=MEDIUM; category=primitive_no_volatile
  Description: Primitive shared across threads without volatile
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: bool err = false;
- Line 55: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: std::snprintf(buf, sizeof(buf), "\\u%04x", c);

### network/wire_protocol_connection_pool.cpp
Total findings: 34

- Line 161: severity=CRITICAL; category=thread_join_no_timeout
  Description: Thread join/wait without timeout (blocking indefinitely)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: maintenance_thread_.join();
- Line 410: severity=CRITICAL; category=blocking_no_timeout
  Description: Blocking operation without timeout — can block indefinitely
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: conn->created_at = std::chrono::steady_clock::now();

                conn->in_use = true;

                

                lock.lock();

                void* socket_key = socket.get();

                pool->all_connections[socket_key] = conn;

                pool->active_count++;
- Line 410: severity=CRITICAL; category=double_lock
  Description: Double lock without unlock (potential deadlock)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: lock.lock();
- Line 410: severity=CRITICAL; category=no_timeout
  Description: mutex_lock without timeout — can block indefinitely
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: lock.lock();
- Line 134: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: while (!shutdown_.load(std::memory_order_acquire)) {
- Line 136: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: std::unique_lock<std::mutex> lock(shutdown_mutex_);
- Line 137: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: if (shutdown_cv_.wait_for(lock, std::chrono::seconds(10),
- Line 260: severity=HIGH; category=no_retry_logic
  Description: RPC/network call without retry logic — transient failures will propagate
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: });

        

        // Attempt async connect

        net::async_connect(*plain_socket, endpoints,

            [&ec](const boost::system::error_code& error, const tcp::endpoint&) {

                ec = error;

            });
- Line 327: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: failed_connections_.fetch_add(1, std::memory_order_relaxed);

                if (timed_out) {

                    auto timeout_seconds = std::chrono::duration_cast<std::chrono::seconds>(config_.connect_timeout).count();

                    throw std::runtime_error("SSL handshake timed out after " + 

                        std::to_string(timeout_seconds) + " seconds");

                }

                throw std::runtime_error("SSL handshake failed: " + ec.message());
- Line 330: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: throw std::runtime_error("SSL handshake timed out after " + 

                        std::to_string(timeout_seconds) + " seconds");

                }

                throw std::runtime_error("SSL handshake failed: " + ec.message());

            }

            

            wrapper = std::make_shared<SocketWrapper>(ssl_stream);
- Line 364: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: WireProtocolConnectionPool::acquireConnection(const std::string& target) {
- Line 368: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: auto deadline = std::chrono::steady_clock::now() + config_.acquire_timeout;
- Line 398: severity=HIGH; category=allocation_loop
  Description: Dynamic allocation in loop — high overhead
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: // Try to create new connection if under limit
- Line 410: severity=HIGH; category=explicit_lock_unlock
  Description: Explicit lock/unlock without RAII guard
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: lock.lock();
- Line 420: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: std::this_thread::sleep_for(std::chrono::milliseconds(100));
- Line 496: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: std::lock_guard<std::mutex> lock(pool->mutex);
- Line 511: severity=HIGH; category=deadlock_risk
  Description: Multiple locks acquired in nested scope — potential deadlock
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: std::lock_guard<std::mutex> pools_lock(pools_mutex_);
- Line 513: severity=HIGH; category=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: for (auto& [target, pool] : target_pools_) {
- Line 514: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: std::lock_guard<std::mutex> lock(pool->mutex);
- Line 645: severity=HIGH; category=deadlock_risk
  Description: Multiple locks acquired in nested scope — potential deadlock
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: std::lock_guard<std::mutex> lock(pools_mutex_);
- Line 647: severity=HIGH; category=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: for (auto& [target, pool] : target_pools_) {
- Line 648: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: std::lock_guard<std::mutex> pool_lock(pool->mutex);
- Line 676: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: stats.acquire_timeouts = acquire_timeouts_.load(std::memory_order_relaxed);
- Line 680: severity=HIGH; category=deadlock_risk
  Description: Multiple locks acquired in nested scope — potential deadlock
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: std::lock_guard<std::mutex> lock(pools_mutex_);
- Line 685: severity=HIGH; category=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: for (const auto& [target, pool] : target_pools_) {
- Line 686: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: std::lock_guard<std::mutex> pool_lock(pool->mutex);
- Line 251: severity=MEDIUM; category=primitive_no_volatile
  Description: Primitive shared across threads without volatile
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: bool timed_out = false;
- Line 389: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: conn->socket->close(ec);
- Line 461: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: conn->socket->close(ec);
- Line 504: severity=MEDIUM; category=expensive_inner_op
  Description: I/O operation in inner loop — very expensive
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: std::cerr << "[WireProtocolConnectionPool] Warmup failed for " << target
- Line 529: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: conn->socket->close(ec);
- Line 654: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: conn->socket->close(ec);
- Line 746: severity=MEDIUM; category=missing_latency_metric
  Description: No latency measurement for operation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: WireProtocolConnectionPool::ConnectionHandle::~ConnectionHandle() {
- Line 752: severity=MEDIUM; category=missing_latency_metric
  Description: No latency measurement for operation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: WireProtocolConnectionPool::ConnectionHandle::ConnectionHandle(ConnectionHandle&& other) noexcept

### network/quic_server.cpp
Total findings: 33

- Line 93: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: if (RAND_bytes(cid->data, static_cast<int>(cid->datalen)) != 1) {
- Line 315: severity=CRITICAL; category=thread_join_no_timeout
  Description: Thread join/wait without timeout (blocking indefinitely)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: t.join();
- Line 564: severity=CRITICAL; category=smart_ptr_misuse
  Description: Raw new without immediate wrapping in smart pointer
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: THEMIS_INFO("[QUICServer] new QUIC connection from {}", key);
- Line 134: severity=HIGH; category=manual_cleanup_in_destructor
  Description: Manual resource cleanup in destructor (should use RAII wrapper)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_raii
  Context: QUICServer::~QUICServer() {
- Line 174: severity=HIGH; category=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::legacy_duplication
  Context: // Advertise HTTP/3 ALPN ("h3") for compatibility with standard HTTP/3
- Line 237: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: if (running_.load(std::memory_order_acquire)) {
- Line 283: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: threads_.emplace_back([this] { io_ctx_->run(); });
- Line 311: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: io_ctx_->stop();
- Line 319: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: io_ctx_->restart();
- Line 356: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: if (running_.load(std::memory_order_acquire)) {
- Line 372: severity=HIGH; category=deadlock_risk
  Description: Multiple locks acquired in nested scope — potential deadlock
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: std::lock_guard<std::mutex> lk(sessions_mutex_);
- Line 525: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 538: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 551: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 601: severity=HIGH; category=manual_cleanup_in_destructor
  Description: Manual resource cleanup in destructor (should use RAII wrapper)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_raii
  Context: QUICClient::Stream::~Stream() {
- Line 602: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: if (open_.load(std::memory_order_acquire)) {
- Line 608: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: if (!open_.load(std::memory_order_acquire)) {
- Line 650: severity=HIGH; category=manual_cleanup_in_destructor
  Description: Manual resource cleanup in destructor (should use RAII wrapper)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_raii
  Context: QUICClient::~QUICClient() {
- Line 795: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: kQuicServerVersion1, &callbacks,

                                    &settings, &params, nullptr, this);

    if (rv != 0) {

        throw std::runtime_error(

            std::string("[QUICClient] ngtcp2_conn_client_new: ") +

            ngtcp2_strerror(rv));

    }
- Line 796: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 844: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: if (!connected_.load(std::memory_order_acquire)) {
- Line 845: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: std::unique_ptr<QUICClient::Stream> QUICClient::openStream() {

    if (!connected_.load(std::memory_order_acquire)) {

        throw std::runtime_error("[QUICClient] openStream called when not connected");

    }



    // Client-initiated bidirectional stream IDs: 0, 4, 8, …  (RFC 9000 §2.1)
- Line 177: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: "\x02h3"          // "h3"   (length 2)
- Line 178: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: "\x04tmdb";       // "tmdb" (length 4)
- Line 236: severity=MEDIUM; category=missing_health_check
  Description: Service initialization without nearby health/status handling
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: void QUICServer::start() {
- Line 299: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: SSL_free(static_cast<SSL*>(tls));
- Line 309: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: socket_->close(ec);
- Line 322: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: SSL_CTX_free(ssl_ctx_);
- Line 389: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: SSL_free(static_cast<SSL*>(tls));
- Line 540: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: SSL_free(ssl);
- Line 554: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: SSL_free(ssl);
- Line 829: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: SSL_free(static_cast<SSL*>(tls));
- Line 836: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: SSL_CTX_free(ssl_ctx_);

### network/kernel_bypass.cpp
Total findings: 29

- Line 144: severity=CRITICAL; category=uninitialized_pointer
  Description: Undefined behavior: potential null-pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::uninitialized
  Context: Pointer declared but not initialized
- Line 358: severity=CRITICAL; category=exception_in_destructor
  Description: Destructors must be noexcept; exceptions here cause std::terminate()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 602: severity=CRITICAL; category=thread_join_no_timeout
  Description: Thread join/wait without timeout (blocking indefinitely)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: if (t.joinable()) t.join();
- Line 704: severity=CRITICAL; category=missing_dtor
  Description: Class io_uring_params allocates resources but has no destructor
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: class/struct io_uring_params
- Line 966: severity=CRITICAL; category=thread_join_no_timeout
  Description: Thread join/wait without timeout (blocking indefinitely)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: if (t.joinable()) t.join();
- Line 29: severity=HIGH; category=posix_only_api
  Description: POSIX-only API pthread_ without platform guard
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: * - CpuPinner uses sched_setaffinity / pthread_setaffinity_np (Linux only).
- Line 125: severity=HIGH; category=posix_only_api
  Description: POSIX-only API pthread_ without platform guard
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: return ::pthread_setaffinity_np(thread.native_handle(),
- Line 192: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: void* NumaAllocator::allocate(size_t size, int node) {
- Line 192: severity=HIGH; category=missing_trace_point
  Description: Critical function allocate without trace point
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: void* NumaAllocator::allocate(size_t size, int node) {
- Line 221: severity=HIGH; category=missing_trace_point
  Description: Critical function deallocate without trace point
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: void NumaAllocator::deallocate(void* ptr, size_t size) noexcept {
- Line 273: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: &nodemask, sizeof(nodemask) * 8 + 1, /* MPOL_MF_MOVE */ 2);
- Line 342: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: NumaAllocator::deallocate(data_, size_);
- Line 631: severity=HIGH; category=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: for (uint16_t i = 0; i < nb_rx; ++i) {
- Line 125: severity=MEDIUM; category=missing_latency_metric
  Description: No latency measurement for operation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: return ::pthread_setaffinity_np(thread.native_handle(),
- Line 312: severity=MEDIUM; category=stale_doc_section_reference
  Description: Code comment references section 'Kernel Bypass Windows Support' that was not found in 'src/network/FUTURE_ENHANCEMENTS.md'
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::themis_doc_freshness_rules
  Context: src/network/FUTURE_ENHANCEMENTS.md §"Kernel Bypass Windows Support"
- Line 417: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: eal_arg_strs.push_back("themisdb");
- Line 421: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: eal_arg_strs.push_back("-m");
- Line 422: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: eal_arg_strs.push_back(std::to_string(config_.huge_pages_mb));
- Line 429: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: eal_arg_strs.push_back("-c");
- Line 434: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: eal_arg_strs.push_back("-a");
- Line 435: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: eal_arg_strs.push_back(config_.pci_address);
- Line 568: severity=MEDIUM; category=primitive_no_volatile
  Description: Primitive shared across threads without volatile
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: int queue_id = 0;
- Line 644: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: rte_pktmbuf_free(m);
- Line 688: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: ::close(fd);
- Line 708: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: ::close(fd);
- Line 939: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: ::close(listen_fd_);
- Line 943: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: ::close(ring_fd_);
- Line 140: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: std::snprintf(path, sizeof(path),
- Line 427: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: std::snprintf(mask_str, sizeof(mask_str), "0x%llX",

### network/raft_load_balancer.cpp
Total findings: 29

- Line 108: severity=CRITICAL; category=thread_join_no_timeout
  Description: Thread join/wait without timeout (blocking indefinitely)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: if (health_check_thread_.joinable()) health_check_thread_.join();
- Line 109: severity=CRITICAL; category=thread_join_no_timeout
  Description: Thread join/wait without timeout (blocking indefinitely)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: if (raft_thread_.joinable())         raft_thread_.join();
- Line 249: severity=CRITICAL; category=db_connection_leak
  Description: Database connection from total_requests never released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: s.total_requests   = total_requests_.load(std::memory_order_acquire);
- Line 250: severity=CRITICAL; category=db_connection_leak
  Description: Database connection from failed_requests never released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: s.failed_requests  = total_failed_.load(std::memory_order_acquire);
- Line 251: severity=CRITICAL; category=db_connection_leak
  Description: Database connection from rebalance_events never released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: s.rebalance_events = rebalance_events_.load(std::memory_order_acquire);
- Line 252: severity=CRITICAL; category=db_connection_leak
  Description: Database connection from failover_events never released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: s.failover_events  = failover_events_.load(std::memory_order_acquire);
- Line 253: severity=CRITICAL; category=db_connection_leak
  Description: Database connection from recovery_events never released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: s.recovery_events  = recovery_events_.load(std::memory_order_acquire);
- Line 97: severity=HIGH; category=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: health_check_thread_ = std::thread([this]() { healthCheckLoop(); });
- Line 98: severity=HIGH; category=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: raft_thread_         = std::thread([this]() { raftLoop(); });
- Line 158: severity=HIGH; category=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: result.push_back(b.get());
- Line 172: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: strategy_.load(std::memory_order_acquire);
- Line 232: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: return role_.load(std::memory_order_acquire) == RaftRole::LEADER;
- Line 236: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: return role_.load(std::memory_order_acquire);
- Line 240: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: return current_term_.load(std::memory_order_acquire);
- Line 279: severity=HIGH; category=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: if (b->address == address) return b.get();
- Line 290: severity=HIGH; category=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: result.push_back(b.get());
- Line 297: severity=HIGH; category=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: if (b->healthy) result.push_back(b.get());
- Line 407: severity=HIGH; category=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: for (const auto& b : backends_) backends_snapshot.push_back(b.get());
- Line 410: severity=HIGH; category=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: for (Backend* b : backends_snapshot) {
- Line 446: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: while (!shutdown_.load(std::memory_order_acquire)) {
- Line 447: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: std::unique_lock<std::mutex> lk(shutdown_mutex_);
- Line 449: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: [this] { return shutdown_.load(std::memory_order_acquire); });
- Line 452: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: if (shutdown_.load(std::memory_order_acquire)) break;
- Line 530: severity=HIGH; category=deadlock_risk
  Description: Multiple locks acquired in nested scope — potential deadlock
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: std::unique_lock<std::mutex> lk(shutdown_mutex_);
- Line 538: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 566: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: [this] { return shutdown_.load(std::memory_order_acquire); });
- Line 268: severity=MEDIUM; category=missing_move_constructor_defaulted
  Description: Missing move ctor prevents efficient rvalue transfers
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 396: severity=MEDIUM; category=missing_move_constructor_defaulted
  Description: Missing move ctor prevents efficient rvalue transfers
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 407: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: for (const auto& b : backends_) backends_snapshot.push_back(b.get());

### network/wire_protocol_zero_copy.cpp
Total findings: 24

- Line 101: severity=CRITICAL; category=no_timeout
  Description: file_io without timeout — can block indefinitely
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: return ::write(fd, header_.data(), HEADER_SIZE);
- Line 149: severity=CRITICAL; category=no_timeout
  Description: file_io without timeout — can block indefinitely
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: fd_ = ::open(path.c_str(), O_RDONLY | O_CLOEXEC);
- Line 42: severity=HIGH; category=shared_state_no_sync
  Description: Shared state accessed without synchronization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: #elif __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
- Line 52: severity=HIGH; category=shared_state_no_sync
  Description: Shared state accessed without synchronization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: #elif __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
- Line 88: severity=HIGH; category=uninitialized_array
  Description: Array contains garbage values; use with zeros or explicit init
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::uninitialized
  Context: Array declared without initialization
- Line 105: severity=HIGH; category=uninitialized_array
  Description: Array contains garbage values; use with zeros or explicit init
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::uninitialized
  Context: Array declared without initialization
- Line 123: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: #ifdef _WIN32

    std::ifstream in(path, std::ios::binary | std::ios::ate);

    if (!in) {

        throw std::runtime_error("MemoryMappedPayload: open failed: " + path);

    }

    const std::streamsize file_size = in.tellg();

    if (file_size <= 0) {
- Line 132: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: size_ = static_cast<size_t>(file_size);

    if (size_ > MAX_MAP_SIZE) {

        throw std::runtime_error("MemoryMappedPayload: file too large (> 256 MiB): " + path);

    }



    void* mem = std::malloc(size_);
- Line 135: severity=HIGH; category=unchecked_malloc
  Description: Unchecked malloc() — missing null pointer check
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: throw std::runtime_error("MemoryMappedPayload: file too large (> 256 MiB): " + path);

    }



    void* mem = std::malloc(size_);

    if (mem == nullptr) {

        throw std::runtime_error("MemoryMappedPayload: allocation failed");

    }
- Line 137: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: void* mem = std::malloc(size_);

    if (mem == nullptr) {

        throw std::runtime_error("MemoryMappedPayload: allocation failed");

    }

    addr_ = mem;
- Line 146: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: std::free(addr_);

        addr_ = MAP_FAILED;

        size_ = 0;

        throw std::runtime_error("MemoryMappedPayload: read failed: " + path);

    }

#else

    fd_ = ::open(path.c_str(), O_RDONLY | O_CLOEXEC);
- Line 149: severity=HIGH; category=path_traversal
  Description: path_traversal_concat_path: Path construction — validate user input
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: fd_ = ::open(path.c_str(), O_RDONLY | O_CLOEXEC);
- Line 151: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: #else

    fd_ = ::open(path.c_str(), O_RDONLY | O_CLOEXEC);

    if (fd_ < 0) {

        throw std::system_error(errno, std::system_category(),

                                "MemoryMappedPayload: open failed: " + path);

    }
- Line 167: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: if (size_ > MAX_MAP_SIZE) {

        ::close(fd_);

        fd_ = -1;

        throw std::runtime_error(

            "MemoryMappedPayload: file too large (> 256 MiB): " + path);

    }
- Line 176: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: const int err = errno;

        ::close(fd_);

        fd_ = -1;

        throw std::system_error(err, std::system_category(),

                                "MemoryMappedPayload: mmap failed: " + path);

    }
- Line 192: severity=HIGH; category=unchecked_malloc
  Description: Unchecked calloc() — missing null pointer check
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: }



#ifdef _WIN32

    addr_ = std::calloc(1, size);

    if (addr_ == nullptr) {

        throw std::runtime_error("MemoryMappedPayload: allocation failed");

    }
- Line 194: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: #ifdef _WIN32

    addr_ = std::calloc(1, size);

    if (addr_ == nullptr) {

        throw std::runtime_error("MemoryMappedPayload: allocation failed");

    }

    size_ = size;

#else
- Line 202: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: addr_ = ::mmap(nullptr, size_, PROT_READ | PROT_WRITE,

                   MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

    if (addr_ == MAP_FAILED) {

        throw std::system_error(errno, std::system_category(),

                                "MemoryMappedPayload: anonymous mmap failed");

    }

    // fd_ stays -1 (anonymous mapping has no backing file).
- Line 209: severity=HIGH; category=manual_cleanup_in_destructor
  Description: Manual resource cleanup in destructor (should use RAII wrapper)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_raii
  Context: MemoryMappedPayload::~MemoryMappedPayload() {
- Line 157: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: ::close(fd_);
- Line 165: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: ::close(fd_);
- Line 174: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: ::close(fd_);
- Line 241: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: if (addr_ != MAP_FAILED && addr_ != nullptr) std::free(addr_);
- Line 244: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: if (fd_ >= 0) ::close(fd_);

### network/quic_transport.cpp
Total findings: 20

- Line 46: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: if (RAND_bytes(cid->data, static_cast<int>(cid->datalen)) != 1) {
- Line 214: severity=CRITICAL; category=thread_join_no_timeout
  Description: Thread join/wait without timeout (blocking indefinitely)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: t.join();
- Line 430: severity=CRITICAL; category=smart_ptr_misuse
  Description: Raw new without immediate wrapping in smart pointer
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: THEMIS_INFO("[QuicTransport] new QUIC connection from {}", key);
- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: * PR History (last 5): #4632 feat(network): QUIC Protoco... (2026-04-13) | #3291 [network] QUIC/HTTP
- Line 67: severity=HIGH; category=manual_cleanup_in_destructor
  Description: Manual resource cleanup in destructor (should use RAII wrapper)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_raii
  Context: QuicTransport::~QuicTransport() {
- Line 180: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: threads_.emplace_back([this] { io_ctx_->run(); });
- Line 210: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: io_ctx_->stop();
- Line 218: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: io_ctx_->restart();
- Line 256: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: if (running_.load(std::memory_order_acquire)) {
- Line 272: severity=HIGH; category=deadlock_risk
  Description: Multiple locks acquired in nested scope — potential deadlock
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: std::lock_guard<std::mutex> lk(sessions_mutex_);
- Line 322: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 375: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 404: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 417: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 198: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: SSL_free(static_cast<SSL*>(tls_handle));
- Line 208: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: socket_->close(ec);
- Line 221: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: SSL_CTX_free(ssl_ctx_);
- Line 289: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: SSL_free(static_cast<SSL*>(tls));
- Line 406: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: SSL_free(ssl);
- Line 420: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: SSL_free(ssl);

### network/qos_manager.cpp
Total findings: 17

- Line 660: severity=CRITICAL; category=command_injection
  Description: command_injection_system: Command injection via system() — validate and escape inputs
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: std::system(cmd);  // NOLINT(cert-env33-c)
- Line 669: severity=CRITICAL; category=command_injection
  Description: command_injection_system: Command injection via system() — validate and escape inputs
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: if (std::system(cmd) != 0) {  // NOLINT(cert-env33-c)
- Line 682: severity=CRITICAL; category=command_injection
  Description: command_injection_system: Command injection via system() — validate and escape inputs
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: if (std::system(cmd) != 0) {  // NOLINT(cert-env33-c)
- Line 96: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: std::this_thread::sleep_for(std::chrono::milliseconds(wait_ms));
- Line 98: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: std::this_thread::sleep_for(std::chrono::milliseconds(1));
- Line 107: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 171: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 238: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 891: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: QoSManager::getConnectionStats(uint64_t connection_id) const {
- Line 942: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: result.push_back(getConnectionStats(id));
- Line 977: severity=HIGH; category=deadlock_risk
  Description: Multiple locks acquired in nested scope — potential deadlock
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: std::lock_guard<std::mutex> lock(tenants_mutex_);
- Line 1033: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 41: severity=MEDIUM; category=duplicate_qualified_signature
  Description: Potential duplicate implementation signature across files: TokenBucket::refill()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::legacy_duplication
  Context: void TokenBucket::refill() {
- Line 625: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: //     prevent shell metacharacter injection via snprintf.
- Line 657: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: std::snprintf(cmd, sizeof(cmd),
- Line 666: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: std::snprintf(cmd, sizeof(cmd),
- Line 678: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: std::snprintf(cmd, sizeof(cmd),

### network/network_audit_log.cpp
Total findings: 13

- Line 85: severity=CRITICAL; category=array_bounds
  Description: Array bounds violation: loop 64 > array 0
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::memory
  Context: uint32_t a=h[0], b=h[1], c=h[2], d=h[3],
- Line 85: severity=CRITICAL; category=array_bounds_violation
  Description: Array bounds violation: loop 64 > array size 0
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: const uint32_t s0 = rotr32(w[i-15], 7) ^ rotr32(w[i-15], 18) ^ (w[i-15] >> 3u);

            const uint32_t s1 = rotr32(w[i- 2], 17) ^ rotr32(w[i- 2], 19) ^ (w[i- 2] >> 10u);

            w[i] = w[i-16] + s0 + w[i-7] + s1;

        }



        uint32_t a=h[0], b=h[1], c=h[2], d=h[3],

                 e=h[4], f=h[5], g=h[6], hh=h[7];



        for (int i = 0; i < 64; ++i) {

            const uint32_t S1  = rotr32(e,6) ^ rotr32(e,11) ^ rotr32(e,25);

            const uint32_t ch  = (e & f) ^ (~e & g);
- Line 86: severity=CRITICAL; category=array_bounds
  Description: Array bounds violation: loop 64 > array 4
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::memory
  Context: e=h[4], f=h[5], g=h[6], hh=h[7];
- Line 86: severity=CRITICAL; category=array_bounds_violation
  Description: Array bounds violation: loop 64 > array size 4
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: const uint32_t s1 = rotr32(w[i- 2], 17) ^ rotr32(w[i- 2], 19) ^ (w[i- 2] >> 10u);

            w[i] = w[i-16] + s0 + w[i-7] + s1;

        }



        uint32_t a=h[0], b=h[1], c=h[2], d=h[3],

                 e=h[4], f=h[5], g=h[6], hh=h[7];



        for (int i = 0; i < 64; ++i) {

            const uint32_t S1  = rotr32(e,6) ^ rotr32(e,11) ^ rotr32(e,25);

            const uint32_t ch  = (e & f) ^ (~e & g);

            const uint32_t tmp1 = hh + S1 + ch + kK[i] + w[i];
- Line 99: severity=CRITICAL; category=array_bounds
  Description: Array bounds violation: loop 8 > array 0
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::memory
  Context: h[0]+=a; h[1]+=b; h[2]+=c; h[3]+=d;
- Line 99: severity=CRITICAL; category=array_bounds_violation
  Description: Array bounds violation: loop 8 > array size 0
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: const uint32_t tmp2 = S0 + maj;

            hh = g; g = f; f = e; e = d + tmp1;

            d  = c; c = b; b = a; a = tmp1 + tmp2;

        }



        h[0]+=a; h[1]+=b; h[2]+=c; h[3]+=d;

        h[4]+=e; h[5]+=f; h[6]+=g; h[7]+=hh;

    }



    std::array<uint8_t, 32> digest{};

    for (int i = 0; i < 8; ++i) {
- Line 100: severity=CRITICAL; category=array_bounds
  Description: Array bounds violation: loop 8 > array 4
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::memory
  Context: h[4]+=e; h[5]+=f; h[6]+=g; h[7]+=hh;
- Line 100: severity=CRITICAL; category=array_bounds_violation
  Description: Array bounds violation: loop 8 > array size 4
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: hh = g; g = f; f = e; e = d + tmp1;

            d  = c; c = b; b = a; a = tmp1 + tmp2;

        }



        h[0]+=a; h[1]+=b; h[2]+=c; h[3]+=d;

        h[4]+=e; h[5]+=f; h[6]+=g; h[7]+=hh;

    }



    std::array<uint8_t, 32> digest{};

    for (int i = 0; i < 8; ++i) {

        digest[4*i    ] = static_cast<uint8_t>(h[i] >> 24u);
- Line 58: severity=HIGH; category=arithmetic_overflow
  Description: Arithmetic operation result assigned to variable (potential overflow)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::type_conversion
  Context: ['    // Pre-processing: padding', '    const uint64_t bit_len = static_cast<uint64_t>(len) * 8u;', '    size_t padded_len = len + 1;', '    while (padded_len % 64 != 56) ++padded_len;', '    padded_len += 8;']
- Line 64: severity=HIGH; category=unchecked_array_index
  Description: Array index not validated before access
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::input_validation
  Context: ['    std::vector<uint8_t> msg(padded_len, 0);', '    std::copy(data, data + len, msg.begin());', '    msg[len] = 0x80u;', '    // Big-endian bit length at the end', '    for (int i = 0; i < 8; ++i) {']
- Line 72: severity=HIGH; category=uninitialized_array
  Description: Array contains garbage values; use with zeros or explicit init
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::uninitialized
  Context: Array declared without initialization
- Line 136: severity=MEDIUM; category=missing_move_constructor_defaulted
  Description: Missing move ctor prevents efficient rvalue transfers
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 152: severity=MEDIUM; category=missing_move_constructor_defaulted
  Description: Missing move ctor prevents efficient rvalue transfers
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety

### network/service_mesh.cpp
Total findings: 12

- Line 135: severity=CRITICAL; category=no_timeout
  Description: file_io without timeout — can block indefinitely
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: boost::asio::write(socket, net::buffer(resp), ec);
- Line 146: severity=CRITICAL; category=no_timeout
  Description: file_io without timeout — can block indefinitely
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: boost::asio::write(socket, net::buffer(resp), ec);
- Line 190: severity=CRITICAL; category=no_timeout
  Description: file_io without timeout — can block indefinitely
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: acceptor_->open(ep.protocol());
- Line 241: severity=CRITICAL; category=thread_join_no_timeout
  Description: Thread join/wait without timeout (blocking indefinitely)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: t.join();
- Line 158: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: while (running_.load(std::memory_order_acquire)) {
- Line 112: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: req.find("GET /healthz\r") != std::string::npos);
- Line 115: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: req.find("GET /readyz\r")  != std::string::npos);
- Line 131: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: "HTTP/1.1 404 Not Found\r\n"
- Line 140: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: "HTTP/1.1 200 OK\r\n"
- Line 141: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: "Content-Type: text/plain\r\n"
- Line 174: severity=MEDIUM; category=missing_health_check
  Description: Service initialization without nearby health/status handling
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: bool ServiceMeshIntegration::start() {
- Line 236: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: acceptor_->close(ec);

### network/wire_protocol_v2.cpp
Total findings: 12

- Line 170: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: hdr.payload_length = static_cast<uint32_t>(payload->size());
- Line 666: severity=CRITICAL; category=thread_join_no_timeout
  Description: Thread join/wait without timeout (blocking indefinitely)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: if (t.joinable()) t.join();
- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: * PR History (last 5): #5081 [Docs][themis] Refresh modu... (2026-05-13) | #4267 feat(themis): Wire
- Line 122: severity=HIGH; category=memory_order
  Description: memory_order_relaxed used — potential visibility issue
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: oss << "v2conn-" << counter.fetch_add(1, std::memory_order_relaxed);
- Line 70: severity=MEDIUM; category=endianness_assumption
  Description: Endianness conversion may have undefined behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: uint32_t len_be     = htonl32(h.payload_length);
- Line 190: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: const std::unordered_map<std::string, std::string>& headers) override {
- Line 205: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: encoded += k + ": " + v + "\n";
- Line 227: severity=MEDIUM; category=endianness_assumption
  Description: Endianness conversion may have undefined behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: uint32_t ec_be = htonl32(error_code);
- Line 251: severity=MEDIUM; category=endianness_assumption
  Description: Endianness conversion may have undefined behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: uint32_t ec_be   = htonl32(error_code);
- Line 262: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: socket_.close();
- Line 656: severity=MEDIUM; category=expensive_inner_op
  Description: I/O operation in inner loop — very expensive
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: std::cerr << "[WireV2] I/O thread error: " << e.what() << '\n';
- Line 656: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: std::cerr << "[WireV2] I/O thread error: " << e.what() << '\n';

### network/adaptive_circuit_breaker.cpp
Total findings: 10

- Line 53: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: if (state_.load(std::memory_order_acquire) == CircuitState::CLOSED) {
- Line 92: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: const CircuitState s = state_.load(std::memory_order_acquire);
- Line 131: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: const CircuitState s = state_.load(std::memory_order_acquire);
- Line 164: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: return state_.load(std::memory_order_acquire);
- Line 217: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 219: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 223: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 226: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 229: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 231: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety

### network/udp_server.cpp
Total findings: 10

- Line 87: severity=CRITICAL; category=thread_join_no_timeout
  Description: Thread join/wait without timeout (blocking indefinitely)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: batch_thread_.join();
- Line 99: severity=CRITICAL; category=thread_join_no_timeout
  Description: Thread join/wait without timeout (blocking indefinitely)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: if (t.joinable()) t.join();
- Line 70: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: io_threads_.emplace_back([this] { io_ctx_->run(); });
- Line 96: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: io_ctx_->stop();
- Line 103: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: io_ctx_->restart();
- Line 136: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: if (running_.load(std::memory_order_acquire)) {
- Line 359: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: std::unique_lock<std::mutex> lk(batch_mutex_);
- Line 47: severity=MEDIUM; category=missing_health_check
  Description: Service initialization without nearby health/status handling
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: void UDPServer::start() {
- Line 94: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: socket_->close(ec);
- Line 469: severity=MEDIUM; category=endianness_assumption
  Description: Endianness conversion may have undefined behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: const uint32_t seq_be = htonl(seq_num);

### network/udp_fast_path.cpp
Total findings: 8

- Line 86: severity=CRITICAL; category=thread_join_no_timeout
  Description: Thread join/wait without timeout (blocking indefinitely)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: if (t.joinable()) t.join();
- Line 70: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: threads_.emplace_back([this] { io_ctx_->run(); });
- Line 83: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: io_ctx_->stop();
- Line 90: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: io_ctx_->restart();
- Line 124: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: if (running_.load(std::memory_order_acquire)) {
- Line 253: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 176: severity=MEDIUM; category=endianness_assumption
  Description: Endianness conversion may have undefined behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: const uint32_t request_id = ntohl(req_id_be);
- Line 397: severity=MEDIUM; category=endianness_assumption
  Description: Endianness conversion may have undefined behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: const uint32_t req_id_be = htonl(request_id);

### network/wire_protocol_performance.cpp
Total findings: 8

- Line 221: severity=CRITICAL; category=blocking_no_timeout
  Description: Blocking operation without timeout — can block indefinitely
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: PayloadBufferPool::~PayloadBufferPool() = default;



PayloadBufferPool::Handle PayloadBufferPool::acquire() {

    std::unique_ptr<Buffer> buf;



    {
- Line 221: severity=CRITICAL; category=no_timeout
  Description: semaphore_wait without timeout — can block indefinitely
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: PayloadBufferPool::Handle PayloadBufferPool::acquire() {
- Line 206: severity=HIGH; category=manual_cleanup_in_destructor
  Description: Manual resource cleanup in destructor (should use RAII wrapper)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_raii
  Context: PayloadBufferPool::Handle::~Handle() {
- Line 221: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: PayloadBufferPool::Handle PayloadBufferPool::acquire() {
- Line 183: severity=MEDIUM; category=missing_latency_metric
  Description: No latency measurement for operation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: PayloadBufferPool::Handle::Handle(Handle&& o) noexcept
- Line 190: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: release();
- Line 198: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: void PayloadBufferPool::Handle::release() noexcept {
- Line 206: severity=MEDIUM; category=missing_latency_metric
  Description: No latency measurement for operation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: PayloadBufferPool::Handle::~Handle() {

### network/connection_compression.cpp
Total findings: 7

- Line 103: severity=CRITICAL; category=unchecked_memcpy
  Description: memcpy size parameter not validated (potential buffer overflow)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::input_validation
  Context: ['    size_t offset = 0;', '    for (size_t i = 0; i < samples.size(); ++i) {', '        std::memcpy(concat.data() + offset, samples[i].data(), samples[i].size());', '        sample_sizes[i] = samples[i].size();', '        offset += samples[i].size();']
- Line 126: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 130: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 132: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 133: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 138: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 139: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety

### network/grpc_transport.cpp
Total findings: 7

- Line 252: severity=CRITICAL; category=thread_join_no_timeout
  Description: Thread join/wait without timeout (blocking indefinitely)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: t.join();
- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: * PR History (last 5): #4330 feat(cache): network-backed... (2026-03-19) | #3577 [MODULE] network +
- Line 160: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: std::lock_guard<std::mutex> lk(stats_mutex_);
- Line 174: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: if (running_.load(std::memory_order_acquire)) {
- Line 18: severity=MEDIUM; category=missing_correlation_id
  Description: Distributed call without correlation ID
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: #include <grpcpp/generic/async_generic_service.h>
- Line 151: severity=MEDIUM; category=primitive_no_volatile
  Description: Primitive shared across threads without volatile
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: bool  ok     = false;
- Line 173: severity=MEDIUM; category=missing_health_check
  Description: Service initialization without nearby health/status handling
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: bool GrpcTransport::start() {

### network/io_uring_batcher.cpp
Total findings: 5

- Line 240: severity=CRITICAL; category=unchecked_memcpy
  Description: memcpy size parameter not validated (potential buffer overflow)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::input_validation
  Context: ['    std::memcpy(sqe + 16, &addr,     sizeof(addr));   // addr (iovec ptr)', '    uint32_t len = static_cast<uint32_t>(iov_cnt);', '    std::memcpy(sqe + 24, &len,      sizeof(len));    // len  (iov_cnt)', '    std::memcpy(sqe + 32, &user_data,sizeof(user_data)); // user_data', '']
- Line 236: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: std::memcpy(sqe + 4,  &fd_i,     sizeof(fd_i));  // fd
- Line 238: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: std::memcpy(sqe + 16, &addr,     sizeof(addr));   // addr (iovec ptr)
- Line 240: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: std::memcpy(sqe + 24, &len,      sizeof(len));    // len  (iov_cnt)
- Line 344: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: ::close(ring_fd_);

### network/wire_protocol_server_ws.cpp
Total findings: 5

- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: * PR History (last 5): #3632 fix(build): register 40+ mi... (2026-03-12) | #3388 feat(network): impl
- Line 549: severity=HIGH; category=unchecked_array_index
  Description: Array index not validated before access
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::input_validation
  Context: ['        const size_t crc_offset = kWireHeaderSize + payload_size;', '        const uint32_t expected_crc =', '            (static_cast<uint32_t>(data[crc_offset]) << 24) |', '            (static_cast<uint32_t>(data[crc_offset + 1]) << 16) |', '            (static_cast<uint32_t>(data[crc_offset + 2]) << 8) |']
- Line 120: severity=MEDIUM; category=generic_catch
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: } else {

        try {

            client_ip_ = ws_.next_layer().socket().remote_endpoint().address().to_string();

        } catch (...) {

            client_ip_ = "unknown";

        }

    }
- Line 120: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: } catch (...) {
- Line 572: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: close();

### network/socket_timeout_manager.cpp
Total findings: 4

- Line 60: severity=CRITICAL; category=missing_dtor
  Description: Class timeval allocates resources but has no destructor
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: class/struct timeval
- Line 191: severity=CRITICAL; category=smart_ptr_misuse
  Description: Raw new without immediate wrapping in smart pointer
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: spdlog::warn("Circuit breaker is open, refusing new connections");
- Line 191: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 362: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: close(socket);

### network/wire_protocol_helpers.cpp
Total findings: 2

- Line 234: severity=HIGH; category=no_retry_logic
  Description: RPC/network call without retry logic — transient failures will propagate
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: }

    

    // Validate required fields

    if (request.collection.empty()) {

        return false;

    }
- Line 291: severity=HIGH; category=fp_exact_comparison
  Description: Floating-point exact comparison (use tolerance/epsilon)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: if (data_density != 0.0) {

### network/PRODUCTION_REQUIREMENTS.md
Total findings: 1

- Line 1: severity=LOW; category=module_doc_linkset_drift
  Description: Module doc 'PRODUCTION_REQUIREMENTS.md' is missing expected cross-links: FUTURE_ENHANCEMENTS.md, README.md, ROADMAP.md
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::themis_doc_freshness_rules
  Context: Refresh the top-level <!-- Links: ... --> metadata to match the current module doc set

### network/ROADMAP.md
Total findings: 1

- Line 1: severity=LOW; category=module_doc_linkset_drift
  Description: Module doc 'ROADMAP.md' is missing expected cross-links: ARCHITECTURE.md, FUTURE_ENHANCEMENTS.md, README.md
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::themis_doc_freshness_rules
  Context: Refresh the top-level <!-- Links: ... --> metadata to match the current module doc set

### network/geo_topology_router.cpp
Total findings: 1

- Line 104: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: (it != config_.region_latency_hints.end()) ? it->second : kUnhintedRegionLatencyMs;

## Update Workflow

- Refresh scanner artifacts with: python tools/gs3_orchestrator.py ./src --output ai_working/gap_scan_results.json
- Regenerate docs with: python tools/module_doc_generator.py . ai_working ai_working/module_gaps
- Add --no-mirror when you only want archive docs in ai_working/module_gaps.

Format: THEMIS_MODULE_GAPS_V4
