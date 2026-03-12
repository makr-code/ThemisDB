### Context

This issue implements the roadmap item 'Materialized Views & Incremental Maintenance' for the query domain. It is sourced from the consolidated roadmap under 🟡 Medium Priority — Near-term (v1.5.0 – v1.8.0) and targets milestone v1.8.0.

Primary detail section: Materialized Views & Incremental Maintenance

### Goal

Deliver the scoped changes for Materialized Views & Incremental Maintenance in src/query/ and complete the linked detail section in a release-ready state for v1.8.0.

### Detailed Scope

### Materialized Views & Incremental Maintenance
**Priority:** Medium  
**Target Version:** v1.8.0

Pre-computed query results with automatic incremental updates.

**Features:**
- View definition and creation
- Automatic query rewriting (use view if applicable)
- Incremental maintenance on data changes
- Partial refresh strategies
- View staleness tracking

**API:**
```cpp
// Define materialized view
class MaterializedView {
public:
    struct Definition {
        std::string name;
        std::string query_aql;
        RefreshStrategy strategy;
        std::chrono::seconds staleness_tolerance{60};
    };
    
    enum RefreshStrategy {
        IMMEDIATE,       // Update on every base table change
        DEFERRED,        // Update on query if stale
        PERIODIC,        // Scheduled refresh
        MANUAL           // User-triggered refresh
    };
    
    // Create view
    static Result<void> create(const Definition& def,
                              QueryEngine& engine);
    
    // Refresh view
    Result<void> refresh(bool incremental = true);
    
    // Check if view can answer query
    static bool canRewrite(const ParsedQuery& query,
                          const MaterializedView& view);
};

// Example: Create view for frequent aggregation
MaterializedView::Definition view_def;
view_def.name = "sales_by_region";
view_def.query_aql = R"(
    FOR sale IN sales
    COLLECT region = sale.region INTO groups
    RETURN {
        region: region,
        total_sales: SUM(groups[*].sale.amount),
        avg_sale: AVG(groups[*].sale.amount),
        count: LENGTH(groups)
    }
)";
view_def.strategy = MaterializedView::DEFERRED;
view_def.staleness_tolerance = std::chrono::minutes(5);

MaterializedView::create(view_def, engine);

// Query automatically uses view if applicable
auto result = engine.execute(
    "FOR r IN sales_by_region FILTER r.region == 'EU' RETURN r"
);
// ^ Uses precomputed view, not raw sales table
```

**Incremental Maintenance:**
```cpp
// When base table changes
void onInsert(const std::string& table, const BaseEntity& entity) {
    for (auto& view : dependent_views[table]) {
        if (view->strategy == IMMEDIATE) {
            // Delta processing
            view->applyDelta(DeltaOp::INSERT, entity);
        } else if (view->strategy == DEFERRED) {
            view->markStale();
        }
    }
}

// Delta computation for common patterns
void applyAggregateDelta(const DeltaOp op, 
                        const BaseEntity& entity) {
    // Example: SUM(amount) - just add/subtract the delta
    if (op == INSERT) {
        aggregate_value += entity.getField("amount").as_double();
    } else if (op == DELETE) {
        aggregate_value -= entity.getField("amount").as_double();
    }
}
```

**Performance Impact:**
- Query speedup: 10-100x (depends on aggregation complexity)
- Insert overhead: 5-20% (incremental maintenance)

---

### Acceptance Criteria

- [ ] View definition and creation
- [ ] Automatic query rewriting (use view if applicable)
- [ ] Incremental maintenance on data changes
- [ ] Partial refresh strategies
- [ ] View staleness tracking
- [ ] Query speedup: 10-100x (depends on aggregation complexity)
- [ ] Insert overhead: 5-20% (incremental maintenance)

### Relationships

- Roadmap row: #195 (🟡 Medium Priority — Near-term (v1.5.0 – v1.8.0))
- Depends on: none identified during generation.
- Part of: consolidated roadmap delivery tracking.

### References

- src/ROADMAP.md
- src/query/FUTURE_ENHANCEMENTS.md#materialized-views--incremental-maintenance
- Source key: roadmap:195:query:v1.8.0:materialized-views-incremental-maintenance

Generated from the consolidated source roadmap. Keep the roadmap and issue in sync when scope changes.

<!-- roadmap-source-key: roadmap:195:query:v1.8.0:materialized-views-incremental-maintenance -->
<!-- roadmap-ref: row=195;module=query;target=v1.8.0 -->
<!-- roadmap-detail: src/query/FUTURE_ENHANCEMENTS.md#materialized-views--incremental-maintenance -->
