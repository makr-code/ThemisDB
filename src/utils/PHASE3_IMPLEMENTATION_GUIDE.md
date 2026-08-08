# Phase 3 Error Contracts - Implementation Guide for Components

This document provides detailed guidance on how to apply error contracts to individual utils components.

## 1. Observability Components (audit_logger, logger, tracing, saga_logger)

### Example: Updating audit_logger.h

#### Before (Current Pattern)
```cpp
/**
 * @brief Log a security event
 * @param event The security event to log
 */
void logEvent(const SecurityEvent& event);
```

#### After (With Error Contracts)
```cpp
/**
 * @brief Log a security event
 * 
 * Logs a security event to the audit trail with encryption and signing.
 * Thread-safe; suitable for concurrent use from multiple threads.
 * 
 * Events are queued in a bounded FIFO queue (default: 10,000 events).
 * When queue is full, the behavior depends on configuration:
 * - Default: Return AUDIT_QUEUE_FULL error
 * - Fallback: Drop oldest event and queue new one
 * 
 * @param event The security event to log
 * @return ErrorCode::UTILS_INVALID_ARGUMENT if event is invalid
 * @return ErrorCode::AUDIT_FORMAT_ERROR if event cannot be formatted
 * @return ErrorCode::AUDIT_ENCRYPTION_FAILED if encryption fails
 * @return ErrorCode::AUDIT_SIGNATURE_FAILED if signing fails
 * @return ErrorCode::AUDIT_QUEUE_FULL if bounded queue is full
 * @return ErrorCode::AUDIT_WRITE_FAILED if write to disk fails
 * @return 0 (success) if event was queued successfully
 * 
 * @error_contract
 * | Condition | ErrorCode | Severity | Logging | Recovery |
 * |-----------|-----------|----------|---------|----------|
 * | Input validation fails (missing required fields) | UTILS_INVALID_ARGUMENT | Warning | Event structure dump | Reject event, return error |
 * | Event formatting fails (too large, encoding error) | AUDIT_FORMAT_ERROR | Error | Event size, format error details | Truncate or skip field, retry |
 * | Encryption fails (key missing, crypto error) | AUDIT_ENCRYPTION_FAILED | Error | Key ID, cipher error details | Fallback to unencrypted (if policy allows) or reject |
 * | Signing fails (signature algorithm error) | AUDIT_SIGNATURE_FAILED | Error | Algorithm, key error details | Fallback to unsigned or reject |
 * | Queue at capacity | AUDIT_QUEUE_FULL | Warning | Queue size, new event summary | Drop oldest or reject |
 * | Write to disk fails (I/O, disk full) | AUDIT_WRITE_FAILED | Error | Disk error, path, available space | Retry with exponential backoff |
 * 
 * @throws May throw std::invalid_argument if event is null/invalid
 * @see ErrorCode - Audit logging error codes (9010-9019)
 * @see error_contracts.h - Error contract framework
 * 
 * @thread_safe Yes - uses internal mutex for queue operations
 * @bounded_resource Audit queue: max 10,000 events (configurable)
 * @recovery_strategy On queue full: drop oldest; on write fail: retry up to 3 times with backoff
 */
ErrorCode logEvent(const SecurityEvent& event);
```

### Example: Updating audit_logger.cpp (excerpt)

