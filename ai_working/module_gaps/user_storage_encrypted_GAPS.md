# user_storage_encrypted Module - Developer Gap Note

> Auto-generated from ai_working/gap_scan_v3_aggregate.json.
> This file is overwritten on each regeneration.

## Scan Snapshot

- Module: user_storage_encrypted
- Generated: 2026-06-02 11:55:48
- Status: Critical Findings Present
- Total Findings: 129
- Actionable Findings (Critical + High): 42
- Affected Files: 12

## Severity Summary

| Severity | Count |
|---|---:|
| Critical | 12 |
| High | 30 |
| Medium | 87 |
| Low | 0 |

## Category Summary

| Category | Count |
|---|---:|
| raii | 39 |
| reliability | 22 |
| performance_patterns | 15 |
| exception_safety | 14 |
| platform | 13 |
| container | 11 |
| memory | 8 |
| performance | 4 |
| audit_logging | 3 |
| observability | 2 |
| uninitialized | 1 |

## File Overview

| File | Findings | Critical | High | Medium | Low |
|---|---:|---:|---:|---:|---:|
| src/user_storage_encrypted/gocryptfs_backend.cpp | 50 | 5 | 6 | 39 | 0 |
| src/user_storage_encrypted/multi_level_storage.cpp | 48 | 3 | 20 | 25 | 0 |
| src/user_storage_encrypted/key_derivation_service.cpp | 9 | 2 | 2 | 5 | 0 |
| src/user_storage_encrypted/key_rotation_scheduler.cpp | 7 | 1 | 1 | 5 | 0 |
| include/user_storage_encrypted/gocryptfs_backend.hpp | 3 | 1 | 1 | 1 | 0 |
| include/user_storage_encrypted/irotation_store.hpp | 3 | 0 | 0 | 3 | 0 |
| include/user_storage_encrypted/user_models.hpp | 3 | 0 | 0 | 3 | 0 |
| include/user_storage_encrypted/encryption_backend_interface.hpp | 2 | 0 | 0 | 2 | 0 |
| include/user_storage_encrypted/key_derivation_service.hpp | 1 | 0 | 0 | 1 | 0 |
| include/user_storage_encrypted/key_rotation_scheduler.hpp | 1 | 0 | 0 | 1 | 0 |
| include/user_storage_encrypted/multi_level_storage.hpp | 1 | 0 | 0 | 1 | 0 |
| include/user_storage_encrypted/security_level.hpp | 1 | 0 | 0 | 1 | 0 |

## Full Scanner Findings

### src/user_storage_encrypted/gocryptfs_backend.cpp
Total findings: 50

- Line 243: severity=CRITICAL; category=no_timeout
  Description: file_io without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: ssize_t n = write(write_fd, ptr + written, static_cast<size_t>(total - written));
