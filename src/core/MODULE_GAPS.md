# core Module - Developer Gap Note

> Auto-generated from ai_working/gap_scan_v3_aggregate.json.
> This file is overwritten on each regeneration.

## Scan Snapshot

- Module: core
- Generated: 2026-05-31 08:50:11
- Status: Critical Findings Present
- Total Findings: 126
- Actionable Findings (Critical + High): 79
- Affected Files: 5

## Severity Summary

| Severity | Count |
|---|---:|
| Critical | 3 |
| High | 76 |
| Medium | 47 |
| Low | 0 |

## Category Summary

| Category | Count |
|---|---:|
| reliability | 38 |
| performance | 28 |
| performance_patterns | 21 |
| exception_safety | 8 |
| raii | 8 |
| observability | 6 |
| container | 5 |
| audit_logging | 3 |
| platform | 3 |
| concurrency | 1 |
| determinism | 1 |
| legacy_duplication | 1 |
| memory | 1 |
| security | 1 |
| uninitialized | 1 |

## File Overview

| File | Findings | Critical | High | Medium | Low |
|---|---:|---:|---:|---:|---:|
| src/core/concerns/redis_cache.cpp | 45 | 3 | 31 | 11 | 0 |
| src/core/concerns/zero_copy_logger.cpp | 37 | 0 | 5 | 26 | 6 |
| src/core/concerns/concerns_context.cpp | 20 | 0 | 20 | 0 | 0 |
| src/core/security_initialization.cpp | 14 | 0 | 14 | 0 | 0 |
| src/core/concerns/lockfree_metrics.cpp | 10 | 0 | 6 | 4 | 0 |

## Full Scanner Findings

### src/core/concerns/redis_cache.cpp
Total findings: 45

- Line 244: severity=CRITICAL; category=missing_dtor
  Description: Class addrinfo allocates resources but has no destructor
  Remediation: Add explicit destructor: ~addrinfo() { /* cleanup */ }
  Context: class/struct addrinfo
- Line 288: severity=CRITICAL; category=missing_dtor
  Description: Class timeval allocates resources but has no destructor
  Remediation: Add explicit destructor: ~timeval() { /* cleanup */ }
  Context: class/struct timeval
- Line 314: severity=CRITICAL; category=missing_dtor
  Description: Class timeval allocates resources but has no destructor
  Remediation: Add explicit destructor: ~timeval() { /* cleanup */ }
  Context: class/struct timeval
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 15: severity=HIGH; category=legacy_duplication; pattern=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Context: * POSIX sockets (Linux/macOS) with a thin Win32 compatibility shim.
  Confidence: band=high; score=0.8
- Line 135: severity=HIGH; category=no_retry_logic
  Description: socket_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: ::closesocket(static_cast<SOCKET>(fd));