```cpp
ErrorCode AuditLogger::logEvent(const SecurityEvent& event) {
    using namespace themis::utils;
    
    // Input validation
    if (!event.isValid()) {
        auto ctx = makeErrorContext(
            ErrorCode::UTILS_INVALID_ARGUMENT,
            fmt::format("Invalid event structure: {}", event.toString()),
            "AuditLogger::logEvent",
            ErrorSeverity::Warning,
            true  // recoverable
        );
        ctx.context_info = fmt::format("Event type: {}", event.type);
        logErrorWithContext(ctx);
        return ErrorCode::UTILS_INVALID_ARGUMENT;
    }
    
    // Try to format event
    std::string formatted;
    try {
        formatted = formatEvent(event);
    } catch (const std::exception& e) {
        auto ctx = makeErrorContext(
            ErrorCode::AUDIT_FORMAT_ERROR,
            fmt::format("Event formatting failed: {}", e.what()),
            "AuditLogger::logEvent",
            ErrorSeverity::Error,
            true
        );
        ctx.context_info = fmt::format("Event size: {} bytes", event.data.size());
        logErrorWithContext(ctx);
        return ErrorCode::AUDIT_FORMAT_ERROR;
    }
    
    // Try to encrypt
    if (config_.encryption_enabled) {
        try {
            formatted = encryptEvent(formatted);
        } catch (const std::exception& e) {
            auto ctx = makeErrorContext(
                ErrorCode::AUDIT_ENCRYPTION_FAILED,
                fmt::format("Encryption failed: {}", e.what()),
                "AuditLogger::logEvent",
                ErrorSeverity::Error,
                true
            );
            ctx.context_info = fmt::format("Key ID: {}", encryption_key_id_);
            logErrorWithContext(ctx);
            
            // Fallback: try unencrypted
            if (config_.allow_unencrypted_fallback) {
                SPDLOG_WARN("Falling back to unencrypted audit logging");
            } else {
                return ErrorCode::AUDIT_ENCRYPTION_FAILED;
            }
        }
    }
    
    // Queue the event (bounded)
    {
        std::lock_guard<std::mutex> lock(queue_mutex_);
        
        if (audit_queue_.size() >= config_.max_queue_size) {
            auto ctx = makeErrorContext(
                ErrorCode::AUDIT_QUEUE_FULL,
                fmt::format("Audit queue full (size: {})", audit_queue_.size()),
                "AuditLogger::logEvent",
                ErrorSeverity::Warning,
                true
            );
            ctx.resource_limit = config_.max_queue_size;
            ctx.resource_current = audit_queue_.size();
            ctx.recovery_hint = "Increase queue size or reduce audit event volume";
            logErrorWithContext(ctx);
            
            if (config_.drop_oldest_when_full) {
                audit_queue_.pop_front();  // Drop oldest
            } else {
                return ErrorCode::AUDIT_QUEUE_FULL;
            }
        }
        
        audit_queue_.push_back(formatted);
    }
    
    // Schedule flush if needed
    if (audit_queue_.size() >= config_.flush_threshold) {
        return flush();
    }
    
    return ErrorCode(0);  // Success
}

// Flush with retry logic and error handling
ErrorCode AuditLogger::flush() {
    using namespace themis::utils;
    
    std::lock_guard<std::mutex> lock(queue_mutex_);
    
    if (audit_queue_.empty()) {
        return ErrorCode(0);
    }
    
    uint32_t retry_count = 0;
    while (retry_count < config_.max_flush_retries) {
        try {
            writeToStorage(audit_queue_);
            audit_queue_.clear();
            return ErrorCode(0);
        } catch (const std::exception& e) {
            retry_count++;
            
            auto ctx = makeErrorContext(
                ErrorCode::AUDIT_WRITE_FAILED,
                fmt::format("Write to storage failed: {}", e.what()),
                "AuditLogger::flush",
                retry_count >= config_.max_flush_retries ? 
                    ErrorSeverity::Fatal : ErrorSeverity::Error,
                retry_count < config_.max_flush_retries
            );
            ctx.retry_count = retry_count;
            ctx.context_info = fmt::format("Queue size: {}", audit_queue_.size());
            logErrorWithContext(ctx);
            
            if (retry_count < config_.max_flush_retries) {
                std::this_thread::sleep_for(
                    std::chrono::milliseconds(100 * (1 << retry_count))  // Exponential backoff
                );
            }
        }
    }
    
    return ErrorCode::AUDIT_WRITE_FAILED;
}
```

## 2. Privacy Detection Components (pii_detector, detection engines)

