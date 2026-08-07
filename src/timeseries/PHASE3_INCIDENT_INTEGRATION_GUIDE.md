# Phase 3: Incident Taxonomy Integration Guide

## Overview

This guide demonstrates how to integrate the unified incident taxonomy into key timeseries components. Phase 3 introduces systematic error handling across all paths:

- **Ingest Path**: Buffer pressure, validation failures, flush timeouts
- **Query Path**: Range errors, consistency failures, timeout
- **Lifecycle Path**: Retention violations, key rotation failures
- **Integration Path**: Remote-write failures, metrics export errors

## Integration Examples

### 1. Adaptive Flush Controller Integration

**File**: `src/timeseries/adaptive_flush_controller.cpp`

#### Header Addition

```cpp
#include "timeseries/timeseries_incident_taxonomy.h"
```

#### Backpressure Event Emission

In the `add()` and `addBatch()` methods, emit incident when watermark is reached:

```cpp
Result<void> AdaptiveFlushController::add(const TSStore::DataPoint& point) {
    if (const char* err = validatePoint(point)) {
        return ErrVoid(errors::ErrorCode::ERR_API_INVALID_REQUEST, err);
    }

    // Backpressure: block producer when buffer is at/above watermark.
    if (watermarkReached()) {
        ++stat_backpressure_events_;
        
        // PHASE 3: Emit incident for backpressure
        IncidentContext ctx{
            .series_id = point.metric,
            .recovery_hint = "reduce_ingest_rate_or_increase_buffer_capacity",
            .caller_tag = "adaptive_flush_controller"
        };
        emitIncident(Incident::warnIngest(
            IngestIncidentCode::BUFFER_PRESSURE_HIGH,
            ctx
        ));
        
        THEMIS_WARN("AdaptiveFlushController backpressure: buffer={}/{} ({}%), blocking producer",
                    buffer_size_.load(std::memory_order_relaxed),
                    config_.buffer_capacity,
                    static_cast<int>(config_.watermark_ratio * 100));

        if (config_.metrics) {
            config_.metrics->recordBackpressure(point.metric);
        }

        // ... rest of backpressure logic
    }
}
```

#### Buffer Overflow Emission

In `flushInternal()`, detect and emit buffer overflow:

```cpp
size_t AdaptiveFlushController::flushInternal() {
    size_t total_flushed = 0;

    while (true) {
        std::vector<TSStore::DataPoint> batch;
        batch.reserve(config_.flush_batch_size);

        {
            std::lock_guard<std::mutex> lock(buffer_mutex_);
            size_t take = std::min(config_.flush_batch_size, buffer_.size());
            if (take == 0) break;

            for (size_t i = 0; i < take; ++i) {
                batch.push_back(std::move(buffer_.front()));
                buffer_.pop_front();
            }

            // PHASE 3: Detect overflow condition
            if (buffer_size_.load(std::memory_order_relaxed) > config_.buffer_capacity * 1.1) {
                IncidentContext ctx{
                    .recovery_hint = "emergency_action_required_buffer_overflow",
                    .extra_info = std::to_string(buffer_size_.load(std::memory_order_relaxed))
                };
                emitIncident(Incident::criticalIngest(
                    IngestIncidentCode::BUFFER_OVERFLOW_IMMINENT,
                    ctx
                ));
            }
        }

        // ... rest of flush logic
    }

    return total_flushed;
}
```

#### Flush Timeout Emission

In `flushThread()`, detect timeout:

```cpp
void AdaptiveFlushController::flushThread() {
    while (running_.load(std::memory_order_relaxed)) {
        auto start = std::chrono::steady_clock::now();
        
        // ... wait logic
        
        bool by_watermark = watermarkReached();
        size_t flushed = flushInternal();
        
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - start
        );
        
        // PHASE 3: Emit timeout if flush took too long
        if (elapsed > config_.flush_interval * 2) {
            IncidentContext ctx{
                .recovery_hint = "check_disk_io_performance",
                .extra_info = std::to_string(elapsed.count()) + "ms"
            };
            emitIncident(Incident::errorIngest(
                IngestIncidentCode::FLUSH_TIMEOUT,
                ctx
            ));
            
            THEMIS_ERROR("AdaptiveFlushController flush timeout: took {}ms", elapsed.count());
        }
    }
}
```

### 2. Retention Module Integration

**File**: `src/timeseries/retention.cpp`

#### Header Addition

```cpp
#include "timeseries/timeseries_incident_taxonomy.h"
```

#### Retention Policy Violation Emission

