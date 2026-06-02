# network Module - Developer Gap Note

> Auto-generated from ai_working/gap_scan_v3_aggregate.json.
> This file is overwritten on each regeneration.

## Scan Snapshot

- Module: network
- Generated: 2026-06-02 11:09:13
- Status: Critical Findings Present
- Total Findings: 91
- Actionable Findings (Critical + High): 18
- Affected Files: 24

## Severity Summary

| Severity | Count |
|---|---:|
| Critical | 1 |
| High | 17 |
| Medium | 73 |
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
| src/network/wire_protocol_server.cpp | 28 | 1 | 0 | 27 | 0 |
| src/network/raft_load_balancer.cpp | 13 | 0 | 8 | 5 | 0 |
| src/network/envoy_xds.cpp | 10 | 0 | 1 | 9 | 0 |
| src/network/kernel_bypass.cpp | 10 | 0 | 3 | 7 | 0 |
| src/network/wire_protocol_connection_pool.cpp | 6 | 0 | 3 | 3 | 0 |
| src/network/grpc_transport.cpp | 3 | 0 | 0 | 3 | 0 |
| src/network/qos_manager.cpp | 3 | 0 | 0 | 3 | 0 |
| src/network/quic_server.cpp | 3 | 0 | 1 | 2 | 0 |
| src/network/wire_protocol_helpers.cpp | 3 | 0 | 1 | 2 | 0 |
| src/network/wire_protocol_v2.cpp | 3 | 0 | 0 | 3 | 0 |
| src/network/udp_server.cpp | 2 | 0 | 0 | 2 | 0 |
| src/network/wire_protocol_performance.cpp | 2 | 0 | 0 | 2 | 0 |
| src/network/network_audit_log.cpp | 1 | 0 | 0 | 1 | 0 |
| src/network/quic_transport.cpp | 1 | 0 | 0 | 1 | 0 |
| src/network/service_mesh.cpp | 1 | 0 | 0 | 1 | 0 |
| src/network/udp_fast_path.cpp | 1 | 0 | 0 | 1 | 0 |
| src/network/wire_protocol_batch.cpp | 1 | 0 | 0 | 1 | 0 |
| src/network/adaptive_circuit_breaker.cpp | 0 | 0 | 0 | 0 | 0 |
| src/network/connection_compression.cpp | 0 | 0 | 0 | 0 | 0 |
| src/network/geo_topology_router.cpp | 0 | 0 | 0 | 0 | 0 |
| src/network/io_uring_batcher.cpp | 0 | 0 | 0 | 0 | 0 |
| src/network/socket_timeout_manager.cpp | 0 | 0 | 0 | 0 | 0 |
| src/network/wire_protocol_server_ws.cpp | 0 | 0 | 0 | 0 | 0 |
| src/network/wire_protocol_zero_copy.cpp | 0 | 0 | 0 | 0 | 0 |

## Full Scanner Findings

### src/network/wire_protocol_server.cpp
Total findings: 28

- Line 344: severity=CRITICAL; category=audit_logging; pattern=sensitive_data_logging
  Description: Potential PII/credential logging: auth
  Context: std::cerr << "[WireProtocol] Invalid configuration: auth_token length exceeds AUTH "
  Confidence: band=very_high; score=0.92
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
- Line 709: severity=MEDIUM; category=observability; pattern=missing_health_check
  Description: Service initialization without nearby health/status handling
  Context: , client_ip_("unknown")  // Will be set after accept in start()
  Confidence: band=medium; score=0.66
