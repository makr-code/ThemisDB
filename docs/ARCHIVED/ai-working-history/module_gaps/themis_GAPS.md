# themis Module - Developer Gap Note

> Auto-generated from ai_working\gap_scan_results.json.
> This file is overwritten on each regeneration.

## Scan Snapshot

- Module: themis
- Generated: 2026-06-04 08:50:22
- Status: Critical Findings Present
- Total Findings: 102
- Actionable Findings (Critical + High): 41
- Affected Files: 13

## Severity Summary

| Severity | Count |
|---|---:|
| Critical | 7 |
| High | 34 |
| Medium | 57 |
| Low | 4 |

## Category Summary

| Category | Count |
|---|---:|
| manual_cleanup | 29 |
| resource_leaked_in_exception | 9 |
| duplicate_qualified_signature | 5 |
| uninitialized_access | 5 |
| copy_overhead | 4 |
| hardcoded_path | 4 |
| posix_only_api | 4 |
| unchecked_array_index | 4 |
| endianness_assumption | 3 |
| hardcoded_output | 3 |
| map_vs_unordered_map | 3 |
| string_concat_loop | 3 |
| legacy_or_compat_path | 2 |
| missing_dtor | 2 |
| missing_latency_metric | 2 |
| module_doc_linkset_drift | 2 |
| no_timeout | 2 |
| o_n_squared | 2 |
| uninitialized_array | 2 |
| generic_catch | 1 |
| layer_dependency_violation | 1 |
| lock_contention | 1 |
| manual_cleanup_in_destructor | 1 |
| missing_health_check | 1 |
| path_traversal | 1 |
| primitive_no_volatile | 1 |
| sensitive_data_logging | 1 |
| size_assumption | 1 |
| thread_join_no_timeout | 1 |
| uncaught_exception | 1 |
| unchecked_memcpy | 1 |

## File Overview

| File | Findings | Critical | High | Medium | Low |
|---|---:|---:|---:|---:|---:|
| themis/wire_protocol_server.cpp | 27 | 1 | 5 | 21 | 0 |
| themis/module_loader.cpp | 20 | 1 | 9 | 10 | 0 |
| themis/license_info.cpp | 13 | 3 | 5 | 3 | 2 |
| themis/module_loader_linux.cpp | 8 | 1 | 4 | 3 | 0 |
| themis/build_info.cpp | 7 | 0 | 2 | 5 | 0 |
| themis/module_dependency_resolver.cpp | 6 | 0 | 1 | 5 | 0 |
| themis/module_signature_verifier.cpp | 6 | 1 | 3 | 2 | 0 |
| themis/module_hash_verifier.cpp | 5 | 0 | 2 | 3 | 0 |
| themis/module_security.cpp | 5 | 0 | 0 | 5 | 0 |
| themis/module_loader_win32.cpp | 2 | 0 | 2 | 0 | 0 |
| themis/FUTURE_ENHANCEMENTS.md | 1 | 0 | 0 | 0 | 1 |
| themis/PRODUCTION_REQUIREMENTS.md | 1 | 0 | 0 | 0 | 1 |
| themis/edition_manager.cpp | 1 | 0 | 1 | 0 | 0 |

## Full Scanner Findings

### themis/wire_protocol_server.cpp
Total findings: 27

- Line 767: severity=CRITICAL; category=unchecked_memcpy
  Description: memcpy size parameter not validated (potential buffer overflow)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::input_validation
  Context: ['    const uint32_t code_be = htonl(err_code);', '    std::memcpy(payload.data(), &code_be, 4u);', '    std::memcpy(payload.data() + 4u, message.data(), message.size());', '', '    WireFrameHeader hdr{};']
- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: * PR History (last 5): #4696 themis docs migration: alig... (2026-04-16) | #3696 fix(network): imple
- Line 19: severity=HIGH; category=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::legacy_duplication
  Context: // (themis::network namespace) for backward compatibility during the v1.7.0
- Line 24: severity=HIGH; category=layer_dependency_violation
  Description: Module 'themis' must not depend on 'query' (layer violation)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::themis_architecture_rules
  Context: #include "query/aql_runner.h"
