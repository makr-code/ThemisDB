# config Module - Developer Gap Note

> Auto-generated from ai_working/gap_scan_v3_aggregate.json.
> This file is overwritten on each regeneration.

## Scan Snapshot

- Module: config
- Generated: 2026-05-31 08:50:11
- Status: Critical Findings Present
- Total Findings: 194
- Actionable Findings (Critical + High): 132
- Affected Files: 6

## Severity Summary

| Severity | Count |
|---|---:|
| Critical | 15 |
| High | 117 |
| Medium | 62 |
| Low | 0 |

## Category Summary

| Category | Count |
|---|---:|
| exception_safety | 45 |
| raii | 35 |
| reliability | 32 |
| container | 17 |
| performance_patterns | 14 |
| legacy_duplication | 12 |
| platform | 8 |
| performance | 6 |
| audit_logging | 5 |
| concurrency | 5 |
| memory | 4 |
| security | 4 |
| determinism | 3 |
| input_validation | 2 |
| uninitialized | 2 |

## File Overview

| File | Findings | Critical | High | Medium | Low |
|---|---:|---:|---:|---:|---:|
| src/config/config_encrypted_store.cpp | 54 | 2 | 39 | 13 | 0 |
| src/config/config_file_watcher.cpp | 50 | 7 | 19 | 24 | 0 |
| src/config/config_path_resolver.cpp | 47 | 1 | 32 | 14 | 0 |
| src/config/config_metrics_exporter.cpp | 26 | 5 | 21 | 0 | 0 |
| src/config/config_schema_validator.cpp | 14 | 0 | 4 | 10 | 0 |
| src/config/config_audit_log.cpp | 3 | 0 | 2 | 1 | 0 |

## Full Scanner Findings

### src/config/config_encrypted_store.cpp
Total findings: 54

- Line 0: severity=CRITICAL; category=uncategorized
  Confidence: band=very_high; score=0.85