### Example: Updating pii_detector.h

```cpp
/**
 * @brief Detect PII entities in text
 * 
 * Scans text for personally identifiable information using loaded detection engines.
 * Supports multiple detection strategies (regex, NER, semantic).
 * 
 * Input and processing are bounded:
 * - Max input size: 10 MB
 * - Max patterns: 10,000
 * - Detection timeout: 5 seconds (default)
 * 
 * If detection timeout occurs, returns partial results.
 * If all engines fail, returns empty results (safe degradation).
 * 
 * @param text Input text to scan (will be truncated if > 10MB)
 * @param options Detection options (engines to use, timeout, etc.)
 * @return Vector of detected PII entities, or empty on engine failure
 * @return ErrorContext if critical failure prevents any detection
 * 
 * @error_contract
 * | Condition | ErrorCode | Severity | Logging | Recovery |
 * |-----------|-----------|----------|---------|----------|
 * | Input is empty | - | - | - | Return empty results (not an error) |
 * | Input text > 10MB | PRIVACY_INVALID_INPUT | Warning | Input size, limit | Truncate to 10MB, continue |
 * | Invalid UTF-8 encoding | PRIVACY_UNICODE_ERROR | Warning | Byte sequence, position | Skip invalid bytes, continue |
 * | Regex pattern too complex | PRIVACY_PATTERN_OVERFLOW | Error | Pattern complexity score, limit | Fallback to simpler patterns |
 * | Detection timeout (5s) | PRIVACY_DETECTION_TIMEOUT | Warning | Elapsed time, partial results | Return results found so far |
 * | Memory limit exceeded | PRIVACY_MEMORY_EXCEEDED | Error | Current memory, limit | Drop results, return empty |
 * | No detection engines available | PRIVACY_NO_ENGINE | Error | Configured engines, available | Return empty (safe) |
 * | Engine threw exception | PRIVACY_ENGINE_FAILED | Error | Engine name, exception details | Skip engine, try next |
 * 
 * @thread_safe Yes (read-only detector state; thread-safe engines)
 * @bounded_resource Input: max 10MB; Patterns: max 10,000; Timeout: 5 seconds; Memory: max 500MB
 * @recovery_strategy On timeout: return partial results; On engine fail: skip engine; On memory: drop results
 */
struct PIIDetectionResult {
    std::vector<PIIFinding> findings;      ///< Detected PII entities
    ErrorCode error_code;                  ///< Error code if applicable
    uint64_t elapsed_ms;                   ///< Detection time in milliseconds
    bool was_truncated;                    ///< Whether input was truncated
    bool was_timed_out;                    ///< Whether detection timed out
    std::vector<std::string> failed_engines; ///< Engines that failed
};

PIIDetectionResult detectInText(
    const std::string& text,
    const DetectionOptions& options = DetectionOptions::defaults()
);
```

### Example: Updating pii_detector.cpp (excerpt)

