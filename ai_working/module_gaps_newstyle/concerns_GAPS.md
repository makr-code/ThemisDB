# concerns Module - Developer Gap Note

> Auto-generated from ai_working\gap_scan_results.json.
> This file is overwritten on each regeneration.

## Scan Snapshot

- Module: concerns
- Generated: 2026-06-03 20:28:49
- Status: Critical Findings Present
- Total Findings: 66
- Actionable Findings (Critical + High): 46
- Affected Files: 4

## Severity Summary

| Severity | Count |
|---|---:|
| Critical | 2 |
| High | 44 |
| Medium | 14 |
| Low | 6 |

## Category Summary

| Category | Count |
|---|---:|
| uncaught_exception | 13 |
| no_retry_logic | 11 |
| string_concat_loop | 9 |
| resource_leaked_in_exception | 8 |
| unstructured_log | 6 |
| hardcoded_output | 3 |
| lock_in_loop | 3 |
| pointer_arithmetic_unbounded | 3 |
| primitive_no_volatile | 3 |
| missing_vector_reserve | 2 |
| thread_join_no_timeout | 2 |
| fp_exact_comparison | 1 |
| legacy_or_compat_path | 1 |
| uninitialized_array | 1 |

## File Overview

| File | Findings | Critical | High | Medium | Low |
|---|---:|---:|---:|---:|---:|
| concerns/redis_cache.cpp | 26 | 1 | 20 | 5 | 0 |
| concerns/concerns_context.cpp | 20 | 0 | 20 | 0 | 0 |
| concerns/zero_copy_logger.cpp | 17 | 0 | 4 | 7 | 6 |
| concerns/lockfree_metrics.cpp | 3 | 1 | 0 | 2 | 0 |

## Full Scanner Findings

### concerns/redis_cache.cpp
Total findings: 26

- Line 983: severity=CRITICAL; category=thread_join_no_timeout
  Description: Thread join/wait without timeout (blocking indefinitely)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: sub_thread_.join();
- Line 16: severity=HIGH; category=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::legacy_duplication
  Context: * POSIX sockets (Linux/macOS) with a thin Win32 compatibility shim.
- Line 136: severity=HIGH; category=no_retry_logic
  Description: RPC/network call without retry logic — transient failures will propagate
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: #else

