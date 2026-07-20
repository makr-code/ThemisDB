# Field Diagnostics Integration Guide

**Version**: 1.0  
**Status**: Production Ready  
**Last Updated**: 2026-07-19  

## Overview

This guide shows how to integrate field diagnostics into ThemisDB modules (NLI Verifier, mTLS Connection Pool, and others).

---

## Quick Start: Emitting Diagnostics

### Basic Pattern

```cpp
#include "observability/field_diagnostics_collector.h"
#include "utils/field_diagnostics_schema.h"

namespace themis {
namespace rag {

class NLIFaithfulnessVerifier {
    void predict() {
        auto& collector = observability::FieldDiagnosticsCollector::getInstance();
        
        DiagnosticEvent evt{
            .timestamp = std::chrono::system_clock::now(),
            .failure_category = observability::DiagnosticFailureCategory::NLI_INFERENCE,
            .module_name = "rag",
            .error_message = "Inference completed successfully",
            .severity_level = observability::DiagnosticSeverity::INFO,
            .deployment_environment = "production",
            .version = "1.5.0"
        };
        
        // Optional: add context data
        evt.context_data["model_type"] = "bert_v2";
        evt.context_data["latency_ms"] = std::to_string(latency_ms);
        evt.context_data["confidence_score"] = std::to_string(confidence);
        
        // Emit (auto-masks PII)
        collector.emitWithPIIMasking(evt);
    }
};

}  // namespace rag
}  // namespace themis
```

---

## NLI Inference Instrumentation

### Location
`src/rag/nli_faithfulness_verifier.cpp`

### Events to Emit

#### 1. Successful Prediction
```cpp
DiagnosticEvent evt{
    .failure_category = DiagnosticFailureCategory::NLI_INFERENCE,
    .module_name = "rag",
    .error_message = "ONNX inference completed",
    .severity_level = DiagnosticSeverity::INFO,
    .context_data = {
        {"model_path", "models/nli_bert_v2.onnx"},
        {"latency_ms", std::to_string(inference_time_ms)},
        {"confidence_score", std::to_string(output_score)},
        {"inference_mode", "onnx"}
    }
};
```

#### 2. Model Load Failure (Fallback to Heuristic)
```cpp
DiagnosticEvent evt{
    .failure_category = DiagnosticFailureCategory::NLI_INFERENCE,
    .module_name = "rag",
    .error_message = "ONNX model load failed, falling back to heuristic",
    .severity_level = DiagnosticSeverity::WARN,
    .context_data = {
        {"model_path", model_path},
        {"error_reason", error_details},
        {"fallback_mode", "heuristic"},
        {"inference_mode", "heuristic"}
    }
};
```

#### 3. High Latency Warning
```cpp
if (inference_time_ms > 5000) {
    DiagnosticEvent evt{
        .failure_category = DiagnosticFailureCategory::NLI_INFERENCE,
        .module_name = "rag",
        .error_message = "NLI inference latency exceeded SLA",
        .severity_level = DiagnosticSeverity::WARN,
        .context_data = {
            {"latency_ms", std::to_string(inference_time_ms)},
            {"sla_threshold_ms", "5000"},
            {"inference_mode", "onnx"}
        }
    };
}
```

### Integration Checklist
- [ ] Include headers in nli_faithfulness_verifier.cpp
- [ ] Emit event on successful prediction (INFO level)
- [ ] Emit event on model load failure (WARN level)
- [ ] Emit event on latency spike (WARN level)
- [ ] Include inference_mode context (onnx vs. heuristic)
- [ ] Test PII masking doesn't break content
- [ ] Verify <1% CPU overhead

---

## mTLS Connection Pool Instrumentation

### Location
`src/sharding/mtls_connection_pool.cpp`

### Events to Emit

#### 1. Connection Acquisition Success
```cpp
DiagnosticEvent evt{
    .failure_category = DiagnosticFailureCategory::MTLS_CONNECTION,
    .module_name = "sharding",
    .error_message = "Connection acquired successfully",
    .severity_level = DiagnosticSeverity::INFO,
    .context_data = {
        {"endpoint", endpoint_address},
        {"pool_size", std::to_string(current_pool_size)},
        {"acquisition_latency_ms", std::to_string(latency_ms)},
        {"connection_reuse", connection_was_reused ? "true" : "false"}
    }
};
```

