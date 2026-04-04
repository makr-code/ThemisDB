### Context

This issue implements the roadmap item 'Time-Travel Query Engine' for the temporal domain. It is sourced from the consolidated roadmap under 🟠 High Priority — Immediate (≤ v1.4.0) and targets milestone v1.2.0.

Primary detail section: Time-Travel Query Engine

### Goal

Deliver the scoped changes for Time-Travel Query Engine in src/temporal/ and complete the linked detail section in a release-ready state for v1.2.0.

### Detailed Scope

### Time-Travel Query Engine
**Priority:** High  
**Target Version:** v1.2.0

Full SQL:2011 temporal query support with optimization.

**Features:**
- FOR SYSTEM_TIME AS OF queries
- FOR SYSTEM_TIME BETWEEN...AND queries
- FOR SYSTEM_TIME FROM...TO queries
- FOR APPLICATION_TIME queries
- Temporal predicates (OVERLAPS, CONTAINS, etc.)
- Query optimization for temporal operations

**Implementation:**
```cpp
class TemporalQueryEngine {
public:
    enum class TemporalClause {
        AS_OF,
        FROM_TO,
        BETWEEN_AND,
        CONTAINED_IN,
        OVERLAPS
    };
    
    struct TemporalQuery {
        TemporalClause clause;
        std::chrono::system_clock::time_point start_time;
        std::chrono::system_clock::time_point end_time;
        bool include_deleted = false;
    };
    
    // Execute temporal query
    Result<std::vector<Document>> executeTemporalQuery(
        const std::string& table_name,
        const std::string& base_query,
        const TemporalQuery& temporal_spec
    );
    
    // Temporal join
    Result<std::vector<Document>> temporalJoin(
        const std::string& left_table,
        const std::string& right_table,
        const std::string& join_condition,
        const TemporalQuery& left_temporal,
        const TemporalQuery& right_temporal
    );
};
```

**Query Examples:**
```sql
-- As of specific time
SELECT * FROM employees
FOR SYSTEM_TIME AS OF TIMESTAMP '2024-01-01 00:00:00'
WHERE department = 'Engineering';

-- All versions in range
SELECT * FROM employees
FOR SYSTEM_TIME FROM TIMESTAMP '2024-01-01'
                  TO TIMESTAMP '2024-12-31'
WHERE employee_id = 12345;

-- Temporal join
SELECT e.name, d.dept_name
FROM employees FOR SYSTEM_TIME AS OF TIMESTAMP '2024-06-01' e
JOIN departments FOR SYSTEM_TIME AS OF TIMESTAMP '2024-06-01' d
  ON e.department_id = d.dept_id;
```

**Optimization:**
- Temporal index utilization
- Version pruning for AS OF queries
- Batch fetching for range queries
- Result caching for frequently accessed historical data

---

### Acceptance Criteria

- [ ] FOR SYSTEM_TIME AS OF queries
- [ ] FOR SYSTEM_TIME BETWEEN...AND queries
- [ ] FOR SYSTEM_TIME FROM...TO queries
- [ ] FOR APPLICATION_TIME queries
- [ ] Temporal predicates (OVERLAPS, CONTAINS, etc.)
- [ ] Query optimization for temporal operations
- [ ] Temporal index utilization
- [ ] Version pruning for AS OF queries
- [ ] Batch fetching for range queries
- [ ] Result caching for frequently accessed historical data

### Relationships

- Roadmap row: #25 (🟠 High Priority — Immediate (≤ v1.4.0))
- Depends on: none identified during generation.
- Part of: consolidated roadmap delivery tracking.

### References

- src/ROADMAP.md
- src/temporal/FUTURE_ENHANCEMENTS.md#time-travel-query-engine
- Source key: roadmap:25:temporal:v1.2.0:time-travel-query-engine

Generated from the consolidated source roadmap. Keep the roadmap and issue in sync when scope changes.

<!-- roadmap-source-key: roadmap:25:temporal:v1.2.0:time-travel-query-engine -->
<!-- roadmap-ref: row=25;module=temporal;target=v1.2.0 -->
<!-- roadmap-detail: src/temporal/FUTURE_ENHANCEMENTS.md#time-travel-query-engine -->