```cpp
PIIDetectionResult PIIDetector::detectInText(
    const std::string& text,
    const DetectionOptions& options) {
    
    using namespace themis::utils;
    PIIDetectionResult result;
    result.error_code = ErrorCode(0);  // Success
    result.was_truncated = false;
    result.was_timed_out = false;
    
    auto start = std::chrono::high_resolution_clock::now();
    
    // Validate input size
    const uint64_t MAX_INPUT_SIZE = 10 * 1024 * 1024;  // 10 MB
    std::string processed_text = text;
    if (text.size() > MAX_INPUT_SIZE) {
        result.was_truncated = true;
        processed_text = text.substr(0, MAX_INPUT_SIZE);
        
        auto ctx = makeErrorContext(
            ErrorCode::PRIVACY_INVALID_INPUT,
            fmt::format("Input text truncated from {} to {} bytes",
                       text.size(), MAX_INPUT_SIZE),
            "PIIDetector::detectInText",
            ErrorSeverity::Warning,
            true
        );
        ctx.context_info = fmt::format("Original size: {} MB", text.size() / 1024 / 1024);
        logErrorWithContext(ctx);
    }
    
    // Validate UTF-8 encoding
    try {
        validateUTF8(processed_text);
    } catch (const std::exception& e) {
        auto ctx = makeErrorContext(
            ErrorCode::PRIVACY_UNICODE_ERROR,
            fmt::format("Invalid UTF-8 encoding: {}", e.what()),
            "PIIDetector::detectInText",
            ErrorSeverity::Warning,
            true
        );
        logErrorWithContext(ctx);
        
        // Fallback: clean invalid bytes
        processed_text = cleanUTF8(processed_text);
    }
    
    // Run detection engines with timeout
    uint64_t timeout_ms = options.timeout_ms > 0 ? 
        options.timeout_ms : 5000;  // Default 5 seconds
    
    for (const auto& [engine_name, engine] : engines_) {
        auto engine_start = std::chrono::high_resolution_clock::now();
        
        try {
            auto findings = engine->detect(processed_text);
            result.findings.insert(result.findings.end(),
                                  findings.begin(), findings.end());
        } catch (const std::exception& e) {
            result.failed_engines.push_back(engine_name);
            
            auto ctx = makeErrorContext(
                ErrorCode::PRIVACY_ENGINE_FAILED,
                fmt::format("Engine '{}' threw exception: {}", 
                           engine_name, e.what()),
                "PIIDetector::detectInText",
                ErrorSeverity::Error,
                true
            );
            ctx.context_info = fmt::format("Engine: {}", engine_name);
            logErrorWithContext(ctx);
            
            // Continue with other engines
            continue;
        }
        
        // Check timeout
        auto elapsed = std::chrono::high_resolution_clock::now() - engine_start;
        if (std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count() 
            > timeout_ms) {
            result.was_timed_out = true;
            
            auto ctx = makeErrorContext(
                ErrorCode::PRIVACY_DETECTION_TIMEOUT,
                fmt::format("Detection timeout after {}ms", timeout_ms),
                "PIIDetector::detectInText",
                ErrorSeverity::Warning,
                true
            );
            ctx.elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(elapsed);
            ctx.context_info = fmt::format("Findings so far: {}", result.findings.size());
            logErrorWithContext(ctx);
            
            break;  // Stop trying more engines
        }
    }
    
    // If all engines failed, log and return empty (safe degradation)
    if (result.failed_engines.size() == engines_.size()) {
        auto ctx = makeErrorContext(
            ErrorCode::PRIVACY_NO_ENGINE,
            "All detection engines failed",
            "PIIDetector::detectInText",
            ErrorSeverity::Error,
            true
        );
        ctx.context_info = fmt::format("Failed: {}",
            fmt::join(result.failed_engines, ", "));
        logErrorWithContext(ctx);
        
        result.error_code = ErrorCode::PRIVACY_NO_ENGINE;
    }
    
    result.elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::high_resolution_clock::now() - start
    ).count();
    
    return result;
}
```

## 3. Cryptography Components (hkdf_helper, hkdf_cache, pki_client)

### Example: Updating hkdf_helper.h