#### 2. SSL/TLS Error
```cpp
DiagnosticEvent evt{
    .failure_category = DiagnosticFailureCategory::MTLS_CONNECTION,
    .module_name = "sharding",
    .error_message = "SSL handshake failed",
    .severity_level = DiagnosticSeverity::ERROR,
    .context_data = {
        {"endpoint", endpoint},
        {"error_code", ssl_error_code},
        {"error_reason", ssl_error_description},
        {"tls_version", negotiated_tls_version}
    }
};
```

#### 3. Connection Pool Exhaustion
```cpp
if (pool_utilization > 0.95) {  // 95% full
    DiagnosticEvent evt{
        .failure_category = DiagnosticFailureCategory::MTLS_CONNECTION,
        .module_name = "sharding",
        .error_message = "Connection pool near exhaustion",
        .severity_level = DiagnosticSeverity::WARN,
        .affected_user_count = estimated_affected_users,
        .context_data = {
            {"endpoint", endpoint},
            {"pool_size", std::to_string(pool_size)},
            {"available_connections", std::to_string(available_count)},
            {"utilization_percent", std::to_string(pool_utilization * 100)}
        }
    };
}
```

#### 4. Endpoint Health Transition
```cpp
// Available → Degraded
DiagnosticEvent evt{
    .failure_category = DiagnosticFailureCategory::MTLS_CONNECTION,
    .module_name = "sharding",
    .error_message = "Endpoint health degraded",
    .severity_level = DiagnosticSeverity::WARN,
    .context_data = {
        {"endpoint", endpoint},
        {"old_status", "available"},
        {"new_status", "degraded"},
        {"error_rate_percent", std::to_string(error_rate)}
    }
};
```

### Integration Checklist
- [ ] Include headers in mtls_connection_pool.cpp
- [ ] Emit event on successful connection (INFO level)
- [ ] Emit event on SSL/TLS error (ERROR level)
- [ ] Emit event on pool exhaustion (WARN level)
- [ ] Emit event on endpoint health change (WARN/ERROR level)
- [ ] Include metrics: latency_ms, pool_size, error_rate
- [ ] Test under concurrent connection scenarios
- [ ] Verify <1% CPU overhead at high connection rates

---

## General Guidelines

### When to Emit

Emit events for:
- **Significant events**: Model load failures, SSL errors, pool exhaustion
- **Performance degradation**: Latency > SLA, high error rate
- **State transitions**: Endpoint health changes, fallback mode activation
- **Resource pressure**: Near-capacity warnings

Do NOT emit for:
- Every successful operation (use counters/gauges in Prometheus instead)
- Low-severity debug information (use structured logs instead)
- Events that would cause high cardinality (avoid per-request events)

### Context Data Guidelines

Good context data:
```cpp
.context_data = {
    {"endpoint", "shard-1:6379"},
    {"latency_ms", "1234"},
    {"error_count", "5"}
}
```

Avoid (PII risk):
```cpp
.context_data = {
    {"query", query_string},           // PII!
    {"user_id", user_id},              // PII!
    {"response_body", response}        // PII!
}
```

### Error Messages

Keep error messages:
- **Concise**: <200 characters
- **Actionable**: Hint at remediation (e.g., "Certificate expired")
- **Sanitized**: No PII (query strings, user IDs, API keys)
- **Consistent**: Use standard terminology

Examples:
```cpp
"ONNX model load failed"                    // ✅ Good
"Model loading returned null pointer at line 42"  // ❌ Too specific
"User 12345 query failed"                   // ❌ Contains PII
"Query execution timeout (SLA: 30s)"        // ✅ Good
```

---

## Testing Instrumentation

### Unit Test Pattern

