### Context

This issue implements the roadmap item 'Temporal Aggregations' for the temporal domain. It is sourced from the consolidated roadmap under 🟡 Medium Priority — Near-term (v1.5.0 – v1.8.0) and targets milestone v1.3.0.

Primary detail section: Temporal Aggregations

### Goal

Deliver the scoped changes for Temporal Aggregations in src/temporal/ and complete the linked detail section in a release-ready state for v1.3.0.

### Detailed Scope

### Temporal Aggregations
**Priority:** Medium  
**Target Version:** v1.3.0

Advanced aggregation operations over temporal data.

**Features:**
- Time-window aggregations
- Sliding window calculations
- Temporal GROUP BY operations
- Snapshot aggregations at regular intervals
- Trend analysis functions

**Implementation:**
```cpp
class TemporalAggregator {
public:
    enum class WindowType {
        TUMBLING,      // Non-overlapping windows
        SLIDING,       // Overlapping windows
        SESSION        // Gap-based windows
    };
    
    struct AggregationSpec {
        WindowType window_type;
        std::chrono::seconds window_size;
        std::chrono::seconds slide_interval;  // For sliding windows
        std::string aggregation_function;  // SUM, AVG, COUNT, etc.
        std::vector<std::string> group_by_columns;
    };
    
    // Aggregate over time windows
    Result<std::vector<AggregateResult>> aggregateOverTime(
        const std::string& table_name,
        const std::string& measure_column,
        const AggregationSpec& spec,
        const TimeRange& range
    );
    
    // Calculate trends
    Result<TrendAnalysis> analyzeTrend(
        const std::string& table_name,
        const std::string& measure_column,
        const TimeRange& range
    );
};

struct AggregateResult {
    std::chrono::system_clock::time_point window_start;
    std::chrono::system_clock::time_point window_end;
    double aggregate_value;
    size_t record_count;
    std::map<std::string, std::string> group_values;
};
```

**Query Examples:**
```sql
-- Monthly sales aggregation
SELECT 
    YEAR(sys_start) as year,
    MONTH(sys_start) as month,
    SUM(amount) as total_sales
FROM sales
FOR SYSTEM_TIME FROM '2024-01-01' TO '2024-12-31'
GROUP BY YEAR(sys_start), MONTH(sys_start);

-- Moving average over time
SELECT 
    sys_start,
    AVG(price) OVER (
        ORDER BY sys_start
        ROWS BETWEEN 29 PRECEDING AND CURRENT ROW
    ) as moving_avg_30day
FROM products
FOR SYSTEM_TIME FROM '2024-01-01' TO '2024-12-31';
```

---

### Acceptance Criteria

- [ ] Time-window aggregations
- [ ] Sliding window calculations
- [ ] Temporal GROUP BY operations
- [ ] Snapshot aggregations at regular intervals
- [ ] Trend analysis functions

### Relationships

- Roadmap row: #211 (🟡 Medium Priority — Near-term (v1.5.0 – v1.8.0))
- Depends on: none identified during generation.
- Part of: consolidated roadmap delivery tracking.

### References

- src/ROADMAP.md
- src/temporal/FUTURE_ENHANCEMENTS.md#temporal-aggregations
- Source key: roadmap:211:temporal:v1.3.0:temporal-aggregations

Generated from the consolidated source roadmap. Keep the roadmap and issue in sync when scope changes.

<!-- roadmap-source-key: roadmap:211:temporal:v1.3.0:temporal-aggregations -->
<!-- roadmap-ref: row=211;module=temporal;target=v1.3.0 -->
<!-- roadmap-detail: src/temporal/FUTURE_ENHANCEMENTS.md#temporal-aggregations -->