```cpp
Result<void> RetentionPolicy::enforcePolicy(const TimeRange& range) {
    auto current_time = std::chrono::system_clock::now();
    
    // Check if range violates retention
    if (range.start < (current_time - max_age_)) {
        IncidentContext ctx{
            .series_id = "",  // could be populated if known
            .recovery_hint = "adjust_retention_policy_or_reduce_max_age",
            .extra_info = "attempted_to_query_retained_data"
        };
        emitIncident(Incident::errorLifecycle(
            LifecycleIncidentCode::RETENTION_POLICY_VIOLATION,
            ctx
        ));
        return ErrVoid(errors::ErrorCode::RETENTION_POLICY_VIOLATION, 
                      "data_outside_retention_window");
    }
    
    return OkVoid();
}
```

#### Deletion Failure Emission

```cpp
Result<void> RetentionPolicy::deleteExpiredChunks() {
    std::vector<std::string> chunks_to_delete;
    
    // Identify expired chunks
    for (const auto& chunk : chunks_) {
        if (isExpired(chunk)) {
            chunks_to_delete.push_back(chunk.path);
        }
    }
    
    for (const auto& chunk_path : chunks_to_delete) {
        Result<void> result = deleteChunkFile(chunk_path);
        if (!result) {
            // PHASE 3: Emit deletion failure
            IncidentContext ctx{
                .series_id = chunk_path,
                .recovery_hint = "check_disk_permissions_or_disk_space",
                .extra_info = result.error().message()
            };
            emitIncident(Incident::errorLifecycle(
                LifecycleIncidentCode::DELETION_FAILED,
                ctx
            ));
            
            THEMIS_ERROR("Retention: failed to delete chunk {}: {}", 
                        chunk_path, result.error().message());
        }
    }
    
    return OkVoid();
}
```

### 3. Prometheus Remote Write Integration

**File**: `src/timeseries/prometheus_remote_write.cpp`

#### Header Addition

```cpp
#include "timeseries/timeseries_incident_taxonomy.h"
```

#### Remote Write Validation Error Emission

```cpp
Result<void> PrometheusRemoteWrite::validateWriteRequest(const WriteRequest& req) {
    // Check schema compatibility
    if (!req.timeseries_schema().IsInitialized()) {
        IncidentContext ctx{
            .recovery_hint = "verify_prometheus_remote_write_schema",
            .extra_info = "missing_required_fields"
        };
        emitIncident(Incident::errorIntegration(
            IntegrationIncidentCode::REMOTE_WRITE_VALIDATION_ERROR,
            ctx
        ));
        return ErrVoid(errors::ErrorCode::REMOTE_WRITE_VALIDATION_ERROR,
                      "schema_validation_failed");
    }
    
    return OkVoid();
}
```

#### Remote Write Retry Exhaustion Emission

```cpp
Result<void> PrometheusRemoteWrite::sendWithRetry(const WriteRequest& req, int max_retries) {
    int attempt = 0;
    Result<void> last_error;
    
    while (attempt < max_retries) {
        Result<void> result = sendRemote(req);
        if (result) {
            return OkVoid();
        }
        
        last_error = result;
        ++attempt;
        
        // Check if error is retryable
        if (isPermanentIntegrationError(last_error.error().code())) {
            break;  // Don't retry permanent errors
        }
        
        // Exponential backoff
        std::this_thread::sleep_for(
            std::chrono::milliseconds(100 * (1 << attempt))
        );
    }
    
    // PHASE 3: Emit retry exhaustion
    if (attempt >= max_retries) {
        IncidentContext ctx{
            .recovery_hint = "check_remote_prometheus_server_health",
            .extra_info = std::to_string(attempt) + "_retries_exhausted"
        };
        ctx.http_status = getHttpStatus(last_error);
        ctx.retry_count = attempt;
        
        emitIncident(Incident::errorIntegration(
            IntegrationIncidentCode::REMOTE_WRITE_RETRIES_EXHAUSTED,
            ctx
        ));
    }
    
    return last_error;
}
```

### 4. Encrypted Chunk Store Integration

**File**: `src/timeseries/encrypted_chunk_store.cpp`

#### Header Addition

```cpp
#include "timeseries/timeseries_incident_taxonomy.h"
```

#### Key Rotation Failure Emission

```cpp
Result<void> EncryptedChunkStore::rotateEncryptionKey(int new_key_version) {
    Result<std::string> key_result = kms_.getKey(new_key_version);
    if (!key_result) {
        IncidentContext ctx{
            .recovery_hint = "contact_kms_administrator_or_check_kms_connectivity",
            .extra_info = "key_version_" + std::to_string(new_key_version)
        };
        
        emitIncident(Incident::criticalLifecycle(
            LifecycleIncidentCode::ENCRYPTION_ROTATION_FAILURE,
            ctx
        ));
        
        return key_result.error();
    }
    
    // Attempt key rotation
    // ...
    
    return OkVoid();
}
```

#### Encryption State Validation