```cpp
#include "observability/field_diagnostics_collector.h"
#include "utils/field_diagnostics_schema.h"

TEST(NLIVerifierTest, EmitsDiagnosticOnSuccess) {
    auto& collector = FieldDiagnosticsCollector::getInstance();
    collector.clearBuffer();
    
    NLIVerifier verifier;
    verifier.predict(query, context);
    
    auto events = collector.getAllEvents();
    ASSERT_EQ(events.size(), 1);
    EXPECT_EQ(events[0].failure_category, DiagnosticFailureCategory::NLI_INFERENCE);
    EXPECT_EQ(events[0].module_name, "rag");
}

TEST(ConnectionPoolTest, EmitsDiagnosticOnSSLError) {
    auto& collector = FieldDiagnosticsCollector::getInstance();
    collector.clearBuffer();
    
    // Trigger SSL error
    pool.acquireConnection("invalid-cert:6379");
    
    auto events = collector.getEventsSince(start_time);
    EXPECT_EQ(events[0].failure_category, DiagnosticFailureCategory::MTLS_CONNECTION);
    EXPECT_EQ(events[0].severity_level, DiagnosticSeverity::ERROR);
}
```

### Integration Test Pattern

```cpp
TEST(DiagnosticsIntegrationTest, MetricsEmittedViaPrometheus) {
    auto& collector = FieldDiagnosticsCollector::getInstance();
    auto& metrics = MetricsCollector::getInstance();
    
    collector.emitDiagnosticEvent(sample_event);
    
    // Verify metric counter incremented
    auto prometheus_output = metrics.getPrometheusMetrics();
    EXPECT_THAT(prometheus_output, 
                HasSubstr("field_diagnostic_events_total{category=\"NLI_INFERENCE\"}"));
}
```

---

## Performance Considerations

### Target Metrics
- **Emission latency**: <100µs per event (typical: 10-50µs)
- **CPU overhead**: <1% even under high event rate (1000+ events/sec)
- **Memory**: ~1KB per event average; 1000-event buffer = ~1MB

### Optimization Tips

1. **Batch diagnostics when possible**:
   ```cpp
   // Instead of emitting 100 events immediately:
   for (int i = 0; i < 100; i++) {
       collector.emitDiagnosticEvent(events[i]);  // Serial: slow
   }
   
   // Aggregate into one event with context:
   DiagnosticEvent batch{
       .error_message = "100 SSL errors detected in last 5 minutes",
       .context_data = {
           {"error_count", "100"},
           {"time_window_seconds", "300"},
           {"error_types", "SSL_HANDSHAKE_FAILED=80, CERT_EXPIRED=20"}
       }
   };
   ```

2. **Use sampling for high-volume events**:
   ```yaml
   field_diagnostics:
     sampling:
       NLI_INFERENCE: 1.0       # Always collect
       MTLS_CONNECTION: 0.5     # 50% sampling
       QUERY_TIMEOUT: 0.1       # 10% sampling
   ```

3. **Only include necessary context**:
   ```cpp
   // Not needed: context_data["timestamp"] (already in event.timestamp)
   // Not needed: context_data["module"] (already in event.module_name)
   // Good: context_data["error_code"], context_data["retry_count"]
   ```

---

## Monitoring Diagnostic Health

### Check Collection is Working

```bash
# Export events from last hour
curl http://localhost:9090/diag/events?since=3600 | jq '. | length'

# Check distribution by category
curl http://localhost:9090/diag/stats | jq '.events_by_category'

# Real-time stream
curl http://localhost:9090/diag/stream?min_severity=ERROR
```

### Validate PII Masking

```bash
# Export events and check for sensitive patterns
curl http://localhost:9090/diag/events?since=3600 | \
    jq '.[] | select(.context_data != null) | .context_data' | \
    grep -iE 'user_|api_key|token|password'

# If any matches: PII leaked (security issue!)
```

---

## Next Steps

1. **Integrate with NLI Verifier** (Phase 1 after ONNX integration)
2. **Integrate with mTLS Connection Pool** (Phase 2 after Factory Refactor)
3. **Instrument Query Timeout paths** (Phase 3)
4. **Build Grafana dashboards** (Phase 4)
5. **Deploy to production** (Phase 5)

---

**Document Version**: 1.0  
**Status**: Ready for Implementation  
**Maintainer**: ThemisDB Observability Team
