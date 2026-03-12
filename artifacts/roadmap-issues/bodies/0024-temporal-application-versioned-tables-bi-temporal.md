### Context

This issue implements the roadmap item 'Application-Versioned Tables (Bi-Temporal)' for the temporal domain. It is sourced from the consolidated roadmap under 🟠 High Priority — Immediate (≤ v1.4.0) and targets milestone v1.2.0.

Primary detail section: Application-Versioned Tables (Bi-Temporal)

### Goal

Deliver the scoped changes for Application-Versioned Tables (Bi-Temporal) in src/temporal/ and complete the linked detail section in a release-ready state for v1.2.0.

### Detailed Scope

### Application-Versioned Tables (Bi-Temporal)
**Priority:** High  
**Target Version:** v1.2.0

Support for user-defined valid time periods alongside system transaction time.

**Features:**
- User-controlled valid time periods
- Bi-temporal queries (transaction time + valid time)
- Temporal foreign keys
- Temporal uniqueness constraints
- Gap and overlap detection

**Implementation:**
```cpp
class ApplicationVersionedTable {
public:
    struct ValidTimePeriod {
        std::chrono::system_clock::time_point valid_from;
        std::chrono::system_clock::time_point valid_to;
        
        bool overlaps(const ValidTimePeriod& other) const;
        bool contains(const std::chrono::system_clock::time_point& t) const;
    };
    
    // Insert with valid time period
    Result<bool> insertWithValidTime(
        const std::string& table_name,
        const Document& doc,
        const ValidTimePeriod& period
    );
    
    // Update affecting specific valid time range
    Result<bool> updateForPeriod(
        const std::string& table_name,
        const std::string& key,
        const Document& updates,
        const ValidTimePeriod& period
    );
    
    // Detect overlapping periods
    Result<std::vector<Document>> findOverlappingPeriods(
        const std::string& table_name,
        const std::string& key,
        const ValidTimePeriod& period
    );
};
```

**DDL Syntax:**
```sql
CREATE TABLE contracts (
    contract_id INTEGER PRIMARY KEY,
    customer_id INTEGER,
    amount DECIMAL(10,2),
    valid_from DATE,
    valid_to DATE,
    PERIOD FOR APPLICATION_TIME (valid_from, valid_to),
    sys_start TIMESTAMP GENERATED ALWAYS AS ROW START,
    sys_end TIMESTAMP GENERATED ALWAYS AS ROW END,
    PERIOD FOR SYSTEM_TIME (sys_start, sys_end)
)
WITH SYSTEM VERSIONING;
```

---

### Acceptance Criteria

- [ ] User-controlled valid time periods
- [ ] Bi-temporal queries (transaction time + valid time)
- [ ] Temporal foreign keys
- [ ] Temporal uniqueness constraints
- [ ] Gap and overlap detection

### Relationships

- Roadmap row: #24 (🟠 High Priority — Immediate (≤ v1.4.0))
- Depends on: none identified during generation.
- Part of: consolidated roadmap delivery tracking.

### References

- src/ROADMAP.md
- src/temporal/FUTURE_ENHANCEMENTS.md#application-versioned-tables-bi-temporal
- Source key: roadmap:24:temporal:v1.2.0:application-versioned-tables-bi-temporal

Generated from the consolidated source roadmap. Keep the roadmap and issue in sync when scope changes.

<!-- roadmap-source-key: roadmap:24:temporal:v1.2.0:application-versioned-tables-bi-temporal -->
<!-- roadmap-ref: row=24;module=temporal;target=v1.2.0 -->
<!-- roadmap-detail: src/temporal/FUTURE_ENHANCEMENTS.md#application-versioned-tables-bi-temporal -->
