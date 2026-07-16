### Context

This issue implements the roadmap item 'Query Plan Caching' for the query domain. It is sourced from the consolidated roadmap under 🟡 Medium Priority — Near-term (v1.5.0 – v1.8.0) and targets milestone v1.7.0.

Primary detail section: Query Plan Caching

### Goal

Deliver the scoped changes for Query Plan Caching in src/query/ and complete the linked detail section in a release-ready state for v1.7.0.

### Detailed Scope

### Query Plan Caching
**Priority:** Medium  
**Target Version:** v1.7.0

Cache optimized query plans to skip parsing and optimization on repeated queries.

**Features:**
- Plan fingerprinting (query structure + statistics)
- Parameterized plan reuse
- Plan invalidation on schema/statistics changes
- Statistics-aware plan selection

**API:**
```cpp
class PlanCache {
public:
    struct CachedPlan {
        std::string query_fingerprint;
        QueryPlan plan;
        std::vector<ParameterInfo> parameters;
        std::chrono::system_clock::time_point created_at;
        Statistics statistics_snapshot;
    };
    
    // Get cached plan
    Result<CachedPlan> get(const std::string& query,
                          const Statistics& current_stats);
    
    // Cache plan
    void put(const std::string& query,
            const QueryPlan& plan,
            const Statistics& stats);
    
    // Invalidate on schema change
    void invalidateTable(const std::string& table);
};

// Example: Parameterized query
std::string query_template = 
    "FOR u IN users FILTER u.age > @age RETURN u";

// First execution: parse + optimize + cache
auto plan1 = engine.prepare(query_template, {{"age", 30}});
plan_cache.put(query_template, plan1, current_stats);

// Second execution: retrieve cached plan
auto cached = plan_cache.get(query_template, current_stats);
if (cached) {
    auto result = engine.execute(cached->plan, {{"age", 40}});
    // Skip parsing and optimization!
}
```

**Invalidation Strategy:**
- Schema changes: Invalidate all plans for affected tables
- Statistics drift: Invalidate if cardinality estimates change >10x
- Periodic: Refresh plans every 24 hours

---

### Acceptance Criteria

- [ ] Plan fingerprinting (query structure + statistics)
- [ ] Parameterized plan reuse
- [ ] Plan invalidation on schema/statistics changes
- [ ] Statistics-aware plan selection
- [ ] Schema changes: Invalidate all plans for affected tables
- [ ] Statistics drift: Invalidate if cardinality estimates change >10x
- [ ] Periodic: Refresh plans every 24 hours

### Relationships

- Roadmap row: #196 (🟡 Medium Priority — Near-term (v1.5.0 – v1.8.0))
- Depends on: none identified during generation.
- Part of: consolidated roadmap delivery tracking.

### References

- src/ROADMAP.md
- src/query/FUTURE_ENHANCEMENTS.md#query-plan-caching
- Source key: roadmap:196:query:v1.7.0:query-plan-caching

Generated from the consolidated source roadmap. Keep the roadmap and issue in sync when scope changes.

<!-- roadmap-source-key: roadmap:196:query:v1.7.0:query-plan-caching -->
<!-- roadmap-ref: row=196;module=query;target=v1.7.0 -->
<!-- roadmap-detail: src/query/FUTURE_ENHANCEMENTS.md#query-plan-caching -->
