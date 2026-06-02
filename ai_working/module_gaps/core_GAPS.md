# core Module - Developer Gap Note

> Auto-generated from ai_working/gap_scan_v3_aggregate.json.
> This file is overwritten on each regeneration.

## Scan Snapshot

- Module: core
- Generated: 2026-06-02 11:55:48
- Status: Critical Findings Present
- Total Findings: 86
- Actionable Findings (Critical + High): 42
- Affected Files: 6

## Severity Summary

| Severity | Count |
|---|---:|
| Critical | 3 |
| High | 39 |
| Medium | 38 |
| Low | 0 |

## Category Summary

| Category | Count |
|---|---:|
| performance | 28 |
| performance_patterns | 14 |
| reliability | 11 |
| exception_safety | 8 |
| raii | 8 |
| observability | 6 |
| container | 5 |
| audit_logging | 3 |
| platform | 3 |
| determinism | 1 |
| legacy_duplication | 1 |
| uninitialized | 1 |

## File Overview

| File | Findings | Critical | High | Medium | Low |
|---|---:|---:|---:|---:|---:|
| src/core/concerns/redis_cache.cpp | 35 | 3 | 24 | 8 | 0 |
| src/core/concerns/zero_copy_logger.cpp | 34 | 0 | 2 | 26 | 6 |
| src/core/concerns/concerns_context.cpp | 8 | 0 | 8 | 0 | 0 |
| src/core/concerns/lockfree_metrics.cpp | 7 | 0 | 3 | 4 | 0 |
| src/core/adapters/otel_tracer.cpp | 1 | 0 | 1 | 0 | 0 |
| src/core/concerns/prometheus_metrics.cpp | 1 | 0 | 1 | 0 | 0 |

## Full Scanner Findings

### src/core/concerns/redis_cache.cpp
Total findings: 35

- Line 245: severity=CRITICAL; category=missing_dtor
  Description: Class addrinfo allocates resources but has no destructor
  Remediation: Add explicit destructor: ~addrinfo() { /* cleanup */ }
  Context: class/struct addrinfo
- Line 289: severity=CRITICAL; category=missing_dtor
  Description: Class timeval allocates resources but has no destructor
  Remediation: Add explicit destructor: ~timeval() { /* cleanup */ }
  Context: class/struct timeval
- Line 315: severity=CRITICAL; category=missing_dtor
  Description: Class timeval allocates resources but has no destructor
  Remediation: Add explicit destructor: ~timeval() { /* cleanup */ }
  Context: class/struct timeval
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 16: severity=HIGH; category=legacy_duplication; pattern=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Context: * POSIX sockets (Linux/macOS) with a thin Win32 compatibility shim.
  Confidence: band=high; score=0.8
- Line 136: severity=HIGH; category=no_retry_logic
  Description: socket_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: ::closesocket(static_cast<SOCKET>(fd));
- Line 257: severity=HIGH; category=no_retry_logic
  Description: socket_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: SOCKET s = ::socket(p->ai_family, p->ai_socktype, p->ai_protocol);
- Line 263: severity=HIGH; category=no_retry_logic
  Description: socket_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: fd = ::socket(p->ai_family, p->ai_socktype, p->ai_protocol);
- Line 273: severity=HIGH; category=no_retry_logic
  Description: socket_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: ::ioctlsocket(static_cast<SOCKET>(fd), FIONBIO, &non_blocking);
