# themis Module - Developer Gap Note

> Auto-generated from ai_working/gap_scan_v3_aggregate.json.
> This file is overwritten on each regeneration.

## Scan Snapshot

- Module: themis
- Generated: 2026-05-31 08:50:11
- Status: Critical Findings Present
- Total Findings: 238
- Actionable Findings (Critical + High): 83
- Affected Files: 12

## Severity Summary

| Severity | Count |
|---|---:|
| Critical | 15 |
| High | 68 |
| Medium | 155 |
| Low | 0 |

## Category Summary

| Category | Count |
|---|---:|
| raii | 63 |
| container | 50 |
| performance_patterns | 34 |
| reliability | 24 |
| platform | 20 |
| concurrency | 13 |
| exception_safety | 9 |
| input_validation | 5 |
| audit_logging | 4 |
| memory | 4 |
| observability | 3 |
| security | 3 |
| legacy_duplication | 2 |
| performance | 2 |
| uninitialized | 2 |

## File Overview

| File | Findings | Critical | High | Medium | Low |
|---|---:|---:|---:|---:|---:|
| src/themis/wire_protocol_server.cpp | 60 | 2 | 28 | 30 | 0 |
| src/themis/module_loader.cpp | 45 | 1 | 10 | 34 | 0 |
| src/themis/build_info.cpp | 32 | 0 | 1 | 31 | 0 |
| src/themis/module_dependency_resolver.cpp | 28 | 1 | 1 | 26 | 0 |
| src/themis/license_info.cpp | 19 | 3 | 10 | 6 | 0 |
| include/themis/network/wire_protocol_server.hpp | 13 | 3 | 4 | 6 | 0 |
| src/themis/module_loader_linux.cpp | 12 | 1 | 5 | 6 | 0 |
| src/themis/module_signature_verifier.cpp | 9 | 1 | 3 | 5 | 0 |
| include/themis/network/wire_protocol_v2.hpp | 6 | 2 | 1 | 3 | 0 |
| src/themis/module_hash_verifier.cpp | 6 | 1 | 1 | 4 | 0 |
| src/themis/edition_manager.cpp | 4 | 0 | 2 | 2 | 0 |
| src/themis/module_loader_win32.cpp | 4 | 0 | 2 | 2 | 0 |

## Full Scanner Findings

### src/themis/wire_protocol_server.cpp
Total findings: 60

- Line 0: severity=CRITICAL; category=uncategorized
  Context: ['    const uint32_t code_be = htonl(err_code);', '    std::memcpy(payload.data(), &code_be, 4u);', '    std::memcpy(payload.data() + 4u, message.data(), message.size());', '', '    WireFrameHeader hdr{};']
  Confidence: band=very_high; score=0.93
- Line 1063: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto it = cursors_.find(req.cursor_id());
- Line 22: severity=HIGH; category=legacy_duplication; pattern=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Context: // (themis::network namespace) for backward compatibility during the v1.7.0
  Confidence: band=high; score=0.8
- Line 75: severity=HIGH; category=deadlock_risk
  Description: Multiple locks acquired in nested scope — potential deadlock
  Remediation: Use std::scoped_lock(m1, m2) or enforce consistent lock ordering
  Context: std::lock_guard<std::mutex> lock(g_wire_bridge_mutex);
- Line 79: severity=HIGH; category=deadlock_risk
  Description: Multiple locks acquired in nested scope — potential deadlock
  Remediation: Use std::scoped_lock(m1, m2) or enforce consistent lock ordering
  Context: std::lock_guard<std::mutex> lock(g_wire_bridge_mutex);
- Line 83: severity=HIGH; category=deadlock_risk
  Description: Multiple locks acquired in nested scope — potential deadlock
  Remediation: Use std::scoped_lock(m1, m2) or enforce consistent lock ordering
  Context: std::lock_guard<std::mutex> lock(g_wire_bridge_mutex);
- Line 87: severity=HIGH; category=deadlock_risk
  Description: Multiple locks acquired in nested scope — potential deadlock
  Remediation: Use std::scoped_lock(m1, m2) or enforce consistent lock ordering
  Context: std::lock_guard<std::mutex> lock(g_wire_bridge_mutex);