```cpp
Result<std::string> EncryptedChunkStore::decryptChunk(const EncryptedChunk& chunk) {
    // Validate encryption state
    if (chunk.key_version() < 0 || chunk.iv().empty() || chunk.ciphertext().empty()) {
        IncidentContext ctx{
            .series_id = chunk.series_name(),
            .recovery_hint = "check_chunk_integrity_or_perform_recovery",
            .extra_info = "encryption_state_validation_failed"
        };
        
        emitIncident(Incident::errorLifecycle(
            LifecycleIncidentCode::ENCRYPTION_STATE_INVALID,
            ctx
        ));
        
        return Err<std::string>(
            errors::ErrorCode::ENCRYPTION_STATE_INVALID,
            "invalid_encryption_state"
        );
    }
    
    // Proceed with decryption
    // ...
    
    return Ok(plaintext);
}
```

### 5. Query Optimizer Integration

**File**: `src/timeseries/query_optimizer.cpp`

#### Header Addition

```cpp
#include "timeseries/timeseries_incident_taxonomy.h"
```

#### Range Validation Emission

```cpp
Result<void> QueryOptimizer::validateTimeRange(const TimeRange& range) {
    if (range.start >= range.end) {
        IncidentContext ctx{
            .recovery_hint = "ensure_query_start_time_is_before_end_time"
        };
        
        emitIncident(Incident::errorQuery(
            QueryIncidentCode::RANGE_INVALID,
            ctx
        ));
        
        return ErrVoid(errors::ErrorCode::RANGE_INVALID, 
                      "invalid_time_range_start_gte_end");
    }
    
    return OkVoid();
}
```

#### Query Timeout Emission

```cpp
Result<std::vector<Point>> QueryOptimizer::executeQuery(
    const QueryRequest& req,
    std::chrono::milliseconds timeout) {
    
    auto start = std::chrono::steady_clock::now();
    std::vector<Point> results;
    
    // Execute query with timeout
    while (true) {
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - start
        );
        
        if (elapsed > timeout) {
            IncidentContext ctx{
                .series_id = req.metric_name(),
                .recovery_hint = "increase_timeout_or_narrow_time_range",
                .extra_info = std::to_string(elapsed.count()) + "ms"
            };
            
            emitIncident(Incident::errorQuery(
                QueryIncidentCode::QUERY_TIMEOUT,
                ctx
            ));
            
            return Err<std::vector<Point>>(
                errors::ErrorCode::QUERY_TIMEOUT,
                "query_exceeded_timeout"
            );
        }
        
        // ... continue query execution
    }
    
    return Ok(results);
}
```

#### Retention Boundary Crossing

```cpp
Result<std::vector<Point>> QueryOptimizer::queryWithRetentionCheck(
    const TimeRange& range,
    const RetentionPolicy& policy) {
    
    auto retention_cutoff = policy.getRetentionCutoff();
    
    if (range.start < retention_cutoff) {
        IncidentContext ctx{
            .recovery_hint = "data_outside_retention_window_may_be_incomplete",
            .extra_info = "requested_before_" + 
                         std::to_string(retention_cutoff.time_since_epoch().count())
        };
        
        emitIncident(Incident::warnQuery(
            QueryIncidentCode::RETENTION_BOUNDARY_CROSSED,
            ctx
        ));
    }
    
    // Proceed with query
    // ...
    
    return Ok(results);
}
```

## Test Coverage Strategy

The comprehensive test file `tests/timeseries/test_timeseries_error_path_phase3.cpp` provides:

1. **40+ Test Cases**: One for each incident code + helpers + factory methods
2. **Handler Registration Tests**: Verify incident handler invocation and lifecycle
3. **Concurrent Emission Tests**: Ensure thread safety under concurrent incident emission
4. **Classification Tests**: Validate error classification helpers (isBackpressureError, etc.)
5. **Performance Tests**: Verify <100µs latency per incident emission
6. **Severity Level Tests**: Validate correct severity assignment

## Integration Checklist

- [ ] Add incident taxonomy header includes to all modified files
- [ ] Replace THEMIS_WARN/ERROR logging with emitIncident() calls
- [ ] Populate IncidentContext with diagnostic information
- [ ] Use appropriate severity levels (CRITICAL, ERROR, WARN, INFO)
- [ ] Document recovery hints for each error path
- [ ] Register handlers for operational monitoring
- [ ] Run focused tests to validate error path coverage
- [ ] Measure incident emission latency in performance tests
- [ ] Update component documentation with error codes

## Performance Guardrails

- Incident creation: <5µs per creation (stack-allocated)
- Incident emission: <100µs per emission (including handler callback)
- No dynamic allocation in hot paths
- No throwing exceptions in incident handlers
- Atomic operations for thread safety (minimal contention)

## Next Steps

1. Apply integration examples to all key components
2. Run full test suite to validate >90% error path coverage
3. Update ROADMAP.md with Phase 3 completion checklist
4. Create operator runbooks for each incident type
5. Validate against Phase 3 acceptance criteria
