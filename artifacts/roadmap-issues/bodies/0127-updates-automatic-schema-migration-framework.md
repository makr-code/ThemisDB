### Context

This issue implements the roadmap item 'Automatic Schema Migration Framework' for the updates domain. It is sourced from the consolidated roadmap under 🟠 High Priority — Near-term (v1.5.0 – v1.8.0) and targets milestone v1.7.0.

Primary detail section: Automatic Schema Migration Framework

### Goal

Deliver the scoped changes for Automatic Schema Migration Framework in src/updates/ and complete the linked detail section in a release-ready state for v1.7.0.

### Detailed Scope

### Automatic Schema Migration Framework
**Priority:** High  
**Target Version:** v1.7.0

Automated schema migration with online DDL (zero-downtime schema changes).

**Features:**
- Schema versioning and tracking
- Online DDL (background schema changes)
- Automatic backfill for new columns
- Index rebuilding without downtime
- Dual-write during migration
- Rollback capability for schema changes

**Migration DSL:**
```cpp
SchemaMigration migration("1.5.0");

// Add column
migration.addColumn("users", {
    .name = "phone_number",
    .type = "VARCHAR(20)",
    .nullable = true,
    .default_value = "NULL"
});

// Rename column
migration.renameColumn("users", "email", "email_address");

// Add index (online)
migration.addIndex("users", {
    .name = "idx_email",
    .columns = {"email_address"},
    .unique = false,
    .build_online = true  // Build in background
});

// Drop column (after grace period)
migration.dropColumn("users", "old_column", {
    .grace_period = std::chrono::hours(24 * 7)  // 7 days
});

// Custom migration logic
migration.addCustomMigration([](MigrationContext& ctx) {
    // Migrate data manually
    auto it = ctx.storage->createIterator("users");
    while (it->valid()) {
        auto data = it->value();
        // Transform data
        ctx.storage->put(it->key(), transformed_data);
        it->next();
    }
    return true;
});

// Apply migration
auto result = migration.apply(storage_engine);
```

**Online DDL Algorithm:**
```
1. Create shadow table with new schema
2. Start dual-write (write to both tables)
3. Background copy old table to shadow table
4. Verify data consistency
5. Atomic swap (rename shadow → main)
6. Drop old table
```

**Rollback Strategy:**
```cpp
// Automatic rollback if migration fails
migration.setRollbackStrategy(RollbackStrategy::AUTOMATIC);

// Manual rollback
if (!migration_result.success) {
    migration.rollback();
}
```

---

### Acceptance Criteria

- [ ] Schema versioning and tracking
- [ ] Online DDL (background schema changes)
- [ ] Automatic backfill for new columns
- [ ] Index rebuilding without downtime
- [ ] Dual-write during migration
- [ ] Rollback capability for schema changes

### Relationships

- Roadmap row: #127 (🟠 High Priority — Near-term (v1.5.0 – v1.8.0))
- Depends on: none identified during generation.
- Part of: consolidated roadmap delivery tracking.

### References

- src/ROADMAP.md
- src/updates/FUTURE_ENHANCEMENTS.md#automatic-schema-migration-framework
- Source key: roadmap:127:updates:v1.7.0:automatic-schema-migration-framework

Generated from the consolidated source roadmap. Keep the roadmap and issue in sync when scope changes.

<!-- roadmap-source-key: roadmap:127:updates:v1.7.0:automatic-schema-migration-framework -->
<!-- roadmap-ref: row=127;module=updates;target=v1.7.0 -->
<!-- roadmap-detail: src/updates/FUTURE_ENHANCEMENTS.md#automatic-schema-migration-framework -->
