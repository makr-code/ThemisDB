### Context

This issue implements the roadmap item 'Metric Aggregation Pipeline' for the observability domain. It is sourced from the consolidated roadmap under 🟠 High Priority — Near-term (v1.5.0 – v1.8.0) and targets milestone v1.6.0.

Primary detail section: Metric Aggregation Pipeline

### Goal

Deliver the scoped changes for Metric Aggregation Pipeline in src/observability/ and complete the linked detail section in a release-ready state for v1.6.0.

### Detailed Scope

### Metric Aggregation Pipeline
**Priority:** High  
**Target Version:** v1.6.0

Pre-aggregate metrics across shards for efficient querying.

**Implementation:**
```cpp
class MetricAggregator {
public:
    // Configure aggregation rules
    void addAggregationRule(const AggregationRule& rule);
    
    // Aggregate metrics from multiple shards
    MetricSnapshot aggregateShardMetrics(const std::vector<ShardMetrics>& shard_metrics);
    
    // Rollup metrics to reduce cardinality
    void rollupMetrics(std::chrono::minutes window);
};

struct AggregationRule {
    std::string metric_name;
    AggregationType type;  // SUM, AVG, MAX, MIN, P99
    std::chrono::seconds interval;
    std::vector<std::string> group_by_labels;
    std::vector<std::string> drop_labels;  // Reduce cardinality
};

// Example: Aggregate query latency across shards
AggregationRule rule;
rule.metric_name = "query_latency_ms";
rule.type = AggregationType::P95;
rule.interval = std::chrono::seconds(60);
rule.group_by_labels = {"tenant_id", "query_type"};
rule.drop_labels = {"shard_id", "instance_id"};  // Drop high-cardinality labels

aggregator.addAggregationRule(rule);
```

---

### Acceptance Criteria

- [ ] Implement the scoped changes described in the linked detail section.
- [ ] Add or update tests that verify the intended behaviour.

### Relationships

- Roadmap row: #81 (🟠 High Priority — Near-term (v1.5.0 – v1.8.0))
- Depends on: none identified during generation.
- Part of: consolidated roadmap delivery tracking.

### References

- src/ROADMAP.md
- src/observability/FUTURE_ENHANCEMENTS.md#metric-aggregation-pipeline
- Source key: roadmap:81:observability:v1.6.0:metric-aggregation-pipeline

Generated from the consolidated source roadmap. Keep the roadmap and issue in sync when scope changes.

<!-- roadmap-source-key: roadmap:81:observability:v1.6.0:metric-aggregation-pipeline -->
<!-- roadmap-ref: row=81;module=observability;target=v1.6.0 -->
<!-- roadmap-detail: src/observability/FUTURE_ENHANCEMENTS.md#metric-aggregation-pipeline -->