inline void closeSocketFd(uintptr_t &fd) noexcept {

    if (fd != static_cast<uintptr_t>(~0ULL)) {

        ::closesocket(static_cast<SOCKET>(fd));

        fd = static_cast<uintptr_t>(~0ULL);

    }

}
- Line 255: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: }



    SocketFd fd = kInvalidSocket;

    for (auto *p = res; p != nullptr; p = p->ai_next) {

#if defined(_WIN32)

        SOCKET s = ::socket(p->ai_family, p->ai_socktype, p->ai_protocol);

        if (s == INVALID_SOCKET) {
- Line 257: severity=HIGH; category=no_retry_logic
  Description: RPC/network call without retry logic — transient failures will propagate
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: SocketFd fd = kInvalidSocket;

    for (auto *p = res; p != nullptr; p = p->ai_next) {

#if defined(_WIN32)

        SOCKET s = ::socket(p->ai_family, p->ai_socktype, p->ai_protocol);

        if (s == INVALID_SOCKET) {

            continue;

        }
- Line 263: severity=HIGH; category=no_retry_logic
  Description: RPC/network call without retry logic — transient failures will propagate
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: }

        fd = static_cast<SocketFd>(s);

#else

        fd = ::socket(p->ai_family, p->ai_socktype, p->ai_protocol);

        if (fd < 0)

            continue;

#endif
- Line 273: severity=HIGH; category=no_retry_logic
  Description: RPC/network call without retry logic — transient failures will propagate
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: ::fcntl(fd, F_SETFL, flags | O_NONBLOCK);

#else

        u_long non_blocking = 1;

        ::ioctlsocket(static_cast<SOCKET>(fd), FIONBIO, &non_blocking);

#endif



        int rv = ::connect(fd,
- Line 276: severity=HIGH; category=no_retry_logic
  Description: RPC/network call without retry logic — transient failures will propagate
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: ::ioctlsocket(static_cast<SOCKET>(fd), FIONBIO, &non_blocking);

#endif



        int rv = ::connect(fd,

#if defined(_WIN32)

                           p->ai_addr, static_cast<int>(p->ai_addrlen)

#else
- Line 363: severity=HIGH; category=no_retry_logic
  Description: RPC/network call without retry logic — transient failures will propagate
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: size_t total = 0;

    while (total < buf.size()) {

#if defined(_WIN32)

        int sent = ::send(static_cast<SOCKET>(fd), buf.data() + total, static_cast<int>(buf.size() - total), 0);

        if (sent == SOCKET_ERROR) {

            return false;

        }
- Line 368: severity=HIGH; category=no_retry_logic
  Description: RPC/network call without retry logic — transient failures will propagate
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: return false;

        }

#else

        ssize_t sent = ::send(fd, buf.data() + total, buf.size() - total, MSG_NOSIGNAL);

        if (sent <= 0)

            return false;

#endif
- Line 383: severity=HIGH; category=no_retry_logic
  Description: RPC/network call without retry logic — transient failures will propagate
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: char ch;

    while (true) {

#if defined(_WIN32)

        int n = ::recv(static_cast<SOCKET>(fd), &ch, 1, 0);

        if (n <= 0) {

            return false;

        }
- Line 388: severity=HIGH; category=no_retry_logic
  Description: RPC/network call without retry logic — transient failures will propagate
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: return false;

        }

#else

        ssize_t n = ::recv(fd, &ch, 1, 0);

        if (n <= 0)

            return false;

#endif
- Line 503: severity=HIGH; category=no_retry_logic
  Description: RPC/network call without retry logic — transient failures will propagate
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: size_t received = 0;

        while (received < static_cast<size_t>(len)) {

#if defined(_WIN32)

            int n = ::recv(static_cast<SOCKET>(fd), &data[received],

                           static_cast<int>(static_cast<size_t>(len) - received), 0);

            if (n <= 0) {

                return false;
- Line 503: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: size_t received = 0;

        while (received < static_cast<size_t>(len)) {

#if defined(_WIN32)

            int n = ::recv(static_cast<SOCKET>(fd), &data[received],

                           static_cast<int>(static_cast<size_t>(len) - received), 0);

            if (n <= 0) {

                return false;
- Line 509: severity=HIGH; category=no_retry_logic
  Description: RPC/network call without retry logic — transient failures will propagate
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: return false;

            }

#else

            ssize_t n = ::recv(fd, &data[received], static_cast<size_t>(len) - received, 0);

            if (n <= 0)

                return false;

#endif
- Line 509: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: return false;

            }

#else

            ssize_t n = ::recv(fd, &data[received], static_cast<size_t>(len) - received, 0);

            if (n <= 0)

                return false;

#endif
- Line 707: severity=HIGH; category=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: for (auto &nc : nodes_) {
- Line 730: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 959: severity=HIGH; category=fp_exact_comparison
  Description: Floating-point exact comparison (use tolerance/epsilon)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: return (total == 0.0L) ? 0.0 : static_cast<double>(static_cast<long double>(h) / total);
- Line 985: severity=HIGH; category=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: for (auto &nc : nodes_) {
- Line 1006: severity=HIGH; category=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: for (auto &nc : nodes_) {
- Line 167: severity=MEDIUM; category=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: nodes_.push_back(std::move(nc));
- Line 440: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: cmd += '$';
- Line 821: severity=MEDIUM; category=primitive_no_volatile
  Description: Primitive shared across threads without volatile
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: constexpr int kSleepSliceMs = 50;
- Line 822: severity=MEDIUM; category=primitive_no_volatile
  Description: Primitive shared across threads without volatile
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: for (int elapsed = 0;
- Line 905: severity=MEDIUM; category=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: parts.push_back(std::move(elem));

### concerns/concerns_context.cpp
Total findings: 20

- Line 57: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: // Validate configuration

    auto log_validation = core::ConfigValidator::validateLogConfig(config.logLevel, config.logPattern);

    if (!log_validation.valid) {

        throw std::runtime_error("Invalid logging configuration:\n" + log_validation.formatErrors());

    }

    

    auto trace_validation = core::ConfigValidator::validateTracingConfig(
- Line 63: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: auto trace_validation = core::ConfigValidator::validateTracingConfig(

        config.tracingEnabled, config.tracingEndpoint, config.tracingServiceName);

    if (!trace_validation.valid) {

        throw std::runtime_error("Invalid tracing configuration:\n" + trace_validation.formatErrors());

    }

    

    auto cache_validation = core::ConfigValidator::validateCacheConfig(
- Line 69: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: auto cache_validation = core::ConfigValidator::validateCacheConfig(

        config.cacheMaxSize, config.cacheDefaultTTL);

    if (!cache_validation.valid) {

        throw std::runtime_error("Invalid cache configuration:\n" + cache_validation.formatErrors());

    }



    auto adapter_validation = core::ConfigValidator::validateAdapterConfig(
- Line 79: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: config.auditAdapter, config.secretsAdapter,

        config.cacheRedisUrl);

    if (!adapter_validation.valid) {

        throw std::runtime_error("Invalid adapter configuration:\n" + adapter_validation.formatErrors());

    }

    

    // Initialize logger
- Line 149: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: } else {

        // "noop" — only reachable after validation passes

        if (production_mode && effective_metrics != "prometheus") {

            throw std::runtime_error(

                "Production mode violation: Metrics are disabled. "

                "Set metricsEnabled=true or metricsAdapter=\"prometheus\" in ConcernsContext::Config for production deployments."

            );
- Line 330: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: bool production_mode = core::ProductionMode::isEnabled();

    

    if (production_mode) {

        throw std::runtime_error(

            "Production mode violation: Cannot create no-op ConcernsContext in production. "

            "Use create() or createCustom() with real implementations instead."

        );
- Line 383: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 384: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: void ConcernsContext::replaceLogger(std::unique_ptr<ILogger> new_logger) {

    if (!new_logger) {

        throw std::invalid_argument("ConcernsContext::replaceLogger: new_logger must not be nullptr");

    }

    std::unique_ptr<ILogger> old;

    {
- Line 399: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 400: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: void ConcernsContext::replaceTracer(std::unique_ptr<ITracer> new_tracer) {

    if (!new_tracer) {

        throw std::invalid_argument("ConcernsContext::replaceTracer: new_tracer must not be nullptr");

    }

    std::unique_ptr<ITracer> old;

    {
- Line 412: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 413: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: void ConcernsContext::replaceMetrics(std::unique_ptr<IMetrics> new_metrics) {

    if (!new_metrics) {

        throw std::invalid_argument("ConcernsContext::replaceMetrics: new_metrics must not be nullptr");

    }

    std::unique_ptr<IMetrics> old;

    {
- Line 425: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 426: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: void ConcernsContext::replaceCache(std::unique_ptr<ICache> new_cache) {

    if (!new_cache) {

        throw std::invalid_argument("ConcernsContext::replaceCache: new_cache must not be nullptr");

    }

    std::unique_ptr<ICache> old;

    {
- Line 438: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 439: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: void ConcernsContext::replaceSecrets(std::unique_ptr<ISecrets> new_secrets) {

    if (!new_secrets) {

        throw std::invalid_argument("ConcernsContext::replaceSecrets: new_secrets must not be nullptr");

    }

    std::unique_ptr<ISecrets> old;

    {
- Line 451: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 452: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: void ConcernsContext::replaceFeatureFlags(std::unique_ptr<IFeatureFlags> new_ff) {

    if (!new_ff) {

        throw std::invalid_argument("ConcernsContext::replaceFeatureFlags: new_ff must not be nullptr");

    }

    std::unique_ptr<IFeatureFlags> old;

    {
- Line 464: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 465: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: void ConcernsContext::replaceAuditLog(std::unique_ptr<IAuditLog> new_audit) {

    if (!new_audit) {

        throw std::invalid_argument("ConcernsContext::replaceAuditLog: new_audit must not be nullptr");

    }

    std::unique_ptr<IAuditLog> old;

    {

### concerns/zero_copy_logger.cpp
Total findings: 17

- Line 151: severity=HIGH; category=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: std::snprintf(ms_str, sizeof(ms_str), "%03lld", static_cast<long long>(ms % 1000));
- Line 305: severity=HIGH; category=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: std::snprintf(buf, sizeof(buf), "\\u%04x", c);
- Line 321: severity=HIGH; category=uninitialized_array
  Description: Array contains garbage values; use with zeros or explicit init
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::uninitialized
  Context: Array declared without initialization
- Line 352: severity=HIGH; category=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: std::snprintf(ms_str, sizeof(ms_str), "%03lld", static_cast<long long>(ms % 1000));
- Line 30: severity=MEDIUM; category=primitive_no_volatile
  Description: Primitive shared across threads without volatile
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: thread_local bool tl_buffer_initialized = false;
- Line 163: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: buf += ",\"";
- Line 177: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: buf += ' ';
- Line 287: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: out += "\\\"";
- Line 353: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: buf += "{\"ts\":\"";
- Line 364: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: buf += ",\"";
- Line 382: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: buf += ' ';
- Line 46: severity=LOW; category=unstructured_log
  Description: Unstructured logging (use structured format)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: return logger_ && logger_->should_log(toSpdlogLevel(level));
- Line 57: severity=LOW; category=unstructured_log
  Description: Unstructured logging (use structured format)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: logger_->log(toSpdlogLevel(level), message);
- Line 105: severity=LOW; category=unstructured_log
  Description: Unstructured logging (use structured format)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: if (!logger_->should_log(toSpdlogLevel(level))) {
- Line 118: severity=LOW; category=unstructured_log
  Description: Unstructured logging (use structured format)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: logger_->log(toSpdlogLevel(level), buf);
- Line 129: severity=LOW; category=unstructured_log
  Description: Unstructured logging (use structured format)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: if (!logger_->should_log(toSpdlogLevel(level))) {
- Line 189: severity=LOW; category=unstructured_log
  Description: Unstructured logging (use structured format)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: logger_->log(toSpdlogLevel(level), buf);

### concerns/lockfree_metrics.cpp
Total findings: 3

- Line 402: severity=CRITICAL; category=thread_join_no_timeout
  Description: Thread join/wait without timeout (blocking indefinitely)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: flush_thread_.join();
- Line 265: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: key += '{';
- Line 269: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: key += ',';

## Update Workflow

- Refresh scanner artifacts with: python tools/gs3_orchestrator.py ./src --output ai_working/gap_scan_results.json
- Regenerate docs with: python tools/module_doc_generator.py . ai_working ai_working/module_gaps
- Add --no-mirror when you only want archive docs in ai_working/module_gaps.

Format: THEMIS_MODULE_GAPS_V4
