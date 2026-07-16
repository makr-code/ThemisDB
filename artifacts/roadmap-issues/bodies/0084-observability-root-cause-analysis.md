### Context

This issue implements the roadmap item 'Root Cause Analysis' for the observability domain. It is sourced from the consolidated roadmap under 🟠 High Priority — Near-term (v1.5.0 – v1.8.0) and targets milestone v1.7.0.

Primary detail section: Root Cause Analysis

### Goal

Deliver the scoped changes for Root Cause Analysis in src/observability/ and complete the linked detail section in a release-ready state for v1.7.0.

### Detailed Scope

### Root Cause Analysis
**Priority:** High  
**Target Version:** v1.7.0

Automated root cause identification for performance issues.

**Implementation:**
```cpp
class RootCauseAnalyzer {
public:
    // Analyze performance degradation
    RootCauseReport analyzeIssue(const PerformanceIssue& issue,
                                 const SystemSnapshot& before,
                                 const SystemSnapshot& after);
    
    // Correlation analysis
    std::vector<CorrelatedMetric> findCorrelations(const std::string& metric_name);
    
    // Causal inference
    CausalGraph buildCausalGraph(const std::vector<TimeSeries>& metrics);
};

struct RootCauseReport {
    std::string primary_cause;  // "High compaction rate"
    double confidence;          // 0.87
    std::vector<std::string> contributing_factors;
    std::vector<std::string> remediation_steps;
    std::map<std::string, double> metric_impacts;  // metric -> change %
};

// Example: Why did query latency spike?
auto report = analyzer.analyzeIssue(latency_spike, before_snapshot, after_snapshot);
// Primary Cause: High compaction rate (87% confidence)
// Contributing Factors:
//   - Write amplification increased from 3.2 to 15.1
//   - Memtable flush rate doubled
//   - Block cache hit rate dropped from 85% to 45%
// Remediation:
//   1. Increase memtable size to reduce flush frequency
//   2. Tune compaction trigger threshold
//   3. Add more cache capacity
```

---

### Acceptance Criteria

- [ ] Implement the scoped changes described in the linked detail section.
- [ ] Add or update tests that verify the intended behaviour.

### Relationships

- Roadmap row: #84 (🟠 High Priority — Near-term (v1.5.0 – v1.8.0))
- Depends on: none identified during generation.
- Part of: consolidated roadmap delivery tracking.

### References

- src/ROADMAP.md
- src/observability/FUTURE_ENHANCEMENTS.md#root-cause-analysis
- Source key: roadmap:84:observability:v1.7.0:root-cause-analysis

Generated from the consolidated source roadmap. Keep the roadmap and issue in sync when scope changes.

<!-- roadmap-source-key: roadmap:84:observability:v1.7.0:root-cause-analysis -->
<!-- roadmap-ref: row=84;module=observability;target=v1.7.0 -->
<!-- roadmap-detail: src/observability/FUTURE_ENHANCEMENTS.md#root-cause-analysis -->