```cpp
/**
 * @brief Derive cryptographic keys using HKDF (RFC 5869)
 * 
 * Derives a new key from input key material using HMAC-based key derivation.
 * Suitable for deriving per-session keys, re-keying schedules, etc.
 * 
 * Bounded resource consumption:
 * - Input key: max 1024 bytes
 * - Salt: max 1024 bytes  
 * - Info: max 256 bytes
 * - Output: max 255 * hash_len
 * - Timeout: 1 second
 * 
 * @param input_key_material Input key material (should have good entropy)
 * @param salt Salt value (can be empty for random salt mode)
 * @param info Context-specific info for differentiation
 * @param output_length Requested output length in bytes
 * @return Derived key material, or empty vector on error
 * 
 * @error_contract
 * | Condition | ErrorCode | Severity | Logging | Recovery |
 * |-----------|-----------|----------|---------|----------|
 * | IKM validation fails | CRYPTO_KEY_INVALID | Warning | IKM size, validation error | Reject, return error |
 * | Salt validation fails | CRYPTO_KEY_INVALID | Warning | Salt size, validation error | Reject, return error |
 * | Output length invalid (too large) | UTILS_INVALID_ARGUMENT | Warning | Requested size, max size | Reject, return error |
 * | HKDF derivation fails | CRYPTO_KEY_DERIVATION_FAILED | Error | Algorithm error details | Retry up to 3 times |
 * | Timeout (1 second) | UTILS_TIMEOUT | Error | Elapsed time | Return error |
 * 
 * @thread_safe Yes (stateless; no shared state)
 * @bounded_resource IKM: max 1024 bytes; Salt: max 1024 bytes; Info: max 256 bytes; Output: max 255*32 = 8160 bytes; Timeout: 1s
 * @recovery_strategy On invalid params: return error immediately; On derivation fail: retry with fresh entropy
 */
std::vector<uint8_t> deriveKey(
    const std::vector<uint8_t>& input_key_material,
    const std::vector<uint8_t>& salt,
    const std::vector<uint8_t>& info,
    size_t output_length
);
```

## 4. Compression Components (zstd_codec, lz4_codec)

### Example: Updating zstd_codec.h

```cpp
/**
 * @brief Compress data using Zstandard (RFC 8878)
 * 
 * Compresses input data with bounded resource consumption:
 * - Max input: 1 GB
 * - Max output: 1.5x input or configurable
 * - Compression level: 0-22 (default: 3)
 * - Timeout: 30 seconds
 * 
 * On decompression bomb detection, returns error instead of decompressing.
 * 
 * @param input Data to compress
 * @param level Compression level (0-22; default: 3)
 * @return Compressed data, or empty on error
 * 
 * @error_contract
 * | Condition | ErrorCode | Severity | Logging | Recovery |
 * |-----------|-----------|----------|---------|----------|
 * | Input is empty | - | - | - | Return empty (not an error) |
 * | Input > 1GB | COMPRESSION_INPUT_INVALID | Warning | Input size, max size | Reject, return error |
 * | Invalid compression level | UTILS_INVALID_ARGUMENT | Warning | Level value, valid range | Reject, return error |
 * | Compression fails (codec error) | COMPRESSION_FAILED | Error | Codec error details | Retry with lower level |
 * | Output > 1.5x input | COMPRESSION_RATIO_EXCEEDED | Warning | Output size, limit | Fallback to no compression |
 * | Buffer allocation fails | UTILS_ALLOCATION_FAILED | Error | Requested size | Return error |
 * | Timeout | UTILS_TIMEOUT | Error | Elapsed time | Return error (partial data lost) |
 * 
 * @thread_safe Yes (stateless; creates fresh codec per call)
 * @bounded_resource Input: max 1GB; Output: max 1.5x input or configurable; Level: 0-22; Timeout: 30s
 * @recovery_strategy On ratio exceeded: fallback to no compression; On fail: retry with lower level
 */
std::vector<uint8_t> compress(
    const std::vector<uint8_t>& input,
    int level = 3
);

/**
 * @brief Decompress Zstandard-compressed data
 * 
 * Decompresses with decompression bomb detection:
 * - Expansion limit: 255x (default, configurable)
 * - Max output: 1 GB
 * - Timeout: 30 seconds
 * 
 * @param compressed Compressed data
 * @param max_expansion_ratio Maximum allowed expansion (default: 255)
 * @return Decompressed data, or empty on error
 * 
 * @error_contract
 * | Condition | ErrorCode | Severity | Logging | Recovery |
 * |-----------|-----------|----------|---------|----------|
 * | Input is empty | - | - | - | Return empty (not an error) |
 * | Input is not valid Zstd | COMPRESSION_INPUT_INVALID | Warning | Input header/magic | Reject, return error |
 * | Decompression bomb detected (>255x) | COMPRESSION_BOMB_DETECTED | Error | Expansion ratio, limit | Reject, return error |
 * | Decompression fails (codec error) | DECOMPRESSION_FAILED | Error | Codec error details | Return error |
 * | Output > 1GB | COMPRESSION_BUFFER_SMALL | Error | Output size needed | Return error |
 * | Timeout | UTILS_TIMEOUT | Error | Elapsed time | Return error (partial data) |
 * 
 * @thread_safe Yes
 * @bounded_resource Max expansion: 255x; Max output: 1GB; Timeout: 30s
 * @recovery_strategy On bomb detected: reject; On codec error: return error; On timeout: return partial data
 */
std::vector<uint8_t> decompress(
    const std::vector<uint8_t>& compressed,
    uint32_t max_expansion_ratio = 255
);
```

