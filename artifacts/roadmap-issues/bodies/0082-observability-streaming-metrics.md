### Context

This issue implements the roadmap item 'Streaming Metrics' for the observability domain. It is sourced from the consolidated roadmap under 🟠 High Priority — Near-term (v1.5.0 – v1.8.0) and targets milestone v1.6.0.

Primary detail section: Streaming Metrics

### Goal

Deliver the scoped changes for Streaming Metrics in src/observability/ and complete the linked detail section in a release-ready state for v1.6.0.

### Detailed Scope

### Streaming Metrics
**Priority:** High  
**Target Version:** v1.6.0

Real-time metric streaming via WebSocket or Server-Sent Events.

**Implementation:**
```cpp
class MetricsStreamServer {
public:
    // Start streaming server
    void start(const std::string& bind_address, uint16_t port);
    
    // Client subscription
    void subscribe(const StreamSubscription& subscription);
    
    // Push metrics to subscribers
    void pushMetrics(const MetricUpdate& update);
};

struct StreamSubscription {
    std::string client_id;
    std::vector<std::string> metric_names;
    std::vector<MetricFilter> filters;
    std::chrono::milliseconds update_interval;
};

// Client-side usage (JavaScript)
const ws = new WebSocket('ws://themisdb:8001/metrics/stream');
ws.send(JSON.stringify({
    subscribe: {
        metrics: ['query_latency_ms', 'cache_hit_rate'],
        filters: [{label: 'tenant_id', value: 'acme'}],
        interval_ms: 1000
    }
}));

ws.onmessage = (event) => {
    const update = JSON.parse(event.data);
    updateDashboard(update.metrics);
};
```

---

### Acceptance Criteria

- [ ] Implement the scoped changes described in the linked detail section.
- [ ] Add or update tests that verify the intended behaviour.

### Relationships

- Roadmap row: #82 (🟠 High Priority — Near-term (v1.5.0 – v1.8.0))
- Depends on: none identified during generation.
- Part of: consolidated roadmap delivery tracking.

### References

- src/ROADMAP.md
- src/observability/FUTURE_ENHANCEMENTS.md#streaming-metrics
- Source key: roadmap:82:observability:v1.6.0:streaming-metrics

Generated from the consolidated source roadmap. Keep the roadmap and issue in sync when scope changes.

<!-- roadmap-source-key: roadmap:82:observability:v1.6.0:streaming-metrics -->
<!-- roadmap-ref: row=82;module=observability;target=v1.6.0 -->
<!-- roadmap-detail: src/observability/FUTURE_ENHANCEMENTS.md#streaming-metrics -->
