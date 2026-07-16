### Context

This issue implements the roadmap item 'Logical Replication' for the replication domain. It is sourced from the consolidated roadmap under 🟠 High Priority — Near-term (v1.5.0 – v1.8.0) and targets milestone v1.7.0.

Primary detail section: Logical Replication

### Goal

Deliver the scoped changes for Logical Replication in src/replication/ and complete the linked detail section in a release-ready state for v1.7.0.

### Detailed Scope

### Logical Replication
**Priority:** High  
**Target Version:** v1.7.0

Replace physical WAL-based replication with logical replication that replicates operations at a higher level, enabling cross-version replication and selective replication.

**Features:**
- Schema-aware replication (replicate DDL changes)
- Selective table/collection replication with filters
- Cross-version replication (v1.5 → v1.6)
- Data transformation during replication
- Conflict-free initial sync for new replicas

**Architecture:**
```cpp
class LogicalReplicationManager {
public:
    struct ReplicationFilter {
        std::vector<std::string> include_collections;
        std::vector<std::string> exclude_collections;
        std::string row_filter_expression;  // AQL expression
        bool replicate_ddl = true;
        bool replicate_dml = true;
    };
    
    struct LogicalReplicationSlot {
        std::string slot_name;
        uint64_t restart_lsn;
        uint64_t confirmed_flush_lsn;
        std::string plugin_name;
        ReplicationFilter filter;
    };
    
    // Create replication slot
    LogicalReplicationSlot createSlot(
        const std::string& slot_name,
        const std::string& output_plugin,
        const ReplicationFilter& filter = {}
    );
    
    // Stream changes from slot
    std::vector<LogicalChange> readChanges(
        const std::string& slot_name,
        uint32_t max_changes = 1000
    );
    
    // Advance slot position (ack)
    void advanceSlot(const std::string& slot_name, uint64_t lsn);
};

struct LogicalChange {
    enum Type { INSERT, UPDATE, DELETE, TRUNCATE, DDL };
    Type type;
    std::string collection;
    std::string schema_version;
    nlohmann::json old_data;  // For UPDATE/DELETE
    nlohmann::json new_data;  // For INSERT/UPDATE
    std::string ddl_statement;  // For DDL
    uint64_t lsn;
    std::chrono::system_clock::time_point timestamp;
};

// Example: Selective replication
ReplicationFilter filter;
filter.include_collections = {"orders", "customers"};
filter.row_filter_expression = "tenant_id == 'acme-corp'";

auto slot = logical_repl.createSlot("acme_replica", "json_output", filter);

// Consumer reads changes
while (true) {
    auto changes = logical_repl.readChanges("acme_replica", 1000);
    for (const auto& change : changes) {
        remote_storage.apply(change);
    }
    logical_repl.advanceSlot("acme_replica", changes.back().lsn);
}
```

**Benefits:**
- Replicate only relevant data (reduce bandwidth and storage)
- Enable multi-tenant replication (separate replica per tenant)
- Easier upgrades (replicate from old version to new version)
- Integrate with external systems (Kafka, Elasticsearch, Snowflake)

**Implementation Notes:**
- Use output plugins for different formats (JSON, Protobuf, Avro)
- Maintain replication slots persistently
- Support parallel decoding for high throughput

---

### Acceptance Criteria

- [ ] Schema-aware replication (replicate DDL changes)
- [ ] Selective table/collection replication with filters
- [ ] Cross-version replication (v1.5 → v1.6)
- [ ] Data transformation during replication
- [ ] Conflict-free initial sync for new replicas
- [ ] Replicate only relevant data (reduce bandwidth and storage)
- [ ] Enable multi-tenant replication (separate replica per tenant)
- [ ] Easier upgrades (replicate from old version to new version)
- [ ] Integrate with external systems (Kafka, Elasticsearch, Snowflake)
- [ ] Use output plugins for different formats (JSON, Protobuf, Avro)
- [ ] Maintain replication slots persistently
- [ ] Support parallel decoding for high throughput

### Relationships

- Roadmap row: #95 (🟠 High Priority — Near-term (v1.5.0 – v1.8.0))
- Depends on: none identified during generation.
- Part of: consolidated roadmap delivery tracking.

### References

- src/ROADMAP.md
- src/replication/FUTURE_ENHANCEMENTS.md#logical-replication
- Source key: roadmap:95:replication:v1.7.0:logical-replication

Generated from the consolidated source roadmap. Keep the roadmap and issue in sync when scope changes.

<!-- roadmap-source-key: roadmap:95:replication:v1.7.0:logical-replication -->
<!-- roadmap-ref: row=95;module=replication;target=v1.7.0 -->
<!-- roadmap-detail: src/replication/FUTURE_ENHANCEMENTS.md#logical-replication -->
