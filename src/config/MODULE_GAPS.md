# config Module - Developer Gap Note

> Auto-generated from ai_working/gap_scan_v3_aggregate.json.
> This file is overwritten on each regeneration.

## Scan Snapshot

- Module: config
- Generated: 2026-06-02 12:40:50
- Status: Critical Findings Present
- Total Findings: 155
- Actionable Findings (Critical + High): 113
- Affected Files: 6

## Severity Summary

| Severity | Count |
|---|---:|
| Critical | 14 |
| High | 99 |
| Medium | 42 |
| Low | 0 |

## Category Summary

| Category | Count |
|---|---:|
| exception_safety | 45 |
| reliability | 31 |
| raii | 26 |
| legacy_duplication | 12 |
| performance_patterns | 11 |
| container | 8 |
| platform | 8 |
| performance | 6 |
| audit_logging | 5 |
| security | 5 |
| concurrency | 3 |
| determinism | 3 |
| input_validation | 2 |
| uninitialized | 2 |

## File Overview

| File | Findings | Critical | High | Medium | Low |
|---|---:|---:|---:|---:|---:|
| src/config/config_encrypted_store.cpp | 42 | 2 | 33 | 7 | 0 |
| src/config/config_file_watcher.cpp | 39 | 7 | 15 | 17 | 0 |
| src/config/config_path_resolver.cpp | 35 | 1 | 24 | 10 | 0 |
| src/config/config_metrics_exporter.cpp | 22 | 4 | 18 | 0 | 0 |
| src/config/config_schema_validator.cpp | 15 | 0 | 7 | 8 | 0 |
| src/config/config_audit_log.cpp | 2 | 0 | 2 | 0 | 0 |

## Full Scanner Findings

