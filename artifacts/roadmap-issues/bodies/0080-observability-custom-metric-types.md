### Context

This issue implements the roadmap item 'Custom Metric Types' for the observability domain. It is sourced from the consolidated roadmap under 🟠 High Priority — Near-term (v1.5.0 – v1.8.0) and targets milestone v1.6.0.

Primary detail section: Custom Metric Types

### Goal

Deliver the scoped changes for Custom Metric Types in src/observability/ and complete the linked detail section in a release-ready state for v1.6.0.

### Detailed Scope

### Custom Metric Types
**Priority:** High  
**Target Version:** v1.6.0

Extended metric types beyond counters, gauges, and histograms.

**Implementation:**
```cpp
class AdvancedMetrics {
public:
    // Summary (like histogram but with quantiles)
    void recordSummary(const std::string& name, double value,
                      const std::vector<double>& quantiles = {0.5, 0.9, 0.95, 0.99});
    
    // Exponential histogram (efficient for wide value ranges)
    void recordExponentialHistogram(const std::string& name, double value,
                                   double scale = 2.0);
    
    // Cardinality metrics
    void recordCardinality(const std::string& name, const std::string& value);
    
    // Time-weighted average
    void recordTimeWeightedAverage(const std::string& name, double value,
                                   std::chrono::seconds window);
    
    // Rate metrics (automatically computed)
    void recordRate(const std::string& name, double value,
                   std::chrono::seconds interval);
};

// Example: Track unique tenant access patterns
metrics.recordCardinality("active_tenants", tenant_id);
metrics.recordTimeWeightedAverage("tenant_qps", qps, std::chrono::minutes(5));
```

---

### Acceptance Criteria

- [ ] Implement the scoped changes described in the linked detail section.
- [ ] Add or update tests that verify the intended behaviour.

### Relationships

- Roadmap row: #80 (🟠 High Priority — Near-term (v1.5.0 – v1.8.0))
- Depends on: none identified during generation.
- Part of: consolidated roadmap delivery tracking.

### References

- src/ROADMAP.md
- src/observability/FUTURE_ENHANCEMENTS.md#custom-metric-types
- Source key: roadmap:80:observability:v1.6.0:custom-metric-types

Generated from the consolidated source roadmap. Keep the roadmap and issue in sync when scope changes.

<!-- roadmap-source-key: roadmap:80:observability:v1.6.0:custom-metric-types -->
<!-- roadmap-ref: row=80;module=observability;target=v1.6.0 -->
<!-- roadmap-detail: src/observability/FUTURE_ENHANCEMENTS.md#custom-metric-types -->