- Line 382: severity=HIGH; category=manual_cleanup_in_destructor
  Description: Manual resource cleanup in destructor (should use RAII wrapper)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_raii
  Context: WireProtocolSession::~WireProtocolSession() {
- Line 985: severity=HIGH; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: std::snprintf(buf, sizeof(buf), "c-%s-%lld",
- Line 188: severity=MEDIUM; category=missing_latency_metric
  Description: No latency measurement for operation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: static uint32_t crc32Compute(const uint8_t* data, std::size_t len) {
- Line 198: severity=MEDIUM; category=endianness_assumption
  Description: Endianness conversion may have undefined behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: const uint32_t magic_be = htonl(h.magic);
- Line 204: severity=MEDIUM; category=endianness_assumption
  Description: Endianness conversion may have undefined behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: const uint32_t len_be = htonl(h.payload_length);
- Line 288: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: out += '?';
- Line 289: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: out += '?';
- Line 396: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: void WireProtocolSession::close(const std::string& /*reason*/) {
- Line 408: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: socket_.close(ec);
- Line 431: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: close();
- Line 466: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: case OpCode::OP_CLOSE:  handle_close(v1::CloseRequest{});   break;
- Line 488: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: close();
- Line 598: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: handle_cursor_close(req);
- Line 690: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: handle_close(req);
- Line 749: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: close();
- Line 765: severity=MEDIUM; category=endianness_assumption
  Description: Endianness conversion may have undefined behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: const uint32_t code_be = htonl(err_code);
- Line 1397: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: close();
- Line 1505: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: void WireProtocolSession::handle_close(const v1::CloseRequest& /*req*/) {
- Line 1506: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: close("client requested close");
- Line 1541: severity=MEDIUM; category=missing_health_check
  Description: Service initialization without nearby health/status handling
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: void WireProtocolServer::start() {
- Line 1583: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: session->close("server shutdown");
- Line 1678: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: session->close("server stopping");
- Line 1682: severity=MEDIUM; category=primitive_no_volatile
  Description: Primitive shared across threads without volatile
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: bool should_continue = false;

### themis/module_loader.cpp
Total findings: 20

- Line 1204: severity=CRITICAL; category=thread_join_no_timeout
  Description: Thread join/wait without timeout (blocking indefinitely)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: watchdogThread_.join();
- Line 80: severity=HIGH; category=path_traversal
  Description: path_traversal_concat_path: Path construction — validate user input
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: return dlopen(path.c_str(), RTLD_LAZY | RTLD_LOCAL);
- Line 986: severity=HIGH; category=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::legacy_duplication
  Context: // ABI compatibility
- Line 1102: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 1112: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 1240: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: std::unique_lock<std::mutex> lk(watchdogMutex_);
- Line 1287: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: auto it = loadedModules_.find(name);
- Line 1643: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 1646: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 1660: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 771: severity=MEDIUM; category=missing_latency_metric
  Description: No latency measurement for operation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: ModuleMetadata ModuleLoader::extractMetadataFromHandle(void* handle) {
- Line 1510: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: return sep == '/' || sep == '\\';
- Line 1717: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: zip_close(archive);
- Line 1726: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: zip_close(archive);
- Line 1742: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: if (nameStr[0] == '/' || nameStr[0] == '\\') {
- Line 1754: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: zip_close(archive);
- Line 1799: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: zip_fclose(zf);
- Line 1800: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: outFile.close();
- Line 1803: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: zip_close(archive);
- Line 1809: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: zip_close(archive);

### themis/license_info.cpp
Total findings: 13

- Line 198: severity=CRITICAL; category=sensitive_data_logging
  Description: Potential PII/credential logging: email
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: oss << "  Contact Email:      " << license.contact_email << "\n";
- Line 410: severity=CRITICAL; category=missing_dtor
  Description: Class ifaddrs allocates resources but has no destructor
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: class/struct ifaddrs
- Line 414: severity=CRITICAL; category=missing_dtor
  Description: Class ifaddrs allocates resources but has no destructor
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: class/struct ifaddrs
- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: * PR History (last 5): #4518 [WIP] Update developer docu... (2026-04-12) | #4351 feat(themis): integ
- Line 315: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 317: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 392: severity=HIGH; category=unchecked_array_index
  Description: Array index not validated before access
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::input_validation
  Context: ['// Compute a hex-encoded SHA-256 of the primary MAC address (or a fallback)', 'static std::string computeFingerprintHash(const std::string& raw) {', '    unsigned char digest[EVP_MAX_MD_SIZE] = {};', '    unsigned int  dlen = 0;', '    EVP_MD_CTX* ctx = EVP_MD_CTX_new();']
- Line 439: severity=HIGH; category=uninitialized_array
  Description: Array contains garbage values; use with zeros or explicit init
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::uninitialized
  Context: Array declared without initialization
- Line 135: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: "QscQaIyIKDiREBnYUmDZXEsCg5HmYgLzGEcNdHd/IxA5vp3Qr\n" \
- Line 399: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: EVP_MD_CTX_free(ctx);
- Line 423: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: ifr.ifr_name[IFNAMSIZ - 1] = '\0'; // ensure null termination
- Line 427: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: std::snprintf(buf, sizeof(buf),
- Line 445: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: std::snprintf(buf, sizeof(buf),

### themis/module_loader_linux.cpp
Total findings: 8

- Line 108: severity=CRITICAL; category=no_timeout
  Description: file_io without timeout — can block indefinitely
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: while ((n = read(pipefd[0], buf, sizeof(buf) - 1)) > 0) {
- Line 81: severity=HIGH; category=posix_only_api
  Description: POSIX-only API pipe( without platform guard
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: if (pipe(pipefd) != 0) {
- Line 82: severity=HIGH; category=posix_only_api
  Description: POSIX-only API pipe( without platform guard
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: spdlog::error("verifyGPGSignature: pipe() failed for: {}", modulePath);
- Line 183: severity=HIGH; category=uninitialized_array
  Description: Array contains garbage values; use with zeros or explicit init
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::uninitialized
  Context: Array declared without initialization
- Line 204: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: while (remaining >= sizeof(Elf64_Nhdr)) {
- Line 95: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: close(pipefd[1]);  // parent closes the write end
- Line 112: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: close(pipefd[0]);
- Line 306: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: if (!metadata.empty()) metadata += "; ";

### themis/build_info.cpp
Total findings: 7

- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: * PR History (last 5): #4215 feat(base, chimera): async ... (2026-03-15) | #3830 feat(themis): Modul
- Line 939: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 572: severity=MEDIUM; category=generic_catch
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: auto [hsm_active, bridge_desc] = fn_copy();

                is_real_hsm = hsm_active;

                desc = bridge_desc;

            } catch (...) {

                // Bridge failure → keep static defaults

            }

        }
- Line 572: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: } catch (...) {
- Line 874: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: result.push_back(mod.name);
- Line 885: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: result.push_back(mod.name);
- Line 952: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: EVP_MD_CTX_free(ctx);

### themis/module_dependency_resolver.cpp
Total findings: 6

- Line 208: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: auto it = modules_.find(n);
- Line 131: severity=MEDIUM; category=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: std::map<std::string, bool> visited;
- Line 201: severity=MEDIUM; category=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: std::map<std::string, int> inDegree;
- Line 202: severity=MEDIUM; category=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: std::map<std::string, std::vector<std::string>> dependents;
- Line 315: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: ready.push_back(kv.first);
- Line 339: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: cycleNodes.push_back(kv.first);

### themis/module_signature_verifier.cpp
Total findings: 6

- Line 247: severity=CRITICAL; category=no_timeout
  Description: file_io without timeout — can block indefinitely
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: while ((n = read(pipefd[0], buf, sizeof(buf) - 1)) > 0) {
- Line 129: severity=HIGH; category=unchecked_array_index
  Description: Array index not validated before access
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::input_validation
  Context: ['                    signer->pChainContext->rgpChain[0]->rgpElement[0]->pCertContext;', '                if (cert) {', '                    char nameBuf[kCertNameBufSize] = {};', '                    CertGetNameStringA(cert, CERT_NAME_SIMPLE_DISPLAY_TYPE, 0,', '                                       nullptr, nameBuf, kCertNameBufSize);']
- Line 218: severity=HIGH; category=posix_only_api
  Description: POSIX-only API pipe( without platform guard
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: if (pipe(pipefd) != 0) {
- Line 220: severity=HIGH; category=posix_only_api
  Description: POSIX-only API pipe( without platform guard
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: "pipe() failed for '{}'", modulePath);
- Line 234: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: close(pipefd[1]); // parent closes the write end
- Line 251: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: close(pipefd[0]);

### themis/module_hash_verifier.cpp
Total findings: 5

- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: * PR History (last 5): #3646 fix(themis): complete build... (2026-03-12) | #2732 [auth] OAuth 2.0 PK
- Line 43: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 57: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: EVP_MD_CTX_free(ctx);
- Line 66: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: EVP_MD_CTX_free(ctx);
- Line 70: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: EVP_MD_CTX_free(ctx);

### themis/module_security.cpp
Total findings: 5

- Line 111: severity=MEDIUM; category=duplicate_qualified_signature
  Description: Potential duplicate implementation signature across files: ModuleSecurityVerifier::calculateFileHash(const std::string& modulePath)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::legacy_duplication
  Context: std::string ModuleSecurityVerifier::calculateFileHash(const std::string& modulePath) {
- Line 115: severity=MEDIUM; category=duplicate_qualified_signature
  Description: Potential duplicate implementation signature across files: ModuleSecurityVerifier::setRequireSignature(bool require)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::legacy_duplication
  Context: void ModuleSecurityVerifier::setRequireSignature(bool require) {
- Line 119: severity=MEDIUM; category=duplicate_qualified_signature
  Description: Potential duplicate implementation signature across files: ModuleSecurityVerifier::setAllowUnsigned(bool allow)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::legacy_duplication
  Context: void ModuleSecurityVerifier::setAllowUnsigned(bool allow) {
- Line 123: severity=MEDIUM; category=duplicate_qualified_signature
  Description: Potential duplicate implementation signature across files: ModuleSecurityVerifier::addWhitelistedHash(const std::string& hash)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::legacy_duplication
  Context: void ModuleSecurityVerifier::addWhitelistedHash(const std::string& hash) {
- Line 127: severity=MEDIUM; category=duplicate_qualified_signature
  Description: Potential duplicate implementation signature across files: ModuleSecurityVerifier::addBlacklistedHash(const std::string& hash)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::legacy_duplication
  Context: void ModuleSecurityVerifier::addBlacklistedHash(const std::string& hash) {

### themis/module_loader_win32.cpp
Total findings: 2

- Line 52: severity=HIGH; category=unchecked_array_index
  Description: Array index not validated before access
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::input_validation
  Context: ['    }', '', '    char buffer[kZoneIdBufferSize] = {};', '    DWORD bytesRead = 0;', '    ReadFile(hFile, buffer, kZoneIdBufferSize - 1, &bytesRead, nullptr);']
- Line 133: severity=HIGH; category=unchecked_array_index
  Description: Array index not validated before access
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::input_validation
  Context: ['                    signer->pChainContext->rgpChain[0]->rgpElement[0]->pCertContext;', '                if (cert) {', '                    char nameBuffer[kCertNameBufferSize] = {};', '                    CertGetNameStringA(cert, CERT_NAME_SIMPLE_DISPLAY_TYPE, 0,', '                                       nullptr, nameBuffer, kCertNameBufferSize);']

### themis/FUTURE_ENHANCEMENTS.md
Total findings: 1

- Line 1: severity=LOW; category=module_doc_linkset_drift
  Description: Module doc 'FUTURE_ENHANCEMENTS.md' is missing expected cross-links: ARCHITECTURE.md
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::themis_doc_freshness_rules
  Context: Refresh the top-level <!-- Links: ... --> metadata to match the current module doc set

### themis/PRODUCTION_REQUIREMENTS.md
Total findings: 1

- Line 1: severity=LOW; category=module_doc_linkset_drift
  Description: Module doc 'PRODUCTION_REQUIREMENTS.md' is missing expected cross-links: FUTURE_ENHANCEMENTS.md, README.md, ROADMAP.md
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::themis_doc_freshness_rules
  Context: Refresh the top-level <!-- Links: ... --> metadata to match the current module doc set

### themis/edition_manager.cpp
Total findings: 1

- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: * PR History (last 5): #3646 fix(themis): complete build... (2026-03-12) | #3598 feat(themis): compl

## Update Workflow

- Refresh scanner artifacts with: python tools/gs3_orchestrator.py ./src --output ai_working/gap_scan_results.json
- Regenerate docs with: python tools/module_doc_generator.py . ai_working ai_working/module_gaps
- Add --no-mirror when you only want archive docs in ai_working/module_gaps.

Format: THEMIS_MODULE_GAPS_V4
