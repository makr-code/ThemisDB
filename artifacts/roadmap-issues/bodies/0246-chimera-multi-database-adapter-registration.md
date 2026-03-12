### Context

This issue implements the roadmap item 'Multi-Database Adapter Registration' for the chimera domain. It is sourced from the consolidated roadmap under 🟢 Low Priority — Future (v1.9.0+) and targets milestone v1.3.0.

Primary detail section: Multi-Database Adapter Registration

### Goal

Deliver the scoped changes for Multi-Database Adapter Registration in src/chimera/ and complete the linked detail section in a release-ready state for v1.3.0.

### Detailed Scope

### Multi-Database Adapter Registration
**Priority:** Low  
**Target Version:** v1.3.0

Support for multiple database backends in a single benchmark run.

**Features:**
- Register multiple implementations of same system
- Version-specific adapters
- Fallback adapter selection
- Adapter capability negotiation

**API:**
```cpp
// Register versioned adapters
AdapterFactory::register_adapter(
    "PostgreSQL:14",
    []() { return std::make_unique<PostgreSQL14Adapter>(); }
);

AdapterFactory::register_adapter(
    "PostgreSQL:15",
    []() { return std::make_unique<PostgreSQL15Adapter>(); }
);

AdapterFactory::register_adapter(
    "PostgreSQL:16",
    []() { return std::make_unique<PostgreSQL16Adapter>(); }
);

// Create specific version
auto pg14 = AdapterFactory::create("PostgreSQL:14");

// Create with fallback
auto pg_latest = AdapterFactory::create_with_fallback(
    {"PostgreSQL:16", "PostgreSQL:15", "PostgreSQL:14"}
);
```

---

### Acceptance Criteria

- [ ] Register multiple implementations of same system
- [ ] Version-specific adapters
- [ ] Fallback adapter selection
- [ ] Adapter capability negotiation

### Relationships

- Roadmap row: #246 (🟢 Low Priority — Future (v1.9.0+))
- Depends on: none identified during generation.
- Part of: consolidated roadmap delivery tracking.

### References

- src/ROADMAP.md
- src/chimera/FUTURE_ENHANCEMENTS.md#multi-database-adapter-registration
- Source key: roadmap:246:chimera:v1.3.0:multi-database-adapter-registration

Generated from the consolidated source roadmap. Keep the roadmap and issue in sync when scope changes.

<!-- roadmap-source-key: roadmap:246:chimera:v1.3.0:multi-database-adapter-registration -->
<!-- roadmap-ref: row=246;module=chimera;target=v1.3.0 -->
<!-- roadmap-detail: src/chimera/FUTURE_ENHANCEMENTS.md#multi-database-adapter-registration -->
