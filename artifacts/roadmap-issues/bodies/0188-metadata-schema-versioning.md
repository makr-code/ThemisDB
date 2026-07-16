### Context

This issue implements the roadmap item 'Schema Versioning' for the metadata domain. It is sourced from the consolidated roadmap under 🟡 Medium Priority — Near-term (v1.5.0 – v1.8.0) and targets milestone v1.8.0.

Primary detail section: Schema Versioning

### Goal

Deliver the scoped changes for Schema Versioning in src/metadata/ and complete the linked detail section in a release-ready state for v1.8.0.

### Detailed Scope

### Schema Versioning
**Priority:** Medium  
**Target Version:** v1.8.0

Track and manage schema changes over time.

**Features:**
- Schema version numbers
- Change history tracking
- Rollback support
- Migration scripts
- Compatibility checking

**Implementation:**
```cpp
class SchemaVersionManager {
public:
    Result<uint64_t> getCurrentVersion(const std::string& table_name);
    Result<bool> createSchemaVersion(const std::string& table_name);
    Result<std::vector<SchemaChange>> getChangeHistory(
        const std::string& table_name
    );
    Result<bool> rollbackToVersion(
        const std::string& table_name,
        uint64_t version
    );
};
```

---

### Acceptance Criteria

- [ ] Schema version numbers
- [ ] Change history tracking
- [ ] Rollback support
- [ ] Migration scripts
- [ ] Compatibility checking

### Relationships

- Roadmap row: #188 (🟡 Medium Priority — Near-term (v1.5.0 – v1.8.0))
- Depends on: none identified during generation.
- Part of: consolidated roadmap delivery tracking.

### References

- src/ROADMAP.md
- src/metadata/FUTURE_ENHANCEMENTS.md#schema-versioning
- Source key: roadmap:188:metadata:v1.8.0:schema-versioning

Generated from the consolidated source roadmap. Keep the roadmap and issue in sync when scope changes.

<!-- roadmap-source-key: roadmap:188:metadata:v1.8.0:schema-versioning -->
<!-- roadmap-ref: row=188;module=metadata;target=v1.8.0 -->
<!-- roadmap-detail: src/metadata/FUTURE_ENHANCEMENTS.md#schema-versioning -->