- Line 254: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: for (auto *p = res; p != nullptr; p = p->ai_next) {
- Line 254: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: for (auto *p = res; p != nullptr; p = p->ai_next) {
- Line 256: severity=HIGH; category=no_retry_logic
  Description: socket_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: SOCKET s = ::socket(p->ai_family, p->ai_socktype, p->ai_protocol);
- Line 262: severity=HIGH; category=no_retry_logic
  Description: socket_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: fd = ::socket(p->ai_family, p->ai_socktype, p->ai_protocol);
- Line 272: severity=HIGH; category=no_retry_logic
  Description: socket_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: ::ioctlsocket(static_cast<SOCKET>(fd), FIONBIO, &non_blocking);
- Line 275: severity=HIGH; category=no_retry_logic
  Description: socket_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: int rv = ::connect(fd,
- Line 291: severity=HIGH; category=posix_only_api
  Description: POSIX-only API select( without platform guard
  Remediation: Wrap in #ifndef _WIN32 ... #endif or provide Windows alternative
  Context: if (::select(fd + 1, nullptr, &wfds, nullptr, &tv) > 0) {
- Line 317: severity=HIGH; category=posix_only_api
  Description: POSIX-only API select( without platform guard
  Remediation: Wrap in #ifndef _WIN32 ... #endif or provide Windows alternative
  Context: const int sel = ::select(0, nullptr, &wfds, nullptr, &tv);
- Line 362: severity=HIGH; category=no_retry_logic
  Description: socket_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: int sent = ::send(static_cast<SOCKET>(fd), buf.data() + total, static_cast<int>(buf.size() - total),
- Line 367: severity=HIGH; category=no_retry_logic
  Description: socket_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: ssize_t sent = ::send(fd, buf.data() + total, buf.size() - total, MSG_NOSIGNAL);
- Line 382: severity=HIGH; category=no_retry_logic
  Description: socket_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: int n = ::recv(static_cast<SOCKET>(fd), &ch, 1, 0);
- Line 387: severity=HIGH; category=no_retry_logic
  Description: socket_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: ssize_t n = ::recv(fd, &ch, 1, 0);
- Line 502: severity=HIGH; category=no_retry_logic
  Description: socket_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: int n = ::recv(static_cast<SOCKET>(fd), &data[received],
- Line 508: severity=HIGH; category=no_retry_logic
  Description: socket_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: ssize_t n = ::recv(fd, &data[received], static_cast<size_t>(len) - received, 0);
- Line 539: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (int i = 0; i < count; ++i) {
  Confidence: band=very_high; score=0.9
- Line 696: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (auto &nc : nodes_) {
  Confidence: band=very_high; score=0.9
- Line 706: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (auto &nc : nodes_) {
  Confidence: band=very_high; score=0.9
- Line 709: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Acquire lock before loop or redesign to minimize lock time
  Context: std::lock_guard<std::mutex> lock(nc->mutex);
- Line 783: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (auto &nc : nodes_) {
  Confidence: band=very_high; score=0.9
- Line 803: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: if (stop_.load(std::memory_order_acquire) || config_.invalidation_channel.empty() || nodes_.empty())
- Line 824: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: std::this_thread::sleep_for(std::chrono::milliseconds(kSleepSliceMs));
- Line 900: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (int i = 0; i < count; ++i) {
  Confidence: band=very_high; score=0.9
- Line 958: severity=HIGH; category=determinism; pattern=fp_exact_comparison
  Description: Floating-point exact comparison (use tolerance/epsilon)
  Context: return (total == 0.0L) ? 0.0 : static_cast<double>(static_cast<long double>(h) / total);
  Confidence: band=very_high; score=0.9
- Line 984: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (auto &nc : nodes_) {
  Confidence: band=very_high; score=0.9
- Line 985: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Acquire lock before loop or redesign to minimize lock time
  Context: std::lock_guard<std::mutex> lock(nc->mutex);
- Line 995: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (auto &nc : nodes_) {
  Confidence: band=very_high; score=0.9
- Line 1005: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (auto &nc : nodes_) {
  Confidence: band=very_high; score=0.9
- Line 1006: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Acquire lock before loop or redesign to minimize lock time
  Context: std::lock_guard<std::mutex> lock(nc->mutex);
- Line 116: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: cfg.nodes.push_back(token);
- Line 120: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: cfg.nodes.push_back("127.0.0.1:6379");
- Line 128: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: ::close(fd);
- Line 166: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: nodes_.push_back(std::move(nc));
  Confidence: band=high; score=0.74
- Line 167: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: nodes_.push_back(std::move(nc));
- Line 439: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: cmd += '$';
  Confidence: band=high; score=0.74
- Line 440: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: cmd += '$';
- Line 442: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: cmd += "\r\n";
- Line 444: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: cmd += "\r\n";
- Line 904: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: parts.push_back(std::move(elem));
  Confidence: band=high; score=0.74
- Line 905: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: parts.push_back(std::move(elem));

### src/core/concerns/zero_copy_logger.cpp
Total findings: 37

- Line 0: severity=HIGH; category=uncategorized
  Context: Array declared without initialization
  Confidence: band=high; score=0.81
- Line 150: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: std::snprintf(ms_str, sizeof(ms_str), "%03lld", static_cast<long long>(ms % 1000));
  Confidence: band=very_high; score=0.9
- Line 304: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: std::snprintf(buf, sizeof(buf), "\\u%04x", c);
  Confidence: band=very_high; score=0.9
- Line 304: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Use <cstdint> types (uint32_t, uint64_t) and sizeof() checks, not constants
  Context: std::snprintf(buf, sizeof(buf), "\\u%04x", c);
- Line 351: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: std::snprintf(ms_str, sizeof(ms_str), "%03lld", static_cast<long long>(ms % 1000));
  Confidence: band=very_high; score=0.9
- Line 162: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: buf += ",\"";
  Confidence: band=high; score=0.74
- Line 163: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: buf += ",\"";
- Line 165: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: buf += "\":\"";
- Line 167: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: buf += "[REDACTED]";
- Line 171: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: buf += '"';
- Line 176: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: buf += ' ';
  Confidence: band=high; score=0.74
- Line 177: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: buf += ' ';
- Line 179: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: buf += '=';
- Line 181: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: buf += "[REDACTED]";
- Line 286: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: out += "\\\"";
  Confidence: band=high; score=0.74
- Line 287: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: out += "\\\"";
- Line 290: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: out += "\\\\";
- Line 293: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: out += "\\n";
- Line 296: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: out += "\\r";
- Line 299: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: out += "\\t";
- Line 304: severity=MEDIUM; category=expensive_inner_op
  Description: I/O operation in inner loop — very expensive
  Remediation: Buffer output or move I/O outside loop
  Context: std::snprintf(buf, sizeof(buf), "\\u%04x", c);
- Line 352: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: buf += "{\"ts\":\"";
  Confidence: band=high; score=0.74
- Line 363: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: buf += ",\"";
  Confidence: band=high; score=0.74
- Line 364: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: buf += ",\"";
- Line 366: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: buf += "\":\"";
- Line 368: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: buf += "[REDACTED]";
- Line 372: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: buf += '"';
- Line 381: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: buf += ' ';
  Confidence: band=high; score=0.74
- Line 382: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: buf += ' ';
- Line 384: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: buf += '=';
- Line 386: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: buf += "[REDACTED]";
- Line 45: severity=LOW; category=observability; pattern=unstructured_log
  Description: Unstructured logging (use structured format)
  Context: return logger_ && logger_->should_log(toSpdlogLevel(level));
  Confidence: band=medium; score=0.6
- Line 56: severity=LOW; category=observability; pattern=unstructured_log
  Description: Unstructured logging (use structured format)
  Context: logger_->log(toSpdlogLevel(level), message);
  Confidence: band=medium; score=0.6
- Line 104: severity=LOW; category=observability; pattern=unstructured_log
  Description: Unstructured logging (use structured format)
  Context: if (!logger_->should_log(toSpdlogLevel(level))) {
  Confidence: band=medium; score=0.6
- Line 117: severity=LOW; category=observability; pattern=unstructured_log
  Description: Unstructured logging (use structured format)
  Context: logger_->log(toSpdlogLevel(level), buf);
  Confidence: band=medium; score=0.6
- Line 128: severity=LOW; category=observability; pattern=unstructured_log
  Description: Unstructured logging (use structured format)
  Context: if (!logger_->should_log(toSpdlogLevel(level))) {
  Confidence: band=medium; score=0.6
- Line 188: severity=LOW; category=observability; pattern=unstructured_log
  Description: Unstructured logging (use structured format)
  Context: logger_->log(toSpdlogLevel(level), buf);
  Confidence: band=medium; score=0.6

### src/core/concerns/concerns_context.cpp
Total findings: 20

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
- Line 48: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error("Invalid logging configuration:\n" + log_validation.formatErrors());
- Line 54: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error("Invalid tracing configuration:\n" + trace_validation.formatErrors());
- Line 60: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error("Invalid cache configuration:\n" + cache_validation.formatErrors());
- Line 70: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error("Invalid adapter configuration:\n" + adapter_validation.formatErrors());
- Line 140: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error(
- Line 321: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error(
- Line 375: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument("ConcernsContext::replaceLogger: new_logger must not be nullptr");
- Line 391: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument("ConcernsContext::replaceTracer: new_tracer must not be nullptr");
- Line 404: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument("ConcernsContext::replaceMetrics: new_metrics must not be nullptr");
- Line 417: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument("ConcernsContext::replaceCache: new_cache must not be nullptr");
- Line 430: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument("ConcernsContext::replaceSecrets: new_secrets must not be nullptr");
- Line 443: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument("ConcernsContext::replaceFeatureFlags: new_ff must not be nullptr");
- Line 456: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument("ConcernsContext::replaceAuditLog: new_audit must not be nullptr");

### src/core/security_initialization.cpp
Total findings: 14

- Line 121: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error(
- Line 130: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error(
- Line 144: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error("Key provider does not implement KeyProvider interface");
- Line 165: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error("Failed to load RBAC policy from: " + rbac_policy_file_);
- Line 174: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error("Invalid JWT configuration:\n" + validation.formatErrors());
- Line 181: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error(
- Line 203: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error("Failed to open file: " + path);
- Line 249: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error("VAULT key provider requires vault_addr and vault_token in config");
- Line 259: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error(
- Line 282: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error("HSM key provider requires library_path in config");
- Line 289: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error("HSM key provider library_path does not point to an existing file: " +
- Line 315: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error("HSM provider initialization failed");
- Line 320: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error("Failed to initialize HSM key provider: " + std::string(e.what()));
- Line 325: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error("Unknown key provider type");

### src/core/concerns/lockfree_metrics.cpp
Total findings: 10

- Line 148: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto &[k, v] : entry->labels) {
  Confidence: band=very_high; score=0.9
- Line 208: severity=HIGH; category=deadlock_risk
  Description: Multiple locks acquired in nested scope — potential deadlock
  Remediation: Use std::scoped_lock(m1, m2) or enforce consistent lock ordering
  Context: std::unique_lock<std::shared_mutex> lock(counters_mu_);
- Line 334: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: if (!slot || !slot->alive.load(std::memory_order_acquire)) {
- Line 368: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (auto &entry : snapshot) {
  Confidence: band=very_high; score=0.9
- Line 369: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: if (!entry->alive.load(std::memory_order_acquire)) {
- Line 406: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: while (running_.load(std::memory_order_acquire)) {
- Line 264: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: key += '{';
  Confidence: band=high; score=0.74
- Line 268: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: key += ',';
  Confidence: band=high; score=0.74
- Line 269: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: key += ',';
- Line 272: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: key += '=';

## Update Workflow

- Refresh scan artifacts with: python tools/gap_scanner_v3.py
- Regenerate all module notes with: python tools/module_doc_generator.py . ai_working ai_working/module_gaps
- The generator mirrors each archive document directly into src/<module>/MODULE_GAPS.md.

Format: THEMIS_MODULE_GAPS_V3
