# themis Module - Developer Gap Note

> Auto-generated from ai_working/gap_scan_v3_aggregate.json.
> This file is overwritten on each regeneration.

## Scan Snapshot

- Module: themis
- Generated: 2026-06-02 11:55:48
- Status: Critical Findings Present
- Total Findings: 179
- Actionable Findings (Critical + High): 59
- Affected Files: 12

## Severity Summary

| Severity | Count |
|---|---:|
| Critical | 11 |
| High | 48 |
| Medium | 120 |
| Low | 0 |

## Category Summary

| Category | Count |
|---|---:|
| container | 39 |
| raii | 39 |
| performance_patterns | 29 |
| reliability | 24 |
| platform | 20 |
| exception_safety | 9 |
| input_validation | 5 |
| audit_logging | 4 |
| memory | 4 |
| observability | 3 |
| legacy_duplication | 2 |
| performance | 2 |
| uninitialized | 2 |

## File Overview

| File | Findings | Critical | High | Medium | Low |
|---|---:|---:|---:|---:|---:|
| src/themis/wire_protocol_server.cpp | 41 | 1 | 13 | 27 | 0 |
| src/themis/build_info.cpp | 33 | 0 | 2 | 31 | 0 |
| src/themis/module_loader.cpp | 24 | 0 | 8 | 16 | 0 |
| src/themis/module_dependency_resolver.cpp | 20 | 0 | 1 | 19 | 0 |
| include/themis/network/wire_protocol_server.hpp | 13 | 3 | 4 | 6 | 0 |
| src/themis/license_info.cpp | 13 | 3 | 6 | 4 | 0 |
| src/themis/module_loader_linux.cpp | 10 | 1 | 5 | 4 | 0 |
| src/themis/module_signature_verifier.cpp | 7 | 1 | 3 | 3 | 0 |
| include/themis/network/wire_protocol_v2.hpp | 6 | 2 | 1 | 3 | 0 |
| src/themis/module_hash_verifier.cpp | 5 | 0 | 2 | 3 | 0 |
| src/themis/module_loader_win32.cpp | 4 | 0 | 2 | 2 | 0 |
| src/themis/edition_manager.cpp | 3 | 0 | 1 | 2 | 0 |

## Full Scanner Findings

### src/themis/wire_protocol_server.cpp
Total findings: 41

- Line 0: severity=CRITICAL; category=uncategorized
  Context: ['    const uint32_t code_be = htonl(err_code);', '    std::memcpy(payload.data(), &code_be, 4u);', '    std::memcpy(payload.data() + 4u, message.data(), message.size());', '', '    WireFrameHeader hdr{};']
  Confidence: band=very_high; score=0.93
- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: * PR History (last 5): #4696 themis docs migration: alig... (2026-04-16) | #3696 fix(network): imple
- Line 19: severity=HIGH; category=legacy_duplication; pattern=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Context: // (themis::network namespace) for backward compatibility during the v1.7.0
  Confidence: band=high; score=0.8