- Line 717: severity=MEDIUM; category=observability; pattern=missing_health_check
  Description: Service initialization without nearby health/status handling
  Context: void WireProtocolServer::Session::start() {
  Confidence: band=medium; score=0.66
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
- Line 2428: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: batch.push_back(entry.results[i]);
  Confidence: band=high; score=0.74
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
- Line 3162: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: response.buckets.push_back(bucket);
  Confidence: band=high; score=0.74
- Line 3218: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: void WireProtocolServer::Session::handleBpmnStartProcess() {
  Confidence: band=high; score=0.74
- Line 3329: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: active_tasks.push_back(instance_id + ":" + token.current_node);
  Confidence: band=high; score=0.74
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

### src/network/raft_load_balancer.cpp
Total findings: 13

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
- Line 296: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: if (b->healthy) result.push_back(b.get());
  Confidence: band=high; score=0.74

### src/network/envoy_xds.cpp
Total findings: 10

- Line 539: severity=HIGH; category=performance; pattern=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Context: const auto q1 = dom_body.find('"', pos);
  Confidence: band=very_high; score=0.9
- Line 46: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: case '"':  out += "\\\""; break;
  Confidence: band=high; score=0.74
- Line 79: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: case '"': value += '"'; break;
  Confidence: band=high; score=0.74
- Line 167: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: items.push_back(array_body.substr(item_start, i - item_start + 1));
  Confidence: band=high; score=0.74
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

### src/network/kernel_bypass.cpp
Total findings: 10

- Line 192: severity=HIGH; category=observability; pattern=missing_trace_point
  Description: Critical function allocate without trace point
  Context: void* NumaAllocator::allocate(size_t size, int node) {
  Confidence: band=very_high; score=0.9
- Line 221: severity=HIGH; category=observability; pattern=missing_trace_point
  Description: Critical function deallocate without trace point
  Context: void NumaAllocator::deallocate(void* ptr, size_t size) noexcept {
  Confidence: band=very_high; score=0.9
- Line 631: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (uint16_t i = 0; i < nb_rx; ++i) {
  Confidence: band=very_high; score=0.9
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
- Line 441: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: eal_argv.push_back(const_cast<char*>(s.c_str()));
  Confidence: band=high; score=0.74
- Line 570: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: workers_.emplace_back([this, core, qid]() {
  Confidence: band=high; score=0.74
- Line 813: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: iovecs.push_back({buf->data(), buf->size()});
  Confidence: band=high; score=0.74
- Line 869: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: workers_.emplace_back([this, i]() { workerLoop(static_cast<int>(i)); });
  Confidence: band=high; score=0.74

### src/network/wire_protocol_connection_pool.cpp
Total findings: 6

- Line 513: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (auto& [target, pool] : target_pools_) {
  Confidence: band=very_high; score=0.9
- Line 647: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (auto& [target, pool] : target_pools_) {
  Confidence: band=very_high; score=0.9
- Line 685: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto& [target, pool] : target_pools_) {
  Confidence: band=very_high; score=0.9
- Line 551: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: targets.push_back(t);
  Confidence: band=high; score=0.74
- Line 746: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: WireProtocolConnectionPool::ConnectionHandle::~ConnectionHandle() {
  Confidence: band=high; score=0.74
- Line 752: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: WireProtocolConnectionPool::ConnectionHandle::ConnectionHandle(ConnectionHandle&& other) noexcept
  Confidence: band=high; score=0.74

### src/network/grpc_transport.cpp
Total findings: 3

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

### src/network/qos_manager.cpp
Total findings: 3

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

### src/network/quic_server.cpp
Total findings: 3

- Line 174: severity=HIGH; category=legacy_duplication; pattern=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Context: // Advertise HTTP/3 ALPN ("h3") for compatibility with standard HTTP/3
  Confidence: band=high; score=0.8
- Line 236: severity=MEDIUM; category=observability; pattern=missing_health_check
  Description: Service initialization without nearby health/status handling
  Context: void QUICServer::start() {
  Confidence: band=medium; score=0.66
- Line 282: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: threads_.emplace_back([this] { io_ctx_->run(); });
  Confidence: band=high; score=0.74

### src/network/wire_protocol_helpers.cpp
Total findings: 3

- Line 291: severity=HIGH; category=determinism; pattern=fp_exact_comparison
  Description: Floating-point exact comparison (use tolerance/epsilon)
  Context: if (data_density != 0.0) {
  Confidence: band=very_high; score=0.9
- Line 143: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: data_.push_back(static_cast<uint8_t>(value & 0xFF));
  Confidence: band=high; score=0.74
- Line 151: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: data_.push_back(static_cast<uint8_t>(value & 0xFF));
  Confidence: band=high; score=0.74

### src/network/wire_protocol_v2.cpp
Total findings: 3

- Line 190: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: const std::unordered_map<std::string, std::string>& headers) override {
  Confidence: band=medium; score=0.66
- Line 205: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: encoded += k + ": " + v + "\n";
  Confidence: band=high; score=0.74
- Line 652: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: io_threads_.emplace_back([this]() {
  Confidence: band=high; score=0.74

### src/network/udp_server.cpp
Total findings: 2

- Line 47: severity=MEDIUM; category=observability; pattern=missing_health_check
  Description: Service initialization without nearby health/status handling
  Context: void UDPServer::start() {
  Confidence: band=medium; score=0.66
- Line 69: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: io_threads_.emplace_back([this] { io_ctx_->run(); });
  Confidence: band=high; score=0.74

### src/network/wire_protocol_performance.cpp
Total findings: 2

- Line 183: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: PayloadBufferPool::Handle::Handle(Handle&& o) noexcept
  Confidence: band=high; score=0.74
- Line 206: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: PayloadBufferPool::Handle::~Handle() {
  Confidence: band=high; score=0.74

### src/network/network_audit_log.cpp
Total findings: 1

- Line 246: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: if (ev.type == type) result.push_back(ev);
  Confidence: band=high; score=0.74

### src/network/quic_transport.cpp
Total findings: 1

- Line 179: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: threads_.emplace_back([this] { io_ctx_->run(); });
  Confidence: band=high; score=0.74

### src/network/service_mesh.cpp
Total findings: 1

- Line 174: severity=MEDIUM; category=observability; pattern=missing_health_check
  Description: Service initialization without nearby health/status handling
  Context: bool ServiceMeshIntegration::start() {
  Confidence: band=medium; score=0.66

### src/network/udp_fast_path.cpp
Total findings: 1

- Line 69: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: threads_.emplace_back([this] { io_ctx_->run(); });
  Confidence: band=high; score=0.74

### src/network/wire_protocol_batch.cpp
Total findings: 1

- Line 174: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: posix_iov.push_back(item);
  Confidence: band=high; score=0.74

### src/network/adaptive_circuit_breaker.cpp
Total findings: 0


### src/network/connection_compression.cpp
Total findings: 0


### src/network/geo_topology_router.cpp
Total findings: 0


### src/network/io_uring_batcher.cpp
Total findings: 0


### src/network/socket_timeout_manager.cpp
Total findings: 0


### src/network/wire_protocol_server_ws.cpp
Total findings: 0


### src/network/wire_protocol_zero_copy.cpp
Total findings: 0

## Update Workflow

- Refresh scan artifacts with: python tools/gap_scanner_v3.py
- Regenerate all module notes with: python tools/module_doc_generator.py . ai_working ai_working/module_gaps
- The generator mirrors each archive document directly into src/<module>/MODULE_GAPS.md.

Format: THEMIS_MODULE_GAPS_V3