## 5. Runtime Services (thread_pool_manager, rate_limiter)

### Example: Updating thread_pool_manager.h

```cpp
/**
 * @brief Submit a task to the thread pool
 * 
 * Submits a task for execution on the thread pool. Task is queued
 * with bounded backpressure.
 * 
 * Queue bounds:
 * - Max capacity: 100,000 (configurable)
 * - When full: reject new tasks (return error)
 * 
 * @param task Function to execute
 * @param priority Task priority (0=highest, higher numbers=lower priority)
 * @return Future for result, or error code if rejected
 * 
 * @error_contract
 * | Condition | ErrorCode | Severity | Logging | Recovery |
 * |-----------|-----------|----------|---------|----------|
 * | Task is null | UTILS_INVALID_ARGUMENT | Warning | Call context | Reject immediately |
 * | Queue at capacity | THREADPOOL_QUEUE_FULL | Warning | Queue size, limit | Reject, caller must retry |
 * | Thread pool is shutting down | THREADPOOL_SHUTDOWN | Error | Shutdown state | Reject, caller must queue elsewhere |
 * | Task throws uncaught exception | - | - | Exception details | Log, fail future |
 * 
 * @thread_safe Yes (thread-safe queue, mutex-protected)
 * @bounded_resource Queue: max 100,000 tasks (configurable); Memory: depends on task size
 * @recovery_strategy On queue full: reject and return error; caller retries with backoff
 */
template<typename F>
std::future<typename std::result_of<F()>::type> submit(
    F&& task,
    uint32_t priority = 0
);
```

## Application Checklist

When applying error contracts to a component:

1. **Add includes**:
   ```cpp
   #include "utils/error_contracts.h"
   ```

2. **Update function signatures**:
   - Add return type (ErrorCode, Result<T>, or exceptions)
   - Document all possible error conditions

3. **Add comprehensive Doxygen @error_contract tags**:
   - Document when each error occurs
   - Specify severity level
   - Explain diagnostic logging
   - Describe recovery strategy

4. **Implement error handling**:
   - Use `makeErrorContext()` to create contexts
   - Use `logErrorWithContext()` to log with diagnostics
   - Return appropriate ErrorCode
   - Implement recovery/fallback behavior

5. **Add resource bounds checks**:
   - Input validation (size limits)
   - Timeout enforcement
   - Memory limits
   - Queue/buffer capacity checks

6. **Test error paths**:
   - Unit tests for each error condition (Phase 4)
   - Verify error logging and context
   - Verify recovery behavior
   - Verify no silent failures

## Resources

- [error_contracts.h](../../include/utils/error_contracts.h) - Error framework
- [error_contracts.cpp](../../src/utils/error_contracts.cpp) - Implementation
- [error_registry.h](../../include/utils/error_registry.h) - Global error registry
- [PHASE3_ERROR_CONTRACTS.md](./PHASE3_ERROR_CONTRACTS.md) - Phase 3 overview
- [ROADMAP.md](./ROADMAP.md) - Utils module roadmap