- Line 191: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: crc = kCrc32Table[(crc ^ data[i]) & 0xFFu] ^ (crc >> 8);
- Line 642: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: handle_geo_query(req);
- Line 650: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: handle_timeseries_query(req);
- Line 848: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: [[maybe_unused]] const std::vector<uint8_t>& data) {
- Line 1126: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: void WireProtocolSession::handle_geo_query(
- Line 1167: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: void WireProtocolSession::handle_timeseries_query(
- Line 1643: severity=HIGH; category=no_retry_logic
  Description: socket_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: void WireProtocolServer::async_accept() {
- Line 1644: severity=HIGH; category=no_retry_logic
  Description: socket_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: acceptor_.async_accept(
- Line 1653: severity=HIGH; category=no_retry_logic
  Description: socket_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: handle_accept(session, ec);
- Line 1657: severity=HIGH; category=no_retry_logic
  Description: socket_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: void WireProtocolServer::handle_accept(
- Line 1687: severity=HIGH; category=no_retry_logic
  Description: socket_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: if (should_continue) async_accept();
- Line 188: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: static uint32_t crc32Compute(const uint8_t* data, std::size_t len) {
  Confidence: band=high; score=0.74
- Line 198: severity=MEDIUM; category=endianness_assumption
  Description: Endianness conversion may have undefined behavior
  Remediation: Document endianness assumptions or use portable serialization
  Context: const uint32_t magic_be = htonl(h.magic);
- Line 204: severity=MEDIUM; category=endianness_assumption
  Description: Endianness conversion may have undefined behavior
  Remediation: Document endianness assumptions or use portable serialization
  Context: const uint32_t len_be = htonl(h.payload_length);
- Line 233: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 254: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(protoValueToJson(entry));
  Confidence: band=high; score=0.74
- Line 255: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.push_back(protoValueToJson(entry));
- Line 288: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: out += '?';
  Confidence: band=high; score=0.74
- Line 288: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: out += '?';
  Confidence: band=high; score=0.74
- Line 288: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: out += '?';
  Confidence: band=high; score=0.74
- Line 289: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: out += '?';
- Line 396: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: void WireProtocolSession::close(const std::string& /*reason*/) {
- Line 408: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: socket_.close(ec);
- Line 431: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: close();
- Line 466: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: case OpCode::OP_CLOSE:  handle_close(v1::CloseRequest{});   break;
- Line 488: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: close();
- Line 598: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: handle_cursor_close(req);
- Line 690: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: handle_close(req);
- Line 749: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: close();
- Line 765: severity=MEDIUM; category=endianness_assumption
  Description: Endianness conversion may have undefined behavior
  Remediation: Document endianness assumptions or use portable serialization
  Context: const uint32_t code_be = htonl(err_code);
- Line 1397: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: close();
- Line 1505: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: void WireProtocolSession::handle_close(const v1::CloseRequest& /*req*/) {
- Line 1506: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: close("client requested close");
- Line 1541: severity=MEDIUM; category=observability; pattern=missing_health_check
  Description: Service initialization without nearby health/status handling
  Context: void WireProtocolServer::start() {
  Confidence: band=medium; score=0.66
- Line 1575: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: sessions_to_close.push_back(kv.second);
  Confidence: band=high; score=0.74
- Line 1576: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: sessions_to_close.push_back(kv.second);
- Line 1583: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: session->close("server shutdown");
- Line 1678: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: session->close("server stopping");

### src/themis/build_info.cpp
Total findings: 33

- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: * PR History (last 5): #4215 feat(base, chimera): async ... (2026-03-15) | #3830 feat(themis): Modul
- Line 58: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: static HsmModuleStatusFn fn;
- Line 166: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: config.modules.push_back({
- Line 175: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: config.modules.push_back({
- Line 182: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: config.modules.push_back({
- Line 214: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: config.modules.push_back({
- Line 223: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: config.modules.push_back({
- Line 230: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: config.modules.push_back({
- Line 409: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: config.modules.push_back({
- Line 418: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: config.modules.push_back({
- Line 425: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: config.modules.push_back({
- Line 435: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: config.modules.push_back({
- Line 442: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: config.modules.push_back({
- Line 451: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: config.modules.push_back({
- Line 458: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: config.modules.push_back({
- Line 467: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: config.modules.push_back({
- Line 474: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: config.modules.push_back({
- Line 484: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: config.modules.push_back({
- Line 491: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: config.modules.push_back({
- Line 507: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: config.modules.push_back({
- Line 516: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: config.modules.push_back({
- Line 523: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: config.modules.push_back({
- Line 532: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: config.modules.push_back({
- Line 539: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: config.modules.push_back({
- Line 562: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: HsmModuleStatusFn fn_copy;
- Line 572: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 873: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(mod.name);
  Confidence: band=high; score=0.74
- Line 873: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(mod.name);
  Confidence: band=high; score=0.74
- Line 874: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.push_back(mod.name);
- Line 884: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(mod.name);
  Confidence: band=high; score=0.74
- Line 885: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.push_back(mod.name);
- Line 952: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: EVP_MD_CTX_free(ctx);

### src/themis/module_loader.cpp
Total findings: 24

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
- Line 986: severity=HIGH; category=legacy_duplication; pattern=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Context: // ABI compatibility
  Confidence: band=high; score=0.8
- Line 1240: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Acquire lock before loop or redesign to minimize lock time
  Context: std::unique_lock<std::mutex> lk(watchdogMutex_);
- Line 1287: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: auto it = loadedModules_.find(name);
- Line 519: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(module);
  Confidence: band=high; score=0.74
- Line 771: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: ModuleMetadata ModuleLoader::extractMetadataFromHandle(void* handle) {
  Confidence: band=high; score=0.74
- Line 940: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: quarantined.push_back(path);
  Confidence: band=high; score=0.74
- Line 1137: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: module.healthChecks.push_back(healthResult);
  Confidence: band=high; score=0.74
- Line 1273: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: snapshot.emplace_back(mod.name, mod.path);
  Confidence: band=high; score=0.74
- Line 1510: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: return sep == '/' || sep == '\\';
- Line 1510: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: return sep == '/' || sep == '\\';
- Line 1717: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: zip_close(archive);
- Line 1726: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: zip_close(archive);
- Line 1742: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: if (nameStr[0] == '/' || nameStr[0] == '\\') {
- Line 1742: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: if (nameStr[0] == '/' || nameStr[0] == '\\') {
- Line 1754: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: zip_close(archive);
- Line 1799: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: zip_fclose(zf);
- Line 1800: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: outFile.close();
- Line 1803: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: zip_close(archive);
- Line 1809: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: zip_close(archive);

### src/themis/module_dependency_resolver.cpp
Total findings: 20

- Line 208: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: auto it = modules_.find(n);
- Line 121: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: all.push_back(kv.first);
  Confidence: band=high; score=0.74
- Line 131: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: std::map<std::string, bool> visited;
  Confidence: band=high; score=0.74
- Line 136: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: precheck.missingRequired.push_back(n);
  Confidence: band=high; score=0.74
- Line 183: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: closure.push_back(kv.first);
  Confidence: band=high; score=0.74
- Line 201: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: std::map<std::string, int> inDegree;
  Confidence: band=high; score=0.74
- Line 202: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: std::map<std::string, std::vector<std::string>> dependents;
  Confidence: band=high; score=0.74
- Line 218: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.missingRequired.push_back(dep.name);
  Confidence: band=high; score=0.74
- Line 218: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.missingRequired.push_back(dep.name);
  Confidence: band=high; score=0.74
- Line 218: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.missingRequired.push_back(dep.name);
  Confidence: band=high; score=0.74
- Line 219: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.missingRequired.push_back(dep.name);
- Line 225: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.missingRequired.push_back(dep.name);
- Line 243: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.versionMismatches.push_back(oss.str());
- Line 270: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.versionMismatches.push_back(oss.str());
- Line 314: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: ready.push_back(kv.first);
  Confidence: band=high; score=0.74
- Line 314: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: ready.push_back(kv.first);
  Confidence: band=high; score=0.74
- Line 315: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: ready.push_back(kv.first);
- Line 338: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: cycleNodes.push_back(kv.first);
  Confidence: band=high; score=0.74
- Line 338: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: cycleNodes.push_back(kv.first);
  Confidence: band=high; score=0.74
- Line 339: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: cycleNodes.push_back(kv.first);

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

### src/themis/license_info.cpp
Total findings: 13

- Line 198: severity=CRITICAL; category=audit_logging; pattern=sensitive_data_logging
  Description: Potential PII/credential logging: email
  Context: oss << "  Contact Email:      " << license.contact_email << "\n";
  Confidence: band=very_high; score=0.92
- Line 410: severity=CRITICAL; category=missing_dtor
  Description: Class ifaddrs allocates resources but has no destructor
  Remediation: Add explicit destructor: ~ifaddrs() { /* cleanup */ }
  Context: class/struct ifaddrs
- Line 414: severity=CRITICAL; category=missing_dtor
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
- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: * PR History (last 5): #4518 [WIP] Update developer docu... (2026-04-12) | #4351 feat(themis): integ
- Line 418: severity=HIGH; category=no_retry_logic
  Description: socket_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: int sock = socket(AF_INET, SOCK_DGRAM, 0);
- Line 135: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: "QscQaIyIKDiREBnYUmDZXEsCg5HmYgLzGEcNdHd/IxA5vp3Qr\n" \
- Line 307: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 399: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: EVP_MD_CTX_free(ctx);
- Line 423: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: ifr.ifr_name[IFNAMSIZ - 1] = '\0'; // ensure null termination

### src/themis/module_loader_linux.cpp
Total findings: 10

- Line 108: severity=CRITICAL; category=no_timeout
  Description: file_io without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: while ((n = read(pipefd[0], buf, sizeof(buf) - 1)) > 0) {
- Line 0: severity=HIGH; category=uncategorized
  Context: Array declared without initialization
  Confidence: band=high; score=0.81
- Line 81: severity=HIGH; category=posix_only_api
  Description: POSIX-only API pipe( without platform guard
  Remediation: Wrap in #ifndef _WIN32 ... #endif or provide Windows alternative
  Context: if (pipe(pipefd) != 0) {
- Line 82: severity=HIGH; category=posix_only_api
  Description: POSIX-only API pipe( without platform guard
  Remediation: Wrap in #ifndef _WIN32 ... #endif or provide Windows alternative
  Context: spdlog::error("verifyGPGSignature: pipe() failed for: {}", modulePath);
- Line 204: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Use <cstdint> types (uint32_t, uint64_t) and sizeof() checks, not constants
  Context: while (remaining >= sizeof(Elf64_Nhdr)) {
- Line 258: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: auto processELFSectionsForMetadata = [&]<typename Ehdr, typename Shdr>(
- Line 95: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: close(pipefd[1]);  // parent closes the write end
- Line 112: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: close(pipefd[0]);
- Line 306: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: if (!metadata.empty()) metadata += "; ";
  Confidence: band=high; score=0.74
- Line 306: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: if (!metadata.empty()) metadata += "; ";
  Confidence: band=high; score=0.74

### src/themis/module_signature_verifier.cpp
Total findings: 7

- Line 247: severity=CRITICAL; category=no_timeout
  Description: file_io without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: while ((n = read(pipefd[0], buf, sizeof(buf) - 1)) > 0) {
- Line 0: severity=HIGH; category=uncategorized
  Context: ['                    signer->pChainContext->rgpChain[0]->rgpElement[0]->pCertContext;', '                if (cert) {', '                    char nameBuf[kCertNameBufSize] = {};', '                    CertGetNameStringA(cert, CERT_NAME_SIMPLE_DISPLAY_TYPE, 0,', '                                       nullptr, nameBuf, kCertNameBufSize);']
  Confidence: band=high; score=0.78
- Line 218: severity=HIGH; category=posix_only_api
  Description: POSIX-only API pipe( without platform guard
  Remediation: Wrap in #ifndef _WIN32 ... #endif or provide Windows alternative
  Context: if (pipe(pipefd) != 0) {
- Line 220: severity=HIGH; category=posix_only_api
  Description: POSIX-only API pipe( without platform guard
  Remediation: Wrap in #ifndef _WIN32 ... #endif or provide Windows alternative
  Context: "pipe() failed for '{}'", modulePath);
- Line 163: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: status, modulePath);
- Line 234: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: close(pipefd[1]); // parent closes the write end
- Line 251: severity=MEDIUM; category=manual_cleanup
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
Total findings: 5

- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: * PR History (last 5): #3646 fix(themis): complete build... (2026-03-12) | #2732 [auth] OAuth 2.0 PK
- Line 57: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: EVP_MD_CTX_free(ctx);
- Line 66: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: EVP_MD_CTX_free(ctx);
- Line 70: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: EVP_MD_CTX_free(ctx);

### src/themis/module_loader_win32.cpp
Total findings: 4

- Line 0: severity=HIGH; category=uncategorized
  Context: ['    }', '', '    char buffer[kZoneIdBufferSize] = {};', '    DWORD bytesRead = 0;', '    ReadFile(hFile, buffer, kZoneIdBufferSize - 1, &bytesRead, nullptr);']
  Confidence: band=high; score=0.81
- Line 0: severity=HIGH; category=uncategorized
  Context: ['                    signer->pChainContext->rgpChain[0]->rgpElement[0]->pCertContext;', '                if (cert) {', '                    char nameBuffer[kCertNameBufferSize] = {};', '                    CertGetNameStringA(cert, CERT_NAME_SIMPLE_DISPLAY_TYPE, 0,', '                                       nullptr, nameBuffer, kCertNameBufferSize);']
  Confidence: band=high; score=0.78
- Line 66: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 164: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: status, modulePath);

### src/themis/edition_manager.cpp
Total findings: 3

- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: * PR History (last 5): #3646 fix(themis): complete build... (2026-03-12) | #3598 feat(themis): compl
- Line 141: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.emplace_back(feat);
  Confidence: band=high; score=0.74
- Line 151: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.emplace_back(feat);
  Confidence: band=high; score=0.74

## Update Workflow

- Refresh scan artifacts with: python tools/gap_scanner_v3.py
- Regenerate all module notes with: python tools/module_doc_generator.py . ai_working ai_working/module_gaps
- The generator mirrors each archive document directly into src/<module>/MODULE_GAPS.md.

Format: THEMIS_MODULE_GAPS_V3