- Line 276: severity=HIGH; category=no_retry_logic
  Description: socket_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: int rv = ::connect(fd,
- Line 292: severity=HIGH; category=posix_only_api
  Description: POSIX-only API select( without platform guard
  Remediation: Wrap in #ifndef _WIN32 ... #endif or provide Windows alternative
  Context: if (::select(fd + 1, nullptr, &wfds, nullptr, &tv) > 0) {
- Line 318: severity=HIGH; category=posix_only_api
  Description: POSIX-only API select( without platform guard
  Remediation: Wrap in #ifndef _WIN32 ... #endif or provide Windows alternative
  Context: const int sel = ::select(0, nullptr, &wfds, nullptr, &tv);
- Line 363: severity=HIGH; category=no_retry_logic
  Description: socket_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: int sent = ::send(static_cast<SOCKET>(fd), buf.data() + total, static_cast<int>(buf.size() - total),
- Line 368: severity=HIGH; category=no_retry_logic
  Description: socket_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: ssize_t sent = ::send(fd, buf.data() + total, buf.size() - total, MSG_NOSIGNAL);
- Line 383: severity=HIGH; category=no_retry_logic
  Description: socket_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: int n = ::recv(static_cast<SOCKET>(fd), &ch, 1, 0);
- Line 388: severity=HIGH; category=no_retry_logic
  Description: socket_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: ssize_t n = ::recv(fd, &ch, 1, 0);
- Line 503: severity=HIGH; category=no_retry_logic
  Description: socket_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: int n = ::recv(static_cast<SOCKET>(fd), &data[received],
- Line 509: severity=HIGH; category=no_retry_logic
  Description: socket_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: ssize_t n = ::recv(fd, &data[received], static_cast<size_t>(len) - received, 0);
- Line 707: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (auto &nc : nodes_) {
  Confidence: band=very_high; score=0.9
- Line 710: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Acquire lock before loop or redesign to minimize lock time
  Context: std::lock_guard<std::mutex> lock(nc->mutex);
- Line 804: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: if (stop_.load(std::memory_order_acquire) || config_.invalidation_channel.empty() || nodes_.empty())
- Line 825: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: std::this_thread::sleep_for(std::chrono::milliseconds(kSleepSliceMs));
- Line 959: severity=HIGH; category=determinism; pattern=fp_exact_comparison
  Description: Floating-point exact comparison (use tolerance/epsilon)
  Context: return (total == 0.0L) ? 0.0 : static_cast<double>(static_cast<long double>(h) / total);
  Confidence: band=very_high; score=0.9
- Line 985: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (auto &nc : nodes_) {
  Confidence: band=very_high; score=0.9
- Line 986: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Acquire lock before loop or redesign to minimize lock time
  Context: std::lock_guard<std::mutex> lock(nc->mutex);
- Line 1006: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (auto &nc : nodes_) {
  Confidence: band=very_high; score=0.9
- Line 1007: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Acquire lock before loop or redesign to minimize lock time
  Context: std::lock_guard<std::mutex> lock(nc->mutex);
- Line 121: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: cfg.nodes.push_back("127.0.0.1:6379");
- Line 129: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: ::close(fd);
- Line 167: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: nodes_.push_back(std::move(nc));
  Confidence: band=high; score=0.74
- Line 440: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: cmd += '$';
  Confidence: band=high; score=0.74
- Line 441: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: cmd += '$';
- Line 443: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: cmd += "\r\n";
- Line 445: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: cmd += "\r\n";
- Line 905: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: parts.push_back(std::move(elem));
  Confidence: band=high; score=0.74

### src/core/concerns/zero_copy_logger.cpp
Total findings: 34

- Line 0: severity=HIGH; category=uncategorized
  Context: Array declared without initialization
  Confidence: band=high; score=0.81
- Line 305: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Use <cstdint> types (uint32_t, uint64_t) and sizeof() checks, not constants
  Context: std::snprintf(buf, sizeof(buf), "\\u%04x", c);
- Line 163: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: buf += ",\"";
  Confidence: band=high; score=0.74
- Line 164: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: buf += ",\"";
- Line 166: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: buf += "\":\"";
- Line 168: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: buf += "[REDACTED]";
- Line 172: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: buf += '"';
- Line 177: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: buf += ' ';
  Confidence: band=high; score=0.74
- Line 178: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: buf += ' ';
- Line 180: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: buf += '=';
- Line 182: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: buf += "[REDACTED]";
- Line 287: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: out += "\\\"";
  Confidence: band=high; score=0.74
- Line 288: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: out += "\\\"";
- Line 291: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: out += "\\\\";
- Line 294: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: out += "\\n";
- Line 297: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: out += "\\r";
- Line 300: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: out += "\\t";
- Line 305: severity=MEDIUM; category=expensive_inner_op
  Description: I/O operation in inner loop — very expensive
  Remediation: Buffer output or move I/O outside loop
  Context: std::snprintf(buf, sizeof(buf), "\\u%04x", c);
- Line 353: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: buf += "{\"ts\":\"";
  Confidence: band=high; score=0.74
- Line 364: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: buf += ",\"";
  Confidence: band=high; score=0.74
- Line 365: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: buf += ",\"";
- Line 367: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: buf += "\":\"";
- Line 369: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: buf += "[REDACTED]";
- Line 373: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: buf += '"';
- Line 382: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: buf += ' ';
  Confidence: band=high; score=0.74
- Line 383: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: buf += ' ';
- Line 385: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: buf += '=';
- Line 387: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: buf += "[REDACTED]";
- Line 46: severity=LOW; category=observability; pattern=unstructured_log
  Description: Unstructured logging (use structured format)
  Context: return logger_ && logger_->should_log(toSpdlogLevel(level));
  Confidence: band=medium; score=0.6
- Line 57: severity=LOW; category=observability; pattern=unstructured_log
  Description: Unstructured logging (use structured format)
  Context: logger_->log(toSpdlogLevel(level), message);
  Confidence: band=medium; score=0.6
- Line 105: severity=LOW; category=observability; pattern=unstructured_log
  Description: Unstructured logging (use structured format)
  Context: if (!logger_->should_log(toSpdlogLevel(level))) {
  Confidence: band=medium; score=0.6
- Line 118: severity=LOW; category=observability; pattern=unstructured_log
  Description: Unstructured logging (use structured format)
  Context: logger_->log(toSpdlogLevel(level), buf);
  Confidence: band=medium; score=0.6
- Line 129: severity=LOW; category=observability; pattern=unstructured_log
  Description: Unstructured logging (use structured format)
  Context: if (!logger_->should_log(toSpdlogLevel(level))) {
  Confidence: band=medium; score=0.6
- Line 189: severity=LOW; category=observability; pattern=unstructured_log
  Description: Unstructured logging (use structured format)
  Context: logger_->log(toSpdlogLevel(level), buf);
  Confidence: band=medium; score=0.6

### src/core/concerns/concerns_context.cpp
Total findings: 8

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
  Context: * PR History (last 5): #5118 docs(core): rebaseline PROD... (2026-05-13) | #4379 [WIP] Update module

### src/core/concerns/lockfree_metrics.cpp
Total findings: 7

- Line 335: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: if (!slot || !slot->alive.load(std::memory_order_acquire)) {
- Line 370: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: if (!entry->alive.load(std::memory_order_acquire)) {
- Line 407: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: while (running_.load(std::memory_order_acquire)) {
- Line 265: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: key += '{';
  Confidence: band=high; score=0.74
- Line 269: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: key += ',';
  Confidence: band=high; score=0.74
- Line 270: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: key += ',';
- Line 273: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: key += '=';

### src/core/adapters/otel_tracer.cpp
Total findings: 1

- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: * PR History (last 5): #2844 feat(core): add Prometheus ... (2026-03-12) | #2843 feat(core): impleme

### src/core/concerns/prometheus_metrics.cpp
Total findings: 1

- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: * PR History (last 5): #2844 feat(core): add Prometheus ... (2026-03-12) | #2843 feat(core): impleme

## Update Workflow

- Refresh scan artifacts with: python tools/gap_scanner_v3.py
- Regenerate all module notes with: python tools/module_doc_generator.py . ai_working ai_working/module_gaps
- The generator mirrors each archive document directly into src/<module>/MODULE_GAPS.md.

Format: THEMIS_MODULE_GAPS_V3