- Line 0: severity=CRITICAL; category=uncategorized
  Confidence: band=very_high; score=0.85
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
- Line 39: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: const uint32_t b0     = data[i];
- Line 40: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: const uint32_t b1     = have2 ? data[i + 1] : 0u;
- Line 41: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: const uint32_t b2     = have3 ? data[i + 2] : 0u;
- Line 140: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument("ConfigEncryptedStore::set: config_key must not be empty");
- Line 150: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw ConfigKeyNotFoundException(config_key);
- Line 190: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto &kv : store_) {
  Confidence: band=very_high; score=0.9
- Line 288: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (auto it = entries.begin(); it != entries.end(); ++it) {
  Confidence: band=very_high; score=0.9
- Line 300: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw ConfigEncryptionException(std::string("deserialize: JSON parse error: ") + ex.what());
- Line 311: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw ConfigEncryptionException("generateKey: RAND_bytes failed");
- Line 319: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw ConfigEncryptionException("generateIV: RAND_bytes failed");
- Line 331: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw ConfigEncryptionException("aesGcmEncrypt: EVP_CIPHER_CTX_new failed");
- Line 342: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw ConfigEncryptionException("aesGcmEncrypt: EVP_EncryptInit_ex (cipher) failed");
- Line 345: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw ConfigEncryptionException("aesGcmEncrypt: set IV length failed");
- Line 348: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw ConfigEncryptionException("aesGcmEncrypt: EVP_EncryptInit_ex (key/iv) failed");
- Line 357: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw ConfigEncryptionException("aesGcmEncrypt: EVP_EncryptUpdate failed");
- Line 362: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw ConfigEncryptionException("aesGcmEncrypt: EVP_EncryptFinal_ex failed");
- Line 367: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw ConfigEncryptionException("aesGcmEncrypt: get GCM tag failed");
- Line 376: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw ConfigEncryptionException("aesGcmDecrypt: IV must be 12 bytes");
- Line 379: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw ConfigEncryptionException("aesGcmDecrypt: tag must be 16 bytes");
- Line 384: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw ConfigEncryptionException("aesGcmDecrypt: EVP_CIPHER_CTX_new failed");
- Line 395: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw ConfigEncryptionException("aesGcmDecrypt: EVP_DecryptInit_ex (cipher) failed");
- Line 398: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw ConfigEncryptionException("aesGcmDecrypt: set IV length failed");
- Line 401: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw ConfigEncryptionException("aesGcmDecrypt: EVP_DecryptInit_ex (key/iv) failed");
- Line 408: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw ConfigEncryptionException("aesGcmDecrypt: EVP_DecryptUpdate failed");
- Line 415: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw ConfigEncryptionException("aesGcmDecrypt: set GCM tag failed");
- Line 421: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw ConfigEncryptionException(
- Line 441: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw ConfigEncryptionException("decryptBlob: blob key version " + std::to_string(blob.key_version)
- Line 0: severity=MEDIUM; category=uncategorized
  Confidence: band=medium; score=0.57
- Line 0: severity=MEDIUM; category=uncategorized
  Confidence: band=medium; score=0.57
- Line 45: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: out += have2 ? kB64Chars[(triple >> 6) & 0x3F] : '=';
  Confidence: band=high; score=0.74
- Line 84: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: out.push_back(static_cast<uint8_t>((a << 2) | (b >> 4)));
  Confidence: band=high; score=0.74
- Line 85: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: out.push_back(static_cast<uint8_t>((a << 2) | (b >> 4)));
- Line 87: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: out.push_back(static_cast<uint8_t>(((b & 0x0F) << 4) | (c >> 2)));
- Line 90: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: out.push_back(static_cast<uint8_t>(((c & 0x03) << 6) | d));
- Line 190: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(kv.first);
  Confidence: band=high; score=0.74
- Line 191: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.push_back(kv.first);
- Line 221: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, ConfigEncryptedBlob> new_store;
  Confidence: band=medium; score=0.66
- Line 286: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, ConfigEncryptedBlob> new_store;
  Confidence: band=medium; score=0.66
- Line 337: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: EVP_CIPHER_CTX_free(p);
- Line 390: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: EVP_CIPHER_CTX_free(p);

### src/config/config_file_watcher.cpp
Total findings: 50

- Line 232: severity=CRITICAL; category=no_timeout
  Description: file_io without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: (void)write(pipe_write_fd_, &dummy, 1);
- Line 237: severity=CRITICAL; category=no_timeout
  Description: file_io without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: (void)write(pipe_write_fd_, &dummy, 1);
- Line 410: severity=CRITICAL; category=no_timeout
  Description: file_io without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: ssize_t len = read(ifd, buf, kBufSize);
- Line 416: severity=CRITICAL; category=missing_dtor
  Description: Class inotify_event allocates resources but has no destructor
  Remediation: Add explicit destructor: ~inotify_event() { /* cleanup */ }
  Context: class/struct inotify_event
- Line 417: severity=CRITICAL; category=missing_dtor
  Description: Class inotify_event allocates resources but has no destructor
  Remediation: Add explicit destructor: ~inotify_event() { /* cleanup */ }
  Context: class/struct inotify_event
- Line 463: severity=CRITICAL; category=no_timeout
  Description: file_io without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: int fd = ::open(path.c_str(), O_RDONLY | O_EVTONLY | O_CLOEXEC);
- Line 465: severity=CRITICAL; category=no_timeout
  Description: file_io without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: spdlog::debug("ConfigFileWatcher: open('{}') failed: {}", path, strerror(errno));
- Line 0: severity=HIGH; category=uncategorized
  Context: ['', '    constexpr int kBufSize = 4096;', '    alignas(struct inotify_event) char buf[kBufSize];', '', '    while (running_.load(std::memory_order_acquire)) {']
  Confidence: band=high; score=0.81
- Line 0: severity=HIGH; category=uncategorized
  Context: ['', '    constexpr DWORD kBufSize = 65536;', '    alignas(DWORD) BYTE buf[kBufSize];', '', '    HANDLE wait_handles[2] = {overlapped.hEvent, static_cast<HANDLE>(stop_event_)};']
  Confidence: band=high; score=0.81
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Context: Array declared without initialization
  Confidence: band=high; score=0.81
- Line 0: severity=HIGH; category=uncategorized
  Context: Array declared without initialization
  Confidence: band=high; score=0.81
- Line 74: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: if (running_.load(std::memory_order_acquire)) {
- Line 98: severity=HIGH; category=posix_only_api
  Description: POSIX-only API pipe( without platform guard
  Remediation: Wrap in #ifndef _WIN32 ... #endif or provide Windows alternative
  Context: if (pipe(fds) != 0) {
- Line 356: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: while (running_.load(std::memory_order_acquire)) {
- Line 361: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Acquire lock before loop or redesign to minimize lock time
  Context: std::lock_guard<std::mutex> lk(debounce_mutex_);
- Line 417: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: ptr += sizeof(struct inotify_event) + ev->len;
- Line 501: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: while (running_.load(std::memory_order_acquire)) {
- Line 506: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Acquire lock before loop or redesign to minimize lock time
  Context: std::lock_guard<std::mutex> lk(debounce_mutex_);
- Line 566: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: for (auto &entry : std::filesystem::directory_iterator(path)) {
- Line 651: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: while (running_.load(std::memory_order_acquire)) {
- Line 655: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Acquire lock before loop or redesign to minimize lock time
  Context: std::lock_guard<std::mutex> lk(debounce_mutex_);
- Line 708: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: static_cast<int>(info->FileNameLength / sizeof(WCHAR)), nullptr, 0,
- Line 708: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: static_cast<int>(info->FileNameLength / sizeof(WCHAR)), nullptr, 0,
- Line 711: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Use <cstdint> types (uint32_t, uint64_t) and sizeof() checks, not constants
  Context: WideCharToMultiByte(CP_UTF8, 0, info->FileName, static_cast<int>(info->FileNameLength / sizeof(WCHAR
- Line 722: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: ptr += info->NextEntryOffset;
- Line 99: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: ::close(kqueue_fd_);
- Line 124: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: ::close(pipe_write_fd_);
- Line 128: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: ::close(pipe_read_fd_);
- Line 133: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: ::close(pipe_write_fd_);
- Line 137: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: ::close(pipe_read_fd_);
- Line 141: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: ::close(kqueue_fd_);
- Line 157: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: ::close(pipe_write_fd_);
- Line 161: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: ::close(pipe_read_fd_);
- Line 166: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: ::close(pipe_write_fd_);
- Line 170: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: ::close(pipe_read_fd_);
- Line 174: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: ::close(kqueue_fd_);
- Line 190: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: ::close(pipe_write_fd_);
- Line 194: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: ::close(pipe_read_fd_);
- Line 199: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: ::close(pipe_write_fd_);
- Line 203: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: ::close(pipe_read_fd_);
- Line 207: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: ::close(kqueue_fd_);
- Line 252: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: ::close(pipe_write_fd_);
- Line 256: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: ::close(pipe_read_fd_);
- Line 261: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: ::close(pipe_write_fd_);
- Line 265: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: ::close(pipe_read_fd_);
- Line 269: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: ::close(kqueue_fd_);
- Line 441: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: ::close(ifd);
- Line 473: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: ::close(fd);
- Line 590: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: ::close(fd);

### src/config/config_path_resolver.cpp
Total findings: 47

- Line 1336: severity=CRITICAL; category=smart_ptr_misuse
  Description: Raw new without immediate wrapping in smart pointer
  Remediation: Use auto ptr = std::make_unique<T>(...);
  Context: spdlog::debug("ConfigPathResolver: Using new config path: {} -> {}",
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
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 30: severity=HIGH; category=legacy_duplication; pattern=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Context: // DeprecationAggregator – tracks per-path legacy usage counts
  Confidence: band=high; score=0.8
- Line 43: severity=HIGH; category=legacy_duplication; pattern=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Context: * Increment the usage counter for a legacy path.
  Confidence: band=high; score=0.8
- Line 129: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Acquire lock before loop or redesign to minimize lock time
  Context: std::unique_lock<std::mutex> lock(cv_mutex_);
- Line 190: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: /// range.  Uses fprintf(stderr) rather than spdlog because this function is
  Confidence: band=very_high; score=0.9
- Line 202: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: fprintf(stderr,
  Confidence: band=very_high; score=0.9
- Line 207: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: fprintf(stderr,
  Confidence: band=very_high; score=0.9
- Line 230: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: fprintf(stderr,
  Confidence: band=very_high; score=0.9
- Line 235: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: fprintf(stderr,
  Confidence: band=very_high; score=0.9
- Line 274: severity=HIGH; category=legacy_duplication; pattern=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Context: // Path Mapping Table: Legacy → New
  Confidence: band=high; score=0.8
- Line 313: severity=HIGH; category=legacy_duplication; pattern=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Context: // Deprecated/Backup Files
  Confidence: band=high; score=0.8
- Line 693: severity=HIGH; category=legacy_duplication; pattern=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Context: // ── Deprecated/Backup Files ───────────────────────────────────────────────
  Confidence: band=high; score=0.8
- Line 1277: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw ConfigNotFoundException(legacy_path, attempted_paths);
- Line 1326: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: spdlog::debug("ConfigPathResolver: Using env overlay path [{}]: {} -> {}",
- Line 1490: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw InvalidPathException(path, "path traversal not allowed");
- Line 1495: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw InvalidPathException(path, "null bytes not allowed in path");
- Line 1510: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw InvalidPathException(path, "absolute path outside config directory");
- Line 1557: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw InvalidPathException(path, "symlink escapes config root");
- Line 74: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: report.push_back(std::move(entry));
  Confidence: band=high; score=0.74
- Line 75: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: report.push_back(std::move(entry));
- Line 1269: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: attempted_paths.push_back("config/" + envToString(env) + "/" + relative_part);
- Line 1273: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: attempted_paths.push_back(new_path);
- Line 1275: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: attempted_paths.push_back(normalized);
- Line 1413: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: const std::map<std::string, std::string>& ConfigPathResolver::legacyPathMappings() {
  Confidence: band=high; score=0.74
- Line 1472: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: std::replace(normalized.begin(), normalized.end(), '\\', '/');
- Line 1472: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: std::replace(normalized.begin(), normalized.end(), '\\', '/');
- Line 1502: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: std::replace(str.begin(), str.end(), '\\', '/');
- Line 1502: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: std::replace(str.begin(), str.end(), '\\', '/');
- Line 1607: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: categories.push_back(entry.first);
  Confidence: band=high; score=0.74
- Line 1608: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: categories.push_back(entry.first);
- Line 1693: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: if (c == '/' || c == '\\' || c == '$' || c == '`' || c == ';' ||
- Line 1693: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: if (c == '/' || c == '\\' || c == '$' || c == '`' || c == ';' ||

### src/config/config_metrics_exporter.cpp
Total findings: 26

- Line 28: severity=CRITICAL; category=missing_dtor
  Description: Class RegisteredMetrics allocates resources but has no destructor
  Remediation: Add explicit destructor: ~RegisteredMetrics() { /* cleanup */ }
  Context: class/struct RegisteredMetrics
- Line 50: severity=CRITICAL; category=missing_dtor
  Description: Class CounterSnapshot allocates resources but has no destructor
  Remediation: Add explicit destructor: ~CounterSnapshot() { /* cleanup */ }
  Context: class/struct CounterSnapshot
- Line 143: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: const uint64_t prev = (prev_it == g_prev_snapshot.legacy_by_category.end()) ? 0 : prev_it->second;
- Line 159: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: const auto collected = g_registry->Collect();
- Line 159: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: const auto collected = g_registry->Collect();
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
- Line 182: severity=HIGH; category=legacy_duplication; pattern=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Context: // Legacy fallbacks (per category)
  Confidence: band=high; score=0.8
- Line 195: severity=HIGH; category=legacy_duplication; pattern=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Context: // New-path hits (backward compatibility)
  Confidence: band=high; score=0.8
- Line 207: severity=HIGH; category=legacy_duplication; pattern=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Context: // Cache hits/misses (backward compatibility)
  Confidence: band=high; score=0.8
- Line 311: severity=HIGH; category=legacy_duplication; pattern=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Context: //   reflect config-path resolution counters (cache hits, legacy fallbacks,
  Confidence: band=high; score=0.8
- Line 331: severity=HIGH; category=legacy_duplication; pattern=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Context: // Gauges use *_current naming; *_aggregate variants provide counter-like totals, while *_total aliases are preserved for backward compatibility (non-counter gauges; planned deprecation in v1.9.0).
  Confidence: band=high; score=0.8
- Line 353: severity=HIGH; category=legacy_duplication; pattern=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Context: // Per-category legacy fallback gauges
  Confidence: band=high; score=0.8
- Line 354: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto& [cat, count] : ConfigPathResolver::legacyFallbacksByCategory()) {
  Confidence: band=very_high; score=0.9
- Line 354: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: for (const auto& [cat, count] : ConfigPathResolver::legacyFallbacksByCategory()) {
- Line 390: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: for (const auto& category : ConfigPathResolver::legacyFallbackCategories()) {

### src/config/config_schema_validator.cpp
Total findings: 14

- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 215: severity=HIGH; category=legacy_duplication; pattern=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Context: // Resolve schema path via ConfigPathResolver so legacy-to-new mapping applies.
  Confidence: band=high; score=0.8
- Line 292: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: if (node->is_object()) {
- Line 385: severity=HIGH; category=determinism; pattern=fp_exact_comparison
  Description: Floating-point exact comparison (use tolerance/epsilon)
  Context: if (v == ref) {
  Confidence: band=very_high; score=0.9
- Line 72: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: arr.push_back(yamlNodeToJsonImpl(child));
  Confidence: band=high; score=0.74
- Line 73: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: arr.push_back(yamlNodeToJsonImpl(child));
- Line 278: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: key += '/';
  Confidence: band=high; score=0.74
- Line 279: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: key += '/';
- Line 282: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: key += '~';
- Line 402: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: visited_refs.push_back(ref);
  Confidence: band=high; score=0.74
- Line 403: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: visited_refs.push_back(ref);
- Line 528: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: known_keys.push_back(k);
  Confidence: band=high; score=0.74
- Line 528: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: known_keys.push_back(k);
  Confidence: band=high; score=0.74
- Line 529: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: known_keys.push_back(k);

### src/config/config_audit_log.cpp
Total findings: 3

- Line 41: severity=HIGH; category=memory_order
  Description: memory_order_relaxed used — potential visibility issue
  Remediation: Use memory_order_acquire/release unless truly lock-free
  Context: if (!enabled_.load(std::memory_order_relaxed)) {
- Line 52: severity=HIGH; category=deadlock_risk
  Description: Multiple locks acquired in nested scope — potential deadlock
  Remediation: Use std::scoped_lock(m1, m2) or enforce consistent lock ordering
  Context: std::lock_guard<std::mutex> lock(mutex_);
- Line 45: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: entries_.push_back(std::move(entry));

## Update Workflow

- Refresh scan artifacts with: python tools/gap_scanner_v3.py
- Regenerate all module notes with: python tools/module_doc_generator.py . ai_working ai_working/module_gaps
- The generator mirrors each archive document directly into src/<module>/MODULE_GAPS.md.

Format: THEMIS_MODULE_GAPS_V3