### src/config/config_encrypted_store.cpp
Total findings: 42

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
- Line 151: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw ConfigKeyNotFoundException(config_key);
- Line 301: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw ConfigEncryptionException(std::string("deserialize: JSON parse error: ") + ex.what());
- Line 312: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw ConfigEncryptionException("generateKey: RAND_bytes failed");
- Line 320: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw ConfigEncryptionException("generateIV: RAND_bytes failed");
- Line 332: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw ConfigEncryptionException("aesGcmEncrypt: EVP_CIPHER_CTX_new failed");
- Line 343: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw ConfigEncryptionException("aesGcmEncrypt: EVP_EncryptInit_ex (cipher) failed");
- Line 346: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw ConfigEncryptionException("aesGcmEncrypt: set IV length failed");
- Line 349: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw ConfigEncryptionException("aesGcmEncrypt: EVP_EncryptInit_ex (key/iv) failed");
- Line 358: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw ConfigEncryptionException("aesGcmEncrypt: EVP_EncryptUpdate failed");
- Line 363: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw ConfigEncryptionException("aesGcmEncrypt: EVP_EncryptFinal_ex failed");
- Line 368: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw ConfigEncryptionException("aesGcmEncrypt: get GCM tag failed");
- Line 377: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw ConfigEncryptionException("aesGcmDecrypt: IV must be 12 bytes");
- Line 380: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw ConfigEncryptionException("aesGcmDecrypt: tag must be 16 bytes");
- Line 385: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw ConfigEncryptionException("aesGcmDecrypt: EVP_CIPHER_CTX_new failed");
- Line 396: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw ConfigEncryptionException("aesGcmDecrypt: EVP_DecryptInit_ex (cipher) failed");
- Line 399: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw ConfigEncryptionException("aesGcmDecrypt: set IV length failed");
- Line 402: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw ConfigEncryptionException("aesGcmDecrypt: EVP_DecryptInit_ex (key/iv) failed");
- Line 409: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw ConfigEncryptionException("aesGcmDecrypt: EVP_DecryptUpdate failed");
- Line 416: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw ConfigEncryptionException("aesGcmDecrypt: set GCM tag failed");
- Line 422: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw ConfigEncryptionException(
- Line 442: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw ConfigEncryptionException("decryptBlob: blob key version " + std::to_string(blob.key_version)
- Line 0: severity=MEDIUM; category=uncategorized
  Confidence: band=medium; score=0.57
- Line 0: severity=MEDIUM; category=uncategorized
  Confidence: band=medium; score=0.57
- Line 46: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: out += have2 ? kB64Chars[(triple >> 6) & 0x3F] : '=';
  Confidence: band=high; score=0.74
- Line 85: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: out.push_back(static_cast<uint8_t>((a << 2) | (b >> 4)));
  Confidence: band=high; score=0.74
- Line 191: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(kv.first);
  Confidence: band=high; score=0.74
- Line 222: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, ConfigEncryptedBlob> new_store;
  Confidence: band=medium; score=0.66
- Line 287: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, ConfigEncryptedBlob> new_store;
  Confidence: band=medium; score=0.66

### src/config/config_file_watcher.cpp
Total findings: 39

- Line 233: severity=CRITICAL; category=no_timeout
  Description: file_io without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: (void)write(pipe_write_fd_, &dummy, 1);
- Line 238: severity=CRITICAL; category=no_timeout
  Description: file_io without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: (void)write(pipe_write_fd_, &dummy, 1);
- Line 411: severity=CRITICAL; category=no_timeout
  Description: file_io without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: ssize_t len = read(ifd, buf, kBufSize);
- Line 417: severity=CRITICAL; category=missing_dtor
  Description: Class inotify_event allocates resources but has no destructor
  Remediation: Add explicit destructor: ~inotify_event() { /* cleanup */ }
  Context: class/struct inotify_event
- Line 418: severity=CRITICAL; category=missing_dtor
  Description: Class inotify_event allocates resources but has no destructor
  Remediation: Add explicit destructor: ~inotify_event() { /* cleanup */ }
  Context: class/struct inotify_event
- Line 464: severity=CRITICAL; category=no_timeout
  Description: file_io without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: int fd = ::open(path.c_str(), O_RDONLY | O_EVTONLY | O_CLOEXEC);
- Line 466: severity=CRITICAL; category=no_timeout
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
- Line 75: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: if (running_.load(std::memory_order_acquire)) {
- Line 99: severity=HIGH; category=posix_only_api
  Description: POSIX-only API pipe( without platform guard
  Remediation: Wrap in #ifndef _WIN32 ... #endif or provide Windows alternative
  Context: if (pipe(fds) != 0) {
- Line 357: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: while (running_.load(std::memory_order_acquire)) {
- Line 362: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Acquire lock before loop or redesign to minimize lock time
  Context: std::lock_guard<std::mutex> lk(debounce_mutex_);
- Line 502: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: while (running_.load(std::memory_order_acquire)) {
- Line 507: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Acquire lock before loop or redesign to minimize lock time
  Context: std::lock_guard<std::mutex> lk(debounce_mutex_);
- Line 567: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: for (auto &entry : std::filesystem::directory_iterator(path)) {
- Line 652: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: while (running_.load(std::memory_order_acquire)) {
- Line 656: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Acquire lock before loop or redesign to minimize lock time
  Context: std::lock_guard<std::mutex> lk(debounce_mutex_);
- Line 712: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Use <cstdint> types (uint32_t, uint64_t) and sizeof() checks, not constants
  Context: WideCharToMultiByte(CP_UTF8, 0, info->FileName, static_cast<int>(info->FileNameLength / sizeof(WCHAR
- Line 134: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: ::close(pipe_write_fd_);
- Line 138: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: ::close(pipe_read_fd_);
- Line 142: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: ::close(kqueue_fd_);
- Line 167: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: ::close(pipe_write_fd_);
- Line 171: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: ::close(pipe_read_fd_);
- Line 175: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: ::close(kqueue_fd_);
- Line 200: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: ::close(pipe_write_fd_);
- Line 204: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: ::close(pipe_read_fd_);
- Line 208: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: ::close(kqueue_fd_);
- Line 253: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: ::close(pipe_write_fd_);
- Line 257: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: ::close(pipe_read_fd_);
- Line 262: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: ::close(pipe_write_fd_);
- Line 266: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: ::close(pipe_read_fd_);
- Line 270: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: ::close(kqueue_fd_);
- Line 442: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: ::close(ifd);
- Line 474: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: ::close(fd);
- Line 591: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: ::close(fd);

### src/config/config_path_resolver.cpp
Total findings: 35

- Line 1337: severity=CRITICAL; category=smart_ptr_misuse
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
- Line 130: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Acquire lock before loop or redesign to minimize lock time
  Context: std::unique_lock<std::mutex> lock(cv_mutex_);
- Line 314: severity=HIGH; category=legacy_duplication; pattern=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Context: // Deprecated/Backup Files
  Confidence: band=high; score=0.8
- Line 694: severity=HIGH; category=legacy_duplication; pattern=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Context: // ── Deprecated/Backup Files ───────────────────────────────────────────────
  Confidence: band=high; score=0.8
- Line 1278: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw ConfigNotFoundException(legacy_path, attempted_paths);
- Line 1327: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: spdlog::debug("ConfigPathResolver: Using env overlay path [{}]: {} -> {}",
- Line 1491: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw InvalidPathException(path, "path traversal not allowed");
- Line 1496: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw InvalidPathException(path, "null bytes not allowed in path");
- Line 1511: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw InvalidPathException(path, "absolute path outside config directory");
- Line 1558: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw InvalidPathException(path, "symlink escapes config root");
- Line 75: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: report.push_back(std::move(entry));
  Confidence: band=high; score=0.74
- Line 1270: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: attempted_paths.push_back("config/" + envToString(env) + "/" + relative_part);
- Line 1414: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: const std::map<std::string, std::string>& ConfigPathResolver::legacyPathMappings() {
  Confidence: band=high; score=0.74
- Line 1473: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: std::replace(normalized.begin(), normalized.end(), '\\', '/');
- Line 1473: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: std::replace(normalized.begin(), normalized.end(), '\\', '/');
- Line 1503: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: std::replace(str.begin(), str.end(), '\\', '/');
- Line 1503: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: std::replace(str.begin(), str.end(), '\\', '/');
- Line 1608: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: categories.push_back(entry.first);
  Confidence: band=high; score=0.74
- Line 1694: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: if (c == '/' || c == '\\' || c == '$' || c == '`' || c == ';' ||
- Line 1694: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: if (c == '/' || c == '\\' || c == '$' || c == '`' || c == ';' ||

### src/config/config_metrics_exporter.cpp
Total findings: 22

- Line 29: severity=CRITICAL; category=missing_dtor
  Description: Class RegisteredMetrics allocates resources but has no destructor
  Remediation: Add explicit destructor: ~RegisteredMetrics() { /* cleanup */ }
  Context: class/struct RegisteredMetrics
- Line 51: severity=CRITICAL; category=missing_dtor
  Description: Class CounterSnapshot allocates resources but has no destructor
  Remediation: Add explicit destructor: ~CounterSnapshot() { /* cleanup */ }
  Context: class/struct CounterSnapshot
- Line 160: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: const auto collected = g_registry->Collect();
- Line 160: severity=CRITICAL; category=data_race
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
- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: * PR History (last 5): #4195 Correct retry attempt stati... (2026-03-14) | #3058 build/docs(config):
- Line 196: severity=HIGH; category=legacy_duplication; pattern=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Context: // New-path hits (backward compatibility)
  Confidence: band=high; score=0.8
- Line 208: severity=HIGH; category=legacy_duplication; pattern=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Context: // Cache hits/misses (backward compatibility)
  Confidence: band=high; score=0.8
- Line 332: severity=HIGH; category=legacy_duplication; pattern=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Context: // Gauges use *_current naming; *_aggregate variants provide counter-like totals, while *_total aliases are preserved for backward compatibility (non-counter gauges; planned deprecation in v1.9.0).
  Confidence: band=high; score=0.8
- Line 355: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: for (const auto& [cat, count] : ConfigPathResolver::legacyFallbacksByCategory()) {
- Line 391: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: for (const auto& category : ConfigPathResolver::legacyFallbackCategories()) {

### src/config/config_schema_validator.cpp
Total findings: 15

- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 293: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: if (node->is_object()) {
- Line 294: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: auto it = node->find(key);
- Line 295: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: if (it == node->end()) {
- Line 299: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: } else if (node->is_array()) {
- Line 307: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: if (idx >= node->size()) {
- Line 386: severity=HIGH; category=determinism; pattern=fp_exact_comparison
  Description: Floating-point exact comparison (use tolerance/epsilon)
  Context: if (v == ref) {
  Confidence: band=very_high; score=0.9
- Line 73: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: arr.push_back(yamlNodeToJsonImpl(child));
  Confidence: band=high; score=0.74
- Line 74: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: arr.push_back(yamlNodeToJsonImpl(child));
- Line 279: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: key += '/';
  Confidence: band=high; score=0.74
- Line 280: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: key += '/';
- Line 283: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: key += '~';
- Line 403: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: visited_refs.push_back(ref);
  Confidence: band=high; score=0.74
- Line 529: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: known_keys.push_back(k);
  Confidence: band=high; score=0.74
- Line 529: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: known_keys.push_back(k);
  Confidence: band=high; score=0.74

### src/config/config_audit_log.cpp
Total findings: 2

- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: * PR History (last 5): #4286 fix(config): Config Audit T... (2026-03-16) | #3026 [config] Wire confi
- Line 42: severity=HIGH; category=memory_order
  Description: memory_order_relaxed used — potential visibility issue
  Remediation: Use memory_order_acquire/release unless truly lock-free
  Context: if (!enabled_.load(std::memory_order_relaxed)) {

## Update Workflow

- Refresh scan artifacts with: python tools/gap_scanner_v3.py
- Regenerate all module notes with: python tools/module_doc_generator.py . ai_working ai_working/module_gaps
- The generator mirrors each archive document directly into src/<module>/MODULE_GAPS.md.

Format: THEMIS_MODULE_GAPS_V3