- Line 194: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: crc = kCrc32Table[(crc ^ data[i]) & 0xFFu] ^ (crc >> 8);
- Line 316: severity=HIGH; category=deadlock_risk
  Description: Multiple locks acquired in nested scope — potential deadlock
  Remediation: Use std::scoped_lock(m1, m2) or enforce consistent lock ordering
  Context: std::lock_guard<std::mutex> lock(s_bridge_mutex);
- Line 321: severity=HIGH; category=deadlock_risk
  Description: Multiple locks acquired in nested scope — potential deadlock
  Remediation: Use std::scoped_lock(m1, m2) or enforce consistent lock ordering
  Context: std::lock_guard<std::mutex> lock(s_bridge_mutex);
- Line 645: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: handle_geo_query(req);
- Line 653: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: handle_timeseries_query(req);
- Line 851: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: [[maybe_unused]] const std::vector<uint8_t>& data) {
- Line 981: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (uint32_t i = 0; i < batch_sz; ++i)
  Confidence: band=very_high; score=0.9
- Line 988: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: std::snprintf(buf, sizeof(buf), "c-%s-%lld",
  Confidence: band=very_high; score=0.9
- Line 1052: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (size_t i = entry.offset; i < end; ++i)
  Confidence: band=very_high; score=0.9
- Line 1129: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: void WireProtocolSession::handle_geo_query(
- Line 1170: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: void WireProtocolSession::handle_timeseries_query(
- Line 1578: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto& kv : sessions_)
  Confidence: band=very_high; score=0.9
- Line 1585: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto& session : sessions_to_close)
  Confidence: band=very_high; score=0.9
- Line 1590: severity=HIGH; category=deadlock_risk
  Description: Multiple locks acquired in nested scope — potential deadlock
  Remediation: Use std::scoped_lock(m1, m2) or enforce consistent lock ordering
  Context: std::lock_guard<std::mutex> lock(state_mutex_);
- Line 1607: severity=HIGH; category=deadlock_risk
  Description: Multiple locks acquired in nested scope — potential deadlock
  Remediation: Use std::scoped_lock(m1, m2) or enforce consistent lock ordering
  Context: std::lock_guard<std::mutex> lock(state_mutex_);
- Line 1612: severity=HIGH; category=deadlock_risk
  Description: Multiple locks acquired in nested scope — potential deadlock
  Remediation: Use std::scoped_lock(m1, m2) or enforce consistent lock ordering
  Context: std::lock_guard<std::mutex> lock(state_mutex_);
- Line 1617: severity=HIGH; category=deadlock_risk
  Description: Multiple locks acquired in nested scope — potential deadlock
  Remediation: Use std::scoped_lock(m1, m2) or enforce consistent lock ordering
  Context: std::lock_guard<std::mutex> lock(state_mutex_);
- Line 1622: severity=HIGH; category=deadlock_risk
  Description: Multiple locks acquired in nested scope — potential deadlock
  Remediation: Use std::scoped_lock(m1, m2) or enforce consistent lock ordering
  Context: std::lock_guard<std::mutex> lock(state_mutex_);
- Line 1646: severity=HIGH; category=no_retry_logic
  Description: socket_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: void WireProtocolServer::async_accept() {
- Line 1647: severity=HIGH; category=no_retry_logic
  Description: socket_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: acceptor_.async_accept(
- Line 1656: severity=HIGH; category=no_retry_logic
  Description: socket_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: handle_accept(session, ec);
- Line 1660: severity=HIGH; category=no_retry_logic
  Description: socket_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: void WireProtocolServer::handle_accept(
- Line 1690: severity=HIGH; category=no_retry_logic
  Description: socket_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: if (should_continue) async_accept();
- Line 191: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: static uint32_t crc32Compute(const uint8_t* data, std::size_t len) {
  Confidence: band=high; score=0.74
- Line 201: severity=MEDIUM; category=endianness_assumption
  Description: Endianness conversion may have undefined behavior
  Remediation: Document endianness assumptions or use portable serialization
  Context: const uint32_t magic_be = htonl(h.magic);
- Line 207: severity=MEDIUM; category=endianness_assumption
  Description: Endianness conversion may have undefined behavior
  Remediation: Document endianness assumptions or use portable serialization
  Context: const uint32_t len_be = htonl(h.payload_length);
- Line 236: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 257: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(protoValueToJson(entry));
  Confidence: band=high; score=0.74
- Line 258: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.push_back(protoValueToJson(entry));
- Line 291: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: out += '?';
  Confidence: band=high; score=0.74
- Line 291: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: out += '?';
  Confidence: band=high; score=0.74
- Line 291: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: out += '?';
  Confidence: band=high; score=0.74
- Line 292: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: out += '?';
- Line 386: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: close();
- Line 399: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: void WireProtocolSession::close(const std::string& /*reason*/) {
- Line 411: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: socket_.close(ec);
- Line 434: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: close();
- Line 469: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: case OpCode::OP_CLOSE:  handle_close(v1::CloseRequest{});   break;
- Line 491: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: close();
- Line 601: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: handle_cursor_close(req);
- Line 693: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: handle_close(req);
- Line 752: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: close();
- Line 768: severity=MEDIUM; category=endianness_assumption
  Description: Endianness conversion may have undefined behavior
  Remediation: Document endianness assumptions or use portable serialization
  Context: const uint32_t code_be = htonl(err_code);
- Line 1081: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: void WireProtocolSession::handle_cursor_close(const v1::CursorCloseRequest& req) {
- Line 1400: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: close();
- Line 1508: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: void WireProtocolSession::handle_close(const v1::CloseRequest& /*req*/) {
- Line 1509: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: close("client requested close");
- Line 1544: severity=MEDIUM; category=observability; pattern=missing_health_check
  Description: Service initialization without nearby health/status handling
  Context: void WireProtocolServer::start() {
  Confidence: band=medium; score=0.66
- Line 1578: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: sessions_to_close.push_back(kv.second);
  Confidence: band=high; score=0.74
- Line 1579: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: sessions_to_close.push_back(kv.second);
- Line 1584: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: acceptor_.close(ec);
- Line 1586: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: session->close("server shutdown");
- Line 1681: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: session->close("server stopping");

### src/themis/module_loader.cpp
Total findings: 45

- Line 967: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto it = failureHistory_.find(modulePath);
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
- Line 469: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: unloadLibrary(it->second.handle);
- Line 988: severity=HIGH; category=legacy_duplication; pattern=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Context: // ABI compatibility
  Confidence: band=high; score=0.8
- Line 1242: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Acquire lock before loop or redesign to minimize lock time
  Context: std::unique_lock<std::mutex> lk(watchdogMutex_);
- Line 1244: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: watchdogCv_.wait_for(
  Confidence: band=very_high; score=0.9
- Line 1289: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: auto it = loadedModules_.find(name);
- Line 91: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: dlclose(handle);
- Line 521: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(module);
  Confidence: band=high; score=0.74
- Line 522: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.push_back(module);
- Line 773: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: ModuleMetadata ModuleLoader::extractMetadataFromHandle(void* handle) {
  Confidence: band=high; score=0.74
- Line 833: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: history.failureTimestamps.push_back(currentTime);
- Line 942: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: quarantined.push_back(path);
  Confidence: band=high; score=0.74
- Line 943: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: quarantined.push_back(path);
- Line 1139: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: module.healthChecks.push_back(healthResult);
  Confidence: band=high; score=0.74
- Line 1140: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: module.healthChecks.push_back(healthResult);
- Line 1275: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: snapshot.emplace_back(mod.name, mod.path);
  Confidence: band=high; score=0.74
- Line 1512: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: return sep == '/' || sep == '\\';
- Line 1512: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: return sep == '/' || sep == '\\';
- Line 1653: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: BIO_free(bio);
- Line 1661: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: EVP_PKEY_free(pkey);
- Line 1668: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: EVP_MD_CTX_free(ctx);
- Line 1669: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: EVP_PKEY_free(pkey);
- Line 1677: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: EVP_MD_CTX_free(ctx);
- Line 1678: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: EVP_PKEY_free(pkey);
- Line 1719: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: zip_close(archive);
- Line 1728: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: zip_close(archive);
- Line 1744: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: if (nameStr[0] == '/' || nameStr[0] == '\\') {
- Line 1744: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: if (nameStr[0] == '/' || nameStr[0] == '\\') {
- Line 1745: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: zip_close(archive);
- Line 1756: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: zip_close(archive);
- Line 1766: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: zip_close(archive);
- Line 1781: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: zip_close(archive);
- Line 1788: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: zip_fclose(zf);
- Line 1789: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: zip_close(archive);
- Line 1801: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: zip_fclose(zf);
- Line 1802: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: outFile.close();
- Line 1805: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: zip_close(archive);
- Line 1811: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: zip_close(archive);
- Line 1852: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: manifestFile.close();
- Line 1875: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: sigFile.close();

### src/themis/build_info.cpp
Total findings: 32

- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 60: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: static HsmModuleStatusFn fn;
- Line 168: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: config.modules.push_back({
- Line 177: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: config.modules.push_back({
- Line 184: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: config.modules.push_back({
- Line 216: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: config.modules.push_back({
- Line 225: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: config.modules.push_back({
- Line 232: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: config.modules.push_back({
- Line 411: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: config.modules.push_back({
- Line 420: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: config.modules.push_back({
- Line 427: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: config.modules.push_back({
- Line 437: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: config.modules.push_back({
- Line 444: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: config.modules.push_back({
- Line 453: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: config.modules.push_back({
- Line 460: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: config.modules.push_back({
- Line 469: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: config.modules.push_back({
- Line 476: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: config.modules.push_back({
- Line 486: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: config.modules.push_back({
- Line 493: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: config.modules.push_back({
- Line 509: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: config.modules.push_back({
- Line 518: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: config.modules.push_back({
- Line 525: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: config.modules.push_back({
- Line 534: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: config.modules.push_back({
- Line 541: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: config.modules.push_back({
- Line 564: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: HsmModuleStatusFn fn_copy;
- Line 574: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 875: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(mod.name);
  Confidence: band=high; score=0.74
- Line 875: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(mod.name);
  Confidence: band=high; score=0.74
- Line 876: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.push_back(mod.name);
- Line 886: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(mod.name);
  Confidence: band=high; score=0.74
- Line 887: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.push_back(mod.name);
- Line 954: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: EVP_MD_CTX_free(ctx);

### src/themis/module_dependency_resolver.cpp
Total findings: 28

- Line 211: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto it = modules_.find(n);
- Line 210: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: auto it = modules_.find(n);
- Line 113: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.push_back(std::move(info));
- Line 123: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: all.push_back(kv.first);
  Confidence: band=high; score=0.74
- Line 124: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: all.push_back(kv.first);
- Line 133: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: std::map<std::string, bool> visited;
  Confidence: band=high; score=0.74
- Line 138: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: precheck.missingRequired.push_back(n);
  Confidence: band=high; score=0.74
- Line 139: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: precheck.missingRequired.push_back(n);
- Line 185: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: closure.push_back(kv.first);
  Confidence: band=high; score=0.74
- Line 186: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: closure.push_back(kv.first);
- Line 203: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: std::map<std::string, int> inDegree;
  Confidence: band=high; score=0.74
- Line 204: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: std::map<std::string, std::vector<std::string>> dependents;
  Confidence: band=high; score=0.74
- Line 220: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.missingRequired.push_back(dep.name);
  Confidence: band=high; score=0.74
- Line 220: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.missingRequired.push_back(dep.name);
  Confidence: band=high; score=0.74
- Line 220: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.missingRequired.push_back(dep.name);
  Confidence: band=high; score=0.74
- Line 221: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.missingRequired.push_back(dep.name);
- Line 227: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.missingRequired.push_back(dep.name);
- Line 245: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.versionMismatches.push_back(oss.str());
- Line 250: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: dependents[dep.name].push_back(n);
- Line 272: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.versionMismatches.push_back(oss.str());
- Line 316: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: ready.push_back(kv.first);
  Confidence: band=high; score=0.74
- Line 316: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: ready.push_back(kv.first);
  Confidence: band=high; score=0.74
- Line 317: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: ready.push_back(kv.first);
- Line 325: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.loadOrder.push_back(cur);
- Line 340: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: cycleNodes.push_back(kv.first);
  Confidence: band=high; score=0.74
- Line 340: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: cycleNodes.push_back(kv.first);
  Confidence: band=high; score=0.74
- Line 341: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: cycleNodes.push_back(kv.first);
- Line 345: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.cycles.push_back(cycleNodes);

### src/themis/license_info.cpp
Total findings: 19

- Line 200: severity=CRITICAL; category=audit_logging; pattern=sensitive_data_logging
  Description: Potential PII/credential logging: email
  Context: oss << "  Contact Email:      " << license.contact_email << "\n";
  Confidence: band=very_high; score=0.92
- Line 412: severity=CRITICAL; category=missing_dtor
  Description: Class ifaddrs allocates resources but has no destructor
  Remediation: Add explicit destructor: ~ifaddrs() { /* cleanup */ }
  Context: class/struct ifaddrs
- Line 416: severity=CRITICAL; category=missing_dtor
  Description: Class ifaddrs allocates resources but has no destructor
  Remediation: Add explicit destructor: ~ifaddrs() { /* cleanup */ }
  Context: class/struct ifaddrs
- Line 0: severity=HIGH; category=uncategorized
  Context: ['// Compute a hex-encoded SHA-256 of the primary MAC address (or a fallback)', 'static std::string computeFingerprintHash(const std::string& raw) {', '    unsigned char digest[EVP_MAX_MD_SIZE] = {};', '    unsigned int  dlen = 0;', '    EVP_MD_CTX* ctx = EVP_MD_CTX_new();']
  Confidence: band=high; score=0.78
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Context: Array declared without initialization
  Confidence: band=high; score=0.81
- Line 7: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: * PR: #4518 [WIP] Update developer documentation to match current source code (2026-04-12T20:32:47Z)
- Line 420: severity=HIGH; category=no_retry_logic
  Description: socket_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: int sock = socket(AF_INET, SOCK_DGRAM, 0);
- Line 429: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: std::snprintf(buf, sizeof(buf),
  Confidence: band=very_high; score=0.9
- Line 447: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: std::snprintf(buf, sizeof(buf),
  Confidence: band=very_high; score=0.9
- Line 551: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: out->append(static_cast<char*>(ptr), size * nmemb);
- Line 758: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: return impl_->refresh();
- Line 137: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: "QscQaIyIKDiREBnYUmDZXEsCg5HmYgLzGEcNdHd/IxA5vp3Qr\n" \
- Line 309: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 320: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: if (!b64) { BIO_free(bmem); return {}; }
- Line 401: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: EVP_MD_CTX_free(ctx);
- Line 425: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: ifr.ifr_name[IFNAMSIZ - 1] = '\0'; // ensure null termination
- Line 434: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: close(sock);

### include/themis/network/wire_protocol_server.hpp
Total findings: 13

- Line 156: severity=CRITICAL; category=missing_dtor
  Description: Class MessageFlags allocates resources but has no destructor
  Remediation: Add explicit destructor: ~MessageFlags() { /* cleanup */ }
  Context: class/struct MessageFlags
- Line 168: severity=CRITICAL; category=missing_dtor
  Description: Class WireFrameHeader allocates resources but has no destructor
  Remediation: Add explicit destructor: ~WireFrameHeader() { /* cleanup */ }
  Context: class/struct WireFrameHeader
- Line 545: severity=CRITICAL; category=missing_dtor
  Description: Class CursorEntry allocates resources but has no destructor
  Remediation: Add explicit destructor: ~CursorEntry() { /* cleanup */ }
  Context: class/struct CursorEntry
- Line 499: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: void handle_geo_query(const v1::GeoQueryRequest& req);
- Line 500: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: void handle_timeseries_query(const v1::TimeSeriesQueryRequest& req);
- Line 607: severity=HIGH; category=no_retry_logic
  Description: socket_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: void async_accept();
- Line 608: severity=HIGH; category=no_retry_logic
  Description: socket_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: void handle_accept(std::shared_ptr<WireProtocolSession> session, const boost::system::error_code& er
- Line 12: severity=MEDIUM; category=unportable_pragma
  Description: Non-standard pragma may not be supported on all platforms
  Remediation: Use standard C++ mechanisms: static_assert, or move to portable config
  Context: #pragma once
- Line 167: severity=MEDIUM; category=unportable_pragma
  Description: Non-standard pragma may not be supported on all platforms
  Remediation: Use standard C++ mechanisms: static_assert, or move to portable config
  Context: #pragma pack(push, 1)
- Line 187: severity=MEDIUM; category=unportable_pragma
  Description: Non-standard pragma may not be supported on all platforms
  Remediation: Use standard C++ mechanisms: static_assert, or move to portable config
  Context: #pragma pack(pop)
- Line 390: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: void close(const std::string& reason = "");
- Line 493: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: void handle_cursor_close(const v1::CursorCloseRequest& req);
- Line 505: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: void handle_close(const v1::CloseRequest& req);

### src/themis/module_loader_linux.cpp
Total findings: 12

- Line 110: severity=CRITICAL; category=no_timeout
  Description: file_io without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: while ((n = read(pipefd[0], buf, sizeof(buf) - 1)) > 0) {
- Line 0: severity=HIGH; category=uncategorized
  Context: Array declared without initialization
  Confidence: band=high; score=0.81
- Line 83: severity=HIGH; category=posix_only_api
  Description: POSIX-only API pipe( without platform guard
  Remediation: Wrap in #ifndef _WIN32 ... #endif or provide Windows alternative
  Context: if (pipe(pipefd) != 0) {
- Line 84: severity=HIGH; category=posix_only_api
  Description: POSIX-only API pipe( without platform guard
  Remediation: Wrap in #ifndef _WIN32 ... #endif or provide Windows alternative
  Context: spdlog::error("verifyGPGSignature: pipe() failed for: {}", modulePath);
- Line 206: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Use <cstdint> types (uint32_t, uint64_t) and sizeof() checks, not constants
  Context: while (remaining >= sizeof(Elf64_Nhdr)) {
- Line 260: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: auto processELFSectionsForMetadata = [&]<typename Ehdr, typename Shdr>(
- Line 92: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: posix_spawn_file_actions_addclose(&actions, pipefd[0]);
- Line 97: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: close(pipefd[1]);  // parent closes the write end
- Line 100: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: close(pipefd[0]);
- Line 114: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: close(pipefd[0]);
- Line 308: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: if (!metadata.empty()) metadata += "; ";
  Confidence: band=high; score=0.74
- Line 308: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: if (!metadata.empty()) metadata += "; ";
  Confidence: band=high; score=0.74

### src/themis/module_signature_verifier.cpp
Total findings: 9

- Line 249: severity=CRITICAL; category=no_timeout
  Description: file_io without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: while ((n = read(pipefd[0], buf, sizeof(buf) - 1)) > 0) {
- Line 0: severity=HIGH; category=uncategorized
  Context: ['                    signer->pChainContext->rgpChain[0]->rgpElement[0]->pCertContext;', '                if (cert) {', '                    char nameBuf[kCertNameBufSize] = {};', '                    CertGetNameStringA(cert, CERT_NAME_SIMPLE_DISPLAY_TYPE, 0,', '                                       nullptr, nameBuf, kCertNameBufSize);']
  Confidence: band=high; score=0.78
- Line 220: severity=HIGH; category=posix_only_api
  Description: POSIX-only API pipe( without platform guard
  Remediation: Wrap in #ifndef _WIN32 ... #endif or provide Windows alternative
  Context: if (pipe(pipefd) != 0) {
- Line 222: severity=HIGH; category=posix_only_api
  Description: POSIX-only API pipe( without platform guard
  Remediation: Wrap in #ifndef _WIN32 ... #endif or provide Windows alternative
  Context: "pipe() failed for '{}'", modulePath);
- Line 165: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: status, modulePath);
- Line 231: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: posix_spawn_file_actions_addclose(&actions, pipefd[0]);
- Line 236: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: close(pipefd[1]); // parent closes the write end
- Line 239: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: close(pipefd[0]);
- Line 253: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: close(pipefd[0]);

### include/themis/network/wire_protocol_v2.hpp
Total findings: 6

- Line 161: severity=CRITICAL; category=missing_dtor
  Description: Class V2ConnectionConfig allocates resources but has no destructor
  Remediation: Add explicit destructor: ~V2ConnectionConfig() { /* cleanup */ }
  Context: class/struct V2ConnectionConfig
- Line 180: severity=CRITICAL; category=missing_dtor
  Description: Class V2Server allocates resources but has no destructor
  Remediation: Add explicit destructor: ~V2Server() { /* cleanup */ }
  Context: class/struct V2Server
- Line 285: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: *   server.set_data_handler([](uint32_t sid, const auto& payload, bool eos) {
- Line 25: severity=MEDIUM; category=unportable_pragma
  Description: Non-standard pragma may not be supported on all platforms
  Remediation: Use standard C++ mechanisms: static_assert, or move to portable config
  Context: #pragma once
- Line 84: severity=MEDIUM; category=unportable_pragma
  Description: Non-standard pragma may not be supported on all platforms
  Remediation: Use standard C++ mechanisms: static_assert, or move to portable config
  Context: #pragma pack(push, 1)
- Line 105: severity=MEDIUM; category=unportable_pragma
  Description: Non-standard pragma may not be supported on all platforms
  Remediation: Use standard C++ mechanisms: static_assert, or move to portable config
  Context: #pragma pack(pop)

### src/themis/module_hash_verifier.cpp
Total findings: 6

- Line 226: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: const auto it = manifest_.find(moduleName);
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 50: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: EVP_MD_CTX_free(ctx);
- Line 59: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: EVP_MD_CTX_free(ctx);
- Line 68: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: EVP_MD_CTX_free(ctx);
- Line 72: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: EVP_MD_CTX_free(ctx);

### src/themis/edition_manager.cpp
Total findings: 4

- Line 195: severity=HIGH; category=deadlock_risk
  Description: Multiple locks acquired in nested scope — potential deadlock
  Remediation: Use std::scoped_lock(m1, m2) or enforce consistent lock ordering
  Context: std::lock_guard<std::mutex> lock(overrides_mutex_);
- Line 200: severity=HIGH; category=deadlock_risk
  Description: Multiple locks acquired in nested scope — potential deadlock
  Remediation: Use std::scoped_lock(m1, m2) or enforce consistent lock ordering
  Context: std::lock_guard<std::mutex> lock(overrides_mutex_);
- Line 143: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.emplace_back(feat);
  Confidence: band=high; score=0.74
- Line 153: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.emplace_back(feat);
  Confidence: band=high; score=0.74

### src/themis/module_loader_win32.cpp
Total findings: 4

- Line 0: severity=HIGH; category=uncategorized
  Context: ['    }', '', '    char buffer[kZoneIdBufferSize] = {};', '    DWORD bytesRead = 0;', '    ReadFile(hFile, buffer, kZoneIdBufferSize - 1, &bytesRead, nullptr);']
  Confidence: band=high; score=0.81
- Line 0: severity=HIGH; category=uncategorized
  Context: ['                    signer->pChainContext->rgpChain[0]->rgpElement[0]->pCertContext;', '                if (cert) {', '                    char nameBuffer[kCertNameBufferSize] = {};', '                    CertGetNameStringA(cert, CERT_NAME_SIMPLE_DISPLAY_TYPE, 0,', '                                       nullptr, nameBuffer, kCertNameBufferSize);']
  Confidence: band=high; score=0.78
- Line 68: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 166: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: status, modulePath);

## Update Workflow

- Refresh scan artifacts with: python tools/gap_scanner_v3.py
- Regenerate all module notes with: python tools/module_doc_generator.py . ai_working ai_working/module_gaps
- The generator mirrors each archive document directly into src/<module>/MODULE_GAPS.md.

Format: THEMIS_MODULE_GAPS_V3
