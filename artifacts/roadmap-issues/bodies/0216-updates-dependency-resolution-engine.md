### Context

This issue implements the roadmap item 'Dependency Resolution Engine' for the updates domain. It is sourced from the consolidated roadmap under 🟡 Medium Priority — Near-term (v1.5.0 – v1.8.0) and targets milestone v1.6.0.

Primary detail section: Dependency Resolution Engine

### Goal

Deliver the scoped changes for Dependency Resolution Engine in src/updates/ and complete the linked detail section in a release-ready state for v1.6.0.

### Detailed Scope

### Dependency Resolution Engine
**Priority:** Medium  
**Target Version:** v1.6.0

Automatic resolution of update dependencies with topological sorting.

**Features:**
- Dependency graph construction
- Topological sort for correct order
- Cycle detection
- Minimum version constraints
- Conflict resolution
- Automatic backfill of missing dependencies

**Dependency Format:**
```cpp
struct Dependency {
    std::string package;                // "themis-storage"
    std::string version_constraint;     // ">=1.4.0,<2.0.0"
    bool optional = false;
    std::vector<std::string> conflicts; // Conflicting packages
};
```

**Usage:**
```cpp
DependencyResolver resolver;

// Add dependencies for version 1.5.0
resolver.addDependency("1.5.0", {
    .package = "themis-storage",
    .version_constraint = ">=1.4.0,<2.0.0"
});

resolver.addDependency("1.5.0", {
    .package = "themis-query",
    .version_constraint = ">=1.4.5"
});

// Resolve dependencies
auto resolution = resolver.resolve("1.5.0", current_versions);
if (resolution.success) {
    LOG_INFO("Update plan:");
    for (const auto& step : resolution.steps) {
        LOG_INFO("  {} {} -> {}", step.package, step.from_version, step.to_version);
    }
    
    // Execute update plan
    for (const auto& step : resolution.steps) {
        engine->applyHotReload(step.to_version);
    }
} else {
    LOG_ERROR("Dependency resolution failed: {}", resolution.error_message);
}
```

**Conflict Resolution:**
```cpp
// Detect conflicts
auto conflicts = resolver.detectConflicts({
    {"themis-storage", "1.5.0"},
    {"themis-query", "1.4.0"}  // Requires themis-storage >= 1.5.1
});

if (!conflicts.empty()) {
    LOG_ERROR("Dependency conflicts:");
    for (const auto& conflict : conflicts) {
        LOG_ERROR("  {} conflicts with {}", conflict.package1, conflict.package2);
    }
}
```

---

### Acceptance Criteria

- [ ] Dependency graph construction
- [ ] Topological sort for correct order
- [ ] Cycle detection
- [ ] Minimum version constraints
- [ ] Conflict resolution
- [ ] Automatic backfill of missing dependencies

### Relationships

- Roadmap row: #216 (🟡 Medium Priority — Near-term (v1.5.0 – v1.8.0))
- Depends on: none identified during generation.
- Part of: consolidated roadmap delivery tracking.

### References

- src/ROADMAP.md
- src/updates/FUTURE_ENHANCEMENTS.md#dependency-resolution-engine
- Source key: roadmap:216:updates:v1.6.0:dependency-resolution-engine

Generated from the consolidated source roadmap. Keep the roadmap and issue in sync when scope changes.

<!-- roadmap-source-key: roadmap:216:updates:v1.6.0:dependency-resolution-engine -->
<!-- roadmap-ref: row=216;module=updates;target=v1.6.0 -->
<!-- roadmap-detail: src/updates/FUTURE_ENHANCEMENTS.md#dependency-resolution-engine -->
