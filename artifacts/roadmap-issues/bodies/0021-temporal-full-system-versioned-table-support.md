### Context

This issue implements the roadmap item 'Full System-Versioned Table Support' for the temporal domain. It is sourced from the consolidated roadmap under 🟠 High Priority — Immediate (≤ v1.4.0) and targets milestone v1.1.0.

Primary detail section: Full System-Versioned Table Support

### Goal

Deliver the scoped changes for Full System-Versioned Table Support in src/temporal/ and complete the linked detail section in a release-ready state for v1.1.0.

### Detailed Scope

### Full System-Versioned Table Support
**Priority:** High  
**Target Version:** v1.1.0

Complete implementation of SQL:2011 temporal table standard.

**Features:**
- Automatic history table creation and management
- Transparent version tracking on all DML operations
- System-generated transaction time columns
- Efficient storage of historical versions
- Integration with table DDL operations

**Implementation:**
```cpp
class SystemVersionedTable {
public:
    struct Config {
        std::string history_table_name;
        bool compress_history = true;
        std::chrono::seconds retention_period{365 * 24 * 3600}; // 1 year
        bool track_user_id = true;
    };
    
    // Create system-versioned table
    Result<bool> createVersionedTable(
        const std::string& table_name,
        const TableSchema& schema,
        const Config& config
    );
    
    // Insert with automatic versioning
    Result<bool> insert(
        const std::string& table_name,
        const Document& doc
    );
    
    // Update with version tracking
    Result<bool> update(
        const std::string& table_name,
        const std::string& key,
        const Document& updates
    );
    
    // Delete with soft-delete in history
    Result<bool> deleteRow(
        const std::string& table_name,
        const std::string& key
    );
};
```

**DDL Syntax:**
```sql
CREATE TABLE employees (
    id INTEGER PRIMARY KEY,
    name VARCHAR(100),
    salary DECIMAL(10,2),
    department VARCHAR(50),
    sys_start TIMESTAMP GENERATED ALWAYS AS ROW START,
    sys_end TIMESTAMP GENERATED ALWAYS AS ROW END,
    PERIOD FOR SYSTEM_TIME (sys_start, sys_end)
)
WITH SYSTEM VERSIONING;
```

**Expected Performance:**
- History table write overhead: <15%
- History table storage overhead: 2-3x with compression
- Time-travel query performance: 80-95% of current table queries

---

### Acceptance Criteria

- [ ] Automatic history table creation and management
- [ ] Transparent version tracking on all DML operations
- [ ] System-generated transaction time columns
- [ ] Efficient storage of historical versions
- [ ] Integration with table DDL operations
- [ ] History table write overhead: <15%
- [ ] History table storage overhead: 2-3x with compression
- [ ] Time-travel query performance: 80-95% of current table queries

### Relationships

- Roadmap row: #21 (🟠 High Priority — Immediate (≤ v1.4.0))
- Depends on: none identified during generation.
- Part of: consolidated roadmap delivery tracking.

### References

- src/ROADMAP.md
- src/temporal/FUTURE_ENHANCEMENTS.md#full-system-versioned-table-support
- Source key: roadmap:21:temporal:v1.1.0:full-system-versioned-table-support

Generated from the consolidated source roadmap. Keep the roadmap and issue in sync when scope changes.

<!-- roadmap-source-key: roadmap:21:temporal:v1.1.0:full-system-versioned-table-support -->
<!-- roadmap-ref: row=21;module=temporal;target=v1.1.0 -->
<!-- roadmap-detail: src/temporal/FUTURE_ENHANCEMENTS.md#full-system-versioned-table-support -->