- Line 333: severity=CRITICAL; category=no_timeout
  Description: file_io without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: while ((bytes_read = read(stdout_pipe[0], buffer, sizeof(buffer))) > 0) {
- Line 493: severity=CRITICAL; category=no_timeout
  Description: file_io without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: ssize_t written = write(stdin_pipe[1], ptr, remaining);
- Line 510: severity=CRITICAL; category=no_timeout
  Description: file_io without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: while ((bytes_read = read(stdout_pipe[0], buffer, sizeof(buffer))) > 0) {
- Line 598: severity=CRITICAL; category=no_timeout
  Description: file_io without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: while ((bytes_read = read(pipe_fd[0], buffer, sizeof(buffer))) > 0) {
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 508: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: char buffer[1024];
- Line 510: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: while ((bytes_read = read(stdout_pipe[0], buffer, sizeof(buffer))) > 0) {
- Line 585: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: execvp(c_args[0], c_args.data());
- Line 596: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: char buffer[1024];
- Line 598: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: while ((bytes_read = read(pipe_fd[0], buffer, sizeof(buffer))) > 0) {
- Line 233: severity=MEDIUM; category=expensive_inner_op
  Description: I/O operation in inner loop — very expensive
  Remediation: Buffer output or move I/O outside loop
  Context: snprintf(buf, sizeof(buf), "%02x", static_cast<unsigned>(byte));
- Line 235: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: hex_key += '\n';
  Confidence: band=high; score=0.74
- Line 281: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: close(stdout_pipe[0]);
- Line 282: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: close(stdout_pipe[1]);
- Line 288: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: close(stdout_pipe[0]); close(stdout_pipe[1]);
- Line 289: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: close(stdin_pipe[0]);  close(stdin_pipe[1]);
- Line 295: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: close(stdin_pipe[1]);   // close write end of stdin pipe in child
- Line 296: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: close(stdout_pipe[0]);  // close read end of stdout pipe in child
- Line 302: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: close(stdin_pipe[0]);
- Line 303: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: close(stdout_pipe[1]);
- Line 307: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: c_args.push_back(const_cast<char*>(arg.c_str()));
  Confidence: band=high; score=0.74
- Line 317: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: close(stdin_pipe[0]);   // read end belongs to child
- Line 318: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: close(stdout_pipe[1]);  // write end belongs to child
- Line 323: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: close(stdin_pipe[1]);  // Signal EOF so gocryptfs sees end-of-passphrase.
- Line 336: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: close(stdout_pipe[0]);
- Line 450: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: close(stdout_pipe[0]);
- Line 451: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: close(stdout_pipe[1]);
- Line 457: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: close(stdout_pipe[0]); close(stdout_pipe[1]);
- Line 458: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: close(stdin_pipe[0]);  close(stdin_pipe[1]);
- Line 464: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: close(stdin_pipe[1]);
- Line 465: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: close(stdout_pipe[0]);
- Line 471: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: close(stdin_pipe[0]);
- Line 472: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: close(stdout_pipe[1]);
- Line 476: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: c_args.push_back(const_cast<char*>(arg.c_str()));
  Confidence: band=high; score=0.74
- Line 486: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: close(stdin_pipe[0]);
- Line 487: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: close(stdout_pipe[1]);
- Line 496: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: close(stdin_pipe[1]);
- Line 497: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: close(stdout_pipe[0]);
- Line 504: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: close(stdin_pipe[1]);  // Signal EOF to child.
- Line 513: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: close(stdout_pipe[0]);
- Line 563: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: close(pipe_fd[0]);
- Line 564: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: close(pipe_fd[1]);
- Line 570: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: close(pipe_fd[0]); // Close read end
- Line 575: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: close(pipe_fd[1]);
- Line 579: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: c_args.push_back(const_cast<char*>(arg.c_str()));
  Confidence: band=high; score=0.74
- Line 580: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: c_args.push_back(const_cast<char*>(arg.c_str()));
- Line 592: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: close(pipe_fd[1]); // Close write end
- Line 601: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: close(pipe_fd[0]);
- Line 604: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: int status;

### src/user_storage_encrypted/multi_level_storage.cpp
Total findings: 48

- Line 561: severity=CRITICAL; category=smart_ptr_misuse
  Description: Raw new without immediate wrapping in smart pointer
  Remediation: Use auto ptr = std::make_unique<T>(...);
  Context: return Result<void>::error("Key rotation failed – cannot create new container: " +
- Line 571: severity=CRITICAL; category=smart_ptr_misuse
  Description: Raw new without immediate wrapping in smart pointer
  Remediation: Use auto ptr = std::make_unique<T>(...);
  Context: return Result<void>::error("Key rotation failed – cannot mount new container: " +
- Line 602: severity=CRITICAL; category=smart_ptr_misuse
  Description: Raw new without immediate wrapping in smart pointer
  Remediation: Use auto ptr = std::make_unique<T>(...);
  Context: return Result<void>::error("Key rotation failed – cannot rename new container into place");
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
- Line 164: severity=HIGH; category=posix_only_api
  Description: POSIX-only API fork( without platform guard
  Remediation: Wrap in #ifndef _WIN32 ... #endif or provide Windows alternative
  Context: pid_t pid = fork();
- Line 176: severity=HIGH; category=posix_only_api
  Description: POSIX-only API fork( without platform guard
  Remediation: Wrap in #ifndef _WIN32 ... #endif or provide Windows alternative
  Context: pid_t pid = fork();
- Line 847: severity=HIGH; category=delete_no_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: After delete: user = nullptr;
  Context: return Result<void>::error("Failed to delete user file");
- Line 866: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: for (const auto& entry : std::filesystem::directory_iterator(users_dir, ec)) {
- Line 914: severity=HIGH; category=delete_no_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: After delete: group = nullptr;
  Context: return Result<void>::error("Failed to delete group file");
- Line 933: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: for (const auto& entry : std::filesystem::directory_iterator(groups_dir, ec)) {
- Line 1062: severity=HIGH; category=posix_only_api
  Description: POSIX-only API fork( without platform guard
  Remediation: Wrap in #ifndef _WIN32 ... #endif or provide Windows alternative
  Context: pid_t pid = fork();
- Line 1082: severity=HIGH; category=posix_only_api
  Description: POSIX-only API fork( without platform guard
  Remediation: Wrap in #ifndef _WIN32 ... #endif or provide Windows alternative
  Context: pid_t pid = fork();
- Line 102: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 156: severity=MEDIUM; category=expensive_inner_op
  Description: I/O operation in inner loop — very expensive
  Remediation: Buffer output or move I/O outside loop
  Context: fprintf(stderr,
- Line 212: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto storage_config = config["multi_level_storage"];
  Confidence: band=high; score=0.74
- Line 233: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto enc = level_json["encryption"];
  Confidence: band=high; score=0.74
- Line 245: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto rot = level_json["rotation"];
  Confidence: band=high; score=0.74
- Line 655: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: out += "# HELP user_storage_mount_operations_total Total mount/unmount operations\n";
- Line 847: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: return Result<void>::error("Failed to delete user file");
- Line 875: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: users.push_back(result.value());
  Confidence: band=high; score=0.74
- Line 876: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: users.push_back(result.value());
- Line 914: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: return Result<void>::error("Failed to delete group file");
- Line 942: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: groups.push_back(result.value());
  Confidence: band=high; score=0.74
- Line 943: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: groups.push_back(result.value());
- Line 953: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: HealthStatus status;
- Line 962: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: status.errors.push_back(level_health.error());
  Confidence: band=high; score=0.74
- Line 963: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: status.errors.push_back(level_health.error());
- Line 966: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: status.errors.push_back(
- Line 982: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: HealthStatus status;
- Line 1021: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: known_mount_points.push_back(cfg.mount_point);
  Confidence: band=high; score=0.74
- Line 1022: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: known_mount_points.push_back(cfg.mount_point);
- Line 1047: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: stale_mounts.push_back(mount_point);
  Confidence: band=high; score=0.74
- Line 1065: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: c_args.push_back(const_cast<char*>(a.c_str()));
  Confidence: band=high; score=0.74
- Line 1065: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: c_args.push_back(const_cast<char*>(a.c_str()));
  Confidence: band=high; score=0.74
- Line 1066: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: c_args.push_back(const_cast<char*>(a.c_str()));
- Line 1085: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: c_args.push_back(const_cast<char*>(a.c_str()));
  Confidence: band=high; score=0.74
- Line 1086: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: c_args.push_back(const_cast<char*>(a.c_str()));

### src/user_storage_encrypted/key_derivation_service.cpp
Total findings: 9

- Line 325: severity=CRITICAL; category=no_timeout
  Description: file_io without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: int fd = open("/dev/urandom", O_RDONLY | O_CLOEXEC);
- Line 332: severity=CRITICAL; category=no_timeout
  Description: file_io without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: ssize_t n = read(fd, salt.data() + total, length - static_cast<size_t>(total));
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: * PR History (last 5): #5086 [Docs][Module] user_storage... (2026-05-13)
- Line 2: severity=MEDIUM; category=observability; pattern=missing_correlation_id
  Description: Distributed call without correlation ID
  Context: * ThemisDB | File: key_derivation_service.cpp | Version: 0.0.12 | Last Modified: 2026-05-31 12:17:24
  Confidence: band=high; score=0.74
- Line 10: severity=MEDIUM; category=observability; pattern=missing_correlation_id
  Description: Distributed call without correlation ID
  Context: #include "user_storage_encrypted/key_derivation_service.hpp"
  Confidence: band=high; score=0.74
- Line 134: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 334: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: close(fd);
- Line 339: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: close(fd);

### src/user_storage_encrypted/key_rotation_scheduler.cpp
Total findings: 7

- Line 99: severity=CRITICAL; category=no_timeout
  Description: thread_join without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: impl_->scheduler_thread.join();
- Line 206: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Acquire lock before loop or redesign to minimize lock time
  Context: std::lock_guard<std::mutex> lock(impl_->mutex);
- Line 0: severity=MEDIUM; category=uncategorized
  Context: Struct with uninitialized fields
  Confidence: band=medium; score=0.65
- Line 137: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 197: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {}
- Line 222: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 286: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {

### include/user_storage_encrypted/gocryptfs_backend.hpp
Total findings: 3

- Line 118: severity=CRITICAL; category=no_timeout
  Description: file_io without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: * @param key_hex  Hex-encoded key to write (cleared on return)
- Line 99: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: * Forks the process, executes @p args[0] via execvp, writes @p stdin_data
- Line 9: severity=MEDIUM; category=unportable_pragma
  Description: Non-standard pragma may not be supported on all platforms
  Remediation: Use standard C++ mechanisms: static_assert, or move to portable config
  Context: #pragma once

### include/user_storage_encrypted/irotation_store.hpp
Total findings: 3

- Line 9: severity=MEDIUM; category=unportable_pragma
  Description: Non-standard pragma may not be supported on all platforms
  Remediation: Use standard C++ mechanisms: static_assert, or move to portable config
  Context: #pragma once
- Line 81: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {}
- Line 149: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {

### include/user_storage_encrypted/user_models.hpp
Total findings: 3

- Line 9: severity=MEDIUM; category=unportable_pragma
  Description: Non-standard pragma may not be supported on all platforms
  Remediation: Use standard C++ mechanisms: static_assert, or move to portable config
  Context: #pragma once
- Line 61: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: bool healthy;                  // Overall health status
- Line 62: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: std::string message;           // Status message

### include/user_storage_encrypted/encryption_backend_interface.hpp
Total findings: 2

- Line 9: severity=MEDIUM; category=unportable_pragma
  Description: Non-standard pragma may not be supported on all platforms
  Remediation: Use standard C++ mechanisms: static_assert, or move to portable config
  Context: #pragma once
- Line 157: severity=MEDIUM; category=expensive_copy
  Description: Unnecessary expensive copy
  Remediation: Use const reference (const T&) or std::move if transfer is needed
  Context: virtual std::string getBackendName() const = 0;

### include/user_storage_encrypted/key_derivation_service.hpp
Total findings: 1

- Line 9: severity=MEDIUM; category=unportable_pragma
  Description: Non-standard pragma may not be supported on all platforms
  Remediation: Use standard C++ mechanisms: static_assert, or move to portable config
  Context: #pragma once

### include/user_storage_encrypted/key_rotation_scheduler.hpp
Total findings: 1

- Line 9: severity=MEDIUM; category=unportable_pragma
  Description: Non-standard pragma may not be supported on all platforms
  Remediation: Use standard C++ mechanisms: static_assert, or move to portable config
  Context: #pragma once

### include/user_storage_encrypted/multi_level_storage.hpp
Total findings: 1

- Line 9: severity=MEDIUM; category=unportable_pragma
  Description: Non-standard pragma may not be supported on all platforms
  Remediation: Use standard C++ mechanisms: static_assert, or move to portable config
  Context: #pragma once

### include/user_storage_encrypted/security_level.hpp
Total findings: 1

- Line 9: severity=MEDIUM; category=unportable_pragma
  Description: Non-standard pragma may not be supported on all platforms
  Remediation: Use standard C++ mechanisms: static_assert, or move to portable config
  Context: #pragma once

## Update Workflow

- Refresh scan artifacts with: python tools/gap_scanner_v3.py
- Regenerate all module notes with: python tools/module_doc_generator.py . ai_working ai_working/module_gaps
- The generator mirrors each archive document directly into src/<module>/MODULE_GAPS.md.

Format: THEMIS_MODULE_GAPS_V3
