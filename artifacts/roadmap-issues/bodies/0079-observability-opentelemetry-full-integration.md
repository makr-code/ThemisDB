### Context

This issue implements the roadmap item 'OpenTelemetry Full Integration' for the observability domain. It is sourced from the consolidated roadmap under 🟠 High Priority — Near-term (v1.5.0 – v1.8.0) and targets milestone v1.6.0.

Primary detail section: OpenTelemetry Full Integration

### Goal

Deliver the scoped changes for OpenTelemetry Full Integration in src/observability/ and complete the linked detail section in a release-ready state for v1.6.0.

### Detailed Scope

### OpenTelemetry Full Integration
**Priority:** High  
**Target Version:** v1.6.0

Complete OpenTelemetry implementation with automatic span propagation across distributed components.

**Features:**
- Automatic instrumentation for all database operations
- W3C Trace Context propagation
- Baggage support for tenant/user context
- Multiple exporter support (Jaeger, Zipkin, OTLP)

**Implementation:**
```cpp
class OpenTelemetryTracer : public ITracer {
public:
    OpenTelemetryTracer(const OTelConfig& config);
    
    // Automatic span creation with context propagation
    std::unique_ptr<ISpan> startSpan(const std::string& name) override;
    
    // Extract context from incoming request
    SpanContext extractContext(const std::map<std::string, std::string>& headers);
    
    // Inject context into outgoing request
    void injectContext(const ISpan& span, std::map<std::string, std::string>& headers);
    
    // Span attributes
    void recordException(const ISpan& span, const std::exception& ex);
    void recordMetrics(const ISpan& span, const MetricSnapshot& metrics);
};

// Configuration
struct OTelConfig {
    std::string service_name = "themisdb";
    std::string service_version = "1.6.0";
    std::string endpoint = "http://otel-collector:4317";
    std::string protocol = "grpc";  // or "http"
    double sample_rate = 1.0;  // 100% sampling
    std::map<std::string, std::string> resource_attributes;
    std::vector<std::string> exporters = {"otlp", "jaeger"};
};
```

**Usage:**
```cpp
// Initialize tracer
OTelConfig config;
config.service_name = "themisdb";
config.endpoint = "http://otel-collector:4317";
config.resource_attributes = {
    {"deployment.environment", "production"},
    {"service.instance.id", "themisdb-node-1"}
};

OpenTelemetryTracer tracer(config);

// Query with distributed tracing
{
    auto span = tracer.startSpan("query_execution");
    span->setAttribute("query.text", query_text);
    span->setAttribute("db.system", "themisdb");
    span->setAttribute("db.operation", "SELECT");
    
    // Child spans automatically inherit context
    executeQuery(query_text);
    
    span->setAttribute("query.result_rows", result_count);
    span->setStatus(true);
}
```

**Benefits:**
- End-to-end request tracing across microservices
- Cross-shard query visualization
- Root cause analysis for distributed issues
- Integration with existing observability platforms

---

### Acceptance Criteria

- [ ] Automatic instrumentation for all database operations
- [ ] W3C Trace Context propagation
- [ ] Baggage support for tenant/user context
- [ ] Multiple exporter support (Jaeger, Zipkin, OTLP)
- [ ] End-to-end request tracing across microservices
- [ ] Cross-shard query visualization
- [ ] Root cause analysis for distributed issues
- [ ] Integration with existing observability platforms

### Relationships

- Roadmap row: #79 (🟠 High Priority — Near-term (v1.5.0 – v1.8.0))
- Depends on: none identified during generation.
- Part of: consolidated roadmap delivery tracking.

### References

- src/ROADMAP.md
- src/observability/FUTURE_ENHANCEMENTS.md#opentelemetry-full-integration
- Source key: roadmap:79:observability:v1.6.0:opentelemetry-full-integration

Generated from the consolidated source roadmap. Keep the roadmap and issue in sync when scope changes.

<!-- roadmap-source-key: roadmap:79:observability:v1.6.0:opentelemetry-full-integration -->
<!-- roadmap-ref: row=79;module=observability;target=v1.6.0 -->
<!-- roadmap-detail: src/observability/FUTURE_ENHANCEMENTS.md#opentelemetry-full-integration -->
